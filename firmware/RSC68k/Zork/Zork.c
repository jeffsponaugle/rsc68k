#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "Shared/Shared.h"
#include "Shared/Interrupt.h"
#include "Hardware/RSC68k.h"
#include "Shared/rtc.h"
#include "Shared/ptc.h"
#include "Shared/Stream.h"
#include "Shared/sbrk.h"
#include "Zork/Zork.h"
#include "Shared/FaultHandler.h"
#include "Shared/LineInput.h"
#include "Shared/16550.h"

static bool sg_bFileOpened = false;
static uint32_t sg_u32Position = 0;

int Zorkfseek(FILE *psFile, long offset, int whence)
{
	assert(SEEK_SET == whence);

	if (offset >= g_u32dtextcSize)
	{
		printf("offset=%d, position=%u\n", offset, sg_u32Position);
		return(EOF);
	}
	else
	{
		sg_u32Position = offset;
		return(sg_u32Position);
	}
}

long Zorkftell(FILE *psFile)
{
	if (sg_bFileOpened)
	{
		return(sg_u32Position);
	}
	else
	{
		return(EOF);
	}
}

int Zorkfread(void *pvBuffer, size_t size, size_t nmemb, FILE *psFile)
{
	int s32Read;

	s32Read = size * nmemb;
	if ((sg_u32Position + s32Read) > g_u32dtextcSize)
	{
		s32Read = 0;
	}
	else
	{
		memcpy((void *) pvBuffer, (void *) &g_u8dtextc[sg_u32Position], s32Read);
		sg_u32Position += s32Read;
	}

	return(s32Read);
}

int Zorkgetc(FILE *psFile)
{
	if (sg_u32Position >= g_u32dtextcSize)
	{
		return(EOF);
	}   			
	else
	{
		return(g_u8dtextc[sg_u32Position++]);
	}
}

FILE *Zorkfopen(const char *path, const char *pefilemode)
{
	if (false == sg_bFileOpened)
	{
		sg_u32Position = 0;
		sg_bFileOpened = true;
		return((FILE *) &sg_u32Position);
	}
	else
	{
		return(NULL);
	}
}

int Zorkfclose(FILE *psFile)
{
	if (sg_bFileOpened)
	{
		sg_bFileOpened = false;
		return(0);
	}
	else
	{
		// No idea what this file is
		return(-1);
	}
}

// Which UART is the console
#define	MONITOR_CONSOLE_UART	0

// Wait for a character to be typed from the console
static EStatus MonitorInputGet(char *peInputChar)
{
	EStatus eStatus = ESTATUS_OK;

	for (;;)
	{
		uint16_t u16DataReceived;

		eStatus = SerialReceiveData(MONITOR_CONSOLE_UART,
									(uint8_t *) peInputChar,
									sizeof(*peInputChar),
									&u16DataReceived);
		if ((ESTATUS_OK == eStatus) && (1 == u16DataReceived))
		{
			break;
		}
	}

errorExit:
	return(eStatus);
}

// Output a character string to the console
static void MonitorOutput(char *peText,
						  uint16_t u16Length)
{
	(void) write(0,
				 peText,
				 (int) u16Length);
}


// Blocks and doesn't return until we have a full line input
char *Zorkfgets(char *str, int size, FILE *psFile)
{
	SLineInput sLineInput;
	EStatus eStatus;
	char *peLineText = NULL;
	uint16_t u16LineLength = 0;

	// Init the line input
	ZERO_STRUCT(sLineInput);

	sLineInput.Output = MonitorOutput;
	sLineInput.InputGet = MonitorInputGet;
	sLineInput.u16InputMax = size;
	sLineInput.u8HistoryMaxDepth = 0;

	eStatus = LineInputInit(&sLineInput);
	assert(ESTATUS_OK == eStatus);

	u16LineLength = size;
	eStatus = LineInputGet(&sLineInput,
						   &peLineText,
						   &u16LineLength);

	*str = '\0';
	strcpy(str, peLineText);
	free(peLineText);

	return(str);
}

int system(const char *peThing)
{
	return(0);
}

int main(int argc, char **argv)
{
	EStatus eStatus;
	uint32_t u32CPUSpeed;

	POST_SET(POSTCODE_FWUPD_START_MAIN);

	// Initialize the interrupt subsystem
	InterruptInit();

	// Reset the heap pointer
	SBRKHeapReset();

	// Install fault handlers
	FaultHandlerInstall();

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

	// Automatic CR/LF when LF provided (the UNIX way)
	StreamSetAutoCR(true);

	Zorkmain(argc, argv);

	return(0);
}

