#include "Startup/app.h"
#include "Win32/sdl/sdl.h"
#include "Win32/host.h"
#include "Application/Mersenne.h"

typedef struct SAxis
{
	UINT32 u32XPoint;
	UINT32 u32YPoint;
} SAxis;

enum
{
	ECOORD_UPPER_LEFT,
	ECOORD_LOWER_LEFT,
	ECOORD_UPPER_RIGHT,
	ECOORD_LOWER_RIGHT,
	ECOORD_CENTER,
	ECOORD_COUNT
};

EGCResultCode PlatformPointerGetRanges(UINT32 *pu32LowX,
									   UINT32 *pu32HighX,
									   UINT32 *pu32LowY,
									   UINT32 *pu32HighY)
{
	*pu32LowX = 0;
	*pu32HighX = 0xfff;
	*pu32LowY = 0;
	*pu32HighY = 0xfff;
	return(GC_OK);
}

typedef struct SCalibration
{
	UINT32 u32XSize;
	UINT32 u32YSize;
	UINT32 u32XLow;
	UINT32 u32XHigh;
	UINT32 u32YLow;
	UINT32 u32YHigh;
	SAxis sAxes[ECOORD_COUNT];
} SCalibration;

static BOOL sg_bCalibrationComplete = TRUE;
static SCalibration sg_sActiveCalibration;

static void SetCalibration(SCalibration *psCal)
{
	// Left side
	if (psCal->sAxes[ECOORD_UPPER_LEFT].u32XPoint <
		psCal->sAxes[ECOORD_LOWER_LEFT].u32XPoint)
	{
		psCal->u32XLow = psCal->sAxes[ECOORD_UPPER_LEFT].u32XPoint;
	}
	else
	{
		psCal->u32XLow = psCal->sAxes[ECOORD_LOWER_LEFT].u32XPoint;
	}

	// Right side
	if (psCal->sAxes[ECOORD_UPPER_RIGHT].u32XPoint <
		psCal->sAxes[ECOORD_LOWER_RIGHT].u32XPoint)
	{
		psCal->u32XHigh = psCal->sAxes[ECOORD_UPPER_RIGHT].u32XPoint;
	}
	else
	{
		psCal->u32XHigh = psCal->sAxes[ECOORD_LOWER_RIGHT].u32XPoint;
	}

	// Top side
	if (psCal->sAxes[ECOORD_UPPER_LEFT].u32YPoint <
		psCal->sAxes[ECOORD_UPPER_RIGHT].u32YPoint)
	{
		psCal->u32YLow = psCal->sAxes[ECOORD_UPPER_LEFT].u32YPoint;
	}
	else
	{
		psCal->u32YLow = psCal->sAxes[ECOORD_UPPER_RIGHT].u32YPoint;
	}

	// Bottom side
	if (psCal->sAxes[ECOORD_LOWER_LEFT].u32YPoint <
		psCal->sAxes[ECOORD_LOWER_RIGHT].u32YPoint)
	{
		psCal->u32YHigh = psCal->sAxes[ECOORD_LOWER_LEFT].u32YPoint;
	}
	else
	{
		psCal->u32YHigh = psCal->sAxes[ECOORD_LOWER_RIGHT].u32YPoint;
	}

	sg_bCalibrationComplete = TRUE;
}

void SetCalibrationScale(UINT32 u32XSize,
						 UINT32 u32YSize,
						 SCalibration *psCal)
{
	psCal->u32XSize = u32XSize;
	psCal->u32YSize = u32YSize;
}

void CoordinatesTransform(UINT32 *pu32XPos,
						  UINT32 *pu32YPos,
						  SCalibration *psCal)
{
	UINT32 u32XPercentage;
	UINT32 u32YPercentage;

	// First, clip it
	if (*pu32XPos < psCal->u32XLow)
	{
		*pu32XPos = psCal->u32XLow;
	}

	if (*pu32XPos > psCal->u32XHigh)
	{
		*pu32XPos = psCal->u32XHigh;
	}

	if (*pu32YPos < psCal->u32YLow)
	{
		*pu32YPos = psCal->u32YLow; 
	}

	if (*pu32YPos > psCal->u32YHigh)
	{
		*pu32YPos = psCal->u32YHigh;
	}

	// Normalize it

	*pu32XPos -= psCal->u32XLow;
	*pu32YPos -= psCal->u32YLow;

	u32XPercentage = (*pu32XPos << 16) / (psCal->u32XHigh - psCal->u32XLow);
	u32YPercentage = (*pu32YPos << 16) / (psCal->u32YHigh - psCal->u32YLow);

	*pu32XPos = (psCal->u32XSize * u32XPercentage) >> 16;
	*pu32YPos = (psCal->u32YSize * u32YPercentage) >> 16;
}

