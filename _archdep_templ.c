/* XXXXXXX.C    (C) Copyright Your name or company, YYYY             */
/*              Short module description goes here                   */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* This module implements blah, blah, blah...                        */
/* ... described in the manual XXnn-nnnn-vv "Name of Manual"...      */
/* Basically a short description of this module's purpose.           */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* Optional Additional credits section:                              */
/*   Name of other people who have contributed to this module        */
/*   and possibly a description of what they contributed,            */
/*   as well as any people who helped or inspired you...             */
/*-------------------------------------------------------------------*/

#if 0 // ** REMOVE ME! ** REMOVE ME! ** REMOVE ME! ** REMOVE ME! **

#include "hstdinc.h"        // (MUST ALWAYS BE FIRST)

#define _XXXXXXX_C_         // (ALWAYS) (see hexterns.h)
#define _XXXXX_DLL_         // (ALWAYS) (usually _HENGINE_DLL_)

#include "hercules.h"       // (ALWAYS) (needed by most modules)

//#include "xxxxxx.h"       // (other needed includes go here)
//#include "xxxxxx.h"       // (other needed includes go here)
//#include "xxxxxx.h"       // (other needed includes go here)

/*-------------------------------------------------------------------*/
/*   ARCH_DEP section: compiled multiple times, once for each arch.  */
/*-------------------------------------------------------------------*/

// Note: non-arch-dep static functions should ideally be at the end
// but you can put them here by using an ifdef similar to the below

#if !defined( COMPILE_THIS_ONLY_ONCE )
#define       COMPILE_THIS_ONLY_ONCE

/*-------------------------------------------------------------------*/
/*                non_arch_dep_static_helper_one                     */
/*-------------------------------------------------------------------*/
static int  non_arch_dep_static_helper_two( xxx... );   // (fwd ref)
static int  non_arch_dep_static_helper_one( xxx... )
{
    //------------------------------------------------------------
    //                     IMPORTANT
    // You MUST NOT use any feature.h macros in your non-arch_dep
    // functions. See the IMPORTANT PROGRAMMING NOTE at the end.
    //------------------------------------------------------------
    return 0;
}
#endif // COMPILE_THIS_ONLY_ONCE

//-------------------------------------------------------------------
//                      ARCH_DEP() code
//-------------------------------------------------------------------
// ARCH_DEP (build-architecture / FEATURE-dependent) functions here.
// All BUILD architecture dependent (ARCH_DEP) function are compiled
// multiple times (once for each defined build architecture) and each
// time they are compiled with a different set of FEATURE_XXX defines
// appropriate for that architecture. Use #ifdef FEATURE_XXX guards
// to check whether the current BUILD architecture has that given
// feature #defined for it or not. WARNING: Do NOT use _FEATURE_XXX.
// The underscore feature #defines mean something else entirely. Only
// test for FEATURE_XXX. (WITHOUT the underscore)
//-------------------------------------------------------------------

/*-------------------------------------------------------------------*/
/*                    arch_dep_helper_func                           */
/*-------------------------------------------------------------------*/
static int  ARCH_DEP( arch_dep_helper_func )( function arguments... )
{
    return 0;
}

/*-------------------------------------------------------------------*/
/* Advanced function description including purpose...                */
/*                                                                   */
/* Input:                                                            */
/*      aaaaa   Some input parameter                                 */
/*      bbbbb   Another input parameter                              */
/*                                                                   */
/* Output:                                                           */
/*      ccccc   Updated parameter                                    */
/*                                                                   */
/* Returns:                                                          */
/*      Description of return code if any                            */
/*                                                                   */
/* Detailed description of functionality... Detailed description     */
/* of functionality... Detailed description of functionality...      */
/* Detailed description of functionality... Detailed description     */
/* of functionality... Detailed description of functionality...      */
/*                                                                   */
/*-------------------------------------------------------------------*/
int ARCH_DEP( foobar_func )( REGS* regs, xxx... )
{
    RADR  raddr;
    VADR  vaddr;
    int   rc;


    /* call non-archdep helper function */
    foo();


    /* call another arch-dep function */
    ARCH_DEP( some_other_func )( regs, &raddr, &vaddr );


#if defined( FEATURE_nnn_XXXX_FACILITY )
    if (FACILITY_ENABLED( nnn_XXXX, regs ))
    {
        rc = 0;
    }
    else
        rc = -1;
#endif // defined( FEATURE_nnn_XXXX_FACILITY )


#if defined( FEATURE_XXX )
    /* Code to handle this feature... */
#endif // defined( FEATURE_XXX )


    return rc;

} /* end function foobar_func */

/*-------------------------------------------------------------------*/
/*          (delineates ARCH_DEP from non-arch_dep)                  */
/*-------------------------------------------------------------------*/

