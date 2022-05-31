#include "Startup/app.h"
#include "Libs/widget/elements/elements.h"
#include "Libs/window/window.h"
#include "Libs/widget/graph/graph.h"
#include "Libs/widget/widget.h"

/*
* Bar element
  - Width
  - Height
  - Orientation (Left->right, top->bottom, bottom->top, right->left)
  - Translucency
  - Draw color
  - Shadow color
  - 3D Bars
    - Shadow position (upper right, upper left, lower right, lower left)
    - Shadow size
    - Shadow intensity
*/

typedef struct STriangleStep
{
	BOOL bInvertXSize;
	BOOL bInvertStartingPixel;
	INT8 s8StepXDir;
} STriangleStep;

static const STriangleStep sg_sStepTable[] =
{
	{FALSE},
	{FALSE, TRUE,	1},	// EORIENT_UPPER_RIGHT
	{FALSE, FALSE,	0}, // EORIENT_UPPER_LEFT
	{TRUE,	TRUE,	-1}, // EORIENT_LOWER_RIGHT
	{TRUE,	FALSE,	0}	// EORIENT_LOWER_LEFT
};

static void BarDrawTriangle(SWindow *psWindow,
							SBar *psBar,
							INT32 s32XPos,
							INT32 s32YPos,
							EElementOrient eOrientation)
{
	INT32 s32MaxXSize = 0;
	INT32 s32MaxYSize = 0;
	INT32 s32XRightMax = 0;
	INT32 s32YBottomMax = 0;
	INT8 s8Step = 0;
	UINT16 *pu16PixelPtr = NULL;
	const STriangleStep *psTriangleStep;
	UINT8 u8ShadowSize = psBar->u8ShadowSize;
	UINT8 u8Translucency = psBar->u8SideShadowTranslucency;
	INT32 s32XStep = -1;	// Assume upper left
	INT32 s32YStep = -1;	// Assume upper

	// EORIENT_UPPER_LEFT:
	//
	// |--
	// | /
	// |/

	// EORIENT_UPPER_RIGHT:
	//
	//  /|
	// / |
	// --|

	// EORIENT_LOWER_LEFT:
	//
	// |--
	// | /
	// |/

	// EORIENT_LOWER_RIGHT:
	//
	// --|
	// \ |
	//  \|

	if ((psBar->u8SideShadowTranslucency < 0x04) ||
		(0 == u8ShadowSize))
	{
		// Transparent - return
		return;
	}

	s32MaxXSize = (INT32) u8ShadowSize;
	s32MaxYSize = (INT32) u8ShadowSize;
	s8Step = (INT8) u8ShadowSize;

	s32XRightMax = (INT32) (psWindow->u32ActiveAreaXPos + psWindow->u32ActiveAreaXSize);
	s32YBottomMax = (INT32) (psWindow->u32ActiveAreaYPos + psWindow->u32ActiveAreaYSize);

	// Clip X and Y
	if ((s32XPos + s32MaxXSize) > s32XRightMax)
	{
		s32MaxXSize = s32XRightMax - (s32XPos - s32MaxXSize);
	}

	if ((s32YPos + s32MaxYSize) > s32YBottomMax)
	{
		s32MaxYSize = s32YBottomMax - (s32YPos - s32MaxYSize);
	}

	// If either are 0 or negative, then bail out, because there's nothing to draw
	if ((s32MaxXSize < 1) || (s32MaxYSize < 1))
	{
		return;
	}

	// Point to where things go
	pu16PixelPtr = psWindow->psWindowImage->pu16ImageData + 
				   s32XPos + (s32YPos * psWindow->psWindowImage->u32Pitch);

	// Figure out which triangle stepping params we're using
	psTriangleStep = &sg_sStepTable[eOrientation];

	while (s32MaxYSize > 0)
	{
		UINT16 *pu16PixelPtrTemp;
		INT32 s32XSize = 0;
		UINT16 u16Color;

		pu16PixelPtrTemp = pu16PixelPtr;
		s32XSize = s8Step;
		if (psTriangleStep->bInvertXSize)
		{
			s32XSize = u8ShadowSize - s8Step;
		}

		if ((s32XPos + s32XSize) > s32XRightMax)
		{
			s32XSize = s32XRightMax - s32XPos;
		}

		if (psTriangleStep->bInvertStartingPixel)
		{
			pu16PixelPtrTemp += ((INT8) u8ShadowSize - s32XSize);
		}

		// Only draw if we have something to draw
		if (s32XSize > 0)
		{
			// pu16PixelPtrTemp
			// Get our draw color here in case we need to modify it
			u16Color = psBar->u16SideShadowColor;

			if (psBar->u8SideShadowTranslucency >= 0xfc)
			{
				INT32 s32Count;

				// Solid! Draw every pixel

				s32Count = s32XSize;

				while (s32Count--)
				{
					*pu16PixelPtrTemp = u16Color;
					pu16PixelPtrTemp++;
				}
			}
			else
			{
				UINT8 u8Red;
				UINT8 u8Green;
				UINT8 u8Blue;
				UINT8 u8RedSrc;
				UINT8 u8GreenSrc;
				UINT8 u8BlueSrc;
				UINT8 u8Translucency = psBar->u8SideShadowTranslucency;
				INT32 s32Count;

				// Not solid. Translucency. Yucko. First, tear the color apart, and put it back together
				// with the transparency we want.

				// Break apart the source colors and preadjust the translucency
				u8RedSrc = ((u16Color >> 11) * u8Translucency) >> 8;
				GCASSERT(u8RedSrc < 0x20);
				u8GreenSrc = (((u16Color >> 5) & 0x3f) * u8Translucency) >> 8;
				GCASSERT(u8GreenSrc < 0x40);
				u8BlueSrc = ((u16Color & 0x1f) * u8Translucency) >> 8;
				GCASSERT(u8BlueSrc < 0x20);

				// Invert the translucency for the target
				u8Translucency = 0xff - u8Translucency;
				s32Count = s32XSize;

				while (s32Count--)
				{
					UINT16 u16Src;

					u16Src = *pu16PixelPtrTemp;

					u8Red = (((u16Src >> 11) * u8Translucency) >> 8) + u8RedSrc;
					u8Green = ((((u16Src >> 5) & 0x3f) * u8Translucency) >> 8) + u8GreenSrc;
					u8Blue = (((u16Src & 0x1f) * u8Translucency) >> 8) + u8BlueSrc;

					*pu16PixelPtrTemp = sg_u16RedGradientSaturation[u8Red] |
									sg_u16GreenGradientSaturation[u8Green] |
									sg_u16BlueGradientSaturation[u8Blue];
					pu16PixelPtrTemp++;
				}
			}
		}

		pu16PixelPtr += psWindow->psWindowImage->u32Pitch;
		s32XPos += psTriangleStep->s8StepXDir;
		s32MaxYSize--;
		s8Step--;
	}
}

