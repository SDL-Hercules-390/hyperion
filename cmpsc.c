/* CMPSC.C      (c) Bernard van der Helm, 2000-2014                  */
/*              S/390 compression call instruction                   */

/*----------------------------------------------------------------------------*/
/* Implementation of the S/390 compression call instruction described in      */
/* SA22-7208-01: Data Compression within the Hercules S/390 emulator.         */
/* This implementation couldn't be done without the test programs from        */
/* Mario Bezzi. Thanks Mario! Also special thanks to Greg Smith who           */
/* introduced iregs, needed when a page fault occurs.                         */
/*                                                                            */
/* Please pay attention to the Q Public License Version 1. This is open       */
/* source, but you are not allowed to "reuse" parts for your own purpose      */
/* without the author's written permission!                                   */
/*                                                                            */
/* Implemented "unique" features:                                             */
/*   8 index symbol block fetching and storing, preventing cbn calculations.  */
/*   Expand symbol translation caching.                                       */
/*   Compression dead end determination and elimination.                      */
/*   Proactive dead end determination and elimination.                        */
/*                                                                            */
/*                              (c) Copyright Bernard van der Helm, 2000-2014 */
/*                              Noordwijkerhout, The Netherlands.             */
/*                                                                            */
/*----------------------------------------------------------------------------*/

#include "hstdinc.h"    // (MUST be first #include in EVERY source file)

#if !defined(_HENGINE_DLL_)
#define _HENGINE_DLL_
#endif

#ifndef _CMPSC_C_
#define _CMPSC_C_
#endif /* #ifndef _CMPSC_C_ */

#if !defined( NOT_HERC )                          // (building Hercules?)
#include "hercules.h"
#include "opcode.h"
#include "inline.h"
#endif

#ifdef FEATURE_COMPRESSION
/*============================================================================*/
/* Common                                                                     */
/*============================================================================*/

/*----------------------------------------------------------------------------*/
/* Debugging options:                                                         */
/*----------------------------------------------------------------------------*/
#if 0
#define OPTION_CMPSC_DEBUG
#define TRUEFALSE(boolean)   ((boolean) ? "True" : "False")
#endif /* #if 0|1 */

/*----------------------------------------------------------------------------*/
/* After a successful compression of characters to an index symbol or a       */
/* successful translation of an index symbol to characters, the registers     */
/* must be updated.                                                           */
/*----------------------------------------------------------------------------*/
#define ADJUSTREGS(r, regs, iregs, len) \
{ \
  SET_GR_A((r), (iregs), (GR_A((r), (iregs)) + (len)) & ADDRESS_MAXWRAP((regs))); \
  SET_GR_A((r) + 1, (iregs), GR_A((r) + 1, (iregs)) - (len)); \
}

/*----------------------------------------------------------------------------*/
/* Commit intermediate registers                                              */
/*----------------------------------------------------------------------------*/
#ifdef OPTION_CMPSC_DEBUG
#define COMMITREGS(regs, iregs, r1, r2) \
  __COMMITREGS((regs), (iregs), (r1), (r2)) \
  logmsg("*** Regs committed\n");
#else
#define COMMITREGS(regs, iregs, r1, r2) \
  __COMMITREGS((regs), (iregs), (r1), (r2))
#endif /* ifdef OPTION_CMPSC_DEBUG */
#define __COMMITREGS(regs, iregs, r1, r2) \
{ \
  SET_GR_A(1, (regs), GR_A(1, (iregs))); \
  SET_GR_A((r1), (regs), GR_A((r1), (iregs))); \
  SET_GR_A((r1) + 1, (regs), GR_A((r1) + 1, (iregs))); \
  SET_GR_A((r2), (regs), GR_A((r2), (iregs))); \
  SET_GR_A((r2) + 1, (regs), GR_A((r2) + 1, (iregs))); \
}

/*----------------------------------------------------------------------------*/
/* Commit intermediate registers, except for GR1                              */
/*----------------------------------------------------------------------------*/
#ifdef OPTION_CMPSC_DEBUG
#define COMMITREGS2(regs, iregs, r1, r2) \
  __COMMITREGS2((regs), (iregs), (r1), (r2)) \
  logmsg("*** Regs committed\n");
#else
#define COMMITREGS2(regs, iregs, r1, r2) \
  __COMMITREGS2((regs), (iregs), (r1), (r2))
#endif /* #ifdef OPTION_CMPSC_DEBUG */
#define __COMMITREGS2(regs, iregs, r1, r2) \
{ \
  SET_GR_A((r1), (regs), GR_A((r1), (iregs))); \
  SET_GR_A((r1) + 1, (regs), GR_A((r1) + 1, (iregs))); \
  SET_GR_A((r2), (regs), GR_A((r2), (iregs))); \
  SET_GR_A((r2) + 1, (regs), GR_A((r2) + 1, (iregs))); \
}

/*----------------------------------------------------------------------------*/
/* Initialize intermediate registers                                          */
/*----------------------------------------------------------------------------*/
#define INITREGS(iregs, regs, r1, r2) \
{ \
  (iregs)->gr[1] = (regs)->gr[1]; \
  (iregs)->gr[(r1)] = (regs)->gr[(r1)]; \
  (iregs)->gr[(r1) + 1] = (regs)->gr[(r1) + 1]; \
  (iregs)->gr[(r2)] = (regs)->gr[(r2)]; \
  (iregs)->gr[(r2) + 1] = (regs)->gr[(r2) + 1]; \
}

#if __GEN_ARCH == 900
#undef INITREGS
#define INITREGS(iregs, regs, r1, r2) \
{ \
  (iregs)->gr[1] = (regs)->gr[1]; \
  (iregs)->gr[(r1)] = (regs)->gr[(r1)]; \
  (iregs)->gr[(r1) + 1] = (regs)->gr[(r1) + 1]; \
  (iregs)->gr[(r2)] = (regs)->gr[(r2)]; \
  (iregs)->gr[(r2) + 1] = (regs)->gr[(r2) + 1]; \
  (iregs)->psw.amode64 = (regs)->psw.amode64; \
}
#endif /* #if __GEN_ARCH == 900 */

/*----------------------------------------------------------------------------*/
/* General Purpose Register 0 macro's (GR0)                                   */
/*----------------------------------------------------------------------------*/
/* cdss  : compressed-data symbol size                                        */
/* e     : expansion operation                                                */
/* f1    : format-1 sibling descriptors                                       */
/* st    : symbol-translation option                                          */
/* zp    : zero padding                                                       */
/*                                                                            */
/* We recognize the zp flag and do conform POP nothing! We do something       */
/* better: Eight index symbol processing. It saves a lot of time in cbn       */
/* processing, index symbol fetching and storing. Please contact me if a      */
/* 100% equal functionality is needed. I doubt it!                            */
/*----------------------------------------------------------------------------*/
#define GR0_cdss(regs)       (((regs)->GR_L(0) & 0x0000F000) >> 12)
#define GR0_e(regs)          ((regs)->GR_L(0) & 0x00000100)
#define GR0_f1(regs)         ((regs)->GR_L(0) & 0x00000200)
#define GR0_st(regs)         ((regs)->GR_L(0) & 0x00010000)
#define GR0_zp(regs)         ((regs)->GR_L(0) & 0x00020000)

/*----------------------------------------------------------------------------*/
/* General Purpose Register 0 macro's (GR0) derived                           */
/*----------------------------------------------------------------------------*/
/* dcten      : # dictionary entries                                          */
/* dctsz      : dictionary size                                               */
/* smbsz      : symbol size                                                   */
/*----------------------------------------------------------------------------*/
#define GR0_dcten(regs)      (0x100 << GR0_cdss((regs)))
#define GR0_dctsz(regs)      (0x800 << GR0_cdss((regs)))
#define GR0_smbsz(regs)      (GR0_cdss((regs)) + 8)

/*----------------------------------------------------------------------------*/
/* General Purpose Register 1 macro's (GR1)                                   */
/*----------------------------------------------------------------------------*/
/* cbn   : compressed-data bit number                                         */
/* dictor: compression dictionary or expansion dictionary                     */
/* sttoff: symbol-translation-table offset                                    */
/*----------------------------------------------------------------------------*/
#define GR1_cbn(regs)        (((regs)->GR_L(1) & 0x00000007))
#define GR1_dictor(regs)     (GR_A(1, (regs)) & ((GREG) 0xFFFFFFFFFFFFF000ULL))
#define GR1_sttoff(regs)     (((regs)->GR_L(1) & 0x00000FF8) << 4)

/*----------------------------------------------------------------------------*/
/* Set the compressed bit number in GR1                                       */
/*----------------------------------------------------------------------------*/
#define GR1_setcbn(regs, cbn) ((regs)->GR_L(1) = ((regs)->GR_L(1) & 0xFFFFFFF8) | ((cbn) & 0x00000007))

/*----------------------------------------------------------------------------*/
/* Constants                                                                  */
/*----------------------------------------------------------------------------*/
#define MINPROC_SIZE         32768     /* Minumum processing size             */

/*----------------------------------------------------------------------------*/
/* Typedefs and prototypes                                                    */
/*----------------------------------------------------------------------------*/
#ifndef NO_2ND_COMPILE
struct cc                              /* Compress context                    */
{
  BYTE *cce;                           /* Character entry under investigation */
  unsigned dctsz;                      /* Dictionary size                     */
  BYTE deadadm[8192][0x100 / 8];       /* Dead end administration             */
  BYTE deadend;                        /* Dead end indicator                  */
  BYTE *dest;                          /* Destination MADDR address           */
  BYTE *dict[32];                      /* Dictionary MADDR addresses          */
  GREG dictor;                         /* Dictionary origin                   */
  BYTE *edict[32];                     /* Expansion dictionary MADDR addrs    */
  int f1;                              /* Indication format-1 sibling descr   */
  REGS *iregs;                         /* Intermediate registers              */
  U16 is[8];                           /* Cache for 8 index symbols           */
  unsigned ofst;                       /* Latest fetched offset               */
  int r1;                              /* Guess what                          */
  int r2;                              /* Yep                                 */
  REGS *regs;                          /* Registers                           */
  BYTE searchadm[1][0x100 / 8];        /* Search administration               */
  unsigned smbsz;                      /* Symbol size                         */
  BYTE *src;                           /* Source MADDR page address           */
  unsigned srclen;                     /* Source length left in page          */
  BYTE st;                             /* Symbol translation                  */
};

struct ec                              /* Expand context                      */
{
  BYTE *dest;                          /* Destination MADDR page address      */
  BYTE *dict[32];                      /* Dictionary MADDR addresses          */
  GREG dictor;                         /* Dictionary origin                   */
  BYTE ec[8192 * 7];                   /* Expanded index symbol cache         */
  int eci[8192];                       /* Index within cache for is           */
  int ecl[8192];                       /* Size of expanded is                 */
  int ecwm;                            /* Water mark                          */
  REGS *iregs;                         /* Intermediate registers              */
  BYTE oc[8 * 260];                    /* Output cache                        */
  unsigned ocl;                        /* Output cache length                 */
  int r1;                              /* Guess what                          */
  int r2;                              /* Yep                                 */
  REGS *regs;                          /* Registers                           */
  unsigned smbsz;                      /* Symbol size                         */
  BYTE *src;                           /* Source MADDR page address           */

#ifdef OPTION_CMPSC_DEBUG
  unsigned dbgac;                      /* Alphabet characters                 */
  unsigned dbgbi;                      /* bytes in                            */
  unsigned dbgbo;                      /* bytes out                           */
  unsigned dbgch;                      /* Cache hits                          */
  unsigned dbgiss;                     /* Expanded iss                        */
#endif /* #ifdef OPTION_CMPSC_DEBUG */

};
#endif /* #ifndef NO_2ND_COMPILE */

