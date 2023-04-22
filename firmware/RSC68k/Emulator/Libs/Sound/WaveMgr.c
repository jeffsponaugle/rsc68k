#include "Startup/app.h"
#include "Libs/Sound/WaveMgr.h"
#include "Libs/widget/widget.h"
#include "Libs/Sound/SoundStream.h"
#include "Libs/Sound/SoundStreamWave.h"

// Sample rate

#define	MAX_WAVES				128

static SWave *sg_psWaveList[MAX_WAVES];

static BOOL WaveFindFreeHandle(SOUNDHANDLE *peWaveHandle)
{
	UINT32 u32Loop;

	for (u32Loop = 0; u32Loop < (sizeof(sg_psWaveList) / sizeof(sg_psWaveList[0])); u32Loop++)
	{
		if (NULL == sg_psWaveList[u32Loop])
		{
			*peWaveHandle = (SOUNDHANDLE) u32Loop;
			return(TRUE);
		}
	}

	return(FALSE);
}

SWave *WaveGetPointer(SOUNDHANDLE eHandle)
{
	if (eHandle >= (sizeof(sg_psWaveList) / sizeof(sg_psWaveList[0])))
	{
		return(NULL);
	}

	return(sg_psWaveList[eHandle]);
}

void WaveMgrInit(void)
{
	SoundErrorCode eSoundCode;

	DebugOut("* Initializing audio\n");
	eSoundCode = SoundInit(SOUND_SAMPLE_RATE,
						   SOUND_SAMPLE_RATE / (GCGetRefreshRate() >> 24));
	GCASSERT(eSoundCode == SERR_OK);
}

ELCDErr WaveLoad(LEX_CHAR *peFilename,
				 SOUNDHANDLE *peWaveHandle)
{
	ELCDErr eErr = LERR_OK;
	SOUNDHANDLE eWaveHandle;
	SoundWav *psWave;
	SWave *psWaveModule;

	// Let's see if we already have this file loaded, and if so, just increase
	// the references to it and return the handle

	for (eWaveHandle = 0; eWaveHandle < (sizeof(sg_psWaveList) / sizeof(sg_psWaveList[0])); eWaveHandle++)
	{
		psWaveModule = sg_psWaveList[eWaveHandle];
		if (psWaveModule)
		{
			GCASSERT(psWaveModule->peFilename);
			if (Lexstrcasecmp(psWaveModule->peFilename, peFilename) == 0)
			{
				psWaveModule->u32References++;
				*peWaveHandle = eWaveHandle;
				return(eWaveHandle);
			}
		}
	}

	eErr = SoundWavLoadFromFile(peFilename, &psWave);
	GOTO_ON_FAIL(eErr, notAllocated);
	
	// Successfully loaded the wave. Gotta have a free handle
	if (FALSE == WaveFindFreeHandle(&eWaveHandle))
	{
		return(LERR_SOUND_WAVE_FULL);
	}

	// Loaded! Allocate memory for a wave handle
	psWaveModule = MemAlloc(sizeof(*psWaveModule));
	if (NULL == psWaveModule)
	{
		eErr = LERR_NO_MEM;
		goto notAllocated;
	}

	// Now a block of memory for the string
	psWaveModule->peFilename = Lexstrdup(peFilename);
	if (NULL == psWaveModule->peFilename)
	{
		eErr = LERR_NO_MEM;
		goto notAllocated;
	}

	psWaveModule->psWav = psWave;
	*peWaveHandle = eWaveHandle;
	psWaveModule->u32References = 1;
	sg_psWaveList[eWaveHandle] = psWaveModule;

	return(LERR_OK);

notAllocated:
	if (psWaveModule)
	{
		if (psWaveModule->peFilename)
		{
			GCFreeMemory(psWaveModule->peFilename);
			psWaveModule->peFilename = NULL;
		}

		GCFreeMemory(psWaveModule);
	}

	if (psWave)
	{
		SoundWavFree(psWave);
	}

	return(eErr);
}

ELCDErr WavePlay(SOUNDHANDLE eWaveHandle,
				 SoundChannel *psChannel)
{
	SWave *psWave;

	psWave = WaveGetPointer(eWaveHandle);
	if (NULL == psWave)
	{
		return(LERR_SOUND_WAVE_BAD_HANDLE);
	}

	// Cool! Let's play it
	SoundPlay(psChannel,
			  psWave->psWav,
			  0);
	return(LERR_OK);
}

ELCDErr WaveDestroy(SOUNDHANDLE eWaveHandle)
{
	SWave *psWave;

	psWave = WaveGetPointer(eWaveHandle);
	if (NULL == psWave)
	{
		return(LERR_SOUND_WAVE_BAD_HANDLE);
	}

	// Got it!
	GCASSERT(psWave->u32References);
	psWave->u32References--;

	if (0 == psWave->u32References)
	{
		// Really wipe it out
		GCFreeMemory(psWave->peFilename);
		psWave->peFilename = NULL;

		// Shut down this wave's stream
		SoundWavFree(psWave->psWav);
		psWave->psWav = NULL;

		// And finally deallocate the structure
		GCFreeMemory(psWave);

		// Make the handle invalid
		sg_psWaveList[eWaveHandle] = NULL;
	}

	// TODO: Ensure wave file isn't playing when it's deleted
	return(LERR_OK);
}
