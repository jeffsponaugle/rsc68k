#include "Startup/app.h"
#include "Libs/Gfx/GraphicsLib.h"
#include "Libs/window/window.h"
#include "Libs/widget/widget.h"
#include "Libs/widget/slider/slider.h"

static ELCDErr SliderWidgetAlloc(SWidget *psWidget,
								 WIDGETHANDLE eHandle)
{
	GCASSERT(psWidget);
	psWidget->uWidgetSpecific.psSlider = MemAlloc(sizeof(*psWidget->uWidgetSpecific.psSlider));
	if (NULL == psWidget->uWidgetSpecific.psSlider)
	{
		return(LERR_NO_MEM);
	}
	else
	{
		psWidget->uWidgetSpecific.psSlider->eSliderHandle = (SLIDERHANDLE) eHandle;
		psWidget->uWidgetSpecific.psSlider->psWidget = psWidget;
		psWidget->uWidgetSpecific.psSlider->eSoundHandle = HANDLE_INVALID;
		return(LERR_OK);
	}
}

static ELCDErr SliderWidgetFree(SWidget *psWidget)
{
	SSlider *psSlider;

	GCASSERT(psWidget);
	psSlider = psWidget->uWidgetSpecific.psSlider;
	GCASSERT(psSlider);
	
	// If we have an image group for "down", delete it
	if (psSlider->psThumb)
	{
		GfxDeleteImageGroup(psSlider->psThumb);
		psSlider->psThumb = NULL;
	}

	if (psSlider->psTicks)
	{
		GfxDeleteImageGroup(psSlider->psTicks);
		psSlider->psTicks = NULL;
	}

	psSlider->eSliderHandle = HANDLE_INVALID;
	GCFreeMemory(psSlider);
	psWidget->uWidgetSpecific.psSlider = NULL;
	return(LERR_OK);
}

static BOOL SliderHitTest(SWidget *psWidget,
						  UINT32 u32XPos, 
						  UINT32 u32YPos)
{
	// Always accept the slider hit since it's always valid
	return(TRUE);
}

static void SliderPaint(SWidget *psWidget,
						BOOL bLock)
{
	SSlider *psSlider = psWidget->uWidgetSpecific.psSlider;
	SWindow *psWindow = NULL;
	INT32 s32WidgetBaseX = 0;
	INT32 s32WidgetBaseY = 0;

	GCASSERT(psSlider);
	psWindow = WindowGetPointer(psWidget->eParentWindow);
	GCASSERT(psWindow);

	s32WidgetBaseX = (INT32) (psWidget->s32XPos + psWindow->u32ActiveAreaXPos);
	s32WidgetBaseY = (INT32) (psWidget->s32YPos + psWindow->u32ActiveAreaYPos);

	// Draw the ticks first
	if (psSlider->psTicks)
	{
		if (psSlider->bEnabled)
		{
			// Normal blit
			GfxBlitImageToImage(psWindow->psWindowImage,
								psSlider->psTicks->psCurrentImage,
								s32WidgetBaseX,
								s32WidgetBaseY,
								psWindow->u32ActiveAreaXPos + psWindow->u32ActiveAreaXSize,
								psWindow->u32ActiveAreaYPos + psWindow->u32ActiveAreaYSize);
		}
		else
		{
			// Greyscaled
			GfxBlitImageToImageGray(psWindow->psWindowImage,
									psSlider->psTicks->psCurrentImage,
									s32WidgetBaseX,
									s32WidgetBaseY,
									psWindow->u32ActiveAreaXPos + psWindow->u32ActiveAreaXSize,
									psWindow->u32ActiveAreaYPos + psWindow->u32ActiveAreaYSize);
		}

		WindowUpdateRegion(psWidget->eParentWindow,
						   (UINT32) s32WidgetBaseX,
						   (UINT32) s32WidgetBaseY,
						   psSlider->psTicks->psCurrentImage->u32XSize,
						   psSlider->psTicks->psCurrentImage->u32YSize);

	}

	// Draw the thumb
	if (psSlider->psThumb)
	{
		if (psSlider->bEnabled)
		{
			GfxBlitImageToImage(psWindow->psWindowImage,
								psSlider->psThumb->psCurrentImage,
								(UINT32) (psSlider->u32ThumbX + s32WidgetBaseX),
								(UINT32) (psSlider->u32ThumbY + s32WidgetBaseY),
								psWindow->u32ActiveAreaXPos + psWindow->u32ActiveAreaXSize,
								psWindow->u32ActiveAreaYPos + psWindow->u32ActiveAreaYSize);
		}
		else
		{
			// Greyscaled
			GfxBlitImageToImageGray(psWindow->psWindowImage,
									psSlider->psThumb->psCurrentImage,
									(UINT32) (psSlider->u32ThumbX + s32WidgetBaseX),
									(UINT32) (psSlider->u32ThumbY + s32WidgetBaseY),
									psWindow->u32ActiveAreaXPos + psWindow->u32ActiveAreaXSize,
									psWindow->u32ActiveAreaYPos + psWindow->u32ActiveAreaYSize);
		}

		WindowUpdateRegion(psWidget->eParentWindow,
						   (UINT32) (psSlider->u32ThumbX + s32WidgetBaseX),
						   (UINT32) (psSlider->u32ThumbY + s32WidgetBaseY),
						   psSlider->psThumb->psCurrentImage->u32XSize,
						   psSlider->psThumb->psCurrentImage->u32YSize);
	}
}

static void SliderErase(SWidget *psWidget)
{
	WidgetEraseStandard(psWidget);
}

