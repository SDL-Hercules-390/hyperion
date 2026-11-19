/* DYNCRYPT_ECC.H                                                    */
/*              Elliptic curve support for the dyncrypt module       */
/*              (Message-Security-Assist Extension 9)                */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*----------------------------------------------------------------------------*/
/* This file is NOT a standalone header. It is included exactly once by       */
/* dyncrypt.c, in its section of architecture independent static functions.  */
/* It contains the elliptic curve arithmetic used by KDSA and by the scalar   */
/* multiply functions of PCC. The multiple-precision arithmetic is provided   */
/* by LibTomMath (public domain, see libtommath/README.hercules.txt), which   */
/* is compiled as part of this file with only the functions needed here.     */
/*                                                                            */
/* All functions work on the parameter block contents as fetched from guest   */
/* storage: big-endian integers, right-aligned (zero-padded on the left) in   */
/* fields of the curve's parameter block field length. They return the       */
/* condition code to be set by the instruction:                               */
/*                                                                            */
/*   0  normal completion                                                     */
/*   1  invalid input parameter (e.g. private key out of range, public key    */
/*      not on the curve)                                                     */
/*   2  signature verification failed, unusable random number (sign) or      */
/*      invalid point / point at infinity (scalar multiply)                   */
/*----------------------------------------------------------------------------*/

#ifndef _DYNCRYPT_ECC_H_
#define _DYNCRYPT_ECC_H_

/*----------------------------------------------------------------------------*/
/* LibTomMath, configured with only the functions needed (dependencies are    */
/* resolved by tommath_class.h)                                               */
/*----------------------------------------------------------------------------*/
#define LTM_NOTHING
#define BN_CUTOFFS_C            /* not pulled in by the dependency lists */
#define BN_MP_RADIX_SMAP_C      /* not pulled in by the dependency lists */
#define BN_MP_ADD_C
#define BN_MP_ADDMOD_C
#define BN_MP_CLEAR_C
#define BN_MP_CLEAR_MULTI_C
#define BN_MP_CMP_C
#define BN_MP_CMP_MAG_C
#define BN_MP_EXCH_C
#define BN_MP_EXPTMOD_C
#define BN_MP_COPY_C
#define BN_MP_FROM_UBIN_C
#define BN_MP_INIT_C
#define BN_MP_INIT_MULTI_C
#define BN_MP_INVMOD_C
#define BN_MP_ISODD_C
#define BN_MP_MOD_C
#define BN_MP_MONTGOMERY_CALC_NORMALIZATION_C
#define BN_MP_MONTGOMERY_REDUCE_C
#define BN_MP_MONTGOMERY_SETUP_C
#define BN_MP_MUL_C
#define BN_MP_MULMOD_C
#define BN_MP_READ_RADIX_C
#define BN_MP_SET_C
#define BN_MP_SQR_C
#define BN_MP_SQRMOD_C
#define BN_MP_SUB_C
#define BN_MP_SUBMOD_C
#define BN_MP_TO_UBIN_C
#define BN_MP_UBIN_SIZE_C
#define BN_MP_ZERO_C

#include "libtommath/mpi.c"

/*----------------------------------------------------------------------------*/
/* Condition codes returned by the elliptic curve functions                   */
/*----------------------------------------------------------------------------*/
#define ECC_CC_OK          0    /* normal completion                         */
#define ECC_CC_INVALID     1    /* invalid input parameter                   */
#define ECC_CC_FAILED      2    /* verification failed, unusable random     */
                                /* number or point at infinity              */

/* Largest parameter block field length (P-521) */
#define ECC_MAX_FIELD     80

/* Error check for LibTomMath calls: an error (out of memory) is reported as */
/* an invalid parameter, there is no better architected way to report it.   */
#define ECC_CHK(_f)  do { if((_f) != MP_OKAY) goto ecc_error; } while(0)

/* Error check for the boolean helper functions below */
#define ECC_T(_f)    do { if(!(_f)) goto ecc_error; } while(0)

/* Load a parameter block field into an mp_int, error exit as ECC_CHK */
#define ECC_LOAD(_a, _src, _len)  do { if(!ecc_load((_a), (_src), (_len))) goto ecc_error; } while(0)

/*----------------------------------------------------------------------------*/
/* Short Weierstrass curves y^2 = x^3 - 3x + b over GF(p) (NIST P-256, P-384, */
/* P-521). All of them have a = -3, which the doubling formula relies on.     */
/*----------------------------------------------------------------------------*/
typedef struct ECC_WCURVE_DEF
{
  int         len;      /* parameter block field length in bytes         */
  const char *p;        /* prime modulus                                 */
  const char *b;        /* curve coefficient b                           */
  const char *n;        /* order of the base point                       */
  const char *gx;       /* base point x                                  */
  const char *gy;       /* base point y                                  */
} ECC_WCURVE_DEF;

#define ECC_P256  0
#define ECC_P384  1
#define ECC_P521  2

static const ECC_WCURVE_DEF ecc_wcurves[3] =
{
  {   /* P-256 */
    32,
    "FFFFFFFF00000001000000000000000000000000FFFFFFFFFFFFFFFFFFFFFFFF",
    "5AC635D8AA3A93E7B3EBBD55769886BC651D06B0CC53B0F63BCE3C3E27D2604B",
    "FFFFFFFF00000000FFFFFFFFFFFFFFFFBCE6FAADA7179E84F3B9CAC2FC632551",
    "6B17D1F2E12C4247F8BCE6E563A440F277037D812DEB33A0F4A13945D898C296",
    "4FE342E2FE1A7F9B8EE7EB4A7C0F9E162BCE33576B315ECECBB6406837BF51F5",
  },
  {   /* P-384 */
    48,
    "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFE"
      "FFFFFFFF0000000000000000FFFFFFFF",
    "B3312FA7E23EE7E4988E056BE3F82D19181D9C6EFE8141120314088F5013875A"
      "C656398D8A2ED19D2A85C8EDD3EC2AEF",
    "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFC7634D81F4372DDF"
      "581A0DB248B0A77AECEC196ACCC52973",
    "AA87CA22BE8B05378EB1C71EF320AD746E1D3B628BA79B9859F741E082542A38"
      "5502F25DBF55296C3A545E3872760AB7",
    "3617DE4A96262C6F5D9E98BF9292DC29F8F41DBD289A147CE9DA3113B5F0B8C0"
      "0A60B1CE1D7E819D7A431D7C90EA0E5F",
  },
  {   /* P-521, note that the field length (80) exceeds the element size (66) */
    80,
    "1FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
      "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
      "FFF",
    "51953EB9618E1C9A1F929A21A0B68540EEA2DA725B99B315F3B8B489918EF109"
      "E156193951EC7E937B1652C0BD3BB1BF073573DF883D2C34F1EF451FD46B503F"
      "00",
    "1FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
      "FFA51868783BF2F966B7FCC0148F709A5D03BB5C9B8899C47AEBB6FB71E91386"
      "409",
    "C6858E06B70404E9CD9E3ECB662395B4429C648139053FB521F828AF606B4D3D"
      "BAA14B5E77EFE75928FE1DC127A2FFA8DE3348B3C1856A429BF97E7E31C2E5BD"
      "66",
    "11839296A789A3BC0045C8A5FB42C7D1BD998F54449579B446817AFBD17273E6"
      "62C97EE72995EF42640C550B9013FAD0761353C7086A272C24088BE94769FD16"
      "650",
  },
};

/* Curve in working form */
typedef struct ECC_WCURVE
{
  int      len;
  mp_int   p, b, n, gx, gy;
  mp_int   one;                 /* R mod p, the Montgomery form of 1     */
  mp_digit rho;                 /* Montgomery constant -1/p mod 2^digit  */
  mp_int   t[12];               /* temporaries of the point arithmetic   */
} ECC_WCURVE;

/* Point in Jacobian coordinates (x/z^2, y/z^3), z = 0 is the point at      */
/* infinity. The coordinates are kept in Montgomery form (see below).       */
typedef struct ECC_JPOINT
{
  mp_int x, y, z;
} ECC_JPOINT;

/*----------------------------------------------------------------------------*/
/* Load and store parameter block fields                                      */
/*----------------------------------------------------------------------------*/
static int ecc_load(mp_int *a, const BYTE *src, int len)
{
  return(mp_from_ubin(a, src, (size_t) len) == MP_OKAY);
}

static int ecc_store(BYTE *dst, int len, const mp_int *a)
{
  size_t size;

  size = mp_ubin_size(a);
  if(size > (size_t) len)
    return(0);
  memset(dst, 0, len);
  return(mp_to_ubin(a, dst + len - size, size, NULL) == MP_OKAY);
}

/*----------------------------------------------------------------------------*/
/* Field arithmetic in Montgomery form (a*R mod p) for the point arithmetic.  */
/* A Montgomery reduction needs neither a division nor memory allocations,    */
/* unlike a general modular reduction (mp_mulmod). Point coordinates are      */
/* kept in this form; points are converted when they are set and when they   */
/* are converted to affine coordinates. All functions return 1 on success.   */
/*----------------------------------------------------------------------------*/
static int ecc_fmul(mp_int *r, const mp_int *a, const mp_int *b, const mp_int *p, mp_digit rho)
{
  return(mp_mul(a, b, r) == MP_OKAY && mp_montgomery_reduce(r, p, rho) == MP_OKAY);
}

static int ecc_fsqr(mp_int *r, const mp_int *a, const mp_int *p, mp_digit rho)
{
  return(mp_sqr(a, r) == MP_OKAY && mp_montgomery_reduce(r, p, rho) == MP_OKAY);
}

