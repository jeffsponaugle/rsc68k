#ifndef _GRAPHICS_H_
#define _GRAPHICS_H_

// Rotational enums

typedef enum 
{
	ROT_0,							// Not rotated
	ROT_90,							// Rotated 90' clockwise
	ROT_180,						// Rotated 180' clockwise
	ROT_270,						// Rotated 270' clockwise
	ROT_INVALID
} ERotation;

typedef enum
{
	GFXTYPE_UNKNOWN,
	GFXTYPE_TGA,
	GFXTYPE_PNG,
	GFXTYPE_BMP,
	GFXTYPE_GIF,
	GFXTYPE_JPG,
	GFXTYPE_INTERNAL				// Internally created image
} EGfxFileType;

typedef struct SDirtyBuffer
{
	// Pointer to dirty buffer array
	UINT8 *pu8DirtyBufferBase;
	UINT8 u8PixelShift;
	UINT8 u8LineShift;

	// Surface sizes in pixels/lines (destination must match source)
	UINT32 u32XSurfaceSize;
	UINT32 u32YSurfaceSize;

	// Surface information
	UINT16 *pu16SourceSurface;
	UINT32 u32SourceSurfacePitch;
	UINT16 *pu16TargetSurface;
	UINT32 u32TargetSurfacePitch;
} SDirtyBuffer;

// Graphical image blob

typedef struct SImage
{
	// What's the source image file type?
	EGfxFileType eSourceImageFileType;

	// 16BPP Stuff
	UINT16 *pu16ImageData;		// Pointer to image data (565 RGB)

	// 8BPP Stuff
	UINT8 *pu8ImageData;		// Pointer to 8BPP image data (palettized)
	UINT16 u16TransparentIndex;	// Index for 8BPP images that means "transparent", or else 0xffff if none
	UINT16 *pu16Palette;		// Color palette (RGB) for this 8bpp image

	UINT32 u32XSize;			// Image X size
	UINT32 u32YSize;			// Image Y size
	UINT32 u32Pitch;			// Image pitch (in pixels)
	UINT8 *pu8Transparent;		// Transparent channel (if it exists) - 0xff=solid, 0x00=transparent
	UINT8 *pu8TranslucentMask;	// Translucency mask
	BOOL bTranslucent;			// Image is translucent

	// pu16OriginalBase is used 

	UINT16 *pu16OriginalBase;	// Original base (non-NULL if viewport enabled)
	UINT32 u32XSizeOriginal;	// Original image's X size
	UINT32 u32YSizeOriginal;	// Original image's Y size
	UINT8 *pu8TransparentOriginal;	// Original transparency base if viewport enabled
	UINT8 *pu8TranslucentMaskOriginal;	// Original translucency base mask if viewport enabled

	// Playback delay
	UINT32 u32MSTimestamp;			// Timestamp (in ms) for this frame
	UINT32 u32FrameTime;			// How long does this frame last?

	// Frame #
	UINT32 u32FrameNumber;		// Our frame #

	// Original rotation
	ERotation eCurrentRotation;	// What is the rotation currently set to?

	// Pointer to next and prior images in link
	struct SImage *psNextLink;	// Pointer to next image
	struct SImage *psPriorLink;	// Pointer to prior image
} SImage;

// Linked list of images at the current layer

typedef struct SImageInstance
{
	SImage *psImage;
	INT32 s32XPos;				// True image X position (not clipped)
	INT32 s32YPos;				// True image Y position (not clipped)
	UINT32 u32XPos;				// Image X position (clipped to integer values)
	UINT32 u32YPos;				// Image Y position (clipped to integer values)
	UINT32 u32XOffset;			// Image X viewport offset
	UINT32 u32YOffset;			// Image Y viewport offset
	UINT32 u32XSizeClipped;		// X Size clipped to viewport
	UINT32 u32YSizeClipped;		// Y Size clipped to viewport
	BOOL bImageVisible;			// Is the image visible?
	UINT16 *pu16DestSurfacePointer;	// Precomputed surface pointer (destination)
	UINT16 *pu16SrcPointer;			// Precomputed source pointer (16bpp)
	UINT8 *pu8SrcPointer;			// Precomputed source pointer (8bpp) 
	UINT8 *pu8Transparent;			// Transparent channel (if it exists) - 0xff=solid, 0x00=transparent
	UINT8 *pu8TranslucentMask;		// Translucency mask
	UINT8 u8Intensity;				// 0=Solid, 0xff=Transparent
	struct SLayer *psParentLayer;	// Which layer does this image belong to?
	struct SImageInstance *psNextLink; // Pointer to the next image
	struct SCLUT *psCLUT;			// Which CLUT are we assigned to? NULL If none
	struct SZBufferLink *psZBufferBehind;	// List if items that intersect behind this instance
	struct SZBufferLink *psZBufferFront;	// List of items that intersect in front of this instance
} SImageInstance;