static ELCDErr SliderSetValueInternal(SSlider *psSlider,
									  INT32 s32Value)
{
	ELCDErr eLCDErr = LERR_OK;
	INT32 s32Low;
	INT32 s32High;
	INT64 s64Result;
	BOOL bReversed = FALSE;
	UINT32 u32ThumbX = psSlider->u32ThumbX;
	UINT32 u32ThumbY = psSlider->u32ThumbY;
	INT32 s32LowValue;
	INT32 s32HighValue;

	if ((ROT_180 == psSlider->u16Orientation) ||
		(ROT_270 == psSlider->u16Orientation))
	{
		s32LowValue = psSlider->s32HighValue;
		s32HighValue = psSlider->s32LowValue;
	}
	else
	{
		s32HighValue = psSlider->s32HighValue;
		s32LowValue = psSlider->s32LowValue;
	}

	if (s32LowValue > s32HighValue)
	{
		s32Low = s32HighValue;
		s32High = s32LowValue;
		bReversed = TRUE;
	}
	else
	{
		s32High = s32HighValue;
		s32Low = s32LowValue;
	}

	if (s32Value < s32Low)
	{
		s32Value = s32Low;
	}

	if (s32Value > s32High)
	{
		s32Value = s32High;
	}

	if ((s32High - s32Low) == 0)
	{
		s64Result = 0;
	}
	else
	{
		s64Result = (((INT64) (s32Value - s32Low) << 16) / ((((INT64)  s32High - (INT64) s32Low))));
	}

	if (s64Result < 0)
	{
		s64Result = -s64Result;
	}

	if (bReversed)
	{
		s64Result = (1 << 16) - s64Result;
	}

	// Compute the new thumb position
	if ((ROT_0 == psSlider->u16Orientation) ||
		(ROT_180 == psSlider->u16Orientation))
	{
		// Vertical axis
		u32ThumbY = (UINT32) ((((INT64) psSlider->u32ThumbTrack) * s64Result) >> 16);
		if (u32ThumbY == psSlider->u32ThumbY)
		{
			eLCDErr = LERR_OK;
			goto errorExit;
		}
	}
	else
	{
		// Horizontal axis
		u32ThumbX = (UINT32) ((((INT64) psSlider->u32ThumbTrack) * s64Result) >> 16);
		if (u32ThumbX == psSlider->u32ThumbX)
		{
			eLCDErr = LERR_OK;
			goto errorExit;
		}
	}

	eLCDErr = WindowLockByHandle(psSlider->eWindowHandle);
	if (eLCDErr != LERR_OK)
	{
		return(eLCDErr);
	}

	WidgetSetUpdate(TRUE);

	// Gotta update. Erase the widget
	WidgetErase(psSlider->psWidget);

	WidgetPaintIntersections(psSlider->psWidget);

	psSlider->u32ThumbX = u32ThumbX;
	psSlider->u32ThumbY = u32ThumbY;

	// Update the slider
	WidgetPaint(psSlider->psWidget,
				FALSE);

	eLCDErr = WindowUnlockByHandle(psSlider->eWindowHandle);

	WidgetSetUpdate(FALSE);

	WindowUpdateRegionCommit();

errorExit:
	if (psSlider->s32CurrentSetting != s32Value)
	{
		UWidgetCallbackData uData;

		memset((void *) &uData, 0, sizeof(uData));
		uData.sSlider.s32SliderValue = s32Value;

		WidgetBroadcastMask((WIDGETHANDLE) psSlider->psWidget->eWidgetHandle,
							WCBK_SPECIFIC,
							&uData);
	}

	psSlider->s32CurrentSetting = s32Value;
	return(eLCDErr);
}

static INT32 SliderGetValueFromThumb(SSlider *psSlider)
{
	INT32 s32Low;
	INT32 s32High;
	UINT32 u32StepFraction;
	INT64 s64Result;
	UINT32 u32Distance;
	UINT32 u32YVal;
	BOOL bReversed = FALSE;
	UINT32 u32Result;
	INT32 s32LowValue;
	INT32 s32HighValue;

	if ((ROT_180 == psSlider->u16Orientation) ||
		(ROT_270 == psSlider->u16Orientation))
	{
		s32LowValue = psSlider->s32HighValue;
		s32HighValue = psSlider->s32LowValue;
	}
	else
	{
		s32LowValue = psSlider->s32LowValue;
		s32HighValue = psSlider->s32HighValue;
	}

	if (s32LowValue > s32HighValue)
	{
		s32Low = s32HighValue;
		s32High = s32LowValue;
		bReversed = TRUE;
	}
	else
	{
		s32High = s32HighValue;
		s32Low = s32LowValue;
	}

	u32Distance = (s32High - s32Low);

	if ((ROT_0 == psSlider->u16Orientation) ||
		(ROT_180 == psSlider->u16Orientation))
	{
		u32YVal = psSlider->u32ThumbY;
	}
	else
	{
		u32YVal = psSlider->u32ThumbX;
	}

	if (u32YVal >= psSlider->u32ThumbTrack)
	{
		u32YVal = psSlider->u32ThumbTrack;
	}

	if (u32Distance)
	{
		u32StepFraction = (0x10000 / (u32Distance));
	}
	else
	{
		u32StepFraction = 0;
	}

	u32Result = ((u32YVal << 16) / (psSlider->u32ThumbTrack));
	if (u32Result > 0x10000)
	{
		u32Result = 0x10000;
	}

	if (bReversed)
	{
		u32Result = (0x10000 + (u32StepFraction >> 1) - u32Result);
	}
	else
	{
		u32Result += (u32StepFraction >> 1);
	}

	// u32Result Now has 0-0x10000. Let's multiply it by our range
	s64Result = (((INT64) u32Distance * (INT64) u32Result) >> 16) + s32Low;

	return((INT32) s64Result);
}