/* r = a + b (mod p) for 0 <= a, b < p */
static int ecc_fadd(mp_int *r, const mp_int *a, const mp_int *b, const mp_int *p)
{
  if(mp_add(a, b, r) != MP_OKAY)
    return(0);
  if(mp_cmp_mag(r, p) != MP_LT)
    return(mp_sub(r, p, r) == MP_OKAY);
  return(1);
}

/* r = a - b (mod p) for 0 <= a, b < p */
static int ecc_fsub(mp_int *r, const mp_int *a, const mp_int *b, const mp_int *p)
{
  if(mp_sub(a, b, r) != MP_OKAY)
    return(0);
  if(mp_isneg(r))
    return(mp_add(r, p, r) == MP_OKAY);
  return(1);
}

/* r = -a (mod p) for 0 <= a < p */
static int ecc_fneg(mp_int *r, const mp_int *a, const mp_int *p)
{
  if(mp_iszero(a))
  {
    mp_zero(r);
    return(1);
  }
  return(mp_sub(p, a, r) == MP_OKAY);
}

/* r = a*R mod p (one = R mod p), a in normal form, 0 <= a < p */
static int ecc_to_mont(mp_int *r, const mp_int *a, const mp_int *one, const mp_int *p)
{
  return(mp_mulmod(a, one, p, r) == MP_OKAY);
}

/* r = a/R mod p */
static int ecc_from_mont(mp_int *r, const mp_int *a, const mp_int *p, mp_digit rho)
{
  return(mp_copy(a, r) == MP_OKAY && mp_montgomery_reduce(r, p, rho) == MP_OKAY);
}

/* Shorthands for the point arithmetic of a curve c */
#define FMUL(_r, _a, _b)  ECC_T(ecc_fmul((_r), (_a), (_b), &c->p, c->rho))
#define FSQR(_r, _a)      ECC_T(ecc_fsqr((_r), (_a), &c->p, c->rho))
#define FADD(_r, _a, _b)  ECC_T(ecc_fadd((_r), (_a), (_b), &c->p))
#define FSUB(_r, _a, _b)  ECC_T(ecc_fsub((_r), (_a), (_b), &c->p))

/*----------------------------------------------------------------------------*/
/* Curve setup and cleanup                                                    */
/*----------------------------------------------------------------------------*/
/* The structure is cleared first, ecc_wcurve_clear() may always be called, */
/* also after a failing ecc_wcurve_init() (mp_clear() ignores cleared ints). */
static int ecc_wcurve_init(ECC_WCURVE *c, int curve)
{
  const ECC_WCURVE_DEF *def = &ecc_wcurves[curve];
  int i;

  memset(c, 0, sizeof(*c));
  c->len = def->len;
  if(mp_init_multi(&c->p, &c->b, &c->n, &c->gx, &c->gy, &c->one, NULL) != MP_OKAY)
    return(0);
  for(i = 0; i < 12; i++)
  {
    if(mp_init(&c->t[i]) != MP_OKAY)
      return(0);
  }
  if(mp_read_radix(&c->p, def->p, 16) != MP_OKAY
  || mp_read_radix(&c->b, def->b, 16) != MP_OKAY
  || mp_read_radix(&c->n, def->n, 16) != MP_OKAY
  || mp_read_radix(&c->gx, def->gx, 16) != MP_OKAY
  || mp_read_radix(&c->gy, def->gy, 16) != MP_OKAY
  || mp_montgomery_setup(&c->p, &c->rho) != MP_OKAY
  || mp_montgomery_calc_normalization(&c->one, &c->p) != MP_OKAY)
    return(0);
  return(1);
}

static void ecc_wcurve_clear(ECC_WCURVE *c)
{
  int i;

  for(i = 0; i < 12; i++)
    mp_clear(&c->t[i]);
  mp_clear_multi(&c->p, &c->b, &c->n, &c->gx, &c->gy, &c->one, NULL);
}

static int ecc_jpoint_init(ECC_JPOINT *P)
{
  memset(P, 0, sizeof(*P));
  return(mp_init_multi(&P->x, &P->y, &P->z, NULL) == MP_OKAY);
}

static void ecc_jpoint_clear(ECC_JPOINT *P)
{
  mp_clear_multi(&P->x, &P->y, &P->z, NULL);
}

static int ecc_jpoint_copy(ECC_JPOINT *R, const ECC_JPOINT *P)
{
  return(mp_copy(&P->x, &R->x) == MP_OKAY && mp_copy(&P->y, &R->y) == MP_OKAY
      && mp_copy(&P->z, &R->z) == MP_OKAY);
}

/* Set P to the point at infinity (z = 0, x and y do not matter) */
static void ecc_jpoint_infinity(ECC_WCURVE *c, ECC_JPOINT *P)
{
  UNREFERENCED(c);
  mp_set(&P->x, 1);
  mp_set(&P->y, 1);
  mp_zero(&P->z);
}

/* Set P to the affine point (x, y), converted to Montgomery form */
static int ecc_jpoint_set(ECC_WCURVE *c, ECC_JPOINT *P, const mp_int *x, const mp_int *y)
{
  return(ecc_to_mont(&P->x, x, &c->one, &c->p) && ecc_to_mont(&P->y, y, &c->one, &c->p)
      && mp_copy(&c->one, &P->z) == MP_OKAY);
}

/*----------------------------------------------------------------------------*/
/* Check that the affine point (x, y) is on the curve: 0 <= x, y < p and      */
/* y^2 = x^3 - 3x + b (mod p). Returns 1 if so, 0 if not, -1 on error.        */
/*----------------------------------------------------------------------------*/
static int ecc_w_on_curve(ECC_WCURVE *c, const mp_int *x, const mp_int *y)
{
  mp_int *l = &c->t[0];
  mp_int *r = &c->t[1];

  if(mp_cmp(x, &c->p) != MP_LT || mp_cmp(y, &c->p) != MP_LT)
    return(0);

  ECC_CHK(mp_sqrmod(y, &c->p, l));              /* l = y^2              */
  ECC_CHK(mp_sqrmod(x, &c->p, r));              /* r = x^2              */
  ECC_CHK(mp_mulmod(r, x, &c->p, r));           /* r = x^3              */
  ECC_CHK(mp_submod(r, x, &c->p, r));           /* r = x^3 - 3x + b     */
  ECC_CHK(mp_submod(r, x, &c->p, r));
  ECC_CHK(mp_submod(r, x, &c->p, r));
  ECC_CHK(mp_addmod(r, &c->b, &c->p, r));
  return(mp_cmp(l, r) == MP_EQ);

ecc_error:
  return(-1);
}

/*----------------------------------------------------------------------------*/
/* R = 2P in Jacobian coordinates for a = -3 ("dbl-2001-b"). R may be P.      */
/*----------------------------------------------------------------------------*/
static int ecc_w_double(ECC_WCURVE *c, ECC_JPOINT *R, const ECC_JPOINT *P)
{
  mp_int *delta = &c->t[0];
  mp_int *gamma = &c->t[1];
  mp_int *beta  = &c->t[2];
  mp_int *alpha = &c->t[3];
  mp_int *x3    = &c->t[4];
  mp_int *y3    = &c->t[5];
  mp_int *z3    = &c->t[6];
  mp_int *u     = &c->t[7];

  /* The double of the point at infinity or of a point of order 2 */
  if(mp_iszero(&P->z) || mp_iszero(&P->y))
  {
    ecc_jpoint_infinity(c, R);
    return(1);
  }

  FSQR(delta, &P->z);                           /* delta = Z^2              */
  FSQR(gamma, &P->y);                           /* gamma = Y^2              */
  FMUL(beta, &P->x, gamma);                     /* beta = X*gamma           */

  FSUB(u, &P->x, delta);                        /* alpha = 3(X-delta)(X+delta) */
  FADD(alpha, &P->x, delta);
  FMUL(u, u, alpha);
  FADD(alpha, u, u);
  FADD(alpha, alpha, u);

  FSQR(x3, alpha);                              /* X3 = alpha^2 - 8beta     */
  FADD(u, beta, beta);                          /* u = 2beta                */
  FADD(u, u, u);                                /* u = 4beta                */
  FSUB(x3, x3, u);
  FSUB(x3, x3, u);

  FADD(z3, &P->y, &P->z);                       /* Z3 = (Y+Z)^2 - gamma - delta */
  FSQR(z3, z3);
  FSUB(z3, z3, gamma);
  FSUB(z3, z3, delta);

  FSUB(y3, u, x3);                              /* Y3 = alpha(4beta - X3) - 8gamma^2 */
  FMUL(y3, alpha, y3);
  FSQR(u, gamma);
  FADD(u, u, u);
  FADD(u, u, u);
  FADD(u, u, u);
  FSUB(y3, y3, u);

  ECC_CHK(mp_copy(x3, &R->x));
  ECC_CHK(mp_copy(y3, &R->y));
  ECC_CHK(mp_copy(z3, &R->z));
  return(1);

ecc_error:
  return(0);
}

