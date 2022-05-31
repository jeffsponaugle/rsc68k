#ifndef _APP_H_
#define _APP_H_

#ifdef IARARM
//#pragma warning(disable : 4068)
#endif // #ifdef IARARM

#include <stdio.h>
#include <time.h>

// Fixed point APIs

typedef enum
{
	GC_OK = 0,
	GC_RESOLUTION_NOT_SUPPORTED,
	GC_BIT_DEPTH_NOT_SUPPORTED,
	GC_MONITOR_TYPE_UNKNOWN,
	GC_VIDEO_MODE_NOT_SET,
	GC_NO_INT_SLOTS_AVAILABLE,
	GC_NO_TIMER_SLOTS_AVAILABLE,
	GC_OUT_OF_MEMORY,
	GC_INVALID_MEMORY_ALLOC_SIZE,
	GC_NO_FREE_BLOCKS_LARGE_ENOUGH,
	GC_BLOCK_NOT_IN_HEAP,
	GC_SAMPLE_RATE_NOT_SUPPORTED,
	GC_OUT_OF_BIOS_MEMORY,
	GC_SOUND_NOT_INITIALIZED,
	GC_ZIP_FILE_NOT_FOUND,
	GC_FILE_WITHIN_ZIP_NOT_FOUND,
	GC_ZIP_FILE_INTEGRITY_ERROR,
	GC_ZIP_ITEM_OPEN_ERROR,
	GC_NO_DATA,
	GC_ERASE_FAILURE,
	GC_PROGRAM_ERROR,
	GC_NO_MODE_SET,
	GC_ILLEGAL_ADDRESS,
	GC_OUT_OF_RANGE,
	GC_DRIVE_NOT_FOUND,
	GC_UNKNOWN_IMAGE_TYPE,
	GC_NOT_SUPPORTED,
	GC_NOT_INITIALIZED,
	GC_EVENT_NOT_FOUND,
	GC_NO_FILESYSTEM,
	GC_SHUTDOWN,

	// SD/MMC card related errors
	GC_CARD_TIMEOUT=0x100,
	GC_RESPONSE_TIMEOUT,
	GC_CARD_ERROR,
	GC_MMC_ALWAYS_BUSY,
	GC_NO_CARD,
	GC_DEVICE_NOT_AVAILABLE,
	GC_MEDIUM_CHANGE,

	// Controller related
	GC_TRACKBALL_OUT_OF_RANGE=0x200,
	GC_IR_NOT_PRESENT,
	GC_DIAG_NOT_ACTIVE,

	// Misc
	GC_COUNTER_OUT_OF_RANGE=0x300,
	GC_PRODUCT_ID_NOT_SET,
	GC_INVALID_PRODUCT_ID,
	GC_SERIAL_PORT_OUT_OF_RANGE,
	GC_NO_ROOM_IN_XMIT_BUFFER,
	GC_BUS_LOCKED,
	GC_BUS_NOT_LOCKED,
	GC_BAD_BAUD_RATE,
	GC_RTC_NOT_AVAILABLE,
	GC_SERIAL_BAUD_RATE_INVALID,
	GC_SERIAL_FLOW_CONTROL_INVALID,
	GC_SERIAL_DATA_BITS_INVALID,
	GC_SERIAL_STOP_BITS_INVALID,
	GC_SERIAL_PARITY_INVALID,
	GC_SERIAL_NO_DEBUG_PORT,
	GC_SERIAL_PORT_ALREADY_OPEN,
	GC_SERIAL_PORT_NOT_OPEN,
	GC_SERIAL_PORT_OPEN,
	GC_SERIAL_PORT_HAL_ERROR,
	GC_SERIAL_PORT_TRUNCATED_WRITE,
	GC_ZLIB_ERROR,
	GC_I2C_BUS_OUT_OF_RANGE,
	GC_I2C_SLAVE_NAK,
	GC_I2C_TRANSACTION_TRUNCATED,
	GC_I2C_STOP_FAULT,
	GC_I2C_BUS_IDLE,
	GC_I2C_BUS_FAULT,

	// Operating system result codes
	GC_ERR_EVENT_TYPE=0x400,
	GC_ERR_PENDING_ISR,
	GC_ERR_POST_NULL_PTR,
	GC_ERR_PEVENT_NULL,
	GC_ERR_POST_ISR,
	GC_ERR_QUERY_ISR,
	GC_ERR_INVALID_OPT,
	GC_ERR_TASK_WAITING,
	GC_TIMEOUT,
	GC_TASK_NOT_EXIST,
	GC_MBOX_FULL,
	GC_Q_FULL,
	GC_PRIO_EXIST,
	GC_PRIO_ERR,
	GC_PRIO_INVALID,
	GC_SEM_OVF,
	GC_TASK_DEL_ERR,
	GC_TASK_DEL_IDLE,
	GC_TASK_DEL_REQ,
	GC_TASK_DEL_ISR,
	GC_NO_MORE_TCB,
	GC_TIME_NOT_DLY,
	GC_TIME_INVALID_MINUTES,
	GC_TIME_INVALID_SECONDS,
	GC_TIME_INVALID_MILLI,
	GC_TIME_ZERO_DLY,
	GC_TASK_SUSPEND_PRIO,
	GC_TASK_SUSPEND_IDLE,
	GC_TASK_RESUME_PRIO,
	GC_TASK_NOT_SUSPENDED,
	GC_MEM_INVALID_PART,
	GC_MEM_INVALID_BLKS,
	GC_MEM_INVALID_SIZE,
	GC_MEM_NO_FREE_BLKS,
	GC_MEM_FULL,
	GC_MEM_INVALID_PBLK,
	GC_MEM_INVALID_PMEM,
	GC_MEM_INVALID_PDATA,
	GC_MEM_INVALID_ADDR,
	GC_ERR_NOT_MUTEX_OWNER,
	GC_TASK_OPT_ERR,
	GC_ERR_DEL_ISR,
	GC_ERR_CREATE_ISR,
	GC_FLAG_INVALID_PGRP,
	GC_FLAG_ERR_WAIT_TYPE,
	GC_FLAG_ERR_NOT_RDY,
	GC_FLAG_INVALID_OPT,
	GC_FLAG_GRP_DEPLETED,
	GC_OS_ERROR_UNKNOWN,
	GC_OS_NOT_RUNNING,
	GC_OUT_OF_SEMAPHORES,
	GC_OUT_OF_QUEUES,
	GC_MUTEX_LOCKED,
	GC_MUTEX_UNLOCKED,
	GC_ERR_PEND_LOCKED,
	GC_QUEUE_EMTPY,

	// Pointer related problems
	GC_POINTER_NOT_AVAILABLE=0x500,
	GC_POINTER_NOT_SUPPORTED,
	GC_POINTER_NOT_CALIBRATED,
	GC_POINTER_CALIBRATION_IN_PROGRESS,

	// Unimplemented function
	GC_NOT_IMPLEMENTED=0x0600,

	// Image related
	GC_CORRUPT_IMAGE=0x0700,
	GC_PARTIAL_IMAGE,
	GC_IMAGE_READ_TRUNCATED,
	GC_IMAGE_TOO_LARGE,
	GC_IMAGE_TOO_SMALL,
	GC_IMAGE_TOO_COMPLEX,

	// File related
	GC_FILE_BASE=0x0800,

	// Network related
	GC_NET_INTERFACE_OUT_OF_RANGE=0x900,
	GC_NET_MAC_NOT_SET,
	GC_NET_MAC_NOT_CHANGEABLE,
	GC_NET_NO_PACKETS_AVAILABLE,
	GC_NET_MAC_UNABLE_TO_SEND,

	// Device/peripheral related:
	GC_STORAGE_BASE=0x0a00,
	GC_STORAGE_TYPE_UNKNOWN=GC_STORAGE_BASE,	// Flash type not known
	GC_STORAGE_ERASE_ERROR,						// Flash erasure error
	GC_STORAGE_PROGRAM_ERROR,					// Flash programming error
	GC_STORAGE_LOCKED,							// Flash page locked (can't program)
	GC_STORAGE_READ_ONLY,						// Read only - don't mess with it!
	GC_STORAGE_UNLOCK_FAILED,					// Flash page unlock failure
	GC_STORAGE_LOCK_FAILED,						// Flash page lock failure
	GC_STORAGE_UNIQUE_ID_NOT_AVAILABLE,			// No unique flash ID available,
	GC_STORAGE_PGM_COMPARE_ERROR,				// Flash program OK from part, but data miscompare
	GC_STORAGE_BAD_ADDRESS,						// Address out of range
	GC_STORAGE_BAD_SIZE,						// Size is too large (off the edge of the flash part)
	GC_STORAGE_BAD_PGM_VOLTAGE,					// Bad programming voltage error
	GC_STORAGE_REGION_INSTANCE_NOT_FOUND,		// Region and/or instance not found
	GC_STORAGE_REGION_BASE_ADDR_NOT_AVAILABLE,	// Base address not available for this device (stream)
	GC_STORAGE_OFFSET_OUT_OF_RANGE,				// Too far out of range
	GC_STORAGE_DEVICE_UNAVAILABLE,
	GC_STORAGE_SERIAL_NUMBER_NOT_AVAILABLE,
	GC_STORAGE_READ_ERROR,
	GC_STORAGE_WRITE_ERROR,
	GC_STORAGE_NOT_INITIALIZED,

	// GPIOs
	GC_GPIO_BASE=0x0b00,
	GC_GPIO_INDEX_INVALID=GC_GPIO_BASE,			// Invalid GPIO index
	GC_GPIO_CFG_FIELD_INVALID,					// Invalid field set in GPIOs
	GC_GPIO_NOT_AN_OUTPUT,						// Attempted to set a GPIO that's not an output

	// Terminator (leave here)
	GC_RESULT_CODE_MAX=0x1000000
} EGCResultCode;
// Controller equates

