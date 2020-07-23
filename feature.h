/* FEATURE.H    (C) Copyright Jan Jaeger, 2000-2012                  */
/*              Architecture-dependent macro definitions             */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#ifdef HAVE_CONFIG_H
  #ifndef    _CONFIG_H
  #define    _CONFIG_H
    #include <config.h>         /* Hercules build configuration      */
  #endif /*  _CONFIG_H*/
#endif

/*-------------------------------------------------------------------*/
/*  The below section of code causes the 'featchk.h' header file     */
/*  to be #included multiple times, each time accomplishing some-    */
/*  thing different.  The first time it simply #defines _FEATUREs    */
/*  (with a leading underscore) depending on whether ANY of the      */
/*  build architectures #define that feature.  The second time it    */
/*  checks the sanity of FEATURE combinations, i.e. if feature X     */
/*  is defined then feature Y must also be defined, etc.  Refer to   */
/*  the actual 'featchk.h' header file itself for details.           */
/*-------------------------------------------------------------------*/

#if !defined( DID_FEATCHK_PASS_1 )      // (did we do pass 1 yet?)

  #define     FEATCHK_DO_DEFINES        // (if not then do it now)

  #include  "featall.h"                 // (#undef *ALL* FEATUREs)
  #include  "feat370.h"                 // (#define 370 FEATUREs)
  #include  "feat390.h"                 // (#define 390 FEATUREs)
  #include  "feat900.h"                 // (#define 900 FEATUREs)
  #include  "featchk.h"                 // (featchk pass 1: defines)

  #undef  FEATCHK_DO_DEFINES            // (enable featchk pass 2)
  #define DID_FEATCHK_PASS_1            // (don't do pass 1 twice)

  #if defined( _FEATURE_2K_STORAGE_KEYS )
    #define _STORKEY_ARRAY_UNITSIZE   2048
  #else
    #define _STORKEY_ARRAY_UNITSIZE   4096
  #endif

#endif /* !defined( DID_FEATCHK_PASS_1 ) */

/*-------------------------------------------------------------------*/
/*  Now #define the FEATUREs for the current build architecture(s)   */
/*-------------------------------------------------------------------*/

#undef       __GEN_ARCH                   // (TWO underscores!)
#if !defined( _GEN_ARCH )                 // (ONE underscore!)
 #define     __GEN_ARCH    _ARCH_NUM_0    // (first build architecture)
#else
 #define     __GEN_ARCH    _GEN_ARCH      // (next build architecture)
#endif

#include  "featall.h"                     // (all FEATUREs #undef'ed here)

#if   __GEN_ARCH == 370                   // (building for S/370?)
 #include      "feat370.h"                // (#define S/370 FEATUREs)

#elif __GEN_ARCH == 390                   // (building for S/390?)
 #include      "feat390.h"                // (#define S/390 FEATUREs)

#elif __GEN_ARCH == 900                   // (building for z/Arch?)
 #include      "feat900.h"                // (#define z/Arch FEATUREs)

#else
 #error Unable to determine Architecture Mode
#endif

#include  "featchk.h"                   // (featchk pass 2: sanity checks)

/*-------------------------------------------------------------------*/
/*  The following are the various constants and macros which vary    */
/*  by build architecture.  When __GEN_ARCH == 370, #defines for     */
/*  the S/370 architecture are made, etc.  Constants and macros      */
/*  common for all build architectures follow much further below,    */
/*  past this section of code.                                       */
/*-------------------------------------------------------------------*/

#undef ARCH_IDX
#undef APPLY_PREFIXING
#undef AMASK
#undef ADDRESS_MAXWRAP
#undef ADDRESS_MAXWRAP_E
#undef REAL_MODE
#undef PER_MODE
#undef ASF_ENABLED
#undef ASN_AND_LX_REUSE_ENABLED
#undef ASTE_AS_DESIGNATOR
#undef ASTE_LT_DESIGNATOR
#undef SAEVENT_BIT
#undef SSEVENT_BIT
#undef SSGROUP_BIT
#undef LSED_UET_HDR
#undef LSED_UET_TLR
#undef LSED_UET_BAKR
#undef LSED_UET_PC
#undef CR12_BRTRACE
#undef CR12_TRACEEA
#undef CHM_GPR2_RESV
#undef DEF_INST
#undef ARCH_DEP
#undef PSA
#undef PSA_SIZE
#undef IA
#undef PX
#undef CR
#undef GR
#undef GR_A
#undef SET_GR_A
#undef MONCODE
#undef TEA
#undef DXC
#undef ET
#undef PX_MASK
#undef RSTOLD
#undef RSTNEW
#undef RADR
#undef F_RADR
#undef VADR
#undef VADR_L
#undef F_VADR
#undef GREG
#undef F_GREG
#undef CREG
#undef F_CREG
#undef AREG
#undef F_AREG
#undef STORE_W
#undef FETCH_W
#undef AIV
#undef AIE
#undef VIE
#undef SIEBK
#undef ZPB
#undef TLB_REAL_ASD
#undef TLB_ASD
#undef TLB_VADDR
#undef TLB_PTE
#undef TLB_PAGEMASK
#undef TLB_BYTEMASK
#undef TLB_PAGESHIFT
#undef TLBID_PAGEMASK
#undef TLBID_BYTEMASK
#undef ASD_PRIVATE
#undef PER_SB
#undef CHANNEL_MASKS

