/*
    vim:ts=4:noexpandtab
*/
#include <math.h>
#include "Startup/app.h"
#include "Application/RSC68k.h"
#include "Libs/Gfx/GraphicsLib.h"
#include "Libs/window/window.h"
#include "Libs/FontMgr/FontMgr.h"
#include "Libs/widget/widget.h"
#include "Libs/widget/console/console.h"
#include "Libs/Sound/WaveMgr.h"
#include "Libs/Sound/SoundStream.h"
#include "Libs/widget/elements/elements.h"

#ifdef _WIN32

// return # of bytes of memory in h/w.  this should be total
// bytes of memory.  Win32 will subtract "BIOS" memory.
UINT32 AppGetSystemMemory(void)
{
	return(128*1024*1024);
}

char *AppGetFilesDir(void)
{
	return "files";
}

char *AppGetArchiveFilesDir(void)
{
	return "files-archive";
}

BOOL AppHasVertMonitor(void)
{
	return(FALSE);
}

#endif /* _WIN32 */

// Define the resolution - depending upon the device

#define	TEST_APP_STACK_SIZE		32768

static UINT8 sg_u8TestAppStack[TEST_APP_STACK_SIZE];


void *RuntimeAllocateMemoryInternal(UINT64 u64Size,
									UINT8 *pu8ModuleName,
									UINT32 u32LineNumber,
									BOOL bClearBlock)
{
	void *pvMemoryBlock;

#ifndef _MYHEAP
	if (bClearBlock)
	{
		return(calloc(1, u64Size));
	}
	else
	{
		return(malloc(u64Size));
	}
#else
	pvMemoryBlock = GCAllocateMemoryInternal((UINT32)u64Size,
											 pu8ModuleName,
											 u32LineNumber,
											 bClearBlock);
#endif
	return(pvMemoryBlock);
}

void *RuntimeReallocMemoryInternal(void *pvOldBlock,
								   UINT64 u64Size,
								   UINT8 *pu8ModuleName,
								   UINT32 u32LineNumber)
{
	void *pvNewBlock = NULL;

#ifndef _MYHEAP
	pvNewBlock = realloc(pvOldBlock, u64Size);
#else
	UINT32 u32BlockSize;
	EGCResultCode eResult;

	// Get the original block size
	eResult = GCGetBlockSize(pvOldBlock,
							 &u32BlockSize);
	GCASSERT(GC_OK == eResult);

	pvNewBlock = RuntimeAllocateMemoryInternal(u64Size,
											   pu8ModuleName,
											   u32LineNumber,
											   TRUE);

	if (NULL == pvNewBlock)
	{
		return(pvNewBlock);
	}

	// No need to copy the "extra" data from the original block
	if (u32BlockSize < u64Size)
	{
		u64Size = u32BlockSize;
	}

	memcpy((void *) pvNewBlock, pvOldBlock, (size_t)u64Size);

	GCFreeMemory(pvOldBlock);
#endif

	return(pvNewBlock);
}

#ifndef _RELEASE
static const DWORD MS_VC_EXCEPTION=0x406d1388;
#endif

