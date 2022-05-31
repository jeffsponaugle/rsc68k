#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "Startup/app.h"
#include "Application/RSC68k.h"

#define	MAX_FILE	128
static SFile *sg_psFileList[MAX_FILE];

/****************************************************************************
						DISK/SD File access routines
 ****************************************************************************/

static LEX_CHAR *sg_peCurrentDirectory;

static ELCDErr SDOpen(struct SFile *psFile, 
					  LEX_CHAR *peDeviceName,
					  LEX_CHAR *peFilename, 
					  LEX_CHAR *peFileMode)
{
	GCFile eSDFileHandle;
	EGCResultCode eResult;
	ELCDErr eErr = LERR_OK;
	LEX_CHAR *peCombinedPath = NULL;
	char *pu8FileMode = LexUnicodeToASCIIAlloc(peFileMode);

	if (NULL == pu8FileMode)
	{
		return(LERR_NO_MEM);
	}

	peCombinedPath = Lexstrdup(peFilename);

	// If nothing was done to the path, then open it as-is
	if (NULL == peCombinedPath)
	{
		char *pu8Filename;

		pu8Filename = LexUnicodeToASCIIAlloc(peFilename);
		if (NULL == pu8Filename)
		{
			GCFreeMemory(pu8FileMode);
			return(LERR_NO_MEM);
		}

		eResult = GCFileOpen(&eSDFileHandle, (const char *) pu8Filename, (const char *) pu8FileMode);
		GCFreeMemory(pu8Filename);
	}
	else
	{
		char *pu8Filename;

		pu8Filename = LexUnicodeToASCIIAlloc(peCombinedPath);
		if (NULL == pu8Filename)
		{
			GCFreeMemory(pu8FileMode);
			GCFreeMemory(peCombinedPath);
			return(LERR_NO_MEM);
		}

		eResult = GCFileOpen(&eSDFileHandle, (const char *) pu8Filename, (const char *) pu8FileMode);
		GCFreeMemory(peCombinedPath);
		GCFreeMemory(pu8Filename);
	}

	GCFreeMemory(pu8FileMode);

	if (GC_OK == eResult)
	{
		// Good deal! The file is open now. Let's allocate some space in the union
		// for file specific information.

		psFile->uFileInfo.psFileInfo = MemAlloc(sizeof(*psFile->uFileInfo.psFileInfo));
		if (NULL == psFile->uFileInfo.psFileInfo)
		{
			// Out of memory. Damn!
			eResult = GCFileClose(&eSDFileHandle);
			return((ELCDErr) (eResult + LERR_GC_ERR_BASE));
		}

		// Store our file pointer away for later
		psFile->uFileInfo.psFileInfo->eFileHandle = eSDFileHandle;

		// Got it!
		return(LERR_OK);
	}
	else
	{
		// Convert the base GCXXX error codes to LERR code.
		return((ELCDErr) (eResult + LERR_GC_ERR_BASE));
	}
}

static ELCDErr SDRead(struct SFile *psFile, 
					  void *pvData, 
					  UINT32 u32ByteCount,
					  UINT32 *pu32BytesRead)
{
	EGCResultCode eResult;

	// Reading 0 bytes?
	if (0 == u32ByteCount)
	{
		*pu32BytesRead = 0;
		return(LERR_OK);
	}

	// Let's go read some data
	eResult = GCFileRead(pvData, u32ByteCount, pu32BytesRead, psFile->uFileInfo.psFileInfo->eFileHandle);
	return((ELCDErr) (eResult + LERR_GC_ERR_BASE));
}

static ELCDErr SDWrite(struct SFile *psFile, 
					   void *pvData, 
					   UINT32 u32ByteCount,
					   UINT32 *pu32BytesWritten)
{
	EGCResultCode eResult;

	// We can do 0 byte writes - don't check for it. That causes truncation should
	// the file be seeked to a particular location in r/w mode.

	// Let's go read some data
	eResult = GCFileWrite(pvData, u32ByteCount, pu32BytesWritten, psFile->uFileInfo.psFileInfo->eFileHandle);
	return((ELCDErr) (eResult + LERR_GC_ERR_BASE));
}

static ELCDErr SDSeek(struct SFile *psFile, 
					  UINT64 u64Offset, 
					  EFileOrigin eOrigin)
{
	INT32 s32Whence = (INT32) (eOrigin);
	EGCResultCode eResult;

	eResult = GCFileSeek(psFile->uFileInfo.psFileInfo->eFileHandle, (off_t) u64Offset, s32Whence);
	return((ELCDErr) (eResult + LERR_GC_ERR_BASE));
}

static ELCDErr SDClose(struct SFile *psFile)
{
	EGCResultCode eResult;

	if (psFile->uFileInfo.psFileInfo)
	{
		eResult = GCFileClose(&psFile->uFileInfo.psFileInfo->eFileHandle);

		GCFreeMemory(psFile->uFileInfo.psFileInfo);
		psFile->uFileInfo.psFileInfo = NULL;
		return((ELCDErr) (eResult + LERR_GC_ERR_BASE));
	}
	else
	{
		return(LERR_OK);
	}
}

static ELCDErr SDFindOpen(struct SFile *psFile, 
						  LEX_CHAR *peDeviceName,
						  LEX_CHAR *pePatternIncoming,
						  LEX_CHAR *peWhatToLookFor)
{
	ELCDErr eErr = LERR_OK;
	LEX_CHAR *pePattern = NULL;

/*	eErr = RelativeToAbsolutePath(sg_peCurrentDirectory,
								  pePatternIncoming,
								  &pePattern,
								  FALSE);

	if (eErr != LERR_OK)
	{
		return(eErr);
	} */

	pePattern = Lexstrdup(peDeviceName);

	// Gotta create ourselves a file find structure
	psFile->uFileInfo.psFileFind = MemAlloc(sizeof(*psFile->uFileInfo.psFileFind));

	if (NULL == psFile->uFileInfo.psFileFind)
	{
		// Out of memory
		eErr = LERR_NO_MEM;
		goto errorExit;
	}

	// Now, let's whip through the "what to look for" string and see what we can
	// find.

	while (peWhatToLookFor && *peWhatToLookFor)
	{
		if ((*peWhatToLookFor == 'r') || (*peWhatToLookFor == 'R'))
		{
			// Read only!
			psFile->uFileInfo.psFileFind->u32DesiredAttributeMask |= FILEATTR_ARDONLY;
		}
		else
		if ((*peWhatToLookFor == 'h') || (*peWhatToLookFor == 'H'))
		{
			// Hidden!
			psFile->uFileInfo.psFileFind->u32DesiredAttributeMask |= FILEATTR_AHIDDEN;
		}
		else
		if ((*peWhatToLookFor == 's') || (*peWhatToLookFor == 'S'))
		{
			// System!
			psFile->uFileInfo.psFileFind->u32DesiredAttributeMask |= FILEATTR_ASYSTEM;
		}
		else
		if ((*peWhatToLookFor == 'v') || (*peWhatToLookFor == 'V'))
		{
			// Volume!
			psFile->uFileInfo.psFileFind->u32DesiredAttributeMask |= FILEATTR_AVOLID;
		}
		else
		if ((*peWhatToLookFor == 'd') || (*peWhatToLookFor == 'D'))
		{
			// Directories!
			psFile->uFileInfo.psFileFind->u32DesiredAttributeMask |= FILEATTR_ADIR;
		}
		else
		if ((*peWhatToLookFor == 'a') || (*peWhatToLookFor == 'A'))
		{
			// Archive attribute!
			psFile->uFileInfo.psFileFind->u32DesiredAttributeMask |= FILEATTR_AARCHIV;
		}
		else
		if ((*peWhatToLookFor == 'n') || (*peWhatToLookFor == 'N'))
		{
			// Normal attribute!
			psFile->uFileInfo.psFileFind->u32DesiredAttributeMask |= FILEATTR_ANORMAL;
		}
		else
		if ((*peWhatToLookFor == ' ') || (*peWhatToLookFor == '\t'))
		{
			// Whitespace! Just ignore it
		}
		else
		{
			// Invalid field.
			eErr = LERR_FILE_FIND_INVALID_FIELD;
			goto errorExit;
		}
		++peWhatToLookFor;
	}

	// If nothing was done to the path, then use it as-is
	if (NULL == pePattern)
	{
		char *pu8Filename;

		pu8Filename = LexUnicodeToASCIIAlloc(pePatternIncoming);
		if (NULL == pu8Filename)
		{
			return(LERR_NO_MEM);
		}

		psFile->uFileInfo.psFileFind->psFileFind = GCFileFindSetup((const char *) pu8Filename);
		GCFreeMemory(pu8Filename);
	}
	else
	{
		char *pu8Filename;

		pu8Filename = LexUnicodeToASCIIAlloc(pePattern);
		if (NULL == pu8Filename)
		{
			GCFreeMemory(pePattern);
			return(LERR_NO_MEM);
		}

		psFile->uFileInfo.psFileFind->psFileFind = GCFileFindSetup((const char *) pu8Filename);
		GCFreeMemory(pu8Filename);
	}

	if (NULL == psFile->uFileInfo.psFileFind->psFileFind)
	{
		eErr = LERR_FILE_FIND_END;
	}

errorExit:
	GCFreeMemory(pePattern);

	if (eErr != LERR_OK)
	{
		if (psFile->uFileInfo.psFileFind)
		{
			GCFreeMemory(psFile->uFileInfo.psFileFind);
			psFile->uFileInfo.psFileFind = NULL;
		}
	}

	return(eErr);
}

