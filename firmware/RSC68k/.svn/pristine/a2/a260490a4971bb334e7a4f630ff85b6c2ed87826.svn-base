#include "Startup/app.h"
#include "Win32/SDL/sdl.h"
#include "Win32/host.h"


#define NUM_BUFFERS 4
WAVEHDR sWaveHdr[NUM_BUFFERS];


HWAVEOUT hWaveOut;
static UINT32 sg_u32BufferPlayed = 0;
static BOOL sg_bHaltAudio = FALSE;
static BOOL sg_bSoundOpen = FALSE;



static void (*sg_psRenderData)(unsigned short int *ps16SignedBuffer,
							   UINT32 u32Samples) = NULL;

/* ************************************************************************* *\
** FUNCTION: SoundInstallCallback
\* ************************************************************************* */
void SoundInstallCallback(void (*Render)(unsigned short int *ps16SignedBuffer,
										  UINT32 u32Samples))
{
	sg_psRenderData = Render;
}

/* ************************************************************************* *\
** FUNCTION: waveOutProc
\* ************************************************************************* */
static void CALLBACK waveOutProc(HWAVEOUT hWaveOut,
								 UINT uMsg, 
								 DWORD dwInstance, 
								 DWORD dwParam1,
								 DWORD dwParam2)
{
	MMRESULT eResult;
//	FILE *fp = NULL;

	// If a block isn't finished, just return. Let's start playing the next block
	// and render the next block

	if (uMsg != WOM_DONE)
	{
		return;
	}

//	fp = fopen("u:/audio.raw", "ab+");
//	GCASSERT(fp);

	++sg_u32BufferPlayed;
	if (NUM_BUFFERS == sg_u32BufferPlayed)
	{
		sg_u32BufferPlayed = 0;
	}

	if (FALSE == sg_bHaltAudio)
	{
		eResult = waveOutWrite(hWaveOut,
							   &sWaveHdr[sg_u32BufferPlayed],
							   sizeof(sWaveHdr[sg_u32BufferPlayed]));

		if (eResult != MMSYSERR_NOERROR)
		{
			printf("eResult=%d\n", eResult);
			GCASSERT(0);
		}
	}

	// Now render the new data into the buffer that will be played
	if (sg_psRenderData)
	{
		sg_psRenderData((unsigned short int *) sWaveHdr[sg_u32BufferPlayed].lpData,
				 	    (sWaveHdr[sg_u32BufferPlayed].dwBufferLength / 2) / 2);
	}
	else
	{
		memset((void *) sWaveHdr[sg_u32BufferPlayed].lpData, 0, sWaveHdr[sg_u32BufferPlayed].dwBufferLength);
	}

//	fwrite(sWaveHdr[sg_u32BufferPlayed].lpData, 1, sWaveHdr[sg_u32BufferPlayed].dwBufferLength, fp);
//	fclose(fp);
}

/* ************************************************************************* *\
** FUNCTION: _EMUCloseSound
\* ************************************************************************* */
static void _EMUCloseSound(void)
{
	UINT32 u32Loop;
	MMRESULT eResult;

	if (!sg_bSoundOpen)
        { 
            return;
        }

	sg_bHaltAudio = TRUE;	// Stop all new audio from being rendered/requeued
	Sleep(NUM_BUFFERS * 50);

	eResult = waveOutReset(hWaveOut);
	if (eResult != MMSYSERR_NOERROR && eResult != MMSYSERR_INVALHANDLE)
	{
		printf("eResult=%d\n", eResult);
		GCASSERT(0);
	}

	eResult = waveOutClose(hWaveOut);
	if (eResult != MMSYSERR_NOERROR && eResult != MMSYSERR_INVALHANDLE)
	{
		printf("eResult=%d\n", eResult);
		GCASSERT(0);
	}

	for (u32Loop = 0; u32Loop < NUM_BUFFERS; u32Loop++)
	{
		free(sWaveHdr[u32Loop].lpData);
	}

	sg_bHaltAudio = FALSE;
	sg_bSoundOpen = FALSE;
} /* EMUCloseSound() */


static UINT32 sg_u32SampleRate = 44100;
static UINT32 sg_u32SampleCountPerChannel = 0;

