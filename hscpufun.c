/* HSCPUFUN.C   (c) Copyright Roger Bowler, 1999-2012                */
/*              (c) Copyright Jan Jaeger, 1999-2012                  */
/*              (c) Copyright "Fish" (David B. Trout), 2002-2009     */
/*              (c) Copyright TurboHercules, SAS 2010-2011           */
/*              CPU functions                                        */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#include "hstdinc.h"

#define _HSCPUFUN_C_
#define _HENGINE_DLL_

#include "hercules.h"

/* Issue generic Missing device number message */
static inline void missing_devnum()
{
    WRMSG(HHC02201,"E");
}

/*-------------------------------------------------------------------*/
/* cpu command - define target cpu for panel display and commands    */
/*-------------------------------------------------------------------*/
int cpu_cmd( int argc, char* argv[], char* cmdline )
{
    BYTE  c;
    int   rc       =  0;
    int   cpu      = -1;
    int   currcpu  = sysblk.pcpu;
    char  cmd[256] = {0};

    if (argc < 2)
    {
        if (!IS_CPU_ONLINE( currcpu ))
        {
            // "Processor %s%02X%s"
            WRMSG( HHC02240, "W", PTYPSTR( currcpu ), currcpu,
                " (currently offline)" );
        }
        else
        {
            // "Processor %s%02X%s"
            WRMSG( HHC02240, "I", PTYPSTR( currcpu ), currcpu, "" );
        }
        return 0;
    }

    if (0
        || sscanf(argv[1], "%x%c", &cpu, &c) != 1
        || cpu < 0
        || cpu >= sysblk.maxcpu
    )
    {
        // "Invalid argument %s%s"
        WRMSG( HHC02205, "E", argv[1], ": target processor is invalid" );
        return -1;
    }

    sysblk.dummyregs.cpuad = cpu;
    sysblk.pcpu = cpu;

    STRLCPY( cmd, cmdline );

    if ( argc > 2 )
    {
         u_int i = 0;
         u_int j = 0;
         u_int n = (u_int)strlen(cmd);

         /* Skip leading blanks, if any */
         for ( ; i < n && cmd[i] == ' '; i++ );

         /* Skip two words */
         for ( ; j < 2; j++ )
         {
           for ( ; i < n && cmd[i] != ' '; i++ );
           for ( ; i < n && cmd[i] == ' '; i++ );
         }

         /* Issue command to temporary target cpu */
         if (i < n)
         {
             rc = HercCmdLine(cmd+i);
             sysblk.pcpu = currcpu;
             sysblk.dummyregs.cpuad = currcpu;
         }
    }

    return rc;
}


/*-------------------------------------------------------------------*/
/* startall command - start all CPU's                                */
/*-------------------------------------------------------------------*/
int startall_cmd(int argc, char *argv[], char *cmdline)
{
    int i;
    int rc = 0;
    CPU_BITMAP mask;

    UNREFERENCED(cmdline);

    if ( argc == 1 )
    {
        OBTAIN_INTLOCK(NULL);
        mask = (~sysblk.started_mask) & sysblk.config_mask;
        for (i = 0; mask; i++)
        {
            if (mask & 1)
            {
                REGS *regs = sysblk.regs[i];
                regs->opinterv = 0;
                regs->cpustate = CPUSTATE_STARTED;
                signal_condition(&regs->intcond);
            }
            mask >>= 1;
        }
        RELEASE_INTLOCK(NULL);
    }
    else
    {
        WRMSG( HHC02299, "E", argv[0] );
        rc = -1;
    }
    return rc;
}


/*-------------------------------------------------------------------*/
/* stopall command - stop all CPU's                                  */
/*-------------------------------------------------------------------*/
DLL_EXPORT int stopall_cmd( int argc, char* argv[], char* cmdline )
{
    int i;
    int rc = 0;
    CPU_BITMAP mask;

    UNREFERENCED(cmdline);

    if ( argc == 1 )
    {
        OBTAIN_INTLOCK(NULL);

        mask = sysblk.started_mask;
        for (i = 0; mask; i++)
        {
            if (mask & 1)
            {
                REGS *regs = sysblk.regs[i];
                regs->opinterv = 1;
                regs->cpustate = CPUSTATE_STOPPING;
                ON_IC_INTERRUPT(regs);
                signal_condition(&regs->intcond);
            }
            mask >>= 1;
        }

        RELEASE_INTLOCK(NULL);
    }
    else
    {
        WRMSG( HHC02299, "E", argv[0] );
        rc = -1;
    }

    return rc;
}

