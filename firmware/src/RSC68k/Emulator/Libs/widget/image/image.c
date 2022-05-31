#include "Startup/app.h"
#include "Libs/Gfx/GraphicsLib.h"
#include "Libs/window/window.h"
#include "Libs/FontMgr/FontMgr.h"
#include "Libs/widget/widget.h"

static ELCDErr ImageWidgetAlloc(SWidget *psWidget,
								WIDGETHANDLE eHandle)
{
	GCASSERT(psWidget);
	psWidget->uWidgetSpecific.psImageWidget = MemAlloc(sizeof(*psWidget->uWidgetSpecific.psImageWidget));
	if (NULL == psWidget->uWidgetSpecific.psImageWidget)
	{
		return(LERR_NO_MEM);
	}
	else
	{
		psWidget->uWidgetSpecific.psImageWidget->eImageHandle = (IMAGEHANDLE) eHandle;
		psWidget->uWidgetSpecific.psImageWidget->psWidget = psWidget;
		return(LERR_OK);
	}
}

static ELCDErr ImageWidgetFree(SWidget *psWidget)
{
	SImageWidget *psImageWidget;

	GCASSERT(psWidget);
	GCASSERT(psWidget->uWidgetSpecific.psImageWidget);

	psImageWidget = psWidget->uWidgetSpecific.psImageWidget;
	GCASSERT(psImageWidget);

	// Now go delete the image group
	if (psImageWidget->psImageGroup)
	{
		GfxDeleteImageGroup(psImageWidget->psImageGroup);
	}

	psImageWidget->psImageGroup = NULL;
	psImageWidget->eImageHandle = HANDLE_INVALID;
	GCFreeMemory(psImageWidget);
	psWidget->uWidgetSpecific.psImageWidget = NULL;
	return(LERR_OK);
}

static void ImageWidgetPaint(SWidget *psWidget,
							 BOOL bLock)
{
	SImageWidget *psImageWidget = psWidget->uWidgetSpecific.psImageWidget;
	SWindow *psWindow;
	
	GCASSERT(psImageWidget);
	psWindow = WindowGetPointer(psWidget->eParentWindow);
	GCASSERT(psWindow);

	// Blit the image
	GfxBlitImageToImage(psWindow->psWindowImage,
						psImageWidget->psImageGroup->psCurrentImage,
					    psImageWidget->psWidget->s32XPos + psWindow->u32ActiveAreaXPos,
					    psImageWidget->psWidget->s32YPos + psWindow->u32ActiveAreaYPos,
						psWindow->u32ActiveAreaXPos + psWindow->u32ActiveAreaXSize,
						psWindow->u32ActiveAreaYPos + psWindow->u32ActiveAreaYSize);

	// Update the region
	WindowUpdateRegion(psImageWidget->psWidget->eParentWindow,
					   psImageWidget->psWidget->s32XPos,
					   psImageWidget->psWidget->s32YPos,
					   psImageWidget->psWidget->u32XSize,
					   psImageWidget->psWidget->u32YSize);
}

static void ImageWidgetErase(SWidget *psWidget)
{
	WidgetEraseStandard(psWidget);
}

