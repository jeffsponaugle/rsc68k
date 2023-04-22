#include "Startup/app.h"
#include "Libs/Gfx/GraphicsLib.h"
#include "Libs/window/window.h"
#include "Libs/FontMgr/FontMgr.h"
#include "Libs/widget/widget.h"
#include "Libs/widget/graph/graph.h"

#define MAX_GRAPHS			64
#define MAX_GRAPH_SERIES	128

static SGraphSeries *sg_psGraphSeriesList[MAX_GRAPH_SERIES];

static SGraphWidget *sg_psGraphWidgetHead = NULL;

static volatile BOOL sg_bEmergencyUpdate = FALSE;

// Timer object for deferred graph updates
static STimerObject *sg_psGraphTimer;

static ELCDErr GraphWidgetAlloc(SWidget *psWidget,
								WIDGETHANDLE eHandle)
{
	GCASSERT(psWidget);
	psWidget->uWidgetSpecific.psGraphWidget = MemAlloc(sizeof(*psWidget->uWidgetSpecific.psGraphWidget));
	if (NULL == psWidget->uWidgetSpecific.psGraphWidget)
	{
		return(LERR_NO_MEM);
	}
	else
	{
		psWidget->uWidgetSpecific.psGraphWidget->eGraphHandle = (GRAPHHANDLE) eHandle;
		psWidget->uWidgetSpecific.psGraphWidget->psWidget = psWidget;
		return(LERR_OK);
	}
}

static void GraphTextDestroyChain(SGfxElement *psElement,
								  SGraphSeries *psGraphSeries)
{
	SGfxElement *psPriorElement = NULL;

	while (psElement)
	{
		psPriorElement = psElement;
		psElement = psElement->psNextLink;

		if (psPriorElement->psFuncs->ElemFree)
		{
			psPriorElement->psFuncs->ElemFree(psGraphSeries,
											  psPriorElement);
		}

		GCFreeMemory(psPriorElement);
	}
}

static void GraphTextDestroyInternal(SGraphWidget *psGraphWidget)
{
	SGfxElement *psElement = NULL;
	
	// Delete the text annotation layer
	GraphTextDestroyChain(psGraphWidget->psTextAnnotation,
						  NULL);
	psGraphWidget->psTextAnnotation = NULL;
}

static void GraphSeriesDelete(SGraphSeries *psGraphSeries,
							 SGraphWidget *psGraphWidget)
{
	SGraphSeries *psGraphSeriesPtr = NULL;
	SGraphSeries *psGraphSeriesPrior = NULL;
	SGfxElement *psElement = NULL;
	UINT32 u32Index = 0;
	UINT32 u32Remaining = 0;

	// It's locked and we have a valid series handle. Let's first find this in
	// the list of active series and eliminate it from the list.
	psGraphSeriesPtr = psGraphWidget->psUserSeriess;

	while (psGraphSeriesPtr && psGraphSeriesPtr != psGraphSeries)
	{
		psGraphSeriesPrior = psGraphSeriesPtr;
		psGraphSeriesPtr = psGraphSeriesPtr->psNextLink;
	}

	// This had better not be NULL. If it asserts, it means the graph series
	// is in the 
	GCASSERT(psGraphSeriesPtr);

	if (psGraphSeriesPtr == psGraphWidget->psUserSeriess)
	{
		psGraphWidget->psUserSeriess = psGraphSeriesPtr->psNextLink;
	}
	else
	{
		psGraphSeriesPrior->psNextLink = psGraphSeriesPtr->psNextLink;
	}

	// Okay - the series has been unlinked! Let's free the individual series data
	u32Remaining = psGraphSeries->u32SeriesCount;
	u32Index = psGraphSeries->u32Tail;
	psElement = &psGraphSeries->psElements[u32Index];

	while (u32Remaining)
	{
		if (psElement->psFuncs->ElemFree)
		{
			psElement->psFuncs->ElemFree(psGraphSeries,
										 psElement);
		}
		u32Index++;
		if (u32Index >= psGraphSeries->u32SeriesElementSize)
		{
			u32Index = 0;
			psElement = psGraphSeries->psElements;
		}
		else
		{
			++psElement;
		}

		u32Remaining--;
	}

	// Get rid of the text annotation layer
	GraphTextDestroyInternal(psGraphWidget);

	if (psGraphSeries->pvSeriesData)
	{
		GCFreeMemory(psGraphSeries->pvSeriesData);
		psGraphSeries->pvSeriesData = NULL;
	}

	if (psGraphSeries->psSeriesImage)
	{
		GfxDeleteImageGroup(psGraphSeries->psSeriesImage);
		psGraphSeries->psSeriesImage = NULL;
	}

	// Free up the series handle now
	sg_psGraphSeriesList[psGraphSeries->eGraphSeriesHandle] = NULL;

	// Now free the user data
	GCFreeMemory(psGraphSeries->psElements);
	psGraphSeries->psElements = NULL;
	GCFreeMemory(psGraphSeries);
}

static ELCDErr GraphWidgetFree(SWidget *psWidget)
{
	SGraphWidget *psGraphWidget;
	SGraphWidget *psGraphWidgetPtr = NULL;
	SGraphWidget *psGraphWidgetPrior = NULL;

	GCASSERT(psWidget);
	GCASSERT(psWidget->uWidgetSpecific.psGraphWidget);
	psGraphWidget = psWidget->uWidgetSpecific.psGraphWidget;

	// Take ourselves out of the master list
	psGraphWidgetPtr = sg_psGraphWidgetHead;

	// Take ourselves out of the master graph widget linked list
	while (psGraphWidgetPtr && psGraphWidgetPtr != psGraphWidget)
	{
		psGraphWidgetPrior = psGraphWidgetPtr;
		psGraphWidgetPtr = psGraphWidgetPtr->psNextLink;
	}

	GCASSERT(psGraphWidgetPtr);
	GCASSERT(psGraphWidgetPtr == psGraphWidget);

	if (NULL == psGraphWidgetPrior)
	{
		sg_psGraphWidgetHead = psGraphWidgetPtr->psNextLink;
	}
	else
	{
		psGraphWidgetPrior->psNextLink = psGraphWidgetPtr->psNextLink;
	}

	// Just need to whip through all of the series and delete 'em!

	GCASSERT(psGraphWidget);

	while (psGraphWidget->psUserSeriess)
	{
//		DebugOut("%s: Series delete 0x%.8x\n", __FUNCTION__, psGraphWidget->psUserSeriess->eGraphSeriesHandle);
		GraphSeriesDelete(psGraphWidget->psUserSeriess,
						 psGraphWidget);
	}

	psGraphWidget->eGraphHandle = HANDLE_INVALID;
	GCFreeMemory(psGraphWidget);
	psWidget->uWidgetSpecific.psGraphWidget = NULL;
	return(LERR_OK);
}

static BOOL GraphSeriesFindFreeHandle(GRAPHSERIESHANDLE *peGraphSeriesHandle)
{
	UINT32 u32Loop;

	for (u32Loop = 0; u32Loop < (sizeof(sg_psGraphSeriesList) / sizeof(sg_psGraphSeriesList[0])); u32Loop++)
	{
		if (NULL == sg_psGraphSeriesList[u32Loop])
		{
			*peGraphSeriesHandle = (GRAPHHANDLE) u32Loop;
			return(TRUE);
		}
	}

	return(FALSE);
}

SGraphSeries *GraphSeriesGetPointer(GRAPHHANDLE eHandle)
{
	if (eHandle >= (sizeof(sg_psGraphSeriesList) / sizeof(sg_psGraphSeriesList[0])))
	{
		return(NULL);
	}

	return(sg_psGraphSeriesList[eHandle]);
}


static void GraphRecalcWidget(SGraphWidget *psGraphWidget,
							  BOOL bLock)
{
	SGraphSeries *psSeries;
	INT32 s32XLow = 99999999;
	INT32 s32XHigh = -99999999;
	INT32 s32YLow = 99999999;
	INT32 s32YHigh = -99999999;
	UINT32 u32XSize = 0;
	UINT32 u32YSize = 0;
	BOOL bChanged = FALSE;
	SGfxElement *psElement;

	// First thing, if we have a background image, let's look at its coordinates
	if (psGraphWidget->psBackgroundImage)
	{
		INT32 s32XLeft = psGraphWidget->psWidget->s32XPos + (INT32) psGraphWidget->u32BackgroundImageOffsetX;
		INT32 s32YTop = psGraphWidget->psWidget->s32YPos + (INT32) psGraphWidget->u32BackgroundImageOffsetY;
		INT32 s32XRight = 0;
		INT32 s32YBottom = 0;

		s32XRight = s32XLeft + (INT32) psGraphWidget->psBackgroundImage->psCurrentImage->u32XSize;
		s32YBottom = s32YTop + (INT32) psGraphWidget->psBackgroundImage->psCurrentImage->u32YSize;

		if (s32XLeft < s32XLow)
		{
			s32XLow = s32XLeft;
		}
		if (s32XRight > s32XHigh)
		{
			s32XHigh = s32XRight;
		}

		if (s32YTop < s32YLow)
		{
			s32YLow = s32YTop;
		}
		if (s32YBottom > s32YHigh)
		{
			s32YHigh = s32YBottom;
		}
	}

	// Now let's whip through all of our series and look for the coordinate extremes

	psSeries = psGraphWidget->psUserSeriess;

	while (psSeries)
	{
		UINT32 u32CountRemaining;
		UINT32 u32Index = psSeries->u32Tail;

		psElement = &psSeries->psElements[u32Index];
		u32CountRemaining = psSeries->u32SeriesCount;

		// If this series isn't visible, don't count it
		if (FALSE == psSeries->bVisible)
		{
			// This will cause the whlie () below to fall through
			u32CountRemaining = 0;
		}

		while (u32CountRemaining--)
		{
			// Check our coordinates to see if they are bigger/smaller than we expect
			ElementCalculateBounds(psElement,
								   &s32XLow,
								   &s32XHigh,
								   &s32YLow,
								   &s32YHigh);

			// Advance to the next element
			++psElement;
			u32Index++;
			if (u32Index >= psSeries->u32SeriesElementSize)
			{
				u32Index = 0;
				psElement = psSeries->psElements;
			}
		}

		// Look at series text stuff, too.
		psElement = psSeries->psSeriesTextElements;

		while (psElement)
		{
			ElementCalculateBounds(psElement,
								   &s32XLow,
								   &s32XHigh,
								   &s32YLow,
								   &s32YHigh);
			psElement = psElement->psNextLink;
		}

		psSeries = psSeries->psNextLink;
	}

	// Include the grid (if applicable)
	if ((psGraphWidget->sGrid.u32XSize) &&
		(psGraphWidget->sGrid.u32YSize))
	{
		INT32 s32XPos;
		INT32 s32YPos;

		s32XPos = psGraphWidget->psWidget->s32XPos + psGraphWidget->sGrid.u32XOffset;
		if (s32XPos < s32XLow)
		{
			s32XLow = s32XPos;
		}

		s32YPos = psGraphWidget->psWidget->s32YPos + (INT32) psGraphWidget->sGrid.u32YOffset;
		if (s32YPos < s32YLow)
		{
			s32YLow = s32YPos;
		}

		s32XPos += (INT32) psGraphWidget->sGrid.u32XSize;
		s32YPos += (INT32) psGraphWidget->sGrid.u32YSize;
		if (s32XPos > s32XHigh)
		{
			s32XHigh = s32XPos;
		}
		if (s32YPos > s32YHigh)
		{
			s32YHigh = s32YPos;
		}
	}

	ElementCalculateBounds(psGraphWidget->psTextAnnotation,
						   &s32XLow,
						   &s32XHigh,
						   &s32YLow,
						   &s32YHigh);

	// Let's check out our new coordinates. 
	if (s32XLow != 99999999)
	{
		// Means that we calculated something real.

		u32XSize = (UINT32) (s32XHigh);
		u32YSize = (UINT32) (s32YHigh);
	}

	// If our new size != old size, then we need to go through some gyrations
	// to set the widget up properly

	if ((u32XSize != psGraphWidget->psWidget->u32XSize) ||
		(u32YSize != psGraphWidget->psWidget->u32YSize))
	{
		// Size change. Let's erase the old widget size.
		WidgetErase(psGraphWidget->psWidget);

		// Set the new size
		WidgetSetSize(psGraphWidget->psWidget,
					  u32XSize,
					  u32YSize,
					  bLock,
					  FALSE,
					  FALSE);
	}
}

