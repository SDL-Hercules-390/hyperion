/* HSCEMODE.C   (C) Copyright Roger Bowler, 1999-2012                */
/*              (C) Copyright Jan Jaeger, 1999-2012                  */
/*              (C) Copyright "Fish" (David B. Trout), 2002-2009     */
/*              (C) Copyright TurboHercules, SAS 2010-2011           */
/*              (C) and others 2011-2023                             */
/*              CE mode functions                                    */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#include "hstdinc.h"

DISABLE_GCC_UNUSED_FUNCTION_WARNING;

#define _HSCEMODE_C_
#define _HENGINE_DLL_

#include "hercules.h"
#include "opcode.h"

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
            // 64-bit due to FEATURE_SIE normally being #defined,
            // causing _FEATURE_ZSIE to be #defined.

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

#if defined(_ARCH_NUM_1)
 #define  _GEN_ARCH _ARCH_NUM_1      // (set next build architecture)
 #include "hscemode.c"              // (compile ourselves again)
#endif

#if defined(_ARCH_NUM_2)
 #undef   _GEN_ARCH
 #define  _GEN_ARCH _ARCH_NUM_2      // (set next build architecture)
 #include "hscemode.c"              // (compile ourselves again)
#endif


/*-------------------------------------------------------------------*/
/*       NON-architecure-dependent code from here onward             */
/*-------------------------------------------------------------------*/

static inline int devnotfound_msg( const U16 lcss, const U16 devnum )
{
    // "%1d:%04X device not found"
    WRMSG( HHC02200, "E", lcss, devnum );
    return -1;
}

static inline void missing_devnum()
{
    // "Device number missing"
    WRMSG( HHC02201, "E" );
}


/*-------------------------------------------------------------------*/
/*                (aea command helper function)                      */
/*-------------------------------------------------------------------*/
static inline const char* aea_mode_str( const BYTE mode)
{
    static const char* name[] =
    {
        "DAT-Off",
        "Primary",
        "AR",
        "Secondary",
        "Home",
        0,
        0,
        0,
        "PER/DAT-Off",
        "PER/Primary",
        "PER/AR",
        "PER/Secondary",
        "PER/Home"
    };

    return name[ (mode & 0x0f) | ((mode & 0xf0) ? 8 : 0) ];
}


/*-------------------------------------------------------------------*/
/*                (aea command helper function)                      */
/*-------------------------------------------------------------------*/
static void report_aea( REGS* regs )
{
    int    i;
    char   buf[128];
    char   wrk[128];

    // HHC02282 == "%s"  // (aea_cmd)

    MSGBUF( buf, "aea mode   %s", aea_mode_str( regs->aea_mode ));
    WRMSG( HHC02282, "I", buf );

    //------------------------------------------------------------

    STRLCPY( buf, "aea ar    " );

    for (i = USE_HOME_SPACE; i < 0; i++)
    {
        if (regs->AEA_AR(i) > 0)
              MSGBUF( wrk, " %2.2X", regs->AEA_AR(i));
        else  MSGBUF( wrk, " %2d",   regs->AEA_AR(i));

        STRLCAT( buf, wrk);
    }

    for (i=0; i < 16; i++)
    {
        if (regs->AEA_AR(i) > 0)
              MSGBUF( wrk, " %2.2X", regs->AEA_AR(i));
        else  MSGBUF( wrk, " %2d",   regs->AEA_AR(i));

        STRLCAT( buf, wrk);
    }

    WRMSG( HHC02282, "I", buf );

    //------------------------------------------------------------

    STRLCPY( buf, "aea common            " );

    if (regs->AEA_COMMON( CR_ASD_REAL ) > 0)
          MSGBUF( wrk, " %2.2X", regs->AEA_COMMON( CR_ASD_REAL ));
    else  MSGBUF( wrk, " %2d",   regs->AEA_COMMON( CR_ASD_REAL ));

    STRLCAT( buf, wrk);

    for (i=0; i < 16; i++)
    {
        if (regs->AEA_COMMON(i) > 0)
              MSGBUF( wrk, " %2.2X", regs->AEA_COMMON(i));
        else  MSGBUF( wrk, " %2d",   regs->AEA_COMMON(i));

        STRLCAT( buf, wrk);
    }

    WRMSG( HHC02282, "I", buf );

    //------------------------------------------------------------

    MSGBUF( buf, "aea cr[1]  %16.16"PRIX64, regs->CR_G(      1      )); WRMSG( HHC02282, "I", buf );
    MSGBUF( buf, "    cr[7]  %16.16"PRIX64, regs->CR_G(      7      )); WRMSG( HHC02282, "I", buf );
    MSGBUF( buf, "    cr[13] %16.16"PRIX64, regs->CR_G(     13      )); WRMSG( HHC02282, "I", buf );
    MSGBUF( buf, "    cr[r]  %16.16"PRIX64, regs->CR_G( CR_ASD_REAL )); WRMSG( HHC02282, "I", buf );

    //------------------------------------------------------------

    for (i=0; i < 16; i++)
    {
        if (regs->AEA_AR(i) > 15)
        {
            MSGBUF( buf, "    alb[%d] %16.16"PRIX64, i, regs->CR_G( CR_ALB_OFFSET + i ));
            WRMSG( HHC02282, "I", buf );
        }
    }
}


/*-------------------------------------------------------------------*/
/* aea - display aea values                                          */
/*-------------------------------------------------------------------*/
int aea_cmd( int argc, char* argv[], char* cmdline )
{
    REGS*  regs;

    UNREFERENCED( argc );
    UNREFERENCED( argv );
    UNREFERENCED( cmdline );

    obtain_lock( &sysblk.cpulock[ sysblk.pcpu ]);
    {
        if (!IS_CPU_ONLINE( sysblk.pcpu ))
        {
            release_lock( &sysblk.cpulock[ sysblk.pcpu ]);
            // "Processor %s%02X: processor is not %s"
            WRMSG( HHC00816, "E", PTYPSTR( sysblk.pcpu ), sysblk.pcpu, "online" );
            return -1;
        }

        regs = sysblk.regs[ sysblk.pcpu ];
        report_aea( regs );

        if (regs->sie_active)
        {
            // HHC02282 == "%s"  // (aea_cmd)
            WRMSG( HHC02282, "I", "aea SIE" );

            regs = GUESTREGS;
            report_aea( regs );
        }
    }
    release_lock( &sysblk.cpulock[ sysblk.pcpu ]);
    return 0;
}


/*-------------------------------------------------------------------*/
/* traceopt - perform display_inst traditionally or new              */
/*-------------------------------------------------------------------*/
int traceopt_cmd( int argc, char* argv[], char* cmdline )
{
    int   i;
    char  msgbuf[ 64 ];
    BYTE  showregsfirst  = sysblk.showregsfirst;
    BYTE  showregsnone   = sysblk.showregsnone;
    BYTE  noch9oflow     = sysblk.noch9oflow;

    UNREFERENCED( cmdline );

    UPPER_ARGV_0( argv );

    if (argc > 3)
    {
        // "Invalid command usage. Type 'help %s' for assistance."
        WRMSG( HHC02299, "E", argv[0] );
        return -1;
    }

    if (argc <= 1)
    {
        MSGBUF( msgbuf, "%s%s"
            , showregsnone  ? "NOREGS" : showregsfirst ? "REGSFIRST" : "TRADITIONAL"
            , noch9oflow ? " NOCH9OFLOW" : ""
        );

        // "%-14s: %s"
        WRMSG( HHC02203, "I", argv[0], msgbuf );
    }
    else // (argc >= 2)
    {
        noch9oflow = FALSE;     // (reset to default)

        for (i=1; i < argc; i++)
        {
            if (CMD( argv[i], TRADITIONAL, 4 ))
            {
                showregsfirst = FALSE;
                showregsnone  = FALSE;
            }
            else if (CMD( argv[i], REGSFIRST, 4 ))
            {
                showregsfirst = TRUE;
                showregsnone  = FALSE;
            }
            else if (CMD( argv[i], NOREGS, 4 ))
            {
                showregsfirst = FALSE;
                showregsnone  = TRUE;
            }
            else if (CMD( argv[i], NOCH9OFLOW, 5 ))
            {
                noch9oflow = TRUE;
            }
            else
            {
                // "Invalid value %s specified for %s"
                WRMSG( HHC01451, "E", argv[i], argv[0] );
                return -1;
            }
        }

        sysblk.showregsfirst = showregsfirst;
        sysblk.showregsnone  = showregsnone;
        sysblk.noch9oflow    = noch9oflow;

        if (MLVL( VERBOSE ))
        {
            MSGBUF( msgbuf, "%s%s"
                , showregsnone  ? "NOREGS" : showregsfirst ? "REGSFIRST" : "TRADITIONAL"
                , noch9oflow ? " NOCH9OFLOW" : ""
            );

            // "%-14s set to %s"
            WRMSG( HHC02204, "I", argv[0], msgbuf );
        }
    }

    return 0;
}


