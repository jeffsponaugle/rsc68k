#include <stdio.h>
#include <math.h>
#include "Startup/app.h"
#include "Libs/widget/elements/elements.h"
#include "Libs/widget/graph/graph.h"

static UINT8 sg_u8ConeWeightingCurve[1 << 8];

#define		FIXED_POINT_FRACTION		24

#define		TAN_RANGE	2.0

void LineDrawAA(WINDOWHANDLE eWindow,
			    SGfxElement *psLineElement,
				SGfxElement *psPriorElement,
				SGraphSeries *psGraphSeries)
{
	UINT16 *pu16Addr;
	INT32 s32DistanceX;
	INT32 s32DistanceY;
	INT32 s32StraightStep;
	INT32 s32DrawStep;
	INT32 s32AngleStep;
	INT32 s32StepAccumulator = 0;
	INT32 s32Step = 0;
	UINT32 u32PixelCount = 0;
	UINT8 u8RedSrc, u8GreenSrc, u8BlueSrc;
	SLine *psLine;
	SWindow *psWindow;

	if (NULL == psPriorElement)
	{
		// Don't draw squat
		return;
	}

	psWindow = WindowGetPointer(eWindow);
	GCASSERT(psWindow);

	// Point to our line
	psLine = &psLineElement->uEData.sLine;

	psLine->s32X0 = psPriorElement->s32XPos;
	psLine->s32Y0 = psPriorElement->s32YPos;
	psLine->s32X1 = psLineElement->s32XPos;
	psLine->s32Y1 = psLineElement->s32YPos;

	u8RedSrc = psLine->u16DrawColor >> 11;
	u8GreenSrc = (psLine->u16DrawColor >> 5) & 0x3f;
	u8BlueSrc = psLine->u16DrawColor & 0x1f;

	// Fix the starting address
	pu16Addr = psWindow->psWindowImage->pu16ImageData + (psLine->s32X0 + (INT32) psWindow->u32ActiveAreaXPos) + 
				((psLine->s32Y0 + (INT32) psWindow->u32ActiveAreaYPos) * psWindow->psWindowImage->u32Pitch);

	// Now figure out the slope
	s32DistanceX = psLine->s32X1 - psLine->s32X0;
	s32DistanceY = psLine->s32Y1 - psLine->s32Y0;

	if (abs(s32DistanceX) > abs(s32DistanceY))
	{
		// This means it's row major (step by X, Y is fractional)

		if (s32DistanceX < 0)
		{
			s32StraightStep = -1;
			s32DrawStep = -((INT32) psWindow->psWindowImage->u32Pitch);
		}
		else
		{
			s32StraightStep = 1;
			s32DrawStep = psWindow->psWindowImage->u32Pitch;
		}

		if (s32DistanceY < 0)
		{
			s32AngleStep = -((INT32) psWindow->psWindowImage->u32Pitch);
		}
		else
		{
			s32AngleStep = psWindow->psWindowImage->u32Pitch;
		}

		if (0 == s32DistanceX)
		{
			s32Step = 0;
		}
		else
		{
			s32Step = (s32DistanceY << FIXED_POINT_FRACTION) / s32DistanceX;
		}

		u32PixelCount = (UINT32) abs(s32DistanceX);
	}
	else
	{
		// This means it's column major (step by Y, X is fractional)

		if (s32DistanceY < 0)
		{
			s32StraightStep = -((INT32) psWindow->psWindowImage->u32Pitch);
			s32DrawStep = -1;
		}
		else
		{
			s32StraightStep = psWindow->psWindowImage->u32Pitch;
			s32DrawStep = 1;
		}

		if (s32DistanceX < 0)
		{
			s32AngleStep = -1;
		}
		else
		{
			s32AngleStep = 1;
		}

		if (0 == s32DistanceY)
		{
			s32Step = 0;
		}
		else
		{
			s32Step = (s32DistanceX << FIXED_POINT_FRACTION) / s32DistanceY;
		}

		u32PixelCount = (UINT32) abs(s32DistanceY);
	}

// Roughly 10%
#define		PRIMARY_REDUCTION_MULTIPLIER		((UINT16) (255.0 * 0.9))

	if (psGraphSeries->psSeriesImage)
	{
		// This will draw from a series image
		UINT8 u8RedSrcRef = psLine->u16DrawColor >> 11;
		UINT8 u8GreenSrcRef = (psLine->u16DrawColor >> 5) & 0x3f;
		UINT8 u8BlueSrcRef = psLine->u16DrawColor & 0x1f;
		INT32 s32ImgXPos = 0;
		INT32 s32ImgYPos = 0;
		UINT16 *pu16ImageSrc = psGraphSeries->psSeriesImage->psCurrentImage->pu16ImageData;

		s32ImgXPos = psLine->s32X0 - psGraphSeries->s32XOffset;
		s32ImgYPos = psLine->s32Y0 - psGraphSeries->s32YOffset;

		// Align the start of our image source
		pu16ImageSrc += s32ImgXPos + (s32ImgYPos * psGraphSeries->psSeriesImage->psCurrentImage->u32Pitch);

		while (u32PixelCount)
		{
			UINT8 u8PrimaryIntensity;
			UINT8 u8PrimaryRemainder;
			UINT8 u8NeighborLeftIntensity;
			UINT8 u8NeighborRightIntensity;
			UINT16 u16StepProc = 0;

			// s32StepAccumulator - closer to 0==Higher intensity

			u8PrimaryIntensity = ((0xff - (abs(s32StepAccumulator) >> 16)) * PRIMARY_REDUCTION_MULTIPLIER) >> 8;

			// Start off assuming we're going to be doing 
			u8RedSrc = u8RedSrcRef; 
			u8GreenSrc = u8GreenSrcRef; 
			u8BlueSrc = u8BlueSrcRef;

			if ((s32ImgXPos >= 0) && (s32ImgYPos >= 0) &&
				(s32ImgXPos < (INT32) psGraphSeries->psSeriesImage->psCurrentImage->u32XSize) &&
				(s32ImgYPos < (INT32) psGraphSeries->psSeriesImage->psCurrentImage->u32YSize))
			{
				// We're within range! Pull out the pixel
				u8RedSrc = *pu16ImageSrc >> 11;
				u8GreenSrc = (*pu16ImageSrc >> 5) & 0x3f;
				u8BlueSrc =  *pu16ImageSrc & 0x1f;
			}

			DRAW_PIXEL(pu16Addr, u8PrimaryIntensity);

			// This is what's left over
			u8PrimaryRemainder = 0xff - u8PrimaryIntensity;

			u16StepProc = (((INT16) ((INT32) s32StepAccumulator >> 16)) + 256) >> 1;

			u8NeighborRightIntensity = (u8PrimaryRemainder * u16StepProc) >> 8;
			u8NeighborLeftIntensity = (u8PrimaryRemainder * (0xff - u16StepProc)) >> 8;

	//		DebugOut("%.8x, %4d, intensity=0x%.2x, left=0x%.2x, right=0x%.2x, all=%d\n", s32StepAccumulator, u16StepProc, u8PrimaryIntensity, u8NeighborLeftIntensity, u8NeighborRightIntensity, u8PrimaryIntensity + u8NeighborLeftIntensity + u8NeighborRightIntensity);

			DRAW_PIXEL(pu16Addr + s32DrawStep, sg_u8ConeWeightingCurve[u8NeighborRightIntensity]);
			DRAW_PIXEL(pu16Addr - s32DrawStep, sg_u8ConeWeightingCurve[u8NeighborLeftIntensity]);

	//		DRAW_PIXEL(pu16Addr + s32DrawStep, u8NeighborRightIntensity);
	//		DRAW_PIXEL(pu16Addr - s32DrawStep, u8NeighborLeftIntensity);

			s32StepAccumulator += s32Step;
			while (s32StepAccumulator >= (1 << FIXED_POINT_FRACTION))
			{
				s32StepAccumulator -= (1 << FIXED_POINT_FRACTION);
				pu16Addr += s32AngleStep;
				if ((-1 == s32AngleStep) || (1 == s32AngleStep))
				{
					pu16ImageSrc += s32AngleStep;
					s32ImgXPos += s32AngleStep;
				}
				else
				if (s32AngleStep < 0)
				{
					// Negative pitch
					s32ImgYPos--;
					pu16ImageSrc -= psGraphSeries->psSeriesImage->psCurrentImage->u32Pitch;
				}
				else
				{
					// Positive pitch
					s32ImgYPos++;
					pu16ImageSrc += psGraphSeries->psSeriesImage->psCurrentImage->u32Pitch;
				}
			}

			while (s32StepAccumulator <= -(1 << FIXED_POINT_FRACTION))
			{
				s32StepAccumulator += (1 << FIXED_POINT_FRACTION);
				pu16Addr += s32AngleStep;
				if ((-1 == s32AngleStep) || (1 == s32AngleStep))
				{
					pu16ImageSrc += s32AngleStep;
					s32ImgXPos += s32AngleStep;
				}
				else
				if (s32AngleStep < 0)
				{
					// Negative pitch
					s32ImgYPos--;
					pu16ImageSrc -= psGraphSeries->psSeriesImage->psCurrentImage->u32Pitch;
				}
				else
				{
					// Positive pitch
					s32ImgYPos++;
					pu16ImageSrc += psGraphSeries->psSeriesImage->psCurrentImage->u32Pitch;
				}
			}

			pu16Addr += s32StraightStep;
			if ((-1 == s32StraightStep) || (1 == s32StraightStep))
			{
				pu16ImageSrc += s32StraightStep;
				s32ImgXPos += s32StraightStep;
			}
			else
			if (s32StraightStep < 0)
			{
				// Negative pitch
				s32ImgYPos--;
				pu16ImageSrc -= psGraphSeries->psSeriesImage->psCurrentImage->u32Pitch;
			}
			else
			{
				// Positive pitch
				s32ImgYPos++;
				pu16ImageSrc += psGraphSeries->psSeriesImage->psCurrentImage->u32Pitch;
			}

			--u32PixelCount;
		}
	}
	else
	{
		// This is the linedraw version that also steps the source pixel from
		// the provided image

		while (u32PixelCount)
		{
			UINT8 u8PrimaryIntensity;
			UINT8 u8PrimaryRemainder;
			UINT8 u8NeighborLeftIntensity;
			UINT8 u8NeighborRightIntensity;
			UINT16 u16StepProc = 0;

			// s32StepAccumulator - closer to 0==Higher intensity

			u8PrimaryIntensity = ((0xff - (abs(s32StepAccumulator) >> 16)) * PRIMARY_REDUCTION_MULTIPLIER) >> 8;
			DRAW_PIXEL(pu16Addr, u8PrimaryIntensity);

			// This is what's left over
			u8PrimaryRemainder = 0xff - u8PrimaryIntensity;

			u16StepProc = (((INT16) ((INT32) s32StepAccumulator >> 16)) + 256) >> 1;

			u8NeighborRightIntensity = (u8PrimaryRemainder * u16StepProc) >> 8;
			u8NeighborLeftIntensity = (u8PrimaryRemainder * (0xff - u16StepProc)) >> 8;

	//		DebugOut("%.8x, %4d, intensity=0x%.2x, left=0x%.2x, right=0x%.2x, all=%d\n", s32StepAccumulator, u16StepProc, u8PrimaryIntensity, u8NeighborLeftIntensity, u8NeighborRightIntensity, u8PrimaryIntensity + u8NeighborLeftIntensity + u8NeighborRightIntensity);

			DRAW_PIXEL(pu16Addr + s32DrawStep, sg_u8ConeWeightingCurve[u8NeighborRightIntensity]);
			DRAW_PIXEL(pu16Addr - s32DrawStep, sg_u8ConeWeightingCurve[u8NeighborLeftIntensity]);

	//		DRAW_PIXEL(pu16Addr + s32DrawStep, u8NeighborRightIntensity);
	//		DRAW_PIXEL(pu16Addr - s32DrawStep, u8NeighborLeftIntensity);

			s32StepAccumulator += s32Step;
			while (s32StepAccumulator >= (1 << FIXED_POINT_FRACTION))
			{
				s32StepAccumulator -= (1 << FIXED_POINT_FRACTION);
				pu16Addr += s32AngleStep;
			}

			while (s32StepAccumulator <= -(1 << FIXED_POINT_FRACTION))
			{
				s32StepAccumulator += (1 << FIXED_POINT_FRACTION);
				pu16Addr += s32AngleStep;
			}

			pu16Addr += s32StraightStep;
			--u32PixelCount;
		}
	}
}

