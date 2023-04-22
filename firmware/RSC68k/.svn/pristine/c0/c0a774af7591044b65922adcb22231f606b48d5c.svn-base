// vim:ts=4:noexpandtab
#include "Startup/app.h"
#include "Libs/Gfx/GraphicsLib.h"
#include "Include/png.h"
#include "Libs/libjpeg/jpeglib.h"
#include "Libs/libjpeg/cdjpeg.h"
#include "Libs/libgif/gif.h"
#include "Application/RSC68k.h"
//#include "Libs/languages/SymbolMgr.h"
//#include "Libs/languages/BASIC/BASIC.h"

#ifndef _WIN32
extern void	ARMBlitTransparent(INT32 s32XBlitSize, UINT8 *pu8TranslucentMask, UINT16 *pu16Src, UINT16 *pu16Dest);
extern void ARMGfxDirtyBufferBlit(SDirtyBuffer *psDirtyBuffer);
#endif

static UINT16 *sg_pu16SurfaceLayer = NULL;				// Combined surface layer
static UINT32 sg_u32SurfaceLayerPitch = 0;				// Surface layer pitch (in pixels, not bytes)
static UINT8 *sg_pu8DirtyBuffer = NULL;					// Dirty buffer pointer
static UINT32 sg_u32DirtyBufferPitch = 0;				// Dirty buffer pitch
static UINT32 sg_u32DirtyBufferSize = 0;				// Size of dirty buffer (in bytes)
static UINT32 sg_u32SurfaceXSize = 0;					// How big is our surface X size wise?
static UINT32 sg_u32SurfaceYSize = 0;					// How big is our surface Y size wise?

static SGfxFile *sg_psFileHead = NULL;

#define DIRTY_X_BLOCK_SIZE			8
#define DIRTY_Y_BLOCK_SIZE			8

// File type checkout
#define FILE_TYPE_CHECK_BUFFER_SIZE	131072
static UINT8 sg_u8FileTypeBuffer[FILE_TYPE_CHECK_BUFFER_SIZE];

typedef struct SGfxImageInfo
{
	EGfxFileType eFileType;
	UINT32 u32XSize;
	UINT32 u32YSize;
	UINT32 u32Pages;
	UINT8 u8BPP;
} SGfxImageInfo;

// Layer definitions

static SLayer *sg_psLayerHead;

#ifdef _WIN32
#pragma pack(1)
#endif

typedef __packed struct _TGAInfo
{
	UINT8
	id_length,
	colormap_type,
	image_type;

	UINT16
	colormap_index,
	colormap_length;

	UINT8
	colormap_size;

	UINT16
	x_origin,
	y_origin,
	width,
	height;

	UINT8
	bits_per_pixel,
	attributes;
} TGAInfo;

#ifdef _WIN32
#pragma pack()
#endif

#define TGANoColormap 0
#define TGAColormap 1
#define TGARGB 2
#define TGAMonochrome 3
#define TGARLEColormap  9
#define TGARLERGB  10
#define TGARLEMonochrome  11

static void *PNGAlloc(png_structp png_ptr, UINT32 u32Size)
{
	return(MemAllocNoClear(u32Size));
}

static void PNGFree(png_structp png_ptr, void *pvMemory)
{
	GCFreeMemory(pvMemory);
}

#define RED_SHIFT	11

const UINT16 sg_u16RedGradientSaturation[0x40] =
{
		0 << RED_SHIFT,
		1 << RED_SHIFT,
		2 << RED_SHIFT,
		3 << RED_SHIFT,
		4 << RED_SHIFT,
		5 << RED_SHIFT,
		6 << RED_SHIFT,
		7 << RED_SHIFT,
		8 << RED_SHIFT,
		9 << RED_SHIFT,
		10 << RED_SHIFT,
		11 << RED_SHIFT,
		12 << RED_SHIFT,
		13 << RED_SHIFT,
		14 << RED_SHIFT,
		15 << RED_SHIFT,
		16 << RED_SHIFT,
		17 << RED_SHIFT,
		18 << RED_SHIFT,
		19 << RED_SHIFT,
		20 << RED_SHIFT,
		21 << RED_SHIFT,
		22 << RED_SHIFT,
		23 << RED_SHIFT,
		24 << RED_SHIFT,
		25 << RED_SHIFT,
		26 << RED_SHIFT,
		27 << RED_SHIFT,
		28 << RED_SHIFT,
		29 << RED_SHIFT,
		30 << RED_SHIFT,
		31 << RED_SHIFT,
		31 << RED_SHIFT,
		31 << RED_SHIFT,
		31 << RED_SHIFT,
		31 << RED_SHIFT,
		31 << RED_SHIFT,
		31 << RED_SHIFT,
		31 << RED_SHIFT,
		31 << RED_SHIFT,
		31 << RED_SHIFT,
		31 << RED_SHIFT,
		31 << RED_SHIFT,
		31 << RED_SHIFT,
		31 << RED_SHIFT,
		31 << RED_SHIFT,
		31 << RED_SHIFT,
		31 << RED_SHIFT,
		31 << RED_SHIFT,
		31 << RED_SHIFT,
		31 << RED_SHIFT,
		31 << RED_SHIFT,
		31 << RED_SHIFT,
		31 << RED_SHIFT,
		31 << RED_SHIFT,
		31 << RED_SHIFT,
		31 << RED_SHIFT,
		31 << RED_SHIFT,
		31 << RED_SHIFT,
		31 << RED_SHIFT,
		31 << RED_SHIFT,
		31 << RED_SHIFT,
		31 << RED_SHIFT,
		31 << RED_SHIFT
};

#define GREEN_SHIFT	5

const UINT16 sg_u16GreenGradientSaturation[] =
{
		0 << GREEN_SHIFT,
		1 << GREEN_SHIFT,
		2 << GREEN_SHIFT,
		3 << GREEN_SHIFT,
		4 << GREEN_SHIFT,
		5 << GREEN_SHIFT,
		6 << GREEN_SHIFT,
		7 << GREEN_SHIFT,
		8 << GREEN_SHIFT,
		9 << GREEN_SHIFT,
		10 << GREEN_SHIFT,
		11 << GREEN_SHIFT,
		12 << GREEN_SHIFT,
		13 << GREEN_SHIFT,
		14 << GREEN_SHIFT,
		15 << GREEN_SHIFT,
		16 << GREEN_SHIFT,
		17 << GREEN_SHIFT,
		18 << GREEN_SHIFT,
		19 << GREEN_SHIFT,
		20 << GREEN_SHIFT,
		21 << GREEN_SHIFT,
		22 << GREEN_SHIFT,
		23 << GREEN_SHIFT,
		24 << GREEN_SHIFT,
		25 << GREEN_SHIFT,
		26 << GREEN_SHIFT,
		27 << GREEN_SHIFT,
		28 << GREEN_SHIFT,
		29 << GREEN_SHIFT,
		30 << GREEN_SHIFT,
		31 << GREEN_SHIFT,
		32 << GREEN_SHIFT,
		33 << GREEN_SHIFT,
		34 << GREEN_SHIFT,
		35 << GREEN_SHIFT,
		36 << GREEN_SHIFT,
		37 << GREEN_SHIFT,
		38 << GREEN_SHIFT,
		39 << GREEN_SHIFT,
		40 << GREEN_SHIFT,
		41 << GREEN_SHIFT,
		42 << GREEN_SHIFT,
		43 << GREEN_SHIFT,
		44 << GREEN_SHIFT,
		45 << GREEN_SHIFT,
		46 << GREEN_SHIFT,
		47 << GREEN_SHIFT,
		48 << GREEN_SHIFT,
		49 << GREEN_SHIFT,
		50 << GREEN_SHIFT,
		51 << GREEN_SHIFT,
		52 << GREEN_SHIFT,
		53 << GREEN_SHIFT,
		54 << GREEN_SHIFT,
		55 << GREEN_SHIFT,
		56 << GREEN_SHIFT,
		57 << GREEN_SHIFT,
		58 << GREEN_SHIFT,
		59 << GREEN_SHIFT,
		60 << GREEN_SHIFT,
		61 << GREEN_SHIFT,
		62 << GREEN_SHIFT,
		63 << GREEN_SHIFT,
		63 << GREEN_SHIFT,
		63 << GREEN_SHIFT,
		63 << GREEN_SHIFT,
		63 << GREEN_SHIFT,
		63 << GREEN_SHIFT,
		63 << GREEN_SHIFT,
		63 << GREEN_SHIFT,
		63 << GREEN_SHIFT,
		63 << GREEN_SHIFT,
		63 << GREEN_SHIFT,
		63 << GREEN_SHIFT,
		63 << GREEN_SHIFT,
		63 << GREEN_SHIFT,
		63 << GREEN_SHIFT,
		63 << GREEN_SHIFT,
		63 << GREEN_SHIFT,
		63 << GREEN_SHIFT,
		63 << GREEN_SHIFT,
		63 << GREEN_SHIFT,
		63 << GREEN_SHIFT,
		63 << GREEN_SHIFT,
		63 << GREEN_SHIFT,
		63 << GREEN_SHIFT,
		63 << GREEN_SHIFT,
		63 << GREEN_SHIFT,
		63 << GREEN_SHIFT,
		63 << GREEN_SHIFT,
		63 << GREEN_SHIFT,
		63 << GREEN_SHIFT,
		63 << GREEN_SHIFT,
		63 << GREEN_SHIFT,
		63 << GREEN_SHIFT,
		63 << GREEN_SHIFT,
		63 << GREEN_SHIFT,
		63 << GREEN_SHIFT,
		63 << GREEN_SHIFT,
		63 << GREEN_SHIFT,
		63 << GREEN_SHIFT,
		63 << GREEN_SHIFT,
		63 << GREEN_SHIFT,
		63 << GREEN_SHIFT,
		63 << GREEN_SHIFT,
		63 << GREEN_SHIFT,
		63 << GREEN_SHIFT,
		63 << GREEN_SHIFT,
		63 << GREEN_SHIFT,
		63 << GREEN_SHIFT,
		63 << GREEN_SHIFT,
		63 << GREEN_SHIFT,
		63 << GREEN_SHIFT,
		63 << GREEN_SHIFT,
		63 << GREEN_SHIFT,
		63 << GREEN_SHIFT,
		63 << GREEN_SHIFT,
		63 << GREEN_SHIFT,
		63 << GREEN_SHIFT,
		63 << GREEN_SHIFT,
		63 << GREEN_SHIFT,
		63 << GREEN_SHIFT,
		63 << GREEN_SHIFT,
		63 << GREEN_SHIFT,
		63 << GREEN_SHIFT
};

#define BLUE_SHIFT	0

const UINT16 sg_u16BlueGradientSaturation[] =
{
		0 << BLUE_SHIFT,
		1 << BLUE_SHIFT,
		2 << BLUE_SHIFT,
		3 << BLUE_SHIFT,
		4 << BLUE_SHIFT,
		5 << BLUE_SHIFT,
		6 << BLUE_SHIFT,
		7 << BLUE_SHIFT,
		8 << BLUE_SHIFT,
		9 << BLUE_SHIFT,
		10 << BLUE_SHIFT,
		11 << BLUE_SHIFT,
		12 << BLUE_SHIFT,
		13 << BLUE_SHIFT,
		14 << BLUE_SHIFT,
		15 << BLUE_SHIFT,
		16 << BLUE_SHIFT,
		17 << BLUE_SHIFT,
		18 << BLUE_SHIFT,
		19 << BLUE_SHIFT,
		20 << BLUE_SHIFT,
		21 << BLUE_SHIFT,
		22 << BLUE_SHIFT,
		23 << BLUE_SHIFT,
		24 << BLUE_SHIFT,
		25 << BLUE_SHIFT,
		26 << BLUE_SHIFT,
		27 << BLUE_SHIFT,
		28 << BLUE_SHIFT,
		29 << BLUE_SHIFT,
		30 << BLUE_SHIFT,
		31 << BLUE_SHIFT,
		31 << BLUE_SHIFT,
		31 << BLUE_SHIFT,
		31 << BLUE_SHIFT,
		31 << BLUE_SHIFT,
		31 << BLUE_SHIFT,
		31 << BLUE_SHIFT,
		31 << BLUE_SHIFT,
		31 << BLUE_SHIFT,
		31 << BLUE_SHIFT,
		31 << BLUE_SHIFT,
		31 << BLUE_SHIFT,
		31 << BLUE_SHIFT,
		31 << BLUE_SHIFT,
		31 << BLUE_SHIFT,
		31 << BLUE_SHIFT,
		31 << BLUE_SHIFT,
		31 << BLUE_SHIFT,
		31 << BLUE_SHIFT,
		31 << BLUE_SHIFT,
		31 << BLUE_SHIFT,
		31 << BLUE_SHIFT,
		31 << BLUE_SHIFT,
		31 << BLUE_SHIFT,
		31 << BLUE_SHIFT,
		31 << BLUE_SHIFT,
		31 << BLUE_SHIFT,
		31 << BLUE_SHIFT,
		31 << BLUE_SHIFT,
		31 << BLUE_SHIFT,
		31 << RED_SHIFT
};

// OS Semaphore 
static SOSSemaphore sg_sGfxSemaphore;

static EGCResultCode GfxDetermineFileType(GCFile *peFile,
										  LEX_CHAR *peFilename,
										  SGfxImageInfo *psImageInfo)
{
	EGCResultCode eResult;
	UINT32 u32ValidBytes;
	char *pu8ASCIIFilename;

	memset((void *) psImageInfo, 0, sizeof(psImageInfo));
	psImageInfo->eFileType = GFXTYPE_UNKNOWN;

	pu8ASCIIFilename = LexUnicodeToASCIIAlloc(peFilename);
	if (NULL == pu8ASCIIFilename)
	{
		return(GC_OUT_OF_MEMORY);
	}

	eResult = GCFileOpen(peFile, (const char *) pu8ASCIIFilename, "rb");
	GCFreeMemory(pu8ASCIIFilename);

	if (eResult != GC_OK)
	{
		// Image not found!
		return(eResult);
	}

	// We've got the file. Now try to load up the first "n" bytes:
	eResult = GCFileRead(sg_u8FileTypeBuffer,
						 sizeof(sg_u8FileTypeBuffer),
						 &u32ValidBytes,
						 *peFile);

	// If it doesn't read correctly, then bail!
	if (eResult != GC_OK)
	{
		goto errorExit;
	}

	// Now that we have at least SOME data, let's rewind the file back to
	// the beginning for later consumption
	eResult = GCFileSeek(*peFile,
						 0, 
						 SEEK_SET);

	// Any failures? Return a fault code
	if (eResult != GC_OK)
	{
		goto errorExit;
	}

	// We now have u32ValidBytes in the sg_u8FileTypeBuffer. Let's figure out what
	// the file is
	
	// Check for BMP (at least 29 bytes in size)
	if ((u32ValidBytes >= 29) &&
		('B' == sg_u8FileTypeBuffer[0]) &&
		('M' == sg_u8FileTypeBuffer[1]))
	{
		// It has the right signature.
		psImageInfo->eFileType = GFXTYPE_BMP;

		psImageInfo->u32XSize = (sg_u8FileTypeBuffer[18]) | (((UINT32) sg_u8FileTypeBuffer[19]) << 8) | (((UINT32) sg_u8FileTypeBuffer[20]) << 16) | (((UINT32) sg_u8FileTypeBuffer[21]) << 24);
		psImageInfo->u32YSize = (sg_u8FileTypeBuffer[22]) | (((UINT32) sg_u8FileTypeBuffer[23]) << 8) | (((UINT32) sg_u8FileTypeBuffer[24]) << 16) | (((UINT32) sg_u8FileTypeBuffer[25]) << 24);
		psImageInfo->u8BPP = sg_u8FileTypeBuffer[28];

		if ((psImageInfo->u8BPP != 8) &&
			(psImageInfo->u8BPP != 16) &&
			(psImageInfo->u8BPP != 24))
		{
			eResult = GC_CORRUPT_IMAGE;
		}

		psImageInfo->u32Pages = 1;
		goto errorExit;
	}

	// Let's try GIF.
	if (u32ValidBytes >= GIF_HEADER_SIZE)
	{
		if ((memcmp((void *) sg_u8FileTypeBuffer, "GIF89a", GIF_HEADER_SIZE) == 0) ||
			(memcmp((void *) sg_u8FileTypeBuffer, "GIF87a", GIF_HEADER_SIZE) == 0))
		{
			UINT32 u32DataAvailable = u32ValidBytes;

			// It's a GIF! Let's get the pertinent info.
			psImageInfo->eFileType = GFXTYPE_GIF;
			u32DataAvailable -= GIF_HEADER_SIZE;

			// Get the X size of the image
			if (u32ValidBytes >= sizeof(UINT16))
			{
				psImageInfo->u32XSize = sg_u8FileTypeBuffer[GIF_HEADER_SIZE] + ((UINT32) sg_u8FileTypeBuffer[GIF_HEADER_SIZE + 1] << 8);
				u32ValidBytes -= sizeof(UINT16);
			}
			else
			{
				eResult = GC_CORRUPT_IMAGE;
				goto errorExit;
			}

			// Get the Y size of the image
			if (u32ValidBytes >= sizeof(UINT16))
			{
				psImageInfo->u32YSize = sg_u8FileTypeBuffer[GIF_HEADER_SIZE + 2] + ((UINT32) sg_u8FileTypeBuffer[GIF_HEADER_SIZE + 3] << 8);
			}
			else
			{
				eResult = GC_CORRUPT_IMAGE;
				goto errorExit;
			}

			// GIF Is, by definition, 8bpp
			psImageInfo->u8BPP = 8;
			psImageInfo->u32Pages = 0;
			eResult = GC_OK;
			goto errorExit;
		}
	}

	// Try PNG.
	if (0 == png_sig_cmp((png_bytep) sg_u8FileTypeBuffer, 0, (size_t) u32ValidBytes))
	{
		png_structp png_ptr = NULL;
		png_infop info_ptr = NULL;
		INT32 s32BitDepth;
		INT32 s32ColorType;

		// It's a PNG file!
		psImageInfo->eFileType = GFXTYPE_PNG;

		png_ptr = png_create_read_struct_2(PNG_LIBPNG_VER_STRING, 
										   NULL,	// Err pointer
										   NULL,	// Err function
										   NULL,	// Warn function
										   NULL,	// mem_ptr
										   (png_malloc_ptr) PNGAlloc,	// malloc_fn
										   (png_free_ptr) PNGFree		// free_fn
										   );

		if (NULL == png_ptr)
		{
			eResult = GC_OUT_OF_MEMORY;
			goto png_cleanup;
		}

		png_set_crc_action(png_ptr, PNG_CRC_QUIET_USE, PNG_CRC_QUIET_USE);

		info_ptr = png_create_info_struct(png_ptr);
		if (NULL == info_ptr)
		{
			eResult = GC_OUT_OF_MEMORY;
			goto png_cleanup;
		}

		png_ptr->io_ptr = (png_voidp) (&sg_u8FileTypeBuffer[8]);
		png_set_sig_bytes(png_ptr, 8);
		png_read_info(png_ptr, info_ptr, TRUE);

		png_get_IHDR(png_ptr, info_ptr, &psImageInfo->u32XSize, &psImageInfo->u32YSize, (int *) &s32BitDepth, (int *) &s32ColorType, NULL, NULL, NULL);

		// Loss of precision here is OK
		psImageInfo->u8BPP = (UINT8) s32BitDepth;
		if (png_ptr->channels != 1)
		{
			// Forces 16BPP image load
			psImageInfo->u8BPP = 16;
		}

png_cleanup:
		// Time to clean up.
		png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);

		psImageInfo->u32Pages = 1;
		goto errorExit;
	}

	// How 'bout  JPG?
	if (u32ValidBytes >= 20)
	{
		struct jpeg_decompress_struct cinfo;
		struct jpeg_error_mgr jerr;
		INT32 s32Result;

		memset((void *) &cinfo, 0, sizeof(cinfo));

		cinfo.err = jpeg_std_error(&jerr);
		jpeg_create_decompress(&cinfo);

		jpeg_memory_src(&cinfo,
						sg_u8FileTypeBuffer,
						u32ValidBytes);

		if (NULL == cinfo.src)
		{
			eResult = GC_OUT_OF_MEMORY;
			goto errorExit;
		}

		// Is the JFIF stuff there?
		if ((strcmp((const char *) &sg_u8FileTypeBuffer[6], "JFIF") == 0) &&
			(0xff == sg_u8FileTypeBuffer[0]) &&
			(0xd8 == sg_u8FileTypeBuffer[1]))
		{
			s32Result = jpeg_read_header(&cinfo, TRUE);
			if (JPEG_HEADER_OK == s32Result)
			{
				// It's a JPEG image!
				psImageInfo->eFileType = GFXTYPE_JPG;

				// Do a start decompress
				jpeg_start_decompress(&cinfo);

				psImageInfo->u32XSize = cinfo.output_width;
				psImageInfo->u32YSize = cinfo.output_height;
				psImageInfo->u32Pages = 1;
				psImageInfo->u8BPP = 16;	// Everything is 16BPP for JPEG
			}
			else
			{
				// Not a JPEG image
			}
		}
		else
		{
			// Not a JPEG file
		}

		// Clean up.
		jpeg_destroy_decompress(&cinfo);
		if (cinfo.src)
		{
			GCFreeMemory(cinfo.src);
			cinfo.src = NULL;
		}

		if (psImageInfo->eFileType != GFXTYPE_UNKNOWN)
		{
			goto errorExit;
		}
	}

	// Let's try TGA as a last resort since it's the least identifiable
	if (u32ValidBytes >= sizeof(TGAInfo))
	{
		TGAInfo sTGAInfo;

		memcpy((void *) &sTGAInfo, sg_u8FileTypeBuffer, sizeof(sTGAInfo));

		// Looks like the ID is the right size. Let's validate the fields and
		// assume it's a TGA file.

		// Validate color map type
		if ((0 == sTGAInfo.colormap_type) ||
			(1 == sTGAInfo.colormap_type) ||
			(sTGAInfo.colormap_type >= 128))
		{
			// The color map type passes the sniff test. Now check the image type.
			if ((TGANoColormap == sTGAInfo.image_type) ||
				(TGAColormap == sTGAInfo.image_type) ||
				(TGARGB == sTGAInfo.image_type) ||
				(TGAMonochrome == sTGAInfo.image_type) ||
				(TGARLEColormap == sTGAInfo.image_type) ||
				(TGARLERGB == sTGAInfo.image_type) ||
				(TGARLEMonochrome == sTGAInfo.image_type))
			{
				// Color map info looks good! Let's see if the pixel depth is OK.

				if ((32 == sTGAInfo.bits_per_pixel) ||
					(24 == sTGAInfo.bits_per_pixel) ||
					(16 == sTGAInfo.bits_per_pixel) ||
					(8 == sTGAInfo.bits_per_pixel))
				{
					// It's a TGA file!
					psImageInfo->eFileType = GFXTYPE_TGA;
					psImageInfo->u32XSize = sTGAInfo.width;
					psImageInfo->u32YSize = sTGAInfo.height;
					psImageInfo->u8BPP = 16;	// TGA Always 16BPP
					psImageInfo->u32Pages = 1;
					goto errorExit;
				}
			}
		}

		// Not a TGA file as far as we can tell.
	}

errorExit:
	// Check the type. If we don't know what it is, go to error exit
	if ((eResult != GC_OK) ||
		(GFXTYPE_UNKNOWN == psImageInfo->eFileType))
	{
		GCFileClose(peFile);
	}

	return(eResult);
}

static void GfxLockFileList(void)
{
	EGCResultCode eResult;

	if (GCOSIsRunning())
	{
		eResult = GCOSSemaphoreGet(sg_sGfxSemaphore,
								   0);
		GCASSERT(GC_OK == eResult);
	}
}

static void GfxUnlockFileList(void)
{
	EGCResultCode eResult;

	if (GCOSIsRunning())
	{
		eResult = GCOSSemaphorePut(sg_sGfxSemaphore);
		GCASSERT(GC_OK == eResult);
	}
}

void GfxInit(void)
{
	EGCResultCode eResult;

	eResult = GCOSSemaphoreCreate(&sg_sGfxSemaphore,
								  1);
	GCASSERT(GC_OK == eResult);
}

#ifdef _WIN32
static void GfxReportImageTree(void)
{
	FILE *fp;
	SLayer *psLayer;
	SImageInstance *psInstance;
	SZBufferLink *psZLink = NULL;

	return;

	fp = fopen("c:/imagetree.txt", "w");
	if (NULL == fp)
	{
		DebugOut("Can't open c:/imagetree.txt for writing\n");
		return;
	}

	psLayer = sg_psLayerHead;
	fprintf(fp, "Layer 0x%.8x:\n", psLayer);

	while (psLayer)
	{
		psInstance = psLayer->psImages;
		while (psInstance)
		{
			fprintf(fp, "  Instance 0x%.8x: (x/ypos=%d,%d  x/yoff=%d,%d x/ysize=%d,%d)\n", psInstance,
						psInstance->u32XPos,
						psInstance->u32YPos,
						psInstance->u32XOffset,
						psInstance->u32YOffset,
						psInstance->u32XSizeClipped,
						psInstance->u32YSizeClipped);
			fprintf(fp, "     Behind:\n");

			psZLink = psInstance->psZBufferBehind;
			while (psZLink)
			{
				fprintf(fp, "       0x%.8x\n", psZLink->psImageInstance);
				psZLink = psZLink->psNextLink;
			}

			fprintf(fp, "     Front :\n");
			psZLink = psInstance->psZBufferFront;
			while (psZLink)
			{
				fprintf(fp, "       0x%.8x\n", psZLink->psImageInstance);
				psZLink = psZLink->psNextLink;
			}

			psInstance = psInstance->psNextLink;
		}

		psLayer = psLayer->psNextLayer;
		if (psLayer)
		{
			fprintf(fp, "Layer 0x%.8x:\n", psLayer);
		}
	}

	fclose(fp);
}
#endif

SImage *GfxAllocateImage(void)
{
	SImage *psImage;

	// Let's allocate an image pointer

	psImage = MemAlloc(sizeof(*psImage));
	if (psImage)
	{
		// Start out indicating the image is not rotated
		psImage->eCurrentRotation = ROT_0;
	}

	return(psImage);
}


static BOOL GfxCoordinatesIntersect(SImageInstance *psImage1,
									UINT32 u32XPos,
									UINT32 u32YPos,
									UINT32 u32XSize,
									UINT32 u32YSize)
{
	UINT32 u32Image1XRight;
	UINT32 u32Image1YBottom;
	UINT32 u32Image2XRight;
	UINT32 u32Image2YBottom;

	GCASSERT(psImage1);

	if (FALSE == psImage1->bImageVisible)
	{
		// No intersection because one isn't visible
		return(FALSE);
	}

	// They are both visible. Let's figure out right/bottom

	// Check right hand side of each rectangle first
	u32Image1XRight = psImage1->u32XPos + psImage1->u32XSizeClipped;
	u32Image2XRight = u32XPos + u32XSize;

	if ((u32Image1XRight < u32XPos) ||
		(u32Image2XRight < psImage1->u32XPos))
	{
		// No intersection on the right on either rectangle
		return(FALSE);
	}

	// We at least have an X intersection. Let's check out the Y intersection

	u32Image1YBottom = psImage1->u32YPos + psImage1->u32YSizeClipped;
	u32Image2YBottom = u32YPos + u32YSize;

	if ((u32Image1YBottom < u32YPos) ||
		(u32Image2YBottom < psImage1->u32YPos))
	{
		// No intersection on the right on either rectangle
		return(FALSE);
	}

	// We have intersection
	return(TRUE);
}

static BOOL GfxCoordinatesObscure(SImageInstance *psImage1,
								  UINT32 u32XPos,
								  UINT32 u32YPos,
								  UINT32 u32XSize,
								  UINT32 u32YSize)
{
	if (FALSE == GfxCoordinatesIntersect(psImage1,
										 u32XPos,
										 u32YPos,
										 u32XSize,
										 u32YSize))
	{
		// They don't intersect at all, so no, they don't obscure
		return(FALSE);
	}

	// They at least intersect. Let's see if psImage1's coordinates are
	// wholly contained within xypos/xysize

	if ((u32XPos < (psImage1->u32XPos)) ||
		((u32XPos + u32XSize) > (psImage1->u32XPos + psImage1->u32XSizeClipped)))
	{
		// XPosition or XPosition+Size is outside of the horizontal axis
		return(FALSE);
	}

	if ((u32YPos < (psImage1->u32YPos)) ||
		((u32YPos + u32YSize) > (psImage1->u32YPos + psImage1->u32YSizeClipped)))
	{
		// YPosition or YPosition+Size is outside of the vertical axis
		return(FALSE);
	}

	// Yup. xysize/xypos is completely obscured
	return(TRUE);
}

static BOOL GfxImagesIntersect(SImageInstance *psImage1,
							   SImageInstance *psImage2)
{
	if (psImage1 == psImage2)
	{
		// Images don't intersect with themselves
		return(FALSE);
	}

	GCASSERT(psImage1);
	GCASSERT(psImage2);

	if (FALSE == psImage2->bImageVisible)
	{
		// No intersection because one isn't visible
		return(FALSE);
	}

	return(GfxCoordinatesIntersect(psImage1,
								   psImage2->u32XPos,
								   psImage2->u32YPos,
								   psImage2->u32XSizeClipped,
								   psImage2->u32YSizeClipped));
}

static void GfxRecalcIntersectionInstance(SImageInstance *psTarget)
{
	SZBufferLink *psZLink;
	SZBufferLink *psPriorZLink = NULL;
	SLayer *psLayer;
	SImageInstance *psImagePtr;

	// First things first, let's deallocate any SZBufferLinks 

	while (psTarget->psZBufferBehind)
	{
		psZLink = psTarget->psZBufferBehind;
		psTarget->psZBufferBehind = psTarget->psZBufferBehind->psNextLink;
		GCFreeMemory(psZLink);
	}

	// Ensure this is NULL now
	GCASSERT(NULL == psTarget->psZBufferBehind);

	while (psTarget->psZBufferFront)
	{
		psZLink = psTarget->psZBufferFront;
		psTarget->psZBufferFront = psTarget->psZBufferFront->psNextLink;
		GCFreeMemory(psZLink);
	}

	// Ensure this is NULL now
	GCASSERT(NULL == psTarget->psZBufferFront);

	// If this image isn't visible, just return

	if (FALSE == psTarget->bImageVisible)
	{
		return;
	}

	// Otherwise, we need to start scanning at this layer and work
	// backwards to compute the psZBufferBehind tree.

	psLayer = psTarget->psParentLayer;
	psZLink = NULL;
	psPriorZLink = NULL;

	while (psLayer)
	{
		psImagePtr = psLayer->psImages;
		while (psImagePtr)
		{
			if (GfxImagesIntersect(psImagePtr,
								   psTarget))
			{
				// They intersect - add a node in the reverse order

				if (psZLink)
				{
					psZLink->psNextLink = MemAlloc(sizeof(*psZLink->psNextLink));
					psPriorZLink = psZLink;
					psZLink = psZLink->psNextLink;
					psZLink->psPriorLink = psPriorZLink;
				}
				else
				{
					// First time, connect it up
					psTarget->psZBufferBehind = MemAlloc(sizeof(*psZLink->psNextLink));
					psZLink = psTarget->psZBufferBehind;
				}

				GCASSERT(psZLink);
				psZLink->psImageInstance = psImagePtr;
			}

			psImagePtr = psImagePtr->psNextLink;
		}

		psLayer = psLayer->psPriorLayer;
	}

	// Now, let's do the front links

	psLayer = psTarget->psParentLayer->psNextLayer;
	psZLink = NULL;
	psPriorZLink = NULL;

	while (psLayer)
	{
		psImagePtr = psLayer->psImages;
		while (psImagePtr)
		{
			if (GfxImagesIntersect(psImagePtr,
								   psTarget))
			{
				// They intersect - add a node in the reverse order

				if (psZLink)
				{
					psZLink->psNextLink = MemAlloc(sizeof(*psZLink->psNextLink));
					psPriorZLink = psZLink;
					psZLink = psZLink->psNextLink;
					psZLink->psPriorLink = psPriorZLink;
				}
				else
				{
					// First time, connect it up
					psTarget->psZBufferFront = MemAlloc(sizeof(*psZLink->psNextLink));
					psZLink = psTarget->psZBufferFront;
				}

				GCASSERT(psZLink);
				psZLink->psImageInstance = psImagePtr;
			}

			psImagePtr = psImagePtr->psNextLink;
		}

		psLayer = psLayer->psNextLayer;
	}

}

