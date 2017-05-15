/* HSCEMODE.C   (c) Copyright Roger Bowler, 1999-2012                */
/*              (c) Copyright Jan Jaeger, 1999-2012                  */
/*              (c) Copyright "Fish" (David B. Trout), 2002-2009     */
/*              (c) Copyright TurboHercules, SAS 2010-2011           */
/*              CE mode functions                                    */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#include "hstdinc.h"

DISABLE_GCC_WARNING( "-Wunused-function" )

#define _HSCEMODE_C_
#define _HENGINE_DLL_

#include "hercules.h"

/*-------------------------------------------------------------------*/
/*    architecture dependent 'pr_cmd' prefix command handler         */
/*-------------------------------------------------------------------*/
int ARCH_DEP( archdep_pr_cmd )( REGS *regs, int argc, char *argv[] )
{
    U64   px;
    char  buf[64];

    if (argc > 1)
    {
        /* Parse requested new prefix register value */
        if (!sscanf( argv[1], "%"SCNx64, &px ))
        {
            // "Invalid argument %s%s"
            WRMSG( HHC02205, "E", argv[1], "" );
            return -1;
        }

        /* Set ignored bits to zero and zero those bits that must be */
        px &= PX_MASK;

        if (px > regs->mainlim)
        {
            // PROGRAMMING NOTE: 'F_RADR' and 'RADR' are very likely
            // 64-bit due to FEATURE_INTERPRETIVE_EXECUTION normally
            // being #defined, causing _FEATURE_ZSIE to be #defined.

            MSGBUF( buf, "A:"F_RADR"  Addressing exception", (RADR) px );
            WRMSG( HHC02290, "E", buf );
            return -1;
        }

        regs->PX = px;              /* set NEW prefix register value */
    }
    else
        px = regs->PX;              /* retrieve CURRENT prefix value */

    // "Prefix register: %s"
    MSGBUF( buf, F_RADR, (RADR) px );   /* Format the prefix address */
    WRMSG( HHC02277, "I", buf );        /* and then show it to them  */
    return 0;
}


/*-------------------------------------------------------------------*/
/* Compile ARCH_DEP() functions for other build architectures...     */
/*-------------------------------------------------------------------*/

#if !defined(_GEN_ARCH)             // (first time here?)

#if defined(_ARCHMODE2)
 #define  _GEN_ARCH _ARCHMODE2      // (set next build architecture)
 #include "hscemode.c"              // (compile ourselves again)
#endif

#if defined(_ARCHMODE3)
 #undef   _GEN_ARCH
 #define  _GEN_ARCH _ARCHMODE3      // (set next build architecture)
 #include "hscemode.c"              // (compile ourselves again)
#endif


/*-------------------------------------------------------------------*/
/*       NON-architecure-dependent code from here onward             */
/*-------------------------------------------------------------------*/

/* Issue generic Device not found error message */
static inline int devnotfound_msg(U16 lcss,U16 devnum)
{
    WRMSG(HHC02200,"E",lcss,devnum);
    return -1;
}

/* Issue generic Missing device number message */
static inline void missing_devnum()
{
    WRMSG(HHC02201,"E");
}


static inline char *aea_mode_str(BYTE mode)
{
static char *name[] = { "DAT-Off", "Primary", "AR", "Secondary", "Home",
                        0, 0, 0, "PER/DAT-Off", "PER/Primary", "PER/AR",
                        "PER/Secondary", "PER/Home" };

    return name[(mode & 0x0f) | ((mode & 0xf0) ? 8 : 0)];
}


/*-------------------------------------------------------------------*/
/* aea - display aea values                                          */
/*-------------------------------------------------------------------*/
int aea_cmd(int argc, char *argv[], char *cmdline)
{
    int     i;                          /* Index                     */
    REGS   *regs;
    char   buf[128];
    int    len;

    UNREFERENCED(argc);
    UNREFERENCED(argv);
    UNREFERENCED(cmdline);

    obtain_lock(&sysblk.cpulock[sysblk.pcpu]);

    if (!IS_CPU_ONLINE(sysblk.pcpu))
    {
        release_lock(&sysblk.cpulock[sysblk.pcpu]);
        WRMSG(HHC00816, "W", PTYPSTR(sysblk.pcpu), sysblk.pcpu, "online");
        return 0;
    }
    regs = sysblk.regs[sysblk.pcpu];

    MSGBUF( buf, "aea mode   %s", aea_mode_str(regs->aea_mode));
    WRMSG(HHC02282, "I", buf);

    len = sprintf(buf, "aea ar    ");
    for (i = USE_HOME_SPACE; i < 0; i++)
         if (regs->AEA_AR(i) > 0)
            len += sprintf(buf + len, " %2.2X",regs->AEA_AR(i));
        else
            len += sprintf(buf + len, " %2d",regs->AEA_AR(i));
    for (i = 0; i < 16; i++)
         if (regs->AEA_AR(i) > 0)
            len += sprintf(buf + len, " %2.2X",regs->AEA_AR(i));
        else
            len += sprintf(buf + len, " %2d",regs->AEA_AR(i));
    WRMSG(HHC02282, "I", buf);

    len = sprintf(buf, "aea common            ");
    if (regs->AEA_COMMON(CR_ASD_REAL) > 0)
        len += sprintf(buf + len, " %2.2X",regs->AEA_COMMON(CR_ASD_REAL));
    else
        len += sprintf(buf + len, " %2d",regs->AEA_COMMON(CR_ASD_REAL));
    for (i = 0; i < 16; i++)
        if (regs->AEA_COMMON(i) > 0)
            len += sprintf(buf + len, " %2.2X",regs->AEA_COMMON(i));
        else
            len += sprintf(buf + len, " %2d",regs->AEA_COMMON(i));
    WRMSG(HHC02282, "I", buf);

    MSGBUF( buf, "aea cr[1]  %16.16"PRIX64, regs->CR_G(1));
    WRMSG(HHC02282, "I", buf);
    MSGBUF( buf, "    cr[7]  %16.16"PRIX64, regs->CR_G(7));
    WRMSG(HHC02282, "I", buf);
    MSGBUF( buf, "    cr[13] %16.16"PRIX64, regs->CR_G(13));
    WRMSG(HHC02282, "I", buf);
    MSGBUF( buf, "    cr[r]  %16.16"PRIX64, regs->CR_G(CR_ASD_REAL));
    WRMSG(HHC02282, "I", buf);

    for (i = 0; i < 16; i++)
        if (regs->AEA_AR(i) > 15)
        {
            MSGBUF( buf, "    alb[%d] %16.16"PRIX64, i,
                    regs->CR_G(CR_ALB_OFFSET + i));
            WRMSG(HHC02282, "I", buf);
        }

    if (regs->sie_active)
    {
        regs = regs->guestregs;

        WRMSG(HHC02282, "I", "aea SIE");
        MSGBUF( buf, "aea mode   %s",aea_mode_str(regs->aea_mode));
        WRMSG(HHC02282, "I", buf);

        len = sprintf(buf, "aea ar    ");
        for (i = USE_HOME_SPACE; i < 0; i++)
            if (regs->AEA_AR(i) > 0)
                len += sprintf(buf + len, " %2.2X",regs->AEA_AR(i));
            else
                len += sprintf(buf + len, " %2d",regs->AEA_AR(i));
        for (i = 0; i < 16; i++)
            if (regs->AEA_AR(i) > 0)
                len += sprintf(buf + len, " %2.2X",regs->AEA_AR(i));
            else
                len += sprintf(buf + len, " %2d",regs->AEA_AR(i));
        WRMSG(HHC02282, "I", buf);

        len = sprintf(buf, "aea common            ");
        if (regs->AEA_COMMON(CR_ASD_REAL) > 0)
            len += sprintf(buf + len, " %2.2X",regs->AEA_COMMON(CR_ASD_REAL));
        else
            len += sprintf(buf + len, " %2d",regs->AEA_COMMON(CR_ASD_REAL));
        for (i = 0; i < 16; i++)
        if (regs->AEA_COMMON(i) > 0)
            len += sprintf(buf + len, " %2.2X",regs->AEA_COMMON(i));
        else
            len += sprintf(buf + len, " %2d",regs->AEA_COMMON(i));
        WRMSG(HHC02282, "I", buf);

        MSGBUF( buf, "aea cr[1]  %16.16"PRIX64, regs->CR_G(1));
        WRMSG(HHC02282, "I", buf);
        MSGBUF( buf, "    cr[7]  %16.16"PRIX64, regs->CR_G(7));
        WRMSG(HHC02282, "I", buf);
        MSGBUF( buf, "    cr[13] %16.16"PRIX64, regs->CR_G(13));
        WRMSG(HHC02282, "I", buf);
        MSGBUF( buf, "    cr[r]  %16.16"PRIX64, regs->CR_G(CR_ASD_REAL));
        WRMSG(HHC02282, "I", buf);

        for (i = 0; i < 16; i++)
            if (regs->AEA_AR(i) > 15)
            {
                MSGBUF( buf, "    alb[%d] %16.16"PRIX64, i,
                        regs->CR_G(CR_ALB_OFFSET + i));
                WRMSG(HHC02282, "I", buf);
            }
    }

    release_lock (&sysblk.cpulock[sysblk.pcpu]);

    return 0;
}



/*-------------------------------------------------------------------*/
/* traceopt - perform display_inst traditionally or new              */
/*-------------------------------------------------------------------*/
int traceopt_cmd(int argc, char *argv[], char *cmdline)
{
    UNREFERENCED(cmdline);
    if ( argc > 2 )
    {
        WRMSG( HHC02299, "E", argv[0] );
        return -1;
    }

    if (argc == 2)
    {
        if ( CMD(argv[1],traditional,4) )
        {
            sysblk.showregsfirst = 0;
            sysblk.showregsnone = 0;
        }
        else if ( CMD(argv[1],regsfirst,4) )
        {
            sysblk.showregsfirst = 1;
            sysblk.showregsnone = 0;
        }
        else if ( CMD(argv[1],noregs,4) )
        {
            sysblk.showregsfirst = 0;
            sysblk.showregsnone = 1;
        }
        else
        {
            WRMSG( HHC01451, "E", argv[1], argv[0] );
            return -1;
        }
        if ( MLVL(VERBOSE) )
        {
            WRMSG(HHC02203, "I", "Hercules inst trace display",
                sysblk.showregsnone ? "noregs mode" :
                sysblk.showregsfirst ? "regsfirst mode" : "traditional mode");
        }
    }
    else
    {
        WRMSG(HHC02203, "I", "Hercules inst trace display",
            sysblk.showregsnone ? "noregs mode" :
            sysblk.showregsfirst ? "regsfirst mode" : "traditional mode");
    }
    return 0;
}


