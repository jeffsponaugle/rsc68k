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
#include "Shared/IDE.h"
#include "Shared/MemTest.h"
#include "Shared/AsmUtils.h"
#include "FlashUpdate/FlashUpdateC.h"
#include "Shared/rtc.h"
#include "Shared/Shared.h"
#include "Shared/ptc.h"
#include "Shared/Stream.h"

// Maximum # of characters per input line
#define	MONITOR_LINE_MAX		256

// Depth of line history (in lines)
#define	MONITOR_HISTORY_DEPTH	20

// # Of lines to dump/disassemble
static uint16_t sg_u16DumpLines = 8;

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

// Output a character string to the console
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

static bool MonitorTerminateDumpCallback(void)
{
	// Look for input of any kind, and if we get it, stop the 
	if (MonitorInputAvailable())
	{
		// Eat whatever character they just sent us
		(void) MonitorInputGet();

		return(true);
	}
	else
	{
		return(false);
	}
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
		u32AddressTo = u32AddressFrom + (sg_u16DumpLines << 4) - 1;

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
			u32AddressTo = u32AddressFrom + (sg_u16DumpLines << 4) - 1;

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

	(void) DumpHex(u32AddressFrom & ~0xf,
				   (u32AddressTo - u32AddressFrom) + 1,
				   0,
				   (uint8_t *) u32AddressFrom,
				   (uint32_t *) pu32AddressPointer,
				   true,
				   MonitorTerminateDumpCallback);

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
	printf("  0x%.6x-0x%.6x - IDE CSA\r\n", RSC68KHW_DEVCOM_IDE_CSA, (RSC68KHW_DEVCOM_IDE_CSB - 1));
	printf("  0x%.6x-0x%.6x - IDE CSB\r\n", RSC68KHW_DEVCOM_IDE_CSB, (RSC68KHW_DEVCOM_STATUS_LED - 1));
	printf("  0x%.6x-0x%.6x - Status LED\r\n", RSC68KHW_DEVCOM_STATUS_LED, (RSC68KHW_DEVCOM_INTC_MASK - 1));
	printf("  0x%.6x-0x%.6x - Interrupt mask 1\r\n", RSC68KHW_DEVCOM_INTC_MASK, (RSC68KHW_DEVCOM_INTC_MASK2 - 1));
	printf("  0x%.6x-0x%.6x - Interrupt mask 2\r\n", RSC68KHW_DEVCOM_INTC_MASK2, (RSC68KHW_DEVCOM_SELF_RESET - 1));
	printf("  0x%.6x-0x%.6x - Self reset\r\n", RSC68KHW_DEVCOM_SELF_RESET, (RSC68KHW_DEVCOM_SELF_RESET + 0xff));

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
		printf("  0x%.6x-0x%.6x - Barrier flags read\r\n", RSC68KWH_DEVSPEC_BARRIER_FLAGS_RD, (RSC68KHW_DEVSPEC_PTC - 1));
		printf("  0x%.6x-0x%.6x - PTC\r\n", RSC68KHW_DEVSPEC_PTC, (RSC68KWH_DEVSPEC_BARRIER_FLAGS_WR - 1));
		printf("  0x%.6x-0x%.6x - Barrier flags write\r\n", RSC68KWH_DEVSPEC_BARRIER_FLAGS_WR, (RSC68KWH_DEVSPEC_UARTA - 1));
		printf("  0x%.6x-0x%.6x - Baseboard UART A\r\n", RSC68KWH_DEVSPEC_UARTA, (RSC68KWH_DEVSPEC_UARTB - 1));
		printf("  0x%.6x-0x%.6x - Baseboard UART B\r\n", RSC68KWH_DEVSPEC_UARTB, (RSC68KHW_DEVSPEC_RESET_DRIVE + 0xff));
	}

	return(ESTATUS_OK);
}

static EStatus MonitorReset(SLex *psLex,
						    const SMonitorCommands *psMonitorCommand,
						    uint32_t *pu32AddressPointer)
{
	printf("Hard reset...\r\n");

	// This causes the system to perform a hardware based reset
	*((volatile uint8_t *) RSC68KHW_DEVCOM_SELF_RESET) = 0xff;

	printf("If you see this, the reset hardware is broken.\r\n");
	return(ESTATUS_OK);
}

#define	MAX_MEMORY_ERRORS	20
#define	PROCESS_ERROR()		u8ErrorCount++; if (u8ErrorCount >= MAX_MEMORY_ERRORS) { printf("Max memory errors of %u reached - stopping\r\n"); goto errorExit; }

