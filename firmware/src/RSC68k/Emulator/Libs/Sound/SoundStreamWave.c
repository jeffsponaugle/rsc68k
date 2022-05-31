#include "Startup/app.h"
#include "Application/RSC68k.h"
#include "Libs/Sound/SoundStream.h"
#include "Libs/Sound/SoundStreamWave.h"

// Parse the file header to see if this is a WAVE file
//
ELCDErr FileIsWave(FILEHANDLE hFile, BOOL* pbIsWave)
{
	ELCDErr eErr = LERR_OK; // No error unless otherwise specified
	UINT8 u8FileHeader[WAVE_HEADER_SIZE];
	UINT32 u32BytesRead = 0;
	UINT64 u64FilePos = 0;

	// Let's be super-polite and make sure we restore the file's position when we're
	// done.
	// 
	eErr = FilePos(hFile, &u64FilePos);
	RETURN_ON_FAIL(eErr);

	eErr = FileSeek(hFile, 0, FILESEEK_SET);
	RETURN_ON_FAIL(eErr);

	eErr = FileRead(hFile, &u8FileHeader, WAVE_HEADER_SIZE, &u32BytesRead);
	RETURN_ON_FAIL(eErr);

	eErr = FileSeek(hFile, u64FilePos, FILESEEK_SET);
	RETURN_ON_FAIL(eErr);
	
	if (u32BytesRead != WAVE_HEADER_SIZE)
	{
		*pbIsWave = FALSE;
	}
	else
	{
		// A WAVE file will have the following header:
		//   0-3 "RIFF"
		//   4-7 <chunk data size>
		//   8-11 "WAVE"
		//
		if ((0 != memcmp(&u8FileHeader[0], (char*) "RIFF", 4)) ||
			(0 != memcmp(&u8FileHeader[8], (char*) "WAVE", 4)))
		{
			*pbIsWave = FALSE;
		}
		else
		{
			*pbIsWave = TRUE;
		}
	}

	return LERR_OK;
}