static void  ARCH_DEP(cmpsc_compress)(int r1, int r2, REGS *regs, REGS *iregs);
static int   ARCH_DEP(cmpsc_compress_single_is)(struct cc *cc);
static void  ARCH_DEP(cmpsc_expand)(int r1, int r2, REGS *regs, REGS *iregs);
static void  ARCH_DEP(cmpsc_expand_is)(struct ec *ec, U16 is);
static int   ARCH_DEP(cmpsc_expand_single_is)(struct ec *ec);
static BYTE *ARCH_DEP(cmpsc_fetch_cce)(struct cc *cc, unsigned index);
static int   ARCH_DEP(cmpsc_fetch_ch)(struct cc *cc);
static int   ARCH_DEP(cmpsc_fetch_is)(struct ec *ec, U16 *is);
static void  ARCH_DEP(cmpsc_fetch_iss)(struct ec *ec, U16 is[8]);
#ifdef OPTION_CMPSC_DEBUG
static void  cmpsc_print_cce(BYTE *cce);
static void  cmpsc_print_ece(BYTE *ece);
static void  cmpsc_print_sd(int f1, BYTE *sd1, BYTE *sd2);
#endif /* #ifdef OPTION_CMPSC_DEBUG */
static int   ARCH_DEP(cmpsc_search_cce)(struct cc *cc, U16 *is);
static int   ARCH_DEP(cmpsc_search_sd)(struct cc *cc, U16 *is);
static int   ARCH_DEP(cmpsc_store_is)(struct cc *cc, U16 is);
static void  ARCH_DEP(cmpsc_store_iss)(struct cc *cc);
static int   ARCH_DEP(cmpsc_test_ec)(struct cc *cc, BYTE *cce);
static int   ARCH_DEP(cmpsc_vstore)(struct ec *ec, BYTE *buf, unsigned len);

/*----------------------------------------------------------------------------*/
/* B263 CMPSC - Compression Call                                        [RRE] */
/*----------------------------------------------------------------------------*/
DEF_INST(legacy_cmpsc)
{
  REGS iregs;                          /* Intermediate registers              */
  int r1;                              /* Guess what                          */
  int r2;                              /* Yep                                 */

  RRE(inst, regs, r1, r2);

#ifdef OPTION_CMPSC_DEBUG
  logmsg("CMPSC: compression call\n");
  logmsg(" r1      : GR%02d\n", r1);
  logmsg(" address : " F_VADR "\n", regs->GR(r1));
  logmsg(" length  : " F_GREG "\n", regs->GR(r1 + 1));
  logmsg(" r2      : GR%02d\n", r2);
  logmsg(" address : " F_VADR "\n", regs->GR(r2));
  logmsg(" length  : " F_GREG "\n", regs->GR(r2 + 1));
  logmsg(" GR00    : " F_GREG "\n", regs->GR(0));
  logmsg("   zp    : %s\n", TRUEFALSE(GR0_zp(regs)));
  logmsg("   st    : %s\n", TRUEFALSE(GR0_st(regs)));
  logmsg("   cdss  : %d\n", GR0_cdss(regs));
  logmsg("   f1    : %s\n", TRUEFALSE(GR0_f1(regs)));
  logmsg("   e     : %s\n", TRUEFALSE(GR0_e(regs)));
  logmsg(" GR01    : " F_GREG "\n", regs->GR(1));
  logmsg("   dictor: " F_GREG "\n", GR1_dictor(regs));
  logmsg("   sttoff: %08X\n", GR1_sttoff(regs));
  logmsg("   cbn   : %d\n", GR1_cbn(regs));
#endif /* #ifdef OPTION_CMPSC_DEBUG */

  /* Check the registers on even-odd pairs and valid compression-data symbol size */
  if(unlikely(r1 & 0x01 || r2 & 0x01 || !GR0_cdss(regs) || GR0_cdss(regs) > 5))
    ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);

  /* Check for empty input */
  if(unlikely(!GR_A(r2 + 1, regs)))
  {

#ifdef OPTION_CMPSC_DEBUG
    logmsg(" Zero input, returning cc0\n");
#endif /* #ifdef OPTION_CMPSC_DEBUG */

    regs->psw.cc = 0;
    return;
  }

  /* Check for empty output */
  if(unlikely(!GR_A(r1 + 1, regs)))
  {

#ifdef OPTION_CMPSC_DEBUG
    logmsg(" Zero output, returning cc1\n");
#endif /* #ifdef OPTION_CMPSC_DEBUG */

    regs->psw.cc = 1;
    return;
  }

  /* Initialize intermediate registers */
  INITREGS(&iregs, regs, r1, r2);

  /* Now go to the requested function */
  if(likely(GR0_e(regs)))
    ARCH_DEP(cmpsc_expand)(r1, r2, regs, &iregs);
  else
    ARCH_DEP(cmpsc_compress)(r1, r2, regs, &iregs);
}

/*============================================================================*/
/* Compress                                                                   */
/*============================================================================*/

/*----------------------------------------------------------------------------*/
/* Compression Character Entry macro's (CCE)                                  */
/*----------------------------------------------------------------------------*/
/* act    : additional-extension-character count                              */
/* cct    : child count                                                       */
/* cptr   : child pointer; index of first child                               */
/* d      : double-character entry                                            */
/* x(i)   : examine child bit for children 1 to 5                             */
/* y(i)   : examine child bit for 6th/13th and 7th/14th sibling               */
/*----------------------------------------------------------------------------*/
#define CCE_act(cce)         ((cce)[1] >> 5)
#define CCE_cct(cce)         ((cce)[0] >> 5)
#define CCE_cptr(cce)        ((((cce)[1] & 0x1f) << 8) | (cce)[2])
#define CCE_d(cce)           ((cce)[1] & 0x20)
#define CCE_x(cce, i)        ((cce)[0] & (0x10 >> (i)))
#define CCE_y(cce, i)        ((cce)[1] & (0x80 >> (i)))

/*----------------------------------------------------------------------------*/
/* Compression Character Entry macro's (CCE) derived                          */
/*----------------------------------------------------------------------------*/
/* cc(i)  : child character                                                   */
/* ccc(i) : indication consecutive child character                            */
/* ccs    : number of child characters                                        */
/* ec(i)  : additional extension character                                    */
/* ecs    : number of additional extension characters                         */
/* mcc    : indication if siblings follow child characters                    */
/*----------------------------------------------------------------------------*/
#define CCE_cc(cce, i)       ((cce)[3 + CCE_ecs((cce)) + (i)])
#define CCE_ccc(cce, i)      (CCE_cc((cce), (i)) == CCE_cc((cce), 0))
#define CCE_ccs(cce)         (CCE_cct((cce)) - (CCE_mcc((cce)) ? 1 : 0))
#define CCE_ec(cce, i)       ((cce)[3 + (i)])
#define CCE_ecs(cce)         ((CCE_cct((cce)) <= 1) ? CCE_act((cce)) : (CCE_d((cce)) ? 1 : 0))
#define CCE_mcc(cce)         ((CCE_cct((cce)) + (CCE_d((cce)) ? 1 : 0) == 6))

/*----------------------------------------------------------------------------*/
/* Format-0 Sibling Descriptors macro's (SD0)                                 */
/*----------------------------------------------------------------------------*/
/* sct    : sibling count                                                     */
/* y(i)   : examine child bit for siblings 1 to 5                             */
/*----------------------------------------------------------------------------*/
#define SD0_sct(sd0)         ((sd0)[0] >> 5)
#define SD0_y(sd0, i)        ((sd0)[0] & (0x10 >> (i)))

/*----------------------------------------------------------------------------*/
/* Format-0 Sibling Descriptors macro's (SD0) derived                         */
/*----------------------------------------------------------------------------*/
/* ccc(i) : indication consecutive child character                            */
/* ecb(i) : examine child bit, if y then 6th/7th fetched from parent          */
/* msc    : indication if siblings follows last sibling                       */
/* sc(i)  : sibling character                                                 */
/* scs    : number of sibling characters                                      */
/*----------------------------------------------------------------------------*/
#define SD0_ccc(sd0, i)      (SD0_sc((sd0), (i)) == SD0_sc((sd0), 0))
#define SD0_ecb(sd0, i, cce, y) (((i) < 5) ? SD0_y((sd0), (i)) : (y) ? CCE_y((cce), ((i) - 5)) : 1)
#define SD0_msc(sd0)         (!SD0_sct((sd0)))
#define SD0_sc(sd0, i)       ((sd0)[1 + (i)])
#define SD0_scs(sd0)         (SD0_msc((sd0)) ? 7 : SD0_sct((sd0)))

/*----------------------------------------------------------------------------*/
/* Format-1 Sibling Descriptors macro's (SD1)                                 */
/*----------------------------------------------------------------------------*/
/* sct    : sibling count                                                     */
/* y(i)   : examine child bit for sibling 1 to 12                             */
/*----------------------------------------------------------------------------*/
#define SD1_sct(sd1)         ((sd1)[0] >> 4)
#define SD1_y(sd1, i)        ((i) < 4 ? ((sd1)[0] & (0x08 >> (i))) : ((sd1)[1] & (0x800 >> (i))))

/*----------------------------------------------------------------------------*/
/* Format-1 Sibling Descriptors macro's (SD1) derived                         */
/*----------------------------------------------------------------------------*/
/* ccc(i) : indication consecutive child character                            */
/* ecb(i) : examine child bit, if y then 13th/14th fetched from parent        */
/* msc    : indication if siblings follows last sibling                       */
/* sc(i)  : sibling character                                                 */
/* scs    : number of sibling characters                                      */
/*----------------------------------------------------------------------------*/
#define SD1_ccc(sd1, sd2, i) (SD1_sc((sd1), (sd2), (i)) == SD1_sc((sd1), (sd2), 0))
#define SD1_ecb(sd1, i, cce, y) (((i) < 12) ? SD1_y((sd1), (i)) : (y) ? CCE_y((cce), ((i) - 12)) : 1)
#define SD1_msc(sd1)         ((SD1_sct((sd1)) == 15))
#define SD1_sc(sd1, sd2, i)  ((i) < 6 ? (sd1)[2 + (i)] : (sd2)[(i) - 6])
#define SD1_scs(sd1)         (SD1_msc((sd1)) ? 14 : SD1_sct((sd1)))

/*----------------------------------------------------------------------------*/
/* Format independent sibling descriptor macro's                              */
/*----------------------------------------------------------------------------*/
#define SD_ccc(f1, sd1, sd2, i) ((f1) ? SD1_ccc((sd1), (sd2), (i)) : SD0_ccc((sd1), (i)))
#define SD_ecb(f1, sd1, i, cce, y) ((f1) ? SD1_ecb((sd1), (i), (cce), (y)) : SD0_ecb((sd1), (i), (cce), (y)))
#define SD_msc(f1, sd1)      ((f1) ? SD1_msc((sd1)) : SD0_msc((sd1)))
#define SD_sc(f1, sd1, sd2, i) ((f1) ? SD1_sc((sd1), (sd2), (i)) : SD0_sc((sd1), (i)))
#define SD_scs(f1, sd1)      ((f1) ? SD1_scs((sd1)) : SD0_scs((sd1)))

/*----------------------------------------------------------------------------*/
/* ADJUSTREGS in compression context                                          */
/*----------------------------------------------------------------------------*/
#define ADJUSTREGSC(cc, r, regs, iregs, len) \
{ \
  ADJUSTREGS((r), (regs), (iregs), (len)) \
  if(likely((cc)->srclen > (len))) \
  { \
    (cc)->src += (len); \
    (cc)->srclen -= (len); \
  } \
  else \
  { \
    (cc)->src = NULL; \
    (cc)->srclen = 0; \
  } \
}
#define BIT_get(array, is, ch) ((array)[(is)][(ch) / 8] & (0x80 >> ((ch) % 8)))
#define BIT_set(array, is, ch) ((array)[(is)][(ch) / 8] |= (0x80 >> ((ch) % 8)))

