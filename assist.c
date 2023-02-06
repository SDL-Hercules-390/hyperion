/* ASSIST.C     (C) Copyright Roger Bowler, 1999-2012                */
/*              (C) Copyright Jan Jaeger, 1999-2012                  */
/*              (C) and others 2013-2023                             */
/*              ESA/390 MVS Assist Routines                          */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/* z/Architecture support - (C) Copyright Jan Jaeger, 1999-2012      */

/*-------------------------------------------------------------------*/
/* This module contains routines which process the MVS Assist        */
/* instructions described in the manual GA22-7079-01.                */
/*-------------------------------------------------------------------*/

/*           Instruction decode rework - Jan Jaeger                  */
/*           Correct address wraparound - Jan Jaeger                 */
/*           Add dummy assist instruction - Jay Maynard,             */
/*               suggested by Brandon Hill                           */

#include "hstdinc.h"

#define _ASSIST_C_
#define _HENGINE_DLL_

#include "hercules.h"
#include "opcode.h"
#include "inline.h"
#include "ecpsvm.h"

DISABLE_GCC_UNUSED_SET_WARNING;

#if !defined(_ASSIST_C)

#define _ASSIST_C

/*-------------------------------------------------------------------*/
/* Control block offsets fixed by architecture                       */
/*-------------------------------------------------------------------*/

/* Prefixed storage area offsets */
#define PSALCPUA        0x2F4           /* Logical CPU address       */
#define PSAHLHI         0x2F8           /* Locks held indicators     */

/* Bit settings for PSAHLHI */
#define PSACMSLI        0x00000002      /* CMS lock held indicator   */
#define PSALCLLI        0x00000001      /* Local lock held indicator */

#define PSACSTK         0x380           /* Ptr to FRR stack          */
#define PSALSFCC        0x3F4           /* Branch addr if stack full */
#define PSAXMFLG        0x49C           /* Cross Memory Flags        */
/* Bit settings for PSAXMFLG */
#define PSAXMODE        0x40            /* 0=Primary, 1=Secondary    */

#define PSAXMCR3        0x5C4           /* CR3 & CR4 in PSA          */

/* Address space control block offsets */
#define ASCBLOCK        0x080           /* Local lock                */
#define ASCBLSWQ        0x084           /* Local lock suspend queue  */

/* Lock interface table offsets */
#define LITOLOC         (-16)           /* Obtain local error exit   */
#define LITRLOC         (-12)           /* Release local error exit  */
#define LITOCMS         (-8)            /* Obtain CMS error exit     */
#define LITRCMS         (-4)            /* Release CMS error exit    */

#endif /*!defined(_ASSIST_C)*/

/* The macro below allows each assist instruction to execute in a virtual
   machine. Per GA22-7072-0, these assist instructions must co-exist with
   ECPS:VM if present (whether enabled or disabled), so that MVS running
   as a guest of VM can use these assists. This is the "Virtual Machine
   Extended Facility Assist" feature, also known as "370E" in VM. The macro
   will allow execution of these privileged assist instructions when the
   real PSW is in the problem state -- if and only if the guest virtual
   machine PSW is in the virtual supervisor state (ECPSVM_CR6_VIRTPROB=0)
   and the 370E feature is enabled (ECPS_CR6_VMMVSAS=1).  Otherwise, the
   PRIV_CHECK macro is invoked to cause a privileged operation exception.
*/
#define GUEST_CHECK( ) \
    if (PROBSTATE( &regs->psw )) \
    { \
        if ((regs->CR_L(6) & (ECPSVM_CR6_VIRTPROB + ECPSVM_CR6_VMMVSAS)) != ECPSVM_CR6_VMMVSAS) \
           PRIV_CHECK( regs ); \
    }

#if !defined( FEATURE_S390_DAT ) && !defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )

