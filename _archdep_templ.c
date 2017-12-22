/* XXXXXXX.C    (C) Copyright Your name or company, YYYY             */
/*              Short module description goes here                   */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#if 0 // ** REMOVE ME! ** REMOVE ME! ** REMOVE ME! ** REMOVE ME! **

/*-------------------------------------------------------------------*/
/* This is an example (sample) source module containing architecture */
/* dependent code. Copy it to whatever name you require, remove the  */
/* #if 0 and #endif to activate it, and change it as needed. Replace */
/* the comments you are reading with a more detailed description of  */
/* the purpose of your module and how it accomplishes its mission.   */
/* Also mention reference manuals used during its development too.   */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* Additional credits:                                               */
/*    List here other people who helped or inspired you...           */
/*    List here other people who helped or inspired you...           */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"        // (MUST ALWAYS BE FIRST!)

#define _XXXXXXX_C_         // (ALWAYS) (see hexterns.h)
#define _XXXXX_DLL_         // (ALWAYS) (usually _HENGINE_DLL_)

#include "hercules.h"       // (ALWAYS) (needed by most modules)

//#include "xxxxxx.h"       // (other needed includes go here)
//#include "xxxxxx.h"       // (other needed includes go here)
//#include "xxxxxx.h"       // (other needed includes go here)

#ifndef _WE_DID_THIS_ALREADY_
#define _WE_DID_THIS_ALREADY_

//-------------------------------------------------------------------
//                  code compiled only one time
//-------------------------------------------------------------------
// Place functions that are NEITHER *build* architecture dependent
// nor architectural FEATURE (nor _FEATURE) dependent here. These
// function get compiled ONE TIME and ONLY one time. Note: you must
// NOT use any build architecture dependent values in any of these
// functions since only the values for the FIRST build architecture
// have been #defined at this point (usually S370). These functions
// may be static or not, or DLL_EXPORT or not, it doesn't matter,
// but static helper functions used by the functions which follow
// are the type of functions which are usually placed here.
//-------------------------------------------------------------------

/*-------------------------------------------------------------------*/
/* Simple function description/purpose                               */
/*-------------------------------------------------------------------*/
static int foo()
{
}

/*-------------------------------------------------------------------*/
/* Simple function description/purpose                               */
/*-------------------------------------------------------------------*/
static int bar()
{
}

#endif /* _WE_DID_THIS_ALREADY_ */

//-------------------------------------------------------------------
//                      ARCH_DEP() code
//-------------------------------------------------------------------
// ARCH_DEP (build-architecture / FEATURE-dependent) functions here.
// All BUILD architecture dependent (ARCH_DEP) function are compiled
// multiple times (once for each defined build architecture) and each
// time they are compiled with a different set of FEATURE_XXX defines
// appropriate for that architecture. Use #ifdef FEATURE_XXX guards
// to check whether the current BUILD architecture has that given
// feature #defined for it or not. WARNING! Do NOT use _FEATURE_XXX!
// The underscore feature #defines mean something else entirely! Only
// test for FEATURE_XXX! (WITHOUT the underscore!) These functions
// may be static or not, or DLL_EXPORT or not, it doesn't matter.
//-------------------------------------------------------------------

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
/*-------------------------------------------------------------------*/
int ARCH_DEP( foobar_func )( RADR *raddr, VADR vaddr, REGS *regs )
{
    /* call non-archdep helper function */
    int foobar = foo();

    /* call another arch dep function */
    ARCH_DEP( some_other_func )( vaddr, regs );

#if defined( FEATURE_XXX )
    /* Code to handle this feature... */
#endif // defined( FEATURE_XXX )

    return foobar;

} /* end function foobar_func */

/*-------------------------------------------------------------------*/
/* Compile ARCH_DEP() functions for other build architectures...     */
/*-------------------------------------------------------------------*/

#if !defined(_GEN_ARCH)             // (first time here?)

#if defined(_ARCH_NUM_1)
 #define  _GEN_ARCH _ARCH_NUM_1      // (set next build architecture)
 #include "_archdep_templ.c"        // (compile ourselves again)
#endif

#if defined(_ARCH_NUM_2)
 #undef   _GEN_ARCH
 #define  _GEN_ARCH _ARCH_NUM_2      // (set next build architecture)
 #include "_archdep_templ.c"        // (compile ourselves again)
#endif

//-------------------------------------------------------------------
//                      _FEATURE_XXX code
//-------------------------------------------------------------------
// Place any _FEATURE_XXX depdendent functions (WITH the underscore!)
// here. You may need to define such functions whenever one or more
// build architectures has a given FEATURE_XXX (WITHOUT underscore!)
// defined for it. The underscore means AT LEAST ONE of the build
// architectures #defined that feature. (See featchk.h) You must NOT
// use any #ifdef FEATURE_XXX here! Test for ONLY for _FEATURE_XXX!
// The functions in this area are compiled ONCE (only ONE time) and
// ONLY one time but are always compiled LAST after everything else.
// These functions may be static or not, or DLL_EXPORT or not, it
// doesn't matter.
//-------------------------------------------------------------------

#if defined( _FEATURE_XXX )       // (is FEATURE_XXX code needed?)
/*-------------------------------------------------------------------*/
/* This function is needed to support FEATURE_XXX.                   */
/* This is an example of a RUN-TIME-ARCHITECTURE dependent function  */
/*-------------------------------------------------------------------*/
int feature_xxx_func()
{
    //switch( sysblk.arch_mode )  // (switch based on RUN-TIME archmode)
    switch( regs->arch_mode )  // (switch based on RUN-TIME archmode)
    {
#if defined( _370 )
    case ARCH_370_IDX:
        return s370_foobar_func( ....arguments go here.... );
#endif
#if defined( _390 )
    case ARCH_390_IDX:
        return s390_foobar_func( ....arguments go here.... );
#endif
#if defined( _900 )
    case ARCH_900_IDX:
        return z900_foobar_func( ....arguments go here.... );
#endif
    }

    return 0;
}
#endif // _FEATURE_XXX

#endif // !defined(_GEN_ARCH)

#endif // ** REMOVE ME! ** REMOVE ME! ** REMOVE ME! ** REMOVE ME! **
