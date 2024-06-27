/* SR.C         (c) Copyright Greg Smith, 2005-2012                  */
/*              (C) and others 2013-2021                             */
/*              Suspend/Resume a Hercules session                    */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#include "hstdinc.h"

#define _HERCULES_SR_C
#define _HENGINE_DLL_

#include "hercules.h"
#include "opcode.h"

//#define SR_DEBUG    // #define to enable TRACE stmts

#if defined(DEBUG) && !defined(SR_DEBUG)
 #define SR_DEBUG
#endif

#if defined(SR_DEBUG)
 #define  ENABLE_TRACING_STMTS   1       // (Fish: DEBUGGING)
 #include "dbgtrace.h"                   // (Fish: DEBUGGING)
 #define  NO_SR_OPTIMIZE                 // (Fish: DEBUGGING) (MSVC only)
#endif

#if defined( _MSVC_ ) && defined( NO_SR_OPTIMIZE )
  #pragma optimize( "", off )           // disable optimizations for reliable breakpoints
#endif

#include "sr.h"     // (must FOLLOW above enable/disable debugging macros)

#if defined(WORDS_BIGENDIAN)
BYTE ourendian = 1;
#else // (little endian)
BYTE ourendian = 0;
#endif // defined(WORDS_BIGENDIAN)
BYTE crwendian;

/* subroutine to check for active devices */
DEVBLK *sr_active_devices()
{
DEVBLK *dev;

    for (dev = sysblk.firstdev; dev; dev = dev->nextdev)
    {
        OBTAIN_DEVLOCK( dev );
        {
            if (dev->busy && !dev->suspended)
            {
                if (dev->devtype != 0x3088)
                {
                    RELEASE_DEVLOCK( dev );
                    return dev;
                }
                else
                {
                    USLEEP(50000);
                    dev->busy = 0;
                }
            }
        }
        RELEASE_DEVLOCK( dev );
    }
    return NULL;
}

