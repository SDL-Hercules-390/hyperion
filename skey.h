/* SKEY.H       (C) Copyright "Fish" (David B. Trout), 2021          */
/*              Storage Key Functions                                */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#ifndef _SKEY_H
#define _SKEY_H

/*-------------------------------------------------------------------*/
/*  non-ARCH_DEP section of header: due to above _SKEY_H guard,      */
/*  the below code is compiled only once, BEFORE the  very first     */
/*  architecture is ever built.                                      */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/*  Lowest-level NON-arch-dep storekey helper macros and functions   */
/*-------------------------------------------------------------------*/

#if defined( OPTION_ATOMIC_SKEYS )
  #define OR_SKEY(  ptr, op )       H_ATOMIC_OP( ptr, op, or,  Or,  | )
  #define AND_SKEY( ptr, op )       H_ATOMIC_OP( ptr, op, and, And, & )
#else
  #define OR_SKEY(  ptr, op )       (*(ptr) |= (op))
  #define AND_SKEY( ptr, op )       (*(ptr) &= (op))
#endif

#if defined( OPTION_SKEY_ABS_CHECK ) // (helps to catch instruction bugs)
  #define ABS_CHECK( abs )      if ((abs) >= sysblk.mainsize) CRASH()
#else
  #define ABS_CHECK( abs )
#endif

#define STOREKEY(  abs, p )   (p)->storkeys[ ((abs) >> _STORKEY_ARRAY_SHIFTAMT)      ]
#define STOREKEY1( abs, p )   (p)->storkeys[ ((abs) >> _STORKEY_ARRAY_SHIFTAMT) & ~1 ]
#define STOREKEY2( abs, p )   (p)->storkeys[ ((abs) >> _STORKEY_ARRAY_SHIFTAMT) |  1 ]

inline BYTE* _get_storekey_ptr( U64 abs, BYTE K )
{
    ABS_CHECK( abs );
    return (4 == K) ? &STOREKEY1( abs, &sysblk ) // (see feature.h PROGRAMMING NOTE)
                    : &STOREKEY(  abs, &sysblk );
}

inline BYTE* _get_dev_storekey_ptr( DEVBLK* dev, U64 abs, BYTE K )
{
    ABS_CHECK( abs );
    return (4 == K) ? &STOREKEY1( abs, dev ) // (see feature.h PROGRAMMING NOTE)
                    : &STOREKEY(  abs, dev );
}

inline BYTE* _get_storekey1_ptr( U64 abs )
{
    ABS_CHECK( abs );
    return &STOREKEY1( abs, &sysblk );
}

inline BYTE* _get_storekey2_ptr( U64 abs )
{
    ABS_CHECK( abs );
    return &STOREKEY2( abs, &sysblk );
}

inline BYTE* _get_dev_storekey1_ptr( DEVBLK* dev, U64 abs )
{
    ABS_CHECK( abs );
    return &STOREKEY1( abs, dev );
}

inline BYTE* _get_dev_storekey2_ptr( DEVBLK* dev, U64 abs )
{
    ABS_CHECK( abs );
    return &STOREKEY2( abs, dev );
}

#endif // defined( _SKEY_H )

/*-------------------------------------------------------------------*/
/*      ARCH_DEP section: the below section of this header           */
/*       is compiled multiple times, ONCE for each arch.             */
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

#if (ARCH_370_IDX == ARCH_IDX && !defined( DID_370_SKEY_H )) \
 || (ARCH_390_IDX == ARCH_IDX && !defined( DID_390_SKEY_H )) \
 || (ARCH_900_IDX == ARCH_IDX && !defined( DID_900_SKEY_H ))

#if      defined( FEATURE_2K_STORAGE_KEYS )
  #undef  SKEY_K
  #define SKEY_K          2
#else // defined( FEATURE_4K_STORAGE_KEYS )
  #undef  SKEY_K
  #define SKEY_K          4
#endif