/* ************************************************************************* *\
** FUNCTION: EMUOpenSound
\* ************************************************************************* */
static void EMUOpenSound(int iSampleRate, 
						 int iBufferSize, 
						 int iChans)
{
	MMRESULT eResult;
	WAVEFORMATEX sWaveFormat;
	UINT32 u32Loop;
	UINT32 u32Counter = 0;

	if (sg_bSoundOpen)
	{
		_EMUCloseSound();
	}

	sg_bSoundOpen = TRUE;

	// Clear out our wave out format structure
	memset((void *) &sWaveFormat, 0, sizeof(sWaveFormat));

	sg_u32SampleRate = (UINT32) iSampleRate;
	sWaveFormat.wFormatTag = WAVE_FORMAT_PCM;
	sWaveFormat.nChannels = iChans,
	sWaveFormat.nSamplesPerSec = iSampleRate;
	sWaveFormat.wBitsPerSample = 16;
	sWaveFormat.nBlockAlign = sWaveFormat.nChannels * (sWaveFormat.wBitsPerSample / 8); // 1;
	sWaveFormat.cbSize = 0;
	sWaveFormat.wFormatTag = WAVE_FORMAT_PCM;
	sWaveFormat.nAvgBytesPerSec = sWaveFormat.nSamplesPerSec * sWaveFormat.nBlockAlign; // iSampleRate * iChans * (sWaveFormat.wBitsPerSample / 8);

	eResult = waveOutOpen(&hWaveOut,
						  0,
						  &sWaveFormat,
						  (DWORD_PTR)waveOutProc,
						  0,
						  CALLBACK_FUNCTION);

	if (eResult != MMSYSERR_NOERROR )
	{
		printf("eResult=%d\n", eResult);
		GCASSERT(0);
	}

	// Now set up some buffers
	for (u32Loop = 0; u32Loop < NUM_BUFFERS; u32Loop++)
	{
		memset((void *) &sWaveHdr[u32Loop], 0, sizeof(sWaveHdr[u32Loop]));

		sWaveHdr[u32Loop].dwBufferLength = iBufferSize * (sWaveFormat.wBitsPerSample / 8) * iChans;
		sWaveHdr[u32Loop].lpData =  (void *) malloc(sWaveHdr[u32Loop].dwBufferLength);
		memset((void *) sWaveHdr[u32Loop].lpData, 0, sWaveHdr[u32Loop].dwBufferLength);
		sWaveHdr[u32Loop].dwUser = (UINT32) &sWaveHdr[u32Loop];

		eResult = waveOutPrepareHeader(hWaveOut,
									   &sWaveHdr[u32Loop],
									   sizeof(sWaveHdr[u32Loop]));

		if (eResult != MMSYSERR_NOERROR)
		{
			printf("eResult=%d\n", eResult);
			GCASSERT(0);
		}
	}

	sg_u32BufferPlayed = NUM_BUFFERS - 1;

	for (u32Loop = 0; u32Loop < NUM_BUFFERS; u32Loop++)
	{
		eResult = waveOutWrite(hWaveOut,
							   &sWaveHdr[u32Loop],
							   sizeof(sWaveHdr[u32Loop]));

		if (eResult != MMSYSERR_NOERROR )
		{
			printf("eResult=%d\n", eResult);
			GCASSERT(0);
		}
	}

	sg_u32SampleCountPerChannel = iBufferSize;
} /* EMUOpenSound() */


/* ************************************************************************* *\
** FUNCTION: GCSoundOpen
\* ************************************************************************* */
EGCResultCode GCSoundOpen(UINT32 u32BufferSampleCountPerChannel)
{
	EMUOpenSound(sg_u32SampleRate, 2048, 2);
	return(GC_OK);
}

/* ************************************************************************* *\
** FUNCTION: GCSoundSetCallback
\* ************************************************************************* */
void GCSoundSetCallback(void (*Handler)(INT16 *ps16Buffer, UINT32 u32StereoSamples))
{
	SoundInstallCallback((void (*)(unsigned short int *, UINT32)) Handler);
}

/* ************************************************************************* *\
** FUNCTION: GCSoundSetSampleRate
\* ************************************************************************* */
EGCResultCode GCSoundSetSampleRate(UINT32 u32SampleRate)
{
	return(GC_OK);
}

UINT32 GCSoundGetSampleRate(void)
{
	return(sg_u32SampleRate);
}

UINT32 GCSoundGetSampleCountPerChannel(void)
{
	return(sg_u32SampleCountPerChannel);
}

/* ************************************************************************* *\
** FUNCTION: GCSoundResume
\* ************************************************************************* */
EGCResultCode GCSoundResume(void)
{
	return(GC_OK);
}

/* ************************************************************************* *\
** FUNCTION: CSoundPause
\* ************************************************************************* */
void CSoundPause(void)
{
}

/* ************************************************************************* *\
** FUNCTION: GCSoundClose
\* ************************************************************************* */
EGCResultCode GCSoundClose(void)
{
	sg_u32SampleRate = 0;
	sg_u32SampleCountPerChannel = 0;
	_EMUCloseSound();
	return(GC_OK);
}

/* ************************************************************************* *\
** ************************************************************************* **
** EOF
** ************************************************************************* **
\* ************************************************************************* */
