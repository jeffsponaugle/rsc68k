#include "Startup/app.h"
#include "Libs/Gfx/GraphicsLib.h"
#include "Libs/window/window.h"
#include "Libs/FontMgr/FontMgr.h"
#include "Libs/widget/widget.h"

static BOOL sg_bEmergencyUpdate = FALSE;

// The head of all console widgets
static SConsole *sg_psConsoleWidgetHead;

// Timer object for deferred console updates
static STimerObject *sg_psConsoleTimer;

static void ConsoleLock(SConsole *psConsole)
{
	psConsole->bUpdateInProgress = TRUE;
}

static void ConsoleUnlock(SConsole *psConsole)
{
	psConsole->bUpdateInProgress = FALSE;
}

static ELCDErr ConsoleWidgetAlloc(SWidget *psWidget,
								  WIDGETHANDLE eHandle)
{
	GCASSERT(psWidget);
	psWidget->uWidgetSpecific.psConsole = MemAlloc(sizeof(*psWidget->uWidgetSpecific.psConsole));
	if (NULL == psWidget->uWidgetSpecific.psConsole)
	{
		return(LERR_NO_MEM);
	}
	else
	{
		psWidget->uWidgetSpecific.psConsole->psWidget = psWidget;
		psWidget->uWidgetSpecific.psConsole->eConsoleHandle = (CONSOLEHANDLE) eHandle;
		return(LERR_OK);
	}
}

static ELCDErr ConsoleWidgetFree(SWidget *psWidget)
{
	SConsole *psConsoleWidgetPtr = NULL;
	SConsole *psConsoleWidgetPrior = NULL;

	GCASSERT(psWidget);

	if (NULL == psWidget->uWidgetSpecific.psConsole)
	{
		return(LERR_OK);
	}

	// Take ourselves out of the master list
	psConsoleWidgetPtr = sg_psConsoleWidgetHead;

	// Take ourselves out of the master console widget linked list
	while (psConsoleWidgetPtr && psConsoleWidgetPtr != psWidget->uWidgetSpecific.psConsole)
	{
		psConsoleWidgetPrior = psConsoleWidgetPtr;
		psConsoleWidgetPtr = psConsoleWidgetPtr->psNextLink;
	}

	GCASSERT(psConsoleWidgetPtr);
	GCASSERT(psConsoleWidgetPtr == psWidget->uWidgetSpecific.psConsole);

	if (NULL == psConsoleWidgetPrior)
	{
		sg_psConsoleWidgetHead = psConsoleWidgetPtr->psNextLink;
	}
	else
	{
		psConsoleWidgetPrior->psNextLink = psConsoleWidgetPtr->psNextLink;
	}

	// Free the character array
	if (psWidget->uWidgetSpecific.psConsole->peCharacters)
	{
		GCFreeMemory(psWidget->uWidgetSpecific.psConsole->peCharacters);
		psWidget->uWidgetSpecific.psConsole->peCharacters = NULL;
	}

	// Free the background colors
	if (psWidget->uWidgetSpecific.psConsole->pu32CharacterColors)
	{
		GCFreeMemory(psWidget->uWidgetSpecific.psConsole->pu32CharacterColors);
		psWidget->uWidgetSpecific.psConsole->pu32CharacterColors = NULL;
	}

	// Free the translucency list
	if (psWidget->uWidgetSpecific.psConsole->pu8TransBackground)
	{
		GCFreeMemory(psWidget->uWidgetSpecific.psConsole->pu8TransBackground);
		psWidget->uWidgetSpecific.psConsole->pu8TransBackground = NULL;
	}

	// Clear out our font handle
	FontFree(psWidget->uWidgetSpecific.psConsole->eFontHandle);

	GCFreeMemory(psWidget->uWidgetSpecific.psConsole);
	psWidget->uWidgetSpecific.psConsole = NULL;

	return(LERR_OK);
}


