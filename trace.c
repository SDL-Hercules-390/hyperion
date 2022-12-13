/* TRACE.C      (C) Copyright Jan Jaeger, 2000-2012                  */
/*              Implicit tracing functions                           */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/* Interpretive Execution - (C) Copyright Jan Jaeger, 1999-2012      */
/* z/Architecture support - (C) Copyright Jan Jaeger, 1999-2012      */

/*-------------------------------------------------------------------*/
/* This module contains procedures for creating entries in the       */
/* system trace table as described in the manuals:                   */
/* SA22-7201 ESA/390 Principles of Operation                         */
/* SA22-7832 z/Architecture Principles of Operation                  */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* Additional credits:                                               */
/*      ASN-and-LX-reuse facility - Roger Bowler            July 2004*/
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

#define _TRACE_C_
#define _HENGINE_DLL_

#include "hercules.h"
#include "opcode.h"
#include "inline.h"

#if defined(FEATURE_TRACING)

#if !defined(_TRACE_H)
#define _TRACE_H

/*-------------------------------------------------------------------*/
/* Format definitions for trace table entries                        */
/*-------------------------------------------------------------------*/
typedef struct _TRACE_F1_BR {
FWORD   newia24;                        /* Bits 0-7 are zeros        */
} TRACE_F1_BR;

typedef struct _TRACE_F2_BR {
FWORD   newia31;                        /* Bit 0 is one              */
} TRACE_F2_BR;

typedef struct _TRACE_F3_BR {
BYTE    format;                         /* B'01010010'               */
#define TRACE_F3_BR_FMT 0x52
BYTE    fmt2;                           /* B'1100' B'0000'           */
#define TRACE_F3_BR_FM2 0xC0
HWORD   resv;
DBLWRD  newia64;
} TRACE_F3_BR;

typedef struct _TRACE_F1_BSG {
BYTE    format;                         /* B'01000001'               */
#define TRACE_F1_BSG_FMT 0x41
BYTE    alet[3];
FWORD   newia;
} TRACE_F1_BSG;

typedef struct _TRACE_F2_BSG {
BYTE    format;                         /* B'01000010'               */
#define TRACE_F2_BSG_FMT 0x42
BYTE    alet[3];
DBLWRD  newia;
} TRACE_F2_BSG;

typedef struct _TRACE_F1_MS {
BYTE    format;
#define TRACE_F1_MS_FMT 0x51
BYTE    fmt2;
#define TRACE_F1_MS_FM2 0x30
HWORD   resv;
FWORD   newia;
} TRACE_F1_MS;

typedef struct _TRACE_F2_MS {
BYTE    format;
#define TRACE_F2_MS_FMT 0x51
BYTE    fmt2;
#define TRACE_F2_MS_FM2 0x20
HWORD   resv;
FWORD   newia;
} TRACE_F2_MS;

typedef struct _TRACE_F3_MS {
BYTE    format;
#define TRACE_F3_MS_FMT 0x52
BYTE    fmt2;
#define TRACE_F3_MS_FM2 0x60
HWORD   resv;
DBLWRD  newia;
} TRACE_F3_MS;


typedef struct _TRACE_F1_MSB {
BYTE    format;
#define TRACE_F1_MSB_FMT 0x51
BYTE    fmt2;
#define TRACE_F1_MSB_FM2 0xA0
HWORD   resv;
FWORD   newia;
} TRACE_F1_MSB;

typedef struct _TRACE_F2_MSB {
BYTE    format;
#define TRACE_F2_MSB_FMT 0x51
BYTE    fmt2;
#define TRACE_F2_MSB_FM2 0xB0
HWORD   resv;
FWORD   newia;
} TRACE_F2_MSB;

typedef struct _TRACE_F3_MSB {
BYTE    format;
#define TRACE_F3_MSB_FMT 0x52
BYTE    fmt2;
#define TRACE_F3_MSB_FM2 0xF0
HWORD   resv;
DBLWRD  newia;
} TRACE_F3_MSB;

typedef struct _TRACE_F1_PT {
BYTE    format;
#define TRACE_F1_PT_FMT 0x31
BYTE    pswkey;
#define TRACE_F1_PT_FM2 0x00
HWORD   newpasn;
FWORD   r2;
} TRACE_F1_PT;

typedef struct _TRACE_F2_PT {
BYTE    format;
#define TRACE_F2_PT_FMT 0x31
BYTE    pswkey;
#define TRACE_F2_PT_FM2 0x08
HWORD   newpasn;
FWORD   r2;
} TRACE_F2_PT;

typedef struct _TRACE_F3_PT {
BYTE    format;
#define TRACE_F3_PT_FMT 0x32
BYTE    pswkey;
#define TRACE_F3_PT_FM2 0x0C
HWORD   newpasn;
DBLWRD  r2;
} TRACE_F3_PT;

typedef struct _TRACE_F1_SSAR {
BYTE    format;
#define TRACE_F1_SSAR_FMT 0x10
BYTE    extfmt;
HWORD   newsasn;
} TRACE_F1_SSAR;

typedef struct _TRACE_F1_TRACE {
BYTE    format;
BYTE    zero;
HWORD   tod1631;
FWORD   tod3263;
FWORD   operand;
FWORD   regs[16];
} TRACE_F1_TRACE;

typedef struct _TRACE_F2_TRACE {
BYTE    format;
BYTE    extfmt;
HWORD   tod1631;
DBLWRD  tod3279;
FWORD   operand;
DBLWRD  regs[16];
} TRACE_F2_TRACE;

typedef struct _TRACE_F1_PR {
BYTE    format;
#define TRACE_F1_PR_FMT 0x32
BYTE    pswkey;
#define TRACE_F1_PR_FM2 0x00
HWORD   newpasn;
FWORD   retna;
FWORD   newia;
} TRACE_F1_PR;

typedef struct _TRACE_F2_PR {
BYTE    format;
#define TRACE_F2_PR_FMT 0x32
BYTE    pswkey;
#define TRACE_F2_PR_FM2 0x02
HWORD   newpasn;
FWORD   retna;
FWORD   newia;
} TRACE_F2_PR;

