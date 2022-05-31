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

// ISA I/O address region
#define	RSC68KHW_SUPERVISOR_ISA_IO			(RSC68KHW_SUPERVISOR_FLASH + RSC68KHW_SUPERVISOR_FLASH_SIZE)
#define	RSC68KHW_SUPERVISOR_ISA_IO_SIZE		(0x10000)

// ISA Memory region from 9000:0000-EFFF:0000
#define	RSC68KHW_SUPERVISOR_ISA_MEM			(RSC68KHW_SUPERVISOR_ISA_IO + RSC68KHW_SUPERVISOR_ISA_IO_SIZE)
#define	RSC68KHW_SUPERVISOR_ISA_MEM_SIZE	(0x60000)

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
#define	RSC68KWH_DEVSPEC_BARRIER_FLAGS_WR	(RSC68KWH_DEVSPEC_BARRIER_FLAGS_RD + 0x100)
#define RSC68KWH_DEVSPEC_UARTA				(RSC68KWH_DEVSPEC_BARRIER_FLAGS_WR + 0x100)
#define RSC68KWH_DEVSPEC_UARTB				(RSC68KWH_DEVSPEC_UARTA + 0x100)

// Locations for boot loader and BIOS
#define	RSC68KHW_BOOTLOADER_BASE			RSC68KHW_BASE_RAM
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

// Baud rate in the boot loader
#define	UART_BAUD_CLOCK						18432000
#define	BOOTLOADER_BAUD_RATE				115200
#define	BOOTLOADER_BAUD_RATE_DIVISOR		((UART_BAUD_CLOCK / 16) / BOOTLOADER_BAUD_RATE)

// Used for the flash/dram mapping register
#define	FLASH_DRAM_READ_MASK				0x03
#define	FLASH_DRAM_WRITE_MASK				0x0c

// Flash and RAM map #defines
#define	FLASH_PAGE_WRITE_SET(x)				*((volatile uint8_t *) RSC68KHW_DEVSPEC_FLASH_DRAM_MAP) = ((*((volatile uint8_t *) RSC68KHW_DEVSPEC_FLASH_DRAM_MAP) & ~0xc) | ((x & 1) << 2))
#define	FLASH_PAGE_READ_SET(x)	   		 	*((volatile uint8_t *) RSC68KHW_DEVSPEC_FLASH_DRAM_MAP) = ((*((volatile uint8_t *) RSC68KHW_DEVSPEC_FLASH_DRAM_MAP) & ~0x3) | (x & 3))
#define	RAM_PAGE_WRITE_SET(x)				*((volatile uint8_t *) RSC68KHW_DEVSPEC_FLASH_DRAM_MAP) &= ~0x0c;
#define RAM_PAGE_READ_SET(x)                *((volatile uint8_t *) RSC68KHW_DEVSPEC_FLASH_DRAM_MAP) = ((*((volatile uint8_t *) RSC68KHW_DEVSPEC_FLASH_DRAM_MAP) & ~0x1) | 0x02);

// Enums for slot IDs
#define	SLOT_ID_UNINITIALIZED				0xfb
#define	SLOT_ID_SUPERVISOR					0x9a
#define	SLOT_ID_BASE						0x17
#define	SLOT_ID_MAX							(SLOT_ID_BASE + 0x20)

// Helpful/useful macros
#define	ZERO_STRUCT(x)						memset((void *) &x, 0, sizeof(x));
#define	ERR_GOTO()							if (eStatus != ESTATUS_OK) { goto errorExit; }

#endif