static void SliderPositionUpdate(SWidget *psWidget,
								 UINT32 u32XPos,
								 UINT32 u32YPos,
								 BOOL bSkipCompare)
{
	INT32 s32NormalizedXPos = 0;
	INT32 s32NormalizedYPos = 0;
	SSlider *psSlider = psWidget->uWidgetSpecific.psSlider;
	INT32 s32ThumbX = 0;
	INT32 s32ThumbY = 0;
	INT32 s32Value = 0;
	ELCDErr eErr = LERR_OK;
	SWindow *psWindow = NULL;

	GCASSERT(psSlider);

	s32NormalizedXPos = (INT32) u32XPos - psWidget->s32XPos;
	s32NormalizedYPos = (INT32) u32YPos - psWidget->s32YPos;

	// If there's no thumb, don't bother
	if (NULL == psSlider->psThumb)
	{
		return;
	}

	// If the slider is disabled, don't do anything
	if (FALSE == psSlider->bEnabled)
	{
		return;
	}

	if ((ROT_0 == psSlider->u16Orientation) ||
		(ROT_180 == psSlider->u16Orientation))
	{
		// It's vertical. Ignore the X, set the Y.

		// Horizontal centering
		if (psSlider->psTicks)
		{
			s32ThumbX = (psSlider->psTicks->psCurrentImage->u32XSize >> 1) - (psSlider->psThumb->psCurrentImage->u32XSize >> 1);
		}
		else
		{
			// No track image. X Offset is 0 (thumb)
			s32ThumbX = 0;
		}

		// Vertical centering
		s32ThumbY = s32NormalizedYPos - (psSlider->psThumb->psCurrentImage->u32YSize >> 1);
		if (s32ThumbY < 0)
		{
			s32ThumbY = 0;
		}

		if (s32ThumbY >= (INT32) ((INT32) psSlider->u32ThumbTrack))
		{
			s32ThumbY = (INT32) ((INT32) psSlider->u32ThumbTrack);
		}
	}
	else
	{
		// Vertical centering
		s32ThumbY = (psSlider->psTicks->psCurrentImage->u32YSize >> 1) - (psSlider->psThumb->psCurrentImage->u32YSize >> 1);

		// Horizontal centering
		s32ThumbX = s32NormalizedXPos - (psSlider->psThumb->psCurrentImage->u32XSize >> 1);
		if (s32ThumbX < 0)
		{
			s32ThumbX = 0;
		}

		if (s32ThumbX >= (INT32) ((INT32) psSlider->u32ThumbTrack))
		{
			s32ThumbX = (INT32) ((INT32) psSlider->u32ThumbTrack);
		}
	}

	// If the thumb positions don't match, update the 
	if (FALSE == bSkipCompare)
	{
		if ((s32ThumbX == (INT32) u32XPos) &&
			(s32ThumbY == (INT32) u32YPos))
		{
			// No update needed
			return;
		}
	}

	psWindow = WindowGetPointer(psWidget->eParentWindow);
	GCASSERT(psWindow);

	// Lock the window
	eErr = WindowLock(psWindow);
	GCASSERT(GC_OK == eErr);
	
	WidgetSetUpdate(TRUE);
	
	// Gotta update. Erase the widget
	WidgetErase(psWidget);

	// Update the position 
	psSlider->u32ThumbX = (UINT32) s32ThumbX;
	psSlider->u32ThumbY = (UINT32) s32ThumbY;

	WidgetPaintIntersections(psWidget);

	// Update the slider
	WidgetPaint(psWidget,
				FALSE);

	WidgetSetUpdate(FALSE);

	eErr = WindowUnlock(psWindow);
	GCASSERT(GC_OK == eErr);

	// See if the value has changed
	s32Value = SliderGetValueFromThumb(psSlider);

	if (s32Value != psSlider->s32CurrentSetting)
	{
		UWidgetCallbackData uData;

		psSlider->s32CurrentSetting = s32Value;

		memset((void *) &uData, 0, sizeof(uData));
		uData.sSlider.s32SliderValue = s32Value;

		WidgetBroadcastMask((WIDGETHANDLE) psSlider->psWidget->eWidgetHandle,
							WCBK_SPECIFIC,
							&uData);
	}

	WindowUpdateRegionCommit();
}

static void SliderPress(SWidget *psWidget,
						UINT32 u32Mask,
						UINT32 u32XPos,
						UINT32 u32YPos)
{
	SSlider *psSlider = psWidget->uWidgetSpecific.psSlider;
	BOOL bSkipCompare = FALSE;

//	DebugOut("%s: u32Mask=0x%.2x, X=%u, Y=%u\n", __FUNCTION__, u32Mask, u32XPos, u32YPos);

	if (psSlider->bLock)
	{
		// If it's locked, don't allow any changes
		return;
	}

	GCASSERT(psSlider);
	if (FALSE == psSlider->bPressed)
	{
		psSlider->bPressed = TRUE;
		bSkipCompare = TRUE;
	}

	WidgetSoundPlay(psSlider->eSoundHandle);

	SliderPositionUpdate(psWidget,
						 u32XPos,
						 u32YPos,
						 bSkipCompare);
}

static void SliderRelease(SWidget *psWidget,
						  UINT32 u32Mask,
						  UINT32 u32XPos,
						  UINT32 u32YPos)
{
	SSlider *psSlider = psWidget->uWidgetSpecific.psSlider;
	ELCDErr eErr;
	INT32 s32Value;

//	DebugOut("%s: u32Mask=0x%.2x\n", __FUNCTION__, u32Mask);
	GCASSERT(psSlider);
	if (FALSE == psSlider->bPressed)
	{
		return;
	}

	psSlider->bPressed = FALSE;

	s32Value = SliderGetValueFromThumb(psSlider);
	eErr = SliderSetValueInternal(psSlider,
								  s32Value);
	GCASSERT(LERR_OK == eErr);
}

static void SliderMouseover(SWidget *psWidget,
							UINT32 u32Mask,
							UINT32 u32XPos,
							UINT32 u32YPos,
							EMouseOverState eMouseoverState)
{
	if (u32Mask)
	{
		SliderPositionUpdate(psWidget,
							 u32XPos,
							 u32YPos,
							 FALSE);
	}
}

static SWidgetFunctions sg_sSliderFunctions =
{
	SliderHitTest,
	SliderPaint,
	SliderErase,
	SliderPress,
	SliderRelease,
	SliderMouseover,			// Mouseover our widget
	NULL,						// Focus
	NULL,						// Keystroke for us
	NULL,						// Animation tick - none for now!
	NULL,
	NULL,						// Mouse wheel
	NULL						// Set disable
};

static SWidgetTypeMethods sg_sSliderMethods = 
{
	&sg_sSliderFunctions,
	LERR_SLIDER_BAD_HANDLE,			// Error when it's not the handle we're looking for
	SliderWidgetAlloc,
	SliderWidgetFree
};

void SliderFirstTimeInit(void)
{
	DebugOut("* Initializing Sliders\n");

	// Register it with the widget manager
	WidgetRegisterTypeMethods(WIDGET_SLIDER,
							  &sg_sSliderMethods);
}

ELCDErr SliderCreate(WINDOWHANDLE eWindowHandle,
					 SLIDERHANDLE *peSliderHandle,
					 INT32 s32XPos,
					 INT32 s32YPos,
					 UINT16 u16Orientation)
{
	ELCDErr eLCDErr;
	SSlider *psSlider = NULL;
	SWidget *psWidget = NULL;

	eLCDErr = WidgetAllocateHandle((WIDGETHANDLE *) peSliderHandle,
								   WIDGET_SLIDER,
								   eWindowHandle,
								   &psWidget);

	RETURN_ON_FAIL(eLCDErr);

	psWidget->uWidgetSpecific.psSlider->u16Orientation = u16Orientation;

	psWidget->s32XPos = s32XPos;
	psWidget->s32YPos = s32YPos;
	psWidget->bWidgetHidden = TRUE;

	// Make sure the widget is enabled
	psWidget->bWidgetEnabled = TRUE;

	return(LERR_OK);
}

