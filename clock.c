/* CLOCK.C      (c) Copyright Jan Jaeger, 2000-2012                  */
/*              TOD Clock functions                                  */

/* The emulated hardware clock is based on the host clock, adjusted  */
/* by means of an offset and a steering rate.                        */

/*

   Hercules Clock and Timer Formats

   64-bit Clock/Timer Format
   +------------------------------+--+
   |                              |  |
   +------------------------------+--+
    0                           59 63

   128-bit Clock Format
   +------------------------------+--+------------------------------+
   |                              |  |                              |
   +------------------------------+--+------------------------------+
    0                           59 63                            127

   where:

   - Bit-59 represents one microsecond
   - Bit-63 represents 62.5 nanoseconds


   Usage Notes:

   - Bits 0-63 of the 64-bit clock format are identical to bits 0-63
     of the 128-bit clock format

   - The 128-bit clock format extends the 64-bit clock format by an
     additional 64-bits to the right (low-order) of the 64-bit clock
     format.

   - Hercules timers only use the 64-bit clock/timer format.

   - The Hercules clock format has a period of over 36,533 years.

   - With masking, the 128-bit clock format may be used for extended TOD clock
     operations.

   - The ETOD2TOD() call may be used to convert a Hercules 128-bit clock value
     to a standard TOD clock value.

*/

#include "hstdinc.h"

DISABLE_GCC_WARNING( "-Wunused-function" )

#if !defined(_HENGINE_DLL_)
#define _HENGINE_DLL_
#endif

#include "hercules.h"
#include "opcode.h"
#include "inline.h"
#include "sr.h"

#if !defined(_CLOCK_C_)
#define _CLOCK_C_

#include "clock.h"


/*----------------------------------------------------------------------------*/
/* host_ETOD - Primary high-resolution clock fetch and conversion             */
/*----------------------------------------------------------------------------*/

ETOD*
host_ETOD (ETOD* ETOD)
{
    struct timespec time;

    /* Should use CLOCK_MONOTONIC + adjustment, but host sleep/hibernate
     * destroys consistent monotonic clock.
     */

    clock_gettime(CLOCK_REALTIME, &time);
    timespec2ETOD(ETOD, &time);
    return ( ETOD );                /* Return address of result               */
}


// static int clock_state = CC_CLOCK_SET;

static CSR episode_old;
static CSR episode_new;
static CSR *episode_current = &episode_new;

void csr_reset()
{
    episode_new.start_time = 0;
    episode_new.base_offset = 0;
    episode_new.fine_s_rate = 0;
    episode_new.gross_s_rate = 0;
    episode_current = &episode_new;
    episode_old = episode_new;
}

static ETOD universal_tod;
static TOD universal_clock(void) /* really: any clock used as a base */
{
    host_ETOD(&universal_tod);
    return (universal_tod.high);
}

#if 0
dead code
static TOD universal_clock_extended(ETOD* ETOD)
{
    host_ETOD(ETOD);
    return (ETOD->high);
}
#endif


/* The hercules hardware clock, based on the universal clock, but    */
/* running at its own speed as optionally set by set_tod_steering()  */
/* The hardware clock returns a unique value                         */
static double hw_steering = 0.0;  /* Current TOD clock steering rate */
static TOD hw_episode;           /* TOD of start of steering episode */
static S64 hw_offset = 0;       /* Current offset between TOD and HW */
// static ETOD hw_tod = {0, 0};       /* Globally defined in clock.h */
static ETOD hw_unique_clock_tick = {0, 0};

static TOD hw_adjust(TOD base_tod);

static TOD
hw_calculate_unique_tick (void)
{
    static const ETOD     m1  = ETOD_init(0,65536);

    ETOD          temp;
    register TOD  result;
    register int  n;

    temp.high = universal_tod.high;
    temp.low  = universal_tod.low;
    hw_unique_clock_tick.low = 1;
    for (n = 0; n < 65536; ++n)
        result = hw_adjust(universal_clock());
    ETOD_sub(&temp, universal_tod, temp);
    ETOD_sub(&temp, temp, m1);
    ETOD_shift(&hw_unique_clock_tick, temp, 16);
    if (hw_unique_clock_tick.low  == 0 &&
        hw_unique_clock_tick.high == 0)
        hw_unique_clock_tick.high = 1;
    #if defined(TOD_95BIT_PRECISION) || \
        defined(TOD_64BIT_PRECISION) || \
        defined(TOD_MIN_PRECISION)
    else
    {
        static const ETOD   adj =
        #if defined(TOD_95BIT_PRECISION)
            ETOD_init(0,0x0000000100000000ULL);
        #else
            ETOD_init(0,0x8000000000000000ULL);
        #endif

        ETOD_add(&hw_unique_clock_tick, hw_unique_clock_tick, adj);

        #if defined(TOD_95BIT_PRECISION)
            hw_unique_clock_tick.low &= 0xFFFFFFFE00000000ULL;
        #else
            hw_unique_clock_tick.low = 0;
        #endif
    }
    #endif

    return ( result );
}


