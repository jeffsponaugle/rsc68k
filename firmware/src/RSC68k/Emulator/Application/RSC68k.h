#ifndef _RSC68KEMU_H_
#define _RSC68KEMU_H_

#include <windows.h>
#ifdef _DEBUG
//#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

// Needed for SHA256
#include <stdint.h>

typedef unsigned short int UINT16;
typedef unsigned char UINT8;
typedef signed short int INT16;
typedef signed char INT8;
typedef unsigned long long int UINT64;
typedef signed long long int INT64;

// Lexical character!
typedef unsigned char LEX_CHAR;


// Memory allocation/related
#define	MemAlloc(x)			RuntimeAllocateMemoryInternal(x, (UINT8 *) __FILE__, __LINE__, TRUE)
#define	MemAllocNoClear(x)	RuntimeAllocateMemoryInternal(x, (UINT8 *) __FILE__, __LINE__, FALSE)
#define MemRealloc(x, y)	RuntimeReallocMemoryInternal(x, y, (UINT8 *) __FILE__, __LINE__)
#define MemFree(x)			GCFreeMemory(x)

// Routines dealing with memory allocation
extern void *RuntimeAllocateMemoryInternal(UINT64 u64Size,
										   UINT8 *pu8ModuleName,
										   UINT32 u32LineNumber,
										   BOOL bClearBlock);
extern void *RuntimeReallocMemoryInternal(void *pvOldBlock,
										  UINT64 u64Size,
										  UINT8 *pu8ModuleName,
										  UINT32 u32LineNumber);
extern void GCFreeMemory(void *pvMemoryLocation);

// Assert related stuff
#define	GCASSERT(expr)	if (!(expr)) { AssertHandler((UINT8 *) #expr, (UINT8 *) __FILE__, (UINT32) __LINE__); }
#define	GCASSERT_MSG(expr)	AssertHandler((UINT8 *) expr, (UINT8 *) __FILE__, (UINT32) __LINE__)
#define GCASSERT_WHY(expr, why)	if (!(expr)) { AssertWhyHandler((UINT8 *) #expr, (char *) why, (UINT8 *) __FILE__, (UINT32) __LINE__); }

extern void AssertHandler(UINT8 *pu8Expression, UINT8 *pu8Module, UINT32 u32Line);
extern void AssertWhyHandler(UINT8 *pu8Expression, char *pu8Why, UINT8 *pu8Module, UINT32 u32Line);

// Diagnostic stuff
extern void DebugOut(const char *pu8Format, ...);
extern void DebugOutFuncInternal(const char *pu8Procedure, const char *pu8Format, ...);
#define	DebugOutFunc(x, ...)	DebugOutFuncInternal(__FUNCTION__, x, __VA_ARGS__)
extern void DumpSetToConsole(BOOL bDump);

// Unicode/string functions
extern LEX_CHAR *Lexstrndup(const LEX_CHAR *peString,
							UINT32 u32MaxLength);
extern LEX_CHAR Lextoupper(LEX_CHAR eChar);
extern LEX_CHAR Lextolower(LEX_CHAR eChar);
extern void LexStripWhitespace(LEX_CHAR *peString);
extern void Lexstrncpy(LEX_CHAR *peDest,
					   LEX_CHAR *peSrc,
					   UINT32 u32Length);
extern INT32 Lexstrncmp(LEX_CHAR *peString1,
					    LEX_CHAR *peString2,
						UINT32 u32Loop);
extern INT32 Lexstrnicmp(LEX_CHAR *peString1,
					     LEX_CHAR *peString2,
						 UINT32 u32Loop);
extern void Lexstrcpy(LEX_CHAR *peDest,
					  LEX_CHAR *peSrc);
extern void Lexstrcat(LEX_CHAR *peDest,
					  LEX_CHAR *peSrc);
extern void Lexstrncat(LEX_CHAR *peDest,
					   LEX_CHAR *peSrc,
					   UINT32 u32Length);
extern UINT32 Lexstrlen(const LEX_CHAR *peString);
extern INT32 Lexstrcmp(LEX_CHAR *peString1,
					   LEX_CHAR *peString2);
extern LEX_CHAR *Lexstrdup(const LEX_CHAR *peString);
extern LEX_CHAR *Lexstrcasestr(LEX_CHAR *peStringToSearch,
							   LEX_CHAR *peSubString);
extern LEX_CHAR *Lexstrcasestrrev(LEX_CHAR *peStringToSearch,
								  LEX_CHAR *peSubString,
								  UINT32 u32StartPosition);
extern LEX_CHAR *Lexstrstr(LEX_CHAR *peStringToSearch,
						   LEX_CHAR *peSubString);
