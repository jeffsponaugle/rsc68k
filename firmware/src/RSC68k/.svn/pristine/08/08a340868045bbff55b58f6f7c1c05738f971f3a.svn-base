#include "Startup/app.h"
#include "Win32/GratOS/kernel/GratOS.h"
#include "Win32/SDL/sdl.h"
#include "Win32/host.h"

#ifdef USE_GRATOS

typedef struct SGratOSToUC
{
	EGOSErr eGratOSErr;
	EGCResultCode eGCErr;
} SGratOSToUC;

static SGratOSToUC sg_sGratOSToUC[] =
{
	{GOSERR_OK,								GC_OK},
	{GOSERR_THREAD_POINTER_NULL,			GC_TASK_OPT_ERR},
	{GOSERR_THREAD_ENTRY_NULL,				GC_TASK_OPT_ERR},
	{GOSERR_THREAD_STACK_NULL,				GC_TASK_OPT_ERR},
	{GOSERR_THREAD_STACK_SIZE_TOO_SMALL,	GC_TASK_OPT_ERR},
	{GOSERR_THREAD_PRIORITY_INVALID,		GC_PRIO_ERR},
	{GOSERR_INT_CTX_BLOCK,					GC_ERR_PENDING_ISR},
	{GOSERR_IDLE_BLOCK,						GC_TASK_SUSPEND_IDLE},
	{GOSERR_OS_NOT_RUNNING_BLOCK,			GC_OS_NOT_RUNNING},
	{GOSERR_SCHEDULER_LOCKED,				GC_ERR_PEND_LOCKED},
	{GOSERR_TIMEOUT,						GC_TIMEOUT},
	{GOSERR_DESTROYED,						GC_TIMEOUT},
	{GOSERR_DESTROY_PENDING,				GC_TIMEOUT},
	{GOSERR_SIGNATURE_BAD,					GC_ERR_INVALID_OPT},
	{GOSERR_SEMAPHORE_OVERFLOW,				GC_SEM_OVF},
	{GOSERR_EVENT_MASK_BAD_OPT,				GC_ERR_INVALID_OPT},
	{GOSERR_MUTEX_OVERFLOW,					GC_SEM_OVF},
	{GOSERR_MUTEX_NOT_LOCKED,				GC_MUTEX_UNLOCKED},
	{GOSERR_QUEUE_ITEM_SIZE_BAD,			GC_ERR_INVALID_OPT},
	{GOSERR_QUEUE_ITEM_COUNT_BAD,			GC_ERR_INVALID_OPT},
	{GOSERR_QUEUE_EMTPY,					GC_QUEUE_EMTPY},
	{GOSERR_QUEUE_FULL,						GC_Q_FULL},
	{GOSERR_TIMER_BAD_START,				GC_ERR_INVALID_OPT},
	{GOSERR_OS_ALREADY_RUNNING,				GC_OS_ERROR_UNKNOWN}
};

static EGCResultCode TranslateGratOSToUC(EGOSErr eErr)
{
	UINT32 u32Loop;

	for (u32Loop = 0; u32Loop < (sizeof(sg_sGratOSToUC) / sizeof(sg_sGratOSToUC[0])); u32Loop++)
	{
		if (eErr == sg_sGratOSToUC[u32Loop].eGratOSErr)
		{
			return(sg_sGratOSToUC[u32Loop].eGCErr);
		}
	}

	// Not found in list. WTF?
	GCASSERT(0);
	return(GC_ERR_INVALID_OPT);
}

UINT32 GCOSStart(void)
{
	EGOSErr eErr;

	eErr = GOSStart();
	return(TranslateGratOSToUC(eErr));
}


EGCResultCode GCOSThreadCreate(void (*pThreadEntry)(void *pvParameter),
												  void *pvEntryParameter,
												  UINT8 *pu8TopOfStack,
												  UINT8 in_id)
{
	SGOSThread *psThread = NULL;
	EGOSErr eErr;

	psThread = calloc(sizeof(*psThread), 1);
	if (NULL == psThread)
	{
		return(GC_OUT_OF_MEMORY);
	}

	eErr = GOSThreadCreate(psThread,
						   FALSE,
						   pThreadEntry,
						   pvEntryParameter,
						   (void *) pu8TopOfStack,
						   4096,
						   (EGOSThreadPriority) in_id);

	if (eErr != GOSERR_OK)
	{
		free(psThread);
	}

	return(TranslateGratOSToUC(eErr));
}

EGCResultCode GCOSSleep(UINT32 u32TicksToSleep)
{
	GOSSleep((EGOSWait) u32TicksToSleep);
	return(GC_OK);
}

