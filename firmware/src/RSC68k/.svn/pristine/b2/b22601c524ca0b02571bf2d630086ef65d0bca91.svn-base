#include "Startup/app.h"
#include "Libs/Gfx/GraphicsLib.h"
#include "Libs/window/window.h"
#include "Libs/widget/widget.h"
#include "Libs/widget/combo/ComboBox.h"

static ELCDErr ComboBoxWidgetAlloc(SWidget *psWidget,
								   WIDGETHANDLE eHandle)
{
	SComboBox *psComboBox;

	GCASSERT(psWidget);
	psComboBox = MemAlloc(sizeof(*psWidget->uWidgetSpecific.psComboBox));
	psWidget->uWidgetSpecific.psComboBox = psComboBox;
	if (NULL == psComboBox)
	{
		return(LERR_NO_MEM);
	}
	else
	{
		UINT8 u8Loop;

		// Set up some defatuls
		psComboBox->eListWindow = HANDLE_INVALID;
		psComboBox->eSliderHandle = HANDLE_INVALID;
		psComboBox->eSelectedText = HANDLE_INVALID;
		psComboBox->eDropListButtonHandle = HANDLE_INVALID;

		for (u8Loop = 0; u8Loop < (sizeof(psComboBox->eTouchHandle) / sizeof(psComboBox->eTouchHandle[0])); u8Loop++)
		{
			psComboBox->eTouchHandle[u8Loop] = HANDLE_INVALID;
		}

		return(LERR_OK);
	}
}

static ELCDErr ComboBoxWidgetFree(SWidget *psWidget)
{
	SComboBox *psComboBox;
	SComboBoxItem *psItem;
	SComboBoxItem *psItemPtr;
	ELCDErr eLCDErr;
	UINT32 u32Loop;

	GCASSERT(psWidget);
	GCASSERT(psWidget->uWidgetSpecific.psComboBox);

	psComboBox = psWidget->uWidgetSpecific.psComboBox;

	// Free a bunch of this stuff

	// Delete the drop list window, which will delete all of the text widgets, sliders, etc...
	WindowDelete(psComboBox->eListWindow);
	psComboBox->eListWindow = HANDLE_INVALID;


	// Get rid of the touch regions
	for (u32Loop = 0; u32Loop < (sizeof(psComboBox->eTouchHandle) / sizeof(psComboBox->eTouchHandle[0])); u32Loop++)
	{
		eLCDErr = WidgetDestroyByHandle((WIDGETHANDLE *) &psComboBox->eTouchHandle[u32Loop],
										WIDGET_TOUCH);
		GCASSERT(LERR_OK == eLCDErr);
	}

	// Free the selections memory
	GCFreeMemory(psComboBox->peSelections);
	psComboBox->peSelections = NULL;

	// Free all of the items in the list
	psItem = psComboBox->psItems;

	while (psItem)
	{
		psItemPtr = psItem;
		psItem = psItem->psNextLink;

		// Free the items within the structure
		if (psItemPtr->peIndexTag)
		{
			GCFreeMemory(psItemPtr->peIndexTag);
			psItemPtr->peIndexTag = NULL;
		}

		memset((void *) psItemPtr, 0, sizeof(*psItemPtr));
		GCFreeMemory(psItemPtr);
	}

	GCFreeMemory(psWidget->uWidgetSpecific.psComboBox);
	psWidget->uWidgetSpecific.psComboBox = NULL;
	return(LERR_OK);
}

static BOOL ComboBoxHitTest(SWidget *psWidget,
							UINT32 u32XPos, 
							UINT32 u32YPos)
{
	SComboBox *psComboBox = psWidget->uWidgetSpecific.psComboBox;

	return(TRUE);
}

static SComboBoxItem *ComboBoxIndexToItem(SComboBox *psComboBox,
										  UINT32 u32Index)
{
	SComboBoxItem *psItem;

	// Start at the beginning
	psItem = psComboBox->psItems;

	if (COMBOBOX_NO_ITEM_SELECTED == u32Index)
	{
		return(psItem);
	}

	// Yeah, I know, linear relationship, but doesn't matter on the CPU and OS we're running on
	while (u32Index && psItem)
	{
		psItem = psItem->psNextLink;
		u32Index--;
	}

	return(psItem);
}

static void ComboBoxPopulateListItems(COMBOBOXHANDLE eComboBoxHandle,
									  SComboBox *psComboBox,
									  UINT32 u32ItemIndex)
{
	SComboBoxItem *psItem;
	ELCDErr eLCDErr;
	UINT32 u32Loop;

	psItem = ComboBoxIndexToItem(psComboBox,
								 u32ItemIndex);

	for (u32Loop = 0; u32Loop < psComboBox->sOptions.u32TextLineCount; u32Loop++)
	{
		if (psItem)
		{
			eLCDErr = TextSetText(psComboBox->peSelections[u32Loop],
								  psItem->peIndexTag);
			GCASSERT(LERR_OK == eLCDErr);

			psItem->eComboBoxHandle = eComboBoxHandle;

			psItem->u32SelectionIndex = u32ItemIndex;
			++u32ItemIndex;
				
		}
		else
		{
			eLCDErr = TextSetText(psComboBox->peSelections[u32Loop],
								  "");
			GCASSERT(LERR_OK == eLCDErr);
		}

		// Widget user data is the combo box item structure
		eLCDErr = WidgetSetUserData((WIDGETHANDLE) psComboBox->peSelections[u32Loop],
									(void *) psItem);
		GCASSERT(LERR_OK == eLCDErr);

		if (psItem)
		{
			psItem = psItem->psNextLink;
		}
	}
}

static void ComboBoxPaint(SWidget *psWidget,
						  BOOL bLock)
{
	ELCDErr eLCDErr;
	SComboBox *psComboBox = psWidget->uWidgetSpecific.psComboBox;

	// If the combo box is locked, just return immediately. We don't update on a combo box lock.
	if (FALSE == psComboBox->bLocked)
	{
		// Update the populated list items
		ComboBoxPopulateListItems(psWidget->eWidgetHandle,
								  psComboBox,
								  psComboBox->u32ItemListBase);
	}

	// Set the selected item (if any)
	if (COMBOBOX_NO_ITEM_SELECTED == psComboBox->u32ItemSelectedIndex)
	{
		eLCDErr = TextSetText(psComboBox->eSelectedText,
							  "");
	}
	else
	{
		SComboBoxItem *psItem;

		psItem = ComboBoxIndexToItem(psComboBox,
									 psComboBox->u32ItemSelectedIndex);

		// If this asserts, the ComboBox widget has an internal/algorithmic problem
		GCASSERT(psItem);

		eLCDErr = TextSetText(psComboBox->eSelectedText,
							  psItem->peIndexTag);
	}

	// This had better not assert
	GCASSERT(LERR_OK == eLCDErr);

	// See if the drop window should (or shouldn't) be visible
	WindowSetVisible(psComboBox->eListWindow,
					 psComboBox->bDroppedList);
}

static void ComboBoxErase(SWidget *psWidget)
{
	WidgetEraseStandard(psWidget);
}

static void ComboBoxSetListBaseAndSelection(SComboBox *psComboBox,
											UINT32 u32ItemListBase,
											UINT32 u32ItemListSelect)
{
	UINT32 u32Loop;
	UINT32 u32AbsoluteSelected = u32ItemListBase + u32ItemListSelect;
	UINT16 u16TextColor;
	UINT32 u32BoxColor;
	ELCDErr eLCDErr;

	for (u32Loop = 0; u32Loop < psComboBox->sOptions.u32TextLineCount; u32Loop++)
	{
		SComboBoxItem *psItem;

		// Get the item structure in question
		eLCDErr = WidgetGetUserData((WIDGETHANDLE) psComboBox->peSelections[u32Loop],
									(void **) &psItem);

		GCASSERT(LERR_OK == eLCDErr);

		if( psItem && 
			(psItem->u32SelectionIndex == u32AbsoluteSelected) &&
			(u32ItemListSelect != COMBOBOX_NO_ITEM_SELECTED) )
		{
			// Selected
			u16TextColor = psComboBox->sOptions.u32MouseoverTextColor;
			u32BoxColor = psComboBox->sOptions.u32MouseoverBoundingBoxColor;
		}
		else
		{
			// Not selected
			u16TextColor = psComboBox->sOptions.u16TextColor;
			u32BoxColor = TEXT_COLOR_NONE;
		}

		// Set the appropriate text color
		eLCDErr = TextSetColor(psComboBox->peSelections[u32Loop],
							   u16TextColor);
		GCASSERT(LERR_OK == eLCDErr);
		eLCDErr = TextSetBoundingBoxColor(psComboBox->peSelections[u32Loop],
										  u32BoxColor,
										  u32BoxColor);
		GCASSERT(LERR_OK == eLCDErr);
	}
}