/*-------------------------------------------------------------------*/
/* E502       - Page Fix                                       [SSE] */
/*-------------------------------------------------------------------*/
DEF_INST(fix_page)
{
int     b1, b2;                         /* Values of base field      */
VADR    effective_addr1,
        effective_addr2;                /* Effective addresses       */
RADR    mplp;

#define MPLPFAL  0x34

    SSE( inst, regs, b1, effective_addr1, b2, effective_addr2 );
    PER_ZEROADDR_XCHECK2( regs, b1, b2 );

    GUEST_CHECK();

    /* The Page Fix assist cannot return via the PTT_ERR( ) method as used
       in most of the other assists here.  Per GA22-7079-1 "IBM System/370
       Assists for MVS", this assist must NOT exit to the next sequential
       instruction.  Instead, we follow the "simplified execution path"
       described on page 3 of that documentation for Fix Page.
    */
    regs->GR_L(14) = PSW_IA_FROM_IP(regs, 0);

    mplp = ARCH_DEP( vfetch4 )( (effective_addr2 & ADDRESS_MAXWRAP( regs )),
        USE_INST_SPACE, regs );

    regs->GR_L(15) = ARCH_DEP( vfetch4 )( (( mplp+MPLPFAL ) & ADDRESS_MAXWRAP( regs )),
        USE_INST_SPACE, regs );

    SET_PSW_IA_AND_MAYBE_IP( regs, regs->GR_L(15) );
}
#endif /* !defined( FEATURE_S390_DAT ) && !defined( FEATURE_001_ZARCH_INSTALLED_FACILITY ) */


/*-------------------------------------------------------------------*/
/* E503       - SVC assist                                     [SSE] */
/*-------------------------------------------------------------------*/
DEF_INST(svc_assist)
{
int     b1, b2;                         /* Values of base field      */
VADR    effective_addr1,
        effective_addr2;                /* Effective addresses       */

    SSE(inst, regs, b1, effective_addr1, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, b1, b2 );

    GUEST_CHECK( );

    PTT_ERR( "*E503 SVCA", effective_addr1, effective_addr2, regs->psw.IA_L );
    /*INCOMPLETE: NO ACTION IS TAKEN, THE SVC IS UNASSISTED
                  AND MVS WILL HAVE TO HANDLE THE SITUATION*/
}


/*-------------------------------------------------------------------*/
/* E504       - Obtain Local Lock                              [SSE] */
/*-------------------------------------------------------------------*/
DEF_INST(obtain_local_lock)
{
    int     b1, b2;                     /* Values of base field      */
    VADR    effective_addr1,
            effective_addr2;            /* Effective addresses       */
    VADR    ascb_addr;                  /* Virtual address of ASCB   */
    U32     hlhi_word;                  /* Highest lock held word    */
    VADR    lit_addr;                   /* Virtual address of lock
                                           interface table           */
    U32     lcpa;                       /* Logical CPU address       */
    VADR    newia;                      /* Unsuccessful branch addr  */
    BYTE   *mainstor;                   /* mainstor address          */
    U32     old;                        /* old value                 */
    U32     new;                        /* new value                 */
    int     acc_mode = 0;               /* access mode to use        */

    SSE(inst, regs, b1, effective_addr1, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, b1, b2 );

    GUEST_CHECK( );

    /* Specification exception if operands are not on word boundary */
    if ((effective_addr1 & 0x00000003) || (effective_addr2 & 0x00000003))
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);

    PERFORM_SERIALIZATION(regs);

    /* MAINLOCK may be required if cmpxchg assists unavailable */
    OBTAIN_MAINLOCK(regs);
    {
        if (ACCESS_REGISTER_MODE(&regs->psw))
            acc_mode = USE_PRIMARY_SPACE;

        /* Load ASCB address from first operand location */
        ascb_addr = ARCH_DEP(vfetch4) ( effective_addr1, acc_mode, regs );

        /* Load locks held bits from second operand location */
        hlhi_word = ARCH_DEP(vfetch4) ( effective_addr2, acc_mode, regs );

        /* Fetch our logical CPU address from PSALCPUA */
        lcpa = ARCH_DEP(vfetch4) ( effective_addr2 - 4, acc_mode, regs );

        /* Get mainstor address of ASCBLOCK word */
        mainstor = MADDRL (ascb_addr + ASCBLOCK, 4, b2, regs, ACCTYPE_WRITE, regs->psw.pkey);

        /* The lock word should contain 0; use this as our compare value.
           Swap in the CPU address in lpca */
        old = 0;
        new = CSWAP32(lcpa);

        /* Try exchanging values; cmpxchg4 returns 0=success, !0=failure */
        if (!cmpxchg4( &old, new, mainstor ))
        {
            /* Store the unchanged value into the second operand to
            ensure suppression in the event of an access exception */
            ARCH_DEP(vstore4) ( hlhi_word, effective_addr2, acc_mode, regs );

            /* Set the local lock held bit in the second operand */
            hlhi_word |= PSALCLLI;
            ARCH_DEP(vstore4) ( hlhi_word, effective_addr2, acc_mode, regs );

            /* Set register 13 to zero to indicate lock obtained */
            regs->GR_L(13) = 0;
        }
        else
        {
            /* Fetch the lock interface table address from the
            second word of the second operand, and load the
            new instruction address and amode from LITOLOC */
            lit_addr = ARCH_DEP(vfetch4) ( effective_addr2 + 4, acc_mode, regs ) + LITOLOC;
            lit_addr &= ADDRESS_MAXWRAP(regs);
            newia = ARCH_DEP(vfetch4) ( lit_addr, acc_mode, regs );

            /* Save the link information in register 12 */
            regs->GR_L(12) = PSW_IA_FROM_IP(regs, 0);

            /* Copy LITOLOC into register 13 to signify obtain failure */
            regs->GR_L(13) = newia;

            /* Update the PSW instruction address */
            SET_PSW_IA_AND_MAYBE_IP(regs, newia);
        }
    }
    RELEASE_MAINLOCK(regs);

    PERFORM_SERIALIZATION(regs);

} /* end function obtain_local_lock */