/*============================================================================*/

/*----------------------------------------------------------------------------*/
/* cmpsc_compress                                                             */
/*----------------------------------------------------------------------------*/
static void ARCH_DEP(cmpsc_compress)(int r1, int r2, REGS *regs, REGS *iregs)
{
  struct cc cc;                        /* Compression context                 */
  int i;                               /* Index                               */
  U16 is;                              /* Last matched index symbol           */
  int j;                               /* Index                               */
  GREG srclen;                         /* Source length                       */

  /* Initialize compression context */
  cc.dctsz = GR0_dctsz(regs);
  memset(cc.deadadm, 0, sizeof(cc.deadadm));
  cc.dest = NULL;
  memset(cc.dict, 0, sizeof(cc.dict));
  memset(cc.edict, 0, sizeof(cc.edict));
  cc.dictor = GR1_dictor(iregs);
  cc.f1 = GR0_f1(regs);
  cc.iregs = iregs;
  cc.r1 = r1;
  cc.r2 = r2;
  cc.regs = regs;
  cc.smbsz = GR0_smbsz(regs);
  cc.src = NULL;
  cc.srclen = 0;
  cc.st = GR0_st(regs) ? 1 : 0;

  /* Initialize values */
  srclen = GR_A(cc.r2 + 1, cc.iregs);

  /*--------------------------------------------------------------------------*/
  /* Process individual index symbols until cbn becomes zero                  */
  while(unlikely(GR1_cbn(cc.iregs)))
  {
    if(unlikely(ARCH_DEP(cmpsc_compress_single_is)(&cc)))
      return;
  }

  /*--------------------------------------------------------------------------*/
  /* Block processing, cbn stays zero                                         */
  while(likely(GR_A(cc.r1 + 1, cc.iregs) >= cc.smbsz))
  {
    for(i = 0; i < 8; i++)
    {
      /* Get the next character, return on end of source */
      if(unlikely(!cc.src && ARCH_DEP(cmpsc_fetch_ch)(&cc)))
      {
        /* Write individual found index symbols */
        for(j = 0; j < i; j++)
          ARCH_DEP(cmpsc_store_is)(&cc, cc.is[j]);
        COMMITREGS(cc.regs, cc.iregs, cc.r1, cc.r2);
        return;
      }

#ifdef OPTION_CMPSC_DEBUG
      logmsg("fetch_ch : %02X at " F_VADR "\n", *cc.src, GR_A(cc.r2, cc.iregs));
#endif /* #ifdef OPTION_CMPSC_DEBUG */

      /* Set the alphabet entry and adjust registers */
      is = *cc.src;
      ADJUSTREGSC(&cc, cc.r2, cc.regs, cc.iregs, 1);

      /* Check for alphabet entry ch dead end combination */
      if(unlikely(!(cc.src && BIT_get(cc.deadadm, is, *cc.src))))
      {
        /* Get the alphabet entry and try to find a child */
        cc.cce = ARCH_DEP(cmpsc_fetch_cce)(&cc, is);
        while(ARCH_DEP(cmpsc_search_cce)(&cc, &is))
        {
          /* Check for other dead end combination */
          if(unlikely(cc.src && BIT_get(cc.deadadm, is, *cc.src)))
          {

#ifdef OPTION_CMPSC_DEBUG
            logmsg("dead end : %04X %02X encountered\n", is, *cc.src);
#endif /* #ifdef OPTION_CMPSC_DEBUG */

            break;
          }
        }

        /* Registrate possible found dead ends */
        if(unlikely(cc.deadend && cc.src))
        {

#ifdef OPTION_CMPSC_DEBUG
          logmsg("dead end : %04X in combination with", is);
          for(j = 0; j < 0x100; j++)
          {
            if(!(j % 16))
              logmsg("\n         :");
            if(BIT_get(cc.searchadm, 0, j))
              logmsg("   ");
            else
              logmsg(" %02X", j);
          }
          logmsg("\n");
#endif /* #ifdef OPTION_CMPSC_DEBUG */

          /* Registrate all discovered dead ends */
          for(j = 0; j < 0x100 / 8; j++)
            cc.deadadm[is][j] = ~cc.searchadm[0][j];
        }
      }

#ifdef OPTION_CMPSC_DEBUG
      else
        logmsg("dead end : %04X %02X encountered\n", is, *cc.src);
#endif /* #ifdef OPTION_CMPSC_DEBUG */

      /* Write the last match */
      cc.is[i] = is;

#ifdef OPTION_CMPSC_DEBUG
      logmsg("compress : is %04X (%d)\n", is, i);
#endif /* #ifdef OPTION_CMPSC_DEBUG */

    }

    /* Write index symbols and commit */
    ARCH_DEP(cmpsc_store_iss)(&cc);
    COMMITREGS2(cc.regs, cc.iregs, cc.r1, cc.r2);

    /* Return with cc3 on interrupt pending after a minumum size of processing */
    if(unlikely(srclen - GR_A(cc.r2 + 1, cc.iregs) >= MINPROC_SIZE && INTERRUPT_PENDING(cc.regs)))
    {

#ifdef OPTION_CMPSC_DEBUG
      logmsg("Interrupt pending, commit and return with cc3\n");
#endif /* #ifdef OPTION_CMPSC_DEBUG */

      cc.regs->psw.cc = 3;
      return;
    }
  }

  /*--------------------------------------------------------------------------*/
  /* Process individual index symbols until end of destination (or source)    */
  while(!likely(ARCH_DEP(cmpsc_compress_single_is)(&cc)));
}

/*----------------------------------------------------------------------------*/
/* cmpsc_compress_single_is                                                   */
/*----------------------------------------------------------------------------*/
static int ARCH_DEP(cmpsc_compress_single_is)(struct cc *cc)
{
  int i;                               /* Index                               */
  U16 is;                              /* index symbol                        */

  /* Get the next character, return -1 on end of source */
  if(unlikely(!cc->src && ARCH_DEP(cmpsc_fetch_ch)(cc)))
    return(-1);

#ifdef OPTION_CMPSC_DEBUG
  logmsg("fetch_ch : %02X at " F_VADR "\n", *cc->src, GR_A(cc->r2, cc->iregs));
#endif /* #ifdef OPTION_CMPSC_DEBUG */

  /* Set the alphabet entry and adjust registers */
  is = *cc->src;
  ADJUSTREGSC(cc, cc->r2, cc->regs, cc->iregs, 1);

  /* Search for child when no src and no dead end combination */
  if(unlikely(!(cc->src && BIT_get(cc->deadadm, is, *cc->src))))
  {
    /* Get the alphabet entry and try to find a child */
    cc->cce = ARCH_DEP(cmpsc_fetch_cce)(cc, is);
    while(ARCH_DEP(cmpsc_search_cce)(cc, &is))
    {
      /* Check for (found cce entry + ch) dead end combination */
      if(unlikely(cc->src && BIT_get(cc->deadadm, is, *cc->src)))
      {

#ifdef OPTION_CMPSC_DEBUG
        logmsg("dead end : %04X %02X encountered\n", is, *cc->src);
#endif /* #ifdef OPTION_CMPSC_DEBUG */

        break;
      }
    }

    /* Registrate possible found dead ends */
    if(unlikely(cc->deadend && cc->src))
    {

#ifdef OPTION_CMPSC_DEBUG
      logmsg("dead end : %04X in combination with", is);
      for(i = 0; i < 0x100; i++)
      {
        if(!(i % 16))
          logmsg("\n         :");
        if(BIT_get(cc->searchadm, 0, i))
          logmsg("   ");
        else
          logmsg(" %02X", i);
      }
      logmsg("\n");
#endif /* #ifdef OPTION_CMPSC_DEBUG */

      /* Registrate all discovered dead ends */
      for(i = 0; i < 0x100 / 8; i++)
        cc->deadadm[is][i] = ~cc->searchadm[0][i];
    }
  }

#ifdef OPTION_CMPSC_DEBUG
  else
    logmsg("dead end : %04X %02X encountered\n", is, *cc->src);
#endif /* #ifdef OPTION_CMPSC_DEBUG */

  /* Write the last match, return on end of destination */
  if(unlikely(ARCH_DEP(cmpsc_store_is)(cc, is)))
    return(-1);

  /* Commit registers */
  COMMITREGS(cc->regs, cc->iregs, cc->r1, cc->r2);
  return(0);
}

/*----------------------------------------------------------------------------*/
/* cmpsc_fetch_cce (compression character entry)                              */
/*----------------------------------------------------------------------------*/
static BYTE *ARCH_DEP(cmpsc_fetch_cce)(struct cc *cc, unsigned index)
{
  BYTE *cce;                           /* Compression child entry             */
  unsigned cct;                        /* Child count                         */

  index *= 8;
  if(unlikely(!cc->dict[index / 0x800]))
    cc->dict[index / 0x800] = MADDR((cc->dictor + (index / 0x800) * 0x800) & ADDRESS_MAXWRAP(cc->regs), cc->r2, cc->regs, ACCTYPE_READ, cc->regs->psw.pkey);
  cce = &cc->dict[index / 0x800][index % 0x800];
  ITIMER_SYNC((cc->dictor + index) & ADDRESS_MAXWRAP(cc->regs), 8 - 1, cc->regs);

#ifdef OPTION_CMPSC_DEBUG
  logmsg("fetch_cce: index %04X\n", index / 8);
  cmpsc_print_cce(cce);
#endif /* #ifdef OPTION_CMPSC_DEBUG */

  /* Check for data exception */
  cct = CCE_cct(cce);
  if(cct < 2)
  {
    if(unlikely(CCE_act(cce) > 4))
    {
      cc->regs->dxc = DXC_DECIMAL;
      ARCH_DEP(program_interrupt)(cc->regs, PGM_DATA_EXCEPTION);
    }
  }
  else
  {
    if(!CCE_d(cce))
    {
      if(unlikely(cct == 7))
      {
        cc->regs->dxc = DXC_DECIMAL;
        ARCH_DEP(program_interrupt)(cc->regs, PGM_DATA_EXCEPTION);
      }
    }
    else
    {
      if(unlikely(cct > 5))
      {
        cc->regs->dxc = DXC_DECIMAL;
        ARCH_DEP(program_interrupt)(cc->regs, PGM_DATA_EXCEPTION);
      }
    }
  }
  return(cce);
}

/*----------------------------------------------------------------------------*/
/* cmpsc_fetch_ch (character)                                                 */
/*----------------------------------------------------------------------------*/
static int ARCH_DEP(cmpsc_fetch_ch)(struct cc *cc)
{
  /* Check for end of source condition */
  if(unlikely(!GR_A(cc->r2 + 1, cc->iregs)))
  {

#ifdef OPTION_CMPSC_DEBUG
    logmsg("fetch_ch : reached end of source\n");
#endif /* #ifdef OPTION_CMPSC_DEBUG */

    cc->regs->psw.cc = 0;
    return(-1);
  }

  /* Calculate source length in page */
  cc->srclen = 0x800 - (GR_A(cc->r2, cc->iregs) & 0x7ff);
  if(unlikely(GR_A(cc->r2 + 1, cc->iregs) < cc->srclen))
    cc->srclen = (unsigned int) (GR_A(cc->r2 + 1, cc->iregs));

  /* Get address */
  cc->src = MADDR(GR_A(cc->r2, cc->iregs) & ADDRESS_MAXWRAP(cc->regs), cc->r2, cc->regs, ACCTYPE_READ, cc->regs->psw.pkey);
  return(0);
}

