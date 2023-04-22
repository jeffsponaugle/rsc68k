#include "Startup/app.h"
#include "Libs/Gfx/GraphicsLib.h"
#include "Libs/window/window.h"
#include "Libs/widget/widget.h"
#include "Libs/widget/LineEdit/LineEdit.h"

#define LINE_EDIT_CURSOR_BLINK_DURATION_MS			(500)
#define LINE_EDIT_CURSOR_WIDTH						(1)

#define LINE_EDIT_MOUSE_X_INVALID					(0xffffffff)

// This procedure is called when a new widget of this type is created. Here
// is where all startup-time allocation is done. If any.
static ELCDErr LineEditWidgetAlloc(SWidget *psWidget,
								   WIDGETHANDLE eHandle)
{
	SLineEdit *psLineEdit;

	GCASSERT(psWidget);
	psLineEdit = MemAlloc(sizeof(*psWidget->uWidgetSpecific.psLineEdit));
	psWidget->uWidgetSpecific.psLineEdit = psLineEdit;

	if (NULL == psLineEdit)
	{
		return(LERR_NO_MEM);
	}
	else
	{
		// Set up your defaults here, or just return if they aren't necessary.

		// Zero out the private data
		memset(psLineEdit, 0, sizeof(psLineEdit));

		psLineEdit->eFont = HANDLE_INVALID;

		psLineEdit->u16MaxLength = 0;

		psLineEdit->bCursorEnabled = FALSE;
		psLineEdit->bCursorVisible = TRUE;
		psLineEdit->u32MouseClickX = LINE_EDIT_MOUSE_X_INVALID;

		psLineEdit->u32BoxColor = TEXT_COLOR_NONE;

		psLineEdit->u16ViewportStartChar = 0;
		psLineEdit->u16ViewportLen = 0;

		psLineEdit->bIsPassword = FALSE;
		psLineEdit->bIsNumeric = FALSE;

		psLineEdit->psWidget = psWidget;

		return(LERR_OK);
	}
}


static void LineEditWidgetLock(SWidget* psWidget)
{
	EGCResultCode eGCErr;

	GCASSERT(psWidget);

	// Lock the widget parameters.  Wait forever.
	eGCErr = GCOSSemaphoreGet(psWidget->uWidgetSpecific.psLineEdit->sParameterLock, 0);
	GCASSERT(GC_OK == eGCErr);
}


static void LineEditWidgetUnlock(SWidget* psWidget)
{
	EGCResultCode eGCErr;

	GCASSERT(psWidget);

	eGCErr = GCOSSemaphorePut(psWidget->uWidgetSpecific.psLineEdit->sParameterLock);
	GCASSERT(GC_OK == eGCErr);
}


// This is called when the widget is destroyed. Here is where you clean/clear
// up/deallocate anything having to do with this widget.
static ELCDErr LineEditWidgetFree(SWidget *psWidget)
{
	SLineEdit *psLineEdit;
	ELCDErr eErr;

	GCASSERT(psWidget);
	GCASSERT(psWidget->uWidgetSpecific.psLineEdit);

	psLineEdit = psWidget->uWidgetSpecific.psLineEdit;

	// Let's see if we have any text
	if (psLineEdit->peText)
	{
		GCFreeMemory(psLineEdit->peText);
		psLineEdit->peText = NULL;
	}

	// Go delete a reference to the font
	eErr = FontFree(psLineEdit->eFont);
	GCPERFASSERT(LERR_OK == eErr);

	// Free the widget specfic data area
	GCFreeMemory(psWidget->uWidgetSpecific.psLineEdit);
	psWidget->uWidgetSpecific.psLineEdit = NULL;
	return(LERR_OK);
}

// This is a hit test check for this widget. The X/Y position on input is
// widget relative. Return TRUE if the widget has been clicked on, or FALSE
// if not.
static BOOL LineEditHitTest(SWidget *psWidget,
							UINT32 u32XPos, 
							UINT32 u32YPos)
{
	// Just return true all the time.  The widget manager only calls this if the coordinates are within the widget boundaries.
	return(TRUE);
}


// ASSUMPTION: Caller has the widget parameters locked
static void LineEditPaintCursor( SWidget *psWidget,
								 INT32 s32XPos, 
								 INT32 s32YPos )
{
	SLineEdit* psLineEdit;
	INT32 s32XSize;
	INT32 s32YSize;
	SWindow *psWindow;

	GCASSERT(psWidget);

	psLineEdit = psWidget->uWidgetSpecific.psLineEdit;

	// Only draw the cursor if it's visible
	if( psWidget->uWidgetSpecific.psLineEdit->bCursorEnabled &&
		psWidget->uWidgetSpecific.psLineEdit->bCursorVisible )
	{
		// Get the window handle
		psWindow = WindowGetPointer(psWidget->eParentWindow);
		GCASSERT(psWindow);

		// X size is a predefined number of pixels
		s32XSize = (INT32) LINE_EDIT_CURSOR_WIDTH;
		// Start with 75 percent of the area where the text will be drawn and then clip to Y extreme
		s32YSize = (INT32) ((psWidget->u32YSize - psWidget->uWidgetSpecific.psLineEdit->u32TextYOffset) * 75 / 100);

		// If the cursor is totally outside of the widget, just don't draw it
		if( (s32XPos > (INT32)(psWidget->s32XPos + psWidget->u32XSize)) ||
			((s32XPos + s32XSize) < psWidget->s32XPos) ||
			(s32YPos > (INT32)(psWidget->s32YPos + psWidget->u32YSize)) ||
			((s32YPos + s32YSize) < psWidget->s32YPos) )
		{
			return;
		}

		// Clip the size to X widget extreme
		if( (s32XSize + s32XPos) > (INT32) (psWidget->s32XPos + psWidget->u32XSize) )
		{
			s32XSize = (psWidget->s32XPos + psWidget->u32XSize) - s32XPos;
		}

		// Clip the size to X window extreme
		if( (s32XSize + s32XPos) > (INT32) (psWindow->u32ActiveAreaXSize + psWindow->u32ActiveAreaXPos) )
		{
			s32XSize = (psWindow->u32ActiveAreaXSize + psWindow->u32ActiveAreaXPos) - s32XPos;
		}

		// Clip to Y widget extreme
		if( (s32YSize + s32YPos) > (INT32) (psWidget->s32YPos + psWidget->u32YSize) )
		{
			s32XSize = (psWidget->s32YPos + psWidget->u32YSize) - s32YPos;
		}

		// Clip to Y window extreme
		if( (s32YSize + s32YPos) > (INT32) (psWindow->u32ActiveAreaYSize + psWindow->u32ActiveAreaYPos) )
		{
			s32YSize = (psWindow->u32ActiveAreaYSize + psWindow->u32ActiveAreaYPos) - s32YPos;
		}

		// If we still have a size in both axes, proceed
		if ((s32XSize > 0) && (s32YSize > 0))
		{
			UINT16 *pu16ColorFill;
			UINT32 u32YCount;
			UINT32 u32XCount;

			pu16ColorFill = psWindow->psWindowImage->pu16ImageData + s32XPos + (s32YPos * psWindow->psWindowImage->u32Pitch);
			u32YCount = (UINT32) s32YSize;
			while (u32YCount--)
			{
				u32XCount = (UINT32) s32XSize;
				while (u32XCount--)
				{
					*pu16ColorFill = ~(*pu16ColorFill);
					++pu16ColorFill;
				}

				// Next scanline
				pu16ColorFill += (psWindow->psWindowImage->u32Pitch - s32XSize);
			}
		}
	}
}


