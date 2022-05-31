#ifndef _WIDGET_H_
#define _WIDGET_H_

typedef enum
{
	EMOUSEOVER_ASSERTED,
	EMOUSEOVER_DEASSERTED,
	EMOUSEOVER_UNCHANGED
} EMouseOverState;

// Callback for generic widgets
typedef struct SWCBKPressRelease
{
	BOOL bPress;			// TRUE=Press, FALSE=Release
	UINT32 u32ButtonMask;	// Which button did we press/release?
	UINT32 u32XPos;			// Widget relative X/Y position
	UINT32 u32YPos;
} SWCBKPressRelease;

typedef struct SWCBKMouseover
{
	EMouseOverState eMouseoverState; // TRUE=Press, FALSE=Release
	UINT32 u32ButtonMask;	// State of buttons
	UINT32 u32XPos;			// Widget relative X/Y position
	UINT32 u32YPos;
} SWCBKMouseover;

#include "Libs/widget/console/console.h"
#include "Libs/widget/button/button.h"
#include "Libs/widget/text/text.h"
#include "Libs/widget/image/image.h"
#include "Libs/widget/graph/graph.h"
#include "Libs/widget/slider/slider.h"
#include "Libs/widget/radio/radio.h"
#include "Libs/widget/checkbox/checkbox.h"
#include "Libs/widget/touch/touch.h"
#include "Libs/widget/combo/ComboBox.h"
#include "Libs/widget/LineEdit/LineEdit.h"
#include "Libs/widget/Terminal/Terminal.h"

typedef union UWidgetCallbackData
{
	// Widget generic
	SWCBKPressRelease sPressRelease;	// For press/release selection
	SWCBKMouseover sMouseOver;			// For mouseover events

	// Widget specific
	SWCBKComboBox sComboBox;			// For combo box stuff
	SWCBKButton sButton;				// For the button widget
	SWCBKText sText;					// For text stuff
	SWCBKSlider sSlider;				// For slider widget
} UWidgetCallbackData;

typedef enum
{
	WIDGET_WTF = 0,
	WIDGET_CONSOLE = 1,
	WIDGET_BUTTON = 2,
	WIDGET_IMAGE = 3,
	WIDGET_SLIDER = 4,
	WIDGET_VIDEO = 5,
	WIDGET_TEXT = 6,
	WIDGET_GRAPH = 7,
	WIDGET_RADIO = 8,
	WIDGET_CHECKBOX = 9,
	WIDGET_TOUCH = 10,
	WIDGET_WAVE = 11,
	WIDGET_COMBOBOX = 12,
	WIDGET_LINE_EDIT = 13,
	WIDGET_TERMINAL = 14,
	WIDGET_IN_PROCESS = 15,

	// Don't move this - always must come at the end
	WIDGET_MAX
} EWidgetType;

typedef struct SWidgetFunctions
{
	BOOL (*WidgetRegionTest)(struct SWidget *psWidget,
							 UINT32 u32XPos, 
							 UINT32 u32YPos);
	void (*WidgetRepaint)(struct SWidget *psWidget,
						  BOOL bLock);
	void (*WidgetErase)(struct SWidget *psWidget);
	void (*WidgetPress)(struct SWidget *psWidget,
						UINT32 u32Mask,
						UINT32 u32XPos,
						UINT32 u32YPos);
	void (*WidgetRelease)(struct SWidget *psWidget,
						  UINT32 u32Mask,
						  UINT32 u32XPos,
						  UINT32 u32YPos);
	void (*WidgetMouseover)(struct SWidget *psWidget,
							UINT32 u32Mask,
							UINT32 u32XPos,
							UINT32 u32YPos,
							EMouseOverState eMouseoverState);
	void (*WidgetSetFocus)(struct SWidget *psWidget,
						   BOOL bFocusGained);
	void (*WidgetKeypress)(struct SWidget *psWidget,
						   EGCCtrlKey eGCKey,
						   LEX_CHAR eUnicode,
						   BOOL bPressed);
	void (*WidgetAnimationTick)(struct SWidget *psWidget,
								UINT32 u32TickTime);
	void (*WidgetCalcIntersection)(struct SWidget *psWidget,
								   INT32 *ps32XPos,
								   INT32 *ps32YPos,
								   UINT32 *pu32XSize,
								   UINT32 *pu32YSize);
	void (*WidgetMousewheel)(struct SWidget *psWidget,
							 UINT32 u32Value);
	void (*WidgetSetDisable)(struct SWidget *psWidget,
							 BOOL bWidgetDisabled);
} SWidgetFunctions;

