#ifndef _FONTMGR_H_
#define _FONTMGR_H_

#include "Libs/window/window.h"
#include "Libs/freetype2/include/ft2build.h"
#include "Libs/Gfx/GraphicsLib.h"

#include FT_FREETYPE_H

typedef struct SFontFile
{	
	LEX_CHAR *peFontFilename;			// Name of font file on disk
	void *pvFontFileInMemory;			// Font file in memory!
	UINT32 u32FileSize;					// Size of font file in memory
	UINT32 u32References;				// # Of outstanding references to this file
	struct SFontFile *psNextLink;		// Pointer to next file in list
} SFontFile;

// The font image
typedef struct SFontCharacter
{
	UINT8 *pu8FontImageData;			// Pointer to font image data
	UINT32 u32Character;				// What character is this?
	UINT32 u32BitmapXSize;				// X/Y Size of bitmap in pixels, for this character
	UINT32 u32BitmapYSize;
	UINT32 u32XBearing;					// X/Y Bearing of font
	UINT32 u32YBearing;
	UINT32 u32XAdvance;					// X/Y Advance for this character
	UINT32 u32YAdvance;
	UINT32 u32YDescender;				// # Of pixels in the Y descender
	struct SFontCharacter *psNextLink;	// Next font image
} SFontCharacter;

// Font handles
typedef UINT32 FONTHANDLE;

// Text character
typedef unsigned char TEXTCHAR;

// The font itself
typedef struct SFont
{
	FONTHANDLE eFontHandle;				// This font's handle
	SFontFile *psFontFile;				// Font file structure
	SFontCharacter *psFontCharacters;	// Pointer to linked list of rendered characters
	UINT32 u32PixelSize;				// Font pixel size (height)
	UINT32 u32References;				// # Of references to this font
	INT32 s32Stride;					// # Of pixels 
	UINT32 u32MaxX;						// Largest character size (X coordinate)
	UINT32 u32MaxY;						// Largest character size (Y coordinate)
	UINT32 u32MinX;						// Smallest character size (X coordinate)
	UINT32 u32MinY;						// Smallest character size (Y coordinate)
	BOOL bKerning;						// Does this font set have kerning at all?
	struct SFont *psNextFont;			// Pointer to the next font
} SFont;


extern void FontMgrInit(void);
extern SFont *FontGetPointer(FONTHANDLE eHandle);
extern ELCDErr FontCreate(LEX_CHAR *pu8Filename,
						  UINT32 u32Size,
						  UINT32 u32Options,
						  FONTHANDLE *peFontHandle);
extern ELCDErr FontIncRef(FONTHANDLE eFontHandle);
extern ELCDErr FontFree(FONTHANDLE eFontHandle);
extern ELCDErr FontGetOverallSize(FONTHANDLE eFontHandle,
								  UINT32 *pu32XSize,
								  UINT32 *pu32YSize);
extern ELCDErr FontRender(FONTHANDLE eFontHandle,
					      WINDOWHANDLE eWINDOWHANDLE,
					      INT32 s32XPos,
					      INT32 s32YPos,
						  INT32 s32XSize,
						  INT32 s32YSize,
					      TEXTCHAR eCharacter,
					      UINT16 u16Color,
					      BOOL bTransparent,
						  INT32 *ps32XAdvance,
						  INT32 *ps32YAdvance,
						  BOOL bErase,
						  ERotation eRotation,
						  SImage *psBlitImage,
						  BOOL bDisabled,
						  BOOL bIncludeBearing);
extern ELCDErr FontGetSize(FONTHANDLE eFontHandle,
						   TEXTCHAR eCharacter,
						   UINT32 *pu32XSize,
						   UINT32 *pu32YSize,
						   INT32 *ps32XAdvance,
						   INT32 *ps32YAdvance,
						   ERotation eRotation,
						   BOOL bIncludeBearing);
extern ELCDErr FontGetStringSize(FONTHANDLE eFontHandle,
								 LEX_CHAR *peString,
								 UINT32 *pu32XSize,
								 UINT32 *pu32YSize,
								 ERotation eRotation,
								 BOOL bIncludeBearing);
extern ELCDErr FontGetStringSizeLen(FONTHANDLE eFontHandle,
									  LEX_CHAR *peString,
									  UINT32 u32StringLength,
									  UINT32 *pu32XSize,
									  UINT32 *pu32YSize,
									  ERotation eRotation,
									  BOOL bIncludeBearing);
extern ELCDErr FontGetStringSizeASCII(FONTHANDLE eFontHandle,
									  char *pu8String,
									  UINT32 *pu32XSize,
									  UINT32 *pu32YSize,
									  ERotation eRotation,
									  BOOL bIncludeBearing);
extern BOOL FontFreeUnused(void);

#endif // #ifndef _FONTMGR_H_