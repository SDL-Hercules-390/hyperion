/* SKEY.C       (C) Copyright "Fish" (David B. Trout), 2021          */
/*              Storage Key Functions                                */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#include "hstdinc.h"

#define _SKEY_C_
#define _HENGINE_DLL_

#include "hercules.h"
#include "opcode.h"
#include "inline.h"

/*-------------------------------------------------------------------*/
/*   ARCH_DEP section: compiled multiple times, once for each arch.  */
/*-------------------------------------------------------------------*/

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
/*             Primary ARCH_DEP Storage Key Functions                */
/*-------------------------------------------------------------------*/

extern inline BYTE  ARCH_DEP(  get_storage_key        )(              U64 abs            );
extern inline void  ARCH_DEP(  put_storage_key        )(              U64 abs, BYTE key  );
extern inline void  ARCH_DEP(  and_storage_key        )(              U64 abs, BYTE bits );
extern inline void  ARCH_DEP(  or_storage_key         )(              U64 abs, BYTE bits );
extern inline void  ARCH_DEP(  or_dev_storage_key     )( DEVBLK* dev, U64 abs, BYTE bits );
extern inline BYTE  ARCH_DEP(  get_dev_storage_key    )( DEVBLK* dev, U64 abs            );
extern inline BYTE* ARCH_DEP(  get_ptr_to_storekey    )(              U64 abs            );
extern inline BYTE  ARCH_DEP(  get_storekey_by_ptr    )( BYTE* skey_ptr                  );
INL_DLL_EXPORT // (function needed by dyncrypt via vstore.h)
       inline void  ARCH_DEP(  or_storage_key_by_ptr  )( BYTE* skey_ptr,       BYTE bits );

/*-------------------------------------------------------------------*/
/*       Page-size-specific functions used by SSK/SSKE et al.        */
/*-------------------------------------------------------------------*/

#if defined( FEATURE_BASIC_STORAGE_KEYS )
  extern inline BYTE*  ARCH_DEP(  get_ptr_to_2K_storekey  )(              U64 abs            );
  extern inline BYTE   ARCH_DEP(  get_2K_storage_key      )(              U64 abs            );
  extern inline void   ARCH_DEP(  put_2K_storage_key      )(              U64 abs, BYTE key  );
  extern inline void   ARCH_DEP(  and_2K_storage_key      )(              U64 abs, BYTE bits );
  extern inline void   ARCH_DEP(  or_2K_storage_key       )(              U64 abs, BYTE bits );
  INL_DLL_EXPORT // (function needed by qeth and zfcp)
         inline BYTE   ARCH_DEP(  get_dev_2K_storage_key  )( DEVBLK* dev, U64 abs            );
  INL_DLL_EXPORT // (function needed by qeth and zfcp)
         inline void   ARCH_DEP(  or_dev_2K_storage_key   )( DEVBLK* dev, U64 abs, BYTE bits );
#endif
#if defined( FEATURE_EXTENDED_STORAGE_KEYS )
  extern inline BYTE*  ARCH_DEP(  get_ptr_to_4K_storekey  )(              U64 abs            );
  extern inline BYTE   ARCH_DEP(  get_4K_storage_key      )(              U64 abs            );
  extern inline void   ARCH_DEP(  put_4K_storage_key      )(              U64 abs, BYTE key  );
  extern inline void   ARCH_DEP(  and_4K_storage_key      )(              U64 abs, BYTE bits );
  extern inline void   ARCH_DEP(  or_4K_storage_key       )(              U64 abs, BYTE bits );
  INL_DLL_EXPORT // (function needed by qeth and zfcp)
         inline BYTE   ARCH_DEP(  get_dev_4K_storage_key  )( DEVBLK* dev, U64 abs            );
  INL_DLL_EXPORT // (function needed by qeth and zfcp)
         inline void   ARCH_DEP(  or_dev_4K_storage_key   )( DEVBLK* dev, U64 abs, BYTE bits );
#endif

/*-------------------------------------------------------------------*/
/*  ARCH_DEP Storage Key helper functions sometimes called directly  */
/*-------------------------------------------------------------------*/

extern inline BYTE ARCH_DEP(  _get_storage_key      )(              U64 abs,            BYTE K );
extern inline void ARCH_DEP(  _put_storage_key      )(              U64 abs, BYTE key,  BYTE K );
extern inline void ARCH_DEP(  _or_storage_key       )(              U64 abs, BYTE bits, BYTE K );
extern inline void ARCH_DEP(  _and_storage_key      )(              U64 abs, BYTE bits, BYTE K );
extern inline void ARCH_DEP(  _or_dev_storage_key   )( DEVBLK* dev, U64 abs, BYTE bits, BYTE K );
extern inline BYTE ARCH_DEP(  _get_dev_storage_key  )( DEVBLK* dev, U64 abs,            BYTE K );

/*-------------------------------------------------------------------*/
/*          (delineates ARCH_DEP from non-arch_dep)                  */
/*-------------------------------------------------------------------*/

#if !defined( _GEN_ARCH )

  #if defined(              _ARCH_NUM_1 )
    #define   _GEN_ARCH     _ARCH_NUM_1
    #include "skey.c"
  #endif

  #if defined(              _ARCH_NUM_2 )
    #undef    _GEN_ARCH
    #define   _GEN_ARCH     _ARCH_NUM_2
    #include "skey.c"
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
/*                    Conditional SSKE helper                        */
/*-------------------------------------------------------------------*/

#if defined( _FEATURE_010_CONDITIONAL_SSKE_FACILITY )
  extern inline bool bypass_skey_update( REGS* regs, BYTE m3, BYTE oldkey, BYTE r1key );
#endif

  extern inline BYTE* _get_storekey_ptr( U64 abs, BYTE K );
  extern inline BYTE* _get_dev_storekey_ptr( DEVBLK* dev, U64 abs, BYTE K );
  extern inline BYTE* _get_storekey1_ptr( U64 abs );
  extern inline BYTE* _get_storekey2_ptr( U64 abs );
  extern inline BYTE* _get_dev_storekey1_ptr( DEVBLK* dev, U64 abs );
  extern inline BYTE* _get_dev_storekey2_ptr( DEVBLK* dev, U64 abs );

#endif /*!defined( _GEN_ARCH )*/
