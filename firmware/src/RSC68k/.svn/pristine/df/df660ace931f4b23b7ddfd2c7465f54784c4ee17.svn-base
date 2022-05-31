/* Structures and defines for Larry's Arcade Emulator */
/* Written by Larry Bank */
/* Copyright 1998 BitBank Software, Inc. */
/* Project started 1/7/98 */

// #define PORTABLE /* Used for compiling no ASM code into the project */

#ifndef _EMU_H_
#define _EMU_H_

#if defined _WIN32 || defined BSD
#define EMUDrawSprite16 C_EMUDrawSprite16
#define EMUDrawTile16 C_EMUDrawTile16
#define ARMDraw8_16 C_ARMDraw8_16
#define ARMGFXCopy16 C_ARMGFXCopy16
#define ARMDraw16_16 C_ARMDraw16_16
#else
#define EMUDrawSprite16 A_EMUDrawSprite16
#define EMUDrawTile16 A_EMUDrawTile16
#define ARMDraw8_16 A_ARMDraw8_16
#define ARMGFXCopy16 A_ARMGFXCopy16
#define ARMDraw16_16 A_ARMDraw16_16
#endif

void ARMGFXCopy16(void *pSrc, void *pDest, int iPixelWidth);
void ARMDraw8_16(unsigned char *pSrc, unsigned short *pDest, int iWidth, unsigned short *pColorConvert);
void CPS1FindBlankSprites(unsigned char *pFlags, unsigned char *pSprites, int iCount, int iSpriteLen, unsigned char ucBlank);


/* Menu / command definitions */
#define IDM_DATA    400
#define IDM_UPDATE  401
#define IDM_FILLLIST 402

/* Atari Mathbox Emulator */
void MBGoWrite(unsigned short usAddr, unsigned char ucByte);
unsigned char MBReadLow(unsigned short usAddr);
unsigned char MBReadHigh(unsigned short usAddr);

/* Pending interrupt bits */
#define INT_NMI  1
#define INT_IRQ2 2  // used in HUC6280
#define INT_FIRQ 2
#define INT_IRQ  4
#define INT_IRQ3 8 // used in HUC6280 (timer)
#define INT_OCI  8  /* Output compare interrupt for M6800 family */
#define INT_TOF  16 /* Timer overflow interrupt for M6800 family */

/* Recording and playback of games */
#define RECORD_OFF        0  // not playing or recording
#define RECORD_PAUSED     1  // paused
#define RECORD_PLAYING    2
#define RECORD_RECORDING  3
#define RECORD_LOAD       4
#define RECORD_SAVE       5
#define RECORD_RESET      6

/* Keyboard bit definitions for main program to process */
#define RKEY_UP_P1      0x00000001
#define RKEY_DOWN_P1    0x00000002
#define RKEY_LEFT_P1    0x00000004
#define RKEY_RIGHT_P1   0x00000008
#define RKEY_BUTT1_P1   0x00000010
#define RKEY_BUTT2_P1   0x00000020
#define RKEY_BUTT3_P1   0x00000040
#define RKEY_BUTT4_P1   0x00000080
#define RKEY_BUTT5_P1   0x00000100
#define RKEY_BUTT6_P1   0x00000200
#define RKEY_BUTT7_P1   0x00000400
#define RKEY_BUTT8_P1   0x00000800
#define RKEY_COIN1      0x00001000
#define RKEY_P1_START   0x00002000
#define RKEY_SERVICE1   0x00004000

#define RKEY_UP_P2      0x00010000
#define RKEY_DOWN_P2    0x00020000
#define RKEY_LEFT_P2    0x00040000
#define RKEY_RIGHT_P2   0x00080000
#define RKEY_BUTT1_P2   0x00100000
#define RKEY_BUTT2_P2   0x00200000
#define RKEY_BUTT3_P2   0x00400000
#define RKEY_BUTT4_P2   0x00800000
#define RKEY_BUTT5_P2   0x01000000
#define RKEY_BUTT6_P2   0x02000000
#define RKEY_BUTT7_P2   0x04000000
#define RKEY_BUTT8_P2   0x08000000
#define RKEY_COIN2      0x10000000
#define RKEY_P2_START   0x20000000
#define RKEY_SERVICE2   0x40000000