/*----------------------------------------------------------------------------*/
/* R = P + Q in Jacobian coordinates ("add-2007-bl"). R may be P or Q.        */
/*----------------------------------------------------------------------------*/
static int ecc_w_add(ECC_WCURVE *c, ECC_JPOINT *R, const ECC_JPOINT *P, const ECC_JPOINT *Q)
{
  mp_int *z1z1 = &c->t[0];
  mp_int *z2z2 = &c->t[1];
  mp_int *u1   = &c->t[2];
  mp_int *u2   = &c->t[3];
  mp_int *s1   = &c->t[4];
  mp_int *s2   = &c->t[5];
  mp_int *h    = &c->t[6];
  mp_int *r    = &c->t[7];
  mp_int *i    = &c->t[8];
  mp_int *j    = &c->t[9];
  mp_int *v    = &c->t[10];
  mp_int *w    = &c->t[11];

  /* Neutral element */
  if(mp_iszero(&P->z))
    return(ecc_jpoint_copy(R, Q));
  if(mp_iszero(&Q->z))
    return(ecc_jpoint_copy(R, P));

  FSQR(z1z1, &P->z);                            /* Z1Z1 = Z1^2              */
  FSQR(z2z2, &Q->z);                            /* Z2Z2 = Z2^2              */
  FMUL(u1, &P->x, z2z2);                        /* U1 = X1*Z2Z2             */
  FMUL(u2, &Q->x, z1z1);                        /* U2 = X2*Z1Z1             */
  FMUL(s1, &P->y, &Q->z);                       /* S1 = Y1*Z2*Z2Z2          */
  FMUL(s1, s1, z2z2);
  FMUL(s2, &Q->y, &P->z);                       /* S2 = Y2*Z1*Z1Z1          */
  FMUL(s2, s2, z1z1);
  FSUB(h, u2, u1);                              /* H = U2 - U1              */
  FSUB(r, s2, s1);                              /* r = 2(S2 - S1)           */
  FADD(r, r, r);

  /* Same x coordinate: P = Q (double) or P = -Q (infinity) */
  if(mp_iszero(h))
  {
    if(mp_iszero(r))
      return(ecc_w_double(c, R, P));
    ecc_jpoint_infinity(c, R);
    return(1);
  }

  FADD(i, h, h);                                /* I = (2H)^2               */
  FSQR(i, i);
  FMUL(j, h, i);                                /* J = H*I                  */
  FMUL(v, u1, i);                               /* V = U1*I                 */

  FADD(w, &P->z, &Q->z);                        /* Z3 = ((Z1+Z2)^2 - Z1Z1 - Z2Z2)*H */
  FSQR(w, w);
  FSUB(w, w, z1z1);
  FSUB(w, w, z2z2);
  FMUL(&R->z, w, h);

  FSQR(u2, r);                                  /* X3 = r^2 - J - 2V        */
  FSUB(u2, u2, j);
  FSUB(u2, u2, v);
  FSUB(u2, u2, v);

  FSUB(v, v, u2);                               /* Y3 = r(V - X3) - 2*S1*J  */
  FMUL(v, r, v);
  FMUL(s1, s1, j);
  FADD(s1, s1, s1);
  FSUB(&R->y, v, s1);
  ECC_CHK(mp_copy(u2, &R->x));
  return(1);

ecc_error:
  return(0);
}

/*----------------------------------------------------------------------------*/
/* R = kP, k given as a big-endian byte string. Fixed 4-bit window: the       */
/* multiples P..15P are precomputed, then for every nibble four doublings and */
/* at most one addition. Leading zero bits are skipped. R may be P.           */
/*----------------------------------------------------------------------------*/
static int ecc_w_mul(ECC_WCURVE *c, ECC_JPOINT *R, const ECC_JPOINT *P, const BYTE *k, int klen)
{
  ECC_JPOINT T[16];
  int i;
  int d;
  int nibble;
  int started = 0;
  int rc = 0;

  memset(T, 0, sizeof(T));
  for(i = 1; i < 16; i++)
  {
    if(!ecc_jpoint_init(&T[i]))
      goto ecc_error;
  }
  if(!ecc_jpoint_copy(&T[1], P) || !ecc_w_double(c, &T[2], &T[1]))
    goto ecc_error;
  for(i = 3; i < 16; i++)
  {
    if(!ecc_w_add(c, &T[i], &T[i - 1], &T[1]))
      goto ecc_error;
  }

  ecc_jpoint_infinity(c, R);
  for(i = 0; i < 2 * klen; i++)
  {
    nibble = (i & 1) ? (k[i / 2] & 0x0F) : (k[i / 2] >> 4);
    if(started)
    {
      for(d = 0; d < 4; d++)
      {
        if(!ecc_w_double(c, R, R))
          goto ecc_error;
      }
      if(nibble && !ecc_w_add(c, R, R, &T[nibble]))
        goto ecc_error;
    }
    else if(nibble)
    {
      if(!ecc_jpoint_copy(R, &T[nibble]))
        goto ecc_error;
      started = 1;
    }
  }
  rc = 1;

ecc_error:
  for(i = 1; i < 16; i++)
    ecc_jpoint_clear(&T[i]);
  return(rc);
}

/*----------------------------------------------------------------------------*/
/* Convert P (not the point at infinity) to affine coordinates (x, y) in      */
/* normal form                                                                */
/*----------------------------------------------------------------------------*/
static int ecc_w_affine(ECC_WCURVE *c, mp_int *x, mp_int *y, const ECC_JPOINT *P)
{
  mp_int *zi  = &c->t[0];
  mp_int *zi2 = &c->t[1];
  mp_int *xn  = &c->t[2];
  mp_int *yn  = &c->t[3];

  ECC_T(ecc_from_mont(zi2, &P->z, &c->p, c->rho));
  ECC_T(ecc_from_mont(xn, &P->x, &c->p, c->rho));
  ECC_T(ecc_from_mont(yn, &P->y, &c->p, c->rho));
  ECC_CHK(mp_invmod(zi2, &c->p, zi));
  ECC_CHK(mp_sqrmod(zi, &c->p, zi2));
  ECC_CHK(mp_mulmod(xn, zi2, &c->p, x));
  ECC_CHK(mp_mulmod(zi2, zi, &c->p, zi2));
  ECC_CHK(mp_mulmod(yn, zi2, &c->p, y));
  return(1);

ecc_error:
  return(0);
}

/*----------------------------------------------------------------------------*/
/* ECDSA verify (KDSA FC 1-3)                                                 */
/*                                                                            */
/* Parameter block: signature r, signature s, hash, public key x, public      */
/* key y, each len bytes. The hash is used as an integer reduced modulo n;    */
/* the program provides it truncated (leftmost bits) as required by ECDSA.    */
/*----------------------------------------------------------------------------*/
static int ecc_ecdsa_verify(int curve, const BYTE *pb)
{
  ECC_WCURVE c;
  ECC_JPOINT G, Q, R;
  mp_int r, s, e, x, y, w;
  BYTE u1[ECC_MAX_FIELD];
  BYTE u2[ECC_MAX_FIELD];
  int len;
  int on;
  int cc = ECC_CC_INVALID;

  /* Cleared so that the cleanup code is safe after any failure */
  memset(&G, 0, sizeof(G)); memset(&Q, 0, sizeof(Q)); memset(&R, 0, sizeof(R));
  memset(&r, 0, sizeof(r)); memset(&s, 0, sizeof(s)); memset(&e, 0, sizeof(e));
  memset(&x, 0, sizeof(x)); memset(&y, 0, sizeof(y)); memset(&w, 0, sizeof(w));

  len = ecc_wcurves[curve].len;
  if(!ecc_wcurve_init(&c, curve)
  || !ecc_jpoint_init(&G) || !ecc_jpoint_init(&Q) || !ecc_jpoint_init(&R)
  || mp_init_multi(&r, &s, &e, &x, &y, &w, NULL) != MP_OKAY)
    goto ecc_error;

  ECC_LOAD(&r, pb, len);
  ECC_LOAD(&s, pb + len, len);
  ECC_LOAD(&e, pb + 2 * len, len);
  ECC_LOAD(&x, pb + 3 * len, len);
  ECC_LOAD(&y, pb + 4 * len, len);

  /* The public key must be a point on the curve */
  on = ecc_w_on_curve(&c, &x, &y);
  if(on < 0)
    goto ecc_error;
  if(!on)
  {
    cc = ECC_CC_INVALID;
    goto ecc_done;
  }

  /* r and s must be in [1, n-1] */
  cc = ECC_CC_FAILED;
  if(mp_iszero(&r) || mp_iszero(&s)
  || mp_cmp(&r, &c.n) != MP_LT || mp_cmp(&s, &c.n) != MP_LT)
    goto ecc_done;

  /* u1 = e/s, u2 = r/s (mod n) */
  ECC_CHK(mp_mod(&e, &c.n, &e));
  ECC_CHK(mp_invmod(&s, &c.n, &w));
  ECC_CHK(mp_mulmod(&e, &w, &c.n, &e));
  ECC_CHK(mp_mulmod(&r, &w, &c.n, &w));
  if(!ecc_store(u1, len, &e) || !ecc_store(u2, len, &w))
    goto ecc_error;

  /* R = u1*G + u2*Q */
  if(!ecc_jpoint_set(&c, &G, &c.gx, &c.gy) || !ecc_jpoint_set(&c, &Q, &x, &y)
  || !ecc_w_mul(&c, &G, &G, u1, len) || !ecc_w_mul(&c, &Q, &Q, u2, len)
  || !ecc_w_add(&c, &R, &G, &Q))
    goto ecc_error;
  if(mp_iszero(&R.z))
    goto ecc_done;

  /* The signature is valid if the x coordinate of R equals r (mod n) */
  if(!ecc_w_affine(&c, &x, &y, &R))
    goto ecc_error;
  ECC_CHK(mp_mod(&x, &c.n, &x));
  if(mp_cmp(&x, &r) == MP_EQ)
    cc = ECC_CC_OK;
  goto ecc_done;

ecc_error:
  cc = ECC_CC_INVALID;

ecc_done:
  mp_clear_multi(&r, &s, &e, &x, &y, &w, NULL);
  ecc_jpoint_clear(&G);
  ecc_jpoint_clear(&Q);
  ecc_jpoint_clear(&R);
  ecc_wcurve_clear(&c);
  return(cc);
}