/*-------------------------------------------------------------------*/
/* aia - display aia values                                          */
/*-------------------------------------------------------------------*/
DLL_EXPORT int aia_cmd( int argc, char* argv[], char* cmdline )
{
    UNREFERENCED( argc );
    UNREFERENCED( argv );
    UNREFERENCED( cmdline );

    obtain_lock( &sysblk.cpulock[ sysblk.pcpu ]);
    {
        REGS*  regs;
        char   buf[128];

        if (!IS_CPU_ONLINE( sysblk.pcpu ))
        {
            release_lock( &sysblk.cpulock[ sysblk.pcpu ]);

            // "Processor %s%02X: processor is not %s"
            WRMSG( HHC00816, "E", PTYPSTR( sysblk.pcpu ),
                sysblk.pcpu, "online" );
            return -1;
        }

        regs = sysblk.regs[ sysblk.pcpu ];

        MSGBUF( buf, "AIV %16.16"PRIx64" aip %p ip %p aie %p",

                regs->AIV_G,
                regs->aip,
                regs->ip,
                regs->aie
        );

        // "%s" (aia_cmd)
        WRMSG( HHC02283, "I", buf );

        if (regs->sie_active)
        {
            char wrk[128];

            regs = GUESTREGS;

            MSGBUF( wrk, "AIV %16.16"PRIx64" aip %p ip %p aie %p"

                , regs->AIV_G
                , regs->aip
                , regs->ip
                , regs->aie
            );

            STRLCPY( buf, "SIE: " );
            STRLCAT( buf, wrk );

            // "%s" (aia_cmd)
            WRMSG( HHC02283, "I", buf );
        }
    }
    release_lock ( &sysblk.cpulock[ sysblk.pcpu ]);

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
    shift = regs->arch_mode == ARCH_370_IDX ? 11 : 12;
    bytemask = regs->arch_mode == ARCH_370_IDX ? 0x1FFFFF : 0x3FFFFF;
    pagemask = regs->arch_mode == ARCH_370_IDX ? 0x00E00000 :
               regs->arch_mode == ARCH_390_IDX ? 0x7FC00000 :
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
        regs = GUESTREGS;
        shift = GUESTREGS->arch_mode == ARCH_370_IDX ? 11 : 12;
        bytemask = regs->arch_mode == ARCH_370_IDX ? 0x1FFFFF : 0x3FFFFF;
        pagemask = regs->arch_mode == ARCH_370_IDX ? 0x00E00000 :
                   regs->arch_mode == ARCH_390_IDX ? 0x7FC00000 :
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
    int    cpu, rc = 0;

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
    case ARCH_370_IDX:
        rc = s370_archdep_pr_cmd( regs, argc, argv ); break;
#endif
#if defined( _390 )
    case ARCH_390_IDX:
        rc = s390_archdep_pr_cmd( regs, argc, argv ); break;
#endif
#if defined( _900 )
    case ARCH_900_IDX:
        rc = z900_archdep_pr_cmd( regs, argc, argv ); break;
#endif
    default: CRASH();
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
                    && (sysblk.arch_mode == ARCH_390_IDX
                        || sysblk.arch_mode == ARCH_900_IDX))
                newam = 31;
            else if (strcmp(argv[n]+3,"64") == 0
                    && sysblk.arch_mode == ARCH_900_IDX)
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
            || sysblk.arch_mode == ARCH_390_IDX
            || sysblk.arch_mode == ARCH_900_IDX))
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
        regs->aie = INVALID_AIE;
    }

    /* Display the PSW and PSW field by field */
    DISPLAY_PSW( regs, buf );
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
    else if ( sysblk.instbreak )                    rc = 3; /* Instruction Step */
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
/* tf -- trace file command: ON|OFF FILE=filename MAX=nnnS NOSTOP    */
/*-------------------------------------------------------------------*/
int tf_cmd( int argc, char* argv[], char* cmdline )
{
    U64          maxsize    = 0;
    FILE*        traceFILE  = NULL;

    char         filename[MAX_PATH+1] = {0};
    char         buf[sizeof(filename)+32] = {0};

    int          mgs = 0;
    char         mg = 'M';
    const char*  quote = "\"";
    bool         enable = false;
    bool         nostop = false;

    UNREFERENCED( cmdline );
    UPPER_ARGV_0( argv );

    // Too many arguments?

    if (argc > 5)
    {
        // "Invalid command usage. Type 'help %s' for assistance."
        WRMSG( HHC02299, "E", argv[0] );
        return -1;
    }

    // Parse new values...

    maxsize = sysblk.maxtracesize;
    enable = sysblk.traceFILE ? true : false;
#ifdef FISH_TFNOSTOP2
    nostop = sysblk.tfnostop2 ? true : false;
#else
    nostop = sysblk.tfnostop ? true : false;
#endif

    if (argc > 1)
    {
        int  i;
        for (i=1; i < argc; i++)
        {
            if (0
                || strcasecmp( argv[i], "ON"   ) == 0
                || strcasecmp( argv[i], "OPEN" ) == 0
            )
            {
                enable = true;
            }
            else if (0
                || strcasecmp( argv[i], "OFF"   ) == 0
                || strcasecmp( argv[i], "CLOSE" ) == 0
            )
            {
                enable = false;
            }
            else if (0
                || strcasecmp( argv[i], "NOSTOP" ) == 0
                || strcasecmp( argv[i], "CONT"   ) == 0
            )
            {
                nostop = true;
            }
            else if (0
                || strcasecmp( argv[i], "STOP"   ) == 0
                || strcasecmp( argv[i], "NOCONT" ) == 0
            )
            {
                nostop = false;
            }
            else if (strncasecmp( argv[i], "FILE=", 5 ) == 0)
            {
                if (traceFILE)
                    VERIFY( 0 == fclose( traceFILE ));

                STRLCPY( filename, argv[i]+5 );
                traceFILE = NULL;

                if (!filename[0])
                {
                    // "Invalid %s= value: %s"
                    WRMSG( HHC02380, "E", "FILE", "(null)" );
                    if (traceFILE)
                        VERIFY( 0 == fclose( traceFILE ));
                    return -1;
                }

                if (!(traceFILE = fopen( filename, "wb" )))
                {
                    // "Trace file open error %s: \"%s\""
                    WRMSG( HHC02377, "E", strerror( errno ), filename );
                    return -1;
                }
            }
            else if (strncasecmp( argv[i], "MAX=", 4 ) == 0)
            {
                char* endptr;
                U64 factor = ONE_GIGABYTE;
                errno = 0;
                maxsize = strtoul( argv[i]+4, &endptr, 10 );

                if (0
                    || errno
                    || (1
                        && endptr[0] != 'M'
                        && endptr[0] != 'm'
                        && endptr[0] != 'G'
                        && endptr[0] != 'g'
                       )
                    || endptr[1] // (not NULL terminator)
                    || maxsize < 1
                    || maxsize > 999
                )
                {
                    // "Invalid MAX= value: %s"
                    WRMSG( HHC02378, "E", argv[i]+4 );
                    if (traceFILE)
                        VERIFY( 0 == fclose( traceFILE ));
                    return -1;
                }

                if (endptr[0] == 'M' ||
                    endptr[0] == 'm') factor = ONE_MEGABYTE;
                else                  factor = ONE_GIGABYTE;

                maxsize *= factor;
            }
            else
            {
                // "Invalid command usage. Type 'help %s' for assistance."
                WRMSG( HHC02299, "E", argv[0] );
                if (traceFILE)
                    VERIFY( 0 == fclose( traceFILE ));
                return -1;
            }
        }
        // end for()

        if (enable)
        {
            // File is required if they want to enable it
            if (!sysblk.traceFILE && !sysblk.tracefilename && !traceFILE)
            {
                // "Invalid %s= value: %s"
                WRMSG( HHC02380, "E", "FILE", "(null)" );
                return -1;
            }

            // A non-zero maximum filesize is also required to enable
            if (!sysblk.maxtracesize && !maxsize)
            {
                // "Invalid %s= value: %s"
                WRMSG( HHC02380, "E", "MAX", "0" );
                if (traceFILE)
                    VERIFY( 0 == fclose( traceFILE ));
                return -1;
            }
        }
        else
        {
            // They don't want to enable it (yet),
            // or they want to disable it.

            if (!maxsize)
                maxsize = sysblk.maxtracesize;
            if (!maxsize)
                maxsize = ONE_MEGABYTE;
        }

        // Activate/Deactivate (enable/disable) tracefile tracing...
        OBTAIN_TRACEFILE_LOCK();
        {
            if (enable)
            {
                void* tracebuff;

                // Allocate a fresh buffer

                if (!(tracebuff = calloc( 1, tf_MAX_RECSIZE() )))
                {
                    RELEASE_TRACEFILE_LOCK();

                    // "Out of memory"
                    WRMSG( HHC00152, "E" );
                    RELEASE_TRACEFILE_LOCK();

                    if (traceFILE)
                        VERIFY( 0 == fclose( traceFILE ));

                    return -1;
                }

                free( sysblk.tracefilebuff );
                sysblk.tracefilebuff = tracebuff;

                // Switch over to using the new file if specified
                // (or reuse existing one if new one wasn't given)

                if (sysblk.traceFILE)
                    tf_close_locked();

                if (traceFILE)
                {
                    // switch over to using their new tracefile
                    sysblk.traceFILE = traceFILE;
                    free( sysblk.tracefilename );
                    sysblk.tracefilename = strdup( filename );
                }
                else // (continue use existing tracefile)
                {
                    if (!(sysblk.traceFILE = fopen( sysblk.tracefilename, "wb" )))
                    {
                        // "Trace file open error %s: \"%s\""
                        WRMSG( HHC02377, "E", strerror( errno ), sysblk.tracefilename );
                        RELEASE_TRACEFILE_LOCK();
                        return -1;
                    }
                }

                sysblk.curtracesize = 0;
            }
            else // (Deactivate/disable)
            {
                // Close trace file and free buffer,
                // but keep existing filename unless
                // they specified a new one.

                if (traceFILE)
                {
                    // (switch to new file)
                    VERIFY( 0 == fclose( traceFILE ));
                    free( sysblk.tracefilename );
                    sysblk.tracefilename = strdup( filename );
                }

                // (close existing file)
                if (sysblk.traceFILE)
                    tf_close_locked();

                free( sysblk.tracefilebuff );
                sysblk.tracefilebuff = NULL;
                sysblk.traceFILE     = NULL;
                sysblk.curtracesize  = 0;
            }

            sysblk.tfnostop = nostop;

            if (maxsize)
                sysblk.maxtracesize = maxsize;
            else
                maxsize = sysblk.maxtracesize;
        }
        RELEASE_TRACEFILE_LOCK();
    }
    // end if (argc > 1)

    // Display current values...

    if ((maxsize = sysblk.maxtracesize) >= ONE_GIGABYTE)
    {
        mg = 'G';
        maxsize = ROUND_UP( maxsize, ONE_GIGABYTE );
        mgs = (int) (maxsize / ONE_GIGABYTE);
    }
    else
    {
        mg = 'M';
        maxsize = ROUND_UP( maxsize, ONE_MEGABYTE );
        mgs = (int) (maxsize / ONE_MEGABYTE);
    }

    if (sysblk.tracefilename)
    {
        STRLCPY( filename, sysblk.tracefilename );
        if (strchr( filename, ' ' )) quote = "\"";
        else                         quote = "";
    }

    MSGBUF( buf, "%s MAX=%d%c %sSTOP %sFILE=%s%s",
        sysblk.traceFILE ? "ON" : "OFF",
        mgs, mg,
        sysblk.tfnostop ? "NO" : "",
        quote, filename, quote
    );

    if (argc > 1)
        // "%-14s set to %s"
        WRMSG( HHC02204, "I", argv[0], buf );
    else
        // "%-14s: %s"
        WRMSG( HHC02203, "I", argv[0], buf );

    /* Auto-stop tracing unless asked not to */
    if (!enable && !nostop)
    {
        if (tf_autostop())
            // "File closed, tracing %s"
            WRMSG( HHC02381, "I", "auto-stopped" );
    }

    return 0;
}