/*-------------------------------------------------------------------*/
/* aia - display aia values                                          */
/*-------------------------------------------------------------------*/
DLL_EXPORT int aia_cmd(int argc, char *argv[], char *cmdline)
{
    REGS   *regs;
    char buf[128];

    UNREFERENCED(argc);
    UNREFERENCED(argv);
    UNREFERENCED(cmdline);

    obtain_lock(&sysblk.cpulock[sysblk.pcpu]);

    if (!IS_CPU_ONLINE(sysblk.pcpu))
    {
        release_lock(&sysblk.cpulock[sysblk.pcpu]);
        WRMSG(HHC00816, "W", PTYPSTR(sysblk.pcpu), sysblk.pcpu, "online");
        return 0;
    }
    regs = sysblk.regs[sysblk.pcpu];

    MSGBUF( buf, "AIV %16.16"PRIx64" aip %p ip %p aie %p aim %p",
            regs->AIV_G,regs->aip,regs->ip,regs->aie,(BYTE *)regs->aim);
    WRMSG(HHC02283, "I", buf);

    if (regs->sie_active)
    {
        regs = regs->guestregs;
        sprintf(buf + sprintf(buf, "SIE: "), "AIV %16.16"PRIx64" aip %p ip %p aie %p",
            regs->AIV_G,regs->aip,regs->ip,regs->aie);
        WRMSG(HHC02283, "I", buf);
    }

    release_lock (&sysblk.cpulock[sysblk.pcpu]);

    return 0;
}


/*-------------------------------------------------------------------*/
/* tlb - display tlb table                                           */
/*-------------------------------------------------------------------*/
/*                                                                   */
/* NOTES:                                                            */
/*   The "tlbid" field is part of TLB_VADDR so it must be extracted  */
/*   whenever it's used or displayed. The TLB_VADDR does not contain */
/*   all of the effective address bits so they are created on-the-fly*/
/*   with (i << shift) The "main" field of the tlb contains an XOR   */
/*   hash of effective address. So MAINADDR() macro is used to remove*/
/*   the hash before it's displayed.                                 */
/*                                                                   */
int tlb_cmd(int argc, char *argv[], char *cmdline)
{
    int     i;                          /* Index                     */
    int     shift;                      /* Number of bits to shift   */
    int     bytemask;                   /* Byte mask                 */
    U64     pagemask;                   /* Page mask                 */
    int     matches = 0;                /* Number aeID matches       */
    REGS   *regs;
    char    buf[128];


    UNREFERENCED(argc);
    UNREFERENCED(argv);
    UNREFERENCED(cmdline);

    obtain_lock(&sysblk.cpulock[sysblk.pcpu]);

    if (!IS_CPU_ONLINE(sysblk.pcpu))
    {
        release_lock(&sysblk.cpulock[sysblk.pcpu]);
        WRMSG(HHC00816, "W", PTYPSTR(sysblk.pcpu), sysblk.pcpu, "online");
        return 0;
    }
    regs = sysblk.regs[sysblk.pcpu];
    shift = regs->arch_mode == ARCH_370 ? 11 : 12;
    bytemask = regs->arch_mode == ARCH_370 ? 0x1FFFFF : 0x3FFFFF;
    pagemask = regs->arch_mode == ARCH_370 ? 0x00E00000 :
               regs->arch_mode == ARCH_390 ? 0x7FC00000 :
                                     0xFFFFFFFFFFC00000ULL;

    MSGBUF( buf, "tlbID 0x%6.6X mainstor %p",regs->tlbID,regs->mainstor);
    WRMSG(HHC02284, "I", buf);
    WRMSG(HHC02284, "I", "  ix              asd            vaddr              pte   id c p r w ky     main");
    for (i = 0; i < TLBN; i++)
    {
        MSGBUF( buf, "%s%3.3X %16.16"PRIX64" %16.16"PRIX64" %16.16"PRIX64" %4.4X %1d %1d %1d %1d %2.2X %8.8X",
         ((regs->tlb.TLB_VADDR_G(i) & bytemask) == regs->tlbID ? "*" : " "),
         i,regs->tlb.TLB_ASD_G(i),
         ((regs->tlb.TLB_VADDR_G(i) & pagemask) | (i << shift)),
         regs->tlb.TLB_PTE_G(i),(int)(regs->tlb.TLB_VADDR_G(i) & bytemask),
         regs->tlb.common[i],regs->tlb.protect[i],
         (regs->tlb.acc[i] & ACC_READ) != 0,(regs->tlb.acc[i] & ACC_WRITE) != 0,
         regs->tlb.skey[i],
         (unsigned int)(MAINADDR(regs->tlb.main[i],
                  ((regs->tlb.TLB_VADDR_G(i) & pagemask) | (unsigned int)(i << shift)))
                  - regs->mainstor));
        matches += ((regs->tlb.TLB_VADDR(i) & bytemask) == regs->tlbID);
       WRMSG(HHC02284, "I", buf);
    }
    MSGBUF( buf, "%d tlbID matches", matches);
    WRMSG(HHC02284, "I", buf);

    if (regs->sie_active)
    {
        regs = regs->guestregs;
        shift = regs->guestregs->arch_mode == ARCH_370 ? 11 : 12;
        bytemask = regs->arch_mode == ARCH_370 ? 0x1FFFFF : 0x3FFFFF;
        pagemask = regs->arch_mode == ARCH_370 ? 0x00E00000 :
                   regs->arch_mode == ARCH_390 ? 0x7FC00000 :
                                         0xFFFFFFFFFFC00000ULL;

        MSGBUF( buf, "SIE: tlbID 0x%4.4x mainstor %p",regs->tlbID,regs->mainstor);
        WRMSG(HHC02284, "I", buf);
        WRMSG(HHC02284, "I", "  ix              asd            vaddr              pte   id c p r w ky       main");
        for (i = matches = 0; i < TLBN; i++)
        {
            MSGBUF( buf, "%s%3.3X %16.16"PRIX64" %16.16"PRIX64" %16.16"PRIX64" %4.4X %1d %1d %1d %1d %2.2X %8.8X",
             ((regs->tlb.TLB_VADDR_G(i) & bytemask) == regs->tlbID ? "*" : " "),
             i,regs->tlb.TLB_ASD_G(i),
             ((regs->tlb.TLB_VADDR_G(i) & pagemask) | (i << shift)),
             regs->tlb.TLB_PTE_G(i),(int)(regs->tlb.TLB_VADDR_G(i) & bytemask),
             regs->tlb.common[i],regs->tlb.protect[i],
             (regs->tlb.acc[i] & ACC_READ) != 0,(regs->tlb.acc[i] & ACC_WRITE) != 0,
             regs->tlb.skey[i],
             (unsigned int) (MAINADDR(regs->tlb.main[i],
                     ((regs->tlb.TLB_VADDR_G(i) & pagemask) | (unsigned int)(i << shift)))
                    - regs->mainstor));
            matches += ((regs->tlb.TLB_VADDR(i) & bytemask) == regs->tlbID);
           WRMSG(HHC02284, "I", buf);
        }
        MSGBUF( buf, "SIE: %d tlbID matches", matches);
        WRMSG(HHC02284, "I", buf);
    }

    release_lock (&sysblk.cpulock[sysblk.pcpu]);

    return 0;
}

#if defined(SIE_DEBUG_PERFMON)
/*-------------------------------------------------------------------*/
/* spm - SIE performance monitor table                               */
/*-------------------------------------------------------------------*/
int spm_cmd(int argc, char *argv[], char *cmdline)
{
    UNREFERENCED(argc);
    UNREFERENCED(argv);
    UNREFERENCED(cmdline);

    sie_perfmon_disp();

    return 0;
}
#endif


/*-------------------------------------------------------------------*/
/* ar command - display access registers                             */
/*-------------------------------------------------------------------*/
int ar_cmd(int argc, char *argv[], char *cmdline)
{
REGS *regs;
char buf[384];

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

    display_aregs( regs, buf, sizeof(buf), "HHC02272I " );
    WRMSG(   HHC02272, "I", "Access registers" );
    LOGMSG( "%s", buf );

    release_lock(&sysblk.cpulock[sysblk.pcpu]);

    return 0;
}


/*-------------------------------------------------------------------*/
/* pr command - display prefix register                              */
/*-------------------------------------------------------------------*/
int pr_cmd( int argc, char *argv[], char *cmdline )
{
    REGS  *regs;
    int    cpu, rc;

    UNREFERENCED( cmdline );

    /* Command affects whatever the panel's current "target" cpu is. */
    cpu = sysblk.pcpu;

    if (!IS_CPU_ONLINE( cpu ))
    {
        // "Processor %s%02X: processor is not %s"
        WRMSG( HHC00816, "W", PTYPSTR( cpu ), cpu, "online" );
        return -1;
    }

    regs = sysblk.regs[ cpu ];
    obtain_lock( &sysblk.cpulock[ cpu ]);

    switch( regs->arch_mode )
    {
#if defined( _370 )
    case ARCH_370:
        rc = s370_archdep_pr_cmd( regs, argc, argv ); break;
#endif
#if defined( _390 )
    case ARCH_390:
        rc = s390_archdep_pr_cmd( regs, argc, argv ); break;
#endif
#if defined( _900 )
    case ARCH_900:
        rc = z900_archdep_pr_cmd( regs, argc, argv ); break;
#endif
    default:
        rc = -1; break;
    }

    release_lock( &sysblk.cpulock[ cpu ]);
    return rc;
}