static ELCDErr SoundStreamWaveInit(SSoundStreamWave* psSoundStreamWave)
{
	ELCDErr eErr = LERR_OK;
	UINT8 u8WavHeader[WAV_MAX_HEADER_SIZE];
	UINT32 u32BytesRead;
	UINT8 *pu8ChunkPtr; // For scratch use
	UINT16 u16WavOffset;
	BOOL bFmtChunkFound = FALSE;
	BOOL bDataChunkFound = FALSE;

	GCASSERT(psSoundStreamWave);
	
	eErr = FileSeek(psSoundStreamWave->hFile, 0, FILESEEK_SET);
	RETURN_ON_FAIL(eErr);

	eErr = FileRead(psSoundStreamWave->hFile, u8WavHeader, WAV_MAX_HEADER_SIZE, &u32BytesRead);
	RETURN_ON_FAIL(eErr);

	if (WAV_MAX_HEADER_SIZE != u32BytesRead)
	{
		return LERR_SOUNDSTREAM_INIT_FAILED;
	}

	// Start the WAV offset off right after the RIFF chunk (12 bytes)
	//
	u16WavOffset = 12;

	// Search for the "fmt" chunk
	//
	while ((FALSE == bFmtChunkFound) || (FALSE == bDataChunkFound))
	{
		pu8ChunkPtr = u8WavHeader + u16WavOffset;
		
		// Fmt chunk - get settings for mono (bytes 10-11), sample rate 
		// (bytes 12-15, and bitness (bytes 22-23).
		//
		if (0 == memcmp((char*) pu8ChunkPtr, (char*) "fmt ", 4))
		{
			bFmtChunkFound = TRUE;
			
			switch (GET_16BIT_LE(pu8ChunkPtr, 10))
			{
				case 1:		psSoundStreamWave->psSoundStream->bMono = TRUE; break;
				case 2:		psSoundStreamWave->psSoundStream->bMono = FALSE; break;
				default:	return LERR_SOUNDSTREAM_INIT_FAILED; // Only support mono or stereo
			}
			
			psSoundStreamWave->psSoundStream->u32SampleRate = GET_32BIT_LE(pu8ChunkPtr, 12);
			
			switch (GET_16BIT_LE(pu8ChunkPtr, 22))
			{
				case 8:		psSoundStreamWave->psSoundStream->b8Bit = TRUE; break;
				case 16:	psSoundStreamWave->psSoundStream->b8Bit = FALSE; break;
				default:	return LERR_SOUNDSTREAM_INIT_FAILED;	// Not a valid size
			}
		}
		// Data chunk - get PCM data start position.  This should be the last chunk.
		// This code is making the assumption that it is.  I don't know what the point
		// to do otherwise would be (what's the point of a "header" at the end of a file?)
		//
		else if (0 == memcmp((char*) pu8ChunkPtr, (char*) "data", 4))
		{
			bDataChunkFound = TRUE;
			psSoundStreamWave->u64FilePosStart = u16WavOffset + 8;
			psSoundStreamWave->u64FilePosEnd = psSoundStreamWave->u64FilePosStart + GET_32BIT_LE(pu8ChunkPtr, 4);
			break; // All done - no more chunks
		}
		
		// Move on to the next chunk - 4 bytes for header, 4 bytes for length
		//
		u16WavOffset += GET_32BIT_LE(pu8ChunkPtr, 4) + 8;

		// If we're over our size limit, we didn't find all the info we needed in the
		// header size we assumed.  Bail in this case.
		//
		if (u16WavOffset >= WAV_MAX_HEADER_SIZE)
		{
			return LERR_SOUNDSTREAM_INIT_FAILED;
		}
	}

	// Since we read past the start of the data, reset the file point back to that position
	//
	eErr = FileSeek(psSoundStreamWave->hFile, psSoundStreamWave->u64FilePosStart, FILESEEK_SET);
	RETURN_ON_FAIL(eErr);
	
	psSoundStreamWave->u64BytesLeft = psSoundStreamWave->u64FilePosEnd - psSoundStreamWave->u64FilePosStart;

	// Calculate the sample "step" size so we know how often to repeat samples in case the WAV
	// sample rate doesn't match the system's rate.
	//
	psSoundStreamWave->u32SampleStep = (UINT32) (((UINT64) psSoundStreamWave->psSoundStream->u32SampleRate << 16) / GCSoundGetSampleRate());
	psSoundStreamWave->u32SampleCounter = 0;
	psSoundStreamWave->s16LastSampleLeft = 0;
	psSoundStreamWave->s16LastSampleRight = 0;
	
	psSoundStreamWave->u32WavDataSize = 0;
	
	psSoundStreamWave->psSoundStream->bValid = TRUE;

	return LERR_OK;
}

