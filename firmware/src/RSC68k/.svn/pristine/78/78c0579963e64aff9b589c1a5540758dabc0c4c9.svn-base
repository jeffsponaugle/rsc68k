/*******************************************************************/
/* 68000 CPU emulator written by Larry Bank                        */
/* Copyright 1999 BitBank Software, Inc.                           */
/*                                                                 */
/* This code was written from scratch using the 68000 data from    */
/* the Motorola databook "MC68030 Enhanced 32-Bit Microprocessor". */
/*                                                                 */
/* Change history:                                                 */
/* 12/30/99 started it - Larry B.                                  */
/* 1/28/03 Picked it up again for real :)  - Larry B.              */
/* 1/1/05 Converted to little-endian word order for faster access  */
/* 3/19/06 - Fixed interrupt mask                                  */
/* 10/25/06 - Fixed CMPM operand read order (SRC must be first)    */
/* 11/4/06 - fixed byte-to-long sign extension                     */
/*******************************************************************/
/* TODO */
/* Write NBCD, PACK, UNPK */
/* Fix NEGX Z FLAG */
#include "Startup/app.h"
#ifndef _WIN32
#include "Application/my_windows.h"
#endif
#include "Application/emu.h"

#define F_CARRY      1
#define F_OVERFLOW   2
#define F_ZERO       4
#define F_NEGATIVE   8
#define F_EXTEND     16
#define F_IRQMASK    0x700
#define F_SUPERVISOR 0x2000
#define F_TRACE      0x8000
#define F_TRACE1     0x4000

#define M68000 // this indicates there is only a 24-bit address bus

//#define COLLECT_HISTO
//#define SPEED_HACKS

#define NEWMOTOSHORT(p) (*(unsigned short *)p)
#define NEWMOTOLONG(p) (*(unsigned short *)p << 16) + *(unsigned short *)(p+2)

/* Some statics */
EMUHANDLERS3 *mem_handlers68K;
unsigned char *cFlagMap68K, *pOpcode, *ucIRQs;
int iClocks, *pOrigClocks;
unsigned long PC, *pCPUOffs;
extern BOOL bTrace;
void TRACE68K(REGS68K *regs, int iClocks);
//void memset(void *, unsigned char, int);


// Slapstic Fetch Hacks Start

#define FETCH_BITS 15
#define FETCH_GRANULARITY (1 << FETCH_BITS)

typedef unsigned char *(*CPUFetchHandler)(unsigned long u32Addr);

// 24 Bits of decode for the 68K
static CPUFetchHandler FetchHandlers[(1 << (24 - FETCH_BITS))];

void RegisterFetchHandler(unsigned long u32Address, unsigned long handler);

// Slapstic Fetch Hacks End


#ifdef COLLECT_HISTO
typedef struct tag_histo
{
unsigned long Read[4];
unsigned long ReadSlow[4];
unsigned long Write[4];
unsigned long WriteSlow[4];
unsigned long ulOps[65536];
} HISTO;
static HISTO Histo;
#endif // COLLECT_HISTO

static unsigned long lQuickVal[8] = {8,1,2,3,4,5,6,7}; // for ADDQ & SUBQ
static unsigned long ulSizeMasks[4] = {0xff, 0xffff, 0xffffffff, 0xffffffff};

// Translate the Mode/Register values from a destination EA by swapping the upper/lower 3 bits
static unsigned char ucModeTrans[64] = {
0,8,0x10,0x18,0x20,0x28,0x30,0x38,0x01,0x09,0x11,0x19,0x21,0x29,0x31,0x39,0x2,0x0a,0x12,0x1a,
0x22,0x2a,0x32,0x3a,0x3,0xb,0x13,0x1b,0x23,0x2b,0x33,0x3b,0x4,0xc,0x14,0x1c,0x24,0x2c,0x34,0x3c,
0x5,0xd,0x15,0x1d,0x25,0x2d,0x35,0x3d,0x6,0x0e,0x16,0x1e,0x26,0x2e,0x36,0x3e,0x7,0x0f,0x17,0x1f,
0x27,0x2f,0x37,0x3f};

// Extra clock cycles associated with each effective address (byte/word)
static unsigned char ucEAClocks_bw[64] = {
      0,0,0,0,0,0,0,0,
      0,0,0,0,0,0,0,0,   // first 16 don't add any
      4,4,4,4,4,4,4,4,
      4,4,4,4,4,4,4,4,   // addr reg indirect
      4,4,4,4,4,4,4,4,
      8,8,8,8,8,8,8,8,   // addr reg ind , addr reg ind + offset
      10,10,10,10,10,10,10,10,
      8,12,12,14,4,0,0,0
      };
// Extra clock cycles associated with each effective address (long)
static unsigned char ucEAClocks_l[64] = {
      0,0,0,0,0,0,0,0,
      0,0,0,0,0,0,0,0,   // first 16 don't add any
      8,8,8,8,8,8,8,8,
      8,8,8,8,8,8,8,8,   // addr reg indirect
      8,8,8,8,8,8,8,8,
      12,12,12,12,12,12,12,12,   // addr reg ind , addr reg ind + offset
      14,14,14,14,14,14,14,14,
      12,16,16,20,8,0,0,0
      };

// Auto increment/decrement sizes
static unsigned char ucIncrement[4] = {1,2,4,2};

void M68KCMP(REGS68K *regs, unsigned long ulData1, unsigned long ulData2, unsigned char ucSize);
void M68K_PUSHDWORD(REGS68K *regs, unsigned long ulData);
void M68K_PUSHWORD(REGS68K *regs, unsigned short usData);
void M68KSETSR(REGS68K *regs, unsigned long ulNewSR);
unsigned char M68KABCD(REGS68K *regs, unsigned char uc1, unsigned char uc2);
unsigned char M68KSBCD(REGS68K *regs, unsigned char uc1, unsigned char uc2);

void Invalid(void)
{
int i;
   i = 0;
}

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : Check68KInterrupts(REGS68K *)                              *
 *                                                                          *
 *  PURPOSE    : Check and handle pending interrupts.                       *
 *                                                                          *
 ****************************************************************************/
void Check68KInterrupts(REGS68K *regs)
{
unsigned char ucLevel = (char)((regs->ulRegSR >> 8) & 7); // current interrupt level
unsigned char *p;
unsigned long ulTemp, ulNewSR;
int iNewLevel;

   if (*ucIRQs >= (1<<(ucLevel+1)) || *ucIRQs & 0x80) // if requested int level greater than mask or NMI
      {
      ulNewSR = ulTemp = regs->ulRegSR; // save current status register
      ulNewSR |= F_SUPERVISOR; // set the supervisor flag
      ulNewSR &= ~(F_TRACE | F_TRACE1); // turn off trace
      ulNewSR &= 0xf0ff; // get rid of irq mask bits
      for (iNewLevel=7; iNewLevel>0; iNewLevel--) // find highest int level that we are using
         if (*ucIRQs & (1<<iNewLevel))
            break;
      ulNewSR |= (iNewLevel << 8); // set current interrupt as new mask level
      M68KSETSR(regs, ulNewSR); // make sure stack pointers are swapped if necessary
      if (regs->bStopped) // stuck on STOP instruction
         {
         PC += 4; // skip over it
         regs->bStopped = FALSE;
         }
      if (regs->ucCPUType == 0x00) // 68000 uses 3 words
         {
         M68K_PUSHDWORD(regs, PC); // push the program counter on the supervisor stack
         M68K_PUSHWORD(regs, (unsigned short)ulTemp); // push old status register
         }
      else // 68010,68020 use a 4 word structure
         {
         M68K_PUSHWORD(regs, (unsigned short)(iNewLevel<<2)); // push the interrupt vector
         M68K_PUSHDWORD(regs, PC); // push the program counter on the supervisor stack
         M68K_PUSHWORD(regs, (unsigned short)ulTemp); // push old status register
         }
      p = (unsigned char *)pCPUOffs[0]; // get address 0
      PC = NEWMOTOLONG(&p[(24 + iNewLevel)<<2]); /* Load the interrupt program counter */
      *ucIRQs &= ~(1<<iNewLevel); // clear pending interrupt flag
      }

} /* Check68KInterrupts() */

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : M68KRead(long, char)                                       *
 *                                                                          *
 *  PURPOSE    : Read a byte, word or long from memory.                     *
 *                                                                          *
 ****************************************************************************/
_inline unsigned long M68KRead(unsigned long ulAddr, unsigned char ucSize)
{
register unsigned char c, *p;
register unsigned long ul;

   ulAddr &= 0xffffff; // 68k only handles 24-bit addresses
//   if (ulAddr >= 0xffd188 && ulAddr < 0xffd18c)
//      c = 0;
   c = cFlagMap68K[ulAddr >> 12]; // Get flag for this address
   if (c & 0x40) /* If special flag (ROM or hardware) */
      {
      if (c == 0xff) // invalid address
         {
         return 0;
         }
      c &= 0x3f;
      *pOrigClocks = iClocks; // update clock count for things such as HBlank timing
#ifdef COLLECT_HISTO
      Histo.ReadSlow[ucSize]++;
#endif
      switch (ucSize)
         {
         case 0: // Byte
            return (mem_handlers68K[c].pfn_read8)(ulAddr ^ 1);
         case 2: // Long
            ul  = (mem_handlers68K[c].pfn_read16)(ulAddr) << 16;
            ul |= (mem_handlers68K[c].pfn_read16)(ulAddr+2);
            return ul;
         case 1: // Word
            ul  = (mem_handlers68K[c].pfn_read16)(ulAddr);
            return ul;
         default:
            Invalid();
            return -1;
         }
      }
   else
      {
#ifdef COLLECT_HISTO
      Histo.Read[ucSize]++;
#endif
      switch (ucSize)
         {
         case 0: // Byte
            p = (unsigned char *)((ulAddr^1) + pCPUOffs[ulAddr >> 16]); // get the address into our memory area
            return p[0];
         case 2: // Long
            p = (unsigned char *)(ulAddr + pCPUOffs[ulAddr >> 16]); // get the address into our memory area
            return NEWMOTOLONG(p); // slow method for mis-aligned data
         case 1: // Word
            p = (unsigned char *)(ulAddr + pCPUOffs[ulAddr >> 16]); // get the address into our memory area
//            return NEWMOTOSHORT(p);
            return p[0] + (p[1]<<8);
         default:
            Invalid();
            return -1;
         }
      }
} /* M68KRead() */

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : M68KWrite(long, long, char)                                *
 *                                                                          *
 *  PURPOSE    : Write a byte, word or long to memory.                      *
 *                                                                          *
 ****************************************************************************/
_inline void M68KWrite(unsigned long ulAddr, unsigned long ulData, unsigned char ucSize)
{
register unsigned char c, *p;

   ulAddr &= 0xffffff; // 68k only handles 24-bit addresses
//   if ((ulAddr >= 0x1c0008 && ulAddr < 0x1c000c) || ulAddr == 0x1c0059)
//      ulAddr |= 0;
   c = cFlagMap68K[ulAddr >> 12]; // Get flag for this address
   if (c & 0x80) /* If special flag (ROM or hardware) */
      {
      *pOrigClocks = iClocks; // update clock count for things such as sound (DAC) updates
      if (c == 0xbf || c == 0xff) /* ROM or invalid address, don't do anything */
	  {
         return;
	  }
      c &= 0x3f;
#ifdef COLLECT_HISTO
      Histo.WriteSlow[ucSize]++;
#endif
      switch (ucSize)
         {
         case 0: // Byte
            (mem_handlers68K[c].pfn_write8)(ulAddr ^ 1, (char)ulData);
            break;
         case 2: // Long
            (mem_handlers68K[c].pfn_write16)(ulAddr, (unsigned short)(ulData >> 16));
            (mem_handlers68K[c].pfn_write16)(ulAddr+2, (unsigned short)ulData);
            break;
         case 1: // Word
            (mem_handlers68K[c].pfn_write16)(ulAddr, (unsigned short)ulData);
            break;
         default:
            Invalid();
            break;
         }
      }
   else /* Normal RAM, just write it and leave */
      {
#ifdef COLLECT_HISTO
      Histo.Write[ucSize]++;
#endif
      switch (ucSize)
         {
         default:// invalid
            Invalid();
            break;
         case 0: // Byte
            p = (unsigned char *)((ulAddr ^ 1)+ pCPUOffs[ulAddr >> 16]); // get the address into our memory area
            p[0] = (unsigned char)ulData;
            break;
         case 2: // Long
            p = (unsigned char *)(ulAddr + pCPUOffs[ulAddr >> 16]); // get the address into our memory area
            p[1] = (unsigned char)(ulData >> 24);
            p[0] = (unsigned char)(ulData >> 16);
            p[3] = (unsigned char)(ulData >> 8);
            p[2] = (unsigned char)ulData;
            break;
         case 1: // Word
            p = (unsigned char *)(ulAddr + pCPUOffs[ulAddr >> 16]); // get the address into our memory area
//            *(unsigned short *)p = (unsigned short)ulData;
            p[0] = (unsigned char)ulData;
            p[1] = (unsigned char)(ulData >> 8);
            break;
         }
      }

} /* M68KWrite() */

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : M68KADDb(REGS68K *, ulong, ulong, char)                    *
 *                                                                          *
 *  PURPOSE    : Add to numbers and set the flags accordingly               *
 *                                                                          *
 ****************************************************************************/
 _inline unsigned char M68KADDb(REGS68K *regs, unsigned long ul1, unsigned long ul2)
 {
 register unsigned long ul;
 
   regs->ulRegSR &= ~(F_CARRY | F_OVERFLOW | F_ZERO | F_NEGATIVE | F_EXTEND); // clear these bits
   ul1 &= 0xff;
   ul2 &= 0xff;
   ul = ul1 + ul2;
   if (!(ul & 0xff))
      regs->ulRegSR |= F_ZERO;
   if (ul & 0x80)
      regs->ulRegSR |= F_NEGATIVE;
   if (ul & 0x100) // carry
      regs->ulRegSR |= (F_EXTEND | F_CARRY);
   if (((ul1^ul) & (ul2^ul)) & 0x80)
      regs->ulRegSR |= F_OVERFLOW;
   return (unsigned char)ul;
 } /* M68KADDb() */

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : M68KADDw(REGS68K *, ulong, ulong, char)                    *
 *                                                                          *
 *  PURPOSE    : Add to numbers and set the flags accordingly               *
 *                                                                          *
 ****************************************************************************/
 _inline unsigned short M68KADDw(REGS68K *regs, unsigned long ul1, unsigned long ul2)
 {
 register unsigned long ul;
 
   regs->ulRegSR &= ~(F_CARRY | F_OVERFLOW | F_ZERO | F_NEGATIVE | F_EXTEND); // clear these bits
   ul1 &= 0xffff;
   ul2 &= 0xffff;
   ul = ul1 + ul2;
   if (!(ul & 0xffff))
      regs->ulRegSR |= F_ZERO;
   if (ul & 0x8000)
      regs->ulRegSR |= F_NEGATIVE;
   if (ul & 0x10000) // carry
      regs->ulRegSR |= (F_EXTEND | F_CARRY);
   if (((ul1^ul) & (ul2^ul)) & 0x8000)
      regs->ulRegSR |= F_OVERFLOW;
   return (unsigned short)ul;
 } /* M68KADDw() */

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : M68KADDl(REGS68K *, ulong, ulong, char)                    *
 *                                                                          *
 *  PURPOSE    : Add to numbers and set the flags accordingly               *
 *                                                                          *
 ****************************************************************************/
 _inline unsigned long M68KADDl(REGS68K *regs, unsigned long ul1, unsigned long ul2)
 {
 register unsigned long ul;
 
   regs->ulRegSR &= ~(F_CARRY | F_OVERFLOW | F_ZERO | F_NEGATIVE | F_EXTEND); // clear these bits
   ul = ul1 + ul2;
   if (!ul)
      regs->ulRegSR |= F_ZERO;
   if (ul & 0x80000000)
      regs->ulRegSR |= F_NEGATIVE;
   if (ul < ul1 && ul < ul2) // carry
      regs->ulRegSR |= (F_EXTEND | F_CARRY);
   if (((ul1^ul) & (ul2^ul)) & 0x80000000)
      regs->ulRegSR |= F_OVERFLOW;
   return ul;
 } /* M68KADDl() */

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : M68KADDX(REGS68K *, ulong, ulong, char)                    *
 *                                                                          *
 *  PURPOSE    : Add to numbers and set the flags accordingly               *
 *                                                                          *
 ****************************************************************************/
 _inline unsigned long M68KADDX(REGS68K *regs, unsigned long ul1, unsigned long ul2, unsigned long ulX, unsigned char ucSize)
 {
 register unsigned long ul;
 
   regs->ulRegSR &= ~(F_CARRY | F_OVERFLOW | F_NEGATIVE | F_EXTEND); // clear these bits
   switch (ucSize)
      {
      case 0: // byte
         ul1 &= 0xff;
         ul2 &= 0xff;
         ul = ul1 + ul2 + ulX;
         if (ul & 0xff)
            regs->ulRegSR &= ~F_ZERO;
         if (ul & 0x80)
            regs->ulRegSR |= F_NEGATIVE;
         if (ul & 0x100) // carry
            regs->ulRegSR |= (F_EXTEND | F_CARRY);
         if (((ul1^ul) & (ul2^ul)) & 0x80)
            regs->ulRegSR |= F_OVERFLOW;
         break;
      case 1: // word
         ul1 &= 0xffff;
         ul2 &= 0xffff;
         ul = ul1 + ul2 + ulX;
         if (ul & 0xffff)
            regs->ulRegSR &= ~F_ZERO;
         if (ul & 0x8000)
            regs->ulRegSR |= F_NEGATIVE;
         if (ul & 0x10000) // carry
            regs->ulRegSR |= (F_EXTEND | F_CARRY);
         if (((ul1^ul) & (ul2^ul)) & 0x8000)
            regs->ulRegSR |= F_OVERFLOW;
         break;
      case 2: // long
         ul = ul1 + ul2 + ulX;
         if (ul)
            regs->ulRegSR &= ~F_ZERO;
         if (ul & 0x80000000)
            regs->ulRegSR |= F_NEGATIVE;
         if (ul < ul1 && ul < ul2) // carry - DEBUG check this
            regs->ulRegSR |= (F_EXTEND | F_CARRY);
         if (((ul1^ul) & (ul2^ul)) & 0x80000000)
            regs->ulRegSR |= F_OVERFLOW;
         break;
      default:
         Invalid();
         break;
      }
   return ul;
 } /* M68KADDX() */

 _inline unsigned char M68KSUBb(REGS68K *regs, unsigned long ulData1, unsigned long ulData2)
 {
unsigned long ulTemp;

   ulData1 &= 0xff;
   ulData2 &= 0xff;

   ulTemp = ulData2 - ulData1;
   regs->ulRegSR &= ~(F_EXTEND | F_CARRY | F_ZERO | F_NEGATIVE | F_OVERFLOW);

   if (!(ulTemp & 0xff))
      regs->ulRegSR |= F_ZERO;
   if (ulTemp & 0x80)
      regs->ulRegSR |= F_NEGATIVE;
   if (ulTemp & 0x100) // carry/borrow
      regs->ulRegSR |= (F_CARRY | F_EXTEND);
   if ((ulData2^ulData1) & (ulTemp^ulData2) & 0x80)
      regs->ulRegSR |= F_OVERFLOW;
   return (unsigned char)ulTemp;

 } /* M68KSUBb() */

 _inline unsigned short M68KSUBw(REGS68K *regs, unsigned long ulData1, unsigned long ulData2)
 {
unsigned long ulTemp;

   ulData1 &= 0xffff;
   ulData2 &= 0xffff;

   ulTemp = ulData2 - ulData1;
   regs->ulRegSR &= ~(F_EXTEND | F_CARRY | F_ZERO | F_NEGATIVE | F_OVERFLOW);

   if (!(ulTemp & 0xffff))
      regs->ulRegSR |= F_ZERO;
   if (ulTemp & 0x8000)
      regs->ulRegSR |= F_NEGATIVE;
   if (ulTemp & 0x10000) // carry/borrow
      regs->ulRegSR |= (F_CARRY | F_EXTEND);
   if ((ulData2^ulData1) & (ulTemp^ulData2) & 0x8000)
      regs->ulRegSR |= F_OVERFLOW;
   return (unsigned short)ulTemp;

 } /* M68KSUBw() */

 _inline unsigned long M68KSUBl(REGS68K *regs, unsigned long ulData1, unsigned long ulData2)
 {
unsigned long ulTemp;

   ulTemp = ulData2 - ulData1;
   regs->ulRegSR &= ~(F_EXTEND | F_CARRY | F_ZERO | F_NEGATIVE | F_OVERFLOW);

   if (ulTemp == 0)
      regs->ulRegSR |= F_ZERO;
   if (ulTemp & 0x80000000)
      regs->ulRegSR |= F_NEGATIVE;
   if (ulData1 > ulData2)
      regs->ulRegSR |= (F_CARRY | F_EXTEND);
   if ((ulData2^ulData1) & (ulTemp^ulData2) & 0x80000000)
      regs->ulRegSR |= F_OVERFLOW;
   return ulTemp;

 } /* M68KSUBl() */

 _inline unsigned long M68KSUBX(REGS68K *regs, unsigned long ulData1, unsigned long ulData2, unsigned long ulX, unsigned char ucSize)
 {
unsigned long ulTemp;

   ulData1 &= ulSizeMasks[ucSize];
   ulData2 &= ulSizeMasks[ucSize];

   ulTemp = ulData2 - ulData1 - ulX;
   regs->ulRegSR &= ~(F_EXTEND | F_CARRY | F_NEGATIVE | F_OVERFLOW);

   switch (ucSize)
      {
      case 0: // Byte
         if (ulTemp & 0xff)
            regs->ulRegSR &= ~F_ZERO;
         if (ulTemp & 0x80)
            regs->ulRegSR |= F_NEGATIVE;
         if (ulTemp & 0x100) // carry/borrow
            regs->ulRegSR |= (F_CARRY | F_EXTEND);
         if ((ulData2^ulData1) & (ulTemp^ulData2) & 0x80)
            regs->ulRegSR |= F_OVERFLOW;
         break;
      case 2: // Long
         if (ulTemp)
            regs->ulRegSR &= ~F_ZERO;
         if (ulTemp & 0x80000000)
            regs->ulRegSR |= F_NEGATIVE;
         if ((ulData1+ulX) > ulData2)
            regs->ulRegSR |= (F_CARRY | F_EXTEND);
         if ((ulData2^ulData1) & (ulTemp^ulData2) & 0x80000000)
            regs->ulRegSR |= F_OVERFLOW;
         break;
      case 1: // Word
         if (ulTemp & 0xffff)
            regs->ulRegSR &= ~F_ZERO;
         if (ulTemp & 0x8000)
            regs->ulRegSR |= F_NEGATIVE;
         if (ulTemp & 0x10000) // carry/borrow
            regs->ulRegSR |= (F_CARRY | F_EXTEND);
         if ((ulData2^ulData1) & (ulTemp^ulData2) & 0x8000)
            regs->ulRegSR |= F_OVERFLOW;
         break;
      }
   return (ulTemp & ulSizeMasks[ucSize]);

 } /* M68KSUBX() */

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : M68KGetEA(REGS68K *, char, char)                           *
 *                                                                          *
 *  PURPOSE    : Calculate and return the EA.                               *
 *                                                                          *
 ****************************************************************************/