typedef struct _TRACE_F3_PR {
BYTE    format;
#define TRACE_F3_PR_FMT 0x33
BYTE    pswkey;
#define TRACE_F3_PR_FM2 0x03
HWORD   newpasn;
FWORD   retna;
DBLWRD  newia;
} TRACE_F3_PR;

typedef struct _TRACE_F4_PR {
BYTE    format;
#define TRACE_F4_PR_FMT 0x32
BYTE    pswkey;
#define TRACE_F4_PR_FM2 0x08
HWORD   newpasn;
FWORD   retna;
FWORD   newia;
} TRACE_F4_PR;

typedef struct _TRACE_F5_PR {
BYTE    format;
#define TRACE_F5_PR_FMT 0x32
BYTE    pswkey;
#define TRACE_F5_PR_FM2 0x0A
HWORD   newpasn;
FWORD   retna;
FWORD   newia;
} TRACE_F5_PR;

typedef struct _TRACE_F6_PR {
BYTE    format;
#define TRACE_F6_PR_FMT 0x33
BYTE    pswkey;
#define TRACE_F6_PR_FM2 0x0B
HWORD   newpasn;
FWORD   retna;
DBLWRD  newia;
} TRACE_F6_PR;

typedef struct _TRACE_F7_PR {
BYTE    format;
#define TRACE_F7_PR_FMT 0x33
BYTE    pswkey;
#define TRACE_F7_PR_FM2 0x0C
HWORD   newpasn;
DBLWRD  retna;
FWORD   newia;
} TRACE_F7_PR;

typedef struct _TRACE_F8_PR {
BYTE    format;
#define TRACE_F8_PR_FMT 0x33
BYTE    pswkey;
#define TRACE_F8_PR_FM2 0x0E
HWORD   newpasn;
DBLWRD  retna;
FWORD   newia;
} TRACE_F8_PR;

typedef struct _TRACE_F9_PR {
BYTE    format;
#define TRACE_F9_PR_FMT 0x34
BYTE    pswkey;
#define TRACE_F9_PR_FM2 0x0F
HWORD   newpasn;
DBLWRD  retna;
DBLWRD  newia;
} TRACE_F9_PR;

typedef struct _TRACE_F1_PC {
BYTE    format;
#define TRACE_F1_PC_FMT 0x21
BYTE    pswkey_pcnum_hi;
HWORD   pcnum_lo;
FWORD   retna;
} TRACE_F1_PC;

typedef struct _TRACE_F2_PC {
BYTE    format;
#define TRACE_F2_PC_FMT 0x22
BYTE    pswkey_pcnum_hi;
HWORD   pcnum_lo;
DBLWRD  retna;
} TRACE_F2_PC;

typedef struct _TRACE_F3_PC {
BYTE    format;
#define TRACE_F3_PC_FMT 0x21
BYTE    pswkey_pcnum_hi;
HWORD   pcnum_lo;
FWORD   retna;
} TRACE_F3_PC;

typedef struct _TRACE_F4_PC {
BYTE    format;
#define TRACE_F4_PC_FMT 0x22
BYTE    pswkey_pcnum_hi;
HWORD   pcnum_lo;
DBLWRD  retna;
} TRACE_F4_PC;

typedef struct _TRACE_F5_PC {
BYTE    format;
#define TRACE_F5_PC_FMT 0x22
BYTE    pswkey;
#define TRACE_F5_PC_FM2 0x08
HWORD   resv;
FWORD   retna;
FWORD   pcnum;
} TRACE_F5_PC;

typedef struct _TRACE_F6_PC {
BYTE    format;
#define TRACE_F6_PC_FMT 0x22
BYTE    pswkey;
#define TRACE_F6_PC_FM2 0x0A
HWORD   resv;
FWORD   retna;
FWORD   pcnum;
} TRACE_F6_PC;

typedef struct _TRACE_F7_PC {
BYTE    format;
#define TRACE_F7_PC_FMT 0x23
BYTE    pswkey;
#define TRACE_F7_PC_FM2 0x0E
HWORD   resv;
DBLWRD  retna;
FWORD   pcnum;
} TRACE_F7_PC;


typedef struct _TRACE_F1_TR {
BYTE    format;
#define TRACE_F1_TR_FMT 0x70
BYTE    fmt2;
#define TRACE_F1_TR_FM2 0x00
HWORD   clk16;
FWORD   clk32;
FWORD   operand;
FWORD   reg[16];
} TRACE_F1_TR;

typedef struct _TRACE_F2_TR {
BYTE    format;
#define TRACE_F2_TR_FMT 0x70
BYTE    fmt2;
#define TRACE_F2_TR_FM2 0x80
HWORD   clk0;
FWORD   clk16;
FWORD   clk48;
FWORD   operand;
DBLWRD  reg[16];
} TRACE_F2_TR;

#endif /*!defined(_TRACE_H)*/

/*-------------------------------------------------------------------*/
/* Reserve space for a new trace entry                               */
/*                                                                   */
/* Input:                                                            */
/*      size    Number of bytes required for trace entry             */
/*      regs    Pointer to the CPU register context                  */
/* Output:                                                           */
/*      abs_guest  Guest absolute address of trace entry (if SIE)    */
/* Return value:                                                     */
/*      Absolute address of new trace entry                          */
/*                                                                   */
/*      This function does not return if a program check occurs.     */
/*-------------------------------------------------------------------*/
static inline RADR ARCH_DEP(get_trace_entry) (RADR *abs_guest, int size, REGS *regs)
{
RADR    n;                              /* Addr of trace table entry */

    /* Obtain the trace entry address from control register 12 */
    n = regs->CR(12) & CR12_TRACEEA;

    /* Apply low-address protection to trace entry address */
    if (ARCH_DEP(is_low_address_protected) (n, regs))
    {
#ifdef FEATURE_SUPPRESSION_ON_PROTECTION
        regs->TEA = (n & STORAGE_KEY_PAGEMASK);
        regs->excarid = 0;
#endif /*FEATURE_SUPPRESSION_ON_PROTECTION*/
        ARCH_DEP(program_interrupt) (regs, PGM_PROTECTION_EXCEPTION);
    }

    /* Program check if trace entry is outside main storage */
    if ( n > regs->mainlim )
        ARCH_DEP(program_interrupt) (regs, PGM_ADDRESSING_EXCEPTION);

    /* Program check if storing would overflow a 4K page boundary */
    if ( ((n + size) & PAGEFRAME_PAGEMASK) != (n & PAGEFRAME_PAGEMASK) )
        ARCH_DEP(program_interrupt) (regs, PGM_TRACE_TABLE_EXCEPTION);

    /* Convert trace entry real address to absolute address */
    n = APPLY_PREFIXING (n, regs->PX);

#if defined(_FEATURE_SIE)
    *abs_guest = n;

    SIE_TRANSLATE(&n, ACCTYPE_WRITE, regs);

#endif /*defined(_FEATURE_SIE)*/

    return n;

} /* end function ARCH_DEP(get_trace_entry) */


