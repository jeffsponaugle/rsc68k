#ifndef _SOUNDSTREAMWAVE_H_
#define _SOUNDSTREAMWAVE_H_

#define WAV_BUFFER_SIZE (300000)

typedef struct SSoundStreamWave
{
	FILEHANDLE hFile;
	UINT64 u64FilePosStart;
	UINT64 u64FilePosEnd;
	UINT64 u64BytesLeft;
	UINT32 u32SampleStep;
	UINT32 u32SampleCounter;
	UINT32 u32WavDataSize;
	UINT8 pu8WavData[WAV_BUFFER_SIZE];
	INT16 s16LastSampleLeft;
	INT16 s16LastSampleRight;

	struct SSoundStream *psSoundStream;
} SSoundStreamWave;

#define WAV_MAX_HEADER_SIZE (255)
#define WAV_HEADER_SUBCHUNK1SIZE_POS (16)
#define WAV_HEADER_SAMPLESIZE_POS (34)

extern ELCDErr FileIsWave(FILEHANDLE hFile, BOOL* pbIsWave);
extern ELCDErr SoundStreamWaveCreate(FILEHANDLE hFile, struct SSoundStream **ppsSoundStream);
extern ELCDErr SoundStreamWaveFillBuffer(struct SSoundStream *psSoundStream, INT16 *ps16Left, INT16 *ps16Right, UINT32 u32LengthInSamples, BOOL *pbComplete);
extern ELCDErr SoundStreamWaveShutdown(struct SSoundStream *psSoundStream);
extern ELCDErr SoundStreamWaveReset(struct SSoundStream *psSoundStream);
extern ELCDErr SoundStreamWaveGetLength(struct SSoundStream *psSoundStream, UINT64 *pu64Length);
extern ELCDErr SoundStreamWaveGetPosition(struct SSoundStream *psSoundStream, UINT64 *pu64Position);
extern ELCDErr SoundStreamWaveSetPosition(struct SSoundStream *psSoundStream, UINT64 u64Position);
extern ELCDErr SoundStreamWaveGetSampleCount(struct SSoundStream *psSoundStream, UINT32 *pu32SampleCount);

#endif	// #ifndef _SOUNDSTREAMWAVE_H_