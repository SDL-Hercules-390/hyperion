/* HOSTOPTS.H   (C) Copyright "Fish" (David B. Trout), 2005-2012     */
/*              Host-specific features and options for Hercules      */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

//   This header file is #included by 'featall.h' and 'hercules.h'

/*---------------------------------------------------------------------

                        PROGRAMMING NOTE

   All HOST-operating-specific (Win32, Apple. Linux, etc) features
   and options that cannot otherwise be determined via configure.ac
   tests should be #defined here, and ONLY here!

   -----------------------------------------------------------------
   REMINDER: please do NOT use host-specific tests anywhere else in
   Hercules source code if you can help it! (e.g. #ifdef WIN32, etc)

   Instead, add a test to configure.ac which tests for the availability
   of the specific feature in question and then #defines a OPTION_XXX
   which can then be used in Hercules source code.
   -----------------------------------------------------------------

   The ONLY allowed exception is in the Hercules.h and htypes.h
   header files where different header files need to be #included
   (e.g. sockets) and/or typedef/#defines need to be made depending
   on the host build system type.

   ONLY IF such a configure.ac test is impractical or otherwise not
   possible should you then hard-code the OPTION_XXX setting here in
   this member (and ONLY in this member!) depending on the host o/s.

   Thus, all of the below hard-coded options are candidates for some
   future configure.ac test.

   Feel free to design one.

   Please.

---------------------------------------------------------------------*/

#ifndef _HOSTOPTS_H
#define _HOSTOPTS_H

#if defined(_MSVC_)
#include "hercwind.h"   // (need HAVE_DECL_SIOCSIFHWADDR, etc)
#endif

#if defined(__sun__) && defined(__svr4__)
#define __SOLARIS__ 1
#endif

/*-------------------------------------------------------------------*/
/* ZZ FIXME
                  'OPTION_SCSI_ERASE_TAPE'
                  'OPTION_SCSI_ERASE_GAP'

    NOTE: The following SHOULD in reality be some sort of test
    within configure.ac, but until we can devise some sort of
    simple configure test, we must hard-code them for now.

    According to the only docs I could find:

       MTERASE   Erase the media from current position.
                 If the field mt_count is nonzero, a full
                 erase is done (from current position to
                 end of media). If mt_count is zero, only
                 an erase gap is written. It is hard to
                 say which drives support only one but not
                 the other option

    HOWEVER, since it's hard to say which drivers support short
    erase-gaps and which support erase-tape (and HOW they support
    them if they do! For example, Cygwin is currently coded to
    perform whichever type of erase the drive happens to support;
    e.g. if you try to do an erase-gap but the drive doesn't support
    short erases, it will end up doing a LONG erase [of the entire
    tape]!! (and vice-versa: doing a long erase-tape on a drive
    that doesn't support it will cause [Cygwin] to do an erase-
    gap instead)).

    THUS, the SAFEST thing to do is to simply treat all "erases",
    whether short or long, as 'nop's for now (in order to prevent
    the accidental erasure of an entire tape!) Once we happen to
    know for DAMN SURE that a particular host o/s ALWAYS does what
    we want it to should we then change the below #defines. (and
    like I said, they really SHOULD be in some type of configure
    test/setting and not here).
*/
/*-------------------------------------------------------------------*\

                     File name comparisons
                  ('strcmp' vs. 'strcasecmp')

   On Windows, file names are not case sensitive. While the case
   of the file name may be preserved by the file system (and thus
   show file names in both upper/lower case in directory listings
   for example), the file system itself is NOT case-sensitive. File
   names "Foo", "foo", "fOo", "FoO", etc, all refer to the same file.

   On other platforms however (e.g. *nix), the file system IS case
   sensitive. File names "Foo", "foo", "fOo", "FoO", etc, all refer
   to different files on such systems. Thus we define a 'strfilecmp'
   macro to be used for filename comparisons and define it to be
   strcasecmp on Win32 platforms and strcmp for other platforms.

\*-------------------------------------------------------------------*/


/*-------------------------------------------------------------------*/
/* Constants used in "#if OPTION_NAME == OPTION_VALUE" statements    */
/*-------------------------------------------------------------------*/

