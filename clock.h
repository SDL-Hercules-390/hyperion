/* CLOCK.H      (C) Copyright Jan Jaeger, 2000-2012                  */
/*              (C) and others 2013-2021                             */
/*              TOD Clock functions                                  */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/* The emulated hardware clock is based on the host clock, adjusted  */
/* by means of an offset and a steering rate.                        */

/* --------------------------------------------------------------------
                    z/Architecture Clock Formats
   --------------------------------------------------------------------



       TOD (SCK, SCKC, STCK, STCKC)
       +--------------------------+------+
       |                          |      |
       +--------------------------+------+
        0                        51     63


   ETOD (STCKE)
   +--+---------------------------+--+------------------------+-----+
   |EI|                           |  |                        | PGF |
   +--+---------------------------+--+------------------------+-----+
   0  7                          59 63                     111    127


   Where:

   - TOD: bit 51 represents one microsecond
   - TOD: bit 63 represents 244 picoseconds

   - ETOD: EI = 8-bit Epoch Index field
   - ETOD: PF = 16-bit Programmable Field (STKPF)
   - ETOD: bit 59 represents one microsecond
   - ETOD: bit 63 represents 62.5 nanoseconds



   --------------------------------------------------------------------
                    Hercules Clock and Timer Formats
   --------------------------------------------------------------------



   64-bit Clock/Timer Format
   +------------------------------+--+
   |                              |  |
   +------------------------------+--+
    0                            59 63


   128-bit Clock Format
   +------------------------------+--+------------------------------+
   |                              |  |                              |
   +------------------------------+--+------------------------------+
    0                            59 63                            127


   Where:

   - Bit 59 represents one microsecond
   - Bit 63 represents 62.5 nanoseconds



   --------------------------------------------------------------------
                            Usage Notes
   --------------------------------------------------------------------

   - Bits 0-63 of the Hercules 64-bit clock format are identical to
     bits 0-63 of the 128-bit Hercules clock format.

   - The 128-bit Hercules clock format extends the 64-bit clock format
     by an additional 64-bits to the right (low-order) of the 64-bit
     Hercules clock format.

   - Hercules timers only use the 64-bit Hercules clock/timer format.

   - The Hercules clock format has a period of over 36,533 years.

   - With masking, the Hercules 128-bit clock format may be used for
     extended TOD clock operations.

   - The ETOD2TOD() call may be used to convert a Hercules 128-bit
     clock value to a standard z/Architecture 64-bit TOD clock value.
*/

#ifndef _CLOCK_H
#define _CLOCK_H

/*-------------------------------------------------------------------*/
/*  non-ARCH_DEP section: compiled only ONCE; applies to ALL archs   */
/*-------------------------------------------------------------------*/

#ifdef WORDS_BIGENDIAN
  struct  ETOD                     { U64 high,  low; };
  #define ETOD_init( _high, _low ) {    _high, _low  }
#else
  struct  ETOD                     { U64 low,  high; };
  #define ETOD_init( _high, _low ) {    _low, _high  }
#endif

typedef struct ETOD  ETOD;
typedef U64           TOD;

/*-------------------------------------------------------------------*/
/* Global clock.c variables */

extern ETOD  hw_tod;
extern S64   tod_epoch;
extern ETOD  tod_value;

/*-------------------------------------------------------------------*/
/* TOD Clock Definitions */

#define TOD_USEC   (4096LL)
#define TOD_SEC    (1000000 * TOD_USEC)
#define TOD_MIN    (60 * TOD_SEC)
#define TOD_HOUR   (60 * TOD_MIN)
#define TOD_DAY    (24 * TOD_HOUR)
#define TOD_YEAR   (365 * TOD_DAY)
#define TOD_LYEAR  (TOD_YEAR + TOD_DAY)
#define TOD_4YEARS (1461 * TOD_DAY)
#define TOD_1970   0x7D91048BCA000000ULL    // TOD base for host epoch of 1970

/* Hercules and Extended TOD Clock Definitions for high-order 64-bits */

#define ETOD_USEC   16LL
#define ETOD_SEC    (1000000 * ETOD_USEC)
#define ETOD_MIN    (60 * ETOD_SEC)
#define ETOD_HOUR   (60 * ETOD_MIN)
#define ETOD_DAY    (24 * ETOD_HOUR)
#define ETOD_YEAR   (365 * ETOD_DAY)
#define ETOD_LYEAR  (ETOD_YEAR + ETOD_DAY)
#define ETOD_4YEARS (1461 * ETOD_DAY)
#define ETOD_1970   0x007D91048BCA0000ULL   // Extended TOD base for host epoch of 1970