static void LineGetSize(SGfxElement *psElement,
						UINT32 *pu32XSize,
						UINT32 *pu32YSize)
{
	INT32 s32XLow = 9999999;
	INT32 s32XHigh = -9999999;
	INT32 s32YLow = 9999999;
	INT32 s32YHigh = -9999999;

	if (psElement->uEData.sLine.s32X0 < s32XLow)
	{
		s32XLow = psElement->uEData.sLine.s32X0;
	}
	if (psElement->uEData.sLine.s32X0 > s32XHigh)
	{
		s32XHigh = psElement->uEData.sLine.s32X0;
	}

	if (psElement->uEData.sLine.s32X1 < s32XLow)
	{
		s32XLow = psElement->uEData.sLine.s32X1;
	}
	if (psElement->uEData.sLine.s32X1 > s32XHigh)
	{
		s32XHigh = psElement->uEData.sLine.s32X1;
	}

	if (psElement->uEData.sLine.s32Y0 < s32YLow)
	{
		s32YLow = psElement->uEData.sLine.s32Y0;
	}
	if (psElement->uEData.sLine.s32Y0 > s32YHigh)
	{
		s32YHigh = psElement->uEData.sLine.s32Y0;
	}

	if (psElement->uEData.sLine.s32Y1 < s32YLow)
	{
		s32YLow = psElement->uEData.sLine.s32Y1;
	}
	if (psElement->uEData.sLine.s32Y1 > s32YHigh)
	{
		s32YHigh = psElement->uEData.sLine.s32Y1;
	}

	*pu32XSize = (UINT32) abs(s32XHigh - s32XLow);
	*pu32YSize = (UINT32) abs(s32YHigh - s32YLow);
}

