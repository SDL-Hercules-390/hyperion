/* DYNCRYPT.C   (c) Bernard van der Helm, 2003-2011                  */
/*              z/Architecture crypto instructions                   */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*----------------------------------------------------------------------------*/
/*                                                                            */
/* Implementation of the z/Architecture crypto instructions described in      */
/* SA22-7832-04: z/Architecture Principles of Operation within the Hercules   */
/* z/Architecture emulator.                                                   */
/*                                                                            */
/*                              (C) Copyright Bernard van der Helm, 2003-2011 */
/*                              Noordwijkerhout, The Netherlands.             */
/*----------------------------------------------------------------------------*/

#include "hstdinc.h"

#define _DYNCRYPT_C_
#define _DYNCRYPT_DLL_

#include "hercules.h"
#include "opcode.h"
#include "inline.h"

#define CRYPTO_EXTPKG_MOD       // (exposes sha2.h internal functions)

#include "crypto_version.h"
#include "rijndael.h"
#include "sha1.h"
#include "sha2.h"
#include "sshdes.h"
#include "hcrypto.h"            // CSRNG constants and functions

DISABLE_GCC_UNUSED_SET_WARNING;

#if defined( FEATURE_017_MSA_FACILITY )
/*----------------------------------------------------------------------------*/
/* Debugging options                                                          */
/*----------------------------------------------------------------------------*/
#if 0
#define OPTION_KIMD_DEBUG
#define OPTION_KLMD_DEBUG
#define OPTION_KM_DEBUG
#define OPTION_KDSA_DEBUG
#define OPTION_KMA_DEBUG
#define OPTION_KMAC_DEBUG
#define OPTION_KMC_DEBUG
#define OPTION_KMCTR_DEBUG
#define OPTION_KMF_DEBUG
#define OPTION_KMO_DEBUG
#define OPTION_PCC_DEBUG
#define OPTION_PCKMO_DEBUG
#define OPTION_PRNO_DEBUG
#endif

#ifndef KMCTR_PBLENS
#define KMCTR_PBLENS

static const int kmctr_wrap[32] =
{
//  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15
    0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  0,  0,  0,  0,

// 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  0,  0,  0
};
static const int kmctr_keylengths[32] =
{
//  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15
    0,  8, 16, 24,  0,  0,  0,  0,  0,  8, 16, 24,  0,  0,  0,  0,

// 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31
    0,  0, 16, 24, 32,  0,  0,  0,  0,  0, 16, 24, 32,  0,  0,  0
};
static const int kmctr_pblens[32] =
{
//  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15
    0,  8, 16, 24,  0,  0,  0,  0,  0, 32, 40, 48,  0,  0,  0,  0,

// 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31
    0,  0, 16, 24, 32,  0,  0,  0,  0,  0, 48, 56, 64,  0,  0,  0
};
#endif // KMCTR_PBLENS

/*----------------------------------------------------------------------------*/
/* General Purpose Register 0 macro's (GR0)                                   */
/*----------------------------------------------------------------------------*/
/* fc   : Function code                                                       */
/* m    : Modifier bit                                                        */
/* lcfb : Length of cipher feedback                                           */
/* wrap : Indication if key is wrapped                                        */
/* tfc  : Function code without wrap indication                               */
/*----------------------------------------------------------------------------*/
#define GR0_fc(regs)    ((regs)->GR_L(0) & 0x0000007F)
#define GR0_m(regs)     (((regs)->GR_L(0) & 0x00000080) ? TRUE : FALSE)
#define GR0_lcfb(regs)  ((regs)->GR_L(0) >> 24)
#define GR0_wrap(egs)   (((regs)->GR_L(0) & 0x08) ? TRUE : FALSE)
#define GR0_tfc(regs)   (GR0_fc(regs) & 0x77)

/*----------------------------------------------------------------------------*/
/* KMA specific GR0 bits                                                      */
/*----------------------------------------------------------------------------*/
/* hs   : Hash-subkey supplied                (bit 53)                        */
/* laad : Last additional authenticated data  (bit 54)                        */
/* lpc  : Last plaintext/ciphertext           (bit 55)                        */
/*----------------------------------------------------------------------------*/
#define GR0_kma_hs(regs)    (((regs)->GR_L(0) & 0x00000400) ? TRUE : FALSE)
#define GR0_kma_laad(regs)  (((regs)->GR_L(0) & 0x00000200) ? TRUE : FALSE)
#define GR0_kma_lpc(regs)   (((regs)->GR_L(0) & 0x00000100) ? TRUE : FALSE)

/*----------------------------------------------------------------------------*/
/* KLMD specific GR0 bits                                                     */
/*----------------------------------------------------------------------------*/
/* ps   : Padding state for SHAKE (bit 55), one when the padding has already  */
/*        been processed and only further output is to be squeezed           */
/*----------------------------------------------------------------------------*/
#define GR0_klmd_ps(regs)   (((regs)->GR_L(0) & 0x00000100) ? TRUE : FALSE)
#define SET_GR0_klmd_ps(regs) ((regs)->GR_L(0) |= 0x00000100)

/*----------------------------------------------------------------------------*/
/* Write bytes on one line                                                    */
/*----------------------------------------------------------------------------*/
#define LOGBYTE(s, v, x) \
{ \
  char buf[128]; \
  int i; \
  \
  buf[0] = 0; \
  for(i = 0; i < (x); i++) \
    snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%02X", (v)[i]); \
  snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), " | "); \
  for(i = 0; i < (x); i++) \
  { \
    if(isprint((unsigned char)guest_to_host((v)[i]))) \
      snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%c", guest_to_host((v)[i])); \
    else \
      snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "."); \
  } \
  snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), " |"); \
  buf[sizeof(buf)-1] = '\0'; \
  WRMSG(HHC90109, "D", s, buf); \
}

/*----------------------------------------------------------------------------*/
/* Write bytes on multiple lines                                              */
/*----------------------------------------------------------------------------*/
#define LOGBYTE2(s, v, x, y) \
{ \
  char buf[128]; \
  int i; \
  int j; \
  \
  buf[0] = 0; \
  WRMSG(HHC90109, "D", s, ""); \
  for(i = 0; i < (y); i++) \
  { \
    for(j = 0; j < (x); j++) \
      snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%02X", (v)[i * (x) + j]); \
    snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), " | "); \
    for(j = 0; j < (x); j++) \
    { \
      if(isprint((unsigned char)guest_to_host((v)[i * (x) + j]))) \
        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%c", guest_to_host((v)[i * (x) + j])); \
      else \
        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "."); \
    } \
    snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), " |"); \
    buf[sizeof(buf)-1] = '\0'; \
    WRMSG(HHC90110, "D", buf); \
    buf[0] = 0; \
  } \
}

/*----------------------------------------------------------------------------*/
/* CPU determined amount of data (processed in one go)                        */
/*----------------------------------------------------------------------------*/
#define PROCESS_MAX        16384

/*----------------------------------------------------------------------------*/
/* Used for printing debugging info                                           */
/*----------------------------------------------------------------------------*/
#define TRUEFALSE(boolean)  ((boolean) ? "True" : "False")

#ifndef __STATIC_FUNCTIONS__
#define __STATIC_FUNCTIONS__

/*----------------------------------------------------------------------------*/
/* Library functions required to be defined external to the crypto library    */
/*----------------------------------------------------------------------------*/
U32 crypto_fetch32( const void* ptr )
{
    return fetch_fw( ptr );
}
void crypto_store32( void* ptr, U32 value )
{
    store_fw( ptr, value );
}
U32 crypto_cswap32( U32 value )
{
    return CSWAP32( value );
}
U64 crypto_cswap64( U64 value )
{
    return CSWAP64( value );
}
void crypto_secure0( void* p, size_t n )
{
#if defined( _MSVC_ )
    SecureZeroMemory( p, n );
#else // (portable C solution for non-buggy compilers)
    volatile unsigned char* v = (volatile unsigned char*) p;
    while( n-- ) *v++ = 0;
#endif
}

/*----------------------------------------------------------------------------*/
/* GCM multiplication over GF(2^128)                                          */
/*----------------------------------------------------------------------------*/
/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@..., http://libtomcrypt.org
*/

/* Remarks Bernard van der Helm & JW: Strongly adjusted for
 * Hercules-390. We need the internal functions gcm_gf_mult
 * and xts_mult_x. The rest of the code is deleted.
 *
 * Thanks Tom!
*/

/* Hercules adjustments */
#define zeromem(dst, len)    memset((dst), 0, (len))
#define XMEMCPY              memcpy

/* Original code from gcm_gf_mult.c */
/* right shift */
static void gcm_rightshift(unsigned char *a)
{
  int x;

  for(x = 15; x > 0; x--)
    a[x] = (a[x] >> 1) | ((a[x-1] << 7) & 0x80);
  a[0] >>= 1;
}

/* c = b*a */
static const unsigned char mask[] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };
static const unsigned char poly[] = { 0x00, 0xE1 };

void gcm_gf_mult(const unsigned char *a, const unsigned char *b, unsigned char *c)
{
  unsigned char Z[16], V[16];
  unsigned char x, y, z;

  zeromem(Z, 16);
  XMEMCPY(V, a, 16);
  for (x = 0; x < 128; x++)
  {
    if(b[x>>3] & mask[x&7])
    {
      for(y = 0; y < 16; y++)
        Z[y] ^= V[y];
    }
    z = V[15] & 0x01;
    gcm_rightshift(V);
    V[0] ^= poly[z];
  }
  XMEMCPY(c, Z, 16);
}

/* I = 2*I */
void xts_mult_x(unsigned char *I)
{
  int x;
  unsigned char t, tt;

  for (x = t = 0; x < 16; x++) {
     tt   = I[x] >> 7;
     I[x] = ((I[x] << 1) | t) & 0xFF;
     t    = tt;
  }
  if (tt) {
     I[0] ^= 0x87;
  }
}

/* c = b*a -- bit swapped call of gcm_gf_mult for use with xts */
void xts_gf_mult(const unsigned char *a, const unsigned char *b, unsigned char *c)
{
  unsigned char a_r[16], b_r[16], c_r[16];
  int i;

  for (i=0; i<=15; i++)
  {
    a_r[i] = reverse_bits( a[i] );
    b_r[i] = reverse_bits( b[i] );
  }

  gcm_gf_mult( a_r, b_r, c_r );

  for (i=0; i <= 15; i++)
      c[i] = reverse_bits( c_r[i] );
}

/*----------------------------------------------------------------------------*/
/* Message Security Assist Extension query                                    */
/*----------------------------------------------------------------------------*/
static int get_msa(REGS *regs)
{
  if (FACILITY_ENABLED( 057_MSA_EXTENSION_5,  regs ))
    return(4);
  if (FACILITY_ENABLED( 077_MSA_EXTENSION_4,  regs ))
    return(4);
  if (FACILITY_ENABLED( 076_MSA_EXTENSION_3,  regs ))
    return(3);
  if (FACILITY_ENABLED( HERC_MSA_EXTENSION_2, regs ))
    return(2);
  if (FACILITY_ENABLED( HERC_MSA_EXTENSION_1, regs ))
    return(1);
  if (FACILITY_ENABLED( 017_MSA,              regs ))
    return(0);
  return(-1);
}

/*----------------------------------------------------------------------------*/
/* Get the chaining vector for output processing                              */
/*----------------------------------------------------------------------------*/
static void sha1_getcv(SHA1_CTX *ctx, BYTE icv[20])
{
  int i, j;

  for(i = 0, j = 0; i < 5; i++)
  {
    icv[j++] = (ctx->state[i] & 0xff000000) >> 24;
    icv[j++] = (ctx->state[i] & 0x00ff0000) >> 16;
    icv[j++] = (ctx->state[i] & 0x0000ff00) >>  8;
    icv[j++] = (ctx->state[i] & 0x000000ff) >>  0;
  }
}

/*----------------------------------------------------------------------------*/
/* Set the initial chaining value                                             */
/*----------------------------------------------------------------------------*/
static void sha1_seticv(SHA1_CTX *ctx, BYTE icv[20])
{
  int i, j;

  for(i = 0, j = 0; i < 5; i++)
  {
    ctx->state[i]  = (((U32)icv[j++]) << 24);
    ctx->state[i] |= (((U32)icv[j++]) << 16);
    ctx->state[i] |= (((U32)icv[j++]) <<  8);
    ctx->state[i] |= (((U32)icv[j++]) <<  0);
  }
}

/*----------------------------------------------------------------------------*/
/* Get the chaining vector for output processing                              */
/*----------------------------------------------------------------------------*/
static void sha256_getcv(SHA2_CTX *ctx, BYTE icv[32])
{
  int i, j;

  for(i = 0, j = 0; i < 8; i++)
  {
    icv[j++] = (ctx->state.st32[i] & 0xff000000) >> 24;
    icv[j++] = (ctx->state.st32[i] & 0x00ff0000) >> 16;
    icv[j++] = (ctx->state.st32[i] & 0x0000ff00) >>  8;
    icv[j++] = (ctx->state.st32[i] & 0x000000ff) >>  0;
  }
}

/*----------------------------------------------------------------------------*/
/* Set the initial chaining value                                             */
/*----------------------------------------------------------------------------*/
static void sha256_seticv(SHA2_CTX *ctx, BYTE icv[32])
{
  int i, j;

  for(i = 0, j = 0; i < 8; i++)
  {
    ctx->state.st32[i]  = (((U32)icv[j++]) << 24);
    ctx->state.st32[i] |= (((U32)icv[j++]) << 16);
    ctx->state.st32[i] |= (((U32)icv[j++]) <<  8);
    ctx->state.st32[i] |= (((U32)icv[j++]) <<  0);
  }
}

/*----------------------------------------------------------------------------*/
/* Get the chaining vector for output processing                              */
/*----------------------------------------------------------------------------*/
static void sha512_getcv(SHA2_CTX *ctx, BYTE icv[64])
{
  int i, j;

  for(i = 0, j = 0; i < 8; i++)
  {
    icv[j++] = (ctx->state.st64[i] & 0xff00000000000000LL) >> 56;
    icv[j++] = (ctx->state.st64[i] & 0x00ff000000000000LL) >> 48;
    icv[j++] = (ctx->state.st64[i] & 0x0000ff0000000000LL) >> 40;
    icv[j++] = (ctx->state.st64[i] & 0x000000ff00000000LL) >> 32;
    icv[j++] = (ctx->state.st64[i] & 0x00000000ff000000LL) >> 24;
    icv[j++] = (ctx->state.st64[i] & 0x0000000000ff0000LL) >> 16;
    icv[j++] = (ctx->state.st64[i] & 0x000000000000ff00LL) >>  8;
    icv[j++] = (ctx->state.st64[i] & 0x00000000000000ffLL) >>  0;
  }
}

/*----------------------------------------------------------------------------*/
/* Set the initial chaining value                                             */
/*----------------------------------------------------------------------------*/
static void sha512_seticv(SHA2_CTX *ctx, BYTE icv[64])
{
  int i, j;

  for(i = 0, j = 0; i < 8; i++)
  {
    ctx->state.st64[i]  = (((U64)icv[j++]) << 56);
    ctx->state.st64[i] |= (((U64)icv[j++]) << 48);
    ctx->state.st64[i] |= (((U64)icv[j++]) << 40);
    ctx->state.st64[i] |= (((U64)icv[j++]) << 32);
    ctx->state.st64[i] |= (((U64)icv[j++]) << 24);
    ctx->state.st64[i] |= (((U64)icv[j++]) << 16);
    ctx->state.st64[i] |= (((U64)icv[j++]) <<  8);
    ctx->state.st64[i] |= (((U64)icv[j++]) <<  0);
  }
}

/*----------------------------------------------------------------------------*/
/* Shif left                                                                  */
/*----------------------------------------------------------------------------*/
void shift_left(BYTE *dst, BYTE* src, int len)
{
  int carry;
  int i;

  carry = 0;
  for(i = 0; i < len; i++)
  {
    if(carry)
    {
      carry = src[len - 1 - i] & 0x80;
      dst[len - 1 - i] = src[len - 1 - i] << 1;
      dst[len - 1 - i] |= 0x01;
    }
    else
    {
      carry = src[len - 1 - i] & 0x80;
      dst[len - 1 - i] = src[len - 1 - i] << 1;
    }
  }
}

/*----------------------------------------------------------------------------*/
/* Internal DES helper functions                                              */
/*----------------------------------------------------------------------------*/
typedef BYTE CHAR8[8];

typedef struct {
    DESContext sched[1];
} des_context;

typedef struct {
    DESContext sched[3];
} des3_context;

void des_set_key(des_context *ctx, CHAR8 key)
{
    DESContext *sched = ctx->sched;
    word32 kL, kR;
    kL = fetch_fw(key);
    kR = fetch_fw(key+4);
    des_key_setup(kL, kR, &sched[0]);
}

void des_encrypt(des_context *ctx, CHAR8 input, CHAR8 output)
{
    DESContext *sched = ctx->sched;
    word32 out[2], xL, xR;
    xL = fetch_fw(input);
    xR = fetch_fw(input+4);
    des_encipher(out, xL, xR, sched);
    store_fw(output, out[0]);
    store_fw(output+4, out[1]);
}

void des_decrypt(des_context *ctx, CHAR8 input, CHAR8 output)
{
    DESContext *sched = ctx->sched;
    word32 out[2], xL, xR;
    xL = fetch_fw(input);
    xR = fetch_fw(input+4);
    des_decipher(out, xL, xR, sched);
    store_fw(output, out[0]);
    store_fw(output+4, out[1]);
}

void des3_set_2keys(des3_context *ctx, CHAR8 k1, CHAR8 k2)
{
    DESContext *sched = ctx->sched;
    word32 kL, kR;
    kL = fetch_fw(k1);
    kR = fetch_fw(k1+4);
    des_key_setup(kL, kR, &sched[0]);
    des_key_setup(kL, kR, &sched[2]);
    kL = fetch_fw(k2);
    kR = fetch_fw(k2+4);
    des_key_setup(kL, kR, &sched[1]);
}

void des3_set_3keys(des3_context *ctx, CHAR8 k1, CHAR8 k2, CHAR8 k3)
{
    DESContext *sched = ctx->sched;
    word32 kL, kR;
    kL = fetch_fw(k1);
    kR = fetch_fw(k1+4);
    des_key_setup(kL, kR, &sched[0]);
    kL = fetch_fw(k2);
    kR = fetch_fw(k2+4);
    des_key_setup(kL, kR, &sched[1]);
    kL = fetch_fw(k3);
    kR = fetch_fw(k3+4);
    des_key_setup(kL, kR, &sched[2]);
}

void des3_encrypt(des3_context *ctx, CHAR8 input, CHAR8 output)
{
    DESContext *sched = ctx->sched;
    word32 out[2], xL, xR;
    xL = fetch_fw(input);
    xR = fetch_fw(input+4);
    des_encipher(out, xL, xR, sched);
    xL = out[0]; xR = out[1];
    des_decipher(out, xL, xR, sched+1);
    xL = out[0]; xR = out[1];
    des_encipher(out, xL, xR, sched+2);
    store_fw(output, out[0]);
    store_fw(output+4, out[1]);
}

void des3_decrypt(des3_context *ctx, CHAR8 input, CHAR8 output)
{
    DESContext *sched = ctx->sched;
    word32 out[2], xL, xR;
    xL = fetch_fw(input);
    xR = fetch_fw(input+4);
    des_decipher(out, xL, xR, sched+2);
    xL = out[0]; xR = out[1];
    des_encipher(out, xL, xR, sched+1);
    xL = out[0]; xR = out[1];
    des_decipher(out, xL, xR, sched);
    store_fw(output, out[0]);
    store_fw(output+4, out[1]);
}

#if defined( _FEATURE_076_MSA_EXTENSION_FACILITY_3 )
/*----------------------------------------------------------------------------*/
/* Unwrap key using aes                                                       */
/*----------------------------------------------------------------------------*/
static int unwrap_aes(BYTE *key, int keylen)
{
  BYTE buf[16];
  rijndael_ctx context;
  BYTE cv[16];
  int i;

  /* Verify verification pattern */
  if(unlikely(memcmp(&key[keylen], sysblk.wkvpaes_reg, 32)))
    return(1);

  rijndael_set_key(&context, sysblk.wkaes_reg, 256);

  switch(keylen)
  {
    case 16:
    {
      rijndael_decrypt(&context, key, key);
      break;
    }
    case 24:
    {
      rijndael_decrypt(&context, &key[8], buf);
      memcpy(&key[8], &buf[8], 8);
      memcpy(cv, key, 8);
      rijndael_decrypt(&context, key, key);
      for(i = 0; i < 8; i++)
        key[i + 16] = buf[i] ^ cv[i];
      break;
    }
    case 32:
    {
      memcpy(cv, key, 16);
      rijndael_decrypt(&context, key, key);
      rijndael_decrypt(&context, &key[16], &key[16]);
      for(i = 0; i < 16; i++)
        key[i + 16] ^= cv[i];
      break;
    }
  }
  return(0);
}

/*----------------------------------------------------------------------------*/
/* Unwrap key using dea                                                       */
/*----------------------------------------------------------------------------*/
static int unwrap_dea(BYTE *key, int keylen)
{
  BYTE cv[16];
  des3_context context;
  int i;
  int j;

  /* Verify verification pattern */
  if(unlikely(memcmp(&key[keylen], sysblk.wkvpdea_reg, 24)))
    return(1);

  des3_set_3keys(&context, sysblk.wkdea_reg, &sysblk.wkdea_reg[8], &sysblk.wkdea_reg[16]);

  for(i = 0; i < keylen; i += 8)
  {
    /* Save cv */
    memcpy(cv, &cv[8], 8);
    memcpy(&cv[8], &key[i], 8);

    des3_decrypt(&context, &key[i], &key[i]);
    des3_encrypt(&context, &key[i], &key[i]);
    des3_decrypt(&context, &key[i], &key[i]);
    if(i)
    {
      /* XOR */
      for(j = 0; j < 8; j++)
        key[i + j] ^= cv[j];
    }
  }
  return(0);
}

/*----------------------------------------------------------------------------*/
/* Wrap key using aes                                                         */
/*----------------------------------------------------------------------------*/
static void wrap_aes(BYTE *key, int keylen)
{
  BYTE buf[16];
  rijndael_ctx context;
  BYTE cv[16];
  int i;

  memcpy(&key[keylen], sysblk.wkvpaes_reg, 32);

  rijndael_set_key(&context, sysblk.wkaes_reg, 256);

  switch(keylen)
  {
    case 16:
    {
      rijndael_encrypt(&context, key, key);
      break;
    }
    case 24:
    {
      rijndael_encrypt(&context, key, cv);
      memcpy(buf, &key[16], 8);
      zeromem(&buf[8], 8);
      for(i = 0; i < 16; i++)
        buf[i] ^= cv[i];
      rijndael_encrypt(&context, buf, buf);
      memcpy(key, cv, 8);
      memcpy(&key[8], buf, 16);
      break;
    }
    case 32:
    {
      rijndael_encrypt(&context, key, key);
      for(i = 0; i < 16; i++)
        key[i + 16] ^= key[i];
      rijndael_encrypt(&context, &key[16], &key[16]);
      break;
    }
  }
}

/*----------------------------------------------------------------------------*/
/* Wrap key using dea                                                         */
/*----------------------------------------------------------------------------*/
static void wrap_dea(BYTE *key, int keylen)
{
  des3_context context;
  int i;
  int j;

  memcpy(&key[keylen], sysblk.wkvpdea_reg, 24);

  des3_set_3keys(&context, sysblk.wkdea_reg, &sysblk.wkdea_reg[8], &sysblk.wkdea_reg[16]);

  for(i = 0; i < keylen; i += 8)
  {
    if(i)
    {
      /* XOR */
      for(j = 0; j < 8; j++)
        key[i + j] ^= key[i + j - 8];
    }
    des3_encrypt(&context, &key[i], &key[i]);
    des3_decrypt(&context, &key[i], &key[i]);
    des3_encrypt(&context, &key[i], &key[i]);
  }
}
#endif // defined( _FEATURE_076_MSA_EXTENSION_FACILITY_3 )

#if defined( _FEATURE_155_MSA_EXTENSION_FACILITY_9 )
/*----------------------------------------------------------------------------*/
/* Wrap and unwrap ECC private keys (PCKMO FC 32-34, 40, 41 and the KDSA      */
/* encrypted-key functions) with the AES wrapping key.                        */
/*----------------------------------------------------------------------------*/
/* The wrapping method of real hardware is not documented, and the wrapping   */
/* key is random per Hercules instance anyway. Hercules uses AES-256 in CBC   */
/* mode with a zero IV over the whole key field (32, 48, 64 or 80 bytes, all  */
/* multiples of the block size); for a 32-byte field this is the same method  */
/* as wrap_aes() uses for AES-256 keys. The key field is followed by the      */
/* 32-byte AES wrapping key verification pattern.                             */
/*----------------------------------------------------------------------------*/
static void wrap_ecc(BYTE *key, int keylen)
{
  rijndael_ctx context;
  int i;
  int j;

  rijndael_set_key(&context, sysblk.wkaes_reg, 256);
  for(i = 0; i < keylen; i += 16)
  {
    if(i)
    {
      for(j = 0; j < 16; j++)
        key[i + j] ^= key[i + j - 16];
    }
    rijndael_encrypt(&context, &key[i], &key[i]);
  }
  memcpy(&key[keylen], sysblk.wkvpaes_reg, 32);
  crypto_secure0(&context, sizeof(context));
}

/* Returns 1 if the verification pattern does not match (cc 1) */
static int unwrap_ecc(BYTE *key, int keylen)
{
  rijndael_ctx context;
  BYTE cv[16];
  BYTE next[16];
  int i;
  int j;

  if(unlikely(memcmp(&key[keylen], sysblk.wkvpaes_reg, 32)))
    return(1);

  rijndael_set_key(&context, sysblk.wkaes_reg, 256);
  memset(cv, 0, 16);
  for(i = 0; i < keylen; i += 16)
  {
    memcpy(next, &key[i], 16);
    rijndael_decrypt(&context, &key[i], &key[i]);
    for(j = 0; j < 16; j++)
      key[i + j] ^= cv[j];
    memcpy(cv, next, 16);
  }
  crypto_secure0(&context, sizeof(context));
  return(0);
}
#endif // defined( _FEATURE_155_MSA_EXTENSION_FACILITY_9 )

