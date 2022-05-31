#include "Startup/app.h"
#include "Libs/Gfx/GraphicsLib.h"
#include "Libs/window/window.h"
#include "Libs/widget/widget.h"
#include "Libs/widget/button/button.h"

static ELCDErr ButtonWidgetAlloc(SWidget *psWidget,
								 WIDGETHANDLE eHandle)
{
	GCASSERT(psWidget);
	psWidget->uWidgetSpecific.psButton = MemAlloc(sizeof(*psWidget->uWidgetSpecific.psButton));
	if (NULL == psWidget->uWidgetSpecific.psButton)
	{
		return(LERR_NO_MEM);
	}
	else
	{
		psWidget->uWidgetSpecific.psButton->eButtonHandle = (BUTTONHANDLE) eHandle;
		psWidget->uWidgetSpecific.psButton->psWidget = psWidget;
		psWidget->uWidgetSpecific.psButton->eButtonDownWave = HANDLE_INVALID;
		psWidget->uWidgetSpecific.psButton->eButtonUpWave = HANDLE_INVALID;
		psWidget->uWidgetSpecific.psButton->eButtonState = BUTTON_STATE_ENABLED_NORMAL;
		return(LERR_OK);
	}
}

static ELCDErr ButtonWidgetFree(SWidget *psWidget)
{
	SButton *psButton;

	psButton = psWidget->uWidgetSpecific.psButton;
	if (NULL == psButton)
	{
		// Nothing to destroy!
		return(LERR_OK);
	}

	// If we have an image group for "up", delete it
	if (psButton->psNormal)
	{
		GfxDeleteImageGroup(psButton->psNormal);
		psButton->psNormal = NULL;
	}

	// If we have an image group for "down", delete it
	if (psButton->psPressed)
	{
		GfxDeleteImageGroup(psButton->psPressed);
		psButton->psPressed = NULL;
	}

	// If we have an image group for state indicator
	if (psButton->psIndicator)
	{
		GfxDeleteImageGroup(psButton->psIndicator);
		psButton->psIndicator = NULL;
	}

	// If we have hit masks, delete them
	if (psButton->pu8NormalHitMask)
	{
		GCFreeMemory(psButton->pu8NormalHitMask);
	}

	// If we have hit masks, delete them
	if (psButton->pu8PressedHitMask && (psButton->pu8NormalHitMask != psButton->pu8PressedHitMask))
	{
		GCFreeMemory(psButton->pu8PressedHitMask);
	}

	// Null out the pointers
	psButton->pu8NormalHitMask = NULL;
	psButton->pu8PressedHitMask = NULL;

	// Go deallocate the sound handles (if applicable)

	(void) WaveDestroy(psButton->eButtonUpWave);
	(void) WaveDestroy(psButton->eButtonDownWave);

	psButton->eButtonHandle = HANDLE_INVALID;

	(void) WaveDestroy(psButton->eButtonUpWave);
	psButton->eButtonUpWave = HANDLE_INVALID;

	(void) WaveDestroy(psButton->eButtonDownWave);
	psButton->eButtonDownWave = HANDLE_INVALID;

	GCFreeMemory(psWidget->uWidgetSpecific.psButton);
	psWidget->uWidgetSpecific.psButton = NULL;

	// SoundStreamWaveShutdown
	return(LERR_OK);
}

static BOOL ButtonHitTest(SWidget *psWidget,
						  UINT32 u32XPos, 
						  UINT32 u32YPos)
{
	SButton *psButton = psWidget->uWidgetSpecific.psButton;
	UINT8 *pu8HitMask = NULL;
	UINT32 u32Pitch = 0;
	INT32 s32XPos = 0;
	INT32 s32YPos = 0;
	SImage *psHitImage = NULL;

	// This assumes that we're at least in the ballpark of this button.
	GCASSERT(psButton);

	// Are we doing a "normal" hit test?

	if (((BUTTON_STATE_ENABLED_NORMAL == psButton->eButtonState) ||
		 (BUTTON_STATE_DISABLED_NORMAL == psButton->eButtonState)) &&
		(psButton->bHitMaskNormal) &&
		(psButton->psNormal) &&
		(psButton->psNormal->psCurrentImage))
	{
		psHitImage = psButton->psNormal->psCurrentImage;
	}

	if (((BUTTON_STATE_ENABLED_PRESSED == psButton->eButtonState) ||
		 (BUTTON_STATE_DISABLED_PRESSED == psButton->eButtonState)) &&
		(psButton->bHitMaskPressed) &&
		(psButton->psPressed) &&
		(psButton->psPressed->psCurrentImage))
	{
		psHitImage = psButton->psPressed->psCurrentImage;
	}

	if (psHitImage)
	{
		u32Pitch = psHitImage->u32Pitch;
		s32XPos = (INT32) u32XPos - (INT32) psButton->psWidget->s32XPos;
		GCASSERT(s32XPos >= 0);
		s32YPos = (INT32) u32YPos - (INT32) psButton->psWidget->s32YPos;
		GCASSERT(s32YPos >= 0);

		if (psHitImage->pu8Transparent)
		{
			pu8HitMask = psHitImage->pu8Transparent;
		}
		else
		{
			pu8HitMask = psHitImage->pu8TranslucentMask;
		}

		if (s32XPos > (INT32) psHitImage->u32XSize)
		{
			return(FALSE);
		}

		if (s32YPos > (INT32) psHitImage->u32YSize)
		{
			return(FALSE);
		}
	}
	else
	{
		pu8HitMask = psButton->pu8NormalHitMask;
	}

	if (pu8HitMask)
	{
		// Whoa. Let's try the hit mask. 
		pu8HitMask += (s32XPos) + (s32YPos * u32Pitch);
		if (*pu8HitMask)
		{
			return(FALSE);
		}
		else
		{
			return(TRUE);
		}
	}
	return(TRUE);
}

