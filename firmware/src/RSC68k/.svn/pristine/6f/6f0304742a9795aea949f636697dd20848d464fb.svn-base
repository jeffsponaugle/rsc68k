	AREA	Init, CODE, READONLY
	CODE32

	IMPORT	GCStartup
	EXPORT	startupEntry
	ENTRY

;	r0 = Base of application area
;	r1 = Size of application area
;	r2 = Command line pointer
;	r3 = Where stack should go
;	r4 = Size of stack

startupEntry
	mov		sp, r3
	stmdb	sp!, {r0-r2,lr}			; Save for later

; ------ BEGIN ARM SDT INITIALIZATION ------

	; Import load addresses
	IMPORT |Image$$ZI$$Base|
	IMPORT |Image$$ZI$$Limit|

_InitZeroData

	LDR		r1,	=|Image$$ZI$$Base|	; Get start	address
	LDR		r2,	=|Image$$ZI$$Limit|	; Get size

	LDR		r5,	=0
	CMP		r1,	r2
	BEQ		_InitData_End

_InitZeroData_Loop
	CMP		r1,	r2					; Set flags	if this	is the last	one
	STRNE	r5,	[r1], #4			; Write	0 to r1	address, advance r1
	BCC		_InitZeroData_Loop		; Continue on

_InitData_End

; ------ END ARM SDT INITIALIZATION ------

	ldmia	sp!, {r0-r2, lr}			; Restore R0-R2, and link registers

; Let's compute the base of the heap start and put it in R0. Then
; Figure out how much space is left 

; r0=Base of free RAM after code, zero init, and initialized RW data
; r1=Length of free RAM minus app stack and BIOS area
; r2=Command line passed to program

	IMPORT	|Image$$RW$$Limit|
	IMPORT	|Image$$RO$$Base|

	ldr		r6, =|Image$$RW$$Limit| + 31
	ldr		r7, =0xffffffe0
	and		r6, r7, r6

	mov		r0, r6

	mov		r1, sp
	sub		r1, r1, r4					; Get final value in r1 - subtract off the stack
	sub		r1, r1, r0					; Subtract off the top of our RW limit to get the available memory

	b		GCStartup

	END