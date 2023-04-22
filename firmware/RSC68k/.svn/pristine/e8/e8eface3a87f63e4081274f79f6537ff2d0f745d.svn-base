#include <string.h>
#include "Startup/app.h"
#include "Include/ram.h"
#include "win32/host.h"

#define FILLER_VALUE		0x12345678

static const UINT8 sg_u8HeaderGuard[] =
{
	0xaa, 0x55, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66,
	0xff, 0xee, 0xdd, 0xcc, 0xbb, 0x99, 0x88, 0x77
};

#define BLOCK_ALIGNMENT		0xf	// Must be 4 byte aligned

#define BLOCK_GUARDS		0x5a82bc9d		// Block has header guards
#define BLOCK_NO_GUARDS		0x7ead36b1		// Block doesn't have header guards
#define BLOCK_BOGUS			0x00000000		// Block is bogus

static CRITICAL_SECTION sg_sRAMCriticalSection;

void GOSPlatformInit(void)
{
	InitializeCriticalSection(&sg_sRAMCriticalSection);
}

static GOSCPUIntContext GOSPlatformCriticalSectionEnter(void)
{
	EnterCriticalSection(&sg_sRAMCriticalSection);
	return(0);
}

static void GOSPlatformCriticalSectionExit(GOSCPUIntContext eCPUContext)
{
	(void) eCPUContext;
	LeaveCriticalSection(&sg_sRAMCriticalSection);
}


static void CheckHeaderGuards(SBlock *psBlock)
{
	UINT8 *pu8CheckArea = NULL;
	BOOL bFailed = FALSE;

	if (psBlock->psFreeNext || psBlock->psFreePrior)
	{
		GCASSERT(BLOCK_BOGUS == psBlock->u32Tag);

		// Don't check header guards on a free block
		return;
	}

	if (BLOCK_NO_GUARDS == psBlock->u32Tag)
	{
		// Don't check header guards on a block that doesn't have them
		return;
	}

	if (psBlock->u32Tag != BLOCK_GUARDS)
	{
		char string[500];
		// This means the tag is bad. Tell the user

		sprintf(string, "\nBlock @ 0x%.8x - invalid block tag\nValue = 0x%.8x\nIndicates prior allocated block\noverwrote this one", psBlock, psBlock->u32Tag);
		GCASSERT_MSG(string);
	}

	// Check the header

	pu8CheckArea = ((UINT8 *) psBlock->pvUserBase) - sizeof(sg_u8HeaderGuard);

	if (memcmp(pu8CheckArea, (void *) sg_u8HeaderGuard, sizeof(sg_u8HeaderGuard)))
	{
		DebugOut("Header guard corrupt @ 0x%.8x.\n", pu8CheckArea);

		if (psBlock->psPrior)
		{
			DebugOut("Previous block most likely the culprit.\n");
		}
		else
		{
			DebugOut("No previous block (first in heap)\n");
		}

		DebugOut("\nCorrupt block info (may be bogus):\n");
		DebugOut("   Module name: %s\n", psBlock->pu8Module);
		DebugOut("   Line #     : %d\n", psBlock->u32Line);
		DebugOut("   Size       : %d\n\n", psBlock->u32BlockSize);

		if (psBlock->psPrior)
		{
			DebugOut("Previous block info:\n");
			DebugOut("   Module name: %s\n", psBlock->psPrior->pu8Module);
			DebugOut("   Line #     : %d\n", psBlock->psPrior->u32Line);
			DebugOut("   Size       : %d\n\n", psBlock->psPrior->u32BlockSize);
		}

		// Expected: aa 55 11 22 33 44 55 66 
		//			 ff ee dd cc bb 99 88 77
		// Got     : 22 5a 32 22 33 44 55 66 
		//			 ff ee dd cc bb 99 88 77

		bFailed = TRUE;
	}

	// Now check the footer

	if (FALSE == bFailed)
	{
		pu8CheckArea += (psBlock->u32BlockSize) - sizeof(sg_u8HeaderGuard);

		if (memcmp(pu8CheckArea, (void *) sg_u8HeaderGuard, sizeof(sg_u8HeaderGuard)))
		{
			// They don't match

			bFailed = TRUE;

			// Set it up for 320x240 (ignore the result code)

			DebugOut("Footer guard corrupt @ 0x%.8x.\n", pu8CheckArea);

			DebugOut("\nCorrupt block info:\n");
			DebugOut("   Module name: %s\n", psBlock->psPrior->pu8Module);
			DebugOut("   Line #     : %d\n", psBlock->psPrior->u32Line);
			DebugOut("   Size       : %d\n\n", psBlock->psPrior->u32BlockSize);
		}
	}

	if (bFailed)
	{
		DebugOut("Expected: %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x\n",
						sg_u8HeaderGuard[0],
						sg_u8HeaderGuard[1],
						sg_u8HeaderGuard[2],
						sg_u8HeaderGuard[3],
						sg_u8HeaderGuard[4],
						sg_u8HeaderGuard[5],
						sg_u8HeaderGuard[6],
						sg_u8HeaderGuard[7]);
		DebugOut("          %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x\n",
						sg_u8HeaderGuard[8],
						sg_u8HeaderGuard[9],
						sg_u8HeaderGuard[10],
						sg_u8HeaderGuard[11],
						sg_u8HeaderGuard[12],
						sg_u8HeaderGuard[13],
						sg_u8HeaderGuard[14],
						sg_u8HeaderGuard[15]);

		DebugOut("Got     : %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x\n",
						*(pu8CheckArea + 0),
						*(pu8CheckArea + 1),
						*(pu8CheckArea + 2),
						*(pu8CheckArea + 3),
						*(pu8CheckArea + 4),
						*(pu8CheckArea + 5),
						*(pu8CheckArea + 6),
						*(pu8CheckArea + 7)
						);

		DebugOut("        : %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x\n",
						*(pu8CheckArea + 8),
						*(pu8CheckArea + 9),
						*(pu8CheckArea + 10),
						*(pu8CheckArea + 11),
						*(pu8CheckArea + 12),
						*(pu8CheckArea + 13),
						*(pu8CheckArea + 14),
						*(pu8CheckArea + 15)
						);

		while (1);
	}
}

