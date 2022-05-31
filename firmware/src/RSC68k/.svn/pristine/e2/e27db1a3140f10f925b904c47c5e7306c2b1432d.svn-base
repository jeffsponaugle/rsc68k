#include <math.h>
#include "Startup/app.h"
#include "Application/RSC68k.h"
#include "Libs/Gfx/GraphicsLib.h"
#include "Libs/window/window.h"
#include "Libs/widget/widget.h"
#include "Libs/widget/elements/elements.h"
#include "Win32/host.h"

// Default widget name
#define	DEFAULT_WIDGET_NAME	"UNNAMED"

// Pointer state info
static UINT32 sg_u32PointerXPos;
static UINT32 sg_u32PointerYPos;
static UINT32 sg_u32Buttons;
static UINT32 sg_u32SinACos[1024];
static BOOL sg_bWidgetShutdown;

// Timer object for pointer devices
static STimerObject *sg_psWidgetTimer;

#define WIDGET_QUEUE_SIZE					1024
#define	WIDGET_THREAD_STACK_SIZE			16384
#define WIDGET_POINTER_THREAD_STACK_SIZE	2048

static SWidgetTypeMethods *sg_psWidgetTypeMethods[WIDGET_MAX];

// Widget thread/stack
static UINT8 sg_u8WidgetThreadStack[WIDGET_THREAD_STACK_SIZE];
static UINT8 sg_u8WidgetPointerThreadStack[WIDGET_POINTER_THREAD_STACK_SIZE];
static SOSQueue sg_sWidgetQueue;

// Current widget that's in focus
static SWidget *sg_psWidgetInFocus = NULL;

// This is a list of all widgets that don't have associated windows
static SWidget *sg_psWidgetsWithoutWindows = NULL;

static EGCResultCode WidgetSendCmd(UINT32 u32Command)
{
	EGCResultCode eResult;

	eResult = GCOSQueueSend(sg_sWidgetQueue,
							(void *) (u32Command));
	return(eResult);
}

void WidgetRemoveFromWidgetsWithoutWindowsList(SWidget *psWidget)
{
	SWidget *psWidgetPtr = NULL;
	SWidget *psWidgetPriorPtr = NULL;

	psWidgetPtr = sg_psWidgetsWithoutWindows;

	while (psWidgetPtr)
	{
		if (psWidgetPtr == psWidget)
		{
			break;
		}

		psWidgetPriorPtr = psWidgetPtr;
		psWidgetPtr = psWidgetPtr->psNextLink;
	}

	// If this is NULL, the widget isn't in the list
	if (NULL == psWidgetPtr)
	{
		return;
	}

	if (psWidgetPriorPtr)
	{
		// Not the first item in the list
		GCASSERT(psWidgetPriorPtr->psNextLink == psWidget);
		// This means it's not the first of the list
		psWidgetPriorPtr->psNextLink = psWidget->psNextLink;
	}
	else
	{
		// First item in the list
		GCASSERT(sg_psWidgetsWithoutWindows == psWidget);

		// Unlink it from the master list
		sg_psWidgetsWithoutWindows = sg_psWidgetsWithoutWindows->psNextLink;
	}

	// Now NULL out the "next" list so it doesn't get linked to other widgets on accident
	psWidget->psNextLink = NULL;
}

void WidgetRegisterTypeMethods(EWidgetType eType,
							   SWidgetTypeMethods *psTypeMethod)
{
	GCASSERT((eType < WIDGET_MAX) && (eType != WIDGET_WTF));
	GCASSERT(NULL == sg_psWidgetTypeMethods[eType]);

	sg_psWidgetTypeMethods[eType] = psTypeMethod;
}

ELCDErr WidgetAllocate(SWidget **ppsWidget)
{
	SWidget *psWidget;

	psWidget = MemAlloc(sizeof(*psWidget));
	if (NULL == psWidget)
	{
		return(LERR_NO_MEM);
	}

	*ppsWidget = psWidget;
	psWidget->eWidgetType = WIDGET_IN_PROCESS;
	psWidget->pu8WidgetName = DEFAULT_WIDGET_NAME;

	return(LERR_OK);
}

ELCDErr WidgetDelete(SWidget *psWidgetToDelete)
{
	SWidget *psWidget = NULL;
	SWindow *psWindow = NULL;
	SWidgetIntersection *psIntersection;
	ELCDErr eErr = LERR_OK;
	SWidgetCallbacks *psCallbacks;

	//  Record the callback list for later
	psCallbacks = psWidgetToDelete->psCallbacks;
	psWidgetToDelete->psCallbacks = NULL;

	// Deallocate all of the callbacks
	while (psCallbacks)
	{
		SWidgetCallbacks *psCallbackTmp;

		psCallbackTmp = psCallbacks;
		psCallbacks = psCallbacks->psNextLink;

		// Free me!
		GCFreeMemory(psCallbackTmp);
	}

	psWindow = WindowGetPointer(psWidgetToDelete->eParentWindow);

	if ((psWindow) && 
		(psWidgetToDelete->eParentWindow != HANDLE_INVALID))
	{
		eErr = WindowLock(psWindow);
		if (eErr != LERR_OK)
		{
			goto errorExit;
		}

		// Go hide the widget
		if (psWidgetToDelete->eWidgetType != WIDGET_IN_PROCESS)
		{
			(void) WindowUnlock(psWindow);

			WidgetSetHide(psWidgetToDelete,
						  TRUE,
						  TRUE);
		}

		if (psWidgetToDelete->eWidgetType != WIDGET_IN_PROCESS)
		{
			eErr = WindowWidgetDisconnect(psWidgetToDelete->eParentWindow,
										  psWidgetToDelete);
			GCASSERT((LERR_OK == eErr) ||
					 (LERR_WIN_WIDGET_NOT_CONNECTED));
		}

		psWidget = psWidgetToDelete;

		// Run through the entire list of widget intersections and remove
		// this widget from their lists

		psIntersection = psWidget->psIntersectionList;

		while (psIntersection)
		{
			SWidgetIntersection *psInt = NULL;
			SWidgetIntersection *psIntPrior = NULL;

			psInt = psIntersection->psWidget->psIntersectionList;
			while (psInt)
			{
				if (psInt->psWidget == psWidgetToDelete)
				{
					// Time to remove this intersection list
					if (NULL == psIntPrior)
					{
						psIntersection->psWidget->psIntersectionList = psInt->psNextLink;
						GCFreeMemory(psInt);
						psInt = psIntersection->psWidget->psIntersectionList;
					}
					else
					{
						psIntPrior->psNextLink = psInt->psNextLink;
						GCFreeMemory(psInt);
						psInt = psIntPrior->psNextLink;
					}
				}
				else
				{
					psIntPrior = psInt;
					psInt = psInt->psNextLink;
				}
			}

			psIntersection = psIntersection->psNextLink;
		}

		// We've deleted this widget from all other widgets that intersect with it.
		// Now we need to erase and redraw all of the interconnected widgets.

		// Wipe out the intersections
		WidgetEraseIntersections(psWidget);

		// Paint any intersecting widgets
		WidgetPaintIntersections(psWidget);
	}
	else
	{
		SWidget *psWidgetPrior = NULL;
		SWidget *psWidgetPtr = NULL;

		// Find the widget in the master widget list for widgets without homes

		psWidgetPtr = sg_psWidgetsWithoutWindows;

		while (psWidgetPtr && psWidgetPtr != psWidgetToDelete)
		{
			psWidgetPrior = psWidgetPtr;
			psWidgetPtr = psWidgetPtr->psNextLink;
		}

		// If this asserts, the widget list is screwed up, or it was never allocated
		// in the first place
		if (psWidgetPtr)
		{
			if (NULL == psWidgetPrior)
			{
				sg_psWidgetsWithoutWindows = psWidgetPtr->psNextLink;
			}
			else
			{
				psWidgetPrior->psNextLink = psWidgetPtr->psNextLink;
			}
		}
		else
		{
			// This is if the widget 
			psWidgetPtr = psWidgetToDelete;
		}

		GCASSERT(psWidgetPtr == psWidgetToDelete);
		psWidget = psWidgetToDelete;
	}

	// Now delete the widget
	psWidget->bWidgetEnabled = FALSE;
	psWidget->bWidgetHidden = TRUE;

	// If there's a function to free this widget's widget specific data,
	// go do it now
	if (sg_psWidgetTypeMethods[psWidget->eWidgetType]->WidgetTypeFree)
	{
		(void) sg_psWidgetTypeMethods[psWidget->eWidgetType]->WidgetTypeFree(psWidget);
	}

	GCFreeMemory(psWidget);

	if (psWindow)
	{
		if (LERR_OK == eErr)
		{
			(void) WindowUnlock(psWindow);
		}
		else
		{
			eErr = WindowUnlock(psWindow);
		}
	}

errorExit:
	return(eErr);
}

ELCDErr WidgetCalcIntersections(SWidget *psWidget)
{
	SWidgetIntersection *psIntersection;
	SWidgetIntersection *psIntersectionPrior;
	SWindow *psWindow;
	SWidget *psWidgetPtr;
	INT32 s32Widget1XRight;
	INT32 s32Widget1YBottom;
	ELCDErr eErr = LERR_OK;

	// If the widget doesn't have a window handle or intersections are ignored
	// for this widget, just return
	if ((HANDLE_INVALID == psWidget->eParentWindow) ||
		(FALSE == psWidget->bIgnoreIntersections))
	{
		return(LERR_OK);
	}

	// First, wipe out any insersection structures we have assigned currently
	psIntersection = psWidget->psIntersectionList;

	// All clean
	psWidget->psIntersectionList = NULL;

	while (psIntersection)
	{
		psIntersectionPrior = psIntersection;
		psIntersection = psIntersection->psNextLink;
		GCFreeMemory(psIntersectionPrior);
	}

	// Get our parent window, and walk the list of coordinates to see if anything
	// intersects

	psWindow = WindowGetPointer(psWidget->eParentWindow);
	GCASSERT(psWindow);

	s32Widget1XRight = psWidget->s32XPos + psWidget->u32XSize;
	s32Widget1YBottom = psWidget->s32YPos + psWidget->u32YSize;

	psWidgetPtr = psWindow->psWidgetHead;
	while (psWidgetPtr)
	{
		INT32 s32Widget2XRight;
		INT32 s32Widget2YBottom;

		// Make sure we don't add ourselves to our own list and we don't add
		// other widgets that don't have any redraw power
		if ((psWidgetPtr != psWidget) &&
			(FALSE == psWidgetPtr->bWidgetHidden) &&
			(FALSE == psWidgetPtr->bIgnoreIntersections))
		{
			INT32 s32XPos;
			INT32 s32YPos;

			if (psWidgetPtr->psWidgetFunc->WidgetCalcIntersection)
			{
				UINT32 u32XSize = 0;
				UINT32 u32YSize = 0;

				s32XPos = 0;
				s32YPos = 0;

				psWidgetPtr->psWidgetFunc->WidgetCalcIntersection(psWidgetPtr,
																  &s32XPos,
																  &s32YPos,
																  &u32XSize,
																  &u32YSize);

				s32Widget2XRight = s32XPos + u32XSize;
				s32Widget2YBottom = s32YPos + u32YSize;
			}
			else
			{
				s32Widget2XRight = psWidgetPtr->s32XPos + psWidgetPtr->u32XSize;
				s32Widget2YBottom = psWidgetPtr->s32YPos + psWidgetPtr->u32YSize;
				s32XPos = psWidgetPtr->s32XPos;
				s32YPos = psWidgetPtr->s32YPos;
			}

			if ((s32Widget2XRight <= psWidget->s32XPos) ||
				(s32XPos > s32Widget1XRight))
			{
				// No X intersection 
			}
			else
			if ((s32Widget2YBottom <= psWidget->s32YPos) ||
				(s32YPos > s32Widget1YBottom))
			{
				// No Y intersection
			}
			else
			{
				SWidgetIntersection *psIntersection;

				// We've got an intersection
				psIntersection = MemAlloc(sizeof(*psIntersection));
				if (NULL == psIntersection)
				{
					return(LERR_NO_MEM);
				}

				// Add the psWidgetPtr to our intersection list
				psIntersection->psWidget = psWidgetPtr;
				psIntersection->psNextLink = psWidget->psIntersectionList;
				psWidget->psIntersectionList = psIntersection;

				// Now, look at the intersection list of the other widget. If it's not there,
				// add ourselves to its intersection list
				psIntersection = psWidgetPtr->psIntersectionList;
				while (psIntersection && psIntersection->psWidget != psWidget)
				{
					psIntersection = psIntersection->psNextLink;
				}

				if (NULL == psIntersection)
				{
					// We've got an intersection
					psIntersection = MemAlloc(sizeof(*psIntersection));
					if (NULL == psIntersection)
					{
						return(LERR_NO_MEM);
					}

					// This means it's not in the other widget's list. Add it.
					psIntersection->psWidget = psWidget;
					psIntersection->psNextLink = psWidgetPtr->psIntersectionList;
					psWidgetPtr->psIntersectionList = psIntersection;
				}
			}
		}

		psWidgetPtr = psWidgetPtr->psNextLink;
	}

	return(eErr);
}

