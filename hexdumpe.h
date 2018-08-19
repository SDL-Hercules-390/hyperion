/* HEXDUMPE.C   (C) Copyright "Fish" (David B. Trout), 2014-2015     */
/*              Format a hex dump into the buffer provided           */
/*              (with minor adjustments to accommodate Hercules)     */
/*                                                                   */
/*   Repository URL:  https://github.com/Fish-Git/hexdump            */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/**********************************************************************

   TITLE            hexdumpaw / hexdumpew

   AUTHOR           "Fish" (David B. Trout)

   COPYRIGHT        (C) 2014 Software Development Laboratories

   VERSION          1.2     (31 Dec 2014)

   DESCRIPTION      Format a hex dump into the buffer provided

   123ABC00            7F454C  46010101 00000000  .ELF............
   123ABC10  44350000 00000000 34002000 06002800  D5......4.....(.
   123ABC20  23002000 060000                      #......

   PARAMETERS

     pfx            string prefixed to each line
     buf            pointer to results buffer ptr
     dat            pointer to data to be dumped
     skp            number of dump bytes to skip
     amt            amount of data to be dumped
     adr            cosmetic dump address of data
     wid            width of dump address in bits
     bpg            bytes per formatted group
     gpl            formatted groups per line

   NOTES

     hexdumpaw and hexdumpew are normally not called directly but
     are instead called via one of the defined hexdumpxnn macros
     where x is either a or e for ascii or ebcdic and nn is the
     width of the cosmetic adr value in bits. Thus the hexdumpa32
     macro will format an ascii dump using 8 hex digit (32-bit)
     wide adr values whereas the hexdumpe64 macro will format an
     ebcdic dump using 64-bit (16 hex digit) wide adr values. The
     parameters passed to the macro are identical other than the
     missing wid parameter which is implied by the macro's name.

     No checking for buffer overflows is performed.  It is the
     responsibility of the caller to ensure that sufficient buffer
     space is available.  If you do not provide a buffer then one
     will be automatically malloc for you which you must then free.

     Neither buf nor dat may be NULL. amt, bpg and gpl must be >= 1.
     skp must be < (bpg * gpl). skp dump bytes are skipped before
     dumping of dat begins. The number of dat bytes to be dumped is
     amt and should not include skp. skp defines where on the first
     dump line the formatting of dat begins. The minimum number of
     bytes required for the results buffer can be calculated using
     the below formulas:

     bpl    = (bpg * gpl)
     lbs    = strlen(pfx) + (wid/4) + 2 + (bpl * 2) + gpl + 1 + bpl + 1
     lines  = (skp + amt + bpl - 1) / bpl
     bytes  = (lines * lbs) + 1

**********************************************************************/

#ifndef _HEXDUMP_H_
#define _HEXDUMP_H_

#include "hercules.h"       // (need hscutl.h HEXDMP_DLL_IMPORT)
#include "codepage.h"       // (need h2g_tab, g2h_tab functions)

/*********************************************************************/
/*                          HEXDUMP                                  */
/*********************************************************************/

typedef
void HEXDUMPE ( const char *pfx, char **buf, const char *dat, size_t skp,
                size_t amt, U64 adr, int wid, size_t bpg, size_t gpl );

HEXDMP_DLL_IMPORT // (hscutl.h)
void hexdumpaw( const char *pfx, char **buf, const char *dat, size_t skp,
                size_t amt, U64 adr, int wid, size_t bpg, size_t gpl );

HEXDMP_DLL_IMPORT // (hscutl.h)
void hexdumpew( const char *pfx, char **buf, const char *dat, size_t skp,
                size_t amt, U64 adr, int wid, size_t bpg, size_t gpl );


#define hexdumpe16( pfx, buf, dat, skp, amt, adr,     bpg, gpl )  \
        hexdumpew(  pfx, buf, dat, skp, amt, adr, 16, bpg, gpl )

#define hexdumpa16( pfx, buf, dat, skp, amt, adr,     bpg, gpl )  \
        hexdumpaw(  pfx, buf, dat, skp, amt, adr, 16, bpg, gpl )

#define hexdumpe24( pfx, buf, dat, skp, amt, adr,     bpg, gpl )  \
        hexdumpew(  pfx, buf, dat, skp, amt, adr, 24, bpg, gpl )

#define hexdumpa24( pfx, buf, dat, skp, amt, adr,     bpg, gpl )  \
        hexdumpaw(  pfx, buf, dat, skp, amt, adr, 24, bpg, gpl )

#define hexdumpe32( pfx, buf, dat, skp, amt, adr,     bpg, gpl )  \
        hexdumpew(  pfx, buf, dat, skp, amt, adr, 32, bpg, gpl )