/*-------------------------------------------------------------------*/
/* E505       - Release Local Lock                             [SSE] */
/*-------------------------------------------------------------------*/
DEF_INST(release_local_lock)
{
int     b1, b2;                         /* Values of base field      */
VADR    effective_addr1,
        effective_addr2;                /* Effective addresses       */
VADR    ascb_addr;                      /* Virtual address of ASCB   */
VADR    lock_addr;                      /* Virtual addr of ASCBLOCK  */
VADR    susp_addr;                      /* Virtual addr of ASCBLSWQ  */
U32     hlhi_word;                      /* Highest lock held word    */
VADR    lit_addr;                       /* Virtual address of lock
                                           interface table           */
U32     lock;                           /* Lock value                */
U32     susp;                           /* Lock suspend queue        */
U32     lcpa;                           /* Logical CPU address       */
VADR    newia;                          /* Unsuccessful branch addr  */
int     acc_mode = 0;                   /* access mode to use        */

    SSE(inst, regs, b1, effective_addr1, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, b1, b2 );

    GUEST_CHECK( );

    /* Specification exception if operands are not on word boundary */
    if ((effective_addr1 & 0x00000003) || (effective_addr2 & 0x00000003))
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);

    /* Obtain main-storage access lock */
    OBTAIN_MAINLOCK_UNCONDITIONAL(regs);

    if (ACCESS_REGISTER_MODE(&regs->psw))
        acc_mode = USE_PRIMARY_SPACE;

    /* Load ASCB address from first operand location */
    ascb_addr = ARCH_DEP(vfetch4) ( effective_addr1, acc_mode, regs );

    /* Load locks held bits from second operand location */
    hlhi_word = ARCH_DEP(vfetch4) ( effective_addr2, acc_mode, regs );

    /* Fetch our logical CPU address from PSALCPUA */
    lcpa = ARCH_DEP(vfetch4) ( effective_addr2 - 4, acc_mode, regs );

    /* Fetch the local lock and the suspend queue from the ASCB */
    lock_addr = (ascb_addr + ASCBLOCK) & ADDRESS_MAXWRAP(regs);
    susp_addr = (ascb_addr + ASCBLSWQ) & ADDRESS_MAXWRAP(regs);
    lock = ARCH_DEP(vfetch4) ( lock_addr, acc_mode, regs );
    susp = ARCH_DEP(vfetch4) ( susp_addr, acc_mode, regs );

    /* Test if this CPU holds the local lock, and does not hold
       any CMS lock, and the local lock suspend queue is empty */
    if (lock == lcpa
        && (hlhi_word & (PSALCLLI | PSACMSLI)) == PSALCLLI
        && susp == 0)
    {
        /* Store the unchanged value into the second operand to
           ensure suppression in the event of an access exception */
        ARCH_DEP(vstore4) ( hlhi_word, effective_addr2, acc_mode, regs );

        /* Clear the local lock held bit in the second operand */
        hlhi_word &= ~PSALCLLI;
        ARCH_DEP(vstore4) ( hlhi_word, effective_addr2, acc_mode, regs );

        /* Set the local lock to zero */
        ARCH_DEP(vstore4) ( 0, lock_addr, acc_mode, regs );

        /* Set register 13 to zero to indicate lock released */
        regs->GR_L(13) = 0;
    }
    else
    {
        /* Fetch the lock interface table address from the
           second word of the second operand, and load the
           new instruction address and amode from LITRLOC */
        lit_addr = ARCH_DEP(vfetch4) ( effective_addr2 + 4, acc_mode, regs ) + LITRLOC;
        lit_addr &= ADDRESS_MAXWRAP(regs);
        newia = ARCH_DEP(vfetch4) ( lit_addr, acc_mode, regs );

        /* Save the link information in register 12 */
        regs->GR_L(12) = PSW_IA_FROM_IP(regs, 0);

        /* Copy LITRLOC into register 13 to signify release failure */
        regs->GR_L(13) = newia;

        /* Update the PSW instruction address */
        SET_PSW_IA_AND_MAYBE_IP(regs, newia);
    }

    /* Release main-storage access lock */
    RELEASE_MAINLOCK_UNCONDITIONAL(regs);

} /* end function release_local_lock */