typedef enum
{
	// Don't edit these

	CTRL_P1U = 0,
	CTRL_P1D = 1,
	CTRL_P1L = 2,
	CTRL_P1R = 3,
	CTRL_P1S = 4,
	CTRL_P1C = 5,
	CTRL_P1B1 = 6,
	CTRL_P1B2 = 7,
	CTRL_P1B3 = 8,
	CTRL_P1B4 = 9,
	CTRL_P1B5 = 10,
	CTRL_P1B6 = 11,
	CTRL_P2U = 12,
	CTRL_P2D = 13,
	CTRL_P2L = 14,
	CTRL_P2R = 15,
	CTRL_P2S = 16,
	CTRL_P2C = 17,
	CTRL_P2B1 = 18,
	CTRL_P2B2 = 19,
	CTRL_P2B3 = 20,
	CTRL_P2B4 = 21,
	CTRL_P2B5 = 22,
	CTRL_P2B6 = 23,
	CTRL_MENU = 24,
	CTRL_TEST = 25,
	CTRL_SERVICE = 26,
	CTRL_SERVICE2 = 27,
	CTRL_END
} EControllerIndex;

#define UNICODE_TAB		('\t')
#define UNICODE_ENTER	('\r')

typedef enum
{
	EGCKey_Unknown = 0,
	EGCKey_Unicode,

	EGCKey_Keypad0,
	EGCKey_Keypad1,
	EGCKey_Keypad2,
	EGCKey_Keypad3,
	EGCKey_Keypad4,
	EGCKey_Keypad5,
	EGCKey_Keypad6,
	EGCKey_Keypad7,
	EGCKey_Keypad8,
	EGCKey_Keypad9,
	EGCKey_KeypadPeriod,
	EGCKey_Up,
	EGCKey_Down,
	EGCKey_Right,
	EGCKey_Left,
	EGCKey_Insert,
	EGCKey_Delete,
	EGCKey_Backspace,
	EGCKey_Home,
	EGCKey_End,
	EGCKey_PageUp,
	EGCKey_PageDown,
	EGCKey_Escape,
	EGCKey_Tab,
	EGCKey_Enter,
	EGCKey_Numlock,
	EGCKey_Capslock,
	EGCKey_Scrolllock,
	EGCKey_RShift,
	EGCKey_LShift,
	EGCKey_RControl,
	EGCKey_LControl,
	EGCKey_RAlt,
	EGCKey_LAlt,
	EGCKey_RMeta,
	EGCKey_LMeta,
	EGCKey_RSuper,
	EGCKey_LSuper,
	EGCKey_Mode,
	EGCKey_Compose,
	EGCKey_Backquote,
	EGCKey_Help,
	EGCKey_Print,
	EGCKey_Sysreq,
	EGCKey_Break,
	EGCKey_Menu,
	EGCKey_Power,
	EGCKey_Undo,
	EGCKey_F1,
	EGCKey_F2,
	EGCKey_F3,
	EGCKey_F4,
	EGCKey_F5,
	EGCKey_F6,
	EGCKey_F7,
	EGCKey_F8,
	EGCKey_F9,
	EGCKey_F10,
	EGCKey_F11,
	EGCKey_F12,
	EGCKey_F13,
	EGCKey_F14,
	EGCKey_F15,

	EGCKey_END
} EGCCtrlKey;

#include "Application/RSC68k.h"

typedef struct SBlock
{
	UINT32 u32BlockSize;			// How big is this block, including this structure, guard, and data?
	UINT32 u32Tag;					// BLOCK_GUARDS, BLOCK_NO_GUARDS
	struct SBlock *psFreePrior;		// Prior free block
	struct SBlock *psFreeNext;		// Next free block

	struct SBlock *psAllocPrior;	// Prior allocated block
	struct SBlock *psAllocNext;		// Next allocated block
	struct SBlock *psPrior;			// Prior block, regardless of allocation
	struct SBlock *psNext;			// Next block, regardless of allocation state

	UINT8 *pu8Module;				// Pointer to which module allocated this
	UINT32 u32Line;					// Line number for which module allocated this block
	void *pvUserBase;				// The base address of the user block
	UINT32 u32Filler1;
} SBlock;