_inline unsigned long M68KGetEA(REGS68K *regs, unsigned char ucModeReg)
{
register unsigned long ul, ulAddr;
register signed long slIndex;
register unsigned char ucScale;

   iClocks -= ucEAClocks_bw[ucModeReg];

   switch (ucModeReg)
      {
      case 0x10: // Address register indirect
      case 0x11:
      case 0x12:
      case 0x13:
      case 0x14:
      case 0x15:
      case 0x16:
      case 0x17:
         return regs->ulRegAddr[ucModeReg & 7].l;

      case 0x18: // Address register indirect with post-increment
      case 0x19:
      case 0x1a:
      case 0x1b:
      case 0x1c:
      case 0x1d:
      case 0x1e:
      case 0x1f:
         ul = regs->ulRegAddr[ucModeReg & 7].l;
         regs->ulRegAddr[ucModeReg & 7].l += 4;
         return ul;

      case 0x20: // Address register indirect with pre-decrement
      case 0x21:
      case 0x22:
      case 0x23:
      case 0x24:
      case 0x25:
      case 0x26:
      case 0x27:
         regs->ulRegAddr[ucModeReg & 7].l -= 4;
         ul = regs->ulRegAddr[ucModeReg & 7].l;
         return ul;

      case 0x28:  // Address register indirect with absolute 16-bit offset
      case 0x29:
      case 0x2a:
      case 0x2b:
      case 0x2c:
      case 0x2d:
      case 0x2e:
      case 0x2f:
         ulAddr = regs->ulRegAddr[ucModeReg & 7].l + (signed short)(NEWMOTOSHORT(&pOpcode[2]));
         PC += 2;
         pOpcode += 2;
         return ulAddr;

      case 0x30: // Address register indirect with 8-bit displacement and index offset
      case 0x31:
      case 0x32:
      case 0x33:
      case 0x34:
      case 0x35:
      case 0x36:
      case 0x37:
         ucScale = (pOpcode[3] >> 1) & 3;
         if (pOpcode[3] & 0x80) // address register
            {
            if (pOpcode[3] & 8) // word/long bit
               slIndex = regs->ulRegAddr[((pOpcode[3] >> 4) & 7)].l;
            else
               slIndex = (signed short)regs->ulRegAddr[((pOpcode[3] >> 4) & 7)].w;
            }
         else
            {
            if (pOpcode[3] & 8) // word/long bit
               slIndex = regs->ulRegData[((pOpcode[3] >> 4) & 7)].l;
            else
               slIndex = (signed short)regs->ulRegData[((pOpcode[3] >> 4) & 7)].w;
            }
         ulAddr = regs->ulRegAddr[ucModeReg & 7].l + (slIndex << ucScale) + (signed char)pOpcode[2];
         PC += 2;
         pOpcode += 2;
         return ulAddr;

      case 0x38: // Absolute short
         PC += 2;
         ulAddr = (signed short)NEWMOTOSHORT(&pOpcode[2]);
         pOpcode += 2;
         return ulAddr;

      case 0x39: // Absolute long
         PC += 4;
         ulAddr = NEWMOTOLONG(&pOpcode[2]);
         pOpcode += 4;
         return ulAddr;

      case 0x3a: // PC indirect with 16-bit offset
         ulAddr = PC + (signed short)NEWMOTOSHORT(&pOpcode[2]);
         PC += 2;
         pOpcode += 2;
         return ulAddr;

      case 0x3b: // PC indirect with index and 8-bit offset
         ucScale = (pOpcode[3] >> 1) & 3;
         if (pOpcode[3] & 0x80) // address register
            {
            if (pOpcode[3] & 8) // word/long bit
               slIndex = regs->ulRegAddr[((pOpcode[3] >> 4) & 7)].l;
            else
               slIndex = (signed short)regs->ulRegAddr[((pOpcode[3] >> 4) & 7)].w;
            }
         else
            {
            if (pOpcode[3] & 8) // word/long bit
               slIndex = regs->ulRegData[((pOpcode[3] >> 4) & 7)].l;
            else
               slIndex = (signed short)regs->ulRegData[((pOpcode[3] >> 4) & 7)].w;
            }
         ulAddr = PC + (slIndex << ucScale) + (signed char)pOpcode[2];
         PC += 2;
         pOpcode += 2;
         return ulAddr;
         
      default: // invalid
         Invalid();
         return -1;
         break;
      }
} /* M68KGetEA() */

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : M68KReadEA(REGS68K *, char, char)                          *
 *                                                                          *
 *  PURPOSE    : Calculate and read data from an EA.                        *
 *                                                                          *
 ****************************************************************************/
_inline unsigned long M68KReadEA(REGS68K *regs, unsigned char ucModeReg, unsigned char ucSize, BOOL bRMW)
{
register unsigned long ul, ulAddr;
register signed long slIndex;
register unsigned char ucScale;

   if (ucSize == 2)
      iClocks -= ucEAClocks_l[ucModeReg];
   else
      iClocks -= ucEAClocks_bw[ucModeReg];

   switch (ucModeReg)
      {
      case 0x00: // read from Dn
      case 0x01:
      case 0x02:
      case 0x03:
      case 0x04:
      case 0x05:
      case 0x06:
      case 0x07:
         return regs->ulRegData[ucModeReg & 7].l;

      case 0x08: // read from An
      case 0x09:
      case 0x0a:
      case 0x0b:
      case 0x0c:
      case 0x0d:
      case 0x0e:
      case 0x0f:
         return regs->ulRegAddr[ucModeReg & 7].l;

      case 0x10: // Address register indirect
      case 0x11:
      case 0x12:
      case 0x13:
      case 0x14:
      case 0x15:
      case 0x16:
      case 0x17:
         return M68KRead(regs->ulRegAddr[ucModeReg & 7].l, ucSize); // read from memory

      case 0x18: // Address register indirect with post-increment
      case 0x19:
      case 0x1a:
      case 0x1b:
      case 0x1c:
      case 0x1d:
      case 0x1e:
      case 0x1f:
         ul = M68KRead(regs->ulRegAddr[ucModeReg & 7].l, ucSize); // read from memory
         if (!bRMW)
            {
            regs->ulRegAddr[ucModeReg & 7].l += ucIncrement[ucSize];
            if (((ucModeReg & 7) == 7) && ucSize == 0) // need to keep stack pointer even
               regs->ulRegAddr[7].l++;
            }
         return ul;

      case 0x20: // Address register indirect with pre-decrement
      case 0x21:
      case 0x22:
      case 0x23:
      case 0x24:
      case 0x25:
      case 0x26:
      case 0x27:
         regs->ulRegAddr[ucModeReg & 7].l -= ucIncrement[ucSize];
         if (((ucModeReg & 7) == 7) && ucSize == 0) // need to keep stack pointer even
            regs->ulRegAddr[7].l--;
         ul = M68KRead(regs->ulRegAddr[ucModeReg & 7].l, ucSize); // read from memory
         return ul;

      case 0x28:  // Address register indirect with absolute 16-bit offset
      case 0x29:
      case 0x2a:
      case 0x2b:
      case 0x2c:
      case 0x2d:
      case 0x2e:
      case 0x2f:
         ulAddr = regs->ulRegAddr[ucModeReg & 7].l + (signed short)(NEWMOTOSHORT(&pOpcode[2]));
         if (!bRMW)
            {
            PC += 2;
            pOpcode += 2;
            }
         return M68KRead(ulAddr, ucSize); // read from memory

      case 0x30: // Address register indirect with 8-bit displacement and index offset
      case 0x31:
      case 0x32:
      case 0x33:
      case 0x34:
      case 0x35:
      case 0x36:
      case 0x37:
         ucScale = (pOpcode[3] >> 1) & 3;
         if (pOpcode[3] & 0x80) // address register
            {
            if (pOpcode[3] & 8) // word/long bit
               slIndex = regs->ulRegAddr[((pOpcode[3] >> 4) & 7)].l;
            else
               slIndex = (signed short)regs->ulRegAddr[((pOpcode[3] >> 4) & 7)].w;
            }
         else
            {
            if (pOpcode[3] & 8) // word/long bit
               slIndex = regs->ulRegData[((pOpcode[3] >> 4) & 7)].l;
            else
               slIndex = (signed short)regs->ulRegData[((pOpcode[3] >> 4) & 7)].w;
            }
         ulAddr = regs->ulRegAddr[ucModeReg & 7].l + (slIndex << ucScale) + (signed char)pOpcode[2];
         if (!bRMW)
            {
            PC += 2;
            pOpcode += 2;
            }
         return M68KRead(ulAddr, ucSize); // read from memory

      case 0x38: // Absolute short
         ulAddr = (signed short)NEWMOTOSHORT(&pOpcode[2]);
         if (!bRMW)
            {
            pOpcode += 2;
            PC += 2;
            }
         return M68KRead(ulAddr, ucSize); // read from memory

      case 0x39: // Absolute long
         ulAddr = NEWMOTOLONG(&pOpcode[2]);
         if (!bRMW)
            {
            pOpcode += 4;
            PC += 4;
            }
         return M68KRead(ulAddr, ucSize); // read from memory

      case 0x3a: // PC indirect with 16-bit offset
         ulAddr = PC + (signed short)NEWMOTOSHORT(&pOpcode[2]);
         if (!bRMW)
            {
            PC += 2;
            pOpcode += 2;
            }
         return M68KRead(ulAddr, ucSize); // read from memory

      case 0x3b: // PC indirect with index and 8-bit offset
         ucScale = (pOpcode[3] >> 1) & 3;
         if (pOpcode[3] & 0x80) // address register
            {
            if (pOpcode[3] & 8) // word/long bit
               slIndex = regs->ulRegAddr[((pOpcode[3] >> 4) & 7)].l;
            else
               slIndex = (signed short)regs->ulRegAddr[((pOpcode[3] >> 4) & 7)].w;
            }
         else
            {
            if (pOpcode[3] & 8) // word/long bit
               slIndex = regs->ulRegData[((pOpcode[3] >> 4) & 7)].l;
            else
               slIndex = (signed short)regs->ulRegData[((pOpcode[3] >> 4) & 7)].w;
            }
         ulAddr = PC + (slIndex << ucScale) + (signed char)pOpcode[2];
         if (!bRMW)
            {
            PC += 2;
            pOpcode += 2;
            }
         return M68KRead(ulAddr, ucSize); // read from memory
         
      case 0x3c: // Immediate data
         switch (ucSize)
            {
            case 3: // invalid
               Invalid();
               break;
            case 0: // Byte
               ul = pOpcode[2];
               PC += 2;
               pOpcode += 2;
               break;
            case 2: // Long
               ul = NEWMOTOLONG(&pOpcode[2]);
               PC += 4;
               pOpcode += 4;
               break;
            case 1: // Word
               ul = NEWMOTOSHORT(&pOpcode[2]);
               PC += 2;
               pOpcode += 2;
               break;
            }
         return ul;

      default: // invalid
         Invalid();
         return -1;
      }
} /* M68KReadEA() */

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : M68KWriteEA(char, char, long, REGS68K *)                   *
 *                                                                          *
 *  PURPOSE    : Calculate and write data to an EA.                         *
 *                                                                          *
 ****************************************************************************/
_inline void M68KWriteEA(unsigned char ucModeReg, unsigned char ucSize, unsigned long ulData, REGS68K *regs, BOOL bRMW)
{
register unsigned long ulAddr;
register signed long slIndex;
register unsigned char ucScale;

   if (ucSize == 2)
      iClocks -= ucEAClocks_l[ucModeReg];
   else
      iClocks -= ucEAClocks_bw[ucModeReg];

   switch (ucModeReg)
      {
      case 0x00: // write to Dn
      case 0x01:
      case 0x02:
      case 0x03:
      case 0x04:
      case 0x05:
      case 0x06:
      case 0x07:
         switch (ucSize)
            {
            case 0: // byte
               regs->ulRegData[ucModeReg & 7].b = (char)ulData;
               break;
            case 1: // word
               regs->ulRegData[ucModeReg & 7].w = (short)ulData;
               break;
            case 2: // long
               regs->ulRegData[ucModeReg & 7].l = ulData;
               break;
            }
         break;

      case 0x08: // Write to An
      case 0x09:
      case 0x0a:
      case 0x0b:
      case 0x0c:
      case 0x0d:
      case 0x0e:
      case 0x0f:
         switch (ucSize)
            {
            case 1: // word
               regs->ulRegAddr[ucModeReg & 7].l = (signed short)ulData;
               break;
            case 2: // long
               regs->ulRegAddr[ucModeReg & 7].l = ulData;
               break;
            }
         break;

      case 0x10: // Address register indirect
      case 0x11:
      case 0x12:
      case 0x13:
      case 0x14:
      case 0x15:
      case 0x16:
      case 0x17:
         M68KWrite(regs->ulRegAddr[ucModeReg & 7].l, ulData, ucSize); // Write to memory
         break;

      case 0x18: // Address register indirect with post-increment
      case 0x19:
      case 0x1a:
      case 0x1b:
      case 0x1c:
      case 0x1d:
      case 0x1e:
      case 0x1f:
         M68KWrite(regs->ulRegAddr[ucModeReg & 7].l, ulData, ucSize); // Write to memory
         regs->ulRegAddr[ucModeReg & 7].l += ucIncrement[ucSize];
         if (((ucModeReg & 7) == 7) && ucSize == 0) // need to keep stack pointer even
            regs->ulRegAddr[7].l++;
         break;

      case 0x20: // Address register indirect with pre-decrement
      case 0x21:
      case 0x22:
      case 0x23:
      case 0x24:
      case 0x25:
      case 0x26:
      case 0x27:
         if (!bRMW)
            {
            regs->ulRegAddr[ucModeReg & 7].l -= ucIncrement[ucSize];
            if (((ucModeReg & 7) == 7) && ucSize == 0) // need to keep stack pointer even
               regs->ulRegAddr[7].l--;
            }
         M68KWrite(regs->ulRegAddr[ucModeReg & 7].l, ulData, ucSize); // Write to memory
         break;

      case 0x28:  // Address register indirect with absolute 16-bit offset
      case 0x29:
      case 0x2a:
      case 0x2b:
      case 0x2c:
      case 0x2d:
      case 0x2e:
      case 0x2f:
         ulAddr = regs->ulRegAddr[ucModeReg & 7].l + (signed short)(NEWMOTOSHORT(&pOpcode[2]));
         PC += 2;
         M68KWrite(ulAddr, ulData, ucSize);
         break;

      case 0x30: // Address register indirect with 8-bit displacement and index offset
      case 0x31:
      case 0x32:
      case 0x33:
      case 0x34:
      case 0x35:
      case 0x36:
      case 0x37:
         ucScale = (pOpcode[3] >> 1) & 3;
         if (pOpcode[3] & 0x80) // address register
            {
            if (pOpcode[3] & 8) // word/long bit
               slIndex = regs->ulRegAddr[((pOpcode[3] >> 4) & 7)].l;
            else
               slIndex = (signed short)regs->ulRegAddr[((pOpcode[3] >> 4) & 7)].w;
            }
         else // data register
            {
            if (pOpcode[3] & 8) // word/long bit
               slIndex = regs->ulRegData[((pOpcode[3] >> 4) & 7)].l;
            else
               slIndex = (signed short)regs->ulRegData[((pOpcode[3] >> 4) & 7)].w;
            }
         ulAddr = regs->ulRegAddr[ucModeReg & 7].l + (slIndex << ucScale) + (signed char)pOpcode[2];
         PC += 2;
         M68KWrite(ulAddr, ulData, ucSize);
         break;

      case 0x38: // Absolute short
         ulAddr = (signed short)NEWMOTOSHORT(&pOpcode[2]);
         PC += 2;
         M68KWrite(ulAddr, ulData, ucSize);
         break;

      case 0x39: // Absolute long
         ulAddr = NEWMOTOLONG(&pOpcode[2]);
         PC += 4;
         M68KWrite(ulAddr, ulData, ucSize);
         break;

// Invalid address modes for writing
      case 0x3a: // PC indirect with 16-bit offset
      case 0x3b: // PC indirect with index and 8-bit offset
      case 0x3c: // Immediate data
      case 0x3d: // invalid
      case 0x3e:
      case 0x3f:
         Invalid();
         break;
      }

} /* M68KWriteEA() */

void M68K_PUSHWORD(REGS68K *regs, unsigned short usData)
{
   regs->ulRegAddr[7].l -= 2;
   M68KWrite(regs->ulRegAddr[7].l, usData, 1);
} /* M68K_PUSHWORD() */

unsigned short M68K_POPWORD(REGS68K *regs)
{
register unsigned short us;

   us = (unsigned short)M68KRead(regs->ulRegAddr[7].l, 1);
   regs->ulRegAddr[7].l += 2;
   return us;
} /* M68K_POPWORD() */

void M68K_PUSHDWORD(REGS68K *regs, unsigned long ulData)
{
   regs->ulRegAddr[7].l -= 4;
   M68KWrite(regs->ulRegAddr[7].l, ulData, 2);
} /* M68K_PUSHDWORD() */

unsigned long M68K_POPDWORD(REGS68K *regs)
{
register unsigned long ul;

   ul = M68KRead(regs->ulRegAddr[7].l, 2);
   regs->ulRegAddr[7].l += 4;
   return ul;
} /* M68K_POPDWORD() */

static _inline int FlagsNZL(unsigned long l)
{
   if (l == 0)
      return F_ZERO;
   if (l & 0x80000000)
      return F_NEGATIVE;
   return 0;
} /* FlagsNZL() */

static _inline int FlagsNZW(unsigned long l)
{
   if ((l & 0xffff) == 0)
      return F_ZERO;
   if (l & 0x8000)
      return F_NEGATIVE;
   return 0;
} /* FlagsNZW() */

static _inline int FlagsNZB(unsigned long l)
{
   if ((l & 0xff) == 0)
      return F_ZERO;
   if (l & 0x80)
      return F_NEGATIVE;
   return 0;
} /* FlagsNZB() */

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : M68KTRAP(REGS68K *, int)                                   *
 *                                                                          *
 *  PURPOSE    : Perform a software exception as specified by the vector.   *
 *                                                                          *
 ****************************************************************************/
void M68KTRAP(REGS68K *regs, int iVector)
{
unsigned long ulOldSR, ulNewSR;

   if (iVector == 8) // priv violation
      iVector |= 0;
   ulOldSR = regs->ulRegSR;
   ulNewSR = (regs->ulRegSR | F_SUPERVISOR) & ~F_TRACE;
   M68KSETSR(regs, ulNewSR); // set supervisor mode, disable trace
//   M68K_PUSHWORD(regs, (unsigned short)(iVector*4));
   M68K_PUSHDWORD(regs, PC);
   M68K_PUSHWORD(regs, (unsigned short)ulOldSR);
   PC = M68KRead(iVector*4, 2); // get the new PC from the TRAP vector

} /* M68KTRAP() */

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : M68KSETSR(REGS68k *, unsigned short)                       *
 *                                                                          *
 *  PURPOSE    : Set the status register value and update the stack pointer.*
 *                                                                          *
 ****************************************************************************/
void M68KSETSR(REGS68K *regs, unsigned long ulNewSR)
{
unsigned long ulNewS, ulOldS;

   ulNewS = ulNewSR & F_SUPERVISOR;
   ulOldS = regs->ulRegSR & F_SUPERVISOR;
   regs->ulRegSR = ulNewSR; // set the new value
   if (ulNewS != ulOldS) // Change in supervisor status
      {
      if (ulOldS) // changing from supervisor to user mode
         {
         regs->ulRegSSP = regs->ulRegAddr[7].l;
         regs->ulRegAddr[7].l = regs->ulRegUSP;
         }
      else
         {
         regs->ulRegUSP = regs->ulRegAddr[7].l;
         regs->ulRegAddr[7].l = regs->ulRegSSP;
         }
      }

} /* M68KSETSR() */

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : RESET68K(char *, REGS68K *)                                *
 *                                                                          *
 *  PURPOSE    : Get the 68000 after a reset.                               *
 *                                                                          *
 ****************************************************************************/
void RESET68K(unsigned long *ulCPUOffs, REGS68K *regs, int iCPU)
{
unsigned char *p;

#ifdef COLLECT_HISTO
   memset(&Histo, 0, sizeof(HISTO));
#endif
   regs->ucCPUType = (unsigned char)iCPU; // save the CPU type (0x00=68000, 0x10=68010, 0x20=68020)
   p = (unsigned char *)ulCPUOffs[0]; // get address 0
   memset(regs, 0, sizeof(REGS68K)); /* Start with a clean slate at reset */
   regs->ulRegAddr[7].l = NEWMOTOLONG(p); /* Load the supervisor stack pointer */
   regs->ulRegUSP = 0; // set user stack pointer to 0
   regs->ulRegSSP = NEWMOTOLONG(&p[0]); /* Load the stack pointer */
   regs->ulRegPC = NEWMOTOLONG(&p[4]); /* Load the reset program counter */
   regs->ulRegSR = F_SUPERVISOR | F_IRQMASK | F_ZERO; /* Start in supervisor state & with IRQs disabled */

   // Clear the Slapstic hack struct.
   memset(FetchHandlers, 0x00, sizeof(FetchHandlers));

} /* RESET68K() */

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : M68KMOVEP(REGS68K *, unsigned long)                        *
 *                                                                          *
 *  PURPOSE    : Move data to peripherals.                                  *
 *                                                                          *
 ****************************************************************************/
