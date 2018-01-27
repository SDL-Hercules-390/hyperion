/* PFPO.C       (C) Copyright Roger Bowler, 2009-2012                */
/*              Perform Floating Point Operation instruction         */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*              (C) Copyright Bernard van der Helm, 2009-2011        */
/*              Noordwijkerhout, The Netherlands                     */

/*-------------------------------------------------------------------*/
/* This module implements the Perform Floating Point Operation       */
/* instruction described in the manual SA22-7832-05.                 */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

#define _PFPO_C_
#define _HENGINE_DLL_

#include "hercules.h"
#include "opcode.h"
#include "inline.h"

#include "decNumber/include/decimal128.h"
#include "decNumber/include/decimal64.h"
#include "decNumber/include/decimal32.h"
#include "decNumber/include/decPacked.h"

#if defined(FEATURE_044_PFPO_FACILITY)

#define CLASS_ZERO      1
#define CLASS_SUBNORMAL 2
#define CLASS_INFINITY  3
#define CLASS_QNAN      4
#define CLASS_SNAN      5
#define CLASS_NORMAL    6

#define INFINITYSTR     "Infinity"
#define QNANSTR         "NaN"
#define SNANSTR         "sNaN"

typedef struct
{
  unsigned base;
  unsigned fpclass;
  char str[256];
}
FLOAT;

#define float_strlcat(s)  strlcat(f->str, s, sizeof(f->str))

/*-------------------------------------------------------------------*/
/* Binairy floating point routines                                   */
/*-------------------------------------------------------------------*/

void BFPshortGet(FLOAT *f, U32 r)
{
  unsigned exp;
  U32 frac;
  unsigned i;
  U32 mask;
  unsigned sign;

  sign = r & 0x80000000 ? 1 : 0;
  exp = (r & 0x7f800000) >> 23;
  frac = r & 0x007fffff;

  if(exp == 127)
  {
    if(!frac)
      f->fpclass = CLASS_ZERO;
    else
      f->fpclass = CLASS_SUBNORMAL;
  }
  else if(exp == 0xff)
  {
    if(!frac)
      f->fpclass = CLASS_INFINITY;
    else
    {
      if(frac & 0x80000000)
        f->fpclass = CLASS_QNAN;
      else
        f->fpclass = CLASS_SNAN;
    }
  }
  else
    f->fpclass = CLASS_NORMAL;

  f->base = 2;
  switch(f->fpclass)
  {
    case CLASS_ZERO:
      strcpy(f->str, "0");
      break;

    case CLASS_INFINITY:
      if(sign)
        strcpy(f->str, "-" INFINITYSTR);
      else
        strcpy(f->str, INFINITYSTR);
      break;

    case CLASS_QNAN:
      strcpy(f->str, QNANSTR);
      break;

    case CLASS_SNAN:
      strcpy(f->str, SNANSTR);
      break;

    case CLASS_NORMAL:
    case CLASS_SUBNORMAL:
      if(sign)
        strcpy(f->str, "-0.");
      else
        strcpy(f->str, "0.");
      if(f->fpclass == CLASS_NORMAL)
        float_strlcat("1");
      else
        float_strlcat("0");
      mask = 0x00400000;
      for(i = 0; i < 23; i++)
      {
        if(frac & mask)
          float_strlcat("1");
        else
          float_strlcat("0");
        mask >>= 1;
      }
      float_strlcat("@");
      if(f->fpclass == CLASS_NORMAL)
        sprintf(&f->str[strlen(f->str)], "%d", exp - 127);
      else
        float_strlcat("1");
      break;
  }
}

