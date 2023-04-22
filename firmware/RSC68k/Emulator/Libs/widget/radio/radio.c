#include "Startup/app.h"
#include "Libs/Gfx/GraphicsLib.h"
#include "Libs/window/window.h"
#include "Libs/widget/widget.h"
#include "Libs/widget/radio/radio.h"

static ELCDErr RadioWidgetAlloc(SWidget *psWidget,
								WIDGETHANDLE eHandle)
{
	GCASSERT(psWidget);
	psWidget->uWidgetSpecific.psRadio = MemAlloc(sizeof(*psWidget->uWidgetSpecific.psRadio));
	if (NULL == psWidget->uWidgetSpecific.psRadio)
	{
		return(LERR_NO_MEM);
	}
	else
	{
		psWidget->uWidgetSpecific.psRadio->eRadioGroupHandle = (RADIOGROUPHANDLE) eHandle;
		psWidget->uWidgetSpecific.psRadio->eSoundHandle = HANDLE_INVALID;
		psWidget->uWidgetSpecific.psRadio->psWidget = psWidget;
		return(LERR_OK);
	}
}

static ELCDErr RadioWidgetFree(SWidget *psWidget)
{
	SRadio *psRadio = NULL;
	SRadioItem *psRadioItem = NULL;
	SRadioItem *psRadioItemPrior = NULL;

	GCASSERT(psWidget);
	psRadio = psWidget->uWidgetSpecific.psRadio;
	GCASSERT(psRadio);

	(void) WaveDestroy(psRadio->eSoundHandle);
	psRadio->eSoundHandle = HANDLE_INVALID;

	psRadioItem = psRadio->psItemList;

	while (psRadioItem)
	{
		psRadioItemPrior = psRadioItem;
		psRadioItem = psRadioItem->psNextLink;

		if (psRadioItemPrior->peText)
		{
			GCFreeMemory(psRadioItemPrior->peText);
			psRadioItemPrior->peText = NULL;
		}

		if ((psRadioItemPrior->psSelectedImage) && (psRadioItemPrior->psSelectedImage != psRadio->psDefaultSelected))
		{
			GfxDeleteImageGroup(psRadioItemPrior->psSelectedImage);
			psRadioItemPrior->psSelectedImage = NULL;
		}

		if ((psRadioItemPrior->psNonselectedImage) && (psRadioItemPrior->psNonselectedImage != psRadio->psDefaultSelected))
		{
			GfxDeleteImageGroup(psRadioItemPrior->psNonselectedImage);
			psRadioItemPrior->psNonselectedImage = NULL;
		}

		(void) FontFree(psRadioItemPrior->eFontHandle);
		psRadioItemPrior->eFontHandle = HANDLE_INVALID;

		GCFreeMemory(psRadioItemPrior);
	}

	if (psRadio->psDefaultSelected)
	{
		GfxDeleteImageGroup(psRadio->psDefaultSelected);
		psRadio->psDefaultSelected = NULL;
	}

	if (psRadio->psDefaultNonselected)
	{
		GfxDeleteImageGroup(psRadio->psDefaultNonselected);
		psRadio->psDefaultNonselected = NULL;
	}

	MemFree(psWidget->uWidgetSpecific.psRadio);
	psWidget->uWidgetSpecific.psRadio = NULL;
	return(LERR_OK);
}

static SImageGroup *RadioGetImageGroup(SRadio *psRadio,
									   SRadioItem *psRadioItem,
									   BOOL bSelected)
{
	if (bSelected)
	{
		if (psRadioItem->psSelectedImage)
		{
			return(psRadioItem->psSelectedImage);
		}
		else
		{
			GCASSERT(psRadio->psDefaultSelected);
			return(psRadio->psDefaultSelected);
		}
	}
	else
	{
		if (psRadioItem->psNonselectedImage)
		{
			return(psRadioItem->psNonselectedImage);
		}
		else
		{
			GCASSERT(psRadio->psDefaultNonselected);
			return(psRadio->psDefaultNonselected);
		}
	}
}

static SRadioItem *RadioCheckHit(SWidget *psWidget,
								 SRadio *psRadio,
								 UINT32 u32XPos,
								 UINT32 u32YPos)
{
	SRadioItem *psRadioItem = NULL;

	GCASSERT(psRadio);

	psRadioItem = psRadio->psItemList;
	u32XPos -= (UINT32) psWidget->s32XPos;
	u32YPos -= (UINT32) psWidget->s32YPos;

	while (psRadioItem)
	{
		UINT32 u32XCalcAdjusted;
		UINT32 u32YCalcAdjusted;

		u32XCalcAdjusted = psRadioItem->u32XOffsetCalculated - psWidget->s32XPos;
		u32YCalcAdjusted = psRadioItem->u32YOffsetCalculated - psWidget->s32YPos;

		// If it's within the x/y position and size of the bounding box for
		// text and graphics and it's not disabled, and it's visible, and it's
		// not already selected, THEN we have a hit
		if ((u32XPos >= (u32XCalcAdjusted)) && 
			(u32XPos < (u32XCalcAdjusted + psRadioItem->u32XSize)) &&
			(u32YPos >= (u32YCalcAdjusted)) &&
			(u32YPos < (u32YCalcAdjusted + psRadioItem->u32YSize)) &&
			(FALSE == psRadioItem->bDisabled) &&
			(psRadioItem->bVisible))
		{
			if ((psRadioItem->bCheckbox) ||
				(FALSE == psRadioItem->bSelected))
			{
				return(psRadioItem);
			}
		}

		psRadioItem = psRadioItem->psNextLink;
	}

	return(NULL);
}

static BOOL RadioHitTest(SWidget *psWidget,
						  UINT32 u32XPos, 
						  UINT32 u32YPos)
{
	SRadio *psRadio = psWidget->uWidgetSpecific.psRadio;
	SRadioItem *psRadioItem = NULL;

	if (FALSE == psRadio->bEnabled)
	{
		return(FALSE);
	}

	psRadioItem = RadioCheckHit(psWidget,
								psWidget->uWidgetSpecific.psRadio,
								u32XPos,
								u32YPos);

	// Couldn't find it
	if (NULL == psRadioItem)
	{
		return(FALSE);
	}

	// Don't need to take any corrective action, here. Just return so that our
	// new widget is hit
	return(TRUE);
}

