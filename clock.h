/* CLOCK.H      (C) Copyright Jan Jaeger, 2000-2012                  */
/*              TOD Clock functions                                  */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#if !defined(_CLOCK_C_)
 #define _CLOCK_EXTERN extern
#else
 #undef  _CLOCK_EXTERN
 #define _CLOCK_EXTERN
#endif


#if !defined(_CLOCK_H)
#define _CLOCK_H



/* TOD Clock Definitions */

#define TOD_USEC   (4096LL)
#define TOD_SEC    (1000000 * TOD_USEC)
#define TOD_MIN    (60 * TOD_SEC)
#define TOD_HOUR   (60 * TOD_MIN)
#define TOD_DAY    (24 * TOD_HOUR)
#define TOD_YEAR   (365 * TOD_DAY)
#define TOD_LYEAR  (TOD_YEAR + TOD_DAY)
#define TOD_4YEARS (1461 * TOD_DAY)
#define TOD_1970   0x7D91048BCA000000ULL        // TOD base for host epoch of 1970

typedef U64     TOD;                            // one microsecond = Bit 51


/* Hercules and Extended TOD Clock Definitions for high-order 64-bits */

#define ETOD_USEC   16LL
#define ETOD_SEC    (1000000 * ETOD_USEC)
#define ETOD_MIN    (60 * ETOD_SEC)
#define ETOD_HOUR   (60 * ETOD_MIN)
#define ETOD_DAY    (24 * ETOD_HOUR)
#define ETOD_YEAR   (365 * ETOD_DAY)
#define ETOD_LYEAR  (ETOD_YEAR + ETOD_DAY)
#define ETOD_4YEARS (1461 * ETOD_DAY)
#define ETOD_1970   0x007D91048BCA0000ULL       // Extended TOD base for host epoch of 1970


#ifdef WORDS_BIGENDIAN
typedef struct ETOD { U64 high, low; } ETOD;
#define ETOD_init(_high,_low) \
        {_high,_low}
#else
typedef struct ETOD { U64 low, high; } ETOD;
#define ETOD_init(_high,_low) \
        {_low,_high}
#endif


static INLINE void
ETOD_add (ETOD* result, const ETOD a, const ETOD b)
{
  register uint64_t high = a.high + b.high;
  register uint64_t low  = a.low + b.low;
  if (low < a.low)
    ++high;
  result->high = high;
  result->low  = low;
}

static INLINE void
ETOD_sub (ETOD* result, const ETOD a, const ETOD b)
{
  register uint64_t high = a.high - b.high;
  register uint64_t low  = a.low - b.low;
  if (a.low < b.low)
    --high;
  result->high = high;
  result->low  = low;
}

static INLINE void
ETOD_shift (ETOD* result, const ETOD a, int shift)
{
  register uint64_t high;
  register uint64_t low;

  if (shift == 0)
  {
    high = a.high;
    low  = a.low;
  }
  else if (shift < 0)
  {
    shift = -shift;
    if (shift >= 64)
    {
      shift -= 64;
      if (shift == 0)
        high = a.low;
      else if (shift > 64)
        high = 0;
      else
        high = a.low << shift;
      low = 0;
    }
    else
    {
      high = a.high << shift |
             a.low >> (64 - shift);
      low  = a.low << shift;
    }
  }
  else if (shift >= 64)
  {
    shift -= 64;
    high   = 0;
    if (shift == 0)
      low = a.high;
    else if (shift < 64)
      low = a.high >> shift;
    else
      low = 0;
  }
  else
  {
    high = a.high >> shift;
    low  = a.high << (64 - shift) |
           a.low >> shift;
  }

  result->low  = low;
  result->high = high;
}


/* Clock Steering Registers */
#if !defined(_CSR_)
#define _CSR_
typedef struct _CSR {
    U64 start_time;
    S64 base_offset;
    S32 fine_s_rate;
    S32 gross_s_rate;
} CSR;
#endif