//       HOW_TO_IMPLEMENT_SH_COMMAND

#define  USE_FORK_API_FOR_SH_COMMAND           4
#define  USE_W32_POOR_MANS_FORK                5
#define  USE_ANSI_SYSTEM_API_FOR_SH_COMMAND    9

//       SET_CONSOLE_CURSOR_SHAPE_METHOD

#define  CURSOR_SHAPE_NOT_SUPPORTED             0
#define  CURSOR_SHAPE_VIA_SPECIAL_LINUX_ESCAPE  1
#define  CURSOR_SHAPE_WINDOWS_NATIVE            2


/*-------------------------------------------------------------------*/
/* The following is now handled automatically for ALL host platforms */
/*-------------------------------------------------------------------*/

#undef    OPTION_TUNTAP_SETNETMASK      /* (default initial setting) */
#undef    OPTION_TUNTAP_SETBRDADDR      /* (default initial setting) */
#undef    OPTION_TUNTAP_GETMACADDR      /* (default initial setting) */
#undef    OPTION_TUNTAP_SETMACADDR      /* (default initial setting) */
#undef    OPTION_TUNTAP_DELADD_ROUTES   /* (default initial setting) */
#undef    OPTION_TUNTAP_CLRIPADDR       /* (default initial setting) */
#undef    OPTION_TUNTAP_LCS_SAME_ADDR   /* (default initial setting) */

#if defined(HAVE_DECL_SIOCSIFNETMASK) && \
            HAVE_DECL_SIOCSIFNETMASK
    #define OPTION_TUNTAP_SETNETMASK    /* TUNTAP_SetNetMask works   */
#endif

#if defined(HAVE_DECL_SIOCSIFBRDADDR) && \
            HAVE_DECL_SIOCSIFBRDADDR
    #define OPTION_TUNTAP_SETBRDADDR    /* TUNTAP_SetBCastAddr works */
#endif

#if defined(HAVE_DECL_SIOCGIFHWADDR) && \
            HAVE_DECL_SIOCGIFHWADDR
   #define OPTION_TUNTAP_GETMACADDR     /* TUNTAP_GetMACAddr works   */
#endif

#if defined(HAVE_DECL_SIOCSIFHWADDR) && \
            HAVE_DECL_SIOCSIFHWADDR
   #define OPTION_TUNTAP_SETMACADDR     /* TUNTAP_SetMACAddr works   */
#endif

#if defined(HAVE_DECL_SIOCADDRT) && defined(HAVE_DECL_SIOCDELRT) && \
            HAVE_DECL_SIOCADDRT  &&         HAVE_DECL_SIOCDELRT
 #define OPTION_TUNTAP_DELADD_ROUTES    /* Del/Add Routes    works   */
#endif

#if defined(HAVE_DECL_SIOCDIFADDR) && \
            HAVE_DECL_SIOCDIFADDR
  #define OPTION_TUNTAP_CLRIPADDR       /* TUNTAP_ClrIPAddr works    */
#endif


/*-------------------------------------------------------------------*/
/* Hard-coded Windows-specific features and options...               */
/*-------------------------------------------------------------------*/
#if defined(WIN32)                      /* "Windows" options         */

#if defined(_MSVC_)

  #define  DLL_IMPORT           __declspec ( dllimport )
  #define  DLL_EXPORT           __declspec ( dllexport )
  #define  INL_DLL_IMPORT
  #define  INL_DLL_EXPORT       extern

#else // (cygwin??)

  #define  DLL_IMPORT           extern
  #define  DLL_EXPORT
  #define  INL_DLL_IMPORT
  #define  INL_DLL_EXPORT       extern

#endif

