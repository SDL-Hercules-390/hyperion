/* OPCODE.H     (c) Copyright Jan Jaeger, 2000-2012                  */
/*              Instruction decoding macros and prototypes           */

/* Interpretive Execution - (c) Copyright Jan Jaeger, 1999-2012      */
/* z/Architecture support - (c) Copyright Jan Jaeger, 1999-2012      */

#ifndef _OPCODE_H
#define _OPCODE_H

#include "hercules.h"

#ifndef _CPU_C_
 #ifndef _HENGINE_DLL_
  #define CPU_DLL_IMPORT DLL_IMPORT
 #else   /* _HENGINE_DLL_ */
  #define CPU_DLL_IMPORT extern
 #endif  /* _HENGINE_DLL_ */
#else   /* _CPU_C_ */
 #define CPU_DLL_IMPORT DLL_EXPORT
#endif /* _CPU_C_ */

#ifndef _OPCODE_C_
 #ifndef _HENGINE_DLL_
  #define OPC_DLL_IMPORT DLL_IMPORT
 #else   /* _HENGINE_DLL_ */
  #define OPC_DLL_IMPORT extern
 #endif  /* _HENGINE_DLL_ */
#else   /* _OPCODE_C_ */
 #define OPC_DLL_IMPORT DLL_EXPORT
#endif /* _OPCODE_C_ */

/* Define the following as DLL_EXPORT from */
/* within files inside the engine which    */
/* define instruction functions            */
#undef  DEF_INST_EXPORT
#define DEF_INST_EXPORT

#if defined(_370)
 #define _GEN370(_name) &s370_ ## _name,
#else
 #define _GEN370(_name)
#endif

#if defined(_390)
 #define _GEN390(_name) &s390_ ## _name,
#else
 #define _GEN390(_name)
#endif

#if defined(_900)
 #define _GEN900(_name) &z900_ ## _name,
#else
 #define _GEN900(_name)
#endif


#define GENx___x___x___ \
    { \
    _GEN370(operation_exception) \
    _GEN390(operation_exception) \
    _GEN900(operation_exception) \
        (void*)&disasm_none, \
        (void*)&"?????" "\0" "?" \
    }

#define GENx370x___x___(_name,_format,_mnemonic) \
    { \
    _GEN370(_name) \
    _GEN390(operation_exception) \
    _GEN900(operation_exception) \
        (void*)&disasm_ ## _format, \
        (void*)& _mnemonic "\0" #_name \
    }

#define GENx___x390x___(_name,_format,_mnemonic) \
    { \
    _GEN370(operation_exception) \
    _GEN390(_name) \
    _GEN900(operation_exception) \
        (void*)&disasm_ ## _format, \
        (void*)& _mnemonic "\0" #_name \
    }

#define GENx370x390x___(_name,_format,_mnemonic) \
    { \
    _GEN370(_name) \
    _GEN390(_name) \
    _GEN900(operation_exception) \
        (void*)&disasm_ ## _format, \
        (void*)& _mnemonic "\0" #_name \
    }

#define GENx___x___x900(_name,_format,_mnemonic) \
    { \
    _GEN370(operation_exception) \
    _GEN390(operation_exception) \
    _GEN900(_name) \
        (void*)&disasm_ ## _format, \
        (void*)& _mnemonic "\0" #_name \
    }

#define GENx370x___x900(_name,_format,_mnemonic) \
    { \
    _GEN370(_name) \
    _GEN390(operation_exception) \
    _GEN900(_name) \
        (void*)&disasm_ ## _format, \
        (void*)& _mnemonic "\0" #_name \
    }

#define GENx___x390x900(_name,_format,_mnemonic) \
    { \
    _GEN370(operation_exception) \
    _GEN390(_name) \
    _GEN900(_name) \
        (void*)&disasm_ ## _format, \
        (void*)& _mnemonic "\0" #_name \
    }

#define GENx370x390x900(_name,_format,_mnemonic) \
    { \
    _GEN370(_name) \
    _GEN390(_name) \
    _GEN900(_name) \
        (void*)&disasm_ ## _format, \
        (void*)& _mnemonic "\0" #_name \
    }

#define GENx37Xx390x___ GENx___x390x___
#define GENx37Xx___x900 GENx___x___x900
#define GENx37Xx390x900 GENx___x390x900

typedef void (ATTR_REGPARM(2) *zz_func) (BYTE inst[], REGS *regs);

#define ILC(_b) ((_b) < 0x40 ? 2 : (_b) < 0xc0 ? 4 : 6)

#define REAL_ILC(_regs) \
 (likely(!(_regs)->execflag) ? (_regs)->psw.ilc : (_regs)->exrl ? 6 : 4)

#define DISASM_INSTRUCTION(_inst, p) \
    disasm_table((_inst), 0, p)

typedef int (*func) ();

extern int disasm_table (BYTE inst[], char mnemonic[], char *p);

#if defined(OPTION_INSTRUCTION_COUNTING)

#define COUNT_INST(_inst, _regs) \
do { \
int used; \
    switch((_inst)[0]) { \
    case 0x01: \
        used = sysblk.imap01[(_inst)[1]]++; \
        break; \
    case 0xA4: \
        used = sysblk.imapa4[(_inst)[1]]++; \
        break; \
    case 0xA5: \
        used = sysblk.imapa5[(_inst)[1] & 0x0F]++; \
        break; \
    case 0xA6: \
        used = sysblk.imapa6[(_inst)[1]]++; \
        break; \
    case 0xA7: \
        used = sysblk.imapa7[(_inst)[1] & 0x0F]++; \
        break; \
    case 0xB2: \
        used = sysblk.imapb2[(_inst)[1]]++; \
        break; \
    case 0xB3: \
        used = sysblk.imapb3[(_inst)[1]]++; \
        break; \
    case 0xB9: \
        used = sysblk.imapb9[(_inst)[1]]++; \
        break; \
    case 0xC0: \
        used = sysblk.imapc0[(_inst)[1] & 0x0F]++; \
        break; \
    case 0xC2:                                     /*@Z9*/ \
        used = sysblk.imapc2[(_inst)[1] & 0x0F]++; /*@Z9*/ \
        break;                                     /*@Z9*/ \
    case 0xC4:                                     /*208*/ \
        used = sysblk.imapc4[(_inst)[1] & 0x0F]++; /*208*/ \
        break;                                     /*208*/ \
    case 0xC6:                                     /*208*/ \
        used = sysblk.imapc6[(_inst)[1] & 0x0F]++; /*208*/ \
        break;                                     /*208*/ \
    case 0xC8: \
        used = sysblk.imapc8[(_inst)[1] & 0x0F]++; \
        break; \
    case 0xE3: \
        used = sysblk.imape3[(_inst)[5]]++; \
        break; \
    case 0xE4: \
        used = sysblk.imape4[(_inst)[1]]++; \
        break; \
    case 0xE5: \
        used = sysblk.imape5[(_inst)[1]]++; \
        break; \
    case 0xEB: \
        used = sysblk.imapeb[(_inst)[5]]++; \
        break; \
    case 0xEC: \
        used = sysblk.imapec[(_inst)[5]]++; \
        break; \
    case 0xED: \
        used = sysblk.imaped[(_inst)[5]]++; \
        break; \
    default: \
        used = sysblk.imapxx[(_inst)[0]]++; \
    } \
    if(!used) \
    { \
    WRMSG(HHC02292, "I", "First use"); \
    ARCH_DEP(display_inst) ((_regs), (_inst)); \
    } \
} while(0)

#else

#define COUNT_INST(_inst, _regs)

#endif