// Once every 15ms (roughly 67 times a second)
#define WIDGET_TIMER_INTERVAL		15
#define POINTER_POLL_INTERVAL		15

// Widget command masks
#define	WCMD_WIDGET_MASK		0xfe000000
#define WDATA_WIDGET_MASK		(~WCMD_WIDGET_MASK)

// Messages for widget thread
#define	WCMD_BUTTON_PRESSED		0x02000000
#define	WCMD_BUTTON_REPEAT		0x04000000
#define WCMD_BUTTON_RELEASED	0x06000000
#define WCMD_POINTER_MOVED		0x08000000
#define WCMD_ANIMATION_TICK		0x0a000000
#define WCMD_WIDGET_SHUTDOWN	0x0c000000
#define WCMD_KEYPRESS			0x0e000000
#define	WCMD_WIDGET_MOUSEWHEEL	0x10000000

// Used to detect a key press
#define	WCMD_PRESSED			0x01000000

// This is our animation counter
static UINT32 sg_u32AnimationCounter = 0;
static UINT32 sg_u32AnimationStep = 0;

#define	ANIMATION_INTERVAL		20			// 20 Times per second

static void WidgetPointerReaderThread(void *pvData)
{
	UINT32 u32X;
	UINT32 u32Y;
	UINT32 u32Buttons;

	ThreadSetName("Widget pointer read thread");
	
	DebugOut("%s: Started\n", __FUNCTION__);
	while (1)
	{
		EGCResultCode eResult;

		if (sg_bWidgetShutdown)
		{
			break;
		}

		eResult = GCPointerGetPosition(0,
									   &u32X,
									   &u32Y,
									   &u32Buttons);
		if( GC_OK == eResult )
		{
			eResult = HostConvertScreenPosition(&u32X, &u32Y);
		}

		// Only update if we have a valid read
		if (GC_OK == eResult) 
		{
			UINT32 u32ButtonDelta;

			// If we have a button held and the X/Y position != last time, send a
			// message indicating the X/Y position of the button (12 bits each)

			if ((u32X != sg_u32PointerXPos) ||
				(u32Y != sg_u32PointerYPos))
			{
				eResult = WidgetSendCmd((WCMD_POINTER_MOVED | 
										(u32X & ((1 << 12) - 1)) |
										((u32Y & ((1 << 12) - 1)) << 12)));
				if (eResult != GC_OK)
				{
					// If we got an error, don't update the last known X/Y position so we catch it
					// the next time around
				}
				else
				{
					// Successful deposit of message - update our last pointer position
					sg_u32PointerXPos = u32X;
					sg_u32PointerYPos = u32Y;
				}
			}

			// Good reading. Let's see if anything has changed - try PRESSED first
			u32ButtonDelta = u32Buttons ^ sg_u32Buttons;
			if (u32ButtonDelta)
			{
				UINT32 u32ChangeMask;

				// SOMETHING Has changed. Let's figure out what.

				// Look for anything pressed
				u32ChangeMask = u32ButtonDelta & u32Buttons;
				if (u32ChangeMask)
				{
					eResult = WidgetSendCmd((WCMD_BUTTON_PRESSED | u32ChangeMask));
					if (GC_OK == eResult)
					{
						// If our result is OK, then let's update the global button delta
						sg_u32Buttons &= ~u32ChangeMask;
						sg_u32Buttons |= (u32Buttons & u32ChangeMask);
					}
					else
					{
						// Otherwise, leave it alone so we'll get it next time
					}
				}

				// Look for anything released
				u32ChangeMask = (u32ButtonDelta & ~u32Buttons);
				if (u32ChangeMask)
				{
					eResult = WidgetSendCmd(WCMD_BUTTON_RELEASED | u32ChangeMask);
					if (GC_OK == eResult)
					{
						// If our result is OK, then let's update the global button delta
						sg_u32Buttons &= ~u32ChangeMask;
						sg_u32Buttons |= (u32Buttons & u32ChangeMask);
					}
					else
					{
						// Otherwise, leave it alone so we'll get it next time
					}
				}
			}
		}

		// ~67 times a second
		(void) GCOSSleep(POINTER_POLL_INTERVAL / GCTimerGetPeriod());
	}

	DebugOut("%s: Exited\n", __FUNCTION__);
}

void WidgetKeypress(EGCCtrlKey eGCKey,
					LEX_CHAR eUnicode,
					BOOL bKeyDown)
{
	UINT32 u32Pressed = 0;

	if (bKeyDown)
	{
		u32Pressed = WCMD_PRESSED;
	}

	// Ignoring the result code for the time being
	(void) WidgetSendCmd(WCMD_KEYPRESS | u32Pressed | ((UINT32) eGCKey << 16) | ((UINT16) eUnicode));
}

static void WidgetTimerCallback(UINT32 u32Value)
{
	EGCResultCode eResult;

	sg_u32AnimationCounter += sg_u32AnimationStep;
	if (sg_u32AnimationCounter >= (1 << 16))
	{
		// Now go queue the window for updating
		eResult = WidgetSendCmd(WCMD_ANIMATION_TICK);

		if (eResult != GC_OK)
		{
			// If we got an error, back up the counter
			sg_u32AnimationCounter -= sg_u32AnimationStep;
		}
		else
		{
			sg_u32AnimationCounter -= (1 << 16);
		}
	}
}

void WidgetListReleaseAllWidgets(SWidget *psWidget,
								 UINT32 u32Mask,
								 UINT32 u32XPos,
								 UINT32 u32YPos)
{
	while (psWidget)
	{
		if (psWidget->bCurrentlyPressed)
		{
			UWidgetCallbackData uData;

			// Clear out our widget data
			memset((void *) &uData, 0, sizeof(uData));

			// Record appropriate data
			uData.sPressRelease.u32XPos = u32XPos;
			uData.sPressRelease.u32YPos = u32YPos;
			uData.sPressRelease.u32ButtonMask = u32Mask;

			if (psWidget->psWidgetFunc->WidgetRelease)
			{
				psWidget->psWidgetFunc->WidgetRelease(psWidget,
												      u32Mask,
													  u32XPos,
													  u32YPos);
			}

			// Broadcast the mask for this widget
			WidgetBroadcastMask(psWidget->eWidgetHandle,
								WCBK_PRESS_RELEASE,
								&uData);

//			DebugOut("%s: uData.bPressRelease=%u\n", __FUNCTION__, uData.sPressRelease.bPress);			// No longer being pressed

			psWidget->bCurrentlyPressed = FALSE;
		}
		psWidget = psWidget->psNextLink;
	}
}

void WidgetDefocusAll( void )
{
	if( sg_psWidgetInFocus )
	{
		sg_psWidgetInFocus->bInFocus = FALSE;
		sg_psWidgetInFocus = NULL;
	}
}

static void WidgetUpdateThread(void *pvParam)
{
	UINT32 u32Event;
	UINT32 u32Cmd;
	EGCResultCode eResult;
	UINT32 u32X = 0xffffffff;
	UINT32 u32Y = 0xffffffff;
	UINT32 u32AnimationTickRate;
	UINT32 u32ButtonMask = 0;

	ThreadSetName("Widget update thread");

	u32AnimationTickRate = 1000 / ANIMATION_INTERVAL;

	DebugOut("%s: Started\n", __FUNCTION__);

	while (1)
	{
		// Wait forever for something to do

		eResult = GCOSQueueReceive(sg_sWidgetQueue,
								   (void **) &u32Event,
								   0);
		GCASSERT(GC_OK == eResult);

		u32Cmd = u32Event & WCMD_WIDGET_MASK;
		u32Event &= WDATA_WIDGET_MASK;

		if (WCMD_POINTER_MOVED == u32Cmd)
		{
			u32X = u32Event & ((1 << 12) - 1);
			u32Y = (u32Event >> 12) & ((1 << 12) - 1);

//			DebugOut("%s: WCMD_POINTER_MOVED - X=%u, Y=%u (mask=0x%.2x)\n", __FUNCTION__, u32X, u32Y, u32ButtonMask);
			WindowPointerUpdate(u32ButtonMask,
								u32X,
							    u32Y);
		}
		else
		if (WCMD_BUTTON_PRESSED == u32Cmd)
		{
			u32ButtonMask |= u32Event;
			WindowButtonPressed(u32Event,
								u32X,
								u32Y);
//			DebugOut("%s: WCMD_BUTTON_PRESSED - u32Event=%u (mask=0x%.2x)\n", __FUNCTION__, u32Event, u32ButtonMask);
		}
		else
		if (WCMD_BUTTON_RELEASED == u32Cmd)
		{
			u32ButtonMask &= ~u32Event;

//			DebugOut("%s: WCMD_BUTTON_RELEASED - u32Event=%u (mask=0x%.2x)\n", __FUNCTION__, u32Event, u32ButtonMask);
			WindowButtonReleased(u32Event,
								 u32X,
								 u32Y);
		}
		else
		if (WCMD_ANIMATION_TICK == u32Cmd)
		{
			WindowAnimationTick(u32AnimationTickRate);
		}
		else
		if (WCMD_WIDGET_SHUTDOWN == u32Cmd)
		{
			// Shutdown request
			break;
		}
		else
		if (WCMD_KEYPRESS == u32Cmd)
		{
			EGCCtrlKey eGCKey;
			LEX_CHAR eUnicode;
			BOOL bPressed = FALSE;

			eGCKey = (EGCCtrlKey) ((u32Event >> 16) & 0xff);
			eUnicode = (LEX_CHAR) ((UINT16) u32Event);
			if (u32Event & WCMD_PRESSED)
			{
				bPressed = TRUE;
			}

			// If we have a widget that needs the event information, then let's call it
			if ((sg_psWidgetInFocus) && (sg_psWidgetInFocus->psWidgetFunc->WidgetKeypress))
			{
				sg_psWidgetInFocus->psWidgetFunc->WidgetKeypress(sg_psWidgetInFocus,
																 eGCKey,
																 eUnicode,
																 bPressed);
			}
		}
		else
		if (WCMD_WIDGET_MOUSEWHEEL == u32Cmd)
		{
			if ((sg_psWidgetInFocus) && (sg_psWidgetInFocus->psWidgetFunc->WidgetMousewheel))
			{
				sg_psWidgetInFocus->psWidgetFunc->WidgetMousewheel(sg_psWidgetInFocus,
																   u32Event);
			}
		}
		else
		{
			DebugOut("%s: Unkonwn widget command = 0x%.8x\n", __FUNCTION__, u32Cmd);
			DebugOut("%s: Before heap check\n", __FUNCTION__);
//			GCHeapCheck();
			DebugOut("%s: After heap check\n", __FUNCTION__);

			// Dump the queue's contents
//			GCOSQueueDump(sg_sWidgetQueue);

			// WTF? What's this?
			GCASSERT(0);
		}
	}

	DebugOut("%s: Exiting\n", __FUNCTION__);
}

