/* CMPSCBIT.H   (C) Copyright "Fish" (David B. Trout), 2012-2014     */
/*              Compression Call Bit Extraction Macros               */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#ifndef _CMPSCBIT_H_
#define _CMPSCBIT_H_

/*********************************************************************\
        Masks for extracting bits from unsigned 64-bit value
\*********************************************************************/

// NOTE: for the benefit of non-mainframe people, mainframes are
//       big-endian and thus we count our bits from left to right.

// (right justified)

#define U8R1        0x01
#define U8R2        0x03
#define U8R3        0x07
#define U8R4        0x0F
#define U8R5        0x1F
#define U8R6        0x3F
#define U8R7        0x7F
#define U8R8        0xFF

#define U16R1       0x0001
#define U16R2       0x0003
#define U16R3       0x0007
#define U16R4       0x000F
#define U16R5       0x001F
#define U16R6       0x003F
#define U16R7       0x007F
#define U16R8       0x00FF
#define U16R9       0x01FF
#define U16R10      0x03FF
#define U16R11      0x07FF
#define U16R12      0x0FFF
#define U16R13      0x1FFF
#define U16R14      0x3FFF
#define U16R15      0x7FFF
#define U16R16      0xFFFF

// (left justified)

#define U8L1        0x80
#define U8L2        0xC0
#define U8L3        0xE0
#define U8L4        0xF0
#define U8L5        0xF8
#define U8L6        0xFC
#define U8L7        0xFE
#define U8L8        0xFF

#define U16L1       0x8000
#define U16L2       0xC000
#define U16L3       0xE000
#define U16L4       0xF000
#define U16L5       0xF800
#define U16L6       0xFC00
#define U16L7       0xFE00
#define U16L8       0xFF00
#define U16L9       0xFF80
#define U16L10      0xFFC0
#define U16L11      0xFFE0
#define U16L12      0xFFF0
#define U16L13      0xFFF8
#define U16L14      0xFFFC
#define U16L15      0xFFFE
#define U16L16      0xFFFF

/*********************************************************************\
        Macros for extracting bits from unsigned 64-bit value
\*********************************************************************/

// NOTE: for the benefit of non-mainframe people, mainframes are
//       big-endian and thus we count our bits from left to right.

#define U64BITSR( u64, start, len,        mask )    (((u64) >> (64-(start+len  ))) & mask)
#define U64BITSL( u64, start, len, width, mask )    (((u64) >> (64-(start+width))) & mask)

#define ECE_U8R(  start, len )      ((U8)  U64BITSR( ece, start, len,     U8R  ## len ))
#define ECE_U16R( start, len )      ((U16) U64BITSR( ece, start, len,     U16R ## len ))

#define CCE_U8R(  start, len )      ((U8)  U64BITSR( cce, start, len,     U8R  ## len ))
#define CCE_U16R( start, len )      ((U16) U64BITSR( cce, start, len,     U16R ## len ))
#define SD1_U8R(  start, len )      ((U8)  U64BITSR( sd1, start, len,     U8R  ## len ))

#define CCE_U16L( start, len )      ((U16) U64BITSL( cce, start, len, 16, U16L ## len ))
#define SD1_U16L( start, len )      ((U16) U64BITSL( sd1, start, len, 16, U16L ## len ))

#endif // _CMPSCBIT_H_
