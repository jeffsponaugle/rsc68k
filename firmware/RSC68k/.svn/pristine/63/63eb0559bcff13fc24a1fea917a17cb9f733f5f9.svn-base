/* vim:ts=4 noexpandtab
   $Id: sound.c,v 1.15 2007/11/03 22:11:55 mcuddy Exp $

	File: sound.c -- main source for implementation of sound library

*/
#include "Startup/app.h"
#include "Libs/Sound/sound.h"
#include "Application/RSC68k.h"

#undef HAVE_FSEEK	// LFS doesn't implement fseek.

// a number picked out of my ass :-)
#define SOUND_DEFAULT_GAIN_SLOPE (10)

typedef enum {
	SOUND_CHANNEL_ACTIVE,
	SOUND_CHANNEL_PAUSING,
	SOUND_CHANNEL_PAUSED
} ESoundChannelState;

/* 
   internal structure for playing sound. 
*/
struct SSoundChannel {
	SoundChannel *pNext;	// link into lists.
	SoundPlayInfo sInfo;		// info about currently playing sound
	UINT32 nOffset;				// offset into sample buffer

	ESoundChannelState eState;

	// this data is for resampling
	UINT32 u32Rate;		// fractional increment
	UINT32 u32Frac;		// fractional step
	INT32 curGain;		// current gain
	INT32 gainSlope;	// max gain change per sample
};

typedef enum {
	STATE_RUNNING,				// kroozin' along..
	STATE_START_SHUTDOWN,		// someone called SoundShutdown()
	STATE_SHUTTING_DOWN,		// ISR picked it up, and is shutting down
	STATE_STOPPED				// all done.  system idle.
} SoundEngineState;

typedef struct soundcontext_f {
	UINT32 u32SampleRate;
	UINT32 u32BufferSize;
    INT32 *ps32Buffer;               // mixing buffer.
	SoundChannel *ActiveChannels;	// currently active channels
	SoundEngineState eEngineState;
	SoundErrorCode eSoundError;		// last error.
	INT32 nVolume;
	SoundUserCallback userPreCallback;
	SoundUserCallback userPostCallback;
	void *userData;
} SoundContext;

volatile static SoundContext g_SoundInitialContext = {
	0,                  // sample rate
	0,                  // buffer size
	NULL,               // ps32buffer
    NULL,               // active channels
	STATE_STOPPED,      // engine state
	SERR_OK,            // last error
	SOUND_GAIN_UNITY,	// attenuation volume (no gain/no loss)
	NULL,               // pre-callback
	NULL,               // post-callback
	NULL                // user data
};

static volatile SoundContext *g_Sound = NULL;

// when we have to mess with g_Sound->ACtiveChannels
// the function doing so sets g_sound_lock = 1. (because that can be done 
// atomically).  If the sound ISR happens, it will increment g_sound_lock to
// 2, store the update buffer pointer in g_soundPtr and g_soundSamples and 
// then return, immediately.  
static void SoundHandler(INT16 *ps16Buffer, UINT32 u32StereoSamples);
static volatile int g_sound_lock;
static INT16 *g_soundPtr;
static UINT32 g_soundSamples;

// call this when you need to manipulate g_Sound->ActiveChannels 
// or g_Sound->FreeChannels.
static void LockSound( void )
{
	GCASSERT(g_sound_lock == 0);
	g_sound_lock = 1;
}

// Call this in the sound update ISR to make sure that the sound system
// isn't locked.
static BOOL SoundLocked( INT16 *ptr, UINT32 nSamples )
{
	if (g_sound_lock)
	{
		g_sound_lock = 2;
		g_soundPtr = ptr;
		g_soundSamples = nSamples;
		return TRUE;
	}
	return FALSE;
}

static void UnlockSound( void )
{
	// if g_sound_lock is 2, then an interrupt happened while we
	// were locked.  So we need to re-call the ISR handler function
	// so that our sound gets mixed.
#ifdef _WIN32
	// on win32, we don't care -- just let the sound skip :-)
	g_sound_lock = 0;
#else
	if (g_sound_lock == 2)
	{
		g_sound_lock = 0;
		SoundHandler(g_soundPtr, g_soundSamples);
		g_soundPtr = NULL;
		g_soundSamples = 0;
	}
	else
	{
		g_sound_lock = 0;
	}
#endif
}

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif 

#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

// -------------------------------------------------------------------------

