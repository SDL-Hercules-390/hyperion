/* CONFIG.C     (C) Copyright Jan Jaeger, 2000-2012                  */
/*              (C) and others 2013-2023                             */
/*              Device and Storage configuration functions           */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* The original configuration builder is now called bldcfg.c         */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

DISABLE_GCC_UNUSED_FUNCTION_WARNING;

#define _CONFIG_C_
#define _HENGINE_DLL_

#include "hercules.h"
#include "opcode.h"
#include "chsc.h"
#include "cckddasd.h"

/*-------------------------------------------------------------------*/
/*   ARCH_DEP section: compiled multiple times, once for each arch.  */
/*-------------------------------------------------------------------*/

// (we have no ARCH_DEP code in this module)

/*-------------------------------------------------------------------*/
/*          (delineates ARCH_DEP from non-arch_dep)                  */
/*-------------------------------------------------------------------*/

#if !defined( _GEN_ARCH )

#if defined(             _ARCH_NUM_1 )
 #define      _GEN_ARCH  _ARCH_NUM_1
 #include "config.c"
 #undef       _GEN_ARCH
#endif

#if defined(             _ARCH_NUM_2 )
 #define      _GEN_ARCH  _ARCH_NUM_2
 #include "config.c"
 #undef       _GEN_ARCH
#endif

/*-------------------------------------------------------------------*/
/*          (delineates ARCH_DEP from non-arch_dep)                  */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/*  non-ARCH_DEP section: compiled only ONCE after last arch built   */
/*-------------------------------------------------------------------*/
/*  Note: the last architecture has been built so the normal non-    */
/*  underscore FEATURE values are now #defined according to the      */
/*  LAST built architecture just built (usually zarch = 900). This   */
/*  means from this point onward (to the end of file) you should     */
/*  ONLY be testing the underscore _FEATURE values to see if the     */
/*  given feature was defined for *ANY* of the build architectures.  */
/*-------------------------------------------------------------------*/

/*********************************************************************/
/*                  IMPORTANT PROGRAMMING NOTE                       */
/*********************************************************************/
/*                                                                   */
/* It is CRITICALLY IMPORTANT to not use any architecture dependent  */
/* macros anywhere in any of your non-arch_dep functions. This means */
/* you CANNOT use GREG, RADR, VADR, etc. anywhere in your function,  */
/* nor can you call "ARCH_DEP(func)(args)" anywhere in your code!    */
/*                                                                   */
/* Basically you MUST NOT use any architecture dependent macro that  */
/* is #defined in the "feature.h" header.  If you you need to use    */
/* any of them, then your function MUST be an "ARCH_DEP" function    */
/* that is placed within the ARCH_DEP section at the beginning of    */
/* this module where it can be compiled multiple times, once for     */
/* each of the supported architectures so the macro gets #defined    */
/* to its proper value for the architecture. YOU HAVE BEEN WARNED.   */
/*                                                                   */
/*********************************************************************/

#if defined( HAVE_MLOCKALL )
/*-------------------------------------------------------------------*/
/*        configure_memlock - lock or unlock storage                 */
/*-------------------------------------------------------------------*/
int configure_memlock( int flags )
{
int rc;

    if(flags)
        rc = mlockall( MCL_CURRENT | MCL_FUTURE );
    else
        rc = munlockall();

    return rc ? errno : 0;
}
#endif /*defined(HAVE_MLOCKALL)*/

/*-------------------------------------------------------------------*/
/*                    configure_region_reloc                         */
/*-------------------------------------------------------------------*/
static void configure_region_reloc()
{
    DEVBLK*  dev;
    int      i;

#if defined(_FEATURE_REGION_RELOCATE)

    /* Initialize base zone storage view (SIE compat) */
    for (i=0; i < FEATURE_SIE_MAXZONES; i++)
    {
        sysblk.zpb[i].mso = 0;
        sysblk.zpb[i].msl = (sysblk.mainsize - 1) >> SHIFT_MEGABYTE;

        if (sysblk.xpndsize)
        {
            sysblk.zpb[i].eso = 0;
            sysblk.zpb[i].esl = ((size_t)sysblk.xpndsize * XSTORE_PAGESIZE - 1) >> SHIFT_MEGABYTE;
        }
        else
        {
            sysblk.zpb[i].eso = -1;
            sysblk.zpb[i].esl = -1;
        }
    }
#endif

    /* Relocate storage for all devices */
    for (dev = sysblk.firstdev; dev; dev = dev->nextdev)
    {
        dev->mainstor = sysblk.mainstor;
        dev->storkeys = sysblk.storkeys;
        dev->mainlim  = sysblk.mainsize ? (sysblk.mainsize - 1) : 0;
    }

    /* Relocate storage for all online cpus */
    for (i=0; i < sysblk.maxcpu; i++)
        if (IS_CPU_ONLINE( i ))
        {
            sysblk.regs[i]->storkeys = sysblk.storkeys;
            sysblk.regs[i]->mainstor = sysblk.mainstor;
            sysblk.regs[i]->mainlim  = sysblk.mainsize ? (sysblk.mainsize - 1) : 0;
        }
}

/*-------------------------------------------------------------------*/
/* configure_memfree - adjust minimum emulator free storage amount   */
/*-------------------------------------------------------------------*/
static U64  config_mfree  = (200 << SHIFT_MEGABYTE);

int configure_memfree( int mfree )
{
    if (mfree < 0)
        return config_mfree >> SHIFT_MEGABYTE;

    config_mfree = ((U64)mfree) << SHIFT_MEGABYTE;
    return 0;
}

/*-------------------------------------------------------------------*/
/*  adjust_mainsize   --   range check MAINSIZE by architecture      */
/*-------------------------------------------------------------------*/
U64 adjust_mainsize( int archnum, U64 mainsize )
{
    static const U64 minmax_mainsize[ NUM_GEN_ARCHS ][2] =
    {
#if defined(       _ARCH_NUM_0 )
  #if       370 == _ARCH_NUM_0
      { MIN_370_MAINSIZE_BYTES,     // (MIN_ARCH_MAINSIZE_BYTES)
        MAX_370_MAINSIZE_BYTES },   // (MAX_ARCH_MAINSIZE_BYTES)

  #elif     390 == _ARCH_NUM_0
      { MIN_390_MAINSIZE_BYTES,     // etc...
        MAX_390_MAINSIZE_BYTES },   // etc...

  #else //  900 == _ARCH_NUM_0
      { MIN_900_MAINSIZE_BYTES,     // etc...
        MAX_900_MAINSIZE_BYTES },   // etc...
  #endif
#endif
#if defined(       _ARCH_NUM_1 )
  #if       370 == _ARCH_NUM_1
      { MIN_370_MAINSIZE_BYTES,     // etc...
        MAX_370_MAINSIZE_BYTES },   // etc...

  #elif     390 == _ARCH_NUM_1
      { MIN_390_MAINSIZE_BYTES,     // etc...
        MAX_390_MAINSIZE_BYTES },   // etc...

  #else //  900 == _ARCH_NUM_1
      { MIN_900_MAINSIZE_BYTES,     // etc...
        MAX_900_MAINSIZE_BYTES },   // etc...
  #endif
#endif
#if defined(       _ARCH_NUM_2 )
  #if       370 == _ARCH_NUM_2
      { MIN_370_MAINSIZE_BYTES,     // etc...
        MAX_370_MAINSIZE_BYTES },   // etc...

  #elif     390 == _ARCH_NUM_2
      { MIN_390_MAINSIZE_BYTES,     // etc...
        MAX_390_MAINSIZE_BYTES },   // etc...

  #else //  900 == _ARCH_NUM_2
      { MIN_900_MAINSIZE_BYTES,     // etc...
        MAX_900_MAINSIZE_BYTES },   // etc...
  #endif
#endif
    };

    if (mainsize < minmax_mainsize[ archnum ][ MIN_ARCH_MAINSIZE_BYTES ])
        mainsize = minmax_mainsize[ archnum ][ MIN_ARCH_MAINSIZE_BYTES ];

    if (mainsize > minmax_mainsize[ archnum ][ MAX_ARCH_MAINSIZE_BYTES ])
        mainsize = minmax_mainsize[ archnum ][ MAX_ARCH_MAINSIZE_BYTES ];

    /* Special case: if no CPUs then no storage is needed */
    if (sysblk.maxcpu <= 0)
        mainsize = 0;

    return mainsize;
}

PUSH_GCC_WARNINGS()
DISABLE_GCC_WARNING( "-Wpointer-to-int-cast" )
DISABLE_GCC_WARNING( "-Wint-to-pointer-cast" )

/*-------------------------------------------------------------------*/
/* configure_storage - configure MAIN storage                        */
/*-------------------------------------------------------------------*/

static U64    config_allocmsize  = 0;
static BYTE*  config_allocmaddr  = NULL;

