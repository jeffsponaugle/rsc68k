#include "Startup/app.h"
#include "Libs/Gfx/GraphicsLib.h"
#include "Libs/window/window.h"
#include "Libs/widget/widget.h"
#include <math.h>

#include "Win32/SDL/sdl.h"
#include "Win32/host.h"


#define WINDOW_BPP				16
#define MAX_WINDOWS				128
#define DIRTY_BUFFER_SQUARE		16

// Thread's stack size
#define WINDOW_THREAD_STACK_SIZE	32768

// How big the window queue is

#ifdef _WIN32
#define	WINDOW_QUEUE_SIZE			128*1024
#else
#define	WINDOW_QUEUE_SIZE			1024
#endif

// Pointer to head of window list
static SWindow *sg_psWindowList[MAX_WINDOWS];
static UINT8 sg_u8WindowThreadStack[WINDOW_THREAD_STACK_SIZE];
static SOSQueue sg_sWindowQueue;
static SWindow *sg_psWindowHead = NULL;
static SOSSemaphore sg_sBlitUpdate;
static SOSSemaphore sg_sWindowListLock;
static SOSSemaphore sg_sWindowScreenshot;
static UINT32 sg_u32TotalWindows;
static BOOL sg_bUserSuspendBlits;				// Controls whether or not blitting to the display surface occurs

typedef struct SWindowAnimation
{
	WINDOWHANDLE eWindowHandle;	// Window handle associated with this animation
	SWidget *psWidget;			// Widget associated with this animation
	UINT32 u32References;		// # Of references to this animation link
	struct SWindowAnimation *psNextLink;	// Pointer to next animation image in the list
} SWindowAnimation;

static SWindowAnimation *sg_psWindowAnimations = NULL;

// Modal related windows
static WINDOWHANDLE sg_eModalWindow = HANDLE_INVALID;

ELCDErr WindowListLock(void)
{
	EGCResultCode eResult;

	eResult = GCOSSemaphoreGet(sg_sWindowListLock,
							   0);
	GCASSERT(GC_OK == eResult);

	return((ELCDErr) (LERR_GC_ERR_BASE + eResult));
}

ELCDErr WindowListUnlock(void)
{
	EGCResultCode eResult;

	eResult = GCOSSemaphorePut(sg_sWindowListLock);
	return((ELCDErr) (LERR_GC_ERR_BASE + eResult));
}

SWindow *WindowGetPointer(WINDOWHANDLE eWINDOWHANDLE)
{
	if (eWINDOWHANDLE >= (sizeof(sg_psWindowList) / sizeof(sg_psWindowList[0])))
	{
		// Window handle out of range
		return(NULL);
	}

	return(sg_psWindowList[eWINDOWHANDLE]);
}

// HACK HACK
typedef struct _SOSSemaphore_t
{
	UINT32 index;
	HANDLE Handle; // Windows Semaphore handle
} SOSSemaphore_t;			

static LEX_CHAR *sg_peScreenshotFilename = NULL;
static ELCDErr sg_eScreenshotErr = LERR_OK;

static void WindowUpdateThread(void *pvEntryParam)
{
	UINT32 u32Event;
	UINT32 u32Cmd;
	EGCResultCode eResult;
	BOOL bUpdateWindows = FALSE;
	BOOL bBlit = FALSE;
	ELCDErr eErr = LERR_OK;
	BOOL bScreenShot = FALSE;

	ThreadSetName("Window update thread");

	DebugOut("%s: Started\n", __FUNCTION__);

	while (1)
	{
		SWindow *psWindow;

		// Wait forever for something to do

		eResult = GCOSQueueReceive(sg_sWindowQueue,
								   (void **) &u32Event,
								   0);
		GCASSERT(GC_OK == eResult);

		do
		{
			u32Cmd = u32Event & WCMD_MASK;

			if (WCMD_WINDOW_HIDE == u32Cmd)
			{
				// Lock the window
				eErr = WindowLockByHandle((WINDOWHANDLE) (u32Event & WDATA_MASK));

				if (LERR_OK == eErr)
				{
					// Go hide a window
					psWindow = WindowGetPointer(u32Event & WDATA_MASK);
//					DebugOut("* Window hide 0x%.8x\n", u32Event & WDATA_MASK);

					if (psWindow)
					{
						GfxSetImageInstance(psWindow->psWindowImageInstance,
											NO_CHANGE,
											NO_CHANGE,
											FALSE);
						psWindow->bVisible = FALSE;
						bBlit = TRUE;
					}
					else
					{
						// Invalid window handle?
						GCASSERT(0);
					}

					// Now unlock the window
					eErr = WindowUnlockByHandle((WINDOWHANDLE) (u32Event & WDATA_MASK));
					GCASSERT(GC_OK == eErr);
				}
			}
			else
			if (WCMD_WINDOW_SHOW == u32Cmd)
			{
				// Lock the window
				eErr = WindowLockByHandle((WINDOWHANDLE) (u32Event & WDATA_MASK));

				if (LERR_OK == eErr)
				{
					// Go show a window
					psWindow = WindowGetPointer(u32Event & WDATA_MASK);
//					DebugOut("* Window show 0x%.8x\n", u32Event & WDATA_MASK);

					if (psWindow && psWindow->psWindowImageInstance)
					{
						GfxSetImageInstance(psWindow->psWindowImageInstance,
											NO_CHANGE,
											NO_CHANGE,
											TRUE);
						psWindow->bVisible = TRUE;
						bBlit = TRUE;
					}
					else
					if (NULL == psWindow)
					{
						// Invalid window handle?
						GCASSERT(0);
					}

					// Now unlock the window
					eErr = WindowUnlockByHandle((WINDOWHANDLE) (u32Event & WDATA_MASK));
					GCASSERT(GC_OK == eErr);
				}
				else
				{
					DebugOut("Failed to lock window handle %u, code 0x%.8x\n", u32Event & WDATA_MASK, eErr);
				}
			}
			else
			if (WCMD_WINDOW_GRAPH_UPDATE == u32Cmd)
			{
				// Update the graph!
				GraphWidgetUpdate((GRAPHHANDLE) u32Event & WDATA_MASK);
			}
			else
			if (WCMD_WINDOW_CONSOLE_UPDATE == u32Cmd)
			{
				// Update the console
				ConsoleWidgetUpdate((CONSOLEHANDLE) (u32Event & WDATA_MASK));
			}
			else
			if (WCMD_WINDOW_UPDATE == u32Cmd)
			{
				// Cause windows to be updated
				bUpdateWindows = TRUE;
			}
			else
			if (WCMD_WINDOW_FORCE_BLIT == u32Cmd)
			{
				bBlit = TRUE;
			}
			else
			if ((WCMD_WINDOW_KEY_HIT == u32Cmd) ||
				(WCMD_WINDOW_KEY_RELEASED == u32Cmd))
			{
				UINT32 u32Value = (u32Event & WDATA_MASK);
				EGCCtrlKey eGCKey;
				LEX_CHAR eUnicode;
				BOOL bPressed = FALSE;

				if (WCMD_WINDOW_KEY_HIT == u32Cmd)
				{
					bPressed = TRUE;
				}

				eGCKey = (UINT8)(u32Value >> 16);
				eUnicode = (LEX_CHAR)u32Value;

				// Deposit it in the widget queue for now. We'll get back to this later when
				// we're doing more complex things, such as selection between windows, but for
				// now it just needs to go to the active widget
				WidgetKeypress(eGCKey,
							   eUnicode,
							   bPressed);

#if 0
				if( EGCKey_Unknown == eGCKey )
				{
					DebugOut("%s: Window queue got unknown %s\n", __FUNCTION__, bPressed?"keydown":"keyup");
				}
				else if( EGCKey_Unicode == eGCKey )
				{
					DebugOut("%s: Window queue got unicode 0x%04x %s\n", __FUNCTION__, eUnicode, bPressed?"keydown":"keyup");
				}
				else
				{
					DebugOut("%s: Window queue got non-unicode %u %s\n", __FUNCTION__, eGCKey, bPressed?"keydown":"keyup");
				}
#endif
			}
			else
			if (WCMD_WINDOW_MOUSEWHEEL == u32Cmd)
			{
				// Mouse wheel up or down - (u32Event & 1) == Up=0, Down=1.
				WidgetMousewheel((u32Event & WDATA_MASK));
			}
			else
			if (WCMD_WINDOW_SET_MODAL == u32Cmd)
			{
				SWindow *psWindow;
				WINDOWHANDLE eWindowHandle = (WINDOWHANDLE) (u32Event & ~WCMD_MASK);

				// Passing an invalid window handle into the window queue is problematic.  Patch it up.
				if( (HANDLE_INVALID & ~WCMD_MASK) == eWindowHandle )
				{
					eWindowHandle = HANDLE_INVALID;
				}

				// See if this handle is valid
				if (eWindowHandle != HANDLE_INVALID)
				{
					psWindow = sg_psWindowHead;
					while (psWindow)
					{
						WidgetListReleaseAllWidgets(psWindow->psWidgetHead,
													0, 0, 0);
						psWindow = psWindow->psNextWindow;
					}
				}

				sg_eModalWindow = eWindowHandle;
			}
			else
			if (WCMD_WINDOW_SHUTDOWN == u32Cmd)
			{
				DebugOut("%s: Shutdown order received\n", __FUNCTION__);
				goto threadExit;
			}
			else
			{
				// Unknown message
				GCASSERT(0);
			}

			eResult = GCOSQueuePeek(sg_sWindowQueue,
									(void **) &u32Event);
		}
		while (GC_OK == eResult);

		// If bUpdateWindows is TRUE, then we need to run through the entire list of
		// windows looking for windows that have regions that need updating

		if (bUpdateWindows)
		{
			bUpdateWindows = FALSE;

			eErr = WindowListLock();
			GCASSERT(LERR_OK == eErr);

			psWindow = sg_psWindowHead;

			while (psWindow)
			{
				// If this window pointer is present/active, and it isn't locked, and we have
				// an update region, then update the window.

				if (psWindow->u32UpdateRegionXMin != 0xffffffff)
				{
					EGCResultCode eResult;
					UINT32 u32UpdateRegionXMax;
					UINT32 u32UpdateRegionYMax;
					UINT32 u32UpdateRegionXMin;
					UINT32 u32UpdateRegionYMin;

					eResult = GCOSSemaphoreGet(psWindow->sDirtyRegionSem,
											   0);
					GCASSERT(GC_OK == eResult);

					// Make a local copy of the update region
					u32UpdateRegionXMax = psWindow->u32UpdateRegionXMax;
					u32UpdateRegionYMax = psWindow->u32UpdateRegionYMax;
					u32UpdateRegionXMin = psWindow->u32UpdateRegionXMin;
					u32UpdateRegionYMin = psWindow->u32UpdateRegionYMin;

					eResult = GCOSSemaphorePut(psWindow->sDirtyRegionSem);
					GCASSERT(GC_OK == eResult);

					// Tag this window so we don't update it again
					psWindow->u32UpdateRegionXMax = 0;
					psWindow->u32UpdateRegionYMax = 0;
					psWindow->u32UpdateRegionXMin = 0xffffffff;
					psWindow->u32UpdateRegionYMin = 0xffffffff;

					// Make sure the update region is reasonable (min <= max)
					if( (u32UpdateRegionXMax < u32UpdateRegionXMin) ||
						(u32UpdateRegionYMax < u32UpdateRegionYMin) )
					{
						DebugOut("Error: Invalid window update region.  Skipping window.\n");
						DebugOut("%u < %u || %u < %u\n", u32UpdateRegionXMax, u32UpdateRegionXMin,
														 u32UpdateRegionYMax, u32UpdateRegionYMin);
						goto next_window;
					}

					// Shift the update region by the viewport offset
					if( u32UpdateRegionXMin >= psWindow->u32ViewportXOffset )
					{
						u32UpdateRegionXMin -= psWindow->u32ViewportXOffset;
						u32UpdateRegionXMax -= psWindow->u32ViewportXOffset;
					}
					else
					{
						u32UpdateRegionXMin = 0;
					}

					if( u32UpdateRegionYMin >= psWindow->u32ViewportYOffset )
					{
						u32UpdateRegionYMin -= psWindow->u32ViewportYOffset;
						u32UpdateRegionYMax -= psWindow->u32ViewportYOffset;
					}
					else
					{
						u32UpdateRegionYMin = 0;
					}

					// Trim the update region to the end of the viewport
					if(  u32UpdateRegionXMax >= psWindow->u32ViewportXSize )
					{
						u32UpdateRegionXMax = psWindow->u32ViewportXSize;
					}

					if(  u32UpdateRegionYMax >= psWindow->u32ViewportYSize )
					{
						u32UpdateRegionYMax = psWindow->u32ViewportYSize;
					}


					eResult = GCOSSemaphoreGet(sg_sBlitUpdate,
											   0);
					GCASSERT(GC_OK == eResult);

					psWindow->psWindowImageInstance->u8Intensity = psWindow->u8WindowIntensity;

					if (psWindow->u8WindowIntensity < 0xff)
					{
						// Translucent window - need to erase it and redraw it

						GfxUpdateImageRegion(psWindow->psWindowImageInstance,
											 TRUE,
											 u32UpdateRegionXMin,
											 u32UpdateRegionYMin,
											 (u32UpdateRegionXMax - u32UpdateRegionXMin),
											 (u32UpdateRegionYMax - u32UpdateRegionYMin));
					}

					if (psWindow->u8WindowIntensity)
					{
						// Go update the image's region
						GfxUpdateImageRegion(psWindow->psWindowImageInstance,
											 FALSE,
											 u32UpdateRegionXMin,
											 u32UpdateRegionYMin,
											 (u32UpdateRegionXMax - u32UpdateRegionXMin),
											 (u32UpdateRegionYMax - u32UpdateRegionYMin));
					}

					eResult = GCOSSemaphorePut(sg_sBlitUpdate);
					GCASSERT(GC_OK == eResult);

					bBlit = TRUE;
				}

next_window:
				psWindow = psWindow->psNextWindow;
			}

			eErr = WindowListUnlock();
		}

		GCASSERT(LERR_OK == eErr);

		// Something was done - we need to blit it to the display
		if ((bBlit) && (FALSE == sg_bUserSuspendBlits))
		{
			eResult = GCOSSemaphoreGet(sg_sBlitUpdate,
									   0);
			GCASSERT(GC_OK == eResult);
			GfxBlit(FALSE);
			eResult = GCOSSemaphorePut(sg_sBlitUpdate);
			GCASSERT(GC_OK == eResult);
			GCWaitForVsync();

			bBlit = FALSE;
		}
	}

threadExit:
	DebugOut("%s: Terminated\n", __FUNCTION__);
}

void WindowSuspendBlits(BOOL bSuspend)
{
	// If we have a suspend->not suspend, deposit a message to blit
	if ((FALSE == bSuspend) && (sg_bUserSuspendBlits))
	{
		EGCResultCode eResult;

		// Do this to allow blits to continue
		sg_bUserSuspendBlits = FALSE;

		// Deposit a message to force a blit
		eResult = GCOSQueueSend(sg_sWindowQueue,
								(void *) (WCMD_WINDOW_FORCE_BLIT));
		GCASSERT(GC_OK == eResult);
	}

	sg_bUserSuspendBlits = bSuspend;
}

ELCDErr WindowKeyHit(EGCCtrlKey eGCKey, LEX_CHAR eKey, BOOL bPressed)
{
	UINT32 u32Message = WCMD_WINDOW_KEY_RELEASED;

	// Assemble the rest of the message
	if( bPressed )
	{
		u32Message = WCMD_WINDOW_KEY_HIT;
	}

	// Add the GCKey descriptor
	u32Message |= ((UINT8)eGCKey) << 16;

	// Now add the unicode value of the key (only if eGCKey == EGCKey_Unicode)
	u32Message |= eKey;

	return((ELCDErr) (LERR_GC_ERR_BASE + GCOSQueueSend(sg_sWindowQueue,(void *) u32Message) ));
}