static void AreaFill(SGfxElement *psLineElement,
					 SGraphSeries *psGraphSeries,
					 INT32 s32XPos,
					 INT32 s32YPos,
					 UINT16 *pu16PixelPosition,
					 UINT32 u32Pitch,
					 UINT16 *pu16WindowBase)
{
	INT32 s32StepCount = 0;
	INT32 s32Step = 0;
	INT32 s32ImageStep = 0;
	UINT8 u8RedSrc;
	UINT8 u8GreenSrc;
	UINT8 u8BlueSrc;

	if (FALSE == psLineElement->uEData.sLine.bAreaFill)
	{
		return;
	}

	if ((s32XPos < 0) || (s32YPos < 0))
	{
		DebugOut("Damnit!\n");
	}

	u8RedSrc = psLineElement->uEData.sLine.u16AreaFillColor >> 11;
	u8GreenSrc = (psLineElement->uEData.sLine.u16AreaFillColor >> 5) & 0x3f;
	u8BlueSrc = psLineElement->uEData.sLine.u16AreaFillColor & 0x1f;

	// We draw a line. Depending upon our orientation as compared to our X/Y position, draw
	// in the proper direction.

	if (EORIGIN_BOTTOM == psGraphSeries->eOrigin)
	{
		// At the bottom. Step Y + 1.
		s32YPos++;
		pu16PixelPosition += u32Pitch;
		s32StepCount = psLineElement->s32YOrigin - s32YPos;
		s32Step = (INT32) u32Pitch;
		s32ImageStep = (INT32) psGraphSeries->psSeriesImage->psCurrentImage->u32Pitch;
	}
	else
	if (EORIGIN_TOP == psGraphSeries->eOrigin)
	{
		// At the top. Step Y - 1.
		s32YPos--;
		pu16PixelPosition -= u32Pitch;
		s32StepCount = s32YPos - psLineElement->s32YOrigin;
		s32Step = -((INT32) u32Pitch);
		s32ImageStep = -((INT32) psGraphSeries->psSeriesImage->psCurrentImage->u32Pitch);
	}
	else
	if (EORIGIN_LEFT == psGraphSeries->eOrigin)
	{
		// At the left. Step X - 1.
		s32XPos--;
		pu16PixelPosition--;
		s32StepCount = s32XPos - psLineElement->s32XOrigin;
		s32Step = -1;
		s32ImageStep = -1;
	}
	else
	if (EORIGIN_RIGHT == psGraphSeries->eOrigin)
	{
		// At the right. Step X + 1.
		s32XPos++;
		pu16PixelPosition++;
		s32StepCount = psLineElement->s32XOrigin - s32XPos;
		s32Step = 1;
		s32ImageStep = 1;
	}
	else
	{
		GCASSERT(0);
	}

	// Now, we DRAW
	if (psGraphSeries->psSeriesImage)
	{
		UINT16 *pu16ImageSrc;

		pu16ImageSrc = psGraphSeries->psSeriesImage->psCurrentImage->pu16ImageData + s32XPos + (s32YPos * psGraphSeries->psSeriesImage->psCurrentImage->u32Pitch);

		// Image area fill
		while (s32StepCount > 0)
		{
			if ((s32XPos < 0) || (s32XPos >= (INT32) psGraphSeries->psSeriesImage->psCurrentImage->u32XSize) ||
				(s32YPos < 0) || (s32YPos >= (INT32) psGraphSeries->psSeriesImage->psCurrentImage->u32YSize))
			{
				// Just draw the default color
				DRAW_PIXEL(pu16PixelPosition, psLineElement->uEData.sLine.u8AreaFillTranslucency);
			}
			else
			{
				UINT8 u8RedSrc;
				UINT8 u8GreenSrc;
				UINT8 u8BlueSrc;

				// Yes, the recreation of these variables here is intentional so it only has this
				// level of scope! Draw the image pixel in question.

				u8RedSrc = *pu16ImageSrc >> 11;
				u8GreenSrc = (*pu16ImageSrc >> 5) & 0x3f;
				u8BlueSrc = *pu16ImageSrc & 0x1f;

				DRAW_PIXEL(pu16PixelPosition, psLineElement->uEData.sLine.u8AreaFillTranslucency);
			}

			pu16PixelPosition += s32Step;
			pu16ImageSrc += s32ImageStep;
			s32StepCount--;

			if ((-1 == s32Step) || (1 == s32Step))
			{
				// Stepping along X axis
				s32XPos += s32Step;
			}
			else
			if (s32Step < 0)
			{
				s32YPos--;
			}
			else
			{
				s32YPos++;
			}
		}
	}
	else
	{
		// Regular area fill
		while (s32StepCount > 0)
		{
			GCASSERT(((UINT32) pu16PixelPosition) > ((UINT32) pu16WindowBase));
			DRAW_PIXEL(pu16PixelPosition, psLineElement->uEData.sLine.u8AreaFillTranslucency);
			pu16PixelPosition += s32Step;
			s32StepCount--;
		}
	}
}

