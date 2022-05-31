#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include "Hardware/RSC68k.h"
#include "BIOS/OS.h"
#include "Shared/Version.h"
#include "Shared/LineInput.h"
#include "Shared/lex.h"
#include "Shared/16550.h"
#include "Shared/sbrk.h"
#include "Shared/dis68k.h"
#include "BIOS/BIOS.h"

// Maximum # of characters per input line
#define	MONITOR_LINE_MAX		256

// Depth of line history (in lines)
#define	MONITOR_HISTORY_DEPTH	20

// # Of lines to dump/disassemble
static uint16_t sg_u16DumpLines = 8;

static const char *sg_eHexTable = "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f202122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f404142434445464748494a4b4c4d4e4f505152535455565758595a5b5c5d5e5f606162636465666768696a6b6c6d6e6f707172737475767778797a7b7c7d7e7f808182838485868788898a8b8c8d8e8f909192939495969798999a9b9c9d9e9fa0a1a2a3a4a5a6a7a8a9aaabacadaeafb0b1b2b3b4b5b6b7b8b9babbbcbdbebfc0c1c2c3c4c5c6c7c8c9cacbcccdcecfd0d1d2d3d4d5d6d7d8d9dadbdcdddedfe0e1e2e3e4e5e6e7e8e9eaebecedeeeff0f1f2f3f4f5f6f7f8f9fafbfcfdfeff";

typedef struct SMonitorCommands
{
	char *peCommandString;
	char *peDescription;
	EStatus (*Handler)(SLex *psLex,
					   const struct SMonitorCommands *psMonitorCommand,
					   uint32_t *pu32Address);
} SMonitorCommands;

// Wait for a character to be typed from the console
static char MonitorInputGet(void)
{
	char eChar;
	int s32Result;

	s32Result = read(0,
					 &eChar,
					 sizeof(eChar));
	assert(sizeof(eChar) == s32Result);
	return(eChar);
}

// Output a character to the console
static void MonitorOutput(char *peText,
						  uint16_t u16Length)
{
	(void) write(0,
				 peText,
				 (int) u16Length);
}

// Returns TRUE if there's a console character available
static bool MonitorInputAvailable(void)
{
	return(SerialDataAvailablePIO((S16550UART*) RSC68KHW_DEVCOM_UARTA));
}

// Used to track lexical state
typedef struct SMonitorLineState
{
	char *peLineText;
	char *peLinePointer;
	uint16_t u16LineLength;
} SMonitorLineState;

// Lexical "open" callback
static EStatus MonitorOpenStream(SLex *psLex,
								 void *pvStreamData)
{
	SMonitorLineState *psLineState = (SMonitorLineState *) pvStreamData;

	// All good
	psLineState->peLinePointer = psLineState->peLineText;
	return(ESTATUS_OK);
}

static EStatus MonitorReadStream(SLex *psLex,
								 void *pvBuffer,
								 uint32_t u32BytesToRead,
								 uint32_t *pu32BytesRead,
								 void *pvStreamData)
{
	SMonitorLineState *psLineState = (SMonitorLineState *) pvStreamData;
	uint16_t u16Position;
	uint16_t u16Remaining;

	// Figure out how far along we are in the stream
	u16Position = (uint16_t) (((uint32_t) psLineState->peLinePointer) - ((uint32_t) psLineState->peLineText));

	u16Remaining = psLineState->u16LineLength - u16Position;
	if (u32BytesToRead > u16Remaining)
	{
		u32BytesToRead = u16Remaining;
	}

	// If we have data to give, copy it
	if (u32BytesToRead)
	{
		memcpy(pvBuffer, (void *) psLineState->peLinePointer, (size_t) u32BytesToRead);
		psLineState->peLinePointer += u32BytesToRead;
	}

	*pu32BytesRead = u32BytesToRead;
	return(ESTATUS_OK);
}

// Called when the lexical stream is closed
static EStatus MonitorCloseStream(SLex *psLex,
								  void **ppvStreamData)
{
	if (NULL == *ppvStreamData)
	{
		free(*ppvStreamData);
		*ppvStreamData = NULL;
	}

	return(ESTATUS_OK);
}

static const SLexStreamFunctions sg_sMonitorLexStreamFunctions =
{
	MonitorOpenStream,		// Open
	MonitorReadStream,		// Read
	MonitorCloseStream		// Close
};

// Checks to see if an address is within the CPU's execution range
static EStatus MonitorCheckAddress(char *peAddressType,
								   uint32_t u32Address)
{
	EStatus eStatus = ESTATUS_OK;

	if (u32Address > 0xffffff)
	{
		printf("%s0x%.8x is beyond the CPU's address space\r\n", peAddressType, u32Address);
		eStatus = ESTATUS_MONITOR_ADDRESS_TOO_HIGH;
	}   								   

	return(eStatus);
}

