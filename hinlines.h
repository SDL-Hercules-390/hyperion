/* HINLINES.H   (C) Copyright Roger Bowler, 1999-2012                */
/*              Hercules-wide inline functions                       */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#ifndef _HINLINES_H
#define _HINLINES_H

/* Define inline assembly style for GNU C compatible compilers */
#if defined(__GNUC__)
    #define asm  __asm__
#endif

/*-------------------------------------------------------------------*/

#if !defined(round_to_hostpagesize)
static inline U64 round_to_hostpagesize(U64 n)
{
    U64 factor = hostinfo.hostpagesz - 1;
    return ((n + factor) & ~factor);
}
#endif

/*-------------------------------------------------------------------*/

#if !defined(clear_io_buffer)

#define clear_storage(_addr, _n)                                       \
      __clear_io_buffer((void *)(_addr), (size_t)(_n))
#define clear_io_buffer(_addr,_n)                                      \
      __clear_io_buffer((void *)(_addr),(size_t)(_n))
#define clear_page(_addr)                                              \
      __clear_page((void *)(_addr), (size_t)( FOUR_KILOBYTE / 64 ) )
#define clear_page_1M(_addr)                                           \
      __clear_page((void *)(_addr), (size_t)( ONE_MEGABYTE  / 64 ) )
#define clear_page_4K(_addr)                                           \
      __clear_page((void *)(_addr), (size_t)( FOUR_KILOBYTE / 64 ) )
#define clear_page_2K(_addr)                                           \
      __clear_page((void *)(_addr), (size_t)( TWO_KILOBYTE  / 64 ) )

/*-------------------------------------------------------------------*/

#if defined(__GNUC__) && defined(__SSE2__) && (__SSE2__ == 1)
  #define _GCC_SSE2_
#elif defined (_MSVC_)
  #if defined( _M_X64 )
    /* "On the x64 platform, __faststorefence generates
        an instruction that is a faster store fence
        than the sfence instruction. Use this intrinsic
        instead of _mm_sfence on the x64 platform."
    */
    #define  SFENCE  __faststorefence
  #else
    #define  SFENCE  _mm_sfence
  #endif
#endif

/*-------------------------------------------------------------------*/

#if defined(_GCC_SSE2_)

static inline void __clear_page( void *addr, size_t pgszmod64 )
{
    unsigned char xmm_save[16];
    unsigned int i;

    asm volatile("": : :"memory");      /* barrier */

    asm volatile("\n\t"
        "movups %%xmm0, (%0)\n\t"
        "xorps  %%xmm0, %%xmm0"
        : : "r" (xmm_save));

    for (i = 0; i < pgszmod64; i++)
        asm volatile("\n\t"
            "movntps %%xmm0,   (%0)\n\t"
            "movntps %%xmm0, 16(%0)\n\t"
            "movntps %%xmm0, 32(%0)\n\t"
            "movntps %%xmm0, 48(%0)"
            : : "r"(addr) : "memory");

    asm volatile("\n\t"
        "sfence\n\t"
        "movups (%0), %%xmm0"
        : : "r" (xmm_save) : "memory");

    return;
}

/*-------------------------------------------------------------------*/

#elif defined (_MSVC_)

static inline void __clear_page( void* addr, size_t pgszmod64 )
{
    // Variables of type __m128 map to one of the XMM[0-7] registers
    // and are used with SSE and SSE2 instructions intrinsics defined
    // in the <xmmintrin.h> header. They are automatically aligned
    // on 16-byte boundaries. You should not access the __m128 fields
    // directly. You can, however, see these types in the debugger.

    unsigned int i;                 /* (work var for loop) */
    __m128 xmm0;                    /* (work XMM register) */

    /* Init work reg to 0 */
    xmm0 = _mm_setzero_ps();        // (suppresses C4700; will be optimized out)
    _mm_xor_ps( xmm0, xmm0 );

    /* Clear requested page without polluting our cache */
    for (i=0; i < pgszmod64; i++, (float*) addr += 16 )
    {
        _mm_stream_ps( (float*) addr+ 0, xmm0 );
        _mm_stream_ps( (float*) addr+ 4, xmm0 );
        _mm_stream_ps( (float*) addr+ 8, xmm0 );
        _mm_stream_ps( (float*) addr+12, xmm0 );
    }

    /* An SFENCE guarantees that every preceding store
       is globally visible before any subsequent store. */
    SFENCE();

    return;
}
#else /* (all others) */
  #define  __clear_page(_addr, _pgszmod64 )    memset((void*)(_addr), 0, ((size_t)(_pgszmod64)) << 6)
#endif

/*-------------------------------------------------------------------*/

