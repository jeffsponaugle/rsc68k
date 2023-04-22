#include "Startup/app.h"
#include "Win32/host.h"

#define GLEW_STATIC
#include "GL/glew.h"
#include "GL/gl.h"


static UINT16 *sg_pu16FrameBuffer  = NULL;

#define PALETTE_FOREGROUND		254
#define	PALETTE_BACKGROUND		255

BOOL sg_bInterceptPaletteEntries = FALSE;

/* ************************************************************************* *\
** FUNCTION: GCDisplayGetDisplayBuffer
\* ************************************************************************* */
EGCResultCode GCDisplayGetDisplayBuffer(void **ppvDisplayBuffer)
{
	Surface *psSurface;

	psSurface = GetActiveSurface();

	if (psSurface)
	{
		*ppvDisplayBuffer = (void *) psSurface->pixels;
	}

	return(GC_OK);
}

/* ************************************************************************* *\
** FUNCTION: GCGetAvailableVideoModes
\* ************************************************************************* */
SVideoModes *GCGetAvailableVideoModes(void)
{
	SVideoModes *psVideoModeHead = NULL;
	SVideoModes *psVideoModePtr = NULL;

		// 2 Modes available (16bpp, 8bpp)
		psVideoModeHead = GCAllocateMemory(sizeof(*psVideoModeHead) * 3);
		psVideoModePtr = psVideoModeHead;
		psVideoModePtr->u32Pitch = 640;
		psVideoModePtr->u32XSize = 640;
		psVideoModePtr->u32YSize = 480;
		psVideoModePtr->u8BPP = 16;
		psVideoModePtr->u8RefreshRate = 60;
		psVideoModePtr++;
		psVideoModePtr->u32Pitch = 640;
		psVideoModePtr->u32XSize = 640;
		psVideoModePtr->u32YSize = 480;
		psVideoModePtr->u8BPP = 8;
		psVideoModePtr->u8RefreshRate = 60;


	return(psVideoModeHead);
}

EGCResultCode GCDisplaySetPaletteEntry(UINT8 u8Index,
									   UINT8 u8Red,
									   UINT8 u8Green,
									   UINT8 u8Blue)
{
	if (sg_bInterceptPaletteEntries)
	{
		if ((PALETTE_FOREGROUND == u8Index) ||
			(PALETTE_BACKGROUND == u8Index))
		{
			// Ignore it if we're doing text printing
			return(GC_OK);
		}
	}

    GCASSERT(0);
    /*
	HostSetPaletteEntry(u8Index,
						u8Red,
						u8Green,
						u8Blue);
     * */
	return(GC_OK);
}

/* ************************************************************************* *\
** FUNCTION: GCDisplaySetMode
\* ************************************************************************* */
EGCResultCode GCDisplaySetMode(UINT32 u32XSize, UINT32 u32YSize, UINT8 u8BPP,
                               UINT32 u32ClearScreenColor)
{
	BOOL bOpenGL = FALSE;

	EnterCriticalSection(&g_sVideoCriticalSection);

	if (17 == u8BPP)	// This is how we signal square graphics modes
	{
		u8BPP = 16;
	}

	if(((INT32)u32XSize) < 0)
	{
		u32XSize = -(INT32)u32XSize;
		bOpenGL = TRUE;
	}

	HostSetFrameRate(GCGetRefreshRate() >> 24);
	
#ifdef _RELEASE
	HostSetDisplaySurface(u32XSize, u32YSize, u8BPP, FALSE, bOpenGL);
#else
	// Otherwise windowed
	HostSetDisplaySurface(u32XSize, u32YSize, u8BPP, TRUE, bOpenGL);
#endif

	LeaveCriticalSection(&g_sVideoCriticalSection);

	return(GC_OK);
}


#define DEFAULT_REFRESH_RATE	(60 << 24)
static UINT32 sg_u32RefreshRate = DEFAULT_REFRESH_RATE;

/* ************************************************************************* *\
** FUNCTION: GCGetRefreshRate
\* ************************************************************************* */
UINT32 GCGetRefreshRate(void)
{
	return(sg_u32RefreshRate);
}

void GCSetRefreshRate(UINT32 u32RefreshRate)
{
	sg_u32RefreshRate = u32RefreshRate;
}

/* ************************************************************************* *\
** FUNCTION: GCDisplayGetXSize
\* ************************************************************************* */
EGCResultCode GCDisplayGetXSize(UINT32 *pu32XSize)
{
	*pu32XSize = g_sGfxLocals.BackBufferWidth;
	return(GC_OK);
}

/* ************************************************************************* *\
** FUNCTION: GCDisplayGetYSize
\* ************************************************************************* */
EGCResultCode GCDisplayGetYSize(UINT32 *pu32YSize)
{
	*pu32YSize = g_sGfxLocals.BackBufferHeight;
	return(GC_OK);
}

/* ************************************************************************* *\
** FUNCTION: GCDisplayGetColorDepth
\* ************************************************************************* */
EGCResultCode GCDisplayGetColorDepth(UINT8 *pu8ColorDepth)
{
	*pu8ColorDepth = g_sGfxLocals.BackBufferColorDepth;
	return(GC_OK);
}

/* ************************************************************************* *\
** FUNCTION: GCDisplayClear
\* ************************************************************************* */
EGCResultCode GCDisplayClear(UINT8 u8ClearColor)
{
	UINT16 *pu16FrameBufferBase;
	UINT32 u32FrameBufferPitch;

	GCDisplayGetDisplayBuffer((void **) &pu16FrameBufferBase);
		GCDisplayGetDisplayPitch(&u32FrameBufferPitch);

	GCASSERT(pu16FrameBufferBase);
	memset((void *) pu16FrameBufferBase, 0, 
                     g_sGfxLocals.BackBufferWidth
                    * g_sGfxLocals.BackBufferHeight
                    * (g_sGfxLocals.BackBufferColorDepth>>3)
                    );
	return(GC_OK);
}

/* ************************************************************************* *\
** FUNCTION: GCWaitForVsync
\* ************************************************************************* */
void GCWaitForVsync(void)
{
	//cglMakeCurrentThread(TRUE);
	HostThrottleAndBlit();
}


/* ************************************************************************* *\
** FUNCTION: GCDisplayGetDisplayPitch
\* ************************************************************************* */
EGCResultCode GCDisplayGetDisplayPitch(UINT32 *pu32DisplayPitch)
{
	switch (g_sGfxLocals.BackBufferColorDepth)
	{
		case 8:
			*pu32DisplayPitch = HostGetBackBufferPitch();
			break;

		case 16:
	        *pu32DisplayPitch = HostGetBackBufferPitch() >> 1;
			break;

		default:
			GCASSERT(0);
	}

	return(GC_OK);
}


/* ************************************************************************* *\
** ************************************************************************* **
** EOF
** ************************************************************************* **
\* ************************************************************************* */