// This method is called whenever the widget manager needs to paint the widget.
// Note that it is not necessary to first *ERASE* the widget area - the widget
// manager takes care of that.
static void LineEditPaint(SWidget *psWidget,
						  BOOL bLock)
{
	SLineEdit *psLineEdit = psWidget->uWidgetSpecific.psLineEdit;
	TEXTCHAR *peTextChar;
	INT32 s32XPos;
	INT32 s32XAdvance;
	INT32 s32YPos;
	INT32 s32YAdvance;
	INT32 s32XSize;
	INT32 s32YSize;
	UINT32 u32CharCount;
	UINT32 u32CharIdx;
	UINT16 u16TextColor;

	GCASSERT(psLineEdit);
	s32XPos = psWidget->s32XPos;
	s32YPos = psWidget->s32YPos;

	// PVTODO: LOCALIZATION: Watch out for utf8 region code preceeding string data in peTextChar ????

	peTextChar = psLineEdit->peText;

	// If this text object is hidden, don't paint it
	if (psWidget->bWidgetHidden)
	{
		return;
	}

	LineEditWidgetLock(psWidget);

	// First, figure out if we need to do a bounding box draw
	if ((psLineEdit->u32BoundingBoxXSize) &&
		(psLineEdit->u32BoundingBoxYSize))
	{
		INT32 s32XSize;
		INT32 s32YSize;
		SWindow *psWindow;
		UINT32 u32BoxColor;

		// Get the window handle
		psWindow = WindowGetPointer(psWidget->eParentWindow);
		GCASSERT(psWindow);

		// Clip to X extreme
		s32XSize = (INT32) psLineEdit->u32BoundingBoxXSize;
		if ((s32XSize + s32XPos) > (INT32) (psWindow->u32ActiveAreaXSize + psWindow->u32ActiveAreaXPos))
		{
			s32XSize = (psWindow->u32ActiveAreaXSize + psWindow->u32ActiveAreaXPos) - (s32XSize + s32XPos);
		}

		// Clip to Y extreme
		s32YSize = (INT32) psLineEdit->u32BoundingBoxYSize;
		if ((s32YSize + s32YPos) > (INT32) (psWindow->u32ActiveAreaYSize + psWindow->u32ActiveAreaYPos))
		{
			s32YSize = (psWindow->u32ActiveAreaYSize + psWindow->u32ActiveAreaYPos) - (s32YSize + s32YPos);
		}

		// If we still have a size in both axes, figure out the color
		if ((s32XSize > 0) && (s32YSize > 0))
		{
			u32BoxColor = psLineEdit->u32BoxColor;

			// If there isn't a box color, then don't highlight it
			if (u32BoxColor != TEXT_COLOR_NONE)
			{
				UINT16 *pu16ColorFill;
				UINT32 u32YCount;
				UINT32 u32XCount;

				pu16ColorFill = psWindow->psWindowImage->pu16ImageData + s32XPos + (s32YPos * psWindow->psWindowImage->u32Pitch);
				u32YCount = (UINT32) s32YSize;
				while (u32YCount--)
				{
					u32XCount = (UINT32) s32XSize;
					while (u32XCount--)
					{
						*pu16ColorFill = (UINT16) u32BoxColor;
						++pu16ColorFill;
					}

					// Next scanline
					pu16ColorFill += (psWindow->psWindowImage->u32Pitch - s32XSize);
				}
			}
		}
	}

	// Figure out which color it should be
	u16TextColor = psLineEdit->u16Color;

	// Add in the textual offset
	s32XPos += (INT32) psLineEdit->u32TextXOffset;
	s32YPos += (INT32) psLineEdit->u32TextYOffset;

	// Get "size".  Not really size but is the maximum position allowed (?Strange?)
	s32XSize = psWidget->u32XSize + s32XPos - psLineEdit->u32TextXOffset;
	s32YSize = psWidget->u32YSize + s32YPos - psLineEdit->u32TextYOffset;

	// We now have text to paint. Let's go paint.
	s32XPos <<= 6;
	s32YPos <<= 6;

	// Get the length of the original string
	u32CharCount = Lexstrlen(peTextChar);

	// Sanity check the viewport parameters
	GCASSERT( u32CharCount >= psLineEdit->u16ViewportLen );
	GCASSERT( psLineEdit->u16ViewportStartChar <= u32CharCount );

	// Display only the text portion of the viewport
	u32CharIdx = psLineEdit->u16ViewportStartChar;
	u32CharCount = psLineEdit->u16ViewportLen;

	// If the mouse was clicked, we don't know where the cursor should be yet.  Put it at the end temporarily.
	if( LINE_EDIT_MOUSE_X_INVALID != psLineEdit->u32MouseClickX )
	{
		psLineEdit->u16CursorPosition = u32CharCount;
	}

	while( u32CharIdx < (psLineEdit->u16ViewportStartChar + psLineEdit->u16ViewportLen) )
	{
		ELCDErr eLCDErr;
		LEX_CHAR eCharacter;

		if( FALSE == psLineEdit->bIsPassword )
		{
			eCharacter = peTextChar[u32CharIdx];
		}
		else
		{
			eCharacter = '*';
		}

		eLCDErr = FontRender(psLineEdit->eFont,
							 psWidget->eParentWindow,
							 s32XPos,
							 s32YPos,
							 s32XSize,
							 s32YSize,
							 (UINT32) eCharacter,
							 u16TextColor,
							 TRUE,
							 &s32XAdvance,
							 &s32YAdvance,
							 FALSE,
							 ROT_0,
							 NULL,
							 FALSE,
							 TRUE);

		// The mouse was clicked, is this the right place to put the cursor?
		if( (LINE_EDIT_MOUSE_X_INVALID != psLineEdit->u32MouseClickX) &&
			((INT32)psLineEdit->u32MouseClickX < ((s32XPos + s32XAdvance)>>6)) )
		{
			psLineEdit->u16CursorPosition = u32CharIdx;
			psLineEdit->u32MouseClickX = LINE_EDIT_MOUSE_X_INVALID;
		}

		// If this is the character position matching the cursor position, draw the cursor now (before advancing s32XPos
		if( u32CharIdx == psLineEdit->u16CursorPosition )
		{
			LineEditPaintCursor( psWidget, s32XPos>>6, s32YPos>>6 );
		}

		if (LERR_OK == eLCDErr)
		{
			s32XPos += s32XAdvance;
		}

		u32CharIdx++;
	}

	// If the cursor position is for the character position following the valid string data, draw it now
	if( (u32CharIdx == psLineEdit->u16CursorPosition) ||
		(psLineEdit->u32MouseClickX != LINE_EDIT_MOUSE_X_INVALID) )
	{
		LineEditPaintCursor( psWidget, s32XPos>>6, s32YPos>>6 );
		psLineEdit->u16CursorPosition = u32CharIdx;
		psLineEdit->u32MouseClickX = LINE_EDIT_MOUSE_X_INVALID;
	}

	LineEditWidgetUnlock(psWidget);

	WindowUpdateRegion(psWidget->eParentWindow,
					   psWidget->s32XPos,
					   psWidget->s32YPos,
					   psWidget->u32XSize,
					   psWidget->u32YSize);
}