ELCDErr SoundStreamWaveFillBuffer(SSoundStream *psSoundStream, INT16 *ps16Left, INT16 *ps16Right, UINT32 u32LengthInSamples, BOOL *pbComplete)
{
	ELCDErr eErr = LERR_OK;
	UINT8 u8BytesPerSample;
	UINT32 u32BytesToRead, u32BytesRead, u32SamplesToProcess, u32BytesUsed, u32BytesNeeded, i;
	SSoundStreamWave *psSoundStreamWave = psSoundStream->uSoundStreamSpecific.psSoundStreamWave;
	
	// By default, let's say we're complete unless we say otherwise
	//
	*pbComplete = TRUE;
	
	// If there's nothing left to read, fill and exit (this quite possibly shouldn't happen,
	// but let's be safe...
	//
	if (0 == psSoundStreamWave->u64BytesLeft)
	{
		memset(ps16Left, 0, u32LengthInSamples * sizeof(SoundSample));
		memset(ps16Right, 0, u32LengthInSamples * sizeof(SoundSample));
		*pbComplete = TRUE;
		return LERR_OK;
	}

	// Figure out how much to read
	//
	u8BytesPerSample = SoundStreamGetBytesPerSample(psSoundStream);

	// The number of bytes needed to store the sample must fit within our data buffer.  If this goes
	// off then WAV_BUFFER_SIZE is too small.
	//
	GCASSERT(u32LengthInSamples * u8BytesPerSample <= WAV_BUFFER_SIZE);

	// The data buffer may have some data left over from last time we read.  Fill in the rest of the
	// buffer now.
	//
	u32BytesNeeded = u32LengthInSamples * u8BytesPerSample;

#if 0
	if (psSoundStreamWave->u32SampleStep > (1 << 16))
	{
		u32BytesNeeded = (UINT32) (((UINT64) u32BytesNeeded * psSoundStreamWave->u32SampleStep) >> 16) + 1;
	}
#endif

	// Here we go from 64-bit land (for large files) to 32-bit land (for how much we'll actually deal with now) - if the 
	// bytes left is >4G then the min will still always be <4G
	//
	u32BytesToRead = (UINT32) MIN(psSoundStreamWave->u64BytesLeft, (u32LengthInSamples * u8BytesPerSample) - psSoundStreamWave->u32WavDataSize);
	
	eErr = FileRead(psSoundStreamWave->hFile, psSoundStreamWave->pu8WavData + psSoundStreamWave->u32WavDataSize, u32BytesToRead, &u32BytesRead);
	RETURN_ON_FAIL(eErr);
	
	psSoundStreamWave->u32WavDataSize += u32BytesRead;

	u32SamplesToProcess = MIN(u32LengthInSamples, psSoundStreamWave->u32WavDataSize / u8BytesPerSample);
	u32BytesUsed = 0;

	if (FALSE == psSoundStream->b8Bit)
	{
		INT16* ps16WavDataPtr = (INT16*) psSoundStreamWave->pu8WavData;
		
		for (i = 0; i < u32SamplesToProcess; i++)
		{
			// Normalize the sample rate.  If we don't exceed the counter, just repeat the sample value.
			//
			psSoundStreamWave->u32SampleCounter += psSoundStreamWave->u32SampleStep;

			if (psSoundStreamWave->u32SampleCounter >= (1 << 16))
			{
				psSoundStreamWave->u32SampleCounter -= (1 << 16);
			
				*ps16Left = *(ps16WavDataPtr++);
				u32BytesUsed += 2;

				if (FALSE == psSoundStream->bMono)
				{
					*ps16Right = *(ps16WavDataPtr++);
					u32BytesUsed += 2;
				}
				else
				{
					*ps16Right = *ps16Left;
				}

				psSoundStreamWave->s16LastSampleLeft = *ps16Left;
				psSoundStreamWave->s16LastSampleRight = *ps16Right;
			}
			else
			{
				*ps16Left = psSoundStreamWave->s16LastSampleLeft;
				*ps16Right = psSoundStreamWave->s16LastSampleRight;
			}

			ps16Left++;
			ps16Right++;
		}
	}
	else
	{
		UINT8* pu8WavDataPtr = psSoundStreamWave->pu8WavData;
		INT8 s8Sample;

		for (i = 0; i < u32SamplesToProcess; i++)
		{
			// Normalize the sample rate.  If we don't exceed the counter, just repeat the sample value.
			//
			psSoundStreamWave->u32SampleCounter += psSoundStreamWave->u32SampleStep;

			if (psSoundStreamWave->u32SampleCounter >= (1 << 16))
			{
				psSoundStreamWave->u32SampleCounter -= (1 << 16);
				
				s8Sample = *(pu8WavDataPtr++) - 128;
				*ps16Left = SIGNED_8_TO_16(s8Sample);
				u32BytesUsed++;

				if (FALSE == psSoundStream->bMono)
				{
					s8Sample = *(pu8WavDataPtr++) - 128;
					*ps16Right = SIGNED_8_TO_16(s8Sample);
					u32BytesUsed++;
				}
				else
				{
					*ps16Right = *ps16Left;
				}

				psSoundStreamWave->s16LastSampleLeft = *ps16Left;
				psSoundStreamWave->s16LastSampleRight = *ps16Right;
			}
			else
			{
				*ps16Left = psSoundStreamWave->s16LastSampleLeft;
				*ps16Right = psSoundStreamWave->s16LastSampleRight;
			}

			ps16Left++;
			ps16Right++;
		}
	}

	// In case we fell short, fill in the rest of the buffers with zeros.  Also update
	// the number of bytes left.
	//
	if (u32SamplesToProcess < u32LengthInSamples)
	{
		psSoundStreamWave->u64BytesLeft = 0; 
		*pbComplete = TRUE;
		return LERR_OK;
	}

	// Copy the unused bytes to the start of the buffer for next time
	//
	GCASSERT(u32BytesUsed <= psSoundStreamWave->u32WavDataSize);
	memmove((UINT8*) psSoundStreamWave->pu8WavData, (UINT8*) psSoundStreamWave->pu8WavData + u32BytesUsed, psSoundStreamWave->u32WavDataSize - u32BytesUsed);
	psSoundStreamWave->u32WavDataSize -= u32BytesUsed;

	psSoundStreamWave->u64BytesLeft -= u32BytesRead;
	
	if (psSoundStreamWave->u64BytesLeft)
	{
		*pbComplete = FALSE;
	}

	return LERR_OK;
}

