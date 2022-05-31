#include "Startup/app.h"
#include "Libs/Gfx/GraphicsLib.h"
#include "Libs/window/window.h"
#include "Libs/widget/widget.h"
#include "Libs/widget/text/text.h"

static ELCDErr TextWidgetAlloc(SWidget *psWidget,
							   WIDGETHANDLE eHandle)
{
	GCASSERT(psWidget);
	psWidget->uWidgetSpecific.psText = MemAlloc(sizeof(*psWidget->uWidgetSpecific.psText));
	if (NULL == psWidget->uWidgetSpecific.psText)
	{
		return(LERR_NO_MEM);
	}
	else
	{
		psWidget->uWidgetSpecific.psText->eTextHandle = (TEXTHANDLE) eHandle;
		psWidget->uWidgetSpecific.psText->eFont = HANDLE_INVALID;
		psWidget->uWidgetSpecific.psText->bTransparent = TRUE;		// Transparent by default
		psWidget->uWidgetSpecific.psText->psWidget = psWidget;
		return(LERR_OK);
	}
}

static ELCDErr TextWidgetFree(SWidget *psWidget)
{
	ELCDErr eErr;
	SText *psText;
	
	GCASSERT(psWidget);
	psText = psWidget->uWidgetSpecific.psText;
	GCASSERT(psText);
	
	// Let's see if we have any text
	if (psText->peText)
	{
		GCFreeMemory(psText->peText);
		psText->peText = NULL;
	}

	// Go delete a reference to the font
	eErr = FontFree(psText->eFont);
	GCPERFASSERT(LERR_OK == eErr);

	psText->eTextHandle = HANDLE_INVALID;
	GCFreeMemory(psText);
	psWidget->uWidgetSpecific.psText = NULL;
	return(eErr);
}

static void TextErase(SWidget *psWidget)
{
	WidgetEraseStandard(psWidget);
}

