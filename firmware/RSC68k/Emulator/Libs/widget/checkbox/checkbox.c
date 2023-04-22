#include "Startup/app.h"
#include "Libs/Gfx/GraphicsLib.h"
#include "Libs/window/window.h"
#include "Libs/widget/widget.h"
#include "Libs/widget/checkbox/checkbox.h"
#include "Libs/widget/radio/radio.h"

#define	MAX_CHECKBOXES		128

static SCheckbox *sg_psCheckboxList[MAX_CHECKBOXES];

static void CheckboxRadioErrNormalize(ELCDErr *peErr)
{
	if ((*peErr &0xff000000) == LERR_RADIO_FULL)
	{
		// Rebase the radio errors if they occur
		*peErr = (*peErr - LERR_RADIO_FULL) + LERR_CHECKBOX_FULL;
	}
}

static ELCDErr CheckboxWidgetAlloc(SWidget *psWidget,
								   WIDGETHANDLE eHandle)
{
	GCASSERT(psWidget);
	psWidget->uWidgetSpecific.psCheckbox = MemAlloc(sizeof(*psWidget->uWidgetSpecific.psCheckbox));
	if (NULL == psWidget->uWidgetSpecific.psCheckbox)
	{
		return(LERR_NO_MEM);
	}
	else
	{
		psWidget->uWidgetSpecific.psCheckbox->eRadioGroupHandle = HANDLE_INVALID;
		return(LERR_OK);
	}
}

static ELCDErr CheckboxWidgetFree(SWidget *psWidget)
{
	ELCDErr eErr;

	GCASSERT(psWidget);
	GCASSERT(psWidget->uWidgetSpecific.psCheckbox);

	// Go delete the radio widget
	eErr = WidgetDestroyByHandle((WIDGETHANDLE *) &psWidget->uWidgetSpecific.psCheckbox->eRadioGroupHandle,
								  WIDGET_RADIO);

	// Now ditch the checkbox info
	GCFreeMemory(psWidget->uWidgetSpecific.psCheckbox);
	psWidget->uWidgetSpecific.psCheckbox = NULL;

	CheckboxRadioErrNormalize(&eErr);
	return(eErr);
}

ELCDErr CheckboxGroupCreate(CHECKBOXGROUPHANDLE *peCheckboxHandle,
						    WINDOWHANDLE eWindowHandle,
						    UINT32 u32XPos,
						    UINT32 u32YPos,
						    BOOL bRightBottomJustified,
						    BOOL bVisible)
{
	ELCDErr eLCDErr = LERR_OK;
	SRadio *psRadio = NULL;
	SCheckbox *psCheckbox = NULL;
	SWidget *psWidget = NULL;

	// Force the checkbox handle to start out being invalid
	*peCheckboxHandle = HANDLE_INVALID;

	eLCDErr = WidgetAllocateHandle((WIDGETHANDLE *) peCheckboxHandle,
								   WIDGET_CHECKBOX,
								   eWindowHandle,
								   &psWidget);

	RETURN_ON_FAIL(eLCDErr);

	eLCDErr = RadioGroupCreate(&psWidget->uWidgetSpecific.psCheckbox->eRadioGroupHandle,
							   eWindowHandle,
							   u32XPos,
							   u32YPos,
							   bRightBottomJustified,
							   bVisible);

	psWidget->uWidgetSpecific.psCheckbox->bRightBottomJustified = bRightBottomJustified;

	if (eLCDErr != LERR_OK)
	{
		(void) WidgetDestroyByHandle((WIDGETHANDLE *) &psWidget->uWidgetSpecific.psCheckbox->eRadioGroupHandle,
									 WIDGET_RADIO);
	}
	else
	{
		eLCDErr = WidgetGetPointerByHandle(psWidget->uWidgetSpecific.psCheckbox->eRadioGroupHandle,
										   WIDGET_RADIO,
										   &psWidget,
										   NULL);
		GCASSERT(LERR_OK == eLCDErr);

		// Tag this as a subordinate group so it won't get deleted by a window shutdown
		eLCDErr = WidgetSetParent((WIDGETHANDLE) psWidget->uWidgetSpecific.psCheckbox->eRadioGroupHandle,
								  WIDGET_RADIO,
								  psWidget->uWidgetSpecific.psCheckbox->eRadioGroupHandle);
	}

	// Make sure the widget is enabled
	psWidget->bWidgetEnabled = TRUE;

	return(eLCDErr);
}