static void ConsoleRepaint(struct SWidget *psWidget,
						   BOOL bLock)
{
	UINT32 u32Loop = 0;
	SConsole *psConsole;
	INT32 s32XPos = 0;
	INT32 s32YPos = 0;
	UINT32 u32XCharPos = 0;
	ELCDErr eLCDErr;
	LEX_CHAR *peCharacterPos;
	UINT32 *pu32CharacterColorsPos;
	UINT8 *pu8TransBackground;
	SFont *psFont = NULL;
	BOOL bErasure = FALSE;

	psConsole = psWidget->uWidgetSpecific.psConsole;

	// Lock it if we eed it
	if (bLock)
	{
		eLCDErr = WindowLockByHandle(psConsole->psWidget->eParentWindow);
		GCASSERT(LERR_OK == eLCDErr);
	}

	if (psConsole->bTransBackground)
	{
		// Erase the widget's bounding area
		WidgetEraseStandard(psWidget);
	}
	else
	{
		// If we currently have a background color set, fill in the window with the
		// background color

		GCASSERT(psConsole->psWidget->u32XSize);;
		GCASSERT(psConsole->psWidget->u32YSize);

		eLCDErr = WindowColorFillRegion(psConsole->psWidget->eParentWindow,
										psConsole->u16TextBackground,
										psConsole->psWidget->s32XPos,
										psConsole->psWidget->s32YPos,
										(INT32) psConsole->psWidget->u32XSize,
										(INT32) psConsole->psWidget->u32YSize,
										FALSE);

		GCASSERT(LERR_OK == eLCDErr);
		bErasure = TRUE;
	}

	// Paint everything we have that'll fit on the screen

	s32XPos = psConsole->s32XOrigin;
	s32YPos = psConsole->s32YOrigin;

	// Set up our pointers
	peCharacterPos = psConsole->peCharacters;
	pu32CharacterColorsPos = psConsole->pu32CharacterColors;
	pu8TransBackground = psConsole->pu8TransBackground;

	psFont = FontGetPointer(psConsole->eFontHandle);
	GCASSERT(psFont);

	u32Loop = 0;

	// Loop through all lines
	while (u32Loop < psConsole->u32YCharacterSize)
	{
		if ((ROT_0 == psConsole->eOrientation) ||
			(ROT_180 == psConsole->eOrientation))
		{
			s32XPos = psConsole->s32XOrigin;
		}
		else
		if ((ROT_90 == psConsole->eOrientation) ||
			(ROT_270 == psConsole->eOrientation))
		{
			s32YPos = psConsole->s32YOrigin;
		}
		else
		{
			GCASSERT(0);
		}

		u32XCharPos = 0;

		while ((*peCharacterPos != NO_CHARACTER) && (u32XCharPos < psConsole->u32XCharacterSize))
		{
			INT32 s32XStride;
			INT32 s32YStride;
			INT32 s32XPaint;
			INT32 s32YPaint;

			eLCDErr = FontGetSize(psConsole->eFontHandle,
								  (TEXTCHAR) *peCharacterPos,
								  NULL,
								  NULL,
								  &s32XStride,
								  &s32YStride,
								  psConsole->eOrientation,
								  TRUE);

			GCASSERT(LERR_OK == eLCDErr);

			// Adjust X/Y positions
			s32XPaint = (psConsole->psWidget->s32XPos << 6) + s32XPos;
			s32YPaint = (psConsole->psWidget->s32YPos << 6) + s32YPos;

			if (*pu8TransBackground)
			{
				// Don't do anything if we have a transparent background
			}
			else
/*			if (bErasure && ((*pu32CharacterColorsPos) >> 16) != psConsole->u16TextBackground)
			{
				// If we've erased the whole region and the character background color equals
				// what we just erased, then don't draw it
			}
			else */
			{
				INT32 s32XCharSize;
				INT32 s32YCharSize;

				// Otherwise, fill it
				// Assume ROT 0 origin

				if (ROT_0 == psConsole->eOrientation)
				{
					s32XCharSize = s32XStride;
					s32YCharSize = (INT32) psConsole->u32CharYMax;
				}
				else
				if (ROT_180 == psConsole->eOrientation)
				{
					s32XCharSize = s32XStride;
					s32YCharSize = (INT32) psConsole->u32CharYMax;
					s32XPaint += s32XStride;
					s32YPaint -= ((INT32) psConsole->u32CharYMax + 0x3f);
				}
				else
				if (ROT_90 == psConsole->eOrientation)
				{
					s32XCharSize = (INT32) psConsole->u32CharYMax;
					s32YCharSize = s32YStride;
					s32XPaint -= s32XCharSize;
				}
				else
				if (ROT_270 == psConsole->eOrientation)
				{
					s32XCharSize = (INT32) psConsole->u32CharYMax;
					s32YCharSize = s32YStride;
					s32YPaint -= s32XCharSize;
				}
				else
				{
					GCASSERT(0);
				}

				s32XCharSize = (abs(s32XCharSize) + 0x3f) >> 6;
				s32YCharSize = (abs(s32YCharSize) + 0x3f) >> 6;

				// This will draw a solid where the character will go. Adding the active
				// area offset isn't needed since that's done by the fill routine
				eLCDErr = WindowColorFillRegion(psConsole->psWidget->eParentWindow,
												(*pu32CharacterColorsPos) >> 16,
												s32XPaint >> 6,
												s32YPaint >> 6,
												s32XCharSize,
												s32YCharSize,
												FALSE);
				GCASSERT(LERR_OK == eLCDErr);
			}

			s32XPaint = (psConsole->psWidget->s32XPos << 6) + s32XPos;
			s32YPaint = (psConsole->psWidget->s32YPos << 6) + s32YPos;

			if (ROT_0 == psConsole->eOrientation)
			{
				// Get our X/Y paint coordinates
			}
			else
			if (ROT_180 == psConsole->eOrientation)
			{
				s32XPaint += s32XStride;
				s32YPaint -= ((INT32) psConsole->u32CharYMax + 0x3f);
			}
			else
			if (ROT_90 == psConsole->eOrientation)
			{
				s32XPaint -= (INT32) psConsole->u32CharYMax;
			}
			else
			if (ROT_270 == psConsole->eOrientation)
			{
				s32YPaint += s32YStride;
			}
			else
			{
				GCASSERT(0);
			}

			DebugOut("ConsoleRepaint: X=%u, Y=%u, charx=%u, chary=%u\n", s32XPos >> 6, s32YPos >> 6, u32XCharPos, u32Loop);

			// Now go render the character
			eLCDErr = FontRender(psConsole->eFontHandle,
								 psConsole->psWidget->eParentWindow,
								 s32XPaint,
								 s32YPaint,
								 -1,
								 -1,
								 *peCharacterPos,
								 (UINT16) (*pu32CharacterColorsPos),
								 FALSE,
								 NULL,
								 NULL,
								 FALSE,
								 psConsole->eOrientation,
								 NULL,
								 FALSE,
								 TRUE);

			if ((ROT_0 == psConsole->eOrientation) ||
				(ROT_180 == psConsole->eOrientation))
			{
				s32XPos += s32XStride;
			}
			else
			{
				s32YPos += s32YStride;
			}

			++peCharacterPos;
			++pu32CharacterColorsPos;
			++pu8TransBackground;
			++u32XCharPos;
		}

		u32Loop++;

		if (ROT_0 == psConsole->eOrientation)
		{
			s32YPos += (INT32) psFont->u32MaxY;
		}
		else
		if (ROT_180 == psConsole->eOrientation)
		{
			s32YPos -= (INT32) psFont->u32MaxY;
		}
		else
		if (ROT_90 == psConsole->eOrientation)
		{
			s32XPos -= (INT32) psFont->u32MaxY;
		}
		else
		if (ROT_270 == psConsole->eOrientation)
		{
			s32XPos += (INT32) psFont->u32MaxY;
		}
		else
		{
			GCASSERT(0);
		}

		peCharacterPos = &psConsole->peCharacters[u32Loop * psConsole->u32XCharacterSize];
		pu32CharacterColorsPos = &psConsole->pu32CharacterColors[u32Loop * psConsole->u32XCharacterSize];
		pu8TransBackground = &psConsole->pu8TransBackground[u32Loop * psConsole->u32XCharacterSize];
	}

	// Update the whole widget
	WindowUpdateRegion(psWidget->eParentWindow,
					   (UINT32) (psConsole->psWidget->s32XPos),
					   (UINT32) (psConsole->psWidget->s32YPos),
					   (UINT32) psConsole->psWidget->u32XSize,
					   (UINT32) psConsole->psWidget->u32YSize);


	if (bLock)
	{
		eLCDErr = WindowUnlockByHandle(psConsole->psWidget->eParentWindow);
		GCASSERT(LERR_OK == eLCDErr);
	}
}

static void ConsoleErase(SWidget *psWidget)
{
	WidgetEraseStandard(psWidget);
}