typedef struct
{
	UINT32 u32HeapSize;			// How big is this heap, total, including this structure?
	UINT8 *pu8HeapName;			// Name of this heap
	void *vpAnchor;				// Do we have an anchor dropped?
	UINT32 u32AnchorItems;		// # Of items anchored in this heap

	SBlock *psFreeBlock;		// Pointer to free block list
	SBlock *psAllocBlock;		// Pointer to allocated block list
	SBlock *psBlock;			// Pointer to the block list
	UINT32 u32FreeBlockCount;	// How many free blocks do we have?

	UINT32 u32AllocBlockCount;	// How many allocated blocks do we have?
	UINT32 u32AllocatedSize;	// How big is our allocated size?
	UINT32 u32FreeSize;			// How big is our free size?
	BOOL bGuardsEnabled;		// Are header guards enabled for this heap?
} SMemoryHeap;

typedef struct STimerObject
{
	UINT32 u32TimerValue;
	UINT32 u32ReloadValue;		// 0 = One shot
	UINT32 u32CallbackValue;
	BOOL bRunning;				// Is this timer running?
	void (*Handler)(UINT32);
	struct STimerObject *psNextLink;
} STimerObject;

typedef UINT32 GCFile;

typedef void *SFileFind;
#define GC_EOF     (-1)

#define GC_SEEK_CUR    1
#define GC_SEEK_END    2
#define GC_SEEK_SET	0

#include "Application/File.h"

// WIN32 Specific segment

#ifndef _WIN32

typedef long long int off_t;

#else // #ifndef _WIN32

#ifndef _OFF_T_DEFINED
typedef __int64 off_t;
typedef long _off_t;                    /* file offset value */
#define _OFF_T_DEFINED
#endif
#define __swi(x)
#define __packed

#endif

#ifdef IARARM

#define __packed
#define __inline
#define __int64 long long int
#define SWI_CALL(x) __swi __arm 
#else  // ARMUK
#define SWI_CALL(x) extern __swi(x) 
#endif

typedef struct SVideoModes
{
	UINT32 u32XSize;
	UINT32 u32YSize;
	UINT8 u8BPP;
	UINT8 u8RefreshRate;
	UINT32 u32Flags;
	UINT32 u32Pitch;
} SVideoModes;

typedef struct SZipList
{
	UINT8 *pu8Filename;
	UINT32 u32CRC;
	BOOL bAvailable;
	UINT32 u32FileSize;
} SZipList;

typedef struct SFileInfo
{
	UINT32 u32CRC;
	BOOL bAvailable;
	UINT32 u32FileSize;
	UINT32 u32CreationTime;
	UINT32 u32ModificationTime;
} SFileInfo;

typedef struct SMemoryInfo
{
	UINT8 *pu8ModuleName;
	UINT32 u32LineNumber;
	BOOL bClearBlock;
} SMemoryInfo;

#define	SERIAL_CTRL_DTR			0x01		// Data terminal ready
#define SERIAL_CTRL_RTS			0x02		// Request to send
#define SERIAL_CTRL_CTS			0x04		// Clear to send
#define SERIAL_CTRL_DCD			0x08		// Data carrier detect
#define SERIAL_CTRL_RI			0x10		// Ring indicate
#define	SERIAL_CTRL_DSR			0x20		// Data set ready
#define SERIAL_OVERRUN_HW		0x40		// Lost character (HW)
#define SERIAL_OVERRUN_SW		0x80		// Lost character (SW)

// Line settings

// Bits 0-1
#define SERIAL_LINE_DATA_MASK		0x03
#define	SERIAL_LINE_DATA_BITS_5		0x00
#define	SERIAL_LINE_DATA_BITS_6		0x01
#define	SERIAL_LINE_DATA_BITS_7		0x02
#define	SERIAL_LINE_DATA_BITS_8		0x03

// Bits 2-4
#define SERIAL_LINE_PARITY_MASK		(0x7 << 2)
#define SERIAL_LINE_PARITY_EVEN		(0 << 2)		
#define SERIAL_LINE_PARITY_ODD		(1 << 2)
#define SERIAL_LINE_PARITY_NONE		(4 << 2)

// Bits 5-6
#define SERIAL_LINE_STOP_BIT_MASK	(0x03 << 5)
#define SERIAL_LINE_STOP_BITS_1		(0 << 5)
#define SERIAL_LINE_STOP_BITS_2		(2 << 5)

// Bits 7-8
#define SERIAL_FLOW_CONTROL_MASK	(0x3 << 7)
#define SERIAL_FLOW_CONTROL_NONE	(0 << 7)
#define SERIAL_FLOW_CONTROL_CTSRTS	(1 << 7)

// GPIO configuration bits

#define GPIO_CFG_OUTPUTDRIVE		0x01	// 1=Output, 0=Input
#define GPIO_CFG_PULLUP				0x02	// 1=Puullup enabled, 0=Pullup disabled
#define GPIO_CFG_DRIVE				0x04	// 1=Totem pole, 0=Low only

#define GPIO_CFG_MASK				(GPIO_CFG_OUTPUTDRIVE | GPIO_CFG_PULLUP | GPIO_CFG_DRIVE)

// GPIOs
SWI_CALL(0x324) EGCResultCode GCGPIOSetState(UINT32 u32GPIONumber,
											 BOOL bState);
SWI_CALL(0x328) EGCResultCode GCGPIOGetState(UINT32 u32GPIONumber,
											 BOOL *pbStateLatched,
											 BOOL *pbStatePins);
SWI_CALL(0x32c) EGCResultCode GCGPIOSetConfig(UINT32 u32GPIONumber,
											  UINT32 u32ConfigBits);

// SPI
typedef enum
{
	// Private SSs are 0x40000000 and greater
	SPISS_TOUCH_SCREEN=0x40000000,
	SPISS_BASE=SPISS_TOUCH_SCREEN,
	SPISS_RTC,
	SPISS_NIC0
} ESPISSEnum;

typedef struct SSPIBusInfo
{
	UINT32 u32Bus;
	UINT32 u32ClockRate;
	
	// Write
	BOOL bWriteClockPolarity;
	BOOL bWriteDataPolarity;
	UINT8 *pu8WriteData;
	UINT32 u32WriteLength;
	
	// Read related stuff
	BOOL bReadClockPolarity;
	BOOL bReadDataPolarity;
	UINT8 *pu8ReadData;
	UINT32 u32ReadLength;
	
	// Sleeps/waits between various transactions
	UINT32 u32USecSleepBeforeSS;
	UINT32 u32USecSleepAfterSS;

	// Slave select GPIOs
	ESPISSEnum eDeviceSS;
	BOOL bUserSSAssertedState;
	
	// Platform specific gook (user doesn't fill in anything beyond this point)
	void *pvSPIBusBase;
} SSPIBusInfo;

