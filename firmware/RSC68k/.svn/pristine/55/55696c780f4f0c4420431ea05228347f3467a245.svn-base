#ifndef _CHECKBOX_H_
#define _CHECKBOX_H_

// Relies on the radio library
#include "Libs/widget/radio/radio.h"

typedef struct SCheckbox
{
	RADIOGROUPHANDLE eRadioGroupHandle;
	BOOL bRightBottomJustified;
} SCheckbox;

typedef enum 
{
	ECHECKBOX_CHECKMARK,
	ECHECKBOX_X,
	ECHECKBOX_SOLID
} ECheckboxStyle;

extern ELCDErr CheckboxGroupCreate(CHECKBOXGROUPHANDLE *peCheckboxHandle,
								   WINDOWHANDLE eWindowHandle,
								   UINT32 u32XPos,
								   UINT32 u32YPos,
								   BOOL bRightBottomJustified,
								   BOOL bVisible);
extern ELCDErr CheckboxRenderImages(SImageGroup **ppsImageGroupUnchecked,
									SImageGroup **ppsImageGroupChecked,
									LEX_CHAR *peFontFilename,
									UINT32 u32FontSize,
									LEX_CHAR *peText,
									UINT16 u16Orientation,
									ECheckboxStyle eStyle,
									UINT32 u32Color,
									UINT32 u32CheckboxSize,
									BOOL bRightSide);
extern ELCDErr CheckboxGroupSet(CHECKBOXGROUPHANDLE eCheckboxHandle,
								LEX_CHAR *peText,
								UINT32 u32TextColor,
								LEX_CHAR *peFontFilename,
								UINT32 u32FontSize,
								UINT16 u16Orientation,
								BOOL bDisabled,
								BOOL bVisible,
								SImageGroup *psImageGroupUnchecked,
								SImageGroup *psImageGroupChecked);
extern ELCDErr CheckboxGroupSetDefaultImages(CHECKBOXGROUPHANDLE eCheckboxGroupHandle,
											 SImageGroup *psDefaultCheckedImage,
											 SImageGroup *psDefaultUncheckedImage,
											 BOOL bLockWindow);
extern ELCDErr CheckboxGroupAdd(CHECKBOXGROUPHANDLE eCheckboxGroupHandle,
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
								SImageGroup *psImageGroupUnchecked);
extern ELCDErr CheckboxGroupDestroy(CHECKBOXGROUPHANDLE *peRadioGroupHandle);
extern ELCDErr CheckboxSetSelected(CHECKBOXGROUPHANDLE eCheckboxGroupHandle,
								   UINT32 u32Index,
								   BOOL bState);
extern ELCDErr CheckboxGetSelected(CHECKBOXGROUPHANDLE eCheckboxGroupHandle,
								   UINT32 u32Index,
								   BOOL *pbState);
extern ELCDErr CheckboxGroupSetEnable(CHECKBOXGROUPHANDLE eCheckboxGroupHandle,
									  UINT32 *pu32Index,
									  BOOL bEnable);
extern ELCDErr CheckboxGroupSetHide(CHECKBOXGROUPHANDLE eCheckboxGroupHandle,
									UINT32 *pu32Index,
									BOOL bHidden);
extern void CheckboxWidgetFirstTimeInit(void);
extern ELCDErr CheckboxSetSound(CHECKBOXGROUPHANDLE eCheckboxGroupHandle,
								SOUNDHANDLE eSoundHandle);

#endif	// #ifndef _RADIO_H_