static void RadioItemDraw(SWidget *psWidget,
						  SWindow *psWindow,
						  SRadio *psRadio,
						  SRadioItem *psRadioItem)
{
	SImageGroup *psImageGroup;
	SImage *psImage;
	LEX_CHAR *peString;
	INT32 s32TextXPos = 0;
	INT32 s32TextYPos = 0;
	UINT32 u32XBase = 0;
	UINT32 u32YBase = 0;
	BOOL bDisabled = FALSE;
	UINT32 u32XGraphic = 0;
	UINT32 u32YGraphic = 0;
	UINT32 u32CharCount = 0;

	if ((FALSE == psRadio->bEnabled) ||
		(psRadioItem->bDisabled))
	{
		bDisabled = TRUE;
	}

	peString = psRadioItem->peText;
	u32CharCount = Lexstrlen(peString);

	psImageGroup = RadioGetImageGroup(psRadio,
									  psRadioItem,
									  psRadioItem->bSelected);
	GCASSERT(psImageGroup);
	psImage = psImageGroup->psCurrentImage;
	GCASSERT(psImage);

	if (ROT_0 == psRadioItem->u16Orientation)
	{
		if (FALSE == psRadio->bRightBottomJustified)
		{
			u32XGraphic = psRadioItem->u32XOffsetCalculated;

			s32TextXPos = (INT32) (psRadioItem->u32XOffsetCalculated + psImage->u32XSize);
		}
		else
		{
			u32XGraphic = psRadioItem->u32XOffsetCalculated + psRadioItem->u32TextXSize;

			s32TextXPos = (INT32) (psRadioItem->u32XOffsetCalculated);
		}

		s32TextYPos = (INT32) (psRadioItem->u32YOffsetCalculated);
		u32YGraphic = psRadioItem->u32YOffsetCalculated;
	}
	else
	if (ROT_90 == psRadioItem->u16Orientation)
	{
		if (FALSE == psRadio->bRightBottomJustified)
		{
			u32YGraphic = psRadioItem->u32YOffsetCalculated;

			s32TextYPos = (INT32) (psRadioItem->u32YOffsetCalculated + psImage->u32YSize);
		}
		else
		{
			u32YGraphic = psRadioItem->u32YOffsetCalculated + psRadioItem->u32TextYSize;

			s32TextYPos = (INT32) (psRadioItem->u32YOffsetCalculated);
		}

		s32TextXPos = (INT32) (psRadioItem->u32XOffsetCalculated);
		u32XGraphic = psRadioItem->u32XOffsetCalculated;
	}
	else
	if (ROT_180 == psRadioItem->u16Orientation)
	{
		if (FALSE == psRadio->bRightBottomJustified)
		{
			u32XGraphic = psRadioItem->u32XOffsetCalculated + psRadioItem->u32TextXSize;

			s32TextXPos = (INT32) (psRadioItem->u32XOffsetCalculated);
		}
		else
		{
			u32XGraphic = psRadioItem->u32XOffsetCalculated;

			s32TextXPos = (INT32) (psRadioItem->u32XOffsetCalculated + psImage->u32XSize);
		}

		s32TextYPos = (INT32) (psRadioItem->u32YOffsetCalculated);
		u32YGraphic = psRadioItem->u32YOffsetCalculated;

		peString += (u32CharCount - 1);
	}
	else
	if (ROT_270 == psRadioItem->u16Orientation)
	{
		if (FALSE == psRadio->bRightBottomJustified)
		{
			u32YGraphic = psRadioItem->u32YOffsetCalculated + psRadioItem->u32TextYSize;

			s32TextYPos = (INT32) (psRadioItem->u32YOffsetCalculated);
		}
		else
		{
			u32YGraphic = psRadioItem->u32YOffsetCalculated;

			s32TextYPos = (INT32) (psRadioItem->u32YOffsetCalculated + psImage->u32YSize);
		}

		s32TextXPos = (INT32) (psRadioItem->u32XOffsetCalculated);
		u32XGraphic = psRadioItem->u32XOffsetCalculated;

		peString += (u32CharCount - 1);
	}
	else
	{
		GCASSERT(0);
	}

	// Draw the graphical indicator
	if ((FALSE == psRadioItem->bDisabled) &&
		(psRadio->bEnabled))
	{
		GfxBlitImageToImage(psWindow->psWindowImage,
							psImage,
							u32XGraphic + psWindow->u32ActiveAreaXPos,
							u32YGraphic + psWindow->u32ActiveAreaYPos,
							psWindow->u32ActiveAreaXPos + psWindow->u32ActiveAreaXSize,
							psWindow->u32ActiveAreaYPos + psWindow->u32ActiveAreaYSize);
	}
	else
	{
		GfxBlitImageToImageGray(psWindow->psWindowImage,
								psImage,
								u32XGraphic + psWindow->u32ActiveAreaXPos,
								u32YGraphic + psWindow->u32ActiveAreaYPos,
								psWindow->u32ActiveAreaXPos + psWindow->u32ActiveAreaXSize,
								psWindow->u32ActiveAreaYPos + psWindow->u32ActiveAreaYSize);
	}

	// This update should not contain the active area XPos/YPos additions
	WindowUpdateRegion(psWidget->eParentWindow,
					   u32XGraphic + psWindow->u32ActiveAreaXPos,
					   u32YGraphic + psWindow->u32ActiveAreaYPos,
					   psImage->u32XSize,
					   psImage->u32YSize);

	u32XBase = (UINT32) s32TextXPos;
	u32YBase = (UINT32) s32TextYPos;

	s32TextXPos <<= 6;
	s32TextYPos <<= 6;

	while (u32CharCount)
	{
		INT32 s32XAdvance;
		INT32 s32YAdvance;

		(void) FontRender(psRadioItem->eFontHandle,
						  psWidget->eParentWindow,
						  s32TextXPos,
						  s32TextYPos,
						  -1,
						  -1,
						  (TEXTCHAR) *peString,
						  CONVERT_24RGB_16RGB(psRadioItem->u32TextColor),
						  TRUE,
						  &s32XAdvance,
						  &s32YAdvance,
						  TRUE,
						  (ERotation) psRadioItem->u16Orientation,
						  NULL,
						  bDisabled,
						  TRUE);

		if (ROT_0 == psRadioItem->u16Orientation)
		{
			s32TextXPos += s32XAdvance;
			++peString;
		}
		else
		if (ROT_90 == psRadioItem->u16Orientation)
		{
			s32TextYPos += s32YAdvance;
			++peString;
		}
		else
		if (ROT_180 == psRadioItem->u16Orientation)
		{
			s32TextXPos -= s32XAdvance;
			--peString;
		}
		else
		if (ROT_270 == psRadioItem->u16Orientation)
		{
			s32TextYPos -= s32YAdvance;
			--peString;
		}
		else
		{
			GCASSERT(0);
		}

		--u32CharCount;
	}

	WindowUpdateRegion(psWidget->eParentWindow,
					   u32XBase + psWindow->u32ActiveAreaXPos,
					   u32YBase + psWindow->u32ActiveAreaYPos,
					   (UINT32) (((UINT32) s32TextXPos) - u32XBase),
					   (UINT32) (((UINT32) s32TextYPos) - u32YBase));
}

