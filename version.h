/*  VERSION.H   (C) Copyright Roger Bowler, 1999-2012                */
/*              (C) and others 2013-2023                             */
/*              Hercules Emulator Version definition                 */
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

#define HERCULES_COPYRIGHT \
       "(C) Copyright 1999-2024 by Roger Bowler, Jan Jaeger, and others"

#if !defined( _MSVC_ )

  // Due to autotool's insistance of defining 'VERSION' for us
  // via a #define within the 'config.h' header (the value of
  // which it derives from the 'configure.ac's "AM_INIT_AUTOMAKE"
  // statement) instead of letting us define it ourselves, we
  // must undefine it to the value that our '_dynamic_version'
  // script determied it should be (which it saved for us in the
  // 'DYNAMIC_VERSION' variable for obvious reasons).

  #undef  VERSION
  #define VERSION   DYNAMIC_VERSION
#endif

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

/* To be more compatible with HDL's "HDL_DEPENDENCY" macro we prefer
   to define a HDL_VERS_HERCULES value that contains ONLY the major,
   intermediate, and minor components and NOT the full version string
   (which also contains the git hash and other unwanted information).
*/
#define HDL_NAME_HERCULES   "HERCULES"
#define HDL_VERS_HERCULES   "SDL " QSTR( VERS_MAJ ) "." QSTR( VERS_INT ) QSTR( VERS_MIN )
#define HDL_SIZE_HERCULES   sizeof( HDL_VERS_HERCULES ) - 1

VER_DLL_IMPORT void init_sysblk_version_str_arrays( const char* prog );

VER_DLL_IMPORT void display_version       ( FILE* f, int httpfd, const char* prog );
VER_DLL_IMPORT void display_build_options ( FILE* f, int httpfd );
VER_DLL_IMPORT void display_extpkg_vers   ( FILE* f, int httpfd );
VER_DLL_IMPORT int  get_buildinfo_strings ( const char*** pppszBldInfoStr );

#endif // _HERCULES_H_
