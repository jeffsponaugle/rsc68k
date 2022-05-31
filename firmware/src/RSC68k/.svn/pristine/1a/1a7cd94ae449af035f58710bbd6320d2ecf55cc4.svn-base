#ifndef _GRAPH_H_
#define _GRAPH_H_

#include "Application/RSC68k.h"
#include "Libs/widget/elements/elements.h"

typedef enum
{
	EORIGIN_TOP,		// 0
	EORIGIN_BOTTOM,		// 1
	EORIGIN_LEFT,		// 2
	EORIGIN_RIGHT		// 3
} EOrigin;

#define	GRID_LEFT		0x01
#define GRID_RIGHT		0x02
#define	GRID_TOP		0x04
#define	GRID_BOTTOM		0x08

typedef struct SGraphGrid
{
	UINT32 u32XOffset;
	UINT32 u32YOffset;
	UINT32 u32XSize;
	UINT32 u32YSize;
	UINT32 u32VerticalCount;
	UINT32 u32VerticalColor;
	UINT8 u8VerticalTranslucency;
	UINT8 u8VerticalThickness;
    UINT32 u32HorizontalCount;
    UINT32 u32HorizontalColor;
    UINT8 u8HorizontalTranslucency;
    UINT8 u8HorizontalThickness;
	UINT8 u8BoundMask;
} SGraphGrid;

typedef struct SGraphSeries
{
	INT32 s32XOffset;		// X/Y Position offset of the Series from the graph widget itself
	INT32 s32YOffset;
	INT32 s32XSize;			// X/Y Size of the Series's visual representation
	INT32 s32YSize;
	UINT32 u32Step;			// X/Y step in 16.16 fixed point
	BOOL bChangeable;		// Is the data user changeable
	BOOL bVisible;			// Is the Series visible?
	EVarType eVarType;		// Series's variable type

	// Boundaries (in window coordinates) of this entire Series
	INT32 s32XMin;
	INT32 s32XMax;
	INT32 s32YMin;
	INT32 s32YMax;

	GRAPHHANDLE eGraphHandle;	// Handle of parent graph widget
	GRAPHSERIESHANDLE eGraphSeriesHandle;	// Handle of this series

	void *pvSeriesData;		// Pointer to series data itself

	SGfxElement *psElements;	// Pointer to series elements

	SImageGroup *psSeriesImage;	// The image for this series

	SGfxElement *psSeriesTextElements;	// Text elements for this series

	UINT32 u32Head;			// Head index in series
	UINT32 u32Tail;			// Tail index in series
	UINT32 u32SeriesCount;	// # Of current items in the series

	union UValue uLow;		// Low point in value
	union UValue uHigh;		// High point in value
	union UValue uRange;	// Total range from high-low
	UINT32 u32SeriesElementSize;	// # Of total elements
	EOrigin eOrigin;			// Our origin (top/bottom/left/right)

	struct SGraphSeries *psNextLink;	// Pointer to next graph series (if any)
} SGraphSeries;

typedef struct SGraphWidget
{
	SGraphSeries *psUserSeriess;		// Pointer to user series
	GRAPHHANDLE eGraphHandle;		// This graph widget's handle
	BOOL bUpdateInProgress;			// Data or something having to do with the series is being modified - don't touch!
	BOOL bRecalcNeeded;				// We've updated something. Time to recalc.

	// Background image
	SImageGroup *psBackgroundImage;		// Pointer to active background image
	UINT32 u32BackgroundImageOffsetX;	// X Offset for background image
	UINT32 u32BackgroundImageOffsetY;	// Y Offset for background image

	// Grid
	SGraphGrid sGrid;				// Grid system?

	// Text annotation
	SGfxElement *psTextAnnotation;	// Pointer to annotation text elements

	// Pointer to widget
	struct SWidget *psWidget;		// Pointer to parent widget

	// Timer related stuff
	BOOL bUpdateTimersRunning;			// TRUE If update timer is running for this graph
	UINT32 u32MSSinceLastFIFOInsert;	// # Of milliseconds since last known FIFO insert
	UINT32 u32MSSinceLastUpdateRequest;	// # Of milliseconds since last update requestc

	struct SGraphWidget *psNextLink;	// Next graph widget
} SGraphWidget;

extern void GraphFirstTimeInit(void);
extern void GraphWidgetUpdate(GRAPHHANDLE eGraphHandle);
extern ELCDErr GraphWidgetCreate(WINDOWHANDLE eWindowHandle,
								 GRAPHHANDLE *peGraphHandle,
								 INT32 s32XPos,
								 INT32 s32YPos,
								 BOOL bVisible);