/*-------------------------------------------------------------------*/
/* psw command - display or alter program status word                */
/*
 * Return Codes:
 * -1 invalid command operands
 * 0  running normal cpu
 * 1  enabled wait
 * 2  disabled wait
 * 3  Instruction Step
 * 4  manual (STOPPED)
 * 5  offline cpu
---------------------------------------------------------------------*/
int psw_cmd(int argc, char *argv[], char *cmdline)
{
REGS *regs;
BYTE  c;
U64   newia=0;
int   newam=0, newas=0, newcc=0, newcmwp=0, newpk=0, newpm=0, newsm=0;
int   updia=0, updas=0, updcc=0, updcmwp=0, updpk=0, updpm=0, updsm=0;
int   n, errflag, modflag=0;
int   rc;
char  buf[512];

    UNREFERENCED(cmdline);

    obtain_lock(&sysblk.cpulock[sysblk.pcpu]);

    if (!IS_CPU_ONLINE(sysblk.pcpu))
    {
        release_lock(&sysblk.cpulock[sysblk.pcpu]);
        WRMSG(HHC00816, "W", PTYPSTR(sysblk.pcpu), sysblk.pcpu, "online");
        rc = 5;
        return rc;
    }

    regs = sysblk.regs[sysblk.pcpu];

    /* Process optional operands */
    for (n = 1; n < argc; n++)
    {
        modflag = 1;
        errflag = 0;
        if (strncasecmp(argv[n],"sm=",3) == 0)
        {
            /* PSW system mask operand */
            if (sscanf(argv[n]+3, "%x%c", &newsm, &c) == 1
                && newsm >= 0 && newsm <= 255)
                updsm = 1;
            else
                errflag = 1;
        }
        else if (strncasecmp(argv[n],"pk=",3) == 0)
        {
            /* PSW protection key operand */
            if (sscanf(argv[n]+3, "%d%c", &newpk, &c) == 1
                && newpk >= 0 && newpk <= 15)
                updpk = 1;
            else
                errflag = 1;
        }
        else if (strncasecmp(argv[n],"cmwp=",5) == 0)
        {
            /* PSW CMWP bits operand */
            if (sscanf(argv[n]+5, "%x%c", &newcmwp, &c) == 1
                && newcmwp >= 0 && newcmwp <= 15)
                updcmwp = 1;
            else
                errflag = 1;
        }
        else if (strncasecmp(argv[n],"as=",3) == 0)
        {
            /* PSW address-space control operand */
            if (strcasecmp(argv[n]+3,"pri") == 0)
                newas = PSW_PRIMARY_SPACE_MODE;
            else if (strcmp(argv[n]+3,"ar") == 0)
                newas = PSW_ACCESS_REGISTER_MODE;
            else if (strcmp(argv[n]+3,"sec") == 0)
                newas = PSW_SECONDARY_SPACE_MODE;
            else if (strcmp(argv[n]+3,"home") == 0)
                newas = PSW_HOME_SPACE_MODE;
            else
                errflag = 1;
            if (errflag == 0) updas = 1;
        }
        else if (strncasecmp(argv[n],"cc=",3) == 0)
        {
            /* PSW condition code operand */
            if (sscanf(argv[n]+3, "%d%c", &newcc, &c) == 1
                && newcc >= 0 && newcc <= 3)
                updcc = 1;
            else
                errflag = 1;
        }
        else if (strncasecmp(argv[n],"pm=",3) == 0)
        {
            /* PSW program mask operand */
            if (sscanf(argv[n]+3, "%x%c", &newpm, &c) == 1
                && newpm >= 0 && newpm <= 15)
                updpm = 1;
            else
                errflag = 1;
        }
        else if (strncasecmp(argv[n],"am=",3) == 0)
        {
            /* PSW addressing mode operand */
            if (strcmp(argv[n]+3,"24") == 0)
                newam = 24;
            else if (strcmp(argv[n]+3,"31") == 0
                    && (sysblk.arch_mode == ARCH_390
                        || sysblk.arch_mode == ARCH_900))
                newam = 31;
            else if (strcmp(argv[n]+3,"64") == 0
                    && sysblk.arch_mode == ARCH_900)
                newam = 64;
            else
                errflag = 1;
        }
        else if (strncasecmp(argv[n],"ia=",3) == 0)
        {
            /* PSW instruction address operand */
            if (sscanf(argv[n]+3, "%"SCNx64"%c", &newia, &c) == 1)
                updia = 1;
            else
                errflag = 1;
        }
        else /* unknown operand keyword */
            errflag = 1;

        /* Error message if this operand was invalid */
        if (errflag)
        {
            WRMSG(HHC02205, "E", argv[n], "");
            release_lock(&sysblk.cpulock[sysblk.pcpu]);
            return -1;
        }
    } /* end for (n) */

    /* Update the PSW system mask, if specified */
    if (updsm)
    {
        regs->psw.sysmask = newsm;
    }

    /* Update the PSW protection key, if specified */
    if (updpk)
    {
        regs->psw.pkey = newpk << 4;
    }

    /* Update the PSW CMWP bits, if specified */
    if (updcmwp)
    {
        regs->psw.states = newcmwp;
    }

    /* Update the PSW address-space control mode, if specified */
    if (updas
        && (ECMODE(&regs->psw)
            || sysblk.arch_mode == ARCH_390
            || sysblk.arch_mode == ARCH_900))
    {
        regs->psw.asc = newas;
    }

    /* Update the PSW condition code, if specified */
    if (updcc)
    {
        regs->psw.cc = newcc;
    }

    /* Update the PSW program mask, if specified */
    if (updpm)
    {
        regs->psw.progmask = newpm;
    }

    /* Update the PSW addressing mode, if specified */
    switch(newam) {
    case 64:
        regs->psw.amode = regs->psw.amode64 = 1;
        regs->psw.AMASK_G = AMASK64;
        break;
    case 31:
        regs->psw.amode = 1;
        regs->psw.amode64 = 0;
        regs->psw.AMASK_G = AMASK31;
        break;
    case 24:
        regs->psw.amode = regs->psw.amode64 = 0;
        regs->psw.AMASK_G = AMASK24;
        break;
    } /* end switch(newam) */

    /* Update the PSW instruction address, if specified */
    if (updia)
    {
        regs->psw.IA_G = newia;
    }

    /* If any modifications were made, reapply the addressing mode mask
       to the instruction address and invalidate the instruction pointer */
    if (modflag)
    {
        regs->psw.IA_G &= regs->psw.AMASK_G;
        regs->aie = NULL;
    }

    /* Display the PSW and PSW field by field */
    display_psw( regs, buf, sizeof(buf) );
    WRMSG( HHC02278, "I", buf );
    WRMSG( HHC02300, "I",
        regs->psw.sysmask,
        regs->psw.pkey >> 4,
        regs->psw.states,
        (regs->psw.asc == PSW_PRIMARY_SPACE_MODE ? "pri" :
            regs->psw.asc == PSW_ACCESS_REGISTER_MODE ? "ar" :
            regs->psw.asc == PSW_SECONDARY_SPACE_MODE ? "sec" :
            regs->psw.asc == PSW_HOME_SPACE_MODE ? "home" : "???"),
        regs->psw.cc,
        regs->psw.progmask,
        (regs->psw.amode == 0 && regs->psw.amode64 == 0 ? "24" :
            regs->psw.amode == 1 && regs->psw.amode64 == 0 ? "31" :
            regs->psw.amode == 1 && regs->psw.amode64 == 1 ? "64" : "???"),
        regs->psw.IA_G);

    if ( WAITSTATE( &regs->psw ) )
    {
        if ( !IS_IC_DISABLED_WAIT_PSW( regs ) )     rc = 1; /* Enabled Wait */
        else                                        rc = 2; /* Disabled Wait */
    }
    else if ( sysblk.inststep )                     rc = 3; /* Instruction Step */
    else if ( regs->cpustate == CPUSTATE_STOPPED )  rc = 4; /* Manual Mode */
    else                                            rc = 0; /* Running Normal */

    release_lock(&sysblk.cpulock[sysblk.pcpu]);

    return rc;
}


/*-------------------------------------------------------------------*/
/* abs or r command - display or alter absolute or real storage      */
/*-------------------------------------------------------------------*/
int abs_or_r_cmd(int argc, char *argv[], char *cmdline)
{
REGS *regs;

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

    alter_display_real_or_abs (regs, argc, argv, cmdline);

    release_lock(&sysblk.cpulock[sysblk.pcpu]);

    return 0;
}


/*-------------------------------------------------------------------*/
/* u command - disassemble                                           */
/*-------------------------------------------------------------------*/
int u_cmd(int argc, char *argv[], char *cmdline)
{
REGS *regs;

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

    disasm_stor (regs, argc-1, argv+1, cmdline);

    release_lock(&sysblk.cpulock[sysblk.pcpu]);

    return 0;
}


/*-------------------------------------------------------------------*/
/* v command - display or alter virtual storage                      */
/*-------------------------------------------------------------------*/
int v_cmd(int argc, char *argv[], char *cmdline)
{
REGS *regs;

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

    alter_display_virt (regs, argc-1, argv+1, cmdline);

    release_lock(&sysblk.cpulock[sysblk.pcpu]);

    return 0;
}



/*-------------------------------------------------------------------*/
/* tracing commands: t, t+, t-, t?, s, s+, s-, s?, b                 */
/*-------------------------------------------------------------------*/
int trace_cmd(int argc, char *argv[], char *cmdline)
{
int  on = 0, off = 0, query = 0;
int  trace = 0;
int  rc;
BYTE c[2];
U64  addr[2];
char range[256];

    trace = cmdline[0] == 't';

    if (strlen(cmdline) > 1)
    {
        on = cmdline[1] == '+'
         || (cmdline[0] == 'b' && cmdline[1] == ' ');
        off = cmdline[1] == '-';
        query = cmdline[1] == '?';
    }

    if (argc > 2 || (off && argc > 1) || (query && argc > 1))
    {
        WRMSG(HHC02205, "E", argv[1], "");
        return -1;
    }

    /* Get address range */
    if (argc == 2)
    {
        rc = sscanf(argv[1], "%"SCNx64"%c%"SCNx64"%c",
                    &addr[0], &c[0], &addr[1], &c[1]);
        if (rc == 1)
        {
            c[0] = '-';
            addr[1] = addr[0];
        }
        else if (rc != 3 || (c[0] != '-' && c[0] != ':' && c[0] != '.'))
        {
            WRMSG(HHC02205, "E", argv[1], "");
            return -1;
        }
        if (c[0] == '.')
            addr[1] += addr[0] - 1;
        if (trace)
        {
            sysblk.traceaddr[0] = addr[0];
            sysblk.traceaddr[1] = addr[1];
        }
        else
        {
            sysblk.stepaddr[0] = addr[0];
            sysblk.stepaddr[1] = addr[1];
        }
    }
    else
        c[0] = '-';

    /* Set tracing/stepping bit on or off */
    if (on || off)
    {
        OBTAIN_INTLOCK(NULL);
        if (trace)
            sysblk.insttrace = on;
        else
            sysblk.inststep = on;
        SET_IC_TRACE;
        RELEASE_INTLOCK(NULL);
    }

    /* Build range for message */
    range[0] = '\0';
    if (trace && (sysblk.traceaddr[0] != 0 || sysblk.traceaddr[1] != 0))
        sprintf(range, "range %"PRIx64"%c%"PRIx64,
                sysblk.traceaddr[0], c[0],
                c[0] != '.' ? sysblk.traceaddr[1] :
                sysblk.traceaddr[1] - sysblk.traceaddr[0] + 1);
    else if (!trace && (sysblk.stepaddr[0] != 0 || sysblk.stepaddr[1] != 0))
        sprintf(range, "range %"PRIx64"%c%"PRIx64,
                sysblk.stepaddr[0], c[0],
                c[0] != '.' ? sysblk.stepaddr[1] :
                sysblk.stepaddr[1] - sysblk.stepaddr[0] + 1);

    /* Determine if this trace is on or off for message */
    on = (trace && sysblk.insttrace) || (!trace && sysblk.inststep);

    /* Display message */

    WRMSG(HHC02229, "I",
           cmdline[0] == 't' ? _("tracing") :
           cmdline[0] == 's' ? _("stepping") : _("break"),
           on ? _("on") : _("off"),
           range);

    return 0;
}



