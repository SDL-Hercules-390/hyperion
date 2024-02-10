/* OPCODE.H     (C) Copyright Jan Jaeger, 2000-2012                  */
/*              (C) and others 2013-2023                             */
/*              Instruction decoding macros and prototypes           */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/* Interpretive Execution - (C) Copyright Jan Jaeger, 1999-2012      */
/* z/Architecture support - (C) Copyright Jan Jaeger, 1999-2012      */

#ifndef _OPCODE_H
#define _OPCODE_H

/*-------------------------------------------------------------------*/
/*          (delineates ARCH_DEP from non-arch_dep)                  */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/*               Architecture *INDEPENDENT* macros                   */
/*-------------------------------------------------------------------*/
/*  The following macros are defined ONE TIME (due to the above      */
/*  "#ifndef _OPCODE_H" guard) and thus are the same for ALL         */
/*   build architectures.                                            */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* helper macros to define an opcode table instruction function name */
/*-------------------------------------------------------------------*/

#if defined( _370 )
 #define  _GEN370( _ifunc_name )      &s370_ ## _ifunc_name,
#else
 #define  _GEN370( _ifunc_name )
#endif

#if defined( _390 )
 #define  _GEN390( _ifunc_name )      &s390_ ## _ifunc_name,
#else
 #define  _GEN390( _ifunc_name )
#endif

#if defined( _900 )
 #define  _GEN900( _ifunc_name )      &z900_ ## _ifunc_name,
#else
 #define  _GEN900( _ifunc_name )
#endif

/*-------------------------------------------------------------------*/
/*            Macros for defining opcode table entries               */
/*-------------------------------------------------------------------*/
/*                                                                   */
/* The below GENx...macros are used to define the NON-archdep master */
/* opcode table entries in opcode.c. Each entry defines a separate   */
/* pointer to an architecture DEPENDENT instruction function for all */
/* three of our supported build architectures, as well as a common   */
/* instruction tracing function (based on the instruction's format)  */
/* and a string used during tracing containing the instruction's     */
/* mnemonic and instruction function's name, because each opcode is  */
/* expected to define the same instruction for each architecture if  */
/* that opcode is defined in the given architecture.                 */
/*                                                                   */
/* That is to say, using the below macros, it is NOT possible to de- */
/* fine a table entry for a given opcode for an instruction that is  */
/* completely different in one architecture than it is in the other  */
/* architectures. Opcode 'D2' for example, is expected to be the op- */
/* code for the "MVC" instruction in all three architectures.        */
/*                                                                   */
/* With later versions of z/Architecture however, this is not neces- */
/* sarily always true. In later versions of z/Architecture, IBM has  */
/* begun re-using opcodes for older S/370-only instructions. In such */
/* type of situations you need to define an architecture DEPENDENT   */
/* opcode table instead, using the new 'AD_GENx...' macros defined   */
/* further below in the architecture-DEPENDENT section of opcode.h   */
/* (which immediately follows the "#endif" for _OPCODE_H).           */
/*                                                                   */
/*-------------------------------------------------------------------*/
/*                                                                   */
/*                      PROGRAMMING NOTE                             */
/*                                                                   */
/* PROGRAMMING NOTE: the '_ifmt' argument in the below "GENx" macros */
/* is currently ignored since it is not being used for anything at   */
/* the moment. At some point in the near future however, if things   */
/* work out, it will actually be used as a function call to decode   */
/* the instruction before being dispatched to the actual function    */
/* that executes the instruction, relieving each instruction from    */
/* having to decode the instruction itself each time (as well as     */
/* relieving the instruction 'iprint' (tracing) functions from also  */
/* having to decode the instruction too!) After all, if there are    */
/* 57 instructions defined that use the 'RR' format and 220 defined  */
/* that use the 'RRE' format, etc, why should they each have to do   */
/* the same thing themselves each time? There needs to be a common   */
/* instruction format decoding function that is called before each   */
/* instruction function is ever reached so that all the instruction  */
/* itself has to do is whatever its purpose is. After all, decoding  */
/* an instruction is LOGICALLY part of the instruction decoding and  */
/* dispatching logic, NOT something that each instruction (or each   */
/* 'iprint' tracing function!) should be doing themselves. This is   */
/* what I intend to (hope to) fix at some point in the near future.  */
/*                                                                   */
/*-------------------------------------------------------------------*/

#define GENx___x___x___                                     \
    {                                                       \
                &operation_exception,                       \
                &operation_exception,                       \
                &operation_exception,                       \
        (void*) &iprint_ASMFMT_none,                        \
        (void*) &"?????" "\0" "?"                           \
    }

#define GENx370x___x___( _mnemonic, _ifmt, _asmfmt, _ifunc_name )  \
    {                                                       \
        _GEN370( _ifunc_name )                              \
                &operation_exception,                       \
                &operation_exception,                       \
        (void*) &iprint_ ## _asmfmt,                        \
        (void*) & _mnemonic "\0" #_ifunc_name               \
    }

#define GENx___x390x___( _mnemonic, _ifmt, _asmfmt, _ifunc_name )  \
    {                                                       \
                &operation_exception,                       \
        _GEN390( _ifunc_name )                              \
                &operation_exception,                       \
        (void*) &iprint_ ## _asmfmt,                        \
        (void*) & _mnemonic "\0" #_ifunc_name               \
    }

#define GENx370x390x___( _mnemonic, _ifmt, _asmfmt, _ifunc_name )  \
    {                                                       \
        _GEN370( _ifunc_name )                              \
        _GEN390( _ifunc_name )                              \
                &operation_exception,                       \
        (void*) &iprint_ ## _asmfmt,                        \
        (void*) & _mnemonic "\0" #_ifunc_name               \
    }

#define GENx___x___x900( _mnemonic, _ifmt, _asmfmt, _ifunc_name )  \
    {                                                       \
                &operation_exception,                       \
                &operation_exception,                       \
        _GEN900( _ifunc_name )                              \
        (void*) &iprint_ ## _asmfmt,                        \
        (void*) & _mnemonic "\0" #_ifunc_name               \
    }

#define GENx370x___x900( _mnemonic, _ifmt, _asmfmt, _ifunc_name )  \
    {                                                       \
        _GEN370( _ifunc_name )                              \
                &operation_exception,                       \
        _GEN900( _ifunc_name )                              \
        (void*) &iprint_ ## _asmfmt,                        \
        (void*) & _mnemonic "\0" #_ifunc_name               \
    }

#define GENx___x390x900( _mnemonic, _ifmt, _asmfmt, _ifunc_name )  \
    {                                                       \
                &operation_exception,                       \
        _GEN390( _ifunc_name )                              \
        _GEN900( _ifunc_name )                              \
        (void*) &iprint_ ## _asmfmt,                        \
        (void*) & _mnemonic "\0" #_ifunc_name               \
    }

#define GENx370x390x900( _mnemonic, _ifmt, _asmfmt, _ifunc_name )  \
    {                                                       \
        _GEN370( _ifunc_name )                              \
        _GEN390( _ifunc_name )                              \
        _GEN900( _ifunc_name )                              \
        (void*) &iprint_ ## _asmfmt,                        \
        (void*) & _mnemonic "\0" #_ifunc_name               \
    }

/*-------------------------------------------------------------------*/
/*                       PROGRAMMING NOTE                            */
/*-------------------------------------------------------------------*/
/* The following set of macros identifies those instructions which   */
/* are a part of the FEATURE_370_EXTENSION backport of some S/390    */
/* and z/Architecture instructions to System/370 mode (refer to the  */
/* the feat370.h header).  All instructions so identified will have  */
/* their instruction generated also for the S/370 architecture mode  */
/* as well, even though such instruction are not formally a part of  */
/* the System/370 architecture.                                      */
/*                                                                   */
/* Whether or not such instructions cause a Program Check Operation  */
/* Exception to occur when executed in S/370 mode is controlled via  */
/* enabling or disabling the 'HERC_370_EXTENSION' archlvl facility.  */
/*                                                                   */
/* When the facility is disabled (default), all such instructions    */
/* will properly Program Check (Operation Exception) when attempted  */
/* to be executed in S/370 mode.  When the facility enabled however, */
/* then all such "37X" instructions are instead allowed to execute.  */
/*-------------------------------------------------------------------*/

#define    GENx37Xx390x___     GENx370x390x___
#define    GENx37Xx___x900     GENx370x___x900
#define    GENx37Xx390x900     GENx370x390x900

#define AD_GENx37Xx390x___  AD_GENx370x390x___
#define AD_GENx37Xx___x900  AD_GENx370x___x900
#define AD_GENx37Xx390x900  AD_GENx370x390x900

/*-------------------------------------------------------------------*/

#define ILC(_b) ((_b) < 0x40 ? 2 : (_b) < 0xc0 ? 4 : 6)

#define REAL_ILC(_regs) \
 (likely(!(_regs)->execflag) ? (_regs)->psw.ilc : (_regs)->exrl ? 6 : 4)

#define ILC_FROM_PIID( piid )    (((piid) & 0x00070000) >> 16)
#define CODE_FROM_PIID( piid )   (((piid) & 0x0000FFFF)      )

/*-------------------------------------------------------------------*/
/*  Instruction tracing helper function to print the instruction     */
/*-------------------------------------------------------------------*/

#define PRINT_INST( _arch_mode, _inst, _prtbuf )     \
                                                     \
           iprint_router_func( (_arch_mode), (_inst), 0, (_prtbuf) )

OPCD_DLL_IMPORT int iprint_router_func( int arch_mode, BYTE inst[], char mnemonic[], char* prtbuf );

/*-------------------------------------------------------------------*/
/*               Individual instruction counting                     */
/*-------------------------------------------------------------------*/

#if defined( OPTION_INSTR_COUNT_AND_TIME )

#define BEG_COUNT_INSTR( _inst, _regs )                             \
    do                                                              \
    {                                                               \
        if (sysblk.icount)                                          \
        {                                                           \
            U64 used;                                               \
            gettimeofday(&sysblk.start_time, NULL);                 \
            switch ((_inst)[0]) {                                   \
            case 0x01:                                              \
                used = sysblk.imaps.imap01[(_inst)[1]]++;           \
                break;                                              \
            case 0xA4:                                              \
                used = sysblk.imaps.imapa4[(_inst)[1]]++;           \
                break;                                              \
            case 0xA5:                                              \
                used = sysblk.imaps.imapa5[(_inst)[1] & 0x0F]++;    \
                break;                                              \
            case 0xA6:                                              \
                used = sysblk.imaps.imapa6[(_inst)[1]]++;           \
                break;                                              \
            case 0xA7:                                              \
                used = sysblk.imaps.imapa7[(_inst)[1] & 0x0F]++;    \
                break;                                              \
            case 0xB2:                                              \
                used = sysblk.imaps.imapb2[(_inst)[1]]++;           \
                break;                                              \
            case 0xB3:                                              \
                used = sysblk.imaps.imapb3[(_inst)[1]]++;           \
                break;                                              \
            case 0xB9:                                              \
                used = sysblk.imaps.imapb9[(_inst)[1]]++;           \
                break;                                              \
            case 0xC0:                                              \
                used = sysblk.imaps.imapc0[(_inst)[1] & 0x0F]++;    \
                break;                                              \
            case 0xC2:                                              \
                used = sysblk.imaps.imapc2[(_inst)[1] & 0x0F]++;    \
                break;                                              \
            case 0xC4:                                              \
                used = sysblk.imaps.imapc4[(_inst)[1] & 0x0F]++;    \
                break;                                              \
            case 0xC6:                                              \
                used = sysblk.imaps.imapc6[(_inst)[1] & 0x0F]++;    \
                break;                                              \
            case 0xC8:                                              \
                used = sysblk.imaps.imapc8[(_inst)[1] & 0x0F]++;    \
                break;                                              \
            case 0xE3:                                              \
                used = sysblk.imaps.imape3[(_inst)[5]]++;           \
                break;                                              \
            case 0xE4:                                              \
                used = sysblk.imaps.imape4[(_inst)[1]]++;           \
                break;                                              \
            case 0xE5:                                              \
                used = sysblk.imaps.imape5[(_inst)[1]]++;           \
                break;                                              \
            case 0xE7:                                              \
                used = sysblk.imaps.imape7[(_inst)[5]]++;           \
                break;                                              \
            case 0xEB:                                              \
                used = sysblk.imaps.imapeb[(_inst)[5]]++;           \
                break;                                              \
            case 0xEC:                                              \
                used = sysblk.imaps.imapec[(_inst)[5]]++;           \
                break;                                              \
            case 0xED:                                              \
                used = sysblk.imaps.imaped[(_inst)[5]]++;           \
                break;                                              \
            default:                                                \
                used = sysblk.imaps.imapxx[(_inst)[0]]++;           \
            }                                                       \
                                                                    \
            if (!used)                                              \
            {                                                       \
                /* "%s" */                                          \
                WRMSG( HHC02292, "I", "First use" );                \
                ARCH_DEP( display_inst )( (_regs), (_inst) );       \
            }                                                       \
        }                                                           \
    } while (0)

#else // !defined( OPTION_INSTR_COUNT_AND_TIME )

#define BEG_COUNT_INSTR(_inst, _regs)

#endif // defined( OPTION_INSTR_COUNT_AND_TIME )

#if defined( OPTION_INSTR_COUNT_AND_TIME )

#define END_COUNT_INSTR(_inst, _regs)                               \
    do                                                              \
    {                                                               \
        if (sysblk.icount)                                          \
        {                                                           \
            struct timeval end_time;                                \
            struct timeval dur;                                     \
            U64 elapsed_usecs;                                      \
                                                                    \
            gettimeofday(&end_time, NULL);                          \
            timeval_subtract(&sysblk.start_time, &end_time, &dur);  \
            elapsed_usecs = (dur.tv_sec * 1000000) + dur.tv_usec;   \
                                                                    \
            switch ((_inst)[0]) {                                   \
            case 0x01:                                              \
                sysblk.imaps.imap01T[(_inst)[1]]+=elapsed_usecs;    \
                break;                                              \
            case 0xA4:                                              \
                sysblk.imaps.imapa4T[(_inst)[1]]+=elapsed_usecs;    \
                break;                                              \
            case 0xA5:                                              \
                sysblk.imaps.imapa5T[(_inst)[1] & 0x0F]+=elapsed_usecs;   \
                break;                                              \
            case 0xA6:                                              \
                sysblk.imaps.imapa6T[(_inst)[1]]+=elapsed_usecs;    \
                break;                                              \
            case 0xA7:                                              \
                sysblk.imaps.imapa7T[(_inst)[1] & 0x0F]+=elapsed_usecs;   \
                break;                                              \
            case 0xB2:                                              \
                sysblk.imaps.imapb2T[(_inst)[1]]+=elapsed_usecs;    \
                break;                                              \
            case 0xB3:                                              \
                sysblk.imaps.imapb3T[(_inst)[1]]+=elapsed_usecs;    \
                break;                                              \
            case 0xB9:                                              \
                sysblk.imaps.imapb9T[(_inst)[1]]+=elapsed_usecs;    \
                break;                                              \
            case 0xC0:                                              \
                sysblk.imaps.imapc0T[(_inst)[1] & 0x0F]+=elapsed_usecs;   \
                break;                                              \
            case 0xC2:                                              \
                sysblk.imaps.imapc2T[(_inst)[1] & 0x0F]+=elapsed_usecs;   \
                break;                                              \
            case 0xC4:                                              \
                sysblk.imaps.imapc4T[(_inst)[1] & 0x0F]+=elapsed_usecs;   \
                break;                                              \
            case 0xC6:                                              \
                sysblk.imaps.imapc6T[(_inst)[1] & 0x0F]+=elapsed_usecs;   \
                break;                                              \
            case 0xC8:                                              \
                sysblk.imaps.imapc8T[(_inst)[1] & 0x0F]+=elapsed_usecs;   \
                break;                                              \
            case 0xE3:                                              \
                sysblk.imaps.imape3T[(_inst)[5]]+=elapsed_usecs;    \
                break;                                              \
            case 0xE4:                                              \
                sysblk.imaps.imape4T[(_inst)[1]]+=elapsed_usecs;    \
                break;                                              \
            case 0xE5:                                              \
                sysblk.imaps.imape5T[(_inst)[1]]+=elapsed_usecs;    \
                break;                                              \
            case 0xE7:                                              \
                sysblk.imaps.imape7T[(_inst)[5]]+=elapsed_usecs;    \
                break;                                              \
            case 0xEB:                                              \
                sysblk.imaps.imapebT[(_inst)[5]]+=elapsed_usecs;    \
                break;                                              \
            case 0xEC:                                              \
                sysblk.imaps.imapecT[(_inst)[5]]+=elapsed_usecs;    \
                break;                                              \
            case 0xED:                                              \
                sysblk.imaps.imapedT[(_inst)[5]]+=elapsed_usecs;    \
                break;                                              \
            default:                                                \
                sysblk.imaps.imapxxT[(_inst)[0]]+=elapsed_usecs;    \
            }                                                       \
        }                                                           \
    } while (0)


#else // !defined( OPTION_INSTR_COUNT_AND_TIME )
#define END_COUNT_INSTR(_inst, _regs)
#endif // defined( OPTION_INSTR_COUNT_AND_TIME )





/*-------------------------------------------------------------------*/
/*                         SIE macros                                */
/*                 (architecture INDEPENDENT)                        */
/*-------------------------------------------------------------------*/

