/* HREXX_O.C    (C) Copyright Enrico Sorichetti, 2012                */
/*              (C) Copyright "Fish" (David B. Trout), 2017          */
/*              OORexx Interpreter Support                           */
/*                                                                   */
/*  Released under "The Q Public License Version 1"                  */
/*  (http://www.hercules-390.org/herclic.html) as modifications to   */
/*  Hercules.                                                        */

/*  inspired by the previous Rexx implementation by Jan Jaeger       */

#include "hstdinc.h"

#define _HREXX_O_C_
#define _HENGINE_DLL_

#include "hercules.h"
#include "hRexx.h"

#ifdef  OBJECT_REXX         // Support for Object Rexx
#define REXX_PKG            OOREXX_PKG
#define REXX_PKGNUM         OOREXX_PKGNUM

/*-------------------------------------------------------------------*/
/* Object Rexx Support headers                                       */
/*-------------------------------------------------------------------*/
#include "rexx.h"
#include "oorexxapi.h"

/*-------------------------------------------------------------------*/
/* Herculess Rexx implementation equivalents for Object Rexx types   */
/*-------------------------------------------------------------------*/
#define HR_REXXRC_T         RexxReturnCode    // (API return code)
#define HR_ENTRY            REXXENTRY         // (calling convention)
#define HR_PFN_T            REXXPFN           // (func ptr type)

#define HR_EXITHAND_RC_T    RexxReturnCode    // (exit handler rc)
#define HR_SUBCOM_RC_T      RexxReturnCode    // (subcom handler rc)
#define HR_EXTFUNC_RC_T     size_t            // (extern func rc)

#define HR_ARGC_T           size_t            // (array count)
#define HR_ARGV_T           PCONSTRXSTRING    // (RXSTRING array)
#define HR_PCSZ_T           CONSTANT_STRING   // (const RXSTRING)
#define HR_CALLTYPE_T       int               // (RexxStart call type)
#define HR_FCODE_T          int               // (func/subfunc code)
#define HR_MEMSIZE_T        size_t            // (AllocateMemory size)
#define HR_PROCESS_ID_T     process_id_t      // (RexxSetHalt argument)
#define HR_THREAD_ID_T      thread_id_t       // (RexxSetHalt argument)
#define HR_USERINFO_T       CONSTANT_STRING   // (RegisterExit argument)

/*-------------------------------------------------------------------*/
/* Object Rexx library names                                         */
/*-------------------------------------------------------------------*/
#if defined( _MSVC_ )
  #define REXX_LIBNAME      "rexx.dll"
  #define REXX_APILIBNAME   "rexxapi.dll"
#elif defined( __APPLE__ )
  #define REXX_LIBNAME      "librexx.dylib"
  #define REXX_APILIBNAME   "librexxapi.dylib"
#else
  #define REXX_LIBNAME      "librexx.so"
  #define REXX_APILIBNAME   "librexxapi.so"
#endif

/*-------------------------------------------------------------------*/
/* Include the remainder of Hercules generic Rexx support functions  */
/*-------------------------------------------------------------------*/
#include "hRexxapi.c"

#endif // OBJECT_REXX