#define OPTION_W32_CTCI                 /* Fish's TunTap for CTCA's  */
#undef  TUNTAP_IFF_RUNNING_NEEDED       /* TunTap32 doesn't allow it */
#define OPTION_SCSI_TAPE                /* SCSI tape support         */
#ifdef _MSVC_
#define OPTION_SCSI_ERASE_TAPE          /* SUPPORTED!                */
#define OPTION_SCSI_ERASE_GAP           /* SUPPORTED!                */
#else // (mingw or cygwin?)
#undef  OPTION_SCSI_ERASE_TAPE          /* (NOT supported!)          */
#undef  OPTION_SCSI_ERASE_GAP           /* (NOT supported!)          */
#endif
#undef  OPTION_FBA_BLKDEVICE            /* (no FBA BLKDEVICE support)*/
#define MAX_DEVICE_THREADS          0   /* (0 == unlimited)          */
#undef  MIXEDCASE_FILENAMES_ARE_UNIQUE  /* ("Foo" same as "fOo"!!)   */

#define CASELESS_SYMBOLS

#ifdef _MSVC_
  #define HOW_TO_IMPLEMENT_SH_COMMAND   USE_W32_POOR_MANS_FORK
  #define SET_CONSOLE_CURSOR_SHAPE_METHOD CURSOR_SHAPE_WINDOWS_NATIVE
  #define OPTION_EXTCURS                /* Extended cursor handling  */
  #undef  SCANDIR_CONST_STRUCT_DIRENT   /* define if scandir uses
                                           const for struct dirent   */
#else  /* Cygwin or MinGW */
  #define HOW_TO_IMPLEMENT_SH_COMMAND   USE_FORK_API_FOR_SH_COMMAND
  #define SET_CONSOLE_CURSOR_SHAPE_METHOD CURSOR_SHAPE_VIA_SPECIAL_LINUX_ESCAPE
  #undef  OPTION_EXTCURS                /* Normal cursor handling    */
  #undef  SCANDIR_CONST_STRUCT_DIRENT   /* define if scandir uses
                                           const for struct dirent   */
#endif

#define IsEventSet(h)   (WaitForSingleObject(h,0) == WAIT_OBJECT_0)

/*-------------------------------------------------------------------*/
/* Because Fish's TUNTAP emulation isn't seen as a network interface */
/* by the host operating system, there is only one MAC address.      */
/* (The Windows "tap" is a capture and re-inject mechanism)          */
/*                                                                   */
/* For other tuntap implementation, the host and guest have          */
/* separate abstracted NIC implementation - and therefore require    */
/* a separate MAC address to address that (lest briding won't work)  */
/*                                                                   */
/* If at one point, a TUNTAP implementation comes up and is then     */
/* seen as a proper network interface by Windows, then this option   */
/* will have to go away - or anyway "undefined" for windows          */
/*-------------------------------------------------------------------*/
#define   OPTION_TUNTAP_LCS_SAME_ADDR   1


/*-------------------------------------------------------------------*/
/* Hard-coded Solaris-specific features and options...               */
/*-------------------------------------------------------------------*/
#elif defined(__SOLARIS__)

/* jbs 10/15/2003 need to define INADDR_NONE if using Solaris 10
   and not Solaris Nevada aka OpenSolaris */
#if !defined(INADDR_NONE)
  #define INADDR_NONE       0xffffffffU
#endif
#undef  OPTION_SCSI_TAPE                /* No SCSI tape support      */
#undef  OPTION_SCSI_ERASE_TAPE          /* (NOT supported)           */
#undef  OPTION_SCSI_ERASE_GAP           /* (NOT supported)           */
#define DLL_IMPORT              extern
#define DLL_EXPORT
#define INL_DLL_IMPORT
#define INL_DLL_EXPORT          extern
#define MAX_DEVICE_THREADS          0   /* (0 == unlimited)          */
#define MIXEDCASE_FILENAMES_ARE_UNIQUE  /* ("Foo" and "fOo" unique)  */
#define HOW_TO_IMPLEMENT_SH_COMMAND       USE_ANSI_SYSTEM_API_FOR_SH_COMMAND
#define SET_CONSOLE_CURSOR_SHAPE_METHOD   CURSOR_SHAPE_NOT_SUPPORTED
#undef  OPTION_EXTCURS                  /* Normal cursor handling    */
#undef  SCANDIR_CONST_STRUCT_DIRENT     /* define if scandir uses
                                           const for struct dirent   */


