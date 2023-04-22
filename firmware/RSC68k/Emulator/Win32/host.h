#ifndef _HOST_H_
#define _HOST_H_
#include "Win32/SDL/sdl.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>

typedef UINT32 GOSCPUIntContext;


extern CRITICAL_SECTION g_sVideoCriticalSection;

/* ************************************************************************* */
typedef struct
{
	UINT32	w;
	UINT32	h;
        UINT16	pitch;
	UINT8	u8ColorDepth;
	void*	pixels;
} Surface;

/* ************************************************************************* */
typedef struct
{
    float x,y,z;
    float u,v;
} Vertex;

void SetVertex(Vertex* in_pVtx,
                float in_x, float in_y, float in_z,
                float in_u, float in_v);
UINT32 GLGetTempTexture( void );
BOOL GLBlitRect( float r, float t, float l, float b );

/* ************************************************************************* */
typedef struct
{
    BOOL	bModeSet;
    BOOL	b2DGraphics;
    BOOL    bVertMonitor;

    // X Windows Configuration
	HDC			hDisplay;
    HWND	    hWnd;
    HGLRC		hGlContext;

    INT32       s32AttributeMask;

    // Screen Size
    INT32 ScreenWidth;
    INT32 ScreenHeight;

    // Back Buffer
    INT32 BackBufferWidth;
    INT32 BackBufferHeight;
    INT32 BackBufferColorDepth;

	// Output Size
	INT32 OutputWidth;
	INT32 OutputHeight;

} GfxLocals;

/* ************************************************************************* */
extern GfxLocals g_sGfxLocals;

BOOL cglCheckErrorInternal(const char*, int);
#define cglCheckError() cglCheckErrorInternal( __FUNCTION__, __LINE__)
#define cglCheckErrorEx(x) cglCheckErrorInternal(x, __LINE__)

BOOL cglMakeCurrentThread( BOOL in_MakeCurrent );
void cglSetGLAware( BOOL in_Aware );

extern void HostDeleteBackBuffer(void *pvBackBuffer);
extern void *HostCreateBackBuffer(UINT32 u32XSize,
								  UINT32 u32YSize,
								  UINT8 u8ColorDepth);
extern UINT32 HostGetBackBufferPitch(void);
extern BOOL HostGfxInit(void);
extern void HostGfxShutdown(void);
extern void HostInputInit(void);
extern void HostSetPaletteEntry(UINT32 u32Entry,
								UINT8 u8Red,
								UINT8 u8Green,
								UINT8 u8Blue);
extern void HostSetDisplaySurface(UINT32 u32XSize,
							      UINT32 u32YSize,
								  UINT8 u8ColorDepth,
								  BOOL bWindowed,
								  BOOL bOpenGL);
extern void HostGameFrame(void);
extern void HostBlit(SDL_Rect *psSource,
					 SDL_Rect *psDest);
extern void HostProcessFrame(void);
extern void HostSetFrameRate(UINT32 u32FPS);
extern void HostThrottleAndBlit(void);
extern void HostSetGameExitCallbackProcedure(void (*pGameExitCallbackProc)(void));
extern EGCResultCode HostConvertScreenPosition(UINT32* pu32X, UINT32* pu32Y);
extern void HostProcessMessages(void);
extern EGCResultCode HostMinimizeApplication( void );

struct SCommandLineOption;

extern void GCStartup(UINT32 u32FreeMemoryBase,
					  UINT32 u32Size,
					  UINT8 *pu8CmdLine,
					  struct SCommandLineOption *psOptions);

extern void SetKeyState(SDL_keysym sKeySymbol, BOOL bMake);
extern void SetKeyCallback(BOOL (*pKeyCallback)(SDL_keysym, BOOL));
extern void SetJoyAxisState(UINT8 joy, UINT8 axis, INT16 value );
extern void SetJoyButtonState(UINT8 joy, UINT8 button, UINT8 bMake);
extern void SetJoyHatState(UINT8 joy, UINT8 hat, UINT8 value );
extern void HostSetMousewheel(UINT32 u32Value);

extern void HostGetTrackState(INT8* ps8X, INT8* ps8Y);
extern void HostOSInit(void);
extern void SDLGfxLayerSetDefaultSurface(SDL_Surface *psSurface);
extern SDL_Surface *SDLGfxLayerGetActiveSurface(void);
extern SDL_Surface *SDLGfxLayerGetDefaultSurface(void);

// Virtual interrupts
#define	INTMASK_TIMER_TICK			0x01
#define INTMASK_NIC					0x02
#define INTMASK_AUDIO				0x04

extern void WindowsVirtualInterruptSignal(UINT32 u32InterruptMask);
extern void WindowsVirtualInterruptClear(UINT32 u32InterruptMask);
extern void NICCallback(void);
extern void Win32DeferredBlit(void);


Surface* GetActiveSurface(void);
Surface* GetDefaultSurface(void);
// #define	USE_GRATOS

#endif // _HOST_H_