static SoundChannel *sg_psWidgetSoundChannel = NULL;

void WidgetSoundPlay(SOUNDHANDLE eSoundHandle)
{
	(void) WavePlay(eSoundHandle,
					sg_psWidgetSoundChannel);
}

void WidgetInit(void)
{
	EGCResultCode eResult;
	void **ppvWidgetQueue;
	UINT32 u32Loop;

	DebugOut("* Initializing widget manager\n");

	sg_psWidgetSoundChannel = SoundChannelCreate(SOUND_PRIO_HIGHEST);
	GCASSERT(sg_psWidgetSoundChannel);

	ElementsInit();
	ConsoleFirstTimeInit();
	ButtonFirstTimeInit();
	TextFirstTimeInit();
	ImageWidgetFirstTimeInit();
	GraphFirstTimeInit();
	SliderFirstTimeInit();
	RadioFirstTimeInit();
	CheckboxWidgetFirstTimeInit();
	TouchRegionInit();
	ComboBoxFirstTimeInit();
	LineEditFirstTimeInit();
	TerminalFirstTimeInit();

	// Fill in the SIN_ACOS table
	for (u32Loop = 0; u32Loop < (sizeof(sg_u32SinACos) / sizeof(sg_u32SinACos[0])); u32Loop++)
	{
		sg_u32SinACos[u32Loop] = (UINT32) (sin(acos((double) u32Loop / 1024.0)) * 0x10000);
	}

	// Set up our animation counter goal
	sg_u32AnimationCounter = 0;
	sg_u32AnimationStep = (UINT32) ((65536.0 * (double) WIDGET_TIMER_INTERVAL) / ((double) WidgetGetAnimationStepTime()));

	// Set up message queue
	
	ppvWidgetQueue = MemAlloc(sizeof(*ppvWidgetQueue) * WIDGET_QUEUE_SIZE);
	GCASSERT(ppvWidgetQueue);

	eResult = GCOSQueueCreate(&sg_sWidgetQueue,
							  ppvWidgetQueue,
							  WIDGET_QUEUE_SIZE);
	GCASSERT(GC_OK == eResult);

	// Now create a window manager thread
	eResult = GCOSThreadCreate(WidgetUpdateThread,
							   NULL,
							   &sg_u8WidgetThreadStack[WIDGET_THREAD_STACK_SIZE - 4],
							   1);
	GCASSERT(GC_OK == eResult);

	// Create the pointer reader thread

	eResult = GCOSThreadCreate(WidgetPointerReaderThread,
							   NULL,
							   &sg_u8WidgetPointerThreadStack[WIDGET_POINTER_THREAD_STACK_SIZE - 4],
							   2);
	GCASSERT(GC_OK == eResult);

	// Check to see if our pointer is available. If not, then don't fire off
	// a timer to grab its value. Otherwise, get the default X and Y
	// positions and button state.

	if (GCPointerGetPosition(0, &sg_u32PointerXPos, &sg_u32PointerYPos, &sg_u32Buttons) != GC_OK)
	{
		DebugOut("  No pointer device supported\n");
	}
	else
	{
		// Set up a timer callback every WIDGET_TIMER_INTERVAL
		eResult = GCTimerCreate(&sg_psWidgetTimer);
		GCASSERT(GC_OK == eResult);
		eResult = GCTimerSetCallback(sg_psWidgetTimer,
									 WidgetTimerCallback,
									 0);
		GCASSERT(GC_OK == eResult);
		eResult = GCTimerSetValue(sg_psWidgetTimer,
								  WIDGET_TIMER_INTERVAL,
								  WIDGET_TIMER_INTERVAL);

		GCASSERT(GC_OK == eResult);
		eResult = GCTimerStart(sg_psWidgetTimer);
		GCASSERT(GC_OK == eResult);
	}
}

static void WidgetStateChangeInternal( SWidget* psWidget, 
									   BOOL bPressed, 
									   BOOL bFocusOnly,
									   UINT32 u32XPos,
									   UINT32 u32YPos,
									   UINT32 u32Mask )
{
	UWidgetCallbackData uData;

	GCASSERT(psWidget);

	if (bPressed != psWidget->bCurrentlyPressed)
	{
		if( FALSE == bFocusOnly )
		{
			// Record appropriate data
			memset(&uData, 0, sizeof(uData));
			uData.sPressRelease.u32XPos = u32XPos;
			uData.sPressRelease.u32YPos = u32YPos;
			uData.sPressRelease.u32ButtonMask = u32Mask;
		}

		// We have a state change
		if (bPressed)
		{
			// Means the widget was pressed
			if( (FALSE == bFocusOnly) && (psWidget->psWidgetFunc->WidgetPress) )
			{
				psWidget->psWidgetFunc->WidgetPress(psWidget,
													u32Mask,
													u32XPos,
													u32YPos);
			}

			// Does this widget have a parent? If so, we might need to change its focus
			if (psWidget->eParentWidget != HANDLE_INVALID)
			{
				SWidget *psParent;
				ELCDErr eLCDErr;

				eLCDErr = WidgetGetPointerByHandle(psWidget->eParentWidget,
												   WidgetGetHandleType(psWidget->eParentWidget),
												   &psParent,
												   NULL);
				GCASSERT(LERR_OK == eLCDErr);

				// Now let's check out the parent
				if ((FALSE == psParent->bInFocus) &&
					(psParent->psWidgetFunc->WidgetSetFocus))
				{
					// If we have another widget in focus, defocus it
					if (sg_psWidgetInFocus)
					{
						sg_psWidgetInFocus->bInFocus = FALSE;
						if( sg_psWidgetInFocus->psWidgetFunc->WidgetSetFocus )
						{
							sg_psWidgetInFocus->psWidgetFunc->WidgetSetFocus(sg_psWidgetInFocus,
																			 FALSE);
						}
					}

					// Set the new widget in focus
					psParent->bInFocus = TRUE;
					psParent->psWidgetFunc->WidgetSetFocus(psParent,
														   TRUE);
					sg_psWidgetInFocus = psParent;
				}
			}
			else
			// Is this widget in focus? If not, then send a focus change message
			if ((FALSE == psWidget->bInFocus) &&
				(psWidget->psWidgetFunc->WidgetSetFocus))
			{
				// If we have another widget in focus, defocus it
				if (sg_psWidgetInFocus)
				{
					sg_psWidgetInFocus->bInFocus = FALSE;
					if( (sg_psWidgetInFocus->psWidgetFunc) && 
						(sg_psWidgetInFocus->psWidgetFunc->WidgetSetFocus) )
					{
						sg_psWidgetInFocus->psWidgetFunc->WidgetSetFocus(sg_psWidgetInFocus,
																		 FALSE);
					}
				}

				// Set the new widget in focus
				psWidget->bInFocus = TRUE;
				psWidget->psWidgetFunc->WidgetSetFocus(psWidget,
													   TRUE);
				sg_psWidgetInFocus = psWidget;
			}

			psWidget->bSelectedSinceLastRead = TRUE;

			if( FALSE == bFocusOnly )
			{
				psWidget->bCurrentlyPressed = TRUE;
			}

			// Fill in the appropriate data - indicate a button press
			uData.sPressRelease.bPress = TRUE;
		}
		else
		{
			// Otherwise release this widget, since it doesn't track
			if( FALSE == bFocusOnly )
			{
				if( psWidget->psWidgetFunc->WidgetRelease )
				{
					psWidget->psWidgetFunc->WidgetRelease(psWidget,
														  u32Mask,
														  u32XPos,
														  u32YPos);
				}

				// Deassert this widget
				psWidget->bCurrentlyPressed = FALSE;
			}
		}

		// Broadcast the mask for this widget
		if( FALSE == bFocusOnly )
		{
			WidgetBroadcastMask(psWidget->eWidgetHandle,
								WCBK_PRESS_RELEASE,
								&uData);
		}
	}
	else
	{
		// Already understood state for this widget
	}
}


// Virtually click this widget in the middle
ELCDErr WidgetVirtualClick( WIDGETHANDLE eHandle )
{
	ELCDErr eErr;
	SWidget *psWidget;
	INT32 s32XPos;
	INT32 s32YPos;

	eErr = WidgetGetPointerByHandle(eHandle,
									WidgetGetHandleType(eHandle),
									&psWidget,
									NULL);

	RETURN_ON_FAIL(eErr);

	GCASSERT(psWidget);

	s32XPos = psWidget->s32XPos + (psWidget->u32XSize >> 1);
	s32YPos = psWidget->s32YPos + (psWidget->u32YSize >> 1);

	// Just ignore coordinates that are off the screen
	if( (s32XPos < 0) || (s32YPos < 0))
	{
		return(LERR_OK);
	}

	// Click (press + release) the widget with coordinates in the center
	WidgetStateChangeInternal( psWidget, TRUE, FALSE, s32XPos, s32XPos, 1 );
	WidgetStateChangeInternal( psWidget, FALSE, FALSE, s32XPos, s32XPos, 0 );

	return(LERR_OK);
}


// Virtually set the widget focus
ELCDErr WidgetVirtualActivate( WIDGETHANDLE eHandle )
{
	ELCDErr eErr;
	SWidget *psWidget;

	eErr = WidgetGetPointerByHandle(eHandle,
									WidgetGetHandleType(eHandle),
									&psWidget,
									NULL);

	RETURN_ON_FAIL(eErr);

	GCASSERT(psWidget);

	// Now got set the focus (taking care of any widget that currently has focus)
	WidgetStateChangeInternal( psWidget, TRUE, TRUE, 0, 0, 0 );

	return(LERR_OK);
}


