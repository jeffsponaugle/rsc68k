#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <machine/endian.h>
#include "BIOS/OS.h"
#include "Shared/IDE.h"
#include "Hardware/RSC68k.h"
#include "Shared/AsmUtils.h"
#include "Shared/Shared.h"

#define	IDE_REG8(csbase, reg)		*((volatile uint8_t *) (csbase + (reg << 5)))
#define	IDE_REG16(csbase, reg)		*((volatile uint16_t *) (csbase + (reg << 5)))
#define	IDE_REG32(csbase, reg)		*((volatile uint32_t *) (csbase + (reg << 5)))

// Uncomment if you want to use the optimized/more efficient sector moving from IDE
#define	FAST_SECTOR_MOVE		1

// IDE Registers
#define	IDE_REG_DATA				IDE_REG32(RSC68KHW_DEVCOM_IDE_CSA, 0)
#define	IDE_REG_ERROR				IDE_REG8(RSC68KHW_DEVCOM_IDE_CSA, 1)
#define	IDE_REG_SECCOUNT0			IDE_REG8(RSC68KHW_DEVCOM_IDE_CSA, 2)
#define	IDE_REG_LBA0  		 		IDE_REG8(RSC68KHW_DEVCOM_IDE_CSA, 3)
#define	IDE_REG_LBA1				IDE_REG8(RSC68KHW_DEVCOM_IDE_CSA, 4)
#define	IDE_REG_LBA2				IDE_REG8(RSC68KHW_DEVCOM_IDE_CSA, 5)
#define	IDE_REG_STATUS				IDE_REG8(RSC68KHW_DEVCOM_IDE_CSA, 7)
#define	IDE_REG_CMD					IDE_REG8(RSC68KHW_DEVCOM_IDE_CSA, 7)
#define	IDE_REG_HDDEVSEL			IDE_REG8(RSC68KHW_DEVCOM_IDE_CSA, 6)
#define	IDE_REG_STATUSALT			IDE_REG8(RSC68KHW_DEVCOM_IDE_CSB, 5)

// IDE Commands
#define	IDE_CMD_READ_PIO		0x20
#define	IDE_CMD_WRITE_PIO		0x30
#define	IDE_CMD_SPINDOWN		0xe0
#define	IDE_CMD_SPINUP			0xe1
#define	IDE_CMD_IDENTIFY   		0xec

// IDE Status bits
#define	IDE_STATUS_BUSY			0x80
#define IDE_STATUS_DISK_READY	0x40
#define IDE_STATUS_WRITE_FAULT	0x20
#define IDE_STATUS_SEEK_DONE	0x10
#define IDE_STATUS_DATA_READY	0x08
#define IDE_STATUS_CORRECTED	0x04
#define IDE_STATUS_INDEX		0x02
#define IDE_STATUS_ERROR		0x01

// IDE Select command bits
typedef enum
{
	EIDESTATE_NOT_PROBED,
	EIDESTATE_NOT_PRESENT,
	EIDESTATE_PRESENT
} EIDEState;

// Block mode
typedef enum
{
	EIDEBLOCK_UNKNOWN,
	EIDEBLOCK_CHS,
	EIDEBLOCK_LBA28,
	EIDEBLOCK_LBA48
} EIDEBlockMode;

// Structure containing information about the disk
typedef struct SIDEInfo
{
	EIDEState eState;					// What's the state of this disk
	char eDriveModel[40];				// Textual model of the drive
	bool bMaster;						// Is this disk the master?
	uint16_t u16DeviceType;				// Device type
	uint16_t u16Capabilities;			// Capabilities
	uint32_t u32CommandSets;			// Command sets supported
	uint32_t u32DiskSectorCount;   		// How many sectors does this disk have?
	EIDEBlockMode eBlockMode;			// What block mode does this disk support?
} SIDEDiskInfo;

// Set TRUE if we have the master disk currently selected
static bool sg_bMasterSelected = false;

// Information about the disks in this system
static SIDEDiskInfo sg_sDisks[2];

// Approximately 1ms delay
#define	IDE_DELAY_MS_CONSTANT	15000