static void DrawCrosshairs(UINT32 u32X, UINT32 u32Y, UINT16 u16Colors, UINT16 *pu16FrameBuffer, UINT32 u32Pitch)
{
#define Y_SIZE	50
#define X_SIZE	50

	INT32 s32XPos = 0;
	INT32 s32YPos = 0;
	UINT32 u32Count = 0;
	UINT32 u32XSize = 0;
	UINT32 u32YSize = 0;
	EGCResultCode eResult;
	UINT16 *pu16Position;

	eResult = GCDisplayGetXSize(&u32XSize);
	GCASSERT(GC_OK == eResult);
	eResult = GCDisplayGetYSize(&u32YSize);
	GCASSERT(GC_OK == eResult);

	// Draw horizontal line

	s32XPos = ((INT32) u32X) - (X_SIZE >> 1);
	s32YPos = (INT32) u32Y;

	u32Count = X_SIZE + 1;

	while (u32Count--) 
	{
		if ((s32XPos >=0 && s32XPos < (INT32) (u32XSize - 1))) 
		{
			pu16Position = pu16FrameBuffer + (s32YPos * u32Pitch) + s32XPos;
			*pu16Position = u16Colors;
		}

		++s32XPos;
	}

	u32Count = Y_SIZE + 1;
	s32XPos = (INT32) u32X;
	s32YPos = ((INT32) u32Y) - (Y_SIZE >> 1);

	while (u32Count--) 
	{
		if ((s32YPos >=0 && s32YPos < (INT32) (u32YSize - 1)))
		{
			pu16Position = pu16FrameBuffer + (s32YPos * u32Pitch) + s32XPos;
			*pu16Position = u16Colors;
		}

		++s32YPos;
	}
}

static void CalibrateInstruction(char *pu8Data)
{
	UINT32 u32XSize;
	UINT32 u32YSize;
	EGCResultCode eResult;

	eResult = GCDisplayGetXSize(&u32XSize);
	GCASSERT(GC_OK == eResult);
	u32XSize >>= 4;
	eResult = GCDisplayGetYSize(&u32YSize);
	GCASSERT(GC_OK == eResult);
	u32YSize >>= 4;
	u32YSize += (u32YSize >> 1);

	u32XSize -= (strlen(pu8Data) >> 1);
}

static void	WaitForTouch(SAxis *psAxis)
{
	EGCResultCode eResult;
	UINT32 u32Counter = (GCGetRefreshRate() >> 24) / 4;
	UINT32 u32AccumulatorX = 0;
	UINT32 u32AccumulatorY = 0;
	UINT32 u32XPos;
	UINT32 u32YPos;
	UINT32 u32Buttons;

	// Wait for the button to NOT be pressed

	while (u32Counter)
	{
		eResult = GCPointerGetPosition(0, &u32XPos, &u32YPos, &u32Buttons);
		if (u32Buttons)
		{
			u32Counter = (GCGetRefreshRate() >> 24) / 4;
		}
		else
		{
			--u32Counter;
		}

		GCWaitForVsync();
	}

	// Okay, it's not being pressed. Let's wait until it is being pressed for a short period of time,
	// then start sampling.

	u32Counter = (GCGetRefreshRate() >> 24) / 2;
	u32AccumulatorX = 0;
	u32AccumulatorY = 0;

	while (u32Counter)
	{
		eResult = GCPointerGetPosition(0, &u32XPos, &u32YPos, &u32Buttons);
		if (0 == u32Buttons)
		{
			u32Counter = (GCGetRefreshRate() >> 24) / 2;
			u32AccumulatorX = 0;
			u32AccumulatorY = 0;
		}
		else
		{
			u32AccumulatorX += u32XPos;
			u32AccumulatorY += u32YPos;
			--u32Counter;
		}

		GCWaitForVsync();
	}

	psAxis->u32XPoint = u32AccumulatorX / ((GCGetRefreshRate() >> 24) / 2);
	psAxis->u32YPoint = u32AccumulatorY / ((GCGetRefreshRate() >> 24) / 2);
}

EGCResultCode GCPointerCalibrate(UINT32 u32Instance)
{
	GCASSERT(0);
	return(GC_OK);
}

EGCResultCode GCPointerGetCalibrationState(UINT32 u32Instance)
{
	if (0 == u32Instance)
	{
		if (FALSE == sg_bCalibrationComplete)
		{
			return(GC_POINTER_NOT_CALIBRATED);
		}
		else
		{
			return(GC_OK);
		}
	}

	return(GC_POINTER_NOT_AVAILABLE);
}