static void ComboBoxTouchSelectedCallback(WIDGETHANDLE eWidgetHandle,
										  UINT32 u32Mask,
										  UWidgetCallbackData *puData)
{
	COMBOBOXHANDLE eComboBoxHandle;
	SComboBox *psComboBox;
	SWidget *psWidget;
	ELCDErr eLCDErr;

	if (puData->sPressRelease.bPress)
	{
		// Now get the parent combo box's handle
		eLCDErr = WidgetGetParent(eWidgetHandle,
								  WIDGET_TOUCH,
								  (WIDGETHANDLE *) &eComboBoxHandle);
		GCASSERT(LERR_OK == eLCDErr);

		// Get the widget specific data
		eLCDErr = WidgetGetWidgetSpecific((WIDGETHANDLE) eComboBoxHandle,
										   WIDGET_COMBOBOX,
										   (void **) &psComboBox,
										   &psWidget);
		GCASSERT(LERR_OK == eLCDErr);

		// If the list is dropped, then hide it
		if (psComboBox->bDroppedList)
		{
			psComboBox->bDroppedList = FALSE;

			// Drop the drop list
			WidgetPaint(psWidget,
						FALSE);
		}
	}
}

static void ComboBoxSliderCallback(WIDGETHANDLE eWidgetHandle,
								   UINT32 u32Mask,
								   UWidgetCallbackData *puData)
{
	ELCDErr eLCDErr;
	COMBOBOXHANDLE eComboBoxHandle;
	SComboBox *psComboBox = NULL;
	SWidget *psWidget;

	// We've got it! Look at our user data. It contains the ComboBox handle.
	eLCDErr = WidgetGetUserData((WIDGETHANDLE) eWidgetHandle,
								(void **) &eComboBoxHandle);
	GCASSERT(LERR_OK == eLCDErr);

	// Now we have our combo box handle. Go get its widget data.
	eLCDErr = WidgetGetPointerByHandle(eComboBoxHandle,
										WIDGET_COMBOBOX,
										&psWidget,
										NULL);
	GCASSERT(LERR_OK == eLCDErr);

	// Now the combo box specific data
	psComboBox = psWidget->uWidgetSpecific.psComboBox;
	GCASSERT(psComboBox);

	// puData.sSlider.s32Value contains the new base position
	psComboBox->u32ItemListBase = (UINT32) puData->sSlider.s32SliderValue;

	// Clear out the item list select
	psComboBox->u32ItemListSelect = COMBOBOX_NO_ITEM_SELECTED;

	// Go repaint the list
	ComboBoxPopulateListItems(eComboBoxHandle,
							  psComboBox,
							  psComboBox->u32ItemListBase);

	// And set the colors accordingly
	ComboBoxSetListBaseAndSelection(psComboBox,
									psComboBox->u32ItemListBase,
									psComboBox->u32ItemListSelect);
}


static void ComboBoxRecalcDropWindow( SWidget* psWidget )
{
	ELCDErr eErr;
	SWindow* psParentWindow = NULL;
	SWindow* psListWindow = NULL;
	SComboBox* psComboBox = NULL;

	return;

	GCASSERT(psWidget);

	psComboBox = psWidget->uWidgetSpecific.psComboBox;
	GCASSERT(psComboBox);

	psParentWindow = WindowGetPointer(psWidget->eParentWindow);
	GCASSERT(psParentWindow);

	// If the combobox is about to be drawn and the parent window has viewporting enabled, 
	//	recalculate the combobox's position based on the parent window's position and viewport
	// 	then reassign the locations and sizes of the droplist window and touch regions.

	if( (psParentWindow->u32ViewportXOffset != 0) || 
		(psParentWindow->u32ViewportYOffset != 0) )
	{
		UINT32 u32XSurfaceSize;
		UINT32 u32YSurfaceSize;

		INT32 s32XPos;
		INT32 s32YPos;
		UINT32 u32XSize;
		UINT32 u32YSize;

		psListWindow = WindowGetPointer(psComboBox->eListWindow);
		GCASSERT(psListWindow);

		GCASSERT(psListWindow->psWindowImageInstance);

		s32XPos = psComboBox->s32DropListXPos - psParentWindow->u32ViewportXOffset;
		s32YPos = psComboBox->s32DropListYPos - psParentWindow->u32ViewportYOffset;
		u32XSize = psListWindow->psWindowImageInstance->u32XSizeClipped;
		u32YSize = psListWindow->psWindowImageInstance->u32YSizeClipped;

		// Calculate the correct position for the combobox (parent window offset handled internally)
		eErr = WindowSetPosition(   psComboBox->eListWindow,
									s32XPos, s32YPos );
		GCASSERT( LERR_OK == eErr );

		// Now recalculate the touch region positions
		GCASSERT(psParentWindow->psWindowImageInstance);

		s32XPos += psParentWindow->psWindowImageInstance->s32XPos;
		s32YPos += psParentWindow->psWindowImageInstance->s32YPos;

		// Get the size of the surface
		{
			EGCResultCode eResult;

			eResult = GCDisplayGetXSize(&u32XSurfaceSize);
			GCASSERT(GC_OK == eResult);
			eResult = GCDisplayGetYSize(&u32YSurfaceSize);
			GCASSERT(GC_OK == eResult);
		}

		// Left side
		eErr = TouchRegionSetSize(psComboBox->eTouchHandle[0],
									0, 0,
									(UINT32) s32XPos, u32YSurfaceSize);
		GCASSERT(LERR_OK == eErr);

		// Right side
		eErr = TouchRegionSetSize(psComboBox->eTouchHandle[1],
									(s32XPos + u32XSize), 0,
									u32XSurfaceSize - (s32XPos + u32XSize), u32YSurfaceSize);
		GCASSERT(LERR_OK == eErr);

		// Top side
		eErr = TouchRegionSetSize(psComboBox->eTouchHandle[2],
									s32XPos, 0,
									u32XSize, (UINT32) s32YPos);
		GCASSERT(LERR_OK == eErr);

		// Bottom side
		eErr = TouchRegionSetSize(psComboBox->eTouchHandle[3],
									s32XPos, s32YPos + u32YSize,
									u32XSize, (UINT32) (u32YSurfaceSize - (s32YPos + u32YSize)));
		GCASSERT(LERR_OK == eErr);
	}
}


static void ComboBoxDropButtonCallback(WIDGETHANDLE eWidgetHandle,
									   UINT32 u32Mask,
									   UWidgetCallbackData *puData)
{
	// Only care about the button being pressed
	if (puData->sButton.bPress)
	{
		ELCDErr eLCDErr;
		COMBOBOXHANDLE eComboBoxHandle;
		SComboBox *psComboBox = NULL;
		SWidget *psWidget;

		// We've got it! Look at our user data. It contains the ComboBox handle.
		eLCDErr = WidgetGetUserData((WIDGETHANDLE) eWidgetHandle,
									(void **) &eComboBoxHandle);
		GCASSERT(LERR_OK == eLCDErr);

		// Now we have our combo box handle. Go get its widget data.
		eLCDErr = WidgetGetPointerByHandle(eComboBoxHandle,
										   WIDGET_COMBOBOX,
										   &psWidget,
										   NULL);
		GCASSERT(LERR_OK == eLCDErr);

		// Now the combo box specific data
		psComboBox = psWidget->uWidgetSpecific.psComboBox;
		GCASSERT(psComboBox);

		// If our drop list is visible, then make it invisible. Otherwise, make it visible
		if (psComboBox->bDroppedList)
		{
			psComboBox->bDroppedList = FALSE;
		}
		else
		{
			psComboBox->bDroppedList = TRUE;
			ComboBoxRecalcDropWindow(psWidget);
		}

		// Update the widget
		WidgetPaint(psWidget,
					FALSE);
	}
}

static SComboBoxItem *ComboBoxFindItem(SComboBox *psComboBox,
									   SComboBoxItem *psItem,
									   LEX_CHAR eKey)
{
	if (NULL == psItem)
	{
		psItem = psComboBox->psItems;
	}
	else
	{
		psItem = psItem->psNextLink;
		if (NULL == psItem)
		{
			return(NULL);
		}

		if (Lextoupper(*psItem->peIndexTag) != eKey)
		{
			psItem = psComboBox->psItems;
		}
		else
		{
			return(psItem);
		}
	}

	// Find the first item in the list
	while (psItem)
	{
		if (Lextoupper(*psItem->peIndexTag) == eKey)
		{
			return(psItem);
		}

		psItem = psItem->psNextLink;
	}

	return(NULL);
}

static UINT32 ComboBoxPreviousItem(SComboBox *psComboBox)
{
	UINT32 u32ItemListSelect = psComboBox->u32ItemListSelect;

	// If it's not the top item, just back up one
	if (u32ItemListSelect)
	{
		u32ItemListSelect--;
	}
	else
	{
		// If it is the top item, back the list up one
		if (psComboBox->u32ItemListBase)
		{
			psComboBox->u32ItemListBase--;
		}
		else
		{
			// Top of the ENTIRE list. Don't do anything.
		}
	}

	return(u32ItemListSelect);
}

static UINT32 ComboBoxNextItem(SComboBox *psComboBox)
{
	UINT32 u32ItemListSelect = psComboBox->u32ItemListSelect;

	// If we're beyond the end of the list, just return
	if ((u32ItemListSelect + psComboBox->u32ItemListBase + 1) >= psComboBox->u32ItemCount)
	{
		return(u32ItemListSelect);
	}

	if (u32ItemListSelect >= (psComboBox->sOptions.u32TextLineCount - 1))
	{
		// Need to scroll the list base down
		psComboBox->u32ItemListBase++;
	}
	else
	{
		++u32ItemListSelect;
	}

	return(u32ItemListSelect);
}