/*----------------------------------------------------------------------------*/
/* 2^m table with GF multiplication entries 127, 126, 125, ...                */
/* 2                  2^b1                                                    */
/* 2*2                2^b10                                                   */
/* 2*2*2*2            2^b100                                                  */
/* 2*2*2*2*2*2*2*2    2^b1000                                                 */
/* ...                                                                        */
/*----------------------------------------------------------------------------*/
static BYTE exp_table[128][16] =
{
  { 0xa4, 0x6d, 0xdb, 0xb6, 0x6d, 0xdb, 0xb6, 0x6d, 0x92, 0x24, 0x49, 0x92, 0x24, 0x49, 0x92, 0x24 },
  { 0x5a, 0x45, 0x51, 0x14, 0x8a, 0xa2, 0x28, 0x8a, 0x61, 0x18, 0x86, 0x61, 0x9a, 0xa6, 0x69, 0x9a },
  { 0xac, 0xb6, 0xf3, 0x3c, 0x45, 0x51, 0xd7, 0x75, 0xdb, 0xb6, 0xae, 0xeb, 0xfb, 0xbe, 0xae, 0xeb },
  { 0x92, 0xcf, 0xb6, 0xe7, 0x28, 0x00, 0x49, 0xdb, 0x75, 0x9e, 0x24, 0x08, 0xc3, 0x71, 0x1c, 0xc7 },
  { 0x98, 0x29, 0xb7, 0x52, 0x3e, 0xd5, 0xac, 0xeb, 0xea, 0x8a, 0x92, 0x86, 0x30, 0x4d, 0xd3, 0x34 },
  { 0xa5, 0x75, 0x1a, 0xb6, 0x81, 0x7a, 0x63, 0x18, 0x61, 0x6e, 0xa4, 0xe3, 0x78, 0xca, 0xb2, 0x2c },
  { 0x06, 0x4f, 0x09, 0x28, 0x0e, 0xbb, 0x06, 0x86, 0x41, 0x19, 0xb8, 0x20, 0x9d, 0x18, 0x86, 0x61 },
  { 0xb9, 0x17, 0xaf, 0x7f, 0xe2, 0xab, 0x24, 0x8a, 0xf9, 0x0d, 0xab, 0x34, 0x69, 0x65, 0x21, 0x08 },
  { 0xc5, 0x8c, 0x3c, 0x3e, 0x5f, 0xa3, 0x15, 0x60, 0xb5, 0x2a, 0x1c, 0x14, 0xde, 0x41, 0x64, 0xdf },
  { 0x6c, 0x77, 0x82, 0x2a, 0xce, 0x66, 0x2b, 0xab, 0x33, 0x0f, 0x8b, 0x64, 0x47, 0x00, 0x93, 0x30 },
  { 0xac, 0x13, 0x6f, 0x36, 0x52, 0x0f, 0xbd, 0xeb, 0x0b, 0xf1, 0xbc, 0xe2, 0x22, 0x9a, 0x6d, 0x9a },
  { 0x9d, 0x77, 0x37, 0x05, 0x91, 0xaf, 0xa7, 0x77, 0x6e, 0xf4, 0x06, 0x8e, 0x3f, 0xe7, 0xa2, 0xeb },
  { 0x68, 0xed, 0xcb, 0x5a, 0xc6, 0xa9, 0x2b, 0x02, 0x67, 0xb6, 0x55, 0x67, 0xe4, 0x86, 0x8b, 0x71 },
  { 0xc1, 0xac, 0xe2, 0x06, 0x2c, 0x14, 0xc3, 0x12, 0xe0, 0xdc, 0x57, 0xfd, 0xc2, 0x66, 0xa7, 0xbe },
  { 0x52, 0xf7, 0x0e, 0x21, 0x67, 0x69, 0xb6, 0x9c, 0x1e, 0xb5, 0xdb, 0x27, 0x85, 0x8d, 0xf5, 0xaa },
  { 0xd5, 0x64, 0x57, 0x5f, 0x5b, 0xf8, 0xbd, 0x46, 0xc2, 0x2d, 0x44, 0x17, 0x1b, 0x43, 0x7a, 0xc7 },
  { 0xca, 0xf4, 0x80, 0xce, 0x40, 0x11, 0x93, 0x8d, 0x08, 0xf9, 0xc4, 0xd8, 0xd7, 0x26, 0x02, 0xef },
  { 0x62, 0x45, 0xee, 0xbb, 0x2b, 0x44, 0x31, 0xfb, 0x88, 0x5c, 0x92, 0x85, 0x56, 0x7a, 0x0a, 0x39 },
  { 0x0f, 0x7e, 0xa0, 0x29, 0x58, 0xda, 0xb3, 0x46, 0x6d, 0x1c, 0xff, 0xab, 0x97, 0xa5, 0xd4, 0x0d },
  { 0x1d, 0x69, 0xf1, 0x7a, 0x67, 0x56, 0xaf, 0xeb, 0x9d, 0x28, 0xeb, 0x4a, 0x04, 0xc7, 0x77, 0xb3 },
  { 0x00, 0x04, 0x5a, 0xb7, 0xc9, 0x36, 0xef, 0xc3, 0x9f, 0xb8, 0x90, 0xe3, 0x45, 0x95, 0x34, 0x74 },
  { 0xa3, 0x57, 0x77, 0x72, 0x64, 0xbc, 0xfb, 0x27, 0xb6, 0xe6, 0xf5, 0x58, 0xe1, 0xe7, 0x7f, 0xbf },
  { 0x53, 0xbe, 0xfd, 0xf9, 0x8f, 0x1b, 0x4b, 0x58, 0xb5, 0xd0, 0x72, 0xbc, 0x18, 0x4c, 0xae, 0xe2 },
  { 0x26, 0x26, 0x80, 0x72, 0x9e, 0x20, 0xa9, 0xab, 0x39, 0x57, 0xb1, 0x96, 0xc8, 0x3b, 0xfb, 0x18 },
  { 0x98, 0xde, 0x73, 0x58, 0x44, 0x8c, 0x90, 0x42, 0xbd, 0xf5, 0x2c, 0xc7, 0x90, 0x58, 0xb8, 0xfa },
  { 0xd8, 0xc1, 0xca, 0x5a, 0x03, 0xee, 0xa7, 0x8d, 0x31, 0x04, 0xa0, 0xb5, 0x53, 0x34, 0x21, 0xc7 },
  { 0xf1, 0xad, 0x31, 0xd1, 0xff, 0x32, 0xad, 0x5b, 0xec, 0xdc, 0x8f, 0xa1, 0x49, 0x3e, 0x40, 0xa6 },
  { 0x55, 0x39, 0x50, 0x01, 0xbe, 0x3d, 0xcf, 0xb5, 0xe1, 0x5b, 0xf5, 0xb1, 0x33, 0x1a, 0x62, 0x8b },
  { 0x7c, 0xc7, 0xb9, 0x38, 0x66, 0xc6, 0x4f, 0xff, 0xd6, 0xb6, 0xba, 0x3e, 0x95, 0x88, 0xf9, 0xa2 },
  { 0x54, 0x51, 0x2d, 0x6b, 0xf0, 0xc0, 0x52, 0x24, 0xa6, 0xf3, 0x82, 0x8f, 0x75, 0x9c, 0xcc, 0x18 },
  { 0xa1, 0x82, 0xb8, 0x37, 0x4d, 0x0e, 0xe3, 0x29, 0xdb, 0xf0, 0x13, 0x09, 0x75, 0xea, 0x7b, 0x21 },
  { 0x7b, 0xdd, 0x20, 0xd6, 0x8e, 0x75, 0xa8, 0xdc, 0xca, 0x98, 0x06, 0x45, 0xc1, 0xdf, 0xe2, 0x40 },
  { 0x9d, 0x67, 0xea, 0x02, 0xab, 0xdf, 0xaa, 0x3c, 0x32, 0x7d, 0x08, 0x5e, 0xa1, 0x24, 0x31, 0x6c },
  { 0x55, 0x0c, 0x78, 0x74, 0xe3, 0x86, 0x8e, 0x04, 0x67, 0xfc, 0x78, 0x0c, 0x0d, 0x22, 0x65, 0x9f },
  { 0xb3, 0x40, 0x1b, 0x97, 0x11, 0x20, 0xc5, 0xf1, 0x4d, 0x64, 0xee, 0x6c, 0x56, 0x04, 0x42, 0x86 },
  { 0x97, 0xf3, 0xa8, 0x2e, 0x6b, 0x65, 0x43, 0x18, 0x25, 0x82, 0x64, 0x53, 0x40, 0x45, 0xdb, 0xef },
  { 0x5b, 0x5f, 0xa3, 0xf2, 0x32, 0x06, 0x0e, 0xf5, 0x8a, 0x2a, 0xf6, 0x96, 0x10, 0xb4, 0x6d, 0x38 },
  { 0xc3, 0x85, 0xf8, 0x69, 0xc8, 0x87, 0x92, 0x87, 0xdd, 0xc3, 0x4b, 0x89, 0x47, 0xa7, 0xad, 0xbb },
  { 0x44, 0xe5, 0xbd, 0x6a, 0x1a, 0xf9, 0xa6, 0xac, 0x37, 0xd7, 0x7c, 0xca, 0x7c, 0xa0, 0x74, 0x55 },
  { 0x06, 0x23, 0xf4, 0x6d, 0xab, 0xdf, 0x4f, 0x49, 0xad, 0x63, 0xe0, 0x3a, 0x34, 0xcf, 0xc0, 0x92 },
  { 0x38, 0xaa, 0x78, 0xfb, 0x3a, 0x81, 0x3c, 0xcd, 0xf4, 0xf3, 0x78, 0x20, 0x67, 0x47, 0x86, 0x7c },
  { 0x31, 0x96, 0x4e, 0x57, 0x2c, 0xa0, 0xd1, 0x9a, 0x1d, 0xc6, 0xc9, 0xa0, 0x51, 0x64, 0x26, 0x28 },
  { 0x3b, 0x27, 0x2e, 0xc5, 0x4c, 0x91, 0x00, 0x78, 0xef, 0xb4, 0x1b, 0x78, 0x59, 0x88, 0x64, 0xd6 },
  { 0xba, 0x5e, 0xda, 0x0d, 0x44, 0x08, 0xed, 0x00, 0x99, 0x5b, 0x80, 0x44, 0xe5, 0x44, 0xa0, 0x59 },
  { 0xed, 0xe9, 0xdc, 0xbf, 0x06, 0xce, 0xc4, 0xdf, 0x07, 0xb3, 0x4d, 0x6c, 0xb1, 0x25, 0x25, 0x05 },
  { 0x45, 0xed, 0x44, 0xc4, 0x78, 0xbd, 0x41, 0x84, 0x73, 0x71, 0xa2, 0x15, 0x19, 0xf2, 0xd3, 0x92 },
  { 0x99, 0xf1, 0x5d, 0xa9, 0xc7, 0x1f, 0x58, 0x13, 0x3b, 0xc9, 0xe0, 0x7f, 0xf6, 0xda, 0x74, 0xca },
  { 0x7a, 0xe4, 0x1e, 0x81, 0xc2, 0x87, 0x2a, 0x04, 0x77, 0xa9, 0xdc, 0xb4, 0x32, 0xd5, 0xa3, 0x79 },
  { 0x6e, 0x96, 0x66, 0xfa, 0x3e, 0x40, 0xc3, 0x1e, 0x89, 0x7f, 0xca, 0x49, 0x1f, 0x1a, 0xed, 0xba },
  { 0xd8, 0x5b, 0xea, 0x61, 0x8f, 0x96, 0xfa, 0xb4, 0x59, 0x8f, 0xdb, 0x07, 0x2a, 0xfa, 0x94, 0xaa },
  { 0x62, 0xec, 0x3d, 0x2e, 0x00, 0x38, 0x51, 0x23, 0x6a, 0x2c, 0xca, 0xc1, 0x14, 0x03, 0x8f, 0x1c },
  { 0x3d, 0x5b, 0x3b, 0xb6, 0xe4, 0x1b, 0xcb, 0x00, 0x56, 0xd4, 0xd6, 0x66, 0x44, 0x81, 0xb4, 0xde },
  { 0x23, 0x22, 0xe5, 0xd1, 0x03, 0x6b, 0x2c, 0x02, 0xb0, 0xc2, 0x6a, 0x48, 0x5b, 0x43, 0x12, 0x5c },
  { 0xbb, 0x59, 0x37, 0x05, 0x00, 0x84, 0x40, 0xe9, 0xbd, 0x81, 0x9d, 0x02, 0x7c, 0x04, 0x4b, 0xfb },
  { 0x6e, 0x36, 0xa0, 0xd5, 0x4e, 0x02, 0xf4, 0x92, 0x4c, 0xff, 0x7f, 0x3b, 0x37, 0x03, 0xb6, 0xc6 },
  { 0x19, 0xc4, 0xd7, 0x7c, 0xdc, 0x4e, 0x10, 0xf9, 0xf4, 0x13, 0x4b, 0xee, 0x1d, 0x0f, 0xfd, 0xca },
  { 0xec, 0x97, 0xcb, 0x2f, 0xec, 0x39, 0x3c, 0xdd, 0xc9, 0xd7, 0xdc, 0x56, 0x6e, 0x70, 0xf9, 0x31 },
  { 0x35, 0x9c, 0x95, 0x7a, 0xca, 0x30, 0xbd, 0x44, 0xee, 0x9c, 0xba, 0x7b, 0x11, 0x4a, 0x2d, 0xf7 },
  { 0x6a, 0x0f, 0x8b, 0x13, 0x3e, 0xc6, 0x55, 0x6f, 0xe9, 0x47, 0x41, 0x6c, 0xcc, 0xf1, 0x4f, 0x74 },
  { 0x6a, 0x54, 0x97, 0xaf, 0x0b, 0x64, 0x42, 0x2b, 0xf9, 0xfc, 0x10, 0xf0, 0x91, 0x44, 0x88, 0xbf },
  { 0x94, 0xa4, 0x61, 0x65, 0x14, 0x32, 0xcb, 0x55, 0xff, 0x8e, 0xbf, 0xd4, 0xa8, 0xad, 0x25, 0xe3 },
  { 0xbc, 0xa3, 0x36, 0x97, 0x7d, 0x65, 0x7f, 0xd4, 0x53, 0xf6, 0x2f, 0xda, 0x31, 0x7e, 0xc5, 0xc2 },
  { 0x65, 0x2a, 0x7e, 0xd8, 0xc6, 0x92, 0x42, 0x0e, 0xc3, 0xcb, 0x40, 0xed, 0xe6, 0x30, 0x9d, 0x7c },
  { 0xf6, 0x99, 0xa2, 0x95, 0xa7, 0x98, 0xe5, 0x44, 0xb9, 0x32, 0x58, 0x6b, 0xea, 0x1f, 0x65, 0x61 },
  { 0xac, 0xf2, 0xa1, 0xa9, 0x37, 0xe1, 0xaa, 0x81, 0x85, 0x81, 0xa1, 0x01, 0x2e, 0x2b, 0x4e, 0xf6 },
  { 0x4f, 0xe8, 0xd6, 0x92, 0x26, 0xa5, 0x6e, 0xac, 0x75, 0x57, 0x3f, 0x7e, 0x13, 0x6c, 0xd4, 0x3d },
  { 0x3a, 0xcb, 0xa8, 0x66, 0xf8, 0xf6, 0xde, 0xdd, 0x56, 0x83, 0x89, 0x3b, 0xfd, 0xf0, 0xd6, 0x9e },
  { 0x44, 0x64, 0xa8, 0x70, 0x98, 0x42, 0x8d, 0xe6, 0xa2, 0x43, 0x3c, 0x7a, 0x82, 0x0e, 0x3e, 0x78 },
  { 0x96, 0xd9, 0x5a, 0x8b, 0x4f, 0xba, 0x28, 0x67, 0x2d, 0xf7, 0xb5, 0xe7, 0x4a, 0xd4, 0x07, 0x9f },
  { 0x35, 0x97, 0xdb, 0x77, 0x6f, 0x9e, 0x05, 0x4e, 0xa1, 0x86, 0x8f, 0x42, 0x74, 0xe7, 0xa4, 0x14 },
  { 0xa8, 0x55, 0x57, 0x73, 0x13, 0x86, 0x22, 0xbb, 0x82, 0x67, 0x14, 0xb0, 0x11, 0x03, 0x74, 0xb7 },
  { 0x73, 0x19, 0x78, 0xac, 0x75, 0xb0, 0x13, 0xe2, 0x93, 0xbd, 0x34, 0x54, 0x43, 0x52, 0x02, 0x74 },
  { 0xf7, 0x60, 0xb8, 0x18, 0x3b, 0x51, 0xcf, 0xfb, 0x4d, 0xc4, 0x52, 0x8b, 0xc5, 0x92, 0xfd, 0xf6 },
  { 0x5f, 0x90, 0x62, 0x93, 0x46, 0x21, 0xa3, 0xab, 0xbe, 0xb3, 0x92, 0xc7, 0xa4, 0x14, 0x58, 0x3c },
  { 0x23, 0x04, 0xda, 0x58, 0xa4, 0x31, 0xbb, 0xcc, 0x36, 0x34, 0x40, 0xc7, 0x51, 0x83, 0x48, 0x60 },
  { 0x96, 0x74, 0x4b, 0x9c, 0x11, 0x53, 0x9b, 0x93, 0x6d, 0x68, 0xf7, 0xe8, 0xd2, 0xa6, 0x7d, 0x09 },
  { 0xdb, 0xe4, 0x08, 0x66, 0xf1, 0x1b, 0xb2, 0xc8, 0xf7, 0x84, 0xb4, 0xee, 0x29, 0x8c, 0x75, 0x4d },
  { 0xa6, 0x66, 0x8f, 0xbd, 0xc0, 0x34, 0x76, 0x68, 0xe5, 0xbd, 0xb1, 0xe8, 0x5a, 0x8e, 0x32, 0x4d },
  { 0xc0, 0xe6, 0x1d, 0x42, 0x59, 0x11, 0x7c, 0xb8, 0xf5, 0x8c, 0x2c, 0x44, 0x1e, 0x8c, 0xcf, 0xfb },
  { 0xfd, 0x81, 0xeb, 0xd9, 0x16, 0x78, 0x1d, 0xfe, 0x18, 0x02, 0x92, 0x72, 0x17, 0xb6, 0x16, 0xe3 },
  { 0x1e, 0x3b, 0x0b, 0x77, 0x6f, 0x6c, 0x21, 0x66, 0xc8, 0xa1, 0x44, 0x70, 0xd9, 0x67, 0xc2, 0xaf },
  { 0xaa, 0x5a, 0x67, 0x0b, 0x5b, 0xd1, 0xa3, 0xab, 0x46, 0x99, 0xbb, 0x99, 0x3e, 0x09, 0x0a, 0x71 },
  { 0x48, 0x11, 0x30, 0x58, 0xba, 0xda, 0x1a, 0x7d, 0x6a, 0x06, 0x5b, 0xc7, 0x5f, 0x85, 0xfb, 0x64 },
  { 0xec, 0x7e, 0x20, 0x8f, 0x42, 0x3b, 0xbf, 0xd9, 0x49, 0x96, 0xa3, 0xd6, 0xd9, 0x7d, 0x80, 0xd3 },
  { 0x85, 0x0c, 0x93, 0x58, 0x82, 0x08, 0x85, 0xe3, 0x0e, 0x14, 0xb9, 0x7a, 0x59, 0xa7, 0xd7, 0xee },
  { 0x99, 0x10, 0xd5, 0x0d, 0x62, 0xd7, 0x7b, 0xe7, 0xb3, 0x4c, 0xb3, 0x75, 0x00, 0x0e, 0xc2, 0xc7 },
  { 0xa8, 0xea, 0xc9, 0x4c, 0x1c, 0x37, 0x46, 0x50, 0x6c, 0xb3, 0x10, 0xaf, 0x68, 0xbe, 0xdd, 0xa7 },
  { 0x12, 0xca, 0x6b, 0x11, 0xc6, 0xc9, 0x94, 0x17, 0x8e, 0xde, 0xf4, 0xb6, 0x8b, 0x54, 0x54, 0x18 },
  { 0x0d, 0xee, 0x14, 0x73, 0x07, 0x9c, 0xe2, 0x2e, 0xa2, 0x92, 0x85, 0xfc, 0x5c, 0xae, 0xfe, 0xdf },
  { 0xf8, 0xa6, 0x83, 0xe1, 0xa9, 0xad, 0xd3, 0xc1, 0xe2, 0x2f, 0xa0, 0xb8, 0x58, 0x3a, 0xab, 0xea },
  { 0xde, 0x64, 0xa5, 0x73, 0xf1, 0x62, 0xea, 0xa5, 0xae, 0xac, 0x73, 0x5a, 0x47, 0x1e, 0x62, 0x1c },
  { 0xa4, 0x17, 0xff, 0x7a, 0x00, 0xaa, 0xbb, 0x09, 0x08, 0xbc, 0xd1, 0xc1, 0xe1, 0x22, 0xb0, 0x20 },
  { 0x74, 0xdd, 0x90, 0xc1, 0x40, 0xc5, 0x16, 0x70, 0x27, 0x0e, 0x70, 0x4c, 0xe0, 0x37, 0xf5, 0xd3 },
  { 0x70, 0x7a, 0x8e, 0xdd, 0x44, 0x7b, 0x0a, 0x80, 0x5f, 0x5a, 0x12, 0x09, 0xdd, 0xb2, 0xd1, 0xca },
  { 0xb2, 0xa7, 0x11, 0x5b, 0xa9, 0x74, 0x0a, 0xde, 0x0f, 0x33, 0xb9, 0x36, 0x22, 0x20, 0xc3, 0xce },
  { 0x2a, 0x05, 0x95, 0x06, 0xf8, 0x7a, 0x77, 0x21, 0x18, 0xb0, 0xd3, 0x5d, 0x99, 0x0a, 0x2d, 0x78 },
  { 0xb0, 0x22, 0xde, 0x68, 0x69, 0x9e, 0x60, 0x62, 0x92, 0xfc, 0xf1, 0xd2, 0xe4, 0xde, 0xdc, 0x60 },
  { 0xc2, 0xf6, 0x38, 0x4f, 0x71, 0x24, 0xb8, 0xfb, 0x67, 0x28, 0xcf, 0x42, 0x73, 0xa7, 0x31, 0xd3 },
  { 0x19, 0xd3, 0x5a, 0x9d, 0xc7, 0x38, 0x17, 0x89, 0x8a, 0x22, 0x2d, 0xc7, 0xbb, 0x6e, 0xe6, 0xcb },
  { 0x53, 0x9c, 0x5e, 0x2c, 0xc5, 0xef, 0xb7, 0xa5, 0x82, 0x5f, 0xf3, 0x16, 0x43, 0x34, 0x0e, 0x15 },
  { 0x29, 0x38, 0xf3, 0xc3, 0x95, 0x69, 0x72, 0x1f, 0x32, 0xf2, 0xc7, 0x53, 0x23, 0xfc, 0xf6, 0x24 },
  { 0x44, 0xc2, 0x34, 0x76, 0x83, 0xb3, 0xad, 0x5a, 0x0d, 0x35, 0x61, 0x06, 0x3d, 0xd8, 0x3b, 0x2c },
  { 0x39, 0x4c, 0xcf, 0x7c, 0x68, 0xf5, 0x4b, 0xe9, 0xa6, 0x99, 0x75, 0x95, 0xf6, 0x4b, 0x84, 0xbb },
  { 0x1d, 0x48, 0x0f, 0x62, 0x6b, 0x93, 0x97, 0x1a, 0x4d, 0x61, 0xad, 0x56, 0x17, 0xa4, 0xf8, 0xc7 },
  { 0xad, 0x45, 0xde, 0xf9, 0x80, 0x51, 0xd1, 0x38, 0xdf, 0xbd, 0x82, 0x95, 0xf6, 0x91, 0xad, 0x83 },
  { 0x4d, 0x26, 0xbe, 0xee, 0x87, 0x45, 0xb1, 0x97, 0x93, 0x57, 0xda, 0x9e, 0x57, 0x13, 0xa5, 0x83 },
  { 0xed, 0x59, 0x06, 0x9c, 0xf0, 0x89, 0x82, 0x21, 0x42, 0x55, 0x93, 0x58, 0x03, 0xa3, 0xb4, 0x11 },
  { 0xb9, 0x82, 0xfe, 0x65, 0xf8, 0xa6, 0xe1, 0x54, 0xde, 0x5f, 0x5e, 0xd3, 0xff, 0xde, 0xaf, 0x01 },
  { 0xb4, 0x11, 0xb3, 0x30, 0x2d, 0x35, 0xd0, 0xca, 0x58, 0x65, 0x75, 0xb7, 0x4e, 0x59, 0x15, 0xb7 },
  { 0x1e, 0xea, 0x04, 0xdf, 0x11, 0x8e, 0x61, 0xea, 0x01, 0x65, 0x2d, 0x31, 0x9b, 0x50, 0x69, 0x8b },
  { 0x87, 0x79, 0x18, 0x75, 0xc7, 0x92, 0xc6, 0x93, 0xc6, 0x86, 0x14, 0x54, 0xd2, 0x40, 0x01, 0x86 },
  { 0x70, 0xef, 0x78, 0x69, 0x19, 0xe6, 0x8f, 0x11, 0x61, 0x09, 0x11, 0x11, 0x11, 0x00, 0x01, 0x10 },
  { 0x40, 0x50, 0x55, 0x50, 0x15, 0x55, 0x05, 0x41, 0x54, 0x44, 0x50, 0x01, 0x04, 0x00, 0x00, 0x00 },
  { 0xc8, 0xcf, 0xf7, 0x93, 0xae, 0x1c, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
  { 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xea, 0xca, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00 },
  { 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x87, 0x00, 0x00, 0x00 },
  { 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00 },
  { 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
  { 0x11, 0x01, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
  { 0x15, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
  { 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
  { 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
  { 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
  { 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
  { 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
  { 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
  { 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
};

/*----------------------------------------------------------------------------*/
/* Keccak-f[1600] permutation (FIPS 202) used by SHA-3 and SHAKE (MSA 6)      */
/*----------------------------------------------------------------------------*/
/* The 200-byte state is kept in the format of the KIMD/KLMD parameter block, */
/* which is the byte order of FIPS 202: lane x+5y occupies the bytes          */
/* 8*(x+5y) to 8*(x+5y)+7 in little-endian order. Message bytes are XORed     */
/* into the state bytes directly and the digest is the leftmost part of the   */
/* state.                                                                     */
/*----------------------------------------------------------------------------*/
#define KECCAK_ROTL64(x, n)  (((x) << (n)) | ((x) >> (64 - (n))))

static void keccak_f1600(BYTE state[200])
{
  static const U64 round_constants[24] =
  {
    0x0000000000000001ULL, 0x0000000000008082ULL, 0x800000000000808AULL, 0x8000000080008000ULL,
    0x000000000000808BULL, 0x0000000080000001ULL, 0x8000000080008081ULL, 0x8000000000008009ULL,
    0x000000000000008AULL, 0x0000000000000088ULL, 0x0000000080008009ULL, 0x000000008000000AULL,
    0x000000008000808BULL, 0x800000000000008BULL, 0x8000000000008089ULL, 0x8000000000008003ULL,
    0x8000000000008002ULL, 0x8000000000000080ULL, 0x000000000000800AULL, 0x800000008000000AULL,
    0x8000000080008081ULL, 0x8000000000008080ULL, 0x0000000080000001ULL, 0x8000000080008008ULL
  };
  /* Rotation offsets and lane order of the combined rho and pi steps */
  static const int rho[24] =
  {
     1,  3,  6, 10, 15, 21, 28, 36, 45, 55,  2, 14, 27, 41, 56,  8, 25, 43, 62, 18, 39, 61, 20, 44
  };
  static const int pi[24] =
  {
    10,  7, 11, 17, 18,  3,  5, 16,  8, 21, 24,  4, 15, 23, 19, 13, 12,  2, 20, 14, 22,  9,  6,  1
  };
  U64 a[25];
  U64 c[5];
  U64 t;
  int round;
  int i;
  int j;

  /* Load the lanes (little-endian) */
  for(i = 0; i < 25; i++)
  {
    a[i] = 0;
    for(j = 7; j >= 0; j--)
      a[i] = (a[i] << 8) | state[i * 8 + j];
  }

  for(round = 0; round < 24; round++)
  {
    /* Theta */
    for(i = 0; i < 5; i++)
      c[i] = a[i] ^ a[i + 5] ^ a[i + 10] ^ a[i + 15] ^ a[i + 20];
    for(i = 0; i < 5; i++)
    {
      t = c[(i + 4) % 5] ^ KECCAK_ROTL64(c[(i + 1) % 5], 1);
      for(j = 0; j < 25; j += 5)
        a[j + i] ^= t;
    }

    /* Rho and pi */
    t = a[1];
    for(i = 0; i < 24; i++)
    {
      j = pi[i];
      c[0] = a[j];
      a[j] = KECCAK_ROTL64(t, rho[i]);
      t = c[0];
    }

    /* Chi */
    for(j = 0; j < 25; j += 5)
    {
      for(i = 0; i < 5; i++)
        c[i] = a[j + i];
      for(i = 0; i < 5; i++)
        a[j + i] ^= (~c[(i + 1) % 5]) & c[(i + 2) % 5];
    }

    /* Iota */
    a[0] ^= round_constants[round];
  }

  /* Store the lanes (little-endian) */
  for(i = 0; i < 25; i++)
  {
    for(j = 0; j < 8; j++)
      state[i * 8 + j] = (BYTE)(a[i] >> (8 * j));
  }
}

/*----------------------------------------------------------------------------*/
/* SHA-3 and SHAKE block size (rate) in bytes for KIMD/KLMD FC 32-37          */
/*----------------------------------------------------------------------------*/
static int sha3_rate(int fc)
{
  static const int rates[6] = { 144, 136, 104, 72, 168, 136 };

  return(rates[fc - 32]);
}

#if defined( _FEATURE_155_MSA_EXTENSION_FACILITY_9 )
/*----------------------------------------------------------------------------*/
/* Elliptic curve support for KDSA and PCC (MSA 9), including LibTomMath      */
/*----------------------------------------------------------------------------*/
#include "dyncrypt_ecc.h"
#endif /* defined( _FEATURE_155_MSA_EXTENSION_FACILITY_9 ) */
#endif /* #ifndef __STATIC_FUNCTIONS__ */

/*----------------------------------------------------------------------------*/
/* Compute intermediate message digest (KIMD) FC 1-3                          */
/*----------------------------------------------------------------------------*/
static void ARCH_DEP(kimd_sha)(int r1, int r2, REGS *regs, int klmd)
{
  SHA1_CTX sha1_ctx;

#if defined( FEATURE_MSA_EXTENSION_FACILITY_1 )
  SHA2_CTX sha2_ctx;
#endif /* defined( FEATURE_MSA_EXTENSION_FACILITY_1 ) */

#if defined( FEATURE_MSA_EXTENSION_FACILITY_2 )
  SHA2_CTX sha512_ctx;
#endif /* defined( FEATURE_MSA_EXTENSION_FACILITY_2 ) */

  int crypted;
  int fc;
  BYTE message_block[128];
  int message_blocklen = 0;
  BYTE parameter_block[64];
  int parameter_blocklen = 0;

  UNREFERENCED(r1);

  /* Initialize values */
  fc = GR0_fc(regs);
  switch(fc)
  {
    case 1: /* sha-1 */
    {
      message_blocklen = 64;
      parameter_blocklen = 20;
      break;
    }

#if defined( FEATURE_MSA_EXTENSION_FACILITY_1 )
    case 2: /* sha-256 */
    {
      message_blocklen = 64;
      parameter_blocklen = 32;
      break;
    }
#endif /* defined( FEATURE_MSA_EXTENSION_FACILITY_1 ) */

#if defined( FEATURE_MSA_EXTENSION_FACILITY_2 )
    case 3: /* sha-512 */
    {
      message_blocklen = 128;
      parameter_blocklen = 64;
    }
#endif /* defined( FEATURE_MSA_EXTENSION_FACILITY_2 ) */

  }

  /* Check special conditions */
  if(unlikely(!klmd && (GR_A(r2 + 1, regs) % message_blocklen)))
    ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);

  /* Return with cc 0 on zero length */
  if(unlikely(!GR_A(r2 + 1, regs)))
  {
    regs->psw.cc = 0;
    return;
  }

  /* Test writeability output chaining value */
  ARCH_DEP(validate_operand)(GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, parameter_blocklen - 1, ACCTYPE_WRITE, regs);

  /* Fetch the parameter block */
  ARCH_DEP(vfetchc)(parameter_block, parameter_blocklen - 1, GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, regs);

#ifdef OPTION_KIMD_DEBUG
  if(parameter_blocklen > 32)
  {
    LOGBYTE2("icv   :", parameter_block, 16, parameter_blocklen / 16);
  }
  else
  {
    LOGBYTE("icv   :", parameter_block, parameter_blocklen);
  }
#endif /* #ifdef OPTION_KIMD_DEBUG */

  /* Set initial chaining value */
  switch(fc)
  {
    case 1: /* sha-1 */
    {
      sha1_seticv(&sha1_ctx, parameter_block);
      break;
    }

#if defined( FEATURE_MSA_EXTENSION_FACILITY_1 )
    case 2: /* sha-256 */
    {
      sha256_seticv(&sha2_ctx, parameter_block);
      break;
    }
#endif /* defined( FEATURE_MSA_EXTENSION_FACILITY_1 ) */

#if defined( FEATURE_MSA_EXTENSION_FACILITY_2 )
    case 3: /* sha-512 */
    {
      sha512_seticv(&sha512_ctx, parameter_block);
      break;
    }
#endif /* defined( FEATURE_MSA_EXTENSION_FACILITY_2 ) */

  }

  /* Try to process the CPU-determined amount of data */
  for(crypted = 0; crypted < PROCESS_MAX; crypted += message_blocklen)
  {
    /* Fetch and process a block of data */
    ARCH_DEP(vfetchc)(message_block, message_blocklen - 1, GR_A(r2, regs) & ADDRESS_MAXWRAP(regs), r2, regs);

#ifdef OPTION_KIMD_DEBUG
    LOGBYTE2("input :", message_block, 16, message_blocklen / 16);
#endif /* #ifdef OPTION_KIMD_DEBUG */

    switch(fc)
    {
      case 1: /* sha-1 */
      {
        SHA1Transform(sha1_ctx.state, message_block);
        sha1_getcv(&sha1_ctx, parameter_block);
        break;
      }

#if defined( FEATURE_MSA_EXTENSION_FACILITY_1 )
      case 2: /* sha-256 */
      {
        SHA256Transform(sha2_ctx.state.st32, message_block);
        sha256_getcv(&sha2_ctx, parameter_block);
        break;
      }
#endif /* defined( FEATURE_MSA_EXTENSION_FACILITY_1 ) */

#if defined( FEATURE_MSA_EXTENSION_FACILITY_2 )
      case 3: /* sha-512 */
      {
        SHA512Transform(sha512_ctx.state.st64, message_block);
        sha512_getcv(&sha512_ctx, parameter_block);
        break;
      }
#endif /* defined( FEATURE_MSA_EXTENSION_FACILITY_2 ) */

    }

    /* Store the output chaining value */
    ARCH_DEP(vstorec)(parameter_block, parameter_blocklen - 1, GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, regs);

#ifdef OPTION_KIMD_DEBUG
    if(parameter_blocklen > 32)
    {
      LOGBYTE2("ocv   :", parameter_block, 16, parameter_blocklen / 16);
    }
    else
    {
      LOGBYTE("ocv   :", parameter_block, parameter_blocklen);
    }
#endif /* #ifdef OPTION_KIMD_DEBUG */

    /* Update the registers */
    SET_GR_A(r2, regs, GR_A(r2, regs) + message_blocklen);
    SET_GR_A(r2 + 1, regs, GR_A(r2 + 1, regs) - message_blocklen);

#ifdef OPTION_KIMD_DEBUG
    WRMSG(HHC90108, "D", r2, (regs)->GR(r2));
    WRMSG(HHC90108, "D", r2 + 1, (regs)->GR(r2 + 1));
#endif /* #ifdef OPTION_KIMD_DEBUG */

    /* check for end of data */
    if(unlikely(GR_A(r2 + 1, regs) < (unsigned) message_blocklen))
    {
      if(unlikely(klmd))
        return;
      regs->psw.cc = 0;
      return;
    }
  }

  /* CPU-determined amount of data processed */
  regs->psw.cc = 3;
}

#if defined( FEATURE_077_MSA_EXTENSION_FACILITY_4 )
/*----------------------------------------------------------------------------*/
/* Compute intermediate message digest (KIMD) FC 65                           */
/*----------------------------------------------------------------------------*/
static void ARCH_DEP(kimd_ghash)(int r1, int r2, REGS *regs)
{
  int crypted;
  int i;
  BYTE message_block[16];
  BYTE parameter_block[32];

  UNREFERENCED(r1);

  /* Check special conditions */
  if(unlikely(GR_A(r2 + 1, regs) % 16))
    ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);

  /* Return with cc 0 on zero length */
  if(unlikely(!GR_A(r2 + 1, regs)))
  {
    regs->psw.cc = 0;
    return;
  }

  /* Test writeability output chaining value */
  ARCH_DEP(validate_operand)(GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, 15, ACCTYPE_WRITE, regs);

  /* Fetch the parameter block */
  ARCH_DEP(vfetchc)(parameter_block, 31, GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, regs);

#ifdef OPTION_KIMD_DEBUG
  LOGBYTE("icv   :", parameter_block, 16);
  LOGBYTE("h     :", &parameter_block[16], 16);
#endif /* #ifdef OPTION_KIMD_DEBUG */

  /* Try to process the CPU-determined amount of data */
  for(crypted = 0; crypted < PROCESS_MAX; crypted += 16)
  {
    /* Fetch and process a block of data */
    ARCH_DEP(vfetchc)(message_block, 15, GR_A(r2, regs) & ADDRESS_MAXWRAP(regs), r2, regs);

#ifdef OPTION_KIMD_DEBUG
    LOGBYTE("input :", message_block, 16);
#endif /* #ifdef OPTION_KIMD_DEBUG */

    /* XOR and multiply */
    for(i = 0; i < 16; i++)
      parameter_block[i] ^= message_block[i];
    gcm_gf_mult(parameter_block, &parameter_block[16], parameter_block);

    /* Store the output chaining value */
    ARCH_DEP(vstorec)(parameter_block, 15, GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, regs);

#ifdef OPTION_KIMD_DEBUG
    LOGBYTE("ocv   :", parameter_block, 16);
#endif /* #ifdef OPTION_KIMD_DEBUG */

    /* Update the registers */
    SET_GR_A(r2, regs, GR_A(r2, regs) + 16);
    SET_GR_A(r2 + 1, regs, GR_A(r2 + 1, regs) - 16);

#ifdef OPTION_KIMD_DEBUG
    WRMSG(HHC90108, "D", r2, (regs)->GR(r2));
    WRMSG(HHC90108, "D", r2 + 1, (regs)->GR(r2 + 1));
#endif /* #ifdef OPTION_KIMD_DEBUG */

    /* check for end of data */
    if(unlikely(!GR_A(r2 + 1, regs)))
    {
      regs->psw.cc = 0;
      return;
    }
  }

  /* CPU-determined amount of data processed */
  regs->psw.cc = 3;
}
#endif /* defined( FEATURE_077_MSA_EXTENSION_FACILITY_4 ) */

/*----------------------------------------------------------------------------*/
/* Compute last message digest (KLMD) FC 1-3                                  */
/*----------------------------------------------------------------------------*/
static void ARCH_DEP(klmd_sha)(int r1, int r2, REGS *regs)
{
  SHA1_CTX sha1_ctx;

#if defined( FEATURE_MSA_EXTENSION_FACILITY_1 )
  SHA2_CTX sha2_ctx;
#endif /* defined( FEATURE_MSA_EXTENSION_FACILITY_1 ) */

#if defined( FEATURE_MSA_EXTENSION_FACILITY_2 )
  SHA2_CTX sha512_ctx;
#endif /* defined( FEATURE_MSA_EXTENSION_FACILITY_2 ) */

  int fc;
  int i;
  int mbllen = 0;
  BYTE message_block[128];
  int message_blocklen = 0;
  BYTE parameter_block[80];
  int parameter_blocklen = 0;

  UNREFERENCED(r1);

  /* Initialize values */
  fc = GR0_fc(regs);
  switch(fc)
  {
    case 1: /* sha-1 */
    {
      mbllen = 8;
      message_blocklen = 64;
      parameter_blocklen = 20;
      break;
    }

#if defined( FEATURE_MSA_EXTENSION_FACILITY_1 )
    case 2: /* sha-256 */
    {
      mbllen = 8;
      message_blocklen = 64;
      parameter_blocklen = 32;
      break;
    }
#endif /* defined( FEATURE_MSA_EXTENSION_FACILITY_1 ) */

#if defined( FEATURE_MSA_EXTENSION_FACILITY_2 )
    case 3: /* sha-512 */
    {
      mbllen = 16;
      message_blocklen = 128;
      parameter_blocklen = 64;
    }
#endif /* defined( FEATURE_MSA_EXTENSION_FACILITY_2 ) */

  }

  /* Process intermediate message blocks */
  if(unlikely(GR_A(r2 + 1, regs) >= (unsigned) message_blocklen))
  {
    ARCH_DEP(kimd_sha)(r1, r2, regs, 1);
    if(regs->psw.cc == 3)
      return;
  }

  /* Test writeability output chaining value */
  ARCH_DEP(validate_operand)(GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, parameter_blocklen - 1, ACCTYPE_WRITE, regs);

  /* Fetch the parameter block */
  ARCH_DEP(vfetchc)(parameter_block, parameter_blocklen + mbllen - 1, GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, regs);

#ifdef OPTION_KLMD_DEBUG
  if(parameter_blocklen > 32)
  {
    LOGBYTE2("icv   :", parameter_block, 16, parameter_blocklen / 16);
  }
  else
  {
    LOGBYTE("icv   :", parameter_block, parameter_blocklen);
  }
  LOGBYTE("mbl   :", &parameter_block[parameter_blocklen], mbllen);
#endif /* #ifdef OPTION_KLMD_DEBUG */

  /* Set initial chaining value */
  switch(fc)
  {
    case 1: /* sha-1 */
    {
      sha1_seticv(&sha1_ctx, parameter_block);
      break;
    }

#if defined( FEATURE_MSA_EXTENSION_FACILITY_1 )
    case 2: /* sha-256 */
    {
      sha256_seticv(&sha2_ctx, parameter_block);
      break;
    }
#endif /* defined( FEATURE_MSA_EXTENSION_FACILITY_1 ) */

#if defined( FEATURE_MSA_EXTENSION_FACILITY_2 )
    case 3: /* sha-512 */
    {
      sha512_seticv(&sha512_ctx, parameter_block);
      break;
    }
#endif /* defined( FEATURE_MSA_EXTENSION_FACILITY_2 ) */

  }

  /* Fetch and process possible last block of data */
  if(likely(GR_A(r2 + 1, regs)))
  {
    ARCH_DEP(vfetchc)(message_block, GR_A(r2 + 1, regs) - 1, GR_A(r2, regs) & ADDRESS_MAXWRAP(regs), r2, regs);

#ifdef OPTION_KLMD_DEBUG
    if(GR_A(r2 + 1, regs) > 32)
    {
      LOGBYTE("input :", message_block, 32);
      LOGBYTE("       ", &message_block[32], (int) GR_A(r2 + 1, regs) - 32);
    }
    else
      LOGBYTE("input :", message_block, (int) GR_A(r2 + 1, regs));
#endif /* #ifdef OPTION_KLMD_DEBUG */

  }

  /* Do the padding */
  i = GR_A(r2 + 1, regs);
  if(unlikely(i >= (message_blocklen - mbllen)))
  {
    message_block[i++] = 0x80;
    while(i < message_blocklen)
      message_block[i++] = 0x00;
    switch(fc)
    {
      case 1: /* sha-1 */
      {
        SHA1Transform(sha1_ctx.state, message_block);
        break;
      }

#if defined( FEATURE_MSA_EXTENSION_FACILITY_1 )
      case 2: /* sha-256 */
      {
        SHA256Transform(sha2_ctx.state.st32, message_block);
        break;
      }
#endif /* defined( FEATURE_MSA_EXTENSION_FACILITY_1 ) */

#if defined( FEATURE_MSA_EXTENSION_FACILITY_2 )
      case 3: /* sha-512 */
      {
        SHA512Transform(sha512_ctx.state.st64, message_block);
        break;
      }
#endif /* defined( FEATURE_MSA_EXTENSION_FACILITY_2 ) */

    }
    for(i = 0; i < message_blocklen - mbllen; i++)
      message_block[i] = 0x00;
  }
  else
  {
    message_block[i++] = 0x80;
    while(i < message_blocklen - mbllen)
      message_block[i++] = 0x00;
  }

  /* Set the message bit length */
  memcpy(&message_block[message_blocklen - mbllen], &parameter_block[parameter_blocklen], mbllen);

  /* Calculate and store the message digest */
  switch(fc)
  {
    case 1: /* sha-1 */
    {
      SHA1Transform(sha1_ctx.state, message_block);
      sha1_getcv(&sha1_ctx, parameter_block);
      break;
    }

#if defined( FEATURE_MSA_EXTENSION_FACILITY_1 )
    case 2: /* sha-256 */
    {
      SHA256Transform(sha2_ctx.state.st32, message_block);
      sha256_getcv(&sha2_ctx, parameter_block);
      break;
    }
#endif /* defined( FEATURE_MSA_EXTENSION_FACILITY_1 ) */

#if defined( FEATURE_MSA_EXTENSION_FACILITY_2 )
    case 3: /* sha-512 */
    {
      SHA512Transform(sha512_ctx.state.st64, message_block);
      sha512_getcv(&sha512_ctx, parameter_block);
      break;
    }
#endif /* defined( FEATURE_MSA_EXTENSION_FACILITY_2 ) */

  }
  ARCH_DEP(vstorec)(parameter_block, parameter_blocklen - 1, GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, regs);

#ifdef OPTION_KLMD_DEBUG
  if(parameter_blocklen > 32)
  {
    LOGBYTE2("md    :", parameter_block, 16, parameter_blocklen / 16);
  }
  else
  {
    LOGBYTE("md    :", parameter_block, parameter_blocklen);
  }
#endif /* #ifdef OPTION_KLMD_DEBUG */

  /* Update registers */
  SET_GR_A(r2, regs, GR_A(r2, regs) + GR_A(r2 + 1, regs));
  SET_GR_A(r2 + 1, regs, 0);

#ifdef OPTION_KLMD_DEBUG
  WRMSG(HHC90108, "D", r2, (regs)->GR(r2));
  WRMSG(HHC90108, "D", r2 + 1, (regs)->GR(r2 + 1));
#endif /* #ifdef OPTION_KLMD_DEBUG */

  /* Set condition code */
  regs->psw.cc = 0;
}

#if defined( FEATURE_057_MSA_EXTENSION_FACILITY_5 )
/*----------------------------------------------------------------------------*/
/* Compute intermediate message digest (KIMD) FC 32-37                        */
/*----------------------------------------------------------------------------*/
/* SHA3-224, SHA3-256, SHA3-384, SHA3-512, SHAKE-128 and SHAKE-256 (MSA 6).   */
/* Message-Security-Assist Extension 6 has no facility bit of its own; like   */
/* extension 7 it is provided together with extension 5.                      */
/*                                                                            */
/* The parameter block is the 200-byte Keccak state. The second operand       */
/* length must be a multiple of the block size (rate) of the function.        */
/*----------------------------------------------------------------------------*/
static void ARCH_DEP(kimd_sha3)(int r1, int r2, REGS *regs)
{
  int crypted;
  int i;
  BYTE message_block[168];
  int rate;
  BYTE state[200];

  UNREFERENCED(r1);

  /* Initialize values */
  rate = sha3_rate(GR0_fc(regs));

  /* Check special conditions */
  if(unlikely(GR_A(r2 + 1, regs) % rate))
    ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);

  /* Return with cc 0 on zero length */
  if(unlikely(!GR_A(r2 + 1, regs)))
  {
    regs->psw.cc = 0;
    return;
  }

  /* Test writeability of the state */
  ARCH_DEP(validate_operand)(GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, 199, ACCTYPE_WRITE, regs);

  /* Fetch the state */
  ARCH_DEP(vfetchc)(state, 199, GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, regs);

#ifdef OPTION_KIMD_DEBUG
  LOGBYTE("icv   :", state, 32);
#endif /* #ifdef OPTION_KIMD_DEBUG */

  /* Try to process the CPU-determined amount of data */
  for(crypted = 0; crypted < PROCESS_MAX; crypted += rate)
  {
    /* Fetch and absorb a block of data */
    ARCH_DEP(vfetchc)(message_block, rate - 1, GR_A(r2, regs) & ADDRESS_MAXWRAP(regs), r2, regs);
    for(i = 0; i < rate; i++)
      state[i] ^= message_block[i];
    keccak_f1600(state);

    /* Store the state */
    ARCH_DEP(vstorec)(state, 199, GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, regs);

#ifdef OPTION_KIMD_DEBUG
    LOGBYTE("ocv   :", state, 32);
#endif /* #ifdef OPTION_KIMD_DEBUG */

    /* Update the registers */
    SET_GR_A(r2, regs, GR_A(r2, regs) + rate);
    SET_GR_A(r2 + 1, regs, GR_A(r2 + 1, regs) - rate);

#ifdef OPTION_KIMD_DEBUG
    WRMSG(HHC90108, "D", r2, (regs)->GR(r2));
    WRMSG(HHC90108, "D", r2 + 1, (regs)->GR(r2 + 1));
#endif /* #ifdef OPTION_KIMD_DEBUG */

    /* check for end of data */
    if(unlikely(!GR_A(r2 + 1, regs)))
    {
      regs->psw.cc = 0;
      return;
    }
  }

  /* CPU-determined amount of data processed */
  regs->psw.cc = 3;
}

/*----------------------------------------------------------------------------*/
/* Compute last message digest (KLMD) FC 32-37                                */
/*----------------------------------------------------------------------------*/
/* SHA3-224, SHA3-256, SHA3-384, SHA3-512, SHAKE-128 and SHAKE-256 (MSA 6).   */
/*                                                                            */
/* The parameter block is the 200-byte Keccak state. Full blocks of the       */
/* second operand are absorbed, then the remaining bytes (possibly none) are  */
/* padded (SHA-3: 0x06...0x80, SHAKE: 0x1F...0x80) and absorbed. For SHA-3    */
/* the digest is the leftmost part of the resulting state.                    */
/*                                                                            */
/* For SHAKE the output is squeezed into the first operand (R1, R1+1). The    */
/* first output block is taken from the state after the padded block has     */
/* been absorbed, every further block after another permutation. When the    */
/* padding has been processed, the padding state (PS) bit 55 of GR0 is set    */
/* to one, so that a resumed execution (cc 3) or a later execution with PS    */
/* set by the program only squeezes: permute, then output the next block.     */
/*----------------------------------------------------------------------------*/
static void ARCH_DEP(klmd_sha3)(int r1, int r2, REGS *regs)
{
  int i;
  BYTE message_block[168];
  int n;
  int processed;
  int rate;
  int shake;
  BYTE state[200];

  /* Initialize values */
  rate = sha3_rate(GR0_fc(regs));
  shake = GR0_fc(regs) >= 36;

  /* Check special conditions */
  if(unlikely(shake && (!r1 || r1 & 0x01)))
    ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);

  /* Test writeability of the state */
  ARCH_DEP(validate_operand)(GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, 199, ACCTYPE_WRITE, regs);

  /* Fetch the state */
  ARCH_DEP(vfetchc)(state, 199, GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, regs);

#ifdef OPTION_KLMD_DEBUG
  LOGBYTE("icv   :", state, 32);
#endif /* #ifdef OPTION_KLMD_DEBUG */

  processed = 0;
  if(!shake || !GR0_klmd_ps(regs))
  {
    /* Absorb the full blocks */
    while(GR_A(r2 + 1, regs) >= (U64) rate)
    {
      /* CPU-determined amount of data processed */
      if(processed >= PROCESS_MAX)
      {
        regs->psw.cc = 3;
        return;
      }

      ARCH_DEP(vfetchc)(message_block, rate - 1, GR_A(r2, regs) & ADDRESS_MAXWRAP(regs), r2, regs);
      for(i = 0; i < rate; i++)
        state[i] ^= message_block[i];
      keccak_f1600(state);
      ARCH_DEP(vstorec)(state, 199, GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, regs);

      SET_GR_A(r2, regs, GR_A(r2, regs) + rate);
      SET_GR_A(r2 + 1, regs, GR_A(r2 + 1, regs) - rate);
      processed += rate;
    }

    /* Pad and absorb the last (partial or empty) block */
    n = (int) GR_A(r2 + 1, regs);
    memset(message_block, 0, rate);
    if(n)
      ARCH_DEP(vfetchc)(message_block, n - 1, GR_A(r2, regs) & ADDRESS_MAXWRAP(regs), r2, regs);
    message_block[n] ^= shake ? 0x1F : 0x06;
    message_block[rate - 1] ^= 0x80;
    for(i = 0; i < rate; i++)
      state[i] ^= message_block[i];
    keccak_f1600(state);

    if(shake)
    {
      /* Output the first block taken from the current state */
      n = (int) min((U64) rate, GR_A(r1 + 1, regs));
      if(n)
        ARCH_DEP(vstorec)(state, n - 1, GR_A(r1, regs) & ADDRESS_MAXWRAP(regs), r1, regs);
    }

    /* Store the state */
    ARCH_DEP(vstorec)(state, 199, GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, regs);

#ifdef OPTION_KLMD_DEBUG
    LOGBYTE("ocv   :", state, 32);
#endif /* #ifdef OPTION_KLMD_DEBUG */

    /* Update the registers */
    SET_GR_A(r2, regs, GR_A(r2, regs) + GR_A(r2 + 1, regs));
    SET_GR_A(r2 + 1, regs, 0);
    if(!shake)
    {
      regs->psw.cc = 0;
      return;
    }
    SET_GR_A(r1, regs, GR_A(r1, regs) + n);
    SET_GR_A(r1 + 1, regs, GR_A(r1 + 1, regs) - n);
    SET_GR0_klmd_ps(regs);
    processed += rate;
  }

  /* Squeeze further output blocks (SHAKE only) */
  while(GR_A(r1 + 1, regs))
  {
    /* CPU-determined amount of data processed */
    if(processed >= PROCESS_MAX)
    {
      regs->psw.cc = 3;
      return;
    }

    keccak_f1600(state);
    n = (int) min((U64) rate, GR_A(r1 + 1, regs));
    ARCH_DEP(vstorec)(state, n - 1, GR_A(r1, regs) & ADDRESS_MAXWRAP(regs), r1, regs);
    ARCH_DEP(vstorec)(state, 199, GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, regs);

    SET_GR_A(r1, regs, GR_A(r1, regs) + n);
    SET_GR_A(r1 + 1, regs, GR_A(r1 + 1, regs) - n);
    processed += rate;
  }

#ifdef OPTION_KLMD_DEBUG
  WRMSG(HHC90108, "D", r1, (regs)->GR(r1));
  WRMSG(HHC90108, "D", r1 + 1, (regs)->GR(r1 + 1));
  WRMSG(HHC90108, "D", r2, (regs)->GR(r2));
  WRMSG(HHC90108, "D", r2 + 1, (regs)->GR(r2 + 1));
#endif /* #ifdef OPTION_KLMD_DEBUG */

  regs->psw.cc = 0;
}
#endif /* defined( FEATURE_057_MSA_EXTENSION_FACILITY_5 ) */

/*----------------------------------------------------------------------------*/
/* Cipher message (KM) FC 1-3 and 9-11                                        */
/*----------------------------------------------------------------------------*/
static void ARCH_DEP(km_dea)(int r1, int r2, REGS *regs)
{
  int crypted;
  des_context des_ctx;
  des3_context des3_ctx;
  int keylen;
  BYTE message_block[8];
  int modifier_bit;
  BYTE parameter_block[48];
  int parameter_blocklen;
  int r1_is_not_r2;
  int tfc;
  int wrap;

  /* Check special conditions */
  if(unlikely(GR_A(r2 + 1, regs) % 8))
    ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);

  /* Return with cc 0 on zero length */
  if(unlikely(!GR_A(r2 + 1, regs)))
  {
    regs->psw.cc = 0;
    return;
  }

  /* Initialize values */
  tfc = GR0_tfc(regs);
  wrap = GR0_wrap(regs);
  keylen = tfc * 8;
  parameter_blocklen = keylen;
  if(wrap)
    parameter_blocklen += 24;

  /* Fetch the parameter block */
  ARCH_DEP(vfetchc)(parameter_block, parameter_blocklen - 1, GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, regs);

#ifdef OPTION_KM_DEBUG
  switch(tfc)
  {
    case 1: /* dea */
    {
      LOGBYTE("k     :", parameter_block, 8);
      break;
    }
    case 2: /* tdea-128 */
    {
      LOGBYTE("k1    :", parameter_block, 8);
      LOGBYTE("k2    :", &parameter_block[8], 8);
      break;
    }
    case 3: /* tdea-192 */
    {
      LOGBYTE("k1    :", parameter_block, 8);
      LOGBYTE("k2    :", &parameter_block[8], 8);
      LOGBYTE("k3    :", &parameter_block[16], 8);
      break;
    }
  }
  if(wrap)
    LOGBYTE("wkvp  :", &parameter_block[keylen], 24);
#endif /* #ifdef OPTION_KM_DEBUG */

#if defined( FEATURE_076_MSA_EXTENSION_FACILITY_3 )
  /* Verify and unwrap */
  if(wrap && unwrap_dea(parameter_block, keylen))
  {

#ifdef OPTION_KM_DEBUG
    WRMSG(HHC90111, "D");
#endif /* #ifdef OPTION_KM_DEBUG */

    regs->psw.cc = 1;
    return;
  }
#endif /* defined( FEATURE_076_MSA_EXTENSION_FACILITY_3 ) */

  /* Set the cryptographic key */
  switch(tfc)
  {
    case 1: /* dea */
    {
      des_set_key(&des_ctx, parameter_block);
      break;
    }
    case 2: /* tdea-128 */
    {
      des3_set_2keys(&des3_ctx, parameter_block, &parameter_block[8]);
      break;
    }
    case 3: /* tdea-192 */
    {
      des3_set_3keys(&des3_ctx, parameter_block, &parameter_block[8], &parameter_block[16]);
      break;
    }
  }

  /* Try to process the CPU-determined amount of data */
  modifier_bit = GR0_m(regs);
  r1_is_not_r2 = r1 != r2;
  for(crypted = 0; crypted < PROCESS_MAX; crypted += 8)
  {
    /* Fetch a block of data */
    ARCH_DEP(vfetchc)(message_block, 7, GR_A(r2, regs) & ADDRESS_MAXWRAP(regs), r2, regs);

#ifdef OPTION_KM_DEBUG
    LOGBYTE("input :", message_block, 8);
#endif /* #ifdef OPTION_KM_DEBUG */

    /* Do the job */
    switch(tfc)
    {
      case 1: /* dea */
      {
        if(modifier_bit)
          des_decrypt(&des_ctx, message_block, message_block);
        else
          des_encrypt(&des_ctx, message_block, message_block);
        break;
      }
      case 2: /* tdea-128 */
      case 3: /* tdea-192 */
      {
        if(modifier_bit)
          des3_decrypt(&des3_ctx, message_block, message_block);
        else
          des3_encrypt(&des3_ctx, message_block, message_block);
        break;
      }
    }

    /* Store the output */
    ARCH_DEP(vstorec)(message_block, 7, GR_A(r1, regs) & ADDRESS_MAXWRAP(regs), r1, regs);

#ifdef OPTION_KM_DEBUG
    LOGBYTE("output:", message_block, 8);
#endif /* #ifdef OPTION_KM_DEBUG */

    /* Update the registers */
    SET_GR_A(r1, regs, GR_A(r1, regs) + 8);
    if(likely(r1_is_not_r2))
      SET_GR_A(r2, regs, GR_A(r2, regs) + 8);
    SET_GR_A(r2 + 1, regs, GR_A(r2 + 1, regs) - 8);

#ifdef OPTION_KM_DEBUG
    WRMSG(HHC90108, "D", r1, (regs)->GR(r1));
    WRMSG(HHC90108, "D", r2, (regs)->GR(r2));
    WRMSG(HHC90108, "D", r2 + 1, (regs)->GR(r2 + 1));
#endif /* #ifdef OPTION_KM_DEBUG */

    /* check for end of data */
    if(unlikely(!GR_A(r2 + 1, regs)))
    {
      regs->psw.cc = 0;
      return;
    }
  }

  /* CPU-determined amount of data processed */
  regs->psw.cc = 3;
}

#if defined( FEATURE_MSA_EXTENSION_FACILITY_1 )
/*----------------------------------------------------------------------------*/
/* Cipher message (KM) FC 18-20 and 26-28                                     */
/*----------------------------------------------------------------------------*/
static void ARCH_DEP(km_aes)(int r1, int r2, REGS *regs)
{
  rijndael_ctx context;
  int crypted;
  int keylen;
  BYTE message_block[16];
  int modifier_bit;
  BYTE parameter_block[64];
  int parameter_blocklen;
  int r1_is_not_r2;
  int tfc;
  int wrap;

  /* Check special conditions */
  if(unlikely(GR_A(r2 + 1, regs) % 16))
    ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);

  /* Return with cc 0 on zero length */
  if(unlikely(!GR_A(r2 + 1, regs)))
  {
    regs->psw.cc = 0;
    return;
  }

  /* Initialize values */
  tfc = GR0_tfc(regs);
  wrap = GR0_wrap(regs);
  keylen = (tfc - 17) * 8 + 8;
  parameter_blocklen = keylen;
  if(wrap)
    parameter_blocklen += 32;

  /* Fetch the parameter block */
  ARCH_DEP(vfetchc)(parameter_block, parameter_blocklen - 1, GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, regs);

#ifdef OPTION_KM_DEBUG
  LOGBYTE("k     :", parameter_block, keylen);
  if(wrap)
    LOGBYTE("wkvp  :", &parameter_block[keylen], 32);
#endif /* #ifdef OPTION_KM_DEBUG */

#if defined( FEATURE_076_MSA_EXTENSION_FACILITY_3 )
  /* Verify and unwrap */
  if(wrap && unwrap_aes(parameter_block, keylen))
  {

#ifdef OPTION_KM_DEBUG
    WRMSG(HHC90111, "D");
#endif /* #ifdef OPTION_KM_DEBUG */

    regs->psw.cc = 1;
    return;
  }
#endif /* defined( FEATURE_076_MSA_EXTENSION_FACILITY_3 ) */

  /* Set the cryptographic keys */
  rijndael_set_key(&context, parameter_block, keylen * 8);

  /* Try to process the CPU-determined amount of data */
  modifier_bit = GR0_m(regs);
  r1_is_not_r2 = r1 != r2;
  for(crypted = 0; crypted < PROCESS_MAX; crypted += 16)
  {
    /* Fetch a block of data */
    ARCH_DEP(vfetchc)(message_block, 15, GR_A(r2, regs) & ADDRESS_MAXWRAP(regs), r2, regs);

#ifdef OPTION_KM_DEBUG
    LOGBYTE("input :", message_block, 16);
#endif /* #ifdef OPTION_KM_DEBUG */

    /* Do the job */
    if(modifier_bit)
      rijndael_decrypt(&context, message_block, message_block);
    else
      rijndael_encrypt(&context, message_block, message_block);

    /* Store the output */
    ARCH_DEP(vstorec)(message_block, 15, GR_A(r1, regs) & ADDRESS_MAXWRAP(regs), r1, regs);

#ifdef OPTION_KM_DEBUG
    LOGBYTE("output:", message_block, 16);
#endif /* #ifdef OPTION_KM_DEBUG */

    /* Update the registers */
    SET_GR_A(r1, regs, GR_A(r1, regs) + 16);
    if(likely(r1_is_not_r2))
      SET_GR_A(r2, regs, GR_A(r2, regs) + 16);
    SET_GR_A(r2 + 1, regs, GR_A(r2 + 1, regs) - 16);

#ifdef OPTION_KM_DEBUG
    WRMSG(HHC90108, "D", r1, (regs)->GR(r1));
    WRMSG(HHC90108, "D", r2, (regs)->GR(r2));
    WRMSG(HHC90108, "D", (regs)->GR(r2 + 1));
#endif /* #ifdef OPTION_KM_DEBUG */

    /* check for end of data */
    if(unlikely(!GR_A(r2 + 1, regs)))
    {
      regs->psw.cc = 0;
      return;
    }
  }

  /* CPU-determined amount of data processed */
  regs->psw.cc = 3;
}
#endif /* defined( FEATURE_MSA_EXTENSION_FACILITY_1 ) */

#if defined( FEATURE_077_MSA_EXTENSION_FACILITY_4 )
/*----------------------------------------------------------------------------*/
/* Cipher message (KM) FC 50, 52, 58 and 60                                   */
/*----------------------------------------------------------------------------*/
static void ARCH_DEP(km_xts_aes)(int r1, int r2, REGS *regs)
{
  rijndael_ctx context;
  int crypted;
  int i;
  int keylen;
  BYTE message_block[16];
  int modifier_bit;
  BYTE parameter_block[80];
  int parameter_blocklen;
  int r1_is_not_r2;
  int tfc;
  int wrap;
  BYTE *xts;

  /* Check special conditions */
  if(unlikely(GR_A(r2 + 1, regs) % 16))
    ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);

  /* Return with cc 0 on zero length */
  if(unlikely(!GR_A(r2 + 1, regs)))
  {
    regs->psw.cc = 0;
    return;
  }

  /* Initialize values */
  tfc = GR0_tfc(regs);
  wrap = GR0_wrap(regs);
  keylen = (tfc - 49) * 8 + 8;
  parameter_blocklen = keylen + 16;
  if(wrap)
    parameter_blocklen += 32;

  /* Test writeability output chaining value */
  ARCH_DEP(validate_operand)((GR_A(1, regs) + parameter_blocklen - 16) & ADDRESS_MAXWRAP(regs), 1, 15, ACCTYPE_WRITE, regs);

  /* Fetch the parameter block */
  ARCH_DEP(vfetchc)(parameter_block, parameter_blocklen - 1, GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, regs);
  xts = &parameter_block[parameter_blocklen - 16];

#ifdef OPTION_KM_DEBUG
  LOGBYTE("k     :", parameter_block, keylen);
  if(wrap)
    LOGBYTE("wkvp  :", &parameter_block[keylen], 32);
  LOGBYTE("xts   :", xts, 16);
#endif /* #ifdef OPTION_KM_DEBUG */

  /* Verify and unwrap */
  if(wrap && unwrap_aes(parameter_block, keylen))
  {

#ifdef OPTION_KM_DEBUG
    WRMSG(HHC90111, "D");
#endif /* #ifdef OPTION_KM_DEBUG */

    regs->psw.cc = 1;
    return;
  }

  /* Set the cryptographic keys */
  rijndael_set_key(&context, parameter_block, keylen * 8);

  /* Try to process the CPU-determined amount of data */
  modifier_bit = GR0_m(regs);
  r1_is_not_r2 = r1 != r2;
  for(crypted = 0; crypted < PROCESS_MAX; crypted += 16)
  {
    /* Fetch a block of data */
    ARCH_DEP(vfetchc)(message_block, 15, GR_A(r2, regs) & ADDRESS_MAXWRAP(regs), r2, regs);

#ifdef OPTION_KM_DEBUG
    LOGBYTE("input :", message_block, 16);
#endif /* #ifdef OPTION_KM_DEBUG */

    /* XOR, decrypt/encrypt and XOR again*/
    for(i = 0; i < 16; i++)
      message_block[i] ^= parameter_block[parameter_blocklen - 16 + i];
    if(modifier_bit)
      rijndael_decrypt(&context, message_block, message_block);
    else
      rijndael_encrypt(&context, message_block, message_block);
    for(i = 0; i < 16; i++)
      message_block[i] ^= parameter_block[parameter_blocklen - 16 + i];

    /* Calculate output XTS */
    xts_mult_x(xts);

    /* Store the output and XTS */
    ARCH_DEP(vstorec)(message_block, 15, GR_A(r1, regs) & ADDRESS_MAXWRAP(regs), r1, regs);
    ARCH_DEP(vstorec)(xts, 15, (GR_A(1, regs) + parameter_blocklen - 16) & ADDRESS_MAXWRAP(regs), 1, regs);

#ifdef OPTION_KM_DEBUG
    LOGBYTE("output:", message_block, 16);
    LOGBYTE("xts   :", xts, 16);
#endif /* #ifdef OPTION_KM_DEBUG */

    /* Update the registers */
    SET_GR_A(r1, regs, GR_A(r1, regs) + 16);
    if(likely(r1_is_not_r2))
      SET_GR_A(r2, regs, GR_A(r2, regs) + 16);
    SET_GR_A(r2 + 1, regs, GR_A(r2 + 1, regs) - 16);

#ifdef OPTION_KM_DEBUG
    WRMSG(HHC90108, "D", r1, (regs)->GR(r1));
    WRMSG(HHC90108, "D", r2, (regs)->GR(r2));
    WRMSG(HHC90108, "D", r2 + 1, (regs)->GR(r2 + 1));
#endif /* #ifdef OPTION_KM_DEBUG */

    /* check for end of data */
    if(unlikely(!GR_A(r2 + 1, regs)))
    {
      regs->psw.cc = 0;
      return;
    }
  }

  /* CPU-determined amount of data processed */
  regs->psw.cc = 3;
}
#endif /* defined( FEATURE_077_MSA_EXTENSION_FACILITY_4 ) */

/*----------------------------------------------------------------------------*/
/* Compute message authentication code (KMAC) FC 1-3 and 9-11                 */
/*----------------------------------------------------------------------------*/
static void ARCH_DEP(kmac_dea)(int r1, int r2, REGS *regs)
{
  des_context context1;
  des_context context2;
  des_context context3;
  int crypted;
  int i;
  int keylen;
  BYTE message_block[8];
  BYTE parameter_block[56];
  int parameter_blocklen;
  int tfc;
  int wrap;

  UNREFERENCED(r1);

  /* Check special conditions */
  if(unlikely(GR_A(r2 + 1, regs) % 8))
    ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);

  /* Return with cc 0 on zero length */
  if(unlikely(!GR_A(r2 + 1, regs)))
  {
    regs->psw.cc = 0;
    return;
  }

  /* Initialize values */
  tfc = GR0_tfc(regs);
  wrap = GR0_wrap(regs);
  keylen = tfc * 8;
  parameter_blocklen = keylen + 8;
  if(wrap)
    parameter_blocklen += 24;

  /* Test writeability output chaining value */
  ARCH_DEP(validate_operand)(GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, 7, ACCTYPE_WRITE, regs);

  /* Fetch the parameter block */
  ARCH_DEP(vfetchc)(parameter_block, parameter_blocklen - 1, GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, regs);

#ifdef OPTION_KMAC_DEBUG
  LOGBYTE("icv   :", parameter_block, 8);
  switch(tfc)
  {
    case 1: /* dea */
    {
      LOGBYTE("k1    :", &parameter_block[8], 8);
      break;
    }
    case 2: /* tdea-128 */
    {
      LOGBYTE("k1    :", &parameter_block[8], 8);
      LOGBYTE("k2    :", &parameter_block[16], 8);
      break;
    }
    case 3: /* tdea-192 */
    {
      LOGBYTE("k1    :", &parameter_block[8], 8);
      LOGBYTE("k2    :", &parameter_block[16], 8);
      LOGBYTE("k3    :", &parameter_block[24], 8);
      break;
    }
  }
  if(wrap)
    LOGBYTE("wkvp  :", &parameter_block[keylen + 8], 24);
#endif /* #ifdef OPTION_KMAC_DEBUG */

#if defined( FEATURE_076_MSA_EXTENSION_FACILITY_3 )
  /* Verify and unwrap */
  if(wrap && unwrap_dea(&parameter_block[8], keylen))
  {

#ifdef OPTION_KM_DEBUG
    WRMSG(HHC90111, "D");
#endif /* #ifdef OPTION_KM_DEBUG */

    regs->psw.cc = 1;
    return;
  }
#endif /* defined( FEATURE_076_MSA_EXTENSION_FACILITY_3 ) */

  /* Set the cryptographic key */
  switch(tfc)
  {
    case 1: /* dea */
    {
      des_set_key(&context1, &parameter_block[8]);
      break;
    }
    case 2: /* tdea-128 */
    {
      des_set_key(&context1, &parameter_block[8]);
      des_set_key(&context2, &parameter_block[16]);
      break;
    }
    case 3: /* tdea-192 */
    {
      des_set_key(&context1, &parameter_block[8]);
      des_set_key(&context2, &parameter_block[16]);
      des_set_key(&context3, &parameter_block[24]);
      break;
    }
  }

  /* Try to process the CPU-determined amount of data */
  for(crypted = 0; crypted < PROCESS_MAX; crypted += 8)
  {
    /* Fetch a block of data */
    ARCH_DEP(vfetchc)(message_block, 7, GR_A(r2, regs) & ADDRESS_MAXWRAP(regs), r2, regs);

#ifdef OPTION_KMAC_DEBUG
    LOGBYTE("input :", message_block, 8);
#endif /* #ifdef OPTION_KMAC_DEBUG */

    /* XOR the message with chaining value */
    for(i = 0; i < 8; i++)
      message_block[i] ^= parameter_block[i];

    /* Calculate the output chaining value */
    switch(tfc)
    {
      case 1: /* dea */
      {
        des_encrypt(&context1, message_block, parameter_block);
        break;
      }
      case 2: /* tdea-128 */
      {
        des_encrypt(&context1, message_block, parameter_block);
        des_decrypt(&context2, parameter_block, parameter_block);
        des_encrypt(&context1, parameter_block, parameter_block);
        break;
      }
      case 3: /* tdea-192 */
      {
        des_encrypt(&context1, message_block, parameter_block);
        des_decrypt(&context2, parameter_block, parameter_block);
        des_encrypt(&context3, parameter_block, parameter_block);
        break;
      }
    }

    /* Store the output chaining value */
    ARCH_DEP(vstorec)(parameter_block, 7, GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, regs);

#ifdef OPTION_KMAC_DEBUG
    LOGBYTE("ocv   :", parameter_block, 8);
#endif /* #ifdef OPTION_KMAC_DEBUG */

    /* Update the registers */
    SET_GR_A(r2, regs, GR_A(r2, regs) + 8);
    SET_GR_A(r2 + 1, regs, GR_A(r2 + 1, regs) - 8);

#ifdef OPTION_KMAC_DEBUG
    WRMSG(HHC90108, "D", r2, (regs)->GR(r2));
    WRMSG(HHC90108, "D", r2 + 1, (regs)->GR(r2 + 1));
#endif /* #ifdef OPTION_KMAC_DEBUG */

    /* check for end of data */
    if(unlikely(!GR_A(r2 + 1, regs)))
    {
      regs->psw.cc = 0;
      return;
    }
  }

  /* CPU-determined amount of data processed */
  regs->psw.cc = 3;
}

#if defined( FEATURE_077_MSA_EXTENSION_FACILITY_4 )
/*----------------------------------------------------------------------------*/
/* Compute message authentication code (KMAC) FC 18-20 and 26-28              */
/*----------------------------------------------------------------------------*/
static void ARCH_DEP(kmac_aes)(int r1, int r2, REGS *regs)
{
  rijndael_ctx context;
  int crypted;
  int i;
  int keylen;
  BYTE message_block[16];
  BYTE parameter_block[80];
  int parameter_blocklen;
  int tfc;
  int wrap;

  UNREFERENCED(r1);

  /* Check special conditions */
  if(unlikely(GR_A(r2 + 1, regs) % 16))
    ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);

  /* Return with cc 0 on zero length */
  if(unlikely(!GR_A(r2 + 1, regs)))
  {
    regs->psw.cc = 0;
    return;
  }

  /* Initialize values */
  tfc = GR0_tfc(regs);
  wrap = GR0_wrap(regs);
  keylen = (tfc - 17) * 8 + 8;
  parameter_blocklen = keylen + 16;
  if(wrap)
    parameter_blocklen += 32;

  /* Test writeability output chaining value */
  ARCH_DEP(validate_operand)(GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, 15, ACCTYPE_WRITE, regs);

  /* Fetch the parameter block */
  ARCH_DEP(vfetchc)(parameter_block, parameter_blocklen - 1, GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, regs);

#ifdef OPTION_KMAC_DEBUG
  LOGBYTE("icv   :", parameter_block, 16);
  LOGBYTE("k     :", &parameter_block[16], keylen);
  if(wrap)
    LOGBYTE("wkvp  :", &parameter_block[keylen + 16], 32);
#endif /* #ifdef OPTION_KMAC_DEBUG */

  /* Verify and unwrap */
  if(wrap && unwrap_aes(&parameter_block[16], keylen))
  {

#ifdef OPTION_KMAC_DEBUG
    WRMSG(HHC90111, "D");
#endif /* #ifdef OPTION_KMAC_DEBUG */

    regs->psw.cc = 1;
    return;
  }

  /* Set the cryptographic key */
  rijndael_set_key(&context, &parameter_block[16], keylen * 8);

  /* Try to process the CPU-determined amount of data */
  for(crypted = 0; crypted < PROCESS_MAX; crypted += 16)
  {
    /* Fetch a block of data */
    ARCH_DEP(vfetchc)(message_block, 15, GR_A(r2, regs) & ADDRESS_MAXWRAP(regs), r2, regs);

#ifdef OPTION_KMAC_DEBUG
    LOGBYTE("input :", message_block, 16);
#endif /* #ifdef OPTION_KMAC_DEBUG */

    /* XOR the message with chaining value */
    for(i = 0; i < 16; i++)
      message_block[i] ^= parameter_block[i];

    /* Calculate the output chaining value */
    rijndael_encrypt(&context, message_block, parameter_block);

    /* Store the output chaining value */
    ARCH_DEP(vstorec)(parameter_block, 15, GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, regs);

#ifdef OPTION_KMAC_DEBUG
    LOGBYTE("ocv   :", parameter_block, 16);
#endif /* #ifdef OPTION_KMAC_DEBUG */

    /* Update the registers */
    SET_GR_A(r2, regs, GR_A(r2, regs) + 16);
    SET_GR_A(r2 + 1, regs, GR_A(r2 + 1, regs) - 16);

#ifdef OPTION_KMAC_DEBUG
    WRMSG(HHC90108, "D", r2, (regs)->GR(r2));
    WRMSG(HHC90108, "D", r2 + 1, (regs)->GR(r2 + 1));
#endif /* #ifdef OPTION_KMAC_DEBUG */

    /* check for end of data */
    if(unlikely(!GR_A(r2 + 1, regs)))
    {
      regs->psw.cc = 0;
      return;
    }
  }

  /* CPU-determined amount of data processed */
  regs->psw.cc = 3;
}
#endif /* defined( FEATURE_077_MSA_EXTENSION_FACILITY_4 ) */

/*----------------------------------------------------------------------------*/
/* Cipher message with chaining (KMC) FC 1-3 and 9-11                         */
/*----------------------------------------------------------------------------*/
static void ARCH_DEP(kmc_dea)(int r1, int r2, REGS *regs)
{
  des_context context1;
  des_context context2;
  des_context context3;
  int crypted;
  int i;
  int keylen;
  BYTE message_block[8];
  int modifier_bit;
  BYTE ocv[8];
  BYTE parameter_block[56];
  int parameter_blocklen;
  int r1_is_not_r2;
  int tfc;
  int wrap;

  /* Check special conditions */
  if(unlikely(GR_A(r2 + 1, regs) % 8))
    ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);

  /* Return with cc 0 on zero length */
  if(unlikely(!GR_A(r2 + 1, regs)))
  {
    regs->psw.cc = 0;
    return;
  }

  /* Initialize values */
  tfc = GR0_tfc(regs);
  wrap = GR0_wrap(regs);
  keylen = tfc * 8;
  parameter_blocklen = keylen + 8;
  if(wrap)
    parameter_blocklen += 24;

  /* Test writeability output chaining value */
  ARCH_DEP(validate_operand)(GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, 7, ACCTYPE_WRITE, regs);

  /* Fetch the parameter block */
  ARCH_DEP(vfetchc)(parameter_block, parameter_blocklen - 1, GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, regs);

#ifdef OPTION_KMC_DEBUG
  LOGBYTE("icv   :", parameter_block, 8);
  switch(tfc)
  {
    case 1: /* dea */
    {
      LOGBYTE("k     :", &parameter_block[8], 8);
      break;
    }
    case 2: /* tdea-128 */
    {
      LOGBYTE("k1    :", &parameter_block[8], 8);
      LOGBYTE("k2    :", &parameter_block[16], 8);
      break;
    }
    case 3: /* tdea-192 */
    {
      LOGBYTE("k1    :", &parameter_block[8], 8);
      LOGBYTE("k2    :", &parameter_block[16], 8);
      LOGBYTE("k3    :", &parameter_block[24], 8);
      break;
    }
  }
  if(wrap)
    LOGBYTE("wkvp  :", &parameter_block[keylen + 8], 24);
#endif /* #ifdef OPTION_KMC_DEBUG */

#if defined( FEATURE_076_MSA_EXTENSION_FACILITY_3 )
  /* Verify and unwrap */
  if(wrap && unwrap_dea(&parameter_block[8], keylen))
  {

#ifdef OPTION_KMC_DEBUG
    WRMSG(HHC90111, "D");
#endif /* #ifdef OPTION_KMC_DEBUG */

    regs->psw.cc = 1;
    return;
  }
#endif /* defined( FEATURE_076_MSA_EXTENSION_FACILITY_3 ) */

  /* Set the cryptographic key */
  switch(tfc)
  {
    case 1: /* dea */
    {
      des_set_key(&context1, &parameter_block[8]);
      break;
    }
    case 2: /* tdea-128 */
    {
      des_set_key(&context1, &parameter_block[8]);
      des_set_key(&context2, &parameter_block[16]);
      break;
    }
    case 3: /* tdea-192 */
    {
      des_set_key(&context1, &parameter_block[8]);
      des_set_key(&context2, &parameter_block[16]);
      des_set_key(&context3, &parameter_block[24]);
      break;
    }
  }

  /* Try to process the CPU-determined amount of data */
  modifier_bit = GR0_m(regs);
  r1_is_not_r2 = r1 != r2;
  for(crypted = 0; crypted < PROCESS_MAX; crypted += 8)
  {
    /* Fetch a block of data */
    ARCH_DEP(vfetchc)(message_block, 7, GR_A(r2, regs) & ADDRESS_MAXWRAP(regs), r2, regs);

#ifdef OPTION_KMC_DEBUG
    LOGBYTE("input :", message_block, 8);
#endif /* #ifdef OPTION_KMC_DEBUG */

    /* Do the job */
    switch(tfc)
    {
      case 1: /* dea */
      {
        if(modifier_bit)
        {
          /* Save, decrypt and XOR */
          memcpy(ocv, message_block, 8);
          des_decrypt(&context1, message_block, message_block);
          for(i = 0; i < 8; i++)
            message_block[i] ^= parameter_block[i];
        }
        else
        {
          /* XOR, encrypt and save */
          for(i = 0; i < 8; i++)
            message_block[i] ^= parameter_block[i];
          des_encrypt(&context1, message_block, message_block);
          memcpy(ocv, message_block, 8);
        }
        break;
      }
      case 2: /* tdea-128 */
      {
        if(modifier_bit)
        {
          /* Save, decrypt and XOR */
          memcpy(ocv, message_block, 8);
          des_decrypt(&context1, message_block, message_block);
          des_encrypt(&context2, message_block, message_block);
          des_decrypt(&context1, message_block, message_block);
          for(i = 0; i < 8; i++)
            message_block[i] ^= parameter_block[i];
        }
        else
        {
          /* XOR, encrypt and save */
          for(i = 0 ; i < 8; i++)
            message_block[i] ^= parameter_block[i];
          des_encrypt(&context1, message_block, message_block);
          des_decrypt(&context2, message_block, message_block);
          des_encrypt(&context1, message_block, message_block);
          memcpy(ocv, message_block, 8);
        }
        break;
      }
      case 3: /* tdea-192 */
      {
        if(modifier_bit)
        {
          /* Save, decrypt and XOR */
          memcpy(ocv, message_block, 8);
          des_decrypt(&context3, message_block, message_block);
          des_encrypt(&context2, message_block, message_block);
          des_decrypt(&context1, message_block, message_block);
          for(i = 0; i < 8; i++)
            message_block[i] ^= parameter_block[i];
        }
        else
        {
          /* XOR, encrypt and save */
          for(i = 0; i < 8; i++)
            message_block[i] ^= parameter_block[i];
          des_encrypt(&context1, message_block, message_block);
          des_decrypt(&context2, message_block, message_block);
          des_encrypt(&context3, message_block, message_block);
          memcpy(ocv, message_block, 8);
        }
        break;
      }
    }

    /* Store the output */
    ARCH_DEP(vstorec)(message_block, 7, GR_A(r1, regs) & ADDRESS_MAXWRAP(regs), r1, regs);

#ifdef OPTION_KMC_DEBUG
    LOGBYTE("output:", message_block, 8);
#endif /* #ifdef OPTION_KMC_DEBUG */

    /* Store the output chaining value */
    ARCH_DEP(vstorec)(ocv, 7, GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, regs);

#ifdef OPTION_KMC_DEBUG
    LOGBYTE("ocv   :", ocv, 8);
#endif /* #ifdef OPTION_KMC_DEBUG */

    /* Update the registers */
    SET_GR_A(r1, regs, GR_A(r1, regs) + 8);
    if(likely(r1_is_not_r2))
      SET_GR_A(r2, regs, GR_A(r2, regs) + 8);
    SET_GR_A(r2 + 1, regs, GR_A(r2 + 1, regs) - 8);

#ifdef OPTION_KMC_DEBUG
    WRMSG(HHC90108, "D", r1, (regs)->GR(r1));
    WRMSG(HHC90108, "D", r2, (regs)->GR(r2));
    WRMSG(HHC90108, "D", r2 + 1, (regs)->GR(r2 + 1));
#endif /* #ifdef OPTION_KMC_DEBUG */

    /* check for end of data */
    if(unlikely(!GR_A(r2 + 1, regs)))
    {
      regs->psw.cc = 0;
      return;
    }

    /* Set cv for next 8 bytes */
    memcpy(parameter_block, ocv, 8);
  }

  /* CPU-determined amount of data processed */
  regs->psw.cc = 3;
}

#if defined( FEATURE_MSA_EXTENSION_FACILITY_1 )
/*----------------------------------------------------------------------------*/
/* Cipher message with chaining (KMC) FC 18-20 and 26-28                      */
/*----------------------------------------------------------------------------*/
static void ARCH_DEP(kmc_aes)(int r1, int r2, REGS *regs)
{
  rijndael_ctx context;
  int crypted;
  int i;
  int keylen;
  BYTE message_block[16];
  int modifier_bit;
  BYTE ocv[16];
  BYTE parameter_block[80];
  int parameter_blocklen;
  int r1_is_not_r2;
  int tfc;
  int wrap;

  /* Check special conditions */
  if(unlikely(GR_A(r2 + 1, regs) % 16))
    ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);

  /* Return with cc 0 on zero length */
  if(unlikely(!GR_A(r2 + 1, regs)))
  {
    regs->psw.cc = 0;
    return;
  }

  /* Initialize values */
  tfc = GR0_tfc(regs);
  wrap = GR0_wrap(regs);
  keylen = (tfc - 17) * 8 + 8;
  parameter_blocklen = keylen + 16;
  if(wrap)
    parameter_blocklen += 32;

  /* Test writeability output chaining value */
  ARCH_DEP(validate_operand)(GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, 15, ACCTYPE_WRITE, regs);

  /* Fetch the parameter block */
  ARCH_DEP(vfetchc)(parameter_block, parameter_blocklen - 1, GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, regs);

#ifdef OPTION_KMC_DEBUG
  LOGBYTE("icv   :", parameter_block, 16);
  LOGBYTE("k     :", &parameter_block[16], keylen);
  if(wrap)
    LOGBYTE("wkvp  :", &parameter_block[keylen + 16], 32);
#endif /* #ifdef OPTION_KMC_DEBUG */

#if defined( FEATURE_076_MSA_EXTENSION_FACILITY_3 )
  /* Verify and unwrap */
  if(wrap && unwrap_aes(&parameter_block[16], keylen))
  {

#ifdef OPTION_KM_DEBUG
    WRMSG(HHC90111, "D");
#endif /* #ifdef OPTION_KMC_DEBUG */

    regs->psw.cc = 1;
    return;
  }
#endif /* defined( FEATURE_076_MSA_EXTENSION_FACILITY_3 ) */

  /* Set the cryptographic key */
  rijndael_set_key(&context, &parameter_block[16], keylen * 8);

  /* Try to process the CPU-determined amount of data */
  modifier_bit = GR0_m(regs);
  r1_is_not_r2 = r1 != r2;
  for(crypted = 0; crypted < PROCESS_MAX; crypted += 16)
  {
    /* Fetch a block of data */
    ARCH_DEP(vfetchc)(message_block, 15, GR_A(r2, regs) & ADDRESS_MAXWRAP(regs), r2, regs);

#ifdef OPTION_KMC_DEBUG
    LOGBYTE("input :", message_block, 16);
#endif /* #ifdef OPTION_KMC_DEBUG */

    /* Do the job */
    if(modifier_bit)
    {

      /* Save, decrypt and XOR */
      memcpy(ocv, message_block, 16);
      rijndael_decrypt(&context, message_block, message_block);
      for(i = 0; i < 16; i++)
        message_block[i] ^= parameter_block[i];
    }
    else
    {
      /* XOR, encrypt and save */
      for(i = 0; i < 16; i++)
        message_block[i] ^= parameter_block[i];
      rijndael_encrypt(&context, message_block, message_block);
      memcpy(ocv, message_block, 16);
    }

    /* Store the output */
    ARCH_DEP(vstorec)(message_block, 15, GR_A(r1, regs) & ADDRESS_MAXWRAP(regs), r1, regs);

#ifdef OPTION_KMC_DEBUG
    LOGBYTE("output:", message_block, 16);
#endif /* #ifdef OPTION_KMC_DEBUG */

    /* Store the output chaining value */
    ARCH_DEP(vstorec)(ocv, 15, GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, regs);

#ifdef OPTION_KMC_DEBUG
    LOGBYTE("ocv   :", ocv, 16);
#endif /* #ifdef OPTION_KMC_DEBUG */

    /* Update the registers */
    SET_GR_A(r1, regs, GR_A(r1, regs) + 16);
    if(likely(r1_is_not_r2))
      SET_GR_A(r2, regs, GR_A(r2, regs) + 16);
    SET_GR_A(r2 + 1, regs, GR_A(r2 + 1, regs) - 16);

#ifdef OPTION_KMC_DEBUG
    WRMSG(HHC90108, "D", r1, (regs)->GR(r1));
    WRMSG(HHC90108, "D", r2, (regs)->GR(r2));
    WRMSG(HHC90108, "D", r2 + 1, (regs)->GR(r2 + 1));
#endif /* #ifdef OPTION_KMC_DEBUG */

    /* check for end of data */
    if(unlikely(!GR_A(r2 + 1, regs)))
    {
      regs->psw.cc = 0;
      return;
    }

    /* Set cv for next 16 bytes */
    memcpy(parameter_block, ocv, 16);
  }

  /* CPU-determined amount of data processed */
  regs->psw.cc = 3;
}
#endif /* defined( FEATURE_MSA_EXTENSION_FACILITY_1 ) */

