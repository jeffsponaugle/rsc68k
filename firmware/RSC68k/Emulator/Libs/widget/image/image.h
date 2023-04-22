#ifndef _IMAGEWIDGET_H_
#define _IMAGEWIDGET_H_

#include "Libs/Gfx/GraphicsLib.h"

typedef struct SImageWidget
{
	FONTHANDLE eFontHandle;				// Find handle this console is related to
	IMAGEHANDLE eImageHandle;			// This image's handle
	SImageGroup *psImageGroup;			// Image group associated with this widget
	SImageInstance *psImageInstance;	// Image instance
	struct SWidget *psWidget;			// Widget pointer
} SImageWidget;

typedef enum
{
	IMGDRAW_SOLID,
	IMGDRAW_TRANSPARENT,
	IMGDRAW_TRANSLUCENT
} EImageWidgetDrawType;

extern void ImageWidgetFirstTimeInit(void);
extern SImageWidget *ImageWidgetGetPointer(IMAGEHANDLE eHandle);
extern ELCDErr ImageWidgetCreate(WINDOWHANDLE eWindowHandle,
								 IMAGEHANDLE *peImageWidgetHandle,
								 LEX_CHAR *peFilename,
								 INT32 s32XPos,
								 INT32 s32YPos,
								 EImageWidgetDrawType eDrawType);
extern ELCDErr ImageWidgetAnimateStart(IMAGEHANDLE eImageWidgetHandle);
extern ELCDErr ImageWidgetAnimateStop(IMAGEHANDLE eImageWidgetHandle);
extern ELCDErr ImageWidgetAnimateStep(IMAGEHANDLE eImageWidgetHandle);
extern ELCDErr ImageWidgetAnimateReset(IMAGEHANDLE eImageWidgetHandle);
extern ELCDErr ImageRotate(IMAGEHANDLE eImageWidgetHandle,
						   UINT16 u16Orientation);
extern ELCDErr ImageGetInfo(IMAGEHANDLE eImageHandle,
							UINT32 *pu32ImageXSize,
							UINT32 *pu32ImageYSize,
							UINT32 *pu32BitsPerPixel,
							BOOL *pbTranslucentTransparent,
							UINT32 *pu32TotalFrames,
							UINT32 *pu32CurrentFrame);

#endif