void BFPlongGet(FLOAT *f, U64 r)
{
  unsigned exp;
  U64 frac;
  unsigned i;
  U64 mask;
  unsigned sign;

  sign = r & 0x8000000000000000 ? 1 : 0;
  exp = (r & 0x7ff0000000000000) >> 52;
  frac = r & 0x000fffffffffffff;

  if(exp == 1023)
  {
    if(!frac)
      f->fpclass = CLASS_ZERO;
    else
      f->fpclass = CLASS_SUBNORMAL;
  }
  else if(exp == 0x7ff)
  {
    if(!frac)
      f->fpclass = CLASS_INFINITY;
    else
    {
      if(frac & 0x8000000000000000)
        f->fpclass = CLASS_QNAN;
      else
        f->fpclass = CLASS_SNAN;
    }
  }
  else
    f->fpclass = CLASS_NORMAL;

  f->base = 2;
  switch(f->fpclass)
  {
    case CLASS_ZERO:
      strcpy(f->str, "0");
      break;

    case CLASS_INFINITY:
      if(sign)
        strcpy(f->str, "-" INFINITYSTR);
      else
        strcpy(f->str, INFINITYSTR);
      break;

    case CLASS_QNAN:
      strcpy(f->str, QNANSTR);
      break;

    case CLASS_SNAN:
      strcpy(f->str, SNANSTR);
      break;

    case CLASS_NORMAL:
    case CLASS_SUBNORMAL:
      if(sign)
        strcpy(f->str, "-0.");
      else
        strcpy(f->str, "0.");
      if(f->fpclass == CLASS_NORMAL)
        float_strlcat("1");
      else
        float_strlcat("0");
      mask = 0x0008000000000000;
      for(i = 0; i < 52; i++)
      {
        if(frac & mask)
          float_strlcat("1");
        else
          float_strlcat("0");
        mask >>= 1;
      }
      float_strlcat("@");
      if(f->fpclass == CLASS_NORMAL)
        sprintf(&f->str[strlen(f->str)], "%d", exp - 1023);
      else
        float_strlcat("1");
      break;
  }
}

void BFPextGet(FLOAT *f, U64 h, U64 l)
{
  unsigned exp;
  U64 frach;
  U64 fracl;
  unsigned i;
  U64 mask;
  unsigned sign;

  sign = h & 0x8000000000000000 ? 1 : 0;
  exp = (h & 0x7fff000000000000) >> 48;
  frach = h & 0x0000ffffffffffff;
  fracl = l;

  if(exp == 16383)
  {
    if(!frach && !fracl)
      f->fpclass = CLASS_ZERO;
    else
      f->fpclass = CLASS_SUBNORMAL;
  }
  else if(exp == 0x7fff)
  {
    if(!frach && !fracl)
      f->fpclass = CLASS_INFINITY;
    else
    {
      if(frach & 0x8000000000000000)
        f->fpclass = CLASS_QNAN;
      else
        f->fpclass = CLASS_SNAN;
    }
  }
  else
    f->fpclass = CLASS_NORMAL;

  f->base = 2;
  switch(f->fpclass)
  {
    case CLASS_ZERO:
      strcpy(f->str, "0");
      break;

    case CLASS_INFINITY:
      if(sign)
        strcpy(f->str, "-" INFINITYSTR);
      else
        strcpy(f->str, INFINITYSTR);
      break;

    case CLASS_QNAN:
      strcpy(f->str, QNANSTR);
      break;

    case CLASS_SNAN:
      strcpy(f->str, SNANSTR);
      break;

    case CLASS_NORMAL:
    case CLASS_SUBNORMAL:
      if(sign)
        strcpy(f->str, "-0.");
      else
        strcpy(f->str, "0.");
      if(f->fpclass == CLASS_NORMAL)
        float_strlcat("1");
      else
        float_strlcat("0");
      mask = 0x0000800000000000;
      for(i = 0; i < 48; i++)
      {
        if(frach & mask)
          float_strlcat("1");
        else
          float_strlcat("0");
        mask >>= 1;
      }
      mask = 0x8000000000000000;
      for(i = 0; i < 64; i++)
      {
        if(fracl & mask)
          float_strlcat("1");
        else
          float_strlcat("0");
        mask >>= 1;
      }
      float_strlcat("@");
      if(f->fpclass == CLASS_NORMAL)
        sprintf(&f->str[strlen(f->str)], "%d", exp - 16383);
      else
        float_strlcat("1");
      break;
  }
}

