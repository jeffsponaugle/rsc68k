/* vim:ts=4 noexpandtab
   $Id: sound.h,v 1.9 2007/09/15 08:16:28 mcuddy Exp $

	File: sound.h -- header file for sound library

*/
#ifndef _H_SOUND_
#define _H_SOUND_

typedef enum {
	SERR_OK = 0,
	SERR_NO_CHANNEL,		// didn't find a channel to play sound on
	SERR_NOT_A_WAV,			// unrecognized WAV file
	SERR_UNSUPPORTED_BITS,  // only 8 or 16 bits loadable
	SERR_INVALID_CHANNELS,	// WAV must have 1 or 2 channels
	SERR_FILE_NOT_FOUND,	// can't find file
	SERR_MALLOC_FAIL,		// out of memory
	SERR_INIT_FAIL,			// sound init failed in BIOS
	SERR_IO_ERROR,			// I/O error while reading sample
	SERR_BAD_SAMPLERATE,	// can't resample, loaded sample rate must match
	SERR_PRIORITY,			// didn't start sound; priority was too low
	SERR_SHUTTING_DOWN,		// tried to start a sound while shutting down
	SERR_OGG_STREAM_ERROR,	// ogg error.
	SERR_VOLUME_INVALID,	// Volume value invalid
	SERR_BAD_CHANNEL,		// Bad channel pointer
	SERR_MAX
} SoundErrorCode;

typedef struct SoundPlayInfo_f SoundPlayInfo;

typedef enum {
	SCB_STOP,				// done playing
	SCB_LOOP,				// keep looping
	SCB_MAX
} SoundCallbackResult;

/* forward declare pointer to sound channel */
typedef struct SSoundChannel SoundChannel;

/* 
	Pointer to function called at end of WAV playback.	Function is passed
	a pointer to the playing sound channel, a pointer TO A POINTER to the
	currently playing wav.

	To Stop sound on the channel, the callback returns SCB_STOP
	To keep going (loop or replace sound), the callback function
	can modify any of the fields in the SoundPlayInfo structure and return
	SCB_LOOP
*/
typedef SoundCallbackResult (*SoundCallback)(SoundChannel *pChan, 
											 SoundPlayInfo *pCh);

/* 
   Could make this UINT32 for more mixing resolution but must reduce to signed
   short for the hardware 
 */
typedef signed short SoundSample;

/*
	Container for a WAV loaded from storage.
*/
typedef struct {
	SoundSample *pLeft;		// left (or single) channel data
	SoundSample *pRight;	// right channel data (for mono, points to 'pLeft')
	UINT32 nLength;			// size (in samples) of wav data
	INT32 nSamplesSec;		// recording rate of wav (i.e.: 44100, 22050)
	BOOL bMono;				// TRUE if mono, FALSE = stereo
	BOOL bOneShot;			// if FALSE, loop sample
	UINT32 dwLoopStart;		// loop start point
	UINT32 dwLoopEnd;		// loop end point
	BOOL bFreeL, bFreeR;	// if library should free pLeft and/or pRight
} SoundWav;

#define SOUND_SHIFT (12)

// XXX -- currently, only SOUND_RATE_UNITY is supported
// the library will raise an assert
#define SOUND_RATE_UNITY (1<<SOUND_SHIFT)

#define SOUND_GAIN_UNITY (1<<SOUND_SHIFT)

// XXX -- currently, only SOUND_PAN_CENTER is supported.
// the library will raise an assert.
// where to PAN a sample.  The macros below are 'limit' 
// values; the actual range is 0 .. (2<<SOUND_SHIFT)
// with (1<<SOUND_SHIFT) being center.
#define SOUND_PAN_LEFT (0<<SOUND_SHIFT)
#define SOUND_PAN_CENTER (1<<SOUND_SHIFT)
#define SOUND_PAN_RIGHT (2<<SOUND_SHIFT)

