#ifndef _FILE_H_
#define _FILE_H_

typedef struct SUCFileInfo
{
	GCFile eFileHandle;						// File handle
	BOOL bBufferDirty;						// Do we have uncommitted/unflushed writes?
} SUCFileInfo;

// Seek type
typedef enum
{
	FILESEEK_SET,
	FILESEEK_CUR,
	FILESEEK_END
} EFileOrigin;

// Forward reference
struct SFile;

typedef struct SFileMethods
{
	LEX_CHAR *pePrefix;							// Prefix to filename to select device
	const struct SFileAccess *psFileAccess;		// File access methods
	BOOL bFind;									// Is this a find match method?
} SFileMethods;

typedef struct SFileAccess
{
	ELCDErr (*OneTimeInit)(const SFileMethods *psMethod);
	ELCDErr (*Open)(struct SFile *psFile, LEX_CHAR *peDeviceName, LEX_CHAR *peFilename, LEX_CHAR *peFileMode);
	ELCDErr (*Read)(struct SFile *psFile, void *pvData, UINT32 u32ByteCount, UINT32 *pu32BytesRead);
	ELCDErr (*Write)(struct SFile *psFile, void *pvData, UINT32 u32ByteCount, UINT32 *pu32BytesWritten);
	ELCDErr (*Seek)(struct SFile *psFile, UINT64 u64Offset, EFileOrigin eOrigin);
	ELCDErr (*Close)(struct SFile *psFile);
	ELCDErr (*FindNext)(struct SFile *psFile, LEX_CHAR *peFilename, UINT32 *pu32Attributes, UINT32 u32MaxLength);
	ELCDErr (*FileSize)(struct SFile *psFile, UINT64 *pu64FileSize);
	ELCDErr (*FileEOF)(struct SFile *psFile, BOOL *pbEOF);
	ELCDErr (*FilePos)(struct SFile *psFile, UINT64 *pu64FilePos);
	ELCDErr (*FileStatus)(struct SFile *psFile, UINT32 *pu32StatusBits);
	ELCDErr (*FileSetTimeout)(struct SFile *psFile, UINT32 u32TimeoutValue);
	ELCDErr (*FileFlush)(struct SFile *psFile);
	ELCDErr (*FileChdir)(const struct SFileAccess *psAccess,
						 LEX_CHAR *peNewDirectory);
	ELCDErr (*FileRmdir)(const struct SFileAccess *psAccess,
						 LEX_CHAR *peDirectory);
	ELCDErr (*FileMkdir)(const struct SFileAccess *psAccess,
						 LEX_CHAR *peDirectory);
	ELCDErr (*FileDelete)(LEX_CHAR *peFilename);
} SFileAccess;

typedef struct SFileFindData
{
	SFileFind *psFileFind;					// Our file find structure for the filesystem
	UINT32 u32DesiredAttributeMask;			// Desired mask for attributes
} SFileFindData;

typedef struct SSerialData
{
	UINT32 u32UARTIndex;					// Which UART is this?
} SSerialData;

typedef enum
{
	ENET_NOT_CONNNECTED,		// Not connnected
	ENET_TCP_OUTGOING,			// Outgoing initiated TCP connection
	ENET_TCP_LISTEN,			// Incoming initiated TCP connection
	ENET_UDP_OUTGOING,			// Outgoing initiated UDP socket
	ENET_UDP_LISTEN				// Incoming initiated UDP socket
} EConnectionType;

typedef struct SNetworkData
{
	UINT32 u32SocketHandle;
	EConnectionType eConnectionType;	// What kind of connection is this?
	UINT32 u32UDPDestIPAddress;			// UDP Destination IP address specified at FileOpen
	UINT16 u16UDPDestPort;				// UDP Destination port specified at FileOpen
} SNetworkData;

// Timeout is set to "forever"
#define	FILE_WAIT_FOREVER		0xffffffff

typedef struct SFile
{
	const SFileAccess *psAccessFunc;		// Access functions for this file type
	UINT32 u32Timeout;						// Timeout for this file

	// Rewind buffer (if applicable)
	UINT8 *pu8RewindBuffer;					// Rewind buffer (if applicable)
	UINT32 u32RewindHead;					// Rewind head pointer
	UINT32 u32RewindTail;					// Rewind tail pointer
	UINT32 u32RewindSize;					// Rewind buffer size
	UINT32 u32RewoundAmount;				// How many characters we're currently rewound

	union
	{
		SUCFileInfo *psFileInfo;			// File type
		SFileFindData *psFileFind;			// File find action
		SSerialData *psSerial;				// Serial data info
		SNetworkData *psNetwork;			// Network info
	} uFileInfo;
} SFile;

