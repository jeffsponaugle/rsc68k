#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "Shared/Flash.h"
#include "Hardware/RSC68k.h"

// Pointer to active flash map
static const SFlashSegment *sg_psFlashLayout;

void FlashInit(const SFlashSegment *psFlashTable)
{
	sg_psFlashLayout = psFlashTable;
}

// Reads a chunk of data out of a flash region. Returns false if the region
// isn't found.
bool FlashReadRegion(EFlashRegion eFlashRegion,
					 uint32_t u32RegionOffset,
					 uint8_t *pu8Buffer,
					 uint32_t u32ReadSize)
{
	const SFlashSegment *psFlashTable;

	psFlashTable = sg_psFlashLayout;

	while ((psFlashTable->eFlashRegion != EFLASHREGION_END) &&
		   (u32ReadSize))
	{
		if ((eFlashRegion == psFlashTable->eFlashRegion) &&
			(u32RegionOffset >= psFlashTable->u32BlockOffset) &&
			(u32RegionOffset < (psFlashTable->u32BlockOffset + psFlashTable->u32FlashPageSize)))
		{
			uint32_t u32ChunkSize = u32ReadSize;
			uint32_t u32Available;

			// Figure out how many bytes we have/want that are available in this offset
			u32Available = (psFlashTable->u32BlockOffset + psFlashTable->u32FlashPageSize) - u32RegionOffset;
			if (u32Available < u32ChunkSize)
			{
				u32ChunkSize = u32Available;
			}

			// Set flash read page
			FLASH_PAGE_READ_SET(psFlashTable->u32FlashPageRead);

			// Copy the data into the target buffer
			if (u32ChunkSize)
			{
				memcpy((void *) pu8Buffer, (void *) (psFlashTable->u32PhysicalAddress + (u32RegionOffset - psFlashTable->u32BlockOffset)), u32ChunkSize);
			}

			u32RegionOffset += u32ChunkSize;
			pu8Buffer += u32ChunkSize;
			u32ReadSize -= u32ChunkSize;
		}

		++psFlashTable;
	}

	// If we have a remaining set of read bytes, then we didn't read the region
	// properly or fully
	if (u32ReadSize)
	{
		return(false);
	}
	else
	{
		// Otherwise, we did!
		return(true);
	}
}

// Calculates the overall size of a flash region. Returns false if the
// region isn't found
bool FlashRegionGetSize(EFlashRegion eFlashRegion,
 					    uint32_t *pu32RegionSize)
{
	uint32_t u32Size = 0;
	const SFlashSegment *psFlashTable;
	bool bRegionFound = false;

	psFlashTable = sg_psFlashLayout;

	while (psFlashTable->eFlashRegion != EFLASHREGION_END)
	{
		if (eFlashRegion == psFlashTable->eFlashRegion)
		{
			u32Size += psFlashTable->u32FlashPageSize;
			bRegionFound = true;
		}

		++psFlashTable;
	}

	if ((bRegionFound) && (pu32RegionSize))
	{
		*pu32RegionSize = u32Size;
	}

	return(bRegionFound);
}

// Gets a physical pointer to a logical region of flash and does whatever
// page selection is necessary for both reads and writes.
bool FlashGetRegionPointer(EFlashRegion eFlashRegion,
 						   uint32_t u32RegionOffset,
 						   uint8_t **ppu8RegionPointer)
{
	bool bRegionFound = false;
	const SFlashSegment *psFlashTable;

	psFlashTable = sg_psFlashLayout;

	while (psFlashTable->eFlashRegion != EFLASHREGION_END)
	{
		if ((eFlashRegion == psFlashTable->eFlashRegion) &&
			(u32RegionOffset >= psFlashTable->u32BlockOffset) &&
			(u32RegionOffset < (psFlashTable->u32BlockOffset + psFlashTable->u32FlashPageSize)))
		{
			// Found it!
			if (ppu8RegionPointer)
			{
				// Set the paging register to ensure we've selected the right page
				FLASH_PAGE_WRITE_SET(psFlashTable->u32FlashPageWrite);
				FLASH_PAGE_READ_SET(psFlashTable->u32FlashPageRead);

				// Return a pointer to the appropriate block address
				*ppu8RegionPointer = (uint8_t *) (psFlashTable->u32PhysicalAddress + (u32RegionOffset - psFlashTable->u32BlockOffset));
			}

			bRegionFound = true;
			goto errorExit;
		}

		++psFlashTable;
	}

errorExit:
	return(bRegionFound);
}