static SWidgetFunctions sg_sConsoleFunctions =
{
	NULL,
	ConsoleRepaint,
	ConsoleErase,
	NULL,
	NULL,						// Release
	NULL,						// Mouseover
	NULL,						// Focus
	NULL,						// Keystroke for us
	NULL,						// Animation tick - none for now!
	NULL,
	NULL,						// Mouse wheel
	NULL						// Set disable
};

static SWidgetTypeMethods sg_sConsoleMethods = 
{
	&sg_sConsoleFunctions,
	LERR_CONSOLE_BAD_HANDLE,			// Error when it's not the handle we're looking for
	ConsoleWidgetAlloc,
	ConsoleWidgetFree
};

#define	CONSOLE_TIMER_INTERVAL	5

#define	CONSOLE_LOW_GUARD_TIME	15			// Roughly 60FPS
#define CONSOLE_HIGH_GUARD_TIME	50			// 20FPS

static void ConsoleTimerCallback(UINT32 u32Param)
{
	SConsole *psConsole = sg_psConsoleWidgetHead;

	while (psConsole)
	{
		if (psConsole->bUpdateTimersRunning)
		{
			psConsole->u32MSSinceLastPrint += CONSOLE_TIMER_INTERVAL;
			psConsole->u32MSSinceLastPrintRequest += CONSOLE_TIMER_INTERVAL;

			if ((psConsole->u32MSSinceLastPrint > CONSOLE_LOW_GUARD_TIME) &&
				(FALSE == sg_bEmergencyUpdate))
			{
				// Time has passed since last print - update!
				if (LERR_OK == WindowDepositMessage(WCMD_WINDOW_CONSOLE_UPDATE | psConsole->eConsoleHandle))
				{
					psConsole->bUpdateTimersRunning = FALSE;
					psConsole->u32MSSinceLastPrint = 0;
					psConsole->u32MSSinceLastPrintRequest = 0;
				}
			}
			else
			if (psConsole->u32MSSinceLastPrintRequest > CONSOLE_HIGH_GUARD_TIME)
			{
				if (FALSE == sg_bEmergencyUpdate)
				{
					if (LERR_OK == WindowDepositMessage(WCMD_WINDOW_CONSOLE_UPDATE | psConsole->eConsoleHandle))
					{
						sg_bEmergencyUpdate = TRUE;
						psConsole->bUpdateTimersRunning = FALSE;
						psConsole->u32MSSinceLastPrintRequest = 0;
						psConsole->u32MSSinceLastPrint = 0;
					}
				}
			}
		}

		psConsole = psConsole->psNextLink;
	}
}

static ELCDErr ConsoleTriggerUpdate(CONSOLEHANDLE eConsoleHandle)
{
	SConsole *psConsole;
	SWidget *psWidget;
	ELCDErr eLCDErr;

	eLCDErr = WidgetGetPointerByHandle((WIDGETHANDLE) eConsoleHandle,
										WIDGET_CONSOLE,
										&psWidget,
										NULL);

	RETURN_ON_FAIL(eLCDErr);

	psConsole = psWidget->uWidgetSpecific.psConsole;
	GCASSERT(psConsole);

	// If widget is hidden, just return. No need to draw.
	if ((psWidget->bWidgetHidden) ||  (psConsole->bUpdateInProgress))
	{
		sg_bEmergencyUpdate = FALSE;
		return(LERR_OK);
	}

	// Let's see what's going on timer-wise with the parent
	if (psConsole->bUpdateTimersRunning)
	{
		// Reset our OCD timer
		psConsole->u32MSSinceLastPrint = 0;
		if (FALSE == psConsole->bUpdateTimersRunning)
		{
			// Timer not running any longer. Clear everything and restart.
			psConsole->u32MSSinceLastPrint = 0;
		}
	}
	else
	{
		psConsole->u32MSSinceLastPrint = 0;
	}

	psConsole->bUpdateTimersRunning = TRUE;

	// Indicate an emergency update isn't needed
	sg_bEmergencyUpdate = FALSE;

	return(LERR_OK);
}

void ConsoleWidgetUpdate(CONSOLEHANDLE eConsoleHandle)
{
	SWidget *psWidget;
	ELCDErr eErr;

	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eConsoleHandle,
									WIDGET_CONSOLE,
									&psWidget,
									NULL);
	if (eErr != LERR_OK)
	{
		return;
	}

	ConsoleRepaint(psWidget, 
				   TRUE);

	WindowUpdateRegionCommit();
}

void ConsoleFirstTimeInit(void)
{
	EGCResultCode eResult;

	// Don't do anything for a first time console init
	DebugOut("* Initializing console\n");

	// Set up a timer callback every CONSOLE_TIMER_INTERVAL
	eResult = GCTimerCreate(&sg_psConsoleTimer);
	GCASSERT(GC_OK == eResult);
	eResult = GCTimerSetCallback(sg_psConsoleTimer,
								 ConsoleTimerCallback,
								 0);
	GCASSERT(GC_OK == eResult);
	eResult = GCTimerSetValue(sg_psConsoleTimer,
							  CONSOLE_TIMER_INTERVAL,
							  CONSOLE_TIMER_INTERVAL);

	GCASSERT(GC_OK == eResult);
	eResult = GCTimerStart(sg_psConsoleTimer);
	GCASSERT(GC_OK == eResult);

	// Register it with the widget manager
	WidgetRegisterTypeMethods(WIDGET_CONSOLE,
							  &sg_sConsoleMethods);
}

#define	MASK_TRANSLUCENT	0x01	// Character is translucent
#define MASK_CHARACTER		0x02	// Character is present