/*-------------------------------------------------------------------*/
/* Hard-coded Apple-specific features and options...                 */
/*-------------------------------------------------------------------*/
#elif defined(__APPLE__)                /* "Apple" options           */

#define DLL_IMPORT              extern
#define DLL_EXPORT
#define INL_DLL_IMPORT
#define INL_DLL_EXPORT          extern
#define TUNTAP_IFF_RUNNING_NEEDED       /* Needed by tuntap driver?? */
#undef  OPTION_SCSI_TAPE                /* No SCSI tape support      */
#undef  OPTION_SCSI_ERASE_TAPE          /* (NOT supported)           */
#undef  OPTION_SCSI_ERASE_GAP           /* (NOT supported)           */
#undef  OPTION_FBA_BLKDEVICE            /* (no FBA BLKDEVICE support)*/
#define MAX_DEVICE_THREADS          0   /* (0 == unlimited)          */
#define MIXEDCASE_FILENAMES_ARE_UNIQUE  /* ("Foo" and "fOo" unique)  */
#define HOW_TO_IMPLEMENT_SH_COMMAND       USE_ANSI_SYSTEM_API_FOR_SH_COMMAND
#define SET_CONSOLE_CURSOR_SHAPE_METHOD   CURSOR_SHAPE_NOT_SUPPORTED
#undef  OPTION_EXTCURS                  /* Normal cursor handling    */
#define SCANDIR_CONST_STRUCT_DIRENT     /* define if scandir uses
                                           const for struct dirent   */


/*-------------------------------------------------------------------*/
/* Hard-coded FreeBSD/NetBSD-specific features and options...        */
/*-------------------------------------------------------------------*/
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)

#define FREEBSD_OR_NETBSD

#define DLL_IMPORT              extern
#define DLL_EXPORT
#define INL_DLL_IMPORT
#define INL_DLL_EXPORT          extern
#define TUNTAP_IFF_RUNNING_NEEDED       /* Needed by tuntap driver?? */
#undef  OPTION_SCSI_ERASE_TAPE          /* (NOT supported)           */
#undef  OPTION_SCSI_ERASE_GAP           /* (NOT supported)           */
#define MAX_DEVICE_THREADS          0   /* (0 == unlimited)          */
#define MIXEDCASE_FILENAMES_ARE_UNIQUE  /* ("Foo" and "fOo" unique)  */
#define HOW_TO_IMPLEMENT_SH_COMMAND       USE_ANSI_SYSTEM_API_FOR_SH_COMMAND
#define SET_CONSOLE_CURSOR_SHAPE_METHOD   CURSOR_SHAPE_NOT_SUPPORTED
#undef  OPTION_EXTCURS                  /* Normal cursor handling    */
#define SCANDIR_CONST_STRUCT_DIRENT     /* define if scandir uses
                                           const for struct dirent   */


/*-------------------------------------------------------------------*/
/* Hard-coded GNU Linux-specific features and options...             */
/*-------------------------------------------------------------------*/
#elif defined(__gnu_linux__) || defined(__linux__) /* Linux options  */

#define DLL_IMPORT              extern
#define DLL_EXPORT
#define INL_DLL_IMPORT
#define INL_DLL_EXPORT          extern
#define TUNTAP_IFF_RUNNING_NEEDED       /* Needed by tuntap driver?? */
#define OPTION_SCSI_TAPE                /* SCSI tape support         */
#undef  OPTION_SCSI_ERASE_TAPE          /* (NOT supported)           */
#undef  OPTION_SCSI_ERASE_GAP           /* (NOT supported)           */
#define OPTION_FBA_BLKDEVICE            /* FBA block device support  */
#define MAX_DEVICE_THREADS          0   /* (0 == unlimited)          */
#define MIXEDCASE_FILENAMES_ARE_UNIQUE  /* ("Foo" and "fOo" unique)  */

#if defined( HAVE_FORK )
  #define HOW_TO_IMPLEMENT_SH_COMMAND     USE_FORK_API_FOR_SH_COMMAND