static void GfxRecalcIntersections(void)
{
	SLayer *psLayer;
	SImageInstance *psInstance;

	psLayer = sg_psLayerHead;

	while (psLayer)
	{
		psInstance = psLayer->psImages;
		while (psInstance)
		{
			GfxRecalcIntersectionInstance(psInstance);
			psInstance = psInstance->psNextLink;
		}

		psLayer = psLayer->psNextLayer;
	}

#ifdef _WIN32
	GfxReportImageTree();
#endif
}

BOOL SingleImageFixup(SImage *psImage,
					  SImageGroup *psImageGroup)
{
	// If we have an image group, let's create appropriate structures for it

	if (psImageGroup)
	{
		psImageGroup->psLinkHead = MemAlloc(sizeof(*psImageGroup->psLinkHead));
		if (NULL == psImageGroup->psLinkHead)
		{
			// Out of memory. Waaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa....
			if (psImage->pu16ImageData)
			{
				GCFreeMemory(psImage->pu16ImageData);
				psImage->pu16ImageData = NULL;
			}

			if (psImage->pu8ImageData)
			{
				GCFreeMemory(psImage->pu8ImageData);
				psImage->pu8ImageData = NULL;
			}

			if (psImage->pu16Palette)
			{
				GCFreeMemory(psImage->pu16Palette);
				psImage->pu16Palette = NULL;
			}

			return(FALSE);
		}

		psImageGroup->psLinkHead->psImage = psImage;
		psImageGroup->psCurrentImage = psImageGroup->psLinkHead->psImage;
		psImageGroup->u32FrameCount = 1;
	}

	return(TRUE);
}

static EGCResultCode GfxLoadBMP(UINT8 *pu8ImagePtr,
							    UINT32 u32ImageSize,
							    SImageGroup *psImageGroup,
							    SImage *psImage)
{
	UINT32 u32ImageOffset;
	UINT16 *pu16Src;
	UINT16 *pu16Dest;
	UINT32 u32Loop;
	UINT8 u8BPP;
	EGCResultCode eResult = GC_OK;
	BOOL bResult;

	u8BPP = *(pu8ImagePtr + 28);

	u32ImageOffset = (*(pu8ImagePtr + 10)) | (*(pu8ImagePtr + 11) << 8) | (*(pu8ImagePtr + 12) << 16) | (*(pu8ImagePtr + 13) << 24);

	if (16 == u8BPP)
	{
		// Now let's copy the image into its final resting place. It's upside down - let's make it right side up

		pu16Dest = psImage->pu16ImageData;
		pu16Src = ((UINT16 *) (pu8ImagePtr + u32ImageOffset)) + ((psImage->u32YSize - 1) * ((psImage->u32XSize + 1) & 0xfffffffe));

		u32Loop = psImage->u32YSize;

		while (u32Loop)
		{
			memcpy((void *) pu16Dest, (void *) pu16Src, (size_t) ((psImage->u32XSize) * sizeof(*psImage->pu16ImageData)));
			pu16Src -= ((psImage->u32XSize + 1) & 0xfffffffe);
			pu16Dest += (psImage->u32Pitch);
			--u32Loop;
		}

		// Now do a 555->565 conversion in place

		pu16Src = psImage->pu16ImageData;
		u32Loop = psImage->u32XSize * psImage->u32YSize;

		if (3 == *(pu8ImagePtr + 0x1e))
		{
			UINT32 u32RedMask;
			UINT32 u32GreenMask;
			UINT32 u32BlueMask;

			// This indicates a standard rrrrrggggggbbbbb type of picture. Let's check the mask.
			// If it's 0xf800, 0x07e0, and 0x1f, then leave the image alone.

			u32RedMask = (*(pu8ImagePtr + 0x36)) | (*(pu8ImagePtr + 0x37) << 8) | (*(pu8ImagePtr + 0x38) << 16) | (*(pu8ImagePtr + 0x39) << 24);
			u32GreenMask = (*(pu8ImagePtr + 0x3a)) | (*(pu8ImagePtr + 0x3b) << 8) | (*(pu8ImagePtr + 0x3c) << 16) | (*(pu8ImagePtr + 0x3d) << 24);
			u32BlueMask = (*(pu8ImagePtr + 0x3e)) | (*(pu8ImagePtr + 0x3f) << 8) | (*(pu8ImagePtr + 0x40) << 16) | (*(pu8ImagePtr + 0x41) << 24);

			if ((0xf800 == u32RedMask) && (0x07e0 == u32GreenMask) && (0x1f == u32BlueMask))
			{
				// We know this mask - leave it!
			}
			else
			{
				// Unknown mask
				eResult = GC_CORRUPT_IMAGE;
				goto errorExit;
			}
		}
		else
		if (0 == *(pu8ImagePtr + 0x1e))
		{
			// Convert 555 into 565

			while (u32Loop--)
			{
				*pu16Src = (*pu16Src & 0x1f) | ((*pu16Src & 0x3e0) << 1) | ((*pu16Src & 0x7c00) << 1);
				++pu16Src;
			}
		}
		else
		{
			eResult = GC_CORRUPT_IMAGE;
			goto errorExit;
		}
	}
	else
	if (24 == u8BPP)
	{
		UINT32 u32XCount = 0;

		// Handle 24bpp images

		u32Loop = psImage->u32XSize * psImage->u32YSize;
		pu16Dest = psImage->pu16ImageData + psImage->u32XSize - 1;
		pu8ImagePtr += u32ImageOffset + (u32Loop * 3) - 1;

		while (u32Loop--)
		{
			UINT8 u8Red;
			UINT8 u8Green;
			UINT8 u8Blue;

			u8Red = *pu8ImagePtr >> 3;
			pu8ImagePtr--;
			u8Green = *pu8ImagePtr >> 2;
			pu8ImagePtr--;
			u8Blue = *pu8ImagePtr >> 3;
			pu8ImagePtr--;

			*pu16Dest = ((UINT16) u8Red << 11) | ((UINT16) u8Green << 5) | u8Blue;
			pu16Dest--;

			u32XCount++;
			if (u32XCount >= psImage->u32XSize)
			{
				u32XCount = 0;
				pu16Dest += (psImage->u32XSize << 1);
			}
		}

	}
	else
	if (8 == u8BPP)
	{
		UINT8 *pu8Target;
		UINT8 *pu8Palette;
		UINT8 u8Extra = psImage->u32XSize & 3; 
		UINT32 u32Y = psImage->u32YSize - 1;
		UINT32 u32X = 0;
		UINT32 u32Index = 0xffffffff;
		UINT8 u8CompressionType;

		if (u8Extra)
		{
			u8Extra = 4 - u8Extra;
		}

		psImage->pu16Palette = MemAlloc(sizeof(*psImage->pu16Palette) * (1 << 8));
		if (NULL == psImage->pu16Palette)
		{
			eResult = GC_OUT_OF_MEMORY;
			goto errorExit;
		}

		// Point to the palette
		pu8Palette = pu8ImagePtr + 0x36;

		for (u32Loop = 0; u32Loop < (1 << 8); u32Loop++)
		{
			UINT8 u8Red;
			UINT8 u8Green;
			UINT8 u8Blue;

			u8Blue = *pu8Palette;
			++pu8Palette;
			u8Green = *pu8Palette;
			++pu8Palette;
			u8Red = *pu8Palette;
			++pu8Palette;
			++pu8Palette;

			if ((0 == u8Blue) &&
				(0 == u8Green) &&
				(0 == u8Red))
			{
				if (0xffffffff == u32Index)
				{
					u32Index = u32Loop;
				}
			}

			u8Red >>= 3;
			u8Green >>= 2;
			u8Blue >>= 3;
			psImage->pu16Palette[u32Loop] = (UINT16) ((u8Red << 11) | (u8Green << 5) | u8Blue);
		}

		if (0xffffffff == u32Index)
		{
			eResult = GC_CORRUPT_IMAGE;
			goto errorExit;
		}

		psImage->u16TransparentIndex = (UINT16) u32Index;
		u8CompressionType = *(pu8ImagePtr + 0x1e);

		pu8ImagePtr += u32ImageOffset;
		pu8Target = psImage->pu8ImageData + (u32Y * psImage->u32XSize);

		if (1 == u8CompressionType)
		{
			// RLE 8 bit per pixel
			while (1)
			{
				UINT8 u8Data;

				u8Data = *pu8ImagePtr;
				pu8ImagePtr++;

				if (u8Data)
				{
					UINT8 u8Color;

					// Nonzero indicates a repeat pixel count

					u8Color = *pu8ImagePtr;
					pu8ImagePtr++;

					while (u8Data--)
					{
						*pu8Target = u8Color;
						++pu8Target;
						u32X++;
						if (u32X >= psImage->u32XSize)
						{
							if (0 == u32Y)
							{
								// All done!
								break;
							}

							--u32Y;
							pu8Target -= u32X;
							pu8Target -= psImage->u32XSize;
							u32X = 0;
						}
					}
				}
				else
				{
					// Command byte

					u8Data = *pu8ImagePtr;
					pu8ImagePtr++;

					if (0 == u8Data)
					{
						// End of line

						if (u32X > u8Extra)
						{
							if (0 == u32Y)
							{
								break;
							}

							u32Y--;
							pu8Target -= psImage->u32XSize;
						}

						pu8Target -= u32X;
						u32X = 0;
					}
					else
					if (1 == u8Data)
					{
						// End of bitmap
						break;
					}
					else
					if (2 == u8Data)
					{
						UINT8 u8Dir;

						// Move
						pu8Target += *pu8ImagePtr;
						u32X += *pu8ImagePtr;
						++pu8ImagePtr;
						u8Dir = *pu8ImagePtr;
						++pu8ImagePtr;
						if (u8Dir > u32Y)
						{
							// Done
							break;
						}

						pu8Target -= (u8Dir * psImage->u32XSize);
					}
					else
					{
						UINT8 u8Length;

						u8Length = u8Data;

						// Uncompressed data

						if ((u32X + u8Length) <= psImage->u32XSize)
						{
							memcpy(pu8Target, pu8ImagePtr, u8Length);
							pu8Target += u8Length;
							pu8ImagePtr += u8Length;
							u32X += u8Length;
						}
						else
						{
							UINT32 u32Delta;

							u32Delta = (psImage->u32XSize - u32X);

							while (u8Data > u32Delta)
							{
								memcpy(pu8Target, pu8ImagePtr, u32Delta);
								u8Data -= u32Delta;
								pu8Target += u8Data;
								pu8ImagePtr += u32Delta;
								u32X = 0;
								if (0 == u32Y)
								{
									break;
								}
								u32Y--;
								pu8Target -= psImage->u32XSize;
							}

							if (u8Data)
							{
								memcpy(pu8Target, pu8ImagePtr, u8Data);
								pu8Target += u8Data;
								pu8ImagePtr += u8Data;
								u32X += u8Data;
							}
						}

						if (u8Length & 1)
						{
							// Put source on an even boundary

							pu8ImagePtr++;
						}
					}
				}
			}
		}
		else
		if (0 == u8CompressionType)
		{
			UINT32 u32Count = psImage->u32XSize * psImage->u32YSize;

			while (u32Count--)
			{
				*pu8Target = *pu8ImagePtr;
				++pu8Target;
				++pu8ImagePtr;
				u32X++;
				if (u32X >= psImage->u32XSize)
				{
					if (0 == u32Y)
					{
						// All done!
						break;
					}

					--u32Y;
					pu8Target -= u32X;
					pu8Target -= psImage->u32XSize;
					pu8ImagePtr += u8Extra;
					u32X = 0;
				}
			}
		}
		else
		{
			GCASSERT(0);
		}
	}
	else
	{
		eResult = GC_CORRUPT_IMAGE;
		goto errorExit;
	}

	// Convert to 4 byte aligned pitch if it's an 8bpp image

	if (8 == u8BPP)
	{
		if (psImage->u32XSize != psImage->u32Pitch)
		{
			UINT8 *pu8DestPtr;
			UINT8 *pu8SrcPtr;
			UINT32 u32Loop;
			
			pu8DestPtr = (psImage->u32Pitch * psImage->u32YSize) + psImage->pu8ImageData;
			pu8SrcPtr = (psImage->u32XSize * psImage->u32YSize) + psImage->pu8ImageData;
			for (u32Loop = 0; u32Loop < psImage->u32YSize; u32Loop++)
			{
				pu8DestPtr -= psImage->u32Pitch;
				pu8SrcPtr -= psImage->u32XSize;
				memmove((void *) pu8DestPtr, pu8SrcPtr, psImage->u32XSize);
			}
		}
	}

	bResult = SingleImageFixup(psImage,
							   psImageGroup);

	if (FALSE == bResult)
	{
		eResult = GC_OUT_OF_MEMORY;
		goto errorExit;
	}

	eResult = GC_OK;

errorExit:
	return(eResult);
}

void png_read_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
//	DebugOut(" png_read_data=%d bytes\n", length);

	if (sizeof(UINT32) == length)
	{
		*((UINT8 *) data) = *((UINT8 *) png_ptr->io_ptr);
		*(((UINT8 *) data) + 1) = *(((UINT8 *) png_ptr->io_ptr) + 1);
		*(((UINT8 *) data) + 2) = *(((UINT8 *) png_ptr->io_ptr) + 2);
		*(((UINT8 *) data) + 3) = *(((UINT8 *) png_ptr->io_ptr) + 3);

		png_ptr->io_ptr = (png_voidp) (((UINT8 *) (png_ptr->io_ptr)) + sizeof(UINT32));
		return;
	}

	memcpy((void *) data, png_ptr->io_ptr, length);
	png_ptr->io_ptr = (png_voidp) (((UINT8 *) (png_ptr->io_ptr)) + length);
}

static EGCResultCode GfxLoadPNG(UINT8 *pu8ImagePtr,
							    UINT32 u32ImageSize,
							    SImageGroup *psImageGroup,
							    SImage *psImage)
{
	png_structp png_ptr = NULL;
	png_infop info_ptr = NULL;
	UINT32 u32RowSize = 0;
	UINT16 *pu16FinalImage = NULL;
	UINT8 *pu8FinalImage = NULL;
	UINT8 *pu8TranslucentMask = NULL;
	UINT32 u32Loop = 0;
	EGCResultCode eResult = GC_OK;
	BOOL bResult = FALSE;

	png_ptr = png_create_read_struct_2(PNG_LIBPNG_VER_STRING, 
									   NULL,	// Err pointer
									   NULL,	// Err function
									   NULL,	// Warn function
									   NULL,	// mem_ptr
									   (png_malloc_ptr) PNGAlloc,	// malloc_fn
									   (png_free_ptr) PNGFree		// free_fn
									   );

	if (NULL == png_ptr)
	{
		eResult = GC_OUT_OF_MEMORY;
		goto errorExit;
	}

	png_set_crc_action(png_ptr, PNG_CRC_QUIET_USE, PNG_CRC_QUIET_USE);

	info_ptr = png_create_info_struct(png_ptr);
	if (NULL == info_ptr)
	{
		eResult = GC_OUT_OF_MEMORY;
		goto errorExit;
	}

	// Skip past the PNG header
	png_ptr->io_ptr = (png_voidp) (pu8ImagePtr + 8);

	// Skip past the signature
	png_set_sig_bytes(png_ptr, 8);

	// Read PNG info
	png_read_info(png_ptr, info_ptr, FALSE);

	// Update the info in the png structure
	png_read_update_info(png_ptr, info_ptr);

	// Figure out how many bytes per row
	u32RowSize = png_get_rowbytes(png_ptr, info_ptr);

	// Set up a translucency mask, but only if has an alpha channel

	if (4 == png_ptr->channels)
	{
		psImage->pu8TranslucentMask = MemAllocNoClear(psImage->u32YSize * psImage->u32Pitch);
		if (NULL == psImage->pu8TranslucentMask)
		{
			eResult = GC_OUT_OF_MEMORY;
			goto errorExit;
		}
	}

	if ((png_ptr->color_type & PNG_COLOR_TYPE_PALETTE) &&
		(info_ptr->palette))
	{
		png_color *psPalettePointer;

		// Gotta create a palette!
		psPalettePointer = info_ptr->palette;

		psImage->pu16Palette = MemAllocNoClear(info_ptr->num_palette * sizeof(*psImage->pu16Palette));
		if (NULL == psImage->pu16Palette)
		{
			eResult = GC_OUT_OF_MEMORY;
			goto errorExit;
		}

		for (u32Loop = 0; u32Loop < info_ptr->num_palette; u32Loop++)
		{
			psImage->pu16Palette[u32Loop] = ((info_ptr->palette[u32Loop].red >> 3) << 11) |
											((info_ptr->palette[u32Loop].green >> 2) << 5) |
											(info_ptr->palette[u32Loop].blue >> 3);
		}

		if (0 == png_ptr->num_trans)
		{
			psImage->u16TransparentIndex = 0xffff;
		}
		else
		{
			if (png_ptr->num_trans >= png_ptr->num_palette)
			{
				psImage->u16TransparentIndex = 0xffff;
			}
			else
			{
				psImage->u16TransparentIndex = png_ptr->num_trans;
			}
		}
	}

	// Now load up the image with conversion in the process
	png_read_image_convert(png_ptr, 
						   info_ptr,
						   psImage);

	// End of our read
	png_read_end(png_ptr, info_ptr);

	/* clean up after the read, and free any memory allocated - REQUIRED */
	png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);

	bResult = SingleImageFixup(psImage,
							   psImageGroup);

	if (FALSE == bResult)
	{
		eResult = GC_OUT_OF_MEMORY;
	}

	return(eResult);

errorExit:
	/* clean up after the read, and free any memory allocated - REQUIRED */
	png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);

	return(eResult);
}

static BOOL TGALoad(UINT8 *pu8ImagePtr,
					UINT32 u32ImageSize,
					SImage *psImage,
					SImageGroup *psImageGroup,
					EGCResultCode *peResult)
{
	TGAInfo sTGAInfo;
	BOOL bResult = FALSE;
	UINT8 *pu8Translucent = NULL;
	UINT16 *pu16Colors = NULL;
	UINT16 *pu16ImageData = NULL;
	UINT32 u32X;
	UINT32 u32Y;
	UINT32 u32Offset = 0;
	UINT32 u32Real = 0;
	BOOL bSkip = FALSE;
	BOOL bFlag = FALSE;
	UINT16 u16Color;
	UINT32 u32RunLength = 0;
	UINT8 u8Translucent;
	INT32 s32Step = 0;
	INT32 s32LineAdjust = 0;
	EGCResultCode eResult = GC_OK;

	if (u32ImageSize < sizeof(sTGAInfo))
	{
		return(FALSE);
	}

	pu16ImageData = psImage->pu16ImageData;
	GCASSERT(pu16ImageData);
	memcpy((void *) &sTGAInfo, pu8ImagePtr, sizeof(sTGAInfo));
	pu8ImagePtr += sizeof(sTGAInfo);
	u32ImageSize -= sizeof(sTGAInfo);

	if ((0 == sTGAInfo.image_type) || (sTGAInfo.image_type > TGARLEMonochrome))
	{
		// Not a TGA image
		return(FALSE);
	}

	// Do we have an alpha channel?
	if (32 == sTGAInfo.bits_per_pixel)
	{
		psImage->pu8TranslucentMask = MemAllocNoClear(psImage->u32YSize * psImage->u32Pitch * (sizeof(*psImage->pu8TranslucentMask)));
		if (NULL == psImage->pu8TranslucentMask)
		{
			bResult = FALSE;
			eResult = GC_OUT_OF_MEMORY;
			goto cleanup;
		}

		pu8Translucent = psImage->pu8TranslucentMask;
	}

	// If we have a TGA ID tag, skip it

	bResult = TRUE;

	// Eat the image tag if applicable

	if (u32ImageSize < sTGAInfo.id_length)
	{
		// Incomplete file
		eResult = GC_CORRUPT_IMAGE;
		goto cleanup;
	}

	u32ImageSize -= sTGAInfo.id_length;
	pu8ImagePtr += sTGAInfo.id_length;

	// If we have a color map, let's create a palette for it

	if (sTGAInfo.colormap_type)
	{
		UINT32 u32Loop;

		pu16Colors = MemAlloc(sTGAInfo.colormap_length * sizeof(*pu16Colors));
		if (NULL == pu16Colors)
		{
			eResult = GC_OUT_OF_MEMORY;
			goto cleanup;
		}

		for (u32Loop = 0; u32Loop < sTGAInfo.colormap_length; u32Loop++)
		{
			switch (sTGAInfo.colormap_size)
			{
				case 8:
				{
					GCASSERT_WHY(0, "8bpp color maps not supported");
					break;	
				}
				case 15:
				{
					GCASSERT_WHY(0, "15bpp color maps not supported");
					break;	
				}
				case 16:
				{
					GCASSERT_WHY(0, "16bpp color maps not supported");
					break;	
				}
				case 32:
				{
					GCASSERT_WHY(0, "32bpp color maps not supported");
					break;	
				}
				case 24:
				{
					UINT8 u8Red;
					UINT8 u8Green;
					UINT8 u8Blue;

					u8Blue = *pu8ImagePtr >> 3;
					++pu8ImagePtr;
					--u32ImageSize;
					if (0 == u32ImageSize)
					{
						eResult = GC_CORRUPT_IMAGE;
						goto cleanup;
					}

					u8Green = *pu8ImagePtr >> 2;
					++pu8ImagePtr;
					--u32ImageSize;
					if (0 == u32ImageSize)
					{
						eResult = GC_CORRUPT_IMAGE;
						goto cleanup;
					}

					u8Red = *pu8ImagePtr >> 3;
					++pu8ImagePtr;
					--u32ImageSize;
					if (0 == u32ImageSize)
					{
						eResult = GC_CORRUPT_IMAGE;
						goto cleanup;
					}

					u16Color = (u8Red << 11) | (u8Green << 5) | u8Blue;
					pu16Colors[u32Loop] = u16Color;
					break;
				}
				default:
				{
					eResult = GC_CORRUPT_IMAGE;
					goto cleanup;
					break;
				}
			}
		}
	}

	// Let's figure out our orientation

	if (sTGAInfo.attributes & (1 << 5))
	{
		// Upper left hand corner origin

		s32Step = 1;
		s32LineAdjust = (psImage->u32Pitch - psImage->u32XSize);
	}
	else
	{
		// Lower left hand corner origin
		s32LineAdjust = -((INT32) psImage->u32XSize + (INT32) psImage->u32Pitch);
		pu16ImageData += ((psImage->u32YSize - 1) * psImage->u32Pitch);
		s32Step = 1;
	}

	// We've read in our color map. Now let's start by decoding the image.

	bSkip = FALSE;
	bFlag = FALSE;
	for (u32Y = 0; u32Y < psImage->u32YSize; u32Y++)
	{
		u32Real = u32Offset;

		for (u32X = 0; u32X < psImage->u32XSize; u32X++)
		{
			// It's a run length image

			if (sTGAInfo.image_type & 0x8)
			{
				// Run length
				if (u32RunLength)
				{
					u32RunLength--;
					if (bFlag)
					{
						bSkip = TRUE;
					}
				}
				else
				{
					// Nothing left in the run length
					if (0 == u32ImageSize)
					{
						eResult = GC_CORRUPT_IMAGE;
						goto cleanup;
					}

					bFlag = FALSE;
					u32RunLength = *pu8ImagePtr;
					if (u32RunLength & 0x80)
					{
						// Run length
						u32RunLength &= 0x7f;
						bFlag = TRUE;
					}

					bSkip = FALSE;
					++pu8ImagePtr;
					u32ImageSize--;
				}
			}

			if (FALSE == bSkip)
			{
				if (8 == sTGAInfo.bits_per_pixel)
				{
					// Palettized
					u16Color = pu16Colors[*pu8ImagePtr];
					++pu8ImagePtr;
					--u32ImageSize;
				}
				else
				if ((15 == sTGAInfo.bits_per_pixel) ||
					(16 == sTGAInfo.bits_per_pixel))
				{
					UINT8 u8Red;
					UINT8 u8Green = 0;
					UINT8 u8Blue;

					if (u32ImageSize < sizeof(u16Color))
					{
						eResult = GC_CORRUPT_IMAGE;
						goto cleanup;
					}

					u8Red = (*(pu8ImagePtr + 1) & 0x7c) >> 2;
					u8Green = (*(pu8ImagePtr) >> 5) | 
							  ((*(pu8ImagePtr + 1) & 0x3) << 3);
					u8Blue = *(pu8ImagePtr) & 0x1f;

					u16Color = (u8Red << 11) | (u8Green << 6) | u8Blue;

					pu8ImagePtr += sizeof(u16Color);
					u32ImageSize -= sizeof(u16Color);
				}
				else
				if ((24 == sTGAInfo.bits_per_pixel) ||
					(32 == sTGAInfo.bits_per_pixel))
				{
					UINT8 u8Red;
					UINT8 u8Green;
					UINT8 u8Blue;

					u8Blue = *pu8ImagePtr >> 3;
					++pu8ImagePtr;
					--u32ImageSize;
					if (0 == u32ImageSize)
					{
						eResult = GC_CORRUPT_IMAGE;
						goto cleanup;
					}

					u8Green = *pu8ImagePtr >> 2;
					++pu8ImagePtr;
					--u32ImageSize;
					if (0 == u32ImageSize)
					{
						eResult = GC_CORRUPT_IMAGE;
						goto cleanup;
					}

					u8Red = *pu8ImagePtr >> 3;
					++pu8ImagePtr;
					--u32ImageSize;
					if (0 == u32ImageSize)
					{
						eResult = GC_CORRUPT_IMAGE;
						goto cleanup;
					}

					u16Color = (u8Red << 11) | (u8Green << 5) | u8Blue;

					if (32 == sTGAInfo.bits_per_pixel)
					{
						u8Translucent = *pu8ImagePtr;
						++pu8ImagePtr;
						--u32ImageSize;
						if (0 == u32ImageSize)
						{
							eResult = GC_CORRUPT_IMAGE;
							goto cleanup;
						}
					}
				}
			}

			*pu16ImageData = u16Color;
			pu16ImageData += s32Step;
			if (pu8Translucent)
			{
				*pu8Translucent = u8Translucent;
				pu8Translucent += s32Step;
			}
		}

		pu16ImageData += s32LineAdjust;

		if (pu8Translucent)
		{
			pu8Translucent += s32LineAdjust;
		}
	}

	eResult = GC_OK;
	
cleanup:
	if (pu16Colors)
	{
		GCFreeMemory(pu16Colors);
	}

	if (eResult != GC_OK)
	{
		// Clean up!
		if (psImage->pu16ImageData)
		{
			GCFreeMemory(psImage->pu16ImageData);
			psImage->pu16ImageData = NULL;
		}

		if (psImage->pu8TranslucentMask)
		{
			GCFreeMemory(psImage->pu8TranslucentMask);
			psImage->pu8TranslucentMask = NULL;
		}
	}

	*peResult = eResult;
	return(TRUE);
}

static EGCResultCode GfxLoadTGA(UINT8 *pu8ImagePtr,
								UINT32 u32ImageSize,
								SImageGroup *psImageGroup,
								SImage *psImage)
{
	BOOL bResult;
	EGCResultCode eResult = GC_OK;

	if (FALSE == TGALoad(pu8ImagePtr,
						 u32ImageSize,
						 psImage,
						 psImageGroup,
						 &eResult))
	{
		return(eResult);
	}

	// Otherwise, we have a good image, but it's only a single since TGA doesn't
	// support multiple images

	if (GC_OK == eResult)
	{
		bResult = SingleImageFixup(psImage,
								   psImageGroup);
		if (FALSE == bResult)
		{
			eResult = GC_OUT_OF_MEMORY;
		}
	}

	return(eResult);
}

static EGCResultCode GfxLoadJPG(UINT8 *pu8ImagePtr,
							    UINT32 u32ImageSize,
							    SImageGroup *psImageGroup,
							    SImage *psImage)
{
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	INT32 s32Result;
	BOOL bResult = FALSE;
	UINT32 u32Loop;
	UINT16 *pu16DataPtr = NULL;
	UINT8 *pu8Scanline = NULL;
	EGCResultCode eResult = GC_OK;

	memset((void *) &cinfo, 0, sizeof(cinfo));

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);

	jpeg_memory_src(&cinfo,
					pu8ImagePtr,
					u32ImageSize);

	if (NULL == cinfo.src)
	{
		return(GC_OUT_OF_MEMORY);
	}

	bResult = TRUE;

	s32Result = jpeg_read_header(&cinfo, TRUE);
	if (s32Result != JPEG_HEADER_OK)
	{
		eResult = GC_CORRUPT_IMAGE;
		goto cleanup;
	}

	jpeg_start_decompress(&cinfo);

	// Let's get some specifics about this image and allocate memory appropriately.

	GCASSERT(psImage->pu16ImageData);
	pu16DataPtr = psImage->pu16ImageData;

	pu8Scanline = MemAlloc(psImage->u32Pitch * sizeof(UINT32));
	if (NULL == pu8Scanline)
	{
		eResult = GC_OUT_OF_MEMORY;
		goto cleanup;
	}

	u32Loop = psImage->u32YSize;
	while (u32Loop)
	{
		JDIMENSION s32Dimension;

		s32Dimension = jpeg_read_scanlines(&cinfo,
										   (JSAMPARRAY) &pu8Scanline,
										   1);

		if (s32Dimension != 1)
		{
			// Truncation problem of some sort
			break;
		}

		if (3 == cinfo.out_color_components)
		{
			UINT8 *pu8Src = pu8Scanline;
			UINT32 u32Loop2;

			// Convert RGB color into something we can actually use
			for (u32Loop2 = 0; u32Loop2 < psImage->u32XSize; u32Loop2++)
			{
				UINT8 u8Red;
				UINT8 u8Green;
				UINT8 u8Blue;

				u8Red = *pu8Src >> 3;	// Make 5 bits
				pu8Src++;
				u8Green = *pu8Src >> 2;	// Make 6 bits
				pu8Src++;
				u8Blue = *pu8Src >> 3;	// Make 5 bits
				pu8Src++;

				*pu16DataPtr = (u8Red << 11) | (u8Green << 5) | (u8Blue);
				++pu16DataPtr;
			}

			pu16DataPtr += (psImage->u32Pitch - psImage->u32XSize);
		}
		else
		{
			// Unknown color depth - ignore it
			pu16DataPtr += psImage->u32Pitch;
		}

		u32Loop--;
	}

	if (0 == u32Loop)
	{
		eResult = GC_OK;
	}
	else
	{
		// This means we got an error.
		eResult = GC_CORRUPT_IMAGE;
	}

