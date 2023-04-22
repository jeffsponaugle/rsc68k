#include "Startup/app.h"
#include "Libs/Gfx/GraphicsLib.h"
#include "Libs/libgif/gif.h"
#include "Application/RSC68k.h"

typedef enum
{
	EGIF_87A,
	EGIF_89A
} EGIFType;

#ifdef _WIN32
#pragma pack(1)
#endif

typedef __packed struct SColorTuple
{
	UINT8 u8Red;
	UINT8 u8Green;
	UINT8 u8Blue;
} SColorTuple;

typedef struct SImageBlockHeader
{
	UINT16 u16XPos;
	UINT16 u16YPos;
	UINT16 u16XSize;
	UINT16 u16YSize;
	UINT8 u8BitField;
} SImageBlockHeader;

#ifdef _WIN32
#pragma pack()
#endif

typedef struct SGIFData
{
	EGIFType eType;
	UINT16 u16XSize;
	UINT16 u16YSize;
	UINT8 u8BitField;
	UINT8 u8BackgroundColor;
	UINT8 u8PixelAspectRatio;
	SColorTuple *psGlobalColorTable;
	UINT32 u32GlobalColorTableEntries;
} SGIFData;

typedef struct SDecodeStruct
{
	UINT8 *pu8ImagePtr;
	UINT32 u32ImageBytesRemaining;
	UINT8 u8BytesInBlockRemaining;
	UINT8 u8BitsRemaining;
	INT32 s32Code;
} SDecodeStruct;

static INT32 GIFGetNextCode(UINT8 u8CodeLength,
							SDecodeStruct *psDecode)
{
	INT32 s32Code;

	// Check to see if we have anything remaining
	while (psDecode->u8BitsRemaining < u8CodeLength)
	{
		if (0 == psDecode->u8BytesInBlockRemaining)
		{
			// Need to read another block's worth of data
			psDecode->u8BytesInBlockRemaining = *psDecode->pu8ImagePtr;
			psDecode->pu8ImagePtr++;

			if (0 == psDecode->u32ImageBytesRemaining)
			{
				// No more data at all
				return(-1);
			}

			psDecode->u32ImageBytesRemaining--;

			if (0 == psDecode->u8BytesInBlockRemaining)
			{
				GCASSERT(0);	// Test me later
				DebugOut(" No more data to read - bail out\n");
				// No more data
				return(-1);
			}

			if (psDecode->u8BytesInBlockRemaining > psDecode->u32ImageBytesRemaining)
			{
				DebugOut(" Data bytes remaining in block > bytes remaining in image file\n");
				return(-1);
			}
		}

		// We need to shift in some new data

		psDecode->s32Code += (*psDecode->pu8ImagePtr << psDecode->u8BitsRemaining);

		psDecode->pu8ImagePtr++;
		psDecode->u32ImageBytesRemaining--;
		psDecode->u8BytesInBlockRemaining--;
		psDecode->u8BitsRemaining += 8;
	}

	s32Code = psDecode->s32Code & ((1 << u8CodeLength) - 1);
	psDecode->s32Code >>= u8CodeLength;
	psDecode->u8BitsRemaining -= u8CodeLength;

	return(s32Code);
}

static void ConvertPalette(UINT16 *pu16ColorPalette,
						   SColorTuple *psColorPalette)
{
	UINT32 u32Loop;

	// Translate 24bpp to 16bpp
	for (u32Loop = 0; u32Loop < (1 << 8); u32Loop++)
	{
		UINT16 u16Red;
		UINT16 u16Green;
		UINT16 u16Blue;

		u16Red = (psColorPalette[u32Loop].u8Red) + 7;
		if (u16Red > 0xff)
		{
			u16Red = 0xff;
		}
		u16Red >>= 3;

		u16Green = (psColorPalette[u32Loop].u8Green) + 3;
		if (u16Green > 0xff)
		{
			u16Green = 0xff;
		}
		u16Green >>= 2;

		u16Blue = (psColorPalette[u32Loop].u8Blue) + 7;
		if (u16Blue > 0xff)
		{
			u16Blue = 0xff;
		}
		u16Blue >>= 3;

		pu16ColorPalette[u32Loop] = (u16Red << 11) | (u16Green << 5) | u16Blue;
	}
}

#define	MAX_CODES	4095
#define	NULL_CODE	-1