ELCDErr SliderSetImages(SLIDERHANDLE eSliderHandle,
						SImageGroup *psTicks,
						SImageGroup *psThumb,
						UINT32 u32TrackLength,
						BOOL bLockWindow)
{
	ELCDErr eLCDErr = LERR_OK;
	SSlider *psSlider = NULL;
	SWidget *psWidget = NULL;
	UINT32 u32XSize = 0;
	UINT32 u32YSize = 0;

	eLCDErr = WidgetGetPointerByHandle((WIDGETHANDLE) eSliderHandle,
										WIDGET_SLIDER,
										&psWidget,
										NULL);

	RETURN_ON_FAIL(eLCDErr);

	psSlider = psWidget->uWidgetSpecific.psSlider;
	GCASSERT(psSlider);

	// 0 Indicates "don't touch" - inherit the existing thumb track size
	if (0 == u32TrackLength)
	{
		u32TrackLength = psSlider->u32ThumbTrack;
	}

	// Figure out how big our images are
	if (psTicks)
	{
		if (psTicks->psCurrentImage->u32XSize > u32XSize)
		{
			u32XSize = psTicks->psCurrentImage->u32XSize;
		}

		if (psTicks->psCurrentImage->u32YSize > u32YSize)
		{
			u32YSize = psTicks->psCurrentImage->u32YSize;
		}
	}
	else
	{
		u32YSize = u32TrackLength;

		if (psThumb)
		{
			u32YSize += psThumb->psCurrentImage->u32YSize;
		}
	}

	if (psThumb)
	{
		UINT32 u32ThumbXSize = psThumb->psCurrentImage->u32XSize;
		UINT32 u32ThumbYSize = psThumb->psCurrentImage->u32YSize;

		if ((ROT_0 == psSlider->u16Orientation) ||
			(ROT_180 == psSlider->u16Orientation))
		{
			u32ThumbYSize += psSlider->u32ThumbTrack;
		}
		else
		{
			u32ThumbXSize += psSlider->u32ThumbTrack;
		}

		if (u32ThumbXSize > u32XSize)
		{
			u32XSize = u32ThumbXSize;
		}

		if (u32ThumbYSize > u32YSize)
		{
			u32YSize = u32ThumbYSize;
		}
	}

	// Lock the handle
	if (bLockWindow)
	{
		eLCDErr = WindowLockByHandle(psSlider->psWidget->eParentWindow);
		if (eLCDErr != LERR_OK)
		{
			// Can't touch it! 
			return(eLCDErr);
		}
	}

	if ((psThumb) && (NULL == psSlider->psThumb))
	{
		// Thumb being set the first time.
		if ((ROT_0 == psSlider->u16Orientation) ||
			(ROT_180 == psSlider->u16Orientation))
		{
			if (psTicks)
			{
				psSlider->u32ThumbX = (psTicks->psCurrentImage->u32XSize >> 1) - (psThumb->psCurrentImage->u32XSize >> 1);
				psSlider->u32ThumbY = 0;
			}
			else
			{
				// No ticks. Just set it to the thumb
				psSlider->u32ThumbX = 0;
				psSlider->u32ThumbY = 0;
			}
		}
		else
		{
			if (psTicks)
			{
				psSlider->u32ThumbX = 0;
				psSlider->u32ThumbY = (psTicks->psCurrentImage->u32YSize >> 1) - (psThumb->psCurrentImage->u32YSize >> 1);
			}
			else
			{
				// No ticks. Just set it to the thumb
				psSlider->u32ThumbX = 0;
				psSlider->u32ThumbY = 0;
			}
		}
	}

	if (psSlider->psThumb)
	{
		GfxDeleteImageGroup(psSlider->psThumb);
		psSlider->psThumb = NULL;
	}

	if (psSlider->psTicks)
	{
		GfxDeleteImageGroup(psSlider->psTicks);
		psSlider->psTicks = NULL;
	}

	// Assign the new size(s) and images
	psSlider->psThumb = psThumb;
	if (psThumb)
	{
		GfxIncRef(psThumb);
	}

	psSlider->psTicks = psTicks;
	if (psTicks)
	{
		GfxIncRef(psTicks);
	}

	psSlider->u32ThumbTrack = u32TrackLength;

	// Now go set the widget size
	eLCDErr = WidgetSetSize(psSlider->psWidget,
							u32XSize,
							u32YSize,
							FALSE,
							FALSE,
							TRUE);

	if (eLCDErr != LERR_OK)
	{
		goto errorExit;
	}

errorExit:
	if (bLockWindow)
	{
		if (LERR_OK == eLCDErr)
		{
			eLCDErr = WindowUnlockByHandle(psSlider->psWidget->eParentWindow);
		}
		else
		{
			(void) WindowUnlockByHandle(psSlider->psWidget->eParentWindow);
		}
	}

	return(eLCDErr);
}

ELCDErr SliderSetEnable(SLIDERHANDLE eSliderHandle,
						BOOL bEnable)
{
	ELCDErr eLCDErr;
	SWidget *psWidget;
	SSlider *psSlider;

	eLCDErr = WidgetGetPointerByHandle((WIDGETHANDLE) eSliderHandle,
										WIDGET_SLIDER,
										&psWidget,
										NULL);

	RETURN_ON_FAIL(eLCDErr);

	psSlider = psWidget->uWidgetSpecific.psSlider;
	GCASSERT(psSlider);

	if (psSlider->bEnabled == bEnable)
	{
		// Nothing to do
		return(LERR_OK);
	}

	if (psSlider->bEnabled)
	{
		WidgetErase(psSlider->psWidget);
	}

	psSlider->bEnabled = bEnable;
	WidgetPaint(psSlider->psWidget,
				FALSE);
	WindowUpdateRegionCommit();
	return(LERR_OK);
}

#define	SLIDER_THUMB_WIDTH_PERCENT			((21 << 16) / 31)
#define SLIDER_THUMB_HEIGHT_PERCENT			((11 << 16) / 75)
#define SLIDER_TRACK_WIDTH_PERCENT			((4 << 16) / 31)
#define SLIDER_TRACK_HEIGHT_PERCENT			((4 << 16) / 75)