static void GraphDrawGrid(SGraphWidget *psGraphWidget,
						  SWindow *psWindow)
{
	UINT32 u32YCount;
	UINT32 u32XCount;
	UINT32 u32XStep = 0;
	UINT32 u32YStep = 0;
	UINT32 u32XPos = 0;
	UINT32 u32YPos = 0;

	if ((0 == psGraphWidget->sGrid.u32XSize) ||
		(0 == psGraphWidget->sGrid.u32YSize))
	{
		// Nothing to do. Just return.
		return;
	}

	// Try vertical first. Need to compute X.

	if (psGraphWidget->sGrid.u8VerticalThickness > psGraphWidget->sGrid.u32XSize)
	{
		// Whoops - can't draw anything
	}
	else
	{
		u32XPos = 0;
		u32XCount = psGraphWidget->sGrid.u32VerticalCount;
		if (u32XCount >= 2)
		{
			u32XStep = ((psGraphWidget->sGrid.u32XSize - psGraphWidget->sGrid.u8VerticalThickness) << 16) / (u32XCount - 1);
		}
		else
		{
			u32XStep = 0;
		}

		if (0 == (GRID_LEFT & psGraphWidget->sGrid.u8BoundMask))
		{
			if (u32XCount)
			{
				u32XCount--;
			}

			u32XPos += u32XStep;
		}

		if (0 == (GRID_RIGHT & psGraphWidget->sGrid.u8BoundMask))
		{
			if (u32XCount)
			{
				u32XCount--;
			}
		}

		u32XPos += (psGraphWidget->sGrid.u32XOffset << 16);
		u32YPos = (psGraphWidget->sGrid.u32YOffset) + psGraphWidget->psWidget->s32YPos;

		while (u32XCount)
		{
			BarDrawRectangleWithImage(psWindow,
									  psGraphWidget->sGrid.u8VerticalTranslucency,
		  							  (UINT16) CONVERT_24RGB_16RGB(psGraphWidget->sGrid.u32VerticalColor),
									  (INT32) ((INT32) (u32XPos >> 16) + psGraphWidget->psWidget->s32XPos),
									  (INT32) u32YPos,
									  psGraphWidget->sGrid.u8VerticalThickness,
									  psGraphWidget->sGrid.u32YSize,
									  NULL);
			u32XPos += u32XStep;
			u32XCount--;
		}
	}

	// Now horizontal

	if (psGraphWidget->sGrid.u8HorizontalThickness > psGraphWidget->sGrid.u32YSize)
	{
		// Whoops - can't draw anything
	}
	else
	{
		u32YCount = psGraphWidget->sGrid.u32HorizontalCount;
		if (u32YCount >= 2)
		{
			u32YStep = ((psGraphWidget->sGrid.u32YSize - psGraphWidget->sGrid.u8HorizontalThickness) << 16) /  (u32YCount - 1);
		}
		else
		{
			u32YStep = 0;
		}

		u32YPos = 0;

		if (0 == (GRID_TOP & psGraphWidget->sGrid.u8BoundMask))
		{
			if (u32YCount)
			{
				u32YCount--;
			}

			u32YPos += u32YStep;
		}

		if (0 == (GRID_BOTTOM & psGraphWidget->sGrid.u8BoundMask))
		{
			if (u32YCount)
			{
				u32YCount--;
			}
		}

		u32YPos += (psGraphWidget->sGrid.u32YOffset << 16) + psGraphWidget->psWidget->s32YPos;
		u32XPos = (psGraphWidget->sGrid.u32XOffset);

		while (u32YCount)
		{
			BarDrawRectangleWithImage(psWindow,
									  psGraphWidget->sGrid.u8HorizontalTranslucency,
		  							  (UINT16) CONVERT_24RGB_16RGB(psGraphWidget->sGrid.u32HorizontalColor),
									  (INT32) u32XPos,
									  (INT32) (u32YPos >> 16),
									  psGraphWidget->sGrid.u32XSize,
									  psGraphWidget->sGrid.u8HorizontalThickness,
									  NULL);
			u32YPos += u32YStep;
			u32YCount--;
		}
	}
}

static void GraphWidgetPaint(SWidget *psWidget,
							 BOOL bLock)
{
	SWindow *psWindow;
	SGraphWidget *psGraphWidget;
	SGraphSeries *psGraphSeries;
	SGfxElement *psElement;

	// Paint!
	psGraphWidget = psWidget->uWidgetSpecific.psGraphWidget;

	if (psGraphWidget->bUpdateInProgress)
	{
		return;
	}

	psWindow = WindowGetPointer(psWidget->eParentWindow);
	GCASSERT(psWindow);

	// If we need to do a recalc, do so
	if (psGraphWidget->bRecalcNeeded)
	{
		GraphRecalcWidget(psGraphWidget,
						  bLock);
		psGraphWidget->bRecalcNeeded = FALSE;
	}

	// Paint the background image (if applicable)
	if (psGraphWidget->psBackgroundImage)
	{
		GfxBlitImageToImage(psWindow->psWindowImage,
							psGraphWidget->psBackgroundImage->psCurrentImage,
							psGraphWidget->psWidget->s32XPos + (INT32) psGraphWidget->u32BackgroundImageOffsetX + (INT32) psWindow->u32ActiveAreaXPos,
							psGraphWidget->psWidget->s32YPos + (INT32) psGraphWidget->u32BackgroundImageOffsetY + (INT32) psWindow->u32ActiveAreaYPos,
							psWindow->u32ActiveAreaXPos + psWindow->u32ActiveAreaXSize,
							psWindow->u32ActiveAreaYPos + psWindow->u32ActiveAreaYSize);
	}

	// Now do the grid
	GraphDrawGrid(psGraphWidget,
				  psWindow);
	
	// Paint the element layer
	psGraphSeries = psGraphWidget->psUserSeriess;

	while (psGraphSeries)
	{
		SGfxElement *psElement;
		SGfxElement *psPriorElement;
		UINT32 u32Index;
		UINT32 u32CountRemaining = 0;

		u32Index = psGraphSeries->u32Tail;
		u32CountRemaining = psGraphSeries->u32SeriesCount;
		psPriorElement = NULL;

		if (psGraphSeries->bVisible)
		{
			while (u32CountRemaining--)
			{
				psElement = &psGraphSeries->psElements[u32Index];
				u32Index++;
				if (u32Index >= psGraphSeries->u32SeriesElementSize)
				{
					u32Index = 0;
				}

				// Go draw the element
				psElement->psFuncs->ElemDraw(psWidget->eParentWindow,
											 psElement,
											 psPriorElement,
											 psGraphSeries);

				psPriorElement = psElement;
			}
		}

		// Now draw any series text

		psElement = psGraphSeries->psSeriesTextElements;
		psPriorElement = NULL;

		while (psElement)
		{
			if (psElement->psFuncs->ElemDraw)
			{
				psElement->psFuncs->ElemDraw(psWidget->eParentWindow,
											 psElement,
											 psPriorElement,
											 psGraphSeries);
			}

			psPriorElement = psElement;
			psElement = psElement->psNextLink;
		}

		psGraphSeries = psGraphSeries->psNextLink;
	}

	// Draw the text annotation layer
	psElement = psGraphWidget->psTextAnnotation;
	while (psElement)
	{
		psElement->psFuncs->ElemDraw(psWidget->eParentWindow,
									 psElement,
									 NULL,
									 NULL);
		psElement = psElement->psNextLink;
	}

	// Update the region
	WindowUpdateRegion(psWidget->eParentWindow,
					   psWidget->s32XPos + (INT32) (psWindow->u32ActiveAreaXPos),
					   psWidget->s32YPos + (INT32) (psWindow->u32ActiveAreaYPos),
					   (INT32) psWidget->u32XSize,
					   (INT32) psWidget->u32YSize);
}

static void GraphWidgetErase(SWidget *psWidget)
{
	WidgetEraseStandard(psWidget);
}


static void GraphWidgetAnimationTick(SWidget *psWidget,
									 UINT32 u32TickTime)
{
/*	ELCDErr eErr;
	SImageWidget *psImage = psWidget->uWidgetSpecific.psImageWidget;
	BOOL bChanged = FALSE;

	eErr = WindowLockByHandle(psWidget->eParentWindow);
	if (eErr != LERR_OK)
	{
		// Just get it next time if we can't get the lock
		return;
	}

	// Do a tick advancement
	bChanged = GfxAnimAdvance(psImage->psImageGroup);
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
	WidgetPaint(psWidget);

	// Unlock the window
	(void) WindowUnlockByHandle(psWidget->eParentWindow);

	// Now commit the changes
	WindowUpdateRegionCommit(); */
}

static SWidgetFunctions sg_sGraphWidgetFunctions =
{
	NULL,						// Widget region test
	GraphWidgetPaint,			// Paint our widget
	GraphWidgetErase,			// Erase our widget
	NULL,						// Press our widget
	NULL,						// Release our widget
	NULL,						// Mouseover
	NULL,						// Focus
	NULL,						// Keystroke for us
	GraphWidgetAnimationTick,	// Animation widget!
	NULL,						// Calc interesction
	NULL,						// Mouse wheel
	NULL						// Set disable
};

// This procedure is called by the windowing code 

void GraphWidgetUpdate(GRAPHHANDLE eGraphHandle)
{
	SGraphWidget *psGraph;
	SWidget *psWidget;
	ELCDErr eErr;

	// This thread is running in the window update thread context - be careful!
	// It can conflict with foreground operations

	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eGraphHandle,
									WIDGET_GRAPH,
									&psWidget,
									NULL);
	if (eErr != LERR_OK)
	{
		return;
	}

	psGraph = psWidget->uWidgetSpecific.psGraphWidget;
	GCASSERT(psGraph);

	// If widget is hidden, just return. No need to draw.
	if ((psWidget->bWidgetHidden) || (FALSE == psWidget->bWidgetEnabled) || (psWidget->uWidgetSpecific.psGraphWidget->bUpdateInProgress))
	{
		sg_bEmergencyUpdate = FALSE;
		return;
	}

	eErr = WindowLockByHandle(psWidget->eParentWindow);
	GCASSERT(LERR_OK == eErr);

	WidgetSetUpdate(TRUE);

	// Erase the widget's old size
	GraphWidgetErase(psWidget);

	// Go draw the widget - already locked
	GraphWidgetPaint(psWidget,
					 FALSE);

	WidgetSetUpdate(FALSE);

	eErr = WindowUnlockByHandle(psWidget->eParentWindow);
	GCASSERT(LERR_OK == eErr);

	// Now commit the changes
	WindowUpdateRegionCommit();

	// Emergency update not needed
	sg_bEmergencyUpdate = FALSE;
}

ELCDErr GraphWidgetCreate(WINDOWHANDLE eWindowHandle,
						  GRAPHHANDLE *peGraphHandle,
						  INT32 s32XPos,
						  INT32 s32YPos,
						  BOOL bVisible)
{
	SWidget *psWidget = NULL;
	SGraphWidget *psGraph = NULL;
	ELCDErr eLCDErr;

	eLCDErr = WidgetAllocateHandle((WIDGETHANDLE *) peGraphHandle,
								   WIDGET_GRAPH,
								   eWindowHandle,
								   &psWidget);

	RETURN_ON_FAIL(eLCDErr);

	// Hook things up
	psWidget->s32XPos = s32XPos;
	psWidget->s32YPos = s32YPos;
	psWidget->bWidgetEnabled = TRUE;

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

	// Insert this graph into the list
	psWidget->uWidgetSpecific.psGraphWidget->psNextLink = sg_psGraphWidgetHead;
	sg_psGraphWidgetHead = psWidget->uWidgetSpecific.psGraphWidget;

	return(LERR_OK);
}