void CheckHeapIntegrity(SMemoryHeap *psMemoryHeap)
{
	SBlock *psBlock = NULL;
	SBlock *psPriorBlock = NULL;
	SBlock *psComputedNext = NULL;
	GOSCPUIntContext eIntCtx;
	
#ifdef _WIN32
	eIntCtx = GOSPlatformCriticalSectionEnter();
#endif // _WIN32

	// Let's start by doing a next/prior check

	psBlock = psMemoryHeap->psBlock;

	while (psBlock)
	{
		psComputedNext = (SBlock *) ((UINT8 *) (psBlock + 1) + psBlock->u32BlockSize);

		if ((psComputedNext != psBlock->psNext) && (psBlock->psNext))
		{
			GCASSERT_MSG("Computed next != linked next");
		}

		if (psBlock->psPrior != psPriorBlock)
		{
			GCASSERT_MSG("Computed prior != linked prior");
		}

		if (psBlock->psAllocNext || psBlock->psAllocPrior)
		{
			CheckHeaderGuards(psBlock);
			GCASSERT(NULL == psBlock->psFreeNext);
			GCASSERT(NULL == psBlock->psFreePrior);
		}

		if (psBlock->psFreeNext || psBlock->psFreePrior)
		{
			CheckHeaderGuards(psBlock);
			GCASSERT(NULL == psBlock->psAllocNext);
			GCASSERT(NULL == psBlock->psAllocPrior);
		}

		psPriorBlock = psBlock;
		psBlock = psBlock->psNext;
	}

	// Okay, next/prior links work. Let's see if ALLOC looks good

	psBlock = psMemoryHeap->psAllocBlock;
	psPriorBlock = NULL;

	while (psBlock)
	{
		if (psBlock->psAllocPrior != psPriorBlock)
		{
			GCASSERT(0);
		}

		psPriorBlock = psBlock;
		psBlock = psBlock->psAllocNext;
	}

	// Okay, next/prior links work. Let's see if FREE looks good

	psBlock = psMemoryHeap->psFreeBlock;
	psPriorBlock = NULL;

	while (psBlock)
	{
		if (psBlock->psFreePrior != psPriorBlock)
		{
			GCASSERT(0);
		}

		psPriorBlock = psBlock;
		psBlock = psBlock->psFreeNext;
	}
#ifdef _WIN32
	// Restore interrupts
	GOSPlatformCriticalSectionExit(eIntCtx);
#endif
}