static ELCDErr SDFindClose(struct SFile *psFile)
{
	if (psFile->uFileInfo.psFileFind)
	{
		if (psFile->uFileInfo.psFileFind->psFileFind)
		{
			GCFileFindEnd(psFile->uFileInfo.psFileFind->psFileFind);
		}

		GCFreeMemory(psFile->uFileInfo.psFileFind);
		psFile->uFileInfo.psFileFind = NULL;
	}

	return(LERR_OK);
}

static ELCDErr SDFindNext(struct SFile *psFile, 
						  LEX_CHAR *peFilename, 
						  UINT32 *pu32Attributes,
						  UINT32 u32MaxLength)

{
	EGCResultCode eResult;
	UINT32 u32Attribute = 0;
	char pu8ReturnedFilename[512];

	GCASSERT(pu32Attributes);
	GCASSERT(u32MaxLength < sizeof(pu8ReturnedFilename));

	while (1)
	{
		// Find the next thing
		eResult = GCFileFindNext(psFile->uFileInfo.psFileFind->psFileFind,
								 (char *) pu8ReturnedFilename,
								 u32MaxLength);

		if (eResult != GC_OK)
		{
			return(LERR_FILE_FIND_END);
		}

		// Convert the ASCII response to unicode
		LexASCIIToUnicode(pu8ReturnedFilename, peFilename);


		// We've found one. Let's see if this is the proper record and
		// it matches our attribute

		eResult = GCFileFindGetAttribute(psFile->uFileInfo.psFileFind->psFileFind,
										 &u32Attribute,
										 NULL,
										 NULL);

		if (eResult != GC_OK)
		{
			return(LERR_FILE_FIND_END);
		}

		if (0 == u32Attribute)
		{
			u32Attribute |= FILEATTR_ANORMAL;
		}

		// Check to see if the remaining requested attributes are set
		if (u32Attribute & (psFile->uFileInfo.psFileFind->u32DesiredAttributeMask))
		{
			// Got it!
			*pu32Attributes = u32Attribute;
			return(LERR_OK);
		}

		// Otherwise, keep looking
	}
}

static ELCDErr SDFileSize(struct SFile *psFile, 
						  UINT64 *pu64Attributes)

{
	EGCResultCode eResult;

	eResult = GCFileSizeByHandle(psFile->uFileInfo.psFileInfo->eFileHandle, pu64Attributes);
	return((ELCDErr) (eResult + LERR_GC_ERR_BASE));
}

static ELCDErr SDFileEOF(struct SFile *psFile, 
						 BOOL *pbEOF)

{
	EGCResultCode eResult;

	eResult = GCFileEOF(psFile->uFileInfo.psFileInfo->eFileHandle, pbEOF);
	return((ELCDErr) (eResult + LERR_GC_ERR_BASE));
}
	
static ELCDErr SDFilePos(struct SFile *psFile, 
						 UINT64 *pu64FilePos)

{
	EGCResultCode eResult;

	eResult = GCFileTell((off_t *) pu64FilePos, psFile->uFileInfo.psFileInfo->eFileHandle);
	return((ELCDErr) (eResult + LERR_GC_ERR_BASE));
}

static ELCDErr SDChdir(const SFileAccess *psMethod,
					   LEX_CHAR *peNewDirectory)
{
	UINT32 u32FinalLength = 0;
	EGCResultCode eErr = LERR_OK;
	LEX_CHAR *peNewCurrent = NULL;

	eErr = RelativeToAbsolutePath(sg_peCurrentDirectory,
								  peNewDirectory,
								  &peNewCurrent,
								  TRUE);

	if (LERR_OK == eErr)
	{
		LEX_CHAR *peOldCurrent = sg_peCurrentDirectory;

		if (NULL == peNewCurrent)
		{
			// Just try going directly to it
			if (GCIsDir((char *) peNewDirectory) == GC_OK)
			{
				sg_peCurrentDirectory = Lexstrdup(peNewDirectory);
				if (NULL == sg_peCurrentDirectory)
				{
					sg_peCurrentDirectory = peOldCurrent;
					eErr = LERR_NO_MEM;
				}
				else
				{
					GCFreeMemory(peOldCurrent);
				}
			}
			else
			{
				eErr = LERR_ENOENT;
			}
		}
		else
		{
			// Check to see if the absolute path is present or not
			if (GCIsDir((char *) peNewCurrent) == GC_OK)
			{
				// Now try opening up the directory. If it's not found by the OS, then bail.
				sg_peCurrentDirectory = peNewCurrent;
				GCFreeMemory(peOldCurrent);
			}
			else
			{
				// Not good. Free it.
				GCFreeMemory(peNewCurrent);
				eErr = LERR_ENOENT;
			}
		}
	}

	return(eErr);
}

static ELCDErr SDMkdir(const SFileAccess *psMethod,
					   LEX_CHAR *peDirectory)
{
	ELCDErr eErr;
	LEX_CHAR *peNewCurrent;

	eErr = RelativeToAbsolutePath(sg_peCurrentDirectory,
								  peDirectory,
								  &peNewCurrent,
								  FALSE);

	if (eErr != LERR_OK)
	{
		return(eErr);
	}

	if (NULL == peNewCurrent)
	{
		eErr = (ELCDErr) (GCmkdir((const char *) peDirectory) + LERR_GC_ERR_BASE);
	}
	else
	{
		eErr = (ELCDErr) (GCmkdir((const char *) peNewCurrent) + LERR_GC_ERR_BASE);
		GCFreeMemory(peNewCurrent);
	}
	return(eErr);
}

static ELCDErr SDRmdir(const SFileAccess *psMethod,
					   LEX_CHAR *peDirectory)
{
	ELCDErr eErr;
	LEX_CHAR *peNewCurrent;

	eErr = RelativeToAbsolutePath(sg_peCurrentDirectory,
								  peDirectory,
								  &peNewCurrent,
								  FALSE);

	if (eErr != LERR_OK)
	{
		return(eErr);
	}

	// 
	if (NULL == peNewCurrent)
	{
		eErr = (ELCDErr) (GCrmdir((const char *) peDirectory) + LERR_GC_ERR_BASE);
	}
	else
	{
		eErr = (ELCDErr) (GCrmdir((const char *) peNewCurrent) + LERR_GC_ERR_BASE);
		GCFreeMemory(peNewCurrent);
	}
	return(eErr);
}

