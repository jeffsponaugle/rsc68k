/*
  RSC68K Hardware definitions module

*/

#ifndef _RSC68K_H_
#define _RSC68K_H_

// Base memory address
#define	RSC68KHW_BASE_RAM					0x000000
#define	RSC68KHW_BASE_RAM_SIZE				0xc00000

// Vectors
#define	VECTOR_STACK_POINTER				0x000000
#define	VECTOR_RESET_PC						0x000004
#define	VECTOR_BUS_ERROR					0x000008
#define	VECTOR_ADDRESS_ERROR				0x00000c
#define	VECTOR_ILLEGAL_INSTRUCTION			0x000010
#define	VECTOR_DIV0							0x000014
#define	VECTOR_PRIVILEGE_VIOLATION			0x00001c

// Shared memory address
#define	RSC68KHW_BASE_SHARED_RAM			(RSC68KHW_BASE_RAM + RSC68KHW_BASE_RAM_SIZE)
#define	RSC68KHW_BASE_SHARED_RAM_SIZE		0x300000

// ---------------------------------------------------------------------
// Supervisor CPU specific defines
// ---------------------------------------------------------------------

// Flash
#define	RSC68KHW_SUPERVISOR_FLASH			(RSC68KHW_BASE_SHARED_RAM + RSC68KHW_BASE_SHARED_RAM_SIZE)
#define	RSC68KHW_SUPERVISOR_FLASH_SIZE		(0x80000)
#define	RSC68KHW_SUPERVISOR_RAM				RSC68KHW_SUPERVISOR_FLASH
#define	RSC68KHW_UPPER_RAM_SIZE				(0xa0000)
#define	RSC68KHW_UPPER_RAM_END				(RSC68KHW_SUPERVISOR_RAM + RSC68KHW_UPPER_RAM_SIZE)

// Supervisor video RAM area 0xfb0000-0xfcffff
#define	RSC68KHW_SUPERVISOR_VIDEO_RAM		(RSC68KHW_SUPERVISOR_FLASH + 0xb0000)
#define	RSC68KHW_SUPERVISOR_VIDEO_RAM_SIZE	(0x20000)

// Supervisor video character RAM 0xfd0000-0xfeffff
#define	RSC68KHW_SUPERVISOR_VIDEO_CHAR		(RSC68KHW_SUPERVISOR_VIDEO_RAM + RSC68KHW_SUPERVISOR_VIDEO_RAM_SIZE)
#define	RSC68KHW_SUPERVISOR_VIDEO_CHAR_SIZE	(0x20000)

// ---------------------------------------------------------------------
// Worker CPU specific defines
// ---------------------------------------------------------------------

// High RAM
#define	RSC68KHW_WORKER_HIGH_RAM			(RSC68KHW_BASE_SHARED_RAM + RSC68KHW_BASE_SHARED_RAM_SIZE)
#define	RSC68KHW_WORKER_HIGH_RAM_SIZE		(RSC68KHW_DEVCOM_BASE - RSC68KHW_SUPERVISOR_FLASH)

// ---------------------------------------------------------------------
// Devices common to all CPUs
// ---------------------------------------------------------------------

#define	RSC68KHW_DEVCOM_BASE				(0xff0000)
#define	RSC68KHW_DEVCOM_POST_LED			RSC68KHW_DEVCOM_BASE
#define	RSC68KHW_DEVCOM_UARTA				(RSC68KHW_DEVCOM_POST_LED + 0x100)
#define	RSC68KHW_DEVCOM_UARTB				(RSC68KHW_DEVCOM_UARTA + 0x100)
#define	RSC68KHW_DEVCOM_IDE_CSA				(RSC68KHW_DEVCOM_UARTB + 0x100)
#define	RSC68KHW_DEVCOM_IDE_CSB				(RSC68KHW_DEVCOM_IDE_CSA + 0x100)
#define	RSC68KHW_DEVCOM_STATUS_LED 			(RSC68KHW_DEVCOM_IDE_CSB + 0x100)
#define	RSC68KHW_DEVCOM_INTC_MASK			(RSC68KHW_DEVCOM_STATUS_LED + 0x200)
#define	RSC68KHW_DEVCOM_INTC_MASK2			(RSC68KHW_DEVCOM_INTC_MASK + 2)
#define	RSC68KHW_DEVCOM_SELF_RESET			(RSC68KHW_DEVCOM_INTC_MASK2 + 4)

