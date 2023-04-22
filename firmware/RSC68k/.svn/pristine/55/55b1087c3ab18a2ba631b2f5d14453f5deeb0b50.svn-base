#include "Startup/app.h"
#include "Win32/SDL/sdl.h"
#include "Win32/host.h"

#ifndef USE_GRATOS

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define MAX_THREADS 32
#define MAX_MUTEXES 32
#define MAX_QUEUES 16
#define MAX_SEMAPHORES 256
#define	MAX_EVENT_FLAGS	256

typedef struct _SOSThread_t
{
	HANDLE Handle; // Windows event handle
	DWORD ThreadId;
	BOOL Suspended;
	BOOL Terminated;
	void (*pThreadEntry)(void *pvParameter);
	void* pParam;
	UINT32 index;
} SOSThread_t;

typedef struct _SOSSemaphore_t
{
	UINT32 index;
	HANDLE Handle; // Windows Semaphore handle
} SOSSemaphore_t;

typedef struct _SOSQueue_t
{
	SOSSemaphore_t *psQueueSemaphore;		// Queue semaphore
	SOSSemaphore_t *psQueueAccessSemaphore;	// Structure access semaphore
	UINT32 **ppu32ArrayItems;
	UINT32 u32HeadPointer;
	UINT32 u32TailPointer;
	UINT32 u32ArraySizeItems;
	UINT32 index;
} SOSQueue_t;

typedef struct _SOSMutex_t
{
	UINT32 index;
	SDL_sem *Handle; // Windows Mutex handle
} SOSMutex_t;

typedef struct SEventWaitList
{
	HANDLE hEvent;
	SEventFlagWait *psWaitData;
	UINT32 *pu32ValueToChange;
	BOOL bFlagDeleted;

	struct SEventWaitList *psNextLink;
} SEventWaitList;

typedef struct SEventFlag
{
	UINT32 u32EventValue;
	CRITICAL_SECTION sEventAccess;

	// Linked list of those who are waiting on this event
	SEventWaitList *psWaitList;
} SEventFlag;

static BOOL s_bRunning = FALSE;
static CRITICAL_SECTION s_CriticalSection;
static CRITICAL_SECTION s_sMutexAccessCriticalSection;
static CRITICAL_SECTION sg_sEventFlagsAccessCriticalSection;
static SOSThread_t s_Threads[MAX_THREADS] = { 0 };
static SOSSemaphore_t s_Semaphores[MAX_SEMAPHORES] = { 0 }; 
static SOSMutex_t s_Mutexes[MAX_MUTEXES] = { 0 }; 
static SOSQueue_t s_Queues[MAX_QUEUES]  = { 0 }; 
static SEventFlag *sg_psEventFlags[MAX_EVENT_FLAGS];

// Semaphore used for OS execution end
static SDL_sem *sg_psOSEndSemaphore;

/* ************************************************************************* *\
** ************************************************************************* **
// Operating system calls
** ************************************************************************* **
\* ************************************************************************* */

BOOL GCOSIsRunning(void)
{
	return(s_bRunning);
}

/* ************************************************************************* *\
** FUNCTION: OSInit
\* ************************************************************************* */
void HostOSInit(void)
{
	UINT32 u32Loop;

	if (NULL == sg_psOSEndSemaphore)
	{
		sg_psOSEndSemaphore = SDL_CreateSemaphore(0);
	}

	// Set up the thread array
	memset((void *) s_Threads, 0, sizeof(s_Threads));
	for (u32Loop = 0; u32Loop < (sizeof(s_Threads) / sizeof(s_Threads[0])); u32Loop++)
	{
		s_Threads[u32Loop].Handle = INVALID_HANDLE_VALUE;
		s_Threads[u32Loop].Terminated = TRUE;
	}

	InitializeCriticalSection(&s_CriticalSection);
	InitializeCriticalSection(&s_sMutexAccessCriticalSection);
	InitializeCriticalSection(&sg_sEventFlagsAccessCriticalSection);
}

static UINT32 sg_u32ExitCode = 0; 

/* ************************************************************************* *\
** FUNCTION: GCOSStart
\* ************************************************************************* */
UINT32 GCOSStart(void)
{
	UINT32 i =0; 

	ThreadSetName("Master management thread");

	if(s_bRunning)
	{
		return(0);
	}

	TlsSetValue(0, &s_Threads[0]);
	s_bRunning = TRUE;
	sg_u32ExitCode = 0;

	// Start any suspeneded threads. 
	for(i = 0; i < MAX_THREADS; ++i)
	{
		if(s_Threads[i].Handle && (FALSE == s_Threads[i].Suspended))
		{
			ResumeThread(s_Threads[i].Handle);
		}
	}

	DebugOut("Operating system started. Waiting for all threads to terminate...\n");

	while (1)
	{
		SDL_SemWait(sg_psOSEndSemaphore);

		// See if we have any threads that are still active
		for(i = 0; i < MAX_THREADS; ++i)
		{
			if (s_Threads[i].Handle != INVALID_HANDLE_VALUE)
			{
				// Still have an active thread
				break;
			}
		}
		
		if (i == MAX_THREADS)
		{
			// Done!
			break;
		}
	}

	DebugOut("All threads stopped. Shutting down OS.\n");

	return(sg_u32ExitCode);
}

/* ************************************************************************* *\
** FUNCTION: GCOSGetThreadID
\* ************************************************************************* */
UINT32 GCOSGetThreadID( UINT32 *pu32ThreadID )
{
	SOSThread_t* pData = (SOSThread_t*)TlsGetValue(0);
	return pData->index;
}

