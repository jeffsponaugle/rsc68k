/*
 * Copyright (c) 2003, Artem B. Bityuckiy, SoftMine Corporation.
 * Rights transferred to Franklin Electronic Publishers.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include <stdio.h>
#include <stdlib.h>
#include <iconv.h>
#include <errno.h>
#include <newlib.h>
#include "check.h"

#ifdef _ICONV_ENABLED

#if defined(_ICONV_FROM_ENCODING_UTF_8) || \
    defined(_ICONV_FROM_ENCODING_ISO_8859_5) || \
    defined(_ICONV_FROM_ENCODING_KOI8_R)

#ifdef _ICONV_FROM_ENCODING_ISO_8859_5
char iso_8859_5[] =
{
    0xbe,0xdf,0xd5,0xe0,0xd0,0xe2,0xde,0xe0,0xeb,0x20,
    0xd2,0x20,0xde,0xd4,0xdd,0xde,0xd9,0x20,0xe1,0xe2,
    0xe0,0xde,0xda,0xd5,0x20,0xd8,0xdc,0xd5,0xee,0xe2,
    0x20,0xde,0xd4,0xd8,0xdd,0xd0,0xda,0xde,0xd2,0xeb,
    0xd9,0x20,0xdf,0xe0,0xd8,0xde,0xe0,0xd8,0xe2,0xd5,
    0xe2,0x0a,0xc1,0xe2,0xe0,0xde,0xda,0xd8,0x20,0xe3,
    0xdf,0xde,0xe0,0xef,0xd4,0xde,0xe7,0xd5,0xdd,0xeb,
    0x20,0xdf,0xde,0x20,0xe3,0xd1,0xeb,0xd2,0xd0,0xdd,
    0xd8,0xee,0x20,0xdf,0xe0,0xd8,0xde,0xe0,0xd8,0xe2,
    0xd5,0xe2,0xde,0xd2,0x2e,0x0a,0x0a,0x2d,0x2d,0x2d,
    0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,
    0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,
    0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,
    0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,
    0x2d,0x2d,0x2d,0x2d,0x20,0xb2,0xeb,0xdf,0xde,0xdb,
    0xdd,0xd5,0xdd,0xd8,0xd5,0x20,0x2d,0x2d,0x2d,0x2d,
    0x2d,0x2d,0x2d,0x2d,0x2d,0x0a,0x28,0x29,0x20,0x20,
    0x5b,0x5d,0x20,0x20,0x2d,0x3e,0x20,0x20,0x2e,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x2d,0x2d,0x2d,0x3e,0x0a,0x21,
    0x20,0x20,0x20,0x7e,0x20,0x20,0x20,0x2b,0x2b,0x20,
    0x20,0x2d,0x2d,0x20,0x20,0x2b,0x20,0x20,0x2d,0x20,
    0x20,0x2a,0x20,0x20,0x26,0x20,0x20,0x28,0x74,0x79,
    0x70,0x65,0x29,0x20,0x20,0x73,0x69,0x7a,0x65,0x6f,
    0x66,0x20,0x20,0x20,0x20,0x20,0x20,0x2d,0x2d,0x2d,
    0x3e,0x0a,0x2a,0x20,0x20,0x2f,0x20,0x20,0x25,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x2d,0x2d,0x2d,0x3e,0x0a,0x2b,0x20,0x20,0x2d,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x2d,0x2d,0x2d,0x3e,0x0a,0x3c,0x3c,
    0x20,0x20,0x3e,0x3e,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x2d,0x2d,0x2d,0x3e,
    0x0a,0x3c,0x20,0x20,0x3c,0x3d,0x20,0x20,0x3e,0x20,
    0x20,0x3e,0x3d,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x2d,
    0x2d,0x2d,0x3e,0x0a,0x3d,0x3d,0x20,0x20,0x21,0x3d,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x2d,0x2d,0x2d,0x3e,0x0a,0x26,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x2d,0x2d,0x2d,0x3e,0x0a,
    0x5e,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x2d,0x2d,
    0x2d,0x3e,0x0a,0x7c,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x2d,0x2d,0x2d,0x3e,0x0a,0x26,0x26,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x2d,0x2d,0x2d,0x3e,0x0a,0x7c,
    0x7c,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x2d,0x2d,0x2d,
    0x3e,0x0a,0x3f,0x3a,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x3c,0x2d,0x2d,0x2d,0x20,0x20,0x20,0x0a,0x3d,0x20,
    0x20,0x2b,0x3d,0x20,0x20,0x2d,0x3d,0x20,0x20,0x2f,
    0x3d,0x20,0x20,0x25,0x3d,0x20,0x20,0x26,0x3d,0x20,
    0x20,0x5e,0x3d,0x20,0x20,0x7c,0x3d,0x20,0x20,0x3c,
    0x3c,0x3d,0x20,0x20,0x3e,0x3e,0x3d,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x3c,0x2d,0x2d,0x2d,
    0x0a,0x2c,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x2d,
    0x2d,0x2d,0x3e,0x0a
};
#endif /* #ifdef _ICONV_FROM_ENCODING_ISO_8859_5 */