static INT32 GIFDecodeImage(UINT8 *pu8ImageBaseOrigin,
							UINT32 u32ImagePitch,
							SImageBlockHeader *psBlockHeader,
							UINT8 **ppu8ImagePtr,
							UINT32 *pu32ImageSize,
							SColorTuple *psColorPalette,
							BOOL bInterlaced,
							UINT32 u32TransparentIndex,
							UINT8 *pu8TransparentMaskOrigin)
{
	SDecodeStruct sDecode;
	UINT8 u8MinimumCodeSize;
	UINT32 u32PixelsRemaining = psBlockHeader->u16XSize * psBlockHeader->u16YSize;
	INT32 s32Result = 0;
	UINT16 *pu16Prefix = NULL;
	UINT8 *pu8Suffix = NULL;
	UINT8 *pu8Stack = NULL;
	UINT8 *pu8SP = NULL;
	UINT32 u32Loop = 0;
	INT32 s32Clear;
	INT32 s32Available;
	INT32 s32OldCode = NULL_CODE;
	INT32 s32First = 0;
	INT32 s32EndOfInfo = 0;
	UINT8 u8CurrentSize;
	UINT32 u32XPos = 0;
	UINT32 u32YPos = 0;
	UINT8 *pu8ImagePtr = pu8ImageBaseOrigin;
	UINT16 *pu16DeinterlaceTable = NULL;
	UINT8 *pu8TransparentMaskPtr;

	pu8TransparentMaskPtr = pu8TransparentMaskOrigin + (psBlockHeader->u16XPos) +
							(psBlockHeader->u16YPos * u32ImagePitch);

	if (bInterlaced)
	{
		UINT16 u16Row = 0;

		pu16DeinterlaceTable = MemAlloc(sizeof(*pu16DeinterlaceTable) * psBlockHeader->u16YSize);
		if (NULL == pu16DeinterlaceTable)
		{
			goto exitDecode;
		}

		for (u32Loop = 0; u32Loop < psBlockHeader->u16YSize; u32Loop += 8, u16Row++)
		{
			pu16DeinterlaceTable[u16Row] = u32Loop;
		}

		for (u32Loop = 4; u32Loop < psBlockHeader->u16YSize; u32Loop += 8, u16Row++)
		{
			pu16DeinterlaceTable[u16Row] = u32Loop;
		}

		for (u32Loop = 2; u32Loop < psBlockHeader->u16YSize; u32Loop += 4, u16Row++)
		{
			pu16DeinterlaceTable[u16Row] = u32Loop;
		}

		for (u32Loop = 1; u32Loop < psBlockHeader->u16YSize; u32Loop += 2, u16Row++)
		{
			pu16DeinterlaceTable[u16Row] = u32Loop;
		}

		GCASSERT(u16Row == psBlockHeader->u16YSize);
	}

	pu16Prefix = MemAlloc(sizeof(*pu16Prefix) * (MAX_CODES + 1));
	if (NULL == pu16Prefix)
	{
		DebugOut(" Out of memory attempting to allocate a prefix\n");
		s32Result = -1;
		goto exitDecode;
	}

	pu8Suffix = MemAlloc(sizeof(*pu8Suffix) * (MAX_CODES + 1));
	if (NULL == pu8Suffix)
	{
		DebugOut(" Out of memory attempting to allocate a suffix\n");
		s32Result = -1;
		goto exitDecode;
	}

	pu8Stack = MemAlloc(sizeof(*pu8Stack) * (MAX_CODES + 1));
	if (NULL == pu8Stack)
	{
		DebugOut(" Out of memory attempting to allocate a stack\n");
		s32Result = -1;
		goto exitDecode;
	}

	// Get our minimum code size
	u8MinimumCodeSize = *(*ppu8ImagePtr);
	u8CurrentSize = u8MinimumCodeSize + 1;
	(*ppu8ImagePtr)++;
	if (0 == *(pu32ImageSize))
	{
		DebugOut(" Truncated image - No data left to read minimum code size\n");
		goto exitDecode;
	}
	--*(pu32ImageSize);

	if ((u8MinimumCodeSize < 2) || (u8MinimumCodeSize > 9))
	{
		DebugOut(" Invalid minimum code size of %d\n", u8MinimumCodeSize);
		goto exitDecode;
	}

	// Seed our decode structure with incoming data
	memset((void *) &sDecode, 0, sizeof(sDecode));

	sDecode.pu8ImagePtr = *ppu8ImagePtr;
	sDecode.u32ImageBytesRemaining = *pu32ImageSize;

	s32Clear = 1 << u8MinimumCodeSize;
	s32EndOfInfo = s32Clear + 1;
	s32Available = s32Clear + 2;
	s32OldCode = 0;
	pu8SP = pu8Stack;
	for (u32Loop = 0; u32Loop < (UINT32) s32Clear; u32Loop++)
	{
		pu8Suffix[u32Loop] = (UINT8) u32Loop;
	}

	while (1)
	{
		INT32 s32Code;

		// Go snag the next code to decode
		s32Code = GIFGetNextCode(u8CurrentSize,
								 &sDecode);

		if (s32Code < 0)
		{
			s32Result = s32Code;
			goto exitDecode;
		}
		else
		if ((s32Code > s32Available) || (s32Code == s32EndOfInfo))
		{
			break;
		}
		else
		if (s32Code == s32Clear)
		{
			u8CurrentSize = (u8MinimumCodeSize + 1);
			s32Clear = 1 << u8MinimumCodeSize;
			s32EndOfInfo = s32Clear + 1;
			s32Available = s32Clear + 2;
			s32OldCode = NULL_CODE;
		}
		else
		if (NULL_CODE == s32OldCode)
		{
			*pu8SP = pu8Suffix[s32Code];
			pu8SP++;
			s32OldCode = s32Code;
			s32First = s32Code;
		}
		else
		{
			INT32 s32InCode;

			s32InCode = s32Code;
			if (s32Code == s32Available)
			{
				*pu8SP = (UINT8) s32First;
				pu8SP++;
				s32Code = s32OldCode;
			}

			while (s32Code > s32Clear)
			{
				*pu8SP = pu8Suffix[s32Code];
				++pu8SP;
				s32Code = pu16Prefix[s32Code];
			}

			s32First = pu8Suffix[s32Code];
			if (s32Available >= (MAX_CODES + 1))
			{
				break;
			}

			*pu8SP = (UINT8) s32First;
			++pu8SP;
			pu16Prefix[s32Available] = (UINT16) s32OldCode;
			pu8Suffix[s32Available] = (UINT8) s32First;
			s32Available++;
			if (((s32Available & ((1 << u8CurrentSize) - 1)) == 0) &&
				(s32Available < (MAX_CODES + 1)))
			{
				u8CurrentSize++;
			}

			s32OldCode = s32InCode;
		}

		while (pu8SP != pu8Stack)
		{
			pu8SP--;

			// Copy Mr. Pixel
			if (*pu8SP != u32TransparentIndex)
			{
				*pu8TransparentMaskPtr = 0x00;
				*pu8ImagePtr = *pu8SP;
			}
			else
			{
				// Don't do anything
				*pu8TransparentMaskPtr = 0xff;
			}

			++pu8ImagePtr;
			++pu8TransparentMaskPtr;
			u32PixelsRemaining--;
			u32XPos++;
			if (u32XPos >= psBlockHeader->u16XSize)
			{
				u32XPos = 0;
				u32YPos++;
				if ((u32YPos >= psBlockHeader->u16YSize) && (u32PixelsRemaining))
				{
					DebugOut(" Image too large!\n");
					s32Result = -1;
					goto exitDecode;
				}

				if (FALSE == bInterlaced)
				{
					pu8ImagePtr += (u32ImagePitch - psBlockHeader->u16XSize);
					pu8TransparentMaskPtr += (u32ImagePitch - psBlockHeader->u16XSize);
				}
				else
				{
					// If we're interlaced, time to swizzle
					pu8ImagePtr = pu8ImageBaseOrigin + (pu16DeinterlaceTable[u32YPos] * u32ImagePitch);
					pu8TransparentMaskPtr = pu8TransparentMaskOrigin + (pu16DeinterlaceTable[u32YPos] * u32ImagePitch);
				}
			}

		}
	}

	// Everything done!
	s32Result = 0;

	// Chew up remaining bytes in block
	sDecode.pu8ImagePtr += sDecode.u8BytesInBlockRemaining;
	sDecode.u32ImageBytesRemaining -= sDecode.u8BytesInBlockRemaining;

	// This means we should see a 0. If not, we have a stream error
	if ((*sDecode.pu8ImagePtr) ||
		(0 == sDecode.u32ImageBytesRemaining))
	{
		s32Result = -1;
		goto exitDecode;
	}
	else
	{
		// Looks like we have a 0x00 in stream, which means no more bytes in this block
		sDecode.pu8ImagePtr++;
		sDecode.u32ImageBytesRemaining--;
	}

	// Store our moved pointer back to the original
	*ppu8ImagePtr = sDecode.pu8ImagePtr;
	*pu32ImageSize = sDecode.u32ImageBytesRemaining;

exitDecode:
	if (pu16Prefix)
	{
		GCFreeMemory(pu16Prefix);
	}

	if (pu8Suffix)
	{
		GCFreeMemory(pu8Suffix);
	}

	if (pu8Stack)
	{
		GCFreeMemory(pu8Stack);
	}

	if (pu16DeinterlaceTable)
	{
		GCFreeMemory(pu16DeinterlaceTable);
	}

	return(s32Result);
}