static TOD hw_adjust(TOD base_tod)
{
    /* Apply hardware offset, this is the offset achieved by all
       previous steering episodes */
    base_tod += hw_offset;

    /* Apply the steering offset from the current steering episode */
    /* TODO: Shift resolution to permit adjustment by less than 62.5
     *       nanosecond increments (1/16 microsecond).
     */
    base_tod += (S64)(base_tod - hw_episode) * hw_steering;

    /* Ensure that the clock returns a unique value */
    if (hw_tod.high < base_tod)
        hw_tod.high = base_tod,
        hw_tod.low  = universal_tod.low;
    else if (hw_unique_clock_tick.low  == 0 &&
             hw_unique_clock_tick.high == 0)
        hw_calculate_unique_tick();
    else
        ETOD_add(&hw_tod, hw_tod, hw_unique_clock_tick);

    return ( hw_tod.high );
}


static TOD hw_clock_l(void)
{
    /* Get time of day (GMT); adjust speed and ensure uniqueness */
    return ( hw_adjust(universal_clock()) );
}


TOD hw_clock(void)
{
register TOD temp_tod;

    obtain_lock(&sysblk.todlock);

    /* Get time of day (GMT); adjust speed and ensure uniqueness */
    temp_tod = hw_clock_l();

    release_lock(&sysblk.todlock);

    return (temp_tod);
}



/* set_tod_steering(double) sets a new steering rate.                */
/* When a new steering episode begins, the offset is adjusted,       */
/* and the new steering rate takes effect                            */
void set_tod_steering(const double steering)
{
    obtain_lock(&sysblk.todlock);

    /* Get current offset between hw_adjust and universal TOD value  */
    hw_offset = hw_clock_l() - universal_tod.high;

    hw_episode = hw_tod.high;
    hw_steering = steering;
    release_lock(&sysblk.todlock);
}


/* Start a new episode */
static INLINE void start_new_episode()
{
    hw_offset = hw_tod.high - universal_tod.high;
    hw_episode = hw_tod.high;
    episode_new.start_time = hw_episode;
    /* TODO: Convert to binary arithmetic to avoid floating point conversions */
    hw_steering = ldexp(2,-44) *
                  (S32)(episode_new.fine_s_rate + episode_new.gross_s_rate);
    episode_current = &episode_new;
}


/* Prepare for a new episode */
static INLINE void prepare_new_episode()
{
    if(episode_current == &episode_new)
    {
        episode_old = episode_new;
        episode_current = &episode_old;
    }
}


/* Ajust the epoch for all active cpu's in the configuration */
static U64 adjust_epoch_cpu_all(const U64 epoch)
{
int cpu;

    /* Update the TOD clock of all CPU's in the configuration
       as we simulate 1 shared TOD clock, and do not support the
       TOD clock sync check */
    for (cpu = 0; cpu < sysblk.maxcpu; cpu++)
    {
        obtain_lock(&sysblk.cpulock[cpu]);
        if (IS_CPU_ONLINE(cpu))
            sysblk.regs[cpu]->tod_epoch = epoch;
        release_lock(&sysblk.cpulock[cpu]);
    }

    return epoch;
}


double get_tod_steering(void)
{
    return hw_steering;
}


void set_tod_epoch(const S64 epoch)
{
    obtain_lock(&sysblk.todlock);
    csr_reset();
    tod_epoch = epoch;
    release_lock(&sysblk.todlock);
    adjust_epoch_cpu_all(epoch);
}


void adjust_tod_epoch(const S64 epoch)
{
    obtain_lock(&sysblk.todlock);
    csr_reset();
    tod_epoch += epoch;
    release_lock(&sysblk.todlock);
    adjust_epoch_cpu_all(tod_epoch);
}


void set_tod_clock(const U64 tod)
{
    set_tod_epoch(tod - hw_clock());
}


S64 get_tod_epoch()
{
    return tod_epoch;
}


static void set_gross_steering_rate(const S32 gsr)
{
    obtain_lock(&sysblk.todlock);
    prepare_new_episode();
    episode_new.gross_s_rate = gsr;
    release_lock(&sysblk.todlock);
}


static void set_fine_steering_rate(const S32 fsr)
{
    obtain_lock(&sysblk.todlock);
    prepare_new_episode();
    episode_new.fine_s_rate = fsr;
    release_lock(&sysblk.todlock);
}


static void set_tod_offset(const S64 offset)
{
    obtain_lock(&sysblk.todlock);
    prepare_new_episode();
    episode_new.base_offset = offset;
    release_lock(&sysblk.todlock);
}


static void adjust_tod_offset(const S64 offset)
{
    obtain_lock(&sysblk.todlock);
    prepare_new_episode();
    episode_new.base_offset = episode_old.base_offset + offset;
    release_lock(&sysblk.todlock);
}


/* The CPU timer is internally kept as an offset to the thread CPU time
 * used in LPAR mode, or TOD clock in BASIC mode. The CPU timer counts
*  down as the clock approaches the timer epoch.
 *
 * To be in agreement with reporting of time in association with real
 * diagnose code x'204' for partition management and resource reporting,
 * only user time is considered as CPU time used. System time is
 * considered to be overhead time of the system (partition overhead or
 * management time).
 */