void BarDrawRectangleWithImage(SWindow *psWindow,
							   UINT8 u8OriginalTranslucency,
							   UINT16 u16Color,
							   INT32 s32XPosTemp,
							   INT32 s32YPosTemp,
							   INT32 s32XSize,
							   INT32 s32YSize,
							   SGraphSeries *psGraphSeries)
{
	UINT16 *pu16PixelPtr = NULL;
	UINT16 *pu16SrcPtr = NULL;
	SGraphWidget *psGraph = NULL;
	INT32 s32XPosSrcNormalized;
	INT32 s32YPosSrcNormalized;
	INT32 s32XImageSize = 0;
	INT32 s32XFillSize = 0;
	INT32 s32YImageSize = 0;
	INT32 s32YFillSize = 0;

	if (psGraphSeries)
	{
		SWidget *psWidget = NULL;
		ELCDErr eErr;

		eErr = WidgetGetPointerByHandle((WIDGETHANDLE) psGraphSeries->eGraphHandle,
										WIDGET_GRAPH,
										&psWidget,
										NULL);
		GCASSERT(LERR_OK == eErr);

		psGraph = psWidget->uWidgetSpecific.psGraphWidget;
		GCASSERT(psGraph);
	}

	// Figure out where we're supposed to draw
	pu16PixelPtr = psWindow->psWindowImage->pu16ImageData + 
				   s32XPosTemp + ((INT32) psWindow->u32ActiveAreaXPos) +
				   ((s32YPosTemp + (INT32) psWindow->u32ActiveAreaYPos) * psWindow->psWindowImage->u32Pitch);

	// Now figure out where the source image is
	s32XPosSrcNormalized = s32XPosTemp;
	GCASSERT(s32XPosSrcNormalized >= 0);
	s32YPosSrcNormalized = s32YPosTemp;
	GCASSERT(s32YPosSrcNormalized >= 0);

	// Go look up the main graph image for x/y offset now that s32XPosSrcNormalized
	// and s32YPosSrcNormalized are in 

	if (psGraphSeries && psGraphSeries->psSeriesImage)
	{
		pu16SrcPtr = psGraphSeries->psSeriesImage->psCurrentImage->pu16ImageData +
					 s32XPosSrcNormalized + (s32YPosSrcNormalized * psGraphSeries->psSeriesImage->psCurrentImage->u32Pitch);

		s32XImageSize = (INT32) psGraphSeries->psSeriesImage->psCurrentImage->u32XSize - s32XPosSrcNormalized;
		s32YImageSize = (INT32) psGraphSeries->psSeriesImage->psCurrentImage->u32YSize - s32YPosSrcNormalized;

		// See if we have too much, and if so, we back off on it
		if (s32XImageSize > s32XSize)
		{
			// It's the full image width - go for it
			s32XImageSize = s32XSize;
			s32XFillSize = 0;
		}
		else
		{
			if (s32XImageSize <= 0)
			{
				// Indicates no image - just do the fill
				s32XImageSize = 0;
				s32XFillSize = s32XSize;
			}
			else
			{
				// Indicates partial image.
				s32XFillSize = s32XSize - s32XImageSize;
			}
		}

		// And try the Y size, too
		if (s32YImageSize > s32YSize)
		{
			// It's the full image length - go for it
			s32YImageSize = s32YSize;
			s32YFillSize = 0;
		}
		else
		{
			if (s32YImageSize <= 0)
			{
				// Indicates no image - just do the fill
				s32YImageSize = 0;
				s32YFillSize = s32YSize;
			}
			else
			{
				// Indicates partial image.
				s32YFillSize = s32YSize - s32YImageSize;
			}
		}
	}

	/***************** DRAW THE PICTURE ***********************/
	if ((s32XImageSize > 0) && (s32YImageSize > 0))
	{
		INT32 s32ImageLoop = s32YImageSize;
		INT32 s32Count = 0;

		while (s32ImageLoop--)
		{
			memcpy((void *) pu16PixelPtr, pu16SrcPtr, s32XImageSize << 1);
			if (s32XFillSize > 0)
			{
				UINT16 *pu16PixelTmp = pu16PixelPtr + s32XImageSize;

				if (u8OriginalTranslucency >= 0xfc)
				{
					// Solid! Draw every pixel
					s32Count = s32XFillSize;
					while (s32Count--)
					{
						*pu16PixelTmp = u16Color;
						pu16PixelTmp++;
					}
				}
				else
				{
					UINT8 u8Red;
					UINT8 u8Green;
					UINT8 u8Blue;
					UINT8 u8RedSrc;
					UINT8 u8GreenSrc;
					UINT8 u8BlueSrc;
					UINT8 u8Translucency = u8OriginalTranslucency;

					// Not solid. Translucency. Yucko. First, tear the color apart, and put it back together
					// with the transparency we want.

					// Break apart the source colors and preadjust the translucency
					u8RedSrc = ((u16Color >> 11) * u8Translucency) >> 8;
					GCASSERT(u8RedSrc < 0x20);
					u8GreenSrc = (((u16Color >> 5) & 0x3f) * u8Translucency) >> 8;
					GCASSERT(u8GreenSrc < 0x40);
					u8BlueSrc = ((u16Color & 0x1f) * u8Translucency) >> 8;
					GCASSERT(u8BlueSrc < 0x20);

					// Invert the translucency for the target
					u8Translucency = 0xff - u8Translucency;

					s32Count = s32XSize;

					while (s32Count--)
					{
						UINT16 u16Src;

						u16Src = *pu16PixelTmp;

						u8Red = (((u16Src >> 11) * u8Translucency) >> 8) + u8RedSrc;
						u8Green = ((((u16Src >> 5) & 0x3f) * u8Translucency) >> 8) + u8GreenSrc;
						u8Blue = (((u16Src & 0x1f) * u8Translucency) >> 8) + u8BlueSrc;

						*pu16PixelTmp = sg_u16RedGradientSaturation[u8Red] |
										sg_u16GreenGradientSaturation[u8Green] |
										sg_u16BlueGradientSaturation[u8Blue];
						pu16PixelTmp++;
					}
				}
			}

			pu16PixelPtr += psWindow->psWindowImage->u32Pitch;
			pu16SrcPtr += psGraphSeries->psSeriesImage->psCurrentImage->u32Pitch;
		}

		s32YSize = s32YFillSize;
	}

	/***************** DRAW THE SOLIDS ***********************/
	if ((s32XSize > 0) && (s32YSize > 0))
	{
		if (u8OriginalTranslucency >= 0xfc)
		{
			// Solid! Draw every pixel
			while (s32YSize)
			{
				INT32 s32Count;

				s32Count = s32XSize;

				while (s32Count--)
				{
					*pu16PixelPtr = u16Color;
					pu16PixelPtr++;
				}

				pu16PixelPtr += (psWindow->psWindowImage->u32Pitch - s32XSize);
				--s32YSize;
			}
		}
		else
		{
			UINT8 u8Red;
			UINT8 u8Green;
			UINT8 u8Blue;
			UINT8 u8RedSrc;
			UINT8 u8GreenSrc;
			UINT8 u8BlueSrc;
			UINT8 u8Translucency = u8OriginalTranslucency;

			// Not solid. Translucency. Yucko. First, tear the color apart, and put it back together
			// with the transparency we want.

			// Break apart the source colors and preadjust the translucency
			u8RedSrc = ((u16Color >> 11) * u8Translucency) >> 8;
			GCASSERT(u8RedSrc < 0x20);
			u8GreenSrc = (((u16Color >> 5) & 0x3f) * u8Translucency) >> 8;
			GCASSERT(u8GreenSrc < 0x40);
			u8BlueSrc = ((u16Color & 0x1f) * u8Translucency) >> 8;
			GCASSERT(u8BlueSrc < 0x20);

			// Invert the translucency for the target
			u8Translucency = 0xff - u8Translucency;

			while (s32YSize)
			{
				INT32 s32Count;

				s32Count = s32XSize;

				while (s32Count--)
				{
					UINT16 u16Src;

					u16Src = *pu16PixelPtr;

					u8Red = (((u16Src >> 11) * u8Translucency) >> 8) + u8RedSrc;
					u8Green = ((((u16Src >> 5) & 0x3f) * u8Translucency) >> 8) + u8GreenSrc;
					u8Blue = (((u16Src & 0x1f) * u8Translucency) >> 8) + u8BlueSrc;

					*pu16PixelPtr = sg_u16RedGradientSaturation[u8Red] |
									sg_u16GreenGradientSaturation[u8Green] |
									sg_u16BlueGradientSaturation[u8Blue];
					pu16PixelPtr++;
				}

				pu16PixelPtr += (psWindow->psWindowImage->u32Pitch - s32XSize);
				--s32YSize;
			}
		}
	}
}