static EStatus MonitorMemoryTest(SLex *psLex,
								 const SMonitorCommands *psMonitorCommand,
								 uint32_t *pu32AddressPointer)
{
	EStatus eStatus;
	ETokenType eToken;
	SToken sToken;
	uint32_t u32Address;
	uint32_t u32FailedAddress;
	uint32_t u32Size;
	uint8_t u8Expected;
	uint8_t u8Got;
	uint16_t u16Expected;
	uint16_t u16Got;
	uint32_t u32Expected;
	uint32_t u32Got;
	uint32_t u32Passes = 1;
	uint32_t u32PassCount = 1;
	uint8_t u8ErrorCount = 0;

	*((uint16_t *) RSC68KHW_DEVCOM_STATUS_LED) = 0xffff;

	ZERO_STRUCT(sToken);
	u32Address = *pu32AddressPointer;

	// First is base address
	eToken = LexGetNextToken(psLex,
							 &sToken);
	if ((ELEX_INT_SIGNED == eToken) ||
		(ELEX_INT_UNSIGNED == eToken))
	{
		u32Address = (uint32_t) sToken.uData.u64IntValue;
	}
	else
	if ((ETokenType) '.' == eToken)
	{
		// Current address - u32Address already has it
	}
	else
	{
		printf("Error: Expecting an address\r\n");
		goto errorExit;
	}

	LexClearToken(&sToken);

	// Now get a count
	eToken = LexGetNextToken(psLex,
							 &sToken);
	if ((ELEX_INT_SIGNED == eToken) ||
		(ELEX_INT_UNSIGNED == eToken))
	{
		u32Size = (uint32_t) sToken.uData.u64IntValue;
	}
	else
	{
		printf("Error: Expecting size of data to test\r\n");
		goto errorExit;
	}

	LexClearToken(&sToken);

	// See if they provided a pass count
	eToken = LexGetNextToken(psLex,
							 &sToken);
	if (ELEX_EOF == eToken)
	{
		// Leave the default count if 1
	}
	else
	if ((ELEX_INT_SIGNED == eToken) ||
		(ELEX_INT_UNSIGNED == eToken))
	{
		u32Passes = (uint32_t) sToken.uData.u64IntValue;
	}
	else
	{
			printf("Error: Expecting pass count - 0==endless\r\n");
			goto errorExit;
	}

	for (;;)
	{
		printf("Pass #%u\r\n", u32PassCount);

		// Data line tests
		printf("Data line test - uint8 : ");
		fflush(stdout);

		if (MemTestDataBusUINT8((volatile uint8_t *) u32Address,
								&u8Expected,
								&u8Got))
		{
			printf("OK\r\n");
		}
		else
		{
			printf("Failed - Address 0x%.6x, expected 0x%.2x, got 0x%.2x\r\n", u32Address, u8Expected, u8Got);
			PROCESS_ERROR();
		}

		// Look for input of any kind, and if we get it, stop the test
		if (MonitorInputAvailable())
		{
			// Eat whatever character they just sent us
			(void) MonitorInputGet();
			goto errorExit;
		}

		printf("Data line test - uint16: ");
		fflush(stdout);

		if (MemTestDataBusUINT16((volatile uint16_t *) u32Address,
								 &u16Expected,
								 &u16Got))
		{
			printf("OK\r\n");
		}
		else
		{
			printf("Failed - Address 0x%.6x, expected 0x%.4x, got 0x%.4x\r\n", u32Address, u16Expected, u16Got);
			PROCESS_ERROR();
		}

		// Look for input of any kind, and if we get it, stop the test
		if (MonitorInputAvailable())
		{
			// Eat whatever character they just sent us
			(void) MonitorInputGet();
			goto errorExit;
		}

		printf("Data line test - uint32: ");
		fflush(stdout);

		if (MemTestDataBusUINT32((volatile uint32_t *) u32Address,
								 &u32Expected,
								 &u32Got))
		{
			printf("OK\r\n");
		}
		else
		{
			printf("Failed - Address 0x%.6x, expected 0x%.8x, got 0x%.8x\r\n", u32Address, u32Expected, u32Got);
			PROCESS_ERROR();
		}

		// Look for input of any kind, and if we get it, stop the test
		if (MonitorInputAvailable())
		{
			// Eat whatever character they just sent us
			(void) MonitorInputGet();
			goto errorExit;
		}

		// Device tests
		printf("Device test    - uint8 : ");
		fflush(stdout);

		if (MemTestDeviceUINT8((volatile uint8_t *) u32Address,
							   u32Size,
							   &u32FailedAddress,
							   &u8Expected,
							   &u8Got))
		{
			printf("OK\r\n");
		}
		else
		{
			printf("Failed - Address 0x%.6x, expected 0x%.2x, got 0x%.2x\r\n", u32FailedAddress, u8Expected, u8Got);
			PROCESS_ERROR();
		}

		// Look for input of any kind, and if we get it, stop the test
		if (MonitorInputAvailable())
		{
			// Eat whatever character they just sent us
			(void) MonitorInputGet();
			goto errorExit;
		}

		printf("Device test    - uint16: ");
		fflush(stdout);

		if (MemTestDeviceUINT16((volatile uint16_t *) u32Address,
								u32Size,
								&u32FailedAddress,
								&u16Expected,
								&u16Got))
		{
			printf("OK\r\n");
		}
		else
		{
			printf("Failed - Address 0x%.6x, expected 0x%.4x, got 0x%.4x\r\n", u32FailedAddress, u8Expected, u8Got);
			PROCESS_ERROR();
		}

		// Look for input of any kind, and if we get it, stop the test
		if (MonitorInputAvailable())
		{
			// Eat whatever character they just sent us
			(void) MonitorInputGet();
			goto errorExit;
		}

		printf("Device test    - uint32: ");
		fflush(stdout);

		if (MemTestDeviceUINT32((volatile uint32_t *) u32Address,
								u32Size,
								&u32FailedAddress,
								&u32Expected,
								&u32Got))
		{
			printf("OK\r\n");
		}
		else
		{
			printf("Failed - Address 0x%.6x, expected 0x%.8x, got 0x%.8x\r\n", u32FailedAddress, u8Expected, u8Got);
			PROCESS_ERROR();
		}

		// Look for input of any kind, and if we get it, stop the test
		if (MonitorInputAvailable())
		{
			// Eat whatever character they just sent us
			(void) MonitorInputGet();
			goto errorExit;
		}

		if (0 == u32Passes)
		{
			// Go forever
		}
		else
		{
			u32Passes--;
			if (0 == u32Passes)
			{
				goto errorExit;
			}
		}

		u32PassCount++;
	}

errorExit:
	return(ESTATUS_OK);
}