EGCResultCode CreateMemoryHeap(UINT32 u32Size,
							   SMemoryHeap *psMemoryHeap,
							   UINT8 *pu8HeapName)
{
	UINT32 u32Adjust;
	SBlock *psBlock;
	GOSCPUIntContext eIntCtx;

	// Ensure we have the RAM stuff initialized properly

#ifdef _WIN32
	eIntCtx = GOSPlatformCriticalSectionEnter();
#endif // _WIN32

	// First things first - let's see if it's aligned to our alignment requirements.
	// Assert - hardcore - if not

	u32Adjust = ((UINT32) psMemoryHeap) & BLOCK_ALIGNMENT;
	GCASSERT(0 == u32Adjust);

	// Align our size to a block alignment size

	u32Size = (u32Size + BLOCK_ALIGNMENT) & ~(BLOCK_ALIGNMENT);

	// Let's make sure the heap is big enough. Must be large enough to cover at least
	// one block the size of our block alignment size

	GCASSERT(u32Size >= (sizeof(*psMemoryHeap) + sizeof(SBlock) + (BLOCK_ALIGNMENT + 1)));

	// Let's start setting things up!

	// First, create the heap information header

	memset((void *) psMemoryHeap, 0, sizeof(*psMemoryHeap));
	u32Size -= sizeof(*psMemoryHeap);
	psMemoryHeap->u32HeapSize = u32Size;

	// Second, create the first block - skip past the memory heap pointer

	psBlock = (SBlock *) (psMemoryHeap + 1);
	memset((void *) psBlock, 0, sizeof(*psBlock));

	// Let's start filling in information - in the block first

	psBlock->u32BlockSize = u32Size - sizeof(*psBlock);
	psBlock->u32Tag = BLOCK_NO_GUARDS;

	// Now fill out the information in the block header

	psMemoryHeap->pu8HeapName = pu8HeapName;
	psMemoryHeap->psFreeBlock = psBlock;			// The block we have is a free block
	psMemoryHeap->psBlock = psBlock;				// First block in the list is this
	psMemoryHeap->u32FreeBlockCount = 1;			// Start off with one free block
	psMemoryHeap->u32FreeSize = u32Size;

#ifdef _WIN32
	// Restore interrupts
	GOSPlatformCriticalSectionExit(eIntCtx);
#endif
	return(GC_OK);	
}

#ifdef _WIN32
typedef struct SRequestSize
{
	UINT32 u32Size;
	UINT32 u32Count;
	struct SRequestSize *psNextLink;
} SRequestSize;

static SRequestSize *sg_psRequestSizeHead = NULL;

static void RequestSizeRecord(UINT32 u32Size)
{
	SRequestSize *psRq = sg_psRequestSizeHead;

	while (psRq)
	{
		if (psRq->u32Size == u32Size)
		{
			psRq->u32Count++;
			return;
		}

		psRq = psRq->psNextLink;
	}

	psRq = malloc(sizeof(*psRq));
	GCASSERT(psRq);
	psRq->u32Size = u32Size;
	psRq->u32Count = 1;
	psRq->psNextLink = sg_psRequestSizeHead;
	sg_psRequestSizeHead = psRq;
}

void HeapStats(FILE *fp)
{
	SRequestSize *psRq = sg_psRequestSizeHead;

	fprintf(fp, "\nCount    Size\n----    -----\n");

	while (psRq)
	{
		fprintf(fp, "%-7u  %-6u%\n", psRq->u32Count, psRq->u32Size);
		psRq = psRq->psNextLink;
	}
}

#endif // #ifdef _WIN32

#define		BLOCK_OVERHEAD_ADDER		((sizeof(SBlock) + (BLOCK_ALIGNMENT + 1)) & ~(BLOCK_ALIGNMENT))

static UINT32 sg_u32MaxSearchDepth = 0;