/*----------------------------------------------------------------------------*/
/* ECDSA sign (KDSA FC 9-11)                                                  */
/*                                                                            */
/* Parameter block: signature r, signature s (both output), hash, private     */
/* key d, random number k, each len bytes. The random number k is passed      */
/* separately, it is either the one from the parameter block (deterministic) */
/* or one generated by the caller. On success r and s are stored into the     */
/* parameter block.                                                           */
/*----------------------------------------------------------------------------*/
static int ecc_ecdsa_sign(int curve, BYTE *pb, const BYTE *k)
{
  ECC_WCURVE c;
  ECC_JPOINT R;
  mp_int e, d, kk, x, y, s;
  BYTE rbuf[ECC_MAX_FIELD];
  BYTE sbuf[ECC_MAX_FIELD];
  int len;
  int cc = ECC_CC_INVALID;

  /* Cleared so that the cleanup code is safe after any failure */
  memset(&R, 0, sizeof(R));
  memset(&e, 0, sizeof(e)); memset(&d, 0, sizeof(d)); memset(&kk, 0, sizeof(kk));
  memset(&x, 0, sizeof(x)); memset(&y, 0, sizeof(y)); memset(&s, 0, sizeof(s));

  len = ecc_wcurves[curve].len;
  if(!ecc_wcurve_init(&c, curve) || !ecc_jpoint_init(&R)
  || mp_init_multi(&e, &d, &kk, &x, &y, &s, NULL) != MP_OKAY)
    goto ecc_error;

  ECC_LOAD(&e, pb + 2 * len, len);
  ECC_LOAD(&d, pb + 3 * len, len);
  ECC_LOAD(&kk, k, len);

  /* The hash must not be zero and the private key must be in [1, n-1] */
  cc = ECC_CC_INVALID;
  if(mp_iszero(&e) || mp_iszero(&d) || mp_cmp(&d, &c.n) != MP_LT)
    goto ecc_done;

  /* The random number must be in [1, n-1] */
  cc = ECC_CC_FAILED;
  if(mp_iszero(&kk) || mp_cmp(&kk, &c.n) != MP_LT)
    goto ecc_done;

  /* r = x(kG) mod n, must not be zero */
  if(!ecc_jpoint_set(&c, &R, &c.gx, &c.gy) || !ecc_w_mul(&c, &R, &R, k, len))
    goto ecc_error;
  if(mp_iszero(&R.z))
    goto ecc_done;
  if(!ecc_w_affine(&c, &x, &y, &R))
    goto ecc_error;
  ECC_CHK(mp_mod(&x, &c.n, &x));
  if(mp_iszero(&x))
    goto ecc_done;

  /* s = (e + r*d)/k mod n, must not be zero */
  ECC_CHK(mp_mod(&e, &c.n, &e));
  ECC_CHK(mp_mulmod(&x, &d, &c.n, &s));
  ECC_CHK(mp_addmod(&s, &e, &c.n, &s));
  ECC_CHK(mp_invmod(&kk, &c.n, &y));
  ECC_CHK(mp_mulmod(&s, &y, &c.n, &s));
  if(mp_iszero(&s))
    goto ecc_done;

  if(!ecc_store(rbuf, len, &x) || !ecc_store(sbuf, len, &s))
    goto ecc_error;
  memcpy(pb, rbuf, len);
  memcpy(pb + len, sbuf, len);
  cc = ECC_CC_OK;
  goto ecc_done;

ecc_error:
  cc = ECC_CC_INVALID;

ecc_done:
  mp_clear_multi(&e, &d, &kk, &x, &y, &s, NULL);
  ecc_jpoint_clear(&R);
  ecc_wcurve_clear(&c);
  return(cc);
}

/*----------------------------------------------------------------------------*/
/* Scalar multiply on P-256, P-384, P-521 (PCC FC 64-66)                      */
/*                                                                            */
/* Parameter block: result x, result y (both output), source x, source y,     */
/* scalar, each len bytes. On success the result is stored into the           */
/* parameter block. An invalid source point or the point at infinity as the   */
/* result gives condition code 2.                                             */
/*----------------------------------------------------------------------------*/
static int ecc_w_scalar_multiply(int curve, BYTE *pb)
{
  ECC_WCURVE c;
  ECC_JPOINT P;
  mp_int x, y;
  BYTE xbuf[ECC_MAX_FIELD];
  BYTE ybuf[ECC_MAX_FIELD];
  int len;
  int on;
  int cc = ECC_CC_FAILED;

  /* Cleared so that the cleanup code is safe after any failure */
  memset(&P, 0, sizeof(P));
  memset(&x, 0, sizeof(x)); memset(&y, 0, sizeof(y));

  len = ecc_wcurves[curve].len;
  if(!ecc_wcurve_init(&c, curve) || !ecc_jpoint_init(&P)
  || mp_init_multi(&x, &y, NULL) != MP_OKAY)
    goto ecc_error;

  ECC_LOAD(&x, pb + 2 * len, len);
  ECC_LOAD(&y, pb + 3 * len, len);

  /* The source must be a point on the curve */
  on = ecc_w_on_curve(&c, &x, &y);
  if(on < 0)
    goto ecc_error;
  if(!on)
    goto ecc_done;

  if(!ecc_jpoint_set(&c, &P, &x, &y) || !ecc_w_mul(&c, &P, &P, pb + 4 * len, len))
    goto ecc_error;
  if(mp_iszero(&P.z))
    goto ecc_done;

  if(!ecc_w_affine(&c, &x, &y, &P)
  || !ecc_store(xbuf, len, &x) || !ecc_store(ybuf, len, &y))
    goto ecc_error;
  memcpy(pb, xbuf, len);
  memcpy(pb + len, ybuf, len);
  cc = ECC_CC_OK;
  goto ecc_done;

ecc_error:
  cc = ECC_CC_INVALID;

ecc_done:
  mp_clear_multi(&x, &y, NULL);
  ecc_jpoint_clear(&P);
  ecc_wcurve_clear(&c);
  return(cc);
}

/*----------------------------------------------------------------------------*/
/* Parameter block field length of a NIST curve, n bit length for k masking   */
/*----------------------------------------------------------------------------*/
static int ecc_wcurve_len(int curve)
{
  return(ecc_wcurves[curve].len);
}

static int ecc_wcurve_order_bits(int curve)
{
  static const int bits[3] = { 256, 384, 521 };

  return(bits[curve]);
}

/*============================================================================*/
/* Edwards curves (Ed25519, Ed448) and Montgomery curves (X25519, X448)       */
/*============================================================================*/

#define ECC_ED25519  0
#define ECC_ED448    1

#define ECC_X25519   0
#define ECC_X448     1

/* Largest encoded length (Ed448: 57) and hash length (Ed448: 114 bytes) */
#define ECC_MAX_ENC   57
#define ECC_MAX_HASH  114

/*----------------------------------------------------------------------------*/
/* Twisted Edwards curves a*x^2 + y^2 = 1 + d*x^2*y^2 over GF(p)              */
/*----------------------------------------------------------------------------*/
typedef struct ECC_ECURVE_DEF
{
  int         enclen;   /* length of encoded points and scalars (RFC 8032) */
  int         fld;      /* parameter block field length in bytes          */
  int         a;        /* curve coefficient a, -1 or 1                   */
  const char *p;        /* prime modulus                                  */
  const char *d;        /* curve coefficient d                            */
  const char *l;        /* order L of the base point                      */
  const char *bx;       /* base point x                                   */
  const char *by;       /* base point y                                   */
  const char *sqrtexp;  /* exponent for the square root candidate         */
  const char *sqrtm1;   /* sqrt(-1) for p = 5 mod 8, NULL for p = 3 mod 4 */
} ECC_ECURVE_DEF;

static const ECC_ECURVE_DEF ecc_ecurves[2] =
{
  {   /* Ed25519 */
    32, 32, -1,
    "7FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFED",
    "52036CEE2B6FFE738CC740797779E89800700A4D4141D8AB75EB4DCA135978A3",
    "1000000000000000000000000000000014DEF9DEA2F79CD65812631A5CF5D3ED",
    "216936D3CD6E53FEC0A4E231FDD6DC5C692CC7609525A7B2C9562D608F25D51A",
    "6666666666666666666666666666666666666666666666666666666666666658",
    "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFE",
    "2B8324804FC1DF0B2B4D00993DFBD7A72F431806AD2FE478C4EE1B274A0EA0B0",
  },
  {   /* Ed448 (untwisted, a = 1) */
    57, 64, 1,
    "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFFFF"
      "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF",
    "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFFFF"
      "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF6756",
    "3FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF7CCA23E9"
      "C44EDB49AED63690216CC2728DC58F552378C292AB5844F3",
    "4F1970C66BED0DED221D15A622BF36DA9E146570470F1767EA6DE324A3D3A464"
      "12AE1AF72AB66511433B80E18B00938E2626A82BC70CC05E",
    "693F46716EB6BC248876203756C9C7624BEA73736CA3984087789C1E05A0C2D7"
      "3AD3FF1CE67C39C4FDBD132C4ED7C8AD9808795BF230FA14",
    "3FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFC0000000"
      "000000000000000000000000000000000000000000000000",
    NULL,
  },
};

/* Curve in working form */
typedef struct ECC_ECURVE
{
  const ECC_ECURVE_DEF *def;
  mp_int   p, d, l, bx, by, sqrtexp, sqrtm1;
  mp_int   one;                 /* R mod p, the Montgomery form of 1     */
  mp_int   dm;                  /* d in Montgomery form                  */
  mp_digit rho;                 /* Montgomery constant -1/p mod 2^digit  */
  mp_int   t[10];               /* temporaries of the point arithmetic   */
} ECC_ECURVE;

/* Point in projective coordinates (x/z, y/z), the neutral element is (0, 1). */
/* The coordinates are kept in Montgomery form.                               */
typedef struct ECC_EPOINT
{
  mp_int x, y, z;
} ECC_EPOINT;

