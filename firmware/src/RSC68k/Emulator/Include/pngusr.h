#ifndef _PNGUSR_H_
#define _PNGUSR_H_

#define PNG_NO_WRITE_SUPPORTED				1		// Don't care about writes
#define PNG_NO_FLOATING_POINT_SUPPORTED		1		// Assume no floating point
#define PNG_SETJMP_NOT_SUPPORTED			1		// We're not using SETJMP
#define PNG_USER_MEM_SUPPORTED				1		// Must use user memory allocation/deallocation
#define PNG_NO_CONSOLE_IO

#endif // #ifndef _PNGUSR_H_