/*-------------------------------------------------------------------*/
/* Clock Steering Registers */

struct CSR
{
    U64 start_time;
    S64 base_offset;
    S32 fine_s_rate;
    S32 gross_s_rate;
};
typedef struct CSR  CSR;

/*-------------------------------------------------------------------*/
/* clock format requested for 'etod_clock()' function */

typedef enum
{
  ETOD_raw,
  ETOD_fast,
  ETOD_standard,
  ETOD_extended
}
ETOD_format;

/*-------------------------------------------------------------------*/
/*                  external clock.c functions                       */
/*-------------------------------------------------------------------*/

extern void   csr_reset();                     /* Reset steering regs*/
extern void   set_tod_steering(const double);  /* Set steering rate  */
extern double get_tod_steering();              /* Get steering rate  */
extern U64    update_tod_clock();              /* Update TOD clock   */
extern S64    get_tod_epoch();                 /* Get TOD epoch      */
extern U64    hw_clock();                      /* Get hardware clock */
extern void   set_tod_clock(const U64);        /* Set TOD clock      */
extern int    clock_hsuspend(void *file);      /* Hercules suspend   */
extern int    clock_hresume(void *file);       /* Hercules resume    */
extern int    query_tzoffset();                /* Current TZOFFSET   */

/*-------------------------------------------------------------------*/
/*                clock.h static INLINE functions                    */
/*              (forward references; defined below)                  */
/*-------------------------------------------------------------------*/

static INLINE void ETOD_add  (ETOD* result, const ETOD a, const ETOD b );
static INLINE void ETOD_sub  (ETOD* result, const ETOD a, const ETOD b );
static INLINE void ETOD_shift(ETOD* result, const ETOD a, int shift    );

static INLINE TOD  host_tod();
static INLINE TOD  ETOD2TOD                 ( const ETOD ETOD );
static INLINE TOD  ETOD_high64_to_TOD_high56( const U64  etod );
static INLINE TOD  TOD_high64_to_ETOD_high56( const TOD   tod );
static INLINE S64  ETOD_high64_to_usecs     ( const S64  etod );

static INLINE void usecs2timeval ( const U64 us, struct timeval* tv );
static INLINE S64  timespec2usecs( const struct timespec* ts );
static INLINE TOD  timespec2ETOD ( ETOD* ETOD, const struct timespec* ts );

/*-------------------------------------------------------------------*/

static INLINE
TOD ETOD2TOD( const ETOD ETOD )
{
    return ((ETOD.high << 8) | (ETOD.low >> (64-8)));
}

/*-------------------------------------------------------------------*/

static INLINE
TOD ETOD_high64_to_TOD_high56( const U64 etod )
{
    return (etod << 8);                 /* Adjust bit 59 to bit 51   */
}

/*-------------------------------------------------------------------*/

static INLINE
TOD TOD_high64_to_ETOD_high56( const TOD tod )
{
    return (tod >> 8);                  /* Adjust bit 51 to bit 59   */
}

/*-------------------------------------------------------------------*/

static INLINE S64
ETOD_high64_to_usecs( const S64 etod )
{
    return (etod >> 4);
}

/*-------------------------------------------------------------------*/

static INLINE
void usecs2timeval( const U64 us, struct timeval* tv )
{
    tv->tv_sec  = us / 1000000;
    tv->tv_usec = us % 1000000;
}

/*-------------------------------------------------------------------*/

static INLINE
S64 timespec2usecs( const struct timespec* ts )
{
    return ((ts->tv_sec) * 1000000 + ((ts->tv_nsec + 500) / 1000));
}

/*-------------------------------------------------------------------*/

static INLINE
void ETOD_add( ETOD* result, const ETOD a, const ETOD b )
{
    register U64 high  = a.high + b.high;
    register U64 low   = a.low  + b.low;

    if (low < a.low)
        ++high;

    result->high = high;
    result->low  = low;
}

/*-------------------------------------------------------------------*/

static INLINE
void ETOD_sub( ETOD* result, const ETOD a, const ETOD b )
{
    register U64 high  = a.high - b.high;
    register U64 low   = a.low  - b.low;

    if (a.low < b.low)
        --high;

    result->high = high;
    result->low  = low;
}

/*-------------------------------------------------------------------*/