TOD
thread_cputime(const REGS *regs)
{
    register TOD    result;

    struct rusage   rusage;
    int             rc;

    rc = getrusage((int)sysblk.cputid[regs->cpuad], &rusage);
    if (unlikely(rc == -1))
        result = host_tod();
    else
        result  = timeval2etod(&rusage.ru_utime);

    return (result);
}


U64
thread_cputime_us(const REGS *regs)
{
    U64 result;

    struct rusage   rusage;
    int             rc;

    rc = getrusage((int)sysblk.cputid[regs->cpuad], &rusage);
    if (unlikely(rc == -1))
        result = etod2us(host_tod());
    else
        result  = timeval2us(&rusage.ru_utime);

    return (result);
}


static INLINE void
set_cpu_timer_epoch(REGS *regs, const TOD epoch)
{
    register REGS*  guestregs = regs->guestregs;
    register REGS*  hostregs  = regs->hostregs;

    /* Set CPU timer epoch */
    regs->cpu_timer_epoch   = epoch;

    /* Reset epoch value for host and guest */
    if (hostregs && hostregs != regs)
        hostregs->cpu_timer_epoch   = epoch;
    if (guestregs && guestregs != regs)
        guestregs->cpu_timer_epoch  = epoch;
}


static INLINE S64
cpu_timer_update(REGS *regs, TOD new_epoch)
{
    register S64    interval = (S64)(new_epoch - regs->cpu_timer_epoch);
    register S64    result = regs->cpu_timer;

    if (interval > 0)
    {
        result                 -= interval;
        regs->cpu_timer_epoch   = new_epoch;
        regs->cpu_timer         = result;
    }

    return (result);
}


static INLINE void
cpu_timer_update_linked(REGS *regs, REGS *linked_regs, TOD new_epoch)
{
    if (linked_regs && linked_regs != regs)
        cpu_timer_update(linked_regs, new_epoch);
}


static INLINE TOD
mode_cputime(REGS *regs)
{
    return (regs->cpu_timer_mode ? thread_cputime(regs) : host_tod());
}


static INLINE int
cpu_timer_mode(REGS *regs)
{
    return (sysblk.lparmode && !WAITSTATE(&regs->psw));
}


void
set_cpu_timer_mode(REGS *regs)
{
    int newmode = cpu_timer_mode(regs);

    /* Update CPU timer epoch if changing mode */
    if ((U32)newmode != regs->cpu_timer_mode)
    {
        cpu_timer(regs);
        regs->cpu_timer_mode = newmode;
        set_cpu_timer_epoch(regs, mode_cputime(regs));
    }
}


S64 set_cpu_timer(REGS *regs, const TOD timer)
{
    regs->cpu_timer_mode = cpu_timer_mode(regs);

    /* Prepare new timer value and epoch value */
    regs->cpu_timer         = tod2etod(timer);
    set_cpu_timer_epoch(regs, mode_cputime(regs));

    return (timer);
}


void set_cpu_timers(REGS *hostregs, const TOD host_timer, REGS *regs, const TOD timer)
{
    set_cpu_timer(hostregs, host_timer);
    if (regs != hostregs)
    {
        regs->cpu_timer = tod2etod(timer);
        regs->cpu_timer_epoch   = hostregs->cpu_timer_epoch;
    }
}

void save_cpu_timers(REGS *hostregs, TOD *host_timer, REGS *regs, TOD *timer)
{
    *host_timer = cpu_timer(hostregs);
    *timer = (regs == hostregs) ?
              *host_timer : (TOD)cpu_timer_SIE(regs);
}


S64 cpu_timer(REGS *regs)
{
    register TOD    new_epoch    = mode_cputime(regs);
    register U64    new_epoch_us = etod2us(new_epoch);
    register S64    result;

    /* If no change from epoch, don't bother updating */
    if (new_epoch <= regs->cpu_timer_epoch)
    {
        result = regs->cpu_timer;
    }
    /* If CPU is stopped, return the CPU timer without updating */
    else if (regs->cpustate == CPUSTATE_STOPPED)
    {
        result = regs->cpu_timer;

        /* Update base CPU time epoch */
        regs->bcputime = new_epoch_us;

        /* Update CPU timer epoch */
        set_cpu_timer_epoch(regs, new_epoch);
    }
    else /* Update and return the CPU timer */
    {
        /* Update real CPU time used and base CPU time epoch */
        regs->rcputime += new_epoch_us - regs->bcputime;
        regs->bcputime  = new_epoch_us;

        /* Process CPU timers */
        result = cpu_timer_update(regs, new_epoch);
        cpu_timer_update_linked(regs, regs->hostregs, new_epoch);
        cpu_timer_update_linked(regs, regs->guestregs, new_epoch);
    }

    /* Change from ETOD format to TOD format */
    result = (S64)etod2tod(result);

    return (result);
}


S64 cpu_timer_SIE(REGS *regs)
{
    register S64    result;

    /* Ensure SIE guestregs are in use */
    if (regs == regs->hostregs)
        regs = regs->guestregs;

    result = etod2tod(regs->cpu_timer);
    return (result);
}