/*-------------------------------------------------------------------*/
/* Commit a new trace entry                                          */
/*                                                                   */
/* Input:                                                            */
/*      abs_guest  Guest absolute address of trace entry (if SIE)    */
/*      raddr   Absolute address of trace entry                      */
/*      size    Number of bytes reserved for trace entry             */
/*      regs    Pointer to the CPU register context                  */
/* Return value:                                                     */
/*      Updated value for CR12 after committing the trace entry      */
/*-------------------------------------------------------------------*/
static inline CREG ARCH_DEP(set_trace_entry) (RADR abs_guest, RADR raddr, int size, REGS *regs)
{
#if defined(_FEATURE_SIE)
RADR abs_host;

    abs_host = raddr;
#endif /*defined(_FEATURE_SIE)*/

    raddr += size;

#if defined(_FEATURE_SIE)
    /* Recalculate the Guest absolute address */
    raddr = abs_guest + (raddr - abs_host);
#endif /*defined(_FEATURE_SIE)*/

    /* Convert trace entry absolute address back to real address */
    raddr = APPLY_PREFIXING (raddr, regs->PX);

    /* Return updated value of control register 12 */
    return (regs->CR(12) & ~CR12_TRACEEA) | raddr;

} /* end function ARCH_DEP(set_trace_entry) */


/*-------------------------------------------------------------------*/
/* Form implicit branch trace entry                                  */
/*                                                                   */
/* Input:                                                            */
/*      amode   Non-zero if branch destination is a 31-bit address   */
/*              or a 64 bit address                                  */
/*      ia      Branch destination address                           */
/*      regs    Pointer to the CPU register context                  */
/* Return value:                                                     */
/*      Updated value for CR12 after adding new trace entry          */
/*                                                                   */
/*      This function does not return if a program check occurs.     */
/*-------------------------------------------------------------------*/
CREG ARCH_DEP(trace_br) (int amode, VADR ia, REGS *regs)
{
RADR raddr;
RADR ag;
int  size;

#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
    if(amode && ia > 0xFFFFFFFFULL)
    {
        TRACE_F3_BR *tte;
        size = sizeof(TRACE_F3_BR);
        raddr = ARCH_DEP(get_trace_entry) (&ag, size, regs);
        tte = (void*)(regs->mainstor + raddr);
        tte->format = TRACE_F3_BR_FMT;
        tte->fmt2 = TRACE_F3_BR_FM2;
        STORE_HW(tte->resv,0);
        STORE_DW(tte->newia64,ia);
    }
    else
#endif /*defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/
    if(amode)
    {
        TRACE_F2_BR *tte;
        size = sizeof(TRACE_F2_BR);
        raddr = ARCH_DEP(get_trace_entry) (&ag, size, regs);
        tte = (void*)(regs->mainstor + raddr);
        STORE_FW(tte->newia31,ia | 0x80000000);
    }
    else
    {
        TRACE_F1_BR *tte;
        size = sizeof(TRACE_F1_BR);
        raddr = ARCH_DEP(get_trace_entry) (&ag, size, regs);
        tte = (void*)(regs->mainstor + raddr);
        STORE_FW(tte->newia24,ia & 0x00FFFFFF);
    }

    return ARCH_DEP(set_trace_entry) (ag, raddr, size, regs);

} /* end function ARCH_DEP(trace_br) */


#if defined(FEATURE_SUBSPACE_GROUP)
/*-------------------------------------------------------------------*/
/* Form implicit BSG trace entry                                     */
/*                                                                   */
/* Input:                                                            */
/*      alet    Destination address space ALET                       */
/*      ia      Branch destination address                           */
/*      regs    Pointer to the CPU register context                  */
/* Return value:                                                     */
/*      Updated value for CR12 after adding new trace entry          */
/*                                                                   */
/*      This function does not return if a program check occurs.     */
/*-------------------------------------------------------------------*/
CREG ARCH_DEP(trace_bsg) (U32 alet, VADR ia, REGS *regs)
{
RADR raddr;
RADR ag;
int  size;

#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
    if(regs->psw.amode64)
    {
        TRACE_F2_BSG *tte;
        size = sizeof(TRACE_F2_BSG);
        raddr = ARCH_DEP(get_trace_entry) (&ag, size, regs);
        tte = (void*)(regs->mainstor + raddr);
        tte->format = TRACE_F2_BSG_FMT;
        tte->alet[0] = (alet >> 16) & 0xFF;
        tte->alet[1] = (alet >> 8) & 0xFF;
        tte->alet[2] = alet & 0xFF;
        STORE_DW(tte->newia,ia);
    }
    else
#endif /*defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/
    {
        TRACE_F1_BSG *tte;
        size = sizeof(TRACE_F1_BSG);
        raddr = ARCH_DEP(get_trace_entry) (&ag, size, regs);
        tte = (void*)(regs->mainstor + raddr);
        tte->format = TRACE_F1_BSG_FMT;
        tte->alet[0] = ((alet >> 17) & 0x80) | ((alet >> 16) & 0x7F);
        tte->alet[1] = (alet >> 8) & 0xFF;
        tte->alet[2] = alet & 0xFF;
        if ((ia & 0x80000000) == 0)
            ia &=0x00FFFFFF;
        STORE_FW(tte->newia,ia);
    }

    return ARCH_DEP(set_trace_entry) (ag, raddr, size, regs);

} /* end function ARCH_DEP(trace_bsg) */
#endif /*defined(FEATURE_SUBSPACE_GROUP)*/