int configure_storage( U64 mainsize /* number of 4K pages */ )
{
    BYTE*  mainstor;
    BYTE*  storkeys;
    BYTE*  dofree = NULL;
    char*  mfree  = NULL;
    U64    storsize;
    U32    skeysize;

    /* Ensure all CPUs have been stopped */
    if (are_any_cpus_started())
        return HERRCPUONL;

    /* Release storage and return if deconfiguring */
    if (mainsize == ~0ULL)
    {
        if (config_allocmaddr)
            free( config_allocmaddr );

        sysblk.storkeys = 0;
        sysblk.mainstor = 0;
        sysblk.mainsize = 0;

        config_allocmsize = 0;
        config_allocmaddr = NULL;

        return 0;
    }

    /* Round requested storage size to architectural segment boundary
     *
     *    ARCH_370_IDX  1   4K page
     *    ARCH_390_IDX  256 4K pages (or 1M)
     *    ARCH_900_IDX  256 4K pages (or 1M)
     */
    if (mainsize)
    {
        if (sysblk.arch_mode == ARCH_370_IDX)
            storsize = MAX( 1, mainsize );
        else
            storsize = (mainsize + 255) & ~255ULL;
        mainsize = storsize;
    }
    else
    {
        /* Adjust zero storage case for a subsystem/device server
         * (MAXCPU 0), allocating minimum storage of 4K.
         */
        storsize = 1;
    }

    /* Storage key array size rounded to next page boundary */
    switch (_STORKEY_ARRAY_UNITSIZE)
    {
        case _2K:
            /* We need one storekey byte for every 2K page instead
               of only one byte for every 4K page, so we need twice
               as many pages as normal for our storage key array.
            */
            skeysize = storsize << (SHIFT_4K - SHIFT_2K);
            break;
        case _4K:
            skeysize = storsize;
            break;
    }

    /* Calculate number of 4K pages we will need to allocate */
    skeysize += (_4K-1);
    skeysize >>= SHIFT_4K;

    /* Add number of pages needed for our storage key array */
    storsize += skeysize;

    /* New memory is obtained only if the requested and calculated size
     * is larger than the last allocated size, or if the request is for
     * less than 2M of memory.
     */
    if (0
        || (storsize > config_allocmsize)
        || (storsize < config_allocmsize && mainsize <= DEF_MAINSIZE_PAGES)
    )
    {
        if (config_mfree && mainsize > DEF_MAINSIZE_PAGES)
            mfree = malloc( config_mfree );

        /* Obtain storage with pagesize hint for cleanest allocation */
        storkeys = calloc( (size_t)(storsize + 1), _4K );

        if (mfree)
            free( mfree );

        if (!storkeys)
        {
            char buf[64];
            char memsize[64];

            sysblk.main_clear = 0;

            fmt_memsize_KB( mainsize << (SHIFT_4K - SHIFT_1K), memsize, sizeof( memsize ));
            MSGBUF( buf, "configure_storage( %s )", memsize );

            // "Error in function %s: %s"
            WRMSG( HHC01430, "S", buf, strerror( errno ));
            return -1;
        }

        /* Previously allocated storage to be freed, update actual
         * storage pointers and adjust new storage to page boundary.
         */
        dofree = config_allocmaddr;

        config_allocmsize = storsize;
        config_allocmaddr = storkeys;

        sysblk.main_clear = 1;

        storkeys = (BYTE*)(((U64)storkeys + (_4K-1)) & ~0x0FFFULL);
    }
    else
    {
        storkeys = sysblk.storkeys;
        sysblk.main_clear = 0;
        dofree = NULL;
    }

    /* MAINSTOR is located on the page boundary just beyond the
       storage key array (which is already on a page boundary).
     */
    mainstor = (BYTE*)((U64)(storkeys + (skeysize << SHIFT_4K)));

    /* Update SYSBLK... */
    sysblk.storkeys = storkeys;
    sysblk.mainstor = mainstor;
    sysblk.mainsize = mainsize << SHIFT_4K;

    /*  Free previously allocated storage if no longer needed
     *
     *  FIXME: The storage ordering further limits the amount of storage
     *         that may be allocated following the initial storage
     *         allocation.
     */
    if (dofree)
        free( dofree );

    /* Initial power-on reset for main storage */
    storage_clear();  /* only clears if needed */

#if 0   /* DEBUG-JJ - 20/03/2000 */

    /* Mark selected frames invalid for debugging purposes */
    for (i = 64; i < (sysblk.mainsize / _STORKEY_ARRAY_UNITSIZE); i += 2)
    {
        if (i < (sysblk.mainsize / _STORKEY_ARRAY_UNITSIZE) - 64)
            sysblk.storkeys[i]   = STORKEY_BADFRM;
        else
            sysblk.storkeys[i++] = STORKEY_BADFRM;
    }

#endif

#if 1 // FIXME: The below is a kludge that will need to be cleaned up at some point in time

    /* Initialize dummy regs.
     * Dummy regs are used by the panel or gui when the target cpu
     * (sysblk.pcpu) is not configured (ie cpu_thread not started).
     */
    sysblk.dummyregs.mainstor = sysblk.mainstor;
    sysblk.dummyregs.psa = (PSA_3XX*)sysblk.mainstor;
    sysblk.dummyregs.storkeys = sysblk.storkeys;
    sysblk.dummyregs.mainlim = sysblk.mainsize ? (sysblk.mainsize - 1) : 0;
    sysblk.dummyregs.dummy = 1;

    initial_cpu_reset( &sysblk.dummyregs );

    sysblk.dummyregs.arch_mode = sysblk.arch_mode;
    sysblk.dummyregs.hostregs = &sysblk.dummyregs;

#endif // kludge?

    configure_region_reloc();
    initial_cpu_reset_all();

    return 0;
}

/*-------------------------------------------------------------------*/
/* configure_xstorage - configure EXPANDED storage                   */
/*-------------------------------------------------------------------*/

static U64    config_allocxsize  = 0;
static BYTE*  config_allocxaddr  = NULL;

int configure_xstorage( U64 xpndsize )
{
#ifdef _FEATURE_EXPANDED_STORAGE

    BYTE*  xpndstor;
    BYTE*  dofree = NULL;
    char*  mfree  = NULL;

    /* Ensure all CPUs have been stopped */
    if (are_any_cpus_started())
        return HERRCPUONL;

    /* Release storage and return if zero or deconfiguring */
    if (!xpndsize || xpndsize == ~0ULL)
    {
        if (config_allocxaddr)
            free(config_allocxaddr);

        sysblk.xpndsize = 0;
        sysblk.xpndstor = 0;

        config_allocxsize = 0;
        config_allocxaddr = NULL;

        return 0;
    }

    /* New memory is obtained only if the requested and calculated size
     * is larger than the last allocated size.
     */
    if (xpndsize > config_allocxsize)
    {
        if (config_mfree)
            mfree = malloc( config_mfree );

        /* Obtain expanded storage, hinting to megabyte boundary */
        xpndstor = calloc( (size_t)(xpndsize + 1), ONE_MEGABYTE );

        if (mfree)
            free( mfree );

        if (!xpndstor)
        {
            char buf[64];
            char memsize[64];

            sysblk.xpnd_clear = 0;

            fmt_memsize_MB( xpndsize, memsize, sizeof( memsize ));
            MSGBUF( buf, "configure_xstorage(%s)", memsize );

            // "Error in function %s: %s"
            WRMSG( HHC01430, "S", buf, strerror( errno ));
            return -1;
        }

        /* Previously allocated storage to be freed, update actual
         * storage pointers and adjust new storage to megabyte boundary.
         */
        dofree = config_allocxaddr;

        config_allocxsize = xpndsize;
        config_allocxaddr = xpndstor;

        sysblk.xpnd_clear = 1;

        xpndstor = (BYTE*)(((U64)xpndstor + (ONE_MEGABYTE - 1)) &
                           ~((U64)ONE_MEGABYTE - 1));

        sysblk.xpndstor = xpndstor;
    }
    else
    {
        xpndstor = sysblk.xpndstor;
        sysblk.xpnd_clear = 0;
        dofree = NULL;
    }

    sysblk.xpndstor = xpndstor;
    sysblk.xpndsize = xpndsize << (SHIFT_MEBIBYTE - XSTORE_PAGESHIFT);

    /*  Free previously allocated storage if no longer needed
     *
     *  FIXME: The storage ordering further limits the amount of storage
     *         that may be allocated following the initial storage
     *         allocation.
     */
    if (dofree)
        free( dofree );

    /* Initial power-on reset for expanded storage */
    xstorage_clear();

    configure_region_reloc();
    initial_cpu_reset_all();

#else /* !_FEATURE_EXPANDED_STORAGE */

    UNREFERENCED( xpndsize );

    // "Expanded storage support not installed"
    WRMSG( HHC01431, "I" );

#endif /* !_FEATURE_EXPANDED_STORAGE */

    return 0;
}

POP_GCC_WARNINGS()

/*-------------------------------------------------------------------*/
/* Next 4 functions used for fast device lookup cache management     */
/*-------------------------------------------------------------------*/

static void AddDevnumFastLookup( DEVBLK *dev, U16 lcss, U16 devnum )
{
    unsigned int Channel;

    int  have_config_lock  = have_lock( &sysblk.config );

    if (!have_config_lock)
        obtain_lock( &sysblk.config );
    {
        if (sysblk.devnum_fl == NULL)
            sysblk.devnum_fl = (DEVBLK***)
                calloc( 256 * FEATURE_LCSS_MAX, sizeof( DEVBLK** ));

        Channel = (devnum >> 8) | ((lcss & (FEATURE_LCSS_MAX-1)) << 8);

        if (sysblk.devnum_fl[Channel] == NULL)
            sysblk.devnum_fl[Channel] = (DEVBLK**)
                calloc( 256, sizeof( DEVBLK* ));

        sysblk.devnum_fl[Channel][devnum & 0xff] = dev;
    }
    if (!have_config_lock)
        release_lock( &sysblk.config );
}