DLL_EXPORT
TOD etod_clock(REGS *regs, ETOD* ETOD, ETOD_format format)
{
    /* STORE CLOCK and STORE CLOCK EXTENDED values must be in ascending
     * order for comparison. Consequently, swap delays for a subsequent
     * STORE CLOCK, STORE CLOCK EXTENDED, or TRACE instruction may be
     * introduced when a STORE CLOCK value is advanced due to the use of
     * the CPU address in bits 66-71.
     *
     * If the regs pointer is null, then the request is a raw request,
     * and the format operand should specify ETOD_raw or ETOD_fast. For
     * raw and fast requests, the CPU address is not inserted into the
     * returned value.
     *
     * A spin loop is used for the introduction of the delay, moderated
     * by obtaining and releasing of the TOD lock. This permits raw and
     * fast clock requests to complete without additional delay.
     */

    U64 high;
    U64 low;
    U8  swapped = 0;

    do
    {
        obtain_lock(&sysblk.todlock);

        high = hw_clock_l();
        low  = hw_tod.low;

        /* If we are in the old episode, and the new episode has arrived
         * then we must take action to start the new episode.
         */
        if (episode_current == &episode_old)
            start_new_episode();

        /* Set the clock to the new updated value with offset applied */
        high += episode_current->base_offset;

        /* Place CPU stamp into clock value for Standard and Extended
         * formats (raw or fast requests fall through)
         */
        if (regs && format >= ETOD_standard)
        {
            register U64    cpuad;
            register U64    amask;
            register U64    lmask;

            /* Set CPU address masks */
            if (sysblk.maxcpu <= 64)
                amask = 0x3F, lmask = 0xFFFFFFFFFFC00000ULL;
            else if (sysblk.maxcpu <= 128)
                amask = 0x7F, lmask = 0xFFFFFFFFFF800000ULL;
            else /* sysblk.maxcpu <= 256) */
                amask = 0xFF, lmask = 0xFFFFFFFFFF000000ULL;

            /* Clean CPU address */
            cpuad = (U64)regs->cpuad & amask;

            switch (format)
            {
                /* Standard TOD format */
                case ETOD_standard:
                    low &= lmask << 40;
                    low |= cpuad << 56;
                    break;

                /* Extended TOD format */
                case ETOD_extended:
                    low &= lmask;
                    low |= cpuad << 16;
                    if (low == 0)
                        low = (amask + 1) << 16;
                    low |= regs->todpr;
                    break;
                default:
                    ASSERT(0); /* unexpected */
                    break;
            }
        }

        if (/* New clock value > Old clock value   */
            high > tod_value.high       ||
            (high == tod_value.high &&
            low > tod_value.low)        ||
            /* or Clock Wrap                       */
            unlikely(unlikely((tod_value.high & 0x8000000000000000ULL) == 0x8000000000000000ULL &&
                              (          high & 0x8000000000000000ULL) == 0)))
        {
            tod_value.high = high;
            tod_value.low  = low;
            swapped = 1;
        }
        else if (format <= ETOD_fast)
        {
            high = tod_value.high;
            low  = tod_value.low;
            swapped = 1;
        }

        if (swapped)
        {
            ETOD->high = high += regs->tod_epoch;
            ETOD->low  = low;
        }

        release_lock(&sysblk.todlock);

    } while (!swapped);

    return ( high );
}


TOD
tod_clock (REGS* regs)
{
    ETOD    ETOD;
    return ( etod_clock(regs, &ETOD, ETOD_fast) );
}

#if defined(_FEATURE_INTERVAL_TIMER)


#if defined(_FEATURE_ECPSVM)
static INLINE S32 ecps_vtimer(const REGS *regs)
{
    return (S32)TOD_TO_ITIMER((S64)(regs->ecps_vtimer - hw_clock()));
}


static INLINE void set_ecps_vtimer(REGS *regs, const S32 vtimer)
{
    regs->ecps_vtimer = (U64)(hw_clock() + ITIMER_TO_TOD(vtimer));
    regs->ecps_oldtmr = vtimer;
}
#endif /*defined(_FEATURE_ECPSVM)*/


static INLINE S32 int_timer(const REGS *regs)
{
    return (S32)TOD_TO_ITIMER((S64)(regs->int_timer - hw_clock()));
}


void set_int_timer(REGS *regs, const S32 itimer)
{
    regs->int_timer = (U64)(hw_clock() + ITIMER_TO_TOD(itimer));
    regs->old_timer = itimer;
}


int chk_int_timer(REGS *regs)
{
S32 itimer;
int pending = 0;

    itimer = int_timer(regs);
    if(itimer < 0 && regs->old_timer >= 0)
    {
        ON_IC_ITIMER(regs);
        pending = 1;
        regs->old_timer=itimer;
    }
#if defined(_FEATURE_ECPSVM)
    if(regs->ecps_vtmrpt)
    {
        itimer = ecps_vtimer(regs);
        if(itimer < 0 && regs->ecps_oldtmr >= 0)
        {
            ON_IC_ECPSVTIMER(regs);
            pending += 2;
        }
    }
#endif /*defined(_FEATURE_ECPSVM)*/

    return pending;
}
#endif /*defined(_FEATURE_INTERVAL_TIMER)*/

