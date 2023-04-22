// vim:ts=4:noexpandtab

#define _USE_32BIT_TIME_T // Yes msft we are not using 64 bit ...

#include "Startup/app.h"
#include "Win32/sdl/sdl.h"
#include "Win32/host.h"
#include "Libs/zlib/zlib.h"
#include "Libs/zlib/unzip.h"
#include "Include/ram.h"
#include "Application/RSC68k.h"
#include "Application/Mersenne.h"
#include "Build/build.h"
#include "Application/CmdLine.h"

#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <mmsystem.h>
#include <time.h>
#include <crtdbg.h>
#include <setupapi.h>
#include <string.h>

// this must be defined by application. returns total memory size 
// of target board. 
// we subtract 1MB (currently) for BIOS.
#define BIOS_RESERVED_MEMORY (1024 * 1024)
static UINT32 sg_u32FPS = 0;
static UINT32 sg_u32HostFPS = 0;

extern UINT32 AppGetSystemMemory(void);

#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include <direct.h>

/* app must define this -- should return directory name where files
   are stored for this APP. */
extern const char *AppGetFilesDir(void);

extern void InputMapInit(char *fn);

static BOOL (*sg_pfKeyCallbackHandler)(GCKey, LEX_CHAR, BOOL) = NULL;
static void (*sg_pMousewheelCallback)(UINT32 u32Value);

struct SFileFind_t
{
	int DirPtr;
	struct _finddata32_t FindData;
	BOOL LastRet;
};


/* ************************************************************************* *\
** FUNCTION: GCRandomNumber
\* ************************************************************************* */
UINT32 GCRandomNumber(void)
{
	return(genrand_int32());
}


/* ************************************************************************* *\
** FUNCTION: GCGetPerformanceCounter
\* ************************************************************************* */
EGCResultCode GCGetPerformanceCounter(UINT32 u32Counter,
									  UINT64 *pu64Value)
{
	LARGE_INTEGER large;
	BOOL bResult;
    if (u32Counter != 0) return(GC_COUNTER_OUT_OF_RANGE);

	bResult = QueryPerformanceCounter(&large);
	GCASSERT(bResult == TRUE);
	GCASSERT(sizeof(*pu64Value) == sizeof(large.QuadPart));
	*pu64Value = (UINT64) large.QuadPart;
	return GC_OK;
}

/* ************************************************************************* *\
** FUNCTION: GCSetPerformanceCounter
\* ************************************************************************* */
EGCResultCode GCSetPerformanceCounter(UINT32 u32Counter,
									  UINT32 u32Value)
{
	return(GC_COUNTER_OUT_OF_RANGE);
}

EGCResultCode GCGetPerformanceCounterFrequency(UINT32 u32Counter,
											   UINT64 *pu64Frequency)
{
	LARGE_INTEGER large;
	BOOL bResult;
    if (u32Counter != 0) return(GC_COUNTER_OUT_OF_RANGE);

	bResult = QueryPerformanceFrequency(&large);
	GCASSERT(bResult == TRUE);
	GCASSERT(sizeof(*pu64Frequency) == sizeof(large.QuadPart));
	*pu64Frequency = (UINT64) large.QuadPart;
	return GC_OK;
}

/* ************************************************************************* *\
** FUNCTION: GCSetProcessorSpeed
\* ************************************************************************* */
EGCResultCode GCSetProcessorSpeed(UINT32 u32Speed)
{
	return(GC_OK);
}

/* ************************************************************************* *\
** ************************************************************************* **
**  NVRAM
** ************************************************************************* **
\* ************************************************************************* */
#define NVSTORE_SIZE ((192 * 1024) - sizeof(UINT32))
static UINT8 sg_u8NVRAM[NVSTORE_SIZE];
static BOOL sg_bNVRAMDirty = FALSE;
static BOOL sg_bNVRAMLoaded = FALSE;

/* ************************************************************************* *\
** FUNCTION: NVRAMLoad
\* ************************************************************************* */
static void NVRAMLoad(void)
{
	int rc;
	FILE *fp;

	if (sg_bNVRAMLoaded) return;

	fp = fopen("NVRAM.bin","rb");
	if (fp == NULL)		// no file?  make a new one.
	{
		GCNVStoreClear();
		return;
	}

	sg_bNVRAMDirty = FALSE;
	rc = fread(sg_u8NVRAM, 1, NVSTORE_SIZE,fp);
	fclose(fp);
	sg_bNVRAMLoaded = TRUE;
	GCASSERT(rc == NVSTORE_SIZE);
}

/* ************************************************************************* *\
** FUNCTION: GCSetRestartOverride
\* ************************************************************************* */

void GCSetRestartOverride(BOOL bFlag)
{
	return;
}

/* ************************************************************************* *\
** FUNCTION: GCNVStoreGetSize
\* ************************************************************************* */
UINT32 GCNVStoreGetSize(void)
{
	return NVSTORE_SIZE;
}


/* ************************************************************************* *\
** FUNCTION: GCNVStoreRead
\* ************************************************************************* */
EGCResultCode GCNVStoreRead(void *pvDataBuffer, 
    UINT32 u32NVOffset, 
    UINT32 u32ByteCount)
{
	NVRAMLoad();
	GCASSERT(u32NVOffset + u32ByteCount <= NVSTORE_SIZE);
	memcpy(pvDataBuffer, sg_u8NVRAM + u32NVOffset, u32ByteCount);

	return GC_OK;
}

/* ************************************************************************* *\
** FUNCTION: GCNVStoreWrite
\* ************************************************************************* */
EGCResultCode GCNVStoreWrite(void *pvDataBuffer, 
     UINT32 u32NVOffset, 
     UINT32 u32ByteCount)
{
	NVRAMLoad();

	GCASSERT(u32NVOffset + u32ByteCount <= NVSTORE_SIZE);
	memcpy(sg_u8NVRAM + u32NVOffset, pvDataBuffer, u32ByteCount);

	sg_bNVRAMDirty = TRUE;
	return GC_OK;
}


/* ************************************************************************* *\
** FUNCTION: GCNVStoreCommit
\* ************************************************************************* */
EGCResultCode GCNVStoreCommit(void)
{
	FILE *fp;
	int rc;

	if (!sg_bNVRAMDirty) return GC_OK;
	fp = fopen("NVRAM.bin","wb");
	GCASSERT(fp != NULL);

	rc = fwrite(sg_u8NVRAM,NVSTORE_SIZE,1,fp);
	GCASSERT(rc > 0);

	fclose(fp);

	sg_bNVRAMDirty = FALSE;
	return GC_OK;
}


/* ************************************************************************* *\
** FUNCTION: GCNVStoreClear
\* ************************************************************************* */
void GCNVStoreClear(void)
{
	memset(sg_u8NVRAM,0xff,NVSTORE_SIZE);
	sg_bNVRAMDirty = TRUE;
	GCNVStoreCommit();
}



/* ************************************************************************* *\
** ************************************************************************* **
**  File IO Stuff
** ************************************************************************* **
\* ************************************************************************* */

/* ************************************************************************* *\
** MakeAppPath
\* ************************************************************************* */
static MakeAppPath(char* out_pPath, const char* in_pBase)
{
	strcpy(out_pPath, in_pBase);
}

/* ************************************************************************* *\
** FUNCTION: GCFileGetInfo
\* ************************************************************************* */

#define		CRC_BUFFER_SIZE		8192

EGCResultCode GCFileGetInfo(UINT8 *pu8Filename,
							SFileInfo *psFileInfo)
{
	char string[500];
	char *pu8Separator;
	FILE *fp;

	GCASSERT(psFileInfo);
	GCASSERT(pu8Filename);

	memset(psFileInfo, 0, sizeof(*psFileInfo));
	memset((void *) string, 0, sizeof(string));
	pu8Separator = strstr(pu8Filename, "archive:");

	if (pu8Separator)
	{
		strncpy(string, (pu8Separator + 8), sizeof(string));
	}
	else
	{
			strncpy(string, pu8Filename, sizeof(string));
	}

	fp = fopen(string, "rb");

	if (fp)
	{
		UINT8 *pu8DataBuffer;
		INT32 s32Result;
		struct _stati64 sAttributes;

		pu8DataBuffer = GCAllocateMemory(CRC_BUFFER_SIZE);
		GCASSERT(pu8DataBuffer);

		do
		{
			s32Result = fread(pu8DataBuffer, 1, CRC_BUFFER_SIZE, fp);
			psFileInfo->u32FileSize += s32Result;
			psFileInfo->u32CRC = crc32(psFileInfo->u32CRC, pu8DataBuffer, s32Result);
		}
		while (s32Result);

		psFileInfo->bAvailable = TRUE;

		GCFreeMemory(pu8DataBuffer);

		fclose(fp);

		// Get more file info
		s32Result = _stati64(string, &sAttributes);

		if( 0 == s32Result )
		{
			psFileInfo->u32CreationTime = sAttributes.st_ctime;
			psFileInfo->u32ModificationTime = sAttributes.st_mtime;
		}

	}
	else
	{
		return(GC_NO_DATA);
	}

	return(GC_OK);
}

/* ************************************************************************* *\
** FUNCTION: GCfopen
\* ************************************************************************* */
GCFile GCfopen(const char* pu8Filename, 
			   const char* pu8Mode)
{
	char string[500];
	char *pu8Separator;

	pu8Separator = strstr(pu8Filename, "archive:");

	if (pu8Separator)
	{
		sprintf(string, "%s/%s", AppGetArchiveFilesDir(), (pu8Separator + 8));
	}
	else
	{
			strncpy(string, pu8Filename, sizeof(string));
	}

	return((GCFile) fopen(string, pu8Mode));
}

/* ************************************************************************* *\
** FUNCTION: GCfclose
\* ************************************************************************* */
INT32 GCfclose(GCFile psFileToClose)
{
	return(fclose((FILE *) psFileToClose));
}

/* ************************************************************************* *\
** FUNCTION: GCfseek
\* ************************************************************************* */
INT32 GCfseek(GCFile psFile, off_t s32Offset, INT32 s32Whence)
{
	return(fseek((FILE *) psFile, (INT32) s32Offset, s32Whence));
}

/* ************************************************************************* *\
** FUNCTION: GCfread
\* ************************************************************************* */
UINT32 GCfread(void *pvData, UINT32 u32Size, UINT32 u32Blocks, GCFile psFile)
{
	return(fread(pvData, u32Size, u32Blocks, (FILE *) psFile));
}

/* ************************************************************************* *\
** FUNCTION: GCftell
\* ************************************************************************* */
UINT32 GCftell(GCFile psFile)
{
	return(ftell((FILE *) psFile));
}

/* ************************************************************************* *\
** FUNCTION: GCfgetpos
\* ************************************************************************* */
INT32 GCfgetpos( GCFile stream, UINT32* pos )
{
	return fgetpos((FILE*)stream, (fpos_t*)pos);
}

/* ************************************************************************* *\
** FUNCTION: GCfwrite
\* ************************************************************************* */
INT32 GCfwrite(void* out_pBuffer, UINT32 in_cnt, UINT32 in_size, GCFile in_pFile)
{
	return fwrite(out_pBuffer, in_cnt, in_size, (FILE *)in_pFile);
}

/* ************************************************************************* *\
** FUNCTION: GCfprintf
\* ************************************************************************* */
INT32 GCfprintf(GCFile stream, const UINT8 * fmt, ...)
{
	va_list va;

	va_start(va, fmt);

	return vfprintf((FILE*)stream, fmt, va);
}

/* ************************************************************************* *\
** FUNCTION: GCfgets
\* ************************************************************************* */
UINT8* GCfgets( char* string,  UINT32 cnt,  GCFile stream  )
{
	return fgets(string, cnt, (FILE*)stream);
}

/* ************************************************************************* *\
** FUNCTION: GCfputs
\* ************************************************************************* */
INT32 GCfputs( const char* str, GCFile stream  )
{
	return fputs(str, (FILE*)stream);
}

/* ************************************************************************* *\
** FUNCTION: GCfeof
\* ************************************************************************* */
INT32 GCfeof( GCFile stream )
{
	return feof((FILE*)stream);
}

/* ************************************************************************* *\
** FUNCTION: GCferror
\* ************************************************************************* */
INT32 GCferror(GCFile stream )
{
	return ferror((FILE*)stream);
}

/* ************************************************************************* *\
** FUNCTION: GCfgetc
\* ************************************************************************* */
INT32 GCfgetc( GCFile stream )
{
	return fgetc((FILE*)stream);
}

/* ************************************************************************* *\
** FUNCTION: GCFileSize
\* ************************************************************************* */
UINT32 GCFileSize(GCFile psFile)
{
	UINT32 u32FileSize = 0;
	UINT32 u32FilePos = 0;

	u32FilePos = ftell((FILE *) psFile); // CLM Preserve file position. 
	fseek((FILE *) psFile, 0, SEEK_END);
	u32FileSize = ftell((FILE *) psFile);
	fseek((FILE *) psFile, u32FilePos, SEEK_SET);
	return(u32FileSize);
}


/* ************************************************************************* *\
** FUNCTION: GCFileFindSetup
\* ************************************************************************* */
typedef struct SFileFindInternal
{
	HANDLE hFindFile;
	WIN32_FIND_DATA sFindData;
	BOOL bFileFound;
	UINT32 u32LastAttributes;
	time_t eModificationTime;
	UINT64 u64FileSize;
} SFileFindInternal;