struct SoundPlayInfo_f {
	SoundWav *pWav;				// pointer to wav or NULL
	INT32 nSamplesSec;			// sample playback rate (.12 fixed)
	INT32 nGain;				// gain (.12 fixed)
#define SOUND_PRIO_LOW (0)
#define SOUND_PRIO_NORMAL (128)
#define SOUND_PRIO_HIGH (255)
#define SOUND_PRIO_HIGHEST (256)

	INT32 nPrio;					// current sound priority
	INT32 nPan;					// pan position (.12 fixed, 0=L, 1=C, 2=R)
	SoundCallback pCallback;	// call at end of playback
	void *pUser;
};

// this value is used as a sentinel value for SoundAlterChannel()
#define SOUND_NO_CHANGE (~0)

/* structure for holding a streaming sound */
typedef struct soundstream_f {
	INT32 nBuffers;				// number of Wav buffers
	INT32 nHead;					// next buffer to play
	INT32 nTail;					// next buffer to write
	SoundWav **pBuffers;		// array of buffers 
	UINT32 nStall;				// statistics: how often did we underflow?
	UINT32 nUpdates;
	BOOL bEOF;
} SoundStream;

/*
	return last error code 
	Note that like 'errno', the error code is not reset to SERR_OK
	when a call completes successfully.   The return value from this
	function is only reliable after an error has been detected (i.e.,
	a Sound...() call returned -1 or NULL)
 */
extern SoundErrorCode SoundGetLastError(void);

/*
	Set global last-error code 
*/
extern void SoundSetLastError(SoundErrorCode sErr);

/*
	Turn an error code into a user string
*/
extern const char *SoundErrorString(SoundErrorCode sErr);

/*	
	Initialize sound library. May be called multiple times to re-init sound
	system to different values.

	u32SampleRate -- output sample rate.  Usually 44100 22050 or 11025
	u32BufferSize -- size of system ping-pong buffers.  Sound system allocates
		two buffers of this size, which it alternates between

	Note: When re-calling this function with sound active, this may block
	for 1-2 frames so that the output waveform can be stopped 
	"gracefully" to eliminate popping.
 */
extern SoundErrorCode SoundInit(  UINT32 u32SampleRate, 
								  UINT32 u32BufferSize );

/*
	Shutdown sound library -- This call may block for one frame so that
		sound can be stopped gracefully. (See description in SoundInit()
		above

	IMPORTANT NOTE: After this call ALL 'SoundChannel' pointers are 
	no-longer valid!
*/
extern void SoundShutdown(void);

/*
   Context Swapping; the sound library supports multiple contexts.
   (similar to GL)

   To swap contexts:
		void *pSaveContext;

		pSaveContext = SoundGetContext(void);
		SoundSetContext(NULL);	
		...

		SoundInit( ... )		// create new context
		... // do stuff
		SoundShutdown( .. )		// when done with 'new' context

		SoundSetContext(pSaveContext);	// restore old context
		...
		SoundShutdown()		// close first context.

	Calling SoundSetContext(NULL) causes the sound ISR to be shutdown
	and the next call to SoundInit() will create a new context. it is
	VITAL (To avoid mem leaks) that both sound contexts are closed.

	A SoundChannel* is specific to a sound context.
*/
void *SoundGetContext(void);
SoundErrorCode SoundSetContext(void *pNewContext);

/*
    Initialize a SoundPlayInfo structure.  if pWav is specified, it is 
	stuffed into pInfo, otherwise pInfo->pWav is set to NULL and 
	pInfo->nSamplesSec is set to the globally-initialized sound rate.
*/
extern void SoundInfoInit(SoundPlayInfo *pInfo, SoundWav *pWav);

/*
    Create a sound channel.  Sounds can then be played on the channel
	with SoundPlay or SoundPlayEx().

	The channel is created in PAUSED state with no attached wav.
*/
extern SoundChannel *SoundChannelCreate(int nPrio);

/* 
	Start a sound playing on a channel.	returns NULL on erorr, pointer to
	playing channel on success.
 
	pChannel == sound channel to play on.  NULL = first available channel
	pWav == wav object to play
	nFlags = bit flags for 'quickly' setting common play modes

	When playing a sound on a channel which already has a WAV playing on it,
	the playing sound will not be interrupted unless the new sound's priority 
	is greater-than or equal-to the currently playing sound.

	If the channel is passed in as NULL, the sound engine will return the
	first available idle channel.

	A call to SoundPlay is simply a wrapper to a call to SoundPlayEx. 

	A return value of NULL indicates some form of error.
 */