#define	CHECK_TRANSPARENT	0x00
#define CHECK_OUTER_UL		0x01
#define CHECK_INNER_UL		0x02
#define CHECK_OUTER_LR		0x03
#define CHECK_INNER_LR		0x04
#define CHECK_FILL			0x05

#define	CHECK_INDEX_COUNT	0x06

#define	COLOR_OUTER_UL		0x808080
#define	COLOR_INNER_UL		0x000000
#define	COLOR_OUTER_LR		0xffffff
#define	COLOR_INNER_LR		0xc0c0c0

ELCDErr CheckBoxRender(UINT32 u32ImageXSize,
					   UINT32 u32ImageYSize,
					   UINT32 u32BoxSize,
					   UINT32 u32XPos,
					   UINT32 u32YPos,
					   UINT32 u32Color,
					   SImage **ppsImage,
					   UINT16 u16Orientation,
					   ECheckboxStyle eStyle,
					   BOOL bChecked)
{
	ELCDErr eErr = LERR_OK;
	UINT16 u16BoxColor;
	UINT32 u32Loop;
	UINT8 *pu8PixelBase;
	UINT8 *pu8PixelTemp;
	UINT8 *pu8PixelTemp2;

	// Gotta be at least 5x5
	if (u32BoxSize < 5)
	{
		eErr = LERR_CHECKBOX_SIZE_TOO_SMALL;
		goto errorExit;
	}

	// Now get the fill color for the box
	u16BoxColor = CONVERT_24RGB_16RGB(u32Color);

	if (NULL == *ppsImage)
	{
		// Now let's create an empty image in which to draw on
		*ppsImage = GfxCreateEmptyImage(u32ImageXSize,
										u32ImageYSize,
										8,
										0,
										FALSE);
		if (NULL == *ppsImage)
		{
			eErr = LERR_NO_MEM;
			goto errorExit;
		}

		(*ppsImage)->pu16Palette = MemAlloc((sizeof(UINT8) << 8) * sizeof((*ppsImage)->pu16Palette));
		if (NULL == (*ppsImage)->pu16Palette)
		{
			// Out of memory
			eErr = LERR_NO_MEM;
			goto errorExit;
		}

		(*ppsImage)->pu16Palette[CHECK_TRANSPARENT] = 0;	// Transparent/black
		(*ppsImage)->pu16Palette[CHECK_OUTER_UL] = CONVERT_24RGB_16RGB(COLOR_OUTER_UL);
		(*ppsImage)->pu16Palette[CHECK_INNER_UL] = CONVERT_24RGB_16RGB(COLOR_INNER_UL);
		(*ppsImage)->pu16Palette[CHECK_OUTER_LR] = CONVERT_24RGB_16RGB(COLOR_OUTER_LR);
		(*ppsImage)->pu16Palette[CHECK_INNER_LR] = CONVERT_24RGB_16RGB(COLOR_INNER_LR);
		(*ppsImage)->pu16Palette[CHECK_FILL] = u16BoxColor;
		(*ppsImage)->u16TransparentIndex = CHECK_TRANSPARENT;
	}

	pu8PixelBase = (*ppsImage)->pu8ImageData + u32XPos + (u32YPos * (*ppsImage)->u32Pitch);

	// Fill da box
	pu8PixelTemp = pu8PixelBase;
	u32Loop = u32BoxSize;
	while (u32Loop--)
	{
		memset((void *) pu8PixelTemp, CHECK_OUTER_LR, u32BoxSize);
		pu8PixelTemp += (*ppsImage)->u32Pitch;
	}

	// Time for top/left color
	pu8PixelTemp = pu8PixelBase;
	u32Loop = 0;
	while (u32Loop < u32BoxSize)
	{
		*(pu8PixelBase + u32Loop) = CHECK_OUTER_UL;
		*pu8PixelTemp = CHECK_OUTER_UL;
		pu8PixelTemp += (*ppsImage)->u32Pitch;
		u32Loop++;
	}

	// Time for right/bottom
	u32Loop = 0;
	pu8PixelTemp -= (*ppsImage)->u32Pitch;
	pu8PixelTemp2 = pu8PixelTemp + (u32BoxSize - 1);
	while (u32Loop < u32BoxSize)
	{
		*(pu8PixelTemp + u32Loop) = CHECK_OUTER_LR;
		*(pu8PixelTemp2) = CHECK_OUTER_LR;
		pu8PixelTemp2 -= (*ppsImage)->u32Pitch;
		u32Loop++;
	}

	// Time for inner top/left color
	pu8PixelTemp = pu8PixelBase + 1 + (*ppsImage)->u32Pitch;
	pu8PixelTemp2 = pu8PixelTemp;
	u32Loop = 0;
	while (u32Loop < (u32BoxSize - 2))
	{
		*(pu8PixelTemp + u32Loop) = CHECK_INNER_UL;
		*(pu8PixelTemp2) = CHECK_INNER_UL;
		pu8PixelTemp2 += (*ppsImage)->u32Pitch;
		u32Loop++;
	}

	// Timer for inner right/bottom color
	u32Loop = 0;
	pu8PixelTemp2 -= (*ppsImage)->u32Pitch;
	pu8PixelTemp += (u32BoxSize - 3);
	while (u32Loop < (u32BoxSize - 2))
	{
		*(pu8PixelTemp2 + u32Loop) = CHECK_INNER_LR;
		*(pu8PixelTemp) = CHECK_INNER_LR;
		pu8PixelTemp += (*ppsImage)->u32Pitch;
		u32Loop++;
	}

	// If it's an X, render it
	if (bChecked)
	{
		if (ECHECKBOX_X == eStyle)
		{
			pu8PixelTemp = (3 + (3 * (*ppsImage)->u32Pitch)) + pu8PixelBase;
			pu8PixelTemp2 = pu8PixelTemp + ((*ppsImage)->u32Pitch * (u32BoxSize - 7));

			u32Loop = 0;
			while (u32Loop < (u32BoxSize - 6))
			{
				*pu8PixelTemp = CHECK_FILL;
				if (u32Loop)
				{
					*(pu8PixelTemp - (*ppsImage)->u32Pitch) = CHECK_FILL;
					*(pu8PixelTemp - 1) = CHECK_FILL;
				}

				*pu8PixelTemp2 = CHECK_FILL;
				if (u32Loop)
				{
					*(pu8PixelTemp2 + (*ppsImage)->u32Pitch) = CHECK_FILL;
					*(pu8PixelTemp2 - 1) = CHECK_FILL;
				}

				pu8PixelTemp += (1 + (*ppsImage)->u32Pitch);
				pu8PixelTemp2 -= ((*ppsImage)->u32Pitch - 1);
				u32Loop++;
			}
		}
		else
		if (ECHECKBOX_SOLID == eStyle)
		{
			// Fill da box
			pu8PixelTemp = pu8PixelBase;
			u32Loop = u32BoxSize;
			while (u32Loop--)
			{
				memset((void *) pu8PixelTemp, CHECK_FILL, u32BoxSize);
				pu8PixelTemp += (*ppsImage)->u32Pitch;
			}
		}
		else
		if (ECHECKBOX_CHECKMARK == eStyle)
		{
			UINT32 u32DownStrideCount;
			UINT32 u32UpStrideCount;

// Percentage through check where it reverses direction (roughly 3/7)
#define		CHECK_MARK_EDGE		0x6db7

			GCASSERT(u32BoxSize >= 6);
			u32BoxSize -= 6;

			// Check mark
			pu8PixelTemp = 3 + ((3 + (u32BoxSize >> 1)) * (*ppsImage)->u32Pitch) + pu8PixelBase;
			u32DownStrideCount = ((u32BoxSize * CHECK_MARK_EDGE) + 0x8000) >> 16;
			u32UpStrideCount = (u32BoxSize - u32DownStrideCount) + 1;

			while (u32DownStrideCount--)
			{
				*pu8PixelTemp = CHECK_FILL;
				*(pu8PixelTemp - (*ppsImage)->u32Pitch) = CHECK_FILL;
				*(pu8PixelTemp + (*ppsImage)->u32Pitch) = CHECK_FILL;
				if (u32DownStrideCount)
				{
					pu8PixelTemp += (*ppsImage)->u32Pitch + 1;
				}
			}

			while (u32UpStrideCount--)
			{
				*pu8PixelTemp = CHECK_FILL;
				*(pu8PixelTemp - (*ppsImage)->u32Pitch) = CHECK_FILL;
				*(pu8PixelTemp + (*ppsImage)->u32Pitch) = CHECK_FILL;
				pu8PixelTemp -= ((*ppsImage)->u32Pitch - 1);
			}
		}
	}

	eErr = LERR_OK;

errorExit:
	if (eErr != LERR_OK)
	{
		GfxDeleteImage(*ppsImage);
		*ppsImage = NULL;
	}

	CheckboxRadioErrNormalize(&eErr);
	return(eErr);
}