/*-------------------------------------------------------------------*/
/* E506       - Obtain CMS Lock                                [SSE] */
/*-------------------------------------------------------------------*/
DEF_INST(obtain_cms_lock)
{
    int     b1, b2;                     /* Values of base field      */
    VADR    effective_addr1,
            effective_addr2;            /* Effective addresses       */
    VADR    ascb_addr;                  /* Virtual address of ASCB   */
    U32     hlhi_word;                  /* Highest lock held word    */
    VADR    lit_addr;                   /* Virtual address of lock
                                           interface table           */
    VADR    lock_addr;                  /* Lock address              */
    int     lock_arn;                   /* Lock access register      */
    U32     lock;                       /* Lock value                */
    VADR    newia;                      /* Unsuccessful branch addr  */
    BYTE   *mainstor;                   /* mainstor address          */
    U32     old;                        /* old value                 */
    U32     new;                        /* new value                 */
    U32     locked = 0;                 /* status of cmpxchg4 result */
    int     acc_mode = 0;               /* access mode to use        */

    SSE(inst, regs, b1, effective_addr1, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, b1, b2 );

    GUEST_CHECK( );

    /* Specification exception if operands are not on word boundary */
    if ((effective_addr1 & 0x00000003) || (effective_addr2 & 0x00000003))
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);

    PERFORM_SERIALIZATION(regs);

    /* General register 11 contains the lock address */
    lock_addr = regs->GR_L(11) & ADDRESS_MAXWRAP(regs);
    lock_arn = 11;

    /* MAINLOCK may be required if cmpxchg assists unavailable */
    OBTAIN_MAINLOCK(regs);
    {
        if (ACCESS_REGISTER_MODE(&regs->psw))
            acc_mode = USE_PRIMARY_SPACE;

        /* Load ASCB address from first operand location */
        ascb_addr = ARCH_DEP(vfetch4) ( effective_addr1, acc_mode, regs );

        /* Load locks held bits from second operand location */
        hlhi_word = ARCH_DEP(vfetch4) ( effective_addr2, acc_mode, regs );

        /* Fetch the lock addressed by general register 11 */
        lock = ARCH_DEP(vfetch4) ( lock_addr, acc_mode, regs );

        /* Validate that the address space meets criteria to obtain the CMS lock:
             the target lock word pointed to by GR11 must be 0,
             the LOCAL lock *must* be held on this CPU,
             and the CMS lock must *not* be held on this CPU.  */
        if (lock == 0
            && (hlhi_word & (PSALCLLI | PSACMSLI)) == PSALCLLI)
        {
            /* Get mainstor address of lock word */
            mainstor = MADDRL (lock_addr, 4, b2, regs, ACCTYPE_WRITE, regs->psw.pkey);

            /* The lock word should contain 0; use this as our compare value.
            Swap in the ASCB address from instruction operand 1            */
            old = 0;
            new = CSWAP32(ascb_addr);

            /* Try exchanging values; cmpxchg4 returns 0=success, !0=failure */
            locked = !cmpxchg4( &old, new, mainstor );
        }

        if (locked)
        {
            /* Store the unchanged value into the second operand to
            ensure suppression in the event of an access exception */
            ARCH_DEP(vstore4) ( hlhi_word, effective_addr2, acc_mode, regs );

            /* Set the CMS lock held bit in the second operand */
            hlhi_word |= PSACMSLI;
            ARCH_DEP(vstore4) ( hlhi_word, effective_addr2, acc_mode, regs );

            /* Set register 13 to zero to indicate lock obtained */
            regs->GR_L(13) = 0;
        }
        else
        {
            /* Fetch the lock interface table address from the
            second word of the second operand, and load the
            new instruction address and amode from LITOCMS */
            lit_addr = ARCH_DEP(vfetch4) ( effective_addr2 + 4, acc_mode, regs ) + LITOCMS;
            lit_addr &= ADDRESS_MAXWRAP(regs);
            newia = ARCH_DEP(vfetch4) ( lit_addr, acc_mode, regs );

            /* Save the link information in register 12 */
            regs->GR_L(12) = PSW_IA_FROM_IP(regs, 0);

            /* Copy LITOCMS into register 13 to signify obtain failure */
            regs->GR_L(13) = newia;

            /* Update the PSW instruction address */
            SET_PSW_IA_AND_MAYBE_IP(regs, newia);
        }
    }
    RELEASE_MAINLOCK(regs);

    PERFORM_SERIALIZATION(regs);

} /* end function obtain_cms_lock */


