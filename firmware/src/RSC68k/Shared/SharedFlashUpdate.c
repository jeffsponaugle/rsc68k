#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "BIOS/OS.h"
#include "Hardware/RSC68k.h"
#include "Shared/Version.h"
#include "Shared/16550.h"
#include "Shared/Flash.h"
#include "Shared/Monitor.h"
#include "Shared/Shared.h"
#include "Shared/Monitor.h"

#define	YMODEM_RECEIVE_TIMEOUT	0x5ffff

// Communication standard characters
#define	COMM_SOH			0x01
#define	COMM_STX			0x02
#define	COMM_EOT			0x04
#define	COMM_ACK			0x06
#define	COMM_NAK			0x15
#define	COMM_CAN			0x18

// Receives a single block of data from a remote unit
static EStatus YModemReceiveBlock(uint8_t *pu8Buffer,
								  uint16_t *pu16DataReceived,
								  uint8_t *pu8BlockNumber,
								  uint16_t *pu16CRCReceived)
{
	EStatus eStatus = ESTATUS_OK;
	uint16_t u16ReceiveCount;
	uint8_t u8BlockNumberInverted;
	uint8_t u8Data = 0;

	*pu16CRCReceived = 0;

	for (;;)
	{
		// Wait for STX (1K), SOH (128), or EOT
		eStatus = SerialReceiveWaitTimeoutPIO((S16550UART *) RSC68KHW_DEVCOM_UARTB,
											  &u8Data,
											  YMODEM_RECEIVE_TIMEOUT);
		ERR_GOTO();

		if (COMM_SOH == u8Data)
		{
			// 128 Byte block
			*pu16DataReceived = 128;
			break;
		}
		else
		if (COMM_STX == u8Data)
		{
			// 1K Block
			*pu16DataReceived = 1024;
			break;
		}
		else
		if (COMM_EOT == u8Data)
		{
			// No data - end of transmission
			*pu16DataReceived = 0;
			eStatus = ESTATUS_OK;
			goto errorExit;
		}
	}

	// Get the block #
	eStatus = SerialReceiveWaitTimeoutPIO((S16550UART *) RSC68KHW_DEVCOM_UARTB,
										  pu8BlockNumber,
										  YMODEM_RECEIVE_TIMEOUT);
	ERR_GOTO();

	// Get the inverted block #
	eStatus = SerialReceiveWaitTimeoutPIO((S16550UART *) RSC68KHW_DEVCOM_UARTB,
										  &u8BlockNumberInverted,
										  YMODEM_RECEIVE_TIMEOUT);
	ERR_GOTO();

	u16ReceiveCount = *pu16DataReceived;

	// Consume all data in this block
	while (u16ReceiveCount)
	{
		eStatus = SerialReceiveWaitTimeoutPIO((S16550UART *) RSC68KHW_DEVCOM_UARTB,
											  pu8Buffer,
											  YMODEM_RECEIVE_TIMEOUT);
		ERR_GOTO();

		++pu8Buffer;
		--u16ReceiveCount;
	}

	// Now the MSB of the CRC
	eStatus = SerialReceiveWaitTimeoutPIO((S16550UART *) RSC68KHW_DEVCOM_UARTB,
										  (uint8_t *) pu16CRCReceived,
										  YMODEM_RECEIVE_TIMEOUT);
	ERR_GOTO();

	// If we're doing block 0, it's 8 bit checksum, so don't get the second CRC byte
	// 

	if ((u8Data == COMM_SOH) && (0 == *pu8BlockNumber))
	{
//		printf("Got checksum\r\n");
	}
	else
	{
		// And the LSB
		eStatus = SerialReceiveWaitTimeoutPIO((S16550UART *) RSC68KHW_DEVCOM_UARTB,
											   ((uint8_t *) pu16CRCReceived) + 1,
											   YMODEM_RECEIVE_TIMEOUT);
		ERR_GOTO();

//		printf("Got CRC16\r\n");
	}

	// Block number and inverted block number inverted should be the same
	if (*pu8BlockNumber != ((uint8_t) ~u8BlockNumberInverted))
	{
		// Failed check
		eStatus = ESTATUS_PROTOCOL_BLOCK_FAULT;
		goto errorExit;
	}

	// All good

errorExit:
	return(eStatus);
}