// SPI
SWI_CALL(0x330) EGCResultCode SPIMasterTransaction(SSPIBusInfo *psBusInfo);

// I2C
SWI_CALL(0x360) EGCResultCode GCI2CMasterWrite(UINT32 u32Bus,
											   UINT8 u8SlaveAddress,
											   UINT8 *pu8DataToWrite,
											   UINT32 u32Count);
SWI_CALL(0x364) EGCResultCode GCI2CMasterRead(UINT32 u32Bus,
											  UINT8 u8SlaveAddress,
											  UINT8 *pu8DataToRead,
											  UINT32 u32Count);
SWI_CALL(0x368) EGCResultCode GCI2CUnstickBus(UINT32 u32Bus);


// Serial
SWI_CALL(0x288) EGCResultCode GCSerialSetControlLines(UINT32 u32Index,
													UINT32 u32ControlLines,
													UINT32 u32ControlStates);
SWI_CALL(0x28c) EGCResultCode GCSerialGetControlLines(UINT32 u32Index,
													UINT32 *pu32ControlStates);
SWI_CALL(0x290) EGCResultCode GCSerialSetBaudRate(UINT32 u32Index,
												UINT32 u32BaudRate);
SWI_CALL(0x300) EGCResultCode GCSerialSendData(UINT32 u32Index,
											   UINT8 *pu8Data,
											   UINT32 u32Count,
											   UINT32 u32Timeout);
SWI_CALL(0x304) EGCResultCode GCSerialGetData(UINT32 u32Index,
											  UINT8 *pu8DataBuffer,
											  UINT32 *pu32Count,
											  UINT32 u32Timeout);
SWI_CALL(0x308) EGCResultCode GCSerialSetDataCallback(UINT32 u32Index,
				 									  void (*ReceiveCallback)(UINT32 u32UARTNumber,
 																		      UINT8 *pu8Data,
 																			  UINT32 u32Count));
SWI_CALL(0x30c) EGCResultCode GCSerialSetBufferSizes(UINT32 u32Index,
													 UINT32 u32SendBufferSize,
													 UINT32 u32ReceiveBufferSize);
SWI_CALL(0x310) EGCResultCode GCSerialGetDataCountAvailable(UINT32 u32Index,
															UINT32 *pu32RXDataCount,
															UINT32 *pu32TXDataCount);
SWI_CALL(0x314) EGCResultCode GCSerialFlushSend(UINT32 u32Index,
												UINT32 u32Timeout);
SWI_CALL(0x318) EGCResultCode GCSerialClearBuffers(UINT32 u32Index,
												   BOOL bClearReceive,
												   BOOL bClearSend);
SWI_CALL(0x31c) EGCResultCode GCSerialSetLine(UINT32 u32Index,
											  UINT32 u32LineSettings);
SWI_CALL(0x320) EGCResultCode GCSerialGetDebugPortNumber(UINT32 *pu32DebugPortNumber);
extern EGCResultCode GCSerialIsOpen(UINT32 u32Index);
extern EGCResultCode GCSerialOpen(UINT32 u32Index);
extern EGCResultCode GCSerialClose(UINT32 u32Index);


// Debug
SWI_CALL(0x1e0) void DebugOutBIOS(const char*);
SWI_CALL(0x1e8) void GCDebugSetReceiveCallback(void (*ReceiveCallback)(UINT32 u32UARTNumber,
																		   UINT8 *pu8Data,
																		   UINT32 u32Count));

// Display
SWI_CALL(0x108) EGCResultCode GCDisplayGetColorDepth(UINT8 *pu8Depth);
SWI_CALL(0x10c) EGCResultCode GCDisplayGetXSize(UINT32 *pu32XSize);
SWI_CALL(0x110) EGCResultCode GCDisplayGetYSize(UINT32 *pu32YSize);
SWI_CALL(0x114) UINT32 GCGetRefreshRate(void);
SWI_CALL(0x20) EGCResultCode GCDisplayGetDisplayPitch(UINT32 *pu32DisplayPitch);
SWI_CALL(0x10) void GCSetFrameCallback(void (*pFrameProc)(void));
SWI_CALL(0x160) void GCSetModeChangeCallback(void (*pModeChangeProc)(void));
SWI_CALL(0x18) EGCResultCode GCDisplaySetMode(UINT32 u32XSize, UINT32 u32YSize, UINT8 u8BPP, UINT32 u32ClearScreenColor);
SWI_CALL(0x1c) EGCResultCode GCDisplayGetDisplayBuffer(void **ppvDisplayBuffer);
SWI_CALL(0xd0) EGCResultCode GCSetFrameBufferAddress(UINT16 *pu16Address);
SWI_CALL(0x1fc) SVideoModes *GCGetAvailableVideoModes(void);
SWI_CALL(0x214) EGCResultCode GCDisplaySetPaletteEntry(UINT8 u8Index, UINT8 u8Red, UINT8 u8Green, UINT8 u8Blue);
SWI_CALL(0x274) EGCResultCode GCSetLCDBacklightLevel(UINT8 u8Level);
extern EGCResultCode GCMinimizeApplication( void );

// Multipage display APIs
SWI_CALL(0x238) void GCWaitForVsyncFlip(void *pvPagePointer);
SWI_CALL(0x23c) EGCResultCode GCDisplayGetDisplayPitchPage(void *pvPagePointer, UINT32 *pu32DisplayPitch);
SWI_CALL(0x240) EGCResultCode GCDisplayGetDisplayBufferPage(void *pvPagePointer, void **ppvPagePointer);
SWI_CALL(0x244) EGCResultCode GCDisplayCreatePage(void **ppvPagePointer);
SWI_CALL(0x248) EGCResultCode GCDisplayDeletePage(void *pvPagePointer);

// 2D
SWI_CALL(0x24) EGCResultCode GCDisplayClear(UINT8 u8FillColor);
SWI_CALL(0x90) void GCprintfInternal(UINT16 u16Foreground, UINT16 u16Background, UINT8 *fmt);
extern			   void GCprintf(UINT16 u16Foreground, UINT16 u16Background, UINT8 *fmt, ...);
SWI_CALL(0x9c) void GotoXY(UINT32 u32X, UINT32 u32Y);
SWI_CALL(0xe4) void GCWaitForVsync(void);

// Timer
SWI_CALL(0x120) EGCResultCode GCTimerCreate(STimerObject **ppsTimer);
SWI_CALL(0x2c) void GCTimerDelete(STimerObject *psTimerToDelete);
SWI_CALL(0x124) EGCResultCode GCTimerSetCallback(STimerObject *psTimer,
													 void (*Handler)(UINT32),
													 UINT32 u32CallbackValue);
SWI_CALL(0x128) EGCResultCode GCTimerSetValue(STimerObject *psTimer,
												  UINT32 u32InitialValue,
												  UINT32 u32ReloadValue);