static void TextPaint(SWidget *psWidget,
					  BOOL bLock)
{
	SText *psText = psWidget->uWidgetSpecific.psText;
	TEXTCHAR *peTextChar;
	INT32 s32XPos;
	INT32 s32XAdvance;
	INT32 s32YPos;
	INT32 s32YAdvance;
	INT32 s32XSize;
	INT32 s32YSize;
	UINT32 u32CharCount;
	UINT16 u16TextColor;

	GCASSERT(psText);
	s32XPos = psWidget->s32XPos;
	s32YPos = psWidget->s32YPos;

	peTextChar = psText->peText;
	if (NULL == peTextChar)
	{
		// No text - just return - nothing to paint
		return;
	}

	// If this text object is hidden, don't paint it
	if (psWidget->bWidgetHidden)
	{
		return;
	}

	// Get the length of our characters
	u32CharCount = Lexstrlen(peTextChar);

	if ((ROT_180 == psText->u16Orientation) ||
		(ROT_270 == psText->u16Orientation))
	{
		if (u32CharCount)
		{
			peTextChar += (u32CharCount - 1);
		}
	}

	// Erase it so we can redraw it, otherwise the translucent pixels will
	// add to eachother, generating an incorrect image
	TextErase(psWidget);

	// First, figure out if we need to do a bounding box draw
	if ((psText->u32BoundingBoxXSize) &&
		(psText->u32BoundingBoxYSize))
	{
		INT32 s32XSize;
		INT32 s32YSize;
		SWindow *psWindow;
		UINT32 u32BoxColor;

		// Get the window handle
		psWindow = WindowGetPointer(psWidget->eParentWindow);
		GCASSERT(psWindow);

		// Clip to X extreme
		s32XSize = (INT32) psText->u32BoundingBoxXSize;
		if ((s32XSize + s32XPos) > (INT32) (psWindow->u32ActiveAreaXSize + psWindow->u32ActiveAreaXPos))
		{
			s32XSize = (psWindow->u32ActiveAreaXSize + psWindow->u32ActiveAreaXPos) - (s32XSize + s32XPos);
		}

		// Clip to Y extreme
		s32YSize = (INT32) psText->u32BoundingBoxYSize;
		if ((s32YSize + s32YPos) > (INT32) (psWindow->u32ActiveAreaYSize + psWindow->u32ActiveAreaYPos))
		{
			s32YSize = (psWindow->u32ActiveAreaYSize + psWindow->u32ActiveAreaYPos) - (s32YSize + s32YPos);
		}

		// If we still have a size in both axes, figure out the color
		if ((s32XSize > 0) && (s32YSize > 0))
		{
			// If the widget is moused over, then choose the higlighted color
			if (FALSE == psWidget->bMouseOverDisabled)
			{
				if (psWidget->bMousedOver)
				{
					u32BoxColor = psText->u32MouseoverBoxColor;
				}
				else
				{
					u32BoxColor = psText->u32BoxColor;
				}
			}
			else
			{
				u32BoxColor = psText->u32BoxColor;
			}

			// If there isn't a box color, then don't highlight it
			if (u32BoxColor != TEXT_COLOR_NONE)
			{
				UINT16 *pu16ColorFill;
				UINT32 u32YCount;
				UINT32 u32XCount;

				pu16ColorFill = psWindow->psWindowImage->pu16ImageData + s32XPos + (s32YPos * psWindow->psWindowImage->u32Pitch);
				u32YCount = (UINT32) s32YSize;
				while (u32YCount--)
				{
					u32XCount = (UINT32) s32XSize;
					while (u32XCount--)
					{
						*pu16ColorFill = (UINT16) u32BoxColor;
						++pu16ColorFill;
					}

					// Next scanline
					pu16ColorFill += (psWindow->psWindowImage->u32Pitch - s32XSize);
				}
			}
		}
	}

	// Figure out which color it should be
	u16TextColor = psText->u16Color;

	// If we have a mouseover color and we're moused over, change the color
	if ((psWidget->bMousedOver) &&
		(FALSE == psWidget->bMouseOverDisabled) && 
		(psText->u32MouseoverTextColor != TEXT_COLOR_NONE))
	{
		u16TextColor = (UINT16) psText->u32MouseoverTextColor;
	}

	// Add in the textual offset
	s32XPos += (INT32) psText->u32TextXOffset;
	s32YPos += (INT32) psText->u32TextYOffset;

	// Get our size. Assume it's the window size by default
	s32XSize = -1;
	s32YSize = -1;

	if (psText->u32TextXClip)
	{
		s32XSize = (psText->u32TextXClip + s32XPos);
	}

	if (psText->u32TextYClip)
	{
		s32YSize = (psText->u32TextYClip + s32YPos);
	}

	// We now have text to paint. Let's go paint.
	s32XPos <<= 6;
	s32YPos <<= 6;

	while (u32CharCount--)
	{
		ELCDErr eLCDErr;

		eLCDErr = FontRender(psText->eFont,
							 psWidget->eParentWindow,
							 s32XPos,
							 s32YPos,
							 s32XSize,
							 s32YSize,
							 (UINT32) *peTextChar,
							 u16TextColor,
							 psText->bTransparent,
							 &s32XAdvance,
							 &s32YAdvance,
							 FALSE,
							 psText->u16Orientation,
							 NULL,
							 FALSE,
							 TRUE);

		if (LERR_OK == eLCDErr)
		{
			if (ROT_0 == psText->u16Orientation)
			{
				s32XPos += s32XAdvance;
				++peTextChar;
			}
			else
			if (ROT_90 == psText->u16Orientation)
			{
				s32YPos += s32YAdvance;
				++peTextChar;
			}
			else
			if (ROT_180 == psText->u16Orientation)
			{
				s32XPos -= s32XAdvance;
				--peTextChar;
			}
			else
			if (ROT_270 == psText->u16Orientation)
			{
				s32YPos -= s32YAdvance;
				--peTextChar;
			}
			else
			{
				GCASSERT(0);
			}
		}
		else
		{
			GCASSERT(0);
		}
	}

	// If this fails, then it means the rendering code and the X size computation code
	// is in disagreement.
	GCASSERT(psWidget->u32YSize == psWidget->u32YSize);

	WindowUpdateRegion(psWidget->eParentWindow,
					   psWidget->s32XPos,
					   psWidget->s32YPos,
					   psWidget->u32XSize,
					   psWidget->u32YSize);
}