#if defined(_FEATURE_SIE)
  #define SIE_MODE(_register_context) \
          unlikely((_register_context)->sie_mode)
  #define SIE_STATE(_register_context) \
          ((_register_context)->sie_state)
  #define SIE_FEATB(_regs, _feat_byte, _feat_name) \
          (((_regs)->siebk->SIE_ ## _feat_byte) & (SIE_ ## _feat_byte ## _ ## _feat_name))
  #define SIE_STATB(_regs, _feat_byte, _feat_name) \
          (SIE_MODE((_regs)) && SIE_FEATB((_regs), _feat_byte, _feat_name) )
  #define SIE_STATNB(_regs, _feat_byte, _feat_name) \
          (SIE_MODE((_regs)) && !SIE_FEATB((_regs), _feat_byte, _feat_name) )
#else
  #define SIE_MODE(_register_context) (0)
  #define SIE_STATE(_register_context) (0)
  #define SIE_FEATB(_register_context, _feat_byte, _feat_name) (0)
  #define SIE_STATB(_register_context, _feat_byte, _feat_name) (0)
#endif


/* The footprint_buffer option saves a copy of the register context
   every time an instruction is executed.  This is for problem
   determination only, as it severely impacts performance.       *JJ */

#if defined(OPTION_FOOTPRINT_BUFFER)
#define FOOTPRINT(_ip, _regs) \
do { \
    sysblk.footprregs[(_regs)->cpuad][sysblk.footprptr[(_regs)->cpuad]] = *(_regs); \
    memcpy(&sysblk.footprregs[(_regs)->cpuad][sysblk.footprptr[(_regs)->cpuad]++].inst,(_ip),6); \
    sysblk.footprptr[(_regs)->cpuad] &= OPTION_FOOTPRINT_BUFFER - 1; \
} while(0)
#endif

#if !defined(FOOTPRINT)
#define FOOTPRINT(_ip, _regs)
#endif

/* PSW Instruction Address manipulation */

#define _PSW_IA(_regs, _n) \
 (VADR)((_regs)->AIV + ((intptr_t)(_regs)->ip - (intptr_t)(_regs)->aip) + (_n))

#define PSW_IA(_regs, _n) \
 (_PSW_IA((_regs), (_n)) & ADDRESS_MAXWRAP((_regs)))

#define SET_PSW_IA(_regs) \
do { \
  if ((_regs)->aie) (_regs)->psw.IA = PSW_IA((_regs), 0); \
} while (0)

#define UPD_PSW_IA(_regs, _addr) \
do { \
  (_regs)->psw.IA = (_addr) & ADDRESS_MAXWRAP(_regs); \
  if (likely((_regs)->aie != NULL)) { \
    if (likely((_regs)->AIV == ((_regs)->psw.IA & (PAGEFRAME_PAGEMASK|1)))) \
      (_regs)->ip = _PSW_IA_MAIN((_regs), (_regs)->psw.IA); \
    else \
      (_regs)->aie = NULL; \
  } \
} while (0)

/*
 * The next three macros are used by branch-and-link type instructions
 * where the addressing mode is known.
 * Note that wrap is not performed for PSW_IA64 and for PSW_IA31.
 * For the latter, we expect branch-and-link code to `or' the hi bit
 * on so there is no need to `and' it off.
 */
#define PSW_IA64(_regs, _n) \
  ((_regs)->AIV \
   + (((uintptr_t)(_regs)->ip + (unsigned int)(_n)) - (uintptr_t)(_regs)->aip))
#define PSW_IA31(_regs, _n) \
  ((_regs)->AIV_L + ((uintptr_t)(_regs)->ip + (unsigned int)(_n)) \
   - (uintptr_t)(_regs)->aip)
#define PSW_IA24(_regs, _n) \
 (((_regs)->AIV_L + ((uintptr_t)(_regs)->ip + (unsigned int)(_n)) \
   - (uintptr_t)(_regs)->aip) & AMASK24)

/* Accelerator for instruction addresses */

#define INVALIDATE_AIA(_regs) \
do { \
  if ((_regs)->aie) { \
    (_regs)->psw.IA = PSW_IA((_regs), 0); \
    (_regs)->aie = NULL; \
  } \
} while (0)

#define INVALIDATE_AIA_MAIN(_regs, _main) \
do { \
  if ((_main) == (_regs)->aip && (_regs)->aie) { \
    (_regs)->psw.IA = PSW_IA((_regs), 0); \
    (_regs)->aie = NULL; \
  } \
} while (0)

#if 1
#define _PSW_IA_MAIN(_regs, _addr) \
 ((BYTE *)((uintptr_t)(_regs)->aip | (uintptr_t)((_addr) & PAGEFRAME_BYTEMASK)))
#else
#define _PSW_IA_MAIN(_regs, _addr) \
 ((BYTE *)((_regs)->aim ^ (uintptr_t)(_addr)))
#endif

#define _VALID_IP(_regs, _exec) \
( \
    ( !(_exec) && (_regs)->ip <  (_regs)->aie ) \
 || \
    ( (_exec) && ((_regs)->ET & (PAGEFRAME_PAGEMASK|0x01)) == (_regs)->AIV \
   && _PSW_IA_MAIN((_regs), (_regs)->ET) < (_regs)->aie \
    ) \
)

/* Instruction fetching */

#define INSTRUCTION_FETCH(_regs, _exec) \
  likely(_VALID_IP((_regs),(_exec))) \
  ? ((_exec) ? _PSW_IA_MAIN((_regs), (_regs)->ET) : (_regs)->ip) \
  : ARCH_DEP(instfetch) ((_regs), (_exec))

/* Instruction execution */

#define EXECUTE_INSTRUCTION(_oct, _ip, _regs) \
do { \
    FOOTPRINT ((_ip), (_regs)); \
    COUNT_INST ((_ip), (_regs)); \
    (_oct)[fetch_hw((_ip))]((_ip), (_regs)); \
} while(0)

#define UNROLLED_EXECUTE(_oct, _regs) \
 if ((_regs)->ip >= (_regs)->aie) break; \
 EXECUTE_INSTRUCTION((_oct), (_regs)->ip, (_regs))

/* Branching */

#define SUCCESSFUL_BRANCH(_regs, _addr, _len) \
do { \
  VADR _newia; \
  UPDATE_BEAR((_regs), 0); \
  _newia = (_addr) & ADDRESS_MAXWRAP((_regs)); \
  if (likely(!(_regs)->permode && !(_regs)->execflag) \
   && likely((_newia & (PAGEFRAME_PAGEMASK|0x01)) == (_regs)->AIV)) { \
    (_regs)->ip = (BYTE *)((uintptr_t)(_regs)->aim ^ (uintptr_t)_newia); \
    return; \
  } else { \
    if (unlikely((_regs)->execflag)) \
      UPDATE_BEAR((_regs), (_len) - ((_regs)->exrl ? 6 : 4)); \
    (_regs)->psw.IA = _newia; \
    (_regs)->aie = NULL; \
    PER_SB((_regs), (_regs)->psw.IA); \
  } \
} while (0)

#define SUCCESSFUL_RELATIVE_BRANCH(_regs, _offset, _len) \
do { \
  UPDATE_BEAR((_regs), 0); \
  if (likely(!(_regs)->permode && !(_regs)->execflag) \
   && likely((_regs)->ip + (_offset) >= (_regs)->aip) \
   && likely((_regs)->ip + (_offset) <  (_regs)->aie)) { \
    (_regs)->ip += (_offset); \
    return; \
  } else { \
    if (likely(!(_regs)->execflag)) \
      (_regs)->psw.IA = PSW_IA((_regs), (_offset)); \
    else { \
      UPDATE_BEAR((_regs), (_len) - ((_regs)->exrl ? 6 : 4)); \
      (_regs)->psw.IA = (_regs)->ET + (_offset); \
      (_regs)->psw.IA &= ADDRESS_MAXWRAP((_regs)); \
    } \
    (_regs)->aie = NULL; \
    PER_SB((_regs), (_regs)->psw.IA); \
  } \
} while (0)

/* BRCL, BRASL can branch +/- 4G.  This is problematic on a 32 bit host */
#define SUCCESSFUL_RELATIVE_BRANCH_LONG(_regs, _offset) \
do { \
  UPDATE_BEAR((_regs), 0); \
  if (likely(!(_regs)->permode && !(_regs)->execflag) \
   && likely((_offset) > -4096) \
   && likely((_offset) <  4096) \
   && likely((_regs)->ip + (_offset) >= (_regs)->aip) \
   && likely((_regs)->ip + (_offset) <  (_regs)->aie)) { \
    (_regs)->ip += (_offset); \
    return; \
  } else { \
    if (likely(!(_regs)->execflag)) \
      (_regs)->psw.IA = PSW_IA((_regs), (_offset)); \
    else { \
      UPDATE_BEAR((_regs), 6 - ((_regs)->exrl ? 6 : 4)); \
      (_regs)->psw.IA = (_regs)->ET + (_offset); \
      (_regs)->psw.IA &= ADDRESS_MAXWRAP((_regs)); \
    } \
    (_regs)->aie = NULL; \
    PER_SB((_regs), (_regs)->psw.IA); \
  } \
} while (0)

/* CPU Stepping or Tracing */

#define CPU_STEPPING(_regs, _ilc) \
  ( \
      sysblk.inststep \
   && ( \
        (sysblk.stepaddr[0] == 0 && sysblk.stepaddr[1] == 0) \
     || (sysblk.stepaddr[0] <= sysblk.stepaddr[1] \
         && PSW_IA((_regs), -(_ilc)) >= sysblk.stepaddr[0] \
         && PSW_IA((_regs), -(_ilc)) <= sysblk.stepaddr[1] \
        ) \
     || (sysblk.stepaddr[0] > sysblk.stepaddr[1] \
         && PSW_IA((_regs), -(_ilc)) >= sysblk.stepaddr[1] \
         && PSW_IA((_regs), -(_ilc)) <= sysblk.stepaddr[0] \
        ) \
      ) \
  )

#define CPU_TRACING(_regs, _ilc) \
  ( \
      sysblk.insttrace \
   && ( \
        (sysblk.traceaddr[0] == 0 && sysblk.traceaddr[1] == 0) \
     || (sysblk.traceaddr[0] <= sysblk.traceaddr[1] \
         && PSW_IA((_regs), -(_ilc)) >= sysblk.traceaddr[0] \
         && PSW_IA((_regs), -(_ilc)) <= sysblk.traceaddr[1] \
        ) \
     || (sysblk.traceaddr[0] > sysblk.traceaddr[1] \
         && PSW_IA((_regs), -(_ilc)) >= sysblk.traceaddr[1] \
         && PSW_IA((_regs), -(_ilc)) <= sysblk.traceaddr[0] \
        ) \
      ) \
  )

#define CPU_STEPPING_OR_TRACING(_regs, _ilc) \
  ( unlikely((_regs)->tracing) && \
    (CPU_STEPPING((_regs), (_ilc)) || CPU_TRACING((_regs), (_ilc))) \
  )

#define CPU_TRACING_ALL \
  (sysblk.insttrace && sysblk.traceaddr[0] == 0 && sysblk.traceaddr[1] == 0)

#define CPU_STEPPING_ALL \
  (sysblk.inststep && sysblk.stepaddr[0] == 0 && sysblk.stepaddr[1] == 0)

#define CPU_STEPPING_OR_TRACING_ALL \
  ( CPU_TRACING_ALL || CPU_STEPPING_ALL )


#define RETURN_INTCHECK(_regs) \
        longjmp((_regs)->progjmp, SIE_NO_INTERCEPT)

#define ODD_CHECK(_r, _regs) \
    if( (_r) & 1 ) \
        (_regs)->program_interrupt( (_regs), PGM_SPECIFICATION_EXCEPTION)

#define ODD2_CHECK(_r1, _r2, _regs) \
    if( ((_r1) & 1) || ((_r2) & 1) ) \
        (_regs)->program_interrupt( (_regs), PGM_SPECIFICATION_EXCEPTION)

#define HW_CHECK(_value, _regs) \
    if( (_value) & 1 ) \
        (_regs)->program_interrupt( (_regs), PGM_SPECIFICATION_EXCEPTION)

#define FW_CHECK(_value, _regs) \
    if( (_value) & 3 ) \
        (_regs)->program_interrupt( (_regs), PGM_SPECIFICATION_EXCEPTION)

#define DW_CHECK(_value, _regs) \
    if( (_value) & 7 ) \
        (_regs)->program_interrupt( (_regs), PGM_SPECIFICATION_EXCEPTION)

#define QW_CHECK(_value, _regs) \
    if( (_value) & 15 ) \
        (_regs)->program_interrupt( (_regs), PGM_SPECIFICATION_EXCEPTION)

    /* Program check if m is not 0, 1, or 4 to 7 */
#define HFPM_CHECK(_m, _regs) \
    if (((_m) == 2) || ((_m) == 3) || ((_m) & 8)) \
        (_regs)->program_interrupt( (_regs), PGM_SPECIFICATION_EXCEPTION)

#define PRIV_CHECK(_regs) \
    if( PROBSTATE(&(_regs)->psw) ) \
        (_regs)->program_interrupt( (_regs), PGM_PRIVILEGED_OPERATION_EXCEPTION)

    /* Program check if r is not 0,1,4,5,8,9,12, or 13 (designating
       the lower-numbered register of a floating-point register pair) */
#define BFPREGPAIR_CHECK(_r, _regs) \
    if( ((_r) & 2) ) \
        (_regs)->program_interrupt( (_regs), PGM_SPECIFICATION_EXCEPTION)

    /* Program check if r1 and r2 are not both 0,1,4,5,8,9,12, or 13
       (lower-numbered register of a floating-point register pair) */
#define BFPREGPAIR2_CHECK(_r1, _r2, _regs) \
    if( ((_r1) & 2) || ((_r2) & 2) ) \
        (_regs)->program_interrupt( (_regs), PGM_SPECIFICATION_EXCEPTION)

    /* Program check if r is not 0,1,4,5,8,9,12, or 13 (designating
       the lower-numbered register of a floating-point register pair) */
#define DFPREGPAIR_CHECK(_r, _regs) \
    if( ((_r) & 2) ) \
        (_regs)->program_interrupt( (_regs), PGM_SPECIFICATION_EXCEPTION)

    /* Program check if r1 and r2 are not both 0,1,4,5,8,9,12, or 13
       (lower-numbered register of a floating-point register pair) */
#define DFPREGPAIR2_CHECK(_r1, _r2, _regs) \
    if( ((_r1) & 2) || ((_r2) & 2) ) \
        (_regs)->program_interrupt( (_regs), PGM_SPECIFICATION_EXCEPTION)

    /* Program check if r1, r2, r3 are not all 0,1,4,5,8,9,12, or 13
       (lower-numbered register of a floating-point register pair) */
#define DFPREGPAIR3_CHECK(_r1, _r2, _r3, _regs) \
    if( ((_r1) & 2) || ((_r2) & 2) || ((_r3) & 2) ) \
        (_regs)->program_interrupt( (_regs), PGM_SPECIFICATION_EXCEPTION)

#define SSID_CHECK(_regs) \
    if((!((_regs)->GR_LHH(1) & 0x0001)) \
    || (_regs)->GR_LHH(1) > (0x0001|(FEATURE_LCSS_MAX-1))) \
        (_regs)->program_interrupt( (_regs), PGM_OPERAND_EXCEPTION)

#define IOID_TO_SSID(_ioid) \
    ((_ioid) >> 16)

#define IOID_TO_LCSS(_ioid) \
    ((_ioid) >> 17)

#define SSID_TO_LCSS(_ssid) \
    ((_ssid) >> 1)

#define LCSS_TO_SSID(_lcss) \
    (((_lcss) << 1) | 1)

/* Virtual Architecture Level Set Facility */
#define FACILITY_ENABLED(_faci, _regs) \
        (((_regs)->facility_list[((STFL_ ## _faci)/8)]) & (0x80 >> ((STFL_ ## _faci) % 8)))
#define FACILITY_ENABLED_DEV(_faci) \
        ((sysblk.facility_list[sysblk.arch_mode][((STFL_ ## _faci)/8)]) & (0x80 >> ((STFL_ ## _faci) % 8)))

#define FACILITY_CHECK(_faci, _regs) \
    do { \
        if(!FACILITY_ENABLED( _faci, _regs ) ) \
          (_regs)->program_interrupt( (_regs), PGM_OPERATION_EXCEPTION); \
    } while (0)


#define PER_RANGE_CHECK(_addr, _low, _high) \
  ( (((_high) & MAXADDRESS) >= ((_low) & MAXADDRESS)) ? \
  (((_addr) >= ((_low) & MAXADDRESS)) && (_addr) <= ((_high) & MAXADDRESS)) : \
  (((_addr) >= ((_low) & MAXADDRESS)) || (_addr) <= ((_high) & MAXADDRESS)) )

#define PER_RANGE_CHECK2(_addr1, _addr2, _low, _high) \
  ( (((_high) & MAXADDRESS) >= ((_low) & MAXADDRESS)) ? \
  (((_addr1) >= ((_low) & MAXADDRESS)) && (_addr1) <= ((_high) & MAXADDRESS)) || \
  (((_addr2) >= ((_low) & MAXADDRESS)) && (_addr2) <= ((_high) & MAXADDRESS)) || \
  (((_addr1) <= ((_low) & MAXADDRESS)) && (_addr2) >= ((_high) & MAXADDRESS)) :  \
  (((_addr2) >= ((_low) & MAXADDRESS)) || (_addr1) <= ((_high) & MAXADDRESS)) )

#ifdef WORDS_BIGENDIAN
 #define CSWAP16(_x) (_x)
 #define CSWAP32(_x) (_x)
 #define CSWAP64(_x) (_x)
#else
 #define CSWAP16(_x) bswap_16(_x)
 #define CSWAP32(_x) bswap_32(_x)
 #define CSWAP64(_x) bswap_64(_x)
#endif

#define FETCH_HW(_value, _storage) (_value) = fetch_hw(_storage)
#define FETCH_FW(_value, _storage) (_value) = fetch_fw(_storage)
#define FETCH_F3(_value, _storage) (_value) = fetch_f3(_storage)
#define FETCH_DW(_value, _storage) (_value) = fetch_dw(_storage)

#define STORE_HW(_storage, _value) store_hw(_storage, _value)
#define STORE_FW(_storage, _value) store_fw(_storage, _value)
#define STORE_F3(_storage, _value) store_f3(_storage, _value)
#define STORE_DW(_storage, _value) store_dw(_storage, _value)

#include "machdep.h"

#endif /*!defined(_OPCODE_H)*/

/* Program check if fpc is not valid contents for FPC register */
#undef FPC_BRM
#undef FPC_CHECK
#if defined(FEATURE_FLOATING_POINT_EXTENSION_FACILITY)          /*810*/
#define FPC_BRM FPC_BRM_3BIT
#define FPC_CHECK(_fpc, _regs) \
    if(((_fpc) & FPC_RESV_FPX) \
     || ((_fpc) & FPC_BRM_3BIT) == BRM_RESV4 \
     || ((_fpc) & FPC_BRM_3BIT) == BRM_RESV5 \
     || ((_fpc) & FPC_BRM_3BIT) == BRM_RESV6) \
        (_regs)->program_interrupt( (_regs), PGM_SPECIFICATION_EXCEPTION)
#else /*!defined(FEATURE_FLOATING_POINT_EXTENSION_FACILITY)*/   /*810*/
#define FPC_BRM FPC_BRM_2BIT
#define FPC_CHECK(_fpc, _regs) \
    if((_fpc) & FPC_RESERVED) \
        (_regs)->program_interrupt( (_regs), PGM_SPECIFICATION_EXCEPTION)
#endif /*!defined(FEATURE_FLOATING_POINT_EXTENSION_FACILITY)*/  /*810*/



#undef SIE_ACTIVE
#if defined(FEATURE_INTERPRETIVE_EXECUTION)
 #define SIE_ACTIVE(_regs) ((_regs)->sie_active)
#else
 #define SIE_ACTIVE(_regs) (0)
#endif

#undef MULTIPLE_CONTROLLED_DATA_SPACE
#if defined(_FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE)
 #define MULTIPLE_CONTROLLED_DATA_SPACE(_regs) \
      ( SIE_FEATB((_regs), MX, XC) && AR_BIT(&(_regs)->psw) )
#else
 #define MULTIPLE_CONTROLLED_DATA_SPACE(_regs) (0)
#endif

/* PER3 Breaking Event Address Recording (BEAR) */

#undef UPDATE_BEAR
#undef SET_BEAR_REG

#if defined(FEATURE_PER3)
 #define UPDATE_BEAR(_regs, _n) (_regs)->bear_ip = (_regs)->ip + (_n)
 #define SET_BEAR_REG(_regs, _ip) \
  do { \
    if ((_ip)) { \
        (_regs)->bear = (_regs)->AIV \
                      + (intptr_t)((_ip) - (_regs)->aip); \
        (_regs)->bear &= ADDRESS_MAXWRAP((_regs)); \
        regs->bear_ip = NULL; \
    } \
  } while (0)
#else
 #define UPDATE_BEAR(_regs, _n)   while (0)
 #define SET_BEAR_REG(_regs, _ip) while (0)
#endif

/* Set addressing mode (BASSM, BSM) */

#undef SET_ADDRESSING_MODE
#if defined(FEATURE_ESAME)
 #define SET_ADDRESSING_MODE(_regs, _addr) \
 do { \
  if ((_addr) & 1) { \
    (_regs)->psw.amode64 = regs->psw.amode = 1; \
    (_regs)->psw.AMASK = AMASK64; \
    (_addr) ^= 1; \
  } else if ((_addr) & 0x80000000) { \
    (_regs)->psw.amode64 = 0; \
    (_regs)->psw.amode = 1; \
    (_regs)->psw.AMASK = AMASK31; \
  } else { \
    (_regs)->psw.amode64 = (_regs)->psw.amode = 0; \
    (_regs)->psw.AMASK = AMASK24; \
  } \
 } while (0)
#else /* !defined(FEATURE_ESAME) */
  #define SET_ADDRESSING_MODE(_regs, _addr) \
 do { \
  if ((_addr) & 0x80000000) { \
    (_regs)->psw.amode = 1; \
    (_regs)->psw.AMASK = AMASK31; \
  } else { \
    (_regs)->psw.amode = 0; \
    (_regs)->psw.AMASK = AMASK24; \
  } \
 } while (0)
#endif

#undef HFPREG_CHECK
#undef HFPREG2_CHECK
#undef HFPODD_CHECK
#undef HFPODD2_CHECK
#undef FPR2I
#undef FPREX

#if defined(FEATURE_BASIC_FP_EXTENSIONS)
#if defined(_FEATURE_SIE)

    /* Program check if BFP instruction is executed when AFP control is zero */
#define BFPINST_CHECK(_regs) \
        if( !((_regs)->CR(0) & CR0_AFP) \
            || (SIE_MODE((_regs)) && !((_regs)->hostregs->CR(0) & CR0_AFP)) ) { \
            (_regs)->dxc = DXC_BFP_INSTRUCTION; \
            (_regs)->program_interrupt( (_regs), PGM_DATA_EXCEPTION); \
        }

    /* Program check if DFP instruction is executed when AFP control is zero */
#define DFPINST_CHECK(_regs) \
        if( !((_regs)->CR(0) & CR0_AFP) \
            || (SIE_MODE((_regs)) && !((_regs)->hostregs->CR(0) & CR0_AFP)) ) { \
            (_regs)->dxc = DXC_DFP_INSTRUCTION; \
            (_regs)->program_interrupt( (_regs), PGM_DATA_EXCEPTION); \
        }

    /* Program check if r1 is not 0, 2, 4, or 6 */
#define HFPREG_CHECK(_r, _regs) \
    if( !((_regs)->CR(0) & CR0_AFP) \
            || (SIE_MODE((_regs)) && !((_regs)->hostregs->CR(0) & CR0_AFP)) ) { \
        if( (_r) & 9 ) { \
                (_regs)->dxc = DXC_AFP_REGISTER; \
        (_regs)->program_interrupt( (_regs), PGM_DATA_EXCEPTION); \
        } \
    }

    /* Program check if r1 and r2 are not 0, 2, 4, or 6 */
#define HFPREG2_CHECK(_r1, _r2, _regs) \
    if( !((_regs)->CR(0) & CR0_AFP) \
            || (SIE_MODE((_regs)) && !((_regs)->hostregs->CR(0) & CR0_AFP)) ) { \
        if( ((_r1) & 9) || ((_r2) & 9) ) { \
                (_regs)->dxc = DXC_AFP_REGISTER; \
        (_regs)->program_interrupt( (_regs), PGM_DATA_EXCEPTION); \
        } \
    }

    /* Program check if r1 is not 0 or 4 */
#define HFPODD_CHECK(_r, _regs) \
    if( (_r) & 2 ) \
        (_regs)->program_interrupt( (_regs), PGM_SPECIFICATION_EXCEPTION); \
    else if( !((_regs)->CR(0) & CR0_AFP) \
               || (SIE_MODE((_regs)) && !((_regs)->hostregs->CR(0) & CR0_AFP)) ) { \
        if( (_r) & 9 ) { \
                (_regs)->dxc = DXC_AFP_REGISTER; \
        (_regs)->program_interrupt( (_regs), PGM_DATA_EXCEPTION); \
        } \
    }

    /* Program check if r1 and r2 are not 0 or 4 */
#define HFPODD2_CHECK(_r1, _r2, _regs) \
    if( ((_r1) & 2) || ((_r2) & 2) ) \
        (_regs)->program_interrupt( (_regs), PGM_SPECIFICATION_EXCEPTION); \
    else if( !((_regs)->CR(0) & CR0_AFP) \
                || (SIE_MODE((_regs)) && !((_regs)->hostregs->CR(0) & CR0_AFP)) ) { \
        if( ((_r1) & 9) || ((_r2) & 9) ) { \
                (_regs)->dxc = DXC_AFP_REGISTER; \
        (_regs)->program_interrupt( (_regs), PGM_DATA_EXCEPTION); \
        } \
    }
#else /*!defined(_FEATURE_SIE)*/

    /* Program check if BFP instruction is executed when AFP control is zero */
#define BFPINST_CHECK(_regs) \
        if( !((_regs)->CR(0) & CR0_AFP) ) { \
            (_regs)->dxc = DXC_BFP_INSTRUCTION; \
            (_regs)->program_interrupt( (_regs), PGM_DATA_EXCEPTION); \
        }

    /* Program check if DFP instruction is executed when AFP control is zero */
#define DFPINST_CHECK(_regs) \
        if( !((_regs)->CR(0) & CR0_AFP) ) { \
            (_regs)->dxc = DXC_DFP_INSTRUCTION; \
            (_regs)->program_interrupt( (_regs), PGM_DATA_EXCEPTION); \
        }


    /* Program check if r1 is not 0, 2, 4, or 6 */
#define HFPREG_CHECK(_r, _regs) \
    if( !((_regs)->CR(0) & CR0_AFP) ) { \
        if( (_r) & 9 ) { \
                (_regs)->dxc = DXC_AFP_REGISTER; \
        (_regs)->program_interrupt( (_regs), PGM_DATA_EXCEPTION); \
        } \
    }

    /* Program check if r1 and r2 are not 0, 2, 4, or 6 */
#define HFPREG2_CHECK(_r1, _r2, _regs) \
    if( !((_regs)->CR(0) & CR0_AFP) ) { \
        if( ((_r1) & 9) || ((_r2) & 9) ) { \
                (_regs)->dxc = DXC_AFP_REGISTER; \
        (_regs)->program_interrupt( (_regs), PGM_DATA_EXCEPTION); \
        } \
    }

    /* Program check if r1 is not 0 or 4 */
#define HFPODD_CHECK(_r, _regs) \
    if( (_r) & 2 ) \
        (_regs)->program_interrupt( (_regs), PGM_SPECIFICATION_EXCEPTION); \
    else if( !((_regs)->CR(0) & CR0_AFP) ) { \
        if( (_r) & 9 ) { \
                (_regs)->dxc = DXC_AFP_REGISTER; \
        (_regs)->program_interrupt( (_regs), PGM_DATA_EXCEPTION); \
        } \
    }

    /* Program check if r1 and r2 are not 0 or 4 */
#define HFPODD2_CHECK(_r1, _r2, _regs) \
    if( ((_r1) & 2) || ((_r2) & 2) ) \
        (_regs)->program_interrupt( (_regs), PGM_SPECIFICATION_EXCEPTION); \
    else if( !((_regs)->CR(0) & CR0_AFP) ) { \
        if( ((_r1) & 9) || ((_r2) & 9) ) { \
                (_regs)->dxc = DXC_AFP_REGISTER; \
        (_regs)->program_interrupt( (_regs), PGM_DATA_EXCEPTION); \
        } \
    }

#endif /*!defined(_FEATURE_SIE)*/


    /* Convert fpr to index */
#define FPR2I(_r) \
    ((_r) << 1)

    /* Offset of extended register */
#define FPREX 4

#else /*!defined(FEATURE_BASIC_FP_EXTENSIONS)*/

    /* Program check if r1 is not 0, 2, 4, or 6 */
#define HFPREG_CHECK(_r, _regs) \
    if( (_r) & 9 ) \
        (_regs)->program_interrupt( (_regs), PGM_SPECIFICATION_EXCEPTION)

    /* Program check if r1 and r2 are not 0, 2, 4, or 6 */
#define HFPREG2_CHECK(_r1, _r2, _regs) \
    if( ((_r1) & 9) || ((_r2) & 9) ) \
        (_regs)->program_interrupt( (_regs), PGM_SPECIFICATION_EXCEPTION)

    /* Program check if r1 is not 0 or 4 */
#define HFPODD_CHECK(_r, _regs) \
    if( (_r) & 11 ) \
        (_regs)->program_interrupt( (_regs), PGM_SPECIFICATION_EXCEPTION)

    /* Program check if r1 and r2 are not 0 or 4 */
#define HFPODD2_CHECK(_r1, _r2, _regs) \
    if( ((_r1) & 11) || ((_r2) & 11) ) \
        (_regs)->program_interrupt( (_regs), PGM_SPECIFICATION_EXCEPTION)

    /* Convert fpr to index */
#define FPR2I(_r) \
    (_r)

    /* Offset of extended register */
#define FPREX 2

#endif /*!defined(FEATURE_BASIC_FP_EXTENSIONS)*/

#define TLBIX(_addr) (((VADR_L)(_addr) >> TLB_PAGESHIFT) & TLB_MASK)

#define MAINADDR(_main, _addr) \
   (BYTE*)((uintptr_t)(_main) ^ (uintptr_t)(_addr))

#define NEW_MAINADDR(_regs, _addr, _aaddr) \
   (BYTE*)((uintptr_t)((_regs)->mainstor \
         + (uintptr_t)(_aaddr)) \
         ^ (uintptr_t)((_addr) & TLB_PAGEMASK))

/* Perform invalidation after storage key update.
 * If the REF or CHANGE bit is turned off for an absolute
 * address then we need to invalidate any cached entries
 * for that address on *all* CPUs.
 * FIXME: Synchronization, esp for the CHANGE bit, should
 * be tighter than what is provided here.
 */

#define STORKEY_INVALIDATE(_regs, _n) \
 do { \
   BYTE *mn; \
   mn = (_regs)->mainstor + ((_n) & PAGEFRAME_PAGEMASK); \
   ARCH_DEP(invalidate_tlbe)((_regs), mn); \
   if (sysblk.cpus > 1) { \
     int i; \
     OBTAIN_INTLOCK ((_regs)); \
     for (i = 0; i < sysblk.hicpu; i++) { \
       if (IS_CPU_ONLINE(i) && i != (_regs)->cpuad) { \
         if ( sysblk.waiting_mask & CPU_BIT(i) ) \
           ARCH_DEP(invalidate_tlbe)(sysblk.regs[i], mn); \
         else { \
           ON_IC_INTERRUPT(sysblk.regs[i]); \
           if (!sysblk.regs[i]->invalidate) { \
             sysblk.regs[i]->invalidate = 1; \
             sysblk.regs[i]->invalidate_main = mn; \
           } else \
             sysblk.regs[i]->invalidate_main = NULL; \
         } \
       } \
     } \
     RELEASE_INTLOCK((_regs)); \
   } \
 } while (0)

#if defined(INLINE_STORE_FETCH_ADDR_CHECK)
 #define FETCH_MAIN_ABSOLUTE(_addr, _regs, _len) \
  ARCH_DEP(fetch_main_absolute)((_addr), (_regs), (_len))
#else
 #define FETCH_MAIN_ABSOLUTE(_addr, _regs, _len) \
  ARCH_DEP(fetch_main_absolute)((_addr), (_regs))
#endif

#define INST_UPDATE_PSW(_regs, _len, _ilc) \
     do { \
            if (_len) (_regs)->ip += (_len); \
            if (_ilc) (_regs)->psw.ilc = (_ilc); \
        } while(0)

/* Instruction decoders */

/*
 * A decoder is placed at the start of each instruction. The purpose
 * of a decoder is to extract the operand fields according to the
 * instruction format; to increment the instruction address (IA) field
 * of the PSW by 2, 4, or 6 bytes; and to set the instruction length
 * code (ILC) field of the PSW in case a program check occurs.
 *
 * Certain decoders have additional forms with 0 and _B suffixes.
 * - the 0 suffix version does not update the PSW ILC.
 * - the _B suffix version updates neither the PSW ILC nor the PSW IA.
 *
 * The "0" versions of the decoders are chosen whenever we know
 * that past this point, no program interrupt will be generated
 * (like most general instructions when no storage access is needed)
 * therefore needing simpler prologue code.
 * The "_B" versions for some of the decoders are intended for
 * "branch" type operations where updating the PSW IA to IA+ILC
 * should only be done after the branch is deemed impossible.
 */

#undef DECODER_TEST_RRE
#define DECODER_TEST_RRF_R
#define DECODER_TEST_RRF_M
#define DECODER_TEST_RRF_M4
#define DECODER_TEST_RRF_RM
#define DECODER_TEST_RRF_MM
#define DECODER_TEST_RRR
#undef DECODER_TEST_RX
#define DECODER_TEST_RXE
#define DECODER_TEST_RXF
#define DECODER_TEST_RXY
#undef DECODER_TEST_RS
#define DECODER_TEST_RSY
#undef DECODER_TEST_RSL
#undef DECODER_TEST_RSI
#undef DECODER_TEST_RI
#define DECODER_TEST_RIL
#define DECODER_TEST_RIL_A
#undef DECODER_TEST_RIS
#undef DECODER_TEST_RRS
#undef DECODER_TEST_SI
#define DECODER_TEST_SIY
#undef DECODER_TEST_SIL
#undef DECODER_TEST_S
#define DECODER_TEST_SS
#define DECODER_TEST_SS_L
#define DECODER_TEST_SSE
#define DECODER_TEST_SSF

/* E implied operands and extended op code */
#undef E
#define E(_inst,_regs) E_DECODER((_inst), (_regs), 2, 2)

#define E_DECODER(_inst, _regs, _len, _ilc) \
        { \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
            UNREFERENCED(_inst); \
        }

/* IE extended op code with two 4-bit immediate fields */       /*912*/
#undef IE
#undef IE0

#define IE(_inst, _regs, _i1, _i2)  \
        IE_DECODER(_inst, _regs, _i1, _i2, 4, 4)
#define IE0(_inst, _regs, _i1, _i2) \
        IE_DECODER(_inst, _regs, _i1, _i2, 4, 0)

#define IE_DECODER(_inst, _regs, _i1, _i2, _len, _ilc) \
        { \
            int i = (_inst)[3]; \
            (_i1) = i >> 4; \
            (_i2) = i & 0x0F; \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
        }

/* MII mask with 12-bit and 24-bit relative address fields */   /*912*/
#undef MII_A
#undef MII_A0

#define MII_A(_inst, _regs, _m1, _addr2, _addr3) \
        MII_A_DECODER(_inst, _regs, _m1, _addr2, _addr3, 6, 6)
#define MII_A0(_inst, _regs, _m1, _addr2, _addr3) \
        MII_A_DECODER(_inst, _regs, _m1, _addr2, _addr3, 6, 0)

#define MII_A_DECODER(_inst, _regs, _m1, _addr2, _addr3, _len, _ilc) \
        { \
            U32 ri2, ri3; S64 offset; \
            U32 temp = fetch_fw(&(_inst)[2]); \
            int i = (_inst)[1]; \
            (_m1) = (i >> 4) & 0x0F; \
            ri2 = (i << 4) | (temp >> 24); \
            ri3 = temp & 0xFFFFFF; \
            offset = 2LL*(S32)ri2; \
            (_addr2) = (likely(!(_regs)->execflag)) ? \
                    PSW_IA((_regs), offset) : \
                    ((_regs)->ET + offset) & ADDRESS_MAXWRAP((_regs)); \
            offset = 2LL*(S32)ri3; \
            (_addr3) = (likely(!(_regs)->execflag)) ? \
                    PSW_IA((_regs), offset) : \
                    ((_regs)->ET + offset) & ADDRESS_MAXWRAP((_regs)); \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
        }

/* RR register to register */
#undef RR
#undef RR0
#undef RR_B

#define RR(_inst, _regs, _r1, _r2)  \
        RR_DECODER(_inst, _regs, _r1, _r2, 2, 2)
#define RR0(_inst, _regs, _r1, _r2) \
        RR_DECODER(_inst, _regs, _r1, _r2, 2, 0)
#define RR_B(_inst, _regs, _r1, _r2) \
        RR_DECODER(_inst, _regs, _r1, _r2, 0, 0)

#define RR_DECODER(_inst, _regs, _r1, _r2, _len, _ilc) \
        { \
            int i = (_inst)[1]; \
            (_r1) = i >> 4; \
            (_r2) = i & 0x0F; \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
        }

/* RR special format for SVC instruction */
#undef RR_SVC

#define RR_SVC(_inst, _regs, _svc) \
        RR_SVC_DECODER(_inst, _regs, _svc, 2, 2)

#define RR_SVC_DECODER(_inst, _regs, _svc, _ilc, _len) \
        { \
            (_svc) = (_inst)[1]; \
            INST_UPDATE_PSW((_regs), (_ilc), (_len)); \
        }

/* RRE register to register with extended op code */
#undef RRE
#undef RRE0
#undef RRE_B

#if !defined(DECODER_TEST)&&!defined(DECODER_TEST_RRE)
 #define RRE(_inst, _regs, _r1, _r2) \
         RRE_DECODER(_inst, _regs, _r1, _r2, 4, 4)
 #define RRE0(_inst, _regs, _r1, _r2) \
         RRE_DECODER(_inst, _regs, _r1, _r2, 4, 0)
 #define RRE_B(_inst, _regs, _r1, _r2) \
         RRE_DECODER(_inst, _regs, _r1, _r2, 0, 0)
#else
 #define RRE(_inst, _regs, _r1, _r2) \
         RRE_DECODER_TEST(_inst, _regs, _r1, _r2, 4, 4)
 #define RRE0(_inst, _regs, _r1, _r2) \
         RRE_DECODER_TEST(_inst, _regs, _r1, _r2, 4, 0)
 #define RRE_B(_inst, _regs, _r1, _r2) \
         RRE_DECODER_TEST(_inst, _regs, _r1, _r2, 0, 0)
#endif

#define RRE_DECODER(_inst, _regs, _r1, _r2, _len, _ilc) \
        { \
            int i = (_inst)[3]; \
            (_r1) = i >> 4; \
            (_r2) = i & 0xf; \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
        }

#define RRE_DECODER_TEST(_inst, _regs, _r1, _r2, _len, _ilc) \
        { \
            int i = (_inst)[3]; \
            (_r2) = i & 0xf; \
            (_r1) = i >> 4; \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
        }

/* RRF register to register with additional R3 field */
#undef RRF_R

#if !defined(DECODER_TEST)&&!defined(DECODER_TEST_RRF_R)
 #define RRF_R(_inst, _regs, _r1, _r2, _r3) \
         RRF_R_DECODER(_inst, _regs, _r1, _r2, _r3, 4, 4)
#else
 #define RRF_R(_inst, _regs, _r1, _r2, _r3) \
         RRF_R_DECODER_TEST(_inst, _regs, _r1, _r2, _r3, 4, 4)
#endif

#define RRF_R_DECODER(_inst, _regs, _r1, _r2, _r3, _len, _ilc) \
        { \
            int i = (_inst)[2]; \
            (_r1) = i >> 4; \
            i = (_inst)[3]; \
            (_r3) = i >> 4; \
            (_r2) = i & 0xf; \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
        }

#define RRF_R_DECODER_TEST(_inst, _regs, _r1, _r2, _r3, _len, _ilc) \
      { U32 temp = fetch_fw(_inst); \
            (_r2) = (temp      ) & 0xf; \
            (_r3) = (temp >>  4) & 0xf; \
            (_r1) = (temp >> 12) & 0xf; \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
        }

/* RRF register to register with additional M3 field */
#undef RRF_M

#if !defined(DECODER_TEST)&&!defined(DECODER_TEST_RRF_M)
 #define RRF_M(_inst, _regs, _r1, _r2, _m3) \
         RRF_M_DECODER(_inst, _regs, _r1, _r2, _m3, 4, 4)
#else
 #define RRF_M(_inst, _regs, _r1, _r2, _m3) \
         RRF_M_DECODER_TEST(_inst, _regs, _r1, _r2, _m3, 4, 4)
#endif

#define RRF_M_DECODER(_inst, _regs, _r1, _r2, _m3, _len, _ilc) \
        { \
            int i = (_inst)[2]; \
            (_m3) = i >> 4; \
            i = (_inst)[3]; \
            (_r1) = i >> 4; \
            (_r2) = i & 0xf; \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
        }

#define RRF_M_DECODER_TEST(_inst, _regs, _r1, _r2, _m3, _len, _ilc) \
      { U32 temp = fetch_fw(_inst); \
            (_m3) = (temp >> 12) & 0xf; \
            (_r2) = (temp      ) & 0xf; \
            (_r1) = (temp >>  4) & 0xf; \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
        }


/* RRF register to register with additional M4 field */
#undef RRF_M4

#if !defined(DECODER_TEST)&&!defined(DECODER_TEST_RRF_M4)
 #define RRF_M4(_inst, _regs, _r1, _r2, _m4) \
         RRF_M4_DECODER(_inst, _regs, _r1, _r2, _m4, 4, 4)
#else
 #define RRF_M4(_inst, _regs, _r1, _r2, _m4) \
         RRF_M4_DECODER_TEST(_inst, _regs, _r1, _r2, _m4, 4, 4)
#endif

#define RRF_M4_DECODER(_inst, _regs, _r1, _r2, _m4, _len, _ilc) \
        { \
            int i = (_inst)[2]; \
            (_m4) = i & 0xf; \
            i = (_inst)[3]; \
            (_r1) = i >> 4; \
            (_r2) = i & 0xf; \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
        }

#define RRF_M4_DECODER_TEST(_inst, _regs, _r1, _r2, _m4, _len, _ilc) \
      { U32 temp = fetch_fw(_inst); \
            (_m4) = (temp >>  8) & 0xf; \
            (_r2) = (temp      ) & 0xf; \
            (_r1) = (temp >>  4) & 0xf; \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
        }

/* RRF register to register with additional R3 and M4 fields */
#undef RRF_RM

#if !defined(DECODER_TEST)&&!defined(DECODER_TEST_RRF_RM)
 #define RRF_RM(_inst, _regs, _r1, _r2, _r3, _m4) \
         RRF_RM_DECODER(_inst, _regs, _r1, _r2, _r3, _m4, 4, 4)
#else
 #define RRF_RM(_inst, _regs, _r1, _r2, _r3, _m4) \
         RRF_RM_DECODER_TEST(_inst, _regs, _r1, _r2, _r3, _m4, 4, 4)
#endif

#define RRF_RM_DECODER(_inst, _regs, _r1, _r2, _r3, _m4, _len, _ilc) \
        { \
            int i = (_inst)[2]; \
            (_r3) = i >> 4; \
            (_m4) = i & 0xf; \
            i = (_inst)[3]; \
            (_r1) = i >> 4; \
            (_r2) = i & 0xf; \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
        }

#define RRF_RM_DECODER_TEST(_inst, _regs, _r1, _r2, _r3, _m4, _len, _ilc) \
      { U32 temp = fetch_fw(_inst); \
            (_r3) = (temp >> 12) & 0xf; \
            (_m4) = (temp >>  8) & 0xf; \
            (_r2) = (temp      ) & 0xf; \
            (_r1) = (temp >>  4) & 0xf; \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
      }

/* RRF register to register with additional M3 and M4 fields */
#undef RRF_MM

#if !defined(DECODER_TEST)&&!defined(DECODER_TEST_RRF_MM)
 #define RRF_MM(_inst, _regs, _r1, _r2, _m3, _m4) \
         RRF_MM_DECODER(_inst, _regs, _r1, _r2, _m3, _m4, 4, 4)
#else
 #define RRF_MM(_inst, _regs, _r1, _r2, _m3, _m4) \
         RRF_MM_DECODER_TEST(_inst, _regs, _r1, _r2, _m3, _m4, 4, 4)
#endif

#define RRF_MM_DECODER(_inst, _regs, _r1, _r2, _m3, _m4, _len, _ilc) \
        { \
            int i = (_inst)[2]; \
            (_m3) = i >> 4; \
            (_m4) = i & 0xf; \
            i = (_inst)[3]; \
            (_r1) = i >> 4; \
            (_r2) = i & 0xf; \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
        }

#define RRF_MM_DECODER_TEST(_inst, _regs, _r1, _r2, _m3, _m4, _len, _ilc) \
      { U32 temp = fetch_fw(_inst); \
            (_m3) = (temp >> 12) & 0xf; \
            (_m4) = (temp >>  8) & 0xf; \
            (_r2) = (temp      ) & 0xf; \
            (_r1) = (temp >>  4) & 0xf; \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
      }

/* RRR register to register with register */
#undef RRR
#undef RRR0

#if !defined(DECODER_TEST)&&!defined(DECODER_TEST_RRR)
 #define RRR(_inst, _regs, _r1, _r2, _r3) \
         RRR_DECODER(_inst, _regs, _r1, _r2, _r3, 4, 4)
 #define RRR0(_inst, _regs, _r1, _r2, _r3) \
         RRR_DECODER(_inst, _regs, _r1, _r2, _r3, 4, 0)
#else
 #define RRR(_inst, _regs, _r1, _r2, _r3) \
         RRR_DECODER_TEST(_inst, _regs, _r1, _r2, _r3, 4, 4)
 #define RRR0(_inst, _regs, _r1, _r2, _r3) \
         RRR_DECODER_TEST(_inst, _regs, _r1, _r2, _r3, 4, 0)
#endif

#define RRR_DECODER(_inst, _regs, _r1, _r2, _r3, _len, _ilc) \
        { \
            int i = (_inst)[2]; \
            (_r3) = i >> 4; \
            i = (_inst)[3]; \
            (_r1) = i >> 4; \
            (_r2) = i & 0xf; \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
        }

#define RRR_DECODER_TEST(_inst, _regs, _r1, _r2, _r3, _len, _ilc) \
      { U32 temp = fetch_fw(_inst); \
            (_r3) = (temp >> 12) & 0xf; \
            (_r2) = (temp      ) & 0xf; \
            (_r1) = (temp >>  4) & 0xf; \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
        }

/* RX register and indexed storage */
#undef RX
#undef RX0
#undef RX_B

#if !defined(DECODER_TEST)&&!defined(DECODER_TEST_RX)
 #define RX(_inst, _regs, _r1, _b2, _effective_addr2) \
         RX_DECODER(_inst, _regs, _r1, _b2, _effective_addr2, 4, 4)
 #define RX0(_inst, _regs, _r1, _b2, _effective_addr2) \
         RX_DECODER(_inst, _regs, _r1, _b2, _effective_addr2, 4, 0)
 #define RX_B(_inst, _regs, _r1, _b2, _effective_addr2) \
         RX_DECODER(_inst, _regs, _r1, _b2, _effective_addr2, 0, 0)
#else
 #define RX(_inst, _regs, _r1, _b2, _effective_addr2) \
         RX_DECODER_TEST(_inst, _regs, _r1, _b2, _effective_addr2, 4, 4)
 #define RX0(_inst, _regs, _r1, _b2, _effective_addr2) \
         RX_DECODER_TEST(_inst, _regs, _r1, _b2, _effective_addr2, 4, 0)
 #define RX_B(_inst, _regs, _r1, _b2, _effective_addr2) \
         RX_DECODER_TEST(_inst, _regs, _r1, _b2, _effective_addr2, 0, 0)
#endif

#define RX_DECODER(_inst, _regs, _r1, _b2, _effective_addr2, _len, _ilc) \
    {   U32 temp = fetch_fw(_inst); \
            (_r1) = (temp >> 20) & 0xf; \
            (_b2) = (temp >> 16) & 0xf; \
            (_effective_addr2) = temp & 0xfff; \
            if(unlikely(_b2)) \
                (_effective_addr2) += (_regs)->GR((_b2)); \
            (_b2) = (temp >> 12) & 0xf; \
            if(likely(_b2)) \
                (_effective_addr2) += (_regs)->GR((_b2)); \
            if((_len)) \
            (_effective_addr2) &= ADDRESS_MAXWRAP((_regs)); \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
    }

#define RX_DECODER_TEST(_inst, _regs, _r1, _b2, _effective_addr2, _len, _ilc) \
    {   U32 temp = fetch_fw(_inst); \
            (_effective_addr2) = temp & 0xfff; \
            (_b2) = (temp >> 16) & 0xf; \
            if((_b2)) \
                (_effective_addr2) += (_regs)->GR((_b2)); \
            (_b2) = (temp >> 12) & 0xf; \
            if((_b2)) \
                (_effective_addr2) += (_regs)->GR((_b2)); \
            if ((_len)) \
                (_effective_addr2) &= ADDRESS_MAXWRAP((_regs)); \
            (_r1) = (temp >> 20) & 0xf; \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
    }

#ifdef OPTION_OPTINST
/* Optimized RX decoder in case of zero X2 */
#undef  RXX0
#define RXX0(_inst, _regs, _r1, _b2, _effective_addr2) \
        RXX0_DECODER(_inst, _regs, _r1, _b2, _effective_addr2, 4, 4)
#define RXX0_DECODER(_inst, _regs, _r1, _b2, _effective_addr2, _len, _ilc) \
{ \
  U32 temp = fetch_fw(_inst); \
  (_effective_addr2) = temp & 0xfff; \
  (_r1) = (temp >> 20) & 0xf; \
  (_b2) = (temp >> 12) & 0xf; \
  if(likely(_b2)) \
    (_effective_addr2) += (_regs)->GR((_b2)); \
  if((_len)) \
    (_effective_addr2) &= ADDRESS_MAXWRAP((_regs)); \
  INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
}

/* Optimized RX decoder in case of zero X2 */
#undef  RXX0_BC
#define RXX0_BC(_inst, _regs, _b2, _effective_addr2) \
        RXX0_BC_DECODER(_inst, _regs, _b2, _effective_addr2)
#define RXX0_BC_DECODER(_inst, _regs, _b2, _effective_addr2) \
{ \
  U32 temp = fetch_fw(_inst); \
  (_effective_addr2) = temp & 0xfff; \
  (_b2) = (temp >> 12) & 0xf; \
  if(likely(_b2)) \
    (_effective_addr2) += (_regs)->GR((_b2)); \
}

/* Optimized RX decoder in case of zero X2 without r1 calculation */
#undef  RXX0RX
#undef  RX0X0RX
#define RXX0RX(_inst, _regs, _b2, _effective_addr2) \
        RXX0RX_DECODER(_inst, _regs, _b2, _effective_addr2, 4, 4)
#define RX0X0RX(_inst, _regs, _b2, _effective_addr2) \
        RXX0RX_DECODER(_inst, _regs, _b2, _effective_addr2, 4, 0)
#define RXX0RX_DECODER(_inst, _regs, _b2, _effective_addr2, _len, _ilc) \
{ \
  U32 temp = fetch_fw(_inst); \
  (_effective_addr2) = temp & 0xfff; \
  (_b2) = (temp >> 12) & 0xf; \
  if(likely(_b2)) \
    (_effective_addr2) += (_regs)->GR((_b2)); \
  if((_len)) \
    (_effective_addr2) &= ADDRESS_MAXWRAP((_regs)); \
  INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
}

/* Optimized RX decoder in case of non-zero X2 */
#undef RXXx
#undef RX0Xx
#define RXXx(_inst, _regs, _r1, _b2, _effective_addr2) \
        RXXx_DECODER(_inst, _regs, _r1, _b2, _effective_addr2, 4, 4)
#define RX0Xx(_inst, _regs, _r1, _b2, _effective_addr2) \
        RXXx_DECODER(_inst, _regs, _r1, _b2, _effective_addr2, 4, 0)
#define RXXx_DECODER(_inst, _regs, _r1, _b2, _effective_addr2, _len, _ilc) \
{ \
  U32 temp = fetch_fw(_inst); \
  (_r1) = (temp >> 20) & 0xf; \
  (_b2) = (temp >> 16) & 0xf; \
  (_effective_addr2) = (temp & 0xfff) + (_regs)->GR((_b2)); \
  (_b2) = (temp >> 12) & 0xf; \
  if(likely(_b2)) \
    (_effective_addr2) += (_regs)->GR((_b2)); \
  if((_len)) \
    (_effective_addr2) &= ADDRESS_MAXWRAP((_regs)); \
  INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
}

/* Optimized RX decoder in case of non-zero X2 */
#undef  RXXx_BC
#define RXXx_BC(_inst, _regs, _b2, _effective_addr2) \
        RXXx_BC_DECODER(_inst, _regs, _b2, _effective_addr2, 0, 0)
#define RXXx_BC_DECODER(_inst, _regs, _b2, _effective_addr2, _len, _ilc) \
{ \
  U32 temp = fetch_fw(_inst); \
  (_b2) = (temp >> 16) & 0xf; \
  (_effective_addr2) = (temp & 0xfff) + (_regs)->GR((_b2)); \
  (_b2) = (temp >> 12) & 0xf; \
  if(likely((_b2))) \
    (_effective_addr2) += (_regs)->GR((_b2)); \
}
#endif /* OPTION_OPTINST */

/* RX_BC register and indexed storage - optimized for BC */
#undef RX_BC

#define RX_BC(_inst, _regs, _b2, _effective_addr2) \
        RX_BC_DECODER(_inst, _regs, _b2, _effective_addr2, 0, 0)

#define RX_BC_DECODER(_inst, _regs, _b2, _effective_addr2, _len, _ilc) \
    {   U32 temp = fetch_fw(_inst); \
            (_effective_addr2) = temp & 0xfff; \
            (_b2) = (temp >> 16) & 0xf; \
            if(unlikely((_b2))) \
                (_effective_addr2) += (_regs)->GR((_b2)); \
            (_b2) = (temp >> 12) & 0xf; \
            if(likely((_b2))) \
                (_effective_addr2) += (_regs)->GR((_b2)); \
    }

/* RXE register and indexed storage with extended op code */
#undef RXE

#if !defined(DECODER_TEST)&&!defined(DECODER_TEST_RXE)
 #define RXE(_inst, _regs, _r1, _b2, _effective_addr2) \
         RXE_DECODER(_inst, _regs, _r1, _b2, _effective_addr2, 6, 6)
#else
 #define RXE(_inst, _regs, _r1, _b2, _effective_addr2) \
         RXE_DECODER_TEST(_inst, _regs, _r1, _b2, _effective_addr2, 6, 6)
#endif

#define RXE_DECODER(_inst, _regs, _r1, _b2, _effective_addr2, _len, _ilc) \
    {   U32 temp = fetch_fw(_inst); \
            (_r1) = (temp >> 20) & 0xf; \
            (_b2) = (temp >> 16) & 0xf; \
            (_effective_addr2) = temp & 0xfff; \
        if((_b2)) \
        { \
        (_effective_addr2) += (_regs)->GR((_b2)); \
        (_effective_addr2) &= ADDRESS_MAXWRAP((_regs)); \
        } \
            (_b2) = (temp >> 12) & 0xf; \
        if((_b2)) \
        { \
        (_effective_addr2) += (_regs)->GR((_b2)); \
        (_effective_addr2) &= ADDRESS_MAXWRAP((_regs)); \
        } \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
    }

#define RXE_DECODER_TEST(_inst, _regs, _r1, _b2, _effective_addr2, _len, _ilc) \
    {   U32 temp = fetch_fw(_inst); \
            (_effective_addr2) = temp & 0xfff; \
            (_b2) = (temp >> 16) & 0xf; \
            if((_b2)) \
                (_effective_addr2) += (_regs)->GR((_b2)); \
            (_b2) = (temp >> 12) & 0xf; \
            if((_b2)) \
                (_effective_addr2) += (_regs)->GR((_b2)); \
            (_effective_addr2) &= ADDRESS_MAXWRAP((_regs)); \
            (_r1) = (temp >> 20) & 0xf; \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
    }

/* RXF register and indexed storage with ext.opcode and additional R3 */
#undef RXF

#if !defined(DECODER_TEST)&&!defined(DECODER_TEST_RXF)
 #define RXF(_inst, _regs, _r1, _r3, _b2, _effective_addr2) \
         RXF_DECODER(_inst, _regs, _r1, _r3, _b2, _effective_addr2, 6, 6)
#else
 #define RXF(_inst, _regs, _r1, _r3, _b2, _effective_addr2) \
         RXF_DECODER_TEST(_inst, _regs, _r1, _r3, _b2, _effective_addr2, 6, 6)
#endif

#define RXF_DECODER(_inst, _regs, _r1, _r3, _b2, _effective_addr2, _len, _ilc) \
    {   U32 temp; \
        (_r1) = (_inst)[4] >> 4; \
            memcpy (&temp, (_inst), 4); \
            temp = CSWAP32(temp); \
            (_r3) = (temp >> 20) & 0xf; \
            (_b2) = (temp >> 16) & 0xf; \
            (_effective_addr2) = temp & 0xfff; \
        if((_b2)) \
        { \
        (_effective_addr2) += (_regs)->GR((_b2)); \
        (_effective_addr2) &= ADDRESS_MAXWRAP((_regs)); \
        } \
            (_b2) = (temp >> 12) & 0xf; \
        if((_b2)) \
        { \
        (_effective_addr2) += (_regs)->GR((_b2)); \
        (_effective_addr2) &= ADDRESS_MAXWRAP((_regs)); \
        } \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
    }

#define RXF_DECODER_TEST(_inst, _regs, _r1, _r3, _b2, _effective_addr2, _len, _ilc) \
    {   U32 temp = fetch_fw(_inst); \
            (_effective_addr2) = temp & 0xfff; \
            (_b2) = (temp >> 16) & 0xf; \
            if((_b2)) \
                (_effective_addr2) += (_regs)->GR((_b2)); \
            (_b2) = (temp >> 12) & 0xf; \
            if((_b2)) \
                (_effective_addr2) += (_regs)->GR((_b2)); \
            (_effective_addr2) &= ADDRESS_MAXWRAP((_regs)); \
            (_r3) = (temp >> 20) & 0xf; \
            (_r1) = (_inst)[4] >> 4; \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
    }

/* RXY register and indexed storage with extended op code
   and long displacement */
#undef RXY
#undef RXY0
#undef RXY_B

#if defined(FEATURE_LONG_DISPLACEMENT)
 #if !defined(DECODER_TEST)&&!defined(DECODER_TEST_RXY)
  #define RXY(_inst, _regs, _r1, _b2, _effective_addr2) \
          RXY_DECODER_LD(_inst, _regs, _r1, _b2, _effective_addr2, 6, 6)
  #define RXY0(_inst, _regs, _r1, _b2, _effective_addr2) \
          RXY_DECODER_LD(_inst, _regs, _r1, _b2, _effective_addr2, 6, 0)
  #define RXY_B(_inst, _regs, _r1, _b2, _effective_addr2) \
          RXY_DECODER_LD(_inst, _regs, _r1, _b2, _effective_addr2, 0, 0)
 #else
  #define RXY(_inst, _regs, _r1, _b2, _effective_addr2) \
          RXY_DECODER_LD_TEST(_inst, _regs, _r1, _b2, _effective_addr2, 6, 6)
  #define RXY0(_inst, _regs, _r1, _b2, _effective_addr2) \
          RXY_DECODER_LD_TEST(_inst, _regs, _r1, _b2, _effective_addr2, 6, 0)
  #define RXY_B(_inst, _regs, _r1, _b2, _effective_addr2) \
          RXY_DECODER_LD_TEST(_inst, _regs, _r1, _b2, _effective_addr2, 0, 0)
 #endif
#else /* !defined(FEATURE_LONG_DISPLACEMENT) */
 #if !defined(DECODER_TEST)&&!defined(DECODER_TEST_RXY)
  #define RXY(_inst, _regs, _r1, _b2, _effective_addr2) \
          RXY_DECODER(_inst, _regs, _r1, _b2, _effective_addr2, 6, 6)
  #define RXY0(_inst, _regs, _r1, _b2, _effective_addr2) \
          RXY_DECODER(_inst, _regs, _r1, _b2, _effective_addr2, 6, 0)
  #define RXY_B(_inst, _regs, _r1, _b2, _effective_addr2) \
          RXY_DECODER(_inst, _regs, _r1, _b2, _effective_addr2, 0, 0)
 #else
  #define RXY(_inst, _regs, _r1, _b2, _effective_addr2) \
          RXY_DECODER_TEST(_inst, _regs, _r1, _b2, _effective_addr2, 6, 6)
  #define RXY0(_inst, _regs, _r1, _b2, _effective_addr2) \
          RXY_DECODER_TEST(_inst, _regs, _r1, _b2, _effective_addr2, 6, 0)
  #define RXY_B(_inst, _regs, _r1, _b2, _effective_addr2) \
          RXY_DECODER_TEST(_inst, _regs, _r1, _b2, _effective_addr2, 0, 0)
 #endif
#endif

#define RXY_DECODER_LD(_inst, _regs, _r1, _b2, _effective_addr2, _len, _ilc) \
    {   U32 temp; S32 temp2; int tempx; \
            temp  = fetch_fw(_inst); \
            (_r1) = (temp >> 20) & 0xf; \
            tempx = (temp >> 16) & 0xf; \
            (_b2) = (temp >> 12) & 0xf; \
            temp2 = (_inst[4] << 12) | (temp & 0xfff); \
            if (temp2 & 0x80000) temp2 |= 0xfff00000; \
            (_effective_addr2) = \
                        (tempx ? (_regs)->GR(tempx) : (GREG)0) + \
                        ((_b2) ? (_regs)->GR((_b2)) : (GREG)0) + \
                        temp2; \
            if ((_len)) \
            (_effective_addr2) &= ADDRESS_MAXWRAP((_regs)); \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
    }

#define RXY_DECODER_LD_TEST(_inst, _regs, _r1, _b2, _effective_addr2, _len, _ilc) \
    {   U32 temp; S32 disp2; \
            temp  = fetch_fw(_inst); \
            (_effective_addr2) = 0; \
            (_b2) = (temp >> 16) & 0xf; \
            if ((_b2)) \
                (_effective_addr2) += (_regs)->GR((_b2)); \
            (_b2) = (temp >> 12) & 0xf; \
            if ((_b2)) \
                (_effective_addr2) += (_regs)->GR((_b2)); \
            disp2 = temp & 0xfff; \
            if (unlikely((_inst)[4])) { \
                disp2 |= (_inst[4] << 12); \
                if (disp2 & 0x80000) disp2 |= 0xfff00000; \
            } \
            (_effective_addr2) += disp2; \
            if ((_len)) \
            (_effective_addr2) &= ADDRESS_MAXWRAP((_regs)); \
            (_r1) = (temp >> 20) & 0xf; \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
    }

#define RXY_DECODER(_inst, _regs, _r1, _b2, _effective_addr2, _len, _ilc) \
    {   U32 temp = fetch_fw(_inst); \
            (_r1) = (temp >> 20) & 0xf; \
            (_b2) = (temp >> 16) & 0xf; \
            (_effective_addr2) = temp & 0xfff; \
        if((_b2)) \
        { \
        (_effective_addr2) += (_regs)->GR((_b2)); \
        if ((_len)) \
        (_effective_addr2) &= ADDRESS_MAXWRAP((_regs)); \
        } \
            (_b2) = (temp >> 12) & 0xf; \
        if((_b2)) \
        { \
        (_effective_addr2) += (_regs)->GR((_b2)); \
        if ((_len)) \
        (_effective_addr2) &= ADDRESS_MAXWRAP((_regs)); \
        } \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
    }

#define RXY_DECODER_TEST(_inst, _regs, _r1, _b2, _effective_addr2, _len, _ilc) \
    {   U32 temp = fetch_fw(_inst); \
            (_effective_addr2) = temp & 0xfff; \
            (_b2) = (temp >> 16) & 0xf; \
            if((_b2)) \
                (_effective_addr2) += (_regs)->GR((_b2)); \
            (_b2) = (temp >> 12) & 0xf; \
            if((_b2)) \
                (_effective_addr2) += (_regs)->GR((_b2)); \
            if ((_len)) \
            (_effective_addr2) &= ADDRESS_MAXWRAP((_regs)); \
            (_r1) = (temp >> 20) & 0xf; \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
    }

#ifdef OPTION_OPTINST
/* Optimized RXY decoder in case of zero X2 */
#undef RXYX0
#ifdef FEATURE_LONG_DISPLACEMENT
  #define RXYX0(_inst, _regs, _r1, _b2, _effective_addr2) \
          RXYX0_DECODER_LD(_inst, _regs, _r1, _b2, _effective_addr2, 6, 6)
#else
  #define RXYX0(_inst, _regs, _r1, _b2, _effective_addr2) \
          RXYX0_DECODER(_inst, _regs, _r1, _b2, _effective_addr2, 6, 6)
#endif /* #ifdef FEATURE_LONG_DISPLACEMENT */

#define RXYX0_DECODER_LD(_inst, _regs, _r1, _b2, _effective_addr2, _len, _ilc) \
{ \
  U32 temp; S32 disp2; \
  temp  = fetch_fw(_inst); \
  (_effective_addr2) = 0; \
  (_b2) = (temp >> 12) & 0xf; \
  if((_b2)) \
    (_effective_addr2) += (_regs)->GR((_b2)); \
  disp2 = temp & 0xfff; \
  if(unlikely((_inst)[4])) \
  { \
    disp2 |= (_inst[4] << 12); \
    if(disp2 & 0x80000) \
      disp2 |= 0xfff00000; \
  } \
  (_effective_addr2) += disp2; \
  if((_len)) \
    (_effective_addr2) &= ADDRESS_MAXWRAP((_regs)); \
  (_r1) = (temp >> 20) & 0xf; \
  INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
}

#define RXYX0_DECODER(_inst, _regs, _r1, _b2, _effective_addr2, _len, _ilc) \
{ \
  U32 temp = fetch_fw(_inst); \
  (_effective_addr2) = temp & 0xfff; \
  (_b2) = (temp >> 12) & 0xf; \
  if((_b2)) \
    (_effective_addr2) += (_regs)->GR((_b2)); \
  if((_len)) \
    (_effective_addr2) &= ADDRESS_MAXWRAP((_regs)); \
  (_r1) = (temp >> 20) & 0xf; \
  INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
}

#endif /* #ifdef OPTION_OPTINST */

/* RS register and storage with additional R3 or M3 field */
#undef RS
#undef RS0
#undef RS_B

#if !defined(DECODER_TEST)&&!defined(DECODER_TEST_RS)
 #define RS(_inst, _regs, _r1, _r3, _b2, _effective_addr2) \
         RS_DECODER(_inst, _regs, _r1, _r3, _b2, _effective_addr2, 4, 4)
 #define RS0(_inst, _regs, _r1, _r3, _b2, _effective_addr2) \
         RS_DECODER(_inst, _regs, _r1, _r3, _b2, _effective_addr2, 4, 0)
 #define RS_B(_inst, _regs, _r1, _r3, _b2, _effective_addr2) \
         RS_DECODER(_inst, _regs, _r1, _r3, _b2, _effective_addr2, 0, 0)
#else
 #define RS(_inst, _regs, _r1, _r3, _b2, _effective_addr2) \
         RS_DECODER_TEST(_inst, _regs, _r1, _r3, _b2, _effective_addr2, 4, 4)
 #define RS0(_inst, _regs, _r1, _r3, _b2, _effective_addr2) \
         RS_DECODER_TEST(_inst, _regs, _r1, _r3, _b2, _effective_addr2, 4, 0)
 #define RS_B(_inst, _regs, _r1, _r3, _b2, _effective_addr2) \
         RS_DECODER_TEST(_inst, _regs, _r1, _r3, _b2, _effective_addr2, 0, 0)
#endif

#define RS_DECODER(_inst, _regs, _r1, _r3, _b2, _effective_addr2, _len, _ilc) \
    {   U32 temp = fetch_fw(_inst); \
            (_r1) = (temp >> 20) & 0xf; \
            (_r3) = (temp >> 16) & 0xf; \
            (_b2) = (temp >> 12) & 0xf; \
            (_effective_addr2) = temp & 0xfff; \
        if((_b2)) \
        { \
        (_effective_addr2) += (_regs)->GR((_b2)); \
        if ((_len)) \
        (_effective_addr2) &= ADDRESS_MAXWRAP((_regs)); \
        } \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
    }

#define RS_DECODER_TEST(_inst, _regs, _r1, _r3, _b2, _effective_addr2, _len, _ilc) \
    {   U32 temp = fetch_fw(_inst); \
            (_effective_addr2) = temp & 0xfff; \
            (_b2) = (temp >> 12) & 0xf; \
            if((_b2)) \
            { \
                (_effective_addr2) += (_regs)->GR((_b2)); \
                if ((_len)) \
                (_effective_addr2) &= ADDRESS_MAXWRAP((_regs)); \
            } \
            (_r3) = (temp >> 16) & 0xf; \
            (_r1) = (temp >> 20) & 0xf; \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
    }

#ifdef OPTION_OPTINST
/* Optimized RS decoder without M3 calculation */
#define RSMX(_inst, _regs, _r1, _b2, _effective_addr2) \
        RSMX_DECODER(_inst, _regs, _r1, _b2, _effective_addr2, 4, 4)
#define RSMX_DECODER(_inst, _regs, _r1, _b2, _effective_addr2, _len, _ilc) \
{ \
  U32 temp = fetch_fw(_inst); \
  (_r1) = (temp >> 20) & 0xf; \
  (_b2) = (temp >> 12) & 0xf; \
  (_effective_addr2) = temp & 0xfff; \
  if((_b2)) \
  { \
    (_effective_addr2) += (_regs)->GR((_b2)); \
    if((_len)) \
      (_effective_addr2) &= ADDRESS_MAXWRAP((_regs)); \
  } \
  INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
}

/* Optimized RS decoder without R1, R3 calculation */
#define RSRR(_inst, _regs, _b2, _effective_addr2) \
        RSRR_DECODER(_inst, _regs, _b2, _effective_addr2, 4, 4)
#define RSRR_DECODER(_inst, _regs, _b2, _effective_addr2, _len, _ilc) \
{ \
  U32 temp = fetch_fw(_inst); \
  (_b2) = (temp >> 12) & 0xf; \
  (_effective_addr2) = temp & 0xfff; \
  if((_b2)) \
  { \
    (_effective_addr2) += (_regs)->GR((_b2)); \
    if((_len)) \
      (_effective_addr2) &= ADDRESS_MAXWRAP((_regs)); \
  } \
  INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
}
#endif /* #ifdef OPTION_OPTINST */


#if 0
/* RSE register and storage with extended op code and additional
   R3 or M3 field (note, this is NOT the ESA/390 vector RSE format) */
/* Note: Effective June 2003, RSE is retired and replaced by RSY */
#undef RSE
#define RSE(_inst, _regs, _r1, _r3, _b2, _effective_addr2) \
    {   U32 temp; \
            memcpy (&temp, (_inst), 4); \
            temp = CSWAP32(temp); \
            (_r1) = (temp >> 20) & 0xf; \
            (_r3) = (temp >> 16) & 0xf; \
            (_b2) = (temp >> 12) & 0xf; \
            (_effective_addr2) = temp & 0xfff; \
        if((_b2) != 0) \
        { \
        (_effective_addr2) += (_regs)->GR((_b2)); \
        (_effective_addr2) &= ADDRESS_MAXWRAP((_regs)); \
        } \
            INST_UPDATE_PSW((_regs), 6, 6); \
    }
#endif

/* RSY register and storage with extended op code, long displacement,
   and additional R3 or M3 field */
#undef RSY
#undef RSY0
#undef RSY_B

#if defined(FEATURE_LONG_DISPLACEMENT)
 #if !defined(DECODER_TEST)&&!defined(DECODER_TEST_RSY)
  #define RSY(_inst, _regs, _r1, _r3, _b2, _effective_addr2) \
          RSY_DECODER_LD(_inst, _regs, _r1, _r3, _b2, _effective_addr2, 6, 6)
  #define RSY0(_inst, _regs, _r1, _r3, _b2, _effective_addr2) \
          RSY_DECODER_LD(_inst, _regs, _r1, _r3, _b2, _effective_addr2, 6, 0)
  #define RSY_B(_inst, _regs, _r1, _r3, _b2, _effective_addr2) \
          RSY_DECODER_LD(_inst, _regs, _r1, _r3, _b2, _effective_addr2, 0, 0)
 #else
  #define RSY(_inst, _regs, _r1, _r3, _b2, _effective_addr2) \
          RSY_DECODER_LD_TEST(_inst, _regs, _r1, _r3, _b2, _effective_addr2, 6, 6)
  #define RSY0(_inst, _regs, _r1, _r3, _b2, _effective_addr2) \
          RSY_DECODER_LD_TEST(_inst, _regs, _r1, _r3, _b2, _effective_addr2, 6, 0)
  #define RSY_B(_inst, _regs, _r1, _r3, _b2, _effective_addr2) \
          RSY_DECODER_LD_TEST(_inst, _regs, _r1, _r3, _b2, _effective_addr2, 0, 0)
 #endif
#else
 #if !defined(DECODER_TEST)&&!defined(DECODER_TEST_RSY)
  #define RSY(_inst, _regs, _r1, _r3, _b2, _effective_addr2) \
          RSY_DECODER(_inst, _regs, _r1, _r3, _b2, _effective_addr2, 6, 6)
  #define RSY0(_inst, _regs, _r1, _r3, _b2, _effective_addr2) \
          RSY_DECODER(_inst, _regs, _r1, _r3, _b2, _effective_addr2, 6, 0)
  #define RSY_B(_inst, _regs, _r1, _r3, _b2, _effective_addr2) \
          RSY_DECODER(_inst, _regs, _r1, _r3, _b2, _effective_addr2, 0, 0)
 #else
  #define RSY(_inst, _regs, _r1, _r3, _b2, _effective_addr2) \
          RSY_DECODER_TEST(_inst, _regs, _r1, _r3, _b2, _effective_addr2, 6, 6)
  #define RSY0(_inst, _regs, _r1, _r3, _b2, _effective_addr2) \
          RSY_DECODER_TEST(_inst, _regs, _r1, _r3, _b2, _effective_addr2, 6, 0)
  #define RSY_B(_inst, _regs, _r1, _r3, _b2, _effective_addr2) \
          RSY_DECODER_TEST(_inst, _regs, _r1, _r3, _b2, _effective_addr2, 0, 0)
 #endif
#endif

#define RSY_DECODER_LD(_inst, _regs, _r1, _r3, _b2, _effective_addr2, _len, _ilc) \
    {   U32 temp; S32 temp2; \
            temp = fetch_fw(_inst); \
            (_r1) = (temp >> 20) & 0xf; \
            (_r3) = (temp >> 16) & 0xf; \
            (_b2) = (temp >> 12) & 0xf; \
            temp2 = (_inst[4] << 12) | (temp & 0xfff); \
            if (temp2 & 0x80000) temp2 |= 0xfff00000; \
            (_effective_addr2) = \
                        ((_b2) ? (_regs)->GR((_b2)) : (GREG)0) + \
                        temp2; \
            if ((_len)) \
            (_effective_addr2) &= ADDRESS_MAXWRAP((_regs)); \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
    }

#define RSY_DECODER_LD_TEST(_inst, _regs, _r1, _r3, _b2, _effective_addr2, _len, _ilc) \
    {   U32 temp; S32 disp2; \
            temp = fetch_fw(_inst); \
            (_effective_addr2) = 0; \
            (_b2) = (temp >> 12) & 0xf; \
            if ((_b2)) \
                _effective_addr2 += (_regs)->GR((_b2)); \
            disp2 = temp & 0xfff; \
            if (unlikely((_inst)[4])) { \
                disp2 |= (_inst[4] << 12); \
                if (disp2 & 0x80000) disp2 |= 0xfff00000; \
            } \
            (_effective_addr2) += disp2; \
            if ((_len)) \
            (_effective_addr2) &= ADDRESS_MAXWRAP((_regs)); \
            (_r3) = (temp >> 16) & 0xf; \
            (_r1) = (temp >> 20) & 0xf; \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
    }

#define RSY_DECODER(_inst, _regs, _r1, _r3, _b2, _effective_addr2, _len, _ilc) \
    {   U32 temp = fetch_fw(_inst); \
            (_r1) = (temp >> 20) & 0xf; \
            (_r3) = (temp >> 16) & 0xf; \
            (_b2) = (temp >> 12) & 0xf; \
            (_effective_addr2) = temp & 0xfff; \
        if((_b2) != 0) \
        { \
        (_effective_addr2) += (_regs)->GR((_b2)); \
        if ((_len)) \
        (_effective_addr2) &= ADDRESS_MAXWRAP((_regs)); \
        } \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
    }

#define RSY_DECODER_TEST(_inst, _regs, _r1, _r3, _b2, _effective_addr2, _len, _ilc) \
    {   U32 temp = fetch_fw(_inst); \
            (_effective_addr2) = temp & 0xfff; \
            (_b2) = (temp >> 12) & 0xf; \
            if((_b2)) \
                (_effective_addr2) += (_regs)->GR((_b2)); \
            if ((_len)) \
            (_effective_addr2) &= ADDRESS_MAXWRAP((_regs)); \
            (_r3) = (temp >> 16) & 0xf; \
            (_r1) = (temp >> 20) & 0xf; \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
    }

/* RSL storage operand with extended op code and 4-bit L field */
#undef RSL

#if !defined(DECODER_TEST)&&!defined(DECODER_TEST_RSL)
 #define RSL(_inst, _regs, _l1, _b1, _effective_addr1) \
         RSL_DECODER(_inst, _regs, _l1, _b1, _effective_addr1, 6, 6)
#else
 #define RSL(_inst, _regs, _l1, _b1, _effective_addr1) \
         RSL_DECODER_TEST(_inst, _regs, _l1, _b1, _effective_addr1, 6, 6)
#endif

#define RSL_DECODER(_inst, _regs, _l1, _b1, _effective_addr1, _len, _ilc) \
    {   U32 temp = fetch_fw(_inst); \
            (_l1) = (temp >> 20) & 0xf; \
            (_b1) = (temp >> 12) & 0xf; \
            (_effective_addr1) = temp & 0xfff; \
            if((_b1) != 0) \
            { \
                (_effective_addr1) += (_regs)->GR((_b1)); \
                (_effective_addr1) &= ADDRESS_MAXWRAP((_regs)); \
            } \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
        }

#define RSL_DECODER_TEST(_inst, _regs, _l1, _b1, _effective_addr1, _len, _ilc) \
    {   U32 temp = fetch_fw(_inst); \
            (_effective_addr1) = temp & 0xfff; \
            (_b1) = (temp >> 12) & 0xf; \
            if((_b1)) { \
                (_effective_addr1) += (_regs)->GR((_b1)); \
                (_effective_addr1) &= ADDRESS_MAXWRAP((_regs)); \
            } \
            (_l1) = (temp >> 20) & 0xf; \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
        }

/* RSL register and storage with extended op code, 8-bit L field, and mask */
#undef RSL_RM

#define RSL_RM(_inst, _regs, _r1, _l2, _b2, _effective_addr2, _m3) \
        RSL_RM_DECODER(_inst, _regs, _r1, _l2, _b2, _effective_addr2, _m3, 6, 6)

#define RSL_RM_DECODER(_inst, _regs, _r1, _l2, _b2, _effective_addr2, _m3, _len, _ilc) \
    {   U32 temp = fetch_fw(&(_inst)[1]); \
            (_m3) = temp & 0xf; \
            (_r1) = (temp >> 4) & 0xf; \
            (_effective_addr2) = (temp >> 8) & 0xfff; \
            (_b2) = (temp >> 20) & 0xf; \
            (_l2) = (temp >> 24) & 0xff; \
            if((_b2)) { \
                (_effective_addr2) += (_regs)->GR((_b2)); \
                (_effective_addr2) &= ADDRESS_MAXWRAP((_regs)); \
            } \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
    }

/* RSI register and immediate with additional R3 field */
#undef RSI
#undef RSI0

#if !defined(DECODER_TEST)&&!defined(DECODER_TEST_RSI)
 #define RSI(_inst, _regs, _r1, _r3, _i2) \
         RSI_DECODER(_inst, _regs, _r1, _r3, _i2, 4, 4)
 #define RSI0(_inst, _regs, _r1, _r3, _i2) \
         RSI_DECODER(_inst, _regs, _r1, _r3, _i2, 4, 0)
#else
 #define RSI(_inst, _regs, _r1, _r3, _i2) \
         RSI_DECODER_TEST(_inst, _regs, _r1, _r3, _i2, 4, 4)
 #define RSI0(_inst, _regs, _r1, _r3, _i2) \
         RSI_DECODER_TEST(_inst, _regs, _r1, _r3, _i2, 4, 0)
#endif

#define RSI_DECODER(_inst, _regs, _r1, _r3, _i2, _len, _ilc) \
    {   U32 temp = fetch_fw(_inst); \
            (_r1) = (temp >> 20) & 0xf; \
            (_r3) = (temp >> 16) & 0xf; \
            (_i2) = temp & 0xffff; \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
    }

#define RSI_DECODER_TEST(_inst, _regs, _r1, _r3, _i2, _len, _ilc) \
    {   U32 temp = fetch_fw(_inst); \
            (_i2) = temp & 0xffff; \
            (_r3) = (temp >> 16) & 0xf; \
            (_r1) = (temp >> 20) & 0xf; \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
    }

/* RI register and immediate with extended 4-bit op code */
#undef RI
#undef RI0
#undef RI_B

#if !defined(DECODER_TEST)&&!defined(DECODER_TEST_RI)
 #define RI(_inst, _regs, _r1, _op, _i2) \
         RI_DECODER(_inst, _regs, _r1, _op, _i2, 4, 4)
 #define RI0(_inst, _regs, _r1, _op, _i2) \
         RI_DECODER(_inst, _regs, _r1, _op, _i2, 4, 0)
 #define RI_B(_inst, _regs, _r1, _op, _i2) \
         RI_DECODER(_inst, _regs, _r1, _op, _i2, 0, 0)
#else
 #define RI(_inst, _regs, _r1, _op, _i2) \
         RI_DECODER_TEST(_inst, _regs, _r1, _op, _i2, 4, 4)
 #define RI0(_inst, _regs, _r1, _op, _i2) \
         RI_DECODER_TEST(_inst, _regs, _r1, _op, _i2, 4, 0)
 #define RI_B(_inst, _regs, _r1, _op, _i2) \
         RI_DECODER_TEST(_inst, _regs, _r1, _op, _i2, 0, 0)
#endif

#define RI_DECODER(_inst, _regs, _r1, _op, _i2, _len, _ilc) \
    {   U32 temp = fetch_fw(_inst); \
            (_r1) = (temp >> 20) & 0xf; \
            (_op) = (temp >> 16) & 0xf; \
            (_i2) = temp & 0xffff; \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
    }

#define RI_DECODER_TEST(_inst, _regs, _r1, _op, _i2) \
    {   U32 temp = fetch_fw(_inst); \
            (_op) = (temp >> 16) & 0xf; \
            (_i2) = temp & 0xffff; \
            (_r1) = (temp >> 20) & 0xf; \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
    }

/* RIE register and immediate with ext.opcode and additional R3 */
#undef RIE
#undef RIE0
#undef RIE_B

#define RIE(_inst, _regs, _r1, _r3, _i2) \
        RIE_DECODER(_inst, _regs, _r1, _r3, _i2, 6, 6)
#define RIE0(_inst, _regs, _r1, _r3, _i2) \
        RIE_DECODER(_inst, _regs, _r1, _r3, _i2, 6, 0)
#define RIE_B(_inst, _regs, _r1, _r3, _i2) \
        RIE_DECODER(_inst, _regs, _r1, _r3, _i2, 0, 0)

#define RIE_DECODER(_inst, _regs, _r1, _r3, _i2, _len, _ilc) \
    {   U32 temp = fetch_fw(_inst); \
            (_r1) = (temp >> 20) & 0xf; \
            (_r3) = (temp >> 16) & 0xf; \
            (_i2) = temp & 0xffff; \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
    }

/* RIE register and immediate with mask */                      /*208*/
#undef RIE_RIM

#define RIE_RIM(_inst, _regs, _r1, _i2, _m3) \
        RIE_RIM_DECODER(_inst, _regs, _r1, _i2, _m3, 6, 6)

#define RIE_RIM_DECODER(_inst, _regs, _r1, _i2, _m3, _len, _ilc) \
    {   U32 temp = fetch_fw(&(_inst)[1]); \
            (_m3) = (temp >> 4) & 0xf; \
            (_i2) = (temp >> 8) & 0xffff; \
            (_r1) = (temp >> 28) & 0xf; \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
    }

/* RIE register to register with immediate and mask */          /*208*/
#undef RIE_RRIM
#undef RIE_RRIM0
#undef RIE_RRIM_B

#define RIE_RRIM(_inst, _regs, _r1, _r2, _i4, _m3) \
        RIE_RRIM_DECODER(_inst, _regs, _r1, _r2, _i4, _m3, 6, 6)
#define RIE_RRIM0(_inst, _regs, _r1, _r2, _i4, _m3) \
        RIE_RRIM_DECODER(_inst, _regs, _r1, _r2, _i4, _m3, 6, 0)
#define RIE_RRIM_B(_inst, _regs, _r1, _r2, _i4, _m3) \
        RIE_RRIM_DECODER(_inst, _regs, _r1, _r2, _i4, _m3, 0, 0)

#define RIE_RRIM_DECODER(_inst, _regs, _r1, _r2, _i4, _m3, _len, _ilc) \
    {   U32 temp = fetch_fw(&(_inst)[1]); \
            (_m3) = (temp >> 4) & 0xf; \
            (_i4) = (temp >> 8) & 0xffff; \
            (_r2) = (temp >> 24) & 0xf; \
            (_r1) = (temp >> 28) & 0xf; \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
    }

/* RIE register and mask with longer immediate and immediate */ /*208*/
#undef RIE_RMII
#undef RIE_RMII0
#undef RIE_RMII_B

#define RIE_RMII(_inst, _regs, _r1, _i2, _m3, _i4) \
        RIE_RMII_DECODER(_inst, _regs, _r1, _i2, _m3, _i4, 6, 6)
#define RIE_RMII0(_inst, _regs, _r1, _i2, _m3, _i4) \
        RIE_RMII_DECODER(_inst, _regs, _r1, _i2, _m3, _i4, 6, 0)
#define RIE_RMII_B(_inst, _regs, _r1, _i2, _m3, _i4) \
        RIE_RMII_DECODER(_inst, _regs, _r1, _i2, _m3, _i4, 0, 0)

#define RIE_RMII_DECODER(_inst, _regs, _r1, _i2, _m3, _i4, _len, _ilc) \
    {   U32 temp = fetch_fw(&(_inst)[1]); \
            (_i2) = temp & 0xff; \
            (_i4) = (temp >> 8) & 0xffff; \
            (_m3) = (temp >> 24) & 0xf; \
            (_r1) = (temp >> 28) & 0xf; \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
    }

/* RIE register to register with three immediate fields */      /*208*/
#undef RIE_RRIII

#define RIE_RRIII(_inst, _regs, _r1, _r2, _i3, _i4, _i5) \
        RIE_RRIII_DECODER(_inst, _regs, _r1, _r2, _i3, _i4, _i5, 6, 6)

#define RIE_RRIII_DECODER(_inst, _regs, _r1, _r2, _i3, _i4, _i5, _len, _ilc) \
    {   U32 temp = fetch_fw(&(_inst)[1]); \
            (_i5) = temp & 0xff; \
            (_i4) = (temp >> 8) & 0xff; \
            (_i3) = (temp >> 16) & 0xff; \
            (_r2) = (temp >> 24) & 0xf; \
            (_r1) = (temp >> 28) & 0xf; \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
    }

/* RIL register and longer immediate with extended 4 bit op code */
#undef RIL
#undef RIL0
#undef RIL_B

#if !defined(DECODER_TEST)&&!defined(DECODER_TEST_RIL)
 #define RIL(_inst, _regs, _r1, _op, _i2) \
         RIL_DECODER(_inst, _regs, _r1, _op, _i2, 6, 6)
 #define RIL0(_inst, _regs, _r1, _op, _i2) \
         RIL_DECODER(_inst, _regs, _r1, _op, _i2, 6, 0)
 #define RIL_B(_inst, _regs, _r1, _op, _i2) \
         RIL_DECODER(_inst, _regs, _r1, _op, _i2, 0, 0)
#else
 #define RIL(_inst, _regs, _r1, _op, _i2) \
         RIL_DECODER_TEST(_inst, _regs, _r1, _op, _i2, 6, 6)
 #define RIL0(_inst, _regs, _r1, _op, _i2) \
         RIL_DECODER_TEST(_inst, _regs, _r1, _op, _i2, 6, 0)
 #define RIL_B(_inst, _regs, _r1, _op, _i2) \
         RIL_DECODER_TEST(_inst, _regs, _r1, _op, _i2, 0, 0)
#endif

#define RIL_DECODER(_inst, _regs, _r1, _op, _i2, _len, _ilc) \
    {   U32 temp = fetch_fw(_inst); \
            (_r1) = (temp >> 20) & 0xf; \
            (_op) = (temp >> 16) & 0xf; \
            (_i2) = ((temp & 0xffff) << 16) \
          | ((_inst)[4] << 8) \
          | (_inst)[5]; \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
    }

#define RIL_DECODER_TEST(_inst, _regs, _r1, _op, _i2, _len, _ilc) \
    { \
            (_i2) = fetch_fw(&(_inst)[2]); \
            (_op) = ((_inst)[1]     ) & 0xf; \
            (_r1) = ((_inst)[1] >> 4) & 0xf; \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
    }

/* RIL register and longer immediate relative address */
#undef RIL_A

#if !defined(DECODER_TEST)&&!defined(DECODER_TEST_RIL_A)
 #define RIL_A(_inst, _regs, _r1, _addr2) \
         RIL_A_DECODER(_inst, _regs, _r1, _addr2, 6, 6)
#else
 #define RIL_A(_inst, _regs, _r1, _addr2) \
         RIL_A_DECODER_TEST(_inst, _regs, _r1, _addr2, 6, 6)
#endif

#define RIL_A_DECODER(_inst, _regs, _r1, _addr2, _len, _ilc) \
    {   U32 temp = fetch_fw(_inst); \
        S64 offset; \
            (_r1) = (temp >> 20) & 0xf; \
            offset = 2LL*(S32)(((temp & 0xffff) << 16) \
                    | ((_inst)[4] << 8) \
                    | (_inst)[5]); \
            (_addr2) = (likely(!(_regs)->execflag)) ? \
                    PSW_IA((_regs), offset) : \
                    ((_regs)->ET + offset) & ADDRESS_MAXWRAP((_regs)); \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
    }

#define RIL_A_DECODER_TEST(_inst, _regs, _r1, _addr2, _len, _ilc) \
    { \
        S64 offset = 2LL*(S32)(fetch_fw(&(_inst)[2])); \
            (_r1) = ((_inst)[1] >> 4) & 0xf; \
            (_addr2) = (likely(!(_regs)->execflag)) ? \
                    PSW_IA((_regs), offset) : \
                    ((_regs)->ET + offset) & ADDRESS_MAXWRAP((_regs)); \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
    }

/* RIS register, immediate, mask, and storage */                /*208*/
#undef RIS
#undef RIS0
#undef RIS_B

#if !defined(DECODER_TEST)&&!defined(DECODER_TEST_RIS)
 #define RIS(_inst, _regs, _r1, _i2, _m3, _b4, _effective_addr4) \
         RIS_DECODER(_inst, _regs, _r1, _i2, _m3, _b4, _effective_addr4, 6, 6)
 #define RIS0(_inst, _regs, _r1, _i2, _m3, _b4, _effective_addr4) \
         RIS_DECODER(_inst, _regs, _r1, _i2, _m3, _b4, _effective_addr4, 6, 0)
 #define RIS_B(_inst, _regs, _r1, _i2, _m3, _b4, _effective_addr4) \
         RIS_DECODER(_inst, _regs, _r1, _i2, _m3, _b4, _effective_addr4, 0, 0)
#else
 #define RIS(_inst, _regs, _r1, _i2, _m3, _b4, _effective_addr4) \
         RIS_DECODER_TEST(_inst, _regs, _r1, _i2, _m3, _b4, _effective_addr4, 6, 6)
 #define RISO(_inst, _regs, _r1, _i2, _m3, _b4, _effective_addr4) \
         RIS_DECODER_TEST(_inst, _regs, _r1, _i2, _m3, _b4, _effective_addr4, 6, 0)
 #define RIS_B(_inst, _regs, _r1, _i2, _m3, _b4, _effective_addr4) \
         RIS_DECODER_TEST(_inst, _regs, _r1, _i2, _m3, _b4, _effective_addr4, 0, 0)
#endif

#define RIS_DECODER(_inst, _regs, _r1, _i2, _m3, _b4, _effective_addr4, _len, _ilc) \
    {   U32 temp = fetch_fw(_inst); \
            (_effective_addr4) = temp & 0xfff; \
            (_b4) = (temp >> 12) & 0xf; \
            if((_b4) != 0) \
            { \
                (_effective_addr4) += (_regs)->GR((_b4)); \
                (_effective_addr4) &= ADDRESS_MAXWRAP((_regs)); \
            } \
            (_m3) = (temp >> 16) & 0xf; \
            (_r1) = (temp >> 20) & 0xf; \
            (_i2) = (_inst)[4]; \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
    }

#define RIS_DECODER_TEST(_inst, _regs, _r1, _i2, _m3, _b4, _effective_addr4, _len, _ilc) \
    {   U32 temp = fetch_fw(_inst); \
            (_effective_addr4) = temp & 0xfff; \
            (_b4) = (temp >> 12) & 0xf; \
            if((_b4)) { \
                (_effective_addr4) += (_regs)->GR((_b4)); \
                (_effective_addr4) &= ADDRESS_MAXWRAP((_regs)); \
            } \
            (_m3) = (temp >> 16) & 0xf; \
            (_r1) = (temp >> 20) & 0xf; \
            (_i2) = (_inst)[4]; \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
    }

/* RRS register, immediate, mask, and storage */                /*208*/
#undef RRS
#undef RRS0
#undef RRS_B

#if !defined(DECODER_TEST)&&!defined(DECODER_TEST_RRS)
 #define RRS(_inst, _regs, _r1, _r2, _m3, _b4, _effective_addr4) \
         RRS_DECODER(_inst, _regs, _r1, _r2, _m3, _b4, _effective_addr4, 6, 6)
 #define RRS0(_inst, _regs, _r1, _r2, _m3, _b4, _effective_addr4) \
         RRS_DECODER(_inst, _regs, _r1, _r2, _m3, _b4, _effective_addr4, 6, 0)
 #define RRS_B(_inst, _regs, _r1, _r2, _m3, _b4, _effective_addr4) \
         RRS_DECODER(_inst, _regs, _r1, _r2, _m3, _b4, _effective_addr4, 0, 0)
#else
 #define RRS(_inst, _regs, _r1, _r2, _m3, _b4, _effective_addr4) \
         RRS_DECODER_TEST(_inst, _regs, _r1, _r2, _m3, _b4, _effective_addr4, 6, 6)
 #define RRS0(_inst, _regs, _r1, _r2, _m3, _b4, _effective_addr4) \
         RRS_DECODER_TEST(_inst, _regs, _r1, _r2, _m3, _b4, _effective_addr4, 6, 0)
 #define RRS_B(_inst, _regs, _r1, _r2, _m3, _b4, _effective_addr4) \
         RRS_DECODER_TEST(_inst, _regs, _r1, _r2, _m3, _b4, _effective_addr4, 0, 0)
#endif

#define RRS_DECODER(_inst, _regs, _r1, _r2, _m3, _b4, _effective_addr4, _len, _ilc) \
    {   U32 temp = fetch_fw(_inst); \
            (_effective_addr4) = temp & 0xfff; \
            (_b4) = (temp >> 12) & 0xf; \
            if((_b4) != 0) \
            { \
                (_effective_addr4) += (_regs)->GR((_b4)); \
                (_effective_addr4) &= ADDRESS_MAXWRAP((_regs)); \
            } \
            (_r2) = (temp >> 16) & 0xf; \
            (_r1) = (temp >> 20) & 0xf; \
            (_m3) = ((_inst)[4] >> 4) & 0xf; \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
    }

#define RRS_DECODER_TEST(_inst, _regs, _r1, _r2, _m3, _b4, _effective_addr4, _len, _ilc) \
    {   U32 temp = fetch_fw(_inst); \
            (_effective_addr4) = temp & 0xfff; \
            (_b4) = (temp >> 12) & 0xf; \
            if((_b4)) { \
                (_effective_addr4) += (_regs)->GR((_b4)); \
                (_effective_addr4) &= ADDRESS_MAXWRAP((_regs)); \
            } \
            (_r2) = (temp >> 16) & 0xf; \
            (_r1) = (temp >> 20) & 0xf; \
            (_m3) = ((_inst)[4] >> 4) & 0xf; \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
    }

/* SI storage and immediate */
#undef SI

#if !defined(DECODER_TEST)&&!defined(DECODER_TEST_SI)
 #define SI(_inst, _regs, _i2, _b1, _effective_addr1) \
         SI_DECODER(_inst, _regs, _i2, _b1, _effective_addr1, 4, 4)
#else
 #define SI(_inst, _regs, _i2, _b1, _effective_addr1) \
         SI_DECODER_TEST(_inst, _regs, _i2, _b1, _effective_addr1, 4, 4)
#endif

#define SI_DECODER(_inst, _regs, _i2, _b1, _effective_addr1, _len, _ilc) \
    {   U32 temp = fetch_fw(_inst); \
            (_i2) = (temp >> 16) & 0xff; \
            (_b1) = (temp >> 12) & 0xf; \
            (_effective_addr1) = temp & 0xfff; \
        if((_b1) != 0) \
        { \
        (_effective_addr1) += (_regs)->GR((_b1)); \
        (_effective_addr1) &= ADDRESS_MAXWRAP((_regs)); \
        } \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
    }

#define SI_DECODER_TEST(_inst, _regs, _i2, _b1, _effective_addr1, _len, _ilc) \
    {   U32 temp = fetch_fw(_inst); \
            (_effective_addr1) = temp & 0xfff; \
            (_b1) = (temp >> 12) & 0xf; \
            if((_b1)) { \
                (_effective_addr1) += (_regs)->GR((_b1)); \
                (_effective_addr1) &= ADDRESS_MAXWRAP((_regs)); \
            } \
            (_i2) = (temp >> 16) & 0xff; \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
    }

#ifdef OPTION_OPTINST
/* Optimized SI decoder without i2 calculation */
#undef SIIX
#define SIIX(_inst, _regs, _b1, _effective_addr1) \
        SIIX_DECODER(_inst, _regs, _b1, _effective_addr1, 4, 4)
#define SIIX_DECODER(_inst, _regs, _b1, _effective_addr1, _len, _ilc) \
{ \
  U32 temp = fetch_fw(_inst); \
  (_effective_addr1) = temp & 0xfff; \
  (_b1) = (temp >> 12) & 0xf; \
  if((_b1)) \
  { \
    (_effective_addr1) += (_regs)->GR((_b1)); \
    (_effective_addr1) &= ADDRESS_MAXWRAP((_regs)); \
  } \
  INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
}
#endif /* #ifdef OPTION_OPTINST */

/* SIY storage and immediate with long displacement */
#undef SIY

#if defined(FEATURE_LONG_DISPLACEMENT)
 #if !defined(DECODER_TEST)&&!defined(DECODER_TEST_SIY)
  #define SIY(_inst, _regs, _i2, _b1, _effective_addr1) \
          SIY_DECODER_LD(_inst, _regs, _i2, _b1, _effective_addr1, 6, 6)
 #else
  #define SIY(_inst, _regs, _i2, _b1, _effective_addr1) \
          SIY_DECODER_LD_TEST(_inst, _regs, _i2, _b1, _effective_addr1, 6, 6)
 #endif
#endif /* defined(FEATURE_LONG_DISPLACEMENT) */

#define SIY_DECODER_LD(_inst, _regs, _i2, _b1, _effective_addr1, _len, _ilc) \
    {   U32 temp; S32 temp1; \
            temp = fetch_fw(_inst); \
            (_i2) = (temp >> 16) & 0xff; \
            (_b1) = (temp >> 12) & 0xf; \
            temp1 = (_inst[4] << 12) | (temp & 0xfff); \
            if (temp1 & 0x80000) temp1 |= 0xfff00000; \
            (_effective_addr1) = \
                        ((_b1) ? (_regs)->GR((_b1)) : (GREG)0) + \
                        temp1; \
            (_effective_addr1) &= ADDRESS_MAXWRAP((_regs)); \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
    }

#define SIY_DECODER_LD_TEST(_inst, _regs, _i2, _b1, _effective_addr1, _len, _ilc) \
    {   U32 temp; S32 disp; \
            temp = fetch_fw(_inst); \
            (_effective_addr1) = 0; \
            (_b1) = (temp >> 12) & 0xf; \
            if ((_b1)) \
                (_effective_addr1) += (_regs)->GR((_b1)); \
            disp = temp & 0xfff; \
            if (unlikely((_inst)[4])) { \
                disp |= (_inst[4] << 12); \
                if (disp & 0x80000) disp |= 0xfff00000; \
            } \
            (_effective_addr1) += disp; \
            (_effective_addr1) &= ADDRESS_MAXWRAP((_regs)); \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
            (_i2) = (temp >> 16) & 0xff; \
    }

/* SIL storage and longer immediate */                          /*208*/
#undef SIL

#if !defined(DECODER_TEST)&&!defined(DECODER_TEST_SIL)
 #define SIL(_inst, _regs, _i2, _b1, _effective_addr1) \
         SIL_DECODER(_inst, _regs, _i2, _b1, _effective_addr1, 6, 6)
#else
 #define SIL(_inst, _regs, _i2, _b1, _effective_addr1) \
         SIL_DECODER_TEST(_inst, _regs, _i2, _b1, _effective_addr1, 6, 6)
#endif

#define SIL_DECODER(_inst, _regs, _i2, _b1, _effective_addr1, _len, _ilc) \
    {   U32 temp = fetch_fw(&(_inst)[2]); \
            (_i2) = temp & 0xffff; \
            (_effective_addr1) = (temp >> 16) & 0xfff; \
            (_b1) = (temp >> 28) & 0xf; \
            if((_b1) != 0) \
            { \
                (_effective_addr1) += (_regs)->GR((_b1)); \
                (_effective_addr1) &= ADDRESS_MAXWRAP((_regs)); \
            } \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
    }

#define SIL_DECODER_TEST(_inst, _regs, _i2, _b1, _effective_addr1, _len, _ilc) \
    {   U32 temp = fetch_fw(&(_inst)[2]); \
            (_i2) = temp & 0xffff; \
            (_effective_addr1) = (temp >> 16) & 0xfff; \
            (_b1) = (temp >> 28) & 0xf; \
            if((_b1) != 0) \
            { \
                (_effective_addr1) += (_regs)->GR((_b1)); \
                (_effective_addr1) &= ADDRESS_MAXWRAP((_regs)); \
            } \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
    }

/* SMI storage with mask and 16-bit relative address */         /*912*/
#undef SMI_A
#undef SMI_A0

#define SMI_A(_inst, _regs, _m1, _addr2, _b3, _addr3) \
        SMI_A_DECODER(_inst, _regs, _m1, _addr2, _b3, _addr3, 6, 6)
#define SMI_A0(_inst, _regs, _m1, _addr2, _b3, _addr3) \
        SMI_A_DECODER(_inst, _regs, _m1, _addr2, _b3, _addr3, 6, 0)

#define SMI_A_DECODER(_inst, _regs, _m1, _addr2, _b3, _addr3, _len, _ilc) \
    { \
            U32 ri2; S64 offset; \
            U32 temp = fetch_fw(&(_inst)[2]); \
            int i = (_inst)[1]; \
            (_m1) = (i >> 4) & 0x0F; \
            ri2 = temp & 0xFFFF; \
            (_addr3) = (temp >> 16) & 0xFFF; \
            (_b3) = (temp >> 28) & 0x0F; \
            if((_b3)) \
            { \
                (_addr3) += (_regs)->GR((_b3)); \
                (_addr3) &= ADDRESS_MAXWRAP((_regs)); \
            } \
            offset = 2LL*(S32)ri2; \
            (_addr2) = (likely(!(_regs)->execflag)) ? \
                    PSW_IA((_regs), offset) : \
                    ((_regs)->ET + offset) & ADDRESS_MAXWRAP((_regs)); \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
    }

/* S storage operand only */
#undef S

#if !defined(DECODER_TEST)&&!defined(DECODER_TEST_S)
 #define S(_inst, _regs, _b2, _effective_addr2) \
         S_DECODER(_inst, _regs, _b2, _effective_addr2, 4, 4)
#else
 #define S(_inst, _regs, _b2, _effective_addr2) \
         S_DECODER_TEST(_inst, _regs, _b2, _effective_addr2, 4, 4)
#endif

#define S_DECODER(_inst, _regs, _b2, _effective_addr2, _len, _ilc) \
    {   U32 temp = fetch_fw(_inst); \
            (_b2) = (temp >> 12) & 0xf; \
            (_effective_addr2) = temp & 0xfff; \
        if((_b2) != 0) \
        { \
        (_effective_addr2) += (_regs)->GR((_b2)); \
        (_effective_addr2) &= ADDRESS_MAXWRAP((_regs)); \
        } \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
    }

#define S_DECODER_TEST(_inst, _regs, _b2, _effective_addr2, _len, _ilc) \
    {   U32 temp = fetch_fw(_inst); \
            (_effective_addr2) = temp & 0xfff; \
            (_b2) = (temp >> 12) & 0xf; \
            if((_b2) != 0) { \
                (_effective_addr2) += (_regs)->GR((_b2)); \
                (_effective_addr2) &= ADDRESS_MAXWRAP((_regs)); \
            } \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
    }

/* SS storage to storage with two 4-bit L or R fields */
#undef SS

#if !defined(DECODER_TEST)&&!defined(DECODER_TEST_SS)
 #define SS(_inst, _regs, _r1, _r3, \
            _b1, _effective_addr1, _b2, _effective_addr2) \
         SS_DECODER(_inst, _regs, _r1, _r3, \
            _b1, _effective_addr1, _b2, _effective_addr2, 6, 6)
#else
 #define SS(_inst, _regs, _r1, _r3, \
            _b1, _effective_addr1, _b2, _effective_addr2) \
         SS_DECODER_TEST(_inst, _regs, _r1, _r3, \
            _b1, _effective_addr1, _b2, _effective_addr2, 6, 6)
#endif

#define SS_DECODER(_inst, _regs, _r1, _r3, \
           _b1, _effective_addr1, _b2, _effective_addr2, _len, _ilc) \
    {   U32 temp = fetch_fw(_inst); \
            (_r1) = (temp >> 20) & 0xf; \
            (_r3) = (temp >> 16) & 0xf; \
            (_b1) = (temp >> 12) & 0xf; \
            (_effective_addr1) = temp & 0xfff; \
        if((_b1) != 0) \
        { \
        (_effective_addr1) += (_regs)->GR((_b1)); \
        (_effective_addr1) &= ADDRESS_MAXWRAP((_regs)); \
        } \
        (_b2) = (_inst)[4] >> 4; \
        (_effective_addr2) = (((_inst)[4] & 0x0F) << 8) | (_inst)[5]; \
        if((_b2) != 0) \
        { \
        (_effective_addr2) += (_regs)->GR((_b2)); \
        (_effective_addr2) &= ADDRESS_MAXWRAP((_regs)); \
        } \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
    }

#define SS_DECODER_TEST(_inst, _regs, _r1, _r3, \
           _b1, _effective_addr1, _b2, _effective_addr2, _len, _ilc) \
    {   U32 temp; \
            temp = fetch_fw((_inst)+2); \
            (_effective_addr1) = (temp >> 16) & 0xfff; \
            (_b1) = (temp >> 28); \
            if ((_b1)) { \
                (_effective_addr1) += (_regs)->GR((_b1)); \
                (_effective_addr1) &= ADDRESS_MAXWRAP((_regs)); \
            } \
            (_effective_addr2) = temp & 0xfff; \
            (_b2) = (temp >> 12) & 0xf; \
            if ((_b2)) { \
                (_effective_addr2) += (_regs)->GR((_b2)); \
                (_effective_addr2) &= ADDRESS_MAXWRAP((_regs)); \
            } \
            (_r3) = ((_inst)[1]     ) & 0xf; \
            (_r1) = ((_inst)[1] >> 4) & 0xf; \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
    }

/* SS storage to storage with one 8-bit L field */
#undef SS_L

#if !defined(DECODER_TEST)&&!defined(DECODER_TEST_SS_L)
 #define SS_L(_inst, _regs, _l, \
              _b1, _effective_addr1, _b2, _effective_addr2) \
         SS_L_DECODER(_inst, _regs, _l, \
              _b1, _effective_addr1, _b2, _effective_addr2, 6, 6)
#else
 #define SS_L(_inst, _regs, _l, \
              _b1, _effective_addr1, _b2, _effective_addr2) \
         SS_L_DECODER_TEST(_inst, _regs, _l, \
              _b1, _effective_addr1, _b2, _effective_addr2, 6, 6)
#endif

#define SS_L_DECODER(_inst, _regs, _l, \
           _b1, _effective_addr1, _b2, _effective_addr2, _len, _ilc) \
    {   U32 temp = fetch_fw(_inst); \
            (_l) = (temp >> 16) & 0xff; \
            (_b1) = (temp >> 12) & 0xf; \
            (_effective_addr1) = temp & 0xfff; \
        if((_b1) != 0) \
        { \
        (_effective_addr1) += (_regs)->GR((_b1)); \
        (_effective_addr1) &= ADDRESS_MAXWRAP((_regs)); \
        } \
        (_b2) = (_inst)[4] >> 4; \
        (_effective_addr2) = (((_inst)[4] & 0x0F) << 8) | (_inst)[5]; \
        if((_b2) != 0) \
        { \
        (_effective_addr2) += (_regs)->GR((_b2)); \
        (_effective_addr2) &= ADDRESS_MAXWRAP((_regs)); \
        } \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
    }

#define SS_L_DECODER_TEST(_inst, _regs, _l, \
           _b1, _effective_addr1, _b2, _effective_addr2, _len, _ilc) \
    {   U32 temp; \
            temp = fetch_fw((_inst)+2); \
            (_effective_addr1) = (temp >> 16) & 0xfff; \
            (_b1) = (temp >> 28); \
            if((_b1)) { \
                (_effective_addr1) += (_regs)->GR((_b1)); \
                (_effective_addr1) &= ADDRESS_MAXWRAP((_regs)); \
            } \
            (_effective_addr2) = temp & 0xfff; \
            (_b2) = (temp >> 12) & 0xf; \
            if ((_b2)) { \
                (_effective_addr2) += (_regs)->GR((_b2)); \
                (_effective_addr2) &= ADDRESS_MAXWRAP((_regs)); \
            } \
            (_l) = (_inst)[1]; \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
    }

#ifdef OPTION_OPTINST
#define SS_LXL(_inst, _regs, _b1, _effective_addr1, _b2, _effective_addr2) \
        SS_LXL_DECODER(_inst, _regs, _b1, _effective_addr1, _b2, _effective_addr2, 6, 6)
#define SS_LXL_DECODER(_inst, _regs, _b1, _effective_addr1, _b2, _effective_addr2, _len, _ilc) \
{ \
  U32 temp = fetch_fw(_inst); \
  (_effective_addr1) = temp & 0xfff; \
  (_b1) = (temp >> 12) & 0xf; \
  if((_b1)) \
  { \
    (_effective_addr1) += (_regs)->GR((_b1)); \
    (_effective_addr1) &= ADDRESS_MAXWRAP((_regs)); \
  } \
  (_effective_addr2) = (((_inst)[4] & 0x0F) << 8) | (_inst)[5]; \
  (_b2) = (_inst)[4] >> 4; \
  if((_b2)) \
  { \
    (_effective_addr2) += (_regs)->GR((_b2)); \
    (_effective_addr2) &= ADDRESS_MAXWRAP((_regs)); \
  } \
  INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
}
#endif /* #ifdef OPTION_OPTINST */

/* SSE storage to storage with extended op code */
#undef SSE

#if !defined(DECODER_TEST)&&!defined(DECODER_TEST_SSE)
 #define SSE(_inst, _regs, _b1, _effective_addr1, \
                     _b2, _effective_addr2) \
         SSE_DECODER(_inst, _regs, _b1, _effective_addr1, \
                     _b2, _effective_addr2, 6, 6)
#else
 #define SSE(_inst, _regs, _b1, _effective_addr1, \
                     _b2, _effective_addr2) \
         SSE_DECODER_TEST(_inst, _regs, _b1, _effective_addr1, \
                     _b2, _effective_addr2, 6, 6)
#endif

#define SSE_DECODER(_inst, _regs, _b1, _effective_addr1, \
                     _b2, _effective_addr2, _len, _ilc) \
    {   U32 temp = fetch_fw(_inst); \
            (_b1) = (temp >> 12) & 0xf; \
            (_effective_addr1) = temp & 0xfff; \
        if((_b1) != 0) \
        { \
        (_effective_addr1) += (_regs)->GR((_b1)); \
        (_effective_addr1) &= ADDRESS_MAXWRAP((_regs)); \
        } \
        (_b2) = (_inst)[4] >> 4; \
        (_effective_addr2) = (((_inst)[4] & 0x0F) << 8) | (_inst)[5]; \
        if((_b2) != 0) \
        { \
        (_effective_addr2) += (_regs)->GR((_b2)); \
        (_effective_addr2) &= ADDRESS_MAXWRAP((_regs)); \
        } \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
    }

#define SSE_DECODER_TEST(_inst, _regs, _b1, _effective_addr1, \
                     _b2, _effective_addr2, _len, _ilc) \
    {   U32 temp = fetch_fw((_inst)+2); \
            (_effective_addr1) = (temp >> 16) & 0xfff; \
            (_b1) = (temp >> 28); \
            if((_b1)) { \
                (_effective_addr1) += (_regs)->GR((_b1)); \
                (_effective_addr1) &= ADDRESS_MAXWRAP((_regs)); \
            } \
            (_effective_addr2) = temp & 0xfff; \
            (_b2) = (temp >> 12) & 0xf; \
            if ((_b2)) { \
                (_effective_addr2) += (_regs)->GR((_b2)); \
                (_effective_addr2) &= ADDRESS_MAXWRAP((_regs)); \
            } \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
    }