static void AddSubchanFastLookup( DEVBLK *dev, U16 ssid, U16 subchan )
{
    unsigned int schw;

    int  have_config_lock  = have_lock( &sysblk.config );

    if (!have_config_lock)
        obtain_lock( &sysblk.config );
    {
        if (sysblk.subchan_fl == NULL)
            sysblk.subchan_fl = (DEVBLK***)
                calloc( 256 * FEATURE_LCSS_MAX, sizeof( DEVBLK** ));

        schw = (subchan >> 8) | (SSID_TO_LCSS( ssid ) << 8);

        if (sysblk.subchan_fl[schw] == NULL)
            sysblk.subchan_fl[schw] = (DEVBLK**)
                calloc( 256, sizeof( DEVBLK* ));

        sysblk.subchan_fl[schw][subchan & 0xff] = dev;
    }
    if (!have_config_lock)
        release_lock( &sysblk.config );
}

static void DelDevnumFastLookup(U16 lcss,U16 devnum)
{
    unsigned int Channel;
    if(sysblk.devnum_fl==NULL)
    {
        return;
    }
    Channel=(devnum & 0xff00)>>8 | ((lcss & (FEATURE_LCSS_MAX-1))<<8);
    if(sysblk.devnum_fl[Channel]==NULL)
    {
        return;
    }
    sysblk.devnum_fl[Channel][devnum & 0xff]=NULL;
}

static void DelSubchanFastLookup(U16 ssid, U16 subchan)
{
    unsigned int schw;
    if(sysblk.subchan_fl==NULL)
    {
        return;
    }
    schw=((subchan & 0xff00)>>8)|(SSID_TO_LCSS(ssid) << 8);
    if(sysblk.subchan_fl[schw]==NULL)
    {
        return;
    }
    sysblk.subchan_fl[schw][subchan & 0xff]=NULL;
}

/*-------------------------------------------------------------------*/
/*                        get_devblk                                 */
/*-------------------------------------------------------------------*/

/* NOTE: also does OBTAIN_DEVLOCK( dev ); */

static DEVBLK *get_devblk(U16 lcss, U16 devnum)
{
DEVBLK*   dev;
DEVBLK**  dvpp;
char      buf[32];

    if (lcss >= FEATURE_LCSS_MAX)
        lcss = 0;

    for (dev = sysblk.firstdev; dev != NULL; dev = dev->nextdev)
        if (!(dev->allocated) && dev->ssid == LCSS_TO_SSID( lcss ))
            break;

    if (!dev)
    {
        size_t  amt  = sizeof( DEVBLK );
                amt  = ROUND_UP( amt, _4K );

        if (!(dev = (DEVBLK*) calloc_aligned( amt, _4K )))
        {
            MSGBUF( buf, "calloc(%d)", (int) amt );
            // "%1d:%04X error in function %s: %s"
            WRMSG( HHC01460, "E", lcss, devnum, buf, strerror( errno ));
            return NULL;
        }

        INIT_BLOCK_HEADER_TRAILER( dev, DEVBLK );

        /* Initialize the device lock and conditions */

        initialize_lock      ( &dev->lock               );
        initialize_condition ( &dev->kbcond             );
#if defined( OPTION_SHARED_DEVICES )
        initialize_condition ( &dev->shiocond           );
#endif // defined( OPTION_SHARED_DEVICES )
#if defined(OPTION_SCSI_TAPE)
        initialize_condition ( &dev->stape_sstat_cond   );
        InitializeListLink   ( &dev->stape_statrq.link  );
        InitializeListLink   ( &dev->stape_mntdrq.link  );
        dev->stape_statrq.dev = dev;
        dev->stape_mntdrq.dev = dev;
#endif
        /* Search for the last device block on the chain */
        for (dvpp = &(sysblk.firstdev); *dvpp != NULL;
            dvpp = &((*dvpp)->nextdev));

        /* Add the new device block to the end of the chain */
        *dvpp = dev;

        dev->ssid = LCSS_TO_SSID( lcss );
        dev->subchan = sysblk.highsubchan[ lcss ]++;
    }

    /* Obtain the device lock. Caller will release it. */
    OBTAIN_DEVLOCK( dev );

    dev->group = NULL;
    dev->member = 0;

    memset(dev->filename, 0, sizeof(dev->filename));

    dev->cpuprio = sysblk.cpuprio;
    dev->devprio = sysblk.devprio;
    dev->hnd = NULL;
    dev->devnum = devnum;

    MSGBUF( buf,  "&dev->lock %1d:%04X", LCSS_DEVNUM );
    set_lock_name( &dev->lock, buf );

    dev->chanset = lcss;
    dev->chptype[0] = CHP_TYPE_EIO; /* Interim - default to emulated */
    dev->fd = -1;
    dev->ioint.dev = dev;
    dev->ioint.pending = 1;
    dev->ioint.priority = -1;
    dev->pciioint.dev = dev;
    dev->pciioint.pcipending = 1;
    dev->pciioint.priority = -1;
    dev->attnioint.dev = dev;
    dev->attnioint.attnpending = 1;
    dev->attnioint.priority = -1;
    dev->oslinux = (OS_LINUX == sysblk.pgminttr);

    /* Initialize storage view */
    dev->mainstor = sysblk.mainstor;
    dev->storkeys = sysblk.storkeys;
    dev->mainlim = sysblk.mainsize - 1;

    /* Initialize the path management control word */
    memset (&dev->pmcw, 0, sizeof(PMCW));
    dev->pmcw.devnum[0] = dev->devnum >> 8;
    dev->pmcw.devnum[1] = dev->devnum & 0xFF;
    dev->pmcw.lpm = 0x80;
    dev->pmcw.pim = 0x80;
    dev->pmcw.pom = 0xFF;
    dev->pmcw.pam = 0x80;
    dev->pmcw.chpid[0] = dev->devnum >> 8;

#if defined(OPTION_SHARED_DEVICES)
    dev->shrdwait = -1;
#endif /*defined(OPTION_SHARED_DEVICES)*/

    if (!dev->pGUIStat)
    {
         dev->pGUIStat = malloc( sizeof(GUISTAT) );
         dev->pGUIStat->pszOldStatStr = dev->pGUIStat->szStatStrBuff1;
         dev->pGUIStat->pszNewStatStr = dev->pGUIStat->szStatStrBuff2;
        *dev->pGUIStat->pszOldStatStr = 0;
        *dev->pGUIStat->pszNewStatStr = 0;
    }

    /* Mark device valid */
    dev->pmcw.flag5 |= PMCW5_V;
    dev->allocated = 1;

    return dev;
}

/*-------------------------------------------------------------------*/
/*                        ret_devblk                                 */
/*-------------------------------------------------------------------*/

/* NOTE: also does RELEASE_DEVLOCK( dev );*/

static void ret_devblk(DEVBLK *dev)
{
    /* PROGRAMMING NOTE: the device buffer will be freed by the
       'attach_device' function whenever it gets reused and not
       here where you would normally expect it to be done since
       doing it here might cause Hercules to crash due to poorly
       written device handlers that still access the buffer for
       a brief period after the device has been detached.
    */
    /* Mark device invalid */
    dev->allocated = 0;
    dev->pmcw.flag5 &= ~PMCW5_V;
    RELEASE_DEVLOCK( dev );
}

/*-------------------------------------------------------------------*/
/* Function to delete a device configuration block                   */
/*-------------------------------------------------------------------*/
static int detach_devblk (DEVBLK *dev, int locked, const char *msg,
                          DEVBLK *errdev, DEVGRP *group)
{
int     i;                              /* Loop index                */

    /* Free the entire group if this is a grouped device */
    if (free_group( dev->group, locked, msg, errdev ))
    {
        /* Group successfully freed. All devices in the group
           have been detached. Nothing remains for us to do.
           All work has been completed. Return to caller.
        */
        return 0;
    }

    /* Restore group ptr that that 'free_group' may have set to NULL */
    dev->group = group;

    /* Obtain the device lock. ret_devblk will release it */
    if (!locked)
        OBTAIN_DEVLOCK( dev );

    DelSubchanFastLookup(dev->ssid, dev->subchan);
    if(dev->pmcw.flag5 & PMCW5_V)
        DelDevnumFastLookup(LCSS_DEVNUM);

    /* Close file or socket */
    if ((dev->fd >= 0) || dev->console)
        /* Call the device close handler */
        (dev->hnd->close)(dev);

    /* Issue device detached message and build channel report */
    if (dev != errdev)
    {
        if (MLVL(DEBUG))
        {
            // "%1d:%04X %s detached"
            WRMSG (HHC01465, "I", LCSS_DEVNUM, msg);
        }

#ifdef _FEATURE_CHANNEL_SUBSYSTEM
        /* Don't bother with channel report if we're shutting down */
        if (!sysblk.shutdown)
        {
#if defined(_370)
            if (sysblk.arch_mode != ARCH_370_IDX)
#endif
                build_detach_chrpt( dev );
        }
#endif /*_FEATURE_CHANNEL_SUBSYSTEM*/
    }

    /* Free the argv array */
    for (i = 0; i < dev->argc; i++)
        if (dev->argv[i])
            free(dev->argv[i]);
    if (dev->argv)
        free(dev->argv);

    /* Free the device type name */
    free(dev->typname);

    /* Release lock and return the device to the DEVBLK pool */
    ret_devblk( dev ); /* also does RELEASE_DEVLOCK( dev );*/

    return 0;
} /* end function detach_devblk */