#ifdef _FEATURE_CPU_RECONFIG

/*-------------------------------------------------------------------*/
/* cf command - configure/deconfigure a CPU                          */
/*-------------------------------------------------------------------*/
int cf_cmd(int argc, char *argv[], char *cmdline)
{
    int on = -1;

    UNREFERENCED(cmdline);

    if (argc == 2)
    {
        if ( CMD(argv[1],on,2) )
            on = 1;
        else if ( CMD(argv[1],off,3) )
            on = 0;
    }

    OBTAIN_INTLOCK(NULL);

    if (IS_CPU_ONLINE(sysblk.pcpu))
    {
        if (on < 0)
            WRMSG(HHC00819, "I", PTYPSTR(sysblk.pcpu), sysblk.pcpu );
        else if (on == 0)
            deconfigure_cpu(sysblk.pcpu);
    }
    else
    {
        if (on < 0)
            WRMSG(HHC00820, "I", PTYPSTR(sysblk.pcpu), sysblk.pcpu );
        else if (on > 0)
            configure_cpu(sysblk.pcpu);
    }

    RELEASE_INTLOCK(NULL);

    if (on >= 0)
    {
        cf_cmd (1, argv, argv[0]);
    }

    return 0;
}


/*-------------------------------------------------------------------*/
/* cfall command - configure/deconfigure all CPU's                   */
/*-------------------------------------------------------------------*/
int cfall_cmd(int argc, char *argv[], char *cmdline)
{
static char *qproc[] = { "qproc", NULL };
int rc = 0;
int on = -1;

    UNREFERENCED(cmdline);

    if (argc == 2)
    {
        if ( CMD(argv[1],on,2) )
            on = 1;
        else if ( CMD(argv[1],off,3) )
            on = 0;
        else
        {
            WRMSG( HHC17000, "E" );
            rc = -1;
        }
        if ( rc == 0 )
        {
            rc = configure_numcpu(on ? sysblk.maxcpu : 0);
        }
    }
    else if ( argc == 1 )
    {
        rc = qproc_cmd(1, qproc, *qproc);
    }
    else
    {
        WRMSG( HHC02299, "E", argv[0] );
        rc = -1;
    }
    return rc;
}

#else
int cf_cmd(int argc, char *argv[], char *cmdline)
{
    UNREFERENCED(argc);
    UNREFERENCED(argv);
    UNREFERENCED(cmdline);

    WRMSG( HHC02310, "S", "cf", "FEATURE_CPU_RECONFIG" );
    return -1;
}
int cfall_cmd(int argc, char *argv[], char *cmdline)
{
    UNREFERENCED(argc);
    UNREFERENCED(argv);
    UNREFERENCED(cmdline);

    WRMSG( HHC02310, "S", "cfall", "FEATURE_CPU_RECONFIG" );
    return -1;
}
#endif /*_FEATURE_CPU_RECONFIG*/


/*-------------------------------------------------------------------*/
/* system reset/system reset clear function                          */
/*-------------------------------------------------------------------*/
static int reset_cmd(int ac,char *av[],char *cmdline,int clear)
{
    int rc;

    UNREFERENCED(ac);
    UNREFERENCED(av);
    UNREFERENCED(cmdline);
    OBTAIN_INTLOCK(NULL);

    /* CPU does not have to be stopped to issue reset                */
    /* SA22-7201-05:                                                 */
    /*  p. 12-5, System-Reset-Clear Key                              */
    /*  p. 12-5, System-Reset-Normal Key                             */
    /*  p. 4-36 -- 4-37, CPU Reset                                   */
    rc = system_reset (sysblk.pcpu, clear, sysblk.arch_mode);

    RELEASE_INTLOCK(NULL);

    return rc;
}