/*-------------------------------------------------------------------*/
/* Form implicit SSAR/SSAIR trace entry                              */
/*                                                                   */
/* Input:                                                            */
/*      ssair   1=SSAIR instruction, 0=SSAR instruction              */
/*      sasn    Secondary address space number                       */
/*      regs    Pointer to the CPU register context                  */
/* Return value:                                                     */
/*      Updated value for CR12 after adding new trace entry          */
/*                                                                   */
/*      This function does not return if a program check occurs.     */
/*-------------------------------------------------------------------*/
CREG ARCH_DEP(trace_ssar) (int ssair, U16 sasn, REGS *regs)
{
RADR raddr;
RADR ag;
int  size;
BYTE nbit = (ssair ? 1 : 0);

    {
        TRACE_F1_SSAR *tte;
        size = sizeof(TRACE_F1_SSAR);
        raddr = ARCH_DEP(get_trace_entry) (&ag, size, regs);
        tte = (void*)(regs->mainstor + raddr);
        tte->format = TRACE_F1_SSAR_FMT;
        tte->extfmt = 0 | nbit;
        STORE_HW(tte->newsasn,sasn);
    }

    return ARCH_DEP(set_trace_entry) (ag, raddr, size, regs);

} /* end function ARCH_DEP(trace_ssar) */


/*-------------------------------------------------------------------*/
/* Form implicit PC trace entry                                      */
/*                                                                   */
/* Input:                                                            */
/*      pcea    PC instruction effective address (20 or 32 bits)     */
/*      regs    Pointer to the CPU register context                  */
/* Return value:                                                     */
/*      Updated value for CR12 after adding new trace entry          */
/*                                                                   */
/*      This function does not return if a program check occurs.     */
/*-------------------------------------------------------------------*/
CREG ARCH_DEP(trace_pc) (U32 pcea, REGS *regs)
{
RADR raddr;
RADR ag;
int  size;
#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
int  eamode;
#endif

    MAYBE_SET_PSW_IA_FROM_IP(regs);

#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)

    eamode = regs->psw.amode64;

    if (ASN_AND_LX_REUSE_ENABLED(regs))
    {
        if ((pcea & PC_BIT44) && regs->psw.amode64 && regs->psw.IA_H)
        {
            /* In 64-bit mode, regardless of resulting mode, when
               ASN-and-LX-reuse is enabled, 32-bit PC number is used,
               and bits 0-31 of return address are not all zeros */
            TRACE_F7_PC *tte;
            size = sizeof(TRACE_F7_PC);
            raddr = ARCH_DEP(get_trace_entry) (&ag, size, regs);
            tte = (void*)(regs->mainstor + raddr);
            tte->format = TRACE_F7_PC_FMT;
            tte->pswkey = regs->psw.pkey | TRACE_F7_PC_FM2 | eamode;
            STORE_HW(tte->resv, 0x0000);
            STORE_DW(tte->retna, regs->psw.IA_G | PROBSTATE(&regs->psw));
            STORE_FW(tte->pcnum, pcea);
        }
        else
        if ((pcea & PC_BIT44) && regs->psw.amode64)
        {
            /* In 64-bit mode, regardless of resulting mode, when
               ASN-and-LX-reuse is enabled, 32-bit PC number is used,
               and bits 0-31 of return address are all zeros */
            TRACE_F6_PC *tte;
            size = sizeof(TRACE_F6_PC);
            raddr = ARCH_DEP(get_trace_entry) (&ag, size, regs);
            tte = (void*)(regs->mainstor + raddr);
            tte->format = TRACE_F6_PC_FMT;
            tte->pswkey = regs->psw.pkey | TRACE_F6_PC_FM2 | eamode;
            STORE_HW(tte->resv, 0x0000);
            STORE_FW(tte->retna, regs->psw.IA_L | PROBSTATE(&regs->psw));
            STORE_FW(tte->pcnum, pcea);
        }
        else
        if ((pcea & PC_BIT44))
        {
            /* In 24-bit or 31-bit mode, regardless of resulting mode, when
               ASN-and-LX-reuse is enabled and 32-bit PC number is used */
            TRACE_F5_PC *tte;
            size = sizeof(TRACE_F5_PC);
            raddr = ARCH_DEP(get_trace_entry) (&ag, size, regs);
            tte = (void*)(regs->mainstor + raddr);
            tte->format = TRACE_F5_PC_FMT;
            tte->pswkey = regs->psw.pkey | TRACE_F5_PC_FM2 | eamode;
            STORE_HW(tte->resv, 0x0000);
            STORE_FW(tte->retna, (regs->psw.amode << 31) | regs->psw.IA_L | PROBSTATE(&regs->psw));
            STORE_FW(tte->pcnum, pcea);
        }
        else
        if(regs->psw.amode64)
        {
            /* In 64-bit mode, regardless of resulting mode, when
               ASN-and-LX-reuse is enabled and 20-bit PC number is used */
            TRACE_F4_PC *tte;
            size = sizeof(TRACE_F4_PC);
            raddr = ARCH_DEP(get_trace_entry) (&ag, size, regs);
            tte = (void*)(regs->mainstor + raddr);
            tte->format = TRACE_F4_PC_FMT;
            tte->pswkey_pcnum_hi = regs->psw.pkey | ((pcea & 0xF0000) >> 16);
            STORE_HW(tte->pcnum_lo, pcea & 0x0FFFF);
            STORE_DW(tte->retna, regs->psw.IA_G | PROBSTATE(&regs->psw));
        }
        else
        {
            /* In 24-bit or 31-bit mode, regardless of resulting mode, when
               ASN-and-LX-reuse is enabled and 20-bit PC number is used */
            TRACE_F3_PC *tte;
            size = sizeof(TRACE_F3_PC);
            raddr = ARCH_DEP(get_trace_entry) (&ag, size, regs);
            tte = (void*)(regs->mainstor + raddr);
            tte->format = TRACE_F3_PC_FMT;
            tte->pswkey_pcnum_hi = regs->psw.pkey | ((pcea & 0xF0000) >> 16);
            STORE_HW(tte->pcnum_lo, pcea & 0x0FFFF);
            STORE_FW(tte->retna, (regs->psw.amode << 31) | regs->psw.IA_L | PROBSTATE(&regs->psw));
        }
    } /* end ASN_AND_LX_REUSE_ENABLED */
    else
    if(regs->psw.amode64)
    {
        /* In 64-bit mode, regardless of resulting mode,
           when ASN-and-LX-reuse is not enabled */
        TRACE_F2_PC *tte;
        size = sizeof(TRACE_F2_PC);
        raddr = ARCH_DEP(get_trace_entry) (&ag, size, regs);
        tte = (void*)(regs->mainstor + raddr);
        tte->format = TRACE_F2_PC_FMT;
        tte->pswkey_pcnum_hi = regs->psw.pkey | ((pcea & 0xF0000) >> 16);
        STORE_HW(tte->pcnum_lo, pcea & 0x0FFFF);
        STORE_DW(tte->retna, regs->psw.IA_G | PROBSTATE(&regs->psw));
    }
    else
#endif /*defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/
    {
        /* In 24-bit or 31-bit mode, regardless of resulting mode,
           when ASN-and-LX-reuse is not enabled */
        TRACE_F1_PC *tte;
        size = sizeof(TRACE_F1_PC);
        raddr = ARCH_DEP(get_trace_entry) (&ag, size, regs);
        tte = (void*)(regs->mainstor + raddr);
        tte->format = TRACE_F1_PC_FMT;
        tte->pswkey_pcnum_hi = regs->psw.pkey | ((pcea & 0xF0000) >> 16);
        STORE_HW(tte->pcnum_lo, pcea & 0x0FFFF);
        STORE_FW(tte->retna, (regs->psw.amode << 31) | regs->psw.IA_L | PROBSTATE(&regs->psw));
    }

    return ARCH_DEP(set_trace_entry) (ag, raddr, size, regs);

} /* end function ARCH_DEP(trace_pc) */