#define RKEY_ALL_P1  RKEY_UP_P1 | RKEY_DOWN_P1 | RKEY_LEFT_P1 | RKEY_RIGHT_P1 | RKEY_BUTT1_P1 | RKEY_BUTT2_P1 | RKEY_BUTT3_P1 | RKEY_BUTT4_P1 | RKEY_BUTT5_P1 | RKEY_BUTT6_P1
#define RKEY_ALL_P2  RKEY_UP_P2 | RKEY_DOWN_P2 | RKEY_LEFT_P2 | RKEY_RIGHT_P2 | RKEY_BUTT1_P2 | RKEY_BUTT2_P2 | RKEY_BUTT3_P2 | RKEY_BUTT4_P2 | RKEY_BUTT5_P2 | RKEY_BUTT6_P2
#define RKEY_ALL_DIRECTIONS RKEY_ALL_P1 | RKEY_ALL_P2
#define RKEY_ALL RKEY_ALL_DIRECTIONS | RKEY_BUTT1_P1 | RKEY_BUTT2_P1 | RKEY_BUTT1_P2 | RKEY_BUTT2_P2

//efine RKEY_PLAYER1 0xffff // all of player 1's controls
//efine RKEY_PLAYER2 0xffff0000 // all of player 2's controls

/* Memory map emulation offsets */
#define MEM_ROMRAM 0x0000 /* Simplified memory map */
#define MEM_FLAGS 0x10000 /* Offset to flags in memory map */
#define MEM_DECRYPT 0x20000 /* Offset to decrypted opcodes */
#define MEM_DECRYPT2 0x200000
#define MEM_FLAGS2 0x100000 /* Offset to flags in memory map */


/* Definitions for external memory read and write routines */
typedef unsigned short (*MEMRPROC3)(unsigned long);
typedef void (*MEMWPROC3)(unsigned long, unsigned short);
typedef unsigned char (*MEMRPROC2)(unsigned long);
typedef void (*MEMWPROC2)(unsigned long, unsigned char);
typedef unsigned char (*MEMRPROC)(unsigned short);
typedef void (*MEMWPROC)(unsigned short, unsigned char);
typedef void (*GAMEPROC)(TCHAR *, int);
//typedef void (*LISTROMPROC)(HWND, int);
typedef BOOL (*LOADSAVEPROC)(int, BOOL);
typedef unsigned long (*MOUSEPROC)(int x, int y);
typedef int (*RECORDPROC)(BOOL bGetPut, unsigned char *pBuf);

/* Structure for holding queued events such as DAC and fast sound chip updates */
typedef struct tagEMU_EVENT_QUEUE
{
int iCount;
int iFrameTicks; // 1 frame's worth of CPU ticks
unsigned long ulTime[16384]; // clock ticks of each event
unsigned char ucEvent[16384]; // byte event
} EMU_EVENT_QUEUE;

#ifdef BOGUS
/* Game display area definition structure */
typedef struct tagSCREENAREA
{
int iScrollX; /* Offset in pixels from left side of source bitmap */
int iScrollY; /* Offset in pixels from top side of source bitmap */
unsigned long lDirty; /* 32-bit dirty rectangle flags for each area */
int iWidth; /* Pixel width of source bitmap */
int iHeight; /* Pixel height of source bitmap */
unsigned char *pArea; /* Pointer to source bitmaps for each area */
RECT rc; /* Rectangles which define the destination area of the display */
} SCREENAREA;
#endif

/* Microsoft WAVE file header structure */
typedef struct myWAVEHEADER
{
char desc1[4];
char fsize[4];
char desc2[4];
char desc3[4];
char ssize[4];
char stype[2];
char stereo[2];
char samples[4];
char bytesec[4];
char blocklen[2];
char bits[2];
char desc4[4];
char datasize[4];
} WAVEHEADER;

typedef struct t_stars
{
unsigned char color, oldcolor, speed;
unsigned int x, y;
} STARS;

/* Sprite/character display structure */
typedef struct tagSPRITECHAR
{
unsigned char *pSprites;
unsigned char *pChars;
int iSpriteCount;
int iCharCount;
} SPRITECHAR;

/* Graphics decode structure */
typedef struct tagGFXDECODE
{
unsigned char cBitCount; /* Number of bits per tile */
unsigned int iDelta;   /* Bytes per original tile */
unsigned char cWidth, cHeight; /* Width and Height of the tile (8x8, 16x16, 32x32) */
unsigned int iPlanes[8]; /* Offset to each bit plane 0-7 */
unsigned int iBitsX[64], iBitsY[64]; /* Array of x,y coordinate bit offsets */
} GFXDECODE;

