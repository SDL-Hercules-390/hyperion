/* HEXDUMPE.C   (C) Copyright "Fish" (David B. Trout), 2014-2015     */
/*              Format a hex dump into the buffer provided           */
/*              (with minor adjustments to accommodate Hercules)     */
/*                                                                   */
/*   Repository URL:  https://github.com/Fish-Git/hexdump            */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#include "hstdinc.h"

#define _HEXDUMPE_C_
#define _HUTIL_DLL_

#include "hercules.h"

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

//#include "stdafx.h"       // (not needed by Hercules)
#include "hexdumpe.h"

/*********************************************************************/
/*                  EBCDIC/ASCII Translation                         */
/*********************************************************************/
static const char
_x2x_tab[16*16] = { /* dummy table to 'translate' to very same value */
0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,
0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,
0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F,
0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4A,0x4B,0x4C,0x4D,0x4E,0x4F,
0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,0x5B,0x5C,0x5D,0x5E,0x5F,
0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6A,0x6B,0x6C,0x6D,0x6E,0x6F,
0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0x7B,0x7C,0x7D,0x7E,0x7F,
0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F,
0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0x9B,0x9C,0x9D,0x9E,0x9F,
0xA0,0xA1,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF,
0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF,
0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,
0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF,
0xE0,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xEB,0xEC,0xED,0xEE,0xEF,
0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF,
};
U8 e2aora2e                     /* Return true/false success/failure */
(
          char    *out,         /* Resulting translated array        */
    const char    *in,          /* Array to be translated            */
    const size_t   len,         /* Length of each array in bytes     */
    const char    *x2xtab       /* Pointer to translation table      */
)
{
    size_t i;
    if (!out || !in || !len || !x2xtab)
        return 0;
    for (i=0; i < len; i++)
        out[i] = x2xtab[(unsigned char) in[i]];
    return 1;
}

/*********************************************************************/
/*                          HEXDUMP                                  */
/*********************************************************************/
static
void _hexlinex( char *buf, const char *dat, size_t skp, size_t amt,
                size_t bpg, size_t gpl, const char* x2x );
static
void _hexdumpxn( const char *pfx, char **buf, const char *dat, size_t skp,
                 size_t amt, U64 adr, int hxd, size_t bpg, size_t gpl,
                 const char* x2x )
{
    char *p;
    size_t n = strlen(pfx);
    size_t bpl = (bpg * gpl);
    size_t lbs = n + hxd + 2 + (bpl * 2) + gpl + 1 + bpl + 1;
    adr &= (0xFFFFFFFFFFFFFFFFULL >> (64 - (hxd * 4)));
    if (!buf || !dat || !amt || !bpg || !gpl || skp >= bpl)
        return;
    if (!(p = *buf)) {
        size_t lines = (skp + amt + bpl - 1) / bpl;
        if (!(p = *buf = (char*) malloc( (lines * lbs) + 1 )))
            return;
    }
    for (; (skp+amt) >= bpl; adr+=bpl, p+=lbs) {
        sprintf( &p[0], "%s%0*"PRIX64"  ", pfx, hxd, adr );
        _hexlinex( &p[n+hxd+2], dat, skp, (bpl-skp), bpg, gpl, x2x );
        dat += (bpl - skp);
        amt -= (bpl - skp);
        skp = 0;
    }
    if (amt) {
        sprintf( &p[0], "%s%0*"PRIX64"  ", pfx, hxd, adr );
        _hexlinex( &p[n+hxd+2], dat, skp, amt, bpg, gpl, x2x );
    }
}

DLL_EXPORT // (Hercules)
void hexdumpaw( const char *pfx, char **buf, const char *dat, size_t skp,
                size_t amt, U64 adr, int bits, size_t bpg, size_t gpl )
{
    _hexdumpxn( pfx, buf, dat, skp, amt, adr, (bits/4), bpg, gpl, _x2x_tab );
}

DLL_EXPORT // (Hercules)
void hexdumpew( const char *pfx, char **buf, const char *dat, size_t skp,
                size_t amt, U64 adr, int bits, size_t bpg, size_t gpl )
{
    _hexdumpxn( pfx, buf, dat, skp, amt, adr, (bits/4), bpg, gpl, e2atab() );
}

/******************************************************************************

   TITLE            _hexlinex

   AUTHOR           "Fish" (David B. Trout)

   COPYRIGHT        (C) 2014 Software Development Laboratories

   VERSION          1.2     (31 Dec 2014)

   DESCRIPTION      hexdump helper to format a single dump line

   SAMPLE

               7F454C  46010101 00000000  .ELF............
     44350000 00000000 34002000 06002800  D5......4.....(.
     23002000 060000                      #......

   PARAMETERS

     buf            pointer to results buffer
     dat            pointer to data to be dumped
     skp            number of dump bytes to skip
     amt            amount of data to be dumped
     bpg            bytes per formatted group
     gpl            formatted groups per line
     x2x            translate table pointer

   NOTES

     Neither buf nor dat may be NULL. amt, bpg and gpl must be >= 1.
     skp must be < (bpg * gpl). skp dump bytes are skipped before
     dumping of dat begins. skp defines where on the first formatted
     dump line the formatting of dat should begin. The maximum amount
     of dat dumped will be (bpg * gpl) - skp, but might be less. No
     buffer overflow checking is performed since this is an internal
     helper function. The caller assumes responsibility for ensuring
     sufficient buffer space is available. The minimum results buffer
     size is: (((bpg * 2) + 1) * gpl) + 1 + (bpg * gpl) + 1 + 1.

******************************************************************************/
static
void _hexlinex( char *buf, const char *dat, size_t skp, size_t amt,
               size_t bpg, size_t gpl, const char* x2x )
{
    size_t i, b, g, s, n;
    register char c;
    if (!buf)
        return;
    if (!dat || !amt || !bpg || !gpl || skp >= (bpg * gpl)) {
        *buf = 0;
        return;
    }
    if ((skp + amt) > (bpg * gpl))
        amt = (bpg * gpl) - skp;
    for (i=g=0, s=skp; g < gpl; g++) {
        for (b=0; b < bpg; b++, i++) {
            n = ((g*(bpg*2))+g+(b*2));
            if (s || (i-skp) >= amt) {
                sprintf( &buf[n],
                    "  " );
                if (s)
                    s--;
            } else
                sprintf(&buf[n],
                    "%02X", dat[i-skp] & 0xff );
        }
        n = ((g*(bpg*2))+g+(b*2));
        sprintf( &buf[((g*(bpg*2))+g+(b*2))], " " );
    }
    n = (bpg*gpl*2)+gpl;
    buf[n] = ' ';
    for (i=0, s=skp, n++; i < (skp+amt); i++) {
        if (s) {
            buf[n+i] = ' ';
            s--;
        } else {
            c = x2x[ (unsigned char) dat[i-skp] ];
            if (' ' == c || isgraph(c & 0xFF))
                buf[n+i] = c;
            else
                buf[n+i] = '.';
        }
    }
    buf[n+i+0] = '\n';
    buf[n+i+1] = 0;
}