#if defined(_MSVC_)
  /* Workaround for "fatal error C1001: INTERNAL COMPILER ERROR" in MSVC */
  #pragma optimize("",off)
#endif /*defined(_MSVC_)*/

#if defined(FEATURE_LINKAGE_STACK)
/*-------------------------------------------------------------------*/
/* Form implicit PR trace entry                                      */
/*                                                                   */
/* Input:                                                            */
/*      newregs Pointer to registers after PR instruction            */
/*      regs    Pointer to registers before PR instruction           */
/* Return value:                                                     */
/*      Updated value for CR12 after adding new trace entry          */
/*                                                                   */
/*      This function does not return if a program check occurs.     */
/*-------------------------------------------------------------------*/
CREG ARCH_DEP(trace_pr) (REGS *newregs, REGS *regs)
{
RADR raddr;
RADR ag;
int  size;

    MAYBE_SET_PSW_IA_FROM_IP(regs);
    MAYBE_SET_PSW_IA_FROM_IP(newregs);

#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
    if(!regs->psw.amode64 && !newregs->psw.amode64)
#endif /*defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/
    {
        TRACE_F1_PR *tte;
        size = sizeof(TRACE_F1_PR);
        raddr = ARCH_DEP(get_trace_entry) (&ag, size, regs);
        tte = (void*)(regs->mainstor + raddr);
        tte->format = TRACE_F1_PR_FMT;
        tte->pswkey = regs->psw.pkey | TRACE_F1_PR_FM2;
        STORE_HW(tte->newpasn, newregs->CR_LHL(4));
        STORE_FW(tte->retna, (newregs->psw.amode << 31)
                                | newregs->psw.IA_L | PROBSTATE(&newregs->psw));
        STORE_FW(tte->newia, (regs->psw.amode << 31)
                                 | regs->psw.IA_L);
    }
#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
    else
    if(regs->psw.amode64 && regs->psw.IA_H == 0 && !newregs->psw.amode64)
    {
        TRACE_F2_PR *tte;
        size = sizeof(TRACE_F2_PR);
        raddr = ARCH_DEP(get_trace_entry) (&ag, size, regs);
        tte = (void*)(regs->mainstor + raddr);
        tte->format = TRACE_F2_PR_FMT;
        tte->pswkey = regs->psw.pkey | TRACE_F2_PR_FM2;
        STORE_HW(tte->newpasn, newregs->CR_LHL(4));
        STORE_FW(tte->retna, (newregs->psw.amode << 31)
                                | newregs->psw.IA_L | PROBSTATE(&newregs->psw));
        STORE_FW(tte->newia, regs->psw.IA_L);
    }
    else
    if(regs->psw.amode64 && regs->psw.IA_H != 0 && !newregs->psw.amode64)
    {
        TRACE_F3_PR *tte;
        size = sizeof(TRACE_F3_PR);
        raddr = ARCH_DEP(get_trace_entry) (&ag, size, regs);
        tte = (void*)(regs->mainstor + raddr);
        tte->format = TRACE_F3_PR_FMT;
        tte->pswkey = regs->psw.pkey | TRACE_F3_PR_FM2;
        STORE_HW(tte->newpasn, newregs->CR_LHL(4));
        STORE_FW(tte->retna, (newregs->psw.amode << 31)
                                | newregs->psw.IA_L | PROBSTATE(&newregs->psw));
        STORE_DW(tte->newia, regs->psw.IA_G);
    }
    else
    if(!regs->psw.amode64  && newregs->psw.amode64 && newregs->psw.IA_H == 0)
    {
        TRACE_F4_PR *tte;
        size = sizeof(TRACE_F4_PR);
        raddr = ARCH_DEP(get_trace_entry) (&ag, size, regs);
        tte = (void*)(regs->mainstor + raddr);
        tte->format = TRACE_F4_PR_FMT;
        tte->pswkey = regs->psw.pkey | TRACE_F4_PR_FM2;
        STORE_HW(tte->newpasn, newregs->CR_LHL(4));
        STORE_FW(tte->retna,  newregs->psw.IA_L | PROBSTATE(&newregs->psw));
        STORE_FW(tte->newia, (regs->psw.amode << 31)
                                 | regs->psw.IA_L);
    }
    else
    if(regs->psw.amode64 && regs->psw.IA_H == 0 && newregs->psw.amode64 && newregs->psw.IA_H == 0)
    {
        TRACE_F5_PR *tte;
        size = sizeof(TRACE_F5_PR);
        raddr = ARCH_DEP(get_trace_entry) (&ag, size, regs);
        tte = (void*)(regs->mainstor + raddr);
        tte->format = TRACE_F5_PR_FMT;
        tte->pswkey = regs->psw.pkey | TRACE_F5_PR_FM2;
        STORE_HW(tte->newpasn, newregs->CR_LHL(4));
        STORE_FW(tte->retna,  newregs->psw.IA_L | PROBSTATE(&newregs->psw));
        STORE_FW(tte->newia, regs->psw.IA_L);
    }
    else
    if(regs->psw.amode64 && regs->psw.IA_H != 0 && newregs->psw.amode64 && newregs->psw.IA_H == 0)
    {
        TRACE_F6_PR *tte;
        size = sizeof(TRACE_F6_PR);
        raddr = ARCH_DEP(get_trace_entry) (&ag, size, regs);
        tte = (void*)(regs->mainstor + raddr);
        tte->format = TRACE_F6_PR_FMT;
        tte->pswkey = regs->psw.pkey | TRACE_F6_PR_FM2;
        STORE_HW(tte->newpasn, newregs->CR_LHL(4));
        STORE_FW(tte->retna,  newregs->psw.IA_L | PROBSTATE(&newregs->psw));
        STORE_DW(tte->newia, regs->psw.IA_G);
    }
    else
    if(!regs->psw.amode64 && newregs->psw.amode64 && newregs->psw.IA_H != 0)
    {
        TRACE_F7_PR *tte;
        size = sizeof(TRACE_F7_PR);
        raddr = ARCH_DEP(get_trace_entry) (&ag, size, regs);
        tte = (void*)(regs->mainstor + raddr);
        tte->format = TRACE_F7_PR_FMT;
        tte->pswkey = regs->psw.pkey | TRACE_F7_PR_FM2;
        STORE_HW(tte->newpasn, newregs->CR_LHL(4));
        STORE_DW(tte->retna,  newregs->psw.IA_G | PROBSTATE(&newregs->psw));
        STORE_FW(tte->newia, (regs->psw.amode << 31)
                                 | regs->psw.IA_L);
    }
    else
    if(regs->psw.amode64 && regs->psw.IA_H == 0 && newregs->psw.amode64 && newregs->psw.IA_H != 0)
    {
        TRACE_F8_PR *tte;
        size = sizeof(TRACE_F8_PR);
        raddr = ARCH_DEP(get_trace_entry) (&ag, size, regs);
        tte = (void*)(regs->mainstor + raddr);
        tte->format = TRACE_F8_PR_FMT;
        tte->pswkey = regs->psw.pkey | TRACE_F8_PR_FM2;
        STORE_HW(tte->newpasn, newregs->CR_LHL(4));
        STORE_DW(tte->retna,  newregs->psw.IA_G | PROBSTATE(&newregs->psw));
        STORE_FW(tte->newia, regs->psw.IA_L);
    }
    else
    /* if(regs->psw.amode64 && regs->psw.IA_H != 0 && newregs->psw.amode64 && newregs->psw.IA_H != 0) */
    {
        TRACE_F9_PR *tte;
        size = sizeof(TRACE_F9_PR);
        raddr = ARCH_DEP(get_trace_entry) (&ag, size, regs);
        tte = (void*)(regs->mainstor + raddr);
        tte->format = TRACE_F9_PR_FMT;
        tte->pswkey = regs->psw.pkey | TRACE_F9_PR_FM2;
        STORE_HW(tte->newpasn, newregs->CR_LHL(4));
        STORE_DW(tte->retna,  newregs->psw.IA_G | PROBSTATE(&newregs->psw));
        STORE_DW(tte->newia, regs->psw.IA_G);
    }
#endif /*defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/

    return ARCH_DEP(set_trace_entry) (ag, raddr, size, regs);

} /* end function ARCH_DEP(trace_pr) */
#endif /*defined(FEATURE_LINKAGE_STACK)*/