/* Sound samples structure */
typedef struct tagSNDSAMPLE16
{
signed short *pSound; /* Pointer to 44.1Khz sample data */
int iLen; /* Length of the sample in bytes */
int iPos; /* Current position of sound cursor */
BOOL bLoop; /* Flag indicating whether sound should loop */
BOOL bActive; /* Flag indicating sound needs to be played */
} SNDSAMPLE16;

/* Sound samples structure */
typedef struct tagSNDSAMPLE
{
signed char *pSound; /* Pointer to 44.1Khz sample data */
int iLen; /* Length of the sample in bytes */
int iPos; /* Current position of sound cursor */
BOOL bLoop; /* Flag indicating whether sound should loop */
BOOL bActive; /* Flag indicating sound needs to be played */
} SNDSAMPLE;
/* Keyboard definition */
typedef struct tagEMUKEYS
{
char *szKeyName; /* ASCII name user sees */
unsigned char ucScancode; /* Put a default value here */
unsigned char ucDefault; /* keyboard default value */
unsigned char ucHotRod; /* hotrod default value */
unsigned long ulKeyMask;  /* defines a bit to return indicating key is down */
} EMUKEYS;

/* ROM Loader structure */
typedef struct tagLOADROM
{
    char *szROMName;
    int  iROMStart;
    int  iROMLen;
    int  iCheckSum;
    MEMRPROC pfn_read;
    MEMWPROC pfn_write;

} LOADROM;

/* Structure to define special memory mapped device handler routines */
typedef struct tagMEMHANDLERS
{
    unsigned short usStart;
    unsigned short usLen;
    MEMRPROC pfn_read;
    MEMWPROC pfn_write;

} MEMHANDLERS;

// Used in 68k machines
typedef struct tagMEMHANDLERS2
{
    unsigned long ulStart;
    unsigned long ulLen;
    MEMRPROC2 pfn_read;
    MEMWPROC2 pfn_write;

} MEMHANDLERS2;

// Used for 68K only
typedef struct tagMEMHANDLERS3
{
    unsigned long ulStart;
    unsigned long ulLen;
    MEMRPROC2 pfn_read8;
    MEMWPROC2 pfn_write8;
    MEMRPROC3 pfn_read16;
    MEMWPROC3 pfn_write16;

} MEMHANDLERS3;

/* Structure to pass to CPU emulator with memory handler routines */
typedef struct tagEMUHANDLERS
{
   MEMRPROC pfn_read;
   MEMWPROC pfn_write;

} EMUHANDLERS;

/* Structure to pass to CPU emulator with memory handler routines */
typedef struct tagEMUHANDLERS2
{
   MEMRPROC2 pfn_read;
   MEMWPROC2 pfn_write;

} EMUHANDLERS2;

/* Structure to pass to CPU emulator with memory handler routines */
typedef struct tagEMUHANDLERS3
{
   MEMRPROC2 pfn_read8;
   MEMWPROC2 pfn_write8;
   MEMRPROC3 pfn_read16;
   MEMWPROC3 pfn_write16;

} EMUHANDLERS3;

#define INTELSHORT(p) *p + *(p+1) * 0x100
#define INTELLONG(p) *p + *(p+1) * 0x100 + *(p+2) * 0x10000 + *(p+3) * 0x1000000
#define MOTOSHORT(p) ((*(p))<<8) + *(p+1)
#define MOTOLONG(p) ((*p)<<24) + ((*(p+1))<<16) + ((*(p+2))<<8) + *(p+3)
#define MOTO24(p) ((*(p))<<16) + ((*(p+1))<<8) + *(p+2)

#define WRITEPATTERN32(p, o, l) p[o] |= (unsigned char)(l >> 24); p[o+1] |= (unsigned char)(l >> 16); p[o+2] |= (unsigned char)(l >> 8); p[o+3] |= (unsigned char)l;
#define WRITEMOTO32(p, o, l) p[o] = (unsigned char)(l >> 24); p[o+1] = (unsigned char)(l >> 16); p[o+2] = (unsigned char)(l >> 8); p[o+3] = (unsigned char)l;
#define WRITEMOTO16(p, o, l) p[o] = (unsigned char)(l >> 8); p[o+1] = (unsigned char)l;