SWI_CALL(0x12c) EGCResultCode GCTimerStart(STimerObject *psTimer);
SWI_CALL(0x130) EGCResultCode GCTimerStop(STimerObject *psTimer);
SWI_CALL(0x134) void GCTimerDelete(STimerObject *psTimerToDelete);
SWI_CALL(0x138) UINT32 GCTimerGetPeriod(void);
SWI_CALL(0x13c) EGCResultCode GCTimerSetPeriod(UINT32 u32Milliseconds);
SWI_CALL(0x220) UINT32 GCGetCPUHz(void);

// Clock related
SWI_CALL(0x1ec)	UINT32 GCGetTimeMS(void);
SWI_CALL(0x224) void GCWaitUSec(UINT32 u32MicrosecondsToWait);
SWI_CALL(0x258) EGCResultCode RTCGetTime(time_t *peTime,
										 BOOL *pbTimeNotSet);
SWI_CALL(0x25c) EGCResultCode RTCSetTime(time_t eTime);
SWI_CALL(0x260) EGCResultCode RTCGetPowerOnSeconds(UINT32 *pu32Seconds);
SWI_CALL(0x264) EGCResultCode RTCSetTimezoneOffset(INT32 s32Offset);
SWI_CALL(0x2cc) EGCResultCode RTCGetTimezoneOffset(INT32 *ps32Offset);
SWI_CALL(0x2d0) EGCResultCode RTCSetDST(BOOL bDSTEnabled);
SWI_CALL(0x2d4) EGCResultCode RTCGetDST(BOOL *pbDSTEnabled);

//Sound
SWI_CALL(0x38) EGCResultCode GCSoundOpen(UINT32 u32BufferSampleCountPerChannel);
SWI_CALL(0x3c) void GCSoundSetCallback(void (*Handler)(INT16 *ps16Buffer, UINT32 u32StereoSamples));
SWI_CALL(0x40) EGCResultCode GCSoundSetSampleRate(UINT32 u32SampleRate);
SWI_CALL(0x200) UINT32 GCSoundGetSampleRate(void);
SWI_CALL(0x204) UINT32 GCSoundGetSampleCountPerChannel(void);
SWI_CALL(0x44) EGCResultCode GCSoundResume(void);
SWI_CALL(0x48) void GCSoundPause(void);
SWI_CALL(0x4c) EGCResultCode GCSoundClose(void);

// File IO (legacy)
SWI_CALL(0x50) GCFile GCfopen(const char *pu8Filename, const char* pu8Mode);
SWI_CALL(0x54) INT32 GCfclose(GCFile psFileToClose);
SWI_CALL(0x58) UINT32 GCFileSize(GCFile psFile);
SWI_CALL(0x5c) INT32 GCfseek(GCFile psFile, off_t s32Offset, INT32 s32Whence);
SWI_CALL(0x60) UINT32 GCfread(void *pvData, UINT32 u32Size,	UINT32 u32Blocks, GCFile psFile);
SWI_CALL(0x64) UINT32 GCftell(GCFile psFile);
SWI_CALL(0x144) INT32 GCfgetpos( GCFile stream, UINT32* pos );
SWI_CALL(0x148) INT32 GCfwrite(void*, UINT32, UINT32, GCFile);
extern				INT32 GCfprintf(GCFile, const UINT8 *, ...);
SWI_CALL(0x14c) UINT8* GCfgets( char* string,  UINT32,  GCFile stream);
SWI_CALL(0x150)	INT32 GCfputs( const char* str, GCFile stream);
SWI_CALL(0x154)	INT32 GCfeof( GCFile stream );
SWI_CALL(0x158)	INT32 GCferror(GCFile stream );
SWI_CALL(0x15c)	INT32 GCfgetc( GCFile stream );
extern UINT32 GCGetDriveBitmap(void);

// File IO (New!)

SWI_CALL(0x294) EGCResultCode GCFileOpen(GCFile *peHandle, const char *pu8Filename, const char* pu8Mode);
SWI_CALL(0x298) EGCResultCode GCFileClose(GCFile *peHandle);
SWI_CALL(0x29c) EGCResultCode GCFileSizeByHandle(GCFile eHandle, UINT64 *pu64FileSize);
SWI_CALL(0x2a0) EGCResultCode GCFileSizeByFilename(const char *pu8Filename, UINT64 *pu64FileSize);
SWI_CALL(0x2a4) EGCResultCode GCFileSeek(GCFile eHandle, off_t s64Offset, INT32 s32Whence);
SWI_CALL(0x2a8) EGCResultCode GCFileRead(void *pvData, UINT32 u32ByteCount, UINT32 *pu32DataRead, GCFile eHandle);
SWI_CALL(0x2ac) EGCResultCode GCFileTell(off_t *ps64Offset, GCFile eHandle);
SWI_CALL(0x2b0) EGCResultCode GCFileGetPos(GCFile eHandle, off_t *ps64Offset);
SWI_CALL(0x2b4) EGCResultCode GCFileWrite(void *pvData, UINT32 u32ByteCount, UINT32 *pu32DataWritten, GCFile eHandle);
extern	EGCResultCode GCFileFprintf(GCFile eHandle, UINT32 *pu32BytesWritten, const char *, ...);
SWI_CALL(0x2b8) EGCResultCode GCFileGets(char **ppu8String, char *pu8Buffer, UINT32 u32BufferSize, GCFile eHandle);
SWI_CALL(0x2bc) EGCResultCode GCFilePuts(const char *pu8String, GCFile eHandle);
SWI_CALL(0x2c0) EGCResultCode GCFileEOF(GCFile eHandle, BOOL *pbFileEOF);
SWI_CALL(0x2c4) EGCResultCode GCFileFgetc(char *pu8Character, GCFile eHandle);
SWI_CALL(0x2c8) EGCResultCode GCFileSystemSync(void);

SWI_CALL(0x230) EGCResultCode GCFileGetInfo(UINT8 *pu8Filename,
											SFileInfo *psFileInfo);
extern LEX_CHAR* GCGetCurrentDirectory( void );

SWI_CALL(0x6c) EGCResultCode GCZipUncompress(UINT8 *pu8ZipFilename, UINT8 *pu8FileWithinZip, UINT8 *pu8Location, UINT8 *pu8Password);
SWI_CALL(0x94) EGCResultCode GCZipGetUncompressedFileSize(const UINT8*pu8ZipFilename, const UINT8* pu8FileWithinZip, UINT32 *pu32UncompressedFileSize);
SWI_CALL(0x98) EGCResultCode GCZipGetUncompressedArchiveFileSize(const UINT8* pu8FileWithinZip, UINT32 *pu32UncompressedFileSize);
SWI_CALL(0x14) EGCResultCode GCZipUncompressFromArchive(const UINT8*pu8FileWithinZip, UINT8 *pu8Location);
SWI_CALL(0x1f0) EGCResultCode GCZipGetCRCFromArchive(UINT8 *pu8FileWithinZip, UINT32 *pu32CRC);
SWI_CALL(0x1f4) EGCResultCode GCZipValidateFileCRCFromArchive(SZipList *psZipList);
SWI_CALL(0x1f8) EGCResultCode GCZipValidateFileCRCFromZip(UINT8 *pu8ZipFilename, SZipList *psZipList);
SWI_CALL(0x21c) EGCResultCode GCCheckDisk(void **ppvHandle, UINT8 *pu8PercentComplete);
SWI_CALL(0x26c) EGCResultCode GCFormatDisk(UINT8* pu8DeviceName, void** ppvHandle, UINT8* pu8PercentComplete);
SWI_CALL(0x268) EGCResultCode GCGetFilesystemSize(UINT8 *pu8Filename, UINT64* pu64TotalBytes, UINT64* pu64FreeBytes);

