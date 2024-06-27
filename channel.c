/* CHANNEL.C    (C) Copyright Roger Bowler,    1999-2012             */
/*              (C) Copyright Jan Jaeger,      1999-2012             */
/*              (C) Copyright Mark L. Gaubatz, 2010, 2013            */
/*              (C) and others 2013-2023                             */
/*              ESA/390 Channel Emulator                             */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/* z/Architecture support -                                          */
/*      (C) Copyright Jan Jaeger, 1999-2012                          */
/* Rewrite of channel code and associated structures -               */
/*      (C) Copyright Mark L. Gaubatz, 2010, 2013                    */

/*-------------------------------------------------------------------*/
/* This module contains the channel subsystem functions for the      */
/* Hercules S/370 and ESA/390 emulator.                              */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* Additional credits:                                               */
/*      Measurement block support by Jan Jaeger                      */
/*      Fix program check on NOP due to addressing - Jan Jaeger      */
/*      Fix program check on TIC as first ccw on RSCH - Jan Jaeger   */
/*      Fix PCI intermediate status flags             - Jan Jaeger   */
/*      64-bit IDAW support - Roger Bowler v209                      */
/*      Incorrect-length-indication-suppression - Jan Jaeger         */
/*      Read backward support contributed by Hackules   13jun2002    */
/*      Read backward fixes contributed by Jay Jaeger   16sep2003    */
/*      MIDAW support - Roger Bowler                    03aug2005    */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

DISABLE_GCC_UNUSED_FUNCTION_WARNING;

#define _CHANNEL_C_
#define _HENGINE_DLL_

#include "hercules.h"
#include "devtype.h"
#include "opcode.h"
#include "chsc.h"
#include "inline.h"

#ifdef FEATURE_S370_CHANNEL
#include "commadpt.h"
#endif /*FEATURE_S370_CHANNEL*/

DISABLE_GCC_UNUSED_SET_WARNING;

/*-------------------------------------------------------------------*/
/* Debug Switches                                                    */
/*-------------------------------------------------------------------*/
#ifndef CHANNEL_DEBUG_SWITCHES
#define CHANNEL_DEBUG_SWITCHES
#define DEBUG_SCSW              0
#define DEBUG_PREFETCH          0
#define DEBUG_DUMP              0
#endif

/*-------------------------------------------------------------------*/
/* CCW Tracing helper macros                                         */
/*-------------------------------------------------------------------*/

#ifndef CCW_TRACING_ACTIVE
#define CCW_TRACING_ACTIVE( dev, tracethis )    ((dev)->ccwtrace \
                                                 || ( tracethis ))
#endif

#ifndef SKIP_CH9UC
#define SKIP_CH9UC( dev, chanstat, unitstat )                       \
                                                                    \
    (1                                                              \
     && !(dev)->ccwtrace                                            \
     && !CPU_STEPPING_OR_TRACING_ALL                                \
     && sysblk.noch9oflow                                           \
     && is_ch9oflow( dev, chanstat, unitstat )                      \
    )
#endif

/*-------------------------------------------------------------------*/
/* helper function to check if printer channel-9 overflow unit check */
/*-------------------------------------------------------------------*/
#ifndef IS_CH9OFLOW
#define IS_CH9OFLOW
static BYTE is_ch9oflow( DEVBLK* dev, BYTE chanstat, BYTE unitstat )
{
    static const BYTE unitck = (CSW_CE | CSW_DE | CSW_UC);

    if (1
        &&    0      == chanstat
        &&  unitck   == unitstat
        && SENSE_CH9 == dev->sense[0]
        &&    0      == dev->sense[1]
        && dev->hnd->query
    )
    {
        char* devclass = NULL;
        dev->hnd->query( NULL, &devclass, 0, NULL );
        if (strcmp( devclass, "PRT" ) == 0)
            return TRUE;
    }
    return FALSE;
}
#endif // IS_CH9OFLOW

/*-------------------------------------------------------------------*/
/* Internal function definitions                                     */
/*-------------------------------------------------------------------*/
void                call_execute_ccw_chain (int arch_mode, void* pDevBlk);
DLL_EXPORT  void*   device_thread (void *arg);
static int          schedule_ioq (const REGS* regs, DEVBLK* dev);
static INLINE void  subchannel_interrupt_queue_cleanup (DEVBLK*);
int                 test_subchan_locked (REGS*, DEVBLK*, IRB*, IOINT**, SCSW**);

#ifndef CHANNEL_INLINES
#define CHANNEL_INLINES

static INLINE U8 IS_CCW_IMMEDIATE( const DEVBLK* dev, const BYTE code )
{
    return
    (0
        || (dev->hnd->immed && dev->hnd->immed[code])
        || (dev->immed      && dev->immed[code])
        || IS_CCW_NOP( dev->code )
    );
}

static INLINE void
set_subchannel_busy(DEVBLK* dev)
{
    dev->busy = 1;
    dev->scsw.flag3 |= SCSW3_AC_SCHAC | SCSW3_SC_INTER;
}

static INLINE void
set_device_busy(DEVBLK* dev)
{
    set_subchannel_busy(dev);
    dev->scsw.flag3 |= SCSW3_AC_DEVAC;
}

static INLINE void
clear_subchannel_busy_scsw(SCSW* scsw)
{
    scsw->flag3 &= ~(SCSW3_AC_SCHAC |
                     SCSW3_AC_DEVAC |
                     SCSW3_SC_INTER);
}

static INLINE void
clear_subchannel_busy(DEVBLK* dev)
{
    clear_subchannel_busy_scsw(&dev->scsw);
    dev->startpending = 0;
    dev->busy = 0;
}

static INLINE void
clear_device_busy_scsw(SCSW* scsw)
{
    scsw->flag3 &= ~SCSW3_AC_DEVAC;
}

static INLINE void
clear_device_busy(DEVBLK* dev)
{
    clear_device_busy_scsw(&dev->scsw);
}

#endif /*CHANNEL_INLINES*/


/*-------------------------------------------------------------------*/
/* Internal I/O Buffer Structure and Inline Support Routines         */
/*-------------------------------------------------------------------*/
#ifndef IOBUF_STRUCT
#define IOBUF_STRUCT

CASSERT( IOBUF_HDRSIZE  == ROUND_UP( IOBUF_HDRSIZE,  IOBUF_ALIGN ), IOBUF_HDRSIZE  );
CASSERT( IOBUF_MINSIZE  == ROUND_UP( IOBUF_MINSIZE,  IOBUF_ALIGN ), IOBUF_MINSIZE  );
CASSERT( IOBUF_INCREASE == ROUND_UP( IOBUF_INCREASE, IOBUF_ALIGN ), IOBUF_INCREASE );

struct IOBUF
{
    /* Every IOBUF struct starts with a 4K header the first few
     * bytes of which hold the IOBUF control variables defining
     * the size of the actual usable channel I/O buffer portion
     * of the IOBUF structure, a pointer to the actual channel
     * I/O buffer itself, and a second pointer pointing to the
     * last logical byte of the channel I/O buffer. The actual
     * channel I/O buffer immediately follows the IOBUF header.
     *
     * Every IOBUF is always aligned on a 4K boundary thus en-
     * suring the actual channel I/O buffer itself is as well.
     *
     * The size of every IOBUF will always be a multiple of 4K.
     *
     * When an IOBUF is reallocated larger, if the old size is
     * larger than IOBUF_MINSIZE bytes, it means the IOBUF was
     * malloc'ed on the heap and thus the old IOBUF should then
     * be freed. Otherwise if the old size is less or equal to
     * IOBUF_MINSIZE, it means the IOBUF was allocated on the
     * stack and thus free should not be done.
     */
    union
    {
        struct
        {
            u_int   size;               /* Channel I/O buffer size   */
            BYTE    *start;             /* Start of data address     */
            BYTE    *end;               /* Last byte address         */

            /* Note:  Remaining space from 4K page is reserved
             *        for debugging and future use. No mapping
             *        of the space is presently done.
             */
        };
        BYTE    pad[IOBUF_HDRSIZE];     /* Pad to alignment boundary */
    };
    BYTE    data[IOBUF_MINSIZE];        /* Channel I/O buffer itself */
};
typedef struct IOBUF IOBUF;

static INLINE void
iobuf_initialize (IOBUF *iobuf, const u_int size)
{
    iobuf->size  = size;
    iobuf->start = (BYTE *)iobuf->data;
    iobuf->end   = (BYTE *)iobuf->start + (size - 1);
}

static INLINE IOBUF *
iobuf_create (u_int size)
{
    IOBUF *iobuf;
    size = ROUND_UP( size, IOBUF_INCREASE );
    iobuf = (IOBUF*)malloc_aligned(offsetof(IOBUF,data) + size, IOBUF_ALIGN);
    if (iobuf != NULL)
        iobuf_initialize(iobuf, size);
    return iobuf;
}

static INLINE u_int
iobuf_validate (IOBUF *iobuf)
{
    return
    ((iobuf == NULL                         ||
      iobuf->size < IOBUF_MINSIZE           ||
      iobuf->start != (BYTE *)iobuf->data   ||
      iobuf->end   != (BYTE *)iobuf->start + (iobuf->size - 1 )) ?
     0 : 1);
}

static INLINE void
iobuf_destroy (IOBUF *iobuf)
{
    /* PROGRAMMING NOTE: The tests below are purposely neither macros
     * or inlined routines. This is to permit breakpoints during
     * development and testing. In the future, instead of aborting
     * and/or crashing, the code will branch (or longjmp) to a point in
     * channel code where the current CCW chain will be terminated,
     * cleanup performed, and a Channel Check, and/or a Machine Check,
     * will be returned.
     */
    if (iobuf == NULL)
        return;
    if (!iobuf_validate(iobuf))
        CRASH();
    if (iobuf->size > IOBUF_MINSIZE)
        free_aligned(iobuf);
}

static INLINE IOBUF*
iobuf_reallocate (IOBUF *iobuf, const u_int size)
{
    IOBUF *iobufnew;
    /* PROGRAMMING NOTE: The tests below are purposely neither macros
     * or inlined routines. This is to permit breakpoints during
     * development and testing. In the future, instead of aborting
     * and/or crashing, the code will branch (or longjmp) to a point in
     * channel code where the current CCW chain will be terminated,
     * cleanup performed, and a Channel Check, and/or a Machine Check,
     * will be returned.
     */
    if (iobuf == NULL)
        return NULL;
    if (!iobuf_validate(iobuf))
        CRASH();
    iobufnew = iobuf_create(size);
    if (iobufnew == NULL)
        return NULL;
    memcpy(iobufnew->start, iobuf->start, MIN(iobuf->size, size));
    if (iobuf->size > IOBUF_MINSIZE)
        free_aligned(iobuf);
    return iobufnew;
}

#endif /* IOBUF_STRUCT */

/*--------------------------------------------------------------------*/
/* CCW prefetch data structure                                        */
/*--------------------------------------------------------------------*/
#ifndef PREFETCH_STRUCT
#define PREFETCH_STRUCT

/* Define 256 entries in the prefetch table */
#define PF_SIZE 256

struct PREFETCH                         /* Prefetch data structure   */
{
    u_int   seq;                        /* Counter                   */
    u_int   pos;                        /* Highest valid position    */
    U32     reqcount;                   /* Requested count           */
    U8      prevcode;                   /* Previous CCW opcode       */
    U8      opcode;                     /* CCW opcode                */
    U16     reserved;
    BYTE    chanstat[PF_SIZE];          /* Channel status            */
    U32     ccwaddr[PF_SIZE];           /* CCW address (SCSW CCW)    */
    U32     datalen[PF_SIZE];           /* Count (length)            */
    RADR    dataaddr[PF_SIZE];          /* Address                   */
    U8      ccwflags[PF_SIZE];          /* CCW flags                 */
    U16     ccwcount[PF_SIZE];          /* Count (length)            */
    U32     idawaddr[PF_SIZE];          /* IDAW address              */
    U8      idawtype[PF_SIZE];          /* IDAW type                 */
    U8      idawflag[PF_SIZE];          /* IDAW flags                */
};
typedef struct PREFETCH PREFETCH;

#endif /* PREFETCH_STRUCT */

/*--------------------------------------------------------------------*/
/* IODELAY - kludge                                                   */
/*--------------------------------------------------------------------*/
#ifdef OPTION_IODELAY_KLUDGE
#define IODELAY(_dev) \
do { \
  if (sysblk.iodelay > 0 && (_dev)->devchar[10] == 0x20) \
    USLEEP(sysblk.iodelay); \
} while(0)
#else
#define IODELAY(_dev)
#endif

/*--------------------------------------------------------------------*/
/* CHADDRCHK - validate guest channel i/o subsystem address           */
/*--------------------------------------------------------------------*/
#undef CHADDRCHK
#if defined(FEATURE_ADDRESS_LIMIT_CHECKING)
#define CHADDRCHK(_addr,_dev)                   \
  (   ((_addr) > (_dev)->mainlim)               \
    || (((_dev)->orb.flag5 & ORB5_A)            \
      && ((((_dev)->pmcw.flag5 & PMCW5_LM_LOW)  \
        && ((_addr) < sysblk.addrlimval))       \
      || (((_dev)->pmcw.flag5 & PMCW5_LM_HIGH)  \
        && ((_addr) >= sysblk.addrlimval)) ) ))
#else /* !defined(FEATURE_ADDRESS_LIMIT_CHECKING) */
#define CHADDRCHK(_addr,_dev) \
        ((_addr) > (_dev)->mainlim)
#endif /* defined(FEATURE_ADDRESS_LIMIT_CHECKING) */

/*--------------------------------------------------------------------*/
/*              C H A N N E L   P R O C E S S I N G                   */
/*--------------------------------------------------------------------*/
#ifndef _CHANNEL_C
#define _CHANNEL_C

static INLINE U64
BytesToEndOfStorage( const RADR addr, const DEVBLK* dev )
{
    if (dev) return ((addr <= dev->mainlim)   ? (dev->mainlim+1  - addr) : 0);
    else     return ((addr < sysblk.mainsize) ? (sysblk.mainsize - addr) : 0);
}

#define CAPPED_BUFFLEN(_addr,_len,_dev) \
    ((u_int)MIN((U64)(_len),(U64)BytesToEndOfStorage((_addr),(_dev))))


/*--------------------------------------------------------------------*/
/*  Inline routines for Conditions and Indications Cleared at the     */
/*  Subchannel by TEST SUBCHANNEL; use of the routines keep the       */
/*  logic flow through the table of conditions clean.                 */
/*                                                                    */
/*  SA22-7832-09, Figure 14-2, Conditions and Indications Cleared at  */
/*                the Subchannel by TEST SUBCHANNEL, p. 14-21.        */
/*                                                                    */
/*--------------------------------------------------------------------*/

static INLINE void
scsw_clear_n_C (SCSW* scsw)
{
    scsw->flag1 &= ~SCSW1_N;
}


static INLINE void
scsw_clear_q_C (SCSW* scsw)
{
    scsw->flag2 &= ~SCSW2_Q;
}


static INLINE void
scsw_clear_fc_C (SCSW* scsw)
{
    scsw->flag2 &= ~SCSW2_FC;
}


static INLINE void
scsw_clear_fc_Nc (SCSW* scsw)
{
    if (scsw->flag2 & SCSW2_FC_HALT &&
        scsw->flag3 & SCSW3_AC_SUSP)
        scsw_clear_fc_C(scsw);
}


static INLINE void
scsw_clear_ac_Cp (SCSW* scsw)
{
    scsw->flag2 &= ~(SCSW2_AC_RESUM |
                         SCSW2_AC_START |
                         SCSW2_AC_HALT  |
                         SCSW2_AC_CLEAR);
    scsw->flag3 &= ~SCSW3_AC_SUSP;
}


static INLINE void
scsw_clear_ac_Nr (SCSW* scsw)
{
    if (scsw->flag2 & SCSW2_FC_START &&
        scsw->flag3 & SCSW3_AC_SUSP)
    {
        if (scsw->flag2 & SCSW2_FC_HALT)
            scsw_clear_ac_Cp(scsw);
        else
        {
            scsw->flag2 &= ~SCSW2_AC_RESUM;
            scsw_clear_n_C(scsw);
        }
    }
}


static INLINE void
scsw_clear_sc_Cs (SCSW* scsw)
{
    scsw->flag3 &= ~(SCSW3_SC_ALERT |
                     SCSW3_SC_INTER |
                     SCSW3_SC_PRI   |
                     SCSW3_SC_SEC   |
                     SCSW3_SC_PEND);
}


/*--------------------------------------------------------------------*/
/*  AIPSX (dev)                                                       */
/*                                                                    */
/*  Generates the AIPSX indication for Figure 16-5, the Deferred-     */
/*  Condition-Code Meaning for Status-Pending Subchannel, Figure      */
/*  16-5.                                                             */
/*--------------------------------------------------------------------*/
static INLINE U8
AIPSX (const SCSW* scsw)
{
    U8  result =  (scsw->flag1 & SCSW1_A)           /* >> 4 << 4  */
               | ((scsw->flag1 & SCSW1_I) >> 2      /* >> 5 << 3  */
               |  (scsw->flag0 & SCSW0_S) >> 2)     /* >> 3 << 1  */
               | ((scsw->flag1 & SCSW1_P) >> 4)     /* >> 6 << 2  */
               |  (scsw->flag3 & SCSW3_SC_PEND);    /* >> 0 << 0  */
    return (result);
}


/*--------------------------------------------------------------------*/
/*  Queue I/O interrupt and update status (locked)                    */
/*                                                                    */
/*  Locks Held on Entry                                               */
/*    sysblk.intlock                                                  */
/*    dev->lock                                                       */
/*  Locks Held on Return                                              */
/*    sysblk.intlock                                                  */
/*    dev->lock                                                       */
/*  Locks Used                                                        */
/*    sysblk.iointqlk                                                 */
/*--------------------------------------------------------------------*/
static INLINE void
queue_io_interrupt_and_update_status_locked(DEVBLK* dev, int clrbsy)
{
    OBTAIN_IOINTQLK();
    {
        /* Ensure the interrupt is queued/dequeued per pending flag */
        if (dev->scsw.flag3 & SCSW3_SC_PEND)
            QUEUE_IO_INTERRUPT_QLOCKED( &dev->ioint, clrbsy );
        else
            DEQUEUE_IO_INTERRUPT_QLOCKED( &dev->ioint );

        /* Perform cleanup for DEVBLK flags being deprecated */
        subchannel_interrupt_queue_cleanup( dev );

        /* Update interrupts */
        UPDATE_IC_IOPENDING_QLOCKED();
    }
    RELEASE_IOINTQLK();

#if defined( OPTION_SHARED_DEVICES )
    /* Wake up any waiters if the device isn't active or reserved */
    if (!(dev->scsw.flag3 & (SCSW3_AC_SCHAC | SCSW3_AC_DEVAC)) &&
        !dev->reserved)
    {
        dev->shioactive = DEV_SYS_NONE;
        if (dev->shiowaiters)
            signal_condition (&dev->shiocond);
    }
#endif // defined( OPTION_SHARED_DEVICES )
}


/*--------------------------------------------------------------------*/
/*  Queue I/O interrupt and update status                             */
/*                                                                    */
/*  Locks Held on Entry                                               */
/*    None                                                            */
/*  Locks Held on Return                                              */
/*    None                                                            */
/*  Locks Used                                                        */
/*    dev->lock                                                       */
/*    sysblk.intlock                                                  */
/*    sysblk.iointqlk                                                 */
/*--------------------------------------------------------------------*/
static INLINE void
queue_io_interrupt_and_update_status(DEVBLK* dev, int clrbsy)
{
    if (likely(dev->scsw.flag3 & SCSW3_SC_PEND))
    {
        OBTAIN_INTLOCK( NULL );
        {
            OBTAIN_DEVLOCK( dev );
            {
                queue_io_interrupt_and_update_status_locked( dev, clrbsy );
            }
            RELEASE_DEVLOCK( dev );
        }
        RELEASE_INTLOCK( NULL );
    }
#if defined( OPTION_SHARED_DEVICES )
    else    /* No interrupt pending */
    {
        /* Wake up any waiters if the device isn't active or reserved */
        if (!(dev->scsw.flag3 & (SCSW3_AC_SCHAC | SCSW3_AC_DEVAC)) &&
            !dev->reserved)
        {
            OBTAIN_DEVLOCK( dev );
            {
                dev->shioactive = DEV_SYS_NONE;
                if (dev->shiowaiters)
                    signal_condition (&dev->shiocond);
            }
            RELEASE_DEVLOCK( dev );
        }
    }
#endif // defined( OPTION_SHARED_DEVICES )
}


/*-------------------------------------------------------------------*/
/* Copy memory backwards for READ BACKWARDS                          */
/*                                                                   */
/* Note: Routines exist that use READ BACKWARDS and PCI, processing  */
/*       data as copied. Consequently, memory copying must be done   */
/*       backwards to ensure that these routines have little chance  */
/*       of failure.                                                 */
/*                                                                   */
/*-------------------------------------------------------------------*/
static INLINE void
memcpy_backwards ( BYTE *to, BYTE *from, int length )
{
    if (length)
    {
        for (to += length, from += length; length; --length)
            *(--to) = *(--from);
    }
}


/*-------------------------------------------------------------------*/
/* FORMAT DATA                                                       */
/*-------------------------------------------------------------------*/
#if !defined(format_data)
#   define format_data(_buffer,_buflen,_data,_datalen)                 \
          _format_data((BYTE *)(_buffer),(u_int)(_buflen),             \
                       (BYTE *)(_data),(u_int)(_datalen))
#endif
static INLINE void
_format_data ( BYTE *buffer,  const u_int buflen,
               const BYTE *a, const u_int len )
{
u_int   i, k;                           /* Array subscripts          */
int     j;

    k = MIN(len, 16);

    if (k)
    {
        j = snprintf((char *)buffer, buflen,
                "%2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X "
                "%2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X",
                a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7],
                a[8], a[9], a[10], a[11], a[12], a[13], a[14], a[15]);
        if (j < 0)
            *buffer = 0;
        else
        {
            /* Blank out unused data */
            if (k != 16)
            {
                i = (k << 1) + (k >> 2);
                memset(buffer + i, ' ', j - i);
            }

            /* If space is available, add translation */
            if ((u_int)j < buflen)
            {
                /* Insert blank separator */
                buffer[j++] = ' ';

                /* If space still available, complete translation */
                if ((u_int)j < buflen)
                {
                    /* Adjust data display size if necessary */
                    if ((j + k) > buflen)
                        k = buflen - j;

                    /* Append translation */
                    prt_guest_to_host(a, buffer + j, k);
                }
            }
        }

    }
    else
        *buffer = 0;

} /* end function format_data */


/***************************************************************/
/* SPECIAL HANDLING FOR E7 Prefix CCW TO TRACE ENTIRE 64 BYTES */
/***************************************************************/
#if !defined(e7_format_data)
#   define e7_format_data(_buffer,_buflen,_data,_datalen)                 \
          _e7_format_data((BYTE *)(_buffer),(u_int)(_buflen),             \
                       (BYTE *)(_data),(u_int)(_datalen))
#endif
static INLINE void
_e7_format_data ( BYTE *buffer,  const u_int buflen,
               const BYTE *a, const u_int len )
{
u_int   i, k;                           /* Array subscripts          */
int     j;

    k = MIN(len, 64);

    if (k)
    {
        j = snprintf((char *)buffer, buflen,

                "%2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X "
                "%2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X "
                "%2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X "
                "%2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X "
                "%2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X "
                "%2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X "
                "%2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X "
                "%2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X"

                ,a[ 0], a[ 1], a[ 2], a[ 3], a[ 4], a[ 5], a[ 6], a[ 7]
                ,a[ 8], a[ 9], a[10], a[11], a[12], a[13], a[14], a[15]

                ,a[16], a[17], a[18], a[19], a[20], a[21], a[22], a[23]
                ,a[24], a[25], a[26], a[26], a[28], a[29], a[30], a[31]

                ,a[32], a[33], a[34], a[35], a[36], a[37], a[38], a[39]
                ,a[40], a[41], a[42], a[43], a[44], a[45], a[46], a[47]

                ,a[48], a[49], a[50], a[51], a[52], a[53], a[54], a[55]
                ,a[56], a[57], a[58], a[59], a[60], a[61], a[62], a[63]
        );

        if (j < 0)
            *buffer = 0;
        else
        {
            /* Blank out unused data */
            if (k != 64)
            {
                i = (k << 1) + (k >> 2);
                memset(buffer + i, ' ', j - i);
            }

            /* If space is available, add translation */
            if ((u_int)j < buflen)
            {
                /* Insert blank separator */
                buffer[j++] = ' ';

                /* If space still available, complete translation */
                if ((u_int)j < buflen)
                {
                    /* Adjust data display size if necessary */
                    if ((j + k) > buflen)
                        k = buflen - j;

                    /* Append translation */
                    prt_guest_to_host(a, buffer + j, k);
                }
            }
        }

    }
    else
        *buffer = 0;

} /* end function format_data */


/*-------------------------------------------------------------------*/
/* FORMAT I/O BUFFER DATA                                            */
/*-------------------------------------------------------------------*/
static INLINE void
format_iobuf_data ( const RADR addr, BYTE *dest, const DEVBLK *dev,
                    const u_int len )
{
u_int   k;                              /* Array subscripts          */
BYTE    workarea[17];                   /* Character string work     */

    k = MIN(sizeof(workarea)-1,CAPPED_BUFFLEN(addr,len,dev));

    if (k)
    {
        memcpy(workarea, dev->mainstor + addr, k);
        memcpy(dest, "=>", 2);
        format_data(dest + 2, 52, workarea,  k);
    }
    else
        *dest = 0;

} /* end function format_iobuf_data */

/***************************************************************/
/* SPECIAL HANDLING FOR E7 Prefix CCW TO TRACE ENTIRE 64 BYTES */
/***************************************************************/
static INLINE void
e7_format_iobuf_data ( const RADR addr, BYTE *dest, const DEVBLK *dev,
                    const u_int len )
{
u_int   k;                              /* Array subscripts          */
BYTE    workarea[4*17];                 /* Character string work     */

    k = MIN(sizeof(workarea)-1,CAPPED_BUFFLEN(addr,len,dev));

    if (k)
    {
        memcpy(workarea, dev->mainstor + addr, k);
        memcpy(dest, "=>", 2);
        e7_format_data(dest + 2, (4*64)-2-1, workarea,  k);
    }
    else
        *dest = 0;

} /* end function format_iobuf_data */


#if !DEBUG_DUMP
#define DUMP( _desc, _addr, _len )
#define DUMP_STORAGE( _desc, _addr, _len )
#else
/*-------------------------------------------------------------------*/
/* Dump data block                                                   */
/*-------------------------------------------------------------------*/

#define DUMP( _desc, _addr, _len ) \
       _dump( (char*)(_desc), (BYTE*)(_addr), (u_int)(_len), \
               __FILE__, __LINE__, __FUNCTION__ )

static void _dump ( const char *description, BYTE *addr, const u_int len,
                    const char* file, int line, const char* func )
{
int     i;
int     k = MIN( len, IOBUF_MINSIZE );
BYTE*   limit;
char    msgbuf[133];

    if (k)
    {
        /* Set limit */
        limit = addr + k;

        if (description && description[0])
            fwritemsg
            (
                file, line, func, WRMSG_NORMAL, stdout,
                MSG( HHC01390, "D", description )
            );

        for (i=0; addr < limit; addr += 16, i += 16, k -= 16)
        {
            MSGBUF( msgbuf, "%4.4X  => ", i );
            format_data( msgbuf+9, sizeof( msgbuf ) - 9, addr,  k );
            fwritemsg
            (
                file, line, func, WRMSG_NORMAL, stdout,
                MSG( HHC01390, "D", msgbuf )
            );
        }
    }
}


/*-------------------------------------------------------------------*/
/* Dump Storage                                                      */
/*-------------------------------------------------------------------*/

#define DUMP_STORAGE( _desc, _addr, _len ) \
       _dump_storage( (char*)(_desc), (RADR)(_addr), (u_int)(_len), \
                       __FILE__, __LINE__, __FUNCTION__ )

static void _dump_storage( const char* description, RADR addr, const u_int len,
                           const char* file, int line, const char* func )
{
u_int   k;                              /* Amount of storage to dump */
BYTE*   storage;                        /* Real storage address      */
BYTE*   limit;                          /* Display limit             */
char    msgbuf[133];                    /* Message buffer            */

    /* Set length to fully reside within defined storage */
    k = CAPPED_BUFFLEN( addr, len, NULL );

    if (k)
    {
        k       = MIN( k, IOBUF_MINSIZE );
        storage = sysblk.mainstor + addr;
        limit   = storage + k;

        if (description && description[0])
            fwritemsg
            (
                file, line, func, WRMSG_NORMAL, stdout,
                MSG( HHC01391, "D", description )
            );

        if (sysblk.mainsize > (2 * ONE_GIGABYTE))
        {
            for (; storage < limit; addr += 16, storage += 16, k -= 16)
            {
                MSGBUF( msgbuf, "%16.16"PRIX64" => ", addr );
                format_data( msgbuf+20, sizeof( msgbuf ) - 20, storage, k );
                fwritemsg
                (
                    file, line, func, WRMSG_NORMAL, stdout,
                    MSG( HHC01391, "D", msgbuf )
                );
            }
        }
        else
        {
            for (; storage < limit; addr += 16, storage += 16, k -= 16)
            {
                MSGBUF( msgbuf, "%8.8X => ", (u_int)addr );
                format_data( msgbuf+12, sizeof( msgbuf ) - 12, storage, k );
                fwritemsg
                (
                    file, line, func, WRMSG_NORMAL, stdout,
                    MSG( HHC01391, "D", msgbuf )
                );
            }
        }
    }
}
#endif // DEBUG_DUMP