/*----------------------------------------------------------------------------*/
#if __GEN_ARCH == 370
/*----------------------------------------------------------------------------*/

#define ARCH_IDX            ARCH_370_IDX
#define ARCH_DEP(_name)     s370_ ## _name

#define APPLY_PREFIXING(addr,pfx) \
    ( ((U32)(addr) & 0x7FFFF000) == 0 || ((U32)(addr) & 0x7FFFF000) == (pfx) \
      ? (U32)(addr) ^ (pfx) \
      : (addr) \
    )

#define AMASK   AMASK_L

#define ADDRESS_MAXWRAP(_register_context) \
    (AMASK24)

#define ADDRESS_MAXWRAP_E(_register_context) \
    (AMASK31)

#define REAL_MODE(p) \
    (!ECMODE(p) || ((p)->sysmask & PSW_DATMODE)==0)

#if defined(_FEATURE_SIE)
#define PER_MODE(_regs) \
        ( (ECMODE(&(_regs)->psw) && ((_regs)->psw.sysmask & PSW_PERMODE)) \
          || (SIE_MODE((_regs)) && ((_regs)->siebk->m & SIE_M_GPE)) )
#else
#define PER_MODE(_regs) \
        (ECMODE(&(_regs)->psw) && ((_regs)->psw.sysmask & PSW_PERMODE))
#endif

#define ASF_ENABLED(_regs)  0 /* ASF is never enabled for S/370 */

#define ASN_AND_LX_REUSE_ENABLED(_regs) 0 /* never enabled for S/370 */

#define ASTE_AS_DESIGNATOR(_aste) \
    ((_aste)[2])

#define ASTE_LT_DESIGNATOR(_aste) \
    ((_aste)[3])

#define SAEVENT_BIT STD_SAEVENT
#define SSEVENT_BIT STD_SSEVENT
#define SSGROUP_BIT STD_GROUP

#define PSA PSA_3XX
#define PSA_SIZE 4096
#define IA  IA_L
#define PX  PX_L
#define CR(_r)  CR_L(_r)
#define GR(_r)  GR_L(_r)
#define GR_A(_r, _regs) ((_regs)->GR_L((_r)))
#define SET_GR_A(_r, _regs,_v) ((_regs)->GR_L((_r))=(_v))
#define MONCODE MC_L
#define TEA EA_L
#define DXC     tea
#define ET  ET_L
#define PX_MASK 0x7FFFF000
#define RSTOLD  iplccw1
#define RSTNEW  iplpsw
#if !defined(_FEATURE_ZSIE)
#define RADR    U32
#define F_RADR  "%8.8"PRIX32
#else
#define RADR    U64
#define F_RADR  "%16.16"PRIX64
#endif
#define VADR    U32
#define VADR_L  VADR
#define F_VADR  "%8.8"PRIX32
#define GREG    U32
#define F_GREG  "%8.8"PRIX32
#define CREG    U32
#define F_CREG  "%8.8"PRIX32
#define AREG    U32
#define F_AREG  "%8.8"PRIX32
#define STORE_W STORE_FW
#define FETCH_W FETCH_FW
#define AIV     AIV_L
#define AIE     AIE_L
#define SIEBK                   SIE1BK
#define ZPB                     ZPB1
#define TLB_REAL_ASD  TLB_REAL_ASD_L
#define TLB_ASD(_n)   TLB_ASD_L(_n)
#define TLB_VADDR(_n) TLB_VADDR_L(_n)
#define TLB_PTE(_n)   TLB_PTE_L(_n)
#define TLB_PAGEMASK  0x00FFF800
#define TLB_BYTEMASK  0x000007FF
#define TLB_PAGESHIFT 11
#define TLBID_PAGEMASK  0x00E00000
#define TLBID_BYTEMASK  0x001FFFFF
#define ASD_PRIVATE   SEGTAB_370_CMN
#define CHANNEL_MASKS(_regs) ((_regs)->CR(2))

// 370

#undef STORAGE_KEY_PAGESHIFT
#undef STORAGE_KEY_PAGESIZE
#undef STORAGE_KEY_BYTEMASK
#undef STORAGE_KEY_PAGEMASK
#undef STORAGE_KEY

#if defined(FEATURE_2K_STORAGE_KEYS)

  #define STORAGE_KEY_PAGESHIFT     11
  #define STORAGE_KEY_PAGESIZE      2048
  #define STORAGE_KEY_BYTEMASK      0x000007FF
  #define STORAGE_KEY_PAGEMASK      0x7FFFF800

#else /* FEATURE_4K_STORAGE_KEYS */

  #define STORAGE_KEY_PAGESHIFT     12
  #define STORAGE_KEY_PAGESIZE      4096
  #define STORAGE_KEY_BYTEMASK      0x00000FFF
  #define STORAGE_KEY_PAGEMASK      0x7FFFF000

#endif