// called when playhead offset is >= wav length
static SoundCallbackResult _DoChannelCallback(SoundChannel *pChan)
{
	SoundCallbackResult rc;

	// time to call callback.
	if (pChan->sInfo.pCallback)
		rc = pChan->sInfo.pCallback(pChan, &pChan->sInfo);
	else
		rc = SCB_STOP;

	if (rc == SCB_LOOP)
	{
		// reset offset to 0.
		pChan->nOffset = 0;
		// pChan->u32Frac = 0;
	} 
	else if (rc == SCB_STOP)
	{
		pChan->eState = SOUND_CHANNEL_PAUSED;
	}
	return rc;
}

static void	_ResampleChannel(SoundChannel *pChan, INT32 *pOut, UINT32 nSamp)
{
	SoundWav *pWav;
	SoundSample *pL, *pR;
	INT32 sL, sR, gain, targetGain;
	UINT32 offset;
	UINT32 frac;
	UINT32 rate;

	// we're being lazy here with stopping/pausing resampled channels.
	// if it's a problem, I'll fix.
	if (pChan->eState == SOUND_CHANNEL_PAUSING)
	{
		pChan->eState = SOUND_CHANNEL_PAUSED;
		return;
	}

restart:
	frac = pChan->u32Frac;
	targetGain = pChan->sInfo.nGain;
	gain = pChan->curGain;
	rate = pChan->u32Rate;
	offset = pChan->nOffset;
	pWav = pChan->sInfo.pWav;

	pL = pWav->pLeft;
	pR = pWav->pRight;
	// Don't duplicate computation for mono samples.
	if (pL == pR) pR = 0;

	while (nSamp--)
	{
		// process left channel.
		sL = pL[offset];
		sL = (sL * gain) >> SOUND_SHIFT;
		// XXX -- add LERP here if it sounds like crap.
		if (pR) {
			// XXX -- add LERP here if it sounds like crap.
			sR = pR[offset];
		}
		else 
			sR = sL;

		*pOut++ += sL;
		*pOut++ += sR;

		// adjust input offset
		frac += rate;
		offset += (frac >> SOUND_SHIFT);
		frac &= ((1 << SOUND_SHIFT) - 1); 		// mask fractional part

		// out of buffer?
		if (offset >= pWav->nLength) {
			// flush cached channel parameters back to channel
			pChan->nOffset = offset;
			pChan->u32Frac = frac;
			pChan->curGain = gain;
			if (_DoChannelCallback(pChan) == SCB_STOP) 
				break;
			// re-fetch cached channel paramters
			goto restart;
		}
	}
	pChan->curGain = gain;
	pChan->nOffset = offset;
	pChan->u32Frac = frac;
}

// called by callback routine to update a channel without resampling.
static void _UpdateChannel(SoundChannel *pChan, INT32 *pOut, UINT32 nSamp)
{
	SoundWav *pWav;
	SoundSample *pL, *pR;
	INT32 sL, sR, gain, targetGain;
	UINT32 nChunk;
	INT32 nShift;
	nShift = 50;	// Spread it out over 50 samples

	gain = pChan->curGain;
	while (nSamp > 0) 
	{
		pWav = pChan->sInfo.pWav;
		targetGain = pChan->sInfo.nGain;

		// Figure out how much data we're going to get from the
		// current channel's buffer. 
		nChunk = MIN(nSamp, pWav->nLength - pChan->nOffset);

		pL = pWav->pLeft + pChan->nOffset; 
		pR = pWav->pRight + pChan->nOffset;

		pChan->nOffset += nChunk;
		nSamp -= nChunk;

		if (pChan->eState == SOUND_CHANNEL_PAUSING)
		{
			while (nChunk-- && nShift--)
			{
				// fetch sample and attenuate
				sL = (((INT32)*pL * gain) >> SOUND_SHIFT); pL++;
				sR = (((INT32)*pR * gain) >> SOUND_SHIFT); pR++;
				*pOut++ += ((((INT32)(sL)*g_Sound->nVolume)) >> SOUND_SHIFT);
				*pOut++ += ((((INT32)(sR)*g_Sound->nVolume)) >> SOUND_SHIFT);
			}
			// all done?
			if (nShift < 0)
			{
				pChan->eState = SOUND_CHANNEL_PAUSED;
				goto done;
			}
		}
		else 
		{
			if (gain == SOUND_GAIN_UNITY && gain == targetGain)
			{
				while (nChunk-- > 0)
				{
					// sum samples into output buffer.
					*pOut++ += *pL++;
					*pOut++ += *pR++;
				}
			} else {
				while (nChunk-- > 0)
				{
					// sum samples into output buffer.
					*pOut++ += ((((INT32)*pL)*gain) >> SOUND_SHIFT); pL++;
					*pOut++ += ((((INT32)*pR)*gain) >> SOUND_SHIFT); pR++;
				}
			}
		}

		if (pChan->nOffset >= pWav->nLength) {
			if (_DoChannelCallback(pChan) == SCB_STOP) 
				break;
		}
	}
done:
	pChan->curGain = gain;
}