cleanup:
	if (pu8Scanline)
	{
		GCFreeMemory(pu8Scanline);
		pu8Scanline = NULL;
	}

	// Only do a finish_decompress call if it's not a bad error
	if ((eResult != GC_CORRUPT_IMAGE) &&
		(eResult != GC_OUT_OF_MEMORY))
	{
		jpeg_finish_decompress(&cinfo);
	}

	jpeg_destroy_decompress(&cinfo);

	if (cinfo.src)
	{
		GCFreeMemory(cinfo.src);
		cinfo.src = NULL;
	}

	if (GC_OK == eResult)
	{
		BOOL bResult;

		bResult = SingleImageFixup(psImage,
								   psImageGroup);
		if (FALSE == bResult)
		{
			eResult = GC_OUT_OF_MEMORY;
		}
	}
	else
	{
		// Ditch the image if it's bad
		if (psImage)
		{
			GfxDeleteImage(psImage);
		}
	}

	return(eResult);
}

static int strcasecmp(const char *s1,
			  const char *s2)
{
	UINT8 u8Char1;
	UINT8 u8Char2;

	while (*s1 != '\0' && *s2 != '\0')
	{
		u8Char1 = *s1;
		u8Char2 = *s2;

		if (u8Char1 >= 'A' && u8Char1 <= 'Z')
		{
			u8Char1 += 0x20;
		}

		if (u8Char2 >= 'A' && u8Char2 <= 'Z')
		{
			u8Char2 += 0x20;
		}

		if (u8Char1 - u8Char2)
		{
			return(u8Char1 - u8Char2);
		}

		s1++;
		s2++;
	}

	return tolower(*(unsigned char *) s1) - tolower(*(unsigned char *) s2);
}

static void GfxDeleteImageInternal(SImage *psImage)
{
	if (psImage->pu16OriginalBase)
	{
		GCFreeMemory(psImage->pu16OriginalBase);
		psImage->pu16OriginalBase = NULL;
	}
	else
	{
		if (psImage->pu16ImageData)
		{
			GCFreeMemory(psImage->pu16ImageData);
			psImage->pu16ImageData = NULL;
		}
	}

	if (psImage->pu8TranslucentMaskOriginal)
	{
		GCFreeMemory(psImage->pu8TranslucentMaskOriginal);
		psImage->pu8TranslucentMaskOriginal = NULL;
	}
	else
	if (psImage->pu8TranslucentMask)
	{
		GCFreeMemory(psImage->pu8TranslucentMask);
		psImage->pu8TranslucentMask = NULL;
	}

	if (psImage->pu8TransparentOriginal)
	{
		GCFreeMemory(psImage->pu8TransparentOriginal);
		psImage->pu8TransparentOriginal = NULL;
	}
	else
	if (psImage->pu8Transparent)
	{
		GCFreeMemory(psImage->pu8Transparent);
		psImage->pu8Transparent = NULL;
	}

	if (psImage->pu8ImageData)
	{
		GCFreeMemory(psImage->pu8ImageData);
		GCASSERT(psImage->pu16Palette);
		psImage->pu8ImageData = NULL;
	}

	if (psImage->pu16Palette)
	{
		GCFreeMemory(psImage->pu16Palette);
		psImage->pu16Palette = NULL;
	}

	GCFreeMemory(psImage);
}

static SImageGroup *GfxLoadImageInternal(LEX_CHAR *peFilenameIncoming,
										 EGCResultCode *peResultCode,
										 BOOL bTrackReference)
{
	UINT32 u32ImageSize = 0;
	UINT8 *pu8ImagePtr = NULL;
	LEX_CHAR *peFilename;
	EGCResultCode eResult = GC_OK;
	GCFile eFileHandle = (GCFile) NULL;
	SGfxFile *psGfxFile = NULL;
	UINT32 u32Loop = 0;
	UINT32 u32ImagePixelCount = 0;
	UINT64 u64ImageSize = 0;
	UINT32 u32DataRead;
	SImageGroup *psImageGroup = NULL;
	SGfxImageInfo sImageInfo;
	SImage *psImage = NULL;

	GCASSERT(peFilenameIncoming);

#ifdef _WIN32
//	BlockReport("heap1.txt");
#endif
	psImageGroup = MemAlloc(sizeof(*psImageGroup));
	if (NULL == psImageGroup)
	{
		eResult = GC_OUT_OF_MEMORY;
		goto errorExit;
	}

	// Go expand the filename (if it needs expanding)
	peFilename = peFilenameIncoming;

	// Let's see if it's in our list already

//	DebugOut(" LoadImage='%s'\n", pu8Filename);

    if (bTrackReference)
    {
		GfxLockFileList();

        psGfxFile = sg_psFileHead;

        while (psGfxFile)
        {
            if (Lexstrcasecmp(peFilename,
							  psGfxFile->peGraphicsFilename) == 0)
            {
                if (peResultCode)
                {	
                    *peResultCode = GC_OK;
                }

				if (peFilename && (peFilename != peFilenameIncoming))
				{
					// Need to free our filename image

					GCFreeMemory(peFilename);
				}

				// Make a copy of the image group
				memcpy((void *) psImageGroup,
					   (void *) psGfxFile->psImageGroup,
					   sizeof(*psImageGroup));

				GfxUnlockFileList();
#ifdef _WIN32
//				BlockReport("heap2.txt");
#endif
				return(psImageGroup);
            }

            psGfxFile = psGfxFile->psNextLink;
        }

		GfxUnlockFileList();
    }

	// Figure out what type of file it is
	eResult = GfxDetermineFileType(&eFileHandle,
								   peFilename,
								   &sImageInfo);

	// If we don't know what the image type is, return an error
	if ((GC_OK == eResult) &&
		(GFXTYPE_UNKNOWN == sImageInfo.eFileType))
	{
		eResult = GC_UNKNOWN_IMAGE_TYPE;
	}

	// If there are any errors, just return. 
	if (eResult != GC_OK)
	{
		goto errorExit;
	}

	// We know how big our image is going to be. If u32Pages is set to 0, then the
	// allocation will be handled by the decompressor.

	if (sImageInfo.u32Pages)
	{
		psImage = GfxAllocateImage();
		if (NULL == psImage)
		{
			eResult = GC_OUT_OF_MEMORY;
			goto errorExit;
		}

		// We've got an image! Let's copy over the pertinent info
		psImage->u32XSize = sImageInfo.u32XSize;
		psImage->u32YSize = sImageInfo.u32YSize;

		// Pitch is 4 byte aligned
		psImage->u32Pitch = (psImage->u32XSize + 3) & 0xfffffffc;

		if (sImageInfo.u8BPP >= 16)
		{
			psImage->pu16ImageData = MemAllocNoClear(psImage->u32Pitch * psImage->u32YSize * sizeof(*psImage->pu16ImageData));
		}
		else
		{
			psImage->pu8ImageData = MemAlloc(psImage->u32Pitch * psImage->u32YSize);
		}

		if ((NULL == psImage->pu16ImageData) &&
			(NULL == psImage->pu8ImageData))
		{
			eResult = GC_OUT_OF_MEMORY;
			goto errorExit;
		}
	}

	// Let's go figure out how big the image is

	pu8ImagePtr = NULL;

	eResult = GCFileSizeByHandle(eFileHandle, &u64ImageSize);
	if (eResult != GC_OK)
	{
		GCFileClose(&eFileHandle);
		goto errorExit;
	}

	u32ImageSize = (UINT32) u64ImageSize;

	// Load up a place for it!

	if (NULL == pu8ImagePtr)
	{
		pu8ImagePtr = MemAllocNoClear(u32ImageSize);
		if (NULL == pu8ImagePtr)
		{
			GCFileClose(&eFileHandle);
			eResult = GC_OUT_OF_MEMORY;
			goto errorExit;
		}
	}

	// Now load up the image
	eResult = GCFileRead((void *) pu8ImagePtr, u32ImageSize, &u32DataRead, eFileHandle);
	if (eResult != GC_OK)
	{
		GCFileClose(&eFileHandle);
		goto errorExit;
	}

	if (u32DataRead != (INT32) u32ImageSize)
	{
		eResult = GC_IMAGE_READ_TRUNCATED;
		goto errorExit;
	}

	eResult = GCFileClose(&eFileHandle);
	if (eResult != GC_OK)
	{
		goto errorExit;
	}

	// Figure out what image type it is:

	switch (sImageInfo.eFileType)
	{
		case GFXTYPE_TGA:
		{
			eResult = GfxLoadTGA(pu8ImagePtr,
								 u32ImageSize,
								 psImageGroup,
								 psImage);
			break;
		}

		case GFXTYPE_PNG:
		{
			eResult = GfxLoadPNG(pu8ImagePtr,
								 u32ImageSize,
								 psImageGroup,
								 psImage);
			break;
		}

		case GFXTYPE_BMP:
		{
			eResult = GfxLoadBMP(pu8ImagePtr,
								 u32ImageSize,
								 psImageGroup,
								 psImage);
			break;
		}

		case GFXTYPE_GIF:
		{
			GCASSERT(NULL == psImage);
			(void) GfxLoadGIF(pu8ImagePtr,
							  u32ImageSize,
							  psImageGroup,
							  &eResult);
			break;
		}

		case GFXTYPE_JPG:
		{
			eResult = GfxLoadJPG(pu8ImagePtr,
								 u32ImageSize,
								 psImageGroup,
								 psImage);
			break;
		}

		default:
		{
			// This condition should've been caught right after the file type
			// determination attempt.
			GCASSERT(0);
			eResult = GC_UNKNOWN_IMAGE_TYPE;
			goto errorExit;
		}
	}

	if (((GC_OK == eResult) || (GC_PARTIAL_IMAGE == eResult)) &&
		(bTrackReference))
	{
		psGfxFile = MemAlloc(sizeof(*psGfxFile));
		if (NULL == psGfxFile)
		{
			eResult = GC_OUT_OF_MEMORY;
			GCFreeMemory(psImageGroup);
			goto errorExit;
		}

		GCASSERT(psGfxFile);
		psGfxFile->psImageGroup = psImageGroup;
		psImageGroup->psGfxFile = psGfxFile;

		psImageGroup = MemAlloc(sizeof(*psImageGroup));

//		DebugOut("%s: Filename='%s', addr=0x%.8x\n", __FUNCTION__, pu8FilenameIncoming, psImageGroup);

		if (NULL == psImageGroup)
		{
			eResult = GC_OUT_OF_MEMORY;
			GCFreeMemory(psImageGroup);
			GCFreeMemory(psGfxFile);
			goto errorExit;
		}

		memcpy((void *) psImageGroup, psGfxFile->psImageGroup, sizeof(*psImageGroup));

		psGfxFile->u32References = 0;
		psGfxFile->peGraphicsFilename = Lexstrdup(peFilename);
		if (NULL == psGfxFile->peGraphicsFilename)
		{
			eResult = GC_OUT_OF_MEMORY;
			GCFreeMemory(psImageGroup);
			GCFreeMemory(psGfxFile);
			goto errorExit;
		}

		psImageGroup->psGfxFile = psGfxFile;

		GfxLockFileList();
		psGfxFile->psNextLink = sg_psFileHead;
		sg_psFileHead = psGfxFile;
		psGfxFile->psPrevLink = NULL;
		if( psGfxFile->psNextLink )
		{
			psGfxFile->psNextLink->psPrevLink = psGfxFile;
		}
		GfxUnlockFileList();
	}

errorExit:
	// If our result isn't OK, just ditch it
	if ((eResult != GC_OK) && 
		(eResult != GC_PARTIAL_IMAGE) && 
		(psGfxFile) &&
		(psGfxFile->psImageGroup))
	{
		SImageGroupLink *psGrp;
		SImageGroupLink *psGrpPrior;

		psGrp = psGfxFile->psImageGroup->psLinkHead;

		while (psGrp)
		{
			GfxDeleteImageInternal(psGrp->psImage);
			psGrp->psImage = NULL;
			psGrpPrior = psGrp;
			psGrp = psGrp->psNextLink;
			GCFreeMemory(psGrpPrior);
		}
	}

	if (psImageGroup && (eResult != GC_OK) && (eResult != GC_PARTIAL_IMAGE))
	{
		// If it's a file reference and we're attempting to delete
		// an image group (like an out of memory condition), falsely
		// increment the reference so GfxDeleteImageGroup() doesn't
		// assert on a 0 reference count.
		if (psImageGroup)
		{
			if (psImageGroup->psGfxFile)
			{
				if (0 == psImageGroup->psGfxFile->u32References)
				{
					psImageGroup->psGfxFile->u32References++;
				}
			}
		}

		GfxDeleteImageGroup(psImageGroup);
		psImageGroup = NULL;
	}

	if (pu8ImagePtr)
	{
		GCFreeMemory(pu8ImagePtr);
		pu8ImagePtr = NULL;
	}

	if (peResultCode)
	{
		*peResultCode = eResult;
	}

	if (peFilename && (peFilename != peFilenameIncoming))
	{
		// Need to free our filename image

		GCFreeMemory(peFilename);
	}

	(void) GCFileClose(&eFileHandle);

	if ((eResult != GC_OK) &&
		(eResult != GC_PARTIAL_IMAGE) &&
		(psImage))
	{
		GfxDeleteImage(psImage);
	}

	return(psImageGroup);
}

SImageGroup *GfxLoadImageGroup(LEX_CHAR *peFilename,
							   EGCResultCode *peResultCode)
{
	SImageGroup *psImageGroup = NULL;

	psImageGroup = GfxLoadImageInternal(peFilename,
										peResultCode,
										TRUE);

	if ((*peResultCode != GC_OK) &&
		(*peResultCode != GC_PARTIAL_IMAGE))
	{
		if (psImageGroup)
		{
			GCFreeMemory(psImageGroup);
			psImageGroup = NULL;
		}
	}
	else
	{
		// If this asserts, then the image group head wasn't filled in by the decoder code
		if (psImageGroup)
		{
			GCASSERT(psImageGroup->psLinkHead);
			psImageGroup->psCurrentImage = psImageGroup->psLinkHead->psImage;
		}
	}
	return(psImageGroup);
}

void GfxIncRef(SImageGroup *psGroup)
{
	if (psGroup)
	{
		if (psGroup->psGfxFile)
		{
			psGroup->psGfxFile->u32References++;
			//DebugOut("%s: File='%s', References=%u\n", __FUNCTION__, psGroup->psGfxFile->peGraphicsFilename, psGroup->psGfxFile->u32References);
		}
		else
		{
			// Not a loaded graphical image (rendered) - don't do anything
		}
	}
}

void GfxDecRef(SImageGroup *psGroup)
{
	if (psGroup)
	{
		if (psGroup->psGfxFile)
		{
			GCASSERT(psGroup->psGfxFile->u32References);
			psGroup->psGfxFile->u32References--;
			//DebugOut("%s: File='%s', References=%u\n", __FUNCTION__, psGroup->psGfxFile->peGraphicsFilename, psGroup->psGfxFile->u32References);
		}
		else
		{
			// Not a loaded graphical image (rendered)
		}
	}
}

EGCResultCode GfxLoadImageRaw(LEX_CHAR *peFilename, 
							  UINT32 *pu32XSize, 
							  UINT32 *pu32YSize, 
							  UINT8* pu8BPP, 
							  UINT16 **ppu16Palette, 
							  void **ppPixels)
{
    SImageGroup *psImageGroup;
	SImage *psImage;
    EGCResultCode erc;

    psImageGroup = GfxLoadImageInternal(peFilename, &erc, FALSE);
    if (psImageGroup == NULL)
    {
        return erc;
    }

	psImage = psImageGroup->psCurrentImage;
	GCASSERT(psImage);

    *pu32XSize = psImage->u32XSize;
    *pu32YSize = psImage->u32YSize;

    // now 'steal' information from image.
    // setting pointer to NULL will cause delete routine to NOT 
    // free the pixel data -- it then becomes the responsibility of the
    // caller.
    if (psImage->pu8ImageData)
    {
        *pu8BPP = 8;
        *ppPixels = (void*) psImage->pu8ImageData;
		*ppu16Palette = psImage->pu16Palette;
        psImage->pu8ImageData = NULL;
		psImage->pu16Palette = NULL;
    }
    else if (psImage->pu16ImageData)
    {
        *pu8BPP = 16;
        *ppPixels = (void*) psImage->pu16ImageData;
        psImage->pu16ImageData = NULL;
    }
    else
    {
        GCASSERT_MSG("SImage should be 8 or 16 bpp");
    }

    // free image structure (but since we 'stole' the pointers above,
	// they will not be free'd.
    GfxDeleteImageInternal(psImage);
    return GC_OK;
}


SImage *GfxCreateEmptyImage(UINT32 u32XSize,
							UINT32 u32YSize,
							UINT8 u8BPP,
							UINT32 u32FillColor,
							BOOL bIncludeTransparency)
{
	SImage *psImage = NULL;
	UINT32 u32Loop = u32XSize * u32YSize;

	// Make sure it's only 16bpp for now

	GCASSERT(u32XSize);			// Neither of these can be 0
	GCASSERT(u32YSize);
	GCASSERT(0 == (u32FillColor >> 16));	// Make sure that we don't try to do a fill color that's larger than what the pixel map can take

	// Let's allocate an image pointer

	psImage = MemAlloc(sizeof(*psImage));
	if (NULL == psImage)
	{
		return(NULL);
	}

	memset((void *) psImage, 0, sizeof(*psImage));
	psImage->eSourceImageFileType = GFXTYPE_INTERNAL;

	// Start out indicating the image is not rotated
	psImage->eCurrentRotation = ROT_0;

	psImage->u32XSize = u32XSize;
	psImage->u32YSize = u32YSize;
	psImage->u32Pitch = u32XSize;

	if (16 == u8BPP)
	{
		psImage->pu16ImageData = MemAlloc(u32Loop * (u8BPP >> 3));	// Assume 16bpp
		if (NULL == psImage->pu16ImageData)
		{
			GCFreeMemory(psImage);
			return(NULL);
		}

		if (bIncludeTransparency)
		{
			psImage->pu8TranslucentMask = MemAlloc(u32Loop);
			if (NULL == psImage->pu8TranslucentMask)
			{
				GCFreeMemory(psImage->pu16ImageData);
				GCFreeMemory(psImage);
				return(NULL);
			}

			memset((void *) psImage->pu8TranslucentMask, 0xff, u32Loop);
		}
	}
	else
	if (8 == u8BPP)
	{
		psImage->pu8ImageData = MemAlloc(u32Loop * (u8BPP >> 3));	// Assume 8bpp
		if (NULL == psImage->pu8ImageData)
		{
			GCFreeMemory(psImage);
			return(NULL);
		}

		GCASSERT(FALSE == bIncludeTransparency);
	}
	else
	{
		GCASSERT(0);
	}

	if (16 == u8BPP)
	{
		while (u32Loop)
		{
			u32Loop--;
			psImage->pu16ImageData[u32Loop] = (UINT16) u32FillColor;	// Precision loss is OK, here
		}
	}
	else
	if (8 == u8BPP)
	{
		memset((void *) psImage->pu8ImageData, u32FillColor, u32Loop);
	}
	else
	{
		GCASSERT(0);
	}

	return(psImage);
}

void GfxSetTransparencyNonKey(UINT16 u16Color,
							  SImage *psImage)
{
	UINT16 *pu16ImageData;
	UINT8 *pu8TransparencyMask;
	UINT32 u32TotalPixels;
	UINT8 u8Mask;

	if (psImage->pu8Transparent)
	{
		GCFreeMemory(psImage->pu8Transparent);
	}

	u32TotalPixels = psImage->u32XSize * psImage->u32YSize;
	pu16ImageData = psImage->pu16ImageData;
	psImage->pu8Transparent = MemAllocNoClear(u32TotalPixels * sizeof(*psImage->pu8Transparent));
	pu8TransparencyMask = psImage->pu8Transparent;

	while (u32TotalPixels)
	{
		u8Mask = 0xff;

		if (u16Color == *pu16ImageData)
		{
			u8Mask = 0;
		}
	
		*pu8TransparencyMask = u8Mask;
		++pu16ImageData;
		++pu8TransparencyMask;
		u32TotalPixels--;
	}
}

void GfxSetImageRotation(SImage *psImage,
						 ERotation eRot)
{
	UINT32 u32SourceXSize;
	UINT32 u32SourceYSize;
	UINT32 u32TargetXSize;
	UINT32 u32TargetYSize;
	UINT32 u32TempSize;
	UINT16 u16SourcePixel;
	UINT8 u8SourceMaskByte;
	UINT16 *pu16OldImage = NULL;
	UINT8 *pu8OldMask = NULL;
	UINT32 u32XPos = 0;
	UINT32 u32YPos = 0;
	UINT32 u32Offset = 0;
	UINT32 u32NewPitch;

	// If the image is already at the stated rotation, just return.

	if (eRot == psImage->eCurrentRotation)
	{
		return;
	}

	// Our original source & target X/Y size

	u32SourceXSize = psImage->u32XSize;
	u32SourceYSize = psImage->u32YSize;

	// Start off assuming they're the same size

	u32TargetXSize = psImage->u32XSize;
	u32TargetYSize = psImage->u32YSize;
	u32NewPitch = psImage->u32Pitch;

	// Now our target size - let's figure out how to rotate it

	if ( ((ROT_0 == psImage->eCurrentRotation || ROT_180 == psImage->eCurrentRotation) && (ROT_90 == eRot || ROT_270 == eRot)) ||
		 ((ROT_90 == psImage->eCurrentRotation || ROT_270 == psImage->eCurrentRotation) && (ROT_0 == eRot || ROT_180 == eRot)) )
	{
		// Need to swap x/y sizes

		u32TempSize = u32TargetXSize;
		u32TargetXSize = u32TargetYSize;
		u32TargetYSize = u32TempSize;
		u32NewPitch = u32TargetXSize;
	}

	// Allocate a new image (16bpp)

	pu16OldImage = (UINT16 *) MemAllocNoClear(u32TargetXSize * u32TargetYSize * 2);
	GCASSERT(pu16OldImage);
	memcpy((void *) pu16OldImage, psImage->pu16ImageData, u32SourceXSize * u32SourceYSize * 2);

	if (psImage->pu8TranslucentMask || psImage->pu8Transparent)
	{
		pu8OldMask = (UINT8 *) MemAllocNoClear(u32TargetXSize * u32TargetYSize);
		GCASSERT(pu8OldMask);

		if (psImage->pu8TranslucentMask)
		{
			memcpy((void *) pu8OldMask, psImage->pu8TranslucentMask, u32TargetXSize * u32TargetYSize);
		}

		if (psImage->pu8Transparent)
		{
			memcpy((void *) pu8OldMask, psImage->pu8Transparent, u32TargetXSize * u32TargetYSize);
		}
	}

	for (u32YPos = 0; u32YPos < u32SourceYSize; u32YPos++)
	{
		for (u32XPos = 0; u32XPos < u32SourceXSize; u32XPos++)
		{
			// u32XPos/u32YPos Is in source image size/coordinates
			//
			// Now, let's figure out where to get our source pixel. For the various rotations:
			//
			// ROT_0	= *(u32XPos + (u32YPos * u32SourcePitch));
			// ROT_90	= *(u32YPos + ((u32SourceXSize - 1 - u32XPos) * u32SourcePitch))
			// ROT_180	= *((u32SourceXSize - 1 - u32XPos) + ((u32SourceYSize - 1 - u32YPos) * u32SourcePitch))
			// ROT_270  = *((u32SourceYSize - 1 - u32YPos) + (u32XPos * u32SourcePitch))

			if (ROT_0 == psImage->eCurrentRotation)
			{
				u32Offset = (u32XPos + (u32YPos * psImage->u32Pitch));
			}
			else
			if (ROT_90 == psImage->eCurrentRotation)
			{
				u32Offset = (u32YPos + ((u32SourceXSize - 1 - u32XPos) * psImage->u32Pitch));
			}
			else
			if (ROT_180 == psImage->eCurrentRotation)
			{
				u32Offset = ((u32SourceXSize - 1 - u32XPos) + ((u32SourceYSize - 1 - u32YPos) * psImage->u32Pitch));
			}
			else
			if (ROT_270 == psImage->eCurrentRotation)
			{
				u32Offset = ((u32SourceYSize - 1 - u32YPos) + (u32XPos * psImage->u32Pitch));
			}
			else
			{
				// Invalid rotation
				GCASSERT(0);
			}

			// Pull out the source pixel
			u16SourcePixel = *(pu16OldImage + u32Offset);

			// Pull out the transparency (if it exists)

			if (pu8OldMask)
			{
				u8SourceMaskByte = *(pu8OldMask + u32Offset);
			}

			// At this point the source information is in u16SourcePixel, u8SourceTransparent, u8SourceTranslucent

			if (ROT_0 == eRot)
			{
				u32Offset = (u32XPos + (u32YPos * u32NewPitch));
			}
			else
			if (ROT_90 == eRot)
			{
				u32Offset = ((u32TargetXSize - u32YPos - 1) + (u32XPos * u32NewPitch));
			}
			else
			if (ROT_180 == eRot)
			{
				u32Offset = ((u32TargetXSize - 1 - u32XPos) + ((u32TargetYSize - 1 - u32YPos) * u32NewPitch));
			}
			else
			if (ROT_270 == eRot)
			{
				u32Offset = (u32YPos + ((u32TargetYSize - 1 - u32XPos) * u32NewPitch));
			}
			else
			{
				// Invalid rotation
				GCASSERT(0);
			}

			// Now put the new pixel in the new place

			*(psImage->pu16ImageData + u32Offset) = u16SourcePixel;

			if (pu8OldMask)
			{
				if (psImage->pu8TranslucentMask)
				{
					*(psImage->pu8TranslucentMask + u32Offset) = u8SourceMaskByte;
				}
				if (psImage->pu8Transparent)
				{
					*(psImage->pu8Transparent + u32Offset) = u8SourceMaskByte;
				}
			}
		}
	}

	// Now let's kill off the old image, and replace it with the new one
	GCFreeMemory(pu16OldImage);

	if (pu8OldMask)
	{
		GCFreeMemory(pu8OldMask);
	}

	psImage->u32Pitch = u32NewPitch;
	psImage->eCurrentRotation = eRot;
	psImage->u32XSize = u32TargetXSize;
	psImage->u32YSize = u32TargetYSize;
}

static UINT32 sg_u32LayerCount = 0;

SLayer *GfxCreateLayer(void)
{
	SLayer *psLayer;
	SLayer *psLayerPtr = NULL;

	psLayer = MemAlloc(sizeof(*psLayer));
	psLayer->psImages = NULL;
	psLayer->psNextLayer = NULL;
	psLayer->psPriorLayer = NULL;

	// Run through our current layers, and attach ourselves to the end

	psLayerPtr = sg_psLayerHead;

	while (psLayerPtr && psLayerPtr->psNextLayer)
	{
		psLayerPtr = psLayerPtr->psNextLayer;
	}

	if (NULL == psLayerPtr)
	{
		// At the head of the pack

		sg_psLayerHead = psLayer;
	}
	else
	{
		psLayerPtr->psNextLayer = psLayer;
		psLayer->psPriorLayer = psLayerPtr;
	}

//	DebugOut("%s: Layer created = 0x%.8x\n", __FUNCTION__, (UINT32) psLayer);
	++sg_u32LayerCount;
	return(psLayer);
}

UINT32 GfxGetLayerPriority(SLayer *psLayer)
{
	UINT32 u32Layer = sg_u32LayerCount;
	SLayer *psLayerPtr = sg_psLayerHead;

	while (u32Layer)
	{
		u32Layer--;

		if (psLayerPtr == psLayer)
		{
			return(u32Layer);
		}

		psLayerPtr = psLayerPtr->psNextLayer;
	}

	// Not found - bail out
	return(0xffffffff);
}

SLayer *GfxGetLayerPointerByPriority(UINT32 u32Priority)
{
	SLayer *psLayerPtr = sg_psLayerHead;

	if (u32Priority >= sg_u32LayerCount)
	{
		return(NULL);
	}

	u32Priority = (sg_u32LayerCount - 1) - u32Priority;

	while (u32Priority)
	{
		psLayerPtr = psLayerPtr->psNextLayer;
		--u32Priority;
	}

	GCASSERT(psLayerPtr);
	return(psLayerPtr);
}

void GfxSetLayerPriority(SLayer *psLayerToSet,
						 SLayer *psLayerReference,
						 BOOL bFront)
{
	SImageInstance *psImageInstance = NULL;
	SLayer *psLayerPrior = NULL;
	SLayer *psLayerPtr = NULL;

	// If the layer to set against is itself, just return OK and don't do anything
	if (psLayerToSet == psLayerReference)
	{
		return;
	}

	// Run through the current layer and hide everything
	psImageInstance = psLayerToSet->psImages;

	while (psImageInstance)
	{
		if (psImageInstance->bImageVisible)
		{
			GfxUpdateImage(psImageInstance,
						   TRUE);
		}

		psImageInstance = psImageInstance->psNextLink;
	}

	// Remove the layer to set
	psLayerPtr = sg_psLayerHead;
	psLayerPrior = NULL;
	while (psLayerPtr != psLayerToSet)
	{
		psLayerPrior = psLayerPtr;
		psLayerPtr = psLayerPtr->psNextLayer;
	}

	if (NULL == psLayerPrior)
	{
		GCASSERT(psLayerToSet == sg_psLayerHead);
		sg_psLayerHead = sg_psLayerHead->psNextLayer;
		if (sg_psLayerHead->psNextLayer)
		{
			// This new layer is the head of the list now and should act like it
			sg_psLayerHead->psNextLayer->psPriorLayer = NULL;
		}
	}
	else
	{
		GCASSERT(psLayerPtr);
		psLayerPrior->psNextLayer = psLayerPtr->psNextLayer;
		if (psLayerPtr->psNextLayer)
		{
			psLayerPtr->psNextLayer->psPriorLayer = psLayerPrior;
		}
	}

	// Set up psLayerPrior/ptr to point to prior and next (insertion point)
	psLayerPtr = sg_psLayerHead;
	psLayerPrior = NULL;

	if (NULL == psLayerReference)
	{
		psLayerToSet->psNextLayer = sg_psLayerHead;
		psLayerToSet->psPriorLayer = NULL;
		sg_psLayerHead = psLayerToSet;
	}
	else
	{
		psLayerToSet->psNextLayer = psLayerReference->psNextLayer;
		psLayerReference->psNextLayer = psLayerToSet;
		psLayerToSet->psPriorLayer = psLayerReference;
	}

	// Recalc the intersections
	GfxRecalcIntersections();

	// Redraw all the images in this layer
	psImageInstance = psLayerToSet->psImages;

	while (psImageInstance)
	{
		if (psImageInstance->bImageVisible)
		{
			GfxUpdateImage(psImageInstance,
						   FALSE);
		}

		psImageInstance = psImageInstance->psNextLink;
	}
}

