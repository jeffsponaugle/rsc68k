#include "Startup/app.h"
#include "Application/RSC68k.h"
#include "Application/Mersenne.h"
#include "Libs/window/window.h"
#include "Libs/widget/widget.h"
#include "Application/emu.h"
#include "Hardware/RSC68k.h"
#include "Shared/16550.h"

static BOOL sg_bReload = TRUE;

static UINT32 sg_u32PeriodicTimer = 0;

static INT64 sg_s64TimerCounter = 0;
static UINT64 sg_u64Seconds = 0;

// CPU Speed
#define CPU_SPEED			16000000
#define	EMU_FPS				60
#define	PERIODIC_TICK		(CPU_SPEED / 100)		// 100Hz timer tick

typedef enum
{
	ECPU_SINGLE_STEP,
	ECPU_RUN
} ECPUState;

typedef struct S7Seg
{
	IMAGEHANDLE e7SegBackground;
	IMAGEHANDLE e7SegSegments[9];	// 7 Segments plus 2 dots
} S7Seg;

// Left 7 segment digit
static S7Seg sg_sDigit1;

// Right 7 segment digit
static S7Seg sg_sDigit2;

// Terminal window stuff
static WINDOWHANDLE sg_eConsoleTerminalWindow;
static TERMINALHANDLE sg_eTerminalHandle;

// Interrupts pending
static UINT16 sg_u16IRQState = 0;

// Interrupt mask
static UINT16 sg_u16IRQMask = 0;

typedef struct SLEDIndicator
{
	IMAGEHANDLE eIndicatorOn;
	IMAGEHANDLE eIndicatorOff;
} SLEDIndicator;

static UINT8 sg_u8KeyQueue[128];
static UINT8 sg_u8KeyQueueHead = 0;
static UINT8 sg_u8KeyQueueTail = 0;

static void KeyQueueDeposit(UINT8 u8Char)
{
	UINT8 u8Head;

	u8Head = sg_u8KeyQueueHead + 1;
	if (u8Head >= sizeof(sg_u8KeyQueue))
	{
		u8Head = 0;
	}
	if (u8Head == sg_u8KeyQueueTail)
	{
		sg_u8KeyQueueTail++;
		if (sg_u8KeyQueueTail >= sizeof(sg_u8KeyQueue))
		{
			sg_u8KeyQueueTail = 0;
		}
	}

	sg_u8KeyQueue[sg_u8KeyQueueHead] = u8Char;
	sg_u8KeyQueueHead = u8Head;
}

#define	UART_CLOCK		1843200

typedef struct S16550UART
{
	UINT16 u16Stipple;
	UINT8 u8Registers[8];
	UINT8 u8FCR;
	UINT16 u16DLAB;

	// Transmitter output
	UINT8 u8XMitBuffer[16];
	UINT8 u8XMitSize;
	UINT8 u8XMitCount;
	UINT8 u8XMitTail;
	UINT8 u8XMitHead;

	// Receiver input
	UINT8 u8RXBuffer[16];	// RX Data buffer
	UINT8 u8RXSize;			// Total size of RX buffer
	UINT8 u8RXCount;		// # Of characters in RX buffer now
	UINT8 u8RXTail;			// Tail rx pointer
	UINT8 u8RXHead;			// Head rx pointer

	// Used for clocking out characters in emulated time
	UINT32 u32TStateCharTime;
	INT32 s32TStateCharTimer;

	// Interrupt control
	BOOL bTHREInterruptPending;
	BOOL bRXInterruptPending;

	// Callback for transmit data
	void (*DataTX)(struct S16550UART *psUART,
				   UINT8 u8Data);
} S16550UART;

// IIR registers
#define UART_IIR_THRE	0x02
#define UART_IIR_RXDATA	0x04

// Divisor latch access bit
#define	UART_LCR_DLAB	0x80

// OUT2
#define	UART_MCR_OUT2	0x08

// Transmitter stuff
#define	UART_LSR_THRE		0x20
#define	UART_LSR_TEMT		0x40

// Receiver stuff
#define	UART_LSR_DR			0x01

// Supervisor UARTs
static S16550UART sg_sUARTA;
static S16550UART sg_sUARTB;

static void UARTWrite(S16550UART *psUART,
					  UINT8 u8Offset,
					  UINT8 u8Data)
{
	// Scratch register?
	if ((UART_REG_SCR == u8Offset) ||
		(UART_REG_LCR == u8Offset))
	{
		psUART->u8Registers[u8Offset] = u8Data;
	}
	else
	if (UART_REG_IIR == u8Offset)
	{

		// This is really the FCR
		psUART->u8FCR = u8Data & 0xc9;

		if (psUART->u8FCR >> 6)
		{
			// FIFOs are ON! Set them both to 16 characters in length
			psUART->u8RXSize = 16;
			psUART->u8XMitSize = 16;
		}
		else
		{
			// FIFOs are OFF! Set them both to 1 character
			psUART->u8RXSize = 1;
			psUART->u8XMitSize = 1;
		}
	}
	else
	if (UART_REG_RBRTHR == u8Offset)
	{
		// If we are in DLAB mode, then set things up
		if (psUART->u8Registers[UART_REG_LCR] & UART_LCR_DLAB)
		{
			psUART->u16DLAB = (psUART->u16DLAB & 0xff00) | u8Data;
			if (psUART->u16DLAB)
			{
				psUART->u32TStateCharTime = (CPU_SPEED / (((UART_CLOCK / 16) / psUART->u16DLAB) / 10));
			}
			else
			{
				psUART->u32TStateCharTime = 0;
			}
		}
		else
		{
			// Clear any THRE pending interrupts
			psUART->bTHREInterruptPending = FALSE;

			// Data write
			if (psUART->u8XMitCount == psUART->u8XMitSize)
			{
				// Too many! Transmitter overrun!
			}
			else
			{
				// Stuff it in the outgoing transmitter buffer
				psUART->u8XMitBuffer[psUART->u8XMitHead++] = u8Data;
				if (psUART->u8XMitHead >= psUART->u8XMitSize)
				{
					psUART->u8XMitHead = 0;
				}

				if (0 == psUART->u8XMitCount)
				{
					psUART->s32TStateCharTimer += (INT32) psUART->u32TStateCharTime;
				}

				psUART->u8XMitCount++;
			}
		}
	}
	else
	if (UART_REG_IER == u8Offset)
	{
		// If we are in DLAB mode, then set things up
		if (psUART->u8Registers[UART_REG_LCR] & UART_LCR_DLAB)
		{
			psUART->u16DLAB = (psUART->u16DLAB & 0xff) | (u8Data << 8);

			if (psUART->u16DLAB)
			{
				psUART->u32TStateCharTime = (CPU_SPEED / (((UART_CLOCK / 16) / psUART->u16DLAB) / 10));
			}
			else
			{
				psUART->u32TStateCharTime = 0;
			}
		}
		else
		{
			// IER write
			psUART->u8Registers[u8Offset] = u8Data;
		}
	}
	else
	if (UART_REG_MCR == u8Offset)
	{
		UINT8 u8Old = psUART->u8Registers[u8Offset];

		// We're updating the modem control register. If we're changing OUT2, then let the
		// display driver know about it
		psUART->u8Registers[u8Offset] = u8Data;

		if ((u8Old ^ u8Data) & UART_MCR_OUT2)
		{
			// Out2 changed. Update the LED.
		}
	}
	else
	{
		// Don't know yet
		GCASSERT(0);
	}
}