extern int Lexstrcasecmp(const LEX_CHAR *s1,
						 const LEX_CHAR *s2);
extern BOOL Lexstrdupsafe(LEX_CHAR **ppeTargetString,
						  LEX_CHAR *peSourceString);
extern void LexASCIIToUnicode(char *pu8SourceString,
							  LEX_CHAR *peTargetString);
extern void LexUnicodeToASCII(char *pu8ASCIIString,
							  LEX_CHAR *peString);
extern LEX_CHAR *LexASCIIToUnicodeAlloc(char *pu8SourceString);
extern char *LexUnicodeToASCIIAlloc(LEX_CHAR *peString);
extern char *ASCIIstrdup(char *pu8String);

extern void ThreadSetName(char *pu8ThreadName);
extern void ThreadSetNameprintf(char *pu8Format, ...);

#define	Lexstristr Lexstrcasestr

// Text handles
typedef UINT32 TEXTHANDLE;

// Button handles
typedef UINT32 BUTTONHANDLE;

// Graph handles
typedef UINT32 GRAPHHANDLE;

// Graph queue handles
typedef UINT32 GRAPHSERIESHANDLE;

// Image handle
typedef UINT32 IMAGEHANDLE;

// Wave handles
typedef UINT32 SOUNDHANDLE;

// Window handles
typedef UINT32 WINDOWHANDLE;

// File handles
typedef UINT32 FILEHANDLE;

// Video handles
typedef UINT32 VIDEOHANDLE;

// Slider handles
typedef UINT32 SLIDERHANDLE;

// Radio handles
typedef UINT32 RADIOGROUPHANDLE;

// Checkbox handles
typedef UINT32 CHECKBOXGROUPHANDLE;

// Touch region handle
typedef UINT32 TOUCHREGIONHANDLE;

// Console handle
typedef UINT32 CONSOLEHANDLE;

// Combo box handle
typedef UINT32 COMBOBOXHANDLE;

// Line edit handle
typedef UINT32 LINEEDITHANDLE;

// Widget handle
typedef UINT32 WIDGETHANDLE;

// Terminal handle
typedef UINT32 TERMINALHANDLE;

// Variable type enumerations

typedef enum 
{
	/*******************************************************/
	// DO NOT CHANGE THE ORDER OF THESE TYPES!

	EVAR_SIGNED_CHAR=0,				// Signed char
	EVAR_CONST_BEGIN=EVAR_SIGNED_CHAR, // Beginning of constants
	EVAR_UNSIGNED_CHAR=1,			// Unsigned char
	EVAR_SIGNED_SHORT=2,			// Signed short
	EVAR_UNSIGNED_SHORT=3,			// Unsigned short
	EVAR_SIGNED_INT=4,				// Signed int
	EVAR_UNSIGNED_INT=5,			// Unsigned int
	EVAR_SIGNED_LONG_INT=6,			// Signed long int
	EVAR_UNSIGNED_LONG_INT=7,		// Unsigned long int
	EVAR_FLOAT=8,					// Single precision floating point
	EVAR_DOUBLE=9,					// Double precision floating point
	EVAR_BOOLEAN=10,				// Boolean
	EVAR_STRING=11,					// String variable
	EVAR_CONST_END=EVAR_STRING,		// End of constants

	EVAR_BUTTONHANDLE,				// Button handle
	EVAR_HANDLE_START=EVAR_BUTTONHANDLE, // Start of all handles
	EVAR_WINDOWHANDLE,				// Window handle
	EVAR_FILEHANDLE,				// File handle
	EVAR_SOUNDHANDLE,				// Sound handle
	EVAR_TEXTHANDLE,				// Text handle
	EVAR_IMAGEHANDLE,				// Image handle
	EVAR_GRAPHHANDLE,				// Graph handle
	EVAR_GRAPHSERIESHANDLE,			// Graph queue handle
	EVAR_VIDEOHANDLE,				// Video handle
	EVAR_SLIDERHANDLE,				// Slider handle
	EVAR_RADIOGROUPHANDLE,			// Radio group handle
	EVAR_CHECKBOXGROUPHANDLE,		// Checkbox handle
	EVAR_TOUCHREGIONHANDLE,			// touch region handle
	EVAR_CONSOLEHANDLE,				// Console handle
	EVAR_HANDLE_END=EVAR_CONSOLEHANDLE, // <--- MUST POINT TO THE ITEM ABOVE THIS!!! ADD TO v

	EVAR_NULL,						// NULL node

	EVAR_UNSPECIFIED,				// Used for inheritance
	EVAR_INVALID,					// Shows that no one is using it
	EVAR_VARIABLE,					// Used for variable number of parameters to procedures
	EVAR_VARTYPE,					// Variable type

	// Leave this at the end
	EVAR_TYPE_COUNT,
	EVAR_TERMINATOR=EVAR_TYPE_COUNT
} EVarType;