static int ecc_ecurve_init(ECC_ECURVE *c, int curve)
{
  const ECC_ECURVE_DEF *def = &ecc_ecurves[curve];
  int i;

  memset(c, 0, sizeof(*c));
  c->def = def;
  if(mp_init_multi(&c->p, &c->d, &c->l, &c->bx, &c->by, &c->sqrtexp, &c->sqrtm1,
                   &c->one, &c->dm, NULL) != MP_OKAY)
    return(0);
  for(i = 0; i < 10; i++)
  {
    if(mp_init(&c->t[i]) != MP_OKAY)
      return(0);
  }
  if(mp_read_radix(&c->p, def->p, 16) != MP_OKAY
  || mp_read_radix(&c->d, def->d, 16) != MP_OKAY
  || mp_read_radix(&c->l, def->l, 16) != MP_OKAY
  || mp_read_radix(&c->bx, def->bx, 16) != MP_OKAY
  || mp_read_radix(&c->by, def->by, 16) != MP_OKAY
  || mp_read_radix(&c->sqrtexp, def->sqrtexp, 16) != MP_OKAY
  || (def->sqrtm1 && mp_read_radix(&c->sqrtm1, def->sqrtm1, 16) != MP_OKAY)
  || mp_montgomery_setup(&c->p, &c->rho) != MP_OKAY
  || mp_montgomery_calc_normalization(&c->one, &c->p) != MP_OKAY
  || !ecc_to_mont(&c->dm, &c->d, &c->one, &c->p))
    return(0);
  return(1);
}

static void ecc_ecurve_clear(ECC_ECURVE *c)
{
  int i;

  for(i = 0; i < 10; i++)
    mp_clear(&c->t[i]);
  mp_clear_multi(&c->p, &c->d, &c->l, &c->bx, &c->by, &c->sqrtexp, &c->sqrtm1,
                 &c->one, &c->dm, NULL);
}

static int ecc_epoint_init(ECC_EPOINT *P)
{
  memset(P, 0, sizeof(*P));
  return(mp_init_multi(&P->x, &P->y, &P->z, NULL) == MP_OKAY);
}

static void ecc_epoint_clear(ECC_EPOINT *P)
{
  mp_clear_multi(&P->x, &P->y, &P->z, NULL);
}

static int ecc_epoint_copy(ECC_EPOINT *R, const ECC_EPOINT *P)
{
  return(mp_copy(&P->x, &R->x) == MP_OKAY && mp_copy(&P->y, &R->y) == MP_OKAY
      && mp_copy(&P->z, &R->z) == MP_OKAY);
}

/* Set P to the affine point (x, y), converted to Montgomery form */
static int ecc_epoint_set(ECC_ECURVE *c, ECC_EPOINT *P, const mp_int *x, const mp_int *y)
{
  return(ecc_to_mont(&P->x, x, &c->one, &c->p) && ecc_to_mont(&P->y, y, &c->one, &c->p)
      && mp_copy(&c->one, &P->z) == MP_OKAY);
}

/*----------------------------------------------------------------------------*/
/* R = P + Q with the complete addition formula for projective twisted        */
/* Edwards coordinates ("add-2008-bbjlp"). R may be P or Q.                   */
/*----------------------------------------------------------------------------*/
static int ecc_e_add(ECC_ECURVE *c, ECC_EPOINT *R, const ECC_EPOINT *P, const ECC_EPOINT *Q)
{
  mp_int *a  = &c->t[0];
  mp_int *b  = &c->t[1];
  mp_int *cc = &c->t[2];
  mp_int *dd = &c->t[3];
  mp_int *e  = &c->t[4];
  mp_int *f  = &c->t[5];
  mp_int *g  = &c->t[6];
  mp_int *u  = &c->t[7];
  mp_int *v  = &c->t[8];

  FMUL(a, &P->z, &Q->z);                        /* A = Z1*Z2                */
  FSQR(b, a);                                   /* B = A^2                  */
  FMUL(cc, &P->x, &Q->x);                       /* C = X1*X2                */
  FMUL(dd, &P->y, &Q->y);                       /* D = Y1*Y2                */
  FMUL(e, &c->dm, cc);                          /* E = d*C*D                */
  FMUL(e, e, dd);
  FSUB(f, b, e);                                /* F = B - E                */
  FADD(g, b, e);                                /* G = B + E                */

  FADD(u, &P->x, &P->y);                        /* X3 = A*F*((X1+Y1)(X2+Y2) - C - D) */
  FADD(v, &Q->x, &Q->y);
  FMUL(u, u, v);
  FSUB(u, u, cc);
  FSUB(u, u, dd);
  FMUL(u, u, f);
  FMUL(u, u, a);

  if(c->def->a < 0)                             /* Y3 = A*G*(D - aC)        */
    FADD(v, dd, cc);
  else
    FSUB(v, dd, cc);
  FMUL(v, v, g);
  FMUL(&R->y, v, a);

  FMUL(&R->z, f, g);                            /* Z3 = F*G                 */
  ECC_CHK(mp_copy(u, &R->x));
  return(1);

ecc_error:
  return(0);
}

/*----------------------------------------------------------------------------*/
/* R = 2P with the doubling formula for projective twisted Edwards            */
/* coordinates ("dbl-2008-bbjlp", 3M + 4S instead of 10M + 1S of the add).    */
/* R may be P.                                                                */
/*----------------------------------------------------------------------------*/
static int ecc_e_double(ECC_ECURVE *c, ECC_EPOINT *R, const ECC_EPOINT *P)
{
  mp_int *b  = &c->t[0];
  mp_int *cc = &c->t[1];
  mp_int *dd = &c->t[2];
  mp_int *f  = &c->t[3];
  mp_int *h  = &c->t[4];
  mp_int *j  = &c->t[5];
  mp_int *u  = &c->t[6];

  FADD(b, &P->x, &P->y);                        /* B = (X1+Y1)^2            */
  FSQR(b, b);
  FSQR(cc, &P->x);                              /* C = X1^2                 */
  FSQR(dd, &P->y);                              /* D = Y1^2                 */
  if(c->def->a < 0)                             /* E = aC, F = E + D        */
    FSUB(f, dd, cc);
  else
    FADD(f, dd, cc);
  FSQR(h, &P->z);                               /* H = Z1^2                 */
  FSUB(j, f, h);                                /* J = F - 2H               */
  FSUB(j, j, h);

  FSUB(u, b, cc);                               /* X3 = (B - C - D)*J       */
  FSUB(u, u, dd);
  FMUL(&R->x, u, j);
  if(c->def->a < 0)                             /* Y3 = F*(E - D)           */
  {
    FADD(u, cc, dd);                            /*    = -F*(C + D), a = -1  */
    FMUL(u, f, u);
    ECC_T(ecc_fneg(&R->y, u, &c->p));
  }
  else
  {
    FSUB(u, cc, dd);                            /*    =  F*(C - D), a = 1   */
    FMUL(&R->y, f, u);
  }
  FMUL(&R->z, f, j);                            /* Z3 = F*J                 */
  return(1);

ecc_error:
  return(0);
}

/*----------------------------------------------------------------------------*/
/* R = kP, k given as a big-endian byte string. Fixed 4-bit window with the   */
/* precomputed multiples P..15P, leading zero bits are skipped. R may be P.   */
/*----------------------------------------------------------------------------*/
static int ecc_e_mul(ECC_ECURVE *c, ECC_EPOINT *R, const ECC_EPOINT *P, const BYTE *k, int klen)
{
  ECC_EPOINT T[16];
  int i;
  int d;
  int nibble;
  int started = 0;
  int rc = 0;

  memset(T, 0, sizeof(T));
  for(i = 1; i < 16; i++)
  {
    if(!ecc_epoint_init(&T[i]))
      goto ecc_error;
  }
  if(!ecc_epoint_copy(&T[1], P) || !ecc_e_double(c, &T[2], &T[1]))
    goto ecc_error;
  for(i = 3; i < 16; i++)
  {
    if(!ecc_e_add(c, &T[i], &T[i - 1], &T[1]))
      goto ecc_error;
  }

  /* Neutral element (0 : 1 : 1) */
  mp_zero(&R->x);
  ECC_CHK(mp_copy(&c->one, &R->y));
  ECC_CHK(mp_copy(&c->one, &R->z));
  for(i = 0; i < 2 * klen; i++)
  {
    nibble = (i & 1) ? (k[i / 2] & 0x0F) : (k[i / 2] >> 4);
    if(started)
    {
      for(d = 0; d < 4; d++)
      {
        if(!ecc_e_double(c, R, R))
          goto ecc_error;
      }
      if(nibble && !ecc_e_add(c, R, R, &T[nibble]))
        goto ecc_error;
    }
    else if(nibble)
    {
      if(!ecc_epoint_copy(R, &T[nibble]))
        goto ecc_error;
      started = 1;
    }
  }
  rc = 1;

ecc_error:
  for(i = 1; i < 16; i++)
    ecc_epoint_clear(&T[i]);
  return(rc);
}

/* Multiply by a scalar given as an mp_int (converted to big-endian bytes) */
static int ecc_e_mul_mp(ECC_ECURVE *c, ECC_EPOINT *R, const ECC_EPOINT *P, const mp_int *k)
{
  BYTE kbuf[ECC_MAX_HASH];
  int rc;

  if(!ecc_store(kbuf, c->def->fld, k))
    return(0);
  rc = ecc_e_mul(c, R, P, kbuf, c->def->fld);
  crypto_secure0(kbuf, sizeof(kbuf));
  return(rc);
}

