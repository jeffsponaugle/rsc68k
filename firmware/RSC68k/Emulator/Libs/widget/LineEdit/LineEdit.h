#ifndef _LINEEDIT_H_
#define _LINEEDIT_H_

#include "Application/RSC68k.h"

typedef enum
{
	LineEdit_IsPassword = 0,
	LineEdit_IsNumeric,
} ELineEditParam;

// Line edit structure

typedef struct SLineEdit
{
	LEX_CHAR* peText;					// Current textual content of the line edit.
	UINT16 u16MaxLength;				// Maximum length in number of characters (not including the null character)

	FONTHANDLE eFont;					// Pointer to font that this text object will use
	UINT16 u16Color;					// Color of the text

	BOOL bCursorEnabled;				// Only relevant when the widget has focus (is selected.)
	BOOL bCursorVisible;				// Is the cusror visible right now?
	UINT16 u16CursorPosition;			// Position of the cursor: range 0 (before the first character) to last character index + 1.
	UINT32 u32MouseClickX;				// X parameter of where the mouse was clicked.  LINE_EDIT_MOUSE_X_INVALID if not valid.

	UINT16 u16ViewportStartChar;		// When the text is too long for the LineEdit, which character will we start with in the display?
	UINT16 u16ViewportLen;				// Number of characters we plan to display

	UINT32 u32BoundingBoxXSize;			// Bounding box size (drawing directly into the window since widget size is used for the text clipping only)
	UINT32 u32BoundingBoxYSize;
	UINT32 u32TextXOffset;				// Text X/Y offset
	UINT32 u32TextYOffset;

	UINT32 u32BoxColor;					// Bounding box color

	BOOL bIsPassword;					// If it's a password, only display '*'
	BOOL bIsNumeric;					// Allow only digts and some punctuation: .,-

	BOOL (*pfKeypressCallback)(LINEEDITHANDLE eWidgetHandle, 
							   EGCCtrlKey eGCKey, 
							   LEX_CHAR eUnicode);	// Registered user keypress callback for this LineEdit

	SOSSemaphore sParameterLock;

	// Pointer to widget
	struct SWidget *psWidget;			// Pointer to parent widget
} SLineEdit;

extern void LineEditFirstTimeInit(void);

// APIs for managing the LineEdit widget 
extern ELCDErr LineEditCreate(LINEEDITHANDLE *peLineEditHandle,		// Pointer to widget handle
							   WINDOWHANDLE eWindowHandle,				// Associated window handle
							   FONTHANDLE eFontHandle,					// Font used
							   INT32 s32XPos,							// X/Y Position of widget
							   INT32 s32YPos,
							   UINT32 u32XSize,							// X/Y Size of text clipping area (Not related to bounding box size)
							   UINT32 u32YSize,
							   BOOL bVisible);							// TRUE=Widget visible, FALSE=Not visible (to start with)
extern ELCDErr LineEditSetBoundingBoxSize(LINEEDITHANDLE eLineEditHandle,
										   UINT32 u32XSize,
										   UINT32 u32YSize);
extern ELCDErr LineEditSetBoundingBoxColor(LINEEDITHANDLE eLineEditHandle,
										   UINT32 u32BoxColor);
extern ELCDErr LineEditSetTextOffset(LINEEDITHANDLE eLineEditHandle,
									 UINT32 u32XOffset,
									 UINT32 u32YOffset);
extern ELCDErr LineEditSetMaxLength(LINEEDITHANDLE eLineEditHandle, 
									UINT16 u16MaxLength);
extern ELCDErr LineEditSetText(LINEEDITHANDLE eLineEditHandle,
							   LEX_CHAR *peText);
extern ELCDErr LineEditGetText( LINEEDITHANDLE eLineEditHandle, 
								LEX_CHAR** ppeNewText );
extern ELCDErr LineEditSetTextColor(LINEEDITHANDLE eLineEditHandle,
									UINT16 u16Color);
extern ELCDErr LineEditSetParam(LINEEDITHANDLE eLineEditHandle,
								 ELineEditParam eParam,
								 void* pvParamData);
extern ELCDErr LineEditSetUserKeystrokeCallback( LINEEDITHANDLE eLineEditHandle,
												  BOOL (*pfKeypressCallback)(LINEEDITHANDLE eWidgetHandle, 
																			 EGCCtrlKey eGCKey, 
																			 LEX_CHAR eUnicode) );
extern ELCDErr LineEditGetDimensions( LINEEDITHANDLE eLineEditHandle, 
									  INT32* ps32XStart, INT32* ps32YStart,
									  UINT32* pu32XSize, UINT32* pu32YSize );

#endif	//#ifndef _LINEEDIT_H_