static void GIFFillImage(UINT8 *pu8ImageBasePtr,
						 UINT8 *pu8ImageMaskPtr,
						 SGIFData *psGIFData,
						 UINT32 u32ImagePitch,
						 UINT32 u32TransparentIndex)
{
	UINT32 u32YPos;
	UINT32 u32XPos;

	for (u32YPos = 0; u32YPos < psGIFData->u16YSize; u32YPos++)
	{
		for (u32XPos = 0; u32XPos < psGIFData->u16XSize; u32XPos++)
		{
			*pu8ImageBasePtr = psGIFData->u8BackgroundColor;
			++pu8ImageBasePtr;

			if (*pu8ImageBasePtr == u32TransparentIndex)
			{
				*pu8ImageMaskPtr = 0xff;
			}
			else
			{
				*pu8ImageMaskPtr = 0x00;
			}
		}

		pu8ImageBasePtr += (u32ImagePitch - psGIFData->u16XSize);
		pu8ImageMaskPtr += (u32ImagePitch - psGIFData->u16XSize);
	}
}

BOOL GfxLoadGIF(UINT8 *pu8ImagePtr,
			    UINT32 u32ImageSize,
			    SImageGroup *psImageGroup,
				EGCResultCode *peResult)
{
	SGIFData sGIFData;
	SColorTuple *psLocalColorTable = NULL;
	UINT8 *pu8ImageBase = NULL;
	UINT8 *pu8TransparentMaskBase = NULL;
	UINT32 u32ImagePitch = 0;
	UINT32 u32TransparentIndex = 0xffffffff;
	UINT32 u32DelayTime = 0;
	UINT32 u32Disposal = 0;
	EGCResultCode eResult = GC_OK;
	SImageGroupLink *psLastGroupLink = NULL;
	UINT32 u32LastDelayTime = 0;

	// Look for GIF87 or GIF89a header
	if (u32ImageSize < 6)
	{
		// Not a GIF image - too small!
		return(FALSE);
	}

	memset((void *) &sGIFData, 0, sizeof(sGIFData));

	if (memcmp((void *) pu8ImagePtr,
			   "GIF89a",
			   GIF_HEADER_SIZE) == 0)
	{
		// 89A version
		sGIFData.eType = EGIF_89A;
	}
	else
	if (memcmp((void *) pu8ImagePtr,
			   "GIF87a",
			   GIF_HEADER_SIZE) == 0)
	{
		// 87A version
		sGIFData.eType = EGIF_87A;
	}
	else
	{
		return(FALSE);
	}
	
	pu8ImagePtr += GIF_HEADER_SIZE;
	u32ImageSize -= GIF_HEADER_SIZE;

	// Now get the width
	if (u32ImageSize < sizeof(sGIFData.u16XSize))
	{
		eResult = GC_CORRUPT_IMAGE;
		goto decodeErrorExit;
	}

	// Start off with 0 frames
	GCASSERT(0 == psImageGroup->u32FrameCount);

	sGIFData.u16XSize = (*pu8ImagePtr) | (*(pu8ImagePtr + 1) << 8);
	pu8ImagePtr += sizeof(sGIFData.u16XSize);
	u32ImageSize -= sizeof(sGIFData.u16XSize);

	// Now the height
	if (u32ImageSize < sizeof(sGIFData.u16YSize))
	{
		eResult = GC_CORRUPT_IMAGE;
		goto decodeErrorExit;
	}

	sGIFData.u16YSize = (*pu8ImagePtr) | (*(pu8ImagePtr + 1) << 8);
	pu8ImagePtr += sizeof(sGIFData.u16YSize);
	u32ImageSize -= sizeof(sGIFData.u16YSize);

	u32ImagePitch = (sGIFData.u16XSize + 3) &0xfffffffc;
	pu8ImageBase = MemAlloc(u32ImagePitch * sGIFData.u16YSize * sizeof(*pu8ImageBase));
	if (NULL == pu8ImageBase)
	{
		eResult = GC_CORRUPT_IMAGE;
		goto decodeErrorExit;
	}

	// Now the bit field
	if (u32ImageSize < sizeof(sGIFData.u8BitField))
	{
		eResult = GC_CORRUPT_IMAGE;
		goto decodeErrorExit;
	}

	// Bit 0-2  - Size of global color table: (2^(1+n))
	// Bit 3    - Sort flag to global color table
	// Bits 4-6 - Color resolution
	// Bit 7	- Global color table flag

	sGIFData.u8BitField = *pu8ImagePtr;
	pu8ImagePtr++;
	u32ImageSize -= sizeof(sGIFData.u8BitField);

	// Set the global
#define	GLOBAL_COLOR_TABLE_ENTRIES_MASK	(0x07)
	sGIFData.u32GlobalColorTableEntries = (1 << ((sGIFData.u8BitField & GLOBAL_COLOR_TABLE_ENTRIES_MASK) + 1));

	// Background color
	if (u32ImageSize < sizeof(sGIFData.u8BackgroundColor))
	{
		eResult = GC_CORRUPT_IMAGE;
		goto decodeErrorExit;
	}

	sGIFData.u8BackgroundColor = *pu8ImagePtr;
	pu8ImagePtr++;
	u32ImageSize -= sizeof(sGIFData.u8BackgroundColor);

	// Pixel aspect ratio
	if (u32ImageSize < sizeof(sGIFData.u8PixelAspectRatio))
	{
		eResult = GC_CORRUPT_IMAGE;
		goto decodeErrorExit;
	}

	sGIFData.u8PixelAspectRatio = *pu8ImagePtr;
	pu8ImagePtr++;
	u32ImageSize -= sizeof(sGIFData.u8PixelAspectRatio);

	// Let's make sure that we have enough bytes for the global color table entry table
	if (u32ImageSize < (sGIFData.u32GlobalColorTableEntries * sizeof(*sGIFData.psGlobalColorTable)))
	{
		eResult = GC_CORRUPT_IMAGE;
		goto decodeErrorExit;
	}

	// Allocate some space for the color table
	sGIFData.psGlobalColorTable = MemAlloc(sGIFData.u32GlobalColorTableEntries * sizeof(*sGIFData.psGlobalColorTable));
	if (NULL == sGIFData.psGlobalColorTable)
	{
		eResult = GC_OUT_OF_MEMORY;
		goto decodeErrorExit;
	}

#define	GLOBAL_COLOR_TABLE	0x80

	// If we have a global color table, let's read it in
	if (sGIFData.u8BitField & GLOBAL_COLOR_TABLE)
	{
		// Now let's copy it in to the data table
		memcpy((void *) sGIFData.psGlobalColorTable,
			   (void *) pu8ImagePtr,
			   (sGIFData.u32GlobalColorTableEntries * sizeof(*sGIFData.psGlobalColorTable)));

		pu8ImagePtr += (sGIFData.u32GlobalColorTableEntries * sizeof(*sGIFData.psGlobalColorTable));
		u32ImageSize -= (sGIFData.u32GlobalColorTableEntries * sizeof(*sGIFData.psGlobalColorTable));
	}

	pu8TransparentMaskBase = MemAlloc(u32ImagePitch * sGIFData.u16YSize * sizeof(*pu8TransparentMaskBase));
	if (NULL == pu8TransparentMaskBase)
	{
		eResult = GC_OUT_OF_MEMORY;
		goto decodeErrorExit;
	}

	// Start off with the correct color
	GIFFillImage(pu8ImageBase,
				 pu8TransparentMaskBase,
				 &sGIFData,
				 u32ImagePitch,
				 0xffffffff);

#define	IMAGE_BLOCK			0x2c
#define	EXTENSION_BLOCK		0x21
#define	TERMINATOR_BLOCK	0x3b

	// Loop through and eat up all of the headers
	while (u32ImageSize)
	{
		UINT8 u8Block;

		u8Block = *pu8ImagePtr;
		++pu8ImagePtr;
		--u32ImageSize;

		if (IMAGE_BLOCK == u8Block)
		{
			SImageBlockHeader sImageBlockHeader;
			INT32 s32Result;
			SColorTuple *psColorPalette;
			UINT8 *pu8ImageBasePtr = NULL;
			UINT8 *pu8TransparentMaskBasePtr = pu8TransparentMaskBase;
			BOOL bInterlaced = FALSE;
			UINT8 *pu8DisposalFrame = NULL;
			SImage *psImage;

			// 0x2c - Image block size
			if (u32ImageSize < sizeof(SImageBlockHeader))
			{
				eResult = GC_CORRUPT_IMAGE;
				goto decodeErrorExit;
			}

			memset((void *) &sImageBlockHeader, 0, sizeof(sImageBlockHeader));

			sImageBlockHeader.u16XPos = (*pu8ImagePtr) | (*(pu8ImagePtr + 1) << 8);
			pu8ImagePtr += sizeof(sImageBlockHeader.u16XPos);
			sImageBlockHeader.u16YPos = (*pu8ImagePtr) | (*(pu8ImagePtr + 1) << 8);
			pu8ImagePtr += sizeof(sImageBlockHeader.u16YPos);
			sImageBlockHeader.u16XSize = (*pu8ImagePtr) | (*(pu8ImagePtr + 1) << 8);
			pu8ImagePtr += sizeof(sImageBlockHeader.u16XSize);
			sImageBlockHeader.u16YSize = (*pu8ImagePtr) | (*(pu8ImagePtr + 1) << 8);
			pu8ImagePtr += sizeof(sImageBlockHeader.u16YSize);
			sImageBlockHeader.u8BitField = *pu8ImagePtr;
			++pu8ImagePtr;
			u32ImageSize -= sizeof(sImageBlockHeader);

			// Bits 2-0 - Size of local color table
			// Bit 4-3	- Reserved
			// Bit 5	- Sort flag
			// Bit 6	- Interlaced?
			// Bit 7	- Local color flag table

#define	LOCAL_COLOR_FLAG				0x80
#define	INTERLACED_FLAG					0x40
#define	LOCAL_COLOR_TABLE_SIZE_MASK		0x07

			// If we have a color table, then load up the color table
			if (sImageBlockHeader.u8BitField & LOCAL_COLOR_FLAG)
			{
				UINT32 u32EntrySize;

				u32EntrySize = (sizeof(*psLocalColorTable) * (1 << ((sImageBlockHeader.u8BitField & LOCAL_COLOR_TABLE_SIZE_MASK) + 1)));

				// Let's see if we have enough room for the local color table
				if (u32ImageSize < u32EntrySize)
				{
					eResult = GC_CORRUPT_IMAGE;
					goto decodeErrorExit;
				}

				psLocalColorTable = MemAlloc(u32EntrySize);
				if (NULL == psLocalColorTable)
				{
					eResult = GC_OUT_OF_MEMORY;
					goto decodeErrorExit;
				}

				memcpy((void *) psLocalColorTable,
					   (void *) pu8ImagePtr,
					   u32EntrySize);

				pu8ImagePtr += u32EntrySize;
				u32ImageSize -= u32EntrySize;
			}

			psColorPalette = sGIFData.psGlobalColorTable;
			if (psLocalColorTable)
			{
				psColorPalette = psLocalColorTable;
			}

			// Is it interlaced?
			bInterlaced = FALSE;
			if (sImageBlockHeader.u8BitField & INTERLACED_FLAG)
			{
				bInterlaced = TRUE;
			}

			// Adjust our pu8ImageBasePtr
			pu8ImageBasePtr = pu8ImageBase + (sImageBlockHeader.u16XPos) +
								(sImageBlockHeader.u16YPos * u32ImagePitch);

			// If we're in disposal 3, let's copy off the segment of the frame
			// for later restoration.

			if (3 == u32Disposal)
			{
				UINT8 *pu8DisposalPtr = NULL;
				UINT8 *pu8SrcFrame = NULL;
				UINT32 u32Loop = sImageBlockHeader.u16YSize;

				pu8DisposalFrame = MemAlloc(sImageBlockHeader.u16XSize * sImageBlockHeader.u16YSize * sizeof(*pu8DisposalFrame));
				if (NULL == pu8DisposalFrame)
				{
					eResult = GC_OUT_OF_MEMORY;
					goto decodeErrorExit;
				}

				pu8DisposalPtr = pu8DisposalFrame;
				pu8SrcFrame = pu8ImageBasePtr;

				while (u32Loop--)
				{
					// Copy this line to the disposal buffer
					memcpy((void *) pu8DisposalPtr, pu8SrcFrame, sImageBlockHeader.u16XSize * sizeof(*pu8DisposalPtr));
					pu8DisposalPtr += sImageBlockHeader.u16XSize;
					pu8SrcFrame += u32ImagePitch;
				}
			}

			s32Result = GIFDecodeImage(pu8ImageBasePtr,
									   u32ImagePitch,
									   &sImageBlockHeader,
									   &pu8ImagePtr,
									   &u32ImageSize,
									   psColorPalette,
									   bInterlaced,
									   u32TransparentIndex,
									   pu8TransparentMaskBase);

			if (s32Result < 0)
			{
				// Get rid of our disposal frame if we've encountered an error
				if (pu8DisposalFrame)
				{
					GCFreeMemory(pu8DisposalFrame);
				}

				eResult = GC_CORRUPT_IMAGE;
				goto decodeErrorExit;
			}

			psImage = GfxAllocateImage();
			if (NULL == psImage)
			{
				eResult = GC_OUT_OF_MEMORY;
				goto decodeErrorExit;
			}

			// Allocate some space for the mask and for the final image
			psImage->pu8ImageData = MemAlloc(u32ImagePitch * sGIFData.u16YSize * sizeof(*pu8ImageBase));
			if (NULL == psImage->pu8ImageData)
			{
				eResult = GC_OUT_OF_MEMORY;
				goto decodeErrorExit;
			}

			memcpy((void *) psImage->pu8ImageData,
				   (void *) pu8ImageBase,
				   u32ImagePitch * sGIFData.u16YSize * sizeof(*pu8ImageBase));

			psImage->u32XSize = sGIFData.u16XSize;
			psImage->u32YSize = sGIFData.u16YSize;
			psImage->u32Pitch = u32ImagePitch;

			if (u32TransparentIndex != 0xffffffff)
			{
				// We have a transparency mask. Let's assign it!
				psImage->pu8Transparent = MemAlloc(u32ImagePitch * sGIFData.u16YSize * sizeof(*pu8TransparentMaskBase));
				if (NULL == psImage->pu8Transparent)
				{
					eResult = GC_OUT_OF_MEMORY;
					goto decodeErrorExit;
				}

				memcpy((void *) psImage->pu8Transparent,
					   (void *) pu8TransparentMaskBase,
					   u32ImagePitch * sGIFData.u16YSize * sizeof(*pu8TransparentMaskBase));
			}
			else
			{
				// No transparency mask. Just ignore it.
				psImage->u16TransparentIndex  = 0xffff;
			}

			// Allocate some space for the palettte
			psImage->pu16Palette = MemAlloc(sizeof(*psImage->pu16Palette) * (1 << 8));
			if (NULL == psImage->pu16Palette)
			{
				if (pu8DisposalFrame)
				{
					GCFreeMemory(pu8DisposalFrame);
				}

				eResult = GC_OUT_OF_MEMORY;
				goto decodeErrorExit;
			}

			ConvertPalette(psImage->pu16Palette,
						   psColorPalette);

			if (NULL == psLastGroupLink)
			{
				// Link it to the head of the group
				psLastGroupLink = MemAlloc(sizeof(*psLastGroupLink));
				if (NULL == psLastGroupLink)
				{
					eResult = GC_OUT_OF_MEMORY;
					goto decodeErrorExit;
				}

				// Hook up the first image
				psImageGroup->psLinkHead = psLastGroupLink;
				psImageGroup->psCurrentImage = psImage;
			}
			else
			{
				// Link it to the head of the group
				psLastGroupLink->psNextLink = MemAlloc(sizeof(*psLastGroupLink));
				psLastGroupLink->psImage->psNextLink = psImage;
				psImage->psPriorLink = psLastGroupLink->psImage;
				psLastGroupLink = psLastGroupLink->psNextLink;
				if (NULL == psLastGroupLink)
				{
					eResult = GC_OUT_OF_MEMORY;
					goto decodeErrorExit;
				}
			}

			psLastGroupLink->psImage = psImage;
			psImage->eSourceImageFileType = GFXTYPE_GIF;
			psImage->u32FrameNumber = psImageGroup->u32FrameCount;
			psImageGroup->psLinkTail = psLastGroupLink;
			psImage = NULL;

			// Increase the number of frames that we've successfully decoded
			psImageGroup->u32FrameCount++;

			// # Of milliseconds to delay. See the following URL for playback
			// details:
			// http://news.deviantart.com/article/27613/

			if (u32DelayTime < 6)
			{
				u32DelayTime = 10;
			}

			u32LastDelayTime = u32LastDelayTime + (u32DelayTime * 10);
			psLastGroupLink->psImage->u32MSTimestamp = u32LastDelayTime;
			psLastGroupLink->psImage->u32FrameTime = (u32DelayTime * 10);
			psImageGroup->u32TotalAnimTime = u32LastDelayTime;

//			DebugOut("%s: Delay=%u\n", __FUNCTION__, psLastGroupLink->psImage->u32MSTimestamp);

			// If we're of type disposition #2, then we need to fill the updated block
			// to the background color index.
			if (2 == u32Disposal)
			{
				GIFFillImage(pu8ImageBase,
							 pu8TransparentMaskBase,
							 &sGIFData,
							 u32ImagePitch,
							 u32TransparentIndex);
			}
			else
			if (3 == u32Disposal)
			{
				UINT8 *pu8DisposalPtr = NULL;
				UINT8 *pu8SrcFrame = NULL;
				UINT32 u32Loop = sImageBlockHeader.u16YSize;

				// If we're disposal mode 3, put the old frame segment back
				GCASSERT(pu8DisposalFrame);

				pu8DisposalPtr = pu8DisposalFrame;
				pu8SrcFrame = pu8ImageBasePtr;

				while (u32Loop--)
				{
					// Copy this line to the disposal buffer
					memcpy((void *) pu8SrcFrame, pu8DisposalPtr, sImageBlockHeader.u16XSize * sizeof(*pu8DisposalPtr));
					pu8DisposalPtr += sImageBlockHeader.u16XSize;
					pu8SrcFrame += u32ImagePitch;
				}

				GCFreeMemory(pu8DisposalFrame);
				pu8DisposalFrame = NULL;
			}

		}
		else
		if (EXTENSION_BLOCK == u8Block)
		{
			UINT8 u8ControlLabel;
			UINT8 u8BlockSize;

			// 0x21 - Extension - let's figure out what type it is (and if we care)
			if (u32ImageSize < 2)
			{
				// Unexpected EOF
				goto decodeErrorExit;
			}

			// Let's get our label
			u8ControlLabel = *pu8ImagePtr;
			pu8ImagePtr++;
			u32ImageSize--;

			u8BlockSize = *pu8ImagePtr;
			pu8ImagePtr++;
			u32ImageSize--;

#define BLOCK_GCE		0xf9
#define BLOCK_PTE		0x01
#define BLOCK_AE		0xff
#define BLOCK_COMMENT	0xfe

			if (u8BlockSize > u32ImageSize)
			{
				eResult = GC_CORRUPT_IMAGE;
				goto decodeErrorExit;
			}

#define	TRANSPARENT_INDEX_MASK	0x01

			// Let's figure out what this field is
			if (BLOCK_GCE == u8ControlLabel)
			{
				// Graphics Control Extension
				if (*pu8ImagePtr & TRANSPARENT_INDEX_MASK)
				{
					u32TransparentIndex = *(pu8ImagePtr + 3);
				}
				else
				{
					u32TransparentIndex = 0xffffffff;
				}

				u32DelayTime = *(pu8ImagePtr + 1) |
							   (*(pu8ImagePtr + 2) << 8);

				u32Disposal = (*(pu8ImagePtr) >> 2) & 0x07;
			}
			else
			if (BLOCK_PTE == u8ControlLabel)
			{
				// Plain text extension
			}
			else
			if ((BLOCK_COMMENT == u8ControlLabel) ||
				(BLOCK_AE == u8ControlLabel))
			{
				// Adjust this backwards so we align properly
				--pu8ImagePtr;
				++u32ImageSize;

				// We just skip the block comment(s)
				do
				{
					if (u32ImageSize < sizeof(u8BlockSize))
					{
						goto decodeErrorExit;
					}

					u8BlockSize = *pu8ImagePtr;
					pu8ImagePtr++;
					u32ImageSize--;

					if (u8BlockSize)
					{
						if (u32ImageSize < u8BlockSize)
						{
							eResult = GC_CORRUPT_IMAGE;
							goto decodeErrorExit;
						}

						pu8ImagePtr += u8BlockSize;
						u32ImageSize -= u8BlockSize;
					}
				}
				while (u8BlockSize);

				// Back up so we're pointing to the 0
				--pu8ImagePtr;
				++u32ImageSize;
			}
			else
			{
				// WTF For now
				eResult = GC_CORRUPT_IMAGE;
				goto decodeErrorExit;
			}

			// Skip over the record
			pu8ImagePtr += u8BlockSize;
			u32ImageSize -= u8BlockSize;

			// We had better be looking at a 0x00 now
			if (0 == u32ImageSize)
			{
				eResult = GC_CORRUPT_IMAGE;
				goto decodeErrorExit;
			}

			if (*pu8ImagePtr != 0x00)
			{
				eResult = GC_CORRUPT_IMAGE;
				goto decodeErrorExit;
			}

			pu8ImagePtr++;
			u32ImageSize--;
		}
		else
		if (TERMINATOR_BLOCK == u8Block)
		{
			// 0x3b - Terminator!
			break;
		}
		else
		{
			// Unknown block type
			eResult = GC_CORRUPT_IMAGE;
			goto decodeErrorExit;
		}
	}

	if (sGIFData.psGlobalColorTable)
	{
		GCFreeMemory((void *) sGIFData.psGlobalColorTable);
	}

	if (pu8TransparentMaskBase)
	{
		GCFreeMemory(pu8TransparentMaskBase);
	}

	if (pu8ImageBase)
	{
		GCFreeMemory(pu8ImageBase);
	}

	*peResult = GC_OK;
	return(TRUE);

decodeErrorExit:
	if (psLocalColorTable)
	{
		GCFreeMemory((void *) psLocalColorTable);
	}

	if (pu8TransparentMaskBase)
	{
		GCFreeMemory(pu8TransparentMaskBase);
	}

	if (pu8ImageBase)
	{
		GCFreeMemory(pu8ImageBase);
	}

	if (sGIFData.psGlobalColorTable)
	{
		GCFreeMemory((void *) sGIFData.psGlobalColorTable);
	}

	// If we have at least 1 decoded frame and our error code is a corrupt image,
	// then let's say we've decoded a partial image
	if ((GC_CORRUPT_IMAGE == eResult) &&
		(psImageGroup->u32FrameCount))
	{
		eResult = GC_PARTIAL_IMAGE;
	}

	// If we got at least one frame, indicate it was a partial image
	if ((GC_OUT_OF_MEMORY == eResult) &&
		(psImageGroup->u32FrameCount));
	{
		eResult = GC_PARTIAL_IMAGE;
	}

	*peResult = eResult;
	return(TRUE);
}