static void WidgetChangedInternal(SWidget *psWidgetHead,
								  UINT32 u32Mask,
								  UINT32 u32XPos,
								  UINT32 u32YPos)
{
	BOOL bHit = FALSE;
	SWidget *psWidget;

#ifdef _WIN32
//	DebugOut("Widget x/y change=%d/%d, %d\n", u32XPos, u32YPos, u32Mask);
#endif

	// X/Y position is now *WINDOW* relative

	psWidget = psWidgetHead;
	while (psWidget)
	{
		EMouseOverState eMouseoverState = EMOUSEOVER_UNCHANGED;
		BOOL bPriorMouseover;

		bHit = FALSE;

		// Does this land within this widget's bounding box?
		bPriorMouseover = psWidget->bMousedOver;

		if (FALSE == psWidget->bWidgetHidden)
		{
			// Is this position change out of range?
			if (((INT32) u32XPos < psWidget->s32XPos) ||
				((INT32) u32XPos >= (psWidget->s32XPos + psWidget->u32XSize)) ||
				((INT32) u32YPos < psWidget->s32YPos) ||
				((INT32) u32YPos >= (psWidget->s32YPos + psWidget->u32YSize)))
			{
				// Out of range of this current widget.
				psWidget->bMousedOver = FALSE;
			}
			else
			{
				// It's within the bounding box
				bHit = TRUE;
				psWidget->bMousedOver = TRUE;

				if ((psWidget->psWidgetFunc) &&
					(psWidget->psWidgetFunc->WidgetRegionTest))
				{
					if (psWidget->psWidgetFunc->WidgetRegionTest(psWidget, u32XPos, u32YPos))
					{
						// This indicates the widget has accepted the hit test - it's ours!

					}
					else
					{
						// Widget has rejected it as a hit, but it IS our widget, so we don't
						// do anything with this update
						psWidget->bMousedOver = FALSE;
					}
				}
			}
		}
		else
		{
			psWidget->bMousedOver = FALSE;
		}

		// Set the mouseover state
		if (bPriorMouseover != psWidget->bMousedOver)
		{
			// If we're moused over now, we're asserting
			if (psWidget->bMousedOver)
			{
				eMouseoverState = EMOUSEOVER_ASSERTED;
			}
			else
			{
				eMouseoverState = EMOUSEOVER_DEASSERTED;
			}
		}
		else
		{
			// Nothing changed
			eMouseoverState = EMOUSEOVER_UNCHANGED;
		}

		// bHit is set to TRUE if this widget is being pressed, otherwise it's FALSE
		// if not.

		// If the widget is disabled, ignore the hit
		if (FALSE == psWidget->bWidgetEnabled)
		{
			bHit = FALSE;
		}

		// If the mask is 0, there's no hit - it's a release
		if (0 == u32Mask)
		{
			bHit = FALSE;
		}

		// If the widget is disabled or hidden, don't trigger anything
		if (FALSE == psWidget->bWidgetEnabled)
		{
			// Don't do anything
		}
		else
		{
			UWidgetCallbackData uData;

			// Clear out our widget data
			memset((void *) &uData, 0, sizeof(uData));

			// The widget itself is enabled, it is moused over, or the mouseover isn't unchanged,
			// inform the widget.

			if ((psWidget->bMousedOver) ||
				(eMouseoverState != EMOUSEOVER_UNCHANGED))
			{
				// Let's see if it wants a mouseover
				if ((psWidget->psWidgetFunc->WidgetMouseover) &&
					(FALSE == psWidget->bMouseOverDisabled))
				{
					psWidget->psWidgetFunc->WidgetMouseover(psWidget,
															u32Mask,
															u32XPos,
															u32YPos,
															eMouseoverState);
				}

				// Record appropriate data
				uData.sMouseOver.u32XPos = u32XPos;
				uData.sMouseOver.u32YPos = u32YPos;
				uData.sMouseOver.u32ButtonMask = u32Mask;
				uData.sMouseOver.eMouseoverState = eMouseoverState;

				// Broadcast moused-over message
				WidgetBroadcastMask(psWidget->eWidgetHandle,
									WCBK_MOUSEOVER,
									&uData);
			}

			WidgetStateChangeInternal( psWidget, bHit, FALSE, u32XPos, u32YPos, u32Mask );
		}

		if (psWidget)
		{
			psWidget = psWidget->psNextLink;
		}
	}
}

void WidgetMousewheel(UINT32 u32Value)
{
	EGCResultCode eResult;

	eResult = GCOSQueueSend(sg_sWidgetQueue,
							(void *) (WCMD_WIDGET_MOUSEWHEEL | u32Value));
	GCASSERT(GC_OK == eResult);
}


void WidgetUpdatePointer(SWidget *psWidget,
						 UINT32 u32ButtonMask,
						 UINT32 u32XPointerPos,
						 UINT32 u32YPointerPos,
						 UINT32 u32XPos,
						 UINT32 u32YPos)
{
	WidgetChangedInternal(sg_psWidgetsWithoutWindows,
						  u32ButtonMask,
						  u32XPointerPos,
						  u32YPointerPos);
	WidgetChangedInternal(psWidget,
						  u32ButtonMask,
						  u32XPos,
						  u32YPos);
}

void WidgetButtonPressed(SWidget *psWidgetHead,
						 UINT32 u32ButtonMask,
						 UINT32 u32XPointerPos,
						 UINT32 u32YPointerPos,
						 UINT32 u32XPos,
						 UINT32 u32YPos)
{
	WidgetChangedInternal(sg_psWidgetsWithoutWindows,
						  u32ButtonMask,
						  u32XPointerPos,
						  u32YPointerPos);
	WidgetChangedInternal(psWidgetHead,
						  u32ButtonMask,
						  u32XPos,
						  u32YPos);
}

void WidgetEraseStandard(SWidget *psWidget)
{
	SWindow *psWindow;

	GCASSERT(psWidget);
	psWindow = WindowGetPointer(psWidget->eParentWindow);
	GCASSERT(psWindow);

	// Mark the region for erasure
	WindowEraseActiveRegion(psWindow,
						    psWidget->s32XPos,
							psWidget->s32YPos,
							psWidget->u32XSize,
							psWidget->u32YSize);

	WindowUpdateRegion(psWidget->eParentWindow,
					   (INT32) (psWidget->s32XPos + psWindow->u32ActiveAreaXPos),
					   (INT32) (psWidget->s32YPos + psWindow->u32ActiveAreaYPos),
					   psWidget->u32XSize,
					   psWidget->u32YSize);
}

void WidgetErase(SWidget *psWidget)
{
	if (psWidget->psWidgetFunc && (FALSE == psWidget->bWidgetHidden))
	{
		if (psWidget->psWidgetFunc->WidgetErase)
		{
			psWidget->psWidgetFunc->WidgetErase(psWidget);
		}
	}
}

void WidgetPaint(SWidget *psWidget,
				 BOOL bLock)
{
	if (psWidget->psWidgetFunc && (FALSE == psWidget->bWidgetHidden))
	{
		if (psWidget->psWidgetFunc->WidgetRepaint)
		{
			psWidget->psWidgetFunc->WidgetRepaint(psWidget,
												  bLock);
		}
	}
}

static void WidgetEraseIntersectionsInternal(SWidgetIntersection *psIntersection)
{
	while (psIntersection)
	{
		if (FALSE == psIntersection->psWidget->bRedrawInProgress)
		{
			WidgetErase(psIntersection->psWidget);
			psIntersection->psWidget->bRedrawInProgress = TRUE;
			WidgetEraseIntersectionsInternal(psIntersection->psWidget->psIntersectionList);
		}

		psIntersection = psIntersection->psNextLink;
	}
}

void WidgetEraseIntersections(SWidget *psTargetWidget)
{
	SWindow *psWindow;
	SWidget *psWidget;

	if (NULL == psTargetWidget->psIntersectionList)
	{
		return;
	}

	// Flag our target widget as "draw in progress" so we don't erase or otherwise
	// touch it.
	psTargetWidget->bRedrawInProgress = TRUE;

	WidgetEraseIntersectionsInternal(psTargetWidget->psIntersectionList);

	// Now run through all widgets in this window and turn off the redraw in progress
	// boolean

	psWindow = WindowGetPointer(psTargetWidget->eParentWindow);
	GCASSERT(psWindow);

	psWidget = psWindow->psWidgetHead;
	while (psWidget)
	{
		if (psWidget != psTargetWidget)
		{
			psWidget->bRedrawInProgress = FALSE;
		}

		psWidget = psWidget->psNextLink;
	}

	// Now, get rid of the marker so we don't redraw it later
	GCASSERT(psTargetWidget->bRedrawInProgress);
	psTargetWidget->bRedrawInProgress = FALSE;
}

static void WidgetPaintIntersectionsInternal(SWidgetIntersection *psIntersection)
{
	while (psIntersection)
	{
		if (FALSE == psIntersection->psWidget->bRedrawInProgress)
		{
			WidgetPaint(psIntersection->psWidget,
						FALSE);
			psIntersection->psWidget->bRedrawInProgress = TRUE;
			WidgetPaintIntersectionsInternal(psIntersection->psWidget->psIntersectionList);
		}

		psIntersection = psIntersection->psNextLink;
	}
}

void WidgetPaintIntersections(SWidget *psTargetWidget)
{
	SWindow *psWindow;
	SWidget *psWidget;

	if (NULL == psTargetWidget->psIntersectionList)
	{
		return;
	}

	// Flag our target widget as "draw in progress" so we don't paint or otherwise
	// touch it.
	psTargetWidget->bRedrawInProgress = TRUE;

	WidgetPaintIntersectionsInternal(psTargetWidget->psIntersectionList);

	// Now run through all widgets in this window and turn off the redraw in progress
	// boolean

	psWindow = WindowGetPointer(psTargetWidget->eParentWindow);
	GCASSERT(psWindow);

	psWidget = psWindow->psWidgetHead;
	while (psWidget)
	{
		if (psWidget != psTargetWidget)
		{
			psWidget->bRedrawInProgress = FALSE;
		}

		psWidget = psWidget->psNextLink;
	}

	GCASSERT(psTargetWidget->bRedrawInProgress);
	psTargetWidget->bRedrawInProgress = FALSE;
}

void WidgetButtonReleased(SWidget *psWidgetHead,
						  UINT32 u32ButtonMask,
						  UINT32 u32XPointerPos,
						  UINT32 u32YPointerPos,
						  UINT32 u32XPos,
						  UINT32 u32YPos)
{
	// u32ButtonMask only contains the button that was released, not the entire
	// button mask

	WidgetListReleaseAllWidgets(psWidgetHead,
								u32ButtonMask,
								u32XPos,
								u32YPos);
	WidgetListReleaseAllWidgets(psWidgetHead,
								u32ButtonMask,
								u32XPointerPos,
								u32YPointerPos);
}

ELCDErr WidgetSetHide(SWidget *psWidget,
					  BOOL bWidgetHidden,
					  BOOL bForceRepaint)
{
	ELCDErr eErr = LERR_OK;

	if ((psWidget->bWidgetHidden == bWidgetHidden) &&
		(FALSE == bForceRepaint))
	{
		// Already in the requested state. Just return.
		goto errorExit;
	}

	eErr = WindowLockByHandle(psWidget->eParentWindow);
	if (eErr != LERR_OK)
	{
		goto errorExit;
	}

	WidgetSetUpdate(TRUE);

	if (bWidgetHidden)
	{
		// This means we want to erase the widget/hide it
		if (psWidget->psWidgetFunc->WidgetErase)
		{
			psWidget->psWidgetFunc->WidgetErase(psWidget);
		}

		WidgetEraseIntersections(psWidget);
		WidgetPaintIntersections(psWidget);

		// Hide it!
		psWidget->bWidgetHidden = TRUE;
	}
	else
	{
		psWidget->bWidgetHidden = FALSE;

		// This means we want to show the widget
		if (psWidget->psWidgetFunc->WidgetRepaint)
		{
			psWidget->psWidgetFunc->WidgetRepaint(psWidget,
												  FALSE);
		}
	}

	// Now we need to recalculate the intersections
	eErr = WidgetCalcIntersections(psWidget);

	if (LERR_OK == eErr)
	{
		eErr = WindowUnlockByHandle(psWidget->eParentWindow);
	}
	else
	{
		(void) WindowUnlockByHandle(psWidget->eParentWindow);
	}

	WidgetSetUpdate(FALSE);

	WindowUpdateRegionCommit();

errorExit:
	return(eErr);
}