static UINT32 FindConvertAttributes(UINT32 u32WinAttr)
{
	UINT32 u32GCAttributes = 0;

	if (u32WinAttr & FILE_ATTRIBUTE_READONLY)
	{
		u32GCAttributes |= FILEATTR_ARDONLY;
	}

	if (u32WinAttr & FILE_ATTRIBUTE_HIDDEN)
	{
		u32GCAttributes |= FILEATTR_AHIDDEN;
	}

	if (u32WinAttr & FILE_ATTRIBUTE_SYSTEM)
	{
		u32GCAttributes |= FILEATTR_ASYSTEM;
	}

	if (u32WinAttr & FILE_ATTRIBUTE_DIRECTORY)
	{
		u32GCAttributes |= FILEATTR_ADIR;
	}

	if (u32WinAttr & FILE_ATTRIBUTE_ARCHIVE)
	{
		u32GCAttributes |= FILEATTR_AARCHIV;
	}

	if (u32WinAttr & FILE_ATTRIBUTE_NORMAL)
	{
		u32GCAttributes |= FILEATTR_ANORMAL;
	}

	return(u32GCAttributes);
}

SFileFind* GCFileFindSetup(const char* pu8Wildcard)
{
	HANDLE hFindFile;
	WIN32_FIND_DATA sFindData;
	SFileFindInternal *psFileFindInternal;
	char pu8FullPath[MAX_PATH];
	BOOL bResult = FALSE;
	SYSTEMTIME sSystemTime;
	struct tm sMakeTime;

	strncpy(pu8FullPath, pu8Wildcard, sizeof(pu8FullPath));
	
	hFindFile = FindFirstFile(pu8FullPath, &sFindData);
	
	psFileFindInternal = GCAllocateMemory(sizeof(*psFileFindInternal));
	GCASSERT(psFileFindInternal);

	psFileFindInternal->hFindFile = hFindFile;
	psFileFindInternal->sFindData = sFindData;
	
	if (INVALID_HANDLE_VALUE == hFindFile)
	{
		psFileFindInternal->bFileFound = FALSE;
		GCFreeMemory(psFileFindInternal);
		return(NULL);
	}
	else
	{
		psFileFindInternal->bFileFound = TRUE;

		psFileFindInternal->u32LastAttributes = FindConvertAttributes(psFileFindInternal->sFindData.dwFileAttributes);
		psFileFindInternal->u64FileSize = ((UINT64) psFileFindInternal->sFindData.nFileSizeHigh << 32) | psFileFindInternal->sFindData.nFileSizeLow;

		psFileFindInternal->sFindData = sFindData;

		bResult = FileTimeToSystemTime(&psFileFindInternal->sFindData.ftLastWriteTime,
									   &sSystemTime);
		GCASSERT(bResult);

		memset((void *) &sMakeTime, 0, sizeof(sMakeTime));

		sMakeTime.tm_sec = sSystemTime.wSecond;
		sMakeTime.tm_min = sSystemTime.wMinute;
		sMakeTime.tm_hour = sSystemTime.wHour;
		sMakeTime.tm_mday = sSystemTime.wDay;
		sMakeTime.tm_mon = sSystemTime.wMonth;
		sMakeTime.tm_year = sSystemTime.wYear - 1900;
		psFileFindInternal->eModificationTime = mktime(&sMakeTime);
	}

	return (SFileFind*) psFileFindInternal;
}

/* ************************************************************************* *\
** FUNCTION: GCFileFindNext
\* ************************************************************************* */
EGCResultCode GCFileFindNext(SFileFind* psFileFind, char* pu8Filename, UINT32 u32FilenameMaxLength)
{
	WIN32_FIND_DATA sFindData;
	BOOL bResult = FALSE;
	SFileFindInternal *psFileFindInternal = (SFileFindInternal*) psFileFind;
	SYSTEMTIME sSystemTime;
	SYSTEMTIME sLocalTime;
	struct tm sMakeTime;
	
	if (FALSE == psFileFindInternal->bFileFound)
		return GC_NO_DATA;

	strncpy(pu8Filename, psFileFindInternal->sFindData.cFileName, u32FilenameMaxLength);
	pu8Filename[u32FilenameMaxLength - 1] = '\0';

	bResult = FileTimeToSystemTime(&psFileFindInternal->sFindData.ftLastWriteTime,
								   &sSystemTime);
	GCASSERT(bResult);
	SystemTimeToTzSpecificLocalTime(NULL, &sSystemTime, &sLocalTime);

	memset((void *) &sMakeTime, 0, sizeof(sMakeTime));

	sMakeTime.tm_sec = sLocalTime.wSecond;
	sMakeTime.tm_min = sLocalTime.wMinute;
	sMakeTime.tm_hour = sLocalTime.wHour;
	sMakeTime.tm_mday = sLocalTime.wDay;
	sMakeTime.tm_mon = sLocalTime.wMonth - 1;
	sMakeTime.tm_year = sLocalTime.wYear - 1900;
	psFileFindInternal->eModificationTime = mktime(&sMakeTime);

	psFileFindInternal->u32LastAttributes = FindConvertAttributes(psFileFindInternal->sFindData.dwFileAttributes);
	psFileFindInternal->u64FileSize = ((UINT64) psFileFindInternal->sFindData.nFileSizeHigh << 32) | psFileFindInternal->sFindData.nFileSizeLow;

 	psFileFindInternal->bFileFound = FindNextFile(psFileFindInternal->hFindFile, &sFindData);
	psFileFindInternal->sFindData = sFindData;
	return GC_OK;
}

/* ************************************************************************* *\
** FUNCTION: GCFileFindGetAttribute
\* ************************************************************************* */
EGCResultCode GCFileFindGetAttribute(SFileFind *psCurrentFile,
									 UINT32 *pu32CurrentAttribute,
									 UINT64 *pu64Size,
									 time_t *peModificationTime)
{
	SFileFindInternal *psFileFindInternal = (SFileFindInternal*) psCurrentFile;

	if (NULL == psCurrentFile)
	{
		return(GC_NO_DATA);
	}

	if (pu32CurrentAttribute)
	{
		*pu32CurrentAttribute = psFileFindInternal->u32LastAttributes;
	}

	if (pu64Size)
	{
		*pu64Size = psFileFindInternal->u64FileSize;
	}

	if (peModificationTime)
	{
		*peModificationTime = psFileFindInternal->eModificationTime;
	}

	return(GC_OK);
}


/* ************************************************************************* *\
** FUNCTION: GCFileFindEnd
\* ************************************************************************* */
void GCFileFindEnd(SFileFind *psFileFind)
{
	SFileFindInternal *psFileFindInternal = (SFileFindInternal*) psFileFind;

	if (psFileFindInternal->hFindFile != INVALID_HANDLE_VALUE)
		FindClose(psFileFindInternal->hFindFile);
	
	GCFreeMemory(psFileFindInternal);
}


UINT32 GCGetPWD(UINT32 u32BufferLength, UINT8* psBuffer)
{
	// Windows currently doesn't return unicode.  How to get unicode path?
	return(GetCurrentDirectory(u32BufferLength, (LPSTR)psBuffer));
}


/* ************************************************************************* *\
** FUNCTION: GCDeleteFile
\* ************************************************************************* */
EGCResultCode GCDeleteFile(const char *in_pPath )
{
	char FileName[MAX_PATH] = "";

	MakeAppPath(FileName, in_pPath);
	if (remove(FileName) != -1)
	{
		return(GC_OK);
	}
	else
	{
		return((EGCResultCode) (ENOENT + GC_FILE_BASE));
	}
}

/* ************************************************************************* *\
** FUNCTION: GCmkdir
\* ************************************************************************* */
EGCResultCode GCmkdir( const char* in_pDirName )
{
	char FileName[MAX_PATH] = "";

	// Translate File Path
	MakeAppPath(FileName, in_pDirName);

	if (_mkdir(FileName) == 0)
	{
		return(GC_OK);
	}
	else
	{
		return((EGCResultCode) (ENOENT + GC_FILE_BASE));
	}
}

/* ************************************************************************* *\
** FUNCTION: GCrmdir
\* ************************************************************************* */
EGCResultCode GCrmdir( const char* in_pDirName )
{
	char FileName[MAX_PATH] = "";

	// Translate File Path
	MakeAppPath(FileName, in_pDirName);

	if (_rmdir(FileName) == 0)
	{
		return(GC_OK);
	}
	else
	{
		return((EGCResultCode) (ENOENT + GC_FILE_BASE));
	}
}

UINT32 GCGetDriveBitmap(void)
{
	return( GetLogicalDrives() );
}



/* ************************************************************************* *\
** FUNCTION: GCAllocateMemoryFromHeap
\* ************************************************************************* */
EGCResultCode GCAllocateMemoryFromHeap(SMemoryHeap *psMemoryHeap, UINT32 u32Size, void **ppvMemoryBlock, SMemoryInfo *psMemoryInfo)
{
	return(AllocateMemoryInternal(psMemoryHeap,
								  u32Size,
								  ppvMemoryBlock,
								  psMemoryInfo->pu8ModuleName,
								  psMemoryInfo->u32LineNumber,
								  psMemoryInfo->bClearBlock));
}

/* ************************************************************************* *\
** FUNCTION: GCFreeMemoryFromHeap
\* ************************************************************************* */
void GCFreeMemoryFromHeap(SMemoryHeap *psMemoryHeap, void **ppvMemoryLocation)
{
	(void) FreeMemory(psMemoryHeap,
					  ppvMemoryLocation);
}

EGCResultCode GCReallocMemoryFromHeap(SMemoryHeap *psMemoryHeap, UINT32 u32Size, void **ppvMemoryBlock, SMemoryInfo *psMemoryInfo)
{
	return(ReallocMemoryInternal(psMemoryHeap,
								 u32Size,
								 ppvMemoryBlock,
								 psMemoryInfo->pu8ModuleName,
								 psMemoryInfo->u32LineNumber));
}

/* ************************************************************************* *\
** FUNCTION: SDRAMDropAnchor
\* ************************************************************************* */
EGCResultCode SDRAMDropAnchor(void)
{
	// Doesn't do anything on this platform
	GCDropAnchor();
	return(GC_OK);
}

/* ************************************************************************* *\
** FUNCTION: SDRAMRaiseAnchor
\* ************************************************************************* */
EGCResultCode SDRAMRaiseAnchor(void)
{
	// Doesn't do anything on this platform
	GCRaiseAnchor();
	return(GC_OK);
}

/* ************************************************************************* *\
** FUNCTION: GCCreateMemoryHeap
\* ************************************************************************* */
EGCResultCode GCCreateMemoryHeap(UINT32 u32Size, SMemoryHeap *psMemoryHeap, UINT8 *pu8HeapName)
{
	return(CreateMemoryHeap(u32Size,
							psMemoryHeap,
							pu8HeapName));
}


/* ************************************************************************* *\
** FUNCTION: GCZipUncompressFromArchive
\* ************************************************************************* */
EGCResultCode GCZipUncompressFromArchive(const UINT8 *pu8FileWithinZip, 
										 UINT8 *pu8Location)
{
	char u8String[MAX_PATH];
	FILE *fp;
	UINT32 u32FileSize = 0;

	if (NULL == strstr(pu8FileWithinZip, "archive:"))
	{
	    sprintf((char *) u8String, "./%s/%s", AppGetFilesDir(), pu8FileWithinZip);
	}
	else
	{
		sprintf((char *) u8String, "./%s/%s", AppGetFilesDir(), pu8FileWithinZip + 8);
	}

	fp = fopen(u8String, "rb");

	if (NULL == fp)
	{
		return(GC_FILE_WITHIN_ZIP_NOT_FOUND);
	}

	// Figure out how big this file is

	fseek(fp, 0, SEEK_END);
	u32FileSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	fread((void *) pu8Location, 1, u32FileSize, fp);
	fclose(fp);

	return(GC_OK);
}

/* ************************************************************************* *\
** FUNCTION: GCZipGetUncompressedArchiveFileSize
\* ************************************************************************* */
EGCResultCode GCZipGetUncompressedArchiveFileSize(const UINT8 *pu8FileWithinZip, UINT32 *pu32UncompressedFileSize)
{
	char u8String[MAX_PATH];
	FILE *fp;

	if (NULL == strstr(pu8FileWithinZip, "archive:"))
	{
	    sprintf((char *) u8String, "./%s/%s", AppGetFilesDir(), pu8FileWithinZip);
	}
	else
	{
		sprintf((char *) u8String, "./%s/%s", AppGetFilesDir(), pu8FileWithinZip + 8);
	}

	fp = fopen(u8String, "rb");

	if (NULL == fp)
	{
		return(GC_FILE_WITHIN_ZIP_NOT_FOUND);
	}

	// Figure out how big this file is

	fseek(fp, 0, SEEK_END);
	*pu32UncompressedFileSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	fclose(fp);

	return(GC_OK);
}

//#pragma comment (lib, "../Win32/ziplib.lib")