ELCDErr ConsoleCreate(WINDOWHANDLE eWindowHandle,
					  FONTHANDLE eFontHandle,
					  CONSOLEHANDLE *peConsoleHandle,
					  UINT16 u16Orientation,
					  INT32 s32XPos,
					  INT32 s32YPos,
					  UINT32 u32XSize,
					  UINT32 u32YSize)
{
	SWidget *psWidget = NULL;
	ELCDErr eLCDErr;
	UINT32 u32CharXSize = 0;
	UINT32 u32CharYSize = 0;
	SFont *psFont;
	UINT32 *pu32ColorPtr = NULL;
	UINT32 u32Loop;

	// See if the rotation makes any sense
	if ((u16Orientation != ROT_0) &&
		(u16Orientation != ROT_90) &&
		(u16Orientation != ROT_180) && 
		(u16Orientation != ROT_270))
	{
		return(LERR_CONSOLE_BAD_ROTATION);
	}

	// And now see if the font handle is OK

	psFont = FontGetPointer(eFontHandle);
	if (NULL == psFont)
	{
		return(LERR_FONT_BAD_HANDLE);
	}

	// Convert x/y
	if ((ROT_0 == u16Orientation) || (ROT_180 == u16Orientation))
	{
		// Normal/expected x=x, y=y
		u32CharXSize = (((u32XSize << 6)) / (psFont->u32MinX));
		u32CharYSize = (((u32YSize << 6))/ (psFont->u32MinY));
	}
	else
	{
		// 90/270 - X&Y are swapped
		u32CharXSize = (((u32YSize << 6)) / (psFont->u32MinX));
		u32CharYSize = (((u32XSize << 6))/ (psFont->u32MinY));
	}

	if (0 == u32CharXSize)
	{
		// Means we don't have enough room for an X size
		return(LERR_CONSOLE_BAD_X_SIZE);
	}

	if (0 == u32CharXSize)
	{
		// Means we don't have enough room for a Y size
		return(LERR_CONSOLE_BAD_Y_SIZE);
	}

	// Now go allocate a widget
	eLCDErr = WidgetAllocateHandle((WIDGETHANDLE *) peConsoleHandle,
								   WIDGET_CONSOLE,
								   eWindowHandle,
								   &psWidget);
	RETURN_ON_FAIL(eLCDErr);

	// Store the # of possible characters in each position
	psWidget->uWidgetSpecific.psConsole->u32XCharacterSize = u32CharXSize;
	psWidget->uWidgetSpecific.psConsole->u32YCharacterSize = u32CharYSize;

	// Store the largest characters (for stride purposes)
	psWidget->uWidgetSpecific.psConsole->u32CharXMax = psFont->u32MaxX;
	psWidget->uWidgetSpecific.psConsole->u32CharYMax = psFont->u32MaxY;

	psWidget->uWidgetSpecific.psConsole->eFontHandle = eFontHandle;
	psWidget->uWidgetSpecific.psConsole->u16TextColor = 0xffff;	// White
	psWidget->uWidgetSpecific.psConsole->u16TextBackground = 0;	// Black
	psWidget->uWidgetSpecific.psConsole->bAutoLF = FALSE;		// No automatic linefeed
	psWidget->uWidgetSpecific.psConsole->bAutoCR = FALSE;		// No automatic carriage return
	psWidget->uWidgetSpecific.psConsole->bTransBackground = FALSE; // Solid background by default

	// Allocate the text array
	psWidget->uWidgetSpecific.psConsole->peCharacters = MemAlloc(u32CharXSize * u32CharYSize * sizeof(*psWidget->uWidgetSpecific.psConsole->peCharacters));
	if (NULL == psWidget->uWidgetSpecific.psConsole->peCharacters)
	{
		goto errorExit;
	}

	// Set every character to a 'non character'
	memset((void *) psWidget->uWidgetSpecific.psConsole->peCharacters, 0xff, u32CharXSize * u32CharYSize * sizeof(*psWidget->uWidgetSpecific.psConsole->peCharacters));

	// Now the color array
	psWidget->uWidgetSpecific.psConsole->pu32CharacterColors = MemAlloc(u32CharXSize * u32CharYSize * sizeof(*psWidget->uWidgetSpecific.psConsole->pu32CharacterColors));
	if (NULL == psWidget->uWidgetSpecific.psConsole->pu32CharacterColors)
	{
		goto errorExit;
	}

	// Now the translucency background order
	psWidget->uWidgetSpecific.psConsole->pu8TransBackground = MemAlloc(u32CharXSize * u32CharYSize * sizeof(*psWidget->uWidgetSpecific.psConsole->pu8TransBackground));
	if (NULL == psWidget->uWidgetSpecific.psConsole->pu8TransBackground)
	{
		goto errorExit;
	}

	// Set the entire background transparent by default
	memset((void *) psWidget->uWidgetSpecific.psConsole->pu8TransBackground, MASK_TRANSLUCENT, u32CharXSize * u32CharYSize * sizeof(*psWidget->uWidgetSpecific.psConsole->pu8TransBackground));

	// Set every character to white by default
	u32Loop = u32CharXSize * u32CharYSize;
	pu32ColorPtr = psWidget->uWidgetSpecific.psConsole->pu32CharacterColors;

	while (u32Loop--)
	{
		*pu32ColorPtr = 0x0000ffff;	// Background black, foreground white
		++pu32ColorPtr;
	}

	// Set up initial pointers, etc...
	psWidget->uWidgetSpecific.psConsole->peCharactersPos = (TEXTCHAR *) psWidget->uWidgetSpecific.psConsole->peCharacters;
	psWidget->uWidgetSpecific.psConsole->pu32CharacterColorsPos = psWidget->uWidgetSpecific.psConsole->pu32CharacterColors;
	psWidget->uWidgetSpecific.psConsole->pu8TransBackgroundPos = psWidget->uWidgetSpecific.psConsole->pu8TransBackground;
	psWidget->uWidgetSpecific.psConsole->eOrientation = (ERotation) u16Orientation;

	if (ROT_0 == u16Orientation)
	{
		// Do nothing - 0, 0 is good enough
		psWidget->uWidgetSpecific.psConsole->s32XOrigin = 0;
		psWidget->uWidgetSpecific.psConsole->s32YOrigin = 0;
	}
	else
	if (ROT_90 == u16Orientation)
	{
		// Rotated 90 degress (to the right)
		psWidget->uWidgetSpecific.psConsole->s32XOrigin = (INT32) (u32XSize) << 6;
		psWidget->uWidgetSpecific.psConsole->s32YOrigin = 0;
	}
	else
	if (ROT_180 == u16Orientation)
	{
		// Rotated 180 degrees
		psWidget->uWidgetSpecific.psConsole->s32XOrigin = (INT32) (u32XSize) << 6;
		psWidget->uWidgetSpecific.psConsole->s32YOrigin = (INT32) (u32YSize) << 6;
	}
	else
	if (ROT_270 == u16Orientation)
	{
		// Rotated 270 degrees
		psWidget->uWidgetSpecific.psConsole->s32XOrigin = 0;
		psWidget->uWidgetSpecific.psConsole->s32YOrigin = (INT32) (u32YSize) << 6;
	}
	else
	{
		// WTF? Why didn't the check at the beginning of the procedure catch this?
		GCASSERT(0);
	}

	eLCDErr = WidgetSetSizeByHandle((WIDGETHANDLE) *peConsoleHandle,
									u32XSize,
									u32YSize,
									TRUE,
									FALSE);


	if (eLCDErr != LERR_OK)
	{
		goto errorExit;
	}

	psWidget->uWidgetSpecific.psConsole->s32XCursorGraphicPos = psWidget->uWidgetSpecific.psConsole->s32XOrigin;
	psWidget->uWidgetSpecific.psConsole->s32YCursorGraphicPos = psWidget->uWidgetSpecific.psConsole->s32YOrigin;

	// Make sure the widget is enabled
	psWidget->bWidgetEnabled = TRUE;

	eLCDErr = WidgetSetPositionByHandle((WIDGETHANDLE) *peConsoleHandle,
										WIDGET_CONSOLE,
										s32XPos,
										s32YPos);

	// Link it in!
	psWidget->uWidgetSpecific.psConsole->psNextLink = sg_psConsoleWidgetHead;
	sg_psConsoleWidgetHead = psWidget->uWidgetSpecific.psConsole;

errorExit:
	// If this isn't completely OK, then bail out
	if (eLCDErr != LERR_OK)
	{
		(void) WidgetDelete(psWidget);
	}

	return(eLCDErr);
}