/*-------------------------------------------------------------------*/
/* Function to delete a device configuration block by subchannel     */
/*-------------------------------------------------------------------*/
static int detach_subchan (U16 lcss, U16 subchan, U16 devnum)
{
DEVBLK *dev;                            /* -> Device block           */
int    rc;
char   str[64];
    /* Find the device block */
    dev = find_device_by_subchan ((LCSS_TO_SSID(lcss)<<16)|subchan);

    MSGBUF( str, "subchannel %1d:%04X", lcss, subchan);

    if (dev == NULL)
    {
        // "%1d:%04X %s does not exist"
        WRMSG (HHC01464, "E", lcss, devnum, str);
        return 1;
    }

    obtain_lock(&sysblk.config);

    if (dev->group)
        MSGBUF( str, "group subchannel %1d:%04X", lcss, subchan);

    rc = detach_devblk( dev, FALSE, str, NULL, dev->group );

    release_lock(&sysblk.config);

    return rc;
}

/*-------------------------------------------------------------------*/
/* Function to terminate all CPUs and devices                        */
/*-------------------------------------------------------------------*/
void release_config( void* arg )
{
DEVBLK *dev;
int     cpu;

    UNREFERENCED( arg );

    /* Deconfigure all CPU's */
    OBTAIN_INTLOCK(NULL);
    for (cpu = 0; cpu < sysblk.maxcpu; cpu++)
        if(IS_CPU_ONLINE(cpu))
            deconfigure_cpu(cpu);
    RELEASE_INTLOCK(NULL);

    /* Dump trace tables at exit */
#if defined( OPTION_SHARED_DEVICES )
    if (sysblk.shrddtax)
        shared_print_trace_table();
#endif
    if (cckd_dtax())
        cckd_print_itrace();
    if (ptt_dtax())
        ptt_pthread_print();

    /* Detach all devices */
    for (dev = sysblk.firstdev; dev != NULL; dev = dev->nextdev)
        if (dev->allocated)
        {
            if (sysblk.arch_mode == ARCH_370_IDX)
                detach_device(LCSS_DEVNUM);
            else
                detach_subchan(SSID_TO_LCSS(dev->ssid), dev->subchan, dev->devnum);
        }

    /* Terminate device threads */
    OBTAIN_IOQLOCK();
    {
        sysblk.devtwait = 0;
        broadcast_condition( &sysblk.ioqcond );
    }
    RELEASE_IOQLOCK();

    /* release storage          */
    sysblk.lock_mainstor = 0;
    WRMSG( HHC01427, "I", "Main", !configure_storage(~0ULL) ? "" : "not " );

    /* release expanded storage */
    sysblk.lock_xpndstor = 0;
    WRMSG( HHC01427, "I", "Expanded", !configure_xstorage(~0ULL) ? "" : "not ");

    WRMSG(HHC01422, "I");
} /* end function release_config */

#if defined( OPTION_SHARED_DEVICES )

/*-------------------------------------------------------------------*/
/*            shrd_devinit                                           */
/*-------------------------------------------------------------------*/
static void* shrd_devinit( void* arg )
{
    DEVBLK* dev = (DEVBLK*) arg;
    dev->hnd->init( dev, 0, NULL );
    return NULL;
}

/*-------------------------------------------------------------------*/
/*                       configure_shrdport                          */
/*-------------------------------------------------------------------*/
int configure_shrdport( U16 shrdport )
{
    int      rc;
    DEVBLK*  dev;
    TID      tid;

    obtain_lock( &sysblk.shrdlock );

    if (shrdport && sysblk.shrdport)
    {
        release_lock( &sysblk.shrdlock );
        // "Shared: Server already active"
        WRMSG( HHC00744, "E" );
        return -1;
    }

    /* Set requested shared port number */
    sysblk.shrdport = shrdport;

    /* Terminate the shared_server thread if requested */
    if (!sysblk.shrdport)
    {
        shutdown_shared_server_locked( NULL );
        release_lock( &sysblk.shrdlock );
        return 0;
    }

    /* Start the shared server thread */
    rc = create_thread( &sysblk.shrdtid, DETACHED,
                        shared_server, NULL, "shared_server" );
    if (rc)
    {
        sysblk.shrdport = 0;
        release_lock( &sysblk.shrdlock );
        // "Error in function create_thread(): %s"
        WRMSG( HHC00102, "E", strerror( rc ));
        return -1;
    }

    release_lock( &sysblk.shrdlock );

    /* Retry any pending connections */
    for (dev = sysblk.firstdev; dev != NULL; dev = dev->nextdev)
    {
        if (dev->connecting)
        {
            rc = create_thread( &tid, DETACHED,
                       shrd_devinit, dev, "shrd_devinit" );
            if (rc)
            {
                // "Error in function create_thread(): %s"
                WRMSG( HHC00102, "E", strerror( rc ));
                return -1;
            }
        }
    }

    return 0;
}
#endif /* defined( OPTION_SHARED_DEVICES ) */

/*-------------------------------------------------------------------*/
/* Check if we're a CPU thread or not.       (boolean function)      */
/*-------------------------------------------------------------------*/
DLL_EXPORT bool are_cpu_thread( int* cpunum )
{
    TID  tid  = thread_id();
    int  i;

    for (i=0; i < sysblk.maxcpu; i++)
    {
        if (equal_threads( sysblk.cputid[ i ], tid ))
        {
            if (cpunum)
                *cpunum = i;
            return true;        // (we ARE a CPU thread)
        }
    }

    if (cpunum)
        *cpunum = -1;

    return false;               // (we are NOT a CPU thead)
}

/*-------------------------------------------------------------------*/
/* Check if we're a CPU executing diagnose   (boolean function)      */
/*-------------------------------------------------------------------*/
DLL_EXPORT bool is_diag_instr()
{
    REGS* regs;
    bool  arecpu;
    int   ourcpu;

    /* Find out if we are a cpu thread */
    if (!(arecpu = are_cpu_thread( &ourcpu )))
        return false;

    /* Point to our REGS structure */
    regs = sysblk.regs[ ourcpu ];

    /* Return TRUE/FALSE boolean as appropriate */
    return regs->diagnose ? true : false;
}

/*-------------------------------------------------------------------*/
/* Function to start a new CPU thread                                */
/* This routine MUST be called with OBTAIN_INTLOCK already held.     */
/*-------------------------------------------------------------------*/
int configure_cpu( int target_cpu )
{
    /* If CPU isn't already configured... */
    if (!IS_CPU_ONLINE( target_cpu ))
    {
        int   rc;
        char  thread_name[32];
        bool  arecpu;
        int   ourcpu;

        /* If no more CPUs are permitted, exit */
        if (sysblk.cpus >= sysblk.maxcpu)
            return HERRCPUOFF; /* CPU offline; maximum reached */

        MSGBUF( thread_name, "Processor %s%02X",
            PTYPSTR( target_cpu ), target_cpu );

        rc = create_thread( &sysblk.cputid[ target_cpu ], JOINABLE,
                            cpu_thread, &target_cpu, thread_name );
        if (rc)
        {
            // "Error in function create_thread(): %s"
            WRMSG( HHC00102, "E", strerror( rc ));
            return HERRCPUOFF; /* CPU offline; create_thread failed */
        }

#if defined( _POSIX_THREAD_CPUTIME ) && (_POSIX_THREAD_CPUTIME >= 0)
        /* Initialise the CPU's thread clockid so that clock_gettime() can use it */
        /* provided the _POSIX_THREAD_CPUTIME is supported.                       */
        pthread_getcpuclockid( sysblk.cputid[ target_cpu ], &sysblk.cpuclockid[ target_cpu ]);
        if (!sysblk.hhc_111_112)
            WRMSG( HHC00111, "I", (long int)_POSIX_THREAD_CPUTIME );
#else
        /* When not supported, we zero the cpuclockid, which will trigger a       */
        /* different approach to obtain the thread CPU time in clock.c            */
        sysblk.cpuclockid[ target_cpu ] = 0;
        if (!sysblk.hhc_111_112)
            WRMSG( HHC00112, "W" );
#endif
        sysblk.hhc_111_112 = true;

        /* Find out if we are a cpu thread */
        arecpu = are_cpu_thread( &ourcpu );

        if (arecpu)
            sysblk.regs[ ourcpu ]->intwait = true;

        /* Wait for CPU thread to initialize */
        while (!IS_CPU_ONLINE( target_cpu ))
            wait_condition( &sysblk.cpucond, &sysblk.intlock );

        /* Now wait for it to reach its STOPPED state */
        while (sysblk.regs[ target_cpu ]->cpustate != CPUSTATE_STOPPED)
            wait_condition( &sysblk.cpucond, &sysblk.intlock );

        if (arecpu)
            sysblk.regs[ ourcpu ]->intwait = false;

#if defined( FEATURE_011_CONFIG_TOPOLOGY_FACILITY )
        /* Set topology-change-report-pending condition */
        if (FACILITY_ENABLED( 011_CONFIG_TOPOLOGY, sysblk.regs[ target_cpu ]))
            sysblk.topchnge = 1;
#endif
    }

    return 0;
} /* end function configure_cpu */