static void ComboBoxKeystroke(struct SWidget *psWidget,
							  EGCCtrlKey eGCKey,
							  LEX_CHAR eUnicode,
							  BOOL bPressed)
{
	SComboBox *psComboBox;
	SComboBoxItem *psItem;
	UINT32 u32Index = COMBOBOX_NO_ITEM_SELECTED;
	UINT32 u32ItemListSelect;
	ELCDErr eLCDErr;

	// Get the combo box structure
	psComboBox = psWidget->uWidgetSpecific.psComboBox;
	GCASSERT(psComboBox);

	// If the list isn't dropped, ignore the keystroke
	if (FALSE == psComboBox->bDroppedList)
	{
		// First give the special handler a shot at handling the keystroke
		if( psComboBox->pfKeypressCallback )
		{
			if( psComboBox->pfKeypressCallback(psWidget->eWidgetHandle, eGCKey, eUnicode) )
			{
				// The keypress was handled by the user callback.  Don't follow the normal path.
				return;
			}
		}

		// Only care if it's pressed
		if (bPressed)
		{
			// Drop the drop list
			psComboBox->bDroppedList = TRUE;

			// Drop the drop list
			WidgetPaint(psWidget, FALSE);
		}

		return;
	}

	// Get our list selection
	u32ItemListSelect = psComboBox->u32ItemListSelect;

	// Only care about actual keystrokes... except for some key modifiers
	if (eGCKey != EGCKey_Unicode)
	{
		// Only care if it's pressed
		if (bPressed)
		{
			// Handle the home key
			if (EGCKey_Home == eGCKey)
			{
				// If we're not at the top of the window, then move it there and call it good
				if (0 == u32ItemListSelect)
				{
					psComboBox->u32ItemListBase = 0;
				}
				u32ItemListSelect = 0;
				goto paintIt;
			}

			// Handle the end key
			if (EGCKey_End == eGCKey)
			{
				if ((u32ItemListSelect + 1) != psComboBox->sOptions.u32TextLineCount)
				{
					// Not at the bottom.
					u32ItemListSelect = psComboBox->sOptions.u32TextLineCount - 1;
				}
				else
				{
					// At the bottom. Move to the end of the entire list
					u32ItemListSelect = psComboBox->sOptions.u32TextLineCount - 1;
					psComboBox->u32ItemListBase = psComboBox->u32ItemCount - u32ItemListSelect - 1;
				}
				goto paintIt;
			}

			// Handle the up arrow
			if (EGCKey_Up == eGCKey)
			{
				u32ItemListSelect = ComboBoxPreviousItem(psComboBox);
				goto paintIt;
			}

			// Handle the down arrow
			if (EGCKey_Down == eGCKey)
			{
				u32ItemListSelect = ComboBoxNextItem(psComboBox);
				goto paintIt;
			}

			// Page up
			if (EGCKey_PageUp == eGCKey)
			{
				u32Index = psComboBox->u32ItemListBase + u32ItemListSelect;
				if (u32Index < psComboBox->sOptions.u32TextLineCount)
				{
					psComboBox->u32ItemListBase = 0;
					u32ItemListSelect = 0;
				}
				else
				{
					psComboBox->u32ItemListBase -= psComboBox->sOptions.u32TextLineCount;
				}

				goto paintIt;
			}

			// Page down
			if (EGCKey_PageDown == eGCKey)
			{
				u32Index = psComboBox->u32ItemListBase + psComboBox->sOptions.u32TextLineCount;
				if (u32Index >= psComboBox->u32ItemCount)
				{
					psComboBox->u32ItemListBase = psComboBox->u32ItemCount - psComboBox->sOptions.u32TextLineCount;
					u32ItemListSelect = psComboBox->sOptions.u32TextLineCount - 1;
				}
				else
				{
					psComboBox->u32ItemListBase += psComboBox->sOptions.u32TextLineCount;
				}

				goto paintIt;
			}

		}

		return;
	}

	// If they key isn't pressed, then return/ignore it
	if (FALSE == bPressed)
	{
		return;
	}

	// If it's a carriage return, let's see which text item is selected
	if( ((LEX_CHAR) UNICODE_ENTER == eUnicode) ||
		((LEX_CHAR) UNICODE_TAB == eUnicode) )
	{
		UWidgetCallbackData uData;

		eLCDErr = WidgetGetUserData(psComboBox->peSelections[psComboBox->u32ItemListSelect],
									(void **) &psItem);

		GCASSERT(LERR_OK == eLCDErr);

		// Set our new item
		psComboBox->u32ItemSelectedIndex = psItem->u32SelectionIndex;

		// Make the drop list disappear
		psComboBox->bDroppedList = FALSE;

		// Post a callback to anyone listening
		memset((void *) &uData, 0, sizeof(uData));
		uData.sComboBox.u32IndexSelected = psComboBox->u32ItemSelectedIndex;

		// Broadcast it to any potential clients
		WidgetBroadcastMask((WIDGETHANDLE) psWidget->eWidgetHandle,
							WCBK_SPECIFIC,
							&uData);

		// Now paint the widget
		WidgetPaint(psWidget,
					FALSE);
		return;
	}

	// Convert it to upper case for the list search
	eUnicode = Lextoupper(eUnicode);

	// It's an actual keystroke. Let's figure out what our current selected entry is (if any)
	psItem = ComboBoxIndexToItem(psComboBox,
									psComboBox->u32ItemListBase + u32ItemListSelect);

	if( NULL == psItem )
	{
		// No prior selection, just start at the top
		psItem = ComboBoxFindItem(psComboBox,
								  NULL,
								  eUnicode);
	}
	else
	{
		// Record our old index
		u32Index = psItem->u32SelectionIndex;

		// Find the next item in our list
		if (Lextoupper(*psItem->peIndexTag) == eUnicode)
		{
			psItem = ComboBoxFindItem(psComboBox,
									  psItem,
									  eUnicode);
		}
		else
		{
			// If the keystroke doesn't match, then start at the top and look for it
			psItem = ComboBoxFindItem(psComboBox,
									  NULL,
									  eUnicode);
		}
	}

	if (NULL == psItem)
	{
		// Not found. Just return.
		return;
	}

	// We found it! Let's see what the new index is.
	if (psItem->u32SelectionIndex == (u32Index + 1))
	{
		// Means we've moved down one. Try moving the whole list if we can, otherwise just move the line selection
		if ((psComboBox->u32ItemListBase + 1 + psComboBox->sOptions.u32TextLineCount) >= psComboBox->u32ItemCount)
		{
			++u32ItemListSelect;
		}
		else
		{
			++psComboBox->u32ItemListBase;
		}
	}
	else
	{
		// Brand new position. Let's see if we need to move the base.
		psComboBox->u32ItemListBase = psItem->u32SelectionIndex - u32ItemListSelect;
		if (u32ItemListSelect > psItem->u32SelectionIndex)
		{
			u32ItemListSelect = psItem->u32SelectionIndex;
			psComboBox->u32ItemListBase = 0;
		}

		if ((psComboBox->u32ItemListBase + psComboBox->sOptions.u32TextLineCount) >= psComboBox->u32ItemCount)
		{
			// This means it's now beyond the end of the list. Move the cursor.
			psComboBox->u32ItemListBase = psComboBox->u32ItemCount - psComboBox->sOptions.u32TextLineCount;
			u32ItemListSelect = psItem->u32SelectionIndex - psComboBox->u32ItemListBase;
		}
	}

paintIt:
	// Set the slider position
	eLCDErr = SliderSetValue(psComboBox->eSliderHandle,
							 (INT32) psComboBox->u32ItemListBase);
	GCASSERT(LERR_OK == eLCDErr);

	psComboBox->u32ItemListSelect = u32ItemListSelect;

	// Go set the text up appropriately
	ComboBoxSetListBaseAndSelection(psComboBox,
									psComboBox->u32ItemListBase,
									psComboBox->u32ItemListSelect);

	// Repaint our widget
	WidgetPaint(psWidget,
				FALSE);
}

static void ComboBoxSetFocus(SWidget *psWidget,
							 BOOL bFocusSet)
{
	ELCDErr eErr;
	UINT32 u32Loop;

	// Enable or disable all the touch regions based on the focus of the combo box
	for (u32Loop = 0; u32Loop < (sizeof(psWidget->uWidgetSpecific.psComboBox->eTouchHandle) / sizeof(psWidget->uWidgetSpecific.psComboBox->eTouchHandle[0])); u32Loop++)
	{
		eErr = WidgetSetEnable( (WIDGETHANDLE) psWidget->uWidgetSpecific.psComboBox->eTouchHandle[u32Loop], bFocusSet );
		GCASSERT(LERR_OK == eErr);
	}

}

