#include "Startup/app.h"
#include "Application/LCDErr.h"
#include "Libs/Gfx/GraphicsLib.h"
#include "Libs/FontMgr/FontMgr.h"
#include "Libs/window/window.h"
#include "Libs/freetype2/include/freetype/ftglyph.h"

#define MAX_FONTS				128

// Pointer to head of window list
static SFont *sg_psFontList[MAX_FONTS];

// 256 Levels of gray
static UINT16 sg_u16GrayPalette[0x100];

// Global init for font library
static FT_Library sg_sFontLibrary;

// Pointer to all font files currently in memory
static SFontFile *sg_psFontFileListHead = NULL;

// Pointer to all fonts currently in memory
static SFont *sg_psFontListHead = NULL;

static SOSSemaphore sg_sFontLock;

static void FontListLock(void)
{
	EGCResultCode eResult;

	eResult = GCOSSemaphoreGet(sg_sFontLock,
							   0);
	GCASSERT((GC_OK == eResult) || (GC_OS_NOT_RUNNING == eResult));
}

static void FontListUnlock(void)
{
	EGCResultCode eResult;

	eResult = GCOSSemaphorePut(sg_sFontLock);
	GCASSERT((GC_OK == eResult) || (GC_OS_NOT_RUNNING == eResult));
}

static ELCDErr FontFileLoad(LEX_CHAR *peFilename,
							SFontFile **ppsFontFilePtr)
{
	SFontFile *psFontFilePtr;
	GCFile eFileHandle = (GCFile) NULL;
	ELCDErr eLCDErr = LERR_OK;

	// Let's first run through our list of files and see if. It's already there.

	FontListLock();
	psFontFilePtr = sg_psFontFileListHead;

	while (psFontFilePtr)
	{
		if (Lexstrcmp(psFontFilePtr->peFontFilename, peFilename) == 0)
		{
			break;
		}

		psFontFilePtr = psFontFilePtr->psNextLink;
	}

	// If this is true, then we didn't find it
	if (NULL == psFontFilePtr)
	{
		UINT64 u64FileSize = 0;
		EGCResultCode eResult;
		UINT32 u32DataRead = 0;
		char *pu8ASCIIFilename = NULL;

		FontListUnlock();

		// It's new! Let's create a new node.

		psFontFilePtr = MemAlloc(sizeof(*psFontFilePtr));
		if (NULL == psFontFilePtr)
		{
			eLCDErr = LERR_NO_MEM;
			goto notAllocated;
		}

		// Now allocate space for the filename

		psFontFilePtr->peFontFilename = Lexstrdup(peFilename);
		if (NULL == psFontFilePtr->peFontFilename)
		{
			eLCDErr = LERR_NO_MEM;
			goto notAllocated;
		}

		// Now try to get the file's size

		pu8ASCIIFilename = LexUnicodeToASCIIAlloc(peFilename);
		if (NULL == pu8ASCIIFilename)
		{
			eLCDErr = LERR_NO_MEM;
			goto notAllocated;
		}

		eResult = GCFileOpen(&eFileHandle, pu8ASCIIFilename, "rb");
		GCFreeMemory(pu8ASCIIFilename);
		if (eResult != GC_OK)
		{
			eLCDErr = (ELCDErr) (eResult + LERR_GC_ERR_BASE);
			goto notAllocated;
		}

		// Now get the file's size
		eResult = GCFileSizeByHandle(eFileHandle ,&u64FileSize);
		if (eResult != GC_OK)
		{
			eLCDErr = (ELCDErr) (LERR_GC_ERR_BASE + eResult);
			goto notAllocated;
		}

		// If the file is bigger than what can be put in 32 bits, gripe
		if (u64FileSize > 0xfffffffe)
		{
			eLCDErr = LERR_FONT_FILE_TOO_LARGE;
			goto notAllocated;
		}

		psFontFilePtr->u32FileSize = (UINT32) u64FileSize;

		// Now allocate some memory for it
		psFontFilePtr->pvFontFileInMemory = MemAlloc(psFontFilePtr->u32FileSize);
		if (NULL == psFontFilePtr->pvFontFileInMemory)
		{
			eLCDErr = LERR_NO_MEM;
			goto notAllocated;
		}

		// Now read the file in
		eResult = GCFileRead(psFontFilePtr->pvFontFileInMemory, (size_t) psFontFilePtr->u32FileSize, &u32DataRead, eFileHandle);
		if (eResult != GC_OK)
		{
			eLCDErr = (ELCDErr) (eResult + LERR_GC_ERR_BASE);
			goto notAllocated;
		}

		if (u32DataRead != psFontFilePtr->u32FileSize)
		{
			eLCDErr = LERR_FONT_INCOMPLETE_READ;
			goto notAllocated;
		}

		// Got it! Let's close the file
		eResult = GCFileClose(&eFileHandle);
		if (eResult != GC_OK)
		{
			eLCDErr = (ELCDErr) (eResult + LERR_GC_ERR_BASE);
			goto notAllocated;
		}

		// Now let's link this in to our list

		FontListLock();
		psFontFilePtr->psNextLink = sg_psFontFileListHead;
		sg_psFontFileListHead = psFontFilePtr;
	}
	else
	{
	}

	// Now, increase references to this file
	psFontFilePtr->u32References++;

	// Return our font file pointer
	*ppsFontFilePtr = psFontFilePtr;

	// Good read!
	return(LERR_OK);

notAllocated:
	if (eFileHandle != (GCFile) NULL)
	{
		// Close da file!
		(void) GCFileClose(&eFileHandle);
	}

	if (psFontFilePtr)
	{
		if (psFontFilePtr->peFontFilename)
		{
			GCFreeMemory(psFontFilePtr->peFontFilename);
			psFontFilePtr->peFontFilename = NULL;
		}

		if (psFontFilePtr->pvFontFileInMemory)
		{
			GCFreeMemory(psFontFilePtr->pvFontFileInMemory);
			psFontFilePtr->pvFontFileInMemory = NULL;
		}

		GCFreeMemory(psFontFilePtr);
	}

	return(eLCDErr);
}

SFont *FontGetPointer(FONTHANDLE eHandle)
{
	if (eHandle >= (sizeof(sg_psFontList) / sizeof(sg_psFontList[0])))
	{
		return(NULL);
	}

	return(sg_psFontList[eHandle]);
}