struct SWidgetIntersection;

typedef struct SWidgetIntersection
{
	struct SWidget *psWidget;
	struct SWidgetIntersection *psNextLink;
} SWidgetIntersection;

typedef struct SWidgetTypeMethods
{
	SWidgetFunctions *psWidgetFunctions;
	ELCDErr eBadHandleError;
	ELCDErr (*WidgetTypeAlloc)(struct SWidget *psWidget,
							   WIDGETHANDLE eWidgetHandle);
	ELCDErr (*WidgetTypeFree)(struct SWidget *psWidget);
} SWidgetTypeMethods;

typedef struct SWidgetCallbacks
{
	UINT32 u32Mask;
	void (*pCallback)(WIDGETHANDLE eWidget,
					  UINT32 u32Mask,
					  UWidgetCallbackData *puWidgetCallbackData);
	struct SWidgetCallbacks *psNextLink;
} SWidgetCallbacks;

typedef struct SWidget
{
	WIDGETHANDLE eWidgetHandle;		// This widget's handle
	WIDGETHANDLE eParentWidget;		// Who is this widget's parent?
	EWidgetType eWidgetType;		// Which widget is this?
	BOOL bWidgetEnabled;			// Is this widget enabled?
	BOOL bWidgetHidden;				// Is this widget hidden?
	BOOL bRedrawInProgress;			// Set TRUE if we are already redrawing this widget
	BOOL bIgnoreIntersections;		// Set TRUE if intersections should be ignored
	BOOL bCurrentlyPressed;			// Someone holding their finger on this?
	BOOL bSelectedSinceLastRead;	// Was this widget selected since we last read it?
	BOOL bMousedOver;				// Currently moused-over?
	BOOL bMouseOverDisabled;		// Is mouseover functionality disabled?
	BOOL bInFocus;					// Is widget currently in focus?
	WINDOWHANDLE eParentWindow;		// Our parent window
	INT32 s32XPos;					// X/Y Position in window of widget
	INT32 s32YPos;
	UINT32 u32XSize;				// X/Y Size of widget's bounding box
	UINT32 u32YSize;
	SWidgetFunctions *psWidgetFunc;	// Pointer to this widget's functions
	char *pu8WidgetName;			// Widget's name (if any)
	void *pvUserData;				// User defined data

	SWidgetIntersection *psIntersectionList; // Pointer to other widgets that intersect with this widget
	SWidgetCallbacks *psCallbacks;	// Widget callbacks

	union
	{
		SConsole *psConsole;		// Pointer to console widget
		SButton *psButton;			// Pointer to button widget
		SText *psText;				// Pointer to text widget
		SImageWidget *psImageWidget;// Pointer to image widget
		SGraphWidget *psGraphWidget;// Pointer to graph widget
		SSlider *psSlider;			// Pointer to slider widget
		SRadio *psRadio;			// Pointer to radio button widget
		SCheckbox *psCheckbox;		// Pointer to checkbox
		STouch *psTouch;			// Pointer to touch widget structure
		SComboBox *psComboBox;		// Pointer to combo box structure
		SLineEdit *psLineEdit;		// Pointer to line edit structure
		STerminal *psTerminal;		// Pointer to terminal structure
	} uWidgetSpecific;

	struct SWidget *psNextLink;		// Next widget in this list
} SWidget;

#define	BUCKET_ITEM_BIT_SIZE		5			// 2^5 positions in this bucket
#define	BUCKET_TYPE_POS				24			// Position for the bucket 

typedef struct SWidgetBucket
{
	UINT8 u8ActiveWidgetCount;					// # Of widgets in this bucket
	UINT32 u32HandlePosition;					// Latest handle position
	SWidget *psWidget[1 << BUCKET_ITEM_BIT_SIZE];
	struct SWidgetBucket *psNextLink;
} SWidgetBucket;

extern void WidgetInit(void);
extern ELCDErr WidgetAllocate(SWidget **ppsWidget);
extern ELCDErr WidgetDelete(SWidget *psWidget);
extern void WidgetUpdatePointer(SWidget *psWidgetHead,
								UINT32 u32ButtonMask,
								UINT32 u32XPointerPos,
								UINT32 u32YPointerPos,
								UINT32 u32XPos,
								UINT32 u32YPos);