/* SSF storage to storage with additional register */
#undef SSF

#if !defined(DECODER_TEST)&&!defined(DECODER_TEST_SSF)
 #define SSF(_inst, _regs, _b1, _effective_addr1, \
                     _b2, _effective_addr2, _r3) \
         SSF_DECODER(_inst, _regs, _b1, _effective_addr1, \
                     _b2, _effective_addr2, _r3, 6, 6)
#else
 #define SSF(_inst, _regs, _b1, _effective_addr1, \
                     _b2, _effective_addr2, _r3) \
         SSF_DECODER_TEST(_inst, _regs, _b1, _effective_addr1, \
                     _b2, _effective_addr2, _r3, 6, 6)
#endif

#define SSF_DECODER(_inst, _regs, _b1, _effective_addr1, \
                     _b2, _effective_addr2, _r3, _len, _ilc) \
    {   U32 temp = fetch_fw(_inst); \
            (_r3) = (temp >> 20) & 0xf; \
            (_b1) = (temp >> 12) & 0xf; \
            (_effective_addr1) = temp & 0xfff; \
        if((_b1) != 0) \
        { \
        (_effective_addr1) += (_regs)->GR((_b1)); \
        (_effective_addr1) &= ADDRESS_MAXWRAP((_regs)); \
        } \
        (_b2) = (_inst)[4] >> 4; \
        (_effective_addr2) = (((_inst)[4] & 0x0F) << 8) | (_inst)[5]; \
        if((_b2) != 0) \
        { \
        (_effective_addr2) += (_regs)->GR((_b2)); \
        (_effective_addr2) &= ADDRESS_MAXWRAP((_regs)); \
        } \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
    }