#if defined(_GCC_SSE2_)
static inline void __optimize_clear(void *addr, size_t n)
{
    char *mem = addr;

    /* Let the compiler perform special case optimization */
    while (n-- > 0)
        *mem++ = 0;

    return;
}
#else /* (all others, including _MSVC_) */
  #define __optimize_clear(p,n)     memset((void*)(p),0,(size_t)(n))
#endif

/*-------------------------------------------------------------------*/

#if defined(_GCC_SSE2_) || defined (_MSVC_)
static inline void __clear_io_buffer(void *addr, size_t n)
{
    unsigned int x;
    void *limit;

    /* Let the C compiler perform special case optimization */
    if ((x = (U64)(uintptr_t)addr & 0x00000FFF))
    {
        unsigned int a = 4096 - x;

        __optimize_clear( addr, a );
        if (!(n -= a))
        {
            return;
        }
    }

    /* Calculate page clear size */
    if ((x = n & ~0x00000FFF))
    {
        /* Set loop limit */
        limit = (BYTE*)addr + x;
        n -= x;

        /* Loop through pages */
        do
        {
            __clear_page( addr, (size_t)( FOUR_KILOBYTE / 64 ) );
            addr = (BYTE*)addr + 4096;
        }
        while (addr < limit);
    }

    /* Clean up any remainder */
    if (n)
    {
        __optimize_clear( addr, n );
    }

    return;

}

/*-------------------------------------------------------------------*/

#else /* (all others) */
  #define  __clear_io_buffer(_addr, _n)  memset((void*)(_addr), 0, (size_t)(_n))
#endif

#endif /* !defined(clear_io_buffer) */


/*-------------------------------------------------------------------*/
/* Convert an SCSW to a CSW for S/360 and S/370 channel support      */
/*-------------------------------------------------------------------*/
static inline void scsw2csw( const SCSW* scsw, BYTE* csw )
{
    memcpy( csw, scsw->ccwaddr, 8 );
    csw[0] = scsw->flag0;
}


/*-------------------------------------------------------------------*/
/* Store an SCSW as a CSW for S/360 and S/370 channel support        */
/*-------------------------------------------------------------------*/
static inline void store_scsw_as_csw( const REGS* regs, const SCSW* scsw )
{
    PSA_3XX*   psa;            /* -> Prefixed storage area  */
    RADR       pfx;            /* Current prefix            */

    /* Establish prefixing */
    pfx =
#if defined(_FEATURE_SIE)
          SIE_MODE(regs) ? regs->sie_px :
#endif
          regs->PX;

    /* Establish current PSA with prefixing applied */
    psa = (PSA_3XX*)(regs->mainstor + pfx);

    /* Store the channel status word at PSA+X'40' (64)*/
    scsw2csw( scsw, psa->csw );

    /* Update storage key for reference and change done by caller */
}

/*-------------------------------------------------------------------*/
/* Synchronize CPUS                                                  */
/*-------------------------------------------------------------------*/
/*                                                                   */
/* Locks                                                             */
/*      INTLOCK(regs)                                                */
/*-------------------------------------------------------------------*/
#define SYNCHRONIZE_CPUS( _regs )   synchronize_cpus( _regs, PTT_LOC )
static inline void synchronize_cpus( REGS* regs, const char* location )
{
    int i, n = 0;
    REGS*  i_regs;

    CPU_BITMAP mask = sysblk.started_mask;

    /* Deselect current processor and waiting processors from mask */
    mask &= ~(sysblk.waiting_mask | HOSTREGS->cpubit);

    /* Deselect processors at a syncpoint and count active processors
     */
    for (i=0; mask && i < sysblk.hicpu; ++i)
    {
        i_regs = sysblk.regs[i];

        if (mask & CPU_BIT( i ))
        {
            if (AT_SYNCPOINT( i_regs ))
            {
                /* Remove CPU already at syncpoint */
                mask ^= CPU_BIT(i);
            }
            else
            {
                /* Update count of active processors */
                ++n;

                /* Test and set interrupt pending conditions */
                ON_IC_INTERRUPT( i_regs );

                if (SIE_MODE( i_regs ))
                    ON_IC_INTERRUPT( GUEST( i_regs ));
            }
        }
    }

    /* If any interrupts are pending with active processors, other than
     * self, open an interrupt window for those processors prior to
     * considering self as synchronized.
     */
    if (n && mask)
    {
        sysblk.sync_mask = mask;
        sysblk.syncing   = true;
        sysblk.intowner  = LOCK_OWNER_NONE;

        hthread_wait_condition( &sysblk.sync_cond, &sysblk.intlock, location );

        sysblk.intowner  = HOSTREGS->cpuad;
        sysblk.syncing   = false;

        hthread_broadcast_condition( &sysblk.sync_bc_cond, location );
    }
    /* All active processors other than self, are now waiting at their
     * respective sync point. We may now safely proceed doing whatever
     * it is we need to do.
     */
}