// This method is called to erase the widget. If all that's needed is for the
// widget to erase the current X/Y position and X/Y size, have it call 
// WidgetEraseStandard() and it will erase the widget as-is.
static void LineEditErase(SWidget *psWidget)
{
	WidgetEraseStandard(psWidget);
}


static UINT32 sg_u32CursorDuration = 0;


// ASSUMPTION: Caller has widget parameters locked
static void LineEditResetCursorDuration( SWidget* psWidget )
{
	GCASSERT(psWidget);

	sg_u32CursorDuration = 0;
	psWidget->uWidgetSpecific.psLineEdit->bCursorVisible = TRUE;
}


// If the X size of the string is too large, calculate the back shift necessary to end the string at the end of the box
// ASSUMPTION: Callers are already holding the line edit parameter lock
static void LineEditRecalcTextSize(SLineEdit *psLineEdit)
{
	UINT16 u16StringLength = 0;

	// Make sure the critical pointers are valid
	GCASSERT(psLineEdit);
	GCASSERT(psLineEdit->psWidget);

	// Get the current length of the string
	if( psLineEdit->peText )
	{
		u16StringLength = Lexstrlen(psLineEdit->peText);
	}

	// Set the viewport defaults ever time
	psLineEdit->u16ViewportStartChar = 0;
	psLineEdit->u16ViewportLen = u16StringLength;

	// Loop over the string, changing the start and length of the string viewport until it fits
	if( psLineEdit->peText )
	{
		UINT32 u32XSize;
		UINT32 u32YSize;
		BOOL bFront = TRUE;

		do
		{
			GCASSERT(psLineEdit->u16ViewportStartChar <= u16StringLength);

			// Get the length of the string in this instance
			// Ignore the return code - errors are OK, here.
			(void) FontGetStringSizeLen(psLineEdit->eFont,
										 &psLineEdit->peText[psLineEdit->u16ViewportStartChar],
										 psLineEdit->u16ViewportLen,
										 &u32XSize,
										 &u32YSize,
										 ROT_0,
										 TRUE);

			// Add in the text offset (if any) to get the overall X/Y size
			u32XSize += psLineEdit->u32TextXOffset;
			u32YSize += psLineEdit->u32TextYOffset;

			// If the X length is longer than the size we have available, reduce the viewport
			if( u32XSize < (psLineEdit->psWidget->u32XSize + LINE_EDIT_CURSOR_WIDTH) )
			{
				// Exit the loop.  We're good.
				break;
			}

			// Clip from either the front or the back as long as we don't get too close to the cursor
			if( bFront && 
				((psLineEdit->u16ViewportStartChar + 10) >= psLineEdit->u16CursorPosition) )
			{
				bFront = FALSE;
			}
			else if( (FALSE == bFront) && 
					 ((psLineEdit->u16ViewportStartChar + psLineEdit->u16ViewportLen - 2) <= psLineEdit->u16CursorPosition) )
			{
				bFront = TRUE;
			}

			if( bFront )
			{
				psLineEdit->u16ViewportStartChar++;
			}

			// Next time take one from the other end
			bFront = !bFront;

			psLineEdit->u16ViewportLen--;

			// A failsafe to make sure this won't run forever
			if( (psLineEdit->u16ViewportStartChar >= u16StringLength) ||
				(0 == psLineEdit->u16ViewportLen) )
			{
				break;
			}

		} while( 1 );
	}
}


// PVTODO: Need a better way to do this.  Not localized.
BOOL LineEditIsNumeric(LEX_CHAR eUnicode)
{
	if( (eUnicode >= '0') && (eUnicode <= '9') )
	{
		return(TRUE);
	}
	else if( ('.' == eUnicode) ||
			 (',' == eUnicode) ||
			 ('-' == eUnicode) )
	{
		return(TRUE);
	}

	return(FALSE);
}


// Set a user keystroke callback that can replace the usual handling
ELCDErr LineEditSetUserKeystrokeCallback( LINEEDITHANDLE eLineEditHandle,
										  BOOL (*pfKeypressCallback)(LINEEDITHANDLE eWidgetHandle, 
																	 EGCCtrlKey eGCKey, 
																	 LEX_CHAR eUnicode) )
{
	SLineEdit *psLineEdit;
	ELCDErr eErr = LERR_OK;
	SWidget *psWidget;

	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eLineEditHandle,
									WIDGET_LINE_EDIT,
									&psWidget,
									NULL);

	RETURN_ON_FAIL(eErr);

	psLineEdit = psWidget->uWidgetSpecific.psLineEdit;
	GCASSERT(psLineEdit);

	psLineEdit->pfKeypressCallback = pfKeypressCallback;

	return(LERR_OK);
}