void WindowBlitLock(BOOL bLock)
{
	EGCResultCode eResult;
	SOSSemaphore_t* pSem = (SOSSemaphore_t*)sg_sBlitUpdate;

	if (bLock)
	{
		eResult = GCOSSemaphoreGet(sg_sBlitUpdate,
								   0);
		GCASSERT(GC_OK == eResult);
	}
	else
	{
		eResult = GCOSSemaphorePut(sg_sBlitUpdate);
		GCASSERT(GC_OK == eResult);
	}
}

ELCDErr WindowLock(SWindow *psWindow)
{
	EGCResultCode eResult;

	// Wait forever
	eResult = GCOSSemaphoreGet(psWindow->sWindowUpdateSem,
							   0);
	return((ELCDErr) (LERR_GC_ERR_BASE + eResult));
}

ELCDErr WindowUnlock(SWindow *psWindow)
{
	EGCResultCode eResult;

	eResult = GCOSSemaphorePut(psWindow->sWindowUpdateSem);
	return((ELCDErr) (LERR_GC_ERR_BASE + eResult));
}

ELCDErr WindowLockByHandle(WINDOWHANDLE eWindow)
{
	SWindow *psWindow;

	psWindow = WindowGetPointer(eWindow);
	if (NULL == psWindow)
	{
		return(LERR_WIN_BAD_HANDLE);
	}

	return(WindowLock(psWindow));;
}

ELCDErr WindowUnlockByHandle(WINDOWHANDLE eWindow)
{
	SWindow *psWindow;

	psWindow = WindowGetPointer(eWindow);
	if (NULL == psWindow)
	{
		return(LERR_WIN_BAD_HANDLE);
	}

	return(WindowUnlock(psWindow));
}

ELCDErr WindowLockByHandleIfNotSubordinate(WINDOWHANDLE eWindow,
										   SWidget *psWidget)
{
	if (HANDLE_INVALID == psWidget->eParentWidget)
	{
		return(WindowLockByHandle(eWindow));
	}

	return(LERR_OK);
}

ELCDErr WindowUnlockByHandleIfNotSubordinate(WINDOWHANDLE eWindow,
											 SWidget *psWidget)
{
	if (HANDLE_INVALID == psWidget->eParentWidget)
	{
		return(WindowUnlockByHandle(eWindow));
	}

	return(LERR_OK);
}


ELCDErr WindowRedrawAllWidgets(WINDOWHANDLE eWindow)
{
	SWindow *psWindow;
	SWidget *psWidget;

	psWindow = WindowGetPointer(eWindow);
	if (NULL == psWindow)
	{
		return(LERR_WIN_BAD_HANDLE);
	}

	psWidget = psWindow->psWidgetHead;

	while (psWidget)
	{
		(void) WidgetPaint(psWidget,
						   FALSE);
		psWidget = psWidget->psNextLink;
	}

	return(LERR_OK);
}

// Route keypresses to the window or widget queue
static BOOL WindowKeyCallback(EGCCtrlKey eGCKey, LEX_CHAR eUnicode, BOOL bPressed)
{
	ELCDErr eErr;

	eErr = WindowKeyHit(eGCKey,eUnicode, bPressed);

	if( LERR_OK == eErr )
	{
		return(TRUE);
	}

	return(FALSE);
}

void WindowUpdateRegion(WINDOWHANDLE eWindow,
					    INT32 s32XPos,
					    INT32 s32YPos,
					    INT32 s32XSize,
					    INT32 s32YSize)
{
	EGCResultCode eResult;
	SWindow *psWindow;

	psWindow = WindowGetPointer(eWindow);
	if (NULL == psWindow)
	{
		return;
	}

	if (FALSE == psWindow->bVisible)
	{
		// Don't do anything if it's not visible
		return;
	}

	// If our X or Y coordinate is less than our current update, then set it appropriately

	// Let's do a left clip
	if (s32XPos < 0)
	{
		s32XSize += s32XPos;
		s32XPos = 0;
	}

	if (s32YPos < 0)
	{
		s32YSize += s32YPos;
		s32YPos = 0;
	}

	// And a right/bottom clip

	// Lock the update region
	eResult = GCOSSemaphoreGet(psWindow->sDirtyRegionSem,
							   0);
	GCASSERT(GC_OK == eResult);

	// Add the blit region

	if ((s32XSize > 0) && (s32YSize > 0))
	{
		if ((UINT32) s32XPos < (INT32) psWindow->u32UpdateRegionXMin)
		{
			psWindow->u32UpdateRegionXMin = s32XPos;
		}

		if ((UINT32) s32YPos < psWindow->u32UpdateRegionYMin)
		{
			psWindow->u32UpdateRegionYMin = s32YPos;
		}

		s32XPos += (s32XSize);
		if ((UINT32) s32XPos > psWindow->psWindowImage->u32XSize)
		{
			s32XPos = psWindow->psWindowImage->u32XSize;
		}

		if ((UINT32) s32XPos > psWindow->u32UpdateRegionXMax)
		{
			psWindow->u32UpdateRegionXMax = (UINT32) s32XPos;
		}

		s32YPos += (s32YSize);
		if ((UINT32) s32YPos > psWindow->psWindowImage->u32YSize)
		{
			s32YPos = psWindow->psWindowImage->u32YSize;
		}

		if ((UINT32) s32YPos > psWindow->u32UpdateRegionYMax)
		{
			psWindow->u32UpdateRegionYMax = (UINT32) s32YPos;
		}
	}

	// Now we can unlock the update region
	eResult = GCOSSemaphorePut(psWindow->sDirtyRegionSem);
	GCASSERT(GC_OK == eResult);
}

static UINT32 sg_u32CommitTimerLastValue = 0;
static BOOL sg_bCommitTimerFlagged = FALSE;

static void WindowCommitTimerCallback(UINT32 u32Value)
{
	if (sg_bCommitTimerFlagged)
	{
		if ((GCGetTimeMS() - sg_u32CommitTimerLastValue) >= 15)
		{
			EGCResultCode eResult;

			eResult = GCOSQueueSend(sg_sWindowQueue,
									(void *) (WCMD_WINDOW_UPDATE));
			if (GC_OK == eResult)
			{
				sg_bCommitTimerFlagged = FALSE;
			}
		}
	}
}

void WindowUpdateRegionCommit(void)
{
	if (FALSE == sg_bCommitTimerFlagged)
	{
		sg_u32CommitTimerLastValue = GCGetTimeMS();
		sg_bCommitTimerFlagged = TRUE;
	}
}

ELCDErr WindowDepositMessage(UINT32 u32Message)
{
	EGCResultCode eResult;

	// Now go queue the window for updating
	eResult = GCOSQueueSend(sg_sWindowQueue,
							(void *) (u32Message));

	return((ELCDErr) (LERR_GC_ERR_BASE + eResult));
}

static STimerObject *sg_psCommitTimer;

void WindowInit(void)
{
	EGCResultCode eResult;
	UINT32 u32XSize;
	UINT32 u32YSize;
	void **ppvWindowQueue;

	// Initialize the graphics subsystem
	GfxInit();

	// No animations due
	sg_psWindowAnimations = NULL;

	// Delete anything and everything in our window list
	memset((void *) sg_psWindowList, 0, sizeof(sg_psWindowList));

	eResult = GCDisplayGetXSize(&u32XSize);
	GCASSERT(GC_OK == eResult);
	eResult = GCDisplayGetYSize(&u32YSize);
	GCASSERT(GC_OK == eResult);

	DebugOut("* Initializing window manager @ %dx%d\n", u32XSize, u32YSize);
	
	// Go create the graphical surface
	GfxSurfaceCreate(u32XSize,
					 u32YSize,
					 WINDOW_BPP);

	// Create a message queue
	
	ppvWindowQueue = MemAlloc(sizeof(*ppvWindowQueue) * WINDOW_QUEUE_SIZE);
	GCASSERT(ppvWindowQueue);

	eResult = GCOSQueueCreate(&sg_sWindowQueue,
							  ppvWindowQueue,
							  WINDOW_QUEUE_SIZE);
	GCASSERT(GC_OK == eResult);

	// Now create a window manager thread
	eResult = GCOSThreadCreate(WindowUpdateThread,
							   NULL,
							   &sg_u8WindowThreadStack[WINDOW_THREAD_STACK_SIZE - 4],
							   0);
	GCASSERT(GC_OK == eResult);

	eResult = GCTimerCreate(&sg_psCommitTimer);
	GCASSERT(GC_OK == eResult);
	eResult = GCTimerSetCallback(sg_psCommitTimer,
								 WindowCommitTimerCallback,
								 0);
	GCASSERT(GC_OK == eResult);
	eResult = GCTimerSetValue(sg_psCommitTimer,
							  1,
							  1);

	GCASSERT(GC_OK == eResult);
	eResult = GCTimerStart(sg_psCommitTimer);
	GCASSERT(GC_OK == eResult);

	eResult = GCOSSemaphoreCreate(&sg_sBlitUpdate,
								  1);
	GCASSERT(GC_OK == eResult);
	eResult = GCOSSemaphoreCreate(&sg_sWindowListLock,
								  1);
	GCASSERT(GC_OK == eResult);
	eResult = GCOSSemaphoreCreate(&sg_sWindowScreenshot,
								  0);
	GCASSERT(GC_OK == eResult);

	// Setup the keyboard callback
	GCSetKeyCallback(WindowKeyCallback);

	// And the mousewheel callback
	GCSetMousewheelCallback(WindowSetMousewheel);
}

static void WindowCalcOverallWindowSize(SWindow *psWindow,
										UINT32 *pu32XSize,
										UINT32 *pu32YSize)
{
	INT32 s32XLow = 0;
	INT32 s32YLow = 0;
	INT32 s32XHigh = 0;
	INT32 s32YHigh = 0;

	GCASSERT(pu32XSize);
	GCASSERT(pu32YSize);

	// Start off with our max X being our window image X/Y size
	s32XHigh = (INT32) psWindow->u32ActiveAreaXSize;
	s32YHigh = (INT32) psWindow->u32ActiveAreaYSize;

	// Figure out the X high - add in border sizes if need be.

	if (psWindow->sShadow[SORG_LEFT].bActive)
	{
		s32XHigh += (INT32) psWindow->sShadow[SORG_LEFT].u32Thickness;
	}

	if (psWindow->sShadow[SORG_RIGHT].bActive)
	{
		s32XHigh += (INT32) psWindow->sShadow[SORG_RIGHT].u32Thickness;
	}

	// Now figure out the X lows

	if (psWindow->sShadow[SORG_TOP].bActive)
	{
		if (psWindow->sShadow[SORG_TOP].s32Offset < s32XLow)
		{
			s32XLow = psWindow->sShadow[SORG_TOP].s32Offset;
		}

		if ((psWindow->sShadow[SORG_TOP].s32Offset +
			 (INT32) psWindow->sShadow[SORG_TOP].u32Length) > s32XHigh)
		{
			s32XHigh = (psWindow->sShadow[SORG_TOP].s32Offset +
						(INT32) psWindow->sShadow[SORG_TOP].u32Length);
		}
	}

	if (psWindow->sShadow[SORG_BOTTOM].bActive)
	{
		if (psWindow->sShadow[SORG_BOTTOM].s32Offset < s32XLow)
		{
			s32XLow = psWindow->sShadow[SORG_BOTTOM].s32Offset;
		}

		if ((psWindow->sShadow[SORG_BOTTOM].s32Offset +
			 (INT32) psWindow->sShadow[SORG_BOTTOM].u32Length) > s32XHigh)
		{
			s32XHigh = (psWindow->sShadow[SORG_BOTTOM].s32Offset +
						(INT32) psWindow->sShadow[SORG_BOTTOM].u32Length);
		}
	}

	/*********************************************************************/

	// Figure out the Y high - add in border sizes if need be.

	if (psWindow->sShadow[SORG_TOP].bActive)
	{
		s32YHigh += (INT32) psWindow->sShadow[SORG_TOP].u32Thickness;
	}

	if (psWindow->sShadow[SORG_BOTTOM].bActive)
	{
		s32YHigh += (INT32) psWindow->sShadow[SORG_BOTTOM].u32Thickness;
	}

	// Now figure out the Y lows

	if (psWindow->sShadow[SORG_LEFT].bActive)
	{
		if (psWindow->sShadow[SORG_LEFT].s32Offset < s32YLow)
		{
			s32YLow = psWindow->sShadow[SORG_LEFT].s32Offset;
		}

		if ((psWindow->sShadow[SORG_LEFT].s32Offset +
			 (INT32) psWindow->sShadow[SORG_LEFT].u32Length) > s32YHigh)
		{
			s32YHigh = (psWindow->sShadow[SORG_LEFT].s32Offset +
						(INT32) psWindow->sShadow[SORG_LEFT].u32Length);
		}
	}

	if (psWindow->sShadow[SORG_RIGHT].bActive)
	{
		if (psWindow->sShadow[SORG_RIGHT].s32Offset < s32YLow)
		{
			s32YLow = psWindow->sShadow[SORG_RIGHT].s32Offset;
		}

		if ((psWindow->sShadow[SORG_RIGHT].s32Offset +
			 (INT32) psWindow->sShadow[SORG_RIGHT].u32Length) > s32YHigh)
		{
			s32YHigh = (psWindow->sShadow[SORG_RIGHT].s32Offset +
						(INT32) psWindow->sShadow[SORG_RIGHT].u32Length);
		}
	}

	// High minus low == window size
	*pu32XSize = (UINT32) (s32XHigh - s32XLow);
	*pu32YSize = (UINT32) (s32YHigh - s32YLow);
}

static void WindowCalcActiveRegionPosition(SWindow *psWindow)
{
	psWindow->u32ActiveAreaXPos = 0;
	psWindow->u32ActiveAreaYPos = 0;

	if (psWindow->sShadow[SORG_LEFT].bActive)
	{
		psWindow->u32ActiveAreaXPos += psWindow->sShadow[SORG_LEFT].u32Thickness;
	}

	// If the top part of the shadow is active and the offset is negative, we need to
	// include it in the X offset.
	
	if ((psWindow->sShadow[SORG_TOP].bActive) &&
		(psWindow->sShadow[SORG_TOP].s32Offset < 0))
	{
		if ((UINT32) abs(psWindow->sShadow[SORG_TOP].s32Offset) > psWindow->u32ActiveAreaXPos)
		{
			psWindow->u32ActiveAreaXPos = (UINT32) abs(psWindow->sShadow[SORG_TOP].s32Offset);
		}
	}

	// If the bottom part of the shadow is active and the offset is negative, we need to
	// include it in the X offset.

	if ((psWindow->sShadow[SORG_BOTTOM].bActive) &&
		(psWindow->sShadow[SORG_BOTTOM].s32Offset < 0))
	{
		if ((UINT32) abs(psWindow->sShadow[SORG_BOTTOM].s32Offset) > psWindow->u32ActiveAreaXPos)
		{
			psWindow->u32ActiveAreaXPos = (UINT32) abs(psWindow->sShadow[SORG_BOTTOM].s32Offset);
		}
	}

	if (psWindow->sShadow[SORG_TOP].bActive)
	{
		psWindow->u32ActiveAreaYPos += psWindow->sShadow[SORG_TOP].u32Thickness;
	}

	// If the left part of the shadow is active and the offset is negative, we need to
	// include it in the Y offset.

	if ((psWindow->sShadow[SORG_LEFT].bActive) &&
		(psWindow->sShadow[SORG_LEFT].s32Offset < 0))
	{
		if ((UINT32) abs(psWindow->sShadow[SORG_LEFT].s32Offset) > psWindow->u32ActiveAreaYPos)
		{
			psWindow->u32ActiveAreaYPos = (UINT32) abs(psWindow->sShadow[SORG_LEFT].s32Offset);
		}
	}

	// If the right part of the shadow is active and the offset is negative, we need to
	// include it in the Y offset.

	if ((psWindow->sShadow[SORG_RIGHT].bActive) &&
		(psWindow->sShadow[SORG_RIGHT].s32Offset < 0))
	{
		if ((UINT32) abs(psWindow->sShadow[SORG_RIGHT].s32Offset) > psWindow->u32ActiveAreaYPos)
		{
			psWindow->u32ActiveAreaYPos = (UINT32) abs(psWindow->sShadow[SORG_RIGHT].s32Offset);
		}
	}
}

