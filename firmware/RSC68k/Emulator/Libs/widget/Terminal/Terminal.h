#ifndef _TERMINAL_H_
#define _TERMINAL_H_

typedef enum
{
	ECHARSTATE_WAIT_ESCAPE,
	ECHARSTATE_WAIT_BRACKET,
	ECHARSTATE_ABSORB_CHARACTERS
} ECharState;

// Terminal structure
typedef struct STerminal
{
	// Byte 0 - Character
	// Byte 1 - Bits 0-3 - Foreground color (bit 3=intensity)
	//			Bits 4-6 - Background color
	//			Bit 7    - Blink attribute

	UINT8 *pu8CharMap;			// Terminal/character map
	UINT32 u32XSize;			// # Of columns
	UINT32 u32YSize;			// # Of rows

	// Setting related
	UINT8 u8Color;				// Current foreground/background colors
	UINT8 u8Attribute;			// Current character attributes

	// Cursor related
	UINT32 u32XCursorPos;		// X Cursor position
	UINT32 u32YCursorPos;		// Y Cursor position
	BOOL bCursorOn;				// Is the cursor on?
	UINT32 u32CursorTimer;		// Cursor blink timer
	BOOL bCursorState;			// Is the cursor character currently shown (for blinky)

	// Blink character related
	UINT32 u32BlinkCounter;
	BOOL bBlinkShow;			// Should we show blinking?

	// Character absorption
	ECharState eCharState;		// Character reception state
	UINT32 u32RXLen;			// # Of bytes received in escaped RX buffer
	UINT8 u8EscapeRXBuffer[256];	// Escape character receive buffer

	// Receiver callback
	void (*pCallback)(struct STerminal *psTerminal,
					  LEX_CHAR eCharacter);
} STerminal;

// Rate of text blink
#define	TEXT_BLINK_RATE	500			// 500msec per blink

// Rate of cursor blink
#define	CURSOR_ON_TIME		750
#define CURSOR_OFF_TIME		250
#define CURSOR_TOTAL_TIME	(CURSOR_ON_TIME + CURSOR_OFF_TIME)

// APIs
extern ELCDErr TerminalCreate(TERMINALHANDLE *peTerminalHandle,
							  WINDOWHANDLE eWindowHandle,
							  INT32 s32XPos,
							  INT32 s32YPos,
							  UINT32 u32XCharSize,
							  UINT32 u32YCharSize);
extern ELCDErr TerminalPutChar(TERMINALHANDLE eTerminalHandle,
							   UINT8 u8Character);
extern ELCDErr TerminalSetCharCallback(TERMINALHANDLE eTerminalHandle,
									   void (*pCallback)(STerminal *psTerminal,
														 LEX_CHAR eChar));

// First time init
extern void TerminalFirstTimeInit(void);

#endif