// Called when the widget is in focus and a keystroke has been hit or released.
static void LineEditKeystroke(struct SWidget *psWidget,
							  EGCCtrlKey eGCKey,
							  LEX_CHAR eUnicode,
							  BOOL bPressed)
{
	ELCDErr eErr;
	SLineEdit* psLineEdit = psWidget->uWidgetSpecific.psLineEdit;
	BOOL bUpdate = FALSE;
	UINT32 u32TextCharLen = 0;
	UINT32 u32NewCharLen = 0;
	UINT32 u32OldCharIdx = 0;
	UINT32 u32NewCharIdx = 0;

	// Only want keydown events here
	if( FALSE == bPressed )
	{
		return;
	}

	GCASSERT(psLineEdit);

	//DebugOutFunc("Got Keystroke: %u %u\n", eGCKey, eUnicode);

	if( psLineEdit->pfKeypressCallback )
	{
		if( psLineEdit->pfKeypressCallback(psWidget->eWidgetHandle, eGCKey, eUnicode) )
		{
			// The keypress was handled by the user callback.  Don't follow the normal path.
			return;
		}
	}

	LineEditWidgetLock(psWidget);

	// Quick sanity check on the cursor position
	if( psLineEdit->peText )
	{
		GCASSERT( psLineEdit->u16CursorPosition <= Lexstrlen(psLineEdit->peText));
	}
	else
	{
		GCASSERT( 0 == psLineEdit->u16CursorPosition );
	}

	// Get the length of the current text
	if( psLineEdit->peText )
	{
		u32TextCharLen = Lexstrlen( psLineEdit->peText );
	}


	//DebugOutFunc("eGCKey: %u, eUnicode: %u\n", eGCKey, eUnicode);


	// If it's a <tab> or <enter>, ignore it.  Not supported but are commonly pressed.
	// Please forgive the magic numbers.  They should have been translated to eGCKey enums (but weren't)
	if( (EGCKey_Unicode == eGCKey) &&
		((UNICODE_TAB == eUnicode) || (UNICODE_ENTER == eUnicode)) )
	{
		// Do nothing with these
	}
	// If a unicode character, alloc a new string and insert the new character at the cursor
	else if( EGCKey_Unicode == eGCKey )
	{
		UINT16 u16Index;

		// If this widget has a character restriction, check it here
		if( psLineEdit->bIsNumeric && (FALSE == LineEditIsNumeric(eUnicode)) )
		{
			goto ignore_keypress;
		}

		if( (u32TextCharLen + 1) <= psLineEdit->u16MaxLength )
		{
			// Start with the index for u32TextCharLen - 1 because we'll copy that to the last character in the string
			//	making sure not to overwrite the null at the end
			if( u32TextCharLen )
			{
				u16Index = u32TextCharLen - 1;

				// Shift all the characters ahead to the right
				while( u16Index >= psLineEdit->u16CursorPosition )
				{
					psLineEdit->peText[u16Index+1] = psLineEdit->peText[u16Index];

					if( 0 == u16Index )
					{
						break;
					}

					u16Index--;
				}
			}

			// Place the new character
			psLineEdit->peText[psLineEdit->u16CursorPosition] = eUnicode;

			// Advance the cursor
			psLineEdit->u16CursorPosition++;
			bUpdate = TRUE;
		}
	}

	// Delete a character
	else if( (EGCKey_Backspace == eGCKey) ||
			 (EGCKey_Delete == eGCKey) )
	{
		UINT16 u16Index;
		UINT16 u16DeleteCharacter = 0xffff;

		// Delete the character before the cursor
		if( EGCKey_Backspace == eGCKey )
		{
			if( psLineEdit->u16CursorPosition )
			{
				u16DeleteCharacter = psLineEdit->u16CursorPosition - 1;
			}
		}
		// Delete the character at the cursor
		else
		{
			u16DeleteCharacter = psLineEdit->u16CursorPosition;
		}

		if( u16DeleteCharacter != 0xffff )
		{
			// Start with the index for u16DeleteCharacter - 1 because we need to overwrite that one with the next one
			u16Index = u16DeleteCharacter;

			// Shift all the characters ahead to the right (+1 to pick up the null)
			while( u16Index < u32TextCharLen + 1 )
			{
				psLineEdit->peText[u16Index] = psLineEdit->peText[u16Index + 1];
				u16Index++;
			}

			// Decrement the cursor
			if( EGCKey_Backspace == eGCKey )
			{
				psLineEdit->u16CursorPosition--;
			}

			bUpdate = TRUE;
		}
	}

	// Handle the cursor
	else if( EGCKey_Right == eGCKey )
	{
		if( psLineEdit->peText )
		{
			psLineEdit->u16CursorPosition++;
			if( psLineEdit->u16CursorPosition > Lexstrlen(psLineEdit->peText) )
			{
				psLineEdit->u16CursorPosition = Lexstrlen(psLineEdit->peText);
			}
			bUpdate = TRUE;
		}
	}
	else if( EGCKey_Left == eGCKey )
	{
		if( psLineEdit->u16CursorPosition )
		{
			psLineEdit->u16CursorPosition--;
			bUpdate = TRUE;
		}
	}
	else if( EGCKey_Home == eGCKey )
	{
		psLineEdit->u16CursorPosition = 0;
		bUpdate = TRUE;
	}
	else if( EGCKey_End == eGCKey )
	{
		if( psLineEdit->peText )
		{
			psLineEdit->u16CursorPosition = Lexstrlen(psLineEdit->peText);
			bUpdate = TRUE;
		}
	}

ignore_keypress:

	LineEditWidgetUnlock(psWidget);

	if( bUpdate )
	{
		eErr = WindowLockByHandleIfNotSubordinate(psWidget->eParentWindow, psWidget);
		GCASSERT( LERR_OK == eErr );

		WindowBlitLock(TRUE);

		// Erase the current text image
		WidgetErase(psWidget);

		// Wipe out the intersections
		WidgetEraseIntersections(psWidget);

		// Paint any intersecting widgets
		WidgetPaintIntersections(psWidget);

		LineEditWidgetLock(psWidget);

		LineEditRecalcTextSize(psLineEdit);

		LineEditWidgetUnlock(psWidget);

		// Repaint the widget
		WidgetPaint(psWidget, FALSE);

		WindowBlitLock(FALSE);

		eErr = WindowUnlockByHandleIfNotSubordinate(psWidget->eParentWindow, psWidget);
		GCASSERT( LERR_OK == eErr );

		// Commit it
		WindowUpdateRegionCommit();
	}
}

