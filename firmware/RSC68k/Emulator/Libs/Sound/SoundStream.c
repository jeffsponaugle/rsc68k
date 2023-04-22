#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include "Startup/app.h"
#include "Application/RSC68k.h"
#include "Libs/Sound/SoundStreamWave.h"
#include "Libs/Sound/SoundStream.h"

static ESoundStreamState sg_eSoundStreamState = ESOUNDSTREAM_STATE_NOTLOADED; // Default = not loaded
static SSoundStream *sg_psCurrentSoundStream = NULL;
static BOOL sg_bLoopMode = FALSE; // Default = off

static SoundChannel *sg_psSoundStreamChannel = NULL;
static SOSQueue sg_sSoundStreamQueue;
static SOSSemaphore sg_sSoundStreamSemaphore;
static SOSSemaphore sg_sSoundStreamFinishedSemaphore;
static UINT8 sg_u8SoundStreamThreadStack[SOUNDSTREAM_THREAD_STACK_SIZE];
static volatile UINT32 sg_u32AudioHeadPointer = 0;
static volatile UINT32 sg_u32AudioTailPointer = 0;
static INT16 sg_s16AudioBufferLeft[AUDIO_BUFFER_LENGTH];
static INT16 sg_s16AudioBufferRight[AUDIO_BUFFER_LENGTH];
static SoundWav *sg_psSoundStreamWav = NULL;
static SoundPlayInfo sg_sSoundStreamPlayInfo;

// A list of all sound streams
static SSoundStream *sg_psSoundStreamHead = NULL;
static SOSSemaphore sg_sSoundStreamListSemaphore;

///////////////////////
// Utility functions //
///////////////////////

static ELCDErr SoundStreamAddStream(SSoundStream *psStream)
{
	ELCDErr eErr = LERR_OK;

	// Get the list semaphore
	eErr = GCOSSemaphoreGet(sg_sSoundStreamListSemaphore, 0);

	if (GC_OK != eErr)
	{
		return((ELCDErr) (eErr + LERR_GC_ERR_BASE));
	}

	// Got the semaphore. Add it to the list.
	if (NULL == sg_psSoundStreamHead)
	{
		sg_psSoundStreamHead = psStream;
	}
	else
	{
		psStream->psNextLink = sg_psSoundStreamHead;
		sg_psSoundStreamHead->psPriorLink = psStream;
		sg_psSoundStreamHead = psStream;
		psStream->psPriorLink = NULL;
	}

	// Now release it
	eErr = GCOSSemaphorePut(sg_sSoundStreamListSemaphore);
	if (eErr)
	{
		return((ELCDErr) (eErr + LERR_GC_ERR_BASE));
	}
	else
	{
		return(eErr);
	}
}

static ELCDErr SoundStreamRemoveStream(SSoundStream *psStream)
{
	ELCDErr eErr = LERR_OK;

	// Get the list semaphore
	eErr = GCOSSemaphoreGet(sg_sSoundStreamListSemaphore, 0);

	if (GC_OK != eErr)
	{
		return((ELCDErr) (eErr + LERR_GC_ERR_BASE));
	}

	if (sg_psSoundStreamHead == psStream)
	{
		// Head of the list!

		// Double check to make sure it is
		GCASSERT(NULL == psStream->psPriorLink);
		sg_psSoundStreamHead = psStream->psNextLink;
		if (sg_psSoundStreamHead)
		{
			sg_psSoundStreamHead->psPriorLink = NULL;
		}
	}
	else
	{
		// This had better be non-null
		GCASSERT(psStream->psPriorLink);

		psStream->psPriorLink = psStream->psNextLink;
		if (psStream->psNextLink)
		{
			psStream->psNextLink->psPriorLink = psStream->psPriorLink;
		}
	}

	// Now release it
	eErr = GCOSSemaphorePut(sg_sSoundStreamListSemaphore);
	if (eErr)
	{
		return((ELCDErr) (eErr + LERR_GC_ERR_BASE));
	}
	else
	{
		return(eErr);
	}
}