/*-------------------------------------------------------------------*/

#define WAKEUP_CPU(r)          wakeup_cpu( r, PTT_LOC )
#define WAKEUP_CPU_MASK(m)     wakeup_cpu_mask( m, PTT_LOC )
#define WAKEUP_CPUS_MASK(m)    wakeup_cpus_mask( m, PTT_LOC )

static inline void wakeup_cpu( REGS* regs, const char* location )
{
    hthread_signal_condition( &regs->intcond, location );
}

/*-------------------------------------------------------------------*/

static inline void wakeup_cpu_mask( CPU_BITMAP mask, const char* location )
{
    REGS*  current_regs;
    REGS*  lru_regs = NULL;
    TOD    current_waittod;
    TOD    lru_waittod;
    int    i;

    if (mask)
    {
        for (i=0; mask; mask >>= 1, ++i)
        {
            if (mask & 1)
            {
                current_regs = sysblk.regs[i];
                current_waittod = current_regs->waittod;

                /* Select least recently used CPU
                 *
                 * The LRU CPU is chosen to keep the CPU threads active
                 * and to distribute the I/O load across the available
                 * CPUs.
                 *
                 * The current_waittod should never be zero; however,
                 * we check it in case the cache from another processor
                 * has not yet been written back to memory, which can
                 * happen once the lock structure is updated for
                 * individual CPU locks. (OBTAIN/RELEASE_INTLOCK(regs)
                 * at present locks ALL CPUs, despite the specification
                 * of regs.)
                 */
                if (lru_regs == NULL ||
                    (current_waittod > 0 &&
                     (current_waittod < lru_waittod ||
                      (current_waittod == lru_waittod &&
                       current_regs->waittime >= lru_regs->waittime))))
                {
                    lru_regs = current_regs;
                    lru_waittod = current_waittod;
                }
            }
        }

        /* Wake up the least recently used CPU */
        wakeup_cpu( lru_regs, location );
    }
}

/*-------------------------------------------------------------------*/

static inline void wakeup_cpus_mask( CPU_BITMAP mask, const char* location )
{
    int i;

    for (i=0; mask; mask >>= 1, i++)
    {
        if (mask & 1)
            wakeup_cpu( sysblk.regs[i], location );
    }
}

/*-------------------------------------------------------------------*/
/*  Obtain/Release master interrupt lock. The master interrupt lock  */
/*  can be obtained by any thread. If obtained by a CPU thread, we   */
/*  check to see if synchronize_cpus is in progress.                 */
/*-------------------------------------------------------------------*/

#define OBTAIN_INTLOCK(r)       Obtain_Interrupt_Lock( r, PTT_LOC )
#define RELEASE_INTLOCK(r)      Release_Interrupt_Lock( r, PTT_LOC )
#define TRY_OBTAIN_INTLOCK(r)   Try_Obtain_Interrupt_Lock( r, PTT_LOC )

static inline void Interrupt_Lock_Obtained( REGS* regs, const char* location )
{
    if (regs)
    {
        while (sysblk.syncing)
        {
            sysblk.sync_mask &= ~HOSTREGS->cpubit;

            if (!sysblk.sync_mask)
                hthread_signal_condition( &sysblk.sync_cond, location );

            hthread_wait_condition( &sysblk.sync_bc_cond, &sysblk.intlock, location );
        }

        HOSTREGS->intwait = false;
        sysblk.intowner = HOSTREGS->cpuad;
    }
    else
        sysblk.intowner = LOCK_OWNER_OTHER;
}

/*-------------------------------------------------------------------*/

static inline void Obtain_Interrupt_Lock( REGS* regs, const char* location )
{
    if (regs)
        HOSTREGS->intwait = true;
    hthread_obtain_lock( &sysblk.intlock, location );
    Interrupt_Lock_Obtained( regs, location );
}

/*-------------------------------------------------------------------*/

static inline int Try_Obtain_Interrupt_Lock( REGS* regs, const char* location )
{
    int rc;
    if (regs)
        HOSTREGS->intwait = true;
    if ((rc = hthread_try_obtain_lock( &sysblk.intlock, location )) == 0)
        Interrupt_Lock_Obtained( regs, location );
    else if (regs)
        HOSTREGS->intwait = false;
    return rc;
}

/*-------------------------------------------------------------------*/

static inline void Release_Interrupt_Lock( REGS* regs, const char* location )
{
    UNREFERENCED( regs );
    sysblk.intowner = LOCK_OWNER_NONE;
    hthread_release_lock( &sysblk.intlock, location );
}

