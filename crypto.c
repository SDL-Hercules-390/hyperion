/* CRYPTO.C     (c) Copyright Jan Jaeger, 2000-2012                  */
/*              Dummy Cryptographic Instructions                     */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#include "hstdinc.h"

#define _CRYPTO_C_

#include "hercules.h"

#ifndef _HENGINE_DLL_
#define _HENGINE_DLL_
#endif

#ifndef _CRYPTO_C_
#define _CRYPTO_C_
#endif

#include "opcode.h"

//efine    WRAPPINGKEYS_DEBUG       // (#define for debugging)

/*-------------------------------------------------------------------*/
/*   ARCH_DEP section: compiled multiple times, once for each arch.  */
/*-------------------------------------------------------------------*/
/*  The dyncrypt.dll (dyncrypt.c) HDL module is automatically        */
/*  pre-loaded by 'hdl_main' during impl/startup and redirects       */
/*  all crypto instructions in the opcode table to itself rather     */
/*  than here. The below dummy instructions are simply to prevent    */
/*  unresolved externs from occuring when 'hengine.dll' is linked.   */
/*-------------------------------------------------------------------*/

#if defined( FEATURE_017_MSA_FACILITY )

  HDL_UNDEF_INST( cipher_message                      )
  HDL_UNDEF_INST( cipher_message_with_chaining        )
  HDL_UNDEF_INST( compute_intermediate_message_digest )
  HDL_UNDEF_INST( compute_last_message_digest         )
  HDL_UNDEF_INST( compute_message_authentication_code )

#endif

/*-------------------------------------------------------------------*/

#if defined( FEATURE_076_MSA_EXTENSION_FACILITY_3 )

  HDL_UNDEF_INST( perform_cryptographic_key_management_operation )

#endif

/*-------------------------------------------------------------------*/

#if defined( FEATURE_077_MSA_EXTENSION_FACILITY_4 )

  HDL_UNDEF_INST( perform_cryptographic_computation   )
  HDL_UNDEF_INST( cipher_message_with_cipher_feedback )
  HDL_UNDEF_INST( cipher_message_with_output_feedback )
  HDL_UNDEF_INST( cipher_message_with_counter         )

#endif

/*-------------------------------------------------------------------*/
/*          (delineates ARCH_DEP from non-arch_dep)                  */
/*-------------------------------------------------------------------*/

#if !defined( _GEN_ARCH )

  #if defined(              _ARCH_NUM_1 )
    #define   _GEN_ARCH     _ARCH_NUM_1
    #include "crypto.c"
  #endif

  #if defined(              _ARCH_NUM_2 )
    #undef    _GEN_ARCH
    #define   _GEN_ARCH     _ARCH_NUM_2
    #include "crypto.c"
  #endif

/*-------------------------------------------------------------------*/
/*            (delineates ARCH_DEP from non-arch_dep)                */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/*  non-ARCH_DEP section: compiled only ONCE after last arch built   */
/*-------------------------------------------------------------------*/
/*  Note: the last architecture has been built so the normal non-    */
/*  underscore FEATURE values are now #defined according to the      */
/*  LAST built architecture just built (usually zarch = 900). This   */
/*  means from this point onward (to the end of file) you should     *
/*  ONLY be testing the underscore _FEATURE values to see if the     */
/*  given feature was defined for *ANY* of the build architectures.  */
/*-------------------------------------------------------------------*/

/*********************************************************************/
/*                  IMPORTANT PROGRAMMING NOTE!                      */
/*********************************************************************/
/*                                                                   */
/* It is CRITICALLY IMPORTANT to not use any architecture dependent  */
/* macros anywhere in any of your non-arch_dep functions! This means */
/* you CANNOT use GREG, RADR, VADR, etc. anywhere in your function!  */
/*                                                                   */
/* Basically you MUST NOT use any architecture dependent macro that  */
/* is #defined in the "feature.h" header.  If you you need to use    */
/* any of them, then your function MUST be an "ARCH_DEP" function    */
/* that is placed within the ARCH_DEP section at the beginning of    */
/* this module where it can be compiled multiple times, once for     */
/* each of the supported architectures so the macro gets #defined    */
/* to its proper value for the architecture! YOU HAVE BEEN WARNED!   */
/*                                                                   */
/*********************************************************************/