/*-------------------------------------------------------------------*/
/* system reset command                                              */
/*-------------------------------------------------------------------*/
int sysreset_cmd(int ac,char *av[],char *cmdline)
{
    int rc;

    if (ac < 2)
        rc = reset_cmd( ac, av, cmdline, 0 );
    else if (ac == 2)
    {
        if (!strcasecmp( "clear", av[1] ))
        {
            rc = reset_cmd( ac, av, cmdline, 1 );
        }
        else if (!strcasecmp( "normal", av[1] ))
        {
            rc = reset_cmd( ac, av, cmdline, 0 );
        }
        else
        {
            // "Invalid argument %s%s"
            WRMSG( HHC02205, "E", av[1], "" );
            rc = -1;
        }
    }
    else // (ac > 2)
    {
        // "Invalid argument %s%s"
        WRMSG( HHC02205, "E", av[2], "" );
        rc = -1;
    }

    if (rc >= 0)
        WRMSG( HHC02311, "I", cmdline ); // "%s completed"

    return rc;
}


/*-------------------------------------------------------------------*/
/* system reset clear command                                        */
/*-------------------------------------------------------------------*/
int sysclear_cmd(int ac,char *av[],char *cmdline)
{
    int rc;

    if (ac > 1)
    {
        // "Invalid argument %s%s"
        WRMSG( HHC02205, "E", av[1], "" );
        rc = -1;
    }

    if ((rc = reset_cmd( ac, av, cmdline, 1 )) >= 0)
        WRMSG( HHC02311, "I", av[0] ); // "%s completed"

    return rc;
}


/*-------------------------------------------------------------------*/
/* ipl function                                                      */
/* Format:
 *      ipl xxxx | cccc [loadparm xxxxnnnn | parm xxxxxxx ] [clear]
*/
/*-------------------------------------------------------------------*/
int ipl_cmd2(int argc, char *argv[], char *cmdline, int clr_prm)
{
BYTE c;                                 /* Character work area       */
int  rc;                                /* Return code               */
int  i;
int  clear = clr_prm;                   /* Called with Clear option  */
int j;
size_t  maxb;
U16  lcss;
U16  devnum;
char *cdev, *clcss;
char save_loadparm[16];
int  rest_loadparm = FALSE;

    save_loadparm[0] = '\0';

    UNREFERENCED( cmdline );

    /* Primary CPU must be online */
    if (!IS_CPU_ONLINE(sysblk.pcpu))
    {
        WRMSG(HHC00816, "E", PTYPSTR(sysblk.pcpu), sysblk.pcpu, "online");
        return -1;
    }

    /* Check that target processor type allows IPL */
    if (sysblk.ptyp[sysblk.pcpu] == SCCB_PTYP_ZAAP
     || sysblk.ptyp[sysblk.pcpu] == SCCB_PTYP_ZIIP)
    {
        WRMSG(HHC00818, "E", PTYPSTR(sysblk.pcpu), sysblk.pcpu);
        return -1;
    }

    /* Check the parameters of the IPL command */
    if (argc < 2)
    {
        missing_devnum();
        return -1;
    }
#define MAXPARMSTRING   sizeof(sysblk.iplparmstring)
    sysblk.haveiplparm=0;
    maxb=0;
    if ( argc > 2 )
    {
        if ( CMD( argv[2], clear, 2 ) )
        {
            clear = 1;
        }
        else if ( CMD( argv[2], loadparm, 4 ) )
        {
            STRLCPY( save_loadparm, str_loadparm() );
            rest_loadparm = TRUE;
            if ( argc == 4 )
                set_loadparm(argv[3]);
        }
        else if ( CMD(argv[2], parm, 4) )
        {
            memset(sysblk.iplparmstring,0,MAXPARMSTRING);
            sysblk.haveiplparm=1;
            for ( i = 3; i < argc && maxb < MAXPARMSTRING; i++ )
            {
                if ( i !=3 )
                {
                    sysblk.iplparmstring[maxb++]=0x40;
                }
                for ( j = 0; j < (int)strlen(argv[i]) && maxb < MAXPARMSTRING; j++ )
                {
                    if ( islower(argv[i][j]) )
                    {
                        argv[i][j]=toupper(argv[i][j]);
                    }
                    sysblk.iplparmstring[maxb]=host_to_guest(argv[i][j]);
                    maxb++;
                }
            }
        }
    }

    OBTAIN_INTLOCK(NULL);

    /* The ipl device number might be in format lcss:devnum */
    if ( (cdev = strchr(argv[1],':')) )
    {
        clcss = argv[1];
        cdev++;
    }
    else
    {
        clcss = NULL;
        cdev = argv[1];
    }

    /* If the ipl device is not a valid hex number we assume */
    /* This is a load from the service processor             */
    if (sscanf(cdev, "%hx%c", &devnum, &c) != 1)
        rc = load_hmc(argv[1], sysblk.pcpu, clear);
    else
    {
#if defined(_FEATURE_SCSI_IPL)
        DEVBLK *dev;
#endif /*defined(_FEATURE_SCSI_IPL)*/

        *--cdev = '\0';

        if (clcss)
        {
            if (sscanf(clcss, "%hd%c", &lcss, &c) != 1)
            {
                WRMSG(HHC02205, "E", clcss, ": LCSS id is invalid" );
                if ( rest_loadparm ) set_loadparm( save_loadparm );
                return -1;
            }
        }
        else
            lcss = 0;

#if defined(_FEATURE_SCSI_IPL)
        if((dev = find_device_by_devnum(lcss,devnum))
          && support_boot(dev) >= 0)
            rc = load_boot(dev, sysblk.pcpu, clear, 0);
        else
#endif /*defined(_FEATURE_SCSI_IPL)*/
            rc = load_ipl (lcss, devnum, sysblk.pcpu, clear);
    }

    RELEASE_INTLOCK(NULL);

    return rc;
}