ELCDErr WidgetSetSize(SWidget *psWidget,
					  UINT32 u32XSize,
					  UINT32 u32YSize,
					  BOOL bWindowLock,
					  BOOL bForceRepaint,
					  BOOL bRepaintAfterSizing)
{
	ELCDErr eErr = LERR_OK;

	if ((psWidget->u32XSize != u32XSize) ||
		(psWidget->u32YSize != u32YSize) ||
		bForceRepaint)
	{
		// We're changing widget sizes. Let's first 
		if (bWindowLock)
		{
			eErr = WindowLockByHandle(psWidget->eParentWindow);
			if (eErr != LERR_OK)
			{
				goto errorExit;
			}
		}

		if (bWindowLock)
		{
			WidgetSetUpdate(TRUE);
		}

		WidgetErase(psWidget);
		WidgetEraseIntersections(psWidget);
		WidgetPaintIntersections(psWidget);

		// Now change the widget's size
		psWidget->u32XSize = u32XSize;
		psWidget->u32YSize = u32YSize;

		eErr = WidgetCalcIntersections(psWidget);

		if (bRepaintAfterSizing)
		{
			WidgetPaint(psWidget,
						FALSE);
		}

		if (bWindowLock)
		{
			if (LERR_OK == eErr)
			{
				eErr = WindowUnlockByHandle(psWidget->eParentWindow);
			}
			else
			{
				(void) WindowUnlockByHandle(psWidget->eParentWindow);
			}
		}

		if (bWindowLock)
		{
			WidgetSetUpdate(FALSE);
		}
	}

errorExit:
	return(eErr);
}

UINT32 WidgetGetAnimationStepTime(void)
{
	return(1000 / ANIMATION_INTERVAL);
}

ELCDErr BoxRender(UINT32 u32XSize,
				  UINT32 u32YSize,
				  UINT32 u32XPos,
				  UINT32 u32YPos,
				  UINT32 u32BoxColor,
				  SImage **ppsImage,
				  BOOL bDown)
{
	ELCDErr eErr = LERR_OK;
	UINT16 u16BoxColor;
	UINT32 u32Loop;
	UINT16 *pu16PixelPtr;

	// Gotta be at least 3x3
	if (((u32XSize < 3) && (u32XSize != 0)) ||
		((u32YSize < 3) && (u32YSize != 0)))
	{
		eErr = LERR_BUTTON_SIZE_TOO_SMALL;
		goto errorExit;
	}

	// Now get the fill color for the box
	u16BoxColor = CONVERT_24RGB_16RGB(u32BoxColor);

	if (NULL == *ppsImage)
	{
		// Now let's create an empty image in which to draw on
		*ppsImage = GfxCreateEmptyImage(u32XSize + u32XPos,
										u32YSize + u32YPos,
										16,
										u16BoxColor,
										FALSE);
		if (NULL == *ppsImage)
		{
			eErr = LERR_NO_MEM;
			goto errorExit;
		}
	}

	pu16PixelPtr = (*ppsImage)->pu16ImageData + u32XPos + (u32YPos * (*ppsImage)->u32Pitch);

	// Draw white side

	if (bDown)
	{
		// Down - draw white on the bottom
		pu16PixelPtr += ((u32YSize - 1) * (*ppsImage)->u32Pitch);
		for (u32Loop = 0; u32Loop < u32XSize; u32Loop++)
		{
			*pu16PixelPtr = BORDER_BRIGHTEDGE;
			++pu16PixelPtr;
		}

		// Down - white on the right
		pu16PixelPtr = (*ppsImage)->pu16ImageData + (u32XSize - 1) + u32XPos + (u32YPos * (*ppsImage)->u32Pitch);
		for (u32Loop = 0; u32Loop < u32YSize; u32Loop++)
		{
			*pu16PixelPtr = BORDER_BRIGHTEDGE;
			pu16PixelPtr += (*ppsImage)->u32Pitch;
		}
	}
	else
	{
		// UP - draw white on top
		for (u32Loop = 0; u32Loop < (u32XSize - 1); u32Loop++)
		{
			*pu16PixelPtr = BORDER_BRIGHTEDGE;
			++pu16PixelPtr;
		}

		// UP - Draw white on left side
		pu16PixelPtr = (*ppsImage)->pu16ImageData + u32XPos + (u32YPos * (*ppsImage)->u32Pitch);
		for (u32Loop = 0; u32Loop < (u32YSize - 1); u32Loop++)
		{
			*pu16PixelPtr = BORDER_BRIGHTEDGE;
			pu16PixelPtr += (*ppsImage)->u32Pitch;
		}
	}

	// Draw black outer edge
	
	pu16PixelPtr = (*ppsImage)->pu16ImageData + u32XPos + (u32YPos * (*ppsImage)->u32Pitch);
	if (bDown)
	{
		// Down - top pixels 
		for (u32Loop = 0; u32Loop < (u32XSize - 1); u32Loop++)
		{
			*pu16PixelPtr = BORDER_OUTER;
			++pu16PixelPtr;
		}

		// Down - left pixels
		pu16PixelPtr = (*ppsImage)->pu16ImageData + u32XPos + (u32YPos * (*ppsImage)->u32Pitch);
		for (u32Loop = 0; u32Loop < (u32YSize - 1); u32Loop++)
		{
			*pu16PixelPtr = BORDER_OUTER;
			pu16PixelPtr += (*ppsImage)->u32Pitch;
		}
	}
	else
	{
		// Up - bottom pixels
		pu16PixelPtr += ((u32YSize - 1) * (*ppsImage)->u32Pitch);
		for (u32Loop = 0; u32Loop < (u32XSize - 1); u32Loop++)
		{
			*pu16PixelPtr = BORDER_OUTER;
			++pu16PixelPtr;
		}

		// Up - right pixels
		pu16PixelPtr = (*ppsImage)->pu16ImageData + (u32XSize - 1) + u32XPos + (u32YPos * (*ppsImage)->u32Pitch);
		for (u32Loop = 0; u32Loop < u32YSize; u32Loop++)
		{
			*pu16PixelPtr = BORDER_OUTER;
			pu16PixelPtr += (*ppsImage)->u32Pitch;
		}
	}

	// Now muted inner image

	pu16PixelPtr = (*ppsImage)->pu16ImageData + u32XPos + (u32YPos * (*ppsImage)->u32Pitch);
	if (bDown)
	{
		// Down - top pixels 
		pu16PixelPtr += (*ppsImage)->u32Pitch + 1;
		for (u32Loop = 0; u32Loop < (u32XSize - 3); u32Loop++)
		{
			*pu16PixelPtr = BORDER_INNER;
			++pu16PixelPtr;
		}

		// Down - left pixels
		pu16PixelPtr = (*ppsImage)->pu16ImageData + (*ppsImage)->u32Pitch + 1 + u32XPos + (u32YPos * (*ppsImage)->u32Pitch);
		for (u32Loop = 0; u32Loop < (u32YSize - 3); u32Loop++)
		{
			*pu16PixelPtr = BORDER_INNER;
			pu16PixelPtr += (*ppsImage)->u32Pitch;
		}
	}
	else
	{
		// Up - bottom pixels
		pu16PixelPtr += ((u32YSize - 2) * (*ppsImage)->u32Pitch) + 1 + u32XPos + (u32YPos * (*ppsImage)->u32Pitch);
		for (u32Loop = 0; u32Loop < (u32XSize - 2); u32Loop++)
		{
			*pu16PixelPtr = BORDER_INNER;
			++pu16PixelPtr;
		}

		// Up - right pixels
		pu16PixelPtr = (*ppsImage)->pu16ImageData + (u32XSize - 2) + (*ppsImage)->u32Pitch + u32XPos + (u32YPos * (*ppsImage)->u32Pitch);
		for (u32Loop = 0; u32Loop < (u32YSize - 2); u32Loop++)
		{
			*pu16PixelPtr = BORDER_INNER;
			pu16PixelPtr += (*ppsImage)->u32Pitch;
		}
	}

errorExit:
	if (eErr != LERR_OK)
	{
		// If there's an image that has been created, delete it
		if (*ppsImage)
		{
			GfxDeleteImage(*ppsImage);
			*ppsImage = NULL;
		}
	}
	return(eErr);
}

void WidgetSetUpdate(BOOL bUpdating)
{
	WindowBlitLock(bUpdating);
}

#define	COLOR_EDGE1	0x00
#define COLOR_EDGE2	0x01
#define COLOR_FILL	0x02

ELCDErr CircleRender(UINT32 u32XPos,
					 UINT32 u32YPos,
					 UINT32 u32Radius,
					 UINT32 u32EdgeColor1,
					 UINT32 u32EdgeColor2,
					 UINT32 u32FillColor,
					 SImage *psImage,
					 UINT8 u8BaseIndex,
					 BOOL bFill)
{
	UINT32 u32InvRadius = (UINT32) ((1.0 / (double) (u32Radius)) * 0x10000);
	INT32 s32DX = 0, s32DY = 0;
	INT32 s32DXOffset = 0, s32DYOffset = 0, s32Offset = 0;
	UINT8 *pu8Data = psImage->pu8ImageData;
	INT32 s32N = 0;

	// Let's see if it'll fit. If not, kick back an error.
	if ( ((u32XPos + (u32Radius << 1)) > psImage->u32XSize) ||
		 ((u32YPos + (u32Radius << 1)) > psImage->u32YSize) )
	{
		return(LERR_IMAGE_TARGET_TOO_SMALL);
	}

	// If the target image is 8bpp, then create a palette
	if (psImage->pu8ImageData)
	{
		// If there's no palette, make one
		psImage->u16TransparentIndex = 0;

		if (NULL == psImage->pu16Palette)
		{
			psImage->pu16Palette = MemAlloc((sizeof(UINT8) << 8) * sizeof(*psImage->pu16Palette));
			if (NULL == psImage->pu16Palette)
			{
				// Out of memory
				return(LERR_NO_MEM);
			}

			// This had better not be 0
			GCASSERT(u8BaseIndex);

			psImage->pu16Palette[u8BaseIndex + COLOR_EDGE1] = CONVERT_24RGB_16RGB(u32EdgeColor1);
			psImage->pu16Palette[u8BaseIndex + COLOR_EDGE2] = CONVERT_24RGB_16RGB(u32EdgeColor2);
			psImage->pu16Palette[u8BaseIndex + COLOR_FILL] = CONVERT_24RGB_16RGB(u32FillColor);
		}
	}
	else
	{
		GCASSERT(0);
	}

	s32Offset = (u32XPos + u32Radius) + ((u32YPos + u32Radius) * psImage->u32Pitch);

	// Is there a fill?
	if (bFill)
	{
		s32DY = (INT32) (u32Radius - 1);
		s32N = 0;

		GCASSERT(pu8Data);

		// Fill it!

		s32DXOffset = (s32DX * psImage->u32Pitch);

		while (s32DX <= s32DY)
		{
			INT32 s32Loop;

			s32DYOffset = s32DY * psImage->u32Pitch;

			for (s32Loop = s32DY; s32Loop >= s32DX; s32Loop--, s32DYOffset -= psImage->u32Pitch)
			{
				pu8Data[s32Offset + s32Loop - s32DXOffset] = (COLOR_FILL + u8BaseIndex);
				pu8Data[s32Offset + s32DX - s32DYOffset] = (COLOR_FILL + u8BaseIndex);
				pu8Data[s32Offset - s32DX - s32DYOffset] = (COLOR_FILL + u8BaseIndex);
				pu8Data[s32Offset - s32Loop - s32DXOffset] = (COLOR_FILL + u8BaseIndex);
				pu8Data[s32Offset - s32Loop + s32DXOffset] = (COLOR_FILL + u8BaseIndex);
				pu8Data[s32Offset + s32DX + s32DYOffset] = (COLOR_FILL + u8BaseIndex);
				pu8Data[s32Offset - s32DX + s32DYOffset] = (COLOR_FILL + u8BaseIndex);
				pu8Data[s32Offset + s32Loop + s32DXOffset] = (COLOR_FILL + u8BaseIndex);
			}

			s32DX++;
			s32DXOffset += psImage->u32Pitch;
			s32N += (INT32) u32InvRadius;
			s32DY = (INT32) ((u32Radius * sg_u32SinACos[(INT32) (s32N >> 6)]) >> 16);
		}
	}

	// Time to draw the outer edge

	s32DY = (INT32) (u32Radius - 1);
	s32N = 0;
	s32DX = 0;

	s32DXOffset = s32DX * psImage->u32Pitch;

	while (s32DX <= s32DY)
	{
		s32DYOffset = s32DY * psImage->u32Pitch;

		pu8Data[s32Offset + s32DY - s32DXOffset] = (COLOR_EDGE2 + u8BaseIndex);
		pu8Data[s32Offset + s32DX - s32DYOffset] = (COLOR_EDGE1 + u8BaseIndex);
		pu8Data[s32Offset - s32DX - s32DYOffset] = (COLOR_EDGE1 + u8BaseIndex);
		pu8Data[s32Offset - s32DY - s32DXOffset] = (COLOR_EDGE1 + u8BaseIndex);
		pu8Data[s32Offset - s32DY + s32DXOffset] = (COLOR_EDGE1 + u8BaseIndex);
		pu8Data[s32Offset - s32DX + s32DYOffset] = (COLOR_EDGE2 + u8BaseIndex);
		pu8Data[s32Offset + s32DX + s32DYOffset] = (COLOR_EDGE2 + u8BaseIndex);
		pu8Data[s32Offset + s32DY + s32DXOffset] = (COLOR_EDGE2 + u8BaseIndex);

		s32DX++;
		s32DXOffset += psImage->u32Pitch;
		s32N += (INT32) u32InvRadius;
		s32DY = (INT32) ((u32Radius * sg_u32SinACos[(INT32) (s32N >> 6)]) >> 16);
	}

	return(LERR_OK);
}

