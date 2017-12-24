/* HQADEFS.H    (C) "Fish" (David B. Trout), 2013                    */
/*              QA Build Configuration Testing Scenarios             */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#ifndef _HQADEFS_H_
#define _HQADEFS_H_

/*-------------------------------------------------------------------*/
/*  HQA Scenario    0      Reserved for normal builds                */
/*-------------------------------------------------------------------*/

#if !defined(HQA_SCENARIO) || HQA_SCENARIO == 0

    /* Do nothing. This effectively does a normal build */

#else

/*-------------------------------------------------------------------*/
/*  HQA Scenario    1      System/370 support only                   */
/*-------------------------------------------------------------------*/

#if HQA_SCENARIO == 1   // System/370 support only

  #undef  CUSTOM_BUILD_STRING
  #define CUSTOM_BUILD_STRING "\n\n         HQA Scenario 1\n"

  #define OPTION_370_MODE
  #define NO_390_MODE
  #define NO_900_MODE

#endif

/*-------------------------------------------------------------------*/
/*  HQA Scenario    2      ESA/390 support only                      */
/*-------------------------------------------------------------------*/

#if HQA_SCENARIO == 2   // ESA/390 support only

  #undef  CUSTOM_BUILD_STRING
  #define CUSTOM_BUILD_STRING "\n\n         HQA Scenario 2\n"

  #define NO_370_MODE
  #define OPTION_390_MODE
  #define NO_900_MODE

#endif

/*-------------------------------------------------------------------*/
/*  HQA Scenario    3      System/370 and ESA/390 support only       */
/*-------------------------------------------------------------------*/

#if HQA_SCENARIO == 3   // System/370 and ESA/390 support only

  #undef  CUSTOM_BUILD_STRING
  #define CUSTOM_BUILD_STRING "\n\n         HQA Scenario 3\n"

  #define OPTION_370_MODE
  #define OPTION_390_MODE
  #define NO_900_MODE

#endif

/*-------------------------------------------------------------------*/
/*  HQA Scenario    4      zArchitecure and ESA/390 support only     */
/*-------------------------------------------------------------------*/

#if HQA_SCENARIO == 4   // zArchitecure and ESA/390 support only

  #undef  CUSTOM_BUILD_STRING
  #define CUSTOM_BUILD_STRING "\n\n         HQA Scenario 4\n"

  #define NO_370_MODE
  #define OPTION_390_MODE
  #define OPTION_900_MODE

#endif

/*-------------------------------------------------------------------*/
/*  HQA Scenario    5      Windows, fthreads                         */
/*-------------------------------------------------------------------*/

#if HQA_SCENARIO == 5   // Windows, fthreads

  #undef  CUSTOM_BUILD_STRING
  #define CUSTOM_BUILD_STRING "\n\n         HQA Scenario 5\n"

  #if !defined(_MSVC_)
    #error Selected HQA scenario is for MSVC only
  #endif

  #define OPTION_FTHREADS

#endif

/*-------------------------------------------------------------------*/
/*  HQA Scenario    6      Windows, Posix threads                    */
/*-------------------------------------------------------------------*/

#if HQA_SCENARIO == 6   // Windows, Posix threads

  #undef  CUSTOM_BUILD_STRING
  #define CUSTOM_BUILD_STRING "\n\n         HQA Scenario 6\n"

  #if defined(_MSVC_)
    #error Selected HQA scenario is not supported for MSVC builds
  #endif
  #if !defined(WIN32) && !defined(_WIN32)
    #error Selected HQA scenario is for Windows only
  #endif

  #undef  OPTION_FTHREADS

#endif

/*-------------------------------------------------------------------*/
/*  HQA Scenario    7      Vista, fthreads                           */
/*-------------------------------------------------------------------*/

#if HQA_SCENARIO == 7   // Vista, fthreads

  #undef  CUSTOM_BUILD_STRING
  #define CUSTOM_BUILD_STRING "\n\n         HQA Scenario 7\n"

  #if !defined(_MSVC_)
    #error Selected HQA scenario is for MSVC only
  #endif

  // Vista or later...

  #undef  _WIN32_WINNT
  #undef  WINVER
  #undef  NTDDI_VERSION
  #undef  _WIN32_IE

  #define _WIN32_WINNT      0x0600          // Vista
  #define WINVER            0x0600          // Vista
  #define NTDDI_VERSION     0x06000100      // Vista SP1
  #define _WIN32_IE         0x0700          // IE 7.0

  #define OPTION_FTHREADS

#endif

/*-------------------------------------------------------------------*/
/*  HQA Scenario    8      Vista, Posix threads                      */
/*-------------------------------------------------------------------*/

