#ifndef _TEXT_H_
#define _TEXT_H_

#include "Application/RSC68k.h"

// Text structure

typedef struct SText
{
	TEXTHANDLE eTextHandle;				// This text's handle
	FONTHANDLE eFont;					// Pointer to font that this text object will use
	TEXTCHAR *peText;					// Text itself
	UINT16 u16Color;					// Color of the text
	BOOL bTransparent;					// Transparent text?
	UINT16 u16Orientation;				// Which way is the text oriented?

	LEX_CHAR *pu8FontFilename;			// Pointer to font filename
	UINT32 u32FontSize;					// Size of our font

	UINT32 u32BoundingBoxXSize;			// Bounding box X/Y size
	UINT32 u32BoundingBoxYSize;
	UINT32 u32TextXOffset;				// Text X/Y offset
	UINT32 u32TextYOffset;

	// Unhighlighted box color
	UINT32 u32BoxColor;					// Bounding box color (unhighlighted)

	// Text clip
	UINT32 u32TextXClip;				// X/Y Clipping for text
	UINT32 u32TextYClip;

	// Mouseover colors
	UINT32 u32MouseoverTextColor;		// Mouseover text color
	UINT32 u32MouseoverBoxColor;		// Mouseover box color

	// Pointer to widget
	struct SWidget *psWidget;			// Pointer to parent widget
} SText;

extern void TextFirstTimeInit(void);
extern SText *TextGetPointer(TEXTHANDLE eHandle);
extern ELCDErr TextCreate(TEXTHANDLE *peTextHandle,
						  WINDOWHANDLE eWINDOWHANDLE,
						  UINT16 u16Orientation);
extern ELCDErr TextSetText(TEXTHANDLE eTextHandle,
						   TEXTCHAR *peText);
extern ELCDErr TextGetText(TEXTHANDLE eTextHandle,
						  LEX_CHAR** ppeText);
extern ELCDErr TextSetTextASCII(TEXTHANDLE eTextHandle,
								char *pu8Text);
extern ELCDErr TextSetFont(TEXTHANDLE eTextHandle,
						   FONTHANDLE eFont);
extern ELCDErr TextGetFont(TEXTHANDLE eTextHandle,
						   FONTHANDLE *peFont);
extern ELCDErr TextSetColor(TEXTHANDLE eTextHandle,
							UINT16 u16Color);
extern ELCDErr TextGetTextSize(FONTHANDLE eFont,
							   TEXTCHAR *peText,
							   UINT32 *pu32XSize,
							   UINT32 *pu32YSize);
extern ELCDErr TextGetTextSizeASCII(FONTHANDLE eFont,
									char *pu8Text,
									UINT32 *pu32XSize,
									UINT32 *pu32YSize);
extern void TextGetActiveFontData(LEX_CHAR **ppu8ActiveFontFilename,
								  UINT32 *pu32ActiveFontSize);
extern UINT16 TextGetActiveFontColor(void);
extern ELCDErr TextDelete(TEXTHANDLE eTextHandle);
extern ELCDErr TextSetFixedFont(LEX_CHAR *pu8Filename,
								UINT32 *pu32FixedFontSize);
extern ELCDErr TextSetProportionalFont(LEX_CHAR *pu8Filename,
									   UINT32 *pu32FixedFontSize);
extern void TextGetFixedFontData(LEX_CHAR **ppeFixedFontFilename,
								 UINT32 *pu32FixedFontSize);
extern void TextGetProportionalFontData(LEX_CHAR **ppeFixedFontFilename,
										UINT32 *pu32FixedFontSize);
extern ELCDErr TextSetActiveFontData(LEX_CHAR *peActiveFontFilename,
									 UINT32 u32ActiveFontSize);

// New APIs

#define	TEXT_COLOR_NONE			0xffffffff			// Used for TextSetBoundingBoxColor() call when no bounding box color is desired

extern ELCDErr TextSetBoundingBoxSize(TEXTHANDLE eTextHandle,
									  UINT32 u32XSize,
									  UINT32 u32YSize);
extern ELCDErr TextSetTextOffset(TEXTHANDLE eTextHandle,
								 UINT32 u32XOffset,
								 UINT32 u32YOffset);
extern ELCDErr TextSetClipBox(TEXTHANDLE eTextHandle,
							  UINT32 u32XClip,
							  UINT32 u32YClip);
extern ELCDErr TextSetBoundingBoxColor(TEXTHANDLE eTextHandle,
									   UINT32 u32BoxColor,
		   							   UINT32 u32MouseoverBoxColor);
extern ELCDErr TextSetMouseoverColor(TEXTHANDLE eTextHandle,
									 UINT32 u32MouseoverTextColor);

// Text same as SWCBKPressRelease but vetted against the widget
typedef SWCBKPressRelease SWCBKText;

#endif	//#ifndef _TEXT_H_