// Erases an entire flash region. true If successful, false if not
bool FlashRegionErase(EFlashRegion eFlashRegion)
{
	bool bRegionFound = false;
	const SFlashSegment *psFlashTable;
	volatile uint16_t *pu16FlashPtr;

	psFlashTable = sg_psFlashLayout;

	while (psFlashTable->eFlashRegion != EFLASHREGION_END)
	{
		if (eFlashRegion == psFlashTable->eFlashRegion)
		{
			// Found it! Erase the page.
			bRegionFound = true;

			// Set up the read and write registers appropriately for this region
			FLASH_PAGE_WRITE_SET(psFlashTable->u32FlashPageWrite);
			FLASH_PAGE_READ_SET(psFlashTable->u32FlashPageRead);

			pu16FlashPtr = (uint16_t *) psFlashTable->u32PhysicalAddress;

			// Sector erase - both chips
			*(pu16FlashPtr + 0x555) = 0xaaaa;
			*(pu16FlashPtr + 0x2aa) = 0x5555;
			*(pu16FlashPtr + 0x555) = 0x8080;
			*(pu16FlashPtr + 0x555) = 0xaaaa;
			*(pu16FlashPtr + 0x2aa) = 0x5555;
			*(pu16FlashPtr + 0x000) = 0x3030;

			// Erase should have begun. Wait for it to complete. See page 15 in the
			// 29F040 manual for the flowchart.
			while (1)
			{
				uint16_t u16FlashRead;

				u16FlashRead = *pu16FlashPtr;
				if (0 == u16FlashRead & (0x4040))
				{
					// Erasure complete (toggle bits not toggling)
					break;
				}

				// DQ5=1?
				if (0x2020 == (u16FlashRead & 0x2020))
				{	
					// Done! Check results. Do two reads 
					u16FlashRead = *pu16FlashPtr;
					u16FlashRead = *pu16FlashPtr;

					if (0x4040 == (u16FlashRead & 0x4040))
					{
						// Failed erasure
						bRegionFound = false;
						goto errorExit;
					}

					break;
				}
			}
		}

		++psFlashTable;
	}

errorExit:
	return(bRegionFound);
}

// Copies flash region to pu8Destination with region offset and read size
bool FlashCopyToRAM(EFlashRegion eFlashRegion,
					uint8_t *pu8Destination,
					uint32_t u32RegionOffset,
					uint32_t u32ReadSize)
{
	const SFlashSegment *psFlashTable;

	psFlashTable = sg_psFlashLayout;

	while ((psFlashTable->eFlashRegion != EFLASHREGION_END) &&
		   (u32ReadSize))
	{
		if ((eFlashRegion == psFlashTable->eFlashRegion) &&
			(u32RegionOffset >= psFlashTable->u32BlockOffset) &&
			(u32RegionOffset < (psFlashTable->u32BlockOffset + psFlashTable->u32FlashPageSize)))
		{
			uint32_t u32ChunkSize = u32ReadSize;
			uint32_t u32Available;

			// Figure out how many bytes we have/want that are available in this offset
			u32Available = (psFlashTable->u32BlockOffset + psFlashTable->u32FlashPageSize) - u32RegionOffset;
			if (u32Available < u32ChunkSize)
			{
				u32ChunkSize = u32Available;
			}

			// Set the read to the flash page
			FLASH_PAGE_READ_SET(psFlashTable->u32FlashPageRead);

			// And write to the RAM page
			RAM_PAGE_WRITE_SET();

			// Copy the data into the target buffer
			if (u32ChunkSize)
			{
				// Copy to self. Looks weird, but is correct.
				memcpy((void *) pu8Destination,
					   (void *) (psFlashTable->u32PhysicalAddress + (u32RegionOffset - psFlashTable->u32BlockOffset)), 
					   u32ChunkSize);
			}

			u32RegionOffset += u32ChunkSize;
			pu8Destination += u32ChunkSize;
			u32ReadSize -= u32ChunkSize;
		}

		++psFlashTable;
	}

	// If we have a remaining set of read bytes, then we didn't read the region
	// properly or fully, or the incoming read size exceeds the size of the
	// region.
	if (u32ReadSize)
	{
		return(false);
	}
	else
	{
		// Otherwise, we did!
		return(true);
	}
}