#if HQA_SCENARIO == 8   // Vista, Posix threads

  #undef  CUSTOM_BUILD_STRING
  #define CUSTOM_BUILD_STRING "\n\n         HQA Scenario 8\n"

  #if defined(_MSVC_)
    #error Selected HQA scenario is not supported for MSVC builds
  #endif
  #if !defined(WIN32) && !defined(_WIN32)
    #error Selected HQA scenario is for Windows only
  #endif

  // Vista or later...

  #undef  _WIN32_WINNT
  #undef  WINVER
  #undef  NTDDI_VERSION
  #undef  _WIN32_IE

  #define _WIN32_WINNT      0x0600          // Vista
  #define WINVER            0x0600          // Vista
  #define NTDDI_VERSION     0x06000100      // Vista SP1
  #define _WIN32_IE         0x0700          // IE 7.0

  #undef  OPTION_FTHREADS

#endif

/*-------------------------------------------------------------------*/
/*  HQA Scenario    9      INLINE == forced inline                   */
/*-------------------------------------------------------------------*/

#if HQA_SCENARIO == 9   // INLINE == forced inline

  #undef  CUSTOM_BUILD_STRING
  #define CUSTOM_BUILD_STRING "\n\n         HQA Scenario 9\n"

  #undef    INLINE
  #if defined(__GNUC__)
    #define INLINE          __inline__ __attribute__((always_inline))
  #elif defined(_MSVC_)
    #define INLINE          __forceinline
  #else
    #define INLINE          __inline
  #endif

#endif

/*-------------------------------------------------------------------*/
/*  HQA Scenario    10     INLINE == inline (just a suggestion)      */
/*-------------------------------------------------------------------*/

#if HQA_SCENARIO == 10  // INLINE == inline (just a suggestion)

  #undef  CUSTOM_BUILD_STRING
  #define CUSTOM_BUILD_STRING "\n\n         HQA Scenario 10\n"

  #undef    INLINE
  #if defined(__GNUC__)
    #define INLINE          __inline__
  #elif defined(_MSVC_)
    #define INLINE          __inline
  #else
    #define INLINE          __inline
  #endif

#endif

/*-------------------------------------------------------------------*/
/*  HQA Scenario    11     INLINE == null (compiler decides on own)  */
/*-------------------------------------------------------------------*/

#if HQA_SCENARIO == 11  // INLINE == null (compiler decides on own)

  #undef  CUSTOM_BUILD_STRING
  #define CUSTOM_BUILD_STRING "\n\n         HQA Scenario 11\n"

  #undef  INLINE
  #define INLINE            /* nothing */

#endif

/*-------------------------------------------------------------------*/
/*  HQA Scenario    12     NO_IEEE_SUPPORT                           */
/*-------------------------------------------------------------------*/

#if HQA_SCENARIO == 12  // NO_IEEE_SUPPORT

  #undef  CUSTOM_BUILD_STRING
  #define CUSTOM_BUILD_STRING "\n\n         HQA Scenario 12\n"

  #define NO_IEEE_SUPPORT

#endif

/*-------------------------------------------------------------------*/
/*  HQA Scenario    13     *UNASSIGNED*                              */
/*-------------------------------------------------------------------*/

#if HQA_SCENARIO == 13  // *UNASSIGNED*

  #undef  CUSTOM_BUILD_STRING
  #define CUSTOM_BUILD_STRING "\n\n         HQA Scenario 13 (*UNASSIGNED*)\n"

//#define xxx...
//#undef  xxx...

#endif

/*-------------------------------------------------------------------*/
/*  HQA Scenario    14     With Shared Devices                       */
/*-------------------------------------------------------------------*/

#if HQA_SCENARIO == 14  // With Shared Devices

  #undef  CUSTOM_BUILD_STRING
  #define CUSTOM_BUILD_STRING "\n\n         HQA Scenario 14\n"

  #define OPTION_SHARED_DEVICES
  #undef  OPTION_NO_SHARED_DEVICES

#endif

/*-------------------------------------------------------------------*/
/*  HQA Scenario    15     Without Shared Devices                    */
/*-------------------------------------------------------------------*/

#if HQA_SCENARIO == 15  // Without Shared Devices

  #undef  CUSTOM_BUILD_STRING
  #define CUSTOM_BUILD_STRING "\n\n         HQA Scenario 15\n"

  #undef  OPTION_SHARED_DEVICES
  #define OPTION_NO_SHARED_DEVICES

#endif

/*-------------------------------------------------------------------*/
/*  HQA Scenario    16     Without Object Rexx                       */
/*-------------------------------------------------------------------*/