static void RadioItemErase(SWidget *psWidget,
						   SWindow *psWindow,
						   SRadioItem *psRadioItem)
{
	WindowEraseActiveRegion(psWindow,
							psRadioItem->u32XOffsetCalculated,
							psRadioItem->u32YOffsetCalculated,
							psRadioItem->u32XSize,
							psRadioItem->u32YSize);

	WindowUpdateRegion(psWidget->eParentWindow,
					   psRadioItem->u32XOffsetCalculated + psWindow->u32ActiveAreaXPos,
					   psRadioItem->u32YOffsetCalculated + psWindow->u32ActiveAreaYPos,
					   psRadioItem->u32XSize,
					   psRadioItem->u32YSize);
}

static void RadioPaint(SWidget *psWidget,
					   BOOL bLock)
{
	SRadio *psRadio = psWidget->uWidgetSpecific.psRadio;
	SRadioItem *psRadioItem = NULL;
	SWindow *psWindow;
	
	GCASSERT(psRadio);
	psWindow = WindowGetPointer(psWidget->eParentWindow);
	GCASSERT(psWindow);

	psRadioItem = psRadio->psItemList;

	while (psRadioItem)
	{
		RadioItemDraw(psWidget,
					  psWindow,
					  psRadio,
					  psRadioItem);
		psRadioItem = psRadioItem->psNextLink;
	}
}

static void RadioErase(SWidget *psWidget)
{
	WidgetEraseStandard(psWidget);
}

static void RadioPress(SWidget *psWidget,
					   UINT32 u32Mask,
					   UINT32 u32XPos,
					   UINT32 u32YPos)
{
	SRadio *psRadio = psWidget->uWidgetSpecific.psRadio;
	SRadioItem *psRadioItem = NULL;
	SWindow *psWindow;
	ELCDErr eErr = LERR_OK;

	psRadioItem = RadioCheckHit(psWidget,
								psWidget->uWidgetSpecific.psRadio,
								u32XPos,
								u32YPos);

	// Couldn't find it
	if (NULL == psRadioItem)
	{
		return;
	}

	// And play a sound if applicable
	WidgetSoundPlay(psRadio->eSoundHandle);


	psWindow = WindowGetPointer(psWidget->eParentWindow);
	GCASSERT(psWindow);

	// We've got a hit! Time for an update.
	eErr = WindowLockByHandle(psWidget->eParentWindow);
	GCASSERT(GC_OK == eErr);

	WidgetSetUpdate(TRUE);

	// Erase the old and new positions but only if it's not a checkbox
	if (psRadio->psItemSelected && FALSE == psRadio->psItemSelected->bCheckbox)
	{
		RadioItemErase(psWidget,
					   psWindow,
					   psRadio->psItemSelected);
	}

	RadioItemErase(psWidget,
				   psWindow,
				   psRadioItem);

	// Mark the old position as not selected
	if (FALSE == psRadio->psItemSelected->bCheckbox)
	{
		// Only do this if it's a radio button
		psRadio->psItemSelected->bSelected = FALSE;
	}

	// Mark the new position as selected
	if (psRadioItem->bCheckbox)
	{
		// Toggle if it's a checkbox
		if (psRadioItem->bSelected)
		{
			psRadioItem->bSelected = FALSE;
		}
		else
		{
			psRadioItem->bSelected = TRUE;
		}
	}
	else
	{
		psRadioItem->bSelected = TRUE;
	}

	// Draw the old position
	if (psRadio->psItemSelected && FALSE == psRadio->psItemSelected->bCheckbox)
	{
		RadioItemDraw(psWidget,
					  psWindow,
					  psRadio,
					  psRadio->psItemSelected);
	}

	// Now draw the new position
	RadioItemDraw(psWidget,
				  psWindow,
				  psRadio,
				  psRadioItem);

	psRadio->psItemSelected = psRadioItem;

	WidgetSetUpdate(FALSE);

	eErr= WindowUnlockByHandle(psWidget->eParentWindow);
	GCASSERT(GC_OK == eErr);

	WindowUpdateRegionCommit();
}

static SWidgetFunctions sg_sRadioFunctions =
{
	RadioHitTest,
	RadioPaint,
	RadioErase,
	RadioPress,
	NULL,
	NULL,						// Mouseover our widget
	NULL,						// Focus
	NULL,						// Keystroke for us
	NULL,						// Animation tick - none for now!
	NULL,						// Calc intersection
	NULL,						// Mouse wheel
	NULL						// Set disable
};

static SWidgetTypeMethods sg_sRadioMethods = 
{
	&sg_sRadioFunctions,
	LERR_RADIO_BAD_HANDLE,			// Error when it's not the handle we're looking for
	RadioWidgetAlloc,
	RadioWidgetFree
};

void RadioFirstTimeInit(void)
{
	DebugOut("* Initializing radio buttons\n");
	WidgetRegisterTypeMethods(WIDGET_RADIO,
							  &sg_sRadioMethods);
}

ELCDErr RadioGroupCreate(RADIOGROUPHANDLE *peRadioGroupHandle,
						 WINDOWHANDLE eWindowHandle,
						 UINT32 u32XPos,
						 UINT32 u32YPos,
						 BOOL bRightBottomJustified,
						 BOOL bVisible)
{
	ELCDErr eLCDErr;
	SWidget *psWidget = NULL;

	eLCDErr = WidgetAllocateHandle((WIDGETHANDLE *) peRadioGroupHandle,
								   WIDGET_RADIO,
								   eWindowHandle,
								   &psWidget);

	RETURN_ON_FAIL(eLCDErr);

	psWidget->uWidgetSpecific.psRadio->bRightBottomJustified = bRightBottomJustified;
	psWidget->s32XPos = (INT32) u32XPos;
	psWidget->s32YPos = (INT32) u32YPos;
	if (bVisible)
	{
		psWidget->bWidgetHidden = FALSE;
	}
	else
	{
		psWidget->bWidgetHidden = TRUE;
	}

	// Make sure the widget is enabled
	psWidget->bWidgetEnabled = TRUE;
	psWidget->uWidgetSpecific.psRadio->bEnabled = TRUE;

	return(eLCDErr);
}