static void ComboBoxMouseWheel(SWidget *psWidget,
							   UINT32 u32Value)
{
	SComboBox *psComboBox;
	UINT32 u32Index;
	ELCDErr eLCDErr;

	// Get the combo box structure
	psComboBox = psWidget->uWidgetSpecific.psComboBox;
	GCASSERT(psComboBox);

	if (0 == u32Value)
	{
		u32Index = ComboBoxPreviousItem(psComboBox);
	}
	else
	if (1 == u32Value)
	{
		u32Index = ComboBoxNextItem(psComboBox);
	}

	// Set the slider position
	eLCDErr = SliderSetValue(psComboBox->eSliderHandle,
							 (INT32) psComboBox->u32ItemListBase);
	GCASSERT(LERR_OK == eLCDErr);

	psComboBox->u32ItemListSelect = u32Index;

	// Go set the text up appropriately
	ComboBoxSetListBaseAndSelection(psComboBox,
									psComboBox->u32ItemListBase,
									psComboBox->u32ItemListSelect);

	// Repaint our widget
	WidgetPaint(psWidget,
				FALSE);
}

static void ComboBoxSetDisable(struct SWidget *psWidget,
							   BOOL bWidgetDisabled)
{
	SComboBox *psComboBox;

	// Get the combo box structure
	psComboBox = psWidget->uWidgetSpecific.psComboBox;
	GCASSERT(psComboBox);

	if (bWidgetDisabled)
	{
		// We are disabling the widget. 

		// Hide the drop list
		psComboBox->bDroppedList = FALSE;

		// Disable the dropped button
		(void) ButtonStateSet(psComboBox->eDropListButtonHandle,
							  BUTTON_STATE_DISABLED_NORMAL);
	}
	else
	{
		// We are reenabling the widget.

		// Enable the dropped button
		(void) ButtonStateSet(psComboBox->eDropListButtonHandle,
							  BUTTON_STATE_ENABLED_NORMAL);
	}

	WidgetPaint(psWidget,
				FALSE);
}

static SWidgetFunctions sg_sComboBoxFunctions =
{
	ComboBoxHitTest,			// Hit test
	ComboBoxPaint,				// Paint
	ComboBoxErase,				// Erase
	NULL,						// Press/button down
	NULL,						// Release/button up
	NULL,						// Mouseover
	ComboBoxSetFocus,			// Widget set focus
	ComboBoxKeystroke,			// Keystroke for us
	NULL,						// Animation tick - none for now!
	NULL,						// Calc intersection
	ComboBoxMouseWheel,			// Mouse wheel
	ComboBoxSetDisable			// Set disable
};

static SWidgetTypeMethods sg_sComboBoxMethods = 
{
	&sg_sComboBoxFunctions,
	LERR_COMBOBOX_BAD_HANDLE,		// Error when it's not the handle we're looking for
	ComboBoxWidgetAlloc,
	ComboBoxWidgetFree
};

void ComboBoxFirstTimeInit(void)
{
	DebugOut("* Initializing combo box\n");
	WidgetRegisterTypeMethods(WIDGET_COMBOBOX,
							  &sg_sComboBoxMethods);
}

// Called when the selected item's text is clicked on
static void ComboBoxTextSelectedCallback(WIDGETHANDLE eTextHandle,
										 UINT32 u32Mask,
										 UWidgetCallbackData *puData)
{
	SWidget *psWidget;
	COMBOBOXHANDLE eComboBoxHandle;
	SComboBox *psComboBox;
	ELCDErr eLCDErr;

	// If they've selected the text, then figure out which, and select it
	if (puData->sText.bPress)
	{
		// Now get the combo box's handle
		eLCDErr = WidgetGetUserData((WIDGETHANDLE) eTextHandle,
									(void **) &eComboBoxHandle);
		GCASSERT(LERR_OK == eLCDErr);

		// Get the widget specific data
		eLCDErr = WidgetGetWidgetSpecific((WIDGETHANDLE) eComboBoxHandle,
										  WIDGET_COMBOBOX,
										  (void **) &psComboBox,
										  &psWidget);
		GCASSERT(LERR_OK == eLCDErr);

		// If the widget is disabled, don't drop the list
		if (FALSE == psWidget->bWidgetEnabled)
		{
			return;
		}

		// If our drop list is visible, then make it invisible. Otherwise, make it visible
		if (psComboBox->bDroppedList)
		{
			psComboBox->bDroppedList = FALSE;
		}
		else
		{
			psComboBox->bDroppedList = TRUE;
			ComboBoxRecalcDropWindow(psWidget);
		}

		// Update the widget
		WidgetPaint(psWidget,
					FALSE);
	}
}


// Called when text is moused over
static void ComboBoxTextMouseoverCallback(WIDGETHANDLE eTextHandle,
										  UINT32 u32Mask,
										  UWidgetCallbackData *puData)
{
	COMBOBOXHANDLE eComboBoxHandle;
	ELCDErr eLCDErr;
	SComboBox *psComboBox;
	SWidget *psWidget;

	// If we're just moving around, then just return
	if (EMOUSEOVER_UNCHANGED == puData->sMouseOver.eMouseoverState)
	{
		return;
	}
	else
	// If we've transitioned to being moused over, then set its color appropriately
	if (EMOUSEOVER_ASSERTED == puData->sMouseOver.eMouseoverState)
	{
		SComboBoxItem *psItem;

		// Now get the parent combo box's handle
		eLCDErr = WidgetGetParent(eTextHandle,
								  WIDGET_TEXT,
								  (WIDGETHANDLE *) &eComboBoxHandle);
		GCASSERT(LERR_OK == eLCDErr);

		// Get the widget specific data
		eLCDErr = WidgetGetWidgetSpecific((WIDGETHANDLE) eComboBoxHandle,
										  WIDGET_COMBOBOX,
										  (void **) &psComboBox,
										  &psWidget);
		GCASSERT(LERR_OK == eLCDErr);

		// Get the item structure
		eLCDErr = WidgetGetUserData((WIDGETHANDLE) eTextHandle,
									(void **) &psItem);
		GCASSERT(LERR_OK == eLCDErr);

		// If there aren't any items in the list, don't do anything else	
		if( NULL == psItem )
		{
			return;
		}

		// Make sure it's in range
		GCASSERT((psItem->u32SelectionIndex - psComboBox->u32ItemListBase) < psComboBox->sOptions.u32TextLineCount);
		psComboBox->u32ItemListSelect = psItem->u32SelectionIndex - psComboBox->u32ItemListBase;
		
		// Set the selected item
		ComboBoxSetListBaseAndSelection(psComboBox,
										psComboBox->u32ItemListBase,
										psComboBox->u32ItemListSelect);
	}
	else
	if (EMOUSEOVER_DEASSERTED == puData->sMouseOver.eMouseoverState)
	{
		// We don't care about deassertion states
	}
	else
	{
		// Unknown state
		GCASSERT(0);
	}

}

// Called when any text item is clicked on
static void ComboBoxTextCallback(WIDGETHANDLE eTextHandle,
								 UINT32 u32Mask,
								 UWidgetCallbackData *puData)
{
	SWidget *psWidget;
	SComboBox *psComboBox;
	SComboBoxItem *psItem;
	ELCDErr eLCDErr;

	// If they've selected the text, then figure out which, and select it
	if (puData->sText.bPress)
	{
		// Now get the combo box's handle
		eLCDErr = WidgetGetUserData((WIDGETHANDLE) eTextHandle,
									(void **) &psItem);
		GCASSERT(LERR_OK == eLCDErr);

		if (psItem)
		{
			UINT32 u32OldSelection;

			eLCDErr = WidgetGetWidgetSpecific((WIDGETHANDLE) psItem->eComboBoxHandle,
											  WIDGET_COMBOBOX,
											  (void **) &psComboBox,
											  &psWidget);
			GCASSERT(LERR_OK == eLCDErr);

			u32OldSelection = psComboBox->u32ItemSelectedIndex;

			// Set the new active item
			psComboBox->u32ItemSelectedIndex = psItem->u32SelectionIndex;

			// If the new index isn't the same as the old index, time to broadcast
			if (u32OldSelection != psItem->u32SelectionIndex)
			{
				UWidgetCallbackData uData;

				memset((void *) &uData, 0, sizeof(uData));
				uData.sComboBox.u32IndexSelected = psItem->u32SelectionIndex;

				// Broadcast it to any potential clients
				WidgetBroadcastMask((WIDGETHANDLE) psItem->eComboBoxHandle,
									WCBK_SPECIFIC,
									&uData);
			}

			// Update the window
			psComboBox->bDroppedList = FALSE;

			// Go paint the window now that we've selected something
			WidgetPaint(psWidget,
						FALSE);
		}
	}
}

