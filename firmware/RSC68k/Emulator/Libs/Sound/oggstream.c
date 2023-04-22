/*
	vim:ts=4 noexpandtab
    $Id: oggstream.c,v 1.3 2007/06/28 00:53:20 mcuddy Exp $

	File: oggstream.h -- header file for ogg streaming.  Requires oggtremor
	library.
*/
#include <string.h>	// mem*
#include "Startup/app.h"
#include "Libs/Sound/oggstream.h"

#define OGG_BUFFER_SIZE (8192)
// read up to half-of-a buffer
#define OGG_READ_SIZE (OGG_BUFFER_SIZE>>1)


// XXX -- these functions use 'int' because that's what the ogg library
// wants.

// wrappers for I/O functions because on the GC board, the GCFile ops are 
// SWIs, which can't be made into function pointers.
static int fseek_wrap( void *vp, ogg_int64_t off, int whence )
{
	GCFile *fp = (GCFile*)vp;
	return (int) GCfseek(fp,off,whence);
}

static int fread_wrap( void *ptr, size_t n, size_t siz, void *fp)
{
	return (int) GCfread(ptr, n,siz,(GCFile*)fp);
}

static int fclose_wrap( void *ptr )
{
	// DON't do anything -- it's our responsibility to close the FILE.
	// return GCfclose(ptr);
	return 0;
}

static long ftell_wrap( void *fp)
{
	return (long) GCftell( (GCFile*)fp);
}

static ov_callbacks gcfile_callbacks = {
	(size_t (*)(void*,size_t,size_t,void*)) fread_wrap,
	(int (*)(void*, ogg_int64_t, int)) fseek_wrap,
	(int (*)(void*)) fclose_wrap,
	(long (*)(void*)) ftell_wrap,
};

// wrappers for I/O functions which stream through a buffer of allocated
// memory
static int mseek_wrap( void *vp, ogg_int64_t loff, int whence )
{
	INT32 off = (INT32) loff;
	OggMemFile *fp = (OggMemFile*)vp;
	switch(whence)
	{
		case SEEK_CUR:
			off = fp->lCursor + off;
			break;
		case SEEK_SET:
			break;
		case SEEK_END:
			off = fp->lSize + whence;
			break;
	}
	if (off < 0 || off > fp->lSize) return -1;
	fp->lCursor = (INT32) off;
	return 0;
}

static int mread_wrap( void *ptr, size_t size, size_t nmemb, void *vp)
{
	OggMemFile *fp = (OggMemFile*)vp;
	INT32 i;
	unsigned char *p = (unsigned char *)ptr;
	unsigned char *q = (unsigned char *)fp->pData + fp->lCursor;

	// easy case, the read will succeed.
	i = size * nmemb;
	if (fp->lCursor + i <= fp->lSize)
	{
		memcpy(p, q, i);
		fp->lCursor += i;
		return nmemb;
	}

	// it's going to be a short read...
	for (i = 0 ; i < (INT32) nmemb; i++ ) 
	{
		if (size + fp->lCursor > (size_t) fp->lSize)
			size = (size_t) (fp->lSize - fp->lCursor);
		if ( size == 0 ) break;
		memcpy(p, q, size);
		fp->lCursor += size;
		p += size;
		q += size;
	}
	return (int) i;
}

static int mclose_wrap( void *ptr )
{
	return 0;
}

static long mtell_wrap( void *vp)
{
	OggMemFile *fp = (OggMemFile *)vp;
	return (long) fp->lCursor;
}

static ov_callbacks memfile_callbacks = {
	(size_t (*)(void*,size_t,size_t,void*)) mread_wrap,
	(int (*)(void*, ogg_int64_t, int)) mseek_wrap,
	(int (*)(void*)) mclose_wrap,
	(long (*)(void*)) mtell_wrap,
};

/*
   these four functions are needed by the ogg library.
   I don't really like the 'realloc' implementation, but the BIOS doesn't 
   provide us with a better solution, so alloc(new) copy(new,old) free(old)
   is the best we can do.
*/

void *_ogg_malloc(unsigned long bytes)
{
	return MemAlloc( bytes );
}

void _ogg_free(void *ptr)
{
	MemAlloc( ptr );
}