/* ************************************************************************* *\
** FUNCTION: GCOSStop
\* ************************************************************************* */
void GCOSStop(BOOL bHardShutdown,
			  UINT32 u32ExitCode)
{
	UINT32 i = 0; 
	UINT32 u32ThreadID = GCOSGetThreadID(NULL);

	sg_u32ExitCode = u32ExitCode;
	
	if (FALSE == bHardShutdown)
	{
		UINT32 u32ActiveThreads = 0;
		// Not a hard shutdown. We need to run through all of our active threads and
		// figure out how many active threads we have.

		do
		{
			u32ActiveThreads = 0;

			for(i = 0; i < MAX_THREADS; ++i)
			{
				if(s_Threads[i].Handle && (FALSE == s_Threads[i].Suspended))
				{
					++u32ActiveThreads;
				}
			}

			// Must have at least one active thread
			GCASSERT(u32ActiveThreads);

			// Wait until we have 1 active thread - this one
			Sleep(10);
		}
		while (u32ActiveThreads != 1);
	}

	// kill all threads
	for(i = 0; i < MAX_THREADS; ++i)
	{
		if(s_Threads[i].Handle && (i != u32ThreadID))
		{
			DebugOut("Forcing Thread (%d) to terminate. ", i);
			TerminateThread(s_Threads[i].Handle, 0);
		}
	}
	memset(s_Threads, 0, sizeof(s_Threads));

	s_bRunning = FALSE;
	SDL_SemPost(sg_psOSEndSemaphore);

	// Terminate ourselves!
	TerminateThread(s_Threads[u32ThreadID].Handle, 0);
}


/* ************************************************************************* *\
** ************************************************************************* **
// Thread manipulation
** ************************************************************************* **
\* ************************************************************************* */

/* ************************************************************************* *\
** FUNCTION: ThreadRoutine
\* ************************************************************************* */
static DWORD WINAPI ThreadRoutine(LPVOID lpParameter)
{
	UINT32 id = (UINT32) lpParameter;

	GCASSERT(id < MAX_THREADS);
	GCASSERT(s_Threads[id].pThreadEntry);

	s_Threads[id].Terminated = FALSE;
	TlsSetValue(0, &s_Threads[id]);

	s_Threads[id].pThreadEntry(s_Threads[id].pParam);

	// Indicate that thread exited normally and that it's now suspended
	s_Threads[id].Suspended = TRUE;
	s_Threads[id].Handle = INVALID_HANDLE_VALUE;
	s_Threads[id].Terminated = TRUE;

	// Post a message to the OS end semaphore so it'll wake up 
	SDL_SemPost(sg_psOSEndSemaphore);

	return 0;
}

/* ************************************************************************* *\
** FUNCTION: GCOSThreadCreate
\* ************************************************************************* */
EGCResultCode GCOSThreadCreate(void (*pThreadEntry)(void *pvParameter),
												  void *pvEntryParameter,
												  UINT8 *pu8TopOfStack,
												  UINT8 in_id)
{
	EGCResultCode ret = GC_OK;

	if(ret == GC_OK)
	{
		DWORD CreateFlags = 0;
		SOSThread_t* pThread = (SOSThread_t*)&(s_Threads[in_id]);
		pThread->pParam = pvEntryParameter;
		pThread->pThreadEntry = pThreadEntry;
		
		GCASSERT(in_id < MAX_THREADS);
		GCASSERT(s_Threads[in_id].Handle == INVALID_HANDLE_VALUE);

		if(FALSE == s_bRunning)
		{
			CreateFlags |= CREATE_SUSPENDED;
		}
		s_Threads[in_id].Suspended = FALSE;
		s_Threads[in_id].index = in_id;
		s_Threads[in_id].Terminated = FALSE;

		pThread->Handle = CreateThread(NULL, 0, ThreadRoutine, (void*)in_id, CreateFlags, &pThread->ThreadId);

		// If the thread priority is 0, that means it's time critical
		if (in_id < 2)
		{
			BOOL bResult;

			bResult = SetThreadPriority(pThread->Handle,
										THREAD_PRIORITY_TIME_CRITICAL);
			GCASSERT(bResult);
		}
	}

	return ret;
}

/* ************************************************************************* *\
** FUNCTION: GCOSThreadSuspend
\* ************************************************************************* */
EGCResultCode GCOSThreadSuspend(UINT8 in_id)
{
	EGCResultCode ret = GC_OK;

	if(ret == GC_OK)
	{
		GCASSERT(in_id < MAX_THREADS);
		GCASSERT(s_Threads[in_id].Handle);
		s_Threads[in_id].Suspended = TRUE;
		if(s_bRunning)
		{
			SuspendThread(s_Threads[in_id].Handle);		
		}
	}

	return ret;
}

/* ************************************************************************* *\
** FUNCTION: GCOSThreadResume
\* ************************************************************************* */
EGCResultCode GCOSThreadResume(UINT8 in_id)
{
	EGCResultCode ret = GC_OK;

	if(ret == GC_OK)
	{
		GCASSERT(in_id < MAX_THREADS);
		GCASSERT(s_Threads[in_id].Handle);	
		GCASSERT(s_Threads[in_id].Suspended);
		s_Threads[in_id].Suspended = FALSE;
		if(s_bRunning)
		{
			ResumeThread(s_Threads[in_id].Handle);
		}
	}

	return ret;
}

/* ************************************************************************* *\
** FUNCTION: GCOSThreadKill
\* ************************************************************************* */
EGCResultCode GCOSThreadKill(UINT8 in_id)
{
	EGCResultCode ret = GC_OK;

	if(ret == GC_OK)
	{
		GCASSERT(in_id < MAX_THREADS);
		if(s_Threads[in_id].Handle)
		{
			HANDLE hHandle = s_Threads[in_id].Handle;

			memset(&s_Threads[in_id], 0, sizeof(s_Threads[in_id]));
			s_Threads[in_id].Suspended = TRUE;
			s_Threads[in_id].Terminated = TRUE;

			TerminateThread(hHandle, 0);
			// Nothing goes here!
		}
	}

	return ret;
}

/* ************************************************************************* *\
** FUNCTION: GCOSThreadChangePriority
\* ************************************************************************* */
EGCResultCode GCOSThreadChangePriority(UINT8 in_OldId,
										UINT8 in_NewId)
{
	EGCResultCode ret = GC_OK;

	if(ret == GC_OK)
	{
		GCASSERT(in_OldId < MAX_THREADS);
		GCASSERT(in_NewId < MAX_THREADS);
		GCASSERT(s_Threads[in_OldId].Handle);

		if(s_Threads[in_NewId].Handle)
		{	
			ret = GCOSThreadKill(in_NewId);
		}
	}

	if(ret == GC_OK)
	{
		s_Threads[in_OldId] = s_Threads[in_NewId];
		memset(&s_Threads[in_OldId], 0, sizeof(s_Threads[in_OldId]));
	}
	return ret;
}