UINT32 GfxGetLayerPriorityByImageInstance(SImageInstance *psImageInstance)
{
	if ((NULL == psImageInstance) ||
		(NULL == psImageInstance->psParentLayer))
	{
		return(0xffffffff);
	}

	return(GfxGetLayerPriority(psImageInstance->psParentLayer));
}

SLayer *GfxCreateLayerBefore(SLayer *psLayerBefore)
{
	SLayer *psLayer;
	SLayer *psLayerPtr = NULL;

	psLayer = MemAlloc(sizeof(*psLayer));
	psLayer->psImages = NULL;
	psLayer->psNextLayer = NULL;
	psLayer->psPriorLayer = NULL;

	// Run through our current layers, and attach ourselves to the end

	psLayerPtr = sg_psLayerHead;

	while (psLayerPtr != psLayerBefore && psLayerPtr->psNextLayer)
	{
		psLayerPtr = psLayerPtr->psNextLayer;
	}

	if (NULL == psLayerPtr)
	{
		// Layer not found - just free the layer and return
		GCFreeMemory(psLayer);
		return(NULL);
	}
	else
	{
		if (NULL == psLayerPtr->psPriorLayer)
		{
			GCASSERT(psLayerPtr == sg_psLayerHead);

			// We're at the head of the list - insert this item

			// Connect the new node prior to the desired layer
			psLayer->psNextLayer = psLayerPtr;
			psLayerPtr->psPriorLayer = psLayer;
			sg_psLayerHead = psLayer;
		}
		else
		{
			// We're between two nodes

			// Link the prior node to this new node
			psLayer->psPriorLayer = psLayerPtr->psPriorLayer;
			psLayerPtr->psPriorLayer->psNextLayer = psLayer;

			// Link this new node to the next node
			psLayer->psNextLayer = psLayerPtr;
			psLayerPtr->psPriorLayer = psLayer;
		}
	}

	++sg_u32LayerCount;
	return(psLayer);
}

SImageGroup *GfxImageGroupCreate(void)
{
	SImageGroup *psImageGroup = NULL;

	psImageGroup = MemAlloc(sizeof(*psImageGroup));
	return(psImageGroup);
}

SImageGroup *GfxImageGroupAppend(SImageGroup *psGrp,
								 SImage *psImage)
{
	SImageGroupLink *psGrpLink = NULL;

	psGrpLink = MemAlloc(sizeof(*psGrp->psLinkTail));
	if (NULL == psGrpLink)
	{
		return(NULL);
	}

	psGrp->u32FrameCount++;

	if (psGrp->psLinkTail)
	{
		psGrp->psLinkTail->psNextLink = psGrpLink;
		psGrp->psLinkTail = psGrpLink;
	}
	else
	{
		// First item in the list
		psGrp->psLinkHead = psGrpLink;
		psGrp->psLinkTail = psGrpLink;
		psGrp->psCurrentImage = psImage;
	}
	
	psGrpLink->psImage = psImage;
	return(psGrp);
}

static BOOL GfxImageIsSolid(SImageInstance *psImageInstance)
{
	GCASSERT(psImageInstance);

	// If the image doesn't have a transparency or translucency mask, then it's solid

	if ((psImageInstance->psImage->pu8Transparent) ||
		(psImageInstance->psImage->pu8TranslucentMask) ||
		(psImageInstance->psImage->bTranslucent))
	{
		return(FALSE);
	}
	
	// Looks like it's solid!
	return(TRUE);
}

static void GfxBlitImageRegion(SImageInstance *psImage,
							   UINT32 u32XPos,
							   UINT32 u32YPos,
							   UINT32 u32XSize,
							   UINT32 u32YSize,
							   BOOL bDrawAsSolid)
{
	INT32 s32XBlitSize;
	INT32 s32YBlitSize;
	UINT16 *pu16Src;
	UINT8 *pu8Src;
	UINT8 *pu8Mask = psImage->pu8Transparent;
	UINT8 *pu8TranslucentMask = psImage->pu8TranslucentMask;
	UINT16 *pu16Dest;

	// Now destination screen surface position
	pu16Dest = sg_pu16SurfaceLayer + u32XPos +
			   (u32YPos * sg_u32SurfaceLayerPitch);

	// Let's draw the partial image if it's in range

	pu16Src = psImage->pu16SrcPointer;
	pu8Src = psImage->pu8SrcPointer;

	// psDestImage = The target/partial redraw image region 
	// psSrcImage = replaced with u32X/YPos and u32X/YSize

	// Get our X and Y blit sizes, and adjust our source/dest pointers
	// psImage is the pointer to the image we're trying to cover with the
	// image in psImagePtr.

	if (u32XPos >= psImage->u32XPos)
	{
		// X offset into image instance
		UINT32 u32Temp = u32XPos - psImage->u32XPos;

		// Add the additional offset of the image instance into the image itself
		u32Temp += psImage->u32XOffset;

		// Apply the offset to all the source buffers
		pu16Src += u32Temp;
		pu8Src += u32Temp;
		pu8Mask += u32Temp;
		pu8TranslucentMask += u32Temp;

		s32XBlitSize = ((psImage->u32XSizeClipped + psImage->u32XPos) - u32XPos);

		if (s32XBlitSize > (INT32) psImage->u32XSizeClipped)
		{
			s32XBlitSize = (INT32) psImage->u32XSizeClipped;
		}

		if (s32XBlitSize > (INT32) u32XSize)
		{
			s32XBlitSize = (INT32) u32XSize;
		}
	}
	else
	{
		s32XBlitSize = u32XSize - (psImage->u32XPos - u32XPos);

		if (s32XBlitSize > (INT32) psImage->u32XSizeClipped)
		{
			s32XBlitSize = (INT32) psImage->u32XSizeClipped;
		}

		pu16Dest += (psImage->u32XPos - u32XPos);
	}

	if (s32XBlitSize <= 0)
	{
		return;
	}

	if (u32YPos >= psImage->u32YPos)
	{
		// Y offset into image instance
		UINT32 u32Temp = ((u32YPos - psImage->u32YPos) * psImage->psImage->u32Pitch);

		// Add the additional offset of the image instance into the image itself
		u32Temp += (psImage->u32YOffset * psImage->psImage->u32Pitch);

		// Apply the offset to all the source buffers
		pu16Src += u32Temp;
		pu8Src += u32Temp;
		pu8Mask += u32Temp;
		pu8TranslucentMask += u32Temp;

		s32YBlitSize = ((psImage->u32YSizeClipped + psImage->u32YPos) - u32YPos);

		if (s32YBlitSize > (INT32) psImage->u32YSizeClipped)
		{
			s32YBlitSize = (INT32) psImage->u32YSizeClipped;
		}

		if (s32YBlitSize > (INT32) u32YSize)
		{
			s32YBlitSize = (INT32) u32YSize;
		}
	}
	else
	{
		s32YBlitSize = u32YSize - (psImage->u32YPos - u32YPos);

		if (s32YBlitSize > (INT32) psImage->u32YSizeClipped)
		{
			s32YBlitSize = (INT32) psImage->u32YSizeClipped;
		}

		pu16Dest += ((psImage->u32YPos - u32YPos) * sg_u32SurfaceLayerPitch);
	}

	if (s32YBlitSize <= 0)
	{
		return;
	}

//	DebugOut("x/ypos=%d/%d - x/ysize=%d/%d\n", u32XPos, u32YPos, s32XBlitSize, s32YBlitSize);

	if (psImage->psImage->pu16ImageData)
	{
		UINT16 *pu16CLUT = NULL;

		if (psImage->psCLUT)
		{
			pu16CLUT = (UINT16 *) psImage->psCLUT->pvCLUT;
		}

		// Only blit if there's something to blit

		if (psImage->psImage->pu8Transparent)
		{
			UINT32 u32Loop;

			// Image is transparent

			while (s32YBlitSize)
			{
//#ifdef WIN32
#if 1
				if (0xff == psImage->u8Intensity)
				{
					// Full intensity
					u32Loop = (UINT32) s32XBlitSize;
					while (u32Loop)
					{
						if (0 == *pu8Mask)
						{
							*pu16Dest = *pu16Src;
						}

						pu16Dest++;
						pu16Src++;
						pu8Mask++;
						u32Loop--;
					}
				}
				else
				{
					UINT8 u8MaskByte = psImage->u8Intensity;

					// Not full intensity. Need to do the ugly shading

					u32Loop = (UINT32) s32XBlitSize;
					while (u32Loop)
					{
						if (0 == *pu8Mask)
						{
							UINT16 u16Src = *pu16Src;
							UINT16 u16Dest = *pu16Dest;
							UINT8 u8Red, u8Green, u8Blue;

							// It's partial/ugly. Time to separate the bands, do the add/multiply, etc...
							u8Red = (((u16Src >> 11) * (u8MaskByte)) >> 8) +
									(((u16Dest >> 11) * (0xff - u8MaskByte)) >> 8);

							u8Green = ((((u16Src >> 5) & 0x3f) * (u8MaskByte)) >> 8) +
									  ((((u16Dest >> 5) & 0x3f) * (0xff - u8MaskByte)) >> 8);

							u8Blue = (((u16Src & 0x1f) * (u8MaskByte)) >> 8) +
									 (((u16Dest & 0x1f) * (0xff - u8MaskByte)) >> 8);

							*pu16Dest = sg_u16RedGradientSaturation[u8Red] |
										sg_u16GreenGradientSaturation[u8Green] |
										sg_u16BlueGradientSaturation[u8Blue];
						}

						pu16Dest++;
						pu16Src++;
						pu8Mask++;
						u32Loop--;
					}
				}

				pu16Src += (psImage->psImage->u32Pitch - s32XBlitSize);
				pu8Mask += (psImage->psImage->u32Pitch - s32XBlitSize);
				pu16Dest += (sg_u32SurfaceLayerPitch - s32XBlitSize);
#else
				ARMBlitTransparent(s32XBlitSize,
								   pu8Mask,
								   pu16Src,
								   pu16Dest);
				pu16Src += psSrcImage->psImage->u32Pitch;
				pu8TranslucentMask += psSrcImage->psImage->u32XSize;
				pu16Dest += sg_u32SurfaceLayerPitch;

#endif
				s32YBlitSize--;
			}
		}
		else
		if (psImage->psImage->bTranslucent)
		{
			// Image is translucent
			UINT32 u32Loop;
			UINT8 u8Red;
			UINT8 u8Green;
			UINT8 u8Blue;
			UINT8 u8ImageTranslucency = psImage->u8Intensity;

			while (s32YBlitSize)
			{
				UINT8 u8MaskByte;

				u32Loop = (UINT32) s32XBlitSize;

				if (0xff == u8ImageTranslucency)
				{
					// Completely solid

					while (u32Loop)
					{
						UINT16 u16Dest;
						UINT16 u16Src;

						u8MaskByte = *pu8TranslucentMask;
						u16Dest = *pu16Dest;
						u16Src = *pu16Src;

						if (u8MaskByte < 0x4)
						{
							// Completely solid
							*pu16Dest = u16Src;
						}
						else
						if (u8MaskByte >= 0xfc)
						{
							// Completely transparent - don't touch
						}
						else
						{
							// It's partial/ugly. Time to separate the bands, do the add/multiply, etc...
							u8Red = (((u16Src >> 11) * (0xff - u8MaskByte)) >> 8) +
									(((u16Dest >> 11) * (u8MaskByte)) >> 8);

							u8Green = ((((u16Src >> 5) & 0x3f) * (0xff - u8MaskByte)) >> 8) +
									  ((((u16Dest >> 5) & 0x3f) * (u8MaskByte)) >> 8);

							u8Blue = (((u16Src & 0x1f) * (0xff - u8MaskByte)) >> 8) +
									 (((u16Dest & 0x1f) * (u8MaskByte)) >> 8);

							*pu16Dest = sg_u16RedGradientSaturation[u8Red] |
										sg_u16GreenGradientSaturation[u8Green] |
										sg_u16BlueGradientSaturation[u8Blue];
						}

						pu16Dest++;
						pu16Src++;
						pu8TranslucentMask++;
						u32Loop--;
					}
				}
				else
				{
					UINT8 u8MaskByte;

					// Translucency

					u32Loop = (UINT32) s32XBlitSize;

					while (u32Loop)
					{
						UINT16 u16Dest;
						UINT16 u16Src;

						u8MaskByte = 0xff - (((0xff - *pu8TranslucentMask) * u8ImageTranslucency) >> 8);
						u16Dest = *pu16Dest;
						u16Src = *pu16Src;

						if (u8MaskByte < 0x4)
						{
							// Completely solid
							*pu16Dest = u16Src;
						}
						else
						if (u8MaskByte >= 0xfc)
						{
							// Completely transparent - don't touch
						}
						else
						{
							// It's partial/ugly. Time to separate the bands, do the add/multiply, etc...
							u8Red = (((u16Src >> 11) * (0xff - u8MaskByte)) >> 8) +
									(((u16Dest >> 11) * (u8MaskByte)) >> 8);

							u8Green = ((((u16Src >> 5) & 0x3f) * (0xff - u8MaskByte)) >> 8) +
									  ((((u16Dest >> 5) & 0x3f) * (u8MaskByte)) >> 8);

							u8Blue = (((u16Src & 0x1f) * (0xff - u8MaskByte)) >> 8) +
									 (((u16Dest & 0x1f) * (u8MaskByte)) >> 8);

							*pu16Dest = sg_u16RedGradientSaturation[u8Red] |
										sg_u16GreenGradientSaturation[u8Green] |
										sg_u16BlueGradientSaturation[u8Blue];
						}

						pu16Dest++;
						pu16Src++;
						pu8TranslucentMask++;
						u32Loop--;
					}
				}

				pu16Src += (psImage->psImage->u32Pitch - s32XBlitSize);
				pu8TranslucentMask += (psImage->psImage->u32XSize - s32XBlitSize);
				pu16Dest += (sg_u32SurfaceLayerPitch - s32XBlitSize);

				s32YBlitSize--;
			}
		}
		else
		{
			// Image is solid

			if (0xff == psImage->u8Intensity)
			{
				while (s32YBlitSize)
				{
					memcpy((void *) pu16Dest, (void *) pu16Src, (size_t) (s32XBlitSize << 1));
					pu16Src += psImage->psImage->u32Pitch;
					pu16Dest += sg_u32SurfaceLayerPitch;

					s32YBlitSize--;
				}
			}
			else
			if (0 == psImage->u8Intensity)
			{
				// Don't do anything - can't see the window
			}
			else
			{
				UINT8 u8Intensity = 0xff - psImage->u8Intensity;

				while (s32YBlitSize)
				{
					UINT32 u32Loop;
					UINT16 u16Dest;
					UINT16 u16Src;
					UINT8 u8Red, u8Green, u8Blue;

					u32Loop = (UINT32) (s32XBlitSize);

					while (u32Loop)
					{
						u16Dest = *pu16Dest;
						u16Src = *pu16Src;

						u8Red = (((u16Src >> 11) * (0xff - u8Intensity)) >> 8) +
								(((u16Dest >> 11) * (u8Intensity)) >> 8);

						u8Green = ((((u16Src >> 5) & 0x3f) * (0xff - u8Intensity)) >> 8) +
								  ((((u16Dest >> 5) & 0x3f) * (u8Intensity)) >> 8);

						u8Blue = (((u16Src & 0x1f) * (0xff - u8Intensity)) >> 8) +
								 (((u16Dest & 0x1f) * (u8Intensity)) >> 8);

						*pu16Dest = sg_u16RedGradientSaturation[u8Red] |
									sg_u16GreenGradientSaturation[u8Green] |
									sg_u16BlueGradientSaturation[u8Blue];

						++pu16Dest;
						++pu16Src;
						u32Loop--;
					}

					pu16Src += (psImage->psImage->u32Pitch - s32XBlitSize);
					pu16Dest += (sg_u32SurfaceLayerPitch - s32XBlitSize);
					s32YBlitSize--;
				}
			}
		}
	}
	else
	{
		UINT16 *pu16Palette;
		UINT16 u16TransparentIndex;

		// 8BPP Image!

		u16TransparentIndex = psImage->psImage->u16TransparentIndex;
		pu16Palette = psImage->psImage->pu16Palette;

		while (s32YBlitSize)
		{
			INT32 s32Loop = 0;

			s32Loop = s32XBlitSize;

			while (s32Loop--)
			{
				UINT8 u8Data;

				u8Data = *pu8Src;
				pu8Src++;
				if ((UINT16) u8Data != u16TransparentIndex)
				{
					*pu16Dest = pu16Palette[u8Data];
				}
				++pu16Dest;
			}

			pu8Src += (psImage->psImage->u32Pitch - s32XBlitSize);
			pu16Dest += (sg_u32SurfaceLayerPitch - s32XBlitSize);

			s32YBlitSize--;
		}
	}
}

static void GfxUpdateImageInternal(SImageInstance *psDestImage,
								   BOOL bEraseImage,
								   UINT32 u32XRelativePos,		// Relative XPos/Ypos within psDestImage
								   UINT32 u32YRelativePos,
								   UINT32 u32XSize,		// Size within psDestImage
								   UINT32 u32YSize)
{
	SZBufferLink *psZPtr;
	UINT32 u32XDirtyBufferSize;
	UINT32 u32YDirtyBufferSize;
	UINT8 *pu8DirtyBuffer;
	INT32 s32XBlitSize = (INT32) u32XSize;
	INT32 s32YBlitSize = (INT32) u32YSize;
	BOOL bDrawTargetAsSolid = FALSE;

	if ((FALSE == bEraseImage) && (FALSE == psDestImage->bImageVisible))
	{
		return;
	}

	// See if this violates the backbuffer surface
	if (((INT32) u32XRelativePos + s32XBlitSize) > (INT32) psDestImage->u32XSizeClipped)
	{
		s32XBlitSize = psDestImage->u32XSizeClipped - ((INT32) u32XRelativePos);
	}

	if (((INT32) u32YRelativePos + s32YBlitSize) > (INT32) psDestImage->u32YSizeClipped)
	{
		s32YBlitSize = psDestImage->u32YSizeClipped - ((INT32) u32YRelativePos);
	}

	if ((s32XBlitSize <= 0) || (s32YBlitSize <= 0))
	{
		return;
	}

	// If we're not erasing the image and we have an image in front of this
	// image region that is completely obscured by it, just return.

	psZPtr = psDestImage->psZBufferFront;
	while (psZPtr)
	{
		// If the image in front of this image obscures the region that we are
		// redrawing, just return and don't do anything since it isn't visible.

		if ((psZPtr->psImageInstance->bImageVisible) &&
			(GfxImageIsSolid(psZPtr->psImageInstance)) &&
			(GfxCoordinatesObscure(psZPtr->psImageInstance,
								   u32XRelativePos + psDestImage->u32XPos,
								   u32YRelativePos + psDestImage->u32YPos,
								   u32XSize,
								   u32YSize)))
		{
			// Image to be drawn on top totally obscures this update region,
			// it's visible, and it's solid. Just return.
			return;
		}

		psZPtr = psZPtr->psNextLink;
	}

	GCASSERT(sg_pu8DirtyBuffer);

	// Mark the dirty buffer region

	pu8DirtyBuffer = sg_pu8DirtyBuffer + 
					 (((psDestImage->u32XPos + u32XRelativePos) / DIRTY_X_BLOCK_SIZE) +
					 (((psDestImage->u32YPos + u32YRelativePos) / DIRTY_Y_BLOCK_SIZE) * sg_u32DirtyBufferPitch));

	u32XDirtyBufferSize = ((((psDestImage->u32XPos + u32XRelativePos) & (DIRTY_X_BLOCK_SIZE - 1)) + s32XBlitSize + (DIRTY_X_BLOCK_SIZE - 1)) / DIRTY_X_BLOCK_SIZE);
	u32YDirtyBufferSize = ((((psDestImage->u32YPos + u32YRelativePos) & (DIRTY_Y_BLOCK_SIZE - 1)) + s32YBlitSize + (DIRTY_Y_BLOCK_SIZE - 1)) / DIRTY_Y_BLOCK_SIZE);

	GCASSERT(u32YDirtyBufferSize);

	// Now update the dirty buffer

	while (u32YDirtyBufferSize)
	{
		memset((void *) pu8DirtyBuffer, 0xff, (size_t) (u32XDirtyBufferSize));

		pu8DirtyBuffer += sg_u32DirtyBufferPitch;
		u32YDirtyBufferSize--;
	}

	// Run through all of the layers, back to front, and update them. If we're 
	// erasing, let's first erase the image, then redraw everything affected

	if (bEraseImage)
	{
		UINT16 *pu16Dest = sg_pu16SurfaceLayer + (u32XRelativePos + psDestImage->u32XPos) + 
						   ((u32YRelativePos + psDestImage->u32YPos) * sg_u32SurfaceLayerPitch);

		while (s32YBlitSize)
		{
			memset((void *) pu16Dest, 0, (size_t) (s32XBlitSize << 1));
			pu16Dest += sg_u32SurfaceLayerPitch;

			s32YBlitSize--;
		}
	}

	// If the image that we're about ready to blit is solid, then don't bother
	// drawing anything underneath it

	if ((FALSE == GfxImageIsSolid(psDestImage)) ||
		(bEraseImage))
	{
		SZBufferLink *psZLastValid = NULL;

		// Image isn't solid, so we need to figure out what images are available
		// and draw, back to front.

		bDrawTargetAsSolid = FALSE;

		// Find the bottom-most image, then draw, back to front

		psZPtr = psDestImage->psZBufferBehind;

		while (psZPtr)
		{
			psZLastValid = psZPtr;
			psZPtr->bDrawImage = FALSE;

			// If the coordinates intersect, indicate that it should be drawn

			// If the image is visible, the images intersect, and is solid, 
			// then stop - no need to go any further.


			if ((psZPtr->psImageInstance->bImageVisible) && 
				(GfxCoordinatesIntersect(psZPtr->psImageInstance,
										 u32XRelativePos + psDestImage->u32XPos,
										 u32YRelativePos + psDestImage->u32YPos,
										 u32XSize,
										 u32YSize)))
			{
				psZPtr->bDrawImage = TRUE;

				if (GfxImageIsSolid(psZPtr->psImageInstance) &&
					GfxCoordinatesObscure(psZPtr->psImageInstance,
										  u32XRelativePos + psDestImage->u32XPos,
										  u32YRelativePos + psDestImage->u32YPos,
										  u32XSize,
										  u32YSize))
				{
					// Optimization - if our image is solid, don't bother drawing anything
					// below it since this totally obscures it.
					break;
				}
			}

			psZPtr = psZPtr->psNextLink;
		}

		// Let's draw in reverse order if we have something to draw

		while (psZLastValid)
		{
			if (psZLastValid->bDrawImage)
			{
				// Image is visible/intersects - go draw the intersection

				GfxBlitImageRegion(psZLastValid->psImageInstance,
								   u32XRelativePos + psDestImage->u32XPos,
								   u32YRelativePos + psDestImage->u32YPos,
								   u32XSize,
								   u32YSize,
								   FALSE);
			}

			psZLastValid->bDrawImage = FALSE;
			psZLastValid = psZLastValid->psPriorLink;
		}
	}
	else
	{
		// This means the target image is solid
		bDrawTargetAsSolid = TRUE;
	}

	// Okay - now the target image gets to be drawn, but only if it's visible
	// and if we're not erasing the image

	if ((psDestImage->bImageVisible) &&
		(FALSE == bEraseImage))
	{
		// Draw target image
		GfxBlitImageRegion(psDestImage,
						   u32XRelativePos + psDestImage->u32XPos,
						   u32YRelativePos + psDestImage->u32YPos,
						   u32XSize,
						   u32YSize,
						   FALSE);
	}

	// The images underneath this window and the target window have been taken
	// care of. Time to draw everything above this window.

	psZPtr = psDestImage->psZBufferFront;

	// Run through and update the intersection list and see if there's anything we
	// can skip.

	while (psZPtr)
	{
		psZPtr->bDrawImage = FALSE;

		if ((psZPtr->psImageInstance->bImageVisible) && 
			(GfxCoordinatesIntersect(psZPtr->psImageInstance,
									 u32XRelativePos + psDestImage->u32XPos,
									 u32YRelativePos + psDestImage->u32YPos,
									 u32XSize,
									 u32YSize)))
		{
			psZPtr->bDrawImage = TRUE;
		}

		psZPtr = psZPtr->psNextLink;
	}

	// Now run through and draw all images that intersect that are on top
	// of the target image

	psZPtr = psDestImage->psZBufferFront;

	while (psZPtr)
	{
		if (psZPtr->bDrawImage)
		{
			// Go draw the image
			GfxBlitImageRegion(psZPtr->psImageInstance,
							   u32XRelativePos + psDestImage->u32XPos,
							   u32YRelativePos + psDestImage->u32YPos,
							   u32XSize,
							   u32YSize,
							   FALSE);
		}

		psZPtr = psZPtr->psNextLink;
	}
}

void GfxUpdateImageRegion(SImageInstance *psDestImage,
						  BOOL bEraseImage,
						  UINT32 u32XPos,
						  UINT32 u32YPos,
						  UINT32 u32XSize,
						  UINT32 u32YSize)
{
	GfxUpdateImageInternal(psDestImage,
						   bEraseImage,
						   u32XPos,
						   u32YPos,
						   u32XSize,
						   u32YSize);
}

void GfxUpdateImage(SImageInstance *psDestImage,
					BOOL bEraseImage)
{
	GfxUpdateImageInternal(psDestImage,
						   bEraseImage,
						   0,
						   0,
						   psDestImage->u32XSizeClipped,
						   psDestImage->u32YSizeClipped);
}

static void GfxRefresh(void)
{
	SLayer *psLayer;
	SImageInstance *psImageInstance;

	psLayer = sg_psLayerHead;

	while (psLayer)
	{
		psImageInstance = psLayer->psImages;

		while (psImageInstance)
		{
			psImageInstance->u32XSizeClipped = psImageInstance->psImage->u32XSize;
			psImageInstance->u32YSizeClipped = psImageInstance->psImage->u32YSize;
			psImageInstance->pu16SrcPointer = psImageInstance->psImage->pu16ImageData;
			psImageInstance->pu8SrcPointer = psImageInstance->psImage->pu8ImageData;
			psImageInstance->pu8Transparent = psImageInstance->psImage->pu8Transparent;
			psImageInstance->pu8TranslucentMask = psImageInstance->psImage->pu8TranslucentMask;

			// Let's see if it's off the screen on either axis. If it is, zero the size.
			// If it's not, but the X position is negative, let's adjust things accordingly

			if (psImageInstance->s32XPos < 0)
			{
				if ((-psImageInstance->s32XPos) >= (INT32) psImageInstance->psImage->u32XSize)
				{
					psImageInstance->u32XSizeClipped = 0;
				}
				else
				{
					psImageInstance->u32XSizeClipped = (UINT32) (((INT32) psImageInstance->psImage->u32XSize) + psImageInstance->s32XPos);
					psImageInstance->pu16SrcPointer += (-psImageInstance->s32XPos);

					if (psImageInstance->pu8Transparent)
					{
						psImageInstance->pu8Transparent += (-psImageInstance->s32XPos);
					}

					if (psImageInstance->pu8TranslucentMask)
					{
						psImageInstance->pu8TranslucentMask += (-psImageInstance->s32XPos);
					}

					psImageInstance->u32XPos = 0;
				}
			}
			else
			{
				psImageInstance->u32XPos = (UINT32) psImageInstance->s32XPos;
			}

			if (psImageInstance->s32YPos < 0)
			{
				if ((-psImageInstance->s32YPos) >= (INT32) psImageInstance->psImage->u32YSize)
				{
					psImageInstance->u32YSizeClipped = 0;
				}
				else
				{
					psImageInstance->u32YSizeClipped = (UINT32) (((INT32) psImageInstance->psImage->u32YSize) + psImageInstance->s32YPos);
					psImageInstance->pu16SrcPointer += ((-psImageInstance->s32YPos) * psImageInstance->psImage->u32Pitch);

					if (psImageInstance->pu8Transparent)
					{
						psImageInstance->pu8Transparent += ((-psImageInstance->s32YPos) * psImageInstance->psImage->u32Pitch);
					}

					if (psImageInstance->pu8TranslucentMask)
					{
						psImageInstance->pu8TranslucentMask += ((-psImageInstance->s32YPos) * psImageInstance->psImage->u32Pitch);
					}

					psImageInstance->u32YPos = 0;
				}
			}
			else
			{
				psImageInstance->u32YPos = (UINT32) psImageInstance->s32YPos;
			}

			// Clip the instance if it goes beyond the surface

			if ((psImageInstance->u32XPos + psImageInstance->u32XSizeClipped) > sg_u32SurfaceXSize)
			{
				psImageInstance->u32XSizeClipped = sg_u32SurfaceXSize - psImageInstance->u32XPos;
			}

			if ((psImageInstance->u32YPos + psImageInstance->u32YSizeClipped) > sg_u32SurfaceYSize)
			{
				psImageInstance->u32YSizeClipped = sg_u32SurfaceYSize - psImageInstance->u32YPos;
			}

			// If our clipped size is too big, zero it

			if (psImageInstance->u32XSizeClipped > psImageInstance->psImage->u32XSize)
			{
				psImageInstance->u32XSizeClipped = 0;
			}

			if (psImageInstance->u32YSizeClipped > psImageInstance->psImage->u32YSize)
			{
				psImageInstance->u32YSizeClipped = 0;
			}

			psImageInstance->pu16DestSurfacePointer = sg_pu16SurfaceLayer;
			psImageInstance->pu16DestSurfacePointer += (psImageInstance->u32XPos + (psImageInstance->u32YPos * sg_u32SurfaceLayerPitch));

			// Now update it, but only if the image is visible
			
			if (psImageInstance->bImageVisible)
			{
				GfxUpdateImage(psImageInstance, FALSE);
			}

			// Update the destination surface pointer


			psImageInstance = psImageInstance->psNextLink;
		}

		psLayer = psLayer->psNextLayer;
	}
}


