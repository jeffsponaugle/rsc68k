#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "Hardware/RSC68k.h"
#include "Shared/Shared.h"
#include "Shared/Flash.h"

void _exit(int status)
{
	__asm("stop	#0x2700");

	// This makes the compiler happy
	while (1);
}

// Binary to POST hex digit table
const uint8_t sg_u8LEDHex[] =
{
	POST_7SEG_HEX_0,
	POST_7SEG_HEX_1,
	POST_7SEG_HEX_2,
	POST_7SEG_HEX_3,
	POST_7SEG_HEX_4,
	POST_7SEG_HEX_5,
	POST_7SEG_HEX_6,
	POST_7SEG_HEX_7,
	POST_7SEG_HEX_8,
	POST_7SEG_HEX_9,
	POST_7SEG_HEX_A,
	POST_7SEG_HEX_B,
	POST_7SEG_HEX_C,
	POST_7SEG_HEX_D,
	POST_7SEG_HEX_E,
	POST_7SEG_HEX_F
};

// Finds the flash table in the boot loader and copies it into local heap
// for later use.
void FlashTableCopy(SImageVersion *psBootLoaderInfo,
					bool bAddFlashOffset)
{
	EVersionCode eVersionCode;
	const SFlashSegment *psFlashMap;
	const SFlashSegment *psFlashMapOriginal;
	uint32_t u32FlashMapSizeBytes;
	SImageVersion *psBootLoaderVersionStruct;
	uint8_t *pu8BootLoaderBase = NULL;

	// If we're pulling it from flash directly, point to the bootloader in flash (base)
	if (bAddFlashOffset)
	{
		pu8BootLoaderBase += RSC68KHW_SUPERVISOR_FLASH;
	}

	// Go find the boot loader version info. It'll be in the lower 128K of RAM.
	eVersionCode = VersionFindStructure((uint8_t *) pu8BootLoaderBase,
										RSC68KHW_BOOTLOADER_SIZE,
										EIMGTYPE_BOOT_LOADER,
										&psBootLoaderVersionStruct);
	// If it's not OK, then exit
	if (eVersionCode != EVERSION_OK)
	{
		printf("\r\nUnable to locate boot loader version structure - %s\r\n", VersionCodeGetText(eVersionCode));

		// Bail out
		exit(0);
	}

	// Image specific pointer is a pointer to the flash table
	psFlashMapOriginal = (const SFlashSegment *) psBootLoaderVersionStruct->pvImageSpecificData;

	// If we're wanting it directly out of the flash page, then add the flash offset to it
	// so we don't fetch the RAM copy
	if (bAddFlashOffset)
	{
		psFlashMapOriginal = (const SFlashSegment *) (((uint32_t) psFlashMapOriginal) + RSC68KHW_SUPERVISOR_FLASH);
	}

	// Figure out how big the flash segment table is
	u32FlashMapSizeBytes = FlashCountSegmentTable(psFlashMapOriginal) * sizeof(*psFlashMapOriginal);

	// Allocate some memory
	psFlashMap = (const SFlashSegment *) malloc(u32FlashMapSizeBytes);
	if (NULL == psFlashMap)
	{
		printf("\r\nFailed to allocate %u bytes for flash segment table\r\n", u32FlashMapSizeBytes);

		// Bail out
		exit(0);
	}

	// Copy the flash table
	memcpy((void *) psFlashMap, (void *) psFlashMapOriginal, u32FlashMapSizeBytes);

	if (psBootLoaderInfo)
	{
		// Copy the boot loader version info if the caller wants it
		memcpy((void *) psBootLoaderInfo, (void *) psBootLoaderVersionStruct, sizeof(*psBootLoaderInfo));
	}

	// Init the flash
	FlashInit(psFlashMap);
}

void SharedSleep(volatile uint32_t u32Microseconds)
{
	u32Microseconds >>= 2;

	while (u32Microseconds)
	{
		u32Microseconds--;
	}
}

//
// Generic hex dump (to console) routine:
//
// u32Offset 			= Starting offset of data
// u32BytesToDump		= How many bytes to dump
// u16LinesToDump		= Number of lines to dump (0=disabled)
// pu8DataPtr			= Pointer to data to dump
// bRepeatDataSilence	= Repeat data will result in a * display
// TerminateCallback	= Callback between lines to check termination (for key hit, etc...)