/* ************************************************************************* *\
** FUNCTION: GCOSSleep
\* ************************************************************************* */
EGCResultCode GCOSSleep(UINT32 u32TicksToSleep)
{
	EGCResultCode ret = GC_OK;

	if(ret == GC_OK)
	{
		Sleep(u32TicksToSleep * GCTimerGetPeriod());
	}

	return ret;
}



/* ************************************************************************* *\
** ************************************************************************* **
// Semaphores
** ************************************************************************* **
\* ************************************************************************* */

/* ************************************************************************* *\
** FUNCTION: GCOSSemaphoreCreate
\* ************************************************************************* */
EGCResultCode GCOSSemaphoreCreate(SOSSemaphore *ppsSemaphore,
								  UINT32 in_InitialValue)
{
	EGCResultCode ret = GC_OK;

	if(ret == GC_OK)
	{
		UINT32 i = 0; 
		GCASSERT(ppsSemaphore);
		
		// Find a semaphore slot
		for(i = 0; i < MAX_SEMAPHORES; ++i)
		{
			if(s_Semaphores[i].Handle == NULL)
			{
				s_Semaphores[i].index = i;
				s_Semaphores[i].Handle = SDL_CreateSemaphore(in_InitialValue);
				GCASSERT(s_Semaphores[i].Handle);
				*ppsSemaphore = &(s_Semaphores[i]);
				return ret;
			}
		}
	}
	
	GCASSERT_MSG("To many semaphores, stupid arbitary limits.");
	return GC_OUT_OF_RANGE;
}

/* ************************************************************************* *\
** FUNCTION: GCOSSemaphorePut
\* ************************************************************************* */
EGCResultCode GCOSSemaphorePut(SOSSemaphore psSemaphore)
{
	int result;
	SOSSemaphore_t* pSem = (SOSSemaphore_t*)psSemaphore;

	if (FALSE == GCOSIsRunning())
	{
		return(GC_OS_NOT_RUNNING);
	}

	if (NULL == pSem)
	{
		return(GC_ERR_INVALID_OPT);
	}

	result = SDL_SemPost(pSem->Handle);
	if (0 == result)
	{
		return(GC_OK);
	}
	else
	{
		return(GC_SEM_OVF);
	}
}

/* ************************************************************************* *\
** FUNCTION: GCOSSemaphoreGet
\* ************************************************************************* */
EGCResultCode GCOSSemaphoreGet(SOSSemaphore psSemaphore, UINT32 in_TimeOut)
{
	int result;
	SOSSemaphore_t* pSem = (SOSSemaphore_t*)psSemaphore;

	if (FALSE == GCOSIsRunning())
	{
		return(GC_OS_NOT_RUNNING);
	}

	if (NULL == pSem)
	{
		return(GC_ERR_INVALID_OPT);
	}

	if (0 == in_TimeOut)
	{
		result = SDL_SemWait(pSem->Handle);
	}
	else if (0xffffffff == in_TimeOut)
	{
		result = SDL_SemTryWait(pSem->Handle);
	}
	else
	{
		result = SDL_SemWaitTimeout(pSem->Handle,
									in_TimeOut * GCTimerGetPeriod());
	}

	if (0 == result)
	{
		return(GC_OK);
	}
	else
	{
		return(GC_TIMEOUT);
	}
}


/* ************************************************************************* *\
** FUNCTION: GCOSSemaphoreDelete
\* ************************************************************************* */
EGCResultCode GCOSSemaphoreDelete(SOSSemaphore psSemaphore,
								  EOSEventDeleteOption eDeleteOption)
{
	EGCResultCode ret = GC_OK;
	SOSSemaphore_t* pSem = (SOSSemaphore_t*)psSemaphore;
	UINT32 i;

	for(i = 0; i < MAX_SEMAPHORES; ++i)
	{
		if(&s_Semaphores[i] == psSemaphore)
		{
			s_Semaphores[i].index = 0;
			s_Semaphores[i].Handle = NULL;
			break;
		}
	}

	GCASSERT(i != MAX_SEMAPHORES);

	SDL_DestroySemaphore(pSem->Handle);
	return(GC_OK);
}



/* ************************************************************************* *\
** ************************************************************************* **
// Queue
** ************************************************************************* **
\* ************************************************************************* */

#define	QUEUE_FILL_PATTERN	0x12345678

/* ************************************************************************* *\
** FUNCTION: GCOSQueueCreate
\* ************************************************************************* */
EGCResultCode GCOSQueueCreate(SOSQueue *ppsQueue,			// Each queue item is assumed to be 4 bytes each
												 void **ppvQueueData,
												 UINT32 u32QueueSizeInElements)
{
	EGCResultCode eResult = GC_OK;
	UINT32 u32Loop = 0;
	UINT32 u32Loop2 = 0;
	SOSQueue_t *psQueue = s_Queues;
	SOSSemaphore psSemaphore = NULL;
	SOSSemaphore psAccessSemaphore = NULL;

	eResult = GCOSSemaphoreCreate(&psSemaphore,
								  0);
	if (eResult != GC_OK)
	{
		return(eResult);
	}

	// Used to control access to the queue structure
	eResult = GCOSSemaphoreCreate(&psAccessSemaphore,
								  1);
	if (eResult != GC_OK)
	{
		(void) GCOSSemaphoreDelete(psSemaphore,
								   DELETEOP_ALWAYS);
		return(eResult);
	}

	for (u32Loop = 0; u32Loop < (sizeof(s_Queues) / sizeof(s_Queues[0])); u32Loop++)
	{
		if (NULL == s_Queues[u32Loop].psQueueSemaphore)
		{
			break;
		}

		++psQueue;
	}

	if (u32Loop == (sizeof(s_Queues) / sizeof(s_Queues[0])))
	{
		// No more queue slots available.
		GCOSSemaphoreDelete(psSemaphore,
							DELETEOP_ALWAYS);
		GCOSSemaphoreDelete(psAccessSemaphore,
							DELETEOP_ALWAYS);
		return(GC_OUT_OF_QUEUES);
	}

	// We've got our queue!

	s_Queues[u32Loop].psQueueSemaphore = psSemaphore;
	s_Queues[u32Loop].psQueueAccessSemaphore = psAccessSemaphore;
	s_Queues[u32Loop].ppu32ArrayItems = (UINT32 **) ppvQueueData;
	s_Queues[u32Loop].u32ArraySizeItems = u32QueueSizeInElements;
	s_Queues[u32Loop].index = u32Loop;

	*ppsQueue = &s_Queues[u32Loop];

	for (u32Loop2 = 0; u32Loop2 < u32QueueSizeInElements; u32Loop2++)
	{
		psQueue->ppu32ArrayItems[u32Loop2] = (UINT32 *) QUEUE_FILL_PATTERN;
	}

	return(GC_OK);
}

