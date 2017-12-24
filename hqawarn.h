/* HQAWARN.H    (C) "Fish" (David B. Trout), 2013                    */
/*              QA Build Configuration Testing Scenarios             */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#ifndef _HQAWARN_H_
#define _HQAWARN_H_
#if defined( HQA_SCENARIO ) && HQA_SCENARIO != 0    // HQA Build?
#if HQA_SCENARIO == 1     // System/370 support only
    NOTE("")
    WARNING("HQA_SCENARIO 1: System/370 support only")
    NOTE("")
#endif
#if HQA_SCENARIO == 2     // ESA/390 support only
    NOTE("")
    WARNING("HQA_SCENARIO 2: ESA/390 support only")
    NOTE("")
#endif
#if HQA_SCENARIO == 3     // System/370 and ESA/390 support only
    NOTE("")
    WARNING("HQA_SCENARIO 3: System/370 and ESA/390 support only")
    NOTE("")
#endif
#if HQA_SCENARIO == 4     // zArchitecure and ESA/390 support only
    NOTE("")
    WARNING("HQA_SCENARIO 4: zArchitecure and ESA/390 support only")
    NOTE("")
#endif
#if HQA_SCENARIO == 5     // Windows, fthreads
    NOTE("")
    WARNING("HQA_SCENARIO 5: Windows, fthreads")
    NOTE("")
#endif
#if HQA_SCENARIO == 6     // Windows, Posix threads
    NOTE("")
    WARNING("HQA_SCENARIO 6: Windows, Posix threads")
    NOTE("")
#endif
#if HQA_SCENARIO == 7     // Vista, fthreads
    NOTE("")
    WARNING("HQA_SCENARIO 7: Vista, fthreads")
    NOTE("")
#endif
#if HQA_SCENARIO == 8     // Vista, Posix threads
    NOTE("")
    WARNING("HQA_SCENARIO 8: Vista, Posix threads")
    NOTE("")
#endif
#if HQA_SCENARIO == 9     // INLINE == forced inline
    NOTE("")
    WARNING("HQA_SCENARIO 9: INLINE == forced inline")
    NOTE("")
#endif
#if HQA_SCENARIO == 10     // INLINE == inline (just a suggestion)
    NOTE("")
    WARNING("HQA_SCENARIO 10: INLINE == inline (just a suggestion)")
    NOTE("")
#endif
#if HQA_SCENARIO == 11     // INLINE == null (compiler decides on own)
    NOTE("")
    WARNING("HQA_SCENARIO 11: INLINE == null (compiler decides on own)")
    NOTE("")
#endif
#if HQA_SCENARIO == 12     // NO_IEEE_SUPPORT
    NOTE("")
    WARNING("HQA_SCENARIO 12: NO_IEEE_SUPPORT")
    NOTE("")
#endif
#if HQA_SCENARIO == 13        // *UNASSIGNED*
    NOTE("")
    WARNING("HQA_SCENARIO == 13: *UNASSIGNED*")
    NOTE("")
#endif
#if HQA_SCENARIO == 14     // With Shared Devices
    NOTE("")
    WARNING("HQA_SCENARIO 14: With Shared Devices")
    NOTE("")
#endif
#if HQA_SCENARIO == 15     // Without Shared Devices
    NOTE("")
    WARNING("HQA_SCENARIO 15: Without Shared Devices")
    NOTE("")
#endif
#if HQA_SCENARIO == 16     // Without Object Rexx
    NOTE("")
    WARNING("HQA_SCENARIO 16: Without Object Rexx")
    NOTE("")
#endif
#if HQA_SCENARIO == 17     // Without Regina Rexx
    NOTE("")
    WARNING("HQA_SCENARIO 17: Without Regina Rexx")
    NOTE("")
#endif
#if HQA_SCENARIO == 18     // Without Object Rexx, Without Regina Rexx
    NOTE("")
    WARNING("HQA_SCENARIO 18: Without Object Rexx, Without Regina Rexx")
    NOTE("")
#endif
#if HQA_SCENARIO == 19     // Without TCP keepalive support
    NOTE("")
    WARNING("HQA_SCENARIO 19: Without TCP keepalive support")
    NOTE("")
#endif
#if HQA_SCENARIO == 20     // With Basic TCP keepalive support
    NOTE("")
    WARNING("HQA_SCENARIO 20: With Basic TCP keepalive support")
    NOTE("")
#endif
#if HQA_SCENARIO == 21     // With Partial TCP keepalive support
    NOTE("")
    WARNING("HQA_SCENARIO 21: With Partial TCP keepalive support")
    NOTE("")
#endif
#if HQA_SCENARIO == 22     // With Full TCP keepalive support
    NOTE("")
    WARNING("HQA_SCENARIO 22: With Full TCP keepalive support")
    NOTE("")
#endif
#if HQA_SCENARIO == 23     // Without S/370 Extensions backport
    NOTE("")
    WARNING("HQA_SCENARIO 23: Without S/370 Extensions backport")
    NOTE("")
#endif
#if HQA_SCENARIO == 24     // Without Optimized Instructions
    NOTE("")
    WARNING("HQA_SCENARIO 24: Without Optimized Instructions")
    NOTE("")
#endif
#if HQA_SCENARIO == 25     // Without 370 Extensions or Optimized Instr
    NOTE("")
    WARNING("HQA_SCENARIO 25: Without 370 Extensions or Optimized Instr")
    NOTE("")
#endif
#if HQA_SCENARIO > 25      // UNKNOWN HQA_SCENARIO
    NOTE("")
    WARNING("HQA_SCENARIO " QSTR( HQA_SCENARIO ) ": *** UNKNOWN HQA_SCENARIO!! ***")
    NOTE("")
#endif
#endif // defined(HQA_SCENARIO) && HQA_SCENARIO != 0
#endif /*_HQAWARN_H_*/