#define STORAGE_KEY(_addr, _pointer) \
   (_pointer)->storkeys[(_addr)>>STORAGE_KEY_PAGESHIFT]

/*----------------------------------------------------------------------------*/
#elif __GEN_ARCH == 390
/*----------------------------------------------------------------------------*/

#define ARCH_IDX            ARCH_390_IDX
#define ARCH_DEP(_name)     s390_ ## _name

#define APPLY_PREFIXING(addr,pfx) \
    ( ((U32)(addr) & 0x7FFFF000) == 0 || ((U32)(addr) & 0x7FFFF000) == (pfx) \
      ? (U32)(addr) ^ (pfx) \
      : (addr) \
    )

#define AMASK   AMASK_L

#define ADDRESS_MAXWRAP(_register_context) \
    ((_register_context)->psw.AMASK)

#define ADDRESS_MAXWRAP_E(_register_context) \
    ((_register_context)->psw.AMASK)

#define REAL_MODE(p) \
    (((p)->sysmask & PSW_DATMODE)==0)

#if defined(_FEATURE_SIE)
#define PER_MODE(_regs) \
        ( ((_regs)->psw.sysmask & PSW_PERMODE) \
          || (SIE_MODE((_regs)) && ((_regs)->siebk->m & SIE_M_GPE)) )
#else
#define PER_MODE(_regs) \
        ((_regs)->psw.sysmask & PSW_PERMODE)
#endif

#define ASF_ENABLED(_regs)  ((_regs)->CR(0) & CR0_ASF)

#define ASN_AND_LX_REUSE_ENABLED(_regs) 0 /* never enabled in ESA/390 */

#define ASTE_AS_DESIGNATOR(_aste) \
    ((_aste)[2])

#define ASTE_LT_DESIGNATOR(_aste) \
    ((_aste)[3])

#define SAEVENT_BIT STD_SAEVENT
#define SSEVENT_BIT STD_SSEVENT
#define SSGROUP_BIT STD_GROUP

#define LSED_UET_HDR    S_LSED_UET_HDR
#define LSED_UET_TLR    S_LSED_UET_TLR
#define LSED_UET_BAKR   S_LSED_UET_BAKR
#define LSED_UET_PC S_LSED_UET_PC
#define CR12_BRTRACE    S_CR12_BRTRACE
#define CR12_TRACEEA    S_CR12_TRACEEA

#define CHM_GPR2_RESV   S_CHM_GPR2_RESV

#define PSA PSA_3XX
#define PSA_SIZE 4096
#define IA  IA_L
#define PX  PX_L
#define CR(_r)  CR_L(_r)
#define GR(_r)  GR_L(_r)
#define GR_A(_r, _regs) ((_regs)->GR_L((_r)))
#define SET_GR_A(_r, _regs,_v) ((_regs)->GR_L((_r))=(_v))
#define MONCODE MC_L
#define TEA EA_L
#define DXC     tea
#define ET  ET_L
#define PX_MASK 0x7FFFF000
#define RSTNEW  iplpsw
#define RSTOLD  iplccw1
#if !defined(_FEATURE_ZSIE)
#define RADR    U32
#define F_RADR  "%8.8"PRIX32
#else
#define RADR    U64
#define F_RADR  "%16.16"PRIX64
#endif
#define VADR    U32
#define VADR_L  VADR
#define F_VADR  "%8.8"PRIX32
#define GREG    U32
#define F_GREG  "%8.8"PRIX32
#define CREG    U32
#define F_CREG  "%8.8"PRIX32
#define AREG    U32
#define F_AREG  "%8.8"PRIX32
#define STORE_W STORE_FW
#define FETCH_W FETCH_FW
#define AIV     AIV_L
#define AIE     AIE_L
#define SIEBK                   SIE1BK
#define ZPB                     ZPB1
#define TLB_REAL_ASD  TLB_REAL_ASD_L
#define TLB_ASD(_n)   TLB_ASD_L(_n)
#define TLB_VADDR(_n) TLB_VADDR_L(_n)
#define TLB_PTE(_n)   TLB_PTE_L(_n)
#define TLB_PAGEMASK  0x7FFFF000
#define TLB_BYTEMASK  0x00000FFF
#define TLB_PAGESHIFT 12
#define TLBID_PAGEMASK  0x7FC00000
#define TLBID_BYTEMASK  0x003FFFFF
#define ASD_PRIVATE   STD_PRIVATE
#ifdef FEATURE_ACCESS_REGISTERS
 #define CHANNEL_MASKS(_regs) 0xFFFFFFFF
#else
 #define CHANNEL_MASKS(_regs) ((_regs)->CR(2))
#endif /* FEATURE_ACCESS_REGISTERS */

// 390

#undef STORAGE_KEY_PAGESHIFT
#undef STORAGE_KEY_PAGESIZE
#undef STORAGE_KEY_BYTEMASK
#undef STORAGE_KEY_PAGEMASK
#undef STORAGE_KEY

#if defined(FEATURE_2K_STORAGE_KEYS)
  #error FEATURE_2K_STORAGE_KEYS unsupported for __GEN_ARCH == 390!