ELCDErr SoundStreamWaveShutdown(SSoundStream* psSoundStream)
{
	ELCDErr eErr = LERR_OK;
	
	GCASSERT(psSoundStream);
	GCASSERT(ESOUNDSTREAM_TYPE_WAVE == psSoundStream->eType);
	
	if (HANDLE_INVALID != psSoundStream->uSoundStreamSpecific.psSoundStreamWave->hFile)
	{
		FileClose(&psSoundStream->uSoundStreamSpecific.psSoundStreamWave->hFile);
	}
	
	if (psSoundStream->uSoundStreamSpecific.psSoundStreamWave)
	{
		GCFreeMemory(psSoundStream->uSoundStreamSpecific.psSoundStreamWave);
	}
	
	eErr = SoundStreamFree(psSoundStream);
	return(eErr);
}

ELCDErr SoundStreamWaveReset(SSoundStream* psSoundStream)
{
	return SoundStreamWaveInit(psSoundStream->uSoundStreamSpecific.psSoundStreamWave);
}

static UINT64 SoundStreamWaveGetTime(SSoundStream* psSoundStream, UINT64 u64FileBytes)
{
	UINT8 u8BytesPerSample;
	UINT32 u32SampleRate;
	UINT64 u64Result;
	
	u8BytesPerSample = SoundStreamGetBytesPerSample(psSoundStream);
	GCASSERT(u8BytesPerSample > 0);

	u32SampleRate = psSoundStream->u32SampleRate; // Samples/sec
	u32SampleRate /= 1000; // Samples/msec
	GCASSERT(u32SampleRate > 0);

	u64Result = u64FileBytes; // Bytes
	u64Result /= u8BytesPerSample; // Bytes -> Samples
	u64Result /= u32SampleRate; // Samples -> Msec

	return u64Result;
}

ELCDErr SoundStreamWaveGetLength(SSoundStream* psSoundStream, UINT64 *pu64Length)
{
	SSoundStreamWave *psWave = NULL;

	GCASSERT(psSoundStream);
	GCASSERT(pu64Length);

	psWave = psSoundStream->uSoundStreamSpecific.psSoundStreamWave;
	GCASSERT(psWave);
	
	GCASSERT(psWave->u64FilePosEnd >= psWave->u64FilePosStart);
	*pu64Length = SoundStreamWaveGetTime(psSoundStream, psWave->u64FilePosEnd - psWave->u64FilePosStart);

	return LERR_OK;
}

ELCDErr SoundStreamWaveGetPosition(SSoundStream* psSoundStream, UINT64 *pu64Position)
{
	SSoundStreamWave *psWave = NULL;

	GCASSERT(psSoundStream);
	GCASSERT(pu64Position);

	psWave = psSoundStream->uSoundStreamSpecific.psSoundStreamWave;
	GCASSERT(psWave);

	GCASSERT(psWave->u64FilePosEnd >= psWave->u64BytesLeft);
	*pu64Position = SoundStreamWaveGetTime(psSoundStream, psWave->u64FilePosEnd - psWave->u64BytesLeft);
	
	return LERR_OK;
}