static void ButtonPaint(SWidget *psWidget,
						BOOL bLock)
{
	SButton *psButton = psWidget->uWidgetSpecific.psButton;
	SWindow *psWindow;
	SImage *psSrcImage = NULL;
	INT32 s32XPos = 0;
	INT32 s32YPos = 0;
	
	GCASSERT(psButton);
	psWindow = WindowGetPointer(psWidget->eParentWindow);
	GCASSERT(psWindow);

	// Set our X/Y position
	s32XPos = (INT32) psWidget->s32XPos;
	s32YPos = (INT32) psWidget->s32YPos;

	// If we're in normal state, then draw it
	if ((BUTTON_STATE_ENABLED_NORMAL == psButton->eButtonState) || (BUTTON_STATE_DISABLED_NORMAL == psButton->eButtonState))
	{
		// Normal button. Go draw it.
		if ((NULL == psButton->psNormal) ||
			(NULL == psButton->psNormal->psCurrentImage))
		{
			return;
		}

		psSrcImage = psButton->psNormal->psCurrentImage;
		s32XPos += psButton->s32XOffsetNormal;
		s32YPos += psButton->s32YOffsetNormal;
	}
	else
	if ((BUTTON_STATE_ENABLED_PRESSED == psButton->eButtonState) || (BUTTON_STATE_DISABLED_PRESSED == psButton->eButtonState))
	{
		if ((NULL == psButton->psPressed) ||
			(NULL == psButton->psPressed->psCurrentImage))
		{
			return;
		}

		// Pressed button. Go draw it.
		psSrcImage = psButton->psPressed->psCurrentImage;
		s32XPos += psButton->s32XOffsetPressed;
		s32YPos += psButton->s32YOffsetPressed;
	}
	else
	{
		GCASSERT(0);
	}

	// If we have nothing to draw, then return
	if (NULL == psSrcImage)
	{
		return;
	}

	// Figure out where specifically to draw it

	if (s32XPos < 0)
	{
		s32XPos = 0;
	}
	if (s32YPos < 0)
	{
		s32YPos = 0;
	}

	// Now add in the active region area

	s32XPos += (INT32) psWindow->u32ActiveAreaXPos;
	s32YPos += (INT32) psWindow->u32ActiveAreaYPos;
	psWidget->u32XSize = psSrcImage->u32XSize;
	psWidget->u32YSize = psSrcImage->u32YSize;

	WidgetErase(psWidget);

	// Go blit the image if it's not disabled
	if ((psButton->eButtonState != BUTTON_STATE_DISABLED_NORMAL) &&
		(psButton->eButtonState != BUTTON_STATE_DISABLED_PRESSED))
	{
		GfxBlitImageToImage(psWindow->psWindowImage,
							psSrcImage,
							(UINT32) s32XPos,
							(UINT32) s32YPos,
							psWindow->u32ActiveAreaXPos + psWindow->u32ActiveAreaXSize,
							psWindow->u32ActiveAreaYPos + psWindow->u32ActiveAreaYSize);
	}
	else
	{
		GfxBlitImageToImageGray(psWindow->psWindowImage,
								psSrcImage,
								(UINT32) s32XPos,
								(UINT32) s32YPos,
								psWindow->u32ActiveAreaXPos + psWindow->u32ActiveAreaXSize,
								psWindow->u32ActiveAreaYPos + psWindow->u32ActiveAreaYSize);
	}

	// Now see if we have an indicator, and if so, draw it

	if ((psButton->psIndicator) && (psButton->bIndicatorOn))
	{
		if ((psButton->eButtonState != BUTTON_STATE_DISABLED_NORMAL) &&
			(psButton->eButtonState != BUTTON_STATE_DISABLED_PRESSED))
		{
			GfxBlitImageToImage(psWindow->psWindowImage,
								psButton->psIndicator->psCurrentImage,
								(UINT32) s32XPos + psButton->s32XOffsetIndicator,
								(UINT32) s32YPos + psButton->s32YOffsetIndicator,
								psWindow->u32ActiveAreaXPos + psWindow->u32ActiveAreaXSize,
								psWindow->u32ActiveAreaYPos + psWindow->u32ActiveAreaYSize);
		}
		else
		{
			GfxBlitImageToImageGray(psWindow->psWindowImage,
									psButton->psIndicator->psCurrentImage,
									(UINT32) s32XPos + psButton->s32XOffsetIndicator,
									(UINT32) s32YPos + psButton->s32YOffsetIndicator,
									psWindow->u32ActiveAreaXPos + psWindow->u32ActiveAreaXSize,
									psWindow->u32ActiveAreaYPos + psWindow->u32ActiveAreaYSize);
		}
	}

	WindowUpdateRegion(psWidget->eParentWindow,
					   (UINT32) s32XPos,
					   (UINT32) s32YPos,
					   psSrcImage->u32XSize,
					   psSrcImage->u32YSize);
}

static void ButtonErase(SWidget *psWidget)
{
	SButton *psButton = psWidget->uWidgetSpecific.psButton;
	SWindow *psWindow;
	SImage *psSrcImage = NULL;
	INT32 s32XPos = 0;
	INT32 s32YPos = 0;
	UINT32 u32XSize = 0;
	UINT32 u32YSize = 0;
	
	GCASSERT(psButton);
	psWindow = WindowGetPointer(psWidget->eParentWindow);
	GCASSERT(psWindow);

	// Set our X/Y position
	s32XPos = psWidget->s32XPos;
	s32YPos = psWidget->s32YPos;

	// If we're in normal state, then draw it
	if ((BUTTON_STATE_ENABLED_NORMAL == psButton->eButtonState) || (BUTTON_STATE_DISABLED_NORMAL == psButton->eButtonState))
	{
		if ((NULL == psButton->psNormal) ||
			(NULL == psButton->psNormal->psCurrentImage))
		{
			return;
		}

		// Normal button. Go draw it.
		psSrcImage = psButton->psNormal->psCurrentImage;
		s32XPos += psButton->s32XOffsetNormal;
		s32YPos += psButton->s32YOffsetNormal;
	}
	else
	if ((BUTTON_STATE_ENABLED_PRESSED == psButton->eButtonState) || (BUTTON_STATE_DISABLED_PRESSED == psButton->eButtonState))
	{
		if ((NULL == psButton->psPressed) ||
			(NULL == psButton->psPressed->psCurrentImage))
		{
			return;
		}

		// Pressed button. Go draw it.
		psSrcImage = psButton->psPressed->psCurrentImage;
		s32XPos += psButton->s32XOffsetPressed;
		s32YPos += psButton->s32YOffsetPressed;
	}
	else
	{
		GCASSERT(0);
	}

	// If we have nothing to draw, then return
	if (NULL == psSrcImage)
	{
		return;
	}

	// Figure out where specifically to erase

	if (s32XPos < 0)
	{
		s32XPos = 0;
	}
	if (s32YPos < 0)
	{
		s32YPos = 0;
	}

	WindowEraseActiveRegion(psWindow,
							(UINT32) s32XPos,
							(UINT32) s32YPos,
							psSrcImage->u32XSize,
							psSrcImage->u32YSize);

	WindowUpdateRegion(psWidget->eParentWindow,
					   (UINT32) s32XPos,
					   (UINT32) s32YPos,
					   psSrcImage->u32XSize,
					   psSrcImage->u32YSize);
}

ELCDErr ButtonStateSet(BUTTONHANDLE eButtonHandle,
					   EButtonState eState)
{
	SButton *psButton;
	SWidget *psWidget;
	ELCDErr eErr;

	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eButtonHandle,
									WIDGET_BUTTON,
									&psWidget,
									NULL);
	RETURN_ON_FAIL(eErr);

	psButton = psWidget->uWidgetSpecific.psButton;
	GCASSERT(psButton);

	if (psButton->eButtonState == eState)
	{
		return(LERR_OK);
	}

	// Let's erase the existing button image
	if( FALSE == psButton->psWidget->bWidgetHidden )
	{
		ButtonErase(psButton->psWidget);
		WidgetEraseIntersections(psButton->psWidget);
		WidgetPaintIntersections(psButton->psWidget);
	}

	psButton->eButtonState = eState;

	// Now draw the button image
	if( FALSE == psButton->psWidget->bWidgetHidden )
	{
		ButtonPaint(psButton->psWidget,
					FALSE);
	}

	// Let's cause a redraw
	WindowUpdateRegionCommit();
	return(LERR_OK);
}

