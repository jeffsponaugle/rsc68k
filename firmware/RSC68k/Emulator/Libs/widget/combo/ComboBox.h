#ifndef _COMBO_H_
#define _COMBO_H_

typedef struct SComboBoxItem
{
	LEX_CHAR *peIndexTag;
	UINT32 u32SelectionIndex;				// Selection index
	COMBOBOXHANDLE eComboBoxHandle;			// Parent combo box handle
	void *pvUserData;						// User defined data
	struct SComboBoxItem *psNextLink;
	struct SComboBoxItem *psPriorLink;
} SComboBoxItem;

typedef struct SComboBoxOptions
{
	// Drop list options
	UINT32 u32TextLineCount;				// # Of lines of text to show in drop list
	UINT32 u32DropListOverallXOffset;		// Drop list overall X offset
	UINT32 u32DropListOverallYOffset;		// Drop list overall Y offset
	UINT32 u32DropListYSpacing;				// Y spacing between text objects
	UINT32 u32ItemXOffset;					// Item's X/Y offset
	UINT32 u32ItemYOffset;
	UINT32 u32ItemXBoundingBox;				// Item's X/Y bounding box
	UINT32 u32ItemYBoundingBox;
	UINT32 u32ItemXTextClip;				// Item's X/Y text clip
	UINT32 u32ItemYTextClip;
	UINT32 u32BoundingBoxColor;				// Bounding box color (if any)
	UINT32 u32MouseoverTextColor;			// Mouseover text color
	UINT32 u32MouseoverBoundingBoxColor;	// Mouseover bounding box color

	UINT16 u16TextColor;					// RGB16 Of the text color
	UINT16 u16BackgroundColor;				// Background color for all items
	UINT32 u32SelectedTextXOffset;			// X/Y offset of selected text
	UINT32 u32SelectedTextYOffset;

	// Slider related
	UINT32 u32XSliderPos;					// Slider X/Y position
	UINT32 u32YSliderPos;
	UINT32 u32TrackLength;					// Length of track

	LEX_CHAR *peDropButtonNormalFilename;	// Image filename for drop button
	LEX_CHAR *peDropButtonPressedFilename;	// Image filename for pressed button
	LEX_CHAR *peSliderTopFilename;			// Slider top filename
	LEX_CHAR *peSliderBottomFilename;		// Slider bottom filename
	LEX_CHAR *peSliderTrackFilename;		// Slider track filename
	LEX_CHAR *peSliderThumbFilename;		// Slider thumb filename
	LEX_CHAR *peDropListWindowBackgroundFilename;	// Drop down list window background filename
} SComboBoxOptions;

typedef struct SComboBox
{
	BOOL bLocked;					// Is the combo box locked insofar as updates go?
	UINT32 u32ItemSelectedIndex;	// Which item is currently selected?
	UINT32 u32ItemListBase;			// List base for item
	UINT32 u32ItemListSelect;		// Which item on the drop list are we?
	UINT32 u32ItemCount;			// Item count (# of items we have)
	BOOL bDroppedList;				// Set to TRUE if list is dropped, otherwise FALSE
	SComboBoxOptions sOptions;		// Combo box options
	SComboBoxItem *psItems;			// Items in this combo box

	INT32 s32DropListXPos;			// X Position of droplist (base coord for recalculation)
	INT32 s32DropListYPos;			// Y Position of droplist (base coord for recalculation)

	// Various handles
	FONTHANDLE eFontHandle;			// Textual font handle (what we're using)
	WINDOWHANDLE eListWindow;		// Window with the list of choosable items
	SLIDERHANDLE eSliderHandle;		// Slider handle
	TOUCHREGIONHANDLE eTouchHandle[4];	// Touch handle (used for dropping/undropping list
	BUTTONHANDLE eDropListButtonHandle;	// Drop list button handle
	TEXTHANDLE eSelectedText;		// Selected text
	TEXTHANDLE *peSelections;		// Selection handles

	// Callback when the box is changed
	void (*pCallback)(COMBOBOXHANDLE eComboBox,
					  UINT32 u32Index,
					  LEX_CHAR *peTag,
					  void *pvUserData);

	// Callback when a key has been pressed
	BOOL (*pfKeypressCallback)(COMBOBOXHANDLE eWidgetHandle, 
							   EGCCtrlKey eGCKey, 
							   LEX_CHAR eUnicode);	// Registered user keypress callback for this LineEdit

} SComboBox;

// This is used as an index for no item selected
#define	COMBOBOX_NO_ITEM_SELECTED		0xffffffff
#define COMBOBOX_ALL_ITEMS				0xffffffff

extern ELCDErr ComboBoxCreate(COMBOBOXHANDLE *peComboBoxHandle,
							  WINDOWHANDLE eWindowHandle,
							  FONTHANDLE eFontHandle,					// Font used
							  INT32 s32XPos,							// X/Y Position of combo box
							  INT32 s32YPos,
							  UINT32 u32XSize,							// X/Y Size of display box
							  UINT32 u32YSize,
							  SComboBoxOptions *psOptions,
							  BOOL bVisible);
extern ELCDErr ComboBoxAddItem(COMBOBOXHANDLE eComboBox,
							   LEX_CHAR *peTag,
							   void *pvUserData,
							   UINT32 *pu32Index,
							   BOOL bSort);
extern BUTTONHANDLE ComboBoxGetButtonHandle( COMBOBOXHANDLE eComboBox );
extern ELCDErr ComboBoxInsertItem(COMBOBOXHANDLE eComboBox,
								  UINT32 u32Index,
								  void *pvUserData,
								  UINT32 *pu32Index);
extern ELCDErr ComboBoxDeleteItem(COMBOBOXHANDLE eComboBox,
								  UINT32 u32Index);
extern ELCDErr ComboBoxGetItemSelected(COMBOBOXHANDLE eComboBox,
									   UINT32 *pu32Index,
									   LEX_CHAR **ppeTag,
									   void **ppvUserData);
extern ELCDErr ComboBoxListLock(COMBOBOXHANDLE eComboBox,
								BOOL bLock);
extern ELCDErr ComboBoxSetItemSelected(COMBOBOXHANDLE eComboBox,
									   UINT32 u32Index);
extern ELCDErr ComboBoxGetIndexedItem(COMBOBOXHANDLE eComboBox,
									  UINT32 u32Index,
									  LEX_CHAR **peTag,
									  void **pvUserData);
extern ELCDErr ComboBoxSetSelectedCallback(COMBOBOXHANDLE eComboBox,
										   void (*pCallback)(COMBOBOXHANDLE eComboBox,
															 UINT32 u32Index,
															 LEX_CHAR *peTag,
															 void *pvUserData));
extern ELCDErr ComboBoxSetUserKeystrokeCallback( COMBOBOXHANDLE eComboBoxHandle,
												 BOOL (*pfKeypressCallback)(LINEEDITHANDLE eWidgetHandle, 
																			EGCCtrlKey eGCKey, 
																			LEX_CHAR eUnicode) );

// Callback when a combo box's value has changed
typedef struct SWCBKComboBox
{
	UINT32 u32IndexSelected;
} SWCBKComboBox;

// First time init
extern void ComboBoxFirstTimeInit(void);

#endif
