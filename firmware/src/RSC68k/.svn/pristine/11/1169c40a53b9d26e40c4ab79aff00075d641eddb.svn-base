/* RSC68k Assembly language routine support
 *
 * 68000 Calling conventions:
 *
 * d0-d1 and a0-a1 are scratch registers
 * d2-d7 and a2-a7 are assumed to be preserved by the callee
 *
 */

#include "../Hardware/RSC68k.h"
#include "../Shared/16550.h"

	.title "AsmUtils.s"

	.global	MoveMultipleWrite
	.global	MoveMultipleRead
	.global	IDESectorMoveRead
	.global	SRSet
	.global	SRGet

/* Back up stack pointer 6 bytes - 4 for the return address and 2 for the word param of the SR value */

SRSet:
	movel	%sp, %a0
	addl	#6, %a0
	movew	(%a0), %d0
	movew	%d0, %sr
	rts

SRGet:
	movew	%sr, %d0
	rts

MoveMultipleWrite:
	moveml	%d2-%d7/%a2,-(%sp)
	moveml	%d0-%d7,-(%a2)
	moveml	(%sp)+,%d2-%d7/%a2
	rts

MoveMultipleRead:
	moveml	%d2-%d7/%a2,-(%sp)
	moveml	(%a2)+, %d0-%d7
	moveml	(%sp)+,%d2-%d7/%a2
	rts

/*

void IDESectorMoveRead(void *pvTargetBuffer,
		       uint32_t u32EndAddress);

*/

IDESectorMoveRead:
	moveml	%d2-%d7/%a2-%a3,-(%sp)

/* pvTargetBuffer == sp + 4 (rtn addrss) + 24 (data registers) + 8 (address registers) */

	movel	%sp, %a0
	addl	#4+24+8, %a0
	movel	(%a0), %a2

/* u32EndAddress ==  sp + 4 (rtn addrss) + 24 (data registers) + 8 (address registers) + 4 (pvTargetBuffer) */
	addl	#4, %a0
	movel	(%a0), %a3

	lea	RSC68KHW_DEVCOM_IDE_CSA, %a0
	movel	#32, %a1

/* a0 Is our IDE_CSA address. 
   a1 Is 32 - the # of bytes we move at a time
   a2 Is the destination
   a3 Is the end address */

sectorLoop:
	moveml	(%a0), %d0-%d7
	moveml	%d0-%d7,(%a2)
	addal	%a1, %a2

	moveml	(%a0), %d0-%d7
	moveml	%d0-%d7,(%a2)
	addal	%a1, %a2

	moveml	(%a0), %d0-%d7
	moveml	%d0-%d7,(%a2)
	addal	%a1, %a2

	moveml	(%a0), %d0-%d7
	moveml	%d0-%d7,(%a2)
	addal	%a1, %a2

	moveml	(%a0), %d0-%d7
	moveml	%d0-%d7,(%a2)
	addal	%a1, %a2

	moveml	(%a0), %d0-%d7
	moveml	%d0-%d7,(%a2)
	addal	%a1, %a2

	moveml	(%a0), %d0-%d7
	moveml	%d0-%d7,(%a2)
	addal	%a1, %a2

	moveml	(%a0), %d0-%d7
	moveml	%d0-%d7,(%a2)
	addal	%a1, %a2

	moveml	(%a0), %d0-%d7
	moveml	%d0-%d7,(%a2)
	addal	%a1, %a2

	moveml	(%a0), %d0-%d7
	moveml	%d0-%d7,(%a2)
	addal	%a1, %a2

	moveml	(%a0), %d0-%d7
	moveml	%d0-%d7,(%a2)
	addal	%a1, %a2

	moveml	(%a0), %d0-%d7
	moveml	%d0-%d7,(%a2)
	addal	%a1, %a2

	moveml	(%a0), %d0-%d7
	moveml	%d0-%d7,(%a2)
	addal	%a1, %a2

	moveml	(%a0), %d0-%d7
	moveml	%d0-%d7,(%a2)
	addal	%a1, %a2

	moveml	(%a0), %d0-%d7
	moveml	%d0-%d7,(%a2)
	addal	%a1, %a2

	moveml	(%a0), %d0-%d7
	moveml	%d0-%d7,(%a2)
	addal	%a1, %a2

	cmpl	%a2, %a3
	bne	sectorLoop

	moveml	(%sp)+,%d2-%d7/%a2-%a3
	rts