//NVRam
SWI_CALL(0x70) UINT32 GCNVStoreGetSize(void);
SWI_CALL(0x74) EGCResultCode GCNVStoreRead(void *pvDataBuffer, UINT32 u32NVOffset, UINT32 u32ByteCount);
SWI_CALL(0x78) EGCResultCode GCNVStoreWrite(void *pvDataBuffer, UINT32 u32NVOffset, UINT32 u32ByteCount);
SWI_CALL(0xb8) EGCResultCode GCNVStoreCommit(void);
SWI_CALL(0xbc) void GCNVStoreClear(void);
SWI_CALL(0x1d8) EGCResultCode GCProgramProductID(UINT32 u32ProductID);
SWI_CALL(0x1dc) EGCResultCode GCGetProductID(UINT32 *pu32ProductID);

// Memory/heap related
SWI_CALL(0x7c) void *SRAMAlloc(UINT32 u32Size);
SWI_CALL(0x80) void SRAMFree(void *pvBase);
SWI_CALL(0x84) EGCResultCode GCCreateMemoryHeap(UINT32 u32Size, SMemoryHeap *psMemoryHeap, UINT8 *pu8HeapName);
SWI_CALL(0x1cc) EGCResultCode GCAllocateMemoryFromHeap(SMemoryHeap *psMemoryHeap, UINT32 u32Size, void **ppvMemoryBlock, SMemoryInfo *psMemoryInfo);
SWI_CALL(0x24c) EGCResultCode GCReallocMemoryFromHeap(SMemoryHeap *psMemoryHeap, UINT32 u32Size, void **ppvMemoryBlock, SMemoryInfo *psMemoryInfo);
SWI_CALL(0x8c) void GCFreeMemoryFromHeap(SMemoryHeap *psMemoryHeap, void *pvMemoryLocation);
SWI_CALL(0xc0) EGCResultCode HeapDropAnchor(SMemoryHeap *psMemoryHeap);
SWI_CALL(0xc4) EGCResultCode HeapRaiseAnchor(SMemoryHeap *psMemoryHeap);
SWI_CALL(0x100) void HeapSetHeaderGuards(SMemoryHeap *psMemoryHeap, BOOL bGuardsEnabled);
SWI_CALL(0x104) void CheckHeapIntegrity(SMemoryHeap *psMemoryHeap);
SWI_CALL(0x140) void GCSetUserHeap(SMemoryHeap *psMemoryUserHeap);
SWI_CALL(0xc8) EGCResultCode SDRAMDropAnchor(void);
SWI_CALL(0xcc) EGCResultCode SDRAMRaiseAnchor(void);
SWI_CALL(0x140) void GCSetUserHeap(SMemoryHeap *psMemoryHeap);
SWI_CALL(0x234) void HeapReport(SMemoryHeap *psMemoryHeap,
								UINT8 *pu8DumpFilename);
SWI_CALL(0x2f4) EGCResultCode GCGetBlockSizeFromHeap(SMemoryHeap *psMemoryHeap,
													 void *pvBlock,
													 UINT32 *pu32BlockSize);
SWI_CALL(0x2f8) void GCGetFreeInfoFromHeap(SMemoryHeap *psMemoryHeap,
										   UINT32 *pu32LargestFree,
										   UINT32 *pu32TotalFree);
SWI_CALL(0x2fc) void GCGetAllocInfoFromHeap(SMemoryHeap *psMemoryHeap,
											UINT32 *pu32LargestAlloc,
											UINT32 *pu32TotalAlloc);

// Network related

#define		NETMASK_LINK_STATUS		0x01		// Link changed
#define		NETMASK_PACKET_RX		0x02		// Got a packet - go read it

SWI_CALL(0x2d8) EGCResultCode GCNICSetReceiveCallback(UINT32 u32Interface, void (*NicRXCallback)(UINT32 u32Interface));
SWI_CALL(0x2d9) EGCResultCode GCNICGetInterruptReason(UINT32 u32Interface, UINT32* pu32ChangeMask);
SWI_CALL(0x2da) EGCResultCode GCNICGetLinkStatus(UINT32 u32Interface, BOOL* pbLinkIsUp);
SWI_CALL(0x2dc) EGCResultCode GCNICSendPacket(UINT32 u32Interface, UINT8 *pu8Data, UINT32 u32Size);
SWI_CALL(0x2ec) EGCResultCode GCNICReceivePacket(UINT32 u32Interface, UINT8 *pu8Data, UINT32 u32Size, UINT32 *pu32PacketSize);
SWI_CALL(0x2e0) EGCResultCode GCNICGetName(UINT32 u32Interface, char *pu8NameBuffer, UINT32 u32MaxChars);
SWI_CALL(0x2e4) EGCResultCode GCNICGetMACAddress(UINT32 u32Interface, UINT8 *pu8MACLocation);
SWI_CALL(0x2e8) EGCResultCode GCNICSetMACAddress(UINT32 u32Interface, UINT8 *pu8MACLocation);

// System Utilities
SWI_CALL(0xec) EGCResultCode GCSetProcessorSpeed(UINT32 u32Speed);
SWI_CALL(0xa0) void GCHalt(void);
SWI_CALL(0xa8) void GCWaitMS(UINT32);
extern				void GCPoll(void);
SWI_CALL(0x118) void GCBoardReset(void);

// Pointer related
SWI_CALL(0x254) EGCResultCode GCPointerGetPosition(UINT32 u32Instance, UINT32 *pu32XPos, UINT32 *pu32YPos, UINT32 *pu32Buttons);
SWI_CALL(0x278) EGCResultCode GCPointerCalibrate(UINT32 u32Instance);
SWI_CALL(0x27c) EGCResultCode GCPointerGetCalibrationState(UINT32 u32Instance);

#define	POINTER_B1			0x0001
#define POINTER_B2			0x0002
#define POINTER_B3			0x0004
#define POINTER_B1RPT		0x0008
#define POINTER_B2RPT		0x0010
#define POINTER_B3RPT		0x0020