/*
 *  is_leapyear ( year )
 *
 *  Returns:
 *
 *  0 - Specified year is NOT a leap year
 *  1 - Specified year is a leap year
 *
 *
 *  Algorithm:
 *
 *    if year modulo 400 is 0 then
 *       is_leap_year
 *    else if year modulo 100 is 0 then
 *       not_leap_year
 *    else if year modulo 4 is 0 then
 *       is_leap_year
 *    else
 *       not_leap_year
 *
 *
 *  Notes and Restrictions:
 *
 *  1) In reality, only valid for years 1582 and later. 1582 was the
 *     first year of Gregorian calendar; actual years are dependent upon
 *     year of acceptance by any given government and/or agency. For
 *     example, Britain and the British empire did not adopt the
 *     calendar until 1752; Alaska did not adopt the calendar until
 *     1867.
 *
 *  2) Minimum validity period for algorithm is 3,300 years after 1582
 *     (4882), at which point the calendar may be off by one full day.
 *
 *  3) Most likely invalid for years after 8000 due to unpredictability
 *     in the earth's long-time rotational changes.
 *
 *  4) For our purposes, year zero is treated as a leap year.
 *
 *
 *  References:
 *
 *  http://scienceworld.wolfram.com/astronomy/LeapYear.html
 *  http://www.timeanddate.com/date/leapyear.html
 *  http://www.usno.navy.mil/USNO/astronomical-applications/
 *         astronomical-information-center/leap-years
 *  http://en.wikipedia.org/wiki/Leap_year
 *  http://en.wikipedia.org/wiki/0_(year)
 *  http://en.wikipedia.org/wiki/1_BC
 *  http://en.wikipedia.org/wiki/Proleptic_calendar
 *  http://en.wikipedia.org/wiki/Proleptic_Julian_calendar
 *  http://en.wikipedia.org/wiki/Proleptic_Gregorian_calendar
 *  http://dotat.at/tmp/ISO_8601-2004_E.pdf
 *  http://tools.ietf.org/html/rfc3339
 *
 */

static INLINE unsigned int
is_leapyear ( const unsigned int year )
{
    return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
}

static INLINE S64 lyear_adjust(const int epoch)
{
int year, leapyear;
TOD tod = hw_clock();

    if(tod >= TOD_YEAR)
    {
        tod -= TOD_YEAR;
        year = (tod / TOD_4YEARS * 4) + 1;
        tod %= TOD_4YEARS;
        if((leapyear = tod / TOD_YEAR) == 4)
            year--;
        year += leapyear;
    }
    else
       year = 0;

    if(epoch > 0)
        return ( ((!is_leapyear(year)) && (((year % 4) - (epoch % 4)) <= 0)) ? -TOD_DAY : 0 );
    else
        return ( ((is_leapyear(year) && (-epoch % 4) != 0) || ((year % 4) + (-epoch % 4) > 4)) ? TOD_DAY : 0 );
}


int default_epoch = 1900;
int default_yroffset = 0;
int default_tzoffset = 0;


static INLINE void configure_time()
{
int epoch;
S64 ly1960;

    /* Set up the system TOD clock offset: compute the number of
     * microseconds offset to 0000 GMT, 1 January 1900.
     */

    if( (epoch = default_epoch) == 1960 )
        ly1960 = ETOD_DAY;
    else
        ly1960 = 0;

    epoch -= 1900 + default_yroffset;

    set_tod_epoch(((epoch*365+(epoch/4))*-ETOD_DAY)+lyear_adjust(epoch)+ly1960);

    /* Set the timezone offset */
    adjust_tod_epoch((((default_tzoffset / 100) * 60) + /* Hours -> Minutes       */
                      (default_tzoffset % 100)) *       /* Minutes                */
                     ETOD_MIN);                         /* Convert to ETOD format */
}


/*-------------------------------------------------------------------*/
/* epoch     1900|1960                                               */
/*-------------------------------------------------------------------*/
int configure_epoch(int epoch)
{
    if(epoch != 1900 && epoch != 1960)
        return -1;

    default_epoch = epoch;

    configure_time();

    return 0;
}


/*-------------------------------------------------------------------*/
/* yroffset  +|-142                                                  */
/*-------------------------------------------------------------------*/
int configure_yroffset(int yroffset)
{
    if(yroffset < -142 || yroffset > 142)
        return -1;

    default_yroffset = yroffset;

    configure_time();

    return 0;
}


/*-------------------------------------------------------------------*/
/* tzoffset  -2359..+2359                                            */
/*-------------------------------------------------------------------*/
int configure_tzoffset(int tzoffset)
{
    if(tzoffset < -2359 || tzoffset > 2359)
        return -1;

    default_tzoffset = tzoffset;

    configure_time();

    return 0;
}


/*-------------------------------------------------------------------*/
/* Query current tzoffset value for reporting                        */
/*-------------------------------------------------------------------*/
int query_tzoffset(void)
{
    return default_tzoffset;
}