void M68KMOVEP(REGS68K *regs, unsigned long usOpcode)
{
unsigned long ulData, ulAddr;

   // Address register indirect with 16-bit offset
   ulAddr = regs->ulRegAddr[usOpcode & 7].l + (signed short)(NEWMOTOSHORT(&pOpcode[2]));
   PC += 2; // 2 bytes of offset
   switch ((usOpcode >> 6) & 7)
      {
      case 4: // word from memory to register
         ulData = M68KRead(ulAddr, 0) << 8;
         ulData |= M68KRead(ulAddr+2,0);
         regs->ulRegData[(usOpcode >> 9) & 7].w = (unsigned short)ulData;
         iClocks -= 16;
         break;

      case 5: // long from memory to register
         ulData = (M68KRead(ulAddr, 0) << 24);
         ulData |= (M68KRead(ulAddr+2,0) << 16);
         ulData |= (M68KRead(ulAddr+4,0) << 8);
         ulData |= M68KRead(ulAddr+6,0);
         regs->ulRegData[(usOpcode >> 9) & 7].l = ulData;
         iClocks -= 24;
         break;

      case 6: // word from register to memory
         ulData = regs->ulRegData[(usOpcode >> 9) & 7].l; // data register
         M68KWrite(ulAddr, (ulData >> 8) & 0xff, 0);
         M68KWrite(ulAddr+2, ulData & 0xff, 0);
         iClocks -= 16;
         break;

      case 7: // long from register to memory
         ulData = regs->ulRegData[(usOpcode >> 9) & 7].l; // data register
         M68KWrite(ulAddr, (ulData >> 24) & 0xff, 0);
         M68KWrite(ulAddr+2, (ulData >> 16) & 0xff, 0);
         M68KWrite(ulAddr+4, (ulData >> 8) & 0xff, 0);
         M68KWrite(ulAddr+6, ulData & 0xff, 0);
         iClocks -= 24;
         break;
      }

} /* M68KMOVEP() */

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : M68KMisc2(REGS68K *, unsigned long)                        *
 *                                                                          *
 *  PURPOSE    : Instructions starting with 0x0000.                         *
 *                                                                          *
 ****************************************************************************/
void M68KMisc2(REGS68K *regs, unsigned long usOpcode)
{
register unsigned long ul, ulTemp, ulRegister;
register unsigned char ucTemp, ucOpMode;
register unsigned short usTemp;

   ucOpMode = (char)(usOpcode & 0x3f);
   switch ((usOpcode >> 6)  & 0x3f) // decode bits 6-11
      {
      case 0: // ORI byte
         if (usOpcode == 0x3c) // ORI to CCR
            {
            iClocks -= 20;
            PC += 2;
            ucTemp = pOpcode[2];
            pOpcode += 2;
            regs->ulRegSR |= ucTemp;
            }
         else
            {
            iClocks -= 8;
            PC += 2;
            ucTemp = pOpcode[2];
            pOpcode += 2;
            regs->ulRegSR &= ~(F_ZERO | F_NEGATIVE | F_CARRY | F_OVERFLOW);
            if (ucOpMode < 8) // if working with a data register, do it faster
               {
               ulRegister = usOpcode & 7;
               regs->ulRegData[ulRegister].b |= ucTemp;
               if (regs->ulRegData[ulRegister].b & 0x80)
                  regs->ulRegSR |= F_NEGATIVE;
               if (!(regs->ulRegData[ulRegister].b))
                  regs->ulRegSR |= F_ZERO;
               }
            else
               {
               ul = M68KReadEA(regs, ucOpMode, 0, TRUE);
               ul |= ucTemp;
               if (ul & 0x80)
                  regs->ulRegSR |= F_NEGATIVE;
               if (!(ul & 0xff))
                  regs->ulRegSR |= F_ZERO;
               M68KWriteEA(ucOpMode, 0, ul, regs, TRUE);
               }
            }
         break;
      case 1: // ORI word
         if (usOpcode == 0x7c) // ORI to SR
            {
            iClocks -= 20;
            PC += 2;
            usTemp = NEWMOTOSHORT(&pOpcode[2]);
            pOpcode += 2;
            ul = regs->ulRegSR | usTemp;
            M68KSETSR(regs, ul); // possible change of privilege state
            Check68KInterrupts(regs);
            }
         else
            {
            iClocks -= 8;
            PC += 2;
            usTemp = NEWMOTOSHORT(&pOpcode[2]);
            pOpcode += 2;
            regs->ulRegSR &= ~(F_ZERO | F_NEGATIVE | F_CARRY | F_OVERFLOW);
            if (ucOpMode < 8) // if working with a data register, do it faster
               {
               ulRegister = usOpcode & 7;
               regs->ulRegData[ulRegister].w |= usTemp;
               if (regs->ulRegData[ulRegister].w & 0x8000)
                  regs->ulRegSR |= F_NEGATIVE;
               if (!(regs->ulRegData[ulRegister].w))
                  regs->ulRegSR |= F_ZERO;
               }
            else
               {
               ul = M68KReadEA(regs, ucOpMode, 1, TRUE);
               ul |= usTemp;
               if (ul & 0x8000)
                  regs->ulRegSR |= F_NEGATIVE;
               if (!(ul & 0xffff))
                  regs->ulRegSR |= F_ZERO;
               M68KWriteEA(ucOpMode, 1, ul, regs, TRUE);
               }
            }
         break;
      case 2: // ORI long
         iClocks -= 16;
         PC += 4;
         ulTemp = NEWMOTOLONG(&pOpcode[2]);
         pOpcode += 4;
         regs->ulRegSR &= ~(F_ZERO | F_NEGATIVE | F_CARRY | F_OVERFLOW);
         if (ucOpMode < 8) // if working with a data register, do it faster
            {
            ulRegister = usOpcode & 7;
            regs->ulRegData[ulRegister].l |= ulTemp;
            if (regs->ulRegData[ulRegister].l & 0x80000000)
               regs->ulRegSR |= F_NEGATIVE;
            if (!(regs->ulRegData[ulRegister].l))
               regs->ulRegSR |= F_ZERO;
            }
         else
            {
            ul = M68KReadEA(regs, ucOpMode, 2, TRUE);
            ul |= ulTemp;
            if (ul & 0x80000000)
               regs->ulRegSR |= F_NEGATIVE;
            if (!ul)
               regs->ulRegSR |= F_ZERO;
            M68KWriteEA(ucOpMode, 2, ul, regs, TRUE);
            }
         break;

      case 0x8: // ANDI byte
         if (usOpcode == 0x23c) // ANDI to CCR
            {
            iClocks -= 20;
            PC += 2;
            ucTemp = pOpcode[2];
            pOpcode += 2;
            regs->ulRegSR &= (0xff00 | ucTemp); // and condition code bits
            }
         else
            {
            iClocks -= 8;
            PC += 2;
            ucTemp = pOpcode[2];
            pOpcode += 2;
            regs->ulRegSR &= ~(F_ZERO | F_NEGATIVE | F_CARRY | F_OVERFLOW);
            if (ucOpMode < 8) // if working with a data register, do it faster
               {
               ulRegister = usOpcode & 7;
               regs->ulRegData[ulRegister].b &= ucTemp;
               if (regs->ulRegData[ulRegister].b & 0x80)
                  regs->ulRegSR |= F_NEGATIVE;
               if (!(regs->ulRegData[ulRegister].b))
                  regs->ulRegSR |= F_ZERO;
               }
            else
               {
               ul = M68KReadEA(regs, ucOpMode, 0, TRUE);
               ul &= ucTemp;
               if (ul & 0x80)
                  regs->ulRegSR |= F_NEGATIVE;
               if (!(ul & 0xff))
                  regs->ulRegSR |= F_ZERO;
               M68KWriteEA(ucOpMode, 0, ul, regs, TRUE);
               }
            }
         break;
      case 0x9: // ANDI word
         if (usOpcode == 0x27c) // ANDI to SR
            {
            if (!(regs->ulRegSR & F_SUPERVISOR))
               M68KTRAP(regs, 8); // privilege violation
            else
               {
               iClocks -= 20;
               PC += 2;
               usTemp = NEWMOTOSHORT(&pOpcode[2]);
               pOpcode += 2;
               ul = regs->ulRegSR & usTemp;
               M68KSETSR(regs, ul); // possible change of privilege state
               Check68KInterrupts(regs);
               }
            }
         else
            {
            iClocks -= 8;
            PC += 2;
            usTemp = NEWMOTOSHORT(&pOpcode[2]);
            pOpcode += 2;
            regs->ulRegSR &= ~(F_ZERO | F_NEGATIVE | F_CARRY | F_OVERFLOW);
            if (ucOpMode < 8) // if working with a data register, do it faster
               {
               ulRegister = usOpcode & 7;
               regs->ulRegData[ulRegister].w &= usTemp;
               if (regs->ulRegData[ulRegister].w & 0x8000)
                  regs->ulRegSR |= F_NEGATIVE;
               if (!(regs->ulRegData[ulRegister].w))
                  regs->ulRegSR |= F_ZERO;
               }
            else
               {
               ul = M68KReadEA(regs, ucOpMode, 1, TRUE);
               ul &= usTemp;
               if (ul & 0x8000)
                  regs->ulRegSR |= F_NEGATIVE;
               if (!(ul & 0xffff))
                  regs->ulRegSR |= F_ZERO;
               M68KWriteEA(ucOpMode, 1, ul, regs, TRUE);
               }
            }
         break;
      case 0xa: // ANDI long
         iClocks -= 16;
         PC += 4;
         ulTemp = NEWMOTOLONG(&pOpcode[2]);
         pOpcode += 4;
         regs->ulRegSR &= ~(F_ZERO | F_NEGATIVE | F_CARRY | F_OVERFLOW);
         if (ucOpMode < 8) // if working with a data register, do it faster
            {
            ulRegister = usOpcode & 7;
            regs->ulRegData[ulRegister].l &= ulTemp;
            if (regs->ulRegData[ulRegister].l & 0x80000000)
               regs->ulRegSR |= F_NEGATIVE;
            if (!(regs->ulRegData[ulRegister].l))
               regs->ulRegSR |= F_ZERO;
            }
         else
            {
            ul = M68KReadEA(regs, ucOpMode, 2, TRUE);
            ul &= ulTemp;
            if (ul & 0x80000000)
               regs->ulRegSR |= F_NEGATIVE;
            if (!ul)
               regs->ulRegSR |= F_ZERO;
            M68KWriteEA(ucOpMode, 2, ul, regs, TRUE);
            }
         break;

      case 0x10: // SUBI byte
         iClocks -= 8;
         PC += 2;
         ucTemp = pOpcode[2];
         pOpcode += 2;
         if (ucOpMode < 8) // faster for data register
            {
            regs->ulRegData[usOpcode & 7].b = M68KSUBb(regs, ucTemp, regs->ulRegData[usOpcode & 7].b);
            }
         else
            {
            ul = M68KReadEA(regs, ucOpMode, 0, TRUE);
            ul = M68KSUBb(regs, ucTemp, ul); 
            M68KWriteEA(ucOpMode, 0, ul, regs, TRUE);
            }
         break;
      case 0x11: // SUBI word
         iClocks -= 8;
         PC += 2;
         usTemp = NEWMOTOSHORT(&pOpcode[2]);
         pOpcode += 2;
         if (ucOpMode < 8) // faster for data register
            {
            regs->ulRegData[usOpcode & 7].w = M68KSUBw(regs, usTemp, regs->ulRegData[usOpcode & 7].w);
            }
         else
            {
            ul = M68KReadEA(regs, ucOpMode, 1, TRUE);
            ul = M68KSUBw(regs, usTemp, ul); 
            M68KWriteEA(ucOpMode, 1, ul, regs, TRUE);
            }
         break;
      case 0x12: // SUBI long
         iClocks -= 16;
         PC += 4;
         ulTemp = NEWMOTOLONG(&pOpcode[2]);
         pOpcode += 4;
         if (ucOpMode < 8) // faster for data register
            {
            regs->ulRegData[usOpcode & 7].l = M68KSUBl(regs, ulTemp, regs->ulRegData[usOpcode & 7].l);
            }
         else
            {
            ul = M68KReadEA(regs, ucOpMode, 2, TRUE);
            ul = M68KSUBl(regs, ulTemp, ul); 
            M68KWriteEA(ucOpMode, 2, ul, regs, TRUE);
            }
         break;
      case 0x20: // static bit TST
         iClocks -= 10;
         PC += 2;
         ucTemp = pOpcode[2]; // get the bit # from a data word
         pOpcode += 2;
         regs->ulRegSR &= ~F_ZERO;
         if ((usOpcode & 0x38) != 0) // byte sized for memory
            {
            ucTemp &= 7;
            ul = M68KReadEA(regs, ucOpMode, 0, FALSE);
            if (!(ul & (1<<ucTemp))) // if bit tested is zero, set Z flag
               regs->ulRegSR |= F_ZERO;
            }
         else // long sized for data register
            {
            ucTemp &= 31;
            ulRegister = usOpcode & 7;
            if (!(regs->ulRegData[ulRegister].l & (1<<ucTemp))) // if bit tested is zero, set Z flag
               regs->ulRegSR |= F_ZERO;
            }
         break;
      case 0x21: // static bit CHG
         iClocks -= 10;
         PC += 2;
         ucTemp = pOpcode[2]; // get the bit # from a data word
         pOpcode += 2;
         regs->ulRegSR &= ~F_ZERO;
         if ((usOpcode & 0x38) != 0) // byte sized for memory
            {
            ucTemp &= 7;
            ul = M68KReadEA(regs, ucOpMode, 0, TRUE);
            if (!(ul & (1<<ucTemp))) // if bit tested is zero, set Z flag
               regs->ulRegSR |= F_ZERO;
            ul ^= (1 << ucTemp);
            M68KWriteEA(ucOpMode, 0, ul, regs, TRUE);
            }
         else // long sized for data register
            {
            ucTemp &= 31;
            ulRegister = usOpcode & 7;
            if (!(regs->ulRegData[ulRegister].l & (1<<ucTemp))) // if bit tested is zero, set Z flag
               regs->ulRegSR |= F_ZERO;
            regs->ulRegData[ulRegister].l ^= (1 << ucTemp);
            }
         break;
      case 0x22: // static bit CLR
         iClocks -= 10;
         PC += 2;
         ucTemp = pOpcode[2]; // get the bit # from a data word
         pOpcode += 2;
         regs->ulRegSR &= ~F_ZERO;
         if ((usOpcode & 0x38) != 0) // byte sized for memory
            {
            ucTemp &= 7;
            ul = M68KReadEA(regs, ucOpMode, 0, TRUE);
            if (!(ul & (1<<ucTemp))) // if bit tested is zero, set Z flag
               regs->ulRegSR |= F_ZERO;
            ul &= ~(1 << ucTemp);
            M68KWriteEA(ucOpMode, 0, ul, regs, TRUE);
            }
         else // long sized for data register
            {
            ucTemp &= 31;
            ulRegister = usOpcode & 7;
            if (!(regs->ulRegData[ulRegister].l & (1<<ucTemp))) // if bit tested is zero, set Z flag
               regs->ulRegSR |= F_ZERO;
            regs->ulRegData[ulRegister].l &= ~(1 << ucTemp);
            }
         break;
      case 0x23: // static bit SET
         iClocks -= 10;
         PC += 2;
         ucTemp = pOpcode[2]; // get the bit # from a data word
         pOpcode += 2;
         regs->ulRegSR &= ~F_ZERO;
         if ((usOpcode & 0x38) != 0) // byte sized for memory
            {
            ucTemp &= 7;
            ul = M68KReadEA(regs, ucOpMode, 0, TRUE);
            if (!(ul & (1<<ucTemp))) // if bit tested is zero, set Z flag
               regs->ulRegSR |= F_ZERO;
            ul |= (1 << ucTemp);
            M68KWriteEA(ucOpMode, 0, ul, regs, TRUE);
            }
         else // long sized for data register
            {
            ucTemp &= 31;
            ulRegister = usOpcode & 7;
            if (!(regs->ulRegData[ulRegister].l & (1<<ucTemp))) // if bit tested is zero, set Z flag
               regs->ulRegSR |= F_ZERO;
            regs->ulRegData[ulRegister].l |= (1 << ucTemp);
            }
         break;

      case 0x18: // ADDI byte
         iClocks -= 8;
         PC += 2;
         ucTemp = pOpcode[2];
         pOpcode += 2;
         if (ucOpMode < 8) // faster for data register
            {
            regs->ulRegData[usOpcode & 7].b = M68KADDb(regs, ucTemp, regs->ulRegData[usOpcode & 7].b);
            }
         else
            {
            ul = M68KReadEA(regs, ucOpMode, 0, TRUE);
            ul = M68KADDb(regs, ul, ucTemp);
            M68KWriteEA(ucOpMode, 0, ul, regs, TRUE);
            }
         break;
      case 0x19: // ADDI word
         iClocks -= 8;
         PC += 2;
         usTemp = NEWMOTOSHORT(&pOpcode[2]);
         pOpcode += 2;
         if (ucOpMode < 8) // faster for data register
            {
            regs->ulRegData[usOpcode & 7].w = M68KADDw(regs, usTemp, regs->ulRegData[usOpcode & 7].w);
            }
         else
            {
            ul = M68KReadEA(regs, ucOpMode, 1, TRUE);
            ul = M68KADDw(regs, ul, usTemp);
            M68KWriteEA(ucOpMode, 1, ul, regs, TRUE);
            }
         break;
      case 0x1a: // ADDI long
         iClocks -= 16;
         PC += 4;
         ulTemp = NEWMOTOLONG(&pOpcode[2]);
         pOpcode += 4;
         if (ucOpMode < 8) // faster for data register
            {
            regs->ulRegData[usOpcode & 7].l = M68KADDl(regs, ulTemp, regs->ulRegData[usOpcode & 7].l);
            }
         else
            {
            ul = M68KReadEA(regs, ucOpMode, 2, TRUE);
            ul = M68KADDl(regs, ul, ulTemp);
            M68KWriteEA(ucOpMode, 2, ul, regs, TRUE);
            }
         break;

      case 0x28: // EORI byte
         if (usOpcode == 0xa3c) // EORI to CCR
            {
            iClocks -= 20;
            PC += 2;
            regs->ulRegSR ^= pOpcode[2];
            }
         else
            {
            iClocks -= 8;
            PC += 2;
            ucTemp = pOpcode[2];
            pOpcode += 2;
            regs->ulRegSR &= ~(F_ZERO | F_NEGATIVE | F_CARRY | F_OVERFLOW);
            if (ucOpMode < 8) // if working with a data register, do it faster
               {
               ulRegister = usOpcode & 7;
               regs->ulRegData[ulRegister].b ^= ucTemp;
               if (regs->ulRegData[ulRegister].b & 0x80)
                  regs->ulRegSR |= F_NEGATIVE;
               if (!(regs->ulRegData[ulRegister].b))
                  regs->ulRegSR |= F_ZERO;
               }
            else
               {
               ul = M68KReadEA(regs, ucOpMode, 0, TRUE);
               ul ^= ucTemp;
               if (ul & 0x80)
                  regs->ulRegSR |= F_NEGATIVE;
               if (!(ul & 0xff))
                  regs->ulRegSR |= F_ZERO;
               M68KWriteEA(ucOpMode, 0, ul, regs, TRUE);
               }
            }
         break;
      case 0x29: // EORI word
         if (usOpcode == 0xa7c) // EORI to SR
            {
            iClocks -= 20;
            PC += 2;
            ul = regs->ulRegSR ^ NEWMOTOSHORT(&pOpcode[2]);
            M68KSETSR(regs, ul);
            Check68KInterrupts(regs);
            }
         else
            {
            iClocks -= 8;
            PC += 2;
            usTemp = NEWMOTOSHORT(&pOpcode[2]);
            pOpcode += 2;
            regs->ulRegSR &= ~(F_ZERO | F_NEGATIVE | F_CARRY | F_OVERFLOW);
            if (ucOpMode < 8) // if working with a data register, do it faster
               {
               ulRegister = usOpcode & 7;
               regs->ulRegData[ulRegister].w ^= usTemp;
               if (regs->ulRegData[ulRegister].w & 0x8000)
                  regs->ulRegSR |= F_NEGATIVE;
               if (!(regs->ulRegData[ulRegister].w))
                  regs->ulRegSR |= F_ZERO;
               }
            else
               {
               ul = M68KReadEA(regs, ucOpMode, 1, TRUE);
               ul ^= usTemp;
               if (ul & 0x8000)
                  regs->ulRegSR |= F_NEGATIVE;
               if (!(ul & 0xffff))
                  regs->ulRegSR |= F_ZERO;
               M68KWriteEA(ucOpMode, 1, ul, regs, TRUE);
               }
            }
         break;
      case 0x2a: // EORI long
         iClocks -= 16;
         PC += 4;
         ulTemp = NEWMOTOLONG(&pOpcode[2]);
         pOpcode += 4;
         regs->ulRegSR &= ~(F_ZERO | F_NEGATIVE | F_CARRY | F_OVERFLOW);
         if (ucOpMode < 8) // if working with a data register, do it faster
            {
            ulRegister = usOpcode & 7;
            regs->ulRegData[ulRegister].l ^= ulTemp;
            if (regs->ulRegData[ulRegister].l & 0x80000000)
               regs->ulRegSR |= F_NEGATIVE;
            if (!(regs->ulRegData[ulRegister].l))
               regs->ulRegSR |= F_ZERO;
            }
         else
            {
            ul = M68KReadEA(regs, ucOpMode, 2, TRUE);
            ul ^= ulTemp;
            if (ul & 0x80000000)
               regs->ulRegSR |= F_NEGATIVE;
            if (!ul)
               regs->ulRegSR |= F_ZERO;
            M68KWriteEA(ucOpMode, 2, ul, regs, TRUE);
            }
         break;
      case 0x2b: // CAS
      case 0x33:
      case 0x3b:
         Invalid();
         break;
      case 0x30: // CMPI byte
         iClocks -= 8;
         PC += 2;
         ucTemp = pOpcode[2];
         pOpcode += 2;
         ul = M68KReadEA(regs, ucOpMode, 0, FALSE);
         M68KCMP(regs, ucTemp, ul, 0);
         break;
      case 0x31: // CMPI word
         iClocks -= 8;
         PC += 2;
         usTemp = NEWMOTOSHORT(&pOpcode[2]);
         pOpcode += 2;
         ul = M68KReadEA(regs, ucOpMode, 1, FALSE);
         M68KCMP(regs, usTemp, ul, 1);
         break;
      case 0x32: // CMPI long
         iClocks -= 14;
         PC += 4;
         ulTemp = NEWMOTOLONG(&pOpcode[2]);
         pOpcode += 4;
         ul = M68KReadEA(regs, ucOpMode, 2, FALSE);
         M68KCMP(regs, ulTemp, ul, 2);
         break;
      case 0x38: // MOVES
      case 0x39:
      case 0x3a:
         if (!(regs->ulRegSR & F_SUPERVISOR))
            M68KTRAP(regs, 8); // privilege violation
         else
            {
            Invalid(); // DEBUG
            }
         break;
      case 4: // dynamic tst d0
      case 0xc: // dynamic tst d1
      case 0x14: // dynamic tst d2
      case 0x1c: // dynamic tst d3
      case 0x24: // dynamic tst d4
      case 0x2c: // dynamic tst d5
      case 0x34: // dynamic tst d6
      case 0x3c: // dynamic tst d7
         if ((usOpcode & 0x38) == 8) // MOVEP
            {
            M68KMOVEP(regs, usOpcode);
            }
         else
            {
            iClocks -= 6;
            ucTemp = regs->ulRegData[(usOpcode >> 9) & 7].b; // get the bit # from a data register
            regs->ulRegSR &= ~F_ZERO;
            if ((usOpcode & 0x38) != 0) // byte sized
               {
               ucTemp &= 7;
               ul = M68KReadEA(regs, ucOpMode, 0, FALSE);
               if (!(ul & (1<<ucTemp))) // if bit tested is zero, set Z flag
                  regs->ulRegSR |= F_ZERO;
               }
            else // long size for data register
               {
               ucTemp &= 31;
               ulRegister = usOpcode & 7;
               if (!(regs->ulRegData[ulRegister].l & (1<<ucTemp))) // if bit tested is zero, set Z flag
                  regs->ulRegSR |= F_ZERO;
               }
            }
         break;
      case 5: // dynamic chg d0
      case 0xd: // dynamic chg d1
      case 0x15: // dynamic chg d2
      case 0x1d: // dynamic chg d3
      case 0x25: // dynamic chg d4
      case 0x2d: // dynamic chg d5
      case 0x35: // dynamic chg d6
      case 0x3d: // dynamic chg d7
         if ((usOpcode & 0x38) == 8) // MOVEP
            {
            M68KMOVEP(regs, usOpcode);
            }
         else
            {
            iClocks -= 8;
            ucTemp = regs->ulRegData[(usOpcode >> 9) & 7].b; // get the bit # from a data register
            regs->ulRegSR &= ~F_ZERO;
            if ((usOpcode & 0x38) != 0) // byte sized
               {
               ucTemp &= 7;
               ul = M68KReadEA(regs, ucOpMode, 0, TRUE);
               if (!(ul & (1<<ucTemp))) // if bit tested is zero, set Z flag
                  regs->ulRegSR |= F_ZERO;
               ul ^= (1 << ucTemp); // change the bit
               M68KWriteEA(ucOpMode, 0, ul, regs, TRUE);
               }
            else // long size for data register
               {
               ucTemp &= 31;
               ulRegister = usOpcode & 7;
               if (!(regs->ulRegData[ulRegister].l & (1<<ucTemp))) // if bit tested is zero, set Z flag
                  regs->ulRegSR |= F_ZERO;
               regs->ulRegData[ulRegister].l ^= (1 << ucTemp);
               }
            }
         break;
      case 6: // dynamic clr d0
      case 0xe: // dynamic clr d1
      case 0x16: // dynamic clr d2
      case 0x1e: // dynamic clr d3
      case 0x26: // dynamic clr d4
      case 0x2e: // dynamic clr d5
      case 0x36: // dynamic clr d6
      case 0x3e: // dynamic clr d7
         if ((usOpcode & 0x38) == 8) // MOVEP
            {
            M68KMOVEP(regs, usOpcode);
            }
         else
            {
            iClocks -= 8;
            ucTemp = regs->ulRegData[(usOpcode >> 9) & 7].b; // get the bit # from a data register
            regs->ulRegSR &= ~F_ZERO;
            if ((usOpcode & 0x38) != 0) // byte sized
               {
               ucTemp &= 7;
               ul = M68KReadEA(regs, ucOpMode, 0, TRUE);
               if (!(ul & (1<<ucTemp))) // if bit tested is zero, set Z flag
                  regs->ulRegSR |= F_ZERO;
               ul &= ~(1 << ucTemp); // clr the bit
               M68KWriteEA(ucOpMode, 0, ul, regs, TRUE);
               }
            else // long size for data register
               {
               ucTemp &= 31;
               ulRegister = usOpcode & 7;
               if (!(regs->ulRegData[ulRegister].l & (1<<ucTemp))) // if bit tested is zero, set Z flag
                  regs->ulRegSR |= F_ZERO;
               regs->ulRegData[ulRegister].l &= ~(1 << ucTemp);
               }
            }
         break;
      case 7: // dynamic set d0
      case 0xf: // dynamic set d1
      case 0x17: // dynamic set d2
      case 0x1f: // dynamic set d3
      case 0x27: // dynamic set d4
      case 0x2f: // dynamic set d5
      case 0x37: // dynamic set d6
      case 0x3f: // dynamic set d7
         if ((usOpcode & 0x38) == 8) // MOVEP
            {
            M68KMOVEP(regs, usOpcode);
            }
         else // dynamic bit
            {
            iClocks -= 8;
            ucTemp = regs->ulRegData[(usOpcode >> 9) & 7].b; // get the bit # from a data register
            regs->ulRegSR &= ~F_ZERO;
            if ((usOpcode & 0x38) != 0) // byte sized
               {
               ucTemp &= 7;
               ul = M68KReadEA(regs, ucOpMode, 0, TRUE);
               if (!(ul & (1<<ucTemp))) // if bit tested is zero, set Z flag
                  regs->ulRegSR |= F_ZERO;
               ul |= (1 << ucTemp); // set the bit
               M68KWriteEA(ucOpMode, 0, ul, regs, TRUE);
               }
            else // long size for data register
               {
               ucTemp &= 31;
               ulRegister = usOpcode & 7;
               if (!(regs->ulRegData[ulRegister].l & (1<<ucTemp))) // if bit tested is zero, set Z flag
                  regs->ulRegSR |= F_ZERO;
               regs->ulRegData[ulRegister].l |= (1 << ucTemp);
               }
            }
         break;

      default: // illegal
         Invalid();
         break;
      }
} /* M68KMisc2() */

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : M68KMOVEM(REGS68K *, unsigned long)                        *
 *                                                                          *
 *  PURPOSE    : Move multiple registers.                                   *
 *                                                                          *
 ****************************************************************************/