void GCOSQueueDump(SOSQueue psQueuep)
{
	SOSQueue_t *psQueue = (SOSQueue_t *) psQueuep;
	UINT32 u32Loop;

	DebugOut("%s: psQueue->u32HeadPointer    = 0x%.8x\n", __FUNCTION__, psQueue->u32HeadPointer);
	DebugOut("%s: psQueue->u32TailPointer    = 0x%.8x\n", __FUNCTION__, psQueue->u32TailPointer);
	DebugOut("%s: psQueue->u32ArraySizeItems = 0x%.8x\n", __FUNCTION__, psQueue->u32ArraySizeItems);

	for (u32Loop = 0; u32Loop < psQueue->u32ArraySizeItems; u32Loop++)
	{
		DebugOut("%s: %4u = 0x%.8x\n", __FUNCTION__, u32Loop, psQueue->ppu32ArrayItems[u32Loop]);
	}
}


/* ************************************************************************* *\
** FUNCTION: GCOSQueueReceive
\* ************************************************************************* */
EGCResultCode GCOSQueueReceive(SOSQueue psQueueVoid,
							   void **pvQueueEvent,
							   UINT32 u32Timeout)
{
	EGCResultCode eResult;
	SOSQueue_t *psQueue = (SOSQueue_t *) psQueueVoid;
	UINT32 u32Value;
	UINT32 u32NewTail;

	eResult = GCOSSemaphoreGet(psQueue->psQueueSemaphore,
							   u32Timeout);

	if (eResult != GC_OK)
	{
		return(eResult);
	}

	// Attempt exclusive access
	eResult = GCOSSemaphoreGet(psQueue->psQueueAccessSemaphore,
							   0);
	if (eResult != GC_OK)
	{
		return(eResult);
	}

	u32NewTail = psQueue->u32TailPointer + 1;
	u32Value = (UINT32) psQueue->ppu32ArrayItems[psQueue->u32TailPointer];

	if (u32NewTail >= psQueue->u32ArraySizeItems)
	{
		// Circular buffer
		u32NewTail = 0;
	}

	psQueue->ppu32ArrayItems[psQueue->u32TailPointer] = (UINT32 *) QUEUE_FILL_PATTERN;
	psQueue->u32TailPointer = u32NewTail;
	
	if (eResult != GC_OK)
	{
		(void) GCOSSemaphorePut(psQueue->psQueueAccessSemaphore);
	}
	else
	{
		eResult = GCOSSemaphorePut(psQueue->psQueueAccessSemaphore);
	}

	*pvQueueEvent = (void *) u32Value;
	return(eResult);
}

/* ************************************************************************* *\
** FUNCTION: GCOSQueueSend
\* ************************************************************************* */
EGCResultCode GCOSQueueSend(SOSQueue psQueueVoid,
							void *pvQueueEvent)
{
	EGCResultCode eResult;
	SOSQueue_t *psQueue = (SOSQueue_t *) psQueueVoid;
	UINT32 u32NewHead;

	// Attempt exclusive access
	eResult = GCOSSemaphoreGet(psQueue->psQueueAccessSemaphore,
							   0);
	if (eResult != GC_OK)
	{
		return(eResult);
	}

	u32NewHead = psQueue->u32HeadPointer + 1;
	if (u32NewHead >= psQueue->u32ArraySizeItems)
	{
		u32NewHead = 0;
	}

	if (u32NewHead == psQueue->u32TailPointer)
	{
		// No room in the queue
		eResult = GC_Q_FULL;
		goto releaseSemaphore;
	}

	if (psQueue->ppu32ArrayItems[psQueue->u32HeadPointer] != (UINT32 *) QUEUE_FILL_PATTERN)
	{
		DebugOut("%s: Ooops - Expected 0x%.8x, got 0x%.8x\n", __FUNCTION__, QUEUE_FILL_PATTERN, psQueue->ppu32ArrayItems[psQueue->u32HeadPointer]);
		GCOSQueueDump(psQueue);
		GCASSERT(0);
	}

	psQueue->ppu32ArrayItems[psQueue->u32HeadPointer] = (UINT32 *) (pvQueueEvent);
	psQueue->u32HeadPointer = u32NewHead;

	eResult = GCOSSemaphorePut(psQueue->psQueueSemaphore);

releaseSemaphore:
	if (eResult != GC_OK)
	{
		(void) GCOSSemaphorePut(psQueue->psQueueAccessSemaphore);
	}
	else
	{
		eResult = GCOSSemaphorePut(psQueue->psQueueAccessSemaphore);
	}
	return(eResult);
}

/* ************************************************************************* *\
** FUNCTION: GCOSQueueDelete
\* ************************************************************************* */
EGCResultCode GCOSQueueDelete(SOSQueue psQueueVoid,
							  EOSEventDeleteOption eDeleteOption)
{
	EGCResultCode ret = GC_OK;
	SOSQueue_t *psQueue = (SOSQueue_t *) psQueueVoid;
	UINT32 u32Index = 0;

	GCASSERT(psQueue);
	GCASSERT(psQueue == &s_Queues[psQueue->index]);
	GCASSERT(psQueue->psQueueSemaphore);
	GCOSSemaphoreDelete(psQueue->psQueueSemaphore,
						DELETEOP_ALWAYS);
	GCOSSemaphoreDelete(psQueue->psQueueAccessSemaphore,
						DELETEOP_ALWAYS);
	psQueue->psQueueSemaphore = NULL;

	u32Index = psQueue->index;

	memset((void *) &s_Queues[u32Index], 0, sizeof(s_Queues[u32Index]));
	return(GC_OK);
}