static void GfxSetImageInstanceInternal(SImageInstance *psImageInstance,
										INT32 s32XPos,
										INT32 s32YPos,
										BOOL bVisible,
										BOOL bForcedRedraw)
{
	// Go erase the image and redraw everything else

	if ((FALSE == psImageInstance->bImageVisible) &&
		(FALSE == bVisible))
	{
		if (s32XPos != NO_CHANGE)
		{
			psImageInstance->s32XPos = s32XPos;
		}

		if (s32YPos != NO_CHANGE)
		{
			psImageInstance->s32YPos = s32YPos;
		}

		return;
	}

/*	if (s32XPos == psImageInstance->s32XPos)
	{
		s32XPos = NO_CHANGE;
	}

	if (s32YPos == psImageInstance->s32YPos)
	{
		s32YPos = NO_CHANGE;
	}
*/

	if ((NO_CHANGE == s32XPos) &&
		(NO_CHANGE == s32YPos) &&
		(bVisible == psImageInstance->bImageVisible))
	{
		// No changes - just return
		return;
	}

	GfxUpdateImage(psImageInstance, TRUE);

	if (s32XPos != NO_CHANGE)
	{
		psImageInstance->s32XPos = s32XPos;
	}

	if (s32YPos != NO_CHANGE)
	{
		psImageInstance->s32YPos = s32YPos;
	}

	psImageInstance->bImageVisible = bVisible;

	GfxRecalcIntersections();

	if (FALSE == bVisible)
	{
		return;
	}

	psImageInstance->pu16SrcPointer = psImageInstance->psImage->pu16ImageData;
	psImageInstance->pu8SrcPointer = psImageInstance->psImage->pu8ImageData;
	psImageInstance->pu8Transparent = psImageInstance->psImage->pu8Transparent;
	psImageInstance->pu8TranslucentMask = psImageInstance->psImage->pu8TranslucentMask;

	// Let's see if it's off the screen on either axis. If it is, zero the size.
	// If it's not, but the X position is negative, let's adjust things accordingly

	if (psImageInstance->s32XPos < 0)
	{
		if ((-psImageInstance->s32XPos) >= (INT32) psImageInstance->psImage->u32XSize)
		{
			psImageInstance->u32XSizeClipped = 0;
		}
		else
		{
			psImageInstance->u32XSizeClipped = (UINT32) (((INT32) psImageInstance->psImage->u32XSize) + psImageInstance->s32XPos);

			if (psImageInstance->pu16SrcPointer)
			{
				psImageInstance->pu16SrcPointer += (-psImageInstance->s32XPos);
			}
			else
			{
				psImageInstance->pu8SrcPointer += (-psImageInstance->s32XPos);
			}


			if (psImageInstance->pu8Transparent)
			{
				psImageInstance->pu8Transparent += (-psImageInstance->s32XPos);
			}

			if (psImageInstance->pu8TranslucentMask)
			{
				psImageInstance->pu8TranslucentMask += (-psImageInstance->s32XPos);
			}

			psImageInstance->u32XPos = 0;
		}
	}
	else
	{
		psImageInstance->u32XPos = (UINT32) psImageInstance->s32XPos;
	}

	if (psImageInstance->s32YPos < 0)
	{
		if ((-psImageInstance->s32YPos) >= (INT32) psImageInstance->psImage->u32YSize)
		{
			psImageInstance->u32YSizeClipped = 0;
		}
		else
		{
			psImageInstance->u32YSizeClipped = (UINT32) (((INT32) psImageInstance->psImage->u32YSize) + psImageInstance->s32YPos);

			if (psImageInstance->pu16SrcPointer)
			{
				psImageInstance->pu16SrcPointer += ((-psImageInstance->s32YPos) * psImageInstance->psImage->u32Pitch);
			}
			else
			{
				psImageInstance->pu8SrcPointer += ((-psImageInstance->s32YPos) * psImageInstance->psImage->u32Pitch);
			}
	
			if (psImageInstance->pu8Transparent)
			{
				psImageInstance->pu8Transparent += ((-psImageInstance->s32YPos) * psImageInstance->psImage->u32Pitch);
			}

			if (psImageInstance->pu8TranslucentMask)
			{
				psImageInstance->pu8TranslucentMask += ((-psImageInstance->s32YPos) * psImageInstance->psImage->u32Pitch);
			}

			psImageInstance->u32YPos = 0;
		}
	}
	else
	{
		psImageInstance->u32YPos = (UINT32) psImageInstance->s32YPos;
	}

	// Clip the instance if it goes beyond the surface

	if ((psImageInstance->u32XPos + psImageInstance->u32XSizeClipped) > sg_u32SurfaceXSize)
	{
		psImageInstance->u32XSizeClipped = sg_u32SurfaceXSize - psImageInstance->u32XPos;
	}

	if ((psImageInstance->u32YPos + psImageInstance->u32YSizeClipped) > sg_u32SurfaceYSize)
	{
		psImageInstance->u32YSizeClipped = sg_u32SurfaceYSize - psImageInstance->u32YPos;
	}

	// If our clipped size is too big, zero it

	if (psImageInstance->u32XSizeClipped > psImageInstance->psImage->u32XSize)
	{
		psImageInstance->u32XSizeClipped = 0;
	}

	if (psImageInstance->u32YSizeClipped > psImageInstance->psImage->u32YSize)
	{
		psImageInstance->u32YSizeClipped = 0;
	}

	// Update the destination surface pointer

	psImageInstance->pu16DestSurfacePointer = sg_pu16SurfaceLayer;
	psImageInstance->pu16DestSurfacePointer += (psImageInstance->u32XPos + (psImageInstance->u32YPos * sg_u32SurfaceLayerPitch));

	// Now update it, but only if the image is visible
	
	if (psImageInstance->bImageVisible)
	{
		GfxUpdateImage(psImageInstance, FALSE);
	}
}

void GfxSetImageInstance(SImageInstance *psImageInstance,
						 INT32 s32XPos,
						 INT32 s32YPos,
						 BOOL bVisible)
{
	GfxSetImageInstanceInternal(psImageInstance,
								s32XPos,
								s32YPos,
								bVisible,
								FALSE);
}

void GfxSetNewImage(SImageInstance *psInstance,
					SImage *psImage)
{
	INT32 s32XPos;
	INT32 s32YPos;

	GCASSERT(psImage);

	psInstance->psImage = psImage;
	psInstance->pu16DestSurfacePointer = NULL;

	psInstance->u32XSizeClipped = psImage->u32XSize;
	psInstance->u32YSizeClipped = psImage->u32YSize;
	psInstance->pu16SrcPointer = psImage->pu16ImageData;
	psInstance->pu8SrcPointer = psImage->pu8ImageData;
	psInstance->pu8TranslucentMask = psImage->pu8TranslucentMask;
	psInstance->pu8Transparent = psImage->pu8Transparent;

	s32XPos = psInstance->s32XPos;
	s32YPos = psInstance->s32YPos;

	// If the image has negative coordinates, it needs to get clipped

	if (s32XPos < 0)
	{
		if ((-psInstance->s32XPos) >= (INT32) psInstance->psImage->u32XSize)
		{
			psInstance->u32XSizeClipped = 0;
		}
		else
		{
			psInstance->u32XSizeClipped = (UINT32) (((INT32) psInstance->psImage->u32XSize) + psInstance->s32XPos);

			if (psInstance->pu16SrcPointer)
			{
				psInstance->pu16SrcPointer += (-psInstance->s32XPos);
			}
			else
			{
				psInstance->pu8SrcPointer += (-psInstance->s32XPos);
			}

			if (psInstance->pu8TranslucentMask)
			{
				psInstance->pu8TranslucentMask += (-psInstance->s32XPos);
			}

			if (psInstance->pu8Transparent)
			{
				psInstance->pu8Transparent += (-psInstance->s32YPos);
			}

			psInstance->u32XPos = 0;
		}
	}
	else
	{
		psInstance->u32XPos = (UINT32) s32XPos;
	}

	if (s32YPos < 0)
	{
		if ((-psInstance->s32YPos) >= (INT32) psInstance->psImage->u32YSize)
		{
			psInstance->u32YSizeClipped = 0;
		}
		else
		{
			psInstance->u32YSizeClipped = (UINT32) (((INT32) psInstance->psImage->u32YSize) + psInstance->s32YPos);

			if (psInstance->pu16SrcPointer)
			{
				psInstance->pu16SrcPointer += ((-psInstance->s32YPos) * psInstance->psImage->u32Pitch);
			}
			else
			{
				psInstance->pu8SrcPointer += ((-psInstance->s32YPos) * psInstance->psImage->u32Pitch);
			}

			if (psInstance->pu8TranslucentMask)
			{
				psInstance->pu8TranslucentMask += ((-psInstance->s32YPos) * psInstance->psImage->u32Pitch);
			}

			if (psInstance->pu8Transparent)
			{
				psInstance->pu8Transparent += ((-psInstance->s32YPos) * psInstance->psImage->u32Pitch);
			}
			psInstance->u32YPos = 0;
		}
	}
	else
	{
		psInstance->u32YPos = (UINT32) s32YPos;
	}

	// Clip the instance if it goes beyond the surface

	if ((psInstance->u32XPos + psInstance->u32XSizeClipped) > sg_u32SurfaceXSize)
	{
		psInstance->u32XSizeClipped = sg_u32SurfaceXSize - psInstance->u32XPos;
	}

	if ((psInstance->u32YPos + psInstance->u32YSizeClipped) > sg_u32SurfaceYSize)
	{
		psInstance->u32YSizeClipped = sg_u32SurfaceYSize - psInstance->u32YPos;
	}

	// If our clipped size is too big, zero it

	if (psInstance->u32XSizeClipped > psInstance->psImage->u32XSize)
	{
		psInstance->u32XSizeClipped = 0;
	}

	if (psInstance->u32YSizeClipped > psInstance->psImage->u32YSize)
	{
		psInstance->u32YSizeClipped = 0;
	}
}

void GfxSetTransparencyKeys(UINT16 *pu16TransparencyColor,
						    UINT32 u32TransparencyColorCount,
						    SImage *psImage)
{
	UINT16 *pu16ImageData;
	UINT8 *pu8TransparencyMask;
	UINT32 u32TotalPixels;
	UINT8 u8Mask;
	UINT32 u32Loop;

	if (psImage->pu8Transparent)
	{
		GCFreeMemory(psImage->pu8Transparent);
	}

	if (NULL == pu16TransparencyColor)
	{
		psImage->pu8Transparent = NULL;
		return;
	}

	u32TotalPixels = psImage->u32XSize * psImage->u32YSize;
	pu16ImageData = psImage->pu16ImageData;
	psImage->pu8Transparent = MemAllocNoClear(u32TotalPixels * sizeof(*psImage->pu8Transparent));
	pu8TransparencyMask = psImage->pu8Transparent;

	while (u32TotalPixels)
	{
		u8Mask = 0xff;

		for (u32Loop = 0; u32Loop < u32TransparencyColorCount; u32Loop++)
		{
			if (pu16TransparencyColor[u32Loop] == *pu16ImageData)
			{
				u8Mask = 0;
				break;
			}
		}
	
		*pu8TransparencyMask = u8Mask;
		++pu16ImageData;
		++pu8TransparencyMask;
		u32TotalPixels--;
	}
}

SImageInstance *GfxCreateImageInstanceInternal(SLayer *psLayer,
											   SImage *psImage,
											   SImageInstance *psImageInstanceIncoming,
											   INT32 s32XPos,
											   INT32 s32YPos,
											   BOOL bImageVisible,
											   SCLUT *psCLUT)
{
	SImageInstance *psInstance;
	SImageInstance *psInstancePtr;

	if (NULL == psImage)
	{
		GCASSERT(psImage);
	}

	if (NULL == psImageInstanceIncoming)
	{
		psInstance = MemAlloc(sizeof(*psInstance));
		if (NULL == psInstance)
		{
			return(NULL);
		}
	}
	else
	{
		psInstance = psImageInstanceIncoming;
	}

	GCASSERT(psInstance);

	psInstance->psImage = psImage;
	psInstance->bImageVisible = bImageVisible;
	psInstance->s32XPos = s32XPos;
	psInstance->s32YPos = s32YPos;
	psInstance->u32XOffset = 0;
	psInstance->u32YOffset = 0;
	psInstance->psNextLink = NULL;
	psInstance->psParentLayer = psLayer;
	psInstance->pu16DestSurfacePointer = NULL;
	psInstance->u8Intensity = 0xff;

	psInstance->u32XSizeClipped = psImage->u32XSize;
	psInstance->u32YSizeClipped = psImage->u32YSize;
	psInstance->pu16SrcPointer = psImage->pu16ImageData;
	psInstance->pu8SrcPointer = psImage->pu8ImageData;
	psInstance->pu8TranslucentMask = psImage->pu8TranslucentMask;
	psInstance->pu8Transparent = psImage->pu8Transparent;
	psInstance->psCLUT = psCLUT;

	// If the image has negative coordinates, it needs to get clipped

	if (s32XPos < 0)
	{
		if ((-psInstance->s32XPos) >= (INT32) psInstance->psImage->u32XSize)
		{
			psInstance->u32XSizeClipped = 0;
		}
		else
		{
			psInstance->u32XSizeClipped = (UINT32) (((INT32) psInstance->psImage->u32XSize) + psInstance->s32XPos);

			if (psInstance->pu16SrcPointer)
			{
				psInstance->pu16SrcPointer += (-psInstance->s32XPos);
			}
			else
			{
				psInstance->pu8SrcPointer += (-psInstance->s32XPos);
			}

			if (psInstance->pu8TranslucentMask)
			{
				psInstance->pu8TranslucentMask += (-psInstance->s32XPos);
			}

			if (psInstance->pu8Transparent)
			{
				psInstance->pu8Transparent += (-psInstance->s32YPos);
			}

			psInstance->u32XPos = 0;
		}
	}
	else
	{
		psInstance->u32XPos = (UINT32) s32XPos;
	}

	if (s32YPos < 0)
	{
		if ((-psInstance->s32YPos) >= (INT32) psInstance->psImage->u32YSize)
		{
			psInstance->u32YSizeClipped = 0;
		}
		else
		{
			psInstance->u32YSizeClipped = (UINT32) (((INT32) psInstance->psImage->u32YSize) + psInstance->s32YPos);

			if (psInstance->pu16SrcPointer)
			{
				psInstance->pu16SrcPointer += ((-psInstance->s32YPos) * psInstance->psImage->u32Pitch);
			}
			else
			{
				psInstance->pu8SrcPointer += ((-psInstance->s32YPos) * psInstance->psImage->u32Pitch);
			}

			if (psInstance->pu8TranslucentMask)
			{
				psInstance->pu8TranslucentMask += ((-psInstance->s32YPos) * psInstance->psImage->u32Pitch);
			}

			if (psInstance->pu8Transparent)
			{
				psInstance->pu8Transparent += ((-psInstance->s32YPos) * psInstance->psImage->u32Pitch);
			}
			psInstance->u32YPos = 0;
		}
	}
	else
	{
		psInstance->u32YPos = (UINT32) s32YPos;
	}

	// Clip the instance if it goes beyond the surface

	if ((psInstance->u32XPos + psInstance->u32XSizeClipped) > sg_u32SurfaceXSize)
	{
		psInstance->u32XSizeClipped = sg_u32SurfaceXSize - psInstance->u32XPos;
	}

	if ((psInstance->u32YPos + psInstance->u32YSizeClipped) > sg_u32SurfaceYSize)
	{
		psInstance->u32YSizeClipped = sg_u32SurfaceYSize - psInstance->u32YPos;
	}

	// If our clipped size is too big, zero it

	if (psInstance->u32XSizeClipped > psInstance->psImage->u32XSize)
	{
		psInstance->u32XSizeClipped = 0;
	}

	if (psInstance->u32YSizeClipped > psInstance->psImage->u32YSize)
	{
		psInstance->u32YSizeClipped = 0;
	}

	psInstance->pu16DestSurfacePointer = sg_pu16SurfaceLayer;
	psInstance->pu16DestSurfacePointer += (psInstance->u32XPos + (psInstance->u32YPos * sg_u32SurfaceLayerPitch));

	// Now add this instance to the end of the layer it's assigned

	if (psLayer && (NULL == psImageInstanceIncoming))
	{
		psInstancePtr = psLayer->psImages;

		while (psInstancePtr && psInstancePtr->psNextLink)
		{
			psInstancePtr = psInstancePtr->psNextLink;
		}

		if (psInstancePtr)
		{
			// Attach it to the end

			psInstancePtr->psNextLink = psInstance;
		}
		else
		{
			// Attach it to the head (first in the list)

			psLayer->psImages = psInstance;
		}
	}

	if (bImageVisible)
	{
		GfxRecalcIntersections();
	}

	// Now go update the image on the surface (if it's displayed)

	if (psInstance->psCLUT)
	{
		GfxAssignCLUT(psInstance->psCLUT,
					  psInstance);
	}
	else
	if ((psInstance->bImageVisible) &&
		(NULL == psInstance->psCLUT))
	{
		GfxUpdateImage(psInstance,
					   FALSE);
	}

	return(psInstance);
}

void GfxSetImageInstanceViewport(SImageInstance *psInstance,
								UINT32 u32XOffset,
								UINT32 u32YOffset,
								UINT32 u32XSize,
								UINT32 u32YSize)
{
	GCASSERT(psInstance);

	psInstance->u32XOffset = u32XOffset;
	psInstance->u32YOffset = u32YOffset;

	// Cap the offset at the size of the image
	if( psInstance->u32XOffset >= psInstance->psImage->u32XSize )
	{
		psInstance->u32XOffset = psInstance->psImage->u32XSize;
	}

	if( psInstance->u32YOffset >= psInstance->psImage->u32YSize )
	{
		psInstance->u32YOffset = psInstance->psImage->u32YSize;
	}

	psInstance->u32XSizeClipped = u32XSize;
	psInstance->u32YSizeClipped = u32YSize;

	// Cap the size
	if( (psInstance->u32XSizeClipped + psInstance->u32XOffset) >= psInstance->psImage->u32XSize )
	{
		psInstance->u32XSizeClipped = psInstance->psImage->u32XSize - psInstance->u32XOffset;
	}

	if( (psInstance->u32YSizeClipped + psInstance->u32YOffset) >= psInstance->psImage->u32YSize )
	{
		psInstance->u32YSizeClipped = psInstance->psImage->u32YSize - psInstance->u32YOffset;
	}
}

SImageInstance *GfxCreateImageInstance(SLayer *psLayer,
									   SImage *psImage,
									   INT32 s32XPos,
									   INT32 s32YPos,
									   BOOL bImageVisible)
{
	return(GfxCreateImageInstanceInternal(psLayer,
										  psImage,
										  NULL,
										  s32XPos,
										  s32YPos,
										  bImageVisible,
										  NULL));
}

SImageInstance *GfxCreateImageInstanceWithCLUT(SLayer *psLayer,
											   SImage *psImage,
											   INT32 s32XPos,
											   INT32 s32YPos,
											   BOOL bImageVisible,
											   SCLUT *psCLUT)
{
	return(GfxCreateImageInstanceInternal(psLayer,
										  psImage,
										  NULL,
										  s32XPos,
										  s32YPos,
										  bImageVisible,
										  psCLUT));
}

void GfxDeleteImageInstance(SImageInstance *psInstance)
{
	SLayer *psLayerPtr = sg_psLayerHead;
	SImageInstance *psImagePtr = NULL;
	SImageInstance *psImagePtrPrior = NULL;

	// Set the image to nonvisible, then recalc the intersections
	if (psInstance->bImageVisible)
	{
		psInstance->bImageVisible = FALSE;
		GfxRecalcIntersections();
		GCASSERT(NULL == psInstance->psZBufferBehind);
		GCASSERT(NULL == psInstance->psZBufferFront);
	}

	// Let's run through the entire sprite system and kill the instance out of the
	// list if we find it.

	while (psLayerPtr)
	{
		psImagePtr = psLayerPtr->psImages;
		psImagePtrPrior = NULL;

		if (psInstance->psParentLayer == psLayerPtr)
		{
			// Only bother looking if it's attached to this layer

			while (psImagePtr)
			{
				if (psImagePtr == psInstance)
				{
					// Found it! Let's remove it from the linked list, but only if it's the final reference

					// If this has a CLUT, let's deassign it

					GfxUnassignCLUT(psInstance,
									FALSE);

					if (NULL == psImagePtrPrior)
					{
						psLayerPtr->psImages = psImagePtr->psNextLink;
					}
					else
					{
						psImagePtrPrior->psNextLink = psImagePtr->psNextLink;
					}

					goto imagePointerRemoved;
				}
				else
				{
					psImagePtrPrior = psImagePtr;
					psImagePtr = psImagePtr->psNextLink;
				}
			}
		}

		psLayerPtr = psLayerPtr->psNextLayer;
	}

	// Let's erase the instance from the screen

imagePointerRemoved:
	if (psInstance->bImageVisible)
	{
		GfxUpdateImage(psInstance, TRUE);
	}

	// Now delete the node from the heap
	GCFreeMemory(psInstance);
}

void GfxReplaceInstanceImage(SImageInstance *psImageInstance,
							 SImage *psNewImage)

{
	GCASSERT(psImageInstance);
	GCASSERT(psNewImage);

	// Erase the old image
	GfxUpdateImage(psImageInstance,
				   TRUE);

	// Let's attach the new image
	psImageInstance->psImage = psNewImage;

	// Update all of the internal pointers and redraw if necessary
	GfxCreateImageInstanceInternal(psImageInstance->psParentLayer,
								   psNewImage,
								   psImageInstance,
								   psImageInstance->s32XPos,
								   psImageInstance->s32YPos,
								   psImageInstance->bImageVisible,
								   psImageInstance->psCLUT);
}

void GfxBlit(BOOL bBlitAll)
{
	UINT32 u32DisplayPitch;
	UINT16 *pu16ScreenBuffer;
	UINT16 *pu16SurfaceBuffer = sg_pu16SurfaceLayer;
	UINT8 *pu8DirtyBuffer = sg_pu8DirtyBuffer;
//	UINT32 u32Y = 0;
//	UINT32 u32X = 0;
	UINT32 u32YSize = 0;
	UINT32 u32XSize = 0;

	// For now, just blit everything

	GCDisplayGetDisplayBuffer((void **) &pu16ScreenBuffer);
	GCDisplayGetDisplayPitch((UINT32 *) &u32DisplayPitch);

	// Copy everything - double size for Windows

/* 	memcpy((void *) pu16ScreenBuffer, pu16SurfaceBuffer, (sg_u32SurfaceXSize << 1) * sg_u32SurfaceYSize);
	return; */

/*
#ifdef _WIN32
	for (u32Y = 0; u32Y < sg_u32SurfaceYSize; u32Y++)
	{
		for (u32X = 0; u32X < sg_u32SurfaceXSize; u32X++)
		{
			*pu16ScreenBuffer = *pu16SurfaceBuffer;
			++pu16ScreenBuffer;
			*pu16ScreenBuffer = *pu16SurfaceBuffer;
			++pu16ScreenBuffer;
			++pu16SurfaceBuffer;
		}
		
		memcpy((void *) pu16ScreenBuffer, (void *) (pu16ScreenBuffer - (sg_u32SurfaceXSize << 1)), sg_u32SurfaceXSize << 2);
		pu16ScreenBuffer += (sg_u32SurfaceXSize << 1);
	}
#else	// Normal size for GC

*/
	// pu16ScreenBuffer is the screen buffer itself
	// sg_pu16SurfaceLayer is the backbuffer

	u32YSize = sg_u32SurfaceYSize / DIRTY_Y_BLOCK_SIZE;

	while (u32YSize)
	{
		u32XSize = sg_u32SurfaceXSize / DIRTY_X_BLOCK_SIZE;
		while (u32XSize)
		{
			if (*pu8DirtyBuffer || bBlitAll)
			{
				UINT16 *pu16ScreenTemp = pu16ScreenBuffer;
				UINT16 *pu16SurfaceTemp = pu16SurfaceBuffer;
				UINT32 u32Run = 0;
				UINT32 u32Loop = 0;

				// Find out how big our run is
				while ((*pu8DirtyBuffer || bBlitAll) && u32XSize)
				{
					*pu8DirtyBuffer = 0;		// Clear the dirty run
					++pu8DirtyBuffer;
					--u32XSize;
					u32Run += (DIRTY_X_BLOCK_SIZE << 1);
				}

				// Now go blit the dirty tile

				for (u32Loop = 0; u32Loop < DIRTY_Y_BLOCK_SIZE; u32Loop++)
				{
					memcpy(pu16ScreenTemp, pu16SurfaceTemp, (size_t) (u32Run));
					pu16ScreenTemp += u32DisplayPitch;
					pu16SurfaceTemp += sg_u32SurfaceLayerPitch;
				}

				u32Run >>= 1;
				pu16SurfaceBuffer += u32Run;
				pu16ScreenBuffer += u32Run;

			}
			else
			{
				pu16SurfaceBuffer += DIRTY_X_BLOCK_SIZE;
				pu16ScreenBuffer += DIRTY_X_BLOCK_SIZE;
				++pu8DirtyBuffer;
				--u32XSize;
			}
		}
		// Next Y coordinate stripe
		pu16ScreenBuffer += (u32DisplayPitch * (DIRTY_Y_BLOCK_SIZE  - 1));
		pu16SurfaceBuffer += (sg_u32SurfaceLayerPitch * (DIRTY_Y_BLOCK_SIZE - 1));
		--u32YSize;
	}

// #endif
}

void GfxViewportCreate(SImageInstance *psImageInstance,
					   UINT32 u32XViewportSize,
					   UINT32 u32YViewportSize)
{
	SImage *psImage;
	UINT32 u32XFinalSize;
	UINT32 u32YFinalSize;
	UINT16 *pu16NewImage = NULL;
	UINT8 *pu8NewTranslucentMask = NULL;
	UINT8 *pu8NewTransparent = NULL;
	UINT32 u32XLoc = 0;
	UINT32 u32YLoc = 0;
	UINT32 u32Y = 0;
	UINT32 u32X = 0;

	psImage = psImageInstance->psImage;
	GCASSERT(psImage);

	psImage->u32XSizeOriginal = psImage->u32XSize;
	psImage->u32YSizeOriginal = psImage->u32YSize;

	// Figure out how big our x and y are

	u32YFinalSize = u32YViewportSize + psImage->u32YSizeOriginal - 1;
	u32XFinalSize = u32XViewportSize + psImage->u32XSizeOriginal - 1;

	// Allocate memory for the new image

	pu16NewImage = MemAllocNoClear(u32XFinalSize * u32YFinalSize * sizeof(*pu16NewImage));
	GCASSERT(pu16NewImage);

	// And let's copy the transparency masks as well if necessary

	if (psImage->pu8Transparent)
	{
		pu8NewTransparent = MemAllocNoClear(u32XFinalSize * u32YFinalSize * sizeof(*pu8NewTransparent));
	}

	// And the translucency mask

	if (psImage->pu8TranslucentMask)
	{
		pu8NewTranslucentMask = MemAllocNoClear(u32XFinalSize * u32YFinalSize * sizeof(*pu8NewTranslucentMask));
	}

	// Let's copy from the old image to the new image space, and wrap if we go beyond the
	// edges

	for (u32Y = 0; u32Y < u32YFinalSize; u32Y++)
	{
		for (u32X = 0; u32X < u32XFinalSize; u32X++)
		{
			// If anything is wrapping, wrap it

			u32XLoc = u32X;

			while (u32XLoc >= psImage->u32XSizeOriginal)
			{
				u32XLoc -= psImage->u32XSizeOriginal;
			}

			u32YLoc = u32Y;

			while (u32YLoc >= psImage->u32YSizeOriginal)
			{
				u32YLoc -= psImage->u32YSizeOriginal;
			}

			// Now let's poke the data from the source to the destination address

//			pu16NewImage[(u32Y * u32XFinalSize) + u32X] = psImage->pu16ImageData[(u32YLoc * psImage->u32Pitch) + u32XLoc];

			// If we've got a transparency mask, let's go for it

			if (pu8NewTransparent)
			{
				pu8NewTransparent[(u32Y * u32XFinalSize) + u32X] = psImage->pu8Transparent[(u32YLoc * psImage->u32Pitch) + u32XLoc];
			}

			// Same deal with translucency

			if (pu8NewTranslucentMask)
			{
				pu8NewTranslucentMask[(u32Y * u32XFinalSize) + u32X] = psImage->pu8TranslucentMask[(u32YLoc * psImage->u32Pitch) + u32XLoc];
			}
		}
	}

	// Save off all of the original info

	psImage->pu8TranslucentMaskOriginal = psImage->pu8TranslucentMask;
	psImage->pu8TransparentOriginal = psImage->pu8TransparentOriginal;

	// Kill the old image

	GCFreeMemory(psImage->pu16ImageData);

	// Now hook up the new/improved image

	psImage->pu16ImageData = pu16NewImage;
	psImage->pu16OriginalBase = psImage->pu16ImageData;
	psImage->pu8Transparent = pu8NewTransparent;
	psImage->pu8TranslucentMask = pu8NewTranslucentMask;
	psImage->u32Pitch = u32XFinalSize;
	psImage->u32XSize = u32XViewportSize;
	psImage->u32YSize = u32YViewportSize;

	// Now go redraw it

//	GfxUpdateImage(psImageInstance, FALSE);
}

void GfxViewportSet(SImageInstance *psImageInstance,
					UINT32 u32XViewport,
					UINT32 u32YViewport)
{
	GCASSERT(psImageInstance->psImage->pu16OriginalBase);

	// Figure out where our viewport should go

	psImageInstance->psImage->pu16ImageData = psImageInstance->psImage->pu16OriginalBase + (u32YViewport * psImageInstance->psImage->u32Pitch) + u32XViewport;

	GfxUpdateImage(psImageInstance, FALSE);
}

static UINT8 sg_u8BPP;

void GfxGetCurrentSurface(UINT32 *pu32XSize,
						  UINT32 *pu32YSize,
						  UINT8 *pu8BPP)
{
	*pu32XSize = sg_u32SurfaceXSize;
	*pu32YSize = sg_u32SurfaceYSize;
	*pu8BPP = sg_u8BPP;
}

void GfxDeleteImageGroup(SImageGroup *psImageGroup)
{
	SImageGroupLink *psLink = NULL;
	SImageGroupLink *psLinkPrior = NULL;

	if( NULL == psImageGroup )
	{
		return;
	}

	GfxLockFileList();

	if (psImageGroup->psGfxFile)
	{
		struct SGfxFile* psGfxFile = NULL;

		GfxDecRef(psImageGroup);

		if( psImageGroup->psGfxFile->u32References )
		{
			GfxUnlockFileList();

			return;
		}

		// Unlink the GfxFile from the linked list
		psGfxFile = psImageGroup->psGfxFile;

		if( sg_psFileHead == psGfxFile )
		{
			sg_psFileHead = psGfxFile->psNextLink;
		}

		if( psGfxFile->psNextLink )
		{
			psGfxFile->psNextLink->psPrevLink = psGfxFile->psPrevLink;
		}

		if( psGfxFile->psPrevLink )
		{
			psGfxFile->psPrevLink->psNextLink = psGfxFile->psNextLink;
		}

		// Free the related data
		if( psGfxFile->peGraphicsFilename )
		{
			MemFree(psGfxFile->peGraphicsFilename);
			psGfxFile->peGraphicsFilename = NULL;
		}

		MemFree(psGfxFile);
		psImageGroup->psGfxFile = NULL;
	}

	// Ditch any and all frames in the image group

	psLink = psImageGroup->psLinkHead;
	while (psLink)
	{
		GfxDeleteImage(psLink->psImage);

		psLink->psImage = NULL;

		psLinkPrior = psLink;
		psLink = psLink->psNextLink;
		GCFreeMemory(psLinkPrior);
	}

	psImageGroup->psLinkHead = NULL;
	psImageGroup->psLinkTail = NULL;

	psImageGroup->psCurrentImage = NULL;
	GCFreeMemory(psImageGroup);

	GfxUnlockFileList();
}