static void TextMouseover(struct SWidget *psWidget,
						  UINT32 u32Mask,
						  UINT32 u32XPos,
						  UINT32 u32YPos,
						  EMouseOverState eMouseoverState)
{
	BOOL bUpdated = FALSE;

	if (EMOUSEOVER_ASSERTED == eMouseoverState)
	{
//		DebugOut("%s: Mouseover asserted\n", __FUNCTION__);
		TextPaint(psWidget,
				  FALSE);
		bUpdated = TRUE;
	}
	else
	if (EMOUSEOVER_DEASSERTED == eMouseoverState)
	{
//		DebugOut("%s: Mouseover deasserted\n", __FUNCTION__);
		TextPaint(psWidget,
				  FALSE);
		bUpdated = TRUE;
	}
	else
	if (EMOUSEOVER_UNCHANGED == eMouseoverState)
	{
//		DebugOut("%s: Mouseover - unchanged\n", __FUNCTION__);
	}
	else
	{
		GCASSERT(0);
	}

	if (bUpdated)
	{
		// Commit it
		WindowUpdateRegionCommit();
	}
}

static void TextPressed(SWidget *psWidget,
						UINT32 u32Mask,
						UINT32 u32XPos,
						UINT32 u32YPos)
{
	UWidgetCallbackData uData;

	// Clear out our data
	memset((void *) &uData, 0, sizeof(uData));

	// Put in callback specific data
	uData.sText.u32ButtonMask = u32Mask;
	uData.sText.u32XPos = u32XPos;
	uData.sText.u32YPos = u32YPos;
	uData.sText.bPress = TRUE;

	WidgetBroadcastMask((WIDGETHANDLE) psWidget->eWidgetHandle,
						WCBK_SPECIFIC,
						&uData);
}

static void TextReleased(SWidget *psWidget,
						 UINT32 u32Mask,
						 UINT32 u32XPos,
						 UINT32 u32YPos)
{
	UWidgetCallbackData uData;

	// Clear out our data
	memset((void *) &uData, 0, sizeof(uData));

	// Put in callback specific data
	uData.sText.u32ButtonMask = u32Mask;
	uData.sText.u32XPos = u32XPos;
	uData.sText.u32YPos = u32YPos;
	uData.sText.bPress = FALSE;

	WidgetBroadcastMask((WIDGETHANDLE) psWidget->eWidgetHandle,
						WCBK_SPECIFIC,
						&uData);
}


static SWidgetFunctions sg_sTextWidgetFunctions =
{
	NULL,				// Hit test - not used
	TextPaint,			// Paint the text
	TextErase,			// Erase the text
	TextPressed,		// Text hit (not used)
	TextReleased,		// Text release (not used)
	TextMouseover,		// Mouseover our widget
	NULL,				// Focus
	NULL,				// Keystroke for us
	NULL,				// Animation tick
	NULL,				// Calc intersection
	NULL,				// Mouse wheel
	NULL				// Set disable
};



ELCDErr TextCreate(TEXTHANDLE *peTextHandle,
				   WINDOWHANDLE eWindowHandle,
				   UINT16 u16Orientation)
{
	ELCDErr eLCDErr;
	SWidget *psWidget = NULL;

	if ((u16Orientation != ROT_0) &&
		(u16Orientation != ROT_90) &&
		(u16Orientation != ROT_180) &&
		(u16Orientation != ROT_270))
	{
		return(LERR_TEXT_BAD_ROTATION);
	}

	eLCDErr = WidgetAllocateHandle((WIDGETHANDLE *) peTextHandle,
								   WIDGET_TEXT,
								   eWindowHandle,
								   &psWidget);

	// Make sure the widget is enabled
	psWidget->bWidgetEnabled = TRUE;
	psWidget->uWidgetSpecific.psText->u16Orientation = u16Orientation;

	// Set things up
	psWidget->uWidgetSpecific.psText->u32MouseoverBoxColor = TEXT_COLOR_NONE;
	psWidget->uWidgetSpecific.psText->u32MouseoverTextColor = TEXT_COLOR_NONE;
	psWidget->uWidgetSpecific.psText->u32BoxColor = TEXT_COLOR_NONE;

	return(eLCDErr);
}

