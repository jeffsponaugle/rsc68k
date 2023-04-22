#ifndef _GIF_H_
#define _GIF_H_

#define	GIF_HEADER_SIZE		6

extern BOOL GfxLoadGIF(UINT8 *pu8ImagePtr,
					   UINT32 u32ImageSize,
					   SImageGroup *psImageGroup,
					   EGCResultCode *peResult);

#endif	// #ifndef _GIF_H_