#else /* FEATURE_4K_STORAGE_KEYS */

  #define STORAGE_KEY_PAGESHIFT     12
  #define STORAGE_KEY_PAGESIZE      4096
  #define STORAGE_KEY_BYTEMASK      0x00000FFF
  #if !defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
    #define STORAGE_KEY_PAGEMASK    0x7FFFF000
  #else
    #define STORAGE_KEY_PAGEMASK    0xFFFFFFFFFFFFF000ULL
  #endif

#endif

#define STORAGE_KEY(_addr, _pointer) \
   (_pointer)->storkeys[(_addr)>>STORAGE_KEY_PAGESHIFT]

/*----------------------------------------------------------------------------*/
#elif __GEN_ARCH == 900
/*----------------------------------------------------------------------------*/

#define ARCH_IDX            ARCH_900_IDX
#define ARCH_DEP(_name)     z900_ ## _name

#define APPLY_PREFIXING(addr,pfx) \
    ( (U64)((addr) & 0xFFFFFFFFFFFFE000ULL) == (U64)0 || (U64)((addr) & 0xFFFFFFFFFFFFE000ULL) == (pfx) \
      ? (addr) ^ (pfx) \
      : (addr) \
    )

#define AMASK   AMASK_G

#define ADDRESS_MAXWRAP(_register_context) \
    ((_register_context)->psw.AMASK)

#define ADDRESS_MAXWRAP_E(_register_context) \
    ((_register_context)->psw.AMASK)

#define REAL_MODE(p) \
    (((p)->sysmask & PSW_DATMODE)==0)

#if defined(_FEATURE_SIE)
#define PER_MODE(_regs) \
        ( ((_regs)->psw.sysmask & PSW_PERMODE) \
          || (SIE_MODE((_regs)) && ((_regs)->siebk->m & SIE_M_GPE)) )
#else
#define PER_MODE(_regs) \
        ((_regs)->psw.sysmask & PSW_PERMODE)
#endif

#define ASF_ENABLED(_regs)  1 /* ASF is always enabled for ESAME */

/* ASN-and-LX-reuse is enabled if the ASN-and-LX-reuse
   facility is installed and CR0 bit 44 is 1 */
#if defined(FEATURE_006_ASN_LX_REUSE_FACILITY)
  #define ASN_AND_LX_REUSE_ENABLED(_regs) \
      (FACILITY_ENABLED( 006_ASN_LX_REUSE, (_regs) ) && ((_regs)->CR_L(0) & CR0_ASN_LX_REUS))
#else /* !defined(FEATURE_006_ASN_LX_REUSE_FACILITY) */
  #define ASN_AND_LX_REUSE_ENABLED(_regs) 0
#endif /* !defined(FEATURE_006_ASN_LX_REUSE_FACILITY) */

#define ASTE_AS_DESIGNATOR(_aste) \
    (((U64)((_aste)[2])<<32)|(U64)((_aste)[3]))

#define ASTE_LT_DESIGNATOR(_aste) \
    ((_aste)[6])

#define SAEVENT_BIT ASCE_S
#define SSEVENT_BIT ASCE_X
#define SSGROUP_BIT ASCE_G

#define LSED_UET_HDR    Z_LSED_UET_HDR
#define LSED_UET_TLR    Z_LSED_UET_TLR
#define LSED_UET_BAKR   Z_LSED_UET_BAKR
#define LSED_UET_PC Z_LSED_UET_PC
#define CR12_BRTRACE    Z_CR12_BRTRACE
#define CR12_TRACEEA    Z_CR12_TRACEEA

#define CHM_GPR2_RESV   Z_CHM_GPR2_RESV

#define PSA PSA_900
#define PSA_SIZE 8192
#define IA  IA_G
#define PX  PX_G
#define CR(_r)  CR_G(_r)
#define GR(_r)  GR_G(_r)
#define GR_A(_r, _regs) ((_regs)->psw.amode64 ? (_regs)->GR_G((_r)) : (_regs)->GR_L((_r)))
#define SET_GR_A(_r, _regs,_v)  \
    do  { \
        if((_regs)->psw.amode64) { \
            ((_regs)->GR_G((_r))=(_v)); \
        } else { \
            ((_regs)->GR_L((_r))=(_v)); \
        } \
    } while(0)

#define MONCODE MC_G
#define TEA EA_G
#define DXC     dataexc
#define ET  ET_G
#define PX_MASK 0x7FFFE000
#define RSTOLD  rstold
#define RSTNEW  rstnew
#define RADR    U64
#define F_RADR  "%16.16"PRIX64
#define VADR    U64
#if SIZEOF_INT == 4
#define VADR_L  U32
#else
#define VADR_L  VADR
#endif
#define F_VADR  "%16.16"PRIX64
#define GREG    U64
#define F_GREG  "%16.16"PRIX64
#define CREG    U64
#define F_CREG  "%16.16"PRIX64
#define AREG    U32
#define F_AREG  "%8.8"PRIX32
#define STORE_W STORE_DW
#define FETCH_W FETCH_DW
#define AIV     AIV_G
#define AIE     AIE_G
#define SIEBK                   SIE2BK
#define ZPB                     ZPB2
#define TLB_REAL_ASD  TLB_REAL_ASD_G
#define TLB_ASD(_n)   TLB_ASD_G(_n)
#define TLB_VADDR(_n) TLB_VADDR_G(_n)
#define TLB_PTE(_n)   TLB_PTE_G(_n)
#define TLB_PAGEMASK  0xFFFFFFFFFFFFF000ULL
#define TLB_BYTEMASK  0x0000000000000FFFULL
#define TLB_PAGESHIFT 12
#define TLBID_PAGEMASK  0xFFFFFFFFFFC00000ULL
#define TLBID_BYTEMASK  0x00000000003FFFFFULL
#define ASD_PRIVATE   (ASCE_P|ASCE_R)
#ifdef FEATURE_ACCESS_REGISTERS
 #define CHANNEL_MASKS(_regs) 0xFFFFFFFF