EGCResultCode AllocateMemoryInternal(SMemoryHeap *psMemoryHeap,
					  UINT32 u32Size,
					  void **ppvMemoryBlock,
					  UINT8 *pu8ModuleName,
					  UINT32 u32LineNumber,
					  BOOL bClearBlock)
{
	UINT32 u32AdjustedAllocSize;
	SBlock *psFreeBlock = NULL;
	SBlock *psNewBlock = NULL;
	SBlock *psBestBlock = NULL;
	UINT32 u32LargestFit = 0xffffffff;
	GOSCPUIntContext eIntCtx;
	UINT32 u32SearchDepth = 0;

	u32Size = (u32Size + BLOCK_ALIGNMENT) & ~(BLOCK_ALIGNMENT);

	// Can't allocate a block of 0 bytes

	if (0 == u32Size)
	{
		return(GC_INVALID_MEMORY_ALLOC_SIZE);
	}

	// Adjust the allocated size depending upon our allocation scheme

	u32AdjustedAllocSize = u32Size;

	// If we have header guards enabled, turn it on

	if (psMemoryHeap->bGuardsEnabled)
	{
		u32AdjustedAllocSize += sizeof(sg_u8HeaderGuard) + sizeof(sg_u8HeaderGuard);
	}

	// Now we have our adjusted/aligned allocation size

	u32AdjustedAllocSize = (u32AdjustedAllocSize + BLOCK_ALIGNMENT) & ~(BLOCK_ALIGNMENT);

	// Let's run through our free list and figure out where to put things

#ifdef _WIN32
	eIntCtx = GOSPlatformCriticalSectionEnter();

	RequestSizeRecord(u32Size);
#endif // _WIN32

	psFreeBlock = psMemoryHeap->psFreeBlock;

	while (psFreeBlock)
	{
		if (psFreeBlock->u32BlockSize == u32AdjustedAllocSize)
		{
			// This means we have an EXACT fit, so break!

			psBestBlock = psFreeBlock;
			psNewBlock = psBestBlock;
			break;
		}

		if (psFreeBlock->u32BlockSize >= (u32AdjustedAllocSize + BLOCK_OVERHEAD_ADDER))
		{
			if (psFreeBlock->u32BlockSize < u32LargestFit)
			{
				u32LargestFit = psFreeBlock->u32BlockSize;
				psBestBlock = psFreeBlock;
			}
//			break;
		}

//		u32SearchDepth++;
		psFreeBlock = psFreeBlock->psFreeNext;
	}

	if (u32SearchDepth > sg_u32MaxSearchDepth)
	{
		sg_u32MaxSearchDepth = u32SearchDepth;
		DebugOut("%s: Max depth=%u\n", __FUNCTION__, u32SearchDepth);;
	}

	// If we're out of memory, BAIL

	if (NULL == psBestBlock)
	{
#ifdef _WIN32
		// Restore interrupts
		GOSPlatformCriticalSectionExit(eIntCtx);
#endif
		return(GC_OUT_OF_MEMORY);
	}

	psFreeBlock = psBestBlock;

	// If we didn't find an exact fit, let's split the block, because we know we have room

	if (NULL == psNewBlock)
	{
		// Split the free block. At the front of the block, let's put the new allocation

		psNewBlock = psFreeBlock;

		psFreeBlock = (SBlock *) ((UINT8 *) (psNewBlock + 1) + u32AdjustedAllocSize);
		memset((void *) psFreeBlock, 0, sizeof(*psFreeBlock));

		// Give the new (reduced) size of the allocated (split) block
		psFreeBlock->u32BlockSize = psNewBlock->u32BlockSize - u32AdjustedAllocSize - sizeof(*psFreeBlock);

		// Unlink the new allocated block from the free list
		psFreeBlock->psFreePrior = psNewBlock->psFreePrior;
		psFreeBlock->psFreeNext = psNewBlock->psFreeNext;

		// If the prior link is the head of the list, attach it to the heap

		if (NULL == psFreeBlock->psFreePrior)
		{
			psMemoryHeap->psFreeBlock = psFreeBlock;
		}
		else
		{
			psFreeBlock->psFreePrior->psFreeNext = psFreeBlock;
		}

		if (psFreeBlock->psFreeNext)
		{
			psFreeBlock->psFreeNext->psFreePrior = psFreeBlock;
		}

		// Now link the next and prior pointers
		psFreeBlock->psPrior = psNewBlock;
		psFreeBlock->psNext = psNewBlock->psNext;

		if (psFreeBlock->psNext)
		{
			psFreeBlock->psNext->psPrior = psFreeBlock;
		}

		// Tag this free block
		psFreeBlock->u32Tag = BLOCK_BOGUS;

		// Now the new block. Let's hook it up.
		psNewBlock->psNext = psFreeBlock;

		// Let's make sure it's not part of the free list
		psNewBlock->psFreePrior = NULL;
		psNewBlock->psFreeNext = NULL;

	}
	else
	{
		// The block is an exact fit. No need to split.

		psMemoryHeap->u32FreeBlockCount++;

		// Unlink the current block from the free list

		if (psNewBlock->psFreeNext)
		{
			psNewBlock->psFreeNext->psFreePrior = psNewBlock->psFreePrior;
		}

		if (psNewBlock->psFreePrior)
		{
			psNewBlock->psFreePrior->psFreeNext = psNewBlock->psFreeNext;
		}
		else
		{
			// New block is at the head of the list
			psMemoryHeap->psFreeBlock = psNewBlock->psFreeNext;
		}

		psNewBlock->psFreePrior = NULL;
		psNewBlock->psFreeNext = NULL;
	}

	// But now we need to make sure it's part of the allocated list
	psNewBlock->psAllocPrior = NULL;
	psNewBlock->psAllocNext = psMemoryHeap->psAllocBlock;
	psMemoryHeap->psAllocBlock = psNewBlock;

	// If AllocNext is non-NULL, assign its prior pointer to this new block

	if (psNewBlock->psAllocNext)
	{
		psNewBlock->psAllocNext->psAllocPrior = psNewBlock;
	}

	// Copy in header guards if need be.

	if (psMemoryHeap->bGuardsEnabled)
	{
		psNewBlock->u32Tag = BLOCK_GUARDS;
		psNewBlock->pvUserBase = (void *) (((UINT8 *) (psNewBlock + 1)) + sizeof(sg_u8HeaderGuard));
		memcpy((void *) (psNewBlock + 1), (void *) sg_u8HeaderGuard, sizeof(sg_u8HeaderGuard));
		memcpy((void *) ((UINT8 *) (psNewBlock + 1) + u32AdjustedAllocSize - sizeof(sg_u8HeaderGuard)), (void *) sg_u8HeaderGuard, sizeof(sg_u8HeaderGuard));
	}
	else
	{
		psNewBlock->u32Tag = BLOCK_NO_GUARDS;
		psNewBlock->pvUserBase = (void *) (psNewBlock + 1);
	}

	psNewBlock->u32BlockSize = u32AdjustedAllocSize;
	psNewBlock->pu8Module = pu8ModuleName;
	psNewBlock->u32Line = u32LineNumber;

	*ppvMemoryBlock = psNewBlock->pvUserBase;

	if (bClearBlock)
	{
		if (BLOCK_NO_GUARDS == psNewBlock->u32Tag)
		{
			memset(psNewBlock->pvUserBase, 0, psNewBlock->u32BlockSize);
		}
		else
		if (BLOCK_GUARDS == psNewBlock->u32Tag)
		{
			memset(psNewBlock->pvUserBase, 0, psNewBlock->u32BlockSize - (sizeof(sg_u8HeaderGuard) << 1));
		}
		else
		{
			// Bogus guard types?
			GCASSERT(0);
		}
	}

	psMemoryHeap->u32AllocatedSize += psNewBlock->u32BlockSize + sizeof(*psNewBlock);
	psMemoryHeap->u32FreeSize -= (psNewBlock->u32BlockSize + sizeof(*psNewBlock));
	psMemoryHeap->u32AllocBlockCount++;
#ifdef _WIN32
	// Restore interrupts
	GOSPlatformCriticalSectionExit(eIntCtx);
#endif

	return(GC_OK);
}