#if defined( FEATURE_MSA_EXTENSION_FACILITY_1 )
/*----------------------------------------------------------------------------*/
/* Cipher message with chaining (KMC) FC 67                                   */
/*----------------------------------------------------------------------------*/
static void ARCH_DEP(kmc_prng)(int r1, int r2, REGS *regs)
{
  des_context context1;
  des_context context2;
  des_context context3;
  int i;
  int crypted;
  BYTE message_block[8];
  BYTE parameter_block[32];
  BYTE ocv[8];
  BYTE tcv[8];
  int r1_is_not_r2;

  /* Check special conditions */
  if(unlikely(GR_A(r2 + 1, regs) % 8))
    ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);

  /* Return with cc 0 on zero length */
  if(unlikely(!GR_A(r2 + 1, regs)))
  {
    regs->psw.cc = 0;
    return;
  }

  /* Test writeability output chaining value */
  ARCH_DEP(validate_operand)(GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, 7, ACCTYPE_WRITE, regs);

  /* Fetch the parameter block */
  ARCH_DEP(vfetchc)(parameter_block, 31, GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, regs);

#ifdef OPTION_KMC_DEBUG
  LOGBYTE("icv   :", parameter_block, 8);
  LOGBYTE("k1    :", &parameter_block[8], 8);
  LOGBYTE("k2    :", &parameter_block[16], 8);
  LOGBYTE("k3    :", &parameter_block[24], 8);