// Determine a sound file's file type and initialize for the type.  
//
static ELCDErr SoundStreamCreateFromFile(FILEHANDLE hFile, SSoundStream **ppsSoundStream)
{
	ELCDErr eErr = LERR_OK;
	BOOL bIsType;
	
	// Is it a WAV?
	//
	eErr = FileIsWave(hFile, &bIsType);
	RETURN_ON_FAIL(eErr);

	if (bIsType)
	{
		eErr = SoundStreamWaveCreate(hFile, ppsSoundStream);
		goto normalExit;
	}

	// Otherwise, we don't know what it is!
	//
	return LERR_SOUNDSTREAM_UNKNOWN_FORMAT;

normalExit:
	if (LERR_OK == eErr)
	{
		eErr = SoundStreamAddStream(*ppsSoundStream);
	}

	return(eErr);
}

// Get the # of active samples
static UINT32 SoundStreamGetAvailableSampleCount(void)
{
	if (sg_u32AudioHeadPointer >= sg_u32AudioTailPointer)
	{
		return(sg_u32AudioHeadPointer - sg_u32AudioTailPointer);
	}
	else
	{
		return((AUDIO_BUFFER_LENGTH - sg_u32AudioTailPointer) + sg_u32AudioHeadPointer);
	}
}

// Get the # of samples we need to read
static UINT32 SoundStreamGetFreeSampleCount(void)
{
	UINT32 u32SampleCount = SoundStreamGetAvailableSampleCount();
	
	// If this happens, reread the available sample count. This means
	// the prior algorithm failed because it was in a transition state
	if (u32SampleCount >= AUDIO_BUFFER_LENGTH)
	{
		u32SampleCount = SoundStreamGetAvailableSampleCount();
	}

	GCASSERT(u32SampleCount < AUDIO_BUFFER_LENGTH);
	return((AUDIO_BUFFER_LENGTH - 1) - u32SampleCount);
}

UINT8 SoundStreamGetBytesPerSample(SSoundStream *psSoundStream)
{
	UINT8 u8BytesPerSample = 1;
	
	if (FALSE == psSoundStream->bMono)
	{
		u8BytesPerSample <<= 1;
	}

	if (FALSE == psSoundStream->b8Bit)
	{
		u8BytesPerSample <<= 1;
	}

	return u8BytesPerSample;
}

////////////////////////
// Sound nuts & bolts //
////////////////////////