EGCResultCode ReallocMemoryInternal(SMemoryHeap *psMemoryHeap,
									UINT32 u32Size,
									void **ppvOriginalBlock,
									UINT8 *pu8ModuleName,
									UINT32 u32LineNumber)
{
	SBlock *psOriginalBlock = NULL;
	EGCResultCode eResult;
	void *pvNewBlock = NULL;
	GOSCPUIntContext eIntCtx;

	// Let's find the original block in the heap

#ifdef _WIN32
	// Restore interrupts
	eIntCtx = GOSPlatformCriticalSectionEnter();
#endif

	psOriginalBlock = psMemoryHeap->psAllocBlock;

	while (psOriginalBlock && psOriginalBlock->pvUserBase != *ppvOriginalBlock)
	{
		psOriginalBlock = psOriginalBlock->psAllocNext;
	}

#ifdef _WIN32
	// Restore interrupts
	GOSPlatformCriticalSectionExit(eIntCtx);
#endif

	// If psOriginalBlock is NULL, we haven't found the original block
	GCASSERT(psOriginalBlock);

	// Got it! Let's allocate new space

	eResult = AllocateMemoryInternal(psMemoryHeap,
									 u32Size,
									 &pvNewBlock,
									 pu8ModuleName,
									 u32LineNumber,
									 FALSE);

	if (eResult != GC_OK)
	{
		return(eResult);
	}

	// Now let's copy from the old space to the new space.

	memcpy(pvNewBlock,
		   psOriginalBlock->pvUserBase,
		   u32Size);

	GCFreeMemoryFromHeap(psMemoryHeap,
						 ppvOriginalBlock);

	*ppvOriginalBlock = pvNewBlock;
	return(GC_OK);
}