#define INT_EXTERNAL 1
#define INT_TIMER    2
#define  I8039_p0	0x100   /* Not used */
#define  I8039_p1	0x101
#define  I8039_p2	0x102
#define  I8039_p4	0x104
#define  I8039_p5	0x105
#define  I8039_p6	0x106
#define  I8039_p7	0x107
#define  I8039_t0	0x110
#define  I8039_t1	0x111
#define  I8039_bus	0x120
/* 8039 registers */
typedef struct tagREGS8039
{
    unsigned short usRegPC;
    unsigned char  ucRegSP;
    unsigned char  ucRegPSW;
    unsigned char  ucRegA;
    unsigned char  ucRegF1;
    unsigned char  ucRegBank;
    unsigned char  ucTimerOn;
    unsigned char  ucCountOn;
    unsigned char  ucMasterClock;
    unsigned char  ucTimer;
    unsigned char  ucT0;
    unsigned char  ucT1;
    unsigned char  ucXIRQ;
    unsigned char  ucTIRQ;
    unsigned char  ucINT; // interrupt in process
    unsigned char  t_flag; /* Timer interrupt occurred flag */
    unsigned char  ucP1; // port 1 mask
    unsigned char  ucP2; // port 2 mask
    unsigned short usA11; // memory bank selector (address line 11)
    unsigned short usA11ff; // memory bank selector previous value
} REGS8039;

typedef struct tagREGSH6280
{
    unsigned long MMR[8];
    unsigned char  ucFastFlags[8];
    unsigned short usRegPC;
    unsigned char  ucRegX;
    unsigned char  ucRegY;
    unsigned char  ucRegS;
    unsigned char  ucRegA;
    unsigned char  ucRegP;
    unsigned char  ucCPUSpeed;
    unsigned char  ucIRQMask;
    unsigned char  ucIRQState;
    unsigned char  ucTimerStatus;
    unsigned char  ucTimerAck;
    unsigned int iTimerPreload;
    signed int iTimerValue;
    unsigned long ulTemp[16];    // space for ASM temp vars
} REGSH6280;

// Register array for quicker access
#define REG_AX 0
#define REG_CX 1
#define REG_DX 2
#define REG_BX 3
#define REG_SP 4
#define REG_BP 5
#define REG_SI 6
#define REG_DI 7
#define REG_F  8
#define REG_IP 9

#define REG_ES 0
#define REG_CS 1
#define REG_SS 2
#define REG_DS 3

/* 8086 registers */
typedef struct tagREGS8086
{
union
   {
   unsigned short w;
   struct
      {
      unsigned char l;
      unsigned char h;
      } b;
   } usData[10];

//  MYSHORT usData[10]; // AX, BX, CX, DX, F, SP, BP, SI, DI, IP
  unsigned long ulSegments[4]; // CS, DS, SS, ES
  unsigned char ucRegHALT;
  unsigned char ucRegNMI;
  unsigned char ucIRQVal;
  unsigned char ucREP; // repeat flag for string instructions
} REGS8086;

/* 6502 registers */
typedef struct tagREGS6502
{
    unsigned long  ulOffsets[32]; // use for bank switching and ARM temp vars
    unsigned short usRegPC;
    unsigned char  ucRegX;
    unsigned char  ucRegY;
    unsigned char  ucRegS;
    unsigned char  ucRegA;
    unsigned char  ucRegP;
} REGS6502;

/* 68000 registers */
typedef struct tagREGS68K
{
union
   {
   unsigned char b;
   unsigned short w;
   unsigned long l;
   } ulRegData[8];

union
   {
   unsigned char b;
   unsigned short w;
   unsigned long l;
   } ulRegAddr[8];
unsigned long ulRegUSP; /* User stack pointer */
unsigned long ulRegSSP; /* Supervisor stack pointer */
unsigned long ulRegPC; /* Program counter */
unsigned long ulRegSR; /* Status Reg/condition codes */
unsigned long ulRegVBR; /* Vector base register */
unsigned char bStopped;
unsigned char ucCPUType;
unsigned long ulTemp[16];  /* Space for internal vars */
} REGS68K;