static BOOL WindowFindFreeHandle(WINDOWHANDLE *peWindowHandle)
{
	UINT32 u32Loop;

	for (u32Loop = 0; u32Loop < (sizeof(sg_psWindowList) / sizeof(sg_psWindowList[0])); u32Loop++)
	{
		if (NULL == sg_psWindowList[u32Loop])
		{
			*peWindowHandle = (WINDOWHANDLE) u32Loop;
			return(TRUE);
		}
	}

	return(FALSE);
}

void WindowEraseActiveRegion(SWindow *psWindow,
							 INT32 s32XPos,
							 INT32 s32YPos,
							 INT32 s32XSize,
							 INT32 s32YSize)
{
	UINT16 *pu16DataPtr;
	UINT32 u32HorizontalFill = 0;
	UINT32 u32VerticalFill = 0;

	GCASSERT(psWindow);

	// Clip our XSize and YSize to the active area

	if (s32XPos < 0)
	{
		s32XSize += s32XPos;
		s32XPos = 0;
	}

	if (s32YPos < 0)
	{
		s32YSize += s32YPos;
		s32YPos = 0;
	}

	if ((s32XSize > 0) && (s32YSize > 0))
	{
		if ((s32XPos + s32XSize) > (INT32) psWindow->u32ActiveAreaXSize)
		{
			s32XSize = (INT32) psWindow->u32ActiveAreaXSize - (INT32) s32XPos;
		}

		if ((s32YPos + s32YSize) > (INT32) psWindow->u32ActiveAreaYSize)
		{
			s32YSize = (INT32) psWindow->u32ActiveAreaYSize - (INT32) s32YPos;
		}

		// If we have a background image, we need to clip to it, too.

		if ((psWindow->psBackgroundImage) && (psWindow->psBackgroundImage->psCurrentImage))
		{
			SImage *psBackgroundImage = psWindow->psBackgroundImage->psCurrentImage;

			if ((s32XPos + s32XSize) > (INT32) psBackgroundImage->u32XSize)
			{
				u32HorizontalFill = (s32XPos + s32XSize) - (s32XPos + psBackgroundImage->u32XSize);
				s32XSize = psBackgroundImage->u32XSize - s32XPos;
			}

			if ((s32YPos + s32YSize) > (INT32) psBackgroundImage->u32YSize)
			{
				u32VerticalFill = (s32YPos + s32YSize) - (s32YPos + psBackgroundImage->u32YSize);
				s32YSize = psBackgroundImage->u32YSize - s32YPos;
			}
		}

		// If we're 0 or negative on either axis, there's nothing to do

		if ((s32XSize <= 0) ||
			(s32YSize <= 0))
		{
			// Nothing to do
			return;
		}

		// Let's first compute the place in the window where we put stuff.

		pu16DataPtr = psWindow->psWindowImage->pu16ImageData +
					  (psWindow->u32ActiveAreaXPos + s32XPos) +
					  ((psWindow->u32ActiveAreaYPos + s32YPos) * psWindow->psWindowImage->u32Pitch);

		// Do we have a background image? If so, we need to draw it (and intensity
		// adjust it, too)

	#define COLOR_ADJUST_INTENSITY(color, intensity) ( ((((color & 0xf800) * intensity) >> 8) & 0xf800) | \
													   ((((color & 0x07e0) * intensity) >> 8) & 0x07e0) | \
													   ((((color & 0x001f) * intensity) >> 8) & 0x001f) )

		if ((psWindow->psBackgroundImage) && (psWindow->psBackgroundImage->psCurrentImage))
		{
			SImage *psBackgroundImage = psWindow->psBackgroundImage->psCurrentImage;
			UINT16 *pu16SrcPtr = NULL;
			UINT8 *pu8SrcPtr = NULL;
			UINT16 *pu16Palette = NULL;
			UINT32 u32XSizeBlit;
			UINT32 u32YSizeBlit;
			UINT16 u16Color;

			// pu16DataPtr Points to the upper left hand corner of the active window image

			// pu16SrcPtr points the part of the image that we may need to suck up.
			// u32XPos/u32YPos == active coordinates, which are always 1:1 with the
			// background image

			if (psBackgroundImage->pu16ImageData)
			{
				pu16SrcPtr = psBackgroundImage->pu16ImageData + 
							 s32XPos + (s32YPos * psBackgroundImage->u32Pitch);
			}
			else
			{
				// It's an 8BPP image
				pu8SrcPtr = psBackgroundImage->pu8ImageData + 
							 s32XPos + (s32YPos * psBackgroundImage->u32Pitch);

				// Better have a palette
				GCASSERT(psBackgroundImage->pu16Palette);
			}

			u32XSizeBlit = (UINT32) s32XSize;
			if (u32XSizeBlit > psWindow->psWindowImage->u32XSize)
			{
				u32XSizeBlit = psWindow->psWindowImage->u32XSize;
			}

			u32YSizeBlit = (UINT32) s32YSize;
			if (u32YSizeBlit > psWindow->psWindowImage->u32YSize)
			{
				u32YSizeBlit = psWindow->psWindowImage->u32YSize;
			}

			// If we're at full intensity, just a lot of memcpys are what's necessary
			if (0xff == psWindow->u8BackgroundIntensityLevel)
			{
				if (pu16SrcPtr)
				{
					while (u32YSizeBlit--)
					{
						memcpy((void *) pu16DataPtr, (void *) pu16SrcPtr, u32XSizeBlit << 1);
						pu16DataPtr += psWindow->psWindowImage->u32Pitch;
						pu16SrcPtr += psBackgroundImage->u32Pitch;
					}
				}
				else
				{
					UINT32 u32Loop;

					// 8BPP blit
					GCASSERT(pu8SrcPtr);

					pu16Palette = psBackgroundImage->pu16Palette;

					if (psBackgroundImage->u16TransparentIndex)
					{
						while (u32YSizeBlit--)
						{
							u32Loop = u32XSizeBlit;
							while (u32Loop)
							{
								*pu16DataPtr = pu16Palette[*pu8SrcPtr];
								pu8SrcPtr++;
								pu16DataPtr++;
								u32Loop--;
							}

							pu16DataPtr += (psWindow->psWindowImage->u32Pitch - u32XSizeBlit);
							pu8SrcPtr += (psBackgroundImage->u32Pitch - u32XSizeBlit);
						}
					}
					else
					{
						// Image has transparencies. Yikes.
						while (u32YSizeBlit--)
						{
							u32Loop = u32XSizeBlit;
							while (u32Loop)
							{
								if (*pu8SrcPtr != (UINT8) psBackgroundImage->u16TransparentIndex)
								{
									*pu16DataPtr = pu16Palette[*pu8SrcPtr];
								}
								else
								{
									// Black pixel
									*pu16DataPtr = 0;
								}

								pu8SrcPtr++;
								pu16DataPtr++;
								u32Loop--;
							}

							pu16DataPtr += (psWindow->psWindowImage->u32Pitch - u32XSizeBlit);
							pu8SrcPtr += (psBackgroundImage->u32Pitch - u32XSizeBlit);
						}
					}
				}
			}
			else
			{
				// The old, slow, ugly way of doing things

				if (pu16SrcPtr)
				{
					while (u32YSizeBlit--)
					{
						UINT32 u32Loop;

						u32Loop = u32XSizeBlit;
						while (u32Loop--)
						{
							*pu16DataPtr = COLOR_ADJUST_INTENSITY(*pu16SrcPtr, psWindow->u8BackgroundIntensityLevel);
							++pu16DataPtr;
							++pu16SrcPtr;
						}

						pu16DataPtr -= u32XSizeBlit;
						pu16SrcPtr -= u32XSizeBlit;
						pu16DataPtr += psWindow->psWindowImage->u32Pitch;
						pu16SrcPtr += psBackgroundImage->u32Pitch;
					}
				}
				else
				{
					// 8bpp image
				if (pu16SrcPtr)
				{
					while (u32YSizeBlit--)
					{
						memcpy((void *) pu16DataPtr, (void *) pu16SrcPtr, u32XSizeBlit << 1);
						pu16DataPtr += psWindow->psWindowImage->u32Pitch;
						pu16SrcPtr += psBackgroundImage->u32Pitch;
					}
				}
				else
				{
					UINT32 u32Loop;

					// 8BPP blit
					GCASSERT(pu8SrcPtr);

					pu16Palette = psBackgroundImage->pu16Palette;

					if (psBackgroundImage->u16TransparentIndex)
					{
						while (u32YSizeBlit--)
						{
							u32Loop = u32XSizeBlit;
							while (u32Loop)
							{
								*pu16DataPtr = COLOR_ADJUST_INTENSITY(pu16Palette[*pu8SrcPtr], psWindow->u8BackgroundIntensityLevel);
								pu8SrcPtr++;
								pu16DataPtr++;
								u32Loop--;
							}

							pu16DataPtr += (psWindow->psWindowImage->u32Pitch - u32XSizeBlit);
							pu8SrcPtr += (psBackgroundImage->u32Pitch - u32XSizeBlit);
						}
					}
					else
					{
						// Image has transparencies. Yikes.
						while (u32YSizeBlit--)
						{
							u32Loop = u32XSizeBlit;
							while (u32Loop)
							{
								if (*pu8SrcPtr != (UINT8) psBackgroundImage->u16TransparentIndex)
								{
									*pu16DataPtr = COLOR_ADJUST_INTENSITY(pu16Palette[*pu8SrcPtr], psWindow->u8BackgroundIntensityLevel);
								}
								else
								{
									// Black pixel
									*pu16DataPtr = 0;
								}

								pu8SrcPtr++;
								pu16DataPtr++;
								u32Loop--;
							}

							pu16DataPtr += (psWindow->psWindowImage->u32Pitch - u32XSizeBlit);
							pu8SrcPtr += (psBackgroundImage->u32Pitch - u32XSizeBlit);
						}
					}
				}
				}
			}

			u16Color = COLOR_ADJUST_INTENSITY(psWindow->u16BackgroundColor, psWindow->u8BackgroundIntensityLevel);

			// If this is nonzero, then we need to do a horizontal fill

			if (u32HorizontalFill)
			{
				UINT32 u32Loop;
				UINT32 u32Loop2;

				// Must go from the right of the image

				pu16DataPtr = psWindow->psWindowImage->pu16ImageData +
							  (psWindow->u32ActiveAreaXPos + psBackgroundImage->u32XSize) +
							  ((psWindow->u32ActiveAreaYPos) * psWindow->psWindowImage->u32Pitch);

				u32Loop = psBackgroundImage->u32YSize;

				while (u32Loop--)
				{
					u32Loop2 = u32HorizontalFill;
					while (u32Loop2--)
					{
						*pu16DataPtr = u16Color;
						++pu16DataPtr;
					}
					pu16DataPtr -= u32HorizontalFill;
					pu16DataPtr += psWindow->psWindowImage->u32Pitch;
				}
			}

			// If this is nonzero, we need to do a vertical fill

			if (u32VerticalFill)
			{
				UINT32 u32Loop;
				UINT32 u32Loop2;

				// Must go from the right of the image

				pu16DataPtr = psWindow->psWindowImage->pu16ImageData +
							  (psWindow->u32ActiveAreaXPos) +
							  ((psWindow->u32ActiveAreaYPos + psBackgroundImage->u32YSize) * psWindow->psWindowImage->u32Pitch);

				u32Loop = u32VerticalFill;

				while (u32Loop--)
				{
					u32Loop2 = psWindow->psWindowImage->u32XSize;
					while (u32Loop2--)
					{
						*pu16DataPtr = u16Color;
						++pu16DataPtr;
					}
					pu16DataPtr -= psWindow->psWindowImage->u32XSize;
					pu16DataPtr += psWindow->psWindowImage->u32Pitch;
				}
			}
		}
		else
		{
			// Just a color fill

			INT32 s32XCount;
			UINT16 u16Fill;
			
			// Figure out what we're filling the window 
			u16Fill = COLOR_ADJUST_INTENSITY(psWindow->u16BackgroundColor, psWindow->u8WindowIntensity);

			if (u16Fill)
			{
				while (s32YSize--)
				{
					s32XCount = s32XSize;
					while (s32XCount--)
					{
						*pu16DataPtr = u16Fill;
						++pu16DataPtr;
					}

					// Advance to the next line
					pu16DataPtr = (pu16DataPtr - s32XSize) + psWindow->psWindowImage->u32Pitch;
				}
			}
			else
			{
				while (s32YSize--)
				{
					memset((void *) pu16DataPtr, 0, s32XSize << 1);

					// Advance to the next line
					pu16DataPtr += psWindow->psWindowImage->u32Pitch;
				}

			}
		}
	}
}

ELCDErr WindowSetViewport(WINDOWHANDLE eWindow,
						  UINT32* pu32XOffset,
						  UINT32* pu32YOffset,
						  UINT32* pu32XSize,
						  UINT32* pu32YSize)
{
	SWindow *psWindow;

	psWindow = WindowGetPointer(eWindow);
	if (NULL == psWindow)
	{
		return(LERR_WIN_BAD_HANDLE);
	}

	// Modify the image instance offset and view size if specified
	if( pu32XOffset )
	{
		if( *pu32XOffset < psWindow->u32ActiveAreaXSize )
		{
			psWindow->u32ViewportXOffset = *pu32XOffset;
		}
	}
	if( pu32YOffset )
	{
		if( *pu32YOffset < psWindow->u32ActiveAreaYSize )
		{
			psWindow->u32ViewportYOffset = *pu32YOffset;
		}
	}
	if( pu32XSize )
	{
		psWindow->u32ViewportXSize = *pu32XSize;
	}
	if( pu32YSize )
	{
		psWindow->u32ViewportYSize = *pu32YSize;
	}

	// Truncate the size of the viewport to the extent of the window
	if( (psWindow->u32ViewportXSize + psWindow->u32ViewportXOffset) > psWindow->psWindowImage->u32XSize )
	{
		psWindow->u32ViewportXSize = psWindow->psWindowImage->u32XSize - psWindow->u32ViewportXOffset;
	}

	if( (psWindow->u32ViewportYSize + psWindow->u32ViewportYOffset) > psWindow->psWindowImage->u32YSize )
	{
		psWindow->u32ViewportYSize = psWindow->psWindowImage->u32YSize - psWindow->u32ViewportYOffset;
	}

	// Update the image instance viewport info
	GfxSetImageInstanceViewport(psWindow->psWindowImageInstance,
								psWindow->u32ViewportXOffset,
								psWindow->u32ViewportYOffset,
								psWindow->u32ViewportXSize,
								psWindow->u32ViewportYSize );

	// Update the image if it's visible
	WindowUpdateRegion(eWindow,
					   psWindow->u32ActiveAreaXPos,
					   psWindow->u32ActiveAreaYPos,
					   psWindow->u32ActiveAreaXSize,
					   psWindow->u32ActiveAreaYSize);

	// Kick off the update handler thread
	WindowUpdateRegionCommit();

	return(LERR_OK);
}

