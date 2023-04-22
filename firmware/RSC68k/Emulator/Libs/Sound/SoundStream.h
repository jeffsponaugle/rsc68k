#ifndef _BASICSOUND_H_
#define _BASICSOUND_H_

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include "Application/RSC68k.h"
#include "Libs/Sound/SoundStreamWave.h"
#include "Libs/Sound/Sound.h"

#define WAVE_HEADER_SIZE (12)
#define SOUNDSTREAM_QUEUE_SIZE (256)
#define	SOUNDSTREAM_THREAD_STACK_SIZE (16384)
#define	AUDIO_BUFFER_LENGTH		(44*1024)
#define	AUDIO_BUFFER_LOW_WATER	(AUDIO_BUFFER_LENGTH - (8*1024))
#define	SAMPLE_CHUNK_PER_SECOND		60
#define SOUNDSTREAM_CMD_TIMOUT_MSEC (5000)
#define SOUNDSTREAM_SOUNDWAV_FILL_SIZE (50000)

#define SIGNED_8_TO_16(x) ((x < 0) ? ((x << 8) | (x & 0x7f)) : ((x << 8) | x))
#define GET_16BIT_LE(data,off) ((data[off+1] << 8) | data[off])
#define GET_32BIT_LE(data,off) ((data[off+3] << 24) | (data[off+2] << 16) | (data[off+1] << 8) | data[off])

#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif 

/////////////
// General //
/////////////

typedef enum
{
	ESOUNDSTREAM_STATE_NOTLOADED,
	ESOUNDSTREAM_STATE_PLAYING,
	ESOUNDSTREAM_STATE_STOPPED,
	ESOUNDSTREAM_STATE_ENDING,
	ESOUNDSTREAM_STATE_ENDED
} ESoundStreamState;

typedef enum
{
	ESOUNDSTREAM_TYPE_UNKNOWN,
	ESOUNDSTREAM_TYPE_WAVE,
} ESoundStreamType;

typedef enum
{
	ESOUNDSTREAM_CMD_AUDIO_CHUNK,
	ESOUNDSTREAM_CMD_PLAY,
	ESOUNDSTREAM_CMD_STOP,
	ESOUNDSTREAM_CMD_REWIND,
	ESOUNDSTREAM_SHUTDOWN
} ESoundStreamCommand;

extern void SoundStreamInit(void);

extern ELCDErr SoundStreamGetState(ESoundStreamState* peState);
extern ELCDErr SoundStreamSetVolume(UINT16 u16Volume);
extern ELCDErr SoundStreamSetFile(LEX_CHAR* peFileName);
extern ELCDErr SoundStreamPlay(BOOL bEnabled);
extern ELCDErr SoundStreamSetPosition(UINT64 u32Position);
extern ELCDErr SoundStreamGetPosition(UINT64 *pu32Position);
extern ELCDErr SoundStreamGetLength(UINT64 *pu64Length);
extern ELCDErr SoundStreamWaitUntilDone(void);
extern ELCDErr SoundStreamSetLoop(BOOL bLoopEnabled);
extern ELCDErr SoundStreamRewind(void);
extern void SoundStreamAddSymbols(void);

///////////////////////////
// Generic Sound Stream //
///////////////////////////

typedef struct SSoundStreamFunctions
{
	ELCDErr (*SoundStreamFillBufferFunc)(struct SSoundStream *psSoundStream, INT16 *ps16Left, INT16 *ps16Right, UINT32 u32LengthInSamples, BOOL *pbComplete);
	ELCDErr (*SoundStreamShutdownFunc)(struct SSoundStream *psSoundStream);
	ELCDErr (*SoundStreamResetFunc)(struct SSoundStream *psSoundStream);
	ELCDErr (*SoundStreamGetLengthFunc)(struct SSoundStream *psSoundStream, UINT64 *pu64Length);
	ELCDErr (*SoundStreamGetPositionFunc)(struct SSoundStream *psSoundStream, UINT64 *pu64Position);
	ELCDErr (*SoundStreamSetPositionFunc)(struct SSoundStream *psSoundStream, UINT64 u64Position);
	ELCDErr (*SoundStreamGetSampleCountFunc)(struct SSoundStream *psSoundStream, UINT32 *pu32SampleCount);
} SSoundStreamFunctions;

typedef struct SSoundStream
{
	ESoundStreamType eType;
	BOOL bMono;
	BOOL b8Bit;
	BOOL bValid;
	UINT32 u32SampleRate;

	union
	{
		SSoundStreamWave *psSoundStreamWave;
	} uSoundStreamSpecific;

	SSoundStreamFunctions *psFunctions;

	struct SSoundStream *psNextLink;
	struct SSoundStream *psPriorLink;
} SSoundStream;

extern void SoundStreamInit(void);
extern ELCDErr SoundStreamAllocate(SSoundStream **ppsSoundStream);
extern ELCDErr SoundStreamFree(SSoundStream *psSoundStream);
extern UINT8 SoundStreamGetBytesPerSample(SSoundStream *psSoundStream);
extern ELCDErr SoundWavLoadFromFile(LEX_CHAR* peFileName, 
									SoundWav **ppsSoundWav);
extern ELCDErr SoundStreamShutdown(void);

#endif	// #ifndef _BASICSOUND_H_