EGCResultCode FreeMemory(SMemoryHeap *psMemoryHeap,
				  void **vppMemoryToFree)
{
	SBlock *psBlockToFree = NULL;
	BOOL bPriorFree = FALSE;
	BOOL bNextFree = FALSE;
	GOSCPUIntContext eIntCtx;

#ifdef _WIN32
	eIntCtx = GOSPlatformCriticalSectionEnter();
#endif // _WIN32

	// Let's see if we can just see our allocated block if we back up (assuming no header guards)

	psBlockToFree = ((SBlock *) ((SBlock *) *vppMemoryToFree)) - 1;

	if ((BLOCK_NO_GUARDS == psBlockToFree->u32Tag) && (psBlockToFree->pvUserBase == *vppMemoryToFree))
	{
		psBlockToFree = psBlockToFree;
		// We found our block
	}
	else
	{
		psBlockToFree = ((SBlock *) (((UINT8 *) *vppMemoryToFree) - sizeof(sg_u8HeaderGuard))) - 1;
		if ((BLOCK_GUARDS == psBlockToFree->u32Tag) && (psBlockToFree->pvUserBase == *vppMemoryToFree))
		{
			// We found our block, with a guard
		}
		else
		{
			// Scan our heap's allocated list and see if that block is there

			while (psBlockToFree && psBlockToFree->pvUserBase != *vppMemoryToFree)
			{
				psBlockToFree = psBlockToFree->psAllocNext;
			}
		}
	}

	// If this is NULL, then we didn't find it in the allocated list

	if (NULL == psBlockToFree)
	{
#ifdef _WIN32
		// Restore interrupts
		GOSPlatformCriticalSectionExit(eIntCtx);
#endif
		DebugOut("%s: Attempted to free block not in heap @ 0x%.8x\n", __FUNCTION__, *((UINT32 *) vppMemoryToFree));
		GCASSERT(0);
		return(GC_BLOCK_NOT_IN_HEAP);
	}

	// Let's make sure we have proper integrity
	
	if (psBlockToFree->psNext && psBlockToFree->psNext->psPrior != psBlockToFree)
	{
		// This means that the next pointer doesn't point back to this guy
		// Likely means that our quick scan above didn't work.
		GCASSERT(0);
	}

	if (psBlockToFree->psPrior && psBlockToFree->psPrior->psNext != psBlockToFree)
	{
		// This means that the prior pointer doesn't point back to this block.
		// Likely means that our quick scan above didn't work.
		GCASSERT(0);
	}

	// If this block has header and footer guards, let's do a comparison

	CheckHeaderGuards(psBlockToFree);

	// We found it. Let's unlink it from the allocated list

	if (psBlockToFree->psAllocPrior)
	{
		psBlockToFree->psAllocPrior->psAllocNext = psBlockToFree->psAllocNext;
	}
	else
	{
		// It's the head of the list

		psMemoryHeap->psAllocBlock = psBlockToFree->psAllocNext;
	}

	if (psBlockToFree->psAllocNext)
	{
		psBlockToFree->psAllocNext->psAllocPrior = psBlockToFree->psAllocPrior;
	}

	psBlockToFree->psAllocPrior = NULL;
	psBlockToFree->psAllocNext = NULL;
	psBlockToFree->pvUserBase = NULL;
	psBlockToFree->u32Tag = BLOCK_BOGUS;

	// It's unlinked from the allocated list. Let's figure out what we need to do

	bPriorFree = FALSE;
	bNextFree = FALSE;

	// See if the prior block is a free block

	if ((psBlockToFree->psPrior) && (NULL == psBlockToFree->psPrior->psAllocNext) && 
		(NULL == psBlockToFree->psPrior->psAllocPrior) && (psBlockToFree->psPrior != psMemoryHeap->psAllocBlock))
	{
		bPriorFree = TRUE;
	}

	if ((psBlockToFree->psNext) && (NULL == psBlockToFree->psNext->psAllocNext) && 
		(NULL == psBlockToFree->psNext->psAllocPrior) && (psBlockToFree->psNext != psMemoryHeap->psAllocBlock))
	{
		bNextFree = TRUE;
	}
	
	if ((bNextFree) && (bPriorFree))
	{
		SBlock *psNextFree = psBlockToFree->psNext;
		SBlock *psPriorFree = psBlockToFree->psPrior;

		GCASSERT(psNextFree);

		GCASSERT(psNextFree->psFreeNext || psNextFree->psFreePrior);
		GCASSERT(psPriorFree->psFreeNext || psPriorFree->psFreePrior);
		GCASSERT(psBlockToFree->psNext->psFreeNext || psBlockToFree->psNext->psFreePrior);

		// Let's first unlink the "next" pointer block from the free list

		if (psNextFree->psFreePrior)
		{
			psNextFree->psFreePrior->psFreeNext = psNextFree->psFreeNext;
		}
		else
		{
			psMemoryHeap->psFreeBlock = psNextFree->psFreeNext;
		}

		// Now the next pointer

		if (psNextFree->psFreeNext)
		{
			psNextFree->psFreeNext->psFreePrior = psNextFree->psFreePrior;
		}

		// Now link the next/prior blocks

		psPriorFree->psNext = psNextFree->psNext;

		if (psNextFree->psNext)
		{
			psNextFree->psNext->psPrior = psPriorFree;
		}

		psBlockToFree->psPrior->u32BlockSize += psBlockToFree->u32BlockSize + sizeof(*psBlockToFree) +
												psNextFree->u32BlockSize + sizeof(*psBlockToFree);
	}
	else
	if (bPriorFree)
	{
		// Prior block is free. Combine the freed block and prior block

		// It's now unlinked. Let's update the prior/next pointers

		GCASSERT(psBlockToFree->psPrior);
		psBlockToFree->psPrior->psNext = psBlockToFree->psNext;

		if (psBlockToFree->psNext)
		{
			psBlockToFree->psNext->psPrior = psBlockToFree->psPrior;
		}

		// The block is completely out of it now. Let's increase the size of the
		// prior block

		psBlockToFree->psPrior->u32BlockSize += psBlockToFree->u32BlockSize + sizeof(*psBlockToFree);
	}
	else
	if (bNextFree)
	{
		SBlock *psNextFree = psBlockToFree->psNext;
		GCASSERT(psNextFree);

		psNextFree->pvUserBase = NULL;

		// Handle the prior pointer first

		if (psNextFree->psFreePrior)
		{
			psNextFree->psFreePrior->psFreeNext = psNextFree->psFreeNext;
		}
		else
		{
			psMemoryHeap->psFreeBlock = psNextFree->psFreeNext;
		}

		// Now the next pointer

		if (psNextFree->psFreeNext)
		{
			psNextFree->psFreeNext->psFreePrior = psNextFree->psFreePrior;
		}

		// Next, link the user given free block to the free list

		psBlockToFree->psFreeNext = psMemoryHeap->psFreeBlock;
		if (psMemoryHeap->psFreeBlock)
		{
			GCASSERT(NULL == psMemoryHeap->psFreeBlock->psFreePrior);
			psMemoryHeap->psFreeBlock->psFreePrior = psBlockToFree;
		}

		psBlockToFree->psFreePrior = NULL;
		psMemoryHeap->psFreeBlock = psBlockToFree;

		// Now link the next/prior stuff so it skips over the next free block

		if (psNextFree->psNext)
		{
			psNextFree->psNext->psPrior = psBlockToFree;
		}

		psBlockToFree->psNext = psNextFree->psNext;

		// Last, increase the size of the free block to cover both blocks

		psBlockToFree->u32BlockSize += psNextFree->u32BlockSize + sizeof(*psNextFree);
	}
	else
	{
		// Neither adjacent block is free. Just link this block to the free list and be done with it.

		psBlockToFree->psFreeNext = psMemoryHeap->psFreeBlock;
		if (psMemoryHeap->psFreeBlock)
		{
			GCASSERT(NULL == psMemoryHeap->psFreeBlock->psFreePrior);
			psMemoryHeap->psFreeBlock->psFreePrior = psBlockToFree;
		}

		psBlockToFree->psFreePrior = NULL;
		psMemoryHeap->psFreeBlock = psBlockToFree;
		bNextFree = TRUE;
		psBlockToFree->pu8Module = NULL;
		psBlockToFree->u32Line = 0;
	}

	GCASSERT(NULL == psBlockToFree->psAllocNext);
	GCASSERT(NULL == psBlockToFree->psAllocPrior);

#ifdef _WIN32
	// Restore interrupts
	GOSPlatformCriticalSectionExit(eIntCtx);
#endif

	return(GC_OK);
}

