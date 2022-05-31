#ifndef _LCDERR_H_
#define _LCDERR_H_

#include "Startup/app.h"

typedef enum
{
	LERR_OK = 0,										// GCErrorCode base
	LERR_GC_ERR_BASE=LERR_OK,							// Good to go

	LERR_RESOLUTION_NOT_SUPPORTED=GC_RESOLUTION_NOT_SUPPORTED,
	LERR_BIT_DEPTH_NOT_SUPPORTED=GC_BIT_DEPTH_NOT_SUPPORTED,
	LERR_MONITOR_TYPE_UNKNOWN=GC_MONITOR_TYPE_UNKNOWN,
	LERR_VIDEO_MODE_NOT_SET=GC_VIDEO_MODE_NOT_SET,
	LERR_NO_INT_SLOTS_AVAILABLE=GC_NO_INT_SLOTS_AVAILABLE,
	LERR_NO_TIMER_SLOTS_AVAILABLE=GC_NO_TIMER_SLOTS_AVAILABLE,
	LERR_OUT_OF_MEMORY=GC_OUT_OF_MEMORY,
	LERR_GC_INVALID_MEMORY_ALLOC_SIZE=GC_INVALID_MEMORY_ALLOC_SIZE,
	LERR_NO_FREE_BLOCKS_LARGE_ENOUGH=GC_NO_FREE_BLOCKS_LARGE_ENOUGH,
	LERR_BLOCK_NOT_IN_HEAP=GC_BLOCK_NOT_IN_HEAP,
	LERR_SAMPLE_RATE_NOT_SUPPORTED=GC_SAMPLE_RATE_NOT_SUPPORTED,
	LERR_OUT_OF_BIOS_MEMORY=GC_OUT_OF_BIOS_MEMORY,
	LERR_SOUND_NOT_INITIALIZED=GC_SOUND_NOT_INITIALIZED,
	LERR_ZIP_FILE_NOT_FOUND=GC_ZIP_FILE_NOT_FOUND,
	LERR_FILE_WITHIN_ZIP_NOT_FOUND=GC_FILE_WITHIN_ZIP_NOT_FOUND,
	LERR_ZIP_FILE_INTEGRITY_ERROR=GC_ZIP_FILE_INTEGRITY_ERROR,
	LERR_ZIP_ITEM_OPEN_ERROR=GC_ZIP_ITEM_OPEN_ERROR,
	LERR_NO_DATA=GC_NO_DATA,
	LERR_ERASE_FAILURE=GC_ERASE_FAILURE,
	LERR_PROGRAM_ERROR=GC_PROGRAM_ERROR,
	LERR_NO_MODE_SET=GC_NO_MODE_SET,
	LERR_ILLEGAL_ADDRESS=GC_ILLEGAL_ADDRESS,
	LERR_OUT_OF_RANGE=GC_OUT_OF_RANGE,
	LERR_DRIVE_NOT_FOUND=GC_DRIVE_NOT_FOUND,
	LERR_UNKNOWN_IMAGE_TYPE=GC_UNKNOWN_IMAGE_TYPE,
	LERR_NOT_SUPPORTED=GC_NOT_SUPPORTED,
	LERR_NOT_INITIALIZED=GC_NOT_INITIALIZED,
	LERR_EVENT_NOT_FOUND=GC_EVENT_NOT_FOUND,
	LERR_NO_FILESYSTEM=GC_NO_FILESYSTEM,

	// SD/MMC card related errors
	LERR_CARD_TIMEOUT=GC_CARD_TIMEOUT,
	LERR_RESPONSE_TIMEOUT=GC_RESPONSE_TIMEOUT,
	LERR_CARD_ERROR=GC_CARD_ERROR,
	LERR_MMC_ALWAYS_BUSY=GC_MMC_ALWAYS_BUSY,
	LERR_NO_CARD=GC_NO_CARD,
	LERR_DEVICE_NOT_AVAILABLE=GC_DEVICE_NOT_AVAILABLE,
	LERR_MEDIUM_CHANGE=GC_MEDIUM_CHANGE,

	// Controller related
	LERR_TRACKBALL_OUT_OF_RANGE=GC_TRACKBALL_OUT_OF_RANGE,
	LERR_IR_NOT_PRESENT=GC_IR_NOT_PRESENT,
	LERR_DIAG_NOT_ACTIVE=GC_DIAG_NOT_ACTIVE,

	// Misc
	LERR_COUNTER_OUT_OF_RANGE=GC_COUNTER_OUT_OF_RANGE,
	LERR_PRODUCT_ID_NOT_SET=GC_PRODUCT_ID_NOT_SET,
	LERR_INVALID_PRODUCT_ID=GC_INVALID_PRODUCT_ID,
	LERR_SERIAL_PORT_OUT_OF_RANGE=GC_SERIAL_PORT_OUT_OF_RANGE,
	LERR_NO_ROOM_IN_XMIT_BUFFER=GC_NO_ROOM_IN_XMIT_BUFFER,
	LERR_BUS_LOCKED=GC_BUS_LOCKED,
	LERR_BUS_NOT_LOCKED=GC_BUS_NOT_LOCKED,
	LERR_BAD_BAUD_RATE=GC_BAD_BAUD_RATE,
	LERR_RTC_NOT_AVAILABLE=GC_RTC_NOT_AVAILABLE,
	LERR_I2C_BUS_OUT_OF_RANGE=GC_I2C_BUS_OUT_OF_RANGE,
	LERR_I2C_SLAVE_NAK=GC_I2C_SLAVE_NAK,
	LERR_I2C_TRANSACTION_TRUNCATED=GC_I2C_TRANSACTION_TRUNCATED,
	LERR_I2C_STOP_FAULT=GC_I2C_STOP_FAULT,
	LERR_I2C_BUS_IDLE=GC_I2C_BUS_IDLE,
	LERR_I2C_BUS_FAULT=GC_I2C_BUS_FAULT,

	// Operating system result codes
	LERR_ERR_EVENT_TYPE=GC_ERR_EVENT_TYPE,
	LERR_ERR_PENDING_ISR=GC_ERR_PENDING_ISR,
	LERR_ERR_POST_NULL_PTR=GC_ERR_POST_NULL_PTR,
	LERR_ERR_PEVENT_NULL=GC_ERR_PEVENT_NULL,
	LERR_ERR_POST_ISR=GC_ERR_POST_ISR,
	LERR_ERR_QUERY_ISR=GC_ERR_QUERY_ISR,
	LERR_ERR_INVALID_OPT=GC_ERR_INVALID_OPT,
	LERR_ERR_TASK_WAITING=GC_ERR_TASK_WAITING,
	LERR_TIMEOUT=GC_TIMEOUT,
	LERR_TASK_NOT_EXIST=GC_TASK_NOT_EXIST,
	LERR_MBOX_FULL=GC_MBOX_FULL,
	LERR_Q_FULL=GC_Q_FULL,
	LERR_PRIO_EXIST=GC_PRIO_EXIST,
	LERR_PRIO_ERR=GC_PRIO_ERR,
	LERR_PRIO_INVALID=GC_PRIO_INVALID,
	LERR_SEM_OVF=GC_SEM_OVF,
	LERR_TASK_DEL_ERR=GC_TASK_DEL_ERR,
	LERR_TASK_DEL_IDLE=GC_TASK_DEL_IDLE,
	LERR_TASK_DEL_REQ=GC_TASK_DEL_REQ,
	LERR_TASK_DEL_ISR=GC_TASK_DEL_ISR,
	LERR_NO_MORE_TCB=GC_NO_MORE_TCB,
	LERR_TIME_NOT_DLY=GC_TIME_NOT_DLY,
	LERR_TIME_INVALID_MINUTES=GC_TIME_INVALID_MINUTES,
	LERR_TIME_INVALID_SECONDS=GC_TIME_INVALID_SECONDS,
	LERR_TIME_INVALID_MILLI=GC_TIME_INVALID_MILLI,
	LERR_TIME_ZERO_DLY=GC_TIME_ZERO_DLY,
	LERR_TASK_SUSPEND_PRIO=GC_TASK_SUSPEND_PRIO,
	LERR_TASK_SUSPEND_IDLE=GC_TASK_SUSPEND_IDLE,
	LERR_TASK_RESUME_PRIO=GC_TASK_RESUME_PRIO,
	LERR_TASK_NOT_SUSPENDED=GC_TASK_NOT_SUSPENDED,
	LERR_MEM_INVALID_PART=GC_MEM_INVALID_PART,
	LERR_MEM_INVALID_BLKS=GC_MEM_INVALID_BLKS,
	LERR_MEM_INVALID_SIZE=GC_MEM_INVALID_SIZE,
	LERR_MEM_NO_FREE_BLKS=GC_MEM_NO_FREE_BLKS,
	LERR_MEM_FULL=GC_MEM_FULL,
	LERR_MEM_INVALID_PBLK=GC_MEM_INVALID_PBLK,
	LERR_MEM_INVALID_PMEM=GC_MEM_INVALID_PMEM,
	LERR_MEM_INVALID_PDATA=GC_MEM_INVALID_PDATA,
	LERR_MEM_INVALID_ADDR=GC_MEM_INVALID_ADDR,
	LERR_ERR_NOT_MUTEX_OWNER=GC_ERR_NOT_MUTEX_OWNER,
	LERR_TASK_OPT_ERR=GC_TASK_OPT_ERR,
	LERR_ERR_DEL_ISR=GC_ERR_DEL_ISR,
	LERR_ERR_CREATE_ISR=GC_ERR_CREATE_ISR,
	LERR_FLAG_INVALID_PGRP=GC_FLAG_INVALID_PGRP,
	LERR_FLAG_ERR_WAIT_TYPE=GC_FLAG_ERR_WAIT_TYPE,
	LERR_GC_FLAG_ERR_NOT_RDY=GC_FLAG_ERR_NOT_RDY,
	LERR_GC_FLAG_INVALID_OPT=GC_FLAG_INVALID_OPT,
	LERR_GC_FLAG_GRP_DEPLETED=GC_FLAG_GRP_DEPLETED,
	LERR_OS_ERROR_UNKNOWN=GC_OS_ERROR_UNKNOWN,
	LERR_OS_NOT_RUNNING=GC_OS_NOT_RUNNING,
	LERR_OUT_OF_SEMAPHORES=GC_OUT_OF_SEMAPHORES,
	LERR_GC_OUT_OF_QUEUES=GC_OUT_OF_QUEUES,
	LERR_GC_MUTEX_LOCKED=GC_MUTEX_LOCKED,
	LERR_GC_MUTEX_UNLOCKED=GC_MUTEX_UNLOCKED,
	LERR_GC_ERR_PEND_LOCKED=GC_ERR_PEND_LOCKED,
	LERR_GC_QUEUE_EMTPY=GC_QUEUE_EMTPY,

	// Pointer related problems
	LERR_POINTER_NOT_AVAILABLE=GC_POINTER_NOT_AVAILABLE,
	LERR_POINTER_NOT_SUPPORTED=GC_POINTER_NOT_SUPPORTED,
	LERR_POINTER_NOT_CALIBRATED=GC_POINTER_NOT_CALIBRATED,

	// Unimplemented function
	LERR_NOT_IMPLEMENTED=GC_NOT_IMPLEMENTED,

	// Image related
	LERR_CORRUPT_IMAGE=GC_CORRUPT_IMAGE,
	LERR_PARTIAL_IMAGE=GC_PARTIAL_IMAGE,
	LERR_IMAGE_READ_TRUNCATED=GC_IMAGE_READ_TRUNCATED,

	// File related
	LERR_ENOENT=(GC_FILE_BASE+2),

	// GPIO Errors
	LERR_GPIO_INDEX_INVALID=GC_GPIO_INDEX_INVALID,		// Invalid GPIO index
	LERR_GPIO_CFG_FIELD_INVALID=GC_GPIO_CFG_FIELD_INVALID,	// Invalid field set in GPIOs
	LERR_GPIO_NOT_AN_OUTPUT=GC_GPIO_NOT_AN_OUTPUT,		// Attempted to set a GPIO that's not an output
	
	// Generic errors

	LERR_GENERIC_BASE=0x1000000,						// Base of generic errors
	LERR_NO_MEM=LERR_GENERIC_BASE,						// Out of memory
	LERR_INTERNAL_ERROR,								// Internal fault
	LERR_FORMAT_TRUNCATED,								// fprintf truncation
	LERR_NO_FORMAT,										// Format string contains no formatting parameters
	LERR_FORMAT_TOO_MANY_PARAMETERS,					// Too many ; separators
	LERR_FORMAT_TRUNCATED_ESCAPE_SEQUENCE,				// Escape sequence character immediately prior to '\0'
	LERR_FORMAT_ILLEGAL_VARIABLE_TYPE,					// Not a string and not numeric
	LERR_FORMAT_UNTERMINATED_STRING,					// String not terminated
	LERR_DIGIT_OVERFLOW,								// Too many significant digits for the format provided
	LERR_BAD_HANDLE,									// Unknown handle - period
	LERR_STRING_BAD_START_POSITION,						// Replace() bad start position
	LERR_STRING_INVALID_COUNT,							// Bad count for Replace() string operation
	LERR_WIDGET_FUNCTION_NOT_SUPPORTED,					// Widget type does not support this function

	// Time related errors

	LERR_INVALID_YEAR,									// Year is <1970
	LERR_INVALID_MONTH,									// Month isn't 0-11
	LERR_INVALID_DAY,									// Bad day
	LERR_INVALID_HOUR,									// Bad hour >23
	LERR_INVALID_MINUTE,								// Bad minute >59
	LERR_INVALID_SECOND,								// Bad second
	LERR_INVALID_FORMAT,								// Invalid date/time format

	// Windowing errors

	LERR_WIN_BASE=0x2000000,							// Base of windowing errors
	LERR_WIN_BACKGROUND_ALREADY_DEFINED=LERR_WIN_BASE,	// Background image already defined
	LERR_WIN_BAD_HANDLE,								// Invalid handle
	LERR_WIN_INVALID_LAYER,								// Attempt to hook up with an invalid (nonexistent) layer
	LERR_WIN_NO_BACKGROUND_IMAGE,						// No background image
	LERR_WIN_INVALID_STYLE,								// Style field is invalid
	LERR_WIN_INVALID_ORIGIN,							// Origin field is invalid
	LERR_WIN_FULL,										// No more windows can be created (no more slots)
	LERR_WIN_WIDGET_NOT_CONNECTED,						// Widget not connected to the window in question
	LERR_WIN_NO_PRIORITY,								// Can't figure out the window's priority (internal bug)

	// File errors
	LERR_FILE_BASE=0x3000000,							// Base of file based errors
	LERR_FILE_NOT_FOUND=LERR_FILE_BASE,					// File not found
	LERR_FILE_BAD_HANDLE,								// Bad file handle
	LERR_FILE_INCOMPLETE_READ,							// Incomplete read on file
	LERR_FILE_UNKNOWN_DEVICE_TYPE,						// Unknown file type
	LERR_FILE_FULL,										// No more handles
	LERR_FILE_INVALID_ACTION,							// Function not supported on this file stream
	LERR_FILE_FIND_INVALID_FIELD,						// Bad field in the invalid field function
	LERR_FILE_FIND_END,									// No more files that match this
	LERR_FILE_INCOMPLETE_WRITE,							// Incomplete write on file
	LERR_FILE_REWIND_PAST_BEGINNING,					// Attempt to rewind past beginning of file
	LERR_FILE_UNTERMINATED_STRING,						// End of file reached before closing double quote encountered
	LERR_FILE_SEEK_PARAM_INVALID,						// Seek parameter invalid
	LERR_FILE_END,										// End of file
	LERR_FILE_REWIND_INVALID,							// Can't rewind that much on this stream
	LERR_FILE_LINE_TOO_LONG,							// String/line too long
	LERR_FILE_CSV_EXTRA_IGNORED,						// CSV - There's extra data on the end of line during a CSV read
	LERR_FILE_CSV_INCOMPLETE_DATA,						// CSV - Incomplete data in source file (not all variables assigned)
	LERR_FILE_CSV_FIELD_OVERFLOW_SIZE,					// CSV - Data in source file too big to fit in variable
	LERR_FILE_CSV_FIELD_SIGN,							// CSV - Attempted to read an unsigned quantity into a signed variable
	LERR_FILE_CSV_FIELD_FP_MISMATCH,					// CSV - Attempted to load a floating point value into an integer quantity
	LERR_FILE_CSV_FIELD_NOT_NUMERIC,					// CSV - Attempted to read a string into a numeric field
	LERR_FILE_CSV_UNTERMINATED_STRING,					// CSV - Unterminated string
	LERR_FILE_CSV_FLOAT_LOSS,							// CSV - double->float - loss of precision
	LERR_FILE_BAD_FILE_MODE,							// Bad file mode

	// Font errors
	LERR_FONT_OK=0x4000000,								// Base of all font based errors
	LERR_FONT_BASE=LERR_FONT_OK,						// Font... good... zog understand.
	LERR_FONT_BAD_HANDLE,								// Bad font handle
	LERR_FONT_CHAR_NONEXISTENT,							// Character nonexistent
	LERR_FONT_INCOMPLETE_READ,							// Incomplete read while accessing the font file
	LERR_FONT_FILE_TOO_LARGE,							// Font file is ridiculously large
	LERR_FONT_SIZE_TOO_SMALL,							// Font size too small
	LERR_FONT_FULL=LERR_FONT_OK+0x100000,				// Font slots all full-up

	// Console errors
	LERR_CONSOLE_FULL=0x5000000,						// Console slots are full-up
	LERR_CONSOLE_BASE=LERR_CONSOLE_FULL,
	LERR_CONSOLE_EMPTY_FONT,							// Font doesn't have anything in them
	LERR_CONSOLE_BAD_X_SIZE,							// No characters can fit in the font's X size
	LERR_CONSOLE_BAD_Y_SIZE,							// No characters can fit in the font's Y size
	LERR_CONSOLE_BAD_HANDLE,							// Bad console handle
	LERR_CONSOLE_BAD_ROTATION,							// Bad rotation parameter

	// Button errors
	LERR_BUTTON_FULL=0x6000000,							// Button slots are full-up
	LERR_BUTTON_BAD_HANDLE,								// Bad button handle
	LERR_BUTTON_NO_NORMAL_IMAGE,						// No normal image to set a hit mask to
	LERR_BUTTON_NORMAL_IMAGE_NO_MASK,					// No transparency/translucency mask
	LERR_BUTTON_DISABLED,								// Button is disabled (state can't be obtained)
	LERR_BUTTON_SIZE_TOO_SMALL,							// Button size is too small
	LERR_BUTTON_TEXT_WONT_FIT,							// Button text won't fit in the area given
	LERR_BUTTON_NOT_ANIMATABLE,							// One or both button images are not animatable
	LERR_BUTTON_NO_INDICATOR_SET,						// No indicator image set

	// Wave errors
	LERR_SOUND_WAVE_FULL=0x70000000,					// Wave slots are full-up
	LERR_SOUND_WAVE_NO_CHANNEL,
	LERR_SOUND_WAV_NOT_A_WAV,
	LERR_SOUND_UNSUPPORTED_BITS,
	LERR_SOUND_FILE_NOT_FOUND,
	LERR_SOUND_MALLOC_FAIL,
	LERR_SOUND_INIT_FAIL,
	LERR_SOUND_IO_ERROR,
	LERR_SOUND_BAD_SAMPLERATE,
	LERR_SOUND_PRIORITY,
	LERR_SOUND_SHUTTING_DOWN,
	LERR_SOUND_WAVE_BAD_HANDLE,							// Bad wave handle
	LERR_SOUND_VOLUME_INVALID,
	LERR_SOUND_BAD_CHANNEL,								// Bad channel pointer

	// Video errors
	LERR_VIDEO_FULL=0x8000000,							// All video slot handles are full
	LERR_VIDEO_FILE_NOT_FOUND,							// Can't find video file
	LERR_VIDEO_FORMAT_UNKNOWN,							// Don't know the video's format
	LERR_VIDEO_BAD_HANDLE,								// Bad video handle
	LERR_VIDEO_ALREADY_ACTIVE,							// Video already playing
	LERR_VIDEO_NOT_ATTACHED,							// Video not attached to a window
	LERR_VIDEO_NOT_ACTIVE,								// Video not playing
	LERR_VIDEO_LOOP_ACTIVE,								// Video is looping - can't wait until done
	LERR_VIDEO_FILE_TOO_LARGE,							// Video file size is >=4GB
	LERR_VIDEO_IMAGE_TOO_LARGE,							// Video image won't fit on the screen
	LERR_VIDEO_FILE_NOT_SET,							// No video file set
	LERR_VIDEO_BAD_POSITION,							// Bad position
	LERR_VIDEO_AUDIO_STREAM_UNKNOWN,					// Unknown audio stream
	LERR_VIDEO_FRAME_NOT_FOUND,							// Frame # not found
	LERR_VIDEO_EMPTY_PAGE,								// Page has nothing in it
	LERR_VIDEO_AUDIO_CHANNELS_NOT_SUPPORTED,			// # Of audio channels not supported in video
	LERR_VIDEO_AUDIO_BIT_DEPTH_NOT_SUPPORTED,			// Audio bit depth not supported

	// Text errors
	LERR_TEXT_FULL=0x9000000,							// All text slot handles are full
	LERR_TEXT_BAD_HANDLE,								// Bad text handle
	LERR_TEXT_BAD_ROTATION,								// Bad rotation parameter

	// Image errors
	LERR_IMAGE_FULL=0xa000000,							// All image slot handles are full
	LERR_IMAGE_BAD_HANDLE,								// Bad image handle
	LERR_IMAGE_NOT_ANIMATABLE,							// Not animatable (only 1 frame)
	LERR_IMAGE_TARGET_TOO_SMALL,						// Can't fit this into the provided image
	LERR_IMAGE_DECOMPRESSION_ERROR,						// Decompression error while loading the image
	LERR_IMAGE_BIT_DEPTH_NOT_SUPPORTED,					// Bit depth not supported for this image type
	LERR_IMAGE_UNKNOWN_FILE_TYPE,						// Unknown image file type
	LERR_IMAGE_NO_FRAMES,								// Image has nothing in it

	// Graph errors
	LERR_GRAPH_FULL=0xb000000,							// All graph slot handles are full
	LERR_GRAPH_SERIES_FULL,								// Graph series full
	LERR_GRAPH_BAD_HANDLE,								// Bad graph handle
	LERR_GRAPH_SERIES_BAD_HANDLE,						// Bad graph series handle
	LERR_GRAPH_BAD_OFFSET,								// Bad offset
	LERR_GRAPH_BAD_SIZE,								// Bad size
	LERR_GRAPH_TYPE_NOT_NUMERIC,						// Type isn't numeric, hoser
	LERR_GRAPH_NOT_ENOUGH_ELEMENTS,						// Element count is < 1
	LERR_GRAPH_SERIES_TYPE_MISMATCH,					// Inserted type != graph series type
	LERR_GRAPH_BAD_ORIGIN,								// Bad origin
	LERR_GRAPH_MIN_LARGER_THAN_MAX,						// Min/max mismatch
	LERR_GRAPH_BAD_ROTATION,							// Bad rotation parameter

	// Network errors
	LERR_NET_ARP_NO_RESPONSE = 0xc000000,				// No response when ARPing for IP address
	LERR_NET_TX_PACKET_TOO_LARGE,						// Can't fit outgoing packet into temp buffer
	LERR_NET_CONFIG_BAD_NV,								// Invalid network configuration image found
	LERR_NET_CONFIG_RESET,								// Network NV configuration reset to defaults						
	LERR_NET_ICMP_RX_PACKET_TOO_SMALL,					// ICMP Packet received is too small
	LERR_NET_ICMP_UNKNOWN_PROTOCOL,						// Unknown ICMP protocol type
	LERR_NET_TCP_SOCKET_CLOSED,							// ICMP reflection TCP socket closed
	LERR_NET_TCP_CONNECT_NOT_ESTABLISHED,				// ICMP reflection TCP socket available but not established
	LERR_NET_UNKNOWN_PROTOCOL,							// Unknown protocol (for service unavailable)
	LERR_NET_NO_DATA,									// No data when function called
	LERR_NET_SOCKET_INVALID,							// Invalid socket handle
	LERR_NET_INVALID_IP_SETTING,						// Bad IP/Netmask setting
	LERR_NET_UNKNOWN_DEVICE_NAME,						// Bad device name
	LERR_NET_UNKNOWN_INTERFACE_TYPE,					// Interface type unknown
	LERR_NET_NO_ROUTE,									// Can't route packet - no interface
	LERR_NET_DNS_SERVER_NOT_CONFIGURED,					// Attempt to DNS lookup when no DNS server configured
	LERR_NET_ROUTE_NOT_PRESENT,							// Can't delete a route that isn't there
	LERR_NET_NTP_BAD_TIME_POINTER,						// NULL Time pointer passed to NTP retrieval
	LERR_NET_NTP_BAD_SERVER,							// NULL Server pointer passed to NTP retrieval
	LERR_NET_CONNECTION_NOT_CLOSED,						// When trying to do a passive connect via TCP
	LERR_NET_TCP_CONNECTION_TIMEOUT,					// TCP Connection timed out
	LERR_NET_CLOSE_ALREADY_IN_PROGRESS,					// Closing connection in progress already
	LERR_NET_NO_CONNECTION_PRESENT,						// Attempting to close a connection on a socket
	LERR_NET_TIMEOUT,									// Timeout during network operation
	LERR_NET_BAD_SOCKET_OPTION,							// Bad socket option passed in
	LERR_NET_TCP_PASSIVE_OPEN_LISTEN,					// Can't turn a passive session into a listen socket
	LERR_NET_NO_ROUTE_TO_HOST,							// Can't get to destination network
	LERR_NET_NO_MAC_ADDRESS,							// MAC Address not set
	LERR_NET_DHCP_ADDRESS_NOT_SET,						// DHCP Address not set
	LERR_NET_UNKNOWN_DIRECTIVE,							// Bogus command found in network FileOpen
	LERR_NET_TCP_ALREADY_SPECIFIED,						// Attempt to define more than one port/protocol (TCP already specified)
	LERR_NET_UDP_ALREADY_SPECIFIED,						// Attempt to define more than one port protocol (UDP already specified)
	LERR_NET_PORT_OUT_OF_RANGE,							// Port # out of range
	LERR_NET_UDP_EQUALS_EXPECTED,						// UDP Specified, equals expected (parse error)
	LERR_NET_UDP_PORT_NUMBER_EXPECTED,					// UDP Specified, port number expected
	LERR_NET_UDP_PORT_NUMBER_OVERFLOW,					// UDP Specified, port number beyond 32 bits
	LERR_NET_TCP_EQUALS_EXPECTED,						// TCP Specified, equals expected (parse error)
	LERR_NET_TCP_PORT_NUMBER_EXPECTED,					// TCP Specified, port number expected
	LERR_NET_TCP_PORT_NUMBER_OVERFLOW,					// TCP Specified, port number beyond 32 bits
	LERR_NET_DESTINATION_ALREADY_SET,					// Destination IP/name already specified
	LERR_NET_SEPARATOR_EXPECTED,						// Separator expected somewhere in the open string but wasn't seen
	LERR_NET_DESTINATION_EQUALS_EXPECTED,				// Equals expected in destination separator
	LERR_NET_PORT_NOT_SPECIFIED,						// Someone forgot to enter a port!
	LERR_NET_DESTINATION_NOT_SPECIFIED,					// Trying to write to a port but no destination present
	LERR_NET_UDP_NOT_BIDIRECTIONAL,						// Can't open UDP in r/w mode
	LERR_NET_CONFIG_MODE_ALREADY_SPECIFIED,				// Attempt to define more than one configuration mode
	LERR_NET_CONFIG_STATICIP_ALREADY_SPECIFIED,			// Attempt to define more than one static IP
	LERR_NET_CONFIG_NETMASKIP_ALREADY_SPECIFIED,		// Attempt to define more than one netmask IP
	LERR_NET_CONFIG_ROUTEIP_ALREADY_SPECIFIED,			// Attempt to define more than one route IP
	LERR_NET_CONFIG_DNS_ALREADY_SPECIFIED,				// Attempt to define more than one DNS IP
	LERR_NET_CONFIG_SYSLOG_ALREADY_SPECIFIED, 			// Attempt to define more than one syslog destination
	LERR_NET_CONFIG_MODE_EQUALS_EXPECTED,				// Mode, equals expected (parse error)
	LERR_NET_STATICIP_EQUALS_EXPECTED,					// Static IP, equals expected (parse error)
	LERR_NET_STATICIP_NUMBER_EXPECTED,					// Static IP, number expected (parse error)
	LERR_NET_STATICIP_NUMBER_OVERFLOW,					// Static IP, beyond 32 bits (parse error)
	LERR_NET_STATICIP_PERIOD_EXPECTED,					// Static IP, period expected (parse error)
	LERR_NET_NETMASKIP_EQUALS_EXPECTED,					// Netmask IP, equals expected (parse error)
	LERR_NET_NETMASKIP_NUMBER_EXPECTED,					// Netmask IP, number expected (parse error)
	LERR_NET_NETMASKIP_NUMBER_OVERFLOW,					// Netmask IP, beyond 32 bits (parse error)
	LERR_NET_NETMASKIP_PERIOD_EXPECTED,					// Netmask IP, period expected (parse error)
	LERR_NET_ROUTEIP_EQUALS_EXPECTED,					// Route IP, equals expected (parse error)
	LERR_NET_ROUTEIP_NUMBER_EXPECTED,					// Route IP, number expected (parse error)
	LERR_NET_ROUTEIP_NUMBER_OVERFLOW,					// Route IP, beyond 32 bits (parse error)
	LERR_NET_ROUTEIP_PERIOD_EXPECTED, 					// Route IP, period expected (parse error)
	LERR_NET_DNSIP_EQUALS_EXPECTED,						// DNS IP, equals expected (parse error)
	LERR_NET_DNSIP_NUMBER_EXPECTED,						// DNS IP, number expected (parse error)
	LERR_NET_DNSIP_NUMBER_OVERFLOW,						// DNS IP, beyond 32 bits (parse error)
	LERR_NET_DNSIP_PERIOD_EXPECTED, 					// DNS IP, period expected (parse error)
	LERR_NET_FTP_INVALID_USER_ID,
	LERR_NET_FTP_INVALID_USERNAME,
	LERR_NET_FTP_INVALID_PASSWORD,
	LERR_NET_FTP_INVALID_HOMEDIR,
	LERR_NET_FTP_INVALID_PERMISSIONS,
	LERR_NET_FTP_PERMISSION_ALREADY_SPECIFIED,

	// Slider errors
	LERR_SLIDER_FULL=0xd000000,							// No more slider slots full
	LERR_SLIDER_BAD_HANDLE,								// Bad slider handle
	LERR_SLIDER_TRACK_TOO_SMALL,						// Track too small
	LERR_SLIDER_BAD_ROTATION,							// Slider orientation bogus

	// Radio group handles
	LERR_RADIO_FULL=0xe000000,							// No more radio group handles
	LERR_RADIO_BAD_HANDLE,								// Bad radio handle
	LERR_RADIO_ITEM_OUT_OF_RANGE,						// Radio item out of range
	LERR_RADIO_ITEM_DISABLED,							// Radio item disabled
	LERR_RADIO_SELECTED_IMAGE_SIZE_MISMATCH,			// Selected/not selected sizes mismatch
	LERR_RADIO_OVERLAP,									// Item overlaps with another item
	LERR_RADIO_SELECTED_IMAGE_MISSING,					// Selected image missing
	LERR_RADIO_NONSELECTED_IMAGE_MISSING,				// Nonselected image missing

	// Combo box handles
	LERR_COMBOBOX_FULL=0xe000000,						// No more combo box handles
	LERR_COMBOBOX_BAD_HANDLE,
	LERR_COMBOBOX_INDEX_NOT_FOUND,						// Can't find the index for this item

	// Terminal handlers
	LERR_TERMINAL_BAD_HANDLE=0xe005000,					// Terminal bad handle

	// NOTE: All error codes need to exactly match the radio group error codes in
	// the sequence they appear above

	LERR_CHECKBOX_FULL=0xf000000,						// No more checkbox handles
	LERR_CHECKBOX_BAD_HANDLE,							// Bad checkbox handle
	LERR_CHECKBOX_OUT_OF_RANGE,							// Checkbox item out of range (not used)
	LERR_CHECKBOX_DISABLED,								// Checkbox item disabled
	LERR_CHECKBOX_SELECTED_IMAGE_SIZE_MISMATCH,			// Selected/not selected sizes mismatch
	LERR_CHECKBOX_OVERLAP,								// Item overlaps with another item (not used)
	LERR_CHECKBOX_CHECKED_IMAGE_MISSING,				// Checked image missing
	LERR_CHECKBOX_UNCHECKED_IMAGE_MISSING,				// Unchecked image missing
	LERR_CHECKBOX_INVALID_STYLE,						// Invalid style provided
	LERR_CHECKBOX_SIZE_TOO_SMALL,						// Checkbox size too small!

	// Sound stream
	LERR_SOUNDSTREAM_INIT_FAILED = 0x10000000,			// Initialization failed
	LERR_SOUNDSTREAM_UNKNOWN_FORMAT,					// File format (ie MP3) is unknown
	LERR_SOUNDSTREAM_NOT_ACTIVE,						// A sound stream is not playing
	LERR_SOUNDSTREAM_LOOP_ACTIVE,						// Operation can not be performed when in loop mode
	LERR_SOUNDSTREAM_ALREADY_ACTIVE,					// The stream is already playing
	LERR_SOUNDSTREAM_NO_STREAM_SET,						// The sound stream has not been set (ie to a file)
	LERR_SOUNDSTREAM_INACCURATE_RESULT,					// Unable to obtain accurate result (ie MP3 VBR)
	LERR_SOUNDSTREAM_POSITION_OUT_OF_RANGE,				// The specific position is out of range
	LERR_SOUNDSTREAM_FRAME_NOT_FOUND,					// The specified frame (ie for MP3) is not found

	// Touch screen
	LERR_TOUCH_BAD_HANDLE = 0x11000000,					// Bad touch widget handle

	// NVStore
	LERR_NVSTORE_BAD_KEY_NAME = 0x12000000,				// Bad key name - either sys: or NULL
	LERR_NVSTORE_KEY_NOT_FOUND,							// NVStore key not found
	LERR_NVSTORE_DATA_TOO_BIG,							// Data too big for destination
	LERR_NVSTORE_KEY_NAME_TOO_LONG,						// Key name too big
	LERR_NVSTORE_HARDWARE_NVSTORE_UNAVAILABLE,			// Hardware layer too small
	LERR_NVSTORE_OUT_OF_STORAGE,						// Not enough data available to store record
	LERR_NVSTORE_MALFORMED_KEY_VALUE,					// Internal consistency check problem
	LERR_NVSTORE_DATA_TYPE_MISMATCH,					// Provided parameters != value stored in NVStore key
	LERR_NVSTORE_PARAMETER_COUNT_MISMATCH,				// Provided parameter count != value stored count
	LERR_NVSTORE_INDEX_OUT_OF_RANGE,					// No more items

	// Serial related
	LERR_SERIAL_BAUD_RATE_EXPECTED_EQUALS = 0x13000000,	// Expecting '=' after baud rate
	LERR_SERIAL_BAUD_RATE_EXPECTED_POSITIVE,			// Expecting positive # after baud rate
	LERR_SERIAL_BAUD_RATE_EXPECTED_NUMBER,				// Expecting a number after the baud rate
	LERR_SERIAL_BAUD_RATE_INVALID,						// Invalid baud rate

	LERR_SERIAL_DATA_BITS_EXPECTED_EQUALS,				// Expecting equals after data bits
	LERR_SERIAL_DATA_BITS_EXPECTED_POSITIVE,			// Expecting positive number after equals
	LERR_SERIAL_DATA_BITS_EXPECTED_NUMBER,				// Expecting a number after equals
	LERR_SERIAL_DATA_BITS_INVALID,						// Bad # of data bits

	LERR_SERIAL_STOP_BITS_EXPECTED_EQUALS,				// Expecting equals after stop bits
	LERR_SERIAL_STOP_BITS_EXPECTED_POSITIVE,			// Expecting positive number after equals
	LERR_SERIAL_STOP_BITS_EXPECTED_NUMBER,				// Expecting a number after equals
	LERR_SERIAL_STOP_BITS_INVALID,						// Bad # of stop bits

	LERR_SERIAL_TXBUF_EXPECTED_EQUALS,					// Expecting equals after transmitter buffer size
	LERR_SERIAL_TXBUF_EXPECTED_POSITIVE,				// Expecting positive number after equals
	LERR_SERIAL_TXBUF_EXPECTED_NUMBER,					// Expecting a number after equals

	LERR_SERIAL_RXBUF_EXPECTED_EQUALS,					// Expecting equals after receive buffer size
	LERR_SERIAL_RXBUF_EXPECTED_POSITIVE,				// Expecting positive number after equals
	LERR_SERIAL_RXBUF_EXPECTED_NUMBER,					// Expecting a number after equals

	LERR_SERIAL_PARITY_INVALID,							// Parity setting invalid
	LERR_SERIAL_FLOW_CONTROL_INVALID,					// Flow control setting invalid
	LERR_SERIAL_FLOW_EXPECTED_EQUALS,					// Expected equals after flow keyword
	LERR_SERIAL_PARITY_EXPECTED_EQUALS,					// Expected equals after parity keyword

	LERR_SERIAL_FIELD_SEPARATOR_EXPECTED,				// Not a , or end of string
	LERR_SERIAL_PORT_ALREADY_OPEN,						// Port already open

	// Animation related
	LERR_ANIMATION_TYPE_INVALID = 0x14000000,			// Animation field is bogus
	LERR_ANIMATION_FRAME_INVALID,						// Animation frame invalid

	// Widget related
	LERR_WIDGET_CALLBACK_NOT_FOUND = 0x14000000,		// Callback registration not found (widget activity callback)

	// Line edit related
	LERR_LINEEDIT_BAD_HANDLE = 0x15000000,				// Line edit routines (bad handle)
	LERR_LINEEDIT_NO_SEMAPHORE,							// Unable to allocate widget semaphore

	// Don't move this
	LERR_MAX
} ELCDErr;

// Used globally to indicate an invalid handle
#define	HANDLE_INVALID			0xffffffff

#endif