// Sleep # of milliseconds
static void IDESleep(uint16_t u16Milliseconds)
{
	volatile uint32_t u32Counter;

	// Appro
	u32Counter = IDE_DELAY_MS_CONSTANT * u16Milliseconds;

	while (u32Counter--);
}

// This waits for an IDE disk to become not busy, or times out
static EStatus IDECheckBusy(void)
{
	uint32_t u32Timeout = 1000000;

	while (IDE_REG_STATUS & IDE_STATUS_BUSY)
	{
		if (0 == u32Timeout)
		{
			return(ESTATUS_TIMEOUT);
		}

		u32Timeout--;
	}
	return(ESTATUS_OK);
}

// Select the master or the slave disk
void IDESelect(bool bMaster)
{
	EStatus eStatus;

	// Select the appropriate disk
	if (bMaster)
	{
		IDE_REG_HDDEVSEL = 0xa0;
	}
	else
	{
		IDE_REG_HDDEVSEL = 0xb0;
	}

	// Wait 1ms for things to settle
	IDESleep(1);

	sg_bMasterSelected = bMaster;
}

// IDE Polling
static EStatus IDEPolling(uint8_t u8Channel,
						  bool bCheckAdvanced)
{
	EStatus eStatus = ESTATUS_OK;
	volatile uint8_t u8Data;
	uint8_t u8Loop;

	// 400ns delay before we start talking to the disk
	for (u8Loop = 0; u8Loop < 8; u8Loop++)
	{
		u8Data = IDE_REG_STATUSALT;
	}

	// Now we wait for the disk to become not busy
	while (1)
	{
		u8Data = IDE_REG_STATUS;
		if (u8Data & IDE_STATUS_BUSY)
		{
			// Keep going
		}
		else
		{
			// No longer busy
			break;
		}
	}

	// If we want more errors to look at, do so
	if (bCheckAdvanced)
	{
		u8Data = IDE_REG_STATUS;

		if (u8Data & IDE_STATUS_ERROR)
		{
			eStatus = ESTATUS_DISK_ERROR;
			printf("Error register - 0x%.2x\r\n", IDE_REG_ERROR);
			goto errorExit;
		}

		if (u8Data & IDE_STATUS_WRITE_FAULT)
		{
			eStatus = ESTATUS_DISK_WRITE_FAULT;
			goto errorExit;
		}

		if (u8Data & IDE_STATUS_DATA_READY)
		{
			eStatus = ESTATUS_DISK_DATA_READY;
			goto errorExit;
		}
	}

errorExit:
	return(eStatus);
}

// Selects a block # - LBA or CHS - and # of sectors to traansfer
static void IDESectorSelect(uint8_t u8Disk,
							uint32_t u32Sector,
							uint8_t u8SectorCount)
{
	SIDEDiskInfo *psDisk;
	uint8_t u8ModeSel;
	uint16_t u16Cylinder;
	uint8_t u8Sector;
	uint8_t u8Head;

	psDisk = &sg_sDisks[u8Disk];

	if (psDisk->bMaster)
	{
		u8ModeSel = 0xa0;
	}
	else 
	{
		u8ModeSel = 0xb0;
	}

	if (EIDEBLOCK_LBA28 == psDisk->eBlockMode)
	{
		// LBA28
		u8ModeSel |= ((u32Sector >> 24) & 0xf) | 0x40;	// Lower 4 bits are the high 4 LBA bits and 0x40 is LBA mode
	}
	else
	if (EIDEBLOCK_LBA48 == psDisk->eBlockMode)
	{
		// LBA48
	}
	else
	if (EIDEBLOCK_CHS == psDisk->eBlockMode)
	{
		// CHS
		u8Sector = (u32Sector % 63) + 1;
		u16Cylinder = ((u32Sector + 1) - (u8Sector)) / (16 * 63);
		u8Head = (uint8_t) (u16Cylinder / 63);
		u8ModeSel |= (u8Head & 0xf);
	}

	IDE_REG_HDDEVSEL = u8ModeSel;
	IDE_REG_SECCOUNT0 = u8SectorCount;

	if (EIDEBLOCK_LBA28 == psDisk->eBlockMode)
	{
		IDE_REG_LBA0 = (uint8_t) u32Sector;
		IDE_REG_LBA1 = (uint8_t) (u32Sector >> 8);
		IDE_REG_LBA2 = (uint8_t) (u32Sector >> 16);
	}
	else
	if (EIDEBLOCK_LBA48 == psDisk->eBlockMode)
	{
		IDE_REG_LBA0 = (uint8_t) u32Sector;
		IDE_REG_LBA1 = (uint8_t) (u32Sector >> 8);
		IDE_REG_LBA2 = (uint8_t) (u32Sector >> 16);
	}
	else
	{
		IDE_REG_LBA0 = (uint8_t) u8Sector;
		IDE_REG_LBA1 = (uint8_t) (u16Cylinder >> 0);
		IDE_REG_LBA2 = (uint8_t) (u16Cylinder);
	}
}

