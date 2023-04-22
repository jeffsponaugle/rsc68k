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
#include "Shared/Stream.h"
#include "Shared/ptc.h"
#include "Shared/rtc.h"
#include "Shared/YModem.h"

// Structure containing some information about the flash program state
typedef struct SFlashProgramState
{
	bool bFlashRegionPrep;				// Set to true if we need to figure out what region we're going to program
	bool bProgrammingComplete;			// Set to true if programming is complete
	EFlashRegion eProgramRegion;		// Which region are we programming?
} SFlashProgramState;

static EStatus SharedFlashUpdateCallback(EStatus eStatusIncoming,
										 uint32_t u32Offset,
										 uint8_t *pu8RXData,
										 uint16_t u16BytesReceived,
										 void *pvUserData)
{
	EStatus eStatus = ESTATUS_OK;
	SFlashProgramState *psState = (void *) pvUserData;
	bool bResult;

	if (ESTATUS_OK == eStatusIncoming)
	{
		if (0 == u32Offset)
		{
			printf("Firmware transfer started\n");
		}

		if ((u32Offset & 0x1fff) == 0)
		{
			printf("%uK\n", u32Offset >> 10);
		}

		POST_SET(POSTCODE_FWUPD_PROGRAM_BLOCK);
		bResult = FlashProgramRegion(psState->eProgramRegion,
									 u32Offset,
									 pu8RXData,
									 u16BytesReceived);
		if (false == bResult)
		{
			printf("\nFailed to program flash region %u offset 0x%.6x\n", psState->eProgramRegion, u32Offset);
			eStatus = ESTATUS_FLASH_PROGRAMMING_FAULT;
			psState->bFlashRegionPrep = true;
		}
		else
		{
			// Good programming
		}
	}
	else
	if (ESTATUS_TRANSFER_COMPLETE == eStatusIncoming)
	{
		printf("%uK\nFirmware transfer complete\n", u32Offset >> 10);
		psState->bProgrammingComplete = true;
	}
	else
	if (ESTATUS_CANCELED == eStatusIncoming)
	{
		printf("Canceled\n");
		psState->bFlashRegionPrep = true;
		eStatus = eStatusIncoming;
	}
	else
	if (ESTATUS_TIMEOUT == eStatusIncoming)
	{
		printf("Timed out - offset 0x%.8x\n", u32Offset);
		psState->bFlashRegionPrep = true;
		eStatus = eStatusIncoming;
	}

	POST_SET(POSTCODE_FWUPD_BLOCK_RECEIVE);

	return(eStatus);
}

// This prepares a flash region for programming, including erasure if need be
static EFlashRegion SharedFlashPrepareRegion(void)
{
	EVersionCode eVersionOp1;
	SImageVersion sVersionStructOp1;
	EVersionCode eVersionOp2;
	SImageVersion sVersionStructOp2;
	EFlashRegion eUpdateRegion = EFLASHREGION_END;
	bool bResult;

	POST_SET(POSTCODE_FWUPD_REGION_CHECKS);

	printf("Examining BIOS update regions\n");

	eVersionOp1 = FlashRegionCheck(EFLASHREGION_OP_1,
								   EIMGTYPE_OPERATIONAL,
								   &sVersionStructOp1,
								   NULL);
	eVersionOp2 = FlashRegionCheck(EFLASHREGION_OP_2,
								   EIMGTYPE_OPERATIONAL,
								   &sVersionStructOp2,
								   NULL);

	// If both op images are OK, figure out which one is older and erase it
	if ((EVERSION_OK == eVersionOp1) &&
		(EVERSION_OK == eVersionOp2))
	{
		printf("Two images valid - ");
		if (sVersionStructOp2.u64BuildTimestamp < sVersionStructOp1.u64BuildTimestamp)
		{
			// Choose OP2 since it's older
			eUpdateRegion = EFLASHREGION_OP_2;
		}
		else
		{
			// Choose OP1 since it's older or equal
			eUpdateRegion = EFLASHREGION_OP_1;
		}
	}
	else
	{
		if (EVERSION_OK == eVersionOp1)
		{
			// Op2
			eUpdateRegion = EFLASHREGION_OP_2;
		}
		else
		{
			// Op1
			eUpdateRegion = EFLASHREGION_OP_1;
		}
	}

	if (FlashRegionCheckErased(eUpdateRegion))
	{
		// Region is already erased. No need to erase it.
		printf("Updating operational region %u\n", (eUpdateRegion - EFLASHREGION_OP_1) + 1);
	}
	else
	{	
		// Let 'em know which region we'll be updating
		printf("Updating operational region %u - erasing\n", (eUpdateRegion - EFLASHREGION_OP_1) + 1);

		POST_SET(POSTCODE_FWUPD_ERASE_REGION);

		// Erase the region we're going to be programming
		bResult = FlashRegionErase(eUpdateRegion);
		if (false == bResult)
		{
			printf("Failed to erase region\n");
			exit(1);
		}
	}

	POST_SET(POSTCODE_FWUPD_START_TRANSFER);

	printf("Waiting for start of transfer on UART B, baud rate %u\n", FLASH_UPDATE_BAUD_RATE);

	return(eUpdateRegion);
}

