#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "BIOS/OS.h"
#include "Hardware/RSC68k.h"
#include "Shared/Version.h"
#include "Shared/Shared.h"
#include "Shared/Stream.h"
#include "Shared/ptc.h"
#include "Shared/rtc.h"

// Main entry point for bpplication
void main(void)
{
	EStatus eStatus;
	uint32_t u32CPUSpeed;

	POST_SET(POSTCODE_APP_START_MAIN);

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

	printf("Application - Version %u.%u.%u - CPU running @ %uMhz\n",
		   g_sImageVersion.u32MajorMinorBuildNumber >> 24,
		   (g_sImageVersion.u32MajorMinorBuildNumber >> 16) & 0xff,
		   (uint16_t)g_sImageVersion.u32MajorMinorBuildNumber,
		   u32CPUSpeed / 1000000);

	while (1);
}