#ifndef NO_2ND_COMPILE
#ifdef OPTION_CMPSC_DEBUG
/*----------------------------------------------------------------------------*/
/* cmpsc_print_cce (compression character entry)                              */
/*----------------------------------------------------------------------------*/
static void cmpsc_print_cce(BYTE *cce)
{
  int j;                               /* Index                               */
  int prt_detail;                      /* Switch for detailed printing        */

  logmsg("  cce    : ");
  prt_detail = 0;
  for(j = 0; j < 8; j++)
  {
    if(!prt_detail && cce[j])
      prt_detail = 1;
    logmsg("%02X", cce[j]);
  }
  logmsg("\n");
  if(prt_detail)
  {
    logmsg("  cct    : %d\n", CCE_cct(cce));
    switch(CCE_cct(cce))
    {
      case 0:
      {
        logmsg("  act    : %d\n", (int) CCE_act(cce));
        if(CCE_act(cce))
        {
          logmsg("  ec(s)  :");
          for(j = 0; j < CCE_ecs(cce); j++)
            logmsg(" %02X", CCE_ec(cce, j));
          logmsg("\n");
        }
        break;
      }
      case 1:
      {
        logmsg("  x1     : %c\n", (int) (CCE_x(cce, 0) ? '1' : '0'));
        logmsg("  act    : %d\n", (int) CCE_act(cce));
        logmsg("  cptr   : %04X\n", CCE_cptr(cce));
        if(CCE_act(cce))
        {
          logmsg("  ec(s)  :");
          for(j = 0; j < CCE_ecs(cce); j++)
            logmsg(" %02X", CCE_ec(cce, j));
          logmsg("\n");
        }
        logmsg("  cc     : %02X\n", CCE_cc(cce, 0));
        break;
      }
      default:
      {
        logmsg("  x1..x5 : ");
        for(j = 0; j < 5; j++)
          logmsg("%c", (int) (CCE_x(cce, j) ? '1' : '0'));
        logmsg("\n  y1..y2 : ");
        for(j = 0; j < 2; j++)
          logmsg("%c", (int) (CCE_y(cce, j) ? '1' : '0'));
        logmsg("\n  d      : %s\n", TRUEFALSE(CCE_d(cce)));
        logmsg("  cptr   : %04X\n", CCE_cptr(cce));
        if(CCE_d(cce))
          logmsg("  ec     : %02X\n", CCE_ec(cce, 0));
        logmsg("  ccs    :");
        for(j = 0; j < CCE_ccs(cce); j++)
          logmsg(" %02X", CCE_cc(cce, j));
        logmsg("\n");
        break;
      }
    }
  }
}

/*----------------------------------------------------------------------------*/
/* cmpsc_print_sd (sibling descriptor)                                        */
/*----------------------------------------------------------------------------*/
static void cmpsc_print_sd(int f1, BYTE *sd1, BYTE *sd2)
{
  int j;                               /* Index                               */
  int prt_detail;                      /* Switch for detailed printing        */

  if(f1)
  {
    logmsg("  sd1    : ");
    prt_detail = 0;
    for(j = 0; j < 8; j++)
    {
      if(!prt_detail && sd1[j])
        prt_detail = 1;
      logmsg("%02X", sd1[j]);
    }
    for(j = 0; j < 8; j++)
    {
      if(!prt_detail && sd2[j])
        prt_detail = 1;
      logmsg("%02X", sd2[j]);
    }
    logmsg("\n");
    if(prt_detail)
    {
      logmsg("  sct    : %d\n", SD1_sct(sd1));
      logmsg("  y1..y12: ");
      for(j = 0; j < 12; j++)
        logmsg("%c", (SD1_y(sd1, j) ? '1' : '0'));
      logmsg("\n  sc(s)  :");
      for(j = 0; j < SD1_scs(sd1); j++)
        logmsg(" %02X", SD1_sc(sd1, sd2, j));
      logmsg("\n");
    }
  }
  else
  {
    logmsg("  sd0    : ");
    prt_detail = 0;
    for(j = 0; j < 8; j++)
    {
      if(!prt_detail && sd1[j])
        prt_detail = 1;
      logmsg("%02X", sd1[j]);
    }
    logmsg("\n");
    if(prt_detail)
    {
      logmsg("  sct    : %d\n", SD0_sct(sd1));
      logmsg("  y1..y5 : ");
      for(j = 0; j < 5; j++)
        logmsg("%c", (SD0_y(sd1, j) ? '1' : '0'));
      logmsg("\n  sc(s)  :");
      for(j = 0; j < SD0_scs(sd1); j++)
        logmsg(" %02X", SD0_sc(sd1, j));
      logmsg("\n");
    }
  }
}
#endif /* #ifdef OPTION_CMPSC_DEBUG */
#endif /* #ifndef NO_2ND_COMPILE */

/*----------------------------------------------------------------------------*/
/* cmpsc_search_cce (compression character entry)                             */
/*----------------------------------------------------------------------------*/
static int ARCH_DEP(cmpsc_search_cce)(struct cc *cc, U16 *is)
{
  BYTE *ccce;                          /* Child compression character entry   */
  int ccs;                             /* Number of child characters          */
  int i;                               /* Child character index               */
  int ind_search_siblings;             /* Indicator for searching siblings    */

  /* Initialize values */
  ccs = CCE_ccs(cc->cce);

  /* Get the next character when there are children */
  if(likely(ccs))
  {
    if(unlikely(!cc->src && ARCH_DEP(cmpsc_fetch_ch(cc))))
      return(0);

#ifdef OPTION_CMPSC_DEBUG
    logmsg("fetch_ch : %02X at " F_VADR "\n", *cc->src, GR_A(cc->r2, cc->iregs));
#endif /* #ifdef OPTION_CMPSC_DEBUG */

    memset(cc->searchadm, 0, sizeof(cc->searchadm));
    cc->deadend = 1;
    ind_search_siblings = 1;

    /* Now check all children in parent */
    for(i = 0; i < ccs; i++)
    {
      /* Stop searching when child tested and no consecutive child character */
      if(unlikely(!ind_search_siblings && !CCE_ccc(cc->cce, i)))
        return(0);

      /* Compare character with child */
      if(unlikely(*cc->src == CCE_cc(cc->cce, i)))
      {
        /* Child is tested, so stop searching for siblings and no dead end */
        ind_search_siblings = 0;
        cc->deadend = 0;

        /* Check if child should not be examined */
        if(unlikely(!CCE_x(cc->cce, i)))
        {
          /* No need to examine child, found the last match */
          ADJUSTREGSC(cc, cc->r2, cc->regs, cc->iregs, 1);
          *is = CCE_cptr(cc->cce) + i;
          return(0);
        }

        /* Found a child get the character entry and check if additional extension characters match */
        ccce = ARCH_DEP(cmpsc_fetch_cce)(cc, CCE_cptr(cc->cce) + i);
        if(likely(!CCE_ecs(ccce) || !ARCH_DEP(cmpsc_test_ec)(cc, ccce)))
        {
          /* Set last match */
          ADJUSTREGSC(cc, cc->r2, cc->regs, cc->iregs, (U32) CCE_ecs(ccce) + 1);
          *is = CCE_cptr(cc->cce) + i;

#ifdef OPTION_CMPSC_DEBUG
          logmsg("search_cce index %04X parent\n", *is);
#endif  /* #ifdef OPTION_CMPSC_DEBUG */

          /* Found a matching child, make it parent and keep searching */
          cc->cce = ccce;
          return(1);
        }
      }
      BIT_set(cc->searchadm, 0, CCE_cc(cc->cce, i));
    }

    /* Are there siblings? */
    if(likely(CCE_mcc(cc->cce)))
      return(ARCH_DEP(cmpsc_search_sd)(cc, is));
  }
  else
  {
    /* No children, no extension character, always a dead end */
    if(!CCE_act(cc->cce))
    {

#ifdef OPTION_CMPSC_DEBUG
      logmsg("dead end : %04X permanent dead end discovered\n", *is);
#endif /* #ifdef OPTION_CMPSC_DEBUG */

      memset(&cc->deadadm[*is], 0xff, 0x100 / 8);
    }
    cc->deadend = 0;
  }

  /* No siblings, write found index symbol */
  return(0);
}

/*----------------------------------------------------------------------------*/
/* cmpsc_search_sd (sibling descriptor)                                       */
/*----------------------------------------------------------------------------*/
static int ARCH_DEP(cmpsc_search_sd)(struct cc *cc, U16 *is)
{
  BYTE *ccce;                          /* Child compression character entry   */
  int i;                               /* Sibling character index             */
  int ind_search_siblings;             /* Indicator for keep searching        */
  U16 index;                           /* Index within dictionary             */
  int scs;                             /* Number of sibling characters        */
  BYTE *sd1;                           /* Sibling descriptor fmt-0|1 part 1   */
  BYTE *sd2 = NULL;                    /* Sibling descriptor fmt-1 part 2     */
  int sd_ptr;                          /* Pointer to sibling descriptor       */
  int searched;                        /* Number of children searched         */
  int y_in_parent;                     /* Indicator if y bits are in parent   */

  /* Initialize values */
  ind_search_siblings = 1;
  sd_ptr = CCE_ccs(cc->cce);
  searched = sd_ptr;
  y_in_parent = 1;

  do
  {
    /* Get the sibling descriptor */
    index = (CCE_cptr(cc->cce) + sd_ptr) * 8;
    if(unlikely(!cc->dict[index / 0x800]))
      cc->dict[index / 0x800] = MADDR((cc->dictor + (index / 0x800) * 0x800) & ADDRESS_MAXWRAP(cc->regs), cc->r2, cc->regs, ACCTYPE_READ, cc->regs->psw.pkey);
    sd1 = &cc->dict[index / 0x800][index % 0x800];
    ITIMER_SYNC((cc->dictor + index) & ADDRESS_MAXWRAP(cc->regs), 8 - 1, cc->regs);

    /* If format-1, get second half from the expansion dictionary */
    if(cc->f1)
    {
      if(unlikely(!cc->edict[index / 0x800]))
        cc->edict[index / 0x800] = MADDR((cc->dictor + cc->dctsz + (index / 0x800) * 0x800) & ADDRESS_MAXWRAP(cc->regs), cc->r2, cc->regs, ACCTYPE_READ, cc->regs->psw.pkey);
      sd2 = &cc->edict[index / 0x800][index % 0x800];
      ITIMER_SYNC((cc->dictor + cc->dctsz + index) & ADDRESS_MAXWRAP(cc->regs), 8 - 1, cc->regs);

#ifdef OPTION_CMPSC_DEBUG
      /* Print before possible exception */
      logmsg("fetch_sd1: index %04X\n", CCE_cptr(cc->cce) + sd_ptr);
      cmpsc_print_sd(1, sd1, sd2);
#endif /* #ifdef OPTION_CMPSC_DEBUG */

      /* Check for data exception */
      if(unlikely(!SD1_sct(sd1)))
      {
        cc->regs->dxc = DXC_DECIMAL;
        ARCH_DEP(program_interrupt)((cc->regs), PGM_DATA_EXCEPTION);
      }
    }

#ifdef OPTION_CMPSC_DEBUG
    else
    {
      logmsg("fetch_sd0: index %04X\n", CCE_cptr(cc->cce) + sd_ptr);
      cmpsc_print_sd(0, sd1, sd2);
    }
#endif /* #ifdef OPTION_CMPSC_DEBUG */

    /* Check all children in sibling descriptor */
    scs = SD_scs(cc->f1, sd1);
    for(i = 0; i < scs; i++)
    {
      /* Stop searching when child tested and no consecutive child character */
      if(unlikely(!ind_search_siblings && !SD_ccc(cc->f1, sd1, sd2, i)))
        return(0);
      if(unlikely(*cc->src == SD_sc(cc->f1, sd1, sd2, i)))
      {
        /* Child is tested, so stop searching for siblings and no dead end */
        ind_search_siblings = 0;
        cc->deadend = 0;

        /* Check if child should not be examined */
        if(unlikely(!SD_ecb(cc->f1, sd1, i, cc->cce, y_in_parent)))
        {
          /* No need to examine child, found the last match */
          ADJUSTREGSC(cc, cc->r2, cc->regs, cc->iregs, 1);
          *is = CCE_cptr(cc->cce) + sd_ptr + i + 1;
          return(0);
        }

        /* Found a child get the character entry and check if additional extension characters match */
        ccce = ARCH_DEP(cmpsc_fetch_cce)(cc, CCE_cptr(cc->cce) + sd_ptr + i + 1);
        if(likely(!CCE_ecs(ccce) || !ARCH_DEP(cmpsc_test_ec)(cc, ccce)))
        {
          /* Set last match */
          ADJUSTREGSC(cc, cc->r2, cc->regs, cc->iregs, (U32) CCE_ecs(ccce) + 1);
          *is = CCE_cptr(cc->cce) + sd_ptr + i + 1;

#ifdef OPTION_CMPSC_DEBUG
          logmsg("search_sd: index %04X parent\n", *is);
#endif /* #ifdef OPTION_CMPSC_DEBUG */

          /* Found a matching child, make it parent and keep searching */
          cc->cce = ccce;
          return(1);
        }
      }
      BIT_set(cc->searchadm, 0, SD_sc(cc->f1, sd1, sd2, i));
    }

    /* Next sibling follows last possible child */
    sd_ptr += scs + 1;

    /* test for searching child 261 */
    searched += scs;
    if(unlikely(searched > 260))
    {
      cc->regs->dxc = DXC_DECIMAL;
      ARCH_DEP(program_interrupt)((cc->regs), PGM_DATA_EXCEPTION);
    }

    /* We get the next sibling descriptor, no y bits in parent for him */
    y_in_parent = 0;
  }
  while(ind_search_siblings && SD_msc(cc->f1, sd1));
  return(0);
}