//  Test for 4K-Byte-Block-Facility only applies to System/370
//  which optionally supported either 2K pages or 4K pages.
//  ESA/390 and z/Arch on the other hand only supports 4K pages.

#undef  IS_DOUBLE_KEYED_4K_BYTE_BLOCK
#define IS_DOUBLE_KEYED_4K_BYTE_BLOCK( K )        \
  (K == 4 && STORAGE_KEY_2K_PAGESIZE == _STORKEY_ARRAY_UNITSIZE)

/*-------------------------------------------------------------------*/
/*  ARCH_DEP Storage Key helper functions sometimes called directly  */
/*-------------------------------------------------------------------*/

inline BYTE ARCH_DEP( _get_storage_key )( U64 abs, BYTE K )
{
    BYTE skey;
    UNREFERENCED( K ); // (for FEATURE_4K_STORAGE_KEYS case)
    if (IS_DOUBLE_KEYED_4K_BYTE_BLOCK( K ))
    {
        skey  = *_get_storekey1_ptr( abs );
        skey |= *_get_storekey2_ptr( abs ) & ~(STORKEY_KEY);
    }
    else
        skey  = *_get_storekey_ptr(  abs, K );
    return skey;
}

inline BYTE ARCH_DEP( _get_dev_storage_key )( DEVBLK* dev, U64 abs, BYTE K )
{
    BYTE skey;
    UNREFERENCED( K ); // (for FEATURE_4K_STORAGE_KEYS case)
    if (IS_DOUBLE_KEYED_4K_BYTE_BLOCK( K ))
    {
        skey  = *_get_dev_storekey1_ptr( dev, abs );
        skey |= *_get_dev_storekey2_ptr( dev, abs ) & ~(STORKEY_KEY);
    }
    else
        skey  = *_get_dev_storekey_ptr(  dev, abs, K );
    return skey;
}

inline void ARCH_DEP( _put_storage_key )( U64 abs, BYTE key, BYTE K )
{
    UNREFERENCED( K ); // (for FEATURE_4K_STORAGE_KEYS case)
    if (IS_DOUBLE_KEYED_4K_BYTE_BLOCK( K ))
    {
        *_get_storekey1_ptr( abs ) = key;
        *_get_storekey2_ptr( abs ) = key;
    }
    else
    {
        *_get_storekey_ptr(  abs, K ) = key;
    }
}

inline void ARCH_DEP( _and_storage_key )( U64 abs, BYTE bits, BYTE K )
{
    UNREFERENCED( K ); // (for FEATURE_4K_STORAGE_KEYS case)
    if (IS_DOUBLE_KEYED_4K_BYTE_BLOCK( K ))
    {
        BYTE* skey1_ptr = _get_storekey1_ptr( abs );
        BYTE* skey2_ptr = _get_storekey2_ptr( abs );
        AND_SKEY( skey1_ptr, ~bits );
        AND_SKEY( skey2_ptr, ~bits );
    }
    else
    {
        BYTE* skey_ptr  = _get_storekey_ptr(  abs, K );
        AND_SKEY( skey_ptr, ~bits );
    }
}

inline void ARCH_DEP( _or_storage_key )( U64 abs, BYTE bits, BYTE K )
{
    UNREFERENCED( K ); // (for FEATURE_4K_STORAGE_KEYS case)
    if (IS_DOUBLE_KEYED_4K_BYTE_BLOCK( K ))
    {
        BYTE* skey1_ptr = _get_storekey1_ptr( abs );
        BYTE* skey2_ptr = _get_storekey2_ptr( abs );
        OR_SKEY( skey1_ptr, bits );
        OR_SKEY( skey2_ptr, bits );
    }
    else
    {
        BYTE* skey_ptr  = _get_storekey_ptr( abs, K );
        OR_SKEY( skey_ptr, bits );
    }
}