// ----------------------
// IDE Registers
// ----------------------

#define	RSC68KW_DEVCOM_IDE_CSA_HDDEVSEL		(RSC68KHW_DEVCOM_IDE_CSA + (6 << 5))
#define	RSC68KW_DEVCOM_IDE_CSA_STATUS_CMD	(RSC68KHW_DEVCOM_IDE_CSA + (7 << 5))

// ---------------------------------------------------------------------
// Devices specific to each CPU (in this context, just the supervisor)
// ---------------------------------------------------------------------

#define	RSC68KHW_DEVSPEC_BASE				(RSC68KHW_DEVCOM_BASE + 0x8000)
#define	RSC68KHW_DEVSPEC_RESET_DRIVE		(RSC68KHW_DEVSPEC_BASE)
#define	RSC68KHW_DEVSPEC_CLOCK_CTRL			(RSC68KHW_DEVSPEC_RESET_DRIVE + 0x100)
#define	RSC68KHW_DEVSPEC_REQUEST_GRANT		(RSC68KHW_DEVSPEC_CLOCK_CTRL + 0x100)
#define	RSC68KHW_DEVSPEC_V82C42				(RSC68KHW_DEVSPEC_REQUEST_GRANT + 0x200)
#define	RSC68KHW_DEVSPEC_RTC				(RSC68KHW_DEVSPEC_V82C42 + 0x100)
#define	RSC68KHW_DEVSPEC_FLASH_DRAM_MAP		(RSC68KHW_DEVSPEC_RTC + 0x200)
#define	RSC68KHW_DEVSPEC_NIC				(RSC68KHW_DEVSPEC_FLASH_DRAM_MAP + 0x100)
#define RSC68KWH_DEVSPEC_BARRIER_FLAGS_RD	(RSC68KHW_DEVSPEC_NIC + 0x100)
#define	RSC68KHW_DEVSPEC_PTC				(RSC68KWH_DEVSPEC_BARRIER_FLAGS_RD + 0x100)
#define	RSC68KWH_DEVSPEC_BARRIER_FLAGS_WR	(RSC68KHW_DEVSPEC_PTC + 0x100)
#define RSC68KWH_DEVSPEC_UARTA				(RSC68KWH_DEVSPEC_BARRIER_FLAGS_WR + 0x100)
#define RSC68KWH_DEVSPEC_UARTB				(RSC68KWH_DEVSPEC_UARTA + 0x100)

// Locations for boot loader and BIOS
#define	RSC68KHW_BOOTLOADER_BASE			RSC68KHW_BASE_RAM
#define	RSC68KHW_BOOTLOADER_SIZE			0x20000
#define	RSC68KHW_BIOS_BASE					RSC68KHW_SUPERVISOR_FLASH

// POST LED related

// 7 Segment segments + decimal point. Layout is as follows:
//
//      A
//   +-----+
//   |     |
// F |     | B
//   |  G  |
//   +-----+
//   |     |
// E |     | C
//   |     |
//   +-----+
//      D    DP

// Each segment is active LOW
#define	POST_7SEG_DP						0xfe
#define	POST_7SEG_A							0xfd
#define	POST_7SEG_B							0xfb
#define	POST_7SEG_C							0xf7
#define	POST_7SEG_D							0xef
#define	POST_7SEG_E							0xdf
#define	POST_7SEG_F							0xbf
#define	POST_7SEG_G							0x7f