/*----------------------------------------------------------------------------*/
/* cmpsc_store_is (index symbol)                                              */
/*----------------------------------------------------------------------------*/
static int ARCH_DEP(cmpsc_store_is)(struct cc *cc, U16 is)
{
  unsigned cbn;                        /* Compressed-data bit number          */
  U32 set_mask;                        /* Mask to set the bits                */
  BYTE work[3];                        /* Work bytes                          */

  /* Initialize values */
  cbn = GR1_cbn(cc->iregs);

  /* Can we write an index or interchange symbol */
  if(unlikely(GR_A(cc->r1 + 1, cc->iregs) < 3 && ((cbn + cc->smbsz - 1) / 8) >= GR_A(cc->r1 + 1, cc->iregs)))
  {
    cc->regs->psw.cc = 1;

#ifdef OPTION_CMPSC_DEBUG
    logmsg("store_is : end of output buffer\n");
#endif /* #ifdef OPTION_CMPSC_DEBUG */

    return(-1);
  }

  /* Check if symbol translation is requested */
  if(unlikely(cc->st))
  {
    /* Get the interchange symbol */
    ARCH_DEP(vfetchc)(work, 1, (cc->dictor + GR1_sttoff(cc->iregs) + is * 2) & ADDRESS_MAXWRAP(cc->regs), cc->r2, cc->regs);

#ifdef OPTION_CMPSC_DEBUG
    logmsg("store_is : %04X -> %02X%02X\n", is, work[0], work[1]);
#endif /* #ifdef OPTION_CMPSC_DEBUG */

    /* set index_symbol to interchange symbol */
    is = (work[0] << 8) + work[1];
  }

  /* Allign set mask */
  set_mask = ((U32) is) << (24 - cc->smbsz - cbn);

  /* Calculate first byte */
  if(likely(cbn))
  {
    work[0] = ARCH_DEP(vfetchb)(GR_A(cc->r1, cc->iregs) & ADDRESS_MAXWRAP(cc->regs), cc->r1, cc->regs);
    work[0] |= (set_mask >> 16) & 0xff;
  }
  else
    work[0] = (set_mask >> 16) & 0xff;

  /* Calculate second byte */
  work[1] = (set_mask >> 8) & 0xff;

  /* Calculate possible third byte and store */
  if(unlikely((cc->smbsz + cbn) > 16))
  {
    work[2] = set_mask & 0xff;
    ARCH_DEP(vstorec)(work, 2, GR_A(cc->r1, cc->iregs) & ADDRESS_MAXWRAP(cc->regs), cc->r1, cc->regs);
  }
  else
    ARCH_DEP(vstorec)(work, 1, GR_A(cc->r1, cc->iregs) & ADDRESS_MAXWRAP(cc->regs), cc->r1, cc->regs);

  /* Adjust destination registers */
  ADJUSTREGS(cc->r1, cc->regs, cc->iregs, (cbn + cc->smbsz) / 8);

  /* Calculate and set the new Compressed-data Bit Number */
  GR1_setcbn(cc->iregs, (cbn + cc->smbsz) % 8);

#ifdef OPTION_CMPSC_DEBUG
  logmsg("store_is : %04X, cbn=%d, GR%02d=" F_VADR ", GR%02d=" F_GREG "\n", is, GR1_cbn(cc->iregs), cc->r1, cc->iregs->GR(cc->r1), cc->r1 + 1, cc->iregs->GR(cc->r1 + 1));
#endif /* #ifdef OPTION_CMPSC_DEBUG */

  return(0);
}

/*----------------------------------------------------------------------------*/
/* cmpsc_store_iss (index symbols)                                            */
/*----------------------------------------------------------------------------*/
static void ARCH_DEP(cmpsc_store_iss)(struct cc *cc)
{
  GREG dictor;                         /* Dictionary origin                   */
  int i;
  U16 *is;                             /* Index symbol array                  */
  unsigned len1;                       /* Length in first page                */
  BYTE *main1;                         /* Address first page                  */
  BYTE mem[13];                        /* Build buffer                        */
  unsigned ofst;                       /* Offset within page                  */
  BYTE *sk;                            /* Storage key                         */

  /* Check if symbol translation is requested */
  if(unlikely(cc->st))
  {
    dictor = cc->dictor + GR1_sttoff(cc->iregs);
    for(i = 0; i < 8; i++)
    {
      /* Get the interchange symbol */
      ARCH_DEP(vfetchc)(mem, 1, (dictor + cc->is[i] * 2) & ADDRESS_MAXWRAP(cc->regs), cc->r2, cc->regs);

#ifdef OPTION_CMPSC_DEBUG
      logmsg("store_iss: %04X -> %02X%02X\n", cc->is[i], mem[0], mem[1]);
#endif /* #ifdef OPTION_CMPSC_DEBUG */

      /* set index_symbol to interchange symbol */
      cc->is[i] = (mem[0] << 8) + mem[1];
    }
  }

  /* Calculate buffer for 8 index symbols */
  is = cc->is;
  switch(cc->smbsz)
  {
    case 9: /* 9-bits */
    {
      /* 0       1       2       3       4       5       6       7       8        */
      /* 012345670123456701234567012345670123456701234567012345670123456701234567 */
      /* 012345678012345678012345678012345678012345678012345678012345678012345678 */
      /* 0        1        2        3        4        5        6        7         */
      mem[0] = (BYTE) (               (is[0] >> 1));
      mem[1] = (BYTE) ((is[0] << 7) | (is[1] >> 2));
      mem[2] = (BYTE) ((is[1] << 6) | (is[2] >> 3));
      mem[3] = (BYTE) ((is[2] << 5) | (is[3] >> 4));
      mem[4] = (BYTE) ((is[3] << 4) | (is[4] >> 5));
      mem[5] = (BYTE) ((is[4] << 3) | (is[5] >> 6));
      mem[6] = (BYTE) ((is[5] << 2) | (is[6] >> 7));
      mem[7] = (BYTE) ((is[6] << 1) | (is[7] >> 8));
      mem[8] = (BYTE) ((is[7])                    );
      break;
    }
    case 10: /* 10-bits */
    {
      /* 0       1       2       3       4       5       6       7       8       9        */
      /* 01234567012345670123456701234567012345670123456701234567012345670123456701234567 */
      /* 01234567890123456789012345678901234567890123456789012345678901234567890123456789 */
      /* 0         1         2         3         4         5         6         7          */
      mem[0] = (BYTE) (               (is[0] >> 2));
      mem[1] = (BYTE) ((is[0] << 6) | (is[1] >> 4));
      mem[2] = (BYTE) ((is[1] << 4) | (is[2] >> 6));
      mem[3] = (BYTE) ((is[2] << 2) | (is[3] >> 8));
      mem[4] = (BYTE) ((is[3])                    );
      mem[5] = (BYTE) (               (is[4] >> 2));
      mem[6] = (BYTE) ((is[4] << 6) | (is[5] >> 4));
      mem[7] = (BYTE) ((is[5] << 4) | (is[6] >> 6));
      mem[8] = (BYTE) ((is[6] << 2) | (is[7] >> 8));
      mem[9] = (BYTE) ((is[7])                    );
      break;
    }
    case 11: /* 11-bits */
    {
      /* 0       1       2       3       4       5       6       7       8       9       a        */
      /* 0123456701234567012345670123456701234567012345670123456701234567012345670123456701234567 */
      /* 0123456789a0123456789a0123456789a0123456789a0123456789a0123456789a0123456789a0123456789a */
      /* 0          1          2          3          4          5          6          7           */
      mem[ 0] = (BYTE) (               (is[0] >>  3));
      mem[ 1] = (BYTE) ((is[0] << 5) | (is[1] >>  6));
      mem[ 2] = (BYTE) ((is[1] << 2) | (is[2] >>  9));
      mem[ 3] = (BYTE) (               (is[2] >>  1));
      mem[ 4] = (BYTE) ((is[2] << 7) | (is[3] >>  4));
      mem[ 5] = (BYTE) ((is[3] << 4) | (is[4] >>  7));
      mem[ 6] = (BYTE) ((is[4] << 1) | (is[5] >> 10));
      mem[ 7] = (BYTE) (               (is[5] >>  2));
      mem[ 8] = (BYTE) ((is[5] << 6) | (is[6] >>  5));
      mem[ 9] = (BYTE) ((is[6] << 3) | (is[7] >>  8));
      mem[10] = (BYTE) ((is[7])                     );
      break;
    }
    case 12: /* 12-bits */
    {
      /* 0       1       2       3       4       5       6       7       8       9       a       b        */
      /* 012345670123456701234567012345670123456701234567012345670123456701234567012345670123456701234567 */
      /* 0123456789ab0123456789ab0123456789ab0123456789ab0123456789ab0123456789ab0123456789ab0123456789ab */
      /* 0           1           2           3           4           5           6           7            */
      mem[ 0] = (BYTE) (               (is[0] >> 4));
      mem[ 1] = (BYTE) ((is[0] << 4) | (is[1] >> 8));
      mem[ 2] = (BYTE) ((is[1])                    );
      mem[ 3] = (BYTE) (               (is[2] >> 4));
      mem[ 4] = (BYTE) ((is[2] << 4) | (is[3] >> 8));
      mem[ 5] = (BYTE) ((is[3])                    );
      mem[ 6] = (BYTE) (               (is[4] >> 4));
      mem[ 7] = (BYTE) ((is[4] << 4) | (is[5] >> 8));
      mem[ 8] = (BYTE) ((is[5])                    );
      mem[ 9] = (BYTE) (               (is[6] >> 4));
      mem[10] = (BYTE) ((is[6] << 4) | (is[7] >> 8));
      mem[11] = (BYTE) ((is[7])                    );
      break;
    }
    case 13: /* 13-bits */
    {
      /* 0       1       2       3       4       5       6       7       8       9       a       b       c        */
      /* 01234567012345670123456701234567012345670123456701234567012345670123456701234567012345670123456701234567 */
      /* 0123456789abc0123456789abc0123456789abc0123456789abc0123456789abc0123456789abc0123456789abc0123456789abc */
      /* 0            1            2            3            4            5            6            7             */
      mem[ 0] = (BYTE) (               (is[0] >>  5));
      mem[ 1] = (BYTE) ((is[0] << 3) | (is[1] >> 10));
      mem[ 2] = (BYTE) (               (is[1] >>  2));
      mem[ 3] = (BYTE) ((is[1] << 6) | (is[2] >>  7));
      mem[ 4] = (BYTE) ((is[2] << 1) | (is[3] >> 12));
      mem[ 5] = (BYTE) (               (is[3] >>  4));
      mem[ 6] = (BYTE) ((is[3] << 4) | (is[4] >>  9));
      mem[ 7] = (BYTE) (               (is[4] >>  1));
      mem[ 8] = (BYTE) ((is[4] << 7) | (is[5] >>  6));
      mem[ 9] = (BYTE) ((is[5] << 2) | (is[6] >> 11));
      mem[10] = (BYTE) (               (is[6] >>  3));
      mem[11] = (BYTE) ((is[6] << 5) | (is[7] >>  8));
      mem[12] = (BYTE) ((is[7])                     );
      break;
    }
  }

  /* Fingers crossed that we stay within one page */
  ofst = GR_A(cc->r1, cc->iregs) & 0x7ff;
  if(likely(ofst + cc->smbsz <= 0x800))
  {
    if(unlikely(!cc->dest))
      cc->dest = MADDR((GR_A(cc->r1, cc->iregs) & ~0x7ff) & ADDRESS_MAXWRAP(cc->regs), cc->r1, cc->regs, ACCTYPE_WRITE, cc->regs->psw.pkey);
    memcpy(&cc->dest[ofst], mem, cc->smbsz);
    ITIMER_UPDATE(GR_A(cc->r1, cc->iregs) & ADDRESS_MAXWRAP(cc->regs), cc->smbsz - 1, cc->regs);

    /* Perfect fit? */
    if(unlikely(ofst + cc->smbsz == 0x800))
      cc->dest = NULL;
  }
  else
  {
    /* We need 2 pages */
    if(unlikely(!cc->dest))
      main1 = MADDR((GR_A(cc->r1, cc->iregs) & ~0x7ff) & ADDRESS_MAXWRAP(cc->regs), cc->r1, cc->regs, ACCTYPE_WRITE_SKP, cc->regs->psw.pkey);
    else
      main1 = cc->dest;
    sk = cc->regs->dat.storkey;
    len1 = 0x800 - ofst;
    cc->dest = MADDR((GR_A(cc->r1, cc->iregs) + len1) & ADDRESS_MAXWRAP(cc->regs), cc->r1, cc->regs, ACCTYPE_WRITE, cc->regs->psw.pkey);
    memcpy(&main1[ofst], mem, len1);
    memcpy(cc->dest, &mem[len1], cc->smbsz - len1);
    *sk |= (STORKEY_REF | STORKEY_CHANGE);
  }
  ADJUSTREGS(cc->r1, cc->regs, cc->iregs, cc->smbsz);

#ifdef OPTION_CMPSC_DEBUG
  logmsg("store_iss:");
  for(i = 0; i < 8; i++)
    logmsg(" %04X", cc->is[i]);
  logmsg(", GR%02d=" F_VADR ", GR%02d=" F_GREG "\n", cc->r1, cc->iregs->GR(cc->r1), cc->r1 + 1, cc->iregs->GR(cc->r1 + 1));
#endif /* #ifdef OPTION_CMPSC_DEBUG */

}

