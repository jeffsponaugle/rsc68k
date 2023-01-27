/*

Stream operator routines for Newlib

*/

#include <sys/stat.h>
#include <stdint.h>
#include <errno.h>
#include <assert.h>
#include "BIOS/OS.h"
#include "Shared/16550.h"
#include "Hardware/RSC68k.h"

// Set true if serial interrupts are in operation
static bool sg_bSerialInterrupts;

int fstat(int s32File,
		  struct stat *psStat)
{
	psStat->st_mode = S_IFCHR;
	return(0);
}

int lseek(int s32File,
		  int s32Offset,
		  int s32Whence)
{
	return(0);
}

int close(int s32File)
{
	return(-1);
}

int write(int s32File,
		  char *pu8Buffer,
		  int s32Count)
{
	int s32Written = 0;

	if (sg_bSerialInterrupts)
	{
		while (s32Count)
		{
			EStatus eStatus;
			uint16_t u16SentDataCount = 0;

			eStatus = SerialTransmitData(0,
										pu8Buffer,
										(uint16_t) s32Count,
										&u16SentDataCount);

			if (ESTATUS_OK == eStatus)
			{
				pu8Buffer += u16SentDataCount;
				s32Count -= u16SentDataCount;
				s32Written += u16SentDataCount;
			}
			else
			{
				assert(0);
			}
		}
	}
	else
	{
		SerialSendPIO((S16550UART *) RSC68KHW_DEVCOM_UARTA,
					  (uint8_t *) pu8Buffer,
					  (uint32_t) s32Count);
		s32Written = s32Count;
	}

	return(s32Written);
}

int isatty(int s32File)
{
	return(1);
}

int read(int s32File,
		 char *pu8Buffer,
		 int s32Count)
{
	int s32Read = 0;

	if (sg_bSerialInterrupts)
	{
		while (s32Count)
		{
			EStatus eStatus;
			uint16_t u16ReceivedDataCount = 0;

			eStatus = SerialReceiveData(0,
										pu8Buffer,
										(uint16_t) s32Count,
										&u16ReceivedDataCount);
			
			if (ESTATUS_OK == eStatus)
			{
				pu8Buffer += u16ReceivedDataCount;
				s32Count -= u16ReceivedDataCount;
				s32Read += u16ReceivedDataCount;
			}
			else
			{
				assert(0);
			}
		}
	}
	else
	{
		while (s32Count)
		{
			*pu8Buffer = SerialReceiveWaitPIO((S16550UART*) RSC68KHW_DEVCOM_UARTA);

			++pu8Buffer;
			++s32Read;
			s32Count--;
		}
	}

	return(s32Read);
}

// The following are required for assert() to work properly
int getpid(void)
{
	return(1);
}

int kill(int pid,
		 int sig)
{
	errno = EINVAL;
	return(-1);
}

EStatus StreamSetConsoleSerialInterruptMode(bool bInterruptMode)
{
	EStatus eStatus = ESTATUS_OK;

	if (bInterruptMode)
	{
		// Interrupt mode for the UARTs
		eStatus = SerialInterruptInit();
		ERR_GOTO();

		// We're interrupt mode!
		sg_bSerialInterrupts = bInterruptMode;
	}
	else
	{
		eStatus = SerialFlush(0);
		ERR_GOTO();

		// Run programmed I/O
		eStatus = SerialInterruptShutdown();
		ERR_GOTO();

		// No longer interrupt mode
		sg_bSerialInterrupts = bInterruptMode;
	}

errorExit:
	return(eStatus);
}