// Called when the focus is either gained or lost
static void LineEditSetFocus(SWidget *psWidget,
							 BOOL bFocusSet)
{
	ELCDErr eErr;

	// The cursor is enabled any time we have focus
	//DebugOutFunc("%s Focus\n", bFocusSet?"Got":"Lost");

	eErr = WindowLockByHandleIfNotSubordinate(psWidget->eParentWindow, psWidget);
	GCASSERT( LERR_OK == eErr );

	// Erase the current text image
	WidgetErase(psWidget);

	// Wipe out the intersections
	WidgetEraseIntersections(psWidget);

	// Paint any intersecting widgets
	WidgetPaintIntersections(psWidget);

	LineEditWidgetLock(psWidget);

	// Is the cursor enabled right now?
	psWidget->uWidgetSpecific.psLineEdit->bCursorEnabled = bFocusSet;

	if( bFocusSet )
	{
		LineEditResetCursorDuration(psWidget);

		// Got focus so add this widget to the window animation list
		eErr = WindowAnimationListAdd( psWidget->eParentWindow, psWidget );
		GCASSERT( LERR_OK == eErr );
	}
	else
	{
		// Lost focus so remove this widget to the window animation list
		WindowAnimationListDelete( psWidget->eParentWindow, psWidget );
	}

	LineEditWidgetUnlock(psWidget);

	// Repaint the widget
	WidgetPaint(psWidget, FALSE);

	eErr = WindowUnlockByHandleIfNotSubordinate(psWidget->eParentWindow, psWidget);
	GCASSERT( LERR_OK == eErr );

	// Commit it
	WindowUpdateRegionCommit();
}

// Method called when a widget is disabled or enabled
static void LineEditSetDisable(struct SWidget *psWidget,
							   BOOL bWidgetDisabled)
{
	// Whatchu staring at?
}

// Called when a mouse button press has occurred. x/y position is the offset
// within the widget. u32Mask indicates the button pressed. Bit 0=left, bit 1=middle,
// bit 2=right
static void LineEditPress(struct SWidget *psWidget,
						  UINT32 u32Mask,
						  UINT32 u32XPos,
						  UINT32 u32YPos)
{
	ELCDErr eErr;

	// Only pay attention to left mouse clicks
	if( u32Mask & 1 )
	{
		//DebugOutFunc("Got Press %u %u %u\n", u32Mask, u32XPos, u32YPos);

		// Take coordinates and set a new cursor position based on the click.
		// Odds are that the cursor position has changed.  Go ahead and repaint it now.

		eErr = WindowLockByHandleIfNotSubordinate(psWidget->eParentWindow, psWidget);
		GCASSERT( LERR_OK == eErr );

		// Erase the current text image
		WidgetErase(psWidget);

		// Wipe out the intersections
		WidgetEraseIntersections(psWidget);

		// Paint any intersecting widgets
		WidgetPaintIntersections(psWidget);

		LineEditWidgetLock(psWidget);

		psWidget->uWidgetSpecific.psLineEdit->u32MouseClickX = u32XPos;

		LineEditWidgetUnlock(psWidget);

		// Repaint the widget
		WidgetPaint(psWidget, FALSE);

		eErr = WindowUnlockByHandleIfNotSubordinate(psWidget->eParentWindow, psWidget);
		GCASSERT( LERR_OK == eErr );

		// Commit it
		WindowUpdateRegionCommit();
	}
}

// Called when a mouse button release has occurred. x/y position is the offset
// within the widget. u32Mask indicates the button released. Bit 0=left, bit 1=middle,
// bit 2=right
static void LineEditRelease(struct SWidget *psWidget,
						    UINT32 u32Mask,
						    UINT32 u32XPos,
						    UINT32 u32YPos)
{
	// Intentionally not using this API.
	//DebugOutFunc("Got Release %u %u %u\n", u32Mask, u32XPos, u32YPos);
}

// Called when a mouseover occurs. The X/Y position is widget relative.
// eMouseoverState indicates whether the mouseover has been asserted,
// unchanged (meaning moused over the widget but not asserting/deasserting),
// or deasserted
static void LineEditMouseover(struct SWidget *psWidget,
							  UINT32 u32Mask,
							  UINT32 u32XPos,
							  UINT32 u32YPos,
							  EMouseOverState eMouseoverState)
{
	// Intentionally not using this API.
	//DebugOutFunc("Got Mouseover %u %u %u %u\n", u32Mask, u32XPos, u32YPos, eMouseoverState);
}


// Called once per widget timer tick. u32TickTime is the interval (in ms)
// and is called regardless of whether or not the widget is in focus, visible,
// etc...
static void LineEditTick(struct SWidget *psWidget,
						 UINT32 u32TickTime)
{
	ELCDErr eErr;

	if( psWidget->uWidgetSpecific.psLineEdit->bCursorEnabled )
	{
		sg_u32CursorDuration += u32TickTime;

		// Repaint the widget if it has been long enough since the last transition
		if( sg_u32CursorDuration > LINE_EDIT_CURSOR_BLINK_DURATION_MS )
		{
			sg_u32CursorDuration -= LINE_EDIT_CURSOR_BLINK_DURATION_MS;

			eErr = WindowLockByHandleIfNotSubordinate(psWidget->eParentWindow, psWidget);
			GCASSERT( LERR_OK == eErr );

			WindowBlitLock(TRUE);

			// Erase the current text image
			WidgetErase(psWidget);

			// Wipe out the intersections
			WidgetEraseIntersections(psWidget);

			// Paint any intersecting widgets
			WidgetPaintIntersections(psWidget);

			LineEditWidgetLock(psWidget);

			// Flip the state of the cursor visibility.
			psWidget->uWidgetSpecific.psLineEdit->bCursorVisible = !( psWidget->uWidgetSpecific.psLineEdit->bCursorVisible );

			LineEditWidgetUnlock(psWidget);

			// Repaint the widget
			WidgetPaint(psWidget, FALSE);

			WindowBlitLock(FALSE);

			eErr = WindowUnlockByHandleIfNotSubordinate(psWidget->eParentWindow, psWidget);
			GCASSERT( LERR_OK == eErr );

			// Commit it
			WindowUpdateRegionCommit();
		}

	}
}