static UINT64 sg_u64LeftChannel;
static UINT64 sg_u64RightChannel;

// hardware callback routine.  Fills in 'ps16Buffer' with u32StereoSamples 
// count of samples.  
static void SoundHandler(INT16 *ps16Buffer, UINT32 u32StereoSamples)
{
	SoundChannel *pChan;
	INT32 nAnyActive;
	
	// If no sound is running, return
	if (NULL == g_Sound)
	{
		return;
	}

	// if sound system isn't running.  don't do anything.
	if (g_Sound->eEngineState == STATE_STOPPED) return;

	if (SoundLocked(ps16Buffer, u32StereoSamples)) return;

	if (g_Sound->userPreCallback)
	{
		// if user callback, user is responsible for initializing buffer.
		g_Sound->userPreCallback(g_Sound->ps32Buffer, 
								 u32StereoSamples, 
								 g_Sound->userData);
	}
	else
	{
		// zero out buffer (since we're going to add samples below)
		memset(g_Sound->ps32Buffer, 0, (int) u32StereoSamples * 2 * sizeof(INT32));
	}

	// if we're shutting down the sound engine, find all active channels
	// that are not already 'stopping' and move them to the stopping state.
	if (g_Sound->eEngineState == STATE_START_SHUTDOWN)
	{
		for (pChan = g_Sound->ActiveChannels; pChan; pChan = pChan->pNext)
		{
			// nothing on channel. skip it.
			if (pChan->eState == SOUND_CHANNEL_PAUSED) continue;

			pChan->eState = SOUND_CHANNEL_PAUSING;
		}
		g_Sound->eEngineState = STATE_SHUTTING_DOWN;
	}

	// loop over all channels, summing them into the ps16Buffer.
	nAnyActive = 0;

	for (pChan = g_Sound->ActiveChannels; pChan; pChan = pChan->pNext)
	{
		if (pChan->eState == SOUND_CHANNEL_PAUSED || pChan->sInfo.pWav==NULL) 
			continue;

		nAnyActive++;

		// special case handling of channels with no resampling
		if (pChan->u32Rate == SOUND_RATE_UNITY)
			_UpdateChannel(pChan,g_Sound->ps32Buffer,u32StereoSamples);
		else
			_ResampleChannel(pChan,g_Sound->ps32Buffer,u32StereoSamples);
	}

	if (g_Sound->userPostCallback)
	{
		g_Sound->userPostCallback(g_Sound->ps32Buffer,
								 u32StereoSamples, 
								 g_Sound->userData);
	}

	// apply global volume
	{
		INT32 s;
		INT32 a = g_Sound->nVolume;
        INT32 *p = g_Sound->ps32Buffer;
        INT16 *q = ps16Buffer;
		UINT32 u32SampleCount = u32StereoSamples;

		if (g_Sound->nVolume != SOUND_GAIN_UNITY)
		{
			while (u32StereoSamples--) 
			{
				// attenuate and clip (left)
				s = (INT32) (*p * a) >> SOUND_SHIFT;
				*q = s > 0x7FFF ? 0x7FFF : s < -0x7FFF ? -0x7FFF : s;
				p++; q++;

				// attenuate and clip (right)
				s = (INT32) (*p * a) >> SOUND_SHIFT;
				*q = s > 0x7FFF ? 0x7FFF : s < -0x7FFF ? -0x7FFF : s;
				p++; q++;
			}
		}
		else
		{
			// Just memcpy the data
			while (u32StereoSamples--) 
			{
				// clip (left)
				s = (INT32) (*p);
				*q = s > 0x7FFF ? 0x7FFF : s < -0x7FFF ? -0x7FFF : s;
				p++; q++;

				// clip (right)
				s = (INT32) (*p);
				*q = s > 0x7FFF ? 0x7FFF : s < -0x7FFF ? -0x7FFF : s;
				p++; q++;
			}
		}

		q = ps16Buffer;
		while (u32SampleCount--)
		{
			INT16 s16Sample;

			s16Sample = *q;
			++q;
			if (s16Sample < 0)
			{
				// Make it positive
				s16Sample = -s16Sample;
			}

			sg_u64LeftChannel += (UINT64) s16Sample;

			s16Sample = *q;
			++q;
			if (s16Sample < 0)
			{
				// Make it positive
				s16Sample = -s16Sample;
			}

			sg_u64RightChannel += (UINT64) s16Sample;
		}
	}

	if (g_Sound->eEngineState == STATE_SHUTTING_DOWN && nAnyActive == 0)
	{
		g_Sound->eEngineState = STATE_STOPPED;
	}

}