void BarDraw(WINDOWHANDLE eWindow,
			 struct SGfxElement *psElement,
			 struct SGfxElement *psPriorElement,
			 SGraphSeries *psGraphSeries)
{
	UINT16 *pu16PixelPtr = NULL;
	INT32 s32XPosTemp = 0;
	INT32 s32YPosTemp = 0;
	INT32 s32XRightMax = 0;
	INT32 s32YBottomMax = 0;
	INT32 s32XSize = 0;
	INT32 s32YSize = 0;
	SBar *psBar = &psElement->uEData.sBar;
	SWindow *psWindow = NULL;

	psWindow = WindowGetPointer(eWindow);
	GCASSERT(psWindow);

	// ***** DRAW THE BAR

	// Compute our literal position within the window
	s32XPosTemp = psElement->s32XPos;
	s32YPosTemp = psElement->s32YPos;

	// If we have an upper left or right, add in the Y size of the shadow
	if ((EORIENT_UPPER_LEFT == psBar->eOrientation) ||
		(EORIENT_UPPER_RIGHT == psBar->eOrientation))
	{
		s32YPosTemp += (INT32) psBar->u8ShadowSize;
	}

	// Upper left or lower left, add in the X size of the shadow
	if ((EORIENT_UPPER_LEFT == psBar->eOrientation) ||
		(EORIENT_LOWER_LEFT == psBar->eOrientation))
	{
		s32XPosTemp += (INT32) psBar->u8ShadowSize;
	}

	// Compute right and bottom
	s32XRightMax = (INT32) (psWindow->u32ActiveAreaXPos + psWindow->u32ActiveAreaXSize);
	s32YBottomMax = (INT32) (psWindow->u32ActiveAreaYPos + psWindow->u32ActiveAreaYSize);

	// Figure out our X/Y size
	s32XSize = psBar->s32XSize;
	if ((s32XPosTemp + s32XSize) > s32XRightMax)
	{
		s32XSize = s32XRightMax - s32XPosTemp;
	}

	s32YSize = psBar->s32YSize;
	if ((s32YPosTemp + s32YSize) > s32YBottomMax)
	{
		s32YSize = s32YBottomMax - s32YPosTemp;
	}

	// Only draw the box if we have something to draw
	if ((s32XSize > 0) && (s32YSize > 0) && (psElement->u8DrawTranslucency >= 0x04))
	{
		// If we have a series image, let's draw the texture map on top of the solid
		// bar part.

		BarDrawRectangleWithImage(psWindow,
								  psElement->u8DrawTranslucency,
								  psElement->u16DrawColor,
								  s32XPosTemp,
								  s32YPosTemp,
								  s32XSize,
								  s32YSize,
								  psGraphSeries);
	}

	// ***** DRAW THE TOP/BOTTOM SHADOW
	if ((psBar->u8ShadowSize) && (psBar->u8TopShadowTranslucency >= 0x04))
	{
		UINT8 u8RedSrc;
		UINT8 u8GreenSrc;
		UINT8 u8BlueSrc;
		UINT8 u8Translucency = psBar->u8TopShadowTranslucency;
		INT32 s32XStep = -1;	// Assume upper left
		INT32 s32YStep = -1;	// Assume upper
		UINT8 u8ShadowSize = psBar->u8ShadowSize;
		INT32 s32OriginalXSize;
		INT32 s32XSizeBoost = 0;

		// Break apart the source colors and preadjust the translucency
		u8RedSrc = ((psBar->u16TopShadowColor >> 11) * u8Translucency) >> 8;
		GCASSERT(u8RedSrc < 0x20);
		u8GreenSrc = (((psBar->u16TopShadowColor >> 5) & 0x3f) * u8Translucency) >> 8;
		GCASSERT(u8GreenSrc < 0x40);
		u8BlueSrc = ((psBar->u16TopShadowColor & 0x1f) * u8Translucency) >> 8;
		GCASSERT(u8BlueSrc < 0x20);

		// Figure out a Y position
		s32XPosTemp = psElement->s32XPos + psWindow->u32ActiveAreaXPos;
		s32YPosTemp = psElement->s32YPos + psWindow->u32ActiveAreaYPos;

		if ((EORIENT_UPPER_RIGHT == psBar->eOrientation) ||
			(EORIENT_LOWER_RIGHT == psBar->eOrientation))
		{
			s32XStep = 1;
		}
		else
		{
			// Left side - need to move the shadow
			s32XPosTemp += psBar->u8ShadowSize;
		}

		if ((EORIENT_UPPER_LEFT == psBar->eOrientation) ||
			(EORIENT_UPPER_RIGHT == psBar->eOrientation))
		{
			s32YPosTemp += psBar->u8ShadowSize;
			if (EORIENT_UPPER_LEFT == psBar->eOrientation)
			{
				s32XPosTemp--;
			}

			s32XSizeBoost = 1;
		}
		else
		{
			// Adjust the position to the bottom
			s32YPosTemp += (psBar->s32YSize - 1);
			s32YStep = 1;
		}

		s32YPosTemp += s32YStep;

		// Compute the start of the location where we start drawing
		pu16PixelPtr = psWindow->psWindowImage->pu16ImageData + 
					   s32XPosTemp + (s32YPosTemp * psWindow->psWindowImage->u32Pitch);

		// Clip the shadow size
		s32YSize = (INT32) psBar->u8ShadowSize;

		// Draw the slanted top/bottom
		while (s32YSize > 0)
		{
			// Figure out how long our line is (from left to right) and clip it
			s32XSize = psBar->s32XSize + s32XSizeBoost;

			if ((s32XPosTemp + s32XSize) > s32XRightMax)
			{
				s32XSize = s32XRightMax - s32XPosTemp;
			}

			s32OriginalXSize = s32XSize;

			if (psBar->u8TopShadowTranslucency >= 0xfc)
			{
				// Solid! Draw every pixel
				while (s32XSize)
				{
					*pu16PixelPtr = psBar->u16TopShadowColor;
					pu16PixelPtr++;
					s32XSize--;
				}
			}
			else
			{
				UINT8 u8Red;
				UINT8 u8Green;
				UINT8 u8Blue;

				// Not solid. Translucency. Yucko. First, tear the color apart, and put it back together
				// with the transparency we want.

				// Invert the translucency for the target
				u8Translucency = 0xff - psBar->u8TopShadowTranslucency;

				while (s32XSize)
				{
					UINT16 u16Src;

					u16Src = *pu16PixelPtr;

					u8Red = (((u16Src >> 11) * u8Translucency) >> 8) + u8RedSrc;
					u8Green = ((((u16Src >> 5) & 0x3f) * u8Translucency) >> 8) + u8GreenSrc;
					u8Blue = (((u16Src & 0x1f) * u8Translucency) >> 8) + u8BlueSrc;

					*pu16PixelPtr = sg_u16RedGradientSaturation[u8Red] |
									sg_u16GreenGradientSaturation[u8Green] |
									sg_u16BlueGradientSaturation[u8Blue];
					pu16PixelPtr++;
					s32XSize--;
				}
			}

			s32XPosTemp += s32XStep;
			pu16PixelPtr = ((pu16PixelPtr + ((INT32) psWindow->psWindowImage->u32Pitch * s32YStep)) - s32OriginalXSize) + s32XStep;
			s32YSize--;
		}
	}

	// ***** DRAW THE LEFT/RIGHT SHADOW
	if ((psBar->u8ShadowSize) && (psBar->u8SideShadowTranslucency >= 0x04))
	{
		s32XPosTemp = psElement->s32XPos + psWindow->u32ActiveAreaXPos;
		s32YPosTemp = psElement->s32YPos + psWindow->u32ActiveAreaYPos;

		if (EORIENT_UPPER_RIGHT == psBar->eOrientation)
		{
			s32XPosTemp += psBar->s32XSize;
			BarDrawTriangle(psWindow, psBar, s32XPosTemp, s32YPosTemp, EORIENT_LOWER_RIGHT);
			BarDrawTriangle(psWindow, psBar, s32XPosTemp, (s32YPosTemp + psBar->s32YSize), EORIENT_UPPER_LEFT);
			s32YPosTemp += psBar->u8ShadowSize;
		}

		if (EORIENT_UPPER_LEFT == psBar->eOrientation)
		{		
			BarDrawTriangle(psWindow, psBar, s32XPosTemp, s32YPosTemp, EORIENT_LOWER_LEFT);
			BarDrawTriangle(psWindow, psBar, s32XPosTemp, (s32YPosTemp + psBar->s32YSize), EORIENT_UPPER_RIGHT);
			s32YPosTemp += psBar->u8ShadowSize;
		}

		if (EORIENT_LOWER_LEFT == psBar->eOrientation)
		{
			BarDrawTriangle(psWindow, psBar, s32XPosTemp, s32YPosTemp, EORIENT_LOWER_RIGHT);
			BarDrawTriangle(psWindow, psBar, s32XPosTemp, (s32YPosTemp + psBar->s32YSize), EORIENT_UPPER_LEFT);
			s32YPosTemp += psBar->u8ShadowSize;
		}

		if (EORIENT_LOWER_RIGHT == psBar->eOrientation)
		{
			s32XPosTemp += psBar->s32XSize;
			BarDrawTriangle(psWindow, psBar, s32XPosTemp, s32YPosTemp, EORIENT_LOWER_LEFT);
			BarDrawTriangle(psWindow, psBar, s32XPosTemp, (s32YPosTemp + psBar->s32YSize), EORIENT_UPPER_RIGHT);
			s32YPosTemp += psBar->u8ShadowSize;
		}

		BarDrawRectangleWithImage(psWindow,
								  psBar->u8SideShadowTranslucency,
								  psBar->u16SideShadowColor,
								  s32XPosTemp - psWindow->u32ActiveAreaXPos,
								  s32YPosTemp - psWindow->u32ActiveAreaYPos,
								  psBar->u8ShadowSize,
								  psBar->s32YSize - psBar->u8ShadowSize,
								  NULL);
	}
}

static void BarGetSize(SGfxElement *psElement,
					   UINT32 *pu32XSize,
					   UINT32 *pu32YSize)
{
	*pu32XSize = (UINT32) (psElement->uEData.sBar.s32XSize + psElement->uEData.sBar.u8ShadowSize);
	*pu32YSize = (UINT32) (psElement->uEData.sBar.s32YSize + psElement->uEData.sBar.u8ShadowSize);
}

static SElementFuncs sg_sBarFunctions =
{
	NULL,
	BarDraw,
	NULL,
	BarGetSize
};

void BarInit(void)
{
	// Register the line type
	ElementRegister(ELEMTYPE_BAR,
					&sg_sBarFunctions);
}