ELCDErr RadioGroupSetDefaultImages(RADIOGROUPHANDLE eRadioGroupHandle,
								   SImageGroup *psDefaultSelectedImage,
								   SImageGroup *psDefaultNonselectedImage,
								   BOOL bLockWindow)
{
	ELCDErr eErr = LERR_OK;
	SWidget *psWidget;
	SRadio *psRadio = NULL;

	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eRadioGroupHandle,
									WIDGET_RADIO,
									&psWidget,
									NULL);
	RETURN_ON_FAIL(eErr);

	psRadio = psWidget->uWidgetSpecific.psRadio;
	GCASSERT(psRadio);

	if (psDefaultSelectedImage && psDefaultNonselectedImage)
	{
		if ((psDefaultSelectedImage->psCurrentImage->u32XSize != psDefaultNonselectedImage->psCurrentImage->u32XSize) ||
			(psDefaultSelectedImage->psCurrentImage->u32YSize != psDefaultNonselectedImage->psCurrentImage->u32YSize))
		{
			return(LERR_RADIO_SELECTED_IMAGE_SIZE_MISMATCH);
		}
	}

	// Lock the handle
	if (bLockWindow)
	{
		eErr = WindowLockByHandle(psRadio->psWidget->eParentWindow);
		if (eErr != LERR_OK)
		{
			// Can't touch it! 
			return(eErr);
		}
	}

	if (psRadio->psDefaultSelected)
	{
		GfxDeleteImageGroup(psRadio->psDefaultSelected);
		psRadio->psDefaultSelected = NULL;
	}

	if (psRadio->psDefaultNonselected)
	{
		GfxDeleteImageGroup(psRadio->psDefaultNonselected);
		psRadio->psDefaultNonselected = NULL;
	}

	psRadio->psDefaultSelected = psDefaultSelectedImage;
	if (psDefaultSelectedImage)
	{
		GfxIncRef(psDefaultSelectedImage);
	}

	psRadio->psDefaultNonselected = psDefaultNonselectedImage;
	if (psDefaultNonselectedImage)
	{
		GfxIncRef(psDefaultNonselectedImage);
	}

	eErr = WindowUnlockByHandle(psRadio->psWidget->eParentWindow);

	return(eErr);
}

ELCDErr RadioGroupSetHide(RADIOGROUPHANDLE eRadioGroupHandle,
						  UINT32 *pu32HideIndex,
						  BOOL bHidden)
{
	SRadio *psRadio;
	ELCDErr eErr;
	SWidget *psWidget;

	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eRadioGroupHandle,
									WIDGET_RADIO,
									&psWidget,
									NULL);
	RETURN_ON_FAIL(eErr);

	psRadio = psWidget->uWidgetSpecific.psRadio;
	GCASSERT(psRadio);

	if (NULL == pu32HideIndex)
	{
		eErr = WidgetSetHide(psRadio->psWidget,
							 bHidden,
							 FALSE);
		return(eErr);
	}
	else
	{
		SRadioItem *psRadioItem = psRadio->psItemList;
		UINT32 u32Loop = *pu32HideIndex;
		SWindow *psWindow = NULL;

		while (u32Loop-- && psRadioItem)
		{
			psRadioItem = psRadioItem->psNextLink;
		}

		if (NULL == psRadioItem)
		{
			// Out of range
			return(LERR_RADIO_ITEM_OUT_OF_RANGE);
		}

		if (bHidden)
		{
			bHidden = FALSE;
		}
		else
		{
			bHidden = TRUE;
		}

		if (psRadioItem->bVisible == bHidden)
		{
			// Nothing to do. Already there.
			return(LERR_OK);
		}

		// Time to play "lock the window"
		eErr = WindowLockByHandle(psRadio->psWidget->eParentWindow);
		GCASSERT(GC_OK == eErr);

		psWindow = WindowGetPointer(psRadio->psWidget->eParentWindow);
		GCASSERT(psWindow);

		WidgetSetUpdate(TRUE);

		psRadioItem->bVisible = bHidden;

		// Remember - "hidden" has been turned in to "visible"
		if (bHidden)
		{
			// We're painting the item
			RadioItemDraw(psRadio->psWidget,
						  psWindow,
						  psRadio,
						  psRadioItem);
		}
		else
		{
			// We're erasing the item
			RadioItemErase(psRadio->psWidget,
						   psWindow,
						   psRadioItem);
		}

		WidgetSetUpdate(FALSE);

		eErr = WindowUnlockByHandle(psRadio->psWidget->eParentWindow);
		GCASSERT(GC_OK == eErr);
		WindowUpdateRegionCommit();
	}

	return(eErr);
}

ELCDErr RadioGroupSetEnable(RADIOGROUPHANDLE eRadioGroupHandle,
							UINT32 *pu32EnableIndex,
							BOOL bEnable)
{
	SRadio *psRadio;
	ELCDErr eErr = LERR_OK;
	SWidget *psWidget;

	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eRadioGroupHandle,
									WIDGET_RADIO,
									&psWidget,
									NULL);
	RETURN_ON_FAIL(eErr);

	psRadio = psWidget->uWidgetSpecific.psRadio;
	GCASSERT(psRadio);

	if (NULL == pu32EnableIndex)
	{
		if (psRadio->bEnabled == bEnable)
		{
			// Nothing to do
			return(LERR_OK);
		}

		if (psRadio->bEnabled)
		{
			WidgetErase(psRadio->psWidget);
		}

		psRadio->bEnabled = bEnable;
		WidgetPaint(psRadio->psWidget,
					FALSE);
		WindowUpdateRegionCommit();
	}
	else
	{
		// This is hiding an individual item within a group
		SRadioItem *psRadioItem = psRadio->psItemList;
		UINT32 u32Loop = *pu32EnableIndex;
		SWindow *psWindow = NULL;

		while (u32Loop-- && psRadioItem)
		{
			psRadioItem = psRadioItem->psNextLink;
		}

		if (NULL == psRadioItem)
		{
			// Out of range
			return(LERR_RADIO_ITEM_OUT_OF_RANGE);
		}

		if (bEnable)
		{
			bEnable = FALSE;
		}
		else
		{
			bEnable = TRUE;
		}

		if (psRadioItem->bDisabled == bEnable)
		{
			// Nothing to do. Already there.
			return(LERR_OK);
		}

		// Time to play "lock the window"
		eErr = WindowLockByHandle(psRadio->psWidget->eParentWindow);
		GCASSERT(GC_OK == eErr);

		psWindow = WindowGetPointer(psRadio->psWidget->eParentWindow);
		GCASSERT(psWindow);

		WidgetSetUpdate(TRUE);

		psRadioItem->bDisabled = bEnable;

		// Remember - "enabled" has been turned in to "disabled"

		// We're erasing the item
		RadioItemErase(psRadio->psWidget,
					   psWindow,
					   psRadioItem);

		// We're painting the item
		RadioItemDraw(psRadio->psWidget,
					  psWindow,
					  psRadio,
					  psRadioItem);

		WidgetSetUpdate(FALSE);

		eErr = WindowUnlockByHandle(psRadio->psWidget->eParentWindow);
		GCASSERT(GC_OK == eErr);
		WindowUpdateRegionCommit();	
	}

	return(eErr);
}