static BOOL FontFindFreeHandle(FONTHANDLE *peFontHandle)
{
	UINT32 u32Loop;

	for (u32Loop = 0; u32Loop < (sizeof(sg_psFontList) / sizeof(sg_psFontList[0])); u32Loop++)
	{
		if (NULL == sg_psFontList[u32Loop])
		{
			*peFontHandle = (FONTHANDLE) u32Loop;
			return(TRUE);
		}
	}

	return(FALSE);
}

#define GRAY(x)	GrayValue(x)

static UINT16 GrayValue(UINT16 u16Color)
{
	UINT8 u8Red;
	UINT8 u8Green;
	UINT8 u8Blue;

	u8Red = ((u16Color & 0xf800) * 0x4d) >> 19;
	u8Green = ((u16Color & 0x07e0) * 0x97) >> 13;
	u8Blue = ((u16Color & 0x1f) * 0x1c) >> 9;

	u8Red += (u8Green + u8Blue);
	if (u8Red > 0x3f)
	{
		u8Red = 0x3f;
	}

	return((u8Red << 5) | (u8Red >> 1) | ((u8Red >> 1) << 11));
}

ELCDErr FontCreate(LEX_CHAR *peFilename,
				   UINT32 u32Size,
				   UINT32 u32Options,
				   FONTHANDLE *peFontHandle)
{
	ELCDErr eResult;
	FONTHANDLE eFontHandle;
	SFontFile *psFontFile = NULL;
	SFontCharacter *psFontChar = NULL;
	SFontCharacter *psFontCharPrior = NULL;
	SFont *psFont = NULL;
	FT_Error eFreetypeError = 0;
	UINT32 u32Index = 0;
	UINT32 u32Loop;
	FT_Face eFace;						// Face handle

	// If the font size is too small, it's too small. Bail out.
	if (u32Size < 6)
	{
		return(LERR_FONT_SIZE_TOO_SMALL);
	}

	// And make sure we have a decent filename pointer
	GCASSERT(peFilename);

	// First, let's try to load up the font in question
	eResult = FontFileLoad(peFilename,
						   &psFontFile);

	// No need to unlock if this is taken since there's no lock if there's an error
	RETURN_ON_FAIL(eResult);

	// Font list is now locked

	// Let's see if we can find a font that already has this size. If so, crank up the
	// # of references to it

	psFont = sg_psFontListHead;

	while (psFont)
	{
		if ((psFont->u32PixelSize == u32Size) &&
			(Lexstrcmp(psFont->psFontFile->peFontFilename, peFilename) == 0))
		{
			// Found a match!
			break;
		}

		psFont = psFont->psNextFont;
	}

	// If psFont is non-NULL, here, increase its reference
	if (psFont)
	{
		// If the handle is invalid, we'll need to allocate one
		if (HANDLE_INVALID == psFont->eFontHandle)
		{
			GCASSERT(0 == psFont->u32References);

			// Let's see if we have an available slot
			if (FALSE == FontFindFreeHandle(&psFont->eFontHandle))
			{
				FontListUnlock();
				return(LERR_FONT_SIZE_TOO_SMALL);
			}

			GCASSERT(NULL == sg_psFontList[psFont->eFontHandle]);
			sg_psFontList[psFont->eFontHandle] = psFont;
		}
		else
		{
			GCASSERT(psFont->u32References);
			GCASSERT(psFont->eFontHandle != HANDLE_INVALID);

			// Don't need the file reference
			psFont->psFontFile->u32References--;
		}

		psFont->u32References++;

		*peFontHandle = (FONTHANDLE) psFont->eFontHandle;
		FontListUnlock();
		return(LERR_OK);
	}

	// Gotta unlock, as we're potentially doing an allocation which could end up
	// in a deadlock situation.
	FontListUnlock();

	// Let's see if we have an available slot
	if (FALSE == FontFindFreeHandle(&eFontHandle))
	{
		return(LERR_FONT_FULL);
	}

	// Now that it's loaded, let's create a new SFont structure
	psFont = MemAlloc(sizeof(*psFont));
	if (NULL == psFont)
	{
		eResult = LERR_NO_MEM;
		goto notAllocatedPreFont;
	}

	// Got it. Let's start plugging handy font data in to this structure.
	psFont->psFontFile = psFontFile;
	psFont->u32PixelSize = u32Size;
	psFont->u32References = 1;

	psFont->u32MaxX = 0;
	psFont->u32MinX = 0xffffffff;
	psFont->u32MaxY = 0;
	psFont->u32MinY = 0xffffffff;

	// Go create our face from in-memory fontage
	eFreetypeError = FT_New_Memory_Face(sg_sFontLibrary,
									    psFont->psFontFile->pvFontFileInMemory,
									    psFont->psFontFile->u32FileSize,
									    0,
									    &eFace);

	if (eFreetypeError != FT_Err_Ok)
	{
		eResult = (ELCDErr) (eFreetypeError + LERR_FONT_OK);
		goto notAllocated;
	}

	// Now set the pixel size on the font
	eFreetypeError = FT_Set_Pixel_Sizes(eFace,
										0,					// Pixel width (ignore)
										u32Size);

	if (eFreetypeError != FT_Err_Ok)
	{
		eResult = (ELCDErr) (eFreetypeError + LERR_FONT_OK);
		goto notAllocated;
	}

	// Select the 0th charmap
	eFreetypeError = FT_Select_Charmap(eFace,
									   eFace->charmaps[0]->encoding);

	if (eFreetypeError != FT_Err_Ok)
	{
		eResult = (ELCDErr) (eFreetypeError + LERR_FONT_OK);
		goto notAllocated;
	}

	// Log the kerning support
	if (FT_HAS_KERNING(eFace))
	{
		psFont->bKerning = TRUE;
	}

	// Now let's render each and every character

	for (u32Loop = 0; u32Loop < 0x100; u32Loop++)
	{
		u32Index = FT_Get_Char_Index(eFace,
									 u32Loop);

		if (u32Index)
		{
			eFreetypeError = FT_Load_Glyph(eFace,
										   u32Index,
										   FT_LOAD_DEFAULT);

			if (FT_Err_Ok == eFreetypeError)
			{
				// Cool - got the glyph! Now render it

				eFreetypeError = FT_Render_Glyph(eFace->glyph,
												 FT_RENDER_MODE_NORMAL);

				if (FT_Err_Ok == eFreetypeError)
				{
					// Got it! Let's make some space for the font, bum only
					// allocate 

					if ((eFace->glyph->metrics.horiAdvance >> 6) &&
						(eFace->glyph->metrics.vertAdvance))
					{
						psFontChar = MemAlloc(sizeof(*psFontChar));
						if (NULL == psFontChar)
						{
							eResult = LERR_NO_MEM;
							goto notAllocated;
						}

						// Let's copy over some info - >> 6 Since units are in 1/64ths of a pixel
						psFontChar->u32Character = u32Loop;

						// Round up if it's even slightly over into the next pixel
						psFontChar->u32BitmapXSize = (eFace->glyph->metrics.width + 0x3f) >> 6;
						psFontChar->u32BitmapYSize = (eFace->glyph->metrics.height + 0x3f) >> 6;
						GCASSERT(psFontChar->u32BitmapXSize == eFace->glyph->bitmap.width);
						GCASSERT(psFontChar->u32BitmapYSize == eFace->glyph->bitmap.rows);

						// Kept in 26.6 fixed point
						psFontChar->u32XAdvance = eFace->glyph->advance.x;
						psFontChar->u32YAdvance = eFace->glyph->linearVertAdvance >> 10;
						psFontChar->u32YDescender = abs(eFace->size->metrics.descender);
						psFontChar->u32XBearing = 0;
						psFontChar->u32YBearing = 0;

						if (FT_HAS_HORIZONTAL(eFace))
						{
							if (eFace->glyph->metrics.horiBearingX >= 0)
							{
								psFontChar->u32XBearing = eFace->glyph->metrics.horiBearingX;
							}
							else
							{
								GCASSERT(abs(eFace->glyph->metrics.horiBearingX) <= (INT32) psFontChar->u32XAdvance);
//								psFontChar->u32XAdvance += eFace->glyph->metrics.horiBearingX;
							}

							if (eFace->glyph->metrics.horiBearingY >= 0)
							{
								psFontChar->u32YBearing = (psFontChar->u32YAdvance - eFace->glyph->metrics.horiBearingY);
							}
						}
						else
						if (FT_HAS_VERTICAL(eFace))
						{
							if (eFace->glyph->metrics.vertBearingX >= 0)
							{
								psFontChar->u32XBearing = eFace->glyph->metrics.vertBearingX;
							}

							if (eFace->glyph->metrics.vertBearingY >= 0)
							{
								psFontChar->u32YBearing = eFace->glyph->metrics.vertBearingY;
							}
						}
						else
						{
							GCASSERT_WHY(0, "Font has no vertical or horizontal bearing metrics");
						}

						if ((psFontChar->u32YAdvance + psFontChar->u32YDescender) > (UINT32) psFont->s32Stride)
						{
							psFont->s32Stride = (INT32) (psFontChar->u32YAdvance + psFontChar->u32YDescender);
						}

						if (psFontChar->u32BitmapXSize && psFontChar->u32BitmapYSize)
						{
							psFontChar->pu8FontImageData = MemAlloc(psFontChar->u32BitmapXSize *
																	  psFontChar->u32BitmapYSize);
							if (NULL == psFontChar->pu8FontImageData)
							{
								GCFreeMemory(psFontChar);
								eResult = LERR_NO_MEM;
								goto notAllocated;
							}

							// Let's copy the bitmap data

							memcpy((void *) psFontChar->pu8FontImageData,
								   (void *) eFace->glyph->bitmap.buffer,
								   psFontChar->u32BitmapXSize * psFontChar->u32BitmapYSize);
						}
						else
						{
							// This means it's a blank character, so don't allocate anything
						}

						if (psFontCharPrior)
						{
							psFontCharPrior->psNextLink = psFontChar;
						}
						else
						{
							psFont->psFontCharacters = psFontChar;
						}

						psFontCharPrior = psFontChar;
					}

					// Figure out the largest/smallest size
					if (psFontChar && psFontChar->u32XAdvance)
					{
						if (psFontChar->u32XAdvance > psFont->u32MaxX)
						{
							psFont->u32MaxX = psFontChar->u32XAdvance;
						}
						if (psFontChar->u32XAdvance < psFont->u32MinX)
						{
							psFont->u32MinX = psFontChar->u32XAdvance;
						}
					}

					if (psFontChar && psFontChar->u32YAdvance)
					{
						if ((psFontChar->u32YAdvance + psFontChar->u32YDescender) > psFont->u32MaxY)
						{
							psFont->u32MaxY = psFontChar->u32YAdvance + psFontChar->u32YDescender;
						}
						if ((psFontChar->u32YAdvance + psFontChar->u32YDescender) < psFont->u32MinY)
						{
							psFont->u32MinY = psFontChar->u32YAdvance + psFontChar->u32YDescender;
						}
					}
				}
			}
			else
			if (FT_Err_Invalid_Composite == eFreetypeError)
			{
				// This is OK - we just ignore the character
				DebugOut(" Font: Glyph index 0x%.4x - Invalid composite\n",
						   u32Loop);
			}
			else
			{
				eFreetypeError = FT_Load_Glyph(eFace,
											   u32Index,
											   FT_LOAD_DEFAULT);

				// Glyph error.
				DebugOut(" Font : Glyph error - 0x%.2x\n",
						   eFreetypeError);
				eResult = (ELCDErr) (eFreetypeError + LERR_FONT_OK);
				goto notAllocated;
			}
		}
		else
		{
#ifdef _WIN32
//			DebugOut("FontCreate: No index mapping for 0x%.2x\n", u32Loop);
#endif // #ifdef _WIN32
		}
	}

	// Finished! Let's get done with da face
	FT_Done_Face(eFace);

	// Uh.... got it! Let's plug everything in to the font slot and return
	// success to the caller.

	FontListLock();
	sg_psFontList[eFontHandle] = psFont;
	if (peFontHandle)
	{
		*peFontHandle = eFontHandle;
	}

	psFont->eFontHandle = eFontHandle;
	psFont->psNextFont = sg_psFontListHead;
	sg_psFontListHead = psFont;
	FontListUnlock();

//	DebugOut("%s: Returning 0x%.8x\n", __FUNCTION__, eFontHandle);

	return(LERR_OK);

notAllocated:

	// Get rid of the face (if it's there)
	FT_Done_Face(eFace);

notAllocatedPreFont:
	// If we have a memory face, we need to get rid of it
	if (psFont)
	{
		// Now free up the font memory
		GCFreeMemory(psFont);
	}

	// Out of memory perhaps?
	if (FT_Err_Out_Of_Memory == eFreetypeError)
	{
		eResult = LERR_NO_MEM;
	}

	// Delete the file out of memory (or a reference to it)

	FontListLock();
	GCASSERT(psFontFile->u32References);
	psFontFile->u32References--;
	FontListUnlock();
	return(eResult);
}

