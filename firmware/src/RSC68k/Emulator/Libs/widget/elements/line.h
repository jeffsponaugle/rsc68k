#ifndef _LINE_H_
#define _LINE_H_

typedef struct SLine
{
	INT32 s32X0;				// From
	INT32 s32Y0;
	INT32 s32X1;				// To
	INT32 s32Y1;
	UINT16 u16DrawColor;		// Draw color
	UINT32 u32Intensity;		// 0-255 Line intensity
	BOOL bAreaFill;				// TRUE If we fill area
	UINT16 u16AreaFillColor;	// Our area fill color
	UINT8 u8AreaFillTranslucency;	// 0xff=Not at all, 0x00=Fully
} SLine;

extern void LineInit(void);
#endif	// #ifndef _LINE_H_