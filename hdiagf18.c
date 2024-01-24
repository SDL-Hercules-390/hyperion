/* HDIAGF18.C   (C) Copyright Harold Grovesteen, 2010-2012            */
/*              Hercules DIAGNOSE code X'F18'                         */
/*                                                                    */
/*   Released under "The Q Public License Version 1"                  */
/*   (http://www.hercules-390.org/herclic.html) as modifications to   */
/*   Hercules.                                                        */

/*--------------------------------------------------------------------*/
/* This module implements Hercules DIAGNOSE code X'F18'               */
/* described in README.HRAF and supporting documentation              */
/*--------------------------------------------------------------------*/


/* The following structure exists between the different functions     */
/* AD == ARCH_DEP                                                     */
/*                                                                    */
/* From diagnose.c                                                    */
/*                                                                    */
/* AD:diagf18_call                                                    */
/*    |                                                               */
/*   QUERY:                                                           */
/*    +                                                               */
/*    |                                                               */
/*   CSOCKET:                                                         */
/*    +                                                               */
/*    |                                                               */
/*   CFILE:                                                           */
/*    +---> diagfi8_FC in dyn76.c                                     */
/*    |                                                               */
/*   NSOCKET:                                                         */
/*    +--->                                                           */
/*    |                                                               */
/*   NFILE:                                                           */
/*    +--->                                                           */
/*                                                                    */
/*  Function         ARCH_DEP   On CPU   On Async    Owns             */
/*                              thread    thread     device           */
/*                                                                    */
/*  diagf18_call        Yes      Yes        No         No             */


#include "hstdinc.h"

#define _HDIAGF18_C_
#define _HENGINE_DLL_

#include "hercules.h"
#include "opcode.h"
#include "inline.h"
#include "hdiagf18.h"

#if defined(_FEATURE_HOST_RESOURCE_ACCESS_FACILITY)

#if !defined(_HDIAGF18_C)
#define _HDIAGF18_C

#if !defined(_HDIAGF18_H)
#define _HDIAGF18_H
/*-------------------------------------------------------------------*/
/* Internal macro definitions                                        */
/*-------------------------------------------------------------------*/
/* Function Subcodes                                                 */
#define QUERY   0x00000000   /* Query Operation */
//#define CSOCKET 0x00000001 /* Socket Function Compatibility Mode */
#define CFILE   0x00000002   /* File Operation Compatibility Mode  */
//#define NSOCKET 0x00000003 /* Socket Function Native Mode */
//#define NFILE   0x00000004 /* File Operation Native Mode  */

/*-------------------------------------------------------------------*/
/* Capability Parameter Block                                        */
/*-------------------------------------------------------------------*/
CPB cap =
{
        0x00,      /* IPv6 Socket Types not supported */
        0x00,      /* IPv4 Socket Types not supported */
        0x00       /* Resources and modes supported   */
#if defined(CSOCKET)
              | 0x80
#endif /* defined(CSOCKET) */
#if defined(CFILE)
              | 0x40
#endif /* defined(CFILE) */
#if defined(NSOCKET)
              | 0x20
#endif /* defined(NSOCKET) */
#if defined(NFILE)
              | 0x10
#endif /* defined(NFILE) */
        ,
                   /*  Indicate if large files supported */
#if defined( _MSVC_ )
             0x80
#elif defined(_LFS_LARGEFILE) || ( defined(SIZEOF_OFF_T) && SIZEOF_OFF_T > 4 )
             0x80
#elif defined(_LFS64_LARGEFILE)
             0x80
#endif
/*        ( 0x80 * ( SIZEOF_SIZE_T == 8 ) ) */
#if   defined(__gnu_linux__) || defined(__linux__)
          + 0x01
#elif defined(WIN32)
          + 0x02
#elif defined(SOLARIS)
          + 0x03
#elif defined(__APPLE__)
          + 0x04
#elif defined( FREEBSD_OR_NETBSD )
          + 0x05
#elif defined(_AIX)
          + 0x06
#endif
        ,
        /* Compatibility Operation Options in big endian format */
        {(COMPAT_OPTIONS+DF18_VER)>>8,(COMPAT_OPTIONS+DF18_VER)&0xFF},
        /* Native Operation Options in big endian format */
        {(NATIVE_OPTIONS+DF18_VER)>>8,(NATIVE_OPTIONS+DF18_VER)&0xFF},
        /* Maximum Guest Shadow Sockets */
        {0,0},
        /* reserved */
        0,0,0,0,0,0
};

#endif /* !defined(_HDIAGF18_H) */

/*-------------------------------------------------------------------*/
/* Internal Architecture Independent Function Prototypes             */
/*-------------------------------------------------------------------*/
U16 df18_ck_opts(U16, U16, REGS *);  /* Check options */