void csr_reset(void);                   /* Reset cs registers        */
void set_tod_steering(const double);    /* Set steering rate         */
double get_tod_steering(void);          /* Get steering rate         */
U64 update_tod_clock(void);             /* Update the TOD clock      */
void update_cpu_timer(void);            /* Update the CPU timer      */
void set_tod_epoch(const S64);          /* Set TOD epoch             */
void adjust_tod_epoch(const S64);       /* Adjust TOD epoch          */
S64 get_tod_epoch(void);                /* Get TOD epoch             */
U64 hw_clock(void);                     /* Get hardware clock        */
S64 cpu_timer(REGS *);                  /* Retrieve CPU timer        */
void set_cpu_timer(REGS *, const S64);  /* Set CPU timer             */
void set_int_timer(REGS *, const S32);  /* Set interval timer        */
TOD tod_clock(REGS *);                  /* Get TOD clock non-unique  */
typedef enum
{
  ETOD_raw,
  ETOD_fast,
  ETOD_standard,
  ETOD_extended
} ETOD_format;
DLL_EXPORT
TOD etod_clock(REGS*, ETOD*,            /* Get extended TOD clock    */
               ETOD_format);
void set_tod_clock(const U64);          /* Set TOD clock             */
int chk_int_timer(REGS *);              /* Check int_timer pending   */
int clock_hsuspend(void *file);         /* Hercules suspend          */
int clock_hresume(void *file);          /* Hercules resume           */
int query_tzoffset(void);               /* Report current TzOFFSET   */

ETOD* host_ETOD (ETOD*);                /* Retrieve extended TOD     */

TOD thread_cputime(const REGS*);        /* Thread real CPU used (TOD)*/
U64 thread_cputime_us(const REGS*);     /* Thread real CPU used (us) */


/*----------------------------------------------------------------------------*/
/*      Define clock_gettime for systems not supporting nanosecond clocks,    */
/*      other than Windows.                                                   */
/*----------------------------------------------------------------------------*/

#if !defined(_MSVC_) && !defined(CLOCK_REALTIME)

typedef int     clockid_t;

/* IDs for various POSIX.1b interval timers and system clocks */

#define CLOCK_REALTIME                  0
#define CLOCK_MONOTONIC                 1
#define CLOCK_PROCESS_CPUTIME_ID        2
#define CLOCK_THREAD_CPUTIME_ID         3
#define CLOCK_MONOTONIC_RAW             4
#define CLOCK_REALTIME_COARSE           5
#define CLOCK_MONOTONIC_COARSE          6
#define CLOCK_BOOTTIME                  7
#define CLOCK_REALTIME_ALARM            8
#define CLOCK_BOOTTIME_ALARM            9


static INLINE int
clock_gettime ( clockid_t clk_id, struct timespec *ts )
{
    register int    result;

    /* Validate parameters... */

    if ( unlikely ( (clk_id > CLOCK_REALTIME) ||
                    !ts ) )
    {
        errno = EINVAL;
        return ( -1 );
    }

    #if defined(__APPLE__) && 0
    {
        /* FIXME: mach/mach.h include is generating invalid storage
         *        class errors. Default to gettimeofday until resolved.
         */

        #include <mach/mach.h>

        /* FIXME: This sequence is slower than gettimeofday call per OS
         *        X Developer Library. Recommend converting to use the
         *        mach_absolute_time call, but that will have to wait
         *        until CLOCK_MONOTONIC support and review of steering
         *        support to avoid double steering.
         */

        mach_timespec_t     mts;
        static clock_serv_t cserv = 0;

        if (!cserv)
        {
            /* Get clock service port */
            result = host_get_clock_service(mach_host_self(),
                                            CALENDAR_CLOCK,
                                            &cserv);
            if (result != KERN_SUCCESS)
            return ( result );
        }

        result = clock_get_time(cserv, &mts);
        if (result == KERN_SUCCESS)
        {
            ts->tv_sec  = mts.tv_sec;
            ts->tv_nsec = mts.tv_nsec;
        }
//      result = mach_port_deallocate(mach_task_self(), cserv);                 /* Can this be ignored until shutdown of Hercules? */
    }
    #else /* Convert standard gettimeofday call */
    {
        struct timeval  tv;

        result = gettimeofday(&tv, NULL);

        /* Handle microsecond overflow */
        if ( unlikely (tv.tv_usec >= 1000000) )
        {
            register U32    temp = tv.tv_usec / 1000000;
            tv.tv_sec  += temp;
            tv.tv_usec -= temp * 1000000;
        }

        /* Convert timeval to timespec */
        ts->tv_sec  = tv.tv_sec;
        ts->tv_nsec = tv.tv_usec * 1000;


        /* Reset clock precision and force host_ETOD to use minimum
         * precision algorithm.
         */

        #if defined(TOD_FULL_PRECISION)
            #undef TOD_FULL_PRECISION
        #endif

        #if defined(TOD_120BIT_PRECISION)
            #undef TOD_120BIT_PRECISION
        #endif

        #if defined(TOD_95BIT_PRECISION)
            #undef TOD_95BIT_PRECISION)
        #endif

        #if defined(TOD_64BIT_PRECISION)
            #undef TOD_64BIT_PRECISION)
        #endif

        #if !defined(TOD_MIN_PRECISION)
            #define TOD_MIN_PRECISION
        #endif
    }
    #endif

    return ( result );
}