// Easy scrolling of window
// Offsets are 0-100%
//	0%:		x and y offsets are zero
//  100%:	x and y offsets are maximum - size of viewport
ELCDErr WindowSetViewportScrollPercent (WINDOWHANDLE eWindow,
										  UINT32* pu32XOffsetPercent,
										  UINT32* pu32YOffsetPercent)
{
	SWindow *psWindow;

	psWindow = WindowGetPointer(eWindow);
	if (NULL == psWindow)
	{
		return(LERR_WIN_BAD_HANDLE);
	}

	// Modify the image instance offset and view size if specified
	if( pu32XOffsetPercent )
	{
		GCASSERT(*pu32XOffsetPercent <= 100);

		psWindow->u32ViewportXOffset = *pu32XOffsetPercent * (psWindow->psWindowImage->u32XSize - psWindow->u32ViewportXSize) / 100;
	}
	if( pu32YOffsetPercent )
	{
		GCASSERT(*pu32YOffsetPercent <= 100);

		psWindow->u32ViewportYOffset = *pu32YOffsetPercent * (psWindow->psWindowImage->u32YSize - psWindow->u32ViewportYSize) / 100;
	}

	// Update the image instance viewport info
	GfxSetImageInstanceViewport(psWindow->psWindowImageInstance,
								psWindow->u32ViewportXOffset,
								psWindow->u32ViewportYOffset,
								psWindow->u32ViewportXSize,
								psWindow->u32ViewportYSize );

	// Update the image if it's visible
	WindowUpdateRegion(eWindow,
					   psWindow->u32ActiveAreaXPos,
					   psWindow->u32ActiveAreaYPos,
					   psWindow->u32ActiveAreaXSize,
					   psWindow->u32ActiveAreaYSize);

	// Kick off the update handler thread
	WindowUpdateRegionCommit();

	return(LERR_OK);
}

ELCDErr WindowCreate(WINDOWHANDLE eParentWindow,
					 INT32 s32XPos,
					 INT32 s32YPos,
					 UINT32 u32XSize,
					 UINT32 u32YSize,
					 WINDOWHANDLE *peWindowHandle)
{
	SLayer *psLayer = NULL;
	SWindow *psWindow;
	SWindow *psParentWindow = NULL;
	EGCResultCode eResult;
	ELCDErr eErr;

	// Make sure we have a valid window handle
	GCASSERT(peWindowHandle);

	// Let's see if we have a free slot
	if (FALSE == WindowFindFreeHandle(peWindowHandle))
	{
		return(LERR_WIN_FULL);
	}

	// Let's create an image and an image instance
	psWindow = MemAlloc(sizeof(*psWindow));
	if (NULL == psWindow)
	{
		goto outOfMemory;
	}

	// Make a note of the parent window handle

	psWindow->eParentWindow = eParentWindow;

	if( eParentWindow != HANDLE_INVALID )
	{
		psParentWindow = WindowGetPointer(eParentWindow);
		if (NULL == psParentWindow)
		{
			return(LERR_WIN_BAD_HANDLE);
		}

		// Offset this sub window by the position of the parent window
		if( psParentWindow->psWindowImageInstance )
		{
			s32XPos += psParentWindow->psWindowImageInstance->s32XPos;
			s32YPos += psParentWindow->psWindowImageInstance->s32YPos;
		}
	}

	// Got the window structure created. Let's create a blank image.

	psWindow->psWindowImage = GfxCreateEmptyImage(u32XSize,
												  u32YSize,
												  WINDOW_BPP,
												  0,
												  FALSE);
	if (NULL == psWindow->psWindowImage)
	{
		goto outOfMemory;
	}

	// Let's create a layer

	psLayer = GfxCreateLayer();
	if (NULL == psLayer)
	{
		goto outOfMemory;
	}

	// Got the image created. Now let's create an image instance
	psWindow->psWindowImageInstance = GfxCreateImageInstance(psLayer,
															 psWindow->psWindowImage,
															 s32XPos,
															 s32YPos,
															 FALSE);

	if (NULL == psWindow->psWindowImageInstance)
	{
		goto outOfMemory;
	}

	// Let's connect things up!
	psWindow->bVisible = FALSE;

	// Set up the default active area size/location
	psWindow->u32ActiveAreaXPos = 0;
	psWindow->u32ActiveAreaYPos = 0;
	psWindow->u32ActiveAreaXSize = u32XSize;
	psWindow->u32ActiveAreaYSize = u32YSize;

	// Set up the default viewport size/location
	psWindow->u32ViewportXOffset = 0;
	psWindow->u32ViewportYOffset = 0;
	psWindow->u32ViewportXSize = u32XSize;
	psWindow->u32ViewportYSize = u32YSize;

	GfxSetImageInstanceViewport(psWindow->psWindowImageInstance,
								psWindow->u32ViewportXOffset,
								psWindow->u32ViewportYOffset,
								psWindow->u32ViewportXSize,
								psWindow->u32ViewportYSize );

	// Full intensity
	psWindow->u8WindowIntensity = 0xff;
	psWindow->u8BackgroundIntensityLevel = 0xff;

	// Set the default window region to update (nothing)
	psWindow->u32UpdateRegionXMax = 0;
	psWindow->u32UpdateRegionYMax = 0;
	psWindow->u32UpdateRegionXMin = 0xffffffff;
	psWindow->u32UpdateRegionYMin = 0xffffffff;

	// Record the window handle for later usage
	psWindow->eWindowHandle = *peWindowHandle;

	// Create a couple of semaphores
	eResult = GCOSSemaphoreCreate(&psWindow->sWindowUpdateSem,
								  0);		// Ensure no one can do anything to it
	if (eResult != GC_OK)
	{
		goto outOfMemory;
	}

	eResult = GCOSSemaphoreCreate(&psWindow->sDirtyRegionSem,
								  0);		// Ensure no one can do anything to it
	if (eResult != GC_OK)
	{
		goto outOfMemory;
	}

	// Go erase the active region to set it all up

	WindowEraseActiveRegion(psWindow,
							0,
							0,
							(INT32) u32XSize,
							(INT32) u32YSize);

	eErr = WindowListLock();
	GCASSERT(LERR_OK == eErr);

	// Hook it in and take that slot
	sg_psWindowList[*peWindowHandle] = psWindow;

	psWindow->psNextWindow = sg_psWindowHead;
	sg_psWindowHead = psWindow;
	sg_u32TotalWindows++;

	eErr = WindowListUnlock();
	GCASSERT(LERR_OK == eErr);

	// Release the dirty region lock and window update lock
	eResult = GCOSSemaphorePut(psWindow->sDirtyRegionSem);
	if (eResult != GC_OK)
	{
		goto outOfMemory;
	}
	else
	{
		// Now release the window lock
		return(WindowUnlock(psWindow));
	}

	// Got it!
	return((ELCDErr) (LERR_GC_ERR_BASE + eResult));

outOfMemory:
	if (psWindow)
	{
		// If we have a window image instance, delete it
		if (psWindow->psWindowImageInstance)
		{
			GfxDeleteImageInstance(psWindow->psWindowImageInstance);
			psWindow->psWindowImageInstance = NULL;
		}

		// If we have a window image created, then let's delete it since it didn't work.
		if (psWindow->psWindowImage)
		{
			GCASSERT(0);	// Convert to image group
//			GfxDeleteImage(psWindow->psWindowImage);
			psWindow->psWindowImage = NULL;
		}

		GCFreeMemory(psWindow);
	}

	// Free up the slot
	sg_psWindowList[*peWindowHandle] = NULL;

	if (psLayer)
	{
		GfxDeleteLayer(psLayer);
	}

	// No more memory available to show window
	return(LERR_NO_MEM);
}

ELCDErr WindowSetBackgroundImage(WINDOWHANDLE eWindow,
								 LEX_CHAR *peFilename)
{
	SWindow *psWindow;
	EGCResultCode eGCResult;
	ELCDErr eErrReturn;

	psWindow = WindowGetPointer(eWindow);
	if (NULL == psWindow)
	{
		return(LERR_WIN_BAD_HANDLE);
	}

	// Got our window. Let's see if we already have a background
	GfxDeleteImageGroup(psWindow->psBackgroundImage);
	psWindow->psBackgroundImage = NULL;

	// Let's try to load up the image
	psWindow->psBackgroundImage = GfxLoadImageGroup(peFilename,
													&eGCResult);

	if (eGCResult != GC_OK)
	{
		// Return whatever error we just got
		eErrReturn = (ELCDErr) (eGCResult);
		goto errorReturn;
	}

	GfxIncRef(psWindow->psBackgroundImage);

	// Go erase the active region
	WindowEraseActiveRegion(psWindow,
							0, 0,
							psWindow->u32ActiveAreaXSize,
							psWindow->u32ActiveAreaYSize);

	// Redraw all widgets that are affected
	eErrReturn = WindowRedrawAllWidgets(eWindow);

	// Looks like everything was created/loaded OK (as long as eErrReturn == LERR_OK

	// Cause an update
	WindowUpdateRegion(eWindow,
					   0,
					   0,
					   psWindow->psWindowImageInstance->u32XSizeClipped,
					   psWindow->psWindowImageInstance->u32YSizeClipped);


errorReturn:
	if (eErrReturn != LERR_OK)
	{
		// Don't do anything
	}
	else
	{
		// Update the entire window

		WindowUpdateRegion(eWindow,
						   psWindow->u32ActiveAreaXPos,
						   psWindow->u32ActiveAreaYPos,
						   psWindow->u32ActiveAreaXSize,
						   psWindow->u32ActiveAreaYSize);
		// Kick off the update handler thread
		WindowUpdateRegionCommit();
	}

	return(eErrReturn);
}

ELCDErr WindowSetForegroundColor(WINDOWHANDLE eWindow,
								 UINT16 u16RGBColor)
{
	SWindow *psWindow;

	psWindow = WindowGetPointer(eWindow);
	if (NULL == psWindow)
	{
		return(LERR_WIN_BAD_HANDLE);
	}

	psWindow->u16ForegroundColor = u16RGBColor;
	return(LERR_OK);
}

ELCDErr WindowSetBackgroundColor(WINDOWHANDLE eWindow,
								 UINT16 u16RGBColor)
{
	ELCDErr eErr;
	SWindow *psWindow;

	psWindow = WindowGetPointer(eWindow);
	if (NULL == psWindow)
	{
		return(LERR_WIN_BAD_HANDLE);
	}

	psWindow->u16BackgroundColor = u16RGBColor;

	// Go erase the active region

	WindowEraseActiveRegion(psWindow,
							0, 0,
							psWindow->u32ActiveAreaXSize,
							psWindow->u32ActiveAreaYSize);

	// Redraw all widgets that are affected
	eErr = WindowRedrawAllWidgets(eWindow);
	return(LERR_OK);
}

ELCDErr WindowSetTransparency(WINDOWHANDLE eWindow,
							  UINT8 u8TransparencyLevel)
{
	SWindow *psWindow;
	ELCDErr eErrReturn;

	psWindow = WindowGetPointer(eWindow);
	if (NULL == psWindow)
	{
		return(LERR_WIN_BAD_HANDLE);
	}

	// Don't do anything if there's no change
	if (psWindow->u8WindowIntensity == u8TransparencyLevel)
	{
		return(LERR_OK);
	}

	eErrReturn = WindowListLock();
	RETURN_ON_FAIL(eErrReturn);

	WindowBlitLock(TRUE);

	psWindow->u8WindowIntensity = u8TransparencyLevel;

	// Cause an update

	WindowUpdateRegion(eWindow,
					   0,
					   0,
					   psWindow->psWindowImageInstance->u32XSizeClipped,
					   psWindow->psWindowImageInstance->u32YSizeClipped);

	WindowBlitLock(FALSE);
	(void) WindowListUnlock();

	WindowUpdateRegionCommit();

	return(eErrReturn);
}

ELCDErr WindowSetBackgroundImageIntensity(WINDOWHANDLE eWindow,
										  UINT8 u8IntensityLevel)
{
	SWindow *psWindow;
	ELCDErr eErr = LERR_OK;

	psWindow = WindowGetPointer(eWindow);
	if (NULL == psWindow)
	{
		return(LERR_WIN_BAD_HANDLE);
	}

	// If we don't have an image, just indicate to the caller that we don't have one

	if (NULL == psWindow->psBackgroundImage)
	{
		return(LERR_WIN_NO_BACKGROUND_IMAGE);
	}

	// Okay, we've got to set the background balance. 255=Full, 0=Dark.

	if (psWindow->u8BackgroundIntensityLevel == u8IntensityLevel)
	{
		// Already there.
		return(LERR_OK);
	}

	psWindow->u8BackgroundIntensityLevel = u8IntensityLevel;

	WindowBlitLock(TRUE);

	WindowEraseActiveRegion(psWindow,
							0, 0,
							psWindow->u32ActiveAreaXSize,
							psWindow->u32ActiveAreaYSize);

	// Redraw all widgets
	
	eErr = WindowRedrawAllWidgets(eWindow);

	// Go update the image
	WindowUpdateRegion(eWindow,
					   psWindow->u32ActiveAreaXPos,
					   psWindow->u32ActiveAreaYPos,
					   psWindow->u32ActiveAreaXSize,
					   psWindow->u32ActiveAreaYSize);

	// Kick off the update handler thread
	WindowUpdateRegionCommit();

	WindowBlitLock(FALSE);
	return(eErr);
}


ELCDErr WindowSetVisible(WINDOWHANDLE eWindow,
						 BOOL bWindowVisible)
{
	SWindow *psWindow;
	EGCResultCode eResult;

	// Make sure we have a good handle
	psWindow = WindowGetPointer(eWindow);
	if (NULL == psWindow)
	{
		return(LERR_WIN_BAD_HANDLE);
	}

	// Got a good handle. 

	if (psWindow->bVisible == bWindowVisible)
	{
		// The window is already set to what it is currently. Just return OK.
		return(LERR_OK);
	}

	// Now set the foreground

	if (bWindowVisible)
	{
		eResult = GCOSQueueSend(sg_sWindowQueue,
								(void *) (WCMD_WINDOW_SHOW | (eWindow & WDATA_MASK)));
		GCASSERT(GC_OK == eResult);
	}
	else
	{
		eResult = GCOSQueueSend(sg_sWindowQueue,
								(void *) (WCMD_WINDOW_HIDE | (eWindow & WDATA_MASK)));
		GCASSERT(GC_OK == eResult);
	}

	return(LERR_OK);
}

ELCDErr WindowSetPosition(WINDOWHANDLE eWindowHandle,
								 INT32 s32XPos,
								 INT32 s32YPos)
{
	SWindow *psWindow;
	SWindow *psParentWindow;
	BOOL bVisibleState = FALSE;
	ELCDErr eErr = LERR_OK;
	EGCResultCode eResult;

	// Make sure we have a good handle
	psWindow = WindowGetPointer(eWindowHandle);
	if (NULL == psWindow)
	{
		return(LERR_WIN_BAD_HANDLE);
	}

	if( psWindow->eParentWindow != HANDLE_INVALID )
	{
		psParentWindow = WindowGetPointer(psWindow->eParentWindow);
		GCASSERT(psParentWindow);

		// Offset this sub window by the position of the parent window
		if( psParentWindow->psWindowImageInstance )
		{
			s32XPos += psParentWindow->psWindowImageInstance->s32XPos;
			s32YPos += psParentWindow->psWindowImageInstance->s32YPos;
		}
	}

	GfxSetImageInstance(psWindow->psWindowImageInstance,
						s32XPos,
						s32YPos,
						psWindow->bVisible);

	eResult = GCOSQueueSend(sg_sWindowQueue,
							(void *) (WCMD_WINDOW_FORCE_BLIT));
	if (eResult != GC_OK)
	{
		eErr = (ELCDErr) (LERR_GC_ERR_BASE + eResult);
	}	

	return(eErr);
}

ELCDErr WindowFill(WINDOWHANDLE eWindow,
				   UINT16 u16FillColor)
{
	SWindow *psWindow;
	UINT16 *pu16Data;
	UINT32 u32Loop;
	UINT32 u32Loop2;

	// Make sure we have a good handle
	psWindow = WindowGetPointer(eWindow);
	if (NULL == psWindow)
	{
		return(LERR_WIN_BAD_HANDLE);
	}

	pu16Data = psWindow->psWindowImage->pu16ImageData;
	for (u32Loop = 0; u32Loop < psWindow->psWindowImage->u32YSize; u32Loop++)
	{
		u32Loop2 = psWindow->psWindowImage->u32XSize;
		while (u32Loop2--)
		{
			*pu16Data = COLOR_ADJUST_INTENSITY(u16FillColor, psWindow->u8WindowIntensity);
			pu16Data++;
		}

		pu16Data -= psWindow->psWindowImage->u32XSize;
		pu16Data += psWindow->psWindowImage->u32Pitch;
	}

	// Update the image if it's visible
	WindowUpdateRegion(eWindow,
					   psWindow->u32ActiveAreaXPos,
					   psWindow->u32ActiveAreaYPos,
					   psWindow->u32ActiveAreaXSize,
					   psWindow->u32ActiveAreaYSize);

	// Kick off the update handler thread
	WindowUpdateRegionCommit();
	return(LERR_OK);
}

