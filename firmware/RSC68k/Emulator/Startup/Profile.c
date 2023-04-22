#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Startup/Profile.h"

#define	PROFILER_HEAP_SIZE	8*1024*1024

#define	MAX_STRING	1024

typedef struct SProfileContext
{
	char u8ContextName[MAX_STRING];
	char u8ModuleName[MAX_STRING];
	UINT32 u32LineNumber;
	UINT64 u64StartTimestamp;
	struct SProfileContext *psNextLink;
} SProfileContext;

typedef struct SProfileTotals
{
	char *pu8ContextName;
	UINT64 u64Total;
	
	struct SProfileTotals *psNextLink;
} SProfileTotals;

// Active context stack
static SProfileContext *sg_psActiveContext;

// Totals
static SProfileTotals *sg_psContextTotals;

// Total time
static UINT64 sg_u64TotalTime;

// Level
static UINT32 sg_u32Level;

// # Of ticks per second
static UINT64 sg_u64OneSecondTicks;

// Semaphore
static SOSSemaphore sg_sProfileSemaphore;

static void LockProfiler(void)
{
	EGCResultCode eResult;

	if (GCOSIsRunning())
	{
		eResult = GCOSSemaphoreGet(sg_sProfileSemaphore,
								   0);
		GCASSERT(GC_OK == eResult);
	}
}

static void UnlockProfiler(void)
{
	EGCResultCode eResult;

	if (GCOSIsRunning())
	{
		eResult = GCOSSemaphorePut(sg_sProfileSemaphore);
		GCASSERT(GC_OK == eResult);
	}
}

static void AddTotals(char *pu8ContextName,
					  char *pu8ModuleName,
					  UINT32 u32LineNumber,
					  UINT64 u64Ticks)
{
	SProfileTotals *psPrior = NULL;
	SProfileTotals *psPtr = NULL;

	// Add it to the total time
	sg_u64TotalTime += u64Ticks;

	psPtr = sg_psContextTotals;

	while (psPtr)
	{
		if (strcmp(pu8ContextName,
				   psPtr->pu8ContextName) == 0)
		{
			break;
		}

		psPrior = psPtr;
		psPtr = psPtr->psNextLink;
	}

	// If the item has a prior entry, move it
	if (psPtr && psPrior)
	{
		// Unlink it
		psPrior->psNextLink = psPtr->psNextLink;

		// Move it to the head
		psPtr->psNextLink = sg_psContextTotals;
		sg_psContextTotals = psPtr;
	}
	else
	{
		if (NULL == psPtr)
		{
			// Need a new node
			psPtr = GCAllocateMemory(sizeof(*psPtr));
			GCASSERT(psPtr);
			psPtr->pu8ContextName = GCAllocateMemory(strlen(pu8ContextName) + 1);
			GCASSERT(psPtr->pu8ContextName);
			strcpy(psPtr->pu8ContextName, pu8ContextName);
			psPtr->psNextLink = sg_psContextTotals;
			sg_psContextTotals = psPtr;
		}
		else
		{
			// Otherwise it's already at the beginning of the list
		}
	}

	GCASSERT(psPtr);
	psPtr->u64Total += u64Ticks;
}

void ProfileContextEnter(char *pu8ContextName,
						 char *pu8ModuleName,
						 UINT32 u32LineNumber)
{
	SProfileContext *psProfile;
	UINT64 u64Timestamp;
	EGCResultCode eResult;

	eResult = GCGetPerformanceCounter(0,
									  &u64Timestamp);
	GCASSERT(GC_OK == eResult);

#ifdef PROFILE_WATCH
	DebugOut("%s: Entering '%s' (%u)\n", __FUNCTION__, pu8ContextName, sg_u32Level);
#endif

	psProfile = GCAllocateMemory(sizeof(*psProfile));
	GCASSERT(psProfile);

	strcpy(psProfile->u8ContextName, pu8ContextName);
	strcpy(psProfile->u8ModuleName, pu8ModuleName);

	psProfile->u32LineNumber = u32LineNumber;

	LockProfiler();

	sg_u32Level++;

	// Add elapsed time to total time
	if (sg_psActiveContext)
	{
		u64Timestamp -= sg_psActiveContext->u64StartTimestamp;

		// Total elapsed time for the prior context is in u64Timestamp

		AddTotals(sg_psActiveContext->u8ContextName,
				  pu8ModuleName,
				  u32LineNumber,
				  u64Timestamp);
	}

	psProfile->psNextLink = sg_psActiveContext;
	sg_psActiveContext = psProfile;
	eResult = GCGetPerformanceCounter(0,
									  &psProfile->u64StartTimestamp);
	GCASSERT(GC_OK == eResult);

	UnlockProfiler();
}

