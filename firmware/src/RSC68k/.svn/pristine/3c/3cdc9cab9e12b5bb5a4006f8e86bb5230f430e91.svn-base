#include <assert.h>
#include <stdint.h>
#include "Shared/Version.h"
#include "Shared/zlib/zlib.h"

typedef struct SVersionCodeText
{
	EVersionCode eVersionCode;
	char *peVersionCodeText;
} SVersionCodeText;

// Translate EVersionCodes to text
static const SVersionCodeText sg_sVersionCodeTable[] =
{
	{EVERSION_UNKNOWN,				"Unknown"},
	{EVERSION_OK,					"Ok"},
	{EVERSION_BAD_PREFIX,			"Bad prefix"},
	{EVERSION_BAD_SUFFIX,			"Bad suffix"},
	{EVERSION_BAD_IMAGE_TYPE,		"Bad image type"},
	{EVERSION_BAD_STRUCT_CRC,		"Bad image structure CRC32"},
	{EVERSION_BAD_IMAGE_CRC,		"Bad image CRC32"},
	{EVERSION_BAD_LOAD_ADDRESS,		"Bad load address"},
	{EVERSION_BAD_ENTRY_POINT,		"Bad entry point"},
	{EVERSION_BAD_BSS_START,		"Bad start of _bss"},
	{EVERSION_BAD_END,				"Bad _end"},
	{EVERSION_BAD_STACK_TOP,		"Bad top of stack"},
	{EVERSION_BAD_IMAGE_SIZE,		"Bad image size"}
};

// Returns textual equivalent of eVersionCode
char *VersionCodeGetText(EVersionCode eVersionCode)
{
	uint8_t u8Loop;

	for (u8Loop = 0; u8Loop < (sizeof(sg_sVersionCodeTable) / sizeof(sg_sVersionCodeTable[0])); u8Loop++)
	{
		if (sg_sVersionCodeTable[u8Loop].eVersionCode == eVersionCode)
		{
			return(sg_sVersionCodeTable[u8Loop].peVersionCodeText);
		}
	}

	return("Unknown");
}

// Validates an image version structure
EVersionCode VersionValidateStructure(SImageVersion *psImageVersion)
{
	EVersionCode eVersionCode;
	uint32_t u32CRC;

	// Do we have a proper prefix?
	if (psImageVersion->u32VersionPrefix != VERSION_PREFIX)
	{
		eVersionCode = EVERSION_BAD_PREFIX;
		goto errorExit;
	}

	// And a proper suffix?
	if (psImageVersion->u32VersionSuffix != VERSION_SUFFIX)
	{
		eVersionCode = EVERSION_BAD_SUFFIX;
		goto errorExit;
	}

	// Is the image type known?
	if ((psImageVersion > EIMGTYPE_UNKNOWN) && (psImageVersion->eImageType < EIMGTYPE_COUNT))
	{
		// Image type is reasonable
	}
	else
	{
		// Image type isn't reasonable
		eVersionCode = EVERSION_BAD_IMAGE_TYPE;
		goto errorExit;
	}

	// Is the load address above this structure? If so, then it's not reasonable
	if (psImageVersion->u32LoadAddress > ((uint32_t) psImageVersion))
	{
		eVersionCode = EVERSION_BAD_LOAD_ADDRESS;
		goto errorExit;
	}

	// If the bss is smaller than the load address, it's bad
	if (psImageVersion->u32BssStart < psImageVersion->u32LoadAddress)
	{
		eVersionCode = EVERSION_BAD_BSS_START;
		goto errorExit;
	}

	// If the entry point is smaller than the load address or the entry point is 
	// past the end of the start of bss, something is wrong
	if ((psImageVersion->u32EntryPoint < psImageVersion->u32LoadAddress) ||
		(psImageVersion->u32EntryPoint >= psImageVersion->u32BssStart))
	{
		eVersionCode = EVERSION_BAD_ENTRY_POINT;
		goto errorExit;
	}

	// u32BssEnd must come after u32BssStart
	if (psImageVersion->u32BssEnd < psImageVersion->u32BssStart)
	{
		eVersionCode = EVERSION_BAD_END;
		goto errorExit;
	}

	// CRC the structure. Remember... don't CRC the CRC32 field
	u32CRC = crc32(VERSION_CRC32_STRUCT_INITIAL,
				   (const unsigned char *) psImageVersion,
				   ((uint32_t) &psImageVersion->u32VersionStructCRC32) - ((uint32_t) psImageVersion));

	// And CRC everything past the u32VersionStructCRC32 member
	u32CRC = crc32(u32CRC,
				   ((const unsigned char *) &psImageVersion->u32VersionStructCRC32) + sizeof(psImageVersion->u32VersionStructCRC32),
				   ((uint32_t) (psImageVersion + 1)) - (((uint32_t) &psImageVersion->u32VersionStructCRC32) + sizeof(psImageVersion->u32VersionStructCRC32)));

	// If the CRC doesn't match, then it's bad
	if (psImageVersion->u32VersionStructCRC32 != u32CRC)
	{
		eVersionCode = EVERSION_BAD_STRUCT_CRC;
		goto errorExit;
	}

	// All good!
	eVersionCode = EVERSION_OK;

errorExit:
	return(eVersionCode);
}

// Finds a version structure within a region of memory.
EVersionCode VersionFindStructure(uint8_t *pu8Address,
								  uint32_t u32RegionSize,
								  SImageVersion **ppsImageVersion)
{
	EVersionCode eVersionCode = EVERSION_UNKNOWN;

	// If the region is smaller than the version structure, then don't even try.
	if (u32RegionSize < sizeof(**ppsImageVersion))
	{
		goto errorExit;
	}

	while (u32RegionSize)
	{
		if (VERSION_PREFIX == *((uint32_t *) pu8Address))
		{
			// See if this is an actual structure and it's good
			eVersionCode = VersionValidateStructure((SImageVersion *) pu8Address);
			if (EVERSION_OK == eVersionCode)
			{
				// Found it!
				*ppsImageVersion = (SImageVersion *) pu8Address;
				goto errorExit;
			}
		}

		u32RegionSize -= sizeof(uint32_t);
		pu8Address += sizeof(uint32_t);
	}

errorExit:
	return(eVersionCode);
}