ELCDErr CheckboxRenderImages(SImageGroup **ppsImageGroupUnchecked,
							 SImageGroup **ppsImageGroupChecked,
							 LEX_CHAR *peFontFilename,
							 UINT32 u32FontSize,
							 LEX_CHAR *peText,
							 UINT16 u16Orientation,
							 ECheckboxStyle eStyle,
							 UINT32 u32Color,
							 UINT32 u32BoxSize,
							 BOOL bRightSide)
{
	ELCDErr eErr = LERR_OK;
	UINT32 u32Length;
	FONTHANDLE eFont = HANDLE_INVALID;
	UINT32 u32TextXSize;
	UINT32 u32TextYSize;
	UINT32 u32BoxPos;
	SImage *psCheckedImage = NULL;
	SImage *psUncheckedImage = NULL;
	EGCResultCode eResult = GC_OK;
	UINT32 u32XPos = 0;
	UINT32 u32YPos = 0;
	UINT32 u32XSizeExtra = 0;
	UINT32 u32YSizeExtra = 0;

	*ppsImageGroupChecked = NULL;
	*ppsImageGroupUnchecked = NULL;

	// See if our checkbox style is valid
	if ((eStyle != ECHECKBOX_SOLID) &&
		(eStyle != ECHECKBOX_CHECKMARK) && 
		(eStyle != ECHECKBOX_X))
	{
		return(LERR_CHECKBOX_INVALID_STYLE);
	}

	// If the checkbox isn't a solid color, make it white, as we'll put an X or
	// a check mark on it later
	if (eStyle != ECHECKBOX_SOLID)
	{
		u32Color = 0;
	}

	// Figure out how big the text is
	eErr = FontCreate(peFontFilename,
					  u32FontSize,
					  0,
					  &eFont);

	if (eErr != LERR_OK)
	{
		goto errorExit;
	}

	eErr = FontGetStringSize(eFont,
							 peText,
							 &u32TextXSize,
							 &u32TextYSize,
							 (ERotation) u16Orientation,
							 TRUE);

	if (eErr != LERR_OK)
	{
		goto errorExit;
	}

	if ((ROT_90 == u16Orientation) ||
		(ROT_270 == u16Orientation))
	{
		u32Length = u32TextXSize;
	}
	else
	{
		u32Length = u32TextYSize;
	}

	// How all the pixels are
	if (u32Length >= u32BoxSize)
	{
		u32BoxPos = (u32Length >> 1) - (u32BoxSize >> 1);
	}
	else
	{
		u32BoxPos = 0;
	}
	
	*ppsImageGroupChecked = GfxImageGroupCreate();
	if (NULL == *ppsImageGroupChecked)
	{
		eErr = LERR_NO_MEM;
		goto errorExit;
	}

	*ppsImageGroupUnchecked = GfxImageGroupCreate();
	if (NULL == *ppsImageGroupUnchecked)
	{
		eErr = LERR_NO_MEM;
		goto errorExit;
	}

#define	CHECKBOX_SPACING		4

	if (ROT_0 == u16Orientation)
	{
		u32XPos = 0;
		u32YPos = u32BoxPos;
	}
	else
	if (ROT_180 == u16Orientation)
	{
		u32XPos = u32BoxPos;
		u32YPos = 0;
	}
	else
	if (ROT_90 == u16Orientation)
	{
		u32XPos = u32BoxPos;
		u32YPos = 0;
	}
	else
	if (ROT_270 == u16Orientation)
	{
		u32XPos = u32BoxPos;
		u32YPos = u32BoxPos;
	}

	if (FALSE == bRightSide)
	{
		// Add some space to the right of the checkbox
		u32XSizeExtra = CHECKBOX_SPACING;
	}
	else
	{
		// Add some space to the left of the checkbox
		u32XPos += CHECKBOX_SPACING;
	}

	// Render the boxes for the unchecked image (solid white)
	eErr = CheckBoxRender(u32BoxSize + u32XPos + u32XSizeExtra,
						  u32BoxSize + u32YPos + u32YSizeExtra,
						  u32BoxSize,
						  u32XPos,
						  u32YPos,
						  0xffffff,
						  &psUncheckedImage,
						  u16Orientation,
						  eStyle,
						  FALSE);
	if (eErr != LERR_OK)
	{
		goto errorExit;
	}

	eResult = GfxRotateImage(&psUncheckedImage,
							 (ERotation) u16Orientation);

	if (eResult != GC_OK)
	{
		eErr = LERR_NO_MEM;
		goto errorExit;
	}

	eErr = CheckBoxRender(u32BoxSize + u32XPos + u32XSizeExtra,
						  u32BoxSize + u32YPos + u32YSizeExtra,
						  u32BoxSize,
						  u32XPos,
						  u32YPos,
						  u32Color,
						  &psCheckedImage,
						  u16Orientation,
						  eStyle,
						  TRUE);
	if (eErr != LERR_OK)
	{
		goto errorExit;
	}

	eResult = GfxRotateImage(&psCheckedImage,
							 (ERotation) u16Orientation);

	if (eResult != GC_OK)
	{
		eErr = LERR_NO_MEM;
		goto errorExit;
	}

	if (NULL == GfxImageGroupAppend(*ppsImageGroupChecked,
									psCheckedImage))
	{
		eErr = LERR_NO_MEM;
		goto errorExit;
	}

	if (NULL == GfxImageGroupAppend(*ppsImageGroupUnchecked,
									psUncheckedImage))
	{
		eErr = LERR_NO_MEM;
		goto errorExit;
	}

errorExit:
	if (eErr != LERR_OK)
	{
		if (*ppsImageGroupUnchecked)
		{
			GfxDeleteImageGroup(*ppsImageGroupUnchecked);
			*ppsImageGroupUnchecked = NULL;
		}

		if (*ppsImageGroupChecked)
		{
			GfxDeleteImageGroup(*ppsImageGroupChecked);
			*ppsImageGroupChecked = NULL;
		}

	}

	// Ditch the font
	(void) FontFree(eFont);

	CheckboxRadioErrNormalize(&eErr);
	return(eErr);
}