/*-------------------------------------------------------------------*/
/* ipending command - display pending interrupts                     */
/*-------------------------------------------------------------------*/
int ipending_cmd(int argc, char *argv[], char *cmdline)
{
    DEVBLK *dev;                        /* -> Device block           */
    IOINT  *io;                         /* -> I/O interrupt entry    */
    U32    *crwarray;                   /* -> Channel Report queue   */
    unsigned crwcount;
    int     i;
    int first, last;
    char    sysid[12];
    BYTE    curpsw[16];
    char *states[] = { "?(0)", "STARTED", "STOPPING", "STOPPED" };
    char buf[256];

    UNREFERENCED(argc);
    UNREFERENCED(argv);
    UNREFERENCED(cmdline);

    first = last = -1;

    for (i = 0; i < sysblk.maxcpu; i++)
    {
        if (!IS_CPU_ONLINE(i))
        {
            if ( first == -1 )
                first = last = i;
            else
                last++;
            continue;
        }

        /*---------------------*/
        /* CPU state and flags */
        /*---------------------*/

        if ( first > 0 )
        {
            if ( first == last )
                WRMSG( HHC00820, "I", PTYPSTR(first), first );
            else
                WRMSG( HHC00815, "I", PTYPSTR(first), first, PTYPSTR(last), last );
            first = last = -1;
        }

// /*DEBUG*/logmsg( _("hsccmd.c: %s%02X: Any cpu interrupt %spending\n"),
// /*DEBUG*/    PTYPSTR(sysblk.regs[i]->cpuad), sysblk.regs[i]->cpuad,
// /*DEBUG*/    sysblk.regs[i]->cpuint ? "" : _("not ") );
//
        WRMSG( HHC00850, "I", PTYPSTR(sysblk.regs[i]->cpuad), sysblk.regs[i]->cpuad,
                              IC_INTERRUPT_CPU(sysblk.regs[i]),
                              sysblk.regs[i]->ints_state,
                              sysblk.regs[i]->ints_mask);
        WRMSG( HHC00851, "I", PTYPSTR(sysblk.regs[i]->cpuad), sysblk.regs[i]->cpuad,
                              IS_IC_INTERRUPT(sysblk.regs[i]) ? "" : "not ");
        WRMSG( HHC00852, "I", PTYPSTR(sysblk.regs[i]->cpuad), sysblk.regs[i]->cpuad,
                              IS_IC_IOPENDING                 ? "" : "not ");
        WRMSG( HHC00853, "I", PTYPSTR(sysblk.regs[i]->cpuad), sysblk.regs[i]->cpuad,
                              IS_IC_CLKC(sysblk.regs[i])      ? "" : "not ");
        WRMSG( HHC00854, "I", PTYPSTR(sysblk.regs[i]->cpuad), sysblk.regs[i]->cpuad,
                              IS_IC_PTIMER(sysblk.regs[i])    ? "" : "not ");

#if defined(_FEATURE_INTERVAL_TIMER)
        WRMSG( HHC00855, "I", PTYPSTR(sysblk.regs[i]->cpuad), sysblk.regs[i]->cpuad,
                              IS_IC_ITIMER(sysblk.regs[i])    ? "" : "not ");
#if defined(_FEATURE_ECPSVM)
        WRMSG( HHC00856, "I", PTYPSTR(sysblk.regs[i]->cpuad), sysblk.regs[i]->cpuad,
                              IS_IC_ECPSVTIMER(sysblk.regs[i])? "" : "not ");
#endif /*defined(_FEATURE_ECPSVM)*/
#endif /*defined(_FEATURE_INTERVAL_TIMER)*/
        WRMSG( HHC00857, "I", PTYPSTR(sysblk.regs[i]->cpuad), sysblk.regs[i]->cpuad,
                              IS_IC_EXTCALL(sysblk.regs[i]) ? "" : "not ");
        WRMSG( HHC00858, "I", PTYPSTR(sysblk.regs[i]->cpuad), sysblk.regs[i]->cpuad,
                              IS_IC_EMERSIG(sysblk.regs[i]) ? "" : "not ");
        WRMSG( HHC00859, "I", PTYPSTR(sysblk.regs[i]->cpuad), sysblk.regs[i]->cpuad,
                              IS_IC_MCKPENDING(sysblk.regs[i]) ? "" : "not ");
        WRMSG( HHC00860, "I", PTYPSTR(sysblk.regs[i]->cpuad), sysblk.regs[i]->cpuad,
                              IS_IC_SERVSIG ? "" : "not ");
        WRMSG( HHC00861, "I", PTYPSTR(sysblk.regs[i]->cpuad), sysblk.regs[i]->cpuad,
                              sysblk.regs[i]->cpuad == sysblk.mainowner ? "yes" : "no");
        WRMSG( HHC00862, "I", PTYPSTR(sysblk.regs[i]->cpuad), sysblk.regs[i]->cpuad,
                              sysblk.regs[i]->cpuad == sysblk.intowner ? "yes" : "no");
        WRMSG( HHC00863, "I", PTYPSTR(sysblk.regs[i]->cpuad), sysblk.regs[i]->cpuad,
                              sysblk.regs[i]->intwait && !(sysblk.waiting_mask & CPU_BIT(i)) ? "yes" : "no");
        WRMSG( HHC00864, "I", PTYPSTR(sysblk.regs[i]->cpuad), sysblk.regs[i]->cpuad,
                              test_lock(&sysblk.cpulock[i]) ? "" : "not ");
        if (ARCH_370 == sysblk.arch_mode)
        {
            if (0xFFFF == sysblk.regs[i]->chanset)
            {
                MSGBUF( buf, "none");
            }
            else
            {
                MSGBUF( buf, "%4.4X", sysblk.regs[i]->chanset);
            }
            WRMSG( HHC00865, "I", PTYPSTR(sysblk.regs[i]->cpuad), sysblk.regs[i]->cpuad, buf );
        }
        WRMSG( HHC00866, "I", PTYPSTR(sysblk.regs[i]->cpuad), sysblk.regs[i]->cpuad,
                              states[sysblk.regs[i]->cpustate] );
        WRMSG( HHC00867, "I", PTYPSTR(sysblk.regs[i]->cpuad), sysblk.regs[i]->cpuad, INSTCOUNT(sysblk.regs[i]));
        WRMSG( HHC00868, "I", PTYPSTR(sysblk.regs[i]->cpuad), sysblk.regs[i]->cpuad, sysblk.regs[i]->siototal);
        copy_psw(sysblk.regs[i], curpsw);
        if (ARCH_900 == sysblk.arch_mode)
        {
            MSGBUF( buf, "%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X",
                curpsw[0], curpsw[1], curpsw[2], curpsw[3],
                curpsw[4], curpsw[5], curpsw[6], curpsw[7],
                curpsw[8], curpsw[9], curpsw[10], curpsw[11],
                curpsw[12], curpsw[13], curpsw[14], curpsw[15]);
        }
        else
        {
            MSGBUF( buf, "%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X",
                curpsw[0], curpsw[1], curpsw[2], curpsw[3],
                curpsw[4], curpsw[5], curpsw[6], curpsw[7]);
        }
        WRMSG( HHC00869, "I", PTYPSTR(sysblk.regs[i]->cpuad), sysblk.regs[i]->cpuad, buf );

        /*--------------------------*/
        /* (same thing but for SIE) */
        /*--------------------------*/

        if (sysblk.regs[i]->sie_active)
        {
            WRMSG( HHC00850, "I", "IE", sysblk.regs[i]->cpuad,
                            IC_INTERRUPT_CPU(sysblk.regs[i]),
                            sysblk.regs[i]->guestregs->ints_state,
                            sysblk.regs[i]->guestregs->ints_mask);
            WRMSG( HHC00851, "I", "IE", sysblk.regs[i]->cpuad,
                            IS_IC_INTERRUPT(sysblk.regs[i]->guestregs) ? "" : "not ");
            WRMSG( HHC00852, "I", "IE", sysblk.regs[i]->cpuad, IS_IC_IOPENDING ? "" : "not ");
            WRMSG( HHC00853, "I", "IE", sysblk.regs[i]->cpuad, IS_IC_CLKC(sysblk.regs[i]->guestregs) ? "" : "not ");
            WRMSG( HHC00854, "I", "IE", sysblk.regs[i]->cpuad, IS_IC_PTIMER(sysblk.regs[i]->guestregs) ? "" : "not ");
            WRMSG( HHC00855, "I", "IE", sysblk.regs[i]->cpuad, IS_IC_ITIMER(sysblk.regs[i]->guestregs) ? "" : "not ");
            WRMSG( HHC00857, "I", "IE", sysblk.regs[i]->cpuad, IS_IC_EXTCALL(sysblk.regs[i]->guestregs) ? "" : "not ");
            WRMSG( HHC00858, "I", "IE", sysblk.regs[i]->cpuad, IS_IC_EMERSIG(sysblk.regs[i]->guestregs) ? "" : "not ");
            WRMSG( HHC00859, "I", "IE", sysblk.regs[i]->cpuad, IS_IC_MCKPENDING(sysblk.regs[i]->guestregs) ? "" : "not ");
            WRMSG( HHC00860, "I", "IE", sysblk.regs[i]->cpuad, IS_IC_SERVSIG ? "" : "not ");
            WRMSG( HHC00864, "I", "IE", sysblk.regs[i]->cpuad, test_lock(&sysblk.cpulock[i]) ? "" : "not ");

            if (ARCH_370 == sysblk.arch_mode)
            {
                if (0xFFFF == sysblk.regs[i]->guestregs->chanset)
                {
                    MSGBUF( buf, "none");
                }
                else
                {
                    MSGBUF( buf, "%4.4X", sysblk.regs[i]->guestregs->chanset);
                }
                WRMSG( HHC00865, "I", "IE", sysblk.regs[i]->cpuad, buf );
            }
            WRMSG( HHC00866, "I", "IE", sysblk.regs[i]->cpuad, states[sysblk.regs[i]->guestregs->cpustate]);
            WRMSG( HHC00867, "I", "IE", sysblk.regs[i]->cpuad, (S64)sysblk.regs[i]->guestregs->instcount);
            WRMSG( HHC00868, "I", "IE", sysblk.regs[i]->cpuad, sysblk.regs[i]->guestregs->siototal);
            copy_psw(sysblk.regs[i]->guestregs, curpsw);
            if (ARCH_900 == sysblk.arch_mode)
            {
               MSGBUF( buf, "%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X",
                   curpsw[0], curpsw[1], curpsw[2], curpsw[3],
                   curpsw[4], curpsw[5], curpsw[6], curpsw[7],
                   curpsw[8], curpsw[9], curpsw[10], curpsw[11],
                   curpsw[12], curpsw[13], curpsw[14], curpsw[15]);
            }
            else
            {
               MSGBUF( buf, "%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X",
                   curpsw[0], curpsw[1], curpsw[2], curpsw[3],
                   curpsw[4], curpsw[5], curpsw[6], curpsw[7]);
            }
            WRMSG(HHC00869, "I", "IE", sysblk.regs[i]->cpuad, buf);
        }
    }

    /*------------------------*/
    /* System masks and locks */
    /*------------------------*/

    if ( first > 0 )
    {
        if ( first == last )
            WRMSG( HHC00820, "I", PTYPSTR(first), first );
        else
            WRMSG( HHC00815, "I", PTYPSTR(first), first, PTYPSTR(last), last );
    }

    PUSH_GCC_WARNINGS()
    DISABLE_GCC_WARNING( "-Wformat" )

    // ZZ FIXME: No printf format support for __uint128_t yet, so we will incorrectly display...
    WRMSG( HHC00870, "I", sysblk.config_mask, sysblk.started_mask, sysblk.waiting_mask );

    // ZZ FIXME: No printf format support for __uint128_t yet, so we will incorrectly display...
    WRMSG( HHC00871, "I", sysblk.sync_mask, sysblk.syncing ? "sync in progress" : "" );

    POP_GCC_WARNINGS()



    WRMSG( HHC00872, "I", test_lock(&sysblk.sigplock) ? "" : "not ");
    WRMSG( HHC00873, "I", test_lock(&sysblk.todlock) ? "" : "not ");
    WRMSG( HHC00874, "I", test_lock(&sysblk.mainlock) ? "" : "not ", sysblk.mainowner);
    WRMSG( HHC00875, "I", test_lock(&sysblk.intlock) ? "" : "not ", sysblk.intowner);
    WRMSG( HHC00876, "I", test_lock(&sysblk.ioqlock) ? "" : "not ");

    /*----------------------*/
    /* Channel Report queue */
    /*----------------------*/

    OBTAIN_CRWLOCK();

    if ((crwarray = sysblk.crwarray) != NULL)
        if ((crwcount = sysblk.crwcount) != 0)
            if ((crwarray = malloc( crwcount * sizeof(U32) )) != NULL)
                memcpy( crwarray, sysblk.crwarray, crwcount * sizeof(U32));

    RELEASE_CRWLOCK();

    if (!crwarray)
        //     HHC00883 "Channel Report queue: (NULL)"
        WRMSG( HHC00883, "I");
    else if (!crwcount)
        //     HHC00884 "Channel Report queue: (empty)"
        WRMSG( HHC00884, "I");
    else
    {
        U32 crw;
        char buf[256];

        //     HHC00885 "Channel Report queue:"
        WRMSG( HHC00885, "I");

        for (i=0; i < (int) crwcount; i++)
        {
            crw = *(crwarray + i);
            //     HHC00886 "CRW 0x%8.8X: %s"
            WRMSG( HHC00886, "I", crw, FormatCRW( crw, buf, sizeof(buf) ));
        }
        free( crwarray );
    }

    /*-------------------------*/
    /* Device interrupt status */
    /*-------------------------*/

    for (dev = sysblk.firstdev; dev != NULL; dev = dev->nextdev)
    {
#if defined( OPTION_SHARED_DEVICES )
        if (dev->shioactive == DEV_SYS_NONE)
            STRLCPY( sysid, "(none)" );
        else if (dev->shioactive == DEV_SYS_LOCAL)
            STRLCPY( sysid, "local" );
        else
            MSGBUF( sysid, "id=%d", dev->shioactive);
#else // !defined( OPTION_SHARED_DEVICES )
        if (dev->busy && !(dev->suspended))
            STRLCPY( sysid, "local" );
        else
            STRLCPY( sysid, "(none)" );
#endif // defined( OPTION_SHARED_DEVICES )
        if (dev->busy && !(dev->suspended
#if defined( OPTION_SHARED_DEVICES )
            && dev->shioactive == DEV_SYS_NONE
#endif // defined( OPTION_SHARED_DEVICES )
        ))
        {
            // "device %1d:%04X: status %s"
            MSGBUF(buf, "busy %s", sysid);
            WRMSG(HHC00880, "I", SSID_TO_LCSS(dev->ssid), dev->devnum, buf);
        }
        if (dev->reserved)
        {
            MSGBUF(buf, "reserved %s", sysid);
            WRMSG(HHC00880, "I", SSID_TO_LCSS(dev->ssid), dev->devnum, buf);
        }
        if (dev->scsw.flag3 & SCSW3_AC_SUSP)
        {
            WRMSG(HHC00880, "I", SSID_TO_LCSS(dev->ssid), dev->devnum, "suspended" );
        }
        if ((dev->scsw.flag3 & SCSW3_SC_PEND) && (dev->pmcw.flag5 & PMCW5_V))
        {
            WRMSG(HHC00880, "I", SSID_TO_LCSS(dev->ssid), dev->devnum, "I/O pending" );
        }
        if ((dev->pciscsw.flag3 & SCSW3_SC_PEND) && (dev->pmcw.flag5 & PMCW5_V))
        {
            WRMSG(HHC00880, "I", SSID_TO_LCSS(dev->ssid), dev->devnum, "PCI pending" );
        }
        if ((dev->attnscsw.flag3 & SCSW3_SC_PEND) && (dev->pmcw.flag5 & PMCW5_V))
        {
            WRMSG(HHC00880, "I", SSID_TO_LCSS(dev->ssid), dev->devnum, "Attn pending" );
        }
        if (test_lock(&dev->lock) && (dev->pmcw.flag5 & PMCW5_V))
        {
            WRMSG(HHC00880, "I", SSID_TO_LCSS(dev->ssid), dev->devnum, "lock held" );
        }
    }

    /*---------------------*/
    /* I/O Interrupt Queue */
    /*---------------------*/

    if (!sysblk.iointq)
        WRMSG( HHC00881, "I", " (NULL)");
    else
        WRMSG( HHC00881, "I", "");

    for (io = sysblk.iointq; io; io = io->next)
    {
        WRMSG( HHC00882, "I", SSID_TO_LCSS(io->dev->ssid), io->dev->devnum
                ,io->pending      ? " normal, " : ""
                ,io->pcipending   ? " PCI,    " : ""
                ,io->attnpending  ? " ATTN,   " : ""
                ,!(io->pending || io->pcipending || io->attnpending) ?
                                    " unknown," : ""
                ,(io->priority >> 16) & 0xFF
                ,(io->priority >>  8) & 0xFF
                , io->priority        & 0xFF
                 );
    }

    return 0;
}