/*-------------------------------------------------------------------*/
/* ipl command                                                       */
/*-------------------------------------------------------------------*/
int ipl_cmd(int argc, char *argv[], char *cmdline)
{
    return ipl_cmd2(argc,argv,cmdline,0);
}


/*-------------------------------------------------------------------*/
/* ipl clear command                                                 */
/*-------------------------------------------------------------------*/
int iplc_cmd(int argc, char *argv[], char *cmdline)
{
    return ipl_cmd2(argc,argv,cmdline,1);
}


/*-------------------------------------------------------------------*/
/* restart command - generate restart interrupt                      */
/*-------------------------------------------------------------------*/
int restart_cmd( int argc, char* argv[], char* cmdline )
{
    UNREFERENCED(argc);
    UNREFERENCED(argv);
    UNREFERENCED(cmdline);

    /* Check that target processor type allows IPL */
    if (sysblk.ptyp[sysblk.pcpu] == SCCB_PTYP_ZAAP
     || sysblk.ptyp[sysblk.pcpu] == SCCB_PTYP_ZIIP)
    {
        // "Processor %s%02X: not eligible for ipl nor restart"
        WRMSG(HHC00818, "E", PTYPSTR(sysblk.pcpu), sysblk.pcpu);
        return -1;
    }

    // "%s key pressed"
    WRMSG(HHC02228, "I", "restart");

    /* Obtain the interrupt lock */
    OBTAIN_INTLOCK(NULL);

    if (!IS_CPU_ONLINE(sysblk.pcpu))
    {
        RELEASE_INTLOCK(NULL);
        // "Processor %s%02X: processor is not %s"
        WRMSG(HHC00816, "W", PTYPSTR(sysblk.pcpu), sysblk.pcpu, "online");
        return +1;
    }

    /* Indicate that a restart interrupt is pending */
    ON_IC_RESTART(sysblk.regs[sysblk.pcpu]);

    /* Ensure that a stopped CPU will recognize the restart */
    if (sysblk.regs[sysblk.pcpu]->cpustate == CPUSTATE_STOPPED)
        sysblk.regs[sysblk.pcpu]->cpustate = CPUSTATE_STOPPING;

    sysblk.regs[sysblk.pcpu]->checkstop = 0;

    /* Signal CPU that an interrupt is pending */
    WAKEUP_CPU (sysblk.regs[sysblk.pcpu]);

    /* Release the interrupt lock */
    RELEASE_INTLOCK(NULL);

    return 0;
}


