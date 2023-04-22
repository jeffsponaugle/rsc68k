#include "Startup/app.h"

#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <mmsystem.h>
#include <time.h>
#include <io.h>
#include <direct.h>
#include <crtdbg.h>
#include "Include/ram.h"
#include "Win32/BIOSShim.h"
#include "Application/CmdLine.h"

#include <stdio.h>

// Init routine - this is the main entry point for the application

static SMemoryHeap *sg_psAppHeap = NULL;

#ifdef _WIN32
//extern CRITICAL_SECTION sg_sRAMCriticalSection;	// ram.c
#endif

#ifdef _WIN32
void HeapReport(SMemoryHeap *psHeap,
				UINT8 *pu8Filename)
{
#ifndef _MYHEAP
	return;
#else
	FILE *fp;
	SBlock *psBlock = NULL;
	UINT32 u32FreeSize = 0;
	UINT32 u32AllocSize = 0;

//	EnterCriticalSection(&sg_sRAMCriticalSection);

	fp = fopen((const char *) pu8Filename, "w");

	GCASSERT(fp);

	psBlock = psHeap->psBlock;

	while (psBlock)	
	{
		if (psBlock->psAllocNext || psBlock->psAllocPrior)
		{
			fprintf(fp, (UINT8 *) "A: (%6d) 0x%.8x %-5d %s\n", psBlock->u32BlockSize, psBlock->pvUserBase, psBlock->u32Line, psBlock->pu8Module);
			u32AllocSize += psBlock->u32BlockSize + sizeof(*psBlock);
		}
		else
		{
			fprintf(fp, (UINT8 *)"F: (%6d) 0x%.8x\n", psBlock->u32BlockSize, psBlock->pvUserBase);
			u32FreeSize += psBlock->u32BlockSize + sizeof(*psBlock);
		}
		psBlock = psBlock->psNext;
	}

	fprintf(fp, (UINT8 *)"Total free : %d\n", u32FreeSize);
	fprintf(fp, (UINT8 *)"Total alloc: %d\n", u32AllocSize);
	fprintf(fp, (UINT8 *)"Total mem  : %d\n", u32FreeSize + u32AllocSize);
	fprintf(fp, (UINT8 *)"Heap size  : %d ", psHeap->u32HeapSize);

	if (psHeap->u32HeapSize == (u32FreeSize + u32AllocSize))
	{
		fprintf(fp, (UINT8 *)"Match!\n");
	}
	else
	{
		fprintf(fp, (UINT8 *)"Mismatch!\n");
	}

	HeapStats(fp);

	fclose(fp);
//	LeaveCriticalSection(&sg_sRAMCriticalSection);
#endif
}
#endif

#ifdef _WIN32
void GCHeapCheck(void)
{
//	CheckHeapIntegrity(sg_psAppHeap);
}

void GCRaiseAnchor(void)
{
	HeapRaiseAnchor(sg_psAppHeap);
}

void GCDropAnchor(void)
{
	HeapDropAnchor(sg_psAppHeap);
}
#endif

static BOOL bInBlockReport = FALSE;
void BlockReport(const char* in_pName)
{
	// avoid double assert...
	if (bInBlockReport) 
	{
		return;
	}

	bInBlockReport = TRUE;
	HeapReport(sg_psAppHeap,
		(UINT8 *) in_pName);
	bInBlockReport = FALSE;
}

void *GCAllocateMemoryInternal(UINT32 u32Size,
							   UINT8 *pu8ModuleName,
							   UINT32 u32LineNumber,
							   BOOL bClearBlock)
{
	void *pvMemoryHeap = NULL;

#ifndef _MYHEAP
	pvMemoryHeap = calloc(1, u32Size);
	return(pvMemoryHeap);
#else
	EGCResultCode eResult;
	SMemoryInfo sMemoryInfo;

	GCHeapCheck();

	sMemoryInfo.pu8ModuleName = pu8ModuleName;
	sMemoryInfo.u32LineNumber = u32LineNumber;
	sMemoryInfo.bClearBlock = bClearBlock;

	eResult = GCAllocateMemoryFromHeap(sg_psAppHeap,
									   u32Size,
									   &pvMemoryHeap,
									   &sMemoryInfo);

//	CheckHeapIntegrity(sg_psAppHeap);

	GCHeapCheck();

	if (eResult != GC_OK)
	{
		return(NULL);
	}
	else
	{
		return(pvMemoryHeap);
	}
#endif
}

