   AREA Graphics, CODE, READONLY  ; name this block of code
   CODE32
   
	EXPORT  ARMBlitTransparent
	EXPORT	ARMBlit8Bit
	EXPORT	ARMGfxDirtyBufferBlit

;
; Fast code to handle transparent blits
; call from C as ARMBlitTransparent(unsigned long u32loop, unsigned char *pu8TranslucentMask, unsigned short *pu16Src, unsigned short *pu16Dest);
;
ARMBlitTransparent
      stmfd	sp!,{r4-r8,lr}   ; preserve all that get used
      tst   r1,#3             ; see if any of the pointers are not dword aligned
      bne   bot_blit          ; if so, need to do it 1 pixel at a time
      tst   r2,#2
      bne   bot_blit
      tst   r3,#2
      bne   bot_blit
      mov   r4,r0,LSR #2      ; see how many groups of 4 pixels we can do
      and   r0,r0,#3          ; get remaining pixels in r0
      mov   r14,#0x30         ; prepare a comparison value for "all transparent"
      orr   r14,r14,r14,LSL #8
      orr   r14,r14,r14,LSL #16
      mov   r8,#0x30000000    ; for single pixel comparison
top_blit
      ldr   r5,[r1]           ; get 4 mask bytes
; stick these 2 instructions here to not waste the pipeline stall time
      orrs  r4,r4,r4          ; any more goups of 4?
      beq   bot_blit          ; do any stragler pixels
      and   r5,r5,r14         ; we only have 2 transparency levels (all or none)
      cmp   r5,r14            ; all transparent?
      beq   skip_blit         ; we can skip all 4
      ldmia r2,{r6,r7}        ; read source data if not transparent
      orrs  r5,r5,r5          ; draw all 4?
      stmeqia r3,{r6,r7}      ; store 4 opaque pixels
      beq   skip_blit         ; skip the slower logic
      cmp   r8,r5,LSL #24     ; first pixel transparent?
      strgth r6,[r3,#0]
      mov   r6,r6,LSR #16
      cmp   r8,r5,LSL #16     ; second pixel transparent?
      strgth r6,[r3,#2]
      cmp   r8,r5,LSL #8      ; third pixel transparent?
      strgth r7,[r3,#4]
      mov   r7,r7,LSR #16
      cmp   r8,r5             ; forth pixel transparent?
      strgth r7,[r3,#6]
skip_blit
      sub   r4,r4,#1          ; decrement count
      add   r1,r1,#4          ; advance 4 mask bytes
      add   r2,r2,#8          ; advance 8 source bytes
      add   r3,r3,#8          ; advance 8 dest bytes
      b     top_blit
; do "odd" pixels
bot_blit
      ldrb  r5,[r1],#1        ; get mask byte
      orrs  r0,r0,r0          ; any single pixels to do?
      beq   exit_blit         ; time to go
      cmp   r5,#0x30          ; transparent?
      ldrlth r6,[r2]          ; grab a source pixel
      sub   r0,r0,#1          ; make use of wasted cycle
      strlth r6,[r3]          ; store it in the destination
      add   r2,r2,#2          ; advance source
      add   r3,r3,#2          ; advance dest
      b     bot_blit          ; loop through all pixels
exit_blit
      ldmia sp!,{r4-r8,pc}   ; restore all and leave
;
; Draw a variable sized 8bpp image onto a 16bpp bitmap with transparency and palette translation
;
;	r0 = srcwidth
;	r1 = srcheight
;  r2 = srcpitch
;	r3 = pSrc
;	sp[0] = pDest
;  sp[4] = destpitch
;	sp[8] = pPalette
;  sp[12] = transparent color
;
; reg vars: r5 = source data, r6 = dest, r7 = pitch, r8 = color table, r9 = trans mask, r10 = #1
;			r4 = line counter, r11 = #0xf, r12,r14 = temp
;
; void ARMBlit8Bit(int iSrcWidth, int iSrcHeight, int iSrcPitch, void *pSrc, void *pDest, int iDestPitch, unsigned short *pPalette, unsigned char ucTransparent)

ARMBlit8Bit
	stmfd	sp!,{r4-r12,lr}
	ldr     r5,[sp,#40]  ; pDest
	ldr     r7,[sp,#52]  ; get the transparent color
	ldr     r14,[sp,#44] ; dest pitch
	mov     r11,#0xff    ; pixel extract mask
	mov     r11,r11,LSL #1  ; need 0x1fe constant (ARM can't do odd constant shifts)
	ldr     r6,[sp,#48]	; get the palette
	mov     r7,r7,LSL #1 ; shift over for faster comparison
	sub     r5,r5,#2     ; pre-decrement dest pointer to speed things up
ARMBlit0
	mov     r9,r3        ; temp source pointer
	mov     r12,r5       ; temp dest pointer
	mov     r4,r0        ; number of pixels to draw
	tst     r9,#3        ; dword aligned source?
	beq     ARMBlitAligned
; take care of the non-aligned pixels first
ARMBlitOdd8
	ldrb    r8,[r9],#1   ; get a single pixel
	orrs    r4,r4,r4     ; anything to draw?
	beq     ARMBlitleftover
	mov     r8,r8,LSL #1
	cmp     r8,r7        ; transparent?
	ldrneh  r10,[r6,r8]  ; translate the color
	sub     r4,r4,#1     ; decrement pixel count
	add     r12,r12,#2   ; increment dest pointer to avoid pipeline stall
	strneh  r10,[r12]    ; draw the opaque pixel
	tst     r9,#3        ; loop here until we get to the dword-aligned portion
	bne     ARMBlitOdd8
ARMBlitAligned
	movs    r8,r4,LSR #2 ; anything groups of 4 to draw?
	beq     ARMBlitleftover
ARMBlit1   
	ldr     r8,[r9],#4   ; grab 4 source pixels
	and     r10,r11,r8,LSL #1  ; get first pixel
	cmp     r10,r7       ; transparent?
	ldrneh  r10,[r6,r10] ; translate the color
	add     r12,r12,#2     ; increment dest pointer to avoid pipe stall   
	strneh  r10,[r12]     ; draw the opaque pixel
	and     r10,r11,r8,LSR #7  ; get second pixel
	cmp     r10,r7       ; transparent?
	ldrneh  r10,[r6,r10] ; translate the color
	add     r12,r12,#2     ; inc dest ptr
	strneh  r10,[r12]     ; draw the opaque pixel
	and     r10,r11,r8,LSR #15 ; get third pixel
	cmp     r10,r7       ; transparent?
	ldrneh  r10,[r6,r10] ; translate the color
	add     r12,r12,#2     ; inc dest
	strneh  r10,[r12]     ; draw the opaque pixel
	and     r10,r11,r8,LSR #23 ; get third pixel
	cmp     r10,r7       ; transparent?
	ldrneh  r10,[r6,r10] ; translate the color
	add     r12,r12,#2
	strneh  r10,[r12]     ; draw the opaque pixel
	sub     r4,r4,#4     ; decrement count
	cmp     r4,#4
	bge     ARMBlit1     ; inner loop
ARMBlitleftover
	orrs    r4,r4,r4     ; any remaining pixels?
	beq     ARMBlitNextLine
ARMBlit2
	ldrb    r8,[r9],#1
	add     r12,r12,#2
	mov     r8,r8,LSL #1
	cmp     r8,r7        ; transparent?
	ldrneh  r10,[r6,r8]
	strneh  r10,[r12]
	subs    r4,r4,#1
	bne     ARMBlit2
ARMBlitNextLine
	add     r5,r5,r14    ; move to next line of dest
	add     r3,r3,r2     ; move to next line of source
	subs    r1,r1,#1     ; decrement Y
	bne     ARMBlit0
	ldmfd	sp!,{r4-r12,pc}

pu8DirtyBufferBase   EQU  0
u8PixelShift         EQU  4
u8LineShift          EQU  5
u32XSurfaceSize      EQU  8
u32YSurfaceSize      EQU  12
pu16SourceSurface    EQU  16
u32SourceSurfacePitch EQU  20
pu16TargetSurface    EQU  24
u32TargetSurfacePitch   EQU  28
;
; Test and blit the "dirty" buffer
;
; void ARMGfxDirtyBufferBlit(SDirtyBuffer *psDirtyBuffer)
ARMGfxDirtyBufferBlit
	stmfd	sp!,{r4-r12,lr}
	ldr     r1,[r0,#u32YSurfaceSize]     ; Y count in pixels
	ldr     r4,[r0,#u32XSurfaceSize]     ; X count in pixels
	ldrb    r2,[r0,#u8PixelShift]    ; get shift value
	mov     r3,#1
	mov     r3,r3,LSL R2 ; get shift amount as a count
	mov     r1,r1,LSR R2 ; get the Y count in units
	mov     r4,r4,LSR R2 ; get the X count in units   
	mov     r12,#0		; current Y position
ARMDirty0	; top of Y loop
	ldr     r8,[r0,#pu8DirtyBufferBase]	; source dirty ptr
	ldrb    r9,[r0,#u8LineShift]	; get dirty buffer line shift
	mov     r5,#0		; current x position
	mov     r6,#0		; length of dirty section
	mov     r10,r12,LSL R9	; calculate offset to this dirty line
	add     r8,r8,r10	; now R8 points to the correct spot in the dirty buffer
	mov     r10,#0    ; keep a zero to clear old dirty sections that we've scanned
ARMDirty_fast	; top of X loop
	ldr     r7,[r8],#4	; test 4 "blocks" for changed bytes
	add     r5,r5,#4	; skip ahead 4 in X direction (and make use of pipe stall)
	orrs    r7,r7,r7
	beq     ARMDirty2bot	; not dirty, keep scanning
; we have at least 1 dirty block, switch to byte-by-byte scanning
	sub     r8,r8,#4
	sub     r5,r5,#4  ; pull X back by 4 also
ARMDirty_slow
	ldrb    r7,[r8],#1
	add     r5,r5,#1	; advance X
	orrs    r7,r7,r7	; dirty?
	andeqs  r9,r5,#3	; see if we're dword aligned again
	beq     ARMDirty2bot	; re-enter faster scanning loop or we could have hit the end of line
	orrs    r7,r7,r7	; dirty?
	strneb  r10,[r8,#-1]  ; clear dirty flag as we scan
	addne   r6,r6,#1	; yes, add to dirty count
	beq     ARMDirty_slow	; keep scanning for start of dirty section
; we've hit a dirty section, check for end of line
	cmp     r5,r4		; end of the line?
	beq     ARMDirty_slow1  ; dirty section is only a single block
; we've hit the beginning of the dirty section
ARMDirty_slow0			; scan for dirty blocks
	ldrb    r7,[r8],#1
	add     r5,r5,#1	; update X position to not go off end
	orrs    r7,r7,r7	; still dirty?
	subeq   r8,r8,#1  ; pull back 1 to keep the starting point correct
	subeq   r5,r5,#1
	beq     ARMDirty_slow1	; end of dirty section
	strb    r10,[r8,#-1]  ; clear dirty flag as we scan
	add     r6,r6,#1	; yes, add to length and keep going
	cmp     r5,r4		; end of line?
	bne     ARMDirty_slow0
; We've come to the end of the dirty section
; or the end of the line
ARMDirty_slow1
; prepare to copy the dirty section	
	stmfd   sp!,{r0-r5,r8,r12}		; save scratch registers
	ldr     r7,[r0,#u32SourceSurfacePitch] ; calc starting addr
	ldr     r9,[r0,#pu16SourceSurface]	; image source ptr
	mov     r7,r7,LSL #1    ; get pitch in bytes
	mov     r10,r7,LSL R2	; shift it the correct number of lines
	mul     r1,r10,r12		; times the current Y position
	add     r9,r9,r1
	sub     r10,r5,r6		; starting X
	mov     r10,r10,LSL R2	; shift into pixels
	mov     r10,r10,LSL #1	; 16-bit pixels
	add     r9,r9,r10		; now R9 points to source pixel
	ldr     r8,[r0,#u32TargetSurfacePitch] ; calc dest addr
	ldr     r11,[r0,#pu16TargetSurface] ; image dest ptr
	mov     r8,r8,LSL #1    ; get pitch in bytes
	mov     r14,r8,LSL R2	; shift it the correct number of lines
	mul     r1,r14,r12		; times the current Y position
	add	  r11,r11,r1		; add Y offset
	add	  r10,r11,r10		; add X offset in 16-bit words
; now copy the multi-line section from src to dest
	mov     r6,r6,LSL R2	; number of pixels to copy
	mov     r6,r6,LSR #3	; number of 16-byte blocks to move
	mov     r0,#1
	mov     r0,r0,LSL R2    ; number of lines to copy
ARMDirtyCopy
   mov     r11,r9          ; get source pointer
   mov     r12,r10         ; get dest pointer
   mov     r5,r6           ; number of 16-byte blocks to copy
ARMDirtyCopy0
   ldmia   r11!,{r1-r4}     ; copy 16 bytes at a time
   stmia   r12!,{r1-r4}
   subs    r5,r5,#1        ; 16-byte block count
   bne     ARMDirtyCopy0
   
   add     r9,r9,r7        ; skip to next src line
   add     r10,r10,r8      ; ditto for dest
   subs    r0,r0,#1        ; decrement line count
   bne     ARMDirtyCopy
	ldmfd   sp!,{r0-r5,r8,r12}		; restore scratch registers
   mov     r10,#0          ; restore 0 we use to clear dirty flags
; continue scanning this line for more dirty sections
	mov     r6,#0	; reset dirty count
	cmp     r5,r4	; end of the line?
	beq     ARMDirty2bot	; yep
	ands    r7,r5,#3	; are we dword aligned?
	beq     ARMDirty_fast	; we can return to a fast scan
	b	ARMDirty_slow	; scan for more dirty sections
ARMDirty2bot
	cmp     r5,r4			; hit end of line?
	bne     ARMDirty_fast		; keep scanning
	add	  r12,r12,#1	; increment Y position
	cmp	  r12,r1			; done with entire scan?
	bne	  ARMDirty0		; keep going
	ldmfd	sp!,{r4-r12,pc} ; return

	END