/* Z80 registers */
// make sure that struct is packed so that C register layout matches ASM.
#ifndef _WIN32
#ifndef BSD
#define PACKED __packed
#else /* ifndef BSD */
#define PACKED
#define PACKEDSTRUCT __attribute__((__packed__))
#endif /* ifndef BSD */
#else	/* ifndef _WIN32 */
#define PACKED
#define PACKEDSTRUCT
#pragma pack( push, 1 )
#endif /* ifndef _WIN32 */

typedef PACKED struct tagREGSZ80
{
    PACKED union
       {
       unsigned short usRegAF;  // 0
       PACKED struct
          {
          unsigned char ucRegF; /* On a big-endian machine, reverse A & B */
          unsigned char ucRegA;
          } PACKEDSTRUCT ByteRegs;
       } PACKEDSTRUCT RegPairAF;
    unsigned short sdummy1;     // 2
    PACKED union
       {
       unsigned short usRegBC;  // 4
       PACKED struct
          {
          unsigned char ucRegC; /* On a big-endian machine, reverse A & B */
          unsigned char ucRegB;
          } PACKEDSTRUCT ByteRegs;
       } PACKEDSTRUCT RegPairBC;
    unsigned short sdummy2;     // 6
    PACKED union
       {
       unsigned short usRegDE;  // 8
       PACKED struct
          {
          unsigned char ucRegE; /* On a big-endian machine, reverse A & B */
          unsigned char ucRegD;
          } PACKEDSTRUCT ByteRegs;
       } PACKEDSTRUCT RegPairDE;
    unsigned short sdummy3;     // 10
    PACKED union
       {
       unsigned short usRegHL;  // 12
       PACKED struct
          {
          unsigned char ucRegL; /* On a big-endian machine, reverse A & B */
          unsigned char ucRegH;
          } PACKEDSTRUCT ByteRegs;
       } PACKEDSTRUCT RegPairHL;
    unsigned short sdummy4;     // 14
    PACKED union
       {
       unsigned short usRegIX;  // 16
       PACKED struct
          {
          unsigned char ucRegIXL; /* On a big-endian machine, reverse A & B */
          unsigned char ucRegIXH;
          } PACKEDSTRUCT ByteRegs;
       } PACKEDSTRUCT RegPairIX;
    unsigned short sdummy9;     // 18
    PACKED union
       {
       unsigned short usRegIY;  // 20
       PACKED struct
          {
          unsigned char ucRegIYL; /* On a big-endian machine, reverse A & B */
          unsigned char ucRegIYH;
          } PACKEDSTRUCT ByteRegs;
       } PACKEDSTRUCT RegPairIY;
unsigned short sdummy10;        // 22
unsigned short usRegSP;         // 24
unsigned short sdummy11;        // 26
unsigned short usRegPC;         // 28
unsigned short sdummy12;        // 30
unsigned char ucRegI;           // 32
unsigned char ucRegR;           // 33
unsigned char ucRegNMI;         // 34 flag indicating in NMI processing
unsigned char ucRegIM;          // 35
unsigned char ucRegIFF1;        // 36
unsigned char ucRegIFF2;        // 37
unsigned char ucIRQVal;         // 38
unsigned char ucRegHALT;        // 39
/* Stick these at the end so that a context switch does not have to copy them every time */
    unsigned short usRegAF1;    // 40
    unsigned short sdummy5;     // 42
    unsigned short usRegBC1;    // 44
    unsigned short sdummy6;     // 46
    unsigned short usRegDE1;    // 48
    unsigned short sdummy7;     // 50
    unsigned short usRegHL1;    // 52
    unsigned short sdummy8;     // 54
    unsigned long ulTemp[16];   // 56 another 64 bytes for temp vars
} PACKEDSTRUCT REGSZ80;

#ifdef _WIN32
#pragma pack( pop )
#endif