// 7 Segment hex digit values
#define	POSTLED(x)							(x) 
#define	POST_7SEG_HEX_0						POSTLED(POST_7SEG_A & POST_7SEG_B & POST_7SEG_C & POST_7SEG_D & POST_7SEG_E & POST_7SEG_F)
#define	POST_7SEG_HEX_1						POSTLED(POST_7SEG_B & POST_7SEG_C)
#define	POST_7SEG_HEX_2						POSTLED(POST_7SEG_A & POST_7SEG_B & POST_7SEG_D & POST_7SEG_E & POST_7SEG_G)
#define	POST_7SEG_HEX_3						POSTLED(POST_7SEG_A & POST_7SEG_B & POST_7SEG_C & POST_7SEG_D & POST_7SEG_G)
#define	POST_7SEG_HEX_4						POSTLED(POST_7SEG_B & POST_7SEG_C & POST_7SEG_F & POST_7SEG_G)
#define	POST_7SEG_HEX_5						POSTLED(POST_7SEG_A & POST_7SEG_F & POST_7SEG_G & POST_7SEG_C & POST_7SEG_D)
#define POST_7SEG_HEX_6						POSTLED(POST_7SEG_A & POST_7SEG_F & POST_7SEG_G & POST_7SEG_C & POST_7SEG_D & POST_7SEG_E)
#define	POST_7SEG_HEX_7						POSTLED(POST_7SEG_A & POST_7SEG_B & POST_7SEG_C)
#define	POST_7SEG_HEX_8						POSTLED(POST_7SEG_A & POST_7SEG_B & POST_7SEG_C & POST_7SEG_D & POST_7SEG_E & POST_7SEG_F & POST_7SEG_G)
#define	POST_7SEG_HEX_9						POSTLED(POST_7SEG_G & POST_7SEG_F & POST_7SEG_A & POST_7SEG_B & POST_7SEG_C & POST_7SEG_D)
#define	POST_7SEG_HEX_A						POSTLED(POST_7SEG_A & POST_7SEG_B & POST_7SEG_C & POST_7SEG_E & POST_7SEG_F & POST_7SEG_G)
#define	POST_7SEG_HEX_B						POSTLED(POST_7SEG_C & POST_7SEG_D & POST_7SEG_E & POST_7SEG_F & POST_7SEG_G)
#define	POST_7SEG_HEX_C						POSTLED(POST_7SEG_A & POST_7SEG_D & POST_7SEG_E & POST_7SEG_F)
#define	POST_7SEG_HEX_D						POSTLED(POST_7SEG_C & POST_7SEG_D & POST_7SEG_E & POST_7SEG_B & POST_7SEG_G)
#define	POST_7SEG_HEX_E						POSTLED(POST_7SEG_A & POST_7SEG_D & POST_7SEG_E & POST_7SEG_F & POST_7SEG_G)
#define	POST_7SEG_HEX_F						POSTLED(POST_7SEG_A & POST_7SEG_E & POST_7SEG_F & POST_7SEG_G)
#define	POST_7SEG_ALPHA_U					POSTLED(POST_7SEG_B & POST_7SEG_C & POST_7SEG_D & POST_7SEG_E & POST_7SEG_F)

#define POST_7SEG_OFF						POSTLED(0xff)

// POST Codes
#define	POST_SET(x)							*((volatile uint16_t *) RSC68KHW_DEVCOM_POST_LED) = (x)

// Boot loader POST codes
#define	POSTCODE_UART_A_INIT					((POST_7SEG_HEX_0 << 8) + POST_7SEG_HEX_0)	// 00 - UART A Init
#define	POSTCODE_BOOTLOADER_COPY				((POST_7SEG_HEX_0 << 8) + POST_7SEG_HEX_1)	// 01 - Boot loader copy to DRAM
#define POSTCODE_BOOTLOADER_HEAP_INIT			((POST_7SEG_HEX_0 << 8) + POST_7SEG_HEX_2)	// 02 - Boot loader heap init
#define	POSTCODE_BOOTLOADER_RAMEXEC				((POST_7SEG_HEX_0 << 8) + POST_7SEG_HEX_3)	// 03 - Boot loader jump to DRAM
#define	POSTCODE_BOOTLOADER_MAIN				((POST_7SEG_HEX_0 << 8) + POST_7SEG_HEX_4)	// 04 - Boot loader has hit main()
#define	POSTCODE_BOOTLOADER_OP_IMAGE_SEARCH		((POST_7SEG_HEX_0 << 8) + POST_7SEG_HEX_5)	// 05 - Boot loader started operational image (BIOS) search
#define	POSTCODE_BOOTLOADER_OP_IMAGE_SEARCH_OP1 ((POST_7SEG_HEX_0 << 8) + POST_7SEG_HEX_6)	// 06 - Checking op image 1 region
#define	POSTCODE_BOOTLOADER_OP_IMAGE_SEARCH_OP2 ((POST_7SEG_HEX_0 << 8) + POST_7SEG_HEX_7)	// 07 - Checking op image 2 region
#define	POSTCODE_BOOTLOADER_CHOOSE_OP_IMAGE		((POST_7SEG_HEX_0 << 8) + POST_7SEG_HEX_8)	// 08 - Choosing which op image to boot to
#define POSTCODE_BOOTLOADER_COPY_OP_IMAGE		((POST_7SEG_HEX_0 << 8) + POST_7SEG_HEX_9)	// 09 - Copy operational image to RAM
#define POSTCODE_BOOTLOADER_VERIFY_OP_IMAGE_COPY ((POST_7SEG_HEX_0 << 8) + POST_7SEG_HEX_A)	// 0a - Verify Flash->RAM copy
#define POSTCODE_BOOTLOADER_OP_IMAGE_EXEC		((POST_7SEG_HEX_0 << 8) + POST_7SEG_HEX_B)	// 0b - Jump to operational BIOS image