static ELCDErr SDDelete(LEX_CHAR *peNewFileToDelete)
{
	ELCDErr eErr;
	LEX_CHAR *peNewCurrent;

	eErr = RelativeToAbsolutePath(sg_peCurrentDirectory,
								  peNewFileToDelete,
								  &peNewCurrent,
								  FALSE);

	if (eErr != LERR_OK)
	{
		return(eErr);
	}

	if (NULL == peNewCurrent)
	{
		eErr = (ELCDErr) (GCDeleteFile((const char *) peNewFileToDelete) + LERR_GC_ERR_BASE);
	}
	else
	{
		eErr = (ELCDErr) (GCDeleteFile((const char *) peNewCurrent) + LERR_GC_ERR_BASE);
		GCFreeMemory(peNewCurrent);
	}

	return(eErr);
}

static ELCDErr SDOneTimeInit(const SFileMethods *psMethod)
{
	// Always OK
	DebugOut("SDOneTimeInit: Initialized\n");
	if (NULL == sg_peCurrentDirectory)
	{
		sg_peCurrentDirectory = GCGetCurrentDirectory();
		GCASSERT(sg_peCurrentDirectory);
		GCASSERT(Lexstrlen(sg_peCurrentDirectory) > 0);
	}
	return(LERR_OK);
}

static ELCDErr SDFlush(struct SFile *psFile)
{
	return((ELCDErr) (GCFileSystemSync() + LERR_GC_ERR_BASE));
}

// Pointers to file functions
const SFileAccess sg_sSDAccess = 
{
	SDOneTimeInit,			// OneTimeInit
	SDOpen,					// Open
	SDRead,					// Read
	SDWrite,				// Write
	SDSeek,					// Seek
	SDClose,				// Close
	NULL,					// FindNext
	SDFileSize,				// FileSize
	SDFileEOF,				// FileEOF
	SDFilePos,				// FilePos
	NULL,					// FileStatus
	NULL,					// FileSetTimeout
	SDFlush,				// FileFlush
	SDChdir,				// FileChdir
	SDRmdir,				// FileRmdir
	SDMkdir,				// FileMkdir
	SDDelete,				// FileDelete
};

const SFileAccess sg_sSDFindFirst = 
{
	NULL,					// OneTimeInit
	SDFindOpen,				// Open
	NULL,					// Read
	NULL,					// Write
	NULL,					// Seek
	SDFindClose,			// Close
	SDFindNext,				// FindNext
	NULL,					// FileSize
	NULL,					// FileEOF
	NULL,					// FilePos
	NULL,					// FileStatus
	NULL,					// FileSetTimeout
	NULL,					// FileFlush
	NULL,					// FileChdir
	NULL,					// FileRmdir
	NULL,					// FileMkdir
	NULL					// FileDelete
};


UINT32 FileConvertTimeoutToOS(SFile *psFile)
{
	UINT32 u32Timeout;

	u32Timeout = psFile->u32Timeout;
	if (FILE_WAIT_FOREVER == psFile->u32Timeout)
	{
		u32Timeout = 0;
	}
	else
	if (0 == psFile->u32Timeout)
	{
		// 1 Clock tick to wait
		u32Timeout = 1;
	}

	return(u32Timeout);
}

static BOOL FileFindFreeHandle(FILEHANDLE *peFileHandle)
{
	UINT32 u32Loop;

	for (u32Loop = 0; u32Loop < (sizeof(sg_psFileList) / sizeof(sg_psFileList[0])); u32Loop++)
	{
		if (NULL == sg_psFileList[u32Loop])
		{
			*peFileHandle = (FILEHANDLE) u32Loop;
			return(TRUE);
		}
	}

	return(FALSE);
}

SFile *FileGetPointer(FILEHANDLE eHandle)
{
	if (eHandle >= (sizeof(sg_psFileList) / sizeof(sg_psFileList[0])))
	{
		return(NULL);
	}

	return(sg_psFileList[eHandle]);
}

BOOL FileIsArchive(LEX_CHAR *peFilename)
{
	// Find the first non-space character
	while (*peFilename && 
			(('\n' == *peFilename) ||
			 ('\r' == *peFilename) ||
			 ('\t' == *peFilename) ||
			 (' ' == *peFilename)))
	{
		++peFilename;
	}

	// Found the first non-whitespace character. Let's see if it matches with
	// archive.

	if (Lexstrnicmp(peFilename, "archive", 7))
	{
		// It's not "archive"
		return(FALSE);
	}

	// It's archive. Let's see if there's a : after it

	peFilename += 7;	// Skip past "archive"
	while (*peFilename && 
			(('\n' == *peFilename) ||
			 ('\r' == *peFilename) ||
			 ('\t' == *peFilename) ||
			 (' ' == *peFilename)))
	{
		++peFilename;
	}

	// Is this character a ":"?
	if (':' == *peFilename)
	{
		return(TRUE);
	}
	else
	{
		return(FALSE);
	}
}

static const SFileMethods sg_sFileMethods[] =
{
	{NULL,			&sg_sSDAccess,				FALSE},	// SD filesystem (normal files)
	{NULL,			&sg_sSDFindFirst,			TRUE},	// SD filesystem (find first/next)
};

static const SFileAccess *FileGetFileProcs(LEX_CHAR *peFilename,
										   LEX_CHAR **ppeStartOfFilename,
										   LEX_CHAR **ppeStartOfDeviceName,
										   BOOL bFind)
{
	const SFileAccess *psAccess = NULL;
	UINT32 u32Loop;

	// If they've given us a legit device name, record it
	if (ppeStartOfDeviceName)
	{
		*ppeStartOfDeviceName = NULL;
	}

	// Eliminate prepended whitespace
	while ((' ' == *peFilename) ||
		   ('\t' == *peFilename))
	{
		++peFilename;
	}

	for (u32Loop = 0; u32Loop < (sizeof(sg_sFileMethods) / sizeof(sg_sFileMethods[0])); u32Loop++)
	{
		// If we're at NULL, then we're doing a default
		if (NULL == sg_sFileMethods[u32Loop].pePrefix)
		{
			// If bFind matches, then exit
			if (bFind == sg_sFileMethods[u32Loop].bFind)
			{
				psAccess = sg_sFileMethods[u32Loop].psFileAccess;
				if (ppeStartOfDeviceName)
				{
					*ppeStartOfDeviceName = peFilename;
				}

				peFilename += Lexstrlen(sg_sFileMethods[u32Loop].pePrefix);
				break;
			}
		}
		else
		{
			// Let's see if we have a device name matching the filename
			if ((Lexstristr(peFilename,
						    sg_sFileMethods[u32Loop].pePrefix)) &&
				(sg_sFileMethods[u32Loop].bFind == bFind))
			{
				// Found it!
				psAccess = sg_sFileMethods[u32Loop].psFileAccess;
				if (ppeStartOfDeviceName)
				{
					*ppeStartOfDeviceName = peFilename;
				}

				peFilename += Lexstrlen(sg_sFileMethods[u32Loop].pePrefix);
				break;
			}
		}
	}

	// Eliminate any whitespace after the device name
	while ((' ' == *peFilename) ||
		   ('\t' == *peFilename))
	{
		++peFilename;
	}

	// This should've gotten caught by the loop above
	GCASSERT(psAccess);

	// Record the start of the filename
	if (ppeStartOfFilename)
	{
		*ppeStartOfFilename = peFilename;
	}

	return(psAccess);
}

/****************************************************************************
						  Generic file I/O routines
 ****************************************************************************/