/*-------------------------------------------------------------------*/
/* Update TOD clock                                                  */
/*                                                                   */
/* This function updates the TOD clock.                              */
/*                                                                   */
/* This function is called by timer_update_thread and by cpu_thread  */
/* instructions that manipulate any of the timer related entities    */
/* (clock comparator, cpu timer and interval timer).                 */
/*                                                                   */
/* Internal function `check_timer_event' is called which will signal */
/* any timer related interrupts to the appropriate cpu_thread.       */
/*                                                                   */
/* Callers *must* own the todlock and *must not* own the intlock.    */
/*                                                                   */
/* update_tod_clock() returns the tod delta, by which the cpu timer  */
/* has been adjusted.                                                */
/*                                                                   */
/*-------------------------------------------------------------------*/
// static ETOD tod_value;
TOD update_tod_clock(void)
{
    TOD new_clock;

    obtain_lock(&sysblk.todlock);

    new_clock = hw_clock_l();

    /* If we are in the old episode, and the new episode has arrived
       then we must take action to start the new episode */
    if (episode_current == &episode_old)
        start_new_episode();

    /* Set the clock to the new updated value with offset applied */
    new_clock += episode_current->base_offset;
    tod_value.high = new_clock;
    tod_value.low  = hw_tod.low;

    release_lock(&sysblk.todlock);

    /* Update the timers and check if either a clock related event has
       become pending */
    update_cpu_timer();

    return new_clock;
}

#define SR_SYS_CLOCK_CURRENT_CSR          ( SR_SYS_CLOCK | 0x001 )
#define SR_SYS_CLOCK_UNIVERSAL_TOD        ( SR_SYS_CLOCK | 0x002 )
#define SR_SYS_CLOCK_HW_STEERING          ( SR_SYS_CLOCK | 0x004 )
#define SR_SYS_CLOCK_HW_EPISODE           ( SR_SYS_CLOCK | 0x005 )
#define SR_SYS_CLOCK_HW_OFFSET            ( SR_SYS_CLOCK | 0x006 )

#define SR_SYS_CLOCK_OLD_CSR              ( SR_SYS_CLOCK | 0x100 )
#define SR_SYS_CLOCK_OLD_CSR_START_TIME   ( SR_SYS_CLOCK | 0x101 )
#define SR_SYS_CLOCK_OLD_CSR_BASE_OFFSET  ( SR_SYS_CLOCK | 0x102 )
#define SR_SYS_CLOCK_OLD_CSR_FINE_S_RATE  ( SR_SYS_CLOCK | 0x103 )
#define SR_SYS_CLOCK_OLD_CSR_GROSS_S_RATE ( SR_SYS_CLOCK | 0x104 )

#define SR_SYS_CLOCK_NEW_CSR              ( SR_SYS_CLOCK | 0x200 )
#define SR_SYS_CLOCK_NEW_CSR_START_TIME   ( SR_SYS_CLOCK | 0x201 )
#define SR_SYS_CLOCK_NEW_CSR_BASE_OFFSET  ( SR_SYS_CLOCK | 0x202 )
#define SR_SYS_CLOCK_NEW_CSR_FINE_S_RATE  ( SR_SYS_CLOCK | 0x203 )
#define SR_SYS_CLOCK_NEW_CSR_GROSS_S_RATE ( SR_SYS_CLOCK | 0x204 )

int clock_hsuspend(void *file)
{
    int i;
    char buf[SR_MAX_STRING_LENGTH];

    i = (episode_current == &episode_new);
    SR_WRITE_VALUE(file, SR_SYS_CLOCK_CURRENT_CSR, i, sizeof(i));
    SR_WRITE_VALUE(file, SR_SYS_CLOCK_UNIVERSAL_TOD, universal_tod.high, sizeof(universal_tod.high));
    MSGBUF(buf, "%f", hw_steering);
    SR_WRITE_STRING(file, SR_SYS_CLOCK_HW_STEERING, buf);
    SR_WRITE_VALUE(file, SR_SYS_CLOCK_HW_EPISODE, hw_episode, sizeof(hw_episode));
    SR_WRITE_VALUE(file, SR_SYS_CLOCK_HW_OFFSET, hw_offset, sizeof(hw_offset));

    SR_WRITE_VALUE(file, SR_SYS_CLOCK_OLD_CSR_START_TIME,   episode_old.start_time,   sizeof(episode_old.start_time));
    SR_WRITE_VALUE(file, SR_SYS_CLOCK_OLD_CSR_BASE_OFFSET,  episode_old.base_offset,  sizeof(episode_old.base_offset));
    SR_WRITE_VALUE(file, SR_SYS_CLOCK_OLD_CSR_FINE_S_RATE,  episode_old.fine_s_rate,  sizeof(episode_old.fine_s_rate));
    SR_WRITE_VALUE(file, SR_SYS_CLOCK_OLD_CSR_GROSS_S_RATE, episode_old.gross_s_rate, sizeof(episode_old.gross_s_rate));

    SR_WRITE_VALUE(file, SR_SYS_CLOCK_NEW_CSR_START_TIME,   episode_new.start_time,   sizeof(episode_new.start_time));
    SR_WRITE_VALUE(file, SR_SYS_CLOCK_NEW_CSR_BASE_OFFSET,  episode_new.base_offset,  sizeof(episode_new.base_offset));
    SR_WRITE_VALUE(file, SR_SYS_CLOCK_NEW_CSR_FINE_S_RATE,  episode_new.fine_s_rate,  sizeof(episode_new.fine_s_rate));
    SR_WRITE_VALUE(file, SR_SYS_CLOCK_NEW_CSR_GROSS_S_RATE, episode_new.gross_s_rate, sizeof(episode_new.gross_s_rate));

    return 0;
}