// Dumps memory contents. Accepts the following:
//
// d 					Dump at pointer for x number of lines)
// d address 			Set pointer to address and dump for # of lines)
// d address count 	 	Set pointer to address and dump for count bytes
// d address-address	Dump from adddress to address
static EStatus MonitorDumpMemory(SLex *psLex,
								 const SMonitorCommands *psMonitorCommand,
								 uint32_t *pu32AddressPointer)
{
	EStatus eStatus;
	uint8_t u8LineBufferPrior[16];
	uint8_t u8LineBuffer[16];
	uint32_t u32AddressFrom;
	uint32_t u32AddressTo;
	SToken sToken;
	ETokenType eToken;
	uint32_t u32Loop;
	bool bLineBufferPrimed = false;
	bool bRepeatActive = false;

	ZERO_STRUCT(sToken);
	u32AddressFrom = *pu32AddressPointer;

	eToken = LexGetNextToken(psLex,
							 &sToken);
	if (ELEX_EOF == eToken)
	{
		// Just the dump by itself. So we go for the # of lines 
		u32AddressTo = u32AddressFrom + (sg_u16DumpLines << 4);

		// Clip it
		if (u32AddressTo > 0xffffff)
		{
			u32AddressTo = 0xffffff;
		}
	}
	else
	if ((ELEX_INT_SIGNED == eToken) ||
		(ELEX_INT_UNSIGNED == eToken) ||
		('.' == eToken))
	{
		if ('.' == eToken)
		{
			// From address is already set to the pointer
		}
		else
		{
			// We have (at least) a start address
			u32AddressFrom = (uint32_t) sToken.uData.u64IntValue;
		}

		LexClearToken(&sToken);

		// Let's see if there's more
		eToken = LexGetNextToken(psLex,
								 &sToken);
		if (ELEX_EOF == eToken)
		{
			// It's just the address. Dump a line count full.
			u32AddressTo = u32AddressFrom + (sg_u16DumpLines << 4);

			// Clip it
			if (u32AddressTo > 0xffffff)
			{
				u32AddressTo = 0xffffff;
			}
		}
		else
		if ((ELEX_INT_SIGNED == eToken) ||
			(ELEX_INT_UNSIGNED == eToken))
		{
			// It's a count of bytes!
			u32AddressTo = u32AddressFrom + ((uint32_t) sToken.uData.u64IntValue);

			// Clip it
			if (u32AddressTo > 0xffffff)
			{
				u32AddressTo = 0xffffff;
			}
		}
		else
		if ('-' == eToken)
		{
			// It's a range
			LexClearToken(&sToken);
			
			eToken = LexGetNextToken(psLex,
									 &sToken);

			// Next should be a number of some sort
			if (ELEX_INT_SIGNED == eToken)
			{
				u32AddressTo = ((uint32_t) (-sToken.uData.s64IntValue));
			}
			else
			if (ELEX_INT_UNSIGNED == eToken)
			{
				u32AddressTo = ((uint32_t) sToken.uData.u64IntValue);
			}
			else
			{
				printf("Error: Expecting dump address-address\r\n");
				eStatus = ESTATUS_PARSE_UINT32_EXPECTED;
				goto errorExit;
			}
		}
		else
		{
			printf("Error: Expecting either dump address, dump address count, or dump address-address\r\n");
			eStatus = ESTATUS_PARSE_UINT32_EXPECTED;
			goto errorExit;
		}
	}
	else
	{
		printf("Error: Expecting dump address\r\n");
		eStatus = ESTATUS_PARSE_UINT32_EXPECTED;
		goto errorExit;
	}

	// If end address is ahead of the start address...
	if (u32AddressTo < u32AddressFrom)
	{
		printf("Start address of 0x%.6x is larger than end address of 0x%.6x\r\n", u32AddressFrom, u32AddressTo);
		eStatus = ESTATUS_MONITOR_START_LARGER_THAN_END_ADDRESS;
		goto errorExit;
	}

	eStatus = MonitorCheckAddress("From address", u32AddressFrom);
	ERR_GOTO();
	eStatus = MonitorCheckAddress("To address", u32AddressTo);
	ERR_GOTO();

	// Quantize to 16 byte increments as we dump
	u32Loop = u32AddressFrom & ~(sizeof(u8LineBuffer) - 1);

	while (u32Loop < u32AddressTo)
	{
		char eDumpLine[90];
		uint8_t u8ColumnCounter;
		char *pePtr;

		// Start the hex dump counter 
		pePtr = eDumpLine;
		memcpy((void *) u8LineBuffer, (void *) u32Loop, sizeof(u8LineBuffer));

		if (bLineBufferPrimed)
		{
			if (false == bRepeatActive)
			{
				// If repeat isn't active, let's see if it's the same as what we had before.
				// If it is, just display a * and keep going until we find the end
				if (memcmp(u8LineBuffer, u8LineBufferPrior, sizeof(u8LineBuffer)) == 0)
				{
					printf("*\r\n");
					bRepeatActive = true;
				}
			}
			else
			{
				// If repeat is active and we're no longer the same as the last line of
				// data, cancel the repeat so we start displaying data again 
				if (memcmp(u8LineBuffer, u8LineBufferPrior, sizeof(u8LineBuffer)))
				{
					bRepeatActive = false;
				}
			}
		}

		// Only display the actual 
		if (false == bRepeatActive)
		{
			u8ColumnCounter = 0;
			while (u8ColumnCounter < sizeof(u8LineBuffer))
			{
				if ((u32Loop < u32AddressFrom) ||
					(u32Loop >= u32AddressTo))
				{
					// We're out of range, so ignore this
					*pePtr = ' ';
					++pePtr;
					*pePtr = ' ';
					++pePtr;
				}
				else
				{
					const char *peHex;

					// Point to the hext digits
					peHex = &sg_eHexTable[u8LineBuffer[u8ColumnCounter] << 1];
					*pePtr = *peHex;
					++pePtr;
					++peHex;
					*pePtr = *peHex;
					++pePtr;
				}

				*pePtr = ' ';
				++pePtr;
				if (7 == u8ColumnCounter)
				{
					// Add an additional space for the dual 8 byte dumps
					*pePtr = ' ';
					++pePtr;
				}

				++u32Loop;
				++u8ColumnCounter;
			}

			*pePtr = ' ';
			++pePtr;
			*pePtr = ' ';
			++pePtr;

			// Now some ASCII
			u8ColumnCounter = 0;
			u32Loop -= sizeof(u8LineBuffer);

			while (u8ColumnCounter < sizeof(u8LineBuffer))
			{
				if ((u32Loop < u32AddressFrom) ||
					(u32Loop >= u32AddressTo))
				{
					// We're out of range, so ignore this
					*pePtr = ' ';
					++pePtr;
				}
				else
				{
					if ((u8LineBuffer[u8ColumnCounter] < ' ') ||
						(u8LineBuffer[u8ColumnCounter] > 0x7f))
					{
						*pePtr = '.';
					}
					else
					{
						*pePtr = u8LineBuffer[u8ColumnCounter];
					}

					++pePtr;
				}

				++u32Loop;
				++u8ColumnCounter;
			}

			// Null terminate the string
			*pePtr = '\0';

			// Dump to the console
			printf("%.6x: %s\r\n", u32Loop - sizeof(u8LineBuffer), eDumpLine);
		}
		else
		{
			u32Loop += sizeof(u8LineBuffer);
		}

		// Look for input of any kind, and if we get it, stop the 
		if (MonitorInputAvailable())
		{
			// Eat whatever character they just sent us
			(void) MonitorInputGet();
			// Set the pointer wherever we've stopped
			u32AddressTo = u32Loop;
			break;
		}

		// Make a copy for the next 
		memcpy((void *) u8LineBufferPrior, (void *) u8LineBuffer, sizeof(u8LineBufferPrior));
		bLineBufferPrimed = true;
	}

	// Record the address pointer since it moved
	*pu32AddressPointer = u32AddressTo;
	eStatus = ESTATUS_OK;

errorExit:
	return(eStatus);
}