/*-------------------------------------------------------------------*/
/*           Atomically update 32-bit/64-bit value                   */
/*-------------------------------------------------------------------*/
static inline void atomic_update32( volatile S32* p, S32 count )
{
#if defined( _MSVC_ )
    InterlockedExchangeAdd( p, count );
#else // GCC (and CLANG?)
  #if defined( HAVE_SYNC_BUILTINS )
    __sync_fetch_and_add( p, count );
  #else
    *p += count;  /* (N.B. non-atomic!) */
  #endif
#endif
}
static inline void atomic_update64( volatile S64* p, S64 count )
{
#if defined( _MSVC_ )
    InterlockedExchangeAdd64( p, count );
#else // GCC (and CLANG?)
  #if defined( HAVE_SYNC_BUILTINS )
    __sync_fetch_and_add( p, count );
  #else
    *p += count;  /* (N.B. non-atomic!) */
  #endif
#endif
}

/*-------------------------------------------------------------------*/
/*           Atomically update SYSBLK Instruction Counter            */
/*-------------------------------------------------------------------*/

#define UPDATE_SYSBLK_INSTCOUNT( _count ) \
        atomic_update64( &sysblk.instcount, (_count) )

/*-------------------------------------------------------------------*/
/*  Atomically update SYSBLK count of CPUs executing a transaction   */
/*-------------------------------------------------------------------*/

#if defined( _FEATURE_073_TRANSACT_EXEC_FACILITY )
#define UPDATE_SYSBLK_TRANSCPUS( _count )                           \
  do                                                                \
  {                                                                 \
    atomic_update32( &sysblk.txf_transcpus, (_count) );             \
    if (sysblk.txf_transcpus < 0)                                   \
      CRASH();                                                      \
  }                                                                 \
  while (0)
#endif

/*-------------------------------------------------------------------*/
/* Stop ALL CPUs                                      (INTLOCK held) */
/*-------------------------------------------------------------------*/
static inline void stop_all_cpus_intlock_held()
{
    CPU_BITMAP  mask;
    REGS*       regs;
    int         i;

    for (i=0, mask = sysblk.started_mask; mask; i++, mask >>= 1)
    {
        if (mask & 1)
        {
            regs = sysblk.regs[i];

            regs->opinterv = 1;
            regs->cpustate = CPUSTATE_STOPPING;

            ON_IC_INTERRUPT( regs );

            signal_condition( &regs->intcond );
        }
    }
}

/*-------------------------------------------------------------------*/
/* Test if any CPUs are in started state              (INTLOCK held) */
/*-------------------------------------------------------------------*/
static inline bool are_any_cpus_started_intlock_held()
{
    int cpu;

    if (sysblk.cpus)
        for (cpu = 0; cpu < sysblk.hicpu; cpu++)
            if (IS_CPU_ONLINE( cpu ))
                if (sysblk.regs[ cpu ]->cpustate == CPUSTATE_STARTED)
                    return true;
    return false;
}

/*-------------------------------------------------------------------*/
/* Test if all CPUs are in stopped state              (INTLOCK held) */
/*-------------------------------------------------------------------*/
static inline bool are_all_cpus_stopped_intlock_held()
{
    int cpu;

    if (sysblk.cpus)
        for (cpu = 0; cpu < sysblk.hicpu; cpu++)
            if (IS_CPU_ONLINE( cpu ))
                if (sysblk.regs[ cpu ]->cpustate != CPUSTATE_STOPPED)
                    return false;
    return true;
}

/*-------------------------------------------------------------------*/
/* Test if any CPUs are in started state          (INTLOCK not held) */
/*-------------------------------------------------------------------*/
static inline bool are_any_cpus_started()
{
    bool any_started;

    OBTAIN_INTLOCK( NULL );
    {
        any_started = are_any_cpus_started_intlock_held();
    }
    RELEASE_INTLOCK( NULL );
    return any_started;
}

/*-------------------------------------------------------------------*/
/* Test if all CPUs are in stopped state          (INTLOCK not held) */
/*-------------------------------------------------------------------*/
static inline bool are_all_cpus_stopped()
{
    bool all_stopped;

    OBTAIN_INTLOCK( NULL );
    {
        all_stopped = are_all_cpus_stopped_intlock_held();
    }
    RELEASE_INTLOCK( NULL );
    return all_stopped;
}

/*-------------------------------------------------------------------*/
/* Stop ALL CPUs                                  (INTLOCK not held) */
/*-------------------------------------------------------------------*/
static inline void stop_all_cpus()
{
    OBTAIN_INTLOCK( NULL );
    {
        stop_all_cpus_intlock_held();
    }
    RELEASE_INTLOCK( NULL );
}

/*-------------------------------------------------------------------*/
#undef asm

#endif // _HINLINES_H