#else
 #define CHANNEL_MASKS(_regs) ((_regs) -> CR(2))
#endif /* FEATURE_ACCESS_REGISTERS */

// 900

#undef STORAGE_KEY_PAGESHIFT
#undef STORAGE_KEY_PAGESIZE
#undef STORAGE_KEY_BYTEMASK
#undef STORAGE_KEY_PAGEMASK
#undef STORAGE_KEY

#if defined(FEATURE_2K_STORAGE_KEYS)
  #error FEATURE_2K_STORAGE_KEYS unsupported for __GEN_ARCH == 900!
#else /* FEATURE_4K_STORAGE_KEYS */

  #define STORAGE_KEY_PAGESHIFT     12
  #define STORAGE_KEY_PAGESIZE      4096
  #define STORAGE_KEY_BYTEMASK      0x00000FFF
  #if !defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
    #define STORAGE_KEY_PAGEMASK    0x7FFFF000
  #else
    #define STORAGE_KEY_PAGEMASK    0xFFFFFFFFFFFFF000ULL
  #endif

#endif

#define STORAGE_KEY(_addr, _pointer) \
   (_pointer)->storkeys[(_addr)>>STORAGE_KEY_PAGESHIFT]

/*----------------------------------------------------------------------------*/
#else // __GEN_ARCH != 370 or 390 or 900...
/*----------------------------------------------------------------------------*/

  WARNING( "__GEN_ARCH must be 370, 390, 900 or undefined" )

#endif

#define DEF_INST(_name)     \
    void ( ATTR_REGPARM(2) ARCH_DEP( _name ) )( BYTE inst[], REGS* regs )

/*-------------------------------------------------------------------*/
/*  The following section of code defines constants and macros that  */
/*  are not generally architecture dependent.  That is, the below    */
/*  are #defines common for all build architectures (or independent  */
/*  of a given build architecture).                                  */
/*-------------------------------------------------------------------*/

#undef STORAGE_KEY1
#undef STORAGE_KEY2

#if defined(_FEATURE_2K_STORAGE_KEYS)
  #define STORAGE_KEY1(_addr, _pointer) \
    (_pointer)->storkeys[((_addr)>>STORAGE_KEY_PAGESHIFT)&~1]
  #define STORAGE_KEY2(_addr, _pointer) \
    (_pointer)->storkeys[((_addr)>>STORAGE_KEY_PAGESHIFT)|1]
#endif

/*-------------------------------------------------------------------*/
/* PAGEFRAME-size related constants                                  */
/*-------------------------------------------------------------------*/

// FIXME: Why do we have separate STORAGE_KEY_xxx and PAGEFRAME_xxx constants?
// FIXME: Choose one or the other. Inconsistent usage is confusing and dangerous.

#undef PAGEFRAME_PAGESIZE
#undef PAGEFRAME_PAGESHIFT
#undef PAGEFRAME_BYTEMASK
#undef PAGEFRAME_PAGEMASK
#undef MAXADDRESS

#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
  #define PAGEFRAME_PAGESIZE        4096
  #define PAGEFRAME_PAGESHIFT       12
  #define PAGEFRAME_BYTEMASK        0x00000FFF
  #define PAGEFRAME_PAGEMASK        0xFFFFFFFFFFFFF000ULL
  #define MAXADDRESS                0xFFFFFFFFFFFFFFFFULL
#elif defined( FEATURE_S390_DAT )
  #define PAGEFRAME_PAGESIZE        4096
  #define PAGEFRAME_PAGESHIFT       12
  #define PAGEFRAME_BYTEMASK        0x00000FFF
  #define PAGEFRAME_PAGEMASK        0x7FFFF000
  #define MAXADDRESS                0x7FFFFFFF
#else // 370
  #define PAGEFRAME_PAGESIZE        2048
  #define PAGEFRAME_PAGESHIFT       11
  #define PAGEFRAME_BYTEMASK        0x000007FF
  #define PAGEFRAME_PAGEMASK        0x7FFFF800
  #if defined(FEATURE_370E_EXTENDED_ADDRESSING)
    #define MAXADDRESS              0x03FFFFFF
  #else
    #define MAXADDRESS              0x00FFFFFF
  #endif
#endif