#define hexdumpa32( pfx, buf, dat, skp, amt, adr,     bpg, gpl )  \
        hexdumpaw(  pfx, buf, dat, skp, amt, adr, 32, bpg, gpl )

#define hexdumpe48( pfx, buf, dat, skp, amt, adr,     bpg, gpl )  \
        hexdumpew(  pfx, buf, dat, skp, amt, adr, 48, bpg, gpl )

#define hexdumpa48( pfx, buf, dat, skp, amt, adr,     bpg, gpl )  \
        hexdumpaw(  pfx, buf, dat, skp, amt, adr, 48, bpg, gpl )

#define hexdumpe56( pfx, buf, dat, skp, amt, adr,     bpg, gpl )  \
        hexdumpew(  pfx, buf, dat, skp, amt, adr, 56, bpg, gpl )

#define hexdumpa56( pfx, buf, dat, skp, amt, adr,     bpg, gpl )  \
        hexdumpaw(  pfx, buf, dat, skp, amt, adr, 56, bpg, gpl )

#define hexdumpe64( pfx, buf, dat, skp, amt, adr,     bpg, gpl )  \
        hexdumpew(  pfx, buf, dat, skp, amt, adr, 64, bpg, gpl )

#define hexdumpa64( pfx, buf, dat, skp, amt, adr,     bpg, gpl )  \
        hexdumpaw(  pfx, buf, dat, skp, amt, adr, 64, bpg, gpl )

#define hexdumpa    hexdumpa32      /* Default to 32-bit */
#define hexdumpe    hexdumpe32      /* Default to 32-bit */
#define hexdump     hexdumpa        /* Default to ASCII  */