static void TextRecalcTextSize(SText *psText)
{
	// Make sure the critical pointers are valid
	GCASSERT(psText);
	GCASSERT(psText->psWidget);

	// Ignore the return code - errors are OK, here.
	(void) FontGetStringSize(psText->eFont,
							 psText->peText,
							 &psText->psWidget->u32XSize,
							 &psText->psWidget->u32YSize,
							 psText->u16Orientation,
							 TRUE);

	// Add in the text offset (if any) to get the overall X/Y size
	psText->psWidget->u32XSize += psText->u32TextXOffset;
	psText->psWidget->u32YSize += psText->u32TextYOffset;

	// If we have X/Y clipping on text, clip it
	if (psText->u32TextXClip)
	{
		if (psText->psWidget->u32XSize > psText->u32TextXClip)
		{
			psText->psWidget->u32XSize = psText->u32TextXClip;
		}
	}

	if (psText->u32TextYClip)
	{
		if (psText->psWidget->u32YSize > psText->u32TextYClip)
		{
			psText->psWidget->u32YSize = psText->u32TextYClip;
		}
	}

	// If a bounding box size is set, then clip it to the bounding box size
	if (psText->u32BoundingBoxXSize)
	{
		psText->psWidget->u32XSize = psText->u32BoundingBoxXSize;
	}
	if (psText->u32BoundingBoxYSize)
	{
		psText->psWidget->u32YSize = psText->u32BoundingBoxYSize;
	}
}

ELCDErr TextSetColor(TEXTHANDLE eTextHandle,
					 UINT16 u16Color)
{
	SText *psText;
	ELCDErr eErr = LERR_OK;
	SWidget *psWidget;

	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eTextHandle,
									WIDGET_TEXT,
									&psWidget,
									NULL);

	RETURN_ON_FAIL(eErr);

	psText = psWidget->uWidgetSpecific.psText;
	GCASSERT(psText);

	// If the color is unchagned, then don't change it
	if (psText->u16Color == u16Color)
	{
		goto errorExit;
	}

	eErr = WindowLockByHandleIfNotSubordinate(psText->psWidget->eParentWindow,
											  psText->psWidget);
	if (eErr != LERR_OK)
	{
		goto errorExit;
	}

	// Erase the current text image
	WidgetErase(psText->psWidget);

	// Wipe out the intersections
	WidgetEraseIntersections(psText->psWidget);

	// Paint any intersecting widgets
	WidgetPaintIntersections(psText->psWidget);

	psText->u16Color = u16Color;
	WidgetPaint(psText->psWidget,
				FALSE);

	eErr = WindowUnlockByHandleIfNotSubordinate(psText->psWidget->eParentWindow,
											    psText->psWidget);
	if (eErr != LERR_OK)
	{
		goto errorExit;
	}

	// Commit it
	WindowUpdateRegionCommit();

	// Color successfully changed
errorExit:
	return(eErr);
}

ELCDErr TextSetFont(TEXTHANDLE eTextHandle,
					FONTHANDLE eFont)
{
	SText *psText;
	SFont *psFont;
	ELCDErr eErr;
	SWidget *psWidget;

	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eTextHandle,
									WIDGET_TEXT,
									&psWidget,
									NULL);

	RETURN_ON_FAIL(eErr);

	psText = psWidget->uWidgetSpecific.psText;
	GCASSERT(psText);

	psFont = FontGetPointer(eFont);
	if (NULL == psFont)
	{
		return(LERR_FONT_BAD_HANDLE);
	}

	// Is this the same font we already have? If so, just return
	if (eFont == psText->eFont)
	{
		return(LERR_OK);
	}

	eErr = WindowLockByHandleIfNotSubordinate(psText->psWidget->eParentWindow,
											  psText->psWidget);
	if (eErr != LERR_OK)
	{
		goto errorExit;
	}

	// New font. Let's erase the old stuff
	WidgetErase(psText->psWidget);

	// Wipe out the intersections
	WidgetEraseIntersections(psText->psWidget);

	// Repaint intersections
	WidgetPaintIntersections(psText->psWidget);

	// Hook up the new font
	psText->eFont = eFont;

	eErr = FontIncRef(psText->eFont);
	GCASSERT(LERR_OK == eErr);

	// Calculate the new drawing size
	TextRecalcTextSize(psText);

	// Recompute the intersections to this widget
	eErr = WidgetCalcIntersections(psText->psWidget);

	// Now go repaint the image
	WidgetPaint(psText->psWidget,
				FALSE);

	if (LERR_OK == eErr)
	{
		eErr = WindowUnlockByHandleIfNotSubordinate(psText->psWidget->eParentWindow,
													psText->psWidget);
	}
	else
	{
		(void) WindowUnlockByHandleIfNotSubordinate(psText->psWidget->eParentWindow,
													psText->psWidget);
	}

	// Commit it
	WindowUpdateRegionCommit();

errorExit:
	return(eErr);
}