// Generic file access functions
extern ELCDErr FileOpen(FILEHANDLE *peFileHandle,
						LEX_CHAR *peFilename,	// Our file name
						LEX_CHAR *peFileMode);
extern ELCDErr FileFindOpen(FILEHANDLE *peFileHandle,
							LEX_CHAR *pePattern,	// Our pattern
							LEX_CHAR *peWhatToLookFor);
extern ELCDErr FileFind(FILEHANDLE peFileHandle,
						LEX_CHAR *peFilenameStorageArea,
						UINT32 *pu32Attributes,
						UINT32 u32MaxLength);
extern ELCDErr FileClose(FILEHANDLE *peFileHandle);
extern ELCDErr FileRead(FILEHANDLE eFileHandle,
						void *pvLocation,
						UINT32 u32BytesToRead,
						UINT32 *pu32BytesRead);
extern ELCDErr FileWrite(FILEHANDLE eFileHandle,
						 void *pvLocation,
						 UINT32 u32BytesToWrite,
						 UINT32 *pu32BytesWrite);
extern ELCDErr FileSeek(FILEHANDLE eFileHandle,
						UINT64 u64Offset,
						EFileOrigin eOrigin);
extern ELCDErr FileSize(FILEHANDLE eFileHandle,
						LEX_CHAR *peFileName,
						UINT64 *pu64FileSize);
extern ELCDErr FileEOF(FILEHANDLE eFileHandle,
					   BOOL *pbFileEOF);
extern ELCDErr FilePos(FILEHANDLE eFileHandle,
					   UINT64 *pu64FilePos);
extern ELCDErr FileRewind(FILEHANDLE eFileHandle,
						  UINT64 u64RewindAmount);
extern ELCDErr FileSetTimeout(FILEHANDLE eFileHandle,
							  UINT32 u32Timeout);
extern ELCDErr FileStatus(FILEHANDLE eFileHandle,
						  UINT32 *pu32FileStatus);
extern ELCDErr Filefprintf(FILEHANDLE eFileHandle, 
						   LEX_CHAR *peFormat, 
						   ...);
extern ELCDErr Filefgetc(FILEHANDLE eFileHandle,
						 UINT8 *pu8Character);
extern ELCDErr FileChdir(LEX_CHAR *peDirectory);
extern ELCDErr FileMkdir(LEX_CHAR *peDirectory);
extern ELCDErr FileRmdir(LEX_CHAR *peDirectory);
extern ELCDErr FileDelete(LEX_CHAR *peFileToDelete);
extern BOOL FileIsArchive(LEX_CHAR *peFilename);
extern void FileAddSymbols(void);
extern ELCDErr FileGetPath(LEX_CHAR *pePathFile,
						   LEX_CHAR **ppePath);
extern ELCDErr FileGetFilename(LEX_CHAR *pePathFile,
							   LEX_CHAR **ppePath);
extern BOOL FilenameIsWildcard(LEX_CHAR *peFilename);
extern BOOL FileExists(LEX_CHAR *peFilename);
extern ELCDErr FileShutdown(void);
extern void FileInit(void);
extern BOOL IsDirSep(LEX_CHAR eChar);
extern ELCDErr FileFlush(FILEHANDLE eFileHandle);
extern ELCDErr FileReadLine(FILEHANDLE eFileHandle, 
							char **ppu8String,
							BOOL bCSVMode);
extern UINT32 FileConvertTimeoutToOS(SFile *psFile);
extern ELCDErr RelativeToAbsolutePath(LEX_CHAR *peCurrentDirectory,
									  LEX_CHAR *peNewPath,
									  LEX_CHAR **ppeAbsolutePath,
									  BOOL bIncludeTrailingSlash);
extern ELCDErr FileReadString(FILEHANDLE eFileHandle,
							  char **pu8String,
							  UINT32 *pu32StringLength);
extern ELCDErr FileWriteString(FILEHANDLE eFileHandle,
							   LEX_CHAR *peString,
							   UINT32 u32StringLength,
							   UINT32 u32BytesToWrite,
							   BOOL bTrailingZero);

// Tweakable buffer sizes
#define	REWIND_BUFFER_SIZE_SERIAL		128
#define REWIND_BUFFER_SIZE_NETWORK		1024

// Maximum line length
#define FILE_MAX_LINE_LENGTH			16384

#endif	// #ifndef _FILE_H_
