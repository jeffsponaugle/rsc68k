#include <math.h>
#include "Startup/app.h"
#include "Libs/widget/elements/elements.h"
#include "Libs/window/window.h"
#include "Libs/widget/graph/graph.h"

static void ImageElementStructInit(struct SGfxElement *psElement)
{
	psElement->uEData.sText.eFontHandle = HANDLE_INVALID;
}

static void ImageElementDraw(WINDOWHANDLE eWindow,
							 struct SGfxElement *psElement,
							 struct SGfxElement *psPriorElement,
							 SGraphSeries *psGraphSeries)
{
	SWindow *psWindow;
	
	psWindow = WindowGetPointer(eWindow);
	GCASSERT(psWindow);

	// Blit the image
	GfxBlitImageToImage(psWindow->psWindowImage,
						psElement->uEData.sImage.psGroup->psCurrentImage,
					    psElement->s32XPos,
					    psElement->s32YPos,
						psWindow->u32ActiveAreaXPos + psWindow->u32ActiveAreaXSize,
						psWindow->u32ActiveAreaYPos + psWindow->u32ActiveAreaYSize);
}

static void ImageGetSize(SGfxElement *psElement,
					     UINT32 *pu32XSize,
					     UINT32 *pu32YSize)
{
	*pu32XSize = psElement->uEData.sImage.psGroup->psCurrentImage->u32XSize;
	*pu32YSize = psElement->uEData.sImage.psGroup->psCurrentImage->u32YSize;
}

static void ImageElementFree(SGraphSeries *psGraphSeries,
							 SGfxElement *psElement)
{
	GCASSERT(psGraphSeries);
	GCASSERT(psElement);

	GfxDeleteImageGroup(psElement->uEData.sImage.psGroup);
	psElement->uEData.sImage.psGroup = NULL;
}

static SElementFuncs sg_sImageElementFunctions =
{
	ImageElementStructInit,
	ImageElementDraw,
	ImageElementFree,
	ImageGetSize
};

void ImageElementInit(void)
{
	// Register the line type
	ElementRegister(ELEMTYPE_IMAGE,
					&sg_sImageElementFunctions);
}