/*-------------------------------------------------------------------*/
/* ext command - generate external interrupt                         */
/*-------------------------------------------------------------------*/
int ext_cmd(int argc, char *argv[], char *cmdline)
{
    UNREFERENCED(cmdline);
    UNREFERENCED(argc);
    UNREFERENCED(argv);

    OBTAIN_INTLOCK(NULL);

    ON_IC_INTKEY;

    // "%s key pressed"
    WRMSG(HHC02228, "I", "interrupt");

    /* Signal waiting CPUs that an interrupt is pending */
    WAKEUP_CPUS_MASK (sysblk.waiting_mask);

    RELEASE_INTLOCK(NULL);

    return 0;
}

/*-------------------------------------------------------------------*/
/* timerint - display or set the timer interval                      */
/*-------------------------------------------------------------------*/
int timerint_cmd( int argc, char *argv[], char *cmdline )
{
    int rc = 0;
    UNREFERENCED( cmdline );

    strupper( argv[0], argv[0] );

    if (argc == 2)  /* Define a new value? */
    {
        if (CMD( argv[1], default, 7 ) || CMD( argv[1], reset, 5 ))
        {
            sysblk.timerint = DEF_TOD_UPDATE_USECS;
            if (MLVL( VERBOSE ))
            {
                // "%-14s set to %s"
                WRMSG( HHC02204, "I", argv[0], argv[1] );
            }
        }
        else
        {
            int timerint = 0; BYTE c;

            if (1
                && sscanf( argv[1], "%d%c", &timerint, &c ) == 1
                && timerint >= MIN_TOD_UPDATE_USECS
                && timerint <= MAX_TOD_UPDATE_USECS
            )
            {
                sysblk.timerint = timerint;
                if (MLVL( VERBOSE ))
                {
                    char buf[25];
                    MSGBUF( buf, "%d", sysblk.timerint );
                    // "%-14s set to %s"
                    WRMSG( HHC02204, "I", argv[0], buf );
                }
            }
            else
            {
                // "Invalid argument '%s'%s"
                WRMSG( HHC02205, "E", argv[1], ": must be 'default' or n where "
                    QSTR( MIN_TOD_UPDATE_USECS ) " <= n <= "
                    QSTR( MAX_TOD_UPDATE_USECS ) );
                rc = -1;
            }
        }
    }
    else if (argc == 1)
    {
        /* Display the current value */
        char buf[25];
        MSGBUF( buf, "%d", sysblk.timerint );
        // "%-14s: %s"
        WRMSG( HHC02203, "I", argv[0], buf );
    }
    else
    {
        // "Invalid command usage. Type 'help %s' for assistance."
        WRMSG( HHC02299, "E", argv[0] );
        rc = -1;
    }

    return rc;
}


/* format_tod - generate displayable date from TOD value */
/* always uses epoch of 1900 */
char * format_tod(char *buf, U64 tod, int flagdate)
{
    int leapyear, years, days, hours, minutes, seconds, microseconds;

    if ( tod >= ETOD_YEAR )
    {
        tod -= ETOD_YEAR;
        years = (tod / ETOD_4YEARS * 4) + 1;
        tod %= ETOD_4YEARS;
        if ( ( leapyear = tod / ETOD_YEAR ) == 4 )
        {
            tod %= ETOD_YEAR;
            years--;
            tod += ETOD_YEAR;
        }
        else
            tod %= ETOD_YEAR;

        years += leapyear;
    }
    else
        years = 0;

    days = tod / ETOD_DAY;
    tod %= ETOD_DAY;
    hours = tod / ETOD_HOUR;
    tod %= ETOD_HOUR;
    minutes = tod / ETOD_MIN;
    tod %= ETOD_MIN;
    seconds = tod / ETOD_SEC;
    microseconds = (tod % ETOD_SEC) / ETOD_USEC;

    if (flagdate)
    {
        years += 1900;
        days += 1;
    }

    sprintf(buf,"%4d.%03d %02d:%02d:%02d.%06d",
        years,days,hours,minutes,seconds,microseconds);

    return buf;
}