static INLINE
void ETOD_shift( ETOD* result, const ETOD a, int shift )
{
    register U64 high;
    register U64 low;

    if (shift == 0)                 /* Zero shift: copy in to out */
    {
        high = a.high;
        low  = a.low;
    }
    else if (shift < 0)             /* Negtative: shift LEFT */
    {
        shift = -shift;             /* (get positive shift amount) */

        if (shift >= 64)            /* Negtative: shift LEFT >= 64 */
        {
            shift -= 64;            /* (get shift amount 0-64) */

            if (shift == 0)
                high = a.low;
            else if (shift > 64)
                high = 0;
            else
                high = a.low << shift;

            low = 0;
        }
        else                        /* Negtative: shift LEFT < 64 */
        {
            high = a.high << shift | a.low >> (64 - shift);
            low  = a.low << shift;
        }
    }
    else if (shift >= 64)           /* Positive: shift RIGHT >= 64 */
    {
        shift -= 64;                /* (get shift amount 0-64) */
        high   = 0;

        if (shift == 0)
            low = a.high;
        else if (shift < 64)
            low = a.high >> shift;
        else
            low = 0;
    }
    else                            /* Positive: shift RIGHT < 64 */
    {
        high = a.high >> shift;
        low  = a.high << (64 - shift) | a.low >> shift;
    }

    result->low  = low;
    result->high = high;
}

/*-------------------------------------------------------------------*/
/*                      clock_gettime                                */
/*-------------------------------------------------------------------*/
/* (for systems not supporting nanosecond clocks other than Windows) */
/*-------------------------------------------------------------------*/

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


static INLINE
int clock_gettime ( clockid_t clk_id, struct timespec *ts )
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


        /* Can this be ignored until shutdown of Hercules? */
//      result = mach_port_deallocate(mach_task_self(), cserv);


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
#endif /* defined(__APPLE__) && 0 */

    return ( result );
}

#elif !(defined(TOD_FULL_PRECISION)     || \
        defined(TOD_120BIT_PRECISION)   || \
        defined(TOD_64BIT_PRECISION)    || \
        defined(TOD_MIN_PRECISION)      || \
        defined(TOD_95BIT_PRECISION))

    /* Define default clock precision as 95-bits
       (only one division required)
    */
    #define TOD_95BIT_PRECISION

#endif /* !defined(_MSVC_) && !defined(CLOCK_REALTIME) */

/*-------------------------------------------------------------------*/

static INLINE
TOD timespec2ETOD( ETOD* ETOD, const struct timespec* ts )
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

#else /* 95-bit resolution */

    {
        ETOD->low <<= 32;           /* Place nanoseconds in high-order word   */
        ETOD->low  /= 125;          /* Divide by 1000 (125 * 2^3)             */
        ETOD->high += ETOD->low >> 31;  /* Adjust and add microseconds and    */
                                    /* first nibble of nanosecond remainder   */
                                    /* to bits 0-63                           */
        ETOD->low <<= 33;           /* Adjust remaining nanosecond fraction   */
                                    /* to bits 64-93                          */
    }

#endif /* defined(TOD_FULL_PRECISION) || .... */

    return ( ETOD->high );          /* Return address of result               */
}

/*-------------------------------------------------------------------*/
/*                           host_tod                                */
/*-------------------------------------------------------------------*/
/* Clock fetch and conversion for routines that DO NOT use the       */
/* synchronized clock services or emulation services (including      */
/* clock steering) and can tolerate duplicate time stamp generation. */
/*-------------------------------------------------------------------*/
static INLINE
TOD host_tod()
{
  register TOD  result;
  register U64  temp;

  /* Use the same clock source as host_ETOD().
     Refer to host_ETOD() in clock.c for additional comments.
   */

#if !defined( _MSVC_ ) && !defined( CLOCK_REALTIME )
  {
    struct timeval time;
    gettimeofday( &time, NULL );    /* Get current host time         */
    result = time.tv_usec << 4;     /* Adjust microseconds to bit-59 */
    temp   = time.tv_sec;           /* Load seconds                  */
  }
#else
  {
    struct timespec time;
    clock_gettime( CLOCK_REALTIME, &time );
    result  = time.tv_nsec;         /* Adjust nanoseconds to bit-59  */
    result <<= 1;                   /* and divide by 1000
                                       (bit-shift compressed)        */
    result  /= 125;                 /* ...                           */
    temp     = time.tv_sec;         /* Load seconds                  */
   }
#endif /* !defined( _MSVC_ ) && !defined( CLOCK_REALTIME ) */

  temp   *= ETOD_SEC;               /* Convert seconds to ETOD fmt   */
  result += temp;                   /* Add seconds                   */
  result += ETOD_1970;              /* Adjust to epoch 1970          */
  return ( result );
}

#endif // _CLOCK_H