#elif !(defined(TOD_FULL_PRECISION)     || \
        defined(TOD_120BIT_PRECISION)   || \
        defined(TOD_64BIT_PRECISION)    || \
        defined(TOD_MIN_PRECISION)      || \
        defined(TOD_95BIT_PRECISION))

    /* Define default clock precision as 95-bits (only one division required) */
    #define TOD_95BIT_PRECISION

#endif

static INLINE S64
timeval2us (const struct timeval* tv)
{
    S64 result;
    result  = (S64)tv->tv_sec * 1000000;
    result += tv->tv_usec;
    return (result);
}


static INLINE void
us2timeval (const U64 us, struct timeval* tv)
{
    tv->tv_sec  = us / 1000000;
    tv->tv_usec = us % 1000000;
}


static INLINE TOD
ns2etod (const S64 ns)
{
    return ((ns << 1) / 125);           /* (ns << 4) / 1000           */
}

static INLINE TOD
us2etod (const S64 us)
{
    return (us << 4);                   /* Adjust to bit 59           */
}

static INLINE TOD
ms2etod (const S64 ms)
{
    return (ms * 16000);                /* (ms * 1000) << 4           */
}

static INLINE TOD
sec2etod (const S64 sec)
{
    return (sec * 16000000);            /* (sec * 1000000) << 4       */
}

static INLINE TOD
timespec2etod (const struct timespec* ts)
{
    register S64 result;
    result  = sec2etod(ts->tv_sec);
    result += ns2etod(ts->tv_nsec);
    return (result);
}

static INLINE TOD
timeval2etod (const struct timeval* tv)
{
    register S64 result;
    result  = sec2etod(tv->tv_sec);
    result += us2etod(tv->tv_usec);
    return (result);
}

static INLINE TOD
etod2tod (const U64 etod)
{
    return (etod << 8);
}

static INLINE S64
etod2ns (const S64 etod)
{
    return ((etod * 125) >> 1);
}

static INLINE S64
etod2us (const S64 etod)
{
    return (etod >> 4);
}

static INLINE S64
etod2sec (const S64 etod)
{
    return (etod / ETOD_SEC);
}

static INLINE void
etod2timespec (const S64 etod, struct timespec* ts)
{
    register S64 work = etod - ETOD_1970;
    ts->tv_sec  = etod2sec(work);
    ts->tv_nsec = etod2ns(work % ETOD_SEC);
}

static INLINE void
etod2timeval (const S64 etod, struct timeval* tv)
{
    register S64 work = etod - ETOD_1970;
    tv->tv_sec  = etod2sec(work);
    tv->tv_usec = etod2us(work % ETOD_SEC);
}


static INLINE TOD
ns2ETOD (ETOD* ETOD, const S64 ns)
{
    /* This conversion, assuming a nanosecond host clock resolution,
     * yields a TOD clock resolution of 120-bits, 95-bits, or 64-bits,
     * with a period of over 36,533 years.
     *
     *
     * Original 128-bit code:
     *
     * register U128 result = ((((U128)time.tv_nsec << 68) / 1000)
     *                         + 0x8000) & ~((U128)0xFFFF);
     *
     * In the 64-bit translation of the routine, bits 121-127 are not
     * calculated as a third division is required.
     */

    register S64 high = 0;
    register S64 low  = ns;

    #if defined(TOD_FULL_PRECISION)       || \
        defined(TOD_120BIT_PRECISION)     || \
        defined(TOD_64BIT_PRECISION)      || \
        defined(TOD_MIN_PRECISION)        || \
        !defined(TOD_95BIT_PRECISION)
    {
        register U64    temp;
        temp        = low;          /* Adjust nanoseconds to bit-59 for       */
        temp      <<= 1;            /* division by 1000 (shift compressed),   */
        temp       /= 125;          /* calculating microseconds and the top   */
                                    /* nibble of the remainder                */
                                    /* (us*16 = ns*16/1000 = ns*2/125)        */
        high = temp;                /* Add to upper 64-bits                   */
        #if defined(TOD_MIN_PRECISION)      || \
            defined(TOD_64BIT_PRECISION)
            low    = 0;             /* Set lower 64-bits to zero              */
                                    /* (gettimeofday or other microsecond     */
                                    /* clock used as clock source)            */
        #else /* Calculate full precision fractional clock value              */
            temp >>= 1;             /* Calculate remainder                    */
            temp  *= 125;           /* ...                                    */
            low   -= temp;          /* ...                                    */
            low  <<= 57;            /* Divide remainder by 1000 and adjust    */
            low   /= 125;           /* to proper position, shifting out high- */
            low  <<= 8;             /* order byte                             */
            low   += 0x8000;        /* Round                                  */
            low   &= ~0xFFFFULL;    /* Mask of low-order two bytes            */
        #endif
    }
    #else /* 95-bit resolution                                                */
    {
        low <<= 32;                 /* Place nanoseconds in high-order word   */
        low  /= 125;                /* Divide by 1000 (125 * 2^3)             */
        high  = low >> 31;          /* Adjust and add microseconds and first  */
                                    /* nibble of nanosecond remainder to bits */
                                    /* 0-63                                   */
        low <<= 33;                 /* Adjust remaining nanosecond fraction   */
                                    /* to bits 64-93                          */
    }
    #endif

    ETOD->high = high,
    ETOD->low  = low;
    return (high);
}