#ifdef _ICONV_FROM_ENCODING_KOI8_R
char koi8_r[] = 
{
    0xef,0xd0,0xc5,0xd2,0xc1,0xd4,0xcf,0xd2,0xd9,0x20,
    0xd7,0x20,0xcf,0xc4,0xce,0xcf,0xca,0x20,0xd3,0xd4,
    0xd2,0xcf,0xcb,0xc5,0x20,0xc9,0xcd,0xc5,0xc0,0xd4,
    0x20,0xcf,0xc4,0xc9,0xce,0xc1,0xcb,0xcf,0xd7,0xd9,
    0xca,0x20,0xd0,0xd2,0xc9,0xcf,0xd2,0xc9,0xd4,0xc5,
    0xd4,0x0a,0xf3,0xd4,0xd2,0xcf,0xcb,0xc9,0x20,0xd5,
    0xd0,0xcf,0xd2,0xd1,0xc4,0xcf,0xde,0xc5,0xce,0xd9,
    0x20,0xd0,0xcf,0x20,0xd5,0xc2,0xd9,0xd7,0xc1,0xce,
    0xc9,0xc0,0x20,0xd0,0xd2,0xc9,0xcf,0xd2,0xc9,0xd4,
    0xc5,0xd4,0xcf,0xd7,0x2e,0x0a,0x0a,0x2d,0x2d,0x2d,
    0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,
    0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,
    0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,
    0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,
    0x2d,0x2d,0x2d,0x2d,0x20,0xf7,0xd9,0xd0,0xcf,0xcc,
    0xce,0xc5,0xce,0xc9,0xc5,0x20,0x2d,0x2d,0x2d,0x2d,
    0x2d,0x2d,0x2d,0x2d,0x2d,0x0a,0x28,0x29,0x20,0x20,
    0x5b,0x5d,0x20,0x20,0x2d,0x3e,0x20,0x20,0x2e,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x2d,0x2d,0x2d,0x3e,0x0a,0x21,
    0x20,0x20,0x20,0x7e,0x20,0x20,0x20,0x2b,0x2b,0x20,
    0x20,0x2d,0x2d,0x20,0x20,0x2b,0x20,0x20,0x2d,0x20,
    0x20,0x2a,0x20,0x20,0x26,0x20,0x20,0x28,0x74,0x79,
    0x70,0x65,0x29,0x20,0x20,0x73,0x69,0x7a,0x65,0x6f,
    0x66,0x20,0x20,0x20,0x20,0x20,0x20,0x2d,0x2d,0x2d,
    0x3e,0x0a,0x2a,0x20,0x20,0x2f,0x20,0x20,0x25,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x2d,0x2d,0x2d,0x3e,0x0a,0x2b,0x20,0x20,0x2d,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x2d,0x2d,0x2d,0x3e,0x0a,0x3c,0x3c,
    0x20,0x20,0x3e,0x3e,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x2d,0x2d,0x2d,0x3e,
    0x0a,0x3c,0x20,0x20,0x3c,0x3d,0x20,0x20,0x3e,0x20,
    0x20,0x3e,0x3d,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x2d,
    0x2d,0x2d,0x3e,0x0a,0x3d,0x3d,0x20,0x20,0x21,0x3d,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x2d,0x2d,0x2d,0x3e,0x0a,0x26,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x2d,0x2d,0x2d,0x3e,0x0a,
    0x5e,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x2d,0x2d,
    0x2d,0x3e,0x0a,0x7c,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x2d,0x2d,0x2d,0x3e,0x0a,0x26,0x26,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x2d,0x2d,0x2d,0x3e,0x0a,0x7c,
    0x7c,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x2d,0x2d,0x2d,
    0x3e,0x0a,0x3f,0x3a,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x3c,0x2d,0x2d,0x2d,0x20,0x20,0x20,0x0a,0x3d,0x20,
    0x20,0x2b,0x3d,0x20,0x20,0x2d,0x3d,0x20,0x20,0x2f,
    0x3d,0x20,0x20,0x25,0x3d,0x20,0x20,0x26,0x3d,0x20,
    0x20,0x5e,0x3d,0x20,0x20,0x7c,0x3d,0x20,0x20,0x3c,
    0x3c,0x3d,0x20,0x20,0x3e,0x3e,0x3d,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x3c,0x2d,0x2d,0x2d,
    0x0a,0x2c,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x2d,
    0x2d,0x2d,0x3e,0x0a
};
#endif /* #ifdef _ICONV_FROM_ENCODING_KOI8_R */