static void SetGraphOrigin(SGraphWidget *psGraphWidget,
						   SGraphSeries *psGraphSeries)
{
	UINT32 u32Loop;
	UINT32 u32Index;

	u32Index = psGraphSeries->u32Tail;

	// Based on our origin, create x/y positions
	if (EORIGIN_TOP == psGraphSeries->eOrigin)
	{
		UINT32 u32XPos = 0;

		psGraphSeries->u32Step = (((psGraphSeries->s32XSize - 1) << 16) / (psGraphSeries->u32SeriesElementSize + 1));
		for (u32Loop = 0; u32Loop < psGraphSeries->u32SeriesElementSize; u32Loop++)
		{
			psGraphSeries->psElements[u32Index].s32XOrigin = (INT32) (u32XPos >> 16) + psGraphSeries->s32XOffset + psGraphWidget->psWidget->s32YPos;
			psGraphSeries->psElements[u32Index].s32YOrigin = psGraphSeries->s32YOffset + psGraphWidget->psWidget->s32YPos;
			u32XPos += psGraphSeries->u32Step;
			u32Index++;
			if (u32Index >= psGraphSeries->u32SeriesElementSize)
			{
				u32Index = 0;
			}
		}
	}
	else
	if (EORIGIN_BOTTOM == psGraphSeries->eOrigin)
	{
		UINT32 u32XPos = 0;

		psGraphSeries->u32Step = (((psGraphSeries->s32XSize - 1) << 16) / (psGraphSeries->u32SeriesElementSize + 1));
		for (u32Loop = 0; u32Loop < psGraphSeries->u32SeriesElementSize; u32Loop++)
		{
			psGraphSeries->psElements[u32Index].s32XOrigin = (INT32) (u32XPos >> 16) + psGraphSeries->s32XOffset + psGraphWidget->psWidget->s32YPos;
			psGraphSeries->psElements[u32Index].s32YOrigin = psGraphSeries->s32YOffset + psGraphWidget->psWidget->s32YPos + (psGraphSeries->s32YSize - 1);
			u32XPos += psGraphSeries->u32Step;
			u32Index++;
			if (u32Index >= psGraphSeries->u32SeriesElementSize)
			{
				u32Index = 0;
			}
		}
	}
	else
	if (EORIGIN_LEFT == psGraphSeries->eOrigin)
	{
		UINT32 u32YPos = 0;

		psGraphSeries->u32Step = (((psGraphSeries->s32YSize - 1) << 16) / (psGraphSeries->u32SeriesElementSize + 1));
		for (u32Loop = 0; u32Loop < psGraphSeries->u32SeriesElementSize; u32Loop++)
		{
			psGraphSeries->psElements[u32Index].s32XOrigin = psGraphSeries->s32XOffset + psGraphWidget->psWidget->s32XPos;
			psGraphSeries->psElements[u32Index].s32YOrigin = (INT32) (u32YPos >> 16) + psGraphSeries->s32YOffset;
			u32YPos += psGraphSeries->u32Step;
			u32Index++;
			if (u32Index >= psGraphSeries->u32SeriesElementSize)
			{
				u32Index = 0;
			}
		}
	}
	else
	if (EORIGIN_RIGHT == psGraphSeries->eOrigin)
	{
		UINT32 u32YPos = 0;

		psGraphSeries->u32Step = (((psGraphSeries->s32YSize - 1) << 16) / (psGraphSeries->u32SeriesElementSize + 1));
		for (u32Loop = 0; u32Loop < psGraphSeries->u32SeriesElementSize; u32Loop++)
		{
			psGraphSeries->psElements[u32Index].s32XOrigin = psGraphSeries->s32XOffset + psGraphWidget->psWidget->s32XPos + (psGraphSeries->s32XSize - 1);
			psGraphSeries->psElements[u32Index].s32YOrigin = (INT32) (u32YPos >> 16) + psGraphSeries->s32YOffset;
			u32YPos += psGraphSeries->u32Step;
			u32Index++;
			if (u32Index >= psGraphSeries->u32SeriesElementSize)
			{
				u32Index = 0;
			}
		}
	}
	else
	{
		// Bad origin
		GCASSERT(0);
	}
}

static ELCDErr CheckMinMax(EVarType eVarType,
						   union UValue *puMinValue,
						   union UValue *puMaxValue)
{
	BOOL bMinMaxFault = FALSE;

	// Check to ensure min is smaller than max
	switch (eVarType)
	{	
		case EVAR_SIGNED_CHAR:
		{
			if (puMinValue->s8Char > puMaxValue->s8Char)
			{
				bMinMaxFault = TRUE;
			}
			break;
		}
		case EVAR_UNSIGNED_CHAR:
		{
			if (puMinValue->u8Char > puMaxValue->u8Char)
			{
				bMinMaxFault = TRUE;
			}
			break;
		}
		case EVAR_SIGNED_SHORT:
		{
			if (puMinValue->s16Short > puMaxValue->s16Short)
			{
				bMinMaxFault = TRUE;
			}
			break;
		}
		case EVAR_UNSIGNED_SHORT:
		{
			if (puMinValue->u16Short > puMaxValue->u16Short)
			{
				bMinMaxFault = TRUE;
			}
			break;
		}
		case EVAR_SIGNED_INT:
		{
			if (puMinValue->s32Int > puMaxValue->s32Int)
			{
				bMinMaxFault = TRUE;
			}
			break;
		}
		case EVAR_UNSIGNED_INT:
		{
			if (puMinValue->u32Int > puMaxValue->u32Int)
			{
				bMinMaxFault = TRUE;
			}
			break;
		}
		case EVAR_SIGNED_LONG_INT:
		{
			if (puMinValue->s64LongInt > puMaxValue->s64LongInt)
			{
				bMinMaxFault = TRUE;
			}
			break;
		}
		case EVAR_UNSIGNED_LONG_INT:
		{
			if (puMinValue->u64LongInt > puMaxValue->u64LongInt)
			{
				bMinMaxFault = TRUE;
			}
			break;
		}
		case EVAR_FLOAT:
		{
			if (puMinValue->fFloat > puMaxValue->fFloat)
			{
				bMinMaxFault = TRUE;
			}
			break;
		}
		case EVAR_DOUBLE:
		{
			if (puMinValue->dDouble > puMaxValue->dDouble)
			{
				bMinMaxFault = TRUE;
			}
			break;
		}
		case EVAR_BOOLEAN:
		{
			// Force min/max to false/true
			if (puMinValue->bBoolean > puMaxValue->bBoolean)
			{
				bMinMaxFault = TRUE;
			}
			break;
		}
		default:
		{
			// Invalid type. Either the parser or the exec didn't do a good enough job
			// on type checking before calling this function.
			GCASSERT(0);
		}
	}

	if (bMinMaxFault)
	{
		return(LERR_GRAPH_MIN_LARGER_THAN_MAX);
	}
	else
	{
		return(LERR_OK);
	}
}


static void SetMinMaxRange(SGraphSeries *psGraphSeries)
{
	switch (psGraphSeries->eVarType)
	{	
		case EVAR_SIGNED_CHAR:
		{
			psGraphSeries->uRange.u64LongInt = (psGraphSeries->uHigh.s8Char - psGraphSeries->uLow.s8Char) + 1;
			break;
		}
		case EVAR_UNSIGNED_CHAR:
		{
			psGraphSeries->uRange.u64LongInt = (psGraphSeries->uHigh.u8Char - psGraphSeries->uLow.u8Char) + 1;
			break;
		}
		case EVAR_SIGNED_SHORT:
		{
			psGraphSeries->uRange.u64LongInt = ((INT32) psGraphSeries->uHigh.s16Short - (INT32) psGraphSeries->uLow.s16Short) + 1;
			break;
		}
		case EVAR_UNSIGNED_SHORT:
		{
			psGraphSeries->uRange.u64LongInt = (psGraphSeries->uHigh.u16Short - psGraphSeries->uLow.u16Short) + 1;
			break;
		}
		case EVAR_SIGNED_INT:
		{
			psGraphSeries->uRange.u64LongInt = (psGraphSeries->uHigh.s32Int - psGraphSeries->uLow.s32Int) + 1;
			break;
		}
		case EVAR_UNSIGNED_INT:
		{
			psGraphSeries->uRange.u64LongInt = (psGraphSeries->uHigh.u32Int - psGraphSeries->uLow.u32Int) + 1;
			break;
		}
		case EVAR_SIGNED_LONG_INT:
		{
			psGraphSeries->uRange.u64LongInt = (psGraphSeries->uHigh.s64LongInt - psGraphSeries->uLow.s64LongInt) + 1;
			break;
		}
		case EVAR_UNSIGNED_LONG_INT:
		{
			psGraphSeries->uRange.u64LongInt = (psGraphSeries->uHigh.u64LongInt - psGraphSeries->uLow.u64LongInt) + 1;
			break;
		}
		case EVAR_FLOAT:
		{
			psGraphSeries->uRange.fFloat = (psGraphSeries->uHigh.fFloat - psGraphSeries->uLow.fFloat) + 1;
			break;
		}
		case EVAR_DOUBLE:
		{
			psGraphSeries->uRange.dDouble = (psGraphSeries->uHigh.dDouble - psGraphSeries->uLow.dDouble) + 1;
			break;
		}
		case EVAR_BOOLEAN:
		{
			// 0-1!
			psGraphSeries->uRange.bBoolean = TRUE;
			break;
		}
		default:
		{
			// Someone forgot a variable type!
			GCASSERT(0);
		}
	}
}
	
ELCDErr GraphSeriesCreate(GRAPHSERIESHANDLE *peGraphSeriesHandle,
						 GRAPHHANDLE eGraphHandle,
						 BOOL bChangeable,
						 INT32 s32XOffset,
						 INT32 s32YOffset,
						 INT32 s32XSize,
						 INT32 s32YSize,
						 EVarType eVarType,
						 union UValue *puMinValue,
						 union UValue *puMaxValue,
						 INT32 s32ElementCount,
						 INT32 s32Origin,
						 BOOL bVisible)
{
	SGraphWidget *psGraphWidget = NULL;
	SGraphSeries *psGraphSeries = NULL;
	SWidget *psWidget = NULL;
	void *pvSeriesElements = NULL;
	ELCDErr eLCDErr = LERR_OK;
	UINT32 u32Loop = 0;
	BOOL bMinMaxFault = FALSE;

	// Verify that the X/Y offset is not <0
	if ((s32XOffset < 0) ||
		(s32YOffset < 0))
	{
		return(LERR_GRAPH_BAD_OFFSET);
	}

	// Verify the size is >0
	if ((s32XSize <= 0) ||
		(s32YSize <= 0))
	{
		return(LERR_GRAPH_BAD_SIZE);
	}

	// Verify that it's a numeric type
	if (FALSE == IsNumericType(eVarType))
	{
		return(LERR_GRAPH_TYPE_NOT_NUMERIC);
	}

	// Verfy that we have enough elements - must be at least 1
	if (s32ElementCount <= 0)
	{
		return(LERR_GRAPH_NOT_ENOUGH_ELEMENTS);
	}

	// Make sure it's a valid origin
	if ((s32Origin != (INT32) EORIGIN_TOP) &&
		(s32Origin != (INT32) EORIGIN_BOTTOM) &&
		(s32Origin != (INT32) EORIGIN_LEFT) &&
		(s32Origin != (INT32) EORIGIN_RIGHT))
	{
		return(LERR_GRAPH_BAD_ORIGIN);
	}

	// Verify that the graph handle is valid
	eLCDErr = WidgetGetPointerByHandle((WIDGETHANDLE) eGraphHandle,
										WIDGET_GRAPH,
										&psWidget,
										NULL);
	RETURN_ON_FAIL(eLCDErr);
	psGraphWidget = psWidget->uWidgetSpecific.psGraphWidget;
	GCASSERT(psGraphWidget);

	eLCDErr = CheckMinMax(eVarType,
						  puMinValue,
						  puMaxValue);

	if (eLCDErr != LERR_OK)
	{
		return(eLCDErr);
	}

	// Get a graph series handle
	if (FALSE == GraphSeriesFindFreeHandle(peGraphSeriesHandle))
	{
		return(LERR_GRAPH_SERIES_FULL);
	}

	// If we have a min/max fault, return an error
	if (bMinMaxFault)
	{
		return(LERR_GRAPH_MIN_LARGER_THAN_MAX);
	}

	// Our series is created. Now create an appropriate series structure.
	psGraphSeries = MemAlloc(sizeof(*psGraphSeries));
	if (NULL == psGraphSeries)
	{
		eLCDErr = LERR_OUT_OF_MEMORY;
		goto notAllocated;
	}

	// Allocate some space for our series elements
	psGraphSeries->pvSeriesData = MemAlloc(VarGetTypeSize(eVarType) * s32ElementCount);
	if (NULL == psGraphSeries->pvSeriesData)
	{
		// Out of memory. Damn.
		eLCDErr = LERR_OUT_OF_MEMORY;
		goto notAllocated;
	}

	// Now allocate space for our elements
	psGraphSeries->psElements = MemAlloc(sizeof(*psGraphSeries->psElements) * s32ElementCount);
	if (NULL == psGraphSeries->psElements)
	{
		// Out of memory. Damn.
		eLCDErr = LERR_OUT_OF_MEMORY;
		goto notAllocated;
	}

	// Hook up our series data
	psGraphSeries->bChangeable = bChangeable;
	psGraphSeries->s32XOffset = s32XOffset;
	psGraphSeries->s32YOffset = s32YOffset;
	psGraphSeries->s32XSize = s32XSize;
	psGraphSeries->s32YSize = s32YSize;
	psGraphSeries->u32SeriesElementSize = (UINT32) s32ElementCount;
	psGraphSeries->uHigh = *puMaxValue;
	psGraphSeries->uLow = *puMinValue;
	psGraphSeries->eVarType = eVarType;
	psGraphSeries->eOrigin = (EOrigin) s32Origin;
	psGraphSeries->eGraphHandle = eGraphHandle;
	psGraphSeries->eGraphSeriesHandle = *peGraphSeriesHandle;
	psGraphSeries->bVisible = bVisible;

	// Go set the graph series range
	SetMinMaxRange(psGraphSeries);

	// Go set the origin
	SetGraphOrigin(psGraphWidget,
				   psGraphSeries);

	// Hook it in to our series list
	sg_psGraphSeriesList[*peGraphSeriesHandle] = psGraphSeries;

	// Now go add it to the master series	
	if (NULL == psGraphWidget->psUserSeriess)
	{
		psGraphWidget->psUserSeriess = psGraphSeries;
	}
	else
	{
		SGraphSeries *psSeriesPtr = NULL;

		psSeriesPtr = psGraphWidget->psUserSeriess;
		while (psSeriesPtr->psNextLink)
		{
			psSeriesPtr = psSeriesPtr->psNextLink;
		}

		// Append this series to the end
		psSeriesPtr->psNextLink = psGraphSeries;
	}

	// All done!
	return(LERR_OK);

notAllocated:
	if (psGraphSeries && psGraphSeries->pvSeriesData)
	{
		GCFreeMemory(psGraphSeries->pvSeriesData);
		psGraphSeries->pvSeriesData = NULL;
	}

	if (psGraphSeries && psGraphSeries->psElements)
	{
		GCFreeMemory(psGraphSeries->psElements);
		psGraphSeries->psElements = NULL;
	}

	if (psGraphSeries)
	{
		GCFreeMemory(psGraphSeries);
	}

	return(eLCDErr);
}