static INLINE TOD
us2ETOD (ETOD* ETOD, const S64 us)
{
    register S64 high = us2etod(us);
    ETOD->high = high;
    ETOD->low  = 0;
    return (high);
}

static INLINE TOD
ms2ETOD (ETOD* ETOD, const S64 ms)
{
    register S64 high = ms2etod(ms);
    ETOD->high = high;
    ETOD->low  = 0;
    return (high);
}

static INLINE TOD
sec2ETOD (ETOD* ETOD, const S64 sec)
{
    register S64 high = sec2etod(sec);
    ETOD->high = high;
    ETOD->low  = 0;
    return (high);
}

static INLINE TOD
timespec2ETOD (ETOD* ETOD, const struct timespec* ts)
{
    /* This conversion, assuming a nanosecond host clock resolution,
     * yields a TOD clock resolution of 120-bits, 95-bits, or 64-bits,
     * with a period of over 36,533 years.
     *
     *
     * Original 128-bit code:
     *
     * register U128 result;
     * result  = ((((U128)time.tvsec * ETOD_SEC) + ETOD_1970) << 64) +
     *           (((U128)time.tv_nsec << 68) / 1000);
     *
     *
     * In the 64-bit translation of the routine, bits 121-127 are not
     * calculated as a third division is required.
     *
     * It is not necessary to normalize the clock_gettime return value,
     * ensuring that the tv_nsec field is less than 1 second, as tv_nsec
     * is a 32-bit field and 64-bit registers are in use.
     *
     */

    ETOD->high  = ts->tv_sec;       /* Convert seconds to microseconds,       */
    ETOD->high *= ETOD_SEC;         /* adjusted to bit-59; truncate above     */
                                    /* extended TOD clock resolution          */
    ETOD->high += ETOD_1970;        /* Adjust for open source epoch of 1970   */
    ETOD->low   = ts->tv_nsec;      /* Copy nanoseconds                       */

    #if defined(TOD_FULL_PRECISION)       || \
        defined(TOD_120BIT_PRECISION)     || \
        defined(TOD_64BIT_PRECISION)      || \
        defined(TOD_MIN_PRECISION)        || \
        !defined(TOD_95BIT_PRECISION)
    {
        register U64    temp;
        temp        = ETOD->low;    /* Adjust nanoseconds to bit-59 for       */
        temp      <<= 1;            /* division by 1000 (shift compressed),   */
        temp       /= 125;          /* calculating microseconds and the top   */
                                    /* nibble of the remainder                */
                                    /* (us*16 = ns*16/1000 = ns*2/125)        */
        ETOD->high += temp;         /* Add to upper 64-bits                   */
        #if defined(TOD_MIN_PRECISION)      || \
            defined(TOD_64BIT_PRECISION)
            ETOD->low   = 0;        /* Set lower 64-bits to zero              */
                                    /* (gettimeofday or other microsecond     */
                                    /* clock used as clock source)            */
        #else /* Calculate full precision fractional clock value              */
            temp      >>= 1;        /* Calculate remainder                    */
            temp       *= 125;      /* ...                                    */
            ETOD->low  -= temp;     /* ...                                    */
            ETOD->low <<= 57;       /* Divide remainder by 1000 and adjust    */
            ETOD->low  /= 125;      /* to proper position, shifting out high- */
            ETOD->low <<= 8;        /* order byte                             */
        #endif
    }
    #else /* 95-bit resolution                                                */
    {
        ETOD->low <<= 32;           /* Place nanoseconds in high-order word   */
        ETOD->low  /= 125;          /* Divide by 1000 (125 * 2^3)             */
        ETOD->high += ETOD->low >> 31;  /* Adjust and add microseconds and    */
                                    /* first nibble of nanosecond remainder   */
                                    /* to bits 0-63                           */
        ETOD->low <<= 33;           /* Adjust remaining nanosecond fraction   */
                                    /* to bits 64-93                          */
    }
    #endif
    return ( ETOD->high );          /* Return address of result               */
}