#endif /* #ifdef OPTION_KMC_DEBUG */

  /* Set the cryptographic keys */
  des_set_key(&context1, &parameter_block[8]);
  des_set_key(&context2, &parameter_block[16]);
  des_set_key(&context3, &parameter_block[24]);

  /* Try to process the CPU-determined amount of data */
  r1_is_not_r2 = r1 != r2;
  for(crypted = 0; crypted < PROCESS_MAX; crypted += 8)
  {
    /* Fetch a block of data */
    ARCH_DEP(vfetchc)(message_block, 7, GR_A(r2, regs) & ADDRESS_MAXWRAP(regs), r2, regs);

#ifdef OPTION_KMC_DEBUG
    LOGBYTE("input :", message_block, 8);
#endif /* #ifdef OPTION_KMC_DEBUG */

    /* Do the job */
    des_encrypt(&context1, message_block, message_block);
    des_decrypt(&context2, message_block, message_block);
    des_encrypt(&context3, message_block, message_block);

    /* Save the temporary cv */
    memcpy(tcv, message_block, 8);

    /* XOR */
    for(i = 0; i < 8; i++)
      message_block[i] ^= parameter_block[i];

    des_encrypt(&context1, message_block, message_block);
    des_decrypt(&context2, message_block, message_block);
    des_encrypt(&context3, message_block, message_block);

    /* Store the output */
    ARCH_DEP(vstorec)(message_block, 7, GR_A(r1, regs) & ADDRESS_MAXWRAP(regs), r1, regs);

#ifdef OPTION_KMC_DEBUG
    LOGBYTE("output:", message_block, 8);
#endif /* #ifdef OPTION_KMC_DEBUG */

    /* XOR */
    for(i = 0; i < 8; i++)
      message_block[i] ^= tcv[i];

    des_encrypt(&context1, message_block, message_block);
    des_decrypt(&context2, message_block, message_block);
    des_encrypt(&context3, message_block, message_block);

    /* Save the ocv */
    memcpy(ocv, message_block, 8);

    /* Store the output chaining value */
    ARCH_DEP(vstorec)(ocv, 7, GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, regs);

#ifdef OPTION_KMC_DEBUG
    LOGBYTE("ocv   :", ocv, 8);
#endif /* #ifdef OPTION_KMC_DEBUG */

    /* Update the registers */
    SET_GR_A(r1, regs, GR_A(r1, regs) + 8);
    if(likely(r1_is_not_r2))
      SET_GR_A(r2, regs, GR_A(r2, regs) + 8);
    SET_GR_A(r2 + 1, regs, GR_A(r2 + 1, regs) - 8);

#ifdef OPTION_KMC_DEBUG
    WRMSG(HHC90108, "D", r1, (regs)->GR(r1));
    WRMSG(HHC90108, "D", r2, (regs)->GR(r2));
    WRMSG(HHC90108, "D", r2 + 1, (regs)->GR(r2 + 1));
#endif /* #ifdef OPTION_KMC_DEBUG */

    /* check for end of data */
    if(unlikely(!GR_A(r2 + 1, regs)))
    {
      regs->psw.cc = 0;
      return;
    }

    /* Set cv for next 8 bytes */
    memcpy(parameter_block, ocv, 8);
  }

  /* CPU-determined amount of data processed */
  regs->psw.cc = 3;
}
#endif /* defined( FEATURE_MSA_EXTENSION_FACILITY_1 ) */

#if defined( FEATURE_077_MSA_EXTENSION_FACILITY_4 )
/*----------------------------------------------------------------------------*/
/* Cipher message with counter (KMCTR) FC 1-3 and 9-11                        */
/*----------------------------------------------------------------------------*/
static void ARCH_DEP(kmctr_dea)(int r1, int r2, int r3, REGS *regs)
{
  des_context context1;
  des_context context2;
  des_context context3;
  BYTE countervalue_block[8];
  int crypted;
  int i;
  int keylen;
  BYTE message_block[8];
  BYTE parameter_block[48];
  int parameter_blocklen;
  int r1_is_not_r2;
  int r1_is_not_r3;
  int r2_is_not_r3;
  int tfc;
  int wrap;

  /* Check special conditions */
  if(unlikely(GR_A(r2 + 1, regs) % 8))
    ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);

  /* Return with cc 0 on zero length */
  if(unlikely(!GR_A(r2 + 1, regs)))
  {
    regs->psw.cc = 0;
    return;
  }

  /* Initialize values */
  tfc = GR0_tfc(regs);
  wrap = GR0_wrap(regs);
  keylen = tfc * 8;
  parameter_blocklen = keylen;
  if(wrap)
    parameter_blocklen += 24;

  /* Fetch the parameter block */
  ARCH_DEP(vfetchc)(parameter_block, parameter_blocklen - 1, GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, regs);

#ifdef OPTION_KMCTR_DEBUG
  switch(tfc)
  {
    case 1: /* dea */
    {
      LOGBYTE("k     :", parameter_block, 8);
      break;
    }
    case 2: /* tdea-128 */
    {
      LOGBYTE("k1    :", parameter_block, 8);
      LOGBYTE("k2    :", &parameter_block[8], 8);
      break;
    }
    case 3: /* tdea-192 */
    {
      LOGBYTE("k1    :", parameter_block, 8);
      LOGBYTE("k2    :", &parameter_block[8], 8);
      LOGBYTE("k3    :", &parameter_block[16], 8);
      break;
    }
  }
  if(wrap)
    LOGBYTE("wkvp  :", &parameter_block[parameter_blocklen - 24], 24);
#endif /* #ifdef OPTION_KMCTR_DEBUG */

  /* Verify and unwrap */
  if(wrap && unwrap_dea(parameter_block, keylen))
  {

#ifdef OPTION_KMCTR_DEBUG
    WRMSG(HHC90111, "D");
#endif /* #ifdef OPTION_KMCTR_DEBUG */

    regs->psw.cc = 1;
    return;
  }

  /* Set the cryptographic key */
  switch(tfc)
  {
    case 1: /* dea */
    {
      des_set_key(&context1, parameter_block);
      break;
    }
    case 2: /* tdea-128 */
    {
      des_set_key(&context1, parameter_block);
      des_set_key(&context2, &parameter_block[8]);
      break;
    }
    case 3: /* tdea-192 */
    {
      des_set_key(&context1, parameter_block);
      des_set_key(&context2, &parameter_block[8]);
      des_set_key(&context3, &parameter_block[16]);
      break;
    }
  }

  /* Try to process the CPU-determined amount of data */
  r1_is_not_r2 = r1 != r2;
  r1_is_not_r3 = r1 != r3;
  r2_is_not_r3 = r2 != r3;
  for(crypted = 0; crypted < PROCESS_MAX; crypted += 8)
  {
    /* Fetch a block of data and counter-value */
    ARCH_DEP(vfetchc)(message_block, 7, GR_A(r2, regs) & ADDRESS_MAXWRAP(regs), r2, regs);
    ARCH_DEP(vfetchc)(countervalue_block, 7, GR_A(r3, regs) & ADDRESS_MAXWRAP(regs), r3, regs);

#ifdef OPTION_KMCTR_DEBUG
    LOGBYTE("input :", message_block, 8);
    LOGBYTE("cv    :", countervalue_block, 8);
#endif /* #ifdef OPTION_KMCTR_DEBUG */

    /* Do the job */
    switch(tfc)
    {
      /* Encrypt */
      case 1: /* dea */
      {
        des_encrypt(&context1, countervalue_block, countervalue_block);
        break;
      }
      case 2: /* tdea-128 */
      {
        des_encrypt(&context1, countervalue_block, countervalue_block);
        des_decrypt(&context2, countervalue_block, countervalue_block);
        des_encrypt(&context1, countervalue_block, countervalue_block);
        break;
      }
      case 3: /* tdea-192 */
      {
        des_encrypt(&context1, countervalue_block, countervalue_block);
        des_decrypt(&context2, countervalue_block, countervalue_block);
        des_encrypt(&context3, countervalue_block, countervalue_block);
        break;
      }
    }

    /* XOR */
    for(i = 0; i < 8; i++)
      countervalue_block[i] ^= message_block[i];

    /* Store the output */
    ARCH_DEP(vstorec)(countervalue_block, 7, GR_A(r1, regs) & ADDRESS_MAXWRAP(regs), r1, regs);

#ifdef OPTION_KMCTR_DEBUG
    LOGBYTE("output:", countervalue_block, 8);
#endif /* #ifdef OPTION_KMCTR_DEBUG */

    /* Update the registers */
    SET_GR_A(r1, regs, GR_A(r1, regs) + 8);
    if(likely(r1_is_not_r2))
      SET_GR_A(r2, regs, GR_A(r2, regs) + 8);
    SET_GR_A(r2 + 1, regs, GR_A(r2 + 1, regs) - 8);
    if(likely(r1_is_not_r3 && r2_is_not_r3))
      SET_GR_A(r3, regs, GR_A(r3, regs) + 8);

#ifdef OPTION_KMCTR_DEBUG
    WRMSG(HHC90108, "D", r1, (regs)->GR(r1));
    WRMSG(HHC90108, "D", r2, (regs)->GR(r2));
    WRMSG(HHC90108, "D", r2 + 1, (regs)->GR(r2 + 1));
    WRMSG(HHC90108, "D", r3, (regs)->GR(r3));
#endif /* #ifdef OPTION_KMCTR_DEBUG */

    /* check for end of data */
    if(unlikely(!GR_A(r2 + 1, regs)))
    {
      regs->psw.cc = 0;
      return;
    }
  }

  /* CPU-determined amount of data processed */
  regs->psw.cc = 3;
}

/*----------------------------------------------------------------------------*/
/* Cipher message with counter (KMCTR) FC 18-20 and 26-28                     */
/*----------------------------------------------------------------------------*/
static void ARCH_DEP(kmctr_aes)(int r1, int r2, int r3, REGS *regs)
{
  rijndael_ctx context;
  BYTE countervalue_block[16];
  int crypted;
  int i;
  int keylen;
  BYTE message_block[16];
  BYTE parameter_block[64];
  int parameter_blocklen;
  int r1_is_not_r2;
  int r1_is_not_r3;
  int r2_is_not_r3;
  int fc;
  int tfc;
  int wrap;

  /* Check special conditions */
  if(unlikely(GR_A(r2 + 1, regs) % 16))
    ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);

  /* Return with cc 0 on zero length */
  if(unlikely(!GR_A(r2 + 1, regs)))
  {
    regs->psw.cc = 0;
    return;
  }

  /* Initialize values */
  fc = GR0_fc(regs);
  tfc = GR0_tfc(regs);
  wrap = kmctr_wrap[fc];
  keylen = kmctr_keylengths[fc];
  parameter_blocklen = kmctr_pblens[fc];
#ifdef OPTION_KMCTR_DEBUG
  logmsg("Feature code %d wrap %d keylen %d pblen %d\n",
   tfc, wrap, keylen, parameter_blocklen);
#endif /* #ifdef OPTION_KMCTR_DEBUG */

  /* Fetch the parameter block */
  ARCH_DEP(vfetchc)(parameter_block, parameter_blocklen - 1, GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, regs);

#ifdef OPTION_KMCTR_DEBUG
  LOGBYTE("k     :", parameter_block, keylen);
  if(wrap)
    LOGBYTE("wkvp  :", &parameter_block[parameter_blocklen - 32], 32);
#endif /* #ifdef OPTION_KMCTR_DEBUG */

  if(wrap && unwrap_aes(parameter_block, keylen))
  {

#ifdef OPTION_KMCTR_DEBUG
    WRMSG(HHC90111, "D");
#endif /* #ifdef OPTION_KMCTR_DEBUG */

    regs->psw.cc = 1;
    return;
  }

  /* Set the cryptographic key */
  rijndael_set_key(&context, parameter_block, keylen * 8);

  /* Try to process the CPU-determined amount of data */
  r1_is_not_r2 = r1 != r2;
  r1_is_not_r3 = r1 != r3;
  /* The original assignment (kept below as a comment) compared R1 with R2 */
  /* a second time instead of R2 with R3. With R1 = R2 (in-place operation */
  /* using the same register) the third operand address in R3 was never    */
  /* advanced, so every block was processed with the first counter block   */
  /* and the output was wrong from the second block on. With R2 = R3 that  */
  /* register was advanced twice per block. kmctr_dea() was always right.  */
  // r2_is_not_r3 = r1 != r2;
  r2_is_not_r3 = r2 != r3;
  for(crypted = 0; crypted < PROCESS_MAX; crypted += 16)
  {
    /* Fetch a block of data and counter-value */
    ARCH_DEP(vfetchc)(message_block, 15, GR_A(r2, regs) & ADDRESS_MAXWRAP(regs), r2, regs);
    ARCH_DEP(vfetchc)(countervalue_block, 15, GR_A(r3, regs) & ADDRESS_MAXWRAP(regs), r3, regs);

#ifdef OPTION_KMCTR_DEBUG
    LOGBYTE("input :", message_block, 16);
    LOGBYTE("cv    :", countervalue_block, 16);
#endif /* #ifdef OPTION_KMCTR_DEBUG */

    /* Do the job */
    /* Encrypt and XOR */
    rijndael_encrypt(&context, countervalue_block, countervalue_block);
    for(i = 0; i < 16; i++)
      countervalue_block[i] ^= message_block[i];

    /* Store the output */
    ARCH_DEP(vstorec)(countervalue_block, 15, GR_A(r1, regs) & ADDRESS_MAXWRAP(regs), r1, regs);

#ifdef OPTION_KMCTR_DEBUG
    LOGBYTE("output:", countervalue_block, 16);
#endif /* #ifdef OPTION_KMCTR_DEBUG */

    /* Update the registers */
    SET_GR_A(r1, regs, GR_A(r1, regs) + 16);
    if(likely(r1_is_not_r2))
      SET_GR_A(r2, regs, GR_A(r2, regs) + 16);
    SET_GR_A(r2 + 1, regs, GR_A(r2 + 1, regs) - 16);
    if(likely(r1_is_not_r3 && r2_is_not_r3))
      SET_GR_A(r3, regs, GR_A(r3, regs) + 16);

#ifdef OPTION_KMCTR_DEBUG
    WRMSG(HHC90108, "D", r1, (regs)->GR(r1));
    WRMSG(HHC90108, "D", r2, (regs)->GR(r2));
    WRMSG(HHC90108, "D", r2 + 1, (regs)->GR(r2 + 1));
    WRMSG(HHC90108, "D", r3, (regs)->GR(r3));
#endif /* #ifdef OPTION_KMCTR_DEBUG */

    /* check for end of data */
    if(unlikely(!GR_A(r2 + 1, regs)))
    {
      regs->psw.cc = 0;
      return;
    }
  }

  /* CPU-determined amount of data processed */
  regs->psw.cc = 3;
}

/*----------------------------------------------------------------------------*/
/* Cipher message with cipher feedback (KMF) FC 1-3 and 9-11                  */
/*----------------------------------------------------------------------------*/
static void ARCH_DEP(kmf_dea)(int r1, int r2, REGS *regs)
{
  des_context context1;
  des_context context2;
  des_context context3;
  int crypted;
  int i;
  int keylen;
  int lcfb;
  BYTE message_block[8];
  int modifier_bit;
  BYTE output_block[8];
  BYTE parameter_block[56];
  int parameter_blocklen;
  int r1_is_not_r2;
  int tfc;
  int wrap;

  /* Initialize values */
  lcfb = GR0_lcfb(regs);

  /* Check special conditions */
  if(unlikely(!lcfb || lcfb > 8 || GR_A(r2 + 1, regs) % lcfb))
    ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);

  /* Return with cc 0 on zero length */
  if(unlikely(!GR_A(r2 + 1, regs)))
  {
    regs->psw.cc = 0;
    return;
  }

  /* Initialize values */
  tfc = GR0_tfc(regs);
  wrap = GR0_wrap(regs);
  keylen = tfc * 8;
  parameter_blocklen = keylen + 8;
  if(wrap)
    parameter_blocklen += 24;

  /* Test writeability output chaining value */
  ARCH_DEP(validate_operand)(GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, 7, ACCTYPE_WRITE, regs);

  /* Fetch the parameter block */
  ARCH_DEP(vfetchc)(parameter_block, parameter_blocklen - 1, GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, regs);