void M68KMOVEM(REGS68K *regs, unsigned long usOpcode)
{
unsigned short usMask;
unsigned char ucSize, ucMode, ucModeReg, ucCount;
unsigned long ulAddr;
register int i;

   usMask = NEWMOTOSHORT(&pOpcode[2]); // get the bit field specifying which registers to use
   ucCount = 0; // number of registers transferred
   PC += 2;
   pOpcode += 2;

   ucMode = (char)((usOpcode >> 3) & 0x7); // EA mode
   ucModeReg = (char)(usOpcode & 0x3f); // EA Mode and register bits
   if (usOpcode & 0x40)
      ucSize = 2; // long
   else
      ucSize = 1; // word
   if (usOpcode & 0x400) // direction
      { // memory to register
      if (ucMode == 3)
         ulAddr = regs->ulRegAddr[ucModeReg & 7].l;
      else
         ulAddr = M68KGetEA(regs, ucModeReg);
      for (i=0; i<16; i++) // registers to transfer
         {
         if (usMask & (1 << i))
            {
            ucCount++;
            if (i < 8) // data regs
               {
               if (ucSize == 1)
                  regs->ulRegData[i].l = (signed short)M68KRead(ulAddr, 1);
               else
                  regs->ulRegData[i].l = M68KRead(ulAddr, 2);
               }
            else // address regs
               {
               if (ucSize == 1)
                  regs->ulRegAddr[i-8].l = (signed short)M68KRead(ulAddr, 1);
               else
                  regs->ulRegAddr[i-8].l = M68KRead(ulAddr, 2);
               }
            ulAddr += (long)ucIncrement[ucSize];
            }
         }
      if (ucMode == 3) // post increment address register
         regs->ulRegAddr[ucModeReg & 7].l = ulAddr; // store the updated value back
      }
   else // register to memory
      {
      if (ucMode == 4) // predecriment
         {
         ulAddr = regs->ulRegAddr[ucModeReg & 7].l;
         for (i=0; i<16; i++) // registers to transfer
            {
            if (usMask & (1 << i))
               {
               ucCount++;
               ulAddr -= ucIncrement[ucSize];
               if (i < 8) // address regs
                  {
                  M68KWrite(ulAddr, regs->ulRegAddr[7-i].l, ucSize);
                  }
               else // data regs
                  {
                  M68KWrite(ulAddr, regs->ulRegData[15-i].l, ucSize);
                  }
               }
            }
         regs->ulRegAddr[ucModeReg & 7].l = ulAddr; // store the updated value back
         }
      else // control alterable addressing modes (postincrement)
         {
         ulAddr = M68KGetEA(regs, ucModeReg);
         for (i=0; i<16; i++) // registers to transfer
            {
            if (usMask & (1 << i))
               {
               ucCount++;
               if (i < 8) // address regs
                  {
                  M68KWrite(ulAddr, regs->ulRegData[i].l, ucSize);
                  }
               else // data regs
                  {
                  M68KWrite(ulAddr, regs->ulRegAddr[i-8].l, ucSize);
                  }
               ulAddr += ucIncrement[ucSize];
               }
            }
         }
      }
   // Subtract clock cycles for the number of words transferred
   if (ucSize < 2)
      iClocks -= ucCount*4;
   else
      iClocks -= ucCount*8;
} /* M68KMOVEM() */

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : M68KEXT(REGS68K *, unsigned long)                          *
 *                                                                          *
 *  PURPOSE    : Sign extend a data register.                               *
 *                                                                          *
 ****************************************************************************/
void M68KEXT(REGS68K *regs, unsigned long usOpcode)
{
register char ucRegister = (char)usOpcode & 7;

   iClocks -= 4;
   regs->ulRegSR &= ~(F_NEGATIVE | F_ZERO | F_OVERFLOW | F_CARRY);

   switch (usOpcode & 0x1c0)
      {
      case 0x80: // byte to word
         if (regs->ulRegData[ucRegister].b & 0x80)
            {
            regs->ulRegData[ucRegister].w |= 0xff00;
            regs->ulRegSR |= F_NEGATIVE;
            }
         else
            {
            regs->ulRegData[ucRegister].w &= 0x00ff;
            if (regs->ulRegData[ucRegister].w == 0)
               regs->ulRegSR |= F_ZERO;
            }
         break;
      case 0xc0: // word to long
         if (regs->ulRegData[ucRegister].w & 0x8000)
            {
            regs->ulRegData[ucRegister].l |= 0xffff0000;
            regs->ulRegSR |= F_NEGATIVE;
            }
         else
            {
            regs->ulRegData[ucRegister].l &= 0x0000ffff;
            if (regs->ulRegData[ucRegister].l == 0)
               regs->ulRegSR |= F_ZERO;
            }
         break;
      case 0x1c0: // byte to long
         if (regs->ulRegData[ucRegister].b & 0x80)
            {
            regs->ulRegData[ucRegister].l |= 0xffffff00;
            regs->ulRegSR |= F_NEGATIVE;
            }
         else
            {
            regs->ulRegData[ucRegister].l &= 0x000000ff;
            if (regs->ulRegData[ucRegister].l == 0)
               regs->ulRegSR |= F_ZERO;
            }
         break;
      }
} /* M68KEXT() */

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : M68KMisc(REGS68K *, unsigned long)                         *
 *                                                                          *
 *  PURPOSE    : Instructions starting with 0x4000.                         *
 *                                                                          *
 ****************************************************************************/
void M68KMisc(REGS68K *regs, unsigned long usOpcode)
{
register unsigned long ul;
register unsigned char ucSize, ucRegister;

   switch ((usOpcode >> 6)  & 0x3f) // decode bits 6-11
      {
      case 0x0: // NEGX byte
         iClocks -= 4;
         ul = M68KReadEA(regs, (char)(usOpcode & 0x3f), 0, TRUE);
         ucRegister = (ul & 0x80) ? 1:0; // set "overflow" if starting negative
         ul = 0-ul;
         if (regs->ulRegSR & F_EXTEND) // subtract extend bit
            ul--;
         regs->ulRegSR &= ~(F_CARRY + F_OVERFLOW + F_NEGATIVE + F_EXTEND);
         if (ucRegister && (ul & 0x80))
            regs->ulRegSR |= F_OVERFLOW;
         if ((ul & 0xff) == 0)
            regs->ulRegSR |= F_EXTEND | F_CARRY;
         else
            regs->ulRegSR &= ~F_ZERO;  // NB: only cleared, not set
         if (ul & 0x80)
            regs->ulRegSR |= F_NEGATIVE;
         M68KWriteEA((char)(usOpcode & 0x3f), 0, ul, regs, TRUE);
         break;
      case 0x1: // NEGX word
         iClocks -= 4;
         ul = M68KReadEA(regs, (char)(usOpcode & 0x3f), 1, TRUE);
         ucRegister = (ul & 0x8000) ? 1:0; // set "overflow" if starting negative
         ul = 0-ul;
         if (regs->ulRegSR & F_EXTEND) // subtract extend bit
            ul--;
         regs->ulRegSR &= ~(F_CARRY + F_OVERFLOW + F_NEGATIVE + F_EXTEND);
         if (ucRegister && (ul & 0x8000))
            regs->ulRegSR |= F_OVERFLOW;
         if ((ul & 0xffff) == 0)
            regs->ulRegSR |= F_EXTEND | F_CARRY;
         else
            regs->ulRegSR &= ~F_ZERO;  // NB: only cleared, not set
         if (ul & 0x8000)
            regs->ulRegSR |= F_NEGATIVE;
         M68KWriteEA((char)(usOpcode & 0x3f), 1, ul, regs, TRUE);
         break;
      case 0x2: // NEGX long
         iClocks -= 6;
         ul = M68KReadEA(regs, (char)(usOpcode & 0x3f), 2, TRUE);
         ucRegister = (ul & 0x80000000) ? 1:0; // set "overflow" if starting negative
         ul = 0-ul;
         if (regs->ulRegSR & F_EXTEND) // subtract extend bit
            ul--;
         regs->ulRegSR &= ~(F_CARRY + F_OVERFLOW + F_NEGATIVE + F_EXTEND);
         if (ucRegister && (ul & 0x80000000))
            regs->ulRegSR |= F_OVERFLOW;
         if (ul == 0)
            regs->ulRegSR |= F_EXTEND | F_CARRY;
         else
            regs->ulRegSR &= ~F_ZERO;  // NB: only cleared, not set
         if (ul & 0x80000000)
            regs->ulRegSR |= F_NEGATIVE;
         M68KWriteEA((char)(usOpcode & 0x3f), 2, ul, regs, TRUE);
         break;

      case 0x3: // MOVE from SR
         iClocks -= 6;
         M68KWriteEA((char)(usOpcode & 0x3f), 1, regs->ulRegSR, regs, FALSE); // Write SR to EA
         break;

      case 0x8: // CLR byte
      case 0x9: // CLR word
         iClocks -= 4;
         regs->ulRegSR &= ~(F_CARRY + F_OVERFLOW + F_NEGATIVE);
         regs->ulRegSR |= F_ZERO;
         M68KWriteEA((char)(usOpcode & 0x3f), (char)((usOpcode >> 6) & 3), 0, regs, FALSE);
         break;
      case 0xa: // CLR long
         iClocks -= 6;
         regs->ulRegSR &= ~(F_CARRY + F_OVERFLOW + F_NEGATIVE);
         regs->ulRegSR |= F_ZERO;
         M68KWriteEA((char)(usOpcode & 0x3f), (char)((usOpcode >> 6) & 3), 0, regs, FALSE);
         break;

      case 0xb: // MOVE from CCR
         iClocks -= 6;
         ul = regs->ulRegSR & 0xff;
         M68KWriteEA((char)(usOpcode & 0x3f), 1, ul, regs, FALSE); // Write CCR to EA
         break;

      case 0x10: // NEG byte
         iClocks -= 4;
         ul = M68KReadEA(regs, (char)(usOpcode & 0x3f), 0, TRUE);
         regs->ulRegSR &= ~(F_OVERFLOW + F_ZERO + F_NEGATIVE);
         ul = M68KSUBb(regs, ul, 0);
         M68KWriteEA((char)(usOpcode & 0x3f), 0, ul, regs, TRUE);
         break;
      case 0x11: // NEG word
         iClocks -= 4;
         ul = M68KReadEA(regs, (char)(usOpcode & 0x3f), 1, TRUE);
         regs->ulRegSR &= ~(F_OVERFLOW + F_ZERO + F_NEGATIVE);
         ul = M68KSUBw(regs, ul, 0);
         M68KWriteEA((char)(usOpcode & 0x3f), 1, ul, regs, TRUE);
         break;
      case 0x12: // NEG long
         iClocks -= 6;
         ul = M68KReadEA(regs, (char)(usOpcode & 0x3f), 2, TRUE);
         regs->ulRegSR &= ~(F_OVERFLOW + F_ZERO + F_NEGATIVE);
         ul = M68KSUBl(regs, ul, 0);
         M68KWriteEA((char)(usOpcode & 0x3f), 2, ul, regs, TRUE);
         break;

      case 0x13: // MOVE to CCR
         iClocks -= 12;
         ul = M68KReadEA(regs, (char)(usOpcode & 0x3f), 1, FALSE); // read a word from EA
         regs->ulRegSR &= 0xff00;
         regs->ulRegSR |= (ul & 0xff); // only affect lower 8 bits (CCR)
         break;

      case 0x18: // NOT byte
         iClocks -= 4;
         ul = M68KReadEA(regs, (char)(usOpcode & 0x3f), 0, TRUE);
         ul = ~ul;
         regs->ulRegSR &= ~(F_CARRY + F_OVERFLOW + F_ZERO + F_NEGATIVE);
         regs->ulRegSR |= FlagsNZB(ul);
         M68KWriteEA((char)(usOpcode & 0x3f), 0, ul, regs, TRUE);
         break;
      case 0x19: // NOT word
         iClocks -= 4;
         ul = M68KReadEA(regs, (char)(usOpcode & 0x3f), 1, TRUE);
         ul = ~ul;
         regs->ulRegSR &= ~(F_CARRY + F_OVERFLOW + F_ZERO + F_NEGATIVE);
         regs->ulRegSR |= FlagsNZW(ul);
         M68KWriteEA((char)(usOpcode & 0x3f), 1, ul, regs, TRUE);
         break;
      case 0x1a: // NOT long
         iClocks -= 6;
         ul = M68KReadEA(regs, (char)(usOpcode & 0x3f), 2, TRUE);
         ul = ~ul;
         regs->ulRegSR &= ~(F_CARRY + F_OVERFLOW + F_ZERO + F_NEGATIVE);
         regs->ulRegSR |= FlagsNZL(ul);
         M68KWriteEA((char)(usOpcode & 0x3f), 2, ul, regs, TRUE);
         break;

      case 0x1b: // MOVE to SR
         iClocks -= 12;
         if (!(regs->ulRegSR & F_SUPERVISOR))
            M68KTRAP(regs, 8); // privilege violation
         else
            {
            ul = M68KReadEA(regs, (char)(usOpcode & 0x3f), 1, FALSE); // read a word from EA
            M68KSETSR(regs, ul);
            Check68KInterrupts(regs);
            }
         break;

      case 0x20: // LINK long
         if ((usOpcode & 0x38) == 0x8) // it's LINK long
            {
            iClocks -= 20;
            ucRegister = (char)(usOpcode & 7);
            M68K_PUSHDWORD(regs, regs->ulRegAddr[ucRegister].l);
            regs->ulRegAddr[ucRegister].l = regs->ulRegAddr[7].l;
            regs->ulRegAddr[7].l += NEWMOTOLONG(&pOpcode[2]);
            PC += 4;
            }
         else // NBCD
            {
            iClocks -= 4;
            ul = M68KReadEA(regs, (char)(usOpcode & 0x3f), 0, TRUE); // read a byte from EA
            ul = 0x100 - ul - (regs->ulRegSR & F_EXTEND) ? 1: 0;
            if (ul & 0x80000000) // negative
               regs->ulRegSR |= (F_CARRY | F_EXTEND);
            else
               regs->ulRegSR &= ~(F_CARRY | F_EXTEND);
            if (ul != 0)
               regs->ulRegSR &= ~F_ZERO;  // only cleared, not set
            // Adjust for valid BCD range
            if ((ul & 0xf) > 9)
               ul += 6;
            if ((ul & 0xf0) > 0x90)
               ul += 0x60;
            M68KWriteEA((char)(usOpcode & 0x3f), 0, ul, regs, TRUE);
            }
         break;

      case 0x21: // PEA / SWAP
         if (usOpcode & 0x38) // if EA mode is non-zero, then it's PEA
            {
            iClocks -= 10;
            ul = M68KGetEA(regs, (char)(usOpcode & 0x3f)); // get the EA
            M68K_PUSHDWORD(regs, ul);
            }
         else // SWAP
            {
            iClocks -= 4;
            ul = (regs->ulRegData[usOpcode & 7].l) >> 16; // get data register upper half
            ul |= (regs->ulRegData[usOpcode & 7].w) << 16; // swap
            regs->ulRegData[usOpcode & 7].l = ul;
            regs->ulRegSR &= ~(F_CARRY + F_OVERFLOW + F_ZERO + F_NEGATIVE);
            regs->ulRegSR |= FlagsNZL(ul);
            }
         break;

      case 0x22: // MOVEM regs to EA / EXTW / EXTL
      case 0x23:
      case 0x32:
      case 0x33:
         if ((usOpcode & 0x38) == 0) // EXT/EXTB
            M68KEXT(regs, usOpcode);
         else
            M68KMOVEM(regs, usOpcode);
         break;

      case 0x28: // TST byte
         iClocks -= 4;
         regs->ulRegSR &= ~(F_NEGATIVE | F_ZERO | F_OVERFLOW | F_CARRY);
         ul = M68KReadEA(regs, (char)(usOpcode & 0x3f), 0, FALSE); // get a byte from EA
         regs->ulRegSR |= FlagsNZB(ul);
         break;
      case 0x29: // TST word
         iClocks -= 4;
         regs->ulRegSR &= ~(F_NEGATIVE | F_ZERO | F_OVERFLOW | F_CARRY);
         ul = M68KReadEA(regs, (char)(usOpcode & 0x3f), 1, FALSE); // get a word from EA
         regs->ulRegSR |= FlagsNZW(ul);
         break;
      case 0x2a: // TST long
         iClocks -= 4;
         regs->ulRegSR &= ~(F_NEGATIVE | F_ZERO | F_OVERFLOW | F_CARRY);
         ul = M68KReadEA(regs, (char)(usOpcode & 0x3f), 2, FALSE); // get a long from EA
         regs->ulRegSR |= FlagsNZL(ul);
         break;

      case 0x2b: // TAS
         if (usOpcode == 0x4afc) // ILLEGAL
            Invalid();
         else
            {
            iClocks -= 4;
            regs->ulRegSR &= ~(F_NEGATIVE | F_ZERO | F_OVERFLOW | F_CARRY);
            ul = M68KReadEA(regs, (char)(usOpcode & 0x3f), 0, TRUE); // get a byte from EA
            regs->ulRegSR |= FlagsNZB(ul);
            ul |= 0x80; // set high bit of operand and write it back
            M68KWriteEA((char)(usOpcode & 0x3f), 0, ul, regs, TRUE);
            }
         break;

      case 0x39: // lots of instructions
         switch (usOpcode & 0x3f)
            {
            case 0x00: // TRAP
            case 0x01:
            case 0x02:
            case 0x03:
            case 0x04:
            case 0x05:
            case 0x06:
            case 0x07:
            case 0x08:
            case 0x09:
            case 0x0a:
            case 0x0b:
            case 0x0c:
            case 0x0d:
            case 0x0e:
            case 0x0f:
               iClocks -= 34;
               M68KTRAP(regs, 32 + (usOpcode & 15)); // get the exception vector offset
               break;

            case 0x10: // LINK word
            case 0x11:
            case 0x12:
            case 0x13:
            case 0x14:
            case 0x15:
            case 0x16:
            case 0x17:
               iClocks -= 16;
               ucRegister = (char)(usOpcode & 7);
               M68K_PUSHDWORD(regs, regs->ulRegAddr[ucRegister].l);
               regs->ulRegAddr[ucRegister].l = regs->ulRegAddr[7].l;
               regs->ulRegAddr[7].l += (signed short)NEWMOTOSHORT(&pOpcode[2]);
               PC += 2;
               break;

            case 0x18: // UNLK
            case 0x19:
            case 0x1a:
            case 0x1b:
            case 0x1c:
            case 0x1d:
            case 0x1e:
            case 0x1f:
               iClocks -= 12;
               ucRegister = (char)(usOpcode & 0x7);
               regs->ulRegAddr[7].l = regs->ulRegAddr[ucRegister].l;
               regs->ulRegAddr[ucRegister].l = M68K_POPDWORD(regs);
               break;

            case 0x20: // MOVE to USP
            case 0x21:
            case 0x22:
            case 0x23:
            case 0x24:
            case 0x25:
            case 0x26:
            case 0x27:
               iClocks -= 4;
               if (!(regs->ulRegSR & F_SUPERVISOR)) // must be supervisor mode
                  {
                  M68KTRAP(regs, 8); // privilege violation
                  }
               else
                  {
                  regs->ulRegUSP = regs->ulRegAddr[usOpcode & 7].l;
                  }
               break;

            case 0x28: // MOVE from USP
            case 0x29:
            case 0x2a:
            case 0x2b:
            case 0x2c:
            case 0x2d:
            case 0x2e:
            case 0x2f:
               iClocks -= 4;
               if (!(regs->ulRegSR & F_SUPERVISOR)) // must be supervisor mode
                  {
                  M68KTRAP(regs, 8); // privilege violation
                  }
               else
                  {
                  regs->ulRegAddr[usOpcode & 7].l = regs->ulRegUSP;
                  }
               break;

            case 0x30: // RESET
//               iClocks -= 512;
               iClocks = 0; // use this as a patch to exit "busy" loops
               break;

            case 0x31: // NOP
               iClocks -= 4;
               break;

            case 0x32: // STOP
               if (regs->ulRegSR & F_SUPERVISOR)
                  {
                  PC += 2; // skip data word
                  ul = NEWMOTOSHORT(&pOpcode[2]);
                  M68KSETSR(regs, ul);
                  ucSize = (char)((regs->ulRegSR >> 8) & 7); // current interrupt level
                  if (*ucIRQs > (1<<ucSize) || *ucIRQs & 0x80) // interrupt will occur, allow it
                     Check68KInterrupts(regs);
                  else
                     {
                     PC -= 4; // stick on this instruction until an interrupt occurs
                     regs->bStopped = TRUE;
                     iClocks = 0; // nothing else to do, exit
                     }
                  }
               else
                  {
                  M68KTRAP(regs, 8); // privilege violation
                  }
               break;

            case 0x33: // RTE
               iClocks -= 20;
               if (regs->ulRegSR & F_SUPERVISOR)
                  {
                  ul = M68K_POPWORD(regs);
                  PC = M68K_POPDWORD(regs);
                  if (regs->ucCPUType == 0x10)
                     M68K_POPWORD(regs); // format word gets tossed
                  M68KSETSR(regs, ul);
                  Check68KInterrupts(regs);
                  }
               else
                  {
                  M68KTRAP(regs, 8); // privilege violation
                  }
               break;

            case 0x34: // RTD
               iClocks -= 20;
               PC = M68K_POPDWORD(regs);
               regs->ulRegAddr[7].l += (signed short)NEWMOTOSHORT(&pOpcode[2]);
               break;

            case 0x35: // RTS
               iClocks -= 16;
               PC = M68K_POPDWORD(regs);
               break;

            case 0x36: // TRAPV
               iClocks -= 4;
               if (regs->ulRegSR & F_OVERFLOW)
                  {
                  iClocks -= 30;
                  M68KTRAP(regs, 7);
                  }
               break;

            case 0x37: // RTR
               iClocks -= 20;
               ul = M68K_POPWORD(regs);
               regs->ulRegSR &= 0xff00; // affect only condition codes
               regs->ulRegSR |= (ul & 0xff);
               PC = M68K_POPDWORD(regs);
               break;

            case 0x3a: // MOVEC
            case 0x3b:
               Invalid();
               break;

            default: // Illegal instruction
               Invalid();
               break;
            }
         break;

      case 0x3a: // JSR
         iClocks -= 16;
         ul = M68KGetEA(regs, (char)(usOpcode & 0x3f));
         M68K_PUSHDWORD(regs, PC); // push latest PC which points to next instruction
         PC = ul;
         break;

      case 0x3b: // JMP
         iClocks -= 8;
         PC = M68KGetEA(regs, (char)(usOpcode & 0x3f));
         break;

      case 0x04: // CHK
      case 0x06:
      case 0x0c:
      case 0x0e:
      case 0x14:
      case 0x16:
      case 0x1c:
      case 0x1e:
      case 0x24:
      case 0x26:
      case 0x2c:
      case 0x2e:
      case 0x34:
      case 0x36:
      case 0x3c:
      case 0x3e:
         iClocks -= 10;
         ucRegister = (char)((usOpcode >> 9) & 7);
         regs->ulRegSR &= ~F_NEGATIVE;
         if (usOpcode & 0x80) // word
            {
            ul = M68KReadEA(regs, (char)(usOpcode & 0x3f), 1, FALSE);
            if (regs->ulRegData[ucRegister].w & 0x8000 || regs->ulRegData[ucRegister].w > ul)
               {
               regs->ulRegSR |= F_NEGATIVE;
               M68KTRAP(regs, 6);
               }
            }
         else // long
            {
            ul = M68KReadEA(regs, (char)(usOpcode & 0x3f), 2, FALSE);
            if (regs->ulRegData[ucRegister].l & 0x80000000 || regs->ulRegData[ucRegister].l > ul)
               {
               regs->ulRegSR |= F_NEGATIVE;
               M68KTRAP(regs, 6);
               }
            }
         break;

      case 0x07: // LEA
      case 0x0f:
      case 0x17:
      case 0x1f:
      case 0x27:
      case 0x2f:
      case 0x37:
      case 0x3f:
         regs->ulRegAddr[(usOpcode >> 9) & 7].l = M68KGetEA(regs, (char)(usOpcode & 0x3f));
         break;

      default: // illegal instruction
         Invalid();
         break;
      }
} /* M68KMisc() */

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : M68KEORb()                                                 *
 *                                                                          *
 *  PURPOSE    : Exclusive OR.                                .             *
 *                                                                          *
 ****************************************************************************/