ELCDErr SoundStreamWaveSetPosition(SSoundStream* psSoundStream, UINT64 u64Position)
{
	ELCDErr eErr;
	UINT64 u64Result;
	SSoundStreamWave *psWave = NULL;

	GCASSERT(psSoundStream);

	psWave = psSoundStream->uSoundStreamSpecific.psSoundStreamWave;
	GCASSERT(psWave);
	
	// Get the position in bytes
	//
	u64Result = u64Position; // Msec
	u64Result *= psSoundStream->u32SampleRate; // Samples * 1000
	u64Result /= 1000; // Samples
	u64Result *= SoundStreamGetBytesPerSample(psSoundStream); // Bytes
	u64Result += psWave->u64FilePosStart; // Bytes from any initial offset

	// If the position is beyond EOF, return a failed an error
	//
	if (u64Result > psWave->u64FilePosEnd)
	{
		return LERR_SOUNDSTREAM_POSITION_OUT_OF_RANGE;
	}
		
	// Invalidate anything we've already read but not yet played
	//
	psWave->u32WavDataSize = 0;

	// Move the file position
	//
	eErr = FileSeek(psWave->hFile, u64Result, FILESEEK_SET);
	RETURN_ON_FAIL(eErr);

	psWave->u64BytesLeft = psWave->u64FilePosEnd - u64Result;

	return LERR_OK;
}

ELCDErr SoundStreamWaveGetSampleCount(struct SSoundStream *psSoundStream, UINT32 *pu32SampleCount)
{
	SSoundStreamWave *psSoundStreamWave = psSoundStream->uSoundStreamSpecific.psSoundStreamWave;
	GCASSERT(psSoundStreamWave);

	*pu32SampleCount = (UINT32) (psSoundStreamWave->u64FilePosEnd - psSoundStreamWave->u64FilePosStart) / SoundStreamGetBytesPerSample(psSoundStream);
	return LERR_OK;
}

///////////////////////////

static SSoundStreamFunctions sg_sSoundStreamWaveFunctions =
{
	SoundStreamWaveFillBuffer,
	SoundStreamWaveShutdown,
	SoundStreamWaveReset,
	SoundStreamWaveGetLength,
	SoundStreamWaveGetPosition,
	SoundStreamWaveSetPosition,
	SoundStreamWaveGetSampleCount
};

ELCDErr SoundStreamWaveCreate(FILEHANDLE hFile, struct SSoundStream **ppsSoundStream)
{
	ELCDErr eErr = LERR_OK;
	SSoundStreamWave *psSoundStreamWave = NULL;

	*ppsSoundStream = NULL;
	
	eErr = SoundStreamAllocate(ppsSoundStream);
	GOTO_ON_FAIL(eErr != LERR_OK, WaveCreateFail)

	psSoundStreamWave = MemAlloc(sizeof(*psSoundStreamWave));
	GOTO_ON_FAIL(NULL == psSoundStreamWave, WaveCreateFail)

	// Hook the two together
	//
	psSoundStreamWave->psSoundStream = *ppsSoundStream;
	(*ppsSoundStream)->uSoundStreamSpecific.psSoundStreamWave = psSoundStreamWave;
	(*ppsSoundStream)->eType = ESOUNDSTREAM_TYPE_WAVE;
	(*ppsSoundStream)->psFunctions = &sg_sSoundStreamWaveFunctions;
	
	// Set up anything else
	//
	psSoundStreamWave->hFile = hFile;
	(*ppsSoundStream)->bValid = FALSE;

	// Initialize!
	//
	eErr = SoundStreamWaveInit(psSoundStreamWave);
	GOTO_ON_FAIL(eErr, WaveCreateFail)
	
	return LERR_OK;

WaveCreateFail:
	if (*ppsSoundStream)
	{
		SoundStreamWaveShutdown(*ppsSoundStream); // Ignore this error, the original error is more interesting
		*ppsSoundStream = NULL;
	}

	return eErr;
}
