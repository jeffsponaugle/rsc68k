/******************************************************************************
**
**  COPYRIGHT (C) 2000, 2001 Intel Corporation.
**
**  This software as well as the software described in it is furnished under 
**  license and may only be used or copied in accordance with the terms of the 
**  license. The information in this file is furnished for informational use 
**  only, is subject to change without notice, and should not be construed as 
**  a commitment by Intel Corporation. Intel Corporation assumes no 
**  responsibility or liability for any errors or inaccuracies that may appear 
**  in this document or any software that may be provided in association with 
**  this document. 
**  Except as permitted by such license, no part of this document may be 
**  reproduced, stored in a retrieval system, or transmitted in any form or by 
**  any means without the express written consent of Intel Corporation. 
**
**  FILENAME:       errors.h
**
**  PURPOSE:        This file contains the Power-On Self Test definitions for
**                  errors for Lubbock.
**
**  LAST MODIFIED:  $Modtime: 9/17/01 3:16p $
******************************************************************************/

#ifndef _errors_h_
#define _errors_h_

#define ERR_LOC_BIT_SHIFT   20           // the bit shift for the location code
#define ERR_SUB_BIT_SHIFT   12           // the sub-location bit shift

#define ERR_NONE            0x0          // Alias for standard C no-problem-found

#define ERR_L_NONE          0x000u
#define ERR_L_XSIC          0x001u   // Main processor Interrupt Controller SW
#define ERR_L_XSDMA         0x002u   // Main processor DMA Controller module
#define ERR_L_FLASH         0x003u   // Flash memory device
#define ERR_L_XSGPIO        0x004u   // Main processor GPIO module
#define ERR_L_LAN91C111      0x005u   // LAN91C111 Ethernet Controller
#define ERR_L_XSFFUART		0x006u   // FFUart device
#define ERR_L_XSBTUART		0x007u   // BTUart device
#define ERR_L_XSSTUART		0x008u   // STUart device
#define ERR_L_XSRTC         0x009u   // Main Processor Real Time Clock
#define ERR_L_XSOST         0x00Au   // Main processor Operating System Timer
#define ERR_L_XSAC97CTRL    0x00Bu   // Main processor AC '97 Controller Unit
#define ERR_L_XSCLKMGR      0x00Cu   // Main processor clock manager
#define ERR_L_AC97          0x00Du   // AC'97 Audio and Modem Codec driver
#define ERR_L_PERIPH_BDCTRL 0x00Eu   // Peripheral board's Control module
#define ERR_L_SK            0x00Fu   // Companion chip's overall SW module
#define ERR_L_SKIC          0x010u   // Companion chip's Interrupt Controller SW
#define ERR_L_CPU           0x011u   // Genernal CPU failures
#define ERR_L_MEMORY        0x012u   // Genernal Memory failures
#define ERR_L_SRAM          0x013u   // SRAM devic
#define ERR_L_SDRAM         0x014u   // SDRAM devic
#define ERR_L_XSUDC         0x015u   // USB device controller
#define ERR_L_USB           0x016u   // USB Host
#define ERR_L_XSSSP         0x017u   // SSP device
#define ERR_L_XSICP         0x018u   // ICP device
#define ERR_L_LCD           0x019u   // LCD device
#define ERR_L_TS            0x01Au   // Touchscreen device
#define ERR_L_PS2           0x01Bu   // PS2 ports device
#define ERR_L_MMC           0x01Cu   // MMC device
#define ERR_L_SKPCMCIA      0x01Du   // SkPcmcia.c (SA-1111 PCMCIA)
#define ERR_L_XSPCMCIA      0x01Eu   // XsPcmcia.c (main processor PCMCIA)
#define ERR_L_SKDMA         0x01Fu   // SkDma.c (SA-1111 DMA)
#define ERR_L_SKGPIO        0x020u   // skgpio.c (SA-1111 GPIO module)
#define ERR_L_PWM           0x021u   // Pulse Width Modulation
#define ERR_L_I2C           0x022u   // I2C Serial EEPROM
#define ERR_L_NET           0x023u   // USB2.0 Controller

