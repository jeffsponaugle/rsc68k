/* -*- tab-width: 4 -*- */

// For the curious, tabs are set to 1, 5, 9, etc... Do not modify and resubmit
// with spaces. It makes the code harder to merge in changes.

#ifndef _TYPES_H_
#define _TYPES_H_

// Include standard headers that we need - common files only!

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <ctype.h>

#ifndef UINT32
typedef unsigned long int		UINT32;
#endif // UINT32

#ifndef INT32
typedef signed long	int			INT32;
#endif // INT32

// Various definitions that we may or may not need to define

#ifndef UINT16
typedef unsigned short int		UINT16;
#endif // UINT16

#ifndef UINT8
typedef unsigned char			UINT8;
#endif // UINT 8

#ifndef INT16
typedef signed short int		INT16;
#endif // INT16

#ifndef INT8
typedef signed char				INT8;
#endif // INT8

#ifndef INT64
typedef signed long long int INT64;
#endif // INT64

#ifndef UINT64
typedef unsigned long long int UINT64;
#endif // UINT64

#define TCHAR UINT8

// Get rid of TRUE/FALSE so we can bind them to enumeration to prevent them
// from being used imporperly

#ifdef TRUE
#undef TRUE
#endif

#ifdef FALSE
#undef FALSE
#endif

#ifdef BOOL
#undef BOOL
#endif

typedef enum
{ 
	FALSE = 0x0,
	TRUE  = 0x1
} BOOL;

#define BOOL_DEFINED

#endif // _TYPES_H_