static SoundCallbackResult SoundStreamCallback(SoundChannel *pChan, SoundPlayInfo *pCh)
{
	UINT32 u32SampleCount;
	UINT32 u32Loop;
	INT16 *ps16Left; 
	INT16 *ps16Right; 
	EGCResultCode eResult;

	// If any of our incoming pointers are null, bail out
	if (NULL == pCh)
	{
		return(SCB_LOOP);
	}

	if (NULL == pCh->pWav)
	{
		return(SCB_LOOP);
	}

	// Pointers are good. At least we hope so
	ps16Left = (INT16 *) pCh->pWav->pLeft;
	ps16Right = (INT16 *) pCh->pWav->pRight;

	// If our left or right buffers are NULL, bail out since it's expecting stereo
	if ((NULL == ps16Left) || (NULL == ps16Right))
	{
		return(SCB_LOOP);
	}

	if ((ESOUNDSTREAM_STATE_PLAYING != sg_eSoundStreamState) && (ESOUNDSTREAM_STATE_ENDING != sg_eSoundStreamState))
	{
		// Nothing playing
		goto renderSilence;
	}

	u32SampleCount = SoundStreamGetAvailableSampleCount();
	u32Loop = pCh->pWav->nLength;

	// One shot copy
	while (u32Loop && u32SampleCount)
	{
		*ps16Left = sg_s16AudioBufferLeft[sg_u32AudioTailPointer];
		ps16Left++;

		*ps16Right = sg_s16AudioBufferRight[sg_u32AudioTailPointer];
		ps16Right++;

		++sg_u32AudioTailPointer;

		if (sg_u32AudioTailPointer >= AUDIO_BUFFER_LENGTH)
		{
			sg_u32AudioTailPointer = 0;
		}

		u32Loop--;
		u32SampleCount--;
	}

	// And fill any remaining samples in with silence
	while (u32Loop)
	{
		*ps16Left = 0;
		++ps16Left;
		*ps16Right = 0;
		++ps16Right;
		u32Loop--;
	}

	// Go calc the samples available
	u32SampleCount = SoundStreamGetAvailableSampleCount();

	if ((u32SampleCount < AUDIO_BUFFER_LOW_WATER) && (ESOUNDSTREAM_STATE_ENDING != sg_eSoundStreamState))
	{
		(void) GCOSQueueSend(sg_sSoundStreamQueue, (void*) ESOUNDSTREAM_CMD_AUDIO_CHUNK);
	}

	if ((0 == u32SampleCount) && (ESOUNDSTREAM_STATE_ENDING == sg_eSoundStreamState))
	{
		if (sg_bLoopMode)
		{
			// We're looping, so instead of finishing, start over via the REWIND and PLAY commands
			//
			eResult = GCOSQueueSend(sg_sSoundStreamQueue, (void*) ESOUNDSTREAM_CMD_REWIND);
			if (GC_OK == eResult)
			{
				eResult = GCOSQueueSend(sg_sSoundStreamQueue, (void*) ESOUNDSTREAM_CMD_PLAY);
			}
		}
		else
		{
			// We're done playing, so set the set to ENDED and signal the "audio finished" semaphore
			//
			sg_eSoundStreamState = ESOUNDSTREAM_STATE_ENDED;
			eResult = GCOSSemaphorePut(sg_sSoundStreamFinishedSemaphore);
			GCASSERT(GC_OK == eResult);
		}
	}

	return(SCB_LOOP);

renderSilence:
	if (pCh)
	{
		if (pCh->pWav)
		{
			if (pCh->pWav->pLeft)
			{
				memset((void *) pCh->pWav->pLeft, 0, pCh->pWav->nLength << 1);
			}

			if (pCh->pWav->pRight)
			{
				memset((void *) pCh->pWav->pRight, 0, pCh->pWav->nLength << 1);
			}
		}
	}

	return(SCB_LOOP);
}