int clock_hresume(void *file)
{
    size_t key, len;
    int i;
    float f;
    char buf[SR_MAX_STRING_LENGTH];

    memset(&episode_old, 0, sizeof(CSR));
    memset(&episode_new, 0, sizeof(CSR));
    episode_current = &episode_new;
    universal_tod.high = universal_tod.low = 0;
    hw_steering = 0.0;
    hw_episode = 0;
    hw_offset = 0;

    do {
        SR_READ_HDR(file, key, len);
        switch (key) {
        case SR_SYS_CLOCK_CURRENT_CSR:
            SR_READ_VALUE(file, len, &i, sizeof(i));
            episode_current = i ? &episode_new : &episode_old;
            break;
        case SR_SYS_CLOCK_UNIVERSAL_TOD:
            SR_READ_VALUE(file, len, &universal_tod.high, sizeof(universal_tod.high));
            break;
        case SR_SYS_CLOCK_HW_STEERING:
            SR_READ_STRING(file, buf, len);
            sscanf(buf, "%f",&f);
            hw_steering = f;
            break;
        case SR_SYS_CLOCK_HW_EPISODE:
            SR_READ_VALUE(file, len, &hw_episode, sizeof(hw_episode));
            break;
        case SR_SYS_CLOCK_HW_OFFSET:
            SR_READ_VALUE(file, len, &hw_offset, sizeof(hw_offset));
            break;
        case SR_SYS_CLOCK_OLD_CSR_START_TIME:
            SR_READ_VALUE(file, len, &episode_old.start_time, sizeof(episode_old.start_time));
            break;
        case SR_SYS_CLOCK_OLD_CSR_BASE_OFFSET:
            SR_READ_VALUE(file, len, &episode_old.base_offset, sizeof(episode_old.base_offset));
            break;
        case SR_SYS_CLOCK_OLD_CSR_FINE_S_RATE:
            SR_READ_VALUE(file, len, &episode_old.fine_s_rate, sizeof(episode_old.fine_s_rate));
            break;
        case SR_SYS_CLOCK_OLD_CSR_GROSS_S_RATE:
            SR_READ_VALUE(file, len, &episode_old.gross_s_rate, sizeof(episode_old.gross_s_rate));
            break;
        case SR_SYS_CLOCK_NEW_CSR_START_TIME:
            SR_READ_VALUE(file, len, &episode_new.start_time, sizeof(episode_new.start_time));
            break;
        case SR_SYS_CLOCK_NEW_CSR_BASE_OFFSET:
            SR_READ_VALUE(file, len, &episode_new.base_offset, sizeof(episode_new.base_offset));
            break;
        case SR_SYS_CLOCK_NEW_CSR_FINE_S_RATE:
            SR_READ_VALUE(file, len, &episode_new.fine_s_rate, sizeof(episode_new.fine_s_rate));
            break;
        case SR_SYS_CLOCK_NEW_CSR_GROSS_S_RATE:
            SR_READ_VALUE(file, len, &episode_new.gross_s_rate, sizeof(episode_new.gross_s_rate));
            break;
        default:
            SR_READ_SKIP(file, len);
            break;
        }
    } while ((key & SR_SYS_MASK) == SR_SYS_CLOCK);
    return 0;
}


#endif


#if defined(FEATURE_INTERVAL_TIMER)
static INLINE void ARCH_DEP(_store_int_timer_2) (REGS *regs,int getlock)
{
S32 itimer;
S32 vtimer=0;

    if(getlock)
    {
        OBTAIN_INTLOCK(regs->hostregs?regs:NULL);
    }
    itimer=int_timer(regs);
    STORE_FW(regs->psa->inttimer, itimer);
#if defined(FEATURE_ECPSVM)
    if(regs->ecps_vtmrpt)
    {
        vtimer=ecps_vtimer(regs);
        // MODRWP
        // STORE_FW(regs->ecps_vtmrpt, itimer);
        STORE_FW(regs->ecps_vtmrpt, vtimer);
    }
#endif /*defined(FEATURE_ECPSVM)*/

    chk_int_timer(regs);
#if defined(FEATURE_ECPSVM)
    if(regs->ecps_vtmrpt)
    {
        regs->ecps_oldtmr = vtimer;
    }
#endif /*defined(FEATURE_ECPSVM)*/

    if(getlock)
    {
        RELEASE_INTLOCK(regs->hostregs?regs:NULL);
    }
}


DLL_EXPORT
void ARCH_DEP(store_int_timer) (REGS *regs)
{
    ARCH_DEP(_store_int_timer_2) (regs,1);
}


void ARCH_DEP(store_int_timer_nolock) (REGS *regs)
{
    ARCH_DEP(_store_int_timer_2) (regs,0);
}