ELCDErr CheckboxGroupSetDefaultImages(CHECKBOXGROUPHANDLE eCheckboxGroupHandle,
									  SImageGroup *psDefaultCheckedImage,
									  SImageGroup *psDefaultUncheckedImage,
									  BOOL bLockWindow)
{
	ELCDErr eErr = LERR_OK;
	SCheckbox *psCheckbox = NULL;
	SWidget *psWidget = NULL;

	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eCheckboxGroupHandle,
									WIDGET_CHECKBOX,
									&psWidget,
									NULL);
	RETURN_ON_FAIL(eErr);

	psCheckbox = psWidget->uWidgetSpecific.psCheckbox;
	GCASSERT(psCheckbox);

	eErr = RadioGroupSetDefaultImages(psCheckbox->eRadioGroupHandle,
									  psDefaultCheckedImage,
									  psDefaultUncheckedImage,
									  bLockWindow);

	CheckboxRadioErrNormalize(&eErr);
	return(eErr);
}

ELCDErr CheckboxGroupAdd(CHECKBOXGROUPHANDLE eCheckboxGroupHandle,
						 UINT32 u32XOrigin,
						 UINT32 u32YOrigin,
						 LEX_CHAR *peText,
						 UINT32 u32TextColor,
						 UINT32 u32CheckColor,
						 LEX_CHAR *peFontFilename,
						 UINT32 u32FontSize,
						 UINT16 u16Orientation,
						 ECheckboxStyle eStyle,
						 UINT32 u32CheckboxSquareSize,
						 BOOL bSelected,
						 BOOL bDisabled,
						 BOOL bVisible,
						 BOOL bSimple,
						 SImageGroup *psImageGroupChecked,
						 SImageGroup *psImageGroupUnchecked)
{
	ELCDErr eErr = LERR_OK;
	SCheckbox *psCheckbox = NULL;
	SWidget *psWidget = NULL;

	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eCheckboxGroupHandle,
									WIDGET_CHECKBOX,
									&psWidget,
									NULL);
	RETURN_ON_FAIL(eErr);

	psCheckbox = psWidget->uWidgetSpecific.psCheckbox;
	GCASSERT(psCheckbox);

	if (bSimple)
	{
		// Go render the checkbox images
		eErr = CheckboxRenderImages(&psImageGroupUnchecked,
									&psImageGroupChecked,
									peFontFilename,
									u32FontSize,
									peText,
									u16Orientation,
									eStyle,
									u32CheckColor,
									u32CheckboxSquareSize,
									psCheckbox->bRightBottomJustified);	

		if (eErr != LERR_OK)
		{
			goto errorExit;
		}
	}

	eErr = RadioGroupAdd(psCheckbox->eRadioGroupHandle,
						 u32XOrigin,
						 u32YOrigin,
						 peText,
						 u32TextColor,
						 peFontFilename,
						 u32FontSize,
						 u16Orientation,
						 bSelected,
						 bDisabled,
						 bVisible,
						 FALSE,
						 psImageGroupChecked,
						 psImageGroupUnchecked,
						 FALSE,
						 TRUE);