static EGCResultCode _GCZipManip(const UINT8 *pu8ZipFilename, 
    const UINT8 *pu8FileWithinZip, 
    UINT8 *pu8Location, 
    UINT8 *pu8Password,
    UINT32 *pu32Len )
{
	unzFile zHandle;
	unz_file_info zFileInfo;
	int rc;
#if 0
	char string[500];
#endif


    // passwords not supported in win shim
    GCASSERT(pu8Password== NULL);
#if 0
	sprintf(string, "%s\\%s", AppGetFilesDir(), pu8ZipFilename);
	zHandle = unzOpen( string );
#else
        zHandle = unzOpen( pu8ZipFilename );
#endif

	if (zHandle == NULL)
		return GC_ZIP_FILE_NOT_FOUND;

	rc = unzLocateFile(zHandle,pu8FileWithinZip, 0);
	if (rc != UNZ_OK) 
    {
        unzClose(zHandle);
        if (rc == UNZ_END_OF_LIST_OF_FILE)
            return GC_FILE_WITHIN_ZIP_NOT_FOUND;
        return GC_ZIP_FILE_INTEGRITY_ERROR;
    }

	rc = unzOpenCurrentFile(zHandle); /* Try to open the file we want */
    if (rc != UNZ_OK)
    {
        unzClose(zHandle);
        return GC_ZIP_ITEM_OPEN_ERROR;
    }

    // Get size from ZIP .. make sure it matches.
    rc = unzGetCurrentFileInfo (zHandle, &zFileInfo, 
            NULL, 0, NULL, 0, NULL, 0);

	// If we failed to get file info, or size doesn't match,
	// return an error.
	if (rc != UNZ_OK) 
    {
        unzClose(zHandle);
        return GC_ZIP_ITEM_OPEN_ERROR;
    }

    if (pu32Len) *pu32Len = zFileInfo.uncompressed_size;

	// Don't load, we're just checking.
	if (pu8Location)
        unzReadCurrentFile(zHandle, pu8Location, 
                zFileInfo.uncompressed_size);

	unzCloseCurrentFile(zHandle);
	unzClose(zHandle);
	return GC_OK;
}


/* ************************************************************************* *\
** FUNCTION: GCZipGetUncompressedFileSize
\* ************************************************************************* */
EGCResultCode GCZipGetUncompressedFileSize(const UINT8* pu8ZipFilename, 
     const UINT8 *pu8FileWithinZip, 
     UINT32 *pu32UncompressedFileSize)
{
    return _GCZipManip( pu8ZipFilename, 
                        pu8FileWithinZip, 
                        NULL, 
                        NULL,
                        pu32UncompressedFileSize);

}

EGCResultCode GCZipValidateFileCRCFromZip(UINT8 *pu8ZipFilename,
										  SZipList *psZipList)
{
	unzFile psZipFile;
	unz_file_info sFileInfo;

	psZipFile = unzOpen((const char *) pu8ZipFilename);

	// Run through the list and set all availability to FALSE

	if (NULL == psZipFile)
	{
		return(GC_FILE_WITHIN_ZIP_NOT_FOUND);
	}

	while (psZipList->pu8Filename)
	{
		if (UNZ_OK == unzLocateFile(psZipFile, (const char *) psZipList->pu8Filename, 3))
		{
			// Found it! Let's get some info.

			if (UNZ_OK == unzGetCurrentFileInfo(psZipFile, &sFileInfo, NULL, 0, NULL, 0, NULL, 0))
			{
				psZipList->u32FileSize = sFileInfo.uncompressed_size;

				if (psZipList->u32CRC == (UINT32) sFileInfo.crc)
				{
					psZipList->bAvailable = TRUE;
				}
			}
		}

		++psZipList;
	}

	unzClose(psZipFile);
	return(GC_OK);
}

EGCResultCode GCZipGetCRCFromArchive(UINT8 *pu8FileWithinZip,
									 UINT32 *pu32CRC)
{
	GCFile psFile;
	UINT32 u32FileSize = 0;
	UINT32 u32BytesRead = 0;
	UINT8 *pu8Temp;

	psFile = GCfopen(pu8FileWithinZip, "rb");

	if (NULL == (void *) psFile)
	{
		return(GC_FILE_WITHIN_ZIP_NOT_FOUND);
	}

	// Figure out how big this file is

	GCfseek(psFile, 0, SEEK_END);
	u32FileSize = GCftell(psFile);
	GCfseek(psFile, 0, SEEK_SET);

	pu8Temp = GCAllocateMemory(u32FileSize);
	GCASSERT(pu8Temp);

	u32BytesRead = GCfread(pu8Temp, 1, u32FileSize, psFile);
	GCASSERT(u32BytesRead == u32FileSize);
	GCfclose(psFile);

	// Now CRC32 it

	*pu32CRC = crc32(0, pu8Temp, u32FileSize);

	GCFreeMemory(pu8Temp);
	return(GC_OK);
}
/* ************************************************************************* *\
** FUNCTION: GCZipUncompress
\* ************************************************************************* */
EGCResultCode GCZipUncompress(UINT8* pu8ZipFilename, 
    UINT8 *pu8FileWithinZip, 
    UINT8 *pu8Location, 
    UINT8 *pu8Password)
{
    return _GCZipManip( pu8ZipFilename, 
                        pu8FileWithinZip, 
                        pu8Location, 
                        pu8Password,
						NULL);
}





/* ************************************************************************* *\
** FUNCTION: GameExitCallback
\* ************************************************************************* */
void GameExitCallback(void)
{
	exit(1);
}


/* ************************************************************************* *\
** FUNCTION: GCTimerGetPeriod
\* ************************************************************************* */
UINT32 GCTimerGetPeriod(void)
{
	return(1);	// Hardcoded to 1MS
}


/* ************************************************************************* *\
** ************************************************************************* **
** Timers
** ************************************************************************* **
\* ************************************************************************* */

static BOOL sg_bTimerStopped = FALSE;

#define MAX_TIMERS		20

static STimerObject sg_sTimers[MAX_TIMERS];
static STimerObject *sg_psTimerHead = NULL;

/* ************************************************************************* *\
** FUNCTION: TimerFindPointer
\* ************************************************************************* */
static STimerObject *TimerFindPointer(STimerObject *psTimerToFind)
{
	UINT32 u32Loop = 0;
	STimerObject *psTimer = &sg_sTimers[0];

	while (u32Loop < MAX_TIMERS)
	{
		if (psTimerToFind == psTimer)
		{
			return(psTimerToFind);
		}

		++psTimer;
		++u32Loop;
	}

	GCASSERT_MSG("TimerFindPointer: Timer pointer invalid");
	return(NULL);
}

/* ************************************************************************* *\
** FUNCTION: TimerProc
\* ************************************************************************* */
static void CALLBACK TimerProc(UINT uTimerID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
	struct STimerObject *psTimerPtr;
	struct STimerObject *psPrior = NULL;

	// Rip through the active timer list and process them

	psTimerPtr = sg_psTimerHead;

	if (FALSE == sg_bTimerStopped)
	{
		while (psTimerPtr)
		{
			if (psTimerPtr->bRunning)
			{
				// Indicates the timer is running

				if (psTimerPtr->u32TimerValue <= GCTimerGetPeriod())
				{
					// If our reload value is 0, just stop the timer and call the handlers
					if (0 == psTimerPtr->u32ReloadValue)
					{
						if (psTimerPtr->Handler)
						{
							psTimerPtr->Handler(psTimerPtr->u32CallbackValue);
						}
						psTimerPtr->bRunning = FALSE;
					}
					else
					{
						// We have a reload value. So, let's compute any amount of remainder we have
						// and add it back in to the value for next time.

						do
						{
							psTimerPtr->u32TimerValue += psTimerPtr->u32ReloadValue;
							if (psTimerPtr->Handler)
							{
								psTimerPtr->Handler(psTimerPtr->u32CallbackValue);
							}
						}
						while (psTimerPtr->u32TimerValue < GCTimerGetPeriod());
					}
				}

				// Timer hasn't (yet) expired
				psTimerPtr->u32TimerValue -= GCTimerGetPeriod();
			}

			psTimerPtr = psTimerPtr->psNextLink;
		}
	}
}

#ifndef USE_GRATOS

/* ************************************************************************* *\
** FUNCTION: GCTimerCreate
\* ************************************************************************* */
EGCResultCode GCTimerCreate(STimerObject **ppsTimer)
{
	UINT32 u32Loop = 0;
	UINT32 u32InterruptPosture = 0;
	STimerObject *psTimer = &sg_sTimers[0];

	if (ppsTimer)
	{
		*ppsTimer = NULL;
	}

	sg_bTimerStopped = TRUE;

	while (u32Loop < MAX_TIMERS)
	{
		if (NULL == psTimer->Handler)
		{
			break;
		}

		++psTimer;
		++u32Loop;
	}

	if (MAX_TIMERS == u32Loop)
	{
		sg_bTimerStopped = FALSE;
		return(GC_NO_TIMER_SLOTS_AVAILABLE);
	}

	// Disable interrupts while we're monkeying with the timers

	psTimer->psNextLink = sg_psTimerHead;
	sg_psTimerHead = psTimer;

	if (ppsTimer)
	{
		*ppsTimer = psTimer;
	}

	sg_bTimerStopped = FALSE;

	return(GC_OK);
}

/* ************************************************************************* *\
** FUNCTION: GCTimerDelete
\* ************************************************************************* */
void GCTimerDelete(STimerObject *psTimerToDelete)
{
	STimerObject *psPrior = NULL;
	STimerObject *psTimer = sg_psTimerHead;

	sg_bTimerStopped = TRUE;

	while (psTimer)
	{
		if (psTimer == psTimerToDelete)
		{
			break;
		}

		psPrior = psTimer;
		psTimer = psTimer->psNextLink;
	}

	if (NULL == psTimer)
	{
		goto exitDelete;
	}

	if (psPrior)
	{
		psPrior->psNextLink = psTimer->psNextLink;
	}
	else
	{
		sg_psTimerHead = psTimer->psNextLink;
	}

	psTimer->Handler = NULL;

exitDelete:
	sg_bTimerStopped = FALSE;
}

/* ************************************************************************* *\
** FUNCTION: GCTimerSetCallback
\* ************************************************************************* */
EGCResultCode GCTimerSetCallback(STimerObject *psTimer,
								 void (*Handler)(UINT32),
								 UINT32 u32CallbackValue)
{
	sg_bTimerStopped = TRUE;
	psTimer->Handler = Handler;
	psTimer->u32CallbackValue = u32CallbackValue;
	sg_bTimerStopped = FALSE;

	return(GC_OK);
}

/* ************************************************************************* *\
** FUNCTION: GCTimerSetValue
\* ************************************************************************* */
EGCResultCode GCTimerSetValue(STimerObject *psTimer,
							  UINT32 u32InitialValue,
							  UINT32 u32ReloadValue)
{
	sg_bTimerStopped = TRUE;
	psTimer->u32TimerValue = u32InitialValue;
	psTimer->u32ReloadValue = u32ReloadValue;
	sg_bTimerStopped = FALSE;
	return(GC_OK);
}

/* ************************************************************************* *\
** FUNCTION: GCTimerStart
\* ************************************************************************* */
EGCResultCode GCTimerStart(STimerObject *psTimer)
{
	psTimer->bRunning = TRUE;
	return(GC_OK);
}

/* ************************************************************************* *\
** FUNCTION: GCTimerStop
\* ************************************************************************* */
EGCResultCode GCTimerStop(STimerObject *psTimer)
{
	psTimer->bRunning = FALSE;
	return(GC_OK);
}

#endif	// #ifndef USE_GRATOS

// extern CRITICAL_SECTION sg_sRAMCriticalSection;	// ram.c
typedef struct SStartupData
{
	UINT8 *pu8FreeMemoryBase;
	UINT32 u32FreeRAMSize;
	LPSTR lpwCmdLine;
	SCommandLineOption *psCommands;
} SStartupData;

static HANDLE sg_hStartupThread;
static UINT32 sg_u32StartupThread;
static BOOL sg_bRunning = TRUE;

static DWORD WINAPI StartupThreadProc(void *pvParameter)
{
	SStartupData *psData = (SStartupData *) pvParameter;

	GCASSERT(psData);

	GCStartup((UINT32) psData->pu8FreeMemoryBase,
			  psData->u32FreeRAMSize,
			  (UINT8 *) psData->lpwCmdLine,
			  psData->psCommands);

	sg_bRunning = FALSE;
	return(0);
}

static UINT32 sg_u32StartTime;

typedef HKEY (__stdcall SETUPDIOPENDEVREGKEY)(HDEVINFO, PSP_DEVINFO_DATA, DWORD, DWORD, DWORD, REGSAM);
typedef BOOL (__stdcall SETUPDICLASSGUIDSFROMNAME)(LPCTSTR, LPGUID, DWORD, PDWORD);
typedef BOOL (__stdcall SETUPDIDESTROYDEVICEINFOLIST)(HDEVINFO);
typedef BOOL (__stdcall SETUPDIENUMDEVICEINFO)(HDEVINFO, DWORD, PSP_DEVINFO_DATA);
typedef HDEVINFO (__stdcall SETUPDIGETCLASSDEVS)(LPGUID, LPCTSTR, HWND, DWORD);
typedef BOOL (__stdcall SETUPDIGETDEVICEREGISTRYPROPERTY)(HDEVINFO, PSP_DEVINFO_DATA, DWORD, PDWORD, PBYTE, DWORD, PDWORD);

typedef struct SSerialPort
{
	char u8FriendlyName[300];
	char u8DeviceName[100];
	SOSSemaphore sSerialAccessSemaphore;
	char *pu8ReceiveBuffer;
	UINT32 u32RXTail;
	UINT32 u32RXHead;
	volatile HANDLE hThreadHandle;
	HANDLE hSerial;
	OVERLAPPED sOverlappedRead;
	OVERLAPPED sOverlappedWrite;
	void (*ReceiveCallback)(UINT32 u32UARTNumber,
						    UINT8 *pu8Data,
							UINT32 u32Count);
} SSerialPort;

// Default serial receive buffer size
#define	SERIAL_RX_BUFFER_SIZE	16384
#define	MAX_SERIAL_PORTS		30
static UINT32 sg_u32SerialPortCount = 0;
static SSerialPort sg_sSerialPorts[MAX_SERIAL_PORTS];