// Burstr/burstw handler 

static EStatus MonitorBurstReadWrite(SLex *psLex,
									 const SMonitorCommands *psMonitorCommand,
									 uint32_t *pu32AddressPointer,
									 bool bWrite)
{
	EStatus eStatus;
	ETokenType eToken;
	SToken sToken;
	uint32_t u32Address;
	uint32_t u32EndAddress;

	ZERO_STRUCT(sToken);
	u32Address = *pu32AddressPointer;

	// First is base address
	eToken = LexGetNextToken(psLex,
							 &sToken);
	if ((ELEX_INT_SIGNED == eToken) ||
		(ELEX_INT_UNSIGNED == eToken))
	{
		u32Address = (uint32_t) sToken.uData.u64IntValue;
	}
	else
	if ((ETokenType) '.' == eToken)
	{
		// Current address - u32Address already has it
	}
	else
	{
		printf("Error: Expecting an address\r\n");
		goto errorExit;
	}

	LexClearToken(&sToken);

	// Expect a '-'
	eToken = LexGetNextToken(psLex,
							 &sToken);
	if ((ETokenType) '-' == eToken)
	{
		// Got it!
	}
	else
	{
		printf("Error: Expecting '-' for address range\r\n");
		goto errorExit;
	}

	LexClearToken(&sToken);

	// Now get the destination address
	eToken = LexGetNextToken(psLex,
							 &sToken);
	if ((ELEX_INT_SIGNED == eToken) ||
		(ELEX_INT_UNSIGNED == eToken))
	{
		u32EndAddress = (uint32_t) sToken.uData.u64IntValue;
	}
	else
	{
		printf("Error: Expecting destination address\r\n");
		goto errorExit;
	}

	if (u32Address > u32EndAddress)
	{
		printf("Error: End address is lower than the start address\r\n");
		goto errorExit;
	}

	if (bWrite)
	{
		while (u32Address < u32EndAddress)
		{
			// 32 Bytes per burst
			u32Address += (sizeof(uint32_t) * 8);
			MoveMultipleWrite((void *) u32Address);
		}
	}
	else
	{
		while (u32Address < u32EndAddress)
		{
			// 32 Bytes per burst
			MoveMultipleRead((void *) u32Address);
			u32Address += (sizeof(uint32_t) * 8);
		}
	}

	LexClearToken(&sToken);

errorExit:
	return(ESTATUS_OK);
}

// Burstr fromaddr-toaddr
static EStatus MonitorBurstRead(SLex *psLex,
								const SMonitorCommands *psMonitorCommand,
								uint32_t *pu32AddressPointer)
{
	return(MonitorBurstReadWrite(psLex,
								 psMonitorCommand,
								 pu32AddressPointer,
								 false));
}


// Burstw fromaddr-toaddr
static EStatus MonitorBurstWrite(SLex *psLex,
								 const SMonitorCommands *psMonitorCommand,
								 uint32_t *pu32AddressPointer)
{
	return(MonitorBurstReadWrite(psLex,
								 psMonitorCommand,
								 pu32AddressPointer,
								 true));
}

// Flash update
static EStatus MonitorFlashUpdate(SLex *psLex,
								  const SMonitorCommands *psMonitorCommand,
								  uint32_t *pu32AddressPointer)
{
	EVersionCode eVersionCode;
	SImageVersion *psImageVersion;
	EStatus eStatus;

	// Now go find the SVersionInfo structure
	eVersionCode = VersionFindStructure(g_u8FlashUpdate,
										g_u8FlashUpdateSize,
										EIMGTYPE_FLASH_UPDATE,
										&psImageVersion);
	if (EVERSION_OK == eVersionCode)
	{
		printf("Dispatching flash update utility\r\n");
	}
	else
	{
		// Couldn't find the entry point for this image
		printf("Couldn't find an entry point\r\n");
		return(ESTATUS_OK);
	}

	// Shut down the 
	eStatus = StreamSetConsoleSerialInterruptMode(false);
	if (eStatus != ESTATUS_OK)
	{
		printf("Error while shutting down interrupt console - %s\r\n", GetErrorText(eStatus));
		goto errorExit;
	}

	// Mask all interrupts, otherwise bad things happen
	*((volatile uint8_t *) RSC68KHW_DEVCOM_INTC_MASK) = 0xff;
	*((volatile uint8_t *) RSC68KHW_DEVCOM_INTC_MASK2) = 0xff;

	// Copy the flash updater into the bottom of the system
	memcpy((void *) NULL, (void *) g_u8FlashUpdate, g_u8FlashUpdateSize);

	// Dispatch to flash update utility
	((void (*)(void))psImageVersion->u32EntryPoint)();

errorExit:
	return(ESTATUS_OK);
}

