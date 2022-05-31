/*

Stream operator routines for the BootLoader in support of Newlib

*/

#include <sys/stat.h>
#include <stdint.h>
#include <errno.h>
#include "Shared/16550.h"
#include "Hardware/RSC68k.h"

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
	SerialSendPIO((S16550UART *) RSC68KHW_DEVCOM_UARTA,
				  (uint8_t *) pu8Buffer,
				  (uint32_t) s32Count);
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

	while (s32Count)
	{
		*pu8Buffer = SerialReceiveWaitPIO((S16550UART*) RSC68KHW_DEVCOM_UARTA);

		++pu8Buffer;
		++s32Read;
		s32Count--;
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