/*-------------------------------------------------------------------*/
/* tracing commands: x, x+, x-, x?, where 'x' is either t, s or b.   */
/*-------------------------------------------------------------------*/
int trace_cmd( int argc, char* argv[], char* cmdline )
{
    U64   addr[2]         =  {0};       /* Parsed address range      */
    BYTE  c[2]            =  {0};       /* [0]=range sep, [1]=sscanf */
    U16   breakasid       =   0;        /* Optional asid argument    */

    char  rangemsg [128]  =  {0};       /* MSGBUF work buffer        */
    char  asidmsg  [128]  =  {0};       /* MSGBUF work buffer        */

    bool  trace   =  false;             /* Whether command was 't'   */
    bool  step    =  false;             /* Whether command was 's'   */
    bool  breakp  =  false;             /* Whether command was 'b'   */
    bool  on      =  false;             /* Whether + was specified   */
    bool  off     =  false;             /* Whether - was specified   */
    bool  query   =  false;             /* Whether ? was specified   */
    bool  update  =  false;             /* Whether parms were given  */
    bool  unlock  =  false;             /* Should do RELEASE_INTLOCK */

    cmdline[0] = tolower( (unsigned char)cmdline[0] );

    trace  = (cmdline[0] == 't');       // trace command
    step   = (cmdline[0] == 's');       // stepping command
    breakp = (cmdline[0] == 'b');       // breakpoint command

    on     = (cmdline[1] == '+');       // explicit set
    off    = (cmdline[1] == '-');       // explicit unset
    query  = (cmdline[1] == '?');       // explicit query
    update = (argc > 1);                // parameters specified

    /* NOTE: normally, a "trace" type of command by itself (without
       an explicit +, -, or ?) means you only want to query/update
       the current parameters, but you don't want to enable tracing
       or stepping or set a breakpoint. Specifying the 't' or 's'
       command by itself without any parameters is identical to the
       't?' and 's?' query commands.

       The 'b' (breakpoint) command is different however. It's an
       exception to the rule. The b command means that you DO want
       to enable a breakpoint using the parameters specified. Thus
       the b and b+ commands ALWAYS require at least one parameter.

       It also goes without saying that the '-' syntax to disable
       tracing or stepping or to remove a breakpoint (as well as
       the explicit '?' syntax to query the current settings) is
       NOT allowed to have any parameters. Note too that the '-'
       syntax only disables tracing or stepping or breakpoints
       but does NOT clear/reset the current settings (range, etc).

       The '+' syntax to enable tracing or stepping or define (set)
       a breakpoint however, is allowed to also specify parameters.
       If specified, they will be used to update the current values
       before actually activating tracing/stepping or breakpoints.
       If no parameters are specified on the '+' command, then the
       tracing/stepping/breakpoint is activated without modifying
       the current settings (i.e. the current values as previously
       established are used instead).
    */

    /* Plain 'b' command is an implied 'set' */
    if (breakp && !(on || off || query))
        on = true;

    /* Plain 't' or 's' command is an implied 'query' */
    if ((trace || step) && !(on || off || update))
        query = true;

    /* Check for invalid number of arguments */
    if (0
        // No more than 3 arguments allowed (cmd, range, asid)
        || (argc > 3)

        // If explicit - or ? then can't change settings
        || ((off || query) && update)

        // If setting breakpoint, parameters are required
        || (breakp && on && !update)

        // Optional asid parameter not allowed for trace
        || (trace && argc > 2)
    )
    {
        // "Invalid argument(s). Type 'help %s' for assistance."
        WRMSG( HHC02211, "E", argv[0] );
        return -1;
    }

    /* (quick sanity check) */
    ASSERT
    (1
        && (on || off || query || update)
        && !(on && off)
        && !(query && (on || off))
        && !((query || off) && update)
    );

    /* Parse arguments, if specified */
    if (update)
    {
        int rc;

        /* Parse the address range */
        rc = sscanf( argv[1], "%"SCNx64"%c%"SCNx64"%c",
                    &addr[0], &c[0], &addr[1], &c[1] );
        if (rc == 1)
        {
            // (only a single address was specified)
            c[0] = '-';
            addr[1] = addr[0]; // (end-of-range = begin-of-range)
        }
        else if (rc != 3 || (c[0] != '-' && c[0] != ':' && c[0] != '.'))
        {
            // "Invalid argument %s%s"
            WRMSG( HHC02205, "E", argv[1], "" );
            return -1;
        }

        /* Adjust ending address if ".len" format was used */
        if (c[0] == '.')
            addr[1] += addr[0] - 1;

        /* Parse optional ASID, if specified */
        if (argc >= 3)
        {
            char* endptr;   // (strtoul work)
            U32 asid;       // (strtoul result)
            errno = 0;      // (reset to detect if error)

            asid = strtoul( argv[2], &endptr, 16 );

            if (0
                || trace              // (not supported for t command)
                || argv[2][0] == 0    // (no value specified at all)
                || *endptr != 0       // (invalid characters found)
                || ERANGE == errno    // (value too big or too small
                || asid == 0          // (can't be zero)
                || asid > USHRT_MAX   // (16 bit maximum)
            )
            {
                // Invalid argument %s%s"
                WRMSG( HHC02205, "E", argv[2], "" );
                return -1;
            }

            breakasid = (U16) (asid & 0xFFFF);
        }
    }
    else
        c[0] = '-';

    /* Process their request */
    unlock = (TRY_OBTAIN_INTLOCK( NULL ) == 0);
    {
        /* Update and/or enable/disable tracing/stepping */
        if (on || off || update)
        {
            int cpu;
            bool auto_disabled = false;

            /* Explicit tracing overrides automatic tracing */
            if (sysblk.auto_trace_beg || sysblk.auto_trace_amt)
            {
                auto_disabled = true;
                sysblk.auto_trace_beg = 0;
                sysblk.auto_trace_amt = 0;
            }

            if (trace)
            {
                if (update)
                {
                    sysblk.traceaddr[0] = addr[0];
                    sysblk.traceaddr[1] = addr[1];
                    sysblk.breakasid    = 0;
                }

                if (on || off)
                {
                    sysblk.insttrace = on;

                    for (cpu=0; cpu < sysblk.maxcpu; cpu++)
                    {
                        if (IS_CPU_ONLINE( cpu ))
                            sysblk.regs[ cpu ]->insttrace = on;
                    }
                }
            }
            else // (step || breakp)
            {
                if (update)
                {
                    sysblk.breakaddr[0] = addr[0];
                    sysblk.breakaddr[1] = addr[1];
                    sysblk.breakasid    = breakasid;
                }

                if (on || off)
                    sysblk.instbreak = on;
            }

            SET_IC_TRACE;

            if (auto_disabled)
            {
                // "Automatic tracing disabled"
                WRMSG( HHC02373, "I" );
            }
        }

        /* Save (possibly updated) settings for user feedback */
        if (trace)
        {
            addr[0]   = sysblk.traceaddr[0];
            addr[1]   = sysblk.traceaddr[1];
            breakasid = 0;
            on        = sysblk.insttrace;
        }
        else // (step || breakp)
        {
            addr[0]   = sysblk.breakaddr[0];
            addr[1]   = sysblk.breakaddr[1];
            breakasid = sysblk.breakasid;
            on        = sysblk.instbreak;
        }
    }
    if (unlock) RELEASE_INTLOCK( NULL );

    /* Build range and asid message fragments, if appropriate */
    if (addr[0] || addr[1])
    {
        MSGBUF( rangemsg, " range %"PRIx64"%c%"PRIx64,

            addr[0], c[0], ('.' == c[0]) ?
                           (addr[1] - addr[0]) + 1
                         :  addr[1]
        );
    }

    if (breakasid)
        MSGBUF( asidmsg, " asid x'%4.4"PRIx16"'", breakasid );

    /* Display (current or new) settings */

    WRMSG( HHC02229, "I",

        // "Instruction %s %s%s%s"

        trace ? "tracing" : step  ? "stepping" : breakp ? "break" : "(ERR)",
        on ? "on" : "off",
        rangemsg,
        asidmsg
    );

    /* Also show automatic tracing settings if enabled */
    if (sysblk.auto_trace_beg || sysblk.auto_trace_amt)
        panel_command( "-t+-" );

#if defined( _FEATURE_073_TRANSACT_EXEC_FACILITY )
    /* Also show txf tracing settings if enabled */
    if (sysblk.txf_tracing & TXF_TR_INSTR)
        panel_command( "-txf" );
#endif

    if (query)
    {
        unlock = (TRY_OBTAIN_INTLOCK( NULL ) == 0);
        {
            DEVBLK* dev;
            char typ[16] = {0};
            char who[1024] = {0};
            int cpu, on = 0, off = 0;

            /* Also show instruction tracing for each individual CPU,
               but only if not all CPUs are being traced or not (i.e.
               only if some are being traced but not others, or vice-
               versa).
            */
            for (cpu=0; cpu < sysblk.maxcpu; cpu++)
            {
                if (IS_CPU_ONLINE( cpu ))
                {
                    if (sysblk.regs[ cpu ]->insttrace)
                        ++on;
                    else
                        ++off;
                }
            }

            /* If only some (but not all) have instruction tracing enabled
               (or vice versa), then show them which ones have it enabled.
            */
            if (on && off)  // Some on, some off? (i.e. neither is zero?)
            {
                ASSERT( sysblk.insttrace ); // sanity check

                for (cpu=0; cpu < sysblk.maxcpu; cpu++)
                {
                    if (IS_CPU_ONLINE( cpu ))
                    {
                        if (sysblk.regs[ cpu ]->insttrace)
                        {
                            MSGBUF( who, "CPU %s%02X",
                                ptyp2short( sysblk.ptyp[ cpu ] ), cpu );

                            // "%stracing active for %s"
                            WRMSG( HHC02382, "I", "instruction ", who );
                        }
                    }
                }
            }

            /* Also show CCW/ORB tracing if enabled */
            for (dev = sysblk.firstdev; dev != NULL; dev = dev->nextdev)
            {
                typ[0] = 0;

                if (dev->orbtrace   ) STRLCAT( typ, "ORB " );
                if (dev->ccwtrace   ) STRLCAT( typ, "CCW " );
                if (dev->ckdkeytrace) STRLCAT( typ, "CKD " );

                if (typ[0])
                {
                    char for_ccws[1024] = {0};

                    if (dev->ccwopstrace)
                    {
                        int  i;
                        char ccwop[4] = {0};

                        STRLCAT( for_ccws, " for CCWs (" );

                        for (i=0; i < 256; ++i)
                        {
                            if (dev->ccwops[i])
                            {
                                MSGBUF( ccwop, "%2.2x,", i );
                                STRLCAT( for_ccws, ccwop );
                            }
                        }

                        rtrim( for_ccws, "," );
                        STRLCAT( for_ccws, ")" );
                    }

                    MSGBUF( who, "device %1d:%04X%s",
                        SSID_TO_LCSS( dev->ssid ), dev->devnum, for_ccws );

                    // "%stracing active for %s"
                    WRMSG( HHC02382, "I", typ, who );
                }
            }
        }
        if (unlock) RELEASE_INTLOCK( NULL );
    }

    return 0;
}