ELCDErr ComboBoxCreate(COMBOBOXHANDLE *peComboBoxHandle,
					   WINDOWHANDLE eWindowHandle,
					   FONTHANDLE eFontHandle,					// Font used
					   INT32 s32XPos,							// X/Y Position of combo box
					   INT32 s32YPos,
					   UINT32 u32XSize,							// X/Y Size of display box
					   UINT32 u32YSize,
					   SComboBoxOptions *psOptions,
					   BOOL bVisible)
{
	ELCDErr eLCDErr = LERR_OK;
	SWidget *psWidget = NULL;
	SComboBox *psComboBox;
	SImageGroup *psImageNormal = NULL;
	SImageGroup *psImagePressed = NULL;
	SImageGroup *psThumb = NULL;
	SImageGroup *psTrack = NULL;
	SImageGroup *psDropWindowBackground = NULL;
	UINT8 u8Loop;
	EGCResultCode eResult;
	UINT32 u32Loop;
	UINT32 u32XSurfaceSize;
	UINT32 u32YSurfaceSize;
	SWindow* psWindow = NULL;

	GCASSERT(psOptions);

	eLCDErr = WidgetAllocateHandle((WIDGETHANDLE *) peComboBoxHandle,
								   WIDGET_COMBOBOX,
								   eWindowHandle,
								   &psWidget);

	RETURN_ON_FAIL(eLCDErr);

	psWindow = WindowGetPointer(eWindowHandle);
	GCASSERT(psWindow);

	psComboBox = psWidget->uWidgetSpecific.psComboBox;
	GCASSERT(psComboBox);

	// Set up the basic widget stuff
	psWidget->s32XPos = s32XPos;
	psWidget->s32YPos = s32YPos;
	psWidget->u32XSize = u32XSize;
	psWidget->u32YSize = u32YSize;
	psWidget->bWidgetHidden = TRUE;
	psWidget->bWidgetEnabled = TRUE;
	
	// Copy the incoming combo box options into the internal structure
	memcpy((void *) &psComboBox->sOptions, (void *) psOptions, sizeof(psComboBox->sOptions));

	// Set up the combobox variables
	psComboBox->u32ItemSelectedIndex = COMBOBOX_NO_ITEM_SELECTED;
	psComboBox->u32ItemCount = 0;
	psComboBox->eFontHandle = eFontHandle;

	// Set up all handles as invalid
	psComboBox->eListWindow = HANDLE_INVALID;
	psComboBox->eSliderHandle = HANDLE_INVALID;

	for (u8Loop = 0; u8Loop < (sizeof(psComboBox->eTouchHandle) / sizeof(psComboBox->eTouchHandle[0])); u8Loop++)
	{
		psComboBox->eTouchHandle[u8Loop] = HANDLE_INVALID;
	}

	psComboBox->eDropListButtonHandle = HANDLE_INVALID;
	psComboBox->eSelectedText = HANDLE_INVALID;

	// Now the big, ugly, complicated stuff

	// Create the drop list button

	// Get the normal button
	psImageNormal = GfxLoadImageGroup(psComboBox->sOptions.peDropButtonNormalFilename,
									  &eResult);

	if (NULL == psImageNormal)
	{
		eLCDErr = (ELCDErr) (eResult + LERR_GC_ERR_BASE);
		goto notAlloc;
	}

	// Adjust the overall widget size to the button
	psWidget->u32XSize -= psImageNormal->psCurrentImage->u32XSize;

	// Get the pressed button
	psImagePressed = GfxLoadImageGroup(psComboBox->sOptions.peDropButtonPressedFilename,
									  &eResult);

	if (NULL == psImagePressed)
	{
		eLCDErr = (ELCDErr) (eResult + LERR_GC_ERR_BASE);
		goto notAlloc;
	}

	// Now create a button
	eLCDErr = ButtonCreate(eWindowHandle,
						   &psComboBox->eDropListButtonHandle,
						   (INT32) ((s32XPos + u32XSize) - psImageNormal->psCurrentImage->u32XSize),
						   s32YPos);
	GOTO_ON_FAIL(eLCDErr, notAlloc);

	// Set the normal image
	eLCDErr = ButtonSetNormalImage(psComboBox->eDropListButtonHandle,
								   psImageNormal,
								   0, 0);
	GOTO_ON_FAIL(eLCDErr, notAlloc);

	// Set the pressed image
	eLCDErr = ButtonSetPressedImage(psComboBox->eDropListButtonHandle,
								    psImagePressed,
								    0, 0);
	GOTO_ON_FAIL(eLCDErr, notAlloc);

	// Now make the button visible
	eLCDErr = WidgetSetHideByHandle(psComboBox->eDropListButtonHandle,
									WIDGET_BUTTON,
									FALSE,
									TRUE);
	GOTO_ON_FAIL(eLCDErr, notAlloc);

	// Tag the widget as a subordinate widget
	eLCDErr = WidgetSetParent(psComboBox->eDropListButtonHandle,
							  WIDGET_BUTTON,
							  *peComboBoxHandle);
	GOTO_ON_FAIL(eLCDErr, notAlloc);

	// Create a text widget for the display line (next to the drop button)
	eLCDErr = TextCreate(&psComboBox->eSelectedText,
						 eWindowHandle,
						 ROT_0);
	GOTO_ON_FAIL(eLCDErr, notAlloc);

	// Set the text's font
	eLCDErr = TextSetFont(psComboBox->eSelectedText,
						  psComboBox->eFontHandle);
	GOTO_ON_FAIL(eLCDErr, notAlloc);

	// Set the text's bounding box
	eLCDErr = TextSetBoundingBoxSize(psComboBox->eSelectedText,
									 u32XSize - psImageNormal->psCurrentImage->u32XSize,
									 u32YSize);
	GOTO_ON_FAIL(eLCDErr, notAlloc);

	// Tag the widget as a subordinate widget
	eLCDErr = WidgetSetParent(psComboBox->eSelectedText,
							  WIDGET_TEXT,
	   						  *peComboBoxHandle);
	GOTO_ON_FAIL(eLCDErr, notAlloc);

	// Now set the text's offset
	eLCDErr = TextSetTextOffset(psComboBox->eSelectedText,
								psComboBox->sOptions.u32SelectedTextXOffset,
								psComboBox->sOptions.u32SelectedTextYOffset);
	GOTO_ON_FAIL(eLCDErr, notAlloc);

	// Set the bounding box size
	eLCDErr = TextSetClipBox(psComboBox->eSelectedText,
							 (INT32) (u32XSize - psImageNormal->psCurrentImage->u32XSize),
							 u32YSize);
	GOTO_ON_FAIL(eLCDErr, notAlloc);

	// Set the text's color
	eLCDErr = TextSetColor(psComboBox->eSelectedText,
						   psComboBox->sOptions.u16TextColor);
	GOTO_ON_FAIL(eLCDErr, notAlloc);

	// Set the text's position
	eLCDErr = WidgetSetPositionByHandle(psComboBox->eSelectedText,
										WIDGET_TEXT,
										s32XPos,
										s32YPos);
	GOTO_ON_FAIL(eLCDErr, notAlloc);

	// Set the text's callback so we can click on it
	eLCDErr = WidgetRegisterCallback((WIDGETHANDLE) psComboBox->eSelectedText,
									 WCBK_SPECIFIC,
									 ComboBoxTextSelectedCallback);
	GOTO_ON_FAIL(eLCDErr, notAlloc);

	// Now make the text visible
	eLCDErr = WidgetSetHideByHandle((WIDGETHANDLE) psComboBox->eSelectedText,
									WIDGET_TEXT,
									FALSE,
									TRUE);
	GOTO_ON_FAIL(eLCDErr, notAlloc);

	// Load up the background window image
	psDropWindowBackground = GfxLoadImageGroup(psComboBox->sOptions.peDropListWindowBackgroundFilename,
											   &eResult);
	if (NULL == psDropWindowBackground)
	{
		eLCDErr = (ELCDErr) (eResult + LERR_GC_ERR_BASE);
		goto notAlloc;
	}

	// Increment a reference since we wipe it out later
	GfxIncRef(psDropWindowBackground);

	psComboBox->s32DropListXPos = s32XPos;
	psComboBox->s32DropListYPos = s32YPos + u32YSize;

	// Based on the image, let's create a window
	eLCDErr = WindowCreate(eWindowHandle,
						   psComboBox->s32DropListXPos, 
						   psComboBox->s32DropListYPos,
						   psDropWindowBackground->psCurrentImage->u32XSize,
						   psDropWindowBackground->psCurrentImage->u32YSize,
						   &psComboBox->eListWindow);
	GOTO_ON_FAIL(eLCDErr, notAlloc);

	// Now set the background image for the window
	eLCDErr = WindowSetBackgroundImage(psComboBox->eListWindow,
									   psComboBox->sOptions.peDropListWindowBackgroundFilename);
	GOTO_ON_FAIL(eLCDErr, notAlloc);

	// Make sure the drop list isn't visible
	WindowSetVisible(psComboBox->eListWindow,
					 FALSE);

	// Now set up a bunch of text widget handles
	psComboBox->peSelections = MemAlloc(sizeof(*psComboBox->peSelections) * psComboBox->sOptions.u32TextLineCount);
	if (NULL == psComboBox->peSelections)
	{
		eLCDErr = LERR_NO_MEM;
		goto notAlloc;
	}

	// Set all of the handles to invalid to start with
	for (u32Loop = 0; u32Loop < psComboBox->sOptions.u32TextLineCount; u32Loop++)
	{
		psComboBox->peSelections[u32Loop] = HANDLE_INVALID;
	}

	// Now run through each handle and allocate a text object for it
	for (u32Loop = 0; u32Loop < psComboBox->sOptions.u32TextLineCount; u32Loop++)
	{
		// Create the text object
		eLCDErr = TextCreate(&psComboBox->peSelections[u32Loop],
							 psComboBox->eListWindow,
							 ROT_0);

		GOTO_ON_FAIL(eLCDErr, notAlloc);

		// Set the font for this text object
		eLCDErr = TextSetFont(psComboBox->peSelections[u32Loop],
							  psComboBox->eFontHandle);
		GOTO_ON_FAIL(eLCDErr, notAlloc);

		// Now set the text's offset
		eLCDErr = TextSetTextOffset(psComboBox->peSelections[u32Loop],
									psComboBox->sOptions.u32ItemXOffset,
									psComboBox->sOptions.u32ItemYOffset);
		GOTO_ON_FAIL(eLCDErr, notAlloc);

		// Set the bounding box size
		eLCDErr = TextSetClipBox(psComboBox->peSelections[u32Loop],
								 psComboBox->sOptions.u32ItemXTextClip,
								 psComboBox->sOptions.u32ItemYTextClip);
		GOTO_ON_FAIL(eLCDErr, notAlloc);

		// Set the text's color
		eLCDErr = TextSetColor(psComboBox->peSelections[u32Loop],
							   psComboBox->sOptions.u16TextColor);
		GOTO_ON_FAIL(eLCDErr, notAlloc);

		// Set up bounding box
		eLCDErr = TextSetBoundingBoxSize(psComboBox->peSelections[u32Loop],
										 psComboBox->sOptions.u32ItemXBoundingBox,
										 psComboBox->sOptions.u32ItemYBoundingBox);
		GOTO_ON_FAIL(eLCDErr, notAlloc);

		// Set the text's position
		eLCDErr = WidgetSetPositionByHandle((WIDGETHANDLE) psComboBox->peSelections[u32Loop],
											WIDGET_TEXT,
											psComboBox->sOptions.u32DropListOverallXOffset,
											psComboBox->sOptions.u32DropListOverallYOffset + (psComboBox->sOptions.u32DropListYSpacing * u32Loop));
		GOTO_ON_FAIL(eLCDErr, notAlloc);

		// Set the callback up for this widget - for clicks
		eLCDErr = WidgetRegisterCallback((WIDGETHANDLE) psComboBox->peSelections[u32Loop],
										 WCBK_SPECIFIC,
										 ComboBoxTextCallback);
		GOTO_ON_FAIL(eLCDErr, notAlloc);

		// Set the callback up for mouseovers
		eLCDErr = WidgetRegisterCallback((WIDGETHANDLE) psComboBox->peSelections[u32Loop],
										 WCBK_MOUSEOVER,
										 ComboBoxTextMouseoverCallback);
		GOTO_ON_FAIL(eLCDErr, notAlloc);

		// Now make the text visible
		eLCDErr = WidgetSetHideByHandle((WIDGETHANDLE) psComboBox->peSelections[u32Loop],
										WIDGET_TEXT,
										FALSE,
										TRUE);
		GOTO_ON_FAIL(eLCDErr, notAlloc);

		eLCDErr = WidgetSetParent((WIDGETHANDLE) psComboBox->peSelections[u32Loop],
								  WIDGET_TEXT,
	   							  *peComboBoxHandle);
		GOTO_ON_FAIL(eLCDErr, notAlloc);

		eLCDErr = WidgetSetMouseoverDisable((WIDGETHANDLE) psComboBox->peSelections[u32Loop],
											FALSE);
		GOTO_ON_FAIL(eLCDErr, notAlloc);
	}

	// Now put in the list selection slider (if provided)
	eLCDErr = SliderCreate(psComboBox->eListWindow,
						   &psComboBox->eSliderHandle,
						   psComboBox->sOptions.u32XSliderPos,
						   psComboBox->sOptions.u32YSliderPos,
						   ROT_0);
	GOTO_ON_FAIL(eLCDErr, notAlloc);

	// Load up the thumb image
	psThumb = GfxLoadImageGroup(psComboBox->sOptions.peSliderThumbFilename,
								&eResult);
	if (NULL == psThumb)
	{
		eLCDErr = (ELCDErr) (eResult + LERR_GC_ERR_BASE);
		goto notAlloc;
	}

	// If there's a track graphic, then load it up
	if (psTrack)
	{
		// Load up the track image
		psTrack = GfxLoadImageGroup(psComboBox->sOptions.peSliderTrackFilename,
									&eResult);
		if (NULL == psTrack)
		{
			eLCDErr = (ELCDErr) (eResult + LERR_GC_ERR_BASE);
			goto notAlloc;
		}

		// Increment a reference since we wipe it out later
		GfxIncRef(psTrack);
	}

	// Set the track image and length
	eLCDErr = SliderSetImages(psComboBox->eSliderHandle,
							  psTrack,
							  psThumb,
							  psComboBox->sOptions.u32TrackLength,
							  FALSE);
	GOTO_ON_FAIL(eLCDErr, notAlloc);

	// Enable the slider
	eLCDErr = SliderSetEnable(psComboBox->eSliderHandle,
							  TRUE);
	GOTO_ON_FAIL(eLCDErr, notAlloc);

	// Set up a callback on the slider
	eLCDErr = WidgetRegisterCallback((WIDGETHANDLE) psComboBox->eSliderHandle,
									 WCBK_SPECIFIC,
									 ComboBoxSliderCallback);
	GOTO_ON_FAIL(eLCDErr, notAlloc);
	
	// Make it visible
	eLCDErr = WidgetSetHideByHandle((WIDGETHANDLE) psComboBox->eSliderHandle,
									WIDGET_SLIDER,
									FALSE,
									TRUE);
	GOTO_ON_FAIL(eLCDErr, notAlloc);

	// Set the user data for the selected text to be the combo box handle
	eLCDErr = WidgetSetUserData((WIDGETHANDLE) psComboBox->eSelectedText,
								(void *) *peComboBoxHandle);
	GOTO_ON_FAIL(eLCDErr, notAlloc);

	// Set the user data for the slider to be the combo box handle
	eLCDErr = WidgetSetUserData((WIDGETHANDLE) psComboBox->eSliderHandle,
								(void *) *peComboBoxHandle);
	GOTO_ON_FAIL(eLCDErr, notAlloc);

	// Set the user data for this button to be the combobox handle
	eLCDErr = WidgetSetUserData((WIDGETHANDLE) psComboBox->eDropListButtonHandle,
								(void *) *peComboBoxHandle);
	GOTO_ON_FAIL(eLCDErr, notAlloc);

	// Set the callback for the drop list button
	eLCDErr = WidgetRegisterCallback((WIDGETHANDLE) psComboBox->eDropListButtonHandle,
									 WCBK_SPECIFIC,
									 ComboBoxDropButtonCallback);
	GOTO_ON_FAIL(eLCDErr, notAlloc);

	// Add touch regions

	// Get the size of the surface
	eResult = GCDisplayGetXSize(&u32XSurfaceSize);
	GCASSERT(GC_OK == eResult);
	eResult = GCDisplayGetYSize(&u32YSurfaceSize);
	GCASSERT(GC_OK == eResult);

	// The touch regions must be offset by the window's position
	s32XPos += psWindow->psWindowImageInstance->u32XPos;
	s32YPos += psWindow->psWindowImageInstance->u32YPos;

	// Left side
	eLCDErr = TouchRegionCreate(&psComboBox->eTouchHandle[0],
								HANDLE_INVALID,
								0, 0,
								(UINT32) s32XPos,
								u32YSurfaceSize);
	GCASSERT(LERR_OK == eLCDErr);

	eLCDErr = WidgetSetEnable( (WIDGETHANDLE) psComboBox->eTouchHandle[0], FALSE );
	GCASSERT(LERR_OK == eLCDErr);


	// Right side
	eLCDErr = TouchRegionCreate(&psComboBox->eTouchHandle[1],
								HANDLE_INVALID,
								(s32XPos + u32XSize), 0,
								u32XSurfaceSize - (s32XPos + u32XSize), u32YSurfaceSize);
	GCASSERT(LERR_OK == eLCDErr);

	eLCDErr = WidgetSetEnable( (WIDGETHANDLE) psComboBox->eTouchHandle[1], FALSE );
	GCASSERT(LERR_OK == eLCDErr);

	// Top side
	eLCDErr = TouchRegionCreate(&psComboBox->eTouchHandle[2],
								HANDLE_INVALID,
								s32XPos, 0,
								u32XSize, (UINT32) s32YPos);
	GCASSERT(LERR_OK == eLCDErr);

	eLCDErr = WidgetSetEnable( (WIDGETHANDLE) psComboBox->eTouchHandle[2], FALSE );
	GCASSERT(LERR_OK == eLCDErr);

	// Bottom side
	eLCDErr = TouchRegionCreate(&psComboBox->eTouchHandle[3],
								HANDLE_INVALID,
								s32XPos, s32YPos + u32YSize + psDropWindowBackground->psCurrentImage->u32YSize,
								u32XSize, (UINT32) (u32YSurfaceSize - (s32YPos + u32YSize + psDropWindowBackground->psCurrentImage->u32YSize)));
	GCASSERT(LERR_OK == eLCDErr);

	eLCDErr = WidgetSetEnable( (WIDGETHANDLE) psComboBox->eTouchHandle[3], FALSE );
	GCASSERT(LERR_OK == eLCDErr);

	// Delete the background image
	GfxDeleteImageGroup(psDropWindowBackground);

	// Register for callbacks for any touch region
	for (u32Loop = 0; u32Loop < (sizeof(psComboBox->eTouchHandle) / sizeof(psComboBox->eTouchHandle[0])); u32Loop++)
	{
		// Set the text's callback so we can click on it
		eLCDErr = WidgetRegisterCallback((WIDGETHANDLE) psComboBox->eTouchHandle[u32Loop],
										 WCBK_PRESS_RELEASE,
										 ComboBoxTouchSelectedCallback);
		GOTO_ON_FAIL(eLCDErr, notAlloc);

		// Set our parent widget
		eLCDErr = WidgetSetParent(psComboBox->eTouchHandle[u32Loop],
								  WIDGET_TOUCH,
								  *peComboBoxHandle);
		GOTO_ON_FAIL(eLCDErr, notAlloc);
	}

	// Update the widget
	WidgetPaint(psWidget,
				FALSE);

	return(eLCDErr);

notAlloc:
	// Destroy any text handles
	if (psComboBox->peSelections)
	{
		for (u32Loop = 0; u32Loop < psComboBox->sOptions.u32TextLineCount; u32Loop++)
		{
			if (psComboBox->peSelections[u32Loop] != HANDLE_INVALID)
			{
				(void) WidgetDestroyByHandle((WIDGETHANDLE *) &psComboBox->peSelections[u32Loop],
											 WIDGET_TEXT);
				psComboBox->peSelections[u32Loop] = HANDLE_INVALID;
			}
		}

		// Now free the selections box
		GCFreeMemory(psComboBox->peSelections);
		psComboBox->peSelections = NULL;
	}

	// If the drop window is allocated, then destroy it
	if (psComboBox->eListWindow != HANDLE_INVALID)
	{
		(void) WindowDelete(psComboBox->eListWindow);
		psComboBox->eListWindow = HANDLE_INVALID;
	}

	// Deallocate display text widget if one is allocated
	if (psComboBox->eSelectedText != HANDLE_INVALID)
	{
		(void) WidgetDestroyByHandle(&psComboBox->eSelectedText,
									 WIDGET_TEXT);
	}

	// If the drop list button handle has been allocated, then just destroy the widget
	// and it'll take care of the images.
	if (HANDLE_INVALID == psComboBox->eDropListButtonHandle)
	{
		// Button hasn't been allocated, but the images might have been, and delete them if so
		if (psImageNormal)
		{
			GfxDeleteImageGroup(psImageNormal);
		}

		if (psImagePressed)
		{
			GfxDeleteImageGroup(psImagePressed);
		}
	}

	// Deallocate the handle - ignore the error code
	(void) WidgetDestroyByHandle(peComboBoxHandle,
								 WIDGET_COMBOBOX);

	return(eLCDErr);
}	