EGCResultCode GCOSSemaphoreCreate(SOSSemaphore *ppsSemaphore,
								  UINT32 in_InitialValue)
{
	SGOSSemaphore *psSemaphore;
	EGOSErr eErr;

	psSemaphore = calloc(sizeof(*psSemaphore), 1);
	if (NULL == psSemaphore)
	{
		return(GC_OUT_OF_MEMORY);
	}

	eErr = GOSSemaphoreCreate(psSemaphore,
							  (EGOSSemaphoreCounter) in_InitialValue);

	if (eErr != GOSERR_OK)
	{
		free(psSemaphore);
	}
	else
	{
		*ppsSemaphore = (SOSSemaphore *) psSemaphore;
	}

	return(TranslateGratOSToUC(eErr));
}

EGCResultCode GCOSSemaphorePut(SOSSemaphore psSemaphore)
{
	EGOSErr eErr;

	eErr = GOSSemaphorePut((SGOSSemaphore *) psSemaphore,
						   1);
	return(TranslateGratOSToUC(eErr));
}

EGCResultCode GCOSSemaphoreGet(SOSSemaphore psSemaphore, UINT32 in_TimeOut)
{
	EGOSErr eErr;

	if (0 == in_TimeOut)
	{
		eErr = GOSSemaphoreGet((SGOSSemaphore *) psSemaphore, 
							   (EGOSWait) GOS_WAIT_FOREVER);
	}
	else
	{
		eErr = GOSSemaphoreGet((SGOSSemaphore *) psSemaphore, 
							   (EGOSWait) in_TimeOut);
	}

	return(TranslateGratOSToUC(eErr));
}


EGCResultCode GCOSSemaphoreDelete(SOSSemaphore psSemaphore,
								  EOSEventDeleteOption eDeleteOption)
{
	GCASSERT(0);
}

EGCResultCode GCOSCriticalSectionEnter(void)
{
	GOSPlatformCriticalSectionEnter();
	return(GC_OK);
}

EGCResultCode GCOSCriticalSectionExit(void)
{
	GOSPlatformCriticalSectionExit(1);
	return(GC_OK);
}

EGCResultCode GCOSEventFlagCreate(SOSEventFlag *ppsEventFlag,
								  UINT32 u32InitialEventFlagSetting)
{
	SGOSEventMask *psEventMask = NULL;
	EGOSErr eErr;

	psEventMask = calloc(sizeof(*psEventMask), 1);
	if (NULL == psEventMask)
	{
		return(GC_OUT_OF_MEMORY);
	}

	eErr = GOSEventMaskCreate(psEventMask,
							  (EGOSEventMask) u32InitialEventFlagSetting);
	
	if (eErr != GOSERR_OK)
	{
		free(psEventMask);
	}
	else
	{
		*ppsEventFlag = (SOSEventFlag) psEventMask;
	}

	return(TranslateGratOSToUC(eErr));
}


EGCResultCode GCOSEventFlagSet(SOSEventFlag psEventFlag,
							   BOOL bSetFlags,
							   UINT32 u32FlagMask)
{
	SGOSEventMask *psEventMask = (SGOSEventMask *) psEventFlag;
	EGOSErr eErr;
	BOOL bClearFlags = FALSE;

	if (FALSE == bSetFlags)
	{
		bClearFlags = TRUE;
	}

	eErr = GOSEventMaskChange(psEventMask,
							  (EGOSEventMask) u32FlagMask,
							  bClearFlags);
	
	return(TranslateGratOSToUC(eErr));
}


EGCResultCode GCOSEventFlagGet(SOSEventFlag psEventFlag,
							   SEventFlagWait *psEventFlagWait,
							   UINT32 *pu32FlagMaskObtained)
{
	EGOSErr eErr;
	EGOSEventMaskOpt eEventMaskOpt;
	EGOSWait eWaitTime = (EGOSWait) psEventFlagWait->u32Timeout;

	if (EFLAG_WAIT_CLEAR_ANY == psEventFlagWait->eWait)
	{
		eEventMaskOpt = EVTMASK_WAIT_CLEAR_ANY;
	}
	else
	{
		// Not implemented
		GCASSERT(0);
	}

	if (0 == eWaitTime)
	{
		eWaitTime = GOS_WAIT_FOREVER;
	}

	eErr = GOSEventMaskGet(psEventFlag,
						   (EGOSEventMask *) pu32FlagMaskObtained,
						   eEventMaskOpt,
						   psEventFlagWait->u32FlagMask,
						   eWaitTime);

	return(TranslateGratOSToUC(eErr));
}

EGCResultCode GCOSEventFlagDelete(SOSEventFlag psEventFlag,
								  EOSEventDeleteOption eDeleteOption)
{
	SGOSEventMask *psEventMask = (SGOSEventMask *) psEventFlag;
	EGOSErr eErr;

	eErr = GOSEventMaskDestroy(psEventMask,
							   FALSE,
							   GOS_WAIT_FOREVER);
	
	return(TranslateGratOSToUC(eErr));
}