int suspend_cmd(int argc, char *argv[],char *cmdline)
{
char    *fn = SR_DEFAULT_FILENAME;
SR_FILE  file;
CPU_BITMAP started_mask;
struct   timeval tv;
time_t   tt;
int      i, j, rc;
REGS    *regs;
DEVBLK  *dev;
IOINT   *ioq;
BYTE     psw[16];

    UNREFERENCED(cmdline);

    if (argc > 2)
    {
        // "SR: too many arguments"
        WRMSG(HHC02000, "E");
        return -1;
    }

    if (argc == 2)
        fn = argv[1];

    file = SR_OPEN (fn, "wb");
    if (file == NULL)
    {
        // "SR: error in function '%s': '%s'"
        WRMSG(HHC02001, "E","open()",strerror(errno));
        return -1;
    }

    TRACE("SR: Begin Suspend Processing...\n");

    /* Save CPU state and stop all CPU's */
    TRACE("SR: Stopping All CPUs...\n");
    OBTAIN_INTLOCK(NULL);
    started_mask = sysblk.started_mask;
    while (sysblk.started_mask)
    {
        for (i = 0; i < sysblk.maxcpu; i++)
        {
            if (IS_CPU_ONLINE(i))
            {
                sysblk.regs[i]->cpustate = CPUSTATE_STOPPING;
                ON_IC_INTERRUPT(sysblk.regs[i]);
                signal_condition(&sysblk.regs[i]->intcond);
            }
        }
        RELEASE_INTLOCK(NULL);
        USLEEP(1000);
        OBTAIN_INTLOCK(NULL);
    }
    RELEASE_INTLOCK(NULL);

    /* Wait for I/O queue to clear out */

    TRACE("SR: Waiting for I/O Queue to clear...\n");

    OBTAIN_IOQLOCK();
    {
        while (sysblk.ioq)
        {
            RELEASE_IOQLOCK();
            {
                USLEEP( 1000 );
            }
            OBTAIN_IOQLOCK();
        }
    }
    RELEASE_IOQLOCK();

    /* Wait for active I/Os to complete */
    TRACE("SR: Waiting for Active I/Os to Complete...\n");
    for (i = 1; i < 5000; i++)
    {
        dev = sr_active_devices();
        if (dev == NULL) break;
        if (i % 500 == 0)
        {
            // "SR: waiting for device %04X"
            WRMSG(HHC02002, "W", dev->devnum);
        }
        USLEEP(10000);
    }
    if (dev != NULL)
    {
        // "SR: device %04X still busy, proceeding anyway"
        WRMSG(HHC02003, "W",dev->devnum);
    }

    /* Write header */
    TRACE("SR: Writing File Header...\n");
    SR_WRITE_STRING(file, SR_HDR_ID, SR_ID);
    SR_WRITE_STRING(file, SR_HDR_VERSION, VERSION);
    gettimeofday(&tv, NULL); tt = tv.tv_sec;
    SR_WRITE_STRING(file, SR_HDR_DATE, ctime(&tt));

    /* Write system data */
    TRACE("SR: Saving System Data...\n");
    SR_WRITE_STRING(file,SR_SYS_ARCH_NAME, get_arch_name( NULL ));
    SR_WRITE_VALUE (file,SR_SYS_STARTED_MASK,started_mask,sizeof(started_mask));
    SR_WRITE_VALUE (file,SR_SYS_MAINSIZE,sysblk.mainsize,sizeof(sysblk.mainsize));
    TRACE("SR: Saving MAINSTOR...\n");
    SR_WRITE_BUF   (file,SR_SYS_MAINSTOR,sysblk.mainstor,sysblk.mainsize);
    SR_WRITE_VALUE (file,SR_SYS_SKEYSIZE,(sysblk.mainsize/_STORKEY_ARRAY_UNITSIZE),sizeof(U32));
    TRACE("SR: Saving Storage Keys...\n");
    SR_WRITE_BUF   (file,SR_SYS_STORKEYS,sysblk.storkeys,sysblk.mainsize/_STORKEY_ARRAY_UNITSIZE);
    SR_WRITE_VALUE (file,SR_SYS_XPNDSIZE,sysblk.xpndsize,sizeof(sysblk.xpndsize));
    TRACE("SR: Saving Expanded Storage...\n");
    SR_WRITE_BUF   (file,SR_SYS_XPNDSTOR,sysblk.xpndstor,4096*sysblk.xpndsize);
    SR_WRITE_VALUE (file,SR_SYS_CPUID,sysblk.cpuid,sizeof(sysblk.cpuid));
    SR_WRITE_VALUE (file,SR_SYS_CPUMODEL,sysblk.cpumodel,sizeof(sysblk.cpumodel));
    SR_WRITE_VALUE (file,SR_SYS_CPUVERSION,sysblk.cpuversion,sizeof(sysblk.cpuversion));
    SR_WRITE_VALUE (file,SR_SYS_CPUSERIAL,sysblk.cpuserial,sizeof(sysblk.cpuserial));
    SR_WRITE_VALUE (file,SR_SYS_OPERATION_MODE,(int)sysblk.operation_mode,sizeof(sysblk.operation_mode));
    SR_WRITE_VALUE (file,SR_SYS_LPARMODE,(int)sysblk.lparmode,sizeof(int));
    SR_WRITE_VALUE (file,SR_SYS_LPARNUM,sysblk.lparnum,sizeof(sysblk.lparnum));
    SR_WRITE_VALUE (file,SR_SYS_CPUIDFMT,sysblk.cpuidfmt,sizeof(sysblk.cpuidfmt));
    SR_WRITE_VALUE (file,SR_SYS_IPLDEV,sysblk.ipldev,sizeof(sysblk.ipldev));
    SR_WRITE_VALUE (file,SR_SYS_IPLCPU,sysblk.iplcpu,sizeof(sysblk.iplcpu));
    SR_WRITE_VALUE (file,SR_SYS_MBO,sysblk.mbo,sizeof(sysblk.mbo));
    SR_WRITE_VALUE (file,SR_SYS_MBK,sysblk.mbk,sizeof(sysblk.mbk));
    SR_WRITE_VALUE (file,SR_SYS_MBM,sysblk.mbm,sizeof(sysblk.mbm));
    SR_WRITE_VALUE (file,SR_SYS_MBD,sysblk.mbd,sizeof(sysblk.mbd));

    for (ioq = sysblk.iointq; ioq; ioq = ioq->next)
        if (ioq->pcipending)
        {
            SR_WRITE_VALUE(file,SR_SYS_PCIPENDING_LCSS, SSID_TO_LCSS(ioq->dev->ssid),sizeof(U16));
            SR_WRITE_VALUE(file,SR_SYS_PCIPENDING, ioq->dev->devnum,sizeof(ioq->dev->devnum));
        }
        else if (ioq->attnpending)
        {
            SR_WRITE_VALUE(file,SR_SYS_ATTNPENDING_LCSS, SSID_TO_LCSS(ioq->dev->ssid),sizeof(U16));
            SR_WRITE_VALUE(file,SR_SYS_ATTNPENDING, ioq->dev->devnum,sizeof(ioq->dev->devnum));
        }
        else
        {
            SR_WRITE_VALUE(file,SR_SYS_IOPENDING_LCSS, SSID_TO_LCSS(ioq->dev->ssid),sizeof(U16));
            SR_WRITE_VALUE(file,SR_SYS_IOPENDING, ioq->dev->devnum,sizeof(ioq->dev->devnum));
        }

    SR_WRITE_VALUE ( file, SR_SYS_CRWCOUNT, sysblk.crwcount, sizeof( sysblk.crwcount ));
    if (sysblk.crwcount)
    {
        SR_WRITE_BUF   ( file, SR_SYS_CRWARRAY,  sysblk.crwarray, sizeof( U32 ) * sysblk.crwcount );
        SR_WRITE_VALUE ( file, SR_SYS_CRWINDEX,  sysblk.crwindex, sizeof( sysblk.crwindex ) );
        SR_WRITE_VALUE ( file, SR_SYS_CRWENDIAN, ourendian, 1 );
    }

    SR_WRITE_VALUE (file,SR_SYS_SERVPARM,sysblk.servparm,sizeof(sysblk.servparm));
    SR_WRITE_VALUE (file,SR_SYS_SIGINTREQ,sysblk.sigintreq,1);
    SR_WRITE_VALUE (file,SR_SYS_IPLED,sysblk.ipled,1);
    SR_WRITE_VALUE (file,SR_SYS_SFCMD,sysblk.sfcmd,1);
    SR_WRITE_STRING(file,SR_SYS_LOADPARM,str_loadparm());
    SR_WRITE_VALUE (file,SR_SYS_INTS_STATE,sysblk.ints_state,sizeof(sysblk.ints_state));
    SR_WRITE_HDR(file, SR_DELIMITER, 0);

    /* Save service console state */
    TRACE("SR: Saving Service Console State...\n");
    SR_WRITE_HDR(file, SR_SYS_SERVC, 0);
    servc_hsuspend(file);
    SR_WRITE_HDR(file, SR_DELIMITER, 0);

    /* Save clock state */
    TRACE("SR: Saving Clock State...\n");
    SR_WRITE_HDR(file, SR_SYS_CLOCK, 0);
    clock_hsuspend(file);
    SR_WRITE_HDR(file, SR_DELIMITER, 0);

    /* Write CPU data */
    for (i = 0; i < sysblk.maxcpu; i++)
    {
        if (!IS_CPU_ONLINE(i)) continue;

        TRACE("SR: Saving CPU %d Data...\n", i);

        regs = sysblk.regs[i];
        SR_WRITE_VALUE(file, SR_CPU, i, sizeof(i));
        SR_WRITE_VALUE(file, SR_CPU_ARCHMODE, regs->arch_mode,sizeof(regs->arch_mode));
        SR_WRITE_VALUE(file, SR_CPU_PX, regs->PX_G,sizeof(regs->PX_G));
        copy_psw (regs, psw);
        SR_WRITE_BUF(file, SR_CPU_PSW, psw, 16);
        for (j = 0; j < 16; j++)
            SR_WRITE_VALUE(file, SR_CPU_GR+j, regs->GR_G(j),sizeof(regs->GR_G(0)));
        for (j = 0; j < 16; j++)
            SR_WRITE_VALUE(file, SR_CPU_CR+j, regs->CR_G(j),sizeof(regs->CR_G(0)));
        for (j = 0; j < 16; j++)
            SR_WRITE_VALUE(file, SR_CPU_AR+j, regs->ar[j],sizeof(regs->ar[0]));
        for (j = 0; j < 32; j++)
            SR_WRITE_VALUE(file, SR_CPU_FPR+j, regs->fpr[j],sizeof(regs->fpr[0]));
        SR_WRITE_VALUE(file, SR_CPU_FPC, regs->fpc, sizeof(regs->fpc));
        SR_WRITE_VALUE(file, SR_CPU_DXC, regs->dxc, sizeof(regs->dxc));
        SR_WRITE_VALUE(file, SR_CPU_MC, regs->MC_G, sizeof(regs->MC_G));
        SR_WRITE_VALUE(file, SR_CPU_EA, regs->EA_G, sizeof(regs->EA_G));
        SR_WRITE_VALUE(file, SR_CPU_PTIMER, get_cpu_timer(regs), sizeof(S64));
        SR_WRITE_VALUE(file, SR_CPU_CLKC, regs->clkc, sizeof(regs->clkc));
        SR_WRITE_VALUE(file, SR_CPU_CHANSET, regs->chanset, sizeof(regs->chanset));
        SR_WRITE_VALUE(file, SR_CPU_TODPR, regs->todpr, sizeof(regs->todpr));
        SR_WRITE_VALUE(file, SR_CPU_MONCLASS, regs->monclass, sizeof(regs->monclass));
        SR_WRITE_VALUE(file, SR_CPU_EXCARID, regs->excarid, sizeof(regs->excarid));
        SR_WRITE_VALUE(file, SR_CPU_BEAR, regs->bear, sizeof(regs->bear));
        SR_WRITE_VALUE(file, SR_CPU_OPNDRID, regs->opndrid, sizeof(regs->opndrid));
        SR_WRITE_VALUE(file, SR_CPU_CHECKSTOP, regs->checkstop, 1);
        SR_WRITE_VALUE(file, SR_CPU_HOSTINT, regs->hostint, 1);
        SR_WRITE_VALUE(file, SR_CPU_EXECFLAG, regs->execflag, 1);
//      SR_WRITE_VALUE(file, SR_CPU_INSTVALID, regs->instvalid, 1);
        SR_WRITE_VALUE(file, SR_CPU_PERMODE, regs->permode, 1);
        SR_WRITE_VALUE(file, SR_CPU_LOADSTATE, regs->loadstate, 1);
        SR_WRITE_VALUE(file, SR_CPU_INVALIDATE, regs->invalidate, 1);
        SR_WRITE_VALUE(file, SR_CPU_SIGP_RESET, regs->sigp_reset, 1);
        SR_WRITE_VALUE(file, SR_CPU_SIGP_INI_RESET, regs->sigp_ini_reset, 1);
        SR_WRITE_VALUE(file, SR_CPU_INTS_STATE, regs->ints_state, sizeof(regs->ints_state));
        SR_WRITE_VALUE(file, SR_CPU_INTS_MASK, regs->ints_mask, sizeof(regs->ints_mask));
        for (j = 0; j < sysblk.maxcpu; j++)
            SR_WRITE_VALUE(file, SR_CPU_MALFCPU+j, regs->malfcpu[j], sizeof(regs->malfcpu[0]));
        for (j = 0; j < sysblk.maxcpu; j++)
            SR_WRITE_VALUE(file, SR_CPU_EMERCPU+j, regs->emercpu[j], sizeof(regs->emercpu[0]));
        SR_WRITE_VALUE(file, SR_CPU_EXTCCPU, regs->extccpu, sizeof(regs->extccpu));
        SR_WRITE_HDR(file, SR_DELIMITER, 0);
    }

    /* Write Device data */
    for (dev = sysblk.firstdev; dev; dev = dev->nextdev)
    {
        if (!(dev->pmcw.flag5 & PMCW5_V)) continue;

        TRACE("SR: Saving Device %4.4X...\n", dev->devnum);

        /* These fields must come first so the device could be attached */
        SR_WRITE_VALUE(file, SR_DEV, dev->devnum, sizeof(dev->devnum));
        SR_WRITE_VALUE(file, SR_DEV_LCSS, SSID_TO_LCSS(dev->ssid), sizeof(U16));
        SR_WRITE_VALUE(file, SR_DEV_ARGC, dev->argc, sizeof(dev->argc));
        for (i = 0; i < dev->argc; i++)
            if (dev->argv[i])
            {
                SR_WRITE_STRING(file, SR_DEV_ARGV, dev->argv[i]);
            }
            else
            {
                SR_WRITE_STRING(file, SR_DEV_ARGV, "");
            }
        SR_WRITE_VALUE(file, SR_DEV_NUMCONFDEV, dev->numconfdev, sizeof(dev->numconfdev));
        SR_WRITE_STRING(file, SR_DEV_TYPNAME, dev->typname);

        /* Common device fields */
        SR_WRITE_BUF  (file, SR_DEV_ORB, &dev->orb, sizeof(ORB));
        SR_WRITE_BUF  (file, SR_DEV_PMCW, &dev->pmcw, sizeof(PMCW));
        SR_WRITE_BUF  (file, SR_DEV_SCSW, &dev->scsw, sizeof(SCSW));
        SR_WRITE_BUF  (file, SR_DEV_PCISCSW, &dev->pciscsw, sizeof(SCSW));
        SR_WRITE_BUF  (file, SR_DEV_ATTNSCSW, &dev->attnscsw, sizeof(SCSW));
        SR_WRITE_BUF  (file, SR_DEV_ESW, &dev->esw, sizeof(ESW));
        SR_WRITE_BUF  (file, SR_DEV_ECW, dev->ecw, 32);
        SR_WRITE_BUF  (file, SR_DEV_SENSE, dev->sense, 32);
        SR_WRITE_VALUE(file, SR_DEV_PGSTAT, dev->pgstat, sizeof(dev->pgstat));
        SR_WRITE_BUF  (file, SR_DEV_PGID, dev->pgid, 11);
        /* By Adrian - SR_DEV_DRVPWD */
        SR_WRITE_BUF  (file, SR_DEV_DRVPWD, dev->drvpwd, 11);
        SR_WRITE_VALUE(file, SR_DEV_BUSY, dev->busy, 1);
        SR_WRITE_VALUE(file, SR_DEV_RESERVED, dev->reserved, 1);
        SR_WRITE_VALUE(file, SR_DEV_SUSPENDED, dev->suspended, 1);
        SR_WRITE_VALUE(file, SR_DEV_PCIPENDING, dev->pcipending, 1);
        SR_WRITE_VALUE(file, SR_DEV_ATTNPENDING, dev->attnpending, 1);
        SR_WRITE_VALUE(file, SR_DEV_PENDING, dev->pending, 1);
        SR_WRITE_VALUE(file, SR_DEV_STARTPENDING, dev->startpending, 1);
        SR_WRITE_VALUE(file, SR_DEV_CCWADDR, dev->ccwaddr, sizeof(dev->ccwaddr));
        SR_WRITE_VALUE(file, SR_DEV_IDAPMASK, dev->idapmask, sizeof(dev->idapmask));
        SR_WRITE_VALUE(file, SR_DEV_IDAWFMT, dev->idawfmt, sizeof(dev->idawfmt));
        SR_WRITE_VALUE(file, SR_DEV_CCWFMT, dev->ccwfmt, sizeof(dev->ccwfmt));
        SR_WRITE_VALUE(file, SR_DEV_CCWKEY, dev->ccwkey, sizeof(dev->ccwkey));

        /* Device type specific data */
        SR_WRITE_VALUE(file, SR_DEV_DEVTYPE, dev->devtype, sizeof(dev->devtype));
        if (dev->hnd->hsuspend)
        {
            rc = (dev->hnd->hsuspend) (dev, file);
            if (rc < 0) goto sr_error_exit;
        }
        SR_WRITE_HDR(file, SR_DELIMITER, 0);
    }

    TRACE("SR: Writing EOF\n");

    SR_WRITE_HDR(file, SR_EOF, 0);
    SR_CLOSE (file);

    TRACE("SR: Suspend Complete; shutting down...\n");

    /* Shutdown */
    do_shutdown();

    return 0;

sr_error_exit:
    // "SR: error processing file '%s'"
    WRMSG(HHC02004, "E", fn);
    SR_CLOSE (file);
    return -1;
}