static void SoundStreamUpdateThread(void *pvParam)
{
	EGCResultCode eResult;
	UINT32 u32Command;

	ThreadSetName("Sound stream thread");

	DebugOut("%s: Started\n", __FUNCTION__);

	sg_psSoundStreamWav = SoundWavAlloc(GCSoundGetSampleRate() / SAMPLE_CHUNK_PER_SECOND,
									    GCSoundGetSampleRate(),
									    FALSE);
	GCASSERT(sg_psSoundStreamWav);

	memset((void *) &sg_sSoundStreamPlayInfo, 0, sizeof(sg_sSoundStreamPlayInfo));
	SoundPlayInfoInit(&sg_sSoundStreamPlayInfo,
					  sg_psSoundStreamWav);

	sg_sSoundStreamPlayInfo.pCallback = SoundStreamCallback;

	// Start playing
	sg_psSoundStreamChannel = SoundPlayEx(sg_psSoundStreamChannel,
										  &sg_sSoundStreamPlayInfo);
	GCASSERT(sg_psSoundStreamChannel);

	while (1)
	{
		// Wait forever for something to do
		eResult = GCOSQueueReceive(sg_sSoundStreamQueue, (void **) &u32Command, 0);
		GCASSERT(GC_OK == eResult);

		switch (u32Command)
		{
			case ESOUNDSTREAM_SHUTDOWN:
			{
				DebugOut("%s: Shutdown order received\n", __FUNCTION__);
				goto terminateThread;
				break;
			}
			case ESOUNDSTREAM_CMD_PLAY:
			{
				ELCDErr eErr = LERR_OK;
				
				if ((ESOUNDSTREAM_STATE_ENDED == sg_eSoundStreamState) || (ESOUNDSTREAM_STATE_ENDING == sg_eSoundStreamState))
				{
					eErr = sg_psCurrentSoundStream->psFunctions->SoundStreamResetFunc(sg_psCurrentSoundStream);

					if (LERR_OK == eErr)
					{
						sg_eSoundStreamState = ESOUNDSTREAM_STATE_PLAYING;
					}
				}
				else if (ESOUNDSTREAM_STATE_STOPPED == sg_eSoundStreamState)
				{
					sg_eSoundStreamState = ESOUNDSTREAM_STATE_PLAYING;
				}

				eResult = GCOSSemaphorePut(sg_sSoundStreamSemaphore);
				GCASSERT(GC_OK == eResult);

				break;
			}

			case ESOUNDSTREAM_CMD_STOP:
			{
				sg_eSoundStreamState = ESOUNDSTREAM_STATE_STOPPED;

				eResult = GCOSSemaphorePut(sg_sSoundStreamSemaphore);
				GCASSERT(GC_OK == eResult);

				break;
			}

			case ESOUNDSTREAM_CMD_REWIND:
			{
				ELCDErr eErr = LERR_OK;

				eErr = sg_psCurrentSoundStream->psFunctions->SoundStreamResetFunc(sg_psCurrentSoundStream);

				if (LERR_OK != eErr)
				{
					sg_eSoundStreamState = ESOUNDSTREAM_STATE_STOPPED;
				}

				eResult = GCOSSemaphorePut(sg_sSoundStreamSemaphore);
				GCASSERT(GC_OK == eResult);

				break;
			}

			case ESOUNDSTREAM_CMD_AUDIO_CHUNK:
			{
				UINT32 u32SampleCount;
				ELCDErr eErr = LERR_OK;
				BOOL bComplete = TRUE;

				u32SampleCount = SoundStreamGetFreeSampleCount();

				if ((sg_u32AudioHeadPointer + u32SampleCount) >= AUDIO_BUFFER_LENGTH)
				{
					UINT32 u32Chunk = AUDIO_BUFFER_LENGTH - sg_u32AudioHeadPointer;

					// Needs to be two reads due to the circular buffer
					eErr = sg_psCurrentSoundStream->psFunctions->SoundStreamFillBufferFunc(
																	sg_psCurrentSoundStream,
																	&sg_s16AudioBufferLeft[sg_u32AudioHeadPointer], 
																	&sg_s16AudioBufferRight[sg_u32AudioHeadPointer], 
																	u32Chunk, 
																	&bComplete); 

					// Stop on error or note if ended
					//
					if (LERR_OK != eErr)
					{
						sg_eSoundStreamState = ESOUNDSTREAM_STATE_STOPPED;
					}
					else if (bComplete)
					{
						sg_eSoundStreamState = ESOUNDSTREAM_STATE_ENDING;
					}
					
					u32SampleCount -= u32Chunk;
					sg_u32AudioHeadPointer = 0;
				}

				// Now go fill up the audio buffer with real data
				if (u32SampleCount)
				{
					eErr = sg_psCurrentSoundStream->psFunctions->SoundStreamFillBufferFunc(
																	sg_psCurrentSoundStream,
																	&sg_s16AudioBufferLeft[sg_u32AudioHeadPointer],
																	&sg_s16AudioBufferRight[sg_u32AudioHeadPointer],
																	u32SampleCount,
																	&bComplete); 

					// Stop on error or note if ended
					//
					if (LERR_OK != eErr)
					{
						sg_eSoundStreamState = ESOUNDSTREAM_STATE_STOPPED;
					}
					else if (bComplete)
					{
						sg_eSoundStreamState = ESOUNDSTREAM_STATE_ENDING;
					}

					sg_u32AudioHeadPointer += u32SampleCount;
					if (sg_u32AudioHeadPointer >= AUDIO_BUFFER_LENGTH)
					{
						sg_u32AudioHeadPointer -= AUDIO_BUFFER_LENGTH;
					}

					GCASSERT(sg_u32AudioHeadPointer != sg_u32AudioTailPointer);
				}
			}

			break;
		}
	}

terminateThread:
	DebugOut("%s: Terminated\n", __FUNCTION__);
}
			

