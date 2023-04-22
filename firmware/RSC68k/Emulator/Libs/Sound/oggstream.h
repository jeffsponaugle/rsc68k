/*
	vim:ts=4 noexpandtab
    $Id: oggstream.h,v 1.3 2007/06/28 00:53:20 mcuddy Exp $

	File: oggstream.h -- header file for ogg streaming.  Requires oggtremor
	library.
*/
#ifndef _H_OGGSTREAM_
#define _H_OGGSTREAM_

#include "Libs/Sound/sound.h"
#include "Libs/oggtremor/ivorbiscodec.h"
#include "Libs/oggtremor/ivorbisfile.h"

typedef struct oggstream_f {
	OggVorbis_File ogg;				// ogg stream object
	GCFile *fp;						// file with ogg data
	INT16 *oggBuffer;				// buffer data read from ogg file
	INT32 oggChunkSize;				// how much data in chunk (in samples)
	BOOL eof;
	SoundStream *pStream;			// underlying stream
} OggStream;

/* 
   for OggStreamCreateFromMemFile
*/
typedef struct memfile_f {
	INT32 lCursor;
	INT32 lSize;
	void *pData;
} OggMemFile;

/* create a memfile object from a pointer and length.  if the passed in pointer
   is NULL, then 'len' bytes of memory will be allocated
*/
OggMemFile *OggMemFileCreate(void *ptr, UINT32 len);

void OggMemFileRewind(OggMemFile *fp);

/* does not free memory contained within ... */
extern void OggMemFileClose(OggMemFile *p);

/*
   Allocate memory to playback an ogg stream from a GCFile* 
*/
extern OggStream *OggStreamCreateFromFile(GCFile *fp,
								   INT32 nBuffers, 
								   UINT32 u32BufferSize);

/*
   Allocate memory to playback an ogg stream from a chunk of memory
*/
extern OggStream *OggStreamCreateFromMemFile(OggMemFile *fp, 
									  INT32 nBuffers, 
									  UINT32 u32BufferSize);

/*
   low-level function to create an OggStream which plays from 
   user provided callbacks (see ivorbisfile)
*/
extern OggStream *_OggStreamCreate(void *fp, 
							INT32 nBuffers, 
							UINT32 u32BufferSize, 
							ov_callbacks *ogg_callbacks);

/*
   Seek to 'ms' in stream.
*/
extern void OggStreamSeekMS(OggStream *pStream, INT32 ms);

/* 
   where are we?
 */
extern INT32 OggStreamTellMS(OggStream *pStream);

/*
   how long is stream (in ms)
*/
extern INT32 OggStreamLengthMS(OggStream *pStream);


/*
   Free memory associated with an ogg stream.  Doesn't close file.
*/
extern void OggStreamFree(OggStream *pOgg);

/*
   Start up an ogg stream.  call OggStreamPump() in your main loop.
   'pChan' is channel to stream sound to.  NULL = any free.  returns channel
   on success or NULL on error.
*/
extern SoundChannel *OggStreamPlay(SoundChannel *pChan, OggStream *pOgg, UINT32 nFlags);

/*
   fetch data from stream.  returns 1 at EOF, 0 if still playing
*/
extern BOOL OggStreamPump(OggStream *pOgg);

#endif /* _H_OGGSTREAM_ */