void SoundAccumulatorGet(UINT64 *pu64LeftChannel,
						 UINT64 *pu64RightChannel)
{
	if (pu64LeftChannel)
	{
		*pu64LeftChannel = sg_u64LeftChannel;
	}
	if (pu64RightChannel)
	{
		*pu64RightChannel = sg_u64RightChannel;
	}
}

void SoundAccumulatorClear(void)
{
	sg_u64LeftChannel = 0;
	sg_u64RightChannel = 0;
}

// -------------------------------------------------------------------------

SoundUserCallback SoundSetUserPreCallback( SoundUserCallback pFunc ) 
{
	SoundUserCallback pOld = g_Sound->userPreCallback;
	g_Sound->userPreCallback = pFunc;
	return pOld;
}

SoundUserCallback SoundSetUserPostCallback( SoundUserCallback pFunc ) 
{
	SoundUserCallback pOld = g_Sound->userPostCallback;
	g_Sound->userPostCallback = pFunc;
	return pOld;
}

void *SoundSetUserData( void *pData ) 
{
	void *pOld = g_Sound->userData;
	g_Sound->userData = pData;
	return pOld;
}

// -------------------------------------------------------------------------

static BOOL SoundSetupBuffer(volatile SoundContext *pSound)
{
    UINT32 newSize;

    // get sample rate and buffer size back from hardware
    // (in case we didn't get what we wanted!)
	pSound->u32SampleRate = GCSoundGetSampleRate();
    newSize = GCSoundGetSampleCountPerChannel();

    if (newSize != pSound->u32BufferSize)
    {
        if (pSound->ps32Buffer)
        {
            GCFreeMemory(pSound->ps32Buffer);
            pSound->ps32Buffer = NULL;
        }
    }
    pSound->u32BufferSize = newSize;
    if (pSound->ps32Buffer == NULL)
    {
        pSound->ps32Buffer = (INT32*)MemAlloc( pSound->u32BufferSize * 2 * sizeof(INT32));
        if (pSound->ps32Buffer == NULL)
            SoundSetLastError(SERR_MALLOC_FAIL);
    }
    return pSound->ps32Buffer ? TRUE : FALSE;
}

// return a pointer to the current sound context.  The pointer is OPAQUE,
// and can only be sent back to 'SoundSetContext()'.  In fact, it MUST
// be passed to SoundSetContext() and then SoundClose() called to 
// close sound activity.
void *SoundGetContext(void)
{
	// PRESS_A2("Get Context= %x\n",g_Sound);
	return (void*)g_Sound;
}

// set current sound context.  restores opaque pointer 'pNewContext' to
// global sounds context and re-activates hardware
// returns SERR_OK or error code on failure.
SoundErrorCode SoundSetContext(void *pNewContext)
{
	// DBGF("Context was %x, setting to %x\n", g_Sound, pNewContext);
	// setting a NULL context?  shutdown sound ISR.
	if (pNewContext == NULL)
	{
		// DBGF("NULL ctx, closing sound\n");
		GCSoundClose();
	}

	// okay to set this to NULL -- a new one will be allocated.
	g_Sound = (SoundContext*)pNewContext;

	// no active sound context? we're done.
	if (g_Sound == NULL)
	{
		// PRESS_A2("Done.\n");
		return SERR_OK;
	}

	GCASSERT(g_Sound->u32BufferSize != 0 ); 

	// have a context ... open up sound hardware.
	if (GCSoundOpen( g_Sound->u32BufferSize ) != GC_OK )
	{
		// PRESS_A2("Sound open fails\n");
		SoundSetLastError(SERR_INIT_FAIL);
		return SoundGetLastError();
	}

	GCASSERT(g_Sound->u32SampleRate != 0 ); 
	if (GCSoundSetSampleRate( g_Sound->u32SampleRate ) != GC_OK )
	{
		// PRESS_A2("Sound set sample rate fails\n");

		SoundSetLastError(SERR_INIT_FAIL);
		return SoundGetLastError();
	}

	if (!SoundSetupBuffer(g_Sound))
    {
        return SoundGetLastError();
    }

	GCSoundSetCallback( SoundHandler );

	SoundSetLastError(SERR_OK);
	// PRESS_A2("context %x set\n", g_Sound);
	return SoundGetLastError();
}