/*-------------------------------------------------------------------*/
/* Display channel command word and data                             */
/*-------------------------------------------------------------------*/

#define DISPLAY_CCW( _did, _dev, _ccw, _addr, _count, _flags ) \
    _display_ccw( (_did), (_dev), (_ccw), (_addr), (_count), (_flags), \
                  __FILE__, __LINE__, __FUNCTION__ )

static void _display_ccw( bool* did_ccw_trace, const DEVBLK* dev,
                          const BYTE ccw[], const U32 addr,
                          const U32 count, const U8 flags,
                          const char* file, int line, const char* func )
{
    if (*did_ccw_trace)         // (did we do this already?)
        return;                 // (then don't do it again!)

    if (1
        &&  dev->ccwopstrace    // (tracing ONLY specific CCWs?)
        && !dev->ccwops[ccw[0]] // (and NOT interested in this one?)
    )
        return;                 // (no, then don't trace it!)

    if (dev->ccwtrace && sysblk.traceFILE)
    {
        BYTE len, amt;

        len = (BYTE) CAPPED_BUFFLEN( addr, count, dev );
        amt = (ccw[0] == 0xE7) ? 64 : 16;
        tf_1315( dev, ccw, addr, count, dev->mainstor + addr, MIN( amt, len ));
    }
    else
    {
        BYTE area[4*64];    // (4 x to trace all 64 bytes of E7 Prefix CCW)

        /* No data to be formatted if CCW is a NOP or TIC
           or the CCW "Skip data transfer" flag is on. */
        if (0
            || flags & CCW_FLAGS_SKIP
            || IS_CCW_NOP( ccw[0] )
            || IS_CCW_TIC( ccw[0] )
        )
            area[0] = 0;
        else
        {
            if (ccw[0] == 0xE7)
                e7_format_iobuf_data( addr, area, dev, count );
            else
                format_iobuf_data( addr, area, dev, count );
        }

        // "%1d:%04X CHAN: ccw %2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X%s"
        fwritemsg
        (
            file, line, func, WRMSG_NORMAL, stdout,
            MSG
            (
                HHC01315, "I", LCSS_DEVNUM,
                ccw[0], ccw[1], ccw[2], ccw[3],
                ccw[4], ccw[5], ccw[6], ccw[7], area
            )
        );
    }

    *did_ccw_trace = true; // (remember we did this)
}


/*-------------------------------------------------------------------*/
/* Format default interpretation of first two sense bytes            */
/*-------------------------------------------------------------------*/
DLL_EXPORT void default_sns( char* buf, size_t buflen, BYTE b0, BYTE b1 )
{
    snprintf( buf, buflen, "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s"

            , (b0 & SENSE_CR  ) ? "CMDREJ " : ""
            , (b0 & SENSE_IR  ) ? "INTREQ " : ""
            , (b0 & SENSE_BOC ) ? "BUSCK "  : ""
            , (b0 & SENSE_EC  ) ? "EQPCK "  : ""
            , (b0 & SENSE_DC  ) ? "DATCK "  : ""
            , (b0 & SENSE_OR  ) ? "OVRUN "  : ""
            , (b0 & SENSE_CC  ) ? "CTLCK "  : ""
            , (b0 & SENSE_OC  ) ? "OPRCK "  : ""

            , (b1 & SENSE1_PER) ? "PERM "   : ""
            , (b1 & SENSE1_ITF) ? "ITF "    : ""
            , (b1 & SENSE1_EOC) ? "EOC "    : ""
            , (b1 & SENSE1_MTO) ? "MSG "    : ""
            , (b1 & SENSE1_NRF) ? "NRF "    : ""
            , (b1 & SENSE1_FP ) ? "FP "     : ""
            , (b1 & SENSE1_WRI) ? "WRI "    : ""
            , (b1 & SENSE1_IE ) ? "IE "     : ""
    );
}


/*-------------------------------------------------------------------*/
/* Display interpretation of first two sense bytes                   */
/*-------------------------------------------------------------------*/

#define DISPLAY_SENSE( _dev ) \
    _display_sense( (_dev), __FILE__, __LINE__, __FUNCTION__ )

static void _display_sense( const DEVBLK* dev,
                            const char* file, int line, const char* func )
{
    char snsbuf[128];

    /* Let the device interpret its own sense bytes if possible */
    if (dev->sns)
        dev->sns( dev, snsbuf, sizeof( snsbuf ));
    else
        /* Otherwise we use our default interpretation */
        default_sns( snsbuf, sizeof( snsbuf ), dev->sense[0], dev->sense[1] );

    // "%1d:%04X CHAN: sense %s"
    fwritemsg
    (
        file, line, func, WRMSG_NORMAL, stdout,
        MSG( HHC01314, "I", LCSS_DEVNUM, snsbuf )
    );
}


/*-------------------------------------------------------------------*/
/* Display IDAW and Data                                             */
/*-------------------------------------------------------------------*/

#define DISPLAY_IDAW( _dev, _type, _flag, _addr, _count ) \
    _display_idaw( (_dev), (_type), (_flag), (_addr), (_count), \
                    __FILE__, __LINE__, __FUNCTION__ )

static void _display_idaw( const DEVBLK* dev, const BYTE type, const BYTE flag,
                           const RADR addr, const U16 count,
                           const char* file, int line, const char* func )
{
    if (1
        &&  dev->ccwopstrace            // (tracing ONLY specific CCWs?)
        && !dev->ccwops[ dev->code ]    // (and NOT interested in this one?)
    )
        return;                         // (no, then don't trace it!)

    if (dev->ccwtrace && sysblk.traceFILE)
    {
        BYTE len, amt;

        len = (BYTE) CAPPED_BUFFLEN( addr, count, dev );
        amt = (dev->code == 0xE7) ? 64 : 16;
        tf_1301( dev, addr, count, dev->mainstor + addr, MIN( amt, len ), flag, type );
    }
    else
    {
        BYTE area[64];

        format_iobuf_data( addr, area, dev, count );

        switch (type)
        {
            case PF_IDAW1:
                // "%1d:%04X CHAN: idaw %8.8"PRIX32", len %3.3"PRIX16": %s"
                fwritemsg
                (
                    file, line, func, WRMSG_NORMAL, stdout,
                    MSG( HHC01302, "I", LCSS_DEVNUM, (U32)addr, count, area )
                );
                break;

            case PF_IDAW2:
                // "%1d:%04X CHAN: idaw %16.16"PRIX64", len %4.4"PRIX16": %s"
                fwritemsg
                (
                    file, line, func, WRMSG_NORMAL, stdout,
                    MSG( HHC01303, "I", LCSS_DEVNUM, (U64)addr, count, area )
                );
                break;

            case PF_MIDAW:
                // "%1d:%04X CHAN: midaw %2.2X %4.4"PRIX16" %16.16"PRIX64": %s"
                fwritemsg
                (
                    file, line, func, WRMSG_NORMAL, stdout,
                    MSG( HHC01301, "I", LCSS_DEVNUM, flag, count, (U64)addr, area )
                );
                break;
        }
    }
}


/*-------------------------------------------------------------------*/
/* DISPLAY CHANNEL STATUS WORD                                       */
/*-------------------------------------------------------------------*/

#define DISPLAY_CSW( _dev, _scsw ) \
    _display_csw( (_dev), (_scsw), __FILE__, __LINE__, __FUNCTION__ )

static void _display_csw( const DEVBLK* dev, const BYTE csw[],
                          const char* file, int line, const char* func )
{
    if (dev->ccwtrace && sysblk.traceFILE)
        tf_1316( dev, csw );
    else
    {
        // "%1d:%04X CHAN: csw %2.2X, stat %2.2X%2.2X, count %2.2X%2.2X, ccw %2.2X%2.2X%2.2X"
        fwritemsg
        (
            file, line, func, WRMSG_NORMAL, stdout,
            MSG
            (
                HHC01316, "I", LCSS_DEVNUM,
                csw[0],
                csw[4], csw[5], csw[6], csw[7],
                csw[1], csw[2], csw[3]
            )
        );
    }
}


/*-------------------------------------------------------------------*/
/* DISPLAY SUBCHANNEL STATUS WORD                                    */
/*-------------------------------------------------------------------*/

#define DISPLAY_SCSW( _dev, _scsw ) \
    _display_scsw( (_dev), (_scsw), __FILE__, __LINE__, __FUNCTION__ )

static void _display_scsw( const DEVBLK* dev, const SCSW scsw,
                           const char* file, int line, const char* func )
{
    if (ARCH_370_IDX == sysblk.arch_mode)
    {
        U8 csw[8];
        scsw2csw( &scsw, csw );
//      DISPLAY_CSW( dev, csw );
        _display_csw( dev, csw, file, line, func );
        return;
    }

    if (dev->ccwtrace && sysblk.traceFILE)
        tf_1317( dev, scsw );
    else
    {
        // "%1d:%04X CHAN: scsw %2.2X%2.2X%2.2X%2.2X, stat %2.2X%2.2X, count %2.2X%2.2X, ccw %2.2X%2.2X%2.2X%2.2X"
        fwritemsg
        (
            file, line, func, WRMSG_NORMAL, stdout,
            MSG
            (
                HHC01317, "I", LCSS_DEVNUM,
                scsw.flag0, scsw.flag1, scsw.flag2, scsw.flag3,
                scsw.unitstat, scsw.chanstat,
                scsw.count[0], scsw.count[1],
                scsw.ccwaddr[0], scsw.ccwaddr[1],
                scsw.ccwaddr[2], scsw.ccwaddr[3]
            )
        );
    }
}


/*-------------------------------------------------------------------*/
/*  Display prefetch table                                           */
/*-------------------------------------------------------------------*/
#if !DEBUG_PREFETCH
#define DISPLAY_PREFETCH( _prefetch, _ps, _count, _residual, _more )
#else
#define DISPLAY_PREFETCH( _prefetch, _ps, _count, _residual, _more ) \
    _display_prefetch( _prefetch, _ps, _count, _residual, _more, \
                       __FILE__, __LINE__, __FUNCTION__ )

static void _display_prefetch( PREFETCH* prefetch, const u_int ps,
                               const U32 count, const U32 residual,
                               const u_int more,
                               const char* file, int line, const char* func )
{
u_int   limit = MIN( prefetch->seq, PF_SIZE );
u_int   ts;
U32     used = count - residual;
char    msgbuf[133];

    MSGBUF( msgbuf, "Prefetch ps=%d count=%llu (%8.8LLX) "
                    "residual=%llu (%8.8LLX) more=%d "
                    "used=%llu (%8.8LLX)",
                    ps, count, count, residual, residual, more,
                    used, used );
    fwritemsg
    (
        file, line, func, WRMSG_NORMAL, stdout,
        MSG( HHC01392, "D", msgbuf )
    );

    MSGBUF( msgbuf, "Prefetch seq=%d req=%d (%04.4X) pos=%d (%04.4X) "
                    "prevcode=%02.2X code=%02.2X",
                    prefetch->seq, prefetch->reqcount,
                    prefetch->reqcount, prefetch->pos, prefetch->pos,
                    prefetch->prevcode, prefetch->opcode );
    fwritemsg
    (
        file, line, func, WRMSG_NORMAL, stdout,
        MSG( HHC01392, "D", msgbuf )
    );

    // (column header...)

    MSGBUF( msgbuf,
            "Prefetch "
            "  n "              /* ts                                */
            "  "                /* Select pointer                    */
            "cs "               /* Channel Status                    */
            "scsw-ccw "         /* CCW Address                       */
            "data-len "         /* Data length                       */
            "    data-address " /* Data address                      */
            "fl "               /* CCW Flags                         */
            "   count "         /* CCW Count                         */
            "idawaddr "         /* IDAW Address                      */
            "it "               /* IDAW Type                         */
            "if"                /* IDAW Flag                         */
          );
    fwritemsg
    (
        file, line, func, WRMSG_NORMAL, stdout,
        MSG( HHC01392, "D", msgbuf )
    );

    for (ts=0; ts < limit; ts++)
    {
        MSGBUF( msgbuf,
                "Prefetch "
                "%3d "           /* ts                               */
                "%2s"            /* Select pointer (ps)              */
                "%2.2X "         /* Channel Status                   */
                "%8.8X "         /* CCW Address (SCSW CCW Address)   */
                "%8.8X "         /* Data length                      */
                "%16.16"PRIX64" "/* Data address                     */
                "%2.2X "         /* CCW Flags                        */
                "%8.8X "         /* CCW Count                        */
                "%8.8X "         /* IDAW Address                     */
                "%2d "           /* IDAW Type                        */
                "%2.2X",         /* IDAW Flag                        */

                ts, ((ps == ts) ? "->" : "  "),
                prefetch->chanstat[ts], prefetch->ccwaddr [ts],
                prefetch->datalen [ts], prefetch->dataaddr[ts],
                prefetch->ccwflags[ts], prefetch->ccwcount[ts],
                prefetch->idawaddr[ts], prefetch->idawtype[ts],
                prefetch->idawflag[ts] );
        fwritemsg
        (
            file, line, func, WRMSG_NORMAL, stdout,
            MSG( HHC01392, "D", msgbuf )
        );
    }
}
#endif /* DEBUG_PREFETCH */


/*-------------------------------------------------------------------*/
/* STORE CHANNEL ID                                                  */
/*-------------------------------------------------------------------*/
int
stchan_id (REGS *regs, U16 chan)
{
U32     chanid;                         /* Channel identifier word   */
int     devcount = 0;                   /* #of devices on channel    */
DEVBLK *dev;                            /* -> Device control block   */
PSA_3XX *psa;                           /* -> Prefixed storage area  */

    /* Find a device on specified channel */
    for (dev = sysblk.firstdev; dev != NULL; dev = dev->nextdev)
    {
        /* Skip "devices" that don't actually exist */
        if (!IS_DEV(dev))
            continue;

        /* Skip the device if not on specified channel */
        if ((dev->devnum & 0xFF00) != chan
         || (dev->pmcw.flag5 & PMCW5_V) == 0
#if defined(FEATURE_CHANNEL_SWITCHING)
         || regs->chanset != dev->chanset
#endif /*defined(FEATURE_CHANNEL_SWITCHING)*/
                                            )
            continue;

        /* Found device on the channel */
        devcount++;
        break;
    } /* end for(dev) */

    /* Exit with condition code 3 if no devices on channel */
    if (devcount == 0)
        return 3;

    /* Construct the channel id word
     *
     * According to GA22-7000-4, Page 192, 2nd paragraph, channel 0 is a
     * Byte Multiplexor.. Return STIDC data accordingly.
     * ISW 20080313
     */
    if(!chan)
    {
        chanid = CHANNEL_MPX;
    }
    else
    {
        /* Translate channel type back to S/370 terms */
        switch (dev->chptype[0])
        {
            case CHP_TYPE_UNDEF:
                chanid = CHANNEL_SEL;
                break;
            case CHP_TYPE_BYTE:
                chanid = CHANNEL_MPX;
                break;
            default:
                chanid = CHANNEL_BMX;
                break;
        }
    }

    /* Store the channel id word at PSA+X'A8' */
    psa = (PSA_3XX*)(regs->mainstor + regs->PX);
    STORE_FW(psa->chanid, chanid);

    /* Exit with condition code 0 indicating channel id stored */
    return 0;

} /* end function stchan_id */


/*-------------------------------------------------------------------*/
/* TEST CHANNEL                                                      */
/*-------------------------------------------------------------------*/
int
testch (REGS *regs, U16 chan)
{
DEVBLK *dev;                            /* -> Device control block   */
int     devcount = 0;                   /* Number of devices found   */
int     cc = 0;                         /* Returned condition code   */

    /* Scan devices on the channel */
    for (dev = sysblk.firstdev; dev != NULL; dev = dev->nextdev)
    {
        /* Skip "devices" that don't actually exist */
        if (!IS_DEV(dev))
            continue;

        /* Skip the device if not on specified channel */
        if ((dev->devnum & 0xFF00) != chan
         || (dev->pmcw.flag5 & PMCW5_V) == 0
#if defined(FEATURE_CHANNEL_SWITCHING)
         || regs->chanset != dev->chanset
#endif /*defined(FEATURE_CHANNEL_SWITCHING)*/
                                          )
            continue;

        /* Increment device count on this channel */
        devcount++;

        /* Test for pending interrupt */
        if (IOPENDING(dev))
        {
            cc = 1;
            break;
        }
    }

    /* Set condition code 3 if no devices found on the channel */
    if (devcount == 0)
        cc = 3;

    return cc;

} /* end function testch */


/*-------------------------------------------------------------------*/
/* TEST I/O                                                          */
/*-------------------------------------------------------------------*/
int
testio (REGS *regs, DEVBLK *dev, BYTE ibyte)
{
    int     cc;                         /* Condition code            */
    IRB     irb;                        /* Interrupt request block   */
    IOINT*  ioint;                      /* -> I/O interrupt          */
    SCSW*   scsw;                       /* -> SCSW                   */

    UNREFERENCED(ibyte);

    OBTAIN_INTLOCK( regs );
    {
        OBTAIN_DEVLOCK( dev );
        {
            /* Test device status and set condition code */
            if ((dev->busy
#if defined( OPTION_SHARED_DEVICES )
                && dev->shioactive == DEV_SYS_LOCAL
#endif
            ) || dev->startpending)
            {
                /* Set condition code 2 for device busy */
                cc = 2;
            }
            else if (IOPENDING(dev))
            {
                /* Issue TEST SUBCHANNEL */
                cc = test_subchan_locked (regs, dev, &irb, &ioint, &scsw);

                if (cc)         /* Status incomplete */
                    cc = 2;
                else            /* Status pending */
                {
                    cc = 1;     /* CSW stored */

                    OBTAIN_IOINTQLK();
                    {
                        /* Dequeue the interrupt */
                        DEQUEUE_IO_INTERRUPT_QLOCKED( ioint );

                        /* Store the channel status word at PSA+X'40' */
                        store_scsw_as_csw( regs, &irb.scsw );

                        /* Cleanup after SCSW */
                        subchannel_interrupt_queue_cleanup( dev );
                        UPDATE_IC_IOPENDING_QLOCKED();
                    }
                    RELEASE_IOINTQLK();
                }
            }
            else
                cc = 0;         /* Available */

            if (dev->ccwtrace)
            {
                if (sysblk.traceFILE)
                    tf_1318( dev, cc );
                else
                    // "%1d:%04X CHAN: test I/O: cc=%d"
                    WRMSG( HHC01318, "I", LCSS_DEVNUM, cc );
            }
        }
        RELEASE_DEVLOCK( dev );
    }
    RELEASE_INTLOCK( regs );

    /* Return the condition code */
    return (cc);

} /* end function testio */


/*-------------------------------------------------------------------*/
/* HALT I/O                                                          */
/*-------------------------------------------------------------------*/
int haltio( REGS *regs, DEVBLK *dev, BYTE ibyte )
{
    int       cc;                       /* Condition code            */
    PSA_3XX*  psa;                      /* -> Prefixed storage area  */

    UNREFERENCED( ibyte );

    if (dev->ccwtrace)
    {
        if (sysblk.traceFILE)
            tf_1329( dev );
        else
            // "%1d:%04X CHAN: halt I/O"
            WRMSG( HHC01329, "I", LCSS_DEVNUM );
    }

    OBTAIN_INTLOCK( regs );
    {
        OBTAIN_DEVLOCK( dev );
        {
            /* Test device status and set condition code */
            if (dev->busy
                /* CTCE devices need dev->hnd->halt to always be called
                   even when not busy!
                */
                || dev->ctctype == CTC_CTCE)
            {
                /* Invoke the provided halt device routine */
                /* if it has been provided by the handler  */
                /* code at init                            */
                if (dev->hnd->halt != NULL)
                {
                    dev->hnd->halt( dev );
                    psa = (PSA_3XX*)( regs->mainstor + regs->PX );
                    psa->csw[4] = 0;    /*  Store partial CSW   */
                    psa->csw[5] = 0;
                    cc = 1;             /*  CC1 == CSW stored   */
                }
                else
                {
                    /* Set condition code 2 if device is busy */
                    cc = 2;

                    /* Tell channel and device to halt */
                    dev->scsw.flag2 |= SCSW2_AC_HALT;

                    /* Clear pending interrupts */

                    DEQUEUE_IO_INTERRUPT_QLOCKED( &dev->pciioint  );
                    DEQUEUE_IO_INTERRUPT_QLOCKED( &dev->ioint     );
                    DEQUEUE_IO_INTERRUPT_QLOCKED( &dev->attnioint );

                    dev->pciscsw.flag3  &= ~(SCSW3_SC_ALERT |
                                             SCSW3_SC_PRI   |
                                             SCSW3_SC_SEC   |
                                             SCSW3_SC_PEND);

                    dev->scsw.flag3     &= ~(SCSW3_SC_ALERT |
                                             SCSW3_SC_PRI   |
                                             SCSW3_SC_SEC   |
                                             SCSW3_SC_PEND);

                    dev->attnscsw.flag3 &= ~(SCSW3_SC_ALERT |
                                             SCSW3_SC_PRI   |
                                             SCSW3_SC_SEC   |
                                             SCSW3_SC_PEND);

                    subchannel_interrupt_queue_cleanup( dev );
                }
            }
            else if (!IOPENDING( dev ) && dev->ctctype != CTC_LCS)
            {
                /* Set condition code 1 */
                cc = 1;

                /* Store the channel status word at PSA+X'40' */
                store_scsw_as_csw( regs, &dev->scsw );

                if (dev->ccwtrace)
                {
                    psa = (PSA_3XX*)(regs->mainstor + regs->PX);
                    DISPLAY_CSW( dev, psa->csw );
                }
            }
            else if (dev->ctctype == CTC_LCS)
            {
                /* Set cc 1 if interrupt is not pending and LCS CTC */
                cc = 1;

                /* Store the channel status word at PSA+X'40' */
                store_scsw_as_csw( regs, &dev->scsw );

                if (dev->ccwtrace)
                {
                    psa = (PSA_3XX*)(regs->mainstor + regs->PX);
                    if (sysblk.traceFILE)
                    {
                        tf_1330( dev );
                        DISPLAY_CSW( dev, psa->csw );
                    }
                    else
                    {
                        // "%1d:%04X CHAN: HIO modification executed: cc=1"
                        WRMSG( HHC01330, "I", LCSS_DEVNUM );
                        DISPLAY_CSW( dev, psa->csw );
                    }
                }
            }
            else
            {
                /* Set condition code 0 if interrupt is pending */
                cc = 0;
            }

            /* Update interrupt status */
            subchannel_interrupt_queue_cleanup( dev );
            UPDATE_IC_IOPENDING_QLOCKED();
        }
        RELEASE_DEVLOCK( dev );
    }
    RELEASE_INTLOCK( regs );

    /* Return the condition code */
    return cc;

} /* end function haltio */


/*-------------------------------------------------------------------*/
/* CANCEL SUBCHANNEL                                                 */
/*-------------------------------------------------------------------*/
/* Input                                                             */
/*      regs    -> CPU register context                              */
/*      dev     -> Device control block                              */
/* Return value                                                      */
/*      The return value is the condition code for the XSCH          */
/*      0=start function cancelled (not yet implemented)             */
/*      1=status pending (no action taken)                           */
/*      2=function not applicable                                    */
/*-------------------------------------------------------------------*/
int
cancel_subchan (REGS *regs, DEVBLK *dev)
{
int     cc;                             /* Condition code            */

    UNREFERENCED(regs);

    OBTAIN_DEVLOCK( dev );
    {
#if defined(_FEATURE_IO_ASSIST)
        if(SIE_MODE(regs)
          && (regs->siebk->zone != dev->pmcw.zone
            || !(dev->pmcw.flag27 & PMCW27_I)))
        {
            RELEASE_DEVLOCK( dev );
            longjmp(regs->progjmp,SIE_INTERCEPT_INST);
        }
#endif

        /* Check pending status */
        if ((dev->pciscsw.flag3 & SCSW3_SC_PEND)
         || (dev->scsw.flag3    & SCSW3_SC_PEND)
         || (dev->attnscsw.flag3    & SCSW3_SC_PEND))
            cc = 1;
        else
        {
            cc = 1;

            OBTAIN_IOQLOCK();
            {
                if(sysblk.ioq != NULL)
                {
                DEVBLK *tmp;

                    /* special case for head of queue */
                    if(sysblk.ioq == dev)
                    {
                        /* Remove device from the i/o queue */
                        sysblk.ioq = dev->nextioq;
                        cc = 0;
                    }
                    else
                    {
                        /* Search for device on i/o queue */
                        for(tmp = sysblk.ioq;
                            tmp->nextioq != NULL && tmp->nextioq != dev;
                            tmp = tmp->nextioq);

                        /* Remove from queue if found */
                        if(tmp->nextioq == dev)
                        {
                            tmp->nextioq = tmp->nextioq->nextioq;
                            sysblk.devtunavail = MAX(0, sysblk.devtunavail - 1);
                            cc = 0;
                        }
                    }
                }
            }
            RELEASE_IOQLOCK();

            /* Reset the device */
            if(!cc)
            {
                /* Terminate suspended channel program */
                if (dev->scsw.flag3 & SCSW3_AC_SUSP)
                {
                    dev->suspended = 0;
                    schedule_ioq(NULL, dev);
                }
                else
                {
                    /* Reset the scsw */
                    dev->scsw.flag2 &= ~(SCSW2_AC_RESUM |
                                         SCSW2_FC_START |
                                         SCSW2_AC_START);
                    dev->scsw.flag3 &= ~(SCSW3_AC_SCHAC |
                                         SCSW3_AC_DEVAC |
                                         SCSW3_AC_SUSP);

                    /* Reset the device busy indicator */
                    clear_subchannel_busy(dev);
                }
            }
        }
    }
    RELEASE_DEVLOCK( dev );

    /* Return the condition code */
    return cc;

} /* end function cancel_subchan */


/*-------------------------------------------------------------------*/
/*  Perform DEVBLK cleanup following queueing/dequeueing of          */
/*  interrupt                                                        */
/*-------------------------------------------------------------------*/
static INLINE void
subchannel_interrupt_queue_cleanup(DEVBLK* dev)
{
    /* Begin the deprecation of the pending status bits in the
     * DEVBLK; the pending bits are now set from the corresponding
     * SCSWs.
     *
     * Implementation Note
     * -------------------
     * While SCSW3_SC_PEND is bit 31 of the SCSW, the old pending flags
     * are at "unknown" bit positions. Ensure that *ALL* C compilers
     * set the target bit fields properly.
     */
    dev->pcipending  = (dev->pciscsw.flag3  & SCSW3_SC_PEND) ? 1 : 0;
    dev->pending     = (dev->scsw.flag3     & SCSW3_SC_PEND) ? 1 : 0;
    dev->attnpending = (dev->attnscsw.flag3 & SCSW3_SC_PEND) ? 1 : 0;
}


/*-------------------------------------------------------------------*/
/* Perform clear operation for TEST SUBCHANNEL                       */
/*                                                                   */
/* SA22-7832-09, Figure 14-2, Conditions and Indications Cleared at  */
/*               the Subchannel by TEST SUBCHANNEL, p. 14-21.        */
/*                                                                   */
/* Locks on Entry                                                    */
/*   dev->lock                                                       */
/*                                                                   */
/*-------------------------------------------------------------------*/
static INLINE int
test_subchan_clear(DEVBLK* dev, SCSW* scsw)
{
    int cc = 1;                         /* Status not cleared        */

    /* Remove pending condition and requirement for TEST SUBCHANNEL */
    dev->tschpending = 0;

    /* Clearing only occurs if status is pending */
    if (scsw != NULL && scsw->flag3 & SCSW3_SC_PEND)
    {
        if (scsw->flag3 & SCSW3_SC_INTER)
        {
                                        /* SA22-7832-09, page 16-22  */
            scsw->flag1 &= ~SCSW1_Z;    /* Reset Zero Condition Code */

            scsw_clear_fc_Nc(scsw);
            scsw_clear_ac_Nr(scsw);
            scsw_clear_sc_Cs(scsw);
            /*          n_Nr(scsw); Implied by ac_NR */
            scsw_clear_q_C(scsw);
            cc = 0;
        }
        else if ((scsw->flag3 & (SCSW3_SC_ALERT  |
                                 SCSW3_SC_PRI    |
                                 SCSW3_SC_SEC))  ||
                 (scsw->flag3 & SCSW3_SC) == SCSW3_SC_PEND)
        {
            /* SA22-7832-09, page 16-17, TEST SUBCHANNEL clears
             * device suspended indication if suppressed
             */
            if (1
                && scsw == &dev->scsw
                && scsw->flag1 & SCSW1_U
                && scsw->flag3 & SCSW3_AC_SUSP
            )
            {
                dev->busy = 0;
                dev->suspended = 0;
            }

            scsw_clear_fc_C(scsw);
            scsw_clear_ac_Cp(scsw);
            scsw_clear_sc_Cs(scsw);
            scsw_clear_n_C(scsw);
            scsw_clear_q_C(scsw);
            cc = 0;
        }
    }

    /* Ensure old DEVBLK status bits are maintained */
    subchannel_interrupt_queue_cleanup(dev);

    /* Return completion code */
    return (cc);
}