static UINT8 sg_u8RoundingTable[0x100] =
{
    0x00, 0x01, 0x03, 0x04, 0x06, 0x07, 0x09, 0x0a,
    0x0c, 0x0e, 0x0f, 0x11, 0x12, 0x14, 0x15, 0x17,
    0x19, 0x1a, 0x1c, 0x1d, 0x1f, 0x20, 0x22, 0x24,
    0x25, 0x27, 0x28, 0x2a, 0x2b, 0x2d, 0x2e, 0x30,
    0x31, 0x33, 0x35, 0x36, 0x38, 0x39, 0x3b, 0x3c,
    0x3e, 0x3f, 0x41, 0x42, 0x44, 0x45, 0x47, 0x48,
    0x4a, 0x4b, 0x4d, 0x4e, 0x50, 0x51, 0x53, 0x54,
    0x56, 0x57, 0x59, 0x5a, 0x5c, 0x5d, 0x5f, 0x60,
    0x61, 0x63, 0x64, 0x66, 0x67, 0x69, 0x6a, 0x6c,
    0x6d, 0x6e, 0x70, 0x71, 0x73, 0x74, 0x75, 0x77,
    0x78, 0x7a, 0x7b, 0x7c, 0x7e, 0x7f, 0x80, 0x82,
    0x83, 0x84, 0x86, 0x87, 0x88, 0x8a, 0x8b, 0x8c,
    0x8e, 0x8f, 0x90, 0x92, 0x93, 0x94, 0x95, 0x97,
    0x98, 0x99, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f, 0xa1,
    0xa2, 0xa3, 0xa4, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa,
    0xab, 0xad, 0xae, 0xaf, 0xb0, 0xb1, 0xb2, 0xb3,
    0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc,
    0xbd, 0xbe, 0xbf, 0xc0, 0xc1, 0xc2, 0xc3, 0xc4,
    0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc,
    0xcd, 0xce, 0xcf, 0xd0, 0xd1, 0xd2, 0xd3, 0xd3,
    0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xd9, 0xda,
    0xdb, 0xdc, 0xdd, 0xdd, 0xde, 0xdf, 0xe0, 0xe1,
    0xe1, 0xe2, 0xe3, 0xe3, 0xe4, 0xe5, 0xe6, 0xe6,
    0xe7, 0xe8, 0xe8, 0xe9, 0xea, 0xea, 0xeb, 0xeb,
    0xec, 0xed, 0xed, 0xee, 0xee, 0xef, 0xef, 0xf0,
    0xf1, 0xf1, 0xf2, 0xf2, 0xf3, 0xf3, 0xf4, 0xf4,
    0xf4, 0xf5, 0xf5, 0xf6, 0xf6, 0xf7, 0xf7, 0xf7,
    0xf8, 0xf8, 0xf9, 0xf9, 0xf9, 0xfa, 0xfa, 0xfa,
    0xfb, 0xfb, 0xfb, 0xfb, 0xfc, 0xfc, 0xfc, 0xfc,
    0xfd, 0xfd, 0xfd, 0xfd, 0xfe, 0xfe, 0xfe, 0xfe,
    0xfe, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

#define	LINEAR_START	0x70
#define LINEAR_END		0xf0

static UINT8 *RenderCornerImage(EShadowStyle eStyle,
								UINT32 u32Thickness)
{
	UINT8 *pu8Image;

	// If there's no thickness, return NULL
	if (0 == u32Thickness)
	{
		return(NULL);
	}

	// Add +1 to x/y size so there's a + in the middle of the whole image 
	pu8Image = MemAlloc(((u32Thickness << 1) + 1) * ((u32Thickness << 1) + 1));
	if (NULL == pu8Image)
	{
		// Not enough memory
		return(NULL);
	}

	// If the style is linear, go render it
	if (SSTYLE_LINEAR == eStyle)
	{
		INT32 s32StepValue;
		INT32 s32StepCounter = (LINEAR_END << 16);
		UINT32 u32Pitch;
		UINT8 *pu8Temp;
		UINT8 *pu8Ptr;
		UINT32 u32Loop;

		u32Pitch = (u32Thickness << 1) + 1;
		pu8Ptr = pu8Image;

		s32StepValue = ((LINEAR_END - LINEAR_START) << 16) / u32Thickness;
		for (u32Loop = u32Thickness; u32Loop != 0; u32Loop--)
		{
			UINT32 u32Loop2;

			// Top part of box
			memset((void *) pu8Ptr, (int) (s32StepCounter >> 16), (u32Loop << 1) + 1);

			// Bottom part of box
			memset((void *) (pu8Ptr + ((u32Loop << 1) * u32Pitch)), (int) (s32StepCounter >> 16), (u32Loop << 1) + 1);

			pu8Temp = pu8Ptr;

			for (u32Loop2 = 0; u32Loop2 < ((u32Loop << 1) + 1); u32Loop2++)
			{
				*pu8Temp = (UINT8) (s32StepCounter >> 16);
				*(pu8Temp + (u32Loop << 1) + 1) = (UINT8) (s32StepCounter >> 16);
				pu8Temp += u32Pitch;
			}

			s32StepCounter -= s32StepValue;
			pu8Ptr += (u32Pitch + 1);
		}
	}

#define	PIR	(3.1415926535 / 180.0)

	if (SSTYLE_ROUNDED == eStyle)
	{
		INT32 s32XPos = 0;
		INT32 s32YPos = 0;
		INT32 s32XOld = 0;
		INT32 s32YOld = 0;
		double dAngle;
		double dStep;
		UINT8 *pu8Ptr;
		UINT32 u32Fixed = (0xff << 16) / u32Thickness;

		// Draw a filled half circle
		memset((void *) pu8Image, 0xff, ((u32Thickness << 1) + 1) *
										((u32Thickness << 1) + 1));

		dStep = 5.0;
		for (dAngle = 270.0; dAngle >= 90.0; dAngle -= dStep)
		{
			s32XPos = (INT32) (((double) u32Thickness) * cos(dAngle * PIR)) + (INT32) u32Thickness;
			s32YPos = (INT32) (((double) u32Thickness) * sin(dAngle * PIR)) + (INT32) u32Thickness;

			pu8Ptr = pu8Image + (s32XPos + (s32YPos * ((u32Thickness << 1) + 1)));
			memset((void *) pu8Ptr, 0, (u32Thickness - (UINT32) s32XPos) << 1);
		}

		// Now run through it, compute distance, weigh it against the table, and
		// create an appropriate mask

		pu8Ptr = pu8Image;

		for (s32YPos = 0; s32YPos < (INT32) ((u32Thickness << 1) + 1); s32YPos++)
		{
			for (s32XPos = 0; s32XPos < (INT32) ((u32Thickness << 1) + 1); s32XPos++)
			{
				if (0 == *pu8Ptr)
				{
					INT32 s32XDistance;
					INT32 s32YDistance;
					UINT32 u32Distance;

					s32XDistance = abs(((INT32) u32Thickness) - s32XPos);
					s32XDistance *= s32XDistance;
					s32YDistance = abs(((INT32) u32Thickness) - s32YPos);
					s32YDistance *= s32YDistance;
					u32Distance = (UINT32) (sqrt(s32XDistance + s32YDistance) + 0.5);

					u32Distance = (u32Distance * u32Fixed) >> 16;
					*pu8Ptr = sg_u8RoundingTable[u32Distance];
				}

				++pu8Ptr;
			}
		}
	}

	// Not a supported style. Return nothing.
	return(pu8Image);
}

static void CopyCorner(UINT8 *pu8SourceMap,
					   UINT32 u32Thickness,
					   SImage *psDest,			// Destination image
					   EShadowCornerOrigin eCorner,
					   UINT16 u16Color)
{
	UINT32 u32SrcPitch;
	UINT32 u32XDest = 0;
	UINT32 u32YDest = 0;
	UINT8 *pu8DestMask = psDest->pu8TranslucentMask;
	UINT16 *pu16DestImage = psDest->pu16ImageData;
	UINT32 u32ThicknessCounter;

	u32SrcPitch = (u32Thickness << 1) + 1;

	if (SORG_CORNER_UPPER_LEFT == eCorner)
	{
		// No need to set - both are 0, 0
	}
	else
	if (SORG_CORNER_UPPER_RIGHT == eCorner)
	{
		u32XDest = psDest->u32XSize - u32Thickness;
		pu8SourceMap += (u32Thickness + 1);
	}
	else
	if (SORG_CORNER_LOWER_LEFT == eCorner)
	{
		u32YDest = psDest->u32YSize - u32Thickness;
		pu8SourceMap += ((u32Thickness + 1) * u32SrcPitch);
	}
	else
	if (SORG_CORNER_LOWER_RIGHT == eCorner)
	{
		u32XDest = psDest->u32XSize - u32Thickness;
		u32YDest = psDest->u32YSize - u32Thickness;
		pu8SourceMap += ((u32Thickness + 1) * u32SrcPitch);
		pu8SourceMap += (u32Thickness + 1);
	}
	else
	{
		GCASSERT(0);
	}

	pu16DestImage = psDest->pu16ImageData + u32XDest + (u32YDest * psDest->u32Pitch);

	if (psDest->pu8TranslucentMask)
	{
		pu8DestMask = psDest->pu8TranslucentMask + u32XDest + (u32YDest * psDest->u32Pitch);
	}
	else
	if (psDest->pu8Transparent)
	{
		pu8DestMask = psDest->pu8Transparent + u32XDest + (u32YDest * psDest->u32Pitch);
	}
	else
	{
		// Shouldn't be getting here if there's no translucent/transparent mask
		GCASSERT(0);
	}

	u32ThicknessCounter = u32Thickness;

	while (u32ThicknessCounter)
	{
		UINT32 u32Loop;

		memcpy((void *) pu8DestMask, pu8SourceMap, u32Thickness);
		for (u32Loop = 0; u32Loop < u32Thickness; u32Loop++)
		{
			*(u32Loop + pu16DestImage) = u16Color;
		}

		pu8DestMask += psDest->u32Pitch;
		pu16DestImage += psDest->u32Pitch;
		pu8SourceMap += u32SrcPitch;
		--u32ThicknessCounter;
	}
}

static void WindowShadowRenderEdge(SWindow *psWindow,
								   UINT8 *pu8MaskPtr,
								   EShadowOrigin eOrigin)
{
	UINT16 *pu16ImagePtr = NULL;
	SShadow *psShadow = &psWindow->sShadow[eOrigin];
	UINT32 u32Index;
	UINT16 u16Color;
	UINT32 u32Loop;
	UINT32 u32Loop2;

	if ((FALSE == psShadow->bActive) ||
		(SSTYLE_NONE == psShadow->eStyle))
	{
		// Shut off - don't do anything
		return;
	}

	u16Color = psShadow->u16Color;

	// Calculate the appropriate starting point for the image pixels and mask

	if (SORG_TOP == eOrigin)
	{
		// Upper left hand corner of the image
		u32Index = ((INT32) psWindow->u32ActiveAreaXPos) + psShadow->s32Offset;
	}
	else
	if (SORG_LEFT == eOrigin)
	{
		// Upper left hand corner of image, normalized to u32ActiveAreaYPos
		u32Index = ((((INT32) psWindow->u32ActiveAreaYPos) + psShadow->s32Offset) * psWindow->psWindowImage->u32Pitch);
	}
	else
	if (SORG_RIGHT == eOrigin)
	{
		// Upper right hand corner of image, normalized to u32ActiveAreaYPos
		u32Index = ((((INT32) psWindow->u32ActiveAreaYPos) + psShadow->s32Offset) * psWindow->psWindowImage->u32Pitch) +
				   psWindow->u32ActiveAreaXPos + psWindow->u32ActiveAreaXSize;
	}
	else
	if (SORG_BOTTOM == eOrigin)
	{
		// Lower left hand corner of the image
		u32Index = ((INT32) ((psWindow->u32ActiveAreaYPos + psWindow->u32ActiveAreaYSize) * psWindow->psWindowImage->u32Pitch) +
					(INT32) (psWindow->u32ActiveAreaXPos) + psShadow->s32Offset);
	}

	// Set up our image pointer and mask pointer with our new index

	pu16ImagePtr = psWindow->psWindowImage->pu16ImageData + u32Index;
	pu8MaskPtr += u32Index;

	// pu16ImagePtr and pu8MaskPtr are now properly aligned. Time to draw whatever.
	
	// Render the side in question with a solid color no matter the type. The
	// only thing that makes things different is the shading.

	if ((SORG_TOP == eOrigin) || (SORG_BOTTOM == eOrigin))
	{
		// Horizontal line

		for (u32Loop = 0; u32Loop < psShadow->u32Thickness; u32Loop++)
		{
			for (u32Loop2 = 0; u32Loop2 < psShadow->u32Length; u32Loop2++)
			{
				*pu16ImagePtr = u16Color;
				++pu16ImagePtr;
			}

			pu16ImagePtr -= psShadow->u32Length;
			pu16ImagePtr += psWindow->psWindowImage->u32Pitch;
		}
	}
	else
	{
		// We're vertical

		for (u32Loop = 0; u32Loop < psShadow->u32Length; u32Loop++)
		{
			for (u32Loop2 = 0; u32Loop2 < psShadow->u32Thickness; u32Loop2++)
			{
				*pu16ImagePtr = u16Color;
				++pu16ImagePtr;
			}

			pu16ImagePtr -= psShadow->u32Thickness;
			pu16ImagePtr += psWindow->psWindowImage->u32Pitch;
		}
	}

	// If it's a line style, set everything to solid

	if (SSTYLE_LINE == psShadow->eStyle)
	{
		// Make the shading solid.

		if ((SORG_TOP == eOrigin) || (SORG_BOTTOM == eOrigin))
		{
			// Horizontal line

			for (u32Loop = 0; u32Loop < psShadow->u32Thickness; u32Loop++)
			{
				memset((void *) pu8MaskPtr, 0, psShadow->u32Length);
				pu8MaskPtr += psWindow->psWindowImage->u32Pitch;
			}
		}
		else
		{
			// Vertical line

			for (u32Loop = 0; u32Loop < psShadow->u32Length; u32Loop++)
			{
				memset((void *) pu8MaskPtr, 0, psShadow->u32Thickness);
				pu8MaskPtr += psWindow->psWindowImage->u32Pitch;
			}
		}

		return;
	}

	// If it's a linear one, we need to do some interesting shading

	if ((SSTYLE_LINEAR == psShadow->eStyle) ||
		(SSTYLE_ROUNDED == psShadow->eStyle))
	{
		INT32 s32StepCounter;
		INT32 s32StepCounterOriginal;
		INT32 s32StepValue;

		/******************************* LINEAR/ROUNDED ******************************/

		// 0xff=Fully transparent
		// 0x00=Solid

		if ((SORG_TOP == eOrigin) || (SORG_BOTTOM == eOrigin))
		{
			// Vertical - Top goes from transparent->solid, bottom goes from solid->transparent

			s32StepValue = 0x1000000 / (psShadow->u32Thickness - 1);

			if (SORG_TOP == eOrigin)
			{
				// Transparent->solid
				s32StepCounter = 0xffffff;
				s32StepValue = -s32StepValue;
			}
			else
			{
				// Solid->transparent
				s32StepCounter = 0;
			}

			for (u32Loop = 0; u32Loop < psShadow->u32Thickness; u32Loop++)
			{
				if (SSTYLE_ROUNDED == psShadow->eStyle)
				{
					memset((void *) pu8MaskPtr, sg_u8RoundingTable[(UINT8) (s32StepCounter >> 16)], psShadow->u32Length);
				}
				else
				{
					UINT8 u8Value;

					u8Value = (UINT8) (((LINEAR_END - LINEAR_START) * s32StepCounter) >> 24) + LINEAR_START;
					memset((void *) pu8MaskPtr, u8Value, psShadow->u32Length);
				}

				pu8MaskPtr += psWindow->psWindowImage->u32Pitch;
				s32StepCounter += s32StepValue;
			}
		}
		else
		{
			// Horizontal - Left goes from transparent->solid, bottom goes from solid->transparent

			s32StepValue = 0x1000000 / (psShadow->u32Thickness - 1);

			if (SORG_LEFT == eOrigin)
			{
				// Transparent->solid
				s32StepCounterOriginal = 0xffffff;
				s32StepValue = -s32StepValue;
			}
			else
			{
				// Solid->transparent
				s32StepCounterOriginal = 0;
			}

			for (u32Loop = 0; u32Loop < psShadow->u32Length; u32Loop++)
			{
				s32StepCounter = s32StepCounterOriginal;

				for (u32Loop2 = 0; u32Loop2 < psShadow->u32Thickness; u32Loop2++)
				{
					UINT8 u8Value;

					if (SSTYLE_LINEAR == psShadow->eStyle)
					{
						u8Value = (UINT8) (((LINEAR_END - LINEAR_START) * s32StepCounter) >> 24) + LINEAR_START;
					}
					else
					{
						u8Value = sg_u8RoundingTable[(UINT8) (s32StepCounter >> 16)];
					}

					if (*pu8MaskPtr > u8Value)
					{
						*pu8MaskPtr = u8Value;
					}

					++pu8MaskPtr;
					s32StepCounter += s32StepValue;
				}

				pu8MaskPtr -= psShadow->u32Thickness;
				pu8MaskPtr += psWindow->psWindowImage->u32Pitch;
			}
		}

		return;
	}
}

typedef struct SCornerGroups
{
	EShadowStyle eOrg1;
	EShadowStyle eOrg2;
	EShadowCornerOrigin eOrigin;
} SCornerGroups;

static SCornerGroups sg_sCorners[] =
{
	{SORG_TOP,		SORG_LEFT,	SORG_CORNER_UPPER_LEFT},
	{SORG_TOP,		SORG_RIGHT,	SORG_CORNER_UPPER_RIGHT},
	{SORG_BOTTOM,	SORG_LEFT,	SORG_CORNER_LOWER_LEFT},
	{SORG_BOTTOM,	SORG_RIGHT,	SORG_CORNER_LOWER_RIGHT}
};

static BOOL WindowShadowRender(SWindow *psWindow)
{
	EShadowStyle eStyleMax = SSTYLE_NONE;
	UINT32 u32Loop;
	UINT8 *pu8Transparent = NULL;
	UINT8 *pu8Translucent = NULL;
	UINT8 *pu8MaskPtr = NULL;

	// It works something like this:
	//
	// SSTYLE_NONE		- Not transparent/not translucent
	// SSTYLE_LINE		- Transparent
	// all else			- Translucent

	for (u32Loop = 0; u32Loop < (sizeof(psWindow->sShadow) / sizeof(psWindow->sShadow[0])); u32Loop++)
	{
		// Only pay attention to the shadow if it's active

		if (psWindow->sShadow[u32Loop].bActive)
		{
			if (psWindow->sShadow[u32Loop].eStyle > eStyleMax)
			{
				eStyleMax = psWindow->sShadow[u32Loop].eStyle;
			}
		}
	}

	// Reallocate transparency/translucency memory needed for this image

	if (SSTYLE_NONE == eStyleMax)
	{
		// None needed!
	}
	else
	if (SSTYLE_LINE == eStyleMax)
	{
		// Must be transparent, but NOT translucent

		pu8Transparent = MemAlloc(psWindow->psWindowImage->u32Pitch *
										  psWindow->psWindowImage->u32YSize);

		if (NULL == pu8Transparent)
		{
			// Out of memory
			return(FALSE);
		}

		memset((void *) pu8Transparent, 0xff, psWindow->psWindowImage->u32Pitch *
											  psWindow->psWindowImage->u32YSize);
	}
	else
	{
		// Must be translucent

		pu8Translucent = MemAlloc(psWindow->psWindowImage->u32Pitch *
										  psWindow->psWindowImage->u32YSize);
		if (NULL == pu8Translucent)
		{
			// Out of memory
			return(FALSE);
		}

		// Set everything to 100% transparent by default
		memset((void *) pu8Translucent, 0xff, psWindow->psWindowImage->u32Pitch *
											  psWindow->psWindowImage->u32YSize);
	}

	// Ensure we have no transparency/translucency

	if (psWindow->psWindowImage->pu8TranslucentMask)
	{
		GCFreeMemory(psWindow->psWindowImage->pu8TranslucentMask);
		psWindow->psWindowImage->pu8TranslucentMask = NULL;
	}

	if (psWindow->psWindowImage->pu8Transparent)
	{
		GCFreeMemory(psWindow->psWindowImage->pu8Transparent);
		psWindow->psWindowImage->pu8Transparent = NULL;
	}

	// Whichever mask we have, let's set everything to completely transparent

	if (pu8Translucent)
	{
		pu8MaskPtr = pu8Translucent;
	}
	else
	{
		pu8MaskPtr = pu8Transparent;
	}

	// Only reconnect things if we have a max style that is != SSTYLE_NONE

	psWindow->psWindowImage->bTranslucent = FALSE;
	if (eStyleMax != SSTYLE_NONE)
	{
		UINT8 *pu8MaskTmp = NULL;

		// There had better be at least one transparent or translucent mask
		GCASSERT(pu8Translucent || pu8Transparent);

		// Now pu8MaskPtr points to the appropriate segment of memory that needs
		// to have transparency/translucency. Set the active area to complete
		// solid. Adjust pu8MaskTmp to be the upper left hand corner of the active
		// area.

		pu8MaskTmp = pu8MaskPtr + (psWindow->u32ActiveAreaYPos * psWindow->psWindowImage->u32Pitch) +
								  psWindow->u32ActiveAreaXPos;

		// Run through all lines of where this image is, and memset everything to 0 (solid)

		for (u32Loop = 0; u32Loop < psWindow->u32ActiveAreaYSize; u32Loop++)
		{
			memset((void *) pu8MaskTmp, 0, psWindow->u32ActiveAreaXSize);
			pu8MaskTmp += psWindow->psWindowImage->u32Pitch;
		}

		// Attach the transparency/translucency head pointers to our window image structure
		psWindow->psWindowImage->pu8TranslucentMask = pu8Translucent;
		psWindow->psWindowImage->pu8Transparent = pu8Transparent;
		psWindow->psWindowImageInstance->pu8TranslucentMask = pu8Translucent;
		psWindow->psWindowImageInstance->pu8Transparent = pu8Transparent;

		if (pu8Translucent)
		{
			psWindow->psWindowImage->bTranslucent = TRUE;
		}
	}

	// Now render all sides

	for (u32Loop = 0; u32Loop < (sizeof(psWindow->sShadow) / sizeof(psWindow->sShadow[0])); u32Loop++)
	{
		// Only pay attention to the shadow if it's active
		WindowShadowRenderEdge(psWindow,
							   pu8MaskPtr,
							   u32Loop);
	}

	// Add corner renderings depending upon the style


	// Top and left corners available? Gotta render a corner

	for (u32Loop = 0; u32Loop < (sizeof(sg_sCorners) / sizeof(sg_sCorners[0])); u32Loop++)
	{
		EShadowStyle eOrg1;
		EShadowStyle eOrg2;

		eOrg1 = sg_sCorners[u32Loop].eOrg1;
		eOrg2 = sg_sCorners[u32Loop].eOrg2;

		if (psWindow->sShadow[eOrg1].bActive &&
			psWindow->sShadow[eOrg2].bActive)
		{
			UINT8 *pu8ShadowImage = NULL;

			pu8ShadowImage = RenderCornerImage(psWindow->sShadow[eOrg1].eStyle,
											   psWindow->sShadow[eOrg1].u32Thickness);

			if (pu8ShadowImage)
			{
				CopyCorner(pu8ShadowImage,
						   psWindow->sShadow[eOrg1].u32Thickness,
						   psWindow->psWindowImage,
						   sg_sCorners[u32Loop].eOrigin,
						   psWindow->sShadow[eOrg1].u16Color);

				GCFreeMemory(pu8ShadowImage);
			}
		}
	}

	return(TRUE);
}

ELCDErr WindowSetShadow(WINDOWHANDLE eWINDOWHANDLE,
					    EShadowStyle eStyle,
					    EShadowOrigin eOrigin,
					    UINT32 u32Thickness,
						INT32 s32Offset,
						UINT32 u32Length,
						UINT16 u16Color)
{
	SWindow *psWindow;
	BOOL bChanged = FALSE;
	BOOL bStyleChanged = FALSE;
	BOOL bSizeChanged = FALSE;
	BOOL bColorChanged = FALSE;
	ELCDErr eErr = LERR_OK;

	// Make sure we have a good handle
	psWindow = WindowGetPointer(eWINDOWHANDLE);
	if (NULL == psWindow)
	{
		return(LERR_WIN_BAD_HANDLE);
	}

	// Validate the style
	if (eStyle >= SSTYLE_COUNT)
	{
		return(LERR_WIN_INVALID_STYLE);
	}

	// Now validate the origin
	if (eOrigin >= SORG_COUNT)
	{
		return(LERR_WIN_INVALID_ORIGIN);
	}

	// Let's see if we're changing anything - start with the style

	if ((psWindow->sShadow[eOrigin].eStyle == eStyle) ||
		(eStyle != NO_CHANGE))
	{
		bStyleChanged = TRUE;
	}

	// If this shadow is false
	if (u32Thickness && (FALSE == psWindow->sShadow[eOrigin].bActive))
	{
		// Means we're creating a new shadow

		bSizeChanged = TRUE;
	}

	if ((0 == u32Thickness) & (psWindow->sShadow[eOrigin].bActive))
	{
		// Means we're deleting it

		bSizeChanged = TRUE;
	}

	if ((psWindow->sShadow[eOrigin].bActive) &&
		(psWindow->sShadow[eOrigin].u32Thickness != u32Thickness))
	{
		// Means we're resizing an existing shadow

		bSizeChanged = TRUE;
	}

	// If this offset isn't the same, and it's not NO_CHANGE, then we need
	// to indicate that size has changed

	if ((s32Offset != psWindow->sShadow[eOrigin].s32Offset) ||
		(u32Length != psWindow->sShadow[eOrigin].u32Length))
	{
		// Offset of the shadow
		bSizeChanged = TRUE;
	}

	if (psWindow->sShadow[eOrigin].u16Color != u16Color)
	{
		// This means the color has changed
		bColorChanged = TRUE;
	}

	// If we need to change size, then we need to create a new image, copy the old data
	// from the old image to the new image, etc.. big PITA

	if (bSizeChanged)
	{
		SImage *psNewImage;
		UINT32 u32XSize;
		UINT32 u32YSize;
		UINT32 u32OldThickness;
		UINT16 *pu16OldActiveRegionPointer;
		UINT16 *pu16NewActiveRegionPointer;
		UINT32 u32Loop;
		BOOL bVisible;

		// Calculate where our upper left hand corner of the active region
		// is. We're going to be copying from this source area.

		pu16OldActiveRegionPointer = psWindow->psWindowImage->pu16ImageData +
									 (psWindow->u32ActiveAreaXPos) + 
									 (psWindow->u32ActiveAreaYPos * psWindow->psWindowImage->u32Pitch);

		u32OldThickness = psWindow->sShadow[eOrigin].u32Thickness;
		psWindow->sShadow[eOrigin].u32Thickness = u32Thickness;
		if (0 == u32Thickness)
		{
			psWindow->sShadow[eOrigin].bActive = FALSE;
		}
		else
		{
			psWindow->sShadow[eOrigin].bActive = TRUE;
		}

		// Store the offset
		psWindow->sShadow[eOrigin].s32Offset = s32Offset;

		// And the length
		psWindow->sShadow[eOrigin].u32Length = u32Length;

		eErr = WindowLock(psWindow);
		if (eErr != LERR_OK)
		{
			goto errorExit;
		}

		// Go figure out how big the new window needs to be
		WindowCalcOverallWindowSize(psWindow,
									&u32XSize,
									&u32YSize);

		psNewImage = GfxCreateEmptyImage(u32XSize,
										 u32YSize,
										 WINDOW_BPP,
										 0,
										 FALSE);
		if (NULL == psNewImage)
		{
			// Wasn't enough memory. ;-(

			psWindow->sShadow[eOrigin].u32Thickness = u32OldThickness;
			(void) WindowUnlock(psWindow);
			return(LERR_NO_MEM);
		}

		// Recalc the window's active region
		WindowCalcActiveRegionPosition(psWindow);

		// New active region
		pu16NewActiveRegionPointer = psNewImage->pu16ImageData +
									 (psWindow->u32ActiveAreaXPos) + 
									 (psWindow->u32ActiveAreaYPos * psNewImage->u32Pitch);

		// Let's copy from the old image to the new image

		for (u32Loop = 0; u32Loop < psWindow->u32ActiveAreaYSize; u32Loop++)
		{
			memcpy((void *) pu16NewActiveRegionPointer,
				   (void *) pu16OldActiveRegionPointer,
				   psWindow->u32ActiveAreaXSize << 1);
			pu16OldActiveRegionPointer += psWindow->psWindowImage->u32Pitch;
			pu16NewActiveRegionPointer += psNewImage->u32Pitch;
		}

		bVisible = psWindow->psWindowImageInstance->bImageVisible;

		// Turn off image from being visible
		GfxSetImageInstance(psWindow->psWindowImageInstance,
							NO_CHANGE,
							NO_CHANGE,
							FALSE);

		// Get rid of the window image
		GfxDeleteImage(psWindow->psWindowImage);

		// Replace this instance's image
		GfxReplaceInstanceImage(psWindow->psWindowImageInstance,
								psNewImage);

		// Set the image back to what it was again
		GfxSetImageInstance(psWindow->psWindowImageInstance,
							NO_CHANGE,
							NO_CHANGE,
							bVisible);

		// Get rid of everything out of memory now
		psWindow->psWindowImage = psNewImage;

		// Force rerendering
		bStyleChanged = TRUE;
	}

	psWindow->sShadow[eOrigin].u16Color = u16Color;

	if (bStyleChanged)
	{
		psWindow->sShadow[eOrigin].eStyle = eStyle;

		// Rerender the entire set of lines
		if (FALSE == WindowShadowRender(psWindow))
		{
			// Unlock the window
			(void) WindowUnlock(psWindow);
			return(LERR_NO_MEM);
		}
	}

	// Unlock the window
	eErr = WindowUnlock(psWindow);
	if (eErr != LERR_OK)
	{
		goto errorExit;
	}

	// Indicate we've been updated
	WindowUpdateRegion(eWINDOWHANDLE,
					   0, 0,
					   psWindow->psWindowImage->u32XSize,
					   psWindow->psWindowImage->u32YSize);

	// Kick off the update handler thread
	WindowUpdateRegionCommit();

errorExit:
	return(eErr);
}

ELCDErr WindowWidgetConnect(WINDOWHANDLE eWindowHandle,
							struct SWidget *psWidget)
{
	SWindow *psWindow;
	SWidget *psWidgetPtr = NULL;
	SWidget *psWidgetPriorPtr = NULL;

	// Make sure we have a good handle
	psWindow = WindowGetPointer(eWindowHandle);
	if (NULL == psWindow)
	{
		return(LERR_WIN_BAD_HANDLE);
	}

	// Get it out of the "widgets without windows" list if it's there
	WidgetRemoveFromWidgetsWithoutWindowsList(psWidget);

	// Good window handle. Let's connect it in to the head of this window's widget
	// linked list.

	psWidgetPtr = psWindow->psWidgetHead;
	while (psWidgetPtr)
	{
		// If this asserts, the widget is already connected to the window
		GCASSERT(psWidgetPtr != psWidget);
		psWidgetPtr = psWidgetPtr->psNextLink;
	}

	psWidget->psNextLink = psWindow->psWidgetHead;
	psWindow->psWidgetHead = psWidget;

	return(LERR_OK);
}

ELCDErr WindowWidgetDisconnect(WINDOWHANDLE eWindowHandle,
							   struct SWidget *psWidgetToDelete)
{
	SWindow *psWindow;
	SWidget *psWidgetPrior = NULL;
	SWidget *psWidget = NULL;

	// Make sure we have a good handle
	psWindow = WindowGetPointer(eWindowHandle);
	if (NULL == psWindow)
	{
		return(LERR_WIN_BAD_HANDLE);
	}

	psWidget = psWindow->psWidgetHead;

	// Now find it in the list if we can.

	while (psWidget && psWidget != psWidgetToDelete)
	{
		psWidgetPrior = psWidget;
		psWidget = psWidget->psNextLink;
	}

	if (NULL == psWidget)
	{
		// Trying to delete a widget that's not connected to this window
		return(LERR_WIN_WIDGET_NOT_CONNECTED);
	}

	if (psWidgetPrior)
	{
		// Unlinked!
		psWidgetPrior->psNextLink = psWidget->psNextLink;
	}
	else
	{
		psWindow->psWidgetHead = psWindow->psWidgetHead->psNextLink;
	}

	return(LERR_OK);
}

#define	NOT_SET		0xffffffff

ELCDErr WindowSetModal(WINDOWHANDLE eWindowHandle)
{
	EGCResultCode eResult;

	eResult = GCOSQueueSend(sg_sWindowQueue,
							(void *) (WCMD_WINDOW_SET_MODAL | (eWindowHandle & WDATA_MASK)));

	return((ELCDErr) (LERR_GC_ERR_BASE + eResult));
}

void WindowSetMousewheel(UINT32 u32Value)
{
	// Ignore error codes - we'll get it on the next move
	(void) GCOSQueueSend(sg_sWindowQueue,
						 (void *) (WCMD_WINDOW_MOUSEWHEEL | (u32Value & WDATA_MASK)));
}

typedef enum
{
	EMOUSE_BUTTON_PRESSED,
	EMOUSE_BUTTON_RELEASED,
	EMOUSE_POSITION_CHANGED
} EMouseAction;

static void WindowAction(UINT32 u32ButtonMask,
						 UINT32 u32XPointerPos,
						 UINT32 u32YPointerPos,
						 EMouseAction eAction)
{
	ELCDErr eErr;
	SWindow *psWindow;
	SWindow *psSubWindow = NULL;
	UINT32 u32XViewportPointerPos;
	UINT32 u32YViewportPointerPos;

	eErr = WindowListLock();
	GCASSERT(LERR_OK == eErr);

	psWindow = sg_psWindowHead;

	while (psWindow)
	{
		// Are we doing a modal window? If so, then see if this window is the one we're looking for so
		// we don't pass events to any other windows
		if (sg_eModalWindow != HANDLE_INVALID)
		{
			psSubWindow = psWindow;

			// Search the ancestor windows for one that is modal
			while( psSubWindow != NULL )
			{
				// If we found the modal window, quit prematurely
				if( sg_eModalWindow == psSubWindow->eWindowHandle )
				{
					break;
				}

				psSubWindow = WindowGetPointer(psSubWindow->eParentWindow);
			}

			// If we came out of the loop we either hit the end or found a the modal window
			if( (NULL == psSubWindow) ||
				(psSubWindow && (sg_eModalWindow != psSubWindow->eWindowHandle)) )
			{
				// Nope.  This is not modal window or the child of one.  Move on.
				goto nextWindow;
			}
		}

		// We only care if the window is visible
//		GCprintf(0xffff, 0, (UINT8 *) "  WinX=%lu, WinY=%lu, visible=%d   \n", psWindow->psWindowImageInstance->u32XPos, psWindow->psWindowImageInstance->u32YPos, psWindow->bVisible);

		if (psWindow->bVisible)
		{
			// Only allow it to do something if it's visible.

			// First check if the mouse is actually in the window's viewport
			if (u32XPointerPos < psWindow->psWindowImageInstance->u32XPos)
			{
				// Too far to the left
			}
			else
			if (u32XPointerPos >= (psWindow->psWindowImageInstance->u32XPos + psWindow->u32ViewportXSize))
			{
				// Too far to the right
			}
			else
			if (u32YPointerPos < psWindow->psWindowImageInstance->u32YPos)
			{
				// Too far above
			}
			else
			if (u32YPointerPos >= (psWindow->psWindowImageInstance->u32YPos + psWindow->u32ViewportYSize))
			{
				// Too far below
			}
			else
			{

				// Offset the mouset pointer into the window based on the viewport offset
				u32XViewportPointerPos = u32XPointerPos + psWindow->u32ViewportXOffset;
				u32YViewportPointerPos = u32YPointerPos + psWindow->u32ViewportYOffset;

				if (u32XViewportPointerPos < (psWindow->psWindowImageInstance->u32XPos + psWindow->u32ActiveAreaXPos))
				{
					// Too far to the left
				}
				else
				if (u32XViewportPointerPos >= (psWindow->psWindowImageInstance->u32XPos + psWindow->u32ActiveAreaXPos + psWindow->u32ActiveAreaXSize))
				{
					// Too far to the right
				}
				else
				if (u32YViewportPointerPos < (psWindow->psWindowImageInstance->u32YPos + psWindow->u32ActiveAreaYPos))
				{
					// Too far above
				}
				else
				if (u32YViewportPointerPos >= (psWindow->psWindowImageInstance->u32YPos + psWindow->u32ActiveAreaYPos + psWindow->u32ActiveAreaYSize))
				{
					// Too far below
				}
				else
				{
					UINT32 u32XNormal = u32XViewportPointerPos - (psWindow->psWindowImageInstance->u32XPos + psWindow->u32ActiveAreaXPos);
					UINT32 u32YNormal = u32YViewportPointerPos - (psWindow->psWindowImageInstance->u32YPos + psWindow->u32ActiveAreaYPos);

					// u32X/YNormal have the normalized X/Y position as it pertains to the window

					if (EMOUSE_POSITION_CHANGED == eAction)
					{
						eErr = WindowListUnlock();
						GCASSERT(LERR_OK == eErr);

						WidgetUpdatePointer(psWindow->psWidgetHead,
											u32ButtonMask,
											u32XViewportPointerPos,
											u32YViewportPointerPos,
											u32XNormal,
											u32YNormal);

						eErr = WindowListLock();
						GCASSERT(LERR_OK == eErr);

						goto out;
					}
					else
					if (EMOUSE_BUTTON_PRESSED == eAction)
					{
						eErr = WindowListUnlock();
						GCASSERT(LERR_OK == eErr);

						WidgetButtonPressed(psWindow->psWidgetHead,
											u32ButtonMask,
											u32XViewportPointerPos,
											u32YViewportPointerPos,
											u32XNormal,
											u32YNormal);

						eErr = WindowListLock();
						GCASSERT(LERR_OK == eErr);

						// Causes windows below this window to not receive button press messages
						goto out;
					}
					else
					if (EMOUSE_BUTTON_RELEASED == eAction)
					{
						eErr = WindowListUnlock();
						GCASSERT(LERR_OK == eErr);

						WidgetButtonReleased(psWindow->psWidgetHead,
											 u32ButtonMask,
											 u32XViewportPointerPos,
											 u32YViewportPointerPos,
											 u32XNormal,
											 u32YNormal);

						eErr = WindowListLock();
						GCASSERT(LERR_OK == eErr);

						goto out;
					}
					else
					{
						GCASSERT(0);
					}
				}
			}
		}

nextWindow:
		psWindow = psWindow->psNextWindow;
	}

out:
	eErr = WindowListUnlock();
	GCASSERT(LERR_OK == eErr);
}

void WindowButtonPressed(UINT32 u32ButtonMask,
						 UINT32 u32XPointerPos,
						 UINT32 u32YPointerPos)
{
	WindowAction(u32ButtonMask,
				 u32XPointerPos,
				 u32YPointerPos,
				 EMOUSE_BUTTON_PRESSED);
}

void WindowButtonReleased(UINT32 u32ButtonMask,
						  UINT32 u32XPointerPos,
						  UINT32 u32YPointerPos)
{
	WindowAction(u32ButtonMask,
				 u32XPointerPos,
				 u32YPointerPos,
				 EMOUSE_BUTTON_RELEASED);
}

void WindowPointerUpdate(UINT32 u32ButtonMask,
						 UINT32 u32XPointerPos,
						 UINT32 u32YPointerPos)
{
	WindowAction(u32ButtonMask,
				 u32XPointerPos,
				 u32YPointerPos,
				 EMOUSE_POSITION_CHANGED);
}

ELCDErr WindowDelete(WINDOWHANDLE eWINDOWHANDLE)
{
	SWindow *psWindow;
	SWindow *psWindowPrior;
	SWindow *psWindowPtr;
	SWidget *psWidget = NULL;
	SLayer *psLayer;
	ELCDErr eErr = LERR_OK;
	EGCResultCode eResult;

	// Make sure we have a good handle
	psWindow = WindowGetPointer(eWINDOWHANDLE);
	if (NULL == psWindow)
	{
		return(LERR_WIN_BAD_HANDLE);
	}

//	DebugOut("%s: Deleting window 0x%.8x (layer=0x%.8x)\n", __FUNCTION__, psWindow, psWindow->psWindowImageInstance->psParentLayer);
	
	// Defocus the current widget.  Most likely, it's being destroyed
	WidgetDefocusAll();

	psWidget = psWindow->psWidgetHead;

	// Ditch all the widgets
	while (psWidget)
	{
		WIDGETHANDLE eHandle;

		psWidget = psWindow->psWidgetHead;
		while (psWidget && (HANDLE_INVALID == psWidget->eParentWidget))
		{
			eHandle = psWidget->eWidgetHandle;

			eErr = WidgetDestroyByHandle(&eHandle,
										 WidgetGetHandleType(eHandle));
			GCASSERT(LERR_OK == eErr);

			psWidget = psWidget->psNextLink;
		}

		if (psWidget)
		{
			eHandle = psWidget->eWidgetHandle;

			// If this asserts, it means there's nothing but subordinate widget left
			// and that a widget is not cleaning up after itself.
			GCASSERT(psWidget);

			eErr = WidgetDestroyByHandle(&eHandle,
										 WidgetGetHandleType(eHandle));
			GCASSERT(LERR_OK == eErr);

			psWidget = psWindow->psWidgetHead;
		}
	}

	eErr = WindowLock(psWindow);
	if (eErr != LERR_OK)
	{
		goto errorExit;
	}

	// Now get it out of the list of all Windows

	eErr = WindowListLock();
	GCASSERT(LERR_OK == eErr);

	sg_u32TotalWindows--;

	psWindowPtr = sg_psWindowHead;
	psWindowPrior = NULL;

	while (psWindowPtr)
	{
		if (psWindowPtr == psWindow)
		{
			break;
		}

		psWindowPrior = psWindowPtr;
		psWindowPtr = psWindowPtr->psNextWindow;
	}

	sg_psWindowList[eWINDOWHANDLE] = NULL;

	if (NULL == psWindowPrior)
	{
		sg_psWindowHead = psWindowPtr->psNextWindow;
	}
	else
	{
		psWindowPrior->psNextWindow = psWindowPtr->psNextWindow;
	}

	// Delete the background image reference if there is one
	if (psWindowPtr->psBackgroundImage)
	{
		GfxDeleteImageGroup(psWindowPtr->psBackgroundImage);
	}

	eErr = WindowListUnlock();
	GCASSERT(LERR_OK == eErr);

	// Erase the window
	GfxSetImageInstance(psWindow->psWindowImageInstance,
						NO_CHANGE,
						NO_CHANGE,
						FALSE);

	psLayer = psWindow->psWindowImageInstance->psParentLayer;
	GfxDeleteLayer(psLayer);
	GfxDeleteImage(psWindow->psWindowImage);

	// Get rid of access semaphores
	eResult = GCOSSemaphoreDelete(psWindow->sDirtyRegionSem,
								  DELETEOP_ALWAYS);
	if (eResult != GC_OK)
	{
		eErr = (ELCDErr) (LERR_GC_ERR_BASE + eResult);
		goto errorExit;
	}

	// Now the update semaphore
	eResult = GCOSSemaphoreDelete(psWindow->sWindowUpdateSem,
								  DELETEOP_ALWAYS);
	eErr = (ELCDErr) (LERR_GC_ERR_BASE + eResult);

	eResult = GCOSQueueSend(sg_sWindowQueue,
							(void *) (WCMD_WINDOW_FORCE_BLIT));
	if (eResult != GC_OK)
	{
		eErr = (ELCDErr) (LERR_GC_ERR_BASE + eResult);
		goto errorExit;
	}

errorExit:
	GCFreeMemory(psWindow);
	return(eErr);
}

void WindowAnimationTick(UINT32 u32TickTime)
{
	SWindowAnimation *psAnim = NULL;
	SWindow *psWindow = NULL;

	psAnim = sg_psWindowAnimations;

	// Run through the list of all animations for all windows and see if we send
	// an animation tick down

	while (psAnim)
	{
		psWindow = WindowGetPointer(psAnim->eWindowHandle);
		if ((psWindow) && (psWindow->bVisible) && (psAnim->psWidget))
		{
			// Only pay attention to it if it's a valid window handle and it's visible
			if ((psAnim->psWidget->bWidgetEnabled) &&
				(FALSE == psAnim->psWidget->bWidgetHidden))
			{
				// Time to send a tick to the widget if it supports animation
				if (psAnim->psWidget->psWidgetFunc->WidgetAnimationTick)
				{
					psAnim->psWidget->psWidgetFunc->WidgetAnimationTick(psAnim->psWidget,
																		u32TickTime);
				}
			}
		}

		psAnim = psAnim->psNextLink;
	}
}

ELCDErr WindowAnimationListAdd(WINDOWHANDLE eWindowHandle,
							   SWidget *psWidget)
{
	SWindowAnimation *psNode = NULL;

	if (NULL == WindowGetPointer(eWindowHandle))
	{
		return(LERR_WIN_BAD_HANDLE);
	}

	// Let's see if it's already there. If so, just return
	psNode = sg_psWindowAnimations;
	while (psNode)
	{
		if ((psNode->eWindowHandle == eWindowHandle) &&
			(psNode->psWidget == psWidget))
		{
			psNode->u32References++;
			return(LERR_OK);
		}

		psNode = psNode->psNextLink;
	}

	psNode = MemAlloc(sizeof(*psNode));
	if (NULL == psNode)
	{
		return(LERR_NO_MEM);
	}

	psNode->eWindowHandle = eWindowHandle;
	psNode->psWidget = psWidget;
	psNode->psNextLink = sg_psWindowAnimations;
	psNode->u32References = 1;
	sg_psWindowAnimations = psNode;
	return(LERR_OK);
}

void WindowAnimationListDelete(WINDOWHANDLE eWindowHandle,
							   SWidget *psWidget)
{
	SWindowAnimation *psNode = NULL;
	SWindowAnimation *psPriorNode = NULL;

	psNode = sg_psWindowAnimations;

	while (psNode)
	{
		if ((psNode->eWindowHandle == eWindowHandle) &&
			(psNode->psWidget == psWidget))
		{
			GCASSERT(psNode->u32References);
			--psNode->u32References;
			if (psNode->u32References)
			{
				// Just return if we still have references left
				return;
			}

			// Otherwise, break out and delete the node
			break;
		}

		psPriorNode = psNode;
		psNode = psNode->psNextLink;
	}

	if (NULL == psNode)
	{
		// Not in list
		return;
	}

	// Take it out of the list
	if (NULL == psPriorNode)
	{
		sg_psWindowAnimations = psNode->psNextLink;
	}
	else
	{
		psPriorNode->psNextLink = psNode->psNextLink;
	}

	GCFreeMemory(psNode);
}

ELCDErr WindowPriorityGet(WINDOWHANDLE eWindowHandle,
						  UINT32 *pu32Priority)
{
	UINT32 u32Priority;
	SWindow *psWindow = NULL;

	// Make sure we have a good handle
	psWindow = WindowGetPointer(eWindowHandle);
	if (NULL == psWindow)
	{
		return(LERR_WIN_BAD_HANDLE);
	}

	u32Priority = GfxGetLayerPriorityByImageInstance(psWindow->psWindowImageInstance);
	if (0xffffffff == u32Priority)
	{
		return(LERR_WIN_NO_PRIORITY);
	}
	else
	{
		*pu32Priority = u32Priority;
		return(LERR_OK);
	}
}

static UINT32 WindowGetPriority(SWindow *psWindow)
{
	SWindow *psWindowPtr;
	UINT32 u32Priority = 0;

	psWindowPtr = sg_psWindowHead;

	while (psWindow != psWindowPtr)
	{
		psWindowPtr = psWindowPtr->psNextWindow;
		++u32Priority;
	}
	
	GCASSERT(psWindow);
	return(u32Priority);
}

static SWindow *WindowGetWindowByPriority(UINT32 u32Priority)
{
	ELCDErr eErr;
	SWindow *psWindowPtr = NULL;
	
	eErr = WindowListLock();
	GCASSERT(LERR_OK == eErr);

	psWindowPtr = sg_psWindowHead;

	while (psWindowPtr && u32Priority)
	{
		// If it's about ready to be NULL and our priority is nonzero, just return the last priority as a link point
		if (NULL == psWindowPtr->psNextWindow)
		{
			break;
		}

		psWindowPtr = psWindowPtr->psNextWindow;
		--u32Priority;
	}

	eErr = WindowListUnlock();
	GCASSERT(LERR_OK == eErr);

	return(psWindowPtr);
}

static void WindowDumpOrder(void)
{
	SWindow *psWindow = sg_psWindowHead;
	UINT32 u32Priority = 0;

//	DebugOut("%s: Dumping window order\n", __FUNCTION__);

	while (psWindow)
	{
//		DebugOut("%s:   %u: 0x%.8x (Layer=0x%.8x)\n", __FUNCTION__, u32Priority, psWindow, psWindow->psWindowImageInstance->psParentLayer);
		psWindow = psWindow->psNextWindow;
		++u32Priority;
	}
}

static void WindowSetPriority(SWindow *psWindowToMove,
							  UINT32 u32NewPosition,
							  BOOL bFront)
{
	UINT32 u32WindowToMovePriority = 0;
	UINT32 u32WindowDestinationPriority = 0;
	SWindow *psLinkPoint = NULL;
	SWindow *psWinPrior = NULL;
	SWindow *psWin = NULL;

//	DebugOut("%s: Moving window @ 0x%.8x\n", __FUNCTION__, psWindowToMove);

	// Figure out what our window to move priority is
	u32WindowToMovePriority = WindowGetPriority(psWindowToMove);

	WindowDumpOrder();

	if (u32NewPosition == u32WindowToMovePriority)
	{
		// Don't do anything. Same spot as before!
		return;
	}

	if (bFront)
	{
		if (u32NewPosition)
		{
			psLinkPoint = WindowGetWindowByPriority(u32NewPosition - 1);
		}
		else
		{
			psLinkPoint = NULL;
		}
	}
	else
	{
		GCASSERT(0);
	}

	if (psWindowToMove == psLinkPoint)
	{
		// Not moving
		return;
	}

	if (0 == u32NewPosition)
	{
		psLinkPoint = NULL;
	}

	// Unlink our window to move
	psWin = sg_psWindowHead;

	while (psWin != psWindowToMove)
	{
		psWinPrior = psWin;
		psWin = psWin->psNextWindow;
	}

	GCASSERT(psWin == psWindowToMove);

	if (NULL == psWinPrior)
	{
		GCASSERT(psWindowToMove == sg_psWindowHead);
		sg_psWindowHead = sg_psWindowHead->psNextWindow;
	}
	else
	{
		psWinPrior->psNextWindow = psWin->psNextWindow;
	}

	// All unlinked now

	// psLinkPoint now points to the structure that this new window is linked *AFTER*.

	if (NULL == psLinkPoint)
	{
		// At the head of the list
		psWindowToMove->psNextWindow = sg_psWindowHead;
		sg_psWindowHead = psWindowToMove;
	}
	else
	{
		psWindowToMove->psNextWindow = psLinkPoint->psNextWindow;
		psLinkPoint->psNextWindow = psWindowToMove;
	}

//	DebugOut("%s: After move\n", __FUNCTION__);
	WindowDumpOrder();

}

ELCDErr WindowPrioritySet(WINDOWHANDLE eWindowHandle,
						  UINT32 u32Priority,
						  BOOL bFront)
{
	BOOL bSucceeded = FALSE;
	SWindow *psWindow = NULL;
	SLayer *psLayer = NULL;
	ELCDErr eErr = LERR_OK;
	BOOL bSuccessful = FALSE;
	EGCResultCode eResult;
	UINT32 u32OldPriority = 0;

	// Make sure we have a good handle
	psWindow = WindowGetPointer(eWindowHandle);
	if (NULL == psWindow)
	{
		return(LERR_WIN_BAD_HANDLE);
	}

	u32OldPriority = GfxGetLayerPriority(psWindow->psWindowImageInstance->psParentLayer);

	if (u32OldPriority < u32Priority)
	{
		u32Priority++;
	}

	psLayer = GfxGetLayerPointerByPriority(u32Priority);

	// Keep anything from blitting
	WindowBlitLock(TRUE);

	// Keep anything from monkeying with the window list
	eErr = WindowListLock();
	GCASSERT(LERR_OK == eErr);

	// Go do the window update (graphical part)
	GfxSetLayerPriority(psWindow->psWindowImageInstance->psParentLayer,
						psLayer,
						bFront);

	// Now relink it in the correct order
	WindowSetPriority(psWindow,
					  u32Priority,
					  bFront);

	eErr = WindowListUnlock();
	GCASSERT(LERR_OK == eErr);

	WindowBlitLock(FALSE);

	eResult = GCOSQueueSend(sg_sWindowQueue,
							(void *) (WCMD_WINDOW_FORCE_BLIT));
	if (eResult != GC_OK)
	{
		eErr = (ELCDErr) (LERR_GC_ERR_BASE + eResult);
	}

	return(eErr);
}

ELCDErr WindowSetShadowUI(WINDOWHANDLE eWindowHandle,
						  EShadowStyle eStyle,
						  EShadowCornerOrigin eOrigin,
						  UINT32 u32Thickness,
						  UINT16 u16Color)
{
	SWindow *psWindow = NULL;
	ELCDErr eErr = LERR_OK;
	UINT32 u32HLength;
	UINT32 u32VLength;

	// Make sure we have a good handle
	psWindow = WindowGetPointer(eWindowHandle);
	if (NULL == psWindow)
	{
		return(LERR_WIN_BAD_HANDLE);
	}

	// Keep anything from blitting
	WindowBlitLock(TRUE);

	// Keep anything from monkeying with the window list
	eErr = WindowListLock();
	GCASSERT(LERR_OK == eErr);

	// Calc some of the interesting things
	u32HLength = psWindow->u32ActiveAreaXSize;
	u32VLength = psWindow->u32ActiveAreaYSize;

	if (SORG_CORNER_UPPER_LEFT == eOrigin)
	{
		// Top side
		eErr = WindowSetShadow(eWindowHandle,
							   eStyle,
							   SORG_TOP,
							   u32Thickness,
							   -((INT32) u32Thickness),
							   psWindow->u32ActiveAreaXSize,
							   u16Color);

		ERROREXIT_ON_FAIL(eErr);

		// Left side
		eErr = WindowSetShadow(eWindowHandle,
							   eStyle,
							   SORG_LEFT,
							   u32Thickness,
							   -((INT32) u32Thickness),
							   psWindow->u32ActiveAreaYSize,
							   u16Color);

		ERROREXIT_ON_FAIL(eErr);
	}
	else
	if (SORG_CORNER_UPPER_RIGHT == eOrigin)
	{
		// Top side
		eErr = WindowSetShadow(eWindowHandle,
							   eStyle,
							   SORG_TOP,
							   u32Thickness,
							   u32Thickness,
							   psWindow->u32ActiveAreaXSize,
							   u16Color);

		ERROREXIT_ON_FAIL(eErr);

		// Right side
		eErr = WindowSetShadow(eWindowHandle,
							   eStyle,
							   SORG_RIGHT,
							   u32Thickness,
							   -((INT32) u32Thickness),
							   psWindow->u32ActiveAreaYSize,
							   u16Color);

		ERROREXIT_ON_FAIL(eErr);
	}
	else
	if (SORG_CORNER_LOWER_LEFT == eOrigin)
	{
		// Bottom side
		eErr = WindowSetShadow(eWindowHandle,
							   eStyle,
							   SORG_BOTTOM,
							   u32Thickness,
							   -((INT32) u32Thickness),
							   psWindow->u32ActiveAreaXSize,
							   u16Color);

		ERROREXIT_ON_FAIL(eErr);

		// Left side
		eErr = WindowSetShadow(eWindowHandle,
							   eStyle,
							   SORG_LEFT,
							   u32Thickness,
							   u32Thickness,
							   psWindow->u32ActiveAreaYSize,
							   u16Color);

		ERROREXIT_ON_FAIL(eErr);
	}
	else
	if (SORG_CORNER_LOWER_RIGHT == eOrigin)
	{
		// Bottom side
		eErr = WindowSetShadow(eWindowHandle,
							   eStyle,
							   SORG_BOTTOM,
							   u32Thickness,
							   u32Thickness,
							   psWindow->u32ActiveAreaXSize,
							   u16Color);

		ERROREXIT_ON_FAIL(eErr);

		// Left side
		eErr = WindowSetShadow(eWindowHandle,
							   eStyle,
							   SORG_RIGHT,
							   u32Thickness,
							   u32Thickness,
							   psWindow->u32ActiveAreaYSize,
							   u16Color);

		ERROREXIT_ON_FAIL(eErr);
	}
	else
	if (SORG_CORNER_ALL == eOrigin)
	{
		eErr = WindowSetShadow(eWindowHandle,
							   eStyle,
							   SORG_TOP,
							   u32Thickness,
							   0,
							   psWindow->u32ActiveAreaXSize,
							   u16Color);

		ERROREXIT_ON_FAIL(eErr);

		// Left side
		eErr = WindowSetShadow(eWindowHandle,
							   eStyle,
							   SORG_LEFT,
							   u32Thickness,
							   0,
							   psWindow->u32ActiveAreaYSize,
							   u16Color);

		ERROREXIT_ON_FAIL(eErr);

		// Bottom side
		eErr = WindowSetShadow(eWindowHandle,
							   eStyle,
							   SORG_BOTTOM,
							   u32Thickness,
							   0,
							   psWindow->u32ActiveAreaXSize,
							   u16Color);

		ERROREXIT_ON_FAIL(eErr);

		// Right side
		eErr = WindowSetShadow(eWindowHandle,
							   eStyle,
							   SORG_RIGHT,
							   u32Thickness,
							   0,
							   psWindow->u32ActiveAreaYSize,
							   u16Color);

		ERROREXIT_ON_FAIL(eErr);

	}
	else
	{
		// Not compatible with the book of Origin
		eErr = LERR_WIN_INVALID_ORIGIN;
	}

errorExit:
	// Unlock everything
	if (eErr != LERR_OK)
	{
		eErr = WindowListUnlock();
	}
	else
	{
		(void) WindowListUnlock();
	}

	WindowBlitLock(FALSE);


	return(eErr);
}

ELCDErr WindowColorFillRegion(WINDOWHANDLE eWindowHandle,
							  UINT16 u16Color,
							  INT32 s32XPos,
							  INT32 s32YPos,
							  INT32 s32XSize,
							  INT32 s32YSize,
							  BOOL bLockWindow)
{
	SWindow *psWindow = NULL;
	ELCDErr eErr = LERR_OK;

	// Make sure we have a good handle
	psWindow = WindowGetPointer(eWindowHandle);
	if (NULL == psWindow)
	{
		return(LERR_WIN_BAD_HANDLE);
	}

	if (bLockWindow)
	{
		// Keep anything from blitting
		WindowBlitLock(TRUE);

		// Keep anything from monkeying with the window list
		eErr = WindowListLock();
		GCASSERT(LERR_OK == eErr);
	}

	// Adjust the x position
	s32XPos += (INT32) psWindow->u32ActiveAreaXPos;
	s32YPos += (INT32) psWindow->u32ActiveAreaYPos;

	// Now clip the size
	if ((s32XPos + s32XSize) > (((INT32) psWindow->u32ActiveAreaXPos) + (INT32) psWindow->u32ActiveAreaXSize))
	{
		s32XSize = (psWindow->u32ActiveAreaXSize - s32XPos);
	}

	if ((s32YPos + s32YSize) > (((INT32) psWindow->u32ActiveAreaXPos) + (INT32) psWindow->u32ActiveAreaYSize))
	{
		s32YSize = (psWindow->u32ActiveAreaYSize - s32YPos);
	}

	if ((s32XSize > 0) && (s32YSize > 0))
	{
		UINT16 *pu16DataPtr;
		INT32 s32YSizeOriginal = s32YSize;

		// In range - time for a fill!
		pu16DataPtr = psWindow->psWindowImage->pu16ImageData + (s32XPos + (s32YPos * psWindow->psWindowImage->u32Pitch));

		while (s32YSize--)
		{
			UINT32 u32Loop;

			u32Loop = (UINT32) s32XSize;
			while (u32Loop--)
			{
				*pu16DataPtr = u16Color;
				++pu16DataPtr;
			}

			pu16DataPtr = (pu16DataPtr - s32XSize) + psWindow->psWindowImage->u32Pitch;
		}

		WindowUpdateRegion(eWindowHandle,
						   s32XPos,
						   s32YPos,
						   s32XSize,
						   s32YSizeOriginal);
	}

	if (bLockWindow)
	{
		eErr = WindowListUnlock();
		WindowBlitLock(FALSE);
	}

	return(eErr);
}

ELCDErr WindowShutdown(void)
{
	ELCDErr eErr;
	UINT32 u32Loop = 0;

	// Loop through all possible window slots and deallocate the windows and the widgets
	for (u32Loop = 0; u32Loop < MAX_WINDOWS; u32Loop++)
	{
		WINDOWHANDLE eWindowHandle;

		// This had better not fail
		eErr = WindowListLock();
		GCASSERT(LERR_OK == eErr);

		if (sg_psWindowList[u32Loop])
		{
			eWindowHandle = (WINDOWHANDLE) u32Loop;
		}
		else
		{
			eWindowHandle = (WINDOWHANDLE) HANDLE_INVALID;
		}

		// Now unlock the list
		eErr = WindowListUnlock();
		GCASSERT(LERR_OK == eErr);

		if (HANDLE_INVALID == eWindowHandle)
		{
			// Skip it
		}
		else
		{
			eErr = WindowDelete(eWindowHandle);
			GCASSERT(LERR_OK == eErr);
		}
	}

	// Post a message to the window thread
	eErr = GCOSQueueSend(sg_sWindowQueue,
						 (void *) (WCMD_WINDOW_SHUTDOWN));
	return(eErr);
}

ELCDErr WindowGetPixel(WINDOWHANDLE eWindowHandle,
					   INT32 s32XPos,
					   INT32 s32YPos,
					   UINT16 *pu16Pixel)
{
	SWindow *psWindow = NULL;
	ELCDErr eErr = LERR_OK;

	// Make sure we have a good handle
	psWindow = WindowGetPointer(eWindowHandle);
	if (NULL == psWindow)
	{
		return(LERR_WIN_BAD_HANDLE);
	}

	if ((s32XPos >= psWindow->psWindowImageInstance->s32XPos) ||
		(s32YPos >= psWindow->psWindowImageInstance->s32YPos))
	{
		return(LERR_OUT_OF_RANGE);
	}

	// Go get the pixel
	*pu16Pixel = psWindow->psWindowImage->pu16ImageData[s32XPos + 
														(s32YPos * psWindow->psWindowImageInstance->psImage->u32Pitch)];

	return(LERR_OK);
}