#if 0
typedef struct tagREGSZ80
{
    unsigned short usRegAF;     // 0
    unsigned short sdummy1;     // 2
    unsigned short usRegBC;     // 4
    unsigned short sdummy2;     // 6
    unsigned short usRegDE;     // 8
    unsigned short sdummy3;     // 10
    unsigned short usRegHL;     // 12
    unsigned short sdummy4;     // 14
    unsigned short usRegIX;     // 16
    unsigned short sdummy9;     // 18
    unsigned short usRegIY;     // 20
unsigned short sdummy10;        // 22
unsigned short usRegSP;         // 24
unsigned short sdummy11;        // 26
unsigned short usRegPC;         // 28
unsigned short sdummy12;        // 30
unsigned char ucRegI;           // 32
unsigned char ucRegR;           // 33
unsigned char ucRegNMI;         // 34 flag indicating in NMI processing
unsigned char ucRegIM;          // 35
unsigned char ucRegIFF1;        // 36
unsigned char ucRegIFF2;        // 37
unsigned char ucIRQVal;         // 38
unsigned char ucRegHALT;        // 39
/* Stick these at the end so that a context switch does not have to copy them every time */
    unsigned short usRegAF1;    // 40
    unsigned short sdummy5;     // 43
    unsigned short usRegBC1;    // 44
    unsigned short sdummy6;     // 46
    unsigned short usRegDE1;    // 48
    unsigned short sdummy7;     // 50
    unsigned short usRegHL1;    // 52
    unsigned short sdummy8;     // 54
    unsigned long ulTemp[16];   // 56 another 64 bytes for temp vars
} REGSZ80;
#endif

/* 6809 registers */
typedef struct tagREGS6809
{
    unsigned long  ulOffsets[32]; // use for bank switching and ARM temp vars
    unsigned short usRegX;
    unsigned short usRegY;
    unsigned short usRegU;
    unsigned short usRegS;
    unsigned short usRegPC;
    union
       {
       unsigned short usRegD;
       struct
          {
          unsigned char ucRegB; /* On a big-endian machine, reverse A & B */
          unsigned char ucRegA;
          } ByteRegs;
       } RegPair;
    unsigned char ucRegCC;
    unsigned char ucRegDP;
} REGS6809;

/* 6800/6803 registers */
typedef struct tagREGS6800
{
    unsigned short usRegX;
    unsigned short usRegS;
    unsigned short usRegPC;
    union
       {
       unsigned short usRegD;
       struct
          {
          unsigned char ucRegB; /* On a big-endian machine, reverse A & B */
          unsigned char ucRegA;
          } ByteRegs;
       } RegPair;
    unsigned char  ucRegCC;
} REGS6800;

/* HuC6280 Emulator */
void RESETH6280(unsigned char *pFlags, unsigned char *mem, REGSH6280 *regs);
void EXECH6280(unsigned char *mem, REGSH6280 *regs, EMUHANDLERS2 *emuh, int *iClocks, unsigned char *ucIRQs, unsigned char *pFlags);
#if defined _WIN32 || defined BSD
#define ARESETH6280 RESETH6280
#define AEXECH6280 EXECH6280
#else
void ARESETH6280(unsigned char *pFlags, unsigned char *mem, REGSH6280 *regs);
void AEXECH6280(unsigned char *mem, REGSH6280 *regs, EMUHANDLERS2 *emuh, int *iClocks, unsigned char *ucIRQs, unsigned char *pFlags);
#endif // _WIN32

/* I8039 Emulator */
void EXEC8039(unsigned char *, REGS8039 *, EMUHANDLERS *, int *, unsigned char *);
void RESET8039(char *, REGS8039 *);

/* V30 Emulator */
void RESETV30(REGS8086 *);
void EXECV30(unsigned char *, REGS8086 *, EMUHANDLERS2 *, int *, unsigned char *, int, unsigned long *);

/* 8086 Emulator */
void RESET8086(REGS8086 *);
void EXEC8086(unsigned char *, REGS8086 *, EMUHANDLERS2 *, int *, unsigned char *, int, unsigned long *);

/* M6502 Emulator */
void EXEC6502(unsigned char *, REGS6502 *, EMUHANDLERS *, int *, unsigned char *);
void EXEC6502DECRYPT(unsigned char *, REGS6502 *, EMUHANDLERS *, int *, unsigned char *);
void RESET6502(unsigned char *, REGS6502 *);