#define SSF_DECODER_TEST(_inst, _regs, _b1, _effective_addr1, \
                     _b2, _effective_addr2, _r3, _len, _ilc) \
    {   U32 temp; \
            temp = fetch_fw((_inst)+2); \
            (_effective_addr1) = (temp >> 16) & 0xfff; \
            (_b1) = (temp >> 28); \
            if((_b1)) { \
                (_effective_addr1) += (_regs)->GR((_b1)); \
                (_effective_addr1) &= ADDRESS_MAXWRAP((_regs)); \
            } \
            (_effective_addr2) = temp & 0xfff; \
            (_b2) = (temp >> 12) & 0xf; \
            if ((_b2)) { \
                (_effective_addr2) += (_regs)->GR((_b2)); \
                (_effective_addr2) &= ADDRESS_MAXWRAP((_regs)); \
            } \
            (_b1) = ((_inst)[1]     ) & 0xf;\
            (_r3) = ((_inst)[1] >> 4) & 0xf; \
            INST_UPDATE_PSW((_regs), (_len), (_ilc)); \
    }

#undef SIE_TRANSLATE_ADDR
#undef SIE_LOGICAL_TO_ABS
#undef SIE_INTERCEPT
#undef SIE_TRANSLATE


#if defined(_FEATURE_SIE)