void SharedFlashUpdate(void)
{
	const SFlashSegment *psFlashMap = NULL;
	EStatus eStatus;
	uint32_t u32CPUSpeed;
	SYModem sYModem;
	SFlashProgramState sFlashProgramState;

	POST_SET(POSTCODE_FWUPD_START_MAIN);

	// Init the RTC
	eStatus = RTCInit();
	assert(ESTATUS_OK == eStatus);

	// See how fast this CPU is running
	eStatus = RTCGetCPUSpeed(&u32CPUSpeed);
	assert(ESTATUS_OK == eStatus);

	// Turn on the counter/timer
	eStatus = PTCInit(u32CPUSpeed << 1);
	assert(ESTATUS_OK == eStatus);

	// Turn on interrupts for both UARTs
	eStatus = StreamSetConsoleSerialInterruptMode(true);
	if (eStatus != ESTATUS_OK)
	{
		printf("StreamSetConsoleSerialInterruptMode failed - %s\n", GetErrorText(eStatus));
		while (1);
	}

	// Force lower part of flash
	FlashPageReadSet(0);

	// Go init the flash table
	FlashTableCopy(NULL,
				   true);

	// Identify the flash part
	FlashIdentify();

	// Init the transfer UART and get it ready
	ZERO_STRUCT(sYModem);
	ZERO_STRUCT(sFlashProgramState);

	eStatus = YModemInit(&sYModem,
						 1,
						 FLASH_UPDATE_BAUD_RATE,
						 (void *) &sFlashProgramState,
						 SharedFlashUpdateCallback);
	assert(ESTATUS_OK == eStatus);

	// Signal that we need to prepare the flash region
	sFlashProgramState.bFlashRegionPrep = true;

	// And we don't know what region
	sFlashProgramState.eProgramRegion = EFLASHREGION_END;

	while (1)
	{
		// If we need to prep the flash, do it
		if (sFlashProgramState.bFlashRegionPrep)
		{
			sFlashProgramState.bFlashRegionPrep = false;
			sFlashProgramState.eProgramRegion = SharedFlashPrepareRegion();
		}

		// If it's complete, time for analysis
		if (sFlashProgramState.bProgrammingComplete)
		{
			EVersionCode eVersionOp;
			SImageVersion sVersionStructOp;
			uint32_t u32VersionStructureOffset;

			sFlashProgramState.bProgrammingComplete = false;

			POST_SET(POSTCODE_FWUPD_PROGRAM_CHECK);

			// Now we check our new region for a new, good, BIOS image.
			eVersionOp = FlashRegionCheck(sFlashProgramState.eProgramRegion,
										  EIMGTYPE_OPERATIONAL,
										  &sVersionStructOp,
										  &u32VersionStructureOffset);

			if (EVERSION_OK == eVersionOp)
			{
				uint32_t u32ImageCRCCalculated = VERSION_CRC32_IMAGE_INITIAL;
				bool bResult;

				// Now check the CRC32

				// Before the version structure
				u32ImageCRCCalculated = FlashCRC32Region(sFlashProgramState.eProgramRegion,
														 u32ImageCRCCalculated,
														 0,
														 u32VersionStructureOffset);

				// After the version structure
				u32ImageCRCCalculated = FlashCRC32Region(sFlashProgramState.eProgramRegion,
														 u32ImageCRCCalculated,
														 u32VersionStructureOffset + sizeof(sVersionStructOp),
														 sVersionStructOp.u32ImageSize - sizeof(sVersionStructOp) - u32VersionStructureOffset);

				if (u32ImageCRCCalculated != sVersionStructOp.u32ImageCRC32)
				{
					printf("CRC Calc'd = 0x%.8x, CRC Expected = 0x%.8x\n", u32ImageCRCCalculated, sVersionStructOp.u32ImageCRC32);
					sFlashProgramState.bFlashRegionPrep = true;
				}
				else
				{
					printf("Successfully updated active BIOS image to %u.%u.%u\n",
						   sVersionStructOp.u32MajorMinorBuildNumber >> 24,
						   (sVersionStructOp.u32MajorMinorBuildNumber >> 16) & 0xff,
						   (uint16_t)sVersionStructOp.u32MajorMinorBuildNumber);

					printf("Erasing other bank for future staging\n");
					if (EFLASHREGION_OP_1 == sFlashProgramState.eProgramRegion)
					{
						sFlashProgramState.eProgramRegion = EFLASHREGION_OP_2;
					}
					else
					{
						sFlashProgramState.eProgramRegion = EFLASHREGION_OP_1;
					}

					POST_SET(POSTCODE_FWUPD_ERASE_STAGING);
					bResult = FlashRegionErase(sFlashProgramState.eProgramRegion);
					if (bResult)
					{
						printf("Erasure succeeded. Restarting with new BIOS\n");
						fflush(stdout);

						// Give some time for the FIFOs to clear
						SharedSleep(25000);

						// Reset the board
						*((volatile uint8_t *) RSC68KHW_DEVCOM_SELF_RESET) = 0xff;

						printf("If you see this, the reset hardware is broken.\n");
					}
					else
					{
						printf("Failed to erase staging region %u - 0x%.8x\n", sFlashProgramState.eProgramRegion, eStatus);
						sFlashProgramState.bFlashRegionPrep = true;
					}
				}
			}
			else
			{
				printf("Failed region check after programming - %s\n", VersionCodeGetText(eVersionOp));
				sFlashProgramState.bFlashRegionPrep = true;
			}
		}

		// YModem message pump
		(void) YModemPump(&sYModem);
	}
}