ELCDErr ConsolePrint(CONSOLEHANDLE eConsoleHandle,
					 LEX_CHAR *peString)
{
	BOOL bDeferredUpdate = FALSE;
	ELCDErr eLCDErr = LERR_OK;
	SWidget *psWidget;
	SConsole *psConsole;
	INT32 s32XPos;
	INT32 s32YPos;
	INT32 s32XStride;
	INT32 s32YStride;
	TEXTCHAR *peCharactersPos;		// Character position
	UINT32 *pu32CharacterColorsPos;	// Character color position
	UINT8 *pu8TransBackgroundPos;	// Background 
	BOOL bScroll = FALSE;			// Don't scroll by default

	// Nothing to do
	if (NULL == peString)
	{
		return(LERR_OK);
	}

	eLCDErr = WidgetGetPointerByHandle((WIDGETHANDLE) eConsoleHandle,
										WIDGET_CONSOLE,
										&psWidget,
										NULL);

	RETURN_ON_FAIL(eLCDErr);

	psConsole = psWidget->uWidgetSpecific.psConsole;
	GCASSERT(psConsole);

	eLCDErr = WindowLockByHandle(psConsole->psWidget->eParentWindow);
	if (eLCDErr != LERR_OK)
	{
		return(eLCDErr);
	}

	ConsoleLock(psConsole);

	// This is to avoid doing update to something that has an update timer running
	bDeferredUpdate = psConsole->bUpdateTimersRunning;

	peCharactersPos = psConsole->peCharactersPos;
	pu32CharacterColorsPos = psConsole->pu32CharacterColorsPos;
	pu8TransBackgroundPos = psConsole->pu8TransBackgroundPos;
	s32XPos = psConsole->s32XCursorGraphicPos;
	s32YPos = psConsole->s32YCursorGraphicPos;

	while (*peString)
	{
		BOOL bCR;
		BOOL bLF;
		BOOL bPrintable = FALSE;
		BOOL bScroll = FALSE;
		UINT32 u32XCursorPos = 0;
		UINT32 u32YCursorPos = 0;
		INT32 s32XOriginal;
		INT32 s32YOriginal;

		bCR = FALSE;
		bLF = FALSE;
		bPrintable = FALSE;
		bScroll = FALSE;

		if ('\r' == *peString)
		{
			bCR = TRUE;
			if (psConsole->bAutoLF)
			{
				bLF = TRUE;
			}
		}
		else
		if ('\n' == *peString)
		{
			bLF = TRUE;
			if (psConsole->bAutoCR)
			{
				bCR = TRUE;
			}
		}
		else
		{
			eLCDErr = FontGetSize(psConsole->eFontHandle,
								  (TEXTCHAR) *peString,
								  NULL,
								  NULL,
								  &s32XStride,
								  &s32YStride,
								  psConsole->eOrientation,
								  TRUE);

			if (eLCDErr != LERR_OK)
			{
				// Not printable
			}
			else
			{
				bPrintable = TRUE;
			}
		}

		// If we've received a CR, see if we should auto-LF it, too
		if ((psConsole->bAutoLF) && (bCR))
		{
			// Automatic LF
			bLF = TRUE;
		}

		u32XCursorPos = psConsole->u32XCursorPos;
		u32YCursorPos = psConsole->u32YCursorPos;
		s32XOriginal = s32XPos;
		s32YOriginal = s32YPos;

		if (bPrintable)
		{
			// If it's a printable character, then store it
			if (ROT_0 == psConsole->eOrientation)
			{
				// Check for X wrap
				s32XPos += s32XStride;

				if (s32XPos > (INT32) (psConsole->psWidget->u32XSize << 6))
				{
					s32XPos = s32XStride;
					s32YPos += (INT32) (psConsole->u32CharYMax);
					s32XOriginal = 0;
					psConsole->u32XCursorPos = 1;
					u32XCursorPos = 0;

					psConsole->u32YCursorPos++;
					s32YOriginal += (INT32) (psConsole->u32CharYMax);
					u32YCursorPos++;
				}
				else
				{
					psConsole->u32XCursorPos++;
				}

				if ((s32YPos + (INT32) (psConsole->u32CharYMax)) >= (INT32) (psConsole->psWidget->u32YSize << 6))
				{
					GCASSERT(psConsole->u32YCursorPos);
					psConsole->u32YCursorPos--;
					u32YCursorPos--;
					s32YPos -= (INT32) (psConsole->u32CharYMax);
					s32YOriginal -= (INT32) (psConsole->u32CharYMax);
					bScroll = TRUE;
				}
			}
			else
			if (ROT_180 == psConsole->eOrientation)
			{
				// Check for X wrap
				s32XPos += s32XStride;

				if (s32XPos < 0)
				{
					s32XPos = (INT32) ((psConsole->psWidget->u32XSize) << 6) + s32XStride;
					s32YPos -= (INT32) (psConsole->u32CharYMax);
					s32XOriginal = (INT32) ((psConsole->psWidget->u32XSize) << 6);
					psConsole->u32XCursorPos = 1;
					u32XCursorPos = 0;

					psConsole->u32YCursorPos++;
					s32YOriginal -= (INT32) (psConsole->u32CharYMax);
					u32YCursorPos++;
				}
				else
				{
					psConsole->u32XCursorPos++;
				}

				if (s32YPos < (INT32) (psConsole->u32CharYMax))
				{
					GCASSERT(psConsole->u32YCursorPos);
					psConsole->u32YCursorPos--;
					u32YCursorPos--;
					s32YPos += (INT32) (psConsole->u32CharYMax);
					s32YOriginal += (INT32) (psConsole->u32CharYMax);
					bScroll = TRUE;
				}
			}
			else
			if (ROT_90 == psConsole->eOrientation)
			{
				// Check for virtual X swap (which is Y)
				s32YPos += s32YStride;

				if (s32YPos > (INT32) (psConsole->psWidget->u32YSize << 6))
				{
					// Wrap.
					s32YPos = s32YStride;
					s32XPos -= (INT32) (psConsole->u32CharYMax);
					s32YOriginal = 0;
					s32XOriginal -= (INT32) (psConsole->u32CharYMax);
					psConsole->u32XCursorPos = 1;
					u32XCursorPos = 0;
					psConsole->u32YCursorPos++;
					u32YCursorPos++;
				}
				else
				{
					psConsole->u32XCursorPos++;
				}

				if (s32XPos < (INT32) (psConsole->u32CharYMax))
				{
					GCASSERT(psConsole->u32YCursorPos);
					psConsole->u32YCursorPos--;
					u32YCursorPos--;
					s32XPos += (INT32) (psConsole->u32CharYMax);
					s32XOriginal = (INT32) (psConsole->u32CharYMax);
					bScroll = TRUE;
				}
			}
			else
			if (ROT_270 == psConsole->eOrientation)
			{
				s32YPos += s32YStride;

				if (s32YPos < 0)
				{
					// Wrap.
					s32YPos = (psConsole->psWidget->u32YSize << 6) + s32YStride;
					s32XPos += (INT32) (psConsole->u32CharYMax);
					s32YOriginal = psConsole->s32YOrigin;
					s32XOriginal += (INT32) (psConsole->u32CharYMax);

					psConsole->u32XCursorPos = 1;
					u32XCursorPos = 0;
					psConsole->u32YCursorPos++;
					u32YCursorPos++;
				}
				else
				{
					psConsole->u32XCursorPos++;
				}

				if ((s32XPos + (INT32) (psConsole->u32CharYMax)) >= (INT32) (psConsole->psWidget->u32XSize << 6))
				{
					GCASSERT(psConsole->u32YCursorPos);
					psConsole->u32YCursorPos--;
					u32YCursorPos--;
					s32XPos -= (INT32) (psConsole->u32CharYMax);
					s32XOriginal -= (INT32) (psConsole->u32CharYMax);
					bScroll = TRUE;
				}
			}
			else
			{
				GCASSERT(0);
			}
		}
		else
		{
			// Not a printable character, but let's see if it's a CR and/or LF

			if (bCR)
			{
				if ((ROT_0 == psConsole->eOrientation) ||
					(ROT_180 == psConsole->eOrientation))
				{
					s32XPos = psConsole->s32XOrigin;
				}
				else
				if ((ROT_90 == psConsole->eOrientation) ||
					(ROT_270 == psConsole->eOrientation))
				{
					s32YPos = psConsole->s32YOrigin;
				}
				else
				{
					GCASSERT(0);
				}

				u32XCursorPos = 0;
				psConsole->u32XCursorPos = 0;
			}

			if (bLF)
			{
				UINT32 u32Loop = 0;
				psConsole->u32YCursorPos++;
				u32YCursorPos++;

				if (ROT_0 == psConsole->eOrientation)
				{
					s32YPos += (INT32) (psConsole->u32CharYMax);

					if ((s32YPos + (INT32) (psConsole->u32CharYMax)) >= (INT32) (psConsole->psWidget->u32YSize << 6))
					{
						GCASSERT(psConsole->u32YCursorPos);
						psConsole->u32YCursorPos--;
						u32YCursorPos--;
						s32YPos -= (INT32) (psConsole->u32CharYMax);
						bScroll = TRUE;
					}
				}
				else
				if (ROT_180 == psConsole->eOrientation)
				{
					s32YPos -= (INT32) (psConsole->u32CharYMax);

					if (s32YPos < (INT32) psConsole->u32CharYMax)
					{
						GCASSERT(psConsole->u32YCursorPos);
						psConsole->u32YCursorPos--;
						u32YCursorPos--;
						s32YPos += (INT32) (psConsole->u32CharYMax);
						bScroll = TRUE;
					}
				}
				else
				if (ROT_90 == psConsole->eOrientation)
				{
					s32XPos -= (INT32) (psConsole->u32CharYMax);

					if (s32XPos < (INT32) psConsole->u32CharYMax)
					{
						GCASSERT(psConsole->u32YCursorPos);
						psConsole->u32YCursorPos--;
						u32YCursorPos--;
						s32XPos += (INT32) (psConsole->u32CharYMax);
						bScroll = TRUE;
					}
				}
				else
				if (ROT_270 == psConsole->eOrientation)
				{
					s32XPos += (INT32) (psConsole->u32CharYMax);;

					if ((s32XPos + (INT32) (psConsole->u32CharYMax)) >= (INT32) (psConsole->psWidget->u32XSize << 6))
					{
						GCASSERT(psConsole->u32YCursorPos);
						psConsole->u32YCursorPos--;
						u32YCursorPos--;
						s32XPos -= (INT32) (psConsole->u32CharYMax);
						bScroll = TRUE;
					}
				}
				else
				{
					GCASSERT(0);
				}

				if (FALSE == bScroll)
				{
					u32Loop = 0;

					peCharactersPos = (TEXTCHAR *) &psConsole->peCharacters[(u32YCursorPos * psConsole->u32XCharacterSize)];
					while (u32Loop < u32XCursorPos)
					{
						*peCharactersPos = (LEX_CHAR) ' ';
						++peCharactersPos;
						u32Loop++;
					}
				}
			}
		}

		GCASSERT(u32YCursorPos < psConsole->u32YCharacterSize);
		GCASSERT(u32XCursorPos < psConsole->u32XCharacterSize);

		// Shall we scroll?
		if (bScroll && (FALSE == psConsole->bScrollLock))
		{
			UINT32 *pu32Colors; 
			UINT32 u32Loop;

			// Scroll a line of characters
			memmove((void *) psConsole->peCharacters,
					(void *) &psConsole->peCharacters[psConsole->u32XCharacterSize],
					sizeof(*psConsole->peCharacters) * psConsole->u32XCharacterSize * (psConsole->u32YCharacterSize - 1));

			// Scroll a line of colors
			memmove((void *) psConsole->pu32CharacterColors,
					(void *) &psConsole->pu32CharacterColors[psConsole->u32XCharacterSize],
					sizeof(*psConsole->pu32CharacterColors) * psConsole->u32XCharacterSize * (psConsole->u32YCharacterSize - 1));

			// Now the transparencies
			memmove((void *) psConsole->pu8TransBackground,
					(void *) &psConsole->pu8TransBackground[psConsole->u32XCharacterSize],
					sizeof(*psConsole->pu8TransBackground) * psConsole->u32XCharacterSize * (psConsole->u32YCharacterSize - 1));

			// Make it so we don't scroll the next time
			bScroll = FALSE;

			// We've got a deferred update going on now
			bDeferredUpdate = TRUE;

			// Now we set the last line to whatever the current setting is

			// Set the characters to "no characters"
			memset((void *) &psConsole->peCharacters[psConsole->u32XCharacterSize * (psConsole->u32YCharacterSize - 1)],
				   0xff,
				   psConsole->u32XCharacterSize * sizeof(*psConsole->peCharacters));

			// Set the transparencies/background
			memset((void *) &psConsole->pu8TransBackground[psConsole->u32XCharacterSize * (psConsole->u32YCharacterSize - 1)],
				   (int) psConsole->bTransBackground,
				   psConsole->u32XCharacterSize * sizeof(*psConsole->pu8TransBackground));

			// Now set the background color
			pu32Colors = &psConsole->pu32CharacterColors[psConsole->u32XCharacterSize * (psConsole->u32YCharacterSize - 1)];
			u32Loop = psConsole->u32XCharacterSize;

			while (u32Loop--)
			{
				*pu32Colors = (UINT32) (psConsole->u16TextColor | ((UINT32) psConsole->u16TextBackground << 16));
				++pu32Colors;
			}

			u32Loop = 0;

			peCharactersPos = (TEXTCHAR *) &psConsole->peCharacters[(u32YCursorPos * psConsole->u32XCharacterSize)];
			while (u32Loop < u32XCursorPos)
			{
				*peCharactersPos = (LEX_CHAR) ' ';
				++peCharactersPos;
				u32Loop++;
			}
		}

		// Reposition the character position stuff
		peCharactersPos = (TEXTCHAR *) &psConsole->peCharacters[u32XCursorPos + (u32YCursorPos * psConsole->u32XCharacterSize)];
		pu32CharacterColorsPos = &psConsole->pu32CharacterColors[u32XCursorPos + (u32YCursorPos * psConsole->u32XCharacterSize)];
		pu8TransBackgroundPos = &psConsole->pu8TransBackground[u32XCursorPos + (u32YCursorPos * psConsole->u32XCharacterSize)];

		// Only store the character if it's printable
		if (bPrintable)
		{
			*peCharactersPos = *peString;
			*pu32CharacterColorsPos = (UINT32) (psConsole->u16TextColor | ((UINT32) psConsole->u16TextBackground << 16));
			*pu8TransBackgroundPos = (UINT8) (psConsole->bTransBackground);
		}

		if ((FALSE == bDeferredUpdate) &&
			(bPrintable))
		{
			INT32 s32XPaint;
			INT32 s32YPaint;

			s32XPaint = (psConsole->psWidget->s32XPos << 6) + s32XOriginal;
			s32YPaint = (psConsole->psWidget->s32YPos << 6) + s32YOriginal;

			// If this current console's background isn't transparent, then draw a solid
			if (FALSE == psConsole->bTransBackground)
			{
				INT32 s32XCharSize;
				INT32 s32YCharSize;

				// Assume ROT 0 origin

				if (ROT_0 == psConsole->eOrientation)
				{
					s32XCharSize = s32XStride;
					s32YCharSize = (INT32) psConsole->u32CharYMax;
				}
				else
				if (ROT_180 == psConsole->eOrientation)
				{
					s32XCharSize = s32XStride;
					s32YCharSize = (INT32) psConsole->u32CharYMax;
					s32XPaint += s32XStride;
					s32YPaint -= (INT32) (s32YCharSize);
					GCASSERT(s32XPaint >= 0);
					GCASSERT(s32YPaint >= 0);
				}
				else
				if (ROT_90 == psConsole->eOrientation)
				{
					s32XCharSize = (INT32) psConsole->u32CharYMax;
					s32YCharSize = s32YStride;
					s32XPaint -= s32XCharSize;
					GCASSERT(s32XPaint >= 0);
					GCASSERT(s32YPaint >= 0);
				}
				else
				if (ROT_270 == psConsole->eOrientation)
				{
					s32XCharSize = (INT32) psConsole->u32CharYMax;;
					s32YCharSize = s32YStride;
					s32YPaint += s32YCharSize;
					GCASSERT(s32XPaint >= 0);
					GCASSERT(s32YPaint >= 0);
				}
				else
				{
					GCASSERT(0);
				}

				s32XPaint >>= 6;
				s32YPaint >>= 6;

				s32XCharSize = (abs(s32XCharSize) + 0x3f) >> 6;
				s32YCharSize = (abs(s32YCharSize) + 0x3f) >> 6;

				// This will draw a solid where the character will go. Adding the active
				// area offset isn't needed since that's done by the fill routine
				eLCDErr = WindowColorFillRegion(psConsole->psWidget->eParentWindow,
												psConsole->u16TextBackground,
												s32XPaint,
												s32YPaint,
												s32XCharSize,
												s32YCharSize,
												FALSE);

				ERROREXIT_ON_FAIL(eLCDErr);		
			}

			s32XPaint = (psConsole->psWidget->s32XPos << 6) + s32XOriginal;
			s32YPaint = (psConsole->psWidget->s32YPos << 6) + s32YOriginal;

			if (ROT_0 == psConsole->eOrientation)
			{
				// Get our X/Y paint coordinates
			}
			else
			if (ROT_180 == psConsole->eOrientation)
			{
				s32XPaint += s32XStride;
				s32YPaint -= ((INT32) psConsole->u32CharYMax + 0x3f);
			}
			else
			if (ROT_90 == psConsole->eOrientation)
			{
				s32XPaint -= ((INT32) psConsole->u32CharYMax + 0x3f);
			}
			else
			if (ROT_270 == psConsole->eOrientation)
			{
				s32YPaint += s32YStride;
			}
			else
			{
				GCASSERT(0);
			}

			// Now draw the character
			eLCDErr = FontRender(psConsole->eFontHandle,
								 psConsole->psWidget->eParentWindow,
								 s32XPaint,
								 s32YPaint,
								 -1,
								 -1,
								 *peString,
								 psConsole->u16TextColor,
								 FALSE,
								 NULL,
								 NULL,
								 FALSE,
								 psConsole->eOrientation,
								 NULL,
								 FALSE,
								 TRUE);

			ERROREXIT_ON_FAIL(eLCDErr);		
		}
		else
		{
			// We're just adding characters and we'll repaint them asynchronously since we'll have to
			// scroll or something
		}

		++peString;
	}

errorExit:
	// Put all of our temp crap back
	psConsole->peCharactersPos = peCharactersPos;
	psConsole->pu32CharacterColorsPos = pu32CharacterColorsPos;
	psConsole->pu8TransBackgroundPos = pu8TransBackgroundPos;
	psConsole->s32XCursorGraphicPos = s32XPos;
	psConsole->s32YCursorGraphicPos = s32YPos;

	ConsoleUnlock(psConsole);

	if (bDeferredUpdate)
	{
		eLCDErr = ConsoleTriggerUpdate(psConsole->eConsoleHandle);
	}

	if (eLCDErr != LERR_OK)
	{
		eLCDErr = WindowUnlockByHandle(psConsole->psWidget->eParentWindow);
	}
	else
	{
		(void) WindowUnlockByHandle(psConsole->psWidget->eParentWindow);
	}

	// Now commit
	WindowUpdateRegionCommit();

	// Return the error code
	return(eLCDErr);
}