inline void ARCH_DEP( _or_dev_storage_key )( DEVBLK* dev, U64 abs, BYTE bits, BYTE K )
{
    UNREFERENCED( K ); // (for FEATURE_4K_STORAGE_KEYS case)
    if (IS_DOUBLE_KEYED_4K_BYTE_BLOCK( K ))
    {
        BYTE* skey1_ptr = _get_dev_storekey1_ptr( dev, abs );
        BYTE* skey2_ptr = _get_dev_storekey2_ptr( dev, abs );
        OR_SKEY( skey1_ptr, bits );
        OR_SKEY( skey2_ptr, bits );
    }
    else
    {
        BYTE* skey_ptr  = _get_dev_storekey_ptr(  dev, abs, K );
        OR_SKEY( skey_ptr, bits );
    }
}

/*-------------------------------------------------------------------*/
/*       Page-size-specific functions used by SSK/SSKE et al.        */
/*-------------------------------------------------------------------*/

#if defined( FEATURE_BASIC_STORAGE_KEYS )
  inline BYTE* ARCH_DEP( get_ptr_to_2K_storekey ) (              U64 abs            ) { return           _get_storekey_ptr     (      abs, 2 );                         }
  inline BYTE  ARCH_DEP( get_2K_storage_key )     (              U64 abs            ) { return ARCH_DEP( _get_storage_key     )(      abs, 2 )  & ~STORKEY_BADFRM;      }
  inline void  ARCH_DEP( put_2K_storage_key )     (              U64 abs, BYTE key  ) {        ARCH_DEP( _put_storage_key     )(      abs, key  & ~STORKEY_BADFRM, 2 ); }
  inline void  ARCH_DEP( and_2K_storage_key )     (              U64 abs, BYTE bits ) {        ARCH_DEP( _and_storage_key     )(      abs, bits & ~STORKEY_BADFRM, 2 ); }
  inline void  ARCH_DEP( or_2K_storage_key )      (              U64 abs, BYTE bits ) {        ARCH_DEP( _or_storage_key      )(      abs, bits & ~STORKEY_BADFRM, 2 ); }
SKEY_INL_DLL_IMPORT // (function needed by qeth and zfcp)
  inline BYTE  ARCH_DEP( get_dev_2K_storage_key ) ( DEVBLK* dev, U64 abs            ) { return ARCH_DEP( _get_dev_storage_key )( dev, abs, 2 )  & ~STORKEY_BADFRM;      }
SKEY_INL_DLL_IMPORT // (function needed by qeth and zfcp)
  inline void  ARCH_DEP( or_dev_2K_storage_key )  ( DEVBLK* dev, U64 abs, BYTE bits ) {        ARCH_DEP( _or_dev_storage_key  )( dev, abs, bits & ~STORKEY_BADFRM, 2 ); }
#endif
#if defined( FEATURE_EXTENDED_STORAGE_KEYS )
  inline BYTE* ARCH_DEP( get_ptr_to_4K_storekey ) (              U64 abs            ) { return           _get_storekey_ptr     (      abs, 4 );                         }
  inline BYTE  ARCH_DEP( get_4K_storage_key )     (              U64 abs            ) { return ARCH_DEP( _get_storage_key     )(      abs, 4 )  & ~STORKEY_BADFRM;      }
  inline void  ARCH_DEP( put_4K_storage_key )     (              U64 abs, BYTE key  ) {        ARCH_DEP( _put_storage_key     )(      abs, key  & ~STORKEY_BADFRM, 4 ); }
  inline void  ARCH_DEP( and_4K_storage_key )     (              U64 abs, BYTE bits ) {        ARCH_DEP( _and_storage_key     )(      abs, bits & ~STORKEY_BADFRM, 4 ); }
  inline void  ARCH_DEP( or_4K_storage_key )      (              U64 abs, BYTE bits ) {        ARCH_DEP( _or_storage_key      )(      abs, bits & ~STORKEY_BADFRM, 4 ); }
SKEY_INL_DLL_IMPORT // (function needed by qeth and zfcp)
  inline BYTE  ARCH_DEP( get_dev_4K_storage_key ) ( DEVBLK* dev, U64 abs            ) { return ARCH_DEP( _get_dev_storage_key )( dev, abs, 4 )  & ~STORKEY_BADFRM;      }