static SFontCharacter *FontLookupChar(SFont *psFont,
									  UINT32 u32Character)
{
	SFontCharacter *psFontChar = psFont->psFontCharacters;
	SFontCharacter *psFontCharPrior = NULL;

	while (psFontChar && psFontChar->u32Character != u32Character)
	{
		psFontCharPrior = psFontChar;
		psFontChar = psFontChar->psNextLink;
	}

	if (NULL == psFontChar)
	{
		// Font character not found
		return(NULL);
	}

	// We found the font. Link it to the head if it's not already there.
	if (NULL == psFontCharPrior)
	{
		return(psFontChar);
	}

	// Otherwise, unlink this font char, and put it at the head of the list
	// so that the most accessed are near the top

	// Commented out for now. Just do a linear search on things. This routine is
	// not thread safe. This was causing missing characters and assertions when
	// a text widget intersected an animated image.

//	psFontCharPrior->psNextLink = psFontChar->psNextLink;
//	psFontChar->psNextLink = psFont->psFontCharacters;
//	psFont->psFontCharacters = psFontChar;

	return(psFontChar);
}

ELCDErr FontRender(FONTHANDLE eFontHandle,
				   WINDOWHANDLE eWINDOWHANDLE,
				   INT32 s32XPos,
				   INT32 s32YPos,
				   INT32 s32XSize,
				   INT32 s32YSize,
				   TEXTCHAR eCharacter,
				   UINT16 u16Color,
				   BOOL bTransparent,
				   INT32 *ps32XAdvance,
				   INT32 *ps32YAdvance,
				   BOOL bErase,
				   ERotation eRot,
				   SImage *psBlitImage,
				   BOOL bDisabled,
				   BOOL bIncludeBearing)
{
	SFont *psFont;
	SWindow *psWindow = NULL;
	SFontCharacter *psFontChar = NULL;
	INT32 s32XBlitSize;
	INT32 s32YBlitSize;
	INT32 s32XPosNormalized;
	INT32 s32YPosNormalized;
	UINT8 *pu8Src = NULL;
	INT32 s32XStep = 0;
	INT32 s32YStep = 0;
	
	// X/Y Positions are in 26.6 fixed point

	// Get our font handle
	psFont = FontGetPointer(eFontHandle);
	if (NULL == psFont)
	{
		// Bad font handle
		return(LERR_FONT_BAD_HANDLE);
	}

	// Good font. Let's see if we have a character map.
	psFontChar = FontLookupChar(psFont,
								eCharacter);
	if (NULL == psFontChar)
	{
		// Nonexistent character!
		return(LERR_FONT_CHAR_NONEXISTENT);
	}

	// Now get our window handle
	if (NULL == psBlitImage)
	{
		psWindow = WindowGetPointer(eWINDOWHANDLE);
		if (NULL == psWindow)
		{
			// Bad window handle
			return(LERR_WIN_BAD_HANDLE);
		}
	}

	// If our active area is set to -1, -1, use the Window's active area
	if (-1 == s32XSize)
	{
		s32XSize = (INT32) psWindow->u32ActiveAreaXSize;
	}

	if (-1 == s32YSize)
	{
		s32YSize = (INT32) psWindow->u32ActiveAreaYSize;
	}

	// Clip the X/Y size - different algorithm depending upon the rotation angle

	if (ROT_0 == eRot)
	{
		// Source is the upper left hand corner of the image
		pu8Src = psFontChar->pu8FontImageData;

		s32XBlitSize = (INT32) psFontChar->u32BitmapXSize;
		s32YBlitSize = (INT32) psFontChar->u32BitmapYSize;

		// Compute the absolute X and Y position in the active image for this character
		s32XPosNormalized = s32XPos;
		
		if (bIncludeBearing)
		{
			s32XPosNormalized += psFontChar->u32XBearing;
		}

		s32YPosNormalized = s32YPos;

		if (bIncludeBearing)
		{
			s32YPosNormalized += psFontChar->u32YBearing;
		}

		// Clip to the left side
		if (s32XPosNormalized < 0)
		{
			s32XBlitSize = (s32XBlitSize + s32XPosNormalized);
			pu8Src += ((-s32XPosNormalized) >> 6);
			s32XPosNormalized = 0;
		}

		if (s32YPosNormalized < 0)
		{
			s32YBlitSize = (s32YBlitSize + s32YPosNormalized);
			pu8Src += (((-s32YPosNormalized) >> 6) * psFontChar->u32BitmapXSize);
			s32YPosNormalized = 0;
		}

		s32XStep = 1;
		s32YStep = (psFontChar->u32BitmapXSize - s32XBlitSize);
	}
	else
	if (ROT_90 == eRot)
	{
		// Source is the lower left hand corner of the image
		pu8Src = psFontChar->pu8FontImageData + (psFontChar->u32BitmapXSize * (psFontChar->u32BitmapYSize - 1));

		s32XBlitSize = (INT32) psFontChar->u32BitmapYSize;
		s32YBlitSize = (INT32) psFontChar->u32BitmapXSize;

		// Compute the absolute X and Y position in the active image for this character
		if (bIncludeBearing)
		{
			s32XPosNormalized = s32XPos + (psFont->u32MaxY - psFontChar->u32YBearing - (psFontChar->u32BitmapYSize << 6));
			s32YPosNormalized = s32YPos + psFontChar->u32XBearing;
		}
		else
		{
			s32XPosNormalized = s32XPos + (psFont->u32MaxY - (psFontChar->u32BitmapYSize << 6));
			s32YPosNormalized = s32YPos;
		}


		// Clip to the left side
		if (s32XPosNormalized < 0)
		{
			s32XBlitSize = (s32XBlitSize + (s32XPosNormalized >> 6));
			pu8Src += -(s32XPosNormalized >> 6);
			s32XPosNormalized = 0;
		}

		if (s32YPosNormalized < 0)
		{
			s32YBlitSize = (s32YBlitSize + (s32YPosNormalized >> 6));
			pu8Src += (-(s32YPosNormalized >> 6));
			s32YPosNormalized = 0;
		}

		s32XStep = -((INT32) psFontChar->u32BitmapXSize);
		s32YStep = 1 + (psFontChar->u32BitmapXSize * psFontChar->u32BitmapYSize);
	}
	else
	if (ROT_180 == eRot)
	{
		// Source is the lower right hand corner of the image
		pu8Src = (psFontChar->pu8FontImageData + (psFontChar->u32BitmapXSize * psFontChar->u32BitmapYSize)) - 1;

		s32XBlitSize = (INT32) psFontChar->u32BitmapXSize;
		s32YBlitSize = (INT32) psFontChar->u32BitmapYSize;

		if (bIncludeBearing)
		{
			// Compute the absolute X and Y position in the active image for this character
			s32XPosNormalized = s32XPos + psFontChar->u32XBearing;

			// Must invert the Y bearing since things are going to be upside down and backwards
			s32YPosNormalized = s32YPos + (psFont->u32MaxY - psFontChar->u32YBearing - (psFontChar->u32BitmapYSize << 6));
		}
		else
		{
			s32XPosNormalized = s32XPos;
			s32YPosNormalized = s32YPos + (psFont->u32MaxY - (psFontChar->u32BitmapYSize << 6));
		}

		// Clip to the left side
		if (s32XPosNormalized < 0)
		{
			s32XBlitSize = (s32XBlitSize + (s32XPosNormalized >> 6));
			pu8Src += (s32XPosNormalized >> 6);
			s32XPosNormalized = 0;
		}

		if (s32YPosNormalized < 0)
		{
			s32YBlitSize = (s32YBlitSize + s32YPosNormalized);
			pu8Src -= (-(s32YPosNormalized >> 6) * psFontChar->u32BitmapXSize);
			s32YPosNormalized = 0;
		}

		// Origin is lower right hand part of the pixel
		s32YStep = -((INT32) psFontChar->u32BitmapXSize - s32XBlitSize);
		s32XStep = -1;
	}
	else
	if (ROT_270 == eRot)
	{
		// Source is the upper right hand corner of the image
		pu8Src = psFontChar->pu8FontImageData + (psFontChar->u32BitmapXSize - 1);

		s32XBlitSize = (INT32) psFontChar->u32BitmapYSize;
		s32YBlitSize = (INT32) psFontChar->u32BitmapXSize;

		// Compute the absolute X and Y position in the active image for this character
		if (bIncludeBearing)
		{
			s32XPosNormalized = s32XPos + psFontChar->u32YBearing;
			s32YPosNormalized = s32YPos + psFontChar->u32XBearing;
		}
		else
		{
			s32XPosNormalized = s32XPos;
			s32YPosNormalized = s32YPos;
		}

		// Clip to the left side
		if (s32XPosNormalized < 0)
		{
			s32XBlitSize = (s32XBlitSize + (s32XPosNormalized >> 6));
			pu8Src += (-(s32XPosNormalized >> 6) * psFontChar->u32BitmapXSize);
			s32XPosNormalized = 0;
		}

		if (s32YPosNormalized < 0)
		{
			pu8Src -= (s32YBlitSize - (s32YBlitSize + (s32YPosNormalized >> 6)));
			s32YBlitSize = (s32YBlitSize + (s32YPosNormalized >> 6));
			s32YPosNormalized = 0;
		}

		s32XStep = psFontChar->u32BitmapXSize;
		s32YStep = -((INT32) (psFontChar->u32BitmapXSize * s32XBlitSize) + 1);
	}
	else
	{
		// WTF?
		GCASSERT(0);
	}

	// Turn s32XPosNormalized into 32.0 instead of 26.6 (and round up!)
	s32XPosNormalized = (s32XPosNormalized + 0x1f) >> 6;
	s32YPosNormalized = (s32YPosNormalized + 0x1f) >> 6;

	// Only clip if the 
	if (NULL == psBlitImage)
	{
		// Clip it to the active area - clip to the right and bottom sides
		if ((s32XPosNormalized + s32XBlitSize) >= s32XSize)
		{
			s32XBlitSize += (s32XSize - ((INT32) s32XPosNormalized + s32XBlitSize));

			if (ROT_0 == eRot)
			{
				// No adjustment needed
			}
			else
			if (ROT_90 == eRot)
			{
				s32YStep = 1 + (psFontChar->u32BitmapXSize * s32XBlitSize);
			}
			else
			if (ROT_180 == eRot)
			{
				s32YStep = -((INT32) psFontChar->u32BitmapXSize - s32XBlitSize);
			}
			else
			if (ROT_270 == eRot)
			{
				s32YStep = -((INT32) (psFontChar->u32BitmapXSize * s32XBlitSize) + 1);
			}
			else
			{
				GCASSERT(0);
			}
		}

		if ((s32YPosNormalized + s32YBlitSize) >= s32YSize)
		{
			s32YBlitSize += (s32YSize - ((INT32) s32YPosNormalized + s32YBlitSize));
		}
	}
	else
	{
		// Blitting to an image
	}

	// Only blit if the image is within the active region and we have something to blit
	if ((s32XBlitSize > 0) && (s32YBlitSize > 0))
	{
		UINT16 *pu16Dest;
		UINT8 u8ColorRed = u16Color >> 11;
		UINT8 u8ColorGreen = (u16Color >> 5) & 0x3f;
		UINT8 u8ColorBlue = u16Color & 0x1f;
		INT32 s32XCount;
		UINT32 u32Pitch;

		// Now go erase what's underneath what we're going to blit

		if (NULL == psBlitImage)
		{
			if (bErase)
			{
				WindowEraseActiveRegion(psWindow,
									    s32XPosNormalized + (INT32) psWindow->u32ActiveAreaXPos,
									    s32YPosNormalized + (INT32) psWindow->u32ActiveAreaYPos,
										s32XBlitSize,
										s32YBlitSize);
			}

			// Update the console window region
			WindowUpdateRegion(eWINDOWHANDLE,
							   s32XPosNormalized + (INT32) psWindow->u32ActiveAreaXPos,
							   s32YPosNormalized + (INT32) psWindow->u32ActiveAreaYPos,
							   (UINT32) s32XBlitSize,
							   (UINT32) s32YBlitSize);
		}

		// Compute our destination pointer

		if (NULL == psBlitImage)
		{
			s32XPosNormalized += psWindow->u32ActiveAreaXPos;
			s32YPosNormalized += psWindow->u32ActiveAreaYPos;

			pu16Dest = psWindow->psWindowImage->pu16ImageData + s32XPosNormalized +
					   (s32YPosNormalized * psWindow->psWindowImage->u32Pitch);
			u32Pitch = (psWindow->psWindowImage->u32Pitch - s32XBlitSize);
		}
		else
		{
			pu16Dest = psBlitImage->pu16ImageData + s32XPosNormalized + (s32YPosNormalized * psBlitImage->u32Pitch);
			u32Pitch = (psBlitImage->u32Pitch - s32XBlitSize);
		}

		// We've now "erased" what's underneath where we're going to be blitting

		if (bDisabled)
		{
			BOOL bToggle = FALSE;
			UINT8 u8ColorRed2;
			UINT8 u8ColorGreen2;
			UINT8 u8ColorBlue2;
			UINT8 u8ColorRed1;
			UINT8 u8ColorGreen1;
			UINT8 u8ColorBlue1;

			u16Color = GRAY(u16Color);

			u8ColorRed1 = u16Color >> 11;
			u8ColorGreen1 = (u16Color >> 5) & 0x3f;
			u8ColorBlue1 = u16Color & 0x1f;

			u8ColorRed2 = u8ColorRed1 >> 1;
			u8ColorGreen2 = u8ColorGreen1 >> 1;
			u8ColorBlue2 = u8ColorBlue1 >> 1;

			while (s32YBlitSize--)
			{
				s32XCount = s32XBlitSize;
				if ((s32XBlitSize & 1) == 0)
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

				while (s32XCount--)
				{
					UINT8 u8MaskByte;

					if (bToggle)
					{
						u8ColorRed = u8ColorRed1;
						u8ColorGreen = u8ColorGreen1;
						u8ColorBlue = u8ColorBlue1;
						bToggle = FALSE;
					}
					else
					{
						u8ColorRed = u8ColorRed2;
						u8ColorGreen = u8ColorGreen2;
						u8ColorBlue = u8ColorBlue2;
						bToggle = TRUE;
					}

					u16Color = (u8ColorRed << 11) | (u8ColorGreen << 5) | u8ColorBlue;

					if (0 == *pu8Src)
					{
						// Just skip this pixel
					}
					else
					if (0xff == *pu8Src)
					{
						// Solid pixel
						*pu16Dest = u16Color;
					}
					else
					{
						UINT16 u16Src;
						UINT8 u8Red;
						UINT8 u8Green;
						UINT8 u8Blue;

						// Ugly! Time to do translucency. Grab the pixel underneath.
						u8MaskByte = *pu8Src;
						u16Src = *pu16Dest;

						// It's partial/ugly. Time to separate the bands, do the add/multiply, etc...
						u8Red = (((u16Src >> 11) * (0xff - u8MaskByte)) >> 8) +
								(((u8ColorRed) * (u8MaskByte)) >> 8);

						u8Green = ((((u16Src >> 5) & 0x3f) * (0xff - u8MaskByte)) >> 8) +
								  ((((u8ColorGreen) & 0x3f) * (u8MaskByte)) >> 8);

						u8Blue = (((u16Src & 0x1f) * (0xff - u8MaskByte)) >> 8) +
								 (((u8ColorBlue) * (u8MaskByte)) >> 8);

						*pu16Dest = sg_u16RedGradientSaturation[u8Red] |
									sg_u16GreenGradientSaturation[u8Green] |
									sg_u16BlueGradientSaturation[u8Blue];
					}

					++pu16Dest;
					pu8Src += s32XStep;
				}

				// Advance our image pointer
				pu16Dest += u32Pitch;
				pu8Src += s32YStep;
			}
		}
		else
		{
			while (s32YBlitSize--)
			{
				s32XCount = s32XBlitSize;
				while (s32XCount--)
				{
					UINT8 u8MaskByte;

					if (0 == *pu8Src)
					{
						// Just skip this pixel
					}
					else
					if (0xff == *pu8Src)
					{
						// Solid pixel
						*pu16Dest = u16Color;
					}
					else
					{
						UINT16 u16Src;
						UINT8 u8Red;
						UINT8 u8Green;
						UINT8 u8Blue;

						// Ugly! Time to do translucency. Grab the pixel underneath.
						u8MaskByte = *pu8Src;
						u16Src = *pu16Dest;

						// It's partial/ugly. Time to separate the bands, do the add/multiply, etc...
						u8Red = (((u16Src >> 11) * (0xff - u8MaskByte)) >> 8) +
								(((u8ColorRed) * (u8MaskByte)) >> 8);

						u8Green = ((((u16Src >> 5) & 0x3f) * (0xff - u8MaskByte)) >> 8) +
								  ((((u8ColorGreen) & 0x3f) * (u8MaskByte)) >> 8);

						u8Blue = (((u16Src & 0x1f) * (0xff - u8MaskByte)) >> 8) +
								 (((u8ColorBlue) * (u8MaskByte)) >> 8);

						*pu16Dest = sg_u16RedGradientSaturation[u8Red] |
									sg_u16GreenGradientSaturation[u8Green] |
									sg_u16BlueGradientSaturation[u8Blue];
					}

					++pu16Dest;
					pu8Src += s32XStep;
				}

				// Advance our image pointer
				pu16Dest += u32Pitch;
				pu8Src += s32YStep;
			}
		}
	}

	if (ps32XAdvance)
	{
		if (ROT_0 == eRot)
		{
			*ps32XAdvance = (INT32) psFontChar->u32XAdvance;
		}
		else
		if (ROT_90 == eRot)
		{
			*ps32XAdvance = 0;
		}
		else
		if (ROT_180 == eRot)
		{
			*ps32XAdvance = -((INT32) psFontChar->u32XAdvance);
		}
		else
		if (ROT_270 == eRot)
		{
			*ps32XAdvance = 0;
		}
		else
		{
			// Bogus rotation value
			GCASSERT(0);
		}
	}

	if (ps32YAdvance)
	{
		if (ROT_0 == eRot)
		{
			*ps32YAdvance = 0;
		}
		else
		if (ROT_90 == eRot)
		{
			*ps32YAdvance = (INT32) psFontChar->u32XAdvance;
		}
		else
		if (ROT_180 == eRot)
		{
			*ps32YAdvance = 0;
		}
		else
		if (ROT_270 == eRot)
		{
			*ps32YAdvance = -((INT32) psFontChar->u32XAdvance);
		}
		else
		{
			// Bogus rotation value
			GCASSERT(0);
		}
	}

	return(LERR_OK);
}