BUTTONHANDLE ComboBoxGetButtonHandle( COMBOBOXHANDLE eComboBox )
{
	ELCDErr eLCDErr = LERR_OK;
	SComboBox *psComboBox = NULL;
	SWidget *psWidget = NULL;

	eLCDErr = WidgetGetWidgetSpecific((WIDGETHANDLE) eComboBox,
									  WIDGET_COMBOBOX,
									  (void **) &psComboBox,
									  &psWidget);
	if( LERR_OK == eLCDErr )
	{
		return( psComboBox->eDropListButtonHandle );
	}

	return( HANDLE_INVALID );
}

ELCDErr ComboBoxAddItem(COMBOBOXHANDLE eComboBox,
						LEX_CHAR *peTag,
						void *pvUserData,
						UINT32 *pu32Index,
						BOOL bSort)
{
	SComboBox *psComboBox = NULL;
	SComboBoxItem *psItem;
	ELCDErr eLCDErr = LERR_OK;
	SWidget *psWidget = NULL;
	UINT32 u32Index = 0;
	SComboBoxItem *psItemPtr = NULL;

	eLCDErr = WidgetGetWidgetSpecific((WIDGETHANDLE) eComboBox,
									  WIDGET_COMBOBOX,
									  (void **) &psComboBox,
									  &psWidget);
	RETURN_ON_FAIL(eLCDErr);

	// Allocate a combo box item
	psItem = MemAlloc(sizeof(*psItem));
	if (NULL == psItem)
	{
		return(LERR_NO_MEM);
	}

	psItem->peIndexTag = Lexstrdup(peTag);
	if (NULL == psItem->peIndexTag)
	{
		GCFreeMemory(psItem);
		return(LERR_NO_MEM);
	}

	psItem->pvUserData = pvUserData;

	// Now, link it in to the master list
	u32Index = 0;
	if (NULL == psComboBox->psItems)
	{
		psComboBox->psItems = psItem;
	}
	else
	{
		SComboBoxItem *psPrior = NULL;

		psItemPtr = psComboBox->psItems;

		// Are we sorting? If so, then put it in its proper place

		if (bSort)
		{
			while (psItemPtr)
			{
				if (Lexstrcasecmp(psItem->peIndexTag, psItemPtr->peIndexTag) > 0)
				{
					break;
				}

				psPrior = psItemPtr;
				psItemPtr = psItemPtr->psNextLink;
				u32Index++;
			}
		}
		else
		{
			// Add to the end
			while (psItemPtr)
			{
				psPrior = psItemPtr;
				psItemPtr = psItemPtr->psNextLink;
				u32Index++;
			}
		}

		if (psPrior)
		{
			psItem->psNextLink = psItemPtr;
			psItem->psPriorLink = psPrior;

			psPrior->psNextLink = psItem;

			if (psItemPtr)
			{
				GCASSERT(psItemPtr->psPriorLink == psPrior);
				psItemPtr->psPriorLink = psItem;
			}
		}
		else
		{
			// Beginning of list
			psItem->psNextLink = psComboBox->psItems;
			psComboBox->psItems = psItem;

			if (psItemPtr)
			{
				psItemPtr->psPriorLink = psItem;
			}
		}
	}

	// Increment our item count
	psComboBox->u32ItemCount++;

	// Set the range of the slider widget to match the number of items

	if (psComboBox->u32ItemCount < psComboBox->sOptions.u32TextLineCount)
	{
		eLCDErr = SliderSetMinMax(psComboBox->eSliderHandle,
								  0,
								  0);
	}
	else
	{
		eLCDErr = SliderSetMinMax(psComboBox->eSliderHandle,
								  0,
								  psComboBox->u32ItemCount - psComboBox->sOptions.u32TextLineCount);
	}

	// Run through all items and set up their indexes
	psItemPtr = psComboBox->psItems;
	u32Index = 0;

	while (psItemPtr)
	{
		if (psItemPtr == psItem)
		{
			// Record the index if we care
			if (pu32Index)
			{
				*pu32Index = u32Index;
			}
		}

		psItemPtr->u32SelectionIndex = u32Index;
		u32Index++;
		psItemPtr = psItemPtr->psNextLink;
	}

	// Make sure our overall count == item count
	GCASSERT(u32Index == psComboBox->u32ItemCount);

	// Go paint the combo box if necessary
	ComboBoxPaint(psWidget,
				  FALSE);

	return(eLCDErr);
}