#define ERR_L_RSVD_FAIL     0x464u   // reserved for FAIL code
#define ERR_L_RSVD_PASS     0x504u   // reserved for PASS code

#define ERR_T_ILLPARAM          0x001   // Illegal parameter
#define ERR_T_TIMEOUT           0x002   // Timeout 
#define ERR_T_NODEVICE          0x003   // A device is not present or cannot be initialized
#define ERR_T_NOBITSET          0x004   // some bit in a register cannot be set or reset
#define ERR_T_INVALIDACC        0x005   // invalid access. Attempt to read/write invalid memory.
#define ERR_T_UNKNOWN           0x006   // unknown error - a generic catch-all 
#define ERR_T_BADRANGE          0x007   // bad range - some number or computation is out of range
#define ERR_T_NORECEIVE         0x008   // Some receiver cannot receive data
#define ERR_T_NOTRANSMIT        0x009   // some transmitter cannot transmit data
#define ERR_T_ILLALIGN          0x00A   // Illegal alignment
#define ERR_T_BUSERRINT         0x00B   // internal bus error interrupt 
#define ERR_T_NODESC            0x00C   // DMA could not get a valid descriptor
#define ERR_T_UNEXPECTED        0x00D   // Unexpected result returned from device
#define ERR_T_NO_HANDLER        0x00E   // An expected interrupt handler was not detected
#define ERR_T_ALREADY_IN_USE    0x00F   // A requested or expected resource was already in use.
#define ERR_T_NOT_AVAIL         0x010   // A requested or expected resource was not available.
#define ERR_T_REG_HANDLER       0x011   // There is a registered interrupt handler.
#define ERR_T_WRONG_STATE       0x012   // The target (SW or HW) was in the wrong state.
#define ERR_T_NO_INT_REASON     0x013   // Int hndlr detected no reason for its invocation.
                                        // Internal software error in reporting module 
#define ERR_T_SW_INTERNAL       0x014   //    or a subroutine. Details in error history.
#define ERR_T_CLIPPED           0x015   // A value or signal was clipped (forcibly truncated)
#define ERR_T_NOT_IMPLEMENTED   0x016   // A requested service is currently not implemented.
#define ERR_T_HW_NOT_SUPPORTED  0x017   // An unsupported hardware device was detected
#define ERR_T_XSAC97CTRL_FIFO   0x018   // AC97 controller FIFO error.  Either underflow or overflow.
#define ERR_T_XSDMA_UNEXPECTED  0x019   // Unexpected status from the DMA controller.
#define ERR_T_WRONG_VALUE       0x01A   // The wrong value was returned
#define ERR_T_BUSY              0x01B   // Device busy
#define ERR_T_CRC               0x01C   // CRC error.
#define ERR_T_DATA_UNDERRUN     0x01D   // Data Underrun error.
#define ERR_T_DATA_OVERRUN      0x01E   // Data Overrun error.
#define ERR_T_NO_MEM_AVAIL      0x01F   // No memory available from memory allocator (mallocx)
#define ERR_T_ILLPARM_PTOV      0x020   // Bad physical address detected by PhysicalToVirtual()
#define ERR_T_ILLPARM_VTOP      0x021   // Bad physical address detected by VirtualToPhysical()

// Memory error codes.
#define ERR_MEMORY              0x00     // General memory error.
#define ERR_SDRAM_WO            0x01     // SDRAM walking ones verify error
#define ERR_SDRAM_WZ            0x02     // SDRAM walking zeros verify error
#define ERR_SDRAM_OS            0x03     // SDRAM ones sum verify error
#define ERR_SDRAM_COS           0x04     // SDRAM complement ones sum verify error

// Register test error codes.
#define ERR_REGISTER_CHANGE     0x05     // Register changed unexpectedly

// UART test error codes.
#define ERR_UART_LOOPBACK       0x01     // UART loopback error

// AC96 test error codes.
#define ERR_AC97                0x00     // General AC97 error
#define ERR_AC97_WRITE          0x01     // AC97 write error
#define ERR_AC97_READ           0x02     // AC97 read error

#define LOGERROR(_logerr,_where,_sub_where,_type,_param1,_param2,_param3)

#endif // _errors_h_