ELCDErr ConsoleSetColor(CONSOLEHANDLE eConsoleHandle,
						UINT32 u32Color)
{
	SWidget *psWidget;
	ELCDErr eErr;
	SConsole *psConsole;

	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eConsoleHandle,
									WIDGET_CONSOLE,
									&psWidget,
									NULL);

	RETURN_ON_FAIL(eErr);

	psConsole = psWidget->uWidgetSpecific.psConsole;
	GCASSERT(psConsole);

	psConsole->u16TextColor = CONVERT_24RGB_16RGB(u32Color);

	return(LERR_OK);
}

ELCDErr ConsoleSetBackgroundColor(CONSOLEHANDLE eConsoleHandle,
								  UINT32 u32Color,
								  BOOL bBackgroundColorEnabled)
{
	SWidget *psWidget;
	ELCDErr eErr;
	SConsole *psConsole;

	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eConsoleHandle,
									WIDGET_CONSOLE,
									&psWidget,
									NULL);

	RETURN_ON_FAIL(eErr);

	psConsole = psWidget->uWidgetSpecific.psConsole;
	GCASSERT(psConsole);

	if (bBackgroundColorEnabled)
	{
		psConsole->bTransBackground = FALSE;
		psConsole->u16TextBackground = CONVERT_24RGB_16RGB(u32Color);
	}
	else
	{
		psConsole->bTransBackground = TRUE;
	}

	return(LERR_OK);
}