// Return the disk identify data
static EStatus IDEIdentify(volatile uint8_t *pu8IdentifyStructure)
{
	EStatus eStatus;
	uint8_t u8Status;
	uint16_t u16Count = 128;
	uint32_t u32Timeout = 100000;

	// Send the identify command
	IDE_REG_CMD = IDE_CMD_IDENTIFY;

	// Wait 1ms for things to settle
	IDESleep(1);

	// Now look at the resulting status
	for (;;)
	{
		// Read the status
		u8Status = IDE_REG_STATUS;

		// If we have a disk error...
		if (u8Status & IDE_STATUS_ERROR)
		{
			eStatus = ESTATUS_DISK_ERROR;
			goto errorExit;
		}

		// If we're not busy and we have data ready, go get it!
		if ((u8Status & (IDE_STATUS_BUSY | IDE_STATUS_DATA_READY)) == IDE_STATUS_DATA_READY)
		{
			// Got it!
			break;
		}

		u32Timeout--;
		if (0 == u32Timeout)
		{
			eStatus = ESTATUS_TIMEOUT;
			goto errorExit;
		}
	}

	// Now read 256 16 bit words
	while (u16Count)
	{
		*((uint32_t *) pu8IdentifyStructure) = IDE_REG_DATA;
		pu8IdentifyStructure += sizeof(uint32_t);
		u16Count--;
	}

	eStatus = ESTATUS_OK;

errorExit:
	return(eStatus);
}

// Checks that the disk, sector #s and count are in range, and the buffer is
// uint32_t aligned
static EStatus IDECheckRanges(uint8_t u8Disk,
							  uint32_t u32Sector,
							  uint32_t u32SectorCount,
							  uint8_t *pu8Buffer)
{
	EStatus eStatus;

	// Check to see if the disk is present
	if (u8Disk >= (sizeof(sg_sDisks) / sizeof(sg_sDisks[0])))
	{
		eStatus = ESTATUS_DISK_OUT_OF_RANGE;
		goto errorExit;
	}

	// And see if the disk is there
	if (sg_sDisks[u8Disk].eState != EIDESTATE_PRESENT)
	{
		eStatus = ESTATUS_DISK_NOT_PRESENT;
		goto errorExit;
	}

	// And see if the sector reads are out of range
	if ((u32Sector + u32SectorCount) > sg_sDisks[u8Disk].u32DiskSectorCount)
	{
		eStatus = ESTATUS_DISK_SECTOR_OUT_OF_RANGE;
		goto errorExit;
	}

	// Make sure the disk buffer is aligned
	if (((uint32_t) pu8Buffer) & 3)
	{
		eStatus = ESTATUS_DISK_BUFFER_NOT_ALIGNED;
		goto errorExit;
	}

	// Can't be no sectors!
	if (0 == u32SectorCount)
	{
		eStatus = ESTATUS_DISK_SECTOR_COUNT_0;
		goto errorExit;
	}

	eStatus = ESTATUS_OK;

errorExit:
	return(eStatus);
}