/*-------------------------------------------------------------------*/
/* Decimal floating point routines                                   */
/*-------------------------------------------------------------------*/

void DFPshortGet(FLOAT *f, U32 r)
{
  unsigned fpclass;
  decimal32 dec32;
  unsigned i;
  U32 temp;

  temp = r;
  for(i = 0; i < 4; i++)
  {
    dec32.bytes[i] = temp & 0xff;
    temp >>= 8;
  }
  decimal32ToString(&dec32, f->str);

  f->base = 10;
  if(!strncmp(f->str, "-0E", 2) || !strncmp(f->str, "0E", 2))
    f->fpclass = CLASS_ZERO;
  else if(strstr(f->str, "E0"))
    f->fpclass = CLASS_SUBNORMAL;
  else if(!strcmp(f->str, "Ininity"))
    f->fpclass = CLASS_INFINITY;
  else if(!strcmp(f->str, "NaN"))
    f->fpclass = CLASS_QNAN;
  else if(!strcmp(f->str, "sNaN"))
    f->fpclass = CLASS_SNAN;
  else
    f->fpclass = CLASS_NORMAL;
}

void DFPlongGet(FLOAT *f, U64 r)
{
  unsigned fpclass;
  decimal64 dec64;
  unsigned i;
  U64 temp;

  temp = r;
  for(i = 0; i < 8; i++)
  {
    dec64.bytes[i] = temp & 0xff;
    temp >>= 8;
  }
  decimal64ToString(&dec64, f->str);

  f->base = 10;
  if(!strncmp(f->str, "-0E", 2) || !strncmp(f->str, "0E", 2))
    f->fpclass = CLASS_ZERO;
  else if(strstr(f->str, "E0"))
    f->fpclass = CLASS_SUBNORMAL;
  else if(!strcmp(f->str, "Ininity"))
    f->fpclass = CLASS_INFINITY;
  else if(!strcmp(f->str, "NaN"))
    f->fpclass = CLASS_QNAN;
  else if(!strcmp(f->str, "sNaN"))
    f->fpclass = CLASS_SNAN;
  else
    f->fpclass = CLASS_NORMAL;
}

void DFPextGet(FLOAT *f, U64 h, U64 l)
{
  unsigned fpclass;
  decimal128 dec128;
  unsigned i;
  U64 temph;
  U64 templ;

  temph = h;
  templ = l;

  for(i = 0; i < 8; i++)
  {
    dec128.bytes[i] = templ & 0xff;
    dec128.bytes[i + 8] = temph & 0xff;
    templ >>= 8;
    temph >>= 8;
  }
  decimal128ToString(&dec128, f->str);

  f->base = 10;
  if(!strncmp(f->str, "-0E", 2) || !strncmp(f->str, "0E", 2))
    f->fpclass = CLASS_ZERO;
  else if(strstr(f->str, "E0"))
    f->fpclass = CLASS_SUBNORMAL;
  else if(!strcmp(f->str, "Ininity"))
    f->fpclass = CLASS_INFINITY;
  else if(!strcmp(f->str, "NaN"))
    f->fpclass = CLASS_QNAN;
  else if(!strcmp(f->str, "sNaN"))
    f->fpclass = CLASS_SNAN;
  else
    f->fpclass = CLASS_NORMAL;
}

/*-------------------------------------------------------------------*/
/* Hexadecimal floating point routines                               */
/*-------------------------------------------------------------------*/

void HFPshortGet(FLOAT *f, U32 r)
{
  unsigned exp;
  U32 frac;
  unsigned i;
  U32 mask;
  unsigned sign;

  sign = r & 0x80000000 ? 1 : 0;
  exp = (r & 0x7f000000) >> 24;
  frac = r & 0x00ffffff;

  if(!frac)
    f->fpclass = CLASS_ZERO;
  else
    f->fpclass = CLASS_NORMAL;

  f->base = 16;
  switch(f->fpclass)
  {
    case CLASS_ZERO:
      strcpy(f->str, "0");
      break;

    case CLASS_NORMAL:
      if(sign)
        strcpy(f->str, "-0.");
      else
        strcpy(f->str, "0.");
      mask = 0x00ff0000;
      for(i = 0; i < 3; i++)
      {
        sprintf(&f->str[strlen(f->str)], "%02x", (r & mask) >> (16 - (i * 8)));
        mask >>= 8;
      }
      float_strlcat("@");
      sprintf(&f->str[strlen(f->str)], "%d", exp - 64);
      break;
  }
}