ELCDErr FontGetSize(FONTHANDLE eFontHandle,
					TEXTCHAR eCharacter,
					UINT32 *pu32XSize,
					UINT32 *pu32YSize,
					INT32 *ps32XAdvance,
					INT32 *ps32YAdvance,
					ERotation eRot,
					BOOL bIncludeBearing)
{
	SFont *psFont;
	SFontCharacter *psFontChar = NULL;

	// Get our font handle
	psFont = FontGetPointer(eFontHandle);
	if (NULL == psFont)
	{
		// Bad font handle
		return(LERR_FONT_BAD_HANDLE);
	}

	// Good font. Let's see if we have a character map.
	psFontChar = FontLookupChar(psFont,
								eCharacter);
	if (NULL == psFontChar)
	{
		// Nonexistent character!
		return(LERR_FONT_CHAR_NONEXISTENT);
	}

	// We've got a character. Let's fill in the info
	if (ps32XAdvance)
	{
		if (ROT_0 == eRot)
		{
			*ps32XAdvance = (INT32) psFontChar->u32XAdvance;
		}
		else
		if (ROT_90 == eRot)
		{
			*ps32XAdvance = (INT32) psFont->s32Stride;
		}
		else
		if (ROT_180 == eRot)
		{
			*ps32XAdvance = -((INT32) psFontChar->u32XAdvance);
		}
		else
		if (ROT_270 == eRot)
		{
			*ps32XAdvance = -((INT32) psFont->s32Stride);
		}
		else
		{
			// Bogus rotation value
			GCASSERT(0);
		}
	}

	if (ps32YAdvance)
	{
		if (ROT_0 == eRot)
		{
			*ps32YAdvance = (INT32) psFont->s32Stride;
		}
		else
		if (ROT_90 == eRot)
		{
			*ps32YAdvance = (INT32) psFontChar->u32XAdvance;
		}
		else
		if (ROT_180 == eRot)
		{
			*ps32YAdvance = (INT32) psFont->s32Stride;
		}
		else
		if (ROT_270 == eRot)
		{
			*ps32YAdvance = -((INT32) psFontChar->u32XAdvance);
		}
		else
		{
			// Bogus rotation value
			GCASSERT(0);
		}
	}

	if (pu32XSize)
	{
		*pu32XSize = psFontChar->u32BitmapXSize;

		// If we're including the bearing or it's 0/180 degrees, include the bearing - always
		if (bIncludeBearing)
		{
			 *pu32XSize += ((psFontChar->u32XBearing + 63) >> 6);
		}
	}

	if (pu32YSize)
	{
		*pu32YSize = psFontChar->u32BitmapYSize;

		if (bIncludeBearing)
		{
			 *pu32YSize += ((psFontChar->u32YBearing + 63) >> 6);
		}
	}

	return(LERR_OK);
}