// Show stack and frame pointer info
static EStatus MonitorStack(SLex *psLex,
						    const SMonitorCommands *psMonitorCommand,
						    uint32_t *pu32AddressPointer)
{
	uint32_t u32HeapEnd;
	uint32_t u32LowestStack;
	uint32_t u32FramePointer;

	SBRKGetInfo(&u32HeapEnd,
				&u32LowestStack,
				&u32FramePointer);

	printf("Frame pointer=0x%.6x\r\n", u32FramePointer);
	printf("Lowest stack =0x%.6x\r\n", u32LowestStack);
	printf("Heap end     =0x%.6x\r\n", u32HeapEnd);
	if (u32HeapEnd < u32LowestStack)
	{
		printf("Detected free=%u bytes\r\n", u32LowestStack - u32HeapEnd);
	}
	else
	{
		printf("Detected free=**COLLISION - OUT OF MEMORY**\r\n");
	}

	return(ESTATUS_OK);
}

// Fill memory with a byte pattern
//
// fill addrstart count pattern
// fill addrstart-addrend pattern

static EStatus MonitorFill(SLex *psLex,
						   const SMonitorCommands *psMonitorCommand,
						   uint32_t *pu32AddressPointer)
{
	EStatus eStatus;
	uint32_t u32AddressFrom;
	uint32_t u32AddressTo;
	SToken sToken;
	ETokenType eToken;
	bool bRange = false;
	uint8_t u8Pattern;

	ZERO_STRUCT(sToken);
	u32AddressFrom = *pu32AddressPointer;

	eToken = LexGetNextToken(psLex,
							 &sToken);
	if ((ELEX_INT_SIGNED == eToken) ||
		(ELEX_INT_UNSIGNED == eToken) ||
		('.' == eToken))
	{
		if ('.' == eToken)
		{
			// From address is already set to the pointer
		}
		else
		{
			// We have (at least) a start address
			u32AddressFrom = (uint32_t) sToken.uData.u64IntValue;
		}

		LexClearToken(&sToken);
	}
	else
	{
		printf("Error: Expecting start address\r\n");
		eStatus = ESTATUS_PARSE_UINT32_EXPECTED;
		goto errorExit;
	}

	eToken = LexGetNextToken(psLex,
							 &sToken);
	if ((ETokenType) '-' == eToken)
	{
		// It's a range
		bRange = true;
		LexClearToken(&sToken);

		// Next should be a count
		eToken = LexGetNextToken(psLex,
								 &sToken);
		if ((ELEX_INT_SIGNED == eToken) ||
			(ELEX_INT_UNSIGNED == eToken))
		{
			u32AddressTo = (uint32_t) sToken.uData.u64IntValue;
		}
		else
		{
			printf("Error: Expecting addfrom-addrto\r\n");
			eStatus = ESTATUS_PARSE_UINT32_EXPECTED;
			goto errorExit;
		}
	}
	else
	if ((ELEX_INT_SIGNED == eToken) ||
		(ELEX_INT_UNSIGNED == eToken))
	{
		// It's a count.
		u32AddressTo = u32AddressFrom + ((uint32_t) sToken.uData.u64IntValue);
	}
	else
	{
		printf("Error: Expecting either a range - addrfrom-addrto or a count - addrfrom count\r\n");
		eStatus = ESTATUS_PARSE_UINT32_EXPECTED;
		goto errorExit;
	}

	LexClearToken(&sToken);

	// Now get the pattern byte
	eToken = LexGetNextToken(psLex,
							 &sToken);
	if ((ELEX_INT_SIGNED == eToken) ||
		(ELEX_INT_UNSIGNED == eToken))
	{
		if (sToken.uData.u64IntValue > 0xff)
		{
			printf("Error: Fill value exceeds 8 bits\r\n");
			eStatus = ESTATUS_PARSE_UINT32_EXPECTED;
			goto errorExit;
		}

		u8Pattern = (uint8_t) sToken.uData.u64IntValue;
	}
	else
	{
		printf("Error: Missing pattern to write\r\n");
		eStatus = ESTATUS_PARSE_UINT32_EXPECTED;
		goto errorExit;
	}

	// If end address is ahead of the start address...
	if (u32AddressTo < u32AddressFrom)
	{
		printf("Error: Start address of 0x%.6x is larger than end address of 0x%.6x\r\n", u32AddressFrom, u32AddressTo);
		eStatus = ESTATUS_MONITOR_START_LARGER_THAN_END_ADDRESS;
		goto errorExit;
	}

	eStatus = MonitorCheckAddress("Error: From address of ", u32AddressFrom);
	ERR_GOTO();
	eStatus = MonitorCheckAddress("Error: To address of ", u32AddressTo);
	ERR_GOTO();

	printf("Filling 0x%.6x-0x%.6x with 0x%.2x - ", u32AddressFrom, u32AddressTo, u8Pattern);
	fflush(stdout);

	memset((void *) u32AddressFrom, u8Pattern, (u32AddressTo - u32AddressFrom) + 1);
	printf("Done\r\n");

	eStatus = ESTATUS_OK;
	*pu32AddressPointer = u32AddressTo + 1;

errorExit:
	return(eStatus);
}


// Set the # of lines of memory to dump when not qualified by count or range
static EStatus MonitorLines(SLex *psLex,
							const SMonitorCommands *psMonitorCommand,
							uint32_t *pu32AddressPointer)
{
	EStatus eStatus;
	SToken sToken;
	ETokenType eToken;

	ZERO_STRUCT(sToken);

	eToken = LexGetNextToken(psLex,
						  &sToken);
	if (ELEX_INT_UNSIGNED == eToken)
	{
		if ((0 == sToken.uData.u64IntValue) ||
			((sToken.uData.u64IntValue) > 65535))
		{
			printf("Error: Valid line count range is 1-65535\r\n");
			eStatus = ESTATUS_PARSE_UINT32_EXPECTED;
			goto errorExit;
		}

		// It's good
	}
	else
	{
		printf("Error: Expected a number\r\n");
		eStatus = ESTATUS_PARSE_UINT32_EXPECTED;
		goto errorExit;
	}

	sg_u16DumpLines = ((uint16_t) sToken.uData.u64IntValue);
	printf("Memory dump/disassembly line count set to %u\r\n", sg_u16DumpLines);
	eStatus = ESTATUS_OK;

errorExit:
	LexClearToken(&sToken);
	return(eStatus);
}