void LineDrawNormal(WINDOWHANDLE eWindow,
					SGfxElement *psLineElement,
					SGfxElement *psPriorElement,
					SGraphSeries *psGraphSeries)
{
	UINT16 *pu16Addr;
	INT32 s32DistanceX;
	INT32 s32DistanceY;
	INT32 s32StraightStep;
	INT32 s32DrawStep;
	INT32 s32AngleStep;
	INT32 s32StepAccumulator = 0;
	INT32 s32Step = 0;
	UINT32 u32PixelCount = 0;
	UINT8 u8RedSrc, u8GreenSrc, u8BlueSrc;
	SLine *psLine;
	SWindow *psWindow;

	if (NULL == psPriorElement)
	{
		// Don't draw squat
		return;
	}

	psWindow = WindowGetPointer(eWindow);
	GCASSERT(psWindow);

	// Point to our line
	psLine = &psLineElement->uEData.sLine;

	psLine->s32X0 = psPriorElement->s32XPos;
	psLine->s32Y0 = psPriorElement->s32YPos;
	psLine->s32X1 = psLineElement->s32XPos;
	psLine->s32Y1 = psLineElement->s32YPos;

	u8RedSrc = psLine->u16DrawColor >> 11;
	u8GreenSrc = (psLine->u16DrawColor >> 5) & 0x3f;
	u8BlueSrc = psLine->u16DrawColor & 0x1f;

	// Fix the starting address
	pu16Addr = psWindow->psWindowImage->pu16ImageData + (psLine->s32X0 + (INT32) psWindow->u32ActiveAreaXPos) + 
				((psLine->s32Y0 + (INT32) psWindow->u32ActiveAreaYPos) * psWindow->psWindowImage->u32Pitch);

	// Now figure out the slope
	s32DistanceX = psLine->s32X1 - psLine->s32X0;
	s32DistanceY = psLine->s32Y1 - psLine->s32Y0;

	if (abs(s32DistanceX) > abs(s32DistanceY))
	{
		// This means it's row major (step by X, Y is fractional)

		if (s32DistanceX < 0)
		{
			s32StraightStep = -1;
			s32DrawStep = -((INT32) psWindow->psWindowImage->u32Pitch);
		}
		else
		{
			s32StraightStep = 1;
			s32DrawStep = psWindow->psWindowImage->u32Pitch;
		}

		if (s32DistanceY < 0)
		{
			s32AngleStep = -((INT32) psWindow->psWindowImage->u32Pitch);
		}
		else
		{
			s32AngleStep = psWindow->psWindowImage->u32Pitch;
		}

		if (0 == s32DistanceX)
		{
			s32Step = 0;
		}
		else
		{
			s32Step = (s32DistanceY << FIXED_POINT_FRACTION) / s32DistanceX;
		}

		u32PixelCount = (UINT32) abs(s32DistanceX);
	}
	else
	{
		// This means it's column major (step by Y, X is fractional)

		if (s32DistanceY < 0)
		{
			s32StraightStep = -((INT32) psWindow->psWindowImage->u32Pitch);
			s32DrawStep = -1;
		}
		else
		{
			s32StraightStep = psWindow->psWindowImage->u32Pitch;
			s32DrawStep = 1;
		}

		if (s32DistanceX < 0)
		{
			s32AngleStep = -1;
		}
		else
		{
			s32AngleStep = 1;
		}

		if (0 == s32DistanceY)
		{
			s32Step = 0;
		}
		else
		{
			s32Step = (s32DistanceX << FIXED_POINT_FRACTION) / s32DistanceY;
		}

		u32PixelCount = (UINT32) abs(s32DistanceY);
	}

// Roughly 10%
#define		PRIMARY_REDUCTION_MULTIPLIER		((UINT16) (255.0 * 0.9))

	if (psGraphSeries->psSeriesImage)
	{
		// This will draw from a series image
		UINT8 u8RedSrcRef = psLine->u16DrawColor >> 11;
		UINT8 u8GreenSrcRef = (psLine->u16DrawColor >> 5) & 0x3f;
		UINT8 u8BlueSrcRef = psLine->u16DrawColor & 0x1f;
		INT32 s32ImgXPos = 0;
		INT32 s32ImgYPos = 0;
		UINT16 *pu16ImageSrc = psGraphSeries->psSeriesImage->psCurrentImage->pu16ImageData;
		INT32 s32XOld = -1;
		INT32 s32YOld = -1;

		s32ImgXPos = psLine->s32X0 - psGraphSeries->s32XOffset;
		s32ImgYPos = psLine->s32Y0 - psGraphSeries->s32YOffset;

		// Align the start of our image source
		pu16ImageSrc += s32ImgXPos + (s32ImgYPos * psGraphSeries->psSeriesImage->psCurrentImage->u32Pitch);

		while (u32PixelCount)
		{
			UINT8 u8PrimaryIntensity = 0xff;
			UINT16 u16StepProc = 0;

			// Start off assuming we're going to be doing 100% solid
			u8RedSrc = u8RedSrcRef; 
			u8GreenSrc = u8GreenSrcRef; 
			u8BlueSrc = u8BlueSrcRef;

			if ((s32ImgXPos >= 0) && (s32ImgYPos >= 0) &&
				(s32ImgXPos < (INT32) psGraphSeries->psSeriesImage->psCurrentImage->u32XSize) &&
				(s32ImgYPos < (INT32) psGraphSeries->psSeriesImage->psCurrentImage->u32YSize))
			{
				// We're within range! Pull out the pixel
				u8RedSrc = *pu16ImageSrc >> 11;
				u8GreenSrc = (*pu16ImageSrc >> 5) & 0x3f;
				u8BlueSrc =  *pu16ImageSrc & 0x1f;
			}

			DRAW_PIXEL(pu16Addr, u8PrimaryIntensity);

			if (((EORIGIN_TOP == psGraphSeries->eOrigin) ||
				 (EORIGIN_BOTTOM == psGraphSeries->eOrigin)) &&
				 (s32ImgXPos != s32XOld))
			{
				AreaFill(psLineElement,
						 psGraphSeries,
						 s32ImgXPos,
						 s32ImgYPos,
						 pu16Addr,
						 psWindow->psWindowImage->u32Pitch,
						 psWindow->psWindowImage->pu16ImageData);
				s32XOld = s32ImgXPos;
			}

			if (((EORIGIN_LEFT == psGraphSeries->eOrigin) ||
				 (EORIGIN_RIGHT == psGraphSeries->eOrigin)) &&
				 (s32ImgYPos != s32YOld))
			{
				AreaFill(psLineElement,
						 psGraphSeries,
						 s32ImgXPos,
						 s32ImgYPos,
						 pu16Addr,
						 psWindow->psWindowImage->u32Pitch,
						 psWindow->psWindowImage->pu16ImageData);
				s32YOld = s32ImgYPos;
			}

			u16StepProc = (((INT16) ((INT32) s32StepAccumulator >> 16)) + 256) >> 1;

			s32StepAccumulator += s32Step;
			while (s32StepAccumulator >= (1 << FIXED_POINT_FRACTION))
			{
				s32StepAccumulator -= (1 << FIXED_POINT_FRACTION);
				pu16Addr += s32AngleStep;
				if ((-1 == s32AngleStep) || (1 == s32AngleStep))
				{
					pu16ImageSrc += s32AngleStep;
					s32ImgXPos += s32AngleStep;
				}
				else
				if (s32AngleStep < 0)
				{
					// Negative pitch
					s32ImgYPos--;
					pu16ImageSrc -= psGraphSeries->psSeriesImage->psCurrentImage->u32Pitch;
				}
				else
				{
					// Positive pitch
					s32ImgYPos++;
					pu16ImageSrc += psGraphSeries->psSeriesImage->psCurrentImage->u32Pitch;
				}
			}

			while (s32StepAccumulator <= -(1 << FIXED_POINT_FRACTION))
			{
				s32StepAccumulator += (1 << FIXED_POINT_FRACTION);
				pu16Addr += s32AngleStep;
				if ((-1 == s32AngleStep) || (1 == s32AngleStep))
				{
					pu16ImageSrc += s32AngleStep;
					s32ImgXPos += s32AngleStep;
				}
				else
				if (s32AngleStep < 0)
				{
					// Negative pitch
					s32ImgYPos--;
					pu16ImageSrc -= psGraphSeries->psSeriesImage->psCurrentImage->u32Pitch;
				}
				else
				{
					// Positive pitch
					s32ImgYPos++;
					pu16ImageSrc += psGraphSeries->psSeriesImage->psCurrentImage->u32Pitch;
				}
			}

			pu16Addr += s32StraightStep;
			if ((-1 == s32StraightStep) || (1 == s32StraightStep))
			{
				pu16ImageSrc += s32StraightStep;
				s32ImgXPos += s32StraightStep;
			}
			else
			if (s32StraightStep < 0)
			{
				// Negative pitch
				s32ImgYPos--;
				pu16ImageSrc -= psGraphSeries->psSeriesImage->psCurrentImage->u32Pitch;
			}
			else
			{
				// Positive pitch
				s32ImgYPos++;
				pu16ImageSrc += psGraphSeries->psSeriesImage->psCurrentImage->u32Pitch;
			}

			--u32PixelCount;
		}
	}
	else
	{
		INT32 s32XPos = 0;
		INT32 s32YPos = 0;
		INT32 s32XOld = -1;
		INT32 s32YOld = -1;

		// This is the linedraw version that also steps the source pixel from
		// the provided image

		s32XPos = psLine->s32X0;
		s32YPos = psLine->s32Y0;

		while (u32PixelCount)
		{
			UINT16 u16StepProc = 0;
			// s32StepAccumulator - closer to 0==Higher intensity
			DRAW_PIXEL(pu16Addr, 0xff);

			// If we have a fill, go draw it

			if (((EORIGIN_TOP == psGraphSeries->eOrigin) ||
				 (EORIGIN_BOTTOM == psGraphSeries->eOrigin)) &&
				 (s32XPos != s32XOld))
			{
				AreaFill(psLineElement,
						 psGraphSeries,
						 s32XPos,
						 s32YPos,
						 pu16Addr,
						 psWindow->psWindowImage->u32Pitch,
						 psWindow->psWindowImage->pu16ImageData);
				s32XOld = s32XPos;
			}

			if (((EORIGIN_LEFT == psGraphSeries->eOrigin) ||
				 (EORIGIN_RIGHT == psGraphSeries->eOrigin)) &&
				 (s32YPos != s32YOld))
			{
				AreaFill(psLineElement,
						 psGraphSeries,
						 s32XPos,
						 s32YPos,
						 pu16Addr,
						 psWindow->psWindowImage->u32Pitch,
						 psWindow->psWindowImage->pu16ImageData);
				s32YOld = s32YPos;
			}

			u16StepProc = (((INT16) ((INT32) s32StepAccumulator >> 16)) + 256) >> 1;

			s32StepAccumulator += s32Step;
			while (s32StepAccumulator >= (1 << FIXED_POINT_FRACTION))
			{
				s32StepAccumulator -= (1 << FIXED_POINT_FRACTION);
				pu16Addr += s32AngleStep;

				if ((-1 == s32AngleStep) || (1 == s32AngleStep))
				{
					s32XPos += s32AngleStep;
				}
				else
				if (s32AngleStep < 0)
				{
					// Negative pitch
					s32YPos--;
				}
				else
				{
					// Positive pitch
					s32YPos++;
				}
			}

			while (s32StepAccumulator <= -(1 << FIXED_POINT_FRACTION))
			{
				s32StepAccumulator += (1 << FIXED_POINT_FRACTION);

				if ((-1 == s32AngleStep) || (1 == s32AngleStep))
				{
					s32XPos += s32AngleStep;
				}
				else
				if (s32AngleStep < 0)
				{
					s32YPos--;
				}
				else
				{
					s32YPos++;
				}

				pu16Addr += s32AngleStep;
			}

			pu16Addr += s32StraightStep;
			if ((-1 == s32StraightStep) || (1 == s32StraightStep))
			{
				s32XPos += s32StraightStep;
			}
			else
			if (s32StraightStep < 0)
			{
				// Negative pitch
				s32YPos--;
			}
			else
			{
				// Positive pitch
				s32YPos++;
			}

			--u32PixelCount;
		}
	}
}