/*-------------------------------------------------------------------*/
/* Function to remove a CPU from the configuration                   */
/* This routine MUST be called with OBTAIN_INTLOCK already held.     */
/*-------------------------------------------------------------------*/
int deconfigure_cpu( int target_cpu )
{
    /* If CPU isn't already deconfigured... */
    if (IS_CPU_ONLINE( target_cpu ))
    {
        int   ourcpu;
        bool  arecpu  = are_cpu_thread( &ourcpu );

        if (!arecpu || target_cpu != ourcpu)  // NORMAL CASE
        {
            /* We're either not a CPU thread, or if we are,
               we're not attempting to deconfigure ourself.
            */

            /* Deconfigure CPU */
            sysblk.regs[ target_cpu ]->configured = 0;
            sysblk.regs[ target_cpu ]->cpustate = CPUSTATE_STOPPING;
            ON_IC_INTERRUPT( sysblk.regs[ target_cpu ]);

            /* Wake up CPU as it may be waiting */
            WAKEUP_CPU( sysblk.regs[ target_cpu ]);

            /* (if we're a cpu thread) */
            if (arecpu)
                sysblk.regs[ ourcpu ]->intwait = true;

            /* Wait for CPU thread to terminate */
            while (IS_CPU_ONLINE( target_cpu ))
                wait_condition( &sysblk.cpucond, &sysblk.intlock );

            /* (if we're a cpu thread) */
            if (arecpu)
                sysblk.regs[ ourcpu ]->intwait = false;

            /* Wait for cpu_thread to completely exit */
            join_thread( sysblk.cputid[ target_cpu ], NULL );
#if defined( OPTION_FTHREADS )
            detach_thread( sysblk.cputid[ target_cpu ]);    // only needed for Fish threads
#endif

            /*-----------------------------------------------------------*/
            /* Note: While this is the logical place to cleanup and to   */
            /*       release the associated regs context, there is also  */
            /*       post-processing that is done by various callers.    */
            /*-----------------------------------------------------------*/
        }
        else // (arecpu && target_cpu == ourcpu)    HIGHLY UNUSUAL!
        {
            /* We ARE a cpu thread *AND* we're trying to deconfigure
               ourself! This can only happen if B220 SERVC instruction
               is executed to deconfigure its own CPU, or else the CPU
               issues a Hercules command via the diagnose-8 interface
               to deconfigure its own CPU (i.e. itself).
            */
            sysblk.regs[ target_cpu ]->configured = 0;
            sysblk.regs[ target_cpu ]->cpustate = CPUSTATE_STOPPING;
            ON_IC_INTERRUPT( sysblk.regs[ target_cpu ]);
        }

        sysblk.cputid    [ target_cpu ] = 0;
        sysblk.cpuclockid[ target_cpu ] = 0;

#if defined( FEATURE_011_CONFIG_TOPOLOGY_FACILITY )
        /* Set topology-change-report-pending condition */

        /* PROGRAMMING NOTE: because the CPU has been deconfigured,
           the REGS pointer in sysblk (i.e. sysblk.regs[ <cpunum> ])
           could now possibly be NULL so to be safe we will use the
           FACILITY_ENABLED_DEV macro instead as "sysblk.arch_mode"
           should ALWAYS be valid.
        */
        if (FACILITY_ENABLED_DEV( 011_CONFIG_TOPOLOGY ))
            sysblk.topchnge = 1;
#endif
    }

    return 0;
} /* end function deconfigure_cpu */

/*-------------------------------------------------------------------*/
/* Configure number of online CPUs                                   */
/*-------------------------------------------------------------------*/
static int configure_numcpu_intlock_held( int numcpu )
{
    int cpu;

    /* Requested number of online CPUs must be <= maximum */
    if (numcpu > sysblk.maxcpu)
        return HERRCPUOFF;  /* CPU offline; number > maximum */

    /* All CPUs must be stopped beforehand */
    if (sysblk.cpus && !are_all_cpus_stopped_intlock_held())
        return HERRCPUONL; /* CPU online; not all are stopped */

    /* Keep deconfiguring CPUS until within desired range */
    for (cpu=sysblk.hicpu-1; cpu >= 0 && sysblk.cpus > numcpu; cpu--)
    {
        /* Deconfigure this CPU if it's currently configured */
        if (IS_CPU_ONLINE( cpu ))
            deconfigure_cpu( cpu );
    }

    /* Keep configuring CPUs until desired amount reached */
    for (cpu=0; cpu < sysblk.maxcpu && sysblk.cpus < numcpu; cpu++)
    {
        /* Configure this CPU if it's not currently configured */
        if (!IS_CPU_ONLINE( cpu ))
            configure_cpu( cpu );
    }

    return 0;
}

/*-------------------------------------------------------------------*/
/* Configure number of online CPUs                                   */
/*-------------------------------------------------------------------*/
int configure_numcpu( int numcpu )
{
    int rc = 0;

    OBTAIN_INTLOCK( NULL );
    {
        rc = configure_numcpu_intlock_held( numcpu );

        /* Update all CPU IDs to reflect new NUMCPU */
        if (rc == 0)
            resetAllCpuIds();
    }
    RELEASE_INTLOCK( NULL );

    return rc;
}

/*-------------------------------------------------------------------*/
/* Configure maximum number of online CPUs                           */
/*-------------------------------------------------------------------*/
int configure_maxcpu( int maxcpu )
{
    int rc = 0;
    OBTAIN_INTLOCK( NULL );
    {
        /* Requested maxumim must be <= absolute maximum possible */
        if (maxcpu > MAX_CPU_ENGS)
        {
            RELEASE_INTLOCK( NULL );
            return HERRCPUOFF;  /* CPU offline; number > maximum */
        }

        /* All CPUs must be stopped beforehand */
        if (sysblk.cpus && !are_all_cpus_stopped_intlock_held())
        {
            RELEASE_INTLOCK( NULL );
            return HERRCPUONL; /* CPU online; not all are stopped */
        }

        /* Set new maximum number of online CPUs */
        sysblk.maxcpu = maxcpu;

        /* Deconfigure excess online CPUs if necessary */
        if (sysblk.cpus > maxcpu)
            rc = configure_numcpu_intlock_held( maxcpu );

        /* Update all CPU IDs to reflect new MAXCPU */
        resetAllCpuIds();
    }
    RELEASE_INTLOCK( NULL );
    return rc;
}

/*-------------------------------------------------------------------*/
/* Function to build a device configuration block                    */
/*-------------------------------------------------------------------*/
int attach_device (U16 lcss, U16 devnum, const char *type,
                   int addargc, char *addargv[],
                   int numconfdev)
{
DEVBLK *dev;                            /* -> Device block           */
int     rc;                             /* Return code               */
int     i;                              /* Loop index                */

    /* Obtain (re)configuration lock */
    obtain_lock(&sysblk.config);

    /* Check whether device number has already been defined */
    if (find_device_by_devnum(lcss,devnum) != NULL)
    {
        // "%1d:%04X device already exists"
        WRMSG (HHC01461, "E", lcss, devnum);
        release_lock(&sysblk.config);
        return 1;
    }

    /* Obtain device block from our DEVBLK pool and lock the device. */
    dev = get_devblk(lcss, devnum); /* does OBTAIN_DEVLOCK( dev ); */

    // PROGRAMMING NOTE: the rule is, once a DEVBLK has been obtained
    // from the pool it can be returned back to the pool via a simple
    // call to ret_devblk if the device handler initialization function
    // has NOT yet been called. Once the device handler initialization
    // function has been called however then you MUST use detach_devblk
    // to return it back to the pool so that the entire group is freed.

    if(!(dev->hnd = hdl_DEVHND(type)))
    {
        // "%1d:%04X devtype %s not recognized"
        WRMSG (HHC01462, "E", lcss, devnum, type);
        ret_devblk(dev); /* also does RELEASE_DEVLOCK( dev );*/
        release_lock(&sysblk.config);
        return 1;
    }

    dev->typname = strdup(type);

    /* Copy the arguments */
    dev->argc = addargc;
    if (addargc)
    {
        dev->argv = malloc ( addargc * sizeof(BYTE *) );
        for (i = 0; i < addargc; i++)
            if (addargv[i])
                dev->argv[i] = strdup(addargv[i]);
            else
                dev->argv[i] = NULL;
    }
    else
        dev->argv = NULL;

    /* Set the number of config statement device addresses */
    dev->numconfdev = numconfdev;

    /* Call the device handler initialization function */
    rc = (int)(dev->hnd->init)(dev, addargc, addargv);

    if (rc < 0)
    {
        // "%1d:%04X device initialization failed"
        WRMSG (HHC01463, "E", lcss, devnum);

        /* Detach the device and return it back to the DEVBLK pool */
        detach_devblk( dev, TRUE, "device", dev, dev->group );

        release_lock(&sysblk.config);
        return 1;
    }

    /* Obtain device data buffer */
    if (dev->bufsize != 0)
    {
        /* PROGRAMMING NOTE: we free the device buffer here, not in
           the 'ret_devblk' function (where you would normally expect
           it to be done) since doing it in 'ret_devblk' might cause
           Hercules to crash due to poorly written device handlers
           that continue accessing the buffer for a brief period even
           though the device has already been detached.
        */
        if (dev->buf && dev->buf == dev->prev_buf)
            free_aligned( dev->buf );

        dev->buf = dev->prev_buf = malloc_aligned( dev->bufsize, _4K );

        if (dev->buf == NULL)
        {
            char buf[64];
            // "%1d:%04X error in function %s: %s"
            MSGBUF( buf, "malloc(%lu)", (unsigned long) dev->bufsize);
            WRMSG (HHC01460, "E", lcss, dev->devnum, buf, strerror(errno));

            /* Detach the device and return it back to the DEVBLK pool */
            detach_devblk( dev, TRUE, "device", dev, dev->group );

            release_lock(&sysblk.config);
            return 1;
        }
    }

    /* Release device lock */
    RELEASE_DEVLOCK( dev );

#ifdef _FEATURE_CHANNEL_SUBSYSTEM
    /* Build Channel Report */
#if defined(_370)
    if (sysblk.arch_mode != ARCH_370_IDX)
#endif
        build_attach_chrpt( dev );
#endif /*_FEATURE_CHANNEL_SUBSYSTEM*/

    /*
    if(lcss!=0 && sysblk.arch_mode==ARCH_370_IDX)
    {
        // "%1d:%04X: only devices on CSS 0 are usable in S/370 mode"
        WRMSG (HHC01458, "W", lcss, devnum);
    }
    */

    release_lock(&sysblk.config);

    if ( rc == 0 && MLVL(DEBUG) )
    {
        // "Device %04X type %04X subchannel %d:%04X attached"
        WRMSG(HHC02198, "I", dev->devnum, dev->devtype, dev->chanset, dev->subchan);
    }

    return 0;
} /* end function attach_device */