errorExit:
	CheckboxRadioErrNormalize(&eErr);
	return(eErr);
}

ELCDErr CheckboxSetSound(CHECKBOXGROUPHANDLE eCheckboxGroupHandle,
						 SOUNDHANDLE eSoundHandle)
{
	ELCDErr eErr;
	SWidget *psWidget;

	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eCheckboxGroupHandle,
									WIDGET_CHECKBOX,
									&psWidget,
									NULL);
	RETURN_ON_FAIL(eErr);

	return(RadioSetSound(psWidget->uWidgetSpecific.psCheckbox->eRadioGroupHandle,
						 eSoundHandle));
}


ELCDErr CheckboxGetSelected(CHECKBOXGROUPHANDLE eCheckboxGroupHandle,
							UINT32 u32Index,
							BOOL *pbState)
{
	ELCDErr eErr = LERR_OK;
	SWidget *psWidget = NULL;

	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eCheckboxGroupHandle,
									WIDGET_CHECKBOX,
									&psWidget,
									NULL);
	RETURN_ON_FAIL(eErr);

	eErr = RadioGroupGetSelected(psWidget->uWidgetSpecific.psCheckbox->eRadioGroupHandle,
								 &u32Index,
								 pbState);

	CheckboxRadioErrNormalize(&eErr);
	return(eErr);
}

