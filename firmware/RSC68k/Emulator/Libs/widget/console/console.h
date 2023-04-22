#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#include "Libs/FontMgr/FontMgr.h"
#include "Libs/widget/text/text.h"

typedef struct SConsole
{
	CONSOLEHANDLE eConsoleHandle;	// Console handle
	UINT32 u32XCharacterSize;		// # Of characters in console
	UINT32 u32YCharacterSize;		// # Of lines in console
	ERotation eOrientation;			// Which direction does the text go?

	// Maximum x/y size of characters
	UINT32 u32CharXMax;
	UINT32 u32CharYMax;

	// Origin information
	INT32 s32XOrigin;
	INT32 s32YOrigin;

	// Currently active cursor/pointer positions
	UINT32 u32XCursorPos;			// X/Y cursor position (in characters)
	UINT32 u32YCursorPos;
	INT32 s32XCursorGraphicPos;		// X/Y Graphics position of cursor (in pixels, relative to the widget)
	INT32 s32YCursorGraphicPos;
	TEXTCHAR *peCharactersPos;		// Character position
	UINT32 *pu32CharacterColorsPos;	// Character color position
	UINT8 *pu8TransBackgroundPos;	// Background 

	// Currently active text related colors
	UINT16 u16TextColor;			// Text color
	UINT16 u16TextBackground;		// Text background color
	BOOL bTransBackground;			// Transparent background?
	BOOL bAutoLF;					// Automatic linefeed when CR encountered?
	BOOL bAutoCR;					// Automatic implied CR when linefeed encountered
	BOOL bScrollLock;				// Are we prohibited from scrolling?

	// Text arrays
	LEX_CHAR *peCharacters;			// Array of characters for this console
	UINT32 *pu32CharacterColors;	// Array of color characters (foreground/background colors)
	UINT8 *pu8TransBackground;		// Transparent background?

	FONTHANDLE eFontHandle;			// Find handle this console is related to
	struct SWidget *psWidget;		// Widget pointer

	// Timer callback stuff
	BOOL bUpdateTimersRunning;		// Is the update timer running?
	UINT32 u32MSSinceLastPrint;		// # Of milliseconds since last print
	UINT32 u32MSSinceLastPrintRequest;	// # Of milliseconds since last print request
	BOOL bUpdateInProgress;			// Is an update in progress?

	// Next widget
	struct SConsole *psNextLink;	// Next console
} SConsole;

#define	NO_CHARACTER	((1 << (sizeof(LEX_CHAR) * 8)) - 1)

extern void ConsoleFirstTimeInit(void);
extern ELCDErr ConsoleCreate(WINDOWHANDLE eWINDOWHANDLE,
							 FONTHANDLE eFontHandle,
							 CONSOLEHANDLE *peConsoleHandle,
							 UINT16 u16Orientation,
							 INT32 s32XPos,
							 INT32 s32YPos,
							 UINT32 u32XSize,
							 UINT32 u32YSize);
extern ELCDErr ConsolePrintCharacter(CONSOLEHANDLE eConsole,
									 TEXTCHAR eCharacter,
									 UINT16 u16Color);
extern SConsole *ConsoleGetPointer(CONSOLEHANDLE eHandle);
extern ELCDErr ConsolePrint(CONSOLEHANDLE eConsoleHandle,
							LEX_CHAR *peString);
extern void ConsoleWidgetUpdate(CONSOLEHANDLE eConsoleHandle);
extern ELCDErr ConsoleSetColor(CONSOLEHANDLE eConsoleHandle,
							   UINT32 u32Color);
extern ELCDErr ConsoleSetBackgroundColor(CONSOLEHANDLE eConsoleHandle,
										 UINT32 u32Color,
										 BOOL bBackgroundColorEnabled);
extern ELCDErr ConsoleSetScrollEnable(CONSOLEHANDLE eConsoleHandle,
									  BOOL bScrollEnabled);
extern ELCDErr ConsoleClear(CONSOLEHANDLE eConsoleHandle);

#endif