/*-------------------------------------------------------------------*/
/* TEST SUBCHANNEL (LOCKED)                                          */
/*-------------------------------------------------------------------*/
/* Core routine for TEST SUBCHANNEL; also used by the                */
/* present_io_interrupt routine for S/360 and S/370 channel support. */
/*                                                                   */
/* Input                                                             */
/*      regs    -> CPU register context                              */
/*      dev     -> Device control block                              */
/* Output                                                            */
/*      irb     -> Interruption response block                       */
/*      ioint   -> Interruption                                      */
/*      scsw    -> SCSW                                              */
/* Return value                                                      */
/*      The return value is the condition code for the TSCH          */
/*      instruction:  0=status was pending and is now cleared,       */
/*      1=no status was pending.  The IRB is updated in both cases.  */
/* Locks held on entry                                               */
/*      INTLOCK         Held by caller                               */
/*      dev->lock       Held by caller                               */
/*      iointqlk        Held by caller                               */
/*-------------------------------------------------------------------*/
int
test_subchan_locked (REGS* regs, DEVBLK* dev,
                     IRB* irb, IOINT** ioint, SCSW** scsw)
{
    enum                                /* Status types              */
    {                                   /* ...                       */
        pci,                            /* PCI Status                */
        normal,                         /* Normal Status             */
        attn                            /* Attention Status          */
    } status;                           /* ...                       */

    int cc;                             /* Returned condition code   */

    UNREFERENCED( regs );

    if (unlikely((dev->pciscsw.flag3 & SCSW3_SC_PEND)))
    {
        status = pci;
        *ioint = &dev->pciioint;
        *scsw  = &dev->pciscsw;
    }
    else if (likely( (dev->scsw.flag3     & SCSW3_SC_PEND)  ||
                    !(dev->attnscsw.flag3 & SCSW3_SC_PEND)))
    {
        status = normal;
        *ioint = &dev->ioint;
        *scsw  = &dev->scsw;
    }
    else
    {
        status = attn;
        *ioint = &dev->attnioint;
        *scsw  = &dev->attnscsw;
    }

    /* Ensure status removed from interrupt queue */
    DEQUEUE_IO_INTERRUPT_QLOCKED(*ioint);

    /* Display the subchannel status word */
    if (dev->ccwtrace)
        DISPLAY_SCSW( dev, **scsw );

    /* Copy the SCSW to the IRB */
    irb->scsw = **scsw;

    /* Clear the ESW and ECW in the IRB */
    switch (status)
    {
        case normal:

            /* Copy the extended status word to the IRB */
            irb->esw = dev->esw;

            /* Copy the extended control word to the IRB */
            memcpy (irb->ecw, dev->ecw, sizeof(irb->ecw));
            break;

        default:
            /* Clear the ESW and ECW in the IRB */
            memset (&irb->esw, 0, sizeof(ESW));
            irb->esw.lpum = 0x80;
            memset (irb->ecw, 0, sizeof(irb->ecw));
            break;
    }

    /* Clear the subchannel and set condition code */
    cc = test_subchan_clear(dev, *scsw);

    /* Update pending interrupts */
    UPDATE_IC_IOPENDING_QLOCKED();

    /* Return condition code */
    return (cc);
}


/*-------------------------------------------------------------------*/
/* TEST SUBCHANNEL                                                   */
/*-------------------------------------------------------------------*/
/* Input                                                             */
/*      regs    -> CPU register context                              */
/*      dev     -> Device control block                              */
/* Output                                                            */
/*      irb     -> Interruption response block                       */
/* Return value                                                      */
/*      The return value is the condition code for the TSCH          */
/*      instruction:  0=status was pending and is now cleared,       */
/*      1=no status was pending.  The IRB is updated in both cases.  */
/*-------------------------------------------------------------------*/
int
test_subchan (REGS *regs, DEVBLK *dev, IRB *irb)
{
    int     cc;                         /* Condition code            */
    IOINT*  ioint;                      /* I/O interrupt structure   */
    SCSW*   scsw;                       /* SCSW structure            */

    OBTAIN_INTLOCK(regs);
    {
        OBTAIN_DEVLOCK( dev );
        {
#if defined(_FEATURE_IO_ASSIST)
            if(SIE_MODE(regs))
            {
                if (regs->siebk->zone != dev->pmcw.zone
                    || !(dev->pmcw.flag27 & PMCW27_I))
                {
                    RELEASE_DEVLOCK( dev );
                    RELEASE_INTLOCK(regs);
                    longjmp(regs->progjmp,SIE_INTERCEPT_INST);
                }

                /* For I/O assisted devices we must intercept if type B
                   status is present on the subchannel */
                if (dev->pciscsw.flag3 & SCSW3_SC_PEND &&
                    (regs->siebk->tschds & dev->pciscsw.unitstat  ||
                     regs->siebk->tschsc & dev->pciscsw.chanstat))
                {
                    dev->pmcw.flag27 &= ~PMCW27_I;
                    RELEASE_DEVLOCK( dev );
                    RELEASE_INTLOCK(regs);
                    longjmp(regs->progjmp,SIE_INTERCEPT_IOINST);
                }
            }
#endif
            OBTAIN_IOINTQLK();
            {
                /* Perform core of TEST SUBCHANNEL work */
                cc = test_subchan_locked( regs, dev, irb, &ioint, &scsw );
            }
            RELEASE_IOINTQLK();

            /* Display the condition code */
            if (dev->ccwtrace)
            {
                if (sysblk.traceFILE)
                    tf_1318( dev, cc );
                else
                    // "%1d:%04X CHAN: test I/O: cc=%d"
                    WRMSG( HHC01318, "I", LCSS_DEVNUM, cc );
            }
        }
        RELEASE_DEVLOCK( dev );
    }
    RELEASE_INTLOCK(regs);

    /* Return the condition code */
    return (cc);

} /* end function test_subchan */


/*-------------------------------------------------------------------*/
/* Perform CLEAR SUBCHANNEL operation                                */
/*-------------------------------------------------------------------*/
/* Input                                                             */
/*      dev     -> Device control block                              */
/* Locks on Entry                                                    */
/*      INTLOCK(NULL)   Held by caller                               */
/*      dev->lock       Held by caller                               */
/* Locks on Return                                                   */
/*      INTLOCK(NULL)   Held by caller                               */
/*      dev->lock       Held by caller                               */
/* Locks Used                                                        */
/*      sysblk.iointqlk                                             */
/*-------------------------------------------------------------------*/
void
perform_clear_subchan (DEVBLK *dev)
{
    /* Dequeue pending interrupts */
    OBTAIN_IOINTQLK();
    {
        DEQUEUE_IO_INTERRUPT_QLOCKED( &dev->ioint );
        dev->scsw.flag3 &= ~SCSW3_SC_PEND;
        dev->pending = 0;
        clear_device_busy(dev);
        clear_subchannel_busy(dev);
        DEQUEUE_IO_INTERRUPT_QLOCKED( &dev->pciioint );
        dev->pciscsw.flag3 &= ~SCSW3_SC_PEND;
        dev->pcipending = 0;
        clear_subchannel_busy_scsw(&dev->pciscsw);
        DEQUEUE_IO_INTERRUPT_QLOCKED( &dev->attnioint );
        dev->attnscsw.flag3 &= ~SCSW3_SC_PEND;
        dev->attnpending = 0;
        dev->tschpending = 0;

        /* [15.3.2] Perform clear function subchannel modification */
        dev->pmcw.pom = 0xFF;
        dev->pmcw.lpum = 0x00;
        dev->pmcw.pnom = 0x00;

        /* [15.3.3] Perform clear function signaling and completion */
        dev->scsw.flag0 = 0;
        dev->scsw.flag1 = 0;
        dev->scsw.flag2 &= ~(SCSW2_FC | SCSW2_AC);
        dev->scsw.flag2 |= SCSW2_FC_CLEAR;
        /* Shortcut setting of flag3 due to clear and setting of just the
         * status pending flag:
         *   scsw.flag3 &= ~(SCSW3_AC | SCSW3_SC);
         *   scsw.flag3 |= SCSW3_SC_PEND;
         */
        dev->scsw.flag3 = SCSW3_SC_PEND;
        store_fw (dev->scsw.ccwaddr, 0);
        dev->scsw.chanstat = 0;
        dev->scsw.unitstat = 0;
        store_hw (dev->scsw.count, 0);

        /* Queue the pending interrupt and update status */
        QUEUE_IO_INTERRUPT_QLOCKED( &dev->ioint, TRUE );
        subchannel_interrupt_queue_cleanup( dev );
        UPDATE_IC_IOPENDING_QLOCKED();
    }
    RELEASE_IOINTQLK();

    if (dev->ccwtrace)
    {
        if (sysblk.traceFILE)
            tf_1308( dev );
        else
            // "%1d:%04X CHAN: clear completed"
            WRMSG( HHC01308, "I", LCSS_DEVNUM );
    }

#if defined( OPTION_SHARED_DEVICES )
    /* Wake up any waiters if the device isn't reserved */
    if (!dev->reserved)
    {
        dev->shioactive = DEV_SYS_NONE;
        if (dev->shiowaiters)
           signal_condition (&dev->shiocond);
    }
#endif // defined( OPTION_SHARED_DEVICES )
}


/*-------------------------------------------------------------------*/
/* CLEAR SUBCHANNEL                                                  */
/*-------------------------------------------------------------------*/
/* Input                                                             */
/*      regs    -> CPU register context                              */
/*      dev     -> Device control block                              */
/*-------------------------------------------------------------------*/
void clear_subchan( REGS* regs, DEVBLK* dev )
{
    UNREFERENCED( regs );

    if (dev->ccwtrace)
    {
        if (sysblk.traceFILE)
            tf_1331( dev );
        else
            // "%1d:%04X CHAN: clear subchannel"
            WRMSG( HHC01331, "I", LCSS_DEVNUM );
    }

#if defined( _FEATURE_IO_ASSIST )
    if (1
        && SIE_MODE( regs )
        && (0
            || regs->siebk->zone != dev->pmcw.zone
            || !(dev->pmcw.flag27 & PMCW27_I)
           )
    )
    {
        longjmp( regs->progjmp, SIE_INTERCEPT_INST );
    }
#endif

    OBTAIN_INTLOCK( NULL );
    {
        OBTAIN_DEVLOCK( dev );
        {
            /* If the device is busy then signal the device to clear */
            if (0
                || (dev->busy
#if defined( OPTION_SHARED_DEVICES )
                && dev->shioactive == DEV_SYS_LOCAL
#endif
                   )
                || dev->startpending
                /* CTCE devices need dev->hnd->halt to always be called
                   even when not busy or startpending!
                */
                || dev->ctctype == CTC_CTCE
            )
            {
                /* Set clear pending condition */
                dev->scsw.flag2 |= SCSW2_FC_CLEAR | SCSW2_AC_CLEAR;

                /* Signal the subchannel to resume if it is suspended */
                if (dev->scsw.flag3 & SCSW3_AC_SUSP)
                {
                    dev->scsw.flag2 |= SCSW2_AC_RESUM;
                    schedule_ioq( NULL, dev );
                }

                /* Invoke the provided halt device routine if it has been
                 * provided by the handler code at init
                 */
                else if (dev->hnd->halt != NULL)
                {
                    /* Revert to just holding dev->lock */
                    RELEASE_DEVLOCK( dev );
                    RELEASE_INTLOCK( NULL );

                    OBTAIN_DEVLOCK( dev );
                    {
                        /* Call the device's halt routine */
                        dev->hnd->halt( dev );
                    }
                    RELEASE_DEVLOCK( dev );

                    /* Return to holding BOTH intlock *and* dev->lock */
                    OBTAIN_INTLOCK( NULL );
                    OBTAIN_DEVLOCK( dev );
                }
            }

            /* Perform clear subchannel operation and queue interrupt */
            perform_clear_subchan( dev );
        }
        RELEASE_DEVLOCK( dev );
    }
    RELEASE_INTLOCK( NULL );

} /* end function clear_subchan */


/*-------------------------------------------------------------------*/
/* Perform HALT and release device lock                              */
/*-------------------------------------------------------------------*/
/* Input                                                             */
/*      dev     -> Device control block                              */
/* Return value                                                      */
/*      None                                                         */
/* Locks                                                             */
/*      INTLOCK   must be held by caller on entry.                   */
/*      dev->lock must be held by caller on entry; lock is released  */
/*                prior to return.                                   */
/*-------------------------------------------------------------------*/
void
perform_halt_and_release_lock (DEVBLK *dev)
{
    /* If status incomplete,
     * [15.4.2] Perform halt function signaling
     */
    if (!(dev->scsw.flag3 & (SCSW3_SC_PRI | SCSW3_SC_SEC) &&
          dev->scsw.flag3 & SCSW3_SC_PEND))
    {
        /* Indicate HALT started */
        set_subchannel_busy(dev);
        dev->scsw.flag2 |= SCSW2_FC_HALT;
        dev->scsw.flag2 &= ~SCSW2_AC_HALT;

        /* If intermediate status pending with subchannel and device
         * active, reset intermediate status pending (HSCH, fifth
         * paragraph).
         */
        if ((dev->scsw.flag3 &
             (SCSW3_AC_SCHAC | SCSW3_AC_DEVAC | SCSW3_SC_INTER | SCSW3_SC_PEND)) ==
             (SCSW3_AC_SCHAC | SCSW3_AC_DEVAC | SCSW3_SC_INTER | SCSW3_SC_PEND))
            dev->scsw.flag3 &= ~(SCSW3_SC_INTER | SCSW3_SC_PEND);

        /* Interim post-facto check for missing deferred condition code
         * Based on the Deferred-Condition-Code Meaning for Status-
         * Pending Subchannel (Figure 16-5).
         */
        if (!(dev->scsw.flag0 & SCSW0_CC) &&
             (dev->scsw.flag2 & (SCSW2_FC_START |
                                 SCSW2_AC_RESUM |
                                 SCSW2_AC_START)))
        {
            switch (AIPSX(&dev->scsw) & 0x0F)
            {
                case 0x0F:
                case 0x0D:
                case 0x09:
                case 0x07:
                case 0x05:
                case 0x03:
                case 0x01:
                    dev->scsw.flag0 &= ~SCSW0_CC;
                    dev->scsw.flag1 &= ~SCSW1_Z;
                    dev->scsw.flag0 |=  SCSW0_CC_1;
                    break;
            }
        }

        /* Invoke the provided halt device routine if it has been
         * provided by the handler code at init; SCSW has not yet been
         * reset to permit device handlers to see HALT condition.
         */
        if (dev->hnd->halt != NULL)
            dev->hnd->halt( dev );

        /* Mark pending interrupt */
        dev->scsw.flag3 |= SCSW3_SC_PEND;
    }

    /* Trace HALT */
    if (dev->ccwtrace)
    {
        if (sysblk.traceFILE)
            tf_1300( dev, 0 );
        else
            // "%1d:%04X CHAN: halt subchannel: cc=%d"
            WRMSG( HHC01300, "I", LCSS_DEVNUM, 0 );
    }

    /* Queue pending I/O interrupt and update status */
    queue_io_interrupt_and_update_status_locked(dev,TRUE);

    RELEASE_DEVLOCK( dev );
}


/*-------------------------------------------------------------------*/
/* Perform HALT                                                      */
/*-------------------------------------------------------------------*/
/* Input                                                             */
/*      dev     -> Device control block                              */
/* Return value                                                      */
/*      None                                                         */
/* Locks on Entry                                                    */
/*      None                                                         */
/* Locks on Return                                                   */
/*      None                                                         */
/* Locks Used                                                        */
/*      dev->lock                                                    */
/*-------------------------------------------------------------------*/
void
perform_halt (DEVBLK *dev)
{
    OBTAIN_INTLOCK( NULL );
    {
        OBTAIN_DEVLOCK( dev );
        perform_halt_and_release_lock(dev);
    }
    RELEASE_INTLOCK( NULL );
}


/*-------------------------------------------------------------------*/
/* HALT SUBCHANNEL                                                   */
/*-------------------------------------------------------------------*/
/* Input                                                             */
/*      regs    -> CPU register context                              */
/*      dev     -> Device control block                              */
/* Return value                                                      */
/*      The return value is the condition code for the HSCH          */
/*      instruction:  0=Halt initiated, 1=Non-intermediate status    */
/*      pending, 2=Busy                                              */
/*-------------------------------------------------------------------*/
int halt_subchan( REGS* regs, DEVBLK* dev)
{
    UNREFERENCED( regs );

    if (dev->ccwtrace)
    {
        if (sysblk.traceFILE)
            tf_1332( dev );
        else
            // "%1d:%04X CHAN: halt subchannel"
            WRMSG( HHC01332, "I", LCSS_DEVNUM );
    }

    OBTAIN_INTLOCK( regs );
    OBTAIN_DEVLOCK( dev );

#if defined( _FEATURE_IO_ASSIST )
    if (1
        && SIE_MODE(regs)
        && (0
            || regs->siebk->zone != dev->pmcw.zone
            || !(dev->pmcw.flag27 & PMCW27_I)
           )
    )
    {
        RELEASE_DEVLOCK( dev );
        RELEASE_INTLOCK( regs );
        longjmp( regs->progjmp, SIE_INTERCEPT_INST );
    }
#endif

    /* Set condition code 1 if subchannel is status pending alone or
       is status pending with alert, primary, or secondary status
    */
    if (0
        || (dev->scsw.flag3 & SCSW3_SC) == SCSW3_SC_PEND
        || (1
                && (dev->scsw.flag3 & SCSW3_SC_PEND)
                && (dev->scsw.flag3 & (SCSW3_SC_ALERT | SCSW3_SC_PRI | SCSW3_SC_SEC))
           )
    )
    {
        if (dev->ccwtrace)
        {
            if (sysblk.traceFILE)
                tf_1300( dev, 1 );
            else
                // "%1d:%04X CHAN: halt subchannel: cc=%d"
                WRMSG( HHC01300, "I", LCSS_DEVNUM, 1 );
        }
        RELEASE_DEVLOCK( dev );
        RELEASE_INTLOCK( regs );
        return 1;
    }

    /* Set condition code 2 if the halt function or the clear
       function is already in progress at the subchannel
    */
    if (dev->scsw.flag2 & (SCSW2_AC_HALT | SCSW2_AC_CLEAR))
    {
        if (dev->ccwtrace)
        {
            if (sysblk.traceFILE)
                tf_1300( dev, 2 );
            else
                // "%1d:%04X CHAN: halt subchannel: cc=%d"
                WRMSG( HHC01300, "I", LCSS_DEVNUM, 2 );
        }
        RELEASE_DEVLOCK( dev );
        RELEASE_INTLOCK( regs );
        return 2;
    }

    /* Ensure SCSW flags are set from the ORB */
    if (dev->orb.flag4 & ORB4_S)
    {
        dev->scsw.flag0     |= SCSW0_S;
        dev->pciscsw.flag0  |= SCSW0_S;
        dev->attnscsw.flag0 |= SCSW0_S;
    }

    if (dev->orb.flag5 & ORB5_F)
    {
        dev->scsw.flag1     |= SCSW1_F;
        dev->pciscsw.flag1  |= SCSW1_F;
        dev->attnscsw.flag1 |= SCSW1_F;
    }

    if (dev->orb.flag5 & ORB5_P)
    {
        dev->scsw.flag1     |= SCSW1_P;
        dev->pciscsw.flag1  |= SCSW1_P;
        dev->attnscsw.flag1 |= SCSW1_P;
    }

    if (dev->orb.flag5 & ORB5_I)
    {
        dev->scsw.flag1     |= SCSW1_I;
        dev->pciscsw.flag1  |= SCSW1_I;
        dev->attnscsw.flag1 |= SCSW1_I;
    }

    if (dev->orb.flag5 & ORB5_A)
    {
        dev->scsw.flag1     |= SCSW1_A;
        dev->pciscsw.flag1  |= SCSW1_A;
        dev->attnscsw.flag1 |= SCSW1_A;
    }

    if (dev->orb.flag5 & ORB5_U)
    {
        dev->scsw.flag1     |= SCSW1_U;
        dev->pciscsw.flag1  |= SCSW1_U;
        dev->attnscsw.flag1 |= SCSW1_U;
    }

    /* Indicate halt pending to the various SCSWs for device */
    dev->attnscsw.flag2 |= SCSW2_AC_HALT;
    dev->pciscsw.flag2  |= SCSW2_AC_HALT;
    dev->scsw.flag2     |= SCSW2_AC_HALT;

    /* If intermediate status pending with subchannel and device active,
     * reset intermediate status pending (HSCH, fifth paragraph).
     */
    if ((dev->scsw.flag3 & (SCSW3_AC_SCHAC | SCSW3_AC_DEVAC | SCSW3_SC_INTER | SCSW3_SC_PEND))
                ==         (SCSW3_AC_SCHAC | SCSW3_AC_DEVAC | SCSW3_SC_INTER | SCSW3_SC_PEND)
    )
        dev->scsw.flag3 &= ~(SCSW3_SC_INTER | SCSW3_SC_PEND);

    /* If the device is busy then signal the subchannel to halt,
       UNLESS it's suspended (in which case signal it to resume;
       the halt will occur when resumed). If it's startpending,
       remove the I/O from the queue and then halt subhchannel.
       Otherwise (not busy/startpending/suspended) do nothing.
    */
    if (0
#if defined( OPTION_SHARED_DEVICES )
        || (dev->busy && dev->shioactive == DEV_SYS_LOCAL)
#else
        || dev->busy
#endif
        || dev->startpending
        || dev->suspended
    )
    {
        /* Set halt condition and reset pending condition */
        dev->scsw.flag2 |= (SCSW2_FC_HALT | SCSW2_AC_HALT);
        dev->scsw.flag3 &= ~SCSW3_SC_PEND;

        /* Signal the subchannel to resume if it is suspended */
        if (dev->scsw.flag3 & SCSW3_AC_SUSP)
        {
            dev->scsw.flag2 |= SCSW2_AC_RESUM;
            schedule_ioq( NULL, dev );
        }
        else /* Device is busy or startpending, NOT suspended */
        {
            /* Remove the device from the ioq if startpending and queued;
             * lock required before test to keep from entering queue and
             * becoming active prior to queue manipulation.
             */
            OBTAIN_IOQLOCK();
            {
                if (dev->startpending)
                {
                    /* If 1st entry in ioq is this device, remove it */
                    if (sysblk.ioq == dev)
                        sysblk.ioq = dev->nextioq;
                    /* Otherwise search queued I/Os if there are any */
                    else if (sysblk.ioq)
                    {
                        DEVBLK* tmp;
                        /* Look for ioq entry for this device */
                        for (tmp = sysblk.ioq;
                             tmp->nextioq && tmp->nextioq != dev;
                             tmp = tmp->nextioq) { };
                        /* Is an I/O queued for this device? */
                        if (tmp->nextioq == dev)
                        {
                            /* Remove this device's ioq entry */
                            tmp->nextioq = tmp->nextioq->nextioq;
                            sysblk.devtunavail = MAX( 0, sysblk.devtunavail - 1 );
                        }
                    }
                    dev->startpending = 0;
                }
            }
            RELEASE_IOQLOCK();
        }
    }

    /* Halt the device */
    perform_halt_and_release_lock( dev );

    RELEASE_INTLOCK( regs );

    /* Return condition code zero */
    return 0;

} /* end function halt_subchan */


/*-------------------------------------------------------------------*/
/* Reset a device to initialized status                              */
/*                                                                   */
/*   Called by:                                                      */
/*     channelset_reset                                              */
/*     chp_reset                                                     */
/*     io_reset                                                      */
/*                                                                   */
/*   Caller holds `intlock'                                          */
/*-------------------------------------------------------------------*/
static void
device_reset (DEVBLK *dev)
{
    OBTAIN_DEVLOCK( dev );
    {
        if (dev->group)
        {
            DEVGRP* group = dev->group;
            if (group->members != group->acount)
            {
                /* Group is incomplete; ignore */
                RELEASE_DEVLOCK( dev );
                return;
            }
        }

        OBTAIN_IOINTQLK();
        {
            DEQUEUE_IO_INTERRUPT_QLOCKED( &dev->ioint );
            DEQUEUE_IO_INTERRUPT_QLOCKED( &dev->pciioint );
            DEQUEUE_IO_INTERRUPT_QLOCKED( &dev->attnioint );
        }
        RELEASE_IOINTQLK();

        clear_subchannel_busy(dev);

        dev->reserved = dev->pending = dev->pcipending =
        dev->attnpending = dev->startpending = 0;
#if defined( OPTION_SHARED_DEVICES )
        dev->shioactive = DEV_SYS_NONE;
#endif
        if (dev->suspended)
        {
            dev->suspended = 0;
            schedule_ioq(NULL, dev);
        }
#if defined( OPTION_SHARED_DEVICES )
        if (dev->shiowaiters)
            signal_condition (&dev->shiocond);
#endif
        store_fw (dev->pmcw.intparm, 0);
        dev->pmcw.flag4 &= ~PMCW4_ISC;
        dev->pmcw.flag5 &= ~(PMCW5_E | PMCW5_LM | PMCW5_MM | PMCW5_D);
        dev->pmcw.pnom = 0;
        dev->pmcw.lpum = 0;
        store_hw (dev->pmcw.mbi, 0);
        dev->pmcw.flag27 &= ~PMCW27_S;
#if defined( _FEATURE_IO_ASSIST )
        dev->pmcw.zone = 0;
        dev->pmcw.flag25 &= ~PMCW25_VISC;
        dev->pmcw.flag27 &= ~PMCW27_I;
#endif
        memset (&dev->scsw, 0, sizeof(SCSW));
        memset (&dev->pciscsw, 0, sizeof(SCSW));
        memset (&dev->attnscsw, 0, sizeof(SCSW));

        dev->readpending = 0;
        dev->ckdxtdef = 0;
        dev->ckdsetfm = 0;
        dev->ckdlcount = 0;
        dev->ckdssi = 0;
        memset (dev->sense, 0, sizeof(dev->sense));
        dev->sns_pending = 0;
        memset (dev->pgid, 0, sizeof(dev->pgid));
        /* By Adrian - Reset drive password */
        memset (dev->drvpwd, 0, sizeof(dev->drvpwd));

        dev->mainstor = sysblk.mainstor;
        dev->storkeys = sysblk.storkeys;
        dev->mainlim = sysblk.mainsize - 1;

        dev->s370start = 0;

        dev->pending = 0;
        dev->ioint.dev = dev;
        dev->ioint.pending = 1;
        dev->pciioint.dev = dev;
        dev->pciioint.pcipending = 1;
        dev->attnioint.dev = dev;
        dev->attnioint.attnpending = 1;
        dev->tschpending = 0;

#if defined( FEATURE_VM_BLOCKIO )
        if (dev->vmd250env)
        {
           free(dev->vmd250env);
           dev->vmd250env = 0 ;
        }
#endif

        if (dev->hnd && dev->hnd->halt)
            dev->hnd->halt(dev);
    }
    RELEASE_DEVLOCK( dev );
} /* end device_reset() */


/*-------------------------------------------------------------------*/
/* Reset all devices on a particular channelset                      */
/*                                                                   */
/*   Called by:                                                      */
/*     SIGP_IMPL    (control.c)                                      */
/*     SIGP_IPR     (control.c)                                      */
/*-------------------------------------------------------------------*/
void
channelset_reset(REGS *regs)
{
DEVBLK *dev;                            /* -> Device control block   */

    /* Reset each device in the configuration */
    for (dev = sysblk.firstdev; dev != NULL; dev = dev->nextdev)
    {
        /* Skip "devices" that don't actually exist */
        if (!IS_DEV(dev))
            continue;

        if( regs->chanset == dev->chanset)
            device_reset(dev);
    }
} /* end function channelset_reset */


/*-------------------------------------------------------------------*/
/* Reset all devices on a particular chpid                           */
/* Called by io.c 'RHCP' Reset Channel Path instruction.             */
/*                                                                   */
/* FIXME: Console code belongs in the device code.                   */
/*                                                                   */
/*-------------------------------------------------------------------*/
int
chp_reset(BYTE chpid, int solicited)
{
DEVBLK *dev;                            /* -> Device control block   */
int i;
int reset = 0;

    /* Reset each device in the configuration with this chpid */
    for (reset=0, dev = sysblk.firstdev; dev != NULL; dev = dev->nextdev)
    {
        /* Skip "devices" that don't actually exist */
        if (!IS_DEV(dev))
            continue;

        for (i=0; i < 8; i++)
        {
            if((chpid == dev->pmcw.chpid[i])
              && (dev->pmcw.pim & dev->pmcw.pam & dev->pmcw.pom & (0x80 >> i)) )
            {
                reset = 1;
                device_reset(dev);
            }
        }
    }

    /* Indicate channel path reset completed */
    build_chp_reset_chrpt( chpid, solicited, reset );
    return 0;

} /* end function chp_reset */