ELCDErr FontGetStringSizeLen(FONTHANDLE eFontHandle,
							  LEX_CHAR *peString,
							  UINT32 u32StringLength,
							  UINT32 *pu32XSize,
							  UINT32 *pu32YSize,
							  ERotation eRotation,
							  BOOL bIncludeBearing)
{
	ELCDErr eErr;
	INT32 s32XPos = 0;
	INT32 s32YPos = 0;
	UINT32 u32XCharSizeMax = 0;
	UINT32 u32YCharSizeMax = 0;
	INT32 s32XAdvance = 0;
	INT32 s32YAdvance = 0;
	UINT32 u32XCharSize = 0;
	UINT32 u32YCharSize = 0;

	// Start off with no size at all
	*pu32XSize = 0;
	*pu32YSize = 0;

	if (NULL == peString)
	{
		return(LERR_OK);
	}

	// Check the font handle. If it's invalid, bail out.
	if (NULL == FontGetPointer(eFontHandle))
	{
		return(LERR_FONT_BAD_HANDLE);
	}

	while (u32StringLength--)
	{

		eErr = FontGetSize(eFontHandle,
						   (TEXTCHAR) *peString,
						   &u32XCharSize,
						   &u32YCharSize,
						   &s32XAdvance,
						   &s32YAdvance,
						   eRotation,
						   bIncludeBearing);

		if (LERR_OK == eErr)
		{
			if ((ROT_0 == eRotation) ||
				(ROT_180 == eRotation))
			{
				s32XPos += s32XAdvance;

				if ((INT32) (u32YCharSize << 6) > s32YAdvance)
				{
					s32YAdvance = (INT32) (u32YCharSize << 6);
				}
				if (abs(s32YAdvance) > s32YPos)
				{
					s32YPos = abs(s32YAdvance);
				}
			}
			else
			if ((ROT_90 == eRotation) ||
				(ROT_270 == eRotation))
			{
				s32YPos += s32YAdvance;
				if (abs(s32XAdvance) > s32XPos)
				{
					s32XPos = abs(s32XAdvance);
				}
			}
			else
			{
				GCASSERT(0);
			}
		}
		
		++peString;
	}

	// If the final character extends past the advance, add in its size instead
	if ((INT32) (u32XCharSize << 6) > s32XAdvance)
	{
		s32XPos -= s32XAdvance;
		s32XPos += (INT32) (u32XCharSize << 6);
	}

	if ((INT32) (u32YCharSize << 6) > s32YAdvance)
	{
		s32YPos -= s32YAdvance;
		s32YPos += (INT32) (u32YCharSize << 6);
	}

	*pu32XSize = (UINT32) ((abs(s32XPos) + 63) >> 6);
	*pu32YSize = (UINT32) ((abs(s32YPos) + 63) >> 6);

	return(LERR_OK);
}