void ProfileContextExit(char *pu8ContextName,
						char *pu8ModuleName,
						UINT32 u32LineNumber)
{
	UINT64 u64Timestamp;
	EGCResultCode eResult;
	SProfileContext *psProfile;

	eResult = GCGetPerformanceCounter(0,
									  &u64Timestamp);
	GCASSERT(GC_OK == eResult);

#ifdef PROFILE_WATCH
	DebugOut("%s: Exiting '%s' (%u)\n", __FUNCTION__, pu8ContextName, sg_u32Level - 1);
#endif

	LockProfiler();

	sg_u32Level--;
	psProfile = sg_psActiveContext;
	sg_psActiveContext = psProfile->psNextLink;

	// There had better be an active context
	GCASSERT(psProfile);

	// And it had better match the context name
	if (strcmp(psProfile->u8ContextName,
			   pu8ContextName))
	{
		DebugOut("%s: Expecting '%s', not '%s'\n", __FUNCTION__, psProfile->u8ContextName, pu8ContextName);
		// Imbalanced context
		GCASSERT(0);
	}

	// We're good!
	u64Timestamp -= psProfile->u64StartTimestamp;
	AddTotals(psProfile->u8ContextName,
			  pu8ModuleName,
			  u32LineNumber,
			  u64Timestamp);

	// Get rid of the profiling context/structure
	GCFreeMemory(psProfile);

	if (sg_psActiveContext)
	{
		eResult = GCGetPerformanceCounter(0,
										  &sg_psActiveContext->u64StartTimestamp);
		GCASSERT(GC_OK == eResult);
	}

	UnlockProfiler();
}

// Total time
static UINT64 sg_u64TotalTime;

// Level
static UINT32 sg_u32Level;

// # Of ticks per second
static UINT64 sg_u64OneSecondTicks;


