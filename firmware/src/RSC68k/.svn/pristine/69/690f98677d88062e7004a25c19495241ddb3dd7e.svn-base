#include <math.h>
#include "Startup/app.h"
#include "Libs/widget/elements/elements.h"
#include "Libs/window/window.h"
#include "Libs/widget/graph/graph.h"

#define	CIRCLE_STEPS		512
#define	ANGLE_STEP			(360.0 / CIRCLE_STEPS)
#define PI					3.1415926535

#define	DECIMAL_PRECISION	24

// Sin/Cos in 8.24 format
static INT32 sg_s32Sin[CIRCLE_STEPS];
static INT32 sg_s32Cos[CIRCLE_STEPS];

void CircleDraw(WINDOWHANDLE eWindow,
				struct SGfxElement *psElement,
				struct SGfxElement *psPriorElement,
				SGraphSeries *psGraphSeries)
{
}

static SElementFuncs sg_sCircleFunctions =
{
	NULL,
	CircleDraw
};

void CircleInit(void)
{
	double dAngle;
	UINT32 u32Index = 0;

	for (dAngle = 0.0; dAngle < 360.0; dAngle += ANGLE_STEP)
	{
		GCASSERT(u32Index < CIRCLE_STEPS);
		sg_s32Sin[u32Index] = (INT32) (sin(dAngle * (PI / 180.0)) * (double) (1 << DECIMAL_PRECISION));
		sg_s32Cos[u32Index++] = (INT32) (cos(dAngle * (PI / 180.0)) * (double) (1 << DECIMAL_PRECISION));
	}

	// Register the line type
	ElementRegister(ELEMTYPE_CIRCLE,
					&sg_sCircleFunctions);
}