extern void WidgetButtonPressed(SWidget *psWidgetHead,
								UINT32 u32ButtonMask,
								UINT32 u32XPointerPos,
								UINT32 u32YPointerPos,
								UINT32 u32XPos,
								UINT32 u32YPos);
extern void WidgetButtonReleased(SWidget *psWidgetHead,
								 UINT32 u32ButtonMask,
								 UINT32 u32XPointerPos,
								 UINT32 u32YPointerPos,
								 UINT32 u32XPos,
								 UINT32 u32YPos);
extern ELCDErr WidgetSetHide(SWidget *psWidget,
							 BOOL bWidgetHidden,
							 BOOL bForceRepaint);
extern void WidgetErase(SWidget *psWidget);
extern void WidgetPaint(SWidget *psWidget,
						BOOL bLock);
extern ELCDErr WidgetSetSize(SWidget *psWidget,
							 UINT32 u32XSize,
							 UINT32 u32YSize,
							 BOOL bWindowLock,
							 BOOL bForceRepaint,
							 BOOL bRepaintAfterSizing);
extern ELCDErr WidgetCalcIntersections(SWidget *psWidget);
extern void WidgetEraseIntersections(SWidget *psWidget);
extern void WidgetPaintIntersections(SWidget *psWidget);
extern UINT32 WidgetGetAnimationStepTime(void);
extern ELCDErr BoxRender(UINT32 u32XSize,
						 UINT32 u32YSize,
						 UINT32 u32XPos,
						 UINT32 u32YPos,
						 UINT32 u32BoxColor,
						 SImage **ppsImage,
						 BOOL bDown);
extern void WidgetSoundPlay(SOUNDHANDLE eSoundHandle);
extern void WidgetSetUpdate(BOOL bUpdating);
extern ELCDErr CircleRender(UINT32 u32XPos,
							UINT32 u32YPos,
							UINT32 u32Radius,
							UINT32 u32EdgeColor1,
							UINT32 u32EdgeColor2,
							UINT32 u32FillColor,
							SImage *psImage,
							UINT8 u8BaseIndex,
							BOOL bFill);
extern void WidgetEraseStandard(SWidget *psWidget);
extern ELCDErr WidgetAllocateHandle(WIDGETHANDLE *peWidgetHandle,
									EWidgetType eWidgetType,
									WINDOWHANDLE eWindowHandle,
									SWidget **ppsWidget);
extern ELCDErr WidgetDeallocateHandle(WIDGETHANDLE *peWidgetHandle);
extern void WidgetRegisterTypeMethods(EWidgetType eType,
									  SWidgetTypeMethods *psTypeMethod);

// Widget handle based stuff
extern ELCDErr WidgetSetHideByHandle(WIDGETHANDLE eHandle,
									 EWidgetType eExpectedType,
									 BOOL bWidgetHidden,
									 BOOL bForceRepaint);
extern EWidgetType WidgetGetHandleType(WIDGETHANDLE eHandle);
extern ELCDErr WidgetGetPointerByHandle(WIDGETHANDLE eHandle,
										EWidgetType eExpectedType,
										SWidget **ppsWidget,
										SWidgetBucket **ppsBucket);
extern ELCDErr WidgetSetPositionByHandle(WIDGETHANDLE eWidgetHandle,
										 EWidgetType eExpectedType,
										 INT32 s32XPos,
										 INT32 s32YPos);
extern ELCDErr WidgetDestroyByHandle(WIDGETHANDLE *peWidgetHandle,
									 EWidgetType eExpectedType);
extern ELCDErr WidgetSetSizeByHandle(WIDGETHANDLE eWidgetHandle,
									 UINT32 u32XSize,
									 UINT32 u32YSize,
									 BOOL bWindowLock,
									 BOOL bForceRepaint);
extern ELCDErr WidgetIsVisible(WIDGETHANDLE eWidgetHandle,
							   BOOL *pbVisible);
extern ELCDErr WidgetHit(WIDGETHANDLE eWidgetHandle,
						 BOOL *pbCurrentlyPressed,
						 BOOL *pbSelectedSinceLastRead);
extern void WidgetPoolDestroy(void);
extern ELCDErr WidgetGetImageGroupByWidget(SVar *psWidgetHandle,
										   SImageGroup **ppsImageGroup);