#if defined( _FEATURE_073_TRANSACT_EXEC_FACILITY )
/*-------------------------------------------------------------------*/
/* txf_cmd - control TXF tracing                                     */
/*-------------------------------------------------------------------*/
int txf_cmd( int argc, char* argv[], char* cmdline )
{
    U32  txf_tracing  = sysblk.txf_tracing;
    U32  txf_why_mask = sysblk.txf_why_mask;
    int  txf_tac      = sysblk.txf_tac;
    int  txf_tnd      = sysblk.txf_tnd;
    int  txf_cpuad    = sysblk.txf_cpuad;
    int  txf_fails    = sysblk.txf_fails;
    int  rc           = 0;
    char c;
    bool stats = false;

    UNREFERENCED( cmdline );

    // txf  [0 | STATS | [INSTR] [U] [C] [GOOD] [BAD] [TDB] [PAGES|LINES]
    //      [WHY hhhhhhhh] [TAC nnn] [TND nn] [CPU nnn] [FAILS nn] ]

    if (argc > 1)  // (define new settings?)
    {
        // Display statistics?
        if (str_caseless_eq( argv[1], "STATS" ))
        {
            if (argc > 2)
                rc = -1;
            else
                stats = true;
        }
        // Disable all TXF tracing?
        else if (str_caseless_eq( argv[1], "0"))
        {
            if (argc > 2)
                rc = -1;
            else
                txf_tracing = 0;
        }
        else // Define new options...
        {
            int i;

            txf_tracing  = 0;
            txf_why_mask = 0;
            txf_tac      = 0;
            txf_tnd      = 0;
            txf_cpuad    = -1;
            txf_fails    = 0;

            for (i=1; i < argc; ++i)
            {
                     if (str_caseless_eq( argv[i], "INSTR"   )) txf_tracing |= TXF_TR_INSTR;
                else if (str_caseless_eq( argv[i], "U"       )) txf_tracing |= TXF_TR_U;
                else if (str_caseless_eq( argv[i], "C"       )) txf_tracing |= TXF_TR_C;
                else if (str_caseless_eq( argv[i], "GOOD"    )) txf_tracing |= TXF_TR_SUCCESS;
                else if (str_caseless_eq( argv[i], "SUCCESS" )) txf_tracing |= TXF_TR_SUCCESS;
                else if (str_caseless_eq( argv[i], "BAD"     )) txf_tracing |= TXF_TR_FAILURE;
                else if (str_caseless_eq( argv[i], "FAILURE" )) txf_tracing |= TXF_TR_FAILURE;
                else if (str_caseless_eq( argv[i], "FAIL"    )) txf_tracing |= TXF_TR_FAILURE;
                else if (str_caseless_eq( argv[i], "TDB"     )) txf_tracing |= TXF_TR_TDB;
                else if (str_caseless_eq( argv[i], "PAGES"   )) txf_tracing |= TXF_TR_PAGES;
                else if (str_caseless_eq( argv[i], "LINES"   )) txf_tracing |= TXF_TR_LINES;
                else if (1
                    && str_caseless_eq(   argv[i+0], "WHY" )
                    &&                    argv[i+1]
                    && sscanf(            argv[i+1], "%"SCNx32"%c", &txf_why_mask, &c ) == 1
                )
                {
                    txf_tracing |= TXF_TR_WHY;
                    ++i;
                }
                else if (1
                    && str_caseless_eq( argv[i+0], "TAC" )
                    &&                  argv[i+1]
                    && (txf_tac = atoi( argv[i+1] )) > 0
                )
                {
                    txf_tracing |= TXF_TR_TAC;
                    ++i;
                }
                else if (1
                    && str_caseless_eq( argv[i+0], "TND" )
                    &&                  argv[i+1]
                    && (txf_tnd = atoi( argv[i+1] )) > 0
                )
                {
                    txf_tracing |= TXF_TR_TND;
                    ++i;
                }
                else if (1
                    && str_caseless_eq(   argv[i+0], "CPU" )
                    &&                    argv[i+1]
                    && (txf_cpuad = atoi( argv[i+1] )) >= 0
                )
                {
                    txf_tracing |= TXF_TR_CPU;
                    ++i;
                }
                else if (1
                    && str_caseless_eq(   argv[i+0], "FAILS" )
                    &&                    argv[i+1]
                    && (txf_fails = atoi( argv[i+1] )) > 0
                )
                {
                    txf_tracing |= TXF_TR_FAILS;
                    ++i;
                }
                else
                {
                    rc = -1;
                    break;
                }
            }

            if (rc == 0)
            {
                //------------------------------------------------
                //   If neither U nor C specified, set both.
                //------------------------------------------------
                if (!(txf_tracing & (TXF_TR_U | TXF_TR_C)))
                {
                    txf_tracing |= (TXF_TR_U | TXF_TR_C);
                }

                //------------------------------------------------
                //  If neither GOOD nor BAD specified, set both.
                //------------------------------------------------
                if (!(txf_tracing & (TXF_TR_SUCCESS | TXF_TR_FAILURE)))
                {
                    txf_tracing |= (TXF_TR_SUCCESS | TXF_TR_FAILURE);
                }

                //------------------------------------------------
                //   If WHY, TAC or FAILS specified, set BAD.
                //------------------------------------------------
                if (txf_tracing & (TXF_TR_WHY | TXF_TR_TAC | TXF_TR_FAILS))
                {
                    txf_tracing |= TXF_TR_FAILURE;
                }

                //------------------------------------------------
                //   If FAILS specified, set C BAD.
                //------------------------------------------------
                if (txf_tracing & TXF_TR_FAILS)
                {
                    txf_tracing |= (TXF_TR_C | TXF_TR_FAILURE);
                }

                //------------------------------------------------
                //      If LINES specified, set PAGES too.
                //------------------------------------------------
                if (txf_tracing & TXF_TR_LINES)
                {
                    txf_tracing |= TXF_TR_PAGES;
                }

                //------------------------------------------------
                //     Ignore TDB unless BAD also specified.
                //------------------------------------------------
                if (!(txf_tracing & TXF_TR_FAILURE))
                {
                    txf_tracing &= ~TXF_TR_TDB;
                }

                //------------------------------------------------
                //     Ignore WHY unless BAD also specified.
                //------------------------------------------------
                if (!(txf_tracing & TXF_TR_FAILURE))
                {
                    txf_tracing &= ~TXF_TR_WHY;
                    txf_why_mask = 0;
                }

                //------------------------------------------------
                //     Ignore TAC unless BAD also specified.
                //------------------------------------------------
                if (!(txf_tracing & TXF_TR_FAILURE))
                {
                    txf_tracing &= ~TXF_TR_TAC;
                    txf_tac = 0;
                }

                //------------------------------------------------
                //     Ignore FAILS unless BAD also specified.
                //------------------------------------------------
                if (!(txf_tracing & TXF_TR_FAILURE))
                {
                    txf_tracing &= ~TXF_TR_FAILS;
                    txf_fails = 0;
                }

                //------------------------------------------------
                //     Ignore FAILS unless C also specified.
                //------------------------------------------------
                if (!(txf_tracing & TXF_TR_C))
                {
                    txf_tracing &= ~TXF_TR_FAILS;
                    txf_fails = 0;
                }
            }
        }

        if (rc == 0)
        {
            sysblk.txf_tracing  = txf_tracing;
            sysblk.txf_why_mask = txf_why_mask;
            sysblk.txf_tac      = txf_tac;
            sysblk.txf_tnd      = txf_tnd;
            sysblk.txf_cpuad    = txf_cpuad;
            sysblk.txf_fails    = txf_fails;
        }
    }

    if (rc != 0)
    {
        // "Invalid command usage. Type 'help %s' for assistance."
        WRMSG( HHC02299, "E", argv[0] );
    }
    else if (!stats) // Display new/current settings
    {
        char buf[1024] = {0};

        // txf  [0 | STATS | [INSTR] [U] [C] [GOOD] [BAD] [TDB] [PAGES|LINES]
        //      [WHY hhhhhhhh] [TAC nnn] [TND nn] [CPU nnn] [FAILS nn] ]

        if (txf_tracing)
        {
            char why[256] = {0}; // WHY hhhhhhhh (xxx xxx xxx ... )
            char tac[64]  = {0};
            char tnd[32]  = {0};
            char cpu[32]  = {0};
            char cfl[32]  = {0};

            if (txf_why_mask)    MSGBUF( why, "WHY 0x%8.8"PRIX32" ",  txf_why_mask );
            if (txf_tac    >  0) MSGBUF( tac, "TAC %d ",              txf_tac      );
            if (txf_tnd    >  0) MSGBUF( tnd, "TND %d ",              txf_tnd      );
            if (txf_cpuad  >= 0) MSGBUF( cpu, "CPU %d ",              txf_cpuad    );
            if (txf_fails  >  0) MSGBUF( cfl, "FAILS %d ",            txf_fails    );

            MSGBUF( buf, "%s%s%s%s%s%s%s%s" "%s%s%s%s%s"

                , txf_tracing & TXF_TR_INSTR   ? "INSTR " : ""
                , txf_tracing & TXF_TR_U       ? "U "     : ""
                , txf_tracing & TXF_TR_C       ? "C "     : ""
                , txf_tracing & TXF_TR_SUCCESS ? "GOOD "  : ""
                , txf_tracing & TXF_TR_FAILURE ? "BAD "   : ""
                , txf_tracing & TXF_TR_TDB     ? "TDB "   : ""
                , txf_tracing & TXF_TR_PAGES   ? "PAGES " : ""
                , txf_tracing & TXF_TR_LINES   ? "LINES " : ""

                , why
                , tac
                , tnd
                , cpu
                , cfl
            );
            RTRIM( buf );
        }

        UPPER_ARGV_0( argv );

        if (argc > 1)  // Defined new options...
        {
            // "%-14s set to %s"
            WRMSG( HHC02204, "I", argv[0], buf );
        }
        else // Display current settings...
        {
            // "%-14s: %s"
            WRMSG( HHC02203, "I", argv[0], buf );
        }
    }
    else // (stats)
    {
        int  contran;
        for (contran=0; contran <= 1; contran++)
        {
            if (sysblk.txf_stats[ contran ].txf_trans)
            {
                double total, count;
                int i;

                // "Total %s Transactions =%12"PRIu64
                WRMSG( HHC17730, "I", TXF_CONSTRAINED( contran ),
                        sysblk.txf_stats[ contran ].txf_trans );
                total = sysblk.txf_stats[ contran ].txf_trans;

                // "  Retries for ANY/ALL reason(s):"
                WRMSG( HHC17731, "I" );

                /* Print buckets up until just BEFORE the last bucket */
                for (i=0; i < (TXF_STATS_RETRY_SLOTS-1); i++)
                {
                    count = sysblk.txf_stats[ contran ].txf_retries[ i ];

                    // "    %1d%cretries =%12"PRIu64"  (%4.1f%%)"
                    WRMSG( HHC17732, "I",
                        i, ' ',
                        sysblk.txf_stats[ contran ].txf_retries[ i ],
                        (count/total) * 100.0 );
                }

                /* Now print the LAST bucket */
                count = sysblk.txf_stats[ contran ].txf_retries[ i ];

                // "    %1d%cretries =%12"PRIu64"  (%4.1f%%)"
                WRMSG( HHC17732, "I",
                    i, '+',
                    sysblk.txf_stats[ contran ].txf_retries[ i ],
                    (count/total) * 100.0 );

                // "    MAXIMUM   =%12"PRIu64
                WRMSG( HHC17733, "I", sysblk.txf_stats[ contran ].txf_retries_hwm );

                /* Report how often a transaction was aborted by TAC */
                for (i = 2; i < TXF_STATS_TAC_SLOTS; i++)
                {
                    /* (TAC 3 == undefined/unassigned; skip) */
                    if (i == 3)
                    {
                        // (sanity check: total for this slot should be zero)
                        ASSERT( 0 == sysblk.txf_stats[ contran ].txf_aborts_by_tac[ i ] );
                        continue; // (skip TAC slot 3 == unassigned)
                    }

                    // "  %12"PRIu64"  (%4.1f%%)  Retries due to TAC %3d %s"
                    count =               sysblk.txf_stats[ contran ].txf_aborts_by_tac[ i ];
                    WRMSG( HHC17734, "I", sysblk.txf_stats[ contran ].txf_aborts_by_tac[ i ],
                        (count/total) * 100.0, i, tac2long( i ) );
                }

                // "  %12"PRIu64"  (%4.1f%%)  Retries due to TAC %3d %s"
                count =               sysblk.txf_stats[ contran ].txf_aborts_by_tac_misc;
                WRMSG( HHC17734, "I", sysblk.txf_stats[ contran ].txf_aborts_by_tac_misc,
                    (count/total) * 100.0, TAC_MISC, tac2long( TAC_MISC ) );

                // "  %12"PRIu64"  (%4.1f%%)  Retries due to other TAC"
                count =               sysblk.txf_stats[ contran ].txf_aborts_by_tac[ 0 ];
                WRMSG( HHC17735, "I", sysblk.txf_stats[ contran ].txf_aborts_by_tac[ 0 ],
                    (count/total) * 100.0 );
            }
        }
    }

    return rc;
}
#endif /* defined( _FEATURE_073_TRANSACT_EXEC_FACILITY ) */