/* ************************************************************************* *\
** FUNCTION: GCOSQueueFlush
\* ************************************************************************* */
EGCResultCode GCOSQueueFlush(SOSQueue psQueue)
{
	EGCResultCode ret = GC_OK;

	if(ret == GC_OK)
	{
		GCASSERT_MSG("Not Implemented");
	}

	return ret;
}

/* ************************************************************************* *\
** FUNCTION: GCOSQueuePeek
\* ************************************************************************* */
EGCResultCode GCOSQueuePeek(SOSQueue psQueueVoid,
							void **pvQueueEvent)
{
	SOSQueue_t *psQueue = (SOSQueue_t *) psQueueVoid;

	if (psQueue->u32TailPointer == psQueue->u32HeadPointer)
	{
		return(GC_QUEUE_EMTPY);
	}

	return(GCOSQueueReceive(psQueue,
							pvQueueEvent,
							0));
}

/* ************************************************************************* *\
** ************************************************************************* **
// Mutex
** ************************************************************************* **
\* ************************************************************************* */

/* ************************************************************************* *\
** FUNCTION: GCOSMutexCreate
\* ************************************************************************* */
EGCResultCode GCOSMutexCreate(SOSMutex *ppsMutex,
								UINT32 u32PriorityInheritanceValue)
{
	EGCResultCode ret = GC_OK;

	if(ret == GC_OK)
	{
		UINT32 i = 0; 
		GCASSERT(ppsMutex);
		
		EnterCriticalSection(&s_sMutexAccessCriticalSection);
		// Find a semaphore slot
		for(i = 0; i < MAX_MUTEXES; ++i)
		{
			if(s_Mutexes[i].Handle == NULL)
			{
				s_Mutexes[i].index = i;
				s_Mutexes[i].Handle = CreateMutex(NULL, FALSE, NULL);

				*ppsMutex = &(s_Mutexes[i]);
				LeaveCriticalSection(&s_sMutexAccessCriticalSection);
				DebugOut("%s: Mutex create 0x%.8x\n", __FUNCTION__, *ppsMutex);
				return ret;
			}
		}

		LeaveCriticalSection(&s_sMutexAccessCriticalSection);
	}
	
	GCASSERT_MSG("To many mutexes, stupid arbitrary limits.");
	return GC_OUT_OF_RANGE;
}

/* ************************************************************************* *\
** FUNCTION: GCOSMutexUnlock
\* ************************************************************************* */
EGCResultCode GCOSMutexUnlock(SOSMutex psMutex)
{
	EGCResultCode ret = GC_OK;

	DebugOut("%s: Mutex unlock 0x%.8x\n", __FUNCTION__, psMutex);
	if(ret == GC_OK)
	{
		SOSMutex_t* pMut = (SOSMutex_t*)psMutex;
		GCASSERT(pMut);
		if(pMut)
		{
			if(ReleaseMutex(pMut->Handle))
			{
//FIXME				ret = GC_INVALID_OPERATION;
			}			
		}	
	}

	return ret;
}

/* ************************************************************************* *\
** FUNCTION: GCOSMutexLock
\* ************************************************************************* */
EGCResultCode GCOSMutexLock(SOSMutex psMutex, UINT32 in_TimeOut)
{
	EGCResultCode ret = GC_OK;

	DebugOut("%s: Mutex lock 0x%.8x\n", __FUNCTION__, psMutex);
	if(ret == GC_OK)
	{
		DWORD to = 0;
		SOSMutex_t* pMut = (SOSMutex_t*)psMutex;
		GCASSERT(pMut);
		if(pMut)
		{
			to = (in_TimeOut == 0)?INFINITE:in_TimeOut;
			if(WaitForSingleObject(pMut->Handle, to) == WAIT_TIMEOUT)
			{
				ret = GC_TIMEOUT;
			}			
		}	
	}

	return ret;
}

/* ************************************************************************* *\
** FUNCTION: GCOSMutexTest
\* ************************************************************************* */
EGCResultCode GCOSMutexTest(SOSMutex psMutex)
{
	EGCResultCode ret = GC_OK;

	if(ret == GC_OK)
	{
		SOSMutex_t* pMut = (SOSMutex_t*)psMutex;
		GCASSERT(pMut);
		if(pMut)
		{
			if(WaitForSingleObject(pMut->Handle, 0) == WAIT_OBJECT_0)
			{
//FIXME				ret = GC_UNLOCKED;
			}			
		}		
	}

	return ret;
}

/* ************************************************************************* *\
** FUNCTION: GCOSMutexDelete
\* ************************************************************************* */
EGCResultCode GCOSMutexDelete(SOSMutex psMutex,
								EOSEventDeleteOption eDeleteOption)
{
	EGCResultCode ret = GC_OK;
	int i;

	DebugOut("%s: Mutex delete 0x%.8x\n", __FUNCTION__, psMutex);

	EnterCriticalSection(&s_sMutexAccessCriticalSection);
	// Find a semaphore slot
	for(i = 0; i < MAX_MUTEXES; ++i)
	{
		if(&s_Mutexes[i] == psMutex)
		{
			break;
		}
	}

	if(i != MAX_MUTEXES)
	{
		SOSMutex_t* pMut = (SOSMutex_t*)psMutex;
		GCASSERT(pMut);
		if(pMut)
		{
			if(eDeleteOption == DELETEOP_ALWAYS)
			{
				if(CloseHandle(pMut->Handle) != 0)
				{
					
				}			
			}
			else //DELETEOP_NO_PENDING
			{
				GCASSERT_MSG("Not Implemented");
			}

			// Clear out the structure
			memset((void *) &s_Mutexes[i], 0, sizeof(s_Mutexes[i]));
		}		
	}
	else
	{
		// Mutex not found.
		ret = GC_ERR_EVENT_TYPE;
	}

	LeaveCriticalSection(&s_sMutexAccessCriticalSection);

	return ret;
}


