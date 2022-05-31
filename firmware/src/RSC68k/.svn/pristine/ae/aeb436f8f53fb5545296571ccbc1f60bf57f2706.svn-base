#ifndef _WINDOW_H_
#define _WINDOW_H_

#include "Libs/Gfx/GraphicsLib.h"
#include "Application/LCDErr.h"
#include "Application/RSC68k.h"

// Command queue related

// Windowing command masks
#define	WCMD_MASK			0xf0000000
#define WDATA_MASK			(~WCMD_MASK)

// Windowing commands
#define WCMD_WINDOW_UPDATE			0x00000000
#define WCMD_WINDOW_KEY_HIT			0x10000000
#define WCMD_WINDOW_KEY_RELEASED	0x20000000
#define WCMD_WINDOW_HIDE			0x30000000
#define WCMD_WINDOW_SHOW			0x40000000
#define WCMD_WINDOW_GRAPH_UPDATE	0x50000000
#define WCMD_WINDOW_FORCE_BLIT		0x60000000
#define WCMD_WINDOW_CONSOLE_UPDATE	0x70000000
#define WCMD_WINDOW_SET_MODAL		0x80000000
#define WCMD_WINDOW_SHUTDOWN		0x90000000
#define WCMD_WINDOW_MOUSEWHEEL		0xa0000000

// Window command modifiers
#define WCMD_KEY_KEYDOWN	0x01000000

// Shadow related
typedef enum 
{
	SORG_TOP=0,
	SORG_LEFT,
	SORG_RIGHT,
	SORG_BOTTOM,
	SORG_COUNT
} EShadowOrigin;

typedef enum
{
	SORG_CORNER_UPPER_LEFT=0,
	SORG_CORNER_LOWER_LEFT,
	SORG_CORNER_UPPER_RIGHT,
	SORG_CORNER_LOWER_RIGHT,
	SORG_CORNER_ALL
} EShadowCornerOrigin;

typedef enum
{
	SSTYLE_NONE=0,						// No shadow, dude!
	SSTYLE_LINE,						// Straightforward solid line
	SSTYLE_LINEAR,						// Linear fade toward the edges
	SSTYLE_ROUNDED,						// The rounded look
	SSTYLE_COUNT
} EShadowStyle;

typedef struct SShadow
{
	BOOL bActive;						// Is this shadow region active?
	EShadowStyle eStyle;				// What style is this shadow
	UINT8 u8Intensity;					// How intense is this shadow?
	UINT32 u32Thickness;				// How many pixels thick is it?
	UINT32 u32Length;					// Length of shadow
	INT32 s32Offset;					// Offset (on whatever axis this is)
	UINT16 u16Color;					// Color of the shadow
} SShadow;

typedef struct SBorder
{
	EShadowStyle eBorder;				// What style is this border
	UINT32 u32XPos;						// X Position of border
	UINT32 u32YPos;						// Y Position of border
	UINT32 u32XSize;					// X Size of the border
	UINT32 u32YSize;					// Y Size of the border
} SBorder;

// Window structure - the whole world starts here

typedef struct SWindow
{
	// General window properties
	UINT16 u16ForegroundColor;			// Default foreground color
	UINT16 u16BackgroundColor;			// Default background color
	BOOL bVisible;						// Is the window visible or not?
	UINT8 u8WindowIntensity;			// Window intensity (0xff=full, 0x00=dim)
	SOSSemaphore sWindowUpdateSem;		// Window updating semaphore
	WINDOWHANDLE eWindowHandle;			// The handle for this window

	// Coordinates
	UINT32 u32ActiveAreaXSize;			// Size of active area
	UINT32 u32ActiveAreaYSize;
	UINT32 u32ActiveAreaXPos;			// Position of active area
	UINT32 u32ActiveAreaYPos;

	// Viewport
	UINT32 u32ViewportXSize;			// size of window viewport
	UINT32 u32ViewportYSize;
	UINT32 u32ViewportXOffset;			// offset into active area of window viewport start
	UINT32 u32ViewportYOffset;

	// Shadow related
	SShadow sShadow[SORG_COUNT];		// All shadow sides/corners (if any)

	// Window image
	SImage *psWindowImage;
	SImageInstance *psWindowImageInstance;

	// Background image group (if applicable)
	SImageGroup *psBackgroundImage;
	UINT8 u8BackgroundIntensityLevel;	 // 255=Full, 0=Dark

	// Widgets!
	struct SWidget *psWidgetList;		// List of widgets attached to this window

	// Dirty region
	SOSSemaphore sDirtyRegionSem;		// Dirty region semaphore
	UINT32 u32UpdateRegionXMin;			// Bounding box for a needed update
	UINT32 u32UpdateRegionYMin;
	UINT32 u32UpdateRegionXMax;
	UINT32 u32UpdateRegionYMax;

	// Pointer to parent window
	WINDOWHANDLE eParentWindow;			// HANDLE_INVALID if there is no parent window

	// Pointer to widget head
	struct SWidget *psWidgetHead;		// Pointer to a linked list of widgets

	// Pointer to next window in the priority/layer list
	struct SWindow *psNextWindow;
} SWindow;

// Function prototypes

extern void WindowInit(void);
extern void WindowBlitLock(BOOL bLock);
extern void WindowSetOrientation(ERotation eOrientation);
extern ELCDErr WindowCreate(WINDOWHANDLE eParentWindow,
							INT32 s32XPos,
							INT32 s32YPos,
							UINT32 u32XSize,
							UINT32 u32YSize,
							WINDOWHANDLE *peWindowHandle);