/*-------------------------------------------------------------------*/
/* automatic tracing command:  "t+- [ BEG=<instrcount>  AMT=num ]"   */
/*-------------------------------------------------------------------*/
int auto_trace_cmd( int argc, char* argv[], char* cmdline )
{
    U64  auto_trace_beg;        // (instrcount to begin tracing)
    U64  auto_trace_amt;        // (amt of instruction to trace)

    UNREFERENCED( cmdline );

    if (argc > 1)
    {
        char c;

        /* Parse/validate arguments */
        if (0
            || (argc != 3)

            || (strncasecmp( "BEG=", argv[1], 4 ) != 0)
            || (strncasecmp( "AMT=", argv[2], 4 ) != 0)

            || (sscanf( argv[1] + 4, "%"SCNu64"%c", &auto_trace_beg, &c ) != 1)
            || (sscanf( argv[2] + 4, "%"SCNu64"%c", &auto_trace_amt, &c ) != 1)
        )
        {
            // "Invalid command usage. Type 'help %s' for assistance."
            WRMSG( HHC02299, "E", argv[0] );
            return -1;
        }

        if (!auto_trace_beg || !auto_trace_amt)
        {
            // "Automatic tracing value(s) must be greater than zero"
            WRMSG( HHC02376, "E" );
            return -1;
        }

        /* Enable automatic tracing */
        OBTAIN_INTLOCK( NULL );

        sysblk.auto_trace_beg = auto_trace_beg;
        sysblk.auto_trace_amt = auto_trace_amt;
    }
    else // (argc <= 1)
    {
        /* Retrieve current settings */
        OBTAIN_INTLOCK( NULL );

        auto_trace_beg = sysblk.auto_trace_beg;
        auto_trace_amt = sysblk.auto_trace_amt;
    }

    RELEASE_INTLOCK( NULL );

    /* Display current settings */
    if (!auto_trace_beg && !auto_trace_amt)
    {
        // "Automatic tracing not enabled"
        WRMSG( HHC02372, "I" );
    }
    else if (!auto_trace_beg)
    {
        // "Automatic tracing is active"
        WRMSG( HHC02375, "I" );
    }
    else // (auto_trace_beg && auto_trace_amt)
    {
        // "Automatic tracing enabled: BEG=%"PRIu64" AMT=%"PRIu64
        WRMSG( HHC02374, "I", auto_trace_beg, auto_trace_amt );
    }

    return 0;
}

