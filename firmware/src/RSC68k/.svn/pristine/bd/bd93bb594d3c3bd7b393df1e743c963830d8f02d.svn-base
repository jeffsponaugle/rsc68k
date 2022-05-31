#ifndef _SLIDER_H_
#define _SLIDER_H_

typedef struct SSlider
{
	SLIDERHANDLE eSliderHandle;		// This slider's handle
	WINDOWHANDLE eWindowHandle;		// Window this button is related to

	INT32 s32CurrentSetting;		// Current slider setting
	INT32 s32LowValue;				// Low value for slider position
	INT32 s32HighValue;				// High value for slider position
	UINT16 u16Orientation;			// Rotational orientation
	BOOL bEnabled;					// TRUE If enabled

	SImageGroup *psThumb;			// Thumb images
	SImageGroup *psTicks;			// Tick images

	SOUNDHANDLE eSoundHandle;		// Sound handle for slider

	// Runtime/active data
	BOOL bPressed;					// TRUE If the slider is currently pressed
	UINT32 u32ThumbX;				// Thumb X position
	UINT32 u32ThumbY;				// Thumb Y position
	UINT32 u32ThumbTrack;			// Length of thumb track
	BOOL bLock;						// TRUE If slider is locked

	// Pointer to widget
	struct SWidget *psWidget;		// Pointer to parent widget
} SSlider;

typedef struct SWCBKSlider
{
	INT32 s32SliderValue;			// Slider value changed
} SWCBKSlider;

extern SSlider *SliderGetPointer(SLIDERHANDLE eHandle);
extern void SliderFirstTimeInit(void);
extern ELCDErr SliderCreate(WINDOWHANDLE eWindowHandle,
							SLIDERHANDLE *peSliderHandle,
							INT32 s32XPos,
							INT32 s32YPos,
							UINT16 u16Orientation);
extern ELCDErr SliderSetImages(SLIDERHANDLE eSliderHandle,
							   SImageGroup *psTicks,
							   SImageGroup *psThumb,
							   UINT32 u32TrackLength,
							   BOOL bLockWindow);
extern ELCDErr SliderRenderSimple(UINT32 u32SliderXSize,
								  UINT32 u32SliderYSize,
								  SImageGroup **ppsThumb,
								  SImageGroup **ppsSlider,
								  UINT32 u32ThumbColor,
								  UINT32 u32TrackColor,
								  UINT32 u32ThumbThickness,
								  UINT16 u16Orientation,
								  UINT32 u32TickCount);
extern ELCDErr SliderSetEnable(SLIDERHANDLE eSliderHandle,
							   BOOL bEnable);
extern ELCDErr SliderSetMinMax(SLIDERHANDLE eSliderHandle,
							   INT32 s32Min,
							   INT32 s32Max);
extern ELCDErr SliderSetValue(SLIDERHANDLE eSliderHandle,
							  INT32 s32Value);
extern ELCDErr SliderSetTrackLength(SLIDERHANDLE eSliderHandle,
									UINT32 u32TrackLength);
extern ELCDErr SliderSetLock(SLIDERHANDLE eSliderHandle,
							 BOOL bLock);
extern ELCDErr SliderGetValue(SLIDERHANDLE eSliderHandle,
							  INT32 *ps32Value);
extern ELCDErr SliderSetSound(SLIDERHANDLE eSliderHandle,
							  SOUNDHANDLE eSoundHandle);
extern ELCDErr SliderRenderStretch(SImageGroup **ppsCreatedTrack,
								   INT32 s32TrackLength,
								   SImageGroup *psTopLeft,
								   SImageGroup *psTrack,
								   SImageGroup *psBottomRight,
								   SImageGroup *psThumb,
								   UINT16 u16Orientation);
#endif // #ifndef _SLIDER_H_