EGCResultCode GCOSQueueCreate(SOSQueue *ppsQueue,			// Each queue item is assumed to be 4 bytes each
							  void **ppvQueueData,
							  UINT32 u32QueueSizeInElements)
{
	SGOSQueue *psQueue = NULL;
	EGOSErr eErr;

	psQueue = calloc(sizeof(*psQueue), 1);
	if (NULL == psQueue)
	{
		return(GC_OUT_OF_MEMORY);
	}

	*ppsQueue = (SOSQueue *) psQueue;
	
	eErr = GOSQueueCreate(psQueue,
						  (void *) ppvQueueData,
						  sizeof(*ppvQueueData),
						  u32QueueSizeInElements);

	if (eErr != GC_OK)
	{
		free(psQueue);
	}

	return(TranslateGratOSToUC(eErr));
}

EGCResultCode GCOSQueueReceive(SOSQueue psQueueVoid,
							   void **pvQueueEvent,
							   UINT32 u32Timeout)
{
	EGOSErr eErr;
	EGOSWait eTimeout;

	if (0 == u32Timeout)
	{
		eTimeout = GOS_WAIT_FOREVER;
	}
	else
	{
		eTimeout = (EGOSWait) u32Timeout;
	}

	eErr = GOSQueueGet(psQueueVoid,
					   pvQueueEvent,
					   eTimeout,
					   FALSE);

	return(TranslateGratOSToUC(eErr));
}

EGCResultCode GCOSQueueSend(SOSQueue psQueueVoid,
							void *pvQueueEvent)
{
	EGOSErr eErr;

	eErr = GOSQueuePut(psQueueVoid,
					   &pvQueueEvent,
					   GOS_WAIT_NONE,
					   FALSE);

	return(TranslateGratOSToUC(eErr));
}

EGCResultCode GCOSQueueDelete(SOSQueue psQueueVoid,
							  EOSEventDeleteOption eDeleteOption)
{
	GCASSERT(0);
}

EGCResultCode GCOSQueueFlush(SOSQueue psQueue)
{
	GCASSERT(0);
}

EGCResultCode GCOSQueuePeek(SOSQueue psQueueVoid,
							void **pvQueueEvent)
{
	GCASSERT(0);
}

EGCResultCode GCOSMutexCreate(SOSMutex *ppsMutex,
								UINT32 u32PriorityInheritanceValue)
{
	GCASSERT(0);
}

EGCResultCode GCOSMutexUnlock(SOSMutex psMutex)
{
	GCASSERT(0);
}

EGCResultCode GCOSMutexLock(SOSMutex psMutex, UINT32 in_TimeOut)
{
	GCASSERT(0);
}

EGCResultCode GCOSMutexTest(SOSMutex psMutex)
{
	GCASSERT(0);
}

EGCResultCode GCOSMutexDelete(SOSMutex psMutex,
								EOSEventDeleteOption eDeleteOption)
{
	GCASSERT(0);
}

void HostOSInit(void)
{
//	GOSInit();
}


EGCResultCode GCTimerCreate(STimerObject **ppsTimer)
{
	SGOSTimer *psTimer = NULL;
	EGOSErr eErr;

	psTimer = calloc(sizeof(*psTimer), 1);
	if (NULL == psTimer)
	{
		return(GC_OUT_OF_MEMORY);
	}

	eErr = GOSTimerCreate(psTimer,
						  0,
						  0,
						  FALSE,
						  NULL,
						  NULL);

	
	if (eErr != GOSERR_OK)
	{
		free(psTimer);
	}

	*ppsTimer = (STimerObject *) psTimer;
	return(TranslateGratOSToUC(eErr));
}

void GCTimerDelete(STimerObject *psTimerToDelete)
{

}


EGCResultCode GCTimerSetCallback(STimerObject *psTimerGC,
								 void (*Handler)(UINT32),
								 UINT32 u32CallbackValue)
{
	SGOSTimer *psTimer = (SGOSTimer *) psTimerGC;
	void (*HandlerGOS)(void *) = Handler;
	EGOSErr eErr;

	eErr = GOSTimerSetCallback(psTimer,
							   HandlerGOS,
							   (void *) u32CallbackValue);

	return(TranslateGratOSToUC(eErr));
}

EGCResultCode GCTimerSetValue(STimerObject *psTimerGC,
							  UINT32 u32InitialValue,
							  UINT32 u32ReloadValue)
{
	SGOSTimer *psTimer = (SGOSTimer *) psTimerGC;
	EGOSErr eErr;

	eErr = GOSTimerSet(psTimer,
					   u32InitialValue,
					   u32ReloadValue);

	return(TranslateGratOSToUC(eErr));
}


EGCResultCode GCTimerStart(STimerObject *psTimerGC)
{
	SGOSTimer *psTimer = (SGOSTimer *) psTimerGC;
	EGOSErr eErr;

	eErr = GOSTimerSetActivate(psTimer,
							   TRUE);

	return(TranslateGratOSToUC(eErr));
}

EGCResultCode GCTimerStop(STimerObject *psTimerGC)
{
	SGOSTimer *psTimer = (SGOSTimer *) psTimerGC;
	EGOSErr eErr;

	eErr = GOSTimerSetActivate(psTimer,
							   FALSE);

	return(TranslateGratOSToUC(eErr));
}

#endif	// #ifdef USE_GRATOS