#define SIE_SET_VI(_who, _when, _why, _regs) \
    { \
        (_regs)->siebk->vi_who = (_who); \
        (_regs)->siebk->vi_when = (_when); \
        STORE_HW((_regs)->siebk->vi_why, (_why)); \
        memset((_regs)->siebk->vi_zero, 0, 6); \
    }

#if __GEN_ARCH == 900 || (__GEN_ARCH == 390 && !defined(_FEATURE_ZSIE))

#define SIE_TRANSLATE_ADDR(_addr, _arn, _regs, _acctype) \
    ARCH_DEP(translate_addr)((_addr), (_arn), (_regs), (_acctype))

#define SIE_LOGICAL_TO_ABS(_addr, _arn, _regs, _acctype, _akey) \
  ( \
    ARCH_DEP(logical_to_main)((_addr), (_arn), (_regs), (_acctype), (_akey)), \
    (_regs)->dat.aaddr \
  )

#elif __GEN_ARCH == 370 && defined(_FEATURE_SIE)

#define SIE_TRANSLATE_ADDR(_addr, _arn, _regs, _acctype)   \
    s390_translate_addr((_addr), (_arn), (_regs), (_acctype))

#define SIE_LOGICAL_TO_ABS(_addr, _arn, _regs, _acctype, _akey) \
  ( \
    s390_logical_to_main((_addr), (_arn), (_regs), (_acctype), (_akey)), \
    (_regs)->dat.aaddr \
  )

#else /*__GEN_ARCH == 390 && defined(_FEATURE_ZSIE)*/

#define SIE_TRANSLATE_ADDR(_addr, _arn, _regs, _acctype)   \
    ( ((_regs)->arch_mode == ARCH_390) ?            \
    s390_translate_addr((_addr), (_arn), (_regs), (_acctype)) : \
    z900_translate_addr((_addr), (_arn), (_regs), (_acctype)) )

#define SIE_LOGICAL_TO_ABS(_addr, _arn, _regs, _acctype, _akey) \
  ( \
    (((_regs)->arch_mode == ARCH_390) \
    ? s390_logical_to_main((_addr), (_arn), (_regs), (_acctype), (_akey)) \
    : z900_logical_to_main((_addr), (_arn), (_regs), (_acctype), (_akey))), \
    (_regs)->dat.aaddr \
  )

#endif

#define SIE_INTERCEPT(_regs) \
do { \
    if(SIE_MODE((_regs))) \
    longjmp((_regs)->progjmp, SIE_INTERCEPT_INST); \
} while(0)

#define SIE_TRANSLATE(_addr, _acctype, _regs) \
do { \
    if(SIE_MODE((_regs)) && !(_regs)->sie_pref) \
    *(_addr) = SIE_LOGICAL_TO_ABS ((_regs)->sie_mso + *(_addr), \
      USE_PRIMARY_SPACE, (_regs)->hostregs, (_acctype), 0); \
} while(0)

#else /*!defined(_FEATURE_SIE)*/

#define SIE_TRANSLATE_ADDR(_addr, _arn, _regs, _acctype)
#define SIE_LOGICAL_TO_ABS(_addr, _arn, _regs, _acctype, _akey)
#define SIE_INTERCEPT(_regs)
#define SIE_TRANSLATE(_addr, _acctype, _regs)

#endif /*!defined(_FEATURE_SIE)*/


#undef SIE_XC_INTERCEPT

#if defined(FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE)

#define SIE_XC_INTERCEPT(_regs) \
    if(SIE_STATB((_regs), MX, XC)) \
        SIE_INTERCEPT((_regs))

#else /*!defined(FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE)*/

#define SIE_XC_INTERCEPT(_regs)

#endif /*!defined(FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE)*/


#if defined(FEATURE_VECTOR_FACILITY)

#if !defined(_VFDEFS)

#define _VFDEFS

#define VOP_CHECK(_regs) \
    if(!((_regs)->CR(0) & CR0_VOP) || !(_regs)->vf->online) \
        (_regs)->program_interrupt((_regs), PGM_VECTOR_OPERATION_EXCEPTION)

#define VR_INUSE(_vr, _regs) \
    ((_regs)->vf->vsr & (VSR_VIU0 >> ((_vr) >> 1)))

#define VR_CHANGED(_vr, _regs) \
    ((_regs)->vf->vsr & (VSR_VCH0 >> ((_vr) >> 1)))

#define SET_VR_INUSE(_vr, _regs) \
    (_regs)->vf->vsr |= (VSR_VIU0 >> ((_vr) >> 1))

#define SET_VR_CHANGED(_vr, _regs) \
    (_regs)->vf->vsr |= (VSR_VCH0 >> ((_vr) >> 1))

#define RESET_VR_INUSE(_vr, _regs) \
    (_regs)->vf->vsr &= ~(VSR_VIU0 >> ((_vr) >> 1))

#define RESET_VR_CHANGED(_vr, _regs) \
    (_regs)->vf->vsr &= ~(VSR_VCH0 >> ((_vr) >> 1))

#define VMR_SET(_section, _regs) \
    ((_regs)->vf->vmr[(_section) >> 3] & (0x80 >> ((_section) & 7)))

#define MASK_MODE(_regs) \
    ((_regs)->vf->vsr & VSR_M)

#define VECTOR_COUNT(_regs) \
        (((_regs)->vf->vsr & VSR_VCT) >> 32)

#define VECTOR_IX(_regs) \
        (((_regs)->vf->vsr & VSR_VIX) >> 16)

#endif /*!defined(_VFDEFS)*/

/* VST and QST formats are the same */
#undef VST
#define VST(_inst, _regs, _vr3, _rt2, _vr1, _rs2) \
    { \
        (_qr3) = (_inst)[2] >> 4; \
        (_rt2) = (_inst)[2] & 0x0F; \
        (_vr1) = (_inst)[3] >> 4; \
        (_rs2) = (_inst)[3] & 0x0F; \
        INST_UPDATE_PSW((_regs), 4, 4); \
    }

/* VR, VV and QV formats are the same */
#undef VR
#define VR(_inst, _regs, _qr3, _vr1, _vr2) \
    { \
        (_qr3) = (_inst)[2] >> 4; \
        (_vr1) = (_inst)[3] >> 4; \
        (_vr2) = (_inst)[3] & 0x0F; \
        INST_UPDATE_PSW((_regs), 4, 4); \
    }

#undef VS
#define VS(_inst, _regs, _rs2) \
    { \
        (_rs2) = (_inst)[3] & 0x0F; \
        INST_UPDATE_PSW((_regs), 4, 4); \
    }

/* The RSE vector instruction format of ESA/390 is referred to as
   VRSE to avoid conflict with the ESAME RSE instruction format */
#undef VRSE
#define VRSE(_inst, _regs, _r3, _vr1, \
                     _b2, _effective_addr2) \
    { \
        (_r3) = (_inst)[2] >> 4; \
        (_vr1) = (_inst)[3] >> 4; \
        (_b2) = (_inst)[4] >> 4; \
        (_effective_addr2) = (((_inst)[4] & 0x0F) << 8) | (_inst)[5]; \
        if((_b2) != 0) \
        { \
        (_effective_addr2) += (_regs)->GR((_b2)); \
        (_effective_addr2) &= ADDRESS_MAXWRAP((_regs)); \
        } \
        INST_UPDATE_PSW((_regs), 6, 6); \
    }

/* S format instructions where the effective address does not wrap */
#undef S_NW
#define S_NW(_inst, _regs, _b2, _effective_addr2) \
    { \
        (_b2) = (_inst)[2] >> 4; \
        (_effective_addr2) = (((_inst)[2] & 0x0F) << 8) | (_inst)[3]; \
        if((_b2) != 0) \
        { \
        (_effective_addr2) += (_regs)->GR((_b2)); \
        } \
        INST_UPDATE_PSW((_regs), 4, 4); \
    }

#endif /*defined(FEATURE_VECTOR_FACILITY)*/

#define PERFORM_SERIALIZATION(_regs) do { } while (0)
#define PERFORM_CHKPT_SYNC(_regs) do { } while (0)

/* Functions in module channel.c */
int  ARCH_DEP(startio) (REGS *regs, DEVBLK *dev, ORB *orb);
void *s370_execute_ccw_chain (void *dev);
void *s390_execute_ccw_chain (void *dev);
void *z900_execute_ccw_chain (void *dev);
int  stchan_id (REGS *regs, U16 chan);
int  testch (REGS *regs, U16 chan);
int  testio (REGS *regs, DEVBLK *dev, BYTE ibyte);
int  test_subchan (REGS *regs, DEVBLK *dev, IRB *irb);
int  cancel_subchan (REGS *regs, DEVBLK *dev);
void clear_subchan (REGS *regs, DEVBLK *dev);
int  halt_subchan (REGS *regs, DEVBLK *dev);
int  haltio (REGS *regs, DEVBLK *dev, BYTE ibyte);
int  resume_subchan (REGS *regs, DEVBLK *dev);
int  ARCH_DEP(present_io_interrupt) (REGS *regs, U32 *ioid,
        U32 *ioparm, U32 *iointid, BYTE *csw);
int ARCH_DEP(present_zone_io_interrupt) (U32 *ioid, U32 *ioparm,
                                              U32 *iointid, BYTE zone);
void io_reset (void);
int  chp_reset(BYTE chpid, int solicited);
void channelset_reset(REGS *regs);


/* Functions in module cpu.c */
/* define all arch_load|store_psw */
/* regardless of current architecture (if any) */
#if defined(_370)
void s370_store_psw (REGS *regs, BYTE *addr);
int  s370_load_psw (REGS *regs, BYTE *addr);
void s370_process_trace (REGS *regs);
#endif
#if defined(_390)
int  s390_load_psw (REGS *regs, BYTE *addr);
void s390_store_psw (REGS *regs, BYTE *addr);
void s390_process_trace (REGS *regs);
#endif /*defined(_FEATURE_ZSIE)*/
#if defined(_900)
int  z900_load_psw (REGS *regs, BYTE *addr);
void z900_store_psw (REGS *regs, BYTE *addr);
void z900_process_trace (REGS *regs);
#endif

int cpu_init (int cpu, REGS *regs, REGS *hostregs);
void ARCH_DEP(perform_io_interrupt) (REGS *regs);
void ARCH_DEP(checkstop_config)(void);
#if defined(_FEATURE_SIE)
CPU_DLL_IMPORT void (ATTR_REGPARM(2) s370_program_interrupt) (REGS *regs, int code);
#endif /*!defined(_FEATURE_SIE)*/
#if defined(_FEATURE_ZSIE)
CPU_DLL_IMPORT void (ATTR_REGPARM(2) s390_program_interrupt) (REGS *regs, int code);
#endif /*!defined(_FEATURE_ZSIE)*/
CPU_DLL_IMPORT void (ATTR_REGPARM(2) ARCH_DEP(program_interrupt)) (REGS *regs, int code);
void *cpu_thread (void *cpu);
DLL_EXPORT void copy_psw (REGS *regs, BYTE *addr);
int display_psw (REGS *regs, char *buf, int buflen);
char *str_psw (REGS *regs, char *buf);


/* Functions in module vm.c */
int  ARCH_DEP(diag_devtype) (int r1, int r2, REGS *regs);
int  ARCH_DEP(syncblk_io) (int r1, int r2, REGS *regs);
int  ARCH_DEP(syncgen_io) (int r1, int r2, REGS *regs);
void ARCH_DEP(extid_call) (int r1, int r2, REGS *regs);
int  ARCH_DEP(cpcmd_call) (int r1, int r2, REGS *regs);
void ARCH_DEP(pseudo_timer) (U32 code, int r1, int r2, REGS *regs);
void ARCH_DEP(access_reipl_data) (int r1, int r2, REGS *regs);
int  ARCH_DEP(diag_ppagerel) (int r1, int r2, REGS *regs);
void ARCH_DEP(vm_info) (int r1, int r2, REGS *regs);
int  ARCH_DEP(device_info) (int r1, int r2, REGS *regs);


/* Functions in module vmd250.c */
int  ARCH_DEP(vm_blockio) (int r1, int r2, REGS *regs);


/* Functions in module control.c */
void ARCH_DEP(load_real_address_proc) (REGS *regs,
    int r1, int b2, VADR effective_addr2);


/* Functions in module decimal.c */
void packed_to_binary (BYTE *dec, int len, U64 *result,
    int *ovf, int *dxf);
void binary_to_packed (S64 bin, BYTE *result);


/* Functions in module diagnose.c */
void ARCH_DEP(diagnose_call) (VADR effective_addr2, int b2, int r1, int r3,
    REGS *regs);


/* Functions in module diagmssf.c */
void ARCH_DEP(scpend_call) (void);
int  ARCH_DEP(mssf_call) (int r1, int r2, REGS *regs);
void ARCH_DEP(diag204_call) (int r1, int r2, REGS *regs);
void ARCH_DEP(diag224_call) (int r1, int r2, REGS *regs);


/* Functions in module external.c */
void ARCH_DEP(perform_external_interrupt) (REGS *regs);
void ARCH_DEP(store_status) (REGS *ssreg, RADR aaddr);
void store_status (REGS *ssreg, U64 aaddr);

/* Function in module hdiagf18.c */
void ARCH_DEP(diagf18_call) (int r1, int r2, REGS *regs);

/* Functions in module ipl.c */
int          load_ipl           (U16 lcss, U16  devnum, int cpu, int clear);
int ARCH_DEP(load_ipl)          (U16 lcss, U16  devnum, int cpu, int clear);
int          system_reset       (const int cpu, const int flags, const int target_mode);
int ARCH_DEP(system_reset)      (const int cpu, const int flags, const int target_mode);
int          cpu_reset          (REGS *regs);
int ARCH_DEP(cpu_reset)         (REGS *regs);
int          initial_cpu_reset  (REGS *regs);
int ARCH_DEP(initial_cpu_reset) (REGS *regs);
int ARCH_DEP(common_load_begin)  (int cpu, int clear);
int ARCH_DEP(common_load_finish) (REGS *regs);
void storage_clear(void);
void xstorage_clear(void);


/* Functions in module scedasd.c */
void         set_sce_dir        (char *path);
char        *get_sce_dir        ();
int          load_main          (char *fname, RADR startloc, int noisy );
int ARCH_DEP(load_main)         (char *fname, RADR startloc, int noisy );
int          load_hmc           (char *fname, int cpu, int clear);
int ARCH_DEP(load_hmc)          (char *fname, int cpu, int clear);
void ARCH_DEP(sclp_scedio_request) (SCCB_HEADER *);
void ARCH_DEP(sclp_scedio_event) (SCCB_HEADER *);


/* Functions in module scescsi.c */
void ARCH_DEP(sclp_hwl_request) (SCCB_HEADER *);
void ARCH_DEP(sclp_hwl_event) (SCCB_HEADER *);
void ARCH_DEP(sclp_sdias_request) (SCCB_HEADER *);
void ARCH_DEP(sclp_sdias_event) (SCCB_HEADER *);
void ARCH_DEP(sdias_store_status_clear) (REGS *);
void ARCH_DEP(sdias_store_status) (REGS *);
int support_boot (DEVBLK *);
int load_boot (DEVBLK *, int, int, int);
int hwldr_cmd (int, char **, char *);
int lddev_cmd (int, char **, char *);


/* Functions in module machchk.c */
int  ARCH_DEP(present_mck_interrupt) (REGS *regs, U64 *mcic, U32 *xdmg,
    RADR *fsta);
U32  get_next_channel_report_word( REGS * );
void machine_check_crwpend (void);
void ARCH_DEP(sync_mck_interrupt) (REGS *regs);
void sigabend_handler (int signo);
void build_attach_chrpt( DEVBLK *dev );
void build_detach_chrpt( DEVBLK *dev );
void build_chp_reset_chrpt( BYTE chpid, int solicited, int found );
int  queue_channel_report( U32* crwarray, U32 crwcount );


/* Functions in module opcode.c */
void init_opcode_tables(void);
void init_opcode_pointers(REGS *regs);


/* Functions in module panel.c */
void ARCH_DEP(display_inst) (REGS *regs, BYTE *inst);
void display_inst (REGS *regs, BYTE *inst);


/* Functions in module sie.c */
void ARCH_DEP(sie_exit) (REGS *regs, int code);
void ARCH_DEP(diagnose_002) (REGS *regs, int r1, int r3);


/* Functions in module stack.c */
void ARCH_DEP(trap_x) (int trap_is_trap4, REGS *regs, U32 trap_operand);
void ARCH_DEP(form_stack_entry) (BYTE etype, VADR retna, VADR calla,
    U32 csi, U32 pcnum, REGS *regs);
VADR ARCH_DEP(locate_stack_entry) (int prinst, LSED *lsedptr,
    REGS *regs);