// -------------------------------------------------------------------------

// setting/getting error code.  Under normal circumstances, the error code
// is saved in the sound context.  However, there are times when the
// sound context isn't allocated that errors will need to be set 
// (most notably, in loading WAVs, which doesn't really need the global
// sound context to be established, but will report an error.
// note that as soon as a context comes into being, the context's error
// code will be used, not the single-global one.

static SoundErrorCode g_lastError;
void SoundSetLastError(SoundErrorCode eCode)
{
    if (g_Sound)
    	g_Sound->eSoundError = eCode;
    else
        g_lastError = eCode;
}

// -------------------------------------------------------------------------

SoundErrorCode SoundGetLastError(void)
{
    if (g_Sound)
	    return g_Sound->eSoundError;
    return g_lastError;
}

// -------------------------------------------------------------------------
SoundErrorCode SoundInit( UINT32 u32SampleRate, 
						  UINT32 u32BufferSize )
{
	SoundContext *pSound;

	// if things are running, shut stuff down first.  This could take 
	// a frame's worth of sound.
	SoundShutdown();

	// allocate memory for a sound context.
	pSound = MemAlloc( sizeof( *pSound ) );
	if (pSound == NULL) 
	{
		return SERR_MALLOC_FAIL;
	}

	// setup initial sound context values.
	*pSound = g_SoundInitialContext;
	pSound->u32SampleRate = u32SampleRate;
	pSound->u32BufferSize = u32BufferSize;

	pSound->ActiveChannels = NULL; 

	GCASSERT(u32BufferSize != 0);

	pSound->nVolume = SOUND_GAIN_UNITY;

	pSound->eEngineState = STATE_RUNNING;
	// start up hardware
	return SoundSetContext(pSound);
}

SoundErrorCode SoundResume(void)
{
	GCASSERT(g_Sound->u32BufferSize != 0);
	if (GCSoundOpen( g_Sound->u32BufferSize ) != GC_OK )
	{
		SoundSetLastError(SERR_INIT_FAIL);
		return SoundGetLastError();
	}
	GCASSERT(g_Sound->u32SampleRate != 0);
	if (GCSoundSetSampleRate( g_Sound->u32SampleRate ) != GC_OK )
	{
		SoundSetLastError(SERR_INIT_FAIL);
		return SoundGetLastError();
	}

    // reallocate buffer, if needed.
    if (!SoundSetupBuffer(g_Sound))
    {
        return SoundGetLastError();
    }

	g_Sound->eEngineState = STATE_RUNNING;
	GCSoundSetCallback( SoundHandler );

	return SERR_OK;
}

// -------------------------------------------------------------------------

void SoundShutdown(void)
{
	SoundChannel *p, *next;
	// this waits until sound runs one more update so that it can 
	// be shutdown in a quiet manner
	if (g_Sound)
	{
		// XXX -- this will probably fail on Win32 unless sound is run
		// in another thread?
		if (g_Sound->ActiveChannels)
		{
			if (g_Sound->eEngineState == STATE_RUNNING)
			{
				g_Sound->eEngineState = STATE_START_SHUTDOWN;
				// wait for sound callback to finish shutting things down
				while (g_Sound->eEngineState != STATE_STOPPED);		
			}

			GCSoundClose();
		}
		for (p = g_Sound->ActiveChannels; p; p = next)
		{
			next = p->pNext;
			GCFreeMemory(p);
		}
		g_Sound->ActiveChannels = NULL;

		g_Sound->u32SampleRate = 0;
		g_Sound->u32BufferSize = 0;
        if (g_Sound->ps32Buffer)
        {
            GCFreeMemory(g_Sound->ps32Buffer);
        }
		// free and clear global context pointer.
		// the next call to SoundInit will make a new one.
		GCFreeMemory((void*)g_Sound); g_Sound = NULL;
	}
}

// -------------------------------------------------------------------------
// Remove pointer 'pChan' from list 'pList'.  Sound must be locked while 
// doing this.  returns NULL if the channel isn't found in the list.
SoundChannel *RemoveChannelFromList(SoundChannel *pChan)
{
	SoundChannel *prev, *p; 

	prev = NULL;
	for (p = g_Sound->ActiveChannels; p; p = p->pNext)
    {
		if (p == pChan) break;
        prev = p;
    }

	if (p)
	{
		if (prev)
			prev->pNext = p->pNext;
		else
			g_Sound->ActiveChannels = p->pNext;
		p->pNext = NULL;
	}
	return p;
}

// -------------------------------------------------------------------------

