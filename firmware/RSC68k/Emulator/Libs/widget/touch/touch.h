#ifndef _TOUCH_H_
#define _TOUCH_H_

typedef struct STouch
{
	BOOL bTouched;					// Has this touch region been touched since the last read?
	BOOL bCurrentlyPressed;			// Set if the touch region is currently being pressed
} STouch;

extern void TouchRegionInit(void);

extern ELCDErr TouchRegionCreate(TOUCHREGIONHANDLE *peTouchRegionHandle,
								 WINDOWHANDLE eWindowHandle,
								 INT32 s32XPos,
								 INT32 s32YPos,
								 UINT32 u32XSize,
								 UINT32 u32YSize);
extern ELCDErr TouchRegionSetSize(TOUCHREGIONHANDLE eTouchRegionHandle,
								 INT32 s32XPos,
								 INT32 s32YPos,
								 UINT32 u32XSize,
								 UINT32 u32YSize);
extern ELCDErr TouchRegionDestroy(TOUCHREGIONHANDLE *peTouchRegionHandle);
extern ELCDErr TouchRegionGetInfo(TOUCHREGIONHANDLE eTouchRegionHandle,
								  BOOL *pbWasPressed,
								  BOOL *pbCurrentlyPressed);

#endif