EGCResultCode GCGetBlockSize(void *pvBlock,
							 UINT32 *pu32BlockSize)
{
#ifndef _MYHEAP
	GCASSERT(0);
	return(GC_OK);
#else
	return(GCGetBlockSizeFromHeap(sg_psAppHeap,
								  pvBlock,
								  pu32BlockSize));
#endif
}

void GCGetFreeInfo(UINT32 *pu32LargestFree,
				   UINT32 *pu32TotalFree)
{
#ifndef _MYHEAP
	GCASSERT(0);
#else
	GCGetFreeInfoFromHeap(sg_psAppHeap,
						  pu32LargestFree,
						  pu32TotalFree);
#endif
}

void GCGetAllocInfo(UINT32 *pu32LargestAlloc,
				    UINT32 *pu32TotalAlloc)
{
#ifndef _MYHEAP
	GCASSERT(0);
#else
	GCGetAllocInfoFromHeap(sg_psAppHeap,
						   pu32LargestAlloc,
						   pu32TotalAlloc);
#endif
 }

void *GCReallocMemoryInternal(void *pvOriginalPointer,
							  UINT32 u32NewSize,
							  UINT8 *pu8ModuleName,
							  UINT32 u32LineNumber)
{
#ifndef _MYHEAP
	return(realloc(pvOriginalPointer, u32NewSize));
#else
	EGCResultCode eResult;
	SMemoryInfo sMemoryInfo;

	sMemoryInfo.pu8ModuleName = pu8ModuleName;
	sMemoryInfo.u32LineNumber = u32LineNumber;
	sMemoryInfo.bClearBlock = FALSE;

	eResult = GCReallocMemoryFromHeap(sg_psAppHeap,
									  u32NewSize,
									  &pvOriginalPointer,
									  &sMemoryInfo);

	if (eResult != GC_OK)
	{
		return(NULL);
	}
	else
	{
		return(pvOriginalPointer);
	}
#endif
}

void GCFreeMemory(void *pvMemoryLocation)
{
#ifndef _MYHEAP
	free(pvMemoryLocation);
#else
	GCFreeMemoryFromHeap(sg_psAppHeap,
			    		 &pvMemoryLocation);
//	CheckHeapIntegrity(sg_psAppHeap);
#endif
}

EGCResultCode BIOSHeapDropAnchor(void)
{
	return(SDRAMDropAnchor());
}

EGCResultCode BIOSHeapRaiseAnchor(void)
{
	return(SDRAMRaiseAnchor());
}

EGCResultCode AppHeapDropAnchor(void)
{
	return(HeapDropAnchor(sg_psAppHeap));
}

EGCResultCode AppHeapRaiseAnchor(void)
{
	return(HeapRaiseAnchor(sg_psAppHeap));
}

void GCCheckHeapIntegrity(void)
{
	CheckHeapIntegrity(sg_psAppHeap);
}

void GCStartup(UINT32 u32FreeMemoryBase,
			   UINT32 u32Size,
			   UINT8 *pu8CmdLine,
			   SCommandLineOption *psOptions)
{
#ifdef _MYHEAP
	EGCResultCode eResultCode;
#endif
	UINT32 u32Overage = (u32FreeMemoryBase + 0x1f) & ~(0x1f);
	UINT32 u32ProductID = 0;

	GCSetRestartOverride(TRUE);

#ifndef _WIN32
	// Force linkage of the signature...
	sg_sAppSignature.u8Length = sizeof(sg_sAppSignature);
#endif // #ifndef _WIN32

#ifndef _MYHEAP
	// Don't create a heap if we're not in debug mode
#else
	u32Size -= 0x20;
	u32FreeMemoryBase = u32Overage;

	// Set up the memory heap
	eResultCode = GCCreateMemoryHeap(u32Size,
								     (SMemoryHeap *) u32FreeMemoryBase,
								     (UINT8 *) "AppHeap");

	GCASSERT(GC_OK == eResultCode);
	sg_psAppHeap = (SMemoryHeap *) u32FreeMemoryBase;
	HeapSetHeaderGuards(sg_psAppHeap,
						FALSE);

	// Call the profiler
	ProfileInit();

	// Tell the BIOS where the user heap is
	GCSetUserHeap(sg_psAppHeap);

	SDRAMInit((void *) u32FreeMemoryBase,
			  u32Size);
#endif

	// Init command line options
	if (FALSE == CmdLineInit(pu8CmdLine,
							 psOptions,
							 "MRTClient.exe"))
	{
		exit(1);
	}

	// Go check the command line options
	CmdLineOptionCheck("MRTClient.exe");

	// Call the profiler
	ProfileInit();

	DebugOut("Executing AppMain()\n");

	AppMain(pu8CmdLine);
}

