/* TARGETVER.H  (C) "Fish" (David B. Trout), 2013-2017               */
/*              Define minimum Windows platform support              */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#ifndef _TARGETVER_H_
#define _TARGETVER_H_

#include "vsvers.h"             /* Visual Studio compiler constants  */

/*-------------------------------------------------------------------*/
/* Define some handy descriptive constants like <SDKDDKVer.h> does.  */
/* Note that we must define our own names here as we cannot simply   */
/* #include <SDKDDKVer.h> beforehand here since it will then end up  */
/* setting _WIN32_WINNT to its default value, which we may not want. */
/*-------------------------------------------------------------------*/

#define IE_IE60         0x0600          /* Internet Explorer 6.0     */
#define IE_IE70         0x0700          /* Internet Explorer 7.0     */
#define IE_IE80         0x0800          /* Internet Explorer 8.0     */
#define IE_IE90         0x0900          /* Internet Explorer 9.0     */
#define IE_IE100        0x0A00          /* Internet Explorer 10.0    */
#define IE_IE110        0x0B00          /* Internet Explorer 11.0    */

#define IE_WS03         IE_IE60         /* Windows Server 2003 SP1   */
#define IE_VISTA        IE_IE70         /* Windows Vista             */
#define IE_WIN7         IE_IE80         /* Windows 7                 */
#define IE_WIN8         IE_IE100        /* Windows 8                 */
#define IE_WIN81        IE_IE110        /* Windows 8.1               */

#define WINNT_WS03      0x0502          /* Windows Server 2003 SP1   */
#define WINNT_VISTA     0x0600          /* Windows Vista             */
#define WINNT_WIN7      0x0601          /* Windows 7                 */
#define WINNT_WIN8      0x0602          /* Windows 8                 */
#define WINNT_WIN81     0x0603          /* Windows 8.1               */

#define WINNT_0x0502    "WINNT_WS03"    /* Value as string           */
#define WINNT_0x0600    "WINNT_VISTA"   /* Value as string           */
#define WINNT_0x0601    "WINNT_WIN7"    /* Value as string           */
#define WINNT_0x0602    "WINNT_WIN8"    /* Value as string           */
#define WINNT_0x0603    "WINNT_WIN81"   /* Value as string           */

/*-------------------------------------------------------------------*/
/*      Target Windows Platform for non-makefile projects            */
/*-------------------------------------------------------------------*/
/*                                                                   */
/*  The following _WIN32_WINNT setting defines the minimum required  */
/*  Windows platform.  The minimum required platform is the oldest   */
/*  version of Windows and Internet Explorer, etc, that has all of   */
/*  the necessary features to run your application.                  */
/*                                                                   */
/*  Only '_WIN32_WINNT' needs to be #defined BEFORE #including the   */
/*  <SDKDDKVer.h> header, which will then define the corresponding   */
/*  NTDDI_VERSION, _WIN32_IE and WINVER #define values accordingly.  */
/*                                                                   */
/*-------------------------------------------------------------------*/
/*  Also note that the below constants will only ever be used for    */
/*  normal NON-MAKEFILE projects.  For makefile projects the target  */
/*  platform is instead controlled via the makefile 'APPVER' value.  */
/*  Refer to the TARGETVER.msvc makefile fragment for details.       */
/*-------------------------------------------------------------------*/

#ifndef _WIN32_WINNT                    /* NON-MAKEFILE build?       */
#define _WIN32_WINNT    WINNT_WIN7      /* Windows 7                 */
#define  WINVER         WINNT_WIN7      /* Windows 7                 */
#define _WIN32_IE       IE_IE110        /* Internet Explorer 11.0    */
#include <SDKDDKVer.h>                  /* then need this header.    */
#endif

/* Report Target Platform = whichever _WIN32_WINNT value we're using */
/* regardless of which build type being done (makefile/non-makefile) */

#define FQ( _s )        #_s
#define FQSTR( _s )     FQ( _s )
#define FQP(a,b)        a##b
#define FQPASTE(a,b)    FQP(a,b)

#pragma message( "Target platform: " FQPASTE( WINNT_, _WIN32_WINNT ) " (" FQSTR( _WIN32_WINNT ) ")")

#endif /*_TARGETVER_H_*/