void *_ogg_calloc(unsigned long bytes, unsigned long chunks)
{
	return MemAlloc( bytes * chunks );
}

void *_ogg_realloc(void *oldPtr, unsigned long bytes)
{
	void *ptr = MemAlloc(bytes);
	if (ptr == NULL) return NULL;
	memcpy(ptr, oldPtr, (int) bytes);
	GCFreeMemory(oldPtr);
	return ptr;
}

/*
   Allocate memory to playback an ogg stream
*/
OggStream *OggStreamCreateFromFile(GCFile *fp, INT32 nBuffers, UINT32 u32BufferSize)
{
	return _OggStreamCreate((void*)fp,nBuffers,u32BufferSize,&gcfile_callbacks);
}

void OggMemFileRewind(OggMemFile *fp)
{
	fp->lCursor = 0;
}

OggMemFile *OggMemFileCreate(void *ptr, UINT32 len)
{
	OggMemFile *p;
	UINT32 allocSize = sizeof(OggMemFile);

	if (ptr == NULL)
		allocSize += len;
	
	p = (OggMemFile*) MemAlloc( allocSize );
	if (p == NULL) 
	{
		SoundSetLastError(SERR_MALLOC_FAIL);
		return NULL;
	}
	if (ptr == NULL)
	{
		// point just after data.
		ptr = (void*) (p+1);
	}
	p->pData = ptr;
	p->lCursor = 0;
	p->lSize = len;
	return p;
}

void OggMemFileClose(OggMemFile *p)
{
	if (p)
	{
		GCFreeMemory(p);
	}
}

OggStream *OggStreamCreateFromMemFile(OggMemFile *fp, INT32 nBuffers, UINT32 u32BufferSize)
{
	return _OggStreamCreate((void*)fp,nBuffers,u32BufferSize,&memfile_callbacks);
}

OggStream *_OggStreamCreate(void *fp, INT32 nBuffers, UINT32 u32BufferSize, ov_callbacks *ogg_callbacks)
{
	OggStream *p = MemAlloc(sizeof(OggStream) + (OGG_BUFFER_SIZE * sizeof(unsigned short)));
	vorbis_info *vi;
	int rc;

	if (p == NULL) {
		SoundSetLastError(SERR_MALLOC_FAIL);
		return NULL;
	}

	p->oggBuffer = (SoundSample*) (p + 1);		// point at bufffer
	p->eof = FALSE;
	p->oggChunkSize = 0;

	// bah -- ogg_callbacks is structure passed by value
	rc = ov_open_callbacks( (void *) fp, &(p->ogg), NULL, 0, *ogg_callbacks );
	if (rc < 0) 
	{
		SoundSetLastError(SERR_OGG_STREAM_ERROR);

		GCFreeMemory(p);
		return NULL;
	}

	// get sample rate and channels from ogg file
	vi = ov_info(&(p->ogg),-1);
	GCASSERT(vi->channels == 1 || vi->channels == 2);

	// create sound library stream object 
	p->pStream = SoundStreamCreate( vi->channels == 1 ? TRUE : FALSE, 
								    nBuffers, vi->rate, u32BufferSize );

	if (p->pStream == NULL) 
	{
		ov_clear(&(p->ogg));
		GCFreeMemory(p);
		// g_eSoundError was set by SoundStreamCreate.
		return NULL;
	}

	// good to go!
	return p;
}

/* seek to 'ms' position in milliseconds.  if at EOF, reset 
   EOF flag.
*/
void OggStreamSeekMS(OggStream *pStream, INT32 ms)
{
	int rc;
	GCASSERT(pStream);

	rc = ov_time_seek(&pStream->ogg, ms);
	if (rc == 0 && pStream->eof)
		pStream->eof = FALSE;
}

INT32 OggStreamTellMS(OggStream *pStream)
{
	INT32 rc;
	GCASSERT(pStream);

	rc = (INT32) ov_time_tell(&pStream->ogg);
	return rc;
}

INT32 OggStreamLengthMS(OggStream *pStream)
{
	INT32 rc;
	GCASSERT(pStream);

	rc = (INT32) ov_time_total(&pStream->ogg, -1);
	return rc;
}