extern ELCDErr FileOpenInternal(FILEHANDLE *peFileHandle,
								LEX_CHAR *peFilename,	// Our file name
								LEX_CHAR *peFileMode,
								BOOL bFind)
{
	ELCDErr eErr = LERR_OK;
	const SFileAccess *psAccess;
	SFile *psFile = NULL;
	LEX_CHAR *peRealFilename = NULL;
	LEX_CHAR *peDeviceName = NULL;

	// Set the file handle as invalid to start with
	*peFileHandle = HANDLE_INVALID;

	// Go snag the access routines for this file type (whatever it may be)
	psAccess = FileGetFileProcs(peFilename,
								&peRealFilename,
								&peDeviceName,
								bFind);

	if (NULL == psAccess)
	{
		eErr = LERR_FILE_UNKNOWN_DEVICE_TYPE;
		goto errorExit;
	}

	// We need a file structure
	psFile = MemAlloc(sizeof(*psFile));
	if (NULL == psFile)
	{
		eErr = LERR_NO_MEM;
		goto errorExit;
	}

	// Plug in the appropriate access functions
	psFile->psAccessFunc = psAccess;

	// Now we need a free handle
	if (FileFindFreeHandle(peFileHandle))
	{
		// Got a handle!
	}
	else
	{
		eErr = LERR_FILE_FULL;
		goto errorExit;
	}

	// Now go try to open it up
	eErr = psAccess->Open(psFile,
						  peDeviceName,
						  peRealFilename,
						  peFileMode);

	if (LERR_OK == eErr)
	{
		// Plug in the file handle
		GCASSERT(NULL == sg_psFileList[*peFileHandle]);
		sg_psFileList[*peFileHandle] = psFile;
	}

errorExit:
	if (eErr != LERR_OK)
	{
		// Take the file handle out of the 
		if (*peFileHandle != HANDLE_INVALID)
		{
			sg_psFileList[*peFileHandle] = NULL;
		}

		// If we have the ability to close it, let's do so
		if (psAccess->Close)
		{
			(void) psAccess->Close(psFile);
		}

		// Now free up the file structure
		GCFreeMemory(psFile);
	}

	return(eErr);
}

ELCDErr FileOpen(FILEHANDLE *peFileHandle,
				 LEX_CHAR *peFilename,	// Our file name
				 LEX_CHAR *peFileMode)
{
	return(FileOpenInternal(peFileHandle,
							peFilename,
							peFileMode,
							FALSE));
}

ELCDErr FileFindOpen(FILEHANDLE *peFileHandle,
					 LEX_CHAR *pePattern,	// Our pattern
					 LEX_CHAR *peWhatToLookFor)
{
	return(FileOpenInternal(peFileHandle,
							pePattern,
							peWhatToLookFor,
							TRUE));
}

ELCDErr FileClose(FILEHANDLE *peFileHandle)
{
	SFile *psFile;
	ELCDErr eErr = LERR_OK;

	psFile = FileGetPointer(*peFileHandle);
	if (NULL == psFile)
	{
		return(LERR_FILE_BAD_HANDLE);
	}

	if (psFile->psAccessFunc->Close)
	{
		// We've got it! Let's close things.
		eErr = psFile->psAccessFunc->Close(psFile);
	}

	// Let's free the pointer
	GCFreeMemory(psFile);
	sg_psFileList[*peFileHandle] = NULL;
	*peFileHandle = HANDLE_INVALID;
	return(eErr);
}

ELCDErr FileRead(FILEHANDLE eFileHandle,
				 void *pvLocation,
				 UINT32 u32BytesToRead,
				 UINT32 *pu32BytesRead)
{
	SFile *psFile;
	ELCDErr eErr = LERR_OK;
	UINT8 *pu8Data = (UINT8 *) pvLocation;
	UINT32 u32DesiredData = u32BytesToRead;

	psFile = FileGetPointer(eFileHandle);
	if (NULL == psFile)
	{
		return(LERR_FILE_BAD_HANDLE);
	}

	if (NULL == psFile->psAccessFunc->Read)
	{
		// Can't do a read. Function not supported
		return(LERR_FILE_INVALID_ACTION);
	}

	// Zero out our count
	if (pu32BytesRead)
	{
		*pu32BytesRead = 0;
	}

	// First, let's suck some data out of the rewind stream if we can
	if (psFile->pu8RewindBuffer)
	{
		while (u32BytesToRead && psFile->u32RewoundAmount)
		{
			*pu8Data = psFile->pu8RewindBuffer[psFile->u32RewindTail++];
			if (psFile->u32RewindTail >= psFile->u32RewindSize)
			{
				psFile->u32RewindTail = 0;
			}

			psFile->u32RewoundAmount--;
			--u32BytesToRead;
			if (pu32BytesRead)
			{
				(*pu32BytesRead)++;
			}
			++pu8Data;
		}
	}

	// If we still have data to read, then go read it
	if (u32BytesToRead)
	{
		UINT32 u32BytesActuallyRead = 0;

		// Our rewind size here had better be zero before we go fetching
		// more data from the device itself.
		GCASSERT(0 == psFile->u32RewoundAmount);

		// These should also be equal if the code is doing its job
		GCASSERT(psFile->u32RewindHead == psFile->u32RewindTail);

		u32BytesActuallyRead = *pu32BytesRead;

		eErr = psFile->psAccessFunc->Read(psFile,
										  (void *) pu8Data,
										  u32BytesToRead,
										  pu32BytesRead);

		// How much data did we get?
		u32BytesActuallyRead = *pu32BytesRead - u32BytesActuallyRead;

		// This means we got some data. Let's suck down the last "n" number of
		// bytes we've just read and put it in the rewind buffer (if there is one)
		if ((LERR_OK == eErr) && (psFile->pu8RewindBuffer) && (u32BytesActuallyRead))
		{
			// Adjust to the end of our data
			pu8Data += u32BytesActuallyRead;

			// Now we're going to back up the size of the rewind buffer or the
			// # Of bytes requested (whichever is smaller)
			if (u32BytesActuallyRead > psFile->u32RewindSize)
			{
				u32BytesActuallyRead = psFile->u32RewindSize;
			}

			// Back up enough so we can copy data 
			pu8Data -= u32BytesActuallyRead;
			while (u32BytesActuallyRead)
			{
				psFile->pu8RewindBuffer[psFile->u32RewindHead++] = *pu8Data;
				psFile->u32RewindTail++;
				++pu8Data;
				if (psFile->u32RewindHead >= psFile->u32RewindSize)
				{
					psFile->u32RewindHead = 0;
				}
				if (psFile->u32RewindTail >= psFile->u32RewindSize)
				{
					psFile->u32RewindTail = 0;
				}

				--u32BytesActuallyRead;
			}
		}
	}

	// Check for incomplete reads in successful conditions
	if (LERR_OK == eErr)
	{
		if (*pu32BytesRead != u32DesiredData)
		{
			eErr = LERR_FILE_INCOMPLETE_READ;
		}
	}

	return(eErr);
}

ELCDErr FileWrite(FILEHANDLE eFileHandle,
				  void *pvLocation,
				  UINT32 u32BytesToWrite,
				  UINT32 *pu32BytesWrite)
{
	SFile *psFile;
	ELCDErr eErr = LERR_OK;

	psFile = FileGetPointer(eFileHandle);
	if (NULL == psFile)
	{
		return(LERR_FILE_BAD_HANDLE);
	}

	if (NULL == psFile->psAccessFunc->Write)
	{
		// Can't do a write. Function not supported
		return(LERR_FILE_INVALID_ACTION);
	}

	return(psFile->psAccessFunc->Write(psFile,
									   pvLocation,
									   u32BytesToWrite,
									   pu32BytesWrite));
}