extern ELCDErr GraphSeriesCreate(GRAPHSERIESHANDLE *peGraphSeriesHandle,
								GRAPHHANDLE eGraphHandle,
								BOOL bChangeable,
								INT32 s32XOffset,
								INT32 s32YOffset,
								INT32 s32XSize,
								INT32 s32YSize,
								EVarType eVarType,
								union UValue *puMinValue,
								union UValue *puMaxValue,
								INT32 s32ElementCount,
								INT32 s32Origin,
								BOOL bVisible);
extern ELCDErr GraphInsertBar(GRAPHSERIESHANDLE eHandle,
							  SVar *psValue,
							  INT32 s32BarPixelWidth,
							  INT32 s32FrontColor,
							  INT32 s32TopColor,
							  INT32 s32SideColor,
							  INT32 s32ShadowPixelWidth,
							  INT32 s32TranslucencyValue,
							  BOOL bBackOfSeries);
extern ELCDErr GraphInsertLineAA(GRAPHSERIESHANDLE eHandle,
								 SVar *psValue,
								 INT32 s32LineColor,
								 INT32 s32TranslucencyValue,
								 BOOL bBackOfSeries,
								 BOOL bAreaFill,
								 UINT16 u16AreaFillColor,
								 UINT8 u8AreaFillTranslucency);
extern ELCDErr GraphInsertLineNormal(GRAPHSERIESHANDLE eHandle,
									 SVar *psValue,
									 INT32 s32LineColor,
									 INT32 s32TranslucencyValue,
									 BOOL bBackOfSeries,
									 BOOL bAreaFill,
									 UINT16 u16AreaFillColor,
									 UINT8 u8AreaFillTranslucency);
extern ELCDErr GraphInsertDot(GRAPHSERIESHANDLE eHandle,
							  SVar *psValue,
							  INT32 s32DotColor,
							  INT32 s32TranslucencyValue,
							  BOOL bBackOfSeries);
extern ELCDErr GraphSeriesSetVisible(GRAPHSERIESHANDLE eGraphSeriesHandle,
									BOOL bVisible);
extern ELCDErr GraphSeriesSetMinMax(GRAPHSERIESHANDLE eGraphSeriesHandle,
								   SVar *psMin,
								   SVar *psMax);
extern ELCDErr GraphInsertText(GRAPHSERIESHANDLE eHandle,
							   SVar *psValue,
							   LEX_CHAR *peText,
							   INT32 s32TextColor,
							   FONTHANDLE eFontHandle,
							   INT32 s32Origin,
							   BOOL bBackOfSeries);
extern ELCDErr GraphSetBackgroundImage(GRAPHHANDLE eGraphHandle,
									   LEX_CHAR *peFilename,
									   UINT32 u32XOffset,
									   UINT32 u32YOffset);
extern ELCDErr GraphSeriesSetImage(GRAPHSERIESHANDLE eHandle,
								  LEX_CHAR *peFilename);
extern void GraphSeriesCommit(SGfxElement *psElement,
							 SGraphSeries *psGraphSeries);
extern ELCDErr GraphSeriesDestroy(GRAPHSERIESHANDLE eHandle);
extern ELCDErr GraphDestroy(GRAPHHANDLE eHandle);
extern ELCDErr GraphTextCreate(GRAPHHANDLE eGraphHandle,
							   LEX_CHAR *peFontFilename,
							   UINT32 u32FontSize,
							   LEX_CHAR *peString,
							   UINT32 u32XOffset,
							   UINT32 u32YOffset,
							   UINT16 u16SColor,
							   UINT16 u16Orientation);
extern ELCDErr GraphTextDestroy(GRAPHHANDLE eGraphHandle);
extern ELCDErr GraphSeriesTextDestroy(GRAPHSERIESHANDLE eGraphHandle);
extern ELCDErr GraphSeriesTextCreate(GRAPHSERIESHANDLE eHandle,
									 FONTHANDLE eFontHandle,
									 LEX_CHAR *peString,
									 UINT32 u32XOffset,
									 UINT32 u32YOffset,
									 UINT16 u16TextColor,
									 UINT16 u16Orientation);
extern ELCDErr GraphGridCreate(GRAPHHANDLE eGraphHandle,
							   SGraphGrid *psGraphGrid);
extern ELCDErr GraphGridDestroy(GRAPHHANDLE eGraphHandle);

#endif // #ifndef _GRAPH_H_