// Incoming data into our UART
static void UARTRXData(S16550UART *psUART,
					   UINT8 u8Char)
{
	// If we have receiver interrupts turned on, then signal that we have an interrupt
	if (psUART->u8Registers[UART_REG_IER] & 1)
	{
		psUART->bRXInterruptPending = TRUE;
	}

	if (psUART->u8RXCount >= psUART->u8RXSize)
	{
		// TODO: Overrun error
		return;
	}

	psUART->u8RXBuffer[psUART->u8RXHead++] = u8Char;
	psUART->u8RXCount++;
	if (psUART->u8RXHead >= psUART->u8RXSize)
	{
		psUART->u8RXHead = 0;
	}
}

static UINT8 UARTRead(S16550UART *psUART,
					  UINT8 u8Offset)
{
	if (((UART_REG_IER == u8Offset) ||
	 	(UART_REG_RBRTHR == u8Offset)) && (psUART->u8Registers[UART_REG_LCR] & UART_LCR_DLAB))
	{
		// Divisor access latch. See which we return
		if (UART_REG_RBRTHR == u8Offset)
		{
			return((UINT8) psUART->u16DLAB);
		}
		else
		{
			return((UINT8) (psUART->u16DLAB >> 8));
		}
	}
	else
	if (UART_REG_MSR == u8Offset)
	{
		return(0xb0);	 // Hardcode DCD, DSR, and CTS to asserted
	}
	else
	if (UART_REG_RBRTHR == u8Offset)
	{
		UINT8 u8Data;

		// Receive buffer register
		u8Data = psUART->u8RXBuffer[psUART->u8RXTail];
		if (psUART->u8RXCount)
		{
			// Only advance the tail pointer if there's data in the buffer
			psUART->u8RXTail++;
			if (psUART->u8RXTail >= psUART->u8RXSize)
			{
				psUART->u8RXTail = 0;
			}

			psUART->u8RXCount--;
			if (0 == psUART->u8RXCount)
			{
				// No more interrupt pending if we sucked out the last byte
				psUART->bRXInterruptPending = FALSE;
			}
		}
		else
		{
			psUART->bRXInterruptPending = FALSE;
		}

		return(u8Data);
	}
	if (UART_REG_LSR == u8Offset)
	{
		UINT8 u8Data;

		u8Data = 0;

		// See if the transmitter holding register has anything in it
		if (0 == psUART->u8XMitCount)
		{
			u8Data |= (UART_LSR_THRE | UART_LSR_TEMT);
		}

		// See if we have any data pending
		if (psUART->u8RXCount)
		{
			u8Data |= UART_LSR_DR;
		}

		return(u8Data);
	}
	else
	if (UART_REG_IIR == u8Offset)
	{
		UINT8 u8Data = 0;

		// Pull in FIFO enable bits (if enabled)
		if (psUART->u8FCR & 1)
		{
			// FIFO is enabled.
			u8Data |= (psUART->u8FCR & 0xc0);
		}

		if (psUART->bRXInterruptPending)
		{
			return(UART_IIR_RXDATA | u8Data);
		}

		if (psUART->bTHREInterruptPending)
		{
			psUART->bTHREInterruptPending = FALSE;
			return(UART_IIR_THRE | u8Data);
		}

		u8Data |= 0x01;	// No interrupt pending
		return(u8Data);
	}
	else
	{
		// Return the register
		return(psUART->u8Registers[u8Offset]);
	}
}

static void UARTStep(S16550UART *psUART,
					 UINT32 u32TStates)
{
	// See if there's something to transmit
	if (psUART->u8XMitCount)
	{
		// Well, we're running, so let's decrement our timer
		psUART->s32TStateCharTimer -= (INT32) u32TStates;
		while (psUART->s32TStateCharTimer < 0)
		{
			UINT8 u8Char;

			// Means we should do something about another character.

			u8Char = psUART->u8XMitBuffer[psUART->u8XMitTail++];
			if (psUART->u8XMitTail >= psUART->u8XMitSize)
			{
				psUART->u8XMitTail = 0;
			}

			if (psUART->DataTX)
			{
				psUART->DataTX(psUART,
							   u8Char);
			}

			psUART->u8XMitCount--;
			if (0 == psUART->u8XMitCount)
			{
				// All done transmitting.
				psUART->s32TStateCharTimer = 0;

				// If THRE interrupts are turned on, then flag it
				if (psUART->u8Registers[UART_REG_IER] & UART_IIR_THRE)
				{
					psUART->bTHREInterruptPending = TRUE;
				}
			}
			else
			{
				// Not done. Add our character time to the char timer
				psUART->s32TStateCharTimer += (INT32) psUART->u32TStateCharTime;
			}
		}
	}
}

void ConsoleInputCharacter(STerminal *psTerminal,
						   LEX_CHAR eChar)
{
	UINT8 u8String[2];
	UINT8 *pu8String = u8String;

	u8String[0] = eChar;
	u8String[1] = '\0';

	if (0 == eChar)
	{
		// Left
		pu8String = "\033[1D";
	}
	else
	if (1 == eChar)
	{
		// Right
		pu8String = "\033[1C";
	}
	else
	if (2 == eChar)
	{
		// Up
		pu8String = "\033[A";
	}
	else
	if (4 == eChar)
	{
		// Down
		pu8String = "\033[B";
	}
	else
	if (5 == eChar)
	{
		// Home
		pu8String = "\033[H";
	}
	else
	if (6 == eChar)
	{
		// End
		pu8String = "\033[F";
	}

	while (*pu8String)
	{
		KeyQueueDeposit(*pu8String);
		++pu8String;
	}
}

static void ConsoleUARTTX(S16550UART *psUART,
						  UINT8 u8Data)
{
	(void) TerminalPutChar(sg_eTerminalHandle,
						   u8Data);
}