_inline unsigned char M68KEORb(REGS68K *regs, unsigned long ulData1, unsigned long ulData2)
{
register unsigned long ul;

   ul = ulData1 ^ ulData2;
   regs->ulRegSR &= ~(F_NEGATIVE | F_ZERO | F_OVERFLOW | F_CARRY);
   if (ul & 0x80)
      regs->ulRegSR |= F_NEGATIVE;
   if (!(ul & 0xff))
      regs->ulRegSR |= F_ZERO;
   return (char)ul;
} /* M68KEORb() */

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : M68KEORw()                                                 *
 *                                                                          *
 *  PURPOSE    : Exclusive OR.                                .             *
 *                                                                          *
 ****************************************************************************/
_inline unsigned short M68KEORw(REGS68K *regs, unsigned long ulData1, unsigned long ulData2)
{
register unsigned long ul;

   ul = ulData1 ^ ulData2;
   regs->ulRegSR &= ~(F_NEGATIVE | F_ZERO | F_OVERFLOW | F_CARRY);
   if (ul & 0x8000)
      regs->ulRegSR |= F_NEGATIVE;
   if (!(ul & 0xffff))
      regs->ulRegSR |= F_ZERO;
   return (short)ul;
} /* M68KEORw() */

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : M68KEORl()                                                 *
 *                                                                          *
 *  PURPOSE    : Exclusive OR.                                .             *
 *                                                                          *
 ****************************************************************************/
_inline unsigned long M68KEORl(REGS68K *regs, unsigned long ulData1, unsigned long ulData2)
{
register unsigned long ul;

   ul = ulData1 ^ ulData2;
   regs->ulRegSR &= ~(F_NEGATIVE | F_ZERO | F_OVERFLOW | F_CARRY);
   if (ul & 0x80000000)
      regs->ulRegSR |= F_NEGATIVE;
   if (!ul)
      regs->ulRegSR |= F_ZERO;
   return ul;
} /* M68KEORl() */

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : M68KRotateByte()                                           *
 *                                                                          *
 *  PURPOSE    : Perform a rotate/shift operation.            .             *
 *                                                                          *
 ****************************************************************************/
unsigned long M68KRotateByte(REGS68K *regs, unsigned long ulData, char ucType, char ucDirection, char ucCount)
{
int i;
unsigned long ul1, ul2, ulLastBit;
unsigned char uc, ucExtend;

   ulData &= 0xff;
   ucExtend = (char)(regs->ulRegSR & F_EXTEND);

   if (ucCount)
      {
      if (ucType != 3) // rotate without extend
         regs->ulRegSR &= ~F_EXTEND;
      regs->ulRegSR &= ~(F_CARRY | F_OVERFLOW | F_NEGATIVE | F_ZERO);
      if (ucDirection) // left
         {
         uc = 8 - ucCount;
         ulLastBit = (ulData >> uc) & 1;
         }
      else // right
         ulLastBit = (ulData >> (ucCount-1)) & 1;
      if (ulLastBit)
         {
         regs->ulRegSR |= F_CARRY;
         if (ucType != 3) // rotate without extend
            regs->ulRegSR |= F_EXTEND;
         }
      }
   else
      {
      regs->ulRegSR &= ~(F_CARRY | F_OVERFLOW | F_NEGATIVE | F_ZERO);
      if (ucType == 2 && ucExtend) // rotate with extend copies extend bit to carry when rotate count == 0
         regs->ulRegSR |= F_CARRY;
      ul1 = ulData; // no change to data
      goto rotate_done;
      }

   switch (ucType)
      {
      case 0: // arithmetic
         ucCount &= 15; // could shift all off the end
         if (ucDirection) // left
            {
            // we need to be sensitive to the overflow bit which is set if the MSB changes at ANY time during the shift
            ul1 = ulData;
            ul2 = ul1 & 0x80; // we only care about the MSB
            for (i=0; i<ucCount; i++) // scan for changing bits
               {
               ul1 <<= 1;
               if ((ul1 & 0x80) != ul2) // overflow
                  regs->ulRegSR |= F_OVERFLOW;
               }
//            ul1 = ulData << ucCount;
            }
         else // right
            {
            if (ulData & 0x80) // if high bit set, drag it across
               {
               ul2 = 0xff - (0xff >> ucCount); // get the high bits
               ul1 = (ulData >> ucCount) | ul2;
               }
            else
               ul1 = ulData >> ucCount;
            }
         break;
      case 1: // logical
         ucCount &= 15;
         if (ucDirection) // left
            ul1 = ulData << ucCount;
         else
            ul1 = ulData >> ucCount;
         break;
      case 2: // rotate with extend
         ucCount &= 7;
         if (ucDirection) // left
            {
         	ul2 = ul1 = ulData;
          	ul1 <<= ucCount;
          	ul2 >>= (8 - ucCount);
          	ul1 |= ul2;
            ul1 &= ~(1 << (ucCount-1));
            if (ucExtend)
               ul1 |= (1 << (ucCount-1));
            }
         else // right
            {
	         ul2 = ul1 = ulData;
	         if (ucCount)
               {
	            ul2 <<= (8 - ucCount);
	            ul1 >>= ucCount;
               ul1 |= ul2;
               ul1 &= ~(1 << (8-ucCount));
               if (ucExtend)
	               ul1 |= (1 << (8-ucCount));
               }
            }
         break;
      case 3: // rotate
         ucCount &= 7;
         if (ucDirection) // left
            {
         	ul2 = ul1 = ulData;
    	      ul2 >>= (8 - ucCount);
    	      ul1 <<= ucCount;
          	ul1 |= ul2;
            }
         else // right
            {
         	ul2 = ul1 = ulData;
    	      ul2 <<= (8 - ucCount);
    	      ul1 >>= ucCount;
    	      ul1 |= ul2;
            }
         break;
      }
rotate_done:
   if (ul1 & 0x80)
      regs->ulRegSR |= F_NEGATIVE;
   if ((ul1 & 0xff) == 0)
      regs->ulRegSR |= F_ZERO;
   return ul1;
} /* M68KRotateByte() */

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : M68KRotateWord()                                           *
 *                                                                          *
 *  PURPOSE    : Perform a rotate/shift operation.            .             *
 *                                                                          *
 ****************************************************************************/
unsigned long M68KRotateWord(REGS68K *regs, unsigned long ulData, char ucType, char ucDirection, char ucCount)
{
int i;
unsigned long ul1, ul2, ulLastBit;
unsigned char uc, ucExtend;

   ulData &= 0xffff;
   ucExtend = (char)(regs->ulRegSR & F_EXTEND);

   if (ucCount)
      {
      if (ucType != 3) // rotate without extend
         regs->ulRegSR &= ~F_EXTEND;
      regs->ulRegSR &= ~(F_CARRY | F_OVERFLOW | F_NEGATIVE | F_ZERO);
      if (ucDirection) // left
         {
         uc = 16 - ucCount;
         ulLastBit = (ulData >> uc) & 1;
         }
      else // right
         ulLastBit = (ulData >> (ucCount-1)) & 1;
      if (ulLastBit)
         {
         regs->ulRegSR |= F_CARRY;
         if (ucType != 3) // rotate without extend
            regs->ulRegSR |= F_EXTEND;
         }
      }
   else
      {
      regs->ulRegSR &= ~(F_CARRY | F_OVERFLOW | F_NEGATIVE | F_ZERO);
      if (ucType == 2 && ucExtend) // rotate with extend copies extend bit to carry when rotate count == 0
         regs->ulRegSR |= F_CARRY;
      ul1 = ulData; // no change to data
      goto rotate_done;
      }

   switch (ucType)
      {
      case 0: // arithmetic
         ucCount &= 31;
         if (ucDirection) // left
            {
            // we need to be sensitive to the overflow bit which is set if the MSB changes at ANY time during the shift
            ul1 = ulData;
            ul2 = ul1 & 0x8000; // we only care about the MSB
            for (i=0; i<ucCount; i++) // scan for changing bits
               {
               ul1 <<= 1;
               if ((ul1 & 0x8000) != ul2) // overflow
                  regs->ulRegSR |= F_OVERFLOW;
               }
//            ul1 = ulData << ucCount;
            }
         else // right
            {
            if (ulData & 0x8000) // if high bit set, drag it across
               {
               ul2 = 0xffff - (0xffff >> ucCount); // get the high bits
               ul1 = (ulData >> ucCount) | ul2;
               }
            else
               ul1 = ulData >> ucCount;
            }
         break;
      case 1: // logical
         ucCount &= 31;
         if (ucDirection) // left
            ul1 = ulData << ucCount;
         else
            ul1 = ulData >> ucCount;
         break;
      case 2: // rotate with extend
         ucCount &= 15;
         if (ucDirection) // left
            {
         	ul2 = ul1 = ulData;
          	ul1 <<= ucCount;
          	ul2 >>= (16 - ucCount);
          	ul1 |= ul2;
            ul1 &= ~(1 << (ucCount-1));
            if (ucExtend)
               ul1 |= (1<<(ucCount-1));
            }
         else // right
            {
	         ul2 = ul1 = ulData;
	         if (ucCount)
               {
	            ul2 <<= (16 - ucCount);
	            ul1 >>= ucCount;
               ul1 |= ul2;
               ul1 &= ~(1 << (16-ucCount));
               if (ucExtend)
	               ul1 |= (1 << (16-ucCount));
               }
            }
         break;
      case 3: // rotate
         ucCount &= 15;
         if (ucDirection) // left
            {
	         ul2 = ul1 = ulData;
    	      ul2 >>= (16 - ucCount);
    	      ul1 <<= ucCount;
          	ul1 |= ul2;
            }
         else // right
            {
	         ul2 = ul1 = ulData;
    	      ul2 <<= (16 - ucCount);
    	      ul1 >>= ucCount;
    	      ul1 |= ul2;
            }
         break;
      }
rotate_done:
   if (ul1 & 0x8000)
      regs->ulRegSR |= F_NEGATIVE;
   if ((ul1 & 0xffff) == 0)
      regs->ulRegSR |= F_ZERO;
   return ul1;
} /* M68KRotateWord() */

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : M68KRotateLong()                                           *
 *                                                                          *
 *  PURPOSE    : Perform a rotate/shift operation.            .             *
 *                                                                          *
 ****************************************************************************/
unsigned long M68KRotateLong(REGS68K *regs, unsigned long ulData, char ucType, char ucDirection, char ucCount)
{
int i;
unsigned long ul1, ul2, ulLastBit;
unsigned char uc, ucExtend;

   ucExtend = (char)(regs->ulRegSR & F_EXTEND);

   if (ucCount)
      {
      if (ucType != 3) // rotate without extend
         regs->ulRegSR &= ~F_EXTEND;
      regs->ulRegSR &= ~(F_CARRY | F_OVERFLOW | F_NEGATIVE | F_ZERO);
      if (ucDirection) // left
         {
         uc = 32 - ucCount;
         ulLastBit = (ulData >> uc) & 1;
         }
      else // right
         ulLastBit = (ulData >> (ucCount-1)) & 1;
      if (ulLastBit)
         {
         regs->ulRegSR |= F_CARRY;
         if (ucType != 3) // rotate without extend doesn't affect extend bit
            regs->ulRegSR |= F_EXTEND;
         }
      }
   else
      {
      regs->ulRegSR &= ~(F_CARRY | F_OVERFLOW | F_NEGATIVE | F_ZERO);
      if (ucType == 2 && ucExtend) // rotate with extend copies extend bit to carry when rotate count == 0
         regs->ulRegSR |= F_CARRY;
      ul1 = ulData; // no change to the data
      goto rotate_done; // no need to rotate since no count
      }

   switch (ucType)
      {
      case 0: // arithmetic
         if (ucDirection) // left
            {
            // we need to be sensitive to the overflow bit which is set if the MSB changes at ANY time during the shift
            ul1 = ulData;
            ul2 = ul1 & 0x80000000; // we only care about the MSB
            for (i=0; i<ucCount; i++) // scan for changing bits
               {
               ul1 <<= 1;
               if ((ul1 & 0x80000000) != ul2) // overflow
                  regs->ulRegSR |= F_OVERFLOW;
               }
//            ul1 = ulData << ucCount;
            }
         else // right
            {
            if (ulData & 0x80000000) // if high bit set, drag it across
               {
               ul2 = 0xffffffff - (0xffffffff >> ucCount); // get the high bits
               ul1 = (ulData >> ucCount) | ul2;
               }
            else
               {
               if (ucCount >= 32)
                  ul1 = 0; // see note below
               else
                  ul1 = ulData >> ucCount;
               }
            }
         break;
      case 1: // logical
         if (ucCount >= 32) // NOTE: result should be 0, but the C >> operator treats it like a shift of 0
            ul1 = 0;
         else
            {
            if (ucDirection) // left
               ul1 = ulData << ucCount;
            else
               ul1 = ulData >> ucCount;
            }
         break;
      case 2: // rotate with extend
         ucCount &= 31;
         if (ucDirection) // left
            {
         	ul2 = ul1 = ulData;
            ul1 <<= ucCount;
          	ul2 >>= (32 - ucCount);
          	ul1 |= ul2;
            ul1 &= ~(1 << (ucCount-1));
            if (ucExtend)
               ul1 |= (1<<(ucCount-1));
            }
         else // right
            {
	         ul2 = ul1 = ulData;
	         if (ucCount)
               {
	            ul2 <<= (32 - ucCount);
	            ul1 >>= ucCount;
               ul1 |= ul2;
               ul1 &= ~(1 << (32-ucCount));
               if (ucExtend)
	               ul1 |= (1 << (32-ucCount));
               }
            }
         break;
      case 3: // rotate
         ucCount &= 31;
         if (ucDirection) // left
            {
         	ul2 = ul1 = ulData;
    	      ul2 >>= (32 - ucCount);
    	      ul1 <<= ucCount;
          	ul1 |= ul2;
            }
         else // right
            {
	         ul2 = ul1 = ulData;
            ul2 <<= (32 - ucCount);
            ul1 >>= ucCount;
            ul1 |= ul2;
            }
         break;
      }

rotate_done:
   if (ul1 & 0x80000000)
      regs->ulRegSR |= F_NEGATIVE;
   if (ul1 == 0)
      regs->ulRegSR |= F_ZERO;
   return ul1;
} /* M68KRotateLong() */

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : EOps(REGS68K *, long)                                      *
 *                                                                          *
 *  PURPOSE    : Opcodes beginning with E000 (Shift/Rotate)   .             *
 *                                                                          *
 ****************************************************************************/