/* Convert P to affine coordinates (x, y) in normal form */
static int ecc_e_affine(ECC_ECURVE *c, mp_int *x, mp_int *y, const ECC_EPOINT *P)
{
  mp_int *zi = &c->t[0];
  mp_int *zn = &c->t[1];
  mp_int *xn = &c->t[2];
  mp_int *yn = &c->t[3];

  ECC_T(ecc_from_mont(zn, &P->z, &c->p, c->rho));
  ECC_T(ecc_from_mont(xn, &P->x, &c->p, c->rho));
  ECC_T(ecc_from_mont(yn, &P->y, &c->p, c->rho));
  ECC_CHK(mp_invmod(zn, &c->p, zi));
  ECC_CHK(mp_mulmod(xn, zi, &c->p, x));
  ECC_CHK(mp_mulmod(yn, zi, &c->p, y));
  return(1);

ecc_error:
  return(0);
}

/*----------------------------------------------------------------------------*/
/* Check that the affine point (x, y) is on the curve: 0 <= x, y < p and      */
/* a*x^2 + y^2 = 1 + d*x^2*y^2. Returns 1 if so, 0 if not, -1 on error.       */
/*----------------------------------------------------------------------------*/
static int ecc_e_on_curve(ECC_ECURVE *c, const mp_int *x, const mp_int *y)
{
  mp_int *x2 = &c->t[0];
  mp_int *y2 = &c->t[1];
  mp_int *l  = &c->t[2];
  mp_int *r  = &c->t[3];

  if(mp_cmp(x, &c->p) != MP_LT || mp_cmp(y, &c->p) != MP_LT)
    return(0);

  ECC_CHK(mp_sqrmod(x, &c->p, x2));
  ECC_CHK(mp_sqrmod(y, &c->p, y2));
  if(c->def->a < 0)                             /* l = a*x^2 + y^2          */
    ECC_CHK(mp_submod(y2, x2, &c->p, l));
  else
    ECC_CHK(mp_addmod(y2, x2, &c->p, l));
  ECC_CHK(mp_mulmod(x2, y2, &c->p, r));         /* r = 1 + d*x^2*y^2        */
  ECC_CHK(mp_mulmod(r, &c->d, &c->p, r));
  mp_set(x2, 1);
  ECC_CHK(mp_addmod(r, x2, &c->p, r));
  return(mp_cmp(l, r) == MP_EQ);

ecc_error:
  return(-1);
}

/*----------------------------------------------------------------------------*/
/* Little-endian byte strings (RFC 8032 encodings) to and from mp_int         */
/*----------------------------------------------------------------------------*/
static int ecc_load_le(mp_int *a, const BYTE *src, int len)
{
  BYTE buf[ECC_MAX_HASH];
  int i;
  int rc;

  for(i = 0; i < len; i++)
    buf[i] = src[len - 1 - i];
  rc = ecc_load(a, buf, len);
  crypto_secure0(buf, sizeof(buf));
  return(rc);
}

static int ecc_store_le(BYTE *dst, int len, const mp_int *a)
{
  BYTE buf[ECC_MAX_HASH];
  int i;

  if(!ecc_store(buf, len, a))
    return(0);
  for(i = 0; i < len; i++)
    dst[i] = buf[len - 1 - i];
  crypto_secure0(buf, sizeof(buf));
  return(1);
}

/* Encode the affine point (x, y): y little-endian, the least significant */
/* bit of x in the most significant bit of the last byte (RFC 8032)       */
static int ecc_e_encode(ECC_ECURVE *c, BYTE *enc, const mp_int *x, const mp_int *y)
{
  int n = c->def->enclen;

  if(!ecc_store_le(enc, n, y))
    return(0);
  if(mp_isodd(x))
    enc[n - 1] |= 0x80;
  return(1);
}

/*----------------------------------------------------------------------------*/
/* Decode a point (RFC 8032 5.1.3 / 5.2.3). Returns 1 if valid, 0 if the      */
/* encoding is invalid, -1 on error.                                          */
/*----------------------------------------------------------------------------*/
static int ecc_e_decode(ECC_ECURVE *c, mp_int *x, mp_int *y, const BYTE *enc)
{
  BYTE buf[ECC_MAX_ENC];
  mp_int *u  = &c->t[4];
  mp_int *v  = &c->t[5];
  mp_int *x2 = &c->t[6];
  mp_int *w  = &c->t[7];
  int n = c->def->enclen;
  int sign;

  memcpy(buf, enc, n);
  sign = (buf[n - 1] >> 7) & 1;
  buf[n - 1] &= 0x7F;
  if(!ecc_load_le(y, buf, n))
    goto ecc_error;
  if(mp_cmp(y, &c->p) != MP_LT)
    return(0);

  /* x^2 = (y^2 - 1) / (d*y^2 - a) */
  ECC_CHK(mp_sqrmod(y, &c->p, w));
  mp_set(u, 1);
  ECC_CHK(mp_submod(w, u, &c->p, u));           /* u = y^2 - 1              */
  ECC_CHK(mp_mulmod(w, &c->d, &c->p, v));       /* v = d*y^2 - a            */
  mp_set(w, 1);
  if(c->def->a < 0)
    ECC_CHK(mp_addmod(v, w, &c->p, v));
  else
    ECC_CHK(mp_submod(v, w, &c->p, v));
  if(mp_iszero(v))
    return(0);
  ECC_CHK(mp_invmod(v, &c->p, x2));
  ECC_CHK(mp_mulmod(u, x2, &c->p, x2));

  /* Square root candidate, for p = 5 mod 8 possibly times sqrt(-1) */
  ECC_CHK(mp_exptmod(x2, &c->sqrtexp, &c->p, x));
  ECC_CHK(mp_sqrmod(x, &c->p, w));
  if(mp_cmp(w, x2) != MP_EQ)
  {
    if(!c->def->sqrtm1)
      return(0);
    ECC_CHK(mp_mulmod(x, &c->sqrtm1, &c->p, x));
    ECC_CHK(mp_sqrmod(x, &c->p, w));
    if(mp_cmp(w, x2) != MP_EQ)
      return(0);
  }

  /* Select the root by the sign bit, x = 0 with sign 1 is invalid */
  if(mp_iszero(x) && sign)
    return(0);
  if((mp_isodd(x) ? 1 : 0) != sign)
    ECC_CHK(mp_sub(&c->p, x, x));
  return(1);

ecc_error:
  return(-1);
}

/*----------------------------------------------------------------------------*/
/* Parameter block fields of EdDSA hold the encodings byte-reversed, i.e. as  */
/* right-aligned big-endian values in fields of fld bytes. Convert a field to */
/* the RFC 8032 (little-endian) encoding and back.                            */
/*----------------------------------------------------------------------------*/
static void ecc_eddsa_field_to_enc(int curve, BYTE *enc, const BYTE *field)
{
  const ECC_ECURVE_DEF *def = &ecc_ecurves[curve];
  int i;

  for(i = 0; i < def->enclen; i++)
    enc[i] = field[def->fld - 1 - i];
}

static void ecc_eddsa_enc_to_field(int curve, BYTE *field, const BYTE *enc)
{
  const ECC_ECURVE_DEF *def = &ecc_ecurves[curve];
  int i;

  memset(field, 0, def->fld);
  for(i = 0; i < def->enclen; i++)
    field[def->fld - 1 - i] = enc[i];
}

static int ecc_ecurve_fld(int curve)
{
  return(ecc_ecurves[curve].fld);
}

static int ecc_ecurve_enclen(int curve)
{
  return(ecc_ecurves[curve].enclen);
}

/*----------------------------------------------------------------------------*/
/* EdDSA hash H: SHA-512 for Ed25519, SHAKE256(dom4(0, "") || ..., 114) for   */
/* Ed448 (pure EdDSA without context, RFC 8032). Streaming interface so that  */
/* the message can be hashed directly from guest storage.                     */
/*----------------------------------------------------------------------------*/
typedef struct ECC_HASH
{
  int      curve;
  SHA2_CTX sha;                 /* Ed25519                               */
  BYTE     state[200];          /* Ed448: SHAKE256 sponge, rate 136      */
  int      pos;
} ECC_HASH;

static void ecc_hash_update(ECC_HASH *h, const BYTE *data, size_t len)
{
  if(h->curve == ECC_ED25519)
  {
    SHA512Update(&h->sha, data, len);
    return;
  }
  while(len--)
  {
    h->state[h->pos++] ^= *data++;
    if(h->pos == 136)
    {
      keccak_f1600(h->state);
      h->pos = 0;
    }
  }
}

static void ecc_hash_init(ECC_HASH *h, int curve)
{
  static const BYTE dom4[10] = { 'S', 'i', 'g', 'E', 'd', '4', '4', '8', 0, 0 };

  memset(h, 0, sizeof(*h));
  h->curve = curve;
  if(curve == ECC_ED25519)
    SHA512Init(&h->sha);
  else
    ecc_hash_update(h, dom4, sizeof(dom4));
}

/* Plain hash of the private key seed: SHA-512, or SHAKE256 without dom4 */
static void ecc_hash_seed(int curve, BYTE *out, const BYTE *seed)
{
  ECC_HASH h;

  memset(&h, 0, sizeof(h));
  h.curve = curve;
  if(curve == ECC_ED25519)
  {
    SHA512Init(&h.sha);
    SHA512Update(&h.sha, seed, 32);
    SHA512Final(out, &h.sha);
  }
  else
  {
    ecc_hash_update(&h, seed, 57);
    h.state[h.pos] ^= 0x1F;
    h.state[135] ^= 0x80;
    keccak_f1600(h.state);
    memcpy(out, h.state, 114);
  }
  crypto_secure0(&h, sizeof(h));
}