/*-------------------------------------------------------------------*/
/* clocks command - display tod clkc and cpu timer                   */
/*-------------------------------------------------------------------*/
int clocks_cmd(int argc, char *argv[], char *cmdline)
{
REGS *regs;
char clock_buf[30];
ETOD tod_now;
ETOD hw_now;
S64 epoch_now;
U64 epoch_now_abs;
char epoch_sign;
U64 clkc_now;
S64 cpt_now;
#if defined(_FEATURE_SIE)
U64 vtod_now = 0;
S64 vepoch_now = 0;
U64 vepoch_now_abs = 0;
char vepoch_sign = ' ';
U64 vclkc_now = 0;
S64 vcpt_now = 0;
char sie_flag = 0;
#endif
U32 itimer = 0;
char itimer_formatted[32];
char arch370_flag = 0;
char buf[64];
int rc = 0;

    UNREFERENCED(cmdline);

    if ( argc == 1 ) for (;;)
    {
        obtain_lock(&sysblk.cpulock[sysblk.pcpu]);

        if (!IS_CPU_ONLINE(sysblk.pcpu))
        {
            release_lock(&sysblk.cpulock[sysblk.pcpu]);
            WRMSG(HHC00816, "W", PTYPSTR(sysblk.pcpu), sysblk.pcpu, "online");
            break;
        }
        regs = sysblk.regs[sysblk.pcpu];

    /* Get the clock values all at once for consistency and so we can
        release the CPU lock more quickly. */
        etod_clock(regs, &tod_now, ETOD_fast);
        hw_now.high = hw_tod.high;
        hw_now.low  = hw_tod.low;
        epoch_now = regs->tod_epoch;
        clkc_now = regs->clkc;
        cpt_now = cpu_timer(regs);
#if defined(_FEATURE_SIE)
        if ( regs->sie_active )
        {
            vtod_now = TOD_CLOCK(regs->guestregs);
            vepoch_now = regs->guestregs->tod_epoch;
            vclkc_now = regs->guestregs->clkc;
            vcpt_now = cpu_timer_SIE(regs->guestregs);
            sie_flag = 1;
        }
#endif
        if (regs->arch_mode == ARCH_370)
        {
            itimer = INT_TIMER(regs);
        /* The interval timer counts 76800 per second, or one every
           13.0208 microseconds. */
            MSGBUF(itimer_formatted,"%02u:%02u:%02u.%06u",
                (itimer/(76800*60*60)),((itimer%(76800*60*60))/(76800*60)),
                ((itimer%(76800*60))/76800),((itimer%76800)*13));
            arch370_flag = 1;
        }

        release_lock(&sysblk.cpulock[sysblk.pcpu]);

        MSGBUF( buf, "tod = %16.16"PRIX64"    %s",
                ETOD2TOD(tod_now), format_tod(clock_buf,tod_now.high,TRUE) );
        WRMSG(HHC02274, "I", buf);

        MSGBUF( buf, "h/w = %16.16"PRIX64"    %s",
                ETOD2TOD(hw_now), format_tod(clock_buf,hw_now.high,TRUE) );
        WRMSG(HHC02274, "I", buf);

        if (epoch_now < 0)
        {
            epoch_now_abs = -(epoch_now);
            epoch_sign = '-';
        }
        else
        {
            epoch_now_abs = epoch_now;
            epoch_sign = ' ';
        }
        MSGBUF( buf, "off = %16.16"PRIX64"   %c%s",
                etod2tod(epoch_now), epoch_sign,
                format_tod(clock_buf,epoch_now_abs,FALSE) );
        WRMSG(HHC02274, "I", buf);

        MSGBUF( buf, "ckc = %16.16"PRIX64"    %s",
                etod2tod(clkc_now), format_tod(clock_buf,clkc_now,TRUE) );
        WRMSG(HHC02274, "I", buf);

        if (regs->cpustate != CPUSTATE_STOPPED)
            MSGBUF( buf, "cpt = %16.16"PRIX64, cpt_now );
        else
            MSGBUF( buf, "cpt = %16.16"PRIX64"         not decrementing", cpt_now );
        WRMSG(HHC02274, "I", buf);

#if defined(_FEATURE_SIE)
        if (sie_flag)
        {

            MSGBUF( buf, "vtod = %16.16"PRIX64"    %s",
                    etod2tod(vtod_now), format_tod(clock_buf,vtod_now,TRUE) );
            WRMSG(HHC02274, "I", buf);

            if (vepoch_now < 0)
            {
                vepoch_now_abs = -(vepoch_now);
                vepoch_sign = '-';
            }
            else
            {
                vepoch_now_abs = vepoch_now;
                vepoch_sign = ' ';
            }
            MSGBUF( buf, "voff = %16.16"PRIX64"   %c%s",
                    etod2tod(vepoch_now), vepoch_sign,
                    format_tod(clock_buf,vepoch_now_abs,FALSE) );
            WRMSG(HHC02274, "I", buf);

            MSGBUF( buf, "vckc = %16.16"PRIX64"    %s",
                    etod2tod(vclkc_now), format_tod(clock_buf,vclkc_now,TRUE) );
            WRMSG(HHC02274, "I", buf);

            MSGBUF( buf, "vcpt = %16.16"PRIX64, vcpt_now );
            WRMSG(HHC02274, "I", buf);
        }
#endif

        if (arch370_flag)
        {
            MSGBUF( buf, "itm = %8.8"PRIX32"                     %s",
                   itimer, itimer_formatted );
            WRMSG(HHC02274, "I", buf);
        }
        break;
    }
    else
    {
        WRMSG( HHC02299, "E", argv[0] );
        rc = -1;
    }

    return rc;
}