void EOps(REGS68K *regs, unsigned long usOpcode)
{
register unsigned long ul;
register unsigned char ucCount, ucDirection, ucType, ucSize, ucCountReg, ucRegister;

   ucCountReg = (char)((usOpcode >> 9) & 7);
   ucRegister = (char)(usOpcode & 7);
   ucDirection = (char)((usOpcode & 0x100) >> 8);
   if ((usOpcode & 0xc0) == 0xc0) // shift/rotate memory
      {
      ul = M68KReadEA(regs, (char)(usOpcode & 0x3f), 1, TRUE); // always a word sized operand
      ucType = (char)((usOpcode >> 9) & 3);
      ul = M68KRotateWord(regs, ul, ucType, ucDirection, 1); // count is always 1 and size is always word
      M68KWriteEA((char)(usOpcode & 0x3f), 1, ul, regs, TRUE); // write back the result
      iClocks -= (8 - ucEAClocks_bw[usOpcode & 0x3f]); // don't count the EA twice
      }
   else // shift/rotate register
      {
      ucSize = (char)((usOpcode >> 6) & 3);
      ucType = (char)((usOpcode >> 3) & 3);
      if (usOpcode & 0x20) // data register has count
         {
         ucCount = regs->ulRegData[(usOpcode >> 9) & 7].b & 63; // count cannot be greater than 63
         }
      else // immediate count value
         {
         ucCount = (char)lQuickVal[(usOpcode >> 9) & 7];
         }
      switch (ucSize)
         {
         case 0: // byte
            iClocks -= (6 + 2*ucCount);
            regs->ulRegData[ucRegister].b = (char)M68KRotateByte(regs, regs->ulRegData[ucRegister].l, ucType, ucDirection, ucCount);
            break;
         case 1: // word
            iClocks -= (6 + 2*ucCount);
            regs->ulRegData[ucRegister].w = (short)M68KRotateWord(regs, regs->ulRegData[ucRegister].l, ucType, ucDirection, ucCount);
            break;
         case 2: // long
            iClocks -= (8 + 2*ucCount);
            regs->ulRegData[ucRegister].l = M68KRotateLong(regs, regs->ulRegData[ucRegister].l, ucType, ucDirection, ucCount);
            break;
         }
      }

} /* EOps() */

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : DOps(REGS68K *, long)                                      *
 *                                                                          *
 *  PURPOSE    : Opcodes beginning with D000 (ADD, ADDA, ADDX).             *
 *                                                                          *
 ****************************************************************************/
void DOps(REGS68K *regs, unsigned long usOpcode)
{
register unsigned long ul, ul2, ulX;
register unsigned char ucSize, ucRegister;


   ucRegister = (char)((usOpcode >> 9) & 7);

   if ((usOpcode & 0x130) == 0x100 && (usOpcode & 0xc0) != 0xc0) // ADDX
      {
      ulX = (regs->ulRegSR & F_EXTEND) ? 1:0;
      ucSize = (char)((usOpcode >> 6) & 3);
      if (usOpcode & 8) // R/M
         { // memory to memory
         regs->ulRegAddr[ucRegister].l -= ucIncrement[ucSize];
         regs->ulRegAddr[usOpcode & 7].l -= ucIncrement[ucSize];
         ul = M68KRead(regs->ulRegAddr[ucRegister].l, ucSize); // pre-decrement
         ul2 = M68KRead(regs->ulRegAddr[usOpcode & 7].l, ucSize); // pre-decrement
         ul = M68KADDX(regs, ul, ul2, ulX, ucSize);
         M68KWrite(regs->ulRegAddr[ucRegister].l, ul, ucSize);
         iClocks -= (ucSize == 2) ? 30 : 18;
         }
      else
         { // data reg to data reg
         switch (ucSize)
            {
            case 0: // byte
               iClocks -= 4;
               regs->ulRegData[ucRegister].b = (char)M68KADDX(regs, regs->ulRegData[ucRegister].b, regs->ulRegData[usOpcode & 7].b, ulX, 0);
               break;
            case 1: // word
               iClocks -= 4;
               regs->ulRegData[ucRegister].w = (short)M68KADDX(regs, regs->ulRegData[ucRegister].w, regs->ulRegData[usOpcode & 7].w, ulX, 1);
               break;
            case 2: // long
               iClocks -= 8;
               regs->ulRegData[ucRegister].l = M68KADDX(regs, regs->ulRegData[ucRegister].l, regs->ulRegData[usOpcode & 7].l, ulX, 2);
               break;
            }
         }
      }
   else // ADD/ADDA
      {
      switch ((usOpcode & 0x1c0) >> 6) // switch on opmode field for data/address/direction resolution
         {
         case 0: // ea + Dn -> Dn
            iClocks -= 4;
            ul = M68KReadEA(regs, (char)(usOpcode & 0x3f), (char)((usOpcode >> 6) & 3), FALSE);
            ul = M68KADDb(regs, ul, regs->ulRegData[ucRegister].b);
            regs->ulRegData[ucRegister].b = (char)ul;
            break;
         case 1:
            iClocks -= 4;
            ul = M68KReadEA(regs, (char)(usOpcode & 0x3f), (char)((usOpcode >> 6) & 3), FALSE);
            ul = M68KADDw(regs, ul, regs->ulRegData[ucRegister].w);
            regs->ulRegData[ucRegister].w = (short)ul;
            break;
         case 2:
            iClocks -= 8;
            ul = M68KReadEA(regs, (char)(usOpcode & 0x3f), (char)((usOpcode >> 6) & 3), FALSE);
            ul = M68KADDl(regs, ul, regs->ulRegData[ucRegister].l);
            regs->ulRegData[ucRegister].l = ul;
            break;

         case 3: // ea + An -> An (word)
            iClocks -= 8;
            ul = (signed short)M68KReadEA(regs, (char)(usOpcode & 0x3f), 1, FALSE);
            regs->ulRegAddr[ucRegister].l += ul;
            break;
         case 7: // ea + An -> An (long)
            iClocks -= 8;
            ul = M68KReadEA(regs, (char)(usOpcode & 0x3f), 2, FALSE);
            regs->ulRegAddr[ucRegister].l += ul;
            break;

         case 4: // Dn + ea -> ea (byte)
            iClocks -= 4;
            ul = M68KReadEA(regs, (char)(usOpcode & 0x3f), 0, TRUE);
            ul = M68KADDb(regs, ul, regs->ulRegData[ucRegister].l);
            M68KWriteEA((char)(usOpcode & 0x3f), 0, ul, regs, TRUE);
            break;
         case 5: // Dn + ea -> ea (word)
            iClocks -= 4;
            ul = M68KReadEA(regs, (char)(usOpcode & 0x3f), 1, TRUE);
            ul = M68KADDw(regs, ul, regs->ulRegData[ucRegister].l);
            M68KWriteEA((char)(usOpcode & 0x3f), 1, ul, regs, TRUE);
            break;
         case 6: // Dn + ea -> ea (long)
            iClocks -= 8;
            ul = M68KReadEA(regs, (char)(usOpcode & 0x3f), 2, TRUE);
            ul = M68KADDl(regs, ul, regs->ulRegData[ucRegister].l);
            M68KWriteEA((char)(usOpcode & 0x3f), 2, ul, regs, TRUE);
            break;
         }
      }

} /* DOps() */

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : M68KDIVU(REGS68K *, char, long)                            *
 *                                                                          *
 *  PURPOSE    : Unsigned divide.                                   .       *
 *                                                                          *
 ****************************************************************************/
void M68KDIVU(REGS68K *regs, unsigned char ucRegister, unsigned short us)
{
unsigned long ulQuotient, ulRemainder;

   regs->ulRegSR &= ~(F_NEGATIVE | F_ZERO | F_OVERFLOW | F_CARRY);

	if (us != 0)
      {
		ulQuotient = regs->ulRegData[ucRegister].l / us;
		ulRemainder = regs->ulRegData[ucRegister].l % us;

		if (ulQuotient < 0x10000)
         {
         if (ulQuotient == 0)
			   regs->ulRegSR |= F_ZERO;
         if (ulQuotient & 0x8000)
            regs->ulRegSR |= F_NEGATIVE;
			regs->ulRegData[ucRegister].l = (ulQuotient & 0xffff) | (ulRemainder << 16);
   		}
      else
		   regs->ulRegSR |= F_OVERFLOW;
   	}
   else
      M68KTRAP(regs, 5); // divide by zero exception

} /* M68KDIVU() */

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : M68KDIVS(REGS68K *, char, long)                            *
 *                                                                          *
 *  PURPOSE    : Signed divide.                                   .         *
 *                                                                          *
 ****************************************************************************/
void M68KDIVS(REGS68K *regs, unsigned char ucRegister, signed short s)
{
signed long lQuotient, lRemainder;

   regs->ulRegSR &= ~(F_NEGATIVE | F_ZERO | F_OVERFLOW | F_CARRY);

	if (s != 0)
      {
		lQuotient = (signed long)(regs->ulRegData[ucRegister].l) / s;
		lRemainder = (signed long)regs->ulRegData[ucRegister].l % s;

		if (lQuotient >= -0x8000 && lQuotient < 0x8000)
         {
         if (lQuotient == 0)
			   regs->ulRegSR |= F_ZERO;
         if (lQuotient & 0x8000)
            regs->ulRegSR |= F_NEGATIVE;
			regs->ulRegData[ucRegister].l = (lQuotient & 0xffff) | (lRemainder << 16);
   		}
      else
		   regs->ulRegSR |= F_OVERFLOW;
   	}
   else
      M68KTRAP(regs, 5); // divide by zero exception

} /* M68KDIVS() */

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : EightOps(REGS68K *, long)                                  *
 *                                                                          *
 *  PURPOSE    : Opcodes beginning with 8000 (OR, DIV, SBCD)        .       *
 *                                                                          *
 ****************************************************************************/
void EightOps(REGS68K *regs, unsigned long usOpcode)
{
register unsigned long ul;
register unsigned char ucRegister;
register unsigned short usTemp, usTemp2;

   ucRegister = (char)((usOpcode >> 9) & 7);
   switch ((usOpcode >> 4) & 0x1f)
      {
      case 0x0: // OR byte to Dn
      case 0x1:
      case 0x2:
      case 0x3:
         iClocks -= 4;
         regs->ulRegSR &= ~(F_NEGATIVE | F_ZERO | F_OVERFLOW | F_CARRY);
         ul = M68KReadEA(regs, (char)(usOpcode & 0x3f), 0, FALSE);
         ucRegister = (char)((usOpcode >> 9) & 7);
         regs->ulRegData[ucRegister].b |= ul;
         if (regs->ulRegData[ucRegister].b & 0x80)
            regs->ulRegSR |= F_NEGATIVE;
         if (regs->ulRegData[ucRegister].b == 0)
            regs->ulRegSR |= F_ZERO;
         break;
      case 0x4: // OR word to Dn
      case 0x5:
      case 0x6:
      case 0x7:
         iClocks -= 4;
         regs->ulRegSR &= ~(F_NEGATIVE | F_ZERO | F_OVERFLOW | F_CARRY);
         ul = M68KReadEA(regs, (char)(usOpcode & 0x3f), 1, FALSE);
         ucRegister = (char)((usOpcode >> 9) & 7);
         regs->ulRegData[ucRegister].w |= ul;
         if (regs->ulRegData[ucRegister].w & 0x8000)
            regs->ulRegSR |= F_NEGATIVE;
         if (regs->ulRegData[ucRegister].w == 0)
            regs->ulRegSR |= F_ZERO;
         break;
      case 0x8: // OR long to Dn
      case 0x9:
      case 0xa:
      case 0xb:
         iClocks -= 8;
         regs->ulRegSR &= ~(F_NEGATIVE | F_ZERO | F_OVERFLOW | F_CARRY);
         ul = M68KReadEA(regs, (char)(usOpcode & 0x3f), 2, FALSE);
         ucRegister = (char)((usOpcode >> 9) & 7);
         regs->ulRegData[ucRegister].l |= ul;
         if (regs->ulRegData[ucRegister].l & 0x80000000)
            regs->ulRegSR |= F_NEGATIVE;
         if (regs->ulRegData[ucRegister].l == 0)
            regs->ulRegSR |= F_ZERO;
         break;
      case 0x11: // OR byte to EA
      case 0x12:
      case 0x13:
         iClocks -= 4;
         regs->ulRegSR &= ~(F_NEGATIVE | F_ZERO | F_OVERFLOW | F_CARRY);
         ul = M68KReadEA(regs, (char)(usOpcode & 0x3f), 0, TRUE);
         ucRegister = (char)((usOpcode >> 9) & 7);
         ul |= regs->ulRegData[ucRegister].b;
         if (ul & 0x80)
            regs->ulRegSR |= F_NEGATIVE;
         if ((ul & 0xff) == 0)
            regs->ulRegSR |= F_ZERO;
         M68KWriteEA((char)(usOpcode & 0x3f), 0, ul, regs, TRUE);
         break;
      case 0x15: // OR word to EA
      case 0x16:
      case 0x17:
         iClocks -= 4;
         regs->ulRegSR &= ~(F_NEGATIVE | F_ZERO | F_OVERFLOW | F_CARRY);
         ul = M68KReadEA(regs, (char)(usOpcode & 0x3f), 1, TRUE);
         ucRegister = (char)((usOpcode >> 9) & 7);
         ul |= regs->ulRegData[ucRegister].w;
         if (ul & 0x8000)
            regs->ulRegSR |= F_NEGATIVE;
         if ((ul & 0xffff) == 0)
            regs->ulRegSR |= F_ZERO;
         M68KWriteEA((char)(usOpcode & 0x3f), 1, ul, regs, TRUE);
         break;
      case 0x19: // OR long to EA
      case 0x1a:
      case 0x1b:
         iClocks -= 8;
         regs->ulRegSR &= ~(F_NEGATIVE | F_ZERO | F_OVERFLOW | F_CARRY);
         ul = M68KReadEA(regs, (char)(usOpcode & 0x3f), 2, TRUE);
         ucRegister = (char)((usOpcode >> 9) & 7);
         ul |= regs->ulRegData[ucRegister].l;
         if (ul & 0x80000000)
            regs->ulRegSR |= F_NEGATIVE;
         if (ul == 0)
            regs->ulRegSR |= F_ZERO;
         M68KWriteEA((char)(usOpcode & 0x3f), 2, ul, regs, TRUE);
         break;
      case 0xc: // DIVU
      case 0xd:
      case 0xe:
      case 0xf:
         iClocks -= 100; // reasonable guess
         ucRegister = (char)((usOpcode >> 9) & 7);
         ul = M68KReadEA(regs, (char)(usOpcode & 0x3f), 1, FALSE);
         M68KDIVU(regs, ucRegister, (unsigned short)ul);
         break;
      case 0x1c: // DIVS
      case 0x1d:
      case 0x1e:
      case 0x1f:
         iClocks -= 100; // reasonable guess
         ucRegister = (char)((usOpcode >> 9) & 7);
         ul = M68KReadEA(regs, (char)(usOpcode & 0x3f), 1, FALSE);
         M68KDIVS(regs, ucRegister, (signed short)ul);
         break;
      case 0x10: // SBCD
         ucRegister = (char)((usOpcode >> 9) & 7);
         if (usOpcode & 8) // memory to memory
            {
            register char uc1, uc2;
            iClocks -= 18;
            uc1 = (char)M68KRead(--regs->ulRegAddr[usOpcode & 7].l, 0);
            uc2 = (char)M68KRead(--regs->ulRegAddr[ucRegister].l, 0);
            uc1 = M68KSBCD(regs, uc1, uc2);
            M68KWrite(regs->ulRegAddr[ucRegister].l, uc1, 0);
            }
         else // register to register
            {
            iClocks -= 6;
            regs->ulRegData[ucRegister].b = M68KSBCD(regs, regs->ulRegData[usOpcode & 7].b, regs->ulRegData[ucRegister].b);
            }
         break;
      case 0x14: // PACK
         usTemp = NEWMOTOSHORT(&pOpcode[2]);
         PC += 2; // extension word
         if (usOpcode & 8) // memory to memory
            {
            iClocks -= 11;
            }
         else
            { // register to register
            iClocks -= 6;
            usTemp += regs->ulRegData[usOpcode & 7].w;
            usTemp2 = regs->ulRegData[(usOpcode & 0xe00) >> 9].w & 0xff00; // trim off bottom 8 bits
            usTemp2 |= (((usTemp & 0xf00) >> 4) | (usTemp & 0xf)); // pack the 2 nibbles together
            regs->ulRegData[(usOpcode & 0xe00) >> 9].w = usTemp2; // put it back
            }
         break;
      case 0x18: // UNPK
         Invalid(); // DEBUG
         break;
      }

} /* EightOps() */

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : NineOps(REGS68K *, long)                                   *
 *                                                                          *
 *  PURPOSE    : Opcodes beginning with 9000 (SUB, SUBA, SUBX)      .       *
 *                                                                          *
 ****************************************************************************/
void NineOps(REGS68K *regs, unsigned long usOpcode)
{
register unsigned long ul, ul2, ulX;
register unsigned char ucSize, ucRegister;


   ucRegister = (char)((usOpcode >> 9) & 7);

   if ((usOpcode & 0x130) == 0x100 && (usOpcode & 0xc0) != 0xc0) // SUBX
      {
      ulX = (regs->ulRegSR & F_EXTEND) ? 1:0;
      ucSize = (char)((usOpcode >> 6) & 3);
      if (usOpcode & 8) // R/M
         { // memory to memory
         regs->ulRegAddr[ucRegister].l -= ucIncrement[ucSize];
         regs->ulRegAddr[usOpcode & 7].l -= ucIncrement[ucSize];
         ul = M68KRead(regs->ulRegAddr[ucRegister].l, ucSize); // pre-decrement
         ul2 = M68KRead(regs->ulRegAddr[usOpcode & 7].l, ucSize); // pre-decrement
         ul = M68KSUBX(regs, ul2, ul, ulX, ucSize);
         M68KWrite(regs->ulRegAddr[ucRegister].l, ul, ucSize);
         iClocks -= (ucSize == 2) ? 30 : 18;
         }
      else
         { // data reg to data reg
         switch (ucSize)
            {
            case 0: // byte
               regs->ulRegData[ucRegister].b = (char)M68KSUBb(regs, regs->ulRegData[usOpcode & 7].b + ulX, regs->ulRegData[ucRegister].b);
               iClocks -= 4;
               break;
            case 1: // word
               regs->ulRegData[ucRegister].w = (short)M68KSUBw(regs, regs->ulRegData[usOpcode & 7].w + ulX, regs->ulRegData[ucRegister].w);
               iClocks -= 4;
               break;
            case 2: // long
               regs->ulRegData[ucRegister].l = M68KSUBl(regs, regs->ulRegData[usOpcode & 7].l + ulX, regs->ulRegData[ucRegister].l);
               iClocks -= 8;
               break;
            }
         }
      }
   else // SUB/SUBA
      {
      switch ((usOpcode & 0x1c0) >> 6) // switch on opmode field for data/address/direction resolution
         {
         case 0: // Dn - ea -> Dn (byte)
            iClocks -= 4;
            ul = M68KReadEA(regs, (char)(usOpcode & 0x3f), 0, FALSE);
            ul = M68KSUBb(regs, ul, regs->ulRegData[ucRegister].b);
            regs->ulRegData[ucRegister].b = (char)ul;
            break;
         case 1: // Dn - ea -> Dn (word)
            iClocks -= 4;
            ul = M68KReadEA(regs, (char)(usOpcode & 0x3f), 1, FALSE);
            ul = M68KSUBw(regs, ul, regs->ulRegData[ucRegister].w);
            regs->ulRegData[ucRegister].w = (short)ul;
            break;
         case 2: // Dn - ea -> Dn (long)
            iClocks -= 8;
            ul = M68KReadEA(regs, (char)(usOpcode & 0x3f), 2, FALSE);
            ul = M68KSUBl(regs, ul, regs->ulRegData[ucRegister].l);
            regs->ulRegData[ucRegister].l = ul;
            break;

         case 3: // An - ea -> An  (word)
            iClocks -= 8;
            ul = (signed short)M68KReadEA(regs, (char)(usOpcode & 0x3f), 1, FALSE);
            regs->ulRegAddr[ucRegister].l -= ul;
            break;
         case 7: // An - ea -> An (long)
            iClocks -= 8;
            ul = M68KReadEA(regs, (char)(usOpcode & 0x3f), 2, FALSE);
            regs->ulRegAddr[ucRegister].l -= ul;
            break;

         case 4: // ea - Dn -> ea (byte)
            iClocks -= 4;
            ul = M68KReadEA(regs, (char)(usOpcode & 0x3f), 0, TRUE);
            ul = M68KSUBb(regs, regs->ulRegData[ucRegister].l, ul);
            M68KWriteEA((char)(usOpcode & 0x3f), 0, ul, regs, TRUE);
            break;
         case 5: // ea - Dn -> ea (word)
            iClocks -= 4;
            ul = M68KReadEA(regs, (char)(usOpcode & 0x3f), 1, TRUE);
            ul = M68KSUBw(regs, regs->ulRegData[ucRegister].l, ul);
            M68KWriteEA((char)(usOpcode & 0x3f), 1, ul, regs, TRUE);
            break;
         case 6: // ea - Dn -> ea (long)
            iClocks -= 8;
            ul = M68KReadEA(regs, (char)(usOpcode & 0x3f), 2, TRUE);
            ul = M68KSUBl(regs, regs->ulRegData[ucRegister].l, ul);
            M68KWriteEA((char)(usOpcode & 0x3f), 2, ul, regs, TRUE);
            break;
         }
      }

} /* NineOps() */

