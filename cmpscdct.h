/* CMPSCDCT.H   (C) Copyright "Fish" (David B. Trout), 2012-2014     */
/*              Compression Call Get Dictionary Entry Functions      */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#ifndef _CMPSCDCT_H_
#define _CMPSCDCT_H_    // Code to be compiled ONLY ONCE goes after here

///////////////////////////////////////////////////////////////////////////////
// Constants and helper macros

#define INDEX_SHIFT           3  // (since each dictionary entry is 8 bytes)
#define INDEX_PAGE_SHIFT      (PAGEFRAME_PAGESHIFT - INDEX_SHIFT)
#define INDEX_TO_PAGENUM(i)   ((U16)( (U16)(i) >> INDEX_PAGE_SHIFT))
#define INDEX_TO_PAGEIDX(i)   ((U16)(((U32)(i) << INDEX_SHIFT) & PAGEFRAME_BYTEMASK))
#define PAGENUM_TO_BYTES(n)   ((U32)( (U32)(n) << PAGEFRAME_PAGESHIFT))

///////////////////////////////////////////////////////////////////////////////
// GetDCT parameters block

struct DCTBLK               // GetDCT parameters block
{
    REGS*  regs;            // Pointer to register context
    U64    pDict;           // VADR of dictionary to retrieve entry from
    U8*    maddr[32];       // Cached mainstor addrs of dictionary pages
    int    arn;             // Operand-2 register number
    U8     pkey;            // PSW key
};
typedef struct DCTBLK DCTBLK;

///////////////////////////////////////////////////////////////////////////////
// Generic/Portable EXPANSION CHARACTER ENTRY structure

struct ECE              //  Portable Expansion Character Entry
{
    union { struct {
    U8      ec[7];      //  0:7  Additional Extension characters
    U8      pad1[1];    //  7:1  (always garbage)
    };      struct {
    U64     ec_dw;      //  0:8  (same thing accessed as U64)
    };};

    U16     pptr;       //  8:2  Predecessor pointer
    U8      csl;        // 10:1  Complete-symbol length
    U8      psl;        // 11:1  Partial-symbol length
    U8      ofst;       // 12:1  Offset
    U8      cached;     // 13:1  Cache entry active flag
};
typedef struct ECE ECE;

///////////////////////////////////////////////////////////////////////////////
// Generic/Portable COMPRESSION CHARACTER ENTRY structure

struct CCE              //  Portable Compression Character Entry
{
    union { struct {
    U8      ec[4];      //  0:4  Additional Extension characters
    U8      pad1[4];    //  4:4  (always garbage)
    };      struct {
    U64     ec_dw;      //  0:8  (same thing accessed as U64)
    };};

    union { struct {
    U8      cc[5];      //  8:5  Child characters (first EC of child)
    U8      pad2[3];    // 13:3  (always garbage)
    };      struct {
    U64     cc_dw;      //  8:8  (same thing accessed as U64)
    };};

    U16     cptr;       // 16:2  Child pointer
    U16     ecb;        // 18:2  Examine-child bits for children 1-5
    U16     yy;         // 20:2  Same thing for sibling's child 6-7 or 13-14

    U8      cct;        // 22:1  Child count
    U8      act;        // 23:1  Additional-extension-character count
    U8      mc;         // 24:1  More children flag
    U8      cached;     // 25:1  Cache entry active flag
};
typedef struct CCE CCE;

///////////////////////////////////////////////////////////////////////////////
// Generic/Portable SIBLING DESCRIPTOR structure

struct SDE              //  Portable Sibling Descriptor Entry
{
    union { struct {
    U8      sc[14];     // 0:14  Sibling characters (first EC of sibling)
    U8      pad2[2];    // 14:2  (always garbage)
    };      struct {
    U64     sc_dw;      //  0:8  (same thing accessed as U64)
    U64     sc_dw2;     //  8:8  (same thing accessed as U64)
    };};

    U16     ecb;        // 16:2  Examine-child bits for children 1-7 or 1-14
    U8      sct;        // 18:1  Sibling count
    U8      ms;         // 19:1  More siblings flag
    U8      cached;     // 13:1  Cache entry active flag
};
typedef struct SDE SDE;

///////////////////////////////////////////////////////////////////////////////
// GetECE parameters block

struct ECEBLK               // GetECE parameters block
{
    DCTBLK*  pDCTBLK;       // Ptr to GetDCT parameters block
    ECE*     pECE;          // Ptr to destination ECE structure
    U16      max_index;     // Max index value (same as index's bitmask value)
    ECE      ece[ MAX_DICT_ENTRIES ];   // ECE cache
};
typedef struct ECEBLK ECEBLK;

///////////////////////////////////////////////////////////////////////////////
// GetCCE parameters block

struct CCEBLK               // GetCCE parameters block
{
    DCTBLK*  pDCTBLK;       // Ptr to GetDCT parameters block
    CCE*     pCCE;          // Ptr to destination CCE structure
    U16      max_index;     // Max index value (same as index's bitmask value)
    CCE      cce[ MAX_DICT_ENTRIES ];   // CCE cache
};
typedef struct CCEBLK CCEBLK;

///////////////////////////////////////////////////////////////////////////////
// GetSDn parameters block

struct SDEBLK               // GetSDn parameters block
{
    DCTBLK*  pDCTBLK;       // Ptr to GetDCT parameters block  (cmp dict)
    DCTBLK*  pDCTBLK2;      // Ptr to GetDCT parameters block  (exp dict)
    SDE*     pSDE;          // Ptr to destination SDE structure
    CCE*     pCCE;          // Ptr to Parent CCE structure where extra
                            // Examine-child bits reside, but ONLY if this
                            // is the parent's first sibling. Otherwise NULL.
    SDE      sde[ MAX_DICT_ENTRIES ];   // SDE cache
};
typedef struct SDEBLK SDEBLK;

typedef U8 (CMPSC_FASTCALL GETSD)( U16 index, SDEBLK* pSDEBLK );

///////////////////////////////////////////////////////////////////////////////
#endif // _CMPSCDCT_H_     // Place all 'ARCH_DEP' code after this statement

extern U64 (CMPSC_FASTCALL ARCH_DEP( GetDCT ))( U16 index, DCTBLK* pDCTBLK );
extern U8  (CMPSC_FASTCALL ARCH_DEP( GetECE ))( U16 index, ECEBLK* pECEBLK );
extern U8  (CMPSC_FASTCALL ARCH_DEP( GetCCE ))( U16 index, CCEBLK* pCCEBLK );
extern U8  (CMPSC_FASTCALL ARCH_DEP( GetSD0 ))( U16 index, SDEBLK* pSDEBLK );
extern U8  (CMPSC_FASTCALL ARCH_DEP( GetSD1 ))( U16 index, SDEBLK* pSDEBLK );

///////////////////////////////////////////////////////////////////////////////