/*----------------------------------------------------------------------------*/
/* cmpsc_test_ec (extension characters)                                       */
/*----------------------------------------------------------------------------*/
static int ARCH_DEP(cmpsc_test_ec)(struct cc *cc, BYTE *cce)
{
  BYTE buf[4];                         /* Cross page buffer                   */
  BYTE *src;                           /* Source pointer                      */

  /* No dead end */
  cc->deadend = 0;

  /* Get address of source */
  if(likely(cc->srclen > (unsigned) CCE_ecs(cce)))
  {
    src = &cc->src[1];
    ITIMER_SYNC((GR_A(cc->r2, cc->iregs) + 1) & ADDRESS_MAXWRAP(cc->regs), CCE_ecs(cce) - 1, cc->regs);
  }
  else
  {
    /* Return nomatch on end of source condition */
    if(unlikely(GR_A(cc->r2 + 1, cc->iregs) <= (unsigned) CCE_ecs(cce)))
      return(1);

    /* Get the cross page data */
    ARCH_DEP(vfetchc)(buf, CCE_ecs(cce) - 1, (GR_A(cc->r2, cc->iregs) + 1) & ADDRESS_MAXWRAP(cc->regs), cc->r2, cc->regs);
    src = buf;
  }

  /* Return results compare */
  return(memcmp(src, &CCE_ec(cce, 0), CCE_ecs(cce)));
}

/*============================================================================*/
/* Expand                                                                     */
/*============================================================================*/

/*----------------------------------------------------------------------------*/
/* Expansion Character Entry macro's (ECE)                                    */
/*----------------------------------------------------------------------------*/
/* bit34 : indication of bits 3 and 4 (what else ;-)                          */
/* csl   : complete symbol length                                             */
/* ofst  : offset from current position in output area                        */
/* pptr  : predecessor pointer                                                */
/* psl   : partial symbol length                                              */
/*----------------------------------------------------------------------------*/
#define ECE_bit34(ece)       ((ece)[0] & 0x18)
#define ECE_csl(ece)         ((ece)[0] & 0x07)
#define ECE_ofst(ece)        ((ece)[7])
#define ECE_pptr(ece)        ((((ece)[0] & 0x1f) << 8) | (ece)[1])
#define ECE_psl(ece)         ((ece)[0] >> 5)

/*----------------------------------------------------------------------------*/
/* cmpsc_expand                                                               */
/*----------------------------------------------------------------------------*/
static void ARCH_DEP(cmpsc_expand)(int r1, int r2, REGS *regs, REGS *iregs)
{
  GREG destlen;                        /* Destination length                  */
  struct ec ec;                        /* Expand cache                        */
  int i;                               /* Index                               */
  U16 iss[8] = {0};                    /* Index symbols                       */

  /* Initialize values */
  destlen = GR_A(r1 + 1, iregs);

  /* Initialize expansion context */
  ec.dest = NULL;
  ec.dictor = GR1_dictor(iregs);
  memset(ec.dict, 0, sizeof(ec.dict));

  /* Initialize expanded index symbol cache and prefill with alphabet entries */
  memset(ec.ecl, 0, sizeof(ec.ecl));
  for(i = 0; i < 256; i++)             /* Alphabet entries                    */
  {
    ec.ec[i] = i;
    ec.eci[i] = i;
    ec.ecl[i] = 1;
  }
  ec.ecwm = 256;                       /* Set watermark after alphabet part   */

  ec.iregs = iregs;
  ec.r1 = r1;
  ec.r2 = r2;
  ec.regs = regs;
  ec.smbsz = GR0_smbsz(regs);
  ec.src = NULL;

#ifdef OPTION_CMPSC_DEBUG
  ec.dbgac = 0;
  ec.dbgbi = 0;
  ec.dbgbo = 0;
  ec.dbgch = 0;
  ec.dbgiss = 0;
#endif /* #ifdef OPTION_CMPSC_DEBUG */

  /*--------------------------------------------------------------------------*/
  /* Process individual index symbols until cbn becomes zero                  */
  while(unlikely(GR1_cbn(ec.iregs)))
  {
    if(unlikely(ARCH_DEP(cmpsc_expand_single_is)(&ec)))
      return;
  }

  /*--------------------------------------------------------------------------*/
  /* Block processing, cbn stays zero                                         */
  while(likely(GR_A(ec.r2 + 1, ec.iregs) >= ec.smbsz))
  {
    ARCH_DEP(cmpsc_fetch_iss)(&ec, iss);
    ec.ocl = 0;                        /* Initialize output cache             */
    for(i = 0; i < 8; i++)
    {

#ifdef OPTION_CMPSC_DEBUG
      logmsg("expand   : is %04X (%d)\n", iss[i], i);
      ec.dbgiss++;
#endif /* #ifdef OPTION_CMPSC_DEBUG */

      if(unlikely(!ec.ecl[iss[i]]))
        ARCH_DEP(cmpsc_expand_is)(&ec, iss[i]);
      else
      {
        memcpy(&ec.oc[ec.ocl], &ec.ec[ec.eci[iss[i]]], ec.ecl[iss[i]]);
        ec.ocl += ec.ecl[iss[i]];

#ifdef OPTION_CMPSC_DEBUG
        if(iss[i] < 0x100)
          ec.dbgac++;
        else
          ec.dbgch++;
#endif /* #ifdef OPTION_CMPSC_DEBUG */

      }
    }

#ifdef OPTION_CMPSC_DEBUG
    ec.dbgbi += ec.smbsz;
    ec.dbgbo += ec.ocl;
    logmsg("Stats: iss %6u; ach %6u: %3d%; hts %6u: %3d%; bin %6u, out %6u: %6d%\n", ec.dbgiss, ec.dbgac, ec.dbgac * 100 / ec.dbgiss, ec.dbgch, ec.dbgch * 100 / ec.dbgiss, ec.dbgbi, ec.dbgbo, ec.dbgbo * 100 / ec.dbgbi);
#endif /* #ifdef OPTION_CMPSC_DEBUG */

    /* Write and commit, cbn unchanged, so no commit for GR1 needed */
    if(unlikely(ARCH_DEP(cmpsc_vstore)(&ec, ec.oc, ec.ocl)))
      return;

    /* Commit registers */
    COMMITREGS2(ec.regs, ec.iregs, ec.r1, ec.r2);

    /* Return with cc3 on interrupt pending */
    if(unlikely(destlen - GR_A(ec.r1 + 1, ec.iregs) >= MINPROC_SIZE && INTERRUPT_PENDING(ec.regs)))
    {

#ifdef OPTION_CMPSC_DEBUG
      logmsg("Interrupt pending, commit and return with cc3\n");
#endif /* #ifdef OPTION_CMPSC_DEBUG */

      ec.regs->psw.cc = 3;
      return;
    }
  }

  /*--------------------------------------------------------------------------*/
  /* Process individual index symbols until end of source (or destination)    */
  while(likely(!ARCH_DEP(cmpsc_expand_single_is)(&ec)));
}