void M68KCMP(REGS68K *regs, unsigned long ulData1, unsigned long ulData2, unsigned char ucSize)
{
unsigned long ulTemp;

   ulData1 &= ulSizeMasks[ucSize];
   ulData2 &= ulSizeMasks[ucSize];

   ulTemp = ulData2 - ulData1;
   regs->ulRegSR &= ~(F_CARRY | F_ZERO | F_NEGATIVE | F_OVERFLOW);

   switch (ucSize)
      {
      case 0: // Byte
         if (!(ulTemp & 0xff))
            regs->ulRegSR |= F_ZERO;
         if (ulTemp & 0x80)
            regs->ulRegSR |= F_NEGATIVE;
         if (ulTemp & 0x100) // carry/borrow
            regs->ulRegSR |= F_CARRY;
         if ((ulData2^ulData1) & (ulTemp^ulData2) & 0x80)
            regs->ulRegSR |= F_OVERFLOW;
         break;
      case 2: // Long
         if (ulTemp == 0)
            regs->ulRegSR |= F_ZERO;
         if (ulTemp & 0x80000000)
            regs->ulRegSR |= F_NEGATIVE;
         if (ulData2 < ulData1) // will cause a carry
            regs->ulRegSR |= F_CARRY;
         if ((ulData2^ulData1) & (ulTemp^ulData2) & 0x80000000)
            regs->ulRegSR |= F_OVERFLOW;
         break;
      case 1: // Word
         if (!(ulTemp & 0xffff))
            regs->ulRegSR |= F_ZERO;
         if (ulTemp & 0x8000)
            regs->ulRegSR |= F_NEGATIVE;
         if (ulTemp & 0x10000) // carry/borrow
            regs->ulRegSR |= F_CARRY;
         if ((ulData2^ulData1) & (ulTemp^ulData2) & 0x8000)
            regs->ulRegSR |= F_OVERFLOW;
         break;
      }
} /* M68KCMP() */

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : M68KSBCD(REGS68K *, char, char)                            *
 *                                                                          *
 *  PURPOSE    : Subtract 2 packed BCD numbers (bytes).                     *
 *                                                                          *
 ****************************************************************************/
unsigned char M68KSBCD(REGS68K *regs, unsigned char uc1, unsigned char uc2)
{
signed char cTemp;
unsigned char ucBorrow, ucResult;

   cTemp = (uc2 & 0xf) - (uc1 & 0xf);
   if (regs->ulRegSR & F_EXTEND)
      cTemp--;
   if (cTemp < 0) // borrow
      {
      cTemp -= 6;
      ucBorrow = 1;
      }
   else
      ucBorrow = 0;
   ucResult = cTemp & 0xf;
   regs->ulRegSR &= ~(F_CARRY | F_EXTEND | F_OVERFLOW | F_NEGATIVE);
   cTemp = (uc2 >> 4) - (uc1 >> 4) - ucBorrow;
   if (cTemp < 0)
      {
      cTemp += 10;
      regs->ulRegSR |= F_CARRY | F_EXTEND | F_NEGATIVE;
      }
   ucResult |= (cTemp << 4); // combine low and high digit
   if (ucResult)
      regs->ulRegSR &= ~F_ZERO;
   return ucResult;

} /* M68KSBCD() */

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : M68KABCD(REGS68K *, char, char)                            *
 *                                                                          *
 *  PURPOSE    : Add 2 packed BCD numbers (bytes).                          *
 *                                                                          *
 ****************************************************************************/
unsigned char M68KABCD(REGS68K *regs, unsigned char uc1, unsigned char uc2)
{
unsigned short usTemp;

   usTemp = (uc1 & 0xf) + (uc2 & 0xf);
   if (regs->ulRegSR & F_EXTEND)
      usTemp++;
   if (usTemp > 9) // overflow
      usTemp += 6;
   regs->ulRegSR &= ~(F_CARRY | F_EXTEND);
   usTemp += (uc1 & 0xf0) + (uc2 & 0xf0);
   if (usTemp > 0x99)
      {
      usTemp -= 0xa0;
      regs->ulRegSR |= F_CARRY | F_EXTEND;
      }
   if (usTemp)
      regs->ulRegSR &= ~F_ZERO; // zero flag ONLY cleared if non-zero result
   return (unsigned char)usTemp;

} /* M68KABCD() */

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : COps(REGS68K *, long)                                      *
 *                                                                          *
 *  PURPOSE    : Opcodes beginning with C000 (AND, MUL, EXG, ABCD).         *
 *                                                                          *
 ****************************************************************************/
void COps(REGS68K *regs, unsigned long usOpcode)
{
register unsigned long ul;
register unsigned char ucRegister, ucOpMode;
register signed short ss;
register signed long sl;


   ucOpMode = (char)(usOpcode & 0x3f);
   switch ((usOpcode >> 4) & 0x1f)
      {
      case 0x00: // AND (ea) n (Dn) -> (Dn) byte
      case 0x01:
      case 0x02:
      case 0x03:
         ucRegister = (char)((usOpcode >> 9) & 7);
         iClocks -= 4;
         regs->ulRegSR &= ~(F_ZERO | F_NEGATIVE | F_OVERFLOW | F_CARRY);
         ul = M68KReadEA(regs, ucOpMode, 0, FALSE);
         regs->ulRegData[ucRegister].b &= ul;
         if (regs->ulRegData[ucRegister].b & 0x80)
            regs->ulRegSR |= F_NEGATIVE;
         if (!(regs->ulRegData[ucRegister].b & 0xff))
            regs->ulRegSR |= F_ZERO;
         break;

      case 0x04: // AND (ea) n (Dn) -> (Dn) word
      case 0x05:
      case 0x06:
      case 0x07:
         ucRegister = (char)((usOpcode >> 9) & 7);
         iClocks -= 4;
         regs->ulRegSR &= ~(F_ZERO | F_NEGATIVE | F_OVERFLOW | F_CARRY);
         ul = M68KReadEA(regs, ucOpMode, 1, FALSE);
         regs->ulRegData[ucRegister].w &= ul;
         if (regs->ulRegData[ucRegister].w & 0x8000)
            regs->ulRegSR |= F_NEGATIVE;
         if (!(regs->ulRegData[ucRegister].w & 0xffff))
            regs->ulRegSR |= F_ZERO;
         break;

      case 0x08: // AND (ea) n (Dn) -> (Dn) long
      case 0x09:
      case 0x0a:
      case 0x0b:
         ucRegister = (char)((usOpcode >> 9) & 7);
         iClocks -= 8;
         regs->ulRegSR &= ~(F_ZERO | F_NEGATIVE | F_OVERFLOW | F_CARRY);
         ul = M68KReadEA(regs, ucOpMode, 2, FALSE);
         regs->ulRegData[ucRegister].l &= ul;
         if (regs->ulRegData[ucRegister].l & 0x80000000)
            regs->ulRegSR |= F_NEGATIVE;
         if (!regs->ulRegData[ucRegister].l)
            regs->ulRegSR |= F_ZERO;
         break;

      case 0x11: // AND (Dn) n (ea) -> (ea) byte
      case 0x12:
      case 0x13:
         ucRegister = (char)((usOpcode >> 9) & 7);
         iClocks -= 4;
         regs->ulRegSR &= ~(F_ZERO | F_NEGATIVE | F_OVERFLOW | F_CARRY);
         ul = M68KReadEA(regs, ucOpMode, 0, TRUE);
         ul &= regs->ulRegData[ucRegister].b;
         if (ul & 0x80)
            regs->ulRegSR |= F_NEGATIVE;
         if (!(ul & 0xff))
            regs->ulRegSR |= F_ZERO;
         M68KWriteEA(ucOpMode, 0, ul, regs, TRUE);
         break;

      case 0x15: // AND (Dn) n (ea) -> (ea) word
      case 0x16:
      case 0x17:
         ucRegister = (char)((usOpcode >> 9) & 7);
         iClocks -= 4;
         regs->ulRegSR &= ~(F_ZERO | F_NEGATIVE | F_OVERFLOW | F_CARRY);
         ul = M68KReadEA(regs, ucOpMode, 1, TRUE);
         ul &= regs->ulRegData[ucRegister].w;
         if (ul & 0x8000)
            regs->ulRegSR |= F_NEGATIVE;
         if (!(ul & 0xffff))
            regs->ulRegSR |= F_ZERO;
         M68KWriteEA(ucOpMode, 1, ul, regs, TRUE);
         break;

      case 0x19:
      case 0x1a:
      case 0x1b: // AND (Dn) n (ea) -> (ea) long
         ucRegister = (char)((usOpcode >> 9) & 7);
         iClocks -= 8;
         regs->ulRegSR &= ~(F_ZERO | F_NEGATIVE | F_OVERFLOW | F_CARRY);
         ul = M68KReadEA(regs, ucOpMode, 2, TRUE);
         ul &= regs->ulRegData[ucRegister].l;
         if (ul & 0x80000000)
            regs->ulRegSR |= F_NEGATIVE;
         if (!ul)
            regs->ulRegSR |= F_ZERO;
         M68KWriteEA(ucOpMode, 2, ul, regs, TRUE);
         break;

      case 0x0c: // MULU
      case 0x0d:
      case 0x0e:
      case 0x0f:
         iClocks -= 56; // a reasonable guess
         ul = (M68KReadEA(regs, ucOpMode, 1, FALSE) & 0xffff);
         ul = ul * (unsigned short)regs->ulRegData[(usOpcode >> 9) & 7].w;
         regs->ulRegSR &= ~(F_NEGATIVE | F_ZERO | F_OVERFLOW | F_CARRY);
         if (ul & 0x80000000)
            regs->ulRegSR |= F_NEGATIVE;
         if (ul == 0)
            regs->ulRegSR |= F_ZERO;
         regs->ulRegData[(usOpcode >> 9) & 7].l = ul;
         break;
      case 0x1c: // MULS
      case 0x1d:
      case 0x1e:
      case 0x1f:
         iClocks -= 56; // a reasonable guess
         ss = (signed short)M68KReadEA(regs, ucOpMode, 1, FALSE);
         sl = ss * (signed short)regs->ulRegData[(usOpcode >> 9) & 7].w;
         regs->ulRegSR &= ~(F_NEGATIVE | F_ZERO | F_OVERFLOW | F_CARRY);
         if (sl & 0x80000000)
            regs->ulRegSR |= F_NEGATIVE;
         if (sl == 0)
            regs->ulRegSR |= F_ZERO;
         regs->ulRegData[(usOpcode >> 9) & 7].l = sl;
         break;

      case 0x10: // ABCD
         ucRegister = (char)((usOpcode >> 9) & 7);
         if (usOpcode & 8) // memory to memory
            {
            register char uc1, uc2;
            iClocks -= 18;
            uc1 = (char)M68KRead(--regs->ulRegAddr[ucRegister].l, 0);
            uc2 = (char)M68KRead(--regs->ulRegAddr[usOpcode & 7].l, 0);
            uc1 = M68KABCD(regs, uc1, uc2);
            M68KWrite(regs->ulRegAddr[ucRegister].l, uc1, 0);
            }
         else // register to register
            {
            iClocks -= 6;
            regs->ulRegData[ucRegister].b = M68KABCD(regs, regs->ulRegData[ucRegister].b, regs->ulRegData[usOpcode & 7].b);
            }
         break;

      case 0x14: // EXG data or addr
         if (usOpcode & 8) // EXG address
            {
            iClocks -= 6;
            ul = regs->ulRegAddr[usOpcode & 7].l;
            regs->ulRegAddr[usOpcode & 7].l = regs->ulRegAddr[(usOpcode >> 9) & 7].l;
            regs->ulRegAddr[(usOpcode >> 9) & 7].l = ul;
            }
         else // EXG data
            {
            iClocks -= 6;
            ul = regs->ulRegData[usOpcode & 7].l;
            regs->ulRegData[usOpcode & 7].l = regs->ulRegData[(usOpcode >> 9) & 7].l;
            regs->ulRegData[(usOpcode >> 9) & 7].l = ul;
            }
         break;
      case 0x18: // EXG data with addr
         iClocks -= 6;
         ul = regs->ulRegAddr[usOpcode & 7].l;
         regs->ulRegAddr[usOpcode & 7].l = regs->ulRegData[(usOpcode >> 9) & 7].l;
         regs->ulRegData[(usOpcode >> 9) & 7].l = ul;
         break;
      }

} /* COps() */

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : BOps(REGS68K *, long)                                      *
 *                                                                          *
 *  PURPOSE    : Opcodes beginning with B000 (CMP, EOR, CMPM).              *
 *                                                                          *
 ****************************************************************************/
void BOps(REGS68K *regs, unsigned long usOpcode)
{
register unsigned long ul, ul2;
register unsigned char ucRegister, ucOpMode;

   ucOpMode = (char)(usOpcode & 0x3f);

   switch ((usOpcode >> 6) & 7)
      {
      case 0x0: // byte compare
         iClocks -= 4;
         ul = M68KReadEA(regs, ucOpMode, 0, FALSE);
         M68KCMP(regs, ul, regs->ulRegData[((usOpcode >> 9) & 7)].b, 0);
         break;
      case 0x1: // word compare
         iClocks -= 4;
         ul = M68KReadEA(regs, ucOpMode, 1, FALSE);
         M68KCMP(regs, ul, regs->ulRegData[((usOpcode >> 9) & 7)].w, 1);
         break;
      case 0x2: // long compare
         iClocks -= 4;
         ul = M68KReadEA(regs, ucOpMode, 2, FALSE);
         M68KCMP(regs, ul, regs->ulRegData[((usOpcode >> 9) & 7)].l, 2);
         break;
      case 0x3: // CMPA - word
         iClocks -= 6;
         ul = (signed short)M68KReadEA(regs, ucOpMode, 1, FALSE); // sign extend to a long
         M68KCMP(regs, ul, regs->ulRegAddr[((usOpcode >> 9) & 7)].l, 2); // use long compare because it's an address reg
         break;
      case 0x4: // CMPM / EOR byte
         if ((usOpcode & 0x38) == 8) // CMPM
            {
            iClocks -= 12;
            ucRegister = (char)((usOpcode >> 9) & 7); // dest addr reg
            ul2 = M68KRead(regs->ulRegAddr[usOpcode & 7].l, 0); // src addr reg
            regs->ulRegAddr[usOpcode & 7].l += 1; // auto increment
            if ((usOpcode & 7) == 7)
               regs->ulRegAddr[usOpcode & 7].l++; // keep Stack even
            ul = M68KRead(regs->ulRegAddr[ucRegister].l, 0);
            regs->ulRegAddr[ucRegister].l += 1; // auto increment
            if (ucRegister == 7)
               regs->ulRegAddr[ucRegister].l++; // keep Stack even
            M68KCMP(regs, ul2, ul, 0);
            }
         else // EOR - byte
            {
            iClocks -= 4;
            if (ucOpMode < 8) // faster for data regs
               {
               regs->ulRegData[usOpcode & 7].b = M68KEORb(regs, regs->ulRegData[usOpcode & 7].b, regs->ulRegData[(usOpcode >> 9)& 7].b);
               }
            else
               {
               ul = M68KReadEA(regs, ucOpMode, 0, TRUE);
               ul = M68KEORb(regs, ul, regs->ulRegData[(usOpcode >> 9)& 7].b);
               M68KWriteEA(ucOpMode, 0, ul, regs, TRUE);
               }
            }
         break;

      case 0x5: // CMPM / EOR word
         if ((usOpcode & 0x38) == 8) // CMPM
            {
            iClocks -= 12;
            ucRegister = (char)((usOpcode >> 9) & 7); // dest addr reg
            ul2 = M68KRead(regs->ulRegAddr[usOpcode & 7].l, 1); // src addr reg
            regs->ulRegAddr[usOpcode & 7].l += 2; // auto increment
            ul = M68KRead(regs->ulRegAddr[ucRegister].l, 1);
            regs->ulRegAddr[ucRegister].l += 2; // auto increment
            M68KCMP(regs, ul2, ul, 1);
            }
         else // EOR - word
            {
            iClocks -= 4;
            if (ucOpMode < 8) // faster for data regs
               {
               regs->ulRegData[usOpcode & 7].w = M68KEORw(regs, regs->ulRegData[usOpcode & 7].w, regs->ulRegData[(usOpcode >> 9)& 7].w);
               }
            else
               {
               ul = M68KReadEA(regs, ucOpMode, 1, TRUE);
               ul = M68KEORw(regs, ul, regs->ulRegData[(usOpcode >>9)& 7].w);
               M68KWriteEA(ucOpMode, 1, ul, regs, TRUE);
               }
            }
         break;

      case 0x6: // CMPM / EOR long
         if ((usOpcode & 0x38) == 8) // CMPM
            {
            iClocks -= 20;
            ucRegister = (char)((usOpcode >> 9) & 7); // dest addr reg
            ul2 = M68KRead(regs->ulRegAddr[usOpcode & 7].l, 2); // src addr reg
            regs->ulRegAddr[usOpcode & 7].l += 4; // auto increment
            ul = M68KRead(regs->ulRegAddr[ucRegister].l, 2);
            regs->ulRegAddr[ucRegister].l += 4; // auto increment
            M68KCMP(regs, ul2, ul, 2);
            }
         else // EOR - long
            {
            iClocks -= 8;
            if (ucOpMode < 8) // faster for data regs
               {
               regs->ulRegData[usOpcode & 7].l = M68KEORl(regs, regs->ulRegData[usOpcode & 7].l, regs->ulRegData[(usOpcode >> 9)& 7].l);
               }
            else
               {
               ul = M68KReadEA(regs, ucOpMode, 2, TRUE);
               ul = M68KEORl(regs, ul, regs->ulRegData[(usOpcode >>9)& 7].l);
               M68KWriteEA(ucOpMode, 2, ul, regs, TRUE);
               }
            }
         break;

      case 0x7: // CMPA - long
         iClocks -= 6;
         ul = M68KReadEA(regs, ucOpMode, 2, FALSE);
         M68KCMP(regs, ul, regs->ulRegAddr[((usOpcode >> 9) & 7)].l, 2);
         break;
      }
} /* BOps() */


void RegisterFetchHandler(unsigned long u32Address, unsigned long handler)
{
    FetchHandlers[u32Address >> FETCH_BITS] = handler;
}


void EXIT68K(void)
{
   iClocks = 0;
} /* EXIT68K() */

/****************************************************************************
 *                                                                          *
 *  FUNCTION   : EXEC68K(char *, REGS68K *, EMUHANDLERS2 *, int *, char *)  *
 *                                                                          *
 *  PURPOSE    : Emulate the M68000 microprocessor for N clock cycles.      *
 *                                                                          *
 ****************************************************************************/

int trigger = 0;