EStatus IDEReadWriteSectors(uint8_t u8Disk,
							uint32_t u32Sector,
							uint32_t u32SectorCount,
							uint8_t *pu8Buffer,
							bool bWrite)
{
	EStatus eStatus;
	uint16_t u16Loop;

	eStatus = IDECheckRanges(u8Disk,
							 u32Sector,
							 u32SectorCount,
							 pu8Buffer);
	ERR_GOTO();

	while (u32SectorCount)
	{
		uint8_t u8SectorChunk;

		if (u32SectorCount > 255)
		{
			u8SectorChunk = 255;
		}
		else
		{
			u8SectorChunk = (uint8_t) u32SectorCount;
		}

		// Go select the sector
		IDESectorSelect(u8Disk,
						u32Sector,
						u8SectorChunk);

		if (bWrite)
		{
			// Now issue the command to write
			IDE_REG_CMD = IDE_CMD_WRITE_PIO;
		}
		else
		{
			// Now issue the command to read
			IDE_REG_CMD = IDE_CMD_READ_PIO;
		}

		// Make the disk go do it
		eStatus = IDEPolling(u8Disk,
							 true);

		if (bWrite)
		{
			if ((eStatus != ESTATUS_OK) &&
				(eStatus != ESTATUS_DISK_DATA_READY))
			{
				printf("%s: Write - Call to IDEPolling failed - %s\r\n", __FUNCTION__, GetErrorText(eStatus));
				goto errorExit;
			}
		}
		else
		{
			if (eStatus != ESTATUS_DISK_DATA_READY)
			{
				printf("%s: Read - Call to IDEPolling failed - 0x%.8x\r\n", __FUNCTION__, GetErrorText(eStatus));
				goto errorExit;
			}
		}

		// Sector selected - time to read or write

		// Convert to # of 32 bit values to read (sector size is 512, <<7 = 128 32 bit values)
		u16Loop = ((uint16_t) u8SectorChunk) << 7;

		if (bWrite)
		{
			while (u16Loop)
			{
				IDE_REG_DATA = *((uint32_t *) pu8Buffer);
				pu8Buffer += sizeof(uint32_t);
				u16Loop--;
			}
		}
		else
		{
			// Move the sector from the disk to main memory
#ifdef FAST_SECTOR_MOVE
			IDESectorMoveRead((void *) pu8Buffer,
							  (((uint32_t) pu8Buffer) + (((uint32_t) u8SectorChunk) << 9)));
			pu8Buffer += (u8SectorChunk << 9);
#else
			while (u16Loop)
			{
				*((uint32_t *) pu8Buffer) = IDE_REG_DATA;
				pu8Buffer += sizeof(uint32_t);
				u16Loop--;
			}
#endif
		}

		u32SectorCount -= u8SectorChunk;
		u32Sector += u8SectorChunk;
	}

	// Got the sector(s)!
	eStatus = ESTATUS_OK;

errorExit:
	return(eStatus);
}

// Identify structure offsets
#define	IDE_DEVICETYPE			0
#define	IDE_IDENT_MODEL			54
#define	IDE_CAPABILITIES		98
#define	IDE_MAX_LBA				120
#define	IDE_COMMANDSETS			164
#define	IDE_MAX_LBA_EXT			200