#if defined(_MSVC_)
  /* Workaround for "fatal error C1001: INTERNAL COMPILER ERROR" in MSVC */
  #pragma optimize("",on)
#endif /*defined(_MSVC_)*/

/*-------------------------------------------------------------------*/
/* Form implicit PT/PTI trace entry                                  */
/*                                                                   */
/* Input:                                                            */
/*      pti     1=PTI instruction, 0=PT instruction                  */
/*      pasn    Primary address space number                         */
/*      gpr2    Contents of PT second operand register               */
/*      regs    Pointer to the CPU register context                  */
/* Return value:                                                     */
/*      Updated value for CR12 after adding new trace entry          */
/*                                                                   */
/*      This function does not return if a program check occurs.     */
/*-------------------------------------------------------------------*/
CREG ARCH_DEP(trace_pt) (int pti, U16 pasn, GREG gpr2, REGS *regs)
{
RADR raddr;
RADR ag;
int  size;
BYTE nbit = (pti ? 1 : 0);

#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
    if(regs->psw.amode64 && gpr2 > 0xFFFFFFFFULL)
    {
        TRACE_F3_PT *tte;
        size = sizeof(TRACE_F3_PT);
        raddr = ARCH_DEP(get_trace_entry) (&ag, size, regs);
        tte = (void*)(regs->mainstor + raddr);
        tte->format = TRACE_F3_PT_FMT;
        tte->pswkey = regs->psw.pkey | TRACE_F3_PT_FM2 | nbit;
        STORE_HW(tte->newpasn, pasn);
        STORE_DW(tte->r2, gpr2);
    }
    else
    if(regs->psw.amode64)
    {
        TRACE_F2_PT *tte;
        size = sizeof(TRACE_F2_PT);
        raddr = ARCH_DEP(get_trace_entry) (&ag, size, regs);
        tte = (void*)(regs->mainstor + raddr);
        tte->format = TRACE_F2_PT_FMT;
        tte->pswkey = regs->psw.pkey | TRACE_F2_PT_FM2 | nbit;
        STORE_HW(tte->newpasn, pasn);
        STORE_FW(tte->r2, gpr2 & 0xFFFFFFFF);
    }
    else
#endif /*defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/
    {
        TRACE_F1_PT *tte;
        size = sizeof(TRACE_F1_PT);
        raddr = ARCH_DEP(get_trace_entry) (&ag, size, regs);
        tte = (void*)(regs->mainstor + raddr);
        tte->format = TRACE_F1_PT_FMT;
        tte->pswkey = regs->psw.pkey | TRACE_F1_PT_FM2 | nbit;
        STORE_HW(tte->newpasn, pasn);
        STORE_FW(tte->r2, gpr2 & 0xFFFFFFFF);
    }

    return ARCH_DEP(set_trace_entry) (ag, raddr, size, regs);

} /* end function ARCH_DEP(trace_pt) */


