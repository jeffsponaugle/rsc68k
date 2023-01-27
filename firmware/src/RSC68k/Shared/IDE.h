#ifndef _IDE_H_
#define _IDE_H_

extern EStatus IDEProbe(void);
extern EStatus IDEReadWriteSectors(uint8_t u8Disk,
								   uint32_t u32Sector,
								   uint32_t u32SectorCount,
								   uint8_t *pu8Buffer,
								   bool bWrite);
#define	IDEReadSector(disk, sector, sector_count, buffer)	IDEReadWriteSectors(disk, sector, sector_count, buffer, false)
#define	IDEWriteSector(disk, sector, sector_count, buffer)	IDEReadWriteSectors(disk, sector, sector_count, buffer, true)

#endif