void ARCH_DEP(stack_modify) (VADR lsea, U32 m1, U32 m2, REGS *regs);
void ARCH_DEP(stack_extract) (VADR lsea, int r1, int code, REGS *regs);
void ARCH_DEP(unstack_registers) (int gtype, VADR lsea, int r1,
    int r2, REGS *regs);
int  ARCH_DEP(program_return_unstack) (REGS *regs, RADR *lsedap, int *rc);


/* Functions in module trace.c */
CREG  ARCH_DEP(trace_br) (int amode, VADR ia, REGS *regs);
#if defined(_FEATURE_ZSIE)
U32  s390_trace_br (int amode, U32 ia, REGS *regs);
#endif /*!defined(_FEATURE_ZSIE)*/
CREG  ARCH_DEP(trace_bsg) (U32 alet, VADR ia, REGS *regs);
CREG  ARCH_DEP(trace_ssar) (int ssair, U16 sasn, REGS *regs);
CREG  ARCH_DEP(trace_pc) (U32 pcea, REGS *regs);
CREG  ARCH_DEP(trace_pr) (REGS *newregs, REGS *regs);
CREG  ARCH_DEP(trace_pt) (int pti, U16 pasn, GREG gpr2, REGS *regs);
CREG  ARCH_DEP(trace_tr) (int r1, int r3, U32 op, REGS *regs);
CREG  ARCH_DEP(trace_tg) (int r1, int r3, U32 op, REGS *regs);
CREG  ARCH_DEP(trace_ms) (int br_ind, VADR ia, REGS *regs);


/* Functions in module plo.c */
int ARCH_DEP(plo_cl) (int r1, int r3, VADR effective_addr2, int b2,
                            VADR effective_addr4, int b4,  REGS *regs);
int ARCH_DEP(plo_clg) (int r1, int r3, VADR effective_addr2, int b2,
                            VADR effective_addr4, int b4,  REGS *regs);
int ARCH_DEP(plo_clgr) (int r1, int r3, VADR effective_addr2, int b2,
                            VADR effective_addr4, int b4,  REGS *regs);
int ARCH_DEP(plo_clx) (int r1, int r3, VADR effective_addr2, int b2,
                            VADR effective_addr4, int b4,  REGS *regs);
int ARCH_DEP(plo_cs) (int r1, int r3, VADR effective_addr2, int b2,
                            VADR effective_addr4, int b4,  REGS *regs);
int ARCH_DEP(plo_csg) (int r1, int r3, VADR effective_addr2, int b2,
                            VADR effective_addr4, int b4,  REGS *regs);
int ARCH_DEP(plo_csgr) (int r1, int r3, VADR effective_addr2, int b2,
                            VADR effective_addr4, int b4,  REGS *regs);
int ARCH_DEP(plo_csx) (int r1, int r3, VADR effective_addr2, int b2,
                            VADR effective_addr4, int b4,  REGS *regs);
int ARCH_DEP(plo_dcs) (int r1, int r3, VADR effective_addr2, int b2,
                            VADR effective_addr4, int b4,  REGS *regs);
int ARCH_DEP(plo_dcsg) (int r1, int r3, VADR effective_addr2, int b2,
                            VADR effective_addr4, int b4,  REGS *regs);
int ARCH_DEP(plo_dcsgr) (int r1, int r3, VADR effective_addr2, int b2,
                            VADR effective_addr4, int b4,  REGS *regs);
int ARCH_DEP(plo_dcsx) (int r1, int r3, VADR effective_addr2, int b2,
                            VADR effective_addr4, int b4,  REGS *regs);
int ARCH_DEP(plo_csst) (int r1, int r3, VADR effective_addr2, int b2,
                            VADR effective_addr4, int b4,  REGS *regs);
int ARCH_DEP(plo_csstg) (int r1, int r3, VADR effective_addr2, int b2,
                            VADR effective_addr4, int b4,  REGS *regs);
int ARCH_DEP(plo_csstgr) (int r1, int r3, VADR effective_addr2, int b2,
                            VADR effective_addr4, int b4,  REGS *regs);
int ARCH_DEP(plo_csstx) (int r1, int r3, VADR effective_addr2, int b2,
                            VADR effective_addr4, int b4,  REGS *regs);
int ARCH_DEP(plo_csdst) (int r1, int r3, VADR effective_addr2, int b2,
                            VADR effective_addr4, int b4,  REGS *regs);
int ARCH_DEP(plo_csdstg) (int r1, int r3, VADR effective_addr2, int b2,
                            VADR effective_addr4, int b4,  REGS *regs);
int ARCH_DEP(plo_csdstgr) (int r1, int r3, VADR effective_addr2, int b2,
                            VADR effective_addr4, int b4,  REGS *regs);
int ARCH_DEP(plo_csdstx) (int r1, int r3, VADR effective_addr2, int b2,
                            VADR effective_addr4, int b4,  REGS *regs);
int ARCH_DEP(plo_cstst) (int r1, int r3, VADR effective_addr2, int b2,
                            VADR effective_addr4, int b4,  REGS *regs);
int ARCH_DEP(plo_cststg) (int r1, int r3, VADR effective_addr2, int b2,
                            VADR effective_addr4, int b4,  REGS *regs);
int ARCH_DEP(plo_cststgr) (int r1, int r3, VADR effective_addr2, int b2,
                            VADR effective_addr4, int b4,  REGS *regs);
int ARCH_DEP(plo_cststx) (int r1, int r3, VADR effective_addr2, int b2,
                            VADR effective_addr4, int b4,  REGS *regs);


/* Instruction functions in opcode.c */
DEF_INST(execute_e3________xx);
DEF_INST(execute_eb________xx);
DEF_INST(execute_ec________xx);
DEF_INST(execute_ed________xx);
DEF_INST(operation_exception);
DEF_INST(dummy_instruction);
DEF_INST(E3_0);


/* Instructions in assist.c */
DEF_INST(fix_page);
DEF_INST(svc_assist);
DEF_INST(obtain_local_lock);
DEF_INST(release_local_lock);
DEF_INST(obtain_cms_lock);
DEF_INST(release_cms_lock);
DEF_INST(trace_svc_interruption);
DEF_INST(trace_program_interruption);
DEF_INST(trace_initial_srb_dispatch);
DEF_INST(trace_io_interruption);
DEF_INST(trace_task_dispatch);
DEF_INST(trace_svc_return);


/* Instructions in cmpsc.c */
#if defined(FEATURE_COMPRESSION)
DEF_INST(cmpsc_2012);
#endif /*defined(FEATURE_COMPRESSION)*/


/* Instructions in control.c */
#if defined(FEATURE_BRANCH_AND_SET_AUTHORITY)
DEF_INST(branch_and_set_authority);
#endif /*defined(FEATURE_BRANCH_AND_SET_AUTHORITY)*/
#if defined(FEATURE_SUBSPACE_GROUP)
DEF_INST(branch_in_subspace_group);
#endif /*defined(FEATURE_SUBSPACE_GROUP)*/
#if defined(FEATURE_LINKAGE_STACK)
DEF_INST(branch_and_stack);
#endif /*defined(FEATURE_LINKAGE_STACK)*/
#if defined(FEATURE_BROADCASTED_PURGING)
DEF_INST(compare_and_swap_and_purge);
#endif /*defined(FEATURE_BROADCASTED_PURGING)*/
DEF_INST(diagnose);
#if defined(FEATURE_DUAL_ADDRESS_SPACE)
DEF_INST(extract_primary_asn);
#endif /*defined(FEATURE_DUAL_ADDRESS_SPACE)*/
#if defined(FEATURE_ASN_AND_LX_REUSE)
DEF_INST(extract_primary_asn_and_instance);
#endif /*defined(FEATURE_ASN_AND_LX_REUSE)*/
#if defined(FEATURE_DUAL_ADDRESS_SPACE)
DEF_INST(extract_secondary_asn);
#endif /*defined(FEATURE_DUAL_ADDRESS_SPACE)*/
#if defined(FEATURE_ASN_AND_LX_REUSE)
DEF_INST(extract_secondary_asn_and_instance);
#endif /*defined(FEATURE_ASN_AND_LX_REUSE)*/
#if defined(FEATURE_LINKAGE_STACK)
DEF_INST(extract_stacked_registers);
DEF_INST(extract_stacked_state);
#endif /*defined(FEATURE_LINKAGE_STACK)*/
#if defined(FEATURE_DUAL_ADDRESS_SPACE)
DEF_INST(insert_address_space_control);
#endif /*defined(FEATURE_DUAL_ADDRESS_SPACE)*/
DEF_INST(insert_psw_key);
#if defined(FEATURE_BASIC_STORAGE_KEYS)
DEF_INST(insert_storage_key);
#endif /*defined(FEATURE_BASIC_STORAGE_KEYS)*/
#if defined(FEATURE_EXTENDED_STORAGE_KEYS)
DEF_INST(insert_storage_key_extended);
#endif /*defined(FEATURE_EXTENDED_STORAGE_KEYS)*/
#if defined(FEATURE_DUAL_ADDRESS_SPACE)
DEF_INST(insert_virtual_storage_key);
#endif /*defined(FEATURE_DUAL_ADDRESS_SPACE)*/
DEF_INST(invalidate_page_table_entry);
#if defined(FEATURE_DUAL_ADDRESS_SPACE)
DEF_INST(load_address_space_parameters);
#endif /*defined(FEATURE_DUAL_ADDRESS_SPACE)*/
DEF_INST(load_control);
DEF_INST(load_program_status_word);
DEF_INST(load_real_address);
DEF_INST(load_using_real_address);
#if defined(FEATURE_LOCK_PAGE)
DEF_INST(lock_page);
#endif /*defined(FEATURE_LOCK_PAGE)*/
#if defined(FEATURE_LINKAGE_STACK)
DEF_INST(modify_stacked_state);
#endif /*defined(FEATURE_LINKAGE_STACK)*/
#if defined(FEATURE_DUAL_ADDRESS_SPACE)
DEF_INST(move_to_primary);
DEF_INST(move_to_secondary);
#endif /*defined(FEATURE_DUAL_ADDRESS_SPACE)*/
DEF_INST(move_with_destination_key);
#if defined(FEATURE_DUAL_ADDRESS_SPACE)
DEF_INST(move_with_key);
#endif /*defined(FEATURE_DUAL_ADDRESS_SPACE)*/
#if defined(FEATURE_MOVE_WITH_OPTIONAL_SPECIFICATIONS)
DEF_INST(move_with_optional_specifications);                    /*208*/
#endif /*defined(FEATURE_MOVE_WITH_OPTIONAL_SPECIFICATIONS)*/
DEF_INST(move_with_source_key);
#if defined(FEATURE_DUAL_ADDRESS_SPACE)
DEF_INST(program_call);
#endif /*defined(FEATURE_DUAL_ADDRESS_SPACE)*/
#if defined(FEATURE_LINKAGE_STACK)
DEF_INST(program_return);
#endif /*defined(FEATURE_LINKAGE_STACK)*/
#if defined(FEATURE_DUAL_ADDRESS_SPACE)
DEF_INST(program_transfer);
#endif /*defined(FEATURE_DUAL_ADDRESS_SPACE)*/
#if defined(FEATURE_ASN_AND_LX_REUSE)
DEF_INST(program_transfer_with_instance);
#endif /*defined(FEATURE_ASN_AND_LX_REUSE)*/
#if defined(FEATURE_ACCESS_REGISTERS)
DEF_INST(purge_accesslist_lookaside_buffer);
#endif /*defined(FEATURE_ACCESS_REGISTERS)*/
DEF_INST(purge_translation_lookaside_buffer);
#if defined(FEATURE_BASIC_STORAGE_KEYS)
DEF_INST(reset_reference_bit);
#endif /*defined(FEATURE_BASIC_STORAGE_KEYS)*/
#if defined(FEATURE_EXTENDED_STORAGE_KEYS)
DEF_INST(reset_reference_bit_extended);
#endif /*defined(FEATURE_EXTENDED_STORAGE_KEYS)*/
#if defined(FEATURE_DUAL_ADDRESS_SPACE)
DEF_INST(set_address_space_control);
#endif /*defined(FEATURE_DUAL_ADDRESS_SPACE)*/
#if defined(FEATURE_SET_ADDRESS_SPACE_CONTROL_FAST)
DEF_INST(set_address_space_control_fast);
#endif /*defined(FEATURE_SET_ADDRESS_SPACE_CONTROL_FAST)*/
DEF_INST(set_clock);
DEF_INST(set_clock_comparator);
#if defined(FEATURE_EXTENDED_TOD_CLOCK)
DEF_INST(set_clock_programmable_field);
#endif /*defined(FEATURE_EXTENDED_TOD_CLOCK)*/
DEF_INST(set_cpu_timer);
DEF_INST(set_prefix);
DEF_INST(set_psw_key_from_address);
#if defined(FEATURE_DUAL_ADDRESS_SPACE)
DEF_INST(set_secondary_asn);
#endif /*defined(FEATURE_DUAL_ADDRESS_SPACE)*/
#if defined(FEATURE_ASN_AND_LX_REUSE)
DEF_INST(set_secondary_asn_with_instance);
#endif /*defined(FEATURE_ASN_AND_LX_REUSE)*/
#if defined(FEATURE_BASIC_STORAGE_KEYS)
DEF_INST(set_storage_key);
#endif /*defined(FEATURE_BASIC_STORAGE_KEYS)*/
#if defined(FEATURE_EXTENDED_STORAGE_KEYS)
DEF_INST(set_storage_key_extended);
#endif /*defined(FEATURE_EXTENDED_STORAGE_KEYS)*/
DEF_INST(set_system_mask);
DEF_INST(signal_processor);
DEF_INST(store_clock_comparator);
DEF_INST(store_control);
DEF_INST(store_cpu_address);
DEF_INST(store_cpu_id);
DEF_INST(store_cpu_timer);
DEF_INST(store_prefix);
#if defined(FEATURE_STORE_SYSTEM_INFORMATION)
DEF_INST(store_system_information);
#endif /*defined(FEATURE_STORE_SYSTEM_INFORMATION)*/
DEF_INST(store_then_and_system_mask);
DEF_INST(store_then_or_system_mask);
DEF_INST(store_using_real_address);
#if defined(FEATURE_ACCESS_REGISTERS)
DEF_INST(test_access);
#endif /*defined(FEATURE_ACCESS_REGISTERS)*/
DEF_INST(test_block);
DEF_INST(test_protection);
DEF_INST(trace);


/* Instructions in decimal.c */
DEF_INST(add_decimal);
DEF_INST(compare_decimal);
DEF_INST(divide_decimal);
DEF_INST(edit_x_edit_and_mark);
DEF_INST(multiply_decimal);
DEF_INST(shift_and_round_decimal);
DEF_INST(subtract_decimal);
DEF_INST(zero_and_add);
DEF_INST(test_decimal);


/* Instructions in vm.c */
#if defined(FEATURE_EMULATE_VM)
DEF_INST(inter_user_communication_vehicle);
#endif /*defined(FEATURE_EMULATE_VM)*/


/* Instructions in sie.c */
#if defined(FEATURE_INTERPRETIVE_EXECUTION)
DEF_INST(start_interpretive_execution);
#endif /*defined(FEATURE_INTERPRETIVE_EXECUTION)*/
#if defined(FEATURE_REGION_RELOCATE)
DEF_INST(store_zone_parameter);
DEF_INST(set_zone_parameter);
#endif /*defined(FEATURE_REGION_RELOCATE)*/
#if defined(FEATURE_IO_ASSIST)
DEF_INST(test_pending_zone_interrupt);
#endif /*defined(FEATURE_IO_ASSIST)*/


/* Instructions in qdio.c */
#if defined(FEATURE_QUEUED_DIRECT_IO)

DEF_INST(signal_adapter);

#if defined(FEATURE_QEBSM)
DEF_INST(set_queue_buffer_state);
DEF_INST(extract_queue_buffer_state);
#endif /*defined(FEATURE_QEBSM)*/

#if defined(FEATURE_SVS)
DEF_INST(set_vector_summary);
#endif /*defined(FEATURE_SVS)*/

#endif /*defined(FEATURE_QUEUED_DIRECT_IO)*/


/* Instructions in float.c */
#if defined(FEATURE_HEXADECIMAL_FLOATING_POINT)
DEF_INST(load_positive_float_long_reg);
DEF_INST(load_negative_float_long_reg);
DEF_INST(load_and_test_float_long_reg);
DEF_INST(load_complement_float_long_reg);
DEF_INST(halve_float_long_reg);
DEF_INST(load_rounded_float_long_reg);
DEF_INST(multiply_float_ext_reg);
DEF_INST(multiply_float_long_to_ext_reg);
DEF_INST(load_float_long_reg);
DEF_INST(compare_float_long_reg);
DEF_INST(add_float_long_reg);
DEF_INST(subtract_float_long_reg);
DEF_INST(multiply_float_long_reg);
DEF_INST(divide_float_long_reg);
DEF_INST(add_unnormal_float_long_reg);
DEF_INST(subtract_unnormal_float_long_reg);
DEF_INST(load_positive_float_short_reg);
DEF_INST(load_negative_float_short_reg);
DEF_INST(load_and_test_float_short_reg);
DEF_INST(load_complement_float_short_reg);
DEF_INST(halve_float_short_reg);
DEF_INST(load_rounded_float_short_reg);
DEF_INST(add_float_ext_reg);
DEF_INST(subtract_float_ext_reg);
DEF_INST(load_float_short_reg);
DEF_INST(compare_float_short_reg);
DEF_INST(add_float_short_reg);
DEF_INST(subtract_float_short_reg);
DEF_INST(multiply_float_short_to_long_reg);
DEF_INST(divide_float_short_reg);
DEF_INST(add_unnormal_float_short_reg);
DEF_INST(subtract_unnormal_float_short_reg);
DEF_INST(store_float_long);
DEF_INST(multiply_float_long_to_ext);
DEF_INST(load_float_long);
DEF_INST(compare_float_long);
DEF_INST(add_float_long);
DEF_INST(subtract_float_long);
DEF_INST(multiply_float_long);
DEF_INST(divide_float_long);
DEF_INST(add_unnormal_float_long);
DEF_INST(subtract_unnormal_float_long);
DEF_INST(store_float_short);
DEF_INST(load_float_short);
DEF_INST(compare_float_short);
DEF_INST(add_float_short);
DEF_INST(subtract_float_short);
DEF_INST(multiply_float_short_to_long);
DEF_INST(divide_float_short);
DEF_INST(add_unnormal_float_short);
DEF_INST(subtract_unnormal_float_short);
DEF_INST(divide_float_ext_reg);
#endif /*defined(FEATURE_HEXADECIMAL_FLOATING_POINT)*/

#if defined(FEATURE_SQUARE_ROOT)
DEF_INST(squareroot_float_long_reg);
DEF_INST(squareroot_float_short_reg);
#endif /*defined(FEATURE_SQUARE_ROOT)*/

#if defined(FEATURE_HFP_EXTENSIONS)
DEF_INST(load_lengthened_float_short_to_long_reg);
DEF_INST(load_lengthened_float_long_to_ext_reg);
DEF_INST(load_lengthened_float_short_to_ext_reg);
DEF_INST(squareroot_float_ext_reg);
DEF_INST(multiply_float_short_reg);
DEF_INST(load_positive_float_ext_reg);
DEF_INST(load_negative_float_ext_reg);
DEF_INST(load_and_test_float_ext_reg);
DEF_INST(load_complement_float_ext_reg);
DEF_INST(load_rounded_float_ext_to_short_reg);
DEF_INST(load_fp_int_float_ext_reg);
DEF_INST(compare_float_ext_reg);
DEF_INST(load_fp_int_float_short_reg);
DEF_INST(load_fp_int_float_long_reg);
DEF_INST(convert_fixed_to_float_short_reg);
DEF_INST(convert_fixed_to_float_long_reg);
DEF_INST(convert_fixed_to_float_ext_reg);
DEF_INST(convert_float_short_to_fixed_reg);
DEF_INST(convert_float_long_to_fixed_reg);
DEF_INST(convert_float_ext_to_fixed_reg);
DEF_INST(load_lengthened_float_short_to_long);
DEF_INST(load_lengthened_float_long_to_ext);
DEF_INST(load_lengthened_float_short_to_ext);
DEF_INST(squareroot_float_short);
DEF_INST(squareroot_float_long);
DEF_INST(multiply_float_short);
#endif /*defined(FEATURE_HEXADECIMAL_FLOATING_POINT)*/

DEF_INST(convert_fix64_to_float_short_reg);
DEF_INST(convert_fix64_to_float_long_reg);
DEF_INST(convert_fix64_to_float_ext_reg);
DEF_INST(convert_float_short_to_fix64_reg); /* under construction! Bernard van der Helm */
DEF_INST(convert_float_long_to_fix64_reg); /* under construction! Bernard van der Helm */
DEF_INST(convert_float_ext_to_fix64_reg); /* under construction! Bernard van der Helm */

#if defined(FEATURE_FPS_EXTENSIONS)
DEF_INST(load_float_ext_reg);
DEF_INST(load_zero_float_short_reg);
DEF_INST(load_zero_float_long_reg);
DEF_INST(load_zero_float_ext_reg);
#endif /*defined(FEATURE_FPS_EXTENSIONS)*/
#if defined(FEATURE_HFP_MULTIPLY_ADD_SUBTRACT)
DEF_INST(multiply_add_float_short_reg);
DEF_INST(multiply_add_float_long_reg);
DEF_INST(multiply_add_float_short);
DEF_INST(multiply_add_float_long);
DEF_INST(multiply_subtract_float_short_reg);
DEF_INST(multiply_subtract_float_long_reg);
DEF_INST(multiply_subtract_float_short);
DEF_INST(multiply_subtract_float_long);
#endif /*defined(FEATURE_HFP_MULTIPLY_ADD_SUBTRACT)*/

#if defined(FEATURE_HFP_UNNORMALIZED_EXTENSION)
DEF_INST(multiply_unnormal_float_long_to_ext_reg);              /*@Z9*/
DEF_INST(multiply_unnormal_float_long_to_ext_low_reg);          /*@Z9*/
DEF_INST(multiply_unnormal_float_long_to_ext_high_reg);         /*@Z9*/
DEF_INST(multiply_add_unnormal_float_long_to_ext_reg);          /*@Z9*/
DEF_INST(multiply_add_unnormal_float_long_to_ext_low_reg);      /*@Z9*/
DEF_INST(multiply_add_unnormal_float_long_to_ext_high_reg);     /*@Z9*/
DEF_INST(multiply_unnormal_float_long_to_ext);                  /*@Z9*/
DEF_INST(multiply_unnormal_float_long_to_ext_low);              /*@Z9*/
DEF_INST(multiply_unnormal_float_long_to_ext_high);             /*@Z9*/
DEF_INST(multiply_add_unnormal_float_long_to_ext);              /*@Z9*/
DEF_INST(multiply_add_unnormal_float_long_to_ext_low);          /*@Z9*/
DEF_INST(multiply_add_unnormal_float_long_to_ext_high);         /*@Z9*/
#endif /*defined(FEATURE_HFP_UNNORMALIZED_EXTENSION)*/

#if defined(FEATURE_LONG_DISPLACEMENT) && defined(FEATURE_HEXADECIMAL_FLOATING_POINT)
DEF_INST(load_float_long_y);
DEF_INST(load_float_short_y);
DEF_INST(store_float_long_y);
DEF_INST(store_float_short_y);
#endif /*defined(FEATURE_LONG_DISPLACEMENT) && defined(FEATURE_HEXADECIMAL_FLOATING_POINT)*/


/* Instructions in general1.c */
DEF_INST(add_register);
DEF_INST(add);
DEF_INST(add_halfword);
DEF_INST(add_logical_register);
DEF_INST(add_logical);
DEF_INST(and_register);
DEF_INST(and);
DEF_INST(and_immediate);
DEF_INST(and_character);
DEF_INST(branch_and_link_register);
DEF_INST(branch_and_link);
DEF_INST(branch_and_save_register);
DEF_INST(branch_and_save);
#if defined(FEATURE_BIMODAL_ADDRESSING)
DEF_INST(branch_and_save_and_set_mode);
DEF_INST(branch_and_set_mode);
#endif /*defined(FEATURE_BIMODAL_ADDRESSING)*/
DEF_INST(branch_on_condition_register);
DEF_INST(branch_on_condition);
DEF_INST(branch_on_count_register);
DEF_INST(branch_on_count);
DEF_INST(branch_on_index_high);
DEF_INST(branch_on_index_low_or_equal);
#if defined(FEATURE_IMMEDIATE_AND_RELATIVE)
DEF_INST(branch_relative_on_condition);
DEF_INST(branch_relative_and_save);
DEF_INST(branch_relative_on_count);
DEF_INST(branch_relative_on_index_high);
DEF_INST(branch_relative_on_index_low_or_equal);
#endif /*defined(FEATURE_IMMEDIATE_AND_RELATIVE)*/
#if defined(FEATURE_CHECKSUM_INSTRUCTION)
DEF_INST(checksum);
#endif /*defined(FEATURE_CHECKSUM_INSTRUCTION)*/
DEF_INST(compare_register);
DEF_INST(compare);
DEF_INST(compare_and_form_codeword);
DEF_INST(compare_and_swap);
DEF_INST(compare_double_and_swap);
#if defined(FEATURE_COMPARE_AND_SWAP_AND_STORE)
DEF_INST(compare_and_swap_and_store);
#endif /*defined(FEATURE_COMPARE_AND_SWAP_AND_STORE)*/
DEF_INST(compare_halfword);
DEF_INST(compare_logical_register);
DEF_INST(compare_logical);
DEF_INST(compare_logical_immediate);
DEF_INST(compare_logical_character);
DEF_INST(compare_logical_characters_under_mask);
DEF_INST(compare_logical_character_long);
#if defined(FEATURE_COMPARE_AND_MOVE_EXTENDED)
DEF_INST(compare_logical_long_extended);
#endif /*defined(FEATURE_COMPARE_AND_MOVE_EXTENDED)*/
#if defined(FEATURE_STRING_INSTRUCTION)
DEF_INST(compare_logical_string);
DEF_INST(compare_until_substring_equal);
#endif /*defined(FEATURE_STRING_INSTRUCTION)*/
#if defined(FEATURE_EXTENDED_TRANSLATION)
DEF_INST(convert_utf16_to_utf8);
DEF_INST(convert_utf8_to_utf16);
#endif /*defined(FEATURE_EXTENDED_TRANSLATION)*/
#if defined(FEATURE_EXTENDED_TRANSLATION_FACILITY_3)
DEF_INST(convert_utf16_to_utf32);
DEF_INST(convert_utf32_to_utf16);
DEF_INST(convert_utf32_to_utf8);
DEF_INST(convert_utf8_to_utf32);
DEF_INST(convert_to_binary);
DEF_INST(convert_to_decimal);
#endif /*defined(FEATURE_EXTENDED_TRANSLATION_FACILITY_3)*/
#if defined(FEATURE_ACCESS_REGISTERS)
DEF_INST(copy_access);
#endif /*defined(FEATURE_ACCESS_REGISTERS)*/
DEF_INST(divide_register);
DEF_INST(divide);
DEF_INST(exclusive_or_register);
DEF_INST(exclusive_or);
DEF_INST(exclusive_or_immediate);
DEF_INST(exclusive_or_character);
DEF_INST(execute);
#if defined(FEATURE_EXECUTE_EXTENSIONS_FACILITY)
DEF_INST(execute_relative_long);                                /*208*/
#endif /*defined(FEATURE_EXECUTE_EXTENSIONS_FACILITY)*/
#if defined(FEATURE_ACCESS_REGISTERS)
DEF_INST(extract_access_register);
#endif /*defined(FEATURE_ACCESS_REGISTERS)*/
DEF_INST(insert_character);
DEF_INST(insert_characters_under_mask);
DEF_INST(insert_program_mask);
DEF_INST(load);
DEF_INST(load_register);
#if defined(FEATURE_ACCESS_REGISTERS)
DEF_INST(load_access_multiple);
#endif /*defined(FEATURE_ACCESS_REGISTERS)*/
DEF_INST(load_address);
DEF_INST(load_address_extended);
DEF_INST(load_and_test_register);
DEF_INST(load_complement_register);
DEF_INST(load_halfword);
#if defined(FEATURE_IMMEDIATE_AND_RELATIVE)
DEF_INST(load_halfword_immediate);
DEF_INST(add_halfword_immediate);
DEF_INST(compare_halfword_immediate);
DEF_INST(multiply_halfword_immediate);
DEF_INST(multiply_single_register);
DEF_INST(multiply_single);
#endif /*defined(FEATURE_IMMEDIATE_AND_RELATIVE)*/
DEF_INST(load_multiple);
DEF_INST(load_negative_register);
DEF_INST(load_positive_register);
DEF_INST(monitor_call);
DEF_INST(move_immediate);
DEF_INST(move_character);
DEF_INST(move_inverse);
DEF_INST(move_long);
#if defined(FEATURE_COMPARE_AND_MOVE_EXTENDED)
DEF_INST(move_long_extended);
#endif /*defined(FEATURE_COMPARE_AND_MOVE_EXTENDED)*/
DEF_INST(move_numerics);
#if defined(FEATURE_STRING_INSTRUCTION)
DEF_INST(move_string);
#endif /*defined(FEATURE_STRING_INSTRUCTION)*/
DEF_INST(move_with_offset);
DEF_INST(move_zones);
DEF_INST(multiply_register);
DEF_INST(multiply);
DEF_INST(multiply_halfword);
DEF_INST(store);
DEF_INST(store_character);
#ifdef OPTION_OPTINST
#define LRdefgen(r1, r2) DEF_INST(18 ## r1 ## r2)
#define LRdefgenr2(r1) \
  LRdefgen(r1, 0); \
  LRdefgen(r1, 1); \
  LRdefgen(r1, 2); \
  LRdefgen(r1, 3); \
  LRdefgen(r1, 4); \
  LRdefgen(r1, 5); \
  LRdefgen(r1, 6); \
  LRdefgen(r1, 7); \
  LRdefgen(r1, 8); \
  LRdefgen(r1, 9); \
  LRdefgen(r1, A); \
  LRdefgen(r1, B); \
  LRdefgen(r1, C); \
  LRdefgen(r1, D); \
  LRdefgen(r1, E); \
  LRdefgen(r1, F)