static void CalcXYPosition(SGfxElement *psElement,
						   SGraphSeries *psGraphSeries,
						   UINT32 u32ElementIndex)
{
	if (EORIGIN_BOTTOM == psGraphSeries->eOrigin)
	{
		// Bar type? If so, do the nasty.
		if (ELEMTYPE_BAR == psElement->eElementType)
		{
			// Compute size of bar
			psElement->uEData.sBar.s32YSize = ((psGraphSeries->s32YSize * psElement->u32VirtualHeight) >> 16);

			// Bottom side bar draw. Move Y position to be origin - bar height
			psElement->s32YPos = psElement->s32YOrigin - psElement->uEData.sBar.s32YSize;
			psElement->s32XPos = psElement->s32XOrigin;

			// Shadow, if any, goes in the upper right hand corner
			psElement->uEData.sBar.eOrientation = EORIENT_UPPER_RIGHT;
		}
		else
		if ((ELEMTYPE_LINE_AA == psElement->eElementType) ||
			(ELEMTYPE_LINE_NORMAL == psElement->eElementType))
		{
			// Bottom side line draw. Move Y position to be origin - bar height
			psElement->s32YPos = psElement->s32YOrigin - ((psGraphSeries->s32YSize * psElement->u32VirtualHeight) >> 16);
			psElement->s32XPos = psElement->s32XOrigin;
		}
		else
		if (ELEMTYPE_DOT == psElement->eElementType)
		{
			// Bottom side bar draw. Move Y position to be origin - bar height
			psElement->s32YPos = psElement->s32YOrigin - ((psGraphSeries->s32YSize * psElement->u32VirtualHeight) >> 16);
			psElement->s32XPos = psElement->s32XOrigin;
		}
		else
		if (ELEMTYPE_TEXT == psElement->eElementType)
		{
			// Bottom side bar draw. Move Y position to be origin - bar height
			psElement->s32YPos = psElement->s32YOrigin - ((psGraphSeries->s32YSize * psElement->u32VirtualHeight) >> 16);
			psElement->s32XPos = psElement->s32XOrigin;
		}
		else
		{
			GCASSERT(0);
		}
	}
	else
	if (EORIGIN_TOP == psGraphSeries->eOrigin)
	{
		// Bar type? If so, do the nasty.
		if (ELEMTYPE_BAR == psElement->eElementType)
		{
			// Compute size of bar
			psElement->uEData.sBar.s32YSize = ((psGraphSeries->s32YSize * psElement->u32VirtualHeight) >> 16);

			// Bottom side bar draw. Move Y position to be origin - bar height
			psElement->s32YPos = psElement->s32YOrigin;
			psElement->s32XPos = psElement->s32XOrigin;

			// Shadow, if any, goes in the upper right hand corner
			psElement->uEData.sBar.eOrientation = EORIENT_LOWER_LEFT;
		}
		else
		if ((ELEMTYPE_LINE_AA == psElement->eElementType) ||
			(ELEMTYPE_LINE_NORMAL == psElement->eElementType))
		{
			// Bottom side line draw. Move Y position to be origin - bar height
			psElement->s32YPos = psElement->s32YOrigin + ((psGraphSeries->s32YSize * psElement->u32VirtualHeight) >> 16);
			psElement->s32XPos = psElement->s32XOrigin;
		}
		else
		{
			GCASSERT(0);
		}
	}
	else
	if (EORIGIN_LEFT == psGraphSeries->eOrigin)
	{
		// Bar type? If so, do the nasty.
		if (ELEMTYPE_BAR == psElement->eElementType)
		{
			// Compute size of bar
			psElement->uEData.sBar.s32XSize = ((psGraphSeries->s32XSize * psElement->u32VirtualHeight) >> 16);

			// Left side bar draw. Move Y position to be origin - bar height
			psElement->s32YPos = psElement->s32YOrigin;
			psElement->s32XPos = psElement->s32XOrigin;

			// Shadow, if any, goes in the upper right hand corner
			psElement->uEData.sBar.eOrientation = EORIENT_LOWER_RIGHT;
		}
		else
		if ((ELEMTYPE_LINE_AA == psElement->eElementType) ||
			(ELEMTYPE_LINE_NORMAL == psElement->eElementType))
		{
			// Bottom side bar draw. Move Y position to be origin - bar height
			psElement->s32YPos = psElement->s32YOrigin;
			psElement->s32XPos = psElement->s32XOrigin + ((psGraphSeries->s32XSize * psElement->u32VirtualHeight) >> 16);
		}
		else
		{
			GCASSERT(0);
		}
	}
	else
	if (EORIGIN_RIGHT == psGraphSeries->eOrigin)
	{
		// Bar type? If so, do the nasty.
		if (ELEMTYPE_BAR == psElement->eElementType)
		{
			// Compute size of bar
			psElement->uEData.sBar.s32XSize = ((psGraphSeries->s32XSize * psElement->u32VirtualHeight) >> 16);

			// Left side bar draw. Move Y position to be origin - bar height
			psElement->s32YPos = psElement->s32YOrigin;
			psElement->s32XPos = psElement->s32XOrigin - psElement->uEData.sBar.s32XSize;

			// Shadow, if any, goes in the upper right hand corner
			psElement->uEData.sBar.eOrientation = EORIENT_LOWER_LEFT;
		}
		else
		if ((ELEMTYPE_LINE_AA == psElement->eElementType) ||
			(ELEMTYPE_LINE_NORMAL == psElement->eElementType))
		{
			// Bottom side line draw. Move Y position to be origin - bar height
			psElement->s32YPos = psElement->s32YOrigin;
			psElement->s32XPos = psElement->s32XOrigin - ((psGraphSeries->s32XSize * psElement->u32VirtualHeight) >> 16);
		}
		else
		{
			GCASSERT(0);
		}
	}
	else
	{
		// Other orientations not (yet) supported
		GCASSERT(0);
	}
}


static void ClipAndAdjustValue(SGraphSeries *psGraphSeries,
							   union UValue *puValue,
							   SGfxElement *psElement)
{
	// Now, depending upon its type, we need to clip
	switch (psGraphSeries->eVarType)
	{	
		case EVAR_SIGNED_CHAR:
		{
			if ((*puValue).s8Char > psGraphSeries->uHigh.s8Char)	{ (*puValue).s8Char = psGraphSeries->uHigh.s8Char; }
			if ((*puValue).s8Char < psGraphSeries->uLow.s8Char) { (*puValue).s8Char = psGraphSeries->uLow.s8Char; }
			(*puValue).s8Char -= psGraphSeries->uLow.s8Char;
			GCASSERT(0);
			break;
		}
		case EVAR_UNSIGNED_CHAR:
		{
			if ((*puValue).u8Char > psGraphSeries->uHigh.u8Char)	{ (*puValue).s8Char = psGraphSeries->uHigh.u8Char; }
			if ((*puValue).u8Char < psGraphSeries->uLow.u8Char) { (*puValue).s8Char = psGraphSeries->uLow.u8Char; }
			(*puValue).u8Char -= psGraphSeries->uLow.u8Char;
			psElement->u32VirtualHeight = (UINT32) (((((UINT32) psGraphSeries->uLow.u8Char) << 16) / ((UINT32) psGraphSeries->uRange.u64LongInt)));
			break;
		}
		case EVAR_SIGNED_SHORT:
		{
			if ((*puValue).s16Short > psGraphSeries->uHigh.s16Short)	{ (*puValue).s16Short = psGraphSeries->uHigh.s16Short; }
			if ((*puValue).s16Short < psGraphSeries->uLow.s16Short) { (*puValue).s16Short = psGraphSeries->uLow.s16Short; }
			(*puValue).s16Short -= psGraphSeries->uLow.s16Short;
			psElement->u32VirtualHeight = (UINT32) (((((UINT32) (*puValue).s16Short) << 16) / ((UINT32) psGraphSeries->uRange.u64LongInt)));
			break;
		}
		case EVAR_UNSIGNED_SHORT:
		{
			if ((*puValue).u16Short > psGraphSeries->uHigh.u16Short)	{ (*puValue).u16Short = psGraphSeries->uHigh.u16Short; }
			if ((*puValue).u16Short < psGraphSeries->uLow.u16Short) { (*puValue).u16Short = psGraphSeries->uLow.u16Short; }
			(*puValue).u16Short -= psGraphSeries->uLow.u16Short;
			GCASSERT(0);
			break;
		}
		case EVAR_SIGNED_INT:
		{
			if ((*puValue).s32Int > psGraphSeries->uHigh.s32Int)	{ (*puValue).s32Int = psGraphSeries->uHigh.s32Int; }
			if ((*puValue).s32Int < psGraphSeries->uLow.s32Int) { (*puValue).s32Int = psGraphSeries->uLow.s32Int; }
			(*puValue).s32Int -= psGraphSeries->uLow.s32Int;
			psElement->u32VirtualHeight = (UINT32) (((((INT64) (*puValue).s32Int) << 16) / ((INT64) psGraphSeries->uRange.u64LongInt)));
			break;
		}
		case EVAR_UNSIGNED_INT:
		{
			if ((*puValue).u32Int > psGraphSeries->uHigh.u32Int)	{ (*puValue).u32Int = psGraphSeries->uHigh.u32Int; }
			if ((*puValue).u32Int < psGraphSeries->uLow.u32Int) { (*puValue).u32Int = psGraphSeries->uLow.u32Int; }
			(*puValue).u32Int -= psGraphSeries->uLow.u32Int;
			psElement->u32VirtualHeight = (UINT32) (((((UINT64) (*puValue).u32Int) << 16) / ((UINT64) psGraphSeries->uRange.u64LongInt)));
			break;
		}
		case EVAR_SIGNED_LONG_INT:
		{
			if ((*puValue).s64LongInt > psGraphSeries->uHigh.s64LongInt)	{ (*puValue).s64LongInt = psGraphSeries->uHigh.s64LongInt; }
			if ((*puValue).s64LongInt < psGraphSeries->uLow.s64LongInt) { (*puValue).s64LongInt = psGraphSeries->uLow.s64LongInt; }
			(*puValue).s64LongInt -= psGraphSeries->uLow.s64LongInt;
			GCASSERT(0);
			break;
		}
		case EVAR_UNSIGNED_LONG_INT:
		{
			if ((*puValue).u64LongInt > psGraphSeries->uHigh.u64LongInt)	{ (*puValue).u64LongInt = psGraphSeries->uHigh.u64LongInt; }
			if ((*puValue).u64LongInt < psGraphSeries->uLow.u64LongInt) { (*puValue).u64LongInt = psGraphSeries->uLow.u64LongInt; }
			(*puValue).u64LongInt -= psGraphSeries->uLow.u64LongInt;
			GCASSERT(0);
			break;
		}
		case EVAR_FLOAT:
		{
			if ((*puValue).fFloat > psGraphSeries->uHigh.fFloat)	{ (*puValue).fFloat = psGraphSeries->uHigh.fFloat; }
			if ((*puValue).fFloat < psGraphSeries->uLow.fFloat) { (*puValue).fFloat = psGraphSeries->uLow.fFloat; }
			(*puValue).fFloat -= psGraphSeries->uLow.fFloat;
			GCASSERT(0);
			break;
		}
		case EVAR_DOUBLE:
		{
			if ((*puValue).dDouble > psGraphSeries->uHigh.dDouble)	{ (*puValue).dDouble = psGraphSeries->uHigh.dDouble; }
			if ((*puValue).dDouble < psGraphSeries->uLow.dDouble) { (*puValue).dDouble = psGraphSeries->uLow.dDouble; }
			(*puValue).dDouble -= psGraphSeries->uLow.dDouble;
			GCASSERT(0);
			break;
		}
		case EVAR_BOOLEAN:
		{
			// No need to do anything as far as range finding/clipping goes
			GCASSERT(0);
			break;
		}
		default:
		{
			// Someone forgot a variable type!
			GCASSERT(0);
		}
	}
}