// How many bytes do we consume?
#define	PROGRAM_BLOCK_SIZE		8192
#define	RX_BLOCK_BUFFER_SIZE	1024

static uint16_t CRC16(uint8_t *pu8Data, 
					  uint16_t u16Count)
{
	uint16_t u16CRC = 0;
	uint8_t u8Mask;
    char i;

    while (u16Count)
    {
        u16CRC = u16CRC ^ (((uint16_t) *pu8Data) << 8);
        u8Mask = 0x80;

		while (u8Mask)
        {
            if (u16CRC & 0x8000)
			{
                u16CRC = (u16CRC << 1) ^ 0x1021;
			}
            else
			{
                u16CRC <<= 1;
			}

			u8Mask >>= 1;
        }

		pu8Data++;
		u16Count--;
    }

    return(u16CRC);
}

static EStatus FlashTransferAndUpdate(EFlashRegion eUpdateRegion)
{
	EStatus eStatus;
	bool bResult;
	uint8_t *pu8ProgramBuffer = NULL;
	uint8_t *pu8ProgramBufferPtr = NULL;
	uint32_t u32ProgramOffset = 0;
	uint16_t u16ProgramBufferCount = 0;
	uint8_t *pu8BlockBuffer = NULL;
	uint8_t *pu8BlockBufferPtr = NULL;
	uint8_t u8LastResponse;
	uint8_t u8BlockExpected = 1;

	// Allocate a programming buffer
	pu8ProgramBuffer = malloc(PROGRAM_BLOCK_SIZE);
	if (NULL == pu8ProgramBuffer)
	{
		printf("malloc() of %u bytes failed - transfer aborted\r\n", PROGRAM_BLOCK_SIZE);
		eStatus = ESTATUS_OUT_OF_MEMORY;
		goto errorExit;
	}

	// Allocate a block buffer
	pu8BlockBuffer = malloc(RX_BLOCK_BUFFER_SIZE);
	if (NULL == pu8BlockBuffer)
	{
		printf("malloc() of %u bytes failed - transfer aborted\r\n", RX_BLOCK_BUFFER_SIZE);
		eStatus = ESTATUS_OUT_OF_MEMORY;
		goto errorExit;
	}

	// Pad everything
	memset((void *) pu8ProgramBuffer, 0xff, PROGRAM_BLOCK_SIZE);

	// Assume a 'C' to start with
	u8LastResponse = 'C';

	// And our program buffer pointer
	pu8ProgramBufferPtr = pu8ProgramBuffer;

	// Clear the UART's incoming characters
	while (SerialDataAvailablePIO((S16550UART *) RSC68KHW_DEVCOM_UARTB))
	{
		(void) SerialReceiveWaitPIO((S16550UART *) RSC68KHW_DEVCOM_UARTB);
	}

	for (;;)
	{
		uint16_t u16DataReceived;
		uint8_t u8BlockReceived;
		uint16_t u16CRCReceived;
		uint16_t u16CRCCalculated;
		bool bBlockZeroSeen = false;

		// Send last response
		SerialSendPIO((S16550UART *) RSC68KHW_DEVCOM_UARTB,
					  &u8LastResponse,
					  sizeof(u8LastResponse));

		// Now go wait for a block of something to come in
		eStatus = YModemReceiveBlock(pu8BlockBuffer,
									 &u16DataReceived,
									 &u8BlockReceived,
									 &u16CRCReceived);
		if (ESTATUS_OK == eStatus)
		{
			POST_SET(POSTCODE_FWUPD_BLOCK_RECEIVE);

			// Got a block of something. Let's see what it is.
			if (0 == u16DataReceived)
			{
				// End of everything
				printf("\r\nAll done\r\n");
				break;
			}
			
			// We've received a block of data. Let's CRC it.
			u16CRCCalculated = CRC16(pu8BlockBuffer,
									 u16DataReceived);
			if ((0 == u8BlockReceived) &&
				(false == bBlockZeroSeen))
			{
				// If it's block 0, then we ignore it (it's just the filename)
				u8LastResponse = COMM_ACK;

				// Send an ACK, then a C
				SerialSendPIO((S16550UART *) RSC68KHW_DEVCOM_UARTB,
							  &u8LastResponse,
							  sizeof(u8LastResponse));

				u8LastResponse = 'C';
				bBlockZeroSeen = true;
			}
			else
			if (u16CRCCalculated != u16CRCReceived)
			{
				printf("\r\nBlock %u CRC mismatch - expected 0x%.4x, got 0x%.4x\r\n", u8BlockReceived, u16CRCCalculated, u16CRCReceived);
				u8LastResponse = COMM_NAK;
			}
			else
			{
				if (u8BlockExpected != u8BlockReceived)
				{
					printf("\r\nExpected block %u, got block %u - canceling\r\n", u8BlockExpected, u8BlockReceived);
					eStatus = ESTATUS_PROTOCOL_WRONG_BLOCK;
					goto errorExit;
				}

				++u8BlockExpected;
				printf("\rBlock %u - %u bytes", u8BlockReceived, u16DataReceived);
				POST_SET(POSTCODE_FWUPD_PROGRAM_BLOCK);

				// CRC Is good. Copy the data into the target program buffer.
				pu8BlockBufferPtr = pu8BlockBuffer;
				while (u16DataReceived)
				{
					*pu8ProgramBufferPtr = *pu8BlockBufferPtr;
					++pu8ProgramBufferPtr;
					++pu8BlockBufferPtr;
					--u16DataReceived;
					++u16ProgramBufferCount;

					if (u16ProgramBufferCount >= PROGRAM_BLOCK_SIZE)
					{
						bool bResult;

						// Time to program the block
						printf("\r\nProgramming offset 0x%.6x\r\n", u32ProgramOffset);

						bResult = FlashProgramRegion(eUpdateRegion,
													 u32ProgramOffset,
													 pu8ProgramBuffer,
													 PROGRAM_BLOCK_SIZE);
						
						if (false == bResult)
						{
							printf("\r\nFailed to program flash region %u offset 0x%.6x\r\n", eUpdateRegion, u32ProgramOffset);
							eStatus = ESTATUS_FLASH_PROGRAMMING_FAULT;
							goto errorExit;
						}

						u16ProgramBufferCount = 0;
						pu8ProgramBufferPtr = pu8ProgramBuffer;
						
						// Clear the block buffer
						memset((void *) pu8ProgramBuffer, 0xff, PROGRAM_BLOCK_SIZE);

						// Next offset
						u32ProgramOffset += PROGRAM_BLOCK_SIZE;
					}
				}

				// Ack the packet
				u8LastResponse = COMM_ACK;
			}
		}
		else
		{
			// Something happened - NAK it
			u8LastResponse = COMM_NAK;
		}
	}

	// If there's more data to program, go do so
	if (u16ProgramBufferCount)
	{
		printf("Programming offset 0x%.6x\r\n", u32ProgramOffset);

		bResult = FlashProgramRegion(eUpdateRegion,
									 u32ProgramOffset,
									 pu8ProgramBuffer,
									 PROGRAM_BLOCK_SIZE);

		if (false == bResult)
		{
			printf("Failed to program flash region %u offset 0x%.6x\r\n", eUpdateRegion, u32ProgramOffset);
			eStatus = ESTATUS_FLASH_PROGRAMMING_FAULT;
			goto errorExit;
		}
	}

	// Send a final ack
	u8LastResponse = COMM_ACK;
	SerialSendPIO((S16550UART *) RSC68KHW_DEVCOM_UARTB,
				  &u8LastResponse,
				  sizeof(u8LastResponse));

	// And C to indicate end of transmit
	u8LastResponse = 'C';
	SerialSendPIO((S16550UART *) RSC68KHW_DEVCOM_UARTB,
				  &u8LastResponse,
				  sizeof(u8LastResponse));

	// All done
	eStatus = ESTATUS_OK;

	printf("File transfer/programming complete\r\n");

errorExit:
	if (pu8ProgramBuffer)
	{
		free(pu8ProgramBuffer);
	}

	if (pu8BlockBuffer)
	{
		free(pu8BlockBuffer);
	}

	return(eStatus);
}