static const char *sg_eHexTable = "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f202122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f404142434445464748494a4b4c4d4e4f505152535455565758595a5b5c5d5e5f606162636465666768696a6b6c6d6e6f707172737475767778797a7b7c7d7e7f808182838485868788898a8b8c8d8e8f909192939495969798999a9b9c9d9e9fa0a1a2a3a4a5a6a7a8a9aaabacadaeafb0b1b2b3b4b5b6b7b8b9babbbcbdbebfc0c1c2c3c4c5c6c7c8c9cacbcccdcecfd0d1d2d3d4d5d6d7d8d9dadbdcdddedfe0e1e2e3e4e5e6e7e8e9eaebecedeeeff0f1f2f3f4f5f6f7f8f9fafbfcfdfeff";

EStatus DumpHex(uint32_t u32Offset,
				uint32_t u32BytesToDump,
				uint16_t u16LinesToDump,
				uint8_t *pu8DataPtr,
				uint32_t *pu32FinalAddress,
				bool bRepeatDataSilence,
				bool (*TerminateCallback)(void))
{
	uint8_t u8LineBufferPrior[16];
	uint8_t u8LineBuffer[16];
	uint32_t u32AddressFrom;
	uint32_t u32AddressTo;
	uint32_t u32Loop;
	bool bLineBufferPrimed = false;
	bool bRepeatActive = false;
	EStatus eStatus = ESTATUS_OK;

	// Quantize to 16 byte increments as we dump
	u32Loop = ((uint32_t) pu8DataPtr) & ~(sizeof(u8LineBuffer) - 1);
	u32AddressTo = ((uint32_t) pu8DataPtr) + u32BytesToDump;

	while (u32Loop < u32AddressTo)
	{
		char eDumpLine[90];
		uint8_t u8ColumnCounter;
		char *pePtr;

		// Start the hex dump counter 
		pePtr = eDumpLine;
		memcpy((void *) u8LineBuffer, (void *) u32Loop, sizeof(u8LineBuffer));

		if ((bLineBufferPrimed) && (bRepeatDataSilence))
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
				if ((u32Loop < ((uint32_t) pu8DataPtr)) ||
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
				if ((u32Loop < ((uint32_t) pu8DataPtr)) ||
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
			printf("%.6x: %s\r\n", u32Offset, eDumpLine);
			u32Offset += sizeof(u8LineBuffer);

			if (u16LinesToDump)
			{
				u16LinesToDump--;
				if (0 == u16LinesToDump)
				{
					eStatus = ESTATUS_OK;
					goto errorExit;
				}
			}
		}
		else
		{
			u32Loop += sizeof(u8LineBuffer);
			u32Offset += sizeof(u8LineBuffer);
		}

		// Look for input of any kind, and if we get it, stop the dump
		if (TerminateCallback)
		{
			if (TerminateCallback())
			{
				eStatus = ESTATUS_CANCELED;
				goto errorExit;
			}
		}
		
		// Make a copy for the next 
		memcpy((void *) u8LineBufferPrior, (void *) u8LineBuffer, sizeof(u8LineBufferPrior));
		bLineBufferPrimed = true;
	}

errorExit:
	if (pu32FinalAddress)
	{
		*pu32FinalAddress = u32Loop;
	}

	return (eStatus);
}


typedef struct SEStatusToText
{
	EStatus eStatus;
	const char *peText;
} SEStatusToText;

// Table to convert enums to textual error strings
static const SEStatusToText sg_sErrors[] =
{
	{ESTATUS_OK,											"Ok"},

	// Misc errors
	{ESTATUS_OUT_OF_MEMORY,									"Out of memory"},
	{ESTATUS_MISSING_FUNCTION,								"Missing function"},
	{ESTATUS_BAD_LENGTH,									"Bad length"},
	{ESTATUS_CTRL_C,										"Ctrl-C pressed"},
	{ESTATUS_TIMEOUT,										"Timeout"},
	{ESTATUS_CANCELED,										"Canceled"},

	// Serial port result codes
	{ESTATUS_SERIAL_HANDLE_OUT_OF_RANGE,					"Serial handle out of range"},

	// Console result codes
	{ESTATUS_CONSOLE_ENUM_INVALID,							"Console enumeration invalid"},

	// System result codes
	{ESTATUS_SYSTEM_NOT_SUPERVISOR_CPU,						"Not supervisor CPU"},

	// Flash programming result codes
	{ESTATUS_FLASH_PROGRAMMING_FAULT,						"Flash programming fault"},

	// Monitor result codes
	{ESTATUS_MONITOR_START_LARGER_THAN_END_ADDRESS,			"Start address larger than end address"},
	{ESTATUS_MONITOR_ADDRESS_TOO_HIGH,						"Address too high"},
	{ESTATUS_MONITOR_ADDRESS_EXPECTED,						"Address expected"},
	{ESTATUS_MONITOR_EQUALS_EXPECTED,						"Equals expected"},
	{ESTATUS_MONITOR_DATA_EXPECTED,							"Data expected"},
	{ESTATUS_MONITOR_DATA_SIZE_OUT_OF_RANGE,				"Data size out of range"},

	// Parse result codes
	{ESTATUS_PARSE_INT16_EXPECTED,							"16 Bit integer expected"},
	{ESTATUS_PARSE_INT32_EXPECTED,							"32 Bit integer expected"},
	{ESTATUS_PARSE_INT64_EXPECTED,							"64 Bit integer expected"},
	{ESTATUS_PARSE_INT8_EXPECTED,							"8 Bit integer expected"},
	{ESTATUS_PARSE_NUMERIC_VALUE_EXPECTED,					"Numeric value expected"},
	{ESTATUS_PARSE_SEMICOLON_EXPECTED,						"Semicolon expected"},
	{ESTATUS_PARSE_STRING_EXPECTED,							"String expected"},
	{ESTATUS_PARSE_UINT16_EXPECTED,							"16 Bit integer expected"}, 
	{ESTATUS_PARSE_UINT32_EXPECTED,                         "32 Bit integer expected"}, 
	{ESTATUS_PARSE_UINT64_EXPECTED,                         "64 Bit integer expected"}, 
	{ESTATUS_PARSE_VALUE_OUT_OF_RANGE,  					"Value out of range"},

	// Disk errors
	{ESTATUS_DISK_ERROR,									"Disk error"},
	{ESTATUS_DISK_WRITE_FAULT,								"Disk write fault"},
	{ESTATUS_DISK_DATA_READY,								"Disk data ready"},
	{ESTATUS_DISK_OUT_OF_RANGE,								"Disk # out of range"},
	{ESTATUS_DISK_NOT_PRESENT,								"Disk not present"},
	{ESTATUS_DISK_SECTOR_OUT_OF_RANGE,						"Sector out of range"},
	{ESTATUS_DISK_BUFFER_NOT_ALIGNED,						"Buffer not 4 byte aligned"},
	{ESTATUS_DISK_SECTOR_COUNT_0,							"Sector count can't be 0"},
	{ESTATUS_PARSE_ERROR,									"Parse error"},

	// Protocol related
	{ESTATUS_PROTOCOL_BLOCK_FAULT,							"Protocol block fault"},
	{ESTATUS_PROTOCOL_BAD_CRC,								"Bad CRC16"},
	{ESTATUS_PROTOCOL_WRONG_BLOCK,							"Unexpected block #"},

	// Interrupt related
	{ESTATUS_INTERRUPT_VECTOR_UNKNOWN,						"Interrupt vector unknown"},
	{ESTATUS_NO_INTERRUPTS,									"No interrupts on device"},

	// Serial port out of range
	{ESTATUS_SERIAL_OUT_OF_RANGE,							"Serial port out of range"},
	{ESTATUS_SERIAL_INTERRUPT_NONFUNCTIONAL,				"Serial port interrupt nonfunctional"},

	// RTC related
	{ESTATUS_RTC_NOT_PRESENT,								"RTC Not present"},

	// PTC related
	{ESTATUS_PTC_NOT_PRESENT,								"PTC Not present"},
	{ESTATUS_PTC_RATE_OUT_OF_RANGE,							"PTC Rate not possible"},
	{ESTATUS_PTC_NOT_INT_CAPABLE,							"PTC Channel not interrupt capable"},
	{ESTATUS_PTC_CHANNEL_OUT_OF_RANGE,						"PTC Channel out of range"},
	{ESTATUS_PTC_CHANNEL_NONFUNCTIONAL,						"PTC Channel not functional"},
	{ESTATUS_PTC_INTERRUPT_NONFUNCTIONAL,					"PTC Channel interrupt not functional"},
};

static char sg_eErrorString[60];

const char *GetErrorText(EStatus eStatus)
{
	uint32_t u32Loop;

	for (u32Loop = 0; u32Loop < (sizeof(sg_sErrors) / sizeof(sg_sErrors[0])); u32Loop++)
	{
		if (sg_sErrors[u32Loop].eStatus == eStatus)
		{
			return(sg_sErrors[u32Loop].peText);
		}
	}

	snprintf(sg_eErrorString, sizeof(sg_eErrorString) - 1, "Unknown eStatus 0x%.8x", eStatus);
	return((const char *) sg_eErrorString);
}