/*-------------------------------------------------------------------*/
/* E507       - Release CMS Lock                               [SSE] */
/*-------------------------------------------------------------------*/
DEF_INST(release_cms_lock)
{
int     b1, b2;                         /* Values of base field      */
VADR    effective_addr1,
        effective_addr2;                /* Effective addresses       */
VADR    ascb_addr;                      /* Virtual address of ASCB   */
U32     hlhi_word;                      /* Highest lock held word    */
VADR    lit_addr;                       /* Virtual address of lock
                                           interface table           */
VADR    lock_addr;                      /* Lock address              */
int     lock_arn;                       /* Lock access register      */
U32     lock;                           /* Lock value                */
U32     susp;                           /* Lock suspend queue        */
VADR    newia;                          /* Unsuccessful branch addr  */
int     acc_mode = 0;                   /* access mode to use        */

    SSE(inst, regs, b1, effective_addr1, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, b1, b2 );

    GUEST_CHECK( );

    /* Specification exception if operands are not on word boundary */
    if ((effective_addr1 & 0x00000003) || (effective_addr2 & 0x00000003))
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);

    /* General register 11 contains the lock address */
    lock_addr = regs->GR_L(11) & ADDRESS_MAXWRAP(regs);
    lock_arn = 11;

    /* Obtain main-storage access lock */
    OBTAIN_MAINLOCK_UNCONDITIONAL(regs);

    if (ACCESS_REGISTER_MODE(&regs->psw))
        acc_mode = USE_PRIMARY_SPACE;

    /* Load ASCB address from first operand location */
    ascb_addr = ARCH_DEP(vfetch4) ( effective_addr1, acc_mode, regs );

    /* Load locks held bits from second operand location */
    hlhi_word = ARCH_DEP(vfetch4) ( effective_addr2, acc_mode, regs );

    /* Fetch the CMS lock and the suspend queue word */
    lock = ARCH_DEP(vfetch4) ( lock_addr, acc_mode, regs );
    susp = ARCH_DEP(vfetch4) ( lock_addr + 4, acc_mode, regs );

    /* Test if current ASCB holds this lock, the locks held indicators
       show a CMS lock is held, and the lock suspend queue is empty */
    if (lock == ascb_addr
        && (hlhi_word & PSACMSLI)
        && susp == 0)
    {
        /* Store the unchanged value into the second operand to
           ensure suppression in the event of an access exception */
        ARCH_DEP(vstore4) ( hlhi_word, effective_addr2, acc_mode, regs );

        /* Clear the CMS lock held bit in the second operand */
        hlhi_word &= ~PSACMSLI;
        ARCH_DEP(vstore4) ( hlhi_word, effective_addr2, acc_mode, regs );

        /* Set the CMS lock to zero */
        ARCH_DEP(vstore4) ( 0, lock_addr, acc_mode, regs );

        /* Set register 13 to zero to indicate lock released */
        regs->GR_L(13) = 0;
    }
    else
    {
        /* Fetch the lock interface table address from the
           second word of the second operand, and load the
           new instruction address and amode from LITRCMS */
        lit_addr = ARCH_DEP(vfetch4) ( effective_addr2 + 4, acc_mode, regs ) + LITRCMS;
        lit_addr &= ADDRESS_MAXWRAP(regs);
        newia = ARCH_DEP(vfetch4) ( lit_addr, acc_mode, regs );

        /* Save the link information in register 12 */
        regs->GR_L(12) = PSW_IA_FROM_IP(regs, 0);

        /* Copy LITRCMS into register 13 to signify release failure */
        regs->GR_L(13) = newia;

        /* Update the PSW instruction address */
        SET_PSW_IA_AND_MAYBE_IP(regs, newia);
    }

    /* Release main-storage access lock */
    RELEASE_MAINLOCK_UNCONDITIONAL(regs);

} /* end function release_cms_lock */


