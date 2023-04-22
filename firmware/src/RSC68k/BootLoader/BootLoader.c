#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "Hardware/RSC68k.h"
#include "Shared/Version.h"
#include "BIOS/OS.h"
#include "Shared/16550.h"
#include "Shared/Flash.h"
#include "Shared/Monitor.h"
#include "Shared/Shared.h"
#include "Shared/SharedFlashUpdate.h"
#include "Shared/Interrupt.h"
#include "Shared/IDE.h"

uint8_t g_u8SlotID = SLOT_ID_UNINITIALIZED;

// Flash map for the RSC68K - 1MB of flash (two 512K parts)
const SFlashSegment sg_sRSC68KFlashMap[] =
{
	// Boot loader - 128K
	{EFLASHREGION_BOOT_LOADER, 0x000000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x000000, 0, 0},
	{EFLASHREGION_BOOT_LOADER, 0x002000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x002000, 0, 0},
	{EFLASHREGION_BOOT_LOADER, 0x004000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x004000, 0, 0},
	{EFLASHREGION_BOOT_LOADER, 0x006000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x006000, 0, 0},
	{EFLASHREGION_BOOT_LOADER, 0x008000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x008000, 0, 0},
	{EFLASHREGION_BOOT_LOADER, 0x00a000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x00a000, 0, 0},
	{EFLASHREGION_BOOT_LOADER, 0x00c000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x00c000, 0, 0},
	{EFLASHREGION_BOOT_LOADER, 0x00e000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x00e000, 0, 0},
	{EFLASHREGION_BOOT_LOADER, 0x010000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x010000, 0, 0},
	{EFLASHREGION_BOOT_LOADER, 0x012000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x012000, 0, 0},
	{EFLASHREGION_BOOT_LOADER, 0x014000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x014000, 0, 0},
	{EFLASHREGION_BOOT_LOADER, 0x016000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x016000, 0, 0},
	{EFLASHREGION_BOOT_LOADER, 0x018000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x018000, 0, 0},
	{EFLASHREGION_BOOT_LOADER, 0x01a000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x01a000, 0, 0},
	{EFLASHREGION_BOOT_LOADER, 0x01c000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x01c000, 0, 0},
	{EFLASHREGION_BOOT_LOADER, 0x01e000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x01e000, 0, 0},

	// Operational image #1 - 448K
	{EFLASHREGION_OP_1, 0x000000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x020000, 0, 0},
	{EFLASHREGION_OP_1, 0x002000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x022000, 0, 0},
	{EFLASHREGION_OP_1, 0x004000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x024000, 0, 0},
	{EFLASHREGION_OP_1, 0x006000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x026000, 0, 0},
	{EFLASHREGION_OP_1, 0x008000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x028000, 0, 0},
	{EFLASHREGION_OP_1, 0x00a000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x02a000, 0, 0},
	{EFLASHREGION_OP_1, 0x00c000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x02c000, 0, 0},
	{EFLASHREGION_OP_1, 0x00e000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x02e000, 0, 0},
	{EFLASHREGION_OP_1, 0x010000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x030000, 0, 0},
	{EFLASHREGION_OP_1, 0x012000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x032000, 0, 0},
	{EFLASHREGION_OP_1, 0x014000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x034000, 0, 0},
	{EFLASHREGION_OP_1, 0x016000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x036000, 0, 0},
	{EFLASHREGION_OP_1, 0x018000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x038000, 0, 0},
	{EFLASHREGION_OP_1, 0x01a000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x03a000, 0, 0},
	{EFLASHREGION_OP_1, 0x01c000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x03c000, 0, 0},
	{EFLASHREGION_OP_1, 0x01e000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x03e000, 0, 0},
	{EFLASHREGION_OP_1, 0x020000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x040000, 0, 0},
	{EFLASHREGION_OP_1, 0x022000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x042000, 0, 0},
	{EFLASHREGION_OP_1, 0x024000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x044000, 0, 0},
	{EFLASHREGION_OP_1, 0x026000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x046000, 0, 0},
	{EFLASHREGION_OP_1, 0x028000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x048000, 0, 0},
	{EFLASHREGION_OP_1, 0x02a000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x04a000, 0, 0},
	{EFLASHREGION_OP_1, 0x02c000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x04c000, 0, 0},
	{EFLASHREGION_OP_1, 0x02e000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x04e000, 0, 0},
	{EFLASHREGION_OP_1, 0x030000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x050000, 0, 0},
	{EFLASHREGION_OP_1, 0x032000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x052000, 0, 0},
	{EFLASHREGION_OP_1, 0x034000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x054000, 0, 0},
	{EFLASHREGION_OP_1, 0x036000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x056000, 0, 0},
	{EFLASHREGION_OP_1, 0x038000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x058000, 0, 0},
	{EFLASHREGION_OP_1, 0x03a000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x05a000, 0, 0},
	{EFLASHREGION_OP_1, 0x03c000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x05c000, 0, 0},
	{EFLASHREGION_OP_1, 0x03e000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x05e000, 0, 0},
	{EFLASHREGION_OP_1, 0x040000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x060000, 0, 0},
	{EFLASHREGION_OP_1, 0x042000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x062000, 0, 0},
	{EFLASHREGION_OP_1, 0x044000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x064000, 0, 0},
	{EFLASHREGION_OP_1, 0x046000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x066000, 0, 0},
	{EFLASHREGION_OP_1, 0x048000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x068000, 0, 0},
	{EFLASHREGION_OP_1, 0x04a000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x06a000, 0, 0},
	{EFLASHREGION_OP_1, 0x04c000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x06c000, 0, 0},
	{EFLASHREGION_OP_1, 0x04e000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x06e000, 0, 0},
	{EFLASHREGION_OP_1, 0x050000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x070000, 0, 0},
	{EFLASHREGION_OP_1, 0x052000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x072000, 0, 0},
	{EFLASHREGION_OP_1, 0x054000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x074000, 0, 0},
	{EFLASHREGION_OP_1, 0x056000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x076000, 0, 0},
	{EFLASHREGION_OP_1, 0x058000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x078000, 0, 0},
	{EFLASHREGION_OP_1, 0x05a000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x07a000, 0, 0},
	{EFLASHREGION_OP_1, 0x05c000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x07c000, 0, 0},
	{EFLASHREGION_OP_1, 0x05e000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x07e000, 0, 0},
	{EFLASHREGION_OP_1, 0x060000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x000000, 1, 1},
	{EFLASHREGION_OP_1, 0x062000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x002000, 1, 1},
	{EFLASHREGION_OP_1, 0x064000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x004000, 1, 1},
	{EFLASHREGION_OP_1, 0x066000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x006000, 1, 1},
	{EFLASHREGION_OP_1, 0x068000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x008000, 1, 1},
	{EFLASHREGION_OP_1, 0x06a000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x00a000, 1, 1},
	{EFLASHREGION_OP_1, 0x06c000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x00c000, 1, 1},
	{EFLASHREGION_OP_1, 0x06e000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x00e000, 1, 1},

	// Operational image #2 - 448K
	{EFLASHREGION_OP_2, 0x000000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x010000, 1, 1},
	{EFLASHREGION_OP_2, 0x002000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x012000, 1, 1},
	{EFLASHREGION_OP_2, 0x004000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x014000, 1, 1},
	{EFLASHREGION_OP_2, 0x006000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x016000, 1, 1},
	{EFLASHREGION_OP_2, 0x008000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x018000, 1, 1},
	{EFLASHREGION_OP_2, 0x00a000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x01a000, 1, 1},
	{EFLASHREGION_OP_2, 0x00c000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x01c000, 1, 1},
	{EFLASHREGION_OP_2, 0x00e000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x01e000, 1, 1},
	{EFLASHREGION_OP_2, 0x010000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x020000, 1, 1},
	{EFLASHREGION_OP_2, 0x012000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x022000, 1, 1},
	{EFLASHREGION_OP_2, 0x014000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x024000, 1, 1},
	{EFLASHREGION_OP_2, 0x016000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x026000, 1, 1},
	{EFLASHREGION_OP_2, 0x018000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x028000, 1, 1},
	{EFLASHREGION_OP_2, 0x01a000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x02a000, 1, 1},
	{EFLASHREGION_OP_2, 0x01c000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x02c000, 1, 1},
	{EFLASHREGION_OP_2, 0x01e000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x02e000, 1, 1},
	{EFLASHREGION_OP_2, 0x020000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x030000, 1, 1},
	{EFLASHREGION_OP_2, 0x022000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x032000, 1, 1},
	{EFLASHREGION_OP_2, 0x024000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x034000, 1, 1},
	{EFLASHREGION_OP_2, 0x026000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x036000, 1, 1},
	{EFLASHREGION_OP_2, 0x028000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x038000, 1, 1},
	{EFLASHREGION_OP_2, 0x02a000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x03a000, 1, 1},
	{EFLASHREGION_OP_2, 0x02c000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x03c000, 1, 1},
	{EFLASHREGION_OP_2, 0x02e000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x03e000, 1, 1},
	{EFLASHREGION_OP_2, 0x030000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x040000, 1, 1},
	{EFLASHREGION_OP_2, 0x032000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x042000, 1, 1},
	{EFLASHREGION_OP_2, 0x034000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x044000, 1, 1},
	{EFLASHREGION_OP_2, 0x036000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x046000, 1, 1},
	{EFLASHREGION_OP_2, 0x038000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x048000, 1, 1},
	{EFLASHREGION_OP_2, 0x03a000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x04a000, 1, 1},
	{EFLASHREGION_OP_2, 0x03c000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x04c000, 1, 1},
	{EFLASHREGION_OP_2, 0x03e000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x04e000, 1, 1},
	{EFLASHREGION_OP_2, 0x040000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x050000, 1, 1},
	{EFLASHREGION_OP_2, 0x042000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x052000, 1, 1},
	{EFLASHREGION_OP_2, 0x044000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x054000, 1, 1},
	{EFLASHREGION_OP_2, 0x046000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x056000, 1, 1},
	{EFLASHREGION_OP_2, 0x048000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x058000, 1, 1},
	{EFLASHREGION_OP_2, 0x04a000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x05a000, 1, 1},
	{EFLASHREGION_OP_2, 0x04c000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x05c000, 1, 1},
	{EFLASHREGION_OP_2, 0x04e000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x05e000, 1, 1},
	{EFLASHREGION_OP_2, 0x050000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x060000, 1, 1},
	{EFLASHREGION_OP_2, 0x052000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x062000, 1, 1},
	{EFLASHREGION_OP_2, 0x054000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x064000, 1, 1},
	{EFLASHREGION_OP_2, 0x056000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x066000, 1, 1},
	{EFLASHREGION_OP_2, 0x058000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x068000, 1, 1},
	{EFLASHREGION_OP_2, 0x05a000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x06a000, 1, 1},
	{EFLASHREGION_OP_2, 0x05c000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x06c000, 1, 1},
	{EFLASHREGION_OP_2, 0x05e000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x06e000, 1, 1},
	{EFLASHREGION_OP_2, 0x060000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x070000, 1, 1},
	{EFLASHREGION_OP_2, 0x062000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x072000, 1, 1},
	{EFLASHREGION_OP_2, 0x064000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x074000, 1, 1},
	{EFLASHREGION_OP_2, 0x066000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x076000, 1, 1},
	{EFLASHREGION_OP_2, 0x068000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x078000, 1, 1},
	{EFLASHREGION_OP_2, 0x06a000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x07a000, 1, 1},
	{EFLASHREGION_OP_2, 0x06c000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x07c000, 1, 1},
	{EFLASHREGION_OP_2, 0x06e000, 0x2000, RSC68KHW_SUPERVISOR_FLASH + 0x07e000, 1, 1},

	// End of segments
	{EFLASHREGION_END}
};