static BOOL GraphWidgetCalcElementBounds(SGraphSeries *psGraphSeries,
							   			 INT32 *ps32XLow,
										 INT32 *ps32XHigh,
										 INT32 *ps32YLow,
										 INT32 *ps32YHigh)
{
	SGfxElement *psElement;
	UINT32 u32Index = psGraphSeries->u32Tail;
	UINT32 u32ElementCount = psGraphSeries->u32SeriesCount;

	psElement = &psGraphSeries->psElements[u32Index];

	*ps32XLow = ((1 << 30) - 1);
	*ps32XHigh = -((1 << 30) - 1);
	*ps32YLow = ((1 << 30) - 1);
	*ps32YHigh = -((1 << 30) - 1);

	while (u32ElementCount)
	{
		UINT32 u32XSize;
		UINT32 u32YSize;
		INT32 s32XRight;
		INT32 s32YBottom;
		
		if (psElement->s32XPos < *ps32XLow)
		{
			*ps32XLow = psElement->s32XPos;
		}

		if (psElement->s32YPos < *ps32YLow)
		{
			*ps32YLow = psElement->s32YPos;
		}

		// Go compute the size
		psElement->psFuncs->ElemComputeBounds(psElement,
											  &u32XSize,
											  &u32YSize);

		s32XRight = psElement->s32XPos + (INT32) u32XSize;
		s32YBottom = psElement->s32YPos + (INT32) u32YSize;

		if (s32XRight > *ps32XHigh)
		{
			*ps32XHigh = s32XRight;
		}

		if (s32YBottom > *ps32YHigh)
		{
			*ps32YHigh = s32YBottom;
		}

		++psElement;
		u32Index++;
		if (u32Index >= psGraphSeries->u32SeriesElementSize)
		{
			u32Index = 0;
			psElement = psGraphSeries->psElements;
		}
		--u32ElementCount;
	}

	if (((1 << 30) - 1) == *ps32XLow)
	{
		// Means no resizing took place
		return(FALSE);
	}
	else
	{
		// Resizing took place
		return(TRUE);
	}
}

/* Series insertion algorithm:
 *
 * 1) Insert at head, series length < # of elements
 *    + Create item at u32Head position
 * 2) Insert at tail, series length < # of elements
 *    + Create an item at u32Tail - 1
 *    + Slide all items' x/y positions toward (u32Tail - 1) item
 *    + Create item at u32Tail position
 * 3) Insert at head, series length == # of elements
 *    + Slide all items' x/y positions from u32Tail to u32Tail + 1
 *    + Create item at u32Tail position
 *    + Increment u32Tail and u32Head
 * 4) Insert at tail, series length == # of elments
 *    + Slide all items' x/y positions from u32Tail + 1 to u32Tail
 *    + Create an item at u32Head
 *    + Decrement u32Tail and u32Head
 */

static SGfxElement *FIFOInsert(SGraphSeries *psGraphSeries,
							   SVar *psValue,
							   EElementType eElementType,
							   EOrigin eOrigin,
							   UINT16 u16DrawColor,
							   BOOL bBackOfSeries)
{
	SGfxElement *psElement = NULL;
	UINT32 u32InsertionIndex = 0;
	SGraphWidget *psGraphWidget = NULL;
	ELCDErr eErr;
	SWidget *psWidget = NULL;

	// If we have an update holdoff, just hang on until we've had a chance to
	// update it.

	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) psGraphSeries->eGraphHandle,
									WIDGET_GRAPH,
									&psWidget,
									NULL);
	GCASSERT(GC_OK == eErr);

	psGraphWidget = psWidget->uWidgetSpecific.psGraphWidget;
	GCASSERT(psGraphWidget);

	// If we already have enough items in the series, we'll need to slide
	if (psGraphSeries->u32SeriesCount >= psGraphSeries->u32SeriesElementSize)
	{
		// Option 3 or 4
		if (bBackOfSeries)
		{
			// Option 4 above - tail of series - full

			UINT32 u32TailIndex = 0;
			UINT32 u32Loop;

			// These had better match
			GCASSERT(psGraphSeries->u32Tail == psGraphSeries->u32Head);

			// Option 3 above - head of series - run through all items in the series
			// and recalculate everything.

			u32TailIndex = psGraphSeries->u32Tail;

			if (0 == psGraphSeries->u32Tail)
			{
				psGraphSeries->u32Tail = psGraphSeries->u32SeriesElementSize - 1;
			}
			else
			{
				psGraphSeries->u32Tail--;
			}

			// Go reset our origins
			SetGraphOrigin(psGraphWidget,
						   psGraphSeries);

			for (u32Loop = 1; u32Loop < psGraphSeries->u32SeriesElementSize; u32Loop++)
			{
				psElement = &psGraphSeries->psElements[u32TailIndex];

				// Calculate the X/Y position of this element
				CalcXYPosition(psElement,
							   psGraphSeries,
							   u32Loop);

				u32TailIndex++;
				if (u32TailIndex >= psGraphSeries->u32SeriesElementSize)
				{
					u32TailIndex = 0;
				}
			}

			psElement = &psGraphSeries->psElements[psGraphSeries->u32Tail];
			u32InsertionIndex = psGraphSeries->u32Tail;

			if (0 == psGraphSeries->u32Head)
			{
				psGraphSeries->u32Head = psGraphSeries->u32SeriesElementSize - 1;
			}
			else
			{
				psGraphSeries->u32Head--;
			}
		}
		else
		{
			UINT32 u32TailIndex = 0;
			UINT32 u32Loop;

			// These had better match
			GCASSERT(psGraphSeries->u32Tail == psGraphSeries->u32Head);

			// Option 3 above - head of series - run through all items in the series
			// and recalculate everything.

			psGraphSeries->u32Tail++;
			if (psGraphSeries->u32Tail >= psGraphSeries->u32SeriesElementSize)
			{
				psGraphSeries->u32Tail = 0;
			}

			// Go reset our origins
			SetGraphOrigin(psGraphWidget,
						   psGraphSeries);

			u32TailIndex = psGraphSeries->u32Tail;
			for (u32Loop = 0; u32Loop < psGraphSeries->u32SeriesElementSize; u32Loop++)
			{
				psElement = &psGraphSeries->psElements[u32TailIndex];

				// Calculate the X/Y position of this element
				CalcXYPosition(psElement,
							   psGraphSeries,
							   u32Loop);

				u32TailIndex++;
				if (u32TailIndex >= psGraphSeries->u32SeriesElementSize)
				{
					u32TailIndex = 0;
				}
			}

			psElement = &psGraphSeries->psElements[psGraphSeries->u32Head];
			u32InsertionIndex = psGraphSeries->u32Head;

			psGraphSeries->u32Head++;
			if (psGraphSeries->u32Head >= psGraphSeries->u32SeriesElementSize)
			{
				psGraphSeries->u32Head = 0;
			}
		}
	}
	else
	{
		// Option 1 or 2
		if (bBackOfSeries)
		{
			// Option 2 above - tail of series

			if (0 == psGraphSeries->u32Tail)
			{
				psGraphSeries->u32Tail = psGraphSeries->u32SeriesElementSize - 1;
			}
			else
			{
				psGraphSeries->u32Tail--;
			}

			u32InsertionIndex = psGraphSeries->u32Tail;
			psElement = &psGraphSeries->psElements[u32InsertionIndex];
		}
		else
		{
			// Option 1 above - head of series

			u32InsertionIndex = psGraphSeries->u32Head;
			psElement = &psGraphSeries->psElements[psGraphSeries->u32Head++];
			if (psGraphSeries->u32Head >= psGraphSeries->u32SeriesElementSize)
			{
				psGraphSeries->u32Head = 0;
			}
		}

		// Add an element to the overall count
		psGraphSeries->u32SeriesCount++;
	}

	// Go free the element if we can
	if (psElement->psFuncs && psElement->psFuncs->ElemFree)
	{
		psElement->psFuncs->ElemFree(psGraphSeries,
									 psElement);
	}

	// Fill in the value
	psElement->uValue = *psValue->puValuePtr;

	// Keep a reference copy, too
	psElement->uReferenceValue = psElement->uValue;

	ClipAndAdjustValue(psGraphSeries,
					   &psElement->uValue,
					   psElement);

	// Go fill in the pertinent data
	psElement = ElementCreate(eElementType,
							  psElement->s32XPos,
							  psElement->s32YPos,
							  u16DrawColor,
							  psElement);

	// Calculate the X/Y position of this element
	CalcXYPosition(psElement,
				   psGraphSeries,
				   u32InsertionIndex);

 	return(psElement);
}

void GraphSeriesCommit(SGfxElement *psElement,
					  SGraphSeries *psGraphSeries)
{
	INT32 s32XLow = 99999999;
	INT32 s32XHigh = -99999999;
	INT32 s32YLow = 99999999;
	INT32 s32YHigh = -99999999;

	// Now go calculate the bounds of this element
	psElement->psFuncs->ElemComputeBounds(psElement,
										  &psElement->u32XSize,
										  &psElement->u32YSize);

	if (FALSE == GraphWidgetCalcElementBounds(psGraphSeries,
											  &s32XLow,
											  &s32XHigh,
											  &s32YLow,
											  &s32YHigh))
	{
		// No update on outer boundaries - don't do squat
	}
	else
	{
		// If anything has changed, let's recalculate the whole widget
		if ((s32XLow != psGraphSeries->s32XMin) ||
			(s32XHigh != psGraphSeries->s32XMax) ||
			(s32YLow != psGraphSeries->s32YMin) ||
			(s32YHigh != psGraphSeries->s32YMax))
		{
			SGraphWidget *psGraph = NULL;
			SWidget *psWidget = NULL;

			psGraphSeries->s32XMin = s32XLow;
			psGraphSeries->s32XMax = s32XHigh;
			psGraphSeries->s32YMin = s32YLow;
			psGraphSeries->s32YMax = s32YHigh;

			(void) WidgetGetPointerByHandle((WIDGETHANDLE) psGraphSeries->eGraphHandle,
											WIDGET_GRAPH,
											&psWidget,
											NULL);

			GCASSERT(psWidget);
			psGraph = psWidget->uWidgetSpecific.psGraphWidget;
			GCASSERT(psGraph);

			// Schedule a recalculation
			psGraph->bRecalcNeeded = TRUE;
		}
	}
}

static ELCDErr GraphLock(GRAPHHANDLE eGraphHandle)
{
	SGraphWidget *psGraphWidget;
	ELCDErr eErr;
	SWidget *psWidget = NULL;

	// If we have an emergency update in progress or needed, back off
	while (sg_bEmergencyUpdate)
	{
		GCOSSleep(5);
	}

	// Verify that the graph handle is valid
	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eGraphHandle,
									WIDGET_GRAPH,
									&psWidget,
									NULL);
	RETURN_ON_FAIL(eErr);

	psGraphWidget = psWidget->uWidgetSpecific.psGraphWidget;
	GCASSERT(psGraphWidget);

	if (NULL == psGraphWidget)
	{
		return(LERR_GRAPH_BAD_HANDLE);
	}

	GCASSERT(FALSE == psGraphWidget->bUpdateInProgress);
	psGraphWidget->bUpdateInProgress = TRUE;

	return(LERR_OK);
}