#if !defined(FEATURE_TRACING)
/*-------------------------------------------------------------------*/
/* E508       - Trace SVC Interruption                         [SSE] */
/*-------------------------------------------------------------------*/
DEF_INST(trace_svc_interruption)
{
int     b1, b2;                         /* Values of base field      */
VADR    effective_addr1,
        effective_addr2;                /* Effective addresses       */

    SSE(inst, regs, b1, effective_addr1, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, b1, b2 );

    GUEST_CHECK( );

    /* Specification exception if operands are not on word boundary */
    if ((effective_addr1 & 0x00000003) || (effective_addr2 & 0x00000003))
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);

    PTT_ERR("*E508 TRSVC",effective_addr1,effective_addr2,regs->psw.IA_L);
    /*INCOMPLETE: NO TRACE ENTRY IS GENERATED*/
}


/*-------------------------------------------------------------------*/
/* E509       - Trace Program Interruption                     [SSE] */
/*-------------------------------------------------------------------*/
DEF_INST(trace_program_interruption)
{
int     b1, b2;                         /* Values of base field      */
VADR    effective_addr1,
        effective_addr2;                /* Effective addresses       */

    SSE(inst, regs, b1, effective_addr1, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, b1, b2 );

    GUEST_CHECK( );

    /* Specification exception if operands are not on word boundary */
    if ((effective_addr1 & 0x00000003) || (effective_addr2 & 0x00000003))
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);

    PTT_ERR("*E509 TRPGM",effective_addr1,effective_addr2,regs->psw.IA_L);
    /*INCOMPLETE: NO TRACE ENTRY IS GENERATED*/
}


/*-------------------------------------------------------------------*/
/* E50A       - Trace Initial SRB Dispatch                     [SSE] */
/*-------------------------------------------------------------------*/
DEF_INST(trace_initial_srb_dispatch)
{
int     b1, b2;                         /* Values of base field      */
VADR    effective_addr1,
        effective_addr2;                /* Effective addresses       */

    SSE(inst, regs, b1, effective_addr1, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, b1, b2 );

    GUEST_CHECK( );

    /* Specification exception if operands are not on word boundary */
    if ((effective_addr1 & 0x00000003) || (effective_addr2 & 0x00000003))
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);

    PTT_ERR("*E50A TRSRB",effective_addr1,effective_addr2,regs->psw.IA_L);
    /*INCOMPLETE: NO TRACE ENTRY IS GENERATED*/
}


/*-------------------------------------------------------------------*/
/* E50B       - Trace I/O Interruption                         [SSE] */
/*-------------------------------------------------------------------*/
DEF_INST(trace_io_interruption)
{
int     b1, b2;                         /* Values of base field      */
VADR    effective_addr1,
        effective_addr2;                /* Effective addresses       */

    SSE(inst, regs, b1, effective_addr1, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, b1, b2 );

    GUEST_CHECK( );

    /* Specification exception if operands are not on word boundary */
    if ((effective_addr1 & 0x00000003) || (effective_addr2 & 0x00000003))
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);

    PTT_ERR("*E50B TRIO",effective_addr1,effective_addr2,regs->psw.IA_L);
    /*INCOMPLETE: NO TRACE ENTRY IS GENERATED*/
}


/*-------------------------------------------------------------------*/
/* E50C       - Trace Task Dispatch                            [SSE] */
/*-------------------------------------------------------------------*/
DEF_INST(trace_task_dispatch)
{
int     b1, b2;                         /* Values of base field      */
VADR    effective_addr1,
        effective_addr2;                /* Effective addresses       */

    SSE(inst, regs, b1, effective_addr1, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, b1, b2 );

    GUEST_CHECK( );

    /* Specification exception if operands are not on word boundary */
    if ((effective_addr1 & 0x00000003) || (effective_addr2 & 0x00000003))
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);

    PTT_ERR("*E50C TRTSK",effective_addr1,effective_addr2,regs->psw.IA_L);
    /*INCOMPLETE: NO TRACE ENTRY IS GENERATED*/
}


/*-------------------------------------------------------------------*/
/* E50D       - Trace SVC Return                               [SSE] */
/*-------------------------------------------------------------------*/
DEF_INST(trace_svc_return)
{
int     b1, b2;                         /* Values of base field      */
VADR    effective_addr1,
        effective_addr2;                /* Effective addresses       */

    SSE(inst, regs, b1, effective_addr1, b2, effective_addr2);
    PER_ZEROADDR_XCHECK2( regs, b1, b2 );

    GUEST_CHECK( );

    /* Specification exception if operands are not on word boundary */
    if ((effective_addr1 & 0x00000003) || (effective_addr2 & 0x00000003))
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);

    PTT_ERR("*E50D TRRTN",effective_addr1,effective_addr2,regs->psw.IA_L);
    /*INCOMPLETE: NO TRACE ENTRY IS GENERATED*/
}
#endif /*!defined(FEATURE_TRACING)*/