static void SerialDeinitSerialIndex(UINT32 u32Index)
{
	// If the overlapped write handle still exists, close it
	if (sg_sSerialPorts[u32Index].sOverlappedWrite.hEvent != INVALID_HANDLE_VALUE)
	{
		if (CloseHandle(sg_sSerialPorts[u32Index].sOverlappedWrite.hEvent))
		{
			DebugOut("%s: sOverlappedWrite event cleared\n", __FUNCTION__);
		}
		else
		{
			DebugOut("%s: sOverlappedWrite event failure - code 0x%.8x\n", __FUNCTION__, GetLastError());
		}
		
		memset((void *) &sg_sSerialPorts[u32Index].sOverlappedWrite, 0, sizeof(sg_sSerialPorts[u32Index].sOverlappedWrite));
		sg_sSerialPorts[u32Index].sOverlappedWrite.hEvent = INVALID_HANDLE_VALUE;
	}

	// If the overlapped read handle still exists, close it
	if (sg_sSerialPorts[u32Index].sOverlappedRead.hEvent != INVALID_HANDLE_VALUE)
	{
		if (CloseHandle(sg_sSerialPorts[u32Index].sOverlappedRead.hEvent))
		{
#ifdef WATCH_SERIAL
			DebugOut("%s: OverlappedRead event cleared\n", __FUNCTION__);
#endif
		}
		else
		{
			DebugOut("%s: OverlappedRead event failure - code 0x%.8x\n", __FUNCTION__, GetLastError());
		}
		
		memset((void *) &sg_sSerialPorts[u32Index].sOverlappedRead, 0, sizeof(sg_sSerialPorts[u32Index].sOverlappedRead));
		sg_sSerialPorts[u32Index].sOverlappedRead.hEvent = INVALID_HANDLE_VALUE;
	}

	// If there's an active semaphore, delete it
	if (sg_sSerialPorts[u32Index].sSerialAccessSemaphore)
	{
#ifdef WATCH_SERIAL
		DebugOut("%s: Serial access semaphore destroyed\n", __FUNCTION__);
#endif
		(void) GCOSSemaphoreDelete(sg_sSerialPorts[u32Index].sSerialAccessSemaphore,
									DELETEOP_ALWAYS);
		sg_sSerialPorts[u32Index].sSerialAccessSemaphore = (SOSSemaphore) 0;
	}

	// If there's a serial handle, kill it
	if (sg_sSerialPorts[u32Index].hSerial != INVALID_HANDLE_VALUE)
	{
		// This will cause the thread to exit because the handle is no longer valid
		if (CloseHandle(sg_sSerialPorts[u32Index].hSerial))
		{
#ifdef WATCH_SERIAL
			DebugOut("%s: Serial port successfully closed\n", __FUNCTION__);
#endif
		}
		else
		{
			DebugOut("%s: Serial port closure failure - code 0x%.8x\n", __FUNCTION__, GetLastError());
		}
		sg_sSerialPorts[u32Index].hSerial = INVALID_HANDLE_VALUE;
	}

	// If there's an active thread, kill it
	if (sg_sSerialPorts[u32Index].hThreadHandle != INVALID_HANDLE_VALUE)
	{
		UINT32 u32Timer = 1000;	// 1 Second should be more than enough time to shut the port down

#ifdef WATCH_SERIAL
		DebugOut("%s: UART %u (%s) - terminating thread\n", __FUNCTION__, u32Index, sg_sSerialPorts[u32Index].u8FriendlyName);
#endif

		while (u32Timer)
		{
			// The thread will self terminate. When it does, it sets hThreadHandle to INVALID_HANDLE_VALUE. Wait for it.
			if (INVALID_HANDLE_VALUE == sg_sSerialPorts[u32Index].hThreadHandle)
			{
				// Successfully terminated
#ifdef WATCH_SERIAL
				DebugOut("%s: Serial thread successfully terminated\n", __FUNCTION__);
#endif
				break;
			}

			Sleep(1);
			--u32Timer;
		}

		if (0 == u32Timer)
		{
			DebugOut("%s: Serial port thread is being ornary. Killing the thread manually.\n", __FUNCTION__);
			if (TerminateThread(sg_sSerialPorts[u32Index].hThreadHandle,
								0))
			{
				DebugOut("%s: Successfully terminated serial thread\n", __FUNCTION__);
			}
			else
			{
				DebugOut("%s: TerminateThread() failed - code 0x%.8x\n", __FUNCTION__, GetLastError());
			}
		}

		sg_sSerialPorts[u32Index].hThreadHandle = INVALID_HANDLE_VALUE;
	}


	// If there's a receive buffer, dealloc it
	if (sg_sSerialPorts[u32Index].pu8ReceiveBuffer)
	{
		GCFreeMemory(sg_sSerialPorts[u32Index].pu8ReceiveBuffer);
		sg_sSerialPorts[u32Index].pu8ReceiveBuffer = NULL;
	}

	// Zero/NULL everything else out
	sg_sSerialPorts[u32Index].ReceiveCallback = NULL;
	sg_sSerialPorts[u32Index].u32RXHead = 0;
	sg_sSerialPorts[u32Index].u32RXTail = 0;
	memset((void *) sg_sSerialPorts[u32Index].u8DeviceName, 0, sizeof(sg_sSerialPorts[u32Index].u8DeviceName));
	memset((void *) sg_sSerialPorts[u32Index].u8FriendlyName, 0, sizeof(sg_sSerialPorts[u32Index].u8FriendlyName));
}

// Completely wipes out the serial structures, any active threads, buffers, etc...
static void SerialDeinit(void)
{
	UINT32 u32Loop;

	for (u32Loop = 0; u32Loop < (sizeof(sg_sSerialPorts) / sizeof(sg_sSerialPorts[0])); u32Loop++)
	{
		SerialDeinitSerialIndex(u32Loop);
	}
}

static void SerialInit(void)
{
	HINSTANCE hSetupAPI;
	DWORD u32LastError;
	SETUPDIOPENDEVREGKEY *SetupDiOpenDevRegKey = NULL;
	SETUPDIGETCLASSDEVS *SetupDiGetClassDevs = NULL;
	SETUPDIGETDEVICEREGISTRYPROPERTY *SetupDiGetDeviceRegistryProperty = NULL;
	SETUPDICLASSGUIDSFROMNAME *SetupDiClassGuidsFromName = NULL;
	SETUPDIDESTROYDEVICEINFOLIST *SetupDiDestroyDeviceInfoList = NULL;
	SETUPDIENUMDEVICEINFO *SetupDiEnumDeviceInfo = NULL;
	HDEVINFO hDevInfoSet;
	BOOL bMoreItems = TRUE;
	INT32 s32Index = 0;
	GUID *pGuids = NULL;
	DWORD dwGuids;
	UINT8 u8COMDevice[255];

	// Deallocate any existing serial port opens
	SerialDeinit();

	// No ports currently exist
	sg_u32SerialPortCount = 0;

	//DebugOut("%s: Searching for COM ports...\n", __FUNCTION__);

	hSetupAPI = LoadLibrary("SETUPAPI.DLL");
	if (NULL == hSetupAPI)
	{
		u32LastError = GetLastError();
		DebugOut("%s: LoadLibrary call failed - last error = 0x%.8x\n", __FUNCTION__, u32LastError);
		goto errorExit;
	}

	// Let's get the addresses of various COM functions
	SetupDiOpenDevRegKey = (SETUPDIOPENDEVREGKEY *) GetProcAddress(hSetupAPI, "SetupDiOpenDevRegKey");
	if (NULL == SetupDiOpenDevRegKey)
	{
		DebugOut("%s: Can't find function 'SetupDiOpenDevRegKey'\n", __FUNCTION__);
		goto errorExit;
	}
	
	SetupDiGetClassDevs = (SETUPDIGETCLASSDEVS *) GetProcAddress(hSetupAPI, "SetupDiGetClassDevsA");
	if (NULL == SetupDiGetClassDevs)
	{
		DebugOut("%s: Can't find function 'SetupDiGetClassDevsA'\n", __FUNCTION__);
		goto errorExit;
	}
	
	SetupDiGetDeviceRegistryProperty = (SETUPDIGETDEVICEREGISTRYPROPERTY *) GetProcAddress(hSetupAPI, "SetupDiGetDeviceRegistryPropertyA");
	if (NULL == SetupDiGetDeviceRegistryProperty)
	{
		DebugOut("%s: Can't find function 'SetupDiGetDeviceRegistryPropertyA'\n", __FUNCTION__);
		goto errorExit;
	}

	SetupDiDestroyDeviceInfoList = (SETUPDIDESTROYDEVICEINFOLIST *) GetProcAddress(hSetupAPI, "SetupDiDestroyDeviceInfoList");
	if (NULL == SetupDiDestroyDeviceInfoList)
	{
		DebugOut("%s: Can't find function 'SetupDiDestroyDeviceInfoList'\n", __FUNCTION__);
		goto errorExit;
	}

	SetupDiEnumDeviceInfo = (SETUPDIENUMDEVICEINFO *) GetProcAddress(hSetupAPI, "SetupDiEnumDeviceInfo");
	if (NULL == SetupDiEnumDeviceInfo)
	{
		DebugOut("%s: Can't find function 'SetupDiEnumDeviceInfo'\n", __FUNCTION__);
		goto errorExit;
	}
	
	SetupDiClassGuidsFromName = (SETUPDICLASSGUIDSFROMNAME *) GetProcAddress(hSetupAPI, "SetupDiClassGuidsFromNameA");
	if (NULL == SetupDiClassGuidsFromName)
	{
		DebugOut("%s: Can't find function 'SetupDiClassGuidsFromName'\n", __FUNCTION__);
		goto errorExit;
	}
	
	SetupDiClassGuidsFromName("Ports", NULL, 0, &dwGuids);
	if (0 == dwGuids)
	{
		DebugOut("%s: Can't get Ports GUID\n", __FUNCTION__);
		goto errorExit;
	}
	
	// Allocate guid spaces
	pGuids = calloc(1, sizeof(*pGuids) * dwGuids);
	if (NULL == pGuids)
	{
		DebugOut("%s: Can't calloc for GUID space\n", __FUNCTION__);
		goto errorExit;
	}

	// Now call the function again
	if (0 == SetupDiClassGuidsFromName("Ports", pGuids, dwGuids, &dwGuids))
	{
		DebugOut("%s:Can't fill in GUID table\n", __FUNCTION__);
		goto errorExit;
	}
		
	// Now create a device information set to enumerate all ports
	hDevInfoSet = SetupDiGetClassDevs(pGuids, NULL, NULL, DIGCF_PRESENT);
	if (INVALID_HANDLE_VALUE == hDevInfoSet)
	{
		DebugOut("%s: Can't open SetupDiGetClassDevs - Error 0x%.8x\n", __FUNCTION__, GetLastError());
		goto errorExit;
	}

	s32Index = 0;

	// Loop through all of our COM ports/items
	while (bMoreItems)
	{
		SP_DEVINFO_DATA sDeviceInfo;
		
		sDeviceInfo.cbSize = sizeof(sDeviceInfo);
		bMoreItems = SetupDiEnumDeviceInfo(hDevInfoSet, s32Index, &sDeviceInfo);
		if (bMoreItems)
		{
			HKEY hDeviceKey;
			BOOL bAdded = FALSE;
			char u8PortName[256];
			
			hDeviceKey = SetupDiOpenDevRegKey(hDevInfoSet, &sDeviceInfo, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_QUERY_VALUE);
			if (hDeviceKey)
			{
				DWORD dwSize;
				DWORD dwType = 0;
				
				bAdded = TRUE;
				
				u8PortName[0] = '\0';
				dwSize = sizeof(u8PortName);
				if ((RegQueryValueEx(hDeviceKey, "PortName", NULL, &dwType, u8PortName, &dwSize) == ERROR_SUCCESS) &&
					(REG_SZ == dwType))
				{
					if (('C' == u8PortName[0]) &&
						('O' == u8PortName[1]) &&
						('M' == u8PortName[2]))
					{
						bAdded = TRUE;
					}
				}
				
				RegCloseKey(hDeviceKey);
			}
			
			if (bAdded)
			{
				char u8FriendlyName[256];
				DWORD dwSize;
				DWORD dwType;
				
				u8FriendlyName[0] = '\0';
				dwSize = sizeof(u8FriendlyName);
				if (SetupDiGetDeviceRegistryProperty(hDevInfoSet, &sDeviceInfo, SPDRP_DEVICEDESC, &dwType, u8FriendlyName, dwSize, &dwSize) &&
					(REG_SZ == dwType))
				{
					// Found it! Let's see if it needs a workaround
					if ((toupper(u8PortName[0]) == 'C') &&
						(toupper(u8PortName[1]) == 'O') &&
						(toupper(u8PortName[2]) == 'M') &&
						(atol((const char *) &u8PortName[3]) >= 10))
					{
						// Needs workaound
						sprintf(u8COMDevice, "\\\\.\\COM%u", atol((const char *) &u8PortName[3]));
					}
					else
					{
						strcpy(u8COMDevice, u8PortName);
					}

					strncpy(sg_sSerialPorts[s32Index].u8FriendlyName, u8FriendlyName, sizeof(sg_sSerialPorts[s32Index].u8FriendlyName));
					strncpy(sg_sSerialPorts[s32Index].u8DeviceName, u8COMDevice, sizeof(sg_sSerialPorts[s32Index].u8DeviceName));
					++sg_u32SerialPortCount;
				}
			}
		}
		
		// Next index
		++s32Index;
	}			

errorExit:
//  DebugOut("%u Serial port(s) found\n", sg_u32SerialPortCount);
//  for (s32Index = 0; s32Index < (INT32) sg_u32SerialPortCount; s32Index++)
//  {
//  	DebugOut("  %u: %s (%s)\n", s32Index, sg_sSerialPorts[s32Index].u8DeviceName, sg_sSerialPorts[s32Index].u8FriendlyName);
//  }

	if (pGuids)
	{
		free(pGuids);
	}
	
	if (hSetupAPI)
	{
		FreeLibrary(hSetupAPI);
	}
	return;
}