static SWidgetFunctions sg_sLineEditFunctions =
{
	LineEditHitTest,			// Hit test
	LineEditPaint,				// Paint
	LineEditErase,				// Erase
	LineEditPress,				// Press/button down
	NULL,						// Release/button up
	NULL,						// Mouseover
	LineEditSetFocus,			// Widget set focus
	LineEditKeystroke,			// Keystroke for us
	LineEditTick,				// Animation tick
	NULL,						// Calc intersection (ignore this - you don't need it)
	NULL,						// Mouse wheel
	LineEditSetDisable			// Set disable
};


static SWidgetTypeMethods sg_sLineEditMethods = 
{
	&sg_sLineEditFunctions,
	LERR_LINEEDIT_BAD_HANDLE,		// Error when it's not the handle we're looking for
	LineEditWidgetAlloc,
	LineEditWidgetFree
};


void LineEditFirstTimeInit(void)
{
	WidgetRegisterTypeMethods(WIDGET_LINE_EDIT,
							  &sg_sLineEditMethods);
}


ELCDErr LineEditSetTextColor(LINEEDITHANDLE eLineEditHandle,
							 UINT16 u16Color)
{
	SLineEdit *psLineEdit;
	ELCDErr eErr = LERR_OK;
	SWidget *psWidget;

	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eLineEditHandle,
									WIDGET_LINE_EDIT,
									&psWidget,
									NULL);

	RETURN_ON_FAIL(eErr);

	psLineEdit = psWidget->uWidgetSpecific.psLineEdit;
	GCASSERT(psLineEdit);

	// If the color is unchagned, then don't change it
	if (psLineEdit->u16Color == u16Color)
	{
		goto errorExit;
	}

	eErr = WindowLockByHandleIfNotSubordinate(psWidget->eParentWindow, psWidget);
	if (eErr != LERR_OK)
	{
		goto errorExit;
	}

	// Erase the current text image
	WidgetErase(psWidget);

	// Wipe out the intersections
	WidgetEraseIntersections(psWidget);

	// Paint any intersecting widgets
	WidgetPaintIntersections(psWidget);

	psLineEdit->u16Color = u16Color;

	// Repaint the widget
	WidgetPaint(psWidget, FALSE);

	eErr = WindowUnlockByHandleIfNotSubordinate(psWidget->eParentWindow, psWidget);
// PVTOOD: Is it necessary to skip the commit if the unlock fails?
//  if (eErr != LERR_OK)
//  {
//  	goto errorExit;
//  }

	// Commit it
	WindowUpdateRegionCommit();

	// Color successfully changed
errorExit:
	return(eErr);
}


// Get the text from the widget.  Will always return a valid string on success. 
// Caller must free string when finished with it.
ELCDErr LineEditGetText( LINEEDITHANDLE eLineEditHandle, 
						 LEX_CHAR** ppeNewText )
{
	SLineEdit* psLineEdit;
	LEX_CHAR* peNewText = NULL;
	UINT32 u32TextCharLen = 0;
	ELCDErr eErr;
	SWidget* psWidget;

	GCASSERT(ppeNewText);

	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eLineEditHandle,
									WIDGET_LINE_EDIT,
									&psWidget,
									NULL);

	RETURN_ON_FAIL(eErr);

	psLineEdit = psWidget->uWidgetSpecific.psLineEdit;
	GCASSERT(psLineEdit);

	LineEditWidgetLock(psWidget);

	// Get the character length
	if( psLineEdit->peText )
	{
		u32TextCharLen = Lexstrlen( psLineEdit->peText );
	}

	peNewText = MemAlloc((u32TextCharLen + 1) * sizeof(*psLineEdit->peText));
	if (NULL == peNewText)
	{
		// Out of memory
		LineEditWidgetUnlock(psWidget);
		return(LERR_NO_MEM);
	}

	if( u32TextCharLen )
	{
		// Copy widget text into the new string
		memcpy((void *) peNewText, psLineEdit->peText, (u32TextCharLen + 1) * sizeof(*psLineEdit->peText));
	}
	else
	{
		// No text present, just set the null character
		*peNewText = 0;
	}

	*ppeNewText = peNewText;
	
	LineEditWidgetUnlock(psWidget);

	return(LERR_OK);
}


ELCDErr LineEditSetMaxLength(LINEEDITHANDLE eLineEditHandle, UINT16 u16MaxLength)
{
	SLineEdit* psLineEdit;
	LEX_CHAR* peNewText = NULL;
	ELCDErr eErr;
	ELCDErr eErr2;
	SWidget* psWidget;
	UINT16 u16CopyLen;

	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eLineEditHandle,
									WIDGET_LINE_EDIT,
									&psWidget,
									NULL);

	RETURN_ON_FAIL(eErr);

	psLineEdit = psWidget->uWidgetSpecific.psLineEdit;
	GCASSERT(psLineEdit);

	if( u16MaxLength == psLineEdit->u16MaxLength )
	{
		// No change, just return
		return(LERR_OK);
	}

	// Going to need to make a new buffer
	peNewText = MemAlloc((u16MaxLength + 1) * sizeof(*peNewText));
	if (NULL == peNewText)
	{
		// Out of memory
		return(LERR_NO_MEM);
	}

	// Zero it out
	memset(peNewText, 0, (u16MaxLength + 1) * sizeof(*peNewText));

	// Decide how much of the old string to copy into the new bufffer
	if( u16MaxLength > psLineEdit->u16MaxLength )
	{
		// If the new one is larger, just copy the old data in
		u16CopyLen = psLineEdit->u16MaxLength;
	}
	else
	{
		// Smaller, just copy the amount that will fit
		u16CopyLen = u16MaxLength;
	}

	memcpy( (void *) peNewText, psLineEdit->peText, (u16CopyLen * sizeof(*peNewText)) );

	eErr = WindowLockByHandleIfNotSubordinate(psWidget->eParentWindow, psWidget);
	if (eErr != LERR_OK)
	{
		goto errorExit;
	}

	// Erase the current text image
	WidgetErase(psWidget);
	WidgetEraseIntersections(psWidget);
	WidgetPaintIntersections(psWidget);

	LineEditWidgetLock(psWidget);

	// Free the old memory
	if (psLineEdit->peText)
	{
		GCFreeMemory(psLineEdit->peText);
	}

	// Assign the new text
	psLineEdit->peText = peNewText;
	psLineEdit->u16MaxLength = u16MaxLength;

	LineEditRecalcTextSize(psLineEdit);

	LineEditWidgetUnlock(psWidget);

	eErr = WidgetCalcIntersections(psWidget);

	// Now draw it
	WidgetPaint(psWidget, FALSE);

	eErr2 = WindowUnlockByHandleIfNotSubordinate(psWidget->eParentWindow, psWidget);

	if (LERR_OK == eErr)
	{
		eErr = eErr2;
	}

	// Commit it
	WindowUpdateRegionCommit();