static void UARTReset(S16550UART *psUART)
{
	psUART->u8Registers[UART_REG_IER] = 0;
	psUART->u8Registers[UART_REG_IIR] = 1;
	psUART->u8Registers[UART_REG_LCR] = 0;
	psUART->u8Registers[UART_REG_MCR] = 0;
	psUART->u8Registers[UART_REG_LSR] = 0x60;
	psUART->u8FCR = 0x0;
	psUART->u16DLAB = 0;

	// Transmitter stuff
	psUART->u8XMitSize = 1;
	psUART->u8XMitCount = 0;
	psUART->u8XMitHead = 0;
	psUART->u8XMitTail = 0;

	// Receiver stuff
	psUART->u8RXCount = 0;
	psUART->u8RXSize = 1;
	psUART->u8RXHead = 0;
	psUART->u8RXTail = 0;

	psUART->u32TStateCharTime = 0;
	psUART->s32TStateCharTimer = 0;
	psUART->bTHREInterruptPending = FALSE;
	psUART->bRXInterruptPending = FALSE;
}

// The UARTs
static S16550UART sg_sUARTConsole;
static S16550UART sg_sUARTUser;

static SLEDIndicator sg_sResetN;		// CPU Reset signal
static SLEDIndicator sg_sReset;			// UART/video controller reset
static SLEDIndicator sg_sRun;			// Is the CPU running?
static SLEDIndicator sg_sIRQNone;		// No IRQ?

// This will repaint a 7 segment display
static void Seg7SetStipple(S7Seg *ps7Seg,
						   UINT16 u16Data)
{
	UINT8 u8Loop;
	ELCDErr eErr;

	// Turn off left decimal points for now
	u16Data |= 0x100;

	// Set all images to hidden
	for (u8Loop = 0; u8Loop < 9; u8Loop++)
	{
		// Turn every one of them off
		eErr = WidgetSetHideByHandle((WIDGETHANDLE) ps7Seg->e7SegSegments[u8Loop],
									 WIDGET_IMAGE,
									 TRUE,
									 FALSE);
		GCASSERT(LERR_OK == eErr);
	}

	// Force a redraw of the background image segment
	eErr = WidgetSetHideByHandle((WIDGETHANDLE) ps7Seg->e7SegBackground,
								 WIDGET_IMAGE,
								 TRUE,
								 FALSE);
	GCASSERT(LERR_OK == eErr);

	// Now we've redrawn it
	eErr = WidgetSetHideByHandle((WIDGETHANDLE) ps7Seg->e7SegBackground,
								 WIDGET_IMAGE,
								 FALSE,
								 TRUE);
	GCASSERT(LERR_OK == eErr);

	// Now run through each segment and see if we redraw it or not
	for (u8Loop = 0; u8Loop < 9; u8Loop++)
	{
		if (0 == (u16Data & 1))
		{
			eErr = WidgetSetHideByHandle((WIDGETHANDLE) ps7Seg->e7SegSegments[u8Loop],
										 WIDGET_IMAGE,
										 FALSE,
										 FALSE);
			GCASSERT(LERR_OK == eErr);
		}

		u16Data >>= 1;
	}
}

static void Seg7Write1(UINT16 u16Offset, UINT8 u8Data)
{
	Seg7SetStipple(&sg_sDigit1, u8Data);
}

static void Seg7Write2(UINT16 u16Offset, UINT8 u8Data)
{
	Seg7SetStipple(&sg_sDigit2, u8Data);
}

typedef struct SDigitSegmentOffset
{
	char *pu8ImageFilename;
	INT32 s32XOffset;
	INT32 s32YOffset;
} SDigitSegmentOffset;

// See Hardware/RSC68k.h for 7 segment mapping

static const SDigitSegmentOffset sg_sDigitSegs[] =
{
	{"files/Images/dotseg.png",	80, 117},		// Lower right dot (DP)
	{"files/Images/hseg.png",	38, 32},		// Top segment (a)
	{"files/Images/vseg.png",	67, 38},		// Upper right segment (b)
	{"files/Images/vseg.png",	62, 75},		// Lower right segment (c)
	{"files/Images/hseg.png",	29, 107},		// Bottom segment (d)
	{"files/Images/vseg.png",   22, 75},		// Lower left segment (e)
	{"files/Images/vseg.png",	26, 38},		// Upper left segment (f)
	{"files/Images/hseg.png",	34, 70},		// Middle segment (g)
	{"files/Images/dotseg.png", 7,  117},		// Lower left dot
};

static WINDOWHANDLE sg_eDisplayWindow;

static void Seg7Init(WINDOWHANDLE eWindowHandle,
					 INT32 s32XPos,
					 INT32 s32YPos,
					 S7Seg *ps7Seg)
{
	UINT8 u8Loop;
	ELCDErr eErr;

	memset((void *) ps7Seg, 0, sizeof(*ps7Seg));

	// Load up the 7 segment image
	eErr = ImageWidgetCreate(eWindowHandle,
							 &ps7Seg->e7SegBackground,
							 "files/Images/7seg.png",
							 s32XPos,
							 s32YPos,
							 IMGDRAW_SOLID);
	GCASSERT(LERR_OK == eErr);
	eErr = WidgetSetHideByHandle((WIDGETHANDLE) ps7Seg->e7SegBackground,
								 WIDGET_IMAGE,
								 FALSE,
								 FALSE);
	GCASSERT(LERR_OK == eErr);

	// Load up each of the images
	for (u8Loop = 0; u8Loop < (sizeof(sg_sDigitSegs) / sizeof(sg_sDigitSegs[0])); u8Loop++)
	{
		eErr = ImageWidgetCreate(eWindowHandle,
								 &ps7Seg->e7SegSegments[u8Loop],
								 sg_sDigitSegs[u8Loop].pu8ImageFilename,
								 sg_sDigitSegs[u8Loop].s32XOffset + s32XPos,
								 sg_sDigitSegs[u8Loop].s32YOffset + s32YPos,
								 IMGDRAW_TRANSPARENT);
		GCASSERT(LERR_OK == eErr);

		eErr = WidgetSetHideByHandle((WIDGETHANDLE) ps7Seg->e7SegSegments[u8Loop],
									 WIDGET_IMAGE,
									 TRUE,
									 TRUE);
	}
}