static SWidgetBucket *sg_psWidgetBuckets[WIDGET_MAX];

ELCDErr WidgetAllocateHandle(WIDGETHANDLE *peWidgetHandle,
							 EWidgetType eWidgetType,
							 WINDOWHANDLE eWindowHandle,
							 SWidget **ppsWidget)
{
	SWidgetBucket *psBucketPtr;
	SWidgetBucket *psBucketPrior = NULL;
	SWidget *psWidget = NULL;
	ELCDErr eErr = LERR_OK;
	UINT32 u32Bucket = 0;
	WIDGETHANDLE eWidgetHandle;

	// If the window handle isn't an invalid handle, ditch it
	if (eWindowHandle != HANDLE_INVALID)
	{
		if (NULL == WindowGetPointer(eWindowHandle))
		{
			return(LERR_WIN_BAD_HANDLE);
		}
	}

	// Set the widget handle to invalid to start with
	*peWidgetHandle = HANDLE_INVALID;

	GCASSERT((eWidgetType > WIDGET_WTF) &&
			 (eWidgetType < WIDGET_MAX));

	psBucketPtr = sg_psWidgetBuckets[eWidgetType];

	// Loop through until we have a bucket that has at least one slot
	while ((psBucketPtr) && (psBucketPtr->u8ActiveWidgetCount >= (1 << BUCKET_ITEM_BIT_SIZE)))
	{
		psBucketPrior = psBucketPtr;
		psBucketPtr = psBucketPtr->psNextLink;
		++u32Bucket;
	}

	// If psBucketPtr is NULL, no room in any existing links
	if (NULL == psBucketPtr)
	{
		if (psBucketPrior)
		{
			psBucketPrior->psNextLink = MemAlloc(sizeof(*psBucketPrior));
			psBucketPrior = psBucketPrior->psNextLink;
		}
		else
		{
			psBucketPrior = MemAlloc(sizeof(*psBucketPrior));
			GCASSERT(NULL == sg_psWidgetBuckets[eWidgetType]);
			sg_psWidgetBuckets[eWidgetType] = psBucketPrior;
		}

		if (NULL == psBucketPrior)
		{
			// This is OK, here. We haven't allocated anything.
			return(LERR_NO_MEM);
		}

		psBucketPtr = psBucketPrior;
	}

	// psBucketPtr now points to a valid bucket. Let's make sure the handle position has
	// a NULL in it.
	GCASSERT(psBucketPtr->u32HandlePosition < (1 << BUCKET_ITEM_BIT_SIZE));
	GCASSERT(NULL == psBucketPtr->psWidget[psBucketPtr->u32HandlePosition]);

	// Fabricate a widget handle
	eWidgetHandle = (eWidgetType << BUCKET_TYPE_POS) | (u32Bucket << BUCKET_ITEM_BIT_SIZE) | psBucketPtr->u32HandlePosition;

	// Go allocate a widget
	eErr = WidgetAllocate(&psWidget);
	RETURN_ON_FAIL(eErr);

	// Record our widget's handle in the widget itself
	psWidget->eWidgetHandle = eWidgetHandle;

	// Der widget be allocated!
	psBucketPtr->psWidget[psBucketPtr->u32HandlePosition] = psWidget;

	// Hook up the widget type
	psWidget->eWidgetType = eWidgetType;

	// Say that it's not a subordinate widget
	psWidget->eParentWidget = HANDLE_INVALID;

	// And the widget's methods
	GCASSERT(sg_psWidgetTypeMethods[eWidgetType]->psWidgetFunctions);
	psWidget->psWidgetFunc = sg_psWidgetTypeMethods[eWidgetType]->psWidgetFunctions;

	// Connect up the parent window
	psWidget->eParentWindow = eWindowHandle;

	// We've allocated our widget. Now the specific stuff gets allocated.
	eErr = sg_psWidgetTypeMethods[eWidgetType]->WidgetTypeAlloc(psWidget,
																eWidgetHandle);

	if (eErr != LERR_OK)
	{
		(void) WidgetDelete(psWidget);
		return(eErr);
	}

	if (ppsWidget)
	{
		*ppsWidget = psWidget;
	}

	if (eWindowHandle != HANDLE_INVALID)
	{
		// Now connect this widget up to the widget manager
		eErr = WindowWidgetConnect(eWindowHandle,
									  psWidget);
		ERROREXIT_ON_FAIL(eErr);
	}
	else
	{
		// Link it in to the homeless widget list
		psWidget->psNextLink = sg_psWidgetsWithoutWindows;
		sg_psWidgetsWithoutWindows = psWidget;
	}

	// Find the next handle position
	psBucketPtr->u8ActiveWidgetCount++;
	if (psBucketPtr->u8ActiveWidgetCount != (1 << BUCKET_ITEM_BIT_SIZE))
	{
		UINT32 u32Loop = 0;

		// Loop around until we find an empty spot
		while (psBucketPtr->psWidget[psBucketPtr->u32HandlePosition] && (u32Loop < (1 << BUCKET_ITEM_BIT_SIZE)))
		{
			psBucketPtr->u32HandlePosition++;
			if (psBucketPtr->u32HandlePosition >= (1 << BUCKET_ITEM_BIT_SIZE))
			{
				psBucketPtr->u32HandlePosition = 0;
			}

			++u32Loop;
		}

		// We'd better have landed on an empty slot. If not, there's something wrong with the widget count
		GCASSERT(NULL == psBucketPtr->psWidget[psBucketPtr->u32HandlePosition]);
	}

	*peWidgetHandle = eWidgetHandle;
	return(eErr);

errorExit:
	if (sg_psWidgetTypeMethods[eWidgetType]->WidgetTypeFree)
	{
		(void) sg_psWidgetTypeMethods[eWidgetType]->WidgetTypeFree(psWidget);
	}

	(void) WidgetDelete(psWidget);

	if (psBucketPtr)
	{
		psBucketPtr->psWidget[psBucketPtr->u32HandlePosition] = NULL;
	}

	return(eErr);
}

void WidgetPoolDestroy(void)
{
	UINT32 u32Loop = 0;

	for (u32Loop = 0; u32Loop < (sizeof(sg_psWidgetBuckets) / sizeof(sg_psWidgetBuckets[0])); u32Loop++)
	{
		while (sg_psWidgetBuckets[u32Loop])
		{
			UINT32 u32Loop2;
			SWidgetBucket *psWidgetBucket = NULL;

			for (u32Loop2 = 0; u32Loop2 < (1 << BUCKET_ITEM_BIT_SIZE); u32Loop2++)
			{
				// If this asserts, then someone, somewhere, forgot to clean up a widget and that shouldn't happen
				GCASSERT(NULL == sg_psWidgetBuckets[u32Loop]->psWidget[u32Loop2]);
			}

			psWidgetBucket = sg_psWidgetBuckets[u32Loop];
			sg_psWidgetBuckets[u32Loop] = sg_psWidgetBuckets[u32Loop]->psNextLink;
			GCFreeMemory(psWidgetBucket);
		}
	}
}

ELCDErr WidgetGetPointerByHandle(WIDGETHANDLE eHandle,
								 EWidgetType eExpectedType,
								 SWidget **ppsWidget,
								 SWidgetBucket **ppsBucket)
{
	EWidgetType eType;
	UINT32 u32Bucket = 0;
	UINT32 u32Slot = 0;
	SWidgetBucket *psBucketPtr;

	if (HANDLE_INVALID == eHandle)
	{
		return(LERR_BAD_HANDLE);
	}

	eType = eHandle >> BUCKET_TYPE_POS;
	u32Bucket = (eHandle >> BUCKET_ITEM_BIT_SIZE) & ((1 << (BUCKET_TYPE_POS - BUCKET_ITEM_BIT_SIZE)) - 1);
	u32Slot = (eHandle & ((1 << BUCKET_ITEM_BIT_SIZE) - 1));

	// If the type is out of range, bail
	if ((WIDGET_WTF == eExpectedType) || (eExpectedType >= WIDGET_MAX))
	{
		GCASSERT(0);
	}

	// If the type is out of range, bail
	if ((WIDGET_WTF == eType) || (eType >= WIDGET_MAX))
	{
		GCASSERT(0);
	}

	psBucketPtr = sg_psWidgetBuckets[eType];
	while (psBucketPtr && u32Bucket)
	{
		u32Bucket--;
		psBucketPtr = psBucketPtr->psNextLink;
	}

	// Bucket better not be NULL!
	GCASSERT(psBucketPtr);

	// Slot better not be NULL!
	if (NULL == psBucketPtr->psWidget[u32Slot])
	{
		// Bad handle!
		return(sg_psWidgetTypeMethods[eExpectedType]->eBadHandleError);
	}

	// Check the widget type
	if (eExpectedType != WIDGET_MAX)
	{
		if (psBucketPtr->psWidget[u32Slot]->eWidgetType != eExpectedType)
		{
			return(sg_psWidgetTypeMethods[eExpectedType]->eBadHandleError);
		}
	}

	// All good.
	if (ppsWidget)
	{
		*ppsWidget = psBucketPtr->psWidget[u32Slot];
	}

	if (ppsBucket)
	{
		*ppsBucket = psBucketPtr;
	}

	return(LERR_OK);
}