// ASSUMPTION: Caller must free the text allocated here
ELCDErr TextGetText(TEXTHANDLE eTextHandle, LEX_CHAR** ppeText)
{
	SText *psText;
	TEXTCHAR *peNewText = NULL;
	UINT32 u32TextCharLen = 0;
	ELCDErr eErr;
	SWidget *psWidget;

	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eTextHandle,
									WIDGET_TEXT,
									&psWidget,
									NULL);

	RETURN_ON_FAIL(eErr);

	psText = psWidget->uWidgetSpecific.psText;
	GCASSERT(psText);

	// Get the character length
	if( psText->peText )
	{
		u32TextCharLen = Lexstrlen((LEX_CHAR *) psText->peText);
	}

	if (u32TextCharLen)
	{
		peNewText = MemAlloc((u32TextCharLen + 1) * sizeof(*psText->peText));
		if (NULL == peNewText)
		{
			// Out of memory
			return(LERR_NO_MEM);
		}

		// Copy the incoming string to the new string
		memcpy((void *) peNewText, psText->peText, (u32TextCharLen + 1) * sizeof(*psText->peText));

		*ppeText = peNewText;
	}
	else
	{
		*ppeText = NULL;
	}

	return(LERR_OK);
}

ELCDErr TextSetText(TEXTHANDLE eTextHandle,
					TEXTCHAR *peText)
{
	SText *psText;
	TEXTCHAR *peNewText = NULL;
	UINT32 u32TextCharLen = 0;
	ELCDErr eErr;
	SWidget *psWidget;

	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eTextHandle,
									WIDGET_TEXT,
									&psWidget,
									NULL);

	RETURN_ON_FAIL(eErr);

	psText = psWidget->uWidgetSpecific.psText;
	GCASSERT(psText);

	// If the text is the same, don't do anything
	if (peText && psText->peText)
	{
		if (Lexstrcmp((LEX_CHAR *) peText, (LEX_CHAR *) psText->peText) == 0)
		{
			return(LERR_OK);
		}
	}

	// Get the character length
	if( peText )
	{
		u32TextCharLen = Lexstrlen((LEX_CHAR *) peText);
	}

	if (u32TextCharLen)
	{
		peNewText = MemAlloc((u32TextCharLen + 1) * sizeof(*peText));
		if (NULL == peNewText)
		{
			// Out of memory
			return(LERR_NO_MEM);
		}

		// Copy the incoming string to the new string
		memcpy((void *) peNewText, peText, (u32TextCharLen + 1) * sizeof(*peText));
	}

	eErr = WindowLockByHandleIfNotSubordinate(psText->psWidget->eParentWindow,
											  psText->psWidget);
	if (eErr != LERR_OK)
	{
		goto errorExit;
	}

	// Wipe out the old text
	WidgetErase(psText->psWidget);

	// Wipe out the intersections
	WidgetEraseIntersections(psText->psWidget);

	// Repaint the intersections
	WidgetPaintIntersections(psText->psWidget);

	// Free the old memory
	if (psText->peText)
	{
		GCFreeMemory(psText->peText);
	}

	// Assign the new text
	psText->peText = peNewText;

	// Calculate the new drawing size
	TextRecalcTextSize(psText);

	// Calculate the new intersections
	eErr = WidgetCalcIntersections(psText->psWidget);

	// Now draw it
	WidgetPaint(psText->psWidget,
				FALSE);

	if (LERR_OK == eErr)
	{
		eErr = WindowUnlockByHandleIfNotSubordinate(psText->psWidget->eParentWindow,
													psText->psWidget);
	}
	else
	{
		(void) WindowUnlockByHandleIfNotSubordinate(psText->psWidget->eParentWindow,
													psText->psWidget);
	}

	// Commit it
	WindowUpdateRegionCommit();

errorExit:
	return(eErr);
}