static EGCResultCode SerialPortLock(UINT32 u32Index)
{
	EGCResultCode eResult;

	eResult = GCOSSemaphoreGet(sg_sSerialPorts[u32Index].sSerialAccessSemaphore,
							   0);

	return(eResult);
}

static EGCResultCode SerialPortUnlock(UINT32 u32Index)
{
	EGCResultCode eResult;

	eResult = GCOSSemaphorePut(sg_sSerialPorts[u32Index].sSerialAccessSemaphore);

	return(eResult);
}

EGCResultCode GCSerialSetDataCallback(UINT32 u32Index,
				 					  void (*ReceiveCallback)(UINT32 u32UARTNumber,
 									        UINT8 *pu8Data,
 										    UINT32 u32Count))
{
	// Go open a serial port
	if (u32Index >= sg_u32SerialPortCount)
	{
		DebugOut("%s: Attempted to set data callback on port index %u - out of range\n", __FUNCTION__, u32Index);
		return(GC_SERIAL_PORT_OUT_OF_RANGE);
	}

	// Is the port already open? If not, fail it
	if (INVALID_HANDLE_VALUE == sg_sSerialPorts[u32Index].hSerial)
	{
		DebugOut("%s: Port index %u not open\n", __FUNCTION__, u32Index);
		return(GC_SERIAL_PORT_NOT_OPEN);
	}

	sg_sSerialPorts[u32Index].ReceiveCallback = ReceiveCallback;
	return(GC_OK);
}

static DWORD WINAPI SerialReceiveThread(void *pvParameter)
{
	UINT32 u32SerialIndex = (UINT32) pvParameter;
	UINT8 u8Buffer[8192];
	SSerialPort *psSerial;

	ThreadSetName("Serial receive thread");

	psSerial = &sg_sSerialPorts[u32SerialIndex];

#ifdef WATCH_SERIAL
	DebugOut("%s: Started - Serial port %u (%s)\n", __FUNCTION__, u32SerialIndex, sg_sSerialPorts[u32SerialIndex].u8FriendlyName);
#endif

	while (1)
	{
		BOOL bResult;
		DWORD u32DataRead;

		u32DataRead = 0;

		// Create an event
		psSerial->sOverlappedRead.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		GCASSERT(psSerial->sOverlappedRead.hEvent != INVALID_HANDLE_VALUE);

		// Wait for data to come in
		bResult = ReadFile(psSerial->hSerial,
						   u8Buffer,
						   1,
						   &u32DataRead,
						   &psSerial->sOverlappedRead);

		if (FALSE == bResult)
		{
			if (ERROR_IO_PENDING == GetLastError())
			{
				// Wait on the overlapped I/O
				if (FALSE == GetOverlappedResult(psSerial->hSerial,
												 &psSerial->sOverlappedRead,
												 &u32DataRead,
												 TRUE))
				{
					if (ERROR_OPERATION_ABORTED == GetLastError())
					{
#ifdef WATCH_SERIAL
						DebugOut("%s: Thread terminate request\n", __FUNCTION__);
#endif
						goto exitThread;
					}

					DebugOut("%s: GetOverlappedResult() Error - 0x%.8x - exiting\n", __FUNCTION__, GetLastError());
					goto exitThread;
				}
				else
				{
					// Successful read.
					bResult = TRUE;
				}
			}
			else
			if (ERROR_OPERATION_ABORTED == GetLastError())
			{
#ifdef WATCH_SERIAL
				DebugOut("%s: Thread terminate request\n", __FUNCTION__);
#endif
				goto exitThread;
			}
			else
			{
				DebugOut("%s: ReadFile() Error - 0x%.8x - exiting\n", __FUNCTION__, GetLastError());
				goto exitThread;
			}
		}

		CloseHandle(psSerial->sOverlappedRead.hEvent);
		psSerial->sOverlappedRead.hEvent = INVALID_HANDLE_VALUE;

		if (bResult)
		{
			// Run through the received data and log it
			if (u32DataRead)
			{
//				DebugOut("%s: (%u) 0x%.2x\n", __FUNCTION__, u32SerialIndex, u8Buffer[0]);

				// Got data! If there's a callback, then call the callback with the data
				if (psSerial->ReceiveCallback)
				{
					psSerial->ReceiveCallback(u32SerialIndex,
											  u8Buffer,
											  (UINT32) u32DataRead);
				}
				else
				{
					EGCResultCode eResult;

					// Gotta stick it in the buffer.
					eResult = SerialPortLock(u32SerialIndex);
					if (GC_OK == eResult)
					{
						UINT8 *pu8DataPtr = u8Buffer;

						// Absorb the data and put it in our UART's buffer
						while (u32DataRead--)
						{
							psSerial->pu8ReceiveBuffer[psSerial->u32RXHead] = *pu8DataPtr;
							psSerial->u32RXHead++;
							if (psSerial->u32RXHead >= SERIAL_RX_BUFFER_SIZE)
							{
								psSerial->u32RXHead = 0;
							}

							// Ring buffer
							if (psSerial->u32RXTail == psSerial->u32RXHead)
							{
								psSerial->u32RXTail++;
								if (psSerial->u32RXTail >= SERIAL_RX_BUFFER_SIZE)
								{
									psSerial->u32RXTail = 0;
								}
							}

							++pu8DataPtr;
						}
					}
					else
					{
						DebugOut("%s: Serial port lock fault - code 0x%.8x - exiting\n", __FUNCTION__, eResult);
						goto exitThread;
					}

					// Now unlock the port
					eResult = SerialPortUnlock(u32SerialIndex);
					if (GC_OK == eResult)
					{
						// All good, drop through
					}
					else
					{
						DebugOut("%s: Serial port unlock fault - code 0x%.8x - exiting\n", __FUNCTION__, eResult);
						goto exitThread;
					}
				}
			}
			else
			{
				// This means timeout
			}
		}
		else
		{
			// Error of some sort
			DebugOut("%s: ReadFile() Error - 0x%.8x - exiting\n", __FUNCTION__, GetLastError());
			goto exitThread;
		}
	}

exitThread:
#ifdef WATCH_SERIAL
	DebugOut("%s: UART %u (%s) - Thread exiting\n", __FUNCTION__, u32SerialIndex, psSerial->u8FriendlyName);
#endif

	// Get rid of the overlapped read handle
	CloseHandle(psSerial->sOverlappedRead.hEvent);
	psSerial->sOverlappedRead.hEvent = INVALID_HANDLE_VALUE;

	// Get rid of the serial read handle
	if( psSerial->hSerial != INVALID_HANDLE_VALUE )
	{
		CloseHandle(psSerial->hSerial);
		psSerial->hSerial = INVALID_HANDLE_VALUE;
	}

	// Make the thread and serial handles invalid
	psSerial->hThreadHandle = INVALID_HANDLE_VALUE;
	ExitThread(0);
}


EGCResultCode GCSerialOpen(UINT32 u32Index)
{
	DCB dcbSerialParams;
	EGCResultCode eResult;
	BOOL bResult;
	COMMTIMEOUTS sTimeouts;

	// If it's 0, then do an init in case the user plugged in additional serial ports
	if (0 == u32Index)
	{
		SerialInit();
	}

	// Go open a serial port
	if (u32Index >= sg_u32SerialPortCount)
	{
		//DebugOut("%s: Attempted to open port index %u - out of range\n", __FUNCTION__, u32Index);
		return(GC_SERIAL_PORT_OUT_OF_RANGE);
	}

	// Is the port already open
	if (INVALID_HANDLE_VALUE != sg_sSerialPorts[u32Index].hSerial)
	{
		DebugOut("%s: Port index %u already open\n", __FUNCTION__, u32Index);
		return(GC_SERIAL_PORT_ALREADY_OPEN);
	}

	// Go open the port
	sg_sSerialPorts[u32Index].hSerial = CreateFile(sg_sSerialPorts[u32Index].u8DeviceName,
												   GENERIC_READ | GENERIC_WRITE,
												   0,
												   0,
												   OPEN_EXISTING,
												   FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
												   0);

	if (INVALID_HANDLE_VALUE == sg_sSerialPorts[u32Index].hSerial)
	{
		DebugOut("%s: Failed to open port index %u - %s (%s) - last error = 0x%.8x\n",
				   __FUNCTION__,
				   u32Index,
				   sg_sSerialPorts[u32Index].u8DeviceName,
				   sg_sSerialPorts[u32Index].u8FriendlyName,
				   GetLastError());
		return(GC_SERIAL_PORT_HAL_ERROR);
	}

	// Port has been opened. Set the control lines appropriately
	memset((void *) &dcbSerialParams, 0, sizeof(dcbSerialParams));
	dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

	if (GetCommState(sg_sSerialPorts[u32Index].hSerial,
					 &dcbSerialParams))
	{
		// Set sensible defaults for things
		dcbSerialParams.BaudRate = CBR_38400;
		dcbSerialParams.fBinary = TRUE;
		dcbSerialParams.fParity = FALSE;
		dcbSerialParams.fOutxCtsFlow = FALSE;
		dcbSerialParams.fOutxDsrFlow = FALSE;
		dcbSerialParams.fDtrControl = DTR_CONTROL_ENABLE;
		dcbSerialParams.fDsrSensitivity = FALSE;
		dcbSerialParams.fTXContinueOnXoff = FALSE;
		dcbSerialParams.fOutX = FALSE;
		dcbSerialParams.fInX = FALSE;
		dcbSerialParams.fErrorChar = FALSE;
		dcbSerialParams.fNull = FALSE;
		dcbSerialParams.fRtsControl = RTS_CONTROL_ENABLE;
		dcbSerialParams.fAbortOnError = FALSE;
		dcbSerialParams.ByteSize = 8;
		dcbSerialParams.Parity = NOPARITY;
		dcbSerialParams.StopBits = ONESTOPBIT;

		// Set them
		if (SetCommState(sg_sSerialPorts[u32Index].hSerial,
						 &dcbSerialParams))
		{
			eResult = GC_OK;
		}
		else
		{
			CloseHandle(sg_sSerialPorts[u32Index].hSerial);
			sg_sSerialPorts[u32Index].hSerial = INVALID_HANDLE_VALUE;
			return(GC_SERIAL_PORT_HAL_ERROR);
		}

	}
	else
	{
		CloseHandle(sg_sSerialPorts[u32Index].hSerial);
		sg_sSerialPorts[u32Index].hSerial = INVALID_HANDLE_VALUE;
		return(GC_SERIAL_PORT_HAL_ERROR);
	}

	// Set the COMM timeouts to forever
	memset((void *) &sTimeouts, 0, sizeof(sTimeouts));

	sTimeouts.ReadIntervalTimeout = 0;
	sTimeouts.ReadTotalTimeoutConstant = 0;
	sTimeouts.ReadTotalTimeoutMultiplier = 0;

	if (FALSE == SetCommTimeouts(sg_sSerialPorts[u32Index].hSerial,
									&sTimeouts))
	{
		DebugOut("%s: Serial port %u (%s) - SetCommTimeouts failed\n", __FUNCTION__, u32Index, sg_sSerialPorts[u32Index].u8FriendlyName);
		CloseHandle(sg_sSerialPorts[u32Index].hSerial);
		sg_sSerialPorts[u32Index].hSerial = INVALID_HANDLE_VALUE;
		return(GC_SERIAL_PORT_HAL_ERROR);
	}

	// Port successfully opened. Set up a buffer.
	sg_sSerialPorts[u32Index].pu8ReceiveBuffer = GCAllocateMemory(SERIAL_RX_BUFFER_SIZE);
	GCASSERT(sg_sSerialPorts[u32Index].pu8ReceiveBuffer);

	// Set the head/tail pointers to 0
	sg_sSerialPorts[u32Index].u32RXHead = 0;
	sg_sSerialPorts[u32Index].u32RXTail = 0;

	// Set up a serial data access semaphore
	eResult = GCOSSemaphoreCreate(&sg_sSerialPorts[u32Index].sSerialAccessSemaphore,
								  1);
	GCASSERT(GC_OK == eResult);

	// Set up overlapped read/write handles
	memset((void *) &sg_sSerialPorts[u32Index].sOverlappedRead, 0, sizeof(sg_sSerialPorts[u32Index].sOverlappedRead));
	memset((void *) &sg_sSerialPorts[u32Index].sOverlappedWrite, 0, sizeof(sg_sSerialPorts[u32Index].sOverlappedWrite));

	// Fire off a thread
	sg_sSerialPorts[u32Index].hThreadHandle = CreateThread(NULL,
															 0,
															 SerialReceiveThread,
															 (LPVOID) u32Index,
															 0,
															 NULL);
	GCASSERT(sg_sSerialPorts[u32Index].hThreadHandle != INVALID_HANDLE_VALUE);	

	// Now set the thread's priority to uber-high
	bResult = SetThreadPriority(sg_sSerialPorts[u32Index].hThreadHandle,
								THREAD_PRIORITY_TIME_CRITICAL);
	GCASSERT(bResult);

#ifdef WATCH_SERIAL
	DebugOut("%s: Port %u - %s (%s) successfully opened\n", __FUNCTION__,
			   u32Index,
			   sg_sSerialPorts[u32Index].u8DeviceName,
			   sg_sSerialPorts[u32Index].u8FriendlyName);
#endif

	return(eResult);
}


