/* PTTRACE.H    (C) Copyright Greg Smith, 2003-2013                  */
/*              Threading and locking trace debugger                 */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* Pthread tracing structures and prototypes                         */
/*-------------------------------------------------------------------*/

#ifndef _PTTHREAD_H_
#define _PTTHREAD_H_

/*-------------------------------------------------------------------*/
/*                    PTT TRACE CLASSES                              */
/*-------------------------------------------------------------------*/
/*                    (System Classes)                               */
/*-------------------------------------------------------------------*/
#define PTT_CL_LOG   0x0000000000000001 /* Logger records            */
#define PTT_CL_TMR   0x0000000000000002 /* Timer/Clock records       */
#define PTT_CL_THR   0x0000000000000004 /* Thread records            */
#define PTT_CL_INF   0x0000000000000008 /* Instruction info          */
#define PTT_CL_ERR   0x0000000000000010 /* Instruction error/unsupp  */
#define PTT_CL_PGM   0x0000000000000020 /* Program interrupt         */
#define PTT_CL_CSF   0x0000000000000040 /* Compare & Swap failure    */
#define PTT_CL_SIE   0x0000000000000080 /* Interpretive Execution    */
#define PTT_CL_SIG   0x0000000000000100 /* SIGP signalling           */
#define PTT_CL_IO    0x0000000000000200 /* IO                        */
#if defined( _FEATURE_073_TRANSACT_EXEC_FACILITY )
#define PTT_CL_TXF   0x0000000000000400 /* Transact. Exec. Facility  */
#else
//efine PTT_CL_XXX   0x0000000000000400 /* System class 11           */
#endif
//efine PTT_CL_XXX   0x0000000000000800 /* System class 12           */
//efine PTT_CL_XXX   0x0000000000001000 /* System class 13           */
//efine PTT_CL_XXX   0x0000000000002000 /* System class 14           */
//efine PTT_CL_XXX   0x0000000000004000 /* System class 15           */
//efine PTT_CL_XXX   0x0000000000008000 /* System class 16           */
/*-------------------------------------------------------------------*/
/*                     (User Classes)                                */
/*-------------------------------------------------------------------*/
#define PTT_CL_LCS1  0x0000000000010000 /* LCS Timing Debug          */
#define PTT_CL_LCS2  0x0000000000020000 /* LCS General Debugging     */
#define PTT_CL_QETH  0x0000000000040000 /* QETH General Debugging    */
#define PTT_CL_XXX   0x0000000000080000 /* Undefined/generic/custom  */
//efine PTT_CL_ZZZ   0x0000000000100000 /* User class 5              */
//efine PTT_CL_ZZZ   0x0000000000200000 /* User class 6              */
//efine PTT_CL_ZZZ   0x0000000000400000 /* User class 7              */
//efine PTT_CL_ZZZ   0x0000000000800000 /* User class 8              */
//efine PTT_CL_ZZZ   0x0000000001000000 /* User class 9              */
//efine PTT_CL_ZZZ   0x0000000002000000 /* User class 10             */
//efine PTT_CL_ZZZ   0x0000000004000000 /* User class 11             */
//efine PTT_CL_ZZZ   0x0000000008000000 /* User class 12             */
//efine PTT_CL_ZZZ   0x0000000010000000 /* User class 13             */
//efine PTT_CL_ZZZ   0x0000000020000000 /* User class 14             */
//efine PTT_CL_ZZZ   0x0000000040000000 /* User class 15             */
//efine PTT_CL_ZZZ   0x0000000080000000 /* User class 16             */
//efine PTT_CL_ZZZ   0x0000000100000000 /* User class 17             */
//efine PTT_CL_ZZZ   0x0000000200000000 /* User class 18             */
//efine PTT_CL_ZZZ   0x0000000400000000 /* User class 19             */
//efine PTT_CL_ZZZ   0x0000000800000000 /* User class 20             */
//efine PTT_CL_ZZZ   0x0000001000000000 /* User class 21             */
//efine PTT_CL_ZZZ   0x0000002000000000 /* User class 22             */
//efine PTT_CL_ZZZ   0x0000004000000000 /* User class 23             */
//efine PTT_CL_ZZZ   0x0000008000000000 /* User class 24             */
//efine PTT_CL_ZZZ   0x0000010000000000 /* User class 25             */
//efine PTT_CL_ZZZ   0x0000020000000000 /* User class 26             */
//efine PTT_CL_ZZZ   0x0000040000000000 /* User class 27             */
//efine PTT_CL_ZZZ   0x0000080000000000 /* User class 28             */
//efine PTT_CL_ZZZ   0x0000100000000000 /* User class 29             */
//efine PTT_CL_ZZZ   0x0000200000000000 /* User class 30             */
//efine PTT_CL_ZZZ   0x0000400000000000 /* User class 31             */
//efine PTT_CL_ZZZ   0x0000800000000000 /* User class 32             */
//efine PTT_CL_ZZZ   0x0001000000000000 /* User class 33             */
//efine PTT_CL_ZZZ   0x0002000000000000 /* User class 34             */
//efine PTT_CL_ZZZ   0x0004000000000000 /* User class 35             */
//efine PTT_CL_ZZZ   0x0008000000000000 /* User class 36             */
//efine PTT_CL_ZZZ   0x0010000000000000 /* User class 37             */
//efine PTT_CL_ZZZ   0x0020000000000000 /* User class 38             */
//efine PTT_CL_ZZZ   0x0040000000000000 /* User class 39             */
//efine PTT_CL_ZZZ   0x0080000000000000 /* User class 40             */
//efine PTT_CL_ZZZ   0x0100000000000000 /* User class 41             */
//efine PTT_CL_ZZZ   0x0200000000000000 /* User class 42             */
//efine PTT_CL_ZZZ   0x0400000000000000 /* User class 43             */
//efine PTT_CL_ZZZ   0x0800000000000000 /* User class 44             */
//efine PTT_CL_ZZZ   0x1000000000000000 /* User class 45             */
//efine PTT_CL_ZZZ   0x2000000000000000 /* User class 46             */
//efine PTT_CL_ZZZ   0x4000000000000000 /* User class 47             */
//efine PTT_CL_ZZZ   0x8000000000000000 /* User class 48             */