/*----------------------------------------------------------------------------*/
/* cmpsc_expand_is (index symbol)                                             */
/*----------------------------------------------------------------------------*/
static void ARCH_DEP(cmpsc_expand_is)(struct ec *ec, U16 is)
{
  int csl;                             /* Complete symbol length              */
  unsigned cw;                         /* Characters written                  */
  BYTE *ece;                           /* Expansion Character Entry           */
  U16 index;                           /* Index within dictionary             */
  int psl;                             /* Partial symbol length               */

  /* Initialize values */
  cw = 0;

  /* Get expansion character entry */
  index = is * 8;
  if(unlikely(!ec->dict[index / 0x800]))
    ec->dict[index / 0x800] = MADDR((ec->dictor + (index / 0x800) * 0x800) & ADDRESS_MAXWRAP(ec->regs), ec->r2, ec->regs, ACCTYPE_READ, ec->regs->psw.pkey);
  ece = &ec->dict[index / 0x800][index % 0x800];
  ITIMER_SYNC((ec->dictor + index) & ADDRESS_MAXWRAP(ec->regs), 8 - 1, ec->regs);

#ifdef OPTION_CMPSC_DEBUG
  logmsg("fetch_ece: index %04X\n", is);
  cmpsc_print_ece(ece);
#endif /* #ifdef OPTION_CMPSC_DEBUG */

  /* Process preceded entries */
  psl = ECE_psl(ece);
  while(likely(psl))
  {
    /* Count and check for writing child 261 and check valid psl */
    cw += psl;
    if(unlikely(cw > 260 || psl > 5))
    {
      ec->regs->dxc = DXC_DECIMAL;
      ARCH_DEP(program_interrupt)((ec->regs), PGM_DATA_EXCEPTION);
    }

    /* Process extension characters in preceded entry */
    memcpy(&ec->oc[ec->ocl + ECE_ofst(ece)], &ece[2], psl);

    /* Get preceding entry */
    index = ECE_pptr(ece) * 8;
    if(unlikely(!ec->dict[index / 0x800]))
      ec->dict[index / 0x800] = MADDR((ec->dictor + (index / 0x800) * 0x800) & ADDRESS_MAXWRAP(ec->regs), ec->r2, ec->regs, ACCTYPE_READ, ec->regs->psw.pkey);
    ece = &ec->dict[index / 0x800][index % 0x800];
    ITIMER_SYNC((ec->dictor + index) & ADDRESS_MAXWRAP(ec->regs), 8 - 1, ec->regs);

#ifdef OPTION_CMPSC_DEBUG
    logmsg("fetch_ece: index %04X\n", index / 8);
    cmpsc_print_ece(ece);
#endif /* #ifdef OPTION_CMPSC_DEBUG */

    /* Calculate partial symbol length */
    psl = ECE_psl(ece);
  }

  /* Count and check for writing child 261, valid csl and invalid bits */
  csl = ECE_csl(ece);
  cw += csl;
  if(unlikely(cw > 260 || !csl || ECE_bit34(ece)))
  {
    ec->regs->dxc = DXC_DECIMAL;
    ARCH_DEP(program_interrupt)((ec->regs), PGM_DATA_EXCEPTION);
  }

  /* Process extension characters in unpreceded entry */
  memcpy(&ec->oc[ec->ocl], &ece[1], csl);

  /* Place within cache */
  memcpy(&ec->ec[ec->ecwm], &ec->oc[ec->ocl], cw);
  ec->eci[is] = ec->ecwm;
  ec->ecl[is] = cw;
  ec->ecwm += cw;

  /* Commit in output buffer */
  ec->ocl += cw;
}

/*----------------------------------------------------------------------------*/
/* cmpsc_expand_single_is (index symbol)                                      */
/*----------------------------------------------------------------------------*/
static int ARCH_DEP(cmpsc_expand_single_is)(struct ec *ec)
{
  U16 is;                              /* Index symbol                        */

  if(unlikely(ARCH_DEP(cmpsc_fetch_is)(ec, &is)))
    return(-1);
  if(!ec->ecl[is])
  {
    ec->ocl = 0;                       /* Initialize output cache             */
    ARCH_DEP(cmpsc_expand_is)(ec, is);
    if(unlikely(ARCH_DEP(cmpsc_vstore)(ec, ec->oc, ec->ocl)))
      return(-1);
  }
  else
  {
    if(unlikely(ARCH_DEP(cmpsc_vstore)(ec, &ec->ec[ec->eci[is]], ec->ecl[is])))
      return(-1);
  }

  /* Commit, including GR1 */
  COMMITREGS(ec->regs, ec->iregs, ec->r1, ec->r2);
  return(0);
}

/*----------------------------------------------------------------------------*/
/* cmpsc_fetch_is (index symbol)                                              */
/*----------------------------------------------------------------------------*/
static int ARCH_DEP(cmpsc_fetch_is)(struct ec *ec, U16 *is)
{
  unsigned cbn;                        /* Compressed-data bit number          */
  U32 mask;                            /* Working mask                        */
  BYTE work[3];                        /* Working field                       */

  /* Initialize values */
  cbn = GR1_cbn(ec->iregs);

  /* Check if we can read an index symbol */
  if(unlikely(GR_A(ec->r2 + 1, ec->iregs) < 3 && ((cbn + ec->smbsz - 1) / 8) >= GR_A(ec->r2 + 1, ec->iregs)))
  {

#ifdef OPTION_CMPSC_DEBUG
    logmsg("fetch_is : reached end of source\n");
#endif /* #ifdef OPTION_CMPSC_DEBUG */

    ec->regs->psw.cc = 0;
    return(-1);
  }

  /* Clear possible fetched 3rd byte */
  work[2] = 0;
  ARCH_DEP(vfetchc)(&work, (ec->smbsz + cbn - 1) / 8, GR_A(ec->r2, ec->iregs) & ADDRESS_MAXWRAP(ec->regs), ec->r2, ec->regs);

  /* Get the bits */
  mask = work[0] << 16 | work[1] << 8 | work[2];
  mask >>= (24 - ec->smbsz - cbn);
  mask &= 0xFFFF >> (16 - ec->smbsz);
  *is = mask;

  /* Adjust source registers */
  ADJUSTREGS(ec->r2, ec->regs, ec->iregs, (cbn + ec->smbsz) / 8);

  /* Calculate and set the new compressed-data bit number */
  GR1_setcbn(ec->iregs, (cbn + ec->smbsz) % 8);

#ifdef OPTION_CMPSC_DEBUG
  logmsg("fetch_is : %04X, cbn=%d, GR%02d=" F_VADR ", GR%02d=" F_GREG "\n", *is, GR1_cbn(ec->iregs), ec->r2, ec->iregs->GR(ec->r2), ec->r2 + 1, ec->iregs->GR(ec->r2 + 1));
#endif /* #ifdef OPTION_CMPSC_DEBUG */

  return(0);
}

/*----------------------------------------------------------------------------*/
/* cmpsc_fetch_iss                                                            */
/*----------------------------------------------------------------------------*/
static void ARCH_DEP(cmpsc_fetch_iss)(struct ec *ec, U16 is[8])
{
  BYTE buf[13];                        /* Buffer for Index Symbols            */

#ifdef OPTION_CMPSC_DEBUG
  int i;
#endif /* #ifdef OPTION_CMPSC_DEBUG */

  unsigned len1;                       /* Lenght in first page                */
  BYTE *mem;                           /* Pointer to maddr or buf             */
  unsigned ofst;                       /* Offset in first page                */

  /* Fingers crossed that we stay within one page */
  ofst = GR_A(ec->r2, ec->iregs) & 0x7ff;
  if(unlikely(!ec->src))
    ec->src = MADDR((GR_A(ec->r2, ec->iregs) & ~0x7ff) & ADDRESS_MAXWRAP(ec->regs), ec->r2, ec->regs, ACCTYPE_READ, ec->regs->psw.pkey);
  if(likely(ofst + ec->smbsz <= 0x800))
  {
    ITIMER_SYNC(GR_A(ec->r2, ec->iregs) & ADDRESS_MAXWRAP(ec->regs), ec->smbsz - 1, ec->regs);
    mem = &ec->src[ofst];

    /* Perfect fit? */
    if(unlikely(ofst + ec->smbsz == 0x800))
      ec->src = NULL;
  }
  else
  {
    /* We need data spread over 2 pages */
    len1 = 0x800 - ofst;
    memcpy(buf, &ec->src[ofst], len1);
    ec->src = MADDR((GR_A(ec->r2, ec->iregs) + len1) & ADDRESS_MAXWRAP(ec->regs), ec->r2, ec->regs, ACCTYPE_READ, ec->regs->psw.pkey);
    memcpy(&buf[len1], ec->src, ec->smbsz - len1);
    mem = buf;
  }

  /* Calculate the 8 index symbols */
  switch(ec->smbsz)
  {
    case 9: /* 9-bits */
    {
      /* 0       1       2       3       4       5       6       7       8        */
      /* 012345670123456701234567012345670123456701234567012345670123456701234567 */
      /* 012345678012345678012345678012345678012345678012345678012345678012345678 */
      /* 0        1        2        3        4        5        6        7         */
      is[0] = ((mem[0] << 1) | (mem[1] >> 7));
      is[1] = ((mem[1] << 2) | (mem[2] >> 6)) & 0x01ff;
      is[2] = ((mem[2] << 3) | (mem[3] >> 5)) & 0x01ff;
      is[3] = ((mem[3] << 4) | (mem[4] >> 4)) & 0x01ff;
      is[4] = ((mem[4] << 5) | (mem[5] >> 3)) & 0x01ff;
      is[5] = ((mem[5] << 6) | (mem[6] >> 2)) & 0x01ff;
      is[6] = ((mem[6] << 7) | (mem[7] >> 1)) & 0x01ff;
      is[7] = ((mem[7] << 8) | (mem[8]     )) & 0x01ff;
      break;
    }
    case 10: /* 10-bits */
    {
      /* 0       1       2       3       4       5       6       7       8       9        */
      /* 01234567012345670123456701234567012345670123456701234567012345670123456701234567 */
      /* 01234567890123456789012345678901234567890123456789012345678901234567890123456789 */
      /* 0         1         2         3         4         5         6         7          */
      is[0] = ((mem[0] << 2) | (mem[1] >> 6));
      is[1] = ((mem[1] << 4) | (mem[2] >> 4)) & 0x03ff;
      is[2] = ((mem[2] << 6) | (mem[3] >> 2)) & 0x03ff;
      is[3] = ((mem[3] << 8) | (mem[4]     )) & 0x03ff;
      is[4] = ((mem[5] << 2) | (mem[6] >> 6));
      is[5] = ((mem[6] << 4) | (mem[7] >> 4)) & 0x03ff;
      is[6] = ((mem[7] << 6) | (mem[8] >> 2)) & 0x03ff;
      is[7] = ((mem[8] << 8) | (mem[9]     )) & 0x03ff;
      break;
    }
    case 11: /* 11-bits */
    {
      /* 0       1       2       3       4       5       6       7       8       9       a        */
      /* 0123456701234567012345670123456701234567012345670123456701234567012345670123456701234567 */
      /* 0123456789a0123456789a0123456789a0123456789a0123456789a0123456789a0123456789a0123456789a */
      /* 0          1          2          3          4          5          6          7           */
      is[0] = ((mem[0] <<  3) | (mem[ 1] >> 5)                );
      is[1] = ((mem[1] <<  6) | (mem[ 2] >> 2)                ) & 0x07ff;
      is[2] = ((mem[2] <<  9) | (mem[ 3] << 1) | (mem[4] >> 7)) & 0x07ff;
      is[3] = ((mem[4] <<  4) | (mem[ 5] >> 4)                ) & 0x07ff;
      is[4] = ((mem[5] <<  7) | (mem[ 6] >> 1)                ) & 0x07ff;
      is[5] = ((mem[6] << 10) | (mem[ 7] << 2) | (mem[8] >> 6)) & 0x07ff;
      is[6] = ((mem[8] <<  5) | (mem[ 9] >> 3)                ) & 0x07ff;
      is[7] = ((mem[9] <<  8) | (mem[10]     )                ) & 0x07ff;
      break;
    }
    case 12: /* 12-bits */
    {
      /* 0       1       2       3       4       5       6       7       8       9       a       b        */
      /* 012345670123456701234567012345670123456701234567012345670123456701234567012345670123456701234567 */
      /* 0123456789ab0123456789ab0123456789ab0123456789ab0123456789ab0123456789ab0123456789ab0123456789ab */
      /* 0           1           2           3           4           5           6           7            */
      is[0] = ((mem[ 0] << 4) | (mem[ 1] >> 4));
      is[1] = ((mem[ 1] << 8) | (mem[ 2]     )) & 0x0fff;
      is[2] = ((mem[ 3] << 4) | (mem[ 4] >> 4));
      is[3] = ((mem[ 4] << 8) | (mem[ 5]     )) & 0x0fff;
      is[4] = ((mem[ 6] << 4) | (mem[ 7] >> 4));
      is[5] = ((mem[ 7] << 8) | (mem[ 8]     )) & 0x0fff;
      is[6] = ((mem[ 9] << 4) | (mem[10] >> 4));
      is[7] = ((mem[10] << 8) | (mem[11]     )) & 0x0fff;
      break;
    }
    case 13: /* 13-bits */
    {
      /* 0       1       2       3       4       5       6       7       8       9       a       b       c        */
      /* 01234567012345670123456701234567012345670123456701234567012345670123456701234567012345670123456701234567 */
      /* 0123456789abc0123456789abc0123456789abc0123456789abc0123456789abc0123456789abc0123456789abc0123456789abc */
      /* 0            1            2            3            4            5            6            7             */
      is[0] = ((mem[ 0] <<  5) | (mem[ 1] >> 3)                 );
      is[1] = ((mem[ 1] << 10) | (mem[ 2] << 2) | (mem[ 3] >> 6)) & 0x1fff;
      is[2] = ((mem[ 3] <<  7) | (mem[ 4] >> 1)                 ) & 0x1fff;
      is[3] = ((mem[ 4] << 12) | (mem[ 5] << 4) | (mem[ 6] >> 4)) & 0x1fff;
      is[4] = ((mem[ 6] <<  9) | (mem[ 7] << 1) | (mem[ 8] >> 7)) & 0x1fff;
      is[5] = ((mem[ 8] <<  6) | (mem[ 9] >> 2)                 ) & 0x1fff;
      is[6] = ((mem[ 9] << 11) | (mem[10] << 3) | (mem[11] >> 5)) & 0x1fff;
      is[7] = ((mem[11] <<  8) | (mem[12]     )                 ) & 0x1fff;
      break;
    }
  }

  /* Adjust source registers */
  ADJUSTREGS(ec->r2, ec->regs, ec->iregs, ec->smbsz);

#ifdef OPTION_CMPSC_DEBUG
  logmsg("fetch_iss:");
  for(i = 0; i < 8; i++)
    logmsg(" %04X", is[i]);
  logmsg(", GR%02d=" F_VADR ", GR%02d=" F_GREG "\n", ec->r2, ec->iregs->GR(ec->r2), ec->r2 + 1, ec->iregs->GR(ec->r2 + 1));
#endif /* #ifdef OPTION_CMPSC_DEBUG */
}