void ProfileReport(char *pu8Filename)
{
	FILE *fp;
	SProfileTotals *psProfile = sg_psContextTotals;
	SProfileTotals *psProfilePrior = NULL;
	UINT32 u32MaxStrlen = 0;
	UINT64 u64Total = 0;
	char u8String[500];
	UINT32 u32Loop = 0;
	BOOL bBubble = TRUE;

	// If there's no profile data, just return
	if (NULL == psProfile)
	{
		return;
	}

	LockProfiler();

	fp = fopen((const char *) pu8Filename, "w");
	GCASSERT(fp);

	fprintf(fp, "Total execution ticks: %llu (%5.6f seconds)\n\nSorted by time\n\n", sg_u64TotalTime, (double) ((double) sg_u64TotalTime / (double) sg_u64OneSecondTicks));

	psProfile = sg_psContextTotals;
	while (psProfile)
	{
		if (strlen(psProfile->pu8ContextName) > u32MaxStrlen)
		{
			u32MaxStrlen = strlen(psProfile->pu8ContextName);
		}

		u64Total += psProfile->u64Total;
		psProfile = psProfile->psNextLink;
	}

	// These should match 100%. If not, it's a bug in the profiler code
	GCASSERT(u64Total == sg_u64TotalTime);
	u32MaxStrlen += 2;

	sprintf(u8String, "%%-%us  %%10.6f   %%5.2f%%%%\n", u32MaxStrlen);

	fprintf(fp, "Context");

	if (u32MaxStrlen < strlen("Context"))
	{
		u32MaxStrlen = strlen("Context");
	}

	u32MaxStrlen -= strlen("Context");
	u32Loop = u32MaxStrlen;
	while (u32Loop--)
	{
		fprintf(fp, " ");
	}

	fprintf(fp,"   Time        Exec %%\n");

	u32Loop = strlen("Context");
	while (u32Loop--)
	{
		fprintf(fp, "-");
	}

	u32Loop = u32MaxStrlen;
	while (u32Loop--)
	{
		fprintf(fp, " ");
	}
	fprintf(fp,"   ----        ------\n");

	// Now go sort it

	while (bBubble)
	{
		psProfile = sg_psContextTotals;
		psProfilePrior = NULL;
		bBubble = FALSE;

		while (psProfile && psProfile->psNextLink)
		{
			if (psProfile->u64Total < psProfile->psNextLink->u64Total)
			{
				// Gotta swap 'em

				// If there's a prior link, fix it
				if (psProfilePrior)
				{
					psProfilePrior->psNextLink = psProfile->psNextLink;
					psProfile->psNextLink = psProfile->psNextLink->psNextLink;
					psProfilePrior->psNextLink->psNextLink = psProfile;

					psProfile = psProfilePrior->psNextLink;
				}
				else
				{
					// Head of the list
					sg_psContextTotals = psProfile->psNextLink;
					psProfile->psNextLink = sg_psContextTotals->psNextLink;
					sg_psContextTotals->psNextLink = psProfile;

					psProfile = sg_psContextTotals;
				}

				bBubble = TRUE;
			}
			else
			{
				psProfilePrior = psProfile;
				psProfile = psProfile->psNextLink;
			}
		}
			
	}

	psProfile = sg_psContextTotals;

	while (psProfile)
	{
		fprintf(fp, u8String, psProfile->pu8ContextName, (double) ((double) psProfile->u64Total / (double) sg_u64OneSecondTicks), (double) (((double) psProfile->u64Total / (double) sg_u64TotalTime) * 100.0)); 
		psProfile = psProfile->psNextLink;
	}

	fprintf(fp, "\n\nSorted by name\n\n");

	fprintf(fp, "Context");

	u32Loop = u32MaxStrlen;
	while (u32Loop--)
	{
		fprintf(fp, " ");
	}

	fprintf(fp,"   Time        Exec %%\n");

	u32Loop = strlen("Context");
	while (u32Loop--)
	{
		fprintf(fp, "-");
	}

	u32Loop = u32MaxStrlen;
	while (u32Loop--)
	{
		fprintf(fp, " ");
	}
	fprintf(fp,"   ----        ------\n");

	bBubble = TRUE;
	while (bBubble)
	{
		psProfile = sg_psContextTotals;
		psProfilePrior = NULL;
		bBubble = FALSE;

		while (psProfile && psProfile->psNextLink)
		{
			if (strcmp(psProfile->pu8ContextName, psProfile->psNextLink->pu8ContextName) > 0)
			{
				// Gotta swap 'em

				// If there's a prior link, fix it
				if (psProfilePrior)
				{
					psProfilePrior->psNextLink = psProfile->psNextLink;
					psProfile->psNextLink = psProfile->psNextLink->psNextLink;
					psProfilePrior->psNextLink->psNextLink = psProfile;

					psProfile = psProfilePrior->psNextLink;
				}
				else
				{
					// Head of the list
					sg_psContextTotals = psProfile->psNextLink;
					psProfile->psNextLink = sg_psContextTotals->psNextLink;
					sg_psContextTotals->psNextLink = psProfile;

					psProfile = sg_psContextTotals;
				}

				bBubble = TRUE;
			}
			else
			{
				psProfilePrior = psProfile;
				psProfile = psProfile->psNextLink;
			}
		}
			
	}

	psProfile = sg_psContextTotals;

	while (psProfile)
	{
		fprintf(fp, u8String, psProfile->pu8ContextName, (double) ((double) psProfile->u64Total / (double) sg_u64OneSecondTicks), (double) (((double) psProfile->u64Total / (double) sg_u64TotalTime) * 100.0)); 
		psProfile = psProfile->psNextLink;
	}

	UnlockProfiler();

	fclose(fp);
}

void ProfileInit(void)
{
	EGCResultCode eResult;

	eResult = GCOSSemaphoreCreate(&sg_sProfileSemaphore,
								  1);
	GCASSERT(GC_OK == eResult);

	eResult = GCGetPerformanceCounterFrequency(0,
											   &sg_u64OneSecondTicks);
	GCASSERT(GC_OK == eResult);

	DebugOut("%s: 1 Second = %u ticks\n", __FUNCTION__, sg_u64OneSecondTicks);
}