// Z Buffer intersection lists

typedef struct SZBufferLink
{
	SImageInstance *psImageInstance;
	BOOL bDrawImage;				// Used internally by the graphics drawing routine
	struct SZBufferLink *psNextLink;
	struct SZBufferLink *psPriorLink;
} SZBufferLink;

// Linked list of image instances

typedef struct SImageInstances
{
	SImageInstance *psImageInstance;
	struct SImageInstances *psNextLink;
	struct SImageInstances *psPriorLink;
} SImageInstances;

// Linked list of layers and pointer to linked lists of images on each layer

typedef struct SLayer
{
	SImageInstance *psImages;		// Pointer to any images that are on this layer currently
	struct SLayer *psPriorLayer;	// Pointer to prior layer
	struct SLayer *psNextLayer;		// Pointer to next layer
} SLayer;

typedef struct SCLUT
{
	UINT8 u8BPP;
	void *pvCLUT;
	SImageInstances *psImageInstances;	// Pointer to a linked list of image instances that use this CLUT
	struct SCLUT *psNextLink;
} SCLUT;

typedef struct SImageGroupLink
{
	SImage *psImage;					// Pointer to image in question
	struct SImageGroupLink *psNextLink;
} SImageGroupLink;

typedef enum
{
	EDIR_STOPPED = 0x00,
	EDIR_RUNNING = 0x02,
	EDIR_FORWARD  = 0x04,
	EDIR_BACKWARD = 0x08,
	EDIR_PINGPONG = 0x0c,
} EPlaybackDir;

#define	EDIR_REPEAT			0x01

struct SGfxFile;

typedef struct SImageGroup
{
	UINT32 u32FrameCount;				// How many frames do we have?
	UINT32 u32HardwareTickRate;			// How often the tick comes (in ms)
	UINT32 u32Accumulator;				// Timer tick accumulator (for animations)
	UINT32 u32TotalAnimTime;			// Total animation time
	UINT32 u32StepCount;				// # Of pending step counts
	BOOL bPendingRefresh;				// Shall a refresh occur?
	EPlaybackDir eActivePlaybackType;	// What does this image do? Active setting
	SImage *psCurrentImage;				// Currently active image
	SImageGroupLink *psLinkHead;		// Pointer to the beginning all image frames
	SImageGroupLink *psLinkTail;		// Pointer to the end of all image frames
	INT8 s8PingPongDirection;			// When in ping pong mode, which direction are we going? -1, or 1
	UINT32 u32PingPongFramesRemaining;	// # Of ping pong frames until we're done with the ping pong cycle
	struct SGfxFile *psGfxFile;			// Pointer back to the graphics file reference if there is one
} SImageGroup;

typedef struct SGfxFile
{
	LEX_CHAR *peGraphicsFilename;
	UINT32 u32References;
	SImageGroup *psImageGroup;
	struct SGfxFile *psPrevLink;
	struct SGfxFile *psNextLink;
} SGfxFile;

// Graphics functions

extern void GfxSetTransparencyKeys(UINT16 *pu16TransparencyColor,
								   UINT32 u32TransparencyColorCount,
								   SImage *psImage);
extern void GfxSetImageInstance(SImageInstance *psImageInstance,
								INT32 s32XPos,
								INT32 s32YPos,
								BOOL bVisible);
extern SImageInstance *GfxCreateImageInstance(SLayer *psLayer,
											  SImage *psImage,
											  INT32 s32XPos,
											  INT32 s32YPos,
											  BOOL bImageVisible);
extern SImageInstance *GfxCreateImageInstanceWithCLUT(SLayer *psLayer,
													  SImage *psImage,
													  INT32 s32XPos,
													  INT32 s32YPos,
													  BOOL bImageVisible,
													  SCLUT *psCLUT);