static UINT32 sg_u32ReturnCode = 0;

static SCommandLineOption sg_sCommands[] =
{
	{"-test",			FALSE,		FALSE,		"Test servers only",				NULL},
	{"-scriptdir",		FALSE,		TRUE,		"Specify script directory",			NULL},
	{"-localdb",		FALSE,		FALSE,		"Local server/database connection only",		NULL},
#ifdef PRE_LAUNCH
	{"-network",		FALSE,		FALSE,	"Networked Servers only",				NULL},
#else
	{"-local",			FALSE,		FALSE,	"Local script files only",				NULL},
#endif

	// List terminator
	{NULL}
};

/* ************************************************************************* *\
** FUNCTION: WinMain
\* ************************************************************************* */
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInstance, LPSTR lpwCmdLine, int nCmdShow)
{
	UINT8 *pu8FreeRAM = NULL;
	UINT32 u32Time = (UINT32) time(0);
	DWORD tmpDbgFlag = 0;
	UINT32 systemAppMemory = AppGetSystemMemory();
	SStartupData sData;
#ifndef _RELEASE
	SMALL_RECT sSmallRect;
	COORD sCoord;
#endif
	UINT32 u32Loop;
	time_t s32Time;

#ifndef _RELEASE
	// Create a console
	AllocConsole();

	memset((void *) &sCoord, 0, sizeof(sCoord));

	sCoord.X = 120;
	sCoord.Y = 9000;
	SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE),
							   sCoord);

	memset((void *) &sSmallRect, 0, sizeof(sSmallRect));
	sSmallRect.Left = 0;
	sSmallRect.Top = 0;
	sSmallRect.Right = sCoord.X - 1;
	sSmallRect.Bottom = 100;

	SetConsoleWindowInfo(GetStdHandle(STD_OUTPUT_HANDLE),
						 FALSE,
						 &sSmallRect);
#endif

	GOSPlatformInit();

	// Log our output
	s32Time = time(0);

	// Clear out the structures
	memset((void *) sg_sSerialPorts, 0, sizeof(sg_sSerialPorts));

	// Set up the serial handles as all being invalid
	for (u32Loop = 0; u32Loop < (sizeof(sg_sSerialPorts) / sizeof(sg_sSerialPorts[0])); u32Loop++)
	{
		sg_sSerialPorts[u32Loop].hThreadHandle = INVALID_HANDLE_VALUE;
		sg_sSerialPorts[u32Loop].hSerial = INVALID_HANDLE_VALUE;
		sg_sSerialPorts[u32Loop].sOverlappedRead.hEvent = INVALID_HANDLE_VALUE;
		sg_sSerialPorts[u32Loop].sOverlappedWrite.hEvent = INVALID_HANDLE_VALUE;
	}

	// Init the serial ports!
	SerialInit();

	tmpDbgFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
//	tmpDbgFlag |= _CRTDBG_DELAY_FREE_MEM_DF;
//  tmpDbgFlag |= _CRTDBG_LEAK_CHECK_DF;
//	tmpDbgFlag |= _CRTDBG_CHECK_ALWAYS_DF;
//	tmpDbgFlag |= _CRTDBG_CHECK_CRT_DF;
//	tmpDbgFlag |= _CRTDBG_ALLOC_MEM_DF;

    _CrtSetDbgFlag(tmpDbgFlag);

	MersenneInit(&u32Time,
  				 sizeof(u32Time));

	HostSetGameExitCallbackProcedure(GameExitCallback);
	HostGfxInit();
	HostInputInit();

	sg_u32StartTime = (UINT32) time(0);

#ifndef _MYHEAP
	// Don't allocate a heap
#else
	// can we really assert here?  I don't know.
	GCASSERT(systemAppMemory > BIOS_RESERVED_MEMORY );

	systemAppMemory -= BIOS_RESERVED_MEMORY;

	pu8FreeRAM = (UINT8 *) malloc(systemAppMemory);
	GCASSERT(pu8FreeRAM);
	memset((void *) pu8FreeRAM, 0, systemAppMemory);
#endif

	(void) timeSetEvent(GCTimerGetPeriod(), 1, TimerProc, 0, TIME_PERIODIC);

	// Initialize OS functions
	HostOSInit();

	memset((void *) &sData, 0, sizeof(sData));
	sData.lpwCmdLine = lpwCmdLine;
	sData.pu8FreeMemoryBase = pu8FreeRAM;
	sData.u32FreeRAMSize = systemAppMemory;
	sData.psCommands = sg_sCommands;
	
	sg_hStartupThread = CreateThread(NULL,
								  0,
								  StartupThreadProc,
								  (void *) &sData,
								  0,
								  &sg_u32StartupThread);

	while (sg_bRunning)
	{
		// Just process messages 
		HostProcessMessages();
		Sleep(10);
	}

	return(sg_u32ReturnCode);
}

/* ************************************************************************* *\
** FUNCTION: GCWaitMS
\* ************************************************************************* */
void GCWaitMS(UINT32 u32Milliseconds)
{
	Sleep((DWORD) u32Milliseconds);
}

/* ************************************************************************* *\
** FUNCTION: GCBoardReset
\* ************************************************************************* */
void GCBoardReset(void)
{
	exit(0);
}

/* ************************************************************************* *\
** FUNCTION: GCerror
\* ************************************************************************* */
INT32 GCerror(void)
{
	return(errno);
}

/* ************************************************************************* *\
** FUNCTION: GCGetTime
\* ************************************************************************* */
UINT32 GCGetTimeMS( void )
{
	return timeGetTime();
}


void GCSetUserHeap(SMemoryHeap *psMemoryUserHeap)
{
}

EGCResultCode GCProgramProductID(UINT32 u32ProductID)
{
	return(GC_OK);
}

EGCResultCode GCGetProductID(UINT32 *pu32ProductID)
{
	*pu32ProductID = PRODUCT_ID;
	return(GC_OK);
}

EGCResultCode GCPointerGetPosition(UINT32 u32Instance,
								   UINT32 *pu32XPos,
								   UINT32 *pu32YPos,
								   UINT32 *pu32Buttons)
{
	INT32 s32X;
	INT32 s32Y;
	UINT8 u8Buttons;

	if (u32Instance)
	{
		return(GC_POINTER_NOT_SUPPORTED);
	}

	u8Buttons = SDL_GetMouseState(&s32X,
								  &s32Y);

	if (pu32Buttons)
	{
		*pu32Buttons = u8Buttons;
	}

	if (pu32XPos)
	{
		*pu32XPos = (UINT32) s32X;
	}

	if (pu32YPos)
	{
		*pu32YPos = (UINT32) s32Y;
	}

	return(GC_OK);
}


/* ************************************************************************* *\
** FUNCTION: GCOutputSet
\* ************************************************************************* */
void GCOutputSet(UINT8 u8OutputNumber,
				 BOOL bState)
{
/*	DebugOut("Port %d, set ", u8OutputNumber);
	if (bState)
	{
		DebugOut("on\n");
	}
	else
	{
		DebugOut("off\n");
	} */
}

EGCResultCode RTCGetTime(time_t *peTime,
						 BOOL *pbTimeNotSet)
{
	if (peTime)
	{
		*peTime = time(0);
	}

	if (pbTimeNotSet)
	{
//		*pbTimeNotSet = FALSE;
		*pbTimeNotSet = TRUE;
	}

	return(GC_OK);
}

EGCResultCode RTCSetTime(time_t eTime)
{
	// Does nothing under Win32
	return(GC_OK);
}

EGCResultCode RTCGetPowerOnSeconds(UINT32 *pu32Seconds)
{
	if (pu32Seconds)
	{
		*pu32Seconds = (UINT32) (time(0) - sg_u32StartTime);
	}
	
	return(GC_OK);
}

EGCResultCode RTCSetTimezoneOffset(INT32 s32Offset)
{
	return(GC_OK);
}

EGCResultCode GCSetLCDBacklightLevel(UINT8 u8Intensity)
{
	// Doesn't do anything under Windows
	return(GC_OK);
}

EGCResultCode RTCGetTimezoneOffset(INT32 *ps32Offset)
{
	if (ps32Offset)
	{
		*ps32Offset = 0;
	}

	return(GC_OK);
}

EGCResultCode RTCSetDST(BOOL bDSTEnabled)
{
	return(GC_OK);
}

EGCResultCode RTCGetDST(BOOL *pbDSTEnabled)
{
	*pbDSTEnabled = TRUE;
	return(GC_OK);
}


/*
#define PCBID_GAMEROOM_CLASSICS		0x25
#define PCBID_DOUG					0xeb
#define PCBID_JET					0x0a
#define PCBID_EA50					0x29
#define	PCBID_UC					0x41479425
*/

UINT32 GCGetPCBID(void)
{
	return(PCBID_UC);
}

EGCResultCode GCIsDir(char *pu8DirectoryName)
{
	DWORD dwAttrib;
	UINT32 u32Length = strlen(pu8DirectoryName) + 2;
	char *pu8Filename;

	pu8Filename = GCAllocateMemory(u32Length);
	if (NULL == pu8Filename)
	{
		return(GC_OUT_OF_MEMORY);
	}

	strncpy(pu8Filename, pu8DirectoryName, u32Length);
	
	dwAttrib = GetFileAttributes((LPCSTR) pu8Filename);
	GCFreeMemory(pu8Filename);

	if ((dwAttrib != INVALID_FILE_ATTRIBUTES) &&
		(dwAttrib & FILE_ATTRIBUTE_DIRECTORY))
	{
		return(GC_OK);
	}
	else
	{
		return((EGCResultCode) (ENOENT + GC_FILE_BASE));
	}
}

EGCResultCode GCFileOpen(GCFile *peHandle, const char *pu8Filename, const char* pu8Mode)
{
	FILE *fp;
	char string[500];
	char *pu8Separator;

	GCASSERT(peHandle);
	// Zero out the file handle
	*peHandle = (GCFile) 0;

	pu8Separator = strstr(pu8Filename, "archive:");

	if ((strcmp(pu8Mode, "r") == 0) ||
		(strcmp(pu8Mode, "R") == 0))
	{
		pu8Mode = "rb";
	}

	if ((strcmp(pu8Mode, "w") == 0) ||
		(strcmp(pu8Mode, "W") == 0))
	{
		pu8Mode = "wb";
	}

	if (pu8Separator)
	{
		// Pass it through
		strcpy(string, pu8Filename);
	}
	else
	{
			strncpy(string, pu8Filename, sizeof(string));
	}

	// Let's try to open the file up

	fp = (FILE *) GCfopen(pu8Filename, pu8Mode);
	if (NULL == fp)
	{
		return((EGCResultCode) (ENOENT + GC_FILE_BASE));
	}

	*peHandle = (GCFile) fp;
	return(GC_OK);
}

EGCResultCode GCFileClose(GCFile *peHandle)
{
	int s32Result;

	if ((NULL == peHandle) || (*peHandle == (GCFile) NULL))
	{
		return(GC_OK);
	}

	s32Result = fclose((FILE *) *peHandle);
	if (s32Result < 0)
	{
		return((EGCResultCode) (errno + GC_FILE_BASE));
	}
	else
	{
		*peHandle = (GCFile) 0;
		return(GC_OK);
	}
}

EGCResultCode GCFileSizeByHandle(GCFile eHandle, UINT64 *pu64FileSize)
{
	UINT32 u32FilePos = 0;

	u32FilePos = GCftell(eHandle);
	GCfseek(eHandle, 0, SEEK_END);
	*pu64FileSize = GCftell(eHandle);
	GCfseek(eHandle, u32FilePos, SEEK_SET);

	return(GC_OK);
}

EGCResultCode GCFileSizeByFilename(const char *pu8Filename, UINT64 *pu64FileSize)
{
	GCFile psFile;
	EGCResultCode eResult;

	psFile = GCfopen(pu8Filename, "rb");

	if (NULL == (void *) psFile)
	{
		return(0);
	}

	eResult = GCFileSizeByHandle(psFile,
								 pu64FileSize);
	GCfclose(psFile);
	return(eResult);
}

EGCResultCode GCFileSeek(GCFile eHandle, off_t s64Offset, INT32 s32Whence)
{
	int s32Result;

	s32Result = fseek((FILE *) eHandle, (long) s64Offset, s32Whence);
	if (0 == s32Result)
	{
		return(GC_OK);
	}
	else
	{
		return((EGCResultCode) (errno + GC_FILE_BASE));
	}
}

EGCResultCode GCFileRead(void *pvData, UINT32 u32ByteCount, UINT32 *pu32DataRead, GCFile eHandle)
{
	size_t s32Size;

	s32Size = fread(pvData, 1, u32ByteCount, (FILE *) eHandle);
	if (s32Size < 0)
	{
		return((EGCResultCode) (ferror((FILE *) eHandle) + GC_FILE_BASE));
	}

	*pu32DataRead = (UINT32) s32Size;
	return(GC_OK);
}

EGCResultCode GCFileTell(off_t *ps64Offset, GCFile eHandle)
{
	int s32Result;

	s32Result = ftell((FILE *) eHandle);
	if (s32Result < 0)
	{
		return((EGCResultCode) (errno + GC_FILE_BASE));
	}
	else
	{
		*ps64Offset = s32Result;
		return(GC_OK);
	}
}

EGCResultCode GCFileGetPos(GCFile eHandle, off_t *ps64Offset)
{
	int s32Result;

	s32Result = fgetpos((FILE *) eHandle, ps64Offset);
	if (s32Result < 0)
	{
		return((EGCResultCode) (errno + GC_FILE_BASE));
	}
	else
	{
		return(GC_OK);
	}
}