// Read transactions from memory
//
// rb addrfrom-addrto addr addr addr ...
// rw addrfrom-addrto addr addr addr ...
// rd addrfrom-addrto addr addr addr ...
static EStatus MonitorRead(SLex *psLex,
						   const SMonitorCommands *psMonitorCommand,
						   uint32_t *pu32AddressPointer)
{
	EStatus eStatus;
	uint32_t u32AddressFrom;
	uint32_t u32AddressTo;
	SToken sToken;
	ETokenType eToken;
	uint8_t u8TransactionSize;
	bool bFetchToken = true;

	ZERO_STRUCT(sToken);

	// Figure out the basic transaction size
	u8TransactionSize = toupper(psMonitorCommand->peCommandString[1]);
	if ('B' == u8TransactionSize)
	{
		// byte/uint8_t
		u8TransactionSize = sizeof(uint8_t);
	}
	else
	if ('W' == u8TransactionSize)
	{
		// word/uint16_t
		u8TransactionSize = sizeof(uint16_t);
	}
	else
	if ('D' == u8TransactionSize)
	{
		// dword/uint32_t
		u8TransactionSize = sizeof(uint32_t);
	}
	else
	{
		// Internal error.
		assert(0);
	}

	for (;;)
	{
		if (bFetchToken)
		{
			LexClearToken(&sToken);

			eToken = LexGetNextToken(psLex,
									 &sToken);
		}

		if ((ELEX_INT_SIGNED == eToken) ||
			(ELEX_INT_UNSIGNED == eToken) ||
			((ETokenType) '.' == eToken))
		{
			if ((ETokenType) '.' == eToken)
			{
				u32AddressFrom = *pu32AddressPointer;
			}
			else
			{
				u32AddressFrom = (uint32_t) sToken.uData.u64IntValue;
			}

			// Make from/to the same for now in case it's a single address
			u32AddressTo = u32AddressFrom;
		}
		else
		if (ELEX_EOF == eToken)
		{
			break;
		}
		else
		{
			printf("Error: Expecting starting address\r\n");
			eStatus = ESTATUS_PARSE_UINT32_EXPECTED;
			goto errorExit;
		}

		LexClearToken(&sToken);

		// Get the next token. If it's a '-', then we're doing and expect a range
		eToken = LexGetNextToken(psLex,
								 &sToken);
		if (((ETokenType) '-') == eToken)
		{
			// It's a range!
			bFetchToken = false;

			LexClearToken(&sToken);

			eToken = LexGetNextToken(psLex,
									 &sToken);

			if ((ELEX_INT_SIGNED == eToken) ||
				(ELEX_INT_UNSIGNED == eToken) ||
				((ETokenType) '.' == eToken))
			{
				if ((ETokenType) '.' == eToken)
				{
					u32AddressTo = *pu32AddressPointer;
				}
				else
				{
					u32AddressTo = (uint32_t) sToken.uData.u64IntValue;
				}
			}
			else
			{
				printf("Error: Expecting ending  address\r\n");
				eStatus = ESTATUS_PARSE_UINT32_EXPECTED;
				goto errorExit;
			}

			bFetchToken = true;
		}
		else
		{
			// It's not a range, 
			bFetchToken = false;
		}

		eStatus = MonitorCheckAddress("Error: From address of ", u32AddressFrom);
		ERR_GOTO();
		eStatus = MonitorCheckAddress("Error: To address of ", u32AddressTo);
		ERR_GOTO();

		while (u32AddressFrom <= u32AddressTo)
		{
			if (sizeof(uint8_t) == u8TransactionSize)
			{
				// Byte
				printf("0x%.6x=0x%.2x\r\n", u32AddressFrom, *((uint8_t *) u32AddressFrom));
			}
			else
			if (sizeof(uint16_t) == u8TransactionSize)
			{
				// Word
				printf("0x%.6x=0x%.4x\r\n", u32AddressFrom, *((uint16_t *) u32AddressFrom));
			}
			else
			if (sizeof(uint32_t) == u8TransactionSize)
			{
				// Dword
				printf("0x%.6x=0x%.8x\r\n", u32AddressFrom, *((uint32_t *) u32AddressFrom));
			}
			else
			{
				// Internal fault
				assert(0);
			}

			u32AddressFrom += u8TransactionSize;

			// Look for input of any kind, and if we get it, stop the 
			if (MonitorInputAvailable())
			{
				// Eat whatever character they just sent us
				(void) MonitorInputGet();
				// Set the pointer wherever we've stopped
				*pu32AddressPointer = u32AddressFrom;
				eStatus = ESTATUS_OK;
				goto errorExit;
			}
		}

		*pu32AddressPointer = u32AddressFrom;
	}

	eStatus = ESTATUS_OK;

errorExit:
	return(eStatus);
}

// This determines the final size of the transaction based on the supplied transaction size
// (assuming there is one) then compared to the incoming transaction size

EStatus MonitorDetermineSize(SToken *psToken,
							 uint8_t u8TransactionSizeIncoming,
							 uint8_t *pu8TransactionSizeAdjusted)
{
	EStatus eStatus;
	uint8_t u8TransactionSizeAdjusted = 0xff;

	if (0 == u8TransactionSizeIncoming)
	{
		// Determine the size of the transaction from the token
		if (ELEX_STRING == psToken->eTokenType)
		{
			// If it's a string, we're doing bytes
			u8TransactionSizeAdjusted = sizeof(uint8_t);
		}
		else
		if (psToken->u8Digits <= 2)
		{
			// Single byte
			u8TransactionSizeAdjusted = sizeof(uint8_t);
		}
		else
		if (psToken->u8Digits <= 4)
		{
			// Word
			u8TransactionSizeAdjusted = sizeof(uint16_t);
		}
		else
		{
			// Assume dword
			u8TransactionSizeAdjusted = sizeof(uint32_t);
		}
	}
	else
	{
		// Use the incoming transaction size
		u8TransactionSizeAdjusted = u8TransactionSizeIncoming;
	}

	// Assuming we've got a numeric value, see if it's in range
	if (psToken->eTokenType != ELEX_STRING)
	{
		if (sizeof(uint8_t) == u8TransactionSizeAdjusted)
		{
			if (psToken->uData.u64IntValue > 0xff)
			{
				printf("Error: Value of 0%.8x out of range for byte transaction sizes\r\n", (uint32_t) psToken->uData.u64IntValue);
				eStatus = ESTATUS_MONITOR_DATA_SIZE_OUT_OF_RANGE;
				goto errorExit;
			}
		}
		else
		if (sizeof(uint16_t) == u8TransactionSizeAdjusted)
		{
			if (psToken->uData.u64IntValue > 0xffff)
			{
				printf("Error: Value of 0%.8x out of range for word/uint16_t transaction sizes\r\n", (uint32_t) psToken->uData.u64IntValue);
				eStatus = ESTATUS_MONITOR_DATA_SIZE_OUT_OF_RANGE;
				goto errorExit;
			}
		}

	}

	assert(u8TransactionSizeAdjusted != 0xff);
	*pu8TransactionSizeAdjusted = u8TransactionSizeAdjusted;
	eStatus = ESTATUS_OK;

errorExit:
	return(eStatus);
}