// Read/write sector(s)
//
// Parses one of the following:
// sector
// sector address
// sector(count)
// sector(count) address

static EStatus MonitorIDEReadWrite(SLex *psLex,
								   const SMonitorCommands *psMonitorCommand,
								   uint32_t *pu32AddressPointer,
								   bool bWrite)
{
	EStatus eStatus;
	SToken sToken;
	ETokenType eToken;
	uint32_t u32Sector;
	uint32_t u32SectorCount = 1;
	uint32_t u32Address = 0;
	uint8_t *pu8TempBufferBase = NULL;
	uint8_t *pu8TempBuffer = NULL;
	bool bAddressProvided = false;

	// Get the sector #
	ZERO_STRUCT(sToken);
	eToken = LexGetNextToken(psLex,
							 &sToken);
	if (eToken != ELEX_INT_UNSIGNED)
	{
		printf("Expected sector #\r\n");
		eStatus = ESTATUS_PARSE_ERROR;
		goto errorExit;
	}

	u32Sector = (uint32_t) sToken.uData.u64IntValue;

	// Now look for another number or (
	LexClearToken(&sToken);
	eToken = LexGetNextToken(psLex,
							 &sToken);
	if (ELEX_EOF == eToken)
	{
		// Just supplied the sector
	}
	else
	if ((ETokenType) '(' == eToken)
	{
		// They are providing a count
		LexClearToken(&sToken);
		eToken = LexGetNextToken(psLex,
								 &sToken);
		if (ELEX_INT_UNSIGNED == eToken)
		{
			u32SectorCount = (uint32_t) sToken.uData.u64IntValue;
			if (0 == u32SectorCount)
			{
				printf("Sector count can't be 0\r\n");
				eStatus = ESTATUS_PARSE_ERROR;
				goto errorExit;
			}
		}
		else
		{
			printf("Sector count expected\r\n");
			eStatus = ESTATUS_PARSE_ERROR;
			goto errorExit;
		}

		LexClearToken(&sToken);
		eToken = LexGetNextToken(psLex,
								 &sToken);
		if (eToken != (ETokenType) ')')
		{
			printf("')' Expected\r\n");
			eStatus = ESTATUS_PARSE_ERROR;
			goto errorExit;
		}

		LexClearToken(&sToken);
		eToken = LexGetNextToken(psLex,
								 &sToken);
		if (ELEX_EOF == eToken)
		{
			// All good - no address provided
		}
		else
		if (ELEX_INT_UNSIGNED == eToken)
		{
			// This is the address
			u32Address = (uint32_t) sToken.uData.u64IntValue;
			bAddressProvided = true;
		}
		else
		{
			printf("Address expected\r\n");
			eStatus = ESTATUS_PARSE_ERROR;
			goto errorExit;
		}
	}
	else
	if (ELEX_INT_UNSIGNED == eToken)
	{
		// This is the address
		u32Address = (uint32_t) sToken.uData.u64IntValue;
		bAddressProvided = true;
	}
	else
	{
		printf("Expected address or (sector count)\r\n");
		eStatus = ESTATUS_PARSE_ERROR;
		goto errorExit;
	}

	if (bAddressProvided)
	{
		if (u32Address > 0xffffff)
		{
			printf("Address beyond range of CPU's address space\r\n");
			eStatus = ESTATUS_MONITOR_ADDRESS_TOO_HIGH;
			goto errorExit;
		}
	}

	// u32Sector = Sector # base to read/write
	// u32SectorCount = # Of sectors to read/write
	// u32Address = Data buffer

	if (bWrite)
	{
		if (false == bAddressProvided)
		{
			printf("Missing source address to write from\r\n");
			eStatus = ESTATUS_PARSE_ERROR;
			goto errorExit;
		}

		printf("Writing sectors %u-%u from memory @ 0x%.6x-0x%.6x: ", u32Sector, u32Sector + u32SectorCount - 1, u32Address, u32Address + (u32SectorCount << 9) - 1);

		eStatus = IDEWriteSector(0,
								 u32Sector,
								 u32SectorCount,
								 (uint8_t *) u32Address);

		printf("%s\r\n", GetErrorText(eStatus));
		goto errorExit;
	}
	else
	{
		// We're reading into a memory location
		if (bAddressProvided)
		{
			// Reading in to a memory location
			printf("Reading sectors %u-%u into memory @ 0x%.6x-0x%.6x: ", u32Sector, u32Sector + u32SectorCount - 1, u32Address, u32Address + (u32SectorCount << 9) - 1);

			eStatus = IDEReadSector(0,
									u32Sector,
									u32SectorCount,
									(uint8_t *) u32Address);

			printf("%s\r\n", GetErrorText(eStatus));
			goto errorExit;
		}
		
		// We're reading the sectors to the console
		pu8TempBufferBase = malloc(512+16);
		if (NULL == pu8TempBufferBase)
		{
			printf("Could not allocate 512+16 bytes for sector buffer\r\n");
			eStatus = ESTATUS_OUT_OF_MEMORY;
			goto errorExit;
		}

		pu8TempBuffer = (uint8_t *) ((((uint32_t) pu8TempBufferBase) + 15) & ~15);

		while (u32SectorCount)
		{
			printf("Sector %u:\r\n", u32Sector);
			eStatus = IDEReadSector(0,
									u32Sector,
									1,
									pu8TempBuffer);
			if (eStatus != ESTATUS_OK)
			{
				printf("Error: %s\r\n", GetErrorText(eStatus));
				goto errorExit;
			}

			// Dump the sector to the console
			DumpHex(0,
					512,
					0,
					pu8TempBuffer,
					NULL,
					false,
					NULL);
			u32Sector++;
			u32SectorCount--;
		}
	}

errorExit:
	if (pu8TempBufferBase)
	{
		free(pu8TempBufferBase);
	}

	return(eStatus);
}