#ifdef OPTION_KMF_DEBUG
  LOGBYTE("cv    :", parameter_block, 8);
  switch(tfc)
  {
    case 1: /* dea */
    {
      LOGBYTE("k     :", &parameter_block[8], 8);
      break;
    }
    case 2: /* tdea-128 */
    {
      LOGBYTE("k1    :", &parameter_block[8], 8);
      LOGBYTE("k2    :", &parameter_block[16], 8);
      break;
    }
    case 3: /* tdea-192 */
    {
      LOGBYTE("k1    :", &parameter_block[8], 8);
      LOGBYTE("k2    :", &parameter_block[16], 8);
      LOGBYTE("k3    :", &parameter_block[24], 8);
      break;
    }
  }
  if(wrap)
    LOGBYTE("wkvp  :", &parameter_block[parameter_blocklen - 24], 24);
#endif /* #ifdef OPTION_KMF_DEBUG */

  /* Verify and unwrap */
  if(wrap && unwrap_dea(&parameter_block[8], keylen))
  {

#ifdef OPTION_KMF_DEBUG
    WRMSG(HHC90111, "D");
#endif /* #ifdef OPTION_KMF_DEBUG */

    regs->psw.cc = 1;
    return;
  }

  /* Set the cryptographic key */
  switch(tfc)
  {
    case 1: /* dea */
    {
      des_set_key(&context1, &parameter_block[8]);
      break;
    }
    case 2: /* tdea-128 */
    {
      des_set_key(&context1, &parameter_block[8]);
      des_set_key(&context2, &parameter_block[16]);
      break;
    }
    case 3: /* tdea-192 */
    {
      des_set_key(&context1, &parameter_block[8]);
      des_set_key(&context2, &parameter_block[16]);
      des_set_key(&context3, &parameter_block[24]);
      break;
    }
  }

  /* Try to process the CPU-determined amount of data */
  modifier_bit = GR0_m(regs);
  r1_is_not_r2 = r1 != r2;
  for(crypted = 0; crypted < PROCESS_MAX; crypted += lcfb)
  {
    /* Do the job */
    switch(tfc)
    {
      case 1: /* dea */
      {
        des_encrypt(&context1, parameter_block, output_block);
        break;
      }
      case 2: /* tdea-128 */
      {
        des_encrypt(&context1, parameter_block, output_block);
        des_decrypt(&context2, output_block, output_block);
        des_encrypt(&context1, output_block, output_block);
        break;
      }
      case 3: /* tdea-192 */
      {
        des_encrypt(&context1, parameter_block, output_block);
        des_decrypt(&context2, output_block, output_block);
        des_encrypt(&context3, output_block, output_block);
        break;
      }
    }
    ARCH_DEP(vfetchc)(message_block, lcfb - 1, GR_A(r2, regs) & ADDRESS_MAXWRAP(regs), r2, regs);

#ifdef OPTION_KMF_DEBUG
    LOGBYTE("input :", message_block, lcfb);
#endif /* #ifdef OPTION_KMF_DEBUG */

    for(i = 0; i < lcfb; i++)
      output_block[i] ^= message_block[i];
    for(i = 0; i < 8 - lcfb; i++)
       parameter_block[i] = parameter_block[i + lcfb];
    if(modifier_bit)
    {
      /* Decipher */
      for(i = 0; i < lcfb; i++)
        parameter_block[i + 8 - lcfb] = message_block[i];
    }
    else
    {
      /* Encipher */
      for(i = 0; i < lcfb; i++)
        parameter_block[i + 8 - lcfb] = output_block[i];
    }

    /* Store the output */
    ARCH_DEP(vstorec)(output_block, lcfb - 1, GR_A(r1, regs) & ADDRESS_MAXWRAP(regs), r1, regs);

#ifdef OPTION_KMF_DEBUG
    LOGBYTE("output:", output_block, lcfb);
#endif /* #ifdef OPTION_KMF_DEBUG */

    /* Store the chaining value */
    ARCH_DEP(vstorec)(parameter_block, 7, GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, regs);

#ifdef OPTION_KMF_DEBUG
    LOGBYTE("cv    :", parameter_block, 8);
#endif /* #ifdef OPTION_KMF_DEBUG */

    /* Update the registers */
    SET_GR_A(r1, regs, GR_A(r1, regs) + lcfb);
    if(likely(r1_is_not_r2))
      SET_GR_A(r2, regs, GR_A(r2, regs) + lcfb);
    SET_GR_A(r2 + 1, regs, GR_A(r2 + 1, regs) - lcfb);

#ifdef OPTION_KMF_DEBUG
    WRMSG(HHC90108, "D", r1, (regs)->GR(r1));
    WRMSG(HHC90108, "D", r2, (regs)->GR(r2));
    WRMSG(HHC90108, "D", r2 + 1, (regs)->GR(r2 + 1));
#endif /* #ifdef OPTION_KMF_DEBUG */

    /* check for end of data */
    if(unlikely(!GR_A(r2 + 1, regs)))
    {
      regs->psw.cc = 0;
      return;
    }
  }

  /* CPU-determined amount of data processed */
  regs->psw.cc = 3;
}

/*----------------------------------------------------------------------------*/
/* Cipher message with cipher feedback (KMF) FC 18-20 and 26-28               */
/*----------------------------------------------------------------------------*/
static void ARCH_DEP(kmf_aes)(int r1, int r2, REGS *regs)
{
  rijndael_ctx context;
  int crypted;
  int i;
  int keylen;
  int lcfb;
  BYTE message_block[16];
  int modifier_bit;
  BYTE output_block[16];
  BYTE parameter_block[80];
  int parameter_blocklen;
  int r1_is_not_r2;
  int tfc;
  int wrap;

  /* Initialize values */
  lcfb = GR0_lcfb(regs);

  /* Check special conditions */
  if(unlikely(GR_A(r2 + 1, regs) % lcfb || !lcfb || lcfb > 16))
    ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);

  /* Return with cc 0 on zero length */
  if(unlikely(!GR_A(r2 + 1, regs)))
  {
    regs->psw.cc = 0;
    return;
  }

  /* Initialize values */
  tfc = GR0_tfc(regs);
  wrap = GR0_wrap(regs);
  keylen = (tfc - 17) * 8 + 8;
  parameter_blocklen = keylen + 16;
  if(wrap)
    parameter_blocklen += 32;

  /* Test writeability output chaining value */
  ARCH_DEP(validate_operand)(GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, 15, ACCTYPE_WRITE, regs);

  /* Fetch the parameter block */
  ARCH_DEP(vfetchc)(parameter_block, parameter_blocklen - 1, GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, regs);

#ifdef OPTION_KMF_DEBUG
  LOGBYTE("cv    :", parameter_block, 16);
  LOGBYTE("k     :", &parameter_block[16], keylen);
  if(wrap)
    LOGBYTE("wkvp  :", &parameter_block[parameter_blocklen - 32], 32);
#endif /* #ifdef OPTION_KMF_DEBUG */

  /* Verify and unwrap */
  if(wrap && unwrap_aes(&parameter_block[16], keylen))
  {

#ifdef OPTION_KMF_DEBUG
    WRMSG(HHC90111, "D");
#endif /* #ifdef OPTION_KMF_DEBUG */

    regs->psw.cc = 1;
    return;
  }

  /* Set the cryptographic key */
  rijndael_set_key(&context, &parameter_block[16], keylen * 8);

  /* Try to process the CPU-determined amount of data */
  modifier_bit = GR0_m(regs);
  r1_is_not_r2 = r1 != r2;
  for(crypted = 0; crypted < PROCESS_MAX; crypted += lcfb)
  {
    rijndael_encrypt(&context, parameter_block, output_block);
    ARCH_DEP(vfetchc)(message_block, lcfb - 1, GR_A(r2, regs) & ADDRESS_MAXWRAP(regs), r2, regs);

#ifdef OPTION_KMF_DEBUG
    LOGBYTE("input :", message_block, lcfb);
#endif /* #ifdef OPTION_KMF_DEBUG */

    for(i = 0; i < lcfb; i++)
      output_block[i] ^= message_block[i];
    for(i = 0; i < 16 - lcfb; i++)
      parameter_block[i] = parameter_block[i + lcfb];
    if(modifier_bit)
    {
      /* Decipher */
      for(i = 0; i < lcfb; i++)
        parameter_block[i + 16 - lcfb] = message_block[i];
    }
    else
    {
      /* Encipher */
      for(i = 0; i < lcfb; i++)
        parameter_block[i + 16 - lcfb] = output_block[i];
    }

    /* Store the output */
    ARCH_DEP(vstorec)(output_block, lcfb - 1, GR_A(r1, regs) & ADDRESS_MAXWRAP(regs), r1, regs);

#ifdef OPTION_KMF_DEBUG
    LOGBYTE("output:", output_block, lcfb);
#endif /* #ifdef OPTION_KMF_DEBUG */

    /* Store the chaining value */
    ARCH_DEP(vstorec)(parameter_block, 15, GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, regs);

#ifdef OPTION_KMF_DEBUG
    LOGBYTE("cv    :", parameter_block, 16);
#endif /* #ifdef OPTION_KMF_DEBUG */

    /* Update the registers */
    SET_GR_A(r1, regs, GR_A(r1, regs) + lcfb);
    if(likely(r1_is_not_r2))
      SET_GR_A(r2, regs, GR_A(r2, regs) + lcfb);
    SET_GR_A(r2 + 1, regs, GR_A(r2 + 1, regs) - lcfb);

#ifdef OPTION_KMF_DEBUG
    WRMSG(HHC90108, "D", r1, (regs)->GR(r1));
    WRMSG(HHC90108, "D", r2, (regs)->GR(r2));
    WRMSG(HHC90108, "D", r2 + 1, (regs)->GR(r2 + 1));
#endif /* #ifdef OPTION_KMF_DEBUG */

    /* check for end of data */
    if(unlikely(!GR_A(r2 + 1, regs)))
    {
      regs->psw.cc = 0;
      return;
    }
  }

  /* CPU-determined amount of data processed */
  regs->psw.cc = 3;
}

/*----------------------------------------------------------------------------*/
/* Cipher message with output feedback (KMO) FC 1-3 and 9-11                  */
/*----------------------------------------------------------------------------*/
static void ARCH_DEP(kmo_dea)(int r1, int r2, REGS *regs)
{
  des_context context1;
  des_context context2;
  des_context context3;
  int crypted;
  int i;
  int keylen;
  BYTE message_block[8];
  BYTE parameter_block[56];
  int parameter_blocklen;
  int r1_is_not_r2;
  int tfc;
  int wrap;

  /* Check special conditions */
  if(unlikely(GR_A(r2 + 1, regs) % 8))
    ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);

  /* Return with cc 0 on zero length */
  if(unlikely(!GR_A(r2 + 1, regs)))
  {
    regs->psw.cc = 0;
    return;
  }

  /* Initialize values */
  tfc = GR0_tfc(regs);
  wrap = GR0_wrap(regs);
  keylen = tfc * 8;
  parameter_blocklen = keylen + 8;
  if(wrap)
    parameter_blocklen += 24;

  /* Test writeability output chaining value */
  ARCH_DEP(validate_operand)(GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, 7, ACCTYPE_WRITE, regs);

  /* Fetch the parameter block */
  ARCH_DEP(vfetchc)(parameter_block, parameter_blocklen - 1, GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, regs);

#ifdef OPTION_KMO_DEBUG
  LOGBYTE("cv    :", parameter_block, 8);
  switch(tfc)
  {
    case 1: /* dea */
    {
      LOGBYTE("k     :", &parameter_block[8], 8);
      break;
    }
    case 2: /* tdea-128 */
    {
      LOGBYTE("k1    :", &parameter_block[8], 8);
      LOGBYTE("k2    :", &parameter_block[16], 8);
      break;
    }
    case 3: /* tdea-192 */
    {
      LOGBYTE("k1    :", &parameter_block[8], 8);
      LOGBYTE("k2    :", &parameter_block[16], 8);
      LOGBYTE("k3    :", &parameter_block[24], 8);
      break;
    }
  }
  if(wrap)
    LOGBYTE("wkvp  :", &parameter_block[keylen + 8], 24);
#endif /* #ifdef OPTION_KMO_DEBUG */

  /* Verify and unwrap */
  if(wrap && unwrap_dea(&parameter_block[8], keylen))
  {

#ifdef OPTION_KMO_DEBUG
    WRMSG(HHC90111, "D");
#endif /* #ifdef OPTION_KMO_DEBUG */

    regs->psw.cc = 1;
    return;
  }

  /* Set the cryptographic key */
  switch(tfc)
  {
    case 1: /* dea */
    {
      des_set_key(&context1, &parameter_block[8]);
      break;
    }
    case 2: /* tdea-128 */
    {
      des_set_key(&context1, &parameter_block[8]);
      des_set_key(&context2, &parameter_block[16]);
      break;
    }
    case 3: /* tdea-192 */
    {
      des_set_key(&context1, &parameter_block[8]);
      des_set_key(&context2, &parameter_block[16]);
      des_set_key(&context3, &parameter_block[24]);
      break;
    }
  }

  /* Try to process the CPU-determined amount of data */
  r1_is_not_r2 = r1 != r2;
  for(crypted = 0; crypted < PROCESS_MAX; crypted += 8)
  {
    /* Do the job */
    switch(tfc)
    {
      case 1: /* dea */
      {
        des_encrypt(&context1, parameter_block, parameter_block);
        break;
      }
      case 2: /* tdea-128 */
      {
        des_encrypt(&context1, parameter_block, parameter_block);
        des_decrypt(&context2, parameter_block, parameter_block);
        des_encrypt(&context1, parameter_block, parameter_block);
        break;
      }
      case 3: /* tdea-192 */
      {
        des_encrypt(&context1, parameter_block, parameter_block);
        des_decrypt(&context2, parameter_block, parameter_block);
        des_encrypt(&context3, parameter_block, parameter_block);
        break;
      }
    }
    ARCH_DEP(vfetchc)(message_block, 7, GR_A(r2, regs) & ADDRESS_MAXWRAP(regs), r2, regs);

#ifdef OPTION_KMO_DEBUG
    LOGBYTE("input :", message_block, 8);
#endif /* #ifdef OPTION_KMO_DEBUG */

    for(i = 0; i < 8; i++)
      message_block[i] ^= parameter_block[i];

    /* Store the output */
    ARCH_DEP(vstorec)(message_block, 7, GR_A(r1, regs) & ADDRESS_MAXWRAP(regs), r1, regs);

#ifdef OPTION_KMO_DEBUG
    LOGBYTE("output:", message_block, 8);
#endif /* #ifdef OPTION_KMO_DEBUG */

    /* Store the chaining value */
    ARCH_DEP(vstorec)(parameter_block, 7, GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, regs);

#ifdef OPTION_KMO_DEBUG
    LOGBYTE("cv    :", parameter_block, 8);
#endif /* #ifdef OPTION_KMO_DEBUG */

    /* Update the registers */
    SET_GR_A(r1, regs, GR_A(r1, regs) + 8);
    if(likely(r1_is_not_r2))
      SET_GR_A(r2, regs, GR_A(r2, regs) + 8);
    SET_GR_A(r2 + 1, regs, GR_A(r2 + 1, regs) - 8);

#ifdef OPTION_KMO_DEBUG
    WRMSG(HHC90108, "D", r1, (regs)->GR(r1));
    WRMSG(HHC90108, "D", r2, (regs)->GR(r2));
    WRMSG(HHC90108, "D", r2 + 1, (regs)->GR(r2 + 1));
#endif /* #ifdef OPTION_KMO_DEBUG */

    /* check for end of data */
    if(unlikely(!GR_A(r2 + 1, regs)))
    {
      regs->psw.cc = 0;
      return;
    }
  }

  /* CPU-determined amount of data processed */
  regs->psw.cc = 3;
}

/*----------------------------------------------------------------------------*/
/* Cipher message with output feedback (KMO) FC 18-20 and 26-28               */
/*----------------------------------------------------------------------------*/
static void ARCH_DEP(kmo_aes)(int r1, int r2, REGS *regs)
{
  rijndael_ctx context;
  int crypted;
  int i;
  int keylen;
  BYTE message_block[16];
  BYTE parameter_block[80];
  int parameter_blocklen;
  int r1_is_not_r2;
  int tfc;
  int wrap;

  /* Check special conditions */
  if(unlikely(GR_A(r2 + 1, regs) % 16))
    ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);

  /* Return with cc 0 on zero length */
  if(unlikely(!GR_A(r2 + 1, regs)))
  {
    regs->psw.cc = 0;
    return;
  }

  /* Initialize values */
  tfc = GR0_tfc(regs);
  wrap = GR0_wrap(regs);
  keylen = (tfc - 17) * 8 + 8;
  parameter_blocklen = keylen + 16;
  if(wrap)
    parameter_blocklen += 32;

  /* Test writeability output chaining value */
  ARCH_DEP(validate_operand)(GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, 15, ACCTYPE_WRITE, regs);

  /* Fetch the parameter block */
  ARCH_DEP(vfetchc)(parameter_block, parameter_blocklen - 1, GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, regs);

#ifdef OPTION_KMO_DEBUG
  LOGBYTE("cv    :", parameter_block, 16);
  LOGBYTE("k     :", &parameter_block[16], keylen);
  if(wrap)
    LOGBYTE("wkvp  :", &parameter_block[keylen + 16], 32);
#endif /* #ifdef OPTION_KMO_DEBUG */

  /* Verify and unwrap */
  if(wrap && unwrap_aes(&parameter_block[16], keylen))
  {

#ifdef OPTION_KMO_DEBUG
    WRMSG(HHC90111, "D");
#endif /* #ifdef OPTION_KMO_DEBUG */

    regs->psw.cc = 1;
    return;
  }

  /* Set the cryptographic key */
  rijndael_set_key(&context, &parameter_block[16], keylen * 8);

  /* Try to process the CPU-determined amount of data */
  r1_is_not_r2 = r1 != r2;
  for(crypted = 0; crypted < PROCESS_MAX; crypted += 16)
  {
    rijndael_encrypt(&context, parameter_block, parameter_block);
    ARCH_DEP(vfetchc)(message_block, 15, GR_A(r2, regs) & ADDRESS_MAXWRAP(regs), r2, regs);

#ifdef OPTION_KMO_DEBUG
    LOGBYTE("input :", message_block, 16);
#endif /* #ifdef OPTION_KMO_DEBUG */

    for(i = 0; i < 16; i++)
      message_block[i] ^= parameter_block[i];

    /* Store the output */
    ARCH_DEP(vstorec)(message_block, 15, GR_A(r1, regs) & ADDRESS_MAXWRAP(regs), r1, regs);

#ifdef OPTION_KMO_DEBUG
    LOGBYTE("output:", message_block, 16);
#endif /* #ifdef OPTION_KMO_DEBUG */

    /* Store the chaining value */
    ARCH_DEP(vstorec)(parameter_block, 15, GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, regs);

#ifdef OPTION_KMO_DEBUG
    LOGBYTE("cv    :", parameter_block, 16);
#endif /* #ifdef OPTION_KMO_DEBUG */

    /* Update the registers */
    SET_GR_A(r1, regs, GR_A(r1, regs) + 16);
    if(likely(r1_is_not_r2))
      SET_GR_A(r2, regs, GR_A(r2, regs) + 16);
    SET_GR_A(r2 + 1, regs, GR_A(r2 + 1, regs) - 16);

#ifdef OPTION_KMO_DEBUG
    WRMSG(HHC90108, "D", r1, (regs)->GR(r1));
    WRMSG(HHC90108, "D", r2, (regs)->GR(r2));
    WRMSG(HHC90108, "D", r2 + 1, (regs)->GR(r2 + 1));
#endif /* #ifdef OPTION_KMO_DEBUG */

    /* check for end of data */
    if(unlikely(!GR_A(r2 + 1, regs)))
    {
      regs->psw.cc = 0;
      return;
    }
  }

  /* CPU-determined amount of data processed */
  regs->psw.cc = 3;
}

/*----------------------------------------------------------------------------*/
/* Perform cryptographic computation (PCC) FC 1-3 and 9-11                    */
/*----------------------------------------------------------------------------*/
static void ARCH_DEP(pcc_cmac_dea)(REGS *regs)
{
  des_context context1;
  des_context context2;
  des_context context3;
  int i;
  BYTE k[8];
  int keylen;
  BYTE mask[8] = { 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff };
  BYTE parameter_block[72];
  int parameter_blocklen;
  BYTE r64[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1b };
  int tfc;
  int wrap;

  /* Check special conditions */
  if(unlikely(GR0_m(regs)))
    ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);

  /* Initialize values */
  tfc = GR0_tfc(regs);
  wrap = GR0_wrap(regs);
  keylen = tfc * 8;
  parameter_blocklen = keylen + 24;
  if(wrap)
    parameter_blocklen += 24;

  /* Test writeability output chaining value */
  ARCH_DEP(validate_operand)((GR_A(1, regs) + 16) & ADDRESS_MAXWRAP(regs), 1, 7, ACCTYPE_WRITE, regs);

  /* Fetch the parameter block */
  ARCH_DEP(vfetchc)(parameter_block, parameter_blocklen - 1, GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, regs);

#ifdef OPTION_PCC_DEBUG
  LOGBYTE("ml    :", parameter_block, 1);
  LOGBYTE("msg   :", &parameter_block[8], 8);
  LOGBYTE("icv   :", &parameter_block[16], 8);
  switch(tfc)
  {
    case 1: /* dea */
    {
      LOGBYTE("k     :", &parameter_block[24], 8);
      break;
    }
    case 2: /* tdea-128 */
    {
      LOGBYTE("k1    :", &parameter_block[24], 8);
      LOGBYTE("k2    :", &parameter_block[32], 8);
      break;
    }
    case 3: /* tdea-192 */
    {
      LOGBYTE("k1    :", &parameter_block[24], 8);
      LOGBYTE("k2    :", &parameter_block[32], 8);
      LOGBYTE("k3    :", &parameter_block[40], 8);
      break;
    }
  }
  if(wrap)
    LOGBYTE("wkvp  :", &parameter_block[keylen + 24], 24);
#endif /* #ifdef OPTION_PCC_DEBUG */

  /* Verify and unwrap */
  if(wrap && unwrap_dea(&parameter_block[24], keylen))
  {

#ifdef OPTION_PCC_DEBUG
    WRMSG(HHC90111, "D");
#endif /* #ifdef OPTION_PCC_DEBUG */

    regs->psw.cc = 1;
    return;
  }

  /* Set the cryptographic key */
  switch(tfc)
  {
    case 1: /* dea */
    {
      des_set_key(&context1, &parameter_block[24]);
      break;
    }
    case 2: /* tdea-128 */
    {
      des_set_key(&context1, &parameter_block[24]);
      des_set_key(&context2, &parameter_block[32]);
      break;
    }
    case 3: /* tdea-192 */
    {
      des_set_key(&context1, &parameter_block[24]);
      des_set_key(&context2, &parameter_block[32]);
      des_set_key(&context3, &parameter_block[40]);
      break;
    }
  }

  /* Check validity ML value */
  if(parameter_block[0] > 64)
  {
    regs->psw.cc = 2;
    return;
  }

  /* Place the one bit */
  if(parameter_block[0] != 64)
    parameter_block[(parameter_block[0] / 8) + 8] |= (0x80 >> (parameter_block[0] % 8));

  /* Pad with zeroes */
  if(parameter_block[0] < 63)
  {
    parameter_block[(parameter_block[0] / 8) + 8] &= mask[parameter_block[0] % 8];
    for(i = (parameter_block[0] / 8) + 1; i < 8; i++)
      parameter_block[i + 8] = 0x00;
  }

#ifdef OPTION_PCC_DEBUG
  LOGBYTE("msg   :", &parameter_block[8], 8);
#endif /* #ifdef OPTION_PCC_DEBUG */

  /* Calculate subkey */
  zeromem(k, 8);
  switch(tfc)
  {
    case 1: /* dea */
    {
      des_encrypt(&context1, k, k);
      break;
    }
    case 2: /* tdea-128 */
    {
      des_encrypt(&context1, k, k);
      des_decrypt(&context2, k, k);
      des_encrypt(&context1, k, k);
      break;
    }
    case 3: /* tdea-192 */
    {
      des_encrypt(&context1, k, k);
      des_decrypt(&context2, k, k);
      des_encrypt(&context3, k, k);
      break;
    }
  }

  /* Calculate subkeys Kx and Ky */
  if(!(k[0] & 0x80))
    shift_left(k, k, 8);
  else
  {
    shift_left(k, k, 8);
    for(i = 0; i < 8; i++)
      k[i] ^= r64[i];
  }
  if(parameter_block[0] != 64)
  {
    if(!(k[0] & 0x80))
      shift_left(k, k, 8);
    else
    {
      shift_left(k, k, 8);
      for(i = 0; i < 8; i++)
        k[i] ^= r64[i];
    }
  }

#ifdef OPTION_PCC_DEBUG
  LOGBYTE("Subkey:", k, 8);
#endif /* #ifdef OPTION_PCC_DEBUG */

  /* XOR with kx or ky and encrypt */
  for(i = 0; i < 8; i++)
  {
    parameter_block[i + 8] ^= k[i];
    parameter_block[i + 8] ^= parameter_block[i + 16];
  }
  switch(tfc)
  {
    case 1: /* dea */
    {
      des_encrypt(&context1, &parameter_block[8], &parameter_block[8]);
      break;
    }
    case 2: /* tdea-128 */
    {
      des_encrypt(&context1, &parameter_block[8], &parameter_block[8]);
      des_decrypt(&context2, &parameter_block[8], &parameter_block[8]);
      des_encrypt(&context1, &parameter_block[8], &parameter_block[8]);
      break;
    }
    case 3: /* tdea-192 */
    {
      des_encrypt(&context1, &parameter_block[8], &parameter_block[8]);
      des_decrypt(&context2, &parameter_block[8], &parameter_block[8]);
      des_encrypt(&context3, &parameter_block[8], &parameter_block[8]);
      break;
    }
  }

#ifdef OPTION_PCC_DEBUG
  LOGBYTE("cmac  :", &parameter_block[8], 8);
#endif /* #ifdef OPTION_PCC_DEBUG */

  /* Store the CMAC */
  ARCH_DEP(vstorec)(&parameter_block[8], 7, (GR_A(1, regs) + 16) & ADDRESS_MAXWRAP(regs), 1, regs);

  /* Normal completion */
  regs->psw.cc = 0;
}

/*----------------------------------------------------------------------------*/
/* Perform cryptographic computation (PCC) FC 18-20 and 26-28                 */
/*----------------------------------------------------------------------------*/
static void ARCH_DEP(pcc_cmac_aes)(REGS *regs)
{
  rijndael_ctx context;
  int i;
  BYTE k[16];
  int keylen;
  BYTE mask[8] = { 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff };
  BYTE parameter_block[104];
  int parameter_blocklen;
  BYTE r128[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x87 };
  int tfc;
  int wrap;

  /* Check special conditions */
  if(unlikely(GR0_m(regs)))
    ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);

  /* Initialize values */
  tfc = GR0_tfc(regs);
  wrap = GR0_wrap(regs);
  keylen = (tfc - 17) * 8 + 8;
  parameter_blocklen = keylen + 40;
  if(wrap)
    parameter_blocklen += 32;

  /* Test writeability output chaining value */
  ARCH_DEP(validate_operand)((GR_A(1, regs) + 24) & ADDRESS_MAXWRAP(regs), 1, 15, ACCTYPE_WRITE, regs);

  /* Fetch the parameter block */
  ARCH_DEP(vfetchc)(parameter_block, parameter_blocklen - 1, GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, regs);

#ifdef OPTION_PCC_DEBUG
  LOGBYTE("ml    :", parameter_block, 1);
  LOGBYTE("msg   :", &parameter_block[8], 16);
  LOGBYTE("icv   :", &parameter_block[24], 16);
  LOGBYTE("k     :", &parameter_block[40], keylen);
  if(wrap)
    LOGBYTE("wkvp  :", &parameter_block[keylen + 40], 32);
#endif /* #ifdef OPTION_PCC_DEBUG */

  /* Verify and unwrap */
  if(wrap && unwrap_aes(&parameter_block[40], keylen))
  {

#ifdef OPTION_PCC_DEBUG
    WRMSG(HHC90111, "D");
#endif /* #ifdef OPTION_PCC_DEBUG */

    regs->psw.cc = 1;
    return;
  }

  /* Set the cryptographic key */
  rijndael_set_key(&context, &parameter_block[40], keylen * 8);

  /* Check validity ML value */
  if(parameter_block[0] > 128)
  {
    regs->psw.cc = 2;
    return;
  }

  /* Place the one bit */
  if(parameter_block[0] != 128)
    parameter_block[(parameter_block[0] / 8) + 8] |= (0x80 >> (parameter_block[0] % 8));

  /* Pad with zeroes */
  if(parameter_block[0] < 127)
  {
    parameter_block[(parameter_block[0] / 8) + 8] &= mask[parameter_block[0] % 8];
    for(i = (parameter_block[0] / 8) + 1; i < 16; i++)
      parameter_block[i + 8] = 0x00;
  }

#ifdef OPTION_PCC_DEBUG
  LOGBYTE("msg   :", &parameter_block[8], 16);
#endif /* #ifdef OPTION_PCC_DEBUG */

  /* Calculate subkeys */
  zeromem(k, 16);
  rijndael_encrypt(&context, k, k);

  /* Calculate subkeys Kx and Ky */
  if(!(k[0] & 0x80))
    shift_left(k, k, 16);
  else
  {
    shift_left(k, k, 16);
    for(i = 0; i < 16; i++)
      k[i] ^= r128[i];
  }
  if(parameter_block[0] != 128)
  {
    if(!(k[0] & 0x80))
      shift_left(k, k, 16);
    else
    {
      shift_left(k, k, 16);
      for(i = 0; i < 16; i++)
        k[i] ^= r128[i];
    }
  }

#ifdef OPTION_PCC_DEBUG
  LOGBYTE("Subkey:", k, 16);
#endif /* #ifdef OPTION_PCC_DEBUG */

  /* XOR with kx or ky and encrypt */
  for(i = 0; i < 16; i++)
  {
    parameter_block[i + 8] ^= k[i];
    parameter_block[i + 8] ^= parameter_block[i + 24];
  }
  rijndael_encrypt(&context, &parameter_block[8], &parameter_block[8]);

#ifdef OPTION_PCC_DEBUG
  LOGBYTE("cmac  :", &parameter_block[8], 16);
#endif /* #ifdef OPTION_PCC_DEBUG */

  /* Store the CMAC */
  ARCH_DEP(vstorec)(&parameter_block[8], 15, (GR_A(1, regs) + 24) & ADDRESS_MAXWRAP(regs), 1, regs);

  /* Normal completion */
  regs->psw.cc = 0;
}

/*----------------------------------------------------------------------------*/
/* Perform cryptographic computation (PCC) FC 50, 52, 58 and 60               */
/*----------------------------------------------------------------------------*/
static void ARCH_DEP(pcc_xts_aes)(REGS *regs)
{
  BYTE *bsn;
  rijndael_ctx context;
  BYTE *ibi;
  int keylen;
  BYTE mask[8] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };
  BYTE parameter_block[128];
  int parameter_blocklen;
  int tfc;
  BYTE *tweak;
  int wrap;
  BYTE *xts;
  BYTE zero[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

  /* Check special conditions */
  if(unlikely(GR0_m(regs)))
    ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);

  /* Initialize values */
  tfc = GR0_tfc(regs);
  wrap = GR0_wrap(regs);
  keylen = (tfc - 49) * 8 + 8;
  parameter_blocklen = keylen + 64;
  if(wrap)
    parameter_blocklen += 32;

  /* Test writeability XTS parameter */
  ARCH_DEP(validate_operand)((GR_A(1, regs) + parameter_blocklen - 16) & ADDRESS_MAXWRAP(regs), 1, 31, ACCTYPE_WRITE, regs);

  /* Fetch the parameter block */
  ARCH_DEP(vfetchc)(parameter_block, parameter_blocklen - 1, GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, regs);
  tweak = &parameter_block[parameter_blocklen - 64];
  bsn = &parameter_block[parameter_blocklen - 48];
  ibi = &parameter_block[parameter_blocklen - 32];
  xts = &parameter_block[parameter_blocklen - 16];

#ifdef OPTION_PCC_DEBUG
  LOGBYTE("k     :", parameter_block, keylen);
  if(wrap)
    LOGBYTE("wkvp  :", &parameter_block[keylen], 32);
  LOGBYTE("tweak :", tweak, 16);
  LOGBYTE("bsn   :", bsn, 16);
  LOGBYTE("ibi   :", ibi, 16);
  LOGBYTE("xts   :", xts, 16);