/*-------------------------------------------------------------------*/
/* I/O RESET  --  handle the "Channel Subsystem Reset" signal        */
/* Resets status of all devices ready for IPL.  Note that device     */
/* positioning is not affected by I/O reset; thus the system can     */
/* be IPLed from current position in a tape or card reader file.     */
/*                                                                   */
/* Caller holds `intlock'                                            */
/*                                                                   */
/* FIXME: Console references belong in the console device driver     */
/*        code!                                                      */
/*-------------------------------------------------------------------*/
void
io_reset (void)
{
DEVBLK *dev;                            /* -> Device control block   */
int i;

    /* Reset channel subsystem back to default initial non-MSS state */
    sysblk.mss = FALSE;                 /* (not enabled by default)  */
    sysblk.lcssmax = 0;                 /* (default to single lcss)  */

    /* reset sclp interface */
    sclp_reset();

    /* Connect each channel set to its home cpu */
    for (i = 0; i < sysblk.maxcpu; i++)
        if (IS_CPU_ONLINE(i))
            sysblk.regs[i]->chanset = i < FEATURE_LCSS_MAX ? i : 0xFFFF;

    /* Reset each device in the configuration */
    for (dev = sysblk.firstdev; dev != NULL; dev = dev->nextdev)
    {
        /* Skip "devices" that don't actually exist */
        if (!IS_DEV(dev))
            continue;

        device_reset(dev);
    }

    /* No crws pending anymore */
    OBTAIN_CRWLOCK();
    sysblk.crwcount = 0;
    sysblk.crwindex = 0;
    RELEASE_CRWLOCK();
    OFF_IC_CHANRPT;

} /* end function io_reset */


/*-------------------------------------------------------------------*/
/* Create a device thread                                            */
/*-------------------------------------------------------------------*/
/*                                                                   */
/* Locks:                                                            */
/*                                                                   */
/* sysblk->ioqlock must be held.                                     */
/*                                                                   */
/*-------------------------------------------------------------------*/
static int
create_device_thread ()
{
int     rc;                             /* Return code               */
TID     tid;                            /* Thread ID                 */

    /* Ensure correct number of ioq entries tracked */
    {
        register int i;
        register DEVBLK  *ioq;

        for (ioq = sysblk.ioq, i = 0;
             ioq;
             ioq = ioq->nextioq, i++);
        sysblk.devtunavail = i;

        /* If no additional work, return */
        if (!i)
            return 0;

    }

    /* If work is waiting and permitted, schedule another device     */
    /* thread to handle                                              */
    if ((sysblk.devtunavail > sysblk.devtwait &&
         (sysblk.devtmax == 0 || sysblk.devtnbr < sysblk.devtmax)) ||
        sysblk.devtmax < 0)
    {
        rc = create_thread (&tid, DETACHED, device_thread, NULL,
                            "idle device thread");
        if (rc)
        {
            WRMSG (HHC00102, "E", strerror(rc));
            return 2;
        }

        /* Update counters */
        sysblk.devtnbr++;
        sysblk.devtwait++;
        if (sysblk.devtnbr > sysblk.devthwm)
            sysblk.devthwm = sysblk.devtnbr;
    }

    /* Signal possible waiting I/O threads */
    signal_condition(&sysblk.ioqcond);

    return 0;
}


/*-------------------------------------------------------------------*/
/* Execute a queued I/O                                              */
/*-------------------------------------------------------------------*/
DLL_EXPORT void* device_thread( void* arg )
{
DEVBLK* dev;
int     current_priority;               /* Current thread priority   */
int     rc = 0;                         /* Return code               */
u_int   waitcount = 0;                  /* Wait counter              */

    UNREFERENCED( arg );

    /* Automatically adjust to priority change if needed */

    current_priority = get_thread_priority();

    if (current_priority != sysblk.devprio)
    {
        SET_THREAD_PRIORITY( sysblk.devprio, sysblk.qos_user_initiated );
        current_priority = sysblk.devprio;
    }

    OBTAIN_IOQLOCK();
    {
        sysblk.devtwait = MAX( 0, sysblk.devtwait - 1 );

        while (1)
        {
            while ((dev=sysblk.ioq)  && !sysblk.shutdown)
            {
                /* Reset local wait count */
                waitcount = 0;

                /* Set next IOQ entry and zero pointer in current DEVBLK */
                sysblk.ioq = dev->nextioq;
                dev->nextioq = 0;

                /* Decrement waiting IOQ count */
                sysblk.devtunavail = MAX( 0, sysblk.devtunavail - 1 );

                /* Create another device thread if pending work */
                create_device_thread();

                /* Set thread id */
                dev->tid = thread_id();

                /* Set thread name */
                {
                    char thread_name[16];
                    MSGBUF( thread_name, "dev %4.4X thrd", dev->devnum );
                    SET_THREAD_NAME( thread_name );
                }

                RELEASE_IOQLOCK();
                {
                    /* Set priority to requested device priority; should not */
                    /* have any Hercules locks held                          */
                    if (dev->devprio != current_priority)
                    {
                        SET_THREAD_PRIORITY( dev->devprio, sysblk.qos_user_initiated );
                        current_priority = dev->devprio;
                    }

                    /* Execute requested CCW chain */
                    call_execute_ccw_chain( sysblk.arch_mode, dev );

                    /* Reset priority back to device default priority */
                    if (current_priority != sysblk.devprio)
                    {
                        SET_THREAD_PRIORITY( sysblk.devprio, sysblk.qos_user_initiated );
                        current_priority = sysblk.devprio;
                    }
                }
                OBTAIN_IOQLOCK();

                dev->tid = 0;
            }
            // end while ((dev=sysblk.ioq)  && !sysblk.shutdown)

            /* Shutdown thread on request, if idle for more than two     */
            /* seconds, or more than four idle threads                   */
            if (0
                || (1
                    && sysblk.devtmax == 0
                    && waitcount >= 20
                    && sysblk.devtwait > 3
                   )
                || (sysblk.devtmax > 0 && sysblk.devtnbr > sysblk.devtmax)
                ||  sysblk.devtmax < 0
                ||  sysblk.shutdown
            )
                break;

            /* Show thread as idle */
            waitcount++;
            sysblk.devtwait++;
            SET_THREAD_NAME( "idle dev thrd" );

            /* Wait for work to arrive */
            rc = timed_wait_condition_relative_usecs
                 (
                     &sysblk.ioqcond,
                     &sysblk.ioqlock,
                     100000, // 100 ms
                     NULL
                 );

            /* Decrement thread waiting count */
            sysblk.devtwait = MAX( 0, sysblk.devtwait - 1 );

            /* If shutdown requested, terminate the thread
               after signaling next I/O thread to shutdown.
            */
            if (sysblk.shutdown)
            {
                signal_condition( &sysblk.ioqcond );
                break;
            }
        }
        // end while (1)

        /* Decrement total number of device threads */
        sysblk.devtnbr = MAX( 0, sysblk.devtnbr - 1 );
    }
    RELEASE_IOQLOCK();

    return ( NULL );

} /* end function device_thread */


/*-------------------------------------------------------------------*/
/* Schedule I/O Request (second half of Schedule IOQ)                */
/*-------------------------------------------------------------------*/
/*                                                                   */
/* Note: Queue is actually split, by priority, with resume requests  */
/*       first for each priority, followed by start requests for     */
/*       the priority.  The code within the locked section MUST be   */
/*       minimized.                                                  */
/*                                                                   */
/* Locks held:                                                       */
/*   dev->lock                                                       */
/*                                                                   */
/* Locks used:                                                       */
/*   sysblk.ioqlock                                                  */
/*                                                                   */
/*  Returns:                                                         */
/*                                                                   */
/*  0 - Success                                                      */
/*  2 - Unable to schedule channel thread (this RC belongs in IOP)   */
/*                                                                   */
/*-------------------------------------------------------------------*/
static int
ScheduleIORequest ( DEVBLK *dev )
{
    DEVBLK *ioq, *previoq, *nextioq;    /* Device I/O queue pointers */
    int     count;                      /* I/O queue length          */
    int     rc = 0;                     /* Return Code               */
    U8      device_resume;              /* Resume I/O flag - Device  */

    /* Determine if the device is resuming */
    device_resume = (dev->scsw.flag2 & SCSW2_AC_RESUM);

    OBTAIN_IOQLOCK();
    {
        /* Insert this I/O request into the appropriate I/O queue slot */
        for (ioq = sysblk.ioq, previoq = NULL, count = 0;
            ioq;
            ++count, previoq = ioq, ioq = ioq->nextioq)
        {
            /* If DEVBLK already in queue, fail queueing of DEVBLK */
            if (ioq == dev)
            {
                rc = 2;
                BREAK_INTO_DEBUGGER();
                break;
            }

            /* 1. Look for priority partition. */
            if (dev->priority > ioq->priority)
                break;
            if (dev->priority < ioq->priority)
                continue;

            /* 2. Resumes precede Start I/Os in each priority partition */
            if (device_resume > (ioq->scsw.flag2 & SCSW2_AC_RESUM))
                break;
        }

        if (rc == 0)
        {
            /* Save next I/O queue position */
            nextioq = ioq;

            /* Validate and complete sanity count of I/O queue entries */
            for (++count;
                 ioq;
                 ++count, ioq = ioq->nextioq)
            {
                /* If DEVBLK already in queue, fail queueing of DEVBLK */
                if (ioq == dev)
                {
                    rc = 2;
                    BREAK_INTO_DEBUGGER();
                    break;
                }
            }

            /* I/O queue has been validated, insert DEVBLK into I/O queue.
             */
            if (rc == 0)
            {
                /* Chain our request ahead of this one */
                dev->nextioq = nextioq;

                /* Chain previous one (if any) to ours */
                if (previoq != NULL)
                    previoq->nextioq = dev;
                else
                    sysblk.ioq = dev;

                /* Update device thread unavailable count. It will be
                 * decremented once a thread grabs this request.
                 */
                sysblk.devtunavail = count;

                /* Create another device thread, if needed, to service this
                 * I/O
                 */
                rc = create_device_thread();
            }
        }
    }
    RELEASE_IOQLOCK();

    /* Return condition code */
    return rc;
}


/*-------------------------------------------------------------------*/
/*  SCHEDULE IOQ                                                     */
/*-------------------------------------------------------------------*/
/*                                                                   */
/*  Schedule I/O work for an IOP to dispatch to channel              */
/*                                                                   */
/*                                                                   */
/*  Format:                                                          */
/*                                                                   */
/*    schedule_ioq (regs, dev)                                       */
/*                                                                   */
/*                                                                   */
/*  Entry Conditions:                                                */
/*                                                                   */
/*    dev->lock     must be held.                                    */
/*                                                                   */
/*                                                                   */
/*  Locks Used:                                                      */
/*                                                                   */
/*    None                                                           */
/*                                                                   */
/*                                                                   */
/*  Returns:                                                         */
/*                                                                   */
/*  0 - Success                                                      */
/*  2 - Unable to schedule channel thread (this RC belongs in IOP)   */
/*                                                                   */
/*-------------------------------------------------------------------*/

static int
schedule_ioq (const REGS *regs, DEVBLK *dev)
{
    int result = 2;                     /* 0=Thread scheduled        */
                                        /* 2=Unable to schedule      */

    /* If shutdown in progress, don't schedule or perform I/O
     * operation.
     */
    if (sysblk.shutdown)
    {
        signal_condition(&sysblk.ioqcond);
        return (result);
    }

    if (dev->s370start && regs != NULL)
    {
        RELEASE_DEVLOCK( dev );
        {
            call_execute_ccw_chain( sysblk.arch_mode, dev );
        }
        OBTAIN_DEVLOCK( dev );
        result = 0;
    }
    else
        result = ScheduleIORequest(dev);

    return (result);
}


/*-------------------------------------------------------------------*/
/* RESUME SUBCHANNEL                                                 */
/*-------------------------------------------------------------------*/
/* Input                                                             */
/*      regs    -> CPU register context                              */
/*      dev     -> Device control block                              */
/* Return value                                                      */
/*      The return value is the condition code for the RSCH          */
/*      instruction:  0=subchannel has been made resume pending,     */
/*      1=status was pending, 2=resume not allowed                   */
/*-------------------------------------------------------------------*/
int
resume_subchan (REGS *regs, DEVBLK *dev)
{
int cc;                                 /* Return code               */

    OBTAIN_DEVLOCK( dev );
    {
#if defined( _FEATURE_IO_ASSIST )
        if(SIE_MODE(regs)
          && (regs->siebk->zone != dev->pmcw.zone
            || !(dev->pmcw.flag27 & PMCW27_I)))
        {
            RELEASE_DEVLOCK( dev );
            longjmp(regs->progjmp,SIE_INTERCEPT_INST);
        }
#endif
        /* Set condition code 1 if subchannel has status pending
         *
         * SA22-7832-09, RESUME SUBCHANNEL, Condition Code 1, p. 14-11
         */
        if (dev->scsw.flag3     & SCSW3_SC_PEND ||
            dev->pciscsw.flag3  & SCSW3_SC_PEND ||
            dev->attnscsw.flag3 & SCSW3_SC_PEND)
        {
            cc = 1;
        }

        /* Set condition code 2 if subchannel has any function active, is
         * already resume pending, or the ORB for the SSCH did not specify
         * suspend control.
         *
         * SA22-7832-09, RESUME SUBCHANNEL, Condition Code 2, p. 14-11
         */
        else if (unlikely(!(dev->orb.flag4  & ORB4_S)           ||
                          !(dev->scsw.flag2 & SCSW2_FC)         ||
                          !(dev->scsw.flag3 & SCSW3_AC_SUSP)    ||
                          (dev->scsw.flag2 &
                           (SCSW2_FC_HALT  | SCSW2_FC_CLEAR |
                            SCSW2_AC_RESUM | SCSW2_AC_START |
                            SCSW2_AC_HALT  | SCSW2_AC_CLEAR))   ||
                          (dev->scsw.flag3 &
                           (SCSW3_AC_SCHAC | SCSW3_AC_DEVAC))))
        {
            cc = 2;
        }

        /* Otherwise, schedule the resume request */
        else
        {
            /* Clear the path not-operational mask if in suspend state */
            if (dev->scsw.flag3 & SCSW3_AC_SUSP)
                dev->pmcw.pnom = 0x00;

            /* Set the resume pending flag and signal the subchannel */
            dev->scsw.flag2 |= SCSW2_AC_RESUM;
            cc = schedule_ioq(NULL, dev);
        }

        /* If tracing, write trace message */
        if (dev->ccwtrace)
        {
            if (sysblk.traceFILE)
                tf_1333( dev, cc );
            else
                // "%1d:%04X CHAN: resume subchannel: cc=%d"
                WRMSG( HHC01333, "I", LCSS_DEVNUM, cc );
        }
    }
    RELEASE_DEVLOCK( dev );

    /* Return the condition code */
    return cc;

} /* end function resume_subchan */


#if defined( OPTION_SHARED_DEVICES )
/*-------------------------------------------------------------------
 * Wait for the device to become available
 *-------------------------------------------------------------------*/
void shared_iowait (DEVBLK *dev)
{
    while (dev->shioactive != DEV_SYS_NONE
        && dev->shioactive != DEV_SYS_LOCAL)
    {
        dev->shiowaiters++;
        wait_condition(&dev->shiocond, &dev->lock);
        dev->shiowaiters = MAX(0, dev->shiowaiters - 1);
    }
}
#endif // defined( OPTION_SHARED_DEVICES )
#endif /*!defined(_CHANNEL_C)*/


/*-------------------------------------------------------------------*/
/* RAISE A PCI INTERRUPT                                             */
/* This function is called during execution of a channel program     */
/* whenever a CCW is fetched which has the CCW_FLAGS_PCI flag set    */
/*                                                                   */
/* Note: PCI interrupts are incorrectly presented as a separate      */
/*       interrupt. The PCI interrupt should be ORed in to the       */
/*       next status read.                                           */
/*                                                                   */
/*-------------------------------------------------------------------*/
/* Input                                                             */
/*      dev     -> Device control block                              */
/*      ccwkey  =  Key in which channel program is executing         */
/*      ccwfmt  =  CCW format (0 or 1)                               */
/*      ccwaddr =  Address of next CCW                               */
/* Output                                                            */
/*      The PCI CSW or SCSW is saved in the device block, and the    */
/*      pcipending flag is set, and an I/O interrupt is scheduled.   */
/*-------------------------------------------------------------------*/
static void
ARCH_DEP(raise_pci) (DEVBLK *dev,       /* -> Device block           */
                     BYTE ccwkey,       /* Bits 0-3=key, 4-7=zeroes  */
                     BYTE ccwfmt,       /* CCW format (0 or 1)       */
                     U32 ccwaddr)       /* Main storage addr of CCW  */
{
#if !defined(FEATURE_CHANNEL_SUBSYSTEM)
    UNREFERENCED(ccwfmt);
#endif

    IODELAY( dev );

    OBTAIN_INTLOCK( NULL );
    {
        OBTAIN_DEVLOCK( dev );
        {
            /* Save the PCI SCSW replacing any previous pending PCI; always
             * track the channel in channel subsystem mode
             */
            dev->pciscsw.flag0 = ccwkey & SCSW0_KEY;
            dev->pciscsw.flag1 = (ccwfmt == 1 ? SCSW1_F : 0);
            dev->pciscsw.flag2 = SCSW2_FC_START;
            dev->pciscsw.flag3 = SCSW3_AC_SCHAC | SCSW3_AC_DEVAC
                               | SCSW3_SC_INTER | SCSW3_SC_PEND;
            STORE_FW(dev->pciscsw.ccwaddr,ccwaddr);
            dev->pciscsw.unitstat = 0;
            dev->pciscsw.chanstat = CSW_PCI;
            store_hw (dev->pciscsw.count, 0);

            /* Queue the PCI pending interrupt */
            OBTAIN_IOINTQLK();
            {
                QUEUE_IO_INTERRUPT_QLOCKED( &dev->pciioint, FALSE );

                /* Update interrupt status */
                subchannel_interrupt_queue_cleanup( dev );
                UPDATE_IC_IOPENDING_QLOCKED();
            }
            RELEASE_IOINTQLK();
        }
        RELEASE_DEVLOCK( dev );
    }
    RELEASE_INTLOCK( NULL );

} /* end function raise_pci */


/*-------------------------------------------------------------------*/
/* FETCH A CHANNEL COMMAND WORD FROM MAIN STORAGE                    */
/*-------------------------------------------------------------------*/
static INLINE void
ARCH_DEP(fetch_ccw) (DEVBLK *dev,       /* -> Device block           */
                     BYTE ccwkey,       /* Bits 0-3=key, 4-7=zeroes  */
                     BYTE ccwfmt,       /* CCW format (0 or 1)       */
                     U32 ccwaddr,       /* Main storage addr of CCW  */
                     BYTE *code,        /* Returned operation code   */
                     U32 *addr,         /* Returned data address     */
                     BYTE *flags,       /* Returned flags            */
                     U32 *count,        /* Returned data count       */
                     BYTE *chanstat)    /* Returned channel status   */
{
BYTE    storkey;                        /* Storage key               */
BYTE   *ccw;                            /* CCW pointer               */

    UNREFERENCED_370(dev);
    *code=0;
    *count=0;
    *flags=0;
    *addr=0;

    /* Channel program check if CCW is not on a doubleword
       boundary or is outside limit of main storage */
    if ( (ccwaddr & 0x00000007) || CHADDRCHK(ccwaddr, dev) )
    {
        *chanstat = CSW_PROGC;
        return;
    }

    /* Channel protection check if CCW is fetch protected */
    storkey = ARCH_DEP( get_dev_storage_key )( dev, ccwaddr );
    if (ccwkey != 0 && (storkey & STORKEY_FETCH)
        && (storkey & STORKEY_KEY) != ccwkey)
    {
        *chanstat = CSW_PROTC;
        return;
    }

    /* Set the main storage reference bit for the CCW location */
    ARCH_DEP( or_dev_storage_key )( dev, ccwaddr, STORKEY_REF );

    /* Point to the CCW in main storage */
    ccw = dev->mainstor + ccwaddr;

    /* Extract CCW opcode, flags, byte count, and data address */
    if (ccwfmt == 0)
    {
        *code = ccw[0];
        *addr = ((U32)(ccw[1]) << 16) | ((U32)(ccw[2]) << 8)
                    | ccw[3];
        *flags = ccw[4];
        *count = ((U16)(ccw[6]) << 8) | ccw[7];
    }
    else
    {
        *code = ccw[0];
        *flags = ccw[1];
        *count = ((U16)(ccw[2]) << 8) | ccw[3];
        *addr = ((U32)(ccw[4]) << 24) | ((U32)(ccw[5]) << 16)
                    | ((U32)(ccw[6]) << 8) | ccw[7];
    }
} /* end function fetch_ccw */


/*-------------------------------------------------------------------*/
/* FETCH AN INDIRECT DATA ADDRESS WORD FROM MAIN STORAGE             */
/*-------------------------------------------------------------------*/
static INLINE void
ARCH_DEP(fetch_idaw) (DEVBLK *dev,      /* -> Device block           */
                      BYTE code,        /* CCW operation code        */
                      BYTE ccwkey,      /* Bits 0-3=key, 4-7=zeroes  */
                      BYTE idawfmt,     /* IDAW format (1 or 2)      */
                      U16 idapmask,     /* IDA page size - 1         */
                      int idaseq,       /* 0=1st IDAW                */
                      U32 idawaddr,     /* Main storage addr of IDAW */
                      RADR *addr,       /* Returned IDAW content     */
                      U16 *len,         /* Returned IDA data length  */
                      BYTE *chanstat)   /* Returned channel status   */
{
RADR    idaw;                           /* Contents of IDAW          */
U32     idaw1;                          /* Format-1 IDAW             */
U64     idaw2;                          /* Format-2 IDAW             */
RADR    idapage;                        /* Addr of next IDA page     */
U16     idalen;                         /* #of bytes until next page */
BYTE    storkey;                        /* Storage key               */

    UNREFERENCED_370(dev);
    *addr = 0;
    *len = 0;

    /* Channel program check if IDAW is not on correct
       boundary or is outside limit of main storage */
    if ((idawaddr & ((idawfmt == PF_IDAW2) ? 0x07 : 0x03))
        || CHADDRCHK(idawaddr, dev)
        /* Program check if Format-0 CCW and IDAW address > 16M      */
        /* SA22-7201-05:                                             */
        /*  p. 16-25, Invalid IDAW Address                           */
        || (!(dev->orb.flag5 & ORB5_F) && (idawaddr & 0xFF000000)))
    {
        *chanstat = CSW_PROGC;
        return;
    }

    /* Channel protection check if IDAW is fetch protected */
    storkey = ARCH_DEP( get_dev_storage_key )( dev, idawaddr );
    if (ccwkey != 0 && (storkey & STORKEY_FETCH)
        && (storkey & STORKEY_KEY) != ccwkey)
    {
        *chanstat = CSW_PROTC;
        return;
    }

    /* Set the main storage reference bit for the IDAW location */
    ARCH_DEP( or_dev_storage_key )( dev, idawaddr, STORKEY_REF );

    /* Fetch IDAW from main storage */
    if (idawfmt == PF_IDAW2)
    {
        /* Fetch format-2 IDAW */
        FETCH_DW(idaw2, dev->mainstor + idawaddr);

#if !defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
        /* Channel program check in ESA/390 mode
           if the format-2 IDAW exceeds 2GB-1 */
        if (idaw2 > 0x7FFFFFFF)
        {
            *chanstat = CSW_PROGC;
            return;
        }
#endif
        /* Save contents of format-2 IDAW */
        idaw = idaw2;
    }
    else
    {
        /* Fetch format-1 IDAW */
        FETCH_FW(idaw1, dev->mainstor + idawaddr);

        /* Channel program check if bit 0 of
           the format-1 IDAW is not zero */
        if (idaw1 & 0x80000000)
        {
            *chanstat = CSW_PROGC;
            return;
        }

        /* Save contents of format-1 IDAW */
        idaw = idaw1;
    }

    /* Channel program check if IDAW data
       location is outside main storage */
    if ( CHADDRCHK(idaw, dev) )
    {
        *chanstat = CSW_PROGC;
        return;
    }

    /* Channel program check if IDAW data location is not
       on a page boundary, except for the first IDAW */
    if (IS_CCW_RDBACK (code))
    {
        if (idaseq > 0 && ((idaw+1) & idapmask) != 0)
        {
            *chanstat = CSW_PROGC;
            return;
        }

        /* Calculate address of next page boundary */
        idapage = (idaw & ~idapmask);
        idalen = (idaw - idapage) + 1;

        /* Return the address and length for this IDAW */
        *addr = idaw;
        *len = idalen;
    }
    else
    {
        if (idaseq > 0 && (idaw & idapmask) != 0)
        {
            *chanstat = CSW_PROGC;
            return;
        }

        /* Calculate address of next page boundary */
        idapage = (idaw + idapmask + 1) & ~idapmask;
        idalen = idapage - idaw;

        /* Return the address and length for this IDAW */
        *addr = idaw;
        *len = idalen;
    }

} /* end function fetch_idaw */


#if defined(FEATURE_MIDAW_FACILITY)
/*-------------------------------------------------------------------*/
/* FETCH A MODIFIED INDIRECT DATA ADDRESS WORD FROM MAIN STORAGE     */
/*-------------------------------------------------------------------*/
static INLINE void
ARCH_DEP(fetch_midaw) (DEVBLK *dev,     /* -> Device block           */
                       BYTE code,       /* CCW operation code        */
                       BYTE ccwkey,     /* Bits 0-3=key, 4-7=zeroes  */
                       int midawseq,    /* 0=1st MIDAW               */
                       U32 midawadr,    /* Main storage addr of MIDAW*/
                       RADR *addr,      /* Returned MIDAW content    */
                       U16 *len,        /* Returned MIDAW data length*/
                       BYTE *flags,     /* Returned MIDAW flags      */
                       BYTE *chanstat)  /* Returned channel status   */
{
U64     mword1, mword2;                 /* MIDAW high and low words  */
RADR    mdaddr;                         /* Data address from MIDAW   */
U16     mcount;                         /* Count field from MIDAW    */
BYTE    mflags;                         /* Flags byte from MIDAW     */
BYTE    storkey;                        /* Storage key               */
U16     maxlen;                         /* Maximum allowable length  */

    UNREFERENCED_370(dev);

    /* Channel program check if MIDAW is not on quadword
       boundary or is outside limit of main storage */
    if ((midawadr & 0x0F)
        || CHADDRCHK(midawadr, dev) )
    {
        *chanstat = CSW_PROGC;
        return;
    }

    /* Channel program check if MIDAW list crosses a page boundary */
    if (midawseq > 0 && (midawadr & PAGEFRAME_BYTEMASK) == 0)
    {
        *chanstat = CSW_PROGC;
        return;
    }

    /* Channel protection check if MIDAW is fetch protected */
    storkey = ARCH_DEP( get_dev_storage_key )( dev, midawadr );
    if (ccwkey != 0 && (storkey & STORKEY_FETCH)
        && (storkey & STORKEY_KEY) != ccwkey)
    {
        *chanstat = CSW_PROTC;
        return;
    }

    /* Set the main storage reference bit for the MIDAW location */
    ARCH_DEP( or_dev_storage_key )( dev, midawadr, STORKEY_REF );

    /* Fetch MIDAW from main storage (MIDAW is quadword
       aligned and so cannot cross a page boundary) */
    FETCH_DW(mword1, dev->mainstor + midawadr);
    FETCH_DW(mword2, dev->mainstor + midawadr + 8);

    /* Channel program check in reserved bits are non-zero */
    if (mword1 & 0xFFFFFFFFFF000000ULL)
    {
        *chanstat = CSW_PROGC;
        return;
    }

    /* Extract fields from MIDAW */
    mflags = mword1 >> 16;
    mcount = mword1 & 0xFFFF;
    mdaddr = (RADR)mword2;

    /* Channel program check if data transfer interrupt flag is set */
    if (mflags & MIDAW_DTI)
    {
        *chanstat = CSW_PROGC;
        return;
    }

    /* Channel program check if MIDAW count is zero */
    if (mcount == 0)
    {
        *chanstat = CSW_PROGC;
        return;
    }

    /* Channel program check if MIDAW data
       location is outside main storage */
    if ( CHADDRCHK(mdaddr, dev) )
    {
        *chanstat = CSW_PROGC;
        return;
    }

    /* Channel program check if skipping not in effect
       and the MIDAW data area crosses a page boundary */
    maxlen = (IS_CCW_RDBACK(code)) ?
                mdaddr - (mdaddr & PAGEFRAME_PAGEMASK) + 1 :
                (mdaddr | PAGEFRAME_BYTEMASK) - mdaddr + 1 ;

    if ((mflags & MIDAW_SKIP) == 0 && mcount > maxlen)
    {
        *chanstat = CSW_PROGC;
        return;
    }

    /* Return the data address, length, flags for this MIDAW */
    *addr = mdaddr;
    *len = mcount;
    *flags = mflags;

} /* end function fetch_midaw */
#endif /*defined(FEATURE_MIDAW_FACILITY)*/