static SElementFuncs sg_sLineAAFunctions =
{
	NULL,
	LineDrawAA,
	NULL,
	LineGetSize
};

static SElementFuncs sg_sLineNormalFunctions =
{
	NULL,
	LineDrawNormal,
	NULL,
	LineGetSize
};

void LineInit(void)
{
	double dStep;
	double dValue = -TAN_RANGE;
	double dConstant;
	UINT32 u32Loop;

	dStep = (TAN_RANGE - -TAN_RANGE) / 256.0;
	dConstant = 128.0 / atan(-TAN_RANGE);

	for (u32Loop = 0; u32Loop < (1 << 8); u32Loop++)
	{
		sg_u8ConeWeightingCurve[255 - u32Loop] = (UINT8) ((INT8) (atan(dValue) * dConstant) + 128);
		dValue += dStep;
	}

	// Register the line types
	ElementRegister(ELEMTYPE_LINE_AA,
					&sg_sLineAAFunctions);

	ElementRegister(ELEMTYPE_LINE_NORMAL,
					&sg_sLineNormalFunctions);
}



/*
void LineDraw(SWindow *psWindow,
			  SLine *psLine)
{
	UINT16 *pu16Addr;
	int dx = psLine->s32X1 - psLine->s32X0;
	int dy = psLine->s32Y1 - psLine->s32Y0;
	int du, dv, u, v, uincr, vincr;
	int uend, d, incrS, incrD, twovdu;
	double invD, invD2du;

	pu16Addr = psWindow->psWindowImage->pu16ImageData + psLine->s32X0 + (psLine->s32Y0 * psWindow->psWindowImage->u32Pitch);

    // By switching to (u,v), we combine all eight octants
    if (abs(dx) > abs(dy))
    {
		// Note: If this were actual C, these integers would be lost
		// at the closing brace.  That's not what I mean to do.  Do what
		// I mean.
		du = abs(dx);
		dv = abs(dy);
		u = psLine->s32X1;
		v = psLine->s32Y1;
		uincr = 1;
		vincr = psWindow->psWindowImage->u32Pitch;
		if (dx < 0) uincr = -uincr;
		if (dy < 0) vincr = -vincr;
    }
    else
    {
		du = abs(dy);
		dv = abs(dx);
		u = psLine->s32Y1;
		v = psLine->s32X1;
		uincr = psWindow->psWindowImage->u32Pitch;
		vincr = 1;
		if (dy < 0) uincr = -uincr;
		if (dx < 0) vincr = -vincr;
    }

    uend = u + (du << 1);
    d = (dv << 1) - du;	    // Initial value as in Bresenham's
    incrS = 2 * dv;	// ?d for straight increments 
    incrD = 2 * (dv - du);	// ?d for diagonal increments 
    twovdu = 0;	// Numerator of distance; starts at 0
    invD = 1.0 / (2.0*sqrt(du*du + dv*dv));   // Precomputed inverse denominator
    invD2du = 2.0 * (du*invD);   // Precomputed constant 

    do
    {
		double dValue = 0;
		UINT8 u8Intensity;
		// Note: this pseudocode doesn't ensure that the address is
		// valid, or that it even represents a pixel on the same side of
		// the screen as the adjacent pixel

		DebugOut("Pixel=%f,%f,%f,\n",
				   twovdu*invD,
				   invD2du - twovdu*invD,
				   invD2du + twovdu*invD);

		dValue = ((twovdu*invD) + 0.5) * 134.22222; 
		if (dValue >= 0x40)	dValue = 63.0;
		u8Intensity = (UINT8) dValue;
		*pu16Addr = (u8Intensity >> 1) | (u8Intensity << 5) | ((u8Intensity >> 1) << 11);

		//******************************************

		dValue = ((invD2du - (twovdu*invD)) + 0.5) * 134.22222; 
		if (dValue >= 0x40)	dValue = 63.0;
		u8Intensity = (UINT8) dValue;
		*(pu16Addr + vincr) = (u8Intensity >> 1) | (u8Intensity << 5) | ((u8Intensity >> 1) << 11);

		//******************************************

		dValue = ((invD2du + (twovdu*invD)) + 0.5) * 134.22222; 
		if (dValue >= 0x40)	dValue = 63.0;
		u8Intensity = (UINT8) dValue;
		*(pu16Addr - vincr) = (u8Intensity >> 1) | (u8Intensity << 5) | ((u8Intensity >> 1) << 11);

//		*(pu16Addr + vincr) = 0xffff;
//		*(pu16Addr - vincr) = 0xffff;

//		dValue = twovdu*invD;

//		DebugOut("Pixel=%f,%f,%f\n",
//				   dValue,
//				   (invD2du - twovdu*invD),
//				   (invD2du + twovdu*invD));

//		DrawPixelD(addr, twovdu*invD);
//		DrawPixelD(addr + vincr, invD2du - twovdu*invD);
//		DrawPixelD(addr - vincr, invD2du + twovdu*invD);

		if (d < 0)
		{
			// choose straight (u direction)
			twovdu = d + du;
			d = d + incrS;
		}
		else
		{
			// choose diagonal (u+v direction)
			twovdu = d - du;
			d = d + incrD;
			v = v+1;
			pu16Addr = pu16Addr + vincr;
		}

		u = u+2;
		pu16Addr = pu16Addr+uincr;
    } while (u < uend);
} */