ELCDErr RadioSetSound(RADIOGROUPHANDLE eRadioGroupHandle,
					  SOUNDHANDLE eSoundHandle)
{
	ELCDErr eErr;
	SWidget *psWidget;

	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eRadioGroupHandle,
									WIDGET_RADIO,
									&psWidget,
									NULL);
	RETURN_ON_FAIL(eErr);

	if ((eSoundHandle != HANDLE_INVALID) && NULL == WaveGetPointer(eSoundHandle))
	{
		return(LERR_SOUND_WAVE_BAD_HANDLE);
	}

	// Need to delete the existing sound (if there is one)
	(void) WaveDestroy(psWidget->uWidgetSpecific.psRadio->eSoundHandle);

	psWidget->uWidgetSpecific.psRadio->eSoundHandle = eSoundHandle;

	return(LERR_OK);

}

static void RadioButtonCalcSize(SRadioItem *psRadioItem,
								SRadio *psRadio)
{
	SImage *psImage = NULL;
	SImageGroup *psImageGroup = NULL;

	if (NULL == psRadioItem->psNonselectedImage)
	{
		psImage = psRadio->psDefaultNonselected->psCurrentImage;
	}
	else
	{
		psImage = psRadioItem->psNonselectedImage->psCurrentImage;
	}

	GCASSERT(psImage);

	if ((ROT_0 == psRadioItem->u16Orientation) ||
		(ROT_180 == psRadioItem->u16Orientation))
	{
		// left/right bottom/top justification doesn't matter
		psRadioItem->u32XSize = psRadioItem->u32TextXSize + psImage->u32XSize;
		psRadioItem->u32YSize = psRadioItem->u32TextYSize;
		if (psImage->u32YSize > psRadioItem->u32YSize)
		{
			psRadioItem->u32YSize = psImage->u32YSize;
		}
	}
	else
	if ((ROT_90 == psRadioItem->u16Orientation) ||
		(ROT_270 == psRadioItem->u16Orientation))
	{
		psRadioItem->u32YSize = psRadioItem->u32TextYSize + psImage->u32YSize;
		psRadioItem->u32XSize = psRadioItem->u32TextXSize;
		if (psImage->u32XSize > psRadioItem->u32XSize)
		{
			psRadioItem->u32XSize = psImage->u32XSize;
		}
	}
	else
	{
		GCASSERT(0);
	}
}

static void RadioButtonCalcPosition(SRadioItem *psRadioItem,
									SRadio *psRadio)
{
	SImage *psImage = NULL;
	SImageGroup *psImageGroup = NULL;

	if (NULL == psRadioItem->psNonselectedImage)
	{
		psImage = psRadio->psDefaultNonselected->psCurrentImage;
	}
	else
	{
		psImage = psRadioItem->psNonselectedImage->psCurrentImage;
	}

	GCASSERT(psImage);

	if (ROT_0 == psRadioItem->u16Orientation)
	{
		if (FALSE == psRadio->bRightBottomJustified)
		{
			psRadioItem->u32XOffsetCalculated = psRadioItem->u32XOffset + psRadio->psWidget->s32XPos;
		}
		else
		{
			psRadioItem->u32XOffsetCalculated = psRadioItem->u32XOffset + (psRadio->u32XMaxSize - psRadioItem->u32TextXSize - psImage->u32XSize) + psRadio->psWidget->s32XPos;
		}

		psRadioItem->u32YOffsetCalculated = psRadioItem->u32YOffset + psRadio->psWidget->s32YPos;
	}
	else
	if (ROT_90 == psRadioItem->u16Orientation)
	{
		if (FALSE == psRadio->bRightBottomJustified)
		{
			psRadioItem->u32YOffsetCalculated = psRadioItem->u32YOffset + psRadio->psWidget->s32YPos;
		}
		else
		{
			psRadioItem->u32YOffsetCalculated = psRadioItem->u32YOffset + (psRadio->u32YMaxSize - psRadioItem->u32TextYSize - psImage->u32YSize) + psRadio->psWidget->s32YPos;
		}

		psRadioItem->u32XOffsetCalculated = psRadioItem->u32XOffset + psRadio->psWidget->s32XPos;
	}
	else
	if (ROT_270 == psRadioItem->u16Orientation)
	{
		if (FALSE == psRadio->bRightBottomJustified)
		{
			psRadioItem->u32YOffsetCalculated = psRadioItem->u32YOffset + (psRadio->u32YMaxSize - psRadioItem->u32TextYSize - psImage->u32YSize) + psRadio->psWidget->s32YPos;
		}
		else
		{
			psRadioItem->u32YOffsetCalculated = psRadioItem->u32YOffset + psRadio->psWidget->s32YPos;
		}

		psRadioItem->u32XOffsetCalculated = psRadioItem->u32XOffset + psRadio->psWidget->s32XPos;
	}
	else
	if (ROT_180 == psRadioItem->u16Orientation)
	{
		if (FALSE == psRadio->bRightBottomJustified)
		{
			psRadioItem->u32XOffsetCalculated = psRadioItem->u32XOffset + (psRadio->u32XMaxSize - psRadioItem->u32TextXSize - psImage->u32XSize) + psRadio->psWidget->s32XPos;
		}
		else
		{
			psRadioItem->u32XOffsetCalculated = psRadioItem->u32XOffset + psRadio->psWidget->s32XPos;
		}

		psRadioItem->u32YOffsetCalculated = psRadioItem->u32YOffset + psRadio->psWidget->s32YPos;
	}
	else
	{
		GCASSERT(0);
	}
}