static ELCDErr DefaultTextCreate(TEXTHANDLE *peTextHandle,
								 WINDOWHANDLE eWindowHandle,
								 FONTHANDLE eFontHandle,
								 INT32 s32XPos,
								 INT32 s32YPos,
								 UINT16 u16Color,
								 LEX_CHAR *peText)
{
	ELCDErr eErr;

	eErr = TextCreate(peTextHandle,
					  eWindowHandle,
					  ROT_0);
	GCASSERT(LERR_OK == eErr);
	eErr = TextSetFont(*peTextHandle,
					   eFontHandle);
	GCASSERT(LERR_OK == eErr);
	eErr = TextSetColor(*peTextHandle,
						u16Color);
	GCASSERT(LERR_OK == eErr);

	eErr = WidgetSetPositionByHandle(*peTextHandle,
									 WIDGET_TEXT,
									 s32XPos,
									 s32YPos);
	GCASSERT(LERR_OK == eErr);
	eErr = WidgetSetHideByHandle((WIDGETHANDLE) *peTextHandle,
								 WIDGET_TEXT,
								 FALSE,
								 TRUE);
	GCASSERT(LERR_OK == eErr);

	eErr = TextSetTextASCII(*peTextHandle,
							peText);
	GCASSERT(LERR_OK == eErr);

	return(eErr);
}

static ELCDErr IndicatorCreate(WINDOWHANDLE eWindow,
							   SLEDIndicator *psLED,
							   INT32 s32XPos,
							   INT32 s32YPos,
							   BOOL bSmall)
{
	ELCDErr eLCDErr;
	char *peOn = "files/Images/redon.png";
	char *peOff = "files/Images/redoff.png";

	if (bSmall)
	{
		peOn = "files/Images/redonsmall.png";
		peOff = "files/Images/redoffsmall.png";
	}

	memset((void *) psLED, 0, sizeof(*psLED));

	eLCDErr = ImageWidgetCreate(sg_eDisplayWindow,
								&psLED->eIndicatorOn,
								peOn,
								s32XPos, s32YPos,
								IMGDRAW_TRANSPARENT);
	GCASSERT(LERR_OK == eLCDErr);

	eLCDErr = WidgetSetHideByHandle((WIDGETHANDLE) psLED->eIndicatorOn,
								 WIDGET_IMAGE,
								 TRUE,
								 FALSE);
	GCASSERT(LERR_OK == eLCDErr);

	eLCDErr = ImageWidgetCreate(sg_eDisplayWindow,
								&psLED->eIndicatorOff,
								peOff,
								s32XPos, s32YPos,
								IMGDRAW_TRANSPARENT);
	GCASSERT(LERR_OK == eLCDErr);
	eLCDErr = WidgetSetHideByHandle((WIDGETHANDLE) psLED->eIndicatorOff,
								 WIDGET_IMAGE,
								 FALSE,
								 TRUE);
	GCASSERT(LERR_OK == eLCDErr);

	return(eLCDErr);
}

static void IndicatorSet(SLEDIndicator *psIndicator,
						 BOOL bLEDOn)
{
	ELCDErr eLCDErr;

	// Hide both images
	eLCDErr = WidgetSetHideByHandle((WIDGETHANDLE) psIndicator->eIndicatorOn,
								 WIDGET_IMAGE,
								 TRUE,
								 FALSE);
	GCASSERT(LERR_OK == eLCDErr);
	eLCDErr = WidgetSetHideByHandle((WIDGETHANDLE) psIndicator->eIndicatorOff,
								 WIDGET_IMAGE,
								 TRUE,
								 FALSE);
	GCASSERT(LERR_OK == eLCDErr);

	// Now turn on the appropriate image
	if (bLEDOn)
	{
		eLCDErr = WidgetSetHideByHandle((WIDGETHANDLE) psIndicator->eIndicatorOn,
									 WIDGET_IMAGE,
									 FALSE,
									 TRUE);
		GCASSERT(LERR_OK == eLCDErr);
	}
	else
	{
		eLCDErr = WidgetSetHideByHandle((WIDGETHANDLE) psIndicator->eIndicatorOff,
									 WIDGET_IMAGE,
									 FALSE,
									 TRUE);
		GCASSERT(LERR_OK == eLCDErr);
	}
}


static ELCDErr ButtonRenderSimple(WINDOWHANDLE eWindowHandle,
								  BUTTONHANDLE *peButtonHandle,
								  INT32 s32XPos,
								  INT32 s32YPos,
								  UINT32 u32XSize,
								  UINT32 u32YSize,
								  UINT32 u32ForegroundColor,
								  UINT32 u32TextColor,
								  char *pu8ButtonText)
{
	SImageGroup *psButtonUp = NULL;
	SImageGroup *psButtonDown = NULL;
	ELCDErr eLCDErr;
	UINT32 u32ProportionalFontSize;
	LEX_CHAR *peProportionalFontFilename = NULL;

	// Get the proportional font data
	TextGetProportionalFontData(&peProportionalFontFilename,
							    &u32ProportionalFontSize);

	u32ProportionalFontSize = 16;

	eLCDErr = ButtonCreate(sg_eDisplayWindow, 
							peButtonHandle,
							s32XPos,
							s32YPos);
	GCASSERT(LERR_OK == eLCDErr);

	eLCDErr = ButtonRender(u32XSize,
							u32YSize,
							peProportionalFontFilename,
							u32ProportionalFontSize,
							pu8ButtonText,
							u32ForegroundColor,
							u32TextColor,
							&psButtonUp,
							FALSE,
							ROT_0);
	GCASSERT(LERR_OK == eLCDErr);

	eLCDErr = ButtonRender(u32XSize,
							u32YSize,
							peProportionalFontFilename,
							u32ProportionalFontSize,
							pu8ButtonText,
							u32ForegroundColor,
							u32TextColor,
							&psButtonDown,
							TRUE,
							ROT_0);
	GCASSERT(LERR_OK == eLCDErr);
		
	eLCDErr = ButtonSetNormalImage(*peButtonHandle,
									psButtonUp,
									0, 0);
	GCASSERT(LERR_OK == eLCDErr);
	eLCDErr = ButtonSetPressedImage(*peButtonHandle,
									psButtonDown,
									0, 0);
	GCASSERT(LERR_OK == eLCDErr);

	eLCDErr = WidgetSetHideByHandle(*peButtonHandle,
									WIDGET_BUTTON,
									FALSE,
									TRUE);
	GCASSERT(LERR_OK == eLCDErr);

	return(eLCDErr);
}

static BUTTONHANDLE sg_eReset;
static BUTTONHANDLE sg_eReload;
static BUTTONHANDLE sg_eDumpRAM;