ELCDErr WidgetDeallocateHandle(WIDGETHANDLE *peWidgetHandle)
{
	ELCDErr eErr;
	SWidgetBucket *psBucket = NULL;
	UINT32 u32Slot = 0;

	eErr = WidgetGetPointerByHandle(*peWidgetHandle,
									WidgetGetHandleType(*peWidgetHandle),
									NULL,
									&psBucket);
	RETURN_ON_FAIL(eErr);
	GCASSERT(psBucket);

	// Got the bucket.
	u32Slot = (*peWidgetHandle & ((1 << BUCKET_ITEM_BIT_SIZE) - 1));
	GCASSERT(u32Slot < (sizeof(psBucket->psWidget) / sizeof(psBucket->psWidget[0])));

	// Null out the pointer
	psBucket->psWidget[u32Slot] = NULL;

	// There had better be at least 1 active widget, otherwise the count is screwed up
	GCASSERT(psBucket->u8ActiveWidgetCount);

	// If this is a completely full bucket, just set the pointer to this slot
	if ((1 << BUCKET_ITEM_BIT_SIZE) == psBucket->u8ActiveWidgetCount)
	{
		psBucket->u32HandlePosition = u32Slot;
	}

	psBucket->u8ActiveWidgetCount--;

	return(LERR_OK);
}


ELCDErr WidgetSetHideByHandle(WIDGETHANDLE eHandle,
							  EWidgetType eExpectedType,
							  BOOL bWidgetHidden,
							  BOOL bForceRepaint)
{
	SWidget *psWidget;
	ELCDErr eErr;

	eErr = WidgetGetPointerByHandle(eHandle,
									eExpectedType,
									&psWidget,
									NULL);

	RETURN_ON_FAIL(eErr);

	return(WidgetSetHide(psWidget,
						 bWidgetHidden,
						 bForceRepaint));
}

EWidgetType WidgetGetHandleType(WIDGETHANDLE eHandle)
{
	if ((eHandle >> BUCKET_TYPE_POS) >= WIDGET_MAX)
	{
		return(WIDGET_WTF);
	}

	return((EWidgetType) (eHandle >> BUCKET_TYPE_POS));
}

ELCDErr WidgetGetPositionByHandle(WIDGETHANDLE eWidgetHandle,
								  INT32 *ps32XPos,
								  INT32 *ps32YPos)
{
	SWidget *psWidget;
	ELCDErr eErr;

	eErr = WidgetGetPointerByHandle(eWidgetHandle,
									WidgetGetHandleType(eWidgetHandle),
									&psWidget,
									NULL);
	RETURN_ON_FAIL(eErr);

	if (ps32XPos)
	{
		*ps32XPos = psWidget->s32XPos;
	}

	if (ps32YPos)
	{
		*ps32YPos = psWidget->s32YPos;
	}

	return(eErr);
}

ELCDErr WidgetGetSizeByHandle(WIDGETHANDLE eWidgetHandle,
							  UINT32 *pu32XSize,
							  UINT32 *pu32YSize)
{
	SWidget *psWidget;
	ELCDErr eErr;

	eErr = WidgetGetPointerByHandle(eWidgetHandle,
									WidgetGetHandleType(eWidgetHandle),
									&psWidget,
									NULL);
	RETURN_ON_FAIL(eErr);

	if (pu32XSize)
	{
		*pu32XSize = psWidget->u32XSize;
	}

	if (pu32YSize)
	{
		*pu32YSize = psWidget->u32YSize;
	}

	return(eErr);
}

void WidgetKeyHit(EGCCtrlKey eGCKey, LEX_CHAR eKey, BOOL bPressed)
{
	EGCResultCode eResult;

	eResult = WindowKeyHit(eGCKey, eKey, bPressed);
	GCASSERT(GC_OK == eResult);
}

ELCDErr WidgetSetPositionByHandle(WIDGETHANDLE eWidgetHandle,
								  EWidgetType eExpectedType,
								  INT32 s32XPos,
								  INT32 s32YPos)
{
	SWidget *psWidget;
	ELCDErr eErr;

	eErr = WidgetGetPointerByHandle(eWidgetHandle,
									eExpectedType,
									&psWidget,
									NULL);
	RETURN_ON_FAIL(eErr);

	// If the widget isn't visible, then simply set the new coordinates and exit
	if (psWidget->bWidgetHidden)
	{
		psWidget->s32XPos = s32XPos;
		psWidget->s32YPos = s32YPos;
		return(LERR_OK);
	}

	// If it's the same position as last time, just return
	if ((psWidget->s32XPos == s32XPos) &&
		(psWidget->s32YPos == s32YPos))
	{
		return(LERR_OK);
	}

	if (psWidget->eWidgetType != WIDGET_TOUCH)
	{
		eErr = WindowLockByHandle(psWidget->eParentWindow);
		if (eErr != LERR_OK)
		{
			goto errorExit;
		}

		// We need to erase the old position first
		WidgetErase(psWidget);

		// Wipe out the intersections
		WidgetEraseIntersections(psWidget);

		// Repaint the intersections
		WidgetPaintIntersections(psWidget);
	}

	// Now set the new position
	psWidget->s32XPos = s32XPos;
	psWidget->s32YPos = s32YPos;

	if (psWidget->eWidgetType != WIDGET_TOUCH)
	{
		// Recalculate the widgets
		eErr = WidgetCalcIntersections(psWidget);

		// Now draw it
		WidgetPaint(psWidget,
					FALSE);

		if (LERR_OK == eErr)
		{
			eErr = WindowUnlockByHandle(psWidget->eParentWindow);
		}
		else
		{
			(void) WindowUnlockByHandle(psWidget->eParentWindow);
		}

		// Commit it!
		WindowUpdateRegionCommit();
	}

errorExit:
	return(eErr);
}

ELCDErr WidgetDestroyByHandle(WIDGETHANDLE *peWidgetHandle,
							  EWidgetType eExpectedType)
{
	SWidget *psWidget;
	ELCDErr eErr;

	eErr = WidgetGetPointerByHandle(*peWidgetHandle,
									eExpectedType,
									&psWidget,
									NULL);
	RETURN_ON_FAIL(eErr);

	eErr = WidgetDelete(psWidget);
	RETURN_ON_FAIL(eErr);

	// Go deallocate the handle
	eErr = WidgetDeallocateHandle(peWidgetHandle);
	RETURN_ON_FAIL(eErr);

	// Invalidate the handle
	*peWidgetHandle = HANDLE_INVALID;

	// All good!
	return(LERR_OK);
}

ELCDErr WidgetSetSizeByHandle(WIDGETHANDLE eWidgetHandle,
							  UINT32 u32XSize,
							  UINT32 u32YSize,
							  BOOL bWindowLock,
							  BOOL bForceRepaint)
{
	SWidget *psWidget;
	ELCDErr eErr;

	eErr = WidgetGetPointerByHandle(eWidgetHandle,
									WidgetGetHandleType(eWidgetHandle),
									&psWidget,
									NULL);
	RETURN_ON_FAIL(eErr);

	eErr = WidgetSetSize(psWidget,
						 u32XSize,
						 u32YSize,
						 bWindowLock,
						 bForceRepaint,
						 TRUE);

	return(eErr);
}

ELCDErr WidgetIsVisible(WIDGETHANDLE eWidgetHandle,
					    BOOL *pbVisible)
{
	SWidget *psWidget;
	ELCDErr eErr;

	eErr = WidgetGetPointerByHandle(eWidgetHandle,
									WidgetGetHandleType(eWidgetHandle),
									&psWidget,
									NULL);
	RETURN_ON_FAIL(eErr);
	GCASSERT(psWidget);
	
	if (pbVisible)
	{
		if (psWidget->bWidgetHidden)
		{
			*pbVisible = FALSE;
		}
		else
		{
			*pbVisible = TRUE;
		}
	}

	return(LERR_OK);
}

ELCDErr WidgetHit(WIDGETHANDLE eWidgetHandle,
				  BOOL *pbCurrentlyPressed,
				  BOOL *pbSelectedSinceLastRead)
{
	SWidget *psWidget;
	ELCDErr eErr;

	eErr = WidgetGetPointerByHandle(eWidgetHandle,
									WidgetGetHandleType(eWidgetHandle),
									&psWidget,
									NULL);
	RETURN_ON_FAIL(eErr);
	GCASSERT(psWidget);

	if (pbCurrentlyPressed)
	{
		*pbCurrentlyPressed = psWidget->bCurrentlyPressed;
	}

	if (pbSelectedSinceLastRead)
	{
		*pbSelectedSinceLastRead = psWidget->bSelectedSinceLastRead;
	}

	psWidget->bSelectedSinceLastRead = FALSE;
	return(LERR_OK);
}

ELCDErr WidgetSetName(WIDGETHANDLE eWidgetHandle,
					  char *pu8WidgetName)
{
	SWidget *psWidget;
	ELCDErr eErr;

	eErr = WidgetGetPointerByHandle(eWidgetHandle,
									WidgetGetHandleType(eWidgetHandle),
									&psWidget,
									NULL);
	RETURN_ON_FAIL(eErr);
	GCASSERT(psWidget);

	if (NULL == pu8WidgetName)
	{
		pu8WidgetName = DEFAULT_WIDGET_NAME;
	}

	psWidget->pu8WidgetName = pu8WidgetName;
	return(LERR_OK);
}

ELCDErr WidgetGetName(WIDGETHANDLE eWidgetHandle,
					  char **ppu8WidgetName)
{
	SWidget *psWidget;
	ELCDErr eErr;

	eErr = WidgetGetPointerByHandle(eWidgetHandle,
									WidgetGetHandleType(eWidgetHandle),
									&psWidget,
									NULL);
	RETURN_ON_FAIL(eErr);
	GCASSERT(psWidget);
	if (ppu8WidgetName)
	{
		*ppu8WidgetName = psWidget->pu8WidgetName;
	}

	return(LERR_OK);
}

ELCDErr WidgetGetImageGroupByWidget(SVar *psWidgetHandle,
									SImageGroup **ppsImageGroup)
{
	ELCDErr eErr;
	WIDGETHANDLE eHandle;
	EWidgetType eWidgetType;
	SWidget *psWidget;

	// Figure out what type it is and stop appropriately
	if (EVAR_IMAGEHANDLE == psWidgetHandle->eType)
	{
		// Image type! Let's go do it!
		eHandle = (WIDGETHANDLE) psWidgetHandle->uValue.eImageWidgetHandle;
		eWidgetType = WIDGET_IMAGE;
	}
	else
	if (EVAR_BUTTONHANDLE == psWidgetHandle->eType)
	{
		// Button type!
		eHandle = (WIDGETHANDLE) psWidgetHandle->uValue.eButtonHandle;
		eWidgetType = WIDGET_BUTTON;
	}
	else
	{
		// Should have been caught by the parser
		GCASSERT(0);
	}

	// Go look up the handle
	eErr = WidgetGetPointerByHandle(eHandle,
									eWidgetType,
									&psWidget,
									NULL);

	if (eErr != LERR_OK)
	{
		return(eErr);
	}

	// Okay, got the widget. Time for the image group!
	if (WIDGET_IMAGE == eWidgetType)
	{
		*ppsImageGroup = psWidget->uWidgetSpecific.psImageWidget->psImageGroup;
	}
	else
	if (WIDGET_BUTTON == eWidgetType)
	{
		// Need to figure out how to do this for normal/pressed images
		GCASSERT(0);
//		*ppsImageGroup = psWidget->uWidgetSpecific.psButton->
	}
	else
	{
		// Invalid!
		GCASSERT(0);
	}

	return(LERR_OK);
}