#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
/*-------------------------------------------------------------------*/
/* Form implicit MS trace entry                                      */
/*                                                                   */
/* Input:                                                            */
/*      br      Mode switch branch indicator                         */
/*      baddr   Branch address for mode switch branch                */
/*      regs    Pointer to the CPU register context                  */
/* Return value:                                                     */
/*      Updated value for CR12 after adding new trace entry          */
/*                                                                   */
/*      This function does not return if a program check occurs.     */
/*-------------------------------------------------------------------*/
CREG ARCH_DEP(trace_ms) (int br, VADR baddr, REGS *regs)
{
RADR raddr;
RADR ag;
int  size;

    MAYBE_SET_PSW_IA_FROM_IP(regs);

    if(!br)
    {
        if(!regs->psw.amode64)
        {
            TRACE_F1_MS *tte;
            size = sizeof(TRACE_F1_MS);
            raddr = ARCH_DEP(get_trace_entry) (&ag, size, regs);
            tte = (void*)(regs->mainstor + raddr);
            tte->format = TRACE_F1_MS_FMT;
            tte->fmt2 = TRACE_F1_MS_FM2;
            STORE_HW(tte->resv, 0);
            STORE_FW(tte->newia, regs->psw.IA | (regs->psw.amode << 31));
        }
        else
        if(regs->psw.amode64 && regs->psw.IA <= 0x7FFFFFFF)
        {
            TRACE_F2_MS *tte;
            size = sizeof(TRACE_F2_MS);
            raddr = ARCH_DEP(get_trace_entry) (&ag, size, regs);
            tte = (void*)(regs->mainstor + raddr);
            tte->format = TRACE_F2_MS_FMT;
            tte->fmt2 = TRACE_F2_MS_FM2;
            STORE_HW(tte->resv, 0);
            STORE_FW(tte->newia, regs->psw.IA);
        }
        else
        {
            TRACE_F3_MS *tte;
            size = sizeof(TRACE_F3_MS);
            raddr = ARCH_DEP(get_trace_entry) (&ag, size, regs);
            tte = (void*)(regs->mainstor + raddr);
            tte->format = TRACE_F3_MS_FMT;
            tte->fmt2 = TRACE_F3_MS_FM2;
            STORE_HW(tte->resv, 0);
            STORE_DW(tte->newia, regs->psw.IA);
        }
    }
    else
    {
        /* if currently in 64-bit, we are switching out */
        if(regs->psw.amode64)
        {
            TRACE_F1_MSB *tte;
            size = sizeof(TRACE_F1_MSB);
            raddr = ARCH_DEP(get_trace_entry) (&ag, size, regs);
            tte = (void*)(regs->mainstor + raddr);
            tte->format = TRACE_F1_MSB_FMT;
            tte->fmt2 = TRACE_F1_MSB_FM2;
            STORE_HW(tte->resv, 0);
            STORE_FW(tte->newia, baddr);
        }
        else
        if(!regs->psw.amode64 && baddr <= 0x7FFFFFFF)
        {
            TRACE_F2_MSB *tte;
            size = sizeof(TRACE_F2_MSB);
            raddr = ARCH_DEP(get_trace_entry) (&ag, size, regs);
            tte = (void*)(regs->mainstor + raddr);
            tte->format = TRACE_F2_MSB_FMT;
            tte->fmt2 = TRACE_F2_MSB_FM2;
            STORE_HW(tte->resv, 0);
            STORE_FW(tte->newia, baddr);
        }
        else
        {
            TRACE_F3_MSB *tte;
            size = sizeof(TRACE_F3_MSB);
            raddr = ARCH_DEP(get_trace_entry) (&ag, size, regs);
            tte = (void*)(regs->mainstor + raddr);
            tte->format = TRACE_F3_MSB_FMT;
            tte->fmt2 = TRACE_F3_MSB_FM2;
            STORE_HW(tte->resv, 0);
            STORE_DW(tte->newia, baddr);
        }
    }

    return ARCH_DEP(set_trace_entry) (ag, raddr, size, regs);

} /* end function ARCH_DEP(trace_ms) */
#endif /*defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/