// Free a sound channel.  if 'bWait' is true, then the channel is stopped
// gracefully, otherwise, it's just yanked from the channel list.
void SoundChannelFree(SoundChannel *pChan, BOOL bWait)
{
	// wait for channel to stop? 
	if (bWait)
	{
		// if we're waiting for a channel, we have to pause it first,
		// and then remove it.  Because paused, it will stay on the 
		// active list.  otherwise, since the ISR will free the memory
		// as soon as the channel is done playing
		SoundChannelPause(pChan,TRUE);
	}
	LockSound();
	// if we fail to find it, then it was already removed, and that's 
	// okay.
	if (RemoveChannelFromList(pChan))
	{
		GCFreeMemory(pChan);
	}
	UnlockSound();
}

// pause a channel.  If 'bWait' is true, then this function doesn't
// return until the sound is paused.  otherwise, the sound will be paused
// on the next audio update.
void SoundChannelPause(SoundChannel *pChan, BOOL bWait)
{
	// channel not active? it's stopped.
	if (pChan->eState == SOUND_CHANNEL_PAUSED) return;

	// state is PAUSING or ACTIVE.

	// wait for channel to stop? 
	pChan->eState = SOUND_CHANNEL_PAUSING;

	if (bWait)
	{
		// wait for state to become 'paused'
		volatile ESoundChannelState *pV;
		pV = (volatile ESoundChannelState*) &(pChan->eState);
		while (*pV != SOUND_CHANNEL_PAUSED) 
		{
			;
		}
		return;
	}
}

// resume a paused channel.
void SoundChannelResume(SoundChannel *pChan)
{
	pChan->eState = SOUND_CHANNEL_ACTIVE;
}

// -------------------------------------------------------------------------

// internal function -- used as callback for sounds played with SoundPlay 
static SoundCallbackResult LoopCallback( SoundChannel *pChan,
										 SoundPlayInfo *pPlayInfo )
{
	// user value contains int # of loops, or -1 to loop forever.
	INT32 iLoop = (INT32) pPlayInfo->pUser;

	if (iLoop < 0) 
		return SCB_LOOP;

	if (! iLoop)
		return SCB_STOP;

	iLoop--;
	pPlayInfo->pUser = (void*) iLoop;
	return SCB_LOOP;
}


// initialize a SoundPlayInfo structure (pWav is optional)
void SoundPlayInfoInit(SoundPlayInfo *pInfo, SoundWav *pWav)
{
	pInfo->nGain = SOUND_GAIN_UNITY;
	pInfo->nPrio = SOUND_PRIO_NORMAL;
	pInfo->nPan = SOUND_PAN_CENTER;
	pInfo->pUser = NULL;
	pInfo->pCallback = NULL;
	pInfo->pWav = pWav;
	if (pWav)
	{
		pInfo->nSamplesSec = pWav->nSamplesSec;
	}
	else
	{
		pInfo->nSamplesSec = g_Sound->u32SampleRate;
	}
}

// -------------------------------------------------------------------------
SoundChannel *SoundPlay(SoundChannel *pChan, SoundWav *pWav, UINT32 nFlags)
{
	SoundPlayInfo sI;

	if ((NULL == pChan) || (NULL == pWav))
	{
		return(NULL);
	}

	sI.pWav = pWav;
	sI.nSamplesSec = pWav->nSamplesSec;
	sI.nGain = SOUND_GAIN_UNITY;
	if (nFlags & SPF_PRIO_HIGHEST)
		sI.nPrio = SOUND_PRIO_HIGHEST;
	else if (nFlags & SPF_PRIO_HIGH)
		sI.nPrio = SOUND_PRIO_HIGH;
	else if (nFlags & SPF_PRIO_LOW)
		sI.nPrio = SOUND_PRIO_LOW;
	else
		sI.nPrio = SOUND_PRIO_NORMAL;

	sI.nPan = SOUND_PAN_CENTER;
	if (nFlags & SPF_LOOP) {
		sI.pUser = (void*)-1;
	} else {
		sI.pUser = 0;
	}
	sI.pCallback = LoopCallback;

	return SoundPlayEx(pChan,&sI);
}

static void calcResampleFracRate(volatile SoundChannel *pChan)
{
	// initialize fractional resampling
	pChan->u32Rate = (pChan->sInfo.nSamplesSec << SOUND_SHIFT) / g_Sound->u32SampleRate;
}