void EXEC68K(unsigned char *mem, REGS68K *regs, EMUHANDLERS3 *emuh, int *iClockTotal, unsigned char *ucIRQ, int iHack, unsigned long *ulCPUOffs)
{
unsigned int usOpcode;
register unsigned long ulTemp, ulData;
register unsigned char ucOpMode, ucTemp;
//register signed char cTemp;
register unsigned short usTemp;
BOOL b;
register signed long lTemp;
static unsigned long ulTestPC = 0x14e;
static unsigned long ulTraceCount = 0;
#ifdef _DEBUG
unsigned long ulOldPC;
#endif
   mem_handlers68K = emuh; /* Assign to static for faster execution */
   cFlagMap68K = mem; /* ditto */
   pCPUOffs = ulCPUOffs;

   PC = regs->ulRegPC;
   ucIRQs = ucIRQ;
   pOrigClocks = iClockTotal; // keep pointer to update this for real-time operations
   iClocks = *iClockTotal;
   Check68KInterrupts(regs); // try to handle pending interrupts
   while (iClocks > 0) /* Execute for the amount of time alloted */
      {

      pOpcode = (unsigned char *)((PC & 0xffffff) + ulCPUOffs[(PC&0xffffff) >> 16]); // get the indirect offset of this 64K block for faster bank switching
      usOpcode = NEWMOTOSHORT(pOpcode); // we've laid out the memory in little-endian order for faster access

#ifdef COLLECT_HISTO
      Histo.ulOps[usOpcode]++;
#endif
      PC += 2;
      switch (usOpcode & 0xf000) /* separate instructions into 16 classes */
         {
         case 0x0000: /* bit manipulation/movep/immediate */
            M68KMisc2(regs, usOpcode);
            break;
         case 0x1000: /* Move Byte */
            iClocks -= 4;
            ucOpMode = (char)(usOpcode & 0x3f); // if source is just a data reg, do it faster
            if (ucOpMode < 8) // data register
               ucTemp = regs->ulRegData[usOpcode & 7].b;
            else
               ucTemp = (unsigned char)M68KReadEA(regs, ucOpMode, 0, FALSE);
            regs->ulRegSR &= ~(F_CARRY + F_OVERFLOW + F_ZERO + F_NEGATIVE);
            regs->ulRegSR |= FlagsNZB(ucTemp);
            if (usOpcode & 0x1c0) // complex EA
               M68KWriteEA(ucModeTrans[(usOpcode >> 6) & 0x3f], 0, ucTemp, regs, FALSE);
            else // simple data register
               regs->ulRegData[(usOpcode >> 9) & 7].b = ucTemp;
            break;
         case 0x2000: /* Move Long */
            iClocks -= 4;
            ucOpMode = (char)(usOpcode & 0x3f); // if source is just a data reg, do it faster
            if (ucOpMode < 8) // data register
               ulTemp = regs->ulRegData[usOpcode & 7].l;
            else
               ulTemp = M68KReadEA(regs, ucOpMode, 2, FALSE);
            if ((usOpcode & 0x1c0) != 0x40) // MOVEA does not affect CCR
               {
               regs->ulRegSR &= ~(F_CARRY + F_OVERFLOW + F_ZERO + F_NEGATIVE);
               regs->ulRegSR |= FlagsNZL(ulTemp);
               if (usOpcode & 0x1c0) // complex EA
                  M68KWriteEA(ucModeTrans[(usOpcode >> 6) & 0x3f], 2, ulTemp, regs, FALSE);
               else // simple data register
                  regs->ulRegData[(usOpcode >> 9) & 7].l = ulTemp;
               }
            else
                regs->ulRegAddr[(usOpcode >> 9) & 7].l = ulTemp; // MOVEA
            break;
         case 0x3000: /* Move Word */
            iClocks -= 4;
            ucOpMode = (char)(usOpcode & 0x3f); // if source is just a data reg, do it faster
            if (ucOpMode < 8) // data register
               usTemp = regs->ulRegData[usOpcode & 7].w;
            else
               usTemp = (unsigned short)M68KReadEA(regs, ucOpMode, 1, FALSE);
            if ((usOpcode & 0x1c0) != 0x40) // MOVEA does not affect CCR, but sign extends to a long
               {
               regs->ulRegSR &= ~(F_CARRY + F_OVERFLOW + F_ZERO + F_NEGATIVE);
               regs->ulRegSR |= FlagsNZW(usTemp);
               if (usOpcode & 0x1c0) // complex EA
                  M68KWriteEA(ucModeTrans[(usOpcode >> 6) & 0x3f], 1, usTemp, regs, FALSE);
               else // simple data register
                  regs->ulRegData[(usOpcode >> 9) & 7].w = usTemp;
               }
            else
               regs->ulRegAddr[(usOpcode >> 9) & 7].l = (signed short)usTemp; // sign extend to a long for MOVEA
            break;
         case 0x4000: /* Misc */
            M68KMisc(regs, usOpcode); // lots of different instructions
            break;
         case 0x5000: /* ADDQ/SUBQ/Scc/DBcc/TRAPcc */
            switch (usOpcode & 0x1f8) // use these 6 bits to separate the various instructions
               {
               case 0x000: // ADDQ - byte
               case 0x008:
               case 0x010:
               case 0x018:
               case 0x020:
               case 0x028:
               case 0x030:
               case 0x038:
                  ulTemp = lQuickVal[(usOpcode >> 9) & 7];
                  iClocks -= 4;
                  if ((usOpcode & 0x38) == 8) // behaves differently for address registers
                     {
                     regs->ulRegAddr[usOpcode & 7].l += ulTemp; // entire register used and no flags affected
                     }
                  else
                     {
                     ucOpMode = (char)(usOpcode & 0x3f);
                     if (ucOpMode < 8) // do it faster for a data reg (no need to get EA)
                        {
                        regs->ulRegData[usOpcode & 7].b = (char)M68KADDb(regs, ulTemp, regs->ulRegData[usOpcode & 7].b);
                        }
                     else
                        {
                        ulData = M68KReadEA(regs, ucOpMode, 0, TRUE);
                        ulData = M68KADDb(regs, ulTemp, ulData); // add the byte,word or dword
                        M68KWriteEA(ucOpMode, 0, ulData, regs, TRUE);
                        }
                     }
                  break;

               case 0x040: // ADDQ - word
               case 0x048:
               case 0x050:
               case 0x058:
               case 0x060:
               case 0x068:
               case 0x070:
               case 0x078:
                  iClocks -= 4;
                  ulTemp = lQuickVal[(usOpcode >> 9) & 7];
                  if ((usOpcode & 0x38) == 8) // behaves differently for address registers
                     {
                     regs->ulRegAddr[usOpcode & 7].l += ulTemp; // entire register used and no flags affected
                     }
                  else
                     {
                     ucOpMode = (char)(usOpcode & 0x3f);
                     if (ucOpMode < 8) // do it faster for a data reg (no need to get EA)
                        {
                        regs->ulRegData[usOpcode & 7].w = (short)M68KADDw(regs, ulTemp, regs->ulRegData[usOpcode & 7].w);
                        }
                     else
                        {
                        ulData = M68KReadEA(regs, ucOpMode, 1, TRUE);
                        ulData = M68KADDw(regs, ulTemp, ulData); // add the byte,word or dword
                        M68KWriteEA(ucOpMode, 1, ulData, regs, TRUE);
                        }
                     }
                  break;

               case 0x080: // ADDQ - long
               case 0x088:
               case 0x090:
               case 0x098:
               case 0x0a0:
               case 0x0a8:
               case 0x0b0:
               case 0x0b8:
                  iClocks -= 4;
                  ulTemp = lQuickVal[(usOpcode >> 9) & 7];
                  if ((usOpcode & 0x38) == 8) // behaves differently for address registers
                     {
                     regs->ulRegAddr[usOpcode & 7].l += ulTemp; // entire register used and no flags affected
                     }
                  else
                     {
                     ucOpMode = (char)(usOpcode & 0x3f);
                     if (ucOpMode < 8) // do it faster for a data reg (no need to get EA)
                        {
                        regs->ulRegData[usOpcode & 7].l = M68KADDl(regs, ulTemp, regs->ulRegData[usOpcode & 7].l);
                        }
                     else
                        {
                        ulData = M68KReadEA(regs, ucOpMode, 2, TRUE);
                        ulData = M68KADDl(regs, ulTemp, ulData); // add the byte,word or dword
                        M68KWriteEA(ucOpMode, 2, ulData, regs, TRUE);
                        }
                     }
                  break;
               case 0x100: // SUBQ - byte
               case 0x108:
               case 0x110:
               case 0x118:
               case 0x120:
               case 0x128:
               case 0x130:
               case 0x138:
                  iClocks -= 4;
                  ulTemp = lQuickVal[(usOpcode >> 9) & 7];
                  if ((usOpcode & 0x38) == 8) // behaves differently for address registers
                     {
                     regs->ulRegAddr[usOpcode & 7].l -= ulTemp; // entire register used and no flags affected
                     }
                  else
                     {
                     ucOpMode = (char)(usOpcode & 0x3f);
                     if (ucOpMode < 8) // data register, do it faster
                        {
                        regs->ulRegData[usOpcode & 7].b = M68KSUBb(regs, ulTemp, regs->ulRegData[usOpcode & 7].b);
                        }
                     else
                        {
                        ulData = M68KReadEA(regs, ucOpMode, 0, TRUE);
                        ulData = M68KSUBb(regs, ulTemp, ulData); // subtract the byte,word or dword
                        M68KWriteEA(ucOpMode, 0, ulData, regs, TRUE);
                        }
                     }
                  break;
               case 0x140: // SUBQ - word
               case 0x148:
               case 0x150:
               case 0x158:
               case 0x160:
               case 0x168:
               case 0x170:
               case 0x178:
                  iClocks -= 4;
                  ulTemp = lQuickVal[(usOpcode >> 9) & 7];
                  if ((usOpcode & 0x38) == 8) // behaves differently for address registers
                     {
                     regs->ulRegAddr[usOpcode & 7].l -= ulTemp; // entire register used and no flags affected
                     }
                  else
                     {
                     ucOpMode = (char)(usOpcode & 0x3f);
                     if (ucOpMode < 8) // data register, do it faster
                        {
                        regs->ulRegData[usOpcode & 7].w = M68KSUBw(regs, ulTemp, regs->ulRegData[usOpcode & 7].w);
                        }
                     else
                        {
                        ulData = M68KReadEA(regs, ucOpMode, 1, TRUE);
                        ulData = M68KSUBw(regs, ulTemp, ulData); // subtract the byte,word or dword
                        M68KWriteEA(ucOpMode, 1, ulData, regs, TRUE);
                        }
                     }
                  break;
               case 0x180: // SUBQ - long
               case 0x188:
               case 0x190:
               case 0x198:
               case 0x1a0:
               case 0x1a8:
               case 0x1b0:
               case 0x1b8:
                  iClocks -= 4;
                  ulTemp = lQuickVal[(usOpcode >> 9) & 7];
                  if ((usOpcode & 0x38) == 8) // behaves differently for address registers
                     {
                     regs->ulRegAddr[usOpcode & 7].l -= ulTemp; // entire register used and no flags affected
                     }
                  else
                     {
                     ucOpMode = (char)(usOpcode & 0x3f);
                     if (ucOpMode < 8) // data register, do it faster
                        {
                        regs->ulRegData[usOpcode & 7].l = M68KSUBl(regs, ulTemp, regs->ulRegData[usOpcode & 7].l);
                        }
                     else
                        {
                        ulData = M68KReadEA(regs, ucOpMode, 2, TRUE);
                        ulData = M68KSUBl(regs, ulTemp, ulData); // subtract the byte,word or dword
                        M68KWriteEA(ucOpMode, 2, ulData, regs, TRUE);
                        }
                     }
                  break;
               case 0x0c0: // DBcc / Scc
               case 0x0c8:
               case 0x0d0:
               case 0x0d8:
               case 0x0e0:
               case 0x0e8:
               case 0x0f0:
               case 0x1c0:
               case 0x1c8:
               case 0x1d0:
               case 0x1d8:
               case 0x1e0:
               case 0x1e8:
               case 0x1f0:
scc_return:
                  iClocks -= 10;
                  switch (usOpcode & 0x0f00) // test condition code
                     {
                     case 0x0000: // always true
                        b = TRUE;
                        break;
                     case 0x0100: // never true
                        b = FALSE; // assume condition is false
                        break;
                     case 0x0200: // HI
                        b = !(regs->ulRegSR & (F_CARRY | F_ZERO));
                        break;
                     case 0x0300: // LS
                        b = regs->ulRegSR & (F_CARRY | F_ZERO);
                        break;
                     case 0x0400: // CC
                        b = !(regs->ulRegSR & F_CARRY);
                        break;
                     case 0x0500: // CS
                        b = regs->ulRegSR & F_CARRY;
                        break;
                     case 0x0600: // NE
                        b = !(regs->ulRegSR & F_ZERO);
                        break;
                     case 0x0700: // EQ
                        b = regs->ulRegSR & F_ZERO;
                        break;
                     case 0x0800: // VC
                        b = !(regs->ulRegSR & F_OVERFLOW);
                        break;
                     case 0x0900: // VS
                        b = regs->ulRegSR & F_OVERFLOW;
                        break;
                     case 0x0A00: // PL
                        b = !(regs->ulRegSR & F_NEGATIVE);
                        break;
                     case 0x0B00: // MI
                        b = regs->ulRegSR & F_NEGATIVE;
                        break;
                     case 0x0C00: // GE
                        b = (!((regs->ulRegSR & F_NEGATIVE) ^ (regs->ulRegSR & F_OVERFLOW)<<2));
                        break;
                     case 0x0D00: // LT
                        b = ((regs->ulRegSR & F_NEGATIVE) ^ (regs->ulRegSR & F_OVERFLOW)<<2);
                        break;
                     case 0x0E00: // GT
                        b = (!((regs->ulRegSR & F_NEGATIVE) ^ (regs->ulRegSR & F_OVERFLOW)<<2 || regs->ulRegSR & F_ZERO));
                        break;
                     case 0x0F00: // LE
                        b = ((regs->ulRegSR & F_NEGATIVE) ^ (regs->ulRegSR & F_OVERFLOW)<<2 || regs->ulRegSR & F_ZERO);
                        break;
                     }
                  if ((usOpcode & 0xf8) == 0xc8) // DBcc
                     {
                     if (!b && --regs->ulRegData[usOpcode & 7].w != 0xffff) // negative 1 means stop
                        {
                        PC += (signed short)NEWMOTOSHORT(&pOpcode[2]); // take the branch
#ifdef SPEED_HACKS
                        if (NEWMOTOSHORT(&pOpcode[2]) == 0xfffe) // branch to itself
                           {
                           // burn through the remaining clock cycles
                           if (regs->ulRegData[usOpcode & 7].w * 10 > iClocks) // can't do the whole count now
                              {
                              regs->ulRegData[usOpcode & 7].w -= iClocks / 10;
                              iClocks = 0;
                              }
                           else
                              {
                              iClocks -= regs->ulRegData[usOpcode & 7].w * 10;
                              regs->ulRegData[usOpcode & 7].w = 0;
                              }
                           }
#endif // SPEED_HACKS
                        }
                     else
                        {
                        PC += 2; // skip branch offset
                        iClocks -= 2; // takes more time when branch NOT taken
                        }
                     }
                  else // Scc
                     {
                     ulTemp = (b) ? 0xff: 0;
                     iClocks += 6; // since we subtracted 10 above
                     M68KWriteEA((char)(usOpcode & 0x3f), 0, ulTemp, regs, FALSE); // write a byte of 0 or 1
                     }
                  break;
               case 0x0f8:
               case 0x1f8: // TRAPcc
                  if ((usOpcode & 0x7) < 2) // really Scc
                     goto scc_return; // kludge, but it needs to be this way
                  switch (usOpcode & 0x0f00) // test condition code
                     {
                     case 0x0000: // always true
                        b = TRUE;
                        break;
                     case 0x0100: // never true
                        b = FALSE; // assume condition is false
                        break;
                     case 0x0200: // HI
                        b = !(regs->ulRegSR & (F_CARRY | F_ZERO));
                        break;
                     case 0x0300: // LS
                        b = regs->ulRegSR & (F_CARRY | F_ZERO);
                        break;
                     case 0x0400: // CC
                        b = !(regs->ulRegSR & F_CARRY);
                        break;
                     case 0x0500: // CS
                        b = regs->ulRegSR & F_CARRY;
                        break;
                     case 0x0600: // NE
                        b = !(regs->ulRegSR & F_ZERO);
                        break;
                     case 0x0700: // EQ
                        b = regs->ulRegSR & F_ZERO;
                        break;
                     case 0x0800: // VC
                        b = !(regs->ulRegSR & F_OVERFLOW);
                        break;
                     case 0x0900: // VS
                        b = regs->ulRegSR & F_OVERFLOW;
                        break;
                     case 0x0A00: // PL
                        b = !(regs->ulRegSR & F_NEGATIVE);
                        break;
                     case 0x0B00: // MI
                        b = regs->ulRegSR & F_NEGATIVE;
                        break;
                     case 0x0C00: // GE
                        b = (!((regs->ulRegSR & F_NEGATIVE) ^ (regs->ulRegSR & F_OVERFLOW)<<2));
                        break;
                     case 0x0D00: // LT
                        b = ((regs->ulRegSR & F_NEGATIVE) ^ (regs->ulRegSR & F_OVERFLOW)<<2);
                        break;
                     case 0x0E00: // GT
                        b = (!((regs->ulRegSR & F_NEGATIVE) ^ (regs->ulRegSR & F_OVERFLOW)<<2 || regs->ulRegSR & F_ZERO));
                        break;
                     case 0x0F00: // LE
                        b = ((regs->ulRegSR & F_NEGATIVE) ^ (regs->ulRegSR & F_OVERFLOW)<<2 || regs->ulRegSR & F_ZERO);
                        break;
                     } // switch
                  iClocks -= 4;
                  if (b)
                     {
                     M68KTRAP(regs, 7);
                     }
                  break;
               }
            break;
         case 0x6000: /* Bcc */
/* Calculate the relative offset */
            iClocks -= 8;
            ucTemp = (unsigned char)usOpcode;
            lTemp = (signed char)ucTemp;
            if (ucTemp == 0) /* 16-bit offset */
               {
               lTemp = (signed short)NEWMOTOSHORT(&pOpcode[2]) - 2;
               PC += 2;
               iClocks -= 2;
               }
            else
            if (ucTemp == 0xff)  /* 32-bit offset */
               {
               lTemp = NEWMOTOLONG(&pOpcode[2]) - 4;
               iClocks -= 4;
               PC += 4;
               }
            switch (usOpcode & 0x0f00)
               {
               case 0x0000: /* BRA */
                  iClocks -= 2;
                  PC += lTemp;
                  if (lTemp == -2)
                     iClocks = 0; // time wasting loop, exit
                  break;
               case 0x0100: /* BSR */
                  iClocks -= 8;
                  M68K_PUSHDWORD(regs, PC);
                  PC += lTemp;
                  break;
               case 0x0200: /* BHI */
                  if (!(regs->ulRegSR & (F_CARRY | F_ZERO)))
                     {
                     PC += lTemp;
                     iClocks -= 2;
                     }
                  break;
               case 0x0300: /* BLS */
                  if (regs->ulRegSR & (F_CARRY | F_ZERO))
                     {
                     PC += lTemp;
                     iClocks -= 2;
                     }
                  break;
               case 0x0400: /* BCC */
                  if (!(regs->ulRegSR & F_CARRY))
                     {
                     PC += lTemp;
                     iClocks -= 2;
#ifdef SPEED_HACKS
                     if ((unsigned long)lTemp >= 0xfffffffc) // probably a tight loop, break out
                        iClocks = 0;
#endif // SPEED_HACKS
                     }
                  break;
               case 0x0500: /* BCS */
                  if (regs->ulRegSR & F_CARRY)
                     {
                     PC += lTemp;
                     iClocks -= 2;
#ifdef SPEED_HACKS
                     if ((unsigned long)lTemp >= 0xfffffffc) // probably a tight loop, break out
                        iClocks = 0;
#endif // SPEED_HACKS
                     }
                  break;
               case 0x0600: /* BNE */
                  if (!(regs->ulRegSR & F_ZERO))
                     {
                     PC += lTemp;
                     iClocks -= 2;
#ifdef SPEED_HACKS
                     if ((unsigned long)lTemp >= 0xfffffffa) // probably a tight loop, break out
                        iClocks = 0;
#endif // SPEED_HACKS
                     }
                  break;
               case 0x0700: /* BEQ */
                  if (regs->ulRegSR & F_ZERO)
                     {
                     PC += lTemp;
                     iClocks -= 2;
#ifdef SPEED_HACKS
                     if ((unsigned long)lTemp >= 0xfffffffa) // probably a tight loop, break out
                        iClocks = 0;
#endif // SPEED_HACKS
                     }
                  break;
               case 0x0800: /* BVC */
                  if (!(regs->ulRegSR & F_OVERFLOW))
                     {
                     PC += lTemp;
                     iClocks -= 2;
                     }
                  break;
               case 0x0900: /* BVS */
                  if (regs->ulRegSR & F_OVERFLOW)
                     {
                     PC += lTemp;
                     iClocks -= 2;
                     }
                  break;
               case 0x0a00: /* BPL */
                  if (!(regs->ulRegSR & F_NEGATIVE))
                     {
                     PC += lTemp;
                     iClocks -= 2;
#ifdef SPEED_HACKS
                     if ((unsigned long)lTemp >= 0xfffffffa) // probably a tight loop, break out
                        iClocks = 0;
#endif // SPEED_HACKS
                     }
                  break;
               case 0x0b00: /* BMI */
                  if (regs->ulRegSR & F_NEGATIVE)
                     {
                     PC += lTemp;
                     iClocks -= 2;
#ifdef SPEED_HACKS
                     if ((unsigned long)lTemp >= 0xfffffffa) // probably a tight loop, break out
                        iClocks = 0;
#endif // SPEED_HACKS
                     }
                  break;
               case 0x0c00: /* BGE */
                  if (!((regs->ulRegSR & F_NEGATIVE) ^ (regs->ulRegSR & F_OVERFLOW)<<2))
                     {
                     PC += lTemp;
                     iClocks -= 2;
                     }
                  break;
               case 0x0d00: /* BLT */
                  if ((regs->ulRegSR & F_NEGATIVE) ^ (regs->ulRegSR & F_OVERFLOW)<<2)
                     {
                     PC += lTemp;
                     iClocks -= 2;
                     }
                  break;
               case 0x0e00: /* BGT */
                  if (!((regs->ulRegSR & F_NEGATIVE) ^ (regs->ulRegSR & F_OVERFLOW)<<2 || regs->ulRegSR & F_ZERO))
                     {
                     PC += lTemp;
                     iClocks -= 2;
                     }
                  break;
               case 0x0f00: /* BLE */
                  if ((regs->ulRegSR & F_NEGATIVE) ^ (regs->ulRegSR & F_OVERFLOW)<<2 || regs->ulRegSR & F_ZERO)
                     {
                     PC += lTemp;
                     iClocks -= 2;
                     }
                  break;
               } /* switch on Bcc */
            break;
         case 0x7000: /* MOVEQ */
            iClocks -= 4;
            ulTemp = (signed char)(usOpcode);
            regs->ulRegData[(usOpcode >> 9) & 0x7].l = ulTemp;
            regs->ulRegSR &= ~(F_CARRY + F_OVERFLOW + F_ZERO + F_NEGATIVE);
            regs->ulRegSR |= FlagsNZL(ulTemp);
            break;
         case 0x8000: /* OR/DIV/SBCD */
            EightOps(regs, usOpcode);
            break;
         case 0x9000: /* SUB/SUBX */
            NineOps(regs, usOpcode);
            break;
         case 0xB000: /* CMP/CMPM/EOR */
            BOps(regs, usOpcode);
            break;
         case 0xC000: /* AND/MUL/ABCD/EXG */
            COps(regs, usOpcode);
            break;
         case 0xD000: /* ADD/ADDX */
            DOps(regs, usOpcode);
            break;
         case 0xE000: /* Shift/Rotate */
            EOps(regs, usOpcode);
            break;
         case 0xA000: /* Unassigned */
            Invalid();
         case 0xF000: /* Unassigned - special use for "branch and exit" */
/* Calculate the relative offset */
            iClocks -= 8;
            lTemp = (signed char)usOpcode;
            switch (usOpcode & 0x0f00)
               {
               case 0x0000: /* BRA */
                  PC += lTemp;
                  iClocks = 0; // exit this time slice after executing the branch
                  break;
               case 0x0600: /* BNE */
                  if (!(regs->ulRegSR & F_ZERO))
                     {
                     PC += lTemp;
                     iClocks = 0; // exit this time slice after executing the branch
                     }
                  break;
               case 0x0700: /* BEQ */
                  if (regs->ulRegSR & F_ZERO)
                     {
                     PC += lTemp;
                     iClocks = 0; // exit this time slice after executing the branch
                     }
                  break;
               case 0x0a00: /* BPL */
                  if (!(regs->ulRegSR & F_NEGATIVE))
                     {
                     PC += lTemp;
                     iClocks = 0; // exit this time slice after executing the branch
                     }
                  break;
               case 0x0b00: /* BMI */
                  if (regs->ulRegSR & F_NEGATIVE)
                     {
                     PC += lTemp;
                     iClocks = 0; // exit this time slice after executing the branch
                     }
                  break;
               }
            break;
         }
      }
   regs->ulRegPC = PC; // store local var back in structure
   *iClockTotal = iClocks;
} /* EXEC68K() */