/*-------------------------------------------------------------------*/
/* gpr command - display or alter general purpose registers          */
/*-------------------------------------------------------------------*/
int gpr_cmd(int argc, char *argv[], char *cmdline)
{
REGS *regs;
char buf[512];

    UNREFERENCED(cmdline);

    obtain_lock(&sysblk.cpulock[sysblk.pcpu]);

    if (!IS_CPU_ONLINE(sysblk.pcpu))
    {
        release_lock(&sysblk.cpulock[sysblk.pcpu]);
        WRMSG(HHC00816, "W", PTYPSTR(sysblk.pcpu), sysblk.pcpu, "online");
        return 0;
    }

    regs = sysblk.regs[sysblk.pcpu];

    if (argc > 1)
    {
        int   reg_num;
        BYTE  equal_sign, c;
        U64   reg_value;

        if (argc > 2)
        {
            release_lock(&sysblk.cpulock[sysblk.pcpu]);
            WRMSG(HHC02205, "E", argv[1], "");
            return 0;
        }

        if (0
            || sscanf( argv[1], "%d%c%"SCNx64"%c", &reg_num, &equal_sign, &reg_value, &c ) != 3
            || 0  > reg_num
            || 15 < reg_num
            || '=' != equal_sign
        )
        {
            release_lock(&sysblk.cpulock[sysblk.pcpu]);
            WRMSG(HHC02205, "E", argv[1], "");
            return 0;
        }

        if ( ARCH_900 == regs->arch_mode )
            regs->GR_G(reg_num) = (U64) reg_value;
        else
            regs->GR_L(reg_num) = (U32) reg_value;
    }

    display_gregs( regs, buf, sizeof(buf), "HHC02269I " );
    WRMSG(   HHC02269, "I", "General purpose registers" );
    LOGMSG( "%s", buf );

    release_lock(&sysblk.cpulock[sysblk.pcpu]);

    return 0;
}


/*-------------------------------------------------------------------*/
/* fpr command - display floating point registers                    */
/*-------------------------------------------------------------------*/
int fpr_cmd(int argc, char *argv[], char *cmdline)
{
REGS *regs;
char buf[512];

    UNREFERENCED(cmdline);

    obtain_lock(&sysblk.cpulock[sysblk.pcpu]);

    if (!IS_CPU_ONLINE(sysblk.pcpu))
    {
        release_lock(&sysblk.cpulock[sysblk.pcpu]);
        // "Processor %s%02X: processor is not %s"
        WRMSG(HHC00816, "W", PTYPSTR(sysblk.pcpu), sysblk.pcpu, "online");
        return 0;
    }
    regs = sysblk.regs[sysblk.pcpu];

    if (argc > 1)
    {
        U64   reg_value;
        int   reg_num, afp = (regs->CR(0) & CR0_AFP) ? TRUE : FALSE;
        BYTE  equal_sign, c;

        if (argc > 2)
        {
            release_lock(&sysblk.cpulock[sysblk.pcpu]);
            // "Invalid argument '%s'%s"
            WRMSG(HHC02205, "E", argv[1], "");
            return 0;
        }

        if (0
            || sscanf( argv[1], "%d%c%"SCNx64"%c", &reg_num, &equal_sign, &reg_value, &c ) != 3
            || reg_num < 0
            || (afp && reg_num > 15)
            || (!afp && reg_num > 6)
            || (!afp && (reg_num & 1))  /* (must be even numbered: 0,2,4,6) */
            || '=' != equal_sign
        )
        {
            release_lock(&sysblk.cpulock[sysblk.pcpu]);
            // "Invalid argument '%s'%s"
            WRMSG(HHC02205, "E", argv[1], "");
            return 0;
        }

        if (afp) reg_num <<= 1; /* (double) */
        regs->fpr[reg_num]   = (U32) (reg_value >> 32);
        regs->fpr[reg_num+1] = (U32) (reg_value & 0xFFFFFFFFULL);
    }

    display_fregs( regs, buf, sizeof(buf), "HHC02270I " );
    WRMSG(   HHC02270, "I", "Floating point registers" );
    LOGMSG( "%s", buf );

    release_lock(&sysblk.cpulock[sysblk.pcpu]);

    return 0;
}