EGCResultCode GCFileWrite(void *pvData, UINT32 u32ByteCount, UINT32 *pu32DataWritten, GCFile eHandle)
{
	size_t s32Size;

	s32Size = fwrite(pvData, 1, u32ByteCount, (FILE *) eHandle);
	if (s32Size < 0)
	{
		return((EGCResultCode) (ferror((FILE *) eHandle) + GC_FILE_BASE));
	}

	*pu32DataWritten = (UINT32) s32Size;
	return(GC_OK);
}

EGCResultCode GCFileGets(char **ppu8String, char *pu8Buffer, UINT32 u32BufferSize, GCFile eHandle)
{
	char *pu8Data;

	pu8Data = fgets(pu8Buffer, u32BufferSize, (FILE *) eHandle);
	if (NULL == pu8Data)
	{
		if (ferror((FILE *) eHandle))
		{
			return((EGCResultCode) (ferror((FILE *) eHandle) + GC_FILE_BASE));
		}
		else
		{
			return(GC_OK);
		}
	}
	else
	{
		*ppu8String = pu8Data;
		return(GC_OK);
	}
}

EGCResultCode GCFilePuts(const char *pu8String, GCFile eHandle)
{
	int s32Result;

	s32Result = fputs(pu8String, (FILE *) eHandle);
	if (-1 == s32Result)
	{
		return((EGCResultCode) (errno + GC_FILE_BASE));
	}
	else
	{
		return(GC_OK);
	}
}

EGCResultCode GCFileEOF(GCFile eHandle, BOOL *pbFileEOF)
{
	EGCResultCode eResult;
	off_t s64FileSize;
	off_t s64Position;

	eResult = GCFileTell(&s64Position, eHandle);
	if (eResult != GC_OK)
	{
		return(eResult);
	}

	eResult =  GCFileSizeByHandle(eHandle, &s64FileSize);
	if (eResult != GC_OK)
	{
		return(eResult);
	}

	if (feof((FILE *) eHandle) || (s64Position >= (long) s64FileSize))
	{
		*pbFileEOF = TRUE;
	}
	else
	{
		*pbFileEOF = FALSE;
	}

	return(GC_OK);
}

EGCResultCode GCFileFgetc(char *pu8Character, GCFile eHandle)
{
	int s32Result;

	s32Result = fgetc((FILE *) eHandle);
	if (s32Result < 0)
	{
		return((EGCResultCode) (errno + GC_FILE_BASE));
	}
	else
	{
		*pu8Character = (char) s32Result;
		return(GC_OK);
	}
}

EGCResultCode GCFileSystemSync(void)
{
	// Doesn't do anything under Windows
	return(GC_OK);
}

LEX_CHAR sg_eCurrentDirectory[MAX_PATH];

LEX_CHAR* GCGetCurrentDirectory( void )
{
   (void) GetCurrentDirectory(sizeof(sg_eCurrentDirectory), sg_eCurrentDirectory);
   return(sg_eCurrentDirectory);
}

// Thread naming is an MSVC debugger exception. See MSDN for more info. That's
// where I got it.

static EGCResultCode SerialCheckIndex(UINT32 u32Index,
									  BOOL bPortOpen,
									  HANDLE *phSerial)
{
	if (u32Index >= sg_u32SerialPortCount)
	{
		return(GC_SERIAL_PORT_OUT_OF_RANGE);
	}

	if (phSerial)
	{
		*phSerial = sg_sSerialPorts[u32Index].hSerial;
	}

	// See if the port's open

	if (bPortOpen)
	{
		if (INVALID_HANDLE_VALUE != sg_sSerialPorts[u32Index].hSerial)
		{
			return(GC_OK);
		}
		else
		{
			// We're expecting the port to be open and it isn't
			return(GC_SERIAL_PORT_NOT_OPEN);
		}
	}
	else
	{
		if (INVALID_HANDLE_VALUE == sg_sSerialPorts[u32Index].hSerial)
		{
			return(GC_OK);
		}
		else
		{
			// We're expecting the port to be closed and it isn't
			return(GC_SERIAL_PORT_OPEN);
		}
	}

	// Shouldn't get here
	GCASSERT(0);
	return(GC_OK);
}

EGCResultCode GCSerialClose(UINT32 u32Index)
{
	EGCResultCode eResult;
	HANDLE hSerial;

	eResult = SerialCheckIndex(u32Index,
							   TRUE,
							   &hSerial);

	if (GC_OK == eResult)
	{
		// Go deinit the serial thread
		SerialDeinitSerialIndex(u32Index);
		eResult = GC_OK;
	}

	return(eResult);
}

EGCResultCode GCSerialGetControlLines(UINT32 u32Index,
									  UINT32 *pu32ControlStates)
{
	EGCResultCode eResult;

	// Not implemented
	GCASSERT(0);
	eResult = SerialCheckIndex(u32Index,
							   TRUE,
							   NULL);

	if (GC_OK == eResult)
	{
		if (pu32ControlStates)
		{
			*pu32ControlStates = 0;
		}
	}

	return(GC_OK);
}

EGCResultCode GCSerialGetDebugPortNumber(UINT32 *pu32DebugPortNumber)
{
	return(GC_SERIAL_NO_DEBUG_PORT);
}

EGCResultCode GCSerialClearBuffers(UINT32 u32Index,
								   BOOL bClearReceive,
								   BOOL bClearSend)
{
	EGCResultCode eResult;
	HANDLE hSerial;

	eResult = SerialCheckIndex(u32Index,
							   TRUE,
							   &hSerial);

	if (GC_OK == eResult)
	{
		if (bClearReceive)
		{
			// Lock the serial port
			eResult = SerialPortLock(u32Index);
			if (eResult != GC_OK)
			{
				goto errorExit;
			}

			// It's locked. Clear the buffer.
			sg_sSerialPorts[u32Index].u32RXHead = 0;
			sg_sSerialPorts[u32Index].u32RXTail = 0;

			// Unlock the serial port
			eResult = SerialPortUnlock(u32Index);
		}

		if (bClearSend)
		{
			BOOL bResult = FALSE;

			bResult =  FlushFileBuffers(hSerial);

			if (bResult)
			{
				eResult = GC_SERIAL_PORT_HAL_ERROR;
			}
			else
			{
				eResult = GC_OK;
			}
		}
	}

errorExit:
	return(eResult);
}

EGCResultCode GCSerialSetBaudRate(UINT32 u32Index,
								  UINT32 u32BaudRate)
{
	EGCResultCode eResult;
	HANDLE hSerial;

	eResult = SerialCheckIndex(u32Index,
							   TRUE,
							   &hSerial);

	if (GC_OK == eResult)
	{
		DCB dcbSerialParams;

		// Port has been opened. Set the control lines appropriately
		memset((void *) &dcbSerialParams, 0, sizeof(dcbSerialParams));
		dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

		if (GetCommState(hSerial,
						 &dcbSerialParams))
		{
			dcbSerialParams.BaudRate = u32BaudRate;

			if (SetCommState(hSerial,
							 &dcbSerialParams))
			{
#ifdef WATCH_SERIAL
				DebugOut("GCSerialSetBaudRate: UART=%u, Baud rate=%u\n", u32Index, u32BaudRate);
#endif
			}
			else
			{
				eResult = GC_SERIAL_PORT_HAL_ERROR;
			}
		}
		else
		{
			eResult = GC_SERIAL_PORT_HAL_ERROR;
		}
	}

	return(eResult);
}

EGCResultCode GCSerialIsOpen(UINT32 u32Index)
{
	EGCResultCode eResult;
	HANDLE hSerial;

	eResult = SerialCheckIndex(u32Index,
							   TRUE,
							   &hSerial);

	if (GC_OK == eResult)
	{
		if (hSerial != INVALID_HANDLE_VALUE)
		{
			eResult = GC_OK;
		}
		else
		{
			eResult = GC_SERIAL_PORT_NOT_OPEN;
		}
	}

	return(eResult);
}

EGCResultCode GCSerialFlushSend(UINT32 u32Index,
								UINT32 u32Timeout)
{
	EGCResultCode eResult;
	HANDLE hSerial;

	eResult = SerialCheckIndex(u32Index,
							   TRUE,
							   &hSerial);

	if (GC_OK == eResult)
	{
		BOOL bResult;

		bResult =  FlushFileBuffers(hSerial);

		if (FALSE == bResult)
		{
			eResult = GC_SERIAL_PORT_HAL_ERROR;
		}
		else
		{
			eResult = GC_OK;
		}
	}

	return(eResult);
}

EGCResultCode GCSerialSetBufferSizes(UINT32 u32Index,
									 UINT32 u32SendBufferSize,
									 UINT32 u32ReceiveBufferSize)
{
	if (u32Index >= sg_u32SerialPortCount)
	{
		return(GC_SERIAL_PORT_OUT_OF_RANGE);
	}

	// Not implemented
	GCASSERT(0);
	DebugOut("GCSerialSetBufferSizes: UART=%u, TXSize=%u, RXSize=%u\n", u32Index, u32SendBufferSize, u32ReceiveBufferSize);

	return(GC_OK);
}

EGCResultCode GCSerialGetData(UINT32 u32Index,
							  UINT8 *pu8DataBuffer,
							  UINT32 *pu32Count,
							  UINT32 u32Timeout)
{
	UINT32 u32BytesRead = 0;
	EGCResultCode eResult;
	HANDLE hSerial;
	UINT32 u32MaxLen = *pu32Count;

	eResult = SerialCheckIndex(u32Index,
							   TRUE,
							   &hSerial);

	if (GC_OK == eResult)
	{
		// Lock the port
		eResult = SerialPortLock(u32Index);
		if (GC_OK == eResult)
		{
			SSerialPort *psSerial = &sg_sSerialPorts[u32Index];

			// Clear the count received
			*pu32Count = 0;

			// It's locked. Let's snag some data if there is any.
			while ((psSerial->u32RXHead != psSerial->u32RXTail) &&
				   (u32MaxLen))
			{
				*pu8DataBuffer = psSerial->pu8ReceiveBuffer[psSerial->u32RXTail++];
				if (psSerial->u32RXTail >= SERIAL_RX_BUFFER_SIZE)
				{
					psSerial->u32RXTail = 0;
				}

				++pu8DataBuffer;
				++(*pu32Count);
				--u32MaxLen;
			}

			// Unlock the port
			eResult = SerialPortUnlock(u32Index);
		}
	}

	return(eResult);
}

EGCResultCode GCSerialSendData(UINT32 u32Index,
							   UINT8 *pu8DataBuffer,
							   UINT32 u32Count,
							   UINT32 u32Timeout)
{
	EGCResultCode eResult;
	HANDLE hSerial;
//	UINT32 u32Loop;
	SSerialPort *psSerial;
	SYSTEMTIME sSystemTime;
//	char u8String[500];
//	char *pu8Data;

	GetSystemTime(&sSystemTime);

/*	sprintf(u8String, "%s: %.2u:%.2u:%.2u.%.3u - (%u) bytes: ", __FUNCTION__, sSystemTime.wHour, sSystemTime.wMinute, sSystemTime.wSecond, sSystemTime.wMilliseconds, u32Count);

	pu8Data = (char *) (u8String + strlen(u8String));

	for (u32Loop = 0; u32Loop < u32Count; u32Loop++)
	{
		UINT8 u8Data;
		UINT8 u8Char;

		u8Data = pu8DataBuffer[u32Loop];
		u8Char = u8Data >> 4;
		if (u8Char > 9)
		{
			u8Char += 7;
		}
		u8Char += '0';
		*pu8Data = u8Char;
		++pu8Data;
		u8Char = u8Data & 0x0f;
		if (u8Char > 9)
		{
			u8Char += 7;
		}
		u8Char += '0';
		*pu8Data = u8Char;
		++pu8Data;
		*pu8Data = ' ';
		++pu8Data;
	}
	*pu8Data = '\n';
	++pu8Data;
	*pu8Data = '\0';
	DebugOut(u8String);
*/

	eResult = SerialCheckIndex(u32Index,
							   TRUE,
							   &hSerial);

	if (GC_OK == eResult)
	{
		UINT32 u32BytesWritten = 0;

		psSerial = &sg_sSerialPorts[u32Index];

		psSerial->sOverlappedWrite.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		GCASSERT(psSerial->sOverlappedWrite.hEvent != INVALID_HANDLE_VALUE);

		if (FALSE == WriteFile(hSerial,
							   pu8DataBuffer,
							   u32Count,
							   &u32BytesWritten,
							   &psSerial->sOverlappedWrite))
		{
			// If the last event was an I/O pending, then wait for the overlapped result
			if (ERROR_IO_PENDING == GetLastError())
			{
				// Wait for the overlapped I/O to be completed
				if (FALSE == GetOverlappedResult(hSerial, &psSerial->sOverlappedWrite, &u32BytesWritten, TRUE))
				{
					eResult = GC_SERIAL_PORT_HAL_ERROR;
					goto errorExit;
				}
				else
				{
					// Successfully completed
				}
			}
			else
			{
				DebugOut("%s: Failed - GetLastError() = 0x%.8x\n", __FUNCTION__, GetLastError());
				eResult = GC_SERIAL_PORT_HAL_ERROR;
				goto errorExit;
			}
		}

		// Data sent. Let's see if we have the right # of bytes.
		if (u32BytesWritten != u32Count)
		{
			eResult = GC_SERIAL_PORT_TRUNCATED_WRITE;
		}
		else
		{
			eResult = GC_OK;
		}
	}

errorExit:
	if (psSerial->sOverlappedWrite.hEvent != INVALID_HANDLE_VALUE)
	{
		CloseHandle(psSerial->sOverlappedWrite.hEvent);
		psSerial->sOverlappedWrite.hEvent = INVALID_HANDLE_VALUE;
	}


	return(eResult);
}

