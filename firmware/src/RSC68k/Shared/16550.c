#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "Shared/16550.h"

// Sends one or more bytes out the serial port via programmed I/O
void SerialSendPIO(S16550UART *psUART,
				   uint8_t *pu8DataToSend,
				   uint32_t u32DataCount)
{
	while (u32DataCount)
	{
		while (0 == (psUART->u8LSR & UART_LSR_THRE))
		{
			// Wait for THRE to be empty
		}

		psUART->u8RBR = *pu8DataToSend;

		--u32DataCount;
		++pu8DataToSend;
	}
}

// Waits for a byte to become available, then returns it
uint8_t SerialReceiveWaitPIO(S16550UART *psUART)
{
	while (0 == (psUART->u8LSR & UART_LSR_DR))
	{
		// Wait for data to be ready
	}

	return(psUART->u8RBR);
}

// Returns true if data is available
bool SerialDataAvailablePIO(S16550UART *psUART)
{
	if (psUART->u8LSR & UART_LSR_DR)
	{
		return(true);
	}
	else
	{
		return(false);
	}
}

// Initializes the 16550. true If successful, otherwise false.
bool SerialInit(S16550UART *psUART,
				uint8_t u8DataBits,
				uint8_t u8StopBits,
				EUARTParity eParity)
{
	uint8_t u8LCR = 0;
	bool bResult = false;

	// Let's see if the UART is there. First check the scratch register
	psUART->u8SCR = 0xa5;
	if (psUART->u8SCR != 0xa5)
	{
		goto errorExit;
	}

	psUART->u8SCR = 0x5a;
	if (psUART->u8SCR != 0x5a)
	{
		goto errorExit;
	}

	// Scratch register appears to be there. Set the IIR to 0 - it should return
	// UART_IIR_NOT_PENDING
	psUART->u8IIR = 0;
	if (psUART->u8IIR != UART_IIR_NOT_PENDING)
	{
		goto errorExit;
	}

	// Now we set up a line crontol register variable
	if ((u8DataBits < 5) || (u8DataBits > 8))
	{
		// Only supports 5-8 data bits
		goto errorExit;
	}

	// 5-8 data bits is 0-3 in the UART, lower 2 bits
	u8LCR = (u8DataBits - 5);

	// Stop bits are either 1 or 2
	if ((u8StopBits != 1) && (u8StopBits != 2))
	{
		// Only supports 1 or 2 stop bits
		goto errorExit;
	}

	u8StopBits--;
	u8LCR |= (u8StopBits << 2);

	if (EUART_PARITY_NONE == eParity)
	{
		u8LCR |= UART_LCR_NO_PARITY;
	}
	else
	if (EUART_PARITY_EVEN == eParity)
	{
		u8LCR |= UART_LCR_EVEN_PARITY;
	}
	else
	if (EUART_PARITY_NONE == eParity)
	{
		u8LCR |= UART_LCR_ODD_PARITY;
	}
	else
	{
		// Bad parity setting
		goto errorExit;
	}

	psUART->u8LCR = u8LCR;
	bResult = true;

	// All good

errorExit:
	return(bResult);
}

// Sets the 16550's baud rate
void SerialSetBaudRate(S16550UART *psUART,
					   uint32_t u32BaudClock,
					   uint32_t u32BaudRateDesired,
					   uint32_t *pu32BaudRateObtained)
{
	uint16_t u16Divisor;
	uint8_t u8LCR;

	u16Divisor = (uint16_t) ((u32BaudClock >> 4) / u32BaudRateDesired);

	if (pu32BaudRateObtained)
	{
		*pu32BaudRateObtained = ((u32BaudClock >> 4) / u16Divisor);
	}

	// Read in the line control register's value and OR in DLAB for
	// the baud rate divisor latch access
	psUART->u8LCR = psUART->u8LCR | UART_LCR_DLAB_ENABLE;

	// Set LSB of the divisor
	psUART->u8RBR = (uint8_t) u16Divisor;

	// And the MSB
	psUART->u8IER = (uint8_t) (u16Divisor >> 8);

	// Now clear DLAB
	psUART->u8LCR = psUART->u8LCR & ~UART_LCR_DLAB_ENABLE;
}

// Sets the DTR/RTS/OUT1/OUT2 pins
void SerialSetOutputs(S16550UART *psUART,
 					  bool bDTR,
 					  bool bRTS,
 					  bool bOUT1,
					  bool bOUT2)
{
	uint8_t u8MCR = 0;

	if (bDTR)
	{
		u8MCR |= UART_MCR_DTR;
	}

	if (bRTS)
	{
		u8MCR |= UART_MCR_RTS;
	}

	if (bOUT1)
	{
		u8MCR |= UART_MCR_OUT1;
	}

	if (bOUT2)
	{
		u8MCR |= UART_MCR_OUT2;
	}

	psUART->u8MCR = u8MCR;
}