static void ButtonPress(SWidget *psWidget,
						UINT32 u32Mask,
						UINT32 u32XPos,
						UINT32 u32YPos)
{
	SButton *psButton = psWidget->uWidgetSpecific.psButton;
	UWidgetCallbackData uData;

	// Clear out our data
	memset((void *) &uData, 0, sizeof(uData));

	// Put in callback specific data
	uData.sButton.u32ButtonMask = u32Mask;
	uData.sButton.u32XPos = u32XPos;
	uData.sButton.u32YPos = u32YPos;

	GCASSERT(psButton);

	// If this button is disabled, throw the message away
	if ((BUTTON_STATE_DISABLED_NORMAL == psButton->eButtonState) ||
		(BUTTON_STATE_DISABLED_PRESSED == psButton->eButtonState))
	{
		return;
	}

	if (BUTTON_STATE_ENABLED_NORMAL == psButton->eButtonState)
	{
		ELCDErr eErr;

		eErr = WindowLockByHandle(psWidget->eParentWindow);
		if (eErr != LERR_OK)
		{
			// Can't touch it! 
			return;
		}

		// Let's erase the existing button image
		if( FALSE == psButton->psWidget->bWidgetHidden )
		{
			ButtonErase(psButton->psWidget);
			WidgetEraseIntersections(psButton->psWidget);
			WidgetPaintIntersections(psButton->psWidget);
		}

		psButton->eButtonState = BUTTON_STATE_ENABLED_PRESSED;

		// Now draw the button image
		if( FALSE == psButton->psWidget->bWidgetHidden )
		{
			ButtonPaint(psButton->psWidget,
						FALSE);
		}

		// Check for a problem seen recently
		GCASSERT( psWidget );
		GCASSERT( psButton );
		GCASSERT( psButton->psPressed );
		GCASSERT( psButton->psPressed->psCurrentImage );

		psWidget->u32XSize = psButton->psPressed->psCurrentImage->u32XSize;
		psWidget->u32YSize = psButton->psPressed->psCurrentImage->u32YSize;

		(void) WindowUnlockByHandle(psWidget->eParentWindow);

		// Let's cause a redraw
		WindowUpdateRegionCommit();

		// And play a sound if applicable
		WidgetSoundPlay(psButton->eButtonDownWave);

		// Button pressed
		uData.sButton.bPress = TRUE;
		goto userCallback;
	}

	if (psButton->bButtonSticky && (BUTTON_STATE_ENABLED_PRESSED == psButton->eButtonState))
	{
		ELCDErr eErr;

		eErr = WindowLockByHandle(psWidget->eParentWindow);
		if (eErr != LERR_OK)
		{
			// Can't touch it! 
			return;
		}

		if( FALSE == psButton->psWidget->bWidgetHidden )
		{
			ButtonErase(psButton->psWidget);
			WidgetEraseIntersections(psButton->psWidget);
			WidgetPaintIntersections(psButton->psWidget);
		}

		psButton->eButtonState = BUTTON_STATE_ENABLED_NORMAL;

		// Now draw the button image
		if( FALSE == psButton->psWidget->bWidgetHidden )
		{
			ButtonPaint(psButton->psWidget,
						FALSE);
		}

		// Now that we're the normal state, let's set the widget's X/Y size to
		// that image
		psWidget->u32XSize = psButton->psNormal->psCurrentImage->u32XSize;
		psWidget->u32YSize = psButton->psNormal->psCurrentImage->u32YSize;

		(void) WindowUnlockByHandle(psWidget->eParentWindow);

		// Let's cause a redraw
		WindowUpdateRegionCommit();

		// And play a sound if applicable
		WidgetSoundPlay(psButton->eButtonUpWave);

		// Button released
		uData.sButton.bPress = FALSE;
		goto userCallback;
	}

	return;

userCallback:
	WidgetBroadcastMask((WIDGETHANDLE) psButton->eButtonHandle,
						WCBK_SPECIFIC,
						&uData);
}

static void ButtonRelease(SWidget *psWidget,
						  UINT32 u32Mask,
						  UINT32 u32XPos,
						  UINT32 u32YPos)
{
	SButton *psButton = psWidget->uWidgetSpecific.psButton;

	UWidgetCallbackData uData;

	// Clear out our data
	memset((void *) &uData, 0, sizeof(uData));

	// Put in callback specific data
	uData.sButton.u32ButtonMask = u32Mask;
	uData.sButton.u32XPos = u32XPos;
	uData.sButton.u32YPos = u32YPos;

	GCASSERT(psButton);
	if ((BUTTON_STATE_DISABLED_NORMAL == psButton->eButtonState) ||
		(BUTTON_STATE_DISABLED_PRESSED == psButton->eButtonState))
	{
		return;
	}

	if (psButton->bButtonSticky)
	{
		// Ignore button releases
		return;
	}

	if (BUTTON_STATE_ENABLED_PRESSED == psButton->eButtonState)
	{
		ELCDErr eErr;

		eErr = WindowLockByHandle(psWidget->eParentWindow);
		if (eErr != LERR_OK)
		{
			// Can't touch it! 
			return;
		}

		// Let's erase the existing button image
		if( FALSE == psButton->psWidget->bWidgetHidden )
		{
			ButtonErase(psButton->psWidget);
			WidgetEraseIntersections(psButton->psWidget);
			WidgetPaintIntersections(psButton->psWidget);
		}

		psButton->eButtonState = BUTTON_STATE_ENABLED_NORMAL;

		// Now draw the button image
		if( FALSE == psButton->psWidget->bWidgetHidden )
		{
			ButtonPaint(psButton->psWidget,
						FALSE);
		}

		(void) WindowUnlockByHandle(psWidget->eParentWindow);

		// Let's cause a redraw
		WindowUpdateRegionCommit();

		// And play a sound if applicable
		WidgetSoundPlay(psButton->eButtonUpWave);

		// Indicate we've released
		uData.sButton.bPress = FALSE;

		WidgetBroadcastMask((WIDGETHANDLE) psButton->eButtonHandle,
							WCBK_SPECIFIC,
							&uData);
	}
}