static EStatus IDEProbeDisk(SIDEDiskInfo *psDisk,
							bool bMaster)
{
	EStatus eStatus;
	uint8_t u8Loop;
	volatile uint8_t *pu8IdentifyInfo = NULL;
	volatile uint16_t *pu16IdentifyInfo;
	char *peDisk = "Master";
	char *peSectorAccess = NULL;

	ZERO_STRUCT(*psDisk);
	psDisk->eState = EIDESTATE_NOT_PROBED;

	if (false == bMaster)
	{
		peDisk = "Slave ";
	}

	// 512 Byte buffer for identify stuff
	pu8IdentifyInfo = malloc(512);
	pu16IdentifyInfo = (uint16_t *) pu8IdentifyInfo;
	if (NULL == pu8IdentifyInfo)
	{
		eStatus = ESTATUS_OUT_OF_MEMORY;
		goto errorExit;
	}

	// Select the disk
	IDESelect(bMaster);

	psDisk->bMaster = bMaster;

	eStatus = IDEIdentify(pu8IdentifyInfo);
	if (ESTATUS_OK == eStatus)
	{
		uint8_t u8Loop;
		uint8_t u8LastNonSpace;

		psDisk->eState = EIDESTATE_PRESENT;
		for (u8Loop = 0; u8Loop < sizeof(psDisk->eDriveModel); u8Loop += 2)
		{
			psDisk->eDriveModel[u8Loop] = *(pu8IdentifyInfo + u8Loop + IDE_IDENT_MODEL + 1);
			psDisk->eDriveModel[u8Loop + 1] = *(pu8IdentifyInfo + u8Loop + IDE_IDENT_MODEL);
		}

		// Zero terminate it
		psDisk->eDriveModel[sizeof(psDisk->eDriveModel) - 1] = '\0';

		// Get rid of trailing spaces
		u8LastNonSpace = 0;
		for (u8Loop = 0; u8Loop < sizeof(psDisk->eDriveModel); u8Loop++)
		{
			if ('\0' == psDisk->eDriveModel[u8Loop])
			{
				// End of string
				break;
			}

			if (psDisk->eDriveModel[u8Loop] != ' ')
			{
				u8LastNonSpace = u8Loop;
			}
		}

		// Pull out various things we might care about
		psDisk->u16DeviceType = __bswap16(*((volatile uint16_t *) (pu8IdentifyInfo + IDE_DEVICETYPE)));
		psDisk->u16Capabilities = __bswap16(*((volatile uint16_t *) (pu8IdentifyInfo + IDE_CAPABILITIES)));
		psDisk->u32CommandSets = (__bswap16(pu16IdentifyInfo[IDE_COMMANDSETS >> 1])) |
								 (__bswap16(pu16IdentifyInfo[(IDE_COMMANDSETS + 2) >> 1]) << 16);

		// Zero terminate the last significant character
		psDisk->eDriveModel[u8LastNonSpace + 1] = '\0';

		if (psDisk->u16Capabilities & 0x200)
		{
			// Supports LBA
			// Figure out if we're 48 or 28 bit addressing
			if (psDisk->u32CommandSets & (1 << 26))
			{
				// 48 Bit LBA
				psDisk->u32DiskSectorCount = *((volatile uint32_t *) (pu8IdentifyInfo + IDE_MAX_LBA_EXT));
				psDisk->eBlockMode = EIDEBLOCK_LBA48;
				peSectorAccess = "LBA48";
			}
			else
			{
				// 28 Bit LBA
				psDisk->u32DiskSectorCount = (__bswap16(pu16IdentifyInfo[IDE_MAX_LBA >> 1])) |
											 (__bswap16(pu16IdentifyInfo[(IDE_MAX_LBA + 2) >> 1]) << 16);
				peSectorAccess = "LBA28";
				psDisk->eBlockMode = EIDEBLOCK_LBA28;
			}
		}
		else
		{
			// CHS Only
			peSectorAccess = "CHS";
			psDisk->eBlockMode = EIDEBLOCK_CHS;
		}

		// Convert # of 512 byte sectors to megabytes
		printf("IDE %s   : %s - %uMB (%s)\r\n", peDisk, psDisk->eDriveModel, (uint32_t) (psDisk->u32DiskSectorCount >> 11), peSectorAccess);
	}
	else
	{
		printf("IDE %s   : Not present\r\n", peDisk);
		psDisk->eState = EIDESTATE_NOT_PRESENT;
	}

errorExit:
	if (pu8IdentifyInfo)
	{
		free((char *) pu8IdentifyInfo);
	}

	return (eStatus);
}

// Probe disks
EStatus IDEProbe(void)
{
	EStatus eStatus;

	ZERO_STRUCT(sg_sDisks);

	// Look for the master
	eStatus = IDEProbeDisk(&sg_sDisks[0],
						   true);

	// And the slave
	eStatus = IDEProbeDisk(&sg_sDisks[1],
						   false);

	return(ESTATUS_OK);
}