/*-------------------------------------------------------------------*/
/* COPY DATA BETWEEN CHANNEL I/O BUFFER AND MAIN STORAGE             */
/*-------------------------------------------------------------------*/
static void
ARCH_DEP(copy_iobuf) (bool* did_ccw_trace,
                      DEVBLK *dev,      /* -> Device block           */
                      BYTE ccw[],       /* CCW                       */
                      BYTE code,        /* CCW operation code        */
                      BYTE flags,       /* CCW flags                 */
                      U32 addr,         /* Data address              */
                      U32 count,        /* Data count                */
                      BYTE ccwkey,      /* Protection key            */
                      BYTE idawfmt,     /* IDAW format (1 or 2)      */
                      U16 idapmask,     /* IDA page size - 1         */
                      BYTE *iobuf,      /* -> Channel I/O buffer     */
                      BYTE *iobufstart, /* -> First byte of buffer   */
                      BYTE *iobufend,   /* -> Last byte of buffer    */
                      BYTE *chanstat,   /* Returned channel status   */
                      U32 *residual,    /* Residual data count       */
                      PREFETCH *prefetch)  /* Prefetch CCW buffer    */
{
BYTE    *iobufptr = 0;                  /* Working I/O buffer addr   */
u_int   ps = 0;                         /* Prefetch entry            */
U32     idawaddr;                       /* Main storage addr of IDAW */
U16     idacount;                       /* IDA bytes remaining       */
int     idaseq;                         /* IDA sequence number       */
RADR    idadata;                        /* IDA data address          */
U16     idalen;                         /* IDA data length           */
int     idasize;                        /* IDAW Size                 */
BYTE    storkey;                        /* Storage key               */
RADR    page,startpage,endpage;         /* Storage key pages         */
BYTE    to_iobuf;                       /* 1=READ, SENSE, or RDBACK  */
BYTE    to_memory;                      /* 1=READ, SENSE, or RDBACK  */
BYTE    readbackwards;                  /* 1=RDBACK                  */
#if defined(FEATURE_MIDAW_FACILITY)
int     midawseq;                       /* MIDAW counter (0=1st)     */
U32     midawptr;                       /* Real addr of MIDAW        */
U16     midawrem;                       /* CCW bytes remaining       */
U16     midawlen=0;                     /* MIDAW data length         */
RADR    midawdat=0;                     /* MIDAW data area addr      */
BYTE    midawflg;                       /* MIDAW flags               */
#endif /*defined(FEATURE_MIDAW_FACILITY)*/

#if !defined(set_chanstat)
#define set_chanstat(_status)                                          \
do {                                                                   \
    if (prefetch->seq)                                                 \
        prefetch->chanstat[ps] = (_status);                            \
    else                                                               \
        *chanstat = (_status);                                         \
} while(0)
#endif

#if !defined(get_new_prefetch_entry)
#define get_new_prefetch_entry(_idawtype,_idawaddr)                    \
do {                                                                   \
    if (prefetch->seq)                                                 \
    {                                                                  \
        ps = prefetch->seq++;                                          \
        if (prefetch->seq > PF_SIZE)                                   \
        {                                                              \
            *chanstat = CSW_CDC;                                       \
            break;                                                     \
        }                                                              \
        prefetch->ccwaddr[ps] = prefetch->ccwaddr[ps-1];               \
        if ((_idawtype) != PF_NO_IDAW)                                 \
        {                                                              \
            prefetch->idawtype[ps] = (_idawtype);                      \
            prefetch->idawaddr[ps] = (_idawaddr);                      \
        }                                                              \
    }                                                                  \
    *chanstat = 0;                                                     \
} while(0)
#endif

    /* Set current prefetch sequence */
    if (prefetch->seq)
        ps = prefetch->seq - 1;

    /* Channel Data Check if invalid I/O buffer */
    if ((size_t)iobufend < 131072   ||  /* Low host OS storage reference */
        iobuf < iobufstart          ||
        iobuf > iobufend)
    {
        set_chanstat(CSW_CDC);
        return;
    }

    /* Exit if no bytes are to be copied */
    if (count == 0 || dev->is_immed)
    {
        set_chanstat(0);
        return;
    }

    /* Set flags indicating direction of data movement */
    if (IS_CCW_RDBACK(code))
    {
        readbackwards = to_memory = 1;
        to_iobuf = 0;
    }
    else
    {
        readbackwards = 0;
        to_iobuf = code & 0x01;
        to_memory = !to_iobuf;
    }


#if defined(FEATURE_MIDAW_FACILITY)
    /* Move data when modified indirect data addressing is used */
    if (flags & CCW_FLAGS_MIDAW)
    {
        if (prefetch->seq)
            prefetch->datalen[ps] = 0;

        midawptr = addr;
        midawrem = count;
        midawflg = 0;

        for (midawseq = 0;
             midawrem > 0 &&
                (midawflg & MIDAW_LAST) == 0 &&
                chanstat != 0 &&
                !(prefetch->seq &&
                    (prefetch->chanstat[ps] ||
                        ((ps+1) < prefetch->seq &&
                            prefetch->chanstat[ps+1])));
             midawseq++)
        {
            /* Get new prefetch entry */
            get_new_prefetch_entry(PF_MIDAW, midawptr);
            if (*chanstat != 0)
                break;

            /* Fetch MIDAW and set data address, length, flags */
            ARCH_DEP(fetch_midaw) (dev, code, ccwkey,
                    midawseq, midawptr,
                    &midawdat, &midawlen, &midawflg, chanstat);

            /* Exit if fetch_midaw detected channel program check */
            if (prefetch->seq)
            {
                prefetch->idawflag[ps] = midawflg;
                prefetch->dataaddr[ps] = midawdat;
                prefetch->datalen[ps] = midawlen;
                if (*chanstat != 0)
                {
                    prefetch->chanstat[ps] = *chanstat;
                    *chanstat = 0;
                    break;
                }
            }
            else if (*chanstat != 0)
                break;

            /* Channel program check if MIDAW length
               exceeds the remaining CCW count */
            if (midawlen > midawrem)
            {
                set_chanstat(CSW_PROGC);
                return;
            }

            /* MIDAW length may be zero during prefetch operations */
            if (!prefetch->seq || (prefetch->seq && midawlen))
            {

                /* Perform data movement unless SKIP flag is set in
                   MIDAW */
                if ((midawflg & MIDAW_SKIP) ==0)
                {
                    /* Note: MIDAW data area cannot cross a page
                       boundary. The fetch_midaw function enforces this
                       restriction */

                    /* Channel protection check if MIDAW data location
                       is fetch protected, or if location is store
                       protected and command is READ, READ BACKWARD, or
                       SENSE */
                    storkey = ARCH_DEP( get_dev_storage_key )( dev, midawdat );
                    if (ccwkey != 0
                        && (storkey & STORKEY_KEY) != ccwkey
                        && ((storkey & STORKEY_FETCH) || to_memory))
                    {
                        set_chanstat(CSW_PROTC);
                        return;
                    }

                    /* Ensure memcpy will stay within buffer         */
                    /* Channel data check if outside buffer          */
                    /* SA22-7201-05:                                 */
                    /*  p. 16-27, Channel-Data Check                 */
                    if (readbackwards)
                    {
                        iobufptr = iobuf + dev->curblkrem + midawrem -
                                   midawlen;
                        if (!midawlen                        ||
                            (iobufptr + midawlen) > iobufend ||
                            iobufptr < iobufstart)
                        {
                            *chanstat = CSW_CDC;
                            return;
                        }
                    }
                    else if (!midawlen                     ||
                             (iobuf + midawlen) > iobufend ||
                             iobuf < iobufstart)
                    {
                            set_chanstat(CSW_CDC);
                            return;
                    }

                    /* Set the main storage reference and change
                       bits */
                    if (to_memory)
                        ARCH_DEP( or_dev_storage_key )( dev, midawdat, (STORKEY_REF | STORKEY_CHANGE) );
                    else
                        ARCH_DEP( or_dev_storage_key )( dev, midawdat, STORKEY_REF );

                    /* Copy data between main storage and channel
                       buffer */
                    if (readbackwards)
                    {
                        midawdat = (midawdat - midawlen) + 1;
                        memcpy_backwards (dev->mainstor + midawdat,
                                          iobufptr,
                                          midawlen);

                        /* Decrement buffer pointer */
                        iobuf -= midawlen;
                    }
                    else
                    {
                        if (to_iobuf)
                        {
                            memcpy (iobuf,
                                    dev->mainstor + midawdat,
                                    midawlen);
                            prefetch->pos += midawlen;
                            if (prefetch->seq)
                                prefetch->datalen[ps] = midawlen;
                        }
                        else
                        {
                            memcpy (dev->mainstor + midawdat,
                                    iobuf,
                                    midawlen);
                        }

                        /* Increment buffer pointer */
                        iobuf += midawlen;
                    }

                } /* end if(!MIDAW_FLAG_SKIP) */

                /* Display the MIDAW if CCW tracing is on */
                if (!prefetch->seq && dev->ccwtrace)
                {
                    /* Trace the CCW first, then the MIDAW, but only
                       if this is a read type CCW as determined by
                       the direction of the copying. (For write type
                       CCws, channel code properly traces CCWs before
                       we're even called.)
                    */
                    if (to_memory)
                        DISPLAY_CCW( did_ccw_trace, dev, ccw, addr, count, flags );
                    DISPLAY_IDAW( dev, PF_MIDAW, midawflg, midawdat, midawlen );
                }
#if DEBUG_DUMP
                if (dev->ccwtrace)
                {
                    if (to_memory)
                        DUMP("iobuf:", iobuf, midawlen);
                    DUMP_STORAGE("Storage:", midawdat, midawlen);
                    if (to_iobuf)
                        DUMP("iobuf:", iobuf, midawlen);
                }
#endif
                /* Decrement remaining count */
                midawrem -= midawlen;

            }

            /* Increment to next MIDAW address */
            midawptr += 16;

        } /* end for(midawseq) */

        /* Channel program check if sum of MIDAW lengths
           did not exhaust the CCW count and no pending status */
        if (1
            && midawrem > 0
            && *chanstat != 0
            && !(prefetch->seq && prefetch->chanstat[ps])
        )
            set_chanstat( CSW_PROGC );

    } /* end if(CCW_FLAGS_MIDAW) */
    else
#endif /*defined(FEATURE_MIDAW_FACILITY)*/
    /* Move data when indirect data addressing is used */
    if (flags & CCW_FLAGS_IDA)
    {
        if (prefetch->seq)
            prefetch->datalen[ps] = 0;

        idawaddr = addr;
        idacount = count;
        idasize = (idawfmt == PF_IDAW1) ? 4 : 8;

        for (idaseq = 0;
             idacount > 0 &&
                chanstat != 0 &&
                !(prefetch->seq &&
                    (prefetch->chanstat[ps] ||
                        ((ps+1) < prefetch->seq &&
                            prefetch->chanstat[ps+1])));
             idaseq++)
        {
            /* Get new prefetch entry */
            get_new_prefetch_entry( idawfmt, idawaddr );

            if (*chanstat != 0)
                break;

            /* Fetch the IDAW and set IDA pointer and length */
            ARCH_DEP( fetch_idaw )( dev, code, ccwkey, idawfmt,
                        idapmask, idaseq, idawaddr,
                        &idadata, &idalen, chanstat );

            /* Exit if fetch_idaw detected channel program check */
            if (prefetch->seq)
            {
                prefetch->dataaddr[ps] = idadata;
                prefetch->datalen[ps]  = idalen;

                if (*chanstat != 0)
                {
                    prefetch->chanstat[ps] = *chanstat;
                    *chanstat = 0;
                    break;
                }
            }
            else if (*chanstat != 0)
                break;

            /* Channel protection check if IDAW data location is
               fetch protected, or if location is store protected
               and command is READ, READ BACKWARD, or SENSE */
            storkey = ARCH_DEP( get_dev_storage_key )( dev, idadata );

            if (1
                && ccwkey != 0
                && (storkey & STORKEY_KEY) != ccwkey
                && ((storkey & STORKEY_FETCH) || to_memory)
            )
            {
                set_chanstat(CSW_PROTC);
                break;
            }

            /* Reduce length if less than one page remaining */
            if (idalen > idacount)
            {
                idalen = idacount;

                if (prefetch->seq)
                   prefetch->datalen[ps] = idacount;
            }

            /* Ensure memcpy will stay within buffer         */
            /* Channel data check if outside buffer          */
            /* SA22-7201-05:                                 */
            /*  p. 16-27, Channel-Data Check                 */
            if (readbackwards)
            {
                iobufptr = iobuf + dev->curblkrem + idacount - idalen;

                if ((iobufptr + idalen) > (iobufend + 1) ||
                    (iobufptr + idalen) <= iobufstart)
                {
                    *chanstat = CSW_CDC;
                    return;
                }

                if (iobufptr < iobufstart)
                {
                    *chanstat = CSW_CDC;

                    /* Reset length to copy to buffer */
                    idalen = iobufptr + idalen - iobufstart;
                }
            }
            else if (iobuf < iobufstart || iobuf > iobufend)
            {
                set_chanstat( CSW_CDC );
                break;
            }
            else if ((iobuf + idalen) > (iobufend + 1))
            {
                /* Reset length to copy to buffer */
                idalen = iobufend - iobuf + 1;

                if (prefetch->seq)
                {
                    prefetch->datalen[ps] = idalen;

                    /* Get new prefetch entry for channel data
                       check */
                    get_new_prefetch_entry( idawfmt, idawaddr );

                    if (*chanstat != 0)
                        break;

                    prefetch->dataaddr[ps] = idadata + idalen;
                    prefetch->datalen[ps]  = 1;
                    prefetch->chanstat[ps] = *chanstat = CSW_CDC;

                    ps -= 1;
                }

                /* Set channel data check and permit copy to/from
                   end-of-buffer */
                else
                    set_chanstat( CSW_CDC );
            }

            /* Copy to I/O buffer */
            if (idalen)
            {
                /* Set the main storage reference and change bits */
                if (to_memory)
                    ARCH_DEP( or_dev_storage_key )( dev, idadata, (STORKEY_REF | STORKEY_CHANGE) );
                else
                    ARCH_DEP( or_dev_storage_key )( dev, idadata, STORKEY_REF );

                /* Copy data between main storage and channel buffer */
                if (readbackwards)
                {
                    idadata = (idadata - idalen) + 1;
                    memcpy_backwards( dev->mainstor + idadata,
                                      iobuf + dev->curblkrem + idacount - idalen,
                                      idalen );

                    /* Decrement buffer pointer for next IDAW*/
                    iobuf -= idalen;
                }
                else
                {
                    if (to_iobuf)
                    {
                        memcpy( iobuf, dev->mainstor + idadata, idalen );
                    }
                    else
                    {
                        memcpy( dev->mainstor + idadata, iobuf, idalen );
                    }

                    /* Increment buffer pointer for next IDAW*/
                    iobuf += idalen;
                }

                /* Update prefetch completed bytes */
                prefetch->pos += idalen;
            }

            /* If not prefetch, display the IDAW if CCW tracing */
            if (!prefetch->seq && dev->ccwtrace)
            {
                /* Trace the CCW first, then the IDAW, but only
                   if this is a read type CCW as determined by
                   the direction of the copying. (For write type
                   CCws, channel code properly traces CCWs before
                   we're even called.)
                */
                if (to_memory)
                    DISPLAY_CCW( did_ccw_trace, dev, ccw, addr, count, flags );
                DISPLAY_IDAW( dev, idawfmt, 0, idadata, idalen );
            }

#if DEBUG_DUMP
            if (dev->ccwtrace)
            {
                if (to_memory)
                    DUMP( "iobuf:", iobuf-idalen, idalen );
                DUMP_STORAGE( "Storage:", idadata, idalen );
                if (to_iobuf)
                    DUMP( "iobuf:", iobuf-idalen, idalen );
            }
#endif
            /* Decrement remaining count, increment buffer pointer */
            idacount -= idalen;

            /* Increment to next IDAW address */
            idawaddr += idasize;

        } /* end for(idaseq) */
    }
    else                              /* Non-IDA data addressing */
    {
        /* Point to start of data for read backward command */
        if (readbackwards)
            addr = addr - (count - 1);

        /* Channel protection check if any data is fetch protected,
           or if location is store protected and command is READ,
           READ BACKWARD, or SENSE.
        */
        startpage = addr;
        endpage = addr + (count - 1);

        for (page = startpage & STORAGE_KEY_PAGEMASK;
             page <= (endpage | STORAGE_KEY_BYTEMASK);
             page += STORAGE_KEY_PAGESIZE)
        {
            /* Channel program check if data is outside main storage */
            if (CHADDRCHK( page, dev ))
            {
                if (prefetch->seq)
                {
                    prefetch->chanstat[ps] = CSW_PROGC;
                    break;
                }

                *chanstat = CSW_PROGC;

                if (readbackwards)
                    return;

                break;
            }

            storkey = ARCH_DEP( get_dev_storage_key )( dev, page );

            if (1
                && ccwkey != 0
                && (storkey & STORKEY_KEY) != ccwkey
                && ((storkey & STORKEY_FETCH) || to_memory)
            )
            {
                if (readbackwards)
                {
                    *residual = MAX( page, addr ) - addr;
                    *chanstat = CSW_PROTC;
                    break;
                }

                /* Calculate residual */
                *residual = count - (MAX( page, addr ) - addr);

                /* Handle prefetch */
                if (prefetch->seq)
                {
                    prefetch->datalen[ps] = MAX( page, addr ) - addr;

                    if (*residual)
                    {
                        /* Split entry */
                        get_new_prefetch_entry( PF_NO_IDAW, page );

                        if (*chanstat != 0)
                            break;

                        prefetch->dataaddr[ps] = page;
                        prefetch->datalen[ps] = *residual;
                    }

                    prefetch->chanstat[ps] = CSW_PROTC;
                    break;
                }

                *chanstat = CSW_PROTC;
                break;
            }

        } /* end for(page) */

        /* Adjust local count for copy to main storage */
        count = MIN( count, MAX( page, addr ) - addr );

        /* Count may be zero during prefetch operations */
        if (count)
        {
            /* Set the main storage reference and change bits */
            for (page = startpage & STORAGE_KEY_PAGEMASK;
                 page <= (endpage | STORAGE_KEY_BYTEMASK);
                 page += STORAGE_KEY_PAGESIZE)
            {
                if (to_memory)
                    ARCH_DEP( or_dev_storage_key )( dev, page, (STORKEY_REF | STORKEY_CHANGE) );
                else
                    ARCH_DEP( or_dev_storage_key )( dev, page, STORKEY_REF );
            } /* end for(page) */

#if DEBUG_PREFETCH
            if (dev->ccwtrace)
            {
                char msgbuf[133];

                MSGBUF( msgbuf,
                    "CCW %2.2X %2.2X %4.4X %8.8X to_memory=%d to_iobuf=%d readbackwards=%d",
                    (U8)code, (U8)flags, (U16)count, (U32)addr, to_memory, to_iobuf, readbackwards );
                WRMSG( HHC01392, "D", msgbuf );
            }
#endif
            /* Copy data between main storage and channel buffer */
            if (readbackwards)
            {
                BYTE *iobufptr = iobuf + dev->curblkrem;

                /* Channel check if outside buffer                   */
                /* SA22-7201-05:                                     */
                /*  p. 16-27, Channel-Data Check                     */
                if (!count                              ||
                    (iobufptr + count) > (iobufend + 1) ||
                    iobufptr < iobufstart)
                    *chanstat = CSW_CDC;
                else
                {
                    /* read backward  - use END of buffer */
                    memcpy_backwards( dev->mainstor + addr,
                        iobufptr, count );
                }
            }

            /* Channel check if outside buffer                       */
            /* SA22-7201-05:                                         */
            /*  p. 16-27, Channel-Data Check                         */
            else if (!count                           ||
                     (iobuf + count) > (iobufend + 1) ||
                     iobuf < iobufstart)
                set_chanstat( CSW_CDC );

            /* Handle Write and Control transfer to I/O buffer */
            else if (to_iobuf)
            {
                memcpy( iobuf, dev->mainstor + addr, count );

                prefetch->pos += count;

                if (prefetch->seq)
                    prefetch->datalen[ps] = count;

            }

            /* Handle Read transfer from I/O buffer */
            else
            {
                memcpy( dev->mainstor + addr, iobuf, count );
            }

#ifdef FEATURE_S370_CHANNEL
            if (dev->devtype == 0x2703)
                if (dev->commadpt->lnctl == COMMADPT_LNCTL_ASYNC)
                    USLEEP(5000);
#endif

#if DEBUG_DUMP
            if (dev->ccwtrace)
            {
                char msgbuf[133];

                MSGBUF( msgbuf, "iobuf->%8.8X.%4.4X", addr, count );
                WRMSG( HHC01390, "D", msgbuf );

                MSGBUF( msgbuf, "addr=%8.8X count=%d residual=%d copy=%d",
                    addr, count, *residual, count );
                WRMSG( HHC01390, "D", msgbuf );

                if (to_memory)
                    DUMP( "iobuf:", iobuf, count );
                DUMP_STORAGE( "Storage:", addr, count );
                if (to_iobuf)
                    DUMP( "iobuf:", iobuf, count );
            }
#endif
        }

    } /* end if(!IDA) */

#undef get_new_prefetch_entry
#undef set_chanstat

} /* end function copy_iobuf */


/*-------------------------------------------------------------------*/
/* DEVICE ATTENTION                                                  */
/* Raises an unsolicited interrupt condition for a specified device. */
/* Return value is 0 if successful, 1 if device is busy or pending   */
/* or 3 if subchannel is not valid or not enabled                    */
/*                                                                   */
/* SA22-7204-00:                                                     */
/*  p. 4-1, Attention                                                */
/* GA22-6974-09:                                                     */
/*  pp. 2-13 -- 2-14, Attention                                      */
/*                                                                   */
/* Note:  An unsolicited interrupt does not always have the          */
/*        attention bit set, and the software differentiates between */
/*        an unsolicited interrupt with the attention bit set and    */
/*        not set. A good example of this are the 3270 device types  */
/*        where the attention bit set on an unsolicited interrupt    */
/*        indicates "data available". Consequently, it is the        */
/*        calling routine's responsibility to set the attention bit, */
/*        when required.                                             */
/*                                                                   */
/*        For further reference, refer to GA23-0218-11, IBM          */
/*        3174 Function Description, 3.1.3.2.3 Unsolicited           */
/*        Status, Table  5-9. Unsolicited Status Conditions          */
/*        (Non-SNA).                                                 */
/*-------------------------------------------------------------------*/
DLL_EXPORT int
ARCH_DEP( device_attention )( DEVBLK* dev, BYTE unitstat )
{
    OBTAIN_INTLOCK( NULL );
    {
        OBTAIN_DEVLOCK( dev );
        {
            if (dev->hnd->attention)
                dev->hnd->attention( dev );

#if defined( FEATURE_CHANNEL_SUBSYSTEM )
            /* If subchannel not valid and enabled, do not present interrupt */
            if (0
                || (dev->pmcw.flag5 & PMCW5_V) == 0
                || (dev->pmcw.flag5 & PMCW5_E) == 0
            )
            {
                RELEASE_DEVLOCK( dev );
                RELEASE_INTLOCK( NULL );
                return 3;
            }
#endif
            /* If device is already busy or interrupt pending */
            if (0
                || dev->busy
                || IOPENDING( dev )
                || (dev->scsw.flag3 & SCSW3_SC_PEND)
            )
            {
                int rc;                         /* Return code               */

                /* Resume the suspended device with attention set            */
                /* SA22-7204-00:                                             */
                /*  p. 4-1, Attention                                        */
                if(dev->scsw.flag3 & SCSW3_AC_SUSP)
                {
                    unitstat |= CSW_ATTN;

                    dev->scsw.unitstat |= unitstat;
                    dev->scsw.flag2    |= SCSW2_AC_RESUM;
                    dev->scsw.flag3    |= SCSW3_SC_ALERT | SCSW3_SC_PEND;

                    schedule_ioq( NULL, dev );

                    if (dev->ccwtrace)
                    {
                        if (sysblk.traceFILE)
                            tf_1304( dev );
                        else
                            // "%1d:%04X CHAN: attention signaled"
                            WRMSG( HHC01304, "I", LCSS_DEVNUM );
                    }
                    rc = 0;
                }
                else
                    rc = 1;

                RELEASE_DEVLOCK( dev );
                RELEASE_INTLOCK( NULL );
                return rc;
            }

            if (dev->ccwtrace)
            {
                if (sysblk.traceFILE)
                    tf_1305( dev );
                else
                    // "%1d:%04X CHAN: attention"
                    WRMSG( HHC01305, "I", LCSS_DEVNUM );
            }

            /* We already have INTLOCK and dev->lock held, so now
               we only need to acquire the interrupt queue lock. */
            OBTAIN_IOINTQLK();
            {
                /* Set SCSW for attention interrupt                              */
                /* SA22-7201-05:                                                 */
                /*  p. 16-3, Unsolicited Interuption Condition                   */
                /*           Solicited Interuption Condition                     */
                /*           Figure 16-1, Interruption Condition for Status-     */
                /*                        Control-Bit Combinations               */
                /*  p. 16-4, Alert Interruption Condition                        */
                /*  p. 16-16 -- 16-17, Alert Status (Bit 27)                     */
                /*  p. 16-18, Status-Pending (Bit 31)                            */
                /*                                                               */
                /*  Hercules maintains for tracking purposes regardless of       */
                /*  architecture.                                                */
                /*                                                               */
                /* Set CSW for attention interrupt when in S/360 or S/370 mode,  */
                /* CSW will be derived from the SCSW when interrupt is issued    */
                /* SA22-7204-00:                                                 */
                /*  p. 4-1, Attention                                            */
                /* GA22-6974-09:                                                 */
                /*  pp. 2-13 -- 2-14, Attention                                  */
                dev->attnscsw.flag3 = SCSW3_SC_ALERT | SCSW3_SC_PEND;
                store_fw (dev->attnscsw.ccwaddr, 0);
                dev->attnscsw.unitstat = unitstat;
                dev->attnscsw.chanstat = 0;
                store_hw (dev->attnscsw.count, 0);

                /* Queue the attention interrupt */
                QUEUE_IO_INTERRUPT_QLOCKED( &dev->attnioint, FALSE );

                /* Update interrupt status */
                subchannel_interrupt_queue_cleanup( dev );
                UPDATE_IC_IOPENDING_QLOCKED();

                /* Release locks and return to caller */
            }
            RELEASE_IOINTQLK();
        }
        RELEASE_DEVLOCK( dev );
    }
    RELEASE_INTLOCK( NULL );

    return 0;
} /* end function device_attention */


/*-------------------------------------------------------------------*/
/* START A CHANNEL PROGRAM                                           */
/* This function is called by the SIO and SSCH instructions          */
/*-------------------------------------------------------------------*/
/* Input                                                             */
/*      dev     -> Device control block                              */
/*      orb     -> Operation request block                           */
/* Output                                                            */
/*      The I/O parameters are stored in the device block, and a     */
/*      thread is created to execute the CCW chain asynchronously.   */
/*      The return value is the condition code for the SIO or        */
/*      SSCH instruction.                                            */
/* Note                                                              */
/*      For S/370 SIO, only the protect key and CCW address are      */
/*      valid, all other ORB parameters are set to zero.             */
/*-------------------------------------------------------------------*/
int
ARCH_DEP(startio) (REGS *regs, DEVBLK *dev, ORB *orb)
{
int     rc;                             /* Return code               */

    OBTAIN_DEVLOCK( dev );
    {
#if defined( _FEATURE_IO_ASSIST )
        if(SIE_MODE(regs)
          && (regs->siebk->zone != dev->pmcw.zone
            || !(dev->pmcw.flag27 & PMCW27_I)))
        {
            RELEASE_DEVLOCK( dev );
            longjmp(regs->progjmp,SIE_INTERCEPT_INST);
        }
#endif

        /* Return condition code 1 if status pending */
        if (unlikely((dev->scsw.flag3     & SCSW3_SC_PEND)  ||
                     (dev->pciscsw.flag3  & SCSW3_SC_PEND)  ||
                     (dev->attnscsw.flag3 & SCSW3_SC_PEND)  ||
                     dev->tschpending))
        {
            RELEASE_DEVLOCK( dev );
            return 1;
        }

        /* Return condition code 2 if device is busy */
#if defined( OPTION_SHARED_DEVICES )
        if (unlikely((dev->busy && dev->shioactive == DEV_SYS_LOCAL)
            || dev->startpending))
#else
        if (unlikely((dev->busy)
            || dev->startpending))
#endif
        {
            RELEASE_DEVLOCK( dev );

            /*************************************************************/
            /* VM system abends IQM00 were found to be caused by startio */
            /* SSCH resulting in cc=2 thanks to this additional log msg. */
            /*                        Peter J. Jansen, 21-Jun-2016       */
            /*************************************************************/
            if (dev->ccwtrace)
            {
                if (sysblk.traceFILE)
                    tf_1336( dev );
                else
                    // "%1d:%04X CHAN: startio cc=2 (busy=%d startpending=%d)"
                    WRMSG( HHC01336, "I", SSID_TO_LCSS(dev->ssid),
                           dev->devnum, dev->busy, dev->startpending );
            }

            return 2;
        }

        /* Ensure clean status flag bits */
        dev->suspended          = 0;
        dev->pending            = 0;
        dev->pcipending         = 0;
        dev->attnpending        = 0;
        dev->startpending       = 0;
        dev->resumesuspended    = 0;
        dev->tschpending        = 0;

        /* Initialize the subchannel status word */
        memset (&dev->scsw,     0, sizeof(SCSW));
        dev->scsw.flag0 = (orb->flag4 & (SCSW0_KEY |
                                         SCSW0_S));
        dev->scsw.flag1 = (orb->flag5 & (SCSW1_F   |
                                         SCSW1_P   |
                                         SCSW1_I   |
                                         SCSW1_A   |
                                         SCSW1_U));

        /* Set the device busy indicator */
        set_subchannel_busy(dev);

        /* Initialize shadow SCSWs */
        memcpy(&dev->pciscsw,  &dev->scsw, sizeof(SCSW));
        memcpy(&dev->attnscsw, &dev->scsw, sizeof(SCSW));

        /* Make the subchannel start-pending */
        dev->scsw.flag2 |= SCSW2_FC_START | SCSW2_AC_START;
        dev->startpending = 1;

        /* Copy the I/O parameter to the path management control word */
        memcpy (dev->pmcw.intparm, orb->intparm,
                            sizeof(dev->pmcw.intparm));

        /* Store the start I/O parameters in the device block */
        if (orb->flag7 & ORB7_X)
        {
            /* Extended ORB */
            memcpy(&dev->orb, orb, sizeof(ORB));
        }
        else
        {
            /* Original ORB size */
            memcpy(&dev->orb, orb, 12);
            memset(&dev->orb.csspriority, 0, sizeof(ORB) - 12);
        }

        if (dev->orbtrace)
        {
            if (dev->ccwtrace && sysblk.traceFILE)
                tf_1334( dev, orb );
            else
            {
                char msgbuf[128] = {0};
                FormatORB( orb, msgbuf, sizeof( msgbuf ));
                // "%1d:%04X CHAN: ORB: %s"
                WRMSG( HHC01334, "I", LCSS_DEVNUM, msgbuf );
            }
        }

        /* Set I/O priority */
        dev->priority &= 0x00FF0000ULL;
        dev->priority |= dev->orb.csspriority << 8;
        dev->priority |= dev->orb.cupriority;

        /* Schedule the I/O for execution */
        rc = schedule_ioq((sysblk.arch_mode == ARCH_370_IDX) ? regs : NULL,
                          dev);
    }
    RELEASE_DEVLOCK( dev );
    return (rc);

} /* end function startio */


