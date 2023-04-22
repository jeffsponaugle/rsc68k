// RSC68k - Power control module
//
// Pin/port definitions:
//
// PA0			- Power button (input)
// PA1			- Reset button (input)
// PA2			- Power LED (output)
// PA3			- System reset (output, active low)
// PA7/PCINT7	- PWROK Signal from power supply (input)
// PB0			- PSON Signal to power supply (output)
// PB1			- Spare (output)
// PB2			- PWR Grant/request (input/output)

#include <stdint.h>
#include <xc.h>
#include <stdbool.h>
#include <avr/io.h>
#include <avr/interrupt.h>

// CPU speed (in hz)
#define CPU_SPEED				8000000

// Timer prescaler
#define TIMER_PRESCALER			1024

// Timer frequency in hz (100hz)
#define TIMER_FREQ				100

// Timer tick frequency
#define TIMER_TICK				(1000 / TIMER_FREQ)

// Divisor
#define TIMER_DIVISOR			((CPU_SPEED / TIMER_PRESCALER) / (TIMER_FREQ))

// Tuneables

// Amount of time (in milliseconds) between PSON to PWROK asserting before we consider
// it a fault condition.
#define PSON_TO_PWROK			2000

// Amount of time (milliseconds) between PWROK and reset being deasserted.
#define PWROK_TO_RESET			150

// Button debounce time (# of milliseconds it must be in a steady state before it's
// considered pressed/released)
#define BUTTON_DEBOUNCE			50

// Power button override time - like the 4 second hold on the power button to power off a system
#define POWER_BUTTON_OVERRIDE	4000

// How long (in milliseconds) do we wait until we deassert PWRGR request/grant set
#define POWER_REQUEST_TIME		20

// State of power control module
typedef enum
{
	ESTATE_INIT,
	ESTATE_MONITOR,
	ESTATE_POWER_ON,
	ESTATE_WAIT_PWROK,
	ESTATE_WAIT_RESET_DEASSERT,
	ESTATE_POWER_OFF,
	ESTATE_WAIT_REQUEST_TIME
} EPowerState;

// LED Alert states
typedef enum
{
	EALERT_OFF,
	EALERT_POWERING_UP,
	EALERT_POWER_FAILURE,
	EALERT_POWER_DOWN_PENDING,
	EALERT_ON,
} EAlerts;

// Macros for accessing various functions. Note that TRUE means *ASSERTED*
// not a logic 1 necessarily. This deals in logical state
#define BUTTON_POWER()		((PINA & (1 << PORTA0)) ? false : true)
#define BUTTON_RESET()		((PINA & (1 << PORTA1)) ? false : true)
#define LED_POWER_SET(x)	if (x) {cli(); PORTA &= (uint8_t) ~(1 << PORTA2); sei();} else {cli(); PORTA |= (1 << PORTA2); sei();}
#define RESET_SET(x)		if (x) {cli(); PORTA &= (uint8_t) ~(1 << PORTA3); DDRA |= (1 << DDA3); sei();} else { cli(); DDRA &= (uint8_t) ~(1 << DDA3); PORTA |= (uint8_t) (1 << PORTA3); sei();}
#define LED_SPARE_SET(x)	if (x) {cli(); PORTB |= (1 << PORTB1); sei();} else {cli(); PORTB &= (uint8_t) ~(1 << PORTB1); sei();}
#define PWR_OK()			((PINA & (1 << PINA7)) ? true : false)
#define PSON_SET(x)			if (x) {cli(); PORTB &= (uint8_t) ~(1 << PORTB0); sei();} else {cli(); PORTB |= (1 << PORTB0); sei();}
#define PWRGR_REQ_SET(x)	if (x) {cli(); INT0_PORT &= (uint8_t) ~(1 << INT0_BIT); INT0_DDR |= (1 << INT0_BIT); sei();} else {cli(); INT0_DDR &= (uint8_t) ~(1 << INT0_BIT); INT0_PORT |= (1 << INT0_BIT); sei();}
#define PWR_REQ_GET()		((INT0_PIN & (1 << INT0_BIT)) ? false : true)

// Other useful macros
#define	IRQ_PWRGR_REQ_ENABLE()				GIMSK |= (1 << INT0);
#define	IRQ_PWRGR_REQ_DISABLE()				GIMSK &= ~(1 << INT0);
#define	IRQ_PWR_OK_ENABLE()					GIMSK |= (1 << PCIE0);
#define	IRQ_PWR_OK_DISABLE()				GIMSK &= ~(1 << PCIE0);


// # Of timer ticks (ever increasing)
static volatile uint16_t sg_u16TimerTicks = 0;