ELCDErr FontGetStringSize(FONTHANDLE eFontHandle,
							  LEX_CHAR *peString,
							  UINT32 *pu32XSize,
							  UINT32 *pu32YSize,
							  ERotation eRotation,
							  BOOL bIncludeBearing)
{
	UINT32 u32StringLength = 0;

	if( peString )
	{
		u32StringLength = Lexstrlen(peString);
	}

	return( FontGetStringSizeLen(eFontHandle,
								  peString,
								  u32StringLength,
								  pu32XSize,
								  pu32YSize,
								  eRotation,
								  bIncludeBearing) );
}

ELCDErr FontGetStringSizeASCII(FONTHANDLE eFontHandle,
							   char *pu8String,
							   UINT32 *pu32XSize,
							   UINT32 *pu32YSize,
							   ERotation eRotation,
							   BOOL bIncludeBearing)
{
	ELCDErr eErr;
	LEX_CHAR *peData;

	// Convert it from ASCII to unicode
	peData = LexASCIIToUnicodeAlloc(pu8String);
	if (NULL == peData)
	{
		return(LERR_NO_MEM);
	}

	eErr = FontGetStringSize(eFontHandle,
							 peData,
							 pu32XSize,
							 pu32YSize,
							 eRotation,
							 bIncludeBearing);

	GCFreeMemory(peData);

	return(eErr);
}