void GfxDeleteImage(SImage *psImage)
{
	GfxDeleteImageInternal(psImage);
}

void GfxSurfaceDelete(void)
{
	// Deallocate anything priorly allocated

	if (sg_pu16SurfaceLayer)
	{
		GCFreeMemory(sg_pu16SurfaceLayer);
		sg_pu16SurfaceLayer = NULL;
		sg_u32SurfaceLayerPitch = 0;
	}

	if (sg_pu8DirtyBuffer)
	{
		GCFreeMemory(sg_pu8DirtyBuffer);
		sg_pu8DirtyBuffer = NULL;
		sg_u32DirtyBufferPitch = 0;
	}

	sg_u32SurfaceXSize = 0;
	sg_u32SurfaceYSize = 0;
}

void GfxSurfaceCreate(UINT32 u32XSize,
					  UINT32 u32YSize,
					  UINT8 u8BPP)
{
	// Only 16bpp supported at this time

	GCASSERT(16 == u8BPP);

	GfxSurfaceDelete();

	// Allocate a surface layer

	sg_pu16SurfaceLayer = MemAlloc(u32XSize * u32YSize * sizeof(*sg_pu16SurfaceLayer));
	GCASSERT(sg_pu16SurfaceLayer);
	sg_u32SurfaceLayerPitch = u32XSize;

	// Allocate a dirty buffer layer

	sg_u32DirtyBufferPitch = ((u32XSize + (DIRTY_X_BLOCK_SIZE - 1)) / DIRTY_X_BLOCK_SIZE);
	sg_u32DirtyBufferSize = ((u32YSize + (DIRTY_Y_BLOCK_SIZE - 1)) / DIRTY_Y_BLOCK_SIZE) * sg_u32DirtyBufferPitch;
	sg_pu8DirtyBuffer = MemAllocNoClear(sg_u32DirtyBufferSize);
	GCASSERT(sg_pu8DirtyBuffer);

	// Start everything off dirty so it gets blit

	memset((void *) sg_pu8DirtyBuffer, 0xff, (size_t) (sg_u32DirtyBufferSize));

	// Set up things

	sg_u32SurfaceXSize = u32XSize;
	sg_u32SurfaceYSize = u32YSize;
	sg_u8BPP = u8BPP;

	GfxRefresh();
	GfxBlit(FALSE);
}

void GfxDeleteLayer(SLayer *psLayer)
{
	SLayer *psLayerPtr = NULL;
	SLayer *psLayerPrior = NULL;
	SImageInstance *psInstance = NULL;
	SImageInstance *psInstanceKill = NULL;

	GCASSERT(psLayer);

//	DebugOut("%s: Deleting layer 0x%.8x\n", __FUNCTION__, (UINT32) psLayer);

	psLayerPtr = sg_psLayerHead;
	psLayerPrior = NULL;

	while (psLayerPtr && psLayerPtr != psLayer)
	{
		psLayerPrior = psLayerPtr;
		psLayerPtr = psLayerPtr->psNextLayer;
	}

	// If this asserts, we couldn't find the layer to delete
	GCASSERT(psLayerPtr);

	psInstance = psLayerPtr->psImages;

	while (psInstance)
	{
		psInstanceKill = psInstance;
		psInstance = psInstance->psNextLink;
		psInstanceKill->psNextLink = NULL;
		GfxDeleteImageInstance(psInstanceKill);
	}

	if (NULL == psLayerPrior)
	{
		// Head of the list.
		sg_psLayerHead = sg_psLayerHead->psNextLayer;
		if (sg_psLayerHead)
		{
			sg_psLayerHead->psPriorLayer = NULL;
		}
	}
	else
	{
		psLayerPrior->psNextLayer = psLayer->psNextLayer;
		if (psLayer->psNextLayer)
		{
			psLayer->psNextLayer = psLayerPrior;
		}
	}

	GCFreeMemory(psLayer);
	--sg_u32LayerCount;
}

SImage *GfxCreateImageFromScreenshot(void)
{
	SImage *psImage = NULL;
	UINT32 u32CurrentX;
	UINT32 u32CurrentY;
	EGCResultCode eResult;
	UINT16 *pu16SurfaceData;

	eResult = GCDisplayGetXSize(&u32CurrentX);
	GCASSERT(GC_OK == eResult);
	eResult = GCDisplayGetYSize(&u32CurrentY);
	GCASSERT(GC_OK == eResult);

	psImage = GfxCreateEmptyImage(u32CurrentX,
								  u32CurrentY,
								  sg_u8BPP,
								  0,
								  FALSE);
	GCASSERT(psImage);

	// Now a block copy of the surface data

	GCDisplayGetDisplayBuffer((void **) &pu16SurfaceData);

	memcpy((void *) psImage->pu16ImageData, pu16SurfaceData, u32CurrentX * u32CurrentY * 2);
	return(psImage);
}

SCLUT *GfxCreateCLUT(UINT8 u8BPP)
{
	SCLUT *psNewCLUT = NULL;

	// Only 16BPP CLUTs supported for the time being

	if (16 == u8BPP)
	{
		psNewCLUT = MemAlloc(sizeof(*psNewCLUT));
		GCASSERT(psNewCLUT);
		memset((void *) psNewCLUT, 0, sizeof(*psNewCLUT));
		psNewCLUT->pvCLUT = MemAlloc(((1 << u8BPP) * sizeof(UINT16)));
		GCASSERT(psNewCLUT->pvCLUT);
	}

	return(psNewCLUT);
}

void GfxDeleteCLUT(SCLUT *psCLUT)
{
	SCLUT *psCLUTPrior = NULL;
	SImageInstances *psInstances = NULL;
	SImageInstances *psInstancesPrior = NULL;

	psInstances = psCLUT->psImageInstances;

	// Run through all image instances that contain this CLUT and clear it/redraw it

	while (psInstances)
	{
		// No more color table
		psInstances->psImageInstance->psCLUT = NULL;

		// Repaint/redraw it now that we don't have a CLUT
		GfxUpdateImage(psInstances->psImageInstance,
					   FALSE);
		psInstancesPrior = psInstances;
		psInstances = psInstances->psNextLink;
		GCFreeMemory(psInstancesPrior);
	}

	// Delete the CLUT itself

	GCFreeMemory(psCLUT->pvCLUT);

	psCLUTPrior = psCLUT;
	psCLUT = psCLUT->psNextLink;
	GCFreeMemory(psCLUTPrior);
}

void GfxUnassignCLUT(SImageInstance *psImageInstance,
					 BOOL bUpdate)
{
	SImageInstances *psInstances;
	SImageInstances *psInstancesPrior = NULL;
	SCLUT *psCLUT;

	if (psImageInstance->psCLUT)
	{
		psCLUT = psImageInstance->psCLUT;

		// Take it out of the CLUT list of images.

		psInstances = psCLUT->psImageInstances;
		psInstancesPrior = NULL;

		// Look for the pointer in our list and remove it

		while (psInstances && psInstances->psImageInstance != psImageInstance)
		{
			psInstancesPrior = psInstances;
			psInstances = psInstances->psNextLink;
		}

		if (psInstances)
		{
			// Only delete it if we found it

			// Correct the forward pointer

			if (psInstancesPrior)
			{
				psInstancesPrior->psNextLink = psInstances->psNextLink;
			}
			else
			{
				psCLUT->psImageInstances = psInstances->psNextLink;
			}

			// Now correct the backward pointer

			if (psInstances->psNextLink)
			{
				psInstances->psNextLink->psPriorLink = psInstancesPrior;
			}

			// Unassign it!
			psInstances->psImageInstance->psCLUT = NULL;

			if (bUpdate)
			{
				GfxUpdateImage(psInstances->psImageInstance,
							   FALSE);
			}

			// Delete the image instance
			GCFreeMemory(psInstances);

		}
	}
}

void GfxAssignCLUT(SCLUT *psCLUT,
				   SImageInstance *psImageInstance)
{
	SImageInstances *psInstances = NULL;

	if (psCLUT)
	{
		// Add this image instance to the CLUT

		psInstances = MemAlloc(sizeof(*psInstances));
		GCASSERT(psInstances);
		psInstances->psImageInstance = psImageInstance;

		if (psCLUT->psImageInstances)
		{
			psCLUT->psImageInstances->psPriorLink = psInstances;
		}

		// Hook it in to the beginning of the list

		psInstances->psNextLink = psCLUT->psImageInstances;
		psCLUT->psImageInstances = psInstances;

		// Now hook in the CLUT
		psImageInstance->psCLUT = psCLUT;

		// Now update the image

		GfxUpdateImage(psImageInstance,
					   FALSE);
	}
}

void GfxSetCLUTFade(SCLUT *psCLUT,
					UINT8 u8Intensity)
{
	UINT32 u32RedCounter = 0;
	UINT32 u32GreenCounter = 0;
	UINT32 u32BlueCounter = 0;
	UINT32 u32RedStep = 0;
	UINT32 u32GreenStep = 0;
	UINT32 u32BlueStep = 0;
	UINT32 u32Red;
	UINT32 u32Green;
	UINT32 u32Blue;
	UINT16 *pu16Color = (UINT16 *) psCLUT->pvCLUT;
	SImageInstances *psInstances = NULL;

	GCASSERT(psCLUT);

	u32RedStep = ((u8Intensity * 0x20) << 16) / 0x20;
	u32GreenStep = ((u8Intensity * 0x40) << 16) / 0x40;
	u32BlueStep = ((u8Intensity * 0x20) << 16) / 0x20;

	// 0000 0000 0000 0000 rrrr rggg gggb bbbb

	for (u32Red = 0; u32Red < 0x20; u32Red++)
	{
		u32GreenCounter = 0;
		for (u32Green = 0; u32Green < 0x40; u32Green++)
		{
			u32BlueCounter = 0;
			for (u32Blue = 0; u32Blue < 0x20; u32Blue++)
			{
				*pu16Color = (u32BlueCounter >> 24) |
							 ((u32GreenCounter >> 19) & 0x07e0) |
							 ((u32RedCounter >> 13) & 0xf800);
				++pu16Color;
				u32BlueCounter += u32BlueStep;
			}
			u32GreenCounter += u32GreenStep;
		}
		u32RedCounter += u32RedStep;
	}

	// Run through all CLUT affected images and update them

	psInstances = psCLUT->psImageInstances;

	while (psInstances)
	{
		GfxUpdateImage(psInstances->psImageInstance,
					   FALSE);
		psInstances = psInstances->psNextLink;
	}
}

SDirtyBuffer *GfxDirtyBufferSurfaceCreate(UINT32 u32CubeSize)
{
	EGCResultCode eResult;
	SDirtyBuffer *psDirtyBuffer;

	psDirtyBuffer = MemAlloc(sizeof(*psDirtyBuffer));
	GCASSERT(psDirtyBuffer);

	// Get the display surface information

	eResult = GCDisplayGetDisplayBuffer((void **) &psDirtyBuffer->pu16TargetSurface);
	GCASSERT(GC_OK == eResult);

	eResult = GCDisplayGetXSize(&psDirtyBuffer->u32XSurfaceSize);
	GCASSERT(GC_OK == eResult);
	eResult = GCDisplayGetYSize(&psDirtyBuffer->u32YSurfaceSize);
	GCASSERT(GC_OK == eResult);
	eResult = GCDisplayGetDisplayPitch(&psDirtyBuffer->u32TargetSurfacePitch);
	GCASSERT(GC_OK == eResult);

	// Now that we have our known main surface size, let's create a backbuffer surface

	psDirtyBuffer->pu16SourceSurface = MemAlloc(psDirtyBuffer->u32XSurfaceSize * 
														psDirtyBuffer->u32YSurfaceSize * 2);
	GCASSERT(psDirtyBuffer->pu16SourceSurface);
	psDirtyBuffer->u32SourceSurfacePitch = psDirtyBuffer->u32XSurfaceSize;

	if (32 == u32CubeSize)
	{
		psDirtyBuffer->u8PixelShift = 5;
	}
	else
	if (16 == u32CubeSize)
	{
		psDirtyBuffer->u8PixelShift = 4;
	}
	else
	if (8 == u32CubeSize)
	{
		psDirtyBuffer->u8PixelShift = 3;
	}
	else
	if (4 == u32CubeSize)
	{
		psDirtyBuffer->u8PixelShift = 2;
	}
	else
	{
		GCASSERT(0);
	}

	psDirtyBuffer->u8LineShift = 1;

	while ((psDirtyBuffer->u32XSurfaceSize >> psDirtyBuffer->u8PixelShift) > (UINT32) (1 << psDirtyBuffer->u8LineShift))
	{
		++psDirtyBuffer->u8LineShift;
	}

	// Now create a dirty buffer array

	psDirtyBuffer->pu8DirtyBufferBase = MemAlloc((1 << psDirtyBuffer->u8LineShift) *
														 ((psDirtyBuffer->u32YSurfaceSize + (u32CubeSize - 1)) >> psDirtyBuffer->u8PixelShift));
	GCASSERT(psDirtyBuffer->pu8DirtyBufferBase);
	return(psDirtyBuffer);
}

void GfxDirtyBufferSurfaceDelete(SDirtyBuffer *psDirtyBuffer)
{
	GCASSERT(psDirtyBuffer);
	GCFreeMemory(psDirtyBuffer->pu8DirtyBufferBase);
	psDirtyBuffer->pu8DirtyBufferBase = NULL;
	GCFreeMemory(psDirtyBuffer->pu16SourceSurface);
	psDirtyBuffer->pu16SourceSurface = NULL;
	GCFreeMemory(psDirtyBuffer);
}

#ifdef _WIN32
static BOOL sg_bUseASMBlit = FALSE;
#else
static BOOL sg_bUseASMBlit = TRUE;
#endif

void GfxDirtyBufferBlit(SDirtyBuffer *psDirtyBuffer)
{
	UINT32 u32YPos = 0;
	UINT32 u32XPos = 0;
	UINT8 u8RunLength;
	UINT8 *pu8DirtyPointer = psDirtyBuffer->pu8DirtyBufferBase;
	UINT16 *pu16Src = psDirtyBuffer->pu16SourceSurface;
	UINT16 *pu16Dest = psDirtyBuffer->pu16TargetSurface;

	// Wait for vertical retrace
	GCWaitForVsync();

	if (FALSE == sg_bUseASMBlit)
	{
		while (u32YPos < psDirtyBuffer->u32YSurfaceSize)
		{
			u32XPos = 0;

			while (u32XPos < psDirtyBuffer->u32XSurfaceSize)
			{
				u8RunLength = 0;

				if (*pu8DirtyPointer)
				{
					UINT16 *pu16BlitSrc = pu16Src;
					UINT16 *pu16BlitDest = pu16Dest;
					UINT32 u32Loop;

					while ((*pu8DirtyPointer) && (u32XPos < psDirtyBuffer->u32XSurfaceSize))
					{
						*pu8DirtyPointer = 0;
						u32XPos += (1 << psDirtyBuffer->u8PixelShift);
						++u8RunLength;
						++pu8DirtyPointer;
						pu16Src += (1 << psDirtyBuffer->u8PixelShift);
						pu16Dest += (1 << psDirtyBuffer->u8PixelShift);
					}

					// Now blit the run length, however long it may be

					for (u32Loop = 0; u32Loop < (UINT32) (1 << psDirtyBuffer->u8PixelShift); u32Loop++)
					{
						memcpy(pu16BlitDest, pu16BlitSrc, (u8RunLength << (psDirtyBuffer->u8PixelShift + 1)));
						pu16BlitDest += psDirtyBuffer->u32TargetSurfacePitch;
						pu16BlitSrc += psDirtyBuffer->u32SourceSurfacePitch;
					}
				}
				else
				{
					// Skip it!

					u32XPos += (1 << psDirtyBuffer->u8PixelShift);
					pu16Src += (1 << psDirtyBuffer->u8PixelShift);
					pu16Dest += (1 << psDirtyBuffer->u8PixelShift);
					++pu8DirtyPointer;
				}
			}

			// End of line. Need to make our pointers do the appropriate dance

			pu8DirtyPointer += ((1 << psDirtyBuffer->u8LineShift) - (psDirtyBuffer->u32XSurfaceSize >> psDirtyBuffer->u8PixelShift));
			pu16Src = (pu16Src - psDirtyBuffer->u32XSurfaceSize) +
					   (psDirtyBuffer->u32SourceSurfacePitch << psDirtyBuffer->u8PixelShift);
			pu16Dest = (pu16Dest - psDirtyBuffer->u32XSurfaceSize) +
						(psDirtyBuffer->u32TargetSurfacePitch << psDirtyBuffer->u8PixelShift);

			u32YPos += (1 << psDirtyBuffer->u8PixelShift);
		}
	}
	else
	{
#ifdef _WIN32
		GCASSERT(0);
#else
		ARMGfxDirtyBufferBlit(psDirtyBuffer);
#endif
	}
}

void GfxDirtyBufferMark(SDirtyBuffer *psDirtyBuffer,
						UINT32 u32XPos,
						UINT32 u32YPos,
						UINT32 u32XSize,
						UINT32 u32YSize)
{
	UINT8 *pu8DirtyOffset;

	// Quantize X's size
	u32XSize += (u32XPos & ((1 << psDirtyBuffer->u8PixelShift) - 1)) + ((1 << psDirtyBuffer->u8PixelShift) - 1);

	// Quantize Y's size
	u32YSize += (u32YPos & ((1 << psDirtyBuffer->u8PixelShift) - 1)) + ((1 << psDirtyBuffer->u8PixelShift) - 1);

	u32XSize >>= psDirtyBuffer->u8PixelShift;
	u32YSize >>= psDirtyBuffer->u8PixelShift;
	u32XPos >>= psDirtyBuffer->u8PixelShift;
	u32YPos >>= psDirtyBuffer->u8PixelShift;

	// Set our dirty offset position

	pu8DirtyOffset = psDirtyBuffer->pu8DirtyBufferBase + u32XPos + (u32YPos << psDirtyBuffer->u8LineShift);

	while (u32YSize--)
	{
		memset((void *) pu8DirtyOffset, 1, u32XSize);
		pu8DirtyOffset += (1 << psDirtyBuffer->u8LineShift);
	}
}

void GfxDirtyBufferMarkSurface(UINT32 u32XPos,
							   UINT32 u32YPos,
							   UINT32 u32XSize,
							   UINT32 u32YSize)
{
	UINT32 u32XDirtyBufferSize;
	UINT32 u32YDirtyBufferSize;
	UINT8 *pu8DirtyBuffer;

	// Mark the dirty buffer region

	pu8DirtyBuffer = sg_pu8DirtyBuffer + 
					 ((u32XPos / DIRTY_X_BLOCK_SIZE) +
					 ((u32YPos / DIRTY_Y_BLOCK_SIZE) * sg_u32DirtyBufferPitch));

	u32XDirtyBufferSize = (((u32XPos & (DIRTY_X_BLOCK_SIZE - 1)) + u32XSize + (DIRTY_X_BLOCK_SIZE - 1)) / DIRTY_X_BLOCK_SIZE);
	u32YDirtyBufferSize = (((u32YPos & (DIRTY_Y_BLOCK_SIZE - 1)) + u32YSize + (DIRTY_Y_BLOCK_SIZE - 1)) / DIRTY_Y_BLOCK_SIZE);

	GCASSERT(u32YDirtyBufferSize);

	// Now update the dirty buffer

	while (u32YDirtyBufferSize)
	{
		memset((void *) pu8DirtyBuffer, 0xff, (size_t) (u32XDirtyBufferSize));

		pu8DirtyBuffer += sg_u32DirtyBufferPitch;
		u32YDirtyBufferSize--;
	}
}

void GfxDirtyBufferRepaint(SDirtyBuffer *psDirtyBuffer)
{
	GfxDirtyBufferMark(psDirtyBuffer,
					   0,
					   0,
					   psDirtyBuffer->u32XSurfaceSize,
					   psDirtyBuffer->u32YSurfaceSize);
}

void GfxDrawImageDirect(SImageInstance *psSrcImage,
						SImageInstance *psDestImage,
						UINT32 u32DestPitch,
					    BOOL bErase,
						SDirtyBuffer *psDirtyBuffer)
{
	INT32 s32XBlitSize;
	INT32 s32YBlitSize;
	INT32 s32XPos;
	INT32 s32YPos;
	UINT16 *pu16Src;
	UINT16 *pu16Dest;
	UINT8 *pu8Src;
	UINT8 *pu8Mask;
	UINT8 *pu8TranslucentMask;
	
	if (NULL == psSrcImage)
	{
		return;
	}

	// Let's draw the partial image if it's in range

	pu8Mask = psSrcImage->pu8Transparent;
	pu8TranslucentMask = psSrcImage->pu8TranslucentMask;
	pu16Src = psSrcImage->pu16SrcPointer;
	pu8Src = psSrcImage->pu8SrcPointer;
	pu16Dest = psDestImage->pu16DestSurfacePointer;

	// Get our X and Y blit sizes, and adjust our source/dest pointers
	// psImage is the pointer to the image we're trying to cover with the
	// image in psImagePtr.

	s32XBlitSize = (INT32) psSrcImage->psImage->u32XSize;
	s32XPos = psSrcImage->s32XPos;

	if (s32XPos < 0)
	{
		s32XBlitSize += (0 + s32XPos);
		pu16Src -= (0 + s32XPos);
		pu8Src -= (0 + s32XPos);
		pu8Mask -= (0 + s32XPos);
		pu8TranslucentMask -= (0 + s32XPos);

		if (s32XBlitSize < 1)
		{
			// Out of range
			return;
		}

		s32XPos = 0;
	}

	s32YBlitSize = (INT32) psSrcImage->psImage->u32YSize;
	s32YPos = psSrcImage->s32YPos;

	if (s32YPos < 0)
	{
		s32YBlitSize += (0 + s32YPos);
		pu16Src -= ((0 + s32YPos) * psSrcImage->psImage->u32Pitch);
		pu8Src -= ((0 + s32YPos) * psSrcImage->psImage->u32Pitch);

		if (s32YBlitSize < 1)
		{
			// Out of range
			return;
		}

		s32YPos = 0;
	}

	if ((s32XPos + s32XBlitSize) >= (INT32) psDestImage->u32XSizeClipped)
	{
		s32XBlitSize = (INT32) psDestImage->u32XSizeClipped - (INT32) s32XPos;
	}

	if ((s32YPos + s32YBlitSize) >= (INT32) psDestImage->u32YSizeClipped)
	{
		s32YBlitSize = (INT32) psDestImage->u32YSizeClipped - (INT32) s32YPos;
	}

	pu16Dest += (s32XPos + (s32YPos * u32DestPitch));

	if (s32XBlitSize > 0 && s32YBlitSize > 0)
	{
		UINT16 *pu16CLUT = NULL;

		if (psDirtyBuffer)
		{
			GfxDirtyBufferMark(psDirtyBuffer,
							   (UINT32) s32XPos,
							   (UINT32) s32YPos,
							   (UINT32) s32XBlitSize,
							   (UINT32) s32YBlitSize);
		}

		if (bErase)
		{
			s32XBlitSize <<= 1;
			while (s32YBlitSize--)
			{
				memset((void *) pu16Dest, 0, s32XBlitSize);
				pu16Dest += u32DestPitch;
			}
			return;
		}
		
		if (psSrcImage->psCLUT)
		{
			pu16CLUT = (UINT16 *) psSrcImage->psCLUT->pvCLUT;
		}

		// Only blit if there's something to blit

		if (psSrcImage->psImage->pu8Transparent)
		{
			UINT32 u32Loop;

			// Image is transparent

			while (s32YBlitSize)
			{
				u32Loop = (UINT32) s32XBlitSize;

				if (pu16CLUT)
				{
					while (u32Loop)
					{
						if (0 == *pu8Mask)
						{
							*pu16Dest = pu16CLUT[*pu16Src];
						}

						pu16Dest++;
						pu16Src++;
						pu8Mask++;
						u32Loop--;
					}
				}
				else
				{
					while (u32Loop)
					{
						if (0 == *pu8Mask)
						{
							*pu16Dest = *pu16Src;
						}

						pu16Dest++;
						pu16Src++;
						pu8Mask++;
						u32Loop--;
					}
				}

				pu16Src += (psSrcImage->psImage->u32Pitch - s32XBlitSize);
				pu8Mask += (psSrcImage->psImage->u32Pitch - s32XBlitSize);
				pu16Dest += (u32DestPitch - s32XBlitSize);

				s32YBlitSize--;
			}
		}
		else
		if (psSrcImage->psImage->bTranslucent)
		{
			// Image is translucent
			UINT32 u32Loop;
///			UINT8 u8Red;
///			UINT8 u8Green;
///			UINT8 u8Blue;

			while (s32YBlitSize)
			{
				UINT8 u8MaskByte;

				u32Loop = (UINT32) s32XBlitSize;

#ifdef _WIN32
				while (u32Loop)
				{
					u8MaskByte = *pu8TranslucentMask;

/*					if (u8MaskByte < 0x10)
					{
						// Completely solid (or solid enough)

						if (NULL == pu16CLUT)
						{
							*pu16Dest = *pu16Src;
						}
						else
						{
							*pu16Dest = pu16CLUT[*pu16Src];
						}

					}
					else */
					if (u8MaskByte > 0x30)
					{
						// Don't do anything - it's completely transparent
					}
					else
					{
//						UINT16 u16Dest;
//						UINT16 u16Src;

//						u16Dest = *pu16Dest;

//						if (NULL == pu16CLUT)
//						{
//							u16Src = *pu16Src;
//						}
//						else
//						{
//							u16Src = pu16CLUT[*pu16Src];
//						}

						// It's partial/ugly. Time to separate the bands, do the add/multiply, etc...

//						u8Red = ((u16Dest >> 11) * (u8MaskByte)) >> 6;
//						u8Green = (((u16Dest >> 5) & 0x3f) * (u8MaskByte)) >> 6;
//						u8Blue = ((u16Dest & 0x1f) * (u8MaskByte)) >> 6;

/*						*pu16Dest = sg_u16RedGradientSaturation[u8Red + (u16Src >> 11)] |
									sg_u16GreenGradientSaturation[u8Green + ((u16Src >> 5) & 0x3f)] |
									sg_u16BlueGradientSaturation[u8Blue + (u16Src & 0x1f)]; */

						*pu16Dest = *pu16Src;
					}
					pu16Dest++;
					pu16Src++;
					pu8TranslucentMask++;
					u32Loop--;
				}

				pu16Src += (psSrcImage->psImage->u32Pitch - s32XBlitSize);
				pu8TranslucentMask += (psSrcImage->psImage->u32XSize - s32XBlitSize);
				pu16Dest += (u32DestPitch - s32XBlitSize);

#else
				ARMBlitTransparent(s32XBlitSize,
								   pu8TranslucentMask,
								   pu16Src,
								   pu16Dest);
				pu16Src += psSrcImage->psImage->u32Pitch;
				pu8TranslucentMask += psSrcImage->psImage->u32XSize;
				pu16Dest += u32DestPitch;
#endif
				
				s32YBlitSize--;
			}
		}
		else
		{
			// Image is solid

			if (pu16CLUT)
			{
				INT32 s32Loop;

				while (s32YBlitSize)
				{
					s32Loop = s32XBlitSize;

					while (s32Loop--)
					{
						*pu16Dest = pu16CLUT[*pu16Src];
						++pu16Dest;
						++pu16Src;
					}

					pu16Src += (psSrcImage->psImage->u32Pitch - s32XBlitSize);
					pu16Dest += (u32DestPitch - s32XBlitSize);

					s32YBlitSize--;
				}
			}
			else
			{
				if (psSrcImage->psImage->pu8ImageData)
				{
#ifdef WIN32
					UINT16 *pu16Palette;
					UINT16 u16TransparentIndex;

					// 8BPP Image!

					u16TransparentIndex = psSrcImage->psImage->u16TransparentIndex;
					pu16Palette = psSrcImage->psImage->pu16Palette;

					while (s32YBlitSize)
					{
						INT32 s32Loop = 0;

						s32Loop = s32XBlitSize;

						while (s32Loop--)
						{
							UINT8 u8Data;

							u8Data = *pu8Src;
							pu8Src++;
							if ((UINT16) u8Data != u16TransparentIndex)
							{
								*pu16Dest = pu16Palette[u8Data];
							}
							++pu16Dest;
						}

						pu8Src += (psSrcImage->psImage->u32Pitch - s32XBlitSize);
						pu16Dest += (u32DestPitch - s32XBlitSize);

						s32YBlitSize--;
					}
#else


					ARMBlit8Bit(s32XBlitSize,
								s32YBlitSize,
								psSrcImage->psImage->u32Pitch,
								pu8Src,
								pu16Dest,
								u32DestPitch << 1,
								psSrcImage->psImage->pu16Palette,
								psSrcImage->psImage->u16TransparentIndex);
#endif
				}
				else
				{
					while (s32YBlitSize)
					{
						memcpy((void *) pu16Dest, (void *) pu16Src, (size_t) (s32XBlitSize << 1));
						pu16Src += psSrcImage->psImage->u32Pitch;
						pu16Dest += u32DestPitch;

						s32YBlitSize--;
					}
				}
			}
		}
	}
}

// Color to black & White: Y=0.3RED+0.59GREEN+0.11Blue
#define GRAY(x, y)	GrayValue(x, y)

UINT16 GrayValue(UINT16 u16Color,
				 UINT8 u8Shift)
{
	UINT8 u8Red;
	UINT8 u8Green;
	UINT8 u8Blue;

	u8Red = ((u16Color & 0xf800) * 0x4d) >> (19 + u8Shift);
	u8Green = ((u16Color & 0x07e0) * 0x97) >> (13 + u8Shift);
	u8Blue = ((u16Color & 0x1f) * 0x1c) >> (9 + u8Shift);

	u8Red += (u8Green + u8Blue);
	if (u8Red > 0x3f)
	{
		u8Red = 0x3f;
	}

	return((u8Red << 5) | (u8Red >> 1) | ((u8Red >> 1) << 11));
}

static UINT16 sg_u16GrayPalette[0x100];