static void StatusWindowInit(void)
{
	ELCDErr eLCDErr;
	TEXTHANDLE eText;
	UINT32 u32ProportionalFontSize;
	LEX_CHAR *peProportionalFontFilename = NULL;
	FONTHANDLE eProportionalFont;

	eLCDErr = TextSetProportionalFont("files/Fonts/AdobeFanHeitiStd-Bold.otf",
								   NULL);
	GCASSERT(LERR_OK == eLCDErr);

	// Get the proportional font data
	TextGetProportionalFontData(&peProportionalFontFilename,
							    &u32ProportionalFontSize);

	u32ProportionalFontSize = 16;

	// Now go create the font
	eLCDErr = FontCreate(peProportionalFontFilename,
						 u32ProportionalFontSize,
						 0,
						 &eProportionalFont);
	GCASSERT(LERR_OK == eLCDErr);

	// Create a display window
	eLCDErr = WindowCreate(HANDLE_INVALID,
						   1000,
						   0,
						   800,
						   400,
						   &sg_eDisplayWindow);
	GCASSERT(LERR_OK == eLCDErr);

	eLCDErr = WindowSetVisible(sg_eDisplayWindow,
							   TRUE);
	GCASSERT(LERR_OK == eLCDErr);

	Seg7Init(sg_eDisplayWindow,
			 0, 0,
			 &sg_sDigit1);
	Seg7Init(sg_eDisplayWindow,
			 100, 0,
			 &sg_sDigit2);

	// Create some text for this
	eLCDErr = DefaultTextCreate(&eText,
								sg_eDisplayWindow,
								eProportionalFont,
								63, 160,
								0xffff,
								"POST LEDs");
	GCASSERT(LERR_OK == eLCDErr);

	// Mow create the state images
	eLCDErr = IndicatorCreate(sg_eDisplayWindow,
							  &sg_sRun,
							  200, 5,
							  FALSE);
	GCASSERT(LERR_OK == eLCDErr);

	eLCDErr = DefaultTextCreate(&eText,
								sg_eDisplayWindow,
								eProportionalFont,
								254, 18,
								0xffff,
								"CPU Run");
	GCASSERT(LERR_OK == eLCDErr);

	eLCDErr = IndicatorCreate(sg_eDisplayWindow,
							  &sg_sIRQNone,
							  200, 55,
							  FALSE);
	GCASSERT(LERR_OK == eLCDErr);

	eLCDErr = DefaultTextCreate(&eText,
								sg_eDisplayWindow,
								eProportionalFont,
								254, 73,
								0xffff,
								"No IRQ");
	GCASSERT(LERR_OK == eLCDErr);

	eLCDErr = IndicatorCreate(sg_eDisplayWindow,
							  &sg_sResetN,
							  200, 105,
							  FALSE);
	GCASSERT(LERR_OK == eLCDErr);
	eLCDErr = DefaultTextCreate(&eText,
								sg_eDisplayWindow,
								eProportionalFont,
								254, 123,
								0xffff,
								"!RESET");
	GCASSERT(LERR_OK == eLCDErr);

	eLCDErr = IndicatorCreate(sg_eDisplayWindow,
							  &sg_sReset,
							  200, 155,
							  FALSE);
	GCASSERT(LERR_OK == eLCDErr);
	eLCDErr = DefaultTextCreate(&eText,
								sg_eDisplayWindow,
								eProportionalFont,
								254, 173,
								0xffff,
								"RESET");
	GCASSERT(LERR_OK == eLCDErr);

	// Some control buttons
	eLCDErr = ButtonRenderSimple(sg_eDisplayWindow,
								 &sg_eReset,
								 510,
								 5,
								 90,
								 35,
								 0xffffff,
								 0x007f00,
								 "Reset CPU");
	GCASSERT(LERR_OK == eLCDErr);

	eLCDErr = ButtonRenderSimple(sg_eDisplayWindow,
								 &sg_eReload,
								 510,
								 47,
								 90,
								 35,
								 0xffffff,
								 0x007f00,
								 "Reload");
	GCASSERT(LERR_OK == eLCDErr);

	eLCDErr = ButtonRenderSimple(sg_eDisplayWindow,
								 &sg_eDumpRAM,
								 510,
								 89,
								 90,
								 35,
								 0xffffff,
								 0x007f00,
								 "Dump RAM");
	GCASSERT(LERR_OK == eLCDErr);

	eLCDErr = WindowCreate(HANDLE_INVALID,
						   1000,
						   400,
						   640,
						   480,
						   &sg_eConsoleTerminalWindow);
	GCASSERT(LERR_OK == eLCDErr);

	eLCDErr = WindowSetVisible(sg_eConsoleTerminalWindow,
							   TRUE);
	GCASSERT(LERR_OK == eLCDErr);

	eLCDErr = TerminalCreate(&sg_eTerminalHandle,
							 sg_eConsoleTerminalWindow,
							 0,
							 0,
							 80,
							 25);
	GCASSERT(LERR_OK == eLCDErr);

	eLCDErr = TerminalSetCharCallback(sg_eTerminalHandle,
									  ConsoleInputCharacter);
	GCASSERT(LERR_OK == eLCDErr);

	eLCDErr = WidgetSetHideByHandle(sg_eTerminalHandle,
									WIDGET_TERMINAL,
									FALSE,
									TRUE);
	GCASSERT(LERR_OK == eLCDErr);
}

static UINT8 sg_u8FlashSRAMMapping = 0;

static UINT8 *sg_pu8FlashImage = NULL;			// Place for flash image (assembled/normalized)
static UINT8 *sg_pu8SupervisorDRAM = NULL;		// Supervisor DRAM (all 16MB)
static UINT8 *sg_pu8SharedDRAM = NULL;			// Shared DRAM (Across all CPUs)
static UINT8 *sg_pu8FlagMap = NULL;				// Flag map. 0x80=Flash/handler 0, 0x81=Flash/Handler1, etc..., 0xff=Invalid, 0=RAM, 4K chunks
static unsigned long **sg_ppu32CPUOffsets = NULL; // CPU offsets (for handlers)

#define	FLASH_EVEN		"../RSC68kFlashEven.bin"
#define	FLASH_ODD		"../RSC68kFlashOdd.bin"