#if !defined( _GEN_ARCH )

  // All of the architecture dependent (i.e. "ARCH_DEP") functions
  // for the default "_ARCH_NUM_0" build architecture have now just
  // been built (usually 370).

  // Now we need to build the same architecture dependent "ARCH_DEP"
  // functions for all of the OTHER build architectures that remain
  // (usually S/390 and z/Arch), so we #include ourselves again but
  // with the next build archiecture #defined instead...

  #if defined(              _ARCH_NUM_1 )     // (usually 390)
    #define   _GEN_ARCH     _ARCH_NUM_1
    #include "ourselves.c"
  #endif

  #if defined(              _ARCH_NUM_2 )     // (usually 900)
    #undef    _GEN_ARCH
    #define   _GEN_ARCH     _ARCH_NUM_2
    #include "ourselves.c"
  #endif

/*-------------------------------------------------------------------*/
/*          (delineates ARCH_DEP from non-arch_dep)                  */
/*-------------------------------------------------------------------*/


/*-------------------------------------------------------------------*/
/*  non-ARCH_DEP section: compiled only ONCE after last arch built   */
/*-------------------------------------------------------------------*/
/*  Note: the last architecture has been built so the normal non-    */
/*  underscore FEATURE values are now #defined according to the      */
/*  LAST built architecture just built (usually zarch = 900). This   */
/*  means from this point onward (to the end of file) you should     */
/*  ONLY be testing the underscore _FEATURE values to see if the     */
/*  given feature was defined for *ANY* of the build architectures.  */
/*-------------------------------------------------------------------*/


/*-------------------------------------------------------------------*/
/*               non_arch_dep_static_helper_two                      */
/*-------------------------------------------------------------------*/
static int  non_arch_dep_static_helper_two( xxx... )
{
    //------------------------------------------------------------
    //                     IMPORTANT
    // You MUST NOT use any feature.h macros in your non-arch_dep
    // functions. See the IMPORTANT PROGRAMMING NOTE just below.
    //------------------------------------------------------------
    return 0;
}


//-------------------------------------------------------------------
//                      _FEATURE_XXX code
//-------------------------------------------------------------------
// Place any _FEATURE_XXX depdendent functions (WITH the underscore)
// here. You may need to define such functions whenever one or more
// build architectures has a given FEATURE_XXX (WITHOUT underscore)
// defined for it. The underscore means AT LEAST ONE of the build
// architectures #defined that feature. (See featchk.h) You must NOT
// use any #ifdef FEATURE_XXX here. Test for ONLY for _FEATURE_XXX.
// The functions in this area are compiled ONCE (only ONE time) and
// ONLY one time but are always compiled LAST after everything else.
//-------------------------------------------------------------------


/*********************************************************************/
/*                  IMPORTANT PROGRAMMING NOTE                       */
/*********************************************************************/
/*                                                                   */
/* It is CRITICALLY IMPORTANT to not use any architecture dependent  */
/* macros anywhere in any of your non-arch_dep functions. This means */
/* you CANNOT use GREG, RADR, VADR, etc. anywhere in your function,  */
/* nor can you call "ARCH_DEP(func)(args)" anywhere in your code!    */
/*                                                                   */
/* Basically you MUST NOT use any architecture dependent macro that  */
/* is #defined in the "feature.h" header.  If you you need to use    */
/* any of them, then your function MUST be an "ARCH_DEP" function    */
/* that is placed within the ARCH_DEP section at the beginning of    */
/* this module where it can be compiled multiple times, once for     */
/* each of the supported architectures so the macro gets #defined    */
/* to its proper value for the architecture. YOU HAVE BEEN WARNED.   */
/*                                                                   */
/*********************************************************************/


#if defined( _FEATURE_XXX )       // (is FEATURE_XXX code needed?)
/*-------------------------------------------------------------------*/
/* This function is needed to support FEATURE_XXX.                   */
/* This is an example of a RUN-TIME-ARCHITECTURE dependent function  */
/*-------------------------------------------------------------------*/
int feature_xxx_func( REGS* regs )
{
    int rc;

    switch( regs->arch_mode )  // (switch based on RUN-TIME archmode)
    {

#if defined( _370 )
    case  ARCH_370_IDX:  rc = s370_foobar_func( regs, ... ); break;
#endif


#if defined( _390 )
    case  ARCH_390_IDX:  rc = s390_foobar_func( regs, ... ); break;
#endif


#if defined( _900 )
    case  ARCH_900_IDX:  rc = z900_foobar_func( regs, ... ); break;
#endif

    default: CRASH();
    }

    return rc;
}
#endif // _FEATURE_XXX

#endif // !defined(_GEN_ARCH)

#endif // ** REMOVE ME! ** REMOVE ME! ** REMOVE ME! ** REMOVE ME! **