/*-------------------------------------------------------------------*/
/*              Operand Length Checking Macros                       */
/*                                                                   */
/* The following macros are used to determine whether an operand     */
/* storage access will cross a page boundary or not.                 */
/*                                                                   */
/* The first 'plain' pair of macros (without the 'L') are used for   */
/* 0-based lengths wherein zero = 1 byte is being referenced and 255 */
/* means 256 bytes are being referenced. They are obviously designed */
/* for maximum length values of 0-255 as used w/MVC instructions.    */
/*                                                                   */
/* The second pair of 'L' macros are using for 1-based lengths where */
/* 0 = no bytes are being referenced, 1 = one byte, etc. They are    */
/* designed for 'Large' maximum length values such as occur with the */
/* MVCL instruction for example (where the length can be over 256).  */
/* NOTE: The 'L' macros are limited to a maximum length of one page! */
/*-------------------------------------------------------------------*/

#undef NOCROSSPAGE
#undef   CROSSPAGE
#undef NOCROSSPAGEL
#undef   CROSSPAGEL

#define NOCROSSPAGE(  addr, len )    likely( ((int)((addr) & PAGEFRAME_BYTEMASK)) <= ((int)(PAGEFRAME_BYTEMASK - (len))) )
#define   CROSSPAGE(  addr, len )  unlikely( ((int)((addr) & PAGEFRAME_BYTEMASK)) >  ((int)(PAGEFRAME_BYTEMASK - (len))) )

#define NOCROSSPAGEL( addr, len )    likely( ((int)((addr) & PAGEFRAME_BYTEMASK)) <= ((int)(PAGEFRAME_PAGESIZE - (len))) )
#define   CROSSPAGEL( addr, len )  unlikely( ((int)((addr) & PAGEFRAME_BYTEMASK)) >  ((int)(PAGEFRAME_PAGESIZE - (len))) )

/*-------------------------------------------------------------------*/
/* EXPANDED STORAGE page-size related constants                      */
/*-------------------------------------------------------------------*/

#define XSTORE_INCREMENT_SIZE       0x00100000
#define XSTORE_PAGESHIFT            12
#define XSTORE_PAGESIZE             4096
#undef  XSTORE_PAGEMASK
#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY) || defined(_FEATURE_ZSIE)
  #define XSTORE_PAGEMASK           0xFFFFFFFFFFFFF000ULL
#else
  #define XSTORE_PAGEMASK           0x7FFFF000
#endif

/*-------------------------------------------------------------------*/
/* Interval Timer update and synchronization macros                  */
/*-------------------------------------------------------------------*/

#undef ITIMER_UPDATE
#undef ITIMER_SYNC

#if defined(FEATURE_INTERVAL_TIMER)
  #define ITIMER_UPDATE(_addr, _len, _regs)      \
    do {                                         \
    if( ITIMER_ACCESS((_addr), (_len)) )         \
            ARCH_DEP(fetch_int_timer) ((_regs)); \
    } while(0)
  #define ITIMER_SYNC(_addr, _len, _regs)        \
    do {                                         \
        if( ITIMER_ACCESS((_addr), (_len)) )     \
        ARCH_DEP(store_int_timer) ((_regs));     \
    } while (0)
#else
  #define ITIMER_UPDATE(_addr, _len, _regs)
  #define ITIMER_SYNC(_addr, _len, _regs)
#endif

/*-------------------------------------------------------------------*/
/* Macros use by Compare and Form Codeword (CFC (B21A)) instruction  */
/*-------------------------------------------------------------------*/

#undef   CFC_A64_OPSIZE
#undef   CFC_DEF_OPSIZE
#undef   CFC_MAX_OPSIZE
#undef   CFC_OPSIZE
#undef   CFC_GR2_SHIFT
#undef   CFC_HIGH_BIT
#undef   AR1
#define  AR1               ( 1 )        /* Access Register 1         */
#define  CFC_A64_OPSIZE    ( 6 )        /* amode-64 operand size     */
#define  CFC_DEF_OPSIZE    ( 2 )        /* non-amode-64 operand size */
#define  CFC_MAX_OPSIZE    ( CFC_A64_OPSIZE > CFC_DEF_OPSIZE ? CFC_A64_OPSIZE : CFC_DEF_OPSIZE )
#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
  #define  CFC_OPSIZE      ( a64 ?   CFC_A64_OPSIZE       :   CFC_DEF_OPSIZE       )
  #define  CFC_GR2_SHIFT   ( a64 ? ( CFC_A64_OPSIZE * 8 ) : ( CFC_DEF_OPSIZE * 8 ) )
  #define  CFC_HIGH_BIT    ( a64 ?  0x8000000000000000ULL :  0x0000000080000000ULL )
#else
  #define  CFC_OPSIZE      ( CFC_DEF_OPSIZE     )
  #define  CFC_GR2_SHIFT   ( CFC_DEF_OPSIZE * 8 )
  #define  CFC_HIGH_BIT    (  0x80000000UL      )
#endif