/*-------------------------------------------------------------------*/
/* fpc command - display floating point control register             */
/*-------------------------------------------------------------------*/
int fpc_cmd(int argc, char *argv[], char *cmdline)
{
REGS *regs;

    UNREFERENCED(cmdline);

    obtain_lock(&sysblk.cpulock[sysblk.pcpu]);

    if (!IS_CPU_ONLINE(sysblk.pcpu))
    {
        release_lock(&sysblk.cpulock[sysblk.pcpu]);
        // "Processor %s%02X: processor is not %s"
        WRMSG(HHC00816, "W", PTYPSTR(sysblk.pcpu), sysblk.pcpu, "online");
        return 0;
    }
    regs = sysblk.regs[sysblk.pcpu];

    if (argc > 1)
    {
        BYTE  c;
        U64   reg_value;

        if (argc > 2)
        {
            release_lock(&sysblk.cpulock[sysblk.pcpu]);
            // "Invalid argument '%s'%s"
            WRMSG(HHC02205, "E", argv[1], "");
            return 0;
        }

        if (0
            || sscanf( argv[1], "%"SCNx64"%c", &reg_value, &c ) != 1
            || reg_value > UINT_MAX
        )
        {
            release_lock(&sysblk.cpulock[sysblk.pcpu]);
            // "Invalid argument '%s'%s"
            WRMSG(HHC02205, "E", argv[1], "");
            return 0;
        }

        regs->fpc = (U32) (reg_value & 0xFFFFFFFFULL);
    }

    // "Floating point control register: %08"PRIX32
    WRMSG(HHC02276, "I", regs->fpc);

    release_lock(&sysblk.cpulock[sysblk.pcpu]);

    return 0;
}


/*-------------------------------------------------------------------*/
/* cr command - display or alter control registers                   */
/*-------------------------------------------------------------------*/
int cr_cmd(int argc, char *argv[], char *cmdline)
{
REGS *regs;
int   cr_num;
BYTE  equal_sign, c;
U64   cr_value;
char buf[512];

    UNREFERENCED(cmdline);

    obtain_lock(&sysblk.cpulock[sysblk.pcpu]);

    if (!IS_CPU_ONLINE(sysblk.pcpu))
    {
        release_lock(&sysblk.cpulock[sysblk.pcpu]);
        WRMSG(HHC00816, "W", PTYPSTR(sysblk.pcpu), sysblk.pcpu, "online");
        return 0;
    }
    regs = sysblk.regs[sysblk.pcpu];

    if (argc > 1)
    {
        if (argc > 2
            || sscanf( argv[1], "%d%c%"SCNx64"%c", &cr_num, &equal_sign, &cr_value, &c ) != 3
            || '=' != equal_sign || cr_num < 0 || cr_num > 15)
        {
            release_lock(&sysblk.cpulock[sysblk.pcpu]);
            WRMSG(HHC02205, "E", argv[1], "");
            return 0;
        }
        if ( ARCH_900 == regs->arch_mode )
            regs->CR_G(cr_num) = (U64)cr_value;
        else
            regs->CR_G(cr_num) = (U32)cr_value;
    }

    display_cregs( regs, buf, sizeof(buf), "HHC02271I " );
    WRMSG(   HHC02271, "I", "Control registers" );
    LOGMSG( "%s", buf );

    release_lock(&sysblk.cpulock[sysblk.pcpu]);

    return 0;
}

/*-------------------------------------------------------------------*/
/* i command - generate I/O attention interrupt for device           */
/*-------------------------------------------------------------------*/
int i_cmd(int argc, char *argv[], char *cmdline)
{
REGS *regs;

    int      rc = 0;
    U16      devnum;
    U16      lcss;
    DEVBLK*  dev;

    UNREFERENCED(cmdline);

    if (argc < 2)
    {
        missing_devnum();
        return -1;
    }

    rc=parse_single_devnum(argv[1],&lcss,&devnum);
    if (rc<0)
    {
        return -1;
    }

    if (!(dev = find_device_by_devnum (lcss, devnum)))
    {
        devnotfound_msg(lcss,devnum);
        return -1;
    }

    rc = device_attention (dev, CSW_ATTN);

    switch (rc) {
        case 0: WRMSG(HHC02230, "I", lcss, devnum);
                break;
        case 1: WRMSG(HHC02231, "E", lcss, devnum);
                break;
        case 2: WRMSG(HHC02232, "E", lcss, devnum);
                break;
        case 3: WRMSG(HHC02233, "E", lcss, devnum);
                break;
    }

    regs = sysblk.regs[sysblk.pcpu];
    if (rc == 3 && IS_CPU_ONLINE(sysblk.pcpu) && CPUSTATE_STOPPED == regs->cpustate)
        WRMSG(HHC02234, "W", devnum );

    return rc;
}


#if defined(OPTION_COUNTING)
/*-------------------------------------------------------------------*/
/* count - display counts                                            */
/*-------------------------------------------------------------------*/
int count_cmd(int argc, char *argv[], char *cmdline)
{
    int     i;                          /* Index                     */
    U64     instcount = 0;              /* Instruction count         */

    UNREFERENCED(argc);
    UNREFERENCED(argv);
    UNREFERENCED(cmdline);

    if (argc > 1 && CMD(argv[1],clear,5) )
    {
        for (i = 0; i < sysblk.maxcpu; i++)
            if (IS_CPU_ONLINE(i))
                sysblk.regs[i]->instcount = sysblk.regs[i]->prevcount = 0;
        for (i = 0; i < OPTION_COUNTING; i++)
            sysblk.count[i] = 0;
    }
    for (i = 0; i < sysblk.maxcpu; i++)
        if (IS_CPU_ONLINE(i))
            instcount += INSTCOUNT(sysblk.regs[i]);
    WRMSG(HHC02254, "I", instcount);

    for (i = 0; i < OPTION_COUNTING; i++)
        WRMSG(HHC02255, "I", i, sysblk.count[i]);

    return 0;
}
#endif