// Misc
extern void ThreadSetName(char *pu8ThreadName);
SWI_CALL(0xa4) UINT32 GCRandomNumber(void);
SWI_CALL(0x68) INT32 GCerror(void);
SWI_CALL(0x11c) void GCSetRestartOverride(BOOL bOverride);
SWI_CALL(0x1d0) EGCResultCode GCSet3DInterruptCallback(void (*pCallback)(void));
SWI_CALL(0x208) void GCOutputSet(UINT8 u8OutputNumber, BOOL bState);
SWI_CALL(0x218) void GCSetHeartbeatEnable(BOOL bHeartbeatEnabled);
SWI_CALL(0x344) EGCResultCode GCSetLogo(UINT16 *pu16Buffer,
										UINT32 u32XSize,
										UINT32 u32YSize,
										UINT32 u32Pitch);
SWI_CALL(0x348) BOOL GCIsLogoProgrammed(void);

typedef enum
{
	LOGSRC_BIOS,		// From the BIOS itself
	LOGSRC_APP,			// From the application itself
	LOGSRC_USER			// From the user (specifically added)
} ELogSource;

typedef struct SLogEvent
{
	UINT8 u8LogSource;
	UINT32 u32LineNumber;
	time_t eTimestamp;
	char *pu8ModuleName;
	UINT32 u32MaxModuleNameLength;
	char *pu8Message;
	UINT32 u32MaxMessageLength;
} SLogEvent;

SWI_CALL(0x34c) EGCResultCode GCLogEntryAdd(SLogEvent *psEvent,
											UINT32 *pu32LogIndex);
SWI_CALL(0x350) EGCResultCode GCLogEntryDelete(UINT32 u32Index);
SWI_CALL(0x354) EGCResultCode GCLogEntryGet(UINT32 u32Index,
											SLogEvent *psLogEntry);
SWI_CALL(0x358) EGCResultCode GCLogGetInfo(UINT32 *pu32TotalEntries,
										   UINT32 *pu32ActiveEntries);
SWI_CALL(0x35c) EGCResultCode GCIsDir(char *pu8DirectoryName);

// File enumeration. 
SWI_CALL(0xd8) SFileFind* GCFileFindSetup(const char* pu8Wildcard);
SWI_CALL(0xdc) EGCResultCode GCFileFindNext(SFileFind* psFileFind,
												char*pu8Filename,
												UINT32 u32FilenameMaxLength);
SWI_CALL(0x228) EGCResultCode GCFileFindGetAttribute(SFileFind *psCurrentFile,
													 UINT32 *pu32CurrentAttribute,
													 UINT64 *pu64FileSize,
													 time_t *peFileModificationTime);
SWI_CALL(0xe0) void GCFileFindEnd(SFileFind *psFileFind);
extern UINT32 GCGetPWD(UINT32 u32BufferLength, UINT8* psBuffer);
SWI_CALL(0x164) EGCResultCode GCmkdir(const char* in_pDirName);
SWI_CALL(0x2f0) EGCResultCode GCrmdir(const char *in_pDirName);
SWI_CALL(0x168) EGCResultCode GCDeleteFile(const char* in_pDirName);
SWI_CALL(0x270)	UINT32 GCGetPCBID(void);

#define PCBID_GAMEROOM_CLASSICS		0x25
#define PCBID_DOUG					0xeb
#define PCBID_JET					0x0a
#define PCBID_EA50					0x29
#define	PCBID_UC					0x41479425

// File find attribute #defines

#define FILEATTR_ARDONLY  0x01 /* Read only */
#define FILEATTR_AHIDDEN  0x02 /* Hidden */
#define FILEATTR_ASYSTEM  0x04 /* System */
#define FILEATTR_AVOLID   0x08 /* Volume label */
#define FILEATTR_ADIR     0x10 /* Directory */
#define FILEATTR_AARCHIV  0x20 /* Modified since last backup */
#define FILEATTR_ALFN     0x0F /* Long file name directory slot (R+H+S+V) */
#define FILEATTR_AALL     0x3F /* Select all attributes */
#define FILEATTR_ANONE    0x00 /* Select no attributes */
#define FILEATTR_ANOVOLID 0x37 /* All attributes but volume label */
#define FILEATTR_ANORMAL  0x80 /* Normal file */

// Performance Counters
SWI_CALL(0xf0) EGCResultCode GCGetPerformanceCounter(UINT32 u32Counter, UINT64 *pu64CounterValue);
SWI_CALL(0xf4) EGCResultCode GCSetPerformanceCounter(UINT32 u32Counter, UINT32 u32Value);
EGCResultCode GCGetPerformanceCounterFrequency(UINT32 u32Counter,
											   UINT64 *pu64Frequency);

// Input
SWI_CALL(0xf8) BOOL GCInputReadState(EControllerIndex eController);
SWI_CALL(0xfc) void GCInputSetCallback(void (*pCallback)(EControllerIndex eIndex, BOOL bNewState));
SWI_CALL(0xe8) EGCResultCode GCInputReadTrackball(UINT8 u8TrackballNumber,
													  INT8 *ps8X, INT8 *ps8Y);
SWI_CALL(0x20c) EGCResultCode GCInputSetTrackball(UINT8 u8TrackballNumber,
								  BOOL bInvertX,
								  BOOL bInvertY,
								  BOOL bSwapXY);
extern void GCSetKeyCallback(BOOL (*pfHandler)(EGCCtrlKey eGCKey, LEX_CHAR eUnicode, BOOL bPressed));
extern void GCSetMousewheelCallback(void (*Callback)(UINT32 u32Value));

// Enumeration for delete semaphore options
typedef enum
{
	DELETEOP_NO_PENDING,		// Delete only if there are no pending threads
	DELETEOP_ALWAYS				// Delete regardless of pending threads
} EOSEventDeleteOption;

// Semaphores are just pointers
typedef void *SOSSemaphore;

// So are queues
typedef void *SOSQueue;

// And Mutexes
typedef void *SOSMutex;

// And event flags
typedef void *SOSEventFlag;

// Operating system calls
SWI_CALL(0x16c) UINT32 GCOSStart(void);
SWI_CALL(0x170) void GCOSStop(BOOL bHardShutdown,
							  UINT32 u32ExitCode);
SWI_CALL(0x1d4) BOOL GCOSIsRunning(void);

// Thread manipulation
SWI_CALL(0x174) EGCResultCode GCOSThreadCreate(void (*pThreadEntry)(void *pvParameter),
												  void *pvEntryParameter,
												  UINT8 *pu8TopOfStack,
												  UINT8 u8ThreadPriority);
SWI_CALL(0x178) EGCResultCode GCOSThreadSuspend(UINT8 u8ThreadPriorityToSuspend);
SWI_CALL(0x17c) EGCResultCode GCOSThreadResume(UINT8 u8ThreadPriorityToSuspend);
SWI_CALL(0x180) EGCResultCode GCOSThreadKill(UINT8 u8ThreadPriority);
SWI_CALL(0x184) EGCResultCode GCOSThreadChangePriority(UINT8 u8ThreadPriorityOld,
														  UINT8 u8ThreadPriorityNew);