void HFPlongGet(FLOAT *f, U64 r)
{
  unsigned fpclass;
  unsigned exp;
  U64 frac;
  unsigned i;
  U64 mask;
  unsigned sign;

  sign = r & 0x8000000000000000 ? 1 : 0;
  exp = (r & 0x7f00000000000000) >> 56;
  frac = r & 0x00ffffffffffffff;

  if(!frac)
    f->fpclass = CLASS_ZERO;
  else
    f->fpclass = CLASS_NORMAL;

  f->base = 16;
  switch(fpclass)
  {
    case CLASS_ZERO:
      strcpy(f->str, "0");
      break;

    case CLASS_NORMAL:
      if(sign)
        strcpy(f->str, "-0.");
      else
        strcpy(f->str, "0.");
      mask = 0x00ff000000000000;
      for(i = 0; i < 7; i++)
      {
        sprintf(&f->str[strlen(f->str)], "%02x", (r & mask) >> (48 - (i * 8)));
        mask >>= 8;
      }
      float_strlcat("@");
      sprintf(&f->str[strlen(f->str)], "%d", exp - 64);
      break;
  }
}

void HFPextGet(FLOAT *f, U64 h, U64 l)
{
  unsigned fpclass;
  unsigned exp;
  U64 frach;
  U64 fracl;
  unsigned i;
  U64 mask;
  unsigned sign;

  sign = h & 0x8000000000000000 ? 1 : 0;
  exp = (h & 0x7f00000000000000) >> 56;
  frach = h & 0x00ffffffffffffff;
  fracl = l & 0x00ffffffffffffff;

  if(!frach && !fracl)
    f->fpclass = CLASS_ZERO;
  else
    f->fpclass = CLASS_NORMAL;

  f->base = 16;
  switch(f->fpclass)
  {
    case CLASS_ZERO:
      strcpy(f->str, "0");
      break;

    case CLASS_NORMAL:
      if(sign)
        strcpy(f->str, "-0.");
      else
        strcpy(f->str, "0.");
      mask = 0x00ff000000000000;
      for(i = 0; i < 7; i++)
      {
        sprintf(&f->str[strlen(f->str)], "%02x", (h & mask) >> (48 - (i * 8)));
        mask >>= 8;
      }
      mask = 0x00ff000000000000;
      for(i = 0; i < 7; i++)
      {
        sprintf(&f->str[strlen(f->str)], "%02x", (l & mask) >> (48 - (i * 8)));
        mask >>= 8;
      }
      float_strlcat("@");
      sprintf(&f->str[strlen(f->str)], "%d", exp - 64);
      break;
  }
}

/*-------------------------------------------------------------------*/
/* 010A PFPO  - Perform Floating Point Operation                 [E] */
/*-------------------------------------------------------------------*/
DEF_INST(perform_floating_point_operation)
{
  FACILITY_CHECK( 044_PFPO, regs );

  E(inst, regs);

  ARCH_DEP(program_interrupt)(regs, PGM_OPERATION_EXCEPTION);

} /* end DEF_INST(perform_floating_point_operation) */

#endif /*defined(FEATURE_044_PFPO_FACILITY)*/

#if !defined(_GEN_ARCH)

#if defined(_ARCH_NUM_1)
  #define  _GEN_ARCH _ARCH_NUM_1
  #include "pfpo.c"
#endif

#if defined(_ARCH_NUM_2)
  #undef   _GEN_ARCH
  #define  _GEN_ARCH _ARCH_NUM_2
  #include "pfpo.c"
#endif

#endif /*!defined(_GEN_ARCH)*/