// Write transactions to memory
//
// w addr=x x x x x,addr=x x x x "xxxx" x x x
// wb addr=x x x x x,addr=x x x x "xxxx" x x x
// ww addr=x x x x x,addr=x x x x "xxxx" x x x
// wd addr=x x x x x,addr=x x x x "xxxx" x x x
static EStatus MonitorWrite(SLex *psLex,
						    const SMonitorCommands *psMonitorCommand,
						    uint32_t *pu32AddressPointer)
{
	EStatus eStatus;
	uint32_t u32Address;
	SToken sToken;
	ETokenType eToken;
	uint8_t u8TransactionSize;
	bool bFetchToken = true;

	// Figure out the basic transaction size
	u8TransactionSize = toupper(psMonitorCommand->peCommandString[1]);
	if ('B' == u8TransactionSize)
	{
		// byte/uint8_t
		u8TransactionSize = sizeof(uint8_t);
	}
	else
	if ('W' == u8TransactionSize)
	{
		// word/uint16_t
		u8TransactionSize = sizeof(uint16_t);
	}
	else
	if ('D' == u8TransactionSize)
	{
		// dword/uint32_t
		u8TransactionSize = sizeof(uint32_t);
	}
	else
	if (0 == u8TransactionSize)
	{
		// This means size is derived from the arguments
	}
	else
	{
		// Internal error.
	}

	ZERO_STRUCT(sToken);

	for (;;)
	{
		uint8_t u8TransactionCount;

		u8TransactionCount = 0;

		// Get an address
		eToken = LexGetNextToken(psLex,
								 &sToken);

		if ((ELEX_INT_SIGNED == eToken) ||
			(ELEX_INT_UNSIGNED == eToken) ||
			((ETokenType) '.' == eToken))
		{
			if ((ETokenType) '.' == eToken)
			{
				u32Address = *pu32AddressPointer;
			}
			else
			{
				u32Address = (uint32_t) sToken.uData.u64IntValue;
			}
		}
		else
		{
			printf("Error: Expecting starting address\r\n");
			eStatus = ESTATUS_PARSE_UINT32_EXPECTED;
			goto errorExit;
		}

		LexClearToken(&sToken);
		*pu32AddressPointer = u32Address;

		// Now look for '='
		eToken = LexGetNextToken(psLex,
								 &sToken);
		if (eToken != ((ETokenType) '='))
		{
			printf("Error: Expecting '=' after address\r\n");
			eStatus = ESTATUS_MONITOR_EQUALS_EXPECTED;
			goto errorExit;
		}
		LexClearToken(&sToken);

		// Loop until end of line or a comma
		for (;;)
		{
			eToken = LexGetNextToken(psLex,
									 &sToken);

			if ((ELEX_EOF == eToken) ||
				(((ETokenType) ',') == eToken))
			{
				break;
			}

			// Either needs to be a number or a string
			if ((ELEX_INT_SIGNED == eToken) ||
				(ELEX_INT_UNSIGNED == eToken) ||
				(ELEX_STRING == eToken))
			{
				uint8_t u8TransactionSizeAdjusted = 0;

				// Figure out what the transaction for this data item should be
				eStatus = MonitorDetermineSize(&sToken,
											   u8TransactionSize,
											   &u8TransactionSizeAdjusted);
				ERR_GOTO();

				if (ELEX_STRING == eToken)
				{
					char *peString = sToken.peString;

					while ((peString) && (*peString))
					{
						// Single transaction.
						if (sizeof(uint8_t) == u8TransactionSizeAdjusted)
						{
							*((uint8_t *) u32Address) = *peString;
						}
						else
						if (sizeof(uint16_t) == u8TransactionSizeAdjusted)
						{
							*((uint16_t *) u32Address) = *peString;
						}
						else
						if (sizeof(uint32_t) == u8TransactionSizeAdjusted)
						{
							*((uint32_t *) u32Address) = *peString;
						}
						else
						{
							assert(0);
						}

						u32Address += u8TransactionSizeAdjusted;
						++peString;
					}
				}
				else
				{
					// Single transaction.
					if (sizeof(uint8_t) == u8TransactionSizeAdjusted)
					{
						*((uint8_t *) u32Address) = (uint8_t) sToken.uData.u64IntValue;
					}
					else
					if (sizeof(uint16_t) == u8TransactionSizeAdjusted)
					{
						*((uint16_t *) u32Address) = (uint16_t) sToken.uData.u64IntValue;
					}
					else
					if (sizeof(uint32_t) == u8TransactionSizeAdjusted)
					{
						*((uint32_t *) u32Address) = (uint32_t) sToken.uData.u64IntValue;
					}
					else
					{
						assert(0);
					}

					u32Address += u8TransactionSizeAdjusted;
				}

				*pu32AddressPointer = u32Address;
				LexClearToken(&sToken);
			}
			else
			if (ELEX_IDENTIFIER == eToken)
			{
				printf("Error: Expected numeric value or string, not '%s'\r\n", sToken.peString);
				eStatus = ESTATUS_MONITOR_DATA_EXPECTED;
				goto errorExit;
			}
			else
			{
				printf("Error: Expected numeric value or string, not '%c'\r\n", eToken);
				eStatus = ESTATUS_MONITOR_DATA_EXPECTED;
				goto errorExit;
			}

			++u8TransactionCount;
		}

		LexClearToken(&sToken);

		if (0 == u8TransactionCount)
		{
			printf("Error: Missing data for target address of 0x%.6\r\n", u32Address);
			eStatus = ESTATUS_MONITOR_DATA_EXPECTED;
			goto errorExit;
		}

		if (ELEX_EOF == eToken)
		{
			break;
		}
		else
		{
			// Otherwise, it's a , so we continue
		}
	}

	eStatus = ESTATUS_OK;

errorExit:
	LexClearToken(&sToken);
	return(eStatus);
}