static INLINE TOD
timeval2ETOD (ETOD* ETOD, const struct timeval* tv)
{
    ETOD->high = timeval2etod(tv);
    ETOD->low  = 0;
    return (ETOD->high);
}


static INLINE TOD
ns2tod (const U64 ns)
{
    return ((ns << 9) / 125);           /* (ns << 12) / 1000          */
}

static INLINE TOD
us2tod (const U64 us)
{
    return (us << 12);                  /* Adjust to bit 51           */
}

static INLINE TOD
ms2tod (const S64 ms)
{
    return (ms * 4096000);              /* (ms * 1000) << 12          */
}

static INLINE TOD
sec2tod (const S64 sec)
{
    return (sec * 4096000000LL);        /* (sec * 1000000) << 12      */
}

static INLINE TOD
tod2etod (const TOD tod)
{
    return (tod >> 8);                  /* Adjust bit 51 to bit 59    */
}

static INLINE TOD
tod2ETOD (const TOD tod, ETOD *ETOD)
{
    ETOD->high = tod >>  8;             /* Adjust bit 51 to bit 59    */
    ETOD->low  = tod << 56;
    return (ETOD->high);
}

static INLINE S64
tod2ns (const TOD tod)
{
    return ((S64)((tod * 125) >> 9));
}

static INLINE S64
tod2us (const TOD tod)
{
    return ((S64)(tod >> 12));          /* Adjust bit 51 to bit 63    */
}

static INLINE S64
tod2sec (const TOD tod)
{
    return ((S64)(tod / TOD_SEC));
}

static INLINE void
tod2timespec (const TOD tod, struct timespec* ts)
{
    register S64 work = (S64)(tod - TOD_1970);
    ts->tv_sec  = tod2sec(work);
    ts->tv_nsec = tod2ns(work % TOD_SEC);
}

static INLINE void
tod2timeval (const TOD tod, struct timeval* tv)
{
    register S64 work = (S64)(tod - TOD_1970);
    tv->tv_sec  = tod2sec(work);
    tv->tv_usec = tod2us(work % TOD_SEC);
}

static INLINE TOD
timespec2tod (const struct timespec* tv)
{
    register S64 result;
    result  = sec2tod(tv->tv_sec);
    result += ns2tod(tv->tv_nsec);
    return (result);
}

static INLINE TOD
timeval2tod (const struct timeval* tv)
{
    register S64 result;
    result  = sec2tod(tv->tv_sec);
    result += us2tod(tv->tv_usec);
    return (result);
}

static INLINE S64
timespec2us (const struct timespec* ts)
{
    return ((ts->tv_sec) * 1000000 + ((ts->tv_nsec + 500) / 1000));
}


/*----------------------------------------------------------------------------*/
/* host_tod - Clock fetch and conversion for routines that DO NOT use the     */
/*            synchronized clock services or emulation services (including    */
/*            clock steering), and can tolerate duplicate time stamp          */
/*            generation.                                                     */
/*----------------------------------------------------------------------------*/

static INLINE TOD
host_tod (void)
{
  register TOD  result;
  register U64  temp;

  /* Use the same clock source as host_ETOD; refer to host_ETOD in clock.c for
   * additional comments.
   */

  #if !defined(_MSVC_) && !defined(CLOCK_REALTIME)
  {
    struct timeval time;
    gettimeofday(&time, NULL);      /* Get current host time                  */
    result = time.tv_usec << 4;     /* Adjust microseconds to bit-59          */
    temp   = time.tv_sec;           /* Load seconds                           */
  }
  #else
  {
    struct timespec time;
    clock_gettime(CLOCK_REALTIME, &time);
    result  = time.tv_nsec;         /* Adjust nanoseconds to bit-59 and       */
    result <<= 1;                   /* divide by 1000 (bit-shift compressed)  */
    result  /= 125;                 /* ...                                    */
    temp     = time.tv_sec;         /* Load seconds                           */
   }
  #endif

  temp   *= ETOD_SEC;               /* Convert seconds to ETOD format         */
  result += temp;                   /* Add seconds                            */
  result += ETOD_1970;              /* Adjust to open source epoch of 1970    */
  return ( result );
}


