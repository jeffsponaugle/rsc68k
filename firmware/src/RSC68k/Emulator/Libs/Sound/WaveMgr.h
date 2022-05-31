#ifndef _WAVEMGR_H_
#define _WAVEMGR_H_

#include "Application/RSC68k.h"
#include "Application/LCDErr.h"
#include "Libs/Sound/sound.h"

#define SOUND_SAMPLE_RATE		44100

// Wave structure for this file

typedef struct SWave
{
	SoundWav *psWav;				// Pointer to wave file
	UINT32 u32References;			// # Of references to this wave
	LEX_CHAR *peFilename;			// Wave filename
} SWave;

extern SWave *WaveGetPointer(SOUNDHANDLE eHandle);
extern void WaveMgrInit(void);
extern ELCDErr WaveLoad(LEX_CHAR *peFilename,
						SOUNDHANDLE *peWaveHandle);
extern ELCDErr WavePlay(SOUNDHANDLE eWaveHandle,
						SoundChannel *psChannel);
extern ELCDErr WaveDestroy(SOUNDHANDLE eWaveHandle);

#endif