/////////////////////////
// Interface functions //
/////////////////////////

void SoundStreamInit(void)
{
	void **ppvSoundStreamQueue;
	EGCResultCode eResult;

	sg_psSoundStreamChannel = SoundChannelCreate(SOUND_PRIO_HIGHEST);
	GCASSERT(sg_psSoundStreamChannel);

	// Set up message queue
	
	ppvSoundStreamQueue = MemAlloc(sizeof(*ppvSoundStreamQueue) * SOUNDSTREAM_QUEUE_SIZE);
	GCASSERT(ppvSoundStreamQueue);

	eResult = GCOSQueueCreate(&sg_sSoundStreamQueue,
							  ppvSoundStreamQueue,
							  SOUNDSTREAM_QUEUE_SIZE);
	GCASSERT(GC_OK == eResult);

	// Semaphore time!
	eResult = GCOSSemaphoreCreate(&sg_sSoundStreamSemaphore,
								  0);
	GCASSERT(GC_OK == eResult);

	// Semaphore time!
	eResult = GCOSSemaphoreCreate(&sg_sSoundStreamFinishedSemaphore,
								  0);
	GCASSERT(GC_OK == eResult);

	// Semaphore time!
	eResult = GCOSSemaphoreCreate(&sg_sSoundStreamListSemaphore,
								  1);
	GCASSERT(GC_OK == eResult);

	// Now create a sound stream thread
	eResult = GCOSThreadCreate(SoundStreamUpdateThread,
							   NULL,
							   &sg_u8SoundStreamThreadStack[SOUNDSTREAM_THREAD_STACK_SIZE - 4],
							   3);
	GCASSERT(GC_OK == eResult);
}

ELCDErr SoundStreamShutdown(void)
{
	ELCDErr eErr = LERR_OK;

	while (sg_psSoundStreamHead)
	{
		eErr = SoundStreamFree(sg_psSoundStreamHead);
		if (eErr != LERR_OK)
		{
			break;
		}
	}

	// Flag the stream thread to shut down
	eErr = GCOSQueueSend(sg_sSoundStreamQueue, (void*) ESOUNDSTREAM_SHUTDOWN);

	return(eErr);
}


/////////////////////////
// Interface functions //
/////////////////////////

ELCDErr SoundStreamGetState(ESoundStreamState* peState)
{
	if (NULL == peState)
	{
		return LERR_MEM_INVALID_PMEM;
	}

	*peState = sg_eSoundStreamState;
	return LERR_OK;
}

ELCDErr SoundStreamSetVolume(UINT16 u16Volume)
{
	SoundErrorCode eCode;
	ELCDErr eErr = LERR_OK;

	// Ignoring error code since it never returns anything other than OK
	eCode = SoundChannelAlter(sg_psSoundStreamChannel,
							  SOUND_NO_CHANGE,
							  (INT32) u16Volume,
							  SOUND_NO_CHANGE);

	if (SERR_VOLUME_INVALID == eCode)
	{
		eErr = LERR_SOUND_VOLUME_INVALID;
	}
	else
	if (SERR_OK == eCode)
	{
		eErr = LERR_OK;
	}
	else
	if (SERR_BAD_CHANNEL == eCode)
	{
		eErr = LERR_SOUND_BAD_CHANNEL;
	}
	else
	{
		// New/unknown code 
		GCASSERT(0);
	}

	return(eErr);
}

