#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "Hardware/RSC68k.h"
#include "Shared/Version.h"
#include "Shared/16550.h"
#include "Shared/Flash.h"

// Flash map for the RSC68K - 1MB of flash (two 512K parts)
const SFlashSegment sg_sRSC68KFlashMap[] =
{
	// Boot loader
	{EFLASHREGION_BOOT_LOADER,		  0,	0x20000,	RSC68KHW_SUPERVISOR_FLASH,			0,	0},	// SA0

	// Operational image #1 (384K)
	{EFLASHREGION_OP_1,				  0,	0x20000,	RSC68KHW_SUPERVISOR_FLASH + 0x20000, 0,	0},	// SA1
	{EFLASHREGION_OP_1,			0x20000,	0x20000,	RSC68KHW_SUPERVISOR_FLASH + 0x40000, 0, 0},	// SA2
	{EFLASHREGION_OP_1,			0x40000,	0x20000,	RSC68KHW_SUPERVISOR_FLASH + 0x60000, 0, 0},	// SA3

	// SA4 Is skipped on purppose - unused

	// Operational image #2 (384K)
	{EFLASHREGION_OP_2,				  0,	0x20000,	RSC68KHW_SUPERVISOR_FLASH + 0x20000, 1, 1},	// SA5
	{EFLASHREGION_OP_2,			0x20000,	0x20000,	RSC68KHW_SUPERVISOR_FLASH + 0x40000, 1, 1},	// SA6
	{EFLASHREGION_OP_2,			0x40000,	0x20000,	RSC68KHW_SUPERVISOR_FLASH + 0x60000, 1, 1},	// SA7

	// End of segments
	{EFLASHREGION_END}
};

void _exit(int status)
{
	__asm("stop	#0x2700");

	// This makes the compiler happy
	while (1);
}

// This routine will perform the following tasks:
//
// * Determines if the flash region has some content
// * If it does, looks for the version structure within it.
// * If the version structure fails integrity, the region gets erased
//
static EVersionCode FlashRegionCheck(EFlashRegion eFlashRegion,
									 SImageVersion **ppsImageVersion)
{
	EVersionCode eVersionCode;
	uint32_t u32FlashSignature[2];
	uint32_t *pu32FlashPointer;
	uint32_t u32FlashRegionSize;
	bool bResult;

	if (ppsImageVersion)
	{
		*ppsImageVersion = NULL;
	}

	// See if the first 8 bytes of the region are all 0xffs or not
	bResult = FlashReadRegion(eFlashRegion,
							  0,
							  (uint8_t *) u32FlashSignature,
							  sizeof(u32FlashSignature));

	if (false == bResult)
	{
		exit(0);
	}

	// If the first 8 bytes are 0xff, then there's nothing here - just return
	if ((0xffffffff == u32FlashSignature[0]) &&
		(0xffffffff == u32FlashSignature[1]))
	{
		eVersionCode = EVERSION_BAD_STRUCT_CRC;
		goto errorExit;
	}

	// There's something there. Let's run through the region and see if we can
	// find the version structure.

	bResult = FlashRegionGetSize(eFlashRegion,
								 &u32FlashRegionSize);
	if (false == bResult)
	{
		exit(0);
	}

	// Get the region's pointer, which will also cause it to be mapped in if it's not
	bResult = FlashGetRegionPointer(eFlashRegion,
									0,
									(uint8_t **) &pu32FlashPointer);
	if (false == bResult)
	{
		exit(0);
	}

	// Go find the version structure.
	u32FlashRegionSize >>= 2; // Convert to # of UINT32s
	while (u32FlashRegionSize)
	{
		// Let's see if this is a signature we recognize that warrants further testing
		if (VERSION_PREFIX == *pu32FlashPointer)
		{
			// We might have something! 
			eVersionCode = VersionValidateStructure((SImageVersion *) pu32FlashPointer);
			if (EVERSION_OK == eVersionCode)
			{
				// We found it!
				if (ppsImageVersion)
				{
					*ppsImageVersion = (SImageVersion *) pu32FlashPointer;
				}

				goto errorExit;
			}
		}

		// Move on to the next UINT32
		++pu32FlashPointer;
		u32FlashRegionSize--;
	}

	// Didn't find it, but we found stuff in this region. Let's erase it.
	bResult = FlashRegionErase(eFlashRegion);

	if (false == bResult)
	{
		printf("\r\nFlashRegionErase for region %u failed\r\n", eFlashRegion);
		exit(0);
	}

	// Let's indicate we didn't find anything.
	eVersionCode = EVERSION_BAD_STRUCT_CRC;

errorExit:
	return (eVersionCode);
}