#if defined( SUPPRESS_128BIT_PRINTF_FORMAT_WARNING )
PUSH_GCC_WARNINGS()
DISABLE_GCC_WARNING( "-Wformat" )
#endif

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

// /*DEBUG*/logmsg( "hsccmd.c: %s%02X: Any cpu interrupt %spending\n",
// /*DEBUG*/    PTYPSTR(sysblk.regs[i]->cpuad), sysblk.regs[i]->cpuad,
// /*DEBUG*/    sysblk.regs[i]->cpuint ? "" : "not " );
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
        if (ARCH_370_IDX == sysblk.arch_mode)
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
        if (ARCH_900_IDX == sysblk.arch_mode)
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
                            GUEST( sysblk.regs[i] )->ints_state,
                            GUEST( sysblk.regs[i] )->ints_mask);
            WRMSG( HHC00851, "I", "IE", sysblk.regs[i]->cpuad,
                            IS_IC_INTERRUPT( GUEST( sysblk.regs[i] )) ? "" : "not ");
            WRMSG( HHC00852, "I", "IE", sysblk.regs[i]->cpuad, IS_IC_IOPENDING ? "" : "not ");
            WRMSG( HHC00853, "I", "IE", sysblk.regs[i]->cpuad, IS_IC_CLKC(       GUEST( sysblk.regs[i] )) ? "" : "not ");
            WRMSG( HHC00854, "I", "IE", sysblk.regs[i]->cpuad, IS_IC_PTIMER(     GUEST( sysblk.regs[i] )) ? "" : "not ");
            WRMSG( HHC00855, "I", "IE", sysblk.regs[i]->cpuad, IS_IC_ITIMER(     GUEST( sysblk.regs[i] )) ? "" : "not ");
            WRMSG( HHC00857, "I", "IE", sysblk.regs[i]->cpuad, IS_IC_EXTCALL(    GUEST( sysblk.regs[i] )) ? "" : "not ");
            WRMSG( HHC00858, "I", "IE", sysblk.regs[i]->cpuad, IS_IC_EMERSIG(    GUEST( sysblk.regs[i] )) ? "" : "not ");
            WRMSG( HHC00859, "I", "IE", sysblk.regs[i]->cpuad, IS_IC_MCKPENDING( GUEST( sysblk.regs[i] )) ? "" : "not ");
            WRMSG( HHC00860, "I", "IE", sysblk.regs[i]->cpuad, IS_IC_SERVSIG ? "" : "not ");
            WRMSG( HHC00864, "I", "IE", sysblk.regs[i]->cpuad, test_lock(&sysblk.cpulock[i]) ? "" : "not ");

            if (ARCH_370_IDX == sysblk.arch_mode)
            {
                if (0xFFFF == GUEST( sysblk.regs[i] )->chanset)
                {
                    MSGBUF( buf, "none");
                }
                else
                {
                    MSGBUF( buf, "%4.4X", GUEST( sysblk.regs[i] )->chanset);
                }
                WRMSG( HHC00865, "I", "IE", sysblk.regs[i]->cpuad, buf );
            }
            WRMSG( HHC00866, "I", "IE", sysblk.regs[i]->cpuad, states[ GUEST( sysblk.regs[i] )->cpustate ]);
            WRMSG( HHC00867, "I", "IE", sysblk.regs[i]->cpuad, (S64)GUEST( sysblk.regs[i] )->instcount);
            WRMSG( HHC00868, "I", "IE", sysblk.regs[i]->cpuad, GUEST( sysblk.regs[i] )->siototal);
            copy_psw( GUEST( sysblk.regs[i] ), curpsw );
            if (ARCH_900_IDX == sysblk.arch_mode)
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

    // "config mask "F_CPU_BITMAP" started mask "F_CPU_BITMAP" waiting mask "F_CPU_BITMAP
    WRMSG( HHC00870, "I", sysblk.config_mask, sysblk.started_mask, sysblk.waiting_mask );

    // "syncbc mask "F_CPU_BITMAP" %s"
    WRMSG( HHC00871, "I", sysblk.sync_mask, sysblk.syncing ? "sync in progress" : "" );

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
            WRMSG(HHC00880, "I", LCSS_DEVNUM, buf);
        }
        if (dev->reserved)
        {
            MSGBUF(buf, "reserved %s", sysid);
            WRMSG(HHC00880, "I", LCSS_DEVNUM, buf);
        }
        if (dev->scsw.flag3 & SCSW3_AC_SUSP)
        {
            WRMSG(HHC00880, "I", LCSS_DEVNUM, "suspended" );
        }
        if ((dev->scsw.flag3 & SCSW3_SC_PEND) && (dev->pmcw.flag5 & PMCW5_V))
        {
            WRMSG(HHC00880, "I", LCSS_DEVNUM, "I/O pending" );
        }
        if ((dev->pciscsw.flag3 & SCSW3_SC_PEND) && (dev->pmcw.flag5 & PMCW5_V))
        {
            WRMSG(HHC00880, "I", LCSS_DEVNUM, "PCI pending" );
        }
        if ((dev->attnscsw.flag3 & SCSW3_SC_PEND) && (dev->pmcw.flag5 & PMCW5_V))
        {
            WRMSG(HHC00880, "I", LCSS_DEVNUM, "Attn pending" );
        }
        if (test_lock(&dev->lock) && (dev->pmcw.flag5 & PMCW5_V))
        {
            WRMSG(HHC00880, "I", LCSS_DEVNUM, "lock held" );
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

#if defined( SUPPRESS_128BIT_PRINTF_FORMAT_WARNING )
POP_GCC_WARNINGS()
#endif


/*-------------------------------------------------------------------*/
/* bear command - display or alter BEAR register                     */
/*-------------------------------------------------------------------*/
int bear_cmd( int argc, char* argv[], char* cmdline )
{
    UNREFERENCED( cmdline );

    UPPER_ARGV_0( argv );

    /* Correct number of arguments? */

    if (argc < 1 || argc > 2)
    {
        // "Invalid command usage. Type 'help %s' for assistance."
        WRMSG( HHC02299, "E", argv[0] );
        return -1;
    }

    obtain_lock( &sysblk.cpulock[ sysblk.pcpu ]);
    {
        REGS* regs = sysblk.regs[ sysblk.pcpu ];
        char cbear[17] = {0};

        if (!IS_CPU_ONLINE( sysblk.pcpu ))
        {
            release_lock( &sysblk.cpulock[ sysblk.pcpu ]);
            // "Processor %s%02X: processor is not %s"
            WRMSG( HHC00816, "E", PTYPSTR( sysblk.pcpu ), sysblk.pcpu, "online");
            return -1;
        }

        if (argc > 1)       // (set new value)
        {
            U64 bear;
            BYTE c;

            if (sscanf( argv[1], "%"SCNx64"%c", &bear, &c) != 1)
            {
                release_lock( &sysblk.cpulock[ sysblk.pcpu ]);
                // "Invalid argument %s%s"
                WRMSG( HHC02205, "E", argv[1], ": invalid address" );
                return -1;
            }

            regs->bear = bear;

            // "%-14s set to %s"
            MSGBUF( cbear, "%"PRIx64, regs->bear );
            WRMSG( HHC02204, "I", argv[0], cbear );
        }
        else // (display current value)
        {
            // "%-14s: %s"
            MSGBUF( cbear, "%"PRIx64, regs->bear );
            WRMSG( HHC02203, "I", argv[0], cbear );
        }
    }
    release_lock( &sysblk.cpulock[ sysblk.pcpu ]);
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

        if ( ARCH_900_IDX == regs->arch_mode )
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
        if ( ARCH_900_IDX == regs->arch_mode )
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
int i_cmd( int argc, char* argv[], char* cmdline )
{
    REGS*    regs;
    DEVBLK*  dev;
    int      rc = 0;
    U16      lcss, devnum;

    UNREFERENCED( cmdline );

    if (argc < 2)
    {
        missing_devnum();
        return -1;
    }

    rc = parse_single_devnum( argv[1], &lcss, &devnum );

    if (rc < 0)
        return -1;  // (error message already issued)

    if (!(dev = find_device_by_devnum( lcss, devnum )))
    {
        devnotfound_msg( lcss, devnum );
        return -1;
    }

    rc = device_attention( dev, CSW_ATTN );

    switch (rc)
    {
        // "%1d:%04X attention request raised"
        case 0: WRMSG( HHC02230, "I", lcss, devnum ); break;

        // "%1d:%04X busy or interrupt pending"
        case 1: WRMSG( HHC02231, "E", lcss, devnum ); break;

        // "%1d:%04X attention request rejected"
        case 2: WRMSG( HHC02232, "E", lcss, devnum ); break;

        // "%1d:%04X subchannel not enabled"
        case 3: WRMSG( HHC02233, "E", lcss, devnum ); break;
    }

    regs = sysblk.regs[ sysblk.pcpu ];

    if (1
        && rc == 3 /* subchannel is not valid or not enabled */
        && IS_CPU_ONLINE( sysblk.pcpu )
        && CPUSTATE_STOPPED == regs->cpustate
    )
    {
        // "Are you sure you didn't mean 'ipl %04X'"
        WRMSG( HHC02234, "W", devnum );
    }

    return rc;
}


#if defined( OPTION_INSTR_COUNT_AND_TIME )
/*-------------------------------------------------------------------*/
/* Definition of opcode execution count entries                      */
/*-------------------------------------------------------------------*/
typedef struct {
    unsigned char opcode1; // Operation code, first byte
    unsigned char opcode2; // Operation code, second byte
    U8 opc2pos;            // Opcode2 position in Instr
    U64 count;             // Execution count from sysblk.imaps.imap??
    U64 time;              // Execution time
} ICOUNT_INSTR;

/*-------------------------------------------------------------------*/
/* icount command sort callback (Descending by exec count)           */
/*-------------------------------------------------------------------*/
static int icount_cmd_sort(const ICOUNT_INSTR *x, const ICOUNT_INSTR *y)
{
    return (x->count < y->count) ? +1 : -1;
}

/*-------------------------------------------------------------------*/
/* icount command - display instruction counts                       */
/*-------------------------------------------------------------------*/
int icount_cmd( int argc, char* argv[], char* cmdline )
{
    int i, i1, i2;
    REGS *regs;
    BYTE fakeinst[6];

#define  MAX_ICOUNT_INSTR   1000    /* Maximum number of instructions
                                     in architecture instruction set */
    U64  total;

    ICOUNT_INSTR icount[MAX_ICOUNT_INSTR];

    char buf[ 192 ];
    char instText[ 7 ];

    regs = sysblk.regs[sysblk.pcpu];

    UNREFERENCED( cmdline );

    UPPER_ARGV_0( argv );

    if (argc > 1)
    {
        if (argc > 2)
        {
            // "Invalid argument(s). Type 'help %s' for assistance."
            WRMSG( HHC02211, "E", argv[0] );
            return -1;
        }
        if (0
            || CMD( argv[1], CLEAR, 1 )
            || CMD( argv[1], RESET, 1 )
            || CMD( argv[1], ZERO,  1 )
        )
        {
            memset( &sysblk.imaps, 0, sizeof sysblk.imaps );
            // "%-14s set to %s"
            WRMSG( HHC02204, "I", argv[0], "ZERO" );
            return 0;
        }
        if (0
            || CMD( argv[1], ENABLE, 1 )
            || CMD( argv[1], START,  3 )
        )
        {
            sysblk.icount = true;
            // "%-14s set to %s"
            WRMSG( HHC02204, "I", argv[0], "ENABLE" );
            return 0;
        }
        if (0
            || CMD( argv[1], DISABLE, 1 )
            || CMD( argv[1], STOP,    3 )
        )
        {
            sysblk.icount = false;
            // "%-14s set to %s"
            WRMSG( HHC02204, "I", argv[0], "DISABLE" );
            return 0;
        }
        // "Invalid argument %s%s"
        WRMSG( HHC02205, "E", argv[1], "" );
        return -1;
    }

    /* (collect...) */

    i = 0;
    total = 0;

    for (i1 = 0; i1 < 256; i1++)
    {
        switch (i1)
        {
#define ICOUNT_COLLECT_CASE( _case, _map, _mapT, _nn, _pos )        \
                                                                    \
            case _case:                                             \
            {                                                       \
                for (i2=0; i2 < _nn; i2++)                          \
                {                                                   \
                    if (sysblk.imaps._map[ i2 ])                    \
                    {                                               \
                        icount[i].opcode1 = i1;                     \
                        icount[i].opcode2 = i2;                     \
                        icount[i].opc2pos = _pos;                   \
                        icount[i].time = sysblk.imaps._mapT[ i2 ];  \
                        icount[i++].count = sysblk.imaps._map[ i2 ];\
                        total += sysblk.imaps._map[ i2 ];           \
                                                                    \
                        if (i == (MAX_ICOUNT_INSTR - 1))            \
                        {                                           \
                            /* "Too many instructions! (Sorry!)" */ \
                            WRMSG( HHC02252, "E" );                 \
                            return -1;                              \
                        }                                           \
                    }                                               \
                }                                                   \
                break;                                              \
            }

            ICOUNT_COLLECT_CASE( 0x01, imap01, imap01T, 256, 1 )
            ICOUNT_COLLECT_CASE( 0xA4, imapa4, imapa4T, 256, 1 )
            ICOUNT_COLLECT_CASE( 0xA5, imapa5, imapa5T,  16, 1 )
            ICOUNT_COLLECT_CASE( 0xA6, imapa6, imapa6T, 256, 1 )
            ICOUNT_COLLECT_CASE( 0xA7, imapa7, imapa7T,  16, 1 )
            ICOUNT_COLLECT_CASE( 0xB2, imapb2, imapb2T, 256, 1 )
            ICOUNT_COLLECT_CASE( 0xB3, imapb3, imapb3T, 256, 1 )
            ICOUNT_COLLECT_CASE( 0xB9, imapb9, imapb9T, 256, 1 )
            ICOUNT_COLLECT_CASE( 0xC0, imapc0, imapc0T,  16, 1 )
            ICOUNT_COLLECT_CASE( 0xC2, imapc2, imapc2T,  16, 1 )
            ICOUNT_COLLECT_CASE( 0xC4, imapc4, imapc4T,  16, 1 )
            ICOUNT_COLLECT_CASE( 0xC6, imapc6, imapc6T,  16, 1 )
            ICOUNT_COLLECT_CASE( 0xC8, imapc8, imapc8T,  16, 1 )
            ICOUNT_COLLECT_CASE( 0xE3, imape3, imape3T, 256, 5 )
            ICOUNT_COLLECT_CASE( 0xE4, imape4, imape4T, 256, 1 )
            ICOUNT_COLLECT_CASE( 0xE5, imape5, imape5T, 256, 1 )
            ICOUNT_COLLECT_CASE( 0xE7, imape7, imape7T, 256, 5 )
            ICOUNT_COLLECT_CASE( 0xEB, imapeb, imapebT, 256, 5 )
            ICOUNT_COLLECT_CASE( 0xEC, imapec, imapecT, 256, 5 )
            ICOUNT_COLLECT_CASE( 0xED, imaped, imapedT, 256, 5 )

            default:
            {

                if (sysblk.imaps.imapxx[ i1 ])
                {
                    icount[i].opcode1 = i1;
                    icount[i].opcode2 = 0;
                    icount[i].opc2pos = 0;
                    icount[i].time = sysblk.imaps.imapxxT[ i1 ];
                    icount[i++].count = sysblk.imaps.imapxx[i1];

                    total += sysblk.imaps.imapxx[ i1 ];

                    if (i == (MAX_ICOUNT_INSTR - 1))
                    {
                        // "Too many instructions! (Sorry!)"
                        WRMSG( HHC02252, "E" );
                        return -1;
                    }
                }
                break;
            }
        }
        /* end switch() */
    }
    /* end for() */

    /* (sort...) */
    qsort(icount, i, sizeof(ICOUNT_INSTR), icount_cmd_sort);

#define  ICOUNT_WIDTH  "12"     /* Print field width */

    /* (print...) */

    // "%s"
    WRMSG( HHC02292, "I", "Sorted icount display:" );

    for (i1 = 0; i1 < i; i1++)
    {
        memset(fakeinst, 0, sizeof(fakeinst));
        int bufl = 0;

        switch (icount[i1].opcode1)
        {
            case 0x01:
            case 0xA4:
            case 0xA5:
            case 0xA6:
            case 0xA7:
            case 0xB2:
            case 0xB3:
            case 0xB9:
            case 0xC0:
            case 0xC2:
            case 0xC4:
            case 0xC6:
            case 0xC8:
            case 0xE3:
            case 0xE4:
            case 0xE5:
            case 0xE7:
            case 0xEB:
            case 0xEC:
            case 0xED:
            {
                MSGBUF( instText, "'%2.2X%2.2X'", icount[i1].opcode1, icount[i1].opcode2 );
                fakeinst[icount[i1].opc2pos] = icount[i1].opcode2;
                break;
            }
            default:
            {
                MSGBUF( instText, "'%2.2X'  ", icount[i1].opcode1 );
                break;
            }
        }

        bufl = MSGBUF
        (
            buf,
            "Inst %s count %" ICOUNT_WIDTH PRIu64 " (%2d%%) time %" ICOUNT_WIDTH PRIu64 " (%10f) ",
            instText,
            icount[i1].count,
            (int)(icount[i1].count * 100 / total),
            icount[i1].time,
            ((float)icount[i1].time / icount[i1].count)
        );

        fakeinst[0] = icount[i1].opcode1;
        bufl += PRINT_INST(regs->arch_mode, fakeinst, buf + bufl);
        WRMSG( HHC02292, "I", buf );
    }
    /* end for() */

    return 0;
}
#endif /* defined( OPTION_INSTR_COUNT_AND_TIME ) */


/*-------------------------------------------------------------------*/
/* createCpuId  -  Create the requested CPU ID                       */
/*-------------------------------------------------------------------*/
U64 createCpuId
(
    const U64  model,
    const U64  version,
    const U64  serial,
    const U64  MCEL
)
{
    U64 result;

    result    =  version;

    result  <<=  24;
    result   |=  serial;

    result  <<=  16;
    result   |=  model;

    result  <<=  16;
    result   |=  MCEL;

    return result;
}


/*-------------------------------------------------------------------*/
/* setCpuIdregs  -  Set the CPU ID for the requested CPU context     */
/*-------------------------------------------------------------------*/
void setCpuIdregs
(
    REGS*  regs,
    S32    arg_model,
    S16    arg_version,
    S32    arg_serial,
    S32    arg_MCEL,
    bool   force
)
{
    U16  model;
    U8   version;
    U32  serial;
    U16  MCEL;

    /* Return if CPU out-of-range */
    if (regs->cpuad >= MAX_CPU_ENGS)
        return;

    /* Gather needed values */
    model   = arg_model   >= 0 ?       arg_model   & 0x000FFFF : sysblk.cpumodel;
    version = arg_version >= 0 ?       arg_version & 0xFF      : sysblk.cpuversion;
    serial  = arg_serial  >= 0 ? (U32) arg_serial              : sysblk.cpuserial;
    MCEL    = arg_MCEL    >= 0 ? (U32) arg_MCEL                : sysblk.cpuid;

    /* Version is always zero in z/Architecture mode */
    if (!force && regs->arch_mode == ARCH_900_IDX)
        version = 0;

    /* Register new CPU ID settings */
    regs->cpumodel   = model;
    regs->cpuversion = version;
    regs->cpuserial  = serial;

    if (ARCH_370_IDX != sysblk.arch_mode)
    {
        /* Handle LPAR formatting */
        if (sysblk.lparmode)
        {
            /* Overlay CPUID serial nibbles 0 and 1 with LPAR or LPAR/CPU.
             * The full serial number is maintained in STSI information.
             */
            serial &= 0x0000FFFF;

            if (sysblk.cpuidfmt)  /* Format 1 CPU ID? */
            {
                /* Set Format 1 bit (bit 48 or MCEL bit 0) */
                MCEL = 0x8000;

                /* Use LPAR number to a maximum of 255 */
                serial |= min( sysblk.lparnum, 255 ) << 16;
            }
            else /* Format 0 CPU ID */
            {
                /* Clear MCEL and leave Format 1 bit as zero */
                MCEL = 0;

                /* Use low-order nibble of LPAR id;
                 * LPARNUM 10 is indicated as a value of 0.
                 */
                serial |= (sysblk.lparnum & 0x0F) << 16;

                /* and a single digit CPU ID to a maximum of 15 */
                serial |= min( regs->cpuad, 15 ) << 20;
            }
        }
        else /* BASIC mode CPU ID */
        {
            /* Format is always stored as zero in BASIC mode */
            MCEL &= 0x7FFF;

            /* Use a single digit CPU ID to a maximum of 15 */
            serial &= 0x000FFFFF;
            serial |= min( regs->cpuad, 15 ) << 20;
        }
    }

    /* Construct new CPU ID */
    regs->cpuid = createCpuId( model, version, serial, MCEL );
}


/*-------------------------------------------------------------------*/
/* setCpuId  -  Set the CPU ID for the requested CPU                 */
/*-------------------------------------------------------------------*/
void setCpuId
(
    const unsigned int  cpu,
    S32                 arg_model,
    S16                 arg_version,
    S32                 arg_serial,
    S32                 arg_MCEL,
    bool                force
)
{
    REGS*  regs;

    /* Return if CPU out-of-range */
    if (cpu >= MAX_CPU_ENGS)
        return;

    /* Return if CPU undefined */
    if (!IS_CPU_ONLINE( cpu ))
        return;

    regs = sysblk.regs[ cpu ];

    /* Set new CPU ID */
    setCpuIdregs( regs, arg_model, arg_version, arg_serial, arg_MCEL, force );
}


/*-------------------------------------------------------------------*/
/* setOperationMode  -  Set the operation mode for the system        */
/*-------------------------------------------------------------------*/
/*                                                                   */
/* Operations Mode:                                                  */
/*                                                                   */
/*   BASIC: lparmode = 0                                             */
/*   MIF:   lparmode = 1; cpuidfmt = 0; lparnum = 1...16 (decimal)   */
/*   EMIF:  lparmode = 1; cpuidfmt = 1                               */
/*                                                                   */
/*-------------------------------------------------------------------*/
void setOperationMode()
{
    sysblk.operation_mode =
      sysblk.lparmode ?
      ((sysblk.cpuidfmt || sysblk.lparnum < 1 || sysblk.lparnum > 16) ?
       om_emif : om_mif) : om_basic;
}


/*-------------------------------------------------------------------*/
/* Set/update all CPU IDs                                            */
/*-------------------------------------------------------------------*/
BYTE setAllCpuIds( const S32 model, const S16 version, const S32 serial, const S32 MCEL, bool force )
{
    U64  mcel;
    int  cpu;

    /* Determine and update system CPU model */
    if (model >= 0)
        sysblk.cpumodel = model & 0x0000FFFF;

    /* Determine and update CPU version */
    if (version >= 0)
        sysblk.cpuversion = version & 0xFF;

    /* Determine and update CPU serial number */
    if (serial >= 0)
        sysblk.cpuserial = serial & 0x00FFFFFF;

    /* Determine and update MCEL */
         if (sysblk.lparmode)                   mcel = sysblk.cpuidfmt ? 0x8000 : 0;
    else if (MCEL >= 0)                         mcel = MCEL & 0xFFFF;
    else if ((sysblk.cpuid & 0xFFFF) == 0x8000) mcel = 0;
    else                                        mcel = sysblk.cpuid & 0xFFFF;

    /* Set the system global CPU ID */
    sysblk.cpuid = createCpuId( sysblk.cpumodel, sysblk.cpuversion, sysblk.cpuserial, mcel );

    /* Set a tailored CPU ID for each and every defined CPU */
    for (cpu=0; cpu < MAX_CPU_ENGS; ++cpu )
        setCpuId( cpu, model, version, serial, MCEL, force );

   return TRUE;
}


/*-------------------------------------------------------------------*/
/* setAllCpuIds_lock  -  Obtain INTLOCK and then set all CPU IDs     */
/*-------------------------------------------------------------------*/
BYTE setAllCpuIds_lock( const S32 model, const S16 version, const S32 serial, const S32 MCEL, bool force )
{
    BYTE success;
    OBTAIN_INTLOCK( NULL );
    {
        /* Call unlocked version of setAllCpuIds */
        success = setAllCpuIds( model, version, serial, MCEL, force );
    }
    RELEASE_INTLOCK( NULL );
    return success;
}


/*-------------------------------------------------------------------*/
/* resetAllCpuIds - Reset all CPU IDs based on sysblk CPU ID updates */
/*-------------------------------------------------------------------*/
BYTE resetAllCpuIds()
{
    return setAllCpuIds( -1, -1, -1, -1, true );
}


/*-------------------------------------------------------------------*/
/* enable_lparmode  -  Enable/Disable LPAR mode                      */
/*-------------------------------------------------------------------*/
void enable_lparmode( const bool enable )
{
    static const int  fbyte  =          (STFL_HERC_LOGICAL_PARTITION / 8);
    static const int  fbit   =  0x80 >> (STFL_HERC_LOGICAL_PARTITION % 8);

    if (enable)
    {
#if defined( _370 )
        sysblk.facility_list[ ARCH_370_IDX ][fbyte] |= fbit;
#endif
#if defined( _390 )
        sysblk.facility_list[ ARCH_390_IDX ][fbyte] |= fbit;
#endif
#if defined( _900 )
        sysblk.facility_list[ ARCH_900_IDX ][fbyte] |= fbit;
#endif
    } else { // disable
#if defined( _370 )
        sysblk.facility_list[ ARCH_370_IDX ][fbyte] &= ~fbit;
#endif
#if defined( _390 )
        sysblk.facility_list[ ARCH_390_IDX ][fbyte] &= ~fbit;
#endif
#if defined( _900 )
        sysblk.facility_list[ ARCH_900_IDX ][fbyte] &= ~fbit;
#endif
    }

    /* Set system lparmode and operation mode indicators accordingly */
    sysblk.lparmode = enable;
    setOperationMode();
}

#endif // !defined(_GEN_ARCH)