#define SR_NULL_REGS_CHECK(_regs)  if ((_regs) == NULL) goto sr_null_regs_exit;

int resume_cmd(int argc, char *argv[],char *cmdline)
{
char    *fn = SR_DEFAULT_FILENAME;
SR_FILE  file;
U32      key = 0, len = 0;
U64      mainsize = 0;
U64      xpndsize = 0;
CPU_BITMAP started_mask = 0;
int      i, rc = -1;
REGS    *regs = NULL;
U16      devnum=0;
U16      lcss=0;
U16      hw;
int      devargc=0;
char    *devargv[16];
int      devargx=0;
DEVBLK  *dev = NULL;
IOINT   *ioq = NULL;
char     buf[SR_MAX_STRING_LENGTH+1];
char     zeros[16];
S64      dreg;
int      numconfdev=0;

    UNREFERENCED(cmdline);

    if (argc > 2)
    {
        // "SR: too many arguments"
        WRMSG(HHC02000, "E");
        return -1;
    }

    if (argc == 2)
        fn = argv[1];

    memset (zeros, 0, sizeof(zeros));

    TRACE("SR: Begin Resume Processing...\n");

    /* Make sure all CPUs are deconfigured or stopped */
    TRACE("SR: Waiting for CPUs to stop...\n");
    if (!are_all_cpus_stopped())
    {
        // "SR: all processors must be stopped to resume"
        WRMSG(HHC02005, "E");
        return -1;
    }

    file = SR_OPEN (fn, "rb");
    if (file == NULL)
    {
        // "SR: error in function '%s': '%s'"
        WRMSG(HHC02001, "E", "open()",strerror(errno));
        return -1;
    }

    /* First key must be SR_HDR_ID and string must match SR_ID */
    TRACE("SR: Reading File Header...\n");
    SR_READ_HDR(file, key, len);
    if (key == SR_HDR_ID) SR_READ_STRING(file, buf, len);
    if (key != SR_HDR_ID || strcmp(buf, SR_ID))
    {
        // "SR: file identifier error"
        WRMSG(HHC02006, "E");
        goto sr_error_exit;
    }

    /* Deconfigure all CPUs */
    TRACE("SR: Deconfiguring all CPUs...\n");
    OBTAIN_INTLOCK(NULL);
    for (i = 0; i < sysblk.maxcpu; i++)
        if (IS_CPU_ONLINE(i))
            deconfigure_cpu(i);
    RELEASE_INTLOCK(NULL);

    TRACE("SR: Processing Resume File...\n");

    while (key != SR_EOF)
    {
        SR_READ_HDR(file, key, len);
        switch (key) {

        case SR_HDR_DATE:
            SR_READ_STRING(file, buf, len);
            if (len >= 2)
            {
                len -= 2;
                while (len > 0 && isspace((unsigned char)buf[len]))
                    --len;
                buf[len+1]=0;
            }
            // "SR: resuming suspended file created on '%s'"
            WRMSG(HHC02007, "I", buf);
            break;

        case SR_SYS_STARTED_MASK:
            SR_READ_VALUE(file, len, &started_mask, sizeof(started_mask));
            break;

        case SR_SYS_ARCH_NAME:
        {
            int save_sysblk_arch_mode = sysblk.arch_mode;

            SR_READ_STRING( file, buf, len );

            for (i=0; i < NUM_GEN_ARCHS; i++)
            {
                sysblk.arch_mode = i; // (is it this archmode?)
                if (strcasecmp( buf, get_arch_name( NULL )) == 0)
                    break; // (yep! found it!)
            }

            if (i >= NUM_GEN_ARCHS)  // (did we find it?)
            {
                sysblk.arch_mode = save_sysblk_arch_mode;
                // "SR: archmode '%s' not supported"
                WRMSG(HHC02008, "E", buf);
                goto sr_error_exit;
            }

            sysblk.pcpu = 0;
            sysblk.dummyregs.arch_mode = sysblk.arch_mode;
        }
        break;

        case SR_SYS_MAINSIZE:
            SR_READ_VALUE(file, len, &mainsize, sizeof(mainsize));
            if (mainsize > sysblk.mainsize)
            {
                char buf1[20];
                char buf2[20];
                MSGBUF(buf1, "%dM", (U32)(mainsize / (1024*1024)));
                MSGBUF(buf2, "%dM", (U32)(sysblk.mainsize / (1024*1024)));
                // "SR: mismatch in '%s': '%s' found, '%s' expected"
                WRMSG(HHC02009, "E", "mainsize", buf1, buf2);
                goto sr_error_exit;
            }
            break;

        case SR_SYS_MAINSTOR:
            TRACE("SR: Restoring MAINSTOR...\n");
            SR_READ_BUF(file, sysblk.mainstor, mainsize);
            break;

        case SR_SYS_SKEYSIZE:
            SR_READ_VALUE(file, len, &len, sizeof(len));
            if (len > (U32)(sysblk.mainsize/_STORKEY_ARRAY_UNITSIZE))
            {
                char buf1[20];
                char buf2[20];
                MSGBUF(buf1, "%d", len);
                MSGBUF(buf2, "%d", (U32)(sysblk.mainsize/_STORKEY_ARRAY_UNITSIZE));
                // "SR: mismatch in '%s': '%s' found, '%s' expected"
                WRMSG(HHC02009, "E", "storkey size", buf1, buf2);
                goto sr_error_exit;
            }
            break;

        case SR_SYS_STORKEYS:
            TRACE("SR: Restoring Storage Keys...\n");
            SR_READ_BUF(file, sysblk.storkeys, len);
            break;

        case SR_SYS_XPNDSIZE:
            SR_READ_VALUE(file, len, &xpndsize, sizeof(xpndsize));
            if (xpndsize > sysblk.xpndsize)
            {
                char buf1[20];
                char buf2[20];
                MSGBUF(buf1, "%dM", (U32)(xpndsize / (256)));
                MSGBUF(buf2, "%dM", sysblk.xpndsize / (256));
                // "SR: mismatch in '%s': '%s' found, '%s' expected"
                WRMSG(HHC02009, "E", "expand size", buf1, buf2);
                goto sr_error_exit;
            }
            break;

        case SR_SYS_XPNDSTOR:
            TRACE("SR: Restoring Expanded Storage...\n");
            SR_READ_BUF(file, sysblk.xpndstor, xpndsize * 4096);
            break;

        case SR_SYS_CPUID:
            SR_READ_VALUE(file, len, &sysblk.cpuid, sizeof(sysblk.cpuid));
             break;

        case SR_SYS_CPUMODEL:
            SR_READ_VALUE(file, len, &sysblk.cpumodel, sizeof(sysblk.cpumodel));
             break;

        case SR_SYS_CPUVERSION:
            SR_READ_VALUE(file, len, &sysblk.cpuversion, sizeof(sysblk.cpuversion));
            break;

        case SR_SYS_CPUSERIAL:
            SR_READ_VALUE(file, len, &sysblk.cpuserial, sizeof(sysblk.cpuserial));
            break;

        case SR_SYS_OPERATION_MODE:
            SR_READ_VALUE (file, len, &sysblk.operation_mode, sizeof(sysblk.operation_mode));
            break;

        case SR_SYS_LPARMODE:
        {
            int value;
            SR_READ_VALUE (file, len, &value, sizeof(int));
            sysblk.lparmode = value;
            break;
        }

        case SR_SYS_LPARNUM:
            SR_READ_VALUE (file, len, &sysblk.lparnum, sizeof(sysblk.lparnum));
            break;

        case SR_SYS_CPUIDFMT:
            SR_READ_VALUE (file, len, &sysblk.cpuidfmt, sizeof(sysblk.cpuidfmt));
            break;

        case SR_SYS_IPLDEV:
            SR_READ_VALUE(file, len, &sysblk.ipldev, sizeof(sysblk.ipldev));
            break;

        case SR_SYS_IPLCPU:
            SR_READ_VALUE(file, len, &sysblk.iplcpu, sizeof(sysblk.iplcpu));
            break;

        case SR_SYS_MBO:
            SR_READ_VALUE(file, len, &sysblk.mbo, sizeof(sysblk.mbo));
            break;

        case SR_SYS_MBK:
            SR_READ_VALUE(file, len, &sysblk.mbk, sizeof(sysblk.mbk));
            break;

        case SR_SYS_MBM:
            SR_READ_VALUE(file, len, &sysblk.mbm, sizeof(sysblk.mbm));
            break;

        case SR_SYS_MBD:
            SR_READ_VALUE(file, len, &sysblk.mbd, sizeof(sysblk.mbd));
            break;

        case SR_SYS_IOPENDING_LCSS:
            SR_READ_VALUE(file,len,&lcss,sizeof(lcss));
            break;

        case SR_SYS_IOPENDING:
            SR_READ_VALUE(file, len, &hw, sizeof(hw));
            dev = find_device_by_devnum(lcss,hw);
            if (dev == NULL) break;
            if (ioq == NULL)
                sysblk.iointq = &dev->ioint;
            else
                ioq->next = &dev->ioint;
            ioq = &dev->ioint;
            dev = NULL;
            lcss = 0;
            break;

        case SR_SYS_PCIPENDING_LCSS:
            SR_READ_VALUE(file,len,&lcss,sizeof(lcss));
            break;

        case SR_SYS_PCIPENDING:
            SR_READ_VALUE(file, len, &hw, sizeof(hw));
            dev = find_device_by_devnum(lcss,hw);
            if (dev == NULL) break;
            if (ioq == NULL)
                sysblk.iointq = &dev->pciioint;
            else
                ioq->next = &dev->pciioint;
            ioq = &dev->pciioint;
            dev = NULL;
            lcss = 0;
            break;

        case SR_SYS_ATTNPENDING_LCSS:
            SR_READ_VALUE(file,len,&lcss,sizeof(lcss));
            break;

        case SR_SYS_ATTNPENDING:
            SR_READ_VALUE(file, len, &hw, sizeof(hw));
            dev = find_device_by_devnum(lcss,hw);
            if (dev == NULL) break;
            if (ioq == NULL)
                sysblk.iointq = &dev->attnioint;
            else
                ioq->next = &dev->attnioint;
            ioq = &dev->attnioint;
            dev = NULL;
            lcss = 0;
            break;

        case SR_SYS_CRWCOUNT:
            SR_READ_VALUE( file, len, &sysblk.crwcount, sizeof( sysblk.crwcount ));
            if (sysblk.crwcount)
            {
                sysblk.crwarray = malloc( sizeof(U32) * sysblk.crwcount );
                if (!sysblk.crwarray)
                {
                    // "SR: error loading CRW queue: not enough memory for %d CRWs"
                    WRMSG(HHC02022, "E", sysblk.crwcount);
                    goto sr_error_exit;
                }
                sysblk.crwalloc = sysblk.crwcount;
            }
            break;

        case SR_SYS_CRWARRAY:
            ASSERT( sysblk.crwarray && sysblk.crwcount );
            TRACE("SR: Restoring CRW Array...\n");
            SR_READ_BUF( file, sysblk.crwarray, sizeof(U32) * sysblk.crwcount );
            break;

        case SR_SYS_CRWINDEX:
            ASSERT( sysblk.crwarray && sysblk.crwcount );
            SR_READ_VALUE( file, len, &sysblk.crwindex, sizeof( sysblk.crwindex ));
            break;

        case SR_SYS_CRWENDIAN:
            ASSERT( sysblk.crwarray && sysblk.crwcount );
            SR_READ_VALUE(file, len, &crwendian, 1);
            if (ourendian != crwendian)
                for (i=0; (U32)i < sysblk.crwcount; i++)
                    *(sysblk.crwarray + i) = bswap_32(*(sysblk.crwarray + i));
            break;

        case SR_SYS_SERVPARM:
            SR_READ_VALUE(file, len, &sysblk.servparm, sizeof(sysblk.servparm));
            break;

        case SR_SYS_SIGINTREQ:
            SR_READ_VALUE(file, len, &rc, sizeof(rc));
            sysblk.sigintreq = rc;
            break;

        case SR_SYS_IPLED:
            SR_READ_VALUE(file, len, &rc, sizeof(rc));
            sysblk.ipled = rc;
            break;

        case SR_SYS_SFCMD:
            SR_READ_VALUE(file, len, &rc, sizeof(rc));
            sysblk.sfcmd = rc;
            break;

        case SR_SYS_LOADPARM:
            SR_READ_STRING(file, buf, len);
            set_loadparm ((char *)buf);
            break;

        case SR_SYS_SERVC:
            TRACE("SR: Restoring Service Console State...\n");
            rc = servc_hresume(file);
            if (rc < 0) goto sr_error_exit;
            break;

        case SR_SYS_CLOCK:
            TRACE("SR: Restoring Clock State...\n");
            rc = clock_hresume(file);
            if (rc < 0) goto sr_error_exit;
            break;

        case SR_CPU:
            SR_READ_VALUE(file, len, &i, sizeof(i));
            TRACE("SR: Restoring CPU %d Data...\n", i);
            if (i >= sysblk.maxcpu)
            {
                // "SR: processor CP%02X exceeds max allowed CP%02X"
                WRMSG(HHC02010, "E", i, sysblk.maxcpu-1);
                goto sr_error_exit;
            }
            OBTAIN_INTLOCK(NULL);
            if (IS_CPU_ONLINE(i))
            {
                RELEASE_INTLOCK(NULL);
                // "SR: processor %s%02X already configured"
                WRMSG(HHC02011, "E", PTYPSTR(i), i);
                goto sr_error_exit;
            }
            rc = configure_cpu(i);
            RELEASE_INTLOCK(NULL);
            if (rc < 0)
            {
                // "SR: processor %s%02X unable to configure online"
                WRMSG(HHC02012, "E", PTYPSTR(i), i);
                goto sr_error_exit;
            }
            regs = sysblk.regs[i];
            break;

        case SR_CPU_PX:
            SR_NULL_REGS_CHECK(regs);
            SR_READ_VALUE(file, len, &regs->px, sizeof(regs->px));
            break;

        case SR_CPU_PSW:
            SR_NULL_REGS_CHECK(regs);
            if (len != 8 && len != 16)
            {
                // "SR: processor %s%02X invalid psw length %d"
                WRMSG(HHC02013, "E", PTYPSTR(regs->cpuad), regs->cpuad, len);
                goto sr_error_exit;
            }
            memset(buf, 0, 16);
            SR_READ_BUF(file, buf, len);
            switch (regs->arch_mode) {
#if defined (_370)
            case ARCH_370_IDX:
                len = 8;
                rc = s370_load_psw(regs, (BYTE *)&buf);
                break;
#endif
#if defined (_390)
            case ARCH_390_IDX:
                len = 8;
                rc = s390_load_psw(regs, (BYTE *)&buf);
                break;
#endif
#if defined (_900)
            case ARCH_900_IDX:
                len = 16;
                rc = z900_load_psw(regs, (BYTE *)&buf);
                break;
#endif
            default: CRASH();
            } /* switch (regs->arch_mode) */
            if (rc != 0 && memcmp(buf, zeros, len))
            {
                // "SR: processor %s%02X error loading psw, rc %d"
                WRMSG(HHC02014, "E", PTYPSTR(regs->cpuad), regs->cpuad, rc);
                goto sr_error_exit;
            }
            break;

        case SR_CPU_GR_0:
        case SR_CPU_GR_1:
        case SR_CPU_GR_2:
        case SR_CPU_GR_3:
        case SR_CPU_GR_4:
        case SR_CPU_GR_5:
        case SR_CPU_GR_6:
        case SR_CPU_GR_7:
        case SR_CPU_GR_8:
        case SR_CPU_GR_9:
        case SR_CPU_GR_10:
        case SR_CPU_GR_11:
        case SR_CPU_GR_12:
        case SR_CPU_GR_13:
        case SR_CPU_GR_14:
        case SR_CPU_GR_15:
            SR_NULL_REGS_CHECK(regs);
            i = key - SR_CPU_GR;
            SR_READ_VALUE(file, len, &regs->gr[i], sizeof(regs->gr[0]));
            break;

        case SR_CPU_CR_0:
        case SR_CPU_CR_1:
        case SR_CPU_CR_2:
        case SR_CPU_CR_3:
        case SR_CPU_CR_4:
        case SR_CPU_CR_5:
        case SR_CPU_CR_6:
        case SR_CPU_CR_7:
        case SR_CPU_CR_8:
        case SR_CPU_CR_9:
        case SR_CPU_CR_10:
        case SR_CPU_CR_11:
        case SR_CPU_CR_12:
        case SR_CPU_CR_13:
        case SR_CPU_CR_14:
        case SR_CPU_CR_15:
            SR_NULL_REGS_CHECK(regs);
            i = key - SR_CPU_CR;
            SR_READ_VALUE(file, len, &regs->CR(i), sizeof(regs->CR(0)));
            break;

        case SR_CPU_AR_0:
        case SR_CPU_AR_1:
        case SR_CPU_AR_2:
        case SR_CPU_AR_3:
        case SR_CPU_AR_4:
        case SR_CPU_AR_5:
        case SR_CPU_AR_6:
        case SR_CPU_AR_7:
        case SR_CPU_AR_8:
        case SR_CPU_AR_9:
        case SR_CPU_AR_10:
        case SR_CPU_AR_11:
        case SR_CPU_AR_12:
        case SR_CPU_AR_13:
        case SR_CPU_AR_14:
        case SR_CPU_AR_15:
            SR_NULL_REGS_CHECK(regs);
            i = key - SR_CPU_AR;
            SR_READ_VALUE(file, len, &regs->ar[i], sizeof(regs->ar[0]));
            break;

        case SR_CPU_FPR_0:
        case SR_CPU_FPR_1:
        case SR_CPU_FPR_2:
        case SR_CPU_FPR_3:
        case SR_CPU_FPR_4:
        case SR_CPU_FPR_5:
        case SR_CPU_FPR_6:
        case SR_CPU_FPR_7:
        case SR_CPU_FPR_8:
        case SR_CPU_FPR_9:
        case SR_CPU_FPR_10:
        case SR_CPU_FPR_11:
        case SR_CPU_FPR_12:
        case SR_CPU_FPR_13:
        case SR_CPU_FPR_14:
        case SR_CPU_FPR_15:
        case SR_CPU_FPR_16:
        case SR_CPU_FPR_17:
        case SR_CPU_FPR_18:
        case SR_CPU_FPR_19:
        case SR_CPU_FPR_20:
        case SR_CPU_FPR_21:
        case SR_CPU_FPR_22:
        case SR_CPU_FPR_23:
        case SR_CPU_FPR_24:
        case SR_CPU_FPR_25:
        case SR_CPU_FPR_26:
        case SR_CPU_FPR_27:
        case SR_CPU_FPR_28:
        case SR_CPU_FPR_29:
        case SR_CPU_FPR_30:
        case SR_CPU_FPR_31:
            SR_NULL_REGS_CHECK(regs);
            i = key - SR_CPU_FPR;
            SR_READ_VALUE(file, len, &regs->fpr[i], sizeof(regs->fpr[0]));
            break;

        case SR_CPU_FPC:
            SR_NULL_REGS_CHECK(regs);
            SR_READ_VALUE(file, len, &regs->fpc, sizeof(regs->fpc));
            break;

        case SR_CPU_DXC:
            SR_NULL_REGS_CHECK(regs);
            SR_READ_VALUE(file, len, &regs->dxc, sizeof(regs->dxc));
            break;

        case SR_CPU_MC:
            SR_NULL_REGS_CHECK(regs);
            SR_READ_VALUE(file, len, &regs->mc, sizeof(regs->mc));
            break;

        case SR_CPU_EA:
            SR_NULL_REGS_CHECK(regs);
            SR_READ_VALUE(file, len, &regs->ea, sizeof(regs->ea));
            break;

        case SR_CPU_PTIMER:
            SR_NULL_REGS_CHECK(regs);
            SR_READ_VALUE(file, len, &dreg, sizeof(S64));
            set_cpu_timer(regs, dreg);
            break;

        case SR_CPU_CLKC:
            SR_NULL_REGS_CHECK(regs);
            SR_READ_VALUE(file, len, &regs->clkc, sizeof(regs->clkc));
            break;

        case SR_CPU_CHANSET:
            SR_NULL_REGS_CHECK(regs);
            SR_READ_VALUE(file, len, &regs->chanset, sizeof(regs->chanset));
            break;

        case SR_CPU_TODPR:
            SR_NULL_REGS_CHECK(regs);
            SR_READ_VALUE(file, len, &regs->todpr, sizeof(regs->todpr));
            break;

        case SR_CPU_MONCLASS:
            SR_NULL_REGS_CHECK(regs);
            SR_READ_VALUE(file, len, &regs->monclass, sizeof(regs->monclass));
            break;

        case SR_CPU_EXCARID:
            SR_NULL_REGS_CHECK(regs);
            SR_READ_VALUE(file, len, &regs->excarid, sizeof(regs->excarid));
            break;

        case SR_CPU_BEAR:
            SR_NULL_REGS_CHECK(regs);
            SR_READ_VALUE(file, len, &regs->bear, sizeof(regs->bear));
            break;

        case SR_CPU_OPNDRID:
            SR_NULL_REGS_CHECK(regs);
            SR_READ_VALUE(file, len, &regs->opndrid, sizeof(regs->opndrid));
            break;

        case SR_CPU_CHECKSTOP:
            SR_NULL_REGS_CHECK(regs);
            SR_READ_VALUE(file, len, &rc, sizeof(rc));
            regs->checkstop = rc;
            break;

        case SR_CPU_HOSTINT:
            SR_NULL_REGS_CHECK(regs);
            SR_READ_VALUE(file, len, &rc, sizeof(rc));
            regs->hostint = rc;
            break;

        case SR_CPU_LOADSTATE:
            SR_NULL_REGS_CHECK(regs);
            SR_READ_VALUE(file, len, &rc, sizeof(rc));
            regs->loadstate = rc;
            break;

        case SR_CPU_INVALIDATE:
            SR_NULL_REGS_CHECK(regs);
            SR_READ_VALUE(file, len, &rc, sizeof(rc));
            regs->invalidate = rc;
            break;

        case SR_CPU_SIGP_RESET:
            SR_NULL_REGS_CHECK(regs);
            SR_READ_VALUE(file, len, &rc, sizeof(rc));
            regs->sigp_reset = rc;
            break;

        case SR_CPU_SIGP_INI_RESET:
            SR_NULL_REGS_CHECK(regs);
            SR_READ_VALUE(file, len, &rc, sizeof(rc));
            regs->sigp_ini_reset = rc;
            break;

        case SR_CPU_INTS_STATE:
            SR_NULL_REGS_CHECK(regs);
            SR_READ_VALUE(file, len, &regs->ints_state, sizeof(regs->ints_state));
            /* Force CPU to examine the interrupt state */
            ON_IC_INTERRUPT(regs);
            break;

        case SR_CPU_INTS_MASK:
            SR_NULL_REGS_CHECK(regs);
            SR_READ_VALUE(file, len, &regs->ints_mask, sizeof(regs->ints_mask));
            break;

        case SR_CPU_MALFCPU_0:
        case SR_CPU_MALFCPU_1:
        case SR_CPU_MALFCPU_2:
        case SR_CPU_MALFCPU_3:
        case SR_CPU_MALFCPU_4:
        case SR_CPU_MALFCPU_5:
        case SR_CPU_MALFCPU_6:
        case SR_CPU_MALFCPU_7:
        case SR_CPU_MALFCPU_8:
        case SR_CPU_MALFCPU_9:
        case SR_CPU_MALFCPU_10:
        case SR_CPU_MALFCPU_11:
        case SR_CPU_MALFCPU_12:
        case SR_CPU_MALFCPU_13:
        case SR_CPU_MALFCPU_14:
        case SR_CPU_MALFCPU_15:
        case SR_CPU_MALFCPU_16:
        case SR_CPU_MALFCPU_17:
        case SR_CPU_MALFCPU_18:
        case SR_CPU_MALFCPU_19:
        case SR_CPU_MALFCPU_20:
        case SR_CPU_MALFCPU_21:
        case SR_CPU_MALFCPU_22:
        case SR_CPU_MALFCPU_23:
        case SR_CPU_MALFCPU_24:
        case SR_CPU_MALFCPU_25:
        case SR_CPU_MALFCPU_26:
        case SR_CPU_MALFCPU_27:
        case SR_CPU_MALFCPU_28:
        case SR_CPU_MALFCPU_29:
        case SR_CPU_MALFCPU_30:
        case SR_CPU_MALFCPU_31:
            SR_NULL_REGS_CHECK(regs);
            i = key - SR_CPU_MALFCPU;
            if (i < sysblk.maxcpu)
                SR_READ_VALUE(file, len, &regs->malfcpu[i], sizeof(regs->malfcpu[0]));
            break;

        case SR_CPU_EMERCPU_0:
        case SR_CPU_EMERCPU_1:
        case SR_CPU_EMERCPU_2:
        case SR_CPU_EMERCPU_3:
        case SR_CPU_EMERCPU_4:
        case SR_CPU_EMERCPU_5:
        case SR_CPU_EMERCPU_6:
        case SR_CPU_EMERCPU_7:
        case SR_CPU_EMERCPU_8:
        case SR_CPU_EMERCPU_9:
        case SR_CPU_EMERCPU_10:
        case SR_CPU_EMERCPU_11:
        case SR_CPU_EMERCPU_12:
        case SR_CPU_EMERCPU_13:
        case SR_CPU_EMERCPU_14:
        case SR_CPU_EMERCPU_15:
        case SR_CPU_EMERCPU_16:
        case SR_CPU_EMERCPU_17:
        case SR_CPU_EMERCPU_18:
        case SR_CPU_EMERCPU_19:
        case SR_CPU_EMERCPU_20:
        case SR_CPU_EMERCPU_21:
        case SR_CPU_EMERCPU_22:
        case SR_CPU_EMERCPU_23:
        case SR_CPU_EMERCPU_24:
        case SR_CPU_EMERCPU_25:
        case SR_CPU_EMERCPU_26:
        case SR_CPU_EMERCPU_27:
        case SR_CPU_EMERCPU_28:
        case SR_CPU_EMERCPU_29:
        case SR_CPU_EMERCPU_30:
        case SR_CPU_EMERCPU_31:
            SR_NULL_REGS_CHECK(regs);
            i = key - SR_CPU_EMERCPU;
            if (i < sysblk.maxcpu)
                SR_READ_VALUE(file, len, &regs->emercpu[i], sizeof(regs->emercpu[0]));
            break;

        case SR_DEV:
            SR_READ_VALUE(file, len, &devnum, sizeof(devnum));
            TRACE("SR: Restoring Device %4.4X...\n", devnum);
            lcss=0;
            break;

        case SR_DEV_LCSS:
            SR_READ_VALUE(file, len, &lcss, sizeof(U16));
            break;

        case SR_DEV_ARGC:
            SR_READ_VALUE(file, len, &devargc, sizeof(devargc));
            if (devargc > 16) devargc = 16;
            for (i = 0; i < devargc; i++) devargv[i] = NULL;
            devargx = 0;
            break;

        case SR_DEV_NUMCONFDEV:
            SR_READ_VALUE(file, len, &numconfdev, sizeof(numconfdev));
            break;

        case SR_DEV_ARGV:
            SR_READ_STRING(file, buf, len);
            if (devargx < devargc) devargv[devargx++] = strdup(buf);
            break;

        case SR_DEV_TYPNAME:
            SR_READ_STRING(file, buf, len);
            dev = find_device_by_devnum(lcss,devnum);
            if (dev == NULL)
            {
                if (numconfdev == 0) numconfdev = 1;
                if (attach_device (lcss, devnum, buf, devargc, devargv, numconfdev))
                {
                    // "SR: %04X: device initialization failed"
                    WRMSG(HHC02015, "E", devnum);
                }
                numconfdev = 0;
            }
            else if (strcmp(dev->typname, buf))
            {
                // "SR: %04X: device type mismatch; '%s' found, '%s' expected"
                WRMSG(HHC02016, "W", devnum, buf, dev->typname);
                dev = NULL;
            }
            for (i = 0; i < devargx; i++)
            {
                if (devargv[i]) free(devargv[i]);
                devargv[i] = NULL;
            }
            devnum = devargc = devargx = 0;
            break;

        case SR_DEV_ORB:
            SR_SKIP_NULL_DEV(dev, file, len);
            if (len != sizeof(ORB))
            {
                // "SR: %04X: '%s' size mismatch: %d found, %d expected"
                WRMSG(HHC02017, "E", dev->devnum, "ORB", len, (int)sizeof(ORB));
                goto sr_error_exit;
            }
            SR_READ_BUF(file, &dev->orb, len);
            break;

        case SR_DEV_PMCW:
            SR_SKIP_NULL_DEV(dev, file, len);
            if (len != sizeof(PMCW))
            {
                // "SR: %04X: '%s' size mismatch: %d found, %d expected"
                WRMSG(HHC02017, "E", dev->devnum, "PMCW", len, (int)sizeof(PMCW));
                goto sr_error_exit;
            }
            SR_READ_BUF(file, &dev->pmcw, len);
            break;

        case SR_DEV_SCSW:
            SR_SKIP_NULL_DEV(dev, file, len);
            if (len != sizeof(SCSW))
            {
                // "SR: %04X: '%s' size mismatch: %d found, %d expected"
                WRMSG(HHC02017, "E", dev->devnum, "SCSW", len, (int)sizeof(SCSW));
                goto sr_error_exit;
            }
            SR_READ_BUF(file, &dev->scsw, len);
            break;

        case SR_DEV_PCISCSW:
            SR_SKIP_NULL_DEV(dev, file, len);
            if (len != sizeof(SCSW))
            {
                // "SR: %04X: '%s' size mismatch: %d found, %d expected"
                WRMSG(HHC02017, "E", dev->devnum, "PCI SCSW", len, (int)sizeof(SCSW));
                goto sr_error_exit;
            }
            SR_READ_BUF(file, &dev->pciscsw, len);
            break;

        case SR_DEV_ATTNSCSW:
            SR_SKIP_NULL_DEV(dev, file, len);
            if (len != sizeof(SCSW))
            {
                // "SR: %04X: '%s' size mismatch: %d found, %d expected"
                WRMSG(HHC02017, "E", dev->devnum, "ATTN SCSW", len, (int)sizeof(SCSW));
                goto sr_error_exit;
            }
            SR_READ_BUF(file, &dev->attnscsw, len);
            break;

        case SR_DEV_ESW:
            SR_SKIP_NULL_DEV(dev, file, len);
            if (len != sizeof(ESW))
            {
                // "SR: %04X: '%s' size mismatch: %d found, %d expected"
                WRMSG(HHC02017, "E", dev->devnum, "ESW", len, (int)sizeof(ESW));
                goto sr_error_exit;
            }
            SR_READ_BUF(file, &dev->esw, len);
            break;

        case SR_DEV_ECW:
            SR_SKIP_NULL_DEV(dev, file, len);
            if (len != 32)
            {
                // "SR: %04X: '%s' size mismatch: %d found, %d expected"
                WRMSG(HHC02017, "E", dev->devnum, "ECW", len, 32);
                goto sr_error_exit;
            }
            SR_READ_BUF(file, &dev->ecw, len);
            break;

        case SR_DEV_SENSE:
            SR_SKIP_NULL_DEV(dev, file, len);
            if (len != 32)
            {
                // "SR: %04X: '%s' size mismatch: %d found, %d expected"
                WRMSG(HHC02017, "E", dev->devnum, "Sense", len, 32);
                goto sr_error_exit;
            }
            SR_READ_BUF(file, &dev->ecw, len);
            break;

        case SR_DEV_PGSTAT:
            SR_SKIP_NULL_DEV(dev, file, len);
            SR_READ_VALUE(file, len, &dev->pgstat, sizeof(dev->pgstat));
            break;

        case SR_DEV_PGID:
            SR_SKIP_NULL_DEV(dev, file, len);
            if (len != 11)
            {
                // "SR: %04X: '%s' size mismatch: %d found, %d expected"
                WRMSG(HHC02017, "E", dev->devnum, "PGID", len, 11);
                goto sr_error_exit;
            }
            SR_READ_BUF(file, &dev->pgid, len);
            break;

        /* By Adrian - SR_DEV_DRVPWD */
        case SR_DEV_DRVPWD:
            SR_SKIP_NULL_DEV(dev, file, len);
            if (len != 11)
            {
                // "SR: %04X: '%s' size mismatch: %d found, %d expected"
                WRMSG(HHC02017, "E", dev->devnum, "DRVPWD", len, 11);
                goto sr_error_exit;
            }
            SR_READ_BUF(file, &dev->drvpwd, len);
            break;

        case SR_DEV_BUSY:
            SR_SKIP_NULL_DEV(dev, file, len);
            SR_READ_VALUE(file, len, &rc, sizeof(rc));
            dev->busy = rc;
            break;

        case SR_DEV_RESERVED:
            SR_SKIP_NULL_DEV(dev, file, len);
            SR_READ_VALUE(file, len, &rc, sizeof(rc));
            dev->reserved = rc;
            break;

        case SR_DEV_SUSPENDED:
            SR_SKIP_NULL_DEV(dev, file, len);
            SR_READ_VALUE(file, len, &rc, sizeof(rc));
            dev->suspended = rc;
            break;

        case SR_DEV_PENDING:
            SR_SKIP_NULL_DEV(dev, file, len);
            SR_READ_VALUE(file, len, &rc, sizeof(rc));
            dev->pending = rc;
            QUEUE_IO_INTERRUPT(&dev->ioint,FALSE);
            break;

        case SR_DEV_PCIPENDING:
            SR_SKIP_NULL_DEV(dev, file, len);
            SR_READ_VALUE(file, len, &rc, sizeof(rc));
            dev->pcipending = rc;
            QUEUE_IO_INTERRUPT(&dev->pciioint,FALSE);
            break;

        case SR_DEV_ATTNPENDING:
            SR_SKIP_NULL_DEV(dev, file, len);
            SR_READ_VALUE(file, len, &rc, sizeof(rc));
            dev->attnpending = rc;
            QUEUE_IO_INTERRUPT(&dev->attnioint,FALSE);
            break;

        case SR_DEV_STARTPENDING:
            SR_SKIP_NULL_DEV(dev, file, len);
            SR_READ_VALUE(file, len, &rc, sizeof(rc));
            dev->startpending = rc;
            break;

        case SR_DEV_CCWADDR:
            SR_SKIP_NULL_DEV(dev, file, len);
            SR_READ_VALUE(file, len, &dev->ccwaddr, sizeof(dev->ccwaddr));
            break;

        case SR_DEV_IDAPMASK:
            SR_SKIP_NULL_DEV(dev, file, len);
            SR_READ_VALUE(file, len, &dev->idapmask, sizeof(dev->idapmask));
            break;

        case SR_DEV_IDAWFMT:
            SR_SKIP_NULL_DEV(dev, file, len);
            SR_READ_VALUE(file, len, &dev->idawfmt, sizeof(dev->idawfmt));
            break;

        case SR_DEV_CCWFMT:
            SR_SKIP_NULL_DEV(dev, file, len);
            SR_READ_VALUE(file, len, &dev->ccwfmt, sizeof(dev->ccwfmt));
            break;

        case SR_DEV_CCWKEY:
            SR_SKIP_NULL_DEV(dev, file, len);
            SR_READ_VALUE(file, len, &dev->ccwkey, sizeof(dev->ccwkey));
            break;

        /* This is the trigger to call the device dependent resume routine */
        case SR_DEV_DEVTYPE:
            SR_SKIP_NULL_DEV(dev, file, len);
            SR_READ_VALUE(file, len, &hw, sizeof(hw));
            if (hw != dev->devtype)
            {
                char buf1[20];
                char buf2[20];
                MSGBUF(buf1, "%04X", hw);
                MSGBUF(buf2, "%04X", dev->devtype);
                // "SR: %04X: device type mismatch; '%s' found, '%s' expected"
                WRMSG(HHC02016, "E", dev->devnum, buf1, buf2);
                goto sr_error_exit;
            }
            if (dev->hnd->hresume)
            {
                rc = (dev->hnd->hresume) (dev, file);
                if (rc < 0) goto sr_error_exit;
            }
            break;

        default:
            if ((key & SR_KEY_ID_MASK) != SR_KEY_ID)
            {
                // "SR: invalid key %8.8X"
                WRMSG(HHC02018, "E", key);
                goto sr_error_exit;
            }
            SR_READ_SKIP(file, len);
            break;

        } /* switch (key) */

    } /* while (key != SR_EOF) */

    TRACE("SR: Resume File Processing Complete...\n");
    TRACE("SR: Resuming Devices...\n");

    /* For all suspended devices, resume the `suspended' state */
    for (dev = sysblk.firstdev; dev; dev = dev->nextdev)
    {
        if (dev->suspended && (dev->pmcw.flag5 & PMCW5_V))
        {
            dev->resumesuspended=1;
            switch (sysblk.arch_mode) {
#if defined(_370)
            case ARCH_370_IDX:
                rc = create_thread (&dev->tid, DETACHED,
                                    s370_execute_ccw_chain, dev, "device thread");
                break;
#endif
#if defined(_390)
            case ARCH_390_IDX:
                rc = create_thread (&dev->tid, DETACHED,
                                    s390_execute_ccw_chain, dev, "device thread");
                break;
#endif
#if defined(_900)
            case ARCH_900_IDX:
                rc = create_thread (&dev->tid, DETACHED,
                                    z900_execute_ccw_chain, dev, "device thread");
                break;
#endif
            default: CRASH();
            } /* switch (sysblk.arch_mode) */
            if (rc != 0)
            {
                // "Error in function create_thread(): %s"
                WRMSG(HHC00102, "E", strerror(rc));
                goto sr_error_exit;
            }
        } /* If suspended device */
    } /* For each device */

    /* Indicate crw pending for any new devices */
#if defined(_370)
    if (sysblk.arch_mode != ARCH_370_IDX)
#endif
    machine_check_crwpend();

    /* Start the CPUs */
    TRACE("SR: Resuming CPUs...\n");
    OBTAIN_INTLOCK(NULL);
    ON_IC_IOPENDING;
    for (i = 0; i < sysblk.maxcpu; i++)
        if (IS_CPU_ONLINE(i) && (started_mask & CPU_BIT(i)))
        {
            sysblk.regs[i]->opinterv = 0;
            sysblk.regs[i]->cpustate = CPUSTATE_STARTED;
            sysblk.regs[i]->checkstop = 0;
            WAKEUP_CPU(sysblk.regs[i]);
        }
    RELEASE_INTLOCK(NULL);

    TRACE("SR: Resume Complete; System Resumed.\n");
    return 0;

sr_null_regs_exit:
    // "SR: CPU key %8.8X found but no active CPU"
    WRMSG(HHC02019, "E", key);
    goto sr_error_exit;
sr_error_exit:
    // "SR: error processing file '%s'"
    WRMSG(HHC02004, "E", fn);
    SR_CLOSE (file);
    return -1;
}

#if defined( _MSVC_ ) && defined( NO_SR_OPTIMIZE )
  #pragma optimize( "", on )            // restore previous settings
#endif