#if HQA_SCENARIO == 16  // Without Object Rexx

  #undef  CUSTOM_BUILD_STRING
  #define CUSTOM_BUILD_STRING "\n\n         HQA Scenario 16\n"

  #undef  OBJECT_REXX_HDR_DIR
  #undef  OBJECT_REXX_HEADER1
  #undef  OBJECT_REXX_HEADER2
  #undef  HAVE_OBJECT_REXX
  #undef  HAVE_REXX_H
  #undef  HAVE_OOREXXAPI_H

#endif

/*-------------------------------------------------------------------*/
/*  HQA Scenario    17     Without Regina Rexx                       */
/*-------------------------------------------------------------------*/

#if HQA_SCENARIO == 17  // Without Regina Rexx

  #undef  CUSTOM_BUILD_STRING
  #define CUSTOM_BUILD_STRING "\n\n         HQA Scenario 17\n"

  #undef  REGINA_REXX_HDR_DIR
  #undef  REGINA_REXX_HEADER1
  #undef  HAVE_REGINA_REXX
  #undef  HAVE_REXXSAA_H

#endif

/*-------------------------------------------------------------------*/
/*  HQA Scenario    18     Without Object Rexx, Without Regina Rexx  */
/*-------------------------------------------------------------------*/

#if HQA_SCENARIO == 18  // Without Object Rexx, Without Regina Rexx

  #undef  CUSTOM_BUILD_STRING
  #define CUSTOM_BUILD_STRING "\n\n         HQA Scenario 18\n"

  #undef  OBJECT_REXX_HDR_DIR
  #undef  OBJECT_REXX_HEADER1
  #undef  OBJECT_REXX_HEADER2
  #undef  HAVE_OBJECT_REXX
  #undef  HAVE_REXX_H
  #undef  HAVE_OOREXXAPI_H

  #undef  REGINA_REXX_HDR_DIR
  #undef  REGINA_REXX_HEADER1
  #undef  HAVE_REGINA_REXX
  #undef  HAVE_REXXSAA_H

#endif

/*-------------------------------------------------------------------*/
/*  HQA Scenario    19     Without TCP keepalive support             */
/*-------------------------------------------------------------------*/

#if HQA_SCENARIO == 19  // Without TCP keepalive support

  #undef  CUSTOM_BUILD_STRING
  #define CUSTOM_BUILD_STRING "\n\n         HQA Scenario 19\n"

  #undef  HAVE_BASIC_KEEPALIVE
  #undef  HAVE_PARTIAL_KEEPALIVE
  #undef  HAVE_FULL_KEEPALIVE
  #undef  HAVE_DECL_SO_KEEPALIVE
  #undef  HAVE_DECL_TCP_KEEPALIVE
  #undef  HAVE_DECL_TCP_KEEPIDLE
  #undef  HAVE_DECL_TCP_KEEPINTVL
  #undef  HAVE_DECL_TCP_KEEPCNT

#endif

/*-------------------------------------------------------------------*/
/*  HQA Scenario    20     With Basic TCP keepalive support          */
/*-------------------------------------------------------------------*/

#if HQA_SCENARIO == 20  // With Basic TCP keepalive support

  #undef  CUSTOM_BUILD_STRING
  #define CUSTOM_BUILD_STRING "\n\n         HQA Scenario 20\n"

  #undef  HAVE_BASIC_KEEPALIVE
  #undef  HAVE_PARTIAL_KEEPALIVE
  #undef  HAVE_FULL_KEEPALIVE
//#undef  HAVE_DECL_SO_KEEPALIVE    // (use whichever this system uses)
//#undef  HAVE_DECL_TCP_KEEPALIVE   // (use whichever this system uses)
  #undef  HAVE_DECL_TCP_KEEPIDLE
  #undef  HAVE_DECL_TCP_KEEPINTVL
  #undef  HAVE_DECL_TCP_KEEPCNT

// (we must presume this system has AT LEAST Basic support)

  #define HAVE_BASIC_KEEPALIVE      1
  #undef  HAVE_PARTIAL_KEEPALIVE
  #undef  HAVE_FULL_KEEPALIVE
//#undef  HAVE_DECL_SO_KEEPALIVE    // (use whichever this system uses)
//#undef  HAVE_DECL_TCP_KEEPALIVE   // (use whichever this system uses)
  #undef  HAVE_DECL_TCP_KEEPIDLE
  #undef  HAVE_DECL_TCP_KEEPINTVL
  #undef  HAVE_DECL_TCP_KEEPCNT

#endif