static void RadioButtonCalcMaxSize(SRadioItem *psRadioItem,
								   SRadio *psRadio,
								   UINT32 *pu32XSizeMax,
								   UINT32 *pu32YSizeMax)
{
	SRadioItem *psItemPtr = psRadio->psItemList;
	SImage *psImage;

	*pu32XSizeMax = 0;
	*pu32YSizeMax = 0;

	while (psItemPtr)
	{
		if (NULL == psItemPtr->psNonselectedImage)
		{
			psImage = psRadio->psDefaultNonselected->psCurrentImage;
		}
		else
		{
			psImage = psItemPtr->psNonselectedImage->psCurrentImage;
		}

		RadioButtonCalcSize(psItemPtr,
							psRadio);

		if (psItemPtr->u32XSize > *pu32XSizeMax)
		{
			*pu32XSizeMax = psItemPtr->u32XSize;
		}

		if (psItemPtr->u32YSize > *pu32YSizeMax)
		{
			*pu32YSizeMax = psItemPtr->u32YSize;
		}

		psItemPtr = psItemPtr->psNextLink;
	}

	// Now the individual item
	if (NULL == psRadioItem->psNonselectedImage)
	{
		psImage = psRadio->psDefaultNonselected->psCurrentImage;
	}
	else
	{
		psImage = psRadioItem->psNonselectedImage->psCurrentImage;
	}

	RadioButtonCalcSize(psRadioItem,
						psRadio);

	if (psRadioItem->u32XSize > *pu32XSizeMax)
	{
		*pu32XSizeMax = psRadioItem->u32XSize;
	}

	if (psRadioItem->u32YSize > *pu32YSizeMax)
	{
		*pu32YSizeMax = psRadioItem->u32YSize;
	}
}

static ELCDErr RadioButtonCheckIntersections(SRadio *psRadio,
											 SRadioItem *psRadioItem)
{
	SRadioItem *psRadioPtr = psRadio->psItemList;
	UINT32 u32XNewItemLeft = 0;
	UINT32 u32XNewItemRight = 0;
	UINT32 u32YNewItemTop = 0;
	UINT32 u32YNewItemBottom = 0;

	u32XNewItemLeft = psRadioItem->u32XOffsetCalculated;
	u32XNewItemRight = psRadioItem->u32XOffsetCalculated + psRadioItem->u32XSize;
	u32YNewItemTop = psRadioItem->u32YOffsetCalculated;
	u32YNewItemBottom = psRadioItem->u32YOffsetCalculated + psRadioItem->u32YSize;
	
	while (psRadioPtr)
	{
		UINT32 u32XLeft = 0;
		UINT32 u32XRight = 0;
		UINT32 u32YTop = 0;
		UINT32 u32YBottom = 0;

		u32XLeft = psRadioPtr->u32XOffsetCalculated;
		u32XRight = u32XLeft + psRadioPtr->u32XSize;
		u32YTop = psRadioPtr->u32YOffsetCalculated;
		u32YBottom = u32YTop + psRadioPtr->u32YSize;
		
		if ((u32XNewItemRight < u32XLeft) ||
			(u32XNewItemLeft >= u32XRight))
		{
			// No X intersection
		}
		else
		if ((u32YNewItemBottom < u32YTop) ||
			(u32YNewItemTop >= u32YBottom))
		{
			// No Y Intersection
		}
		else
		{
			return(LERR_RADIO_OVERLAP);
		}

		psRadioPtr = psRadioPtr->psNextLink;
	}

	return(LERR_OK);
}

static void RadioCalcOverallSize(SRadio *psRadio,
								 UINT32 *pu32XSize,
								 UINT32 *pu32YSize)
{
	SRadioItem *psRadioPtr = psRadio->psItemList;
	UINT32 u32XMax;
	UINT32 u32YMax;

	*pu32XSize = 0;
	*pu32YSize = 0;

	while (psRadioPtr)
	{
		u32XMax = (psRadioPtr->u32XOffsetCalculated - (UINT32) psRadio->psWidget->s32XPos) + psRadioPtr->u32XSize;
		u32YMax = (psRadioPtr->u32YOffsetCalculated - (UINT32) psRadio->psWidget->s32YPos) + psRadioPtr->u32YSize;

		if (u32XMax > *pu32XSize)
		{
			*pu32XSize = u32XMax;
		}

		if (u32YMax > *pu32YSize)
		{
			*pu32YSize = u32YMax;
		}

		psRadioPtr = psRadioPtr->psNextLink;
	}
}

#define	RADIO_BUTTON_SIZE		0xa000
#define	RADIO_SELECTED_SIZE		0x1000