/* 6802/6809 Emu */
void EXEC6802(unsigned char *, REGS6800 *, EMUHANDLERS *, signed int *, unsigned char *, int, unsigned long *);
void RESET6802(unsigned char *, REGS6800 *);
void EXEC6803(unsigned char *, REGS6800 *, EMUHANDLERS *, int *, unsigned char *, int, unsigned long *);
void RESET6803(unsigned char *, REGS6800 *);
void EXEC6809(unsigned char *, REGS6809 *, EMUHANDLERS *, int *, unsigned char *, int, unsigned long *);
void RESET6809(unsigned char *, REGS6809 *, unsigned long *);
#if defined _WIN32 || defined BSD
#define ARESET6809 RESET6809
#define AEXEC6809 EXEC6809
#else
void AEXEC6809(unsigned char *, REGS6809 *, EMUHANDLERS *, int *, unsigned char *, int, unsigned long *);
void ARESET6809(unsigned char *, REGS6809 *, unsigned long *);
#endif
/* Z80 Emulator */
void RESETZ80(REGSZ80 *, int);
void EXECZ80(unsigned char *, REGSZ80 *, EMUHANDLERS *, int *, unsigned char *, int, unsigned long *);
void EXECZ80DECRYPT(unsigned char *, REGSZ80 *, EMUHANDLERS *, int *, unsigned char *, int);
#if defined _WIN32 || defined BSD
#define ARESETZ80 RESETZ80
#define AEXECZ80 EXECZ80
#else
void ARESETZ80(REGSZ80 *, int);
void AEXECZ80(unsigned char *, REGSZ80 *, EMUHANDLERS *, int *, unsigned char *, int, unsigned long *);
#endif
#ifdef PORTABLE
   #define AEXECZ80DECRYPT EXECZ80DECRYPT
#else
void AEXECZ80DECRYPT(char *, REGSZ80 *, EMUHANDLERS *, int *, unsigned char *, int);
#endif

/* M68K emulator */
void EXEC68020(unsigned char *, REGS68K *, EMUHANDLERS2 *, int *, unsigned char *, int, unsigned long *);
void RESET68020(unsigned long *ulCPUOffs, REGS68K *regs);
void AEXEC68020(unsigned char *, REGS68K *, EMUHANDLERS2 *, int *, unsigned char *, int, unsigned long *);
void ARESET68020(unsigned long *ulCPUOffs, REGS68K *regs);
void EXEC68K(unsigned char *, REGS68K *, EMUHANDLERS3 *, int *, unsigned char *, int, unsigned long *);
void RESET68K(unsigned long *ulCPUOffs, REGS68K *regs, int iCPU);
#if defined _WIN32 || defined BSD
#define AEXEC68K EXEC68K
#define ARESET68K RESET68K
#define AEXEC68020 EXEC68020
#define ARESET68020 RESET68020
#else
void AEXEC68K(unsigned char *, REGS68K *, EMUHANDLERS3 *, int *, unsigned char *, int, unsigned long *);
void ARESET68K(unsigned long *ulCPUOffs, REGS68K *regs, int iCPU);
#endif

#define Set68KIRQ(flags, level) flags |= (1<<level)
#define Clr68KIRQ(flags, level) flags &= ~(1<<level)


#ifndef MAIN_MODULE
extern HWND hwndDebug, hwndClient;
extern int *iCharX, *iCharY, iRecordState;
extern iMagX1, iMagY1, iMagX2, iMagY2; // joystick / mouse magnitude
extern volatile int iFrame;
extern iOldFrame;
extern unsigned char cTransparent, cTransparent2;
extern int iFrameTime;
extern unsigned int iTimerFreq;
extern int iVolShift1, iVolShift2;
extern BOOL bAutoLoad, bPerformanceCounter;
extern unsigned long lTargTime, lCurrTime;
//extern LARGE_INTEGER iLargeFreq;
extern volatile unsigned long ulKeys, ulKeys2, ulJoy;
extern unsigned long lDirtyRect;
extern int iDirtyHeight, iDirtySections;
extern signed char cRangeTable[];
extern unsigned char cSpriteFlags[];
extern int iFPS; /* Frames per second */
extern int iSampleRate; /* Audio sample rate 0-2 = (1<<i)*11025 */
extern int iSpriteLimitX, iSpriteLimitY;
extern SNDSAMPLE *pSounds; /* Sound samples structure */
extern unsigned long *lCharAddr, *lCharAddr2; /* Array to translate arcade screen address into PC video address */
extern unsigned char *pSoundPROM, *cDirtyChar;
extern int iNumAreas;
extern BOOL bHiLoaded, bThrottle;
extern unsigned long ulCPUOffsets[], ulCPUOffsets2[], ulCPUOffsets3[], ulCPUOffsets4[];
#endif

#endif // #ifndef _EMU_H_