#if defined( _FEATURE_076_MSA_EXTENSION_FACILITY_3 )

/*-------------------------------------------------------------------*/
/*               Function: renew_wrapping_keys                       */
/*                                                                   */
/* Each time a clear reset is performed a new set of wrapping keys   */
/* and their associated verification patterns are generated. The     */
/* contents of the two wrapping-key registers are kept internal to   */
/* the model so that no program, including the operating system,     */
/* can directly observe their clear value.                           */
/*-------------------------------------------------------------------*/

void renew_wrapping_keys()
{
    U64   cpuid;
    int   i, randval;
    BYTE  lparname[8];
#if defined( WRAPPINGKEYS_DEBUG )
    char  buf[128];
#endif

    srandom( (unsigned) time(0) );

    /* Randomize related to time */
    for (i=0; i < 256; i++)
    {
        randval = (int) random() * (int) (host_tod() & 0xFFFFFFFF);
        srandom( (unsigned) randval );
    }

    obtain_wrlock( &sysblk.wklock );
    {
        for (i=0; i < 32; i++)
            sysblk.wkaes_reg[i] = random() & 0xFF;

        for (i=0; i < 24; i++)
            sysblk.wkdea_reg[i] = random() & 0xFF;

        /*
        ** We set the verification pattern to:
        **
        **    CPUID         (8 bytes)
        **    LPAR Name     (8 bytes)
        **    LAPR Number   (1 bytes)
        **    Random number (8 bytes)  (at the end)
        */

        memset( sysblk.wkvpaes_reg, 0, 32 );
        memset( sysblk.wkvpdea_reg, 0, 24 );
        cpuid = sysblk.cpuid;

        for (i=0; i < 8; i++)
        {
            sysblk.wkvpaes_reg[ 7 - i ] = cpuid & 0xff;
            sysblk.wkvpdea_reg[ 7 - i ] = cpuid & 0xff;
            cpuid >>= 8;
        }

        get_lparname( lparname );

        memcpy( &sysblk.wkvpaes_reg[8], lparname, 8 );
        memcpy( &sysblk.wkvpdea_reg[8], lparname, 8 );

        sysblk.wkvpaes_reg[16] = sysblk.lparnum;
        sysblk.wkvpdea_reg[16] = sysblk.lparnum;

        for (i=0; i < 8; i++)
            sysblk.wkvpaes_reg[ 31 - i ] = sysblk.wkvpdea_reg[ 23 - i ] = random() & 0xff;
    }
    release_rwlock( &sysblk.wklock );

#if defined( WRAPPINGKEYS_DEBUG )

    STRLCPY( buf, "AES wrapping key:    " );

    for (i=0; i < 32; i++)
        snprintf( buf + strlen( buf ), sizeof( buf ) - strlen( buf ),
            "%02X", sysblk.wkaes_reg[i] );

    WRMSG( HHC90190, "D", buf );

    //-------------------------------------------------------------

    STRLCPY( buf, "AES wrapping key vp: " );

    for (i=0; i < 32; i++)
        snprintf( buf + strlen( buf ), sizeof( buf ) - strlen( buf ),
            "%02X", sysblk.wkvpaes_reg[i] );

    WRMSG( HHC90190, "D", buf );

    //-------------------------------------------------------------

    STRLCPY( buf, "DEA wrapping key:    " );

    for (i=0; i < 24; i++)
        snprintf( buf + strlen( buf ), sizeof( buf ) - strlen( buf ),
            "%02X", sysblk.wkdea_reg[i] );

    WRMSG( HHC90190, "D", buf );

    //-------------------------------------------------------------

    STRLCPY( buf, "DEA wrapping key vp: " );

    for (i=0; i < 24; i++)
        snprintf( buf + strlen( buf ), sizeof( buf ) - strlen( buf ),
            "%02X", sysblk.wkvpdea_reg[i] );

    WRMSG( HHC90190, "D", buf );

#endif // defined( WRAPPINGKEYS_DEBUG )
}
#endif /* defined( _FEATURE_076_MSA_EXTENSION_FACILITY_3 ) */

#endif /* !defined( _GEN_ARCH ) */