static void ImageWidgetAnimationTick(SWidget *psWidget,
									 UINT32 u32TickTime)
{
	ELCDErr eErr;
	SImageWidget *psImage = psWidget->uWidgetSpecific.psImageWidget;
	BOOL bChanged = FALSE;

	eErr = WindowLockByHandle(psWidget->eParentWindow);
	if (eErr != LERR_OK)
	{
		// Just get it next time if we can't get the lock
		return;
	}

	// Do a tick advancement
	bChanged = GfxAnimAdvance(psImage->psImageGroup,
							  u32TickTime);
	if (FALSE == bChanged)
	{
		// Don't do anything - the image hasn't changed yet
		(void) WindowUnlockByHandle(psWidget->eParentWindow);
		return;
	}

	// Erase the old image
	ImageWidgetErase(psWidget);

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

static SWidgetFunctions sg_sImageWidgetFunctions =
{
	NULL,						// Widget region test
	ImageWidgetPaint,			// Paint our widget
	ImageWidgetErase,			// Erase our widget
	NULL,						// Press our widget
	NULL,						// Release our widget
	NULL,						// Mouseover our widget
	NULL,						// Focus
	NULL,						// Keystroke for us
	ImageWidgetAnimationTick,	// Animation widget!
	NULL,						// Calc intersection
	NULL,						// Mouse wheel
	NULL						// Set disable
};

static SWidgetTypeMethods sg_sImageMethods = 
{
	&sg_sImageWidgetFunctions,
	LERR_IMAGE_BAD_HANDLE,			// Error when it's not the handle we're looking for
	ImageWidgetAlloc,
	ImageWidgetFree
};

void ImageWidgetFirstTimeInit(void)
{
	// Nothing needed for first time init for the image widget subsystem
	DebugOut("* Initializing image widget\n");
	WidgetRegisterTypeMethods(WIDGET_IMAGE,
							  &sg_sImageMethods);

}

ELCDErr ImageWidgetCreate(WINDOWHANDLE eWindowHandle,
						  IMAGEHANDLE *peImageWidgetHandle,
						  LEX_CHAR *peFilename,
						  INT32 s32XPos,
						  INT32 s32YPos,
						  EImageWidgetDrawType eDrawType)
{
	ELCDErr eLCDErr;
	SWidget *psWidget = NULL;
	SImageWidget *psImageWidget = NULL;
	EGCResultCode eResultCode;

	eLCDErr = WidgetAllocateHandle((WIDGETHANDLE *) peImageWidgetHandle,
								   WIDGET_IMAGE,
								   eWindowHandle,
								   &psWidget);

	RETURN_ON_FAIL(eLCDErr);

	psImageWidget = psWidget->uWidgetSpecific.psImageWidget;

	// Now go do a load!
	psImageWidget->psImageGroup = GfxLoadImageGroup(peFilename,
													&eResultCode);

	if ((eResultCode != GC_OK) &&
		(eResultCode != GC_PARTIAL_IMAGE))
	{
		GCASSERT(NULL == psImageWidget->psImageGroup);
		eLCDErr = (ELCDErr) (eResultCode + LERR_GC_ERR_BASE);
		goto notAllocated;
	}


	// It's invisible to start with
	psWidget->bWidgetHidden = TRUE;

	// Now set the coordinates
	psWidget->s32XPos = s32XPos;
	psWidget->s32YPos = s32YPos;

	// Now the image size
	psWidget->u32XSize = psImageWidget->psImageGroup->psCurrentImage->u32XSize;
	psWidget->u32YSize = psImageWidget->psImageGroup->psCurrentImage->u32YSize;

	// Set the animation tick rate
	GfxAnimSetTickRate(psImageWidget->psImageGroup,
					   WidgetGetAnimationStepTime());

	// Now set the interval - 1:1
	GfxAnimSetPlaybackSpeed(psImageWidget->psImageGroup,
							0x10000);

	// Set the animation type - stopped to start with
	GfxAnimSetActiveType(psImageWidget->psImageGroup,
						 EDIR_FORWARD);

	// Add it to the list of managed images if there's > 1 frame
	if (psImageWidget->psImageGroup->u32FrameCount > 1)
	{
		eLCDErr = WindowAnimationListAdd(eWindowHandle,
										 psWidget);
		if (eLCDErr != LERR_OK)
		{
			goto notAllocated;
		}
	}

	GfxIncRef(psImageWidget->psImageGroup);

	// Make sure the widget is enabled
	psWidget->bWidgetEnabled = TRUE;

	return(LERR_OK);

notAllocated:
	if (psWidget)
	{
		WidgetDelete(psWidget);
	}

	return(eLCDErr);
}

static ELCDErr ImageWidgetPrepAnimation(IMAGEHANDLE eImageWidgetHandle,
										SImageWidget **ppsImageWidget)
{
	ELCDErr eErr;
	SWidget *psWidget;

	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eImageWidgetHandle,
									WIDGET_IMAGE,
									&psWidget,
									NULL);

	RETURN_ON_FAIL(eErr);

	*ppsImageWidget = psWidget->uWidgetSpecific.psImageWidget;

	// Got the widget.
	if ((*ppsImageWidget)->psImageGroup->u32FrameCount < 2)
	{
		return(LERR_IMAGE_NOT_ANIMATABLE);
	}

	return(LERR_OK);
}

ELCDErr ImageWidgetAnimateStart(IMAGEHANDLE eImageWidgetHandle)
{
	ELCDErr eErr;
	SImageWidget *psImageWidget;

	eErr = ImageWidgetPrepAnimation(eImageWidgetHandle,
									&psImageWidget);
	if (LERR_OK == eErr)
	{
		GfxAnimStart(psImageWidget->psImageGroup);
	}

	return(eErr);
}

ELCDErr ImageWidgetAnimateStop(IMAGEHANDLE eImageWidgetHandle)
{
	ELCDErr eErr;
	SImageWidget *psImageWidget;

	eErr = ImageWidgetPrepAnimation(eImageWidgetHandle,
									&psImageWidget);
	if (LERR_OK == eErr)
	{
		GfxAnimStop(psImageWidget->psImageGroup);
	}

	return(eErr);
}

