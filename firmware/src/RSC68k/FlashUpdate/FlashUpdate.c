#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "BIOS/OS.h"
#include "Hardware/RSC68k.h"
#include "Shared/Version.h"
#include "Shared/16550.h"
#include "Shared/Flash.h"
#include "Shared/Monitor.h"
#include "Shared/Shared.h"
#include "Shared/Monitor.h"
#include "Shared/SharedFlashUpdate.h"

uint8_t g_u8SlotID = SLOT_ID_SUPERVISOR;

// Main entry point for boot loader
void main(void)
{
	EVersionCode eVersionOp1;
	SImageVersion sVersionStructOp1;
	EVersionCode eVersionOp2;
	SImageVersion sVersionStructOp2;
	const SFlashSegment *psFlashMap = NULL;
	EFlashRegion eUpdateRegion = EFLASHREGION_END;
	bool bResult;
	EStatus eStatus;

	POST_SET(POSTCODE_FWUPD_START_MAIN);

	printf("\r\nFlash update utility - %u.%u.%u\r\n",
		   g_sImageVersion.u32MajorMinorBuildNumber >> 24,
		   (g_sImageVersion.u32MajorMinorBuildNumber >> 16) & 0xff,
		   (uint16_t)g_sImageVersion.u32MajorMinorBuildNumber);

	SharedFlashUpdate();
}

