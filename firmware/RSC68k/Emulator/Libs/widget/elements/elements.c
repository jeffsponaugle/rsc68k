#include <math.h>
#include "Startup/app.h"
#include "Libs/widget/elements/elements.h"
#include "Libs/window/window.h"
#include "Libs/widget/graph/graph.h"

static SElementFuncs *sg_psElementFunctions[ELEMTYPE_COUNT];

void ElementRegister(EElementType eType,
					 SElementFuncs *psFuncs)
{
	GCASSERT(eType < (sizeof(sg_psElementFunctions) / sizeof(sg_psElementFunctions[0])));
	GCASSERT(NULL == sg_psElementFunctions[eType]);

	// Hook up the functions
	sg_psElementFunctions[eType] = psFuncs;
}

void ElementsInit(void)
{
	LineInit();
	BarInit();
	CircleInit();
	TextElementInit();
	ImageElementInit();
	DotInit();
}

SGfxElement *ElementCreate(EElementType eType,
						   INT32 s32XPos,
						   INT32 s32YPos,
						   UINT16 u16DrawColor,
						   SGfxElement *psElementSource)
{
	SGfxElement *psGfxElement = NULL;

	GCASSERT(eType < (sizeof(sg_psElementFunctions) / sizeof(sg_psElementFunctions[0])));
	GCASSERT(sg_psElementFunctions[eType]);

	// Allocate space for this if needed
	if (NULL == psElementSource)
	{
		psGfxElement = MemAlloc(sizeof(*psGfxElement));
		if (NULL == psGfxElement)
		{
			return(NULL);
		}

		psGfxElement->bHeapAllocated = TRUE;
	}
	else
	{
		psGfxElement = psElementSource;
	}

	psGfxElement->psFuncs = sg_psElementFunctions[eType];
	psGfxElement->s32XPos = s32XPos;
	psGfxElement->s32YPos = s32YPos;
	psGfxElement->u16DrawColor = u16DrawColor;
	psGfxElement->eElementType = eType;

	// Start off drawing completely solid (not translucent)
	psGfxElement->u8DrawTranslucency = 0xff;

	if (psGfxElement->psFuncs->ElemInit)
	{
		psGfxElement->psFuncs->ElemInit(psGfxElement);
	}

	return(psGfxElement);
}

void ElementDraw(WINDOWHANDLE eWindow,
				 SGfxElement *psElement,
				 SGfxElement *psPriorElement,
				 SGraphSeries *psGraphSeries)
{
	GCASSERT(psElement);
	if (psElement->psFuncs->ElemDraw)
	{
		psElement->psFuncs->ElemDraw(eWindow,
									 psElement,
									 psPriorElement,
									 psGraphSeries);
	}
}

void ElementSetDrawColor(SGfxElement *psElement,
						 UINT16 u16DrawColor)
{
	GCASSERT(psElement);
	psElement->u16DrawColor = u16DrawColor;
}

void ElementSetDrawTranslucency(SGfxElement *psElement,
								UINT8 u8DrawTranslucency)
{
	GCASSERT(psElement);
	psElement->u8DrawTranslucency = u8DrawTranslucency;
}

void ElementSetPosition(SGfxElement *psElement,
						INT32 s32XPos,
						INT32 s32YPos)
{
	GCASSERT(psElement);
	psElement->s32XPos = s32XPos;
	psElement->s32YPos = s32YPos;
}

void ElementCalculateBounds(SGfxElement *psElement,
							INT32 *ps32XLow,
							INT32 *ps32XHigh,
							INT32 *ps32YLow,
							INT32 *ps32YHigh)
{
	while (psElement)
	{
		if (psElement->s32XPos < *ps32XLow)
		{
			*ps32XLow = psElement->s32XPos;
		}

		if (psElement->s32YPos < *ps32YLow)
		{
			*ps32YLow = psElement->s32YPos;
		}

		if ((psElement->s32XPos + (INT32) psElement->u32XSize) > *ps32XHigh)
		{
			*ps32XHigh = (psElement->s32XPos + (INT32) psElement->u32XSize);
		}

		if ((psElement->s32YPos + (INT32) psElement->u32YSize) > *ps32YHigh)
		{
			*ps32YHigh = (psElement->s32YPos + (INT32) psElement->u32YSize);
		}

		psElement = psElement->psNextLink;
	}
}