ELCDErr ComboBoxInsertItem(COMBOBOXHANDLE eComboBox,
						   UINT32 u32Index,
						   void *pvUserData,
						   UINT32 *pu32Index)
{
	GCASSERT(0);
	return(LERR_OK);
}

ELCDErr ComboBoxDeleteItem(COMBOBOXHANDLE eComboBox,
						   UINT32 u32Index)
{
	SComboBox *psComboBox = NULL;
	ELCDErr eLCDErr = LERR_OK;
	SWidget *psWidget = NULL;
	BOOL bListLock = FALSE;
	SComboBoxItem* psItem = NULL;
	UINT32 u32DeletedIndex = u32Index;

	eLCDErr = WidgetGetWidgetSpecific((WIDGETHANDLE) eComboBox,
									  WIDGET_COMBOBOX,
									  (void **) &psComboBox,
									  &psWidget);
	RETURN_ON_FAIL(eLCDErr);

	if( u32Index >= psComboBox->u32ItemCount )
	{
		return( LERR_COMBOBOX_INDEX_NOT_FOUND );
	}

	psItem = psComboBox->psItems;

	// Seek to the item in the list corresponding to this index
	while( psItem && u32DeletedIndex )
	{
		u32DeletedIndex--;
		psItem = psItem->psNextLink;
	}

	// If we found it, remove it
	if( psItem && (0 == u32DeletedIndex) )
	{
		psComboBox->u32ItemCount--;

		// Adjust the selected index
		if( psComboBox->u32ItemCount )
		{
			if( (psComboBox->u32ItemSelectedIndex != COMBOBOX_NO_ITEM_SELECTED) &&
				(psComboBox->u32ItemSelectedIndex >= psComboBox->u32ItemCount) )
			{
				psComboBox->u32ItemSelectedIndex = psComboBox->u32ItemCount - 1;
			}

			if( psComboBox->u32ItemListBase && 
				(psComboBox->u32ItemListBase >= psComboBox->u32ItemCount) )
			{
				psComboBox->u32ItemListBase = psComboBox->u32ItemCount - 1;
			}

			if( psComboBox->u32ItemListSelect && 
				(psComboBox->u32ItemListSelect != COMBOBOX_NO_ITEM_SELECTED) &&
				((psComboBox->u32ItemListBase + psComboBox->u32ItemListSelect) >= psComboBox->u32ItemCount) )
			{
				psComboBox->u32ItemListSelect = (psComboBox->u32ItemCount - 1) - psComboBox->u32ItemListBase;
			}
		}
		else
		{
			psComboBox->u32ItemSelectedIndex = COMBOBOX_NO_ITEM_SELECTED;
			psComboBox->u32ItemListBase = 0;
			psComboBox->u32ItemListSelect = 0;
		}

		// Mid list?
		if( psItem->psNextLink )
		{
			psItem->psNextLink->psPriorLink = psItem->psPriorLink;

			if( psItem->psPriorLink )
			{
				psItem->psPriorLink->psNextLink = psItem->psNextLink;
			}

			if( psComboBox->psItems == psItem )
			{
				psComboBox->psItems = psItem->psNextLink;
			}
		}
		// At the end of the list
		else if( psItem->psPriorLink )
		{
			psComboBox->psItems = psItem->psPriorLink;
			psItem->psPriorLink->psNextLink = psItem->psNextLink;
		}
		// Last item!
		else
		{
			psComboBox->psItems = NULL;
		}

		if( psItem->peIndexTag )
		{
			GCFreeMemory(psItem->peIndexTag);
			psItem->peIndexTag = NULL;
		}

		MemFree(psItem);
		psItem = NULL;

		ComboBoxSetListBaseAndSelection( psComboBox,
										 psComboBox->u32ItemListBase,
										 psComboBox->u32ItemListSelect );
	}

	return(LERR_OK);
}

