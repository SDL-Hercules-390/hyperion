/* HCRYPTO.H    (C) Copyright "Fish" (David B. Trout), 2018-2019     */
/*                  Cryptograhically Secure Random Number Generator  */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#ifndef _HCRYPTO_H_
#define _HCRYPTO_H_

/*-------------------------------------------------------------------*/
/*                        CRYPTO constants                           */
/*-------------------------------------------------------------------*/

#if defined( _WIN32 )

  #define NEED_CSRNG_INIT           // (BCryptOpenAlgorithmProvider)

  #ifndef   NT_SUCCESS
    #define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
  #endif

#elif defined( BSD ) || defined( __sun__ )

  #define USE_ARC4RANDOM            // (use 'arc4random_buf()' API)

#elif defined( __linux__ )

  #define RNDGETENTCNT              0x80045200  // entropy count ioctl
  #define RNDGETENTCNT_ALT          0x40045200  // entropy count ioctl

  #if defined( SYS_getrandom )      // syscall( SYS_getrandom ) ??

    #define USE_SYS_GETRANDOM       // syscall( SYS_getrandom ) !!

    #define MAX_CSRNG_BYTES         ((32*(1024*1024))-1)  // 32MB-1

  #else // USE_DEV_URANDOM

    #define USE_DEV_URANDOM         // read from /dev/urandom
    #define NEED_CSRNG_INIT         // wait for entropy

    #define MAX_CSRNG_BYTES         SSIZE_MAX
    #define MIN_ENTROPY_BITS        128  // ALWAYS! (apparently!)

  #endif

#else // (default to previous insecure srand() + rand() APIs)

  WARNING( "Using default insecure 'rand()' API for Crypto!" )

  #define USE_RAND_API              // rand()
  #define NEED_CSRNG_INIT           // srand()

#endif

/*-------------------------------------------------------------------*/
/*                        CRYPTO functions                           */
/*-------------------------------------------------------------------*/

#define DUMMY_CYRPTO_HANDLE         0x7FD9D5C7

extern bool hopen_CSRNG( HRANDHAND* randhand );
extern bool hclose_CSRNG( HRANDHAND* randhand );
extern bool hget_random_bytes( BYTE* buf, size_t amt, HRANDHAND* randhand );

#endif // _HCRYPTO_H_