ELCDErr CheckboxSetSelected(CHECKBOXGROUPHANDLE eCheckboxGroupHandle,
							UINT32 u32Index,
							BOOL bState)
{
	ELCDErr eErr = LERR_OK;
	SCheckbox *psCheckbox = NULL;
	SWidget *psWidget = NULL;

	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eCheckboxGroupHandle,
									WIDGET_CHECKBOX,
									&psWidget,
									NULL);
	RETURN_ON_FAIL(eErr);

	eErr = RadioGroupSetSelected(psWidget->uWidgetSpecific.psCheckbox->eRadioGroupHandle,
								 u32Index,
								 bState);

	CheckboxRadioErrNormalize(&eErr);
	return(eErr);
}

ELCDErr CheckboxGroupSetEnable(CHECKBOXGROUPHANDLE eCheckboxGroupHandle,
							   UINT32 *pu32Index,
							   BOOL bEnable)
{
	ELCDErr eErr = LERR_OK;
	SWidget *psWidget = NULL;

	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eCheckboxGroupHandle,
									WIDGET_CHECKBOX,
									&psWidget,
									NULL);
	RETURN_ON_FAIL(eErr);

	eErr = RadioGroupSetEnable(psWidget->uWidgetSpecific.psCheckbox->eRadioGroupHandle,
							   pu32Index,
							   bEnable);

	CheckboxRadioErrNormalize(&eErr);
	return(eErr);
}