/* Finish the hash: 64 (Ed25519) or 114 (Ed448) bytes */
static void ecc_hash_final(ECC_HASH *h, BYTE *out)
{
  if(h->curve == ECC_ED25519)
    SHA512Final(out, &h->sha);
  else
  {
    h->state[h->pos] ^= 0x1F;
    h->state[135] ^= 0x80;
    keccak_f1600(h->state);
    memcpy(out, h->state, 114);
  }
  crypto_secure0(h, sizeof(*h));
}

/*----------------------------------------------------------------------------*/
/* EdDSA sign (KDSA FC 40, 44), split into steps because the message is       */
/* hashed twice by the caller directly from guest storage:                    */
/*                                                                            */
/*   ecc_eddsa_sign_start   secret scalar s, prefix and public key A from the */
/*                          private key seed                                  */
/*   caller                 rhash = H(prefix || M)                            */
/*   ecc_eddsa_sign_r       r = rhash mod L, R = rB                           */
/*   caller                 khash = H(R || A || M)                            */
/*   ecc_eddsa_sign_finish  S = r + k*s mod L, store R and S                  */
/*----------------------------------------------------------------------------*/
typedef struct ECC_EDDSA_SIGN
{
  BYTE s[ECC_MAX_ENC];          /* secret scalar (little-endian)         */
  BYTE prefix[ECC_MAX_ENC];     /* second half of the seed hash          */
  BYTE a_enc[ECC_MAX_ENC];      /* encoded public key A                  */
  BYTE r[ECC_MAX_ENC];          /* r = H(prefix || M) mod L (little-end.) */
  BYTE r_enc[ECC_MAX_ENC];      /* encoded R                             */
} ECC_EDDSA_SIGN;

static int ecc_eddsa_sign_start(int curve, ECC_EDDSA_SIGN *ctx, const BYTE *seed)
{
  ECC_ECURVE c;
  ECC_EPOINT A;
  mp_int s, x, y;
  BYTE h[ECC_MAX_HASH];
  int n;
  int rc = 0;

  memset(&A, 0, sizeof(A));
  memset(&s, 0, sizeof(s)); memset(&x, 0, sizeof(x)); memset(&y, 0, sizeof(y));
  n = ecc_ecurves[curve].enclen;

  /* Secret scalar s: the first half of the hash, pruned (RFC 8032) */
  ecc_hash_seed(curve, h, seed);
  memcpy(ctx->s, h, n);
  memcpy(ctx->prefix, h + n, n);
  if(curve == ECC_ED25519)
  {
    ctx->s[0] &= 248;
    ctx->s[31] &= 127;
    ctx->s[31] |= 64;
  }
  else
  {
    ctx->s[0] &= 252;
    ctx->s[55] |= 128;
    ctx->s[56] = 0;
  }

  /* Public key A = sB */
  if(!ecc_ecurve_init(&c, curve) || !ecc_epoint_init(&A)
  || mp_init_multi(&s, &x, &y, NULL) != MP_OKAY)
    goto ecc_error;
  if(!ecc_load_le(&s, ctx->s, n)
  || !ecc_epoint_set(&c, &A, &c.bx, &c.by) || !ecc_e_mul_mp(&c, &A, &A, &s)
  || !ecc_e_affine(&c, &x, &y, &A) || !ecc_e_encode(&c, ctx->a_enc, &x, &y))
    goto ecc_error;
  rc = 1;

ecc_error:
  crypto_secure0(h, sizeof(h));
  mp_clear_multi(&s, &x, &y, NULL);
  ecc_epoint_clear(&A);
  ecc_ecurve_clear(&c);
  return(rc);
}

static int ecc_eddsa_sign_r(int curve, ECC_EDDSA_SIGN *ctx, const BYTE *rhash)
{
  ECC_ECURVE c;
  ECC_EPOINT R;
  mp_int r, x, y;
  int n;
  int rc = 0;

  memset(&R, 0, sizeof(R));
  memset(&r, 0, sizeof(r)); memset(&x, 0, sizeof(x)); memset(&y, 0, sizeof(y));
  n = ecc_ecurves[curve].enclen;

  if(!ecc_ecurve_init(&c, curve) || !ecc_epoint_init(&R)
  || mp_init_multi(&r, &x, &y, NULL) != MP_OKAY)
    goto ecc_error;
  if(!ecc_load_le(&r, rhash, 2 * n))
    goto ecc_error;
  ECC_CHK(mp_mod(&r, &c.l, &r));
  if(!ecc_store_le(ctx->r, n, &r)
  || !ecc_epoint_set(&c, &R, &c.bx, &c.by) || !ecc_e_mul_mp(&c, &R, &R, &r)
  || !ecc_e_affine(&c, &x, &y, &R) || !ecc_e_encode(&c, ctx->r_enc, &x, &y))
    goto ecc_error;
  rc = 1;

ecc_error:
  mp_clear_multi(&r, &x, &y, NULL);
  ecc_epoint_clear(&R);
  ecc_ecurve_clear(&c);
  return(rc);
}

/* Store the signature into the first two parameter block fields */
static int ecc_eddsa_sign_finish(int curve, ECC_EDDSA_SIGN *ctx, const BYTE *khash, BYTE *pb)
{
  ECC_ECURVE c;
  mp_int k, r, s;
  BYTE s_enc[ECC_MAX_ENC];
  int n;
  int fld;
  int rc = 0;

  memset(&k, 0, sizeof(k)); memset(&r, 0, sizeof(r)); memset(&s, 0, sizeof(s));
  n = ecc_ecurves[curve].enclen;
  fld = ecc_ecurves[curve].fld;

  if(!ecc_ecurve_init(&c, curve) || mp_init_multi(&k, &r, &s, NULL) != MP_OKAY)
    goto ecc_error;
  if(!ecc_load_le(&k, khash, 2 * n) || !ecc_load_le(&r, ctx->r, n) || !ecc_load_le(&s, ctx->s, n))
    goto ecc_error;
  ECC_CHK(mp_mod(&k, &c.l, &k));                /* S = r + k*s mod L        */
  ECC_CHK(mp_mulmod(&k, &s, &c.l, &s));
  ECC_CHK(mp_addmod(&s, &r, &c.l, &s));
  if(!ecc_store_le(s_enc, n, &s))
    goto ecc_error;
  ecc_eddsa_enc_to_field(curve, pb, ctx->r_enc);
  ecc_eddsa_enc_to_field(curve, pb + fld, s_enc);
  rc = 1;

ecc_error:
  crypto_secure0(s_enc, sizeof(s_enc));
  mp_clear_multi(&k, &r, &s, NULL);
  ecc_ecurve_clear(&c);
  return(rc);
}

/*----------------------------------------------------------------------------*/
/* EdDSA verify (KDSA FC 32, 36)                                              */
/*                                                                            */
/* Parameter block: R, S, public key A, each a byte-reversed encoding in a    */
/* field of fld bytes. khash = H(R || A || M) is computed by the caller.      */
/* Checks [S]B = R + [k]A (cofactorless). Condition code 1 if A is not a      */
/* valid encoding, 2 if the signature is invalid.                             */
/*----------------------------------------------------------------------------*/
static int ecc_eddsa_verify(int curve, const BYTE *pb, const BYTE *khash)
{
  ECC_ECURVE c;
  ECC_EPOINT A, R, SB;
  mp_int ax, ay, rx, ry, s, k;
  BYTE enc[ECC_MAX_ENC];
  int n;
  int fld;
  int ok;
  int cc = ECC_CC_INVALID;

  memset(&A, 0, sizeof(A)); memset(&R, 0, sizeof(R)); memset(&SB, 0, sizeof(SB));
  memset(&ax, 0, sizeof(ax)); memset(&ay, 0, sizeof(ay)); memset(&rx, 0, sizeof(rx));
  memset(&ry, 0, sizeof(ry)); memset(&s, 0, sizeof(s)); memset(&k, 0, sizeof(k));
  n = ecc_ecurves[curve].enclen;
  fld = ecc_ecurves[curve].fld;

  if(!ecc_ecurve_init(&c, curve)
  || !ecc_epoint_init(&A) || !ecc_epoint_init(&R) || !ecc_epoint_init(&SB)
  || mp_init_multi(&ax, &ay, &rx, &ry, &s, &k, NULL) != MP_OKAY)
    goto ecc_error;

  /* The public key must be a valid point encoding */
  ecc_eddsa_field_to_enc(curve, enc, pb + 2 * fld);
  ok = ecc_e_decode(&c, &ax, &ay, enc);
  if(ok < 0)
    goto ecc_error;
  if(!ok)
    goto ecc_done;

  /* R must be a valid point encoding and S must be less than L */
  cc = ECC_CC_FAILED;
  ecc_eddsa_field_to_enc(curve, enc, pb);
  ok = ecc_e_decode(&c, &rx, &ry, enc);
  if(ok < 0)
    goto ecc_error;
  if(!ok)
    goto ecc_done;
  ecc_eddsa_field_to_enc(curve, enc, pb + fld);
  if(!ecc_load_le(&s, enc, n))
    goto ecc_error;
  if(mp_cmp(&s, &c.l) != MP_LT)
    goto ecc_done;
  if(!ecc_load_le(&k, khash, 2 * n))
    goto ecc_error;
  ECC_CHK(mp_mod(&k, &c.l, &k));

  /* [S]B and R + [k]A */
  if(!ecc_epoint_set(&c, &SB, &c.bx, &c.by) || !ecc_e_mul_mp(&c, &SB, &SB, &s)
  || !ecc_epoint_set(&c, &A, &ax, &ay) || !ecc_e_mul_mp(&c, &A, &A, &k)
  || !ecc_epoint_set(&c, &R, &rx, &ry) || !ecc_e_add(&c, &R, &R, &A)
  || !ecc_e_affine(&c, &ax, &ay, &SB) || !ecc_e_affine(&c, &rx, &ry, &R))
    goto ecc_error;
  if(mp_cmp(&ax, &rx) == MP_EQ && mp_cmp(&ay, &ry) == MP_EQ)
    cc = ECC_CC_OK;
  goto ecc_done;

ecc_error:
  cc = ECC_CC_INVALID;

ecc_done:
  mp_clear_multi(&ax, &ay, &rx, &ry, &s, &k, NULL);
  ecc_epoint_clear(&A);
  ecc_epoint_clear(&R);
  ecc_epoint_clear(&SB);
  ecc_ecurve_clear(&c);
  return(cc);
}

