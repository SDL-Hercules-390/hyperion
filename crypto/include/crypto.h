#ifndef _CRYPTO_H_
#define _CRYPTO_H_

#define CRYPTO_EXTPKG_MOD   /* (makes it easier to identify changes) */

/*-------------------------------------------------------------------*/
/* Library functions required to be defined external to the library  */
/*-------------------------------------------------------------------*/

extern u_int32_t  crypto_fetch32( const void* ptr );
extern void       crypto_store32( void* ptr, u_int32_t value );

extern u_int32_t  crypto_cswap32( u_int32_t value );
extern u_int64_t  crypto_cswap64( u_int64_t value );

extern void       crypto_secure0( void* p, size_t n );

/*-------------------------------------------------------------------*/
/* Helper macros to call the above external functions                */
/*-------------------------------------------------------------------*/

#define GETU32( pt )                crypto_fetch32( pt )
#define PUTU32( ct, st )            crypto_store32( ct, st )
#define swap32( v )                 crypto_cswap32( v )
#define swap64( v )                 crypto_cswap64( v )
#define explicit_bzero( p, n )      crypto_secure0( p, n )

#endif // _CRYPTO_H_