/*-------------------------------------------------------------------*/
/* execute_ccw_chain exit functions                                  */
/*-------------------------------------------------------------------*/

#ifndef EXECUTE_CCW_CHAIN_RETURN_FUNCS
#define EXECUTE_CCW_CHAIN_RETURN_FUNCS

static INLINE void* execute_ccw_chain_fast_return
(
    IOBUF*  pIOBUF,
    IOBUF*  pInitial_IOBUF,
    void*   pvRetVal
)
{
    if (pIOBUF != pInitial_IOBUF)
        iobuf_destroy( pIOBUF );
    return pvRetVal;
}

static INLINE void* execute_ccw_chain_clear_busy_unlock_and_return
(
    DEVBLK*  dev,
    IOBUF*   pIOBUF,
    IOBUF*   pInitial_IOBUF,
    void*    pvRetVal
)
{
    clear_subchannel_busy( dev );
    RELEASE_DEVLOCK( dev );
    return execute_ccw_chain_fast_return( pIOBUF, pInitial_IOBUF, pvRetVal );
}

static INLINE void* execute_ccw_chain_clear_busy_and_return
(
    DEVBLK*  dev,
    IOBUF*   pIOBUF,
    IOBUF*   pInitial_IOBUF,
    void*    pvRetVal
)
{
    OBTAIN_DEVLOCK( dev );
    return execute_ccw_chain_clear_busy_unlock_and_return( dev, pIOBUF, pInitial_IOBUF, pvRetVal );
}
#endif // EXECUTE_CCW_CHAIN_RETURN_FUNCS