/*-------------------------------------------------------------------*/
/* B242       - Add FRR                                        [RRE] */
/*-------------------------------------------------------------------*/
DEF_INST( add_frr )
{
/*                          ADDFRR

        Add new Functional-Recovery-Routine Stack entry

   What little documentation that exists for this feature can be found
   on page 9 of IBM System/370 Assists for MVS, GA22-7079-1 (the text
   of which is copied verbatim further below), which can be found on
   Bitsavers at:

      * https://bitsavers.org/pdf/ibm/370/MVS/GA22-7079-1_IBM_System_370_Assists_for_MVS_2nd_ed_198110.pdf

   The rest of the documentation regarding how it works is found in the
   way MVS itself manages the FRR stack. The assist feature is part of
   the 3033 Extensions.

   r1 contains an entry value designating the type of FRR and action to
   take by this assist. r2 contains the address of the FRR to be added.
*/
/*                          ADDFRR

    "A new entry is added to the top of the current functional-
     recovery-routine (FRR) stack. The entry is initialized with
     values provided in general registers and with the PSW S bit
     (bit 16)."

    "Optionally, the contents of control registers 3 and 4 are
     saved in an entry in a separate table."

    "The general register designated by the r2 field provides the
     logical address of the FRR entry point."

    "Before instruction execution, the general register designated
     by the r1 field provides three bytes that are stored in the
     FRR entry and whose value determines if control registers 3
     and 4 are to be stored as well. When instruction execution is
     completed, the register designated by r1 contains the logical
     address of the six-word work area within the new, current
     FRR-stack entry."

    "Logical location 380 hex contains the logical address of the
     stack-table header. The stack-table header contains (1) a logical
     address which is 32 less than the address of the first dynamic
     entry in the stack table, (2) the logical address of the last
     entry in the stack table, and (3) the logical address of the
     current stack-table entry."

    "At an offset from the beginning of the stack-table header is
     found a table of stack-entry-extension entries. Optionally,
     the contents of control registers 3 and 4 are saved in an
     extension entry. One extension entry corresponds to each entry
     in the stack table. The offset to the table of extension entries,
     and the encoded length of an extension entry, are found in the
     word at logical location BA8 hex."

    Condition Code: The code remains unchanged.

    Program Exceptions:
    Access (storage operands)
    Operation (when the instruction is not installed)
    Privileged operation
    Specification
*/

int     r1, r2;
VADR    frrstak;
VADR    frrparm;
VADR    frrlast;
U32     frrsize;
VADR    frrcurr;
VADR    frrnext;
VADR    newia;
VADR    cr_ptr;
BYTE    entrycode;
U32     size, len;
VADR    clear_vaddr;
static const BYTE zeros[256] = {0};

#define FRRSPARM    0x08

/* Entry code bits in r1 */
#define EUT         0x80
#define FULLXM      0x08
#define PRIMARY     0x04
#define LOCAL       0x02
#define GLOBAL      0x01
#define HOME        0x00

    RRE( inst, regs, r1, r2 );

    PRIV_CHECK( regs );

    /* Obtain needed values from the FRR stack pointers */
    frrstak = ARCH_DEP( vfetch4 )( PSACSTK,                                  USE_PRIMARY_SPACE, regs ) & ADDRESS_MAXWRAP( regs );
    frrparm = ARCH_DEP( vfetch4 )( (frrstak +  0),                           USE_PRIMARY_SPACE, regs ) & ADDRESS_MAXWRAP( regs );
    frrlast = ARCH_DEP( vfetch4 )( (frrstak +  4) & ADDRESS_MAXWRAP( regs ), USE_PRIMARY_SPACE, regs ) & ADDRESS_MAXWRAP( regs );
    frrsize = ARCH_DEP( vfetch4 )( (frrstak +  8) & ADDRESS_MAXWRAP( regs ), USE_PRIMARY_SPACE, regs );
    frrcurr = ARCH_DEP( vfetch4 )( (frrstak + 12) & ADDRESS_MAXWRAP( regs ), USE_PRIMARY_SPACE, regs ) & ADDRESS_MAXWRAP( regs );
    frrnext = (frrcurr + frrsize) & ADDRESS_MAXWRAP( regs );

    /* Determine if FRR stack is full. If yes, then
       branch to the system supplied code at PSALFSCC
    */
    if (frrnext > frrlast)
    {
        newia = ARCH_DEP( vfetch4 )( PSALSFCC, USE_PRIMARY_SPACE, regs ) & ADDRESS_MAXWRAP( regs );
        SET_PSW_IA_AND_MAYBE_IP( regs, newia );
        return;
    }

    /* Perform exactly one of the following three functions based on the entry code from r1 */
    entrycode = regs->GR_LHLCL( r1 );

    /*  1. SETFRR  A,MODE=HOME (no LOCAL/GLOBAL or EUT specification) */

    if (entrycode == HOME)
    {
        /* Set the FRR entry point from r2 in the stack */
        ARCH_DEP( vstore4 )( regs->GR_L(r2), frrnext, USE_PRIMARY_SPACE, regs );
    }

    /*  2. SETFRR A,MODE=(HOME, with any combination of LOCAL or GLOBAL or EUT=YES.  */

    else if ((!(entrycode & (FULLXM + PRIMARY))) && (entrycode & (EUT + GLOBAL + LOCAL)))
    {
        /* Set the FRR entry point from r2 in the stack */
        ARCH_DEP( vstore4 )( regs->GR_L(r2) | 0x00000001, frrnext, USE_PRIMARY_SPACE, regs );

        /* The entry code is stored in the FRR stack */
        ARCH_DEP( vstoreb )( entrycode, (frrnext + 7) & ADDRESS_MAXWRAP( regs ), USE_PRIMARY_SPACE, regs );
    }

    /*  3. SETFRR A,MODE=(FULLXM | PRIMARY, with any or no combination of LOCAL or GLOBAL or EUT=YES.  */

    else
    {
        BYTE cr34[8];  // CR3 and CR4

        /* Set the FRR entry point from r2 in the stack */
        ARCH_DEP( vstore4 )( regs->GR_L(r2) | 0x00000001, frrnext, USE_PRIMARY_SPACE, regs );

        /* Check if in secondary access mode; if yes turn on secondary bit in the entry code */
        if (ARCH_DEP( vfetchb )( (PSAXMFLG & ADDRESS_MAXWRAP( regs )), USE_PRIMARY_SPACE, regs ) & PSAXMODE)
            entrycode |= PSAXMODE;  // indicate to FRR in secondary mode

        /* The entry code is stored in the FRR stack */
        ARCH_DEP( vstoreb )( entrycode, ((frrnext + 7) & ADDRESS_MAXWRAP( regs )), USE_PRIMARY_SPACE, regs );

        /* Compute the address of the FRR area where CR3 and CR4 are copied from the PSA */
        cr_ptr = frrnext - frrparm;
        cr_ptr = cr_ptr >> 2;
        cr_ptr = (frrstak + cr_ptr + 120) & ADDRESS_MAXWRAP( regs );

        /* Copy CR3 and CR4 values from PSA to computed FRR area */
        ARCH_DEP( vfetchc )( cr34, 8-1, PSAXMCR3, USE_PRIMARY_SPACE, regs );
        ARCH_DEP( vstorec )( cr34, 8-1, cr_ptr, 0, regs );
    }

    /* Update the FRR stack pointers to point to the newly added FRR */
    ARCH_DEP( vstore4 )( frrnext, (frrstak + 12) & ADDRESS_MAXWRAP( regs ), USE_PRIMARY_SPACE, regs );

    /* Return with the FRRSPARM area address in r1 per the assist documentation  */
    regs->GR_L(r1) = (frrnext + FRRSPARM) & ADDRESS_MAXWRAP( regs );

    /* Initialize (clear) the rest of the FRR stack */
    clear_vaddr = (frrnext + 8) & ADDRESS_MAXWRAP( regs );
    len = 256;
    size = frrsize;
    while (size)
    {
        if (len > size) len = size;
        ARCH_DEP( vstorec )( zeros, len-1, clear_vaddr, 0, regs );
        size        -= len;
        clear_vaddr += len;
        clear_vaddr &= ADDRESS_MAXWRAP( regs );
    }
}


#if !defined(_GEN_ARCH)

#if defined(_ARCH_NUM_1)
 #define  _GEN_ARCH _ARCH_NUM_1
 #include "assist.c"
#endif

#if defined(_ARCH_NUM_2)
 #undef   _GEN_ARCH
 #define  _GEN_ARCH _ARCH_NUM_2
 #include "assist.c"
#endif

#endif /*!defined(_GEN_ARCH)*/