// Returns true if the EEPROM is busy
static bool EEPROMBusy(void)
{
	if (EECR & (1 << EEPE))
	{
		return(true);
	}
	else
	{
		return(false);
	}
}

// Write a byte to the EEPROM. NOTE: Consumers of this function
// must call EEPROMBusy() repeatedly until it returns false before
// calling this function.
static void EEPROMWrite(uint8_t u8Address,
						uint8_t u8Data)
{
	// Set programming mode
	EECR = (0 << EEPM1) | (0 << EEPM0);
	
	// Set the address
	EEAR = u8Address;

	// Now the data register	
	EEDR = u8Data;
	
	// Start write
	EECR |= (1 << EEMPE);
	EECR |= (1 << EEPE);
}

// Read a byte from the EEPROM
static uint8_t EEPROMRead(uint8_t u8Address)
{
	// Set the address
	EEAR = u8Address;

	// Flag a read
	EECR |= (1 << EERE);
	
	return(EEDR);	
}

// Power and reset button debounce
static uint8_t sg_u8PowerButtonDebounce;
static bool sg_bPowerButtonState;
static uint16_t sg_u16PowerOverrideTimer;
static uint8_t sg_u8ResetButtonDebounce;
static bool sg_bResetButtonState;

// And generic, logical states
static bool sg_bPowerRequest;				// Set true if someone has requested a power request
static bool sg_bPowerOverrideHeld;			// Set true if someone held the power button long enough for an override
static bool sg_bResetButtonReleased;		// Set true if someone has released the reset button
static bool sg_bCoordinatedShutdownMode;	// Set true if we've had a coordinated shutdown mode requested
static bool sg_bCoordinatedShutdown;		// Set true if the host has requested we shut down
static bool sg_bPowerFault;					// Set true if there was a power fault condition (unexpected loss of PSPWRGOOD)

// Read the timer ticks
static uint16_t TimerReadTicks(void)
{
	uint16_t u16Ticks;
	
	cli();
	u16Ticks = sg_u16TimerTicks;
	sei();
	return(u16Ticks);
}

// LED Pattern for various LED states
typedef struct SAlertLEDPattern
{
	const uint16_t u16PatternLengthInBits;
	const uint8_t *pu8PatternBase;
} SAlertLEDPattern;

static const uint8_t sg_u8LEDOff[] =
{
	0x00
};

static const uint8_t sg_u8LEDOn[] =
{
	0x01
};