ELCDErr SliderRenderSimple(UINT32 u32SliderXSize,
						   UINT32 u32SliderYSize,
						   SImageGroup **ppsThumb,
						   SImageGroup **ppsSlider,
						   UINT32 u32ThumbColor,
						   UINT32 u32TrackColor,
						   UINT32 u32ThumbThickness,
						   UINT16 u16Orientation,
						   UINT32 u32TickCount)
{
	ELCDErr eErr = LERR_OK;
	UINT32 u32XSize;
	UINT32 u32YSize;
	UINT32 u32Loop;
	UINT32 u32Loop2;
	SImage *psImage = NULL;
	SImage *psTrack = NULL;
	UINT16 *pu16Src = NULL;
	UINT8 *pu8Dest = NULL;

	if ((u16Orientation != ROT_0) &&
		(u16Orientation != ROT_90) &&
		(u16Orientation != ROT_180) && 
		(u16Orientation != ROT_270))
	{
		return(LERR_SLIDER_BAD_ROTATION);
	}

	GCASSERT(ppsSlider);
	GCASSERT(ppsThumb);
	*ppsSlider = NULL;
	*ppsThumb = NULL;

	// Render the thumb
	if ((ROT_0 == u16Orientation) ||
		(ROT_180 == u16Orientation))
	{
		u32XSize = (u32SliderXSize * SLIDER_THUMB_WIDTH_PERCENT) >> 16;
		u32YSize = (u32SliderYSize * SLIDER_THUMB_HEIGHT_PERCENT) >> 16;

		if (u32ThumbThickness)
		{
			u32YSize = u32ThumbThickness;
		}
	}
	else
	{
		// Horizontal
		u32XSize = (u32SliderXSize * SLIDER_THUMB_HEIGHT_PERCENT) >> 16;
		u32YSize = (u32SliderYSize * SLIDER_THUMB_WIDTH_PERCENT) >> 16;

		if (u32ThumbThickness)
		{
			u32XSize = u32ThumbThickness;
		}
	}

	if (u32XSize < 3)
	{
		u32XSize = 3;
	}

	if (u32YSize < 3)
	{
		u32YSize = 3;
	}

	// Go render the thumb
	eErr = BoxRender(u32XSize,
					 u32YSize,
					 0, 0,
					 u32ThumbColor,
					 &psImage,
					 FALSE);
	if (eErr != LERR_OK)
	{
		goto errorExit;
	}

	if ((ROT_180 == u16Orientation) ||
		(ROT_270 == u16Orientation))
	{
		// Regardless of the perspective, rotate it 180 degrees from how it's drawn
		eErr = GfxRotateImage(&psImage,
							  ROT_180);

		if (eErr != LERR_OK)
		{
			goto errorExit;
		}
	}

	// Make a group and assign the thumb to it
	*ppsThumb = GfxImageGroupCreate();
	if (NULL == *ppsThumb)
	{
		eErr = LERR_NO_MEM;
		goto errorExit;
	}

	// Now append this image to the group
	if (NULL == GfxImageGroupAppend(*ppsThumb,
									psImage))
	{
		eErr = LERR_NO_MEM;
		goto errorExit;
	}

	// Make an image group out of it
	psImage = NULL;

	// Now, render the track
	if ((ROT_0 == u16Orientation) ||
		(ROT_180 == u16Orientation))
	{
		u32XSize = (u32SliderXSize * SLIDER_TRACK_WIDTH_PERCENT) >> 16;
		u32YSize = u32SliderYSize;
	}
	else
	{
		// Horizontal
		u32XSize = u32SliderXSize;
		u32YSize = (u32SliderYSize * SLIDER_TRACK_WIDTH_PERCENT) >> 16;
	}

	// Go render the track
	if (u32XSize < 3)
	{
		u32XSize = 3;
	}

	if (u32YSize < 3)
	{
		u32YSize = 3;
	}

	eErr = BoxRender(u32XSize,
					 u32YSize,
					 0, 0,
					 u32TrackColor,
					 &psImage,
					 TRUE);
	if (eErr != LERR_OK)
	{
		goto errorExit;
	}

	// Now create an 8bpp image for the final tick mark/tray

	psTrack = GfxCreateEmptyImage(u32SliderXSize,
								  u32SliderYSize,
								  8,
								  0,
								  FALSE);

	if (NULL == psTrack)
	{
		eErr = LERR_NO_MEM;
		goto errorExit;
	}

	// Let's copy the track from the 16bpp image to an 8bpp image including
	// palette translation

	// 0 - Transparent
	// 1 - BORDER_OUTER
	// 2 - BORDER_INNER
	// 3 - BORDER_BRIGHTEDGE
	// 4 - Track bar color

	pu16Src = psImage->pu16ImageData;
	pu8Dest = psTrack->pu8ImageData;

	// Figure out where the slider starts
	if ((ROT_0 == u16Orientation) ||
		(ROT_180 == u16Orientation))
	{
		pu8Dest += ((u32SliderXSize >> 1) - (psImage->u32XSize >> 1));
	}
	else
	{
		pu8Dest += (((u32SliderYSize >> 1) - (psImage->u32YSize >> 1)) * psImage->u32Pitch);
	}

	// Copy it to an 8bpp image
	for (u32Loop = 0; u32Loop < psImage->u32YSize; u32Loop++)
	{
		for (u32Loop2 = 0; u32Loop2 < psImage->u32XSize; u32Loop2++)
		{
			if (BORDER_OUTER == *pu16Src)
			{
				*pu8Dest = 1;
			}
			else
			if (BORDER_INNER == *pu16Src)
			{
				*pu8Dest = 2;
			}
			else
			if (BORDER_BRIGHTEDGE == *pu16Src)
			{
				*pu8Dest = 3;
			}
			else
			{
				*pu8Dest = 4;
			}

			++pu8Dest;
			++pu16Src;
		}

		// Adjust the pointers
		pu8Dest += (psTrack->u32Pitch - psImage->u32XSize);
		pu16Src += (psImage->u32Pitch - psImage->u32XSize);
	}

	// Now set the palette up for this image
	psTrack->pu16Palette = MemAlloc(5 * sizeof(*psTrack->pu16Palette));
	if (NULL == psTrack->pu16Palette)
	{
		eErr = LERR_NO_MEM;
		goto errorExit;
	}

	// Create a palette for this image
	// Leave 0 alone - it's black and it's transparent already
	psTrack->pu16Palette[1] = BORDER_OUTER;
	psTrack->pu16Palette[2] = BORDER_INNER;
	psTrack->pu16Palette[3] = BORDER_BRIGHTEDGE;
	psTrack->pu16Palette[4] = CONVERT_24RGB_16RGB(u32TrackColor);

// This is how far in the tick height is for normal ticks
#define SLIDER_TICK_HEIGHT_INLET_PERCENT		((15 << 16) / 300)

// This is how far in the tick height is for endpoint ticks
#define SLIDER_END_TICK_HEIGHT_INLET_PERCENT	((15 << 16) / 300)

// This is how far in the tick ends (closest to the track)
#define SLIDER_TICK_WIDTH_INLET_PERCENT			((20 << 16) / 100)

	// Only include ticks if the count is non-0
	if (u32TickCount)
	{
		UINT32 u32EndTickSize;
		UINT32 u32MiddleTickSize;
		UINT32 u32EndTickOffset;
		UINT32 u32MiddleTickOffset;

		pu8Dest = psTrack->pu8ImageData;

		if ((ROT_0 == u16Orientation) ||
			(ROT_180 == u16Orientation))
		{
			UINT32 u32YTickSize = 0;
			UINT32 u32StepValue = 0;
			UINT32 u32Counter = 0;
			BOOL bFirst = TRUE;

			// Set up the tick sizes and offsets
			u32EndTickSize = ((u32SliderXSize * SLIDER_TICK_WIDTH_INLET_PERCENT) >> 16) -
							 ((u32SliderXSize * SLIDER_END_TICK_HEIGHT_INLET_PERCENT) >> 16);
			u32EndTickOffset = (u32SliderXSize * SLIDER_END_TICK_HEIGHT_INLET_PERCENT) >> 16;

			u32MiddleTickSize = ((u32SliderXSize * SLIDER_TICK_WIDTH_INLET_PERCENT) >> 16) -
								((u32SliderXSize * SLIDER_TICK_HEIGHT_INLET_PERCENT) >> 16);
			u32MiddleTickOffset = (u32SliderXSize * SLIDER_TICK_HEIGHT_INLET_PERCENT) >> 16;

			// Figure out how far in from the edges we should start, in pixels.
			// Multiply by 2 
			u32YTickSize = (u32SliderYSize * (SLIDER_TICK_HEIGHT_INLET_PERCENT << 1)) >> 16;
			u32StepValue = ((u32SliderYSize - u32YTickSize) << 16) / (u32TickCount - 1);	

			// Adjust the pointer down to the Y coordinate
			pu8Dest += ((u32YTickSize >> 1) * psTrack->u32Pitch);

			// Loop through the Y positions and figure out where to draw the bars
			while (u32TickCount)
			{
				UINT8 *pu8DestTemp;
				UINT32 u32XOffset = 0;
				UINT32 u32XFill = 0;

				// Y Coordinate achieved
				pu8DestTemp = pu8Dest + ((u32Counter >> 16) * psTrack->u32Pitch);

				// Now figure out how far in we need to make this based on the tick count
				if ((1 == u32TickCount) ||
					(bFirst))
				{
					// First or last
					bFirst = FALSE;
					u32XFill = u32EndTickSize;
					u32XOffset = u32EndTickOffset;
				}
				else
				{
					// Not first or last
					u32XFill = u32MiddleTickSize;
					u32XOffset = u32MiddleTickOffset;
				}

				// Fill the left side tick
				memset((void *) (pu8DestTemp + u32XOffset), 4, u32XFill);

				// Now the right side tick
				memset((void *) (pu8DestTemp + (u32SliderXSize - u32XOffset - u32XFill)), 4, u32XFill);

				u32Counter += u32StepValue;
				u32TickCount--;
			}
		}
		else
		{
			// Horizontal draw
			UINT32 u32XTickSize = 0;
			UINT32 u32StepValue = 0;
			UINT32 u32Counter = 0;
			BOOL bFirst = TRUE;

			// Set up the tick sizes and offsets
			u32EndTickSize = ((u32SliderYSize * SLIDER_TICK_WIDTH_INLET_PERCENT) >> 16) -
							 ((u32SliderYSize * SLIDER_END_TICK_HEIGHT_INLET_PERCENT) >> 16);
			u32EndTickOffset = (u32SliderYSize * SLIDER_END_TICK_HEIGHT_INLET_PERCENT) >> 16;

			u32MiddleTickSize = ((u32SliderYSize * SLIDER_TICK_WIDTH_INLET_PERCENT) >> 16) -
								((u32SliderYSize * SLIDER_TICK_HEIGHT_INLET_PERCENT) >> 16);
			u32MiddleTickOffset = (u32SliderYSize * SLIDER_TICK_HEIGHT_INLET_PERCENT) >> 16;

			// Figure out how far in from the edges we should start, in pixels.
			// Multiply by 2 
			u32XTickSize = (u32SliderXSize * (SLIDER_TICK_HEIGHT_INLET_PERCENT << 1)) >> 16;
			u32StepValue = ((u32SliderXSize - u32XTickSize) << 16) / (u32TickCount - 1);	

			// Adjust the pointer down to the Y coordinate
			pu8Dest += (u32EndTickOffset * psTrack->u32Pitch) + (u32XTickSize >> 1);

			// Loop through the X positions and figure out where to draw the bars
			while (u32TickCount)
			{
				UINT8 *pu8DestTemp;
				UINT32 u32YOffset = 0;
				UINT32 u32YFill = 0;
				UINT32 u32YFillTemp = 0;

				// Y Coordinate achieved
				pu8DestTemp = pu8Dest + (u32Counter >> 16);

				// Now figure out how far in we need to make this based on the tick count
				if ((1 == u32TickCount) ||
					(bFirst))
				{
					// First or last
					bFirst = FALSE;
					u32YFill = u32EndTickSize;
					u32YOffset = u32EndTickOffset;
				}
				else
				{
					// Not first or last
					u32YFill = u32MiddleTickSize;
					u32YOffset = u32MiddleTickOffset;
				}

				// Fill the top side track

				pu8DestTemp += (psTrack->u32Pitch * (u32YFill - 1));
				u32YFillTemp = u32YFill;
				while (u32YFillTemp)
				{
					*pu8DestTemp = 4;
					pu8DestTemp -= psTrack->u32Pitch;
					u32YFillTemp--;
				}

				// Fill the bottom side tick
				pu8DestTemp = pu8Dest + (u32Counter >> 16) + ((u32SliderYSize - u32YOffset - u32YFill - 1) * psTrack->u32Pitch);

				while (u32YFill)
				{
					*pu8DestTemp = 4;
					pu8DestTemp += psTrack->u32Pitch;
					u32YFill--;
				}

				u32Counter += u32StepValue;
				u32TickCount--;
			}
		}
	}

	// Draw a box around the track image
	if (0)
	{
		pu8Dest = psTrack->pu8ImageData;

		memset((void *) pu8Dest, 4, psTrack->u32XSize);

		for (u32Loop = 0; u32Loop < (psTrack->u32YSize - 1); u32Loop++)
		{
			*(pu8Dest) = 4;
			*(pu8Dest + psTrack->u32XSize - 1) = 4;
			pu8Dest += psTrack->u32Pitch;
		}

		memset((void *) pu8Dest, 4, psTrack->u32XSize);
	}

	if ((ROT_180 == u16Orientation) ||
		(ROT_270 == u16Orientation))
	{
		// Regardless of the perspective, rotate it 180 degrees from how it's drawn
		eErr = GfxRotateImage(&psTrack,
							  ROT_180);

		if (eErr != LERR_OK)
		{
			goto errorExit;
		}
	}

	*ppsSlider = GfxImageGroupCreate();
	if (NULL == *ppsSlider)
	{
		eErr = LERR_NO_MEM;
		goto errorExit;
	}

	if (NULL == GfxImageGroupAppend(*ppsSlider,
									psTrack))
	{
		eErr = LERR_NO_MEM;
		goto errorExit;
	}

	// Delete the rendered track image - not needed
	GfxDeleteImage(psImage);
	psImage = NULL;

errorExit:
	if (eErr != LERR_OK)
	{
		// Bail out!
		if (*ppsThumb)
		{
			GfxDeleteImageGroup(*ppsThumb);
			*ppsThumb = NULL;
		}

		if (*ppsSlider)
		{
			GfxDeleteImageGroup(*ppsSlider);
			*ppsSlider = NULL;
		}

		if (psImage)
		{
			GfxDeleteImage(psImage);
		}

		if (psTrack)
		{
			GfxDeleteImage(psTrack);
		}
	}

	return(eErr);
}