LRdefgenr2(0);
LRdefgenr2(1);
LRdefgenr2(2);
LRdefgenr2(3);
LRdefgenr2(4);
LRdefgenr2(5);
LRdefgenr2(6);
LRdefgenr2(7);
LRdefgenr2(8);
LRdefgenr2(9);
LRdefgenr2(A);
LRdefgenr2(B);
LRdefgenr2(C);
LRdefgenr2(D);
LRdefgenr2(E);
LRdefgenr2(F);
#define ALRdefgen(r1, r2) DEF_INST(1E ## r1 ## r2)
#define ALRdefgenr2(r1) \
  ALRdefgen(r1, 0); \
  ALRdefgen(r1, 1); \
  ALRdefgen(r1, 2); \
  ALRdefgen(r1, 3); \
  ALRdefgen(r1, 4); \
  ALRdefgen(r1, 5); \
  ALRdefgen(r1, 6); \
  ALRdefgen(r1, 7); \
  ALRdefgen(r1, 8); \
  ALRdefgen(r1, 9); \
  ALRdefgen(r1, A); \
  ALRdefgen(r1, B); \
  ALRdefgen(r1, C); \
  ALRdefgen(r1, D); \
  ALRdefgen(r1, E); \
  ALRdefgen(r1, F)
ALRdefgenr2(0);
ALRdefgenr2(1);
ALRdefgenr2(2);
ALRdefgenr2(3);
ALRdefgenr2(4);
ALRdefgenr2(5);
ALRdefgenr2(6);
ALRdefgenr2(7);
ALRdefgenr2(8);
ALRdefgenr2(9);
ALRdefgenr2(A);
ALRdefgenr2(B);
ALRdefgenr2(C);
ALRdefgenr2(D);
ALRdefgenr2(E);
ALRdefgenr2(F);
#define CLRdefgen(r1, r2) DEF_INST(15 ## r1 ## r2)
#define CLRdefgenr2(r1) \
  CLRdefgen(r1, 0); \
  CLRdefgen(r1, 1); \
  CLRdefgen(r1, 2); \
  CLRdefgen(r1, 3); \
  CLRdefgen(r1, 4); \
  CLRdefgen(r1, 5); \
  CLRdefgen(r1, 6); \
  CLRdefgen(r1, 7); \
  CLRdefgen(r1, 8); \
  CLRdefgen(r1, 9); \
  CLRdefgen(r1, A); \
  CLRdefgen(r1, B); \
  CLRdefgen(r1, C); \
  CLRdefgen(r1, D); \
  CLRdefgen(r1, E); \
  CLRdefgen(r1, F)
CLRdefgenr2(0);
CLRdefgenr2(1);
CLRdefgenr2(2);
CLRdefgenr2(3);
CLRdefgenr2(4);
CLRdefgenr2(5);
CLRdefgenr2(6);
CLRdefgenr2(7);
CLRdefgenr2(8);
CLRdefgenr2(9);
CLRdefgenr2(A);
CLRdefgenr2(B);
CLRdefgenr2(C);
CLRdefgenr2(D);
CLRdefgenr2(E);
CLRdefgenr2(F);
DEF_INST(4100);
DEF_INST(4110);
DEF_INST(4120);
DEF_INST(4130);
DEF_INST(4140);
DEF_INST(4150);
DEF_INST(4160);
DEF_INST(4170);
DEF_INST(4180);
DEF_INST(4190);
DEF_INST(41A0);
DEF_INST(41B0);
DEF_INST(41C0);
DEF_INST(41D0);
DEF_INST(41E0);
DEF_INST(41F0);
DEF_INST(47_0);
DEF_INST(nop4);
DEF_INST(4710);
DEF_INST(4720);
DEF_INST(4730);
DEF_INST(4740);
DEF_INST(4750);
DEF_INST(4770);
DEF_INST(4780);
DEF_INST(47A0);
DEF_INST(47B0);
DEF_INST(47C0);
DEF_INST(47D0);
DEF_INST(47E0);
DEF_INST(47F0);
DEF_INST(5000);
DEF_INST(5010);
DEF_INST(5020);
DEF_INST(5030);
DEF_INST(5040);
DEF_INST(5050);
DEF_INST(5060);
DEF_INST(5070);
DEF_INST(5080);
DEF_INST(5090);
DEF_INST(50A0);
DEF_INST(50B0);
DEF_INST(50C0);
DEF_INST(50D0);
DEF_INST(50E0);
DEF_INST(50F0);
DEF_INST(5500);
DEF_INST(5510);
DEF_INST(5520);
DEF_INST(5530);
DEF_INST(5540);
DEF_INST(5550);
DEF_INST(5560);
DEF_INST(5570);
DEF_INST(5580);
DEF_INST(5590);
DEF_INST(55A0);
DEF_INST(55B0);
DEF_INST(55C0);
DEF_INST(55D0);
DEF_INST(55E0);
DEF_INST(55F0);
DEF_INST(5800);
DEF_INST(5810);
DEF_INST(5820);
DEF_INST(5830);
DEF_INST(5840);
DEF_INST(5850);
DEF_INST(5860);
DEF_INST(5870);
DEF_INST(5880);
DEF_INST(5890);
DEF_INST(58A0);
DEF_INST(58B0);
DEF_INST(58C0);
DEF_INST(58D0);
DEF_INST(58E0);
DEF_INST(58F0);
DEF_INST(A714);
DEF_INST(A724);
DEF_INST(A734);
DEF_INST(A744);
DEF_INST(A754);
DEF_INST(A774);
DEF_INST(A784);
DEF_INST(A7A4);
DEF_INST(A7B4);
DEF_INST(A7C4);
DEF_INST(A7D4);
DEF_INST(A7E4);
DEF_INST(A7F4);
DEF_INST(BF_7);
DEF_INST(BF_F);
DEF_INST(BF_x);
DEF_INST(D500);
DEF_INST(D501);
DEF_INST(D503);
DEF_INST(D507);
DEF_INST(D200);
#endif /* OPTION_OPTINST */

/* Instructions in general2.c */
DEF_INST(or_register);
DEF_INST(or);
DEF_INST(or_immediate);
DEF_INST(or_character);
#if defined(FEATURE_PERFORM_LOCKED_OPERATION)
DEF_INST(perform_locked_operation);
#endif /*defined(FEATURE_PERFORM_LOCKED_OPERATION)*/
DEF_INST(pack);
#if defined(FEATURE_STRING_INSTRUCTION)
DEF_INST(search_string);
#endif /*defined(FEATURE_STRING_INSTRUCTION)*/
DEF_INST(search_string_unicode);
#if defined(FEATURE_ACCESS_REGISTERS)
DEF_INST(set_access_register);
#endif /*defined(FEATURE_ACCESS_REGISTERS)*/
DEF_INST(set_program_mask);
DEF_INST(shift_left_double);
DEF_INST(shift_left_double_logical);
DEF_INST(shift_left_single);
DEF_INST(shift_left_single_logical);
DEF_INST(shift_right_double);
DEF_INST(shift_right_double_logical);
DEF_INST(shift_right_single);
DEF_INST(shift_right_single_logical);
#if defined(FEATURE_ACCESS_REGISTERS)
DEF_INST(store_access_multiple);
#endif /*defined(FEATURE_ACCESS_REGISTERS)*/
DEF_INST(store_characters_under_mask);
DEF_INST(store_clock);
#if defined(FEATURE_EXTENDED_TOD_CLOCK)
DEF_INST(store_clock_extended);
#endif /*defined(FEATURE_EXTENDED_TOD_CLOCK)*/
#if defined(FEATURE_STORE_CLOCK_FAST)
DEF_INST(store_clock_fast);                                     /*@Z9*/
#endif /*defined(FEATURE_STORE_CLOCK_FAST)*/
DEF_INST(store_halfword);
DEF_INST(store_multiple);
DEF_INST(subtract_register);
DEF_INST(subtract);
DEF_INST(subtract_halfword);
DEF_INST(subtract_logical_register);
DEF_INST(subtract_logical);
DEF_INST(supervisor_call);
DEF_INST(test_and_set);
DEF_INST(test_under_mask);
#if defined(FEATURE_IMMEDIATE_AND_RELATIVE)
DEF_INST(test_under_mask_high);
DEF_INST(test_under_mask_low);
#endif /*defined(FEATURE_IMMEDIATE_AND_RELATIVE)*/
DEF_INST(translate);
DEF_INST(translate_and_test);
DEF_INST(translate_and_test_reverse);
#if defined(FEATURE_PARSING_ENHANCEMENT_FACILITY)
DEF_INST(translate_and_test_extended);                          /*208*/
DEF_INST(translate_and_test_reverse_extended);                  /*208*/
#endif /*defined(FEATURE_PARSING_ENHANCEMENT_FACILITY)*/

#if defined(FEATURE_EXTENDED_TRANSLATION)
DEF_INST(translate_extended);
#endif /*defined(FEATURE_EXTENDED_TRANSLATION)*/
DEF_INST(unpack);
DEF_INST(update_tree);
#ifdef OPTION_OPTINST
DEF_INST(9180);
DEF_INST(9140);
DEF_INST(9120);
DEF_INST(9110);
DEF_INST(9108);
DEF_INST(9104);
DEF_INST(9102);
DEF_INST(9101);
#define SLRdefgen(r1, r2) DEF_INST(1F ## r1 ## r2)
#define SLRdefgenr2(r1) \
  SLRdefgen(r1, 0); \
  SLRdefgen(r1, 1); \
  SLRdefgen(r1, 2); \
  SLRdefgen(r1, 3); \
  SLRdefgen(r1, 4); \
  SLRdefgen(r1, 5); \
  SLRdefgen(r1, 6); \
  SLRdefgen(r1, 7); \
  SLRdefgen(r1, 8); \
  SLRdefgen(r1, 9); \
  SLRdefgen(r1, A); \
  SLRdefgen(r1, B); \
  SLRdefgen(r1, C); \
  SLRdefgen(r1, D); \
  SLRdefgen(r1, E); \
  SLRdefgen(r1, F)
SLRdefgenr2(0);
SLRdefgenr2(1);
SLRdefgenr2(2);
SLRdefgenr2(3);
SLRdefgenr2(4);
SLRdefgenr2(5);
SLRdefgenr2(6);
SLRdefgenr2(7);
SLRdefgenr2(8);
SLRdefgenr2(9);
SLRdefgenr2(A);
SLRdefgenr2(B);
SLRdefgenr2(C);
SLRdefgenr2(D);
SLRdefgenr2(E);
SLRdefgenr2(F);
#endif /* OPTION_OPTINST */


/* Instructions in general3.c */
#if defined(FEATURE_GENERAL_INSTRUCTIONS_EXTENSION_FACILITY)
DEF_INST(add_immediate_long_storage);                           /*208*/
DEF_INST(add_immediate_storage);                                /*208*/
DEF_INST(add_logical_with_signed_immediate);                    /*208*/
DEF_INST(add_logical_with_signed_immediate_long);               /*208*/
DEF_INST(compare_and_branch_register);                          /*208*/
DEF_INST(compare_and_branch_long_register);                     /*208*/
DEF_INST(compare_and_branch_relative_register);                 /*208*/
DEF_INST(compare_and_branch_relative_long_register);            /*208*/
DEF_INST(compare_and_trap_long_register);                       /*208*/
DEF_INST(compare_and_trap_register);                            /*208*/
DEF_INST(compare_halfword_immediate_halfword_storage);          /*208*/
DEF_INST(compare_halfword_immediate_long_storage);              /*208*/
DEF_INST(compare_halfword_immediate_storage);                   /*208*/
DEF_INST(compare_halfword_long);                                /*208*/
DEF_INST(compare_halfword_relative_long);                       /*208*/
DEF_INST(compare_halfword_relative_long_long);                  /*208*/
DEF_INST(compare_immediate_and_branch);                         /*208*/
DEF_INST(compare_immediate_and_branch_long);                    /*208*/
DEF_INST(compare_immediate_and_branch_relative);                /*208*/
DEF_INST(compare_immediate_and_branch_relative_long);           /*208*/
DEF_INST(compare_immediate_and_trap);                           /*208*/
DEF_INST(compare_immediate_and_trap_long);                      /*208*/
DEF_INST(compare_logical_and_branch_long_register);             /*208*/
DEF_INST(compare_logical_and_branch_register);                  /*208*/
DEF_INST(compare_logical_and_branch_relative_long_register);    /*208*/
DEF_INST(compare_logical_and_branch_relative_register);         /*208*/
DEF_INST(compare_logical_and_trap_long_register);               /*208*/
DEF_INST(compare_logical_and_trap_register);                    /*208*/
DEF_INST(compare_logical_immediate_and_branch);                 /*208*/
DEF_INST(compare_logical_immediate_and_branch_long);            /*208*/
DEF_INST(compare_logical_immediate_and_branch_relative);        /*208*/
DEF_INST(compare_logical_immediate_and_branch_relative_long);   /*208*/
DEF_INST(compare_logical_immediate_and_trap_fullword);          /*208*/
DEF_INST(compare_logical_immediate_and_trap_long);              /*208*/
DEF_INST(compare_logical_immediate_fullword_storage);           /*208*/
DEF_INST(compare_logical_immediate_halfword_storage);           /*208*/
DEF_INST(compare_logical_immediate_long_storage);               /*208*/
DEF_INST(compare_logical_relative_long);                        /*208*/
DEF_INST(compare_logical_relative_long_halfword);               /*208*/
DEF_INST(compare_logical_relative_long_long);                   /*208*/
DEF_INST(compare_logical_relative_long_long_fullword);          /*208*/
DEF_INST(compare_logical_relative_long_long_halfword);          /*208*/
DEF_INST(compare_relative_long);                                /*208*/
DEF_INST(compare_relative_long_long);                           /*208*/
DEF_INST(compare_relative_long_long_fullword);                  /*208*/
DEF_INST(extract_cache_attribute);                              /*208*/
DEF_INST(load_address_extended_y);                              /*208*/
DEF_INST(load_and_test_long_fullword);                          /*208*/
DEF_INST(load_halfword_relative_long);                          /*208*/
DEF_INST(load_halfword_relative_long_long);                     /*208*/
DEF_INST(load_logical_halfword_relative_long);                  /*208*/
DEF_INST(load_logical_halfword_relative_long_long);             /*208*/
DEF_INST(load_logical_relative_long_long_fullword);             /*208*/
DEF_INST(load_relative_long);                                   /*208*/
DEF_INST(load_relative_long_long);                              /*208*/
DEF_INST(load_relative_long_long_fullword);                     /*208*/
DEF_INST(move_fullword_from_halfword_immediate);                /*208*/
DEF_INST(move_halfword_from_halfword_immediate);                /*208*/
DEF_INST(move_long_from_halfword_immediate);                    /*208*/
DEF_INST(multiply_halfword_y);                                  /*208*/
DEF_INST(multiply_single_immediate_fullword);                   /*208*/
DEF_INST(multiply_single_immediate_long_fullword);              /*208*/
DEF_INST(multiply_y);                                           /*208*/
DEF_INST(prefetch_data);                                        /*208*/
DEF_INST(prefetch_data_relative_long);                          /*208*/
DEF_INST(rotate_then_and_selected_bits_long_reg);               /*208*/
DEF_INST(rotate_then_exclusive_or_selected_bits_long_reg);      /*208*/
DEF_INST(rotate_then_insert_selected_bits_long_reg);            /*208*/
DEF_INST(rotate_then_or_selected_bits_long_reg);                /*208*/
DEF_INST(store_halfword_relative_long);                         /*208*/
DEF_INST(store_relative_long);                                  /*208*/
DEF_INST(store_relative_long_long);                             /*208*/
#endif /*defined(FEATURE_GENERAL_INSTRUCTIONS_EXTENSION_FACILITY)*/

#if defined(FEATURE_HIGH_WORD_FACILITY)
DEF_INST(add_high_high_high_register);                          /*810*/
DEF_INST(add_high_high_low_register);                           /*810*/
DEF_INST(add_high_immediate);                                   /*810*/
DEF_INST(add_logical_high_high_high_register);                  /*810*/
DEF_INST(add_logical_high_high_low_register);                   /*810*/
DEF_INST(add_logical_with_signed_immediate_high);               /*810*/
DEF_INST(add_logical_with_signed_immediate_high_n);             /*810*/
DEF_INST(branch_relative_on_count_high);                        /*810*/
DEF_INST(compare_high_high_register);                           /*810*/
DEF_INST(compare_high_low_register);                            /*810*/
DEF_INST(compare_high_fullword);                                /*810*/
DEF_INST(compare_high_immediate);                               /*810*/
DEF_INST(compare_logical_high_high_register);                   /*810*/
DEF_INST(compare_logical_high_low_register);                    /*810*/
DEF_INST(compare_logical_high_fullword);                        /*810*/
DEF_INST(compare_logical_high_immediate);                       /*810*/
DEF_INST(load_byte_high);                                       /*810*/
DEF_INST(load_fullword_high);                                   /*810*/
DEF_INST(load_halfword_high);                                   /*810*/
DEF_INST(load_logical_character_high);                          /*810*/
DEF_INST(load_logical_halfword_high);                           /*810*/
DEF_INST(rotate_then_insert_selected_bits_high_long_reg);       /*810*/
DEF_INST(rotate_then_insert_selected_bits_low_long_reg);        /*810*/
DEF_INST(store_character_high);                                 /*810*/
DEF_INST(store_fullword_high);                                  /*810*/
DEF_INST(store_halfword_high);                                  /*810*/
DEF_INST(subtract_high_high_high_register);                     /*810*/
DEF_INST(subtract_high_high_low_register);                      /*810*/
DEF_INST(subtract_logical_high_high_high_register);             /*810*/
DEF_INST(subtract_logical_high_high_low_register);              /*810*/
#endif /*defined(FEATURE_HIGH_WORD_FACILITY)*/

#if defined(FEATURE_INTERLOCKED_ACCESS_FACILITY)
DEF_INST(load_and_add);                                         /*810*/
DEF_INST(load_and_add_logical);                                 /*810*/
DEF_INST(load_and_and);                                         /*810*/
DEF_INST(load_and_exclusive_or);                                /*810*/
DEF_INST(load_and_or);                                          /*810*/
DEF_INST(load_pair_disjoint);                                   /*810*/
#endif /*defined(FEATURE_INTERLOCKED_ACCESS_FACILITY)*/

#if defined(FEATURE_INTERLOCKED_ACCESS_FACILITY) && defined(FEATURE_ESAME)
DEF_INST(load_and_add_long);                                    /*810*/
DEF_INST(load_and_add_logical_long);                            /*810*/
DEF_INST(load_and_and_long);                                    /*810*/
DEF_INST(load_and_exclusive_or_long);                           /*810*/
DEF_INST(load_and_or_long);                                     /*810*/
DEF_INST(load_pair_disjoint_long);                              /*810*/
#endif /*defined(FEATURE_INTERLOCKED_ACCESS_FACILITY) && defined(FEATURE_ESAME)*/

#if defined(FEATURE_LOAD_STORE_ON_CONDITION_FACILITY)
DEF_INST(load_on_condition_register);                           /*810*/
DEF_INST(load_on_condition_long_register);                      /*810*/
DEF_INST(load_on_condition);                                    /*810*/
DEF_INST(load_on_condition_long);                               /*810*/
DEF_INST(store_on_condition);                                   /*810*/
DEF_INST(store_on_condition_long);                              /*810*/
#endif /*defined(FEATURE_LOAD_STORE_ON_CONDITION_FACILITY)*/

#if defined(FEATURE_DISTINCT_OPERANDS_FACILITY)
DEF_INST(add_distinct_register);                                /*810*/
DEF_INST(add_distinct_long_register);                           /*810*/
DEF_INST(add_distinct_halfword_immediate);                      /*810*/
DEF_INST(add_distinct_long_halfword_immediate);                 /*810*/
DEF_INST(add_logical_distinct_register);                        /*810*/
DEF_INST(add_logical_distinct_long_register);                   /*810*/
DEF_INST(add_logical_distinct_signed_halfword_immediate);       /*810*/
DEF_INST(add_logical_distinct_long_signed_halfword_immediate);  /*810*/
DEF_INST(and_distinct_register);                                /*810*/
DEF_INST(and_distinct_long_register);                           /*810*/
DEF_INST(exclusive_or_distinct_register);                       /*810*/
DEF_INST(exclusive_or_distinct_long_register);                  /*810*/
DEF_INST(or_distinct_register);                                 /*810*/
DEF_INST(or_distinct_long_register);                            /*810*/
DEF_INST(shift_right_single_distinct);                          /*810*/
DEF_INST(shift_left_single_distinct);                           /*810*/
DEF_INST(shift_right_single_logical_distinct);                  /*810*/
DEF_INST(shift_left_single_logical_distinct);                   /*810*/
DEF_INST(subtract_distinct_register);                           /*810*/
DEF_INST(subtract_distinct_long_register);                      /*810*/
DEF_INST(subtract_logical_distinct_register);                   /*810*/
DEF_INST(subtract_logical_distinct_long_register);              /*810*/
#endif /*defined(FEATURE_DISTINCT_OPERANDS_FACILITY)*/

#if defined(FEATURE_POPULATION_COUNT_FACILITY)
DEF_INST(population_count);                                     /*810*/
#endif /*defined(FEATURE_POPULATION_COUNT_FACILITY)*/

#if defined(FEATURE_LOAD_AND_TRAP_FACILITY)
DEF_INST(load_and_trap);                                        /*912*/
DEF_INST(load_long_and_trap);                                   /*912*/
DEF_INST(load_fullword_high_and_trap);                          /*912*/
DEF_INST(load_logical_long_fullword_and_trap);                  /*912*/
DEF_INST(load_logical_long_thirtyone_and_trap);                 /*912*/
#endif /*defined(FEATURE_LOAD_AND_TRAP_FACILITY)*/

#if defined(FEATURE_MISC_INSTRUCTION_EXTENSIONS_FACILITY)
DEF_INST(compare_logical_and_trap);                             /*912*/
DEF_INST(compare_logical_and_trap_long);                        /*912*/
DEF_INST(rotate_then_insert_selected_bits_long_reg_n);          /*912*/
#endif /*defined(FEATURE_MISC_INSTRUCTION_EXTENSIONS_FACILITY)*/

#if defined(FEATURE_EXECUTION_HINT_FACILITY)
DEF_INST(branch_prediction_preload);                            /*912*/
DEF_INST(branch_prediction_relative_preload);                   /*912*/
DEF_INST(next_instruction_access_intent);                       /*912*/
#endif /*defined(FEATURE_EXECUTION_HINT_FACILITY)*/

/* Instructions in io.c */
#if defined(FEATURE_CHANNEL_SUBSYSTEM)
DEF_INST(clear_subchannel);
DEF_INST(halt_subchannel);
DEF_INST(modify_subchannel);
DEF_INST(resume_subchannel);
DEF_INST(set_address_limit);
DEF_INST(set_channel_monitor);
DEF_INST(reset_channel_path);
DEF_INST(start_subchannel);
DEF_INST(store_channel_path_status);
DEF_INST(store_channel_report_word);
DEF_INST(store_subchannel);
DEF_INST(test_pending_interruption);
DEF_INST(test_subchannel);
#endif /*defined(FEATURE_CHANNEL_SUBSYSTEM)*/
#if defined(FEATURE_CANCEL_IO_FACILITY)
DEF_INST(cancel_subchannel);
#endif /*defined(FEATURE_CANCEL_IO_FACILITY)*/
#if defined(FEATURE_S370_CHANNEL)
DEF_INST(start_io);
DEF_INST(test_io);
DEF_INST(halt_io);
DEF_INST(test_channel);
DEF_INST(store_channel_id);
#endif /*defined(FEATURE_S370_CHANNEL)*/
#if defined(FEATURE_CHANNEL_SWITCHING)
DEF_INST(connect_channel_set);
DEF_INST(disconnect_channel_set);
#endif /*defined(FEATURE_CHANNEL_SWITCHING)*/


/* Instructions in service.c */
#if defined(FEATURE_SERVICE_PROCESSOR)
DEF_INST(service_call);
#endif /*defined(FEATURE_SERVICE_PROCESSOR)*/


/* Instructions in chsc.c */
#if defined(FEATURE_CHSC)
DEF_INST(channel_subsystem_call);
#endif /*defined(FEATURE_CHSC)*/


/* Instructions in xstore.c */
#if defined(FEATURE_EXPANDED_STORAGE)
DEF_INST(page_in);
DEF_INST(page_out);
#endif /*defined(FEATURE_EXPANDED_STORAGE)*/
#if defined(FEATURE_MOVE_PAGE_FACILITY_2)
DEF_INST(move_page);
DEF_INST(invalidate_expanded_storage_block_entry);
#endif /*defined(FEATURE_MOVE_PAGE_FACILITY_2)*/


/* Instructions in vector.c */
#if defined(FEATURE_VECTOR_FACILITY)
DEF_INST(v_test_vmr);
DEF_INST(v_complement_vmr);
DEF_INST(v_count_left_zeros_in_vmr);
DEF_INST(v_count_ones_in_vmr);
DEF_INST(v_extract_vct);
DEF_INST(v_extract_vector_modes);
DEF_INST(v_restore_vr);
DEF_INST(v_save_changed_vr);
DEF_INST(v_save_vr);
DEF_INST(v_load_vmr);
DEF_INST(v_load_vmr_complement);
DEF_INST(v_store_vmr);
DEF_INST(v_and_to_vmr);
DEF_INST(v_or_to_vmr);
DEF_INST(v_exclusive_or_to_vmr);
DEF_INST(v_save_vsr);
DEF_INST(v_save_vmr);
DEF_INST(v_restore_vsr);
DEF_INST(v_restore_vmr);
DEF_INST(v_load_vct_from_address);
DEF_INST(v_clear_vr);
DEF_INST(v_set_vector_mask_mode);
DEF_INST(v_load_vix_from_address);
DEF_INST(v_store_vector_parameters);
DEF_INST(v_save_vac);
DEF_INST(v_restore_vac);
#endif /*defined(FEATURE_VECTOR_FACILITY)*/


/* Instructions in esame.c */
#if defined(FEATURE_BINARY_FLOATING_POINT)
DEF_INST(store_fpc);
DEF_INST(load_fpc);
DEF_INST(set_fpc);
DEF_INST(extract_fpc);
DEF_INST(set_bfp_rounding_mode_2bit);
#endif /*defined(FEATURE_BINARY_FLOATING_POINT)*/