ELCDErr TextSetTextASCII(TEXTHANDLE eTextHandle,
						 char *pu8Text)
{
	SText *psText;
	TEXTCHAR *peNewText = NULL;
	UINT32 u32TextCharLen = 0;
	UINT32 u32Loop = 0;
	ELCDErr eErr;
	BOOL bEqual = TRUE;
	BOOL bComparing;
	SWidget *psWidget;

	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eTextHandle,
									WIDGET_TEXT,
									&psWidget,
									NULL);

	RETURN_ON_FAIL(eErr);

	psText = psWidget->uWidgetSpecific.psText;
	GCASSERT(psText);

	// Get the character length
	if (pu8Text)
	{
		u32TextCharLen = strlen(pu8Text);
	}

	if (u32TextCharLen)
	{
		peNewText = MemAlloc((u32TextCharLen + 1) * sizeof(*peNewText));
		if (NULL == peNewText)
		{
			// Out of memory
			return(LERR_NO_MEM);
		}

		bEqual = TRUE;
		bComparing = TRUE;
		if (NULL == psText->peText)
		{
			bComparing = FALSE;

			if (pu8Text)
			{
				bEqual = FALSE;
			}
		}
		else
		{
			// If they are different lengths, they are automatically not the same
			if( u32TextCharLen != Lexstrlen(psText->peText) )
			{
				bComparing = FALSE;
				bEqual = FALSE;
			}
		}

		// Copy the incoming string to the new string, but we need to promote it to TEXTCHAR
		for (u32Loop = 0; u32Loop < u32TextCharLen ;u32Loop++)
		{
			if (bComparing)
			{
				if (*pu8Text != psText->peText[u32Loop])
				{
					bComparing = FALSE;
					bEqual = FALSE;
				}

				if (('\0' == *pu8Text) ||
					(0 == psText->peText[u32Loop]))
				{
					if (*pu8Text != psText->peText[u32Loop])
					{
						bEqual = FALSE;
					}

					bComparing = FALSE;
				}
			}
			else
			{
				// No longer comparing
			}

			peNewText[u32Loop] = *pu8Text;
			pu8Text++;
		}
	}
	else
	{
		if (psText->peText)
		{
			GCFreeMemory(psText->peText);
			psText->peText = NULL;
		}
	}

	// If it's the same string that's already there, then bail out
	if (bEqual)
	{
		// Just deallocate and return - they're the same
		if (peNewText)
		{
			GCFreeMemory(peNewText);
		}
		return(LERR_OK);
	}

	eErr = WindowLockByHandleIfNotSubordinate(psText->psWidget->eParentWindow,
											  psText->psWidget);
	if (eErr != LERR_OK)
	{
		goto errorExit;
	}

	// Wipe out the old text
	WidgetErase(psText->psWidget);

	// Free the old memory
	if (psText->peText)
	{
		GCFreeMemory(psText->peText);
	}

	// Wipe out the intersections
	WidgetEraseIntersections(psText->psWidget);

	// Repaint the intersections
	WidgetPaintIntersections(psText->psWidget);

	// Assign the new text
	psText->peText = peNewText;

	// Calculate the new drawing size
	TextRecalcTextSize(psText);

	// Calculate the new intersections
	eErr = WidgetCalcIntersections(psText->psWidget);

	// Now draw it
	WidgetPaint(psText->psWidget,
				FALSE);

	if (LERR_OK == eErr)
	{
		eErr = WindowUnlockByHandleIfNotSubordinate(psText->psWidget->eParentWindow,
													psText->psWidget);
	}
	else
	{
		(void) WindowUnlockByHandleIfNotSubordinate(psText->psWidget->eParentWindow,
													psText->psWidget);
	}

	// Commit it
	WindowUpdateRegionCommit();

errorExit:
	return(eErr);
}

#define	MAX_FILENAME_LENGTH	256

#define	DEFAULT_FIXED_FONT_FILENAME				"archive:fonts/monofont.ttf"
//#define	DEFAULT_FIXED_FONT_FILENAME				"archive:fonts/COURIER.ttf"
#define DEFAULT_FIXED_FONT_POINTS				10

#define	DEFAULT_PROPORTIONAL_FONT_FILENAME		"archive:fonts/unispace.ttf"
#define DEFAULT_PROPORTIONAL_FONT_POINTS		10

// Fixed point system font
static LEX_CHAR *sg_pu8FixedFontFilename = NULL;
static UINT32 sg_u32FixedFontSize = DEFAULT_FIXED_FONT_POINTS;

// Proportional system font
static LEX_CHAR *sg_pu8ProportionalFontFilename = NULL;
static UINT32 sg_u32ProportionalFontSize = DEFAULT_PROPORTIONAL_FONT_POINTS;

// Active system font
static LEX_CHAR *sg_pu8ActiveFontFilename;
static UINT32 sg_u32ActiveFontSize;
static UINT16 sg_u16ActiveFontColor;

void TextGetActiveFontData(LEX_CHAR **ppu8ActiveFontFilename,
						   UINT32 *pu32ActiveFontSize)
{
	if (ppu8ActiveFontFilename)
	{
		*ppu8ActiveFontFilename = sg_pu8ActiveFontFilename;
	}

	if (pu32ActiveFontSize)
	{
		*pu32ActiveFontSize = sg_u32ActiveFontSize;
	}
}