ELCDErr SliderSetMinMax(SLIDERHANDLE eSliderHandle,
						INT32 s32Min,
						INT32 s32Max)
{
	ELCDErr eLCDErr;
	SWidget *psWidget;
	SSlider *psSlider;
	INT32 s32Low;
	INT32 s32High;

	eLCDErr = WidgetGetPointerByHandle((WIDGETHANDLE) eSliderHandle,
										WIDGET_SLIDER,
										&psWidget,
										NULL);

	RETURN_ON_FAIL(eLCDErr);

	psSlider = psWidget->uWidgetSpecific.psSlider;
	GCASSERT(psSlider);

	if (s32Min > s32Max)
	{
		s32Low = s32Max;
		s32High = s32Min;
	}
	else
	{
		s32Low = s32Min;
		s32High = s32Max;
	}

	if (psSlider->s32CurrentSetting > s32High)
	{
		psSlider->s32CurrentSetting = s32High;
	}

	if (psSlider->s32CurrentSetting < s32Low)
	{
		psSlider->s32CurrentSetting = s32Low;
	}

	psSlider->s32LowValue = s32Min;
	psSlider->s32HighValue = s32Max;

	eLCDErr = SliderSetValueInternal(psSlider, psSlider->s32CurrentSetting);

	return(eLCDErr);
}

