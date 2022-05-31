#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "Hardware/RSC68k.h"
#include "BIOS/OS.h"
#include "Shared/Version.h"
#include "Shared/16550.h"
#include "Shared/Flash.h"
#include "Shared/LinkerDefines.h"
#include "Shared/LineInput.h"
#include "Shared/Monitor.h"
#include "BIOS/BIOS.h"

void _exit(int status)
{
	__asm("stop     #0x2700");

	// This makes the compiler happy
	while (1);
}

// Our slot ID (uninitialized by default - filled in by boot process)
uint8_t g_u8SlotID = SLOT_ID_UNINITIALIZED;

// Making a copy of our boot loader version info (if applicable)
static SImageVersion sg_sBootLoaderVersionInfo;

// Finds the flash table in the boot loader and copies it into local heap
// for later use.
static void FlashTableCopy(void)
{
	EVersionCode eVersionCode;
	const SFlashSegment *psFlashMap;
	const SFlashSegment *psFlashMapOriginal;
	uint32_t u32FlashMapSizeBytes;
	SImageVersion *psBootLoaderVersionStruct;

	// Go find the boot loader version info. It'll be in the lower 128K of RAM.
	eVersionCode = VersionFindStructure((uint8_t *) NULL,
										128*1024,
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

	// Copy the boot loader version info
	memcpy((void *) &sg_sBootLoaderVersionInfo, (void *) psBootLoaderVersionStruct, sizeof(sg_sBootLoaderVersionInfo));

	// Init the flash
	FlashInit(psFlashMap);
}

// Main entry point for BIOS
void main(void)
{
	EVersionCode eVersionCode;
	EStatus eStatus;

	// Indicate that we're at main() on the POST code LEDs
	POST_SET(POSTCODE_BIOS_MAIN);

	// See if the version code is OK for the BIOS
	eVersionCode = VersionValidateStructure((SImageVersion *) &g_sImageVersion);

	// If it's not OK, then emit an error to the console and halt
	if (eVersionCode != EVERSION_OK)
	{
		printf("\r\nBIOS version structure validation fault - %s\r\n", VersionCodeGetText(eVersionCode));

		// Bail out
		exit(0);
	}

	// If we're the supervisor CPU, then find and make a copy of the flash table
	if (SLOT_ID_SUPERVISOR == g_u8SlotID)
	{
		// Copy the flash table out of the boot loader
		POST_SET(POSTCODE_BIOS_FLASH_COPY);
		FlashTableCopy();
	}
	else
	{
		// Skip it if it's a worker CPU
	}

	// Console initialize!
	POST_SET(POSTCODE_BIOS_CONSOLE_INIT);
	printf("\033[2JRSC68K BIOS Version %u.%u.%u\r\n",
		   g_sImageVersion.u32MajorMinorBuildNumber >> 24,
		   (g_sImageVersion.u32MajorMinorBuildNumber >> 16) & 0xff,
		   (uint16_t)g_sImageVersion.u32MajorMinorBuildNumber);

	// If we're the supervisor CPU, then display the boot loader info
	if (SLOT_ID_SUPERVISOR == g_u8SlotID)
	{
		printf("Boot loader Version %u.%u.%u\r\n",
		   sg_sBootLoaderVersionInfo.u32MajorMinorBuildNumber >> 24,
		   (sg_sBootLoaderVersionInfo.u32MajorMinorBuildNumber >> 16) & 0xff,
		   (uint16_t)sg_sBootLoaderVersionInfo.u32MajorMinorBuildNumber);
	}

	// Display CPU ID information
	if (SLOT_ID_SUPERVISOR == g_u8SlotID)
	{
		printf("CPU: Supervisor\r\n");
	}
	else
	if ((g_u8SlotID >= SLOT_ID_BASE) && (g_u8SlotID < SLOT_ID_MAX))
	{
		printf("CPU: Worker #%u\r\n", g_u8SlotID - SLOT_ID_BASE);
	}
	else
	if (SLOT_ID_UNINITIALIZED == g_u8SlotID)
	{
		printf("CPU Slot ID not initialized - halting\r\n");
		exit(0);
	}
	else
	{
		printf("CPU Slot ID byte unknown - 0x%.2x\r\n - halting\r\n", g_u8SlotID);
		exit(0);
	}

	// Start the in-flash monitor
	eStatus = MonitorStart();

	// End of everything
	exit(0);
}