#ifdef _ICONV_FROM_ENCODING_UTF_8
char utf8[] =
{
    0xd0,0x9e,0xd0,0xbf,0xd0,0xb5,0xd1,0x80,0xd0,0xb0,
    0xd1,0x82,0xd0,0xbe,0xd1,0x80,0xd1,0x8b,0x20,0xd0,
    0xb2,0x20,0xd0,0xbe,0xd0,0xb4,0xd0,0xbd,0xd0,0xbe,
    0xd0,0xb9,0x20,0xd1,0x81,0xd1,0x82,0xd1,0x80,0xd0,
    0xbe,0xd0,0xba,0xd0,0xb5,0x20,0xd0,0xb8,0xd0,0xbc,
    0xd0,0xb5,0xd1,0x8e,0xd1,0x82,0x20,0xd0,0xbe,0xd0,
    0xb4,0xd0,0xb8,0xd0,0xbd,0xd0,0xb0,0xd0,0xba,0xd0,
    0xbe,0xd0,0xb2,0xd1,0x8b,0xd0,0xb9,0x20,0xd0,0xbf,
    0xd1,0x80,0xd0,0xb8,0xd0,0xbe,0xd1,0x80,0xd0,0xb8,
    0xd1,0x82,0xd0,0xb5,0xd1,0x82,0x0a,0xd0,0xa1,0xd1,
    0x82,0xd1,0x80,0xd0,0xbe,0xd0,0xba,0xd0,0xb8,0x20,
    0xd1,0x83,0xd0,0xbf,0xd0,0xbe,0xd1,0x80,0xd1,0x8f,
    0xd0,0xb4,0xd0,0xbe,0xd1,0x87,0xd0,0xb5,0xd0,0xbd,
    0xd1,0x8b,0x20,0xd0,0xbf,0xd0,0xbe,0x20,0xd1,0x83,
    0xd0,0xb1,0xd1,0x8b,0xd0,0xb2,0xd0,0xb0,0xd0,0xbd,
    0xd0,0xb8,0xd1,0x8e,0x20,0xd0,0xbf,0xd1,0x80,0xd0,
    0xb8,0xd0,0xbe,0xd1,0x80,0xd0,0xb8,0xd1,0x82,0xd0,
    0xb5,0xd1,0x82,0xd0,0xbe,0xd0,0xb2,0x2e,0x0a,0x0a,
    0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,
    0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,
    0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,
    0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,
    0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x20,0xd0,0x92,
    0xd1,0x8b,0xd0,0xbf,0xd0,0xbe,0xd0,0xbb,0xd0,0xbd,
    0xd0,0xb5,0xd0,0xbd,0xd0,0xb8,0xd0,0xb5,0x20,0x2d,
    0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x2d,0x0a,0x28,
    0x29,0x20,0x20,0x5b,0x5d,0x20,0x20,0x2d,0x3e,0x20,
    0x20,0x2e,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x2d,0x2d,0x2d,
    0x3e,0x0a,0x21,0x20,0x20,0x20,0x7e,0x20,0x20,0x20,
    0x2b,0x2b,0x20,0x20,0x2d,0x2d,0x20,0x20,0x2b,0x20,
    0x20,0x2d,0x20,0x20,0x2a,0x20,0x20,0x26,0x20,0x20,
    0x28,0x74,0x79,0x70,0x65,0x29,0x20,0x20,0x73,0x69,
    0x7a,0x65,0x6f,0x66,0x20,0x20,0x20,0x20,0x20,0x20,
    0x2d,0x2d,0x2d,0x3e,0x0a,0x2a,0x20,0x20,0x2f,0x20,
    0x20,0x25,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x2d,0x2d,0x2d,0x3e,0x0a,0x2b,0x20,
    0x20,0x2d,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x2d,0x2d,0x2d,0x3e,
    0x0a,0x3c,0x3c,0x20,0x20,0x3e,0x3e,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x2d,
    0x2d,0x2d,0x3e,0x0a,0x3c,0x20,0x20,0x3c,0x3d,0x20,
    0x20,0x3e,0x20,0x20,0x3e,0x3d,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x2d,0x2d,0x2d,0x3e,0x0a,0x3d,0x3d,0x20,
    0x20,0x21,0x3d,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x2d,0x2d,0x2d,0x3e,0x0a,
    0x26,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x2d,0x2d,
    0x2d,0x3e,0x0a,0x5e,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x2d,0x2d,0x2d,0x3e,0x0a,0x7c,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x2d,0x2d,0x2d,0x3e,0x0a,0x26,
    0x26,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x2d,0x2d,0x2d,
    0x3e,0x0a,0x7c,0x7c,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x2d,0x2d,0x2d,0x3e,0x0a,0x3f,0x3a,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x3c,0x2d,0x2d,0x2d,0x20,0x20,0x20,
    0x0a,0x3d,0x20,0x20,0x2b,0x3d,0x20,0x20,0x2d,0x3d,
    0x20,0x20,0x2f,0x3d,0x20,0x20,0x25,0x3d,0x20,0x20,
    0x26,0x3d,0x20,0x20,0x5e,0x3d,0x20,0x20,0x7c,0x3d,
    0x20,0x20,0x3c,0x3c,0x3d,0x20,0x20,0x3e,0x3e,0x3d,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x3c,
    0x2d,0x2d,0x2d,0x0a,0x2c,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
    0x20,0x20,0x2d,0x2d,0x2d,0x3e,0x0a
};
#endif

