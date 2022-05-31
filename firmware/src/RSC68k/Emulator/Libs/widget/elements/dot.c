#include <stdio.h>
#include <math.h>
#include "Startup/app.h"
#include "Libs/widget/elements/elements.h"
#include "Libs/window/window.h"
#include "Libs/widget/graph/graph.h"

void DotDraw(WINDOWHANDLE eWindow,
			 SGfxElement *psDotElement,
			 SGfxElement *psPriorElement,
			 SGraphSeries *psGraphSeries)
{
	SWindow *psWindow = NULL;
	UINT16 *pu16PixelPtr;
	UINT16 u16DotToDraw;

	psWindow = WindowGetPointer(eWindow);
	GCASSERT(psWindow);

	// Clip
	if ((psDotElement->s32XPos < (INT32) psWindow->u32ActiveAreaXPos) ||
		(psDotElement->s32YPos < (INT32) psWindow->u32ActiveAreaYPos))
	{
		// Violation of left or top window side - just exit
		return;
	}

	if ((psDotElement->s32XPos >= ((INT32) psWindow->u32ActiveAreaXPos + (INT32) psWindow->u32ActiveAreaXSize)) ||
		(psDotElement->s32YPos >= ((INT32) psWindow->u32ActiveAreaYPos + (INT32) psWindow->u32ActiveAreaYSize)))
	{
		// Violation of right or bottom of window side - just exit
		return;
	}

	// Get our dot color
	u16DotToDraw = psDotElement->u16DrawColor;

	// Get our pixel address
	pu16PixelPtr = psWindow->psWindowImage->pu16ImageData + 
				   psDotElement->s32XPos + (psDotElement->s32YPos * psWindow->psWindowImage->u32Pitch);

	// If we have a group image, let's try to draw from it if we can
	if (psGraphSeries->psSeriesImage)
	{
		INT32 s32XPosSrcNormalized;
		INT32 s32YPosSrcNormalized;

		// Now figure out where the source image is
		s32XPosSrcNormalized = (psDotElement->s32XPos - (INT32) psWindow->u32ActiveAreaXPos);
		GCASSERT(s32XPosSrcNormalized >= 0);
		s32YPosSrcNormalized = (psDotElement->s32YPos - (INT32) psWindow->u32ActiveAreaYPos);
		GCASSERT(s32YPosSrcNormalized >= 0);

		if (((UINT32) s32XPosSrcNormalized >= psGraphSeries->psSeriesImage->psCurrentImage->u32XSize) ||
			((UINT32) s32YPosSrcNormalized >= psGraphSeries->psSeriesImage->psCurrentImage->u32YSize))
		{
			// Outside the range of the picture.
		}
		else
		{
			// Pull the dot out of the image
			u16DotToDraw = *(psGraphSeries->psSeriesImage->psCurrentImage->pu16ImageData +
							 s32XPosSrcNormalized + 
							 (s32YPosSrcNormalized * psGraphSeries->psSeriesImage->psCurrentImage->u32Pitch));
		}
	}

	if (psDotElement->u8DrawTranslucency >= 0xfc)
	{
		// Solid
		*pu16PixelPtr = u16DotToDraw;
	}
	else
	if (psDotElement->u8DrawTranslucency < 4)
	{
		// Transparent - don't draw anything
	}
	else
	{
		UINT8 u8RedSrc;
		UINT8 u8GreenSrc;
		UINT8 u8BlueSrc;

		// Yucky. Translucent time.

		u8RedSrc = u16DotToDraw >> 11; 
		u8GreenSrc = u16DotToDraw & 0x3f; 
		u8BlueSrc = u16DotToDraw & 0x1f;

		DRAW_PIXEL(pu16PixelPtr, psDotElement->u8DrawTranslucency);
	}
}

static void DotGetSize(SGfxElement *psElement,
					   UINT32 *pu32XSize,
					   UINT32 *pu32YSize)
{
	// It's a dot!
	*pu32XSize = 1;
	*pu32YSize = 1;
}

static SElementFuncs sg_sDotFunctions =
{
	NULL,
	DotDraw,
	NULL,
	DotGetSize
};

void DotInit(void)
{
	// Register the line type
	ElementRegister(ELEMTYPE_DOT,
					&sg_sDotFunctions);
}