static ELCDErr GraphUnlock(GRAPHHANDLE eGraphHandle)
{
	SGraphWidget *psGraphWidget;
	SWidget *psWidget = NULL;
	ELCDErr eErr;

	// Verify that the graph handle is valid
	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eGraphHandle,
									WIDGET_GRAPH,
									&psWidget,
									NULL);
	RETURN_ON_FAIL(eErr);

	psGraphWidget = psWidget->uWidgetSpecific.psGraphWidget;
	GCASSERT(psGraphWidget);

	GCASSERT(psGraphWidget->bUpdateInProgress);
	psGraphWidget->bUpdateInProgress = FALSE;

	return(LERR_OK);
}

static ELCDErr GraphTriggerUpdate(GRAPHHANDLE eGraphHandle,
								  SGraphSeries *psGraphSeries,
								  BOOL bRecalcNeeded)
{
	SGraphWidget *psGraph;
	SWidget *psWidget;
	ELCDErr eErr;

	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eGraphHandle,
									WIDGET_GRAPH,
									&psWidget,
									NULL);
	RETURN_ON_FAIL(eErr);

	psGraph = psWidget->uWidgetSpecific.psGraphWidget;
	GCASSERT(psGraph);

	if ((psGraphSeries) && (FALSE == psGraphSeries->bVisible))
	{
		// If the graph series isn't visible, then don't update the graph
		return(LERR_OK);
	}

	if (bRecalcNeeded)
	{
		// Indicate we need a recalc!
		psGraph->bRecalcNeeded = TRUE;
	}

	// Let's see what's goin on timer-wise with the parent
	if (psGraph->bUpdateTimersRunning)
	{
		// Reset our OCD timer
		psGraph->u32MSSinceLastFIFOInsert = 0;
		if (FALSE == psGraph->bUpdateTimersRunning)
		{
			// Timer not running any longer. Clear everything and restart.
			psGraph->u32MSSinceLastFIFOInsert = 0;
		}
	}
	else
	{
		psGraph->u32MSSinceLastFIFOInsert = 0;
	}

	psGraph->bUpdateTimersRunning = TRUE;
	return(LERR_OK);
}

static ELCDErr GraphUnlockAndTriggerUpdate(GRAPHHANDLE eGraphHandle,
										   ELCDErr eErr)
{
	eErr = GraphUnlock(eGraphHandle);

	if (LERR_OK == eErr)
	{
		// Now schedule an update - recalc is needed
		eErr = GraphTriggerUpdate(eGraphHandle,
								  NULL,
								  TRUE);
	}
	else
	{
		(void) GraphTriggerUpdate(eGraphHandle,
								  NULL,
								  TRUE);
	}

	return(eErr);
}

ELCDErr GraphInsertBar(GRAPHSERIESHANDLE eHandle,
					   SVar *psValue,
					   INT32 s32BarPixelWidth,
					   INT32 s32FrontColor,
					   INT32 s32TopColor,
					   INT32 s32SideColor,
					   INT32 s32ShadowPixelWidth,
					   INT32 s32TranslucencyValue,
					   BOOL bBackOfSeries)
{
	SGraphSeries *psGraphSeries = NULL;
	SGfxElement *psGfxElement = NULL;
	ELCDErr eErr;

	// Check the graph series
	psGraphSeries = GraphSeriesGetPointer(eHandle);
	if (NULL == psGraphSeries)
	{
		return(LERR_GRAPH_SERIES_BAD_HANDLE);
	}

	// See if the value type matches the series type
	if (psValue->eType != psGraphSeries->eVarType)
	{
		return(LERR_GRAPH_SERIES_TYPE_MISMATCH);
	}

	// Lock the graph - let 'em know we're updating it
	eErr = GraphLock(psGraphSeries->eGraphHandle);
	if (eErr != LERR_OK)
	{
		return(eErr);
	}

	// Got the right type. Now insert it into the series for this series handle
	psGfxElement = FIFOInsert(psGraphSeries,
							  psValue,
							  ELEMTYPE_BAR,
							  psGraphSeries->eOrigin,
							  (UINT16) CONVERT_24RGB_16RGB(s32FrontColor),
							  bBackOfSeries);
	GCASSERT(psGfxElement);

	// Copy in bar specific data
	psGfxElement->uEData.sBar.u8ShadowSize = (UINT8) s32ShadowPixelWidth;

	if ((EORIGIN_TOP == psGraphSeries->eOrigin) ||
		(EORIGIN_BOTTOM == psGraphSeries->eOrigin))
	{
		psGfxElement->uEData.sBar.s32XSize = s32BarPixelWidth;
	}
	else
	{
		psGfxElement->uEData.sBar.s32YSize = s32BarPixelWidth;
	}

	psGfxElement->uEData.sBar.u16SideShadowColor = (UINT16) CONVERT_24RGB_16RGB(s32SideColor);
	psGfxElement->uEData.sBar.u16TopShadowColor = (UINT16) CONVERT_24RGB_16RGB(s32TopColor);
	psGfxElement->uEData.sBar.u8SideShadowTranslucency = (UINT8) (s32TranslucencyValue);
	psGfxElement->uEData.sBar.u8TopShadowTranslucency = (UINT8) (s32TranslucencyValue);

	// Go commit the graph series
	GraphSeriesCommit(psGfxElement,
					  psGraphSeries);

	// Unlock the graph from usage now
	eErr = GraphUnlockAndTriggerUpdate(psGraphSeries->eGraphHandle,
									   eErr);

	return(eErr);
}

static ELCDErr GraphInsertLineInternal(GRAPHSERIESHANDLE eHandle,
									    SVar *psValue,
									    INT32 s32LineColor,
									    INT32 s32TranslucencyValue,
									    BOOL bBackOfSeries,
										EElementType eLineElementType,
										BOOL bAreaFill,
										UINT16 u16AreaFillColor,
										UINT8 u8AreaFillTranslucency)
{
	SGraphSeries *psGraphSeries = NULL;
	SGfxElement *psGfxElement = NULL;
	ELCDErr eErr;

	// Check the graph series
	psGraphSeries = GraphSeriesGetPointer(eHandle);
	if (NULL == psGraphSeries)
	{
		return(LERR_GRAPH_SERIES_BAD_HANDLE);
	}

	// See if the value type matches the series type
	if (psValue->eType != psGraphSeries->eVarType)
	{
		return(LERR_GRAPH_SERIES_TYPE_MISMATCH);
	}

	// Lock the graph - let 'em know we're updating it
	eErr = GraphLock(psGraphSeries->eGraphHandle);
	if (eErr != LERR_OK)
	{
		return(eErr);
	}

	// Better be of a line type
	GCASSERT((ELEMTYPE_LINE_AA == eLineElementType) ||
			 (ELEMTYPE_LINE_NORMAL == eLineElementType));

	// Got the right type. Now insert it into the series for this series handle
	psGfxElement = FIFOInsert(psGraphSeries,
							  psValue,
							  eLineElementType,
							  psGraphSeries->eOrigin,
							  (UINT16) CONVERT_24RGB_16RGB(s32LineColor),
							  bBackOfSeries);
	GCASSERT(psGfxElement);

	// Copy in line specific data

	psGfxElement->uEData.sLine.u16DrawColor = (UINT16) CONVERT_24RGB_16RGB(s32LineColor);
	psGfxElement->uEData.sLine.u32Intensity = (UINT32) s32TranslucencyValue;
	psGfxElement->uEData.sLine.bAreaFill = bAreaFill;
	psGfxElement->uEData.sLine.u16AreaFillColor = u16AreaFillColor;
	psGfxElement->uEData.sLine.u8AreaFillTranslucency = u8AreaFillTranslucency;

	// Go commit the graph series
	GraphSeriesCommit(psGfxElement,
					 psGraphSeries);

	// Unlock the graph from usage now
	eErr = GraphUnlockAndTriggerUpdate(psGraphSeries->eGraphHandle,
									   eErr);

	return(eErr);
}

ELCDErr GraphInsertLineAA(GRAPHSERIESHANDLE eHandle,
								 SVar *psValue,
								 INT32 s32LineColor,
								 INT32 s32TranslucencyValue,
								 BOOL bBackOfSeries,
								 BOOL bAreaFill,
								 UINT16 u16AreaFillColor,
								 UINT8 u8AreaFillTranslucency)
{
	return(GraphInsertLineInternal(eHandle,
								   psValue,
								   s32LineColor,
								   s32TranslucencyValue,
								   bBackOfSeries,
								   ELEMTYPE_LINE_AA,
								   bAreaFill,
								   u16AreaFillColor,
								   u8AreaFillTranslucency));
}

ELCDErr GraphInsertLineNormal(GRAPHSERIESHANDLE eHandle,
							  SVar *psValue,
							  INT32 s32LineColor,
							  INT32 s32TranslucencyValue,
							  BOOL bBackOfSeries,
							  BOOL bAreaFill,
							  UINT16 u16AreaFillColor,
							  UINT8 u8AreaFillTranslucency)
{
	return(GraphInsertLineInternal(eHandle,
								   psValue,
								   s32LineColor,
								   s32TranslucencyValue,
								   bBackOfSeries,
								   ELEMTYPE_LINE_NORMAL,
								   bAreaFill,
								   u16AreaFillColor,
								   u8AreaFillTranslucency));
}

ELCDErr GraphInsertDot(GRAPHSERIESHANDLE eHandle,
					   SVar *psValue,
					   INT32 s32DotColor,
					   INT32 s32TranslucencyValue,
					   BOOL bBackOfSeries)
{
	SGraphSeries *psGraphSeries = NULL;
	SGfxElement *psGfxElement = NULL;
	ELCDErr eErr;

	// Check the graph series
	psGraphSeries = GraphSeriesGetPointer(eHandle);
	if (NULL == psGraphSeries)
	{
		return(LERR_GRAPH_SERIES_BAD_HANDLE);
	}

	// See if the value type matches the series type
	if (psValue->eType != psGraphSeries->eVarType)
	{
		return(LERR_GRAPH_SERIES_TYPE_MISMATCH);
	}

	// Lock the graph - let 'em know we're updating it
	eErr = GraphLock(psGraphSeries->eGraphHandle);
	if (eErr != LERR_OK)
	{
		return(eErr);
	}

	// Got the right type. Now insert it into the series for this series handle
	psGfxElement = FIFOInsert(psGraphSeries,
							  psValue,
							  ELEMTYPE_DOT,
							  psGraphSeries->eOrigin,
							  (UINT16) CONVERT_24RGB_16RGB(s32DotColor),
							  bBackOfSeries);
	GCASSERT(psGfxElement);

	// Go commit the graph series
	GraphSeriesCommit(psGfxElement,
					 psGraphSeries);

	// Unlock the graph from usage now
	eErr = GraphUnlockAndTriggerUpdate(psGraphSeries->eGraphHandle,
									   eErr);

	return(eErr);
}