ELCDErr RadioGroupAdd(RADIOGROUPHANDLE eRadioGroupHandle,
					  UINT32 u32XOrigin,
					  UINT32 u32YOrigin,
					  LEX_CHAR *peText,
					  UINT32 u32TextColor,
					  LEX_CHAR *peFontFilename,
					  UINT32 u32FontSize,
					  UINT16 u16Orientation,
					  BOOL bSelected,
					  BOOL bDisabled,
					  BOOL bVisible,
					  BOOL bSimple,
					  SImageGroup *psSelected,
					  SImageGroup *psNonselected,
					  BOOL bForceChecked,
					  BOOL bCheckbox)
{
	SRadio *psRadio;
	ELCDErr eErr = LERR_OK;
	SRadioItem *psRadioItem = NULL;
	SRadioItem *psItemPtr = NULL;
	UINT32 u32Length;
	SImage *psSelectedImage = NULL;
	SImage *psNonselectedImage = NULL;
	UINT32 u32Radius = 0;
	UINT32 u32XSize;
	UINT32 u32YSize;
	SWidget *psWidget;

	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eRadioGroupHandle,
									WIDGET_RADIO,
									&psWidget,
									NULL);
	RETURN_ON_FAIL(eErr);

	psRadio = psWidget->uWidgetSpecific.psRadio;
	GCASSERT(psRadio);

	psRadioItem = MemAlloc(sizeof(*psRadioItem));
	if (NULL == psRadioItem)
	{
		eErr = LERR_NO_MEM;
		goto errorExit;
	}

	// If this item is selected, deselect all the others
	if (bForceChecked)
	{
		if (bSelected)
		{
			SRadioItem *psItemPtr = psRadio->psItemList;

			while (psItemPtr)
			{
				psItemPtr->bSelected = FALSE;
				psItemPtr = psItemPtr->psNextLink;
			}
		}
	}

	psRadioItem->bVisible = bVisible;
	psRadioItem->bSelected = bSelected;
	psRadioItem->bDisabled = bDisabled;
	psRadioItem->u32TextColor = u32TextColor;
	psRadioItem->eFontHandle = HANDLE_INVALID;
	psRadioItem->peText = Lexstrdup(peText);
	psRadioItem->u32XOffset = u32XOrigin;
	psRadioItem->u32YOffset = u32YOrigin;
	psRadioItem->u16Orientation = u16Orientation;
	psRadioItem->bCheckbox = bCheckbox;

	if (NULL == psRadioItem->peText)
	{
		eErr = LERR_NO_MEM;
		goto errorExit;
	}

	// Figure out how big the text is going to be
	eErr = FontCreate(peFontFilename,
					  u32FontSize,
					  0,
					  &psRadioItem->eFontHandle);

	if (eErr != LERR_OK)
	{
		goto errorExit;
	}

	eErr = FontGetStringSize(psRadioItem->eFontHandle,
							 psRadioItem->peText,
							 &psRadioItem->u32TextXSize,
							 &psRadioItem->u32TextYSize,
							 (ERotation) u16Orientation,
							 TRUE);

	if (eErr != LERR_OK)
	{
		goto errorExit;
	}

	if ((ROT_90 == u16Orientation) ||
		(ROT_270 == u16Orientation))
	{
		u32Length = psRadioItem->u32TextXSize;
	}
	else
	{
		u32Length = psRadioItem->u32TextYSize;
	}

	if (bSimple)
	{
		// Create a couple of blank images for the selected/not selected graphics
		psSelectedImage = GfxCreateEmptyImage(u32Length, u32Length, 8, 0, FALSE);
		if (NULL == psSelectedImage)
		{
			eErr = LERR_NO_MEM;
			goto errorExit;
		}

		psNonselectedImage = GfxCreateEmptyImage(u32Length, u32Length, 8, 0, FALSE);
		if (NULL == psSelectedImage)
		{
			GCFreeMemory(psNonselectedImage);
			eErr = LERR_NO_MEM;
			goto errorExit;
		}

		psRadioItem->psSelectedImage = GfxImageGroupCreate();
		if (NULL == psRadioItem->psSelectedImage)
		{
			eErr = LERR_NO_MEM;
			goto errorExit;
		}

		psRadioItem->psNonselectedImage = GfxImageGroupCreate();
		if (NULL == psRadioItem->psNonselectedImage)
		{
			eErr = LERR_NO_MEM;
			goto errorExit;
		}

		if (NULL == GfxImageGroupAppend(psRadioItem->psSelectedImage,
										psSelectedImage))
		{
			eErr = LERR_NO_MEM;
			goto errorExit;
		}

		if (NULL == GfxImageGroupAppend(psRadioItem->psNonselectedImage,
										psNonselectedImage))
		{
			eErr = LERR_NO_MEM;
			goto errorExit;
		}

		u32Radius = (u32Length * RADIO_BUTTON_SIZE) >> 17;
		if (u32Radius < 1)
		{
			u32Radius = 1;
		}

		// Render the nonselected image
		eErr = CircleRender((psRadioItem->psNonselectedImage->psCurrentImage->u32XSize >> 1) - u32Radius,
							(psRadioItem->psNonselectedImage->psCurrentImage->u32YSize >> 1) - u32Radius,
							u32Radius,
							0x202020,
							0x808080,
							0xffffff,
							psRadioItem->psNonselectedImage->psCurrentImage,
							1,
							TRUE);

		if (eErr != LERR_OK)
		{
			goto errorExit;
		}

		// Render the outer selected image
		eErr = CircleRender((psRadioItem->psSelectedImage->psCurrentImage->u32XSize >> 1) - u32Radius,
							(psRadioItem->psSelectedImage->psCurrentImage->u32YSize >> 1) - u32Radius,
							u32Radius,
							0x202020,
							0x808080,
							0xffffff,
							psRadioItem->psSelectedImage->psCurrentImage,
							1,
							TRUE);

		if (eErr != LERR_OK)
		{
			goto errorExit;
		}

		// Render the inner selected image (black center)
		eErr = CircleRender((psRadioItem->psSelectedImage->psCurrentImage->u32XSize >> 1) - u32Radius,
							(psRadioItem->psSelectedImage->psCurrentImage->u32YSize >> 1) - u32Radius,
							u32Radius,
							0xffffff,
							0xffffff,
							0x0,
							psRadioItem->psSelectedImage->psCurrentImage,
							5,
							TRUE);

		if (eErr != LERR_OK)
		{
			goto errorExit;
		}
	}
	else
	{
		// Not simple - from images
		psRadioItem->psSelectedImage = psSelected;
		psRadioItem->psNonselectedImage = psNonselected;

		if (NULL == psSelected)
		{
			psSelected = psRadio->psDefaultSelected;
		}
		else
		{
			GfxIncRef(psSelected);
		}

		if (NULL == psNonselected)
		{
			psNonselected = psRadio->psDefaultNonselected;
		}
		else
		{
			GfxIncRef(psNonselected);
		}

		if (NULL == psSelected)
		{
			eErr = LERR_RADIO_SELECTED_IMAGE_MISSING;
			goto errorExit;
		}

		if (NULL == psNonselected)
		{
			eErr = LERR_RADIO_NONSELECTED_IMAGE_MISSING;
			goto errorExit;
		}
	}

	// Calculate the X/Y size of this item
	RadioButtonCalcSize(psRadioItem,
						psRadio);

	// Recalculate the max size now
	RadioButtonCalcMaxSize(psRadioItem,
						   psRadio,
						   &psRadio->u32XMaxSize,
						   &psRadio->u32YMaxSize);

	// Calculate the X/Y size of this item and all others
	RadioButtonCalcPosition(psRadioItem,
							psRadio);

	psItemPtr = psRadio->psItemList;
	while (psItemPtr)
	{
		RadioButtonCalcPosition(psItemPtr,
								psRadio);
		psItemPtr = psItemPtr->psNextLink;
	}

	eErr = RadioButtonCheckIntersections(psRadio,
										 psRadioItem);
	if (eErr != LERR_OK)
	{
		goto errorExit;
	}

	// Add this item to the item list
	psItemPtr = psRadio->psItemList;
	while (psItemPtr && psItemPtr->psNextLink)
	{
		psItemPtr = psItemPtr->psNextLink;
	}

	if (NULL == psItemPtr)
	{
		psRadio->psItemList = psRadioItem;
		psRadio->psItemSelected = psRadioItem;
		if (bForceChecked)
		{
			psRadioItem->bSelected = TRUE;
		}
	}
	else
	{
		psItemPtr->psNextLink = psRadioItem;
	}

	if (psRadioItem->bSelected)
	{
		psRadio->psItemSelected = psRadioItem;
	}

	// Figure out how big the widget is
	RadioCalcOverallSize(psRadio,
						 &u32XSize,
						 &u32YSize);

	// Now go set the widget size
	eErr = WidgetSetSize(psRadio->psWidget,
						 u32XSize,
						 u32YSize,
						 TRUE,
						 TRUE,
						 TRUE);

	if (eErr != LERR_OK)
	{
		goto errorExit;
	}