/*-------------------------------------------------------------------*/
/* Macros use by Update Tree (CFC (0102)) instruction                */
/*-------------------------------------------------------------------*/
#undef   UPT_ALIGN_MASK
#undef   UPT_SHIFT_MASK
#undef   UPT_HIGH_BIT
#undef   AR4
#define  AR4  (4)                       /* Access Register 4         */
#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
  #define  UPT_ALIGN_MASK   ( a64 ? 0x000000000000000FULL : 0x0000000000000007ULL )
  #define  UPT_SHIFT_MASK   ( a64 ? 0xFFFFFFFFFFFFFFF0ULL : 0xFFFFFFFFFFFFFFF8ULL )
  #define  UPT_HIGH_BIT     ( a64 ? 0x8000000000000000ULL : 0x0000000080000000ULL )
#else
  #define  UPT_ALIGN_MASK   ( 0x00000007 )
  #define  UPT_SHIFT_MASK   ( 0xFFFFFFF8 )
  #define  UPT_HIGH_BIT     ( 0x80000000 )
#endif

/* Macros for accelerated lookup */
#undef SPACE_BIT
#undef AR_BIT
#undef PRIMARY_SPACE_MODE
#undef SECONDARY_SPACE_MODE
#undef ACCESS_REGISTER_MODE
#undef HOME_SPACE_MODE
#undef AEA_MODE
#undef SET_AEA_COMMON
#undef SET_AEA_MODE
#undef _CASE_AR_SET_AEA_MODE
#undef _CASE_DAS_SET_AEA_MODE
#undef _CASE_HOME_SET_AEA_MODE
#undef TEST_SET_AEA_MODE
#undef SET_AEA_AR
#undef MADDR

#if defined(FEATURE_DUAL_ADDRESS_SPACE) && defined(FEATURE_LINKAGE_STACK)
#define SET_AEA_COMMON(_regs) \
do { \
    (_regs)->AEA_COMMON(1)  = ((_regs)->CR(1)  & ASD_PRIVATE) == 0; \
    (_regs)->AEA_COMMON(7)  = ((_regs)->CR(7)  & ASD_PRIVATE) == 0; \
    (_regs)->AEA_COMMON(13) = ((_regs)->CR(13) & ASD_PRIVATE) == 0; \
} while (0)
#elif defined(FEATURE_DUAL_ADDRESS_SPACE)
  #define SET_AEA_COMMON(_regs) \
do { \
    (_regs)->AEA_COMMON(1)  = ((_regs)->CR(1)  & ASD_PRIVATE) == 0; \
    (_regs)->AEA_COMMON(7)  = ((_regs)->CR(7)  & ASD_PRIVATE) == 0; \
} while (0)
#else
  #define SET_AEA_COMMON(_regs) \
do { \
    (_regs)->AEA_COMMON(1)  = ((_regs)->CR(1)  & ASD_PRIVATE) == 0; \
} while (0)
#endif

#if defined(FEATURE_DUAL_ADDRESS_SPACE) || defined(FEATURE_LINKAGE_STACK)
  #define SPACE_BIT(p) \
    (((p)->asc & BIT(PSW_SPACE_BIT)) != 0)
  #define AR_BIT(p) \
    (((p)->asc & BIT(PSW_AR_BIT)) != 0)
  #define PRIMARY_SPACE_MODE(p) \
    ((p)->asc == PSW_PRIMARY_SPACE_MODE)
  #define SECONDARY_SPACE_MODE(p) \
    ((p)->asc == PSW_SECONDARY_SPACE_MODE)
  #define ACCESS_REGISTER_MODE(p) \
    ((p)->asc == PSW_ACCESS_REGISTER_MODE)
  #define HOME_SPACE_MODE(p) \
    ((p)->asc == PSW_HOME_SPACE_MODE)
  #define AEA_MODE(_regs) \
    ( ( REAL_MODE(&(_regs)->psw) ? (SIE_STATE_BIT_ON((_regs), MX, XC) && AR_BIT(&(_regs)->psw) ? 2 : 0) : (((_regs)->psw.asc >> 6) + 1) ) \
    | ( PER_MODE((_regs)) ? 0x40 : 0 ) \
 )
#else
  #define SPACE_BIT(p) (0)
  #define AR_BIT(p) (0)
  #define PRIMARY_SPACE_MODE(p) (1)
  #define SECONDARY_SPACE_MODE(p) (0)
  #define ACCESS_REGISTER_MODE(p) (0)
  #define HOME_SPACE_MODE(p) (0)
  #define AEA_MODE(_regs) \
    ( (REAL_MODE(&(_regs)->psw) ? 0 : 1 ) | (PER_MODE((_regs)) ? 0x40 : 0 ) )
#endif

#if defined(FEATURE_ACCESS_REGISTERS)
 /*
   * Update the aea_ar vector whenever an access register
   * is changed and in armode
  */
  #define SET_AEA_AR(_regs, _arn) \
  do \
  { \
    if (ACCESS_REGISTER_MODE(&(_regs)->psw) && (_arn) > 0) { \
      if ((_regs)->AR((_arn)) == ALET_PRIMARY) \
        (_regs)->AEA_AR((_arn)) = 1; \
      else if ((_regs)->AR((_arn)) == ALET_SECONDARY) \
        (_regs)->AEA_AR((_arn)) = 7; \
      else \
        (_regs)->AEA_AR((_arn)) = 0; \
     } \
  } while (0)
