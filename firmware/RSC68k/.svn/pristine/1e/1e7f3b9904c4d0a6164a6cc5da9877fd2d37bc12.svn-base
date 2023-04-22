// Shim file for being able to compile a BIOS filesystem into the code

#include "Startup/app.h"
#define _GCBIOS_H_
#include "Win32/BIOS/Include/storage.h"
#include "Win32/BIOS/Include/MmcSd.h"
#include "Win32/BIOS/Include/GCControls.h"
#include "Win32/BIOS/Include/unzip.h"
#define _OS_H_
#include "Win32/BIOS/Boot/fat32rw/fat.h"

static FILE *sg_pDiskFile = NULL;
static BOOL sg_bBIOSShimActive = FALSE;

EGCResultCode GCOSSemaphorePut_Safe(SOSSemaphore psSemaphore)
{
	return(GCOSSemaphorePut(psSemaphore));
}

EGCResultCode GCOSSemaphoreGet_Safe(SOSSemaphore psSemaphore)
{
	return(GCOSSemaphoreGet(psSemaphore,
							0));
}

EGCResultCode GCOSSemaphoreCreate_SafeInternal(SOSSemaphore *ppsSemaphore,
											  UINT32 u32InitialValue,
											  char *pu8Filename,
											  UINT32 u32LineNumber)
{
	return(GCOSSemaphoreCreate(ppsSemaphore,
							   u32InitialValue));
}

EGCResultCode GCOSSemaphoreDelete_Safe(	SOSSemaphore psSemaphore,
										EOSEventDeleteOption eDeleteOption)
{
	return(GCOSSemaphoreDelete(psSemaphore,
							   eDeleteOption));
}

EGCResultCode GCOSSemaphoreDelete_SafeInternal(SOSSemaphore *ppsSemaphore,
												EOSEventDeleteOption eDeleteOption,
											  char *pu8Filename,
											  UINT32 u32LineNumber)
{
	return(GCOSSemaphoreDelete(ppsSemaphore,
							   eDeleteOption));
}

void SDRAMFree(void *pvPointer)
{
	GCFreeMemory(pvPointer);
}

void SRAMFree(void *pvPointer)
{
	GCASSERT(0);
}

void UserFree(void *pvBase)
{
	GCASSERT(0);
}

BOOL UserAppRunning(void)
{
	GCASSERT(0);
}

void *UserAllocInternal(UINT32 u32Size, UINT8 *pu8ModuleName, UINT32 u32LineNumber, BOOL bClearBlock)
{
	GCASSERT(0);
}

EGCResultCode StorageRegionWrite(EStorageRegionType eRegionType,
								 UINT32 u32Instance,
								 UINT64 u64Offset,
								 void *pvData,
								 UINT64 u64Size)
{
	int iResult;

	// Seek to the offset we're expecting
	iResult = fseek(sg_pDiskFile, u64Offset, SEEK_SET);
	GCASSERT(0 == iResult);

	// Now read
	iResult = fwrite(pvData, 1, u64Size, sg_pDiskFile);
//	GCASSERT(iResult == u64Size);

	return(GC_OK);
}

void StorageGetMap(struct SStorageMap **ppsMap,
						  EStorageRegionType eRegionType,
						  UINT32 u32Instance)
{
	// Not used on the shim - give it a non-zero value to make it happy
	*ppsMap = GCAllocateMemory(512);
}

EGCResultCode StorageRegionRead(EStorageRegionType eRegionType,
									   UINT32 u32Instance,
									   UINT64 u64Offset,
									   void *pvData,
									   UINT64 u64Size)
{
	int iResult;

	// Seek to the offset we're expecting
	iResult = fseek(sg_pDiskFile, u64Offset, SEEK_SET);
	GCASSERT(0 == iResult);

	// Now read
	iResult = fread(pvData, 1, u64Size, sg_pDiskFile);
	GCASSERT(iResult == u64Size);

	return(GC_OK);
}