ELCDErr SliderSetValue(SLIDERHANDLE eSliderHandle,
					   INT32 s32Value)
{
	INT32 s32Low;
	INT32 s32High;
	ELCDErr eErr = LERR_OK;
	SSlider *psSlider;
	SWidget *psWidget;

	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eSliderHandle,
									WIDGET_SLIDER,
									&psWidget,
									NULL);

	RETURN_ON_FAIL(eErr);

	psSlider = psWidget->uWidgetSpecific.psSlider;
	GCASSERT(psSlider);

	if (psSlider->s32LowValue > psSlider->s32HighValue)
	{
		s32Low = psSlider->s32HighValue;
		s32High = psSlider->s32LowValue;
	}
	else
	{
		s32High = psSlider->s32HighValue;
		s32Low = psSlider->s32LowValue;
	}

	if (s32Value < s32Low)
	{
		s32Value = s32Low;
	}

	if (s32Value > s32High)
	{
		s32Value = s32High;
	}

	eErr = SliderSetValueInternal(psSlider, s32Value);

	return(eErr);
}

ELCDErr SliderSetTrackLength(SLIDERHANDLE eSliderHandle,
							 UINT32 u32TrackLength)
{
	ELCDErr eLCDErr;
	SWidget *psWidget;
	SSlider *psSlider;

	eLCDErr = WidgetGetPointerByHandle((WIDGETHANDLE) eSliderHandle,
										WIDGET_SLIDER,
										&psWidget,
										NULL);

	RETURN_ON_FAIL(eLCDErr);

	psSlider = psWidget->uWidgetSpecific.psSlider;
	GCASSERT(psSlider);

	eLCDErr = SliderSetImages(eSliderHandle,
							  psSlider->psTicks,
							  psSlider->psThumb,
							  u32TrackLength,
							  TRUE);


	return(eLCDErr);
}

ELCDErr SliderSetLock(SLIDERHANDLE eSliderHandle,
					  BOOL bLock)
{
	ELCDErr eLCDErr;
	SWidget *psWidget;

	eLCDErr = WidgetGetPointerByHandle((WIDGETHANDLE) eSliderHandle,
										WIDGET_SLIDER,
										&psWidget,
										NULL);

	RETURN_ON_FAIL(eLCDErr);

	psWidget->uWidgetSpecific.psSlider->bLock = bLock;

	return(LERR_OK);
}

ELCDErr SliderGetValue(SLIDERHANDLE eSliderHandle,
					   INT32 *ps32Value)
{
	ELCDErr eLCDErr;
	SWidget *psWidget;

	eLCDErr = WidgetGetPointerByHandle((WIDGETHANDLE) eSliderHandle,
										WIDGET_SLIDER,
										&psWidget,
										NULL);

	RETURN_ON_FAIL(eLCDErr);

	if (ps32Value)
	{
		*ps32Value = SliderGetValueFromThumb(psWidget->uWidgetSpecific.psSlider);
	}

	return(LERR_OK);
}