errorExit:
	if (eErr != LERR_OK)
	{
		if (psRadioItem)
		{
			if (psRadioItem->psSelectedImage)
			{
				GfxDeleteImageGroup(psRadioItem->psSelectedImage);
				psRadioItem->psSelectedImage = NULL;
			}

			if (psRadioItem->psNonselectedImage)
			{
				GfxDeleteImageGroup(psRadioItem->psNonselectedImage);
				psRadioItem->psNonselectedImage = NULL;
			}

			if (psRadioItem->peText)
			{
				GCFreeMemory(psRadioItem->peText);
				psRadioItem->peText = NULL;
			}

			if (psRadioItem->eFontHandle != HANDLE_INVALID)
			{
				(void) FontFree(psRadioItem->eFontHandle);
				psRadioItem->eFontHandle = HANDLE_INVALID;
			}
		}
	}

	return(eErr);
}

ELCDErr RadioGroupSetSelected(RADIOGROUPHANDLE eRadioGroupHandle,
							  UINT32 u32Selected,
							  BOOL bState)
{
	SRadio *psRadio;
	SWindow *psWindow;
	ELCDErr eErr = LERR_OK;
	SRadioItem *psRadioItem = NULL;
	SWidget *psWidget;

	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eRadioGroupHandle,
									WIDGET_RADIO,
									&psWidget,
									NULL);
	RETURN_ON_FAIL(eErr);

	psRadio = psWidget->uWidgetSpecific.psRadio;
	GCASSERT(psRadio);

	psWidget = psRadio->psWidget;
	psRadioItem = psRadio->psItemList;

	psWindow = WindowGetPointer(psWidget->eParentWindow);
	GCASSERT(psWindow);

	while (u32Selected && psRadioItem)
	{
		psRadioItem = psRadioItem->psNextLink;
		u32Selected--;
	}

	if (NULL == psRadioItem)
	{
		return(LERR_RADIO_ITEM_OUT_OF_RANGE);
	}

	// Disabled?
	if (psRadioItem->bDisabled)
	{
		return(LERR_RADIO_ITEM_DISABLED);
	}

	// Already selected? Just return OK.
	if (psRadioItem->bCheckbox)
	{
		if (bState == psRadioItem->bSelected)
		{
			return(LERR_OK);
		}

		psRadioItem->bSelected = bState;
	}
	else
	{
		if (psRadioItem->bSelected)
		{
			return(LERR_OK);
		}
	}

	if (FALSE == psRadio->psWidget->bWidgetHidden)
	{
		// We've got a hit! Time for an update.
		eErr = WindowLockByHandle(psWidget->eParentWindow);
		GCASSERT(GC_OK == eErr);

		WidgetSetUpdate(TRUE);

		// Erase the old and new positions
		if (FALSE == psRadioItem->bCheckbox)
		{
			RadioItemErase(psWidget,
						   psWindow,
						   psRadio->psItemSelected);
		}

		RadioItemErase(psWidget,
					   psWindow,
					   psRadioItem);
	}

	// Mark the old position as not selected
	if (FALSE == psRadioItem->bCheckbox)
	{
		psRadio->psItemSelected->bSelected = FALSE;

		// Mark the new position as selected
		psRadioItem->bSelected = TRUE;
	}

	if (FALSE == psRadio->psWidget->bWidgetHidden)
	{
		if (FALSE == psRadioItem->bCheckbox)
		{
			// Draw the old position
			RadioItemDraw(psWidget,
						  psWindow,
						  psRadio,
						  psRadio->psItemSelected);
		}

		// Now draw the new position
		RadioItemDraw(psWidget,
					  psWindow,
					  psRadio,
					  psRadioItem);
	}

	psRadio->psItemSelected = psRadioItem;

	if (FALSE == psRadio->psWidget->bWidgetHidden)
	{
		WidgetSetUpdate(FALSE);

		eErr = WindowUnlockByHandle(psWidget->eParentWindow);
		GCASSERT(GC_OK == eErr);

		WindowUpdateRegionCommit();
	}

	return(LERR_OK);
}

ELCDErr RadioGroupGetSelected(RADIOGROUPHANDLE eRadioGroupHandle,
							  UINT32 *pu32Selected,
							  BOOL *pbState)
{
	SRadio *psRadio;
	SWindow *psWindow;
	ELCDErr eErr = LERR_OK;
	SRadioItem *psRadioItem = NULL;
	SWidget *psWidget;
	UINT32 u32Loop;

	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eRadioGroupHandle,
									WIDGET_RADIO,
									&psWidget,
									NULL);
	RETURN_ON_FAIL(eErr);

	psRadio = psWidget->uWidgetSpecific.psRadio;
	GCASSERT(psRadio);

	psRadioItem = psRadio->psItemList;

	psWindow = WindowGetPointer(psWidget->eParentWindow);
	GCASSERT(psWindow);

	if (NULL == pbState)
	{
		*pu32Selected = 0;

		while (psRadioItem != psRadio->psItemSelected)
		{
			psRadioItem = psRadioItem->psNextLink;
			(*pu32Selected)++;
		}
	}
	else
	{
		u32Loop = *pu32Selected;

		while (u32Loop-- && psRadioItem)
		{
			psRadioItem = psRadioItem->psNextLink;
		}

		if (NULL == psRadioItem)
		{
			// Out of range
			return(LERR_RADIO_ITEM_OUT_OF_RANGE);
		}

		*pbState = psRadioItem->bSelected;
	}

	return(LERR_OK);
}

ELCDErr RadioGroupSetCheckbox(RADIOGROUPHANDLE eRadioGroupHandle,
							  UINT32 u32Selected,
							  BOOL bCheckbox)
{
	SRadio *psRadio;
	SWindow *psWindow;
	ELCDErr eErr = LERR_OK;
	SRadioItem *psRadioItem = NULL;
	SWidget *psWidget;

	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eRadioGroupHandle,
									WIDGET_RADIO,
									&psWidget,
									NULL);
	RETURN_ON_FAIL(eErr);

	psRadio = psWidget->uWidgetSpecific.psRadio;
	GCASSERT(psRadio);

	psRadioItem = psRadio->psItemList;

	psWindow = WindowGetPointer(psWidget->eParentWindow);
	GCASSERT(psWindow);

	while (u32Selected && psRadioItem)
	{
		psRadioItem = psRadioItem->psNextLink;
		u32Selected--;
	}

	if (NULL == psRadioItem)
	{
		return(LERR_RADIO_ITEM_OUT_OF_RANGE);
	}

	psRadioItem->bCheckbox = bCheckbox;

	return(LERR_OK);
}