DLL_EXPORT
void ARCH_DEP(fetch_int_timer) (REGS *regs)
{
S32 itimer;
    FETCH_FW(itimer, regs->psa->inttimer);
    OBTAIN_INTLOCK(regs->hostregs?regs:NULL);
    set_int_timer(regs, itimer);
#if defined(FEATURE_ECPSVM)
    if(regs->ecps_vtmrpt)
    {
        FETCH_FW(itimer, regs->ecps_vtmrpt);
        set_ecps_vtimer(regs, itimer);
    }
#endif /*defined(FEATURE_ECPSVM)*/
    RELEASE_INTLOCK(regs->hostregs?regs:NULL);
}
#endif


#if defined(FEATURE_TOD_CLOCK_STEERING_FACILITY)

void ARCH_DEP(set_gross_s_rate) (REGS *regs)
{
S32 gsr;
    gsr = ARCH_DEP(vfetch4) (regs->GR(1) & ADDRESS_MAXWRAP(regs), 1, regs);

    set_gross_steering_rate(gsr);
}


void ARCH_DEP(set_fine_s_rate) (REGS *regs)
{
S32 fsr;
    fsr = ARCH_DEP(vfetch4) (regs->GR(1) & ADDRESS_MAXWRAP(regs), 1, regs);

    set_fine_steering_rate(fsr);
}


void ARCH_DEP(set_tod_offset) (REGS *regs)
{
S64 offset;
    offset = ARCH_DEP(vfetch8) (regs->GR(1) & ADDRESS_MAXWRAP(regs), 1, regs);

    set_tod_offset(offset >> 8);
}


void ARCH_DEP(adjust_tod_offset) (REGS *regs)
{
S64 offset;
    offset = ARCH_DEP(vfetch8) (regs->GR(1) & ADDRESS_MAXWRAP(regs), 1, regs);

    adjust_tod_offset(offset >> 8);
}


void ARCH_DEP(query_physical_clock) (REGS *regs)
{
    ARCH_DEP(vstore8) (universal_clock() << 8, regs->GR(1) & ADDRESS_MAXWRAP(regs), 1, regs);
}


void ARCH_DEP(query_steering_information) (REGS *regs)
{
PTFFQSI qsi;
    obtain_lock(&sysblk.todlock);
    STORE_DW(qsi.physclk, universal_clock() << 8);
    STORE_DW(qsi.oldestart, episode_old.start_time << 8);
    STORE_DW(qsi.oldebase, episode_old.base_offset << 8);
    STORE_FW(qsi.oldfsr, episode_old.fine_s_rate );
    STORE_FW(qsi.oldgsr, episode_old.gross_s_rate );
    STORE_DW(qsi.newestart, episode_new.start_time << 8);
    STORE_DW(qsi.newebase, episode_new.base_offset << 8);
    STORE_FW(qsi.newfsr, episode_new.fine_s_rate );
    STORE_FW(qsi.newgsr, episode_new.gross_s_rate );
    release_lock(&sysblk.todlock);

    ARCH_DEP(vstorec) (&qsi, sizeof(qsi)-1, regs->GR(1) & ADDRESS_MAXWRAP(regs), 1, regs);
}


void ARCH_DEP(query_tod_offset) (REGS *regs)
{
PTFFQTO qto;
    obtain_lock(&sysblk.todlock);
    STORE_DW(qto.todoff, (hw_clock_l() - universal_tod.high) << 8);
    STORE_DW(qto.physclk, (universal_tod.high << 8) | (universal_tod.low >> 56));
    STORE_DW(qto.ltodoff, episode_current->base_offset << 8);
    STORE_DW(qto.todepoch, regs->tod_epoch << 8);
    release_lock(&sysblk.todlock);

    ARCH_DEP(vstorec) (&qto, sizeof(qto)-1, regs->GR(1) & ADDRESS_MAXWRAP(regs), 1, regs);
}


void ARCH_DEP(query_available_functions) (REGS *regs)
{
PTFFQAF qaf;
    STORE_FW(qaf.sb[0] , 0xF0000000);        /* Functions 0x00..0x1F */
    STORE_FW(qaf.sb[1] , 0x00000000);        /* Functions 0x20..0x3F */
    STORE_FW(qaf.sb[2] , 0xF0000000);        /* Functions 0x40..0x5F */
    STORE_FW(qaf.sb[3] , 0x00000000);        /* Functions 0x60..0x7F */

    ARCH_DEP(vstorec) (&qaf, sizeof(qaf)-1, regs->GR(1) & ADDRESS_MAXWRAP(regs), 1, regs);
}
#endif /*defined(FEATURE_TOD_CLOCK_STEERING_FACILITY)*/

#if !defined(_GEN_ARCH)

#if defined(_ARCHMODE2)
 #define  _GEN_ARCH _ARCHMODE2
 #include "clock.c"
#endif

#if defined(_ARCHMODE3)
 #undef   _GEN_ARCH
 #define  _GEN_ARCH _ARCHMODE3
 #include "clock.c"
#endif


#endif /*!defined(_GEN_ARCH)*/