// Used by clients that just want to reuse the handle without allocating a new font by filename
ELCDErr FontIncRef(FONTHANDLE eFontHandle)
{
	SFont *psFont;

//	DebugOut("%s: Called with 0x%.8x\n", __FUNCTION__, eFontHandle);

	// Get our font handle
	psFont = FontGetPointer(eFontHandle);
	if (NULL == psFont)
	{
		// Bad font handle
		GCASSERT(NULL != psFont);
		return(LERR_FONT_BAD_HANDLE);
	}

	FontListLock();

	psFont->u32References++;
	GCASSERT(psFont->u32References);
	GCASSERT(eFontHandle == psFont->eFontHandle);

	FontListUnlock();

	return(LERR_OK);
}

ELCDErr FontFree(FONTHANDLE eFontHandle)
{
	SFont *psFont;

//	DebugOut("%s: Called with 0x%.8x\n", __FUNCTION__, eFontHandle);

	// Get our font handle
	psFont = FontGetPointer(eFontHandle);
	if (NULL == psFont)
	{
		// Bad font handle
		GCASSERT(NULL != psFont);
		return(LERR_FONT_BAD_HANDLE);
	}

	FontListLock();
	GCASSERT(psFont->u32References);
	psFont->u32References--;
	GCASSERT(eFontHandle == psFont->eFontHandle);

	if (0 == psFont->u32References)
	{
		GCASSERT(psFont->psFontFile->u32References);
		psFont->psFontFile->u32References--;
		psFont->eFontHandle = HANDLE_INVALID;

		// Free up the font handle
		sg_psFontList[eFontHandle] = NULL;
	}

	FontListUnlock();

	return(LERR_OK);
}