errorExit:
	return(eErr);
}


ELCDErr LineEditSetText(LINEEDITHANDLE eLineEditHandle,
						LEX_CHAR *peText)
{
	SLineEdit* psLineEdit;
	UINT32 u32TextCharLen = 0;
	ELCDErr eErr;
	SWidget* psWidget;

	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eLineEditHandle,
									WIDGET_LINE_EDIT,
									&psWidget,
									NULL);

	RETURN_ON_FAIL(eErr);

	psLineEdit = psWidget->uWidgetSpecific.psLineEdit;
	GCASSERT(psLineEdit);

	// If the text is the same, don't do anything
	if (peText && psLineEdit->peText)
	{
		if (Lexstrcmp(peText, psLineEdit->peText) == 0)
		{
			return(LERR_OK);
		}
	}

	// Get the character length
	u32TextCharLen = Lexstrlen( peText );

	// Zero out the internal buffer and copy in the new data
	if( psLineEdit->peText )
	{
		memset(psLineEdit->peText, 0, (psLineEdit->u16MaxLength + 1) * sizeof(*peText));
		if( peText )
		{
			Lexstrncpy(psLineEdit->peText, peText, psLineEdit->u16MaxLength);
		}
	}

	eErr = WindowLockByHandleIfNotSubordinate(psWidget->eParentWindow, psWidget);
	if (eErr != LERR_OK)
	{
		goto errorExit;
	}

	// Set the cursor position to the end of the new text
	psLineEdit->u16CursorPosition = u32TextCharLen;

	// Erase the current text image
	WidgetErase(psWidget);
	WidgetEraseIntersections(psWidget);
	WidgetPaintIntersections(psWidget);

	LineEditWidgetLock(psWidget);

	LineEditRecalcTextSize(psLineEdit);

	LineEditWidgetUnlock(psWidget);

	// Now draw it
	WidgetPaint(psWidget, FALSE);

	eErr = WindowUnlockByHandleIfNotSubordinate(psWidget->eParentWindow, psWidget);

	// Commit it
	WindowUpdateRegionCommit();

errorExit:
	return(eErr);
}


static ELCDErr LineEditWidgetUpdatePreamble(WIDGETHANDLE eWidgetHandle,
							 SWidget **ppsWidget)
{
	ELCDErr eErr;
	SWidget *psWidget;

	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eWidgetHandle,
									WIDGET_LINE_EDIT,
									&psWidget,
									NULL);

	RETURN_ON_FAIL(eErr);

	eErr = WindowLockByHandle(psWidget->eParentWindow);
	if (eErr != LERR_OK)
	{
		goto errorExit;
	}

	// If there's a widget handle, let the caller know
	if (ppsWidget)
	{
		*ppsWidget = psWidget;
	}

	// Wipe out the widget
	WidgetErase(psWidget);

	// Wipe out the intersections
	WidgetEraseIntersections(psWidget);

	// Repaint the intersections
	WidgetPaintIntersections(psWidget);

errorExit:
	return(eErr);
}


static ELCDErr LineEditWidgetUpdateFinished(WIDGETHANDLE eWidgetHandle)
{
	ELCDErr eErr;
	SWidget *psWidget;

	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eWidgetHandle,
									WIDGET_LINE_EDIT,
									&psWidget,
									NULL);

	RETURN_ON_FAIL(eErr);

	// Calculate the new intersections
	eErr = WidgetCalcIntersections(psWidget);

	// Now draw the widget
	WidgetPaint(psWidget,
				FALSE);


	if (LERR_OK == eErr)
	{
		eErr = WindowUnlockByHandle(psWidget->eParentWindow);
	}
	else
	{
		(void) WindowUnlockByHandle(psWidget->eParentWindow);
	}

	// Commit it
	WindowUpdateRegionCommit();

	return(eErr);
}


ELCDErr LineEditSetTextOffset(LINEEDITHANDLE eLineEditHandle,
								 UINT32 u32XOffset,
								 UINT32 u32YOffset)
{
	ELCDErr eErr;
	SWidget *psWidget;
	SLineEdit* psLineEdit;

	eErr = LineEditWidgetUpdatePreamble(eLineEditHandle, &psWidget);
	RETURN_ON_FAIL(eErr);

	psLineEdit = psWidget->uWidgetSpecific.psLineEdit;
	GCASSERT(psLineEdit);

	LineEditWidgetLock(psWidget);

	psLineEdit->u32TextXOffset = u32XOffset;
	psLineEdit->u32TextYOffset = u32YOffset;
	
	LineEditRecalcTextSize(psLineEdit);

	LineEditWidgetUnlock(psWidget);

	eErr = LineEditWidgetUpdateFinished( eLineEditHandle );
	return(eErr);
}


// Since the text is clipped by the widget itself, allow the filling of a bounding box around the text
ELCDErr LineEditSetBoundingBoxSize(LINEEDITHANDLE eLineEditHandle,
								   UINT32 u32XSize,
								   UINT32 u32YSize)
{
	ELCDErr eErr;
	SWidget *psWidget;
	SLineEdit *psLineEdit;

	eErr = LineEditWidgetUpdatePreamble( eLineEditHandle, &psWidget );
	RETURN_ON_FAIL(eErr);

	psLineEdit = psWidget->uWidgetSpecific.psLineEdit;
	GCASSERT(psLineEdit);

	LineEditWidgetLock(psWidget);

	psLineEdit->u32BoundingBoxXSize = u32XSize;
	psLineEdit->u32BoundingBoxYSize = u32YSize;

	LineEditRecalcTextSize(psLineEdit);

	LineEditWidgetUnlock(psWidget);

	eErr = LineEditWidgetUpdateFinished( eLineEditHandle );
	return(eErr);
}