#if defined(OPTION_INSTRUCTION_COUNTING)
/*-------------------------------------------------------------------*/
/* icount command - display instruction counts                       */
/*-------------------------------------------------------------------*/
int icount_cmd(int argc, char *argv[], char *cmdline)
{
    int i, i1, i2, i3;

#define  MAX_ICOUNT_INSTR   1000    /* Maximum number of instructions
                                     in architecture instruction set */

    unsigned char opcode1[MAX_ICOUNT_INSTR];
    unsigned char opcode2[MAX_ICOUNT_INSTR];
    U64 count[MAX_ICOUNT_INSTR];
    U64 total;
    char buf[128];

    UNREFERENCED(cmdline);

    if ( argc > 1 && CMD(argv[1],clear,5) )
    {
        memset(IMAP_FIRST, 0, IMAP_SIZE);
        WRMSG(HHC02204, "I", "instruction counts", "zero");
        return 0;
    }

    if ( argc > 1 && CMD(argv[1],sort,4) )
    {
      memset(opcode1,0x00,sizeof(opcode1));
      memset(opcode2,0x00,sizeof(opcode2));
      memset(count,0x00,sizeof(count));

      /* Collect */
      i = 0;
      total = 0;
      for ( i1 = 0; i1 < 256; i1++ )
      {
        switch(i1)
        {
          case 0x01:
          {
            for ( i2 = 0; i2 < 256; i2++ )
            {
              if (sysblk.imap01[i2])
              {
                opcode1[i] = i1;
                opcode2[i] = i2;
                count[i++] = sysblk.imap01[i2];
                total += sysblk.imap01[i2];
                if (i == (MAX_ICOUNT_INSTR-1) )
                {
                  WRMSG(HHC02252, "E");
                  return 0;
                }
              }
            }
            break;
          }
          case 0xA4:
          {
            for ( i2 = 0; i2 < 256; i2++ )
            {
              if (sysblk.imapa4[i2])
              {
                opcode1[i] = i1;
                opcode2[i] = i2;
                count[i++] = sysblk.imapa4[i2];
                total += sysblk.imapa4[i2];
                if (i == (MAX_ICOUNT_INSTR-1))
                {
                  WRMSG(HHC02252, "E");
                  return 0;
                }
              }
            }
            break;
          }
          case 0xA5:
          {
            for ( i2 = 0; i2 < 16; i2++ )
            {
              if (sysblk.imapa5[i2])
              {
                opcode1[i] = i1;
                opcode2[i] = i2;
                count[i++] = sysblk.imapa5[i2];
                total += sysblk.imapa5[i2];
                if (i == (MAX_ICOUNT_INSTR-1))
                {
                  WRMSG(HHC02252, "E");
                  return 0;
                }
              }
            }
            break;
          }
          case 0xA6:
          {
            for (i2 = 0; i2 < 256; i2++)
            {
              if (sysblk.imapa6[i2])
              {
                opcode1[i] = i1;
                opcode2[i] = i2;
                count[i++] = sysblk.imapa6[i2];
                total += sysblk.imapa6[i2];
                if (i == (MAX_ICOUNT_INSTR-1))
                {
                  WRMSG(HHC02252, "E");
                  return 0;
                }
              }
            }
            break;
          }
          case 0xA7:
          {
            for (i2 = 0; i2 < 16; i2++)
            {
              if (sysblk.imapa7[i2])
              {
                opcode1[i] = i1;
                opcode2[i] = i2;
                count[i++] = sysblk.imapa7[i2];
                total += sysblk.imapa7[i2];
                if (i == (MAX_ICOUNT_INSTR-1))
                {
                  WRMSG(HHC02252, "E");
                  return 0;
                }
              }
            }
            break;
          }
          case 0xB2:
          {
            for (i2 = 0; i2 < 256; i2++)
            {
              if (sysblk.imapb2[i2])
              {
                opcode1[i] = i1;
                opcode2[i] = i2;
                count[i++] = sysblk.imapb2[i2];
                total += sysblk.imapb2[i2];
                if (i == (MAX_ICOUNT_INSTR-1))
                {
                  WRMSG(HHC02252, "E");
                  return 0;
                }
              }
            }
            break;
          }
          case 0xB3:
          {
            for (i2 = 0; i2 < 256; i2++)
            {
              if (sysblk.imapb3[i2])
              {
                opcode1[i] = i1;
                opcode2[i] = i2;
                count[i++] = sysblk.imapb3[i2];
                total += sysblk.imapb3[i2];
                if (i == (MAX_ICOUNT_INSTR-1))
                {
                  WRMSG(HHC02252, "E");
                  return 0;
                }
              }
            }
            break;
          }
          case 0xB9:
          {
            for (i2 = 0; i2 < 256; i2++)
            {
              if (sysblk.imapb9[i2])
              {
                opcode1[i] = i1;
                opcode2[i] = i2;
                count[i++] = sysblk.imapb9[i2];
                total += sysblk.imapb9[i2];
                if (i == (MAX_ICOUNT_INSTR-1))
                {
                  WRMSG(HHC02252, "E");
                  return 0;
                }
              }
            }
            break;
          }
          case 0xC0:
          {
            for (i2 = 0; i2 < 16; i2++)
            {
              if (sysblk.imapc0[i2])
              {
                opcode1[i] = i1;
                opcode2[i] = i2;
                count[i++] = sysblk.imapc0[i2];
                total += sysblk.imapc0[i2];
                if (i == (MAX_ICOUNT_INSTR-1))
                {
                  WRMSG(HHC02252, "E");
                  return 0;
                }
              }
            }
            break;
          }
          case 0xC2:
          {
            for (i2 = 0; i2 < 16; i2++)
            {
              if (sysblk.imapc2[i2])
              {
                opcode1[i] = i1;
                opcode2[i] = i2;
                count[i++] = sysblk.imapc2[i2];
                total += sysblk.imapc2[i2];
                if (i == (MAX_ICOUNT_INSTR-1))
                {
                  WRMSG(HHC02252, "E");
                  return 0;
                }
              }
            }
            break;
          }
          case 0xC4:
          {
            for (i2 = 0; i2 < 16; i2++)
            {
              if (sysblk.imapc4[i2])
              {
                opcode1[i] = i1;
                opcode2[i] = i2;
                count[i++] = sysblk.imapc4[i2];
                total += sysblk.imapc4[i2];
                if (i == (MAX_ICOUNT_INSTR-1))
                {
                  WRMSG(HHC02252,"E");
                  return 0;
                }
              }
            }
            break;
          }
          case 0xC6:
          {
            for (i2 = 0; i2 < 16; i2++)
            {
              if (sysblk.imapc6[i2])
              {
                opcode1[i] = i1;
                opcode2[i] = i2;
                count[i++] = sysblk.imapc6[i2];
                total += sysblk.imapc6[i2];
                if (i == (MAX_ICOUNT_INSTR-1))
                {
                  WRMSG(HHC02252,"E");
                  return 0;
                }
              }
            }
            break;
          }
          case 0xC8:
          {
            for (i2 = 0; i2 < 16; i2++)
            {
              if (sysblk.imapc8[i2])
              {
                opcode1[i] = i1;
                opcode2[i] = i2;
                count[i++] = sysblk.imapc8[i2];
                total += sysblk.imapc8[i2];
                if (i == (MAX_ICOUNT_INSTR-1))
                {
                  WRMSG(HHC02252, "E");
                  return 0;
                }
              }
            }
            break;
          }
          case 0xE3:
          {
            for (i2 = 0; i2 < 256; i2++)
            {
              if (sysblk.imape3[i2])
              {
                opcode1[i] = i1;
                opcode2[i] = i2;
                count[i++] = sysblk.imape3[i2];
                total += sysblk.imape3[i2];
                if (i == (MAX_ICOUNT_INSTR-1))
                {
                  WRMSG(HHC02252,"E");
                  return 0;
                }
              }
            }
            break;
          }
          case 0xE4:
          {
            for (i2 = 0; i2 < 256; i2++)
            {
              if (sysblk.imape4[i2])
              {
                opcode1[i] = i1;
                opcode2[i] = i2;
                count[i++] = sysblk.imape4[i2];
                total += sysblk.imape4[i2];
                if (i == (MAX_ICOUNT_INSTR-1))
                {
                  WRMSG(HHC02252,"E");
                  return 0;
                }
              }
            }
            break;
          }
          case 0xE5:
          {
            for (i2 = 0; i2 < 256; i2++)
            {
              if (sysblk.imape5[i2])
              {
                opcode1[i] = i1;
                opcode2[i] = i2;
                count[i++] = sysblk.imape5[i2];
                total += sysblk.imape5[i2];
                if (i == (MAX_ICOUNT_INSTR-1))
                {
                  WRMSG(HHC02252,"E");
                  return 0;
                }
              }
            }
            break;
          }
          case 0xEB:
          {
            for (i2 = 0; i2 < 256; i2++)
            {
              if (sysblk.imapeb[i2])
              {
                opcode1[i] = i1;
                opcode2[i] = i2;
                count[i++] = sysblk.imapeb[i2];
                total += sysblk.imapeb[i2];
                if (i == (MAX_ICOUNT_INSTR-1))
                {
                  WRMSG(HHC02252,"E");
                  return 0;
                }
              }
            }
            break;
          }
          case 0xEC:
          {
            for (i2 = 0; i2 < 256; i2++)
            {
              if (sysblk.imapec[i2])
              {
                opcode1[i] = i1;
                opcode2[i] = i2;
                count[i++] = sysblk.imapec[i2];
                total += sysblk.imapec[i2];
                if (i == (MAX_ICOUNT_INSTR-1))
                {
                  WRMSG(HHC02252,"E");
                  return 0;
                }
              }
            }
            break;
          }
          case 0xED:
          {
            for (i2 = 0; i2 < 256; i2++)
            {
              if (sysblk.imaped[i2])
              {
                opcode1[i] = i1;
                opcode2[i] = i2;
                count[i++] = sysblk.imaped[i2];
                total += sysblk.imaped[i2];
                if (i == (MAX_ICOUNT_INSTR-1))
                {
                  WRMSG(HHC02252,"E");
                  return 0;
                }
              }
            }
            break;
          }
          default:
          {
            if (sysblk.imapxx[i1])
            {
              opcode1[i] = i1;
              opcode2[i] = 0;
              count[i++] = sysblk.imapxx[i1];
              total += sysblk.imapxx[i1];
              if (i == (MAX_ICOUNT_INSTR-1))
              {
                WRMSG(HHC02252,"E");
                return 0;
              }
            }
            break;
          }
        }
      }

      /* Sort */
      for (i1 = 0; i1 < i; i1++)
      {
        /* Find Highest */
        for (i2 = i1, i3 = i1; i2 < i; i2++)
        {
          if (count[i2] > count[i3])
            i3 = i2;
        }
        /* Exchange */
        opcode1[(MAX_ICOUNT_INSTR-1)] = opcode1[i1];
        opcode2[(MAX_ICOUNT_INSTR-1)] = opcode2[i1];
        count  [(MAX_ICOUNT_INSTR-1)] = count  [i1];

        opcode1[i1] = opcode1[i3];
        opcode2[i1] = opcode2[i3];
        count  [i1] = count  [i3];

        opcode1[i3] = opcode1[(MAX_ICOUNT_INSTR-1)];
        opcode2[i3] = opcode2[(MAX_ICOUNT_INSTR-1)];
        count  [i3] = count  [(MAX_ICOUNT_INSTR-1)];
      }

#define  ICOUNT_WIDTH  "12"     /* Print field width */

      /* Print */
      WRMSG(HHC02292, "I", "Sorted instruction count display:");
      for (i1 = 0; i1 < i; i1++)
      {
        switch(opcode1[i1])
        {
          case 0x01:
          {
            MSGBUF( buf, "Inst '%2.2X%2.2X' count %" ICOUNT_WIDTH PRIu64" (%2d%%)", opcode1[i1], opcode2[i1], count[i1], (int) (count[i1] * 100 / total));
            WRMSG(HHC02292, "I", buf);
            break;
          }
          case 0xA4:
          {
            MSGBUF( buf, "Inst '%2.2X%2.2X' count %" ICOUNT_WIDTH PRIu64" (%2d%%)", opcode1[i1], opcode2[i1], count[i1], (int) (count[i1] * 100 / total));
            WRMSG(HHC02292, "I", buf);
            break;
          }
          case 0xA5:
          {
            MSGBUF( buf, "Inst '%2.2Xx%1.1X' count %" ICOUNT_WIDTH PRIu64" (%2d%%)", opcode1[i1], opcode2[i1], count[i1], (int) (count[i1] * 100 / total));
            WRMSG(HHC02292, "I", buf);
            break;
          }
          case 0xA6:
          {
            MSGBUF( buf, "Inst '%2.2X%2.2X' count %" ICOUNT_WIDTH PRIu64" (%2d%%)", opcode1[i1], opcode2[i1], count[i1], (int) (count[i1] * 100 / total));
            WRMSG(HHC02292, "I", buf);
            break;
          }
          case 0xA7:
          {
            MSGBUF( buf, "Inst '%2.2Xx%1.1X' count %" ICOUNT_WIDTH PRIu64" (%2d%%)", opcode1[i1], opcode2[i1], count[i1], (int) (count[i1] * 100 / total));
            WRMSG(HHC02292, "I", buf);
            break;
          }
          case 0xB2:
          {
            MSGBUF( buf, "Inst '%2.2X%2.2X' count %" ICOUNT_WIDTH PRIu64" (%2d%%)", opcode1[i1], opcode2[i1], count[i1], (int) (count[i1] * 100 / total));
            WRMSG(HHC02292, "I", buf);
            break;
          }
          case 0xB3:
          {
            MSGBUF( buf, "Inst '%2.2X%2.2X' count %" ICOUNT_WIDTH PRIu64" (%2d%%)", opcode1[i1], opcode2[i1], count[i1], (int) (count[i1] * 100 / total));
            WRMSG(HHC02292, "I", buf);
            break;
          }
          case 0xB9:
          {
            MSGBUF( buf, "Inst '%2.2X%2.2X' count %" ICOUNT_WIDTH PRIu64" (%2d%%)", opcode1[i1], opcode2[i1], count[i1], (int) (count[i1] * 100 / total));
            WRMSG(HHC02292, "I", buf);
            break;
          }
          case 0xC0:
          {
            MSGBUF( buf, "Inst '%2.2Xx%1.1X' count %" ICOUNT_WIDTH PRIu64" (%2d%%)", opcode1[i1], opcode2[i1], count[i1], (int) (count[i1] * 100 / total));
            WRMSG(HHC02292, "I", buf);
            break;
          }
          case 0xC2:
          {
            MSGBUF( buf, "Inst '%2.2Xx%1.1X' count %" ICOUNT_WIDTH PRIu64" (%2d%%)", opcode1[i1], opcode2[i1], count[i1], (int) (count[i1] * 100 / total));
            WRMSG(HHC02292, "I", buf);
            break;
          }
          case 0xC4:
          {
            MSGBUF( buf, "Inst '%2.2Xx%1.1X' count %" ICOUNT_WIDTH PRIu64" (%2d%%)", opcode1[i1], opcode2[i1], count[i1], (int) (count[i1] * 100 / total));
            WRMSG(HHC02292, "I", buf);
            break;
          }
          case 0xC6:
          {
            MSGBUF( buf, "Inst '%2.2Xx%1.1X' count %" ICOUNT_WIDTH PRIu64" (%2d%%)", opcode1[i1], opcode2[i1], count[i1], (int) (count[i1] * 100 / total));
            WRMSG(HHC02292, "I", buf);
            break;
          }
          case 0xC8:
          {
            MSGBUF( buf, "Inst '%2.2Xx%1.1X' count %" ICOUNT_WIDTH PRIu64" (%2d%%)", opcode1[i1], opcode2[i1], count[i1], (int) (count[i1] * 100 / total));
            WRMSG(HHC02292, "I", buf);
            break;
          }
          case 0xE3:
          {
            MSGBUF( buf, "Inst '%2.2X%2.2X' count %" ICOUNT_WIDTH PRIu64" (%2d%%)", opcode1[i1], opcode2[i1], count[i1], (int) (count[i1] * 100 / total));
            WRMSG(HHC02292, "I", buf);
            break;
          }
          case 0xE4:
          {
            MSGBUF( buf, "Inst '%2.2X%2.2X' count %" ICOUNT_WIDTH PRIu64" (%2d%%)", opcode1[i1], opcode2[i1], count[i1], (int) (count[i1] * 100 / total));
            WRMSG(HHC02292, "I", buf);
            break;
          }
          case 0xE5:
          {
            MSGBUF( buf, "Inst '%2.2X%2.2X' count %" ICOUNT_WIDTH PRIu64" (%2d%%)", opcode1[i1], opcode2[i1], count[i1], (int) (count[i1] * 100 / total));
            WRMSG(HHC02292, "I", buf);
            break;
          }
          case 0xEB:
          {
            MSGBUF( buf, "Inst '%2.2X%2.2X' count %" ICOUNT_WIDTH PRIu64" (%2d%%)", opcode1[i1], opcode2[i1], count[i1], (int) (count[i1] * 100 / total));
            WRMSG(HHC02292, "I", buf);
            break;
          }
          case 0xEC:
          {
            MSGBUF( buf, "Inst '%2.2X%2.2X' count %" ICOUNT_WIDTH PRIu64" (%2d%%)", opcode1[i1], opcode2[i1], count[i1], (int) (count[i1] * 100 / total));
            WRMSG(HHC02292, "I", buf);
            break;
          }
          case 0xED:
          {
            MSGBUF( buf, "Inst '%2.2X%2.2X' count %" ICOUNT_WIDTH PRIu64" (%2d%%)", opcode1[i1], opcode2[i1], count[i1], (int) (count[i1] * 100 / total));
            WRMSG(HHC02292, "I", buf);
            break;
          }
          default:
          {
            MSGBUF( buf, "Inst '%2.2X'   count %" ICOUNT_WIDTH PRIu64" (%2d%%)", opcode1[i1], count[i1], (int) (count[i1] * 100 / total));
            WRMSG(HHC02292, "I", buf);
            break;
          }
        }
      }
      return 0;
    }

    WRMSG(HHC02292, "I", "Instruction count display:");
    for (i1 = 0; i1 < 256; i1++)
    {
        switch (i1)
        {
            case 0x01:
                for (i2 = 0; i2 < 256; i2++)
                    if (sysblk.imap01[i2])
                    {
                        MSGBUF( buf, "Inst '%2.2X%2.2X' count %" ICOUNT_WIDTH PRIu64,
                            i1, i2, sysblk.imap01[i2]);
                        WRMSG(HHC02292, "I", buf);
                    }
                break;
            case 0xA4:
                for (i2 = 0; i2 < 256; i2++)
                    if (sysblk.imapa4[i2])
                    {
                        MSGBUF( buf, "Inst '%2.2X%2.2X' count %" ICOUNT_WIDTH PRIu64,
                            i1, i2, sysblk.imapa4[i2]);
                        WRMSG(HHC02292, "I", buf);
                    }
                break;
            case 0xA5:
                for (i2 = 0; i2 < 16; i2++)
                    if (sysblk.imapa5[i2])
                    {
                        MSGBUF( buf, "Inst '%2.2Xx%1.1X' count %" ICOUNT_WIDTH PRIu64,
                            i1, i2, sysblk.imapa5[i2]);
                        WRMSG(HHC02292, "I", buf);
                    }
                break;
            case 0xA6:
                for (i2 = 0; i2 < 256; i2++)
                    if (sysblk.imapa6[i2])
                    {
                        MSGBUF( buf, "Inst '%2.2X%2.2X' count %" ICOUNT_WIDTH PRIu64,
                            i1, i2, sysblk.imapa6[i2]);
                        WRMSG(HHC02292, "I", buf);
                    }
                break;
            case 0xA7:
                for (i2 = 0; i2 < 16; i2++)
                    if (sysblk.imapa7[i2])
                    {
                        MSGBUF( buf, "Inst '%2.2Xx%1.1X' count %" ICOUNT_WIDTH PRIu64,
                            i1, i2, sysblk.imapa7[i2]);
                        WRMSG(HHC02292, "I", buf);
                    }
                break;
            case 0xB2:
                for (i2 = 0; i2 < 256; i2++)
                    if (sysblk.imapb2[i2])
                    {
                        MSGBUF( buf, "Inst '%2.2X%2.2X' count %" ICOUNT_WIDTH PRIu64,
                            i1, i2, sysblk.imapb2[i2]);
                        WRMSG(HHC02292, "I", buf);
                    }
                break;
            case 0xB3:
                for (i2 = 0; i2 < 256; i2++)
                    if (sysblk.imapb3[i2])
                    {
                        MSGBUF( buf, "Inst '%2.2X%2.2X' count %" ICOUNT_WIDTH PRIu64,
                            i1, i2, sysblk.imapb3[i2]);
                        WRMSG(HHC02292, "I", buf);
                    }
                break;
            case 0xB9:
                for (i2 = 0; i2 < 256; i2++)
                    if (sysblk.imapb9[i2])
                    {
                        MSGBUF( buf, "Inst '%2.2X%2.2X' count %" ICOUNT_WIDTH PRIu64,
                            i1, i2, sysblk.imapb9[i2]);
                        WRMSG(HHC02292, "I", buf);
                    }
                break;
            case 0xC0:
                for (i2 = 0; i2 < 16; i2++)
                    if (sysblk.imapc0[i2])
                    {
                        MSGBUF( buf, "Inst '%2.2Xx%1.1X' count %" ICOUNT_WIDTH PRIu64,
                            i1, i2, sysblk.imapc0[i2]);
                        WRMSG(HHC02292, "I", buf);
                    }
                break;
            case 0xC2:                                                      /*@Z9*/
                for (i2 = 0; i2 < 16; i2++)                                  /*@Z9*/
                    if (sysblk.imapc2[i2])                                   /*@Z9*/
                    {
                        MSGBUF( buf, "Inst '%2.2Xx%1.1X' count %" ICOUNT_WIDTH PRIu64,  /*@Z9*/
                            i1, i2, sysblk.imapc2[i2]);                     /*@Z9*/
                        WRMSG(HHC02292, "I", buf);
                    }
                break;                                                      /*@Z9*/
            case 0xC4:
                for (i2 = 0; i2 < 16; i2++)
                    if (sysblk.imapc4[i2])
                    {
                        MSGBUF( buf, "Inst '%2.2Xx%1.1X' count %" ICOUNT_WIDTH PRIu64,
                            i1, i2, sysblk.imapc4[i2]);
                        WRMSG(HHC02292, "I", buf);
                    }
                break;
            case 0xC6:
                for (i2 = 0; i2 < 16; i2++)
                    if (sysblk.imapc6[i2])
                    {
                        MSGBUF( buf, "Inst '%2.2Xx%1.1X' count %" ICOUNT_WIDTH PRIu64,
                            i1, i2, sysblk.imapc6[i2]);
                        WRMSG(HHC02292, "I", buf);
                    }
                break;
            case 0xC8:
                for (i2 = 0; i2 < 16; i2++)
                    if (sysblk.imapc8[i2])
                    {
                        MSGBUF( buf, "Inst '%2.2Xx%1.1X' count %" ICOUNT_WIDTH PRIu64,
                            i1, i2, sysblk.imapc8[i2]);
                        WRMSG(HHC02292, "I", buf);
                    }
                break;
            case 0xE3:
                for (i2 = 0; i2 < 256; i2++)
                    if (sysblk.imape3[i2])
                    {
                        MSGBUF( buf, "Inst '%2.2X%2.2X' count %" ICOUNT_WIDTH PRIu64,
                            i1, i2, sysblk.imape3[i2]);
                        WRMSG(HHC02292, "I", buf);
                    }
                break;
            case 0xE4:
                for (i2 = 0; i2 < 256; i2++)
                    if (sysblk.imape4[i2])
                    {
                        MSGBUF( buf, "Inst '%2.2X%2.2X' count %" ICOUNT_WIDTH PRIu64,
                            i1, i2, sysblk.imape4[i2]);
                        WRMSG(HHC02292, "I", buf);
                    }
                break;
            case 0xE5:
                for (i2 = 0; i2 < 256; i2++)
                    if (sysblk.imape5[i2])
                    {
                        MSGBUF( buf, "Inst '%2.2X%2.2X' count %" ICOUNT_WIDTH PRIu64,
                            i1, i2, sysblk.imape5[i2]);
                        WRMSG(HHC02292, "I", buf);
                    }
                break;
            case 0xEB:
                for (i2 = 0; i2 < 256; i2++)
                    if (sysblk.imapeb[i2])
                    {
                        MSGBUF( buf, "Inst '%2.2X%2.2X' count %" ICOUNT_WIDTH PRIu64,
                            i1, i2, sysblk.imapeb[i2]);
                        WRMSG(HHC02292, "I", buf);
                    }
                break;
            case 0xEC:
                for (i2 = 0; i2 < 256; i2++)
                    if (sysblk.imapec[i2])
                    {
                        MSGBUF( buf, "Inst '%2.2X%2.2X' count %" ICOUNT_WIDTH PRIu64,
                            i1, i2, sysblk.imapec[i2]);
                        WRMSG(HHC02292, "I", buf);
                    }
                break;
            case 0xED:
                for (i2 = 0; i2 < 256; i2++)
                    if (sysblk.imaped[i2])
                    {
                        MSGBUF( buf, "Inst '%2.2X%2.2X' count %" ICOUNT_WIDTH PRIu64,
                            i1, i2, sysblk.imaped[i2]);
                        WRMSG(HHC02292, "I", buf);
                    }
                break;
            default:
                if (sysblk.imapxx[i1])
                {
                    MSGBUF( buf, "Inst '%2.2X'   count %" ICOUNT_WIDTH PRIu64,
                        i1, sysblk.imapxx[i1]);
                    WRMSG(HHC02292, "I", buf);
                }
                break;
        }
    }
    return 0;
}

#endif /*defined(OPTION_INSTRUCTION_COUNTING)*/

#endif // !defined(_GEN_ARCH)