static const uint8_t sg_u8LEDPoweringUp[] =
{
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t sg_u8LEDPowerFault[] =
{
	0xff, 0xff, 0x0f, 0x0, 0x0, 0x0, 0xfc, 0xff, 0x3f, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x00
};

// Array of LED patterns/stipples
static const SAlertLEDPattern sg_sLEDAlertPatterns[] =
{
	{1, sg_u8LEDOff},			// EALERT_OFF
	{100, sg_u8LEDPoweringUp},	// EALERT_POWERING_UP	(50% duty cycle per second)
	{150, sg_u8LEDPowerFault},	// EALERT_POWER_FAILURE
	{100, sg_u8LEDPoweringUp},	// EALERT_POWER_DOWN_PENDING (same as up)
	{1, sg_u8LEDOn},			// EALERT_ON
};

static const uint8_t *sg_pu8LEDStippleBase;
static uint16_t sg_u16LEDIndex;
static uint16_t sg_u16LEDIndexMax;

static void AlertSetState(EAlerts eAlertState)
{
	cli();
	sg_pu8LEDStippleBase = sg_sLEDAlertPatterns[eAlertState].pu8PatternBase;
	sg_u16LEDIndexMax = sg_sLEDAlertPatterns[eAlertState].u16PatternLengthInBits;
	sg_u16LEDIndex = 0;
	sei();
}

// Timer interrupt. Called once per TIMER_PERIOD milliseconds
ISR(TIM0_COMPA_vect, ISR_NOBLOCK)
{
	bool bState;
	
	sg_u16TimerTicks++;
	
	// Handle LED state
	if (sg_pu8LEDStippleBase[sg_u16LEDIndex >> 3] & (1 << (sg_u16LEDIndex & 7)))
	{
		bState = true;
	}
	else
	{
		bState = false;
	}
	
	// Set the power LED's state
	LED_POWER_SET(bState);
	
	++sg_u16LEDIndex;
	if (sg_u16LEDIndex >= sg_u16LEDIndexMax)
	{
		sg_u16LEDIndex = 0;
	}
	
	// Handle power button
	bState = BUTTON_POWER();
	if (bState != sg_bPowerButtonState)
	{
		sg_u8PowerButtonDebounce++;
		if (sg_u8PowerButtonDebounce >= (BUTTON_DEBOUNCE / TIMER_TICK))
		{
			// We've changed state!

			// If not pressed to pressed..
			if (bState)			
			{
				// We've gone from not pressed to pressed
				sg_bPowerRequest = true;
				sg_u16PowerOverrideTimer = 0;
			}
			else
			{
				// We've gone from pressed to released
			}
			
			// Record our new state
			sg_bPowerButtonState = bState;
			sg_u8PowerButtonDebounce = 0;
			sg_u16PowerOverrideTimer = 0;
			sg_bPowerOverrideHeld = false;
		}
	}
	else
	{
		// It's steady. Keep the debounce
		sg_u8PowerButtonDebounce = 0;
	}
	
	// If the power button is held and continues to be held, then increase our override counter
	// until it overflows.
	if (sg_bPowerButtonState && bState)
	{
		if (sg_u16PowerOverrideTimer >= (POWER_BUTTON_OVERRIDE / TIMER_TICK))
		{
			// No further!
		}
		else
		{
			sg_u16PowerOverrideTimer++;
			if (sg_u16PowerOverrideTimer >= (POWER_BUTTON_OVERRIDE / TIMER_TICK))
			{
				sg_bPowerOverrideHeld = true;
			}
		}
	}
	
	// And the reset button
	bState = BUTTON_RESET();
	if (bState != sg_bResetButtonState)
	{
		sg_u8ResetButtonDebounce++;
		if (sg_u8ResetButtonDebounce >= (BUTTON_DEBOUNCE / TIMER_TICK))
		{
			// We've changed state!

			// If not pressed to pressed..
			if (bState)
			{
				// Assert reset
				RESET_SET(true);
			}
			else
			{
				// We've gone from pressed to released
				sg_bResetButtonReleased = true;
			}
			
			// Record our new state
			sg_bResetButtonState = bState;
			sg_u8ResetButtonDebounce = 0;
		}
	}
	else
	{
		// It's steady. Keep the debounce
		sg_u8ResetButtonDebounce = 0;
	}
}

// Falling edge of INT0/PB2 is a controlled power request
ISR(EXT_INT0_vect, ISR_NOBLOCK)
{
	if (false == sg_bCoordinatedShutdownMode)
	{
		// This is to indicate a coordinated shutdown
		sg_bCoordinatedShutdownMode = true;
	}
	else
	{
		// This is a power off request
		sg_bCoordinatedShutdown = true;
		
		// And we power off
		sg_bPowerRequest = true;
	}
}

// Look for PWR_GOOD going away
ISR(PCINT0_vect, ISR_NOBLOCK) 
{
	if (PWR_OK())
	{
		// Don't care about positive going power changes
	}
	else
	{
		// Stop both interrupts
		IRQ_PWRGR_REQ_DISABLE();		// Stop INT0/Power grant/request interrupts
		IRQ_PWR_OK_DISABLE();		// Shut off PWR_OK interrupts
		
		// Power fault! Assert reset.
		RESET_SET(true);
		
		// Deassert PSON
		PSON_SET(false);
		
		// Signal a power fault
		sg_bPowerFault = true;
	}
}

// EEPROM Definitions for power state retention
#define POWER_STATE_RETENTION_ADDRESS	0x00
#define POWER_STATE_ON					0x4c
#define POWER_STATE_OFF					0xeb

int main(void)
{
	EPowerState eState = ESTATE_INIT;
	bool bResetAssert = false;
	bool bPowerOn = false;
	uint16_t u16Timestamp;
	
	cli();
	
	// First thing - read PWROK signal from the power supply. If it's asserted,
	// we need to be certain we're asserting PSON in order to prevent glitching.
	
	// Set up all of port A properly. 1 Is an output, 0 is an input.
	DDRA = (1 << PORTA2);
	
	// Enable pullups on all inputs
	PORTA = (1 << PORTA0) | (1 << PORTA1) | (1 << PORTA3) | (1 << PORTA7);

	// If PWR_OK is asserted, then keep 
	if (PWR_OK())
	{
		// Power good is OK, assert PSON.
		
		// Drive PB0 low to keep PSON
		PORTB &= (uint8_t) ~(1 << PORTB0);
		
		// Jump to the powered-on state
		eState = ESTATE_MONITOR;
	}
	else
	{
		// Power good is not asserted, deassert PSON and assert reset.
		// Drive PB0 high to deassert PSON
		PORTB |= (1 << PORTB0);
		
		// Ensure reset gets asserted
		bResetAssert = true;
	}
	
	// Set as output for PSON and spare
	DDRB = (1 << DDB0) | (1 << DDB1);
	
	// Assert reset accordingly
	RESET_SET(bResetAssert);

	// Turn off the power LED
	LED_POWER_SET(false);

	// Turn off the spare LED
	LED_SPARE_SET(false);
	
	// Turn off the power LED
	AlertSetState(EALERT_OFF);
	
	// Sample the state of the power and reset buttons
	sg_bPowerButtonState = BUTTON_POWER();
	sg_bResetButtonState = BUTTON_RESET();
	
	// Ensure the power request/grant signal is set as an input and not an output
	PWRGR_REQ_SET(false);
	
	// Set clock prescaler to /1 for 8mhz internal operation
	CLKPR = (1 << CLKPCE);
	CLKPR = 0;
	
	// Now init timer 0
	
	// TCCR0A - pins disconnected, CTC mode, reset at OCRA
	TCCR0A = (1 << WGM01);
	
	// Prescaler to /1024
	TCCR0B = (1 << CS02) | (1 << CS00);
	
	// Counter values
	TCNT0 = 0;
	OCR0A = TIMER_DIVISOR;
	
	// Init interrupt on A
	TIMSK0 |= (1 << OCIE0A);
	
	// Enable all interrupts!
	sei();
	
	u16Timestamp = TimerReadTicks();
	
	// Loop forever
	for (;;)	
	{
		switch (eState)
		{
			case ESTATE_INIT:
			{
				uint8_t u8Data;
				
				// Figure out of power state retention should shut off or turn on power.
				u8Data = EEPROMRead(POWER_STATE_RETENTION_ADDRESS);
				if (POWER_STATE_ON == u8Data)
				{
					// Power on!
					eState = ESTATE_POWER_ON;
				}
				else
				{
					// No matter the state, power off and record as such
					eState = ESTATE_POWER_OFF;
				}
				
				break;
			}
			
			case ESTATE_MONITOR:
			{
				// Here's where we're looking for things to do
				
				// If power is on and they're holding the power button down, do a forced shutoff.
				if ((sg_bPowerOverrideHeld) &&
					(bPowerOn))
				{
					sg_bPowerRequest = true;
				}
				
				// If they're pressing the power button and power is currently off, turn power on
				if (sg_bPowerRequest)
				{
					// If power is off, turn power on
					if (false == bPowerOn)
					{
						// Power is off. Turn it on.
						eState = ESTATE_POWER_ON;
					}
					else
					{
						// We're doing a power off, but what kind? If it's not a coordinated shutdown, then
						// just power off
						if ((sg_bCoordinatedShutdownMode) &&
							(false == sg_bPowerOverrideHeld))
						{
							IRQ_PWRGR_REQ_DISABLE();	// Stop INT0/Power grant/request interrupts
							
							// Assert the power grant/request signal
							PWRGR_REQ_SET(true);
							
							// Sample our timestamp
							u16Timestamp = TimerReadTicks();
							
							eState = ESTATE_WAIT_REQUEST_TIME;
						
							// Indicate we're doing a coordinated power down and we're waiting for the host to power us down
							AlertSetState(EALERT_POWER_DOWN_PENDING);
						}
						else
						{
							// Shut things off
							eState = ESTATE_POWER_OFF;
						}
						
						EEPROMWrite(POWER_STATE_RETENTION_ADDRESS,
									POWER_STATE_OFF);
					}
					
					// Clear the fact that we've caught a power request
					sg_bPowerRequest = false;
				}

				// If we've released reset and power is on, then deassert after the reset pulse time
				if (sg_bResetButtonReleased)
				{
					// Only pay attention to it if power is on
					if (bPowerOn)
					{
						// We've released a reset button! Let's delay, then come back to monitor.
						u16Timestamp = TimerReadTicks();
							
						eState = ESTATE_WAIT_RESET_DEASSERT;
					}
					
					// We always clear the reset button release state
					sg_bResetButtonReleased = false;
				}
				
				if ((sg_bCoordinatedShutdown) || (sg_bPowerFault))
				{
					// We're doing a coordinated shutdown!
					sg_bCoordinatedShutdown = false;
					
					// Shut things off
					eState = ESTATE_POWER_OFF;
				}
				break;
			}
			
			case ESTATE_POWER_ON:
			{
				// We're powering on! 

				IRQ_PWRGR_REQ_DISABLE();	// Stop INT0/Power grant/request interrupts
				IRQ_PWR_OK_DISABLE();		// Shut off PWR_OK interrupts
								
				// Set the power LED that we're powering up
				AlertSetState(EALERT_POWERING_UP);
				
				// Assert reset
				RESET_SET(true);
				
				// Turn PSON
				PSON_SET(true);
				
				// Sample our timestamp
				u16Timestamp = TimerReadTicks();
				
				// It's not a coordinated shutdown if we're powering up
				sg_bCoordinatedShutdown = false;
				
				// Now we wait for PWROK
				eState = ESTATE_WAIT_PWROK; 				
				break;
			}
			
			case ESTATE_WAIT_PWROK:
			{
				// Waiting for a power OK signal after being powered on
				if (PWR_OK())
				{
					// We have power OK! Deassert reset and change states

					// Deassert reset
					RESET_SET(false);
					
					// Sample our timestamp
					u16Timestamp = TimerReadTicks();
					
					// And jump to reset
					eState = ESTATE_WAIT_RESET_DEASSERT;

					// Write the fact that we're powered up
					EEPROMWrite(POWER_STATE_RETENTION_ADDRESS,
								POWER_STATE_ON);
								
					// Set the LED state accordingly
					AlertSetState(EALERT_ON);
				}
				else
				if ((TimerReadTicks() - u16Timestamp) >= (PSON_TO_PWROK / TIMER_TICK))
				{
					// Fault condition
					
					// Force a power off condition
					eState = ESTATE_POWER_OFF;

					// Indicate a power fault/failure
					sg_bPowerFault = true;
				}
				else
				{
					// Keep waiting until we have our signal
				}
				break;
			}
			
			case ESTATE_WAIT_RESET_DEASSERT:
			{
				// If we've held reset long enough, deassert it and consider ourselves
				// powered on
				if ((TimerReadTicks() - u16Timestamp) >= (PWROK_TO_RESET / TIMER_TICK))
				{
					// We are powered up now. Let's make sure the EEPROM isn't busy
					if (EEPROMBusy()) 
					{
						// Fall through
					}
					else
					{
						eState = ESTATE_MONITOR;
						
						// Indicate we're powered on
						bPowerOn = true;
						
						// Configure the INT0 interrupt so we catch power grant/requests
						MCUCR |= 1 << ISC01;		// Configure INT0 to interrupt on falling edge of INT0 pin
						MCUCR &= ~(1 << ISC00);		// .. interrupt at any logical change at INT0
						IRQ_PWRGR_REQ_ENABLE();		// Enable INT0 interrupt
						
						// Now PCINT0 to catch PWR_OK on PCINT7
						PCMSK0 |= (1 << PCINT7_BIT);
						PCMSK1 |= (1 << PCINT10_BIT);
						IRQ_PWR_OK_ENABLE();
						
						// Deassert reset
						RESET_SET(false);
					}
				}
							
				break;
			}
			
			case ESTATE_POWER_OFF:
			{
				IRQ_PWRGR_REQ_DISABLE();	// Stop INT0/Power grant/request interrupts
				IRQ_PWR_OK_DISABLE();		// Shut off PWR_OK interrupts
				
				// Assert reset
				RESET_SET(true);
				
				// Deassert PWRON
				PSON_SET(false);

				// Release the power/grant request				
				PWRGR_REQ_SET(false);
	
				// Wait for PWROK to go away
				if (PWR_OK())				
				{
					// Keep waiting
				}
				else
				// Wait until the EEPROM isn't busy
				if (EEPROMBusy())
				{
					// Keep looping
				}
				else
				{
					// Indicate power is now shut off
					eState = ESTATE_MONITOR;
					
					// Indicate we're powered off
					bPowerOn = false;
					
					// Clear any pending activities
					sg_bPowerRequest = false;
					sg_bPowerOverrideHeld = false; 
					sg_bCoordinatedShutdownMode = false;
					sg_bCoordinatedShutdown = false;
					
					if (sg_bPowerFault)
					{
						sg_bPowerFault = false;
						
						// Indicate fault state
						AlertSetState(EALERT_POWER_FAILURE);
					}
					else
					{
						// Set the LED state accordingly
						AlertSetState(EALERT_OFF);
					}
				}
				
				break;
			}
			
			case ESTATE_WAIT_REQUEST_TIME:
			{
				// Wait until we there's enough time that has passed for the power request
				if ((TimerReadTicks() - u16Timestamp) >= (POWER_REQUEST_TIME / TIMER_TICK))
				{
					// Deassert it and go back to monitoring
					PWRGR_REQ_SET(false);

					IRQ_PWRGR_REQ_ENABLE();		// Enable INT0/Power grant/request interrupts

					eState = ESTATE_MONITOR;
				}
				
				break;
			}

			default:
			{
				// Shouldn't get here
				while (1);
			}
		}
	}
}