ELCDErr ConsoleSetScrollEnable(CONSOLEHANDLE eConsoleHandle,
							   BOOL bScrollEnabled)
{
	SWidget *psWidget;
	ELCDErr eErr;
	SConsole *psConsole;

	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eConsoleHandle,
									WIDGET_CONSOLE,
									&psWidget,
									NULL);

	RETURN_ON_FAIL(eErr);

	psConsole = psWidget->uWidgetSpecific.psConsole;
	GCASSERT(psConsole);

	if (bScrollEnabled)
	{
		psConsole->bScrollLock = FALSE;
	}
	else
	{
		psConsole->bScrollLock = TRUE;
	}

	return(LERR_OK);
}

ELCDErr ConsoleClear(CONSOLEHANDLE eConsoleHandle)
{
	SWidget *psWidget;
	ELCDErr eErr;
	SConsole *psConsole;
	UINT32 *pu32Color;
	UINT32 u32Loop;

	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eConsoleHandle,
									WIDGET_CONSOLE,
									&psWidget,
									NULL);

	RETURN_ON_FAIL(eErr);

	psConsole = psWidget->uWidgetSpecific.psConsole;
	GCASSERT(psConsole);

	eErr = WindowLockByHandle(psConsole->psWidget->eParentWindow);
	if (eErr != LERR_OK)
	{
		return(eErr);
	}

	ConsoleLock(psConsole);

	// Set every character to a 'non character'
	memset((void *) psConsole->peCharacters, 0xff, psConsole->u32XCharacterSize * psConsole->u32YCharacterSize * sizeof(*psConsole->peCharacters));

	// Set the transparency stuff appropriately
	memset((void *) psConsole->pu8TransBackground, (int) psConsole->bTransBackground, psConsole->u32XCharacterSize * psConsole->u32YCharacterSize * sizeof(*psConsole->pu8TransBackground));

	// Set to the appropriate foreground/background color
	u32Loop = psConsole->u32XCharacterSize * psConsole->u32YCharacterSize;

	pu32Color = psConsole->pu32CharacterColors;
	while (u32Loop--)
	{
		*pu32Color = ((UINT32) psConsole->u16TextBackground << 16) | psConsole->u16TextColor;
		++pu32Color;
	}

	psConsole->u32XCursorPos = 0;
	psConsole->u32YCursorPos = 0;

	psConsole->peCharactersPos = (TEXTCHAR *) psWidget->uWidgetSpecific.psConsole->peCharacters;
	psConsole->pu32CharacterColorsPos = psWidget->uWidgetSpecific.psConsole->pu32CharacterColors;
	psConsole->pu8TransBackgroundPos = psWidget->uWidgetSpecific.psConsole->pu8TransBackground;

	psConsole->s32XCursorGraphicPos = psConsole->s32XOrigin;
	psConsole->s32YCursorGraphicPos = psConsole->s32YOrigin;

	ConsoleUnlock(psConsole);

	// Now go trigger an update
	eErr = ConsoleTriggerUpdate(psConsole->eConsoleHandle);

	if (LERR_OK == eErr)
	{
		eErr = WindowUnlockByHandle(psConsole->psWidget->eParentWindow);
	}
	else
	{
		(void) WindowUnlockByHandle(psConsole->psWidget->eParentWindow);
	}

	// Now commit
	WindowUpdateRegionCommit();

	return(eErr);
}
