/* ASSIST.C     (C) Copyright Roger Bowler, 1999-2012                */
/*              (C) Copyright Jan Jaeger, 1999-2012                  */
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

/*              Instruction decode rework - Jan Jaeger               */
/*              Correct address wraparound - Jan Jaeger              */
/*              Add dummy assist instruction - Jay Maynard,          */
/*                  suggested by Brandon Hill                        */

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

/* Address space control block offsets */
#define ASCBLOCK        0x080           /* Local lock                */
#define ASCBLSWQ        0x084           /* Local lock suspend queue  */

/* Lock interface table offsets */
#define LITOLOC         (-16)           /* Obtain local error exit   */
#define LITRLOC         (-12)           /* Release local error exit  */
#define LITOCMS         (-8)            /* Obtain CMS error exit     */
#define LITRCMS         (-4)            /* Release CMS error exit    */

#endif /*!defined(_ASSIST_C)*/

/* The macro below allows each assist instruction to execute in a virtual machine. Per
   GA22-7072-0, these assist instructions must co-exist with ECPS:VM if present (whether
   enabled or disabled), so that MVS running as a guest of VM can use these assists.
   This is the Virtual Machine Extended Facility Assist feature, also known as "370E"
   in VM. The macro will allow execution of these privileged assist instructions when
   the real PSW is in the problem state - if and only if the guest virtual machine PSW
   is in the virtual supervisor state (ECPSVM_CR6_VIRTPROB=0) and the 370E feature is
   enabled (ECPS_CR6_VMMVSAS=1). Otherwise, PRIV_CHECK is invoked to cause a
   privileged operation exception.
*/
#define GUEST_CHECK( ) \
    if(PROBSTATE(&regs->psw)) \
    { \
        if ((regs->CR_L(6) & (ECPSVM_CR6_VIRTPROB + ECPSVM_CR6_VMMVSAS)) != ECPSVM_CR6_VMMVSAS) \
           PRIV_CHECK(regs); \
    }

#if !defined(FEATURE_S390_DAT) && !defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)

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

    SSE(inst, regs, b1, effective_addr1, b2, effective_addr2);

    GUEST_CHECK( );

    /* The Page Fix assist cannot return via the PTT_ERR( ) method as used in most of the
       other assists here.  Per GA22-7079-1 IBM System/370 Assists for MVS, this assist must
       NOT exit to the next sequential instruction.  Instead, we follow the 'simplified
       execution path' described on page 3 of that documentation for Fix Page. */
    regs->GR_L(14) = PSW_IA_FROM_IP(regs, 0);
    mplp = ARCH_DEP( vfetch4 )( (effective_addr2 & ADDRESS_MAXWRAP( regs )), USE_INST_SPACE, regs );
    regs->GR_L(15) = ARCH_DEP( vfetch4 )( (( mplp+MPLPFAL ) & ADDRESS_MAXWRAP( regs )), USE_INST_SPACE, regs );
    SET_PSW_IA_AND_MAYBE_IP(regs, regs->GR_L(15));
}
#endif /*!defined(FEATURE_S390_DAT) && !defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/


/*-------------------------------------------------------------------*/
/* E503       - SVC assist                                     [SSE] */
/*-------------------------------------------------------------------*/
DEF_INST(svc_assist)
{
int     b1, b2;                         /* Values of base field      */
VADR    effective_addr1,
        effective_addr2;                /* Effective addresses       */

    SSE(inst, regs, b1, effective_addr1, b2, effective_addr2);

    GUEST_CHECK( );

    PTT_ERR("*E503 SVCA",effective_addr1,effective_addr2,regs->psw.IA_L);
    /*INCOMPLETE: NO ACTION IS TAKEN, THE SVC IS UNASSISTED
                  AND MVS WILL HAVE TO HANDLE THE SITUATION*/

}