ELCDErr ComboBoxGetItemSelected(COMBOBOXHANDLE eComboBox,
								UINT32 *pu32Index,
								LEX_CHAR **ppeTag,
								void **ppvUserData)
{
	SComboBox *psComboBox = NULL;
	SComboBoxItem *psItem;
	ELCDErr eLCDErr = LERR_OK;
	SWidget *psWidget = NULL;
	UINT32 u32Index = 0;

	eLCDErr = WidgetGetWidgetSpecific((WIDGETHANDLE) eComboBox,
									  WIDGET_COMBOBOX,
									  (void **) &psComboBox,
									  &psWidget);
	RETURN_ON_FAIL(eLCDErr);

	psItem = psComboBox->psItems;
	while (psItem)
	{
		if( psItem->u32SelectionIndex == psComboBox->u32ItemSelectedIndex )
		{
			break;
		}

		psItem = psItem->psNextLink;
		u32Index++;
	}

	if (psItem)
	{
		if (ppeTag)
		{
			*ppeTag = psItem->peIndexTag;
		}

		if (pu32Index)
		{
			*pu32Index = u32Index;
		}

		if (ppvUserData)
		{
			*ppvUserData = psItem->pvUserData;
		}

		return(LERR_OK);
	}
	else
	{
		// Index doesn't exist
		return(LERR_COMBOBOX_INDEX_NOT_FOUND);
	}
}

ELCDErr ComboBoxSetItemSelected(COMBOBOXHANDLE eComboBox,
								UINT32 u32Index)
{
	SComboBox *psComboBox = NULL;
	ELCDErr eLCDErr = LERR_OK;
	SWidget *psWidget = NULL;

	eLCDErr = WidgetGetWidgetSpecific((WIDGETHANDLE) eComboBox,
									  WIDGET_COMBOBOX,
									  (void **) &psComboBox,
									  &psWidget);
	RETURN_ON_FAIL(eLCDErr);

	// If they are setting it to no items selected, then do so
	if (COMBOBOX_NO_ITEM_SELECTED == u32Index)
	{
		psComboBox->u32ItemSelectedIndex = COMBOBOX_NO_ITEM_SELECTED;
	}
	else
	{
		SComboBoxItem *psItem;

		// Let's see if this item is in the list
		psItem = ComboBoxIndexToItem(psComboBox,
									 u32Index);

		// If this is NULL, the item doesn't exist, and return an error
		if (NULL == psItem)
		{
			return(LERR_COMBOBOX_INDEX_NOT_FOUND);
		}

		// Set the correct indexes
		psComboBox->u32ItemSelectedIndex = psItem->u32SelectionIndex;

		if( (psComboBox->u32ItemSelectedIndex + psComboBox->sOptions.u32TextLineCount) >= psComboBox->u32ItemCount )
		{
			if( psComboBox->u32ItemCount >= psComboBox->sOptions.u32TextLineCount )
			{
				psComboBox->u32ItemListBase = psComboBox->u32ItemCount - psComboBox->sOptions.u32TextLineCount;
			}
			else
			{
				psComboBox->u32ItemListBase = 0;
			}
			psComboBox->u32ItemListSelect = psComboBox->u32ItemSelectedIndex - psComboBox->u32ItemListBase;
		}
		else
		{
			psComboBox->u32ItemListBase = psComboBox->u32ItemSelectedIndex;
			psComboBox->u32ItemListSelect = 0;
		}
	}

	// Update the combobox widget (if necessary)
	ComboBoxPaint(psWidget,
				  FALSE);

	return(LERR_OK);
}

ELCDErr ComboBoxGetIndexedItem(COMBOBOXHANDLE eComboBox,
							   UINT32 u32Index,
							   LEX_CHAR **ppeTag,
							   void **ppvUserData)
{
	SComboBox *psComboBox = NULL;
	SComboBoxItem *psItem;
	ELCDErr eLCDErr = LERR_OK;
	SWidget *psWidget = NULL;

	eLCDErr = WidgetGetWidgetSpecific((WIDGETHANDLE) eComboBox,
									  WIDGET_COMBOBOX,
									  (void **) &psComboBox,
									  &psWidget);
	RETURN_ON_FAIL(eLCDErr);

	psItem = ComboBoxIndexToItem(psComboBox,
								 u32Index);

	if (psItem)
	{
		if (ppeTag)
		{
			*ppeTag = psItem->peIndexTag;
		}

		if (ppvUserData)
		{
			*ppvUserData = psItem->pvUserData;
		}

		return(LERR_OK);
	}
	else
	{
		return(LERR_COMBOBOX_INDEX_NOT_FOUND);
	}
}

ELCDErr ComboBoxSetSelectedCallback(COMBOBOXHANDLE eComboBox,
									void (*pCallback)(COMBOBOXHANDLE eComboBox,
													  UINT32 u32Index,
													  LEX_CHAR *peTag,
													  void *pvUserData))
{
	SComboBox *psComboBox = NULL;
	ELCDErr eLCDErr = LERR_OK;
	SWidget *psWidget = NULL;

	eLCDErr = WidgetGetWidgetSpecific((WIDGETHANDLE) eComboBox,
									  WIDGET_COMBOBOX,
									  (void **) &psComboBox,
									  &psWidget);
	RETURN_ON_FAIL(eLCDErr);

	// Set the callback
	psComboBox->pCallback = pCallback;

	return(LERR_OK);
}

ELCDErr ComboBoxListLock(COMBOBOXHANDLE eComboBox,
						 BOOL bLock)
{
	SComboBox *psComboBox = NULL;
	ELCDErr eLCDErr = LERR_OK;
	SWidget *psWidget = NULL;

	eLCDErr = WidgetGetWidgetSpecific((WIDGETHANDLE) eComboBox,
									  WIDGET_COMBOBOX,
									  (void **) &psComboBox,
									  &psWidget);
	RETURN_ON_FAIL(eLCDErr);

	// Set the list lock variable
	psComboBox->bLocked = bLock;

	// Update the combobox widget (if necessary)
	WidgetPaint(psWidget,
				FALSE);

	return(LERR_OK);
}

// Set a user keystroke callback that can replace the usual handling
ELCDErr ComboBoxSetUserKeystrokeCallback( COMBOBOXHANDLE eComboBoxHandle,
										  BOOL (*pfKeypressCallback)(LINEEDITHANDLE eWidgetHandle, 
																	 EGCCtrlKey eGCKey, 
																	 LEX_CHAR eUnicode) )
{
	SComboBox *psComboBox;
	ELCDErr eErr = LERR_OK;
	SWidget *psWidget;

	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eComboBoxHandle,
									WIDGET_COMBOBOX,
									&psWidget,
									NULL);

	RETURN_ON_FAIL(eErr);

	psComboBox = psWidget->uWidgetSpecific.psComboBox;
	GCASSERT(psComboBox);

	psComboBox->pfKeypressCallback = pfKeypressCallback;

	return(LERR_OK);
}