extern void WidgetRemoveFromWidgetsWithoutWindowsList(SWidget *psWidget);
extern void	WidgetListReleaseAllWidgets(SWidget *psWidget,
										UINT32 u32Mask,
										UINT32 u32XPos,
										UINT32 u32YPos);
extern void WidgetDefocusAll( void );
extern void WidgetShutdown(void);
extern void WidgetKeyHit(EGCCtrlKey eGCKey, LEX_CHAR eKey, BOOL bPressed);
extern ELCDErr WidgetSetName(WIDGETHANDLE eWidgetHandle,
							 char *pu8WidgetName);
extern ELCDErr WidgetGetName(WIDGETHANDLE eWidgetHandle,
							 char **ppu8WidgetName);
extern ELCDErr WidgetSetUserData(WIDGETHANDLE eWidgetHandle,
								 void *pvUserData);
extern ELCDErr WidgetGetUserData(WIDGETHANDLE eWidgetHandle,
								 void **ppvUserData);
extern ELCDErr WidgetUpdatePreamble(WIDGETHANDLE eWidgetHandle,
									SWidget **ppsWidget);
extern ELCDErr WidgetUpdateFinished(WIDGETHANDLE eWidgetHandle);
extern ELCDErr WidgetMousedOver(WIDGETHANDLE eWidgetHandle,
								BOOL *pbMousedOver);
extern ELCDErr WidgetGetWidgetSpecific(WIDGETHANDLE eWidgetHandle,
									   EWidgetType eType,
									   void **ppvWidgetSpecific,
									   SWidget **ppsWidget);
extern ELCDErr WidgetSetParent(WIDGETHANDLE eWidgetHandle,
							   EWidgetType eType,
							   WIDGETHANDLE eParentWidget);
extern ELCDErr WidgetGetParent(WIDGETHANDLE eWidgetHandle,
							   EWidgetType eType,
							   WIDGETHANDLE *peParentWidget);
extern ELCDErr WidgetRegisterCallback(WIDGETHANDLE eWidgetHandle,
									  UINT32 u32WidgetMask,
									  void (*Callback)(WIDGETHANDLE eWidgetHandle,
													   UINT32 u32WidgetMask,
													   UWidgetCallbackData *puWidgetSpecificData));
extern ELCDErr WidgetUnregisterCallback(WIDGETHANDLE eWidgetHandle,
										UINT32 u32WidgetMask,
										void (*Callback)(WIDGETHANDLE eWidgetHandle,
														 UINT32 u32WidgetMask,
														 UWidgetCallbackData *puWidgetSpecificData));
extern void WidgetBroadcastMask(WIDGETHANDLE eWidgetHandle,
								UINT32 u32Mask,
								UWidgetCallbackData *puWidgetSpecificData);
extern ELCDErr WidgetSetEnable(WIDGETHANDLE eWidgetHandle,
							   BOOL bEnabled);
extern void WidgetReleaseAllFocus(SWidget *psWidget);
extern void WidgetKeypress(EGCCtrlKey eGCKey,
						   LEX_CHAR eUnicode,
						   BOOL bKeyDown);
extern void WidgetMousewheel(UINT32 u32Value);
extern ELCDErr WidgetIsMouseover(WIDGETHANDLE eWidgetHandle,
								 BOOL *pbMouseover);
extern ELCDErr WidgetSetMouseoverDisable(WIDGETHANDLE eWidgetHandle,
										 BOOL bMouseoverDisabled);
extern ELCDErr WidgetVirtualClick( WIDGETHANDLE eHandle );
extern ELCDErr WidgetVirtualActivate( WIDGETHANDLE eHandle );


// Registration events
#define	WCBK_PRESS_RELEASE		0x00000001		// If widget pressed or released (not qualified - generic)
#define WCBK_MOUSEOVER			0x00000002		// If widget is moused over (or under - not qualified - generic)
#define WCBK_SPECIFIC			0x80000000		// Widget specific callback

// Used for box/button creation
#define	BORDER_OUTER		CONVERT_24RGB_16RGB(0x404040)
#define	BORDER_INNER		CONVERT_24RGB_16RGB(0x808080)
#define	BORDER_BRIGHTEDGE	CONVERT_24RGB_16RGB(0xffffff)

#endif