#ifndef _TEXTELEMENT_H_
#define _TEXTELEMENT_H_

#include "Libs/FontMgr/FontMgr.h"

typedef struct STextElement
{
	LEX_CHAR *peText;			// Pointer to the text
	FONTHANDLE eFontHandle;		// Handle associated with the font we're using
	ERotation eRotation;		// Which direction does this text go?
} STextElement;

extern void TextElementSetText(struct SGfxElement *psElement,
							   LEX_CHAR *peString);
extern void TextElementSetFont(struct SGfxElement *psElement,
							   FONTHANDLE eFontHandle);
extern void TextElementSetRotation(struct SGfxElement *psElement,
								   ERotation eRot);
extern void TextElementInit(void);

#endif	// #ifndef _TEXTELEMENT_H_