ELCDErr TextSetActiveFontData(LEX_CHAR *peActiveFontFilename,
							  UINT32 u32ActiveFontSize)
{
	LEX_CHAR *peActiveFontTmp = NULL;

	if (peActiveFontFilename)
	{
		peActiveFontTmp = Lexstrdup(peActiveFontFilename);
		if (NULL == peActiveFontTmp)
		{
			return(LERR_NO_MEM);
		}

		if (sg_pu8ActiveFontFilename)
		{
			GCFreeMemory(sg_pu8ActiveFontFilename);
		}

		sg_pu8ActiveFontFilename = peActiveFontTmp;
	}

	sg_u32ActiveFontSize = u32ActiveFontSize;
	return(LERR_OK);
}

void TextGetFixedFontData(LEX_CHAR **ppeFixedFontFilename,
						  UINT32 *pu32FixedFontSize)
{
	if (ppeFixedFontFilename)
	{
		*ppeFixedFontFilename = sg_pu8FixedFontFilename;
	}

	if (pu32FixedFontSize)
	{
		*pu32FixedFontSize = sg_u32FixedFontSize;
	}
}

void TextGetProportionalFontData(LEX_CHAR **ppeProportionalFontFilename,
								 UINT32 *pu32ProportionalFontSize)
{
	if (ppeProportionalFontFilename)
	{
		*ppeProportionalFontFilename = sg_pu8ProportionalFontFilename;
	}

	if (pu32ProportionalFontSize)
	{
		*pu32ProportionalFontSize = sg_u32ProportionalFontSize;
	}
}

UINT16 TextGetActiveFontColor(void)
{
	return(sg_u16ActiveFontColor);
}

ELCDErr TextGetFont(TEXTHANDLE eTextHandle,
					FONTHANDLE *peFont)
{
	SText *psText;
	TEXTCHAR *peNewText = NULL;
	UINT32 u32TextCharLen = 0;
	UINT32 u32Loop = 0;
	SWidget *psWidget;
	ELCDErr eErr;

	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eTextHandle,
									WIDGET_TEXT,
									&psWidget,
									NULL);

	RETURN_ON_FAIL(eErr);

	psText = psWidget->uWidgetSpecific.psText;
	GCASSERT(psText);

	if (peFont)
	{
		*peFont = psText->eFont;
	}

	return(LERR_OK);
}

ELCDErr TextSetFixedFont(LEX_CHAR *pu8Filename,
						 UINT32 *pu32FixedFontSize)
{
	LEX_CHAR *pu8NewFilename = NULL;

	if (NULL == pu8Filename)
	{
		// Set it to the factory
		pu8NewFilename = Lexstrdup((LEX_CHAR *) DEFAULT_FIXED_FONT_FILENAME);
	}
	else
	{
		pu8NewFilename = Lexstrdup(pu8Filename);
	}

	if (NULL == pu8NewFilename)
	{
		return(LERR_NO_MEM);
	}

	GCASSERT(sg_pu8FixedFontFilename);
	GCFreeMemory(sg_pu8FixedFontFilename);
	sg_pu8FixedFontFilename = pu8NewFilename;

	if (pu32FixedFontSize)
	{
		sg_u32FixedFontSize = *pu32FixedFontSize;
	}
	else
	{
		sg_u32FixedFontSize = DEFAULT_FIXED_FONT_POINTS;
	}

	return(LERR_OK);
}

ELCDErr TextSetProportionalFont(LEX_CHAR *pu8Filename,
								UINT32 *pu32FixedFontSize)
{
	LEX_CHAR *pu8NewFilename = NULL;

	if (NULL == pu8Filename)
	{
		// Set it to the factory
		pu8NewFilename = Lexstrdup((LEX_CHAR *) DEFAULT_PROPORTIONAL_FONT_FILENAME);
	}
	else
	{
		pu8NewFilename = Lexstrdup(pu8Filename);
	}

	if (NULL == pu8NewFilename)
	{
		return(LERR_NO_MEM);
	}

	if (sg_pu8ProportionalFontFilename)
	{
		GCASSERT(sg_pu8ProportionalFontFilename);
		GCFreeMemory(sg_pu8ProportionalFontFilename);
	}

	sg_pu8ProportionalFontFilename = pu8NewFilename;

	if (pu32FixedFontSize)
	{
		sg_u32ProportionalFontSize = *pu32FixedFontSize;
	}
	else
	{
		sg_u32ProportionalFontSize = DEFAULT_PROPORTIONAL_FONT_POINTS;
	}

	return(LERR_OK);
}