// iderd sector
// iderd sector address
// iderd sector(count)
// iderd sector(count) address
//
static EStatus MonitorIDERd(SLex *psLex,
							const SMonitorCommands *psMonitorCommand,
							uint32_t *pu32AddressPointer)
{
	MonitorIDEReadWrite(psLex,
						psMonitorCommand,
						pu32AddressPointer,
						false);

	return(ESTATUS_OK);
}

// idewr sector
// idewr sector address
// idewr sector(count)
// idewr sector(count) address
//
static EStatus MonitorIDEWr(SLex *psLex,
							const SMonitorCommands *psMonitorCommand,
							uint32_t *pu32AddressPointer)
{
	MonitorIDEReadWrite(psLex,
						psMonitorCommand,
						pu32AddressPointer,
						true);

	return(ESTATUS_OK);
}
static void ShowDateTime(void)
{
	time_t eTime;
	struct tm *psTime;

	eTime = time(0);
	psTime = localtime((time_t *) &eTime);

	printf("Current time/date is %.4u-%.2u-%.2u %.2u:%.2u:%.2u\r\n", psTime->tm_year + 1900, psTime->tm_mon + 1, psTime->tm_mday, psTime->tm_hour, psTime->tm_min, psTime->tm_sec);
}

// Time command - show or set
static EStatus MonitorTime(SLex *psLex,
						   const SMonitorCommands *psMonitorCommand,
						   uint32_t *pu32AddressPointer)
{
	SToken sToken;
	ETokenType eToken;
	uint8_t u8Hour;
	uint8_t u8Minute;
	uint8_t u8Second;
	time_t eTime;
	struct tm *psTime;

	ZERO_STRUCT(sToken);
	eToken = LexGetNextToken(psLex,
							 &sToken);
	if (ELEX_EOF == eToken)
	{
		ShowDateTime();
		goto errorExit;
	}
	else
	if (ELEX_INT_UNSIGNED == eToken)
	{
		if (sToken.uData.u64IntValue >= 24)
		{
			printf("Hour must be in 0-23 range\r\n");
			goto errorExit;
		}

		u8Hour = (uint8_t) sToken.uData.u64IntValue;
	}
	else
	{
		printf("Expected hour - 0-23\r\n");
		goto errorExit;
	}

	LexClearToken(&sToken);
	eToken = LexGetNextToken(psLex,
							 &sToken);
	if ((ETokenType) ':' == eToken)
	{
		// All good!
	}
	else
	{
		printf(": Expected after hour\r\n");
		goto errorExit;
	}

	LexClearToken(&sToken);
	eToken = LexGetNextToken(psLex,
							 &sToken);
	if (ELEX_INT_UNSIGNED == eToken)
	{
		if (sToken.uData.u64IntValue >= 60)
		{
			printf("Minute must be in 0-59 range\r\n");
			goto errorExit;
		}

		u8Minute = (uint8_t) sToken.uData.u64IntValue;
	}
	else
	{
		printf("Expected minute - 0-59\r\n");
		goto errorExit;
	}

	LexClearToken(&sToken);
	eToken = LexGetNextToken(psLex,
							 &sToken);
	if ((ETokenType) ':' == eToken)
	{
		// All good!
	}
	else
	{
		printf(": Expected after minute\r\n");
		goto errorExit;
	}

	LexClearToken(&sToken);
	eToken = LexGetNextToken(psLex,
							 &sToken);
	if (ELEX_INT_UNSIGNED == eToken)
	{
		if (sToken.uData.u64IntValue >= 60)
		{
			printf("Second must be in 0-59 range\r\n");
			goto errorExit;
		}

		u8Second = (uint8_t) sToken.uData.u64IntValue;
	}
	else
	{
		printf("Expected second - 0-59\r\n");
		goto errorExit;
	}

	// u8Hour/u8Minute/u8Second have been set! Now let's pull things apart via time(0)
	eTime = time(0);
	psTime = localtime((time_t *) &eTime);

	psTime->tm_hour = u8Hour;
	psTime->tm_min = u8Minute;
	psTime->tm_sec = u8Second;

	RTCSetTime(mktime(psTime));

	ShowDateTime();

errorExit:
	return(ESTATUS_OK);
}