/*-------------------------------------------------------------------*/
/* store command - store CPU status at absolute zero                 */
/*-------------------------------------------------------------------*/
int store_cmd(int argc, char *argv[], char *cmdline)
{
REGS *regs;

    UNREFERENCED(cmdline);
    UNREFERENCED(argc);
    UNREFERENCED(argv);

    obtain_lock(&sysblk.cpulock[sysblk.pcpu]);

    if (!IS_CPU_ONLINE(sysblk.pcpu))
    {
        release_lock(&sysblk.cpulock[sysblk.pcpu]);
        WRMSG(HHC00816, "W", PTYPSTR(sysblk.pcpu), sysblk.pcpu, "online");
        return 0;
    }
    regs = sysblk.regs[sysblk.pcpu];

    /* Command is valid only when CPU is stopped */
    if (regs->cpustate != CPUSTATE_STOPPED)
    {
        WRMSG(HHC02224, "E");
        return -1;
    }

    /* Store status in 512 byte block at absolute location 0 */
    store_status (regs, 0);

#if defined(_FEATURE_HARDWARE_LOADER)
    ARCH_DEP(sdias_store_status)(regs);
#endif /*defined(_FEATURE_HARDWARE_LOADER)*/

    release_lock(&sysblk.cpulock[sysblk.pcpu]);

    WRMSG(HHC00817, "I", PTYPSTR(regs->cpuad), regs->cpuad);

    return 0;
}

/*-------------------------------------------------------------------*/
/* start command - start current CPU                                 */
/*-------------------------------------------------------------------*/
int start_cmd_cpu( int argc, char* argv[], char* cmdline )
{
    int rc = 0;

    UNREFERENCED(argc);
    UNREFERENCED(argv);
    UNREFERENCED(cmdline);

    OBTAIN_INTLOCK(NULL);

    if (IS_CPU_ONLINE(sysblk.pcpu))
    {
        REGS *regs = sysblk.regs[sysblk.pcpu];
        if ( regs->cpustate == CPUSTATE_STARTED )
        {
            WRMSG(HHC00816, "W", PTYPSTR(sysblk.pcpu), sysblk.pcpu, "stopped");
            rc = 1;
        }
        else
        {
            regs->opinterv = 0;
            regs->cpustate = CPUSTATE_STARTED;
            regs->checkstop = 0;
            WAKEUP_CPU(regs);
            WRMSG( HHC00834, "I", PTYPSTR(sysblk.pcpu), sysblk.pcpu, "running state selected" );
        }
    }
    else
    {
        WRMSG(HHC00816, "W", PTYPSTR(sysblk.pcpu), sysblk.pcpu, "online");
        rc = 1;
    }

    RELEASE_INTLOCK(NULL);

    return rc;
}