/*-------------------------------------------------------------------*/
/* Function to delete a device configuration block by device number  */
/*-------------------------------------------------------------------*/
int detach_device (U16 lcss,U16 devnum)
{
DEVBLK *dev;                            /* -> Device block           */
int    rc;
char* str = "device";

    /* Find the device block */
    dev = find_device_by_devnum (lcss,devnum);

    if (dev == NULL)
    {
        // "%1d:%04X %s does not exist"
        WRMSG (HHC01464, "E", lcss, devnum, str);
        return 1;
    }

    obtain_lock(&sysblk.config);

    if (dev->group)
        str = "group device";

    rc = detach_devblk( dev, FALSE, str, NULL, dev->group );

    release_lock(&sysblk.config);

    return rc;
}

/*-------------------------------------------------------------------*/
/* Function to rename a device configuration block                   */
/*-------------------------------------------------------------------*/
int define_device (U16 lcss, U16 olddevn,U16 newdevn)
{
DEVBLK *dev;                            /* -> Device block           */

    /* Obtain (re)configuration lock */
    obtain_lock(&sysblk.config);

    /* Find the device block */
    dev = find_device_by_devnum (lcss, olddevn);

    if (dev == NULL)
    {
        WRMSG (HHC01464, "E", lcss, olddevn, "device");
        release_lock(&sysblk.config);
        return 1;
    }

    /* Check that new device number does not already exist */
    if (find_device_by_devnum(lcss, newdevn) != NULL)
    {
        WRMSG (HHC01461, "E", lcss, newdevn);
        release_lock(&sysblk.config);
        return 1;
    }

#ifdef _FEATURE_CHANNEL_SUBSYSTEM
    /* Build Channel Report */
#if defined(_370)
    if (sysblk.arch_mode != ARCH_370_IDX)
#endif
        build_detach_chrpt( dev );
#endif /*_FEATURE_CHANNEL_SUBSYSTEM*/

    /* Obtain the device lock */
    OBTAIN_DEVLOCK( dev );

    /* Update the device number in the DEVBLK */
    dev->devnum = newdevn;

    /* Update the device number in the PMCW */
    dev->pmcw.devnum[0] = newdevn >> 8;
    dev->pmcw.devnum[1] = newdevn & 0xFF;

    DelDevnumFastLookup(lcss,olddevn);
    AddDevnumFastLookup(dev,lcss,newdevn);

    /* Release device lock */
    RELEASE_DEVLOCK( dev );

#ifdef _FEATURE_CHANNEL_SUBSYSTEM
    /* Build Channel Report */
#if defined(_370)
    if (sysblk.arch_mode != ARCH_370_IDX)
#endif
        build_attach_chrpt( dev );
#endif /*_FEATURE_CHANNEL_SUBSYSTEM*/

//    WRMSG (HHC01459, "I", lcss, olddevn, lcss, newdevn);

    release_lock(&sysblk.config);
    return 0;
} /* end function define_device */

/*-------------------------------------------------------------------*/
/* Function to group devblk's belonging to one device (eg OSA, LCS)  */
/*                                                                   */
/* group_device is intended to be called from within a device        */
/* initialisation routine to group 1 or more devices to a logical    */
/* device group.                                                     */
/*                                                                   */
/* group_device will return true for the device that completes       */
/* the device group. (ie the last device to join the group)          */
/*                                                                   */
/* when no group exists, and device group is called with a device    */
/* count of zero, then no group will be created.  Otherwise          */
/* a new group will be created and the currently attaching device    */
/* will be the first in the group.                                   */
/*                                                                   */
/* when a device in a group is detached, all devices in the group    */
/* will be detached. The first device to be detached will enter      */
/* its close routine with the group intact.  Subsequent devices      */
/* being detached will no longer have access to previously detached  */
/* devices.                                                          */
/*                                                                   */
/* Example of a fixed count device group:                            */
/*                                                                   */
/* device_init(dev)                                                  */
/* {                                                                 */
/*    if( !device_group(dev, 2) )                                    */
/*       return 0;                                                   */
/*                                                                   */
/*    ... all devices in the group have been attached,               */
/*    ... group initialisation may proceed.                          */
/*                                                                   */
/* }                                                                 */
/*                                                                   */
/*                                                                   */
/* Variable device group example:                                    */
/*                                                                   */
/* device_init(dev)                                                  */
/* {                                                                 */
/*    if( !group_device(dev, 0) && dev->group )                      */
/*        return 0;                                                  */
/*                                                                   */
/*    if( !device->group )                                           */
/*    {                                                              */
/*        ... process parameters to determine number of devices      */
/*                                                                   */
/*        // Create group                                            */
/*        if( !group_device(dev, variable_count) )                   */
/*            return 0;                                              */
/*    }                                                              */
/*                                                                   */
/*    ... all devices in the group have been attached,               */
/*    ... group initialisation may proceed.                          */
/* }                                                                 */
/*                                                                   */
/*                                                                   */
/* dev->group      : pointer to DEVGRP structure or NULL             */
/* dev->member     : index into memdev array in DEVGRP structure for */
/*                 : current DEVBLK                                  */
/* group->members  : number of members in group                      */
/* group->acount   : number active members in group                  */
/* group->memdev[] : array of DEVBLK pointers of member devices      */
/*                                                                   */
/*                                                                   */
/* members will be equal to acount for a complete group              */
/*                                                                   */
/*                                                                   */
/* Always: (for grouped devices)                                     */
/*   dev->group->memdev[dev->member] == dev                          */
/*                                                                   */
/*                                                                   */
/*                                           Jan Jaeger, 23 Apr 2004 */
/*-------------------------------------------------------------------*/
DLL_EXPORT int group_device(DEVBLK *dev, int members)
{
DEVBLK *tmp;

    // Find a compatible group that is incomplete
    for (tmp = sysblk.firstdev;
         tmp != NULL
           && (!tmp->allocated      // not allocated
             || !tmp->group          // not a group device
             || strcmp(tmp->typname,dev->typname)  // unequal type
             || (tmp->group->members == tmp->group->acount) ); // complete
         tmp = tmp->nextdev) ;

    if(tmp)
    {
        // Join Group
        dev->group = tmp->group;
        dev->member = dev->group->acount++;
        dev->group->memdev[dev->member] = dev;
    }
    else if(members)
    {
        // Allocate a new Group when requested
        size_t size = sizeof(DEVGRP) + (members * sizeof(DEVBLK*));
        dev->group = malloc( size );
        memset( dev->group, 0, size );
        dev->group->members = members;
        dev->group->acount = 1;
        dev->group->memdev[0] = dev;
        dev->member = 0;
    }

    return (dev->group && (dev->group->members == dev->group->acount));
}

/*-------------------------------------------------------------------*/
/* Function to free a device group  (i.e. all devices in the group)  */
/*-------------------------------------------------------------------*/
DLL_EXPORT BYTE free_group( DEVGRP *group, int locked,
                            const char *msg, DEVBLK *errdev )
{
    DEVBLK *dev;
    int i;

    if (!group || !group->members)
        return FALSE;       // no group or group has no members

    for (i=0; i < group->acount; i++)
    {
        if ((dev = group->memdev[i]) && dev->allocated)
        {
            // PROGRAMMING NOTE: detach_devblk automatically calls
            // free_group (i.e. THIS function) to try and free the
            // entire group at once in case it is a grouped device.
            // Therefore we must clear dev->group to NULL *before*
            // calling detach_devblk to prevent infinite recursion.

            dev->group = NULL;
            detach_devblk( dev, dev == errdev && locked, msg, errdev,
                           group );
        }
    }

    free( group );
    return TRUE;        // group successfully freed
}

