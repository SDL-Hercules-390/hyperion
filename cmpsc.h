/* CMPSC.H      (C) Copyright "Fish" (David B. Trout), 2012-2019     */
/*              S/390 Compression Call Instruction Functions         */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#ifndef _CMPSC_H_
#define _CMPSC_H_   // Code to be compiled ONLY ONCE goes after here

///////////////////////////////////////////////////////////////////////////////
// Tweakable constants (i.e. "tuning knobs")...

#define MIN_CDSS              ( 1 )                   // SA22-7208-01
#define MAX_CDSS              ( 5 )                   // SA22-7208-01
#define MAX_DICT_ENTRIES      ( 1 << (8 + MAX_CDSS )) // SA22-7208-01
#define MAX_SYMLEN            ( 260 )                 // SA22-7208-01
#define MAX_CHILDREN          ( 260 )                 // SA22-7208-01

#define MIN_CMPSC_CPU_AMT     ( 1024 )                // 1K
#define DEF_CMPSC_CPU_AMT     ( 1024 * 1024 )         // 1MB
#define MAX_CMPSC_CPU_AMT     ( 1024 * 1024 * 1024 )  // 1GB

#define CMPSC_INLINE          INLINE                // __inline
#define CMPSC_FASTCALL        ATTR_REGPARM( 3 )     // __fastcall

#define CMPSC_EXPAND8         // (#define to enable expand-8 logic)
#define CMPSC_SYMCACHE        // (#define to enable symcache logic)

#define CMPSC_SYMCACHE_SIZE   ( 1024 * 32 )     // (must be < 64K)

///////////////////////////////////////////////////////////////////////////////
// Dictionary sizes in bytes by CDSS

extern const U32 g_nDictSize[ MAX_CDSS ];   // (table of dict sizes in bytes)

///////////////////////////////////////////////////////////////////////////////
// CMPSCBLK: Compression Call parameters block

struct CMPSCBLK             // CMPSC instruction control block
{
    U64     nLen1;          // First-Operand Length               (DST)
    U64     nLen2;          // Second-Operand Length              (SRC)
    void*   dbg;            // Debugging context or NULL          (opt)
    REGS*   regs;           // Pointer to register context        (HERC)
    U64     pOp1;           // First-Operand Address              (DST)
    U64     pOp2;           // Second-Operand Address             (SRC)
    U64     pDict;          // Dictionary-Origin                  (CMP/EXP)
    U32     nCPUAmt;        // CPU determined processing limit    (CC3)
    U16     stt;            // Symbol-translation-table offset    (0-511)
    U16     pic;            // Program Interruption code          (7,5,etc)
    U8      r1;             // Operand-1 register number          (HERC)
    U8      r2;             // Operand-2 register number          (HERC)
    U8      st;             // Symbol-translation option          (0/1)
    U8      f1;             // Format-1 sibling descriptors flag  (0/1)
    U8      cdss;           // Compressed-data symbol size        (1-5)
    U8      cbn;            // Compressed-data bit number         (0-7)
    U8      cc;             // Condition-Code value               (0-3)
    U8      zp;             // Zero-pad operand-1 when done       (pad)
};
typedef struct CMPSCBLK CMPSCBLK;

///////////////////////////////////////////////////////////////////////////////
#endif // _CMPSC_H_     // Place all 'ARCH_DEP' code after this statement

///////////////////////////////////////////////////////////////////////////////
// COMPRESSION CALL: Compress or Expand requested data
//
// pCMPSCBLK:   ptr to Compression Call parameters block (CMPSCBLK)
// Returns:     TRUE/FALSE (success/failure)
//
///////////////////////////////////////////////////////////////////////////////

extern U8 (CMPSC_FASTCALL ARCH_DEP( cmpsc_Expand  ))( CMPSCBLK* pCMPSCBLK );
extern U8 (CMPSC_FASTCALL ARCH_DEP( cmpsc_Compress))( CMPSCBLK* pCMPSCBLK );

///////////////////////////////////////////////////////////////////////////////
// Our's is the master header so we automatically #include all of our other
// headers too so all modules just need to #include ours and not any others.

#include "cmpscbit.h"       // (Bit Extraction Macros)
#include "cmpscmem.h"       // (Memory Access Functions)
#include "cmpscget.h"       // (Get Next Index Functions)
#include "cmpscput.h"       // (Put Next Index Functions)
#include "cmpscdct.h"       // (Get Dictionary Entry Functions)
#include "cmpscdbg.h"       // (Debugging Functions)

///////////////////////////////////////////////////////////////////////////////