ELCDErr SliderSetSound(SLIDERHANDLE eSliderHandle,
					   SOUNDHANDLE eSoundHandle)
{
	ELCDErr eLCDErr;
	SWidget *psWidget;

	eLCDErr = WidgetGetPointerByHandle((WIDGETHANDLE) eSliderHandle,
										WIDGET_SLIDER,
										&psWidget,
										NULL);

	RETURN_ON_FAIL(eLCDErr);

	if ((eSoundHandle != HANDLE_INVALID) && NULL == WaveGetPointer(eSoundHandle))
	{
		return(LERR_SOUND_WAVE_BAD_HANDLE);
	}

	psWidget->uWidgetSpecific.psSlider->eSoundHandle = eSoundHandle;

	return(LERR_OK);
}

ELCDErr SliderRenderStretch(SImageGroup **ppsCreatedTrack,
							INT32 s32TrackLength,
							SImageGroup *psTopLeft,
							SImageGroup *psTrack,
							SImageGroup *psBottomRight,
							SImageGroup *psThumb,
							UINT16 u16Orientation)
{
	UINT32 u32XSize = 0;
	UINT32 u32YSize = 0;
	SImage *psImage = NULL;
	ELCDErr eErr = LERR_OK;

	// Set the current X/Y size based on the top/left
	if (psTopLeft)
	{
		u32XSize = psTopLeft->psCurrentImage->u32XSize;
		u32YSize = psTopLeft->psCurrentImage->u32YSize;
	}

	// Now the bottom/right
	if (psBottomRight)
	{
		if ((ROT_0 == u16Orientation) ||
			(ROT_180 == u16Orientation))
		{
			u32YSize += psBottomRight->psCurrentImage->u32YSize;
			if (psBottomRight->psCurrentImage->u32XSize > u32XSize)
			{
				u32XSize = psBottomRight->psCurrentImage->u32XSize;
			}
		}
		else
		{
			u32XSize += psBottomRight->psCurrentImage->u32XSize;
			if (psBottomRight->psCurrentImage->u32YSize > u32YSize)
			{
				u32YSize = psBottomRight->psCurrentImage->u32YSize;
			}
		}
	}

	// Now the track segment
	if (psTrack)
	{
		if ((ROT_0 == u16Orientation) ||
			(ROT_180 == u16Orientation))
		{
			u32YSize += (s32TrackLength * psTrack->psCurrentImage->u32YSize);
			if (psTrack->psCurrentImage->u32XSize > u32XSize)
			{
				u32XSize = psTrack->psCurrentImage->u32XSize;
			}
		}
		else
		{
			u32XSize += (s32TrackLength * psTrack->psCurrentImage->u32XSize);
			if (psTrack->psCurrentImage->u32YSize > u32YSize)
			{
				u32YSize = psTrack->psCurrentImage->u32YSize;
			}
		}
	}

	*ppsCreatedTrack = GfxImageGroupCreate();
	if (NULL == *ppsCreatedTrack)
	{
		eErr = LERR_NO_MEM;
		goto errorExit;
	}

	if (psThumb)
	{
		if ((ROT_0 == u16Orientation) ||
			(ROT_180 == u16Orientation))
		{
			if (u32XSize < psThumb->psCurrentImage->u32XSize)
			{
				u32XSize = psThumb->psCurrentImage->u32XSize;
			}
		}
		else
		{
			if (u32YSize < psThumb->psCurrentImage->u32YSize)
			{
				u32YSize = psThumb->psCurrentImage->u32YSize;
			}
		}
	}

	psImage = GfxCreateEmptyImage(u32XSize, u32YSize, 16, 0, TRUE);
	if (NULL == psImage)
	{
		eErr = LERR_NO_MEM;
		goto errorExit;
	}

	if (NULL == GfxImageGroupAppend(*ppsCreatedTrack,
									psImage))
	{
		eErr = LERR_NO_MEM;
		goto errorExit;
	}

	if ((ROT_0 == u16Orientation) ||
		(ROT_180 == u16Orientation))
	{
		UINT32 u32YPos = 0;

		// Vertical slider - top
		if (psTopLeft)
		{
			GfxBlitImageToImage(psImage,
								psTopLeft->psCurrentImage,
								(u32XSize >> 1) - (psTopLeft->psCurrentImage->u32XSize >> 1),
								u32YPos,
								0, 0);

			u32YPos += psTopLeft->psCurrentImage->u32YSize;
		}
	
		// Vertical - middle image

		if (psTrack)
		{
			while (s32TrackLength > 0)
			{
				GfxBlitImageToImage(psImage,
									psTrack->psCurrentImage,
									(u32XSize >> 1) - (psTopLeft->psCurrentImage->u32XSize >> 1),
									u32YPos,
									0, 0);
				u32YPos += psTrack->psCurrentImage->u32YSize;
				s32TrackLength--;
			}
		}

		// Vertical - bottom
		if (psBottomRight)
		{
			GfxBlitImageToImage(psImage,
								psBottomRight->psCurrentImage,
								(u32XSize >> 1) - (psTopLeft->psCurrentImage->u32XSize >> 1),
								u32YPos,
								0, 0);
		}
	}
	else
	{
		UINT32 u32XPos = 0;

		// Horizontal - left side
		if (psTopLeft)
		{
			GfxBlitImageToImage(psImage,
								psTopLeft->psCurrentImage,
								u32XPos,
								0,
								0, 0);

			u32XPos += psTopLeft->psCurrentImage->u32XSize;
		}
	
		// Horizontal - middle image

		if (psTrack)
		{
			while (s32TrackLength > 0)
			{
				GfxBlitImageToImage(psImage,
									psTrack->psCurrentImage,
									u32XPos,
									0,
									0, 0);
				u32XPos += psTrack->psCurrentImage->u32XSize;
				s32TrackLength--;
			}
		}

		// Horizontal - right side
		if (psBottomRight)
		{
			GfxBlitImageToImage(psImage,
								psBottomRight->psCurrentImage,
								u32XPos,
								0,
								0, 0);
		}
	}

	eErr = LERR_OK;

errorExit:
	if (eErr != LERR_OK)
	{
		if (psImage)
		{
			GfxDeleteImage(psImage);
		}

		if (*ppsCreatedTrack)
		{
			GfxDeleteImageGroup(*ppsCreatedTrack);
			*ppsCreatedTrack = NULL;
		}
	}

	return(eErr);
}