ELCDErr SoundStreamSetFile(LEX_CHAR* peFileName)
{
	ELCDErr eErr = LERR_OK;
	FILEHANDLE hFile;

	// Clean up if there's already a stream open
	//
	if (sg_psCurrentSoundStream)
	{
		// First, stop playing
		//
		eErr = SoundStreamPlay(FALSE);
		RETURN_ON_FAIL(eErr);

		// Second, shut down the stream
		//
		eErr = sg_psCurrentSoundStream->psFunctions->SoundStreamShutdownFunc(sg_psCurrentSoundStream);
		RETURN_ON_FAIL(eErr);	

		// Set the states
		//
		sg_psCurrentSoundStream = NULL;
		sg_eSoundStreamState = ESOUNDSTREAM_STATE_NOTLOADED;
	}

	// If NULL or empty string is passed, there's nothing more to do
	//
	if ((NULL == peFileName) || (((LEX_CHAR) '\0') == *peFileName))
	{
		return LERR_OK;
	}

	// Open the new file
	//
	eErr = FileOpen(&hFile, peFileName, LEX_ASCII8("rb"));
	RETURN_ON_FAIL(eErr);

	// Init for the given type
	//
	eErr = SoundStreamCreateFromFile(hFile, &sg_psCurrentSoundStream);

	// If init failed or we don't know what it is, close the file and exit (and return the original error)
	//
	if (LERR_OK != eErr)
	{
		if (sg_psCurrentSoundStream)
		{
			SoundStreamFree(sg_psCurrentSoundStream); // Ignore this return code, the init error is more interesting
			sg_psCurrentSoundStream = NULL;
		}

		// Close the stream file since we don't know what it is
		(void) FileClose(&hFile);

		return eErr;
	}
	
	// If everything is good, we're going to leave the stream in stopped state after initialization
	//
	sg_eSoundStreamState = ESOUNDSTREAM_STATE_STOPPED;

	return LERR_OK;
}

ELCDErr SoundStreamPlay(BOOL bEnabled)
{
	EGCResultCode eResult;

	// Can't stop or play anything if no stream is set
	//
	if (ESOUNDSTREAM_STATE_NOTLOADED == sg_eSoundStreamState)
	{
		return LERR_SOUNDSTREAM_NO_STREAM_SET;
	}
	
	if (bEnabled)
	{
		if (ESOUNDSTREAM_STATE_PLAYING == sg_eSoundStreamState)
		{
			return LERR_SOUNDSTREAM_ALREADY_ACTIVE;
		}
		
		eResult = GCOSQueueSend(sg_sSoundStreamQueue, (void*) ESOUNDSTREAM_CMD_PLAY);
	}
	else
	{
		if ((ESOUNDSTREAM_STATE_STOPPED == sg_eSoundStreamState) || (ESOUNDSTREAM_STATE_ENDED == sg_eSoundStreamState))
		{
			return LERR_SOUNDSTREAM_NOT_ACTIVE;
		}

		eResult = GCOSQueueSend(sg_sSoundStreamQueue, (void*) ESOUNDSTREAM_CMD_STOP);
	}

	if (GC_OK != eResult)
	{
		return((ELCDErr) (eResult + LERR_GC_ERR_BASE));
	}

	eResult = GCOSSemaphoreGet(sg_sSoundStreamSemaphore, SOUNDSTREAM_CMD_TIMOUT_MSEC / GCTimerGetPeriod());

	if (GC_OK != eResult)
	{
		return((ELCDErr) (eResult + LERR_GC_ERR_BASE));
	}

	return LERR_OK;
}

ELCDErr SoundStreamSetPosition(UINT64 u64Position)
{
	// Make sure we have a stream in the first place
	//
	if (ESOUNDSTREAM_STATE_NOTLOADED == sg_eSoundStreamState)
	{
		return LERR_SOUNDSTREAM_NO_STREAM_SET;
	}

	GCASSERT(sg_psCurrentSoundStream);
	GCASSERT(sg_psCurrentSoundStream->psFunctions);

	if (NULL == sg_psCurrentSoundStream->psFunctions->SoundStreamSetPositionFunc)
	{
		return LERR_NOT_IMPLEMENTED;
	}

	return sg_psCurrentSoundStream->psFunctions->SoundStreamSetPositionFunc(sg_psCurrentSoundStream, u64Position);
}

