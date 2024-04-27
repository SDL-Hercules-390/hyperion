/*ECPSVM.C      (C) Copyright Roger Bowler, 2000-2012                */
/*              (C) and others 2013-2021                             */
/*              Hercules ECPS:VM Support                             */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/***********************************************************/
/* HERCULES ECPS:VM Support                                */
/* (C) Copyright 2003-2009 Roger Bowler and Others         */
/* Use of this program is governed by the QPL License      */
/* Original Author : Ivan Warren                           */
/* Prime Maintainer : Ivan Warren                          */
/***********************************************************/

/***********************************************************/
/*                                                         */
/* General guidelines about E6XX instruction class         */
/* this is an implementation of ECPS:VM Level 20           */
/*                                                         */
/* General rule is : Only do what is safe to do. In doubt, */
/*                   give control to CP back (and act as   */
/*                   a NO-OP). All instructions have this  */
/*                   behaviour, therefore allowing only    */
/*                   partial implementation, or bypassing  */
/*                   tricky cases                          */
/*                                                         */
/* NOTE : ECPS:VM is only available for S/370 architecture */
/*                                                         */
/* In order for CP ASSIST to be active, a CONFIGURATION    */
/* statement is added : ECPSVM lvl|no                      */
/* lvl is the ASSIST level (20 is recommended)             */
/* no means CP ASSIST is disabled (default)                */
/*                                                         */
/* Currently supported CP ASSIST instructions :            */
/* +-----+-------+----------------------------------------+*/
/* |opc  | Mnemo | Function                               |*/
/* +-----+-------+----------------------------------------+*/
/* |E602 | LCKPG | Lock Page in core table                |*/
/* |E603 | ULKPG | Unlock page in core table              |*/
/* |E604 | DNCCW | Decode next CCW (CCW translation)      |*/
/* |E605 | FCCWS | Free CCW storage                       |*/
/* |E606 | SCNVU | Scan Virtual Unit control blocks       |*/
/* |E607 | DISP1 | Dispatcher assist                      |*/
/* |E608 | TRBRG | LRA + Basic checks on VPAGE            |*/
/* |E609 | TRLOK | Same as TRBRG + Lock page in core      |*/
/* |E60A | VIST  | Invalidate shadow segment table        |*/
/* |E60B | VIPT  | Invalidate shadow page table           |*/
/* |E60C | DFCCW | Decode first CCW (CCW translation)     |*/
/* |E60D | DISP0 | Dispatcher assist                      |*/
/* |E60E | SCNRU | Scan Real Unit control blocks          |*/
/* |E60F | CCWGN | General CCW translation                |*/
/* |E610 | UXCCW | Untranslate CCW                        |*/
/* |E611 | DISP2 | Dispatcher assist                      |*/
/* |E612 | STLVL | Store ECPS:VM Level                    |*/
/* |E613 | LCSPG | Locate changed shared page             |*/
/* |E614 | FREEX | Allocate CP FREE Storage from subpool  |*/
/* |E615 | FRETX | Release CP FREE Storage to subpool     |*/
/* |0A08 | LINK  | CP SVC 8 (LINK) assist                 |*/
/* |0A0C | RETRN | CP SVC 12 (RETURN) assist              |*/
/* +-----+-------+----------------------------------------+*/
/*                                                         */
/* Currently supported VM ASSIST instructions :            */
/* +-----+-------+----------------------------------------+*/
/* |opc  | Mnemo | Function                               |*/
/* +-----+-------+----------------------------------------+*/
/* |0A   | SVC   | Virtual SVC Assist                     |*/
/* |80   | SSM   | Virtual SSM Assist                     |*/
/* |82   | LPSW  | Virtual LPSW Assist                    |*/
/* |AC   | STNSM | Virtual STNSM Assist                   |*/
/* |AD   | STOSM | Virtual STOSM Assist                   |*/
/* |B1   | LRA   | Virtual LRA Assist                     |*/
/* |B6   | STCTL | Virtual STCTL Assist                   |*/
/* |B7   | LCTL  | Virtual LCTL Assist                    |*/
/* +-----+-------+----------------------------------------+*/
/***********************************************************/
/*

// $Log$    Update revision number in define variable ECPSCODEVER, below.
//
// Revision 1.88  2021/03/25 18:55:00  bobpolmanter
// CP Assist DISP2 not reloading floating point registers correctly
//  resulting in arithmetic errors for virtual machines using floating point.
// Remove SIO and DIAG assists due to incompatibilities.

// Revision 1.87  2018/08/05 14:20:00  bobpolmanter
// CP Assist DFCCW failing on tranbrng error; let CP handle this instead
//  of transferring control to CCWBAD.
// SIO Assist giving bogus PIC 04 if PSW key is non-zero at SIO instruction
//  execution.
// CP Assist RETRN not completing successfully if SAVEAREA is between
//  end of nucleus and the dynamic paging area.
// Add support for DIAG instruction assist.
// Add support for STCTL instruction assist.
// Add support for CP assist LCSPG.
//
// Revision 1.86  2018/01/23 10:20:00  Peter Jansen
// CPU Timer fix to stop 2nd Level VM abends  (in SPT macro)
//
// Revision 1.85  2017/07/11 08:00:00  bobpolmanter
// Fix LRA bug with 2K page frames (incorrectly using 'bytemask' value
//  against CP page table entries),
// Remove checking for MICBLOK crossing a page boundary (not part of the
//  specification and causing ECPS to be ignored for some virtual machines).
//
// Revision 1.84  2017/07/02 10:50:00  bobpolmanter
// Fix LRA bug with 2K page frames (frame shift amount incorrect)
//
// Revision 1.83  2017/05/25 19:12:00  bobpolmanter/petercoghlan
// Fix DISP2 incorrect check of VMV370R and mis-loaded control registers;
// Remove DISP2 debug message causing page faults in CP.
//
// Revision 1.82  2017/04/30 13:54:00  bobpolmanter
// Fix two minor coding errors in the SIO instruction assist.
//
// Revision 1.81  2017/04/12 14:10:00  bobpolmanter
// Add support for LRA instruction assist.
// Fix two minor issues in DISP0 that did not match DMKDSPCH.
// Fix DISP0 to set VMPSWAIT on in exit #28.
//
// Revision 1.80  2017/02/18 14:05:00  bobpolmanter
// Add new support for the CCW translation assists
//  DFCCW, DNCCW, CCWGN, FCCWS, UXCCW.
//
// Revision 1.79  2017/02/10 07:25:00  bobpolmanter
// Add new support for the CP SVC 8/12 LINK/RETURN functions.
// Add new support for SIO/SIOF VM Assist.
//
// Revision 1.78  2017/02/05 08:15:00  bobpolmanter
// Add new support to allow assists to operate with the CP
//  FREE/FRET trap in effect.  Support "ECPSVM YES TRAP/NOTRAP".
//
// Revision 1.77  2017/02/04 15:45:00  bobpolmanter
// DISP2 dispatching user that is in virtual wait state;
//  add check for this condition and let CP handle it.
//
// Revision 1.76  2017/01/29 09:55:00  bobpolmanter
// DISP2 assist not completing for DAT-on guests due to incorrect
//  checking of shadow table and invalidate page table flags.
//
// Revision 1.75  2017/01/28 15:15:00  bobpolmanter
// Add new support for STNSM/STOSM instruction assists.
//
// Revision 1.74  2017/01/27 15:20:00  bobpolmanter
// Fix the reversed order of the EVM_ST operands in the CPEXBLOK FRET exit
//  of assist DISP2; was causing CP storage overlays and PRG001 failures.
//
// Revision 1.73  2017/01/25 18:22:00  bobpolmanter
// Generate assist-flagged CP trace table entries as documented by
//  IBM for FREEX, FRETX, and DISP2 assists.
//
// Revision 1.72  2017/01/24 12:53:00  bobpolmanter
// Instruction assists must go back to CP if virtual PSW in problem state
//
// Revision 1.71  2017/01/18 19:33:00  bobpolmanter
// Offset in PSA for APSTAT2 is incorrect in ecpsvm.h
//
// Revision 1.70  2017/01/15 12:00:00  bobpolmanter
// Add new support for CP Assists VIST and VIPT;
// Update comments throughout this source to reflect what is and is not supported.
//
// Revision 1.69  2017/01/12 12:00:00  bobpolmanter
// LCTL assist should not load DAS control regs 3-7;
// Virtual interval timer issue fixed in clock.c
//
// Revision 1.68  2007/06/23 00:04:09  ivan
// Update copyright notices to include current year (2007)
//
// Revision 1.67  2007/01/13 07:18:14  bernard
// backout ccmask
//
// Revision 1.66  2007/01/12 15:22:37  bernard
// ccmask phase 1
//
// Revision 1.65  2006/12/31 17:53:48  gsmith
// 2006 Dec 31 Update ecpsvm.c for new psw IA scheme
//
// Revision 1.64  2006/12/08 09:43:20  jj
// Add CVS message log
//
*/

#include "hstdinc.h"

#define _ECPSVM_C_
#define _HENGINE_DLL_

#include "hercules.h"
#include "opcode.h"
#include "inline.h"
#include "ecpsvm.h"

DISABLE_GCC_UNUSED_SET_WARNING;

#ifdef FEATURE_ECPSVM

#define ECPSCODEVER 1.88    //  <--------------- UPDATE CODE VERSION

ECPSVM_CMDENT *ecpsvm_getcmdent(char *cmd);

struct _ECPSVM_CPSTATS
{
    ECPSVM_STAT_DCL(SVC);
    ECPSVM_STAT_DCL(SSM);
    ECPSVM_STAT_DCL(LPSW);
    ECPSVM_STAT_DCL(STNSM);
    ECPSVM_STAT_DCL(STOSM);
    ECPSVM_STAT_DCL(SIO);
    ECPSVM_STAT_DCL(VTIMER);
    ECPSVM_STAT_DCL(STCTL);
    ECPSVM_STAT_DCL(LCTL);
    ECPSVM_STAT_DCL(DIAG);
    ECPSVM_STAT_DCL(IUCV);
    ECPSVM_STAT_DCL(LRA);
} ecpsvm_sastats={
    ECPSVM_STAT_DEF(SVC),
    ECPSVM_STAT_DEF(SSM),
    ECPSVM_STAT_DEF(LPSW),
    ECPSVM_STAT_DEF(STNSM),
    ECPSVM_STAT_DEF(STOSM),
    ECPSVM_STAT_DEFU(SIO),
    ECPSVM_STAT_DEF(VTIMER),
    ECPSVM_STAT_DEF(STCTL),
    ECPSVM_STAT_DEF(LCTL),
    ECPSVM_STAT_DEFU(DIAG),
    ECPSVM_STAT_DEFU(IUCV),
    ECPSVM_STAT_DEF(LRA),
};
struct _ECPSVM_SASTATS
{
    ECPSVM_STAT_DCL(FREE);
    ECPSVM_STAT_DCL(FRET);
    ECPSVM_STAT_DCL(LCKPG);
    ECPSVM_STAT_DCL(ULKPG);
    ECPSVM_STAT_DCL(SCNRU);
    ECPSVM_STAT_DCL(SCNVU);
    ECPSVM_STAT_DCL(DISP0);
    ECPSVM_STAT_DCL(DISP1);
    ECPSVM_STAT_DCL(DISP2);
    ECPSVM_STAT_DCL(DNCCW);
    ECPSVM_STAT_DCL(DFCCW);
    ECPSVM_STAT_DCL(FCCWS);
    ECPSVM_STAT_DCL(CCWGN);
    ECPSVM_STAT_DCL(UXCCW);
    ECPSVM_STAT_DCL(TRBRG);
    ECPSVM_STAT_DCL(TRLOK);
    ECPSVM_STAT_DCL(VIST);
    ECPSVM_STAT_DCL(VIPT);
    ECPSVM_STAT_DCL(STEVL);
    ECPSVM_STAT_DCL(FREEX);
    ECPSVM_STAT_DCL(FRETX);
    ECPSVM_STAT_DCL(PMASS);
    ECPSVM_STAT_DCL(LCSPG);
    ECPSVM_STAT_DCL(LINK);
    ECPSVM_STAT_DCL(RETRN);
} ecpsvm_cpstats={
    ECPSVM_STAT_DEFU(FREE),
    ECPSVM_STAT_DEFU(FRET),
    ECPSVM_STAT_DEF(LCKPG),
    ECPSVM_STAT_DEF(ULKPG),
    ECPSVM_STAT_DEF(SCNRU),
    ECPSVM_STAT_DEF(SCNVU),
    ECPSVM_STAT_DEF(DISP0),
    ECPSVM_STAT_DEF(DISP1),
    ECPSVM_STAT_DEF(DISP2),
    ECPSVM_STAT_DEF(DNCCW),
    ECPSVM_STAT_DEF(DFCCW),
    ECPSVM_STAT_DEF(FCCWS),
    ECPSVM_STAT_DEF(CCWGN),
    ECPSVM_STAT_DEF(UXCCW),
    ECPSVM_STAT_DEF(TRBRG),
    ECPSVM_STAT_DEF(TRLOK),
    ECPSVM_STAT_DEF(VIST),
    ECPSVM_STAT_DEF(VIPT),
    ECPSVM_STAT_DEF(STEVL),
    ECPSVM_STAT_DEF(FREEX),
    ECPSVM_STAT_DEF(FRETX),
    ECPSVM_STAT_DEFU(PMASS),
    ECPSVM_STAT_DEF(LCSPG),
    ECPSVM_STAT_DEF(LINK),
    ECPSVM_STAT_DEF(RETRN),
};

#define DEBUG_CPASSIST
#define DEBUG_SASSIST

#define DODEBUG_ASSIST(_cond,x)  \
{ \
    if((_cond)) \
    { \
        x; \
    }\
}

#if defined(DEBUG_SASSIST)
#define DEBUG_SASSISTX(_inst,x) \
{ \
    DODEBUG_ASSIST(ecpsvm_sastats._inst.debug,x) \
}
#else
#define DEBUG_SASSISTX(_cond,x)
#endif
#if defined(DEBUG_CPASSIST)
#define DEBUG_CPASSISTX(_inst,x) \
{ \
    DODEBUG_ASSIST(ecpsvm_cpstats._inst.debug,x) \
}
#else
#define DEBUG_CPASSISTX(_cond,x)
#endif

/* Utility macros because I am very lazy */

#define EVM_IC( x )  ARCH_DEP( vfetchb )( ((x) & ADDRESS_MAXWRAP( regs )), USE_REAL_ADDR, regs )
#define EVM_LH( x )  ARCH_DEP( vfetch2 )( ((x) & ADDRESS_MAXWRAP( regs )), USE_REAL_ADDR, regs )
#define EVM_L(  x )  ARCH_DEP( vfetch4 )( ((x) & ADDRESS_MAXWRAP( regs )), USE_REAL_ADDR, regs )
#define EVM_LD( x )  ARCH_DEP( vfetch8 )( ((x) & ADDRESS_MAXWRAP( regs )), USE_REAL_ADDR, regs )

#define EVM_STD( x, y )  ARCH_DEP( vstore8 )( (x), ((y) & ADDRESS_MAXWRAP( regs )), USE_REAL_ADDR, regs )
#define EVM_ST(  x, y )  ARCH_DEP( vstore4 )( (x), ((y) & ADDRESS_MAXWRAP( regs )), USE_REAL_ADDR, regs )
#define EVM_STH( x, y )  ARCH_DEP( vstore2 )( (x), ((y) & ADDRESS_MAXWRAP( regs )), USE_REAL_ADDR, regs )
#define EVM_STC( x, y )  ARCH_DEP( vstoreb )( (x), ((y) & ADDRESS_MAXWRAP( regs )), USE_REAL_ADDR, regs )

#define EVM_MVC( x, y, z )  ARCH_DEP( vfetchc )( (x), (z), (y), USE_REAL_ADDR, regs )

#define BR14 SET_PSW_IA_AND_MAYBE_IP(regs, regs->GR_L(14))

#define INITPSEUDOIP(_regs) \
    do {    \
        (_regs).ip=(BYTE*)"\0\0";  \
    } while(0)

#define INITPSEUDOREGS(_regs) \
    do { \
        memset(&(_regs), 0, sysblk.regs_copy_len); \
        INITPSEUDOIP((_regs)); \
    } while(0)

#define CPASSIST_HIT(_stat) ecpsvm_cpstats._stat.hit++

#define SASSIST_HIT(_stat) ecpsvm_sastats._stat.hit++

#define SASSIST_LPSW(_regs) \
    do { \
        MAYBE_SET_PSW_IA_FROM_IP(&(_regs)); \
        SET_PSW_IA_AND_MAYBE_IP(regs, _regs.psw.IA); \
        regs->psw.cc=_regs.psw.cc; \
        regs->psw.pkey=_regs.psw.pkey; \
        regs->psw.progmask=_regs.psw.progmask; \
    } \
    while(0)