#if defined(FEATURE_FLOATING_POINT_EXTENSION_FACILITY)
DEF_INST(set_bfp_rounding_mode_3bit);                           /*810*/
#endif /*defined(FEATURE_FLOATING_POINT_EXTENSION_FACILITY)*/
#if defined(FEATURE_LINKAGE_STACK)
DEF_INST(trap2);
DEF_INST(trap4);
#endif /*defined(FEATURE_LINKAGE_STACK)*/
#if defined(FEATURE_RESUME_PROGRAM)
DEF_INST(resume_program);
#endif /*defined(FEATURE_RESUME_PROGRAM)*/
DEF_INST(trace_long);
DEF_INST(convert_to_binary_long);
DEF_INST(convert_to_decimal_long);
DEF_INST(multiply_logical_long);
DEF_INST(multiply_logical_long_register);
DEF_INST(divide_logical_long);
DEF_INST(divide_logical_long_register);
DEF_INST(add_logical_carry_long_register);
DEF_INST(subtract_logical_borrow_long_register);
DEF_INST(add_logical_carry_long);
DEF_INST(subtract_logical_borrow_long);
#if defined(FEATURE_ESAME_N3_ESA390) || defined(FEATURE_ESAME)
DEF_INST(add_logical_carry);
DEF_INST(add_logical_carry_register);
DEF_INST(divide_logical);
DEF_INST(divide_logical_register);
DEF_INST(extract_psw);
DEF_INST(load_address_relative_long);
DEF_INST(multiply_logical);
DEF_INST(multiply_logical_register);
DEF_INST(rotate_left_single_logical);
DEF_INST(set_addressing_mode_24);
DEF_INST(set_addressing_mode_31);
DEF_INST(subtract_logical_borrow);
DEF_INST(subtract_logical_borrow_register);
#endif /*defined(FEATURE_ESAME_N3_ESA390) || defined(FEATURE_ESAME)*/
DEF_INST(divide_single_long);
DEF_INST(divide_single_long_fullword);
DEF_INST(divide_single_long_register);
DEF_INST(divide_single_long_fullword_register);
DEF_INST(load_logical_long_character);
DEF_INST(load_logical_long_halfword);
DEF_INST(store_pair_to_quadword);
DEF_INST(load_pair_from_quadword);
DEF_INST(extract_stacked_registers_long);
DEF_INST(extract_and_set_extended_authority);
DEF_INST(test_addressing_mode);
#if defined(FEATURE_ENHANCED_DAT_FACILITY)
DEF_INST(perform_frame_management_function);                    /*208*/
#endif /*defined(FEATURE_ENHANCED_DAT_FACILITY)*/
#if defined(FEATURE_TOD_CLOCK_STEERING)
DEF_INST(perform_timing_facility_function);                     /*@Z9*/
#endif /*defined(FEATURE_TOD_CLOCK_STEERING)*/
#if defined(FEATURE_CONFIGURATION_TOPOLOGY_FACILITY)
DEF_INST(perform_topology_function);                            /*208*/
#endif /*defined(FEATURE_CONFIGURATION_TOPOLOGY_FACILITY)*/
#if defined(FEATURE_RESET_REFERENCE_BITS_MULTIPLE_FACILITY)
DEF_INST(reset_reference_bits_multiple);                        /*810*/
#endif /*defined(FEATURE_RESET_REFERENCE_BITS_MULTIPLE_FACILITY)*/
#if defined(FEATURE_STORE_FACILITY_LIST)
DEF_INST(store_facility_list);
#endif /*defined(STORE_FACILITY_LIST)*/
#if defined(FEATURE_STORE_FACILITY_LIST_EXTENDED)
DEF_INST(store_facility_list_extended);                         /*@Z9*/
#endif /*defined(FEATURE_STORE_FACILITY_LIST_EXTENDED)*/
DEF_INST(load_long_halfword_immediate);
DEF_INST(add_long_halfword_immediate);
DEF_INST(multiply_long_halfword_immediate);
DEF_INST(compare_long_halfword_immediate);
DEF_INST(and_long);
DEF_INST(or_long);
DEF_INST(exclusive_or_long);
DEF_INST(and_long_register);
DEF_INST(or_long_register);
DEF_INST(exclusive_or_long_register);
DEF_INST(load_long_register);
DEF_INST(add_logical_long_register);
DEF_INST(add_logical_long_fullword_register);
DEF_INST(subtract_logical_long_register);
DEF_INST(subtract_logical_long_fullword_register);
DEF_INST(load_control_long);
DEF_INST(store_control_long);
DEF_INST(load_multiple_disjoint);
DEF_INST(load_multiple_high);
DEF_INST(load_multiple_long);
DEF_INST(store_multiple_high);
DEF_INST(store_multiple_long);
DEF_INST(load_using_real_address_long);
DEF_INST(store_using_real_address_long);
DEF_INST(set_addressing_mode_24);
DEF_INST(set_addressing_mode_31);
DEF_INST(set_addressing_mode_64);
DEF_INST(load_program_status_word_extended);
DEF_INST(store_long);
DEF_INST(store_real_address);
DEF_INST(load_long);
DEF_INST(multiply_single_long_register);
DEF_INST(multiply_single_long_fullword_register);
DEF_INST(multiply_single_long);
DEF_INST(multiply_single_long_fullword);
DEF_INST(rotate_left_single_logical_long);
DEF_INST(shift_right_single_long);
DEF_INST(shift_left_single_long);
DEF_INST(shift_right_single_logical_long);
DEF_INST(shift_left_single_logical_long);
DEF_INST(compare_logical_long);
DEF_INST(compare_logical_long_fullword);
DEF_INST(compare_logical_long_fullword_register);
DEF_INST(load_logical_long_thirtyone_register);
DEF_INST(compare_logical_long_register);
DEF_INST(test_under_mask_high_high);
DEF_INST(test_under_mask_high_low);
DEF_INST(branch_relative_on_count_long);
DEF_INST(load_positive_long_register);
DEF_INST(load_negative_long_register);
DEF_INST(load_and_test_long_register);
DEF_INST(load_complement_long_register);
DEF_INST(load_real_address_long);
DEF_INST(load_long_fullword_register);
DEF_INST(add_long_register);
DEF_INST(add_long_fullword_register);
DEF_INST(subtract_long_register);
DEF_INST(subtract_long_fullword_register);
DEF_INST(add_logical_long);
DEF_INST(add_logical_long_fullword);
DEF_INST(add_long);
DEF_INST(add_long_fullword);
DEF_INST(subtract_logical_long);
DEF_INST(subtract_logical_long_fullword);
DEF_INST(subtract_long);
DEF_INST(subtract_long_fullword);
DEF_INST(compare_long_register);
DEF_INST(compare_long);
DEF_INST(branch_on_count_long_register);
DEF_INST(branch_on_count_long);
DEF_INST(compare_and_swap_long);
DEF_INST(compare_double_and_swap_long);
DEF_INST(branch_on_index_high_long);
DEF_INST(branch_on_index_low_or_equal_long);
DEF_INST(branch_relative_on_index_high_long);
DEF_INST(branch_relative_on_index_low_or_equal_long);
DEF_INST(compare_logical_characters_under_mask_high);
DEF_INST(store_characters_under_mask_high);
DEF_INST(insert_characters_under_mask_high);
DEF_INST(branch_relative_on_condition_long);
DEF_INST(branch_relative_and_save_long);
DEF_INST(compare_long_fullword_register);
DEF_INST(load_positive_long_fullword_register);
DEF_INST(load_negative_long_fullword_register);
DEF_INST(load_and_test_long_fullword_register);
DEF_INST(load_complement_long_fullword_register);
DEF_INST(load_long_fullword);
DEF_INST(load_long_halfword);
DEF_INST(compare_long_fullword);
DEF_INST(load_logical_long_fullword_register);
DEF_INST(load_logical_long_fullword);
DEF_INST(load_logical_long_thirtyone);
DEF_INST(insert_immediate_high_high);
DEF_INST(insert_immediate_high_low);
DEF_INST(insert_immediate_low_high);
DEF_INST(insert_immediate_low_low);
DEF_INST(and_immediate_high_high);
DEF_INST(and_immediate_high_low);
DEF_INST(and_immediate_low_high);
DEF_INST(and_immediate_low_low);
DEF_INST(or_immediate_high_high);
DEF_INST(or_immediate_high_low);
DEF_INST(or_immediate_low_high);
DEF_INST(or_immediate_low_low);
DEF_INST(load_logical_immediate_high_high);
DEF_INST(load_logical_immediate_high_low);
DEF_INST(load_logical_immediate_low_high);
DEF_INST(load_logical_immediate_low_low);
#if defined(FEATURE_LOAD_REVERSED) || defined(FEATURE_ESAME_N3_ESA390)
DEF_INST(load_reversed_register);
DEF_INST(load_reversed_long);
DEF_INST(load_reversed);
DEF_INST(load_reversed_half);
DEF_INST(store_reversed);
DEF_INST(store_reversed_half);
#if defined(FEATURE_ESAME)
DEF_INST(load_reversed_long_register);
DEF_INST(store_reversed_long);
#endif /*defined(FEATURE_ESAME)*/
#endif /*defined(FEATURE_LOAD_REVERSED) || defined(FEATURE_ESAME_N3_ESA390)*/
#if defined(FEATURE_EXTENDED_TRANSLATION_FACILITY_2)
DEF_INST(pack_ascii);
DEF_INST(pack_unicode);
DEF_INST(unpack_ascii);
DEF_INST(unpack_unicode);
DEF_INST(translate_two_to_two);
DEF_INST(translate_two_to_one);
DEF_INST(translate_one_to_two);
DEF_INST(translate_one_to_one);
DEF_INST(move_long_unicode);
DEF_INST(compare_logical_long_unicode);
#endif /*defined(FEATURE_EXTENDED_TRANSLATION_FACILITY_2)*/
#if defined(FEATURE_LONG_DISPLACEMENT)
DEF_INST(add_y);
DEF_INST(add_halfword_y);
DEF_INST(add_logical_y);
DEF_INST(and_immediate_y);
DEF_INST(and_y);
DEF_INST(compare_y);
DEF_INST(compare_and_swap_y);
DEF_INST(compare_double_and_swap_y);
DEF_INST(compare_halfword_y);
DEF_INST(compare_logical_y);
DEF_INST(compare_logical_immediate_y);
DEF_INST(compare_logical_characters_under_mask_y);
DEF_INST(convert_to_binary_y);
DEF_INST(convert_to_decimal_y);
DEF_INST(exclusive_or_immediate_y);
DEF_INST(exclusive_or_y);
DEF_INST(insert_character_y);
DEF_INST(insert_characters_under_mask_y);
DEF_INST(load_y);
#if defined(FEATURE_ACCESS_REGISTERS)
DEF_INST(load_access_multiple_y);
#endif /*defined(FEATURE_ACCESS_REGISTERS)*/
DEF_INST(load_address_y);
DEF_INST(load_byte);
DEF_INST(load_byte_long);
DEF_INST(load_halfword_y);
DEF_INST(load_multiple_y);
DEF_INST(load_real_address_y);
DEF_INST(move_immediate_y);
DEF_INST(multiply_single_y);
DEF_INST(or_immediate_y);
DEF_INST(or_y);
DEF_INST(store_y);
#if defined(FEATURE_ACCESS_REGISTERS)
DEF_INST(store_access_multiple_y);
#endif /*defined(FEATURE_ACCESS_REGISTERS)*/
DEF_INST(store_character_y);
DEF_INST(store_characters_under_mask_y);
DEF_INST(store_halfword_y);
DEF_INST(store_multiple_y);
DEF_INST(subtract_y);
DEF_INST(subtract_halfword_y);
DEF_INST(subtract_logical_y);
DEF_INST(test_under_mask_y);
#endif /*defined(FEATURE_LONG_DISPLACEMENT)*/

#if defined(FEATURE_DAT_ENHANCEMENT)
DEF_INST(compare_and_swap_and_purge_long);
DEF_INST(invalidate_dat_table_entry);
#endif /*defined(FEATURE_DAT_ENHANCEMENT)*/
#if defined(FEATURE_DAT_ENHANCEMENT_FACILITY_2)
DEF_INST(load_page_table_entry_address);                        /*@Z9*/
#endif /*defined(FEATURE_DAT_ENHANCEMENT_FACILITY_2)*/
#if defined(FEATURE_EXTENDED_IMMEDIATE)
DEF_INST(add_fullword_immediate);                               /*@Z9*/
DEF_INST(add_long_fullword_immediate);                          /*@Z9*/
DEF_INST(add_logical_fullword_immediate);                       /*@Z9*/
DEF_INST(add_logical_long_fullword_immediate);                  /*@Z9*/
DEF_INST(and_immediate_high_fullword);                          /*@Z9*/
DEF_INST(and_immediate_low_fullword);                           /*@Z9*/
DEF_INST(compare_fullword_immediate);                           /*@Z9*/
DEF_INST(compare_long_fullword_immediate);                      /*@Z9*/
DEF_INST(compare_logical_fullword_immediate);                   /*@Z9*/
DEF_INST(compare_logical_long_fullword_immediate);              /*@Z9*/
DEF_INST(exclusive_or_immediate_high_fullword);                 /*@Z9*/
DEF_INST(exclusive_or_immediate_low_fullword);                  /*@Z9*/
DEF_INST(insert_immediate_high_fullword);                       /*@Z9*/
DEF_INST(insert_immediate_low_fullword);                        /*@Z9*/
DEF_INST(load_long_fullword_immediate);                         /*@Z9*/
DEF_INST(load_logical_immediate_high_fullword);                 /*@Z9*/
DEF_INST(load_logical_immediate_low_fullword);                  /*@Z9*/
DEF_INST(or_immediate_high_fullword);                           /*@Z9*/
DEF_INST(or_immediate_low_fullword);                            /*@Z9*/
DEF_INST(subtract_logical_fullword_immediate);                  /*@Z9*/
DEF_INST(subtract_logical_long_fullword_immediate);             /*@Z9*/

DEF_INST(load_and_test);                                        /*@Z9*/
DEF_INST(load_and_test_long);                                   /*@Z9*/
DEF_INST(load_byte_register);                                   /*@Z9*/
DEF_INST(load_long_byte_register);                              /*@Z9*/
DEF_INST(load_halfword_register);                               /*@Z9*/
DEF_INST(load_long_halfword_register);                          /*@Z9*/
DEF_INST(load_logical_character);                               /*@Z9*/
DEF_INST(load_logical_character_register);                      /*@Z9*/
DEF_INST(load_logical_long_character_register);                 /*@Z9*/
DEF_INST(load_logical_halfword);                                /*@Z9*/
DEF_INST(load_logical_halfword_register);                       /*@Z9*/
DEF_INST(load_logical_long_halfword_register);                  /*@Z9*/
DEF_INST(find_leftmost_one_long_register);                      /*@Z9*/
#endif /*defined(FEATURE_EXTENDED_IMMEDIATE)*/
#if defined(FEATURE_EXTRACT_CPU_TIME)
DEF_INST(extract_cpu_time);
#endif /*defined(FEATURE_EXTRACT_CPU_TIME)*/
#if defined(FEATURE_LOAD_PROGRAM_PARAMETER_FACILITY)
DEF_INST(load_program_parameter);                               /*810*/
#endif /*defined(FEATURE_LOAD_PROGRAM_PARAMETER_FACILITY)*/
#ifdef OPTION_OPTINST
DEF_INST(E3_0______04);
DEF_INST(E3_0______24);
#endif /* #ifdef OPTION_OPTINST */

/* Instructions in ecpsvm.c */
#if defined(FEATURE_ECPSVM)
DEF_INST(ecpsvm_basic_freex);
DEF_INST(ecpsvm_basic_fretx);
DEF_INST(ecpsvm_lock_page);
DEF_INST(ecpsvm_unlock_page);
DEF_INST(ecpsvm_decode_next_ccw);
DEF_INST(ecpsvm_free_ccwstor);
DEF_INST(ecpsvm_locate_vblock);
DEF_INST(ecpsvm_disp1);
DEF_INST(ecpsvm_tpage);
DEF_INST(ecpsvm_tpage_lock);
DEF_INST(ecpsvm_inval_segtab);
DEF_INST(ecpsvm_inval_ptable);
DEF_INST(ecpsvm_decode_first_ccw);
DEF_INST(ecpsvm_dispatch_main);
DEF_INST(ecpsvm_locate_rblock);
DEF_INST(ecpsvm_comm_ccwproc);
DEF_INST(ecpsvm_unxlate_ccw);
DEF_INST(ecpsvm_disp2);
DEF_INST(ecpsvm_store_level);
DEF_INST(ecpsvm_loc_chgshrpg);
DEF_INST(ecpsvm_extended_freex);
DEF_INST(ecpsvm_extended_fretx);
DEF_INST(ecpsvm_prefmach_assist);
#endif /*defined(FEATURE_ECPSVM)*/


/* Instructions in ieee.c */
#if defined(FEATURE_FPS_EXTENSIONS)
DEF_INST(convert_bfp_long_to_float_long_reg);
DEF_INST(convert_bfp_short_to_float_long_reg);
DEF_INST(convert_float_long_to_bfp_long_reg);
DEF_INST(convert_float_long_to_bfp_short_reg);
#endif /*defined(FEATURE_FPS_EXTENSIONS)*/

#if defined(FEATURE_BINARY_FLOATING_POINT)
DEF_INST(add_bfp_ext_reg);
DEF_INST(add_bfp_long_reg);
DEF_INST(add_bfp_long);
DEF_INST(add_bfp_short_reg);
DEF_INST(add_bfp_short);
DEF_INST(compare_bfp_ext_reg);
DEF_INST(compare_bfp_long_reg);
DEF_INST(compare_bfp_long);
DEF_INST(compare_bfp_short_reg);
DEF_INST(compare_bfp_short);
DEF_INST(compare_and_signal_bfp_ext_reg);
DEF_INST(compare_and_signal_bfp_long_reg);
DEF_INST(compare_and_signal_bfp_long);
DEF_INST(compare_and_signal_bfp_short_reg);
DEF_INST(compare_and_signal_bfp_short);
DEF_INST(convert_fix32_to_bfp_ext_reg);
DEF_INST(convert_fix32_to_bfp_long_reg);
DEF_INST(convert_fix32_to_bfp_short_reg);
DEF_INST(convert_fix64_to_bfp_ext_reg);
DEF_INST(convert_fix64_to_bfp_long_reg);
DEF_INST(convert_fix64_to_bfp_short_reg);
DEF_INST(convert_bfp_ext_to_fix32_reg);
DEF_INST(convert_bfp_long_to_fix32_reg);
DEF_INST(convert_bfp_ext_to_fix64_reg);
DEF_INST(convert_bfp_long_to_fix64_reg);
DEF_INST(convert_bfp_short_to_fix64_reg);
DEF_INST(convert_bfp_short_to_fix32_reg);
DEF_INST(divide_bfp_ext_reg);
DEF_INST(divide_bfp_long_reg);
DEF_INST(divide_bfp_long);
DEF_INST(divide_bfp_short_reg);
DEF_INST(divide_bfp_short);
DEF_INST(divide_integer_bfp_long_reg);
DEF_INST(divide_integer_bfp_short_reg);
DEF_INST(load_and_test_bfp_ext_reg);
DEF_INST(load_and_test_bfp_long_reg);
DEF_INST(load_and_test_bfp_short_reg);
DEF_INST(load_fp_int_bfp_ext_reg);
DEF_INST(load_fp_int_bfp_long_reg);
DEF_INST(load_fp_int_bfp_short_reg);
DEF_INST(load_lengthened_bfp_short_to_long_reg);
DEF_INST(load_lengthened_bfp_short_to_long);
DEF_INST(load_lengthened_bfp_long_to_ext_reg);
DEF_INST(load_lengthened_bfp_long_to_ext);
DEF_INST(load_lengthened_bfp_short_to_ext_reg);
DEF_INST(load_lengthened_bfp_short_to_ext);
DEF_INST(load_negative_bfp_ext_reg);
DEF_INST(load_negative_bfp_long_reg);
DEF_INST(load_negative_bfp_short_reg);
DEF_INST(load_complement_bfp_ext_reg);
DEF_INST(load_complement_bfp_long_reg);
DEF_INST(load_complement_bfp_short_reg);
DEF_INST(load_positive_bfp_ext_reg);
DEF_INST(load_positive_bfp_long_reg);
DEF_INST(load_positive_bfp_short_reg);
DEF_INST(load_rounded_bfp_long_to_short_reg);
DEF_INST(load_rounded_bfp_ext_to_long_reg);
DEF_INST(load_rounded_bfp_ext_to_short_reg);
DEF_INST(multiply_bfp_ext_reg);
DEF_INST(multiply_bfp_long_to_ext_reg);
DEF_INST(multiply_bfp_long_to_ext);
DEF_INST(multiply_bfp_long_reg);
DEF_INST(multiply_bfp_long);
DEF_INST(multiply_bfp_short_to_long_reg);
DEF_INST(multiply_bfp_short_to_long);
DEF_INST(multiply_bfp_short_reg);
DEF_INST(multiply_bfp_short);
DEF_INST(multiply_add_bfp_long_reg);
DEF_INST(multiply_add_bfp_long);
DEF_INST(multiply_add_bfp_short_reg);
DEF_INST(multiply_add_bfp_short);
DEF_INST(multiply_subtract_bfp_long_reg);
DEF_INST(multiply_subtract_bfp_long);
DEF_INST(multiply_subtract_bfp_short_reg);
DEF_INST(multiply_subtract_bfp_short);
DEF_INST(squareroot_bfp_ext_reg);
DEF_INST(squareroot_bfp_long_reg);
DEF_INST(squareroot_bfp_long);
DEF_INST(squareroot_bfp_short_reg);
DEF_INST(squareroot_bfp_short);
DEF_INST(subtract_bfp_ext_reg);
DEF_INST(subtract_bfp_long_reg);
DEF_INST(subtract_bfp_long);
DEF_INST(subtract_bfp_short_reg);
DEF_INST(subtract_bfp_short);
DEF_INST(test_data_class_bfp_short);
DEF_INST(test_data_class_bfp_long);
DEF_INST(test_data_class_bfp_ext);
#endif /*defined(FEATURE_BINARY_FLOATING_POINT)*/

#if defined(FEATURE_FLOATING_POINT_EXTENSION_FACILITY)
DEF_INST(convert_fix32_to_dfp_ext_reg);                         /*810*/
DEF_INST(convert_fix32_to_dfp_long_reg);                        /*810*/
DEF_INST(convert_u32_to_dfp_ext_reg);                           /*810*/
DEF_INST(convert_u32_to_dfp_long_reg);                          /*810*/
DEF_INST(convert_u64_to_dfp_ext_reg);                           /*810*/
DEF_INST(convert_u64_to_dfp_long_reg);                          /*810*/
DEF_INST(convert_dfp_ext_to_fix32_reg);                         /*810*/
DEF_INST(convert_dfp_long_to_fix32_reg);                        /*810*/
DEF_INST(convert_dfp_ext_to_u32_reg);                           /*810*/
DEF_INST(convert_dfp_long_to_u32_reg);                          /*810*/
DEF_INST(convert_dfp_ext_to_u64_reg);                           /*810*/
DEF_INST(convert_dfp_long_to_u64_reg);                          /*810*/
DEF_INST(convert_u32_to_bfp_ext_reg);                           /*810*/
DEF_INST(convert_u32_to_bfp_long_reg);                          /*810*/
DEF_INST(convert_u32_to_bfp_short_reg);                         /*810*/
DEF_INST(convert_u64_to_bfp_ext_reg);                           /*810*/
DEF_INST(convert_u64_to_bfp_long_reg);                          /*810*/
DEF_INST(convert_u64_to_bfp_short_reg);                         /*810*/
DEF_INST(convert_bfp_ext_to_u32_reg);                           /*810*/
DEF_INST(convert_bfp_long_to_u32_reg);                          /*810*/
DEF_INST(convert_bfp_short_to_u32_reg);                         /*810*/
DEF_INST(convert_bfp_ext_to_u64_reg);                           /*810*/
DEF_INST(convert_bfp_long_to_u64_reg);                          /*810*/
DEF_INST(convert_bfp_short_to_u64_reg);                         /*810*/
#endif /*defined(FEATURE_FLOATING_POINT_EXTENSION_FACILITY)*/

/* Instructions in dfp.c */
#if defined(FEATURE_FPS_ENHANCEMENT)
DEF_INST(copy_sign_fpr_long_reg);
DEF_INST(load_complement_fpr_long_reg);
DEF_INST(load_fpr_from_gr_long_reg);
DEF_INST(load_gr_from_fpr_long_reg);
DEF_INST(load_negative_fpr_long_reg);
DEF_INST(load_positive_fpr_long_reg);
DEF_INST(set_dfp_rounding_mode);
#endif /*defined(FEATURE_FPS_ENHANCEMENT)*/

#if defined(FEATURE_IEEE_EXCEPTION_SIMULATION)
DEF_INST(load_fpc_and_signal);
DEF_INST(set_fpc_and_signal);
#endif /*defined(FEATURE_IEEE_EXCEPTION_SIMULATION)*/

#if defined(FEATURE_DECIMAL_FLOATING_POINT)
DEF_INST(add_dfp_ext_reg);
DEF_INST(add_dfp_long_reg);
DEF_INST(compare_dfp_ext_reg);
DEF_INST(compare_dfp_long_reg);
DEF_INST(compare_and_signal_dfp_ext_reg);
DEF_INST(compare_and_signal_dfp_long_reg);
DEF_INST(compare_exponent_dfp_ext_reg);
DEF_INST(compare_exponent_dfp_long_reg);
DEF_INST(convert_fix64_to_dfp_ext_reg);
DEF_INST(convert_fix64_to_dfp_long_reg);
DEF_INST(convert_sbcd128_to_dfp_ext_reg);
DEF_INST(convert_sbcd64_to_dfp_long_reg);
DEF_INST(convert_ubcd128_to_dfp_ext_reg);
DEF_INST(convert_ubcd64_to_dfp_long_reg);
DEF_INST(convert_dfp_ext_to_fix64_reg);
DEF_INST(convert_dfp_long_to_fix64_reg);
DEF_INST(convert_dfp_ext_to_sbcd128_reg);
DEF_INST(convert_dfp_long_to_sbcd64_reg);
DEF_INST(convert_dfp_ext_to_ubcd128_reg);
DEF_INST(convert_dfp_long_to_ubcd64_reg);
DEF_INST(divide_dfp_ext_reg);
DEF_INST(divide_dfp_long_reg);
DEF_INST(extract_biased_exponent_dfp_ext_to_fix64_reg);
DEF_INST(extract_biased_exponent_dfp_long_to_fix64_reg);
DEF_INST(extract_significance_dfp_ext_reg);
DEF_INST(extract_significance_dfp_long_reg);
DEF_INST(insert_biased_exponent_fix64_to_dfp_ext_reg);
DEF_INST(insert_biased_exponent_fix64_to_dfp_long_reg);
DEF_INST(load_and_test_dfp_ext_reg);
DEF_INST(load_and_test_dfp_long_reg);
DEF_INST(load_fp_int_dfp_ext_reg);
DEF_INST(load_fp_int_dfp_long_reg);
DEF_INST(load_lengthened_dfp_long_to_ext_reg);
DEF_INST(load_lengthened_dfp_short_to_long_reg);
DEF_INST(load_rounded_dfp_ext_to_long_reg);
DEF_INST(load_rounded_dfp_long_to_short_reg);
DEF_INST(multiply_dfp_ext_reg);
DEF_INST(multiply_dfp_long_reg);
DEF_INST(quantize_dfp_ext_reg);
DEF_INST(quantize_dfp_long_reg);
DEF_INST(reround_dfp_ext_reg);
DEF_INST(reround_dfp_long_reg);
DEF_INST(shift_coefficient_left_dfp_ext);
DEF_INST(shift_coefficient_left_dfp_long);
DEF_INST(shift_coefficient_right_dfp_ext);
DEF_INST(shift_coefficient_right_dfp_long);
DEF_INST(subtract_dfp_ext_reg);
DEF_INST(subtract_dfp_long_reg);
DEF_INST(test_data_class_dfp_ext);
DEF_INST(test_data_class_dfp_long);
DEF_INST(test_data_class_dfp_short);
DEF_INST(test_data_group_dfp_ext);
DEF_INST(test_data_group_dfp_long);
DEF_INST(test_data_group_dfp_short);
#endif /*defined(FEATURE_DECIMAL_FLOATING_POINT)*/

#if defined(FEATURE_DFP_ZONED_CONVERSION_FACILITY)
DEF_INST(convert_zoned_to_dfp_ext);                             /*912*/
DEF_INST(convert_zoned_to_dfp_long);                            /*912*/
DEF_INST(convert_dfp_ext_to_zoned);                             /*912*/
DEF_INST(convert_dfp_long_to_zoned);                            /*912*/
#endif /*defined(FEATURE_DFP_ZONED_CONVERSION_FACILITY)*/

/* Instructions in pfpo.c */
#if defined(FEATURE_PFPO)
DEF_INST(perform_floating_point_operation);
#endif /*defined(FEATURE_PFPO)*/

/* end of OPCODE.H */