// BIOS POST Codes
#define	POSTCODE_BIOS_START						((POST_7SEG_HEX_1 << 8) + POST_7SEG_HEX_0)	// 10 - Start of BIOS exection
#define	POSTCODE_BIOS_UART_A_INIT				((POST_7SEG_HEX_1 << 8) + POST_7SEG_HEX_1)	// 11 - Initialization of UART A
#define	POSTCODE_BIOS_RAM_CHECK	   			 	((POST_7SEG_HEX_1 << 8) + POST_7SEG_HEX_2)	// 12 - Start of BIOS area RAM test
#define	POSTCODE_BIOS_MAIN_EXEC					((POST_7SEG_HEX_1 << 8) + POST_7SEG_HEX_3)	// 13 - Jumping into CRT startup code
#define	POSTCODE_BIOS_MAIN 		 				((POST_7SEG_HEX_1 << 8) + POST_7SEG_HEX_4)	// 14 - At main() function in BIOS
#define	POSTCODE_BIOS_FLASH_COPY				((POST_7SEG_HEX_1 << 8) + POST_7SEG_HEX_5)	// 15 - Start of flash table search and copy
#define	POSTCODE_BIOS_CONSOLE_INIT				((POST_7SEG_HEX_1 << 8) + POST_7SEG_HEX_6)	// 16 - Start of console initialize

// Flash update POST codes
#define	POSTCODE_FWUPD_START					((POST_7SEG_HEX_4 << 8) + POST_7SEG_HEX_0)	// 40 - Start of firmware update
#define	POSTCODE_FWUPD_START_MAIN				((POST_7SEG_HEX_4 << 8) + POST_7SEG_HEX_1)	// 41 - Hit main()
#define	POSTCODE_FWUPD_REGION_CHECKS			((POST_7SEG_HEX_4 << 8) + POST_7SEG_HEX_2)	// 42 - Checking both flash op regions
#define	POSTCODE_FWUPD_ERASE_REGION				((POST_7SEG_HEX_4 << 8) + POST_7SEG_HEX_3)	// 43 - Erasing the region to be programmed
#define	POSTCODE_FWUPD_START_TRANSFER			((POST_7SEG_HEX_4 << 8) + POST_7SEG_HEX_4)	// 44 - Start transfer
#define	POSTCODE_FWUPD_BLOCK_RECEIVE			((POST_7SEG_HEX_4 << 8) + POST_7SEG_HEX_5)	// 45 - Block receive
#define	POSTCODE_FWUPD_PROGRAM_BLOCK			((POST_7SEG_HEX_4 << 8) + POST_7SEG_HEX_6)	// 46 - Program block
#define POSTCODE_FWUPD_PROGRAM_CHECK			((POST_7SEG_HEX_4 << 8) + POST_7SEG_HEX_7)	// 47 - Programming complete. Checking region.
#define	POSTCODE_FWUPD_ERASE_STAGING			((POST_7SEG_HEX_4 << 8) + POST_7SEG_HEX_8)	// 48 - Erase staging area