/*
   Free memory associated with an ogg stream.  Doesn't close file.
*/
void OggStreamFree(OggStream *pOgg)
{
	if (pOgg)
	{
		if (pOgg->pStream) GCFreeMemory(pOgg->pStream);
		pOgg->pStream = NULL;
		ov_clear(&pOgg->ogg);
		GCFreeMemory(pOgg);
	}
}

/*
   Start up an ogg stream.  call OggStreamPump() in your main loop.
   'nChan' is channel to stream sound to.  NULL = create new.  returns channel
   on success or NULL on error.
*/
SoundChannel *OggStreamPlay(SoundChannel *pChan, OggStream *pOgg, UINT32 nFlags)
{
	return SoundStreamPlay(pChan, pOgg->pStream, nFlags);
}

/*
   fetch data from stream.  returns 1 at EOF.
*/
BOOL OggStreamPump(OggStream *pOgg)
{
	SoundSample *pL, *pR; 
	signed short *q;
	SoundWav *pW;
	INT32 n, chunk, sampNeeded;
	int current_section;

	if (pOgg->eof) return pOgg->eof;

	pW = SoundStreamPump(pOgg->pStream);
	if (! pW) return pOgg->eof;

	// point at output sample data.
	pL = pW->pLeft;
	pR = pW->pRight;
	chunk = pW->nLength;
	sampNeeded = pW->bMono ? chunk : chunk * 2;
	
	// stream at end?  fill buffer with NULL

	// have to get some data ('chunk' samples) from ogg..
	while (chunk > 0) {
		// if we're not at EOF and we need to get more data into the OGG buffer,
		// do that.
		if (! pOgg->eof)
		{
			while (pOgg->oggChunkSize < sampNeeded) 
			{

				// get some more data from the ogg decoder.
				n = ov_read( &(pOgg->ogg),
							 (char*)(pOgg->oggBuffer+pOgg->oggChunkSize), 
							 OGG_READ_SIZE, &current_section);
				// DebugOut("Ogg Chunk: %d\n", n);
				n >>= 1;	// bytes to samples.
			

				// fwrite(pOgg->oggBuffer+pOgg->oggChunkSize, n, 2, outogg);

				// no more samples?  mark that we got eof.
				if (n == 0)
				{
					SoundStreamEOF(pOgg->pStream);
					pOgg->eof = TRUE;
					break;
				}

				// 'n' is bytes. chunksize is in samples.
				pOgg->oggChunkSize += n;
			}
		}

#define MIN(a,b) ((a) > (b) ? (b) : (a))
		// now, we have some data in the ogg buffer, copy as much 
		// as we can out to the pWav object.
		if (pW->bMono)
		{
			// copy out as many samples as we need into wav buffer.
			// 'n' is now samples, not bytes.
			n = MIN(chunk, pOgg->oggChunkSize);
			q = pOgg->oggBuffer;
			chunk -= n;
			pOgg->oggChunkSize -= n;
			while (n--)
			{
				*pL++ = *q++;		// copy out samples.
			}
		}
		else
		{
			// stereo -- oggBuffer has pairs of samples
			// copy out as many samples as we need into wav buffer.
			// 'n' is now samples, not bytes.
			n = MIN(chunk, pOgg->oggChunkSize>>1);
			q = pOgg->oggBuffer;
			chunk -= n;
			pOgg->oggChunkSize -= (n<<1);
			while (n--)
			{
				*pL++ = *q++;		// copy out samples.
				*pR++ = *q++;		// copy out samples.
			}
		}
		// if we still have samples left in the ogg buffer, move 'em down
		// q points to data in ogg buffer
		if (pOgg->oggChunkSize)
		{
			signed short *p;
			p = pOgg->oggBuffer;
			n = pOgg->oggChunkSize;
			while ( n-- ) {
				*p++ = *q++;
			}
		}

		// if we're at EOF and there's no more data, fill buffers 
		// with zero.
		if (pOgg->eof && pOgg->oggChunkSize == 0)
		{
			if (pW->bMono)
			{
				// mono stream
				while (chunk--)
				{
					*pL++ = 0;
				}
			} 
			else 
			{
				// stereo stream
				while (chunk--)
				{
					*pL++ = 0;
					*pR++ = 0;
				}
			}
		}
	}
	return pOgg->eof;
}