/*-------------------------------------------------------------------*/
/* E504       - Obtain Local Lock                              [SSE] */
/*-------------------------------------------------------------------*/
DEF_INST(obtain_local_lock)
{
int     b1, b2;                         /* Values of base field      */
VADR    effective_addr1,
        effective_addr2;                /* Effective addresses       */
VADR    ascb_addr;                      /* Virtual address of ASCB   */
VADR    lock_addr;                      /* Virtual addr of ASCBLOCK  */
U32     hlhi_word;                      /* Highest lock held word    */
VADR    lit_addr;                       /* Virtual address of lock
                                           interface table           */
U32     lock;                           /* Lock value                */
U32     lcpa;                           /* Logical CPU address       */
VADR    newia;                          /* Unsuccessful branch addr  */
int     acc_mode = 0;                   /* access mode to use        */

    SSE(inst, regs, b1, effective_addr1, b2, effective_addr2);

    GUEST_CHECK( );

    /* Specification exception if operands are not on word boundary */
    if ((effective_addr1 & 0x00000003) || (effective_addr2 & 0x00000003))
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);

    PERFORM_SERIALIZATION(regs);

    /* Obtain main-storage access lock */
    OBTAIN_MAINLOCK(regs);

    if (ACCESS_REGISTER_MODE(&regs->psw))
        acc_mode = USE_PRIMARY_SPACE;

    /* Load ASCB address from first operand location */
    ascb_addr = ARCH_DEP(vfetch4) ( effective_addr1, acc_mode, regs );

    /* Load locks held bits from second operand location */
    hlhi_word = ARCH_DEP(vfetch4) ( effective_addr2, acc_mode, regs );

    /* Fetch our logical CPU address from PSALCPUA */
    lcpa = ARCH_DEP(vfetch4) ( effective_addr2 - 4, acc_mode, regs );

    lock_addr = (ascb_addr + ASCBLOCK) & ADDRESS_MAXWRAP(regs);

    /* Fetch the local lock from the ASCB */
    lock = ARCH_DEP(vfetch4) ( lock_addr, acc_mode, regs );

    /* Obtain the local lock if not already held by any CPU */
    if (lock == 0
        && (hlhi_word & PSALCLLI) == 0)
    {
        /* Store the unchanged value into the second operand to
           ensure suppression in the event of an access exception */
        ARCH_DEP(vstore4) ( hlhi_word, effective_addr2, acc_mode, regs );

        /* Store our logical CPU address in ASCBLOCK */
        ARCH_DEP(vstore4) ( lcpa, lock_addr, acc_mode, regs );

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

    /* Release main-storage access lock */
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

    GUEST_CHECK( );

    /* Specification exception if operands are not on word boundary */
    if ((effective_addr1 & 0x00000003) || (effective_addr2 & 0x00000003))
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);

    /* Obtain main-storage access lock */
    OBTAIN_MAINLOCK(regs);

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

        /* Set the local lock to zero */
        ARCH_DEP(vstore4) ( 0, lock_addr, acc_mode, regs );

        /* Clear the local lock held bit in the second operand */
        hlhi_word &= ~PSALCLLI;
        ARCH_DEP(vstore4) ( hlhi_word, effective_addr2, acc_mode, regs );

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
    RELEASE_MAINLOCK(regs);

} /* end function release_local_lock */


/*-------------------------------------------------------------------*/
/* E506       - Obtain CMS Lock                                [SSE] */
/*-------------------------------------------------------------------*/
DEF_INST(obtain_cms_lock)
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
VADR    newia;                          /* Unsuccessful branch addr  */
int     acc_mode = 0;                   /* access mode to use        */

    SSE(inst, regs, b1, effective_addr1, b2, effective_addr2);

    GUEST_CHECK( );

    /* Specification exception if operands are not on word boundary */
    if ((effective_addr1 & 0x00000003) || (effective_addr2 & 0x00000003))
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);

    PERFORM_SERIALIZATION(regs);

    /* General register 11 contains the lock address */
    lock_addr = regs->GR_L(11) & ADDRESS_MAXWRAP(regs);
    lock_arn = 11;

    /* Obtain main-storage access lock */
    OBTAIN_MAINLOCK(regs);

    if (ACCESS_REGISTER_MODE(&regs->psw))
        acc_mode = USE_PRIMARY_SPACE;

    /* Load ASCB address from first operand location */
    ascb_addr = ARCH_DEP(vfetch4) ( effective_addr1, acc_mode, regs );

    /* Load locks held bits from second operand location */
    hlhi_word = ARCH_DEP(vfetch4) ( effective_addr2, acc_mode, regs );

    /* Fetch the lock addressed by general register 11 */
    lock = ARCH_DEP(vfetch4) ( lock_addr, acc_mode, regs );

    /* Obtain the lock if not held by any ASCB, and if this CPU
       holds the local lock and does not hold a CMS lock */
    if (lock == 0
        && (hlhi_word & (PSALCLLI | PSACMSLI)) == PSALCLLI)
    {
        /* Store the unchanged value into the second operand to
           ensure suppression in the event of an access exception */
        ARCH_DEP(vstore4) ( hlhi_word, effective_addr2, acc_mode, regs );

        /* Store the ASCB address in the CMS lock */
        ARCH_DEP(vstore4) ( ascb_addr, lock_addr, acc_mode, regs );

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

    /* Release main-storage access lock */
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

    GUEST_CHECK( );

    /* Specification exception if operands are not on word boundary */
    if ((effective_addr1 & 0x00000003) || (effective_addr2 & 0x00000003))
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);

    /* General register 11 contains the lock address */
    lock_addr = regs->GR_L(11) & ADDRESS_MAXWRAP(regs);
    lock_arn = 11;

    /* Obtain main-storage access lock */
    OBTAIN_MAINLOCK(regs);

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

        /* Set the CMS lock to zero */
        ARCH_DEP(vstore4) ( 0, lock_addr, acc_mode, regs );

        /* Clear the CMS lock held bit in the second operand */
        hlhi_word &= ~PSACMSLI;
        ARCH_DEP(vstore4) ( hlhi_word, effective_addr2, acc_mode, regs );

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
    RELEASE_MAINLOCK(regs);

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

    GUEST_CHECK( );

    /* Specification exception if operands are not on word boundary */
    if ((effective_addr1 & 0x00000003) || (effective_addr2 & 0x00000003))
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);

    PTT_ERR("*E50D TRRTN",effective_addr1,effective_addr2,regs->psw.IA_L);
    /*INCOMPLETE: NO TRACE ENTRY IS GENERATED*/

}
#endif /*!defined(FEATURE_TRACING)*/


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