void GfxBlitImageToImageGray(SImage *psDest,
							 SImage *psSrc,
							 UINT32 u32XPos,
							 UINT32 u32YPos,
							 UINT32 u32XDestMax,
							 UINT32 u32YDestMax)
{
	INT32 s32XBlitSize;
	INT32 s32YBlitSize;
	UINT16 *pu16Src;
	UINT16 *pu16Dest;
	BOOL bToggle = FALSE;

	GCASSERT(psDest);
	GCASSERT(psSrc);

	if (0 == u32XDestMax)
	{
		u32XDestMax = psDest->u32XSize;
	}

	if (0 == u32YDestMax)
	{
		u32YDestMax = psDest->u32YSize;
	}

	if (u32XDestMax > psDest->u32XSize)
	{
		u32XDestMax = psDest->u32XSize;
	}

	if (u32YDestMax > psDest->u32YSize)
	{
		u32YDestMax = psDest->u32YSize;
	}

	s32XBlitSize = (INT32) psSrc->u32XSize;
	s32YBlitSize = (INT32) psSrc->u32YSize;

	if ((u32XPos + psSrc->u32XSize) > u32XDestMax)
	{
		s32XBlitSize = (INT32) u32XDestMax - (INT32) u32XPos;
	}

	if ((u32YPos + psSrc->u32YSize) > u32YDestMax)
	{
		s32YBlitSize = (INT32) u32YDestMax - (INT32) u32YPos;
	}

	if ((s32XBlitSize < 0) ||
		(s32YBlitSize < 0))
	{
		// Nothing to blit
		return;
	}

	pu16Src = psSrc->pu16ImageData;
	pu16Dest = psDest->pu16ImageData + (psDest->u32Pitch * u32YPos) + u32XPos;

	if (psSrc->pu8Transparent)
	{
		// Normal transparent
		UINT8 *pu8Mask = psSrc->pu8Transparent;
		INT32 s32XRemaining = 0;

		while (s32YBlitSize--)
		{
			s32XRemaining = s32XBlitSize;
			while (s32XRemaining)
			{
				if (0 == *pu8Mask)
				{
					*pu16Dest = GRAY(*pu16Src, (UINT8) bToggle);
				}

				if (bToggle)
				{
					bToggle = FALSE;
				}
				else
				{
					bToggle = TRUE;
				}

				pu16Dest++;
				pu16Src++;
				pu8Mask++;
				s32XRemaining--;
			}

			if (0 == (s32XBlitSize & 1))
			{
				if (bToggle)
				{
					bToggle = FALSE;
				}
				else
				{
					bToggle = TRUE;
				}
			}

			pu16Src += (psSrc->u32Pitch - s32XBlitSize);
			pu8Mask += (psSrc->u32Pitch - s32XBlitSize);
			pu16Dest += (psDest->u32Pitch - s32XBlitSize);
		}
	}
	else
	if (psSrc->pu8TranslucentMask)
	{
		// Normal translucent
		UINT32 u32Loop;
		UINT8 *pu8TranslucentMask = psSrc->pu8TranslucentMask;

		while (s32YBlitSize)
		{
			UINT8 u8MaskByte;

			u32Loop = (UINT32) s32XBlitSize;

			while (u32Loop)
			{
				UINT16 u16Dest;
				UINT16 u16Src;
				UINT8 u8Red;
				UINT8 u8Green;
				UINT8 u8Blue;

				u8MaskByte = *pu8TranslucentMask;
				u16Dest = *pu16Dest;

				u16Src = GRAY(*pu16Src, (UINT8) bToggle);

				if (bToggle)
				{
					bToggle = FALSE;
				}
				else
				{
					bToggle = TRUE;
				}

				// It's partial/ugly. Time to separate the bands, do the add/multiply, etc...
				u8Red = (((u16Src >> 11) * (0xff - u8MaskByte)) >> 8) +
						(((u16Dest >> 11) * (u8MaskByte)) >> 8) ;

				u8Green = ((((u16Src >> 5) & 0x3f) * (0xff - u8MaskByte)) >> 8) +
						  ((((u16Dest >> 5) & 0x3f) * (u8MaskByte)) >> 8);

				u8Blue = (((u16Src & 0x1f) * (0xff - u8MaskByte)) >> 8) +
						 (((u16Dest & 0x1f) * (u8MaskByte)) >> 8);

				*pu16Dest = sg_u16RedGradientSaturation[u8Red] |
							sg_u16GreenGradientSaturation[u8Green] |
							sg_u16BlueGradientSaturation[u8Blue];

				pu16Dest++;
				pu16Src++;
				pu8TranslucentMask++;
				u32Loop--;
			}

			if (0 == (s32XBlitSize & 1))
			{
				if (bToggle)
				{
					bToggle = FALSE;
				}
				else
				{
					bToggle = TRUE;
				}
			}

			pu16Src += (psSrc->u32Pitch - s32XBlitSize);
			pu8TranslucentMask += (psSrc->u32Pitch - s32XBlitSize);
			pu16Dest += (psDest->u32Pitch - s32XBlitSize);

			s32YBlitSize--;
		}
	}
	else
	{
		INT32 s32XRemaining = 0;

		// Solid image

		if (psSrc->pu16ImageData)
		{
			while (s32YBlitSize--)
			{
				s32XRemaining = s32XBlitSize;
				while (s32XRemaining)
				{
					*pu16Dest = GRAY(*pu16Src, (UINT8) bToggle);
					if (bToggle)
					{
						bToggle = FALSE;
					}
					else
					{
						bToggle = TRUE;
					}

					pu16Dest++;
					pu16Src++;
					s32XRemaining--;
				}

				if (0 == (s32XBlitSize & 1))
				{
					if (bToggle)
					{
						bToggle = FALSE;
					}
					else
					{
						bToggle = TRUE;
					}
				}

				pu16Src += (psSrc->u32Pitch - s32XBlitSize);
				pu16Dest += (psDest->u32Pitch - s32XBlitSize);
			}
		}
		else
		{
			UINT8 *pu8Src = psSrc->pu8ImageData;
			UINT16 u16TransparentIndex = psSrc->u16TransparentIndex;
			UINT32 u32Loop;

			// 8BPP Image. Yuck. First, convert the palette to greyscale
			for (u32Loop = 0; u32Loop < (sizeof(sg_u16GrayPalette) / sizeof(sg_u16GrayPalette[0])); u32Loop++)
			{
				sg_u16GrayPalette[u32Loop] = GRAY(psSrc->pu16Palette[u32Loop], 0);
			}

			while (s32YBlitSize--)
			{
				s32XRemaining = s32XBlitSize;
				while (s32XRemaining)
				{
					if ((UINT16) *pu8Src != u16TransparentIndex)
					{
						if (bToggle)
						{
							*pu16Dest = sg_u16GrayPalette[*pu8Src];
						}
						else
						{
							*pu16Dest = GRAY(sg_u16GrayPalette[*pu8Src], 1);
						}
					}

					if (bToggle)
					{
						bToggle = FALSE;
					}
					else
					{
						bToggle = TRUE;
					}

					pu16Dest++;
					pu8Src++;
					s32XRemaining--;
				}

				if (0 == (s32XBlitSize & 1))
				{
					if (bToggle)
					{
						bToggle = FALSE;
					}
					else
					{
						bToggle = TRUE;
					}
				}

				pu8Src += (psSrc->u32Pitch - s32XBlitSize);
				pu16Dest += (psDest->u32Pitch - s32XBlitSize);
			}
		}
	}
}

void GfxBlitImageToImage(SImage *psDest,
						 SImage *psSrc,
						 UINT32 u32XPos,
						 UINT32 u32YPos,
						 UINT32 u32XDestMax,
						 UINT32 u32YDestMax)
{
	INT32 s32XBlitSize;
	INT32 s32YBlitSize;
	UINT16 *pu16Dest;

	GCASSERT(psDest);
	GCASSERT(psSrc);

	if (0 == u32XDestMax)
	{
		u32XDestMax = psDest->u32XSize;
	}

	if (0 == u32YDestMax)
	{
		u32YDestMax = psDest->u32YSize;
	}

	if (u32XDestMax > psDest->u32XSize)
	{
		u32XDestMax = psDest->u32XSize;
	}

	if (u32YDestMax > psDest->u32YSize)
	{
		u32YDestMax = psDest->u32YSize;
	}

	s32XBlitSize = (INT32) psSrc->u32XSize;
	s32YBlitSize = (INT32) psSrc->u32YSize;

	if ((u32XPos + psSrc->u32XSize) > u32XDestMax)
	{
		s32XBlitSize = (INT32) u32XDestMax - (INT32) u32XPos;
	}

	if ((u32YPos + psSrc->u32YSize) > u32YDestMax)
	{
		s32YBlitSize = (INT32) u32YDestMax - (INT32) u32YPos;
	}

	if ((s32XBlitSize < 0) ||
		(s32YBlitSize < 0))
	{
		// Nothing to blit
		return;
	}

	// Compute our 16bpp destination
	pu16Dest = psDest->pu16ImageData + (psDest->u32Pitch * u32YPos) + u32XPos;

	if (psSrc->pu16ImageData)
	{
		UINT16 *pu16Src;

		// 16BPP blitting!
		pu16Src = psSrc->pu16ImageData;

		if (psSrc->pu8Transparent)
		{
			// Normal transparent
			UINT8 *pu8Mask = psSrc->pu8Transparent;
			INT32 s32XRemaining = 0;

			while (s32YBlitSize--)
			{
				s32XRemaining = s32XBlitSize;
				while (s32XRemaining--)
				{
					if (0 == *pu8Mask)
					{
						*pu16Dest = *pu16Src;
					}

					pu16Dest++;
					pu16Src++;
					pu8Mask++;
				}

				pu16Src += (psSrc->u32Pitch - s32XBlitSize);
				pu8Mask += (psSrc->u32Pitch - s32XBlitSize);
				pu16Dest += (psDest->u32Pitch - s32XBlitSize);
			}
		}
		else
		if (psSrc->pu8TranslucentMask)
		{
			// Normal translucent
			UINT32 u32Loop;
			UINT8 *pu8TranslucentMask = psSrc->pu8TranslucentMask;

			while (s32YBlitSize)
			{
				UINT8 u8MaskByte;

				u32Loop = (UINT32) s32XBlitSize;

				while (u32Loop)
				{
					UINT16 u16Dest;
					UINT16 u16Src;

					u8MaskByte = *pu8TranslucentMask;
					u16Dest = *pu16Dest;
					u16Src = *pu16Src;

					if (u8MaskByte < 0x4)
					{
						// Completely solid
						*pu16Dest = u16Src;
					}
					else
					if (u8MaskByte >= 0xfc)
					{
						// Completely transparent - don't touch
					}
					else
					{
						UINT8 u8Red;
						UINT8 u8Green;
						UINT8 u8Blue;

						// It's partial/ugly. Time to separate the bands, do the add/multiply, etc...
						u8Red = (((u16Src >> 11) * (0xff - u8MaskByte)) >> 8) +
								(((u16Dest >> 11) * (u8MaskByte)) >> 8);

						u8Green = ((((u16Src >> 5) & 0x3f) * (0xff - u8MaskByte)) >> 8) +
								  ((((u16Dest >> 5) & 0x3f) * (u8MaskByte)) >> 8);

						u8Blue = (((u16Src & 0x1f) * (0xff - u8MaskByte)) >> 8) +
								 (((u16Dest & 0x1f) * (u8MaskByte)) >> 8);

						*pu16Dest = sg_u16RedGradientSaturation[u8Red] |
									sg_u16GreenGradientSaturation[u8Green] |
									sg_u16BlueGradientSaturation[u8Blue];
					}

					pu16Dest++;
					pu16Src++;
					pu8TranslucentMask++;
					u32Loop--;
				}

				pu16Src += (psSrc->u32Pitch - s32XBlitSize);
				pu8TranslucentMask += (psSrc->u32Pitch - s32XBlitSize);
				pu16Dest += (psDest->u32Pitch - s32XBlitSize);

				s32YBlitSize--;
			}
		}
		else
		{
			UINT8 *pu8Mask = psDest->pu8Transparent;

			if (NULL == pu8Mask)
			{
				pu8Mask = psDest->pu8TranslucentMask;
			}

			if (pu8Mask)
			{
				pu8Mask += (psDest->u32Pitch * u32YPos) + u32XPos;
			}

			// Solid image
			while (s32YBlitSize--)
			{
				if (pu8Mask)
				{
					memset((void *) pu8Mask, 0, s32XBlitSize);
					pu8Mask += psDest->u32Pitch;
				}

				memcpy((void *) pu16Dest,
					   (void *) pu16Src,
					   s32XBlitSize << 1);
				pu16Dest += psDest->u32Pitch;
				pu16Src += psSrc->u32Pitch;
			}
		}
	}
	else
	{
		UINT16 *pu16Palette = psSrc->pu16Palette;
		UINT8 *pu8Src;

		// 8BPP blitting!
		pu8Src = psSrc->pu8ImageData;

		if (psSrc->pu8Transparent)
		{
			// Normal transparent
			UINT8 *pu8Mask = psSrc->pu8Transparent;
			INT32 s32XRemaining = 0;

			if (psDest->pu8TranslucentMask)
			{
				UINT8 *pu8TranslucentMask = psDest->pu8TranslucentMask + (psDest->u32Pitch * u32YPos) + u32XPos;

				// Blit from (possibly) transparent to translucent

				while (s32YBlitSize--)
				{
					s32XRemaining = s32XBlitSize;
					while (s32XRemaining--)
					{
						if (0 == *pu8Mask)
						{
							*pu16Dest = pu16Palette[*pu8Src];
						}

						*pu8TranslucentMask = 0x00;
						pu16Dest++;
						pu8Src++;
						pu8Mask++;
						pu8TranslucentMask++;
					}

					pu8Src += (psSrc->u32Pitch - s32XBlitSize);
					pu8Mask += (psSrc->u32Pitch - s32XBlitSize);
					pu16Dest += (psDest->u32Pitch - s32XBlitSize);
					pu8TranslucentMask += (psDest->u32Pitch - s32XBlitSize);
				}
			}
			else
			{
				while (s32YBlitSize--)
				{
					s32XRemaining = s32XBlitSize;
					while (s32XRemaining--)
					{
						if (0 == *pu8Mask)
						{
							*pu16Dest = pu16Palette[*pu8Src];
						}

						pu16Dest++;
						pu8Src++;
						pu8Mask++;
					}

					pu8Src += (psSrc->u32Pitch - s32XBlitSize);
					pu8Mask += (psSrc->u32Pitch - s32XBlitSize);
					pu16Dest += (psDest->u32Pitch - s32XBlitSize);
				}
			}
		}
		else
		if (psSrc->pu8TranslucentMask)
		{
			// Normal translucent
			UINT32 u32Loop;
			UINT8 *pu8TranslucentMask = psSrc->pu8TranslucentMask;

			while (s32YBlitSize)
			{
				UINT8 u8MaskByte;

				u32Loop = (UINT32) s32XBlitSize;

				while (u32Loop)
				{
					UINT16 u16Dest;
					UINT16 u16Src;

					u8MaskByte = *pu8TranslucentMask;
					u16Dest = *pu16Dest;
					u16Src = pu16Palette[*pu8Src];

					if (u8MaskByte < 0x4)
					{
						// Completely solid
						*pu16Dest = u16Src;
					}
					else
					if (u8MaskByte >= 0xfc)
					{
						// Completely transparent - don't touch
					}
					else
					{
						UINT8 u8Red;
						UINT8 u8Green;
						UINT8 u8Blue;

						// It's partial/ugly. Time to separate the bands, do the add/multiply, etc...
						u8Red = (((u16Src >> 11) * (0xff - u8MaskByte)) >> 8) +
								(((u16Dest >> 11) * (u8MaskByte)) >> 8);

						u8Green = ((((u16Src >> 5) & 0x3f) * (0xff - u8MaskByte)) >> 8) +
								  ((((u16Dest >> 5) & 0x3f) * (u8MaskByte)) >> 8);

						u8Blue = (((u16Src & 0x1f) * (0xff - u8MaskByte)) >> 8) +
								 (((u16Dest & 0x1f) * (u8MaskByte)) >> 8);

						*pu16Dest = sg_u16RedGradientSaturation[u8Red] |
									sg_u16GreenGradientSaturation[u8Green] |
									sg_u16BlueGradientSaturation[u8Blue];
					}

					pu16Dest++;
					pu8Src++;
					pu8TranslucentMask++;
					u32Loop--;
				}

				pu8Src += (psSrc->u32Pitch - s32XBlitSize);
				pu8TranslucentMask += (psSrc->u32Pitch - s32XBlitSize);
				pu16Dest += (psDest->u32Pitch - s32XBlitSize);

				s32YBlitSize--;
			}
		}
		else
		{
			// Solid image

			INT32 s32XRemaining = 0;
			UINT16 u16Index = psSrc->u16TransparentIndex;

			if (psDest->pu8TranslucentMask)
			{
				UINT8 *pu8TranslucentMask = psDest->pu8TranslucentMask + (psDest->u32Pitch * u32YPos) + u32XPos;
				UINT8 *pu8Mask = psDest->pu8Transparent;

				if (NULL == pu8Mask)
				{
					pu8Mask = psDest->pu8TranslucentMask;
				}

				if (pu8Mask)
				{
					pu8Mask += (psDest->u32Pitch * u32YPos) + u32XPos;
				}

				while (s32YBlitSize--)
				{
					s32XRemaining = s32XBlitSize;

					if (pu8Mask)
					{
						// Target is transparent and/or translucent
						while (s32XRemaining--)
						{
							if ((UINT16) *pu8Src != u16Index)
							{
								*pu16Dest = pu16Palette[*pu8Src];
								*pu8TranslucentMask = 0x00;
							}
							else
							{
								*pu8TranslucentMask = 0xff;
							}

							*pu8Mask = 0;
							++pu8TranslucentMask;
							pu16Dest++;
							pu8Src++;
							pu8Mask++;
						}
					}
					else
					{
						// Target is not transparent nor translucent
						while (s32XRemaining--)
						{
							if ((UINT16) *pu8Src != u16Index)
							{
								*pu16Dest = pu16Palette[*pu8Src];
								*pu8TranslucentMask = 0x00;
							}
							else
							{
								*pu8TranslucentMask = 0xff;
							}

							++pu8TranslucentMask;
							pu16Dest++;
							pu8Src++;
						}
					}

					if (pu8Mask)
					{
						pu8Mask += (psDest->u32Pitch - s32XBlitSize);
					}

					pu8Src += (psSrc->u32Pitch - s32XBlitSize);
					pu16Dest += (psDest->u32Pitch - s32XBlitSize);
					pu8TranslucentMask += (psDest->u32Pitch - s32XBlitSize);
				}
			}
			else
			{
				while (s32YBlitSize--)
				{
					s32XRemaining = s32XBlitSize;
					while (s32XRemaining--)
					{
						if ((UINT16) *pu8Src != u16Index)
						{
							*pu16Dest = pu16Palette[*pu8Src];
						}
						pu16Dest++;
						pu8Src++;
					}
					pu8Src += (psSrc->u32Pitch - s32XBlitSize);
					pu16Dest += (psDest->u32Pitch - s32XBlitSize);
				}
			}
		}
	}
}

void GfxAnimReset(SImageGroup *psImageGroup)
{
	EPlaybackDir eDir;

	// Get our direction. If we're stopped, get it from the latched
	// playback type.
	eDir = psImageGroup->eActivePlaybackType & ~(EDIR_REPEAT | EDIR_RUNNING);

	psImageGroup->u32Accumulator = 0;

	if (eDir == EDIR_PINGPONG)
	{
		psImageGroup->psCurrentImage = psImageGroup->psLinkHead->psImage;
		psImageGroup->s8PingPongDirection = 1;
		psImageGroup->u32PingPongFramesRemaining = (psImageGroup->u32FrameCount << 1) - 2;
	}
	else
	if (eDir == EDIR_FORWARD)
	{
		psImageGroup->psCurrentImage = psImageGroup->psLinkHead->psImage;
	}
	else
	if (eDir == EDIR_BACKWARD)
	{
		psImageGroup->psCurrentImage = psImageGroup->psLinkTail->psImage;
		psImageGroup->u32Accumulator = psImageGroup->u32TotalAnimTime;
	}
	else
	{
		GCASSERT(0);
	}

	psImageGroup->bPendingRefresh = TRUE;
}

void GfxAnimSetTickRate(SImageGroup *psImageGroup,
						UINT32 u32Milliseconds)
{
	psImageGroup->u32HardwareTickRate = u32Milliseconds;
}

void GfxAnimSetPlaybackSpeed(SImageGroup *psImageGroup,
							 UINT32 u32PlaybackSpeed)
{
	psImageGroup->u32Accumulator = 0;
}

static BOOL GfxAnimStepAdvance(EPlaybackDir eDirActive,
							   SImageGroup *psImageGroup,
							   BOOL bForceRepeat,
							   BOOL *pbExtremeHit)
{
	EPlaybackDir eDir;
	BOOL bRepeat = FALSE;
	BOOL bNewFrame = FALSE;

	// Get the direction
	eDir = (EPlaybackDir) (eDirActive & ~(EDIR_REPEAT | EDIR_RUNNING));

	// And the repeat rate
	if ((eDirActive & EDIR_REPEAT) ||
		(bForceRepeat))
	{
		bRepeat = TRUE;
	}

	if (EDIR_FORWARD == eDir)
	{
		// We're stepping one frame forward
		GCASSERT(psImageGroup->psCurrentImage);

		if (psImageGroup->psCurrentImage->psNextLink)
		{
			psImageGroup->psCurrentImage = psImageGroup->psCurrentImage->psNextLink;
			bNewFrame = TRUE;
		}
		else
		{
			// On our last frame. Let's see what we should do, based on the repeat boolean
			if (bRepeat)
			{
				psImageGroup->psCurrentImage = psImageGroup->psLinkHead->psImage;
				bNewFrame = TRUE;
			}
			else
			{
				// Don't do anything - we're on the last frame - stick here
			}

			if (pbExtremeHit)
			{
				*pbExtremeHit = TRUE;
			}
		}
	}
	else
	if (EDIR_BACKWARD == eDir)
	{
		// We're stepping one frame backward
		GCASSERT(psImageGroup->psCurrentImage);

		if (psImageGroup->psCurrentImage->psPriorLink)
		{
			psImageGroup->psCurrentImage = psImageGroup->psCurrentImage->psPriorLink;
			bNewFrame = TRUE;
		}
		else
		{
			// We're at the beginning of our frames
			if (bRepeat)
			{
				psImageGroup->psCurrentImage = psImageGroup->psLinkTail->psImage;
				bNewFrame = TRUE;
			}
			else
			{
				// Don't do anything - we're on the first frame - stick here
			}

			if (pbExtremeHit)
			{
				*pbExtremeHit = TRUE;
			}
		}
	}
	else
	if (EDIR_PINGPONG == eDir)
	{
//		DebugOut("%s: Frame=%u, Remaining=%u\n", __FUNCTION__, psImageGroup->psCurrentImage->u32FrameNumber, psImageGroup->u32PingPongFramesRemaining);
		// If we have frames remaining, let's go for it
		if (psImageGroup->u32PingPongFramesRemaining)
		{
			BOOL bExtremeHit = FALSE;

			if (1 == psImageGroup->s8PingPongDirection)
			{
				// Forward. Let's step!
				if (GfxAnimStepAdvance(EDIR_FORWARD,
									   psImageGroup,
									   FALSE,
									   &bExtremeHit))
				{
					// We've got a new frame!
					bNewFrame = TRUE;
				}
			}
			else
			if (-1 == psImageGroup->s8PingPongDirection)
			{
				// Backward. Let's step!
				if (GfxAnimStepAdvance(EDIR_BACKWARD,
									   psImageGroup,
									   FALSE,
									   &bExtremeHit))
				{
					// We've got a new frame!
					bNewFrame = TRUE;
				}
			}
			else
			{
				// WTF Is wrong with the ping pong direction?
				GCASSERT(0);
			}

			// If we've hit an extreme, time to reverse
			if (bExtremeHit)
			{
				if (1 == psImageGroup->s8PingPongDirection)
				{
					psImageGroup->s8PingPongDirection = -1;
				}
				else
				if (-1 == psImageGroup->s8PingPongDirection)
				{
					psImageGroup->s8PingPongDirection = 1;
				} 
				else
				{
					GCASSERT(0);
				}

				return(GfxAnimStepAdvance(EDIR_PINGPONG | (bRepeat ? EDIR_REPEAT : 0),
										  psImageGroup,
										  FALSE,
										  pbExtremeHit));
			}
			else
			{
				// One less frame to worry about
				psImageGroup->u32PingPongFramesRemaining--;
			}
		}
		else
		{
			// No frames remaining. If there's no repeat, we don't reload.
			if (bRepeat)
			{
				psImageGroup->s8PingPongDirection = 1;
				psImageGroup->u32PingPongFramesRemaining = (psImageGroup->u32FrameCount << 1) - 2;

				return(GfxAnimStepAdvance(EDIR_PINGPONG | (bRepeat ? EDIR_REPEAT : 0),
										  psImageGroup,
										  FALSE,
										  pbExtremeHit));
			}
		}
	}
	else
	{
		// WTF? Something wrong with our direction type
		GCASSERT(0);
	}

	return(bNewFrame);
}

static BOOL GfxAnimTimeAdvance(EPlaybackDir eDirActive,
							   SImageGroup *psImageGroup,
							   UINT32 u32TickTime,
							   BOOL *pbExtremeHit)
{
	EPlaybackDir eDir = eDirActive & ~(EDIR_RUNNING | EDIR_REPEAT);
	BOOL bRepeat = FALSE;
	BOOL bRunning = FALSE;
	BOOL bAdvance = FALSE;
	
	if (eDirActive & EDIR_RUNNING)
	{
		bRunning = TRUE;
	}

	// If we're not moving, then just return
	if ((0 == u32TickTime) || (FALSE == bRunning))
	{
		return(FALSE);
	}

	if (eDirActive & EDIR_REPEAT)
	{
		bRepeat = TRUE;
	}

	if (pbExtremeHit)
	{
		*pbExtremeHit = FALSE;
	}

	if (EDIR_FORWARD == eDir)
	{
		// Moving forward in time
		psImageGroup->u32Accumulator += u32TickTime;

		// Skip through frames until we've either hit the end of the frame list or
		// hit a frame that's timestamped higher than the current timestamp
		while (psImageGroup->psCurrentImage && (psImageGroup->u32Accumulator > psImageGroup->psCurrentImage->u32MSTimestamp))
		{
			psImageGroup->psCurrentImage = psImageGroup->psCurrentImage->psNextLink;
			bAdvance = TRUE;
		}

		if (NULL == psImageGroup->psCurrentImage)
		{
			bAdvance = FALSE;

			if (pbExtremeHit)
			{
				*pbExtremeHit = TRUE;
			}

			if (bRepeat)
			{
				psImageGroup->u32Accumulator -= psImageGroup->u32TotalAnimTime;
			}
			else
			{
				// Need to adjust the backup time
				psImageGroup->u32Accumulator -= (u32TickTime << 1);
			}

			if (bRepeat)
			{
				bAdvance = TRUE;

				// Back to the beginning
				psImageGroup->psCurrentImage = psImageGroup->psLinkHead->psImage;

				// Advance to the first image at this timestamp
				while (psImageGroup->psCurrentImage && (psImageGroup->u32Accumulator > psImageGroup->psCurrentImage->u32MSTimestamp))
				{
					psImageGroup->psCurrentImage = psImageGroup->psCurrentImage->psNextLink;
				}
			}
			else
			{
				// We're not repeating. Stop it.
				if (bRepeat)
				{
					psImageGroup->eActivePlaybackType = psImageGroup->eActivePlaybackType & ~EDIR_RUNNING;
				}

				GCASSERT(psImageGroup->psLinkHead);
				if (bRepeat)
				{
					psImageGroup->psCurrentImage = psImageGroup->psLinkHead->psImage;

					// Stick on the last frame
					bAdvance = FALSE;
				}
				else
				{
					// If we're not repeating, point to the LAST frame
					psImageGroup->psCurrentImage = psImageGroup->psLinkTail->psImage;

					// Adjust our timing to the frame matching the accumulator
					while (psImageGroup->psCurrentImage && 
						   (psImageGroup->u32Accumulator < (psImageGroup->psCurrentImage->u32MSTimestamp - psImageGroup->psCurrentImage->u32FrameTime)))
					{
						GCASSERT(psImageGroup->psCurrentImage);
						psImageGroup->psCurrentImage = psImageGroup->psCurrentImage->psPriorLink;
						GCASSERT(psImageGroup->psCurrentImage);
						bAdvance = TRUE;
					}
				}

				GCASSERT(psImageGroup->psCurrentImage);
			}
		}
	}
	else
	if (EDIR_BACKWARD == eDir)
	{
		// Moving backward in time
		psImageGroup->u32Accumulator -= u32TickTime;

		// Special case 
		if (psImageGroup->u32Accumulator > psImageGroup->u32TotalAnimTime)
		{
			psImageGroup->psCurrentImage = NULL;
		}

		// If we've not gotten an image, start at the end!
		if (psImageGroup->psCurrentImage)
		{
			// Back up a frame if necessary
			while (psImageGroup->psCurrentImage && 
				   (psImageGroup->u32Accumulator < (psImageGroup->psCurrentImage->u32MSTimestamp - psImageGroup->psCurrentImage->u32FrameTime)))
			{
				GCASSERT(psImageGroup->psCurrentImage);
				psImageGroup->psCurrentImage = psImageGroup->psCurrentImage->psPriorLink;
				GCASSERT(psImageGroup->psCurrentImage);
				bAdvance = TRUE;
			}
		}

		// If we've hit the beginning, then we need to see what we need to do
		if (NULL == psImageGroup->psCurrentImage)
		{
			bAdvance = FALSE;

			if (pbExtremeHit)
			{
				*pbExtremeHit = TRUE;
			}

			// Link to the end
			psImageGroup->psCurrentImage = psImageGroup->psLinkTail->psImage;

			if (bRepeat)
			{
				psImageGroup->u32Accumulator += psImageGroup->u32TotalAnimTime;
			}
			else
			{
				psImageGroup->u32Accumulator += u32TickTime;
			}

			// Back up a frame if necessary
			while (psImageGroup->psCurrentImage && 
				   (psImageGroup->u32Accumulator < (psImageGroup->psCurrentImage->u32MSTimestamp - psImageGroup->psCurrentImage->u32FrameTime)))
			{
				GCASSERT(psImageGroup->psCurrentImage);
				psImageGroup->psCurrentImage = psImageGroup->psCurrentImage->psPriorLink;
				GCASSERT(psImageGroup->psCurrentImage);
				bAdvance = TRUE;
			}

			if (bRepeat)
			{
				// Update the display
				bAdvance = TRUE;
			}
			else
			{
				// We're not repeating. Stop it.
				if (bRepeat)
				{
					psImageGroup->eActivePlaybackType = psImageGroup->eActivePlaybackType & ~EDIR_RUNNING;
				}

				GCASSERT(psImageGroup->psLinkTail);
				if (bRepeat)
				{
					psImageGroup->psCurrentImage = psImageGroup->psLinkTail->psImage;
				}
				else
				{
					// If we're not repeating, point to the FIRST frame
					psImageGroup->psCurrentImage = psImageGroup->psLinkHead->psImage;
				}

				GCASSERT(psImageGroup->psCurrentImage);
			}
		}
	}
	else
	if (EDIR_PINGPONG == eDir)
	{
		BOOL bSwitch = FALSE;

//		DebugOut("%s: Incoming Frame #=%u (remaining=%u) (time=%u) (dir=%d)\n", __FUNCTION__, psImageGroup->psCurrentImage->u32FrameNumber, psImageGroup->u32PingPongFramesRemaining, psImageGroup->u32Accumulator, psImageGroup->s8PingPongDirection);

		if (0 == psImageGroup->u32PingPongFramesRemaining)
		{
			BOOL bTerminated = FALSE;

			if (psImageGroup->u32Accumulator)
			{
				// We've still got time on the clock!
				if (psImageGroup->u32Accumulator < u32TickTime)
				{
					// Subtract out the delta between our tick time and the accumulator so we have a remainder in u32TickTime
					// should the pingpong algorithm need the overflow
					u32TickTime -= psImageGroup->u32Accumulator;
					psImageGroup->u32Accumulator = 0;
					bTerminated = TRUE;
				}
				else
				{
					psImageGroup->u32Accumulator -= u32TickTime;
				}
			}
			else
			{
				// No frames remaining, no accumulator remaining. We're done.
				bTerminated = TRUE;
			}

			if (bTerminated)
			{
				if (bRepeat)
				{
					// Start it all over again
					psImageGroup->s8PingPongDirection = 1;
					psImageGroup->u32PingPongFramesRemaining = (psImageGroup->u32FrameCount << 1) - 2;

					// Zero out the accumulator. We'll be passing in the remaining time to the subordinate/recursive
					// call if there's any leftover.
					psImageGroup->u32Accumulator = 0;
				}
				else
				{
					// Stop the pingpong
					psImageGroup->eActivePlaybackType &= ~EDIR_RUNNING;
					bAdvance = FALSE;
					psImageGroup->s8PingPongDirection = 0;
					psImageGroup->u32Accumulator = 0;
				}
			}
			else
			{
				// Not advancing. Almost out of time 
				return(FALSE);
			}
		}

		if (1 == psImageGroup->s8PingPongDirection)
		{

			// Step forward in time, but only subtract a frame if we've actually changed it
			if (GfxAnimTimeAdvance(EDIR_FORWARD | EDIR_RUNNING,
								   psImageGroup,
								   u32TickTime,
								   &bSwitch))
			{
				bAdvance = TRUE;
				psImageGroup->u32PingPongFramesRemaining--;
			}

			// We've hit an extreme. Go backwards.
			if (bSwitch)
			{
				psImageGroup->s8PingPongDirection = -1;
			}
		}
		else
		if (-1 == psImageGroup->s8PingPongDirection)
		{
			// Step backward in time, but only subtract a frame if we've actually changed it
			if (GfxAnimTimeAdvance(EDIR_BACKWARD | EDIR_RUNNING,
								   psImageGroup,
								   u32TickTime,
								   &bSwitch))
			{
				bAdvance = TRUE;
				psImageGroup->u32PingPongFramesRemaining--;
			}

			// If we've hit an extreme. Go forwards.
			if (bSwitch)
			{
				psImageGroup->s8PingPongDirection = 1;
			}
		}

		// If we've had a directional switch, kick it back in the other direction immediately
		if (bSwitch)
		{
			return(GfxAnimTimeAdvance(EDIR_PINGPONG | EDIR_RUNNING,
									  psImageGroup,
									  u32TickTime,
									  NULL));
		}

//		if (bAdvance)
		{
//			DebugOut("%s: Frame #=%u          (remaining=%u) (time=%u) (dir=%d)\n", __FUNCTION__, psImageGroup->psCurrentImage->u32FrameNumber, psImageGroup->u32PingPongFramesRemaining, psImageGroup->u32Accumulator, psImageGroup->s8PingPongDirection);
		}
	}
	else
	{
		GCASSERT(0);
	}

	return(bAdvance);
}

