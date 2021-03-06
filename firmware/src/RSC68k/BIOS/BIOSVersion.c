#include <stdint.h>
#include <stddef.h>
#include "Shared/Version.h"

// AUTOMATICALLY GENERATED - DO NOT CHECK IN!!!

// Linker-provided variables
extern uint8_t __DYNAMIC;
extern void _start(void);
extern uint8_t __bss_start;
extern uint8_t __end;
extern uint8_t _ram_end;
extern uint8_t g_u8SlotID;
extern void BIOSEntry(void);

// Image version structure
volatile const SImageVersion g_sImageVersion =
{
	VERSION_PREFIX,						// Version struct prefix

	EIMGTYPE_OPERATIONAL,				// BIOS (operational) image
	(((uint32_t) 1 << 24) | ((uint32_t) 0 << 16) | 7299),	// Major/minor/build #

	0,								// Build timestamp (64 bit time_t)
	STAMP_IMAGE_CRC32_FILL,	 		// Image CRC32 (filled in by stamp)

	(uint32_t) &__DYNAMIC,	 		// Load address of image
	(uint32_t) BIOSEntry,  			// Entry point
	(uint32_t) &__bss_start,		// bss start
	(uint32_t) &__end,				// End of bss/start of heap

	(void *) &g_u8SlotID,			// Address of slot ID

	STAMP_VERSION_CRC32_FILL,		// Version structure CRC32 (filled in by stamp)

	VERSION_SUFFIX					// Version structure suffix
};