extern SoundChannel *SoundPlay(SoundChannel *pChan, SoundWav *pWav, UINT32 nFlags);

#define SPF_LOOP		 (1<<0)	// loop infinitely
#define SPF_PRIO_LOW	 (1<<1)	// low priority (0)
#define SPF_PRIO_NORMAL	 (1<<2)	// normal priority (128 -- the default)
#define SPF_PRIO_HIGH	 (1<<3)	// high priority (254)
#define SPF_PRIO_HIGHEST (1<<4)	// highest priority (255)

// initialize a SoundPlayInfo structure with default values
// (pWav is optional).
extern void SoundPlayInfoInit(SoundPlayInfo *pInfo, SoundWav *pWav);

/* 
	full bannana sound playing function -- See description of fields 
	in SoundChannel structure above.

	The values are copied from the 'SoundPlayInfo' structure, so it 
	may be re-used as soon as this function returns.

	A return value of NULL indicates some form of error.
*/
extern SoundChannel *SoundPlayEx(SoundChannel *pChan, SoundPlayInfo *pInfo);

/*
    alter parameters on an ACTIVE channel.  Any of the parameter values 
	(except nChan, obviously) can be SOUND_NO_CHANGE which will cause
	the value to be untouched.

	NOTE: nPan is currently unimplemented.

	returns SERR_OK or an error code
*/
extern SoundErrorCode SoundChannelAlter(SoundChannel *pChan,
							  INT32 nSamplesSec, 
							  INT32 nGain, 
							  INT32 nPan );

/*
   Return non-zero if channel 'pChan' is playing.
*/
extern BOOL SoundChannelGetActive(SoundChannel *pChan);

/*
   stop sound on channel 'nChan'.  if 'bWait' is set, it will wait until
   the sound channel has stopped playing (i.e.: one sound update)

   Once this function returns, the channel will be free'd and 'pChan' is no 
   longer valid.
*/
extern void SoundChannelFree(SoundChannel *pChan, BOOL bWait);
/*
    pause sound on channel 'nChan'.  if 'bWait' is set, it will wait until 
	the sound channel has actually paused (i..e: one sound update)
*/
extern void SoundChannelPause(SoundChannel *pChan, BOOL bWait);

/*  
	restart sound on a previously paused channel.  CAUTION: Will assert if the 
	channel's state is not SOUND_CHANNEL_PAUSED (so if you recently paused
	the sound and didn't wait for it to become paused, you must make sure that
	at least one sound frame has passed!)
*/
void SoundChannelResume(SoundChannel *pChan);

/*
   Allocate an "empty" wav.  used internally to allocate space for a wav
   loaded from the filesystem, also used as loop buffers for sound streaming
   code.
*/
extern SoundWav *SoundWavAlloc(INT32 nSamples, INT32 nSamplesSec, BOOL bMono);

/*
   create a handle for a WAV when you already have the WAV data in memory
   somewhere.  if pLeft and pRight are passed, WAV is assumed to be stereo,
   otherwise, if pRight is NULL, wav is MONO.

   In the case of Stereo or Mono, nSamples is the # of samples for ONE channel.
*/
extern SoundWav *SoundWavInitStatic(INT32 nSamples, INT32 nSamplesSec, 
								    SoundSample *pLeft, SoundSample *pRight );

/* 
    Low level interface to allocate a SoundWav object. 'nSamples' is the # of 
    samples PER CHANNEL (i.e.: # of 'frames' of samples).  'nSamplesSec' is 
    sample rate of data. 'bMono' indicates one or two channels of audio.
	if 'pLeft' (and pRight, if ! bMono) is given, no additional memory is
	allocated for the sample data.
	if 'bFreeL' (and bFreeR, if !bMono) is TRUE, then pLeft (pRight) will be
	free'd when the SoundWav structure is freed.
*/
SoundWav *_SoundWavAlloc(INT32 nSamples, 
					    INT32 nSamplesSec, 
						BOOL bMono, 
						SoundSample *pLeft, 
						BOOL bFreeL,
						SoundSample *pRight,
						BOOL bFreeR);