#endif /* #ifdef OPTION_PCC_DEBUG */

  /* Verify and unwrap */
  if(wrap && unwrap_aes(parameter_block, keylen))
  {

#ifdef OPTION_PCC_DEBUG
    WRMSG(HHC90111, "D");
#endif /* #ifdef OPTION_PCC_DEBUG */

    regs->psw.cc = 1;
    return;
  }

  /* Encrypt tweak */
  rijndael_set_key(&context, parameter_block, keylen * 8);
  rijndael_encrypt(&context, tweak, tweak);

  /* Check block sequential number (j) == 0 */
  if(!memcmp(bsn, zero, 16))
  {
    zeromem(ibi, 15);
    ibi[15] = 128;
    memcpy(xts, tweak, 16);
  }
  else
  {
    /* Check intermediate block index (t) > 127 */
    if(memcmp(ibi, zero, 15) || ibi[15] > 127)
    {
      /* Invalid intermediate block index, return with cc2 */
      regs->psw.cc = 2;
      return;
    }

    /* Initial execution? */
    if(!ibi[15]) memcpy(xts, tweak, 16);

    /* Calculate xts parameter */
    do
    {
      if(bsn[ibi[15] / 8] & mask[ibi[15] % 8])
      {
#ifdef OPTION_PCC_DEBUG
        LOGBYTE("ibi   :", ibi, 16);
        LOGBYTE("xts   :", xts, 16);
#endif /* #ifdef OPTION_PCC_DEBUG */
        xts_gf_mult(xts, exp_table[ibi[15]], xts);
      }
      ibi[15]++;
    }
    while(ibi[15] != 128);
  }

#ifdef OPTION_PCC_DEBUG
  LOGBYTE("ibi   :", ibi, 16);
  LOGBYTE("xts   :", xts, 16);
#endif /* #ifdef OPTION_PCC_DEBUG */

  /* Store Intermediate Bit Index and XTS */
  ARCH_DEP(vstorec)(ibi, 31, (GR_A(1, regs) + parameter_blocklen - 32) & ADDRESS_MAXWRAP(regs), 1, regs);

  /* Normal completion */
  regs->psw.cc = 0;
}
#endif /* defined( FEATURE_077_MSA_EXTENSION_FACILITY_4 ) */

#if defined( FEATURE_076_MSA_EXTENSION_FACILITY_3 )
/*----------------------------------------------------------------------------*/
/* Perform cryptographic key management operation (PCKMO) FC 1-3        [RRE] */
/*----------------------------------------------------------------------------*/
static void ARCH_DEP(pckmo_dea)(REGS *regs)
{
  int fc;
  int keylen;
  BYTE parameter_block[64];
  int parameter_blocklen;

  /* Initialize values */
  fc = GR0_fc(regs);
  keylen = fc * 8;
  parameter_blocklen = keylen + 24;

  /* Test writeability */
  ARCH_DEP(validate_operand)(GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, parameter_blocklen - 1, ACCTYPE_WRITE, regs);

  /* Fetch the parameter block */
  ARCH_DEP(vfetchc)(parameter_block, parameter_blocklen - 1, GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, regs);

#ifdef OPTION_PCKMO_DEBUG
  LOGBYTE("key in : ", parameter_block, keylen);
  LOGBYTE("wkvp   : ", &parameter_block[keylen], parameter_blocklen - keylen);
#endif /* #ifdef OPTION_PCKMO_DEBUG */

  /* Encrypt the key and fill the wrapping key verification pattern */
  wrap_dea(parameter_block, keylen);

  /* Store the parameterblock */
  ARCH_DEP(vstorec)(parameter_block, parameter_blocklen - 1, GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, regs);

#ifdef OPTION_PCKMO_DEBUG
  LOGBYTE("key out: ", parameter_block, keylen);
  LOGBYTE("wkvp   : ", &parameter_block[keylen], parameter_blocklen - keylen);
#endif /* #ifdef OPTION_PCKMO_DEBUG */
}

/*----------------------------------------------------------------------------*/
/* Perform cryptographic key management operation (PCKMO) FC 18-20      [RRE] */
/*----------------------------------------------------------------------------*/
static void ARCH_DEP(pckmo_aes)(REGS *regs)
{
  int fc;
  int keylen;
  BYTE parameter_block[64];
  int parameter_blocklen;

  /* Initialize values */
  fc = GR0_fc(regs);
  keylen = (fc - 16) * 8;
  parameter_blocklen = keylen + 32;

  /* Test writeability */
  ARCH_DEP(validate_operand)(GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, parameter_blocklen - 1, ACCTYPE_WRITE, regs);

  /* Fetch the parameter block */
  ARCH_DEP(vfetchc)(parameter_block, parameter_blocklen - 1, GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, regs);

#ifdef OPTION_PCKMO_DEBUG
  LOGBYTE("key in : ", parameter_block, keylen);
  LOGBYTE("wkvp   : ", &parameter_block[keylen], parameter_blocklen - keylen);
#endif /* #ifdef OPTION_PCKMO_DEBUG */

  /* Encrypt the key and fill the wrapping key verification pattern */
  wrap_aes(parameter_block, keylen);

  /* Store the parameterblock */
  ARCH_DEP(vstorec)(parameter_block, parameter_blocklen - 1, GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, regs);

#ifdef OPTION_PCKMO_DEBUG
  LOGBYTE("key out: ", parameter_block, keylen);
  LOGBYTE("wkvp   : ", &parameter_block[keylen], parameter_blocklen - keylen);
#endif /* #ifdef OPTION_PCKMO_DEBUG */
}
#endif /* defined( FEATURE_076_MSA_EXTENSION_FACILITY_3 ) */

#if defined( FEATURE_146_MSA_EXTENSION_FACILITY_8 )
/*----------------------------------------------------------------------------*/
/* Cipher message with authentication (KMA) FC 18-20 and 26-28                */
/*----------------------------------------------------------------------------*/
/* Galois/Counter Mode (GCM) with AES as specified in NIST SP 800-38D.        */
/*                                                                            */
/* Parameter block:                                                           */
/*                                                                            */
/*   offset  len  field                                                       */
/*   ------  ---  ---------------------------------------------------------   */
/*      0     12  reserved                                                    */
/*     12      4  CV     32-bit counter value of the last counter block used  */
/*     16     16  T      tag (intermediate GHASH value until LPC processing)  */
/*     32     16  H      hash subkey (computed and stored when HS is zero)    */
/*     48      8  TAADL  total AAD length in bits (used when LPC is one)      */
/*     56      8  TPCL   total plaintext/ciphertext length in bits (LPC)      */
/*     64     16  J0     pre-counter block                                    */
/*     80  16-32  K      cryptographic key                                    */
/*  80+kl     32  WKaVP  AES wrapping key verification pattern (FC 26-28)     */
/*                                                                            */
/* The third operand (AAD) is authenticated first. Then the second operand    */
/* is encrypted (M=0) or decrypted (M=1) into the first operand location and  */
/* the ciphertext is authenticated. The counter block for each 16-byte block  */
/* is formed from bytes 0-11 of J0 and the incremented counter value. When    */
/* LPC is one and all data has been processed, the length block (TAADL ||     */
/* TPCL) is authenticated and the final tag E(K,J0) xor GHASH replaces T.     */
/* Comparing the tag after decryption is the responsibility of the program.   */
/*                                                                            */
/* A partial last block is only allowed for the AAD when LAAD is one and for  */
/* the plaintext/ciphertext when LPC is one.                                  */
/*----------------------------------------------------------------------------*/
static void ARCH_DEP(kma_gcm_aes)(int r1, int r2, int r3, REGS *regs)
{
  rijndael_ctx context;
  BYTE counter_block[16];
  U32 cv;
  BYTE ghash_block[16];
  BYTE *h;
  int i;
  BYTE *j0;
  BYTE *key;
  int keylen;
  BYTE message_block[16];
  int modifier_bit;
  int n;
  int processed;
  BYTE parameter_block[80 + 32 + 32];
  int parameter_blocklen;
  int r1_is_not_r2;
  BYTE *t;
  int tfc;
  int wrap;

  /* Check special conditions */
  if(unlikely(!GR0_kma_laad(regs) && (GR_A(r3 + 1, regs) % 16)))
    ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
  if(unlikely(!GR0_kma_lpc(regs) && (GR_A(r2 + 1, regs) % 16)))
    ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);

  /* Initialize values */
  tfc = GR0_tfc(regs);
  wrap = GR0_wrap(regs);
  modifier_bit = GR0_m(regs);
  keylen = (tfc - 16) * 8;
  parameter_blocklen = 80 + keylen;
  if(wrap)
    parameter_blocklen += 32;
  t = &parameter_block[16];
  h = &parameter_block[32];
  j0 = &parameter_block[64];
  key = &parameter_block[80];

  /* Test writeability of the counter value, tag and hash subkey fields */
  ARCH_DEP(validate_operand)(GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, 47, ACCTYPE_WRITE, regs);

  /* Fetch the parameter block */
  ARCH_DEP(vfetchc)(parameter_block, parameter_blocklen - 1, GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, regs);

#ifdef OPTION_KMA_DEBUG
  LOGBYTE("cv    :", &parameter_block[12], 4);
  LOGBYTE("t     :", t, 16);
  LOGBYTE("h     :", h, 16);
  LOGBYTE("taadl :", &parameter_block[48], 8);
  LOGBYTE("tpcl  :", &parameter_block[56], 8);
  LOGBYTE("j0    :", j0, 16);
  LOGBYTE("k     :", key, keylen);
  if(wrap)
    LOGBYTE("wkvp  :", &key[keylen], 32);
#endif /* #ifdef OPTION_KMA_DEBUG */

  /* Verify and unwrap the key */
  if(wrap && unwrap_aes(key, keylen))
  {

#ifdef OPTION_KMA_DEBUG
    WRMSG(HHC90111, "D");
#endif /* #ifdef OPTION_KMA_DEBUG */

    regs->psw.cc = 1;
    return;
  }

  /* Set the cryptographic key */
  rijndael_set_key(&context, key, keylen * 8);

  /* Compute and store the hash subkey if it is not supplied */
  if(!GR0_kma_hs(regs))
  {
    memset(h, 0, 16);
    rijndael_encrypt(&context, h, h);
    ARCH_DEP(vstorec)(h, 15, (GR_A(1, regs) + 32) & ADDRESS_MAXWRAP(regs), 1, regs);

#ifdef OPTION_KMA_DEBUG
    LOGBYTE("h out :", h, 16);
#endif /* #ifdef OPTION_KMA_DEBUG */

  }

  cv = fetch_fw(&parameter_block[12]);
  processed = 0;

  /* Authenticate the additional authenticated data (third operand) */
  while(GR_A(r3 + 1, regs) && processed < PROCESS_MAX)
  {
    n = (int) min(16, GR_A(r3 + 1, regs));

    /* Fetch a block of data, a partial block is padded with zeros */
    memset(message_block, 0, 16);
    ARCH_DEP(vfetchc)(message_block, n - 1, GR_A(r3, regs) & ADDRESS_MAXWRAP(regs), r3, regs);

#ifdef OPTION_KMA_DEBUG
    LOGBYTE("aad   :", message_block, n);
#endif /* #ifdef OPTION_KMA_DEBUG */

    /* XOR and multiply */
    for(i = 0; i < 16; i++)
      t[i] ^= message_block[i];
    gcm_gf_mult(t, h, t);

    /* Store the intermediate tag */
    ARCH_DEP(vstorec)(t, 15, (GR_A(1, regs) + 16) & ADDRESS_MAXWRAP(regs), 1, regs);

    /* Update the registers */
    SET_GR_A(r3, regs, GR_A(r3, regs) + n);
    SET_GR_A(r3 + 1, regs, GR_A(r3 + 1, regs) - n);
    processed += 16;

#ifdef OPTION_KMA_DEBUG
    WRMSG(HHC90108, "D", r3, (regs)->GR(r3));
    WRMSG(HHC90108, "D", r3 + 1, (regs)->GR(r3 + 1));
#endif /* #ifdef OPTION_KMA_DEBUG */

  }

  /* Encrypt or decrypt the second operand and authenticate the ciphertext */
  r1_is_not_r2 = r1 != r2;
  while(GR_A(r2 + 1, regs) && processed < PROCESS_MAX)
  {
    n = (int) min(16, GR_A(r2 + 1, regs));

    /* Fetch a block of data */
    ARCH_DEP(vfetchc)(message_block, n - 1, GR_A(r2, regs) & ADDRESS_MAXWRAP(regs), r2, regs);

#ifdef OPTION_KMA_DEBUG
    LOGBYTE("input :", message_block, n);
#endif /* #ifdef OPTION_KMA_DEBUG */

    /* Compute the key stream from the next counter block */
    cv++;
    memcpy(counter_block, j0, 12);
    store_fw(&counter_block[12], cv);
    rijndael_encrypt(&context, counter_block, counter_block);

    /* XOR, the ciphertext is the input for decryption and the output for */
    /* encryption. A partial block is padded with zeros for the GHASH.     */
    memset(ghash_block, 0, 16);
    for(i = 0; i < n; i++)
    {
      if(modifier_bit)
        ghash_block[i] = message_block[i];
      message_block[i] ^= counter_block[i];
      if(!modifier_bit)
        ghash_block[i] = message_block[i];
    }

    /* XOR and multiply */
    for(i = 0; i < 16; i++)
      t[i] ^= ghash_block[i];
    gcm_gf_mult(t, h, t);

    /* Store the output */
    ARCH_DEP(vstorec)(message_block, n - 1, GR_A(r1, regs) & ADDRESS_MAXWRAP(regs), r1, regs);

#ifdef OPTION_KMA_DEBUG
    LOGBYTE("output:", message_block, n);
#endif /* #ifdef OPTION_KMA_DEBUG */

    /* Store the counter value and the intermediate tag */
    store_fw(&parameter_block[12], cv);
    ARCH_DEP(vstorec)(&parameter_block[12], 19, (GR_A(1, regs) + 12) & ADDRESS_MAXWRAP(regs), 1, regs);

    /* Update the registers */
    SET_GR_A(r1, regs, GR_A(r1, regs) + n);
    if(likely(r1_is_not_r2))
      SET_GR_A(r2, regs, GR_A(r2, regs) + n);
    SET_GR_A(r2 + 1, regs, GR_A(r2 + 1, regs) - n);
    processed += 16;

#ifdef OPTION_KMA_DEBUG
    WRMSG(HHC90108, "D", r1, (regs)->GR(r1));
    WRMSG(HHC90108, "D", r2, (regs)->GR(r2));
    WRMSG(HHC90108, "D", r2 + 1, (regs)->GR(r2 + 1));
#endif /* #ifdef OPTION_KMA_DEBUG */

  }

  /* CPU-determined amount of data processed */
  if(GR_A(r3 + 1, regs) || GR_A(r2 + 1, regs))
  {
    regs->psw.cc = 3;
    return;
  }

  /* Compute the final tag after the last plaintext/ciphertext */
  if(GR0_kma_lpc(regs))
  {
    /* Authenticate the length block TAADL || TPCL */
    for(i = 0; i < 16; i++)
      t[i] ^= parameter_block[48 + i];
    gcm_gf_mult(t, h, t);

    /* XOR with the encrypted pre-counter block */
    rijndael_encrypt(&context, j0, counter_block);
    for(i = 0; i < 16; i++)
      t[i] ^= counter_block[i];

    /* Store the tag */
    ARCH_DEP(vstorec)(t, 15, (GR_A(1, regs) + 16) & ADDRESS_MAXWRAP(regs), 1, regs);
  }

#ifdef OPTION_KMA_DEBUG
  LOGBYTE("cv out:", &parameter_block[12], 4);
  LOGBYTE("t out :", t, 16);
#endif /* #ifdef OPTION_KMA_DEBUG */

  regs->psw.cc = 0;
}
#endif /* defined( FEATURE_146_MSA_EXTENSION_FACILITY_8 ) */