BOOL FontFreeUnused(void)
{
	SFontFile *psFontFilePrior = NULL;
	SFontFile *psFontFilePtr = NULL;
	SFont *psFont = NULL;
	SFont *psFontPrior = NULL;

	// First try fonts themselves

	FontListLock();

	psFont = sg_psFontListHead;
	while (psFont && psFont->u32References)
	{
		psFontPrior = psFont;
		psFont = psFont->psNextFont;
	}

	if (psFont)
	{
		SFontCharacter *psCharPrior = NULL;
		SFontCharacter *psChar = NULL;

		// Time to ditch this font
		GCASSERT(0 == psFont->u32References);

		psChar = psFont->psFontCharacters;

		while (psChar)
		{
			psCharPrior = psChar;
			psChar = psChar->psNextLink;
			if (psCharPrior->pu8FontImageData)
			{
				GCFreeMemory(psCharPrior->pu8FontImageData);
			}
			memset((void *) psCharPrior, 0, sizeof(*psCharPrior));

			GCFreeMemory(psCharPrior);
		}

		GCASSERT(psFont->psFontFile);

		if (NULL == psFontPrior)
		{
			sg_psFontListHead = sg_psFontListHead->psNextFont;
		}
		else
		{
			psFontPrior->psNextFont = psFont->psNextFont;
		}
	
		GCFreeMemory(psFont);
		FontListUnlock();
		return(TRUE);
	}

	// Now try the font file(source)

	psFontFilePtr = sg_psFontFileListHead;

	while (psFontFilePtr)
	{
		if (0 == psFontFilePtr->u32References)
		{
			break;
		}

		psFontFilePrior = psFontFilePtr;
		psFontFilePtr = psFontFilePtr->psNextLink;
	}

	if (NULL == psFontFilePtr)
	{
		FontListUnlock();
		// Nothing to free. ;-(
		return(FALSE);
	}

	// Unlink it from the master list 'o fonts

	if (NULL == psFontFilePrior)
	{
		sg_psFontFileListHead = psFontFilePtr->psNextLink;
	}
	else
	{
		psFontFilePrior->psNextLink = psFontFilePtr->psNextLink;
	}

	FontListUnlock();

	// Time to free it!
	if (psFontFilePtr->pvFontFileInMemory)
	{
		GCFreeMemory(psFontFilePtr->pvFontFileInMemory);
		psFontFilePtr->pvFontFileInMemory = NULL;
	}

	if (psFontFilePtr->peFontFilename)
	{
		GCFreeMemory(psFontFilePtr->peFontFilename);
		psFontFilePtr->peFontFilename = NULL;
	}

	GCFreeMemory(psFontFilePtr);

	return(TRUE);
}

void FontMgrInit(void)
{
	EGCResultCode eResult;
	FT_Error eError;
	UINT32 u32Loop;

	// Let's initialize the Freetype library

	DebugOut("* Initializing FreeType\n");

	eError = FT_Init_FreeType(&sg_sFontLibrary);
	GCASSERT(eError == FT_Err_Ok);

	for (u32Loop = 0; u32Loop < (sizeof(sg_u16GrayPalette) / sizeof(sg_u16GrayPalette[0])); u32Loop++)
	{
		sg_u16GrayPalette[u32Loop] = GRAY(u32Loop);
	}

	// Create it as unlocked
	eResult = GCOSSemaphoreCreate(&sg_sFontLock,
								  1);
	GCASSERT(GC_OK == eResult);
}
