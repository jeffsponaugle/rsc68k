#ifndef _BUTTON_H_
#define _BUTTON_H_

#include "Libs/Sound/sound.h"
#include "Libs/Sound/WaveMgr.h"

typedef enum
{
	BUTTON_STATE_BOGUS,
	BUTTON_STATE_ENABLED_NORMAL,
	BUTTON_STATE_ENABLED_PRESSED,
	BUTTON_STATE_DISABLED_NORMAL,
	BUTTON_STATE_DISABLED_PRESSED
} EButtonState;

typedef struct SButton
{
	BUTTONHANDLE eButtonHandle;		// This button's handle

	// Button state info
	EButtonState eButtonState;		// Current button's visual state
	BOOL bButtonSticky;				// TRUE If the button is a "sticky" type of button, FALSE if momentary

	// Normal button information
	INT32 s32XOffsetNormal;			// X/Y Offset when normal
	INT32 s32YOffsetNormal;
	BOOL bNormalRendered;			// TRUE If the button library created the normal image
	BOOL bHitMaskNormal;			// Set to true if we pay attention to the normal image hit mask
	SImageGroup *psNormal;			// Pointer to normal image
	UINT8 *pu8NormalHitMask;		// 0=No hit, 1=Hit - directly proportional to the psNormal image size

	// Pressed button information
	INT32 s32XOffsetPressed;		// X/Y Offset when pressed
	INT32 s32YOffsetPressed;
	BOOL bPressedRendered;			// TRUE If the button library created the pressed image
	BOOL bHitMaskPressed;			// Set to true if we pay attention to the pressed image hit mask
	SImageGroup *psPressed;			// Pointer to pressed image
	UINT8 *pu8PressedHitMask;		// 0=No hit, 1=Hit - directly proportional to the psPressed image size

	// State indicator
	INT32 s32XOffsetIndicator;		// X/Y Offset for indicator
	INT32 s32YOffsetIndicator;
	SImageGroup *psIndicator;
	BOOL bIndicatorOn;

	// Sound related
	SOUNDHANDLE eButtonDownWave;		// Button down sound
	SOUNDHANDLE eButtonUpWave;		// Button up sound

	// Pointer to window handle 
	WINDOWHANDLE eWindowHandle;		// Window this button is related to

	// Pointer to widget
	struct SWidget *psWidget;		// Pointer to parent widget
} SButton;

extern void ButtonFirstTimeInit(void);
extern ELCDErr ButtonCreate(WINDOWHANDLE eWindowHandle,
							BUTTONHANDLE *peButtonHandle,
							INT32 s32XPos,
							INT32 s32YPos);
extern ELCDErr ButtonSetNormalImage(BUTTONHANDLE eButtonHandle,
									SImageGroup *psButtonImageGroup,
									INT32 s32XOffset,
									INT32 s32YOffset);
extern ELCDErr ButtonSetPressedImage(BUTTONHANDLE eButtonHandle,
									 SImageGroup *psButtonImageGroup,
									 INT32 s32XOffset,
									 INT32 s32YOffset);
extern ELCDErr ButtonStateSet(BUTTONHANDLE eButtonHandle,
							  EButtonState eState);
extern ELCDErr ButtonSetCallback(BUTTONHANDLE eButtonHandle,
								 void (*pCallback)(BUTTONHANDLE eButtonHandle,
												   BOOL bButtonState));
extern ELCDErr ButtonSetSticky(BUTTONHANDLE eButtonHandle,
							   BOOL bSticky);
extern ELCDErr ButtonSetIndicatorImage(BUTTONHANDLE eButtonHandle,
									   SImageGroup *psIndicatorImageGroup,
									   INT32 s32XOffset,
									   INT32 s32YOffset);
extern ELCDErr ButtonSetIndicatorState(BUTTONHANDLE eButtonHandle,
									   BOOL bIndicatorState);
extern ELCDErr ButtonSetHitMaskNormal(BUTTONHANDLE eButtonHandle);
extern ELCDErr ButtonSetHitMaskPressed(BUTTONHANDLE eButtonHandle);
extern ELCDErr ButtonSetButtonDownWave(BUTTONHANDLE eButtonHandle,
									   SOUNDHANDLE eWaveHandle);
extern ELCDErr ButtonSetButtonUpWave(BUTTONHANDLE eButtonHandle,
									 SOUNDHANDLE eWaveHandle);
extern ELCDErr ButtonSetHide(BUTTONHANDLE eButtonHandle,
							 BOOL bHidden);
extern ELCDErr ButtonGetState(BUTTONHANDLE eButtonHandle,
							  EButtonState *peState);
extern ELCDErr ButtonGetDisabled(BUTTONHANDLE eButtonHandle,
								 BOOL *pbButtonDisabled);
extern ELCDErr ButtonRender(UINT32 u32XSize,
							UINT32 u32YSize,
							LEX_CHAR *pu8FontFilename,
							UINT32 u32FontSize,
							LEX_CHAR *pu8Text,
							UINT32 u32TextColor,
							UINT32 u32ButtonColor,
							SImageGroup **ppsImageGroup,
							BOOL bDown,
							UINT16 u16Orientation);

extern ELCDErr ButtonWidgetAnimateStart(BUTTONHANDLE eButtonWidgetHandle);
extern ELCDErr ButtonWidgetAnimateStop(BUTTONHANDLE eButtonWidgetHandle);
extern ELCDErr ButtonWidgetAnimateStep(BUTTONHANDLE eButtonWidgetHandle);
extern ELCDErr ButtonWidgetAnimateReset(BUTTONHANDLE eButtonWidgetHandle);

// Button same as SWCBKPressRelease but vetted against button enable/disable
typedef SWCBKPressRelease SWCBKButton;

#endif	//#ifndef _BUTTON_H_