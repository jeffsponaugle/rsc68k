#ifndef _BAR_H_
#define _BAR_H_

typedef struct SBar
{
	INT32 s32XSize;				// Size of bar (x)
	INT32 s32YSize;				// Size of bar (y)
	UINT8 u8ShadowSize;			// Shadow size

	UINT16 u16TopShadowColor;		// Top shadow color
	UINT8 u8TopShadowTranslucency;	// Top shadow translucency (0xff=Invisible, 0x00=Solid)

	UINT16 u16SideShadowColor;		// Side shadow color
	UINT8 u8SideShadowTranslucency;	// Side shadow translucency (0xff=Invisible, 0x00=Solid)
	EElementOrient eOrientation;	// Orientation
} SBar;

struct SGraphSeries;

extern void BarInit(void);
extern void BarDrawRectangleWithImage(SWindow *psWindow,
									  UINT8 u8OriginalTranslucency,
									  UINT16 u16Color,
									  INT32 s32XPosTemp,
									  INT32 s32YPosTemp,
									  INT32 s32XSize,
									  INT32 s32YSize,
									  struct SGraphSeries *psGraphSeries);

#endif	// #ifndef _BAR_H_