// Date command - show or set
static EStatus MonitorDate(SLex *psLex,
						   const SMonitorCommands *psMonitorCommand,
						   uint32_t *pu32AddressPointer)
{
	SToken sToken;
	ETokenType eToken;
	uint16_t u16Year;
	uint8_t u8Month;
	uint8_t u8Day;
	time_t eTime;
	struct tm *psTime;

	ZERO_STRUCT(sToken);
	eToken = LexGetNextToken(psLex,
							 &sToken);
	if (ELEX_EOF == eToken)
	{
		ShowDateTime();
		goto errorExit;
	}
	else
	if (ELEX_INT_UNSIGNED == eToken)
	{
		if ((sToken.uData.u64IntValue < YEAR_BASELINE) || (sToken.uData.u64IntValue >= (YEAR_BASELINE + 100)))
		{
			printf("Year must be in %u-%u range\r\n", YEAR_BASELINE, YEAR_BASELINE + 99);
			goto errorExit;
		}

		u16Year = (uint16_t) sToken.uData.u64IntValue;
	}
	else
	{
		printf("Expected year - %u-%u range\r\n", YEAR_BASELINE, YEAR_BASELINE + 99);
		goto errorExit;
	}

	LexClearToken(&sToken);
	eToken = LexGetNextToken(psLex,
							 &sToken);
	if ((ETokenType) '-' == eToken)
	{
		// All good!
	}
	else
	{
		printf("- Expected after year\r\n");
		goto errorExit;
	}

	LexClearToken(&sToken);
	eToken = LexGetNextToken(psLex,
							 &sToken);
	if (ELEX_INT_UNSIGNED == eToken)
	{
		if ((sToken.uData.u64IntValue < 1) || (sToken.uData.u64IntValue > 12))
		{
			printf("Month must be in 1-12 range\r\n");
			goto errorExit;
		}

		u8Month = (uint8_t) sToken.uData.u64IntValue;
	}
	else
	{
		printf("Expected month - 1-12\r\n");
		goto errorExit;
	}

	LexClearToken(&sToken);
	eToken = LexGetNextToken(psLex,
							 &sToken);
	if ((ETokenType) '-' == eToken)
	{
		// All good!
	}
	else
	{
		printf("- Expected after month\r\n");
		goto errorExit;
	}

	LexClearToken(&sToken);
	eToken = LexGetNextToken(psLex,
							 &sToken);
	if (ELEX_INT_UNSIGNED == eToken)
	{
		if ((sToken.uData.u64IntValue < 1) ||
			(sToken.uData.u64IntValue > 31))
		{
			printf("Day must be in 1-31 range\r\n");
			goto errorExit;
		}

		u8Day = (uint8_t) sToken.uData.u64IntValue;
	}
	else
	{
		printf("Expected day - 1-31\r\n");
		goto errorExit;
	}

	// u8Hour/u8Minute/u8Second have been set! Now let's pull things apart via time(0)
	eTime = time(0);
	psTime = localtime((time_t *) &eTime);

	psTime->tm_year = u16Year - 1900;
	psTime->tm_mon = u8Month - 1;
	psTime->tm_mday = u8Day;

	RTCSetTime(mktime(psTime));

	ShowDateTime();

errorExit:
	return(ESTATUS_OK);
}

// How many seconds do we test for by default?
#define	IDE_TEST_TIME_DEFAULT	10

// # Of sectors per pass
#define	IDE_SECTOR_CHUNK_SIZE	1024

// IDE speed test
// idetest (default 10 seconds)
// idetest seconds
static EStatus MonitorIDETest(SLex *psLex,
							  const SMonitorCommands *psMonitorCommand,
							  uint32_t *pu32AddressPointer)
{
	uint8_t *pu8BufferBase = NULL;
	uint8_t *pu8Buffer = NULL;
	uint32_t u32TestTime = IDE_TEST_TIME_DEFAULT;
	uint32_t u32SectorsRead = 0;
	uint32_t u32KPerSec = 0;
	time_t eStartTime;
	EStatus eStatus;
	SToken sToken;
	ETokenType eToken;

	ZERO_STRUCT(sToken);
	eToken = LexGetNextToken(psLex,
							 &sToken);
	if (ELEX_EOF == eToken)
	{
		// Use the default test time
	}
	else
	if (ELEX_INT_UNSIGNED == eToken)
	{
		if (0 == sToken.uData.u64IntValue)
		{
			printf("Second count must be 1 or greater\r\n");
			goto errorExit;
		}

		u32TestTime = (uint32_t) sToken.uData.u64IntValue;
	}
	else
	{
		printf("Expected time (in seconds)\r\n", YEAR_BASELINE, YEAR_BASELINE + 99);
		goto errorExit;
	}

	LexClearToken(&sToken);

/*	pu8BufferBase = malloc((IDE_SECTOR_CHUNK_SIZE << 9) + 4);
	if (NULL == pu8BufferBase)
	{
		printf("Out of memory while trying to allocate %u bytes\r\n", (IDE_SECTOR_CHUNK_SIZE << 9) + 4);
		goto errorExit;
	}

	// Aligned buffer for reading
	pu8Buffer = (uint8_t *) ((((uint32_t) pu8BufferBase) + 3) & ~3); */
	pu8Buffer = (uint8_t *) 0x10000;

	// Get second aligned
	eStartTime = time(0);
	while (eStartTime == time(0));
	eStartTime = time(0);

	printf("Testing disk speed for %u second(s)\r\n", u32TestTime);

	// Consume data repeatedly
	while ((time(0) - eStartTime) < u32TestTime)
	{
		eStatus = IDEReadSector(0,
								0,
								IDE_SECTOR_CHUNK_SIZE,
								pu8Buffer);
		if (eStatus != ESTATUS_OK)
		{
			printf("Error during read: %s\r\n", GetErrorText(eStatus));
			goto errorExit;
		}

		u32SectorsRead += IDE_SECTOR_CHUNK_SIZE;
	}

	u32KPerSec = (u32SectorsRead >> 1) / u32TestTime;
	printf("Disk read speed: %u total sectors, %uk/sec\r\n", u32SectorsRead, u32KPerSec);

errorExit:
	if (pu8BufferBase)
	{
//		free(pu8BufferBase);
	}

	return(ESTATUS_OK);
}