// change characteristics of a running sound channel.
// things that can be changed currently are:
// pI->nSamplesSec
// pI->nPan
// pI->nGain
// if any of these has the value SOUND_NO_CHANGE, then the value is not
// updated.  This routine must be careful -- the channel info can
// change out from under it because of the sound ISR.
SoundErrorCode SoundChannelAlter(SoundChannel *pChan,
							 INT32 nSamplesSec, 
							 INT32 nGain, 
							 INT32 nPan )
{
	GCASSERT(pChan);

	if (NULL == pChan)
	{
		return(SERR_BAD_CHANNEL);
	}

	if (nGain != SOUND_NO_CHANGE)
	{
		pChan->sInfo.nGain = nGain;
		pChan->curGain = nGain;
	}

	if (nSamplesSec != SOUND_NO_CHANGE)
	{
		pChan->sInfo.nSamplesSec = nSamplesSec;
		calcResampleFracRate(pChan);
	}

	if (nPan != SOUND_NO_CHANGE)
	{
		pChan->sInfo.nPan = nPan;
	}
	return SERR_OK;
}

// -------------------------------------------------------------------------

static SoundChannel *_SoundChannelNew(SoundPlayInfo *pInfo)
{
	SoundChannel *pChan = MemAlloc(sizeof(SoundChannel));
	GCASSERT(pChan);

	// struct copy.
	pChan->pNext = NULL;
	pChan->eState = SOUND_CHANNEL_PAUSED;

	pChan->sInfo = *pInfo;

    pChan->pNext = g_Sound->ActiveChannels;
	pChan->gainSlope = SOUND_DEFAULT_GAIN_SLOPE;
	// add channel to head of playing channels.
	// this is atomic, so it's ISR safe.
	g_Sound->ActiveChannels = pChan;

	return pChan;
}

/*
    allocate and fill-in sound channel structure.  No WAV is attached
	to the Channel, but the channel is activated and the channel's state 
	is set to 'paused'.
*/
SoundChannel *SoundChannelCreate(int nPrio)
{
	SoundPlayInfo sInfo;

	memset(&sInfo,0,sizeof(sInfo));
	sInfo.nPrio = nPrio;
	sInfo.nGain = SOUND_GAIN_UNITY;

	return _SoundChannelNew(&sInfo);
}

// full bannana sound playing function -- See description of fields 
//	in SoundChannel structure above.
//
//	The values are copied from the 'SoundPlayInfo' structure, so it 
//	may be re-used as soon as this function returns.
//
//	A return value of -1 indicates some form of error.  Call SoundGetLastError()
//  to find out what went wrong.

SoundChannel *SoundPlayEx(SoundChannel *pChan, SoundPlayInfo *pI)
{
	GCASSERT(pI);
	GCASSERT(pI->pWav->pLeft);
	GCASSERT(pI->pWav->pRight);

	// if sound system is stopping, just return.
	if (g_Sound->eEngineState != STATE_RUNNING)
	{
		SoundSetLastError(SERR_SHUTTING_DOWN);
		return NULL;
	}

	if (pChan)
	{
		// if channel is active, we only play if pI->nPrio is greater than
		// the current priority
		if (pChan->eState == SOUND_CHANNEL_ACTIVE)
		{
			if (pI->nPrio < pChan->sInfo.nPrio)
			{
				SoundSetLastError(SERR_PRIORITY);
				return NULL;
			}
		}

		// don't process this channel, if we ISR before
		// we're done setting it up.
		pChan->eState = SOUND_CHANNEL_PAUSED;
		// copy sound play information into channel buffer
		pChan->sInfo = *pI; // structure copy
	}
	else
	{
		// null?  make a new channel.
		pChan = _SoundChannelNew(pI);
	}

	// reset channel playhead
    calcResampleFracRate(pChan);
	pChan->nOffset = 0;
	pChan->u32Frac = 0;
	pChan->curGain = pChan->sInfo.nGain;

	pChan->eState = SOUND_CHANNEL_ACTIVE;

	// everything's hunky-dory.
	return pChan;
}

// -------------------------------------------------------------------------

// allocate a SoundWav object.  'nSamples' is # of samples PER CHANNEL
// bMono == TRUE means that only one channel's worth of data will be allocated
// otherwise, two channels will be allocated.
SoundWav *SoundWavAlloc(INT32 nSamples, INT32 nSamplesSec, BOOL bMono)
{
	return _SoundWavAlloc(nSamples, nSamplesSec, bMono, NULL, FALSE, NULL, FALSE );
}