ELCDErr ButtonSetSticky(BUTTONHANDLE eButtonHandle,
						BOOL bSticky)
{
	SButton *psButton;
	ELCDErr eErr;
	SWidget *psWidget;

	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eButtonHandle,
									WIDGET_BUTTON,
									&psWidget,
									NULL);
	RETURN_ON_FAIL(eErr);

	psButton = psWidget->uWidgetSpecific.psButton;
	GCASSERT(psButton);

	if (psButton->bButtonSticky == bSticky)
	{
		return(LERR_OK);
	}

	eErr = WindowLockByHandle(psButton->psWidget->eParentWindow);
	if (eErr != LERR_OK)
	{
		// Can't touch it! 
		goto errorExit;
	}

	if (psButton->bButtonSticky && ((BUTTON_STATE_ENABLED_PRESSED == psButton->eButtonState) ||
									(BUTTON_STATE_DISABLED_PRESSED == psButton->eButtonState)))
	{
		// We need to set the button's state to not pressed since we're
		// unstickying a button

		if (BUTTON_STATE_ENABLED_PRESSED)
		{
			eErr = ButtonStateSet(eButtonHandle,
								  BUTTON_STATE_ENABLED_NORMAL);
		}
		else
		{
			eErr = ButtonStateSet(eButtonHandle,
								  BUTTON_STATE_DISABLED_NORMAL);
		}
	}

	psButton->bButtonSticky = bSticky;
	if (LERR_OK == eErr)
	{
		eErr = WindowUnlockByHandle(psButton->psWidget->eParentWindow);
	}
	else
	{
		(void) WindowUnlockByHandle(psButton->psWidget->eParentWindow);
	}

errorExit:
	return(eErr);
}

static void ButtonCheckIntersection(SWidget *psWidget,
									INT32 *ps32XPos,
									INT32 *ps32YPos,
									UINT32 *pu32XSize,
									UINT32 *pu32YSize)
{
	INT32 s32MinX = (INT32) (((UINT32) 1 << 31) - 1);
	INT32 s32MinY = (INT32) (((UINT32) 1 << 31) - 1);
	INT32 s32MaxX = -(1 << 31);
	INT32 s32MaxY = -(1 << 31);
	INT32 s32XPos = 0;
	INT32 s32YPos = 0;
	SButton *psButton;

	psButton = psWidget->uWidgetSpecific.psButton;
	GCASSERT(psButton);

	// Check for the nonpressed image
	if (psButton->psNormal)
	{
		s32XPos = psWidget->s32XPos + psButton->s32XOffsetNormal;
		s32YPos = psWidget->s32YPos + psButton->s32YOffsetNormal;

		if (s32XPos < s32MinX)
		{
			s32MinX = s32XPos;
		}

		if (s32YPos < s32MinY)
		{
			s32MinY = s32YPos;
		}

		s32XPos += psButton->psNormal->psCurrentImage->u32XSize;
		s32YPos += psButton->psNormal->psCurrentImage->u32YSize;

		if (s32XPos > s32MaxX)
		{
			s32MaxX = s32XPos;
		}
		if (s32YPos > s32MaxY)
		{
			s32MaxY = s32YPos;
		}
	}

	// Now the pressed image
	if (psButton->psPressed)
	{
		s32XPos = psWidget->s32XPos + psButton->s32XOffsetPressed;
		s32YPos = psWidget->s32YPos + psButton->s32YOffsetPressed;

		if (s32XPos < s32MinX)
		{
			s32MinX = s32XPos;
		}

		if (s32YPos < s32MinY)
		{
			s32MinY = s32YPos;
		}

		s32XPos += psButton->psPressed->psCurrentImage->u32XSize;
		s32YPos += psButton->psPressed->psCurrentImage->u32YSize;

		if (s32XPos > s32MaxX)
		{
			s32MaxX = s32XPos;
		}
		if (s32YPos > s32MaxY)
		{
			s32MaxY = s32YPos;
		}
	}

	// Now the indicator image
	if (psButton->psIndicator)
	{
		s32XPos = psWidget->s32XPos + psButton->s32XOffsetIndicator;
		s32YPos = psWidget->s32YPos + psButton->s32YOffsetIndicator;

		if (s32XPos < s32MinX)
		{
			s32MinX = s32XPos;
		}

		if (s32YPos < s32MinY)
		{
			s32MinY = s32YPos;
		}

		s32XPos += psButton->psIndicator->psCurrentImage->u32XSize;
		s32YPos += psButton->psIndicator->psCurrentImage->u32YSize;

		if (s32XPos > s32MaxX)
		{
			s32MaxX = s32XPos;
		}
		if (s32YPos > s32MaxY)
		{
			s32MaxY = s32YPos;
		}
	}

	// If we have an actual setting, then assign it
	if (s32MaxX != -(1 << 31))
	{
		*ps32XPos = s32MinX;
		*ps32YPos = s32MinY;
		*pu32XSize = (UINT32) (s32MaxX - s32MinX);
		*pu32YSize = (UINT32) (s32MaxY - s32MinY);
	}
}

static void ButtonWidgetAnimationTick(SWidget *psWidget,
									  UINT32 u32TickTime)
{
	ELCDErr eErr;
	SButton *psButton = psWidget->uWidgetSpecific.psButton;
	BOOL bChanged = FALSE;

	eErr = WindowLockByHandle(psWidget->eParentWindow);
	if (eErr != LERR_OK)
	{
		// Just get it next time if we can't get the lock
		return;
	}

	// Do a tick advancement
	if (psButton->psNormal)
	{
		bChanged = GfxAnimAdvance(psButton->psNormal,
								  u32TickTime);
	}

	if (psButton->psPressed)
	{
		if (bChanged)
		{
			(void) GfxAnimAdvance(psButton->psPressed,
								  u32TickTime);
		}
		else
		{
			bChanged = GfxAnimAdvance(psButton->psPressed,
									  u32TickTime);
		}
	}

	if (FALSE == bChanged)
	{
		// Don't do anything - the image hasn't changed yet
		(void) WindowUnlockByHandle(psWidget->eParentWindow);
		return;
	}

	// Erase the old image
	ButtonErase(psWidget);

	// Erase the intersections
	WidgetEraseIntersections(psWidget);

	// Paint the intersections
	WidgetPaintIntersections(psWidget);

	// Now paint the widget
	WidgetPaint(psWidget,
				FALSE);

	// Unlock the window
	(void) WindowUnlockByHandle(psWidget->eParentWindow);

	// Now commit the changes
	WindowUpdateRegionCommit();
}

static void ButtonSetFocus(SWidget *psWidget,
						   BOOL bFocusSet)
{
	DebugOut("%s: Focus value=%u\n", __FUNCTION__, bFocusSet);
}

static SWidgetFunctions sg_sButtonWidgetFunctions =
{
	ButtonHitTest,
	ButtonPaint,
	ButtonErase,
	ButtonPress,
	ButtonRelease,
	NULL,					// Mouseover
	ButtonSetFocus,			// Set/lose focus
	NULL,						// Keystroke for our widget
	ButtonWidgetAnimationTick,
	ButtonCheckIntersection,
	NULL,						// Mouse wheel
	NULL						// Set disable
};