EGCResultCode HeapDropAnchor(SMemoryHeap *psMemoryHeap)
{
	SBlock *psBlock;
	UINT32 *pu32Allocated = NULL;
	UINT32 u32AllocCount = 0;

	if (psMemoryHeap->vpAnchor)
	{
		GCASSERT_MSG("Anchor already dropped for this heap");
	}

	psBlock = psMemoryHeap->psAllocBlock;

	while (psBlock)
	{
		u32AllocCount++;
		psBlock = psBlock->psAllocNext;
	}

	if (u32AllocCount)
	{
		pu32Allocated = GCAllocateMemory(u32AllocCount * sizeof(UINT32));
		GCASSERT(pu32Allocated);
		psMemoryHeap->vpAnchor = (void *) pu32Allocated;

		psBlock = psMemoryHeap->psAllocBlock;
		u32AllocCount = 0;

		while (psBlock)
		{
			*(pu32Allocated + u32AllocCount) = (UINT32) psBlock;
			psBlock = psBlock->psAllocNext;
			u32AllocCount++;
		}

		psMemoryHeap->u32AnchorItems = u32AllocCount;
	}

	return(GC_OK);
}

EGCResultCode HeapRaiseAnchor(SMemoryHeap *psMemoryHeap)
{
	SBlock *psBlock = NULL;
	SBlock **ppsAnchorBlock = NULL;
	UINT32 u32Leaks = 0;
	UINT32 u32Loop = 0;

	if (NULL == psMemoryHeap->vpAnchor)
	{
		GCASSERT_MSG("Anchor not previously dropped for this heap");
	}

	psBlock = psMemoryHeap->psAllocBlock;

	while (psBlock)
	{
		// Let's see if this psBlock is in the heap anywhere.

		ppsAnchorBlock = (SBlock **) psMemoryHeap->vpAnchor;
		u32Loop = 0;

		while (u32Loop < psMemoryHeap->u32AnchorItems)
		{
			if ((*ppsAnchorBlock == psMemoryHeap->vpAnchor) ||
				(*ppsAnchorBlock == psBlock))
			{
				break;
			}

			u32Loop++;
			++ppsAnchorBlock;
		}

		if (psMemoryHeap->u32AnchorItems == u32Loop)
		{
			// We've got problems!

			if (0 == u32Leaks)
			{
				DebugOut("Memory leaks in %s:\n", psMemoryHeap->pu8HeapName);
			}
		
			DebugOut("%s: Line %d, %d bytes\n", psBlock->pu8Module, psBlock->u32Line, psBlock->u32BlockSize);
			
			++u32Leaks;
//  		if (u32Leaks > 10)
//  		{
//  			while (1);
//  		}
		}

		psBlock = psBlock->psAllocNext;
	}

	if (u32Leaks)
	{
		while (1);
	}

	GCFreeMemory(psMemoryHeap->vpAnchor);
	psMemoryHeap->u32AnchorItems = 0;
	psMemoryHeap->vpAnchor = NULL;
	return(GC_OK);
}