// "call" To an address
static EStatus MonitorGo(SLex *psLex,
						 const SMonitorCommands *psMonitorCommand,
						 uint32_t *pu32AddressPointer)
{
	EStatus eStatus;
	uint32_t u32AddressGo;
	SToken sToken;
	ETokenType eToken;

	ZERO_STRUCT(sToken);
	eToken = LexGetNextToken(psLex,
							 &sToken);

	if ((ELEX_INT_SIGNED == eToken) ||
		(ELEX_INT_UNSIGNED == eToken) ||
		((ETokenType) '.' == eToken))
	{
		if ((ETokenType) '.' == eToken)
		{
			u32AddressGo = *pu32AddressPointer;
		}
		else
		{
			u32AddressGo = (uint32_t) sToken.uData.u64IntValue;
		}
	}
	else
	{
		printf("Error: Expected target address\r\n");
		eStatus = ESTATUS_MONITOR_ADDRESS_EXPECTED;
		goto errorExit;
	}

	eStatus = MonitorCheckAddress("Error: Execution address ", u32AddressGo);
	ERR_GOTO();

	printf("Calling to address 0x%.6x:\r\n", u32AddressGo);
	fflush(stdout);
	((void (*)(void))u32AddressGo)();
	printf("\r\nReturning from call to 0x%.6x\r\n", u32AddressGo);
	eStatus = ESTATUS_OK;

errorExit:
	return(eStatus);
}

// Disassembles code.
//
// dis
// dis addr
// dis addrfrom-addrto
static EStatus MonitorDisassemble(SLex *psLex,
								  const SMonitorCommands *psMonitorCommand,
								  uint32_t *pu32AddressPointer)
{
	EStatus eStatus;
	uint32_t u32AddressFrom;
	uint32_t u32AddressTo;
	SToken sToken;
	ETokenType eToken;
	bool bRange = false;
	uint8_t u8Pattern;
	uint16_t u16LineCount = sg_u16DumpLines;

	ZERO_STRUCT(sToken);
	u32AddressFrom = *pu32AddressPointer;

	eToken = LexGetNextToken(psLex,
							 &sToken);
	if (ELEX_EOF == eToken)
	{
		// Disassemble at the address pointer for u16LineCount lines
	}
	else
	{
		if ((ELEX_INT_SIGNED == eToken) ||
			(ELEX_INT_UNSIGNED == eToken) ||
			('.' == eToken))
		{
			if ('.' == eToken)
			{
				// From address is already set to the pointer
			}
			else
			{
				// We have (at least) a start address
				u32AddressFrom = (uint32_t) sToken.uData.u64IntValue;
			}

			LexClearToken(&sToken);
		}
		else
		{
			printf("Error: Expecting start address\r\n");
			eStatus = ESTATUS_PARSE_UINT32_EXPECTED;
			goto errorExit;
		}

		eToken = LexGetNextToken(psLex,
								 &sToken);
		if ((ETokenType) '-' == eToken)
		{
			// It's a range
			bRange = true;
			LexClearToken(&sToken);

			// Next should be a count
			eToken = LexGetNextToken(psLex,
									 &sToken);
			if ((ELEX_INT_SIGNED == eToken) ||
				(ELEX_INT_UNSIGNED == eToken))
			{
				u32AddressTo = (uint32_t) sToken.uData.u64IntValue;
			}
			else
			{
				printf("Error: Expecting addfrom-addrto\r\n");
				eStatus = ESTATUS_PARSE_UINT32_EXPECTED;
				goto errorExit;
			}

			u16LineCount = 0;
		}
		else
		if ((ELEX_INT_SIGNED == eToken) ||
			(ELEX_INT_UNSIGNED == eToken))
		{
			// It's a line count
		}
		else
		{
			printf("Error: Expecting a range - addrfrom-addrto \r\n");
			eStatus = ESTATUS_PARSE_UINT32_EXPECTED;
			goto errorExit;
		}
	}

	LexClearToken(&sToken);

	eStatus = MonitorCheckAddress("Error: Disassembly from address ", u32AddressFrom);
	ERR_GOTO();
	eStatus = MonitorCheckAddress("Error: Disassembly to address ", u32AddressTo);
	ERR_GOTO();

	if (u16LineCount)
	{
		while (u16LineCount)
		{
			u32AddressFrom = disasm(u32AddressFrom);
			*pu32AddressPointer = u32AddressFrom;
			u16LineCount--;

			// Look for input of any kind, and if we get it, stop the 
			if (MonitorInputAvailable())
			{
				// Eat whatever character they just sent us
				(void) MonitorInputGet();
				u16LineCount = 0;
			}
		}
	}
	else
	{
		// Dissassemble from->to
		while (u32AddressFrom < u32AddressTo)
		{
			u32AddressFrom = disasm(u32AddressFrom);
			*pu32AddressPointer = u32AddressFrom;
			// Look for input of any kind, and if we get it, stop the 
			if (MonitorInputAvailable())
			{
				// Eat whatever character they just sent us
				(void) MonitorInputGet();
				u32AddressFrom = u32AddressTo;
			}
		}
	}

	eStatus = ESTATUS_OK;

errorExit:
	return(eStatus);
}