#else
  #define HOW_TO_IMPLEMENT_SH_COMMAND     USE_ANSI_SYSTEM_API_FOR_SH_COMMAND
#endif
#define SET_CONSOLE_CURSOR_SHAPE_METHOD   CURSOR_SHAPE_VIA_SPECIAL_LINUX_ESCAPE
#undef  OPTION_EXTCURS                  /* Normal cursor handling    */
#define SCANDIR_CONST_STRUCT_DIRENT     /* define if scandir uses
                                           const for struct dirent   */


/*-------------------------------------------------------------------*/
/* Hard-coded AIX-specific features and options...      -- bozy      */
/*-------------------------------------------------------------------*/
#elif defined(_AIX)                     /* AIX 5.3 options           */

#define SOL_TCP            IPPROTO_TCP  /* (both mean same thing)    */
#define DLL_IMPORT              extern
#define DLL_EXPORT
#define INL_DLL_IMPORT
#define INL_DLL_EXPORT          extern
#undef  TUNTAP_IFF_RUNNING_NEEDED       /* (tuntap support unknown)  */
#undef  OPTION_SCSI_TAPE                /* (NO SCSI tape support)    */
#undef  OPTION_SCSI_ERASE_TAPE          /* (NOT supported)           */
#undef  OPTION_SCSI_ERASE_GAP           /* (NOT supported)           */
#define OPTION_FBA_BLKDEVICE            /* FBA block device support  */
#define MAX_DEVICE_THREADS        255   /* (0 == unlimited)          */
#define MIXEDCASE_FILENAMES_ARE_UNIQUE  /* ("Foo" and "fOo" unique)  */
#if defined( HAVE_FORK )
  #define HOW_TO_IMPLEMENT_SH_COMMAND     USE_FORK_API_FOR_SH_COMMAND
#else
  #define HOW_TO_IMPLEMENT_SH_COMMAND     USE_ANSI_SYSTEM_API_FOR_SH_COMMAND
#endif
#define SET_CONSOLE_CURSOR_SHAPE_METHOD   CURSOR_SHAPE_NOT_SUPPORTED
#undef  OPTION_EXTCURS                  /* Normal cursor handling    */
#define SCANDIR_CONST_STRUCT_DIRENT     /* define if scandir uses
                                           const for struct dirent   */


/*-------------------------------------------------------------------*/
/* Hard-coded OTHER (DEFAULT) host-specific features and options...  */
/*-------------------------------------------------------------------*/
#else                                   /* "Other platform" options  */

WARNING( "unknown target platform: defaulting to generic platform settings" )

#define DLL_IMPORT              extern  /* (a safe default)          */
#define DLL_EXPORT
#define INL_DLL_IMPORT
#define INL_DLL_EXPORT          extern
#undef  TUNTAP_IFF_RUNNING_NEEDED       /* (tuntap support unknown)  */
#undef  OPTION_SCSI_TAPE                /* (NO SCSI tape support)    */
#undef  OPTION_SCSI_ERASE_TAPE          /* (NOT supported)           */
#undef  OPTION_SCSI_ERASE_GAP           /* (NOT supported)           */
#undef  OPTION_FBA_BLKDEVICE            /* (no FBA BLKDEVICE support)*/
#define MAX_DEVICE_THREADS          0   /* (0 == unlimited)          */
#define MIXEDCASE_FILENAMES_ARE_UNIQUE  /* ("Foo" and "fOo" unique)  */
#if defined( HAVE_FORK )
  #define HOW_TO_IMPLEMENT_SH_COMMAND     USE_FORK_API_FOR_SH_COMMAND
#else
  #define HOW_TO_IMPLEMENT_SH_COMMAND     USE_ANSI_SYSTEM_API_FOR_SH_COMMAND
#endif
#define SET_CONSOLE_CURSOR_SHAPE_METHOD   CURSOR_SHAPE_NOT_SUPPORTED
#undef  OPTION_EXTCURS                  /* Normal cursor handling    */
#define SCANDIR_CONST_STRUCT_DIRENT     /* define if scandir uses
                                           const for struct dirent   */


#endif // (host-specific tests)

#endif // _HOSTOPTS_H