/* ************************************************************************* *\
** ************************************************************************* **
// Event flags
** ************************************************************************* **
\* ************************************************************************* */

/* ************************************************************************* *\
** FUNCTION: GCOSEventFlagCreate
\* ************************************************************************* */

EGCResultCode GCOSEventFlagCreate(SOSEventFlag *ppsEventFlag,
								  UINT32 u32InitialEventFlagSetting)
{
	SEventFlag *psEvent;
	UINT32 u32Loop;

	EnterCriticalSection(&sg_sEventFlagsAccessCriticalSection);

	for (u32Loop = 0; u32Loop < sizeof(sg_psEventFlags) / sizeof(sg_psEventFlags[0]); u32Loop++)
	{
		if (sg_psEventFlags[u32Loop] == NULL)
		{
			break;
		}
	}

	if (u32Loop == (sizeof(sg_psEventFlags) / sizeof(sg_psEventFlags[0])))
	{
		// Whoops - out of space
		LeaveCriticalSection(&sg_sEventFlagsAccessCriticalSection);
		return(GC_OUT_OF_MEMORY);
	}

	psEvent = GCAllocateMemory(sizeof(*psEvent));

	if (NULL == psEvent)
	{
		LeaveCriticalSection(&sg_sEventFlagsAccessCriticalSection);
		return(GC_OUT_OF_MEMORY);
	}

	*ppsEventFlag = (SOSEventFlag *) (u32Loop + 1);
	sg_psEventFlags[u32Loop] = psEvent;

	// Now create a critical section
	InitializeCriticalSection(&psEvent->sEventAccess);

	// Set our initial flag setting
	psEvent->u32EventValue = u32InitialEventFlagSetting;

	LeaveCriticalSection(&sg_sEventFlagsAccessCriticalSection);

	// All done!
	return(GC_OK);
}

static SEventFlag *EventFlagLockHandle(UINT32 u32Handle)
{
	SEventFlag *psEvent;

	if (0 == u32Handle)
	{
		return(NULL);
	}

	EnterCriticalSection(&sg_sEventFlagsAccessCriticalSection);

	GCASSERT(u32Handle);
	u32Handle--;
	if (NULL == sg_psEventFlags[u32Handle])
	{
		LeaveCriticalSection(&sg_sEventFlagsAccessCriticalSection);
		return(NULL);
	}

	// Got it! Let's lock it
	psEvent = sg_psEventFlags[u32Handle];
	GCASSERT(psEvent);

	// First thing, lock this event
	EnterCriticalSection(&psEvent->sEventAccess);

	return(psEvent);
}

/* ************************************************************************* *\
** FUNCTION: GCOSEventFlagSet
\* ************************************************************************* */

EGCResultCode GCOSEventFlagSet(SOSEventFlag psEventFlag,
							   BOOL bSetFlags,
							   UINT32 u32FlagMask)
{
	SEventFlag *psEvent;
	SEventWaitList *psWait = NULL;
	SEventWaitList *psPriorWait = NULL;
	UINT32 u32FinalFlags = 0;
	UINT32 u32Changed = 0;
	UINT32 u32FlagsToClear = 0;

//	DebugOut("%s: Flag group %d setting 0x%.8x\n", __FUNCTION__, psEventFlag, u32FlagMask);

	psEvent = EventFlagLockHandle((UINT32) psEventFlag);
	if (NULL == psEvent)
	{
		return(GC_FLAG_INVALID_PGRP);
	}

	// All ours!
	if (bSetFlags)
	{
		// We set flags
		u32FinalFlags = psEvent->u32EventValue | u32FlagMask;
	}
	else
	{
		// We clear flags
		u32FinalFlags = psEvent->u32EventValue & (~u32FlagMask);
	}

	// Get our changed mask
	u32Changed = u32FinalFlags ^ psEvent->u32EventValue;

	// If nothing changed, then just return. No need to wake the dead.
	if (0 == u32Changed)
	{
		goto unlockOK;
	}

	// Set our new event value
	psEvent->u32EventValue = u32FinalFlags;

	// Run through the list of everyone waiting and wake up anyone applicable
	psWait = psEvent->psWaitList;

	// Figure out if any are waiting on what just changed
	while (psWait)
	{
		BOOL bWake;

		// Indicate that we're not waking this waiting node

		bWake = FALSE;
		if (EFLAG_WAIT_CLEAR_ALL == psWait->psWaitData->eWait)
		{
			// Not tested. Remove assert at your own risk
			GCASSERT(0);

			// If all of the bits we're waiting for are clear, then wake it and clear them
			if ((psWait->psWaitData->u32FlagMask & u32FinalFlags) == psWait->psWaitData->u32FlagMask)
			{
				bWake = TRUE;
				u32FlagsToClear |= (psWait->psWaitData->u32FlagMask & u32FinalFlags);
			}
		}
		else
		if (EFLAG_WAIT_CLEAR_ANY == psWait->psWaitData->eWait)
		{
			// If any of the bits we're waiting for are clear, then wake it and clear them
			if ((psWait->psWaitData->u32FlagMask & u32FinalFlags))
			{
				bWake = TRUE;
				u32FlagsToClear |= (psWait->psWaitData->u32FlagMask & u32FinalFlags);
			}
		}
		else
		if (EFLAG_WAIT_SET_ALL == psWait->psWaitData->eWait)
		{
			// If all of the bits we're waiting for are clear, then wake it but don't clear them
			if ((psWait->psWaitData->u32FlagMask & u32FinalFlags) == psWait->psWaitData->u32FlagMask)
			{
				bWake = TRUE;
			}
		}
		else
		if (EFLAG_WAIT_SET_ANY == psWait->psWaitData->eWait)
		{
			// If any of the bits we're waiting for are clear, then wake it but don't clear them
			if ((psWait->psWaitData->u32FlagMask & u32FinalFlags))
			{
				bWake = TRUE;
			}
		}
		else
		{
			// WTF? Bad option.
			GCASSERT(0);
		}

		// Wake up any and all affected consumers
		if (bWake)
		{
			BOOL bResult;

			// This means we wake up this thread.
//			DebugOut("%s: Waking event 0x%.8x, 0x%.8x\n", __FUNCTION__, psEventFlag, psWait->hEvent);

			// Indicate that this flag was not deleted
			psWait->bFlagDeleted = FALSE;

			// Set the value of the flags that woke this thread up
			if (psWait->pu32ValueToChange)
			{
				*psWait->pu32ValueToChange = (u32Changed & psWait->psWaitData->u32FlagMask);
			}

			// Unlink this from the wait list. 
			if (psPriorWait)
			{
				// Not the first in the list
				psPriorWait->psNextLink = psWait->psNextLink;
			}
			else
			{
				// First in the list
				GCASSERT(psWait == psEvent->psWaitList);
				psEvent->psWaitList = psWait->psNextLink;
			}

			// It's out of the wait list now. Signal the task.
//			DebugOut("%s: Setting event 0x%.8x, 0x%.8x\n", __FUNCTION__, psEventFlag, psWait->hEvent);
			bResult = SetEvent(psWait->hEvent);
			GCASSERT(bResult);

			// Don't update the prior.
			psWait = psWait->psNextLink;
		}
		else
		{
			psPriorWait = psWait;
			psWait = psWait->psNextLink;
		}
	}

	// Clear out any/all flags that we need to clear
	psEvent->u32EventValue &= ~u32FlagsToClear;

unlockOK:
	// Unlock the event flag group
	LeaveCriticalSection(&psEvent->sEventAccess);

	// Get rid of our event flags access critical section
	LeaveCriticalSection(&sg_sEventFlagsAccessCriticalSection);

	return(GC_OK);
}