// iderw passes
#define	IDERW_BUFFER_SIZE	(1024*1024)
#define	IDERW_BUFFER_BASE	0x100000

static EStatus MonitorIDEReadWriteTest(SLex *psLex,
									   const SMonitorCommands *psMonitorCommand,
									   uint32_t *pu32AddressPointer)
{
	uint32_t u32Pass = 1;
	uint32_t u32PassesRemaining = 10;
	SToken sToken;
	ETokenType eToken;
	char *pu8Buffer1 = NULL;
	char *pu8Buffer2 = NULL;
	uint16_t u16Pattern = 0;
	uint16_t *pu16BufferBase;
	uint16_t *pu16BufferBase2;
	uint32_t u32Loop;
	EStatus eStatus;
	bool bErrors = false;
	uint32_t u32Offset = 0;
	
	ZERO_STRUCT(sToken);
	eToken = LexGetNextToken(psLex,
							 &sToken);
	if (ELEX_EOF == eToken)
	{
		// Use the default test time
	}
	else
	if (ELEX_INT_UNSIGNED == eToken)
	{
		if (0 == sToken.uData.u64IntValue)
		{
			printf("Pass count must be 1 or greater\r\n");
			goto errorExit;
		}

		u32PassesRemaining = (uint32_t) sToken.uData.u64IntValue;
	}
	else
	{
		printf("Expected a pass count\r\n");
		goto errorExit;
	}

	pu8Buffer1 = (uint8_t *) IDERW_BUFFER_BASE;
	pu8Buffer2 = pu8Buffer1 + IDERW_BUFFER_SIZE;

	printf("IDE read/write test - %u passes, %u byte buffer\r\n", u32PassesRemaining, IDERW_BUFFER_SIZE);

	while (u32PassesRemaining)
	{
		uint32_t *pu32BufferBase = NULL;

		printf("Read/write test pass #%u\r\n", u32Pass);

		// Fill the primary buffer with an alternating 16 bit pattern. This will produce
		// the following fill pattern:
		// 0000
		// ffff
		// 0001
		// fffe
		// 0002
		// fffd
		// ...

		printf("Fill buffer\r\n");
		u32Loop = IDERW_BUFFER_SIZE / sizeof(*pu16BufferBase) / sizeof(*pu16BufferBase);
		pu16BufferBase = (uint16_t *) pu8Buffer1;
		u16Pattern = (uint16_t) u32Pass;
		while (u32Loop)
		{
			*pu16BufferBase = u16Pattern;
			++pu16BufferBase;
			u16Pattern = (uint16_t) ~u16Pattern;
			*pu16BufferBase = u16Pattern;
			++pu16BufferBase;
			u16Pattern = (uint16_t) ~u16Pattern;
			u16Pattern++;
			u32Loop--;
		}

		// Now write it to disk
		printf("Write to disk...\r\n");

		eStatus = IDEWriteSector(0,
								 0,
								 IDERW_BUFFER_SIZE / 512,
								 pu8Buffer1);
		if (eStatus != ESTATUS_OK)
		{
			printf("Writing data to IDE failed: %s\r\n", GetErrorText(eStatus));
			goto errorExit;
		}

		// Now read it back from disk
		printf("Read from disk...\r\n");

		// Clear out the read buffer
		u32Loop = IDERW_BUFFER_SIZE / sizeof(*pu16BufferBase) / sizeof(*pu16BufferBase);
		pu32BufferBase = (uint32_t *) pu8Buffer2;
		while (u32Loop)
		{
			*pu32BufferBase = 0xffffffff;
			++pu32BufferBase;
			--u32Loop;
		}

		// Now read all the data back in to memory
		eStatus = IDEReadSector(0,
								0,
								IDERW_BUFFER_SIZE / 512,
								pu8Buffer2);
		if (eStatus != ESTATUS_OK)
		{
			printf("Reading data from IDE failed: %s\r\n", GetErrorText(eStatus));
			goto errorExit;
		}

		// Now compare the buffers
		u32Loop = IDERW_BUFFER_SIZE / sizeof(*pu16BufferBase);
		pu16BufferBase = (uint16_t *) pu8Buffer1;
		pu16BufferBase2 = (uint16_t *) pu8Buffer2;

		// Compare the read vs. written
		u32Offset = 0;
		while (u32Loop)
		{
			if (*pu16BufferBase != *pu16BufferBase2)
			{
				printf("Offset 0x%.6x: Expected 0x%.4x, got 0x%.4x\r\n", u32Offset, *pu16BufferBase, *pu16BufferBase2);
				bErrors = true;
			}

			++pu16BufferBase;
			++pu16BufferBase2;
			u32Offset += 2;
			u32Loop--;
		}

		if (bErrors)
		{
			printf("Errors found - stopping test\r\n");
			goto errorExit;
		}

		u32PassesRemaining--;
		++u32Pass;
	}

errorExit:
	return(ESTATUS_OK);
}