/*-------------------------------------------------------------------*/
/*   ARCH_DEP section: compiled multiple times, once for each arch.  */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/*                  external clock.c functions                       */
/*                    (defined in clock.c)                           */
/*-------------------------------------------------------------------*/

CLOCK_DLL_IMPORT    void ARCH_DEP( fetch_int_timer )       ( REGS* );
CLOCK_DLL_IMPORT    void ARCH_DEP( store_int_timer )       ( REGS* );
extern              void ARCH_DEP( store_int_timer_locked )( REGS* );

extern              void ARCH_DEP(set_gross_s_rate) (REGS *);
extern              void ARCH_DEP(set_fine_s_rate) (REGS *);
extern              void ARCH_DEP(set_tod_offset) (REGS *);
extern              void ARCH_DEP(set_tod_offset_user) (REGS *);
extern              void ARCH_DEP(adjust_tod_offset) (REGS *);
extern              void ARCH_DEP(query_physical_clock) (REGS *);
extern              void ARCH_DEP(query_steering_information) (REGS *);
extern              void ARCH_DEP(query_tod_offset) (REGS *);
extern              void ARCH_DEP(query_tod_offset_user) (REGS *);
extern              void ARCH_DEP(query_available_functions) (REGS *);
extern              void ARCH_DEP(query_utc_information) (REGS *);
#if defined( FEATURE_139_MULTIPLE_EPOCH_FACILITY )
extern              void ARCH_DEP(query_steering_information_extended) (REGS *);
extern              void ARCH_DEP(query_tod_offset_user_extended) (REGS *);
extern              void ARCH_DEP(set_tod_offset_extended) (REGS *);
extern              void ARCH_DEP(set_tod_offset_user_extended) (REGS *);
#endif

CLOCK_DLL_IMPORT    TOD  etod_clock( REGS*, ETOD*,          /* Get extended TOD clock    */
                                     ETOD_format );
extern              TOD  get_tod_clock(REGS *);             /* Get TOD clock non-unique  */
extern              S64  get_cpu_timer(REGS *);             /* Retrieve CPU timer        */
extern              void set_cpu_timer(REGS *, const S64);  /* Set CPU timer             */
extern              void set_int_timer(REGS *, const S32);  /* Set interval timer        */
extern              int  chk_int_timer(REGS *);             /* Check int_timer pending   */
extern              TOD  thread_cputime(const REGS*);       /* Thread real CPU used (TOD)*/
extern              U64  thread_cputime_us(const REGS*);    /* Thread real CPU used (us) */

//----------------------------------------------------------------------
//                  Interval Timer Conversions
//              to/from Extended TOD Clock Values
//----------------------------------------------------------------------
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
// GA22-6821-7  IBM System/360 Principles of Operation, Timer Feature,
//              p. 17.1
// GA22-6942-1  IBM System/370 Model 155 Functional Characteristics,
//              Interval Timer, p. 7
//
//----------------------------------------------------------------------

#define CPU_TIMER(_regs)            (get_cpu_timer(_regs))
#define ITIMER_TO_TOD(_units)       (((S64)(_units) * 625) / 3)
#define TOD_TO_ITIMER(_units)       ((S32)(((S64)(_units) * 3) / 625))

#define TOD_CLOCK(_regs)            \
                                    \
    ((tod_value.high & 0x00FFFFFFFFFFFFFFULL) + (_regs)->tod_epoch)

#define INT_TIMER(_regs)            \
                                    \
    ((S32)TOD_TO_ITIMER((S64)((_regs)->int_timer - hw_tod.high)))

#define ITIMER_ACCESS(_addr, _len)  \
                                    \
    (unlikely(unlikely((_addr) < 84) && (((_addr) + (_len)) >= 80)))

/*-------------------------------------------------------------------*/

#undef ITIMER_UPDATE
#undef ITIMER_SYNC

#if defined( FEATURE_INTERVAL_TIMER )

 #define ITIMER_UPDATE(_addr, _len, _regs)                            \
    do {                                                              \
        if( ITIMER_ACCESS((_addr), (_len)) )                          \
            ARCH_DEP(fetch_int_timer) ((_regs));                      \
    } while(0)

 #define ITIMER_SYNC(_addr, _len, _regs)                              \
    do {                                                              \
        if( ITIMER_ACCESS((_addr), (_len)) )                          \
            ARCH_DEP(store_int_timer) ((_regs));                      \
    } while (0)

#else

 #define ITIMER_UPDATE(_addr, _len, _regs)
 #define ITIMER_SYNC(_addr, _len, _regs)

#endif /* defined( FEATURE_INTERVAL_TIMER ) */

/*-------------------------------------------------------------------*/