static void RSC68kLoadFlash(void)
{
	ELCDErr eErr;
	FILEHANDLE eFileHandle;
	UINT32 u32BytesRead = 0;
	UINT64 u64FileSizeEven;
	UINT64 u64FileSizeOdd;
	UINT8 *pu8DataTemp = NULL;
	UINT64 u64Loop;

	memset((void *) sg_pu8FlashImage, 0xff, RSC68KHW_SUPERVISOR_FLASH_SIZE<<1);

	eErr = FileSize(HANDLE_INVALID, FLASH_EVEN, &u64FileSizeEven);
	GCASSERT(GC_OK == eErr);
	eErr = FileSize(HANDLE_INVALID, FLASH_ODD, &u64FileSizeOdd);
	GCASSERT(GC_OK == eErr);

	// We are swapping even and odd due to endianness - assumed to be running on an x86

	// Load up the odd side
	eErr = FileOpen(&eFileHandle, FLASH_ODD, "rb");
	GCASSERT(GC_OK == eErr);
	pu8DataTemp = MemAlloc(u64FileSizeOdd);
	GCASSERT(pu8DataTemp);
	eErr = FileRead(eFileHandle,
					(void *) pu8DataTemp,
					(UINT32) u64FileSizeOdd,
					&u32BytesRead);
	GCASSERT(LERR_OK == eErr);
	for (u64Loop = 0; u64Loop < u64FileSizeOdd; u64Loop++)
	{
		sg_pu8FlashImage[u64Loop << 1] = pu8DataTemp[u64Loop];
	}
	eErr = FileClose(&eFileHandle);
	GCASSERT(LERR_OK == eErr);
	MemFree(pu8DataTemp);

	// Now the even side
	eErr = FileOpen(&eFileHandle, FLASH_EVEN, "rb");
	GCASSERT(GC_OK == eErr);
	pu8DataTemp = MemAlloc(u64FileSizeEven);
	GCASSERT(pu8DataTemp);
	eErr = FileRead(eFileHandle,
					(void *) pu8DataTemp,
					(UINT32) u64FileSizeEven,
					&u32BytesRead);
	GCASSERT(LERR_OK == eErr);
	for (u64Loop = 0; u64Loop < u64FileSizeEven; u64Loop++)
	{
		sg_pu8FlashImage[(u64Loop << 1) + 1] = pu8DataTemp[u64Loop];
	}
	eErr = FileClose(&eFileHandle);
	GCASSERT(LERR_OK == eErr);
	MemFree(pu8DataTemp);
}

static void RAMDump(void)
{
	FILE *psFile;

	psFile = fopen("s:/RSC68k/RAMDump.bin", "wb");
	GCASSERT(psFile);

	fwrite(sg_pu8SupervisorDRAM, 1, 16*1024*1024, psFile);
	fclose(psFile);
}

// Sets flash/SRAM mapping byte and appropriate handlers
static void FlashSRAMSetMapping(UINT8 u8Mapping)
{
	UINT8 u8Read;
	UINT8 *pu8Read;
	UINT32 u32Loop = 0;

//	DebugOut("%s: FlashSRAMSetMapping=0x%.2x\n", __FUNCTION__, u8Mapping);

	sg_u8FlashSRAMMapping = u8Mapping;

	// Set up read address handlers - bottom two bits
	u8Read = u8Mapping & 3;
	if (0 == u8Read)
	{
		// Lower 512K of flash
		pu8Read = (sg_pu8FlashImage - RSC68KHW_SUPERVISOR_FLASH);
	}
	else
	if (1 == u8Read)
	{
		// Upper 512K of flash
		pu8Read = (sg_pu8FlashImage - RSC68KHW_SUPERVISOR_FLASH) + RSC68KHW_SUPERVISOR_FLASH_SIZE;
	}
	else
	{
		// Upper memory region
		pu8Read = sg_pu8SupervisorDRAM - RSC68KHW_BASE_RAM; 
	}

	// Upper memory area
	for (u32Loop = RSC68KHW_SUPERVISOR_FLASH; u32Loop < (RSC68KHW_SUPERVISOR_FLASH + RSC68KHW_SUPERVISOR_FLASH_SIZE); u32Loop += (1 << 16))
	{
		sg_ppu32CPUOffsets[u32Loop >> 16] = (unsigned long *) pu8Read;
	}
}

static void RSC68kReset(unsigned long **ppu32CPUOffsets,
						REGS68K *psRegs,
						UINT8 *pu8FlagMap)
{
	UINT32 u32Loop;

	// Now reset the CPU
	RESET68K((unsigned long *) ppu32CPUOffsets, psRegs, 0x00);

	psRegs->ulRegSSP = sg_pu8FlashImage[2] | (sg_pu8FlashImage[3] << 8) | (sg_pu8FlashImage[0] << 16) | (sg_pu8FlashImage[1] << 24);
	psRegs->ulRegPC = sg_pu8FlashImage[6] | (sg_pu8FlashImage[7] << 8) | (sg_pu8FlashImage[4] << 16) | (sg_pu8FlashImage[5] << 24);

	sg_u32PeriodicTimer = 0;

	// Fill the supervisor and shared RAM with random stuff
	for (u32Loop = 0; u32Loop < 0x1000000; u32Loop++)
	{
		sg_pu8SupervisorDRAM[u32Loop] = (UINT8) rand();
	}

	for (u32Loop = 0; u32Loop < RSC68KHW_BASE_SHARED_RAM_SIZE; u32Loop++)
	{
		sg_pu8SharedDRAM[u32Loop] = (UINT8) rand();
	}

	// Set everything to read from flash, write to SRAM (default)
	FlashSRAMSetMapping(0);

	sg_s64TimerCounter = 0;
	sg_u64Seconds = 0;

	// Clear out any queued-up input
	sg_u8KeyQueueHead = 0;
	sg_u8KeyQueueTail = 0;
}

// **************************************
// Common device handlers
// **************************************

static unsigned char RSC68kDevComRead8(unsigned long u32Address)
{
	// UART A?
	if ((u32Address >= RSC68KHW_DEVCOM_UARTA) && (u32Address < RSC68KHW_DEVCOM_UARTB))
	{
		u32Address = (u32Address >> 1) & 0x07;
		return(UARTRead(&sg_sUARTA, (UINT8) u32Address));
	}

	// UART B?
	if ((u32Address >= RSC68KHW_DEVCOM_UARTB) && (u32Address < RSC68KHW_DEVCOM_IDE_CSA))
	{
		u32Address = (u32Address >> 1) & 0x07;
		return(UARTRead(&sg_sUARTB, (UINT8) u32Address));
	}

	// Flash/SRAM mapping?
	if ((u32Address >= RSC68KHW_DEVSPEC_FLASH_DRAM_MAP) && (u32Address < RSC68KHW_DEVSPEC_NIC))
	{
		return(sg_u8FlashSRAMMapping);
	}

	GCASSERT(0);
	return(0xff);
}