ELCDErr WidgetSetUserData(WIDGETHANDLE eWidgetHandle,
						  void *pvUserData)
{
	SWidget *psWidget;
	ELCDErr eErr;

	eErr = WidgetGetPointerByHandle(eWidgetHandle,
									WidgetGetHandleType(eWidgetHandle),
									&psWidget,
									NULL);
	RETURN_ON_FAIL(eErr);
	GCASSERT(psWidget);

	psWidget->pvUserData = pvUserData;
	return(LERR_OK);
}

ELCDErr WidgetGetUserData(WIDGETHANDLE eWidgetHandle,
						  void **ppvUserData)
{
	SWidget *psWidget;
	ELCDErr eErr;

	eErr = WidgetGetPointerByHandle(eWidgetHandle,
									WidgetGetHandleType(eWidgetHandle),
									&psWidget,
									NULL);
	RETURN_ON_FAIL(eErr);
	GCASSERT(psWidget);

	if (ppvUserData)
	{
		*ppvUserData = psWidget->pvUserData;
	}

	return(LERR_OK);
}

void WidgetShutdown(void)
{
	EGCResultCode eResult;

	// Shut down the widget pool
	WidgetPoolDestroy();

	// Causes the widget manager to shut down
	eResult = GCOSQueueSend(sg_sWidgetQueue,
							(void *) (WCMD_WIDGET_SHUTDOWN));
	GCASSERT(GC_OK == eResult);

	// This will cause the pointer thread to shut down
	sg_bWidgetShutdown = TRUE;
}

ELCDErr WidgetUpdatePreamble(WIDGETHANDLE eWidgetHandle,
							 SWidget **ppsWidget)
{
	ELCDErr eErr;
	SWidget *psWidget;

	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eWidgetHandle,
									WIDGET_TEXT,
									&psWidget,
									NULL);

	RETURN_ON_FAIL(eErr);

	eErr = WindowLockByHandle(psWidget->eParentWindow);
	if (eErr != LERR_OK)
	{
		goto errorExit;
	}

	// If there's a widget handle, let the caller know
	if (ppsWidget)
	{
		*ppsWidget = psWidget;
	}

	// Wipe out the widget
	WidgetErase(psWidget);

	// Wipe out the intersections
	WidgetEraseIntersections(psWidget);

	// Repaint the intersections
	WidgetPaintIntersections(psWidget);

errorExit:
	return(eErr);
}

ELCDErr WidgetUpdateFinished(WIDGETHANDLE eWidgetHandle)
{
	ELCDErr eErr;
	SWidget *psWidget;

	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eWidgetHandle,
									WIDGET_TEXT,
									&psWidget,
									NULL);

	RETURN_ON_FAIL(eErr);

	// Calculate the new intersections
	eErr = WidgetCalcIntersections(psWidget);

	// Now draw the widget
	WidgetPaint(psWidget,
				FALSE);


	if (LERR_OK == eErr)
	{
		eErr = WindowUnlockByHandle(psWidget->eParentWindow);
	}
	else
	{
		(void) WindowUnlockByHandle(psWidget->eParentWindow);
	}

	// Commit it
	WindowUpdateRegionCommit();

	return(eErr);
}

ELCDErr WidgetMousedOver(WIDGETHANDLE eWidgetHandle,
								BOOL *pbMousedOver)
{
	ELCDErr eErr;
	SWidget *psWidget;

	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eWidgetHandle,
									WIDGET_TEXT,
									&psWidget,
									NULL);

	RETURN_ON_FAIL(eErr);

	if (pbMousedOver)
	{
		*pbMousedOver = psWidget->bMousedOver;
	}

	return(eErr);
}

ELCDErr WidgetGetWidgetSpecific(WIDGETHANDLE eWidgetHandle,
								EWidgetType eType,
								void **ppvWidgetSpecific,
								SWidget **ppsWidget)
{
	ELCDErr eLCDErr;
	SWidget *psWidget;

	// Go get the combo box
	eLCDErr = WidgetGetPointerByHandle((WIDGETHANDLE) eWidgetHandle,
									   eType,
									   &psWidget,
									   NULL);
	RETURN_ON_FAIL(eLCDErr);

	// If the caller wants the widget pointer, get it
	if (ppsWidget)
	{
		*ppsWidget = psWidget;
	}

	// Copy in our appropriate pointer
	if (WIDGET_COMBOBOX == eType)
	{
		*ppvWidgetSpecific = (void *) psWidget->uWidgetSpecific.psComboBox;
	}
	else
	{
		// Type not yet supported
		GCASSERT(0);
	}

	return(LERR_OK);
}

ELCDErr WidgetSetParent(WIDGETHANDLE eWidgetHandle,
						EWidgetType eType,
						WIDGETHANDLE eParentWidget)
{
	ELCDErr eLCDErr;
	SWidget *psWidget;

	// Go get the combo box
	eLCDErr = WidgetGetPointerByHandle((WIDGETHANDLE) eWidgetHandle,
									   eType,
									   &psWidget,
									   NULL);
	RETURN_ON_FAIL(eLCDErr);

	psWidget->eParentWidget = eParentWidget;
	return(LERR_OK);
}

ELCDErr WidgetRegisterCallback(WIDGETHANDLE eWidgetHandle,
							   UINT32 u32WidgetMask,
							   void (*Callback)(WIDGETHANDLE eWidgetHandle,
												UINT32 u32WidgetMask,
												UWidgetCallbackData *puWidgetSpecificData))
{
	ELCDErr eLCDErr;
	SWidget *psWidget;
	SWidgetCallbacks *psCallback = NULL;

	// Go get the combo box
	eLCDErr = WidgetGetPointerByHandle((WIDGETHANDLE) eWidgetHandle,
									   WidgetGetHandleType((WIDGETHANDLE) eWidgetHandle),
									   &psWidget,
									   NULL);
	RETURN_ON_FAIL(eLCDErr);

	psCallback = MemAlloc(sizeof(*psCallback));
	if (NULL == psCallback)
	{
		return(LERR_NO_MEM);
	}

	psCallback->pCallback = Callback;
	psCallback->u32Mask = u32WidgetMask;
	psCallback->psNextLink = psWidget->psCallbacks;
	psWidget->psCallbacks = psCallback;

	return(LERR_OK);
}

ELCDErr WidgetUnregisterCallback(WIDGETHANDLE eWidgetHandle,
										UINT32 u32WidgetMask,
										void (*Callback)(WIDGETHANDLE eWidgetHandle,
														 UINT32 u32WidgetMask,
														 UWidgetCallbackData *puWidgetSpecificData))
{
	ELCDErr eLCDErr;
	SWidget *psWidget;
	SWidgetCallbacks *psCallback = NULL;
	SWidgetCallbacks *psCallbackPrior = NULL;

	// Go get the combo box
	eLCDErr = WidgetGetPointerByHandle((WIDGETHANDLE) eWidgetHandle,
									   WidgetGetHandleType((WIDGETHANDLE) eWidgetHandle),
									   &psWidget,
									   NULL);
	RETURN_ON_FAIL(eLCDErr);

	psCallback = psWidget->psCallbacks;
	while (psCallback)
	{
		if ((u32WidgetMask == psCallback->u32Mask) &&
			(psCallback->pCallback == Callback))
		{
			// Found it!
			break;
		}

		psCallbackPrior = psCallback;
		psCallback = psCallback->psNextLink;
	}

	if (NULL == psCallback)
	{
		// Means we didn't find it
		return(LERR_WIDGET_CALLBACK_NOT_FOUND);
	}

	// Found it! Unlink it from the list
	if (psCallbackPrior)
	{
		psCallbackPrior->psNextLink = psCallback->psNextLink;
	}
	else
	{
		GCASSERT(psWidget->psCallbacks == psCallback);
		psWidget->psCallbacks = psCallback->psNextLink;
	}

	// Unlinked. Free it!
	GCFreeMemory(psCallback);

	return(LERR_OK);
}

void WidgetBroadcastMask(WIDGETHANDLE eWidgetHandle,
						 UINT32 u32Mask,
						 UWidgetCallbackData *puWidgetSpecificData)
{
	ELCDErr eLCDErr;
	SWidget *psWidget;
	SWidgetCallbacks *psCallbacks;

	// Go get the combo box
	eLCDErr = WidgetGetPointerByHandle(eWidgetHandle,
									   WidgetGetHandleType(eWidgetHandle),
									   &psWidget,
									   NULL);
	GCASSERT(LERR_OK == eLCDErr);

	psCallbacks = psWidget->psCallbacks;

	// Run through all of the callbacks for this widget and see if we have any
	// consumers that want the data.
	while (psCallbacks)
	{
		// If this mask matches, then call the callback routine
		if (u32Mask & psCallbacks->u32Mask)
		{
			psCallbacks->pCallback(eWidgetHandle,
								   u32Mask,
								   puWidgetSpecificData);
		}

		psCallbacks = psCallbacks->psNextLink;
	}
}

ELCDErr WidgetSetEnable(WIDGETHANDLE eWidgetHandle,
						BOOL bEnabled)
{
	SWidget *psWidget;
	ELCDErr eLCDErr;

	eLCDErr = WidgetGetPointerByHandle(eWidgetHandle,
									   WidgetGetHandleType(eWidgetHandle),
									   &psWidget,
									   NULL);
	if (eLCDErr != LERR_OK)
	{
		return(eLCDErr);
	}

	psWidget->bWidgetEnabled = bEnabled;

	// Invert it since we're setting a DISABLE
	if (bEnabled)
	{
		bEnabled = FALSE;
	}
	else
	{
		bEnabled = TRUE;
	}

	// If the widget has a disable method, then call it
	if (psWidget->psWidgetFunc->WidgetSetDisable)
	{

		psWidget->psWidgetFunc->WidgetSetDisable(psWidget,
												 bEnabled);
	}

	return(LERR_OK);
}


ELCDErr WidgetIsMouseover(WIDGETHANDLE eWidgetHandle,
						  BOOL *pbMouseover)
{
	ELCDErr eLCDErr;
	SWidget *psWidget;

	// Go get the combo box
	eLCDErr = WidgetGetPointerByHandle(eWidgetHandle,
									   WidgetGetHandleType(eWidgetHandle),
									   &psWidget,
									   NULL);
	if (LERR_OK == eLCDErr)
	{
		if (pbMouseover)
		{
			*pbMouseover = psWidget->bMousedOver;
		}
	}

	return(eLCDErr);
}

ELCDErr WidgetGetParent(WIDGETHANDLE eWidgetHandle,
						EWidgetType eType,
						WIDGETHANDLE *peParentWidget)
{
	ELCDErr eLCDErr;
	SWidget *psWidget;

	// Go get the combo box
	eLCDErr = WidgetGetPointerByHandle(eWidgetHandle,
									   WidgetGetHandleType(eWidgetHandle),
									   &psWidget,
									   NULL);
	if (LERR_OK == eLCDErr)
	{
		if (peParentWidget)
		{
			*peParentWidget = (WIDGETHANDLE) psWidget->eParentWidget;
		}
	}

	return(eLCDErr);
}

ELCDErr WidgetSetMouseoverDisable(WIDGETHANDLE eWidgetHandle,
								  BOOL bMouseoverDisabled)
{
	ELCDErr eLCDErr;
	SWidget *psWidget;

	// Go get the combo box
	eLCDErr = WidgetGetPointerByHandle(eWidgetHandle,
									   WidgetGetHandleType(eWidgetHandle),
									   &psWidget,
									   NULL);
	if (LERR_OK == eLCDErr)
	{
		psWidget->bMouseOverDisabled = bMouseoverDisabled;
	}

	return(eLCDErr);
}