ELCDErr ButtonCreate(WINDOWHANDLE eWindowHandle,
					 BUTTONHANDLE *peButtonHandle,
					 INT32 s32XPos,
					 INT32 s32YPos)
{
	ELCDErr eLCDErr;
	SWidget *psWidget;

	eLCDErr = WidgetAllocateHandle((WIDGETHANDLE *) peButtonHandle,
								   WIDGET_BUTTON,
								   eWindowHandle,
								   &psWidget);

	RETURN_ON_FAIL(eLCDErr);

	psWidget->s32XPos = s32XPos;
	psWidget->s32YPos = s32YPos;
	psWidget->bWidgetHidden = TRUE;

	// Make sure the widget is enabled
	psWidget->bWidgetEnabled = TRUE;

	return(LERR_OK);
}

ELCDErr ButtonSetNormalImage(BUTTONHANDLE eButtonHandle,
							 SImageGroup *psButtonImageGroup,
							 INT32 s32XOffset,
							 INT32 s32YOffset)
{
	SButton *psButton;
	ELCDErr eErr = LERR_OK;
	SWidget *psWidget;

	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eButtonHandle,
									WIDGET_BUTTON,
									&psWidget,
									NULL);
	RETURN_ON_FAIL(eErr);

	psButton = psWidget->uWidgetSpecific.psButton;
	GCASSERT(psButton);

	eErr = WindowLockByHandle(psButton->psWidget->eParentWindow);
	if (eErr != LERR_OK)
	{
		// Can't touch it! 
		return(eErr);
	}

	if ((BUTTON_STATE_ENABLED_NORMAL == psButton->eButtonState) ||
		(BUTTON_STATE_DISABLED_NORMAL == psButton->eButtonState))
	{
		// Let's erase the existing button image
		if( FALSE == psButton->psWidget->bWidgetHidden )
		{
			ButtonErase(psButton->psWidget);
			WidgetEraseIntersections(psButton->psWidget);
			WidgetPaintIntersections(psButton->psWidget);
		}
	}

	if (psButton->psNormal)
	{
		// If the button has more than 1 frame, delete it from the animation list
		if (psButton->psNormal->u32FrameCount > 1)
		{
			(void) WindowAnimationListDelete(psWidget->eParentWindow,
											 psWidget);
		}

		GfxDeleteImageGroup(psButton->psNormal);
		psButton->psNormal = NULL;
	}

	// Now set the normal image
	psButton->psNormal = psButtonImageGroup;
	GfxIncRef(psButton->psNormal);

	if (psButton->psNormal)
	{
		// Set the animation tick rate
		GfxAnimSetTickRate(psButton->psNormal,
						   WidgetGetAnimationStepTime());

		// Now set the interval - 1:1
		GfxAnimSetPlaybackSpeed(psButton->psNormal,
								0x10000);

		// Set the animation type - stopped to start with
		GfxAnimSetActiveType(psButton->psNormal,
							 EDIR_FORWARD);

		// Now shut off the repeat
		GfxAnimSetActiveRepeat(psButton->psNormal,
							   FALSE);

		// Add it to the list of managed images if there's > 1 frame
		if (psButton->psNormal->u32FrameCount > 1)
		{
			eErr = WindowAnimationListAdd(psWidget->eParentWindow,
										  psWidget);
			if (eErr != LERR_OK)
			{
				goto errorExit;
			}
		}
	}

	if (s32XOffset != NO_CHANGE)
	{
		psButton->s32XOffsetNormal = s32XOffset;
	}

	if (s32YOffset != NO_CHANGE)
	{
		psButton->s32YOffsetNormal = s32YOffset;
	}
	eErr = WidgetCalcIntersections(psButton->psWidget);

	if (((BUTTON_STATE_ENABLED_NORMAL == psButton->eButtonState) ||
		 (BUTTON_STATE_DISABLED_NORMAL == psButton->eButtonState)) &&
		(LERR_OK == eErr))
	{
		// Now draw the button image
		if( FALSE == psButton->psWidget->bWidgetHidden )
		{
			ButtonPaint(psButton->psWidget,
						FALSE);
		}

		// Let's cause a redraw
		WindowUpdateRegionCommit();
	}

	if (eErr != LERR_OK)
	{
		// Can't touch it! 
		goto errorExit;
	}

errorExit:
	if (LERR_OK == eErr)
	{
		eErr = WindowUnlockByHandle(psButton->psWidget->eParentWindow);
	}
	else
	{
		(void) WindowUnlockByHandle(psButton->psWidget->eParentWindow);
	}

	return(eErr);
}

ELCDErr ButtonSetPressedImage(BUTTONHANDLE eButtonHandle,
							  SImageGroup *psButtonImageGroup,
							  INT32 s32XOffset,
							  INT32 s32YOffset)
{
	SButton *psButton;
	ELCDErr eErr = LERR_OK;
	SWidget *psWidget;

	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eButtonHandle,
									WIDGET_BUTTON,
									&psWidget,
									NULL);
	RETURN_ON_FAIL(eErr);

	psButton = psWidget->uWidgetSpecific.psButton;
	GCASSERT(psButton);

	eErr = WindowLockByHandle(psButton->psWidget->eParentWindow);
	if (eErr != LERR_OK)
	{
		// Can't touch it! 
		return(eErr);
	}

	if ((BUTTON_STATE_ENABLED_PRESSED == psButton->eButtonState) ||
		(BUTTON_STATE_DISABLED_PRESSED == psButton->eButtonState))
	{
		// Let's erase the existing button image, but only if it's in the pressed state
		if( FALSE == psButton->psWidget->bWidgetHidden )
		{
			ButtonErase(psButton->psWidget);
			WidgetEraseIntersections(psButton->psWidget);
			WidgetPaintIntersections(psButton->psWidget);
		}
	}

	if (psButton->psPressed)
	{
		GfxDeleteImageGroup(psButton->psPressed);
		psButton->psPressed = NULL;
	}

	// Now set the normal image
	psButton->psPressed = psButtonImageGroup;
	GfxIncRef(psButton->psPressed);

	if (s32XOffset != NO_CHANGE)
	{
		psButton->s32XOffsetPressed = s32XOffset;
	}

	if (s32YOffset != NO_CHANGE)
	{
		psButton->s32YOffsetPressed = s32YOffset;
	}

	eErr = WidgetCalcIntersections(psButton->psWidget);

	if ((BUTTON_STATE_ENABLED_PRESSED == psButton->eButtonState) ||
		(BUTTON_STATE_DISABLED_PRESSED == psButton->eButtonState))
	{
		// Now draw the button image
		if( FALSE == psButton->psWidget->bWidgetHidden )
		{
			ButtonPaint(psButton->psWidget,
						FALSE);
		}

		// Let's cause a redraw
		WindowUpdateRegionCommit();
	}

	if (LERR_OK == eErr)
	{
		eErr = WindowUnlockByHandle(psButton->psWidget->eParentWindow);
	}
	else
	{
		(void) WindowUnlockByHandle(psButton->psWidget->eParentWindow);
	}

	return(eErr);
}

