/*  VERSION.H   (c) Copyright Roger Bowler, 1999-2012                */
/*      HERCULES Emulator Version definition                         */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* Header file defining the Hercules version number.                 */
/*                                                                   */
/* NOTE: If you're looking for the place to actually change the      */
/* number, it's in configure.ac, near the top.                       */
/*-------------------------------------------------------------------*/

#ifndef _HERCULES_H_
#define _HERCULES_H_

#include "hercules.h"
#include "_dynamic_version.h"  /* created by _dynamic_version script */

#ifndef _VERSION_C_
#ifndef _HUTIL_DLL_
#define VER_DLL_IMPORT DLL_IMPORT
#else   /* _HUTIL_DLL_ */
#define VER_DLL_IMPORT extern
#endif  /* _HUTIL_DLL_ */
#else   /* _LOGGER_C_ */
#define VER_DLL_IMPORT DLL_EXPORT
#endif /* _LOGGER_C_ */

#if !defined(VERSION)
  #if defined(VERS_MAJ) && defined(VERS_INT) && defined(VERS_MIN) && defined(VERS_BLD)
    #define VER VERS_MAJ##.##VERS_INT##.##VERS_MIN##.##VERS_BLD
    #define VERSION QSTR(VER)
  #endif
#endif

#if defined( _MSVC_ )
  /* Some modules, such as dyngui, might need these values,
     since they are ALWAYS numeric whereas VERSION is not. */
  #if !defined(VERS_MAJ) || !defined(VERS_INT) || !defined(VERS_MIN) || !defined(VERS_BLD)
    #error "VERSION not defined properly"
  #endif
#endif

/*
    The 'VERSION' string can be any value the user wants.
*/
#if !defined(VERSION)
  WARNING("No version specified")
  #define VERSION              "0.0.0.0-(unknown!)"  /* ensure a numeric unknown version  */
  #define CUSTOM_BUILD_STRING  "('VERSION' was not defined!)"
#endif

#define HDL_VERS_HERCULES          VERSION
#define HDL_SIZE_HERCULES   sizeof(VERSION)

VER_DLL_IMPORT void display_version       ( FILE* f, int httpfd, char* prog );
VER_DLL_IMPORT void display_build_options ( FILE* f, int httpfd );
VER_DLL_IMPORT void display_extpkg_vers   ( FILE* f, int httpfd );
VER_DLL_IMPORT int  get_buildinfo_strings ( const char*** pppszBldInfoStr );

#define HERCULES_COPYRIGHT \
       "(C) Copyright 1999-2016 by Roger Bowler, Jan Jaeger, and others"
#endif // _HERCULES_H_