// ELF File transfer POST codes
#define	POSTCODE_ELF_START						((POST_7SEG_HEX_5 << 8) + POST_7SEG_HEX_0)	// 50 - Start of ELF transfer
#define	POSTCODE_ELF_HEADER                     ((POST_7SEG_HEX_5 << 8) + POST_7SEG_HEX_1)	// 51 - ELF Header consumed
#define	POSTCODE_ELF_PROGRAM_HEADER             ((POST_7SEG_HEX_5 << 8) + POST_7SEG_HEX_2)	// 52 - Finding program header
#define	POSTCODE_ELF_PROGRAM_CONSUME1			((POST_7SEG_HEX_5 << 8) + POST_7SEG_HEX_3)	// 53 - Consuming ELF program
#define	POSTCODE_ELF_PROGRAM_CONSUME2			((POST_7SEG_HEX_5 << 8) + POST_7SEG_HEX_4)	// 54 - Consuming ELF program
#define	POSTCODE_ELF_COMPLETE                   ((POST_7SEG_HEX_5 << 8) + POST_7SEG_HEX_5)	// 55 - ELF Program load complete
#define	POSTCODE_ELF_BAD_IMAGE                  ((POST_7SEG_HEX_5 << 8) + POST_7SEG_HEX_6)	// 56 - Bad ELF image for one or more reasons
#define	POSTCODE_ELF_DISPATCH_PROGRAM           ((POST_7SEG_HEX_5 << 8) + POST_7SEG_HEX_7)	// 57 - Dispatching control to ELF program

// Application POST codes
#define	POSTCODE_APP_START						((POST_7SEG_HEX_6 << 8) + POST_7SEG_HEX_0)	// 60 - Start of application execution
#define	POSTCODE_APP_START_MAIN					((POST_7SEG_HEX_6 << 8) + POST_7SEG_HEX_1)	// 61 - Application execution at main()

// UART fault
#define	POSTFAULT_UART_ISR						((POST_7SEG_ALPHA_U << 8) + POST_7SEG_OFF)	// U  - UART Problem

// Interrupt vectors
#define	INTVECT_IRQL7_DEBUGGER					0xf0
#define	INTVECT_IRQ6A_PTC1						0xe1
#define	INTVECT_IRQ6B_PTC2						0xe2
#define	INTVECT_IRQ5A_UARTA						0xd1
#define	INTVECT_IRQ5B_UARTB						0xd2
#define	INTVECT_IRQ4A_RTC						0xc1
#define	INTVECT_IRQ4B_IDE						0xc2
#define	INTVECT_IRQ3A_KEYBOARD					0xb1
#define	INTVECT_IRQ3B_NIC						0xb2

// Baud rate in the boot loader
#define	UART_BAUD_CLOCK							18432000
#define	BOOTLOADER_BAUD_RATE					115200
#define	FLASH_UPDATE_BAUD_RATE					230400
#define	BOOTLOADER_BAUD_RATE_DIVISOR			((UART_BAUD_CLOCK / 16) / BOOTLOADER_BAUD_RATE)

// Programmable timer counter divsior (from the main system clock)
#define	PTC_COUNTER0_DIV						64
#define	PTC_COUNTER1_DIV						32
#define	PTC_COUNTER2_DIV						64

// Default PTC rates (in hz)
#define	PTC_COUNTER0_HZ		  					10			// 100ms
#define	PTC_COUNTER1_HZ							100			// 10ms

// Default master clock speed
#define	RSC68K_MASTER_CLOCK_DEFAULT				32000000

// Used for the flash/dram mapping register
#define	FLASH_DRAM_READ_MASK					0x03
#define	FLASH_DRAM_WRITE_MASK					0x0c

// Enums for slot IDs
#define	SLOT_ID_UNINITIALIZED					0xfb
#define	SLOT_ID_SUPERVISOR						0x9a
#define	SLOT_ID_BASE							0x17
#define	SLOT_ID_MAX								(SLOT_ID_BASE + 0x20)

// Helpful/useful macros
#define	ZERO_STRUCT(x)							memset((void *) &(x), 0, sizeof(x));
#define	ERR_GOTO()								if (eStatus != ESTATUS_OK) { goto errorExit; }

#endif