#define SASSIST_PROLOG( _instname) \
    VADR amicblok; \
    char buf[512]; \
    VADR vpswa; \
    BYTE *vpswa_p; \
    REGS vpregs; \
    BYTE  micpend; \
    U32   CR6; \
    ECPSVM_MICBLOK micblok; \
    BYTE micevma; \
    BYTE micevma2; \
    BYTE micevma3; \
    BYTE micevma4; \
    if(SIE_STATE(regs)) \
        return(1); \
    if(!PROBSTATE(&regs->psw)) \
    { \
          return(1); \
    } \
    if(!sysblk.ecpsvm.available) \
    { \
          DEBUG_SASSISTX(_instname,WRMSG(HHC90000, "D", "SASSIST "#_instname" ECPS:VM Disabled in configuration")); \
          return(1); \
    } \
    if(!ecpsvm_sastats._instname.enabled) \
    { \
          DEBUG_SASSISTX(_instname,WRMSG(HHC90000, "D", "SASSIST "#_instname" ECPS:VM Disabled by command")); \
          return(1); \
    } \
    CR6=regs->CR_L(6); \
    regs->ecps_vtmrpt = NULL; /* Assume vtimer off until validated */ \
    if(!(CR6 & ECPSVM_CR6_VMASSIST)) \
    { \
        DEBUG_SASSISTX(_instname,WRMSG(HHC90000, "D", "EVMA Disabled by guest")); \
        return(1); \
    } \
    /* 2017-01-23  Reject if Virtual PSW is in problem state */ \
    /* All instruction assists should be rejected if VPSW is in problem state and be reflected back    */  \
    /* to CP for handling.  This affects 2nd level VM hosting 3rd level guests.                        */  \
    if(CR6 & ECPSVM_CR6_VIRTPROB) \
    { \
        DEBUG_SASSISTX(_instname,WRMSG(HHC90000, "D", "SASSIST "#_instname" reject : Virtual problem state")); \
        return(1); \
    } \
    /* End of 2017-01-23   */  \
    /* Increment call now (don't count early misses) */ \
    ecpsvm_sastats._instname.call++; \
    amicblok=CR6 & ECPSVM_CR6_MICBLOK; \
    /* Load the micblok copy */ \
    micblok.MICRSEG=EVM_L(amicblok); \
    micblok.MICCREG=EVM_L(amicblok+4); \
    micblok.MICVPSW=EVM_L(amicblok+8); \
    micblok.MICWORK=EVM_L(amicblok+12); \
    micblok.MICVTMR=EVM_L(amicblok+16); \
    micblok.MICACF=EVM_L(amicblok+20); \
    micpend=(micblok.MICVPSW >> 24); \
    vpswa=micblok.MICVPSW & ADDRESS_MAXWRAP(regs); \
    micevma=(micblok.MICACF >> 24); \
    micevma2=((micblok.MICACF & 0x00ff0000) >> 16); \
    micevma3=((micblok.MICACF & 0x0000ff00) >> 8); \
    micevma4=(micblok.MICACF  & 0x000000ff); \
    if((CR6 & ECPSVM_CR6_VIRTTIMR)) \
    { \
        regs->ecps_vtmrpt = MADDR(micblok.MICVTMR,USE_REAL_ADDR,regs,ACCTYPE_READ,0); \
    } \
    /* Set ref bit on page where Virtual PSW is stored */ \
    vpswa_p=MADDR(vpswa,USE_REAL_ADDR,regs,ACCTYPE_READ,0); \
    DEBUG_SASSISTX(_instname,MSGBUF(buf, "SASSIST "#_instname" VPSWA= %8.8X Virtual",vpswa)); \
    DEBUG_SASSISTX(_instname,WRMSG(HHC90000, "D", buf)); \
    DEBUG_SASSISTX(_instname,MSGBUF(buf, "SASSIST "#_instname" CR6= %8.8X",CR6)); \
    DEBUG_SASSISTX(_instname,WRMSG(HHC90000, "D", buf)); \
    DEBUG_SASSISTX(_instname,MSGBUF(buf, "SASSIST "#_instname" MICVTMR= %8.8X",micblok.MICVTMR)); \
    DEBUG_SASSISTX(_instname,WRMSG(HHC90000, "D", buf)); \
    DEBUG_SASSISTX(_instname,MSGBUF(buf, "SASSIST "#_instname" Real PSW=")); \
    DEBUG_SASSISTX(_instname,display_psw(regs, &buf[strlen(buf)], sizeof(buf)-(int)strlen(buf))); \
    /* Load the Virtual PSW in a temporary REGS structure */ \
    INITPSEUDOREGS(vpregs); \
    ARCH_DEP(load_psw) (&vpregs,vpswa_p); \
    STRLCAT( buf, " PSW=" ); \
    DEBUG_SASSISTX(_instname,display_psw(&vpregs, &buf[strlen(buf)], sizeof(buf)-(int)strlen(buf))); \
    DEBUG_SASSISTX(_instname,WRMSG(HHC90000, "D", buf)); \

#define ECPSVM_PROLOG(_inst) \
int  b1, b2; \
VADR effective_addr1, effective_addr2; \
     SSE(inst, regs, b1, effective_addr1, b2, effective_addr2); \
     PRIV_CHECK(regs); \
     SIE_INTERCEPT(regs); \
     if(!sysblk.ecpsvm.available) \
     { \
          DEBUG_CPASSISTX(_inst,WRMSG(HHC90000, "D", "CPASSTS "#_inst" ECPS:VM Disabled in configuration")); \
          ARCH_DEP(program_interrupt) (regs, PGM_OPERATION_EXCEPTION); \
     } \
     PRIV_CHECK(regs); /* No problem state please */ \
     if(!ecpsvm_cpstats._inst.enabled) \
     { \
          DEBUG_CPASSISTX(_inst,WRMSG(HHC90000, "D", "CPASSTS "#_inst" Disabled by command")); \
          return; \
     } \
     if(!(regs->CR_L(6) & 0x02000000)) \
     { \
        return; \
     } \
     ecpsvm_cpstats._inst.call++; \
     DEBUG_CPASSISTX(_inst,WRMSG(HHC90000, "D", #_inst" called"));


/* DISPx Utility macros */

#define STPT(_x) \
{ \
    EVM_STD(get_cpu_timer(regs),_x); \
}

#define SPT(_x) \
{ \
    set_cpu_timer(regs,EVM_LD(_x)); \
    OBTAIN_INTLOCK(regs); \
    if(CPU_TIMER(regs) < 0) \
    { \
        ON_IC_PTIMER(regs); \
    } \
    else \
    { \
        OFF_IC_PTIMER(regs); \
    } \
    RELEASE_INTLOCK(regs); \
}

#define CHARGE_STOP(_x) \
{ \
    STPT(_x+VMTTIME); \
}

#define CHARGE_START(_x) \
{ \
    SPT(_x+VMTTIME); \
}

#define CHARGE_SWITCH(_x,_y) \
{ \
    CHARGE_STOP(_x); \
    CHARGE_START(_y); \
    (_x)=(_y); \
}

/* Function prototype declarations */

static int ecpsvm_int_lra(REGS *,VADR,RADR *);
static int ecpsvm_tranbrng(REGS *,VADR,VADR,RADR *);

/* 2017-01-25 CP Assist trace table support */
/* Utility function to get the next trace table entry for those CP assists */
/* that should generate a trace entry                                      */
VADR ecpsvm_get_trace_entry(REGS *regs)
{
VADR traceptr;
VADR nextptr;
    traceptr=EVM_L(TRACCURR);
    nextptr=traceptr+16;
    if (nextptr==EVM_L(TRACEND))
    {
        nextptr=EVM_L(TRACSTRT);
    }
    EVM_ST(nextptr,TRACCURR);
    return traceptr;
}
/* End of 2017-01-25 */

/* CP ASSISTs */

int ecpsvm_do_fretx(REGS *regs,VADR block,U16 numdw,VADR maxsztbl,VADR fretl);

/* E600 FREE Instruction */
/* CPASSIST FREE (Basic) Not supported */
/* This is part of ECPS:VM Level 18 and 19 */
/* (ECPS:VM Level 20 use FREEX, later below) */
/*-------------------------------------------------------------------*/
/* E600 FREE - Allocate CP Storage                             [SSE] */
/*-------------------------------------------------------------------*/
DEF_INST(ecpsvm_basic_freex)
{
    ECPSVM_PROLOG(FREE);
}

/* E601 FRET Instruction */
/* CPASSIST FRET (Basic) Not supported */
/* This is part of ECPS:VM Level 18 and 19 */
/* (ECPS:VM Level 20 use FRETX, later below) */
/*-------------------------------------------------------------------*/
/* E601 FRET - Return CP storage                               [SSE] */
/*-------------------------------------------------------------------*/
DEF_INST(ecpsvm_basic_fretx)
{
    ECPSVM_PROLOG(FRET);
}

/* Lockpage common code (LCKPG/TRLOK) */
static void ecpsvm_lockpage1(REGS *regs,RADR cortab,RADR pg)
{
    char buf[256];
    BYTE corcode;
    VADR corte;
    U32  lockcount;
    RADR cortbl;

    DEBUG_CPASSISTX(LCKPG,MSGBUF(buf, "LKPG coreptr = "F_RADR" Frame = "F_RADR,cortab,pg));
    DEBUG_CPASSISTX(LCKPG,WRMSG(HHC90000, "D", buf));
    cortbl=EVM_L(cortab);
    corte=cortbl+((pg & 0xfff000)>>8);
    DEBUG_CPASSISTX(LCKPG,MSGBUF(buf, "LKPG corete = %6.6X",corte));
    DEBUG_CPASSISTX(LCKPG,WRMSG(HHC90000, "D", buf));
    corcode=EVM_IC(corte+8);
    if(corcode & 0x80)
    {
        lockcount=EVM_L(corte+4);
        lockcount++;
    }
    else
    {
        lockcount=1;
        corcode|=0x80;
        EVM_STC(corcode,corte+8);
    }
    EVM_ST(lockcount,corte+4);
    DEBUG_CPASSISTX(LCKPG,MSGBUF(buf, "LKPG Page locked. Count = %6.6X",lockcount));
    DEBUG_CPASSISTX(LCKPG,WRMSG(HHC90000, "D", buf));
    return;
}

/* E602 LCKPG Instruction */
/* LCKPG D1(R1,B1),D2(R2,B2) */
/* 1st operand : PTR_PL -> Address of coretable */
/* 2nd Operand : Page address to be locked */
/*-------------------------------------------------------------------*/
/* E602 LCKPG - Lock Page                                      [SSE] */
/*-------------------------------------------------------------------*/
DEF_INST(ecpsvm_lock_page)
{
    char buf[256];
    VADR ptr_pl;
    VADR pg;

    ECPSVM_PROLOG(LCKPG);

    ptr_pl=effective_addr1;
    pg=effective_addr2;

    DEBUG_CPASSISTX(LCKPG,MSGBUF(buf, "LKPG PAGE=%6.6X, PTRPL=%6.6X",pg,ptr_pl));
    DEBUG_CPASSISTX(LCKPG,WRMSG(HHC90000, "D", buf));

    ecpsvm_lockpage1(regs,ptr_pl,pg);
    regs->psw.cc=0;
    BR14;
    CPASSIST_HIT(LCKPG);
    return;
}

/* Unlock Page common code (ULKPG/FCCWS) */
/* effective_addr1 : PTR_PL -> +0 - Maxsize, +4 Coretable */
/* effective_addr2 : Page address to be unlocked */
static int ecpsvm_unlockpage1(REGS *regs,VADR effective_addr1,VADR effective_addr2)
{
    char buf[256];
    VADR ptr_pl;
    VADR pg;
    VADR corsz;
    VADR cortbl;
    VADR corte;
    BYTE corcode;
    U32  lockcount;

    ptr_pl=effective_addr1;
    pg=effective_addr2;

    DEBUG_CPASSISTX(ULKPG,MSGBUF(buf, "ULKPG PAGE=%6.6X, PTRPL=%6.6X",pg,ptr_pl));
    DEBUG_CPASSISTX(ULKPG,WRMSG(HHC90000, "D", buf));

    corsz=EVM_L(ptr_pl);
    cortbl=EVM_L(ptr_pl+4);
    if((pg+4095)>corsz)
    {
        DEBUG_CPASSISTX(ULKPG,MSGBUF(buf, "ULKPG Page beyond core size of %6.6X",corsz));
        DEBUG_CPASSISTX(ULKPG,WRMSG(HHC90000, "D", buf));
        return(1);
    }
    corte=cortbl+((pg & 0xfff000)>>8);
    corcode=EVM_IC(corte+8);
    if(corcode & 0x80)
    {
        lockcount=EVM_L(corte+4);
        lockcount--;
    }
    else
    {
        DEBUG_CPASSISTX(ULKPG,WRMSG(HHC90000, "D", "ULKPG Attempting to unlock page that is not locked"));
        return(1);
    }
    if(lockcount==0)
    {
        corcode &= ~(0x80|0x02);
        EVM_STC(corcode,corte+8);
        DEBUG_CPASSISTX(ULKPG,WRMSG(HHC90000, "D", "ULKPG now unlocked"));
    }
    else
    {
        DEBUG_CPASSISTX(ULKPG,MSGBUF(buf, "ULKPG Page still locked. Count = %6.6X",lockcount));
        DEBUG_CPASSISTX(ULKPG,WRMSG(HHC90000, "D", buf));
    }
    EVM_ST(lockcount,corte+4);
    return(0);
}

/* E603 ULKPG Instruction */
/* ULKPG D1(R1,B1),D2(R2,B2) */
/* 1st operand : PTR_PL -> +0 - Maxsize, +4 Coretable */
/* 2nd Operand : Page address to be unlocked */
/*-------------------------------------------------------------------*/
/* E603 ULKPG - Unlock Page                                    [SSE] */
/*-------------------------------------------------------------------*/
DEF_INST(ecpsvm_unlock_page)
{
    ECPSVM_PROLOG(ULKPG);
    if(ecpsvm_unlockpage1(regs,effective_addr1,effective_addr2)!=0)
    {
        return;
    }

    CPASSIST_HIT(ULKPG);
    BR14;
    return;
}

/* Core Function "DECCW1":  Decode First CCW.
   This function is used by assists E604 DNCCW, and E60C DFCCW.

   On Entry:  R6 -> RADR of previous real CCW
              R9 -> VADR of previous virtual CCW
             R11 -> VMBLOK
             R13 -> SAVEAREA
   E60C Operand 1 -> data list of addresses (dl)  "CCWDATA"
                      +0 = V(DMKSYSCS)
                      +4 = A(CCWGENRL)
   E60C Operand 2 -> exit list of addresses (el)  "CCWEXITS"
                      +0 = A(CCWBAD)
                      +4 = A(CCWNROOM)
                      +8 = A(CCWTIC1)
                      +12= A(ADDRINVL)
                      +16= A(CCWTIC)
                      +20= A(NGCCW)
*/
int ecpsvm_do_deccw1(REGS *regs, VADR effective_addr1, VADR effective_addr2,RADR raddr)
{
VADR dl;
VADR el;
VADR savearea;
VADR rcw;
VADR devtable;
VADR devrtn;
int  cc;
BYTE ccwop;
BYTE ccwfl;
BYTE prevccwop;

    dl=effective_addr1;
    el=effective_addr2;

    /* Load required registers:
       R1 -> virtual CCW address
       R8 -> VDEVBLOK
       R3 contains 1st word of CCW
       R4 contains 2nd word of CCW
       Cache a copy of the CCW opcode and CCW flags
    */
    regs->GR_L(1)=regs->GR_L(9);
    savearea=regs->GR_L(13);
    regs->GR_L(8)=EVM_L(savearea+SAVER8)+EVM_L(regs->GR_L(11)+VMDVSTRT);
    regs->GR_L(3)=EVM_L(raddr);
    regs->GR_L(4)=EVM_L(raddr+4);
    ccwop=regs->GR_LHHCH(3);
    ccwfl=regs->GR_LHHCH(4);

    /* Append the CCW to the real CCW string; clear the CP control byte */
    EVM_ST(regs->GR_L(3),regs->GR_L(6));
    EVM_ST(regs->GR_L(4),regs->GR_L(6)+4);
    EVM_STC(0,regs->GR_L(6)+RCWCTL);

    /* Isolate CCW data address; A(CCWGENRL) must be in R2 before any exit */
    regs->GR_L(1)=regs->GR_L(3) & 0x00FFFFFF;
    regs->GR_L(2)=EVM_L(dl+4);

    /* Special processing only if the chain-data flag is set */
    if(EVM_IC(savearea+PRVFLAG) & CD)
    {
        if((ccwop & 0x0F)==0x08)                    /* TIC CCW ? */
        {
            DEBUG_CPASSISTX(DFCCW,WRMSG(HHC90000, "D", "DFCCW - Exit CCWTIC1"));
            EVM_STC(CDTIC,regs->GR_L(6));
            SET_PSW_IA_AND_MAYBE_IP(regs,EVM_L(el+8));           /* TIC follows chain data (CD), exit to CCWTIC1 */
            return(1);
        }
        else
        {                                           /* if not TIC but CD is set, do "CHOOSEOP" rtn */
            rcw=EVM_L(savearea+THISRCW)+RCWHSIZ;
            if(regs->GR_L(6)==rcw)
            {
                prevccwop=EVM_IC(savearea+PRVCOMND);
                regs->GR_LHHCH(3)=prevccwop;
                EVM_STC(prevccwop,regs->GR_L(6));
            }
            else
            {
                prevccwop=0x00;
                regs->GR_LHHCH(3)=0x00;
                EVM_STC(prevccwop,regs->GR_L(6));
            }
        }
    }
    else
    {
        EVM_STC(ccwop,savearea+VIRCOMND);
    }
    EVM_STC(ccwfl & (CD+CC),savearea+VIRFLAG);

    /* See if user data area is in a valid DAT segment */
    cc=ecpsvm_int_lra(regs,regs->GR_L(1),&raddr);
    if (cc==1)
    {
        DEBUG_CPASSISTX(DFCCW,WRMSG(HHC90000, "D", "DFCCW - Exit ADDRINVAL"));
        SET_PSW_IA_AND_MAYBE_IP(regs,EVM_L(el+12));          /* Invalid user data addr, exit to ADDRINVL */
        return(1);
    }

    /* Compute index for device type specific handler.  If CCW count is 0, check
       that the CCW command is a TIC, otherwise it is a bad CCW.
    */
    ccwop=EVM_IC(savearea+VIRCOMND) & 0x0F;
    ccwop*=2;
    regs->GR_L(4)&=0x0000FFFF;
    if(regs->GR_L(4)==0)
    {
        if(ccwop!=16)                           /* is CCW op not a TIC  (08 x 2) ?  */
        {
            DEBUG_CPASSISTX(DFCCW,WRMSG(HHC90000, "D", "DFCCW - Exit CCWBAD"));
            regs->GR_L(1)=regs->GR_L(9);
            SET_PSW_IA_AND_MAYBE_IP(regs,EVM_L(el+0));           /* CCW count is 0; exit to CCWBAD */
            return(1);
        }
        else
        {
            DEBUG_CPASSISTX(DFCCW,WRMSG(HHC90000, "D", "DFCCW - Exit CCWTIC"));
            regs->GR_L(4)--;
            SET_PSW_IA_AND_MAYBE_IP(regs,EVM_L(el+16));          /* CCW count is 0, but CCW was a TIC; its ok.  Exit to CCWTIC */
            return(1);
        }
    }
    DEBUG_CPASSISTX(DFCCW,WRMSG(HHC90000, "D", "DFCCW - Complete"));
    regs->GR_L(4)--;
    devtable=EVM_L(savearea+DEVTABLE);
    devrtn=regs->GR_L(12) + EVM_LH(devtable+ccwop);
    SET_PSW_IA_AND_MAYBE_IP(regs,devrtn);                    /* Success.  Exit to indexed device handler code */
    CPASSIST_HIT(DFCCW);
    return(0);
}

/* E604 DNCCW Instruction
   "Decode the next CCW"

   On Entry:  R6 -> RADR of previous real CCW
              R7 -> Address past the end of RCWTASK's CCW space
              R9 -> VADR of previous virtual CCW
             R11 -> VMBLOK
             R13 -> SAVEAREA
   E604 Operand 1 -> data list of addresses (dl)  "CCWDATA"
                      +0 = V(DMKSYSCS)
                      +4 = A(CCWGENRL)
   E604 Operand 2 -> exit list of addresses (el)  "CCWEXITS"
                      +0 = A(CCWBAD)
                      +4 = A(CCWNROOM)
                      +8 = A(CCWTIC1)
                      +12= A(ADDRINVL)
                      +16= A(CCWTIC)
                      +20= A(NGCCW)
*/

/*-------------------------------------------------------------------*/
/* E604 DNCCW - Decode Next CCW                                [SSE] */
/*-------------------------------------------------------------------*/
DEF_INST(ecpsvm_decode_next_ccw)
{
VADR dl;
VADR el;
RADR raddr;
VADR vaddr;

    ECPSVM_PROLOG(DNCCW);
    dl=effective_addr1;
    el=effective_addr2;

    vaddr=regs->GR_L(9)+8;
    if(ecpsvm_tranbrng(regs,dl+0,vaddr,&raddr)!=0)
    {
        DEBUG_CPASSISTX(DNCCW,WRMSG(HHC90000, "D", "DNCCW cant translate vaddr; back to CP"));
        return;
    }

    /* advance virtual CCW addr, real CCW addr.  Save VIRCOMND/VIRFLAG into PRVCOMND/PRVFLAG */
    regs->GR_L(9)+=8;
    regs->GR_L(6)+=8;
    EVM_STH(EVM_LH(regs->GR_L(13)+VIRCOMND),regs->GR_L(13)+PRVCOMND);

    /* See if there is room for one more CCW in RCWTASK.  If not we have to exit */
    if(regs->GR_L(6)+8 > regs->GR_L(7))
    {
        DEBUG_CPASSISTX(DFCCW,WRMSG(HHC90000, "D", "DNCCW - Exit CCWNROOM"));
        SET_PSW_IA_AND_MAYBE_IP(regs,EVM_L(el+4));           /* Not enough room in RCWTASK; exit to CCWNROOM */
        return;
    }
    /* Go directly into "decode first CCW" assist, and count a call to it */
    ecpsvm_cpstats.DFCCW.call++;
    ecpsvm_do_deccw1(regs,dl,el,raddr);
    CPASSIST_HIT(DNCCW);
    return;
}

/* E605 FCCWS Instruction
   Free CCW Storage (DMKUNTFR)

   On Entry: R10 -> IOBLOK
             R11 -> VMBLOK
             R13 -> SAVEAREA
   E605 Operand 1 -> data list of addresses (dl)  "DMKUNTF1"
                      +0 = V(DMKPTRPL)
                      +4 = V(DMKFREMX)
                      +8 = V(DMKFRETL)
   E605 Operand 2 -> exit list of addresses (el)  "DMKUNTF2"
                      +0 = A(UNREL)
                      +4 = A(UNTFRET)
                      +8 = A(PTRUL2)
                      +12= A(UNTFRXIT)
                      +16= A(NXTCCW)
                      +20= A(PTRUL1)
                      +24= A(ITSAREL)

  The nature of this assist leaves some uncertainties.  Following along with
  the code in DMKUNTFR, there are two other possible exit points that are not
  provided in the exit list documented above.  Should we just bail out at any need to
  exit at one of these other places and let CP re-execute the entire DMKUNTFR routine
  without any assist?  Should we attempt to determine some other exit point which
  runs the risk of change if someone reassembled DMKUNT?

  Instead we do this:  We will attempt to execute the assist step by step and if we do come
  to one of the two branch decisions where we need to exit but without a documented exit point
  in the list above, we will exit to the documented exit point NXTCCW and let CP re-execute
  the code from that point and make the appropriate exit decision. These places are marked by
  comments below.

  Most of the work is done in the S/370 registers rather than local variables because of the
  need to have the registers contain certain values along the way as required by the various
  exit points.
*/

/*-------------------------------------------------------------------*/
/* E605 FCCWS - Free CCW Storage                               [SSE] */
/*-------------------------------------------------------------------*/
DEF_INST(ecpsvm_free_ccwstor)
{
VADR dl;
VADR el;
VADR ptr_pl;
VADR rcw;
VADR corsz;
U32  i;
BYTE B_RCWCTL;
BYTE B_IOBSPEC2;

    ECPSVM_PROLOG(FCCWS);
    dl=effective_addr1;
    el=effective_addr2;
    ptr_pl=EVM_L(dl);               /* -> DMKPTRPL */
    corsz=EVM_L(ptr_pl);            /* DMKSYSRM - real machine size */

    for(i=0;i<12;i++)
    {
        EVM_ST(regs->GR_L(i),regs->GR_L(13)+SAVEREGS+i*4);
    }

    regs->GR_L(9)=corsz;
    rcw=(EVM_L(regs->GR_L(10)+IOBCAW) & 0x00FFFFFF) - 16;
    regs->GR_L(4)=rcw;

    while (rcw!=0)
    {
        /* At NXTFRET */
        regs->GR_L(7)=rcw+16;
        regs->GR_L(6)=EVM_LH(rcw+RCWRCNT);
        while (regs->GR_L(6) > 0)
        {
            /* At NXTCCW */
            B_RCWCTL=EVM_IC(regs->GR_L(7)+RCWCTL);
            if(B_RCWCTL & (RCWIO | RCWSHR))
            {
                regs->GR_L(5)=EVM_L(regs->GR_L(7)) & 0x00FFFFFF;
                if(EVM_IC(regs->GR_L(7)+RCWFLAG) & IDA)
                {
                    /* At IDASET */
                    regs->GR_L(1)=EVM_LH(regs->GR_L(7)+RCWCNT);
                    regs->GR_L(1)--;
                    regs->GR_L(14)=EVM_L(regs->GR_L(5));
                    regs->GR_L(15)=regs->GR_L(14);
                    if((EVM_IC(regs->GR_L(7)+RCWCOMND) & 0x0F) == 12)
                    {
                        regs->GR_L(14)-=regs->GR_L(1);  /* decr start addr if read backward CCW cmd */
                    }
                    else
                    {
                        regs->GR_L(15)+=regs->GR_L(1);  /* compute end addr of data buffer */
                    }
                    /* At IDLCHK */
                    /* Determine how many IDAWs we need to examine; count ends up in R0 */
                    regs->GR_L(14)=regs->GR_L(14) >> 11;
                    regs->GR_L(15)=regs->GR_L(15) >> 11;
                    regs->GR_L(15)-=regs->GR_L(14);
                    regs->GR_L(0)=regs->GR_L(15)+1;

                    if(B_RCWCTL & (RCWHMR | RCW2311))
                    {
                        SET_PSW_IA_AND_MAYBE_IP(regs,EVM_L(el+0));   /* Must do un-relocate.  Exit to UNREL */
                        return;
                    }

                    /* At UNLOCK */
                    /* Unlock the pages pointed to by each IDAW */
                    while (regs->GR_L(0) !=0)
                    {
                        regs->GR_L(2)=EVM_L(regs->GR_L(5));
                        if(regs->GR_L(2) < corsz)
                        {
                            if(!(B_RCWCTL & RCWIO))
                            {
                                SET_PSW_IA_AND_MAYBE_IP(regs,EVM_L(el+16));      /* No exit point provided for this condition, so */
                                return;                             /* let's exit NXTCCW and let CP re-do this.      */
                            }
                            else
                            {
                                if(ecpsvm_unlockpage1(regs,ptr_pl,regs->GR_L(2))!=0)
                                {
                                    SET_PSW_IA_AND_MAYBE_IP(regs,EVM_L(el+8));   /* Something wrong; exit to PTRUL2 */
                                    return;
                                }
                            }
                            /* Point to next IDAW; decrement remaining IDAW count */
                            regs->GR_L(5)+=4;
                            regs->GR_L(0)--;
                        }
                        else
                        {
                            break;      /* from while GR0!=0; this will take us to INCR8 below */
                        }
                    }
                }
                else
                {
                    /* unlock the page pointed to by the CCW data addresss */
                    regs->GR_L(2)=regs->GR_L(5);
                    if(!(B_RCWCTL & RCWIO))
                    {
                        SET_PSW_IA_AND_MAYBE_IP(regs,EVM_L(el+16));      /* No exit point provided for this condition, so */
                        return;                             /* let's exit NXTCCW and let CP re-do this.      */
                    }
                    else
                    {
                        if(ecpsvm_unlockpage1(regs,ptr_pl,regs->GR_L(2))!=0)
                        {
                            SET_PSW_IA_AND_MAYBE_IP(regs,EVM_L(el+20));  /* Something wrong; exit to PTRUL1 */
                            return;
                        }
                    }
                }
            }
            /* At INCR8 */
            /* Point to next real CCW; decrement remaining CCW count */
            regs->GR_L(7)+=8;
            regs->GR_L(6)--;
        }

        /* At FRETRCW */
        /* Load pointer to next RCWTASK if any, and FRET the current RCWTASK */
        regs->GR_L(0)=EVM_LH(rcw+RCWCCNT);
        regs->GR_L(1)=rcw;
        rcw=EVM_L(rcw+RCWPNT);
        regs->GR_L(4)=rcw;
        if(ecpsvm_do_fretx(regs,regs->GR_L(1),regs->GR_L(0),EVM_L(dl+4),EVM_L(dl+8))!=0)
        {
            SET_PSW_IA_AND_MAYBE_IP(regs,EVM_L(el+4));       /* Cant do FRETX, exit to UNTFRET */
            return;
        }
    }

    /* After the FRET loop */
    EVM_ST(0,regs->GR_L(10)+IOBCAW);
    B_IOBSPEC2=EVM_IC(regs->GR_L(10)+IOBSPEC2);
    if(B_IOBSPEC2 & IOBUNREL)
    {
        if(!(B_IOBSPEC2 & IOBCLN))
        {
            SET_PSW_IA_AND_MAYBE_IP(regs,EVM_L(el+24));      /* MDISK with reserve/release, exit to ITSAREL */
            return;
        }
    }
    SET_PSW_IA_AND_MAYBE_IP(regs,EVM_L(el+12));      /* Success.  Exit to UNTFRXIT */
    CPASSIST_HIT(FCCWS);
    return;
}

/* Common routine for SCNVU Function */
int  ecpsvm_do_scnvu(REGS *regs,VADR effective_addr1,VADR effective_addr2, U32 vdev)
{
    char buf[256];
    U32  vchix;
    U32  vcuix;
    U32  vdvix;
    VADR vchtbl;
    VADR vch;
    VADR vcu;
    VADR vdv;

    vchtbl=effective_addr1;

    vchix=EVM_LH(vchtbl+((vdev & 0xf00)>>7));   /* Get Index */
    if(vchix & 0x8000)
    {
        DEBUG_CPASSISTX(SCNVU,MSGBUF(buf, "SCNVU Virtual Device %4.4X has no VCHAN block",vdev));
        DEBUG_CPASSISTX(SCNVU,WRMSG(HHC90000, "D", buf));
        return(1);
    }
    vch=EVM_L(effective_addr2)+vchix;

    vcuix=EVM_LH(vch+8+((vdev & 0xf0)>>3));
    if(vcuix & 0x8000)
    {
        DEBUG_CPASSISTX(SCNVU,MSGBUF(buf,"SCNVU Virtual Device %4.4X has no VCU block",vdev));
        DEBUG_CPASSISTX(SCNVU,WRMSG(HHC90000, "D", buf));
        return(1);
    }
    vcu=EVM_L(effective_addr2+4)+vcuix;

    vdvix=EVM_LH(vcu+8+((vdev & 0xf)<<1));
    if(vdvix & 0x8000)
    {
        DEBUG_CPASSISTX(SCNVU,MSGBUF(buf, "SCNVU Virtual Device %4.4X has no VDEV block",vdev));
        DEBUG_CPASSISTX(SCNVU,WRMSG(HHC90000, "D", buf));
        return(1);
    }
    vdv=EVM_L(effective_addr2+8)+vdvix;
    DEBUG_CPASSISTX(SCNVU,MSGBUF(buf, "SCNVU %4.4X : VCH = %8.8X, VCU = %8.8X, VDEV = %8.8X",
                vdev,
                vch,
                vcu,
                vdv));
    DEBUG_CPASSISTX(SCNVU,WRMSG(HHC90000, "D", buf));
    regs->GR_L(6)=vch;
    regs->GR_L(7)=vcu;
    regs->GR_L(8)=vdv;
    regs->psw.cc=0;
    return(0);
}

/* E606 SCNVU Instruction */
/* SCNVU : Scan for Virtual Device blocks */
/* On entry: GR1 contains the virtual device address */
/*-------------------------------------------------------------------*/
/* E606 SCNVU - Scan for Virtual Device Blocks                 [SSE] */
/*-------------------------------------------------------------------*/
DEF_INST(ecpsvm_locate_vblock)
{

    ECPSVM_PROLOG(SCNVU);
    if(ecpsvm_do_scnvu(regs,effective_addr1,effective_addr2,regs->GR_L(1))!=0)
    {
        return;             /* something wrong, let CP do it */
    }

    CPASSIST_HIT(SCNVU);
    BR14;
    return;
}

/* DISP1 Core */
/* rc : 0 - Done */
/* rc : 1 - No-op */
/* rc : 2 - Invoke DISP2 */
int ecpsvm_do_disp1(REGS *regs,VADR dl,VADR el)
{
    char buf[256];
    VADR vmb;
    U32 F_VMFLGS;       /* Aggregate for quick test */
    U32 F_SCHMASK;      /* Flags to test */
    U32 F_SCHMON;       /* Flags allowed on for quick dispatch */
    VADR F_ASYSVM;      /* System VMBLOK */
    VADR SCHDL;         /* SCHDL Exit */

    BYTE B_VMOSTAT;
    BYTE B_VMQSTAT;
    BYTE B_VMRSTAT;

    vmb=regs->GR_L(11);
    DEBUG_CPASSISTX(DISP1,MSGBUF(buf, "DISP1 Data list = %6.6X VM=%6.6X",dl,vmb));
    DEBUG_CPASSISTX(DISP1,WRMSG(HHC90000, "D", buf));
    F_VMFLGS=EVM_L(vmb+VMRSTAT);
    F_SCHMASK=EVM_L(dl+64);
    F_SCHMON=EVM_L(dl+68);
    if((F_VMFLGS & F_SCHMASK) == F_SCHMON)
    {
        DEBUG_CPASSISTX(DISP1,WRMSG(HHC90000, "D", "DISP1 Quick Check complete"));
        return(2);
    }
    else
    {
        DEBUG_CPASSISTX(DISP1,MSGBUF(buf, "DISP1 Quick Check failed : %8.8X != %8.8X",(F_VMFLGS & F_SCHMASK),F_SCHMON));
        DEBUG_CPASSISTX(DISP1,WRMSG(HHC90000, "D", buf));
    }

    F_ASYSVM=EVM_L(ASYSVM);
    if(vmb==F_ASYSVM)
    {
        DEBUG_CPASSISTX(DISP1,WRMSG(HHC90000, "D", "DISP1 VMB is SYSTEM VMBLOCK"));
        return(2);
    }
    SCHDL=EVM_L(el+4);
    B_VMOSTAT=EVM_IC(vmb+VMOSTAT);
    if(!(B_VMOSTAT & VMKILL))
    {
        DEBUG_CPASSISTX(DISP1,WRMSG(HHC90000, "D", "DISP1 Call SCHEDULE because VMKILL not set"));
        SET_PSW_IA_AND_MAYBE_IP(regs, SCHDL);
        return(0);
    }
    B_VMQSTAT=EVM_IC(vmb+VMQSTAT);
    if(!(B_VMQSTAT & VMCFREAD))
    {
        if(B_VMOSTAT & VMCF)
        {
            DEBUG_CPASSISTX(DISP1,WRMSG(HHC90000, "D", "DISP1 Call SCHEDULE because VMKILL & VMCF & !VMCFREAD set"));
            SET_PSW_IA_AND_MAYBE_IP(regs, SCHDL);
            return(0);
        }
    }
    /* At DSP - OFF */
    B_VMQSTAT &= ~VMCFREAD;
    B_VMOSTAT &= ~VMKILL;
    EVM_STC(B_VMQSTAT,vmb+VMQSTAT);
    EVM_STC(B_VMOSTAT,vmb+VMOSTAT);
    B_VMRSTAT=EVM_IC(vmb+VMRSTAT);
    if(B_VMRSTAT & VMLOGOFF)
    {
        DEBUG_CPASSISTX(DISP1,WRMSG(HHC90000, "D", "DISP1 Continue because already logging off"));
        return(2);
    }
    B_VMRSTAT |= VMLOGOFF;
    EVM_STC(B_VMRSTAT,vmb+VMRSTAT);
    SET_PSW_IA_AND_MAYBE_IP(regs, EVM_L(el+0));
    DEBUG_CPASSISTX(DISP1,WRMSG(HHC90000, "D", "DISP1 : Call USOFF"));
    return(0);
}

/* DISP2 Core */
int ecpsvm_do_disp2(REGS *regs,VADR dl,VADR el)
{
    char buf[1024];
    VADR vmb;   /* Current VMBLOK */
    VADR svmb;  /* ASYSVM */
    VADR runu;  /* RUNUSER */
    VADR lastu; /* LASTUSER */
    VADR F_TRQB;
    VADR F_CPEXB;
    VADR F,B;
    VADR traceptr;
    U16  HW1;
    U32  FW1;
    U64  DW1;
    U32  CPEXBKUP[15];  /* CPEXBLOK Regs backup except GPR15 which is useless */
    VADR F_ECBLOK;      /* Pointer to user's EC block for extended VM */
    VADR F_CPEXADD;
    U32  F_QUANTUM;
    REGS wregs; /* Work REGS structure of PSW manipulation for Virtual PSW */
    REGS rregs; /* Work REGS structure of PSW manipulation for Real    PSW */
    int i;

    BYTE B_VMDSTAT,B_VMRSTAT,B_VMESTAT,B_VMPSTAT,B_VMMCR6,B_MICVIP;
    BYTE B_VMOSTAT,B_VMPEND;
    VADR F_MICBLOK;
    U32 F_VMIOINT,F_VMPXINT;
    U32 F_VMVCR0;
    U32 NCR0,NCR1;
    BYTE *work_p;

    vmb=regs->GR_L(11);
    DEBUG_CPASSISTX(DISP2,MSGBUF(buf,"DISP2 Data list=%6.6X VM=%6.6X",dl,vmb));
    DEBUG_CPASSISTX(DISP2,WRMSG(HHC90000, "D", buf));
    CHARGE_STOP(vmb);
    if(EVM_IC(XTENDLOCK) == XTENDLOCKSET)
    {
        DEBUG_CPASSISTX(DISP2,WRMSG(HHC90000, "D", "DISP2 Exit 8 : System extending"));
        /* System in Extend process */
        SET_PSW_IA_AND_MAYBE_IP(regs, EVM_L(el+8));
        return(0);
    }
    if(EVM_IC(APSTAT2) & CPMCHLK)
    {
        DEBUG_CPASSISTX(DISP2,WRMSG(HHC90000, "D", "DISP2 Exit 8 : MCH Recovery"));
        /* Machine Check recovery in progress */
        SET_PSW_IA_AND_MAYBE_IP(regs, EVM_L(el+8));
        return(0);
    }
    svmb=EVM_L(ASYSVM);
    /* Check IOB/TRQ for dispatch */
    /* at DMKDSP CKCPSTAK */
    F_TRQB=EVM_L(dl+8);
    if(F_TRQB!=dl)
    {
        DEBUG_CPASSISTX(DISP2,MSGBUF(buf, "DISP2 TRQ/IOB @ %6.6X Exit being routed",F_TRQB));
        DEBUG_CPASSISTX(DISP2,WRMSG(HHC90000, "D", buf));
        /* We have a TRQ/IOB */
        /* Update stack */
        /* at DMKDSP PROCIOB */
        F=EVM_L(F_TRQB+8);
        B=EVM_L(F_TRQB+12);
        EVM_ST(F,B+8);
        EVM_ST(B,F+12);
        /* Get VMBLOK Responsible for this block */
        vmb=EVM_L(F_TRQB+0x18);
        /* Update stack count for the VMBLOK */
        HW1=EVM_LH(vmb+VMSTKCNT);
        HW1--;
        EVM_STH(HW1,vmb+VMSTKCNT);
        /* Start charging user for processor time */
        CHARGE_START(vmb);
        EVM_ST(vmb,STACKVM);
        /* Update registers for TRQ/IOB exit */
        regs->GR_L(10)=F_TRQB;
        regs->GR_L(11)=vmb;
        regs->GR_L(12)=EVM_L(F_TRQB+0x1C);
        SET_PSW_IA_AND_MAYBE_IP(regs, regs->GR_L(12));
        DEBUG_CPASSISTX(DISP2,MSGBUF(buf, "DISP2 TRQ/IOB @ %6.6X IA = %6.6X",F_TRQB,regs->GR_L(12)));
        DEBUG_CPASSISTX(DISP2,WRMSG(HHC90000, "D", buf));
        return(0);
    }
    /* Check CPEX BLOCK for dispatch */
    /* at DMKDSP WINDOW */
    F_CPEXB=EVM_L(dl+0);
    if(F_CPEXB!=dl)
    {
        DEBUG_CPASSISTX(DISP2,MSGBUF(buf, "DISP2 CPEXBLOK Exit being routed CPEX=%6.6X",F_CPEXB));
        DEBUG_CPASSISTX(DISP2,WRMSG(HHC90000, "D", buf));
        /* We have a CPEXBLOCK */
        /* Update stack */
        F=EVM_L(F_CPEXB+0);
        B=EVM_L(F_CPEXB+4);
        EVM_ST(F,B+0);
        EVM_ST(B,F+4);
        vmb=EVM_L(F_CPEXB+0x10+(11*4));
        HW1=EVM_LH(vmb+VMSTKCNT);
        HW1--;
        EVM_STH(HW1,vmb+VMSTKCNT);
        CHARGE_START(vmb);
        EVM_ST(vmb,STACKVM);
        /* Copy CPEXBLOCK Contents, and attempt FRET */
        /* If fret fails, use exit #12 */
        for(i=0;i<15;i++)
        {
            CPEXBKUP[i]=EVM_L(F_CPEXB+0x10+(i*4));
        }
        F_CPEXADD=EVM_L(F_CPEXB+0x0C);
        if(ecpsvm_do_fretx(regs,F_CPEXB,10,EVM_L(dl+28),EVM_L(dl+32))!=0)
        {
            DEBUG_CPASSISTX(DISP2,MSGBUF(buf, "DISP2 CPEXBLOK CPEX=%6.6X Fret Failed",F_CPEXB));
            DEBUG_CPASSISTX(DISP2,WRMSG(HHC90000, "D", buf));
            regs->GR_L(0)=10;
            regs->GR_L(1)=F_CPEXB;
            for(i=2;i<12;i++)
            {
                regs->GR_L(i)=CPEXBKUP[i];
            }
            /* Save GPRS 12-1 (wraping) in DSPSAVE (datalist +40) */
            /* So that LM 12,1,DSPSAVE in DMKDSP works after call to DMKFRET */
            /* 2017-01-27 Fix order of EVM_ST operands to prevent stg overlays */
            EVM_ST(CPEXBKUP[12],dl+40);
            EVM_ST(CPEXBKUP[13],dl+44);
            EVM_ST(CPEXBKUP[14],dl+48);
            EVM_ST(EVM_L(F_CPEXB+12),dl+52); /* DSPSAVE + 12 = CPEXADD */
            EVM_ST(CPEXBKUP[0],dl+56);
            EVM_ST(CPEXBKUP[1],dl+60);  /* Note : DMKDSP Is wrong -  SCHMASK is at +64 (not +60) */
            /* End of 2017-01-27 */

            /* Upon taking this exit, GPRS 12-15 are same as entry */
            SET_PSW_IA_AND_MAYBE_IP(regs, EVM_L(el+12));
            return(0);
        }
        for(i=0;i<15;i++)
        {
            regs->GR_L(i)=CPEXBKUP[i];
        }
        regs->GR_L(15)=F_CPEXADD;
        SET_PSW_IA_AND_MAYBE_IP(regs, F_CPEXADD);
        DEBUG_CPASSISTX(DISP2,MSGBUF(buf, "DISP2 CPEXBLOK CPEX=%6.6X IA=%6.6X",F_CPEXB,F_CPEXADD));
        DEBUG_CPASSISTX(DISP2,WRMSG(HHC90000, "D", buf));
        return(0);  /* CPEXBLOCK Branch taken */
    }
    /* Check for a USER run */
    /* AT DMKDSP - DONE */
    if(EVM_IC(CPSTAT2) & CPSHRLK)
    {
        DEBUG_CPASSISTX(DISP2,WRMSG(HHC90000, "D", "DISP2 Exit 24 : CPSHRLK Set in CPSTAT2"));
        SET_PSW_IA_AND_MAYBE_IP(regs, EVM_L(el+24));  /* IDLEECPS */
        return(0);
    }
    /* Scan Scheduler IN-Q */
    DEBUG_CPASSISTX(DISP2,WRMSG(HHC90000, "D", "DISP2 : Scanning Scheduler IN-Queue"));
    FW1=EVM_L(dl+24);
    for(vmb=EVM_L(FW1);vmb!=FW1;vmb=EVM_L(vmb))
    {
        /* 2017-02-04 Check for V PSW wait */
        if(EVM_LH(vmb+VMPSW) & 0x0002)
        {
            DEBUG_CPASSISTX(DISP2,MSGBUF(buf, "DISP2 : VMB @ %6.6X Not eligible : User in virtual PSW wait",vmb));
            DEBUG_CPASSISTX(DISP2,WRMSG(HHC90000, "D", buf));
            continue;
        }
        /* end of 2017-02-04 */

        if(!(EVM_IC(vmb+VMDSTAT) & VMRUN))
        {
            DEBUG_CPASSISTX(DISP2,MSGBUF(buf, "DISP2 : VMB @ %6.6X Not eligible : VMRUN not set",vmb));
            DEBUG_CPASSISTX(DISP2,WRMSG(HHC90000, "D", buf));
            continue;
        }
        if(EVM_IC(vmb+VMRSTAT) & VMCPWAIT)
        {
            DEBUG_CPASSISTX(DISP2,MSGBUF(buf, "DISP2 : VMB @ %6.6X Not eligible : VMCPWAIT set",vmb));
            DEBUG_CPASSISTX(DISP2,WRMSG(HHC90000, "D", buf));
            continue;
        }
        if(EVM_IC(vmb+VMNOECPS))
        {
            DEBUG_CPASSISTX(DISP2,MSGBUF(buf, "DISP2 : Exit 20 : VMB @ %6.6X Has VMNOECPS Set to %2.2X",vmb,EVM_IC(vmb+VMNOECPS)));
            DEBUG_CPASSISTX(DISP2,WRMSG(HHC90000, "D", buf));
            regs->GR_L(1)=vmb;
            regs->GR_L(11)=EVM_L(ASYSVM);
            SET_PSW_IA_AND_MAYBE_IP(regs, EVM_L(el+20));  /* FREELOCK */
            return(0);
        }
        DEBUG_CPASSISTX(DISP2,MSGBUF(buf, "DISP2 : VMB @ %6.6X Will now be dispatched",vmb));
        DEBUG_CPASSISTX(DISP2,WRMSG(HHC90000, "D", buf));
        runu=EVM_L(RUNUSER);
        F_QUANTUM=EVM_L(QUANTUM);
        if(vmb!=runu)
        {
            /* User switching */
            /* DMKDSP - FNDUSRD */
            DEBUG_CPASSISTX(DISP2,MSGBUF(buf, "DISP2 : User switch from %6.6X to %6.6X",runu,vmb));
            DEBUG_CPASSISTX(DISP2,WRMSG(HHC90000, "D", buf));
            runu=EVM_L(RUNUSER);
            EVM_STC(EVM_IC(runu+VMDSTAT) & ~VMDSP,runu+VMDSTAT);
            lastu=EVM_L(LASTUSER);
            DEBUG_CPASSISTX(DISP2,MSGBUF(buf, "DISP2 : RUNU=%6.6X, LASTU=%6.6X",runu,lastu));
            DEBUG_CPASSISTX(DISP2,WRMSG(HHC90000, "D", buf));
            if(lastu!=svmb && lastu!=vmb)
            {
                if(EVM_IC(lastu+VMOSTAT) & VMSHR)       /* Running shared sys */
                {
                    DEBUG_CPASSISTX(DISP2,MSGBUF(buf, "DISP2 : Exit 16 : LASTU=%6.6X has shared sys & LCSHPG not impl",lastu));
                    DEBUG_CPASSISTX(DISP2,WRMSG(HHC90000, "D", buf));
                    CHARGE_START(lastu);
                    /* LCSHRPG not implemented yet */
                    regs->GR_L(10)=vmb;
                    regs->GR_L(11)=lastu;
                    SET_PSW_IA_AND_MAYBE_IP(regs, EVM_L(el+16));
                    return(0);
                    /* A CHARGE_STOP(runu) is due when LCSHRPG is implemented */
                }
            }
        }
        if(vmb!=runu || (vmb==runu && (F_QUANTUM & 0x80000000)))
        {
            DEBUG_CPASSISTX(DISP2,WRMSG(HHC90000, "D", "DISP2 : Restarting Time Slice"));
            F_QUANTUM=EVM_L(dl+16);
            if(EVM_IC(vmb+VMQLEVEL) & VMCOMP)
            {
                F_QUANTUM <<= 2;
            }
        }
        EVM_ST(F_QUANTUM,INTTIMER);
        CHARGE_START(vmb);
        EVM_ST(vmb,LASTUSER);
        EVM_ST(vmb,RUNUSER);

        /***  Prepare to run a user ***/

        /* Cache some important VMBLOK flag bytes */
        B_VMDSTAT=EVM_IC(vmb+VMDSTAT);
        B_VMRSTAT=EVM_IC(vmb+VMRSTAT);
        B_VMPSTAT=EVM_IC(vmb+VMPSTAT);
        B_VMESTAT=EVM_IC(vmb+VMESTAT);
        B_VMOSTAT=EVM_IC(vmb+VMOSTAT);
        B_VMPEND =EVM_IC(vmb+VMPEND);
        B_VMMCR6=EVM_IC(vmb+VMMCR6);
        F_MICBLOK=EVM_L(vmb+VMMCR6) & ADDRESS_MAXWRAP(regs);

        /* LOAD FPRS */
        regs->FPR_L(0) = EVM_LD(vmb+VMFPRS+0);
        regs->FPR_L(2) = EVM_LD(vmb+VMFPRS+8);
        regs->FPR_L(4) = EVM_LD(vmb+VMFPRS+16);
        regs->FPR_L(6) = EVM_LD(vmb+VMFPRS+24);

        INITPSEUDOREGS(wregs);
        work_p=MADDR(vmb+VMPSW,0,regs,USE_REAL_ADDR,0);
        ARCH_DEP(load_psw) (&wregs,work_p);    /* Load user's Virtual PSW in work structure */
        MAYBE_SET_PSW_IA_FROM_IP(&wregs);

        /* Build REAL PSW */
        INITPSEUDOREGS(rregs);
        /* Copy IAR */
        SET_PSW_IA_AND_MAYBE_IP(&rregs, wregs.psw.IA);
        /* Copy CC, PSW KEYs and PGM Mask */
        rregs.psw.cc=wregs.psw.cc;
        rregs.psw.pkey=wregs.psw.pkey;
        /* Indicate Translation + I/O + Ext + Ecmode + Problem + MC */
        rregs.psw.sysmask=0x07; /* I/O + EXT + Trans */
        rregs.psw.states = BIT(PSW_EC_BIT)         /* ECMODE */
                         | BIT(PSW_PROB_BIT)       /* Problem state */
                         | BIT(PSW_MACH_BIT);      /* MC Enabled */
        rregs.psw.intcode=0;    /* Clear intcode */
        rregs.psw.progmask=wregs.psw.progmask;

        NCR0=EVM_L(CPCREG0);    /* Assume for now */
        NCR1=EVM_L(vmb+VMSEG);  /* Ditto          */

        /* Disable ECPS:VM in VM-REAL CR6 For now */
        B_VMMCR6&=~(VMMSHADT|VMMPROB|VMMNOSK|VMMFE);

        /* We load VMECEXT Even if it's not a ECMODE VM */
        /* in which case F_ECBLOK is also Virtual CR0   */

        F_ECBLOK=EVM_L(vmb+VMECEXT);

        /* ECMODE VM ? */
        if(B_VMPSTAT & VMV370R)
        {
            DEBUG_CPASSISTX(DISP2,MSGBUF(buf, "DISP2 : VMB @ %6.6X has ECMODE ON",vmb));
            DEBUG_CPASSISTX(DISP2,WRMSG(HHC90000, "D", buf));
            /* Is this an ECMODE PSW Machine ? */
            if(B_VMESTAT & VMEXTCM)
            {
                if((B_VMESTAT & (VMINVSEG|VMNEWCR0)) == (VMINVSEG|VMNEWCR0))
                {
                    /* CP Say this is NOT good */
                    /* Take exit 28 */
                    WRMSG(HHC01700,"W");
                    SET_PSW_IA_AND_MAYBE_IP(regs, EVM_L(el+28));
                    return(0);
                }
                /* Check 3rd level translation */
                if(wregs.psw.sysmask & 0x04)
                {
                    NCR0=EVM_L(F_ECBLOK+EXTSHCR0);
                    NCR1=EVM_L(F_ECBLOK+EXTSHCR1);
                    B_VMMCR6|=VMMSHADT;   /* re-enable Shadow Table management in CR6 */
                }
            }

        }
        /* Invalidate Shadow Tables if necessary */
        /* 2017-01-29 if statement corrected */
        if((B_VMESTAT & (VMINVPAG | VMSHADT)) == (VMINVPAG|VMSHADT))
        {
            DEBUG_CPASSISTX(DISP2,MSGBUF(buf, "DISP2 : VMB @ %6.6X Refusing to simulate DMKVATAB",vmb));
            DEBUG_CPASSISTX(DISP2,WRMSG(HHC90000, "D", buf));
            /* Really looks like DMKVATAB is a huge thing to simulate */
            /* My belief is that the assist can't handle this one     */
            /* Return to caller as a NO-OP on this one                */
            return(1);
            /* ecpsvm_inv_shadtab_pages(regs,vmb); */
        }
        B_VMESTAT&=~VMINVPAG;
        B_VMDSTAT|=VMDSP;
        /* Test for CPMICON in DMKDSP useless here */
        /* if CPMICON was off, we would have never */
        /* been called anyway                      */
        if(F_MICBLOK!=0)        /* That is SET ASSIST ON */
        {
            B_MICVIP=0;
            /* Check tracing (incompatible with assist) */
            if(!(EVM_IC(vmb+VMTRCTL) & (VMTRSVC|VMTRPRV|VMTRBRIN)))
            {
                B_VMMCR6|=VMMFE;
                if(B_VMOSTAT & VMSHR)
                {
                    /* Cannot allow ISK/SSK in shared sys VM */
                    B_VMMCR6|=VMMNOSK;
                }
                if(PROBSTATE(&wregs.psw))
                {
                    B_VMMCR6|=VMMPROB;
                }
                /* Set MICPEND if necessary */
                /* (assist stuff to ensure LPSW/SSM/SVC sim */
                /* does not re-enable VPSW when an interrupt */
                /* is pending)                               */
                while(1)
                {
                    B_MICVIP=0;
                    F_VMIOINT=EVM_LH(vmb+VMIOINT);
                    if(EVM_LH(vmb+VMIOINT)!=0)
                        {
                        F_VMIOINT<<=16;
                        if(B_VMESTAT & VMEXTCM)
                        {
                            if(F_VMIOINT&=EVM_L(F_ECBLOK))
                            {
                                B_MICVIP|=0x80;
                                break;
                            }
                        }
                        else
                        {
                            B_MICVIP|=0x80;
                            break;
                        }
                    }
                    if(B_VMESTAT & VMEXTCM)
                    {
                        if(B_VMPEND & VMPGPND)
                        {
                            B_MICVIP|=0x80;
                        }
                    }
                    if(B_VMPSTAT & VMV370R)
                    {
                        F_VMVCR0=EVM_L(F_ECBLOK+0);
                    }
                    else
                    {
                        F_VMVCR0=F_ECBLOK;
                    }
                    for(F_VMPXINT=EVM_L(vmb+VMPXINT);F_VMPXINT;F_VMPXINT=EVM_L(F_VMPXINT)) /* XINTNEXT at +0 */
                    {
                        if(F_VMVCR0 & EVM_LH(F_VMPXINT+10))
                        {
                            B_MICVIP|=0x80;
                            break;
                        }
                    }
                    break;      /* Terminate dummy while loop */
                } /* While dummy loop for MICPEND */
            } /* if(Not tracing) */
            EVM_STC(B_MICVIP,F_MICBLOK+8);      /* Save new MICVIP */
        } /* if(F_MICBLOCK!=0) */
        /* If an Extended VM, Load CRs 4-13 */
        /* CR6 Will be overwritten in a second */
        if(B_VMPSTAT & VMV370R)
        {
            for(i=4;i<14;i++)
            {
                regs->CR_L(i)=EVM_L(F_ECBLOK+(i*4));
            }
        }
        /* Update VMMICRO */
        EVM_STC(B_VMMCR6,vmb+VMMCR6);
        /* Update PER Control */
        if(EVM_IC(vmb+VMTRCTL) & VMTRPER)
        {
            DEBUG_CPASSISTX(DISP2,WRMSG(HHC90000, "D", "DISP2 : PER ON"));
            FW1=EVM_L(vmb+VMTREXT);
            regs->CR_L( 9)=EVM_L(FW1+0x1C);
            regs->CR_L(10)=EVM_L(FW1+0x20);
            regs->CR_L(11)=EVM_L(FW1+0x24);
            rregs.psw.sysmask |= 0x40;  /* PER Mask in PSW */
        }
        /* Update CR6 */
        regs->CR_L(6)=EVM_L(vmb+VMMCR6);
        /* Insure proper re-entry */
        EVM_ST(0,STACKVM);
        /* Update PROBLEM Start time */
        DW1=EVM_LD(vmb+VMTMOUTQ);
        EVM_STD(DW1,PROBSTRT);

        /* Checkpoint Interval Timer */
        FW1=EVM_L(INTTIMER);
        EVM_ST(FW1,QUANTUM);

        /* Update REAL CR0/CR1 */
        regs->CR_L(0)=NCR0;
        regs->CR_L(1)=NCR1;

        /* Indicate RUNNING a user */
        EVM_STC(CPRUN,CPSTATUS);

        /* Update real PSW with working PSW */

        /* Update regs */
        for(i=0;i<16;i++)
        {
            regs->GR_L(i)=EVM_L(vmb+VMGPRS+(i*4));
        }
        /* Clear I/O Old PSW Byte 0 */
        EVM_STC(0,IOOPSW);
        /* Issue PTLB if necessary */
        if(EVM_IC(APSTAT2) & CPPTLBR)
        {
            DEBUG_CPASSISTX(DISP2,WRMSG(HHC90000, "D", "DISP2 : Purging TLB"));
            ARCH_DEP(purge_tlb)(regs);
            EVM_STC(EVM_IC(APSTAT2) & ~CPPTLBR,APSTAT2);
        }

        /* Update cached VMBLOK flags */
        EVM_STC(B_VMDSTAT,vmb+VMDSTAT);
        EVM_STC(B_VMRSTAT,vmb+VMRSTAT);
        EVM_STC(B_VMESTAT,vmb+VMESTAT);
        EVM_STC(B_VMPSTAT,vmb+VMPSTAT);
        EVM_STC(B_VMOSTAT,vmb+VMOSTAT);
        work_p=MADDR(vmb+VMPSW,USE_REAL_ADDR,regs,ACCTYPE_WRITE,0); \
        ARCH_DEP(store_psw) (&wregs,work_p);


        /* Stop charging current VM Block for Supervisor time */
        CHARGE_STOP(vmb);

        /* Rest goes for problem state */
        SPT(vmb+VMTMOUTQ);
        /* Save RUNCR0, RUNCR1 & RUNPSW */
        /* Might be used by later CP Modules (including DMKPRV) */
        EVM_ST(NCR0,RUNCR0);
        EVM_ST(NCR1,RUNCR1);
        work_p=MADDR(RUNPSW,USE_REAL_ADDR,regs,ACCTYPE_WRITE,0); \
        ARCH_DEP(store_psw) (&rregs,work_p);

        /* Write the trace entry before the user's PSW key is loaded into the real PSW */
        if (EVM_IC(TRACFLG2) & TRAC0A)
        {
            traceptr=ecpsvm_get_trace_entry(regs);
            EVM_ST(0,traceptr);
            EVM_STC(TRCRUN,traceptr);
            EVM_ST(vmb,traceptr+4);
            EVM_STD(EVM_LD(RUNPSW),traceptr+8);
        }

        DEBUG_CPASSISTX(DISP2,MSGBUF(buf, "DISP2 : Entry Real PSW="));
        DEBUG_CPASSISTX(DISP2,display_psw(regs, &buf[strlen(buf)], sizeof(buf)-(int)strlen(buf)));
        DEBUG_CPASSISTX(DISP2,WRMSG(HHC90000, "D", buf));
        ARCH_DEP(load_psw) (regs,work_p);
        DEBUG_CPASSISTX(DISP2,MSGBUF(buf, "DISP2 : VMB @ %6.6X Now being dispatched",vmb));
        DEBUG_CPASSISTX(DISP2,WRMSG(HHC90000, "D", buf));
        DEBUG_CPASSISTX(DISP2,MSGBUF(buf, "DISP2 : Real PSW="));
        DEBUG_CPASSISTX(DISP2,display_psw(regs, &buf[strlen(buf)], sizeof(buf)-(int)strlen(buf)));
        DEBUG_CPASSISTX(DISP2,WRMSG(HHC90000, "D", buf));
        DEBUG_CPASSISTX(DISP2,MSGBUF(buf, "DISP2 : Virtual PSW="));
        DEBUG_CPASSISTX(DISP2,display_psw(&wregs, &buf[strlen(buf)], sizeof(buf)-(int)strlen(buf)));
        DEBUG_CPASSISTX(DISP2,WRMSG(HHC90000, "D", buf));

        /* TEST */
        ARCH_DEP(purge_tlb)(regs);
        SET_IC_MASK(regs);
        SET_AEA_MODE(regs);
        SET_AEA_COMMON(regs);
        MAYBE_SET_PSW_IA_FROM_IP(regs);
        /* Dispatch..... */
        DEBUG_CPASSISTX(DISP2,MSGBUF(buf, "DISP2 - Dispatch...\n"));
        DEBUG_CPASSISTX(DISP2,display_gregs(regs, &buf[strlen(buf)], sizeof(buf)-(int)strlen(buf), "HHC90000D "));
        DEBUG_CPASSISTX(DISP2,STRLCAT( buf, "\n" ));
        DEBUG_CPASSISTX(DISP2,display_cregs(regs, &buf[strlen(buf)], sizeof(buf)-(int)strlen(buf), "HHC90000D "));
        DEBUG_CPASSISTX(DISP2,WRMSG(HHC90000, "D", buf));
        return(2);      /* OK - Perform INTCHECK */
    }
    /* Nothing else to do - wait state */
    DEBUG_CPASSISTX(DISP2,WRMSG(HHC90000, "D", "DISP2 : Nothing to dispatch - IDLEECPS"));
    SET_PSW_IA_AND_MAYBE_IP(regs, EVM_L(el+24));      /* IDLEECPS */
    return(0);
}

/* E607 DISP1 Instruction */
/* DISP1 : Early tests part 2 */
/*   DISP1 Checks if the user is OK to run */
/*         early tests part 1 already done by DISP0 */
/*-------------------------------------------------------------------*/
/* E607 DISP1 - Dispatch 1                                     [SSE] */
/*-------------------------------------------------------------------*/
DEF_INST(ecpsvm_disp1)
{

    ECPSVM_PROLOG(DISP1);
    switch(ecpsvm_do_disp1(regs,effective_addr1,effective_addr2))
    {
        case 0: /* Done */
            CPASSIST_HIT(DISP1);
            return;
        case 1: /* No-op */
            break;
        case 2: /* Call DISP2 - INTCHECK NOT needed */
            switch(ecpsvm_do_disp2(regs,effective_addr1,effective_addr2))
            {
                case 0:
                    CPASSIST_HIT(DISP1);
                    return;
                case 1:
                    return;
                case 2:
                    CPASSIST_HIT(DISP1);
                    RETURN_INTCHECK(regs);
                default:
                    break;
            }
            return;
        default:
            return;
    }
}

static int ecpsvm_int_lra(REGS *regs,VADR pgadd,RADR *raddr)
{
    int cc;
    cc = ARCH_DEP(translate_addr) (pgadd , USE_PRIMARY_SPACE, regs, ACCTYPE_LRA);
    *raddr = regs->dat.raddr;
    return cc;
}

/* TRANBRNG/TRANLOCK Common code */
static int ecpsvm_tranbrng(REGS *regs,VADR cortabad,VADR pgadd,RADR *raddr)
{
    char buf[256];
    int cc;
    int corcode;
#if defined(FEATURE_2K_STORAGE_KEYS)
    RADR pg1,pg2;
#endif
    VADR cortab;
    cc=ecpsvm_int_lra(regs,pgadd,raddr);
    if(cc!=0)
    {
        DEBUG_CPASSISTX(TRBRG,MSGBUF(buf, "Tranbring : LRA cc = %d",cc));
        DEBUG_CPASSISTX(TRBRG,WRMSG(HHC90000, "D", buf));
        return(1);
    }
    /* Get the core table entry from the Real address */
    cortab=EVM_L( cortabad );
    cortab+=((*raddr) & 0xfff000) >> 8;
    corcode=EVM_IC(cortab+8);
    if(!(corcode & 0x08))
    {
        DEBUG_CPASSISTX(TRBRG,MSGBUF(buf, "Page not shared - OK %d",cc));
        DEBUG_CPASSISTX(TRBRG,WRMSG(HHC90000, "D", buf));
        return(0);      /* Page is NOT shared.. All OK */
    }
#if defined(FEATURE_2K_STORAGE_KEYS)
    pg1=(*raddr & 0xfff000);
    pg2=pg1+0x800;
    DEBUG_CPASSISTX(TRBRG,MSGBUF(buf, "Checking 2K Storage keys @"F_RADR" & "F_RADR"",pg1,pg2));
    DEBUG_CPASSISTX(TRBRG,WRMSG(HHC90000, "D", buf));
    if(0
        || (ARCH_DEP( get_storage_key )( pg1 ) & STORKEY_CHANGE)
        || (ARCH_DEP( get_storage_key )( pg2 ) & STORKEY_CHANGE)
    )
#else
    DEBUG_CPASSISTX(TRBRG,MSGBUF(buf, "Checking 4K Storage keys @"F_RADR,*raddr));
    DEBUG_CPASSISTX(TRBRG,WRMSG(HHC90000, "D", buf));
    if (ARCH_DEP( get_storage_key )( *raddr ) & STORKEY_CHANGE)
#endif
    {
        DEBUG_CPASSISTX(TRBRG,WRMSG(HHC90000, "D", "Page shared and changed"));
        return(1);      /* Page shared AND changed */
    }
    DEBUG_CPASSISTX(TRBRG,WRMSG(HHC90000, "D", "Page shared but not changed"));
    return(0);  /* All done */
}

/* E608 TRBRG Instruction */
/* TRBRG : Translate a page address */
/* TRBRG D1(R1,B1),D2(R2,B2) */
/* 1st operand : Coretable address */
/* 2nd operand : Virtual address */
/* Note : CR1 Contains the relevant segment table */
/*        pointers                                */
/* The REAL address is resolved. If the page is flagged */
/* as shared in the core table, the page is checked for */
/* the change bit                                       */
/* If no unusual condition is detected, control is returned */
/* to the address in GPR 14. Otherwise, TRBRG is a no-op */
/*-------------------------------------------------------------------*/
/* E608 TRBRG - Translate Page Address                         [SSE] */
/*-------------------------------------------------------------------*/
DEF_INST(ecpsvm_tpage)
{
    int rc;
    RADR raddr;
    ECPSVM_PROLOG(TRBRG);
    DEBUG_CPASSISTX(TRBRG,WRMSG(HHC90000, "D", "TRANBRNG"));
    rc=ecpsvm_tranbrng(regs,effective_addr1,regs->GR_L(1),&raddr);
    if(rc)
    {
        DEBUG_CPASSISTX(TRBRG,WRMSG(HHC90000, "D", "TRANBRNG - Back to CP"));
        return; /* Something not right : NO OP */
    }
    regs->psw.cc=0;
    regs->GR_L(2)=raddr;
    SET_PSW_IA_AND_MAYBE_IP(regs, effective_addr2);
    CPASSIST_HIT(TRBRG);
    return;
}

/* E609 TRLOK Instruction */
/* TRLOK : Translate a page address and lock */
/* TRLOK D1(R1,B1),D2(R2,B2) */
/* See TRBRG. */
/* If sucessfull, the page is also locked in the core table */
/*-------------------------------------------------------------------*/
/* E609 TRLOK - Translate Page Address and Lock                [SSE] */
/*-------------------------------------------------------------------*/
DEF_INST(ecpsvm_tpage_lock)
{
    int rc;
    RADR raddr;
    ECPSVM_PROLOG(TRLOK);
    DEBUG_CPASSISTX(TRLOK,WRMSG(HHC90000, "D", "TRANLOCK"));
    rc=ecpsvm_tranbrng(regs,effective_addr1,regs->GR_L(1),&raddr);
    if(rc)
    {
        DEBUG_CPASSISTX(TRLOK,WRMSG(HHC90000, "D", "TRANLOCK - Back to CP"));
        return; /* Something not right : NO OP */
    }
    /* Lock the page in Core Table */
    ecpsvm_lockpage1(regs,effective_addr1,raddr);
    regs->psw.cc=0;
    regs->GR_L(2)=raddr;
    SET_PSW_IA_AND_MAYBE_IP(regs, effective_addr2);
    CPASSIST_HIT(TRLOK);
    return;
}

/* Common routine for E60A VIST and E60B VIPT */
/* This routine invalidates one complete shadow page table */
/* On entry:  archtect points to the ARCHTECT structure within DMKVAT */
/*            pindex contains the index value for the correct ARCHTECT structure to use */
/*            General register 6 points to the shadow segment table entry pointing to the page table to invalidate */
void ecpsvm_zappage(REGS *regs, VADR archtect, VADR pindex)
{
    VADR ptr_segtabl;
    VADR ptr_pagetabl;
    U16 page_table_len;
    BYTE page_invalid_bit_fmt;

    /* get segment table address.  Then isolate the page table pointer; if zero then */
    /* this page table is unused and we can exit with invalidation completed   */
    ptr_segtabl=regs->GR_L(6);

    ptr_pagetabl=EVM_L(ptr_segtabl) & 0x00FFFFF8;
    if (ptr_pagetabl==0)
    {
        return;
    }

    /* Since the segment table entry contains a pointer to a page table, mark the segment table entry as */
    /* valid.  Then obtain the format of the page invalid bit from the ARCHTECT table as well as the     */
    /* the page table length.                                                                            */
    EVM_ST(EVM_L(ptr_segtabl) & 0xFFFFFFFE, ptr_segtabl);
    page_invalid_bit_fmt=EVM_IC(archtect+pindex+PINVBIT);
    page_table_len=EVM_LH(archtect+pindex+PAGTLEN);

    /* fill the entire page table with the page invalid bit format */
    memset((char*)regs->mainstor+ptr_pagetabl, page_invalid_bit_fmt, page_table_len);
    return;
}

/* E60A VIST Instruction */
/* Invalidate Shadow Segment Table */
/* VIST D1(R1,B1),D2(R2,B2) */
/* 1st operand : Address of ARCHTECT structure in DMKVAT containing segment and page table format information */
/* 2nd Operand : Address of EXTSHCR1 in ECBLOK  */
/* On entry:  General register 9 contains the index into the ARCHTECT structure */
/*            General register 8 contains the return address of the caller upon successful completion */
/*-------------------------------------------------------------------*/
/* E60A VIST - Invalidate Shadow Segment Table                 [SSE] */
/*-------------------------------------------------------------------*/
DEF_INST(ecpsvm_inval_segtab)
{
    char buf[256];
    VADR ptr_segtabl;
    VADR ptr_segtabl_end;
    VADR seg;
    VADR pindex;
    U16 segment_table_len;

    ECPSVM_PROLOG(VIST);

    /* get shadow segment table origin and table length from ECBLOK */
    ptr_segtabl=EVM_L(effective_addr2) & 0x00FFFFF8;
    segment_table_len=EVM_LH(effective_addr2+4);
    DEBUG_CPASSISTX(VIST,MSGBUF(buf, "VIST  : Seg table addr = %x,  len = %x",ptr_segtabl,segment_table_len));
    DEBUG_CPASSISTX(VIST,WRMSG(HHC90000, "D", buf));

    ptr_segtabl_end=ptr_segtabl+segment_table_len;
    pindex=regs->GR_L(9);

    /* Invalidate this segment, then invalidate the page table */
    for (seg=ptr_segtabl; seg<ptr_segtabl_end; seg+=4)
    {
        EVM_ST(EVM_L(seg) | 0x00000001, seg);
        regs->GR_L(6)=seg;
        ecpsvm_zappage(regs, effective_addr1, pindex);
    }

    /* Indicate Purge TLB required and return via GR8 */
    EVM_STC(EVM_IC(APSTAT2) | CPPTLBR,APSTAT2);
    SET_PSW_IA_AND_MAYBE_IP(regs, regs->GR_L(8));
    CPASSIST_HIT(VIST);
    return;
}

/* E60B VIPT Instruction */
/* Invalidate Shadow Page Table */
/* VIPT D1(R1,B1),D2(R2,B2) */
/* 1st operand : Address of ARCHTECT structure in DMKVAT containing segment and page table format information */
/* 2nd Operand : Index value to be added to ARCHTECT address for the segment/page format in use  */
/* On entry:  R6 -> segment table entry for the page table */
/*-------------------------------------------------------------------*/
/* E60B VIPT - Invalidate Shadow Page Table                    [SSE] */
/*-------------------------------------------------------------------*/
DEF_INST(ecpsvm_inval_ptable)
{
    ECPSVM_PROLOG(VIPT);
    ecpsvm_zappage(regs, effective_addr1, effective_addr2);
    BR14;
    CPASSIST_HIT(VIPT);
    return;

}
/* E60C DFCCW Instruction */
/* DFCCW : Decode First CCW */
/* Entry and exit lists are detailed in function "ecpsvm_do_deccw1"  */
/*-------------------------------------------------------------------*/
/* E60C DFCCW - Decode First CCW                               [SSE] */
/*-------------------------------------------------------------------*/
DEF_INST(ecpsvm_decode_first_ccw)
{

RADR raddr;

    ECPSVM_PROLOG(DFCCW);

    if(ecpsvm_tranbrng(regs,effective_addr1,regs->GR_L(9),&raddr)!=0)
    {
        DEBUG_CPASSISTX(DFCCW,WRMSG(HHC90000, "D", "DFCCW cant translate vaddr; back to CP"));
        return;
    }

    ecpsvm_do_deccw1(regs,effective_addr1,effective_addr2,raddr);
    return;
}


/* DISP0 Utility functions */

/* DMKDSP - INCPROBT */
static int ecpsvm_disp_incprobt(REGS *regs,VADR vmb)
{
    char buf[256];
    U64 tspent;
    U64 DW_VMTMOUTQ;
    U64 DW_PROBSTRT;
    U64 DW_PROBTIME;

    DEBUG_CPASSISTX(DISP0,MSGBUF(buf, "INCPROBT Entry : VMBLOK @ %8.8X",vmb));
    DEBUG_CPASSISTX(DISP0,WRMSG(HHC90000, "D", buf));
    DW_VMTMOUTQ=EVM_LD(vmb+VMTMOUTQ);
    DW_PROBSTRT=EVM_LD(PROBSTRT);
    DEBUG_CPASSISTX(DISP0,MSGBUF(buf, "INCPROBT Entry : VMTMOUTQ = %16.16"PRIx64,DW_VMTMOUTQ));
    DEBUG_CPASSISTX(DISP0,WRMSG(HHC90000, "D", buf));
    DEBUG_CPASSISTX(DISP0,MSGBUF(buf, "INCPROBT Entry : PROBSTRT = %16.16"PRIx64,DW_PROBSTRT));
    DEBUG_CPASSISTX(DISP0,WRMSG(HHC90000, "D", buf));
    if(DW_VMTMOUTQ==DW_PROBSTRT)
    {
        DEBUG_CPASSISTX(DISP0,WRMSG(HHC90000, "D", "INCPROBT Already performed"));
        return(2);      /* continue */
    }
    tspent=DW_PROBSTRT-DW_VMTMOUTQ;
    DEBUG_CPASSISTX(DISP0,MSGBUF(buf, "INCPROBT TSPENT = %16.16"PRIx64,tspent));
    DEBUG_CPASSISTX(DISP0,WRMSG(HHC90000, "D", buf));
    DW_PROBTIME=EVM_LD(PROBTIME);
    DW_PROBTIME-=tspent;
    EVM_STD(DW_PROBTIME,PROBTIME);
    DEBUG_CPASSISTX(DISP0,MSGBUF(buf, "INCPROBT NEW PROBTIME = %16.16"PRIx64,DW_PROBTIME));
    DEBUG_CPASSISTX(DISP0,WRMSG(HHC90000, "D", buf));
    return(2);
}

/* DMKDSP RUNTIME */
static int ecpsvm_disp_runtime(REGS *regs,VADR *vmb_p,VADR dlist,VADR exitlist)
{
    char buf[256];
    U64 DW_VMTTIME;
    U64 DW_VMTMINQ;
    BYTE B_VMDSTAT;
    BYTE B_VMTLEVEL;
    BYTE B_VMMCR6;
    U32  F_QUANTUM;
    U32  F_QUANTUMR;
    U32  F_ITIMER;
    int cc;
    RADR raddr;
    VADR tmraddr;
    U32  oldtimer,newtimer;
    VADR vmb;
    VADR runu;

    vmb=*vmb_p;
    DEBUG_CPASSISTX(DISP0,MSGBUF(buf, "RUNTIME Entry : VMBLOK @ %8.8X",vmb));
    DEBUG_CPASSISTX(DISP0,WRMSG(HHC90000, "D", buf));
    runu=EVM_L(RUNUSER);
    /* BAL RUNTIME Processing */
    EVM_STC(CPEX+CPSUPER,CPSTATUS);
    CHARGE_STOP(vmb);
    if(vmb!=runu)
    {
        DEBUG_CPASSISTX(DISP0,MSGBUF(buf, "RUNTIME Switching to RUNUSER VMBLOK @ %8.8X",runu));
        DEBUG_CPASSISTX(DISP0,WRMSG(HHC90000, "D", buf));
        CHARGE_SWITCH(vmb,runu);    /* Charge RUNUSER */
        F_ITIMER=EVM_L(QUANTUMR);
        *vmb_p=vmb;
    }
    else
    {
        F_ITIMER=EVM_L(INTTIMER);
    }
    DEBUG_CPASSISTX(DISP0,MSGBUF(buf, "RUNTIME : VMBLOK @ %8.8X",vmb));
    DEBUG_CPASSISTX(DISP0,WRMSG(HHC90000, "D", buf));
    /* vmb is now RUNUSER */
    /* Check if time slice is over */
    if(F_ITIMER & 0x80000000)
    {
        B_VMDSTAT=EVM_IC(vmb+VMDSTAT);
        B_VMDSTAT&=~VMDSP;
        B_VMDSTAT|=VMTSEND;
        EVM_STC(B_VMDSTAT,vmb+VMDSTAT);
    }
    /* Check if still eligible for current run Q */
    DW_VMTTIME=EVM_LD(vmb+VMTTIME);
    DW_VMTMINQ=EVM_LD(vmb+VMTMINQ);
    /* Check 1st 5 bytes */
    /*2017-03-27 added equality check*/
    if((DW_VMTTIME & 0xffffffffff000000ULL) <= (DW_VMTMINQ & 0xffffffffff000000ULL))
    {
        B_VMDSTAT=EVM_IC(vmb+VMDSTAT);
        B_VMDSTAT&=~VMDSP;
        B_VMDSTAT|=VMQSEND;
        EVM_STC(B_VMDSTAT,vmb+VMDSTAT);
    }
    ecpsvm_disp_incprobt(regs,vmb);
    F_QUANTUM=EVM_L(QUANTUM);
    EVM_ST(F_ITIMER,QUANTUM);
    /* Check if Virtual Timer assist is active */
    B_VMMCR6=EVM_IC(vmb+VMMCR6);
    if (B_VMMCR6 & 0x01)      /* Virtual Timer Flag */
    {
        DEBUG_CPASSISTX(DISP0,WRMSG(HHC90000, "D", "RUNTIME : Complete - VTIMER Assist active"));
        return(2);      /* End of "RUNTIME" here */
    }
    /* Check SET TIMER ON or SET TIMER REAL */
    B_VMTLEVEL=EVM_IC(vmb+VMTLEVEL);
    if(!(B_VMTLEVEL & (VMTON | VMRON)))
    {
        DEBUG_CPASSISTX(DISP0,WRMSG(HHC90000, "D", "RUNTIME : Complete - SET TIMER OFF"));
        return(2);
    }
    /* Update virtual interval timer */
    F_QUANTUMR=EVM_L(QUANTUMR);
    F_QUANTUM-=F_QUANTUMR;
    if(F_QUANTUM & 0x80000000)
    {
        /* Abend condition detected during virtual time update - exit at +32 */
        DEBUG_CPASSISTX(DISP0,WRMSG(HHC90000, "D", "RUNTIME : Bad ITIMER - Taking Exit #32"));
        SET_PSW_IA_AND_MAYBE_IP(regs, EVM_L(exitlist+32));
        return(0);
    }
    /* Load CR1 with the vmblock's VMSEG */
    regs->CR_L(1)=EVM_L(vmb+VMSEG);
    /* Do LRA - Don't access the page directly yet - Could yield a Paging fault */
    cc = ecpsvm_int_lra(regs,INTTIMER,&raddr);
    if(cc!=0)
    {
        /* Update VMTIMER instead */
        tmraddr=vmb+VMTIMER;
    }
    else
    {
        tmraddr=(VADR)raddr;
    }
    oldtimer=EVM_L(tmraddr);
    newtimer=oldtimer-F_QUANTUM;
    EVM_ST(newtimer,tmraddr);
    if((newtimer & 0x80000000) != (oldtimer & 0x80000000))
    {
        /* Indicate XINT to be generated (exit + 8) */
        /* Setup a few regs 1st */
        regs->GR_L(3)=0;
        regs->GR_L(4)=0x00800080;
        regs->GR_L(9)=EVM_L(dlist+4);
        regs->GR_L(11)=vmb;
        /*2017-03-27 ensure VMDSP is off*/
        B_VMDSTAT=EVM_IC(vmb+VMDSTAT);
        B_VMDSTAT&=~VMDSP;
        EVM_STC(B_VMDSTAT,vmb+VMDSTAT);
        /* end of 2017-03-27 */

        SET_PSW_IA_AND_MAYBE_IP(regs, EVM_L(exitlist+8));
        DEBUG_CPASSISTX(DISP0,WRMSG(HHC90000, "D", "RUNTIME : Complete - Taking exit #8"));
        return(0);
    }
    /* Return - Continue DISP0 Processing */
    DEBUG_CPASSISTX(DISP0,WRMSG(HHC90000, "D", "RUNTIME : Complete - ITIMER Updated"));
    return(2);
}

/* E60D DISP0 Instruction */
/* DISP0 : Operand 1 : DISP0 Data list, Operand 2 : DISP0 Exit list */
/*         R11 : User to dispatch                                   */
/*-------------------------------------------------------------------*/
/* E60D DISP0 - Dispatch 0                                     [SSE] */
/*-------------------------------------------------------------------*/
DEF_INST(ecpsvm_dispatch_main)
{
    char buf[256];
    VADR dlist;
    VADR elist;
    VADR vmb;
    /* PSA Fetched Values */
    BYTE B_CPSTATUS;
    BYTE B_VMDSTAT;
    BYTE B_VMPSTAT;
    BYTE B_VMRSTAT;
    BYTE B_VMPEND;
    BYTE B_VMESTAT;

    VADR F_VMPXINT;
    VADR OXINT; /* Back chain ptr for exit 20 */
    U32  F_VMPSWHI;
    U32  F_VMVCR0;
    U32  F_VMIOINT;
    U32 F_VMVCR2;
    U32 DISPCNT;

    U16  H_XINTMASK;

    U32 iomask;
    BYTE extendmsk;     /* Extended I/O mask */

    ECPSVM_PROLOG(DISP0);

    dlist=effective_addr1;
    elist=effective_addr2;
    vmb=regs->GR_L(11);
    DISPCNT=EVM_L(dlist);
    DISPCNT++;
    /* Question #1 : Are we currently running a user */
    B_CPSTATUS=EVM_IC(CPSTATUS);
    if((B_CPSTATUS & CPRUN))
    {
        DEBUG_CPASSISTX(DISP0,WRMSG(HHC90000, "D", "DISP0 : CPRUN On"));
        switch(ecpsvm_disp_runtime(regs,&vmb,dlist,elist))
        {
            case 0: /* Exit taken - success */
            EVM_ST(DISPCNT,dlist);
            CPASSIST_HIT(DISP0);
            return;
            case 1: /* no-op DISP0 */
            return;
            default: /* Continue processing */
            break;
        }
        /* Load VMDSTAT */
        B_VMDSTAT=EVM_IC(vmb+VMDSTAT);
        /* Check if I/O Old PSW has tranlation on */
        if(regs->mainstor[0x38] & 0x04)
        {
            DEBUG_CPASSISTX(DISP0,WRMSG(HHC90000, "D", "DISP0 : I/O Old as XLATE on"));
            /* Yes - I/O Interrupt while running a USER */
            if(B_VMDSTAT & VMDSP)
            {
                DEBUG_CPASSISTX(DISP0,WRMSG(HHC90000, "D", "DISP0 : VMDSP on in VMBLOK - Clean status (Exit #36)"));
                /* Clean status - Do exit 36 */
                regs->GR_L(11)=vmb;
                SET_PSW_IA_AND_MAYBE_IP(regs, EVM_L(elist+36));
                EVM_ST(DISPCNT,dlist);
                CPASSIST_HIT(DISP0);
                return;
            }
        }
    }
    else
    {
        DEBUG_CPASSISTX(DISP0,WRMSG(HHC90000, "D", "DISP0 : CPRUN Off"));
        /* Check if was in Wait State */
        if(B_CPSTATUS & CPWAIT)
        {
            DEBUG_CPASSISTX(DISP0,WRMSG(HHC90000, "D", "DISP0 : CPWAIT On : Exit #4"));
            /* Take exit #4 : Coming out of wait state */
            /* DMKDSPC3 */
            /* No need to update R11 */
            CPASSIST_HIT(DISP0);
            SET_PSW_IA_AND_MAYBE_IP(regs, EVM_L(elist+4));
            EVM_ST(DISPCNT,dlist);
            return;
        }
    }
    /* VMB is now either original GPR11 or RUNUSER */
    /* DMKDSP - UNSTACK */
    DEBUG_CPASSISTX(DISP0,MSGBUF(buf, "DISP0 : At UNSTACK : VMBLOK = %8.8X",vmb));
    DEBUG_CPASSISTX(DISP0,WRMSG(HHC90000, "D", buf));
    B_VMRSTAT=EVM_IC(vmb+VMRSTAT);
    if(B_VMRSTAT & VMCPWAIT)
    {
        DEBUG_CPASSISTX(DISP0,MSGBUF(buf, "DISP0 : VMRSTAT VMCPWAIT On (%2.2X) - Taking exit #12",B_VMRSTAT));
        DEBUG_CPASSISTX(DISP0,WRMSG(HHC90000, "D", buf));
        /* Take Exit 12 */
        regs->GR_L(11)=vmb;
        SET_PSW_IA_AND_MAYBE_IP(regs, EVM_L(elist+12));
        CPASSIST_HIT(DISP0);
        EVM_ST(DISPCNT,dlist);
        return;
    }
    /* Check for PER/PPF (CKPEND) */
    B_VMPEND=EVM_IC(vmb+VMPEND);
    if(B_VMPEND & (VMPERPND | VMPGPND))
    {
        DEBUG_CPASSISTX(DISP0,WRMSG(HHC90000, "D", "DISP0 : PER/PPF Pending - Taking exit #16"));
        /* Take Exit 16 */
        regs->GR_L(11)=vmb;
        SET_PSW_IA_AND_MAYBE_IP(regs, EVM_L(elist+16));
        CPASSIST_HIT(DISP0);
        EVM_ST(DISPCNT,dlist);
        return;
    }
    /* Now, check if we should unstack an external int */
    /* 1st check if VMPXINT is NULL */
    F_VMPSWHI=EVM_L(vmb+VMPSW);     /* Load top of virt PSW - Will need it */
    B_VMPSTAT=EVM_IC(vmb+VMPSTAT);   /* Will need VMPSTAT for I/O ints too */
    F_VMPXINT=EVM_L(vmb+VMPXINT);
    DEBUG_CPASSISTX(DISP0,MSGBUF(buf, "DISP0 : Checking for EXT; Base VMPXINT=%8.8X",F_VMPXINT));
    DEBUG_CPASSISTX(DISP0,WRMSG(HHC90000, "D", buf));
    /* This is DMKDSP - CKEXT */
    if(F_VMPXINT!=0)
    {
        DEBUG_CPASSISTX(DISP0,MSGBUF(buf, "DISP0 : VPSW HI = %8.8X",F_VMPSWHI));
        DEBUG_CPASSISTX(DISP0,WRMSG(HHC90000, "D", buf));
        OXINT=vmb+VMPXINT;
        /* Check if Virtual PSW enabled for Externals */
        /* (works in both BC & EC modes) */
        if(F_VMPSWHI & 0x01000000)
        {
            DEBUG_CPASSISTX(DISP0,WRMSG(HHC90000, "D", "DISP0 : PSW Enabled for EXT"));
            /* Use VMVCR0 or CR0 in ECBLOK */
            F_VMVCR0=EVM_L(vmb+VMVCR0);     /* CR0 or ECBLOK Address */
            if(B_VMPSTAT & VMV370R) /* SET ECMODE ON ?? */
            {
                F_VMVCR0=EVM_L(F_VMVCR0+0); /* EXTCR0 at disp +0 in ECBLOK */
            }
            DEBUG_CPASSISTX(DISP0,MSGBUF(buf, "DISP0 : CR0 = %8.8X",F_VMVCR0));
            DEBUG_CPASSISTX(DISP0,WRMSG(HHC90000, "D", buf));
            /* scan the XINTBLOKS for a mask match */
            /* Save OXINT in the loop for exit 20  */
            for(;F_VMPXINT;OXINT=F_VMPXINT,F_VMPXINT=EVM_L(F_VMPXINT))      /* XINTNEXT @ +0 in XINTBLOK */
            {
                H_XINTMASK=EVM_LH(F_VMPXINT+10);    /* Get interrupt subclass in XINTBLOK */
                DEBUG_CPASSISTX(DISP0,MSGBUF(buf, "DISP0 : XINTMASK =  %4.4X\n",H_XINTMASK));
                DEBUG_CPASSISTX(DISP0,WRMSG(HHC90000, "D", buf));
                H_XINTMASK &= F_VMVCR0;
                if(H_XINTMASK)           /* Check against CR0 (External subclass mask) */
                {
                    DEBUG_CPASSISTX(DISP0,WRMSG(HHC90000, "D", "DISP0 : EXT Hit - Taking exit #20"));
                    /* Enabled for this external */
                    /* Take exit 20 */
                    regs->GR_L(4)=H_XINTMASK;            /* Enabled subclass bits */
                    regs->GR_L(5)=OXINT;                 /* XINTBLOK Back pointer (or VMPXINT) */
                    regs->GR_L(6)=F_VMPXINT;             /* Current XINTBLOK */
                    regs->GR_L(11)=vmb;                  /* RUNUSER */
                    SET_PSW_IA_AND_MAYBE_IP(regs, EVM_L(elist+20));   /* Exit +20 */
                    EVM_ST(DISPCNT,dlist);
                    CPASSIST_HIT(DISP0);
                    return;
                }
            }
        }
    }
    /* After CKEXT : No external pending/reflectable */

    /* This is DMKDSP UNSTIO */
    /* Check for pending I/O Interrupt */

    /* Load PIM */
    F_VMIOINT=EVM_LH(vmb+VMIOINT);
    DEBUG_CPASSISTX(DISP0,MSGBUF(buf, "DISP0 : Checking for I/O; VMIOINT=%8.8X",F_VMIOINT));
    DEBUG_CPASSISTX(DISP0,WRMSG(HHC90000, "D", buf));
    if(F_VMIOINT!=0)        /* If anything in the pipe */
    {
        F_VMIOINT <<=16;    /* Put IOINT mask in bits 0-15 */
        /* Is V-PSW in EC Mode ? */
        iomask=0;
        extendmsk=0;
        B_VMESTAT=EVM_L(VMESTAT);
        if(B_VMESTAT & VMEXTCM)     /* Implies VMV370R on */
        {
            /* Check I/O bit */
            if(F_VMPSWHI & 0x02000000)
            {
                iomask=0;
                extendmsk=1;
            }
        }
        else
        {
            /* BC Mode PSW */
            /* Isolate channel masks for channels 0-5 */
            iomask=F_VMPSWHI & 0xfc000000;
            if(B_VMPSTAT & VMV370R) /* SET ECMODE ON ? */
            {
                if(F_VMPSWHI & 0x02000000)
                {
                    extendmsk=1;
                }
            }
        }
        if(extendmsk)
        {
            F_VMVCR2=EVM_L(vmb+VMECEXT);
            F_VMVCR2=EVM_L(F_VMVCR2+8);
            iomask |= F_VMVCR2;
        }
        if(iomask & 0xffff0000)
        {
            F_VMIOINT&=iomask;
            if(F_VMIOINT)
            {
                DEBUG_CPASSISTX(DISP0,WRMSG(HHC90000, "D", "DISP0 : I/O Hit - Taking exit #24"));
                /* Take Exit 24 */
                regs->GR_L(7)=F_VMIOINT;
                regs->GR_L(11)=vmb;
                SET_PSW_IA_AND_MAYBE_IP(regs, EVM_L(elist+24));   /* Exit +24 */
                EVM_ST(DISPCNT,dlist);
                CPASSIST_HIT(DISP0);
                return;
            }
        }
    }
    /* DMKDSP - CKWAIT */
    /* Clear Wait / Idle bits in VMRSTAT */
    B_VMRSTAT=EVM_IC(vmb+VMRSTAT);
    B_VMRSTAT &= ~(VMPSWAIT | VMIDLE);
    if(F_VMPSWHI & 0x00020000)
    {
        DEBUG_CPASSISTX(DISP0,WRMSG(HHC90000, "D", "DISP0 : VWAIT - Taking exit #28"));
        /* Take exit 28  */
        /* 2017-03-27 Set VMPSWAIT */
        B_VMRSTAT |= VMPSWAIT;
        EVM_STC(B_VMRSTAT,vmb+VMRSTAT);
        /* end of 2017-03-27 */
        regs->GR_L(11)=vmb;
        SET_PSW_IA_AND_MAYBE_IP(regs, EVM_L(elist+28));   /* Exit +28 */
        CPASSIST_HIT(DISP0);
        EVM_ST(DISPCNT,dlist);
        return;
    }
    /* Take exit 0 (DISPATCH) */
    DEBUG_CPASSISTX(DISP0,WRMSG(HHC90000, "D", "DISP0 : DISPATCH - Taking exit #0"));
    /* 2017-03-27 */
    EVM_STC(B_VMRSTAT,vmb+VMRSTAT);
    /* end of 2017-03-27 */
    regs->GR_L(11)=vmb;
    SET_PSW_IA_AND_MAYBE_IP(regs, EVM_L(elist+0));   /* Exit +0 */
    CPASSIST_HIT(DISP0);
    EVM_ST(DISPCNT,dlist);
    return;
}

/******************************************************/
/* SCNRU Instruction : Scan Real Unit                 */
/* Invoked by DMKSCN                                  */
/* E60E                                               */
/* SCNRU D1(R1,B1),D2(R2,B2)                          */
/*                                                    */
/* Operations                                         */
/* The Device Address specified in operand 1 is       */
/* the real device address for which control block    */
/* addresses are to be returned. The storage area     */
/* designated as the 2nd operand is a list of 4       */
/* consecutive fullword (note : the 2nd operand is    */
/* treated as a Real Address, regardless of any       */
/* translation mode that may be in effect)            */
/* +--------------------+-------------------+         */
/* | CHNLINDEX          | RCHTABLE          |         */
/* +--------------------+-------------------+         */
/* | RCUTABLE           | RDVTABLE          |         */
/* +--------------------+-------------------+         */
/* CHNLINDEX is an array of 16 halfwords, each        */
/* representing the offset of the target's device     */
/* channel's RCHBLOCK. If the channel is not defined  */
/* in the RIO table, the index has bit 0, byte 0 set  */
/* to 1                                               */
/*                                                    */
/* The RCHBLOK has at offset +X'20' a table of        */
/* 32 possible control unit indices                   */
/* the device's address bit 8 to 12 are used to fetch */
/* the index specified at the table. If it has        */
/* bit 0 byte 0 set, then the same operation is       */
/* attempted with bits 8 to 11                        */
/* The RCUBLOK then fetched from RCUTABLE + the index */
/* fetched from the RCHBLOK has a DEVICE index table  */
/* at offset +X'28' which can be fetched by using     */
/* bits 5-7                                           */
/* If the RCUBLOK designates an alternate control     */
/* unit block, (Offset +X'5' bit 1 set), then         */
/* the primary RCUBLOK is fetched from offset X'10'   */
/*                                                    */
/* If no RCHBLOK is found, R6,R7 and R8 contain -1    */
/* and Condition Code 3 is set                        */
/*                                                    */
/* if no RCUBLOK is found, R6 contains the RCHBLOK,   */
/* R7 and R8 contain -1, and Condition Code 2 is set  */
/*                                                    */
/* if no RDVBLOK is found, R6 contains the RCHBLOK,   */
/* R7 contains the RCUBLOK and R8 contains -1, and    */
/* condition code 1 is set                            */
/*                                                    */
/* If all 3 control blocks are found, R6 contains the */
/* the RCHBLOK, R7 Contains the RCUBLOK and R8 cont-  */
/* ains the RDVBLOK. Condition code 0 is set          */
/*                                                    */
/* If the instruction is sucesfull, control is        */
/* returned at the address specified by GPR14.        */
/* Otherwise, the next sequential instruction is      */
/* executed, and no GPR or condition code is changed. */
/*                                                    */
/* Exceptions :                                       */
/*       Operation Exception : ECPS:VM Disabled       */
/*       Priviledged Exception : PSW in problem state */
/*                                                    */
/* Note : no access exception is generated for        */
/*        the second operand.                         */
/*                                                    */
/* Note : as of yet, for any other situation than     */
/*        finding all 3 control blocks, SCNRU acts    */
/*        as a NO-OP                                  */
/******************************************************/
/*-------------------------------------------------------------------*/
/* E60E SCNRU - Scan Real Unit                                 [SSE] */
/*-------------------------------------------------------------------*/
DEF_INST(ecpsvm_locate_rblock)
{
    char buf[256];
    U32 chix;           /* offset of RCH in RCH Array */
    U32 cuix;           /* Offset of RCU in RCU Array */
    U32 dvix;           /* Offset of RDV in RDV Array */
    VADR rchixtbl;      /* RCH Index Table */
    VADR rchtbl;        /* RCH Array */
    VADR rcutbl;        /* RCU Array */
    VADR rdvtbl;        /* RDV Array */
    VADR arioct;        /* Data list for SCNRU */

    VADR rchblk;        /* Effective RCHBLOK Address */
    VADR rcublk;        /* Effective RCUBLOK Address */
    VADR rdvblk;        /* Effective RDVBLOK Address */
    U16 rdev;

    ECPSVM_PROLOG(SCNRU);

    /* Obtain the Device address */
    rdev=(effective_addr1 & 0xfff);
    /* And the DMKRIO tables addresses */
    arioct=effective_addr2;

    DEBUG_CPASSISTX(SCNRU,MSGBUF(buf, "ECPS:VM SCNRU called; RDEV=%4.4X ARIOCT=%6.6X",effective_addr1,arioct));
    DEBUG_CPASSISTX(SCNRU,WRMSG(HHC90000, "D", buf));

    /* Get the Channel Index Table */
    rchixtbl= EVM_L(effective_addr2);

    /* Obtain the RCH offset */
    chix=EVM_LH(rchixtbl+((rdev & 0xf00) >> 7));

    DEBUG_CPASSISTX(SCNRU,MSGBUF(buf, "ECPS:VM SCNRU : RCH IX = %x",chix));
    DEBUG_CPASSISTX(SCNRU,WRMSG(HHC90000, "D", buf));

    /* Check if Bit 0 set (no RCH) */
    if(chix & 0x8000)
    {
        // WRMSG(HHC90000, "D", "ECPS:VM SCNRU : NO CHANNEL");
        /*
        regs->GR_L(6)=~0;
        regs->GR_L(7)=~0;
        regs->GR_L(8)=~0;
        SET_PSW_IA_AND_MAYBE_IP(regs, regs->GR_L(14));
        regs->psw.cc=1;
        */
        /* Right now, let CP handle the case */
        return;
    }

    /* Obtain the RCH Table pointer */
    rchtbl=EVM_L(arioct+4);

    /* Add the RCH Index offset */
    rchblk=rchtbl+chix;

    /* Try to obtain RCU index with bits 8-12 of the device */
    cuix=EVM_LH(rchblk+0x20+((rdev & 0xf8)>>2));
    if(cuix & 0x8000)
    {
        /* Try with bits 8-11 */
        cuix=EVM_LH(rchblk+0x20+((rdev & 0xf0)>>2));
        if(cuix & 0x8000)
        {
            // WRMSG(HHC90000, "D", "ECPS:VM SCNRU : NO CONTROL UNIT");
            /*
            regs->GR_L(6)=rchblk;
            regs->GR_L(7)=~0;
            regs->GR_L(8)=~0;
            SET_PSW_IA_AND_MAYBE_IP(regs, regs->GR_L(14));
            regs->psw.cc=2;
            */
            return;
        }
    }
    DEBUG_CPASSISTX(SCNRU,MSGBUF(buf, "ECPS:VM SCNRU : RCU IX = %x",cuix));
    DEBUG_CPASSISTX(SCNRU,WRMSG(HHC90000, "D", buf));
    rcutbl=EVM_L(arioct+8);
    rcublk=rcutbl+cuix;
    dvix=EVM_LH(rcublk+0x28+((rdev & 0x00f)<<1));
    if(EVM_IC(rcublk+5)&0x40)
    {
        rcublk=EVM_L(rcublk+0x10);
    }
    if(dvix & 0x8000)
    {
        // WRMSG(HHC90000, "D", "ECPS:VM SCNRU : NO RDEVBLOK");
        /*
        regs->GR_L(6)=rchblk;
        regs->GR_L(7)=rcublk;
        regs->GR_L(8)=~0;
        SET_PSW_IA_AND_MAYBE_IP(regs, regs->GR_L(14));
        regs->psw.cc=3;
        */
        return;
    }
    DEBUG_CPASSISTX(SCNRU,MSGBUF(buf, "ECPS:VM SCNRU : RDV IX = %x",dvix));
    DEBUG_CPASSISTX(SCNRU,WRMSG(HHC90000, "D", buf));
    dvix<<=3;
    rdvtbl=EVM_L(arioct+12);
    rdvblk=rdvtbl+dvix;
    DEBUG_CPASSISTX(SCNRU,MSGBUF(buf, "ECPS:VM SCNRU : RCH = %6.6X, RCU = %6.6X, RDV = %6.6X",rchblk,rcublk,rdvblk));
    DEBUG_CPASSISTX(SCNRU,WRMSG(HHC90000, "D", buf));
    regs->GR_L(6)=rchblk;
    regs->GR_L(7)=rcublk;
    regs->GR_L(8)=rdvblk;
    regs->psw.cc=0;
    regs->GR_L(15)=0;
    BR14;
    CPASSIST_HIT(SCNRU);
}

/* E60F CCWGN Instruction
   CCWGN : "General CCW Processing"

   On Entry:  R1 -> VADR of CCW data area
              R4 =  byte count from CCW minus 1
              R6 -> RADR of real CCW
              R9 -> VADR of virtual CCW
             R11 -> VMBLOK
             R13 -> SAVEAREA

   E60F Operand 1 -> data list of addresses (dl) "GENDATA"
                      +0 = V(DMKSYSCS)
                      +4 = A(DIALTBL)
   E60F Operand 2 -> exit list of addresses (el) "GENEXITS"
                      +0 = A(FWDIDAL)
                      +4 = A(CCWMANYF)
                      +8 = A(CCWNXT9A)
                      +12= A(SHRDPAGE)
                      +16= A(CCWNXT10)
                      +20= A(CCWNEXT)
                      +24= A(CCWNEWV2)
                      +28= A(TICSCAN)
                      +32= A(CCWDIAL)
                      +36= A(CALLISM)
                      +40= A(CCWEXIT)
                      +44= A(CCWNXT12)
                      +48= A(ITSAREL)
*/
/*-------------------------------------------------------------------*/
/* E60F CCWGN - General CCW Processing                         [SSE] */
/*-------------------------------------------------------------------*/
DEF_INST(ecpsvm_comm_ccwproc)
{
VADR dl;
VADR el;
VADR vlast_page;
VADR vstart_page;
VADR cortable;
VADR savearea;
VADR vmb;
VADR vdev;
VADR rcw;
VADR iob;
VADR rcaw;
RADR raddr;
U32  ccwcount;
int  rc;
BYTE ccwop;
BYTE B_RCWCTL;
BYTE B_VMOSTAT;
BYTE B_VDEVTYPC;

    ECPSVM_PROLOG(CCWGN);
    dl=effective_addr1;
    el=effective_addr2;
    savearea=regs->GR_L(13);
    vmb=regs->GR_L(11);

    if(EVM_IC(regs->GR_L(6)+RCWFLAG) & IDA)
    {
        DEBUG_CPASSISTX(CCWGN,WRMSG(HHC90000, "D", "CCWGN - Exit DWDIDAL"));
        SET_PSW_IA_AND_MAYBE_IP(regs,EVM_L(el+0));           /* IDA bit is set in CCW, exit to FWDIDAL */
        return;
    }

    vlast_page=(regs->GR_L(1)+regs->GR_L(4)) & 0x00FFF000;
    vstart_page=regs->GR_L(1) & 0x00FFF000;
    if(vstart_page != vlast_page)
    {
        DEBUG_CPASSISTX(CCWGN,WRMSG(HHC90000, "D", "CCWGN - Exit CCWMANYF"));
        SET_PSW_IA_AND_MAYBE_IP(regs,EVM_L(el+4));           /* CCW data area crosses page boundary, exit CCWMANYF */
        return;
    }

    /* Check if the CCW data area page is paged-in and accessible.  If not,
       bring in the page where the CCW data area is located and lock it.
       If this function cannot be accomplished give it back to CP without
       changing the PSW IA.  This will "no-op" the assist and CP will redo CCWGENRL.
    */

    /* At CCWNXT9 */
    rc=ecpsvm_int_lra(regs,regs->GR_L(1),&raddr);
    if(rc)
    {
        rc=ecpsvm_tranbrng(regs,dl+0,regs->GR_L(1),&raddr);
        if(rc)
        {
            DEBUG_CPASSISTX(CCWGN,WRMSG(HHC90000, "D", "CCWGN - Cant bring in the page; back to CP"));
            return;                                /* Cant bring in the page; give it back to CP */
        }
    }
    ecpsvm_lockpage1(regs,dl+0,raddr);
    regs->GR_L(2)=raddr;

    /* Plug the real data address into the real CCW;
       Keep local copy of RCWCTL and indicate page is to be unlocked later.
    */

    /* At CCWNXT9B */
    ccwop=EVM_IC(regs->GR_L(6));
    EVM_ST((ccwop << 24) | regs->GR_L(2),regs->GR_L(6));
    B_RCWCTL=EVM_IC(regs->GR_L(6)+RCWCTL) | RCWIO;
    EVM_STC(B_RCWCTL,regs->GR_L(6)+RCWCTL);

    /* User running a saved system?  Check if CCW data is with a shared page */
    /* At CCWCHKSH */
    B_VMOSTAT=EVM_IC(vmb+VMOSTAT);
    if(B_VMOSTAT & VMSHR)
    {
        vstart_page = regs->GR_L(2) & 0x00FFF000;
        cortable=EVM_L(dl+0);
        cortable+=vstart_page >> 8;
        if(EVM_IC(cortable+CORFLAG) & CORSHARE)
        {
            DEBUG_CPASSISTX(CCWGN,WRMSG(HHC90000, "D", "CCWGN - Exit SHRDPAGE"));
            SET_PSW_IA_AND_MAYBE_IP(regs,EVM_L(el+12));      /* exit; CCW data area is in shared page  (SHRDPAGE) */
        }
    }

    /* At CCWNXT11 */
    if(B_RCWCTL & RCWSHR)
    {
        B_RCWCTL&=~RCWIO;
        EVM_STC(B_RCWCTL,regs->GR_L(6)+RCWCTL);
    }

    /* At CCWNXT10/CLRSENSE */
    regs->GR_L(8)=EVM_L(savearea+SAVER8)+EVM_L(vmb+VMDVSTRT);
    vdev=regs->GR_L(8);
    if(EVM_IC(vdev+VDEVFLAG) & VDEVUC)
    {
        DEBUG_CPASSISTX(CCWGN,WRMSG(HHC90000, "D", "CCWGN - Exit CCWNXT10"));
        SET_PSW_IA_AND_MAYBE_IP(regs,EVM_L(el+16));          /* exit to CCWNXT10 in CP; sense bytes are present */
        return;
    }

    /* At CCWNXT12 */
    if(B_VMOSTAT & VMSHR)
    {
        DEBUG_CPASSISTX(CCWGN,WRMSG(HHC90000, "D", "CCWGN - Exit CCWNXT12"));
        SET_PSW_IA_AND_MAYBE_IP(regs,EVM_L(el+44));          /* exit to CCWNXT12 in CP; running with shared segments */
        CPASSIST_HIT(CCWGN);                    /* partial translation was assisted */
        return;
    }

    /* At CCWNXT14 */
    if((EVM_IC(savearea+VIRFLAG) & (CD+CC)))
    {
        DEBUG_CPASSISTX(CCWGN,WRMSG(HHC90000, "D", "CCWGN - Exit CCWNEXT"));
        SET_PSW_IA_AND_MAYBE_IP(regs,EVM_L(el+20));          /* exit to CCWNEXT; CD or CC flag set in CCW, get next CCW */
        CPASSIST_HIT(CCWGN);
        return;
    }

    /* At CCWCHKPV */
    if(EVM_IC(savearea+PRVFLAG) & (SMCOM+FWDTIC))
    {
        DEBUG_CPASSISTX(CCWGN,WRMSG(HHC90000, "D", "CCWGN - Exit CCWNEXT(2)"));
        SET_PSW_IA_AND_MAYBE_IP(regs,EVM_L(el+20));          /* exit to CCWNEXT; previous CCW status modifier or fwd TIC */
        CPASSIST_HIT(CCWGN);
        return;
    }

    /* Compute # of virtual CCWs, # of real CCWs, and plug into the RCWCCW block.  This must be in R10. */

    /* At CCWNXT13 */
    regs->GR_L(10)=EVM_L(savearea+THISRCW);
    rcw=regs->GR_L(10);
    ccwcount=(regs->GR_L(9)+8 - EVM_L(rcw+RCWVCAW)) >> 3;  /* end of virt CCW string minus start, div by 8 */
    EVM_STH(ccwcount,rcw+RCWVCNT);
    ccwcount=((regs->GR_L(6)+8) - (rcw+RCWHSIZ))  >> 3;    /* end of real CCW string minus start, div by 8 */
    EVM_STH(ccwcount,rcw+RCWRCNT);

    /* At CCWNXT16 */
    if(EVM_IC(savearea+MEMO2) & STRTNEW)
    {
        DEBUG_CPASSISTX(CCWGN,WRMSG(HHC90000, "D", "CCWGN - Exit CCWNEWV2"));
        SET_PSW_IA_AND_MAYBE_IP(regs,EVM_L(el+24));          /* exit to CCWNEWV2; start new CCW string */
        CPASSIST_HIT(CCWGN);
        return;
    }
    if(EVM_IC(savearea+MEMO1) & HADUTIC)
    {
        DEBUG_CPASSISTX(CCWGN,WRMSG(HHC90000, "D", "CCWGN - Exit TICSCAN"));
        SET_PSW_IA_AND_MAYBE_IP(regs,EVM_L(el+28));          /* exit to TICSCAN; unprocessed TICs remain */
        CPASSIST_HIT(CCWGN);
        return;
    }

    /* Plug the real CCW string address into the IOBLOK */
    /* At CCWNXT18 */
    regs->GR_L(10)=EVM_L(savearea+SAVER10);
    iob=regs->GR_L(10);
    rcaw=EVM_L(savearea+FIRSTRCW)+RCWHSIZ;
    EVM_ST(rcaw,iob+IOBCAW);

    if(EVM_L(savearea+DEVTABLE) == EVM_L(dl+4))
    {
        DEBUG_CPASSISTX(CCWGN,WRMSG(HHC90000, "D", "CCWGN - Exit CCWDIAL"));
        SET_PSW_IA_AND_MAYBE_IP(regs,EVM_L(el+32));          /* exit to CCWDIAL if dialed line */
        CPASSIST_HIT(CCWGN);
        return;
    }

    B_VDEVTYPC=EVM_IC(vdev+VDEVTYPC);
    if(B_VDEVTYPC & CLASGRAF)
    {
        if(!(EVM_IC(vdev+VDEVTYPE) & (TYP3277|TYP3278)))
        {
            DEBUG_CPASSISTX(CCWGN,WRMSG(HHC90000, "D", "CCWGN - Exit CCWEXIT"));
            SET_PSW_IA_AND_MAYBE_IP(regs,EVM_L(el+40));      /* Not 3270 device; exit to CCWEXIT; we're done */
            CPASSIST_HIT(CCWGN);
            return;
        }
        if(!(EVM_IC(vdev+VDEVSTAT) & VDEVDED))
        {
            DEBUG_CPASSISTX(CCWGN,WRMSG(HHC90000, "D", "CCWGN - Exit CCWDIAL(2)"));
            SET_PSW_IA_AND_MAYBE_IP(regs,EVM_L(el+32));      /* exit to CCWDIAL if i/o to non-dialed 3270 */
            CPASSIST_HIT(CCWGN);
            return;
        }
    }

    /* At CCWNXT19 */
    if(!(B_VDEVTYPC & CLASDASD))
    {
        DEBUG_CPASSISTX(CCWGN,WRMSG(HHC90000, "D", "CCWGN - Exit CCWEXIT(2)"));
        SET_PSW_IA_AND_MAYBE_IP(regs,EVM_L(el+40));          /* Not DASD; exit to CCWEXIT; we're done */
        CPASSIST_HIT(CCWGN);
        return;
    }
    if(EVM_IC(vdev+VDEVFLG2) & VDEVRRF)
    {
        DEBUG_CPASSISTX(CCWGN,WRMSG(HHC90000, "D", "CCWGN - Exit ITSAREL"));
        SET_PSW_IA_AND_MAYBE_IP(regs,EVM_L(el+48));          /* DASD w/reserve-release; exit to ITSAREL */
        CPASSIST_HIT(CCWGN);
        return;
    }

    /* At CCWNXT28 */
    if(EVM_IC(savearea+MEMO1) & HADISAM)
    {
        if(EVM_IC(vdev+VDEVSTAT) & VDEVDED)
        {
            DEBUG_CPASSISTX(CCWGN,WRMSG(HHC90000, "D", "CCWGN - Exit CALLISM"));
            SET_PSW_IA_AND_MAYBE_IP(regs,EVM_L(el+36));      /* ISAM ok to dedicated DASD; exit to CALLISM */
            CPASSIST_HIT(CCWGN);
            return;
        }
        if(EVM_LH(vdev+VDEVRELN)==0)
        {
            DEBUG_CPASSISTX(CCWGN,WRMSG(HHC90000, "D", "CCWGN - Exit CALLISM(2)"));
            SET_PSW_IA_AND_MAYBE_IP(regs,EVM_L(el+36));      /* ISAM ok on full-volume MDISK; exit to CALLISM */
            CPASSIST_HIT(CCWGN);
            return;
        }
    }

    SET_PSW_IA_AND_MAYBE_IP(regs,EVM_L(el+40));          /* Exit to CCWEXIT; we're done */
    CPASSIST_HIT(CCWGN);
    return;
}

/* E610 UXCCW Instruction       */
/* Untranslate CCW              */
/* UXCCW D1(R1,B1),D2(R2,B2)    */
/* 1st operand : Address of VDEVCSW in the VDEVBLOK  */
/* 2nd Operand : not used or provided */
/*-------------------------------------------------------------------*/
/* E610 UXCCW - Untranslate CCW                                [SSE] */
/*-------------------------------------------------------------------*/
DEF_INST(ecpsvm_unxlate_ccw)
{
VADR vcsw;
VADR ccwaddr;
U16  ccwctl;
int  realct;

    ECPSVM_PROLOG(UXCCW);

    vcsw=EVM_L(effective_addr1);
    ccwaddr=vcsw & 0x00FFFFFF;
    if(!ccwaddr)
    {
        BR14;
        return;         /* all done if VDEVCSW contains 0 */
    }
    realct=0;
    ccwaddr-=8;
    ccwctl=1;
    while (ccwctl != 0xFFFF)
    {
        ccwctl=EVM_LH(ccwaddr+4);
        if(!(ccwctl & RCWGEN))          /* RCWGEN is set if CP generated the CCW */
        {
            realct+=8;                  /* count length of real CCWs that are not CP generated */
        }
        ccwaddr-=8;                     /* back up one CCW, or into RCWBLOK header area */
    }

    /* ccwaddr now points at RCWTASK block */
    ccwaddr=EVM_L(ccwaddr+4);           /* Get RCWVCAW (vaddr of virtual CCW string) */
    ccwaddr+=realct;                    /* Compute ending vaddr to be placed in virtual CSW */
    vcsw=(vcsw & 0xFF000000) | ccwaddr;
    EVM_ST(vcsw,effective_addr1);
    BR14;
    CPASSIST_HIT(UXCCW);
    return;
}

/* E611 DISP2 Instruction */
/* DISP2 */
/*-------------------------------------------------------------------*/
/* E611 DISP2 - Dispatch 2                                     [SSE] */
/*-------------------------------------------------------------------*/
DEF_INST(ecpsvm_disp2)
{
    ECPSVM_PROLOG(DISP2);
    switch(ecpsvm_do_disp2(regs,effective_addr1,effective_addr2))
    {
        case 0: /* Done */
            CPASSIST_HIT(DISP2);
            return;
        case 1: /* No-op */
            return;
        case 2: /* Done */
            CPASSIST_HIT(DISP2);
            RETURN_INTCHECK(regs);
    }
    return;
}

/* E612 STEVL Instruction */
/* STEVL : Store ECPS:VM support level */
/* STEVL D1(R1,B1),D2(R2,B2) */
/* 1st operand : Fullword address in which to store ECPS:VM Support level */
/* 2nd operand : ignored */
/*-------------------------------------------------------------------*/
/* E612 STEVL - Store ECPS:VM Version/Level                    [SSE] */
/*-------------------------------------------------------------------*/
DEF_INST(ecpsvm_store_level)
{
    char buf[256];
    VADR    ia;
    PSA_3XX *psa;
    REGS newr;

    ECPSVM_PROLOG(STEVL);
    EVM_ST(sysblk.ecpsvm.level,effective_addr1);
    DEBUG_CPASSISTX(STEVL,MSGBUF(buf, "ECPS:VM STORE LEVEL %d called",sysblk.ecpsvm.level));
    DEBUG_CPASSISTX(STEVL,WRMSG(HHC90000, "D", buf));
    CPASSIST_HIT(STEVL);

    sysblk.ecpsvm.freetrap=0;           /* assume no free trap in effect */
    if(!sysblk.ecpsvm.enabletrap)       /* exit if ECPSVM YES NOTRAP was specified */
    {
        return;
    }

    if(sysblk.ecpsvm.level != 20)
    {
        return;
    }

    /* Let's validate several CP instructions in DMKCPI to try to locate the code that
       NO-OPs the assist functions when the FRET/FRET trap is installed in CP.  During
       validation, do not examine base and displacements that could be different simply due
       to a re-assembly of DMKCPI.  If we don't find what we expect at any time, just
       exit from this function and allow CP to NO-OP the assist functions as usual.
    */
    ia=PSW_IA_FROM_IP(regs,0);
    if(EVM_L(ia) != 0x960C034A)         /*  OI    CPSTAT2,CPASTAVL+CPASTON  */
        return;
    ia+=4;
    if(EVM_L(ia) != 0xB7660440)         /*  LCTL  R6,R6,ZEROES */
        return;
    ia+=4;
    if(EVM_L(ia) != 0xD50304B0)         /*  CLC   F20,0(R3) */
        return;
    ia+=6;
    if(EVM_LH(ia) != 0x4740)            /*  BH    CPIPINT3 */
        return;
    ia+=4;
    if(EVM_LH(ia) != 0x4780)            /*  BE    CPINTFRE */
        return;

    /* get B2(D2) effective addr from the BE instruction */
    effective_addr2=EVM_LH(ia+2);
    b2=(effective_addr2 >> 12) & 0x0F;
    effective_addr2&=0x0FFF;
    effective_addr2+=regs->GR_L(b2);

    /* effective_addr2 now points to label 'CPINTFRE' which is one instruction prior to where
       the trap disables this assist.  Ensure that code is present and if it is, continue.
       Else, bail out.
    */
    if(EVM_L(effective_addr2) != 0xD2070068)    /* CPINTFRE  MVC  PRNPSW(8),CPIPSWS+2*8 */
        return;

    /* Ensure there are five consecutive L and MVC instructions that NO-OP the assist */
    /* That is, 5 ea 4-byte instructions + 5 ea 6-byte instructions = 50 bytes to validate */
    for(ia=effective_addr2+6;ia<effective_addr2+50;ia+=10)
    {
        if(EVM_LH(ia) != 0x5860)            /*  L    R6,CPATxxxx  */
        return;
        if(EVM_L(ia+4) != 0xD2056000)       /*  MVC  0(6,R6),NOOP */
        return;
    }

    /* Validate the instruction where we will resume execution upon return from STEVL */
    if(EVM_LH(ia) != 0x5840)                /*  L    R4,=F'-16'   */
        return;

    /* If we make it here, we have validated that the FREE/FRET trap code exists. */
    /* Now perform the necessary instructions in DMKCPI that we are skipping over */
    EVM_STC(EVM_IC(CPSTAT2) | (CPASTAVL+CPASTON),CPSTAT2);
    regs->CR_L(6)=0;

    /* effective_addr2 still points to a MVC instruction that fills in the PGM New PSW;
       We need to calculate D2(B2) from the MVC to point to the PSW and then move it
       into PGM New.
    */
    effective_addr2=EVM_LH(effective_addr2+4);
    b2=(effective_addr2 >> 12) & 0x0F;
    effective_addr2&=0x0FFF;
    effective_addr2+=regs->GR_L(b2);
    INITPSEUDOREGS(newr);
    ARCH_DEP(load_psw) (&newr, (BYTE *)&regs->mainstor[effective_addr2]);
    psa=(PSA_3XX *)MADDR((VADR)0 , USE_PRIMARY_SPACE, regs, ACCTYPE_WRITE, 0);
    ARCH_DEP(store_psw) (&newr, (BYTE *)&psa->pgmnew);

    /* Indicate that this assist is now running with the free trap present.
       Then, reset the PSW IA to the value in 'ia' right now; that is,
       just past the end of the assist no-ops.  CP FREE/FRET trap remains in effect.
    */
    DEBUG_CPASSISTX(STEVL,WRMSG(HHC90000, "D", "CP FREE/FRET trap detected; assist operational with trap in effect"));
    sysblk.ecpsvm.freetrap=1;
    SET_PSW_IA_AND_MAYBE_IP(regs, ia);
    return;
}

/* E613 LCSPG Instruction */
/* LCSPG : Locate Changed Shared Page */
/* LCSPG D1(R1,B1),D2(R2,B2) */
/* 1st operand : Address of VMBLOK */
/* 2nd operand : ignored */

/* Verify that every page frame that belongs to every shared segment currently   */
/* loaded by the current user does not have the change bit set in the key.       */
/* Assists the CP code in DMKVMASH.                                              */

/*-------------------------------------------------------------------*/
/* E613 LCSPG - Locate Changed Shared Page                     [SSE] */
/*-------------------------------------------------------------------*/
DEF_INST(ecpsvm_loc_chgshrpg)
{

VADR  vmb;
VADR  vma;
VADR  vmseg;
VADR  shrtable;
VADR  ste;
VADR  p_pte;
VADR  page;
VADR  page2;
U32   shrsegct;
U32   i;
U32   j;
U32   segnum;
U32   pagnum;
U16   pte;

    ECPSVM_PROLOG(LCSPG);

    vmb=effective_addr1;
    vma=EVM_L(vmb+VMASSIST);
    vmseg=EVM_L(vmb+VMSEG);

    while(vma)
    {
        shrtable=EVM_L(vma+VMASHRBK);
        shrsegct=EVM_L(shrtable+SHRSEGCT);
        for (i=0;i<shrsegct;i++)
        {
            segnum=EVM_IC(shrtable+SHRSEGNM+i) * 4;
            ste=EVM_L(vmseg+segnum);                // get the seg table entry for this segment
            if(!(ste & 0x00000001))                 // if segment is not invalid...
            {
                pagnum=ste & 0xf0000000;            // isolate page table length
                pagnum=pagnum >> 28;                // compute number of pages in segment
                pagnum++;                           // make relative to 1
                p_pte=ste & 0x00fffff8;             // isolate the PTE address
                for(j=0;j<pagnum;j++)
                {
                    pte=EVM_LH(p_pte);
                    if(!(pte & 0x0008))             // if page is not invalid...
                    {
                        page=pte & 0xfff0;          // isolate page index
                        page=page<< 8;              // compute real page address
#if defined(FEATURE_2K_STORAGE_KEYS)
                        page2=page+0x800;
                        if(0
                           || (ARCH_DEP( get_storage_key )( page ) & STORKEY_CHANGE)
                           || (ARCH_DEP( get_storage_key )( page2 ) & STORKEY_CHANGE))
#else
                        if (ARCH_DEP( get_storage_key )( page ) & STORKEY_CHANGE)
#endif
                        {
                            return;                 // let CP handle if a page is changed
                        }
                    } /* if pte... */
                    p_pte+=2;                       // -> next pte
                }  /* next j */
            } /* if ste... */
        } /* next i */
        vma=EVM_L(vma+VMAFPNT);                     // -> next VMABLOK
    } /* while vma != 0 */

    regs->psw.cc=0;                                 // no pages changed in any shared segment
    BR14;
    CPASSIST_HIT(LCSPG);
    return;
}

/*************************************************************/
/* E614 FREEX Instruction                                    */
/* FREEX : Allocate CP Storage Extended                      */
/* FREEX B1(R1,B1),D2(R2,B2)                                 */
/* 1st operand : Address of FREEX Parameter list             */
/*               +0 : Maxsize = Max number of DW allocatable */
/*                              with FREEX                   */
/*               +4- : Subpool index table                   */
/* 2nd operand : Subpool table (indexed)                     */
/* GPR 0 : Number of DWs to allocate                         */
/*                                                           */
/* Each allocatable block is forward chained                 */
/* if the subpool is empty, return to caller                 */
/* if the subpool has an entry, allocate from the subpool    */
/* and save the next block address as the subpool chain head */
/* return allocated block in GPR1. return at address in GPR14*/
/* if allocation succeeded                                   */
/* if allocate fails, return at next sequential instruction  */
/*************************************************************/
/*-------------------------------------------------------------------*/
/* E614 FREEX - Allocate CP Storage Extended                   [SSE] */
/*-------------------------------------------------------------------*/
DEF_INST(ecpsvm_extended_freex)
{
    char buf[256];
    U32 maxdw;
    U32 numdw;
    U32 numbytes;
    U32 maxsztbl;
    U32 spixtbl;
    BYTE spix;
    U32 freeblock;
    U32 trapdata;
    U32 nextblk;
    VADR traceptr;

    ECPSVM_PROLOG(FREEX);
    numdw=regs->GR_L(0);
    spixtbl=effective_addr2;
    maxsztbl=effective_addr1;
    DEBUG_CPASSISTX(FREEX,MSGBUF(buf, "ECPS:VM FREEX DW = %4.4X",numdw));
    DEBUG_CPASSISTX(FREEX,WRMSG(HHC90000, "D", buf));
    if(numdw==0)
    {
        return;
    }
    DEBUG_CPASSISTX(FREEX,MSGBUF(buf, "MAXSIZE ADDR = %6.6X, SUBPOOL INDEX TABLE = %6.6X",maxsztbl,spixtbl));
    DEBUG_CPASSISTX(FREEX,WRMSG(HHC90000, "D", buf));

    if (sysblk.ecpsvm.freetrap)
    {
        numdw++;
    }

    /* E1 = @ of MAXSIZE (maximum # of DW allocatable by FREEX from subpools) */
    /*      followed by subpool pointers                                      */
    /* E2 = @ of subpool indices                                              */
    maxdw=EVM_L(maxsztbl);
    if(numdw>maxdw)
    {
        DEBUG_CPASSISTX(FREEX,WRMSG(HHC90000, "D", "FREEX request beyond subpool capacity"));
        return;
    }
    /* Fetch subpool index */
    spix=EVM_IC(spixtbl+numdw);
    DEBUG_CPASSISTX(FREEX,MSGBUF(buf, "Subpool index = %X",spix));
    DEBUG_CPASSISTX(FREEX,WRMSG(HHC90000, "D", buf));
    /* Fetch value */
    freeblock=EVM_L(maxsztbl+4+spix);
    DEBUG_CPASSISTX(FREEX,MSGBUF(buf, "Value in subpool table = %6.6X",freeblock));
    DEBUG_CPASSISTX(FREEX,WRMSG(HHC90000, "D", buf));
    if(freeblock==0)
    {
        /* Can't fullfill request here */
        return;
    }
    nextblk=EVM_L(freeblock);
    EVM_ST(nextblk,maxsztbl+4+spix);

    /* If we are running with the FREE trap, fill the block with EEs and plug the
       trap identifier and the address of the caller at the end of the new block
    */
    if (sysblk.ecpsvm.freetrap)
    {
        numbytes=8*(numdw-1);
        memset((char*)regs->mainstor+freeblock, 0xEE, numbytes);
        trapdata=freeblock+numbytes;
        EVM_ST(0x9AC7E5D5,trapdata);
        trapdata+=4;
        EVM_ST(regs->GR_L(14),trapdata);
        EVM_STC(regs->GR_LHLCL(0),trapdata);        /* store original number of doublewords requested */
    }

    if (EVM_IC(TRACFLG1) & TRAC67)
    {
        traceptr=ecpsvm_get_trace_entry(regs);
        EVM_ST(regs->GR_L(11),traceptr);
        EVM_STC(TRCFREE,traceptr);
        EVM_ST(regs->GR_L(0),traceptr+4);
        EVM_ST(freeblock,traceptr+8);
        EVM_ST(regs->GR_L(14),traceptr+12);
    }

    DEBUG_CPASSISTX(FREEX,MSGBUF(buf, "New Value in subpool table = %6.6X",nextblk));
    DEBUG_CPASSISTX(FREEX,WRMSG(HHC90000, "D", buf));
    regs->GR_L(1)=freeblock;
    regs->psw.cc=0;
    BR14;
    CPASSIST_HIT(FREEX);
    return;
}

/*************************************************************/
/* E615 FRETX Instruction                                    */
/* FRETX : Return CP Free storage                            */
/* FRETX D1(R1,B1),R2(D2,B2)                                 */
/* 1st operand : Max DW for subpool free/fret                */
/* 2nd Operand : FRET PLIST                                  */
/*               +0 Coretable address                        */
/*               +4 CL4'FREE'                                */
/*               +8 Maxsize (same as operand 1)              */
/*               +12 Subpool Table Index                     */
/* The block is checked VS the core table to check if it     */
/* is eligible to be returned to the subpool chains          */
/* If it is, then it is returned. Control is returned at     */
/* the address in GPR 14. Otherwise, if anything cannot      */
/* be resolved, control is returned at the next sequential   */
/* instruction.                                              */
/* Note: this routine is also called by DISP2                */
/*************************************************************/
int ecpsvm_do_fretx(REGS *regs,VADR block,U16 numdw,VADR maxsztbl,VADR fretl)
{
    char buf[256];
    U32 cortbl;
    U32 maxdw;
    U32 cortbe; /* Core table Page entry for fretted block */
    U32 prevblk;
    BYTE spix;
    VADR traceptr;
    U32 trapdata;
    U32 numbytes;

    DEBUG_CPASSISTX(FRETX,MSGBUF(buf, "X fretx called AREA=%6.6X, DW=%4.4X",regs->GR_L(1),regs->GR_L(0)));
    DEBUG_CPASSISTX(FRETX,WRMSG(HHC90000, "D", buf));
    if(numdw==0)
    {
        DEBUG_CPASSISTX(FRETX,WRMSG(HHC90000, "D", "ECPS:VM Cannot FRETX : DWORDS = 0"));
        return(1);
    }
    if (sysblk.ecpsvm.freetrap)
    {
        numdw++;
    }
    maxdw=EVM_L(maxsztbl);
    if(numdw>maxdw)
    {
        DEBUG_CPASSISTX(FRETX,MSGBUF(buf, "ECPS:VM Cannot FRETX : DWORDS = %d > MAXDW %d",numdw,maxdw));
        DEBUG_CPASSISTX(FRETX,WRMSG(HHC90000, "D", buf));
        return(1);
    }
    cortbl=EVM_L(fretl);
    cortbe=cortbl+((block & 0xfff000)>>8);
    if(EVM_L(cortbe)!=EVM_L(fretl+4))
    {
        DEBUG_CPASSISTX(FRETX,WRMSG(HHC90000, "D", "ECPS:VM Cannot FRETX : Area not in Core Free area"));
        return(1);
    }
    if(EVM_IC(cortbe+8)!=0x02)
    {
        DEBUG_CPASSISTX(FRETX,WRMSG(HHC90000, "D", "ECPS:VM Cannot FRETX : Area flag != 0x02"));
        return(1);
    }
    spix=EVM_IC(fretl+11+numdw);
    prevblk=EVM_L(maxsztbl+4+spix);
    if(prevblk==block)
    {
        DEBUG_CPASSISTX(FRETX,WRMSG(HHC90000, "D", "ECPS:VM Cannot FRETX : fretted block already on subpool chain"));
        return(1);
    }

    /* If the FREE/FRET trap is operational, validate the trap signature at the end of the block
       to be freed.  If valid, replace the signature with C'FREE' in EBCDIC to catch any 2nd
       attempt to fret this same block.  If the signature is invalid, bail out and let CP
       do the FRET so it can result in the FRE013 system abend.
    */
    if (sysblk.ecpsvm.freetrap)
    {
        numdw--;
        numbytes=8*numdw;
        trapdata=block+numbytes;
        if(EVM_L(trapdata) != 0x9AC7E5D5)
        {
            return(1);
        }
        EVM_ST(0xC6D9C5C5,trapdata);
    }

    EVM_ST(block,maxsztbl+4+spix);
    EVM_ST(prevblk,block);

    if (EVM_IC(TRACFLG1) & TRAC67)
    {
        traceptr=ecpsvm_get_trace_entry(regs);
        EVM_ST(regs->GR_L(11),traceptr);
        EVM_STC(TRCFRET,traceptr);
        EVM_ST(numdw,traceptr+4);
        EVM_ST(block,traceptr+8);
        EVM_ST(regs->GR_L(14),traceptr+12);
    }
    return(0);
}

/*-------------------------------------------------------------------*/
/* E615 FRETX - Return CP Storage Extended                     [SSE] */
/*-------------------------------------------------------------------*/
DEF_INST(ecpsvm_extended_fretx)
{
    U32 fretl;
    U32 maxsztbl;
    U32 numdw;
    U32 block;

    ECPSVM_PROLOG(FRETX);

    numdw=regs->GR_L(0);
    block=regs->GR_L(1) & ADDRESS_MAXWRAP(regs);
    maxsztbl=effective_addr1 & ADDRESS_MAXWRAP(regs);
    fretl=effective_addr2 & ADDRESS_MAXWRAP(regs);
    if(ecpsvm_do_fretx(regs,block,numdw,maxsztbl,fretl)==0)
    {
        BR14;
        CPASSIST_HIT(FRETX);
    }
    return;
}

/* E616 PRFMA Instruction */
/* PRFMA  Preferred Machine Assist:  Not supported */
/*-------------------------------------------------------------------*/
/* E616 PRFMA - Preferred Machine Assist                       [SSE] */
/*-------------------------------------------------------------------*/
DEF_INST(ecpsvm_prefmach_assist)
{
    ECPSVM_PROLOG(PMASS);
}

/**********************************/
/* VM ASSISTS                     */
/**********************************/

/******************************************************************/
/* LPSW/SSM/STxSM :                                               */
/* Not sure about the current processing ..                       */
/* *MAYBE* we need to invoke DMKDSPCH when the newly loaded PSW   */
/* does not need further checking. Now.. I wonder what the point  */
/* is to return to CP anyway, as we have entirelly validated the  */
/* new PSW (i.e. for most of it, this is essentially a BRANCH     */
/* However...                                                     */
/* Maybe we should call DMKDSPCH (from the DMKPRVMA list)         */
/* only if re-enabling bits (and no Int pending)                  */
/*                                                                */
/* For the time being, we do THIS :                               */
/* If the new PSW 'disables' bits or enables bit but MICPEND=0    */
/* we just update the VPSW and continue                           */
/* Same for LPSW.. But we also update the IA                      */
/* If we encounter ANY issue, we just return to caller (which will*/
/* generate a PRIVOP) thus invoking CP like for non-EVMA          */
/******************************************************************/

/******************************************************************/
/* Check psw Transition validity                                  */
/******************************************************************/
/* NOTE : oldr/newr Only have the PSW field valid (the rest is not initialised) */
int ecpsvm_check_pswtrans(REGS *regs,ECPSVM_MICBLOK *micblok, BYTE micpend, REGS *oldr, REGS *newr)
{
    UNREFERENCED(micblok);
    UNREFERENCED(regs);

    MAYBE_SET_PSW_IA_FROM_IP(newr);
    MAYBE_SET_PSW_IA_FROM_IP(oldr);

    /* Check for a switch from BC->EC or EC->BC */
    if(ECMODE(&oldr->psw)!=ECMODE(&newr->psw))
    {
        DEBUG_SASSISTX(LPSW,WRMSG(HHC90000, "D", "New and Old PSW have a EC/BC transition"));
        return(1);
    }
    /* Check if PER or DAT is being changed */
    if(ECMODE(&newr->psw))
    {
        if((newr->psw.sysmask & 0x44) != (oldr->psw.sysmask & 0x44))
        {
            DEBUG_SASSISTX(LPSW,WRMSG(HHC90000, "D", "New PSW Enables DAT or PER"));
            return(1);
        }
    }
    /* Check if a Virtual interrupt is pending and new interrupts are being enabled */
    if(micpend & 0x80)
    {
        if(ECMODE(&newr->psw))
        {
            if(((~oldr->psw.sysmask) & 0x03) & newr->psw.sysmask)
            {
                DEBUG_SASSISTX(LPSW,WRMSG(HHC90000, "D", "New PSW Enables interrupts and MICPEND (EC)"));
                return(1);
            }
        }
        else
        {
            if(~oldr->psw.sysmask & newr->psw.sysmask)
            {
                DEBUG_SASSISTX(LPSW,WRMSG(HHC90000, "D","New PSW Enables interrupts and MICPEND (BC)"));
                return(1);
            }
        }
    }
    if(WAITSTATE(&newr->psw))
    {
        DEBUG_SASSISTX(LPSW,WRMSG(HHC90000, "D", "New PSW is a WAIT PSW"));
        return(1);
    }
    if(ECMODE(&newr->psw))
    {
        if(newr->psw.sysmask & 0xb8)
        {
            DEBUG_SASSISTX(LPSW,WRMSG(HHC90000, "D", "New PSW sysmask incorrect"));
            return(1);
        }
    }
    if(newr->psw.IA & 0x01)
    {
        DEBUG_SASSISTX(LPSW,WRMSG(HHC90000, "D", "New PSW has ODD IA"));
        return(1);
    }
    return(0);
}

/* 80 - SSM Instruction Assist */
int ecpsvm_dossm(REGS *regs,int b2,VADR effective_addr2)
{
    char buf2[256];
    BYTE  reqmask;
    BYTE *cregs;
    U32   creg0;
    REGS  npregs;

    SASSIST_PROLOG(SSM);

    /*
    if(!(micevma & MICSTSM))
    {
        DEBUG_SASSISTX(SSM,MSGBUF(buf2, "SASSIST SSM reject : SSM Disabled in MICEVMA; EVMA=%2.2X",micevma));
        DEBUG_SASSISTX(SSM,WRMSG(HHC90000, "D", buf2));
        return(1);
    }
    */
    /* Get CR0 - set ref bit on  fetched CR0 (already done in prolog for MICBLOK) */
    cregs=MADDR(micblok.MICCREG,USE_REAL_ADDR,regs,ACCTYPE_READ,0);
    FETCH_FW(creg0,cregs);

    /* Reject if V CR0 specifies SSM Suppression */
    if(creg0 & 0x40000000)
    {
        DEBUG_SASSISTX(SSM,WRMSG(HHC90000, "D", "SASSIST SSM reject : V SSM Suppr"));
        return(1);
    }
    /* Load the requested SSM Mask */
    /* USE Normal vfetchb here ! not only do we want tranlsation */
    /* but also fetch protection control, ref bit, etc.. */
    reqmask=ARCH_DEP(vfetchb) (effective_addr2,b2,regs);

    INITPSEUDOREGS(npregs);
    /* Load the virtual PSW AGAIN in a new structure */

    ARCH_DEP(load_psw) (&npregs,vpswa_p);

    npregs.psw.sysmask=reqmask;

    if(ecpsvm_check_pswtrans(regs,&micblok,micpend,&vpregs,&npregs))       /* Check PSW transition capability */
    {
        DEBUG_SASSISTX(SSM,WRMSG(HHC90000, "D", "SASSIST SSM Reject : New PSW too complex"));
        return(1); /* Something in the NEW PSW we can't handle.. let CP do it */
    }

    /* While we are at it, set the IA in the V PSW */
    MAYBE_SET_PSW_IA_FROM_IP(regs);
    SET_PSW_IA_AND_MAYBE_IP(&npregs, regs->psw.IA);

    /* Set the change bit */
    MADDR(vpswa,USE_REAL_ADDR,regs,ACCTYPE_WRITE,0);
    /* store the new PSW */
    ARCH_DEP(store_psw) (&npregs,vpswa_p);
    DEBUG_SASSISTX(SSM,MSGBUF(buf2, "SASSIST SSM Complete : new SM = %2.2X",reqmask));
    DEBUG_SASSISTX(SSM,WRMSG(HHC90000, "D", buf2));
    DEBUG_SASSISTX(SSM,MSGBUF(buf2, "SASSIST SSM New VIRT "));
    DEBUG_SASSISTX(SSM,display_psw(&npregs, &buf2[strlen(buf2)], sizeof(buf2)-(int)strlen(buf2)));
    DEBUG_SASSISTX(SSM,WRMSG(HHC90000, "D", buf2));
    DEBUG_SASSISTX(SSM,MSGBUF(buf2, "SASSIST SSM New REAL "));
    DEBUG_SASSISTX(SSM,display_psw(regs, &buf2[strlen(buf2)], sizeof(buf2)-(int)strlen(buf2)));
    DEBUG_SASSISTX(SSM,WRMSG(HHC90000, "D", buf2));
    SASSIST_HIT(SSM);
    return(0);
}

/* Common code for SVC assists */

/* CP LINK (SVC 8 Supervisor state) */
int     ecpsvm_doCPlink(REGS *regs)
{
VADR svclist;
VADR vnextsave;
VADR svcR12;
VADR svcR13;
VADR traceptr;
BYTE *work_p;

    if(!sysblk.ecpsvm.available)
    {
        DEBUG_CPASSISTX(LINK,WRMSG(HHC90000, "D", "CPASSTS LINK ECPS:VM Disabled in configuration"));
        return(1);
    }
    if(!ecpsvm_cpstats.LINK.enabled)
    {
        DEBUG_CPASSISTX(LINK,WRMSG(HHC90000, "D", "CPASSTS LINK Disabled by command"));
        return(1);
    }
    if(!(regs->CR_L(6) & 0x02000000))
    {
        return(1);
    }
    ecpsvm_cpstats.LINK.call++;
    DEBUG_CPASSISTX(LINK,WRMSG(HHC90000, "D", "LINK called"));


    svclist=EVM_L(ASVCLIST);
    vnextsave=EVM_L(svclist+NEXTSAVE);
    if(!vnextsave)
    {
        return(1);                      /* no save area available */
    }
    if(regs->GR_L(15) >= EVM_L(APAGCP))
    {
        return(1);                      /* called module not in nucleus */
    }

    /* Link the save areas together and set GR12 to the destination address */
    svcR12=regs->GR_L(12);
    svcR13=regs->GR_L(13);
    regs->GR_L(13)=vnextsave;
    EVM_ST(EVM_L(regs->GR_L(13)+SAVENEXT),svclist+NEXTSAVE);
    EVM_ST(svcR12,regs->GR_L(13)+SAVER12);
    EVM_ST(svcR13,regs->GR_L(13)+SAVER13);
    regs->GR_L(14)=PSW_IA_FROM_IP(regs,0);
    EVM_ST(regs->GR_L(14),regs->GR_L(13)+SAVERETN);
    regs->GR_L(12)=regs->GR_L(15);

    if (EVM_IC(TRACFLG1) & TRAC02)
    {
        traceptr=ecpsvm_get_trace_entry(regs);
        EVM_ST(regs->GR_L(15),traceptr);
        EVM_STC(TRCSVC,traceptr);
        EVM_ST(0x00020008,traceptr+4);              /* always ILC=2 and INTC=08 */
        work_p=MADDR((VADR)traceptr+8,USE_REAL_ADDR,regs,ACCTYPE_WRITE,0);
        ARCH_DEP(store_psw) (regs,work_p);
    }

    CPASSIST_HIT(LINK);
    SET_PSW_IA_AND_MAYBE_IP(regs,regs->GR_L(12));
    return(0);
}

/* CP RETURN (SVC 12 Supervisor state) */
int     ecpsvm_doCPretrn(REGS *regs)
{
VADR svclist;
VADR retaddr;
VADR traceptr;
U32  svchi;
U32  svclo;
U32  slcadr;
BYTE *work_p;

    if(!sysblk.ecpsvm.available)
    {
        DEBUG_CPASSISTX(RETRN,WRMSG(HHC90000, "D", "CPASSTS RETRN ECPS:VM Disabled in configuration"));
        return(1);
    }
    if(!ecpsvm_cpstats.RETRN.enabled)
    {
        DEBUG_CPASSISTX(RETRN,WRMSG(HHC90000, "D", "CPASSTS RETRN Disabled by command"));
        return(1);
    }
    if(!(regs->CR_L(6) & 0x02000000))
    {
        return(1);
    }
    ecpsvm_cpstats.RETRN.call++;
    DEBUG_CPASSISTX(RETRN,WRMSG(HHC90000, "D", "RETRN called"));

    if(regs->GR_L(12) >= EVM_L(APAGCP))
    {
        DEBUG_CPASSISTX(RETRN,WRMSG(HHC90000, "D", "RETRN reject : module not in nucleus"));
        return(1);                      /* returning from a module not in nucleus, let CP do it */
    }

    svclist=EVM_L(ASVCLIST);
    slcadr=EVM_L(svclist+SLCADDR);
    svchi=EVM_L(svclist+DMKSVCHI);
    svclo=EVM_L(svclist+DMKSVCLO);

    if(regs->GR_L(12) < slcadr)
    {
        DEBUG_CPASSISTX(RETRN,WRMSG(HHC90000, "D", "RETRN reject : exit from V=R"));
        return(1);                      /* exit from V=R storage, let CP do it */
    }
    if(regs->GR_L(13) < svchi)
    {
        if(regs->GR_L(13) > svclo)
        {
            DEBUG_CPASSISTX(RETRN,WRMSG(HHC90000, "D", "RETRN reject : savearea in paging area"));
           return(1);                      /* Let CP handle if SAVEAREA is in dynamic paging area */
        }
        if(regs->GR_L(13) < slcadr)
        {
            DEBUG_CPASSISTX(RETRN,WRMSG(HHC90000, "D", "RETRN reject : savearea in V=R area"));
           return(1);                      /* Let CP handle this */
        }
    }

    /* Get return address and unlink the save areas; restore caller's R12/R13 and return */
    retaddr=EVM_L(regs->GR_L(13)+SAVERETN) & 0x00FFFFFF;
    regs->GR_L(14)=EVM_L(svclist+NEXTSAVE);
    EVM_ST(regs->GR_L(14),regs->GR_L(13)+SAVENEXT);
    EVM_ST(regs->GR_L(13),svclist+NEXTSAVE);
    regs->GR_L(12)=EVM_L(regs->GR_L(13)+SAVER12);
    regs->GR_L(13)=EVM_L(regs->GR_L(13)+SAVER13);

    if (EVM_IC(TRACFLG1) & TRAC02)
    {
        traceptr=ecpsvm_get_trace_entry(regs);
        EVM_ST(retaddr,traceptr);
        EVM_STC(TRCSVC,traceptr);
        EVM_ST(0x0002000C,traceptr+4);              /* always ILC=2 and INTC=0C */
        work_p=MADDR((VADR)traceptr+8,USE_REAL_ADDR,regs,ACCTYPE_WRITE,0);
        ARCH_DEP(store_psw) (regs,work_p);
    }

    CPASSIST_HIT(RETRN);
    SET_PSW_IA_AND_MAYBE_IP(regs,retaddr);
    return(0);
}

/* VM Assist SVC (Problem state) */
int     ecpsvm_doassistsvc(REGS *regs,int svccode)
{
    PSA_3XX *psa;
    REGS newr;

    SASSIST_PROLOG(SVC);

    if(svccode==76)     /* NEVER trap SVC 76 */
    {
        DEBUG_SASSISTX(SVC,WRMSG(HHC90000, "D", "SASSIST SVC Reject : SVC 76"));
        return(1);
    }
    /* Get what the NEW PSW should be */

    psa=(PSA_3XX *)MADDR((VADR)0 , USE_PRIMARY_SPACE, regs, ACCTYPE_READ, 0);
                                                                                       /* Use all around access key 0 */
                                                                                          /* Also sets reference bit     */
    INITPSEUDOREGS(newr);
    ARCH_DEP(load_psw) (&newr, (BYTE *)&psa->svcnew);   /* Ref bit set above */
    DEBUG_SASSISTX(SVC,MSGBUF(buf, "SASSIST SVC NEW VIRT "));
    DEBUG_SASSISTX(SVC,display_psw(&newr, &buf[strlen(buf)], sizeof(buf)-(int)strlen(buf)));
    DEBUG_SASSISTX(SVC,WRMSG(HHC90000, "D", buf));
    /* Get some stuff from the REAL Running PSW to put in OLD SVC PSW */
    MAYBE_SET_PSW_IA_FROM_IP(regs);
    SET_PSW_IA_AND_MAYBE_IP(&vpregs, regs->psw.IA);            /* Instruction Address */
    vpregs.psw.cc=regs->psw.cc;                   /* Condition Code      */
    vpregs.psw.pkey=regs->psw.pkey;               /* Protection Key      */
    vpregs.psw.progmask=regs->psw.progmask;       /* Program Mask        */
    vpregs.psw.intcode=svccode;                   /* SVC Interrupt code  */
    DEBUG_SASSISTX(SVC,MSGBUF(buf, "SASSIST SVC OLD VIRT "));
    DEBUG_SASSISTX(SVC,display_psw(&vpregs, &buf[strlen(buf)], sizeof(buf)-(int)strlen(buf)));
    DEBUG_SASSISTX(SVC,WRMSG(HHC90000, "D", buf));
    if(ecpsvm_check_pswtrans(regs,&micblok,micpend,&vpregs,&newr))       /* Check PSW transition capability */
    {
        DEBUG_SASSISTX(SVC,WRMSG(HHC90000, "D", "SASSIST SVC Reject : Cannot make transition to new PSW"));
        return(1); /* Something in the NEW PSW we can't handle.. let CP do it */
    }

    /* Set intcode in PSW (for BC mode) */
    ARCH_DEP(store_psw) (&vpregs, (BYTE *)&psa->svcold);

    if(ECMODE(&vpregs.psw))
    {
        /* Also set SVC interrupt code */
        /* and ILC                     */
        STORE_FW((BYTE *)&psa->svcint,0x00020000 | svccode);
    }
    /* Now, update some stuff in the REAL PSW */
    SASSIST_LPSW(newr);
    /* Now store the new PSW in the area pointed by the MICBLOK */
    ARCH_DEP(store_psw) (&newr,vpswa_p);
    DEBUG_SASSISTX(SVC,WRMSG(HHC90000, "D", "SASSIST SVC Done"));
    SASSIST_HIT(SVC);
    return(0);
}

/* 0A - SVC Assist Main Entry */
int     ecpsvm_dosvc(REGS *regs,int svccode)
{

    if(regs->CR_L(6) & ECPSVM_CR6_SVCINHIB)
    {
        DEBUG_SASSISTX(SVC,WRMSG(HHC90000, "D", "SASSIST SVC Reject : SVC Assist Inhibit"));
        return(1);      /* SVC SASSIST INHIBIT ON */
    }

    /* Check first if the CPU is in supervisor state.  If so, this is a CP-issued SVC.
       If it is SVC 8 (LINK) or SVC 12 (RETURN), attempt to handle these for CP.  All other
       SVC numbers are handled by CP.

       If the CPU is in problem state, then we can attempt to assist this SVC via VMA.
    */
    if(!PROBSTATE(&regs->psw))
    {
          if(svccode==8)
          {
              return ecpsvm_doCPlink(regs);
          }
          if(svccode==12)
          {
              return ecpsvm_doCPretrn(regs);
          }
          return(1);                                /* all other SVC numbers let CP handle */
    }
    return ecpsvm_doassistsvc(regs,svccode);        /* problem state:  do VM assist for SVCs */
}

/* 82 - LPSW Instruction Assist */
int ecpsvm_dolpsw(REGS *regs,int b2,VADR e2)
{
    BYTE * nlpsw;
    REGS nregs;

    SASSIST_PROLOG(LPSW);

    /* Reject if MICEVMA says not to do LPSW sim */
    if(!(micevma & MICLPSW))
    {
        DEBUG_SASSISTX(LPSW,WRMSG(HHC90000, "D", "SASSIST LPSW reject : LPSW disabled in MICEVMA"));
        return(1);
    }
    if(e2&0x03)
    {
        DEBUG_SASSISTX(LPSW,MSGBUF(buf, "SASSIST LPSW %6.6X - Alignement error",e2));
        DEBUG_SASSISTX(LPSW,WRMSG(HHC90000, "D", buf));
        return(1);

    }
    nlpsw=MADDR(e2,b2,regs,ACCTYPE_READ,regs->psw.pkey);
    INITPSEUDOREGS(nregs);
    ARCH_DEP(load_psw) (&nregs,nlpsw);
    if(ecpsvm_check_pswtrans(regs,&micblok,micpend,&vpregs,&nregs))
    {
        DEBUG_SASSISTX(LPSW,WRMSG(HHC90000, "D", "SASSIST LPSW Rejected - Cannot make PSW transition"));
        return(1);

    }
    SASSIST_LPSW(nregs);
    MADDR(vpswa,USE_REAL_ADDR,regs,ACCTYPE_WRITE,0);
    /* Set ref bit in address pointed by MICBLOK */
    ARCH_DEP(store_psw) (&nregs,vpswa_p);
    DEBUG_SASSISTX(LPSW,MSGBUF(buf, "SASSIST LPSW New VIRT "));
    DEBUG_SASSISTX(LPSW,display_psw(&nregs, &buf[strlen(buf)], sizeof(buf)-(int)strlen(buf)));
    DEBUG_SASSISTX(LPSW,WRMSG(HHC90000, "D", buf));
    DEBUG_SASSISTX(LPSW,MSGBUF(buf, "SASSIST LPSW New REAL "));
    DEBUG_SASSISTX(LPSW,display_psw(regs, &buf[strlen(buf)], sizeof(buf)-(int)strlen(buf)));
    DEBUG_SASSISTX(LPSW,WRMSG(HHC90000, "D", buf));
    SASSIST_HIT(LPSW);
    return(0);
}

/* The ecpsvm_virttmr_ext( ) function is not called or executed */
#if 0 // please leave this code in place for reference
int ecpsvm_virttmr_ext(REGS *regs)
{
    char buf[256];

    DEBUG_SASSISTX(VTIMER,WRMSG(HHC90000, "D", "SASSIST VTIMER Checking if we can IRPT"));
    MSGBUF(buf, "SASSIST VTIMER Virtual");
    display_psw(regs, &buf[strlen(buf)], sizeof(buf)-(int)strlen(buf));
    DEBUG_SASSISTX(VTIMER,WRMSG(HHC90000, "D", buf));
    if(IS_IC_ECPSVTIMER(regs))
    {
        DEBUG_SASSISTX(VTIMER,WRMSG(HHC90000, "D", "SASSIST VTIMER Not pending"));
        return(1);
    }
    if(!PROBSTATE(&regs->psw))
    {
        DEBUG_SASSISTX(VTIMER,WRMSG(HHC90000, "D", "SASSIST VTIMER Not dispatching a VM"));
        return(1);
    }
    if(!(regs->psw.sysmask & PSW_EXTMASK))
    {
        DEBUG_SASSISTX(VTIMER,WRMSG(HHC90000, "D", "SASSIST VTIMER Test int : Not enabled for EXT"));
        return(1);
    }
    if(!(regs->CR_L(6) & ECPSVM_CR6_VIRTTIMR))
    {
        DEBUG_SASSISTX(VTIMER,WRMSG(HHC90000, "D", "SASSIST VTIMER Test int : Not enabled for VTIMER"));
        return(1);
    }
    DEBUG_SASSISTX(VTIMER,WRMSG(HHC90000, "D", "SASSIST VTIMER Please, do"));
    return(0);
}
#endif // please leave this code in place for reference

/* SIO/SIOF Assist */
/* Not supported */
int ecpsvm_dosio(REGS *regs,int b2,VADR e2)
{
    SASSIST_PROLOG(SIO);
    UNREFERENCED(b2);
    UNREFERENCED(e2);
    return(1);
}


/* AC - STNSM Instruction Assist */
int ecpsvm_dostnsm(REGS *regs,int b1,VADR effective_addr1,int imm2)
{
    BYTE  oldmask;
    REGS  npregs;

    SASSIST_PROLOG(STNSM);

    /* Reject if MICEVMA says not to do STNSM sim */
    if(!(micevma & MICSTSM))
    {
        DEBUG_SASSISTX(STNSM,WRMSG(HHC90000, "D", "SASSIST STNSM reject : STNSM disabled in MICEVMA"));
        return(1);
    }

    INITPSEUDOREGS(npregs);
    /* Load the virtual PSW in a new structure */
    ARCH_DEP(load_psw) (&npregs,vpswa_p);

    /* get the old system mask before we AND it */
    oldmask=npregs.psw.sysmask;
    npregs.psw.sysmask&=imm2;

    if(ecpsvm_check_pswtrans(regs,&micblok,micpend,&vpregs,&npregs))       /* Check PSW transition capability */
    {
        DEBUG_SASSISTX(STNSM,WRMSG(HHC90000, "D", "SASSIST STNSM Reject : New PSW too complex"));
        return(1); /* Something in the NEW PSW we can't handle.. let CP do it */
    }

    /* Set the old system  mask byte in the byte designated by the STNSM instruction */
    /* USE Normal vstoreb here! not only do we want tranlsation */
    /* but also store protection, change bit, etc.. */
    ARCH_DEP(vstoreb) (oldmask,effective_addr1,b1,regs);

    /* While we are at it, set the IA in the V PSW */
    MAYBE_SET_PSW_IA_FROM_IP(regs);
    SET_PSW_IA_AND_MAYBE_IP(&npregs, regs->psw.IA);

    /* Set the change bit for the V PSW */
    MADDR(vpswa,USE_REAL_ADDR,regs,ACCTYPE_WRITE,0);
    /* store the new PSW */
    ARCH_DEP(store_psw) (&npregs,vpswa_p);
    DEBUG_SASSISTX(STNSM,MSGBUF(buf, "SASSIST STNSM Complete : old SM = %2.2X mask = %2.2X New SM = %2.2X",oldmask,imm2,npregs.psw.sysmask));
    DEBUG_SASSISTX(STNSM,WRMSG(HHC90000, "D", buf));
    DEBUG_SASSISTX(STNSM,MSGBUF(buf, "SASSIST STNSM New VIRT "));
    DEBUG_SASSISTX(STNSM,display_psw(&npregs, &buf[strlen(buf)], sizeof(buf)-(int)strlen(buf)));
    DEBUG_SASSISTX(STNSM,WRMSG(HHC90000, "D", buf));
    DEBUG_SASSISTX(STNSM,MSGBUF(buf, "SASSIST STNSM New REAL "));
    DEBUG_SASSISTX(STNSM,display_psw(regs, &buf[strlen(buf)], sizeof(buf)-(int)strlen(buf)));
    DEBUG_SASSISTX(STNSM,WRMSG(HHC90000, "D", buf));
    SASSIST_HIT(STNSM);
    return(0);
}

/* AD - STOSM Instruction Assist */
int ecpsvm_dostosm(REGS *regs,int b1,VADR effective_addr1,int imm2)
{
    BYTE  oldmask;
    REGS  npregs;

    SASSIST_PROLOG(STOSM);

    /* Reject if MICEVMA says not to do STOSM sim */
    if(!(micevma & MICSTSM))
    {
        DEBUG_SASSISTX(STOSM,WRMSG(HHC90000, "D", "SASSIST STOSM reject : STOSM disabled in MICEVMA"));
        return(1);
    }

    INITPSEUDOREGS(npregs);
    /* Load the virtual PSW in a new structure */
    ARCH_DEP(load_psw) (&npregs,vpswa_p);

    /* get the old system mask before we OR it */
    oldmask=npregs.psw.sysmask;
    npregs.psw.sysmask|=imm2;

    if(ecpsvm_check_pswtrans(regs,&micblok,micpend,&vpregs,&npregs))       /* Check PSW transition capability */
    {
        DEBUG_SASSISTX(STOSM,WRMSG(HHC90000, "D", "SASSIST STOSM Reject : New PSW too complex"));
        return(1); /* Something in the NEW PSW we can't handle.. let CP do it */
    }

    /* Set the old system  mask byte in the byte designated by the STOSM instruction */
    /* USE Normal vstoreb here! not only do we want tranlsation */
    /* but also store protection, change bit, etc.. */
    ARCH_DEP(vstoreb) (oldmask,effective_addr1,b1,regs);

    /* While we are at it, set the IA in the V PSW */
    MAYBE_SET_PSW_IA_FROM_IP(regs);
    SET_PSW_IA_AND_MAYBE_IP(&npregs, regs->psw.IA);

    /* Set the change bit for the V PSW */
    MADDR(vpswa,USE_REAL_ADDR,regs,ACCTYPE_WRITE,0);
    /* store the new PSW */
    ARCH_DEP(store_psw) (&npregs,vpswa_p);
    DEBUG_SASSISTX(STOSM,MSGBUF(buf, "SASSIST STOSM Complete : old SM = %2.2X mask = %2.2X New SM = %2.2X",oldmask,imm2,npregs.psw.sysmask));
    DEBUG_SASSISTX(STOSM,WRMSG(HHC90000, "D", buf));
    DEBUG_SASSISTX(STOSM,MSGBUF(buf, "SASSIST STOSM New VIRT "));
    DEBUG_SASSISTX(STOSM,display_psw(&npregs, &buf[strlen(buf)], sizeof(buf)-(int)strlen(buf)));
    DEBUG_SASSISTX(STOSM,WRMSG(HHC90000, "D", buf));
    DEBUG_SASSISTX(STOSM,MSGBUF(buf, "SASSIST STOSM New REAL "));
    DEBUG_SASSISTX(STOSM,display_psw(regs, &buf[strlen(buf)], sizeof(buf)-(int)strlen(buf)));
    DEBUG_SASSISTX(STOSM,WRMSG(HHC90000, "D", buf));
    SASSIST_HIT(STOSM);
    return(0);
}

/* B6 - STCTL Instruction Assist */
int ecpsvm_dostctl(REGS *regs,int r1,int r3,int b2,VADR effective_addr2)
{

    U32 crs[16];
    BYTE *ecb_p;
    VADR F_ECBLOK,vmb;
    BYTE B_VMPSTAT;

    int i,j,numcrs;

    SASSIST_PROLOG(STCTL);

    if(effective_addr2 & 0x03)
    {
        DEBUG_SASSISTX(STCTL,WRMSG(HHC90000, "D", "SASSIST STCTL Reject : Not aligned"));
        return(1);
    }

    vmb=vpswa-0xA8;
    B_VMPSTAT=EVM_IC(vmb+VMPSTAT);

    if((!(B_VMPSTAT & VMV370R)) && ((r1!=r3) || (r1!=0)))
    {
        DEBUG_SASSISTX(STCTL,WRMSG(HHC90000, "D", "SASSIST STCTL Reject : BC Mode VM & STCTL != 0,0"));
        return(1);
    }

    /* Load the CRs from the ECBLOK */
    if(B_VMPSTAT & VMV370R)
    {
        F_ECBLOK=fetch_fw(regs->mainstor+vmb+VMECEXT);
        for(i=0;i<16;i++)
        {
             ecb_p=MADDR(F_ECBLOK+(i*4),USE_REAL_ADDR,regs,ACCTYPE_READ,0);
             crs[i]=fetch_fw(ecb_p);
        }
    }
    else
    {
        F_ECBLOK=vmb+VMECEXT;  /* Update ECBLOK ADDRESS for VCR0 Update */
        ecb_p=MADDR(F_ECBLOK,USE_REAL_ADDR,regs,ACCTYPE_READ,0);
        /* Load OLD CR0 From VMBLOK */
        crs[0]=fetch_fw(ecb_p);
    }

    /* Determine the range of CRs to be stored */
    if(r1>r3)
    {
        numcrs=(r3+16)-r1;
    }
    else
    {
        numcrs=r3-r1;
    }
    numcrs++;

    /* store the selected CRs into the user's virtual storage */
    for(j=r1,i=0;i<numcrs;i++,j++)
    {
        if(j>15)
        {
            j-=16;
        }

        ARCH_DEP(vstore4)(crs[j],effective_addr2+(i*4),b2,regs);
    }

    DEBUG_SASSISTX(STCTL,MSGBUF(buf, "SASSIST STCTL %d,%d Done",r1,r3));
    DEBUG_SASSISTX(STCTL,WRMSG(HHC90000, "D", buf));
    SASSIST_HIT(STCTL);
    return 0;
}

/* B7 - LCTL Instruction Assist */
int ecpsvm_dolctl(REGS *regs,int r1,int r3,int b2,VADR effective_addr2)
{
    U32 crs[16];        /* New CRs */
    U32 rcrs[16];       /* REAL CRs */
    U32 ocrs[16];       /* Old CRs */
    BYTE *ecb_p;
    VADR F_ECBLOK,vmb;
    BYTE B_VMPSTAT;

    int i,j,numcrs;

    SASSIST_PROLOG(LCTL);
    if(effective_addr2 & 0x03)
    {
        DEBUG_SASSISTX(LCTL,WRMSG(HHC90000, "D", "SASSIST LCTL Reject : Not aligned"));
        return(1);
    }

    vmb=vpswa-0xA8;
    B_VMPSTAT=EVM_IC(vmb+VMPSTAT);

    if((!(B_VMPSTAT & VMV370R)) && ((r1!=r3) || (r1!=0)))
    {
        DEBUG_SASSISTX(LCTL,WRMSG(HHC90000, "D", "SASSIST LCTL Reject : BC Mode VM & LCTL != 0,0"));
        return(1);
    }
    /* Determine the range of CRs to be loaded */
    if(r1>r3)
    {
        numcrs=(r3+16)-r1;
    }
    else
    {
        numcrs=r3-r1;
    }
    numcrs++;
    for(j=r1,i=0;i<numcrs;i++,j++)
    {
        if(j>15)
        {
            j-=16;
        }
        crs[j]=ARCH_DEP(vfetch4)((effective_addr2+(i*4)) & ADDRESS_MAXWRAP(regs),b2,regs);
    }
    if(B_VMPSTAT & VMV370R)
    {
        F_ECBLOK=fetch_fw(regs->mainstor+vmb+VMECEXT);
    for(i=0;i<16;i++)
    {
        ecb_p=MADDR(F_ECBLOK+(i*4),USE_REAL_ADDR,regs,ACCTYPE_READ,0);
        ocrs[i]=fetch_fw(ecb_p);
    }
    }
    else
    {
        F_ECBLOK=vmb+VMECEXT;  /* Update ECBLOK ADDRESS for VCR0 Update */
    ecb_p=MADDR(F_ECBLOK,USE_REAL_ADDR,regs,ACCTYPE_READ,0);
        /* Load OLD CR0 From VMBLOK */
    ocrs[0]=fetch_fw(ecb_p);
    }
    for(i=0;i<16;i++)
    {
        rcrs[i]=regs->CR_L(i);
    }
    /* Source safely loaded into "crs" array */
    /* Load the CRS - exit from loop if it's not possible */
    DEBUG_SASSISTX(LCTL,MSGBUF(buf, "SASSIST LCTL %d,%d : Modifying %d cregs",r1,r3,numcrs));
    DEBUG_SASSISTX(LCTL,WRMSG(HHC90000, "D", buf));
    for(j=r1,i=0;i<numcrs;i++,j++)
    {
        if(j>15)
        {
            j-=16;
        }
        switch(j)
        {
            case 0:     /* CR0 Case */
                /* Check 1st 2 bytes of CR0 - No change allowed */
                if((ocrs[0] & 0xffff0000) != (crs[0] & 0xffff0000))
                {
                    DEBUG_SASSISTX(LCTL,WRMSG(HHC90000, "D", "SASSIST LCTL Reject : CR0 High changed"));
                    return 1;
                }
                /* Not allowed if : NEW mask is being enabled AND MICPEND AND PSW has EXT enabled */
                if(vpregs.psw.sysmask & 0x01)
                {
                    if(micpend & 0x80)
                    {
                        if((~(ocrs[0] & 0xffff)) & (crs[0] & 0xffff))
                        {
                            DEBUG_SASSISTX(LCTL,WRMSG(HHC90000, "D", "SASSIST LCTL Reject : CR0 EXTSM Enables new EXTS"));
                            return 1;
                        }
                    }
                }
                ocrs[0]=crs[0];
                break;
            case 1:
                if(ocrs[1] != crs[1])
                {
                    DEBUG_SASSISTX(LCTL,WRMSG(HHC90000, "D", "SASSIST LCTL Reject : CR1 Updates shadow table"));
                    return 1;
                }
                break;
            case 2:
                /* Not allowed if : NEW Channel mask is turned on AND micpend AND PSW Extended I/O Mask is on */
                if(vpregs.psw.sysmask & 0x02)
                {
                    if((~ocrs[2]) & crs[2])
                    {
                        if(micpend & 0x80)
                        {
                            DEBUG_SASSISTX(LCTL,WRMSG(HHC90000, "D", "SASSIST LCTL Reject : CR2 IOCSM Enables I/O Ints"));
                            return(1);
                        }
                    }
                }
                ocrs[2]=crs[2];
                break;
            case 3:     /* DAS Control regs (not used under VM/370) */
            case 4:
            case 5:
            case 7:
                /* 2017-01-12
                LCTL assist should not update real CR3-CR7 with values
                from a virtual machine execution of LCTL.  CR3-CR7 are
                for the DAS feature.  If any of these four control regs
                are specified then the assist should kick it back to CP
                and let CP handle it, because different versions of VM
                do different things with these CRs depending on whether
                DAS is available or not.

                original code:

                ocrs[j]=crs[j];
                rcrs[j]=crs[j]
                break;

                replacement code: */

                DEBUG_SASSISTX(LCTL,MSGBUF(buf, "SASSIST LCTL Reject : DAS CR%d Update",j));
                DEBUG_SASSISTX(LCTL,WRMSG(HHC90000, "D", buf));
                return(1);
                /* end of 2017-01-12 */

            case 6: /* VCR6 Ignored on real machine */
                ocrs[j]=crs[j];
                break;
            case 8: /* Monitor Calls */
                DEBUG_SASSISTX(LCTL,WRMSG(HHC90000, "D", "SASSIST LCTL Reject : MC CR8 Update"));
                return(1);
            case 9: /* PER Control Regs */
            case 10:
            case 11:
                DEBUG_SASSISTX(LCTL,MSGBUF(buf, "SASSIST LCTL Reject : PER CR%d Update",j));
                DEBUG_SASSISTX(LCTL,WRMSG(HHC90000, "D", buf));
                return(1);
            case 12:
            case 13: /* 12-13 : Unused */
                ocrs[j]=crs[j];
                rcrs[j]=crs[j];
                break;
            case 14:
            case 15: /* 14-15 Machine Check & I/O Logout control (plus DAS) */
                ocrs[j]=crs[j];
                break;
            default:
                break;
        }
    }
    /* Update REAL Control regs */
    for(i=0;i<16;i++)
    {
        regs->CR_L(i)=rcrs[i];
    }
    /* Update ECBLOK/VMBLOK Control regs */
    /* Note : if F_ECBLOK addresses VMVCR0 in the VMBLOCK */
    /*        check has already been done to make sure    */
    /*        r1=0 and numcrs=1                           */
    for(j=r1,i=0;i<numcrs;i++,j++)
    {
        if(j>15)
        {
            j-=16;
        }
        ecb_p=MADDR(F_ECBLOK+(j*4),USE_REAL_ADDR,regs,ACCTYPE_WRITE,0);
        store_fw(ecb_p,ocrs[j]);
    }
    DEBUG_SASSISTX(LCTL,MSGBUF(buf, "SASSIST LCTL %d,%d Done",r1,r3));
    DEBUG_SASSISTX(LCTL,WRMSG(HHC90000, "D", buf));
    SASSIST_HIT(LCTL);
    return 0;
}

/* IUCV Assist */
/* Not supported */
int ecpsvm_doiucv(REGS *regs,int b2,VADR effective_addr2)
{
    SASSIST_PROLOG(IUCV);

    UNREFERENCED(b2);
    UNREFERENCED(effective_addr2);
    return(1);
}

/* DIAG Assist */
/* Not supported */
int ecpsvm_dodiag(REGS *regs,int r1,int r3,int b2,VADR effective_addr2)
{
    SASSIST_PROLOG(DIAG);
    UNREFERENCED(r1);
    UNREFERENCED(r3);
    UNREFERENCED(b2);
    UNREFERENCED(effective_addr2);
    return(1);
}

/*  B1 - LRA Instruction Assist */

/*  Supporting the LRA instruction assist is necessarily complicated due to the need to
    process through two sets of DAT tables: the real DAT tables and the virtual DAT tables.

    The real DAT tables belong to CP and are pointed to by MICRSEG and VMSEG.  These map
    the 2nd level virtual storage of the virtual machine user.  This storage appears to
    be "real" storage to the virtual machine.

    The virtual DAT tables belong to the guest running in the virtual machine.  The tables
    are built and maintained by the guest and are pointed to by the guest's CR1 (the virtual
    CR1).  These tables of course map the guest's virtual storage and is considered 3rd
    level storage.

    The LRA needs to translate the virtual address in the guest's virtual storage to what
    appears to be a real address to the guest (but is actually 2nd level storage).  This means
    that the assist must use the virtual DAT tables to perform the translation.  Because the
    virtual DAT tables are in 2nd level storage we need to refer to the real DAT tables (via
    MICRSEG) to locate the real page frames in 1st level CP storage that contain the virtual
    DAT tables so we can reference them here.

    Further, CP may have paged out a guest machine page containing the virtual DAT tables
    referenced.  This means the page invalid bit would be on in the real DAT table entry (for
    example).  When something like this is encountered, exit to CP and let him do the LRA
    simulation.

    For VIRTUAL=REAL guests, the rules are somewhat different.  If the shadow table bypass
    assist is active, the guest's virtual DAT tables can be used to perform the LRA
    translation directly, as they will be resident in V=R storage and do not need a
    second translation reference via the real DAT tables in MICRSEG.  If shadow table
    bypass assist is not active, then LRA simulation proceeds using virtual and real DAT
    tables as described above.

    The simulation of LRA is vey complicated, and it is extremely helpful to refer to
    the IBM documentation GA22-7074-0 Virtual Machine Assist and Shadow Table Bypass Assist.
    */
int ecpsvm_dolra(REGS *regs,int r1,int b2,VADR effective_addr2)
{
    RADR  raddr;
    VADR  vmb;
    VADR  v_sto;
    VADR  r_sto;
    VADR  v_ste;
    VADR  r_ste;
    VADR  v_ste_ptr;
    VADR  r_ste_ptr;
    VADR  r_pto;
    VADR  r_pte_ptr;
    VADR  v_pto;
    VADR  v_pte_ptr;
    BYTE *cregs;
    U32   seglenchk;
    U32   cc;
    U32   vmsize;
    U32   cr0;
    U32   cr1;
    U32   frame_addr;
    U32   byte_addr;
    U32   pagshift;
    U32   frameshift;
    U32   bytemask;
    U32   segshift;
    U32   segmask;
    U32   pagmask;
    U32   offset_ste;
    U32   offset_pte;
    U32   v_segtbllen;
    U32   r_segtbllen;
    U32   lenindex;
    U32   pagindex;
    U16   r_pte;
    U16   v_pte;
    U16   paginvmask;
    U16   pagfmtmask;
    U16   framemask;

    SASSIST_PROLOG(LRA);

    UNREFERENCED(b2);

    if(!ECMODE(&vpregs.psw))
    {

        DEBUG_SASSISTX(LRA,WRMSG(HHC90000, "D", "SASSIST LRA Reject : VPSW is BC mode\n"));
        return(1);
    }

    /* Check to see if can perform the Shadow Table Bypass Assist version of LOAD
       REAL ADDRESS first.  The virtual machine user must have CP STBYPASS VR set active
       and the virtual PSW must be DAT enabled, or this assist will not be performed and
       instead will proceed on to the VMA version of the LOAD REAL ADDRESS assist.
    */
    if((micevma2 & (MICSTBVR | MICLRA2)) && (vpregs.psw.sysmask & 0x04))
    {
        cc = ecpsvm_int_lra(regs, effective_addr2, &raddr);
        regs->GR_L(r1) = raddr;
        regs->psw.cc = cc;
        DEBUG_SASSISTX(LRA,MSGBUF(buf, "SASSIST LRA Complete: CC=%d; Operands r1=%8.8X, d2(b2)=%8.8X (STBYPASS)\n",cc,regs->GR_L(r1),effective_addr2));
        DEBUG_SASSISTX(LRA,WRMSG(HHC90000, "D", buf));
        SASSIST_HIT(LRA);
        return(0);
    }

    /* Do VMA LOAD REAL ADDRESS assist */

    /* We need to translate a 3rd level address (operand 2 of the LRA) to a 2nd level address.
       The translation must use the second level segment and page tables (the "virtual tables")
       but because these virtual tables are themselves in virtual storage, we will need to use
       the segment and page tables in first level storage (the "real tables") in order to locate
       them.

       These are the general steps that must be performed. If any validation or check fails,
       we exit to CP which can better deal with exceptions.

       1. Validate the paging architecture in virtual CR0 and CR1.
       2. Get the segment index of the 3rd level subject address.
       3. Check to ensure the segment index is not beyond the limit of the virtual tables.
       4. Compute the virtual address of the virtual STE for the 3rd level address.
       5. Get the segment index of the virtual STE address.
       6. Check to ensure the segment index is not beyond the limit of the real tables.
       7. Compute the address of the real STE that represents the real segment containing the virtual STE.
       8. Fetch the real STE; validate it's contents for format and segment-invalid.
       9. Get the page index of the virtual STE address.
      10. Check to ensure the page index is not beyond the limit of the real page table pointed to
          by the real STE fetched in step 8.
      11. Compute the address of the real PTE that represents the real page containing the virtual STE.
      12. Fetch the real PTE; validate its contents for format and page-invalid.
      13. Get the real page frame address from the real PTE.
      14. Compute the real address of the virtual STE.
      15. Fetch the virtual STE.  Validate its format and segment-invalid bit.
      16. Extract from the virtual STE the virtual address of the virtual page table.
      17. Get the page index from the 3rd level subject address.
      18. Check to ensure that the page index is not beyond the limit of the virtual page table.
      19. Use the segment index of the virtual PTE address to compute the address of the real STE
          that represents the real segment containing the virtual page table.
      20. Check to ensure the segment index is not beyond the length of the tables.
      21. Fetch the real STE; validate it's contents for format and segment-invalid.
      22. Get the page index of the virtual PTE address.
      23. Check to ensure the page index is not beyond the limit of the real page table pointed to
          by the real STE fetched in step 21.
      24. Compute the address of the real PTE that represents the real page containing the virtual PTE.
      25. Fetch the real PTE; validate its contents for format and page-invalid.
      26. Get the real page frame address from the real PTE.
      27. Compute the real address of the virtual PTE.
      28. Fetch the virtual PTE.  Validate its format and page-invalid bit.
      29. Fetch the virtual page frame address from the virtual PTE.
      30. Combine the virtual page frame address with the byte-index value of the 3rd level address.
      31. Return the translated 2nd level address from step 30 in the LRA r1 operand and set CC=0.
    */


    /* Get virtual control regs 0 and 1 */
    cregs=MADDR(micblok.MICCREG,USE_REAL_ADDR,regs,ACCTYPE_READ,0);
    FETCH_FW(cr0,cregs);
    FETCH_FW(cr1,cregs+4);

    /* Separate out the segment table lengths and segment table origins in V-CR1 and MICRSEG */
    vmb = vpswa-0xA8;
    vmsize = EVM_L(vmb+VMSIZE);
    v_segtbllen = (cr1 & 0xff000000) >> 24;
    v_sto = cr1 & 0x00ffffc0;
    r_segtbllen = (micblok.MICRSEG & 0xff000000) >> 24;
    r_sto = micblok.MICRSEG & 0x00ffffc0;

    if(v_sto + ((v_segtbllen + 1) * 64) > vmsize)
    {
        DEBUG_SASSISTX(LRA,WRMSG(HHC90000, "D", "SASSIST LRA Reject : Invalid STO in virtual CR1\n"));
        return(1);
    }

                      // cr0 bits 8, 9, 10, 11, 12   (bits 10,12 must always be zero)
#define P2KS64K  8    //          0  1   0   0   0  = page2k, segment64k
#define P4KS64K 16    //          1  0   0   0   0  = page4k, segment64k
#define P2KS1M  10    //          0  1   0   1   0  = page2k, segment1M
#define P4KS1M  18    //          1  0   0   1   0  = page4k, segment1M

    /* Get the paging architecture bits from V-CR0, then shift to bits 27-31.
       Then load the appropriate settings for the defined paging architecture.
       The shift amounts are 1 or 2 bits less for the page index and segment index,
       respectively.  This "pre-multiplies" the index result so it can be directly
       be used as an index into the start of the page or segment table.
    */
    cr0 = (cr0 & 0x00f80000) >> 19;
    switch (cr0)
    {
        case P2KS64K:
            pagshift = 10;          /* right shift amt to get page index from a virtual address        */
            pagmask = 0x0000f800;   /* mask to extract the page index from a virtual address           */
            paginvmask = 0x0004;    /* mask to test page invalid bit                                   */
            pagfmtmask = 0x0002;    /* must-be-zero bits in a PTE to ensure valid format               */
            frameshift = 8;         /* left shift amt to left justify the page frame addr in a PTE     */
            framemask = 0xfff8;     /* mask to extract the page frame addr in a PTE                    */
            bytemask = 0x000007ff;  /* mask to extract the byte index from a virtual address           */
            segshift = 14;          /* right shift amt to get segment index from a virt address        */
            segmask = 0x00ff0000;   /* mask to extract the segment index from a virtual address        */
            seglenchk = TRUE;       /* indicates that a segment table length check is required         */
            break;

        case P4KS64K:
            pagshift = 11;
            pagmask = 0x0000f000;
            paginvmask = 0x0008;
            pagfmtmask = 0x0006;
            frameshift = 8;
            framemask = 0xfff0;
            bytemask = 0x00000fff;
            segshift = 14;
            segmask = 0x00ff0000;
            seglenchk = TRUE;
            break;

        case P2KS1M:
            pagshift = 10;
            pagmask = 0x0000f800;
            paginvmask = 0x0004;
            pagfmtmask = 0x0002;
            frameshift = 8;
            framemask = 0xfff8;
            bytemask = 0x000007ff;
            segshift = 18;
            segmask = 0x00f00000;
            seglenchk = FALSE;
            break;

        case P4KS1M:
            pagshift = 11;
            pagmask = 0x0000f000;
            paginvmask = 0x0008;
            pagfmtmask = 0x0006;
            frameshift = 8;
            framemask = 0xfff0;
            bytemask = 0x00000fff;
            segshift = 18;
            segmask = 0x00f00000;
            seglenchk = FALSE;
            break;

        /* Bad architecture bits in cr0; let CP handle this LRA */
        default:
            DEBUG_SASSISTX(LRA,WRMSG(HHC90000, "D", "SASSIST LRA Reject : Invalid page/segment bits in virtual CR0\n"));
            return(1);
            break;
    }

    /* For 64K segments only, we need to validate that the segment table length in virtual CR1
       is not less than the segment index of bits 8-11 of the LRA second operand address (i.e.,
       does the virtual address exceed the tables?).  If it is less, then we need to return cc=3
       and also return in register operand 1 the address of the virtual STE that would have been
       referred to had the length violation not occurred.
       Per LOAD REAL ADDRESS assist step #6, documented in GA22-7074-0.
    */
    if(seglenchk)
    {
        lenindex = (effective_addr2 & 0x00f00000) >> 20;
        if(v_segtbllen < lenindex)
        {
            offset_ste = (effective_addr2 & segmask) >> segshift;
            regs->GR_L(r1) = v_sto + offset_ste;
            regs->psw.cc = 3;
            DEBUG_SASSISTX(LRA,MSGBUF(buf, "SASSIST LRA Complete: CC=3; Operands r1=%8.8X, d2(b2)=%8.8X\n",regs->GR_L(r1),effective_addr2));
            DEBUG_SASSISTX(LRA,WRMSG(HHC90000, "D", buf));
            SASSIST_HIT(LRA);
            return(0);
        }
    }

    /* Compute the virtual address of the virtual STE derived from the 2nd operand address */
    offset_ste = (effective_addr2 & segmask) >> segshift;
    v_ste_ptr = v_sto + offset_ste;

    /* For 64K segments only, we need to validate that the segment table length in MICRSEG
       is not less than the segment index of bits 8-11 of the virtual STE address.  If
       it is less, then we need let CP handle this LRA (virtual STE address is beyond the tables).
       Per LOAD REAL ADDRESS assist step #7, documented in GA22-7074-0.
    */
    if(seglenchk)
    {
        lenindex = (v_ste_ptr & 0x00f00000) >> 20;
        if(r_segtbllen < lenindex)
        {
            DEBUG_SASSISTX(LRA,WRMSG(HHC90000, "D", "SASSIST LRA Reject : V-STE address exceeds real segment table length\n"));
            return(1);
        }
    }

    /* Compute the real address of the real STE that tranlates the virtual STE address. */
    offset_ste = (v_ste_ptr & segmask) >> segshift;
    r_ste_ptr = r_sto + offset_ste;

    /* Get the real segment table entry that points to the segment that contains the V-STE.
       Let CP handle if "must-be-zero" bits are set (including the segment invalid bit).
    */
    r_ste = EVM_L(r_ste_ptr);
    if (r_ste & 0x0F000007)
    {
        DEBUG_SASSISTX(LRA,MSGBUF(buf, "SASSIST LRA Reject : Real STE at %6.6X is invalid; STE=%8.8X\n",r_ste_ptr,r_ste));
        DEBUG_SASSISTX(LRA,WRMSG(HHC90000, "D", buf));
        return(1);
    }

    /* Get the length of the real page table from the real STE.  Ensure that the leftmost 4 bits
       of the real page index of the virtual STE address is not greater than the real page table
       length; if so, it's CP's problem.  Use hardcoded shift of 12 to compute real pagindex
       because CP's page frames are always 4K.
    */
    lenindex = (r_ste & 0xF0000000) >> 28;
    pagindex = (v_ste_ptr & 0x0000F000) >> 12;
    if (lenindex < pagindex)
    {
        DEBUG_SASSISTX(LRA,WRMSG(HHC90000, "D", "SASSIST LRA Reject : V-STE address exceeds real page table length\n"));
        return(1);
    }

    /* Compute the real page table origin for this real segment.  Fetch the real PTE and
       exit to CP if it is in an invalid format or if the page-invalid bit is set.
    */
    r_pto = (r_ste & 0x00FFFFF8);
    r_pte_ptr = r_pto + pagindex * 2;
    r_pte = EVM_LH(r_pte_ptr);
    if (r_pte & (paginvmask | pagfmtmask))
    {
        DEBUG_SASSISTX(LRA,MSGBUF(buf, "SASSIST LRA Reject : Real PTE at %6.6X is invalid; PTE=%4.4X\n",r_pte_ptr,r_pte));
        DEBUG_SASSISTX(LRA,WRMSG(HHC90000, "D", buf));
        return(1);
    }

    /* r_pte now contains the page table entry containing the real address of the page
       frame containing the virtual STE.  Compute the real address of the virtual STE
       entry pointed to by v_ste_ptr and fetch the virtual STE entry
    */
    frame_addr = (U32) ((r_pte & framemask) << frameshift);
    byte_addr  = v_ste_ptr & 0x0FFF;
    v_ste = EVM_L(frame_addr + byte_addr);

    /* Validate the virtual STE entry.  If the segment invalid bit is set, place the
       virtual address of this virtual STE in register operand 1 and return with the
       LRA completed and psw cc=1.  If the STE format is incorrect, return to CP to handle.
    */
    if (v_ste & 0x00000001)
    {
        regs->GR_L(r1) = v_ste_ptr;
        regs->psw.cc = 1;
        DEBUG_SASSISTX(LRA,MSGBUF(buf, "SASSIST LRA Complete: CC=1; Operands r1=%8.8X, d2(b2)=%8.8X\n",regs->GR_L(r1),effective_addr2));
        DEBUG_SASSISTX(LRA,WRMSG(HHC90000, "D", buf));
        SASSIST_HIT(LRA);
        return(0);
    }
    if (v_ste & 0x0F000006)
    {
        DEBUG_SASSISTX(LRA,MSGBUF(buf, "SASSIST LRA Reject : Virtual STE at %6.6X is invalid; STE=%8.8X\n",v_ste_ptr,v_ste));
        DEBUG_SASSISTX(LRA,WRMSG(HHC90000, "D", buf));
        return(1);
    }

    /* Get the length of the virtual page table from the virtual STE.  Ensure that the leftmost 4 bits
       of the virtual page index of the LRA second operand address is not greater than the virtual
       page table length; if it is less, then we need to return cc=3 and also return in register
       operand 1, the address of the virtual PTE that would have been referred to had the length
       violation not occurred.  Per LOAD REAL ADDRESS assist step #15, documented in GA22-7074-0.
    */
    lenindex = (v_ste & 0xF0000000) >> 28;
    pagindex = (effective_addr2 & 0x0000F000) >> 12;
    v_pto = v_ste & 0x00fffff8;
    offset_pte = (effective_addr2 & pagmask) >> pagshift;
    if (lenindex < pagindex)
    {
            regs->GR_L(r1) = v_pto + offset_pte;
            regs->psw.cc = 3;
            DEBUG_SASSISTX(LRA,MSGBUF(buf, "SASSIST LRA Complete: CC=3; Operands r1=%8.8X, d2(b2)=%8.8X\n",regs->GR_L(r1),effective_addr2));
            DEBUG_SASSISTX(LRA,WRMSG(HHC90000, "D", buf));
            SASSIST_HIT(LRA);
            return(0);
    }

    /* Compute the virtual address of the virtual PTE derived from the virtual STE + virtual page index */
    v_pte_ptr = v_pto + offset_pte;

    /* For 64K segments only, we need to validate that the segment table length in MICRSEG
       is not less than the segment index of bits 8-11 of the virtual PTE address.  If
       it is less, then we need let CP handle this LRA (virtual PTE address is beyond the tables).
       Per LOAD REAL ADDRESS assist step #16, documented in GA22-7074-0.
    */
    if(seglenchk)
    {
        lenindex = (v_pte_ptr & 0x00f00000) >> 20;
        if(r_segtbllen < lenindex)
        {
            DEBUG_SASSISTX(LRA,WRMSG(HHC90000, "D", "SASSIST LRA Reject : V-PTE address exceeds real segment table length\n"));
            return(1);
        }
    }

    /* Compute the real address of the real STE that tranlates the virtual PTE address. */
    offset_ste = (v_pte_ptr & segmask) >> segshift;
    r_ste_ptr = r_sto + offset_ste;

    /* Get the real segment table entry that points to the segment that contains the V-PTE.
       Let CP handle if "must-be-zero" bits are set (including the segment invalid bit).
    */
    r_ste = EVM_L(r_ste_ptr);
    if (r_ste & 0x0F000007)
    {
        DEBUG_SASSISTX(LRA,MSGBUF(buf, "SASSIST LRA Reject : Real STE at %6.6X is invalid; STE=%8.8X\n",r_ste_ptr,r_ste));
        DEBUG_SASSISTX(LRA,WRMSG(HHC90000, "D", buf));
        return(1);
    }

    /* Get the length of the real page table from the real STE.  Ensure that the leftmost 4 bits
       of the real page index of the virtual STE address is not greater than the real page table
       length; if so, it's CP's problem.  Use hardcoded shift of 12 to compute real pagindex
       because CP's page frames are always 4K.
    */
    lenindex = (r_ste & 0xF0000000) >> 28;
    pagindex = (v_pte_ptr & 0x0000F000) >> 12;
    if (lenindex < pagindex)
    {
        DEBUG_SASSISTX(LRA,WRMSG(HHC90000, "D", "SASSIST LRA Reject : V-PTE address exceeds real page table length\n"));
        return(1);
    }

    /* Compute the real page table origin for this real segment.  Fetch the real PTE and
       exit to CP if it is in an invalid format or if the page-invalid bit is set.
       Per LOAD REAL ADDRESS assist steps #19-20, documented in GA22-7074-0.
    */
    r_pto = (r_ste & 0x00FFFFF8);
    r_pte_ptr = r_pto + pagindex * 2;
    r_pte = EVM_LH(r_pte_ptr);
    if (r_pte & (paginvmask | pagfmtmask))
    {
        DEBUG_SASSISTX(LRA,MSGBUF(buf, "SASSIST LRA Reject : Real PTE at %6.6X is invalid; PTE=%4.4X\n",r_pte_ptr,r_pte));
        DEBUG_SASSISTX(LRA,WRMSG(HHC90000, "D", buf));
        return(1);
    }

    /* r_pte now contains the page table entry containing the real address of the page
       frame containing the virtual PTE.  Compute the real address of the virtual PTE
       pointed to by v_pte_ptr and fetch the virtual PTE
    */
    frame_addr = (U32) ((r_pte & framemask) << frameshift);
    byte_addr  = v_pte_ptr & 0x0FFF;
    v_pte = EVM_LH(frame_addr + byte_addr);

    /* Validate the virtual PTE entry.  If the page invalid bit is set, place the
       virtual address of this virtual PTE in register operand 1 and return with the
       LRA completed and psw cc=2.  If the PTE format is incorrect, return to CP to handle.
    */
    if (v_pte & paginvmask)
    {
        regs->GR_L(r1) = v_pte_ptr;
        regs->psw.cc = 2;
        DEBUG_SASSISTX(LRA,MSGBUF(buf, "SASSIST LRA Complete: CC=2; Operands r1=%8.8X, d2(b2)=%8.8X\n",regs->GR_L(r1),effective_addr2));
        DEBUG_SASSISTX(LRA,WRMSG(HHC90000, "D", buf));
        SASSIST_HIT(LRA);
        return(0);
    }
    if (v_pte & pagfmtmask)
    {
        DEBUG_SASSISTX(LRA,MSGBUF(buf, "SASSIST LRA Reject : Virtual PTE at %6.6X is invalid; STE=%8.8X\n",v_pte_ptr,v_pte));
        DEBUG_SASSISTX(LRA,WRMSG(HHC90000, "D", buf));
        return(1);
    }

    /* v_pte contains the page frame address we need to finally resolve the LRA. */
    frame_addr = (U32) ((v_pte & framemask) << frameshift);
    byte_addr  = effective_addr2 & bytemask;
    regs->GR_L(r1) = frame_addr + byte_addr;
    regs->psw.cc = 0;
    DEBUG_SASSISTX(LRA,MSGBUF(buf, "SASSIST LRA Complete: CC=0; Operands r1=%8.8X, d2(b2)=%8.8X\n",regs->GR_L(r1),effective_addr2));
    DEBUG_SASSISTX(LRA,WRMSG(HHC90000, "D", buf));
    SASSIST_HIT(LRA);
    return(0);
}

/* Misc functions for statistics */

static int ecpsvm_sortstats( const void *a, const void *b )
{
    ECPSVM_STAT *ea, *eb;

    ea = (ECPSVM_STAT*) a;
    eb = (ECPSVM_STAT*) b;

    return (int) (eb->call - ea->call);
}

static void ecpsvm_showstats2( ECPSVM_STAT *ar, size_t count )
{
    char nname[32];
    U64  callt    = 0;
    U64  hitt     = 0;
    U64  unsupcc  = 0;
    U64  notshown = 0;

    size_t i;

    for (i=0; i < count; i++)
    {
        if (ar[i].call)
        {
            callt += ar[i].call;
            hitt  += ar[i].hit;

            if (!ar[i].support)
                unsupcc += ar[i].call;

            MSGBUF( nname, "%s%s", ar[i].name, ar[i].support ? "" : "*" );

            if (!ar[i].enabled) STRLCAT( nname, "-" );
            if ( ar[i].debug)   STRLCAT( nname, "%" );
            if ( ar[i].total)   STRLCAT( nname, "+" );

            // "| %-9s | %10"PRIu64" | %10"PRIu64" |  %3"PRIu64"%% |"
            WRMSG( HHC01701, "I", nname, ar[i].call, ar[i].hit,
                    ar[i].call ? (ar[i].hit * 100) / ar[i].call : 0 );
        }
        else
            notshown++;
    }

    if (callt)
    {
        // "+-----------+------------+------------+-------+"
        WRMSG( HHC01702, "I" );
    }
    // "| %-9s | %10"PRIu64" | %10"PRIu64" |  %3"PRIu64"%% |"
    WRMSG( HHC01701, "I", "Total", callt, hitt,
            callt ? (hitt * 100) / callt : 0 );
    // "+-----------+------------+------------+-------+"
    WRMSG( HHC01702, "I" );

    if (unsupcc)
    {
        // "* : Unsupported, - : Disabled, %% - Debug"
        WRMSG( HHC01703, "I" );
    }
    if (notshown)
    {
        // "%"PRIu64" entries not shown and never invoked"
        WRMSG( HHC01704, "I", notshown );
    }
    if (unsupcc)
    {
        // "%"PRIu64" call(s) were made to unsupported functions"
        WRMSG( HHC01705, "I", unsupcc );
    }
    return;
}

/* SHOW STATS */
void ecpsvm_showstats( int ac, char **av )
{
    size_t      asize;
    ECPSVM_STAT *ar;

    UNREFERENCED( ac );
    UNREFERENCED( av );

    WRMSG( HHC01725, "I", ECPSCODEVER);     // ECPS:VM Code version %.02f
    if (sysblk.ecpsvm.freetrap)
    {
        // "ECPS:VM Operating with CP FREE/FRET trap in effect"
        WRMSG( HHC01724, "I" );
    }

    // "+-----------+------------+------------+-------+"
    WRMSG( HHC01702, "I" );
    // "| %-9s | %10s | %10s | %-5s |"
    WRMSG( HHC01706, "I", "VM ASSIST", "Calls  ", "Hits  ", "Ratio" );
    // "+-----------+------------+------------+-------+"
    WRMSG(HHC01702,"I");

    ar = malloc( sizeof( ecpsvm_sastats ));
    memcpy( ar, &ecpsvm_sastats, sizeof( ecpsvm_sastats ));
    asize = sizeof( ecpsvm_sastats ) / sizeof( ECPSVM_STAT );
    qsort( ar, asize, sizeof( ECPSVM_STAT ), ecpsvm_sortstats );
    ecpsvm_showstats2( ar, asize );
    free( ar );

    // "+-----------+------------+------------+-------+"
    WRMSG( HHC01702, "I" );
    // "| %-9s | %10s | %10s | %-5s |"
    WRMSG( HHC01706, "I", "CP ASSIST", "Calls  ", "Hits  ", "Ratio" );
    // "+-----------+------------+------------+-------+"
    WRMSG( HHC01702, "I" );

    ar = malloc( sizeof( ecpsvm_cpstats ));
    memcpy( ar, &ecpsvm_cpstats, sizeof( ecpsvm_cpstats ));
    asize = sizeof( ecpsvm_cpstats ) / sizeof( ECPSVM_STAT );
    qsort( ar, asize, sizeof( ECPSVM_STAT ), ecpsvm_sortstats );
    ecpsvm_showstats2( ar, asize );
    free( ar );
}

ECPSVM_STAT *ecpsvm_findstat(char *feature,char **fclass)
{
    ECPSVM_STAT *es;
    ECPSVM_STAT *esl;
    int i;
    int fcount;
    fcount=sizeof(ecpsvm_sastats)/sizeof(ECPSVM_STAT);
    esl=(ECPSVM_STAT *)&ecpsvm_sastats;
    for(i=0;i<fcount;i++)
    {
        es=&esl[i];
        if(strcasecmp(feature,es->name)==0)
        {
            *fclass="VM ASSIST";
            return(es);
        }
    }
    esl=(ECPSVM_STAT *)&ecpsvm_cpstats;
    fcount=sizeof(ecpsvm_cpstats)/sizeof(ECPSVM_STAT);
    for(i=0;i<fcount;i++)
    {
        es=&esl[i];
        if(strcasecmp(feature,es->name)==0)
        {
            *fclass="CP ASSIST";
            return(es);
        }
    }
    return(NULL);
}

void ecpsvm_enadisaall(char *fclass,ECPSVM_STAT *tbl,size_t count,int onoff,int debug)
{
    ECPSVM_STAT *es;
    size_t i;
    char *enadisa,*debugonoff;
    enadisa=onoff?"Enabled":"Disabled";
    debugonoff=debug?"On":"Off";
    for(i=0;i<count;i++)
    {
        es=&tbl[i];
        if(onoff>=0)
        {
            es->enabled=onoff;
            WRMSG(HHC01707,"I",fclass,es->name," ", enadisa);
        }
        if(debug>=0)
        {
            es->debug=debug;
            WRMSG(HHC01707,"I",fclass,es->name," Debug ", debugonoff);
        }
    }
    if(onoff>=0)
    {
        WRMSG(HHC01708,"I",fclass,"",enadisa);
    }
    if(debug>=0)
    {
        WRMSG(HHC01708,"I",fclass,"Debug ",debugonoff);
    }
}

void ecpsvm_enable_disable(int ac,char **av,int onoff,int debug)
{
    char        *fclass;
    char        *enadisa,*debugonoff;
    int i;

    size_t      sacount,cpcount;
    ECPSVM_STAT *es;
    ECPSVM_STAT *sal,*cpl;

    sal=(ECPSVM_STAT *)&ecpsvm_sastats;
    cpl=(ECPSVM_STAT *)&ecpsvm_cpstats;
    sacount=sizeof(ecpsvm_sastats)/sizeof(ECPSVM_STAT);
    cpcount=sizeof(ecpsvm_cpstats)/sizeof(ECPSVM_STAT);

    enadisa=onoff?"Enabled":"Disabled";
    debugonoff=debug?"On":"Off";
    if(ac==1)
    {
        ecpsvm_enadisaall("VM ASSIST",sal,sacount,onoff,debug);
        ecpsvm_enadisaall("CP ASSIST",cpl,cpcount,onoff,debug);
        if(debug>=0)
        {
            sysblk.ecpsvm.debug=debug;
            WRMSG(HHC01709,"I",debugonoff);
        }
        return;
    }
    for(i=1;i<ac;i++)
    {
        if(strcasecmp(av[i],"ALL")==0)
        {
            ecpsvm_enadisaall("VM ASSIST",sal,sacount,onoff,debug);
            ecpsvm_enadisaall("CP ASSIST",cpl,cpcount,onoff,debug);
            return;
        }
        if(strcasecmp(av[i],"VMA")==0)
        {
            ecpsvm_enadisaall("VM ASSIST",sal,sacount,onoff,debug);
            return;
        }
        if(strcasecmp(av[i],"CPA")==0)
        {
            ecpsvm_enadisaall("CP ASSIST",cpl,cpcount,onoff,debug);
            return;
        }
        es=ecpsvm_findstat(av[i],&fclass);
        if(es!=NULL)
        {
            if(onoff>=0)
            {
                es->enabled=onoff;
                WRMSG(HHC01710,"I",fclass,es->name,"",enadisa);
            }
            if(debug>=0)
            {
                es->debug=debug;
                WRMSG(HHC01710,"I",fclass,es->name,"Debug ",debugonoff);
            }
        }
        else
        {
            WRMSG(HHC01711,"I",av[i]);
        }
    }
}

void ecpsvm_disable (int ac, char **av) { ecpsvm_enable_disable( ac, av,  0, -1 ); }
void ecpsvm_enable  (int ac, char **av) { ecpsvm_enable_disable( ac, av,  1, -1 ); }
void ecpsvm_debug   (int ac, char **av) { ecpsvm_enable_disable( ac, av, -1,  1 ); }
void ecpsvm_nodebug (int ac, char **av) { ecpsvm_enable_disable( ac, av, -1,  0 ); }

void ecpsvm_level(int ac,char **av)
{
    int lvl;

    WRMSG(HHC01712,"I",sysblk.ecpsvm.level);
    if(!sysblk.ecpsvm.available)
        WRMSG(HHC01713,"I");

    if( ac > 1 )
    {
        lvl=atoi(av[1]);
        WRMSG(HHC01714,"I",lvl);
        sysblk.ecpsvm.level=lvl;
    }

    if(sysblk.ecpsvm.level!=20)
    {
          WRMSG(HHC01715,"W",sysblk.ecpsvm.level);
          WRMSG(HHC01716,"I");
    }
}

static  void ecpsvm_helpcmd(int,char **);

static ECPSVM_CMDENT ecpsvm_cmdtab[]={

    {"Help",    1,ecpsvm_helpcmd,"Show help",
            "format : \"ecpsvm help [cmd]\" Shows help on the specified\n"
            "        ECPSVM subcommand"},

    {"STats",   2,ecpsvm_showstats,"Show statistical counters",
            "format : ecpsvm stats : Shows various ECPSVM Counters"},

    {"DIsable", 2,ecpsvm_disable,"Disable ECPS:VM Features",
            "format : ecpsvm disable [ALL|feat1[ feat2|...]"},

    {"ENable",  2,ecpsvm_enable,"Enable ECPS:VM Features",
            "format : ecpsvm enable [ALL|feat1[ feat2|...]"},

#if defined(DEBUG_SASSIST) || defined(DEBUG_CPASSIST)
    {"DEBUG",   5,ecpsvm_debug,"Debug ECPS:VM Features",
            "format : ecpsvm debug [ALL|feat1[ feat2|...]"},

    {"NODebug", 3,ecpsvm_nodebug,"Turn Debug off for ECPS:VM Features",
            "format : ecpsvm NODebug [ALL|feat1[ feat2|...]"},
#endif

    {"Level",   1,ecpsvm_level,"Set/Show ECPS:VM level",
            "format : ecpsvm Level [nn]\n"},

    {NULL,0,NULL,NULL,NULL}};

static void ecpsvm_helpcmdlist(void)
{
    int i;
    ECPSVM_CMDENT *ce;

    for(i=0;ecpsvm_cmdtab[i].name;i++)
    {
        ce=&ecpsvm_cmdtab[i];
        WRMSG(HHC01717,"I",ce->name,ce->expl);
    }
    return;
}

void ecpsvm_helpcmd(int ac,char **av)
{
    ECPSVM_CMDENT *ce;
    if(ac==1)
    {
        ecpsvm_helpcmdlist();
        return;
    }
    ce=ecpsvm_getcmdent(av[1]);
    if(ce==NULL)
    {
        WRMSG(HHC01718,"E",av[1]);
        ecpsvm_helpcmdlist();
        return;
    }
    WRMSG(HHC01717,"I",ce->name,ce->help);
    return;
}

ECPSVM_CMDENT *ecpsvm_getcmdent(char *cmd)
{
    ECPSVM_CMDENT *ce;
    int i;
    int clen;
    for(i=0;ecpsvm_cmdtab[i].name;i++)
    {
        ce=&ecpsvm_cmdtab[i];
        if(strlen(cmd)<=strlen(ce->name) && strlen(cmd)>=(size_t)ce->abbrev)
        {
            clen=(int)strlen(cmd);
            if(strncasecmp(cmd,ce->name,clen)==0)
            {
                return(ce);
            }
        }
    }
    return(NULL);
}

int ecpsvm_command( int ac, char **av )
{
    ECPSVM_CMDENT  *ce;

    // "ECPS:VM Command processor invoked"
    WRMSG( HHC01719, "I" );

    if (ac <= 1)
    {
        // "No ECPS:VM subcommand. Type \"ecpsvm help\" for a list of valid subcommands"
        WRMSG( HHC01720, "E" );
        return -1;
    }

    if (!(ce = ecpsvm_getcmdent( av[1] )))
    {
        // "Unknown ECPS:VM subcommand %s"
        WRMSG( HHC01721, "E", av[1] );
        return -1;
    }

    ce->fun( ac-1, av+1 );

    // "ECPS:VM Command processor complete"
    WRMSG( HHC01722, "I" );
    return 0;
}

#endif /* ifdef FEATURE_ECPSVM */

/* Note : The following forces inclusion for S/390 & z/ARCH */
/*        This is necessary just in order to properly define */
/*        S/390 auxialiary routines invoked by S/370 routines */
/*        because of SIE                                     */

#if !defined(_GEN_ARCH)

#if defined(_ARCH_NUM_1)
 #define  _GEN_ARCH _ARCH_NUM_1
 #include "ecpsvm.c"
#endif

#if defined(_ARCH_NUM_2)
 #undef   _GEN_ARCH
 #define  _GEN_ARCH _ARCH_NUM_2
 #include "ecpsvm.c"
#endif

#endif /*!defined(_GEN_ARCH)*/