/*-------------------------------------------------------------------*/
/* Function to find a device block given the device number           */
/*-------------------------------------------------------------------*/
DLL_EXPORT DEVBLK* find_device_by_devnum( U16 lcss, U16 devnum )
{
    DEVBLK*   dev;
    DEVBLK**  devtab;
    int       chan;

    chan = (devnum & 0xff00) >> 8 | ((lcss & (FEATURE_LCSS_MAX-1)) << 8);

    if (sysblk.devnum_fl)
    {
        devtab = sysblk.devnum_fl[ chan ];

        if (devtab)
        {
            dev = devtab[ devnum & 0xff ];

            if (dev && IS_DEV( dev ) && dev->devnum == devnum)
                return dev;
            else
                DelDevnumFastLookup( lcss, devnum );
        }
    }

    for (dev = sysblk.firstdev; dev != NULL; dev = dev->nextdev)
        if (1
            && IS_DEV( dev )
            && dev->devnum == devnum
            && lcss == SSID_TO_LCSS( dev->ssid )
        )
            break;

    if (dev)
        AddDevnumFastLookup( dev, lcss, devnum );

    return dev;
}

/*-------------------------------------------------------------------*/
/* Function to find a device block given the subchannel number       */
/*-------------------------------------------------------------------*/
DEVBLK* find_device_by_subchan( U32 ioid )
{
    DEVBLK*  dev;
    U16      subchan   = ioid & 0xFFFF;
    unsigned int schw  = ((subchan & 0xff00)>>8) | (IOID_TO_LCSS( ioid ) << 8);

    if (1
        && sysblk.subchan_fl
        && sysblk.subchan_fl[ schw ]
        && sysblk.subchan_fl[ schw ][ subchan & 0xff ]
    )
        return sysblk.subchan_fl[ schw ][ subchan & 0xff ];

    for (dev = sysblk.firstdev; dev != NULL; dev = dev->nextdev)
        if (dev->ssid == IOID_TO_SSID( ioid ) && dev->subchan == subchan)
            break;

    if (dev)
        AddSubchanFastLookup( dev, IOID_TO_SSID( ioid ), subchan );
    else
        DelSubchanFastLookup( IOID_TO_SSID( ioid ), subchan );

    return dev;
}

/*-------------------------------------------------------------------*/
/* Function to Parse a LCSS specification in a device number spec    */
/* Syntax : [lcss:]Anything...                                       */
/* Function args :                                                   */
/*               const char * spec : Parsed string                   */
/*               char **rest : Rest of string (or original str)      */
/* Returns :                                                         */
/*               int : 0 if not specified, 0<=n<FEATURE_LCSS_MAX     */
/*                     -1 Spec error                                 */
/*                                                                   */
/* If the function returns a positive value, then *rest should       */
/* be freed by the caller.                                           */
/*-------------------------------------------------------------------*/
static int parse_lcss( const char* spec, char** rest, int verbose )
{
    int     lcssid;
    char*   wrk;
    char*   lcss;
    char*   r;
    char*   strptr;
    char*   garbage;
    char*   strtok_str = NULL;
    size_t  len;

    len = strlen( spec ) + 1;
    wrk = malloc( len );
    strlcpy( wrk, spec, len );

    if (!(lcss = strtok_r( wrk, ":", &strtok_str )))
    {
        if (verbose)
        {
            // "Unspecified error occured while parsing Logical Channel Subsystem Identification"
            WRMSG( HHC01466, "E" );
        }
        free( wrk );
        return -1;
    }


    if (!(r=strtok_r( NULL, ":", &strtok_str )))
    {
        *rest = wrk;
        return 0;
    }

    garbage = strtok_r( NULL, ":", &strtok_str );

    if (garbage)
    {
        if (verbose)
        {
            // "No more than 1 Logical Channel Subsystem Identification may be specified"
            WRMSG( HHC01467, "E" );
        }
        free( wrk );
        return -1;
    }

    lcssid = strtoul( lcss, &strptr, 10 );

    if (*strptr != 0)
    {
        if (verbose)
        {
            // "Non-numeric Logical Channel Subsystem Identification %s"
            WRMSG( HHC01468, "E", lcss );
        }
        free( wrk );
        return -1;
    }

    if (lcssid >= FEATURE_LCSS_MAX)
    {
        if (verbose)
        {
            // "Logical Channel Subsystem Identification %d exceeds maximum of %d"
            WRMSG( HHC01469, "E", lcssid, FEATURE_LCSS_MAX-1 );
        }
        free( wrk );
        return -1;
    }

    len = strlen( r ) + 1;
    *rest = malloc( len );
    strlcpy( *rest, r, len );
    free( wrk );

    return lcssid;
}

/*-------------------------------------------------------------------*/
/*               parse_single_devnum_INTERNAL                       */
/*-------------------------------------------------------------------*/
static int parse_single_devnum_INTERNAL
(
    const char*  spec,
    U16*         p_lcss,
    U16*         p_devnum,
    int          verbose
)
{
    int     rc;
    U16     lcss;
    char    *r;
    char    *strptr;

    if ((rc = parse_lcss( spec, &r, verbose )) < 0)
        return -1;

    lcss = rc;

    if (str_caseless_eq( r, "sysg" ))
    {
        *p_devnum = 0;
        *p_lcss   = lcss;
        free( r );
        return 0;
    }

    rc = strtoul( r, &strptr, 16 );

    if (0
        || rc < 0
        || rc > 0xffff
        || *strptr != 0
    )
    {
        int err = 1;

        /* Maybe it's just a statement comment? */
        if (1
            && rc >= 0
            && rc <= 0xffff
            && *strptr != 0
        )
        {
            while (*strptr == ' ')
                strptr++;

            /* End of statement or start of comment? */
            if (0
                || *strptr ==  0
                || *strptr == '#'
            )
                err = 0;  /* Then not an error! */
        }

        if (err)
        {
            if (verbose)
            {
                // "Incorrect %s near character '%c'"
                WRMSG( HHC01470, "E", "device address specification", *strptr );
            }
            free( r );
            return -1;
        }
    }

    *p_devnum = rc;
    *p_lcss   = lcss;

    free( r );
    return 0;
}

/*-------------------------------------------------------------------*/
/*                    parse_single_devnum                            */
/*-------------------------------------------------------------------*/
DLL_EXPORT int parse_single_devnum( const char* spec, U16* lcss, U16* devnum )
{
    int verbose = TRUE;
    return parse_single_devnum_INTERNAL( spec, lcss, devnum, verbose );
}

/*-------------------------------------------------------------------*/
/*                  parse_single_devnum_silent                       */
/*-------------------------------------------------------------------*/
int parse_single_devnum_silent( const char* spec, U16* lcss, U16* devnum )
{
    int verbose = FALSE;
    return parse_single_devnum_INTERNAL( spec, lcss, devnum, verbose );
}