// allocate a SoundWav object. 'nSamples' is the # of samples PER CHANNEL
// (i.e.: # of 'frames' of samples).  'nSamplesSec' is sample rate of data
// 'bMono' indicates one or two channels of audio.
// if 'pLeft' (and pRight, if ! bMono) is given, no additional memory is
// allocated for the sample data.
// if 'bFreeL' (and bFreeR, if !bMono) is TRUE, then pLeft (pRight) will be
// free'd when the SoundWav structure is freed.
SoundWav *_SoundWavAlloc(INT32 nSamples, 
					    INT32 nSamplesSec, 
						BOOL bMono, 
						SoundSample *pLeft, 
						BOOL bFreeL,
						SoundSample *pRight,
						BOOL bFreeR)
{
	SoundWav *pWav;
	INT32 samplesToAlloc;

	// were we given sample data?
	if (pLeft)
	{
		samplesToAlloc = 0;
		if (bMono)
		{
			pRight = pLeft;
		}
		else
		{
			// better give both left and right.
			GCASSERT(pRight != NULL);
		}
	} 
	else
	{
		// need to allocate space for sample data
		// but we don't 'own' it, because we're allocating it as 
		// one large chunk.
		bFreeL = FALSE;
		bFreeR = FALSE;
		if (bMono)
		{
			samplesToAlloc = nSamples;
		}
		else
		{
			samplesToAlloc = nSamples * 2;
		}
	}

	// get memory for the wave header and the data.
	pWav=MemAlloc((samplesToAlloc*sizeof(SoundSample))+sizeof(SoundWav));
	if (NULL == pWav)
	{
		SoundSetLastError(SERR_MALLOC_FAIL);
		return NULL;
	}

	pWav->bMono = bMono;

	if (samplesToAlloc)
	{
		// setup left and right data pointers
		pLeft = (SoundSample*) (((unsigned char *)pWav) + sizeof(*pWav));
		if (bMono) 
		{
			// right side plays same data as left.
			pRight = pLeft;
		}
		else
		{
			pRight = pLeft + nSamples;
		}
	}

	pWav->pLeft = pLeft;
	pWav->pRight = pRight;
	pWav->bFreeL = bFreeL;
	pWav->bFreeR = bFreeR;

	pWav->nSamplesSec = nSamplesSec;
	pWav->nLength = nSamples;
	pWav->bOneShot = TRUE;

	// default loop points
	pWav->dwLoopStart = 0;
	pWav->dwLoopEnd = pWav->nLength;

	return pWav;
}

//	Free up storage associated with a WAV.	Note that there is NO checking
//	to see if the wav is currently playing, and free'ing a currently
//	playing WAV would be a Bad Thing (tm).
void SoundWavFree(SoundWav *p)
{
	if (p)
	{
		if (p->bFreeL && p->pLeft)
		{
			GCFreeMemory(p->pLeft);
		}
		if (p->bFreeR && p->pRight)
		{
			GCFreeMemory(p->pRight);
		}
	}
	GCFreeMemory(p);
}

// initialize a 'static' wav (i.e.: data is already loaded from some other
// source.
SoundWav *SoundWavInitStatic(INT32 nSamples, INT32 nSamplesSec, SoundSample *pLeft, SoundSample *pRight )
{
	return _SoundWavAlloc(nSamples, nSamplesSec, 
						pRight ? FALSE : TRUE, 
						pLeft,  FALSE,
						pRight, FALSE );
}

#if 0
SoundWav *SoundWavInitStatic(INT32 nSamples, INT32 nSamplesSec, SoundSample *pLeft, SoundSample *pRight )
{
	SoundWav *pWav;

	pWav=MemAlloc(sizeof(SoundWav));
	if (pWav == NULL)
	{
		SoundSetLastError(SERR_MALLOC_FAIL);
		return NULL;
	}
	pWav->pLeft = pLeft;
	if (pRight)
	{
		pWav->pRight = pRight;
		pWav->bMono = FALSE;
	}
	else
	{
		pWav->pRight = pLeft;
		pWav->bMono = TRUE;
	}

	pWav->nLength = nSamples;
	pWav->nSamplesSec = nSamplesSec;
	pWav->bOneShot = TRUE;
	pWav->dwLoopStart = 0;
	pWav->dwLoopEnd = nSamples;

	return pWav;
}
#endif

//	Set overall volume.  value is 0 .. 31
void SoundVolumeSet(INT32 nVolume)
{
	if (nVolume < 0)
	{
		nVolume = 0;
	}
	g_Sound->nVolume = nVolume;
}

INT32 SoundVolumeGet(void)
{
	return g_Sound->nVolume;
}

// Return non-zero if channel 'pChan' is playing.
BOOL SoundChannelGetActive(SoundChannel *pChan)
{
	return (pChan->eState == SOUND_CHANNEL_ACTIVE) ? TRUE : FALSE;
}