ELCDErr FileSeek(FILEHANDLE eFileHandle,
				 UINT64 u64Offset,
				 EFileOrigin eOrigin)
{
	SFile *psFile;
	ELCDErr eErr = LERR_OK;

	psFile = FileGetPointer(eFileHandle);
	if (NULL == psFile)
	{
		return(LERR_FILE_BAD_HANDLE);
	}

	if (NULL == psFile->psAccessFunc->Seek)
	{
		// Can't do a write. Function not supported
		return(LERR_FILE_INVALID_ACTION);
	}

	// If we have an invalid origin, kick back an error
	if ((eOrigin != FILESEEK_SET) &&
		(eOrigin != FILESEEK_CUR) &&
		(eOrigin != FILESEEK_END))
	{
		return(LERR_FILE_SEEK_PARAM_INVALID);
	}

	return(psFile->psAccessFunc->Seek(psFile,
									  u64Offset,
									  eOrigin));
}

ELCDErr FileFind(FILEHANDLE eFileHandle,
				 LEX_CHAR *peFilenameStorageArea,
				 UINT32 *pu32Attributes,
				 UINT32 u32MaxLength)
{
	SFile *psFile;
	ELCDErr eErr = LERR_OK;

	psFile = FileGetPointer(eFileHandle);
	if (NULL == psFile)
	{
		return(LERR_FILE_BAD_HANDLE);
	}

	if (NULL == psFile->psAccessFunc->FindNext)
	{
		// Can't do a write. Function not supported
		return(LERR_FILE_INVALID_ACTION);
	}

	return(psFile->psAccessFunc->FindNext(psFile,
										  peFilenameStorageArea,
										  pu32Attributes,
										  u32MaxLength));
}

ELCDErr FileSize(FILEHANDLE eFileHandle,
				 LEX_CHAR *peFileName,
				 UINT64 *pu64FileSize)
{
	SFile *psFile;
	ELCDErr eErr = LERR_OK;
	EGCResultCode eResult;

	if (peFileName)
	{
		eResult = GCFileSizeByFilename((const char *) peFileName, pu64FileSize);
		eErr = (ELCDErr) (LERR_GC_ERR_BASE + eResult);
	}
	else
	{
		psFile = FileGetPointer(eFileHandle);
		if (NULL == psFile)
		{
			return(LERR_FILE_BAD_HANDLE);
		}

		if (NULL == psFile->psAccessFunc->FileSize)
		{
			// Can't do a file size. Function not supported
			return(LERR_FILE_INVALID_ACTION);
		}

		eErr = psFile->psAccessFunc->FileSize(psFile,
											  pu64FileSize);
	}

	return(eErr);
}

ELCDErr FileEOF(FILEHANDLE eFileHandle,
				BOOL *pbFileEOF)
{
	SFile *psFile;
	ELCDErr eErr = LERR_OK;

	psFile = FileGetPointer(eFileHandle);
	if (NULL == psFile)
	{
		return(LERR_FILE_BAD_HANDLE);
	}

	if (NULL == psFile->psAccessFunc->FileSize)
	{
		// Can't do a file eof. Function not supported
		return(LERR_FILE_INVALID_ACTION);
	}

	// If we have a rewind characters, we know it's not done, so just return
	if (psFile->u32RewoundAmount)
	{
		*pbFileEOF = FALSE;
		return(LERR_OK);
	}

	eErr = psFile->psAccessFunc->FileEOF(psFile,
										 pbFileEOF);

	return(eErr);
}

ELCDErr FilePos(FILEHANDLE eFileHandle,
				UINT64 *pu64FileSize)
{
	SFile *psFile;
	ELCDErr eErr = LERR_OK;

	psFile = FileGetPointer(eFileHandle);
	if (NULL == psFile)
	{
		return(LERR_FILE_BAD_HANDLE);
	}

	if (NULL == psFile->psAccessFunc->FilePos)
	{
		// Can't do a file position. Function not supported
		return(LERR_FILE_INVALID_ACTION);
	}

	eErr = psFile->psAccessFunc->FilePos(psFile,
										 pu64FileSize);

	return(eErr);
}

ELCDErr FileRewind(FILEHANDLE eFileHandle,
				   UINT64 u64RewindAmount)
{
	SFile *psFile;
	ELCDErr eErr = LERR_OK;
	UINT64 u64Position;

	psFile = FileGetPointer(eFileHandle);
	if (NULL == psFile)
	{
		return(LERR_FILE_BAD_HANDLE);
	}

	if ((NULL == psFile->psAccessFunc->FilePos) ||
		(NULL == psFile->psAccessFunc->Seek))
	{
		// Doesn't support seeking or changing the position of the file. Let's use our
		// rewind buffer.

		if (((UINT64) psFile->u32RewoundAmount + u64RewindAmount) > psFile->u32RewindSize)
		{
			// Can't rewind that far
			return(LERR_FILE_REWIND_INVALID);
		}

		// We can rewind!
		psFile->u32RewoundAmount += (UINT32) u64RewindAmount;
		if (psFile->u32RewindTail < (UINT32) u64RewindAmount)
		{
			// We're wrapping
			psFile->u32RewindTail = (psFile->u32RewindTail - (UINT32) u64RewindAmount) + psFile->u32RewindSize;
		}
		else
		{
			// We're not wrapping
			psFile->u32RewindTail -= (UINT32) u64RewindAmount;
		}

		return(LERR_OK);
	}

	eErr = psFile->psAccessFunc->FilePos(psFile,
										 &u64Position);

	if (eErr != LERR_OK)
	{
		return(eErr);
	}

	if (u64RewindAmount > u64Position)
	{
		return(LERR_FILE_REWIND_PAST_BEGINNING);
	}

	// Figure out the new absolute position
	u64Position -= u64RewindAmount;

	eErr = psFile->psAccessFunc->Seek(psFile,
									  u64Position,
									  FILESEEK_SET);

	return(eErr);
}

// Temporary buffer for line length
static char sg_u8Line[FILE_MAX_LINE_LENGTH];

ELCDErr FileReadString(FILEHANDLE eFileHandle,
					   char **pu8String,
					   UINT32 *pu32StringLength)
{
	ELCDErr eErr = LERR_OK;
	UINT32 u32Pos = 0;
	UINT32 u32BytesRead = 0;

	while (u32Pos < (sizeof(sg_u8Line) - 1))
	{
		eErr = FileRead(eFileHandle,
						&sg_u8Line[u32Pos],
						sizeof(sg_u8Line[u32Pos]),
						&u32BytesRead);

		ERROREXIT_ON_FAIL(eErr);

		if (0 == sg_u8Line[u32Pos])
		{
			// End of string!
			++u32Pos;
			break;
		}

		++u32Pos;
	}

	// Only fall through here if we're out of room or
	if (u32Pos >= (sizeof(sg_u8Line) - 1))
	{
		eErr = LERR_FILE_LINE_TOO_LONG;
		goto errorExit;
	}

	*pu8String = MemAlloc(u32Pos);
	if (NULL == *pu8String)
	{
		eErr = LERR_NO_MEM;
		goto errorExit;
	}

	if (pu32StringLength)
	{
		*pu32StringLength = u32Pos;
	}

	strcpy(*pu8String, sg_u8Line);
	eErr = LERR_OK;

errorExit:
	return(eErr);
}