ELCDErr ImageWidgetAnimateStep(IMAGEHANDLE eImageWidgetHandle)
{
	ELCDErr eErr;
	SImageWidget *psImageWidget;

	eErr = ImageWidgetPrepAnimation(eImageWidgetHandle,
									&psImageWidget);
	if (LERR_OK == eErr)
	{
		GfxAnimStep(psImageWidget->psImageGroup);
	}

	return(eErr);
}

ELCDErr ImageWidgetAnimateReset(IMAGEHANDLE eImageWidgetHandle)
{
	ELCDErr eErr;
	SImageWidget *psImageWidget;

	eErr = ImageWidgetPrepAnimation(eImageWidgetHandle,
									&psImageWidget);
	if (LERR_OK == eErr)
	{
		GfxAnimReset(psImageWidget->psImageGroup);
	}

	return(eErr);
}

ELCDErr ImageRotate(IMAGEHANDLE eImageWidgetHandle,
					UINT16 u16Orientation)
{
	SImageWidget *psImageWidget;
	ELCDErr eErr;
	EGCResultCode eGCResult;
	SWidget *psWidget;

	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eImageWidgetHandle,
									WIDGET_IMAGE,
									&psWidget,
									NULL);

	RETURN_ON_FAIL(eErr);

	psImageWidget = psWidget->uWidgetSpecific.psImageWidget;
	GCASSERT(psImageWidget);

	eErr = WindowLockByHandle(psImageWidget->psWidget->eParentWindow);
	if (eErr != LERR_OK)
	{
		goto errorExit;
	}

	// Now we rotate
	eGCResult = GfxRotateImageGroup(psImageWidget->psImageGroup,
								    (ERotation) u16Orientation);

	if (LERR_OK == eErr)
	{
		eErr = WidgetSetSize(psImageWidget->psWidget,
							 psImageWidget->psImageGroup->psCurrentImage->u32XSize,
							 psImageWidget->psImageGroup->psCurrentImage->u32YSize,
							 FALSE,
							 TRUE,
							 TRUE);
	}

	if (LERR_OK == eErr)
	{
		eErr = WindowUnlockByHandle(psImageWidget->psWidget->eParentWindow);
	}
	else
	{
		(void) WindowUnlockByHandle(psImageWidget->psWidget->eParentWindow);
	}

	// Commit it!
	WindowUpdateRegionCommit();

errorExit:
	return(eErr);
}

ELCDErr ImageGetInfo(IMAGEHANDLE eImageWidgetHandle,
					 UINT32 *pu32ImageXSize,
					 UINT32 *pu32ImageYSize,
					 UINT32 *pu32BitsPerPixel,
					 BOOL *pbTranslucentTransparent,
					 UINT32 *pu32TotalFrames,
					 UINT32 *pu32CurrentFrame)
{
	SImageGroup *psGroup;
	SWidget *psWidget;
	ELCDErr eErr;

	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eImageWidgetHandle,
									WIDGET_IMAGE,
									&psWidget,
									NULL);

	RETURN_ON_FAIL(eErr);

	psGroup = psWidget->uWidgetSpecific.psImageWidget->psImageGroup;
	GCASSERT(psGroup);

	if (pu32ImageXSize)
	{
		*pu32ImageXSize = psGroup->psCurrentImage->u32XSize;
	}

	if (pu32ImageYSize)
	{
		*pu32ImageYSize = psGroup->psCurrentImage->u32YSize;
	}

	if (pu32BitsPerPixel)
	{
		if (psGroup->psCurrentImage->pu8ImageData)
		{
			*pu32BitsPerPixel = 8;
		}
		else
		{
			*pu32BitsPerPixel = 16;
		}
	}

	if (pbTranslucentTransparent)
	{
		if ((psGroup->psCurrentImage->pu8TranslucentMask) ||
			(psGroup->psCurrentImage->pu8Transparent) ||
			(psGroup->psCurrentImage->pu8ImageData))
		{
			*pbTranslucentTransparent = TRUE;
		}
		else
		{
			*pbTranslucentTransparent = FALSE;
		}
	}

	if (pu32TotalFrames)
	{
		*pu32TotalFrames = psGroup->u32FrameCount;
	}

	if (pu32CurrentFrame)
	{
		*pu32CurrentFrame = psGroup->psCurrentImage->u32FrameNumber;
	}

	return(LERR_OK);
}