EGCResultCode GCGetBlockSizeFromHeap(SMemoryHeap *psMemoryHeap,
									 void *pvBlock,
									 UINT32 *pu32BlockSize)
{
	SBlock *psOriginalBlock = NULL;
	EGCResultCode eResult = GC_OK;
	void *pvNewBlock = NULL;
	GOSCPUIntContext eIntCtx;

	// Let's find the original block in the heap

#ifdef _WIN32
	// Restore interrupts
	eIntCtx = GOSPlatformCriticalSectionEnter();
#endif

	psOriginalBlock = psMemoryHeap->psAllocBlock;

	while (psOriginalBlock && psOriginalBlock->pvUserBase != pvBlock)
	{
		psOriginalBlock = psOriginalBlock->psAllocNext;
	}

	if (NULL == psOriginalBlock)
	{
		eResult = GC_BLOCK_NOT_IN_HEAP;		
	}
	else
	{
		if (pu32BlockSize)
		{
			*pu32BlockSize = psOriginalBlock->u32BlockSize;
		}
	}

#ifdef _WIN32
	// Restore interrupts
	GOSPlatformCriticalSectionExit(eIntCtx);
#endif

	return(eResult);
}

void GCGetFreeInfoFromHeap(SMemoryHeap *psMemoryHeap,
						   UINT32 *pu32LargestFree,
						   UINT32 *pu32TotalFree)
{
	SBlock *psBlock = NULL;
	EGCResultCode eResult = GC_OK;
	void *pvNewBlock = NULL;
	GOSCPUIntContext eIntCtx;

	// Let's find the original block in the heap

#ifdef _WIN32
	// Restore interrupts
	eIntCtx = GOSPlatformCriticalSectionEnter();
#endif

	if (pu32LargestFree)
	{
		*pu32LargestFree = 0;
	}

	if (pu32TotalFree)
	{
		*pu32TotalFree = 0;;
	}

	psBlock = psMemoryHeap->psFreeBlock;

	while (psBlock)
	{
		if (pu32LargestFree)
		{
			if (psBlock->u32BlockSize > *pu32LargestFree)
			{
				*pu32LargestFree = psBlock->u32BlockSize;
			}
		}

		if (pu32TotalFree)
		{
			(*pu32TotalFree) += psBlock->u32BlockSize;
		}

		psBlock = psBlock->psFreeNext;
	}

#ifdef _WIN32
	// Restore interrupts
	GOSPlatformCriticalSectionExit(eIntCtx);
#endif
}

void GCGetAllocInfoFromHeap(SMemoryHeap *psMemoryHeap,
							UINT32 *pu32LargestAlloc,
							UINT32 *pu32TotalAlloc)
{
	SBlock *psBlock = NULL;
	EGCResultCode eResult = GC_OK;
	void *pvNewBlock = NULL;
	GOSCPUIntContext eIntCtx;

	// Let's find the original block in the heap

#ifdef _WIN32
	// Restore interrupts
	eIntCtx = GOSPlatformCriticalSectionEnter();
#endif

	if (pu32LargestAlloc)
	{
		*pu32LargestAlloc = 0;
	}

	if (pu32TotalAlloc)
	{
		*pu32TotalAlloc = 0;
	}

	psBlock = psMemoryHeap->psAllocBlock;

	while (psBlock)
	{
		if (pu32LargestAlloc)
		{
			if (psBlock->u32BlockSize > *pu32LargestAlloc)
			{
				*pu32LargestAlloc = psBlock->u32BlockSize;
			}
		}

		if (pu32TotalAlloc)
		{
			(*pu32TotalAlloc) += psBlock->u32BlockSize;
		}

		psBlock = psBlock->psAllocNext;
	}

#ifdef _WIN32
	// Restore interrupts
	GOSPlatformCriticalSectionExit(eIntCtx);
#endif
}

void HeapSetHeaderGuards(SMemoryHeap *psMemoryHeap,
						 BOOL bGuardsEnabled)
{
	psMemoryHeap->bGuardsEnabled = bGuardsEnabled;
}

static SMemoryHeap *sg_psSDRAMArea;

void SDRAMInit(void *pvSDRAMBase,
			   UINT32 u32SDRAMSize)
{
/*	EGCResultCode eResult;
	
	eResult = CreateMemoryHeap(u32SDRAMSize,
							   (SMemoryHeap *) pvSDRAMBase,
							   (UINT8 *) "BIOS Heap");

	GCASSERT(GC_OK == eResult); */

	sg_psSDRAMArea = (SMemoryHeap *) pvSDRAMBase;
}

void *SDRAMAllocInternal(UINT32 u32Size,
						 UINT8 *pu8ModuleName,
						 UINT32 u32LineNumber)
{
	void *pvMemoryBlock = NULL;

	if (AllocateMemoryInternal(sg_psSDRAMArea,
							   u32Size,
							   &pvMemoryBlock,
							   pu8ModuleName,
							   u32LineNumber,
							   TRUE) != GC_OK)
	{
		return(NULL);
	}

	return(pvMemoryBlock);
}