ELCDErr FileWriteString(FILEHANDLE eFileHandle,
					    LEX_CHAR *peString,
					    UINT32 u32StringLength,
						UINT32 u32BytesToWrite,
						BOOL bTrailingZero)
{
	ELCDErr eErr = LERR_OK;
	UINT32 u32BytesWritten;
	UINT32 u32DataToWrite;
	UINT32 u32ZeroCount;

	if (u32BytesToWrite <= u32StringLength)
	{
		u32DataToWrite = u32BytesToWrite;
		u32ZeroCount = 0;
	}
	else
	if (u32BytesToWrite > u32StringLength)
	{
		u32DataToWrite = u32StringLength;
		u32ZeroCount = u32BytesToWrite - u32DataToWrite;
	}

	if ((peString) && (u32DataToWrite))
	{
		eErr = FileWrite(eFileHandle,
						 (void *) peString,
						 sizeof(*peString) * u32DataToWrite,
						 &u32BytesWritten);
	}

	if (bTrailingZero)
	{
		++u32ZeroCount;
	}

	if (u32ZeroCount)
	{
		UINT8 u8Zero = 0;

		while (u32ZeroCount--)
		{
			eErr = FileWrite(eFileHandle,
							 &u8Zero,
							 sizeof(u8Zero),
							 &u32BytesWritten);
			ERROREXIT_ON_FAIL(eErr);
		}
	}

errorExit:
	return(eErr);
}

ELCDErr FileReadLine(FILEHANDLE eFileHandle, 
					 char **pu8String,
					 BOOL bCSVMode)
{
	SFile *psFile;
	ELCDErr eErr = LERR_OK;
	UINT32 u32Pos = 0;
	UINT32 u32DataRead = 0;

	psFile = FileGetPointer(eFileHandle);
	if (NULL == psFile)
	{
		return(LERR_FILE_BAD_HANDLE);
	}

	// If this asserts it means the incoming string pointer to a pointer
	GCASSERT(NULL == *pu8String);

	while (u32Pos < (sizeof(sg_u8Line) - 1))
	{
		eErr = FileRead(eFileHandle,
						(void *) &sg_u8Line[u32Pos],
						sizeof(sg_u8Line[u32Pos]),
						&u32DataRead);
		// If we have an incomplete read but we have data in our buffer, it's OK
		if ((LERR_FILE_INCOMPLETE_READ == eErr) && (u32Pos))
		{
			eErr = LERR_OK;
			break;
		}

		ERROREXIT_ON_FAIL(eErr);

		if ('\r' == sg_u8Line[u32Pos])
		{
			char eData;

			// End of line. Let's see if there's a trailing '\n'
			eErr = FileRead(eFileHandle,
							(void *) &eData,
							sizeof(eData),
							&u32DataRead);

			// If we have an incomplete read but we have data in our buffer, it's OK
			if ((LERR_FILE_INCOMPLETE_READ == eErr) && (u32Pos))
			{
				eErr = LERR_OK;
				break;
			}

			ERROREXIT_ON_FAIL(eErr);

			// If we have a linefeed, eat it. Otherwise, rewind a character.
			if (eData != '\n')
			{
				eErr = FileRewind(eFileHandle, sizeof(eData));
				ERROREXIT_ON_FAIL(eErr);
			}

			break;
		}

		// Newline character - bail out
		if ('\n' == sg_u8Line[u32Pos])
		{
			break;
		}

		// End of string character - bail out
		if ('\0' == sg_u8Line[u32Pos])
		{
			break;
		}

		// We're accepting the character
		u32Pos++;

		if ((bCSVMode) && ('\"' == sg_u8Line[u32Pos - 1]))
		{
			// Quoted string! Let's keep rockin'!

			while (u32Pos < (sizeof(sg_u8Line) - 1))
			{
				eErr = FileRead(eFileHandle,
								(void *) &sg_u8Line[u32Pos],
								sizeof(sg_u8Line[u32Pos]),
								&u32DataRead);

				// If we have an incomplete read but we have data in our buffer, it's OK
				if (LERR_FILE_INCOMPLETE_READ == eErr)
				{
					eErr = LERR_FILE_UNTERMINATED_STRING;
					goto errorExit;
				}
				
				u32Pos++;
				if (u32Pos >= (sizeof(sg_u8Line) - 1))
				{
					eErr = LERR_FILE_UNTERMINATED_STRING;
					goto errorExit;
				}

				// If the prior character was a ", then we need to check for another "

				if ('\"' == sg_u8Line[u32Pos - 1])
				{
					eErr = FileRead(eFileHandle,
									(void *) &sg_u8Line[u32Pos],
									sizeof(sg_u8Line[u32Pos]),
									&u32DataRead);

					if (LERR_FILE_INCOMPLETE_READ == eErr)
					{
						// This means we didn't get a character - just drop out
						break;
					}

					// We got a character. If it's another ", then eat it, otherwise, rewind
					if ('\"' == sg_u8Line[u32Pos])
					{
						// It's a double double quote, so store it
						u32Pos++;
					}
					else
					{
						// It's a single double quote, so we're at the end of the string now
						eErr  = FileRewind(eFileHandle,
										   sizeof(sg_u8Line[u32Pos]));
						ERROREXIT_ON_FAIL(eErr);
						break;
					}
				}
			}
		}
	}

	// Only fall through here if we're out of room or
	if (u32Pos >= (sizeof(sg_u8Line) - 1))
	{
		eErr = LERR_FILE_LINE_TOO_LONG;
		goto errorExit;
	}

	sg_u8Line[u32Pos] = '\0';

	// Duplicate the string
	*pu8String = ASCIIstrdup(sg_u8Line);
	if (NULL == *pu8String)
	{
		eErr = LERR_NO_MEM;
	}

errorExit:
	return(eErr);
}


#define	MAX_FPRINTF_BUFFER	16384

ELCDErr Filefprintf(FILEHANDLE eFileHandle, 
					LEX_CHAR *peFormat, 
					...)
{
	va_list ap;
	LEX_CHAR *pu8Buffer;
	int s32Result;
	ELCDErr eErr;
	SFile *psFile;
	UINT32 u32BytesWritten;

	psFile = FileGetPointer(eFileHandle);
	if (NULL == psFile)
	{
		return(LERR_FILE_BAD_HANDLE);
	}

	pu8Buffer = MemAlloc(MAX_FPRINTF_BUFFER);
	if (NULL == pu8Buffer)
	{
		return(LERR_NO_MEM);
	}

	va_start(ap, peFormat);	
	s32Result = vsprintf((char *) pu8Buffer,  (const char *) peFormat, ap);
	va_end(ap);
	
	if (s32Result == (MAX_FPRINTF_BUFFER - 1))
	{
		// Truncated!
		eErr = LERR_FORMAT_TRUNCATED;
		goto errorExit;
	}

	// Got it - now write it out
	eErr = FileWrite(eFileHandle,
					 pu8Buffer,
					 s32Result,
					 &u32BytesWritten);

errorExit:
	GCFreeMemory(pu8Buffer);
	return(eErr);
}

ELCDErr Filefgetc(FILEHANDLE eFileHandle,
				  UINT8 *pu8Character)
{
	UINT32 u32BytesRead;

	return(FileRead(eFileHandle,
					(void *) pu8Character,
					1,
					&u32BytesRead));
}

BOOL IsDirSep(LEX_CHAR eChar)
{
	if ((((LEX_CHAR) '/') == eChar) ||
		(((LEX_CHAR) '\\') == eChar))
	{
		return(TRUE);
	}
	else
	{
		return(FALSE);
	}
}

static BOOL IsWildcardChar(LEX_CHAR eChar)
{
	if ((((LEX_CHAR) '*') == eChar) ||
		(((LEX_CHAR) '?') == eChar))
	{
		return(TRUE);
	}
	else
	{
		return(FALSE);
	}
}