void SharedFlashUpdate(void)
{
	EVersionCode eVersionOp1;
	SImageVersion sVersionStructOp1;
	EVersionCode eVersionOp2;
	SImageVersion sVersionStructOp2;
	const SFlashSegment *psFlashMap = NULL;
	EFlashRegion eUpdateRegion = EFLASHREGION_END;
	bool bResult;
	EStatus eStatus;

	POST_SET(POSTCODE_FWUPD_START_MAIN);

	// Init UART B to the usual 115.2kbps 8N1 settings
	bResult = SerialInit((S16550UART *) RSC68KHW_DEVCOM_UARTB,
						 8,
						 1,
						 EUART_PARITY_NONE);
	assert(bResult);

	// Turn on RTS and DTR and the OUTs
	SerialSetOutputs((S16550UART *) RSC68KHW_DEVCOM_UARTB,
					 true,
					 true,
					 true,
					 true);

	// Set UART B's baud rate
	SerialSetBaudRate((S16550UART *) RSC68KHW_DEVCOM_UARTB,
					  UART_BAUD_CLOCK,
					  FLASH_UPDATE_BAUD_RATE,
					  NULL);

	// Force lower part of flash
	FlashPageReadSet(0);

	// Go init the flash table
	FlashTableCopy(NULL,
				   true);

	// Identify the flash part
	FlashIdentify();

	while (1)
	{
		uint8_t u8Char;

		POST_SET(POSTCODE_FWUPD_REGION_CHECKS);

		printf("Examining BIOS update regions\r\n");

		eVersionOp1 = FlashRegionCheck(EFLASHREGION_OP_1,
									   EIMGTYPE_OPERATIONAL,
									   &sVersionStructOp1,
									   NULL);
		eVersionOp2 = FlashRegionCheck(EFLASHREGION_OP_2,
									   EIMGTYPE_OPERATIONAL,
									   &sVersionStructOp2,
									   NULL);

		// If both op images are OK, figure out which one is older and erase it
		if ((EVERSION_OK == eVersionOp1) &&
			(EVERSION_OK == eVersionOp2))
		{
			printf("Two images valid - ");
			if (sVersionStructOp2.u64BuildTimestamp < sVersionStructOp1.u64BuildTimestamp)
			{
				// Choose OP2 since it's older
				eUpdateRegion = EFLASHREGION_OP_2;
			}
			else
			{
				// Choose OP1 since it's older or equal
				eUpdateRegion = EFLASHREGION_OP_1;
			}
		}
		else
		{
			if (EVERSION_OK == eVersionOp1)
			{
				// Op2
				eUpdateRegion = EFLASHREGION_OP_2;
			}
			else
			{
				// Op1
				eUpdateRegion = EFLASHREGION_OP_1;
			}
		}

		if (FlashRegionCheckErased(eUpdateRegion))
		{
			// Region is already erased. No need to erase it.
			printf("Updating operational region %u\r\n", (eUpdateRegion - EFLASHREGION_OP_1) + 1);
		}
		else
		{	
			// Let 'em know which region we'll be updating
			printf("Updating operational region %u - erasing\r\n", (eUpdateRegion - EFLASHREGION_OP_1) + 1);

			POST_SET(POSTCODE_FWUPD_ERASE_REGION);

			// Erase the region we're going to be programming
			bResult = FlashRegionErase(eUpdateRegion);
			if (false == bResult)
			{
				printf("Failed to erase region\r\n");
				exit(1);
			}
		}

		POST_SET(POSTCODE_FWUPD_START_TRANSFER);

		printf("Waiting for start of transfer on UART B, baud rate %u\r\n", FLASH_UPDATE_BAUD_RATE);

		// Go do a YModem transfer
		eStatus = FlashTransferAndUpdate(eUpdateRegion);
		if (ESTATUS_OK == eStatus)
		{
			uint32_t u32VersionStructureOffset;

			POST_SET(POSTCODE_FWUPD_PROGRAM_CHECK);

			// Sending cancel bytes are required to get sz to not continue batch operation
			u8Char = COMM_CAN;
			SerialSendPIO((S16550UART *) RSC68KHW_DEVCOM_UARTB,
						  &u8Char,
						  sizeof(u8Char));
			SerialSendPIO((S16550UART *) RSC68KHW_DEVCOM_UARTB,
						  &u8Char,
						  sizeof(u8Char));

			// Now we check our new region for a new, good, BIOS image.
			eVersionOp1 = FlashRegionCheck(eUpdateRegion,
										   EIMGTYPE_OPERATIONAL,
										   &sVersionStructOp1,
										   &u32VersionStructureOffset);
			if (EVERSION_OK == eVersionOp1)
			{
				uint32_t u32ImageCRCCalculated = VERSION_CRC32_IMAGE_INITIAL;

				// Now check the CRC32

				// Before the version structure
				u32ImageCRCCalculated = FlashCRC32Region(eUpdateRegion,
														 u32ImageCRCCalculated,
														 0,
														 u32VersionStructureOffset);

				// After the version structure
				u32ImageCRCCalculated = FlashCRC32Region(eUpdateRegion,
														 u32ImageCRCCalculated,
														 u32VersionStructureOffset + sizeof(sVersionStructOp1),
														 sVersionStructOp1.u32ImageSize - sizeof(sVersionStructOp1) - u32VersionStructureOffset);

				if (u32ImageCRCCalculated != sVersionStructOp1.u32ImageCRC32)
				{
					printf("CRC Calc'd = 0x%.8x, CRC Expected = 0x%.8x\r\n", u32ImageCRCCalculated, sVersionStructOp1.u32ImageCRC32);
				}
				else
				{
					printf("Successfully updated active BIOS image to %u.%u.%u\r\n",
						   sVersionStructOp1.u32MajorMinorBuildNumber >> 24,
						   (sVersionStructOp1.u32MajorMinorBuildNumber >> 16) & 0xff,
						   (uint16_t)sVersionStructOp1.u32MajorMinorBuildNumber);

					printf("Erasing other bank for future staging\r\n");
					if (EFLASHREGION_OP_1 == eUpdateRegion)
					{
						eUpdateRegion = EFLASHREGION_OP_2;
					}
					else
					{
						eUpdateRegion = EFLASHREGION_OP_1;
					}

					POST_SET(POSTCODE_FWUPD_ERASE_STAGING);
					bResult = FlashRegionErase(eUpdateRegion);
					if (bResult)
					{
						printf("Erasure succeeded. Restarting with new BIOS\r\n");
						fflush(stdout);

						// Give some time for the FIFOs to clear
						SharedSleep(25000);

						// Reset the board
						*((volatile uint8_t *) RSC68KHW_DEVCOM_SELF_RESET) = 0xff;

						printf("If you see this, the reset hardware is broken.\r\n");
					}
					else
					{
						printf("Failed to erase staging region %u - 0x%.8x\r\n", eUpdateRegion, eStatus);
					}
				}
			}
			else
			{
				printf("Failed region check after programming - %s\r\n", VersionCodeGetText(eVersionOp1));
			}
		}
		else
		{
			printf("Update failed - %s\r\n", GetErrorText(eStatus));
		}

		// Send 2X CAN and restart the whole sequence
		u8Char = COMM_CAN;
		SerialSendPIO((S16550UART *) RSC68KHW_DEVCOM_UARTB,
					  &u8Char,
					  sizeof(u8Char));
		SerialSendPIO((S16550UART *) RSC68KHW_DEVCOM_UARTB,
					  &u8Char,
					  sizeof(u8Char));
	}
}


