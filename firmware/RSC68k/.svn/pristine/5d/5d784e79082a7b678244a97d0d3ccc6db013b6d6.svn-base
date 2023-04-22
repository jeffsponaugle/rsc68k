#include <math.h>
#include "Startup/app.h"
#include "Libs/widget/elements/elements.h"
#include "Libs/window/window.h"
#include "Libs/widget/graph/graph.h"

static void TextElementStructInit(struct SGfxElement *psElement)
{
	psElement->uEData.sText.eFontHandle = HANDLE_INVALID;
}

static void TextElementDraw(WINDOWHANDLE eWindow,
							struct SGfxElement *psElement,
							struct SGfxElement *psPriorElement,
							SGraphSeries *psGraphSeries)
{
	INT32 s32XPos = 0;
	INT32 s32YPos = 0;
	LEX_CHAR *peStringPtr = psElement->uEData.sText.peText;
	ELCDErr eLCDErr;
	INT32 s32XAdvance;
	INT32 s32YAdvance;
	SWindow *psWindow;

	if (NULL == peStringPtr)
	{
		return;
	}

	psWindow = WindowGetPointer(eWindow);
	GCASSERT(psWindow);

	s32XPos = psElement->s32XPos << 6;
	s32YPos = psElement->s32YPos << 6;

	// Adjust the X/Y position(s)

	if ((ROT_180 == psElement->uEData.sText.eRotation) ||
		(ROT_270 == psElement->uEData.sText.eRotation))
	{
		INT32 s32XSize;
		INT32 s32YSize;

		// Adjust our X position to be at the end of the string
		while (*peStringPtr)
		{
			eLCDErr = FontGetSize(psElement->uEData.sText.eFontHandle,
								  (TEXTCHAR) *peStringPtr,
								  NULL,
								  NULL,
								  &s32XSize,
								  &s32YSize,
								  psElement->uEData.sText.eRotation,
								  TRUE);

			// This is done so that the last character's width isn't added since our origin
			// of all characters displayed are in the upper left hand corner
			if (*(peStringPtr + 1))
			{
				if (ROT_180 == psElement->uEData.sText.eRotation)
				{
					s32XPos += -s32XSize;
				}
				else
				{
					s32YPos += -s32YSize;
				}
			}

			peStringPtr++;
		}
	}

	peStringPtr = psElement->uEData.sText.peText;

	if (NULL == peStringPtr)
	{
		return;
	}

	while (*peStringPtr)
	{
		eLCDErr = FontRender(psElement->uEData.sText.eFontHandle,
							 eWindow,
							 s32XPos,
							 s32YPos,
							 -1,
							 -1,
							 (TEXTCHAR) *peStringPtr,
							 psElement->u16DrawColor,
							 FALSE,
							 &s32XAdvance,
							 &s32YAdvance,
							 FALSE,
							 psElement->uEData.sText.eRotation,
							 NULL,
							 FALSE,
							 TRUE);
		
		if ((ROT_0 == psElement->uEData.sText.eRotation) ||
			(ROT_180 == psElement->uEData.sText.eRotation))
		{
			s32YAdvance = 0;
		}

		if ((ROT_90 == psElement->uEData.sText.eRotation) ||
			(ROT_270 == psElement->uEData.sText.eRotation))
		{
			s32XAdvance = 0;
		}

		s32XPos += s32XAdvance;
		s32YPos += s32YAdvance;
		++peStringPtr;
	}
}

static void TextFree(SGraphSeries *psGraphSeries,
					 struct SGfxElement *psElement)
{
	if (psElement->uEData.sText.peText)
	{
		GCFreeMemory(psElement->uEData.sText.peText);
		psElement->uEData.sText.peText = NULL;
	}

	(void) FontFree(psElement->uEData.sText.eFontHandle);
	psElement->uEData.sText.eFontHandle = HANDLE_INVALID;
}

static void TextGetSize(SGfxElement *psElement,
						UINT32 *pu32XSize,
						UINT32 *pu32YSize)
{
	// Go get the x/y size of this string
	(void) FontGetStringSize(psElement->uEData.sText.eFontHandle,
							 psElement->uEData.sText.peText,
							 pu32XSize,
							 pu32YSize,
							 psElement->uEData.sText.eRotation,
							 TRUE);
}

						
static SElementFuncs sg_sTextFunctions =
{
	TextElementStructInit,
	TextElementDraw,
	TextFree,
	TextGetSize
};

void TextElementInit(void)
{
	// Register the line type
	ElementRegister(ELEMTYPE_TEXT,
					&sg_sTextFunctions);
}

void TextElementSetText(struct SGfxElement *psElement,
						LEX_CHAR *peString)
{
	psElement->uEData.sText.peText = peString;
}

void TextElementSetFont(struct SGfxElement *psElement,
						FONTHANDLE eFontHandle)
{
	psElement->uEData.sText.eFontHandle = eFontHandle;
}

void TextElementSetRotation(struct SGfxElement *psElement,
							ERotation eRot)
{
	psElement->uEData.sText.eRotation = eRot;
}