/*-------------------------------------------------------------------*/
/* EXECUTE A CHANNEL PROGRAM                                         */
/*-------------------------------------------------------------------*/
void*
ARCH_DEP(execute_ccw_chain) (void *arg)
{
DEVBLK *dev = (DEVBLK*) arg;            /* Device Block pointer      */
IOBUF  *iobuf;                          /* Pointer to I/O buffer     */
U32     ccwaddr = 0;                    /* Address of CCW            */
U32     ticaddr = 0;                    /* Previous CCW was a TIC    */
U16     ticback = 0;                    /* Backwards TIC counter     */
U16     idapmask = 0;                   /* IDA page size - 1         */
BYTE    idawfmt = 0;                    /* IDAW format (1 or 2)      */
BYTE    ccwfmt = 0;                     /* CCW format (0 or 1)       */
BYTE    ccwkey = 0;                     /* Bits 0-3=key, 4-7=zero    */
BYTE    opcode;                         /* CCW operation code        */
BYTE    flags;                          /* CCW flags                 */
U32     addr;                           /* CCW data address          */
#ifdef FEATURE_CHANNEL_SUBSYSTEM
U32     mbaddr;                         /* Measure block address     */
MBK    *mbk;                            /* Measure block             */
U16     mbcount;                        /* Measure block count       */
#endif /*FEATURE_CHANNEL_SUBSYSTEM*/
U32     count;                          /* CCW byte count            */
BYTE   *ccw;                            /* CCW pointer               */
BYTE    unitstat;                       /* Unit status               */
BYTE    chanstat;                       /* Channel status            */
U32     residual = 0;                   /* Residual byte count       */
BYTE    more;                           /* 1=Count exhausted         */
BYTE    chain = 1;                      /* 1=Chain to next CCW       */
BYTE    tracethis = 0;                  /* 1=Trace this CCW chain    */
BYTE    ioerror = 0;                    /* 1=CCW I/O error           */
bool    did_ccw_trace = false;          /* true = CCW traced         */
BYTE    firstccw = 1;                   /* 1=First CCW               */
BYTE    area[64];                       /* Message area              */
u_int   bufpos = 0;                     /* Position in I/O buffer    */
u_int   skip_ccws = 0;                  /* Skip ccws                 */
int     cmdretry = 255;                 /* Limit command retry       */
U32     prevccwaddr = 1;                /* Previous CCW address      */
U32     prefetch_remaining;             /* Prefetch bytes remaining  */

u_int       ps = 0;                     /* Local prefetch sequence   */
u_int       ts;                         /* Test prefetch sequence    */

CACHE_ALIGN
PREFETCH    prefetch;                   /* Prefetch buffer           */

__ALIGN( IOBUF_ALIGN )
IOBUF iobuf_initial;                    /* Channel I/O buffer        */

    /* Initialize prefetch */
    memset(&prefetch, 0, sizeof(prefetch));

    /* Point to initial I/O buffer and initialize */
    iobuf = &iobuf_initial;
    iobuf_initialize(iobuf, sizeof(iobuf_initial.data));

    OBTAIN_DEVLOCK( dev );

#if defined( OPTION_SHARED_DEVICES )
    /* Wait for the device to become available */
    if (dev->shareable)
    {
        shared_iowait( dev );
    }
    dev->shioactive = DEV_SYS_LOCAL;
#endif // defined( OPTION_SHARED_DEVICES )

    set_subchannel_busy(dev);
    dev->startpending = 0;

    /* Increment excp count */
    dev->excps++;

    /* Indicate that we're started */
    dev->scsw.flag2 |= SCSW2_FC_START;
    dev->scsw.flag2 &= ~SCSW2_AC_START;

    /* Handle early clear or halt */
    if (dev->scsw.flag2 & (SCSW2_AC_CLEAR | SCSW2_AC_HALT))
    {
        RELEASE_DEVLOCK( dev );
        if (dev->scsw.flag2 & SCSW2_AC_CLEAR)
            goto execute_clear;
        goto execute_halt;
    }

    /* For hercules `resume' resume suspended state */
    if (dev->resumesuspended || dev->suspended ||
        (dev->scsw.flag2 & SCSW2_AC_RESUM))
    {
        /* Restore CCW execution variables */
        ccwaddr = dev->ccwaddr;
        idapmask = dev->idapmask;
        idawfmt = dev->idawfmt;
        ccwfmt = dev->ccwfmt;
        ccwkey = dev->ccwkey;


        /* Turn the `suspended' bits off */
        dev->suspended =
        dev->resumesuspended=0;

#if defined( OPTION_SHARED_DEVICES )
        /* Wait for the device to become available */
        shared_iowait( dev );
        dev->shioactive = DEV_SYS_LOCAL;
#endif // defined( OPTION_SHARED_DEVICES )

        set_device_busy(dev);

        if (CCW_TRACING_ACTIVE( dev, tracethis ))
        {
            if (dev->s370start)
            {
                if (sysblk.traceFILE)
                    tf_1321( dev );
                else
                    /* State successful conversion from synchronous
                     * to asynchronous for 370 mode.
                     */
                    // "%1d:%04X CHAN: start I/O S/370 conversion to asynchronous operation successful"
                    WRMSG( HHC01321, "I", LCSS_DEVNUM );
            }
            else
            {
                if (sysblk.traceFILE)
                    tf_1311( dev );
                else
                    /* Trace I/O resumption */
                    // "%1d:%04X CHAN: resumed"
                    WRMSG (HHC01311, "I", LCSS_DEVNUM );
            }
        }

        /* Reset the suspended status in the SCSW */
        dev->scsw.flag2 &= ~SCSW2_AC_RESUM;
        dev->scsw.flag3 &= ~(SCSW3_AC_SUSP  |
                             SCSW3_SC_ALERT |
                             SCSW3_SC_INTER |
                             SCSW3_SC_PEND);
        dev->scsw.flag3 |= (SCSW3_AC_SCHAC | SCSW3_AC_DEVAC);

        /* Call the i/o resume exit if not clearing */
        if (!(dev->scsw.flag2 & SCSW2_AC_CLEAR))
        {
            /* Don't execute resume exit if S/370 channel start */
            if (!dev->s370start &&
                dev->hnd->resume)
                (dev->hnd->resume) (dev);

            /* Refetch the suspended CCW */
            ccwaddr -= 8;
        }

        /* Reset S/370 channel SIO resume indicator */
        dev->s370start = 0;

        goto resume_suspend;
    }

    /* Hercules deviation; always zero the SCSW CCW address to start */
    store_fw(dev->scsw.ccwaddr, 0);

    /* Call the i/o start exit */
    if (dev->hnd->start)
    {
        RELEASE_DEVLOCK( dev );
        {
            (dev->hnd->start) (dev);
        }
        OBTAIN_DEVLOCK( dev );
    }

    /* Extract the I/O parameters from the ORB */
    FETCH_FW(ccwaddr, dev->orb.ccwaddr);
    dev->ccwaddr = ccwaddr;
    dev->ccwfmt = ccwfmt = (dev->orb.flag5 & ORB5_F) ? 1 : 0;
    dev->ccwkey = ccwkey = dev->orb.flag4 & ORB4_KEY;
    dev->idawfmt = idawfmt = (dev->orb.flag5 & ORB5_H) ? PF_IDAW2 : PF_IDAW1;

    /* Determine IDA page size */
    if (idawfmt == PF_IDAW2)
    {
        /* Page size is 2K or 4K depending on flag bit */
        idapmask = (dev->orb.flag5 & ORB5_T) ? STORAGE_KEY_2K_BYTEMASK
                                             : STORAGE_KEY_4K_BYTEMASK;
    } else {
        /* Page size is always 2K for format-1 IDAW */
        idapmask = STORAGE_KEY_2K_BYTEMASK;
    }


resume_suspend:

    /* Turn off the start pending bit in the SCSW */
    dev->scsw.flag2 &= ~SCSW2_AC_START;

    dev->chained = dev->prev_chained =
    dev->code    = dev->prevcode     = dev->ccwseq = 0;

#if defined(_FEATURE_IO_ASSIST)
 #define _IOA_MBO sysblk.zpb[dev->pmcw.zone].mbo
 #define _IOA_MBM sysblk.zpb[dev->pmcw.zone].mbm
 #define _IOA_MBK sysblk.zpb[dev->pmcw.zone].mbk
#else /*defined(_FEATURE_IO_ASSIST)*/
 #define _IOA_MBO sysblk.mbo
 #define _IOA_MBM sysblk.mbm
 #define _IOA_MBK sysblk.mbk
#endif /*defined(_FEATURE_IO_ASSIST)*/

#ifdef FEATURE_CHANNEL_SUBSYSTEM
    /* Update the measurement block if applicable */
    if (_IOA_MBM && (dev->pmcw.flag5 & PMCW5_MM_MBU) &&
        !(dev->scsw.flag2 & (SCSW2_AC_CLEAR | SCSW2_AC_HALT)))
    {
        mbaddr = _IOA_MBO;
        mbaddr += (dev->pmcw.mbi[0] << 8 | dev->pmcw.mbi[1]) << 5;
        if ( !CHADDRCHK(mbaddr, dev)
            && (((ARCH_DEP( get_dev_storage_key )( dev, mbaddr ) & STORKEY_KEY) == _IOA_MBK)
                || (_IOA_MBK == 0)))
        {
            ARCH_DEP( or_dev_storage_key )( dev, mbaddr, (STORKEY_REF | STORKEY_CHANGE) );
            mbk = (MBK*)&dev->mainstor[mbaddr];
            FETCH_HW(mbcount,mbk->srcount);
            mbcount++;
            STORE_HW(mbk->srcount,mbcount);
        } else {
            /* Generate subchannel logout indicating program
               check or protection check, and set the subchannel
               measurement-block-update-enable to zero */
            dev->pmcw.flag5 &= ~PMCW5_MM_MBU;
            dev->esw.scl0 |= !CHADDRCHK(mbaddr, dev) ?
                                 SCL0_ESF_MBPTK : SCL0_ESF_MBPGK;
            /*INCOMPLETE*/
        }
    }
#endif /*FEATURE_CHANNEL_SUBSYSTEM*/

    RELEASE_DEVLOCK( dev );

    /* Execute the CCW chain */
    /* On entry : No locks held */
    while ( chain )
    {
        /* Test for clear subchannel request or system shutdown */
        if (dev->scsw.flag2 & SCSW2_AC_CLEAR ||
            sysblk.shutdown)
        {

            /* No I/O delays are to be taken during clear operations */

            /* Call the i/o end exit */
            if (dev->hnd->end) (dev->hnd->end) (dev);

execute_clear:
            /* Get necessary locks */
            OBTAIN_INTLOCK( NULL );
            {
                OBTAIN_DEVLOCK( dev );
                {
                    /* Execute clear function */
                    perform_clear_subchan( dev );
                }
                RELEASE_DEVLOCK( dev );
            }
            RELEASE_INTLOCK( NULL );

            /* Return */
            return execute_ccw_chain_fast_return( iobuf, &iobuf_initial, NULL );

        } /* end perform clear subchannel */

        /* Test for halt subchannel request */
        if (dev->scsw.flag2 & SCSW2_AC_HALT)
        {
            IODELAY(dev);

            /* Call the i/o end exit */
            if (dev->hnd->end) (dev->hnd->end) (dev);

execute_halt:
            perform_halt(dev);

            if (tracethis)
            {
                if (dev->ccwtrace && sysblk.traceFILE)
                    tf_1309( dev );
                else
                    // "%1d:%04X CHAN: halt completed"
                    WRMSG( HHC01309, "I", LCSS_DEVNUM );
            }

            return execute_ccw_chain_fast_return( iobuf, &iobuf_initial, NULL );

        } /* end perform halt subchannel */

        /* Test for attention status from device */
        if (dev->scsw.flag3 & SCSW3_SC_ALERT)
        {
            IODELAY(dev);

            /* Call the i/o end exit */
            if (dev->hnd->end) (dev->hnd->end) (dev);

            /* Queue the pending interrupt and update status */
            queue_io_interrupt_and_update_status(dev,TRUE);

            if (CCW_TRACING_ACTIVE( dev, tracethis ))
            {
                if (dev->ccwtrace && sysblk.traceFILE)
                    tf_1307( dev );
                else
                    // "%1d:%04X CHAN: attention completed"
                    WRMSG( HHC01307, "I", LCSS_DEVNUM );
            }

            return execute_ccw_chain_fast_return( iobuf, &iobuf_initial, NULL );

        } /* end attention processing */

        /* Clear the channel status and unit status, unless skipping
           to end of chain data */
        if (!skip_ccws)
        {
            chanstat = 0;
            unitstat = 0;
        }

        /* Fetch the next CCW */
        ARCH_DEP(fetch_ccw) (dev, ccwkey, ccwfmt, ccwaddr, &opcode,
                             &addr, &flags, &count, &chanstat);

        /*************************************************************/
        /* NOTE: Order of process must not only match the Principles */
        /*       of Operations, but must also match the processing   */
        /*       order of real channels.                             */
        /*************************************************************/

        /* For an invalid CCW address in a TIC we must backup to
           TIC+8 */
        if(ticaddr && (chanstat & CSW_PROGC))
            ccwaddr = ticaddr-8;

        /* Point to the CCW in main storage */
        ccw = dev->mainstor + ccwaddr;

        /* Increment to next CCW address */
        ccwaddr += 8;

        /* If prefetch, update prefetch table */
        if (prefetch.seq && (dev->chained & CCW_FLAGS_CD))
        {
            ps = prefetch.seq++;
            if (prefetch.seq > PF_SIZE)
                goto breakchain;
            prefetch.ccwaddr[ps] = ccwaddr;
            prefetch.ccwflags[ps] = flags;
            prefetch.ccwcount[ps] =
            prefetch.datalen[ps] = count;
            prefetch.dataaddr[ps] = addr;

            /* Exit if fetch_ccw detected channel program check */
            if (chanstat)
                goto prefetch;
        }
        else
        {
            /* Update the CCW address in the SCSW */
            STORE_FW(dev->scsw.ccwaddr,ccwaddr);

            /* Exit if fetch_ccw detected channel program check */
            if (chanstat != 0)
                goto breakchain;

            /* Trace the CCW -- UNLESS ... it's a read type! For read
               type CCWs, we defer the tracing until AFTER the CCW has
               been executed and the data copied to storage (i.e. we
               don't trace the CCW until after BOTH the driver's CCW
               handler (dev->hnd->exec) and ARCH_DEP(copy_iobuf) have
               been called; see much further below).
            */
            if (1
                && dev->ccwtrace
                && !(0
                     || IS_CCW_READ  ( ccw[0] )
                     || IS_CCW_RDBACK( ccw[0] )
                     || IS_CCW_SENSE ( ccw[0] )
                    )
            )
                DISPLAY_CCW( &did_ccw_trace, dev, ccw, addr, count, flags );
        }

        /* Channel program check if invalid Format-1 CCW             */
        /* SA22-7201-05:                                             */
        /*  pp. 15-23 -- 15-24, Channel_Command Word                 */
        /*  p. 15-25, Count                                          */
        /*  p. 16-25, Invalid Count, Format 1                        */
        if (ccwfmt == 1 &&                  /* Validate Format 1     */
            ((opcode != 0x08)              &&
             (dev->chained & CCW_FLAGS_CD) &&
             !count))
            {
                chanstat = CSW_PROGC;
                if (prefetch.seq)
                    goto prefetch;
                goto breakchain;
            }

        /* Channel program check if invalid TIC opcode or invalid    */
        /* TIC Format-1 CCW                                          */
        /* SA22-7201-05:                                             */
        /*  p. 15-25, Figure 15-5. Command Code Assignment           */
        /*  p. 15-36, Figure 15-7. Command Codes and Flags           */
        /*  p. 15-37, Transfer in Channel                            */
        if (opcode == 0x08 ||               /* Validate TIC          */
            (ccwfmt == 0 &&
             ((opcode & 0x0F) == 0x08)))
        {
            if (ticaddr                  || /* No TIC-to-TIC         */
                (addr & 0x03)            || /* Must be aligned DWORD */
                (ccwfmt == 1 &&             /* Validate Format 1     */
                 (flags           ||
                  count))                ||
                ticback > 255               /* Exceeded TIC limit?   */
                )
            {
                chanstat = CSW_PROGC;
                if (prefetch.seq)
                {
                    prefetch.ccwflags[ps] = 0;
                    prefetch.datalen[ps] = 0;
                    goto prefetch;
                }
                goto breakchain;
            }

            /* Reuse prefetch entry for next CCW */
            if (prefetch.seq)
            {
                prefetch.ccwaddr[ps] = 0;
                prefetch.ccwflags[ps] = 0;
                prefetch.ccwcount[ps] = 0;
                prefetch.dataaddr[ps] = 0;
                prefetch.datalen[ps] = 0;
                ps--;
                prefetch.seq--;
            }

            /* Update backwards TIC counter */
            if (addr < ccwaddr)
                ticback++;
            else
                ticback = 0;

            /* Set new CCW address (leaving the values of chained and
               code untouched to allow data-chaining through TIC)    */
            ticaddr = ccwaddr;
            ccwaddr = addr;
            chain = 1;
            continue;
        }
        ticaddr = 0;                    /* Reset the TIC-to-TIC flag */

        /* Reset TIC back counter if Read or Write CCW               */
        if (IS_CCW_WRITE(opcode) ||
            IS_CCW_READ(opcode)  ||
            IS_CCW_RDBACK(opcode))
            ticback = 0;

        /* At this point, the CCW now has "control" of the I/O       */
        /* operation (SA22-7201 p. 15-24, PCI). Signal I/O interrupt */
        /* if PCI flag is set                                        */
        if (flags & CCW_FLAGS_PCI)   /* -- Debug && !prefetch.seq -- */
        {
            ARCH_DEP(raise_pci) (dev, ccwkey, ccwfmt, ccwaddr);
        }

        /* Validate basic CCW command                                */
        /* SA22-7201-05:                                             */
        /*  p. 15-25, Figure 15-5. Command Code Assignment           */
        /* Note: TIC validation not included as TIC has already been */
        /*       validated and processed.                            */
        if (!(dev->chained & CCW_FLAGS_CD) &&
            !((opcode & 0x0F) != 0      &&
              (IS_CCW_WRITE(opcode)   ||
               IS_CCW_READ(opcode)    ||
               IS_CCW_RDBACK(opcode)  ||
               IS_CCW_CONTROL(opcode) ||
               IS_CCW_SENSE(opcode))))
        {
            chanstat = CSW_PROGC;
            if (prefetch.seq)
                goto prefetch;
            goto breakchain;
        }

        /* Validate chain data (CD) flag                             */
        /* SA22-7201-05:                                             */
        /*  p. 16-26, program check if suspend specified             */
        /*  p. 15-24, Chain-Data (CD) Flag                           */
        if (flags & CCW_FLAGS_CD)
        {
            if (flags & CCW_FLAGS_SUSP)
            {
                chanstat = CSW_PROGC;
                if (prefetch.seq)
                    goto prefetch;
                goto breakchain;
            }

            /* Turn off suppress indicator bits for processing       */
            /* purposes as setting is ignored with CD.               */
            flags &= ~CCW_FLAGS_SLI;
        }

        /* Validate command chain (CC) flag                          */
        /* SA22-7201-05:                                             */
        /*  p. 15-24, Chain-Command (CC) Flag                        */
        /*-------------------------------------------------------------
        //
        // Note: With CD check first, this test will always be false.
        //       Code left here in comment for documentation purposes.
        //
        // if (flags & (CCW_FLAGS_CC | CCW_FLAGS_CD) ==
        //             (CCW_FLAGS_CC | CCW_FLAGS_CD))
        //     flags &= ~CCW_FLAGS_CC;
        -------------------------------------------------------------*/

        /* Validate suppress length indication (SLI) flag            */
        /* SA22-7201-05:                                             */
        /*  p. 15-24, Supress-Length-Indication (SLI) Flag           */
        /*-------------------------------------------------------------
        //
        // Note: With CD check first, this test will always be false.
        //       Code left here in comment for documentation purposes.
        //
        // if (flags & (CCW_FLAGS_CD | CCW_FLAGS_SLI)) ==
        //             (CCW_FLAGS_CD | CCW_FLAGS_SLI)))
        //     flags &= ~CCW_FLAGS_SLI;
        -------------------------------------------------------------*/

        /* Validate skip flag                                        */
        /* SA22-7201-05:                                             */
        /*  p. 15-24, Skip (SKIP) Flag                               */
        /*-------------------------------------------------------------
        //
        // Note: Check against MIDAW down in MIDAW validation section.
        //       Code left here in comment for documentation purposes.
        //
        // if (flags & CCW_FLAGS_SKIP)
        // {}
        -------------------------------------------------------------*/

        /* Validate program controlled interruption (PCI) flag       */
        /* SA22-7201-05:                                             */
        /*  p. 15-24, Program Controlled Interruption (PCI) Flag     */
        /*-------------------------------------------------------------
        //
        // Note: No validation required.
        //       Code left here in comment for documentation purposes.
        //
        // if (flags & CCW_FLAGS_PCI)
        // {}
        -------------------------------------------------------------*/

#if !defined(FEATURE_MIDAW_FACILITY)
        /* Channel program check if MIDAW not installed */
        if (flags & CCW_FLAGS_MIDAW)
        {
            chanstat = CSW_PROGC;
            if (prefetch.seq)
                goto prefetch;
            goto breakchain;
        }
#endif /*!defined(FEATURE_MIDAW_FACILITY)*/

#if defined(FEATURE_MIDAW_FACILITY)
        /* Channel program check if MIDAW not enabled in ORB, or     */
        /* with SKIP or IDA specified                                */
        if ((flags & CCW_FLAGS_MIDAW) &&
            ((dev->orb.flag7 & ORB7_D) == 0 ||
             (flags & (CCW_FLAGS_SKIP | CCW_FLAGS_IDA))))
        {
            chanstat = CSW_PROGC;
            if (prefetch.seq)
                goto prefetch;
            goto breakchain;
        }
#endif /*defined(FEATURE_MIDAW_FACILITY)*/

        /* Suspend supported prior to GA22-7000-10 for the S/370     */
        /* Suspend channel program if suspend flag is set */
        if (flags & CCW_FLAGS_SUSP)
        {
            /* Channel program check if the ORB suspend control bit
               was zero, or if this is a data chained CCW */
            if ((dev->orb.flag4 & ORB4_S) == 0
                || (dev->chained & CCW_FLAGS_CD))
            {
                chanstat = CSW_PROGC;
                if (prefetch.seq)
                    goto prefetch;
                goto breakchain;
            }

            IODELAY(dev);

            /* If halt or clear, abort suspend operation */
            if (dev->scsw.flag2 & (SCSW2_AC_HALT | SCSW2_AC_CLEAR))
            {
                if (dev->scsw.flag2 & SCSW2_AC_HALT)
                    goto execute_halt;
                goto execute_clear;
            }

            /* Call the i/o suspend exit */
            if (dev->hnd->suspend)
               (dev->hnd->suspend)( dev );

            OBTAIN_INTLOCK( NULL );
            {
                OBTAIN_DEVLOCK( dev );
                {
                    /* Suspend the device if not already resume pending */
                    if (!(dev->scsw.flag2 & (SCSW2_AC_RESUM)))
                    {
                        /* Clean up device and complete suspension exit */
                        clear_subchannel_busy(dev);

                        /* Set the subchannel status word to suspended       */
                        /* SA22-7201-05:                                     */
                        /*  p. 16-15, Subchannel-Active (Bit 24)             */
                        /*  pp. 16-15 -- 16-16, Device-Active (Bit 25)       */
                        /*  p. 16-16, Suspended (Bit 26)                     */
                        /*  p. 16-16, Alert Status (Bit 27)                  */
                        /*  p. 16-17, Intermediate Status (Bit 28)           */
                        /*  p. 16-18, Status-Pending (Bit 31)                */
                        dev->scsw.flag3 &= ~(SCSW3_AC_SCHAC |
                                             SCSW3_AC_DEVAC);
                        dev->scsw.flag3 |= SCSW3_AC_SUSP;
                        /* Principles violation. Some operating systems use
                         * CLI to check for suspend, intermediate and pending
                         * status (x'29') instead of the Principles statement
                         * with alert status set (x'39'). This also appears to
                         * be consistent with older machines.
                         * FIXME: Place conformance in user configuration?
                         *        flag3 |= SCSW3_SC_ALERT;
                         */

                        dev->scsw.unitstat = 0;

                        if (flags & CCW_FLAGS_PCI)
                        {
                            dev->scsw.chanstat   = CSW_PCI;
                            dev->scsw.flag3     |= SCSW3_SC_INTER   |
                                                   SCSW3_SC_PEND;
                            dev->pciscsw.flag3  &= ~SCSW3_SC_PEND;
                        }
                        else
                        {
                            dev->scsw.chanstat   = 0;
                            if (!(dev->scsw.flag1 & SCSW1_U))
                                dev->scsw.flag3 |= SCSW3_SC_INTER   |
                                                   SCSW3_SC_PEND;
                        }

                        STORE_HW(dev->scsw.count,count);

                        /* Update local copy of ORB */
                        STORE_FW(dev->orb.ccwaddr, (ccwaddr-8));

                        /* Preserve CCW execution variables for validation */
                        dev->ccwaddr = ccwaddr;
                        dev->idapmask = idapmask;
                        dev->idawfmt = idawfmt;
                        dev->ccwfmt = ccwfmt;
                        dev->ccwkey = ccwkey;

                        /* Turn on the "suspended" bit.  This enables remote
                         * systems to use the device while we're waiting
                         */
                        dev->suspended = 1;

                        /* Trace suspension point */
                        if (unlikely( CCW_TRACING_ACTIVE( dev, tracethis )))
                        {
                            if (dev->ccwtrace && sysblk.traceFILE)
                                tf_1310( dev );
                            else
                                // "%1d:%04X CHAN: suspended"
                                WRMSG( HHC01310, "I", LCSS_DEVNUM );
                        }

                        /* Present the interrupt and return */
                        if (dev->scsw.flag3 & SCSW3_SC_PEND)
                            queue_io_interrupt_and_update_status_locked(dev,FALSE);

                        RELEASE_DEVLOCK( dev );
                        RELEASE_INTLOCK(NULL);

                        if (dev->scsw.flag2 & (SCSW2_AC_CLEAR | SCSW2_AC_HALT))
                        {
                            if (dev->scsw.flag2 & SCSW2_AC_CLEAR)
                                goto execute_clear;
                            goto execute_halt;
                        }

                        return execute_ccw_chain_fast_return( iobuf, &iobuf_initial, NULL );
                    }
                }
                RELEASE_DEVLOCK( dev );
            }
            RELEASE_INTLOCK( NULL );

        } /* end if(CCW_FLAGS_SUSP) */

        /* Update current CCW opcode, unless data chaining */
        if (!(skip_ccws ||
              (dev->chained & CCW_FLAGS_CD)))
        {
            dev->prevcode = dev->code;
            dev->code = opcode;
            did_ccw_trace = false;

            /* Allow the device handler to determine whether this is
               an immediate CCW (i.e. CONTROL with no data transfer) */
            dev->is_immed = IS_CCW_IMMEDIATE(dev, opcode);

            /*-- TBD ------------------------------------------------*/
            /*                                                       */
            /*   Initiation and check of opcode with control unit    */
            /*   belongs here.                                       */
            /*                                                       */
            /*-------------------------------------------------------*/

        }

        /* If immediate, chain data and address are ignored */
        if (dev->is_immed)
        {
            flags &= ~CCW_FLAGS_CD;
            dev->chained &= ~CCW_FLAGS_CD;
            addr = 0;
        }

        /* Channel program check if CCW refers to invalid storage    */
        /* SA22-7201-05:                                             */
        /*  p. 15-24, Data Address                                   */
        /*  p. 15-25, Count                                          */
        /*  pp. 15-25 -- 15-27, Designation of Storage Area          */
        if ((count &&
             (!(flags & CCW_FLAGS_SKIP)) &&
             (((flags & CCW_FLAGS_IDA)   &&
               ((addr & 0x03) ||
                CHADDRCHK(addr, dev)))                      ||
#if defined(FEATURE_MIDAW_FACILITY)
              ((flags & CCW_FLAGS_MIDAW) &&
               ((addr & 0x0F) ||
                CHADDRCHK(addr, dev)))                      ||
#endif /*defined(FEATURE_MIDAW_FACILITY)*/
              (!(flags & (CCW_FLAGS_IDA | CCW_FLAGS_MIDAW))     &&
               ((ccwfmt == 0 &&
                 ((addr & ~0x00FFFFFF)                      ||
                  ((addr + (count - 1)) & ~0x00FFFFFF)      ||
                  CHADDRCHK((addr + (count - 1)), dev)))    ||
#if defined(FEATURE_CHANNEL_SUBSYSTEM)
                (ccwfmt == 1 &&
                 ((addr & ~0x7FFFFFFF)                      ||
                  ((addr + count - 1) & ~0x7FFFFFFF)        ||
                  CHADDRCHK((addr + (count - 1)), dev)))        ||
#endif /*defined(FEATURE_CHANNEL_SUBSYSTEM)*/
                 CHADDRCHK(addr, dev)))))
#if defined(FEATURE_CHANNEL_SUBSYSTEM)
         || (!count &&
             (ccwfmt == 1 && (flags & CCW_FLAGS_CD)))
#endif /*defined(FEATURE_CHANNEL_SUBSYSTEM)*/
            )
        {
            chanstat = CSW_PROGC;
            if (prefetch.seq)
                goto prefetch;
            goto breakchain;
        }

        /* Suspend and reschedule I/O at this point if SIO and CPU   */
        /* not yet released; if IDA specified, first IDA must be     */
        /* verified before suspend and reschedule.                   */
        if (dev->s370start)
        {
            /* Note: dev->s370start is reset in resume processing;
             *       dev->suspended is NOT set as it is not the intent
             *       to permit another system to update the device.
             */

            OBTAIN_DEVLOCK( dev );
            {
                /* State converting from SIO synchronous to asynchronous */
                if (CCW_TRACING_ACTIVE( dev, tracethis ))
                {
                    if (dev->ccwtrace && sysblk.traceFILE)
                        tf_1320( dev );
                    else
                        // "%1d:%04X CHAN: start I/O S/370 conversion to asynchronous operation started"
                        WRMSG( HHC01320, "I", LCSS_DEVNUM );
                }

                /* Update local copy of ORB */
                STORE_FW(dev->orb.ccwaddr, (ccwaddr-8));

                /* Preserve CCW execution variables for validation */
                dev->ccwaddr = ccwaddr;
                dev->idapmask = idapmask;
                dev->idawfmt = idawfmt;
                dev->ccwfmt = ccwfmt;
                dev->ccwkey = ccwkey;

                /* Set the resume pending flag and signal the subchannel;
                 * NULL is used for the requeue regs as the execution of
                 * the I/O is no longer attached to a specific processor.
                 */
                dev->scsw.flag2 |= SCSW2_AC_RESUM;
                schedule_ioq(NULL, dev);

                /* Leave device as busy, unlock device and return */
            }
            RELEASE_DEVLOCK( dev );

            return execute_ccw_chain_fast_return( iobuf, &iobuf_initial, NULL );
        }

        /* Handle initial status settings on first non-immediate CCW */
        /* to the device                                             */
        if (firstccw && !dev->is_immed)
        {
            /* Reset first CCW indication as we're starting the      */
            /* subchannel                                            */
            firstccw = 0;

            /* Subchannel and device are now active, set bits in     */
            /* SCSW                                                  */
            /* SA22-7201-05:                                         */
            /*  p. 16-14, Subchannel-Active                          */
            /*  pp. 16-14 -- 16-15, Device-Active                    */
            dev->scsw.flag3 |= (SCSW3_AC_SCHAC | SCSW3_AC_DEVAC);

            /* Process Initial-Status-Interruption Request           */
            /* SA22-7201-05:                                         */
            /*  p. 16-11, Zero Condition Code                        */
            if (dev->scsw.flag1 & SCSW1_I)
            {
                OBTAIN_DEVLOCK( dev );
                {
                    /* Update the CCW address in the SCSW */
                    STORE_FW(dev->scsw.ccwaddr,ccwaddr);

                    /* Set the zero condition-code flag in the SCSW */
                    dev->scsw.flag1 |= SCSW1_Z;

                    /* Set intermediate status in the SCSW */
                    dev->scsw.flag3 |= (SCSW3_SC_INTER | SCSW3_SC_PEND);
                }
                RELEASE_DEVLOCK( dev );

                /* Queue the interrupt and update interrupt status */
                queue_io_interrupt_and_update_status( dev, FALSE );

                if (CCW_TRACING_ACTIVE( dev, tracethis ))
                {
                    if (dev->ccwtrace && sysblk.traceFILE)
                        tf_1306( dev );
                    else
                        // "%1d:%04X CHAN: initial status interrupt"
                        WRMSG( HHC01306, "I", LCSS_DEVNUM );
                }
            }
        }

        /* For WRITE and non-immediate CONTROL operations,
           copy data from main storage into channel buffer */
        if (!skip_ccws &&
            (prefetch.seq ||
             (!dev->is_immed                 &&
              (IS_CCW_WRITE(dev->code)  ||
               IS_CCW_CONTROL(dev->code)))))
        {
            /* Clear prefetch sequence table and I/O buffer if first
               entry */
            if (!prefetch.seq)
            {
                clear_io_buffer(iobuf->data, iobuf->size);
                clear_io_buffer(&prefetch, sizeof(prefetch));
                ps = 0;
                prefetch.seq = 1;
                prefetch.prevcode = dev->prevcode;
                prefetch.opcode = dev->code;
                prefetch.ccwaddr[ps] = ccwaddr;
                prefetch.ccwflags[ps] = flags;
                prefetch.ccwcount[ps] = count;
                prefetch.chanstat[ps] = chanstat;
                chanstat = 0;
            }
prefetch:
            /* Finish prefetch table entry initialization */
            if (opcode == 0x08 ||
                (ccwfmt == 0 && ((opcode & 0x0f) == 0x08)))
            {
                prefetch.ccwflags[ps] = 0;
                prefetch.ccwcount[ps] = 0;
            }
            else
            {
                prefetch.ccwflags[ps] = flags & ~(CCW_FLAGS_SKIP |
                                                  CCW_FLAGS_SUSP);
                prefetch.reqcount += count;
            }

            /* Copy address to prefetch table */
            prefetch.dataaddr[ps] = addr;

            /* Ignore additional checks if error */
            if (chanstat)
            {
                prefetch.chanstat[ps] = chanstat;
                chanstat = 0;
            }

            /* Don't copy if immediate and zero count */
            else if (dev->is_immed && !count);

            /* Otherwise, copy data into channel buffer */
            else
            {
                U32 newsize = bufpos + count;

                prefetch.datalen[ps] = count;

                /* Extend buffer if overflow */
                if (newsize > iobuf->size)
                {
                    IOBUF *iobufnew;

                    iobufnew = iobuf_reallocate(iobuf, newsize);

                    /* If new I/O buffer allocation failed, force a
                     * Channel Data Check (CDC). Otherwise, set the
                     * iobuf pointer to the new I/O buffer space.
                     */
                    if (iobufnew == NULL)
                        prefetch.chanstat[ps] = CSW_CDC;
                    else
                        iobuf = iobufnew;
                }

                /* If no errors, prefetch data to I/O buffer */
                if (!prefetch.chanstat[ps])
                {
                    ARCH_DEP(copy_iobuf) ( &did_ccw_trace, dev, ccw, dev->code, flags, addr,
                                          count, ccwkey,
                                          idawfmt, idapmask,
                                          iobuf->data + bufpos,
                                          iobuf->start, iobuf->end,
                                          &chanstat, &residual, &prefetch);

                    /* Update local copy of prefetch sequence entry */
                    ps = prefetch.seq - 1;

                    /* Update number of bytes in channel buffer */
                    bufpos = prefetch.pos;
                }
            }

            /* If the device handler has requested merging of data
               chained write CCWs, then collect the data from the
               chained-data CCWs in the sequence before passing buffer
               to device handler */

            /* Note: This test is commented out as we prefetch the data;
                     chain data, therefore, must always be handled.

            if ((dev->orb.flag5 & ORB5_P) ||
                dev->cdwmerge)                                        */
            {
                if (dev->code != 0x08 &&
                    !(ccwfmt == 0 && ((dev->code & 0x0F) == 0x08)) &&
                    flags & CCW_FLAGS_CD)
                {
                    /* If this is the first CCW in the data chain,
                       then save the chaining flags from the
                       previous CCW */
                    if ((dev->chained & CCW_FLAGS_CD) == 0)
                        dev->prev_chained = dev->chained;

                    /* Process next CCW in data chain */
                    dev->chained = CCW_FLAGS_CD;
                    chain = 1;
                    continue;
                }

                /* If this is the last CCW in the data chain, then
                   restore the chaining flags from the previous
                   CCW */
                if (dev->chained & CCW_FLAGS_CD)
                    dev->chained = dev->prev_chained;

            } /* end if(dev->cdwmerge) */

            /* Reset pointers */
            ccwaddr = prefetch.ccwaddr[0];
            dev->prevcode = prefetch.prevcode;
            dev->code = prefetch.opcode;
            count = prefetch.reqcount;

        }   /* End prefetch */

        /* Set chaining flag */
        chain = ( flags & (CCW_FLAGS_CD | CCW_FLAGS_CC) ) ? 1 : 0;

        /* If first in sequence, begin execution and channel data
           transfer */
        if (!skip_ccws)
        {
            /* Initialize residual byte count */
            residual = count;
            more = bufpos = unitstat = chanstat = 0;

            /* Pass the CCW to the device handler for execution */
            dev->iobuf.length = iobuf->size;
            dev->iobuf.data = iobuf->data;
            (dev->hnd->exec) (dev, dev->code, flags, dev->chained,
                              count, dev->prevcode, dev->ccwseq,
                              iobuf->data,
                              &more, &unitstat, &residual);
            dev->iobuf.length = 0;
            dev->iobuf.data   = 0;

            /* Check for Command Retry (suggested by Jim Pierson) */
            if ( --cmdretry && unitstat == ( CSW_CE | CSW_DE | CSW_UC | CSW_SM ) )
            {
                chain    = 1;
                if (prefetch.seq)
                {
                    ccwaddr = prefetch.ccwaddr[0];
                    prefetch.seq = 0;
                }
                ccwaddr -= 8;   /* (retry same ccw again) */
                continue;
            }

            /* Handle command reject with no data transfer */
            if (residual == count &&
                (unitstat & CSW_UC) &&
                (dev->sense[0] & SENSE_CR))
            {
                residual = 0;
                if (prefetch.seq)
                {
                    ps = 0;
                    ccwaddr = prefetch.ccwaddr[0];
                }

                goto breakchain;
            }

            /* Handle prefetch (write) conditions */
            else if (prefetch.seq)
            {
                /* Set prefetch remaining byte count */
                prefetch_remaining = count - residual;

                /* Determine prefetched CCW limit and raise requested
                   PCI interrupts */
                for (ps = ts = 0,
                        prevccwaddr = 1;
                     ts < prefetch.seq;
                     ts++)
                {
                    /* Set clean index not affected by loop completion
                       */
                    ps = ts;

                    /* ccwaddr and flags match each CCW encountered */
                    if (prevccwaddr != prefetch.ccwaddr[ps])
                    {
                        flags = prefetch.ccwflags[ps];
                        prevccwaddr = prefetch.ccwaddr[ps];
                        residual = count = prefetch.ccwcount[ps];

                        /* Raise PCI interrupt */
                        if (flags & CCW_FLAGS_PCI)
                            ARCH_DEP(raise_pci) (dev, ccwkey, ccwfmt,
                                                 prevccwaddr);
                    }

                    /* Adjust counts */
                    if (prefetch.datalen[ps])
                    {
                        if (residual >= prefetch.datalen[ps])
                            residual -= prefetch.datalen[ps];
                        else
                            residual = 0;

                        if (prefetch_remaining >= prefetch.datalen[ps])
                            prefetch_remaining -= prefetch.datalen[ps];
                        else
                        {
                            prefetch_remaining = 0;
                            break;
                        }
                    }

                    /* Check for prefetch replay completed */
                    if (prefetch.chanstat[ps] ||
                        (!prefetch_remaining &&
                         (ps >= prefetch.seq)))
                        break;

                }

                /* Update SCSW CCW address and channel status */
                ccwaddr  = prefetch.ccwaddr[ps];
                chanstat = prefetch.chanstat[ps];

            } /* End prefetch status update */


            /* For READ, SENSE, and READ BACKWARD operations, copy data
               from channel buffer to main storage, unless SKIP is set
            */
            else if (1
                && !dev->is_immed
                && !skip_ccws
                && !(flags & CCW_FLAGS_SKIP)
                && (0
                    || IS_CCW_READ(   dev->code )
                    || IS_CCW_SENSE(  dev->code )
                    || IS_CCW_RDBACK( dev->code )
                   )
            )
            {
                /* Copy data from I/O buffer to main storage */
                ARCH_DEP(copy_iobuf) ( &did_ccw_trace, dev, ccw, dev->code, flags, addr,
                                      count - residual, ccwkey,
                                      idawfmt, idapmask,
                                      iobuf->data,
                                      iobuf->start, iobuf->end,
                                      &chanstat, &residual, &prefetch);

                /* Update number of bytes in channel buffer */
                bufpos = prefetch.pos;

                /* If error during copy, skip remaining CD CCWs */
                if (chanstat && (flags & CCW_FLAGS_CD))
                    skip_ccws = 1;
            }
        }

        /* Check for incorrect length */
        if ((residual || more)
            && !(chanstat & ~(CSW_PCI | CSW_IL)))
        {
            /* Set incorrect length status if not suppressed         */
            /* SA22-7201-05:                                         */
            /*  pp. 16-24 -- 16-25, Incorrect Length                 */
            /* GA22-7000-10:                                         */
            /*  p. 13-70, Incorrect Length                           */
            /*

            The SLI bit in the CCW ALWAYS suppresses incorrect length,
            regardless of Format-0 or Format-1 and regardless of whether
            the CCW is an immediate operation or not and regardless of
            whether the count field is zero or not (i.e. regardless of
            whether there is a residual or not).

            Incorrect length is also NEVER indicated for an immediate
            operation if the count field is zero, REGARDLESS of the
            SLI bit setting. This however only pertains to Format-1
            CCWs as only Format-1 CCWs may contain a length field of
            zero. The length field for a Format-0 CCW must always have
            a non-zero value from 1 to 65,535.

            So we only need to check for possible incorrect length
            if we have a residual.

            BUT... when:

                a)  the CCW is an IMMEDIATE operation, and
                b)  SLI bit NOT set (is zero), and
                c)  the count field (residual) is non-zero,

            Then:

              Format-0:  Incorrect length is always SUPPRESSED.

              Format-1:  Incorrect length is always INDICATED...
                         UNLESS Incorrect-Length-Suppression Mode
                         is requested via the ORB 'L' flag!

                         That is to say, without the 'L' flag, Incor-
                         rect Length errors WILL occur for Format-1
                         immediate operations when there's a residual.

                         The ORB 'L' flag however, SUPPRESSES Incorrect
                         Length error for Format-1 immediate operations
                         causing them to behave the same as Format-0.
            */
            if (((residual && !(flags & CCW_FLAGS_SLI)) ||
                 ((more || (residual && prefetch.seq)) &&
                  !(flags & (CCW_FLAGS_CD | CCW_FLAGS_SLI))))

                && !(dev->is_immed && (flags & CCW_FLAGS_CC))  /* DSF Debug */

                && (0
                    || !dev->is_immed
#if defined( FEATURE_INCORRECT_LENGTH_INDICATION_SUPPRESSION )

                    /* Immediate operation. If Format-1 WITHOUT the
                       ORB 'L' flag, then Incorrect Length. Otherwise
                       if Format-0, or Format-1 WITH Incorrect Length
                       Suppresion enabled, then IL is NOT indicated
                       (i.e. is SUPPRESSED).
                    */
                    || (1
                        &&  (dev->orb.flag5 & ORB5_F)
                        && !(dev->orb.flag7 & ORB7_L)
                       )
#endif /* defined( FEATURE_INCORRECT_LENGTH_INDICATION_SUPPRESSION ) */
                   )
            )
                chanstat |= CSW_IL;
        }

breakchain:

        if (unlikely((chanstat & (CSW_PROGC | CSW_PROTC | CSW_CDC |
                                  CSW_CCC   | CSW_ICC   | CSW_CHC))
                  || ((unitstat & CSW_UC) && dev->sense[0])))
        {

            BYTE  tracing_active  = CCW_TRACING_ACTIVE( dev, tracethis );
            BYTE  cpu_tracing     = CPU_STEPPING_OR_TRACING_ALL;
            BYTE  ostailor_quiet  = (sysblk.pgminttr == 0);
            BYTE  ccw_tracing     = dev->ccwtrace;
            BYTE  skip_ch9uc      = SKIP_CH9UC( dev, chanstat, unitstat );

            /* Trace the CCW if not already done */
            if (1
                && !tracing_active
                && (0
                    || cpu_tracing
                    || !ostailor_quiet
                    || ccw_tracing
                   )
                && !skip_ch9uc
            )
            {
                ioerror = 1;
                DISPLAY_CCW( &did_ccw_trace, dev, ccw, addr, count, flags );
            }

            /* Activate tracing for this CCW chain only
               if any trace is already active */
            if (0
                || ccw_tracing
                || cpu_tracing
                || (1
                    && !ostailor_quiet
                    && !skip_ch9uc
                   )
            )
            {
                tracethis = 1;
            }
        }
        else
            ioerror = 0;

        /* Trace the results of CCW execution */
        if (unlikely( CCW_TRACING_ACTIVE( dev, tracethis )))
        {
#if DEBUG_PREFETCH
            if (!prefetch.seq)
            {
                char msgbuf[133];
                MSGBUF( msgbuf, "flags=%2.2X count=%d (%4.4X) "
                                "residual=%d (%4.4X) "
                                "more=%d "
                                "bufpos=%d",
                                flags,
                                count, count,
                                residual, residual,
                                more, bufpos );
                WRMSG( HHC01392, "D", msgbuf );
            }
#endif

            /* Display prefetch data */
            if (prefetch.seq)
            {
                /* Display prefetch table */
                DISPLAY_PREFETCH(&prefetch, ps, count, residual, more);

                /* Loop through prefetch table for CCW/IDAW display */
                for (ts = 0, prevccwaddr = 1;
                     ts < ps && ts < prefetch.seq;
                     ts++)
                {
                    if (prevccwaddr != prefetch.ccwaddr[ts])
                    {
                        prevccwaddr = prefetch.ccwaddr[ts];
                        if (ts)
                        {
                            /* Display CCW */
                            DISPLAY_CCW( &did_ccw_trace, dev,
                                         dev->mainstor +
                                           (prefetch.ccwaddr[ts] - 8),
                                         prefetch.dataaddr[ts],
                                         prefetch.datalen[ts],
                                         flags );

                        }
                    }
                    if (prefetch.idawtype[ts])
                        DISPLAY_IDAW(dev,
                                     prefetch.idawtype[ts],
                                     prefetch.idawflag[ts],
                                     prefetch.dataaddr[ts],
                                     prefetch.datalen[ts]);
                }

                /* Don't display data area on status */
                area[0] = 0x00;
            }

            /* Trace the read type CCW, UNLESS... the IDA/MIDA flag
               is on. When the IDA/MIDA flag is on for read type CCWs,
               the copy_iobuf function traces the CCW before it traces
               the IDA/MIDA.
            */
            if (1
                && !(flags & CCW_FLAGS_IDA)
                && !(flags & CCW_FLAGS_MIDAW)
            )
            {
                /* If we're tracing due to an I/O error, then the CCW
                   has already been traced further above, so we DON'T
                   want to do it here again!
                */
                if (!ioerror)
                    DISPLAY_CCW( &did_ccw_trace, dev, ccw, addr, count, flags );
            }

            ioerror = 0; // (reset flag)

            /* Display status and residual byte count */

            if (0
                || !dev->ccwopstrace          // (NOT tracing only specific CCWs?)
                ||  dev->ccwops[ dev->code ]  // (or IS one they're interested in?)
            )
            {
                if (dev->ccwtrace && sysblk.traceFILE)
                    tf_1312( dev, unitstat, chanstat, (BYTE) MIN( count, 16 ), residual, iobuf->data );
                else
                    // "%1d:%04X CHAN: stat %2.2X%2.2X, count %4.4X"
                    WRMSG( HHC01312, "I", LCSS_DEVNUM,
                        unitstat, chanstat, residual );
            }

            /* Display sense bytes if unit check is indicated */
            if (unitstat & CSW_UC)
            {
                if (dev->ccwtrace && sysblk.traceFILE)
                    tf_1313( dev );
                else
                {
                    register BYTE* sense = dev->sense;

                    // "%1d:%04X CHAN: sense %2.2X%2.2X%2.2X%2.2X ...
                    WRMSG( HHC01313, "I", LCSS_DEVNUM,
                            sense[ 0], sense[ 1], sense[ 2], sense[ 3],
                            sense[ 4], sense[ 5], sense[ 6], sense[ 7],
                            sense[ 8], sense[ 9], sense[10], sense[11],
                            sense[12], sense[13], sense[14], sense[15],
                            sense[16], sense[17], sense[18], sense[19],
                            sense[20], sense[21], sense[22], sense[23],
                            sense[24], sense[25], sense[26], sense[27],
                            sense[28], sense[29], sense[30], sense[31] );

                    if (sense[0] != 0 || sense[1] != 0)
                        DISPLAY_SENSE( dev );
                }
            }
        }

        /* Terminate the channel program if any unusual status */
        if (chanstat != 0
            || (unitstat & ~CSW_SM) != (CSW_CE | CSW_DE))
        {
            if (firstccw && !dev->is_immed && (dev->scsw.flag1 & SCSW1_I))
            {
                /* Set the zero condition-code flag in the SCSW */
                dev->scsw.flag1 |= SCSW1_Z;
                firstccw = 0;
            }
            chain = 0;
        }

        /* Increment CCW address if device returned status modifier  */
        /* SA22-7201-05:                                             */
        /*  p. 15-30, Command Chaining                               */
        else if ((unitstat & (CSW_DE | CSW_SM)) == (CSW_DE | CSW_SM))
            ccwaddr += 8;

        /* Update the chaining flags */
        dev->chained = flags & (CCW_FLAGS_CD | CCW_FLAGS_CC);

        /* Update the CCW sequence number unless data chained */
        if ((flags & CCW_FLAGS_CD) == 0)
        {
            dev->ccwseq++;

            /* Reset prefetch table and master skip data */
            dev->is_immed =
            skip_ccws =
            ps =
            prefetch.seq =
            prefetch.pos =
            bufpos = 0;
        }

        /* If Halt or Clear Pending, restart chaining to reset */
        if (dev->scsw.flag2 & (SCSW2_AC_HALT | SCSW2_AC_CLEAR))
            chain = 1;

    } /* end while(chain) */

    IODELAY(dev);

    /* Call the i/o end exit */
    if (dev->hnd->end) (dev->hnd->end) (dev);

    /* If we're shutting down, skip final sequence and just exit now */
    if (sysblk.shutdown)
        return execute_ccw_chain_fast_return( iobuf, &iobuf_initial, NULL );

    /* Final sequence MUST be performed with INTLOCK held to prevent
       I/O instructions (such as test_subchan) from proceeding before
       we can set our flags properly and queue the actual I/O interrupt.
    */
    OBTAIN_INTLOCK( NULL );
    {
        OBTAIN_DEVLOCK( dev );
        {
            /* Complete the subchannel status word */
            dev->scsw.flag3 &= ~(SCSW3_AC_SCHAC | SCSW3_AC_DEVAC | SCSW3_SC_INTER);
            dev->scsw.flag3 |= (SCSW3_SC_PRI | SCSW3_SC_SEC | SCSW3_SC_PEND);
            STORE_FW(dev->scsw.ccwaddr,ccwaddr);
            dev->scsw.unitstat = unitstat;
            dev->scsw.chanstat = chanstat;
            STORE_HW(dev->scsw.count,residual);

            /* Set alert status if terminated by any unusual condition */
            if (chanstat != 0 || unitstat != (CSW_CE | CSW_DE))
                dev->scsw.flag3 |= SCSW3_SC_ALERT;

            /* Clear the Extended Status Word (ESW) and set LPUM in Word 0,
               defaulting to a Format-1 ESW if no other action taken */
            memset (&dev->esw, 0,sizeof(ESW));
            dev->esw.lpum = 0x80;

            /* Format-0 if CDC, CCC OR IFCC */
            if (chanstat & (CSW_CDC | CSW_CCC | CSW_ICC))
            {
                /* SA22-7201-5:                                              */
                /*  p. 16-34, Field Validity Flags (FVF)                     */
                /*  p. 16-34 -- 16-35, Termination Code                      */
                /*  p. 16-35 -- 16-36, Sequence Code (SC)                    */
                /*                                                           */
                /*  TBD: Further refinement needed as only CDC is even close */
                /*       to being properly implemented.                      */
                dev->esw.scl2 = 0x78;   /* FVF: LPUM, termination code,      */
                                        /*      sequence code and device     */
                                        /*      status valid                 */
                /* Set data direction */
                if (!dev->is_immed)
                {
                    if (prefetch.seq)
                        dev->esw.scl2 |= 0x02;  /* Write operation */
                    else if (IS_CCW_RDBACK(dev->code))
                        dev->esw.scl2 |= 0x03;  /* Read backward */
                    else
                        dev->esw.scl2 |= 0x01;  /* Read */
                }
                dev->esw.scl3 = 0x45;   /* Bits 24-25: Termination Code      */
                                        /* Bits 29-31: Sequence Code         */
            }

            /* Clear the extended control word */
            memset (dev->ecw, 0, sizeof(dev->ecw));

            /* Return sense information if PMCW allows concurrent sense */
            if ((unitstat & CSW_UC) && (dev->pmcw.flag27 & PMCW27_S))
            {
                dev->scsw.flag1 |= SCSW1_E;
                dev->esw.erw0 |= ERW0_S;
                dev->esw.erw1 = (BYTE)((dev->numsense < (int)sizeof(dev->ecw)) ?
                                dev->numsense : (int)sizeof(dev->ecw));
                memcpy (dev->ecw, dev->sense, dev->esw.erw1 & ERW1_SCNT);
                memset (dev->sense, 0, sizeof(dev->sense));
                dev->sns_pending = 0;
            }

            /* Late notification. If halt or clear in process, go clear the  */
            /* mess.                                                         */
            if (dev->scsw.flag2 & (SCSW2_AC_HALT | SCSW2_AC_CLEAR))
            {
                U8 halt = (dev->scsw.flag2 & SCSW2_AC_HALT) ? TRUE : FALSE;
                RELEASE_DEVLOCK( dev );
                RELEASE_INTLOCK(NULL);
                if (halt)
                    goto execute_halt;
                goto execute_clear;
            }

            /* Present the interrupt and return */
            queue_io_interrupt_and_update_status_locked( dev, TRUE );
        }
        RELEASE_DEVLOCK( dev );
    }
    RELEASE_INTLOCK( NULL );

    return execute_ccw_chain_fast_return( iobuf, &iobuf_initial, NULL );

} /* end function execute_ccw_chain */