SWI_CALL(0x188) EGCResultCode GCOSSleep(UINT32 u32TicksToSleep);
SWI_CALL(0x280) EGCResultCode GCOSGetThreadID(UINT32 *pu32ThreadID);
SWI_CALL(0x284) EGCResultCode GCOSGetMaxThreadCount(UINT32 *pu32MaxThreadCount);

// Semaphores
SWI_CALL(0x18c) EGCResultCode GCOSSemaphoreCreate(SOSSemaphore *ppsSemaphore,
											     UINT32 u32InitialValue);
SWI_CALL(0x190) EGCResultCode GCOSSemaphorePut(SOSSemaphore psSemaphore);
SWI_CALL(0x194) EGCResultCode GCOSSemaphoreGet(SOSSemaphore psSempahore,
												  UINT32 u32Timeout);			// 0=Wait forever, all else == # of ticks
SWI_CALL(0x198) EGCResultCode GCOSSemaphoreDelete(SOSSemaphore psSemaphore,
												     EOSEventDeleteOption eDeleteOption);
SWI_CALL(0x22c)	EGCResultCode GCOSSemaphoreGetCount(SOSSemaphore psSemaphore,
													UINT32 *pu32SemaphoreCount);

// Queue
SWI_CALL(0x19c) EGCResultCode GCOSQueueCreate(SOSQueue *ppsQueue,			// Each queue item is assumed to be 4 bytes each
												 void **pvQueueData,
												 UINT32 u32QueueSizeInElements);
SWI_CALL(0x1a0) EGCResultCode GCOSQueueReceive(SOSQueue psQueue,
												  void **pvQueueEvent,
												  UINT32 u32Timeout);			// 0=Wait forever, all else == # of ticks
SWI_CALL(0x1a4) EGCResultCode GCOSQueueSend(SOSQueue psQueue,
											   void *pvQueueEvent);
SWI_CALL(0x1a8) EGCResultCode GCOSQueueDelete(SOSQueue psQueue,
												 EOSEventDeleteOption eDeleteOption);
SWI_CALL(0x1ac) EGCResultCode GCOSQueueFlush(SOSQueue psQueue);
SWI_CALL(0x250) EGCResultCode GCOSQueuePeek(SOSQueue psQueue,
											void **pvQueueEvent);

// Mutex
SWI_CALL(0x1b0) EGCResultCode GCOSMutexCreate(SOSMutex *ppsMutex,
												 UINT32 u32PriorityInheritanceValue);
SWI_CALL(0x1b4) EGCResultCode GCOSMutexUnlock(SOSMutex psMutex);
SWI_CALL(0x1b8) EGCResultCode GCOSMutexLock(SOSMutex psMutex,
											UINT32 u32Timeout);
SWI_CALL(0x1bc) EGCResultCode GCOSMutexTest(SOSMutex psMutex);
SWI_CALL(0x1c0) EGCResultCode GCOSMutexDelete(SOSMutex psMutex,
												 EOSEventDeleteOption eDeleteOption);

typedef enum
{
	EFLAG_WAIT_CLEAR_ALL,
	EFLAG_WAIT_CLEAR_ANY,
	EFLAG_WAIT_SET_ALL,
	EFLAG_WAIT_SET_ANY
} EEventFlagWaitType;

typedef struct SEventFlagWait
{
	UINT32 u32FlagMask;
	EEventFlagWaitType eWait;
	UINT32 u32Timeout;
} SEventFlagWait;

// Event flags
SWI_CALL(0x334) EGCResultCode GCOSEventFlagCreate(SOSEventFlag *ppsEventFlag,
												  UINT32 u32InitialEventFlagSetting);
SWI_CALL(0x338) EGCResultCode GCOSEventFlagSet(SOSEventFlag psEventFlag,
											   BOOL bSetFlags,
											   UINT32 u32FlagMask);
SWI_CALL(0x33c) EGCResultCode GCOSEventFlagGet(SOSEventFlag psEventFlag,
											   SEventFlagWait *psEventFlagWait,
											   UINT32 *pu32FlagMaskObtained);
SWI_CALL(0x340) EGCResultCode GCOSEventFlagDelete(SOSEventFlag psEventFlag,
												  EOSEventDeleteOption eDeleteOption);

// Critical sections
SWI_CALL(0x1c4) EGCResultCode GCOSCriticalSectionEnter(void);
SWI_CALL(0x1c8) EGCResultCode GCOSCriticalSectionExit(void);

// Video modes

#define MODE_320X240X16				320, 240, 16
#define MODE_256X256X16STRETCHED	256, 256, 16
#define MODE_256X256X16SQUARE		256, 256, 17
#define MODE_256X240X16STRETCHED	256, 240, 16
#define MODE_256X240X16SQUARE		256, 240, 17
#define MODE_388X224X16				388, 224, 16

extern EGCResultCode BIOSHeapDropAnchor(void);
extern EGCResultCode BIOSHeapRaiseAnchor(void);
extern EGCResultCode AppHeapDropAnchor(void);
extern EGCResultCode AppHeapRaiseAnchor(void);

//#define	GCPERFASSERT(expr)	GCASSERT(expr)
#define	GCPERFASSERT(expr)

#define GCAllocateMemory(x) GCAllocateMemoryInternal(x, (UINT8 *) __FILE__, (UINT32) __LINE__, TRUE)
#define GCAllocateMemoryNoClear(x) GCAllocateMemoryInternal(x, (UINT8 *) __FILE__, (UINT32) __LINE__, FALSE)
#define GCReallocMemory(x, y) GCReallocMemoryInternal(x, y, (UINT8 *) __FILE__, (UINT32) __LINE__)

extern void GCFreeMemoryNoClear(void *pvMemoryLocation);
extern void AppMain(UINT8 *pu8CmdLine);
extern void *GCAllocateMemoryInternal(UINT32 u32Size, UINT8 *pu8ModuleName, UINT32 u32LineNumber, BOOL bClearBlock);
extern void *GCReallocMemoryInternal(void *pvBlock, UINT32 u32Size, UINT8 *pu8ModuleName, UINT32 u32LineNumber);
extern void GCCheckHeapIntegrity(void);
extern void BlockReport(const char* in_pName);
extern EGCResultCode GCGetBlockSize(void *pvBlock,
									UINT32 *pu32BlockSize);
extern void GCGetFreeInfo(UINT32 *pu32LargestFree,
						  UINT32 *pu32TotalFree);
extern void GCGetAllocInfo(UINT32 *pu32LargestAlloc,
						   UINT32 *pu32TotalAlloc);

extern void GCSetReturnCode( UINT32 u32ReturnCode );

#ifdef _WIN32
extern void GCDropAnchor(void);
extern void GCRaiseAnchor(void);
#endif

#define PRODUCT_ID				0x12345678

#include "Startup/Profile.h"
#include "Application/RSC68k.h"

#endif	// #ifndef _APP_H_