extern void GfxSetImageInstanceViewport(SImageInstance *psInstance,
										UINT32 u32XOffset,
										UINT32 u32YOffset,
										UINT32 u32XSize,
										UINT32 u32YSize);
extern void GfxDrawImageDirect(SImageInstance *psInstance,
							   SImageInstance *psDestImage,
							   UINT32 u32DestPitch,
							   BOOL bErase,
							   SDirtyBuffer *psDirtyBuffer);
extern void GfxAnimGetFrameNumber(SImageGroup *psImageGroup,
								  UINT32 *pu32FrameNumber);
extern void GfxDeleteImageInstance(SImageInstance *psInstance);

extern SImage *GfxLoadImage(UINT8 *pu8Filename,
							EGCResultCode *peResultCode);
extern EGCResultCode GfxLoadImageRaw(LEX_CHAR *peFilename, 
                                     UINT32 *pu32XSize, 
                                     UINT32 *pu32YSize, 
                                     UINT8* pu8BPP, 
                                     UINT16 **pu16Palette, 
                                     void **ppPixels);
extern SImageGroup *GfxLoadImageGroup(LEX_CHAR *peFilename,
									  EGCResultCode *peResultCode);
extern SLayer *GfxCreateLayer(void);
extern SCLUT *GfxCreateCLUT(UINT8 u8BPP);
extern void GfxDeleteCLUT(SCLUT *psCLUT);
extern void GfxUnassignCLUT(SImageInstance *psImageInstance,
							BOOL bUpdate);
extern void GfxAssignCLUT(SCLUT *psCLUT,
						  SImageInstance *psImageInstance);
extern void GfxSetCLUTFade(SCLUT *psCLUT,
						   UINT8 u8Intensity);
extern SImageGroup *GfxImageGroupCreate(void);
extern SImageGroup *GfxImageGroupAppend(SImageGroup *psGrp,
										SImage *psImage);
extern void GfxBlit(BOOL bBlitAll);
extern void GfxSurfaceCreate(UINT32 u32XSize,
							 UINT32 u32YSize,
							 UINT8 u8BPP);
extern void GfxViewportCreate(SImageInstance *psImageInstance,
							  UINT32 u32XViewportSize,
							  UINT32 u32YViewportSize);
extern void GfxViewportSet(SImageInstance *psImageInstance,
						   UINT32 u32XViewport,
						   UINT32 u32YViewport);
extern void GfxDeleteImage(SImage *psImage);
extern void GfxDeleteImageGroup(SImageGroup *psImageGroup);
extern void GfxSetImageRotation(SImage *psImage,
								ERotation eRot);
extern SImage *GfxCreateEmptyImage(UINT32 u32XSize,
								   UINT32 u32YSize,
								   UINT8 u8BPP,
								   UINT32 u32FillColor,
								   BOOL bTransparency);
extern void GfxDeleteLayer(SLayer *psLayer);
extern SLayer *GfxCreateLayerBefore(SLayer *psLayerBefore);
extern void GfxSetLayerPriority(SLayer *psLayerToSet,
								SLayer *psLayerReference,
								BOOL bFront);
extern void GfxGetCurrentSurface(UINT32 *pu32XSize,
								 UINT32 *pu32YSize,
								 UINT8 *pu8BPP);
extern SLayer *GfxGetLayerPointerByPriority(UINT32 u32Priority);
extern SImage *GfxCreateImageFromScreenshot(void);
extern void GfxSetTransparencyNonKey(UINT16 u16Color,
									 SImage *psImage);

extern const UINT16 sg_u16RedGradientSaturation[];
extern const UINT16 sg_u16GreenGradientSaturation[];
extern const UINT16 sg_u16BlueGradientSaturation[];
extern void GfxSetNewImage(SImageInstance *psInstance,
						   SImage *psImage);
extern SDirtyBuffer *GfxDirtyBufferSurfaceCreate(UINT32 u32CubeSize);
extern void GfxDirtyBufferSurfaceDelete(SDirtyBuffer *psDirtyBuffer);
extern void GfxDirtyBufferBlit(SDirtyBuffer *psDirtyBuffer);
extern void GfxDirtyBufferMark(SDirtyBuffer *psDirtyBuffer,
							   UINT32 u32XPos,
							   UINT32 u32YPos,
							   UINT32 u32XSize,
							   UINT32 u32YSize);