// How big is our buffer for verification?
#define	VERIFY_CHUNK_SIZE			1024

static uint8_t sg_u8VerifyBuffer[VERIFY_CHUNK_SIZE];

// Verifies that RAM == Flash at the given addresses/destination/offset
bool FlashVerifyCopy(EFlashRegion eFlashRegion,
					 uint32_t u32RegionOffset,
					 uint32_t u32ReadSize,
					 uint32_t *pu32FailedAddress,
					 uint32_t *pu32ExpectedData,
					 uint32_t *pu32GotData)
{
	const SFlashSegment *psFlashTable;

	psFlashTable = sg_psFlashLayout;

	while ((psFlashTable->eFlashRegion != EFLASHREGION_END) &&
		   (u32ReadSize))
	{
		if ((eFlashRegion == psFlashTable->eFlashRegion) &&
			(u32RegionOffset >= psFlashTable->u32BlockOffset) &&
			(u32RegionOffset < (psFlashTable->u32BlockOffset + psFlashTable->u32FlashPageSize)))
		{
			uint32_t u32ChunkSize = u32ReadSize;
			uint32_t u32Available;
			uint32_t *pu32ReadAddress;
			uint32_t *pu32VerifyAddress;
			uint32_t u32Loop;

			// Figure out how many bytes we have/want that are available in this offset
			u32Available = (psFlashTable->u32BlockOffset + psFlashTable->u32FlashPageSize) - u32RegionOffset;
			if (u32Available < u32ChunkSize)
			{
				u32ChunkSize = u32Available;
			}

			// Ensure the chunk size doesn't exceed the verify buffer size
			if (u32ChunkSize > sizeof(sg_u8VerifyBuffer))
			{
				u32ChunkSize = sizeof(sg_u8VerifyBuffer);
			}

			// Record the read address
			pu32ReadAddress = (uint32_t *) (psFlashTable->u32PhysicalAddress + (u32RegionOffset - psFlashTable->u32BlockOffset));

			// Set the read to the flash page
			FLASH_PAGE_READ_SET(psFlashTable->u32FlashPageRead);

			// Copy data into the verify buffer
			memcpy((void *) sg_u8VerifyBuffer,
				   (void *) pu32ReadAddress, 
				   u32ChunkSize);

			// Verify that they match
			pu32VerifyAddress = (uint32_t *) sg_u8VerifyBuffer;
			for (u32Loop = 0; u32Loop < u32ChunkSize; u32Loop += sizeof(*pu32VerifyAddress))
			{
				if (*pu32VerifyAddress != *pu32ReadAddress)
				{
					// Mismatch
					*pu32FailedAddress = (uint32_t) pu32ReadAddress;
					*pu32ExpectedData = *pu32VerifyAddress;
					*pu32GotData = *pu32ReadAddress;
					goto errorExit;
				}

				pu32ReadAddress++;
				pu32VerifyAddress++;
			}

			u32RegionOffset += u32ChunkSize;
			u32ReadSize -= u32ChunkSize;
		}
		else
		{
			++psFlashTable;
		}
	}

	// If we have a remaining set of read bytes, then we didn't read the region
	// properly or fully
errorExit:
	if (u32ReadSize)
	{
		return(false);
	}
	else
	{
		// Otherwise, we did!
		return(true);
	}
}

// Counts the number of entries in a flash segment table (including the terminator)
uint32_t FlashCountSegmentTable(const SFlashSegment *psFlashSegment)
{
	uint32_t u32FlashSegmentCount = 1;	// Add 1 for the terminator

	while (psFlashSegment->eFlashRegion != EFLASHREGION_END)
	{
		++u32FlashSegmentCount;
		++psFlashSegment;
	}

	return(u32FlashSegmentCount);
}