ELCDErr SoundStreamGetPosition(UINT64 *pu64Position)
{
	GCASSERT(pu64Position);
	
	// Make sure we have a stream in the first place
	//
	if (ESOUNDSTREAM_STATE_NOTLOADED == sg_eSoundStreamState)
	{
		return LERR_SOUNDSTREAM_NO_STREAM_SET;
	}

	GCASSERT(sg_psCurrentSoundStream);
	GCASSERT(sg_psCurrentSoundStream->psFunctions);

	if (NULL == sg_psCurrentSoundStream->psFunctions->SoundStreamGetPositionFunc)
	{
		return LERR_NOT_IMPLEMENTED;
	}

	return sg_psCurrentSoundStream->psFunctions->SoundStreamGetPositionFunc(sg_psCurrentSoundStream, pu64Position);
}

ELCDErr SoundStreamGetLength(UINT64 *pu64Length)
{
	GCASSERT(pu64Length);
	
	// Make sure we have a stream in the first place
	//
	if (ESOUNDSTREAM_STATE_NOTLOADED == sg_eSoundStreamState)
	{
		return LERR_SOUNDSTREAM_NO_STREAM_SET;
	}

	GCASSERT(sg_psCurrentSoundStream);
	GCASSERT(sg_psCurrentSoundStream->psFunctions);

	if (NULL == sg_psCurrentSoundStream->psFunctions->SoundStreamGetLengthFunc)
	{
		return LERR_NOT_IMPLEMENTED;
	}

	return sg_psCurrentSoundStream->psFunctions->SoundStreamGetLengthFunc(sg_psCurrentSoundStream, pu64Length);
}

ELCDErr SoundStreamWaitUntilDone(void)
{
	EGCResultCode eResult;
	
	// Make sure we're playing something
	//
	if (ESOUNDSTREAM_STATE_PLAYING != sg_eSoundStreamState)
	{
		return LERR_SOUNDSTREAM_NOT_ACTIVE;
	}

	// Make sure we're not looping (otherwise we'd wait forever!)
	//
	if (sg_bLoopMode != FALSE)
	{
		return LERR_SOUNDSTREAM_LOOP_ACTIVE;
	}

	// Make sure semaphore count starts at 0
	//
	do
	{
		eResult = GCOSSemaphoreGet(sg_sSoundStreamFinishedSemaphore, 1);
	}
	while (GC_OK == eResult);

	// Wait for the finished semaphore
	//
	eResult = GCOSSemaphoreGet(sg_sSoundStreamFinishedSemaphore, 0);

	if (eResult != GC_OK)
	{
		return (ELCDErr) (eResult + LERR_GC_ERR_BASE);
	}

	return LERR_OK;
}

ELCDErr SoundStreamSetLoop(BOOL bLoopEnabled)
{
	sg_bLoopMode = bLoopEnabled;
	return LERR_OK;
}

ELCDErr SoundStreamRewind(void)
{
	EGCResultCode eResult;

	// Can't rewind if there's no stream loaded!
	//
	if (ESOUNDSTREAM_STATE_NOTLOADED == sg_eSoundStreamState)
	{
		return LERR_SOUNDSTREAM_NO_STREAM_SET;
	}
	
	eResult = GCOSQueueSend(sg_sSoundStreamQueue, (void*) ESOUNDSTREAM_CMD_REWIND);

	if (GC_OK != eResult)
	{
		return((ELCDErr) (eResult + LERR_GC_ERR_BASE));
	}

	eResult = GCOSSemaphoreGet(sg_sSoundStreamSemaphore, SOUNDSTREAM_CMD_TIMOUT_MSEC / GCTimerGetPeriod());
	return((ELCDErr) (eResult + LERR_GC_ERR_BASE));
}

ELCDErr SoundStreamAllocate(SSoundStream **ppsSoundStream)
{
	SSoundStream *psSoundStream;

	psSoundStream = MemAlloc(sizeof(*psSoundStream));

	if (NULL == psSoundStream)
	{
		return LERR_NO_MEM;
	}

	*ppsSoundStream = psSoundStream;
	psSoundStream->eType = ESOUNDSTREAM_TYPE_UNKNOWN;
	psSoundStream->bValid = FALSE;

	return LERR_OK;
}