/*----------------------------------------------------------------------------*/
/* B93E KIMD  - Compute intermediate message digest                     [RRE] */
/*----------------------------------------------------------------------------*/
DEF_INST(dyn_compute_intermediate_message_digest)
{
  int msa;
  BYTE query_bits[][16] =
  {
    { 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /**/ { 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
  };
  int r1;
  int r2;

  RRE(inst, regs, r1, r2);
  PER_ZEROADDR_CHECK2( regs, 1, r2 );

  /* The following is the same as doing a FACILITY_CHECK */
  msa = get_msa(regs);
  if(msa < 0)
    ARCH_DEP(program_interrupt)(regs, PGM_OPERATION_EXCEPTION);

#ifdef OPTION_KIMD_DEBUG
  WRMSG(HHC90100, "D", "KIMD: compute intermediate message digest");
  WRMSG(HHC90101, "D", 1, r1);
  WRMSG(HHC90102, "D", regs->GR(r1));
  WRMSG(HHC90101, "D", 2, r2);
  WRMSG(HHC90102, "D", regs->GR(r2));
  WRMSG(HHC90103, "D", regs->GR(r2 + 1));
  WRMSG(HHC90104, "D", 0, regs->GR(0));
  WRMSG(HHC90105, "D", TRUEFALSE(GR0_m(regs)));
  WRMSG(HHC90106, "D", GR0_fc(regs));
  WRMSG(HHC90104, "D", 1, regs->GR(1));
#endif /* #ifdef OPTION_KIMD_DEBUG */

  /* Check special conditions */
  if(unlikely(!r2 || r2 & 0x01 || GR0_m(regs)))
    ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);

  switch(GR0_fc(regs))
  {
    case 0: /* Query */
    {
#if defined( FEATURE_057_MSA_EXTENSION_FACILITY_5 )
      /* SHA-3 and SHAKE (MSA 6) are provided together with MSA 5 */
      if(FACILITY_ENABLED( 057_MSA_EXTENSION_5, regs ))
        query_bits[msa][4] |= 0xFC;
#endif /* defined( FEATURE_057_MSA_EXTENSION_FACILITY_5 ) */

      /* Store the parameter block */
      ARCH_DEP(vstorec)(query_bits[msa], 15, GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, regs);

#ifdef OPTION_KIMD_DEBUG
      LOGBYTE("output:", query_bits[msa], 16);
#endif /* #ifdef OPTION_KIMD_DEBUG */

      /* Set condition code 0 */
      regs->psw.cc = 0;
      return;
    }
    case 1: /* sha-1 */
    {
      ARCH_DEP(kimd_sha)(r1, r2, regs, 0);
      break;
    }

#if defined( FEATURE_MSA_EXTENSION_FACILITY_1 )
    case 2: /* sha-256 */
    {
      if(msa >= 1)
        ARCH_DEP(kimd_sha)(r1, r2, regs, 0);
      else
        ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
      break;
    }
#endif /* defined( FEATURE_MSA_EXTENSION_FACILITY_1 ) */

#if defined( FEATURE_MSA_EXTENSION_FACILITY_2 )
    case 3: /* sha-512 */
    {
      if(msa >= 2)
        ARCH_DEP(kimd_sha)(r1, r2, regs, 0);
      else
        ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
      break;
    }
#endif /* defined( FEATURE_MSA_EXTENSION_FACILITY_2 ) */

#if defined( FEATURE_077_MSA_EXTENSION_FACILITY_4 )
    case 65: /* ghash */
    {
      if(msa >= 4)
        ARCH_DEP(kimd_ghash)(r1, r2, regs);
      else
        ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
      break;
    }
#endif /* defined( FEATURE_077_MSA_EXTENSION_FACILITY_4 ) */

#if defined( FEATURE_057_MSA_EXTENSION_FACILITY_5 )
    case 32: /* sha3-224 */
    case 33: /* sha3-256 */
    case 34: /* sha3-384 */
    case 35: /* sha3-512 */
    case 36: /* shake-128 */
    case 37: /* shake-256 */
    {
      if(FACILITY_ENABLED( 057_MSA_EXTENSION_5, regs ))
        ARCH_DEP(kimd_sha3)(r1, r2, regs);
      else
        ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
      break;
    }
#endif /* defined( FEATURE_057_MSA_EXTENSION_FACILITY_5 ) */

    default:
    {
      ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
      break;
    }
  }
}

/*----------------------------------------------------------------------------*/
/* B93F KLMD  - Compute last message digest                             [RRE] */
/*----------------------------------------------------------------------------*/
DEF_INST(dyn_compute_last_message_digest)
{
  int msa;
  BYTE query_bits[][16] =
  {
    { 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /**/ { 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /**/ { 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
  };
  int r1;
  int r2;

  RRE(inst, regs, r1, r2);

#if defined( FEATURE_PER_ZERO_ADDRESS_DETECTION_FACILITY )
    if (0
        || GR_A( 1,  regs ) == 0
        || GR_A( r1, regs ) == 0
        || GR_A( r2, regs ) == 0
    )
        ARCH_DEP( per3_zero )( regs );
#endif

  /* The following is the same as doing a FACILITY_CHECK */
  msa = get_msa(regs);
  if(msa < 0)
    ARCH_DEP(program_interrupt)(regs, PGM_OPERATION_EXCEPTION);

#ifdef OPTION_KLMD_DEBUG
  WRMSG(HHC90100, "D", "KLMD: compute last message digest");
  WRMSG(HHC90101, "D", 1, r1);
  WRMSG(HHC90102, "D", regs->GR(r1));
  WRMSG(HHC90101, "D", 2, r2);
  WRMSG(HHC90102, "D", regs->GR(r2));
  WRMSG(HHC90103, "D", regs->GR(r2 + 1));
  WRMSG(HHC90104, "D", 0, regs->GR(0));
  WRMSG(HHC90105, "D", TRUEFALSE(GR0_m(regs)));
  WRMSG(HHC90106, "D", GR0_fc(regs));
  WRMSG(HHC90104, "D", 1, regs->GR(1));
#endif /* #ifdef OPTION_KLMD_DEBUG */

  /* Check special conditions */
  if(unlikely(!r2 || r2 & 0x01 || GR0_m(regs)))
    ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);

  switch(GR0_fc(regs))
  {
    case 0: /* Query */
    {
#if defined( FEATURE_057_MSA_EXTENSION_FACILITY_5 )
      /* SHA-3 and SHAKE (MSA 6) are provided together with MSA 5 */
      if(FACILITY_ENABLED( 057_MSA_EXTENSION_5, regs ))
        query_bits[msa][4] |= 0xFC;
#endif /* defined( FEATURE_057_MSA_EXTENSION_FACILITY_5 ) */

      /* Store the parameter block */
      ARCH_DEP(vstorec)(query_bits[msa], 15, GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, regs);

#ifdef OPTION_KLMD_DEBUG
      LOGBYTE("output:", query_bits[msa], 16);
#endif /* #ifdef OPTION_KLMD_DEBUG */

      /* Set condition code 0 */
      regs->psw.cc = 0;
      return;
    }
    case 1: /* sha-1 */
    {
      ARCH_DEP(klmd_sha)(r1, r2, regs);
      break;
    }
#if defined( FEATURE_MSA_EXTENSION_FACILITY_1 )
    case 2: /* sha-256 */
    {
      if(msa >= 1)
        ARCH_DEP(klmd_sha)(r1, r2, regs);
      else
        ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
      break;
    }
#endif /* defined( FEATURE_MSA_EXTENSION_FACILITY_1 ) */

#if defined( FEATURE_MSA_EXTENSION_FACILITY_2 )
    case 3: /* sha-512 */
    {
      if(msa >= 2)
        ARCH_DEP(klmd_sha)(r1, r2, regs);
      else
        ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
      break;
    }
#endif /* defined( FEATURE_MSA_EXTENSION_FACILITY_2 ) */

#if defined( FEATURE_057_MSA_EXTENSION_FACILITY_5 )
    case 32: /* sha3-224 */
    case 33: /* sha3-256 */
    case 34: /* sha3-384 */
    case 35: /* sha3-512 */
    case 36: /* shake-128 */
    case 37: /* shake-256 */
    {
      if(FACILITY_ENABLED( 057_MSA_EXTENSION_5, regs ))
        ARCH_DEP(klmd_sha3)(r1, r2, regs);
      else
        ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
      break;
    }
#endif /* defined( FEATURE_057_MSA_EXTENSION_FACILITY_5 ) */

    default:
    {
      ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
      break;
    }
  }
}

/*----------------------------------------------------------------------------*/
/* B92E KM    - Cipher message                                          [RRE] */
/*----------------------------------------------------------------------------*/
DEF_INST(dyn_cipher_message)
{
  int msa;
  BYTE query_bits[][16] =
  {
    { 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0xf0, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0xf0, 0x00, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0xf0, 0x70, 0x38, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0xf0, 0x70, 0x38, 0x38, 0x00, 0x00, 0x28, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
  };
  int r1;
  int r2;

  RRE(inst, regs, r1, r2);

#if defined( FEATURE_PER_ZERO_ADDRESS_DETECTION_FACILITY )
    if (0
        || GR_A( 1,  regs ) == 0
        || GR_A( r1, regs ) == 0
        || GR_A( r2, regs ) == 0
    )
        ARCH_DEP( per3_zero )( regs );
#endif

  /* The following is the same as doing a FACILITY_CHECK */
  msa = get_msa(regs);
  if(msa < 0)
    ARCH_DEP(program_interrupt)(regs, PGM_OPERATION_EXCEPTION);

#ifdef OPTION_KM_DEBUG
  WRMSG(HHC90100, "D", "KM: cipher message");
  WRMSG(HHC90101, "D", 1, r1);
  WRMSG(HHC90102, "D", regs->GR(r1));
  WRMSG(HHC90101, "D", 2, r2);
  WRMSG(HHC90102, "D", regs->GR(r2));
  WRMSG(HHC90103, "D", regs->GR(r2 + 1));
  WRMSG(HHC90104, "D", 0, regs->GR(0));
  WRMSG(HHC90107, "D", TRUEFALSE(GR0_m(regs)));
  WRMSG(HHC90106, "D", GR0_fc(regs));
  WRMSG(HHC90104, "D", 1, regs->GR(1));
#endif /* #ifdef OPTION_KM_DEBUG */

  /* Check special conditions */
  if(unlikely(!r1 || r1 & 0x01 || !r2 || r2 & 0x01))
    ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);

  switch(GR0_fc(regs))
  {
    case 0: /* Query */
    {
      /* Store the parameter block */
      ARCH_DEP(vstorec)(query_bits[msa], 15, GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, regs);

#ifdef OPTION_KM_DEBUG
      LOGBYTE("output:", query_bits[msa], 16);
#endif /* #ifdef OPTION_KM_DEBUG */

      /* Set condition code 0 */
      regs->psw.cc = 0;
      return;
    }
    case 1: /* dea */
    case 2: /* tdea-128 */
    case 3: /* tdea-192 */
    {
      ARCH_DEP(km_dea)(r1, r2, regs);
      break;
    }

#if defined( FEATURE_076_MSA_EXTENSION_FACILITY_3 )
    case 9: /* encrypted dea */
    case 10: /* encrypted tdea-128 */
    case 11: /* encrypted tdea-192 */
    {
      if(msa >= 3)
        ARCH_DEP(km_dea)(r1, r2, regs);
      else
        ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
      break;
    }
#endif /* defined( FEATURE_076_MSA_EXTENSION_FACILITY_3 ) */

#if defined( FEATURE_MSA_EXTENSION_FACILITY_1 )
    case 18: /* aes-128 */
    {
      if(msa >= 1)
        ARCH_DEP(km_aes)(r1, r2, regs);
      else
        ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
      break;
    }
#endif /* defined( FEATURE_MSA_EXTENSION_FACILITY_1 ) */

#if defined( FEATURE_MSA_EXTENSION_FACILITY_2 )
    case 19: /* aes-192 */
    case 20: /* aes-256 */
    {
      if(msa >= 2)
        ARCH_DEP(km_aes)(r1, r2, regs);
      else
        ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
      break;
    }
#endif /* defined( FEATURE_MSA_EXTENSION_FACILITY_2 ) */

#if defined( FEATURE_076_MSA_EXTENSION_FACILITY_3 )
    case 26: /* encrypted aes-128 */
    case 27: /* encrypted aes-192 */
    case 28: /* encrypted aes-256 */
    {
      if(msa >= 3)
        ARCH_DEP(km_aes)(r1, r2, regs);
      else
        ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
      break;
    }
#endif /* defined( FEATURE_076_MSA_EXTENSION_FACILITY_3 ) */

#if defined( FEATURE_077_MSA_EXTENSION_FACILITY_4 )
    case 50: /* xts aes-128 */
    case 52: /* xts aes-256 */
    case 58: /* encrypted xts aes-128 */
    case 60: /* encrypted xts aes-256 */
    {
      if(msa >= 4)
        ARCH_DEP(km_xts_aes)(r1, r2, regs);
      else
        ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
      break;
    }
#endif /* defined( FEATURE_077_MSA_EXTENSION_FACILITY_4 ) */

    default:
    {
      ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
      break;
    }
  }
}

/*----------------------------------------------------------------------------*/
/* B91E KMAC  - Compute message authentication code                     [RRE] */
/*----------------------------------------------------------------------------*/
DEF_INST(dyn_compute_message_authentication_code)
{
  int msa;
  BYTE query_bits[][16] =
  {
    { 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /**/ { 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /**/ { 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0xf0, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0xf0, 0x70, 0x38, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
  };
  int r1;
  int r2;

  RRE(inst, regs, r1, r2);
  PER_ZEROADDR_CHECK2( regs, 1, r2 );

  /* The following is the same as doing a FACILITY_CHECK */
  msa = get_msa(regs);
  if(msa < 0)
    ARCH_DEP(program_interrupt)(regs, PGM_OPERATION_EXCEPTION);

#ifdef OPTION_KMAC_DEBUG
  WRMSG(HHC90100, "D", "KMAC: compute message authentication code");
  WRMSG(HHC90101, "D", 2, r2);
  WRMSG(HHC90102, "D", regs->GR(r2));
  WRMSG(HHC90103, "D", regs->GR(r2 + 1));
  WRMSG(HHC90104, "D", 0, regs->GR(0));
  WRMSG(HHC90105, "D", TRUEFALSE(GR0_m(regs)));
  WRMSG(HHC90106, "D", GR0_fc(regs));
  WRMSG(HHC90104, "D", 1, regs->GR(1));
#endif /* #ifdef OPTION_KMAC_DEBUG */

  /* Check special conditions */
  if(unlikely(!r2 || r2 & 0x01 || GR0_m(regs)))
    ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);

  switch(GR0_fc(regs))
  {
    case 0: /* Query */
    {
      /* Store the parameter block */
      ARCH_DEP(vstorec)(query_bits[msa], 15, GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, regs);

#ifdef OPTION_KMAC_DEBUG
      LOGBYTE("output:", query_bits[msa], 16);
#endif /* #ifdef OPTION_KMAC_DEBUG */

      /* Set condition code 0 */
      regs->psw.cc = 0;
      return;
    }
    case 1: /* dea */
    case 2: /* tdea-128 */
    case 3: /* tdea-192 */
    {
      ARCH_DEP(kmac_dea)(r1, r2, regs);
      break;
    }

#if defined( FEATURE_076_MSA_EXTENSION_FACILITY_3 )
    case 9: /* encrypted dea */
    case 10: /* encrypted tdea-128 */
    case 11: /* encrypted tdea-192 */
    {
      if(msa >= 3)
        ARCH_DEP(kmac_dea)(r1, r2, regs);
      else
        ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
      break;
    }
#endif /* defined( FEATURE_076_MSA_EXTENSION_FACILITY_3 ) */

#if defined( FEATURE_077_MSA_EXTENSION_FACILITY_4 )
    case 18: /* aes */
    case 19: /* aes-192 */
    case 20: /* aes-256 */
    case 26: /* encrypted aes */
    case 27: /* encrypted aes-192 */
    case 28: /* encrypted aes-256 */
    {
      if(msa >= 4)
        ARCH_DEP(kmac_aes)(r1, r2, regs);
      else
        ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
      break;
    }
#endif /* defined( FEATURE_077_MSA_EXTENSION_FACILITY_4 ) */

    default:
    {
      ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
      break;
    }
  }
}

/*----------------------------------------------------------------------------*/
/* B92F KMC   - Cipher message with chaining                            [RRE] */
/*----------------------------------------------------------------------------*/
DEF_INST(dyn_cipher_message_with_chaining)
{
  int msa;
  BYTE query_bits[][16] =
  {
    { 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0xf0, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0xf0, 0x00, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0xf0, 0x70, 0x38, 0x38, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /**/ { 0xf0, 0x70, 0x38, 0x38, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
  };
  int r1;
  int r2;

  RRE(inst, regs, r1, r2);

#if defined( FEATURE_PER_ZERO_ADDRESS_DETECTION_FACILITY )
    if (0
        || GR_A( 1,  regs ) == 0
        || GR_A( r1, regs ) == 0
        || GR_A( r2, regs ) == 0
    )
        ARCH_DEP( per3_zero )( regs );
#endif

  /* The following is the same as doing a FACILITY_CHECK */
  msa = get_msa(regs);
  if(msa < 0)
    ARCH_DEP(program_interrupt)(regs, PGM_OPERATION_EXCEPTION);

#ifdef OPTION_KMC_DEBUG
  WRMSG(HHC90100, "D", "KMC: cipher message with chaining");
  WRMSG(HHC90101, "D", 1, r1);
  WRMSG(HHC90102, "D", regs->GR(r1));
  WRMSG(HHC90101, "D", 2, r2);
  WRMSG(HHC90102, "D", regs->GR(r2));
  WRMSG(HHC90103, "D", regs->GR(r2 + 1));
  WRMSG(HHC90104, "D", 0, regs->GR(0));
  WRMSG(HHC90107, "D", TRUEFALSE(GR0_m(regs)));
  WRMSG(HHC90106, "D", GR0_fc(regs));
  WRMSG(HHC90104, "D", 1, regs->GR(1));
#endif /* #ifdef OPTION_KMC_DEBUG */

  /* Check special conditions */
  if(unlikely(!r1 || r1 & 0x01 || !r2 || r2 & 0x01))
    ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);

  switch(GR0_fc(regs))
  {
    case 0: /* Query */
    {
      /* Store the parameter block */
      ARCH_DEP(vstorec)(query_bits[msa], 15, GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, regs);

#ifdef OPTION_KMC_DEBUG
      LOGBYTE("output:", query_bits[msa], 16);
#endif /* #ifdef OPTION_KMC_DEBUG */

      /* Set condition code 0 */
      regs->psw.cc = 0;
      return;
    }
    case 1: /* dea */
    case 2: /* tdea-128 */
    case 3: /* tdea-192 */
    {
      ARCH_DEP(kmc_dea)(r1, r2, regs);
      break;
    }

#if defined( FEATURE_076_MSA_EXTENSION_FACILITY_3 )
    case 9: /* encrypted dea */
    case 10: /* encrypted tdea-128 */
    case 11: /* encrypted tdea-192 */
    {
      if(msa >= 3)
        ARCH_DEP(kmc_dea)(r1, r2, regs);
      else
        ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
      break;
    }
#endif /* defined( FEATURE_076_MSA_EXTENSION_FACILITY_3 ) */

#if defined( FEATURE_MSA_EXTENSION_FACILITY_1 )
    case 18: /* aes-128 */
    {
      if(msa >= 1)
        ARCH_DEP(kmc_aes)(r1, r2, regs);
      else
        ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
      break;
    }
#endif /* defined( FEATURE_MSA_EXTENSION_FACILITY_1 ) */

#if defined( FEATURE_MSA_EXTENSION_FACILITY_2 )
    case 19: /* aes-192 */
    case 20: /* aes-256 */
    {
      if(msa >= 2)
        ARCH_DEP(kmc_aes)(r1, r2, regs);
      else
        ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
      break;
    }
#endif /* defined( FEATURE_MSA_EXTENSION_FACILITY_2 ) */

#if defined( FEATURE_076_MSA_EXTENSION_FACILITY_3 )
    case 26: /* encrypted aes-128 */
    case 27: /* encrypted aes-192 */
    case 28: /* encrypted aes-256 */
    {
      if(msa >= 3)
        ARCH_DEP(kmc_aes)(r1, r2, regs);
      else
        ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
      break;
    }
#endif /* defined( FEATURE_076_MSA_EXTENSION_FACILITY_3 ) */

#if defined( FEATURE_MSA_EXTENSION_FACILITY_1 )
    case 67: /* prng */
    {
      if(msa >= 1)
        ARCH_DEP(kmc_prng)(r1, r2, regs);
      else
        ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
      break;
    }
#endif /* defined( FEATURE_MSA_EXTENSION_FACILITY_1 ) */

    default:
    {
      ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
      break;
    }
  }
}

/*----------------------------------------------------------------------------*/
/* B929 KMA   - Cipher message with authentication                    [RRF-b] */
/*----------------------------------------------------------------------------*/
/* The GCM-AES functions require Message-Security-Assist Extension 8          */
/* (facility 146). When the facility is disabled with the FACILITY command,   */
/* the facility support removes this instruction (operation exception), just */
/* like on a machine without MSA 8. If the instruction is reached without     */
/* MSA 8 nevertheless (e.g. a build without FEATURE_146), only the query      */
/* function is provided, which reports no function besides query itself.     */
/* This compatibility support exists because z/OS ICSF issues the KMA query   */
/* regardless of facility 146 (SDL-Hercules-390/hyperion issue #890).         */
/*----------------------------------------------------------------------------*/
DEF_INST(dyn_cipher_message_with_authentication)
{
  static const BYTE query_bits[16] =
  {
    0x80, 0x00, 0x38, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  };
  static const BYTE query_bits_no_msa8[16] = { 0x80 };
  int msa8 = FALSE;
  int r1;
  int r2;
  int r3;

  RRF_M(inst, regs, r1, r2, r3);
  PER_ZEROADDR_CHECK( regs, 1 );
  PER_ZEROADDR_LCHECK2( regs, r1, r2 + 1, r2, r2 + 1 );
  PER_ZEROADDR_LCHECK( regs, r3, r3 + 1 );
  TXFC_INSTR_CHECK( regs );

  /* The query function is available with any MSA level, see above */
  FACILITY_CHECK( 017_MSA, regs );

#if defined( FEATURE_146_MSA_EXTENSION_FACILITY_8 )
  msa8 = FACILITY_ENABLED( 146_MSA_EXTENSION_8, regs );
#endif /* defined( FEATURE_146_MSA_EXTENSION_FACILITY_8 ) */

#ifdef OPTION_KMA_DEBUG
  WRMSG(HHC90100, "D", "KMA: cipher message with authentication");
  WRMSG(HHC90101, "D", 1, r1);
  WRMSG(HHC90102, "D", regs->GR(r1));
  WRMSG(HHC90101, "D", 2, r2);
  WRMSG(HHC90102, "D", regs->GR(r2));
  WRMSG(HHC90103, "D", regs->GR(r2 + 1));
  WRMSG(HHC90101, "D", 3, r3);
  WRMSG(HHC90102, "D", regs->GR(r3));
  WRMSG(HHC90103, "D", regs->GR(r3 + 1));
  WRMSG(HHC90104, "D", 0, regs->GR(0));
  WRMSG(HHC90105, "D", TRUEFALSE(GR0_m(regs)));
  WRMSG(HHC90106, "D", GR0_fc(regs));
  WRMSG(HHC90104, "D", 1, regs->GR(1));
#endif /* #ifdef OPTION_KMA_DEBUG */

  /* Register designators must be valid even for query */
  if(unlikely(!r1 || r1 & 0x01 || !r2 || r2 & 0x01 || !r3 || r3 & 0x01 || r3 == r1 || r3 == r2))
    ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);

  switch(GR0_fc(regs))
  {
    case 0: /* Query */
    {
      /* Store the parameter block */
      ARCH_DEP(vstorec)(msa8 ? query_bits : query_bits_no_msa8, 15, GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, regs);

#ifdef OPTION_KMA_DEBUG
      LOGBYTE("output:", msa8 ? query_bits : query_bits_no_msa8, 16);
#endif /* #ifdef OPTION_KMA_DEBUG */

      /* Set condition code 0 */
      regs->psw.cc = 0;
      return;
    }

#if defined( FEATURE_146_MSA_EXTENSION_FACILITY_8 )
    case 18: /* gcm-aes-128 */
    case 19: /* gcm-aes-192 */
    case 20: /* gcm-aes-256 */
    case 26: /* gcm-encrypted-aes-128 */
    case 27: /* gcm-encrypted-aes-192 */
    case 28: /* gcm-encrypted-aes-256 */
    {
      if(msa8)
        ARCH_DEP(kma_gcm_aes)(r1, r2, r3, regs);
      else
        ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
      break;
    }
#endif /* defined( FEATURE_146_MSA_EXTENSION_FACILITY_8 ) */

    default:
    {
      ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
      break;
    }
  }
}

#if defined( FEATURE_155_MSA_EXTENSION_FACILITY_9 )
/*----------------------------------------------------------------------------*/
/* Fetch and store parameter blocks longer than 256 bytes (MSA 9)             */
/*----------------------------------------------------------------------------*/
static void ARCH_DEP(fetch_parameter_block)(BYTE *dst, int len, REGS *regs)
{
  int done;
  int n;

  for(done = 0; done < len; done += n)
  {
    n = min(256, len - done);
    ARCH_DEP(vfetchc)(dst + done, n - 1, (GR_A(1, regs) + done) & ADDRESS_MAXWRAP(regs), 1, regs);
  }
}

static void ARCH_DEP(store_parameter_block)(const BYTE *src, int len, REGS *regs)
{
  int done;
  int n;

  for(done = 0; done < len; done += n)
  {
    n = min(256, len - done);
    ARCH_DEP(vstorec)(src + done, n - 1, (GR_A(1, regs) + done) & ADDRESS_MAXWRAP(regs), 1, regs);
  }
}

static void ARCH_DEP(validate_parameter_block)(int len, REGS *regs)
{
  int done;
  int n;

  for(done = 0; done < len; done += n)
  {
    n = min(256, len - done);
    ARCH_DEP(validate_operand)((GR_A(1, regs) + done) & ADDRESS_MAXWRAP(regs), 1, n - 1, ACCTYPE_WRITE, regs);
  }
}

/*----------------------------------------------------------------------------*/
/* Hash the second operand (the message of KDSA EdDSA) from guest storage.    */
/* The registers are not updated here, an access exception leaves the        */
/* instruction restartable.                                                   */
/*----------------------------------------------------------------------------*/
static void ARCH_DEP(kdsa_hash_message)(ECC_HASH *h, int r2, REGS *regs)
{
  BYTE buf[256];
  U64 done;
  U64 len;
  int n;

  len = GR_A(r2 + 1, regs);
  for(done = 0; done < len; done += n)
  {
    n = (int) min((U64) 256, len - done);
    ARCH_DEP(vfetchc)(buf, n - 1, (GR_A(r2, regs) + done) & ADDRESS_MAXWRAP(regs), r2, regs);
    ecc_hash_update(h, buf, n);
  }
}
#endif /* defined( FEATURE_155_MSA_EXTENSION_FACILITY_9 ) */

/*----------------------------------------------------------------------------*/
/* B93A KDSA  - Compute digital signature authentication                [RRE] */
/*----------------------------------------------------------------------------*/
/* The digital signature functions require Message-Security-Assist           */
/* Extension 9 (facility 155). Without it only the query function is         */
/* provided, which reports no function besides query itself. This            */
/* compatibility support exists because z/OS ICSF issues the KDSA query      */
/* regardless of facility 155 (SDL-Hercules-390/hyperion issue #890); for    */
/* the same reason disabling facility 155 does not remove the instruction.   */
/*                                                                            */
/* Functions:                                                                 */
/*                                                                            */
/*   1-3   ECDSA-Verify-P256, -P384, -P521                                    */
/*   9-11  ECDSA-Sign-P256, -P384, -P521                                      */
/*   17-19 Encrypted-ECDSA-Sign-P256, -P384, -P521                            */
/*   32    EdDSA-Verify-Ed25519            36   EdDSA-Verify-Ed448            */
/*   40    EdDSA-Sign-Ed25519              44   EdDSA-Sign-Ed448              */
/*   48    Encrypted-EdDSA-Sign-Ed25519    52   Encrypted-EdDSA-Sign-Ed448    */
/*                                                                            */
/* The encrypted-key functions take the private key wrapped by PCKMO          */
/* (FC 32-34, 40, 41) followed by the 32-byte AES wrapping key verification   */
/* pattern: for ECDSA after the random number field, for EdDSA after the key  */
/* field. A mismatching verification pattern gives condition code 1.         */
/*                                                                            */
/* EdDSA (pure, RFC 8032) takes the message from the second operand (R2,      */
/* R2+1), which is processed completely; R2 and R2+1 are updated at the end.  */
/* The parameter block has fields of 32 (Ed25519) or 64 (Ed448) bytes, the    */
/* encodings of R, S and the public key are stored byte-reversed (right-      */
/* aligned big-endian), the private key seed as is, right-aligned:            */
/*                                                                            */
/*   verify:  R, S, public key                                                */
/*   sign:    R, S (both output), private key                                 */
/*                                                                            */
/* ECDSA parameter blocks consist of 5 fields of 32 (P-256), 48 (P-384) or    */
/* 80 (P-521) bytes, each a right-aligned big-endian integer:                 */
/*                                                                            */
/*   verify:  r, s, hash, public key x, public key y                          */
/*   sign:    r, s (both output), hash, private key, random number            */
/*                                                                            */
/* Bit 56 of GR0 is the deterministic bit for ECDSA sign: when one, the       */
/* random number in the parameter block is used, otherwise the CPU generates  */
/* its own.                                                                   */
/*                                                                            */
/* Condition codes: 0 verified / signed, 1 invalid parameter (hash zero,      */
/* private key out of range, public key not on the curve or not a valid      */
/* encoding), 2 signature invalid (verify) or random number unusable (ECDSA  */
/* sign, the program should retry).                                           */
/*----------------------------------------------------------------------------*/
DEF_INST(dyn_compute_digital_signature_authentication)
{
  BYTE query_bits[16] = { 0x80 };
  int msa9 = FALSE;
#if defined( FEATURE_155_MSA_EXTENSION_FACILITY_9 )
  BYTE parameter_block[5 * ECC_MAX_FIELD + 32];
  BYTE random_number[ECC_MAX_FIELD];
  BYTE enc[ECC_MAX_ENC];
  BYTE rhash[ECC_MAX_HASH];
  BYTE khash[ECC_MAX_HASH];
  BYTE wrapped[ECC_MAX_FIELD + 32];
  ECC_HASH hash;
  ECC_EDDSA_SIGN sign;
  int cc;
  int curve;
  int encrypted;
  int fc;
  int i;
  int len;
  int mask_bits;
  int n;
  int tries;
#endif /* defined( FEATURE_155_MSA_EXTENSION_FACILITY_9 ) */
  int r1;
  int r2;

  RRE(inst, regs, r1, r2);
  PER_ZEROADDR_CHECK( regs, 1 );
  PER_ZEROADDR_LCHECK( regs, r2, r2 + 1 );
  TXFC_INSTR_CHECK( regs );

  /* The query function is available with any MSA level, see above */
  FACILITY_CHECK( 017_MSA, regs );

#if defined( FEATURE_155_MSA_EXTENSION_FACILITY_9 )
  msa9 = FACILITY_ENABLED( 155_MSA_EXTENSION_9, regs );
#endif /* defined( FEATURE_155_MSA_EXTENSION_FACILITY_9 ) */

#ifdef OPTION_KDSA_DEBUG
  WRMSG(HHC90100, "D", "KDSA: compute digital signature authentication");
  WRMSG(HHC90101, "D", 2, r2);
  WRMSG(HHC90102, "D", regs->GR(r2));
  WRMSG(HHC90103, "D", regs->GR(r2 + 1));
  WRMSG(HHC90104, "D", 0, regs->GR(0));
  WRMSG(HHC90105, "D", TRUEFALSE(GR0_m(regs)));
  WRMSG(HHC90106, "D", GR0_fc(regs));
  WRMSG(HHC90104, "D", 1, regs->GR(1));
#endif /* #ifdef OPTION_KDSA_DEBUG */

  /* Register designators must be valid even for query. The R1 field   */
  /* is not used by KDSA and is ignored; programs may issue the query   */
  /* with a nonzero R1, e.g. KDSA 14,14 (B93A00EE).                     */
  if(unlikely(!r2 || (r2 & 1)))
    ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);

  switch(GR0_fc(regs))
  {
    case 0: /* Query */
    {
      if(msa9)
      {
        query_bits[0] = 0xF0;   /* query, ecdsa-verify-p256/p384/p521 */
        query_bits[1] = 0x70;   /* ecdsa-sign-p256/p384/p521          */
        query_bits[2] = 0x70;   /* encrypted-ecdsa-sign-p256/p384/p521 */
        query_bits[4] = 0x88;   /* eddsa-verify-ed25519/ed448         */
        query_bits[5] = 0x88;   /* eddsa-sign-ed25519/ed448           */
        query_bits[6] = 0x88;   /* encrypted-eddsa-sign-ed25519/ed448 */
      }

      /* Store the parameter block */
      ARCH_DEP(vstorec)(query_bits, 15, GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, regs);

#ifdef OPTION_KDSA_DEBUG
      LOGBYTE("output:", query_bits, 16);
#endif /* #ifdef OPTION_KDSA_DEBUG */

      regs->psw.cc = 0;
      return;
    }

#if defined( FEATURE_155_MSA_EXTENSION_FACILITY_9 )
    case 1: /* ecdsa-verify-p256 */
    case 2: /* ecdsa-verify-p384 */
    case 3: /* ecdsa-verify-p521 */
    {
      if(!msa9)
        ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);

      fc = GR0_fc(regs);
      curve = fc - 1;
      len = ecc_wcurve_len(curve);
      ARCH_DEP(fetch_parameter_block)(parameter_block, 5 * len, regs);
      regs->psw.cc = ecc_ecdsa_verify(curve, parameter_block);
      return;
    }
    case 9: /* ecdsa-sign-p256 */
    case 10: /* ecdsa-sign-p384 */
    case 11: /* ecdsa-sign-p521 */
    case 17: /* encrypted-ecdsa-sign-p256 */
    case 18: /* encrypted-ecdsa-sign-p384 */
    case 19: /* encrypted-ecdsa-sign-p521 */
    {
      if(!msa9)
        ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);

      fc = GR0_fc(regs);
      encrypted = fc >= 17;
      curve = (fc & 0x07) - 1;
      len = ecc_wcurve_len(curve);

      /* The signature is stored into the first two fields. With an     */
      /* encrypted key the wrapping key verification pattern follows    */
      /* the random number.                                             */
      ARCH_DEP(validate_parameter_block)(2 * len, regs);
      ARCH_DEP(fetch_parameter_block)(parameter_block, 5 * len + (encrypted ? 32 : 0), regs);

      if(encrypted)
      {
        memcpy(wrapped, parameter_block + 3 * len, len);
        memcpy(wrapped + len, parameter_block + 5 * len, 32);
        if(unwrap_ecc(wrapped, len))
        {
          /* Verification pattern mismatch */
          crypto_secure0(parameter_block, sizeof(parameter_block));
          regs->psw.cc = 1;
          return;
        }
        memcpy(parameter_block + 3 * len, wrapped, len);
        crypto_secure0(wrapped, sizeof(wrapped));
      }

      if(GR0_m(regs))
      {
        /* Deterministic: use the random number of the parameter block */
        cc = ecc_ecdsa_sign(curve, parameter_block, parameter_block + 4 * len);
      }
      else
      {
        /* Generate the random number, with the leading bits beyond the  */
        /* bit length of the order cleared, until it is usable           */
        if(!sysblk.PRNOrandhand)
          VERIFY( hopen_CSRNG( &sysblk.PRNOrandhand ) );
        mask_bits = 8 * len - ecc_wcurve_order_bits(curve);
        cc = ECC_CC_FAILED;
        for(tries = 0; tries < 64 && cc == ECC_CC_FAILED; tries++)
        {
          VERIFY( hget_random_bytes( random_number, len, &sysblk.PRNOrandhand ) );
          for(i = 0; i < mask_bits / 8; i++)
            random_number[i] = 0;
          if(mask_bits % 8)
            random_number[mask_bits / 8] &= 0xFF >> (mask_bits % 8);
          cc = ecc_ecdsa_sign(curve, parameter_block, random_number);
        }
        crypto_secure0(random_number, sizeof(random_number));
      }

      if(cc == ECC_CC_OK)
        ARCH_DEP(store_parameter_block)(parameter_block, 2 * len, regs);

      /* Do not leave the private key and random number behind */
      crypto_secure0(parameter_block, sizeof(parameter_block));

      regs->psw.cc = cc;
      return;
    }
    case 32: /* eddsa-verify-ed25519 */
    case 36: /* eddsa-verify-ed448 */
    {
      if(!msa9)
        ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);

      curve = GR0_fc(regs) == 32 ? ECC_ED25519 : ECC_ED448;
      len = ecc_ecurve_fld(curve);
      n = ecc_ecurve_enclen(curve);
      ARCH_DEP(fetch_parameter_block)(parameter_block, 3 * len, regs);

      /* k = H(R || A || M) */
      ecc_hash_init(&hash, curve);
      ecc_eddsa_field_to_enc(curve, enc, parameter_block);
      ecc_hash_update(&hash, enc, n);
      ecc_eddsa_field_to_enc(curve, enc, parameter_block + 2 * len);
      ecc_hash_update(&hash, enc, n);
      ARCH_DEP(kdsa_hash_message)(&hash, r2, regs);
      ecc_hash_final(&hash, khash);

      cc = ecc_eddsa_verify(curve, parameter_block, khash);

      /* The message has been processed completely */
      SET_GR_A(r2, regs, GR_A(r2, regs) + GR_A(r2 + 1, regs));
      SET_GR_A(r2 + 1, regs, 0);

      regs->psw.cc = cc;
      return;
    }
    case 40: /* eddsa-sign-ed25519 */
    case 44: /* eddsa-sign-ed448 */
    case 48: /* encrypted-eddsa-sign-ed25519 */
    case 52: /* encrypted-eddsa-sign-ed448 */
    {
      if(!msa9)
        ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);

      fc = GR0_fc(regs);
      encrypted = fc >= 48;
      curve = (fc == 40 || fc == 48) ? ECC_ED25519 : ECC_ED448;
      len = ecc_ecurve_fld(curve);
      n = ecc_ecurve_enclen(curve);

      /* The signature is stored into the first two fields. With an     */
      /* encrypted key the wrapping key verification pattern follows    */
      /* the key.                                                       */
      ARCH_DEP(validate_parameter_block)(2 * len, regs);
      ARCH_DEP(fetch_parameter_block)(parameter_block, 3 * len + (encrypted ? 32 : 0), regs);

      if(encrypted && unwrap_ecc(parameter_block + 2 * len, len))
      {
        /* Verification pattern mismatch */
        crypto_secure0(parameter_block, sizeof(parameter_block));
        regs->psw.cc = 1;
        return;
      }

      /* Secret scalar, prefix and public key from the seed, which is */
      /* right-aligned in the third field                             */
      cc = ECC_CC_INVALID;
      if(ecc_eddsa_sign_start(curve, &sign, parameter_block + 3 * len - n))
      {
        /* r = H(prefix || M) mod L, R = rB */
        ecc_hash_init(&hash, curve);
        ecc_hash_update(&hash, sign.prefix, n);
        ARCH_DEP(kdsa_hash_message)(&hash, r2, regs);
        ecc_hash_final(&hash, rhash);
        if(ecc_eddsa_sign_r(curve, &sign, rhash))
        {
          /* k = H(R || A || M), S = r + k*s mod L */
          ecc_hash_init(&hash, curve);
          ecc_hash_update(&hash, sign.r_enc, n);
          ecc_hash_update(&hash, sign.a_enc, n);
          ARCH_DEP(kdsa_hash_message)(&hash, r2, regs);
          ecc_hash_final(&hash, khash);
          if(ecc_eddsa_sign_finish(curve, &sign, khash, parameter_block))
            cc = ECC_CC_OK;
        }
      }

      if(cc == ECC_CC_OK)
      {
        ARCH_DEP(store_parameter_block)(parameter_block, 2 * len, regs);

        /* The message has been processed completely */
        SET_GR_A(r2, regs, GR_A(r2, regs) + GR_A(r2 + 1, regs));
        SET_GR_A(r2 + 1, regs, 0);
      }

      /* Do not leave the private key and derived secrets behind */
      crypto_secure0(&sign, sizeof(sign));
      crypto_secure0(rhash, sizeof(rhash));
      crypto_secure0(parameter_block, sizeof(parameter_block));

      regs->psw.cc = cc;
      return;
    }
#endif /* defined( FEATURE_155_MSA_EXTENSION_FACILITY_9 ) */

    default:
    {
      ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
      break;
    }
  }
}

#if defined( FEATURE_077_MSA_EXTENSION_FACILITY_4 )
/*----------------------------------------------------------------------------*/
/* B92D KMCTR - Cipher message with counter                           [RRF-b] */
/*----------------------------------------------------------------------------*/
DEF_INST(dyn_cipher_message_with_counter)
{
  int msa;
  BYTE query_bits[][16] =
  {
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0xf0, 0x70, 0x38, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
  };
  int r1;
  int r2;
  int r3;

  RRF_M(inst, regs, r1, r2, r3);
  PER_ZEROADDR_CHECK( regs, 1 );
  PER_ZEROADDR_CHECK2( regs, r1, r3 );
  PER_ZEROADDR_LCHECK( regs, r2, r2+1 );

  /* The following is the same as doing a FACILITY_CHECK */
  msa = get_msa(regs);
  if(msa < 4)
    ARCH_DEP(program_interrupt)(regs, PGM_OPERATION_EXCEPTION);

#ifdef OPTION_KMCTR_DEBUG
  WRMSG(HHC90100, "D", "KMCTR: cipher message with counter");
  WRMSG(HHC90101, "D", 1, r1);
  WRMSG(HHC90102, "D", regs->GR(r1));
  WRMSG(HHC90101, "D", 2, r2);
  WRMSG(HHC90102, "D", regs->GR(r2));
  WRMSG(HHC90103, "D", regs->GR(r2 + 1));
  WRMSG(HHC90101, "D", 3, r3);
  WRMSG(HHC90104, "D", 0, regs->GR(0));
  WRMSG(HHC90106, "D", GR0_fc(regs));
  WRMSG(HHC90104, "D", 1, regs->GR(1));
#endif /* #ifdef OPTION_KMCTR_DEBUG */

  /* Check special conditions */
  if(unlikely(!r1 || r1 & 0x01 || !r2 || r2 & 0x01 || !r3 || r3 & 0x01))
    ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);

  switch(GR0_fc(regs))
  {
    case 0: /* Query */
    {
      /* Store the parameter block */
      ARCH_DEP(vstorec)(query_bits[msa], 15, GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, regs);

#ifdef OPTION_KMCTR_DEBUG
      LOGBYTE("output:", query_bits[msa], 16);
#endif /* #ifdef OPTION_KMCTR_DEBUG */

      /* Set condition code 0 */
      regs->psw.cc = 0;
      return;
    }

    case 1: /* dea */
    case 2: /* tdea-128 */
    case 3: /* tdea-192 */
    case 9: /* encrypted dea */
    case 10: /* encrypted tdea-128 */
    case 11: /* encrypted tdea-192 */
    {
      if(msa >= 4)
        ARCH_DEP(kmctr_dea)(r1, r2, r3, regs);
      else
        ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
      break;
    }
    case 18: /* aes-128 */
    case 19: /* aes-192 */
    case 20: /* aes-256 */
    case 26: /* encrypted aes-128 */
    case 27: /* encrypted aes-192 */
    case 28: /* encrypted aes-256 */
    {
      if(msa >= 4)
        ARCH_DEP(kmctr_aes)(r1, r2, r3, regs);
      else
        ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
      break;
    }
    default:
    {
      ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
      break;
    }
  }
}

/*----------------------------------------------------------------------------*/
/* B92A KMF   - Cipher message with cipher feedback                     [RRE] */
/*----------------------------------------------------------------------------*/
DEF_INST(dyn_cipher_message_with_cipher_feedback)
{
  int msa;
  BYTE query_bits[][16] =
  {
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0xf0, 0x70, 0x38, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
  };
  int r1;
  int r2;

  RRE(inst, regs, r1, r2);

#if defined( FEATURE_PER_ZERO_ADDRESS_DETECTION_FACILITY )
    if (0
        || GR_A( 1,  regs ) == 0
        || GR_A( r1, regs ) == 0
        || GR_A( r2, regs ) == 0
    )
        ARCH_DEP( per3_zero )( regs );
#endif

  /* The following is the same as doing a FACILITY_CHECK */
  msa = get_msa(regs);
  if(msa < 4)
    ARCH_DEP(program_interrupt)(regs, PGM_OPERATION_EXCEPTION);

#ifdef OPTION_KMF_DEBUG
  WRMSG(HHC90100, "D", "KMF: cipher message with cipher feedback");
  WRMSG(HHC90101, "D", 1, r1);
  WRMSG(HHC90102, "D", regs->GR(r1));
  WRMSG(HHC90101, "D", 2, r2);
  WRMSG(HHC90102, "D", regs->GR(r2));
  WRMSG(HHC90103, "D", regs->GR(r2 + 1));
  WRMSG(HHC90104, "D", 0, regs->GR(0));
  WRMSG(HHC90112, "D", GR0_lcfb(regs));
  WRMSG(HHC90107, "D", TRUEFALSE(GR0_m(regs)));
  WRMSG(HHC90106, "D", GR0_fc(regs));
  WRMSG(HHC90104, "D", 1, regs->GR(1));
#endif /* #ifdef OPTION_KMF_DEBUG */

  /* Check special conditions */
  if(unlikely(!r1 || r1 & 0x01 || !r2 || r2 & 0x01))
    ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);

  switch(GR0_fc(regs))
  {
    case 0: /* Query */
    {
      /* Store the parameter block */
      ARCH_DEP(vstorec)(query_bits[msa], 15, GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, regs);

#ifdef OPTION_KMF_DEBUG
      LOGBYTE("output:", query_bits[msa], 16);
#endif /* #ifdef OPTION_KMF_DEBUG */

      /* Set condition code 0 */
      regs->psw.cc = 0;
      return;
    }

    case 1: /* dea */
    case 2: /* tdea-128 */
    case 3: /* tdea-192 */
    case 9: /* encrypted dea */
    case 10: /* encrypted tdea-128 */
    case 11: /* encrypted tdea-192 */
    {
      if(msa >= 4)
        ARCH_DEP(kmf_dea)(r1, r2, regs);
      else
        ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
      break;
    }
    case 18: /* aes-128 */
    case 19: /* aes-192 */
    case 20: /* aes-256 */
    case 26: /* encrypted aes-128 */
    case 27: /* encrypted aes-192 */
    case 28: /* encrypted aes-256 */
    {
      if(msa >= 4)
        ARCH_DEP(kmf_aes)(r1, r2, regs);
      else
        ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
      break;
    }
    default:
    {
      ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
      break;
    }
  }
}

/*----------------------------------------------------------------------------*/
/* B92B KMO   - Cipher message with output feedback                     [RRE] */
/*----------------------------------------------------------------------------*/
DEF_INST(dyn_cipher_message_with_output_feedback)
{
  int msa;
  BYTE query_bits[][16] =
  {
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0xf0, 0x70, 0x38, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
  };
  int r1;
  int r2;

  RRE(inst, regs, r1, r2);

#if defined( FEATURE_PER_ZERO_ADDRESS_DETECTION_FACILITY )
    if (0
        || GR_A( 1,  regs ) == 0
        || GR_A( r1, regs ) == 0
        || GR_A( r2, regs ) == 0
    )
        ARCH_DEP( per3_zero )( regs );
#endif

  /* The following is the same as doing a FACILITY_CHECK */
  msa = get_msa(regs);
  if(msa < 4)
    ARCH_DEP(program_interrupt)(regs, PGM_OPERATION_EXCEPTION);

#ifdef OPTION_KMO_DEBUG
  WRMSG(HHC90100, "D", "KMO: cipher message with output feedback");
  WRMSG(HHC90101, "D", 1, r1);
  WRMSG(HHC90102, "D", regs->GR(r1));
  WRMSG(HHC90101, "D", 2, r2);
  WRMSG(HHC90102, "D", regs->GR(r2));
  WRMSG(HHC90103, "D", regs->GR(r2 + 1));
  WRMSG(HHC90104, "D", 0, regs->GR(0));
  WRMSG(HHC90106, "D", GR0_fc(regs));
  WRMSG(HHC90104, "D", 1, regs->GR(1));
#endif /* #ifdef OPTION_KMO_DEBUG */

  /* Check special conditions */
  if(unlikely(!r1 || r1 & 0x01 || !r2 || r2 & 0x01))
    ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);

  switch(GR0_fc(regs))
  {
    case 0: /* Query */
    {
      /* Store the parameter block */
      ARCH_DEP(vstorec)(query_bits[msa], 15, GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, regs);

#ifdef OPTION_KMO_DEBUG
      LOGBYTE("output:", query_bits[msa], 16);
#endif /* #ifdef OPTION_KMO_DEBUG */

      /* Set condition code 0 */
      regs->psw.cc = 0;
      return;
    }
    case 1: /* dea */
    case 2: /* tdea-128 */
    case 3: /* tdea-192 */
    case 9: /* encrypted dea */
    case 10: /* encrypted tdea-128 */
    case 11: /* encrypted tdea-192 */
    {
      if(msa >= 4)
        ARCH_DEP(kmo_dea)(r1, r2, regs);
      else
        ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
      break;
    }
    case 18: /* aes-128 */
    case 19: /* aes-192 */
    case 20: /* aes-256 */
    case 26: /* encrypted aes-128 */
    case 27: /* encrypted aes-192 */
    case 28: /* encrypted aes-256 */
    {
      if(msa >= 4)
        ARCH_DEP(kmo_aes)(r1, r2, regs);
      else
        ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
      break;
    }
    default:
    {
      ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
      break;
    }
  }
}

#if defined( FEATURE_155_MSA_EXTENSION_FACILITY_9 )
/*----------------------------------------------------------------------------*/
/* Perform cryptographic computation (PCC) FC 64-66, 72-73 and 80-81          */
/*----------------------------------------------------------------------------*/
/* Scalar multiply (MSA 9). All values are right-aligned big-endian integers  */
/* in fields of 32 (P-256, Ed25519, X25519), 48 (P-384), 64 (Ed448, X448) or  */
/* 80 (P-521) bytes:                                                          */
/*                                                                            */
/*   64-66  P-256, P-384, P-521    result x, result y, source x, source y,    */
/*   72-73  Ed25519, Ed448         scalar (affine coordinates)                */
/*   80-81  X25519, X448           result u, source u, scalar                 */
/*                                                                            */
/* The scalar is used as given (X25519/X448 clamping is done by the program). */
/* Condition code 2 for an invalid source point or the point at infinity      */
/* (neutral element) as the result.                                           */
/*----------------------------------------------------------------------------*/
static void ARCH_DEP(pcc_scalar_multiply)(REGS *regs)
{
  BYTE parameter_block[5 * ECC_MAX_FIELD];
  int cc;
  int fc;
  int len;
  int fields;
  int results;

  fc = GR0_fc(regs);
  if(fc <= 66)
  {
    len = ecc_wcurve_len(fc - 64);
    fields = 5;
    results = 2;
  }
  else if(fc <= 73)
  {
    len = ecc_ecurve_fld(fc - 72);
    fields = 5;
    results = 2;
  }
  else
  {
    len = ecc_ecurve_fld(fc - 80);
    fields = 3;
    results = 1;
  }

  /* The result is stored into the first field(s) */
  ARCH_DEP(validate_parameter_block)(results * len, regs);
  ARCH_DEP(fetch_parameter_block)(parameter_block, fields * len, regs);

  if(fc <= 66)
    cc = ecc_w_scalar_multiply(fc - 64, parameter_block);
  else if(fc <= 73)
    cc = ecc_e_scalar_multiply(fc - 72, parameter_block);
  else
    cc = ecc_x_scalar_multiply(fc - 80, parameter_block);

  if(cc == ECC_CC_OK)
    ARCH_DEP(store_parameter_block)(parameter_block, results * len, regs);

  /* The scalar may be a private key */
  crypto_secure0(parameter_block, sizeof(parameter_block));

  regs->psw.cc = cc;
}
#endif /* defined( FEATURE_155_MSA_EXTENSION_FACILITY_9 ) */