// Main entry point for boot loader
void main(void)
{
	EVersionCode eVersionCode;
	EVersionCode eVersionOp1;
	SImageVersion sVersionStructOp1;
	EVersionCode eVersionOp2;
	SImageVersion sVersionStructOp2;
	SImageVersion *psVersion = NULL;
	EFlashRegion eFlashRegion;
	uint32_t u32Address;
	uint32_t u32Expected;
	uint32_t u32Got;
	bool bResult;

	// Select master IDE drive (assumpution is it's there)
	IDE_REG_HDDEVSEL = IDE_MASTER;

	// Initialize the interrupt subsystem
	InterruptInit();

	// Add a carriage return for every linefeed
	StreamSetAutoCR(true);

	// Init the UART - turn on FIFOs
	bResult = SerialInit((S16550UART *) RSC68KHW_DEVCOM_UARTA,
						 8,
						 1,
						 EUART_PARITY_NONE);
	assert(bResult);

	// Indicate that we're at main() on the POST code LEDs
	POST_SET(POSTCODE_BOOTLOADER_MAIN);

 	// See if the version code is OK for the boot loader
	eVersionCode = VersionValidateStructure((SImageVersion *) &g_sImageVersion);

	// If it's not OK, then emit an error to the console and halt
	if (eVersionCode != EVERSION_OK)
	{
		printf("\nBoot loader version structure validation fault - %s\n", VersionCodeGetText(eVersionCode));

		// Bail out
		exit(0);
	}

	// Indicate we're now looking for a good operational BIOS image
	POST_SET(POSTCODE_BOOTLOADER_OP_IMAGE_SEARCH);

	// Init the flash subsystem
	FlashInit(sg_sRSC68KFlashMap);

	// Send "spin up" command
	IDE_REG_CMD = IDE_CMD_SPINUP;

	// Look at OP 1
	POST_SET(POSTCODE_BOOTLOADER_OP_IMAGE_SEARCH_OP1);
	eVersionOp1 = FlashRegionCheck(EFLASHREGION_OP_1,
								   EIMGTYPE_OPERATIONAL,
								   &sVersionStructOp1,
								   NULL);

	// Now at Op 2
	POST_SET(POSTCODE_BOOTLOADER_OP_IMAGE_SEARCH_OP2);
	eVersionOp2 = FlashRegionCheck(EFLASHREGION_OP_2,
								   EIMGTYPE_OPERATIONAL,
								   &sVersionStructOp2,
								   NULL);

	// Select master IDE drive (assumpution is it's there)
	IDE_REG_HDDEVSEL = IDE_SLAVE;

	// If we can't proceed because of two bad images, alert the user
	if ((eVersionOp1 != EVERSION_OK) &&
		(eVersionOp2 != EVERSION_OK))
	{
		printf("\nNo bootable flash images:\n Op1 Code=%s\n Op2 Code=%s\n", VersionCodeGetText(eVersionOp1), VersionCodeGetText(eVersionOp2));

		printf("Invoking serial port based flash updater\n");

		SharedFlashUpdate();
	}

	// Now we make the choice
	POST_SET(POSTCODE_BOOTLOADER_CHOOSE_OP_IMAGE);

	// If both images are bootable, then erase the oldest one
	if ((EVERSION_OK == eVersionOp1) &&
		(EVERSION_OK == eVersionOp2))
	{
		// Whichever is older, that's what we erase and don't choose.
		if (sVersionStructOp1.u64BuildTimestamp < sVersionStructOp2.u64BuildTimestamp)
		{
			// Invalidate region 1
			eFlashRegion = EFLASHREGION_OP_1;
			eVersionOp1 = EVERSION_BAD_STRUCT_CRC;
		}
		else
		{
			// Invalidate region 2
			eFlashRegion = EFLASHREGION_OP_2;
			eVersionOp2 = EVERSION_BAD_STRUCT_CRC;
		}

		// Now go erase the offending region
		bResult = FlashRegionErase(eFlashRegion);
		if (false == bResult)
		{
			printf("\nFlashRegionErase for region %u failed\n", eFlashRegion);
			exit(0);
		}
	}

	// Figure out which region we should boot
	if (EVERSION_OK == eVersionOp1)
	{
		eFlashRegion = EFLASHREGION_OP_1;
		psVersion = &sVersionStructOp1;
	}
	else
	if (EVERSION_OK == eVersionOp2)
	{
		eFlashRegion = EFLASHREGION_OP_2;
		psVersion = &sVersionStructOp2;
	}
	else
	{
		// How did we get here? This is an algorithmic failure of some sort
		printf("\nBoot loader internal boot region choice fault\n");
		exit(0);
	}

	// We've figured out which region we're booting. Now we need to do a flash copy from
	// flash to RAM.
	POST_SET(POSTCODE_BOOTLOADER_COPY_OP_IMAGE);
	bResult = FlashCopyToRAM(eFlashRegion,
							 (uint8_t *) RSC68KHW_SUPERVISOR_RAM,
							 0,
							 psVersion->u32BssEnd - psVersion->u32LoadAddress);

	if (false == bResult)
	{
		printf("\nFailed to copy BIOS from op image %u\n", eFlashRegion);
		exit(0);
	}

	// Now verify it
	POST_SET(POSTCODE_BOOTLOADER_VERIFY_OP_IMAGE_COPY);
	bResult = FlashVerifyCopy(eFlashRegion,
							  0,
							  psVersion->u32BssEnd - psVersion->u32LoadAddress,
							  &u32Address,
							  &u32Expected,
							  &u32Got);

	if (false == bResult)
	{
		printf("\nFailed to verify BIOS from op image %u -\n Address 0x%.8x - expected 0x%.8x, got 0x%.8x\n", (eFlashRegion - EFLASHREGION_OP_1) + 1, u32Address, u32Expected, u32Got);
		exit(0);
	}

	// We now have paged out the flash part and are running completely out of RAM
	FlashPageRAMReadSet();
	FlashPageRAMWriteSet();

	// Since we're the boot loader, poke in the supervisor slot ID into the system BIOS's slot ID
	// variable.
	*((uint8_t *) psVersion->pvImageSpecificData) = SLOT_ID_SUPERVISOR;

	// Transferring control to the operational image!
	POST_SET(POSTCODE_BOOTLOADER_OP_IMAGE_EXEC);

	// Send "spin up" command
	IDE_REG_CMD = IDE_CMD_SPINUP;

	((void (*)(void))psVersion->u32EntryPoint)();

	// SHOULD NOT GET HERE
	printf("\nReturned from dispatch to operational image - this should not happen!\n");
	exit(0);
	
}

int f_lseek(void)
{

}

int f_close(int foo)
{

}

int f_write(int foo)
{

}

int f_read(int foo)
{

}

int f_open(char *foo, int blah, int foop)
{
}