ELCDErr ButtonSetIndicatorImage(BUTTONHANDLE eButtonHandle,
							    SImageGroup *psIndicatorImageGroup,
								INT32 s32XOffset,
								INT32 s32YOffset)
{
	SButton *psButton;
	ELCDErr eErr;
	SWidget *psWidget;

	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eButtonHandle,
									WIDGET_BUTTON,
									&psWidget,
									NULL);
	RETURN_ON_FAIL(eErr);

	psButton = psWidget->uWidgetSpecific.psButton;
	GCASSERT(psButton);

	eErr = WindowLockByHandle(psButton->psWidget->eParentWindow);
	if (eErr != LERR_OK)
	{
		// Can't touch it! 
		goto errorExit;
	}

	// Let's erase the existing button image
	if( FALSE == psButton->psWidget->bWidgetHidden )
	{
		ButtonErase(psButton->psWidget);
		WidgetEraseIntersections(psButton->psWidget);
		WidgetPaintIntersections(psButton->psWidget);
	}

	if (psButton->psIndicator)
	{
		GfxDeleteImageGroup(psButton->psIndicator);
		psButton->psIndicator = NULL;
	}
	psButton->psIndicator = psIndicatorImageGroup;
	GfxIncRef(psButton->psIndicator);

	psButton->s32XOffsetIndicator = s32XOffset;
	psButton->s32YOffsetIndicator = s32YOffset;

	eErr = WidgetCalcIntersections(psButton->psWidget);

	// Now draw the button image
	if( FALSE == psButton->psWidget->bWidgetHidden )
	{
		ButtonPaint(psButton->psWidget,
					FALSE);
	}

	if (LERR_OK == eErr)
	{
		eErr = WindowUnlockByHandle(psButton->psWidget->eParentWindow);
	}
	else
	{
		(void) WindowUnlockByHandle(psButton->psWidget->eParentWindow);
	}

	// Let's cause a redraw
	WindowUpdateRegionCommit();

errorExit:
	return(eErr);
}

ELCDErr ButtonSetIndicatorState(BUTTONHANDLE eButtonHandle,
								BOOL bIndicatorState)
{
	SButton *psButton;
	ELCDErr eErr;
	SWidget *psWidget;

	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eButtonHandle,
									WIDGET_BUTTON,
									&psWidget,
									NULL);
	RETURN_ON_FAIL(eErr);

	psButton = psWidget->uWidgetSpecific.psButton;
	GCASSERT(psButton);

	// If there's no indicator image, kick back an error
	if (NULL == psButton->psIndicator)
	{
		return(LERR_BUTTON_NO_INDICATOR_SET);
	}

	// If we're already in this state, just return
	if (psButton->bIndicatorOn == bIndicatorState)
	{
		return(LERR_OK);
	}

	psButton->bIndicatorOn = bIndicatorState;

	eErr = WindowLockByHandle(psButton->psWidget->eParentWindow);
	if (eErr != LERR_OK)
	{
		// Can't touch it! 
		goto errorExit;
	}

	// Erase the button
	if( FALSE == psButton->psWidget->bWidgetHidden )
	{
		ButtonErase(psButton->psWidget);
		WidgetEraseIntersections(psButton->psWidget);
		WidgetPaintIntersections(psButton->psWidget);
	}

	// Now draw the button image
	if( FALSE == psButton->psWidget->bWidgetHidden )
	{
		ButtonPaint(psButton->psWidget,
					FALSE);
	}

	eErr = WindowUnlockByHandle(psButton->psWidget->eParentWindow);

	// Let's cause a redraw
	WindowUpdateRegionCommit();

errorExit:
	return(LERR_OK);
}

ELCDErr ButtonSetHitMaskNormal(BUTTONHANDLE eButtonHandle)
{
	SButton *psButton;
	SWidget *psWidget;
	ELCDErr eErr;

	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eButtonHandle,
									WIDGET_BUTTON,
									&psWidget,
									NULL);
	RETURN_ON_FAIL(eErr);

	psButton = psWidget->uWidgetSpecific.psButton;
	GCASSERT(psButton);

	// Normal image alpha. If we can.
	if (psButton->psNormal)
	{
		if ((NULL == psButton->psNormal->psCurrentImage))
		{
			return(LERR_BUTTON_NO_NORMAL_IMAGE);
		}

		if (psButton->psNormal->psCurrentImage->pu8TranslucentMask ||
			psButton->psNormal->psCurrentImage->pu8Transparent ||
			psButton->psNormal->psCurrentImage->pu16Palette)
		{
			psButton->bHitMaskNormal = TRUE;
			return(LERR_OK);
		}
		else
		{
			return(LERR_BUTTON_NORMAL_IMAGE_NO_MASK);
		}
	}
	else
	{
		return(LERR_BUTTON_NO_NORMAL_IMAGE);
	}

	return(LERR_OK);
}

ELCDErr ButtonSetHitMaskPressed(BUTTONHANDLE eButtonHandle)
{
	SButton *psButton;
	SWidget *psWidget;
	ELCDErr eErr;

	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eButtonHandle,
									WIDGET_BUTTON,
									&psWidget,
									NULL);
	RETURN_ON_FAIL(eErr);

	psButton = psWidget->uWidgetSpecific.psButton;
	GCASSERT(psButton);

	// Normal image alpha. If we can.
	if (psButton->psPressed)
	{
		if (NULL == psButton->psPressed->psCurrentImage)
		{
			return(LERR_BUTTON_NO_NORMAL_IMAGE);
		}

		if (psButton->psPressed->psCurrentImage->pu8TranslucentMask ||
			psButton->psPressed->psCurrentImage->pu8Transparent ||
			psButton->psNormal->psCurrentImage->pu16Palette)
		{
			psButton->bHitMaskPressed = TRUE;
			return(LERR_OK);
		}
		else
		{
			return(LERR_BUTTON_NORMAL_IMAGE_NO_MASK);
		}
	}
	else
	{
		return(LERR_BUTTON_NO_NORMAL_IMAGE);
	}

	return(LERR_OK);
}

ELCDErr ButtonSetButtonDownWave(BUTTONHANDLE eButtonHandle,
								SOUNDHANDLE eWaveHandle)
{
	SButton *psButton;
	SWidget *psWidget;
	ELCDErr eErr;

	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eButtonHandle,
									WIDGET_BUTTON,
									&psWidget,
									NULL);
	RETURN_ON_FAIL(eErr);

	psButton = psWidget->uWidgetSpecific.psButton;
	GCASSERT(psButton);

	if ((eWaveHandle != HANDLE_INVALID) && NULL == WaveGetPointer(eWaveHandle))
	{
		return(LERR_SOUND_WAVE_BAD_HANDLE);
	}

	// Got it! Attach it.
	psButton->eButtonDownWave = eWaveHandle;
	return(LERR_OK);
}