static void RSC68kDevComWrite8(unsigned long u32Address,
								   unsigned char u8Data)
{
	// UART A?
	if ((u32Address >= RSC68KHW_DEVCOM_UARTA) && (u32Address < RSC68KHW_DEVCOM_UARTB))
	{
		u32Address = (u32Address >> 1) & 0x07;
		UARTWrite(&sg_sUARTA, (UINT8) u32Address, u8Data);
		return;
	}

	// UART B?
	if ((u32Address >= RSC68KHW_DEVCOM_UARTB) && (u32Address < RSC68KHW_DEVCOM_IDE_CSA))
	{
		u32Address = (u32Address >> 1) & 0x07;
		UARTWrite(&sg_sUARTB, (UINT8) u32Address, u8Data);
		return;
	}

	// Flash/SRAM mapping?
	if ((u32Address >= RSC68KHW_DEVSPEC_FLASH_DRAM_MAP) && (u32Address < RSC68KHW_DEVSPEC_NIC))
	{
		FlashSRAMSetMapping(u8Data);
		return;
	}

	GCASSERT(0);
}

static unsigned short RSC68kDevComRead16(unsigned long u32Address)
{
	RAMDump();
	Sleep(1000);
	GCASSERT(0);
	return(0xff);
}

static void RSC68kDevComWrite16(unsigned long u32Address,
									unsigned short u16Data)
{
	// 7 Segment LEDs?
	if ((u32Address >= RSC68KHW_DEVCOM_POST_LED) && (u32Address < RSC68KHW_DEVCOM_UARTA))
	{
		Seg7Write1(0, (UINT8) (u16Data >> 8));
		Seg7Write2(0, (UINT8) u16Data);
		return;
	}

	// 8 Individual status LEDs?
	if ((u32Address >= RSC68KHW_DEVCOM_STATUS_LED) && (u32Address < RSC68KHW_DEVCOM_INTC_MASK))
	{
		return;
	}

	GCASSERT(0);
}

// **************************************
// CPU specific devices
// **************************************

// **************************************
// High page read/write for all CPUs
// **************************************

static unsigned char RSC68HighPageRead8(unsigned long u32Address)
{
	UINT8 u8Map = sg_u8FlashSRAMMapping & 3;

	if (0 == u8Map)
	{
		// Flash A18=0
		return(sg_pu8FlashImage[(u32Address - RSC68KHW_SUPERVISOR_FLASH)]);
	}
	else
	if (1 == u8Map)
	{
		// Flash A18=1
		return(sg_pu8FlashImage[(u32Address - RSC68KHW_SUPERVISOR_FLASH) + RSC68KHW_SUPERVISOR_FLASH_SIZE]);
	}
	else
	{
		// DRAM
		return(sg_pu8SupervisorDRAM[u32Address]);
	}
}

static void RSC68HighPageWrite8(unsigned long u32Address,
							    unsigned char u8Data)
{
	UINT8 u8Map = (sg_u8FlashSRAMMapping >> 2) & 3;

	if (0 == u8Map)
	{
		// DRAM
		sg_pu8SupervisorDRAM[u32Address] = u8Data;
	}
	else
	{
		// Don't write to flash
		GCASSERT(0);
	}	
}

static unsigned short RSC68HighPageRead16(unsigned long u32Address)
{
	UINT8 u8Map = sg_u8FlashSRAMMapping & 3;

	if (0 == u8Map)
	{
		// Flash A18=0
		return(*((UINT16 *) &sg_pu8FlashImage[(u32Address - RSC68KHW_SUPERVISOR_FLASH)]));
	}
	else
	if (1 == u8Map)
	{
		// Flash A18=1
		return(*((UINT16 *) &sg_pu8FlashImage[(u32Address - RSC68KHW_SUPERVISOR_FLASH) + RSC68KHW_SUPERVISOR_FLASH_SIZE]));
	}
	else
	{
		// DRAM
		return(*((UINT16 *) &sg_pu8SupervisorDRAM[u32Address]));
	}
}

static void RSC68HighPageWrite16(unsigned long u32Address,
								 unsigned short u16Data)
{
	UINT8 u8Map = (sg_u8FlashSRAMMapping >> 2) & 3;

	if (0 == u8Map)
	{
		// DRAM
		*((UINT16 *) (&sg_pu8SupervisorDRAM[u32Address])) = u16Data;
	}
	else
	{
		// Don't write to flash
		GCASSERT(0);
	}
}

static EMUHANDLERS3 sg_sEmuHandlers[] =
{
	{RSC68kDevComRead8,		RSC68kDevComWrite8,		RSC68kDevComRead16,		RSC68kDevComWrite16},	// Peripheral handler
	{RSC68HighPageRead8,	RSC68HighPageWrite8,	RSC68HighPageRead16,	RSC68HighPageWrite16},	// High page handler
};


static INT64 sg_s64TotalTimer;

#define		OFFSET_COUNT	(16*1024*1024) / (256*256)