EGCResultCode StorageRegionFlush(EStorageRegionType eRegionType,
										UINT32 u32Instance)
{
	GCASSERT(0);
}

EGCResultCode StorageGetRegionInstanceByInstance(EStorageRegionType eRegionType,
														UINT32 u32DesiredInstance,
														ENVDevice *peDevice)
{
	if (u32DesiredInstance > 0)
	{
		return(GC_STORAGE_REGION_INSTANCE_NOT_FOUND);
	}

	*peDevice = NVDEV_SD;
	return(GC_OK);
}

EGCResultCode StorageRegionGetSize(EStorageRegionType eRegionType,
									 	  UINT32 u32Instance,
										  UINT64 *pu64RegionSize)
{
	GCASSERT(0);
}

EGCResultCode StorageGetSerialNumber(EStorageRegionType eRegionType,
											UINT32 u32Instance,
											UINT8 *pu8SerialNumberLocation,
											UINT32 *pu32SerialNumberLength)
{
	GCASSERT(0);
}

EGCResultCode MMCSDRead(SMMCSDAttrib *psCardAttributes,
						UINT8 *pu8SourceBuffer,
						UINT64 u64SourceSize,
						UINT64 u64DeviceOffset)
{
	int iResult;

	// Seek to the offset we're expecting
	iResult = fseek(sg_pDiskFile, u64DeviceOffset, SEEK_SET);
	GCASSERT(0 == iResult);

	// Now read
	iResult = fread(pu8SourceBuffer, 1, u64SourceSize, sg_pDiskFile);
	GCASSERT(iResult == u64SourceSize);

	return(GC_OK);
}

UINT32 PlatformMemoryBootStart(void)
{
	GCASSERT(0);
}

UINT32 PlatformMemoryBootLength(void)
{
	GCASSERT(0);
}

UINT32 PlatformMemorySDRAMGetSize(void)
{
	GCASSERT(0);
}

static UINT8 sg_u8EA50BlowfishKey[] =
{
	0xba,	0x4c,	0xa2,	0xe3,
	0x99,	0xc4,	0x32,	0xd7,
	0x27,	0xec,	0x4d,	0xd2,
	0x74,	0x02,	0x07,	0x56,
	0xd9,	0x72,	0xf4,	0x5f,
	0x48,	0xda,	0x79,	0x7e,
	0x56,	0x2f,	0xbc,	0x0c,
	0x7c,	0x9a,	0xaf,	0x55,
	0xd0,	0x2b,	0xaa,	0x48,
	0xd8,	0x26,	0x08,	0x74,
	0xa1,	0x63,	0xaa,	0xb4,
	0x93,	0x93,	0x52,	0x53,
	0x00,	0xa5,	0x4a,	0xc3,
	0x74,	0x24,	0xec,	0xdb,
	0xff,	0xa0,	0x82,	0x67,
	0x10,	0x61,	0x1a,	0x0a
};

static SBoardFile sg_sEA50Wad =
{
	(UINT8 *) "master.core",
	sg_u8EA50BlowfishKey,
	sizeof(sg_u8EA50BlowfishKey)
};

SBoardFile *GCGetBoardFile(void)
{
	return(&sg_sEA50Wad);
}

UINT32 GCGetHardwareFeatures(void)
{
	GCASSERT(0);
}

BOOL BIOSShimActive(void)
{
	return(sg_bBIOSShimActive);
}

void BIOSShimInit(char *pu8Filename)
{
	int s32FATMountResult;

	if (NULL == pu8Filename)
	{
		sg_bBIOSShimActive = FALSE;
	}
	else
	{
		sg_pDiskFile = fopen(pu8Filename, "rb");
		GCASSERT(sg_pDiskFile);

		// MOUNT It. ;-)
		GCFilesystemInit();

		GCChangeDrive("sd0");
		sg_bBIOSShimActive = TRUE;
	}
}