/*-------------------------------------------------------------------*/
/*                  Primary PTT Tracing macro                        */
/*-------------------------------------------------------------------*/
#define PTT( _class, _msg, _data1, _data2, _rc )                     \
do {                                                                 \
  if (pttclass & (_class))                                           \
    ptt_pthread_trace( (_class), (_msg),(void*)(uintptr_t)(_data1),  \
                                         (void*)(uintptr_t)(_data2), \
                                         PTT_LOC,                    \
                                         (S64)(_rc),NULL);           \
} while(0)

/*-------------------------------------------------------------------*/
/*      PTT macros with trace class as part of their name            */
/*-------------------------------------------------------------------*/
#define PTT_LOG(   m, d1, d2, rc )  PTT( PTT_CL_LOG,   m, d1, d2, rc )
#define PTT_TMR(   m, d1, d2, rc )  PTT( PTT_CL_TMR,   m, d1, d2, rc )
#define PTT_THR(   m, d1, d2, rc )  PTT( PTT_CL_THR,   m, d1, d2, rc )
#define PTT_INF(   m, d1, d2, rc )  PTT( PTT_CL_INF,   m, d1, d2, rc )
#define PTT_ERR(   m, d1, d2, rc )  PTT( PTT_CL_ERR,   m, d1, d2, rc )
#define PTT_PGM(   m, d1, d2, rc )  PTT( PTT_CL_PGM,   m, d1, d2, rc )
#define PTT_CSF(   m, d1, d2, rc )  PTT( PTT_CL_CSF,   m, d1, d2, rc )
#define PTT_SIE(   m, d1, d2, rc )  PTT( PTT_CL_SIE,   m, d1, d2, rc )
#define PTT_SIG(   m, d1, d2, rc )  PTT( PTT_CL_SIG,   m, d1, d2, rc )
#define PTT_IO(    m, d1, d2, rc )  PTT( PTT_CL_IO,    m, d1, d2, rc )
#if defined( _FEATURE_073_TRANSACT_EXEC_FACILITY )
#define PTT_TXF(   m, d1, d2, rc )  PTT( PTT_CL_TXF,   m, d1, d2, rc )
#else
#define PTT_TXF(   m, d1, d2, rc )  // (nothing)
#endif
#define PTT_LCS1(  m, d1, d2, rc )  PTT( PTT_CL_LCS1,  m, d1, d2, rc )
#define PTT_LCS2(  m, d1, d2, rc )  PTT( PTT_CL_LCS2,  m, d1, d2, rc )
#define PTT_QETH(  m, d1, d2, rc )  PTT( PTT_CL_QETH,  m, d1, d2, rc )
#define PTT_XXX(   m, d1, d2, rc )  PTT( PTT_CL_XXX,   m, d1, d2, rc )

/*-------------------------------------------------------------------*/
/*           Shorter name than 'struct timeval'                      */
/*-------------------------------------------------------------------*/
#ifndef TIMEVAL
#define TIMEVAL                 struct timeval
#endif

/*-------------------------------------------------------------------*/
/*               Exported Functions and Variables                    */
/*-------------------------------------------------------------------*/
PTT_DLL_IMPORT void ptt_trace_init    ( int nTableSize, BOOL init );
PTT_DLL_IMPORT int  ptt_cmd           ( int argc, char* argv[], char* cmdline );
PTT_DLL_IMPORT void ptt_pthread_trace ( U64, const char*, const void*, const void*, const char*, S64, TIMEVAL* );
PTT_DLL_IMPORT int  ptt_pthread_print ();/* rc = #of entries printed */
PTT_DLL_IMPORT U64  pttclass;
PTT_DLL_IMPORT bool ptt_dtax(); // Dump Table At Exit

/*-------------------------------------------------------------------*/
/*                      Misc Helper Macro                            */
/*-------------------------------------------------------------------*/
#define PTT_MAGIC           -99         /* means rc is uninteresting */

#endif /* _PTTHREAD_H_ */