void AppEntry(void *pvData)
{
	ECPUState eCPUState = ECPU_RUN;
	INT32 s32TimeAccumulator = 0;
	BOOL bHalted = TRUE;
	EButtonState eLastReset = BUTTON_STATE_ENABLED_NORMAL;
	EButtonState eReloadState = BUTTON_STATE_ENABLED_NORMAL;
	EButtonState eLastReload = BUTTON_STATE_ENABLED_NORMAL;
	EButtonState eDumpState = BUTTON_STATE_ENABLED_NORMAL;
	EButtonState eLastDump = BUTTON_STATE_ENABLED_NORMAL;
	REGS68K s68KRegs;
	int iClockTotal;
	unsigned char uIRQ = 0;
	unsigned long **ppu32CPUOffsets = NULL;
	int iCounter = 0;
	time_t sTime = time(0);
	UINT32 u32Loop;
	BOOL bDumped = FALSE;

	memset((void *) &s68KRegs, 0, sizeof(s68KRegs));
	StatusWindowInit();

	IndicatorSet(&sg_sRun, TRUE);

	// Allocate memory for our flash image
	sg_pu8FlashImage = MemAlloc(RSC68KHW_SUPERVISOR_FLASH_SIZE<<1);
	GCASSERT(sg_pu8FlashImage);
	memset((void *) sg_pu8FlashImage, 0xff, RSC68KHW_SUPERVISOR_FLASH_SIZE<<1);

	// CPU offsets
	ppu32CPUOffsets = MemAlloc(sizeof(*ppu32CPUOffsets) * OFFSET_COUNT);
	GCASSERT(ppu32CPUOffsets);
	sg_ppu32CPUOffsets = ppu32CPUOffsets;

	// Supervisor DRAM - full 16 megabytes
	sg_pu8SupervisorDRAM = MemAlloc(16*1024*1024);
	GCASSERT(sg_pu8SupervisorDRAM);
	memset((void *) sg_pu8SupervisorDRAM, 0xff, 16*1024*1024);

	// Shared DRAM
	sg_pu8SharedDRAM = MemAlloc(RSC68KHW_BASE_SHARED_RAM_SIZE);
	GCASSERT(sg_pu8SharedDRAM);

	// Flag map for the entire 16MB address space
	sg_pu8FlagMap = MemAlloc((16*1024*1024) / 4096);
	GCASSERT(sg_pu8FlagMap);

	// Set up our RAM offsets for the supervisor DRAM
	for (u32Loop = RSC68KHW_BASE_RAM; u32Loop < 0x1000000; u32Loop += (1 << 16))
	{
		sg_ppu32CPUOffsets[u32Loop >> 16] = (unsigned long *) (sg_pu8SupervisorDRAM - RSC68KHW_BASE_RAM);
	}
	memset((void *) &sg_pu8FlagMap[RSC68KHW_BASE_RAM >> 12], 0, RSC68KHW_BASE_RAM_SIZE >> 12);

	// Now the shared RAM
	for (u32Loop = RSC68KHW_BASE_SHARED_RAM; u32Loop < (RSC68KHW_BASE_SHARED_RAM + RSC68KHW_BASE_SHARED_RAM_SIZE); u32Loop += (1 << 16))
	{
		sg_ppu32CPUOffsets[u32Loop >> 16] = (unsigned long *) (sg_pu8SharedDRAM - RSC68KHW_BASE_SHARED_RAM);
	}
	memset((void *) &sg_pu8FlagMap[RSC68KHW_BASE_SHARED_RAM >> 12], 0, RSC68KHW_BASE_SHARED_RAM_SIZE >> 12);

	// Handler index 1
	memset((void *) &sg_pu8FlagMap[RSC68KHW_SUPERVISOR_FLASH >> 12], 0xc1, RSC68KHW_SUPERVISOR_FLASH_SIZE >> 12);

	// Hook up the handlers for the peripheral
	for (u32Loop = RSC68KHW_DEVCOM_BASE; u32Loop < 0x1000000; u32Loop += (1 << 12))
	{
		// 0th index into the handler - reads and writes
		sg_pu8FlagMap[u32Loop >> 12] = 0xc0;
	}

	// Force the flash image(s) to be loaded
	sg_bReload = TRUE;

	// Reset both UARTs
	UARTReset(&sg_sUARTA);
	UARTReset(&sg_sUARTB);

	// Install handlers
	sg_sUARTA.DataTX = ConsoleUARTTX;

	while (1)
	{
		EButtonState eResetState;
		ELCDErr eLCDErr;

		// If this is set, we want to reload the flash and reset everything
		if (sg_bReload)
		{
			// Load the flash images
			RSC68kLoadFlash();

			// Now reset the CPU
			RSC68kReset(ppu32CPUOffsets,
						&s68KRegs,
						sg_pu8FlagMap);

			// So we don't ereload anything
			sg_bReload = FALSE;

			// Reset the display - random state coming up
			Seg7Write1(0, 0xa5);
			Seg7Write2(0, 0x5a);
		}

		// See if we should reset the CPU
		eLCDErr = ButtonGetState(sg_eReset,
							     &eResetState);

		if (eResetState != eLastReset)
		{
			if (eResetState)
			{
				RSC68kReset(ppu32CPUOffsets,
							&s68KRegs,
							sg_pu8FlagMap);

				// Reset the display
				Seg7Write1(0, 0xff);
				Seg7Write2(0, 0xff);
			}

			eLastReset = eResetState;
		}

		iClockTotal = (CPU_SPEED / EMU_FPS) + iCounter;
		iCounter = 0;

		uIRQ = 0;

// # Of clocks for each exec chunk
#define CPU_EXEC_CHUNK		500

		// Execute a frame's worth of emulation
		while (iCounter < iClockTotal)
		{
			int s32Delta;
			int s32CountStart;
			
			s32Delta = iClockTotal - iCounter;
			if (s32Delta > CPU_EXEC_CHUNK)
			{
				s32Delta = CPU_EXEC_CHUNK;
			}

			s32CountStart = s32Delta;

			EXEC68K((unsigned char *) sg_pu8FlagMap,
					&s68KRegs,
					sg_sEmuHandlers,
					&s32Delta,
					&uIRQ,
					0,
					(unsigned long *) ppu32CPUOffsets);

			// s32Delta contains the # of clocks we've just executed
			s32Delta = s32CountStart - s32Delta;
			UARTStep(&sg_sUARTA,
					 s32Delta);
			UARTStep(&sg_sUARTB,
					 s32Delta);
			iCounter += s32Delta;
			sg_u32PeriodicTimer += s32Delta;
			sg_s64TotalTimer += s32Delta;
		}


		// If we have no data in our buffer, then potentially dequeue character data
		if (0 == sg_sUARTA.u8RXCount)
		{
			if (sg_u8KeyQueueHead == sg_u8KeyQueueTail)
			{
				// No data available
			}
			else
			{
				UINT8 u8Char;

				u8Char = sg_u8KeyQueue[sg_u8KeyQueueTail++];
				if (sg_u8KeyQueueTail >= sizeof(sg_u8KeyQueue))
				{
					sg_u8KeyQueueTail = 0;
				}

				UARTRXData(&sg_sUARTA,
						   u8Char);
			}
		}

		sg_s64TimerCounter += iCounter;
		if (sg_s64TimerCounter > CPU_SPEED)
		{
			sg_s64TimerCounter -= CPU_SPEED;
			sg_u64Seconds++;
			DebugOut("Seconds=%llu\n", sg_u64Seconds);
		}
	
//		DebugOut("Ticks = %I64d, counter = %d\n", sg_s64TotalTimer, iCounter);
		GCWaitForVsync();

		iCounter = iClockTotal - iCounter;

		// See if we should reload
		eLCDErr = ButtonGetState(sg_eReload,
								 &eReloadState);
		if (eReloadState != eLastReload)
		{
			if (BUTTON_STATE_ENABLED_PRESSED == eReloadState)
			{
				sg_bReload = TRUE;
			}

			eLastReload = eReloadState;
		}

		eLCDErr = ButtonGetState(sg_eDumpRAM,
								 &eDumpState);
		// See if we should dump
		if (eDumpState != eLastDump)
		{
			if (BUTTON_STATE_ENABLED_PRESSED == eDumpState)
			{
				RAMDump();
				DebugOut("Dumped RAM\n");
			}

			eLastDump = eDumpState;
		}
	}
}