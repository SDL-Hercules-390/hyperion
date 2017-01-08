#ifndef _CRYPTO_H_
#define _CRYPTO_H_

/*----------------------------------------------------------------------------*/
/* Required fetch, store and swap functions defined external to the library   */
/*----------------------------------------------------------------------------*/

extern u_int32_t  crypto_fetch32( const void* ptr );
extern void       crypto_store32(       void* ptr, u_int32_t value );
extern u_int32_t  crypto_cswap32(                  u_int32_t value );
extern u_int64_t  crypto_cswap64(                  u_int64_t value );

#endif // _CRYPTO_H_