#else
  #define SET_AEA_AR(_regs, _arn)
#endif


/*
 * Conditionally reset the aea_ar vector
 */
#define TEST_SET_AEA_MODE(_regs) \
do \
{ \
  if ((_regs)->aea_mode != AEA_MODE((_regs))) { \
    SET_AEA_MODE((_regs)); \
  } \
} while (0)


 /*
  * Reset aea_ar vector to indicate the appropriate
  * control register:
  *   0 - unresolvable (armode and alet is not 0 or 1)
  *   1 - primary space
  *   7 - secondary space
  *  13 - home space
  *  16 - real
  */
#if defined(FEATURE_ACCESS_REGISTERS)
  #define _CASE_AR_SET_AEA_MODE(_regs) \
    case 2: /* AR */ \
      for(i = USE_INST_SPACE; i < 16; i++) \
          (_regs)->AEA_AR(i) = 1; \
      for (i = 1; i < 16; i++) { \
        if ((_regs)->AR(i) == ALET_SECONDARY) (_regs)->AEA_AR(i) = 7; \
        else if ((_regs)->AR(i) != ALET_PRIMARY) (_regs)->AEA_AR(i) = 0; \
      } \
      break;
#else
  #define _CASE_AR_SET_AEA_MODE(_regs)
#endif

#if defined(FEATURE_DUAL_ADDRESS_SPACE)
  #define _CASE_DAS_SET_AEA_MODE(_regs) \
    case 3: /* SEC */ \
      (_regs)->AEA_AR(USE_INST_SPACE) = 1; \
      for(i = 0; i < 16; i++) \
          (_regs)->AEA_AR(i) = 7; \
      break;
#else
  #define _CASE_DAS_SET_AEA_MODE(_regs)
#endif

#if defined(FEATURE_LINKAGE_STACK)
  #define _CASE_HOME_SET_AEA_MODE(_regs) \
    case 4: /* HOME */ \
      for(i = USE_INST_SPACE; i < 16; i++) \
          (_regs)->AEA_AR(i) = 13; \
      break;
#else
  #define _CASE_HOME_SET_AEA_MODE(_regs)
#endif

#define SET_AEA_MODE(_regs) \
do { \
  int i; \
  int inst_cr = (_regs)->AEA_AR(USE_INST_SPACE); \
  BYTE oldmode = (_regs)->aea_mode; \
  (_regs)->aea_mode = AEA_MODE((_regs)); \
  switch ((_regs)->aea_mode & 0x0F) { \
    case 1: /* PRIM */ \
      for(i = USE_INST_SPACE; i < 16; i++) \
          (_regs)->AEA_AR(i) = 1; \
      break; \
    _CASE_AR_SET_AEA_MODE((_regs)) \
    _CASE_DAS_SET_AEA_MODE((_regs)) \
    _CASE_HOME_SET_AEA_MODE((_regs)) \
    default: /* case 0: REAL */ \
      for(i = USE_INST_SPACE; i < 16; i++) \
          (_regs)->AEA_AR(i) = CR_ASD_REAL; \
  } \
  if (inst_cr != (_regs)->AEA_AR(USE_INST_SPACE)) \
    INVALIDATE_AIA((_regs)); \
  if ((oldmode & PSW_PERMODE) == 0 && ((_regs)->aea_mode & PSW_PERMODE) != 0) { \
    INVALIDATE_AIA((_regs)); \
    if (EN_IC_PER_SA((_regs))) \
      ARCH_DEP(invalidate_tlb)((_regs),~(ACC_WRITE|ACC_CHECK)); \
  } \
} while (0)

/*
 * Accelerated TLB lookup
 */
#define MADDRL(           _addr,   _len,   _arn,   _regs,   _acctype,   _akey  ) \
    ARCH_DEP( maddr_l )( (_addr), (_len), (_arn), (_regs), (_acctype), (_akey) )

/* Old style accelerated lookup (without length) */
#define MADDR(_addr, _arn, _regs, _acctype, _akey) \
    MADDRL( (_addr), 1, (_arn), (_regs), (_acctype), (_akey))

/*
 * PER Successful Branch
 */
#if defined(FEATURE_PER)
 #if defined(FEATURE_PER2)
  #define PER_SB(_regs, _addr) \
   do { \
    if (unlikely(EN_IC_PER_SB((_regs))) \
     && (!((_regs)->CR(9) & CR9_BAC) \
      || PER_RANGE_CHECK((_addr) & ADDRESS_MAXWRAP((_regs)), \
                          (_regs)->CR(10), (_regs)->CR(11)) \
        ) \
       ) \
     ON_IC_PER_SB((_regs)); \
   } while (0)
 #else /*!defined(FEATURE_PER2)*/
  #define PER_SB(_regs, _addr) \
   do { \
    if (unlikely(EN_IC_PER_SB((_regs)))) \
     ON_IC_PER_SB((_regs)); \
   } while (0)
 #endif /*!defined(FEATURE_PER2)*/
#else /*!defined(FEATURE_PER)*/
 #define PER_SB(_regs,_addr)
#endif /*!defined(FEATURE_PER)*/

/* end of FEATURES.H */