EGCResultCode GCSerialSetControlLines(UINT32 u32Index,
									  UINT32 u32ControlLines,
									  UINT32 u32ControlStates)
{
	EGCResultCode eResult;
	HANDLE hSerial;

	eResult = SerialCheckIndex(u32Index,
							   TRUE,
							   &hSerial);

	if (GC_OK == eResult)
	{
		DCB dcbSerialParams;

		// Port has been opened. Set the control lines appropriately
		memset((void *) &dcbSerialParams, 0, sizeof(dcbSerialParams));
		dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

		if (GetCommState(hSerial,
						 &dcbSerialParams))
		{
			UINT32 u32Bits = u32ControlLines & SERIAL_LINE_DATA_MASK;
			UINT32 u32Parity = u32ControlLines & SERIAL_LINE_PARITY_MASK;
			UINT32 u32StopBits = u32ControlLines & SERIAL_LINE_STOP_BIT_MASK;
			UINT32 u32FlowControl = u32ControlLines & SERIAL_FLOW_CONTROL_MASK;

			// Only "none" supported for flow control
			GCASSERT(SERIAL_FLOW_CONTROL_NONE == u32FlowControl);

			// Data bits
			if (SERIAL_LINE_DATA_BITS_8 == u32Bits)
			{
				dcbSerialParams.ByteSize = 8;
			}
			else
			if (SERIAL_LINE_DATA_BITS_7 == u32Bits)
			{
				dcbSerialParams.ByteSize = 7;
			}
			else
			if (SERIAL_LINE_DATA_BITS_6 == u32Bits)
			{
				dcbSerialParams.ByteSize = 6;
			}
			else
			if (SERIAL_LINE_DATA_BITS_5 == u32Bits)
			{
				dcbSerialParams.ByteSize = 5;
			}
			else
			{
				eResult = GC_SERIAL_DATA_BITS_INVALID;
				goto errorExit;
			}

			// Parity
			if (SERIAL_LINE_PARITY_NONE == u32Parity)
			{
				dcbSerialParams.Parity = NOPARITY;
			}
			else
			if (SERIAL_LINE_PARITY_EVEN == u32Parity)
			{
				dcbSerialParams.Parity = EVENPARITY;
			}
			else
			if (SERIAL_LINE_PARITY_ODD == u32Parity)
			{
				dcbSerialParams.Parity = ODDPARITY;
			}
			else
			{
				eResult = GC_SERIAL_PARITY_INVALID;
				goto errorExit;
			}

			// Stop bits
			if (SERIAL_LINE_STOP_BITS_1 == u32StopBits)
			{
				dcbSerialParams.StopBits = ONESTOPBIT;
			}
			else
			if (SERIAL_LINE_STOP_BITS_2 == u32StopBits)
			{
				dcbSerialParams.StopBits = TWOSTOPBITS;
			}
			else
			{
				eResult = GC_SERIAL_STOP_BITS_INVALID;
				goto errorExit;
			}

			if (SetCommState(hSerial,
							 &dcbSerialParams))
			{
				// Successfully set
#ifdef WATCH_SERIAL
				DebugOut("%s: UART=%u, u32ControlLines=%.8x, u32ControlStates=%.8x\n", __FUNCTION__, u32Index, u32ControlLines, u32ControlStates);
#endif
			}
			else
			{
				eResult = GC_SERIAL_PORT_HAL_ERROR;
			}
		}
		else
		{
			eResult = GC_SERIAL_PORT_HAL_ERROR;
		}
	}

errorExit:
	return(eResult);
}

EGCResultCode GCSerialSetLine(UINT32 u32Index,
							  UINT32 u32LineSettings)
{
	if (u32Index >= sg_u32SerialPortCount)
	{
		return(GC_SERIAL_PORT_OUT_OF_RANGE);
	}

	DebugOut("GCSerialSetLine: UART=%u, u32LineSettings=%.8x\n", u32Index, u32LineSettings);

	return(GC_OK);
}

EGCResultCode GCSerialGetDataCountAvailable(UINT32 u32Index,
											UINT32 *pu32RXDataCount,
											UINT32 *pu32TXDataCount)
{
	if (pu32RXDataCount)
	{
		*pu32RXDataCount = 0;
	}

	if (pu32TXDataCount)
	{
		*pu32TXDataCount = 0;
	}

	return(GC_OK);
}

#define	MAX_GPIO_COUNT		32

EGCResultCode GCGPIOSetState(UINT32 u32GPIONumber,
							 BOOL bState)
{
	if (u32GPIONumber >= MAX_GPIO_COUNT)
	{
		return(GC_GPIO_INDEX_INVALID);
	}

	DebugOut("%s: GPIO %u set %u\n", __FUNCTION__, u32GPIONumber, bState);

	return(GC_OK);
}

EGCResultCode GCGPIOGetState(UINT32 u32GPIONumber,
							 BOOL *pbState,
							 BOOL *pbStatePins)
{
	if (u32GPIONumber >= MAX_GPIO_COUNT)
	{
		return(GC_GPIO_INDEX_INVALID);
	}

	if (pbState)
	{
		*pbState = FALSE;
	}

	if (pbStatePins)
	{
		*pbStatePins = FALSE;
	}

	return(GC_OK);
}

EGCResultCode GCGPIOSetConfig(UINT32 u32GPIONumber,
							  UINT32 u32ConfigBits)
{
	if (u32GPIONumber >= MAX_GPIO_COUNT)
	{
		return(GC_GPIO_INDEX_INVALID);
	}

	if ((~GPIO_CFG_MASK) & u32ConfigBits)
	{
		return(GC_GPIO_CFG_FIELD_INVALID);
	}

	return(GC_OK);
}

EGCResultCode GCSetLogo(UINT16 *pu16Pixels,
						UINT32 u32XSize,
						UINT32 u32YSize,
						UINT32 u32Pitch)
{
	DebugOut("%s: Set new splash screen image that's %ux%u\n", __FUNCTION__, u32XSize, u32YSize);
	return(GC_OK);
}

BOOL GCIsLogoProgrammed(void)
{
	return(TRUE);
}

EGCResultCode GCMinimizeApplication( void )
{
	return( HostMinimizeApplication() );
}


typedef struct
{
	SDLKey eSDLKey;
	EGCCtrlKey eGCKey;
} SGCSDLKeyMap;

// Map SDL keys to EGCCtrlKey
SGCSDLKeyMap sg_sGCKeyMap[] = {
	{ SDLK_KP0, EGCKey_Keypad0 },
	{ SDLK_KP1, EGCKey_Keypad1 },
	{ SDLK_KP2, EGCKey_Keypad2 },
	{ SDLK_KP3, EGCKey_Keypad3 },
	{ SDLK_KP4, EGCKey_Keypad4 },
	{ SDLK_KP5, EGCKey_Keypad5 },
	{ SDLK_KP6, EGCKey_Keypad6 },
	{ SDLK_KP7, EGCKey_Keypad7 },
	{ SDLK_KP8, EGCKey_Keypad8 },
	{ SDLK_KP9, EGCKey_Keypad9 },
	{ SDLK_KP_PERIOD, EGCKey_KeypadPeriod },

	/* Arrows + Home/End pad */
	{ SDLK_UP, EGCKey_Up },
	{ SDLK_DOWN, EGCKey_Down },
	{ SDLK_RIGHT, EGCKey_Right },
	{ SDLK_LEFT, EGCKey_Left },
	{ SDLK_INSERT, EGCKey_Insert },
	{ SDLK_DELETE, EGCKey_Delete },
	{ SDLK_BACKSPACE, EGCKey_Backspace },
	{ SDLK_HOME, EGCKey_Home },
	{ SDLK_END, EGCKey_End },
	{ SDLK_PAGEUP, EGCKey_PageUp },
	{ SDLK_PAGEDOWN, EGCKey_PageDown },

	/* Function keys */
	{ SDLK_F1, EGCKey_F1 },
	{ SDLK_F2, EGCKey_F2 },
	{ SDLK_F3, EGCKey_F3 },
	{ SDLK_F4, EGCKey_F4 },
	{ SDLK_F5, EGCKey_F5 },
	{ SDLK_F6, EGCKey_F6 },
	{ SDLK_F7, EGCKey_F7 },
	{ SDLK_F8, EGCKey_F8 },
	{ SDLK_F9, EGCKey_F9 },
	{ SDLK_F10, EGCKey_F10 },
	{ SDLK_F11, EGCKey_F11 },
	{ SDLK_F12, EGCKey_F12 },
	{ SDLK_F13, EGCKey_F13 },
	{ SDLK_F14, EGCKey_F14 },
	{ SDLK_F15, EGCKey_F15 },

	{ SDLK_ESCAPE, EGCKey_Escape },
	{ SDLK_TAB, EGCKey_Tab },
	{ SDLK_KP_ENTER, EGCKey_Enter },
	{ SDLK_RETURN, EGCKey_Enter },

	/* Key state modifier keys */
	{ SDLK_NUMLOCK, EGCKey_Numlock },
	{ SDLK_CAPSLOCK, EGCKey_Capslock },
	{ SDLK_SCROLLOCK, EGCKey_Scrolllock },
	{ SDLK_RSHIFT, EGCKey_RShift },
	{ SDLK_LSHIFT, EGCKey_LShift },
	{ SDLK_RCTRL, EGCKey_RControl },
	{ SDLK_LCTRL, EGCKey_LControl },
	{ SDLK_RALT, EGCKey_RAlt },
	{ SDLK_LALT, EGCKey_LAlt },
	{ SDLK_RMETA, EGCKey_RMeta },
	{ SDLK_LMETA, EGCKey_LMeta },
	{ SDLK_RSUPER, EGCKey_RSuper },
	{ SDLK_LSUPER, EGCKey_LSuper },
	{ SDLK_MODE, EGCKey_Mode },
	{ SDLK_COMPOSE, EGCKey_Compose },

	/* Miscellaneous function keys */
	{ SDLK_BACKQUOTE, EGCKey_Backquote },	// FnLock on some laptops
	{ SDLK_HELP, EGCKey_Help },
	{ SDLK_PRINT, EGCKey_Print },
	{ SDLK_SYSREQ, EGCKey_Sysreq },
	{ SDLK_BREAK, EGCKey_Break },
	{ SDLK_MENU, EGCKey_Menu },
	{ SDLK_POWER, EGCKey_Power },
	{ SDLK_UNDO, EGCKey_Undo },

	{ SDLK_LAST, EGCKey_Unknown },	// terminator.
};


EGCCtrlKey GCConvertSDLKey(SDLKey eSDLKey)
{
	UINT16 u16Index = 0;
	EGCCtrlKey eGCKey = EGCKey_Unknown;

	while(1)
	{
		if( SDLK_LAST == sg_sGCKeyMap[u16Index].eSDLKey )
		{
			break;
		}
		else if( eSDLKey == sg_sGCKeyMap[u16Index].eSDLKey )
		{
			eGCKey = sg_sGCKeyMap[u16Index].eGCKey;
		}

		u16Index++;
	}

	return(eGCKey);
}


static BOOL GotHostKeyCallback(SDL_keysym sKeySymbol, BOOL bPressed)
{
	BOOL bHandledSuccessfully = FALSE;
	EGCCtrlKey eGCKey = EGCKey_Unknown;

	if( sg_pfKeyCallbackHandler != NULL )
	{
		// If the unicode value is zero then translate the control key
		//	(special case for backspace *8*)
		if( (0 == sKeySymbol.unicode) ||
			(8 == sKeySymbol.unicode) )
		{
			// Convert this SDLKey to a EGCKey
			eGCKey = GCConvertSDLKey(sKeySymbol.sym);

			if( EGCKey_Unknown == eGCKey )
			{
				// If we failed on a key up, then just ignore it.  We don't get unicode values on keyup.
				if( FALSE == bPressed )
				{
					return(TRUE);
				}
				// If we failed to convert the key on a keydown, we're missing a keymap...
				else
				{
					// Uncomment for keymap debugging
					//GCASSERT(0);
				}
			}
			
		}
		else
		{
			eGCKey = EGCKey_Unicode;
		}

		bHandledSuccessfully = sg_pfKeyCallbackHandler(eGCKey, sKeySymbol.unicode, bPressed);;
	}

	return(bHandledSuccessfully);
}

// Wrapper for scancode callback allows type checking
void GCSetKeyCallback(BOOL (*pfHandler)(EGCCtrlKey, LEX_CHAR, BOOL))
{
	if( pfHandler )
	{
		sg_pfKeyCallbackHandler = pfHandler;
		SetKeyCallback(GotHostKeyCallback);
	}
	else
	{
		sg_pfKeyCallbackHandler = NULL;
		SetKeyCallback(NULL);
	}
}

void HostSetMousewheel(UINT32 u32Value)
{
	if (sg_pMousewheelCallback)
	{
		sg_pMousewheelCallback(u32Value);
	}
}

void GCSetMousewheelCallback(void (*Callback)(UINT32 u32Value))
{
	sg_pMousewheelCallback = Callback;
}

void GCSetReturnCode( UINT32 u32ReturnCode )
{
	sg_u32ReturnCode = u32ReturnCode;
}


/* ************************************************************************* *\
** ************************************************************************* **
** EOF
** ************************************************************************* **
\* ************************************************************************* */