ELCDErr ButtonSetButtonUpWave(BUTTONHANDLE eButtonHandle,
							  SOUNDHANDLE eWaveHandle)
{
	SButton *psButton;
	SWidget *psWidget;
	ELCDErr eErr;

	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eButtonHandle,
									WIDGET_BUTTON,
									&psWidget,
									NULL);
	RETURN_ON_FAIL(eErr);

	psButton = psWidget->uWidgetSpecific.psButton;
	GCASSERT(psButton);

	if ((eWaveHandle != HANDLE_INVALID) && NULL == WaveGetPointer(eWaveHandle))
	{
		return(LERR_SOUND_WAVE_BAD_HANDLE);
	}

	// Got it! Attach it.
	psButton->eButtonUpWave = eWaveHandle;
	return(LERR_OK);
}

ELCDErr ButtonGetState(BUTTONHANDLE eButtonHandle,
					   EButtonState *peState)
{
	SWidget *psWidget;
	ELCDErr eErr;

	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eButtonHandle,
									WIDGET_BUTTON,
									&psWidget,
									NULL);
	RETURN_ON_FAIL(eErr);

	if (peState)
	{
		*peState = psWidget->uWidgetSpecific.psButton->eButtonState;
	}

	return(LERR_OK);
}

ELCDErr ButtonRender(UINT32 u32XSize,
					 UINT32 u32YSize,
					 LEX_CHAR *pu8FontFilename,
					 UINT32 u32FontSize,
					 LEX_CHAR *pu8Text,
					 UINT32 u32TextColor,
					 UINT32 u32ButtonColor,
					 SImageGroup **ppsImageGroup,
					 BOOL bDown,
					 UINT16 u16Orientation)
{
	ELCDErr eErr = LERR_OK;
	FONTHANDLE eFontHandle = HANDLE_INVALID;
	EGCResultCode eResult = GC_OK;
	SImage *psImage = NULL;
	UINT16 u16TextColor;
	UINT32 u32StringXSize = 0;
	UINT32 u32StringYSize = 0;
	INT32 s32XPos; 
	INT32 s32YPos;
	UINT32 u32CharCount;

	// Ensure the image group is NULL
	*ppsImageGroup = NULL;

	// Gotta be at least 3x3
	if (((u32XSize < 3) && (u32XSize != 0)) ||
		((u32YSize < 3) && (u32YSize != 0)))
	{
		eErr = LERR_BUTTON_SIZE_TOO_SMALL;
		goto errorExit;
	}

	// Mr. Font filename! If it's NULL, get the default
	if (NULL == pu8FontFilename)
	{
		TextGetActiveFontData(&pu8FontFilename,
							  NULL);
	}

	// Now, let's get the font size
	if (0 == u32FontSize)
	{
		TextGetActiveFontData(NULL,
							  &u32FontSize);
	}

	// Go load up the font for processing.
	eErr = FontCreate(pu8FontFilename,
					  u32FontSize,
					  0,
					  &eFontHandle);

	if (eErr != LERR_OK)
	{
		goto errorExit;
	}

	// Figure out if the font is going to fit
	eErr = FontGetStringSize(eFontHandle,
							 pu8Text,
							 &u32StringXSize,
							 &u32StringYSize,
							 ROT_0,
							 TRUE);
	if (eErr != LERR_OK)
	{
		goto errorExit;
	}

	// If 0 is passed in as a size on either axis, use the string size + a few
	if (0 == u32XSize)
	{
		u32XSize = u32StringXSize + 8;
	}

	if (0 == u32YSize)
	{
		u32YSize = u32StringYSize + 8;
	}

	// Does it fit?
	if ((u32StringXSize > (u32XSize - 3)) ||
		(u32StringYSize > (u32YSize - 3)))
	{
		eErr = LERR_BUTTON_TEXT_WONT_FIT;
		goto errorExit;
	}

	// Now the button
	u16TextColor = CONVERT_24RGB_16RGB(u32TextColor);

	eErr = BoxRender(u32XSize,
					 u32YSize,
					 0, 0,
					 u32ButtonColor,
					 &psImage,
					 bDown);

	// Now draw the string
	s32XPos = (INT32) ((u32XSize >> 1) - (u32StringXSize >> 1));
	s32YPos = (INT32) ((u32YSize >> 1) - (u32StringYSize >> 1));

	u32CharCount = Lexstrlen(pu8Text);

	// If we're pushing the button down, increment X/Y position by 1 each
	if (bDown)
	{
		s32XPos += 2;	
		s32YPos += 2;
	}

	// Move to 26.6 fixed point coordinates
	s32XPos <<= 6;
	s32YPos <<= 6;

	while (u32CharCount)
	{
		ELCDErr eLCDErr;
		INT32 s32XAdvance;
		INT32 s32YAdvance;
		
		eLCDErr = FontRender(eFontHandle,
							 HANDLE_INVALID,
							 s32XPos,
							 s32YPos,
							 u32XSize,
							 u32YSize,
							 (UINT32) *pu8Text,
							 u16TextColor,
							 TRUE,
							 &s32XAdvance,
							 &s32YAdvance,
							 FALSE,
							 ROT_0,
							 psImage,
							 FALSE,
							 TRUE);

		// In case the character isn't implemented
		if (LERR_OK == eLCDErr)
		{
			s32XPos += s32XAdvance;
		}

		++pu8Text;
		--u32CharCount;
	}

	// Now go create an image group
	*ppsImageGroup = GfxImageGroupCreate();
	if (NULL == *ppsImageGroup)
	{
		eErr = LERR_NO_MEM;
		goto errorExit;
	}

	// Rotate the image
	eResult = GfxRotateImage(&psImage,
							 (ERotation) u16Orientation);
	
	if (eResult != GC_OK)
	{
		eErr = LERR_NO_MEM;
		goto errorExit;
	}

	// Now append the image
	if (NULL == GfxImageGroupAppend(*ppsImageGroup,
									psImage))
	{
		eErr = LERR_NO_MEM;
	}

errorExit:
	// Destroy the font (ignore the error) since we don't need it
	(void) FontFree(eFontHandle);

	if (eErr != LERR_OK)
	{
		// If there's an image that has been created, delete it
		if (psImage)
		{
			GfxDeleteImage(psImage);
		}

		// Same deal with a group
		if (*ppsImageGroup)
		{
			GfxDeleteImageGroup(*ppsImageGroup);
			*ppsImageGroup = NULL;
		}
	}
	return(eErr);
}

static SWidgetTypeMethods sg_sButtonMethods = 
{
	&sg_sButtonWidgetFunctions,
	LERR_BUTTON_BAD_HANDLE,			// Error when it's not the handle we're looking for
	ButtonWidgetAlloc,
	ButtonWidgetFree
};

typedef enum
{
	EIMGSTATE_NONEXISTENT,			// Button image nonexistent
	EIMGSTATE_ANIMATABLE,			// Button image animatable
	EIMGSTATE_NONANIMATABLE		// Button image not animatable
} EImageState;