/*----------------------------------------------------------------------------*/
/* Scalar multiply on Ed25519 and Ed448 (PCC FC 72, 73)                       */
/*                                                                            */
/* Parameter block: result x, result y (both output), source x, source y,     */
/* scalar, each a right-aligned big-endian integer of fld bytes (affine       */
/* coordinates, not encodings). Condition code 2 for an invalid source point  */
/* or the neutral element as the result.                                      */
/*----------------------------------------------------------------------------*/
static int ecc_e_scalar_multiply(int curve, BYTE *pb)
{
  ECC_ECURVE c;
  ECC_EPOINT P;
  mp_int x, y;
  BYTE xbuf[ECC_MAX_FIELD];
  BYTE ybuf[ECC_MAX_FIELD];
  int fld;
  int on;
  int cc = ECC_CC_FAILED;

  memset(&P, 0, sizeof(P));
  memset(&x, 0, sizeof(x)); memset(&y, 0, sizeof(y));
  fld = ecc_ecurves[curve].fld;

  if(!ecc_ecurve_init(&c, curve) || !ecc_epoint_init(&P)
  || mp_init_multi(&x, &y, NULL) != MP_OKAY)
    goto ecc_error;

  ECC_LOAD(&x, pb + 2 * fld, fld);
  ECC_LOAD(&y, pb + 3 * fld, fld);
  on = ecc_e_on_curve(&c, &x, &y);
  if(on < 0)
    goto ecc_error;
  if(!on)
    goto ecc_done;

  if(!ecc_epoint_set(&c, &P, &x, &y) || !ecc_e_mul(&c, &P, &P, pb + 4 * fld, fld)
  || !ecc_e_affine(&c, &x, &y, &P))
    goto ecc_error;

  /* The neutral element (0, 1) as the result */
  if(mp_iszero(&x))
    goto ecc_done;

  if(!ecc_store(xbuf, fld, &x) || !ecc_store(ybuf, fld, &y))
    goto ecc_error;
  memcpy(pb, xbuf, fld);
  memcpy(pb + fld, ybuf, fld);
  cc = ECC_CC_OK;
  goto ecc_done;

ecc_error:
  cc = ECC_CC_INVALID;

ecc_done:
  mp_clear_multi(&x, &y, NULL);
  ecc_epoint_clear(&P);
  ecc_ecurve_clear(&c);
  return(cc);
}

/*----------------------------------------------------------------------------*/
/* Montgomery curves: scalar multiply on X25519 and X448 (PCC FC 80, 81)      */
/*                                                                            */
/* Parameter block: result u (output), source u, scalar, each a right-aligned */
/* big-endian integer of fld bytes (32 or 64). The scalar is used as given,   */
/* clamping (RFC 7748) is done by the program. The x-only Montgomery ladder   */
/* runs over all bits of the scalar field. Condition code 2 if u is not less  */
/* than p or the result is zero (point at infinity or of small order).        */
/*----------------------------------------------------------------------------*/
static int ecc_x_scalar_multiply(int curve, BYTE *pb)
{
  static const mp_digit a24v[2] = { 121665, 39081 };
  mp_int p, one, a24, x1, x2, z2, x3, z3, a, aa, b, bb, e, cc, dd, da, cb, t;
  mp_digit rho;
  BYTE ubuf[ECC_MAX_FIELD];
  const BYTE *k;
  int fld;
  int i;
  int kt;
  int started = 0;
  int swap = 0;
  int rc = ECC_CC_FAILED;

  memset(&p, 0, sizeof(p)); memset(&one, 0, sizeof(one)); memset(&a24, 0, sizeof(a24));
  memset(&x1, 0, sizeof(x1)); memset(&x2, 0, sizeof(x2)); memset(&z2, 0, sizeof(z2));
  memset(&x3, 0, sizeof(x3)); memset(&z3, 0, sizeof(z3)); memset(&a, 0, sizeof(a));
  memset(&aa, 0, sizeof(aa)); memset(&b, 0, sizeof(b)); memset(&bb, 0, sizeof(bb));
  memset(&e, 0, sizeof(e)); memset(&cc, 0, sizeof(cc)); memset(&dd, 0, sizeof(dd));
  memset(&da, 0, sizeof(da)); memset(&cb, 0, sizeof(cb)); memset(&t, 0, sizeof(t));
  fld = ecc_ecurves[curve].fld;               /* same as for Ed25519/Ed448 */
  k = pb + 2 * fld;

  if(mp_init_multi(&p, &one, &a24, &x1, &x2, &z2, &x3, &z3, &a, &aa, &b, &bb, &e,
                   &cc, &dd, &da, &cb, &t, NULL) != MP_OKAY)
    goto ecc_error;
  ECC_CHK(mp_read_radix(&p, ecc_ecurves[curve].p, 16));
  ECC_CHK(mp_montgomery_setup(&p, &rho));
  ECC_CHK(mp_montgomery_calc_normalization(&one, &p));
  ECC_LOAD(&t, pb + fld, fld);
  if(mp_cmp(&t, &p) != MP_LT)
    goto ecc_done;

  /* Montgomery form: x1 = u, (x2:z2) = (1:0), (x3:z3) = (u:1), a24 */
  ECC_T(ecc_to_mont(&x1, &t, &one, &p));
  mp_set(&t, a24v[curve]);
  ECC_T(ecc_to_mont(&a24, &t, &one, &p));
  ECC_CHK(mp_copy(&one, &x2));
  mp_zero(&z2);
  ECC_CHK(mp_copy(&x1, &x3));
  ECC_CHK(mp_copy(&one, &z3));

  /* Ladder over the bits of the scalar, leading zero bits are skipped    */
  /* (they would leave (x2:z2) and (x3:z3) unchanged as projective points) */
  for(i = 0; i < 8 * fld; i++)
  {
    kt = (k[i / 8] >> (7 - (i % 8))) & 1;
    if(!started)
    {
      if(!kt)
        continue;
      started = 1;
    }
    swap ^= kt;
    if(swap)
    {
      mp_exch(&x2, &x3);
      mp_exch(&z2, &z3);
    }
    swap = kt;

    ECC_T(ecc_fadd(&a, &x2, &z2, &p));          /* A = x2 + z2, AA = A^2    */
    ECC_T(ecc_fsqr(&aa, &a, &p, rho));
    ECC_T(ecc_fsub(&b, &x2, &z2, &p));          /* B = x2 - z2, BB = B^2    */
    ECC_T(ecc_fsqr(&bb, &b, &p, rho));
    ECC_T(ecc_fsub(&e, &aa, &bb, &p));          /* E = AA - BB              */
    ECC_T(ecc_fadd(&cc, &x3, &z3, &p));         /* C = x3 + z3              */
    ECC_T(ecc_fsub(&dd, &x3, &z3, &p));         /* D = x3 - z3              */
    ECC_T(ecc_fmul(&da, &dd, &a, &p, rho));     /* DA = D*A                 */
    ECC_T(ecc_fmul(&cb, &cc, &b, &p, rho));     /* CB = C*B                 */
    ECC_T(ecc_fadd(&t, &da, &cb, &p));          /* x3 = (DA + CB)^2         */
    ECC_T(ecc_fsqr(&x3, &t, &p, rho));
    ECC_T(ecc_fsub(&t, &da, &cb, &p));          /* z3 = x1*(DA - CB)^2      */
    ECC_T(ecc_fsqr(&t, &t, &p, rho));
    ECC_T(ecc_fmul(&z3, &x1, &t, &p, rho));
    ECC_T(ecc_fmul(&x2, &aa, &bb, &p, rho));    /* x2 = AA*BB               */
    ECC_T(ecc_fmul(&t, &a24, &e, &p, rho));     /* z2 = E*(AA + a24*E)      */
    ECC_T(ecc_fadd(&t, &t, &aa, &p));
    ECC_T(ecc_fmul(&z2, &e, &t, &p, rho));
  }
  if(swap)
  {
    mp_exch(&x2, &x3);
    mp_exch(&z2, &z3);
  }

  /* u = x2/z2 in normal form, zero (or z2 = 0, the point at infinity) */
  /* is an error                                                       */
  ECC_T(ecc_from_mont(&a, &x2, &p, rho));
  ECC_T(ecc_from_mont(&b, &z2, &p, rho));
  if(mp_iszero(&b))
    goto ecc_done;
  ECC_CHK(mp_invmod(&b, &p, &t));
  ECC_CHK(mp_mulmod(&a, &t, &p, &t));
  if(mp_iszero(&t))
    goto ecc_done;
  if(!ecc_store(ubuf, fld, &t))
    goto ecc_error;
  memcpy(pb, ubuf, fld);
  rc = ECC_CC_OK;
  goto ecc_done;

ecc_error:
  rc = ECC_CC_INVALID;

ecc_done:
  mp_clear_multi(&p, &one, &a24, &x1, &x2, &z2, &x3, &z3, &a, &aa, &b, &bb, &e,
                 &cc, &dd, &da, &cb, &t, NULL);
  return(rc);
}

#endif /* _DYNCRYPT_ECC_H_ */