#if defined( _FEATURE_SIE )

  #define SIE_MODE( _regs )         ((_regs)->sie_mode)
  #define SIE_STATE( _regs )        ((_regs)->sie_state)

  #define SIE_FEAT_BIT_ON(   _regs, _byte, _bit )   ((_regs)->siebk->SIE_ ## _byte & SIE_ ## _byte ## _ ## _bit)
  #define SIE_EC_BIT_ON(     _regs, _byte, _bit )   ((_regs)->siebk->SIE_ ## _byte & SIE_ ## _byte ## _ ## _bit)

  #define SIE_FEAT_BIT_OFF(  _regs, _byte, _bit )   !SIE_FEAT_BIT_ON( _regs, _byte, _bit )
  #define SIE_EC_BIT_OFF(    _regs, _byte, _bit )   !SIE_EC_BIT_ON(   _regs, _byte, _bit )

  #define SIE_STATE_BIT_ON(  _regs, _byte, _bit )   (SIE_MODE((_regs)) && SIE_FEAT_BIT_ON(  (_regs), _byte, _bit ))
  #define SIE_STATE_BIT_OFF( _regs, _byte, _bit )   (SIE_MODE((_regs)) && SIE_FEAT_BIT_OFF( (_regs), _byte, _bit ))


  #define TXF_SIE_INTERCEPT( _regs, _name )                     \
    do                                                          \
    {                                                           \
        /* Only allow direct execution of TXF instructions      \
           if the z/VM host says to allow it. Otherwise let     \
           the z/VM host intercept this instruction so it       \
           can simulate it, throw a PIC001, or at least be      \
           informed that we are executing this instruction.     \
        */                                                      \
        if (1                                                   \
            && SIE_MODE( (_regs) )                              \
            && SIE_EC_BIT_OFF( (_regs), ECB0, TXF )             \
        )                                                       \
        {                                                       \
            if (TXF_TRACING())                                  \
            {                                                   \
                /* "TXF: %s%02X: SIE: Intercepting              \
                         %s instruction" */                     \
                WRMSG( HHC17715, "D",                           \
                    TXF_CPUAD( _regs ), #_name );               \
            }                                                   \
            longjmp( (_regs)->progjmp, SIE_INTERCEPT_INST );    \
        }                                                       \
    }                                                           \
    while (0)

#else // !defined( _FEATURE_SIE )

  #define SIE_MODE(          _regs )                (0)
  #define SIE_STATE(         _regs )                (0)

  #define SIE_FEAT_BIT_ON(   _regs, _byte, _bit )   (0)
  #define SIE_EC_BIT_ON(     _regs, _byte, _bit )   (0)
  #define SIE_STATE_BIT_ON(  _regs, _byte, _bit )   (0)

  #define SIE_FEAT_BIT_OFF(  _regs, _byte, _bit )   (1)
  #define SIE_EC_BIT_OFF(    _regs, _byte, _bit )   (1)
  #define SIE_STATE_BIT_OFF( _regs, _byte, _bit )   (1)

  #define TXF_SIE_INTERCEPT( _regs, _name )  /* (do nothing) */

#endif // defined( _FEATURE_SIE )

#if defined( _FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE )
  #undef              MULTIPLE_CONTROLLED_DATA_SPACE
  #define             MULTIPLE_CONTROLLED_DATA_SPACE( _regs )   \
      (SIE_FEAT_BIT_ON( (_regs), MX, XC ) && AR_BIT( &(_regs)->psw ))
#else
  #undef  MULTIPLE_CONTROLLED_DATA_SPACE
  #define MULTIPLE_CONTROLLED_DATA_SPACE( _regs )   (0)
#endif

#if defined( FEATURE_SIE )
  #undef  SIE_ACTIVE
  #define SIE_ACTIVE( _regs )     ((_regs)->sie_active)
#else
  #undef  SIE_ACTIVE
  #define SIE_ACTIVE( _regs )     (0)
#endif

/*-------------------------------------------------------------------*/
/*                   Instruction "FOOTPRINT"                         */
/*-------------------------------------------------------------------*/
/* The footprint_buffer option saves a copy of the register context  */
/* every time an instruction is executed.  This is for problem       */
/* determination only, as it SEVERELY impacts performance.      *JJ  */
/*-------------------------------------------------------------------*/

#if defined( OPTION_FOOTPRINT_BUFFER )
#define FOOTPRINT(_ip, _regs) \
do { \
    sysblk.footprregs[(_regs)->cpuad][sysblk.footprptr[(_regs)->cpuad]] = *(_regs); \
    memcpy(&sysblk.footprregs[(_regs)->cpuad][sysblk.footprptr[(_regs)->cpuad]++].inst,(_ip),6); \
    sysblk.footprptr[(_regs)->cpuad] &= OPTION_FOOTPRINT_BUFFER - 1; \
} while(0)
#endif

#if !defined( FOOTPRINT )
#define FOOTPRINT(_ip, _regs)
#endif

/*-------------------------------------------------------------------*/
/*                  CPU Stepping or Tracing                          */
/*-------------------------------------------------------------------*/

#define TXF_INSTR_TRACING()                                           \
  (sysblk.txf_tracing & TXF_TR_INSTR)


#define TXF_CONSTRAINED_TRANS_INSTR( _regs )                          \
  ((sysblk.txf_tracing & TXF_TR_C)                                    \
    && (_regs)->txf_tnd && (_regs)->txf_contran)


#define TXF_UNCONSTRAINED_TRANS_INSTR( _regs )                        \
  ((sysblk.txf_tracing & TXF_TR_U)                                    \
    && (_regs)->txf_tnd && !(_regs)->txf_contran)


#define TXF_TRACE_THIS_INSTR( _regs )                                 \
  (1                                                                  \
   && TXF_TRACE_CPU( _regs )                                          \
   && TXF_TRACE_TND( _regs )                                          \
   && (0                                                              \
       || TXF_CONSTRAINED_TRANS_INSTR( _regs )                        \
       || TXF_UNCONSTRAINED_TRANS_INSTR( _regs )                      \
      )                                                               \
  )


#define _CPU_STEP_OR_TRACE(_breakaddr_or_traceaddr, _regs, _ilc)      \
   (0                                                                 \
       || !TXF_INSTR_TRACING()                                        \
       ||  TXF_TRACE_THIS_INSTR( _regs )                              \
   )                                                                  \
   &&                                                                 \
   (                                                                  \
        (sysblk._breakaddr_or_traceaddr[0] == 0 &&                    \
         sysblk._breakaddr_or_traceaddr[1] == 0)                      \
                                                                      \
     || (sysblk._breakaddr_or_traceaddr[0] <=                         \
         sysblk._breakaddr_or_traceaddr[1]                            \
                                                                      \
         && PSW_IA_FROM_IP((_regs), -(_ilc)) >=                       \
            sysblk._breakaddr_or_traceaddr[0]                         \
         && PSW_IA_FROM_IP((_regs), -(_ilc)) <=                       \
            sysblk._breakaddr_or_traceaddr[1]                         \
        )                                                             \
                                                                      \
     || (sysblk._breakaddr_or_traceaddr[0] >                          \
         sysblk._breakaddr_or_traceaddr[1]                            \
                                                                      \
         && PSW_IA_FROM_IP((_regs), -(_ilc)) >=                       \
            sysblk._breakaddr_or_traceaddr[1]                         \
         && PSW_IA_FROM_IP((_regs), -(_ilc)) <=                       \
            sysblk._breakaddr_or_traceaddr[0]                         \
        )                                                             \
   )                                                                  \


#define CPU_STEPPING(_regs, _ilc)                                     \
  (sysblk.instbreak  && _CPU_STEP_OR_TRACE(breakaddr,(_regs),(_ilc)))


#define CPU_TRACING(_regs, _ilc)                                      \
  (1                                                                  \
   && sysblk.insttrace                                                \
   && (_regs)->insttrace                                              \
   && _CPU_STEP_OR_TRACE( traceaddr, (_regs), (_ilc) )                \
  )


#define CPU_STEPPING_OR_TRACING(_regs, _ilc)                          \
  ( unlikely((_regs)->breakortrace) &&                                \
    (CPU_STEPPING((_regs), (_ilc)) || CPU_TRACING((_regs), (_ilc)))   \
  )


#define _CPU_TRACE_ALL                                                \
   (sysblk.traceaddr[0] == 0 &&                                       \
    sysblk.traceaddr[1] == 0 &&                                       \
  /*sysblk.insttrace*/ insttrace_all())


#define _CPU_STEP_ALL                                                 \
  (sysblk.breakaddr[0] == 0 &&                                        \
   sysblk.breakaddr[1] == 0 &&                                        \
   sysblk.instbreak)


#define CPU_STEPPING_OR_TRACING_ALL  (_CPU_STEP_ALL || _CPU_TRACE_ALL)


#define PROCESS_TRACE( _regs, _ip, _goto )                            \
  do                                                                  \
  {                                                                   \
    /* If stepping or tracing, trace this instruction */              \
    if ((_regs)->breakortrace)                                        \
    {                                                                 \
      ARCH_DEP( process_trace )( (_regs), (_ip) );                    \
                                                                      \
      /* If the aie was invalidated, re-fetch the instruction.        \
         Another CPU executing e.g. a IPTE instruction during         \
         instruction stepping while process_trace was waiting         \
         for the user to press the enter key can allow this to        \
         occur. Otherwise it is impossible to occur. */               \
      if (1                                                           \
          && (_regs)->stepping                                        \
          && !VALID_AIE( _regs )                                      \
      )                                                               \
      {                                                               \
        /* "Processor %s%02X: aie invalidated; instruction refetched" */    \
        WRMSG( HHC00835, "W", PTYPSTR( (_regs)->cpuad ), (_regs)->cpuad );  \
        goto _goto;                                                   \
      }                                                               \
    }                                                                 \
  }                                                                   \
  while (0)

/*-------------------------------------------------------------------*/
/*         Simple helper macro that instructions can use             */
/*         to force an immediate check for interrupts.               */
/*-------------------------------------------------------------------*/

#define RETURN_INTCHECK(_regs)                                        \
        longjmp((_regs)->progjmp, SIE_NO_INTERCEPT)

/*-------------------------------------------------------------------*/
/*                Instruction validity checking                      */
/*-------------------------------------------------------------------*/

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

#if defined( _FEATURE_S370_S390_VECTOR_FACILITY )

#ifndef _VFDEFS
#define _VFDEFS
#define VOP_CHECK( _regs )              if (!((_regs)->CR(0) & CR0_VOP) || !(_regs)->vf->online) \
                                            (_regs)->program_interrupt( (_regs), PGM_VECTOR_OPERATION_EXCEPTION )
#define VR_INUSE( _vr, _regs )          ((_regs)->vf->vsr &   (VSR_VIU0 >> ((_vr) >> 1)))
#define VR_CHANGED( _vr, _regs )        ((_regs)->vf->vsr &   (VSR_VCH0 >> ((_vr) >> 1)))
#define SET_VR_INUSE( _vr, _regs )       (_regs)->vf->vsr |=  (VSR_VIU0 >> ((_vr) >> 1))
#define SET_VR_CHANGED( _vr, _regs )     (_regs)->vf->vsr |=  (VSR_VCH0 >> ((_vr) >> 1))
#define RESET_VR_INUSE( _vr, _regs )     (_regs)->vf->vsr &= ~(VSR_VIU0 >> ((_vr) >> 1))
#define RESET_VR_CHANGED( _vr, _regs )   (_regs)->vf->vsr &= ~(VSR_VCH0 >> ((_vr) >> 1))
#define VMR_SET( _section, _regs )      ((_regs)->vf->vmr[(_section) >> 3] & (0x80 >> ((_section) & 7)))
#define MASK_MODE( _regs )              ((_regs)->vf->vsr & VSR_M)
#define VECTOR_COUNT( _regs )           (((_regs)->vf->vsr & VSR_VCT) >> 32)
#define VECTOR_IX( _regs )              (((_regs)->vf->vsr & VSR_VIX) >> 16)
#endif /* _VFDEFS */
#endif /* defined( _FEATURE_S370_S390_VECTOR_FACILITY ) */

/*-------------------------------------------------------------------*/
/*              Device  IOID / SSID / LCSS  macros                   */
/*-------------------------------------------------------------------*/

#define IOID_TO_SSID( _ioid )       ((_ioid) >> 16)
#define IOID_TO_LCSS( _ioid )       ((_ioid) >> 17)
#define SSID_TO_LCSS( _ssid )       ((_ssid) >> 1 )
#define LCSS_TO_SSID( _lcss )       (((_lcss) << 1 ) | 1)

/*-------------------------------------------------------------------*/
/*            Virtual Architecture Level Set Facility                */
/*-------------------------------------------------------------------*/

#define FACILITY_ENABLED(_faci, _regs) \
        (((_regs)->facility_list                  [ ((STFL_ ## _faci)/8) ]) & (0x80 >> ((STFL_ ## _faci) % 8)))

#define FACILITY_ENABLED_DEV(_faci) \
        ((sysblk.facility_list[ sysblk.arch_mode ][ ((STFL_ ## _faci)/8) ]) & (0x80 >> ((STFL_ ## _faci) % 8)))

#define FACILITY_ENABLED_ARCH( _faci, _arch ) \
        ((sysblk.facility_list[     (_arch)      ][ ((STFL_ ## _faci)/8) ]) & (0x80 >> ((STFL_ ## _faci) % 8)))

#define FACILITY_CHECK(_faci, _regs) \
    do { \
        if(!FACILITY_ENABLED( _faci, _regs ) ) \
          (_regs)->program_interrupt( (_regs), PGM_OPERATION_EXCEPTION); \
    } while (0)

/*-------------------------------------------------------------------*/
/*                     PER range checking                            */
/*-------------------------------------------------------------------*/

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

/*-------------------------------------------------------------------*/
/*                   Byte swapping macros                            */
/*-------------------------------------------------------------------*/
/* The "CSWAPxx()" macros CONDITIONALLY swap the endianness of the   */
/* given argument depending on the endianness of the current host,   */
/* much like the "htonl()" networking API functions. If this build   */
/* of Hercules is for running on a big endian host, then CSWAPxx()   */
/* will do absolutely nothing since the argument should already be   */
/* in big endian format. If this build of Hercules is for running    */
/* on a little endian host however, it will perform the byte swap    */
/* so that the result is a big endian value (since z/Architecture    */
/* is big endian).                                                   */
/*                                                                   */
/* The SWAPxx() macros however, UNCONDITIONALLY swap the endianness  */
/* of the specified value REGARDLESS of the endianness Hercules was  */
/* built for or the endianness of the host it is running on. It is   */
/* designed for situations such as what might exist when a number    */
/* is read or written to/from disk in a format different from the    */
/* format of the Hercules build or the host it is running on (such   */
/* as what occurs with Hercules's emulated dasd files). In such a    */
/* situation the device driver detects the endianness of the system  */
/* it is running on differs from the endianness that the DASD file   */
/* was written in, thereby requiring it to *UNCONDITIONALLY* swap    */
/* the value that was read from disk, REGARDLESS of the endianness   */
/* of the Hercules build or the host it is currently running on.     */
/*-------------------------------------------------------------------*/

#ifdef WORDS_BIGENDIAN
 #define CSWAP16(_x)    (_x)            // (result ALWAYS big endian)
 #define CSWAP32(_x)    (_x)            // (result ALWAYS big endian)
 #define CSWAP64(_x)    (_x)            // (result ALWAYS big endian)
#else
 #define CSWAP16(_x)    bswap_16(_x)    // (result ALWAYS big endian)
 #define CSWAP32(_x)    bswap_32(_x)    // (result ALWAYS big endian)
 #define CSWAP64(_x)    bswap_64(_x)    // (result ALWAYS big endian)
#endif

 #define SWAP16(_x)     bswap_16(_x)    // (result OPPOSITE of input)
 #define SWAP32(_x)     bswap_32(_x)    // (result OPPOSITE of input)
 #define SWAP64(_x)     bswap_64(_x)    // (result OPPOSITE of input)

 #define SWAP_OFF_T(o)  (sizeof(o) <= 4 ? SWAP32((U32)o) : SWAP64(o))

/*-------------------------------------------------------------------*/
/*             Guest storage FETCH/STORE macros                      */
/*-------------------------------------------------------------------*/
/* The following macros fetch a value from emulated guest storage    */
/* into a local work variable or store a local work variable into    */
/* emulated guest storage, performing a CONDITIONAL swap in between  */
/* (via the "CSWAPxx()" macro) to ensure the value placed into guest */
/* storage is always big endian or that the local work variable is   */
/* always in the expected big or little endian format (depending on  */
/* which endianness Hercules was built for).                         */
/*-------------------------------------------------------------------*/

#define FETCH_HW( _val, _stor )     (_val) = fetch_hw( _stor )
#define FETCH_FW( _val, _stor )     (_val) = fetch_fw( _stor )
#define FETCH_F3( _val, _stor )     (_val) = fetch_f3( _stor )
#define FETCH_DW( _val, _stor )     (_val) = fetch_dw( _stor )

#define STORE_HW( _stor, _val )     store_hw( _stor, _val )
#define STORE_FW( _stor, _val )     store_fw( _stor, _val )
#define STORE_F3( _stor, _val )     store_f3( _stor, _val )
#define STORE_DW( _stor, _val )     store_dw( _stor, _val )

/*-------------------------------------------------------------------*/
/*            CKD/CCKD header field FETCH/STORE macros               */
/*-------------------------------------------------------------------*/
/* The following macros fetch a value from a CCKD dasd header field  */
/* into a local work variable or store a local work variable into    */
/* a CCKD dasd header field (e.g. CKD_DEVHDR, CCKD_DEVHDR) doing an  */
/* UNCONDITIONAL "SWAPxx()" in between to ensure the numeric value   */
/* stored into, or fetched from, the CCKD header field is always in  */
/* LITTLE endian format, accomplishing the complete opposite of the  */
/* above "FETCH_FW/STORE_FW/etc" macros.                             */
/*-------------------------------------------------------------------*/

#define FETCH_LE_HW( _val, _stor )  (_val) = SWAP16( fetch_hw( _stor ))
#define FETCH_LE_FW( _val, _stor )  (_val) = SWAP32( fetch_fw( _stor ))
#define FETCH_LE_DW( _val, _stor )  (_val) = SWAP64( fetch_dw( _stor ))

#define STORE_LE_HW( _stor, _val )  store_hw( _stor, SWAP16( _val ))
#define STORE_LE_FW( _stor, _val )  store_fw( _stor, SWAP32( _val ))
#define STORE_LE_DW( _stor, _val )  store_dw( _stor, SWAP64( _val ))

#include "machdep.h"

#endif /* !defined( _OPCODE_H ) */

/*-------------------------------------------------------------------*/
/*          (delineates ARCH_DEP from non-arch_dep)                  */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/*               Architecture *DEPENDENT* macros                     */
/*-------------------------------------------------------------------*/
/*  The below macros (due to being outside of the above "#endif"     */
/*  for "_OPCODE_H") are undef'ed and then re-defined differently    */
/*  for each subsequent new build architecture.                      */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/*    Macros for defining ARCH_DEP master opcode table entries       */
/*-------------------------------------------------------------------*/

#undef AD_GENx___x___x___
#undef AD_GENx370x___x___
#undef AD_GENx___x390x___
#undef AD_GENx370x390x___
#undef AD_GENx___x___x900
#undef AD_GENx370x___x900
#undef AD_GENx___x390x900
#undef AD_GENx370x390x900

#if   __GEN_ARCH == 370

#define AD_GENx___x___x___                                  \
    {                                                       \
                &operation_exception,                       \
                &operation_exception,                       \
                &operation_exception,                       \
        (void*) &iprint_ASMFMT_none,                        \
        (void*) &"?????" "\0" "?"                           \
    }

#define AD_GENx370x___x___( _mnemonic, _ifmt, _asmfmt, _ifunc_name )  \
    {                                                       \
        _GEN370( _ifunc_name )                              \
                &operation_exception,                       \
                &operation_exception,                       \
        (void*) &iprint_ ## _asmfmt,                        \
        (void*) & _mnemonic "\0" #_ifunc_name               \
    }

#define AD_GENx___x390x___( _mnemonic, _ifmt, _asmfmt, _ifunc_name )  \
    {                                                       \
                &operation_exception,                       \
                &operation_exception,                       \
                &operation_exception,                       \
        (void*) &iprint_ ## _asmfmt,                        \
        (void*) & _mnemonic "\0" #_ifunc_name               \
    }

#define AD_GENx370x390x___( _mnemonic, _ifmt, _asmfmt, _ifunc_name )  \
    {                                                       \
        _GEN370( _ifunc_name )                              \
                &operation_exception,                       \
                &operation_exception,                       \
        (void*) &iprint_ ## _asmfmt,                        \
        (void*) & _mnemonic "\0" #_ifunc_name               \
    }

#define AD_GENx___x___x900( _mnemonic, _ifmt, _asmfmt, _ifunc_name )  \
    {                                                       \
                &operation_exception,                       \
                &operation_exception,                       \
                &operation_exception,                       \
        (void*) &iprint_ ## _asmfmt,                        \
        (void*) & _mnemonic "\0" #_ifunc_name               \
    }

#define AD_GENx370x___x900( _mnemonic, _ifmt, _asmfmt, _ifunc_name )  \
    {                                                       \
        _GEN370( _ifunc_name )                              \
                &operation_exception,                       \
                &operation_exception,                       \
        (void*) &iprint_ ## _asmfmt,                        \
        (void*) & _mnemonic "\0" #_ifunc_name               \
    }

#define AD_GENx___x390x900( _mnemonic, _ifmt, _asmfmt, _ifunc_name )  \
    {                                                       \
                &operation_exception,                       \
                &operation_exception,                       \
                &operation_exception,                       \
        (void*) &iprint_ ## _asmfmt,                        \
        (void*) & _mnemonic "\0" #_ifunc_name               \
    }

#define AD_GENx370x390x900( _mnemonic, _ifmt, _asmfmt, _ifunc_name )  \
    {                                                       \
        _GEN370( _ifunc_name )                              \
                &operation_exception,                       \
                &operation_exception,                       \
        (void*) &iprint_ ## _asmfmt,                        \
        (void*) & _mnemonic "\0" #_ifunc_name               \
    }

#elif __GEN_ARCH == 390

#define AD_GENx___x___x___                                  \
    {                                                       \
                &operation_exception,                       \
                &operation_exception,                       \
                &operation_exception,                       \
        (void*) &iprint_ASMFMT_none,                        \
        (void*) &"?????" "\0" "?"                           \
    }

#define AD_GENx370x___x___( _mnemonic, _ifmt, _asmfmt, _ifunc_name )  \
    {                                                       \
                &operation_exception,                       \
                &operation_exception,                       \
                &operation_exception,                       \
        (void*) &iprint_ ## _asmfmt,                        \
        (void*) & _mnemonic "\0" #_ifunc_name               \
    }

#define AD_GENx___x390x___( _mnemonic, _ifmt, _asmfmt, _ifunc_name )  \
    {                                                       \
                &operation_exception,                       \
        _GEN390( _ifunc_name )                              \
                &operation_exception,                       \
        (void*) &iprint_ ## _asmfmt,                        \
        (void*) & _mnemonic "\0" #_ifunc_name               \
    }

#define AD_GENx370x390x___( _mnemonic, _ifmt, _asmfmt, _ifunc_name )  \
    {                                                       \
                &operation_exception,                       \
        _GEN390( _ifunc_name )                              \
                &operation_exception,                       \
        (void*) &iprint_ ## _asmfmt,                        \
        (void*) & _mnemonic "\0" #_ifunc_name               \
    }

#define AD_GENx___x___x900( _mnemonic, _ifmt, _asmfmt, _ifunc_name )  \
    {                                                       \
                &operation_exception,                       \
                &operation_exception,                       \
                &operation_exception,                       \
        (void*) &iprint_ ## _asmfmt,                        \
        (void*) & _mnemonic "\0" #_ifunc_name               \
    }

#define AD_GENx370x___x900( _mnemonic, _ifmt, _asmfmt, _ifunc_name )  \
    {                                                       \
                &operation_exception,                       \
                &operation_exception,                       \
                &operation_exception,                       \
        (void*) &iprint_ ## _asmfmt,                        \
        (void*) & _mnemonic "\0" #_ifunc_name               \
    }

#define AD_GENx___x390x900( _mnemonic, _ifmt, _asmfmt, _ifunc_name )  \
    {                                                       \
                &operation_exception,                       \
        _GEN390( _ifunc_name )                              \
                &operation_exception,                       \
        (void*) &iprint_ ## _asmfmt,                        \
        (void*) & _mnemonic "\0" #_ifunc_name               \
    }

#define AD_GENx370x390x900( _mnemonic, _ifmt, _asmfmt, _ifunc_name )  \
    {                                                       \
                &operation_exception,                       \
        _GEN390( _ifunc_name )                              \
                &operation_exception,                       \
        (void*) &iprint_ ## _asmfmt,                        \
        (void*) & _mnemonic "\0" #_ifunc_name               \
    }

#elif __GEN_ARCH == 900

#define AD_GENx___x___x___                                  \
    {                                                       \
                &operation_exception,                       \
                &operation_exception,                       \
                &operation_exception,                       \
        (void*) &iprint_ASMFMT_none,                        \
        (void*) &"?????" "\0" "?"                           \
    }

#define AD_GENx370x___x___( _mnemonic, _ifmt, _asmfmt, _ifunc_name )  \
    {                                                       \
                &operation_exception,                       \
                &operation_exception,                       \
                &operation_exception,                       \
        (void*) &iprint_ ## _asmfmt,                        \
        (void*) & _mnemonic "\0" #_ifunc_name               \
    }

#define AD_GENx___x390x___( _mnemonic, _ifmt, _asmfmt, _ifunc_name )  \
    {                                                       \
                &operation_exception,                       \
                &operation_exception,                       \
                &operation_exception,                       \
        (void*) &iprint_ ## _asmfmt,                        \
        (void*) & _mnemonic "\0" #_ifunc_name               \
    }

#define AD_GENx370x390x___( _mnemonic, _ifmt, _asmfmt, _ifunc_name )  \
    {                                                       \
                &operation_exception,                       \
                &operation_exception,                       \
                &operation_exception,                       \
        (void*) &iprint_ ## _asmfmt,                        \
        (void*) & _mnemonic "\0" #_ifunc_name               \
    }

#define AD_GENx___x___x900( _mnemonic, _ifmt, _asmfmt, _ifunc_name )  \
    {                                                       \
                &operation_exception,                       \
                &operation_exception,                       \
        _GEN900( _ifunc_name )                              \
        (void*) &iprint_ ## _asmfmt,                        \
        (void*) & _mnemonic "\0" #_ifunc_name               \
    }

#define AD_GENx370x___x900( _mnemonic, _ifmt, _asmfmt, _ifunc_name )  \
    {                                                       \
                &operation_exception,                       \
                &operation_exception,                       \
        _GEN900( _ifunc_name )                              \
        (void*) &iprint_ ## _asmfmt,                        \
        (void*) & _mnemonic "\0" #_ifunc_name               \
    }

#define AD_GENx___x390x900( _mnemonic, _ifmt, _asmfmt, _ifunc_name )  \
    {                                                       \
                &operation_exception,                       \
                &operation_exception,                       \
        _GEN900( _ifunc_name )                              \
        (void*) &iprint_ ## _asmfmt,                        \
        (void*) & _mnemonic "\0" #_ifunc_name               \
    }

#define AD_GENx370x390x900( _mnemonic, _ifmt, _asmfmt, _ifunc_name )  \
    {                                                       \
                &operation_exception,                       \
                &operation_exception,                       \
        _GEN900( _ifunc_name )                              \
        (void*) &iprint_ ## _asmfmt,                        \
        (void*) & _mnemonic "\0" #_ifunc_name               \
    }

#else
#error HUH?!
#endif

/*-------------------------------------------------------------------*/
/*               PSW Instruction Address macros                      */
/*-------------------------------------------------------------------*/
// The _IA_FROM_IP primary helper macro returns a raw virtual PSW
// 'IA' value corresponding to where an offset mainstor 'ip' points,
// i.e. psw.ia <== aiv + ((ip + offset) - aip).

#undef  _IA_FROM_IP
#define _IA_FROM_IP( _aiv, _regs, _offset )                           \
  (                                                                   \
    (_regs)->_aiv                                                     \
    + ((uintptr_t)(_regs)->ip - (uintptr_t)(_regs)->aip)              \
    + (_offset)                                                       \
  )

//---------------------------------------------------------------------
// The generic PSW_IA_FROM_IP macro returns a PSW 'IA' as VADR value
// corresponding to where regs 'ip' is pointing plus a passed offset,
// with the resulting PSW 'IA' value masked with ADDRESS_MAXWRAP.

#undef  PSW_IA_FROM_IP
#define PSW_IA_FROM_IP( _regs, _offset )                              \
                                                                      \
 ((VADR)(_IA_FROM_IP( AIV, (_regs), (_offset)) & ADDRESS_MAXWRAP( _regs )))

//---------------------------------------------------------------------
// The next three macros are minor architectural variations of the
// above PSW_IA_FROM_IP macro that return a PSW 'IA' value matching
// where 'ip' plus a passed offset is pointing. The only difference is
// each are specific to the implied architecture, none of them cast
// the result to VADR, and only PSW_IA24 applies an addressing mask.

#undef  PSW_IA64
#define PSW_IA64( _regs, _offset )                                    \
                                                                      \
      (_IA_FROM_IP( AIV_G, (_regs), (_offset)))

#undef  PSW_IA31
#define PSW_IA31( _regs, _offset )                                    \
                                                                      \
      (_IA_FROM_IP( AIV_L, (_regs), (_offset)))

#undef  PSW_IA24
#define PSW_IA24( _regs, _offset )                                    \
                                                                      \
      (_IA_FROM_IP( AIV_L, (_regs), (_offset)) & AMASK24)

//---------------------------------------------------------------------
// The PSEUDO_INVALID_AIE value is used whenever you wish to force
// a break in normal instruction processing and cause the 'instfetch'
// function to be called to perform a full address translation and
// fetch of the next instruction. It is mostly used by instruction
// stepping and tracing logic as well as by the SUCCESS_BRANCH macro
// when the target of the branch is in a different mainstor page.
// The "INVALID_AIE" value is used only by the INVALIDATE_AIA and
// INVALIDATE_AIA_MAIN macros to ALSO to prevent any unwanted PSW
// updating by short circuiting both the MAYBE_SET_PSW_IA_FROM_IP and
// SET_PSW_IA_AND_MAYBE_IP macros. The "PSEUDO_INVALID_AIE" value
// also forces instfetch to be called but additionally allows both
// the MAYBE_SET_PSW_IA_FROM_IP and SET_PSW_IA_AND_MAYBE_IP macros
// to continue behaving normally.

#define INVALID_AIE             (NULL)
#define PSEUDO_INVALID_AIE      ((BYTE*) 1)

#undef  VALID_AIE
#define VALID_AIE( _regs )      ((_regs)->aie != INVALID_AIE)

//---------------------------------------------------------------------
// The MAYBE_SET_PSW_IA_FROM_IP macro (notice the 'MAYBE' and 'SET'
// in its name) CONDITIONALLY sets the virtual psw 'IA' value to point
// to the same corresponding place as where the current mainstor 'ip'
// currently points (with no offset), but does so IF AND ONLY IF the
// 'aie' value is still valid. Otherwise it does absolutely nothing.

#undef  MAYBE_SET_PSW_IA_FROM_IP
#define MAYBE_SET_PSW_IA_FROM_IP( _regs )                             \
  do                                                                  \
  {                                                                   \
    if (VALID_AIE( _regs ))                                           \
      (_regs)->psw.IA = PSW_IA_FROM_IP( (_regs), 0 );                 \
  }                                                                   \
  while (0)

//---------------------------------------------------------------------
// The SET_PSW_IA_AND_MAYBE_IP macro (notice the 'MAYBE' in its name)
// performs the opposite of the above MAYBE_SET_PSW_IA_FROM_IP macro:
// it sets the mainstor 'ip' to the same corresponding place as where
// the passed virtual psw 'IA' value points. It first UNCONDITIONALLY
// SETS the PSW IA to the passed value and then MAYBE also sets the
// 'ip' to the same corresponding place in mainstor, but it ONLY does
// so IF AND ONLY IF the updated 'ia' value still points within the
// same virtual page. If it doesn't, then the 'aie' value is set to
// NULL to force a full 'instfetch' which then calculates a new 'ip',
// 'aie' and 'aip' value on the next instruction fetch.

#undef  SET_PSW_IA_AND_MAYBE_IP
#define SET_PSW_IA_AND_MAYBE_IP( _regs, _ia )                         \
  do                                                                  \
  {                                                                   \
    (_regs)->psw.IA = (_ia) & ADDRESS_MAXWRAP( _regs );               \
                                                                      \
    if (VALID_AIE( _regs ))                                           \
    {                                                                 \
      if ((_regs)->AIV == ((_regs)->psw.IA & (PAGEFRAME_PAGEMASK|1))) \
        (_regs)->ip = _PSW_IA_MAIN( (_regs), (_regs)->psw.IA );       \
      else                                                            \
        (_regs)->aie = INVALID_AIE;                                   \
    }                                                                 \
  }                                                                   \
  while (0)

//---------------------------------------------------------------------
// The INST_UPDATE_PSW macro sets the psw ILC to the passed value and
// bumps the regs->ip instruction pointer by the passed _len value,
// BUT ONLY IF the passed value is non-zero.
//
// It is typically used by the instruction decoder macros to set the
// ilc for the instruction just decoded and to then bump the 'ip' to
// the next sequential instruction.
//
// Normally '_len' and '_ilc' are the same value (being the length
// of the instruction being decoded). The reason they are passed as
// separate values however is to allow for branch instructions to
// set the ilc value while at the same time NOT bumping the 'ip'
// since they don't know yet if the branch will be taken or not.
// If it is, they call the SUCCESSFUL_BRANCH macro. If not, they
// call the INST_UPDATE_PSW to go on to the next instruction.

#undef  INST_UPDATE_PSW
#define INST_UPDATE_PSW( _regs, _len, _ilc )                          \
  do                                                                  \
  {                                                                   \
    (_regs)->ip += (_len);                                            \
    if (_ilc)                                                         \
      (_regs)->psw.ilc = (_ilc);                                      \
  }                                                                   \
  while(0)

/*-------------------------------------------------------------------*/
/*             Accelerator for Instruction Addresses                 */
/*-------------------------------------------------------------------*/
/* The AIA is a term used to refer to the set of REGS fields that    */
/* together control instruction execution. It consists if the 'ip',  */
/* 'aip', 'aie' and 'aiv' fields. The 'ip' field of course is the    */
/* mainstor instruction pointer. The 'aip' is the page address of    */
/* the mainstor page associated with the 'ip'. The 'aiv' field is    */
/* the guest virtual address of the page corresponding to the aip.   */
/* The 'aie' is the "logical end" of the 'aip'. It is the highest    */
/* mainstor 'ip' address that we can safely DIRECTLY fetch the next  */
/* instruction from. Once the 'ip' reaches the 'aie', instruction    */
/* fetching is forced to do a call to the "instfetch" function to    */
/* perform full translation of the address of the next instruction,  */
/* recalculating new 'ip', 'aip', 'aiv' and 'aie' values which can   */
/* then be used again for DIRECT instruction fetching. This allows   */
/* us to "accelerate" instruction fetching since we don't have to    */
/* go through full address translation for each instruction fetch.   */
/* We only need to do so when the instruction being fetched crosses  */
/* over into a new page. That's the 'aie'. It is set to the end of   */
/* the 'aip' page minus 5 bytes. As long as 'ip' is LESS than aie,   */
/* we know we can safely fetch the next instruction from that same   */
/* mainstor page. Otherwise we need to do a full instfetch.          */
/*-------------------------------------------------------------------*/

// The INVALIDATE_AIA macro essentially renders all AIA fields as
// being invalid, thereby forcing a full instfetch to be performed.
// It first ensures that the IA (address of the next instruction)
// in the guest PSW structure matches where the current regs 'ip'
// instruction pointer is pointing, and then "invalidates" all AIA
// fields by setting the 'aie' value to NULL, thereby forcing the
// next instruction fetch to call the 'instfetch' function.

#undef  INVALIDATE_AIA
#define INVALIDATE_AIA( _regs )                                       \
  do                                                                  \
  {                                                                   \
    if (VALID_AIE( _regs ))                                           \
    {                                                                 \
      (_regs)->psw.IA = PSW_IA_FROM_IP( (_regs), 0 );                 \
      (_regs)->aie = INVALID_AIE;                                     \
    }                                                                 \
  }                                                                   \
  while (0)

//---------------------------------------------------------------------
// The INVALIDATE_AIA_MAIN macro performs a conditional invalidation
// of the AIA, but if and ONLY if the passed mainstor address matches
// the 'aip'. It is used (e.g. the "invalidate_tlbe" function called
// by the STORKEY_INVALIDATE macro) to invalidate the AIA whenever a
// DAT page table entry is invalidated that happens to correspond to
// the mainstor page we're currently fetching instructions from. It
// first updates the psw IA to match the current instruction address
// and then sets the 'aie' to NULL to force a full instruction fetch.

#undef  INVALIDATE_AIA_MAIN
#define INVALIDATE_AIA_MAIN( _regs, _main )                           \
  do                                                                  \
  {                                                                   \
    if (VALID_AIE( _regs ) &&                                         \
        (((uintptr_t)(_main)) & PAGEFRAME_PAGEMASK) ==                \
         ((uintptr_t)(_regs)->aip)                                    \
    )                                                                 \
    {                                                                 \
      (_regs)->psw.IA = PSW_IA_FROM_IP( (_regs), 0 );                 \
      (_regs)->aie = INVALID_AIE;                                     \
    }                                                                 \
  }                                                                   \
  while (0)

//---------------------------------------------------------------------
// The _PSW_IA_MAIN helper macro returns the mainstor address
// corresponding to a given guest virtual address. NOTE CAREFULLY
// that it PRESUMES the virtual address that is passed is within the
// same mainstor page as we're currently fetching instructions from!

#undef  _PSW_IA_MAIN
#define _PSW_IA_MAIN( _regs, _addr )                                  \
  (                                                                   \
    (BYTE*)                                                           \
    (                                                                 \
        (uintptr_t)(_regs)->aip                                       \
        |                                                             \
        (uintptr_t)((_addr) & PAGEFRAME_BYTEMASK)                     \
    )                                                                 \
  )

/*-------------------------------------------------------------------*/
/*                     Instruction fetching                          */
/*-------------------------------------------------------------------*/

// The _VALID_IP helper macro is used only by the INSTRUCTION_FETCH
// macro and returns either true/false indicating whether the current
// instruction pointer is still valid or not (so INSTRUCTION_FETCH
// can know whether to call the instfetch' function or not). It does
// this by verifying regs->ip is either still less than regs->aie,
// or for the 'exec' case, that the mainstor address corresponding
// to regs->ET (the target of the execute instruction) is still less
// than regs->aie. It determines the mainstor address of regs->ET
// via the _PSW_IA_MAIN helper macro.

#undef  _VALID_IP
#define _VALID_IP( _regs, _exec )                                     \
  (                                                                   \
      /* Instr NOT being EXecuted and instr ptr in same page */       \
      (1                                                              \
        && !(_exec)                                                   \
        &&  (_regs)->ip < (_regs)->aie                                \
      )                                                               \
  ||                                                                  \
      /* Instr IS being EXecuted and instr ptr in same page */        \
      (1                                                              \
        && (_exec)                                                    \
        /* Execute target in same virtual page? */                    \
        && ((_regs)->ET & (PAGEFRAME_PAGEMASK|0x01)) == (_regs)->AIV  \
        /* Execute target in same mainstor page? */                   \
        && _PSW_IA_MAIN( (_regs), (_regs)->ET ) < (_regs)->aie        \
      )                                                               \
  )

//---------------------------------------------------------------------
// The INSTRUCTION_FETCH macro returns a mainstor 'ip' pointer
// pointing to the next instruction to be executed. It either
// returns regs->ip directly (or for the 'exec' case, the
// corresponding mainstor address of the instruction being
// executed), or else calls the 'instfetch' function to calculate
// a new set of AIA values (ip, aie, aip and aiv).

#undef  INSTRUCTION_FETCH
#define INSTRUCTION_FETCH( _regs, _exec )                             \
                                                                      \
  /* If ip still valid, use current ip or target of executed instr */ \
  likely( _VALID_IP( (_regs), (_exec) )) ?                            \
  (                                                                   \
    (_exec) ?                                                         \
      _PSW_IA_MAIN( (_regs), (_regs)->ET )                            \
      :                                                               \
      (_regs)->ip                                                     \
  )                                                                   \
  /* Else do a full instruction fetch (which updates the AIA too) */  \
  : ARCH_DEP( instfetch )( (_regs), (_exec) )

/*-------------------------------------------------------------------*/
/*                   Instruction execution                           */
/*-------------------------------------------------------------------*/

#if !defined( FEATURE_073_TRANSACT_EXEC_FACILITY )

  #undef  ABORT_TRANS                                   /* (nothing) */
  #define ABORT_TRANS( _regs, _retry, _tac )            /* (nothing) */

  #undef  TXF_INSTRADDR_CONSTRAINT                      /* (nothing) */
  #define TXF_INSTRADDR_CONSTRAINT( _regs )             /* (nothing) */

  #undef  TXF_INSTRCOUNT_CONSTRAINT                     /* (nothing) */
  #define TXF_INSTRCOUNT_CONSTRAINT( _ip, _regs )       /* (nothing) */

  #undef  TXF_RAND_ABORT_CONSTRAINT                     /* (nothing) */
  #define TXF_RAND_ABORT_CONSTRAINT( _regs )            /* (nothing) */

  #undef  CHECK_TXF_CONSTRAINTS                         /* (nothing) */
  #define CHECK_TXF_CONSTRAINTS( _ip, _regs )           /* (nothing) */

#else /* defined( FEATURE_073_TRANSACT_EXEC_FACILITY ) */

  #undef  ABORT_TRANS
  #define ABORT_TRANS( _regs, _retry, _tac )                          \
    ARCH_DEP( abort_transaction )( (_regs), (_retry), (_tac), PTT_LOC )

  #undef  TXF_INSTRADDR_CONSTRAINT
  #define TXF_INSTRADDR_CONSTRAINT( _regs )                           \
  do {                                                                \
    if (1                                                             \
      && (_regs)->txf_contran                                         \
      && (_regs)->ip >= (_regs)->txf_aie                              \
    )                                                                 \
    {                                                                 \
      (_regs)->txf_why |= TXF_WHY_INSTRADDR;                          \
      ABORT_TRANS( (_regs), -ABORT_RETRY_PGMCHK, TAC_INSTR );         \
    }                                                                 \
  } while (0)

  #undef  TXF_INSTRCOUNT_CONSTRAINT
  #define TXF_INSTRCOUNT_CONSTRAINT( _ip, _regs )                     \
  do {                                                                \
    if (1                                                             \
      && (_regs)->txf_contran                                         \
      && (_regs)->txf_instctr > MAX_TXF_CONTRAN_INSTR                 \
      && memcmp( (_ip), "\xb2\xf8", 2 ) != 0                          \
    )                                                                 \
    {                                                                 \
      (_regs)->txf_why |= TXF_WHY_INSTRCOUNT;                         \
      ABORT_TRANS( (_regs), -ABORT_RETRY_PGMCHK, TAC_INSTR );         \
    }                                                                 \
  } while (0)

  #undef  TXF_RAND_ABORT_CONSTRAINT
  #define TXF_RAND_ABORT_CONSTRAINT( _regs )                          \
  do {                                                                \
    if (1                                                             \
      && (_regs)->txf_abortctr                                        \
      && (_regs)->txf_instctr >= (_regs)->txf_abortctr                \
    )                                                                 \
    {                                                                 \
      (_regs)->txf_why |= TXF_WHY_RAND_ABORT;                         \
      ABORT_TRANS( (_regs), -ABORT_RETRY_PGMCHK,                      \
        (_regs)->txf_random_tac );                                    \
    }                                                                 \
  } while (0)

  #undef  CHECK_TXF_CONSTRAINTS
  #define CHECK_TXF_CONSTRAINTS( _ip, _regs )                         \
  do {                                                                \
    if ((_regs)->txf_tnd)                                             \
    {                                                                 \
      TXF_INSTRADDR_CONSTRAINT( (_regs) );                            \
      (_regs)->txf_instctr++;                                         \
      TXF_INSTRCOUNT_CONSTRAINT( (_ip), (_regs) );                    \
      TXF_RAND_ABORT_CONSTRAINT( (_regs) );                           \
    }                                                                 \
  } while (0)

#endif /* !defined( FEATURE_073_TRANSACT_EXEC_FACILITY ) */

#undef  EXECUTE_INSTRUCTION
#define EXECUTE_INSTRUCTION( _oct, _ip, _regs )                       \
do {                                                                  \
    FOOTPRINT( (_ip), (_regs) );                                      \
    BEG_COUNT_INSTR( (_ip), (_regs) );                                \
    (_oct)[ fetch_hw( (_ip) )]( (_ip), (_regs) );                     \
    END_COUNT_INSTR( (_ip), (_regs) );                                \
} while (0)

#if defined( FEATURE_073_TRANSACT_EXEC_FACILITY )

  #undef  TXF_EXECUTE_INSTRUCTION
  #define TXF_EXECUTE_INSTRUCTION( _oct, _ip, _regs )                 \
  do {                                                                \
      CHECK_TXF_CONSTRAINTS( (_ip), (_regs) );                        \
      FOOTPRINT( (_ip), (_regs) );                                    \
      BEG_COUNT_INSTR( (_ip), (_regs) );                              \
      (_oct)[ fetch_hw( (_ip) )]( (_ip), (_regs) );                   \
      END_COUNT_INSTR( (_ip), (_regs) );                              \
  } while (0)

  #undef  TXF_UNROLLED_EXECUTE
  #define TXF_UNROLLED_EXECUTE( _oct, _regs )                         \
    if ((_regs)->ip >= (_regs)->aie) break;                           \
    TXF_EXECUTE_INSTRUCTION( (_oct), (_regs)->ip, (_regs) )

#endif /* !defined( FEATURE_073_TRANSACT_EXEC_FACILITY ) */

#undef  UNROLLED_EXECUTE
#define UNROLLED_EXECUTE( _oct, _regs )                               \
  if ((_regs)->ip >= (_regs)->aie) break;                             \
  EXECUTE_INSTRUCTION( (_oct), (_regs)->ip, (_regs) )

/*-------------------------------------------------------------------*/
/*                        Branching                                  */
/*-------------------------------------------------------------------*/

#undef  SUCCESSFUL_BRANCH
#define SUCCESSFUL_BRANCH( _regs, _addr )                             \
                                                                      \
    ARCH_DEP( SuccessfulBranch )( (_regs), (_addr) )

#undef  SUCCESSFUL_RELATIVE_BRANCH
#define SUCCESSFUL_RELATIVE_BRANCH( _regs, _offset )                  \
                                                                      \
    ARCH_DEP( SuccessfulRelativeBranch )( (_regs), (_offset) )

/*-------------------------------------------------------------------*/
/*                         (other)                                   */
/*-------------------------------------------------------------------*/

/* Program check if fpc is not valid contents for FPC register */
#undef  FPC_CHECK
#define FPC_CHECK( _fpc, _regs )    ARCH_DEP( FPC_check )( (_regs), (_fpc) )

/*-------------------------------------------------------------------*/
/*        PER 1 GRA (General Register Alteration) support            */
/*-------------------------------------------------------------------*/

#undef PER_GRA_MASK
#undef PER_GRA_MASK2
#undef PER_GRA_MASK4
#undef PER_GRA_CHECK

#if defined( FEATURE_PER1 )

  #define PER_GRA_MASK(  _r1 )                  (0x8000 >> (_r1))
  #define PER_GRA_MASK2( _r1, _r2 )             (PER_GRA_MASK(  _r1 )      | PER_GRA_MASK(  _r2 ))
  #define PER_GRA_MASK4( _r1, _r2, _r3, _r4 )   (PER_GRA_MASK2( _r1, _r2 ) | PER_GRA_MASK2( _r3, _r4 ))
  #define PER_GRA_CHECK( _regs, _mask )                               \
    do                                                                \
    {                                                                 \
      if (1                                                           \
          && EN_IC_PER_GRA( _regs )                                   \
          && (_mask) & ((_regs)->CR(9) & CR9_GRMASK)                  \
      )                                                               \
        ARCH_DEP( per1_gra )( _regs );                                \
    }                                                                 \
    while (0)

#else /* !defined( FEATURE_PER1 ) */

  #define PER_GRA_MASK(  _r1 )
  #define PER_GRA_MASK2( _r1, _r2 )
  #define PER_GRA_MASK4( _r1, _r2, _r3, _r4 )
  #define PER_GRA_CHECK( _regs, _mask )

#endif /* defined( FEATURE_PER1 ) */

/*-------------------------------------------------------------------*/
/*          PER3 Breaking-Event-Address Register (BEAR)              */
/*-------------------------------------------------------------------*/

#undef SET_BEAR_REG
#undef SET_BEAR_EX_REG

#if defined( FEATURE_PER3 )

  #define SET_BEAR_REG( _regs, _ip )                                  \
    ARCH_DEP( Set_BEAR_Reg )( &(_regs)->bear,    (_regs), (_ip) )

  #define SET_BEAR_EX_REG( _regs, _ip )                               \
    ARCH_DEP( Set_BEAR_Reg )( &(_regs)->bear_ex, (_regs), (_ip) )

#else

  #define SET_BEAR_REG(    _regs, _ip )
  #define SET_BEAR_EX_REG( _regs, _ip )

#endif

/*-------------------------------------------------------------------*/
/*                 PER3 Event Suppression                            */
/*-------------------------------------------------------------------*/

#undef  IS_PER_SUPRESS

#if defined( FEATURE_PER3 )

  #define IS_PER_SUPRESS( _regs, _event )                             \
    ARCH_DEP( is_per3_event_suppressed )( (_regs), (_event) )

#else

  #define IS_PER_SUPRESS( _regs, _event )                             \
    false

#endif

/*-------------------------------------------------------------------*/
/*            PER3 Zero-Address Detection Facility                   */
/*-------------------------------------------------------------------*/

#undef  PER_ZEROADDR_CHECK
#undef  PER_ZEROADDR_CHECK2

#undef  PER_ZEROADDR_LCHECK
#undef  PER_ZEROADDR_LCHECK2

#undef  PER_ZEROADDR_L24CHECK
#undef  PER_ZEROADDR_L24CHECK2

#undef  PER_ZEROADDR_XCHECK
#undef  PER_ZEROADDR_XCHECK2

#if defined( FEATURE_PER_ZERO_ADDRESS_DETECTION_FACILITY )

  // The CHECK macros are designed for instructions where the operand
  // address is specified in a register but the operand length is not
  // (or is otherwise implied, perhaps by a function code in another
  // operand register for example), and/or for instructions where it
  // is otherwise unpredictable whether operand data will actually be
  // accessed or not. Thus the only thing we can check is whether the
  // register holding the address of the operand is zero or not.

  #define PER_ZEROADDR_CHECK(   _regs,  _r1 )                         \
   ARCH_DEP( per3_zero_check )((_regs),(_r1))

  #define PER_ZEROADDR_CHECK2(   _regs,  _r1,  _r2 )                  \
   ARCH_DEP( per3_zero_check2 )((_regs),(_r1),(_r2))

  // The LCHECK macros are designed for RR and RRE and similar format
  // type instructions where a PER Zero-Address event does NOT occur
  // when the operand length (specified in another register) is zero,
  // thus causing that operand's storage to never be accessed. Note
  // that all 32 (or 64) bits of the length register are checked.

  #define PER_ZEROADDR_LCHECK(   _regs,  _r1,  _l1 )                  \
   ARCH_DEP( per3_zero_lcheck )((_regs),(_r1),(_l1))

  #define PER_ZEROADDR_LCHECK2(   _regs,  _r1,  _l1,  _r2,  _l2 )     \
   ARCH_DEP( per3_zero_lcheck2 )((_regs),(_r1),(_l1),(_r2),(_l2))

  // The L24CHECK macros are identical to the LCHECK macros except
  // for the operand length check: instead of checking all 32 or 64
  // bits of the register containing the operand length, we instead
  // check only the low-order 24 bits of the length register via the
  // GR_LA24 macro. They are designed for MVCL and CLCL and other
  // similar type instructions.

  #define PER_ZEROADDR_L24CHECK(   _regs,  _r1,  _l1 )                \
   ARCH_DEP( per3_zero_l24check )((_regs),(_r1),(_l1))

  #define PER_ZEROADDR_L24CHECK2(   _regs,  _r1,  _l1,  _r2,  _l2 )   \
   ARCH_DEP( per3_zero_l24check2 )((_regs),(_r1),(_l1),(_r2),(_l2))

  // The XCHECK macros are designed for RS/RX/S and similar format
  // type instructions where a PER Zero-Address event does NOT occur
  // unless the specified base or index register number is non-zero.
  // When the base or index register number is specified as zero, it
  // is not used in effective address calculations and thus PER Zero
  // Address Detection does not apply for that register.

  #define PER_ZEROADDR_XCHECK(   _regs,  _b1 )                        \
   ARCH_DEP( per3_zero_xcheck )((_regs),(_b1))

  #define PER_ZEROADDR_XCHECK2(   _regs,  _x2,  _b2 )                 \
   ARCH_DEP( per3_zero_xcheck2 )((_regs),(_x2),(_b2))

#else

  #define PER_ZEROADDR_CHECK(  _regs, _r1 )
  #define PER_ZEROADDR_CHECK2( _regs, _r1, _r2 )

  #define PER_ZEROADDR_LCHECK(  _regs, _r1, _l1 )
  #define PER_ZEROADDR_LCHECK2( _regs, _r1, _l1, _r2, _l2 )

  #define PER_ZEROADDR_L24CHECK(  _regs, _r1, _l1 )
  #define PER_ZEROADDR_L24CHECK2( _regs, _r1, _l1, _r2, _l2 )

  #define PER_ZEROADDR_XCHECK(  _regs, _b1 )
  #define PER_ZEROADDR_XCHECK2( _regs, _x2, _b2 )

#endif

/*-------------------------------------------------------------------*/
/*              Set addressing mode (BASSM, BSM)                     */
/*-------------------------------------------------------------------*/

#undef SET_ADDRESSING_MODE

#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )

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

#else /* !defined( FEATURE_001_ZARCH_INSTALLED_FACILITY ) */

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

#endif /* defined( FEATURE_001_ZARCH_INSTALLED_FACILITY ) */


/*-------------------------------------------------------------------*/
/*                 Floating-point helper macros                      */
/*-------------------------------------------------------------------*/

// Plain vanilla S/370 mode macros...

#undef _370_HFPREG_CHECK
#undef _370_HFPREG2_CHECK
#undef _370_HFPODD_CHECK
#undef _370_HFPODD2_CHECK

/* Program check if r1 is not 0, 2, 4, or 6 */
#define _370_HFPREG_CHECK(_r, _regs)                                        \
                                                                            \
  if ((_r) & 9)                                                             \
      (_regs)->program_interrupt( (_regs), PGM_SPECIFICATION_EXCEPTION )


/* Program check if r1 and r2 are not 0, 2, 4, or 6 */
#define _370_HFPREG2_CHECK(_r1, _r2, _regs)                                 \
                                                                            \
  if (0                                                                     \
      || ((_r1) & 9)                                                        \
      || ((_r2) & 9)                                                        \
  )                                                                         \
      (_regs)->program_interrupt( (_regs), PGM_SPECIFICATION_EXCEPTION )


/* Program check if r1 is not 0 or 4 */
#define _370_HFPODD_CHECK(_r, _regs)                                        \
                                                                            \
  if ((_r) & 11)                                                            \
      (_regs)->program_interrupt( (_regs), PGM_SPECIFICATION_EXCEPTION )


/* Program check if r1 and r2 are not 0 or 4 */
#define _370_HFPODD2_CHECK(_r1, _r2, _regs)                                 \
                                                                            \
  if (0                                                                     \
      || ((_r1) & 11)                                                       \
      || ((_r2) & 11)                                                       \
  )                                                                         \
      (_regs)->program_interrupt( (_regs), PGM_SPECIFICATION_EXCEPTION )


// Macros for 390 or z/Arch mode, -OR for 370 mode
// with HERC_370_EXTENSION *maybe* enabled or not...

#undef HFPREG_CHECK
#undef HFPREG2_CHECK
#undef HFPODD_CHECK
#undef HFPODD2_CHECK

#if !defined( FEATURE_BASIC_FP_EXTENSIONS )

  // Plain vanilla S/370 mode macros...

  #define HFPREG_CHECK(_r,_regs)        _370_HFPREG_CHECK((_r),(_regs))
  #define HFPREG2_CHECK(_r1,_r2,_regs)  _370_HFPREG2_CHECK((_r1),(_r2),(_regs))
  #define HFPODD_CHECK(_r,_regs)        _370_HFPODD_CHECK((_r),(_regs))
  #define HFPODD2_CHECK(_r1,_r2,_regs)  _370_HFPODD2_CHECK((_r1),(_r2),(_regs))

#else /* defined( FEATURE_BASIC_FP_EXTENSIONS ) */

  #if defined( _FEATURE_SIE )

    /* Program check if DFP instruction is executed when AFP control is zero */
    #define DFPINST_CHECK(_regs)                                                    \
                                                                                    \
        if (0                                                                       \
            || !((_regs)->CR(0) & CR0_AFP)                                          \
            || (SIE_MODE((_regs)) && !(HOST(_regs)->CR(0) & CR0_AFP))               \
        )                                                                           \
        {                                                                           \
            (_regs)->dxc = DXC_DFP_INSTRUCTION;                                     \
            (_regs)->program_interrupt( (_regs), PGM_DATA_EXCEPTION );              \
        }

    /* Program check if BFP instruction is executed when AFP control is zero */
    #define BFPINST_CHECK(_regs)                                                    \
                                                                                    \
        if (0                                                                       \
            || !((_regs)->CR(0) & CR0_AFP)                                          \
            || (SIE_MODE((_regs)) && !(HOST(_regs)->CR(0) & CR0_AFP))               \
        )                                                                           \
        {                                                                           \
            (_regs)->dxc = DXC_BFP_INSTRUCTION;                                     \
            (_regs)->program_interrupt( (_regs), PGM_DATA_EXCEPTION );              \
        }

    /* Program check if r1 is not 0, 2, 4, or 6 */
    #define HFPREG_CHECK(_r, _regs)                                                 \
                                                                                    \
        if (0                                                                       \
            || !((_regs)->CR(0) & CR0_AFP)                                          \
            || (SIE_MODE((_regs)) && !(HOST(_regs)->CR(0) & CR0_AFP))               \
        )                                                                           \
        {                                                                           \
            if (1                                                                   \
                && ARCH_370_IDX == sysblk.arch_mode                                 \
                && !FACILITY_ENABLED_ARCH( HERC_370_EXTENSION, ARCH_370_IDX )       \
            )                                                                       \
            {                                                                       \
                /* Normal 370 case: HERC_370_EXTENSION not enabled */               \
                _370_HFPREG_CHECK((_r),(_regs));                                    \
            }                                                                       \
            else /* 390 or zArch, -OR- 370 with HERC_370_EXTENSION enabled */       \
            {                                                                       \
                if ((_r) & 9)                                                       \
                {                                                                   \
                    (_regs)->dxc = DXC_AFP_REGISTER;                                \
                    (_regs)->program_interrupt( (_regs), PGM_DATA_EXCEPTION );      \
                }                                                                   \
            }                                                                       \
        }

    /* Program check if r1 and r2 are not 0, 2, 4, or 6 */
    #define HFPREG2_CHECK(_r1, _r2, _regs)                                          \
                                                                                    \
        if (0                                                                       \
            || !((_regs)->CR(0) & CR0_AFP)                                          \
            || (SIE_MODE((_regs)) && !(HOST(_regs)->CR(0) & CR0_AFP))               \
        )                                                                           \
        {                                                                           \
            if (1                                                                   \
                && ARCH_370_IDX == sysblk.arch_mode                                 \
                && !FACILITY_ENABLED_ARCH( HERC_370_EXTENSION, ARCH_370_IDX )       \
            )                                                                       \
            {                                                                       \
                /* Normal 370 case: HERC_370_EXTENSION not enabled */               \
                _370_HFPREG2_CHECK((_r1),(_r2),(_regs));                            \
            }                                                                       \
            else /* 390 or zArch, -OR- 370 with HERC_370_EXTENSION enabled */       \
            {                                                                       \
                if (0                                                               \
                    || ((_r1) & 9)                                                  \
                    || ((_r2) & 9)                                                  \
                )                                                                   \
                {                                                                   \
                    (_regs)->dxc = DXC_AFP_REGISTER;                                \
                    (_regs)->program_interrupt( (_regs), PGM_DATA_EXCEPTION );      \
                }                                                                   \
            }                                                                       \
        }

    /* Program check if r1 is not 0 or 4 */
    #define HFPODD_CHECK(_r, _regs)                                                 \
                                                                                    \
        if (1                                                                       \
            && ARCH_370_IDX == sysblk.arch_mode                                     \
            && !FACILITY_ENABLED_ARCH( HERC_370_EXTENSION, ARCH_370_IDX )           \
        )                                                                           \
        {                                                                           \
            /* Normal 370 case: HERC_370_EXTENSION not enabled */                   \
            _370_HFPODD_CHECK((_r),(_regs));                                        \
        }                                                                           \
        else /* 390 or zArch, -OR- 370 with HERC_370_EXTENSION enabled */           \
        {                                                                           \
            if ((_r) & 2)                                                           \
            {                                                                       \
                (_regs)->program_interrupt( (_regs), PGM_SPECIFICATION_EXCEPTION ); \
            }                                                                       \
            else if (0                                                              \
                || !((_regs)->CR(0) & CR0_AFP)                                      \
                || (SIE_MODE((_regs)) && !(HOST(_regs)->CR(0) & CR0_AFP))           \
            )                                                                       \
            {                                                                       \
                if ((_r) & 9)                                                       \
                {                                                                   \
                    (_regs)->dxc = DXC_AFP_REGISTER;                                \
                    (_regs)->program_interrupt( (_regs), PGM_DATA_EXCEPTION );      \
                }                                                                   \
            }                                                                       \
        }

    /* Program check if r1 and r2 are not 0 or 4 */
    #define HFPODD2_CHECK(_r1, _r2, _regs)                                          \
                                                                                    \
        if (1                                                                       \
            && ARCH_370_IDX == sysblk.arch_mode                                     \
            && !FACILITY_ENABLED_ARCH( HERC_370_EXTENSION, ARCH_370_IDX )           \
        )                                                                           \
        {                                                                           \
            /* Normal 370 case: HERC_370_EXTENSION not enabled */                   \
            _370_HFPODD2_CHECK((_r1),(_r2),(_regs));                                \
        }                                                                           \
        else /* 390 or zArch, -OR- 370 with HERC_370_EXTENSION enabled */           \
        {                                                                           \
            if (0                                                                   \
                || ((_r1) & 2)                                                      \
                || ((_r2) & 2)                                                      \
            )                                                                       \
            {                                                                       \
                (_regs)->program_interrupt( (_regs), PGM_SPECIFICATION_EXCEPTION ); \
            }                                                                       \
            else if (0                                                              \
                || !((_regs)->CR(0) & CR0_AFP)                                      \
                || (SIE_MODE((_regs)) && !(HOST(_regs)->CR(0) & CR0_AFP))           \
            )                                                                       \
            {                                                                       \
                if (0                                                               \
                    || ((_r1) & 9)                                                  \
                    || ((_r2) & 9)                                                  \
                )                                                                   \
                {                                                                   \
                    (_regs)->dxc = DXC_AFP_REGISTER;                                \
                    (_regs)->program_interrupt( (_regs), PGM_DATA_EXCEPTION );      \
                }                                                                   \
            }                                                                       \
        }

  #else /* !defined( _FEATURE_SIE ) */

    /* Program check if DFP instruction is executed when AFP control is zero */
    #define DFPINST_CHECK(_regs)                                                    \
                                                                                    \
        if (!((_regs)->CR(0) & CR0_AFP))                                            \
        {                                                                           \
            (_regs)->dxc = DXC_DFP_INSTRUCTION;                                     \
            (_regs)->program_interrupt( (_regs), PGM_DATA_EXCEPTION );              \
        }

    /* Program check if BFP instruction is executed when AFP control is zero */
    #define BFPINST_CHECK(_regs)                                                    \
                                                                                    \
        if (!((_regs)->CR(0) & CR0_AFP))                                            \
        {                                                                           \
            (_regs)->dxc = DXC_BFP_INSTRUCTION;                                     \
            (_regs)->program_interrupt( (_regs), PGM_DATA_EXCEPTION );              \
        }

    /* Program check if r1 is not 0, 2, 4, or 6 */
    #define HFPREG_CHECK(_r,_regs)                                                  \
                                                                                    \
        if (1                                                                       \
            && ARCH_370_IDX == sysblk.arch_mode                                     \
            && !FACILITY_ENABLED_ARCH( HERC_370_EXTENSION, ARCH_370_IDX )           \
        )                                                                           \
        {                                                                           \
            /* Normal 370 case: HERC_370_EXTENSION not enabled */                   \
            _370_HFPREG_CHECK((_r),(_regs));                                        \
        }                                                                           \
        else /* 390 or zArch, -OR- 370 with HERC_370_EXTENSION enabled */           \
        {                                                                           \
            if (!((_regs)->CR(0) & CR0_AFP))                                        \
            {                                                                       \
                if ((_r) & 9)                                                       \
                {                                                                   \
                    (_regs)->dxc = DXC_AFP_REGISTER;                                \
                    (_regs)->program_interrupt( (_regs), PGM_DATA_EXCEPTION );      \
                }                                                                   \
            }                                                                       \
        }

    /* Program check if r1 and r2 are not 0, 2, 4, or 6 */
    #define HFPREG2_CHECK(_r1,_r2,_regs)                                            \
                                                                                    \
        if (1                                                                       \
            && ARCH_370_IDX == sysblk.arch_mode                                     \
            && !FACILITY_ENABLED_ARCH( HERC_370_EXTENSION, ARCH_370_IDX )           \
        )                                                                           \
        {                                                                           \
            /* Normal 370 case: HERC_370_EXTENSION not enabled */                   \
            _370_HFPREG2_CHECK((_r1),(_r2),(_regs));                                \
        }                                                                           \
        else /* 390 or zArch, -OR- 370 with HERC_370_EXTENSION enabled */           \
        {                                                                           \
            if (!((_regs)->CR(0) & CR0_AFP) )                                       \
            {                                                                       \
                if (0                                                               \
                    || ((_r1) & 9)                                                  \
                    || ((_r2) & 9)                                                  \
                )                                                                   \
                {                                                                   \
                    (_regs)->dxc = DXC_AFP_REGISTER;                                \
                    (_regs)->program_interrupt( (_regs), PGM_DATA_EXCEPTION );      \
                }                                                                   \
            }                                                                       \
        }

    /* Program check if r1 is not 0 or 4 */
    #define HFPODD_CHECK(_r,_regs)                                                  \
                                                                                    \
        if (1                                                                       \
            && ARCH_370_IDX == sysblk.arch_mode                                     \
            && !FACILITY_ENABLED_ARCH( HERC_370_EXTENSION, ARCH_370_IDX )           \
        )                                                                           \
        {                                                                           \
            /* Normal 370 case: HERC_370_EXTENSION not enabled */                   \
            _370_HFPODD_CHECK((_r),(_regs));                                        \
        }                                                                           \
        else /* 390 or zArch, -OR- 370 with HERC_370_EXTENSION enabled */           \
        {                                                                           \
            if ((_r) & 2)                                                           \
            {                                                                       \
                (_regs)->program_interrupt( (_regs), PGM_SPECIFICATION_EXCEPTION ); \
            }                                                                       \
            else if (!((_regs)->CR(0) & CR0_AFP) )                                  \
            {                                                                       \
                if ((_r) & 9)                                                       \
                {                                                                   \
                    (_regs)->dxc = DXC_AFP_REGISTER;                                \
                    (_regs)->program_interrupt( (_regs), PGM_DATA_EXCEPTION );      \
                }                                                                   \
            }                                                                       \
        }

    /* Program check if r1 and r2 are not 0 or 4 */
    #define HFPODD2_CHECK(_r1,_r2,_regs)                                            \
                                                                                    \
        if (1                                                                       \
            && ARCH_370_IDX == sysblk.arch_mode                                     \
            && !FACILITY_ENABLED_ARCH( HERC_370_EXTENSION, ARCH_370_IDX )           \
        )                                                                           \
        {                                                                           \
            /* Normal 370 case: HERC_370_EXTENSION not enabled */                   \
            _370_HFPODD2_CHECK((_r1),(_r2),(_regs));                                \
        }                                                                           \
        else /* 390 or zArch, -OR- 370 with HERC_370_EXTENSION enabled */           \
        {                                                                           \
            if (0                                                                   \
                || ((_r1) & 2)                                                      \
                || ((_r2) & 2)                                                      \
            )                                                                       \
            {                                                                       \
                (_regs)->program_interrupt( (_regs), PGM_SPECIFICATION_EXCEPTION ); \
            }                                                                       \
            else if (!((_regs)->CR(0) & CR0_AFP) )                                  \
            {                                                                       \
                if (0                                                               \
                    || ((_r1) & 9)                                                  \
                    || ((_r2) & 9)                                                  \
                )                                                                   \
                {                                                                   \
                    (_regs)->dxc = DXC_AFP_REGISTER;                                \
                    (_regs)->program_interrupt( (_regs), PGM_DATA_EXCEPTION );      \
                }                                                                   \
            }                                                                       \
        }

  #endif /* !defined( _FEATURE_SIE ) */

#endif /* !defined( FEATURE_BASIC_FP_EXTENSIONS ) */

/*-------------------------------------------------------------------*/
/*               (end floating-point helper macros)                  */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/*        Special FPR2I/FPREX handling for HERC_370_EXTENSION        */
/*-------------------------------------------------------------------*/
#undef FPR2I
#undef FPREX
#if defined( FEATURE_BASIC_FP_EXTENSIONS )

  // 390 or zArch -OR- 370 with the "FEATURE_370_EXTENSION" BUILD
  // option #defined, which causes the "FEATURE_BASIC_FP_EXTENSIONS"
  // BUILD to necessarily ALSO be #defined, forcing us to check if
  // the "HERC_370_EXTENSION" facility is enabled or not.
  //
  // PLEASE NOTE that enabling the "HERC_370_EXTENSION" facility
  // causes nonconformant/incorrect S/370 floating point behavior!

  #define FPR2I(_r)     /* Convert fpr to index */                    \
  ((0                                                                 \
      || ARCH_370_IDX != sysblk.arch_mode                             \
      || FACILITY_ENABLED_ARCH( HERC_370_EXTENSION, ARCH_370_IDX )    \
  )                                                                   \
  ? ((_r) << 1) : (_r))

  #define FPREX         /* Offset of extended register */             \
  ((0                                                                 \
      || ARCH_370_IDX != sysblk.arch_mode                             \
      || FACILITY_ENABLED_ARCH( HERC_370_EXTENSION, ARCH_370_IDX )    \
  )                                                                   \
  ? 4 : 2 )

#else // !defined( FEATURE_BASIC_FP_EXTENSIONS )

  // S/370 without the "FEATURE_370_EXTENSION" BUILD option defined,
  // which causes the "FEATURE_BASIC_FP_EXTENSIONS" BUILD option to
  // NOT be #defined, which provides normal S/370 behavior.

  #define FPR2I(_r)     (_r)        /* Convert fpr to index */
  #define FPREX           2         /* Offset of extended register */

#endif // defined( FEATURE_BASIC_FP_EXTENSIONS)
/*-------------------------------------------------------------------*/
/*     (end special fpr2i/fprex handling for herc_370_extension)     */
/*-------------------------------------------------------------------*/



#define TLBIX(_addr) (((VADR_L)(_addr) >> TLB_PAGESHIFT) & TLB_MASK)

#define MAINADDR(_main, _addr) \
   (BYTE*)((uintptr_t)(_main) ^ (uintptr_t)(_addr))

#define NEW_MAINADDR(_regs, _addr, _aaddr) \
   (BYTE*)((uintptr_t)((_regs)->mainstor \
         + (uintptr_t)(_aaddr)) \
         ^ (uintptr_t)((_addr) & TLB_PAGEMASK))

#define MAIN_TO_ABS(_main)  ((U64)((BYTE*)(_main) - sysblk.mainstor))


/*-------------------------------------------------------------------*/
/*        Perform invalidation after storage key update...           */
/*-------------------------------------------------------------------*/
/*                                                                   */
/*  If the REF or CHANGE bit is turned off for an absolute address,  */
/*  then we need to invalidate any cached entries for that address   */
/*  on *ALL* CPUs.                                                   */
/*                                                                   */
/*  FIXME: Synchronization, esp. for the CHANGE bit, should be       */
/*  tighter than what is provided here.                              */
/*                                                                   */
/*-------------------------------------------------------------------*/
#define STORKEY_INVALIDATE_LOCKED( _regs, _n )                          \
                                                                        \
 do                                                                     \
 {                                                                      \
   BYTE* abs = (_regs)->mainstor + ((_n) & PAGEFRAME_PAGEMASK);         \
                                                                        \
   /* Do it for the current CPU first */                                \
   ARCH_DEP( invalidate_tlbe )( (_regs), abs );                         \
                                                                        \
   if (sysblk.cpus > 1)                                                 \
   {                                                                    \
     int cpu;        /* CPU being examined */                           \
     REGS* cregs;    /* register context for CPU being examined */      \
                                                                        \
     /* Do invalidate for all of the other online CPUs too */           \
     for (cpu=0; cpu < sysblk.hicpu; cpu++)                             \
     {                                                                  \
       /* Skip our own CPU and CPUs which aren't online */              \
       if (IS_CPU_ONLINE( cpu ) && cpu != (_regs)->cpuad)               \
       {                                                                \
         cregs = sysblk.regs[cpu];                                      \
                                                                        \
         /* Is this CPU waiting for an interrupt? */                    \
         if (sysblk.waiting_mask & CPU_BIT( cpu ))                      \
         {                                                              \
           /* Yes, then we can do the invalidate right now... */        \
           switch (cregs->arch_mode)                                    \
           {                                                            \
             case ARCH_370_IDX:                                         \
             {                                                          \
               abs = cregs->mainstor + ((_n) & PAGEFRAME_370_PAGEMASK); \
               s370_invalidate_tlbe( cregs, abs );                      \
               break;                                                   \
             }                                                          \
             case ARCH_390_IDX:                                         \
             {                                                          \
               abs = cregs->mainstor + ((_n) & PAGEFRAME_390_PAGEMASK); \
               s390_invalidate_tlbe( cregs, abs );                      \
               break;                                                   \
             }                                                          \
             case ARCH_900_IDX:                                         \
             {                                                          \
               abs = cregs->mainstor + ((_n) & PAGEFRAME_900_PAGEMASK); \
               z900_invalidate_tlbe( cregs, abs );                      \
               break;                                                   \
             }                                                          \
             default:                                                   \
             {                                                          \
               CRASH();                                                 \
             }                                                          \
           }                                                            \
         }                                                              \
         else /* Otherwise we need to schedule it ... */                \
         {                                                              \
           ON_IC_INTERRUPT( cregs );                                    \
                                                                        \
           if (!cregs->invalidate)                                      \
           {                                                            \
             cregs->invalidate = 1;                                     \
             cregs->invalidate_main = abs;                              \
           }                                                            \
           else                                                         \
           {                                                            \
             cregs->invalidate_main = NULL;                             \
           }                                                            \
         }                                                              \
       }                                                                \
     }                                                                  \
   }                                                                    \
 }                                                                      \
 while (0)

#define STORKEY_INVALIDATE( _regs, _n )                                 \
 do                                                                     \
 {                                                                      \
   OBTAIN_INTLOCK( (_regs) );                                           \
   {                                                                    \
      STORKEY_INVALIDATE_LOCKED( (_regs), (_n) );                       \
   }                                                                    \
   RELEASE_INTLOCK( (_regs) );                                          \
 }                                                                      \
 while (0)

#if defined( INLINE_STORE_FETCH_ADDR_CHECK )
 #define FETCH_MAIN_ABSOLUTE(_addr, _regs, _len) \
  ARCH_DEP( fetch_main_absolute )((_addr), (_regs), (_len))
#else
 #define FETCH_MAIN_ABSOLUTE(_addr, _regs, _len) \
  ARCH_DEP( fetch_main_absolute )((_addr), (_regs))
#endif

/*-------------------------------------------------------------------*/
/*      Transactional-Execution Facility (TXF) support macros        */
/*-------------------------------------------------------------------*/

#undef TXFC_INSTR_CHECK                     /* 'TXFC' == Constrained */
#undef TXFC_INSTR_CHECK_IP                  /* 'TXFC' == Constrained */
#undef TXFC_BRANCH_CHECK_IP                 /* 'TXFC' == Constrained */
#undef TXFC_RELATIVE_BRANCH_CHECK_IP        /* 'TXFC' == Constrained */

#undef TXF_INSTR_CHECK
#undef TXF_FLOAT_INSTR_CHECK
#undef TXF_ACCESS_INSTR_CHECK
#undef TXF_NONRELATIVE_BRANCH_CHECK_IP
#undef TXF_BRANCH_SET_MODE_CHECK_IP
#undef TXF_SET_ADDRESSING_MODE_CHECK
#undef TXF_MISC_INSTR_CHECK
#undef TXF_EXECUTE_INSTR_CHECK
#undef TXF_ALLOCMAP
#undef TXF_FREEMAP
#undef TXF_MADDRL

#if !defined( FEATURE_073_TRANSACT_EXEC_FACILITY )

  #define TXFC_INSTR_CHECK( _regs )
  #define TXFC_INSTR_CHECK_IP( _regs )
  #define TXFC_BRANCH_CHECK_IP( _regs, _m3, _i4 )
  #define TXFC_RELATIVE_BRANCH_CHECK_IP( _regs )

  #define TXF_INSTR_CHECK( _regs )
  #define TXF_FLOAT_INSTR_CHECK( _regs )
  #define TXF_ACCESS_INSTR_CHECK( _regs )
  #define TXF_MISC_INSTR_CHECK( _regs )
  #define TXF_NONRELATIVE_BRANCH_CHECK_IP( _regs, _r )
  #define TXF_BRANCH_SET_MODE_CHECK_IP( _regs, _r2 )
  #define TXF_SET_ADDRESSING_MODE_CHECK( _regs )
  #define TXF_EXECUTE_INSTR_CHECK( _regs )
  #define TXF_ALLOCMAP( _regs )
  #define TXF_FREEMAP( _regs )

  #define TXF_MADDRL( _vaddr, _len, _arn, _regs, _acctype, _maddr ) \
    /* Return the very same address as what was passed */ (_maddr)

#else /* defined( FEATURE_073_TRANSACT_EXEC_FACILITY ) */

  #define TXFC_INSTR_CHECK( _regs )                                                     \
    /* Restricted instruction in CONSTRAINED transaction mode */                        \
    do {                                                                                \
      if ((_regs)->txf_contran)                                                         \
      {                                                                                 \
        (_regs)->txf_why |= TXF_WHY_CONTRAN_INSTR;                                      \
        ABORT_TRANS( (_regs), ABORT_RETRY_PGMCHK, TAC_INSTR );                          \
      }                                                                                 \
    } while (0)

  #define TXFC_INSTR_CHECK_IP( _regs )                                                  \
    /* Restricted instruction in CONSTRAINED transaction mode */                        \
    do {                                                                                \
      if ((_regs)->txf_contran)                                                         \
      {                                                                                 \
        /* Since the instruction hasn't been decoded yet, regs->ip is still             \
           pointing to the instruction so we use NEGATIVE "ABORT_RETRY_PGMCHK".         \
           See the comments in the "abort_transaction" function in transact.c           \
        */                                                                              \
        (_regs)->txf_why |= TXF_WHY_CONTRAN_INSTR;                                      \
        ABORT_TRANS( (_regs), -ABORT_RETRY_PGMCHK, TAC_INSTR );                         \
      }                                                                                 \
    } while (0)

  #define TXFC_BRANCH_CHECK_IP( _regs, _m3, _i4 )                                       \
    /* Branches restricted in CONSTRAINED mode if mask zero or offset negative */       \
    do {                                                                                \
      if ((_regs)->txf_contran &&                                                       \
      (0                                                                                \
        || (_m3) == 0x00            /* zero mask (nop)   not allowed */                 \
        || (_i4) < 0                /* backward branches not allowed */                 \
      ))                                                                                \
      {                                                                                 \
        /* Since the instruction hasn't been decoded yet, regs->ip is still             \
           pointing to the instruction so we use NEGATIVE "ABORT_RETRY_PGMCHK".         \
           See the comments in the "abort_transaction" function in transact.c           \
        */                                                                              \
        (_regs)->txf_why |= TXF_WHY_CONTRAN_BRANCH;                                     \
        ABORT_TRANS( (_regs), -ABORT_RETRY_PGMCHK, TAC_INSTR );                         \
      }                                                                                 \
    } while (0)

  #define TXFC_RELATIVE_BRANCH_CHECK_IP( _regs )                                        \
    /* Relative branches restricted in CONSTRAINED mode */                              \
    /* if the mask is zero or the offset is negative    */                              \
    do {                                                                                \
      if ((_regs)->txf_contran &&                                                       \
      (0                                                                                \
        || (inst[1] & 0xf0) == 0x00                                                     \
        || (inst[2] & 0x80)                                                             \
      ))                                                                                \
      {                                                                                 \
        /* Since the instruction hasn't been decoded yet, regs->ip is still             \
           pointing to the instruction so we use NEGATIVE "ABORT_RETRY_PGMCHK".         \
           See the comments in the "abort_transaction" function in transact.c           \
        */                                                                              \
        (_regs)->txf_why |= TXF_WHY_CONTRAN_RELATIVE_BRANCH;                            \
        ABORT_TRANS( (_regs), -ABORT_RETRY_PGMCHK, TAC_INSTR );                         \
      }                                                                                 \
    } while (0)

  #define TXF_INSTR_CHECK( _regs )                                                      \
    /* Restricted instruction in any transaction mode */                                \
    do {                                                                                \
      if ((_regs)->txf_tnd)                                                             \
      {                                                                                 \
        (_regs)->txf_why |= TXF_WHY_TRAN_INSTR;                                         \
        ABORT_TRANS( (_regs), ABORT_RETRY_PGMCHK, TAC_INSTR );                          \
      }                                                                                 \
    } while (0)

  #define TXF_FLOAT_INSTR_CHECK( _regs )                                                \
    /* Restricted instruction if CONSTRAINED mode or float bit zero */                  \
    do {                                                                                \
      if (1                                                                             \
        && (_regs)->txf_tnd                                                             \
        && (0                                                                           \
          || (_regs)->txf_contran                                                       \
          || !((_regs)->txf_ctlflag & TXF_CTL_FLOAT)                                    \
        )                                                                               \
      )                                                                                 \
      {                                                                                 \
        (_regs)->txf_why |= TXF_WHY_TRAN_FLOAT_INSTR;                                   \
        ABORT_TRANS( (_regs), ABORT_RETRY_PGMCHK, TAC_INSTR );                          \
      }                                                                                 \
    } while (0)

  #define TXF_ACCESS_INSTR_CHECK( _regs )                                               \
    /* Restricted instruction if access control bit zero */                             \
    do {                                                                                \
      if (1                                                                             \
        && (_regs)->txf_tnd                                                             \
        && !((_regs)->txf_ctlflag & TXF_CTL_AR)                                         \
      )                                                                                 \
      {                                                                                 \
        (_regs)->txf_why |= TXF_WHY_TRAN_ACCESS_INSTR;                                  \
        ABORT_TRANS( (_regs), ABORT_RETRY_PGMCHK, TAC_INSTR );                          \
      }                                                                                 \
    } while (0)

  #define TXF_NONRELATIVE_BRANCH_CHECK_IP( _regs, _r )                                  \
    /* BALR/BASR/BASSM are restricted when the branch     */                            \
    /* register is non-zero and BRANCH tracing is enabled */                            \
    do {                                                                                \
      if (1                                                                             \
          && (_regs)->txf_tnd                                                           \
          && ((_r) != 0 && ((_regs)->CR(12) & CR12_BRTRACE))                            \
      )                                                                                 \
      {                                                                                 \
        /* Since the instruction hasn't been decoded yet, regs->ip is still             \
           pointing to the instruction so we use NEGATIVE "ABORT_RETRY_PGMCHK".         \
           See the comments in the "abort_transaction" function in transact.c           \
        */                                                                              \
        (_regs)->txf_why |= TXF_WHY_TRAN_NONRELATIVE_BRANCH;                            \
        ABORT_TRANS( (_regs), -ABORT_RETRY_PGMCHK, TAC_INSTR );                         \
      }                                                                                 \
    } while (0)

  #define TXF_BRANCH_SET_MODE_CHECK_IP( _regs, _r2 )                                    \
    /* BASSM/BSM are restricted if the r2 field */                                      \
    /* is non-zero and MODE tracing is enabled. */                                      \
    do {                                                                                \
      if (1                                                                             \
          && (_regs)->txf_tnd                                                           \
          && ((_r2) != 0 && ((_regs)->CR(12) & CR12_MTRACE))                            \
      )                                                                                 \
      {                                                                                 \
        /* Since the instruction hasn't been decoded yet, regs->ip is still             \
           pointing to the instruction so we use NEGATIVE "ABORT_RETRY_PGMCHK".         \
           See the comments in the "abort_transaction" function in transact.c           \
        */                                                                              \
        (_regs)->txf_why |= TXF_WHY_TRAN_BRANCH_SET_MODE;                               \
        ABORT_TRANS( (_regs), -ABORT_RETRY_PGMCHK, TAC_INSTR );                         \
      }                                                                                 \
    } while (0)

  #define TXF_SET_ADDRESSING_MODE_CHECK( _regs )                                        \
    /* SAM24/31/64 is restricted if mode tracing is enabled. */                         \
    do {                                                                                \
      if (1                                                                             \
          &&  (_regs)->txf_tnd                                                          \
          && ((_regs)->CR(12) & CR12_MTRACE)                                            \
      )                                                                                 \
      {                                                                                 \
        (_regs)->txf_why |= TXF_WHY_TRAN_SET_ADDRESSING_MODE;                           \
        ABORT_TRANS( (_regs), ABORT_RETRY_PGMCHK, TAC_INSTR );                          \
      }                                                                                 \
    } while (0)

  #define TXF_MISC_INSTR_CHECK( _regs )                                                 \
    /* Restricted instruction in any transaction mode */                                \
    do {                                                                                \
      if ((_regs)->txf_tnd)                                                             \
      {                                                                                 \
        (_regs)->txf_why |= TXF_WHY_TRAN_MISC_INSTR;                                    \
        ABORT_TRANS( (_regs), ABORT_RETRY_PGMCHK, TAC_MISC );                           \
      }                                                                                 \
    } while (0)

  #define TXF_EXECUTE_INSTR_CHECK( _regs )                                              \
    /* Most all TXF instructions cannot be executed */                                  \
    do {                                                                                \
      if ((_regs)->execflag)                                                            \
      {                                                                                 \
        ARCH_DEP( program_interrupt )( (_regs), PGM_EXECUTE_EXCEPTION );                \
      }                                                                                 \
    } while (0)

  #define TXF_MADDRL(   _vaddr,   _len,   _arn,   _regs,   _acctype,   _maddr  )        \
          txf_maddr_l( (_vaddr), (_len), (_arn), (_regs), (_acctype), (_maddr) )

  #define TXF_ALLOCMAP( _regs )     alloc_txfmap( _regs )
  #define TXF_FREEMAP( _regs )      free_txfmap( _regs )

#endif /* defined( FEATURE_073_TRANSACT_EXEC_FACILITY ) */

/*-------------------------------------------------------------------*/
/*                     Instruction decoders                          */
/*-------------------------------------------------------------------*/

#include "instfmts.h"     // (moved into separate #include header)

/*-------------------------------------------------------------------*/
/*                 SIE address translation macros                    */
/*-------------------------------------------------------------------*/

#undef SIE_LOGICAL_TO_ABS
#undef SIE_TRANSLATE_ADDR
#undef SIE_TRANSLATE

#if defined( _FEATURE_SIE )

  //-------------------------------------------------------------------
  //   SIE_LOGICAL_TO_ABS:   SIE host virt  -->  SIE host abs
  //   SIE_TRANSLATE_ADDR:   SIE host virt  -->  SIE host real; rc
  //-------------------------------------------------------------------

  // --------------------------------------------------------------------------------
  //          z/Arch running under z/VM, or ESA/390 running under VM/ESA
  // --------------------------------------------------------------------------------

  #if __GEN_ARCH == 900 || (__GEN_ARCH == 390 && !defined( _FEATURE_ZSIE ))

    #define SIE_LOGICAL_TO_ABS( _addr, _arn, _regs, _acctype, _akey )                   \
      (                                                                                 \
        ARCH_DEP( logical_to_main_l )( (_addr), (_arn), (_regs), (_acctype), (_akey), 1 ), \
        (_regs)->dat.aaddr   /* THIS IS THE ACTUAL VALUE THE MACRO RETURNS! */          \
      )

    #define SIE_TRANSLATE_ADDR( _addr, _arn, _regs, _acctype )                          \
                                                                                        \
        ARCH_DEP( translate_addr )( (_addr), (_arn), (_regs), (_acctype) )

  // --------------------------------------------------------------------------------
  //                      S/370 running under VM/ESA
  // --------------------------------------------------------------------------------

  #elif __GEN_ARCH == 370 && defined( _FEATURE_SIE )

    #define SIE_LOGICAL_TO_ABS( _addr, _arn, _regs, _acctype, _akey )                   \
      (                                                                                 \
        s390_logical_to_main_l( (_addr), (_arn), (_regs), (_acctype), (_akey), 1 ),     \
        (_regs)->dat.aaddr   /* THIS IS THE ACTUAL VALUE THE MACRO RETURNS! */          \
      )

    #define SIE_TRANSLATE_ADDR( _addr, _arn, _regs, _acctype )                          \
                                                                                        \
        s390_translate_addr( (_addr), (_arn), (_regs), (_acctype) )

  // --------------------------------------------------------------------------------
  //                      ESA/390 running under z/VM
  // --------------------------------------------------------------------------------

  #else /* __GEN_ARCH == 390 && defined( _FEATURE_ZSIE ) */

    #define SIE_LOGICAL_TO_ABS( _addr, _arn, _regs, _acctype, _akey )                   \
      (                                                                                 \
        ((ARCH_390_IDX == (_regs)->arch_mode)                                           \
        ? s390_logical_to_main_l( (_addr), (_arn), (_regs), (_acctype), (_akey), 1 )    \
        : z900_logical_to_main_l( (_addr), (_arn), (_regs), (_acctype), (_akey), 1 )),  \
        (_regs)->dat.aaddr   /* THIS IS THE ACTUAL VALUE THE MACRO RETURNS! */          \
      )

    #define SIE_TRANSLATE_ADDR( _addr, _arn, _regs, _acctype )                          \
      (                                                                                 \
        (ARCH_390_IDX == (_regs)->arch_mode)                                            \
        ? s390_translate_addr( (_addr), (_arn), (_regs), (_acctype) )                   \
        : z900_translate_addr( (_addr), (_arn), (_regs), (_acctype) )                   \
      )

  #endif // __GEN_ARCH == ...

  // --------------------------------------------------------------------------------
  //  SIE_TRANSLATE:  SIE guest abs  -->  SIE host abs   (or nop if not in SIE mode)
  // --------------------------------------------------------------------------------

  #define SIE_TRANSLATE( _addr, _acctype, _regs )                     \
                                                                      \
    do {                                                              \
      if (SIE_MODE( (_regs) ) && !(_regs)->sie_pref) /* SIE mode? */  \
        *(_addr) = SIE_LOGICAL_TO_ABS(  /* host virt --> host abs */  \
           (_regs)->sie_mso + *(_addr), /* guest abs is host virt */  \
           USE_PRIMARY_SPACE,           /* host primary virtual   */  \
           HOST(_regs),                 /* host register context  */  \
           (_acctype), 0 );             /* access type and skey   */  \
    } while (0)

#else // !defined( _FEATURE_SIE )

  #define SIE_TRANSLATE_ADDR( _addr, _arn, _regs, _acctype )
  #define SIE_LOGICAL_TO_ABS( _addr, _arn, _regs, _acctype, _akey )
  #define SIE_TRANSLATE( _addr, _acctype, _regs )

#endif // defined( _FEATURE_SIE )

/*-------------------------------------------------------------------*/
/*                     other SIE helper macros                       */
/*-------------------------------------------------------------------*/

#undef  SIE_INTERCEPT
#undef  SIE_XC_INTERCEPT
#undef  SIE_SET_VI

#if defined( _FEATURE_SIE )

  /*---------------------------------------------*/
  /*      SIE intercept if SIE mode              */
  /*---------------------------------------------*/
  #define SIE_INTERCEPT( _regs )                                      \
                                                                      \
    do {                                                              \
      if (SIE_MODE( _regs ))                                          \
        longjmp( (_regs)->progjmp, SIE_INTERCEPT_INST );              \
    }                                                                 \
    while (0)

  /*---------------------------------------------*/
  /*        SIE intercept if XC mode guest       */
  /*---------------------------------------------*/
  #if defined( FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE )
    #define SIE_XC_INTERCEPT( _regs )                                 \
                                                                      \
      do {                                                            \
        if (SIE_MODE( _regs ) && SIE_STATE_BIT_ON( (_regs), MX, XC )) \
          longjmp( (_regs)->progjmp, SIE_INTERCEPT_INST );            \
      }                                                               \
      while (0)
  #else
    #define SIE_XC_INTERCEPT( _regs )
  #endif

  /*---------------------------------------------*/
  /*     Set SIE Validity Intercept fields       */
  /*---------------------------------------------*/
  #define SIE_SET_VI( _who, _when, _why, _regs )                      \
    do {                                                              \
                   (_regs)->siebk->vi_who  = (_who);                  \
                   (_regs)->siebk->vi_when = (_when);                 \
        STORE_HW(  (_regs)->siebk->vi_why,   (_why)   );              \
        memset(    (_regs)->siebk->vi_zero, 0, 6 );                   \
    }                                                                 \
    while (0)

#else // !defined( _FEATURE_SIE )

  #define SIE_INTERCEPT( _regs )
  #define SIE_XC_INTERCEPT( _regs )
  #define SIE_SET_VI( _who, _when, _why, _regs )

#endif // defined( _FEATURE_SIE )

/*-------------------------------------------------------------------*/
/*                  Instruction serialization                        */
/*-------------------------------------------------------------------*/

#if defined( OPTION_HARDWARE_SYNC_ALL )
  #define PERFORM_SERIALIZATION( _regs )    HARDWARE_SYNC()
  #define PERFORM_CHKPT_SYNC( _regs )       do{}while(0)
#else
  #define PERFORM_SERIALIZATION( _regs )    do{}while(0)
  #define PERFORM_CHKPT_SYNC( _regs )       do{}while(0)
#endif

/*-------------------------------------------------------------------*/
/*               External function declarations                      */
/*-------------------------------------------------------------------*/

/* Functions in module channel.c */
int  ARCH_DEP( startio ) (REGS *regs, DEVBLK *dev, ORB *orb);

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

int  ARCH_DEP( present_io_interrupt ) (REGS *regs, U32 *ioid, U32 *ioparm, U32 *iointid, BYTE *csw, DEVBLK** pdev );
int  ARCH_DEP( present_zone_io_interrupt ) (U32 *ioid, U32 *ioparm, U32 *iointid, BYTE zone);

void io_reset (void);
int  chp_reset(BYTE chpid, int solicited);
void channelset_reset(REGS *regs);


/* Functions in module cpu.c */
/* define all arch_load|store_psw */
/* regardless of current architecture (if any)
*/
#if defined( _370 )
void s370_store_psw (REGS *regs, BYTE *addr);
int  s370_load_psw (REGS *regs, BYTE *addr);
void s370_process_trace( REGS* regs, BYTE* dest );
#endif

#if defined( _390 )
int  s390_load_psw (REGS *regs, BYTE *addr);
void s390_store_psw (REGS *regs, BYTE *addr);
void s390_process_trace( REGS* regs, BYTE* dest );
#endif

#if defined( _900 )
int  z900_load_psw (REGS *regs, BYTE *addr);
void z900_store_psw (REGS *regs, BYTE *addr);
void z900_process_trace( REGS* regs, BYTE* dest );
#endif

int cpu_init (int cpu, REGS *regs, REGS *hostregs);
void ARCH_DEP( perform_io_interrupt ) (REGS *regs);
void ARCH_DEP( checkstop_all_cpus )( REGS* regs );
U64 make_psw64( REGS* regs, int arch /*370/390/900*/, bool bc );

#if defined( FEATURE_PER3 )
CPU_DLL_IMPORT void ARCH_DEP( Set_BEAR_Reg )( U64* bear, REGS* regs, BYTE* ip );
#endif

CPU_DLL_IMPORT void ARCH_DEP( SuccessfulBranch          )( REGS* regs, VADR vaddr );
CPU_DLL_IMPORT void ARCH_DEP( SuccessfulRelativeBranch  )( REGS* regs, S64 offset );
CPU_DLL_IMPORT int  ARCH_DEP( fix_program_interrupt_PSW )( REGS* regs );
CPU_DLL_IMPORT void ARCH_DEP( trace_program_interrupt   )( REGS* regs, int pcode, int ilc );

void *cpu_thread (void *cpu);
CPU_DLL_IMPORT void copy_psw (REGS *regs, BYTE *addr);
int   display_psw(                 REGS* regs, char* buf, int buflen );
char* str_psw(                     REGS* regs, char* buf, int buflen );
char* str_arch_psw( int arch_mode, REGS* regs, char* buf, int buflen );
// (helper macros for use with char arrays. DON'T USE IF BUF IS CHAR POINTER!)
#define DISPLAY_PSW(         _regs, _buf ) display_psw  (          (_regs), (_buf), (int) sizeof( _buf ))
#define STR_PSW(             _regs, _buf ) str_psw      (          (_regs), (_buf), (int) sizeof( _buf ))
#define STR_ARCH_PSW( _arch, _regs, _buf ) str_arch_psw ( (_arch), (_regs), (_buf), (int) sizeof( _buf ))

#define DO_AUTOMATIC_TRACING() if (sysblk.auto_trace_amt) do_automatic_tracing();
void do_automatic_tracing();


/* Functions in module vm.c */
int  ARCH_DEP( diag_devtype )      (     int r1, int r2, REGS *regs);
int  ARCH_DEP( syncblk_io )        (     int r1, int r2, REGS *regs);
int  ARCH_DEP( syncgen_io )        (     int r1, int r2, REGS *regs);
void ARCH_DEP( extid_call )        (     int r1, int r2, REGS *regs);
int  ARCH_DEP( cpcmd_call )        (     int r1, int r2, REGS *regs);
int  ARCH_DEP( diag_ppagerel )     (     int r1, int r2, REGS *regs);
void ARCH_DEP( vm_info )           (     int r1, int r2, REGS *regs);
int  ARCH_DEP( device_info )       (     int r1, int r2, REGS *regs);
void ARCH_DEP( access_reipl_data ) (     int r1, int r2, REGS *regs);
void ARCH_DEP( pseudo_timer ) (U32 code, int r1, int r2, REGS *regs);


/* Functions in module vmd250.c */
int  ARCH_DEP( vm_blockio ) (int r1, int r2, REGS *regs);


/* Functions in module control.c */
void ARCH_DEP( load_real_address_proc ) (REGS *regs,
    int r1, int b2, VADR effective_addr2);


/* Functions in module decimal.c */
void packed_to_binary (BYTE *dec, int len, U64 *result,
    int *ovf, int *dxf);
void binary_to_packed (S64 bin, BYTE *result);


/* Functions in module diagnose.c */
void ARCH_DEP( diagnose_call )( REGS* regs, int r1, int r3, int b2, VADR effective_addr2 );


/* Functions in module diagmssf.c */
void ARCH_DEP( scpend_call ) (void);
int  ARCH_DEP( mssf_call )    (int r1, int r2, REGS *regs);
void ARCH_DEP( diag204_call ) (int r1, int r2, REGS *regs);
void ARCH_DEP( diag224_call ) (int r1, int r2, REGS *regs);


/* Functions in module external.c */
void ARCH_DEP( perform_external_interrupt ) (REGS *regs);
void ARCH_DEP( store_status ) (REGS *ssreg, RADR aaddr);
void store_status (REGS *ssreg, U64 aaddr);


/* Function in module hdiagf18.c */
void ARCH_DEP( diagf18_call ) (int r1, int r2, REGS *regs);

/* Functions in module ipl.c */
int           load_ipl             (U16 lcss, U16  devnum, int cpu, int clear);
int ARCH_DEP( load_ipl )           (U16 lcss, U16  devnum, int cpu, int clear);
int           system_reset         ( const int target_mode, const bool clear, const bool ipl, const int cpu );
int ARCH_DEP( system_reset )       ( const int target_mode, const bool clear, const bool ipl, const int cpu );
int           cpu_reset            (REGS *regs);
int ARCH_DEP( cpu_reset )          (REGS *regs);
void          initial_cpu_reset_all();
int           initial_cpu_reset    (REGS *regs);
int ARCH_DEP( initial_cpu_reset )  (REGS *regs);
int ARCH_DEP( common_load_begin )  (int cpu, int clear);
int ARCH_DEP( common_load_finish ) (REGS *regs);

void storage_clear(void);
void xstorage_clear(void);


/* Functions in module scedasd.c */
void          set_sce_dir            (char *path);
char         *get_sce_dir            ();
int           load_main              (char *fname, RADR startloc, int noisy );
int ARCH_DEP( load_main )            (char *fname, RADR startloc, int noisy );
int           load_hmc               (char *fname, int cpu, int clear);
int ARCH_DEP( load_hmc )             (char *fname, int cpu, int clear);
void ARCH_DEP( sclp_scedio_request ) (SCCB_HEADER *);
void ARCH_DEP( sclp_scedio_event )   (SCCB_HEADER *);


/* Functions in module scescsi.c */
void ARCH_DEP( sclp_hwl_request         ) (SCCB_HEADER *);
void ARCH_DEP( sclp_hwl_event           ) (SCCB_HEADER *);
void ARCH_DEP( sclp_sdias_request       ) (SCCB_HEADER *);
void ARCH_DEP( sclp_sdias_event         ) (SCCB_HEADER *);
void ARCH_DEP( sdias_store_status_clear ) (REGS *);
void ARCH_DEP( sdias_store_status       ) (REGS *);

int support_boot (DEVBLK *);
int load_boot (DEVBLK *, int, int, int);

int hwldr_cmd (int, char **, char *);
int lddev_cmd (int, char **, char *);


/* Functions in module machchk.c */
int  ARCH_DEP( present_mck_interrupt ) (REGS *regs, U64 *mcic, U32 *xdmg, RADR *fsta);
void machine_check_crwpend (void);
void build_attach_chrpt( DEVBLK *dev );
void build_detach_chrpt( DEVBLK *dev );
void build_chp_reset_chrpt( BYTE chpid, int solicited, int found );
int  queue_channel_report( U32* crwarray, U32 crwcount );
U32  get_next_channel_report_word( REGS * );


/* Functions in module opcode.c */
void init_runtime_opcode_tables();
void init_regs_runtime_opcode_pointers( REGS* regs );


/* Functions in module hscmisc.c */
void ARCH_DEP( display_inst )       ( REGS* regs, BYTE* inst );
void ARCH_DEP( display_pgmint_inst )( REGS* regs, BYTE* inst );


/* Functions in module sie.c */
void ARCH_DEP( sie_exit ) (REGS *regs, int code);
void ARCH_DEP( diagnose_002 ) (REGS *regs, int r1, int r3);


/* Functions in module stack.c */
void ARCH_DEP( trap_x ) (int trap_is_trap4, REGS *regs, U32 trap_operand);
void ARCH_DEP( form_stack_entry ) (BYTE etype, VADR retna, VADR calla,
    U32 csi, U32 pcnum, REGS *regs);
VADR ARCH_DEP( locate_stack_entry ) (int prinst, LSED *lsedptr,
    REGS *regs);
void ARCH_DEP( stack_modify ) (VADR lsea, U32 m1, U32 m2, REGS *regs);
void ARCH_DEP( stack_extract ) (VADR lsea, int r1, int code, REGS *regs);
void ARCH_DEP( unstack_registers ) (int gtype, VADR lsea, int r1,
    int r2, REGS *regs);
int  ARCH_DEP( program_return_unstack ) (REGS *regs, RADR *lsedap, int *rc);


/* Functions in module trace.c */
CREG  ARCH_DEP( trace_br ) (int amode, VADR ia, REGS *regs);

#if defined( _FEATURE_ZSIE )
U32  s390_trace_br (int amode, U32 ia, REGS *regs);
#endif

CREG  ARCH_DEP( trace_bsg  ) (U32 alet, VADR ia, REGS *regs);
CREG  ARCH_DEP( trace_ssar ) (int ssair, U16 sasn, REGS *regs);
CREG  ARCH_DEP( trace_pc   ) (U32 pcea, REGS *regs);
CREG  ARCH_DEP( trace_pr   ) (REGS *newregs, REGS *regs);
CREG  ARCH_DEP( trace_pt   ) (int pti, U16 pasn, GREG gpr2, REGS *regs);
CREG  ARCH_DEP( trace_tr   ) (int r1, int r3, U32 op, REGS *regs);
CREG  ARCH_DEP( trace_tg   ) (int r1, int r3, U32 op, REGS *regs);
CREG  ARCH_DEP( trace_ms   ) (int br_ind, VADR ia, REGS *regs);


/* Functions in module plo.c */
int ARCH_DEP( plo_cl ) (int r1, int r3, VADR effective_addr2, int b2,
                            VADR effective_addr4, int b4,  REGS *regs);
int ARCH_DEP( plo_clg ) (int r1, int r3, VADR effective_addr2, int b2,
                            VADR effective_addr4, int b4,  REGS *regs);
int ARCH_DEP( plo_clgr ) (int r1, int r3, VADR effective_addr2, int b2,
                            VADR effective_addr4, int b4,  REGS *regs);
int ARCH_DEP( plo_clx ) (int r1, int r3, VADR effective_addr2, int b2,
                            VADR effective_addr4, int b4,  REGS *regs);
int ARCH_DEP( plo_cs ) (int r1, int r3, VADR effective_addr2, int b2,
                            VADR effective_addr4, int b4,  REGS *regs);
int ARCH_DEP( plo_csg ) (int r1, int r3, VADR effective_addr2, int b2,
                            VADR effective_addr4, int b4,  REGS *regs);
int ARCH_DEP( plo_csgr ) (int r1, int r3, VADR effective_addr2, int b2,
                            VADR effective_addr4, int b4,  REGS *regs);
int ARCH_DEP( plo_csx ) (int r1, int r3, VADR effective_addr2, int b2,
                            VADR effective_addr4, int b4,  REGS *regs);
int ARCH_DEP( plo_dcs ) (int r1, int r3, VADR effective_addr2, int b2,
                            VADR effective_addr4, int b4,  REGS *regs);
int ARCH_DEP( plo_dcsg ) (int r1, int r3, VADR effective_addr2, int b2,
                            VADR effective_addr4, int b4,  REGS *regs);
int ARCH_DEP( plo_dcsgr ) (int r1, int r3, VADR effective_addr2, int b2,
                            VADR effective_addr4, int b4,  REGS *regs);
int ARCH_DEP( plo_dcsx ) (int r1, int r3, VADR effective_addr2, int b2,
                            VADR effective_addr4, int b4,  REGS *regs);
int ARCH_DEP( plo_csst ) (int r1, int r3, VADR effective_addr2, int b2,
                            VADR effective_addr4, int b4,  REGS *regs);
int ARCH_DEP( plo_csstg ) (int r1, int r3, VADR effective_addr2, int b2,
                            VADR effective_addr4, int b4,  REGS *regs);
int ARCH_DEP( plo_csstgr ) (int r1, int r3, VADR effective_addr2, int b2,
                            VADR effective_addr4, int b4,  REGS *regs);
int ARCH_DEP( plo_csstx ) (int r1, int r3, VADR effective_addr2, int b2,
                            VADR effective_addr4, int b4,  REGS *regs);
int ARCH_DEP( plo_csdst ) (int r1, int r3, VADR effective_addr2, int b2,
                            VADR effective_addr4, int b4,  REGS *regs);
int ARCH_DEP( plo_csdstg ) (int r1, int r3, VADR effective_addr2, int b2,
                            VADR effective_addr4, int b4,  REGS *regs);
int ARCH_DEP( plo_csdstgr ) (int r1, int r3, VADR effective_addr2, int b2,
                            VADR effective_addr4, int b4,  REGS *regs);
int ARCH_DEP( plo_csdstx ) (int r1, int r3, VADR effective_addr2, int b2,
                            VADR effective_addr4, int b4,  REGS *regs);
int ARCH_DEP( plo_cstst ) (int r1, int r3, VADR effective_addr2, int b2,
                            VADR effective_addr4, int b4,  REGS *regs);
int ARCH_DEP( plo_cststg ) (int r1, int r3, VADR effective_addr2, int b2,
                            VADR effective_addr4, int b4,  REGS *regs);
int ARCH_DEP( plo_cststgr ) (int r1, int r3, VADR effective_addr2, int b2,
                            VADR effective_addr4, int b4,  REGS *regs);
int ARCH_DEP( plo_cststx ) (int r1, int r3, VADR effective_addr2, int b2,
                            VADR effective_addr4, int b4,  REGS *regs);


/*-------------------------------------------------------------------*/
/*                         DEF_INST                                  */
/*                   (Define Instructions)                           */
/*-------------------------------------------------------------------*/
/* From this point onward to the end of file are all of the DEF_INST */
/* statements (possibly guarded with #if defined feature tests) that */
/* define all instructions currently supported by Hercules. For ease */
/* of maintainability please try to keep all of the below statements */
/* in FEATURE sequence.  It is also NOT necessary to document which  */
/* source file the code for the instruction exists in. A simple grep */
/* can determine that quite easily without cluttering this header.   */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/*        FEATUREs with STFL/STFLE facility bits defined             */
/*-------------------------------------------------------------------*/

#if defined( FEATURE_000_N3_INSTR_FACILITY )
DEF_INST( add_logical_carry );
DEF_INST( add_logical_carry_register );
DEF_INST( branch_relative_and_save_long );
DEF_INST( branch_relative_on_condition_long );
DEF_INST( divide_logical );
DEF_INST( divide_logical_register );
DEF_INST( extract_psw );
DEF_INST( load_address_relative_long );
DEF_INST( load_reversed );
DEF_INST( load_reversed_half );
DEF_INST( load_reversed_register );
DEF_INST( multiply_logical );
DEF_INST( multiply_logical_register );
DEF_INST( rotate_left_single_logical );
DEF_INST( set_addressing_mode_24 );
DEF_INST( set_addressing_mode_31 );
DEF_INST( store_facility_list );
DEF_INST( store_reversed );
DEF_INST( store_reversed_half );
DEF_INST( subtract_logical_borrow );
DEF_INST( subtract_logical_borrow_register );
DEF_INST( test_addressing_mode );
#endif

#if defined( FEATURE_003_DAT_ENHANCE_FACILITY_1 )
DEF_INST( compare_and_swap_and_purge_long );
DEF_INST( invalidate_dat_table_entry );
#endif

#if defined( FEATURE_006_ASN_LX_REUSE_FACILITY )
DEF_INST( extract_primary_asn_and_instance );
DEF_INST( extract_secondary_asn_and_instance );
DEF_INST( program_transfer_with_instance );
DEF_INST( set_secondary_asn_with_instance );
#endif

#if defined( FEATURE_007_STFL_EXTENDED_FACILITY )
DEF_INST( store_facility_list_extended );
#endif

#if defined( FEATURE_008_ENHANCED_DAT_FACILITY_1 )
DEF_INST( perform_frame_management_function );
#endif

#if defined( FEATURE_011_CONFIG_TOPOLOGY_FACILITY )
DEF_INST( perform_topology_function );
#endif

#if defined( FEATURE_016_EXT_TRANSL_FACILITY_2 )
DEF_INST( pack_ascii );
DEF_INST( pack_unicode );
DEF_INST( unpack_ascii );
DEF_INST( unpack_unicode );
DEF_INST( test_decimal );
DEF_INST( translate_two_to_two );
DEF_INST( translate_two_to_one );
DEF_INST( translate_one_to_two );
DEF_INST( translate_one_to_one );
DEF_INST( move_long_unicode );
DEF_INST( compare_logical_long_unicode );
#endif

#if defined( FEATURE_017_MSA_FACILITY )
DEF_INST( cipher_message );
DEF_INST( cipher_message_with_chaining );
DEF_INST( compute_intermediate_message_digest );
DEF_INST( compute_last_message_digest );
DEF_INST( compute_message_authentication_code );
#endif

#if defined( FEATURE_018_LONG_DISPL_INST_FACILITY )
DEF_INST( add_y );
DEF_INST( add_halfword_y );
DEF_INST( add_logical_y );
DEF_INST( and_immediate_y );
DEF_INST( and_y );
DEF_INST( compare_y );
DEF_INST( compare_and_swap_y );
DEF_INST( compare_double_and_swap_y );
DEF_INST( compare_halfword_y );
DEF_INST( compare_logical_y );
DEF_INST( compare_logical_immediate_y );
DEF_INST( compare_logical_characters_under_mask_y );
DEF_INST( convert_to_binary_y );
DEF_INST( convert_to_decimal_y );
DEF_INST( exclusive_or_immediate_y );
DEF_INST( exclusive_or_y );
DEF_INST( insert_character_y );
DEF_INST( insert_characters_under_mask_y );
DEF_INST( load_y );
DEF_INST( load_address_y );
DEF_INST( load_byte );
DEF_INST( load_byte_long );
DEF_INST( load_halfword_y );
DEF_INST( load_multiple_y );
DEF_INST( load_real_address_y );
DEF_INST( move_immediate_y );
DEF_INST( multiply_single_y );
DEF_INST( or_immediate_y );
DEF_INST( or_y );
DEF_INST( store_y );
DEF_INST( store_character_y );
DEF_INST( store_characters_under_mask_y );
DEF_INST( store_halfword_y );
DEF_INST( store_multiple_y );
DEF_INST( subtract_y );
DEF_INST( subtract_halfword_y );
DEF_INST( subtract_logical_y );
DEF_INST( test_under_mask_y );
#endif /* defined( FEATURE_018_LONG_DISPL_INST_FACILITY ) */

#if defined( FEATURE_018_LONG_DISPL_INST_FACILITY ) && defined( FEATURE_ACCESS_REGISTERS )
DEF_INST( load_access_multiple_y );
DEF_INST( store_access_multiple_y );
#endif

#if defined( FEATURE_018_LONG_DISPL_INST_FACILITY ) && defined( FEATURE_HEXADECIMAL_FLOATING_POINT )
DEF_INST( load_float_long_y );
DEF_INST( load_float_short_y );
DEF_INST( store_float_long_y );
DEF_INST( store_float_short_y );
#endif

#if defined( FEATURE_020_HFP_MULT_ADD_SUB_FACILITY )
DEF_INST( multiply_add_float_short_reg );
DEF_INST( multiply_add_float_long_reg );
DEF_INST( multiply_add_float_short );
DEF_INST( multiply_add_float_long );
DEF_INST( multiply_subtract_float_short_reg );
DEF_INST( multiply_subtract_float_long_reg );
DEF_INST( multiply_subtract_float_short );
DEF_INST( multiply_subtract_float_long );
#endif

#if defined( FEATURE_021_EXTENDED_IMMED_FACILITY )
DEF_INST( add_fullword_immediate );
DEF_INST( add_long_fullword_immediate );
DEF_INST( add_logical_fullword_immediate );
DEF_INST( add_logical_long_fullword_immediate );
DEF_INST( and_immediate_high_fullword );
DEF_INST( and_immediate_low_fullword );
DEF_INST( compare_fullword_immediate );
DEF_INST( compare_long_fullword_immediate );
DEF_INST( compare_logical_fullword_immediate );
DEF_INST( compare_logical_long_fullword_immediate );
DEF_INST( exclusive_or_immediate_high_fullword );
DEF_INST( exclusive_or_immediate_low_fullword );
DEF_INST( insert_immediate_high_fullword );
DEF_INST( insert_immediate_low_fullword );
DEF_INST( load_long_fullword_immediate );
DEF_INST( load_logical_immediate_high_fullword );
DEF_INST( load_logical_immediate_low_fullword );
DEF_INST( or_immediate_high_fullword );
DEF_INST( or_immediate_low_fullword );
DEF_INST( subtract_logical_fullword_immediate );
DEF_INST( subtract_logical_long_fullword_immediate );
DEF_INST( load_and_test );
DEF_INST( load_and_test_long );
DEF_INST( load_byte_register );
DEF_INST( load_long_byte_register );
DEF_INST( load_halfword_register );
DEF_INST( load_long_halfword_register );
DEF_INST( load_logical_character );
DEF_INST( load_logical_character_register );
DEF_INST( load_logical_long_character_register );
DEF_INST( load_logical_halfword );
DEF_INST( load_logical_halfword_register );
DEF_INST( load_logical_long_halfword_register );
DEF_INST( find_leftmost_one_long_register );
#endif /*defined( FEATURE_021_EXTENDED_IMMED_FACILITY )*/

#if defined( FEATURE_022_EXT_TRANSL_FACILITY_3 )
DEF_INST( convert_utf16_to_utf32 );
DEF_INST( convert_utf32_to_utf16 );
DEF_INST( convert_utf32_to_utf8 );
DEF_INST( convert_utf8_to_utf32 );
DEF_INST( search_string_unicode );
DEF_INST( translate_and_test_reverse );
#endif

#if defined( FEATURE_023_HFP_UNNORM_EXT_FACILITY )
DEF_INST( multiply_unnormal_float_long_to_ext_reg );
DEF_INST( multiply_unnormal_float_long_to_ext_low_reg );
DEF_INST( multiply_unnormal_float_long_to_ext_high_reg );
DEF_INST( multiply_add_unnormal_float_long_to_ext_reg );
DEF_INST( multiply_add_unnormal_float_long_to_ext_low_reg );
DEF_INST( multiply_add_unnormal_float_long_to_ext_high_reg );
DEF_INST( multiply_unnormal_float_long_to_ext );
DEF_INST( multiply_unnormal_float_long_to_ext_low );
DEF_INST( multiply_unnormal_float_long_to_ext_high );
DEF_INST( multiply_add_unnormal_float_long_to_ext );
DEF_INST( multiply_add_unnormal_float_long_to_ext_low );
DEF_INST( multiply_add_unnormal_float_long_to_ext_high );
#endif

#if defined( FEATURE_025_STORE_CLOCK_FAST_FACILITY )
DEF_INST( store_clock_fast );
#endif

#if defined( FEATURE_026_PARSING_ENHANCE_FACILITY )
DEF_INST( translate_and_test_extended );
DEF_INST( translate_and_test_reverse_extended );
#endif

#if defined( FEATURE_027_MVCOS_FACILITY )
DEF_INST( move_with_optional_specifications );
#endif

#if defined( FEATURE_028_TOD_CLOCK_STEER_FACILITY )
DEF_INST( perform_timing_facility_function );
#endif

#if defined( FEATURE_031_EXTRACT_CPU_TIME_FACILITY )
DEF_INST( extract_cpu_time );
#endif

#if defined( FEATURE_032_CSS_FACILITY )
DEF_INST( compare_and_swap_and_store );
#endif

#if defined( FEATURE_034_GEN_INST_EXTN_FACILITY )
DEF_INST( add_immediate_long_storage );
DEF_INST( add_immediate_storage );
DEF_INST( add_logical_with_signed_immediate );
DEF_INST( add_logical_with_signed_immediate_long );
DEF_INST( compare_and_branch_register );
DEF_INST( compare_and_branch_long_register );
DEF_INST( compare_and_branch_relative_register );
DEF_INST( compare_and_branch_relative_long_register );
DEF_INST( compare_and_trap_long_register );
DEF_INST( compare_and_trap_register );
DEF_INST( compare_halfword_immediate_halfword_storage );
DEF_INST( compare_halfword_immediate_long_storage );
DEF_INST( compare_halfword_immediate_storage );
DEF_INST( compare_halfword_long );
DEF_INST( compare_halfword_relative_long );
DEF_INST( compare_halfword_relative_long_long );
DEF_INST( compare_immediate_and_branch );
DEF_INST( compare_immediate_and_branch_long );
DEF_INST( compare_immediate_and_branch_relative );
DEF_INST( compare_immediate_and_branch_relative_long );
DEF_INST( compare_immediate_and_trap );
DEF_INST( compare_immediate_and_trap_long );
DEF_INST( compare_logical_and_branch_long_register );
DEF_INST( compare_logical_and_branch_register );
DEF_INST( compare_logical_and_branch_relative_long_register );
DEF_INST( compare_logical_and_branch_relative_register );
DEF_INST( compare_logical_and_trap_long_register );
DEF_INST( compare_logical_and_trap_register );
DEF_INST( compare_logical_immediate_and_branch );
DEF_INST( compare_logical_immediate_and_branch_long );
DEF_INST( compare_logical_immediate_and_branch_relative );
DEF_INST( compare_logical_immediate_and_branch_relative_long );
DEF_INST( compare_logical_immediate_and_trap_fullword );
DEF_INST( compare_logical_immediate_and_trap_long );
DEF_INST( compare_logical_immediate_fullword_storage );
DEF_INST( compare_logical_immediate_halfword_storage );
DEF_INST( compare_logical_immediate_long_storage );
DEF_INST( compare_logical_relative_long );
DEF_INST( compare_logical_relative_long_halfword );
DEF_INST( compare_logical_relative_long_long );
DEF_INST( compare_logical_relative_long_long_fullword );
DEF_INST( compare_logical_relative_long_long_halfword );
DEF_INST( compare_relative_long );
DEF_INST( compare_relative_long_long );
DEF_INST( compare_relative_long_long_fullword );
DEF_INST( extract_cpu_attribute );
DEF_INST( load_address_extended_y );
DEF_INST( load_and_test_long_fullword );
DEF_INST( load_halfword_relative_long );
DEF_INST( load_halfword_relative_long_long );
DEF_INST( load_logical_halfword_relative_long );
DEF_INST( load_logical_halfword_relative_long_long );
DEF_INST( load_logical_relative_long_long_fullword );
DEF_INST( load_relative_long );
DEF_INST( load_relative_long_long );
DEF_INST( load_relative_long_long_fullword );
DEF_INST( move_fullword_from_halfword_immediate );
DEF_INST( move_halfword_from_halfword_immediate );
DEF_INST( move_long_from_halfword_immediate );
DEF_INST( multiply_halfword_y );
DEF_INST( multiply_single_immediate_fullword );
DEF_INST( multiply_single_immediate_long_fullword );
DEF_INST( multiply_y );
DEF_INST( prefetch_data );
DEF_INST( prefetch_data_relative_long );
DEF_INST( rotate_then_and_selected_bits_long_reg );
DEF_INST( rotate_then_exclusive_or_selected_bits_long_reg );
DEF_INST( rotate_then_insert_selected_bits_long_reg );
DEF_INST( rotate_then_or_selected_bits_long_reg );
DEF_INST( store_halfword_relative_long );
DEF_INST( store_relative_long );
DEF_INST( store_relative_long_long );
#endif /*defined( FEATURE_034_GEN_INST_EXTN_FACILITY )*/

#if defined( FEATURE_035_EXECUTE_EXTN_FACILITY )
DEF_INST( execute_relative_long );
#endif

#if defined( FEATURE_037_FP_EXTENSION_FACILITY )
DEF_INST( convert_fix32_to_dfp_ext_reg );
DEF_INST( convert_fix32_to_dfp_long_reg );
DEF_INST( convert_u32_to_dfp_ext_reg );
DEF_INST( convert_u32_to_dfp_long_reg );
DEF_INST( convert_u64_to_dfp_ext_reg );
DEF_INST( convert_u64_to_dfp_long_reg );
DEF_INST( convert_dfp_ext_to_fix32_reg );
DEF_INST( convert_dfp_long_to_fix32_reg );
DEF_INST( convert_dfp_ext_to_u32_reg );
DEF_INST( convert_dfp_long_to_u32_reg );
DEF_INST( convert_dfp_ext_to_u64_reg );
DEF_INST( convert_dfp_long_to_u64_reg );
DEF_INST( convert_u32_to_bfp_ext_reg );
DEF_INST( convert_u32_to_bfp_long_reg );
DEF_INST( convert_u32_to_bfp_short_reg );
DEF_INST( convert_u64_to_bfp_ext_reg );
DEF_INST( convert_u64_to_bfp_long_reg );
DEF_INST( convert_u64_to_bfp_short_reg );
DEF_INST( convert_bfp_ext_to_u32_reg );
DEF_INST( convert_bfp_long_to_u32_reg );
DEF_INST( convert_bfp_short_to_u32_reg );
DEF_INST( convert_bfp_ext_to_u64_reg );
DEF_INST( convert_bfp_long_to_u64_reg );
DEF_INST( convert_bfp_short_to_u64_reg );
DEF_INST( set_bfp_rounding_mode_3bit );
#endif /* defined( FEATURE_037_FP_EXTENSION_FACILITY ) */

#if defined( FEATURE_040_LOAD_PROG_PARAM_FACILITY )
DEF_INST( load_program_parameter );
#endif

/*-------------------------------------------------------------------*/

#if defined( FEATURE_041_FPS_ENHANCEMENT_FACILITY )

 #if defined( FEATURE_041_DFP_ROUNDING_FACILITY )
  DEF_INST( set_dfp_rounding_mode );
 #endif

 #if defined( FEATURE_041_FPR_GR_TRANSFER_FACILITY )
  DEF_INST( load_fpr_from_gr_long_reg );
  DEF_INST( load_gr_from_fpr_long_reg );
 #endif

 #if defined( FEATURE_041_FPS_SIGN_HANDLING_FACILITY )
  DEF_INST( copy_sign_fpr_long_reg );
  DEF_INST( load_complement_fpr_long_reg );
  DEF_INST( load_negative_fpr_long_reg );
  DEF_INST( load_positive_fpr_long_reg );
 #endif

 #if defined( FEATURE_041_IEEE_EXCEPT_SIM_FACILITY )
  DEF_INST( load_fpc_and_signal );
  DEF_INST( set_fpc_and_signal );
 #endif

#endif /* defined( FEATURE_041_FPS_ENHANCEMENT_FACILITY ) */

/*-------------------------------------------------------------------*/

#if defined( FEATURE_042_DFP_FACILITY )
DEF_INST( add_dfp_ext_reg );
DEF_INST( add_dfp_long_reg );
DEF_INST( compare_dfp_ext_reg );
DEF_INST( compare_dfp_long_reg );
DEF_INST( compare_and_signal_dfp_ext_reg );
DEF_INST( compare_and_signal_dfp_long_reg );
DEF_INST( compare_exponent_dfp_ext_reg );
DEF_INST( compare_exponent_dfp_long_reg );
DEF_INST( convert_fix64_to_dfp_ext_reg );
DEF_INST( convert_fix64_to_dfp_long_reg );
DEF_INST( convert_sbcd128_to_dfp_ext_reg );
DEF_INST( convert_sbcd64_to_dfp_long_reg );
DEF_INST( convert_ubcd128_to_dfp_ext_reg );
DEF_INST( convert_ubcd64_to_dfp_long_reg );
DEF_INST( convert_dfp_ext_to_fix64_reg );
DEF_INST( convert_dfp_long_to_fix64_reg );
DEF_INST( convert_dfp_ext_to_sbcd128_reg );
DEF_INST( convert_dfp_long_to_sbcd64_reg );
DEF_INST( convert_dfp_ext_to_ubcd128_reg );
DEF_INST( convert_dfp_long_to_ubcd64_reg );
DEF_INST( divide_dfp_ext_reg );
DEF_INST( divide_dfp_long_reg );
DEF_INST( extract_biased_exponent_dfp_ext_to_fix64_reg );
DEF_INST( extract_biased_exponent_dfp_long_to_fix64_reg );
DEF_INST( extract_significance_dfp_ext_reg );
DEF_INST( extract_significance_dfp_long_reg );
DEF_INST( insert_biased_exponent_fix64_to_dfp_ext_reg );
DEF_INST( insert_biased_exponent_fix64_to_dfp_long_reg );
DEF_INST( load_and_test_dfp_ext_reg );
DEF_INST( load_and_test_dfp_long_reg );
DEF_INST( load_fp_int_dfp_ext_reg );
DEF_INST( load_fp_int_dfp_long_reg );
DEF_INST( load_lengthened_dfp_long_to_ext_reg );
DEF_INST( load_lengthened_dfp_short_to_long_reg );
DEF_INST( load_rounded_dfp_ext_to_long_reg );
DEF_INST( load_rounded_dfp_long_to_short_reg );
DEF_INST( multiply_dfp_ext_reg );
DEF_INST( multiply_dfp_long_reg );
DEF_INST( quantize_dfp_ext_reg );
DEF_INST( quantize_dfp_long_reg );
DEF_INST( reround_dfp_ext_reg );
DEF_INST( reround_dfp_long_reg );
DEF_INST( shift_coefficient_left_dfp_ext );
DEF_INST( shift_coefficient_left_dfp_long );
DEF_INST( shift_coefficient_right_dfp_ext );
DEF_INST( shift_coefficient_right_dfp_long );
DEF_INST( subtract_dfp_ext_reg );
DEF_INST( subtract_dfp_long_reg );
DEF_INST( test_data_class_dfp_ext );
DEF_INST( test_data_class_dfp_long );
DEF_INST( test_data_class_dfp_short );
DEF_INST( test_data_group_dfp_ext );
DEF_INST( test_data_group_dfp_long );
DEF_INST( test_data_group_dfp_short );
#endif /*defined( FEATURE_042_DFP_FACILITY )*/

#if defined( FEATURE_044_PFPO_FACILITY )
DEF_INST( perform_floating_point_operation );
#endif

#if defined( FEATURE_045_DISTINCT_OPERANDS_FACILITY )
DEF_INST( add_distinct_register );
DEF_INST( add_distinct_long_register );
DEF_INST( add_distinct_halfword_immediate );
DEF_INST( add_distinct_long_halfword_immediate );
DEF_INST( add_logical_distinct_register );
DEF_INST( add_logical_distinct_long_register );
DEF_INST( add_logical_distinct_signed_halfword_immediate );
DEF_INST( add_logical_distinct_long_signed_halfword_immediate );
DEF_INST( and_distinct_register );
DEF_INST( and_distinct_long_register );
DEF_INST( exclusive_or_distinct_register );
DEF_INST( exclusive_or_distinct_long_register );
DEF_INST( or_distinct_register );
DEF_INST( or_distinct_long_register );
DEF_INST( shift_right_single_distinct );
DEF_INST( shift_left_single_distinct );
DEF_INST( shift_right_single_logical_distinct );
DEF_INST( shift_left_single_logical_distinct );
DEF_INST( subtract_distinct_register );
DEF_INST( subtract_distinct_long_register );
DEF_INST( subtract_logical_distinct_register );
DEF_INST( subtract_logical_distinct_long_register );
#endif /*defined( FEATURE_045_DISTINCT_OPERANDS_FACILITY )*/

#if defined( FEATURE_045_HIGH_WORD_FACILITY )
DEF_INST( add_high_high_high_register );
DEF_INST( add_high_high_low_register );
DEF_INST( add_high_immediate );
DEF_INST( add_logical_high_high_high_register );
DEF_INST( add_logical_high_high_low_register );
DEF_INST( add_logical_with_signed_immediate_high );
DEF_INST( add_logical_with_signed_immediate_high_n );
DEF_INST( branch_relative_on_count_high );
DEF_INST( compare_high_high_register );
DEF_INST( compare_high_low_register );
DEF_INST( compare_high_fullword );
DEF_INST( compare_high_immediate );
DEF_INST( compare_logical_high_high_register );
DEF_INST( compare_logical_high_low_register );
DEF_INST( compare_logical_high_fullword );
DEF_INST( compare_logical_high_immediate );
DEF_INST( load_byte_high );
DEF_INST( load_fullword_high );
DEF_INST( load_halfword_high );
DEF_INST( load_logical_character_high );
DEF_INST( load_logical_halfword_high );
DEF_INST( rotate_then_insert_selected_bits_high_long_reg );
DEF_INST( rotate_then_insert_selected_bits_low_long_reg );
DEF_INST( store_character_high );
DEF_INST( store_fullword_high );
DEF_INST( store_halfword_high );
DEF_INST( subtract_high_high_high_register );
DEF_INST( subtract_high_high_low_register );
DEF_INST( subtract_logical_high_high_high_register );
DEF_INST( subtract_logical_high_high_low_register );
#endif /*defined( FEATURE_045_HIGH_WORD_FACILITY )*/

#if defined( FEATURE_045_INTERLOCKED_ACCESS_FACILITY_1 )
DEF_INST( load_and_add );
DEF_INST( load_and_add_logical );
DEF_INST( load_and_and );
DEF_INST( load_and_exclusive_or );
DEF_INST( load_and_or );
DEF_INST( load_pair_disjoint );
DEF_INST( load_and_add_long );
DEF_INST( load_and_add_logical_long );
DEF_INST( load_and_and_long );
DEF_INST( load_and_exclusive_or_long );
DEF_INST( load_and_or_long );
DEF_INST( load_pair_disjoint_long );
#endif

#if defined( FEATURE_045_LOAD_STORE_ON_COND_FACILITY_1 )
DEF_INST( load_on_condition_register );
DEF_INST( load_on_condition_long_register );
DEF_INST( load_on_condition );
DEF_INST( load_on_condition_long );
DEF_INST( store_on_condition );
DEF_INST( store_on_condition_long );
#endif

#if defined( FEATURE_045_POPULATION_COUNT_FACILITY )
DEF_INST( population_count );
#endif

#if defined( FEATURE_048_DFP_ZONE_CONV_FACILITY )
DEF_INST( convert_zoned_to_dfp_ext );
DEF_INST( convert_zoned_to_dfp_long );
DEF_INST( convert_dfp_ext_to_zoned );
DEF_INST( convert_dfp_long_to_zoned );
#endif

#if defined( FEATURE_049_EXECUTION_HINT_FACILITY )
DEF_INST( branch_prediction_preload );
DEF_INST( branch_prediction_relative_preload );
DEF_INST( next_instruction_access_intent );
#endif

#if defined( FEATURE_049_LOAD_AND_TRAP_FACILITY )
DEF_INST( load_and_trap );
DEF_INST( load_long_and_trap );
DEF_INST( load_fullword_high_and_trap );
DEF_INST( load_logical_long_fullword_and_trap );
DEF_INST( load_logical_long_thirtyone_and_trap );
#endif

#if defined( FEATURE_049_MISC_INSTR_EXT_FACILITY_1 )
DEF_INST( compare_logical_and_trap );
DEF_INST( compare_logical_and_trap_long );
DEF_INST( rotate_then_insert_selected_bits_long_reg_n );
#endif

#if defined( FEATURE_049_PROCESSOR_ASSIST_FACILITY )
DEF_INST( perform_processor_assist );
#endif

#if defined( FEATURE_050_CONSTR_TRANSACT_FACILITY )
DEF_INST( transaction_begin_constrained );
#endif

#if defined( FEATURE_053_LOAD_STORE_ON_COND_FACILITY_2 )
DEF_INST( load_halfword_high_immediate_on_condition );
DEF_INST( load_halfword_immediate_on_condition );
DEF_INST( load_halfword_immediate_on_condition_grande );
DEF_INST( load_high_on_condition );
DEF_INST( load_high_on_condition_register );
DEF_INST( store_high_on_condition );
#endif

#if defined( FEATURE_053_LOAD_ZERO_RIGHTMOST_FACILITY )
DEF_INST( load_and_zero_rightmost_byte_grande );
DEF_INST( load_logical_and_zero_rightmost_byte );
DEF_INST( load_and_zero_rightmost_byte );
#endif

#if defined( FEATURE_058_MISC_INSTR_EXT_FACILITY_2 )
DEF_INST( branch_indirect_on_condition );
DEF_INST( add_long_halfword );
DEF_INST( subtract_long_halfword );
DEF_INST( multiply_long_register );
DEF_INST( multiply_long );
DEF_INST( multiply_long_halfword );
DEF_INST( multiply_single_register_cc );
DEF_INST( multiply_single_cc );
DEF_INST( multiply_single_long_register_cc );
DEF_INST( multiply_single_long_cc );
#endif

#if defined( FEATURE_061_MISC_INSTR_EXT_FACILITY_3 )
DEF_INST( and_register_with_complement );
DEF_INST( and_register_long_with_complement );
DEF_INST( nand_register );
DEF_INST( nand_register_long );
DEF_INST( not_xor_register );
DEF_INST( not_xor_register_long );
DEF_INST( nor_register );
DEF_INST( nor_register_long );
DEF_INST( or_register_with_complement );
DEF_INST( or_register_long_with_complement );
DEF_INST( select_register );
DEF_INST( select_register_long );
DEF_INST( select_fullword_high_register );
DEF_INST( move_right_to_left );
#endif

#if defined( FEATURE_066_RES_REF_BITS_MULT_FACILITY )
DEF_INST( reset_reference_bits_multiple );
#endif

#if defined( FEATURE_067_CPU_MEAS_COUNTER_FACILITY )
DEF_INST( extract_coprocessor_group_address );
DEF_INST( extract_cpu_counter );
DEF_INST( extract_peripheral_counter );
DEF_INST( load_cpu_counter_set_controls );
DEF_INST( load_peripheral_counter_set_controls );
DEF_INST( query_counter_information );
DEF_INST( set_cpu_counter );
DEF_INST( set_peripheral_counter );
#endif

#if defined( FEATURE_068_CPU_MEAS_SAMPLNG_FACILITY )
DEF_INST( load_sampling_controls );
DEF_INST( query_sampling_information );
#endif

#if defined( FEATURE_073_TRANSACT_EXEC_FACILITY)
DEF_INST( transaction_end );
DEF_INST( extract_transaction_nesting_depth );
DEF_INST( transaction_abort );
DEF_INST( nontransactional_store );
DEF_INST( transaction_begin );
#endif

#if defined( FEATURE_074_STORE_HYPER_INFO_FACILITY )
DEF_INST( store_hypervisor_information );
#endif

#if defined( FEATURE_076_MSA_EXTENSION_FACILITY_3 )
DEF_INST( perform_cryptographic_key_management_operation );
#endif

#if defined( FEATURE_077_MSA_EXTENSION_FACILITY_4 )
DEF_INST( perform_cryptographic_computation );
DEF_INST( cipher_message_with_cipher_feedback );
DEF_INST( cipher_message_with_output_feedback );
DEF_INST( cipher_message_with_counter );
#endif

#if defined( FEATURE_080_DFP_PACK_CONV_FACILITY )
DEF_INST(convert_packed_to_dfp_ext);
DEF_INST(convert_packed_to_dfp_long);
DEF_INST(convert_dfp_ext_to_packed);
DEF_INST(convert_dfp_long_to_packed);
#endif

#if defined( FEATURE_145_INS_REF_BITS_MULT_FACILITY )
DEF_INST( insert_reference_bits_multiple );
#endif

#if defined( FEATURE_193_BEAR_ENH_FACILITY )
DEF_INST( load_bear );
DEF_INST( store_bear );
DEF_INST( load_program_status_word_extended_y );
#endif

/*-------------------------------------------------------------------*/
/*      FEATUREs that DON'T have any facility bits defined           */
/*-------------------------------------------------------------------*/

#if defined( FEATURE_ACCESS_REGISTERS )
DEF_INST( purge_accesslist_lookaside_buffer );
DEF_INST( test_access );
DEF_INST( copy_access );
DEF_INST( extract_access_register );
DEF_INST( load_access_multiple );
DEF_INST( set_access_register );
DEF_INST( store_access_multiple );
#endif

#if defined( FEATURE_BASIC_STORAGE_KEYS )
DEF_INST( insert_storage_key );
DEF_INST( reset_reference_bit );
DEF_INST( set_storage_key );
#endif

#if defined( FEATURE_BIMODAL_ADDRESSING ) || defined( FEATURE_370_EXTENSION )
DEF_INST( branch_and_save_and_set_mode );
DEF_INST( branch_and_set_mode );
#endif

#if defined( FEATURE_BINARY_FLOATING_POINT )
DEF_INST( add_bfp_ext_reg );
DEF_INST( add_bfp_long );
DEF_INST( add_bfp_long_reg );
DEF_INST( add_bfp_short );
DEF_INST( add_bfp_short_reg );
DEF_INST( compare_and_signal_bfp_ext_reg );
DEF_INST( compare_and_signal_bfp_long );
DEF_INST( compare_and_signal_bfp_long_reg );
DEF_INST( compare_and_signal_bfp_short );
DEF_INST( compare_and_signal_bfp_short_reg );
DEF_INST( compare_bfp_ext_reg );
DEF_INST( compare_bfp_long );
DEF_INST( compare_bfp_long_reg );
DEF_INST( compare_bfp_short );
DEF_INST( compare_bfp_short_reg );
DEF_INST( convert_bfp_ext_to_fix32_reg );
DEF_INST( convert_bfp_ext_to_fix64_reg );
DEF_INST( convert_bfp_long_to_fix32_reg );
DEF_INST( convert_bfp_long_to_fix64_reg );
DEF_INST( convert_bfp_short_to_fix32_reg );
DEF_INST( convert_bfp_short_to_fix64_reg );
DEF_INST( convert_fix32_to_bfp_ext_reg );
DEF_INST( convert_fix32_to_bfp_long_reg );
DEF_INST( convert_fix32_to_bfp_short_reg );
DEF_INST( convert_fix64_to_bfp_ext_reg );
DEF_INST( convert_fix64_to_bfp_long_reg );
DEF_INST( convert_fix64_to_bfp_short_reg );
DEF_INST( divide_bfp_ext_reg );
DEF_INST( divide_bfp_long );
DEF_INST( divide_bfp_long_reg );
DEF_INST( divide_bfp_short );
DEF_INST( divide_bfp_short_reg );
DEF_INST( divide_integer_bfp_long_reg );
DEF_INST( divide_integer_bfp_short_reg );
DEF_INST( extract_fpc );
DEF_INST( load_and_test_bfp_ext_reg );
DEF_INST( load_and_test_bfp_long_reg );
DEF_INST( load_and_test_bfp_short_reg );
DEF_INST( load_complement_bfp_ext_reg );
DEF_INST( load_complement_bfp_long_reg );
DEF_INST( load_complement_bfp_short_reg );
DEF_INST( load_fp_int_bfp_ext_reg );
DEF_INST( load_fp_int_bfp_long_reg );
DEF_INST( load_fp_int_bfp_short_reg );
DEF_INST( load_fpc );
DEF_INST( load_lengthened_bfp_long_to_ext );
DEF_INST( load_lengthened_bfp_long_to_ext_reg );
DEF_INST( load_lengthened_bfp_short_to_ext );
DEF_INST( load_lengthened_bfp_short_to_ext_reg );
DEF_INST( load_lengthened_bfp_short_to_long );
DEF_INST( load_lengthened_bfp_short_to_long_reg );
DEF_INST( load_negative_bfp_ext_reg );
DEF_INST( load_negative_bfp_long_reg );
DEF_INST( load_negative_bfp_short_reg );
DEF_INST( load_positive_bfp_ext_reg );
DEF_INST( load_positive_bfp_long_reg );
DEF_INST( load_positive_bfp_short_reg );
DEF_INST( load_rounded_bfp_ext_to_long_reg );
DEF_INST( load_rounded_bfp_ext_to_short_reg );
DEF_INST( load_rounded_bfp_long_to_short_reg );
DEF_INST( multiply_add_bfp_long );
DEF_INST( multiply_add_bfp_long_reg );
DEF_INST( multiply_add_bfp_short );
DEF_INST( multiply_add_bfp_short_reg );
DEF_INST( multiply_bfp_ext_reg );
DEF_INST( multiply_bfp_long );
DEF_INST( multiply_bfp_long_reg );
DEF_INST( multiply_bfp_long_to_ext );
DEF_INST( multiply_bfp_long_to_ext_reg );
DEF_INST( multiply_bfp_short );
DEF_INST( multiply_bfp_short_reg );
DEF_INST( multiply_bfp_short_to_long );
DEF_INST( multiply_bfp_short_to_long_reg );
DEF_INST( multiply_subtract_bfp_long );
DEF_INST( multiply_subtract_bfp_long_reg );
DEF_INST( multiply_subtract_bfp_short );
DEF_INST( multiply_subtract_bfp_short_reg );
DEF_INST( set_bfp_rounding_mode_2bit );
DEF_INST( set_fpc );
DEF_INST( squareroot_bfp_ext_reg );
DEF_INST( squareroot_bfp_long );
DEF_INST( squareroot_bfp_long_reg );
DEF_INST( squareroot_bfp_short );
DEF_INST( squareroot_bfp_short_reg );
DEF_INST( store_fpc );
DEF_INST( subtract_bfp_ext_reg );
DEF_INST( subtract_bfp_long );
DEF_INST( subtract_bfp_long_reg );
DEF_INST( subtract_bfp_short );
DEF_INST( subtract_bfp_short_reg );
DEF_INST( test_data_class_bfp_ext );
DEF_INST( test_data_class_bfp_long );
DEF_INST( test_data_class_bfp_short );
#endif /*defined( FEATURE_BINARY_FLOATING_POINT )*/

#if defined( FEATURE_BRANCH_AND_SET_AUTHORITY )
DEF_INST( branch_and_set_authority );
#endif

#if defined( FEATURE_BROADCASTED_PURGING )
DEF_INST( compare_and_swap_and_purge );
#endif

#if defined( FEATURE_CANCEL_IO_FACILITY )
DEF_INST( cancel_subchannel );
#endif

#if defined( FEATURE_CHANNEL_SUBSYSTEM )
DEF_INST( clear_subchannel );
DEF_INST( halt_subchannel );
DEF_INST( modify_subchannel );
DEF_INST( resume_subchannel );
DEF_INST( set_address_limit );
DEF_INST( set_channel_monitor );
DEF_INST( reset_channel_path );
DEF_INST( start_subchannel );
DEF_INST( store_channel_path_status );
DEF_INST( store_channel_report_word );
DEF_INST( store_subchannel );
DEF_INST( test_pending_interruption );
DEF_INST( test_subchannel );
#endif /*defined( FEATURE_CHANNEL_SUBSYSTEM )*/

#if defined( FEATURE_CHANNEL_SWITCHING )
DEF_INST( connect_channel_set );
DEF_INST( disconnect_channel_set );
#endif

#if defined( FEATURE_CHECKSUM_INSTRUCTION )
DEF_INST( checksum );
#endif

#if defined( FEATURE_CHSC )
DEF_INST( channel_subsystem_call );
#endif

#if defined( FEATURE_CMPSC )
DEF_INST( cmpsc_2012 );
#endif

#if defined( FEATURE_COMPARE_AND_MOVE_EXTENDED )
DEF_INST( compare_logical_long_extended );
DEF_INST( move_long_extended );
#endif

#if defined( FEATURE_DAT_ENHANCEMENT_FACILITY_2 )
DEF_INST( load_page_table_entry_address );
#endif

#if defined( FEATURE_DUAL_ADDRESS_SPACE )
DEF_INST( extract_primary_asn );
DEF_INST( extract_secondary_asn );
DEF_INST( insert_address_space_control );
DEF_INST( insert_virtual_storage_key );
DEF_INST( load_address_space_parameters );
DEF_INST( move_to_primary );
DEF_INST( move_to_secondary );
DEF_INST( move_with_key );
DEF_INST( program_call );
DEF_INST( program_transfer );
DEF_INST( set_address_space_control );
DEF_INST( set_secondary_asn );
#endif

#if defined( FEATURE_ECPSVM )
DEF_INST( ecpsvm_basic_freex );
DEF_INST( ecpsvm_basic_fretx );
DEF_INST( ecpsvm_lock_page );
DEF_INST( ecpsvm_unlock_page );
DEF_INST( ecpsvm_decode_next_ccw );
DEF_INST( ecpsvm_free_ccwstor );
DEF_INST( ecpsvm_locate_vblock );
DEF_INST( ecpsvm_disp1 );
DEF_INST( ecpsvm_tpage );
DEF_INST( ecpsvm_tpage_lock );
DEF_INST( ecpsvm_inval_segtab );
DEF_INST( ecpsvm_inval_ptable );
DEF_INST( ecpsvm_decode_first_ccw );
DEF_INST( ecpsvm_dispatch_main );
DEF_INST( ecpsvm_locate_rblock );
DEF_INST( ecpsvm_comm_ccwproc );
DEF_INST( ecpsvm_unxlate_ccw );
DEF_INST( ecpsvm_disp2 );
DEF_INST( ecpsvm_store_level );
DEF_INST( ecpsvm_loc_chgshrpg );
DEF_INST( ecpsvm_extended_freex );
DEF_INST( ecpsvm_extended_fretx );
DEF_INST( ecpsvm_prefmach_assist );
#endif /*defined( FEATURE_ECPSVM )*/

#if defined( FEATURE_EMULATE_VM )
DEF_INST( inter_user_communication_vehicle );
#endif

#if defined( FEATURE_EXPANDED_STORAGE )
DEF_INST( page_in );
DEF_INST( page_out );
#endif

#if defined( FEATURE_EXTENDED_STORAGE_KEYS )
DEF_INST( insert_storage_key_extended );
DEF_INST( reset_reference_bit_extended );
DEF_INST( set_storage_key_extended );
#endif

#if defined( FEATURE_EXTENDED_TOD_CLOCK )
DEF_INST( set_clock_programmable_field );
DEF_INST( store_clock_extended );
#endif

#if defined( FEATURE_EXTENDED_TRANSLATION_FACILITY_1 )
DEF_INST( convert_utf16_to_utf8 );
DEF_INST( convert_utf8_to_utf16 );
DEF_INST( translate_extended );
#endif

#if defined( FEATURE_FPS_EXTENSIONS )
DEF_INST( convert_bfp_long_to_float_long_reg );
DEF_INST( convert_bfp_short_to_float_long_reg );
DEF_INST( convert_float_long_to_bfp_long_reg );
DEF_INST( convert_float_long_to_bfp_short_reg );
DEF_INST( load_float_ext_reg );
DEF_INST( load_zero_float_short_reg );
DEF_INST( load_zero_float_long_reg );
DEF_INST( load_zero_float_ext_reg );
#endif

#if defined( FEATURE_HEXADECIMAL_FLOATING_POINT )
DEF_INST( add_float_ext_reg );
DEF_INST( add_float_long );
DEF_INST( add_float_long_reg );
DEF_INST( add_float_short );
DEF_INST( add_float_short_reg );
DEF_INST( add_unnormal_float_long );
DEF_INST( add_unnormal_float_long_reg );
DEF_INST( add_unnormal_float_short );
DEF_INST( add_unnormal_float_short_reg );
DEF_INST( compare_float_long );
DEF_INST( compare_float_long_reg );
DEF_INST( compare_float_short );
DEF_INST( compare_float_short_reg );
DEF_INST( divide_float_ext_reg );
DEF_INST( divide_float_long );
DEF_INST( divide_float_long_reg );
DEF_INST( divide_float_short );
DEF_INST( divide_float_short_reg );
DEF_INST( halve_float_long_reg );
DEF_INST( halve_float_short_reg );
DEF_INST( load_and_test_float_long_reg );
DEF_INST( load_and_test_float_short_reg );
DEF_INST( load_complement_float_long_reg );
DEF_INST( load_complement_float_short_reg );
DEF_INST( load_float_long );
DEF_INST( load_float_long_reg );
DEF_INST( load_float_short );
DEF_INST( load_float_short_reg );
DEF_INST( load_negative_float_long_reg );
DEF_INST( load_negative_float_short_reg );
DEF_INST( load_positive_float_long_reg );
DEF_INST( load_positive_float_short_reg );
DEF_INST( load_rounded_float_long_reg );
DEF_INST( load_rounded_float_short_reg );
DEF_INST( multiply_float_ext_reg );
DEF_INST( multiply_float_long );
DEF_INST( multiply_float_long_reg );
DEF_INST( multiply_float_long_to_ext );
DEF_INST( multiply_float_long_to_ext_reg );
DEF_INST( multiply_float_short_to_long );
DEF_INST( multiply_float_short_to_long_reg );
DEF_INST( store_float_long );
DEF_INST( store_float_short );
DEF_INST( subtract_float_ext_reg );
DEF_INST( subtract_float_long );
DEF_INST( subtract_float_long_reg );
DEF_INST( subtract_float_short );
DEF_INST( subtract_float_short_reg );
DEF_INST( subtract_unnormal_float_long );
DEF_INST( subtract_unnormal_float_long_reg );
DEF_INST( subtract_unnormal_float_short );
DEF_INST( subtract_unnormal_float_short_reg );
#endif /* defined( FEATURE_HEXADECIMAL_FLOATING_POINT ) */

#if defined( FEATURE_HFP_EXTENSIONS )
DEF_INST( load_lengthened_float_short_to_long_reg );
DEF_INST( load_lengthened_float_long_to_ext_reg );
DEF_INST( load_lengthened_float_short_to_ext_reg );
DEF_INST( squareroot_float_ext_reg );
DEF_INST( multiply_float_short_reg );
DEF_INST( load_positive_float_ext_reg );
DEF_INST( load_negative_float_ext_reg );
DEF_INST( load_and_test_float_ext_reg );
DEF_INST( load_complement_float_ext_reg );
DEF_INST( load_rounded_float_ext_to_short_reg );
DEF_INST( load_fp_int_float_ext_reg );
DEF_INST( compare_float_ext_reg );
DEF_INST( load_fp_int_float_short_reg );
DEF_INST( load_fp_int_float_long_reg );
DEF_INST( convert_fixed_to_float_short_reg );
DEF_INST( convert_fixed_to_float_long_reg );
DEF_INST( convert_fixed_to_float_ext_reg );
DEF_INST( convert_float_short_to_fixed_reg );
DEF_INST( convert_float_long_to_fixed_reg );
DEF_INST( convert_float_ext_to_fixed_reg );
DEF_INST( load_lengthened_float_short_to_long );
DEF_INST( load_lengthened_float_long_to_ext );
DEF_INST( load_lengthened_float_short_to_ext );
DEF_INST( squareroot_float_short );
DEF_INST( squareroot_float_long );
DEF_INST( multiply_float_short );
#endif /*defined( FEATURE_HFP_EXTENSIONS )*/

#if defined( FEATURE_IMMEDIATE_AND_RELATIVE )
DEF_INST( add_halfword_immediate );
DEF_INST( branch_relative_and_save );
DEF_INST( branch_relative_on_condition );
DEF_INST( branch_relative_on_count );
DEF_INST( branch_relative_on_index_high );
DEF_INST( branch_relative_on_index_low_or_equal );
DEF_INST( compare_halfword_immediate );
DEF_INST( load_halfword_immediate );
DEF_INST( multiply_halfword_immediate );
DEF_INST( multiply_single );
DEF_INST( multiply_single_register );
DEF_INST( test_under_mask_high );
DEF_INST( test_under_mask_low );
#endif

#if defined( FEATURE_SIE )
DEF_INST( start_interpretive_execution );
#endif

#if defined( FEATURE_IO_ASSIST )
DEF_INST( test_pending_zone_interrupt );
#endif

#if defined( FEATURE_LINKAGE_STACK )
DEF_INST( branch_and_stack );
DEF_INST( extract_stacked_registers );
DEF_INST( extract_stacked_state );
DEF_INST( modify_stacked_state );
DEF_INST( program_return );
DEF_INST( trap2 );
DEF_INST( trap4 );
#endif

#if defined( FEATURE_LOCK_PAGE )
DEF_INST( lock_page );
#endif

#if defined( FEATURE_MOVE_PAGE_FACILITY_2 )
DEF_INST( move_page );
DEF_INST( invalidate_expanded_storage_block_entry );
#endif

#if defined( FEATURE_NEW_ZARCH_ONLY_INSTRUCTIONS ) // 'N' instructions
DEF_INST( add_logical_carry_long );
DEF_INST( add_logical_carry_long_register );
DEF_INST( add_logical_long );
DEF_INST( add_logical_long_fullword );
DEF_INST( add_logical_long_fullword_register );
DEF_INST( add_logical_long_register );
DEF_INST( add_long );
DEF_INST( add_long_fullword );
DEF_INST( add_long_fullword_register );
DEF_INST( add_long_halfword_immediate );
DEF_INST( add_long_register );
DEF_INST( and_immediate_high_high );
DEF_INST( and_immediate_high_low );
DEF_INST( and_immediate_low_high );
DEF_INST( and_immediate_low_low );
DEF_INST( and_long );
DEF_INST( and_long_register );
DEF_INST( branch_on_count_long );
DEF_INST( branch_on_count_long_register );
DEF_INST( branch_on_index_high_long );
DEF_INST( branch_on_index_low_or_equal_long );
DEF_INST( branch_relative_on_count_long );
DEF_INST( branch_relative_on_index_high_long );
DEF_INST( branch_relative_on_index_low_or_equal_long );
DEF_INST( compare_and_swap_long );
DEF_INST( compare_double_and_swap_long );
DEF_INST( compare_logical_characters_under_mask_high );
DEF_INST( compare_logical_long );
DEF_INST( compare_logical_long_fullword );
DEF_INST( compare_logical_long_fullword_register );
DEF_INST( compare_logical_long_register );
DEF_INST( compare_long );
DEF_INST( compare_long_fullword );
DEF_INST( compare_long_fullword_register );
DEF_INST( compare_long_halfword_immediate );
DEF_INST( compare_long_register );
DEF_INST( convert_fix64_to_float_ext_reg );
DEF_INST( convert_fix64_to_float_long_reg );
DEF_INST( convert_fix64_to_float_short_reg );
DEF_INST( convert_float_ext_to_fix64_reg );
DEF_INST( convert_float_long_to_fix64_reg );
DEF_INST( convert_float_short_to_fix64_reg );
DEF_INST( convert_to_binary_long );
DEF_INST( convert_to_decimal_long );
DEF_INST( divide_logical_long );
DEF_INST( divide_logical_long_register );
DEF_INST( divide_single_long );
DEF_INST( divide_single_long_fullword );
DEF_INST( divide_single_long_fullword_register );
DEF_INST( divide_single_long_register );
DEF_INST( exclusive_or_long );
DEF_INST( exclusive_or_long_register );
DEF_INST( extract_and_set_extended_authority );
DEF_INST( extract_stacked_registers_long );
DEF_INST( insert_characters_under_mask_high );
DEF_INST( insert_immediate_high_high );
DEF_INST( insert_immediate_high_low );
DEF_INST( insert_immediate_low_high );
DEF_INST( insert_immediate_low_low );
DEF_INST( load_and_test_long_fullword_register );
DEF_INST( load_and_test_long_register );
DEF_INST( load_complement_long_fullword_register );
DEF_INST( load_complement_long_register );
DEF_INST( load_control_long );
DEF_INST( load_logical_immediate_high_high );
DEF_INST( load_logical_immediate_high_low );
DEF_INST( load_logical_immediate_low_high );
DEF_INST( load_logical_immediate_low_low );
DEF_INST( load_logical_long_character );
DEF_INST( load_logical_long_fullword );
DEF_INST( load_logical_long_fullword_register );
DEF_INST( load_logical_long_halfword );
DEF_INST( load_logical_long_thirtyone );
DEF_INST( load_logical_long_thirtyone_register );
DEF_INST( load_long );
DEF_INST( load_long_fullword );
DEF_INST( load_long_fullword_register );
DEF_INST( load_long_halfword );
DEF_INST( load_long_halfword_immediate );
DEF_INST( load_long_register );
DEF_INST( load_multiple_disjoint );
DEF_INST( load_multiple_high );
DEF_INST( load_multiple_long );
DEF_INST( load_negative_long_fullword_register );
DEF_INST( load_negative_long_register );
DEF_INST( load_pair_from_quadword );
DEF_INST( load_positive_long_fullword_register );
DEF_INST( load_positive_long_register );
DEF_INST( load_program_status_word_extended );
DEF_INST( load_real_address_long );
DEF_INST( load_reversed_long );
DEF_INST( load_reversed_long_register );
DEF_INST( load_using_real_address_long );
DEF_INST( multiply_logical_long );
DEF_INST( multiply_logical_long_register );
DEF_INST( multiply_long_halfword_immediate );
DEF_INST( multiply_single_long );
DEF_INST( multiply_single_long_fullword );
DEF_INST( multiply_single_long_fullword_register );
DEF_INST( multiply_single_long_register );
DEF_INST( or_immediate_high_high );
DEF_INST( or_immediate_high_low );
DEF_INST( or_immediate_low_high );
DEF_INST( or_immediate_low_low );
DEF_INST( or_long );
DEF_INST( or_long_register );
DEF_INST( rotate_left_single_logical_long );
DEF_INST( set_addressing_mode_64 );
DEF_INST( shift_left_single_logical_long );
DEF_INST( shift_left_single_long );
DEF_INST( shift_right_single_logical_long );
DEF_INST( shift_right_single_long );
DEF_INST( store_characters_under_mask_high );
DEF_INST( store_control_long );
DEF_INST( store_long );
DEF_INST( store_multiple_high );
DEF_INST( store_multiple_long );
DEF_INST( store_pair_to_quadword );
DEF_INST( store_real_address );
DEF_INST( store_reversed_long );
DEF_INST( store_using_real_address_long );
DEF_INST( subtract_logical_borrow_long );
DEF_INST( subtract_logical_borrow_long_register );
DEF_INST( subtract_logical_long );
DEF_INST( subtract_logical_long_fullword );
DEF_INST( subtract_logical_long_fullword_register );
DEF_INST( subtract_logical_long_register );
DEF_INST( subtract_long );
DEF_INST( subtract_long_fullword );
DEF_INST( subtract_long_fullword_register );
DEF_INST( subtract_long_register );
DEF_INST( test_under_mask_high_high );
DEF_INST( test_under_mask_high_low );
DEF_INST( trace_long );
#endif /* defined( FEATURE_NEW_ZARCH_ONLY_INSTRUCTIONS ) */

#if defined( FEATURE_PERFORM_LOCKED_OPERATION )
DEF_INST( perform_locked_operation );
#endif

#if defined( FEATURE_QUEUED_DIRECT_IO )
 DEF_INST( signal_adapter );

 #if defined( FEATURE_QEBSM )
 DEF_INST( set_queue_buffer_state );
 DEF_INST( extract_queue_buffer_state );
 #endif

 #if defined( FEATURE_SVS )
 DEF_INST( set_vector_summary );
 #endif
#endif

#if defined( FEATURE_REGION_RELOCATE )
DEF_INST( store_zone_parameter );
DEF_INST( set_zone_parameter );
#endif

#if defined( FEATURE_RESUME_PROGRAM )
DEF_INST( resume_program );
#endif

#if defined( FEATURE_S370_CHANNEL )
DEF_INST( start_io );
DEF_INST( test_io );
DEF_INST( halt_io );
DEF_INST( test_channel );
DEF_INST( store_channel_id );
#endif

#if defined( FEATURE_S370_S390_VECTOR_FACILITY )
DEF_INST( v_test_vmr );
DEF_INST( v_complement_vmr );
DEF_INST( v_count_left_zeros_in_vmr );
DEF_INST( v_count_ones_in_vmr );
DEF_INST( v_extract_vct );
DEF_INST( v_extract_vector_modes );
DEF_INST( v_restore_vr );
DEF_INST( v_save_changed_vr );
DEF_INST( v_save_vr );
DEF_INST( v_load_vmr );
DEF_INST( v_load_vmr_complement );
DEF_INST( v_store_vmr );
DEF_INST( v_and_to_vmr );
DEF_INST( v_or_to_vmr );
DEF_INST( v_exclusive_or_to_vmr );
DEF_INST( v_save_vsr );
DEF_INST( v_save_vmr );
DEF_INST( v_restore_vsr );
DEF_INST( v_restore_vmr );
DEF_INST( v_load_vct_from_address );
DEF_INST( v_clear_vr );
DEF_INST( v_set_vector_mask_mode );
DEF_INST( v_load_vix_from_address );
DEF_INST( v_store_vector_parameters );
DEF_INST( v_save_vac );
DEF_INST( v_restore_vac );
#endif /* defined( FEATURE_S370_S390_VECTOR_FACILITY ) */

#if defined( FEATURE_SERVICE_PROCESSOR )
DEF_INST( service_call );
#endif

#if defined( FEATURE_SET_ADDRESS_SPACE_CONTROL_FAST )
DEF_INST( set_address_space_control_fast );
#endif

#if defined( FEATURE_SQUARE_ROOT )
DEF_INST( squareroot_float_long_reg );
DEF_INST( squareroot_float_short_reg );
#endif

#if defined( FEATURE_STORE_SYSTEM_INFORMATION )
DEF_INST( store_system_information );
#endif

#if defined( FEATURE_STRING_INSTRUCTION )
DEF_INST( compare_logical_string );
DEF_INST( compare_until_substring_equal );
DEF_INST( move_string );
DEF_INST( search_string );
#endif

#if defined( FEATURE_SUBSPACE_GROUP )
DEF_INST( branch_in_subspace_group );
#endif

#if defined( FEATURE_TCPIP_EXTENSION )
DEF_INST( tcpip );
#endif

#if defined( FEATURE_ZVM_ESSA )
DEF_INST( extract_and_set_storage_attributes );
#endif

/*-------------------------------------------------------------------*/
/*   Instructions NOT associated with ANY facility or feature        */
/*-------------------------------------------------------------------*/

DEF_INST( execute_e3________xx );
DEF_INST( execute_eb________xx );
DEF_INST( execute_ec________xx );
DEF_INST( execute_ed________xx );

DEF_INST( add );
DEF_INST( add_decimal );
DEF_INST( add_frr );
DEF_INST( add_halfword );
DEF_INST( add_logical );
DEF_INST( add_logical_register );
DEF_INST( add_register );
DEF_INST( and );
DEF_INST( and_character );
DEF_INST( and_immediate );
DEF_INST( and_register );
DEF_INST( branch_and_link );
DEF_INST( branch_and_link_register );
DEF_INST( branch_and_save );
DEF_INST( branch_and_save_register );
DEF_INST( branch_on_condition );
DEF_INST( branch_on_condition_register );
DEF_INST( branch_on_count );
DEF_INST( branch_on_count_register );
DEF_INST( branch_on_index_high );
DEF_INST( branch_on_index_low_or_equal );
DEF_INST( compare );
DEF_INST( compare_and_form_codeword );
DEF_INST( compare_and_swap );
DEF_INST( compare_decimal );
DEF_INST( compare_double_and_swap );
DEF_INST( compare_halfword );
DEF_INST( compare_logical );
DEF_INST( compare_logical_character );
DEF_INST( compare_logical_character_long );
DEF_INST( compare_logical_characters_under_mask );
DEF_INST( compare_logical_immediate );
DEF_INST( compare_logical_register );
DEF_INST( compare_register );
DEF_INST( convert_to_binary );
DEF_INST( convert_to_decimal );
DEF_INST( diagnose );
DEF_INST( divide );
DEF_INST( divide_decimal );
DEF_INST( divide_register );
DEF_INST( edit_x_edit_and_mark );
DEF_INST( exclusive_or );
DEF_INST( exclusive_or_character );
DEF_INST( exclusive_or_immediate );
DEF_INST( exclusive_or_register );
DEF_INST( execute );
DEF_INST( fix_page );
DEF_INST( insert_character );
DEF_INST( insert_characters_under_mask );
DEF_INST( insert_program_mask );
DEF_INST( insert_psw_key );
DEF_INST( invalidate_page_table_entry );
DEF_INST( load );
DEF_INST( load_address );
DEF_INST( load_address_extended );
DEF_INST( load_and_test_register );
DEF_INST( load_complement_register );
DEF_INST( load_control );
DEF_INST( load_halfword );
DEF_INST( load_multiple );
DEF_INST( load_negative_register );
DEF_INST( load_positive_register );
DEF_INST( load_program_status_word );
DEF_INST( load_real_address );
DEF_INST( load_register );
DEF_INST( load_using_real_address );
DEF_INST( monitor_call );
DEF_INST( move_character );
DEF_INST( move_immediate );
DEF_INST( move_inverse );
DEF_INST( move_long );
DEF_INST( move_numerics );
DEF_INST( move_with_destination_key );
DEF_INST( move_with_offset );
DEF_INST( move_with_source_key );
DEF_INST( move_zones );
DEF_INST( multiply );
DEF_INST( multiply_decimal );
DEF_INST( multiply_halfword );
DEF_INST( multiply_register );
DEF_INST( obtain_cms_lock );
DEF_INST( obtain_local_lock );
DEF_INST( operation_exception  );
DEF_INST( or );
DEF_INST( or_character );
DEF_INST( or_immediate );
DEF_INST( or_register );
DEF_INST( pack );
DEF_INST( purge_translation_lookaside_buffer );
DEF_INST( release_cms_lock );
DEF_INST( release_local_lock );
DEF_INST( set_clock );
DEF_INST( set_clock_comparator );
DEF_INST( set_cpu_timer );
DEF_INST( set_prefix );
DEF_INST( set_program_mask );
DEF_INST( set_psw_key_from_address );
DEF_INST( set_system_mask );
DEF_INST( shift_and_round_decimal );
DEF_INST( shift_left_double );
DEF_INST( shift_left_double_logical );
DEF_INST( shift_left_single );
DEF_INST( shift_left_single_logical );
DEF_INST( shift_right_double );
DEF_INST( shift_right_double_logical );
DEF_INST( shift_right_single );
DEF_INST( shift_right_single_logical );
DEF_INST( signal_processor );
DEF_INST( store );
DEF_INST( store_character );
DEF_INST( store_characters_under_mask );
DEF_INST( store_clock );
DEF_INST( store_clock_comparator );
DEF_INST( store_control );
DEF_INST( store_cpu_address );
DEF_INST( store_cpu_id );
DEF_INST( store_cpu_timer );
DEF_INST( store_halfword );
DEF_INST( store_multiple );
DEF_INST( store_prefix );
DEF_INST( store_then_and_system_mask );
DEF_INST( store_then_or_system_mask );
DEF_INST( store_using_real_address );
DEF_INST( subtract );
DEF_INST( subtract_decimal );
DEF_INST( subtract_halfword );
DEF_INST( subtract_logical );
DEF_INST( subtract_logical_register );
DEF_INST( subtract_register );
DEF_INST( supervisor_call );
DEF_INST( svc_assist );
DEF_INST( test_and_set );
DEF_INST( test_block );
DEF_INST( test_protection );
DEF_INST( test_under_mask );
DEF_INST( trace );
DEF_INST( trace_initial_srb_dispatch );
DEF_INST( trace_io_interruption );
DEF_INST( trace_program_interruption );
DEF_INST( trace_svc_interruption );
DEF_INST( trace_svc_return );
DEF_INST( trace_task_dispatch );
DEF_INST( translate );
DEF_INST( translate_and_test );
DEF_INST( unpack );
DEF_INST( update_tree );
DEF_INST( zero_and_add );

/*-------------------------------------------------------------------*/
/*                    Optimized Instructions                         */
/*-------------------------------------------------------------------*/
/* The following instructions have separate instruction functions    */
/* defined for multiple variants of itself, such as the operand-2    */
/* register number of the Load/Add/Compare Register instruction or   */
/* the condition code of a Branch on Condition instruction. Doing    */
/* so allows the instruction's decoder and execution logic to use    */
/* pre-known constant values resulting in much shorter and simpler   */
/* code, thus allowing the C compiler to more efficiently optimize   */
/* the code even further, resulting in a much faster intruction.     */
/*-------------------------------------------------------------------*/

#ifdef OPTION_OPTINST

// Load Register

#define LRdefgen(r1, r2) \
DEF_INST(18 ## r1 ## r2)

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

// Add Logical Register

#define ALRdefgen(r1, r2) \
DEF_INST(1E ## r1 ## r2)

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

// Compare Logical Register

#define CLRdefgen(r1, r2) \
DEF_INST(15 ## r1 ## r2)

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

// Load Address

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

// Branch on Condition

DEF_INST(47_0);
DEF_INST( nop4 );
DEF_INST(4710);
DEF_INST(4720);
DEF_INST(4730);
DEF_INST(4740);
DEF_INST(4750);
//F_INST(47_0);     // (why?!)
DEF_INST(4770);
DEF_INST(4780);
//F_INST(47_0);     // (why?!)
DEF_INST(47A0);
DEF_INST(47B0);
DEF_INST(47C0);
DEF_INST(47D0);
DEF_INST(47E0);
DEF_INST(47F0);

// Store

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

// Compare Logical

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

// Load

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

// Branch Relative on Condition

DEF_INST( A714 );
DEF_INST( A724 );
DEF_INST( A734 );
DEF_INST( A744 );
DEF_INST( A754 );
DEF_INST( A774 );
DEF_INST( A784 );
DEF_INST( A7A4 );
DEF_INST( A7B4 );
DEF_INST( A7C4 );
DEF_INST( A7D4 );
DEF_INST( A7E4 );
DEF_INST( A7F4 );

// Insert Characters Under Mask

DEF_INST( BF_7 );
DEF_INST( BF_F );
DEF_INST( BF_x );

// Test Under Mask

DEF_INST(9180);
DEF_INST(9140);
DEF_INST(9120);
DEF_INST(9110);
DEF_INST(9108);
DEF_INST(9104);
DEF_INST(9102);
DEF_INST(9101);

// Subtract Logical Register

#define SLRdefgen(r1, r2) \
DEF_INST(1F ## r1 ## r2)

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

#if !defined( OPTION_NO_E3_OPTINST )

// Load/Store 64-bit

DEF_INST( E3_0 );
DEF_INST( E3_0______04 );
DEF_INST( E3_0______24 );

#endif

#endif /* OPTION_OPTINST */

/* end of OPCODE.H */