/*
	Free up storage associated with a WAV.	Note that there is NO checking
	to see if the wav is currently playing, and free'ing a currently
	playing WAV would be a Bad Thing (tm).
*/
extern void SoundWavFree(SoundWav *pWav);

/*
    Load a RAW wav.  see traw.c for format.
	This function will try to load from the archive or, if that fails, 
	from a file.
*/
SoundWav *SoundTRawLoad( const char *pu8Filename );

SoundWav *SoundTRawLoadFromData( UINT8 *data, UINT32 fileSize);

/*
	WAV file loader; returns a dynamically allocated SoundWav 
	structure or NULL on error.  The SoundWav object should
	be deleted with SoundWavFree()
*/
extern SoundWav *SoundWavLoad(char *pName);

/* typedefs for function ptrs. to handle seeking and reading on streams. */
typedef UINT32 (*WavIOReadFunc)(void *pvData, UINT32 u32Size, UINT32 u32Blocks, void *psFile);
typedef INT32 (*WavIOSeekFunc)(void *psFile, off_t s32Offset, INT32 s32Whence);

/*
    load a WAV from a buffer of data.  works the same as SoundWavLoad
*/
SoundWav *SoundWavLoadFromData( UINT8 *data, UINT32 len);

/*
	Load a wav file from an archive. Works the same as SoundWavLoad, but pulls
	it from the master.img or equivalent.
*/
extern SoundWav *SoundWavLoadFromArchive( char *pu8Filename);

/*
    load a WAV.  Low level routine -- provide your own I/O functions!
*/
SoundWav *_SoundWavLoad(void *handle, WavIOReadFunc readFunc, WavIOSeekFunc seekFunc);

/*
	Set overall volume.  value is 0 (mute) .. 31 (full volume)
*/
extern void SoundVolumeSet(INT32 nVolume);

/*
    Get overall volume.  value is 0 (mute) .. 31 (full volume)
*/
extern INT32 SoundVolumeGet(void);

/*
    Playing streamed audio:

	Overview:
		Create a stream object with SoundStreamCreate()
		Call SoundStreamPlay() to start stream playing
		loop:
			Call SoundStreamPump(), if it returns non NULL, fill in 
				returned buffer.

		To Stop sound, stop the sound channel the stream is playing on.
		or call SoundStreamEOF() which will cause the sound to stop playing
		when the FIFO empties.

	NOTES:

		If bMono == TRUE, routine that fills in buffer returned by 
		SoundStreamPump() should only fill in pWav->pLeft, 
		otherwise, both pWav->pLeft and pWav->pRight should be filled
		in.  Fill in pWav->nLength samples.

		while (1)
		{
			pWav = SoundStreamPump(pStream);
			if (pWav) 
			{
				OggFillBuffer16(pWav->pLeft,pWav->pRight,
								pWav->nSamples * sizeof(SoundSample)
			}
		}
*/

// set 'user' callback functions.  the 'pre' callback is called just
// before the sound mixing is done, and the 'post' callback is called
// just after the sound mixing is done.  Each function is passed the 
// user data set by SoundSetUserData().
//
// note that the samples in pSampleData will be clipped to 16bits!
//
// Each function returns the previously set value.
typedef void (*SoundUserCallback)(INT32 *pSampleData, 
							      UINT32 u32Samples, 
								  void* pUserData);

extern SoundUserCallback SoundSetUserPreCallback( SoundUserCallback pFunc );
extern SoundUserCallback SoundSetUserPostCallback( SoundUserCallback pFunc );
extern void *SoundSetUserData( void *pData );
extern void SoundAccumulatorGet(UINT64 *pu64LeftChannel,
								UINT64 *pu64RightChannel);
extern void SoundAccumulatorClear(void);

#endif // _H_SOUND_