/*----------------------------------------------------------------------------*/
/* B92C PCC   - Perform cryptographic computation                       [RRE] */
/*----------------------------------------------------------------------------*/
DEF_INST(dyn_perform_cryptographic_computation)
{
  int msa = get_msa(regs);
  static const BYTE query_bits[][16] =
  {
    { 0xf0, 0x70, 0x38, 0x38, 0x00, 0x00, 0x28, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
  };
  BYTE bits[16];

  UNREFERENCED(inst);              /* This operation has no operands */
  INST_UPDATE_PSW(regs, 4, 4);        /* All operands implied        */
  PER_ZEROADDR_CHECK( regs, 1 );

  /* The following is the same as doing a FACILITY_CHECK */
  if(msa < 4)
    ARCH_DEP(program_interrupt)(regs, PGM_OPERATION_EXCEPTION);
  else if (msa > 4) msa = 4;

#ifdef OPTION_PCC_DEBUG
  WRMSG(HHC90100, "D", "PCC: perform cryptographic computation");
  WRMSG(HHC90104, "D", 0, regs->GR(0));
  WRMSG(HHC90106, "D", GR0_fc(regs));
  WRMSG(HHC90104, "D", 1, regs->GR(1));
#endif /* #ifdef OPTION_PCC_DEBUG */

  switch(GR0_fc(regs))
  {
    case 0: /* Query */
      memcpy(bits, query_bits[msa - 4], 16);

#if defined( FEATURE_155_MSA_EXTENSION_FACILITY_9 )
      /* Scalar multiply P-256, P-384, P-521, Ed25519, Ed448, X25519, X448 (MSA 9) */
      if(FACILITY_ENABLED( 155_MSA_EXTENSION_9, regs ))
      {
        bits[8] |= 0xE0;
        bits[9] |= 0xC0;
        bits[10] |= 0xC0;
      }
#endif /* defined( FEATURE_155_MSA_EXTENSION_FACILITY_9 ) */

      /* Store the parameter block */
      ARCH_DEP(vstorec)(bits, 15, GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, regs);

#ifdef OPTION_PCC_DEBUG
      LOGBYTE("output:", bits, 16);
#endif /* #ifdef OPTION_PCC_DEBUG */

      /* Set condition code 0 */
      regs->psw.cc = 0;
      return;
    case  1: /* dea */
    case  2: /* tdea-128 */
    case  3: /* tdea-192 */
    case  9: /* encrypted dea */
    case 10: /* encrypted tdea-128 */
    case 11: /* encrypted tdea-192 */
      ARCH_DEP(pcc_cmac_dea)(regs);
      break;
    case 18: /* aes-128 */
    case 19: /* aes-192 */
    case 20: /* aes-256 */
    case 26: /* encrypted aes-128 */
    case 27: /* encrypted aes-192 */
    case 28: /* encrypted aes-256 */
      ARCH_DEP(pcc_cmac_aes)(regs);
      break;
    case 50: /* aes-128 */
    case 52: /* aes-256 */
    case 58: /* encrypted aes-128 */
    case 60: /* encrypted aes-256 */
      ARCH_DEP(pcc_xts_aes)(regs);
      break;

#if defined( FEATURE_155_MSA_EXTENSION_FACILITY_9 )
    case 64: /* scalar-multiply-p256 */
    case 65: /* scalar-multiply-p384 */
    case 66: /* scalar-multiply-p521 */
    case 72: /* scalar-multiply-ed25519 */
    case 73: /* scalar-multiply-ed448 */
    case 80: /* scalar-multiply-x25519 */
    case 81: /* scalar-multiply-x448 */
      if(FACILITY_ENABLED( 155_MSA_EXTENSION_9, regs ))
        ARCH_DEP(pcc_scalar_multiply)(regs);
      else
        ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
      break;
#endif /* defined( FEATURE_155_MSA_EXTENSION_FACILITY_9 ) */

    default:
      ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
      break;
  }
}
#endif /* defined( FEATURE_077_MSA_EXTENSION_FACILITY_4 ) */

#if defined( FEATURE_076_MSA_EXTENSION_FACILITY_3 )
#if defined( FEATURE_155_MSA_EXTENSION_FACILITY_9 )
/*----------------------------------------------------------------------------*/
/* Perform cryptographic key management operation (PCKMO) FC 32-34, 40-41     */
/*----------------------------------------------------------------------------*/
/* Encrypt-ECC-P256/P384/P521-Key and Encrypt-ECC-Ed25519/Ed448-Key (MSA 9).  */
/* The parameter block holds the clear private key in the format of the KDSA  */
/* sign parameter block (32, 48, 80, 32 or 64 bytes, right-aligned) followed  */
/* by 32 bytes for the AES wrapping key verification pattern. The key is      */
/* replaced by the wrapped key and the verification pattern is stored.        */
/*----------------------------------------------------------------------------*/
static void ARCH_DEP(pckmo_ecc)(REGS *regs)
{
  BYTE parameter_block[ECC_MAX_FIELD + 32];
  int keylen;

  switch(GR0_fc(regs))
  {
    case 32: keylen = 32; break;        /* P-256   */
    case 33: keylen = 48; break;        /* P-384   */
    case 34: keylen = 80; break;        /* P-521   */
    case 40: keylen = 32; break;        /* Ed25519 */
    default: keylen = 64; break;        /* Ed448   */
  }

  /* Test writeability */
  ARCH_DEP(validate_operand)(GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, keylen + 31, ACCTYPE_WRITE, regs);

  /* Fetch the key, wrap it and store the parameter block */
  ARCH_DEP(vfetchc)(parameter_block, keylen - 1, GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, regs);

#ifdef OPTION_PCKMO_DEBUG
  LOGBYTE("key in : ", parameter_block, keylen > 32 ? 32 : keylen);
#endif /* #ifdef OPTION_PCKMO_DEBUG */

  wrap_ecc(parameter_block, keylen);
  ARCH_DEP(vstorec)(parameter_block, keylen + 31, GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, regs);

#ifdef OPTION_PCKMO_DEBUG
  LOGBYTE("key out: ", parameter_block, keylen > 32 ? 32 : keylen);
  LOGBYTE("wkvp   : ", &parameter_block[keylen], 32);
#endif /* #ifdef OPTION_PCKMO_DEBUG */

  crypto_secure0(parameter_block, sizeof(parameter_block));
}
#endif /* defined( FEATURE_155_MSA_EXTENSION_FACILITY_9 ) */

/*----------------------------------------------------------------------------*/
/* B928 PCKMO - Perform cryptographic key management operation          [RRE] */
/*----------------------------------------------------------------------------*/
DEF_INST(dyn_perform_cryptographic_key_management_operation)
{
  int fc;
  int msa;
  BYTE query_bits[][16] =
  {
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0xf0, 0x00, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    /**/ { 0xf0, 0x00, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
  };
  int r1;
  int r2;

  RRE(inst, regs, r1, r2);
  PER_ZEROADDR_CHECK( regs, 1 );
  TXF_INSTR_CHECK( regs );

  /* The following is the same as doing a FACILITY_CHECK */
  msa = get_msa(regs);
  if(msa < 3)
    ARCH_DEP(program_interrupt)(regs, PGM_OPERATION_EXCEPTION);

#ifdef OPTION_PCKMO_DEBUG
  WRMSG(HHC90100, "D", "PCKMO: perform cryptographic key management operation");
  WRMSG(HHC90104, "D", 0, regs->GR(0));
  WRMSG(HHC90105, "D", TRUEFALSE(GR0_m(regs)));
  WRMSG(HHC90106, "D", GR0_fc(regs));
  WRMSG(HHC90104, "D", 1, regs->GR(1));
#endif /* #ifdef OPTION_PCKMO_DEBUG */

  /* Privileged operation */
  PRIV_CHECK(regs);

  /* Check special conditions */
  if(unlikely(GR0_m(regs)))
    ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);

  /* Initialize values */
  fc = GR0_fc(regs);
  switch(fc)
  {
    case 0: /* Query */
    {
#if defined( FEATURE_155_MSA_EXTENSION_FACILITY_9 )
      /* Encrypt-ECC-P256/P384/P521-Key, -Ed25519/Ed448-Key (MSA 9) */
      if(FACILITY_ENABLED( 155_MSA_EXTENSION_9, regs ))
      {
        query_bits[msa][4] |= 0xE0;
        query_bits[msa][5] |= 0xC0;
      }
#endif /* defined( FEATURE_155_MSA_EXTENSION_FACILITY_9 ) */

      /* Store the parameter block */
      ARCH_DEP(vstorec)(query_bits[msa], 15, GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, regs);

#ifdef OPTION_PCKMO_DEBUG
      LOGBYTE("output:", query_bits[msa], 16);
#endif /* #ifdef OPTION_PCKMO_DEBUG */

      return;
    }
    case 1: /* encrypt-dea */
    case 2: /* encrypt-tdea-128 */
    case 3: /* encrypt-tdea-192 */
    {
      if(msa >= 3)
        ARCH_DEP(pckmo_dea)(regs);
      else
        ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
      break;
    }
    case 18: /* encrypt-aes-128 */
    case 19: /* encrypt-aes-192 */
    case 20: /* encrypt-aes-256 */
    {
      if(msa >= 3)
        ARCH_DEP(pckmo_aes)(regs);
      else
        ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
      break;
    }

#if defined( FEATURE_155_MSA_EXTENSION_FACILITY_9 )
    case 32: /* encrypt-ecc-p256-key */
    case 33: /* encrypt-ecc-p384-key */
    case 34: /* encrypt-ecc-p521-key */
    case 40: /* encrypt-ecc-ed25519-key */
    case 41: /* encrypt-ecc-ed448-key */
    {
      if(FACILITY_ENABLED( 155_MSA_EXTENSION_9, regs ))
        ARCH_DEP(pckmo_ecc)(regs);
      else
        ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
      break;
    }
#endif /* defined( FEATURE_155_MSA_EXTENSION_FACILITY_9 ) */

    default:
    {
      ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
      break;
    }
  }
}
#endif /* defined( FEATURE_076_MSA_EXTENSION_FACILITY_3 ) */

#if defined( FEATURE_057_MSA_EXTENSION_FACILITY_5 )
/*----------------------------------------------------------------------------*/
/* PRNO SHA-512-DRNG: Hash_DRBG of NIST SP 800-90A with SHA-512               */
/*----------------------------------------------------------------------------*/
/* Parameter block (240 bytes):                                               */
/*                                                                            */
/*   offset  len  field                                                       */
/*   ------  ---  ---------------------------------------------------------   */
/*      0      4  reserved                                                    */
/*      4      4  reseed counter (zero before the first seed operation)      */
/*      8      8  stream bytes (number of bytes generated)                    */
/*     16      1  reserved                                                    */
/*     17    111  V (seedlen 888 bits)                                        */
/*    128      1  reserved                                                    */
/*    129    111  C                                                           */
/*                                                                            */
/* Seed (GR0 bit 56 one): the second operand is the seed material. With a     */
/* reseed counter of zero the DRBG is instantiated, V = Hash_df(material)     */
/* and the stream bytes are cleared, otherwise it is reseeded, V =            */
/* Hash_df(0x01 || V || material). Then C = Hash_df(0x00 || V) and the        */
/* reseed counter is set to one. The second operand is consumed.             */
/*                                                                            */
/* Generate (GR0 bit 56 zero): the first operand is filled with Hashgen(V),   */
/* then V = V + Hash(0x03 || V) + C + reseed counter (mod 2^888), the reseed  */
/* counter is incremented and the stream bytes are increased. A generate      */
/* request produces at most 64 KB (the SP 800-90A limit per request); for a  */
/* longer first operand condition code 3 is set and the program repeats the  */
/* instruction, which continues with the next request.                        */
/*                                                                            */
/* Verified against the NIST Hash_DRBG SHA-512 test vector (Count #0) of the  */
/* Linux s390 prng self test. This replaces a placeholder which recognized    */
/* one known-answer test and otherwise returned plain random bytes.           */
/*----------------------------------------------------------------------------*/
#define DRBG_SEEDLEN    111           /* 888 bits                          */
#define DRBG_MAX_GEN    65536         /* bytes per generate request        */

/* v = v + a (mod 2^888), a is a big-endian number of alen bytes */
static void ARCH_DEP(drbg_add)(BYTE *v, const BYTE *a, int alen)
{
  int carry = 0;
  int i;
  int k;

  for(i = DRBG_SEEDLEN - 1, k = alen - 1; i >= 0; i--, k--)
  {
    carry += v[i] + (k >= 0 ? a[k] : 0);
    v[i] = (BYTE) carry;
    carry >>= 8;
  }
}

/* out = Hash_df(prefix || second operand, 888 bits). The second operand is */
/* only included when r2 is not negative.                                   */
static void ARCH_DEP(drbg_hash_df)(BYTE *out, const BYTE *prefix, int prefixlen, int r2, REGS *regs)
{
  SHA2_CTX ctx;
  BYTE buf[256];
  BYTE digest[SHA512_DIGEST_LENGTH];
  BYTE hdr[5];
  U64 done;
  U64 len;
  int i;
  int n;

  for(i = 0; i * SHA512_DIGEST_LENGTH < DRBG_SEEDLEN; i++)
  {
    hdr[0] = (BYTE)(i + 1);                     /* counter                  */
    store_fw(&hdr[1], DRBG_SEEDLEN * 8);        /* number of bits to return */
    SHA512Init(&ctx);
    SHA512Update(&ctx, hdr, sizeof(hdr));
    if(prefixlen)
      SHA512Update(&ctx, prefix, prefixlen);
    if(r2 >= 0)
    {
      len = GR_A(r2 + 1, regs);
      for(done = 0; done < len; done += n)
      {
        n = (int) min((U64) sizeof(buf), len - done);
        ARCH_DEP(vfetchc)(buf, n - 1, (GR_A(r2, regs) + done) & ADDRESS_MAXWRAP(regs), r2, regs);
        SHA512Update(&ctx, buf, n);
      }
    }
    SHA512Final(digest, &ctx);
    memcpy(out + i * SHA512_DIGEST_LENGTH, digest,
           min(SHA512_DIGEST_LENGTH, DRBG_SEEDLEN - i * SHA512_DIGEST_LENGTH));
  }
  crypto_secure0(&ctx, sizeof(ctx));
  crypto_secure0(buf, sizeof(buf));
  crypto_secure0(digest, sizeof(digest));
}

static void ARCH_DEP(prno_sha512_drng)(int r1, int r2, REGS *regs)
{
  SHA2_CTX ctx;
  BYTE parameter_block[240];
  BYTE prefix[1 + DRBG_SEEDLEN];
  BYTE data[DRBG_SEEDLEN];
  BYTE out[256];
  BYTE digest[SHA512_DIGEST_LENGTH];
  BYTE counter[4];
  static const BYTE one[1] = { 1 };
  BYTE *v = &parameter_block[17];
  BYTE *c = &parameter_block[129];
  U32 rc;
  U64 sb;
  U64 done;
  U64 len;
  int k;
  int n;

  /* The parameter block is updated */
  ARCH_DEP(validate_operand)(GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, 239, ACCTYPE_WRITE, regs);
  ARCH_DEP(vfetchc)(parameter_block, 239, GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, regs);
  rc = fetch_fw(&parameter_block[4]);
  sb = fetch_dw(&parameter_block[8]);

#ifdef OPTION_PRNO_DEBUG
  LOGBYTE("V in  :", v, 32);
  LOGBYTE("C in  :", c, 32);
#endif /* #ifdef OPTION_PRNO_DEBUG */

  if(GR0_m(regs))
  {
    /* Seed: instantiate or reseed */
    if(!rc)
    {
      ARCH_DEP(drbg_hash_df)(v, NULL, 0, r2, regs);
      sb = 0;
    }
    else
    {
      prefix[0] = 0x01;
      memcpy(&prefix[1], v, DRBG_SEEDLEN);
      ARCH_DEP(drbg_hash_df)(v, prefix, sizeof(prefix), r2, regs);
    }
    prefix[0] = 0x00;
    memcpy(&prefix[1], v, DRBG_SEEDLEN);
    ARCH_DEP(drbg_hash_df)(c, prefix, sizeof(prefix), -1, regs);
    store_fw(&parameter_block[4], 1);
    store_dw(&parameter_block[8], sb);
    ARCH_DEP(vstorec)(parameter_block, 239, GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, regs);

    /* The seed material has been consumed */
    SET_GR_A(r2, regs, GR_A(r2, regs) + GR_A(r2 + 1, regs));
    SET_GR_A(r2 + 1, regs, 0);
    regs->psw.cc = 0;
  }
  else
  {
    /* Generate: Hashgen output, stored in chunks of up to 256 bytes. An */
    /* access exception leaves the state unchanged (restartable).        */
    len = min((U64) GR_A(r1 + 1, regs), (U64) DRBG_MAX_GEN);
    memcpy(data, v, DRBG_SEEDLEN);
    for(done = 0; done < len; done += n)
    {
      n = (int) min((U64) sizeof(out), len - done);
      for(k = 0; k < n; k += SHA512_DIGEST_LENGTH)
      {
        SHA512Init(&ctx);
        SHA512Update(&ctx, data, DRBG_SEEDLEN);
        SHA512Final(digest, &ctx);
        memcpy(&out[k], digest, min(SHA512_DIGEST_LENGTH, n - k));
        ARCH_DEP(drbg_add)(data, one, sizeof(one));
      }
      ARCH_DEP(vstorec)(out, n - 1, (GR_A(r1, regs) + done) & ADDRESS_MAXWRAP(regs), r1, regs);
    }

    /* V = V + Hash(0x03 || V) + C + reseed counter */
    prefix[0] = 0x03;
    memcpy(&prefix[1], v, DRBG_SEEDLEN);
    SHA512Init(&ctx);
    SHA512Update(&ctx, prefix, sizeof(prefix));
    SHA512Final(digest, &ctx);
    store_fw(counter, rc);
    ARCH_DEP(drbg_add)(v, digest, sizeof(digest));
    ARCH_DEP(drbg_add)(v, c, DRBG_SEEDLEN);
    ARCH_DEP(drbg_add)(v, counter, sizeof(counter));
    store_fw(&parameter_block[4], rc + 1);
    store_dw(&parameter_block[8], sb + len);
    ARCH_DEP(vstorec)(parameter_block, 239, GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, regs);

    SET_GR_A(r1, regs, GR_A(r1, regs) + len);
    SET_GR_A(r1 + 1, regs, GR_A(r1 + 1, regs) - len);
    regs->psw.cc = GR_A(r1 + 1, regs) ? 3 : 0;
  }

#ifdef OPTION_PRNO_DEBUG
  LOGBYTE("V out :", v, 32);
  LOGBYTE("C out :", c, 32);
#endif /* #ifdef OPTION_PRNO_DEBUG */

  /* Do not leave the DRBG state behind */
  crypto_secure0(&ctx, sizeof(ctx));
  crypto_secure0(parameter_block, sizeof(parameter_block));
  crypto_secure0(prefix, sizeof(prefix));
  crypto_secure0(data, sizeof(data));
  crypto_secure0(out, sizeof(out));
  crypto_secure0(digest, sizeof(digest));
}

/*----------------------------------------------------------------------------*/
/* B93C PRNO - Perform random number operation                          [RRE] */
/*----------------------------------------------------------------------------*/
DEF_INST(dyn_perform_random_number_operation)
{
  int fc;
  int msa;
  U64 randbytes;
  U64 randnum;
  U64 randaddr;
#define         RAND_CHUNK_SIZE     256
  BYTE randbuf[ RAND_CHUNK_SIZE ] = {0};
  BYTE query_bits[16] = { 0x90, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x00 };
  int r1;
  int r2;
  /* TRNG Query Raw to Conditioned Ratio -- hardcoded for now */
  BYTE trng_query_bits[8] = { 0x00, 0x00, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x20 };

  RRE(inst, regs, r1, r2);
  PER_ZEROADDR_CHECK( regs, 1 );
  TXF_INSTR_CHECK( regs );

  msa = get_msa(regs);
  if (FACILITY_ENABLED( 057_MSA_EXTENSION_5,  regs ))
    msa = 5;
  if (msa < 5)
    ARCH_DEP(program_interrupt)(regs, PGM_OPERATION_EXCEPTION);

#ifdef OPTION_PRNO_DEBUG
  WRMSG(HHC90100, "D", "PRNO: perform random number operation");
  WRMSG(HHC90104, "D", 0, regs->GR(0));
  WRMSG(HHC90105, "D", TRUEFALSE(GR0_m(regs)));
  WRMSG(HHC90106, "D", GR0_fc(regs));
  WRMSG(HHC90104, "D", 1, regs->GR(1));
  WRMSG(HHC90104, "D", r1, regs->GR(r1));
  WRMSG(HHC90104, "D", r1 + 1, regs->GR(r1+1));
  WRMSG(HHC90104, "D", r2, regs->GR(r2));
  WRMSG(HHC90104, "D", r2 + 1, regs->GR(r2+1));
#endif

  if (!sysblk.PRNOrandhand)
    VERIFY( hopen_CSRNG( &sysblk.PRNOrandhand ) );

  /* Initialize values */
  fc = GR0_fc(regs);
  switch(fc)
  {
    case 0: /* Query */
    {
      ARCH_DEP(vstorec)(query_bits, 15, GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, regs);
      /* Set condition code 0 */
      regs->psw.cc = 0;
      return;
    }
    case 3: /* SHA-512-DRNG */
    {
      ARCH_DEP(prno_sha512_drng)(r1, r2, regs);
      return;
    }
    case 112: /* TRNG-Query-Raw-to-Conditioned-Ratio */
    {
      /* Store the parameter block -- hardcoded for now */
      ARCH_DEP(vstorec)(trng_query_bits, 7, GR_A(1, regs) & ADDRESS_MAXWRAP(regs), 1, regs);
      /* Set condition code 0 */
      regs->psw.cc = 0;
      return;
    }
    case 114: /* TRNG */
    {
      if (GR_A( r1 + 1, regs ) != 0 )
      {
        randaddr  = GR_A( r1,     regs );
        randbytes = GR_A( r1 + 1, regs );
#ifdef OPTION_PRNO_DEBUG
        LOGMSG("+++ %s mode TRNG Generate: r1+1 randbytes = %"PRIu64"\n", regs->psw.amode64 ? "64" : regs->psw.amode ? "31" : "24", randbytes );
#endif
        do
        {
          randnum = min( RAND_CHUNK_SIZE, randbytes );
          VERIFY( hget_random_bytes( randbuf, randnum, &sysblk.PRNOrandhand ));
          ARCH_DEP(vstorec)(randbuf, randnum - 1, randaddr & ADDRESS_MAXWRAP(regs), 1, regs);
          if (randbytes > RAND_CHUNK_SIZE)
          {
            randbytes -=  RAND_CHUNK_SIZE;
            randaddr  +=  RAND_CHUNK_SIZE;
          }
          else randbytes = 0;
        }
        while (randbytes);

        SET_GR_A( r1,     regs, regs->GR(r1) + GR_A(r1 + 1, regs));
        SET_GR_A( r1 + 1, regs, 0 );
      }
      if (GR_A(r2 + 1, regs) != 0)
      {
        randaddr  = GR_A( r2,     regs);
        randbytes = GR_A( r2 + 1, regs );
#ifdef OPTION_PRNO_DEBUG
        LOGMSG("+++ %s mode TRNG Generate: r2+1 randbytes = %"PRIu64"\n", regs->psw.amode64 ? "64" : regs->psw.amode ? "31" : "24", randbytes );
#endif
        do
        {
          randnum = min( RAND_CHUNK_SIZE, randbytes );
          VERIFY( hget_random_bytes( randbuf, randnum, &sysblk.PRNOrandhand ));
          ARCH_DEP(vstorec)(randbuf, randnum - 1, randaddr & ADDRESS_MAXWRAP(regs), 1, regs);
          if (randbytes > RAND_CHUNK_SIZE)
          {
            randbytes -=  RAND_CHUNK_SIZE;
            randaddr  +=  RAND_CHUNK_SIZE;
          }
          else randbytes = 0;
        }
        while (randbytes);

        SET_GR_A( r2,     regs, regs->GR(r2) + GR_A(r2 + 1, regs));
        SET_GR_A( r2 + 1, regs, 0);
      }
      /* Set condition code 0 */
      regs->psw.cc = 0;
      return;
    }
    default:
    {
      ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
      break;
    }
  }
}
#endif /* defined( FEATURE_057_MSA_EXTENSION_FACILITY_5 ) */

#endif /* defined( FEATURE_017_MSA_FACILITY ) */

/*----------------------------------------------------------------------------*/
/*   Program Check Operation Exception if facility not enabled for arch       */
/*----------------------------------------------------------------------------*/

#if !defined( FEATURE_017_MSA_FACILITY )
 HDL_UNDEF_INST( dyn_cipher_message                      )
 HDL_UNDEF_INST( dyn_cipher_message_with_chaining        )
 HDL_UNDEF_INST( dyn_compute_intermediate_message_digest )
 HDL_UNDEF_INST( dyn_compute_last_message_digest         )
 HDL_UNDEF_INST( dyn_compute_message_authentication_code )
 HDL_UNDEF_INST( dyn_cipher_message_with_authentication  )
 HDL_UNDEF_INST( dyn_compute_digital_signature_authentication )
#endif

#if !defined( FEATURE_076_MSA_EXTENSION_FACILITY_3 )
 HDL_UNDEF_INST( dyn_perform_cryptographic_key_management_operation )
#endif

#if !defined( FEATURE_077_MSA_EXTENSION_FACILITY_4 )
 HDL_UNDEF_INST( dyn_perform_cryptographic_computation   )
 HDL_UNDEF_INST( dyn_cipher_message_with_cipher_feedback )
 HDL_UNDEF_INST( dyn_cipher_message_with_output_feedback )
 HDL_UNDEF_INST( dyn_cipher_message_with_counter         )
#endif

#if !defined( FEATURE_057_MSA_EXTENSION_FACILITY_5 )
 HDL_UNDEF_INST( dyn_perform_random_number_operation )
#endif

/*-------------------------------------------------------------------*/
/*          (delineates ARCH_DEP from non-arch_dep)                  */
/*-------------------------------------------------------------------*/

#if !defined( _GEN_ARCH )

  #if defined(              _ARCH_NUM_1 )
    #define   _GEN_ARCH     _ARCH_NUM_1
    #include "dyncrypt.c"
  #endif

  #if defined(              _ARCH_NUM_2 )
    #undef    _GEN_ARCH
    #define   _GEN_ARCH     _ARCH_NUM_2
    #include "dyncrypt.c"
  #endif

/*-------------------------------------------------------------------*/
/*          (delineates ARCH_DEP from non-arch_dep)                  */
/*-------------------------------------------------------------------*/

HDL_DEPENDENCY_SECTION;
{
   HDL_DEPENDENCY(HERCULES);
   HDL_DEPENDENCY(REGS);
// HDL_DEPENDENCY(DEVBLK);
   HDL_DEPENDENCY(SYSBLK);
// HDL_DEPENDENCY(WEBBLK);
}
END_DEPENDENCY_SECTION;

HDL_INSTRUCTION_SECTION;
{
    // (allows for a much shorter HDL_INST statement)

#define HDL_INST            HDL_DEF_INST
#define ARCH_____390_900                       HDL_INSTARCH_390 | HDL_INSTARCH_900
#define ARCH_________900                                          HDL_INSTARCH_900
#define ARCH_370_390_900    HDL_INSTARCH_370 | HDL_INSTARCH_390 | HDL_INSTARCH_900
#define ARCH_370_____900    HDL_INSTARCH_370                    | HDL_INSTARCH_900
#define OPCODE( _opcode )   0x ## _opcode

  /* Install our instructions for the architectures we support */

#if defined( _FEATURE_017_MSA_FACILITY )
  HDL_INST( ARCH_________900, OPCODE( B929 ), dyn_cipher_message_with_authentication );
  HDL_INST( ARCH_________900, OPCODE( B93A ), dyn_compute_digital_signature_authentication );
  #if !defined( _FEATURE_370_EXTENSION )
  HDL_INST( ARCH_____390_900, OPCODE( B93E ), dyn_compute_intermediate_message_digest );
  HDL_INST( ARCH_____390_900, OPCODE( B93F ), dyn_compute_last_message_digest         );
  HDL_INST( ARCH_____390_900, OPCODE( B92E ), dyn_cipher_message                      );
  HDL_INST( ARCH_____390_900, OPCODE( B91E ), dyn_compute_message_authentication_code );
  HDL_INST( ARCH_____390_900, OPCODE( B92F ), dyn_cipher_message_with_chaining        );
  #else
  HDL_INST( ARCH_370_390_900, OPCODE( B93E ), dyn_compute_intermediate_message_digest );
  HDL_INST( ARCH_370_390_900, OPCODE( B93F ), dyn_compute_last_message_digest         );
  HDL_INST( ARCH_370_390_900, OPCODE( B92E ), dyn_cipher_message                      );
  HDL_INST( ARCH_370_390_900, OPCODE( B91E ), dyn_compute_message_authentication_code );
  HDL_INST( ARCH_370_390_900, OPCODE( B92F ), dyn_cipher_message_with_chaining        );
  #endif
#endif

#if defined( _FEATURE_076_MSA_EXTENSION_FACILITY_3 )
  #if !defined( _FEATURE_370_EXTENSION )
  HDL_INST( ARCH_________900, OPCODE( B928 ), dyn_perform_cryptographic_key_management_operation );
  #else
  HDL_INST( ARCH_370_____900, OPCODE( B928 ), dyn_perform_cryptographic_key_management_operation );
  #endif
#endif

#if defined( _FEATURE_077_MSA_EXTENSION_FACILITY_4 )
  #if !defined( _FEATURE_370_EXTENSION )
  HDL_INST( ARCH_________900, OPCODE( B92D ), dyn_cipher_message_with_counter         );
  HDL_INST( ARCH_________900, OPCODE( B92A ), dyn_cipher_message_with_cipher_feedback );
  HDL_INST( ARCH_________900, OPCODE( B92B ), dyn_cipher_message_with_output_feedback );
  HDL_INST( ARCH_________900, OPCODE( B92C ), dyn_perform_cryptographic_computation   );
  #else
  HDL_INST( ARCH_370_____900, OPCODE( B92D ), dyn_cipher_message_with_counter         );
  HDL_INST( ARCH_370_____900, OPCODE( B92A ), dyn_cipher_message_with_cipher_feedback );
  HDL_INST( ARCH_370_____900, OPCODE( B92B ), dyn_cipher_message_with_output_feedback );
  HDL_INST( ARCH_370_____900, OPCODE( B92C ), dyn_perform_cryptographic_computation   );
  #endif
#endif

#if defined( _FEATURE_057_MSA_EXTENSION_FACILITY_5 )
  #if !defined( _FEATURE_370_EXTENSION )
  HDL_INST( ARCH_________900, OPCODE( B93C ), dyn_perform_random_number_operation );
  #else
  HDL_INST( ARCH_370_____900, OPCODE( B93C ), dyn_perform_random_number_operation );
  #endif
#endif
}
END_INSTRUCTION_SECTION;

HDL_REGISTER_SECTION;
{
  /* Display copyright, version and level of support information */

  UNREFERENCED( regsym );   // (HDL_REGISTER_SECTION parameter)

  // "%s module loaded%s"
  WRMSG( HHC00150, "I", "Crypto", " (C) Copyright 2003-2026 by Bernard van der Helm, Juergen Winkelmann & Mike Grossmann");

  // "Activated facility: %s"
  WRMSG( HHC00151, "I", "Message Security Assist");

#if defined( _FEATURE_155_MSA_EXTENSION_FACILITY_9 ) && defined( _FEATURE_146_MSA_EXTENSION_FACILITY_8 ) && defined( _FEATURE_057_MSA_EXTENSION_FACILITY_5 )
  WRMSG( HHC00151, "I", "Message Security Assist Extension 1, 2, 3, 4, 5, 6, 7, 8 and 9");
#elif defined( _FEATURE_146_MSA_EXTENSION_FACILITY_8 ) && defined( _FEATURE_057_MSA_EXTENSION_FACILITY_5 )
  WRMSG( HHC00151, "I", "Message Security Assist Extension 1, 2, 3, 4, 5, 6, 7 and 8");
#elif defined( _FEATURE_057_MSA_EXTENSION_FACILITY_5 )
  WRMSG( HHC00151, "I", "Message Security Assist Extension 1, 2, 3, 4, 5, 6 and 7");
#else
  #if defined( _FEATURE_077_MSA_EXTENSION_FACILITY_4 )
    WRMSG( HHC00151, "I", "Message Security Assist Extension 1, 2, 3 and 4");
  #else
    #if defined( _FEATURE_076_MSA_EXTENSION_FACILITY_3 )
      WRMSG( HHC00151, "I", "Message Security Assist Extension 1, 2 and 3");
    #else
      #if defined( _FEATURE_MSA_EXTENSION_FACILITY_2 )
        WRMSG( HHC00151, "I", "Message Security Assist Extension 1 and 2");
      #else
        #if defined( _FEATURE_MSA_EXTENSION_FACILITY_1 )
          WRMSG( HHC00151, "I", "Message Security Assist Extension 1");
        #endif /* defined( _FEATURE_MSA_EXTENSION_FACILITY_1 ) */
      #endif /* defined( _FEATURE_MSA_EXTENSION_FACILITY_2 ) */
    #endif /* defined( _FEATURE_076_MSA_EXTENSION_FACILITY_3 ) */
  #endif /* defined( _FEATURE_077_MSA_EXTENSION_FACILITY_4 ) */
#endif /* defined( _FEATURE_057_MSA_EXTENSION_FACILITY_5 ) */
}
END_REGISTER_SECTION;

#endif /* #ifdef _GEN_ARCH */