static EStatus MonitorIDEProbe(SLex *psLex,
							   const SMonitorCommands *psMonitorCommand,
							   uint32_t *pu32AddressPointer)
{
	(void) IDEProbe();

	return(ESTATUS_OK);
}

#define	SR_INT_BIT				8
#define	SR_INT_PRIORITY_MASK	(0x7 << SR_INT_BIT)

static EStatus MonitorIntMask(SLex *psLex,
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
		// Just dump the interrupt mask
		printf("Interrupt mask: %u\r\n", (SRGet() & SR_INT_PRIORITY_MASK) >> SR_INT_BIT);
	}
	else
	if ((ELEX_INT_SIGNED == eToken) ||
		(ELEX_INT_UNSIGNED == eToken))
	{
		if (sToken.uData.u64IntValue > 7)
		{
			printf("Interrupt mask out of range - must be 0-7\r\n");
		}
		else
		{
			SRSet((SRGet() & ~SR_INT_PRIORITY_MASK) | (((uint16_t) sToken.uData.u64IntValue) << SR_INT_BIT));
			printf("Interrupt mask set to %u\r\n", (uint8_t) (((SRGet() & SR_INT_PRIORITY_MASK) >> SR_INT_BIT)));
		}

		LexClearToken(&sToken);
	}
	else
	{
		printf("Expecting nothing or an (up to) 8 bit mask\r\n");
	}

	return(ESTATUS_OK);
}

// Show system counters
static EStatus MonitorCounters(SLex *psLex,
							   const SMonitorCommands *psMonitorCommand,
							   uint32_t *pu32AddressPointer)
{
	EStatus eStatus;
	uint16_t u16Count;
	uint8_t u8Channel;
	uint32_t u32Interrupts;

	printf("Power on seconds    : %u\r\n", RTCGetPowerOnSeconds());

	for (u8Channel = 0; u8Channel < 2; u8Channel++)
	{
		uint32_t u32InterruptCount;

		eStatus = PTCGetInterruptCounter(u8Channel,
										 &u32InterruptCount);
		printf("Counter %u ints      : %u\r\n", u8Channel, u32InterruptCount);
	}

	for (u8Channel = 0; u8Channel < 3; u8Channel++)
	{
		eStatus = PTCGetCount(u8Channel,
							  &u16Count);
		ERR_GOTO();
		printf("PTC Channel %u value : 0x%.4x\r\n", u8Channel, u16Count);
	}

	for (u8Channel = 0; u8Channel < 2; u8Channel++)
	{
		eStatus = SerialGetInterruptCounter(u8Channel,
											&u32Interrupts);
		ERR_GOTO();
		printf("UART %c IRQ count    : %u\r\n", u8Channel + 'A', u32Interrupts);
	}

errorExit:
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
	{"flashupdate",	"Updates the system flash via UART",		MonitorFlashUpdate},
	{"go",		"Call to an address",							MonitorGo},
	{"lines",	"Set memory dump/disassembly line count",  		MonitorLines},
	{"map",		"Show a system memory map",						MonitorMemoryMap},
	{"reset",	"Do a hard reset",								MonitorReset},
	{"rb",		"Read memory (byte)",							MonitorRead},
	{"rw",		"Read memory (word/uint16)",  					MonitorRead},
	{"rd",		"Read memory (dword/uint32)", 					MonitorRead},
	{"help",	"List all commands",							MonitorHelp},
	{"time",	"Show time/date",								MonitorTime},
	{"date",	"Show time/date",								MonitorDate},
	{"stack",	"Show information on heap and stack",			MonitorStack},
	{"memtest",	"Perform a data line, address line, and cell test",	MonitorMemoryTest},
	{"burstr",	"Burst read function",							MonitorBurstRead},
	{"burstw",	"Burst write function",							MonitorBurstWrite},
	{"iderd",  	"Read IDE sectors",								MonitorIDERd},
	{"idewr",  	"Write IDE sectors",							MonitorIDEWr},
	{"idetest",	"Speed test for IDE drive",						MonitorIDETest},
	{"iderw",	"IDE read/write test",							MonitorIDEReadWriteTest},
	{"ideprobe","Rescan the IDE bus",							MonitorIDEProbe},
	{"intmask",	"Read or set the interrupt mask",				MonitorIntMask},
	{"counters","Show various system counters",					MonitorCounters},
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
		printf("%-12s - %s\r\n", sg_sMonitorCommands[u16Loop].peCommandString, sg_sMonitorCommands[u16Loop].peDescription);
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