#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO
{
	DWORD dwType; // Must be 0x1000.
	LPCSTR szName; // Pointer to name (in user addr space).
	DWORD dwThreadID; // Thread ID (-1=caller thread).
	DWORD dwFlags; // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)

// 8K ought to be enough
#define	PRINT_BUFFER_SIZE	8192

void ThreadSetName(char *pu8ThreadName)
{
#ifndef _RELEASE
	THREADNAME_INFO sInfo;

	memset((void *) &sInfo, 0, sizeof(sInfo));

	sInfo.dwType = 0x1000;
	sInfo.szName = pu8ThreadName;
	sInfo.dwThreadID = GetCurrentThreadId();
	sInfo.dwFlags = 0;

	RaiseException(MS_VC_EXCEPTION, 0, sizeof(sInfo) / sizeof(ULONG_PTR), (ULONG_PTR *) &sInfo);
#endif	// #ifndef _RELEASE
}

// Printf-style thread name settings
void ThreadSetNameprintf(char *pu8Format, ...)
{
#ifndef _RELEASE
	va_list ap;
	UINT8 u8MsgBuf[PRINT_BUFFER_SIZE];

	va_start(ap, pu8Format);
	vsprintf((char *) u8MsgBuf, (const char *) pu8Format, ap);
	va_end(ap);

	ThreadSetName(u8MsgBuf);
#endif
}

static BOOL IsSpace(LEX_CHAR eChar)
{
	if (((LEX_CHAR) ' ' == eChar) ||
		((LEX_CHAR) '\t' == eChar))
	{
		return(TRUE);
	}
	else
	{
		return(FALSE);
	}
}

void LexStripWhitespace(LEX_CHAR *peStringStart)
{
	LEX_CHAR *peString = peStringStart;
	UINT32 u32Len;

	if (NULL == peString)
	{
		return;
	}

	while ((*peString) && 
		   (IsSpace(*peString)))
	{
		peString++;
	}

	if (peString != peStringStart)
	{
		strcpy((char *) peStringStart, (char *) peString);
	}

	u32Len = Lexstrlen(peStringStart);
	if (u32Len)
	{
		peString = peStringStart + (u32Len - 1);

		while (u32Len)
		{
			if (IsSpace(*peString))
			{
				u32Len--;
				--peString;
			}
			else
			{
				++peString;
				break;
			}
		}

		*peString = '\0';
	}
}

int Lexstrcasecmp(const LEX_CHAR *s1,
				  const LEX_CHAR *s2)
{
	LEX_CHAR eChar1;
	LEX_CHAR eChar2;
	INT16 s16Result;

	while (*s1 != '\0' && *s2 != '\0')
	{
		eChar1 = *s1;
		eChar2 = *s2;

		if (eChar1 >= 'A' && eChar1 <= 'Z')
		{
			eChar1 += 0x20;
		}

		if (eChar2 >= 'A' && eChar2 <= 'Z')
		{
			eChar2 += 0x20;
		}

		if (eChar1 - eChar2)
		{
			if ((eChar1 - eChar2) < 0)
			{
				return(1);
			}
			else
			if ((eChar1 - eChar2) > 0)
			{
				return(-1);
			}
		}

		s1++;
		s2++;
	}

	s16Result = tolower(*(unsigned char *) s1) - tolower(*(unsigned char *) s2);
	if (s16Result < 0)
	{
		return(1);
	}
	else
	if (s16Result > 0)
	{
		return(-1);
	}
	else
	{
		return(0);
	}
}

void Lexstrncpy(LEX_CHAR *peDest,
				LEX_CHAR *peSrc,
				UINT32 u32Length)
{
	while (*peSrc && u32Length)
	{
		*peDest = *peSrc;
		++peDest;
		++peSrc;
		u32Length--;
	}

	*peDest = (LEX_CHAR) '\0';
}

void Lexstrcpy(LEX_CHAR *peDest,
			   LEX_CHAR *peSrc)
{
	while (*peSrc)
	{
		*peDest = *peSrc;
		++peDest;
		++peSrc;
	}

	*peDest = (LEX_CHAR) '\0';
}

void Lexstrcat(LEX_CHAR *peDest,
			   LEX_CHAR *peSrc)
{
	while (*peDest) 
	{
		peDest++;
	}

	while (*peSrc)
	{
		*peDest = *peSrc;
		++peDest;
		++peSrc;
	}

	*peDest = (LEX_CHAR) '\0';
}

void Lexstrncat(LEX_CHAR *peDest,
			    LEX_CHAR *peSrc,
				UINT32 u32Length)
{
	while (*peDest && u32Length)
	{
		peDest++;
		u32Length--;
	}

	while (*peSrc && u32Length)
	{
		*peDest = *peSrc;
		++peDest;
		++peSrc;
		u32Length--;
	}

	*peDest = (LEX_CHAR) '\0';
}

INT32 Lexstrcmp(LEX_CHAR *peString1,
			    LEX_CHAR *peString2)
{
	while ((*peString1 == *peString2) &&
		   (*peString1))
	{
		peString1++;
		peString2++;
	}

	if ((*peString1 - *peString2) < 0)
	{
		return(1);
	}
	else
	if ((*peString1 - *peString2) > 0)
	{
		return(-1);
	}
	else
	{
		return(0);
	}
}

INT32 Lexstrncmp(LEX_CHAR *peString1,
			     LEX_CHAR *peString2,
				 UINT32 u32Loop)
{
	while ((*peString1 == *peString2) &&
		   (*peString1) && (u32Loop))
	{
		peString1++;
		peString2++;
		--u32Loop;
	}

	if (0 == u32Loop)
	{
		return(0);
	}

	if ((*peString1 - *peString2) < 0)
	{
		return(1);
	}
	else
	if ((*peString1 - *peString2) > 0)
	{
		return(-1);
	}
	else
	{
		return(0);
	}
}

LEX_CHAR Lextoupper(LEX_CHAR eChar)
{
	if ((eChar >= (LEX_CHAR) 'a') && (eChar <= (LEX_CHAR) 'z'))
	{
		return((LEX_CHAR) (eChar - 0x20));
	}
	else
	{
		return(eChar);
	}
}

LEX_CHAR Lextolower(LEX_CHAR eChar)
{
	if ((eChar >= (LEX_CHAR) 'A') && (eChar <= (LEX_CHAR) 'Z'))
	{
		return((LEX_CHAR) (eChar + 0x20));
	}
	else
	{
		return(eChar);
	}
}

INT32 Lexstrnicmp(LEX_CHAR *peString1,
			      LEX_CHAR *peString2,
				  UINT32 u32Loop)
{
	while ((Lextoupper(*peString1) == Lextoupper(*peString2)) &&
		   (*peString1) && (u32Loop))
	{
		peString1++;
		peString2++;
		--u32Loop;
	}

	if (0 == u32Loop)
	{
		return(0);
	}

	if ((Lextoupper(*peString1) - Lextoupper(*peString2)) < 0)
	{
		return(1);
	}
	else
	if ((Lextoupper(*peString1) - Lextoupper(*peString2)) > 0)
	{
		return(-1);
	}
	else
	{
		return(0);
	}
}

void LexASCIIToUnicode(char *pu8SourceString,
					   LEX_CHAR *peTargetString)
{
	while (*pu8SourceString)
	{
		*peTargetString = *pu8SourceString;
		++peTargetString;
		++pu8SourceString;
	}

	// NULL Terminate it (works in any language)
	*peTargetString = 0;
}

char *ASCIIstrdup(char *pu8String)
{
	char *pu8Data;

	pu8Data = MemAlloc(strlen(pu8String) + 1);
	if (pu8Data)
	{
		strcpy(pu8Data, pu8String);
	}

	return(pu8Data);
}

void LexUnicodeToASCII(char *pu8ASCIIFilename,
					   LEX_CHAR *peFilename)
{
	while (*peFilename)
	{
		// Make sure it's actually ASCII
		GCASSERT(*peFilename < 0x100);

		*pu8ASCIIFilename = (UINT8) *peFilename;
		++pu8ASCIIFilename;
		++peFilename;
	}

	*pu8ASCIIFilename = '\0';
}

LEX_CHAR *LexASCIIToUnicodeAlloc(char *pu8SourceString)
{
	LEX_CHAR *peStringHead;
	LEX_CHAR *peString;

	peString = MemAlloc((strlen(pu8SourceString) + 1) * sizeof(*peString));
	peStringHead = peString;
	if (NULL == peString)
	{
		// Out of memory
		return(NULL);
	}

	// Copy the string to its unicode equivalent
	while (*pu8SourceString)
	{
		*peString = *pu8SourceString;
		++peString;
		++pu8SourceString;
	}

	return(peStringHead);
}

UINT32 Lexstrlen(const LEX_CHAR *peString)
{
	UINT32 u32Length = 0;

	if (NULL == peString)
	{
		return(0);
	}

	while (*peString)
	{
		++peString;
		u32Length++;
	}

	return(u32Length);
}

char *LexUnicodeToASCIIAlloc(LEX_CHAR *peFilename)
{
	char *pu8StringHead;
	char *pu8String;

	pu8String = MemAlloc((Lexstrlen(peFilename) + 1) * sizeof(*pu8String));
	pu8StringHead = pu8String;

	// Copy the unicode filename to ASCII
	while (*peFilename)
	{
		// Make sure it really is ASCII in UTF-8 format
		GCASSERT(*peFilename <= 0x100);
		*pu8String = (UINT8) *peFilename;
		++peFilename;
		++pu8String;
	}

	return(pu8StringHead);
}

LEX_CHAR *Lexstrdup(const LEX_CHAR *peString)
{
	LEX_CHAR *peStringNew = NULL;
	UINT32 u32Length = 0;

	u32Length = (Lexstrlen(peString) + 1) * sizeof(*peString);
	peStringNew = MemAlloc(u32Length);

	if (peStringNew)
	{
		memcpy(peStringNew,
			   peString,
			   u32Length);
	}

	return(peStringNew);
}

BOOL Lexstrdupsafe(LEX_CHAR **ppeTargetString,
				   LEX_CHAR *peSourceString)
{
	// Both strings are NULL. Ditch 'em.
	if (NULL == peSourceString)
	{
		*ppeTargetString = NULL;
		return(TRUE);
	}

	// Now copy
	*ppeTargetString = Lexstrdup(peSourceString);
	if (NULL == *ppeTargetString)
	{
		return(FALSE);
	}
	else
	{
		return(TRUE);
	}
}

LEX_CHAR *Lexstrndup(const LEX_CHAR *peString,
					 UINT32 u32MaxLength)
{
	LEX_CHAR *peStringNew = NULL;
	UINT32 u32Length;

	if (NULL == peString)
	{
		return(NULL);
	}

	u32Length = Lexstrlen(peString);
	if (u32Length > u32MaxLength)
	{
		u32Length = u32MaxLength;
	}

	u32Length++;
	peStringNew = MemAlloc(u32Length * sizeof(*peString));

	memcpy(peStringNew, peString, (u32Length - 1) * sizeof(*peString));
	peStringNew[u32Length - 1] = '\0';
	return(peStringNew);
}

LEX_CHAR *Lexstrcasestr(LEX_CHAR *peStringToSearch,
						LEX_CHAR *peSubString)
{
	LEX_CHAR *h;
	LEX_CHAR *n;

	h = peStringToSearch;
	n = peSubString;

	while (*peStringToSearch) 
	{
		if (tolower ((unsigned char)*h) == tolower ((unsigned char)*n)) 
		{
			h++;
			n++;
			if (!*n)
			{
				return peStringToSearch;
			}
		} 
		else 
		{
			h = ++peStringToSearch;
			n = peSubString;
		}
	}

	return NULL;
}

LEX_CHAR *Lexstrcasestrrev(LEX_CHAR *peStringToSearch,
						   LEX_CHAR *peSubString,
						   UINT32 u32StartPosition)
{
	LEX_CHAR *h;
	LEX_CHAR *n;
	UINT32 u32StringToSearchLen = Lexstrlen(peStringToSearch);
	UINT32 u32SubStringLen = Lexstrlen(peSubString);

	// Substring is bigger than the to-be-searched string. Can't possibly work.
	if (u32SubStringLen > u32StringToSearchLen)
	{
		return(NULL);
	}

	if (u32StartPosition > (u32StringToSearchLen - u32SubStringLen))
	{
		u32StartPosition = (u32StringToSearchLen - u32SubStringLen);
	}

	peStringToSearch += u32StartPosition;
	h = peStringToSearch;
	n = peSubString;

	while (*peStringToSearch) 
	{
		if (tolower ((unsigned char)*h) == tolower ((unsigned char)*n)) 
		{
			h++;
			n++;
			if (!*n)
			{
				return peStringToSearch;
			}
		} 
		else 
		{
			--peStringToSearch;
			h = peStringToSearch;
			n = peSubString;
		}
	}

	return NULL;
}

LEX_CHAR *Lexstrstr(LEX_CHAR *peStringToSearch,
					LEX_CHAR *peSubString)
{
	UINT32 u32StringToSearchLen = Lexstrlen(peStringToSearch);
	UINT32 u32SubStringLen = Lexstrlen(peSubString);
	UINT32 u32Count = 0;

	if (u32SubStringLen > u32StringToSearchLen)
	{
		return(NULL);
	}

	u32Count = u32StringToSearchLen - u32SubStringLen;

	do
	{
		if (memcmp(peStringToSearch, peSubString, u32SubStringLen) == 0)
		{
			return(peStringToSearch);
		}

		++peStringToSearch;
	}
	while (u32Count--);

	return(NULL);
}

void DebugOut(const char *pu8Format, ...)
{
	va_list ap;
	UINT8 msgbuf[PRINT_BUFFER_SIZE];
	UINT8* pu8Message = msgbuf;
	HANDLE eConsoleHandle;

	if( 1 )
	{
		SYSTEMTIME sSystemTime;
		GetSystemTime(&sSystemTime);
		pu8Message += _snprintf(pu8Message, (sizeof(msgbuf) - (pu8Message - msgbuf)), "%.2u:%.2u:%.2u.%.3u: ", sSystemTime.wHour, sSystemTime.wMinute, sSystemTime.wSecond, sSystemTime.wMilliseconds);
	}

	va_start(ap, pu8Format);
	vsnprintf((char *) pu8Message, (sizeof(msgbuf) - (pu8Message - msgbuf)), (const char *) pu8Format, ap);
	va_end(ap);

	OutputDebugString(pu8Message);
	eConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	if (eConsoleHandle != INVALID_HANDLE_VALUE)
	{
		DWORD u32Foo;	// Throwaway value we don't care about

		(void) WriteFile(eConsoleHandle,
						 pu8Message,
						 (DWORD) strlen(pu8Message),
						 &u32Foo,
						 NULL);
	}
}

void DebugOutFuncInternal(const char *pu8Function,
						  const char *pu8Format, ...)
{
	va_list ap;
	UINT8 msgbuf[PRINT_BUFFER_SIZE];
	UINT8* pu8Message = msgbuf;
	HANDLE eConsoleHandle;

	if( 1 )
	{
		SYSTEMTIME sSystemTime;
		GetSystemTime(&sSystemTime);
		pu8Message += _snprintf(pu8Message, (sizeof(msgbuf) - (pu8Message - msgbuf)), "%.2u:%.2u:%.2u.%.3u: ", sSystemTime.wHour, sSystemTime.wMinute, sSystemTime.wSecond, sSystemTime.wMilliseconds);
	}

	pu8Message += _snprintf(pu8Message, (sizeof(msgbuf) - (pu8Message - msgbuf)), "%s: ", pu8Function);

	va_start(ap, pu8Format);
	vsnprintf((char *) pu8Message, (sizeof(msgbuf) - (pu8Message - msgbuf)), (const char *) pu8Format, ap);
	va_end(ap);

	OutputDebugString(pu8Message);
	eConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	if (eConsoleHandle != INVALID_HANDLE_VALUE)
	{
		DWORD u32Foo;	// Throwaway value we don't care about

		(void) WriteFile(eConsoleHandle,
						 pu8Message,
						 (DWORD) strlen(pu8Message),
						 &u32Foo,
						 NULL);
	}
}

static const UINT32 sg_u32ElementSize[] =
{
	sizeof(INT8),					// EVAR_SIGNED_CHAR
	sizeof(UINT8),					// EVAR_UNSIGNED_CHAR
	sizeof(INT16),					// EVAR_SIGNED_SHORT
	sizeof(UINT16),					// EVAR_UNSIGNED_SHORT
	sizeof(INT32),					// EVAR_SIGNED_INT
	sizeof(UINT32),					// EVAR_UNSIGNED_INT
	sizeof(INT64),					// EVAR_SIGNED_LONG_INT
	sizeof(UINT64),					// EVAR_UNSIGNED_LONG_INT
	sizeof(float),					// EVAR_FLOAT
	sizeof(double),					// EVAR_DOUBLE
	sizeof(BOOL),					// EVAR_BOOLEAN
	sizeof(LEX_CHAR *),				// EVAR_STRING
	sizeof(BUTTONHANDLE),			// EVAR_BUTTON,
	sizeof(WINDOWHANDLE),			// EVAR_WINDOW
	sizeof(FILEHANDLE),				// EVAR_FILEHANDLE
	sizeof(SOUNDHANDLE),			// EVAR_SOUNDHANDLE
	sizeof(TEXTHANDLE),				// EVAR_TEXTHANDLE
	sizeof(IMAGEHANDLE),			// EVAR_IMAGEHANDLE
	sizeof(GRAPHHANDLE),			// EVAR_GRAPHHANDLE
	sizeof(GRAPHSERIESHANDLE),		// EVAR_GRAPHSERIESHANDLE
	sizeof(VIDEOHANDLE),			// EVAR_VIDEOHANDLE
	sizeof(SLIDERHANDLE),			// EVAR_SLIDERHANDLE
	sizeof(RADIOGROUPHANDLE),		// EVAR_RADIOGROUPHANDLE
	sizeof(CHECKBOXGROUPHANDLE),	// EVAR_CHECKBOXGROUPHANDLE
	sizeof(TOUCHREGIONHANDLE),		// EVAR_TOUCHREGIONHANDLE
	sizeof(CONSOLEHANDLE),			// EVAR_CONSOLEHANDLE
};

UINT32 VarGetTypeSize(EVarType eVarType)
{
	GCASSERT(eVarType < (sizeof(sg_u32ElementSize) / sizeof(sg_u32ElementSize[0])));
	return(sg_u32ElementSize[eVarType]);
}

BOOL IsNumericType(EVarType eVarType)
{
	if ((eVarType != EVAR_SIGNED_CHAR) &&
		(eVarType != EVAR_UNSIGNED_CHAR) &&
		(eVarType != EVAR_SIGNED_SHORT) &&
		(eVarType != EVAR_UNSIGNED_SHORT) &&
		(eVarType != EVAR_SIGNED_INT) &&
		(eVarType != EVAR_UNSIGNED_INT) &&
		(eVarType != EVAR_SIGNED_LONG_INT) &&
		(eVarType != EVAR_UNSIGNED_LONG_INT) &&
		(eVarType != EVAR_FLOAT) &&
		(eVarType != EVAR_DOUBLE) &&
		(eVarType != EVAR_BOOLEAN))
	{
		return(FALSE);
	}
	else
	{
		return(TRUE);
	}
}

// ASSERT yourself
static CRITICAL_SECTION sg_eAssertCriticalSection;

static void AssertHandlerProc(UINT8 *pu8String, UINT8 *pu8Module, UINT32 u32Line)
{
	// Keep reentrancy from happening
	EnterCriticalSection(&sg_eAssertCriticalSection);

	// Scheduler is supposed to be locked at this point to prevent other
	// cascading threads from asserting

#ifdef _DEBUG
	if (_CrtDbgReport( _CRT_ASSERT, pu8Module, u32Line, "RSC68k.exe",
		"%s", pu8String) == 1)
	{
		DebugBreak();
	}
#else

	MessageBox(NULL,
			   pu8String,
			   "RSC68k",
			   MB_OK);
			  
	exit(1);

#endif
}

void AssertHandler(UINT8 *pu8Expression, UINT8 *pu8Module, UINT32 u32Line)
{
	char u8String[512];

	Sleep(500);

	DebugOut("Assert: %s\nModule: %s\nLine: %u\n", pu8Expression, pu8Module, u32Line);
	_snprintf(u8String, sizeof(u8String) - 1, "%s\nModule: %s\nLine: %u\n", pu8Expression, pu8Module, u32Line);
	AssertHandlerProc(u8String, pu8Module, u32Line);
}

void AssertWhyHandler(UINT8 *pu8Expression, char *pu8Why, UINT8 *pu8Module, UINT32 u32Line)
{
	char u8String[512];

	DebugOut("AssertWhy: %s\nAssert: %s\nModule: %s\nLine: %u\n", pu8Why, pu8Expression, pu8Module, u32Line);
	_snprintf(u8String, sizeof(u8String) - 1, "Why: %s\nAssert: %s\nModule: %s\nLine: %u\n", pu8Why, pu8Expression, pu8Module, u32Line);
	AssertHandlerProc(u8String, pu8Module, u32Line);
}

void AppMain(UINT8 *pu8CmdLine)
{
	EGCResultCode eResult;
	UINT32 u32DisplayXSize = 1920;
	UINT32 u32DisplayYSize = 1080;

	// Init the assert reentrancy critical section
	InitializeCriticalSection(&sg_eAssertCriticalSection);

	eResult = GCDisplaySetMode(u32DisplayXSize, u32DisplayYSize, 16, 0);
	GCASSERT(GC_OK == eResult);

	DebugOut("* Setting video mode to %dx%d\n", u32DisplayXSize, u32DisplayYSize);

	// Clear the screen
	eResult = GCDisplayClear(0);
	GCASSERT(GC_OK == eResult);

	// Initialize the sound subsystem
	WaveMgrInit();

	// Initialize the window subsystem to whatever the display resolution is
	WindowInit();

	// Now the font manager
	FontMgrInit();

	// Now the widget manager
	WidgetInit();

	// Now the sound stream
	SoundStreamInit();
	
	// Now create a window manager thread
	eResult = GCOSThreadCreate(AppEntry,
							   NULL,
							   &sg_u8TestAppStack[TEST_APP_STACK_SIZE - 4],
							   4);
	GCASSERT(GC_OK == eResult);

	// Start the OS!
	(void) GCOSStart();
}
