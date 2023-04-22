#ifndef _ELEMENTS_H_
#define _ELEMENTS_H_

// Gotta have the window stuff in here
#include "Libs/window/window.h"

// Generic orientations
typedef enum
{
	EORIENT_NONE,
	EORIENT_UPPER_RIGHT,
	EORIENT_UPPER_LEFT,
	EORIENT_LOWER_RIGHT,
	EORIENT_LOWER_LEFT
} EElementOrient;

typedef enum
{
	ELEMTYPE_NONE,
	ELEMTYPE_BAR,
	ELEMTYPE_LINE_AA,
	ELEMTYPE_LINE_NORMAL,
	ELEMTYPE_TEXT,
	ELEMTYPE_IMAGE,
	ELEMTYPE_CIRCLE,
	ELEMTYPE_DOT,
	ELEMTYPE_COUNT
} EElementType;

struct SGfxElement;
struct SGraphSeries;

typedef struct SElementFuncs
{
	void (*ElemInit)(struct SGfxElement *psElement);
	void (*ElemDraw)(WINDOWHANDLE eWindow,
					 struct SGfxElement *psElement,
					 struct SGfxElement *psPriorElement,
					 struct SGraphSeries *psGraphSeries);
	void (*ElemFree)(struct SGraphSeries *psGraphSeries,
					 struct SGfxElement *psElement);
	void (*ElemComputeBounds)(struct SGfxElement *psElement,
							  UINT32 *pu32XSize,
							  UINT32 *pu32YSize);
} SElementFuncs;

#include "Libs/widget/elements/bar.h"
#include "Libs/widget/elements/line.h"
#include "Libs/widget/elements/TextElement.h"
#include "Libs/widget/elements/circle.h"
#include "Libs/widget/elements/ImageElement.h"
#include "Libs/widget/elements/dot.h"

typedef struct SGfxElement
{
	INT32 s32XOrigin;			// Origin of the element
	INT32 s32YOrigin;
	INT32 s32XPos;				// X/Y position of the element
	INT32 s32YPos;
	UINT32 u32XSize;			// X Size of element
	UINT32 u32YSize;			// Y Size of element
	EElementType eElementType;	// What element type is this?
	UINT16 u16DrawColor;		// Draw color RGB16
	UINT8 u8DrawTranslucency;	// Draw translucency (0xff=Invisible, 0x00=Solid)
	SElementFuncs *psFuncs;		// Pointer to element functions
	BOOL bHeapAllocated;		// TRUE=Allocated on the heap, FALSE=Not
	union UValue uValue;		// Current adjusted value of graph element
	union UValue uReferenceValue;	// Reference value
	UINT32 u32VirtualHeight;	// Virtual height value - 0xffff=Full height, 0=No height

	union
	{
		SBar sBar;				// For bars
		SLine sLine;			// And lines
		STextElement sText;		// Text element
		SCircle sCircle;		// Circle element
		SImageElement sImage;	// Image element
								// Nothing specific for dots
	} uEData;

	struct SGfxElement *psNextLink;	// Used for text/freeform objects
} SGfxElement;

// External functions for the elements
extern void ElementsInit(void);
extern SGfxElement *ElementCreate(EElementType eType,
								  INT32 s32XPos,
								  INT32 s32YPos,
								  UINT16 u16DrawColor,
								  SGfxElement *psElementSource);
extern void ElementRegister(EElementType eType,
						    SElementFuncs *psFuncs);
extern void ElementDraw(WINDOWHANDLE eWindow,
					    SGfxElement *psElement,
						SGfxElement *psPriorElement,
						struct SGraphSeries *psGraphSeries);
extern void ElementSetDrawColor(SGfxElement *psElement,
								UINT16 u16DrawColor);
extern void ElementSetDrawTranslucency(SGfxElement *psElement,
									   UINT8 u8DrawTranslucency);
extern void ElementSetPosition(SGfxElement *psElement,
							   INT32 s32XPos,
							   INT32 s32YPos);
extern void ElementCalculateBounds(SGfxElement *psElement,
								   INT32 *ps32XLow,
								   INT32 *ps32XHigh,
								   INT32 *ps32YLow,
								   INT32 *ps32YHigh);

#define	DRAW_PIXEL(x, y) { \
							UINT8 u8Red, u8Green, u8Blue; \
							u8Red = *(x) >> 11 ; u8Green = (*(x) >> 5) & 0x3f; u8Blue = *(x) & 0x1f; \
							*(x) = sg_u16RedGradientSaturation[((u8RedSrc * y) >> 8) + ((u8Red * (0xff - y)) >> 8)] | \
								   sg_u16GreenGradientSaturation[((u8GreenSrc * y) >> 8) + ((u8Green * (0xff - y)) >> 8)] | \
								   sg_u16BlueGradientSaturation[((u8BlueSrc * y) >> 8) + ((u8Blue * (0xff - y)) >> 8)]; \
						 }

#endif // #ifndef _ELEMENTS_H_