ELCDErr SoundStreamFree(SSoundStream *psSoundStream)
{	
	ELCDErr eErr = LERR_OK;
	
	GCASSERT(psSoundStream);
	
	// If what we're playing is the same as what we're deleting, stop the playback first
	if (psSoundStream == sg_psCurrentSoundStream)
	{
		// First, stop playing
		//
		eErr = SoundStreamPlay(FALSE);
		RETURN_ON_FAIL(eErr);

		// Second, shut down the stream
		//
		eErr = sg_psCurrentSoundStream->psFunctions->SoundStreamShutdownFunc(sg_psCurrentSoundStream);
		if ((eErr != LERR_SOUNDSTREAM_NOT_ACTIVE) &&
			(eErr != LERR_OK))
		{
			goto errorExit;
		}

		// Set the states
		//
		sg_psCurrentSoundStream = NULL;
		sg_eSoundStreamState = ESOUNDSTREAM_STATE_NOTLOADED;
	}

	eErr = SoundStreamRemoveStream(psSoundStream);
	GCFreeMemory(psSoundStream);

errorExit:
	return eErr;
}

ELCDErr SoundWavLoadFromFile(LEX_CHAR* peFileName, SoundWav **ppsSoundWav)
{
	ELCDErr eErr = LERR_OK;
	FILEHANDLE hFile = HANDLE_INVALID;
	SSoundStream *psSoundStream = NULL;
	SoundWav *psSoundWav = NULL;
	UINT32 u32SampleCount, u32SamplesFilled;
	BOOL bComplete;

	eErr = FileOpen(&hFile, peFileName, "rb");
	ERROREXIT_ON_FAIL(eErr);
	
	eErr = SoundStreamCreateFromFile(hFile, &psSoundStream);
	ERROREXIT_ON_FAIL(eErr);

	if ((NULL == psSoundStream->psFunctions->SoundStreamGetSampleCountFunc) ||
		(NULL == psSoundStream->psFunctions->SoundStreamFillBufferFunc))
	{
		eErr = LERR_NOT_IMPLEMENTED;
		goto errorExit;
	}

	eErr = psSoundStream->psFunctions->SoundStreamGetSampleCountFunc(psSoundStream, &u32SampleCount);
	ERROREXIT_ON_FAIL(eErr);

	psSoundWav = SoundWavAlloc(u32SampleCount, psSoundStream->u32SampleRate, psSoundStream->bMono);

	if (NULL == psSoundWav)
	{
		eErr = LERR_NO_MEM;
		goto errorExit;
	}

	bComplete = FALSE;
	u32SamplesFilled = 0;

	while (u32SamplesFilled < u32SampleCount)
	{
		UINT32 u32SamplesToFill = MIN(u32SampleCount, SOUNDSTREAM_SOUNDWAV_FILL_SIZE);

		eErr = psSoundStream->psFunctions->SoundStreamFillBufferFunc(psSoundStream, 
																	 psSoundWav->pLeft + u32SamplesFilled,
																	 psSoundWav->pRight + u32SamplesFilled,
																	 u32SamplesToFill,
																	 &bComplete);
		ERROREXIT_ON_FAIL(eErr);

		u32SamplesFilled += u32SamplesToFill;
	}

	// If the fill func doesn't signal complete, someone must be counting samples wrong
	//
	GCASSERT(bComplete != FALSE); 

errorExit:
	if (hFile != HANDLE_INVALID)
	{
		if (eErr != LERR_OK)
		{
			(void) FileClose(&hFile);
		}
		else
		{
			eErr = FileClose(&hFile);
		}
	}

	if (psSoundStream)
	{
		if (eErr != LERR_OK)
		{
			(void) psSoundStream->psFunctions->SoundStreamShutdownFunc(psSoundStream);
		}
		else
		{
			eErr = psSoundStream->psFunctions->SoundStreamShutdownFunc(psSoundStream);
		}

		// At this point, psSoundStream has been freed
	}

	// Don't free the SoundWav if we succeeded because we're returning it!
	//
	if ((psSoundWav != NULL) && (eErr != LERR_OK))
	{
		SoundWavFree(psSoundWav);
	}
	else
	{
		*ppsSoundWav = psSoundWav;
	}

	return eErr;
}