// Show the system's memory map
static EStatus MonitorMemoryMap(SLex *psLex,
								const SMonitorCommands *psMonitorCommand,
								uint32_t *pu32AddressPointer)
{
	const char *peBoardType = "Worker";

	if (SLOT_ID_SUPERVISOR == g_u8SlotID)
	{
		peBoardType = "Supervisor";
	}

	printf("%s memory map:\r\n", peBoardType);

	printf("  0x%.6x-0x%.6x - Base memory\r\n", RSC68KHW_BASE_RAM, (RSC68KHW_BASE_RAM + RSC68KHW_BASE_RAM_SIZE - 1));
	printf("  0x%.6x-0x%.6x - Shared memory\r\n", RSC68KHW_BASE_SHARED_RAM, (RSC68KHW_BASE_SHARED_RAM + RSC68KHW_BASE_SHARED_RAM_SIZE - 1));

	if (SLOT_ID_SUPERVISOR == g_u8SlotID)
	{
		printf("  0x%.6x-0x%.6x - BIOS Flash/RAM region\r\n", RSC68KHW_SUPERVISOR_FLASH, (RSC68KHW_SUPERVISOR_FLASH + RSC68KHW_SUPERVISOR_FLASH_SIZE - 1));
	}
	else
	{
		printf("  0x%.6x-0x%.6x - BIOS RAM region\r\n", RSC68KHW_SUPERVISOR_FLASH, (RSC68KHW_SUPERVISOR_FLASH + RSC68KHW_SUPERVISOR_FLASH_SIZE - 1));
		printf("  0x%.6x-0x%.6x - High RAM\r\n", RSC68KHW_WORKER_HIGH_RAM, (RSC68KHW_WORKER_HIGH_RAM + RSC68KHW_WORKER_HIGH_RAM_SIZE - 1));
	}

	printf("Common peripherals:\r\n");
	printf("  0x%.6x-0x%.6x - POST LEDs\r\n", RSC68KHW_DEVCOM_POST_LED, (RSC68KHW_DEVCOM_UARTA - 1));
	printf("  0x%.6x-0x%.6x - UART A (console)\r\n", RSC68KHW_DEVCOM_UARTA, (RSC68KHW_DEVCOM_UARTB - 1));
	printf("  0x%.6x-0x%.6x - UART B\r\n", RSC68KHW_DEVCOM_UARTB, (RSC68KHW_DEVCOM_IDE_CSA - 1));
	printf("  0x%.6x-0x%.6x - IDE CSA\r\n", RSC68KHW_DEVCOM_IDE_CSA, (RSC68KHW_DEVCOM_IDE_CSA - 1));
	printf("  0x%.6x-0x%.6x - IDE CSB\r\n", RSC68KHW_DEVCOM_IDE_CSB, (RSC68KHW_DEVCOM_IDE_CSB - 1));
	printf("  0x%.6x-0x%.6x - Status LED\r\n", RSC68KHW_DEVCOM_STATUS_LED, (RSC68KHW_DEVCOM_STATUS_LED - 1));
	printf("  0x%.6x-0x%.6x - Interrupt mask 1\r\n", RSC68KHW_DEVCOM_INTC_MASK, (RSC68KHW_DEVCOM_INTC_MASK - 1));
	printf("  0x%.6x-0x%.6x - Interrupt mask 2\r\n", RSC68KHW_DEVCOM_INTC_MASK2, (RSC68KHW_DEVCOM_INTC_MASK2 - 1));
	printf("  0x%.6x-0x%.6x - Self reset\r\n", RSC68KHW_DEVCOM_SELF_RESET, (RSC68KHW_DEVCOM_SELF_RESET - 1));

	if (SLOT_ID_SUPERVISOR == g_u8SlotID)
	{
		// Supervisor specific peripherals
		printf("Device specific peripherals:\r\n");
		printf("  0x%.6x-0x%.6x - Reset drive\r\n", RSC68KHW_DEVSPEC_RESET_DRIVE, (RSC68KHW_DEVSPEC_CLOCK_CTRL - 1));
		printf("  0x%.6x-0x%.6x - Clock control\r\n", RSC68KHW_DEVSPEC_CLOCK_CTRL, (RSC68KHW_DEVSPEC_REQUEST_GRANT - 1));
		printf("  0x%.6x-0x%.6x - Worker request/grant\r\n", RSC68KHW_DEVSPEC_REQUEST_GRANT, (RSC68KHW_DEVSPEC_V82C42 - 1));
		printf("  0x%.6x-0x%.6x - V82C42 Keyboard/mouse controller\r\n", RSC68KHW_DEVSPEC_V82C42, (RSC68KHW_DEVSPEC_RTC - 1));
		printf("  0x%.6x-0x%.6x - RTC\r\n", RSC68KHW_DEVSPEC_RTC, (RSC68KHW_DEVSPEC_FLASH_DRAM_MAP - 1));
		printf("  0x%.6x-0x%.6x - Flash/DRAM map\r\n", RSC68KHW_DEVSPEC_FLASH_DRAM_MAP, (RSC68KHW_DEVSPEC_NIC - 1));
		printf("  0x%.6x-0x%.6x - NIC\r\n", RSC68KHW_DEVSPEC_NIC, (RSC68KWH_DEVSPEC_BARRIER_FLAGS_RD - 1));
		printf("  0x%.6x-0x%.6x - Barrier flags read\r\n", RSC68KWH_DEVSPEC_BARRIER_FLAGS_RD, (RSC68KWH_DEVSPEC_BARRIER_FLAGS_WR - 1));
		printf("  0x%.6x-0x%.6x - Barrier flags write\r\n", RSC68KWH_DEVSPEC_BARRIER_FLAGS_WR, (RSC68KWH_DEVSPEC_UARTA - 1));
		printf("  0x%.6x-0x%.6x - Baseboard UART A\r\n", RSC68KWH_DEVSPEC_UARTA, (RSC68KWH_DEVSPEC_UARTB - 1));
		printf("  0x%.6x-0x%.6x - Baseboard UART B\r\n", RSC68KWH_DEVSPEC_UARTB, (RSC68KHW_DEVSPEC_RESET_DRIVE + 0xff));

	}

	return(ESTATUS_OK);
}

// Help command function prototype
static EStatus MonitorHelp(SLex *psLex,
						   const SMonitorCommands *psMonitorCommand,
						   uint32_t *pu32AddressPointer);