// Binary to POST hex digit table
static const uint8_t sg_u8LEDHex[] =
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

#define	POST_HEX(x) POST_SET((sg_u8LEDHex[(x) >> 4] << 8) | sg_u8LEDHex[(x) & 0x0f])

// Main entry point for boot loader
void main(void)
{
	EVersionCode eVersionCode;
	EVersionCode eVersionOp1;
	SImageVersion sVersionStructOp1;
	EVersionCode eVersionOp2;
	SImageVersion sVersionStructOp2;
	SImageVersion *psVersion;
	EFlashRegion eFlashRegion;
	uint32_t u32Address;
	uint32_t u32Expected;
	uint32_t u32Got;
	bool bResult;

	// Indicate that we're at main() on the POST code LEDs
	POST_SET(POSTCODE_BOOTLOADER_MAIN);

 	// See if the version code is OK for the boot loader
	eVersionCode = VersionValidateStructure((SImageVersion *) &g_sImageVersion);

	// If it's not OK, then emit an error to the console and halt
	if (eVersionCode != EVERSION_OK)
	{
		printf("\r\nBoot loader version structure validation fault - %s\r\n", VersionCodeGetText(eVersionCode));

		// Bail out
		exit(0);
	}

	// Indicate we're now looking for a good operational BIOS image
	POST_SET(POSTCODE_BOOTLOADER_OP_IMAGE_SEARCH);

	// Init the flash subsystem
	FlashInit(sg_sRSC68KFlashMap);

	// Look at OP 1
	POST_SET(POSTCODE_BOOTLOADER_OP_IMAGE_SEARCH_OP1);
	eVersionOp1 = FlashRegionCheck(EFLASHREGION_OP_1,
								   &psVersion);
	if (EVERSION_OK == eVersionOp1)
	{
		// Make a copy of the version structure if we have one
		memcpy((void *) &sVersionStructOp1, (void *) psVersion, sizeof(sVersionStructOp1));
	}

	// Now at Op 2
	POST_SET(POSTCODE_BOOTLOADER_OP_IMAGE_SEARCH_OP2);
	eVersionOp2 = FlashRegionCheck(EFLASHREGION_OP_2,
								   &psVersion);
	if (EVERSION_OK == eVersionOp2)
	{
		// Make a copy of the version structure if we have one
		memcpy((void *) &sVersionStructOp2, (void *) psVersion, sizeof(sVersionStructOp2));
	}


	// If we can't proceed because of two bad images, alert the user
	if ((eVersionOp1 != EVERSION_OK) &&
		(eVersionOp2 != EVERSION_OK))
	{
		printf("\r\nNo bootable flash images:\r\n Op1 Code=%s\r\n Op2 Code=%s\r\n", VersionCodeGetText(eVersionOp1), VersionCodeGetText(eVersionOp2));

		// Bail out
		exit(0);
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
			printf("\r\nFlashRegionErase for region %u failed\r\n", eFlashRegion);
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
		printf("\r\nBoot loader internal boot region choice fault\r\n");
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
		printf("\r\nFailed to copy BIOS from op image %u\r\n", eFlashRegion);
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
		printf("\r\nFailed to verify BIOS from op image %u -\r\n Address 0x%.8x - expected 0x%.8x, got 0x%.8x\r\n", (eFlashRegion - EFLASHREGION_OP_1) + 1, u32Address, u32Expected, u32Got);
		exit(0);
	}

	// We now have paged out the flash part and are running completely out of RAM
	RAM_PAGE_READ_SET();
	RAM_PAGE_WRITE_SET();

	// Since we're the boot loader, poke in the supervisor slot ID into the system BIOS's slot ID
	// variable.
	*((uint8_t *) psVersion->pvImageSpecificData) = SLOT_ID_SUPERVISOR;

	// Transferring control to the operational image!
	POST_SET(POSTCODE_BOOTLOADER_OP_IMAGE_EXEC);
	((void (*)(void))psVersion->u32EntryPoint)();

	// SHOULD NOT GET HERE
	printf("\r\nReturned from dispatch to operational image - this should not happen!\r\n");
	exit(0);
	
}