/*-------------------------------------------------------------------*/
/* TEST WHETHER INTERRUPTS ARE ENABLED FOR THE SPECIFIED DEVICE      */
/* When configured for S/370 channels, the PSW system mask and/or    */
/* the channel masks in control register 2 determine whether the     */
/* device is enabled.  When configured for the XA or ESA channel     */
/* subsystem, the interrupt subclass masks in control register 6     */
/* determine eligability; the PSW system mask is not tested, because */
/* the TPI instruction can operate with I/O interrupts masked off.   */
/* Returns non-zero if interrupts enabled, 0 if interrupts disabled. */
/*-------------------------------------------------------------------*/
/* I/O Assist:                                                       */
/* This routine must return:                                         */
/*                                                                   */
/* 0                   - In the case of no pending interrupt         */
/* SIE_NO_INTERCEPT    - For a non-intercepted I/O interrupt         */
/* SIE_INTERCEPT_IOINT - For an intercepted I/O interrupt            */
/* SIE_INTERCEPT_IOINTP- For a pending I/O interception              */
/*                                                                   */
/* SIE_INTERCEPT_IOINT may only be returned to a guest               */
/*-------------------------------------------------------------------*/
static INLINE int
ARCH_DEP(interrupt_enabled) (REGS *regs, DEVBLK *dev)
{
int     i;                              /* Interruption subclass     */

    /* Ignore this device if subchannel not valid */
    if (!(dev->pmcw.flag5 & PMCW5_V))
        return 0;

#if defined(_FEATURE_IO_ASSIST)
    /* For I/O Assist the zone must match the guest zone */
    if(SIE_MODE(regs) && regs->siebk->zone != dev->pmcw.zone)
        return 0;
#endif

#if defined(_FEATURE_IO_ASSIST)
    /* The interrupt interlock control bit must be on
       if not we must intercept */
    if(SIE_MODE(regs) && !(dev->pmcw.flag27 & PMCW27_I))
        return SIE_INTERCEPT_IOINT;
#endif

#ifdef FEATURE_S370_CHANNEL

#if defined(FEATURE_CHANNEL_SWITCHING)
    /* Is this device on a channel connected to this CPU? */
    if(
#if defined(_FEATURE_IO_ASSIST)
       !SIE_MODE(regs) &&
#endif
       regs->chanset != dev->chanset)
        return 0;
#endif /*defined(FEATURE_CHANNEL_SWITCHING)*/

    /* Isolate the channel number */
    i = dev->devnum >> 8;
    if (!ECMODE(&regs->psw) && i < 6)
    {
#if defined(_FEATURE_IO_ASSIST)
        /* We must always intercept in BC mode */
        if(SIE_MODE(regs))
            return SIE_INTERCEPT_IOINT;
#endif
        /* For BC mode channels 0-5, test system mask bits 0-5 */
        if ((regs->psw.sysmask & (0x80 >> i)) == 0)
            return 0;
    }
    else
    {
        /* For EC mode and channels 6-31, test system mask bit 6 */
        if ((regs->psw.sysmask & PSW_IOMASK) == 0)
            return 0;

        /* If I/O mask is enabled, test channel masks in CR2 */
        if (i > 31) i = 31;
        if ((CHANNEL_MASKS(regs) & (0x80000000 >> i)) == 0)
            return
#if defined(_FEATURE_IO_ASSIST)
                   SIE_MODE(regs) ? SIE_INTERCEPT_IOINTP :
#endif
                                                           0;
    }
#endif /*FEATURE_S370_CHANNEL*/

#ifdef FEATURE_CHANNEL_SUBSYSTEM
    /* Ignore this device if subchannel not enabled */
    if (!(dev->pmcw.flag5 & PMCW5_E))
        return 0;

    /* Isolate the interruption subclass */
    i =
#if defined(_FEATURE_IO_ASSIST)
        /* For I/O Assisted devices use the guest (V)ISC */
        SIE_MODE(regs) ? (dev->pmcw.flag25 & PMCW25_VISC) :
#endif
        ((dev->pmcw.flag4 & PMCW4_ISC) >> 3);

    /* Test interruption subclass mask bit in CR6 */
    if ((regs->CR_L(6) & (0x80000000 >> i)) == 0)
        return
#if defined(_FEATURE_IO_ASSIST)
                   SIE_MODE(regs) ? SIE_INTERCEPT_IOINTP :
#endif
                                                           0;
#endif /*FEATURE_CHANNEL_SUBSYSTEM*/

    /* Interrupts are enabled for this device */
    return SIE_NO_INTERCEPT;
} /* end function interrupt_enabled */


/*-------------------------------------------------------------------*/
/*                 PRESENT PENDING I/O INTERRUPT                     */
/*-------------------------------------------------------------------*/
/*                                                                   */
/* Finds a device with a pending condition for which an interrupt    */
/* is allowed by the CPU whose regs struct is passed as a parameter. */
/*                                                                   */
/* Clears the interrupt condition and returns the I/O address and    */
/* I/O interruption parameter (for channel subsystem) or the I/O     */
/* address and CSW (for S/370 channels).                             */
/*                                                                   */
/* The return value is the condition code for the TPI instruction:   */
/* 0 if no allowable pending interrupt exists, otherwise 1.          */
/*                                                                   */
/*-------------------------------------------------------------------*/
/*                                                                   */
/*   NOTE: Caller MUST hold the interrupt lock (sysblk.intlock).     */
/*   NOTE: This routine does NOT perform a PSW switch.               */
/*   NOTE: The CSW pointer is NULL in the case of TPI.               */
/*                                                                   */
/*-------------------------------------------------------------------*/
/*                         I/O ASSIST                                */
/*-------------------------------------------------------------------*/
/*                                                                   */
/* This routine MUST return:                                         */
/*                                                                   */
/*  0                   - In the case of no pending interrupt        */
/*                                                                   */
/*  SIE_NO_INTERCEPT    - For a non-intercepted I/O interrupt        */
/*                                                                   */
/*  SIE_INTERCEPT_IOINT - For an I/O interrupt which must intercept  */
/*                        (may ONLY be returned to a guest)          */
/*                                                                   */
/*  SIE_INTERCEPT_IOINTP- For a pending I/O interception             */
/*                                                                   */
/*-------------------------------------------------------------------*/
int ARCH_DEP( present_io_interrupt )( REGS* regs, U32* ioid, U32* ioparm,
                                      U32* iointid, BYTE* csw,
                                      DEVBLK** pdev )
{
IOINT  *io, *io2;                       /* -> I/O interrupt entry    */
DEVBLK *dev;                            /* -> Device control block   */
int     icode = 0;                      /* Intercept code            */
bool    dotsch = true;                  /* perform TSCH after int    */
                                        /* except for THININT        */

#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY) || defined(_FEATURE_IO_ASSIST)
#if defined(FEATURE_QDIO_THININT)
/* The 2 following variables are only used for QDIO Thin Interrupt Processing */
bool    saved_dotsch  = dotsch;
bool    PCI_dequeued  = false;
#endif
#endif

    UNREFERENCED_370(ioparm);
    UNREFERENCED_370(iointid);
#if defined(_FEATURE_IO_ASSIST)
    UNREFERENCED_390(iointid);
#endif
    UNREFERENCED_390(csw);
    UNREFERENCED_900(csw);

retry:

    /* Find a device with pending interrupt...

       Please remember that the device lock CANNOT be acquired
       while iointqlk is held. The iointqlk MUST be acquired AFTER
       the device lock is first acquired. Thus the retry logic.
    */
    dev = NULL;

    OBTAIN_IOINTQLK();
    {
        for (io = sysblk.iointq; io != NULL; io = io->next)
        {
            /* Can't present interrupt while TEST SUBCHANNEL required
             * (interrupt already presented for this device)
             */
            if (io->dev->tschpending)
                continue;

            /* Exit loop if enabled for interrupts from this device */
            if ((icode = ARCH_DEP( interrupt_enabled )( regs, io->dev ))

#if defined( _FEATURE_IO_ASSIST )
              && icode != SIE_INTERCEPT_IOINTP
#endif
            )
            {
                dev = io->dev;
                break;
            }

            /* See if another CPU can take this interrupt */
            {
                REGS *regs;
                CPU_BITMAP mask = sysblk.waiting_mask;
                CPU_BITMAP wake;
                int i;

                /* If any CPUs are waiting, isolate to subgroup enabled for
                 * I/O interrupts.
                 */
                if (mask)
                {
                    wake = mask;

                    /* Turn off wake mask bits for waiting CPUs that aren't
                     * enabled for I/O interrupts for the device.
                     */
                    for (i=0; mask; mask >>= 1, ++i)
                    {
                        if (mask & 1)
                        {
                            regs = sysblk.regs[i];

                            if (!ARCH_DEP( interrupt_enabled )( regs, io->dev ))
                                wake ^= regs->cpubit;
                        }
                    }

                    /* Wakeup the LRU waiting CPU enabled for I/O
                     * interrupts.
                     */
                    WAKEUP_CPU_MASK( wake );
                }
            }

        } /* end for(io) */

#if defined( _FEATURE_IO_ASSIST )
        /* In the case of I/O assist, do a rescan, to see
           if there are any devices with pending subclasses
           for which we are not enabled, if so cause an
           interception.
        */
        if (io == NULL && SIE_MODE( regs ))
        {
            /* Find a device with a pending interrupt, regardless
               of the interrupt subclass mask
            */
            ASSERT( dev == NULL );

            for (io = sysblk.iointq; io != NULL; io = io->next)
            {
                /* Exit loop if pending interrupts from this device */
                if (ARCH_DEP( interrupt_enabled )( regs, io->dev ))
                {
                    dev = io->dev;
                    break;
                }
            } /* end for(io) */
        }
#endif
        /* If no interrupt pending, or no device, exit with
         * condition code 0
         */
        if (io == NULL || dev == NULL)
        {
            /* Pass back pointer to device block for device with interrupt */
            *pdev = dev;

            if (dev != NULL)
                subchannel_interrupt_queue_cleanup( dev );

            UPDATE_IC_IOPENDING_QLOCKED();

            RELEASE_IOINTQLK();
            return 0;
        }
    }
    RELEASE_IOINTQLK();

    /* Obtain device lock for device with interrupt */
    OBTAIN_DEVLOCK( dev );
    {
        /* Pass back pointer to device block for device with interrupt */
        *pdev = dev;

        /* Verify that the interrupt for this device still exists and that
           TEST SUBCHANNEL has to be issued to clear an existing interrupt.
         */
        OBTAIN_IOINTQLK();
        {
            for (io2 = sysblk.iointq; io2 != NULL && io2 != io; io2 = io2->next);

            if (io2 == NULL || dev->tschpending)
            {
                /* Our interrupt was dequeued; retry */
                RELEASE_IOINTQLK();
                RELEASE_DEVLOCK( dev );
                goto retry;
            }

            /* Extract the I/O address and interrupt parameter */
            *ioid = (dev->ssid << 16) | dev->subchan;
            FETCH_FW( *ioparm,dev->pmcw.intparm );

#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY ) || defined( _FEATURE_IO_ASSIST )
#if defined( FEATURE_QDIO_THININT )
            if (unlikely( FACILITY_ENABLED( HERC_QDIO_THININT, regs )
                && (dev->pciscsw.flag2 & SCSW2_Q) && dev->qdio.thinint ))
            {
                saved_dotsch = dotsch;
                PCI_dequeued = false;

                dotsch = false;  /* Do not require TSCH after INT */

                *iointid = 0x80000000
                     | (
#if defined( _FEATURE_IO_ASSIST )
                        /* For I/O Assisted devices use (V)ISC */
                        (SIE_MODE(regs)) ?
                          (icode == SIE_NO_INTERCEPT) ?
                            ((dev->pmcw.flag25 & PMCW25_VISC) << 27) :
                            ((dev->pmcw.flag25 & PMCW25_VISC) << 27)
                              | (dev->pmcw.zone << 16)
                              | ((dev->pmcw.flag27 & PMCW27_I) << 8) :
#endif
                        ((dev->pmcw.flag4 & PMCW4_ISC) << 24)
                          | ((dev->pmcw.flag25 & PMCW25_TYPE) << 7)
#if defined( _FEATURE_IO_ASSIST )
                          | (dev->pmcw.zone << 16)
                          | ((dev->pmcw.flag27 & PMCW27_I) << 8)
#endif
                ); // *iointid =

                if (!SIE_MODE(regs) || icode != SIE_INTERCEPT_IOINTP)
                {
                    /* Dequeue the interrupt */
                    PCI_dequeued = DEQUEUE_IO_INTERRUPT_QLOCKED( &dev->pciioint ) == 0 ? true : false;

                    if (!PCI_dequeued)
                        dotsch = saved_dotsch; // (restore)
                    else
                    {
                        /* Clear the pending PCI status */
                        dev->pciscsw.flag2 &= ~(SCSW2_FC | SCSW2_AC);
                        dev->pciscsw.flag3 &= ~(SCSW3_SC);
                    }
                }
                else
                    PCI_dequeued = true; // (keep same logic as before!)
            }

            /* If no PCI interrupt dequeued, then dequeue a normal interrupt */
            if (!PCI_dequeued)
#endif /*defined( FEATURE_QDIO_THININT )*/

                *iointid = (
#if defined(_FEATURE_IO_ASSIST)
                            /* For I/O Assisted devices use (V)ISC */
                            (SIE_MODE(regs)) ?
                              (icode == SIE_NO_INTERCEPT) ?
                                ((dev->pmcw.flag25 & PMCW25_VISC) << 27) :
                                ((dev->pmcw.flag25 & PMCW25_VISC) << 27)
                                  | (dev->pmcw.zone << 16)
                                  | ((dev->pmcw.flag27 & PMCW27_I) << 8) :
#endif
                             ((dev->pmcw.flag4 & PMCW4_ISC) << 24)
                               | ((dev->pmcw.flag25 & PMCW25_TYPE) << 7)
#if defined(_FEATURE_IO_ASSIST)
                               | (dev->pmcw.zone << 16)
                               | ((dev->pmcw.flag27 & PMCW27_I) << 8)
#endif
                ); // *iointid =
#endif /*defined( FEATURE_001_ZARCH_INSTALLED_FACILITY ) || defined( _FEATURE_IO_ASSIST ) */

#if defined( _FEATURE_IO_ASSIST )
            /* Do not drain pending interrupts
               on intercept due to zero ISC mask
             */
            if(!SIE_MODE(regs) || icode != SIE_INTERCEPT_IOINTP)
#endif
            {
                if (!SIE_MODE(regs) || icode != SIE_NO_INTERCEPT)
                    dev->pmcw.flag27 &= ~PMCW27_I;

                /* Dequeue the interrupt */
                DEQUEUE_IO_INTERRUPT_QLOCKED( io );
            }

            /* TEST SUBCHANNEL is now required to clear the interrupt */
            dev->tschpending = dotsch;

            /* Perform additional architecture post processing and cleanup */
            switch (sysblk.arch_mode)
            {
                case ARCH_370_IDX:
                {
                    IOINT*  ioint;              /* -> I/O interrupt          */
                    IRB     irb;                /* -> IRB                    */
                    SCSW*   scsw;               /* -> SCSW                   */
                    int     cc;                 /* Condition code ignored    */

                    /* Extract the I/O address and CSW */
                    *ioid = dev->devnum;

                    /* Perform core of TEST SUBCHANNEL work and store CSW */
                    cc = test_subchan_locked( regs, dev, &irb, &ioint, &scsw );
                    store_scsw_as_csw( regs, scsw );
                    break;
                }

                case ARCH_390_IDX: break;
                case ARCH_900_IDX: break;
                default: CRASH();
            }

            subchannel_interrupt_queue_cleanup( dev );
            UPDATE_IC_IOPENDING_QLOCKED();
        }
        RELEASE_IOINTQLK();

#if DEBUG_SCSW
        if (unlikely(dev->ccwtrace))
        {
            SCSW* scsw;   /* (selected SCSW) */

                 if (dev->pciscsw.flag3  & SCSW3_SC_PEND) scsw = &dev->pciscsw;
            else if (dev->scsw.flag3     & SCSW3_SC_PEND) scsw = &dev->scsw;
         /* else if (dev->attnscsw.flag3 & SCSW3_SC_PEND) scsw = &dev->attnscsw; */
            else scsw = NULL;

            /* Check interrupt validity */
            if (1
                && scsw != NULL
                && !(scsw->flag2 & (SCSW2_FC | SCSW2_AC))
                && !(scsw->flag3 & SCSW3_AC)
            )
            {
                WRMSG( HHC01393, "E", "  CHAN: Invalid SCSW presentation" );
                DISPLAY_SCSW( dev, *scsw );
            }
        }
#endif /*DEBUG_SCSW*/
    }
    RELEASE_DEVLOCK( dev );

    /* Exit with condition code indicating queued interrupt cleared */
    return icode;

} /* end function present_io_interrupt */


/*-------------------------------------------------------------------*/
/* present_zone_io_interrupt                                         */
/*-------------------------------------------------------------------*/
#if defined(_FEATURE_IO_ASSIST)
int
ARCH_DEP(present_zone_io_interrupt) (U32 *ioid, U32 *ioparm,
                                     U32 *iointid, BYTE zone)
{
IOINT  *io;                             /* -> I/O interrupt entry    */
DEVBLK *dev;                            /* -> Device control block   */
typedef struct _DEVLIST {               /* list of device block ptrs */
    struct _DEVLIST *next;              /* next list entry or NULL   */
    DEVBLK          *dev;               /* DEVBLK in requested zone  */
    U16              ssid;              /* Subsystem ID incl. lcssid */
    U16              subchan;           /* Subchannel number         */
    FWORD            intparm;           /* Interruption parameter    */
    int              visc;              /* Guest Interrupt Subclass  */
} DEVLIST;
DEVLIST *pDEVLIST, *pPrevDEVLIST = NULL;/* (work)                    */
DEVLIST *pZoneDevs = NULL;              /* devices in requested zone */

    /* Gather devices within our zone with pending interrupt flagged */
    for (dev = sysblk.firstdev; dev; dev = dev->nextdev)
    {
        /* Skip "devices" that don't actually exist */
        if (!IS_DEV(dev))
            continue;

        OBTAIN_DEVLOCK( dev );
        {
            if (1
                /* Subchannel valid and enabled */
                && ((dev->pmcw.flag5 & (PMCW5_E | PMCW5_V)) == (PMCW5_E | PMCW5_V))
                /* Within requested zone */
                && (dev->pmcw.zone == zone)
                /* Pending interrupt flagged */
                && ((dev->scsw.flag3 | dev->pciscsw.flag3) & SCSW3_SC_PEND)
            )
            {
                /* (save this device for further scrutiny) */
                pDEVLIST          = (DEVLIST *)malloc( sizeof(DEVLIST) );
                pDEVLIST->next    = NULL;
                pDEVLIST->dev     = dev;
                pDEVLIST->ssid    = dev->ssid;
                pDEVLIST->subchan = dev->subchan;
//              pDEVLIST->intparm = dev->pmcw.intparm;
                memcpy( pDEVLIST->intparm, dev->pmcw.intparm, sizeof(pDEVLIST->intparm) );
                pDEVLIST->visc    = (dev->pmcw.flag25 & PMCW25_VISC);

                if (!pZoneDevs)
                    pZoneDevs = pDEVLIST;

                if (pPrevDEVLIST)
                    pPrevDEVLIST->next = pDEVLIST;

                pPrevDEVLIST = pDEVLIST;
            }
        }
        RELEASE_DEVLOCK( dev );
    }

    /* Exit with condition code 0 if no devices
       within our zone with a pending interrupt */
    if (!pZoneDevs)
        return 0;

    /* Remove from our list those devices
       without a pending interrupt queued */
    OBTAIN_IOINTQLK();
    {
        for (pDEVLIST = pZoneDevs, pPrevDEVLIST = NULL; pDEVLIST;)
        {
            /* Search interrupt queue for this device */
            for (io = sysblk.iointq; io != NULL && io->dev != pDEVLIST->dev; io = io->next);

            /* Is interrupt queued for this device? */
            if (io == NULL)
            {
                /* No, remove it from our list */
                if (!pPrevDEVLIST)
                {
                    ASSERT(pDEVLIST == pZoneDevs);
                    pZoneDevs = pDEVLIST->next;
                    free(pDEVLIST);
                    pDEVLIST = pZoneDevs;
                }
                else
                {
                    pPrevDEVLIST->next = pDEVLIST->next;
                    free(pDEVLIST);
                    pDEVLIST = pPrevDEVLIST->next;
                }
            }
            else
            {
                /* Yes, go on to next list entry */
                pPrevDEVLIST = pDEVLIST;
                pDEVLIST = pDEVLIST->next;
            }
        }
    }
    RELEASE_IOINTQLK();

    /* If no devices remain, exit with condition code 0 */
    if (!pZoneDevs)
        return 0;

    /* Extract the I/O address and interrupt parameter
       for the first pending subchannel */
    dev = pZoneDevs->dev;
    *ioid = (pZoneDevs->ssid << 16) | pZoneDevs->subchan;
    FETCH_FW(*ioparm,pZoneDevs->intparm);
    *iointid = (0x80000000 >> pZoneDevs->visc) | (zone << 16);
    pDEVLIST = pZoneDevs->next;
    free (pZoneDevs);

    /* Find all other pending subclasses */
    while (pDEVLIST)
    {
        *iointid |= (0x80000000 >> pDEVLIST->visc);
        pPrevDEVLIST = pDEVLIST;
        pDEVLIST = pDEVLIST->next;
        free (pPrevDEVLIST);
    }

    /* Exit with condition code indicating interrupt pending */
    return 1;

} /* end function present_zone_io_interrupt */
#endif

/*-------------------------------------------------------------------*/
/*            END OF PRIMARY CHANNEL PROCESSING CODE                 */
/*-------------------------------------------------------------------*/

#if !defined(_GEN_ARCH)

#if defined(_ARCH_NUM_1)
 #define  _GEN_ARCH _ARCH_NUM_1
 #include "channel.c"
#endif

#if defined(_ARCH_NUM_2)
 #undef   _GEN_ARCH
 #define  _GEN_ARCH _ARCH_NUM_2
 #include "channel.c"
#endif

/*-------------------------------------------------------------------*/
/*            Exported non-ARCH_DEP functions go here                */
/*-------------------------------------------------------------------*/

DLL_EXPORT int device_attention (DEVBLK *dev, BYTE unitstat)
{
    switch(sysblk.arch_mode)
    {
#if defined(_370)
        case ARCH_370_IDX:
            /* Do NOT raise if initial power-on state */
            if (!INITIAL_POWERON_370())
                return s370_device_attention(dev, unitstat);
            return 3;   /* subchannel is not valid or not enabled */
#endif
#if defined(_390)
        case ARCH_390_IDX: return s390_device_attention(dev, unitstat);
#endif
#if defined(_900)
        case ARCH_900_IDX: return z900_device_attention(dev, unitstat);
#endif
        default: CRASH();
    }
    return 3;   /* subchannel is not valid or not enabled */
}

void call_execute_ccw_chain (int arch_mode, void* pDevBlk)
{
    switch (arch_mode)
    {
#if defined(_370)
        case ARCH_370_IDX: s370_execute_ccw_chain((DEVBLK*)pDevBlk); break;
#endif
#if defined(_390)
        case ARCH_390_IDX: s390_execute_ccw_chain((DEVBLK*)pDevBlk); break;
#endif
#if defined(_900)
        case ARCH_900_IDX: z900_execute_ccw_chain((DEVBLK*)pDevBlk); break;
#endif
        default: CRASH();
    }
}

/*-------------------------------------------------------------------*/
/*  Functions to queue/dequeue device on I/O interrupt queue.        */
/*  sysblk.iointqlk is ALWAYS needed to examine sysblk.iointq        */
/*-------------------------------------------------------------------*/

DLL_EXPORT void Queue_IO_Interrupt( IOINT* io, U8 clrbsy, const char* location )
{
    OBTAIN_IOINTQLK();
    {
        Queue_IO_Interrupt_QLocked( io, clrbsy, location );
    }
    RELEASE_IOINTQLK();
}

DLL_EXPORT void Queue_IO_Interrupt_QLocked( IOINT* io, U8 clrbsy, const char* location )
{
IOINT* prev;

    UNREFERENCED( location );

    /* Check if an I/O interrupt is already queued for this device */
    for
    (
        prev = (IOINT*) &sysblk.iointq;
        (1
            && prev->next != NULL
            && prev->next != io
            && prev->next->priority >= io->dev->priority
        );
        prev = prev->next
    )
    {
        ;   /* (do nothing, we are only searching) */
    }

    /* If no interrupt in queue for this device then add one */
    if (prev->next != io)
    {
        io->next     = prev->next;
        prev->next   = io;
        io->priority = io->dev->priority;
    }

    /* Update device flags according to interrupt type */
         if (io->pending)     io->dev->pending     = 1;
    else if (io->pcipending)  io->dev->pcipending  = 1;
    else if (io->attnpending) io->dev->attnpending = 1;

    /* While I/O interrupt queue is locked
       clear subchannel busy if asked to do so */
    if (clrbsy)
    {
        io->dev->scsw.flag3 &= ~( SCSW3_AC_SCHAC |
                                  SCSW3_AC_DEVAC |
                                  SCSW3_SC_INTER );
        io->dev->startpending = 0;
        io->dev->busy = 0;
    }

#if 0 // (debugging example)
    if (sysblk.fishtest && io->dev->devnum == 0x0604)
        TRACE("+++ %s interrupt Queued for %4.4X @ %s\n",
            io->pending     ? "Normal" :
            io->attnpending ? "ATTN"   :
            io->pcipending  ? "PCI"    : "UNKNOWN!",
            io->dev->devnum, location );
#endif
}

DLL_EXPORT int Dequeue_IO_Interrupt( IOINT* io, const char* location )
{
int rc;
    OBTAIN_IOINTQLK();
    {
        rc = Dequeue_IO_Interrupt_QLocked( io, location );
    }
    RELEASE_IOINTQLK();
    return rc;
}

DLL_EXPORT int Dequeue_IO_Interrupt_QLocked( IOINT* io, const char* location )
{
IOINT* prev;
int rc = -1;        /* No I/O interrupts were queued for this device */

    UNREFERENCED( location );

    /* Search the I/O interrupt queue for an interrupt
       for this device and dequeue it if one is found. */
    for
    (
        prev = (IOINT*) &sysblk.iointq;
        prev->next != NULL;
        prev = prev->next
    )
    {
        /* Is this I/O interrupt for requested device? */
        if (prev->next == io)
        {
            /* Yes, dequeue the I/O interrupt and update
               device flags according to interrupt type. */
            prev->next = io->next;
                 if (io->pending)     io->dev->pending     = 0;
            else if (io->pcipending)  io->dev->pcipending  = 0;
            else if (io->attnpending) io->dev->attnpending = 0;

            rc = 0;   /* I/O interrupt successfully dequeued */
            break;    /* I/O interrupt successfully dequeued */
        }
    }
#if 0 // (debugging example)
    if (sysblk.fishtest && io->dev->devnum == 0x0604)
        TRACE("+++ %s interrupt %sDEQueued for %4.4X @ %s\n",
            io->pending     ? "Normal" :
            io->attnpending ? "ATTN"   :
            io->pcipending  ? "PCI"    : "UNKNOWN!",
            rc == 0 ? "" : "NOT ",
            io->dev->devnum, location );
#endif
    return rc;  /* rc=0: interrupt dequeued, rc=-1: NOTHING dequeued */
}

/*-------------------------------------------------------------------*/
/*  NOTE: sysblk.iointqlk needed to examine sysblk.iointq.           */
/*  sysblk.intlock (which MUST be held before calling these          */
/*  functions) needed in order to set/reset IC_IOPENDING flag.       */
/*-------------------------------------------------------------------*/

DLL_EXPORT void Update_IC_IOPENDING()
{
    OBTAIN_IOINTQLK();
    {
        Update_IC_IOPENDING_QLocked();
    }
    RELEASE_IOINTQLK();
}

DLL_EXPORT void Update_IC_IOPENDING_QLocked()
{
    if (!sysblk.iointq)
    {
        OFF_IC_IOPENDING;
    }
    else
    {
        ON_IC_IOPENDING;
    }
}

#endif /*!defined(_GEN_ARCH)*/
