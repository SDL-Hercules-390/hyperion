/* CRYPTO.C     (c) Copyright Jan Jaeger, 2000-2012                  */
/*              Cryptographic instructions                           */



//   ZZ FIXME:  this logic needs moved to dyncrypt where it belongs!
//   ZZ FIXME:  this logic needs moved to dyncrypt where it belongs!
//   ZZ FIXME:  this logic needs moved to dyncrypt where it belongs!


#include "hstdinc.h"
#include "hercules.h"

//efine    WRAPPINGKEYS_DEBUG       // (#define for debugging)

#define _CRYPTO_C_

#if defined( _FEATURE_076_MSA_EXTENSION_FACILITY_3 )

/*----------------------------------------------------------------------------*/
/* Function: renew_wrapping_keys                                              */
/*                                                                            */
/* Each time a clear reset is performed, a new set of wrapping keys and their */
/* associated verification patterns are generated. The contents of the two    */
/* wrapping-key registers are kept internal to the model so that no program,  */
/* including the operating system, can directly observe their clear value.    */
/*----------------------------------------------------------------------------*/
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