extern void GfxDirtyBufferMarkSurface(UINT32 u32XPos,
									  UINT32 u32YPos,
									  UINT32 u32XSize,
									  UINT32 u32YSize);
extern void GfxUpdateImage(SImageInstance *psDestImage,
						   BOOL bEraseImage);
extern void GfxUpdateImageRegion(SImageInstance *psDestImage,
								 BOOL bEraseImage,
								 UINT32 u32XPos,
								 UINT32 u32YPos,
								 UINT32 u32XSize,
								 UINT32 u32YSize);
	
extern void GfxDirtyBufferRepaint(SDirtyBuffer *psDirtyBuffer);
extern void GfxSurfaceDelete(void);
extern void GfxReplaceInstanceImage(SImageInstance *psImageInstance,
									SImage *psNewImage);
extern void GfxBlitImageToImageGray(SImage *psDest,
									SImage *psSrc,
									UINT32 u32XPos,
									UINT32 u32YPos,
									UINT32 u32XDestMax,
									UINT32 u32YDestMax);
extern void GfxBlitImageToImage(SImage *psDest,
								SImage *psSrc,
								UINT32 u32XPos,
								UINT32 u32YPos,
								UINT32 u32XDestMax,
								UINT32 u32YDestMax);
extern void ARMBlit8Bit(UINT32 iSrcWidth, 
						UINT32 iSrcHeight, 
						UINT32 iSrcPitch, 
						UINT8 *pSrc, 
						UINT16 *pDest, 
						UINT32 iDestPitch, 
						UINT16 *pPalette, 
						UINT8 ucTransparent);
extern BOOL SingleImageFixup(SImage *psImage,
							 SImageGroup *psImageGroup);
extern SImage *GfxAllocateImage(void);
extern void GfxAnimReset(SImageGroup *psImageGroup);
extern void GfxAnimSetTickRate(SImageGroup *psImageGroup,
							   UINT32 u32Milliseconds);
extern void GfxAnimSetPlaybackSpeed(SImageGroup *psImageGroup,
									UINT32 u32PlaybackSpeed);
extern BOOL GfxAnimAdvance(SImageGroup *psImageGroup,
						   UINT32 u32TickTime);
extern void GfxAnimSetActiveType(SImageGroup *psImageGroup,
								 EPlaybackDir eDir);
extern void GfxAnimSetActiveRepeat(SImageGroup *psImageGroup,
								   BOOL bRepeat);
extern void GfxAnimSetActiveRunning(SImageGroup *psImageGroup,
									BOOL bRunning);
extern BOOL GfxAnimSetFrameNumber(SImageGroup *psGroup,
								  UINT32 u32FrameNumber);
extern void GfxAnimGetActiveType(SImageGroup *psImageGroup,
								 EPlaybackDir *peDir,
								 BOOL *pbRepeat);
extern void GfxAnimStart(SImageGroup *psImageGroup);
extern void GfxAnimStop(SImageGroup *psImageGroup);
extern void GfxAnimStep(SImageGroup *psImageGroup);
extern EGCResultCode GfxRotateImage(SImage **ppsImage,
									ERotation eRot);
extern EGCResultCode GfxRotateImageGroup(SImageGroup *psImageGroup,
										 ERotation eRot);
extern SImage* GfxResizeImage( SImage* psSourceImage, UINT32 u32TargetXSize, UINT32 u32TargetYSize );
extern UINT32 GfxGetLayerPriority(SLayer *psLayer);
extern UINT32 GfxGetLayerPriorityByImageInstance(SImageInstance *psImageInstance);
extern void GfxAddSymbols(void);
extern BOOL GfxFree(BOOL bClearAll);
extern void GfxInit(void);
extern void GfxIncRef(SImageGroup *psGroup);
extern void GfxDecRef(SImageGroup *psGroup);

#define	DIRTY_PIXEL(z, x, y)	*(z->pu8DirtyBufferBase + ((x) >> z->u8PixelShift) + (((y) >> z->u8PixelShift) << z->u8LineShift)) = 1;

#define NO_CHANGE		0x7fffffff

#endif // #ifndef _GRAPHICS_H_