ELCDErr GraphInsertText(GRAPHSERIESHANDLE eHandle,
					    SVar *psValue,
					    LEX_CHAR *peText,
						INT32 s32TextColor,
						FONTHANDLE eFontHandle,
						INT32 s32Origin,
						BOOL bBackOfSeries)
{
	SGraphSeries *psGraphSeries = NULL;
	SGfxElement *psGfxElement = NULL;
	ELCDErr eErr;

	// Make sure it's a valid origin
	if ((s32Origin != (INT32) EORIGIN_TOP) &&
		(s32Origin != (INT32) EORIGIN_BOTTOM) &&
		(s32Origin != (INT32) EORIGIN_LEFT) &&
		(s32Origin != (INT32) EORIGIN_RIGHT))
	{
		return(LERR_GRAPH_BAD_ORIGIN);
	}

	// Check the graph series
	psGraphSeries = GraphSeriesGetPointer(eHandle);
	if (NULL == psGraphSeries)
	{
		return(LERR_GRAPH_SERIES_BAD_HANDLE);
	}

	// See if the value type matches the series type
	if (psValue->eType != psGraphSeries->eVarType)
	{
		return(LERR_GRAPH_SERIES_TYPE_MISMATCH);
	}

	// Lock the graph - let 'em know we're updating it
	eErr = GraphLock(psGraphSeries->eGraphHandle);
	if (eErr != LERR_OK)
	{
		return(eErr);
	}

	// Got the right type. Now insert it into the series for this series handle
	psGfxElement = FIFOInsert(psGraphSeries,
							  psValue,
							  ELEMTYPE_TEXT,
							  psGraphSeries->eOrigin,
							  (UINT16) CONVERT_24RGB_16RGB(s32TextColor),
							  bBackOfSeries);
	GCASSERT(psGfxElement);

	// Now copy the text object into it
	psGfxElement->uEData.sText.peText = Lexstrdup(peText);
	psGfxElement->uEData.sText.eFontHandle = eFontHandle;

	if (EORIGIN_BOTTOM == s32Origin)
	{
		psGfxElement->uEData.sText.eRotation = ROT_0;
	}
	else
	if (EORIGIN_TOP == s32Origin)
	{
		psGfxElement->uEData.sText.eRotation = ROT_180;
	}
	else
	if (EORIGIN_LEFT == s32Origin)
	{
		psGfxElement->uEData.sText.eRotation = ROT_270;
	}
	else
	if (EORIGIN_RIGHT == s32Origin)
	{
		psGfxElement->uEData.sText.eRotation = ROT_90;
	}
	else
	{
		// Somebody screwed up in checking the origin above.
		GCASSERT(0);
	}

	// Go calculate the size
	psGfxElement->psFuncs->ElemComputeBounds(psGfxElement,
											 &psGfxElement->u32XSize,
											 &psGfxElement->u32YSize);

	if (NULL == psGfxElement->uEData.sText.peText)
	{
		// Out of memory. Yikes!
		(void) GraphUnlock(psGraphSeries->eGraphHandle);
		eErr = LERR_OUT_OF_MEMORY;
	}
	else
	{
		// Go commit the graph series
		GraphSeriesCommit(psGfxElement,
						 psGraphSeries);

		// Unlock the graph from usage now
		eErr = GraphUnlockAndTriggerUpdate(psGraphSeries->eGraphHandle,
										   eErr);
	}

	return(eErr);
}


ELCDErr GraphSeriesSetVisible(GRAPHSERIESHANDLE eGraphSeriesHandle,
							 BOOL bVisible)
{
	SGraphSeries *psGraphSeries = NULL;
	ELCDErr eErr = LERR_OK;

	// Check the graph series
	psGraphSeries = GraphSeriesGetPointer(eGraphSeriesHandle);
	if (NULL == psGraphSeries)
	{
		return(LERR_GRAPH_SERIES_BAD_HANDLE);
	}

	if (psGraphSeries->bVisible != bVisible)
	{
		// Means we have a change.
		psGraphSeries->bVisible = bVisible;

		eErr = GraphTriggerUpdate(psGraphSeries->eGraphHandle,
								  NULL,
								  TRUE);
	}

	return(eErr);
}

ELCDErr GraphSeriesSetMinMax(GRAPHSERIESHANDLE eGraphSeriesHandle,
							SVar *psMin,
							SVar *psMax)
{
	SGraphSeries *psGraphSeries = NULL;
	ELCDErr eErr = LERR_OK;
	UINT32 u32Index;
	UINT32 u32Count;

	// Check the graph series
	psGraphSeries = GraphSeriesGetPointer(eGraphSeriesHandle);
	if (NULL == psGraphSeries)
	{
		return(LERR_GRAPH_SERIES_BAD_HANDLE);
	}

	// See if our series type matches the variable types
	if ((psMin->eType != psGraphSeries->eVarType) ||
		(psMax->eType != psGraphSeries->eVarType))
	{
		return(LERR_GRAPH_SERIES_TYPE_MISMATCH);
	}

	// Lock it!
	eErr = GraphLock(psGraphSeries->eGraphHandle);
	if (eErr != LERR_OK)
	{
		return(eErr);
	}

	// Run through all elements in this series and recalc them
	u32Index = psGraphSeries->u32Tail;
	u32Count = psGraphSeries->u32SeriesCount;

	// Set our new min/max values!
	psGraphSeries->uLow = *psMin->puValuePtr;
	psGraphSeries->uHigh = *psMax->puValuePtr;

	// Set the min/max range
	SetMinMaxRange(psGraphSeries);

	while (u32Count)
	{
		SGfxElement *psElement;

		// Take our reference value, copy it in
		psElement = &psGraphSeries->psElements[u32Index];

		psElement->uValue = psElement->uReferenceValue;

		ClipAndAdjustValue(psGraphSeries,
						   &psElement->uValue,
						   psElement);

		// Calculate the X/Y position of this element
		CalcXYPosition(psElement,
					   psGraphSeries,
					   u32Index);

		u32Index++;
		if (u32Index >= psGraphSeries->u32SeriesElementSize)
		{
			u32Index = 0;
		}

		--u32Count;
	}

	// Unlock the graph from usage now
	eErr = GraphUnlockAndTriggerUpdate(psGraphSeries->eGraphHandle,
									   eErr);

	return(eErr);
}

ELCDErr GraphSetBackgroundImage(GRAPHHANDLE eGraphHandle,
								LEX_CHAR *peFilename,
								UINT32 u32XOffset,
								UINT32 u32YOffset)
{
	SGraphWidget *psGraphWidget = NULL;
	ELCDErr eErr = LERR_OK;
	EGCResultCode eGCResult;
	BOOL bUpdateWidget = FALSE;
	SWidget *psWidget = NULL;

	// Verify that the graph handle is valid
	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eGraphHandle,
									WIDGET_GRAPH,
									&psWidget,
									NULL);
	RETURN_ON_FAIL(eErr);

	psGraphWidget = psWidget->uWidgetSpecific.psGraphWidget;
	GCASSERT(psGraphWidget);

	// Lock it!
	eErr = GraphLock(eGraphHandle);
	if (eErr != LERR_OK)
	{
		return(eErr);
	}

	// If we currently have an image, let's delete it
	if (psGraphWidget->psBackgroundImage)
	{
		// We have an image, and we're clearing/deleting it

		GfxDecRef(psGraphWidget->psBackgroundImage);
		psGraphWidget->psBackgroundImage = NULL;
		bUpdateWidget = TRUE;
	}

	// If we have provided a new filename, let's load it
	if (peFilename)
	{
		psGraphWidget->psBackgroundImage = GfxLoadImageGroup(peFilename,
															 &eGCResult);
		eErr = (ELCDErr) (eGCResult);
		if (LERR_OK == eErr)
		{
			// Only update if we didn't get an error
			bUpdateWidget = TRUE;
		}

		GfxIncRef(psGraphWidget->psBackgroundImage);
		psGraphWidget->u32BackgroundImageOffsetX = u32XOffset;
		psGraphWidget->u32BackgroundImageOffsetY = u32YOffset;
	}

	if (bUpdateWidget)
	{
		// Kick off/schedule an update for this widget
		if (LERR_OK == eErr)
		{
			eErr = GraphTriggerUpdate(eGraphHandle,
									  NULL,
									  TRUE);
		}
		else
		{
			(void) GraphTriggerUpdate(eGraphHandle,
									  NULL,
									  TRUE);
		}
	}

	if (LERR_OK == eErr)
	{
		eErr = GraphUnlock(eGraphHandle);
	}
	else
	{
		(void) GraphUnlock(eGraphHandle);
	}

	return(eErr);
}

ELCDErr GraphSeriesSetImage(GRAPHSERIESHANDLE eHandle,
						   LEX_CHAR *peFilename)
{
	SGraphSeries *psGraphSeries = NULL;
	SGfxElement *psGfxElement = NULL;
	ELCDErr eErr = LERR_OK;
	EGCResultCode eGCResult = GC_OK;
	BOOL bUpdateWidget = FALSE;

	// Check the graph series
	psGraphSeries = GraphSeriesGetPointer(eHandle);
	if (NULL == psGraphSeries)
	{
		return(LERR_GRAPH_SERIES_BAD_HANDLE);
	}

	// Lock the graph
	eErr = GraphLock(psGraphSeries->eGraphHandle);
	if (eErr != LERR_OK)
	{
		return(eErr);
	}

	// Do we have an image currently?
	if (psGraphSeries->psSeriesImage)
	{
		// Delete it.
		GfxDeleteImageGroup(psGraphSeries->psSeriesImage);
		psGraphSeries->psSeriesImage = NULL;
		bUpdateWidget = TRUE;
	}

	// Only attempt to open a file if there's a filename passed to us
	if (peFilename)
	{
		psGraphSeries->psSeriesImage = GfxLoadImageGroup(peFilename,
													   &eGCResult);
		eErr = (ELCDErr) (eGCResult);
		if (LERR_OK == eErr)
		{
			// Only update if we didn't get an error
			bUpdateWidget = TRUE;
			GfxIncRef(psGraphSeries->psSeriesImage);
		}
	}

	// Go schedule this widget for an update
	if (bUpdateWidget)
	{
		// Kick off/schedule an update for this widget
		if (LERR_OK == eErr)
		{
			eErr = GraphTriggerUpdate(psGraphSeries->eGraphHandle,
									  NULL,
									  FALSE);
		}
		else
		{
			(void) GraphTriggerUpdate(psGraphSeries->eGraphHandle,
									  NULL,
									  FALSE);
		}
	}

	// Only reassign the error if it fails
	if (LERR_OK == eErr)
	{
		eErr = GraphUnlock(psGraphSeries->eGraphHandle);
	}
	else
	{
		(void) GraphUnlock(psGraphSeries->eGraphHandle);
	}

	return(eErr);
}

ELCDErr GraphSeriesDestroy(GRAPHSERIESHANDLE eHandle)
{
	SGraphSeries *psGraphSeries = NULL;
	SGraphWidget *psGraphWidget = NULL;
	ELCDErr eErr = LERR_OK;
	SWidget *psWidget;

	// Check the graph series
	psGraphSeries = GraphSeriesGetPointer(eHandle);
	if (NULL == psGraphSeries)
	{
		return(LERR_GRAPH_SERIES_BAD_HANDLE);
	}

	// Now get the graph handle
	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) psGraphSeries->eGraphHandle,
									WIDGET_GRAPH,
									&psWidget,
									NULL);
	RETURN_ON_FAIL(eErr);

	psGraphWidget = psWidget->uWidgetSpecific.psGraphWidget;
	GCASSERT(psGraphWidget);

	// Lock the graph
	eErr = GraphLock(psGraphSeries->eGraphHandle);
	if (eErr != LERR_OK)
	{
		return(eErr);
	}

	// Go delete the graph series
	GraphSeriesDelete(psGraphSeries,
					  psGraphWidget);

	// Now schedule an update - recalc is needed
	eErr = GraphUnlockAndTriggerUpdate(psGraphSeries->eGraphHandle,
									   eErr);

	return(eErr);
}

ELCDErr GraphDestroy(GRAPHHANDLE eHandle)
{
	ELCDErr eErr = LERR_OK;

	// Lock the graph - prevent updates - no unlock needed
	eErr = GraphLock(eHandle);
	if (eErr != LERR_OK)
	{
		return(eErr);
	}

	eErr = WidgetDestroyByHandle(&eHandle,
								 WIDGET_GRAPH);

	if (LERR_OK == eErr)
	{
		eErr = GraphUnlock(eHandle);
	}
	else
	{
		(void *) GraphUnlock(eHandle);
	}

	return(eErr);
}

#define	GRAPH_TIMER_INTERVAL	5

#define	GRAPH_LOW_GUARD_TIME	15			// Roughly 60FPS
#define GRAPH_HIGH_GUARD_TIME	100			// 10FPS

