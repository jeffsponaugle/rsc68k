#ifndef _PROFILE_H_
#define _PROFILE_H_

#include "Startup/app.h"

// Overall control of profiling
#ifdef _WIN32
#define	PROFILING_ENABLED	0
#else
#define PROFILING_ENABLED	0
#endif

// If profiling is enabled, what pieces are?

// Parsing
#define	PROFILING_PARSE_ENABLED		1
// Lexing
#define	PROFILING_LEX_ENABLED		1

extern void ProfileInit(void);
extern void ProfileContextEnter(char *pu8ContextName,
								char *pu8ModuleName,
								UINT32 u32LineNumber);
extern void ProfileContextExit(char *pu8ContextName,
							   char *pu8ModuleName,
							   UINT32 u32LineNumber);
extern void ProfileReport(char *pu8Filename);

#if PROFILING_ENABLED

#if PROFILING_PARSE_ENABLED
#define PROFPARSEENTER()			ProfileContextEnter(__FUNCTION__, __FILE__, (UINT32) __LINE__)
#define PROFPARSEEXIT()				ProfileContextExit(__FUNCTION__, __FILE__, (UINT32) __LINE__)
#define PROFPARSEENTERTAG(x)		ProfileContextEnter(x, __FILE__, (UINT32) __LINE__)
#define PROFPARSEEXITTAG(x)			ProfileContextExit(x, __FILE__, (UINT32) __LINE__)
#else
#define PROFPARSEENTER()
#define PROFPARSEEXIT()
#define PROFPARSEEXITTAG(x)
#define PROFPARSEENTERTAG(x)
#endif

#if PROFILING_LEX_ENABLED
#define PROFLEXENTER()				ProfileContextEnter(__FUNCTION__, __FILE__, (UINT32) __LINE__)
#define PROFLEXEXIT()				ProfileContextExit(__FUNCTION__, __FILE__, (UINT32) __LINE__)
#else
#define PROFLEXENTER()
#define PROFLEXEXIT()
#endif

#else
#define PROFPARSEENTER()
#define PROFPARSEEXIT()
#define PROFLEXENTER()
#define PROFLEXEXIT()
#define PROFPARSEEXITTAG(x)
#define PROFPARSEENTERTAG(x)
#endif

#endif