static ELCDErr ButtonWidgetPrepAnimation(BUTTONHANDLE eButtonWidgetHandle,
										 SButton **ppsButtonWidget)
{
	ELCDErr eErr;
	SWidget *psWidget;
	EImageState eNormalAnimatable = EIMGSTATE_NONEXISTENT;
	EImageState ePressedAnimatable = EIMGSTATE_NONEXISTENT;

	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eButtonWidgetHandle,
									WIDGET_BUTTON,
									&psWidget,
									NULL);

	RETURN_ON_FAIL(eErr);

	*ppsButtonWidget = psWidget->uWidgetSpecific.psButton;

	// Got the widget. Let's check the foreground image
	if ((*ppsButtonWidget)->psNormal)
	{
		if ((*ppsButtonWidget)->psNormal->u32FrameCount < 2)
		{
			eNormalAnimatable = EIMGSTATE_NONANIMATABLE;
		}
		else
		{
			eNormalAnimatable = EIMGSTATE_ANIMATABLE;
		}
	}

	if ((*ppsButtonWidget)->psPressed)
	{
		if ((*ppsButtonWidget)->psPressed->u32FrameCount < 2)
		{
			ePressedAnimatable = EIMGSTATE_NONANIMATABLE;
		}
		else
		{
			ePressedAnimatable = EIMGSTATE_ANIMATABLE;
		}
	}

	// If neither button is animatable, then we kick back an error
	if ((ePressedAnimatable != EIMGSTATE_ANIMATABLE) &&
		(eNormalAnimatable != EIMGSTATE_ANIMATABLE))
	{
		return(LERR_BUTTON_NOT_ANIMATABLE);
	}
	else
	{
		return(LERR_OK);
	}
}

ELCDErr ButtonWidgetAnimateStart(BUTTONHANDLE eButtonWidgetHandle)
{
	ELCDErr eErr;
	SButton *psButtonWidget;

	eErr = ButtonWidgetPrepAnimation(eButtonWidgetHandle,
									 &psButtonWidget);
	if (LERR_OK == eErr)
	{
		eErr = WindowLockByHandle(psButtonWidget->psWidget->eParentWindow);
		if (eErr != LERR_OK)
		{
			// Can't touch it! 
			goto errorExit;
		}

		// Now set the latched type

		if (psButtonWidget->psPressed)
		{
			GfxAnimSetActiveType(psButtonWidget->psPressed,
								 EDIR_FORWARD | EDIR_REPEAT);
			GfxAnimStart(psButtonWidget->psPressed);
		}

		if (psButtonWidget->psNormal)
		{
			GfxAnimSetActiveType(psButtonWidget->psPressed,
								 EDIR_FORWARD | EDIR_REPEAT);
			GfxAnimStart(psButtonWidget->psNormal);
		}

		// Now unlock the window
		eErr = WindowUnlockByHandle(psButtonWidget->psWidget->eParentWindow);
	}

errorExit:
	return(eErr);
}

ELCDErr ButtonWidgetAnimateStop(BUTTONHANDLE eButtonWidgetHandle)
{
	ELCDErr eErr;
	SButton *psButtonWidget;

	eErr = ButtonWidgetPrepAnimation(eButtonWidgetHandle,
									 &psButtonWidget);
	if (LERR_OK == eErr)
	{
		eErr = WindowLockByHandle(psButtonWidget->psWidget->eParentWindow);
		if (eErr != LERR_OK)
		{
			// Can't touch it! 
			goto errorExit;
		}

		if (psButtonWidget->psPressed)
		{
			GfxAnimStop(psButtonWidget->psPressed);
		}

		if (psButtonWidget->psNormal)
		{
			GfxAnimStop(psButtonWidget->psNormal);
		}

		// Now unlock the window
		eErr = WindowUnlockByHandle(psButtonWidget->psWidget->eParentWindow);
	}

errorExit:
	return(eErr);
}

ELCDErr ButtonWidgetAnimateStep(BUTTONHANDLE eButtonWidgetHandle)
{
	ELCDErr eErr;
	SButton *psButtonWidget;

	eErr = ButtonWidgetPrepAnimation(eButtonWidgetHandle,
									 &psButtonWidget);
	if (LERR_OK == eErr)
	{
		eErr = WindowLockByHandle(psButtonWidget->psWidget->eParentWindow);
		if (eErr != LERR_OK)
		{
			// Can't touch it! 
			goto errorExit;
		}

		if (psButtonWidget->psPressed)
		{
			GfxAnimStep(psButtonWidget->psPressed);
		}

		if (psButtonWidget->psNormal)
		{
			GfxAnimStep(psButtonWidget->psNormal);
		}

		// Now unlock the window
		eErr = WindowUnlockByHandle(psButtonWidget->psWidget->eParentWindow);
	}

errorExit:
	return(eErr);
}

ELCDErr ButtonWidgetAnimateReset(BUTTONHANDLE eButtonWidgetHandle)
{
	ELCDErr eErr;
	SButton *psButtonWidget;

	eErr = ButtonWidgetPrepAnimation(eButtonWidgetHandle,
									 &psButtonWidget);
	if (LERR_OK == eErr)
	{
		eErr = WindowLockByHandle(psButtonWidget->psWidget->eParentWindow);
		if (eErr != LERR_OK)
		{
			// Can't touch it! 
			goto errorExit;
		}

		if (psButtonWidget->psPressed)
		{
			GfxAnimReset(psButtonWidget->psPressed);
		}

		if (psButtonWidget->psNormal)
		{
			GfxAnimReset(psButtonWidget->psNormal);
		}

		// Now unlock the window
		eErr = WindowUnlockByHandle(psButtonWidget->psWidget->eParentWindow);
	}

errorExit:
	return(eErr);
}

ELCDErr ButtonGetDisabled(BUTTONHANDLE eButtonHandle,
						  BOOL *pbButtonDisabled)
{
	ELCDErr eErr;
	SButton *psButton;
	SWidget *psWidget;

	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eButtonHandle,
									WIDGET_BUTTON,
									&psWidget,
									NULL);
	RETURN_ON_FAIL(eErr);

	psButton = psWidget->uWidgetSpecific.psButton;
	GCASSERT(psButton);	

	if (LERR_OK == eErr)
	{
		if (pbButtonDisabled)
		{
			if ((BUTTON_STATE_DISABLED_NORMAL == psButton->eButtonState) ||
				(BUTTON_STATE_DISABLED_PRESSED == psButton->eButtonState))
			{
				*pbButtonDisabled = TRUE;
			}
			else
			{
				*pbButtonDisabled = FALSE;
			}
		}
	}

	return(eErr);
}

void ButtonFirstTimeInit(void)
{
	DebugOut("* Initializing button widget\n");
	WidgetRegisterTypeMethods(WIDGET_BUTTON,
							  &sg_sButtonMethods);
}