ELCDErr CheckboxGroupSetHide(CHECKBOXGROUPHANDLE eCheckboxGroupHandle,
							 UINT32 *pu32Index,
							 BOOL bHidden)
{
	ELCDErr eErr = LERR_OK;
	SWidget *psWidget = NULL;

	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eCheckboxGroupHandle,
									WIDGET_CHECKBOX,
									&psWidget,
									NULL);
	RETURN_ON_FAIL(eErr);

	eErr = RadioGroupSetHide(psWidget->uWidgetSpecific.psCheckbox->eRadioGroupHandle,
							 pu32Index,
							 bHidden);

	CheckboxRadioErrNormalize(&eErr);
	return(eErr);
}

static SWidgetFunctions sg_sImageWidgetFunctions =
{
	NULL,						// Widget region test
	NULL,						// Paint our widget
	NULL,						// Erase our widget
	NULL,						// Press our widget
	NULL,						// Release our widget
	NULL,						// Mouseover our widget
	NULL,						// Focus
	NULL,						// Keystroke for our widget
	NULL,						// Animation widget!
	NULL,						// Intersection calc
	NULL,						// Mouse wheel
	NULL						// Set disable
};

static SWidgetTypeMethods sg_sImageMethods = 
{
	&sg_sImageWidgetFunctions,
	LERR_CHECKBOX_BAD_HANDLE,			// Error when it's not the handle we're looking for
	CheckboxWidgetAlloc,
	CheckboxWidgetFree
};

void CheckboxWidgetFirstTimeInit(void)
{
	// Nothing needed for first time init for the image widget subsystem
	DebugOut("* Initializing checkbox widget\n");
	WidgetRegisterTypeMethods(WIDGET_CHECKBOX,
							  &sg_sImageMethods);
}