SKEY_INL_DLL_IMPORT // (function needed by qeth and zfcp)
  inline void  ARCH_DEP( or_dev_4K_storage_key )  ( DEVBLK* dev, U64 abs, BYTE bits ) {        ARCH_DEP( _or_dev_storage_key  )( dev, abs, bits & ~STORKEY_BADFRM, 4 ); }
#endif

/*-------------------------------------------------------------------*/
/*             Primary ARCH_DEP Storage Key Functions                */
/*-------------------------------------------------------------------*/

inline BYTE ARCH_DEP( get_storage_key )( U64 abs )
{
#if defined( FEATURE_2K_STORAGE_KEYS )
    return ARCH_DEP( get_2K_storage_key )( abs );
#else
    return ARCH_DEP( get_4K_storage_key )( abs );
#endif
}

inline BYTE ARCH_DEP( get_dev_storage_key )( DEVBLK* dev, U64 abs )
{
#if defined( FEATURE_2K_STORAGE_KEYS )
    return ARCH_DEP( get_dev_2K_storage_key )( dev, abs );
#else
    return ARCH_DEP( get_dev_4K_storage_key )( dev, abs );
#endif
}

inline void ARCH_DEP( put_storage_key )( U64 abs, BYTE key )
{
#if defined( FEATURE_2K_STORAGE_KEYS )
    ARCH_DEP( put_2K_storage_key )( abs, key );
#else
    ARCH_DEP( put_4K_storage_key )( abs, key );
#endif
}

inline void ARCH_DEP( and_storage_key )( U64 abs, BYTE bits )
{
#if defined( FEATURE_2K_STORAGE_KEYS )
    ARCH_DEP( and_2K_storage_key )( abs, bits );
#else
    ARCH_DEP( and_4K_storage_key )( abs, bits );
#endif
}

inline void ARCH_DEP( or_storage_key )( U64 abs, BYTE bits )
{
#if defined( FEATURE_2K_STORAGE_KEYS )
    ARCH_DEP( or_2K_storage_key )( abs, bits );
#else
    ARCH_DEP( or_4K_storage_key )( abs, bits );
#endif
}

inline void ARCH_DEP( or_dev_storage_key )( DEVBLK* dev, U64 abs, BYTE bits )
{
#if defined( FEATURE_2K_STORAGE_KEYS )
    ARCH_DEP( or_dev_2K_storage_key )( dev, abs, bits );
#else
    ARCH_DEP( or_dev_4K_storage_key )( dev, abs, bits );
#endif
}

inline BYTE* ARCH_DEP( get_ptr_to_storekey )( U64 abs )
{
#if defined( FEATURE_2K_STORAGE_KEYS )
    return ARCH_DEP( get_ptr_to_2K_storekey )( abs );
#else
    return ARCH_DEP( get_ptr_to_4K_storekey )( abs );
#endif
}

inline BYTE ARCH_DEP( get_storekey_by_ptr )( BYTE* skey_ptr )
{
    U64 abs = (skey_ptr - sysblk.storkeys) << _STORKEY_ARRAY_SHIFTAMT;
#if defined( FEATURE_2K_STORAGE_KEYS )
    return ARCH_DEP( get_2K_storage_key )( abs );
#else
    return ARCH_DEP( get_4K_storage_key )( abs );
#endif
}

SKEY_INL_DLL_IMPORT // (function needed by dyncrypt via vstore.h)
inline void ARCH_DEP( or_storage_key_by_ptr )( BYTE* skey_ptr, BYTE bits )
{
    U64 abs = (skey_ptr - sysblk.storkeys) << _STORKEY_ARRAY_SHIFTAMT;
#if defined( FEATURE_2K_STORAGE_KEYS )
    ARCH_DEP( or_2K_storage_key )( abs, bits );
#else
    ARCH_DEP( or_4K_storage_key )( abs, bits );
#endif
}