BOOL GfxAnimAdvance(SImageGroup *psImageGroup,
					UINT32 u32TickTime)
{
	BOOL bRetCode = FALSE;

	// First handle any stepping that we need
	while (psImageGroup->u32StepCount)
	{
		if (GfxAnimStepAdvance(psImageGroup->eActivePlaybackType,
							   psImageGroup,
							   FALSE,
							   NULL))
		{
			bRetCode = TRUE;
		}

		psImageGroup->u32StepCount--;
//		DebugOut("Decremented to %u\n", psImageGroup->u32StepCount);
	}

	// Now advance in time if we have any
	if (GfxAnimTimeAdvance(psImageGroup->eActivePlaybackType,
						   psImageGroup,
						   u32TickTime,
						   NULL))
	{
		bRetCode = TRUE;
	}

	// If there's a pending refresh, redraw it as-is
	if (psImageGroup->bPendingRefresh)
	{
		bRetCode = TRUE;
		psImageGroup->bPendingRefresh = FALSE;
	}

	return(bRetCode);
}

void GfxAnimGetActiveType(SImageGroup *psImageGroup,
						  EPlaybackDir *peDir,
						  BOOL *pbRepeat)
{
	if (peDir)
	{
		*peDir = psImageGroup->eActivePlaybackType & ~(EDIR_REPEAT | EDIR_RUNNING);
	}

	if (pbRepeat)
	{
		*pbRepeat = FALSE;
		if (psImageGroup->eActivePlaybackType & EDIR_REPEAT)
		{
			*pbRepeat = TRUE;
		}
	}
}

void GfxAnimSetActiveType(SImageGroup *psImageGroup,
						  EPlaybackDir eDir)
{
	UINT32 u32RepeatRunning = psImageGroup->eActivePlaybackType & (EDIR_REPEAT | EDIR_RUNNING);

	eDir &= ~(EDIR_REPEAT | EDIR_RUNNING);

	if (eDir == EDIR_PINGPONG)
	{
		psImageGroup->s8PingPongDirection = 1;
		psImageGroup->u32PingPongFramesRemaining = (psImageGroup->u32FrameCount << 1) - 2;
		psImageGroup->eActivePlaybackType = eDir;
	}
	else
	if (eDir == EDIR_FORWARD)
	{
		psImageGroup->eActivePlaybackType = eDir;
	}
	else
	if (eDir == EDIR_BACKWARD)
	{
		psImageGroup->eActivePlaybackType = eDir;
	}
	else
	{
		GCASSERT(0);
	}

	psImageGroup->eActivePlaybackType = (EPlaybackDir) (eDir | u32RepeatRunning);
}

void GfxAnimSetActiveRepeat(SImageGroup *psImageGroup,
							BOOL bRepeat)
{
	psImageGroup->eActivePlaybackType &= ~EDIR_REPEAT;
	if (bRepeat)
	{
		psImageGroup->eActivePlaybackType |= EDIR_REPEAT;
	}
}

void GfxAnimSetActiveRunning(SImageGroup *psImageGroup,
							BOOL bRunning)
{
	psImageGroup->eActivePlaybackType &= ~EDIR_RUNNING;
	if (bRunning)
	{
		psImageGroup->eActivePlaybackType |= EDIR_RUNNING;
	}
}

void GfxAnimGetFrameNumber(SImageGroup *psImageGroup,
						   UINT32 *pu32FrameNumber)
{
	if (pu32FrameNumber)
	{
		if (psImageGroup->psCurrentImage)
		{
			*pu32FrameNumber = psImageGroup->psCurrentImage->u32FrameNumber;
		}
	}
}

BOOL GfxAnimSetFrameNumber(SImageGroup *psGroup,
						   UINT32 u32FrameNumber)
{
	SImageGroupLink *psCurrentImage = psGroup->psLinkHead;

	while ((u32FrameNumber) && (psCurrentImage))
	{
		psCurrentImage = psCurrentImage->psNextLink;
		--u32FrameNumber;
	}

	if (psCurrentImage)
	{
		psGroup->psCurrentImage = psCurrentImage->psImage;
		return(TRUE);
	}
	else
	{
		return(FALSE);
	}
}

void GfxAnimStart(SImageGroup *psImageGroup)
{
	psImageGroup->eActivePlaybackType |= EDIR_RUNNING;
}

void GfxAnimStop(SImageGroup *psImageGroup)
{
	psImageGroup->eActivePlaybackType &= ~EDIR_RUNNING;
}

void GfxAnimStep(SImageGroup *psImageGroup)
{
	psImageGroup->u32StepCount++;
//	DebugOut("Incremented to %u\n", psImageGroup->u32StepCount);
}

EGCResultCode GfxRotateImage(SImage **ppsImage,
							 ERotation eRot)
{
	SImage *psImage = NULL;

	if ((NULL == ppsImage) ||
		(NULL == *ppsImage))
	{
		return(GC_OK);
	}

	// If it's ROT 90 or 270, allocate a new image
	if ((ROT_90 == eRot) ||
		(ROT_270 == eRot))
	{
		UINT32 u32XSize;
		UINT32 u32YSize;
		UINT8 u8BPP;

		if ((*ppsImage)->pu8ImageData)
		{
			u8BPP = 8;
		}
		else
		{
			u8BPP = 16;
		}

		u32XSize = ((*ppsImage)->u32YSize + 3) & 0xfffffffc;
		u32YSize = (*ppsImage)->u32XSize;

		psImage = GfxCreateEmptyImage(u32XSize,
									  u32YSize,
									  u8BPP,
									  0,
									  FALSE);

		if (NULL == psImage)
		{
			return(GC_OUT_OF_MEMORY);
		}

		psImage->u32XSize = (*ppsImage)->u32YSize;
		psImage->u32YSize = (*ppsImage)->u32XSize;

		if ((*ppsImage)->pu8TranslucentMask ||
			(*ppsImage)->pu8Transparent)
		{
			UINT8 *pu8Mask;

			pu8Mask = MemAlloc(u32XSize * u32YSize);
			if (NULL == pu8Mask)
			{
				GfxDeleteImage(psImage);
				return(GC_OUT_OF_MEMORY);
			}

			if ((*ppsImage)->pu8TranslucentMask)
			{
				psImage->pu8TranslucentMask = pu8Mask;
			}
			else
			{
				psImage->pu8Transparent = pu8Mask;
			}
		}

		if ((*ppsImage)->pu16Palette)
		{
			psImage->pu16Palette = MemAlloc(sizeof(*psImage->pu16Palette) * (1 << 8));
			if (NULL == psImage->pu16Palette)
			{
				GfxDeleteImage(psImage);
				return(GC_OUT_OF_MEMORY);
			}

			memcpy((void *) psImage->pu16Palette, (*ppsImage)->pu16Palette, sizeof(*psImage->pu16Palette) * (1 << 8));
		}
	}

	if (ROT_0 == eRot)
	{
		return(GC_OK);
	}
	else
	if (ROT_90 == eRot)
	{
		UINT32 u32YCount = 0;
		UINT32 u32XCount = 0;
		UINT8 *pu8ImgSrc = NULL;
		UINT8 *pu8ImgDest = NULL;
		UINT16 *pu16ImgSrc = NULL;
		UINT16 *pu16ImgDest = NULL;
		UINT8 *pu8MaskSrc = NULL;
		UINT8 *pu8MaskDest = NULL;

		pu8MaskSrc = (*ppsImage)->pu8TranslucentMask;
		if (NULL == pu8MaskSrc)
		{
			pu8MaskSrc = (*ppsImage)->pu8Transparent;
		}

		if ((*ppsImage)->pu8ImageData)
		{
			// Source is upper left hand corner
			pu8ImgSrc = (*ppsImage)->pu8ImageData;

			// 90 degree rotation == upper right hand corner
			pu8ImgDest = psImage->pu8ImageData + (psImage->u32XSize - 1);
		}
		else
		{
			pu16ImgSrc = (*ppsImage)->pu16ImageData;
			pu16ImgDest = psImage->pu16ImageData + (psImage->u32XSize - 1);
		}

		pu8MaskDest = psImage->pu8TranslucentMask;
		if (NULL == pu8MaskDest)
		{
			pu8MaskDest = psImage->pu8Transparent;
		}

		if (pu8MaskDest)
		{
			pu8MaskDest = pu8MaskDest + (psImage->u32XSize - 1);
		}

		u32YCount = (*ppsImage)->u32YSize;
		while (u32YCount--)
		{
			// For 90 degree rotation, we're scanning the source from left/right and
			// the target from top/bottom

			u32XCount = psImage->u32YSize;

			if (pu8ImgSrc)
			{
				while (u32XCount--)
				{
					*pu8ImgDest = *pu8ImgSrc;
					++pu8ImgSrc;
					pu8ImgDest += psImage->u32Pitch;
				}

				pu8ImgSrc += ((*ppsImage)->u32Pitch - (*ppsImage)->u32XSize);
				pu8ImgDest -= ((psImage->u32Pitch * psImage->u32YSize) + 1);
			}
			else
			{
				// 16BPP image
				while (u32XCount--)
				{
					*pu16ImgDest = *pu16ImgSrc;
					++pu16ImgSrc;
					pu16ImgDest += psImage->u32Pitch;
				}

				pu16ImgSrc += ((*ppsImage)->u32Pitch - (*ppsImage)->u32XSize);
				pu16ImgDest -= ((psImage->u32Pitch * psImage->u32YSize) + 1);
			}

			if (pu8MaskSrc)
			{
				u32XCount = (*ppsImage)->u32XSize;

				while (u32XCount--)
				{
					*pu8MaskDest = *pu8MaskSrc;
					++pu8MaskSrc;
					pu8MaskDest += psImage->u32Pitch;
				}

				pu8MaskSrc += ((*ppsImage)->u32Pitch - (*ppsImage)->u32XSize);
				pu8MaskDest -= ((psImage->u32Pitch * psImage->u32YSize) + 1);
			}
		}
	}
	else
	if (ROT_180 == eRot)
	{
		UINT32 u32YCount = 0;
		UINT32 u32XCount = 0;
		UINT8 *pu8ImgSrc = NULL;
		UINT8 *pu8ImgDest = NULL;
		UINT16 *pu16ImgSrc = NULL;
		UINT16 *pu16ImgDest = NULL;
		UINT8 *pu8MaskSrc = NULL;
		UINT8 *pu8MaskDest = NULL;

		pu8MaskSrc = (*ppsImage)->pu8TranslucentMask;
		if (NULL == pu8MaskSrc)
		{
			pu8MaskSrc = (*ppsImage)->pu8Transparent;
		}

		u32YCount = ((*ppsImage)->u32YSize + 1) >> 1;

		// 8BPP image. Swap it in place.
		if ((*ppsImage)->pu8ImageData)
		{
			pu8ImgSrc = (*ppsImage)->pu8ImageData;
			pu8ImgDest = pu8ImgSrc + (((*ppsImage)->u32YSize - 1) * (*ppsImage)->u32Pitch) + ((*ppsImage)->u32XSize - 1);
		}
		else
		{
			pu16ImgSrc = (*ppsImage)->pu16ImageData;
			pu16ImgDest = pu16ImgSrc + (((*ppsImage)->u32YSize - 1) * (*ppsImage)->u32Pitch) + ((*ppsImage)->u32XSize - 1);
		}

		if (pu8MaskSrc)
		{
			pu8MaskDest = pu8MaskSrc + ((((*ppsImage)->u32YSize - 1) * (*ppsImage)->u32Pitch) + ((*ppsImage)->u32XSize - 1));
		}

		while (u32YCount--)
		{
			UINT32 u32XCountOriginal = 0;

			u32XCountOriginal = (*ppsImage)->u32XSize;
			if ((pu8ImgSrc + (*ppsImage)->u32XSize - 1) == pu8ImgDest)
			{
				u32XCountOriginal = (u32XCountOriginal + 1) >> 1;
			}

			// If the image is an odd Y size, then we need to only swap half the horizontal pixels on the middle row of pixels
			if ((0 == u32YCount) && ((*ppsImage)->u32YSize & 1))
			{
				u32XCount = u32XCountOriginal >> 1;
				u32XCountOriginal = u32XCount;
			}
			else
			{
				u32XCount = u32XCountOriginal;
			}

			if (pu8ImgSrc)
			{
				while (u32XCount--)
				{
					UINT8 u8Data;		
	
					u8Data = *pu8ImgSrc;
					*pu8ImgSrc = *pu8ImgDest;
					*pu8ImgDest = u8Data;
					++pu8ImgSrc;
					--pu8ImgDest;
				}

				pu8ImgSrc += ((*ppsImage)->u32Pitch - (*ppsImage)->u32XSize);
				pu8ImgDest -= ((*ppsImage)->u32Pitch - (*ppsImage)->u32XSize);
			}
			else
			{
				// 16bpp image
				while (u32XCount--)
				{
					UINT16 u16Data;		
	
					u16Data = *pu16ImgSrc;
					*pu16ImgSrc = *pu16ImgDest;
					*pu16ImgDest = u16Data;
					++pu16ImgSrc;
					--pu16ImgDest;
				}

				pu16ImgSrc += ((*ppsImage)->u32Pitch - (*ppsImage)->u32XSize);
				pu16ImgDest -= ((*ppsImage)->u32Pitch - (*ppsImage)->u32XSize);
			}

			// If we have a mask, move it
			if (pu8MaskSrc)
			{
				u32XCount = u32XCountOriginal;

				while (u32XCount--)
				{
					UINT8 u8Data;

					u8Data = *pu8MaskSrc;
					*pu8MaskSrc = *pu8MaskDest;
					*pu8MaskDest = u8Data;
					++pu8MaskSrc;
					--pu8MaskDest;
				}

				pu8MaskSrc += ((*ppsImage)->u32Pitch - (*ppsImage)->u32XSize);
				pu8MaskDest -= ((*ppsImage)->u32Pitch - (*ppsImage)->u32XSize);
			}
		}
	}
	else
	if (ROT_270 == eRot)
	{
		UINT32 u32YCount = 0;
		UINT32 u32XCount = 0;
		UINT8 *pu8ImgSrc = NULL;
		UINT8 *pu8ImgDest = NULL;
		UINT16 *pu16ImgSrc = NULL;
		UINT16 *pu16ImgDest = NULL;
		UINT8 *pu8MaskSrc = NULL;
		UINT8 *pu8MaskDest = NULL;

		pu8MaskSrc = (*ppsImage)->pu8TranslucentMask;
		if (NULL == pu8MaskSrc)
		{
			pu8MaskSrc = (*ppsImage)->pu8Transparent;
		}

		if ((*ppsImage)->pu8ImageData)
		{
			// Source is upper left hand corner
			pu8ImgSrc = (*ppsImage)->pu8ImageData;

			// 270 degree rotation == lower left hand corner
			pu8ImgDest = psImage->pu8ImageData + ((psImage->u32YSize - 1) * psImage->u32Pitch);
		}
		else
		{
			pu16ImgSrc = (*ppsImage)->pu16ImageData;
			pu16ImgDest = psImage->pu16ImageData + ((psImage->u32YSize - 1) * psImage->u32Pitch);
		}

		pu8MaskDest = psImage->pu8TranslucentMask;
		if (NULL == pu8MaskDest)
		{
			pu8MaskDest = psImage->pu8Transparent;
		}

		if (pu8MaskDest)
		{
			pu8MaskDest = pu8MaskDest + ((psImage->u32YSize - 1) * psImage->u32Pitch);
		}

		u32YCount = (*ppsImage)->u32YSize;
		while (u32YCount--)
		{
			// For 270 degree rotation, we're scanning the source from left/right and
			// the target from bottom/top

			u32XCount = psImage->u32YSize;

			if (pu8ImgSrc)
			{
				while (u32XCount--)
				{
					*pu8ImgDest = *pu8ImgSrc;
					++pu8ImgSrc;
					pu8ImgDest -= psImage->u32Pitch;
				}

				pu8ImgSrc += ((*ppsImage)->u32Pitch - (*ppsImage)->u32XSize);
				pu8ImgDest += ((psImage->u32Pitch * psImage->u32YSize) + 1);
			}
			else
			{
				// 16BPP image
				while (u32XCount--)
				{
					*pu16ImgDest = *pu16ImgSrc;
					++pu16ImgSrc;
					pu16ImgDest -= psImage->u32Pitch;
				}

				pu16ImgSrc += ((*ppsImage)->u32Pitch - (*ppsImage)->u32XSize);
				pu16ImgDest += ((psImage->u32Pitch * psImage->u32YSize) + 1);
			}

			if (pu8MaskSrc)
			{
				u32XCount = (*ppsImage)->u32XSize;

				while (u32XCount--)
				{
					*pu8MaskDest = *pu8MaskSrc;
					++pu8MaskSrc;
					pu8MaskDest -= psImage->u32Pitch;
				}

				pu8MaskSrc += ((*ppsImage)->u32Pitch - (*ppsImage)->u32XSize);
				pu8MaskDest += ((psImage->u32Pitch * psImage->u32YSize) + 1);
			}
		}
	}
	else
	{
		// Unknown rotation
		GCASSERT(0);
	}

	if (psImage)
	{
		GfxDeleteImage((*ppsImage));
		(*ppsImage) = psImage;
	}

	return(GC_OK);
}

EGCResultCode GfxRotateImageGroup(SImageGroup *psImageGroup,
								  ERotation eRot)
{
	EGCResultCode eResult;
	SImageGroupLink *psImageLink;

	psImageLink = psImageGroup->psLinkHead;

	while (psImageLink)
	{
		SImage *psOldImage;

		psOldImage = psImageLink->psImage;

		eResult = GfxRotateImage(&psImageLink->psImage,
								 eRot);

		if (eResult != GC_OK)
		{
			return(eResult);
		}

		if (psImageGroup->psCurrentImage == psOldImage)
		{
			psImageGroup->psCurrentImage = psImageLink->psImage;
		}

		psImageLink = psImageLink->psNextLink;
	}

	return(eResult);
}

BOOL GfxFree(BOOL bClearAll)
{
	SGfxFile *psPriorPtr = NULL;
	SGfxFile *psTempPtr = NULL;
	SGfxFile *psPtr = NULL;
	SGfxFile *psPrior = NULL;

	GfxLockFileList();
	psTempPtr = sg_psFileHead;

	bClearAll = FALSE;

	// Find the last file in the list

	while (psTempPtr)
	{
		if (0 == psTempPtr->u32References)
		{
			psPtr = psTempPtr;
			psPrior = psPriorPtr;
		}

		psPriorPtr = psTempPtr;
		psTempPtr = psTempPtr->psNextLink;
	}

	// If psPtr is NULL here and we want to clear everything, then run through and find any node

	if ((NULL == psPtr) && (bClearAll))
	{
		psPrior = NULL;
		psPtr = sg_psFileHead;
	}

	if (psPtr)
	{
		SImageGroupLink *psGrp;
		SImageGroupLink *psGrpPrior;

//		DebugOut("%s: Freeing '%s'", __FUNCTION__, psPtr->pu8GraphicsFilename);
		if (psPtr->psImageGroup)
		{
			if (psPtr->psImageGroup->psGfxFile)
			{
//				DebugOut(" Ref=%u", psPtr->psImageGroup->psGfxFile->u32References);
			}
		}

//		DebugOut("\n");

		// Unlink it from the linked list
		if (NULL == psPrior)
		{
			GCASSERT(psPtr == sg_psFileHead);
			sg_psFileHead = psPtr->psNextLink;
		}
		else
		{
			psPrior->psNextLink = psPtr->psNextLink;
		}

		GfxUnlockFileList();

		// Get rid of all of the images

		psGrp = psPtr->psImageGroup->psLinkHead;

		while (psGrp)
		{
			GfxDeleteImageInternal(psGrp->psImage);
			psGrp->psImage = NULL;
			psGrpPrior = psGrp;
			psGrp = psGrp->psNextLink;
			GCFreeMemory(psGrpPrior);
		}

		// Get rid of the image group
		GCFreeMemory(psPtr->psImageGroup);
		psPtr->psImageGroup = NULL;

		// Get rid of the filename
		GCFreeMemory(psPtr->peGraphicsFilename);

		// Now free the SGfxFile structure
		GCFreeMemory(psPtr);

		return(TRUE);
	}
	else
	{
		GfxUnlockFileList();
		// Nothing we can evict. ;-(
		return(FALSE);
	}
}


#define GET_PIXEL_COLOR_U16(value, mask, offset)     (UINT16)(((value) >> (offset)) & (mask))
#define SET_PIXEL_COLOR_U16(value, mask, offset)     (UINT16)(((value) & (mask)) << (offset))

#define GET_RED_565_U16(value)                       GET_PIXEL_COLOR_U16(value, 0x1f, 11)
#define GET_GREEN_565_U16(value)                     GET_PIXEL_COLOR_U16(value, 0x3f, 5)
#define GET_BLUE_565_U16(value)                      GET_PIXEL_COLOR_U16(value, 0x1f, 0)

#define SET_RED_565_U16(value)                       SET_PIXEL_COLOR_U16(value, 0x1f, 11)
#define SET_GREEN_565_U16(value)                     SET_PIXEL_COLOR_U16(value, 0x3f, 5)
#define SET_BLUE_565_U16(value)                      SET_PIXEL_COLOR_U16(value, 0x1f, 0)

#define PIXEL_AVG_16BPP(x,y)        (   SET_RED_565_U16((GET_RED_565_U16(x) + GET_RED_565_U16(y)) >> 1) | \
                                        SET_GREEN_565_U16((GET_GREEN_565_U16(x) + GET_GREEN_565_U16(y)) >> 1) | \
                                        SET_BLUE_565_U16((GET_BLUE_565_U16(x) + GET_BLUE_565_U16(y)) >> 1)  )


// Implement a smooth bresenham per line
static void ResizeBresenhamRow( UINT8* pu8Src, UINT32 u32SrcX,
								UINT8* pu8Dst, UINT32 u32DstX )
{
	UINT32 u32Columns = u32DstX;
	UINT32 u32Error = 0;
	UINT32 u32ErrorThreshold = u32DstX / 2;
	UINT16 u16Pixel;

	// Don't average past the end of the row when zooming
	if( u32DstX > u32SrcX )
	{
		u32Columns--;
	}

	while( u32Columns )
	{
		// Decide how to handle the current pixel in the destination (copy or average)
		u16Pixel = *(UINT16*)pu8Src;

        if( u32Error > u32ErrorThreshold )
        {
            UINT16 u16Pixel2 = *(UINT16*)(pu8Src+2);

            // AVERAGE of this pixel and the next
            u16Pixel = PIXEL_AVG_16BPP(u16Pixel, u16Pixel2);
        }

		*(UINT16*)pu8Dst = u16Pixel;
		pu8Dst += 2;

		// Accumulate the error
		u32Error += u32SrcX;

		// If it's overflowed, move onto the next source pixel
		while( u32Error >= u32DstX )
		{
			u32Error -= u32DstX;
			pu8Src += 2;
		}

		u32Columns--;
	}

	// If zooming, at the end of the row, just copy the pixel
	if( u32DstX > u32SrcX )
	{
		*(UINT16*)pu8Dst = *(UINT16*)pu8Src;
	}
}


// Assumption: This resize is only for shrinking images.  Enhancements required for growing it.
static UINT8* ResizeBresenham( UINT8* pu8Source, UINT32 u32SrcX, UINT32 u32SrcY,
							   UINT8* pu8Destination, UINT32 u32DstX, UINT32 u32DstY )
{
	UINT8* pu8Src = pu8Source;
    UINT8* pu8DstRow = pu8Destination;
    UINT8* pu8Dst = NULL;
	UINT8* pu8RowBuffer = NULL;

	UINT32 u32Rows = u32DstY;
	UINT32 u32Error = 0;
	UINT32 u32ErrorThreshold = u32DstY / 2;
	UINT16 u16Pixel;
//  UINT32 u32DstPitch = ((u32DstX * 2) + 3) & ~3;      ???????
    UINT32 u32DstPitch = (u32DstX * 2);
    UINT32 u32SrcPitch = ((u32SrcX * 2) + 7) & ~7;

	GCASSERT(u32DstX > 1);
	GCASSERT(u32DstY > 1);

	// Don't average past the end of the row when zooming
	if( u32DstY > u32SrcY )
	{
		u32Rows--;
	}

	// Make a row buffer
	pu8RowBuffer = (UINT8*)GCAllocateMemoryNoClear( u32DstX * u32DstY * 2);

	while( u32Rows )
	{
		UINT32 u32Columns = u32DstX;
		UINT8* pu8RowBufferSrc = pu8RowBuffer;

        pu8Dst = pu8DstRow;
		
		// Convert this row
		ResizeBresenhamRow( pu8Src, u32SrcX,
							pu8Dst, u32DstX );	

		// Fill the buffer with the next row
		ResizeBresenhamRow( (pu8Src + u32SrcPitch), u32SrcX,
							pu8RowBuffer, u32DstX );

		while( u32Columns )
		{
			// Decide whether to average with the pixel in the next row
			u16Pixel = *(UINT16*)pu8Dst;

            if( u32Error > u32ErrorThreshold )
            {
                UINT16 u16Pixel2 = *(UINT16*)pu8RowBufferSrc;

                // AVERAGE of this pixel and the next
                u16Pixel = PIXEL_AVG_16BPP(u16Pixel, u16Pixel2);
            }

			*(UINT16*)pu8Dst = u16Pixel;
			pu8Dst += 2;
			pu8RowBufferSrc += 2;

			u32Columns--;
		}
		
		// Accumulate the error
		u32Error += u32SrcY;
		
		// If it's overflowed, move onto the next source row
		while( u32Error >= u32DstY )
		{
			u32Error -= u32DstY;
			pu8Src += u32SrcPitch;
		}
	
        pu8DstRow += u32DstPitch;
		u32Rows--;
	}
	
	// If zooming, at the last row, just convert it
	if( u32DstY > u32SrcY )
	{
		ResizeBresenhamRow( pu8Src, u32SrcX,
							pu8Dst, u32DstX );	
	}

	GCFreeMemory(pu8RowBuffer);
	pu8RowBuffer = NULL;

	return(pu8Destination);
}


SImage* GfxResizeImage( SImage* psSourceImage, UINT32 u32TargetXSize, UINT32 u32TargetYSize )
{
	SImage* psNewImage = NULL;
	UINT8* pu8NewImageData = NULL;
	UINT8* pu8SrcImageData = NULL;
	UINT8* pu8ResizedImageData = NULL;

	UINT8 u8BPP;

	UINT32 u32SrcX, u32SrcY;
	UINT32 u32TmpX, u32TmpY;
	
	if( psSourceImage->pu8ImageData )
	{
		// Palletized data not currently supported
		DebugOut("GfxResizeImage: 8 bit image format not currently supported.\n");
		u8BPP = 8;
		pu8SrcImageData = psSourceImage->pu8ImageData;
		return(NULL);
	}
	else
	{
		u8BPP = 16;
		pu8SrcImageData = (UINT8*)psSourceImage->pu16ImageData;
	}
	GCASSERT(pu8SrcImageData);

	// Get the original image data
	u32SrcX = psSourceImage->u32XSize;
	u32SrcY = psSourceImage->u32YSize;

	// Allocate space for the final target image
	psNewImage = GfxCreateEmptyImage(u32TargetXSize, u32TargetYSize, u8BPP, 0, FALSE);
	GCASSERT(psNewImage);
	if( psNewImage->pu8ImageData )
	{
		pu8NewImageData = psNewImage->pu8ImageData;
	}
	else
	{
		pu8NewImageData = (UINT8*)psNewImage->pu16ImageData;
	}
	GCASSERT(pu8NewImageData);

	u32TmpX = u32SrcX;
	u32TmpY = u32SrcY;
	
    pu8ResizedImageData = ResizeBresenham( pu8SrcImageData, u32TmpX, u32TmpY,
                                           pu8NewImageData, u32TargetXSize, u32TargetYSize );

	if( NULL == pu8ResizedImageData )
	{
		DebugOut("GfxResizeImage: resize operation failed.\n");
		GCASSERT(0);
	}
	
	return(psNewImage);
}