/*-------------------------------------------------------------------*/
/*  HQA Scenario    21     With Partial TCP keepalive support        */
/*-------------------------------------------------------------------*/

#if HQA_SCENARIO == 21  // With Partial TCP keepalive support

  #undef  CUSTOM_BUILD_STRING
  #define CUSTOM_BUILD_STRING "\n\n         HQA Scenario 21\n"

  #undef  HAVE_BASIC_KEEPALIVE
  #undef  HAVE_PARTIAL_KEEPALIVE
  #undef  HAVE_FULL_KEEPALIVE
//#undef  HAVE_DECL_SO_KEEPALIVE    // (use whichever this system uses)
//#undef  HAVE_DECL_TCP_KEEPALIVE   // (use whichever this system uses)
  #undef  HAVE_DECL_TCP_KEEPIDLE
  #undef  HAVE_DECL_TCP_KEEPINTVL
  #undef  HAVE_DECL_TCP_KEEPCNT

  // (we must presume this system has AT LEAST partial support)

  #define HAVE_BASIC_KEEPALIVE      1
  #define HAVE_PARTIAL_KEEPALIVE    1
  #undef  HAVE_FULL_KEEPALIVE
//#define HAVE_DECL_SO_KEEPALIVE    // (use whichever this system uses)
//#define HAVE_DECL_TCP_KEEPALIVE   // (use whichever this system uses)
  #define HAVE_DECL_TCP_KEEPIDLE    1
  #undef  HAVE_DECL_TCP_KEEPINTVL
  #define HAVE_DECL_TCP_KEEPCNT     1

#endif

/*-------------------------------------------------------------------*/
/*  HQA Scenario    22     With Full TCP keepalive support            */
/*-------------------------------------------------------------------*/

#if HQA_SCENARIO == 22  // With Full TCP keepalive support

  #undef  CUSTOM_BUILD_STRING
  #define CUSTOM_BUILD_STRING "\n\n         HQA Scenario 22\n"

  #undef  HAVE_BASIC_KEEPALIVE
  #undef  HAVE_PARTIAL_KEEPALIVE
  #undef  HAVE_FULL_KEEPALIVE
//#undef  HAVE_DECL_SO_KEEPALIVE    // (use whichever this system uses)
//#undef  HAVE_DECL_TCP_KEEPALIVE   // (use whichever this system uses)
  #undef  HAVE_DECL_TCP_KEEPIDLE
  #undef  HAVE_DECL_TCP_KEEPINTVL
  #undef  HAVE_DECL_TCP_KEEPCNT

  // (we must presume this system has full support BY DEFAULT)

  #define HAVE_BASIC_KEEPALIVE      1
  #define HAVE_PARTIAL_KEEPALIVE    1
  #define HAVE_FULL_KEEPALIVE       1
//#define HAVE_DECL_SO_KEEPALIVE    // (use whichever this system uses)
//#define HAVE_DECL_TCP_KEEPALIVE   // (use whichever this system uses)
  #define HAVE_DECL_TCP_KEEPIDLE    1
  #define HAVE_DECL_TCP_KEEPINTVL   1
  #define HAVE_DECL_TCP_KEEPCNT     1

#endif

/*-------------------------------------------------------------------*/
/*  HQA Scenario    23     Without S/370 Extensions backport          */
/*-------------------------------------------------------------------*/

#if HQA_SCENARIO == 23  // Without S/370 Extensions backport

  #undef  CUSTOM_BUILD_STRING
  #define CUSTOM_BUILD_STRING "\n\n         HQA Scenario 23\n"

  #define NO_370_EXTENSION

#endif

/*-------------------------------------------------------------------*/
/*  HQA Scenario    24     Without Optimized Instructions            */
/*-------------------------------------------------------------------*/

#if HQA_SCENARIO == 24  // Without Optimized Instructions

  #undef  CUSTOM_BUILD_STRING
  #define CUSTOM_BUILD_STRING "\n\n         HQA Scenario 24\n"

  #define NO_OPTINST

#endif

/*-------------------------------------------------------------------*/
/*  HQA Scenario    25     Without 370 Extensions or Optimized Instr */
/*-------------------------------------------------------------------*/

#if HQA_SCENARIO == 25  // Without 370 Extensions or Optimized Instr

  #undef  CUSTOM_BUILD_STRING
  #define CUSTOM_BUILD_STRING "\n\n         HQA Scenario 25\n"

  #define NO_370_EXTENSION
  #define NO_OPTINST

#endif

/*-------------------------------------------------------------------*/

#endif // !defined(HQA_SCENARIO) || HQA_SCENARIO == 0

#endif /*_HQADEFS_H_*/