/*-------------------------------------------------------------------*/
/*      We only need to compile the above section of this header     */
/*                only ONCE for each architecture                    */
/*-------------------------------------------------------------------*/

#if      ARCH_370_IDX == ARCH_IDX
  #define DID_370_SKEY_H
#endif
#if      ARCH_390_IDX == ARCH_IDX
  #define DID_390_SKEY_H
#endif
#if      ARCH_900_IDX == ARCH_IDX
  #define DID_900_SKEY_H
#endif

#endif // #if (ARCH_xxx_IDX == ARCH_IDX && !defined( DID_xxx_SKEY_H )) ...

/*-------------------------------------------------------------------*/
/*      This section of the header is compiled ONLY ONCE             */
/*      after *ALL* build architecture FEATURES are defined          */
/*-------------------------------------------------------------------*/

#if defined ( DID_FEATCHK_PASS_1 ) // (last architecture built yet?)

#if defined( _FEATURE_010_CONDITIONAL_SSKE_FACILITY )
#ifndef DID_BYPASS_SKEY_UPDATE
#define DID_BYPASS_SKEY_UPDATE
/*-------------------------------------------------------------------*/
/*     "Should Storage Key Update be BYPASSED?" helper function      */
/*-------------------------------------------------------------------*/
/*  Common Conditional-SSKE Facility decision-making logic used by   */
/*  both the SSKE instruction as well as the PFMF instruction.       */
/*                                                                   */
/*      regs    Register context                                     */
/*      m3      Operand-3 mask field from SSKE instruction,          */
/*              or PFMF instruction r1 register bits 52-55.          */
/*      oldkey  Contents of storage key before modification          */
/*      r1key   Register r1 storage key comparison value             */
/*                                                                   */
/*  Returns:                                                         */
/*                                                                   */
/*      true    Updating of storage key *SHOULD* be bypassed         */
/*              (i.e. do *NOT* update the storage key)               */
/*                                                                   */
/*      false   Updating of storage key should *NOT* be bypassed     */
/*              (i.e. perform normal storage key updating)           */
/*                                                                   */
/*-------------------------------------------------------------------*/
inline bool bypass_skey_update( REGS* regs, BYTE m3, BYTE oldkey, BYTE r1key )
{
    /* If the Conditional-SSKE Facility is not installed, or if both
       the MR and MC bits are both zero, OR if storage key and fetch
       bits are not the same as the values in R1 register bits 56-60,
       then update the key normally (i.e. do NOT bypass updating key).
    */
    if (0
        || !FACILITY_ENABLED( 010_CONDITIONAL_SSKE, regs )
        || (m3 & (SSKE_MASK_MR | SSKE_MASK_MC)) == 0
        || (oldkey & (STORKEY_KEY | STORKEY_FETCH)) !=
           (r1key  & (STORKEY_KEY | STORKEY_FETCH))
    )
        return false; /* (DON'T bypass updating this page's key) */

    /* If both the MR and MC mask bits are both one, or the MR bit is
       zero and the reference bit is equal to bit 61 of register r1,
       or the MC bit is zero and the change bit is equal to bit 62 of
       register r1, then BYPASS updating (do NOT update) storage key.
    */
    if (0
        || ((m3 & (SSKE_MASK_MR | SSKE_MASK_MC))
              ==  (SSKE_MASK_MR | SSKE_MASK_MC))
        || ((m3 & SSKE_MASK_MR) == 0 && (oldkey & STORKEY_REF   ) == (r1key & STORKEY_REF   ))
        || ((m3 & SSKE_MASK_MC) == 0 && (oldkey & STORKEY_CHANGE) == (r1key & STORKEY_CHANGE))
    )
        return true; /***>>  BYPASS updating this page's key <<***/

    return false;     /* (DON'T bypass updating this page's key) */
}
#endif // DID_BYPASS_SKEY_UPDATE
#endif /* defined( _FEATURE_010_CONDITIONAL_SSKE_FACILITY ) */

#endif // defined ( DID_FEATCHK_PASS_1 )

/* end of SKEY.H */