// Union for all variable types
union UValue
{
	BOOL bBoolean;					// Boolean
	INT8 s8Char;					// Signed char
	UINT8 u8Char;					// Unsigned char
	INT16 s16Short;					// Signed short
	UINT16 u16Short;				// Unsigned short
	INT32 s32Int;					// Signed int
	UINT32 u32Int;					// Unsigned int
	INT64 s64LongInt;				// Signed long int
	UINT64 u64LongInt;				// Unsigned long int
	float fFloat;					// Floating point
	double dDouble;					// Double precision floating point
	LEX_CHAR *peString;				// String
	time_t eTime;					// Time/date literal
	BUTTONHANDLE eButtonHandle;		// Button handle
	WINDOWHANDLE eWindowHandle;		// Window handle
	SOUNDHANDLE eSoundHandle;		// Sound handle
	TEXTHANDLE eTextHandle;			// Text handle
	FILEHANDLE eFileHandle;			// File handle
	IMAGEHANDLE eImageWidgetHandle;	// Image handle
	GRAPHHANDLE eGraphHandle;		// Graph widget handle
	GRAPHSERIESHANDLE eGraphSeriesHandle;	// Graph series handle
	VIDEOHANDLE eVideoHandle;		// Video handle
	SLIDERHANDLE eSliderHandle;		// Slider handle
	RADIOGROUPHANDLE eRadioGroupHandle;	// Radio group handle
	CHECKBOXGROUPHANDLE eCheckboxGroupHandle;	// Check box handle
	TOUCHREGIONHANDLE eTouchRegionHandle;	// Touch region handle
	CONSOLEHANDLE eConsoleHandle;	// Console handle
	WIDGETHANDLE eWidgetHandle;		// Generic widget handle
	EVarType eVarType;				// Variable type
	void *pvArrayData;				// Array data, if applicable
};

// # Of array dimensions we can have
#define	MAX_ARRAY_DIMENSIONS		3

typedef struct SVar
{
	// Must update VarCopy() if anything is added

	EVarType eType;					// Variable's type
	UINT8 u8Scope;					// Variable's scope
	LEX_CHAR *peVarName;			// Variable's name (if applicable)
	UINT8 u8Dimensions;				// # Of dimensions for this variable (0=Single variable)
//	struct SExpr *psExpressions[MAX_ARRAY_DIMENSIONS];	// Pointer to expressions that evaluate the array sizes
	UINT32 u32DimensionSize[MAX_ARRAY_DIMENSIONS];		// Size of each dimension (in # of elements)
	union UValue uValue;			// Pointer to our variable's value (needs to be since they can be arrays)
	union UValue *puValuePtr;		// Pointer to actual variable value storage location
	struct SVar *psNextLink;		// Pointer to next variable in the chain
} SVar;

#include "Application/LCDErr.h"

#define RETURN_ON_FAIL(eErr) { if ((eErr != LERR_OK)) { return ((eErr)); } }
#define ERROREXIT_ON_FAIL(eErr) { if ((eErr != LERR_OK)) { goto errorExit; } }
#define GOTO_ON_FAIL(eErr, x) { if ((eErr != LERR_OK)) { goto x; } }

#define RETURN_ON_NULL(pVal, x) { if ((NULL == (pVal))) { return ((x)); } }
#define ERROREXIT_ON_NULL(pVal) { if ((NULL == (pVal))) { goto errorExit; } }
#define GOTO_ON_NULL(pVal, x) { if ((NULL == (pVal))) { goto x; } }

#define RETURN_ON_FALSE(pVal, x) { if ((FALSE == (pVal))) { return ((x)); } }
#define ERROREXIT_ON_FALSE(pVal) { if ((FALSE == (pVal))) { goto errorExit; } }
#define GOTO_ON_FALSE(pVal, x) { if ((FALSE == (pVal))) { goto x; } }

#define	LEX_ASCII8(x) ((LEX_CHAR *) x)

#define	CONVERT_24RGB_16RGB(x) (((x >> 8) & 0xf800) | ((x >> 5) & 0x07e0) |(((x >> 3) & 0x1f)))

extern BOOL IsNumericType(EVarType eVarType);
extern UINT32 VarGetTypeSize(EVarType eVarType);
extern char *AppGetArchiveFilesDir(void);
extern void GOSPlatformInit(void);
extern void AppEntry(void *pvData);

#endif