#ifndef NO_2ND_COMPILE
#ifdef OPTION_CMPSC_DEBUG
/*----------------------------------------------------------------------------*/
/* cmpsc_print_ece (expansion character entry)                                */
/*----------------------------------------------------------------------------*/
static void cmpsc_print_ece(BYTE *ece)
{
  int i;                               /* Index                               */
  int prt_detail;                      /* Switch detailed printing            */

  logmsg("  ece    : ");
  prt_detail = 0;
  for(i = 0; i < 8; i++)
  {
    if(!prt_detail && ece[i])
      prt_detail = 1;
    logmsg("%02X", ece[i]);
  }
  logmsg("\n");
  if(prt_detail)
  {
    if(ECE_psl(ece))
    {
      logmsg("  psl    : %d\n", ECE_psl(ece));
      logmsg("  pptr   : %04X\n", ECE_pptr(ece));
      logmsg("  ecs    :");
      for(i = 0; i < ECE_psl(ece); i++)
        logmsg(" %02X", ece[i + 2]);
      logmsg("\n");
      logmsg("  ofst   : %02X\n", ECE_ofst(ece));
    }
    else
    {
      logmsg("  psl    : %d\n", ECE_psl(ece));
      logmsg("  bit34  : %s\n", TRUEFALSE(ECE_bit34(ece)));
      logmsg("  csl    : %d\n", ECE_csl(ece));
      logmsg("  ecs    :");
      for(i = 0; i < ECE_csl(ece); i++)
        logmsg(" %02X", ece[i + 1]);
      logmsg("\n");
    }
  }
}
#endif /* #ifdef OPTION_CMPSC_DEBUG */
#endif /* #ifndef NO_2ND_COMPILE */

/*----------------------------------------------------------------------------*/
/* cmpsc_vstore                                                               */
/*----------------------------------------------------------------------------*/
static int ARCH_DEP(cmpsc_vstore)(struct ec *ec, BYTE *buf, unsigned len)
{
  unsigned len1;                       /* Length in first page                */
  unsigned len2;                       /* Length in second page               */
  BYTE *main1;                         /* Address first page                  */
  unsigned ofst;                       /* Offset within page                  */
  BYTE *sk;                            /* Storage key                         */

#ifdef OPTION_CMPSC_DEBUG
  unsigned i;                          /* Index                               */
  unsigned j;                          /* Index                               */
  static BYTE pbuf[2060];              /* Print buffer                        */
  static unsigned plen = 2061;         /* Impossible value                    */
#endif /* #ifdef OPTION_CMPSC_DEBUG */

  /* Check destination size */
  if(unlikely(GR_A(ec->r1 + 1, ec->iregs) < len))
  {

#ifdef OPTION_CMPSC_DEBUG
    logmsg("vstore   : Reached end of destination\n");
#endif /* #ifdef OPTION_CMPSC_DEBUG */

    /* Indicate end of destination */
    ec->regs->psw.cc = 1;
    return(-1);
  }

#ifdef OPTION_CMPSC_DEBUG
  if(plen == len && !memcmp(pbuf, buf, plen))
    logmsg(F_GREG " - " F_GREG " Same buffer as previously shown\n", ec->iregs->GR(ec->r1), ec->iregs->GR(ec->r1) + len - 1);
  else
  {
    for(i = 0; i < len; i += 32)
    {
      logmsg(F_GREG, ec->iregs->GR(ec->r1) + i);
      if(i && i + 32 < len && !memcmp(&buf[i], &buf[i - 32], 32))
      {
        for(j = i + 32; j + 32 < len && !memcmp(&buf[j], &buf[j - 32], 32); j += 32);
        if(j > 32)
        {
          logmsg(":  Same line as above\n" F_GREG, ec->iregs->GR(ec->r1) + j);
          i = j;
        }
      }
      logmsg(": ");
      for(j = 0; j < 32; j++)
      {
        if(!(j % 8))
          logmsg(" ");
        if(i + j >= len)
          logmsg("  ");
        else
          logmsg("%02X", buf[i + j]);
      }
      logmsg(" | ");
      for(j = 0; j < 32; j++)
      {
        if(i + j >= len)
          logmsg(" ");
        else
        {
          if(isprint(guest_to_host(buf[i + j])))
            logmsg("%c", guest_to_host(buf[i + j]));
          else
            logmsg(".");
        }
      }
      logmsg(" |\n");
    }
    memcpy(pbuf, buf, len);
    plen = len;
  }
#endif /* #ifdef OPTION_CMPSC_DEBUG */

  /* Fingers crossed that we stay within one page */
  ofst = GR_A(ec->r1, ec->iregs) & 0x7ff;
  if(likely(ofst + len <= 0x800))
  {
    if(unlikely(!ec->dest))
      ec->dest = MADDR((GR_A(ec->r1, ec->iregs) & ~0x7ff) & ADDRESS_MAXWRAP(ec->regs), ec->r1, ec->regs, ACCTYPE_WRITE, ec->regs->psw.pkey);
    memcpy(&ec->dest[ofst], buf, len);
    ITIMER_UPDATE(GR_A(ec->r1, ec->iregs) & ADDRESS_MAXWRAP(ec->regs), len - 1, ec->regs);

    /* Perfect fit? */
    if(unlikely(ofst + len == 0x800))
      ec->dest = NULL;
  }
  else
  {
    /* We need multiple pages */
    if(unlikely(!ec->dest))
      main1 = MADDR((GR_A(ec->r1, ec->iregs) & ~0x7ff) & ADDRESS_MAXWRAP(ec->regs), ec->r1, ec->regs, ACCTYPE_WRITE_SKP, ec->regs->psw.pkey);
    else
      main1 = ec->dest;
    sk = ec->regs->dat.storkey;
    len1 = 0x800 - ofst;
    ec->dest = MADDR((GR_A(ec->r1, ec->iregs) + len1) & ADDRESS_MAXWRAP(ec->regs), ec->r1, ec->regs, ACCTYPE_WRITE, ec->regs->psw.pkey);
    memcpy(&main1[ofst], buf, len1);
    len2 = len - len1; /* We always start with a len2 */
    do
    {
      memcpy(ec->dest, &buf[len1], (len2 > 0x800 ? 0x800 : len2));
      *sk |= (STORKEY_REF | STORKEY_CHANGE);
      if(unlikely(len2 >= 0x800))
      {
        len1 += 0x800;
        len2 -= 0x800;

        /* Perfect fit? */
        if(unlikely(!len2))
          ec->dest = NULL;
        else
          ec->dest = MADDR((GR_A(ec->r1, ec->iregs) + len1) & ADDRESS_MAXWRAP(ec->regs), ec->r1, ec->regs, ACCTYPE_WRITE, ec->regs->psw.pkey);
        sk = ec->regs->dat.storkey;
      }
      else
        len2 = 0;
    }
    while(len2);
  }

  ADJUSTREGS(ec->r1, ec->regs, ec->iregs, len);
  return(0);
}

#define NO_2ND_COMPILE
#endif /* FEATURE_COMPRESSION */

#ifndef _GEN_ARCH
  #ifdef _ARCHMODE2
    #define _GEN_ARCH _ARCHMODE2
    #include "cmpsc.c"
  #endif /* #ifdef _ARCHMODE2 */
  #ifdef _ARCHMODE3
    #undef _GEN_ARCH
    #define _GEN_ARCH _ARCHMODE3
    #include "cmpsc.c"
  #endif /* #ifdef _ARCHMODE3 */


#if !defined( NOT_HERC )        // (building Hercules?)

HDL_DEPENDENCY_SECTION;
{
    HDL_DEPENDENCY( HERCULES );
    HDL_DEPENDENCY( REGS );
}
END_DEPENDENCY_SECTION;

HDL_INSTRUCTION_SECTION;
{
    char info[256];
    const char* fn = NULL;
    if (!fn) fn = strrchr( __FILE__, '\\' );
    if (!fn) fn = strrchr( __FILE__ , '/' );
    if (!fn) fn =          __FILE__        ; else fn++;
#ifdef __TIMESTAMP__
    MSGBUF( info, "%s version %s last updated on %s", fn, VERSION, __TIMESTAMP__ );
#else
    MSGBUF( info, "%s version %s compiled on %s at %s", fn, VERSION, __DATE__, __TIME__ );
#endif
    WRMSG( HHC01417, "I", info );

    HDL_DEFINST( HDL_INSTARCH_390 | HDL_INSTARCH_900, 0xB263, legacy_cmpsc );
}
END_INSTRUCTION_SECTION;

#endif // !defined( NOT_HERC )  // (building Hercules?)

#endif /* #ifndef _GEN_ARCH */
