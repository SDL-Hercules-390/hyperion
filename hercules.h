/* HERCULES.H   (C) Copyright Roger Bowler, 1999-2012                */
/*              (C) and others 2013-2023                             */
/*              Hercules Header Files                                */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/* Interpretive Execution - (C) Copyright Jan Jaeger, 1999-2012      */
/* z/Architecture support - (C) Copyright Jan Jaeger, 1999-2012      */

/*-------------------------------------------------------------------*/
/*                                                                   */
/*                      PROGRAMMING NOTE                             */
/*                                                                   */
/*  The "feature.h" header MUST be #included AFTER <config.h> *AND*  */
/*  BEFORE the _HERCULES_H pre-processor variable gets #defined.     */
/*                                                                   */
/*  This is to enure it is ALWAYS #included regardless of whether    */
/*  the "hercules.h" header has already been #included or not.       */
/*                                                                   */
/*  This is so the various architecture dependent source modules     */
/*  compile properly since they #include themselves several times    */
/*  so as to cause them to be compiled multiple times, each time     */
/*  with a new architecture mode #defined (e.g. 370/390/900) which   */
/*  is what the "feature.h" header reacts to.                        */
/*                                                                   */
/*  Refer to the very end of the source member "general2.c" for a    */
/*  typical example of this very technique.                          */
/*                                                                   */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/*  These headers are #included multiple times as explained above    */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"      /* Standard system header includes         */

#include "feature.h"      /* Hercules [manually maintained] features */
                          /* auto-includes featall.h and hostopts.h  */

                          /* ALWAYS include cpuint.h after feature.h */
                          /* and also assure it is re-included for   */
                          /* each archs.                             */

#include "cpuint.h"       /* ALWAYS after "feature.h                 */

/*-------------------------------------------------------------------*/
/*       The following header are only ever #included once           */
/*-------------------------------------------------------------------*/

#ifndef _HERCULES_H       /* MUST come AFTER "feature.h" is included */
#define _HERCULES_H       /* MUST come AFTER "feature.h" is included */

/*-------------------------------------------------------------------*/
/*              Private Hercules-specific headers                    */
/*-------------------------------------------------------------------*/

#include "impexp.h"       /* DLL_IMPORT/DLL_EXPORT/extern control    */
#include "linklist.h"     /* Hercules-wide linked-list macros        */
#include "hconsts.h"      /* Hercules-wide #define constants         */
#include "hthreads.h"     /* Hercules-wide threading macros          */
#include "hmacros.h"      /* Hercules-wide #define macros            */
#include "hmalloc.h"      /* Hercules malloc/free functions          */
#include "herror.h"       /* Hercules-wide error definitions         */
#include "extstring.h"    /* Extended string handling routines       */

#if !defined( HAVE_BYTESWAP_H ) || defined( NO_ASM_BYTESWAP )
 #include "hbyteswp.h"    /* Hercules equivalent of <byteswap.h>     */
#endif

#if !defined( HAVE_MEMRCHR )
  #include "memrchr.h"    /* Hercules Right to Left memory scan      */
#endif

#include "hostinfo.h"
#include "version.h"
#include "esa390.h"       /* ESA/390 structure definitions           */
#include "hscutl.h"       /* utility functions                       */
#include "w32util.h"      /* win32 porting functions                 */
#include "clock.h"        /* TOD definitions                         */
#include "qeth.h"         /* QETH device definitions                 */
#include "qdio.h"         /* QDIO instructions, functions, structs   */
#include "codepage.h"
#include "logger.h"       /* logmsg, etc                             */
#include "hdl.h"          /* Hercules Dynamic Loader                 */
#include "cache.h"
#include "devtype.h"      /* DEVHND struct                           */
#include "dasdtab.h"      /* DASD table structures                   */
#include "shared.h"       /* Hercules Shared Device Server           */
#include "hetlib.h"       /* Hercules Emulated Tape library          */
#include "sockdev.h"
#include "w32ctca.h"
#include "service.h"
#include "hsocket.h"

#ifdef _MSVC_
  #include "w32mtio.h"    /* mtio.h needed by below "hstructs.h"     */
  #include "sys/locking.h"
#endif // _MSVC_

#include "hstructs.h"     /* Hercules-wide structures                */
#include "hexterns.h"     /* Hercules-wide function declarations     */
#include "msgenu.h"       /* Hercules messages                       */
#include "hinlines.h"     /* Hercules-wide inline functions          */
#include "archlvl.h"      /* Architecture Level   functions          */
#include "facility.h"     /* Facility Bit         functions          */

#endif // _HERCULES_H
