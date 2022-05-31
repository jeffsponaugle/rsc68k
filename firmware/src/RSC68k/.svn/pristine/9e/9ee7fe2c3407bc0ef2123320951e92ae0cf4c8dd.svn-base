#include "Startup/app.h"
#include "Libs/window/window.h"
#include "Libs/widget/widget.h"

static ELCDErr TouchWidgetAlloc(SWidget *psWidget,
								WIDGETHANDLE eHandle)
{
	GCASSERT(psWidget);
	psWidget->uWidgetSpecific.psTouch = MemAlloc(sizeof(*psWidget->uWidgetSpecific.psTouch));
	if (NULL == psWidget->uWidgetSpecific.psTouch)
	{
		return(LERR_NO_MEM);
	}
	else
	{
		return(LERR_OK);
	}
}

static ELCDErr TouchWidgetFree(SWidget *psWidget)
{
	GCASSERT(psWidget);

	if (NULL == psWidget->uWidgetSpecific.psTouch)
	{
		return(LERR_OK);
	}
	
	GCFreeMemory(psWidget->uWidgetSpecific.psTouch);
	psWidget->uWidgetSpecific.psTouch = NULL;
	return(LERR_OK);
}

static BOOL TouchHitTest(SWidget *psWidget,
						 UINT32 u32XPos, 
						 UINT32 u32YPos)
{
	// Always accept the touch region hit since it's always valid
	return(TRUE);
}

static void TouchPress(SWidget *psWidget,
					   UINT32 u32Mask,
					   UINT32 u32XPos,
					   UINT32 u32YPos)
{
	GCASSERT(psWidget->uWidgetSpecific.psTouch);
	psWidget->uWidgetSpecific.psTouch->bTouched = TRUE;
}

static void TouchRelease(struct SWidget *psWidget,
						 UINT32 u32Mask,
						 UINT32 u32XPos,
						 UINT32 u32YPos)
{
	GCASSERT(psWidget->uWidgetSpecific.psTouch);
	psWidget->uWidgetSpecific.psTouch->bTouched = FALSE;
}

static SWidgetFunctions sg_sTouchFunctions =
{
	TouchHitTest,
	NULL,
	NULL,
	TouchPress,
	TouchRelease,
	NULL,						// Mouseover our widget
	NULL,						// Focus
	NULL,						// Keystroke for us
	NULL,						// Animation tick - none for now!
	NULL,						// Calc intersection
	NULL,						// Mouse wheel
	NULL						// Set disable
};

static SWidgetTypeMethods sg_sTouchMethods = 
{
	&sg_sTouchFunctions,
	LERR_TOUCH_BAD_HANDLE,			// Error when it's not the handle we're looking for
	TouchWidgetAlloc,
	TouchWidgetFree
};

void TouchRegionInit(void)
{
	DebugOut("* Initializing touch widget\n");

	// Register it with the widget manager
	WidgetRegisterTypeMethods(WIDGET_TOUCH,
							  &sg_sTouchMethods);
}

ELCDErr TouchRegionCreate(TOUCHREGIONHANDLE *peTouchRegionHandle,
						  WINDOWHANDLE eWindowHandle,
						  INT32 s32XPos,
						  INT32 s32YPos,
						  UINT32 u32XSize,
						  UINT32 u32YSize)
{
	ELCDErr eLCDErr;
	SWidget *psWidget = NULL;

	eLCDErr = WidgetAllocateHandle((WIDGETHANDLE *) peTouchRegionHandle,
								   WIDGET_TOUCH,
								   eWindowHandle,
								   &psWidget);

	RETURN_ON_FAIL(eLCDErr);

	psWidget->bIgnoreIntersections = TRUE;

	// Make sure the widget is enabled
	psWidget->bWidgetEnabled = TRUE;

	eLCDErr = WidgetSetSize(psWidget,
							u32XSize,
							u32YSize,
							FALSE,
							FALSE,
							FALSE);
	RETURN_ON_FAIL(eLCDErr);

	eLCDErr = WidgetSetPositionByHandle((WIDGETHANDLE) *peTouchRegionHandle,
										WIDGET_TOUCH,
										s32XPos,
										s32YPos);

	return(eLCDErr);
}


ELCDErr TouchRegionSetSize(TOUCHREGIONHANDLE eTouchRegionHandle,
								 INT32 s32XPos,
								 INT32 s32YPos,
								 UINT32 u32XSize,
								 UINT32 u32YSize)
{
	ELCDErr eLCDErr;
	SWidget *psWidget;

	eLCDErr = WidgetGetPointerByHandle((WIDGETHANDLE) eTouchRegionHandle,
									WIDGET_TOUCH,
									&psWidget,
									NULL);

	RETURN_ON_FAIL(eLCDErr);

	eLCDErr = WidgetSetSize(psWidget,
							u32XSize,
							u32YSize,
							FALSE,
							FALSE,
							FALSE);
	RETURN_ON_FAIL(eLCDErr);

	eLCDErr = WidgetSetPositionByHandle((WIDGETHANDLE) eTouchRegionHandle,
										WIDGET_TOUCH,
										s32XPos,
										s32YPos);
	return(eLCDErr);
}


ELCDErr TouchRegionGetInfo(TOUCHREGIONHANDLE eTouchRegionHandle,
						   BOOL *pbWasPressed,
						   BOOL *pbCurrentlyPressed)
{
	ELCDErr eErr;
	SWidget *psWidget;

	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eTouchRegionHandle,
									WIDGET_TOUCH,
									&psWidget,
									NULL);

	RETURN_ON_FAIL(eErr);

	if (pbWasPressed)
	{
		*pbWasPressed = psWidget->uWidgetSpecific.psTouch->bTouched;
		psWidget->uWidgetSpecific.psTouch->bTouched = FALSE;
	}

	if (pbCurrentlyPressed)
	{
		*pbCurrentlyPressed = psWidget->bCurrentlyPressed;
	}

	return(eErr);
}