/*-------------------------------------------------------------------*/
/*  Check Operation Options                                          */
/*-------------------------------------------------------------------*/
U16 df18_ck_opts(U16 regopts, U16 invalid, REGS *regs)
{
U16 amode;     /* address mode requested for parameter block */

    amode = regopts & AMODE_MASK;
    if (
           /* version must be specified */
           ( (regopts & VER_MASK)==0 ) ||
           /* version must not be higher than we support */
           ( (regopts & VER_MASK)>DF18_VER )  ||
           /* options are valid */
           ( ((regopts & OPTIONS_MASK) & invalid ) != 0 ) ||
           /* one and only one amode requested */
           ( !(amode==0x0040 || amode==0x0020 || amode==0x0010) )
       )
    {
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
    }
    return regopts;
}

#endif /*!defined(_HDIAGF18_C)*/

/*-------------------------------------------------------------------*/
/* Internal Architecture Dependent Function Prototypes               */
/*-------------------------------------------------------------------*/
/* Input/Output Request Functions */
void ARCH_DEP(hdiagf18_FC) (U32, VADR, REGS *);

/*-------------------------------------------------------------------*/
/* Access Host Resource (Function code 0xF18)                        */
/*-------------------------------------------------------------------*/
/* r1   - Function subcode                                           */
/*        0x00000000 - Query Capability                              */
/*        0x00000001 - Socket Function Compatibility Mode            */
/*        0x00000002 - File Operation Compatibility Mode             */
/*        0x00000003 - Socket Function Native Mode                   */
/*        0x00000004 - File Operation Native Mode                    */
/* r1+1 - Operation options                                          */
/* r2   - Parameter block real address                               */

void ARCH_DEP(diagf18_call) (int r1, int r2, REGS *regs)
{
/* Guest related paramters and values                                */
U16     options;                     /* supplied options             */

#if 0
    if (sizeof(CPB) != 16)
        LOGMSG( "CPB size not 8: %d\n", sizeof( CPB ));
#endif

    /* Specification exception if Rx is not even/odd or facility not enabled */
    if ( (!FACILITY_ENABLED( HERC_HOST_RESOURCE_ACCESS, regs )) ||
         ((r1 & 0x1) != 0)
       )
    {
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
    }

    switch(regs->GR_L(r1))
    {

/*--------------------------------------------------------*/
/* Perform the Query Operation                            */
/*--------------------------------------------------------*/
    case QUERY:

        /* Specification exception if CPB is not on a doubleword boundary */
        if ( (regs->GR(2) & 0x7 ) !=0 )
        {
            ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
        }

        /* Store the CPB at the designated location */
        ARCH_DEP(wstorec)
            (&cap,(BYTE)sizeof(CPB)-1,(VADR)GR_A(r2,regs),USE_REAL_ADDR,regs);

        break;

#if defined(CSOCKET)
/*--------------------------------------------------------*/
/* Perform Socket Function in Compatibility Mode          */
/*--------------------------------------------------------*/
    case CSOCKET:

#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
        if (regs->psw.amode64)
        {
            ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
        }
#endif /* defined(ESAME) */
         break;

#endif /* defined(CSOCKET) */

#if defined(CFILE)
/*--------------------------------------------------------*/
/* Perform File Operation in Compatibility Mode           */
/*--------------------------------------------------------*/
    case CFILE:

#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
        if (regs->psw.amode64)
        {
            ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
        }
#endif /* defined(ESAME) */

        options = df18_ck_opts
            ( (U16)regs->GR_L(r1+1) & 0x0000FFFF, (U16) COMPAT_INVALID, regs );

        /* Retrieve the Parameter Block address from Ry */
        ARCH_DEP(hdiagf18_FC) (options, (VADR)GR_A(r2,regs), regs);

        break;

#endif /* defined(CFILE) */

#if defined(NSOCKET)
/*--------------------------------------------------------*/
/* Perform Socket Function in Native Mode                 */
/*--------------------------------------------------------*/
    case NSOCKET:
        break;

#endif /* defined(NSOCKET) */

#if defined(NFILE)
/*--------------------------------------------------------*/
/* Perform File Operation in Native Mode                  */
/*--------------------------------------------------------*/
    case NFILE:
        break;

#endif /* defined(NFILE) */

    default:
        ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);

    } /* end switch(regs->GR_L(r1)) */

} /* end function ARCH_DEP( diagf18_call ) */


#endif /* defined(_FEATURE_HOST_RESOURCE_ACCESS_FACILITY) */

#if !defined(_GEN_ARCH)

#if defined(_ARCH_NUM_1)
 #define  _GEN_ARCH _ARCH_NUM_1
 #include "hdiagf18.c"
#endif

#if defined(_ARCH_NUM_2)
 #undef   _GEN_ARCH
 #define  _GEN_ARCH _ARCH_NUM_2
 #include "hdiagf18.c"
#endif

#endif /*!defined(_GEN_ARCH)*/