ELCDErr TextSetBoundingBoxSize(TEXTHANDLE eTextHandle,
							   UINT32 u32XSize,
							   UINT32 u32YSize)
{
	ELCDErr eErr;
	SWidget *psWidget;
	SText *psText;

	eErr = WidgetUpdatePreamble(eTextHandle,
								&psWidget);
	RETURN_ON_FAIL(eErr);

	psText = psWidget->uWidgetSpecific.psText;
	GCASSERT(psText);

	psText->u32BoundingBoxXSize = u32XSize;
	psText->u32BoundingBoxYSize = u32YSize;

	// Calculate the new drawing size
	TextRecalcTextSize(psText);

	eErr = WidgetUpdateFinished(eTextHandle);
	return(eErr);
}

ELCDErr TextSetTextOffset(TEXTHANDLE eTextHandle,
								 UINT32 u32XOffset,
								 UINT32 u32YOffset)
{
	ELCDErr eErr;
	SWidget *psWidget;
	SText *psText;

	eErr = WidgetUpdatePreamble(eTextHandle,
								&psWidget);
	RETURN_ON_FAIL(eErr);

	psText = psWidget->uWidgetSpecific.psText;
	GCASSERT(psText);

	psText->u32TextXOffset = u32XOffset;
	psText->u32TextYOffset = u32YOffset;
	
	// Calculate the new drawing size
	TextRecalcTextSize(psText);

	eErr = WidgetUpdateFinished(eTextHandle);
	return(eErr);
}

ELCDErr TextSetClipBox(TEXTHANDLE eTextHandle,
					   UINT32 u32XClip,
					   UINT32 u32YClip)
{
	ELCDErr eErr;
	SWidget *psWidget;
	SText *psText;

	eErr = WidgetUpdatePreamble(eTextHandle,
								&psWidget);
	RETURN_ON_FAIL(eErr);

	psText = psWidget->uWidgetSpecific.psText;
	GCASSERT(psText);

	psText->u32TextXClip = u32XClip;
	psText->u32TextYClip = u32YClip;

	// Calculate the new drawing size
	TextRecalcTextSize(psText);

	eErr = WidgetUpdateFinished(eTextHandle);
	return(eErr);
}

ELCDErr TextSetBoundingBoxColor(TEXTHANDLE eTextHandle,
								UINT32 u32BoxColor,
								UINT32 u32MouseoverBoxColor)
{
	ELCDErr eErr;
	SWidget *psWidget;
	SText *psText;

	eErr = WidgetUpdatePreamble(eTextHandle,
								&psWidget);
	RETURN_ON_FAIL(eErr);

	psText = psWidget->uWidgetSpecific.psText;
	GCASSERT(psText);

	psText->u32BoxColor = u32BoxColor;
	psText->u32MouseoverBoxColor = u32MouseoverBoxColor;

	eErr = WidgetUpdateFinished(eTextHandle);
	return(eErr);
}

ELCDErr TextSetMouseoverColor(TEXTHANDLE eTextHandle,
							  UINT32 u32MouseoverTextColor)
{
	ELCDErr eErr;
	SWidget *psWidget;
	SText *psText;

	eErr = WidgetUpdatePreamble(eTextHandle,
								&psWidget);
	RETURN_ON_FAIL(eErr);

	psText = psWidget->uWidgetSpecific.psText;
	GCASSERT(psText);

	psText->u32MouseoverTextColor = u32MouseoverTextColor;

	eErr = WidgetUpdateFinished(eTextHandle);
	return(eErr);
}

static SWidgetTypeMethods sg_sTextMethods = 
{
	&sg_sTextWidgetFunctions,
	LERR_TEXT_BAD_HANDLE,			// Error when it's not the handle we're looking for
	TextWidgetAlloc,
	TextWidgetFree
};

void TextFirstTimeInit(void)
{
	DebugOut("* Initializing text widget\n");

	sg_u32ActiveFontSize = sg_u32FixedFontSize;
	sg_u16ActiveFontColor = 0xffff;				// White!

	WidgetRegisterTypeMethods(WIDGET_TEXT,
							  &sg_sTextMethods);
}