/*-------------------------------------------------------------------*/
/* Form explicit TRACE trace entry                                   */
/*                                                                   */
/* Input:                                                            */
/*      r1, r3  registers identifying register space to be written   */
/*      op      Trace operand                                        */
/*      regs    Pointer to the CPU register context                  */
/* Return value:                                                     */
/*      Updated value for CR12 after adding new trace entry          */
/*                                                                   */
/*      This function does not return if a program check occurs.     */
/*-------------------------------------------------------------------*/
CREG ARCH_DEP(trace_tr) (int r1, int r3, U32 op, REGS *regs)
{
RADR raddr;
RADR ag;
int  size;
int  i, j, n;
U64  dreg;
ETOD ETOD;

    {
        TRACE_F1_TR *tte;
        BYTE fast;

        size = sizeof(TRACE_F1_TR);
        raddr = ARCH_DEP(get_trace_entry) (&ag, size, regs);
        tte = (void*)(regs->mainstor + raddr);

        /* Determine if fast clock format */
#if defined(FEATURE_025_STORE_CLOCK_FAST_FACILITY)
        if (FACILITY_ENABLED( 025_STORE_CLOCK_FAST, regs ))
            fast = (regs->CR(0) & CR0_TRACE_TOD) >> 31;
        else
#endif
            fast = 0;

        /* Calculate the number of registers to be traced, minus 1 */
        n = ( r3 < r1 ) ? r3 + 16 - r1 : r3 - r1;

        /* Retrieve the TOD clock value bits 16-63 */
        etod_clock(regs, &ETOD, fast ? ETOD_fast : ETOD_standard);
        dreg = ETOD2TOD(ETOD) & 0x0000FFFFFFFFFFFFULL;

        /* Set format information */
        dreg |= /* tte->format = */ ((U64)(TRACE_F1_TR_FMT | n)) << 56;
        dreg |= /* tte->fmt2 =   */  (U64)TRACE_F1_TR_FM2        << 48;

        /* Store format information and bits 16-63 of the TOD clock
         * value
         */
        STORE_DW(tte, dreg);

        /* Store trace operand */
#if defined(FEATURE_025_STORE_CLOCK_FAST_FACILITY)
        if (FACILITY_ENABLED( 025_STORE_CLOCK_FAST, regs ))
        {
            if (fast)
            {
                op &= 0xFF00FFFF;
                /* FIXME: OR-in model dependent code
                op |= model_code << 16;
                 */
            }
        }
#endif
        STORE_FW(tte->operand, op);

        for(i = r1, j = 0; ; )
        {
            STORE_FW(tte->reg[j++], regs->GR_L(i));

            /* Regdump is complete when r3 is done */
            if(r3 == i) break;

            /* Update register number and wrap */
            i++; i &= 15;
        }

    }

    return ARCH_DEP(set_trace_entry) (ag, raddr, size - (4 * (15 - n)), regs);

} /* end function ARCH_DEP(trace_tr) */


#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
/*-------------------------------------------------------------------*/
/* Form explicit TRACG trace entry                                   */
/*                                                                   */
/* Input:                                                            */
/*      r1, r3  registers identifying register space to be written   */
/*      op      Trace operand                                        */
/*      regs    Pointer to the CPU register context                  */
/* Return value:                                                     */
/*      Updated value for CR12 after adding new trace entry          */
/*                                                                   */
/*      This function does not return if a program check occurs.     */
/*-------------------------------------------------------------------*/
CREG ARCH_DEP(trace_tg) (int r1, int r3, U32 op, REGS *regs)
{
RADR raddr;
RADR ag;
int  size;
int  i, j, n;
ETOD ETOD;
U64  dreg;

    {
        TRACE_F2_TR *tte;
        size = sizeof(TRACE_F2_TR);
        raddr = ARCH_DEP(get_trace_entry) (&ag, size, regs);
        tte = (void*)(regs->mainstor + raddr);

        /* Calculate the number of registers to be traced, minus 1 */
        n = ( r3 < r1 ) ? r3 + 16 - r1 : r3 - r1;

        /* Retrieve the extended TOD clock value including the epoch */
        etod_clock(regs, &ETOD, ETOD_extended);

        /* Convert clock value to 80-bit trace format; clean and leave
         * TODEX bit 7 along with TOD clock bits 0-79
         */
        ETOD_shift(&ETOD, ETOD, 8);
        dreg = ETOD.high & 0x0001FFFFFFFFFFFFULL;       /* Clean up */

        /* Set format information */
        dreg |= /* tte->format = */ ((U64)(TRACE_F2_TR_FMT | n)) << 56;
        dreg |= /* tte->fmt2 =   */  (U64)TRACE_F2_TR_FM2        << 48;

        /* Store format information and bits 0-79 of the TOD clock
         * value
         */
        STORE_DW(tte, dreg);
        STORE_FW(tte->clk48, ETOD.low >> 32);

        /* Store trace operand */
#if defined(FEATURE_025_STORE_CLOCK_FAST_FACILITY)
        if (FACILITY_ENABLED( 025_STORE_CLOCK_FAST, regs ))
        {
            if ((regs->CR(0) & CR0_TRACE_TOD))
            {
                op &= 0xFF00FFFF;
                /* FIXME: OR-in model dependent code
                op |= model_code << 16;
                 */
            }
        }
#endif
        STORE_FW(tte->operand, op);

        for (i = r1, j = 0; ; )
        {
            STORE_DW(tte->reg[j++], regs->GR_G(i));

            /* Regdump is complete when r3 is done */
            if(r3 == i) break;

            /* Update register number and wrap */
            i++; i &= 15;
        }

    }

    return ARCH_DEP(set_trace_entry) (ag, raddr, size - (8 * (15 - n)), regs);

} /* end function ARCH_DEP(trace_tg) */
#endif /*defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/

#endif /*defined(FEATURE_TRACING)*/


#if !defined(_GEN_ARCH)

#if defined(_ARCH_NUM_1)
 #define  _GEN_ARCH _ARCH_NUM_1
 #include "trace.c"
#endif

#if defined(_ARCH_NUM_2)
 #undef   _GEN_ARCH
 #define  _GEN_ARCH _ARCH_NUM_2
 #include "trace.c"
#endif

#endif /*!defined(_GEN_ARCH)*/
