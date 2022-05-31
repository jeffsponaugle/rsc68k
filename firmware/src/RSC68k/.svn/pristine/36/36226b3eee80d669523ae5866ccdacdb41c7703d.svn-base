/******************************************************************************
**
**  COPYRIGHT (C) 2000 Intel Corporation
**
**  FILENAME:      systypes.h
**
**  PURPOSE:       Contains common type definitions used in this project
**
**  $Modtime: 3/29/01 9:13a $
**
******************************************************************************/

#ifndef _SYSTYPES_H
#define _SYSTYPES_H

/*
*******************************************************************************

    Portable Integral Type Aliases and associated pointer types
    - Must be verified for all compiler ports.

    The underlying size of one of these data types may be larger than implied
    by its name.  The underlying types here assume the ARM ADS 1.01 compiler,
    and in that case the sizes ares exactly as implied.

    INT64 and UINT64 types are permitted but not standardized by ANSI C.
    Their existence and behavior are implementation-dependent.

    Some information relative to UINT64 or INT64 types in the ARM compilers:
    "The following restrictions apply to long long:
     ·long long enumerators are not available.
     ·The controlling expression of a switch statement can not have
        (unsigned) long long type.  Consequently case labels must
        also have values that can be contained in a variable of
        type unsigned long."

*******************************************************************************
*/

#ifndef _WIN32
typedef enum {FALSE=0, TRUE=1} BOOL;
typedef enum {NOERROR=0, ERROR=1} ERRORTYPE;
#endif

typedef void(*FnPVOID)(void);

typedef unsigned int        UINT,     *PUINT;    // The size is not important
typedef unsigned long long  UINT64,   *PUINT64;
typedef unsigned int        UINT32,   *PUINT32;
typedef unsigned short      UINT16,   *PUINT16;
typedef unsigned char       UINT8,    *PUINT8;
typedef unsigned char       UCHAR,*PUCHAR;

#ifndef _BYTE_DEFINED
#define _BYTE_DEFINED
typedef unsigned char       BYTE;
#endif

typedef int                 INT,      *PINT;    // The size is not important
typedef long long           INT64,    *PINT64;
typedef int                 INT32,    *PINT32;
typedef short               INT16,    *PINT16;
typedef char                INT8,     *PINT8;
typedef char                CHAR,     *PCHAR;

#ifndef _WIN32
typedef void                VOID,     *PVOID;
#endif

typedef volatile  UINT      VUINT,    *PVUINT;    // The size is not important
typedef volatile  UINT64    VUINT64,  *PVUINT64;
typedef volatile  UINT32    VUINT32,  *PVUINT32;
typedef volatile  UINT16    VUINT16,  *PVUINT16;
typedef volatile  UINT8     VUINT8,   *PVUINT8;
typedef volatile  UCHAR     VUCHAR,   *PVUCHAR;

typedef volatile  INT       VINT,     *PVINT;    // The size is not important
typedef volatile  INT64     VINT64,   *PVINT64;
typedef volatile  INT32     VINT32,   *PVINT32;
typedef volatile  INT16     VINT16,   *PVINT16;
typedef volatile  INT8      VINT8,    *PVINT8;
typedef volatile  CHAR      VCHAR,    *PVCHAR;

#endif // #ifndef _SYSTYPES_H
