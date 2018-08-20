/* CRYPTO.H     (C) Copyright "Fish" (David B. Trout), 2018          */
/*                  Cryptograhically Secure Random Number Generator  */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#ifndef _CRYPTO_H_
#define _CRYPTO_H_

/*-------------------------------------------------------------------*/
/*                        CRYPTO constants                           */
/*-------------------------------------------------------------------*/

#if defined( _WIN32 )

  #include <bcrypt.h>               // (CNG = Crypto Next Generation)
  #pragma comment( lib, "bcrypt" )

  #define NEED_CSRNG_INIT           // (BCryptOpenAlgorithmProvider)

  #ifndef   NT_SUCCESS
    #define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
  #endif

#elif defined( BSD )

  #define USE_ARC4RANDOM            // (use 'arc4random_buf()' API)
  #undef  NEED_CSRNG_INIT           // (no init needed)

#elif defined( __linux__ )
  
  #include <linux/random.h>
 

  #if defined( SYS_getrandom )      // syscall( SYS_getrandom ) ??

    #define USE_SYS_GETRANDOM       // syscall( SYS_getrandom ) !!
    #undef  NEED_CSRNG_INIT         // (no init needed)

    #define MAX_CSRNG_BYTES         ((32*(1024*1024))-1)  // 32MB-1

  #else

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

#if !defined( _WIN32 )
  #define DUMMY_CYRPTO_HANDLE       0x7FD9D5C7
  #include <poll.h>                 // (need struct pollfd)
#endif
/*-------------------------------------------------------------------*/
/*                        CRYPTO functions                           */
/*-------------------------------------------------------------------*/

extern bool hopen_CSRNG();
extern bool hclose_CSRNG();
extern bool hget_random_bytes( BYTE* buf, size_t amt );

#endif // _CRYPTO_H_