ELCDErr LineEditSetBoundingBoxColor(LINEEDITHANDLE eLineEditHandle,
									UINT32 u32BoxColor)
{
	ELCDErr eErr;
	SWidget *psWidget;
	SLineEdit* psLineEdit;

	eErr = LineEditWidgetUpdatePreamble( eLineEditHandle, &psWidget );
	RETURN_ON_FAIL(eErr);

	psLineEdit = psWidget->uWidgetSpecific.psLineEdit;
	GCASSERT(psLineEdit);

	LineEditWidgetLock(psWidget);

	psLineEdit->u32BoxColor = u32BoxColor;

	LineEditWidgetUnlock(psWidget);

	eErr = LineEditWidgetUpdateFinished( eLineEditHandle );
	return(eErr);
}


ELCDErr LineEditSetParam(LINEEDITHANDLE eLineEditHandle,
						 ELineEditParam eParam,
						 void* pvParamData)
{
	ELCDErr eErr;
	SWidget *psWidget;
	SLineEdit* psLineEdit;

	GCASSERT(pvParamData);

	eErr = LineEditWidgetUpdatePreamble( eLineEditHandle, &psWidget );
	RETURN_ON_FAIL(eErr);

	psLineEdit = psWidget->uWidgetSpecific.psLineEdit;
	GCASSERT(psLineEdit);

	LineEditWidgetLock(psWidget);

	if( LineEdit_IsPassword == eParam )
	{
		psLineEdit->bIsPassword = *((BOOL*)pvParamData);
	}
	else if( LineEdit_IsNumeric == eParam )
	{
		psLineEdit->bIsNumeric = *((BOOL*)pvParamData);
	}

	// Didn't recognize the parameter
	else
	{
		eErr = LERR_OUT_OF_RANGE;
	}

	LineEditWidgetUnlock(psWidget);

	// Everything ok with the param, update the widget
	if( LERR_OK == eErr )
	{
		eErr = LineEditWidgetUpdateFinished( eLineEditHandle );
	}

	return(eErr);
}


ELCDErr LineEditCreate(LINEEDITHANDLE *peLineEditHandle,		// Pointer to widget handle
					   WINDOWHANDLE eWindowHandle,				// Associated window handle
					   FONTHANDLE eFontHandle,					// Font used
					   INT32 s32XPos,							// X/Y Position of widget
					   INT32 s32YPos,
					   UINT32 u32XSize,							// X/Y Size of widget
					   UINT32 u32YSize,
					   BOOL bVisible)							// TRUE=Widget visible, FALSE=Not visible (to start with)
{
	ELCDErr eLCDErr = LERR_OK;
	EGCResultCode eGCErr = GC_OK;
	SWidget *psWidget = NULL;
	SLineEdit *psLineEdit;
	SFont* psFont;

	psFont = FontGetPointer(eFontHandle);
	if (NULL == psFont)
	{
		return(LERR_FONT_BAD_HANDLE);
	}

	// First, go allocate a common handle for this. This winds up calling the
	eLCDErr = WidgetAllocateHandle((WIDGETHANDLE *) peLineEditHandle,
								   WIDGET_LINE_EDIT,
								   eWindowHandle,
								   &psWidget);
	RETURN_ON_FAIL(eLCDErr);

	// This is the line edit structure that was created in the LineEditWidgetAlloc
	// call.
	psLineEdit = psWidget->uWidgetSpecific.psLineEdit;
	GCASSERT(psLineEdit);

	// Set up the basic widget stuff
	psWidget->s32XPos = s32XPos;
	psWidget->s32YPos = s32YPos;
	psWidget->u32XSize = u32XSize;
	psWidget->u32YSize = u32YSize;
	psWidget->bWidgetEnabled = TRUE;

	// Set whether or not the widget is 
	if (bVisible)
	{
		psWidget->bWidgetHidden = FALSE;
	}
	else
	{
		psWidget->bWidgetHidden = TRUE;
	}


	// Any more init stuff should go *HERE*. If there are allocation errors where
	// the widget can't be created, then "goto notAlloc" and clean it up there.

	psLineEdit->eFont = eFontHandle;

	eLCDErr = FontIncRef(psLineEdit->eFont);
	GCASSERT(LERR_OK == eLCDErr);

	// Create a widget to protect the widget parameters
	eGCErr = GCOSSemaphoreCreate(&psLineEdit->sParameterLock, 1);
	if( eGCErr != GC_OK )
	{
		eLCDErr = LERR_LINEEDIT_NO_SEMAPHORE;
		goto notAlloc;
	}

	// Update the widget
	WidgetPaint(psWidget,
				FALSE);

	return(eLCDErr);

notAlloc:
	// Deallocate the handle - ignore the error code
	(void) WidgetDestroyByHandle(peLineEditHandle,
								 WIDGET_LINE_EDIT);

	return(eLCDErr);
}	


ELCDErr LineEditGetDimensions( LINEEDITHANDLE eLineEditHandle, 
							   INT32* ps32XStart, INT32* ps32YStart,
							   UINT32* pu32XSize, UINT32* pu32YSize )
{
	ELCDErr eErr;
	SWidget *psWidget;
	SLineEdit* psLineEdit;

	eErr = WidgetGetPointerByHandle((WIDGETHANDLE) eLineEditHandle,
									WIDGET_LINE_EDIT,
									&psWidget,
									NULL);

	RETURN_ON_FAIL(eErr);


	psLineEdit = psWidget->uWidgetSpecific.psLineEdit;
	GCASSERT( psLineEdit && psLineEdit->psWidget );

	if( ps32XStart )
	{
		*ps32XStart = psLineEdit->psWidget->s32XPos;
	}

	if( ps32YStart )
	{
		*ps32YStart = psLineEdit->psWidget->s32YPos;
	}

	if( pu32XSize )
	{
		*pu32XSize = psLineEdit->psWidget->u32XSize;
	}

	if( pu32YSize )
	{
		*pu32YSize = psLineEdit->psWidget->u32YSize;
	}

	return(LERR_OK);
}