static INLINE TOD
ETOD2tod (const ETOD ETOD)
{
  return ( (ETOD.high << 8) | (ETOD.low >> 56) );
}

static INLINE TOD
ETOD2TOD (const ETOD ETOD)
{
  return ETOD2tod(ETOD);
}

static INLINE U64
ETOD2us (const ETOD ETOD)
{
    return (ETOD.high >> 4);
}

#endif

DLL_EXPORT
void ARCH_DEP(store_int_timer) (REGS *);
void ARCH_DEP(store_int_timer_nolock) (REGS *);
DLL_EXPORT
void ARCH_DEP(fetch_int_timer) (REGS *);

void ARCH_DEP(set_gross_s_rate) (REGS *);
void ARCH_DEP(set_fine_s_rate) (REGS *);
void ARCH_DEP(set_tod_offset) (REGS *);
void ARCH_DEP(adjust_tod_offset) (REGS *);
void ARCH_DEP(query_physical_clock) (REGS *);
void ARCH_DEP(query_steering_information) (REGS *);
void ARCH_DEP(query_tod_offset) (REGS *);
void ARCH_DEP(query_available_functions) (REGS *);


_CLOCK_EXTERN ETOD  tod_value;          /* Bits 0-7 TOD clock epoch     */
                                        /* Bits 8-63 TOD bits 0-55      */
                                        /* Bits 64-111 TOD bits 56-103  */

_CLOCK_EXTERN S64   tod_epoch;          /* Bits 0-7 TOD clock epoch     */
                                        /* Bits 8-63 offset bits 0-55   */

_CLOCK_EXTERN ETOD  hw_tod;             /* Hardware clock               */


#define TOD_CLOCK(_regs) \
    ((tod_value.high & 0x00FFFFFFFFFFFFFFULL) + (_regs)->tod_epoch)

#define CPU_TIMER(_regs) \
    (cpu_timer(_regs))


//----------------------------------------------------------------------
//
// Interval Timer Conversions to/from Extended TOD Clock Values
//
// S/360 - Decrementing at 50 or 60 cycles per second, depending on line
//         frequency, effectively decrementing at 1/300 second in bit
//         position 23.
//
// S/370 - Decrementing at 1/300 second in bit position 23.
//
// Conversions:
//
//         ITIMER -> ETOD = (units * ETOD_SEC) / (300 << 8)
//                        = (units * 16000000) /      76800
//                        = (units *      625) /          3
//
//         ETOD -> ITIMER = (units * (300 << 8)) / ETOD_SEC
//                        = (units *        768) / 16000000
//                        = (units *          3) /      625
//
// References:
//
//  A22-6821-7  IBM System/360 Principles of Operation, Timer Feature,
//              p. 17.1
// GA22-6942-1  IBM System/370 Model 155 Functional Characteristics,
//              Interval Timer, p. 7
//

#define ITIMER_TO_TOD(_units) \
    (((S64)(_units) * 625) / 3)

#define TOD_TO_ITIMER(_units) \
    ((S32)(((S64)(_units) * 3) / 625))

#define INT_TIMER(_regs) \
    ((S32)TOD_TO_ITIMER((S64)((_regs)->int_timer - hw_tod.high)))

#define ITIMER_ACCESS(_addr, _len) \
    (unlikely(unlikely((_addr) < 84) && (((_addr) + (_len)) >= 80)))

#undef ITIMER_UPDATE
#undef ITIMER_SYNC
#if defined(FEATURE_INTERVAL_TIMER)
 #define ITIMER_UPDATE(_addr, _len, _regs)       \
    do {                                         \
        if( ITIMER_ACCESS((_addr), (_len)) )     \
            ARCH_DEP(fetch_int_timer) ((_regs)); \
    } while(0)
 #define ITIMER_SYNC(_addr, _len, _regs)         \
    do {                                         \
        if( ITIMER_ACCESS((_addr), (_len)) )     \
            ARCH_DEP(store_int_timer) ((_regs)); \
    } while (0)
#else
 #define ITIMER_UPDATE(_addr, _len, _regs)
 #define ITIMER_SYNC(_addr, _len, _regs)
#endif
