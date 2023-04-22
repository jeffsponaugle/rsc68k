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
#include "Shared/Stream.h"
#include "Shared/Shared.h"
#include "Shared/rtc.h"
#include "Shared/IDE.h"
#include "Shared/Interrupt.h"
#include "Shared/ptc.h"

// Our slot ID (uninitialized by default - filled in by boot process)
uint8_t g_u8SlotID = SLOT_ID_UNINITIALIZED;

// Making a copy of our boot loader version info (if applicable)
static SImageVersion sg_sBootLoaderVersionInfo;

// Main entry point for BIOS
void main(void)
{
	EVersionCode eVersionCode;
	EStatus eStatus;
	bool bResult;
	uint32_t u32CPUSpeed = 0;

	// Indicate that we're at main() on the POST code LEDs
	POST_SET(POSTCODE_BIOS_MAIN);

	// Initialize the interrupt subsystem
	InterruptInit();

	// Add a carriage return for every linefeed
	StreamSetAutoCR(true);

	// See if the version code is OK for the BIOS
	eVersionCode = VersionValidateStructure((SImageVersion *) &g_sImageVersion);

	// If it's not OK, then emit an error to the console and halt
	if (eVersionCode != EVERSION_OK)
	{
		printf("\nBIOS version structure fault - %s\n", VersionCodeGetText(eVersionCode));

		// Bail out
		exit(0);
	}


	// If we're the supervisor CPU, then find and make a copy of the flash table
	if (SLOT_ID_SUPERVISOR == g_u8SlotID)
	{
		// Copy the flash table out of the boot loader
		POST_SET(POSTCODE_BIOS_FLASH_COPY);
		FlashTableCopy(&sg_sBootLoaderVersionInfo,
					   false);
	}
	else
	{
		// Skip it if it's a worker CPU
	}

	// Init the UART - turn on FIFOs
	bResult = SerialInit((S16550UART *) RSC68KHW_DEVCOM_UARTA,
						 8,
						 1,
						 EUART_PARITY_NONE);
	assert(bResult);

	// Console initialize!
	POST_SET(POSTCODE_BIOS_CONSOLE_INIT);
	printf("%c[2J%c[HRSC68K BIOS Version %u.%u.%u\n",
		   0x1b, 0x1b,
		   g_sImageVersion.u32MajorMinorBuildNumber >> 24,
		   (g_sImageVersion.u32MajorMinorBuildNumber >> 16) & 0xff,
		   (uint16_t)g_sImageVersion.u32MajorMinorBuildNumber);

	// If we're the supervisor CPU, then display the boot loader info
	if (SLOT_ID_SUPERVISOR == g_u8SlotID)
	{
		printf("Boot loader Version %u.%u.%u\n",
			   sg_sBootLoaderVersionInfo.u32MajorMinorBuildNumber >> 24,
			   (sg_sBootLoaderVersionInfo.u32MajorMinorBuildNumber >> 16) & 0xff,
			   (uint16_t)sg_sBootLoaderVersionInfo.u32MajorMinorBuildNumber);
	}

	// Display CPU ID information
	if (SLOT_ID_SUPERVISOR == g_u8SlotID)
	{
		printf("\nCPU          : Supervisor\n");
	}
	else
	if ((g_u8SlotID >= SLOT_ID_BASE) && (g_u8SlotID < SLOT_ID_MAX))
	{
		printf("\nCPU          : Worker #%u\n", g_u8SlotID - SLOT_ID_BASE);
	}
	else
	if (SLOT_ID_UNINITIALIZED == g_u8SlotID)
	{
		printf("CPU Slot ID not initialized - halting\n");
		exit(0);
	}
	else
	{
		printf("CPU Slot ID byte unknown - 0x%.2x\n - halting\n", g_u8SlotID);
		exit(0);
	}

	if (SLOT_ID_SUPERVISOR == g_u8SlotID)
	{
		struct tm *psTime;
		time_t eTime;

		// Init the RTC

		eStatus = RTCInit();
		if (ESTATUS_OK == eStatus)
		{
			eStatus = RTCGetCPUSpeed(&u32CPUSpeed);
			if (ESTATUS_OK == eStatus)
			{
				printf("CPU Speed    : %uMhz\n", u32CPUSpeed / 1000000);
			}
			else
			{
				printf("CPU Speed    : Failed - %s\n", GetErrorText(eStatus));
			}

			printf("RTC          : OK\n");
		}
		else
		{
			printf("CPU Speed        : (unknown - failed RTC)\n");
			printf("RTC              : Failed - %s\n", GetErrorText(eStatus));
		}
	}

	// Init the PTC
	printf("PTC          : ");

	// Master clock is CPU speed * 2
	eStatus = PTCInit(u32CPUSpeed<<1);
	if (ESTATUS_OK == eStatus)
	{
		printf("Ok\n");
	}
	else
	{
		// PTC Will emit a detailed error so no need to display eStatus
	}

	// Serial port interrupt tests
	printf("UART A/B IRQ : ");

	eStatus = SerialInterruptTest();
	if (ESTATUS_OK == eStatus)
	{
		printf("Ok\n");
	}
	else
	{
		// Serial port code will emit a detailed error
	}

	// Status doesn't really matter
	(void) IDEProbe();

	// Start the in-flash monitor
	eStatus = MonitorStart();

	// End of everything
	exit(0);
}