/*-------------------------------------------------------------------*/
/* stop command - stop current CPU                                   */
/*-------------------------------------------------------------------*/
int stop_cmd_cpu( int argc, char* argv[], char* cmdline )
{
    int rc = 0;

    UNREFERENCED(argc);
    UNREFERENCED(argv);
    UNREFERENCED(cmdline);

    OBTAIN_INTLOCK(NULL);

    if (IS_CPU_ONLINE(sysblk.pcpu))
    {
        REGS *regs = sysblk.regs[sysblk.pcpu];
        if ( regs->cpustate != CPUSTATE_STARTED )
        {
            WRMSG(HHC00816, "W", PTYPSTR(sysblk.pcpu), sysblk.pcpu, "started");
            rc = 1;
        }
        else
        {
            regs->opinterv = 1;
            regs->cpustate = CPUSTATE_STOPPING;
            ON_IC_INTERRUPT(regs);
            WAKEUP_CPU (regs);
            WRMSG( HHC00834, "I", PTYPSTR(sysblk.pcpu), sysblk.pcpu, "manual state selected" );
        }
    }
    else
    {
        WRMSG(HHC00816, "W", PTYPSTR(sysblk.pcpu), sysblk.pcpu, "online");
        rc = 1;
    }

    RELEASE_INTLOCK(NULL);

    return rc;
}

#if defined(_FEATURE_ASN_AND_LX_REUSE)
/*-------------------------------------------------------------------*/
/* alrf command - display or set asn_and_lx_reuse                    */
/*-------------------------------------------------------------------*/
int alrf_cmd(int argc, char *argv[], char *cmdline)
{
char    buffer[128] = {0};
char   *archlvl_func;

    UNREFERENCED(cmdline);

    if (argc < 1 || argc > 2)
    {
        WRMSG( HHC02299, "E", argv[0] );
        return -1;
    }

    // HHC02256 "Command %s is deprecated, use %s instead"

    if (argc == 1)
    {
        archlvl_func = "query";
    }
    else /* (argc == 2) */
    {
        archlvl_func = argv[1];
    }

    MSGBUF( buffer, "archlvl %s asn_lx_reuse", archlvl_func );
    WRMSG( HHC02256, "W", "ALRF", buffer );
    return InternalHercCmd( buffer );
}
#endif /*defined(_FEATURE_ASN_AND_LX_REUSE)*/

#if defined(_FEATURE_CMPSC_ENHANCEMENT_FACILITY)
/*-------------------------------------------------------------------*/
/* cmpscpad command - set CMPSC instruction padding alignment        */
/*-------------------------------------------------------------------*/
int cmpscpad_cmd( int argc, char* argv[], char* cmdline )
{
    int bits;
    const char* ptr;
    char* nxt;
    char buf[8];

    strupper( argv[0], argv[0] );

    UNREFERENCED( cmdline );

    if ( argc > 2 )
    {
        // "Invalid number of arguments for %s"
        WRMSG( HHC01455, "E", argv[0] );
        return HERROR;
    }

    /* Ensure all CPUs have been stopped */

    OBTAIN_INTLOCK( NULL );

    if (are_any_cpus_started_intlock_held())
    {
        RELEASE_INTLOCK( NULL );
        // "CPUs must be offline or stopped"
        WRMSG( HHC02389, "E" );
        return HERRCPUONL;
    }

    if ( argc == 2 )
    {
        ptr   = argv[1];
        errno = 0;
        bits  = (int) strtoul( ptr, &nxt, 10 );

        if (0
            || errno != 0
            || nxt == ptr
            || *nxt != 0
            || bits < MIN_CMPSC_ZP_BITS
            || bits > MAX_CMPSC_ZP_BITS
        )
        {
            RELEASE_INTLOCK( NULL );
            // "Specified value is invalid or outside of range %d to %d"
            WRMSG( HHC17014 , "E", MIN_CMPSC_ZP_BITS,
                                   MAX_CMPSC_ZP_BITS );
            return HERROR;
        }

        /* Update SYSBLK with new value */
        sysblk.zpbits = (BYTE) bits;
        RELEASE_INTLOCK( NULL );

        MSGBUF( buf, "%d", bits );
        // "%-14s set to %s"
        WRMSG( HHC02204, "I", argv[0], buf );
    }
    else
    {
        /* Display current SYSBLK value */
        bits = sysblk.zpbits;
        RELEASE_INTLOCK( NULL );

        MSGBUF( buf, "%d", bits );
        // "%-14s: %s"
        WRMSG( HHC02203, "I", argv[0], buf );
    }

    return HNOERROR;
}
#endif /* defined(_FEATURE_CMPSC_ENHANCEMENT_FACILITY) */