ELCDErr RelativeToAbsolutePath(LEX_CHAR *peCurrentDirectory,
							   LEX_CHAR *peNewPath,
							   LEX_CHAR **ppeAbsolutePath,
							   BOOL bIncludeTrailingSlash)
{
	LEX_CHAR *peSanitizedNewPathHead = NULL;
	LEX_CHAR *peSanitizedNewPath = NULL;
	LEX_CHAR *peFinalPathTmp = NULL;
	LEX_CHAR *peFinalPathTmpHead = NULL;

	UINT32 u32Position = 0;
	
	GCASSERT(peCurrentDirectory);
	GCASSERT(peNewPath);

	peSanitizedNewPath = Lexstrdup(peNewPath);
	peSanitizedNewPathHead = peSanitizedNewPath;
	
	if (NULL == peSanitizedNewPathHead)
	{
		return(LERR_NO_MEM);
	}

	// Eliminate the following:
	//
	// * Duplicate directory separators
	// * Leading or trailing spaces on any directory names

	while (*peSanitizedNewPath)
	{
		if (IsDirSep(*peSanitizedNewPath) && 
			IsDirSep(*(peSanitizedNewPath + 1)))
		{
			Lexstrcpy(peSanitizedNewPath, peSanitizedNewPath + 1);
		}
		else
		if (IsDirSep(*peSanitizedNewPath))
		{
			while ((u32Position) &&
				((('\t' == *(peSanitizedNewPath - 1)) ||
				  (' ' == *(peSanitizedNewPath - 1)))))
			{
				--peSanitizedNewPath;
				--u32Position;
				// Get rid of any trailing spaces
				Lexstrcpy(peSanitizedNewPath, peSanitizedNewPath + 1);
			}

			if (('\t' == *(peSanitizedNewPath + 1)) ||
				(' ' == *(peSanitizedNewPath + 1)))
			{
				Lexstrcpy(peSanitizedNewPath + 1, peSanitizedNewPath + 2);
			}
			else
			{
				++peSanitizedNewPath;
				++u32Position;
			}
		}
		else
		{
			++peSanitizedNewPath;
			++u32Position;
		}
	}
	
	// Now eliminate any "current directory" stupidity, such as blah/blah/./foo/./blahblah

	peSanitizedNewPath = peSanitizedNewPathHead;
	while (*peSanitizedNewPath)
	{
		// If it's a .., skip over it
		if (((LEX_CHAR) '.' == *peSanitizedNewPath) &&
			((LEX_CHAR) '.' == *(peSanitizedNewPath + 1)))
		{
			peSanitizedNewPath += 2;
		}
		else
		if (((LEX_CHAR) '.' == *peSanitizedNewPath) &&
			 IsDirSep(*(peSanitizedNewPath + 1)))
		{
			// We have a ./ that we need to eliminate
			Lexstrcpy(peSanitizedNewPath, peSanitizedNewPath + 2);
		}
		else
		{
			peSanitizedNewPath++;
		}
	}

	// If the last character isn't a directory separator, make it one
	if ((FALSE == IsDirSep(*(peSanitizedNewPathHead + Lexstrlen(peSanitizedNewPathHead) - 1))) &&
		(bIncludeTrailingSlash))
	{
		UINT32 u32Length = Lexstrlen(peSanitizedNewPathHead);

		*(peSanitizedNewPathHead + u32Length) = '/';
		*(peSanitizedNewPathHead + u32Length + 1) = '\0';
	}

	if (FALSE == IsDirSep(*peSanitizedNewPathHead))
	{
		// This means it's a relative path, so we'll have to take the current directory
		// and process relative pathing/directories to come up with a new master
		// directory path.

		peFinalPathTmp = MemAlloc((Lexstrlen(peSanitizedNewPathHead) + Lexstrlen(peCurrentDirectory)) * sizeof(*peFinalPathTmp) + sizeof(*peCurrentDirectory) + sizeof(*peCurrentDirectory));
		peFinalPathTmpHead = peFinalPathTmp;
		if (NULL == peFinalPathTmp)
		{
			GCFreeMemory(peSanitizedNewPathHead);
			return(LERR_NO_MEM);
		}

		// Copy in our current directory
		Lexstrcpy(peFinalPathTmpHead, peCurrentDirectory);

		// Make sure the current directory has a trailing separator already
		if (FALSE == IsDirSep(*(peFinalPathTmpHead + Lexstrlen(peFinalPathTmpHead) - 1)))
		{
			// No trailing directory separator. Add one.
			Lexstrcat(peFinalPathTmpHead, "/");
		}

		// Point peFinalPathTmp to the next position in the final path
		peFinalPathTmp = peFinalPathTmpHead + Lexstrlen(peFinalPathTmpHead);

		peSanitizedNewPath = peSanitizedNewPathHead;
		while (*peSanitizedNewPath)
		{
			if (((LEX_CHAR) '.' == *peSanitizedNewPath) &&
				((LEX_CHAR) '.' == *(peSanitizedNewPath + 1)))
			{
				// Go up a directory, but only if there's a separator character after it or
				// if the final path tmp and new path pointers are the same, this is an invalid
				// path

				if ((FALSE == IsDirSep(*(peSanitizedNewPath + 2))) ||
					(peFinalPathTmpHead == (peFinalPathTmp - 1)))
				{
					GCFreeMemory(peFinalPathTmpHead);
					GCFreeMemory(peSanitizedNewPathHead);
					return(LERR_ENOENT);
				}

				// Looks good! Back up over the directory separator and keep backing up
				// until we hit another one
				--peFinalPathTmp;

				// If this asserts, the log above it failed
				GCASSERT(IsDirSep(*peFinalPathTmp));
				--peFinalPathTmp;

				while (FALSE == IsDirSep(*peFinalPathTmp))
				{
					--peFinalPathTmp;
				}

				++peFinalPathTmp;
				*peFinalPathTmp = (LEX_CHAR) '\0';

				// Skip over ../
				peSanitizedNewPath += (3 * sizeof(*peSanitizedNewPath));
			}
			else
			{
				// This is a directory string we need to add
				while (1)
				{
					*peFinalPathTmp = *peSanitizedNewPath;
					++peFinalPathTmp;
					++peSanitizedNewPath;
					if (IsDirSep(*(peSanitizedNewPath - 1)) || ((LEX_CHAR) '\0' == *peSanitizedNewPath))
					{
						// It's copied! Bail out
						break;
					}
				}
			}
		}
	}
	else
	{
		// It's an absolute path
	}

	// We're done with the temp sanitization
	GCFreeMemory(peSanitizedNewPathHead);

	*ppeAbsolutePath = peFinalPathTmpHead;
	return(LERR_OK);
}

ELCDErr FileGetPath(LEX_CHAR *pePathFile,
					LEX_CHAR **ppePath)
{
	UINT32 u32Loop;
	LEX_CHAR *pePtr = NULL;

	if (NULL == pePathFile)
	{
		*ppePath = NULL;
		return(LERR_OK);
	}

	u32Loop = Lexstrlen(pePathFile);

	if (u32Loop)
	{
		u32Loop--;
		pePtr = pePathFile + u32Loop;

		while ((pePtr != pePathFile) && 
			    (IsDirSep(*pePtr) == FALSE))
		{
			--pePtr;
			--u32Loop;
		}
	}

	if ((pePtr == pePathFile) && (FALSE == IsDirSep(*pePtr)))
	{
		// No path anywhere here
		*ppePath = NULL;
		return(LERR_OK);
	}
	else
	{
		// We've got a path of some sort
		u32Loop++;
		*ppePath = Lexstrndup(pePathFile, u32Loop);
		if (NULL == *ppePath)
		{
			return(LERR_NO_MEM);
		}
		else
		{
			return(LERR_OK);
		}
	}

}

ELCDErr FileGetFilename(LEX_CHAR *pePathFile,
						LEX_CHAR **ppeBaseFilename)
{
	UINT32 u32Loop;
	LEX_CHAR *pePtr = NULL;

	if (NULL == pePathFile)
	{
		*ppeBaseFilename = NULL;
		return(LERR_OK);
	}

	u32Loop = Lexstrlen(pePathFile);

	if (u32Loop)
	{
		u32Loop--;
		pePtr = pePathFile + u32Loop;

		while ((pePtr != pePathFile) && 
			    (IsDirSep(*pePtr) == FALSE))
		{
			--pePtr;
			--u32Loop;
		}
	}

	if (IsDirSep(*pePtr))
	{
		pePtr++;
	}

	*ppeBaseFilename = Lexstrdup(pePtr);
	if (NULL == *ppeBaseFilename)
	{
		return(LERR_NO_MEM);
	}
	else
	{
		return(LERR_OK);
	}
}