/*********************************************************************/
/*               EBCDIC/ASCII Translation Tables                     */
/*********************************************************************/
#if 0 // (Hercules uses its own translation tables)
static const char
_a2e_tab[] = { /* ASCII code page 1252 to EBCDIC code page 1140 */
/*      x0   x1   x2   x3   x4   x5   x6   x7   x8   x9   xA   xB   xC   xD   xE   xF*/
/*0x*/0x00,0x01,0x02,0x03,0x37,0x2D,0x2E,0x2F,0x16,0x05,0x25,0x0B,0x0C,0x0D,0x0E,0x0F,
/*1x*/0x10,0x11,0x12,0x13,0x3C,0x3D,0x32,0x26,0x18,0x19,0x3F,0x27,0x1C,0x1D,0x1E,0x1F,
/*2x*/0x40,0x5A,0x7F,0x7B,0x5B,0x6C,0x50,0x7D,0x4D,0x5D,0x5C,0x4E,0x6B,0x60,0x4B,0x61,
/*3x*/0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0x7A,0x5E,0x4C,0x7E,0x6E,0x6F,
/*4x*/0x7C,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,
/*5x*/0xD7,0xD8,0xD9,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xBA,0xE0,0xBB,0xB0,0x6D,
/*6x*/0x79,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x91,0x92,0x93,0x94,0x95,0x96,
/*7x*/0x97,0x98,0x99,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xC0,0x4F,0xD0,0xA1,0x07,
/*8x*/0x9F,0x21,0x22,0x23,0x24,0x15,0x06,0x17,0x28,0x29,0x2A,0x2B,0x2C,0x09,0x0A,0x1B,
/*9x*/0x30,0x31,0x1A,0x33,0x34,0x35,0x36,0x08,0x38,0x39,0x3A,0x3B,0x04,0x14,0x3E,0xFF,
/*Ax*/0x41,0xAA,0x4A,0xB1,0x20,0xB2,0x6A,0xB5,0xBD,0xB4,0x9A,0x8A,0x5F,0xCA,0xAF,0xBC,
/*Bx*/0x90,0x8F,0xEA,0xFA,0xBE,0xA0,0xB6,0xB3,0x9D,0xDA,0x9B,0x8B,0xB7,0xB8,0xB9,0xAB,
/*Cx*/0x64,0x65,0x62,0x66,0x63,0x67,0x9E,0x68,0x74,0x71,0x72,0x73,0x78,0x75,0x76,0x77,
/*Dx*/0xAC,0x69,0xED,0xEE,0xEB,0xEF,0xEC,0xBF,0x80,0xFD,0xFE,0xFB,0xFC,0xAD,0xAE,0x59,
/*Ex*/0x44,0x45,0x42,0x46,0x43,0x47,0x9C,0x48,0x54,0x51,0x52,0x53,0x58,0x55,0x56,0x57,
/*Fx*/0x8C,0x49,0xCD,0xCE,0xCB,0xCF,0xCC,0xE1,0x70,0xDD,0xDE,0xDB,0xDC,0x8D,0x8E,0xDF,
};/*    x0   x1   x2   x3   x4   x5   x6   x7   x8   x9   xA   xB   xC   xD   xE   xF*/
static const char
_e2a_tab[] = { /* EBCDIC code page 1140 to ASCII code page 1252 */
/*      x0   x1   x2   x3   x4   x5   x6   x7   x8   x9   xA   xB   xC   xD   xE   xF*/
/*0x*/0x00,0x01,0x02,0x03,0x9C,0x09,0x86,0x7F,0x97,0x8D,0x8E,0x0B,0x0C,0x0D,0x0E,0x0F,
/*1x*/0x10,0x11,0x12,0x13,0x9D,0x85,0x08,0x87,0x18,0x19,0x92,0x8F,0x1C,0x1D,0x1E,0x1F,
/*2x*/0xA4,0x81,0x82,0x83,0x84,0x0A,0x17,0x1B,0x88,0x89,0x8A,0x8B,0x8C,0x05,0x06,0x07,
/*3x*/0x90,0x91,0x16,0x93,0x94,0x95,0x96,0x04,0x98,0x99,0x9A,0x9B,0x14,0x15,0x9E,0x1A,
/*4x*/0x20,0xA0,0xE2,0xE4,0xE0,0xE1,0xE3,0xE5,0xE7,0xF1,0xA2,0x2E,0x3C,0x28,0x2B,0x7C,
/*5x*/0x26,0xE9,0xEA,0xEB,0xE8,0xED,0xEE,0xEF,0xEC,0xDF,0x21,0x24,0x2A,0x29,0x3B,0xAC,
/*6x*/0x2D,0x2F,0xC2,0xC4,0xC0,0xC1,0xC3,0xC5,0xC7,0xD1,0xA6,0x2C,0x25,0x5F,0x3E,0x3F,
/*7x*/0xF8,0xC9,0xCA,0xCB,0xC8,0xCD,0xCE,0xCF,0xCC,0x60,0x3A,0x23,0x40,0x27,0x3D,0x22,
/*8x*/0xD8,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0xAB,0xBB,0xF0,0xFD,0xFE,0xB1,
/*9x*/0xB0,0x6A,0x6B,0x6C,0x6D,0x6E,0x6F,0x70,0x71,0x72,0xAA,0xBA,0xE6,0xB8,0xC6,0x80,
/*Ax*/0xB5,0x7E,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0xA1,0xBF,0xD0,0xDD,0xDE,0xAE,
/*Bx*/0x5E,0xA3,0xA5,0xB7,0xA9,0xA7,0xB6,0xBC,0xBD,0xBE,0x5B,0x5D,0xAF,0xA8,0xB4,0xD7,
/*Cx*/0x7B,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0xAD,0xF4,0xF6,0xF2,0xF3,0xF5,
/*Dx*/0x7D,0x4A,0x4B,0x4C,0x4D,0x4E,0x4F,0x50,0x51,0x52,0xB9,0xFB,0xFC,0xF9,0xFA,0xFF,
/*Ex*/0x5C,0xF7,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,0xB2,0xD4,0xD6,0xD2,0xD3,0xD5,
/*Fx*/0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0xB3,0xDB,0xDC,0xD9,0xDA,0x9F,
};/*    x0   x1   x2   x3   x4   x5   x6   x7   x8   x9   xA   xB   xC   xD   xE   xF*/
#endif // (Hercules uses its own translation tables)

/*********************************************************************/
/*              EBCDIC/ASCII Translation Functions                   */
/*********************************************************************/
/*
**              Get pointer to translation table
*/
#if 0 // (Hercules uses its own translation tables)
static INLINE  const char*  a2etab()  { return _a2e_tab; }
static INLINE  const char*  e2atab()  { return _e2a_tab; }
#else // (Hercules uses its own translation tables)
static INLINE  const char*  a2etab()  { return (const char*) h2g_tab(); }
static INLINE  const char*  e2atab()  { return (const char*) g2h_tab(); }
#endif // (Hercules uses its own translation tables)
/*
**                  Translate single byte
*/
static INLINE  char  a2e( char b )    { return a2etab()[ (unsigned char) b ]; }
static INLINE  char  e2a( char b )    { return e2atab()[ (unsigned char) b ]; }
/*
**              Translate an entire array of bytes
*/
extern U8 e2aora2e              /* Return true/false success/failure */
(
          char    *out,         /* Resulting translated array        */
    const char    *in,          /* Array to be translated            */
    const size_t   len,         /* Length of each array in bytes     */
    const char    *x2xtab       /* Pointer to translation table      */
);

#endif // _HEXDUMP_H_