/*-------------------------------------------------------------------*/
/* Function to Parse compound device numbers                         */
/* Syntax : [lcss:]CCUU[-CUU][,CUU..][.nn][...]                      */
/* Examples : 200-23F                                                */
/*            200,201                                                */
/*            200.16                                                 */
/*            200-23F,280.8                                          */
/*            etc...                                                 */
/* : is the LCSS id separator (only 0 or 1 allowed and it must be 1st*/
/* - is the range specification (from CUU to CUU)                    */
/* , is the separator                                                */
/* . is the count indicator (nn is decimal)                          */
/* 1st parm is the specification string as specified above           */
/* 2nd parm is the address of an array of DEVARRAY                   */
/* Return value : 0 - Parsing error, etc..                           */
/*                >0 - Size of da                                    */
/*                                                                   */
/* NOTE : A basic validity check is made for the following :         */
/*        All CUUs must belong on the same channel                   */
/*        (this check is to eventually pave the way to a formal      */
/*         channel/cu/device architecture)                           */
/*        no 2 identical CCUUs                                       */
/*   ex : 200,300 : WRONG                                            */
/*        200.12,200.32 : WRONG                                      */
/*        2FF.2 : WRONG                                              */
/* NOTE : caller should free the array returned in da if the return  */
/*        value is not 0                                             */
/*-------------------------------------------------------------------*/
DLL_EXPORT size_t parse_devnums( const char* spec, DEVNUMSDESC* dd )
{
    size_t     gcount;          /* Group count                       */
    size_t     i;               /* Index runner                      */
    char*      grps;            /* Pointer to current devnum group   */
    char*      sc;              /* Specification string copy         */
    DEVARRAY*  dgrs;            /* Device groups                     */
    U16        cuu1, cuu2;      /* CUUs                              */
    char*      strptr;          /* strtoul ptr-ptr                   */
    int        basechan = 0;    /* Channel for all CUUs              */
    int        duplicate;       /* duplicated CUU indicator          */
    int        badcuu;          /* offending CUU                     */
    int        rc;              /* Return code work var              */
    char*      strtok_str;      /* Last token                        */

    strtok_str = NULL;

    if ((rc = parse_lcss( spec, &sc, 1 )) < 0)
    {
        return 0;
    }

    dd->lcss = rc;

    /* Split by ',' groups */

    gcount = 0;
    grps   = strtok_r( sc, ",", &strtok_str );
    dgrs   = NULL;

    while (grps)
    {
        if (!dgrs)
            dgrs = malloc( sizeof( DEVARRAY ));
        else
            dgrs = realloc( dgrs, sizeof( DEVARRAY ) * (gcount + 1));

        cuu1 = strtoul( grps, &strptr, 16 );

        switch (*strptr)
        {
        case 0:     /* Single CUU */

            cuu2 = cuu1;
            break;

        case '-':   /* CUU Range */

            cuu2 = strtoul( &strptr[1], &strptr, 16 );

            if (*strptr)
            {
                // "Incorrect %s near character '%c'"
                WRMSG( HHC01470, "E", "second device number in device range", *strptr );
                goto error_ret;
            }
            break;

        case '.':   /* CUU Count */

            cuu2 = cuu1 + strtoul( &strptr[1], &strptr, 10 );
            cuu2--;

            if (*strptr)
            {
                // "Incorrect %s near character '%c'"
                WRMSG( HHC01470, "E", "device count", *strptr );
                goto error_ret;
            }
            break;

        default:

            // "Incorrect %s near character '%c'"
            WRMSG( HHC01470, "E", "device specification", *strptr );
            goto error_ret;
        }

        /* Check cuu1 <= cuu2 */

        if (cuu1 > cuu2)
        {
            // "Incorrect device address range %04X < %04X"
            WRMSG( HHC01471, "E", cuu2, cuu1 );
            goto error_ret;
        }

        if (!gcount)
        {
            basechan = (cuu1 >> 8) & 0xff;
        }

        badcuu = -1;

        if (((cuu1 >> 8) & 0xff) != basechan)
        {
            badcuu = cuu1;
        }
        else
        {
            if (((cuu2 >> 8) & 0xff) != basechan)
            {
                badcuu = cuu2;
            }
        }

        if (badcuu >= 0)
        {
            // "%1d:%04X is on wrong channel, 1st device defined on channel %02X"
            WRMSG( HHC01472, "E", dd->lcss, badcuu, basechan );
            goto error_ret;
        }

        /* Check for duplicates */

        duplicate = 0;

        for (i=0; i < gcount; i++)
        {
            /* check 1st cuu not within existing range */
            if (cuu1 >= dgrs[i].cuu1 && cuu1 <= dgrs[i].cuu2)
            {
                duplicate = 1;
                break;
            }

            /* check 2nd cuu not within existing range */
            if (cuu2 >= dgrs[i].cuu1 && cuu1 <= dgrs[i].cuu2)
            {
                duplicate = 1;
                break;
            }

            /* check current range doesn't completelly overlap existing range */
            if (cuu1 < dgrs[i].cuu1 && cuu2 > dgrs[i].cuu2)
            {
                duplicate = 1;
                break;
            }
        }

        if (duplicate)
        {
            // "Some or all devices in %04X-%04X duplicate already defined devices"
            WRMSG( HHC01473, "E", cuu1, cuu2 );
            goto error_ret;
        }

        dgrs[ gcount ].cuu1 = cuu1;
        dgrs[ gcount ].cuu2 = cuu2;

        gcount++;

        grps = strtok_r( NULL, ",", &strtok_str );
    }

    dd->da = dgrs;

    free( sc );

    return gcount;

error_ret:

    free( dgrs );
    free( sc );

    return 0;
}

/*-------------------------------------------------------------------*/
/*                  parse_and_attach_devices                         */
/*-------------------------------------------------------------------*/
int parse_and_attach_devices(const char *sdevnum,
                        const char *sdevtype,
                        int  addargc,
                        char **addargv)
{
        DEVNUMSDESC dnd;
        int         baddev;
        size_t      devncount;
        DEVARRAY    *da;
        int         i;
        U16         devnum;
        int         rc;
        int         numconfdev = 0;
        int         j;
        char        **newargv;
        char        **orig_newargv;

        devncount=parse_devnums(sdevnum,&dnd);

        if(devncount==0)
            return HERRDEVIDA; /* Invalid Device Address */

        /* Calculate the number of config statement device addresses */
        for (i=0;i<(int)devncount;i++)
        {
            da=dnd.da;
            numconfdev = numconfdev + ((da[i].cuu2 - da[i].cuu1) + 1);
        }

        newargv=malloc(MAX_ARGS*sizeof(char *));
        orig_newargv=malloc(MAX_ARGS*sizeof(char *));

        for(baddev=0,i=0;i<(int)devncount;i++)
        {
            da=dnd.da;
            for(devnum=da[i].cuu1;devnum<=da[i].cuu2;devnum++)
            {
               char wrkbfr[32];
               MSGBUF( wrkbfr, "%3.3X",devnum);
               set_symbol("CUU",wrkbfr);
               MSGBUF( wrkbfr, "%4.4X",devnum);
               set_symbol("CCUU",wrkbfr);
               set_symbol("DEVN", wrkbfr);
               MSGBUF( wrkbfr, "%d",dnd.lcss);
               set_symbol("CSS",wrkbfr);

               for(j=0;j<addargc;j++)
               {
                   orig_newargv[j]=newargv[j]=resolve_symbol_string(addargv[j]);
               }
               /* Build the device configuration block */
               rc=attach_device(dnd.lcss, devnum, sdevtype, addargc, newargv, devnum - da[i].cuu1 + 1);
               for(j=0;j<addargc;j++)
               {
                   free(orig_newargv[j]);
               }

               if(rc!=0)
               {
                   baddev=1;
               }
            }
        }

        free(newargv);
        free(orig_newargv);
        free(dnd.da);
        return baddev?-1:0;
}

#define MAX_LOGO_LINES 256
/*-------------------------------------------------------------------*/
/*                          clearlogo                                */
/*-------------------------------------------------------------------*/
DLL_EXPORT void clearlogo()
{
    size_t i;
    if(sysblk.herclogo!=NULL)
    {
        for(i=0;i<sysblk.logolines;i++)
        {
            free(sysblk.herclogo[i]);
        }
        free(sysblk.herclogo);
        sysblk.herclogo=NULL;
    }
}

/*-------------------------------------------------------------------*/
/*                          readlogo                                 */
/*-------------------------------------------------------------------*/
DLL_EXPORT int readlogo(char *fn)
{
    char    **data;
    char     bfr[256];
    char    *rec;
    FILE    *lf;
    size_t   len;

    clearlogo();

    lf=fopen(fn,"r");
    if(lf==NULL)
    {
        return -1;
    }
    data=malloc(sizeof(char *)*MAX_LOGO_LINES);
    sysblk.logolines=0;
    while((rec=fgets(bfr,sizeof(bfr),lf))!=NULL)
    {
        rec[strlen(rec)-1]=0;
        len = strlen(rec)+1;
        data[sysblk.logolines]=malloc(len);
        strlcpy(data[sysblk.logolines],rec,len);
        sysblk.logolines++;
        if(sysblk.logolines>MAX_LOGO_LINES)
        {
            break;
        }
    }
    fclose(lf);
    sysblk.herclogo=data;
    return 0;
}

/*-------------------------------------------------------------------*/
/*                        parse_conkpalv                             */
/*-------------------------------------------------------------------*/
DLL_EXPORT int parse_conkpalv(char* s, int* idle, int* intv, int* cnt )
{
    size_t n; char *p1, *p2, *p3, c;
    ASSERT(s && *s && idle && intv && cnt);
    if (!s || !*s || !idle || !intv || !cnt) return -1;
    // Format: "(idle,intv,cnt)". All numbers. No spaces.
    if (0
        || (n = strlen(s)) < 7
        || s[0]   != '('
        || s[n-1] != ')'
    )
        return -1;
    // 1st sub-operand
    if (!(p1 = strchr(s+1, ','))) return -1;
    c = *p1; *p1 = 0;
    if ( strspn( s+1, "0123456789" ) != strlen(s+1) )
    {
        *p1 = c;
        return -1;
    }
    *p1 = c;
    // 2nd sub-operand
    if (!(p2 = strchr(p1+1, ','))) return -1;
    c = *p2; *p2 = 0;
    if ( strspn( p1+1, "0123456789" ) != strlen(p1+1) )
    {
        *p2 = c;
        return -1;
    }
    *p2 = c;
    // 3rd sub-operand
    if (!(p3 = strchr(p2+1, ')'))) return -1;
    c = *p3; *p3 = 0;
    if ( strspn( p2+1, "0123456789" ) != strlen(p2+1) )
    {
        *p3 = c;
        return -1;
    }
    *p3 = c;
    // convert each to number
    c = *p1; *p1 = 0; *idle = atoi(s+1);  *p1 = c;
    c = *p2; *p2 = 0; *intv = atoi(p1+1); *p2 = c;
    c = *p3; *p3 = 0; *cnt  = atoi(p2+1); *p3 = c;
    // check results
    if (*idle <= 0 || INT_MAX == *idle) return -1;
    if (*intv <= 0 || INT_MAX == *intv) return -1;
    if (*cnt  <= 0 || INT_MAX == *cnt ) return -1;
    return 0;
}

#endif /*!defined(_GEN_ARCH)*/