BOOL FilenameIsWildcard(LEX_CHAR *peFilename)
{
	UINT32 u32Loop;
	LEX_CHAR *pePtr = NULL;

	// If it's NULL, it's not a wildcard
	if (NULL == peFilename)
	{
		return(FALSE);
	}

	u32Loop = Lexstrlen(peFilename);

	if (u32Loop)
	{
		u32Loop--;
		pePtr = peFilename + u32Loop;

		// Only pay attention to the part of the text that's a filename (ignore directories)

		while ((pePtr != peFilename) && 
			    (IsDirSep(*pePtr) == FALSE))
		{
			if (IsWildcardChar(*pePtr))
			{
				return(TRUE);
			}

			--pePtr;
			--u32Loop;
		}
	}

	return(FALSE);
}

BOOL FileExists(LEX_CHAR *peFilename)
{
	FILEHANDLE eFileHandle;
	ELCDErr eErr;

	eErr = FileOpen(&eFileHandle,
					peFilename,
					(LEX_CHAR *) "rb");

	if (LERR_OK == eErr)
	{
		(void) FileClose(&eFileHandle);
		return(TRUE);
	}

	return(FALSE);
}

ELCDErr FileSetTimeout(FILEHANDLE eFileHandle,
					   UINT32 u32Timeout)
{
	SFile *psFile;

	psFile = FileGetPointer(eFileHandle);
	if (NULL == psFile)
	{
		return(LERR_FILE_BAD_HANDLE);
	}

	if (NULL == psFile->psAccessFunc->FileSetTimeout)
	{
		// Can't do a set timeout. Function not supported
		return(LERR_FILE_INVALID_ACTION);
	}

	return(psFile->psAccessFunc->FileSetTimeout(psFile,
												u32Timeout));
}

ELCDErr FileStatus(FILEHANDLE eFileHandle,
				   UINT32 *pu32FileStatus)
{
	SFile *psFile;

	psFile = FileGetPointer(eFileHandle);
	if (NULL == psFile)
	{
		return(LERR_FILE_BAD_HANDLE);
	}

	if (NULL == psFile->psAccessFunc->FileStatus)
	{
		// Can't do a status. Function not supported
		return(LERR_FILE_INVALID_ACTION);
	}

	return(psFile->psAccessFunc->FileStatus(psFile,
											pu32FileStatus));

}

ELCDErr FileFlush(FILEHANDLE eFileHandle)
{
	SFile *psFile;

	psFile = FileGetPointer(eFileHandle);
	if (NULL == psFile)
	{
		return(LERR_FILE_BAD_HANDLE);
	}

	if (NULL == psFile->psAccessFunc->FileFlush)
	{
		// Can't do a status. Function not supported
		return(LERR_FILE_INVALID_ACTION);
	}

	return(psFile->psAccessFunc->FileFlush(psFile));
}

ELCDErr FileChdir(LEX_CHAR *peDirectory)
{
	const SFileAccess *psAccess;
	LEX_CHAR *peRealFilename = NULL;
	LEX_CHAR *peDeviceName = NULL;
	ELCDErr eErr = LERR_OK;

	// Go snag the access routines for this file type (whatever it may be)
	psAccess = FileGetFileProcs(peDirectory,
								&peRealFilename,
								&peDeviceName,
								FALSE);

	if (NULL == psAccess)
	{
		eErr = LERR_FILE_UNKNOWN_DEVICE_TYPE;
		goto errorExit;
	}
	
	if (NULL == psAccess->FileChdir)
	{
		eErr = LERR_FILE_INVALID_ACTION;
		goto errorExit;
	}

	// Looks good! Go set the current directory (if we can)
	eErr = psAccess->FileChdir(psAccess,
							   peRealFilename);

errorExit:
	return(eErr);
}

ELCDErr FileMkdir(LEX_CHAR *peDirectory)
{
	const SFileAccess *psAccess;
	LEX_CHAR *peRealFilename = NULL;
	LEX_CHAR *peDeviceName = NULL;
	ELCDErr eErr = LERR_OK;

	// Go snag the access routines for this file type (whatever it may be)
	psAccess = FileGetFileProcs(peDirectory,
								&peRealFilename,
								&peDeviceName,
								FALSE);

	if (NULL == psAccess)
	{
		eErr = LERR_FILE_UNKNOWN_DEVICE_TYPE;
		goto errorExit;
	}
	
	if (NULL == psAccess->FileMkdir)
	{
		eErr = LERR_FILE_INVALID_ACTION;
		goto errorExit;
	}

	// Looks good! Go make the directory (if possible)
	eErr = psAccess->FileMkdir(psAccess,
							   peRealFilename);

errorExit:
	return(eErr);
}

ELCDErr FileRmdir(LEX_CHAR *peDirectory)
{
	const SFileAccess *psAccess;
	LEX_CHAR *peRealFilename = NULL;
	LEX_CHAR *peDeviceName = NULL;
	ELCDErr eErr = LERR_OK;

	// Go snag the access routines for this file type (whatever it may be)
	psAccess = FileGetFileProcs(peDirectory,
								&peRealFilename,
								&peDeviceName,
								FALSE);

	if (NULL == psAccess)
	{
		eErr = LERR_FILE_UNKNOWN_DEVICE_TYPE;
		goto errorExit;
	}
	
	if (NULL == psAccess->FileRmdir)
	{
		eErr = LERR_FILE_INVALID_ACTION;
		goto errorExit;
	}

	// Looks good! Go remove the directory if we can
	eErr = psAccess->FileRmdir(psAccess,
							   peRealFilename);

errorExit:
	return(eErr);
}

ELCDErr FileDelete(LEX_CHAR *peFileToDelete)
{
	const SFileAccess *psAccess;
	LEX_CHAR *peRealFilename = NULL;
	LEX_CHAR *peDeviceName = NULL;
	ELCDErr eErr = LERR_OK;

	// Go snag the access routines for this file type (whatever it may be)
	psAccess = FileGetFileProcs(peFileToDelete,
								&peRealFilename,
								&peDeviceName,
								FALSE);

	if (NULL == psAccess)
	{
		eErr = LERR_FILE_UNKNOWN_DEVICE_TYPE;
		goto errorExit;
	}
	
	if (NULL == psAccess->FileDelete)
	{
		eErr = LERR_FILE_INVALID_ACTION;
		goto errorExit;
	}

	// Looks good! Go remove the directory if we can
	eErr = psAccess->FileDelete(peRealFilename);

errorExit:
	return(eErr);
}

ELCDErr FileShutdown(void)
{
	UINT32 u32Loop = 0;

	for (u32Loop = 0; u32Loop < (sizeof(sg_psFileList) / sizeof(sg_psFileList[0])); u32Loop++)
	{
		if (sg_psFileList[u32Loop])
		{
			FILEHANDLE eFileHandle = (FILEHANDLE) u32Loop;

			(void) FileClose(&eFileHandle);
		}
	}

	return(LERR_OK);
}

void FileInit(void)
{
	UINT32 u32Loop = 0;
	ELCDErr eErr = LERR_OK;

	for (u32Loop = 0; u32Loop < (sizeof(sg_sFileMethods) / sizeof(sg_sFileMethods[0])); u32Loop++)
	{
		if (sg_sFileMethods[u32Loop].psFileAccess)
		{
			if (sg_sFileMethods[u32Loop].psFileAccess->OneTimeInit)
			{
				eErr = sg_sFileMethods[u32Loop].psFileAccess->OneTimeInit(&sg_sFileMethods[u32Loop]);
				GCASSERT(LERR_OK == eErr);
			}
		}
	}
}