// Monitor commands go here!
static const SMonitorCommands sg_sMonitorCommands[] =
{
	{"d",  		"Byte-wise memory dump",						MonitorDumpMemory},
	{"dis",		"Disassemble code at supplied address",			MonitorDisassemble},
	{"fill",	"Fills memory with a byte pattern",				MonitorFill},
	{"go",		"Call to an address",							MonitorGo},
	{"lines",	"Set memory dump/disassembly line count",  		MonitorLines},
	{"map",		"Show a system memory map",						MonitorMemoryMap},
	{"rb",		"Read memory (byte)",							MonitorRead},
	{"rw",		"Read memory (word/uint16)",  					MonitorRead},
	{"rd",		"Read memory (dword/uint32)", 					MonitorRead},
	{"help",	"List all commands",							MonitorHelp},
	{"stack",	"Show information on heap and stack",			MonitorStack},
	{"w",		"Write memory (size derived from arguments)",	MonitorWrite},
	{"wb",		"Write memory (byte)",							MonitorWrite},
	{"ww",		"Write memory (word/uint16)",					MonitorWrite},
	{"wd",		"Write memory (dword/uint32)",					MonitorWrite},
};

// Function needs to come after sg_sMonitorCommands[] due to a forward reference
static EStatus MonitorHelp(SLex *psLex,
						   const SMonitorCommands *psMonitorCommand,
						   uint32_t *pu32Address)
{
	uint16_t u16Loop;

	printf("Supported commands:\r\n");
	for (u16Loop = 0; u16Loop < (sizeof(sg_sMonitorCommands) / sizeof(sg_sMonitorCommands[0])); u16Loop++)
	{
		printf("%-6s - %s\r\n", sg_sMonitorCommands[u16Loop].peCommandString, sg_sMonitorCommands[u16Loop].peDescription);
	}

	return(ESTATUS_OK);
}

static EStatus MonitorParse(char *peLineText,
							uint16_t u16LineLength,
							uint32_t *pu32AddressPointer)
{
	SLex *psLex = NULL;
	EStatus eStatus = ESTATUS_OK;
	SMonitorLineState *psLexLine = NULL;
	SToken sToken;
	ETokenType eToken;
	uint16_t u16Loop;

	// If we have nothing, then return
	if (0 == u16LineLength)
	{
		goto errorExit;
	}

	assert(peLineText);

	// Create a line state structure
	psLexLine = malloc(sizeof(*psLexLine));
	if (NULL == psLexLine)
	{
		eStatus = ESTATUS_OUT_OF_MEMORY;
		goto errorExit;
	}

	// Fill it in with line data
	psLexLine->peLineText = peLineText;
	psLexLine->u16LineLength = u16LineLength;

	// Now open up the lexer stream
	eStatus = LexOpen(&sg_sMonitorLexStreamFunctions,
					  (void *) psLexLine,
					  &psLex,
					  NULL);
	ERR_GOTO();

	ZERO_STRUCT(sToken);

	eToken = LexGetNextToken(psLex,
							 &sToken);
	if (ELEX_EOF == eToken)
	{
		// Fall through - blank lines
	}
	else
	if ((ELEX_INT_SIGNED == eToken) ||
		(ELEX_INT_UNSIGNED == eToken))
	{
		uint32_t u32Address;

		// Might be a pointer setting
		u32Address = (uint32_t) sToken.uData.u64IntValue;
		eStatus = MonitorCheckAddress("Error: Pointer ", u32Address);
		ERR_GOTO();

		printf("Address pointer set to 0x%.6x\r\n", u32Address);
		*pu32AddressPointer = u32Address;
		eStatus = ESTATUS_OK;
	}
	else
	if (ELEX_IDENTIFIER == eToken)
	{
		// Yes, it's an identifier. See if this is a command we know and
		for (u16Loop = 0; u16Loop < (sizeof(sg_sMonitorCommands) / sizeof(sg_sMonitorCommands[0])); u16Loop++)
		{
			if (strcasecmp(sg_sMonitorCommands[u16Loop].peCommandString, sToken.peString) == 0)
			{
				// Found it!
				LexClearToken(&sToken);

				eStatus = sg_sMonitorCommands[u16Loop].Handler(psLex,
															   &sg_sMonitorCommands[u16Loop],
															   pu32AddressPointer);
				break;
			}
		}

		if (u16Loop == (sizeof(sg_sMonitorCommands) / sizeof(sg_sMonitorCommands[0])))
		{
			printf("Unknown command '%s' - type 'help' or '?' for a list of valid commands\r\n", sToken.peString);
		}

		LexClearToken(&sToken);
	}
	else
	if ('?' == eToken)
	{
		// Alias for help
		eStatus = MonitorHelp(psLex,
							  NULL,
							  pu32AddressPointer);
	}
	else
	{
		printf("Expected a command - type 'help' or '?' for a list of valid commands\r\n", sToken.peString);
		LexClearToken(&sToken);
		eStatus = ESTATUS_OK;
	}

errorExit:
	// Don't worry about psLexLine. It'll get cleaned up by the close.
	LexClose(&psLex);

	if (eStatus != ESTATUS_OK)
	{
		printf("Error on parse - 0x%.8x\r\n", eStatus);
	}

	return(eStatus);
}

// Entry point for the monitor
EStatus MonitorStart(void)
{
	EStatus eStatus;
	SLineInput sLineInput;
	char *peLineText = NULL;
	uint16_t u16LineLength;
	uint32_t u32AddressPointer = 0;

	printf("\r\nRSC68k Monitor. Type '?' or 'help' for a list of commands\r\n");

	// Init the line input
	ZERO_STRUCT(sLineInput);

	sLineInput.Output = MonitorOutput;
	sLineInput.InputGet = MonitorInputGet;
	sLineInput.u16InputMax = MONITOR_LINE_MAX;
	sLineInput.u8HistoryMaxDepth = MONITOR_HISTORY_DEPTH;

	eStatus = LineInputInit(&sLineInput);
	assert(ESTATUS_OK == eStatus);

	while (1)
	{
		peLineText = NULL;
		u16LineLength = MONITOR_LINE_MAX;

		// Display the prompt
		printf("%.6x>", u32AddressPointer);

		// This is necessary for when printf is used and there isn't a line terminator
		fflush(stdout);

		// Go get a line!
		eStatus = LineInputGet(&sLineInput,
							   &peLineText,
							   &u16LineLength);

		if (ESTATUS_OK == eStatus)
		{
			// They hit enter
			MonitorParse(peLineText,
						 u16LineLength,
						 &u32AddressPointer);
		}
		else
		if (ESTATUS_CTRL_C == eStatus)
		{
			// Ctrl-C
		}
		else
		{
			// Unknown result.

			printf("eStatus != ESTATUS_OK - 0x%.8x\r\n", eStatus);
			assert(0);
		}

		free(peLineText);
	}

	return(ESTATUS_OK);
}