static void GraphTimerCallback(UINT32 u32Param)
{
	SGraphWidget *psGraph = sg_psGraphWidgetHead;

	while (psGraph)
	{
		if (psGraph->bUpdateTimersRunning)
		{
			psGraph->u32MSSinceLastFIFOInsert += GRAPH_TIMER_INTERVAL;
			psGraph->u32MSSinceLastUpdateRequest += GRAPH_TIMER_INTERVAL;

			if ((psGraph->u32MSSinceLastFIFOInsert > GRAPH_LOW_GUARD_TIME) &&
				(FALSE == sg_bEmergencyUpdate))
			{
				// Time has passed since last FIFO insert - update!
				if (LERR_OK == WindowDepositMessage(WCMD_WINDOW_GRAPH_UPDATE | psGraph->eGraphHandle))
				{
					psGraph->bUpdateTimersRunning = FALSE;
					psGraph->u32MSSinceLastFIFOInsert = 0;
					psGraph->u32MSSinceLastUpdateRequest = 0;
				}
			}
			else
			if (psGraph->u32MSSinceLastUpdateRequest > GRAPH_HIGH_GUARD_TIME)
			{
				if (FALSE == sg_bEmergencyUpdate)
				{
					if (LERR_OK == WindowDepositMessage(WCMD_WINDOW_GRAPH_UPDATE | psGraph->eGraphHandle))
					{
						sg_bEmergencyUpdate = TRUE;
						psGraph->bUpdateTimersRunning = FALSE;
						psGraph->u32MSSinceLastUpdateRequest = 0;
						psGraph->u32MSSinceLastFIFOInsert = 0;
					}
				}
			}
		}

		psGraph = psGraph->psNextLink;
	}
}

ELCDErr GraphTextCreate(GRAPHHANDLE eGraphHandle,
						LEX_CHAR *peFontFilename,
						UINT32 u32FontSize,
						LEX_CHAR *peString,
						UINT32 u32XOffset,
						UINT32 u32YOffset,
						UINT16 u16SColor,
						UINT16 u16Orientation)
{
	SGraphWidget *psGraphWidget = NULL;
	SGfxElement *psElement = NULL;
	ELCDErr eErr = LERR_OK;
	FONTHANDLE eFontHandle;
	SWidget *psWidget;

	// Now get the graph handle
	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eGraphHandle,
									WIDGET_GRAPH,
									&psWidget,
									NULL);
	RETURN_ON_FAIL(eErr);

	psGraphWidget = psWidget->uWidgetSpecific.psGraphWidget;
	GCASSERT(psGraphWidget);

	// Now we do a font allocate
	eErr = FontCreate(peFontFilename,
					  u32FontSize,
					  0,
					  &eFontHandle);
	if (eErr != LERR_OK)
	{
		goto errorExit;
	}

	// Lock the graph
	eErr = GraphLock(eGraphHandle);
	if (eErr != LERR_OK)
	{
		return(eErr);
	}

	// Make a copy of this text for later
	peString = Lexstrdup(peString);
	if (NULL == peString)
	{
		// Out of memory, damnit
		eErr = LERR_NO_MEM;
		goto errorExit;
	}

	// Go create a text element for this
	psElement = ElementCreate(ELEMTYPE_TEXT,
							  (INT32) u32XOffset + psGraphWidget->psWidget->s32XPos,
							  (INT32) u32YOffset + psGraphWidget->psWidget->s32YPos,
							  u16SColor,
							  NULL);

	if (NULL == psElement)
	{
		eErr = LERR_NO_MEM;
		goto errorExit;
	}

	// We've got an element created. Let's fill in more stuff.

	psElement->psNextLink = psGraphWidget->psTextAnnotation;
	psGraphWidget->psTextAnnotation = psElement;

	// Now put in the text
	psElement->uEData.sText.eFontHandle = eFontHandle;
	psElement->uEData.sText.eRotation = (ERotation) u16Orientation;
	psElement->uEData.sText.peText = peString;

	// Now schedule an update - recalc is needed
	eErr = GraphTriggerUpdate(eGraphHandle,
							  NULL,
							  TRUE);

errorExit:
	// Only reassign the error if it fails
	if (LERR_OK == eErr)
	{
		eErr = GraphUnlock(eGraphHandle);
	}
	else
	{
		(void) GraphUnlock(eGraphHandle);
	}

	return(eErr);
}

ELCDErr GraphTextDestroy(GRAPHHANDLE eGraphHandle)
{
	SGraphWidget *psGraphWidget = NULL;
	SGfxElement *psElement = NULL;
	ELCDErr eErr = LERR_OK;
	SWidget *psWidget;

	// Now get the graph handle
	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eGraphHandle,
									WIDGET_GRAPH,
									&psWidget,
									NULL);
	RETURN_ON_FAIL(eErr);

	psGraphWidget = psWidget->uWidgetSpecific.psGraphWidget;
	GCASSERT(psGraphWidget);

	// Lock the graph
	eErr = GraphLock(eGraphHandle);
	if (eErr != LERR_OK)
	{
		return(eErr);
	}

	// Go wipe out the layer
	GraphTextDestroyInternal(psGraphWidget);

	// Now schedule an update - recalc is needed
	eErr = GraphTriggerUpdate(eGraphHandle,
							  NULL,
							  TRUE);

	// Only reassign the error if it fails
	if (LERR_OK == eErr)
	{
		eErr = GraphUnlock(eGraphHandle);
	}
	else
	{
		(void) GraphUnlock(eGraphHandle);
	}

	return(eErr);
}

static SWidgetTypeMethods sg_sGraphMethods = 
{
	&sg_sGraphWidgetFunctions,
	LERR_GRAPH_BAD_HANDLE,			// Error when it's not the handle we're looking for
	GraphWidgetAlloc,
	GraphWidgetFree
};

void GraphFirstTimeInit(void)
{
	EGCResultCode eResult;

	DebugOut("* Initializing graph widget\n");

	// Fire up the deferred update timer

	// Set up a timer callback every GRAPH_TIMER_INTERVAL
	eResult = GCTimerCreate(&sg_psGraphTimer);
	GCASSERT(GC_OK == eResult);
	eResult = GCTimerSetCallback(sg_psGraphTimer,
								 GraphTimerCallback,
								 0);
	GCASSERT(GC_OK == eResult);
	eResult = GCTimerSetValue(sg_psGraphTimer,
							  GRAPH_TIMER_INTERVAL,
							  GRAPH_TIMER_INTERVAL);

	GCASSERT(GC_OK == eResult);
	eResult = GCTimerStart(sg_psGraphTimer);
	GCASSERT(GC_OK == eResult);

	WidgetRegisterTypeMethods(WIDGET_GRAPH,
							  &sg_sGraphMethods);
}

ELCDErr GraphSeriesTextCreate(GRAPHSERIESHANDLE eHandle,
							  FONTHANDLE eFontHandle,
							  LEX_CHAR *peString,
							  UINT32 u32XOffset,
							  UINT32 u32YOffset,
							  UINT16 u16TextColor,
							  UINT16 u16Orientation)
{
	SGraphWidget *psGraphWidget = NULL;
	SGraphSeries *psGraphSeries = NULL;
	SGfxElement *psElement = NULL;
	SGfxElement *psElementTmp = NULL;
	SWidget *psWidget = NULL;
	ELCDErr eErr;

	if ((u16Orientation != ROT_0) &&
		(u16Orientation != ROT_90) &&
		(u16Orientation != ROT_180) && 
		(u16Orientation != ROT_270))
	{
		return(LERR_GRAPH_BAD_ROTATION);
	}

	// Check the graph series
	psGraphSeries = GraphSeriesGetPointer(eHandle);
	if (NULL == psGraphSeries)
	{
		return(LERR_GRAPH_SERIES_BAD_HANDLE);
	}

	// Now get the graph handle
	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) psGraphSeries->eGraphHandle,
									WIDGET_GRAPH,
									&psWidget,
									NULL);
	RETURN_ON_FAIL(eErr);

	psGraphWidget = psWidget->uWidgetSpecific.psGraphWidget;
	GCASSERT(psGraphWidget);

	// If it's a NULL or empty string, just return OK and don't do anything.
	if ((NULL == peString) || (Lexstrlen(peString) == 0))
	{
		return(LERR_OK);
	}

	// Go create a text element
	psElement = ElementCreate(ELEMTYPE_TEXT,
							  (INT32) u32XOffset + psGraphWidget->psWidget->s32XPos,
							  (INT32) u32YOffset + psGraphWidget->psWidget->s32YPos,
							  u16TextColor,
							  NULL);

	if (NULL == psElement)
	{
		eErr = LERR_NO_MEM;
		goto errorExit;
	}

	psElement->uEData.sText.eFontHandle = eFontHandle;
	psElement->uEData.sText.eRotation = u16Orientation;
	psElement->uEData.sText.peText = Lexstrdup(peString);
	psElement->u16DrawColor = u16TextColor;

	if (NULL == psElement->uEData.sText.peText)
	{
		// Out of memory. Damn.
		GraphTextDestroyChain(psElement,
							  psGraphSeries);
		eErr = LERR_NO_MEM;
		goto errorExit;
	}

	// Figure out how big the text is in this orientation and record the X/Y size
	eErr = FontGetStringSize(psElement->uEData.sText.eFontHandle,
							 psElement->uEData.sText.peText,
							 &psElement->u32XSize,
							 &psElement->u32YSize,
							 psElement->uEData.sText.eRotation,
							 TRUE);

	if (eErr != LERR_OK)
	{
		GraphTextDestroyChain(psElement,
						      psGraphSeries);
		goto errorExit;
	}

	// Lock the graph - let 'em know we're updating it
	eErr = GraphLock(psGraphSeries->eGraphHandle);
	if (eErr != LERR_OK)
	{
		return(eErr);
	}

	// Append this to the last item in the chain 
	psElementTmp = psGraphSeries->psSeriesTextElements;

	while (psElementTmp && psElementTmp->psNextLink)
	{
		psElementTmp = psElementTmp->psNextLink;
	}

	if (NULL == psElementTmp)
	{
		psGraphSeries->psSeriesTextElements = psElement;
	}
	else
	{
		psElementTmp->psNextLink = psElement;
	}

errorExit:
	eErr = GraphUnlockAndTriggerUpdate(psGraphSeries->eGraphHandle,
									   eErr);

	return(eErr);
}

ELCDErr GraphSeriesTextDestroy(GRAPHSERIESHANDLE eGraphSeriesHandle)
{
	SGraphSeries *psGraphSeries = NULL;
	ELCDErr eErr;

	// Check the graph series
	psGraphSeries = GraphSeriesGetPointer(eGraphSeriesHandle);
	if (NULL == psGraphSeries)
	{
		return(LERR_GRAPH_SERIES_BAD_HANDLE);
	}

	// Lock the graph - let 'em know we're updating it
	eErr = GraphLock(psGraphSeries->eGraphHandle);
	if (eErr != LERR_OK)
	{
		return(eErr);
	}

	// Now go delete the chain
	GraphTextDestroyChain(psGraphSeries->psSeriesTextElements,
						  psGraphSeries);
	psGraphSeries->psSeriesTextElements = NULL;

	eErr = GraphUnlockAndTriggerUpdate(psGraphSeries->eGraphHandle,
									   eErr);

	return(eErr);
}

ELCDErr GraphGridCreate(GRAPHHANDLE eGraphHandle,
						SGraphGrid *psGraphGrid)
{
	SGraphWidget *psGraphWidget = NULL;
	ELCDErr eErr;
	SWidget *psWidget;

	// Check the graph
	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eGraphHandle,
									WIDGET_GRAPH,
									&psWidget,
									NULL);
	RETURN_ON_FAIL(eErr);

	psGraphWidget = psWidget->uWidgetSpecific.psGraphWidget;
	GCASSERT(psGraphWidget);

	// Lock the graph - let 'em know we're updating it
	eErr = GraphLock(eGraphHandle);
	if (eErr != LERR_OK)
	{
		return(eErr);
	}

	// Copy in our new graph grid
	memcpy((void *) &psGraphWidget->sGrid, psGraphGrid, sizeof(psGraphWidget->sGrid));

	// Go update/draw. Nothing to calc.
	eErr = GraphUnlockAndTriggerUpdate(eGraphHandle,
									   eErr);

	return(eErr);
}

ELCDErr GraphGridDestroy(GRAPHHANDLE eGraphHandle)
{
	SGraphWidget *psGraphWidget = NULL;
	ELCDErr eErr;
	SWidget *psWidget;

	// Check the graph
	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eGraphHandle,
									WIDGET_GRAPH,
									&psWidget,
									NULL);
	RETURN_ON_FAIL(eErr);

	psGraphWidget = psWidget->uWidgetSpecific.psGraphWidget;
	GCASSERT(psGraphWidget);

	// Lock the graph - let 'em know we're updating it
	eErr = GraphLock(eGraphHandle);
	if (eErr != LERR_OK)
	{
		return(eErr);
	}

	// Clear out the graph grid
	memset((void *) &psGraphWidget->sGrid, 0, sizeof(psGraphWidget->sGrid));

	// Go update/draw. Nothing to calc.
	eErr = GraphUnlockAndTriggerUpdate(eGraphHandle,
									   eErr);

	return(eErr);

}
						