extern ELCDErr WindowDelete(WINDOWHANDLE eWINDOWHANDLE);
extern ELCDErr WindowSetViewport(WINDOWHANDLE eWindow,
							 UINT32* pu32XOffset,
							 UINT32* pu32YOffset,
							 UINT32* pu32XSize,
							 UINT32* pu32YSize);
extern ELCDErr WindowSetViewportScrollPercent (WINDOWHANDLE eWindow,
										  UINT32* pu32XOffsetPercent,
										  UINT32* pu32YOffsetPercent);
extern ELCDErr WindowSetPosition(WINDOWHANDLE eWindowHandle,
								 INT32 s32XPos,
								 INT32 s32YPos);
extern ELCDErr WindowSetBackgroundImage(WINDOWHANDLE eWindow,
										LEX_CHAR *peFilename);
extern ELCDErr WindowSetBackgroundImageIntensity(WINDOWHANDLE eWindow,
												 UINT8 u8IntensityLevel);
extern ELCDErr WindowSetVisible(WINDOWHANDLE eWindow,
								BOOL bWindowVisible);
extern ELCDErr WindowSetBackgroundColor(WINDOWHANDLE eWindow,
										UINT16 u16RGBColor);
extern ELCDErr WindowSetForegroundColor(WINDOWHANDLE eWindow,
										UINT16 u16RGBColor);
extern ELCDErr WindowSetTransparency(WINDOWHANDLE eWindow,
									 UINT8 u8TransparencyLevel);
extern ELCDErr WindowFill(WINDOWHANDLE eWindow,
						  UINT16 u16FillColor);
extern ELCDErr WindowSetShadow(WINDOWHANDLE eWINDOWHANDLE,
							   EShadowStyle eStyle,
							   EShadowOrigin eOrigin,
							   UINT32 u32Thickness,
							   INT32 s32Offset,
							   UINT32 u32Length,
							   UINT16 u16Color);
extern SWindow *WindowGetPointer(WINDOWHANDLE eHandle);
extern void WindowEraseActiveRegion(SWindow *psWindow,
									INT32 s32XPos,
									INT32 s32YPos,
									INT32 s32XSize,
									INT32 s32YSize);
extern ELCDErr WindowColorFillRegion(WINDOWHANDLE eWindowHandle,
									 UINT16 u16Color,
									 INT32 s32XPos,
									 INT32 s32YPos,
									 INT32 s32XSize,
									 INT32 s32YSize,
									 BOOL bLockWindow);
extern void WindowUpdateRegion(WINDOWHANDLE eWindow,
							   INT32 s32XPos,
							   INT32 s32YPos,
							   INT32 s32XSize,
							   INT32 s32YSize);
extern void WindowUpdateRegionCommit(void);
extern ELCDErr WindowWidgetConnect(WINDOWHANDLE eWINDOWHANDLE,
							       struct SWidget *psWidget);
extern ELCDErr WindowWidgetDisconnect(WINDOWHANDLE eWindowHandle,
									  struct SWidget *psWidgetToDelete);
extern void WindowButtonPressed(UINT32 u32ButtonMask,
								UINT32 u32XPointerPos,
								UINT32 u32YPointerPos);
extern void WindowButtonReleased(UINT32 u32ButtonMask,
								 UINT32 u32XPointerPos,
								 UINT32 u32YPointerPos);
extern void WindowPointerUpdate(UINT32 u32ButtonMask,
							    UINT32 u32XPos,
								UINT32 u32YPos);
extern ELCDErr WindowLock(SWindow *psWindow);
extern ELCDErr WindowUnlock(SWindow *psWindow);
extern ELCDErr WindowLockByHandle(WINDOWHANDLE eHandle);
extern ELCDErr WindowUnlockByHandle(WINDOWHANDLE eHandle);
extern void WindowAnimationTick(UINT32 u32TickTime);
extern ELCDErr WindowAnimationListAdd(WINDOWHANDLE eWindowHandle,
									  struct SWidget *psWidget);
extern void WindowAnimationListDelete(WINDOWHANDLE eWindowHandle,
									  struct SWidget *psWidget);
extern ELCDErr WindowDepositMessage(UINT32 u32Message);
extern ELCDErr WindowPriorityGet(WINDOWHANDLE eWindowHandle,
								 UINT32 *pu32Priority);
extern ELCDErr WindowPrioritySet(WINDOWHANDLE eWindowHandle,
								 UINT32 u32Priority,
								 BOOL bFront);
extern ELCDErr WindowSetShadowUI(WINDOWHANDLE eWINDOWHANDLE,
								 EShadowStyle eStyle,
								 EShadowCornerOrigin eOrigin,
								 UINT32 u32Thickness,
								 UINT16 u16Color);
extern ELCDErr WindowShutdown(void);
extern ELCDErr WindowGetPixel(WINDOWHANDLE eWindowHandle,
							  INT32 s32XPos,
							  INT32 s32YPos,
							  UINT16 *pu16Pixel);
extern void WindowSuspendBlits(BOOL bSuspend);
extern ELCDErr WindowSetModal(WINDOWHANDLE eWindowHandle);
extern ELCDErr WindowKeyHit(EGCCtrlKey eGCKey, LEX_CHAR eKey, BOOL bPressed);
extern ELCDErr WindowLockByHandleIfNotSubordinate(WINDOWHANDLE eWindow,
												  struct SWidget *psWidget);
extern ELCDErr WindowUnlockByHandleIfNotSubordinate(WINDOWHANDLE eWindow,
													struct SWidget *psWidget);
extern void WindowSetMousewheel(UINT32 u32Value);

#endif // #ifndef _WINDOW_H_