struct iconv_data
{
    int len;
    char *name;
    char *data;
};

#define CONVERSIONS 3

struct iconv_data data[] = 
{
#ifdef _ICONV_FROM_ENCODING_ISO_8859_5
    {sizeof(iso_8859_5), "ISO-8859-5", (char *)iso_8859_5},
#endif
#ifdef _ICONV_FROM_ENCODING_KOI8_R
    {sizeof(koi8_r), "KOI8-R", (char *)koi8_r},
#endif
#ifdef _ICONV_FROM_ENCODING_UTF_8
    {sizeof(utf8), "UTF-8", (char *)utf8},
#endif
    {0, NULL, NULL}
};

#define OUTBUF_LEN 1500
char ob[OUTBUF_LEN];

iconv_t descs[CONVERSIONS*CONVERSIONS];

#define ERROR 0

int main(int argc, char **argv)
{
    int i, j, k, d = 0;
    size_t n;
    char *outbuf, *inbuf;
    int conversions = sizeof(data)/sizeof(struct iconv_data) - 1;

    puts("RU iconv test");
    
    for (i = 0; i < conversions; i++)
    {
        for (j = 0; j < conversions; j++)
	{
	    descs[d] = iconv_open(data[j].name, data[i].name);
	    if (descs[d++] == (iconv_t)-1)
	    {
	        printf("iconv_open(%s, %s)\n", data[i].name, data[j].name);
	        perror("");
                CHECK(ERROR);
	    }
	}
    }
    
    d = 0;
    for (i = 0; i < conversions; i++)
    {
        for (j = 0; j < conversions; j++)
	{
	    size_t inbytes = data[i].len;
	    size_t outbytes = OUTBUF_LEN;
	    inbuf = data[i].data;
	    outbuf = (char *)ob;

	    if (iconv(descs[d], NULL, NULL, (char **)&outbuf, &outbytes)                                                                       == (size_t)-1)
            {
                perror("Can't reset shift state");
                CHECK(ERROR);
            }
	    
            n = iconv(descs[d++], (const char **)&(inbuf), &inbytes, 
	                          (char **)&outbuf, &outbytes);
            if (n == (size_t)-1)
            {
	        printf("Conversion from %s to %s FAILED - iconv() "
                       "returned -1\n", data[i].name, data[j].name);
		perror("");
                CHECK(ERROR);
            }
	    
	    if (data[j].len != OUTBUF_LEN - outbytes)
	    {
                printf("Conversion from %s to %s FAILED",
                       data[i].name, data[j].name);
	        printf(" - bad output buffer length (%d instead of %d)\n",
		       OUTBUF_LEN - outbytes, data[j].len);
                CHECK(ERROR);
	    }
	    
	    for (k = 0; k < data[j].len; k++)
	    {
	        if (ob[k] != data[j].data[k])
		{
                    printf("Conversion from %s to %s FAILED",
                           data[i].name, data[j].name);
   	            printf("Error: byte %d is wrong\n", k);
		    printf("outbuf value: %#x, inbuf value %#x, "
		           "right value: %#x\n",
          	           (int)ob[k], (int)(data[i].data[k]), 
		           (int)(data[j].data[k]));
                    CHECK(ERROR);
		}
	    }

	    printf("iconv from %s to %s was successfully done\n",
                   data[i].name, data[j].name); 
            
	}
    }
    
    d = 0;
    for (i = 0; i < conversions; i++)
        for (j = 0; j < conversions; j++)
            CHECK(iconv_close(descs[d++]) != -1);

    exit(0);
}

#else /* #if defined(_ICONV_FROM_ENCODING_UTF_8) || ... */
int main(int argc, char **argv)
{
    puts("None of ISO-8859-5, KOI8-R and UTF-8 converters linked, SKIP test");
    exit(0);
}
#endif /* #if defined(_ICONV_FROM_ENCODING_UTF_8) || ... */

#else /* #ifdef _ICONV_ENABLED */
int main(int argc, char **argv)
{
    puts("iconv library is disabled, SKIP test");
    exit(0);
}
#endif /* #ifdef _ICONV_ENABLED */