/* ************************************************************************* *\
** FUNCTION: GCOSEventFlagGet
\* ************************************************************************* */

EGCResultCode GCOSEventFlagGet(SOSEventFlag psEventFlag,
							   SEventFlagWait *psEventFlagWait,
							   UINT32 *pu32FlagMaskObtained)
{
	SEventFlag *psEvent;
	BOOL bImmediateMatch = FALSE;
	EGCResultCode eResult;
	SEventWaitList *psWait = NULL;

//	DebugOut("%s: Flag group %d getting 0x%.8x\n", __FUNCTION__, psEventFlag, psEventFlagWait->u32FlagMask);
	psEvent = EventFlagLockHandle((UINT32) psEventFlag);
	if (NULL == psEventFlag)
	{
		return(GC_FLAG_INVALID_PGRP);
	}

	// Let's see if we've obtained our event immediately or we'll have to wait

	if (EFLAG_WAIT_CLEAR_ALL == psEventFlagWait->eWait)
	{
		// Code not tested. Remove the assert at your own risk.
		GCASSERT(0);

		// If all of the bits we're waiting for are set, then match it and clear them
		if ((psEventFlagWait->u32FlagMask & psEvent->u32EventValue) == psEventFlagWait->u32FlagMask)
		{
			// One or more of the bits are set
			bImmediateMatch = TRUE;
			if (pu32FlagMaskObtained)
			{
				*pu32FlagMaskObtained = psEventFlagWait->u32FlagMask & psEvent->u32EventValue;
			}

			// Clear the changed bit(s)
			psEvent->u32EventValue &= ~(psEventFlagWait->u32FlagMask & psEvent->u32EventValue);
		}
	}
	else
	if (EFLAG_WAIT_CLEAR_ANY == psEventFlagWait->eWait)
	{
		// If any of the bits we're waiting for are set, then match it and clear them
		if (psEventFlagWait->u32FlagMask & psEvent->u32EventValue)
		{
			// One or more of the bits are set
			bImmediateMatch = TRUE;
			if (pu32FlagMaskObtained)
			{
				*pu32FlagMaskObtained = psEventFlagWait->u32FlagMask & psEvent->u32EventValue;
			}

			// Clear the changed bit(s)
			psEvent->u32EventValue &= ~(psEventFlagWait->u32FlagMask & psEvent->u32EventValue);
		}
	}
	else
	if (EFLAG_WAIT_SET_ALL == psEventFlagWait->eWait)
	{
		// If all of the bits we're waiting for are set, then match it but don't clear them
		if ((psEventFlagWait->u32FlagMask & psEvent->u32EventValue) == psEventFlagWait->u32FlagMask)
		{
			// One or more of the bits are set
			bImmediateMatch = TRUE;
			if (pu32FlagMaskObtained)
			{
				*pu32FlagMaskObtained = psEventFlagWait->u32FlagMask & psEvent->u32EventValue;
			}
		}
	}
	else
	if (EFLAG_WAIT_SET_ANY == psEventFlagWait->eWait)
	{
		// Suspect code.  Shouldn't be clearing bits here.  Please verify.
		GCASSERT(0);
		
		// If any of the bits we're waiting for are set, then match it but don't clear them
		if (psEventFlagWait->u32FlagMask & psEvent->u32EventValue)
		{
			// One or more of the bits are set
			bImmediateMatch = TRUE;
			if (pu32FlagMaskObtained)
			{
				*pu32FlagMaskObtained = psEventFlagWait->u32FlagMask & psEvent->u32EventValue;
			}

			// Clear the changed bit(s)
			psEvent->u32EventValue &= ~(psEventFlagWait->u32FlagMask & psEvent->u32EventValue);
		}
	}
	else
	{
		// WTF? Bad option.
		GCASSERT(0);
	}

	if (bImmediateMatch)
	{
		// We're good! Just return
		eResult = GC_OK;
	}
	else
	{
		DWORD u32Result;
		SEventWaitList *psWaitPrior = NULL;
		SEventWaitList *psWaitPtr = NULL;
		HANDLE hEvent;

		// Need to create a wait structure and block on it

		psWait = GCAllocateMemory(sizeof(*psWait));
		if (NULL == psWait)
		{
			eResult = GC_OUT_OF_MEMORY;
			goto errorExit;
		}

		// Time to *WAIT*
		psWait->bFlagDeleted = FALSE;
		psWait->pu32ValueToChange = pu32FlagMaskObtained;

		psWait->hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		GCASSERT(psWait->hEvent);

		psWait->psWaitData = psEventFlagWait;

		// Add ourselves to the event list queue
		psWait->psNextLink = psEvent->psWaitList;
		psEvent->psWaitList = psWait;

		// Make a copy of our event
		hEvent = psWait->hEvent;

		// Unlock the master event and begin waiting for it to get triggered
		LeaveCriticalSection(&psEvent->sEventAccess);

		// Get rid of our event flags access critical section
		LeaveCriticalSection(&sg_sEventFlagsAccessCriticalSection);

//		DebugOut("%s: Waiting on event 0x%.8x, 0x%.8x\n", __FUNCTION__, psEventFlag, psWait->hEvent);

		if (0 == psEventFlagWait->u32Timeout)
		{
			u32Result = WaitForSingleObject(hEvent, INFINITE);
		}
		else if (0xffffffff == psEventFlagWait->u32Timeout)
		{
			u32Result = WaitForSingleObject(hEvent, 0);
		}
		else
		{
			u32Result = WaitForSingleObject(hEvent, psEventFlagWait->u32Timeout * GCTimerGetPeriod());
		}

//		DebugOut("%s: Locking handle 0x%.8x\n", __FUNCTION__, psEventFlag);

		psEvent = EventFlagLockHandle((UINT32) psEventFlag);
		if (NULL == psEvent)
		{
//			DebugOut("%s: Event deleted 0x%.8x, 0x%.8x - ", __FUNCTION__, psEventFlag, psWait->hEvent);
			return(GC_FLAG_INVALID_PGRP);
		}
		else
		{
//			DebugOut("%s: Signaled event 0x%.8x, 0x%.8x - ", __FUNCTION__, psEventFlag, psWait->hEvent);
			if (0 == u32Result)
			{
				// Got it!
				eResult = GC_OK;
//				DebugOut("Obtained\n");
			}
			else
			if (WAIT_TIMEOUT == u32Result)
			{
//				DebugOut("Timed out\n");
				// Timed out!
				eResult = GC_TIMEOUT;

				// Run through the wait list on this wait object and see if it's still
				// linked

				psWaitPrior = NULL;
				psWaitPtr = psEvent->psWaitList;

				while (psWaitPtr)
				{
					if (psWaitPtr == psWait)
					{
						// Found it! Unlink it
						if (psWaitPrior)
						{
							psWaitPrior->psNextLink = psWait->psNextLink;
							break;
						}
						else
						{
							psEvent->psWaitList = psWait->psNextLink;
						}

						psWaitPtr = NULL;
					}
					else
					{
						psWaitPtr = psWaitPtr->psNextLink;
					}
				}
			}
			else
			{
				// Some other problem
				GCASSERT(0);
			}

			LeaveCriticalSection(&psEvent->sEventAccess);

			// At this point the wait event has been unlinked from the event flag group.
			// Time to free the wait object

			CloseHandle(psWait->hEvent);
			if (psWait->bFlagDeleted)
			{
				eResult = GC_TIMEOUT;
			}
		}

		// Get rid of our event flags access critical section
		LeaveCriticalSection(&sg_sEventFlagsAccessCriticalSection);

		GCFreeMemory(psWait);
		return(eResult);
	}

errorExit:
	LeaveCriticalSection(&psEvent->sEventAccess);

	// Get rid of our event flags access critical section
	LeaveCriticalSection(&sg_sEventFlagsAccessCriticalSection);
	return(eResult);
}

