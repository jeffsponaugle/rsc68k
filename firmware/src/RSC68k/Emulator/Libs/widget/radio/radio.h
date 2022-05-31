#ifndef _RADIO_H_
#define _RADIO_H_

typedef struct SRadioItem
{
	UINT32 u32XOffset;
	UINT32 u32YOffset;
	BOOL bCheckbox;					// TRUE=Checkbox item

	// Runtime calculated positions
	UINT32 u32XOffsetCalculated;
	UINT32 u32YOffsetCalculated;
	UINT32 u32XSize;
	UINT32 u32YSize;

	// Text gunk
	LEX_CHAR *peText;
	FONTHANDLE eFontHandle;
	UINT16 u16Orientation;
	UINT32 u32TextColor;
	UINT32 u32TextXSize;
	UINT32 u32TextYSize;

	SImageGroup *psSelectedImage;
	SImageGroup *psNonselectedImage;

	BOOL bSelected;
	BOOL bDisabled;
	BOOL bVisible;

	struct SRadioItem *psNextLink;
} SRadioItem;

typedef struct SRadio
{
	RADIOGROUPHANDLE eRadioGroupHandle;	// This radio's handle
	WINDOWHANDLE eWindowHandle;		// Window this radio button is related to

	BOOL bEnabled;					// Is the radio group enabled
	BOOL bRightBottomJustified;		// Bottom/right justified?

	UINT32 u32XMaxSize;				// Largest X item size
	UINT32 u32YMaxSize;				// Largest Y item size

	SImageGroup *psDefaultSelected;	// Default selected image
	SImageGroup *psDefaultNonselected;	// Default nonselected image

	SOUNDHANDLE eSoundHandle;		// Radio sound handle

	SRadioItem *psItemList;			// Radio item list
	SRadioItem *psItemSelected;		// Which item is currently selected?

	// Pointer to widget
	struct SWidget *psWidget;		// Pointer to parent widget
} SRadio;

extern SRadio *RadioGetPointer(RADIOGROUPHANDLE eHandle);
extern void RadioFirstTimeInit(void);
extern ELCDErr RadioGroupCreate(RADIOGROUPHANDLE *peRadioGroupHandle,
								WINDOWHANDLE eWindowHandle,
								UINT32 u32XPos,
								UINT32 u32YPos,
								BOOL bRightBottomJustified,
								BOOL bVisible);
extern ELCDErr RadioGroupSetDefaultImages(RADIOGROUPHANDLE eRadioGroupHandle,
										  SImageGroup *psDefaultSelectedImage,
										  SImageGroup *psDefaultNonselectedImage,
										  BOOL bLockWindow);
extern ELCDErr RadioGroupSetHide(RADIOGROUPHANDLE eRadioGroupHandle,
								 UINT32 *pu32HideIndex,
								 BOOL bHidden);
extern ELCDErr RadioGroupSetEnable(RADIOGROUPHANDLE eRadioGroupHandle,
								   UINT32 *pu32EnableIndex,
								   BOOL bEnable);
extern ELCDErr RadioGroupDestroy(RADIOGROUPHANDLE *peRadioGroupHandle);
extern ELCDErr RadioSetSound(RADIOGROUPHANDLE eRadioGroupHandle,
							 SOUNDHANDLE eSoundHandle);
extern ELCDErr RadioGroupAdd(RADIOGROUPHANDLE eRadioGroupHandle,
							 UINT32 u32XOrigin,
							 UINT32 u32YOrigin,
							 LEX_CHAR *peText,
							 UINT32 u32TextColor,
							 LEX_CHAR *peFontFilename,
							 UINT32 u32FontSize,
							 UINT16 u16Orientation,
							 BOOL bSelected,
							 BOOL bDisabled,
							 BOOL bVisible,
							 BOOL bSimple,
							 SImageGroup *psSelected,
							 SImageGroup *psNonselected,
							 BOOL bForceSelected,
							 BOOL bCheckbox);
extern ELCDErr RadioGroupSetSelected(RADIOGROUPHANDLE eRadioGroupHandle,
									 UINT32 u32Selected,
									 BOOL bState);
extern ELCDErr RadioGroupGetSelected(RADIOGROUPHANDLE eRadioGroupHandle,
									 UINT32 *pu32Selected,
									 BOOL *pbState);

#endif	// #ifndef _RADIO_H_