/* ************************************************************************* *\
** FUNCTION: GCOSEventFlagDelete
\* ************************************************************************* */

EGCResultCode GCOSEventFlagDelete(SOSEventFlag psEventFlag,
								  EOSEventDeleteOption eDeleteOption)
{
	SEventWaitList *psWait;
	SEventFlag *psEvent;

	psEvent = EventFlagLockHandle((UINT32) psEventFlag);
	if (NULL == psEvent)
	{
		return(GC_FLAG_INVALID_PGRP);
	}

	sg_psEventFlags[((UINT32) psEventFlag) - 1] = NULL;

	psWait = psEvent->psWaitList;

	// Run through all waiting threads - tag them as deleted
	while (psWait)
	{
		psWait->bFlagDeleted = TRUE;
		if (psWait->pu32ValueToChange)
		{
			*psWait->pu32ValueToChange = 0;
		}
		
		// Signal the waiting thread
		SetEvent(psWait->hEvent);
		psWait = psWait->psNextLink;
	}

	// Null out the wait list.
	psEvent->psWaitList = NULL;

	// Unlock the event
	LeaveCriticalSection(&psEvent->sEventAccess);

	// Delete the critical section
	DeleteCriticalSection(&psEvent->sEventAccess);

	// Get rid of the event
	GCFreeMemory(psEvent);

	// Get rid of our event flags access critical section
	LeaveCriticalSection(&sg_sEventFlagsAccessCriticalSection);

	return(GC_OK);	
}


/* ************************************************************************* *\
** ************************************************************************* **
// Critical sections
** ************************************************************************* **
\* ************************************************************************* */

/* ************************************************************************* *\
** FUNCTION: GCOSCriticalSectionEnter
\* ************************************************************************* */
EGCResultCode GCOSCriticalSectionEnter(void)
{
	EGCResultCode ret = GC_OK;

	if (s_bRunning)
	{
		EnterCriticalSection(&s_CriticalSection);
	}

	return ret;
}

/* ************************************************************************* *\
** FUNCTION: GCOSCriticalSectionExit
\* ************************************************************************* */
EGCResultCode GCOSCriticalSectionExit(void)
{
	EGCResultCode ret = GC_OK;

	if (s_bRunning)
	{
		LeaveCriticalSection(&s_CriticalSection);
	}

	return ret;
}

/* ************************************************************************* *\
** ************************************************************************* **
** EOF
** ************************************************************************* **
\* ************************************************************************* */

#endif	 // #ifdef USE_GRATOS
