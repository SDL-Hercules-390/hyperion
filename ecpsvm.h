/*ECPSVM.H      (c) Copyright Roger Bowler, 2000-2012                */
/*              Hercules ECPS:VM Support                             */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#ifndef __ECPSVM_H__
#define __ECPSVM_H__

/* CR6 Definitions */
#define ECPSVM_CR6_VMASSIST 0x80000000         /* DO Privop Sim */
#define ECPSVM_CR6_VIRTPROB 0x40000000         /* Running user in Problem State */
#define ECPSVM_CR6_ISKINHIB 0x20000000         /* Inhibit ISK/SSK Sim */
#define ECPSVM_CR6_S360ONLY 0x10000000         /* Only S/360 Operations */
#define ECPSVM_CR6_SVCINHIB 0x08000000         /* No SVC sim */
#define ECPSVM_CR6_STVINHIB 0x04000000         /* No Shadow Table Validation */
#define ECPSVM_CR6_ECPSVM   0x02000000         /* ECPS:VM Enable */
#define ECPSVM_CR6_VIRTTIMR 0x01000000         /* Virtual Interval Timer update */
#define ECPSVM_CR6_MICBLOK  0x00FFFFF8         /* MICBLOK Address mask */
#define ECPSVM_CR6_VMMVSAS  0x00000004         /* VM Assists for MVS Enable (370E) */

/* CR6 Definitions (VMBLOK style) */
#define VMMFE 0x80
#define VMMPROB 0x40
#define VMMNOSK 0x20
#define VMM360 0x10
#define VMMSVC 0x08
#define VMMSHADT 0x04
#define VMMCPAST 0x02
#define VMMVTMR 0x01

/* MICBLOK */
typedef struct _ECPSVM_MICBLOK
{
    U32 MICRSEG;
    U32 MICCREG;
    U32 MICVPSW;
#define MICVIP MICVPSW
#define MICPEND 0x80
    U32 MICWORK;
    U32 MICVTMR;
    U32 MICACF;
    U32 RESERVED1;
    U32 RESERVED2;
    U32 MICCREG0;
    U32 RESERVED3;
    /* Bits defined in MICEVMA */
#define MICLPSW 0x80    /* LPSW SIM */
#define MICPTLB 0x40    /* PTLB SIM */
#define MICSCSP 0x20    /* SCKC, SPT SIM */
#define MICSIO  0x10    /* SIO, SIOF SIM */
#define MICSTSM 0x08    /* SSM, STNSM, STOSM SIM */
#define MICSTPT 0x04    /* STPT SIM */
#define MICTCH  0x02    /* TCH SIM */
#define MICDIAG 0x01    /* DIAG SIM */
    /* Hint : The following bits may be irrelevant for ECPS:VM Level 20 */
    /* Bits defined in MICEVMA2 */
    /* V=R Shadow Table Bypass assists */
#define MICSTBVR 0x80   /* V=R STBYPASS Assist active */
#define MICPTLB2 0x40   /* VRSTBYPASS PTLB Simulation */
#define MICIPTP2 0x20   /* VRSTBYPASS IPTE/TPRT Simulation */
#define MICVPFR2 0x10   /* Virtual Page Fault reflection Assists */
#define MICLRA2  0x08   /* VRSTBYPASS LRA Simulation */
#define MICSTSM2 0x02   /* VRSTBYPASS SSM/STxSM Sim */
#define MICLCTL2 0x01   /* VRSTBYPASS LCTL Sim */
    /* Bits define in MICEVMA3 */
#define MICSKYMD 0x20   /* Unknown */
#define MICISKE  0x10   /* PTLB Sim */
#define MICRRBE  0x08   /* IPTE/TPRT Sim */
#define MICSSKE  0x04   /* V Page Fault Sim */
    /* Bits defined in MICEVMA4 */
#define MICSVC4  0x40   /* SVC/LPSW/LCTL Assists Extentions */
#define MICSPT4  0x20   /* SPT Assist Extension */
#define MICIUCV  0x10   /* IUCV ASSIST */
} ECPSVM_MICBLOK;

/* PSA Useful Values */
#define PGMOPSW  0x028
#define IOOPSW   0x038
#define PGMINT   0x08C

#define QUANTUMR 0x04C  /* Usefull little unused space for MVCing NEW ITIMER */
#define INTTIMER 0x050
#define QUANTUM  0x054  /* Usefull little unused space for MVCing OLD ITIMER */

/* PSA Displacements used by ECPS:VM */
/* PSA Entries used by DISPx Instructions */
#define CPSTATUS 0x348
/* PSA + 348 : CPSTATUS */
#define CPWAIT 0x80     /* CP IN WAIT STATE */
#define CPRUN  0x40     /* RUNUSER is use running */
#define CPEX   0x20     /* CPEXBLOK RUNNING */
#define CPSUPER 0x08    /* Supervisor State */

/* PSA + 349 : SYSTEM EXTENDING FREE STORAGE BYTE */
/* Note : PSA+0X349 = 0xFF if storage extending */
#define XTENDLOCK 0x349
#define XTENDLOCKSET 0xff

/* PSA + 34A : CPSTAT2 */
#define CPSTAT2 0x34A
#define CPMICAVL 0x80
#define CPMICON  0x40
#define CPSHRLK 0x20
#define CPASTAVL 0x08
#define CPASTON 0x04

/* RUNPSW */
/* PSW used during dispatch */
#define RUNPSW 0x330

/* RUNUSER PSA+338 */
#define RUNUSER 0x338

/* RUNCR0, RUNCR1 */
/* CR0 & CR1 used during dispatch */
#define RUNCR0 0x340
#define RUNCR1 0x344

/* ASYSVM : PSA+37C */
#define ASYSVM 0x37C

/* PSA + X'3D0'-X'3EC' - ASSISTS STUFF */
#define APAGCP   0x3D0
#define CPCREG0  0x3D4
#define CPCREG6  0x3D8
#define CPCREG8  0x3DC
#define TIMEDISP 0x3E0
#define ASVCLIST 0x3E4
#define AVMALIST 0x3E8
#define LASTUSER 0x3EC

/* PSA + 328 : PROBTIME */
/* Total time spent in problem state (2's complement) */
#define PROBTIME 0x328

/* PSA + 69B : APSTAT2 - Machine check recov & PTLB Required */
#define APSTAT2  0x69B     /* 2017-01-18 corrected offset from 0x69D to 0x69B */
#define CPMCHLK  0x10
#define CPPTLBR  0x02

/* PSA + 6A8 : PROBSTRT */
/* TOD Clock at Proble state entry */
#define PROBSTRT 0x6A8

/* PSA + 6D0 : STACKVM - GPR11 Backup for dispatcher */
#define STACKVM 0x6D0

/* CP ASSIST SVC (Not VM Assist SVC) LIST pointed to by ASVCLIST */
/* ASSISTS FOR CP LINK/RETURN SVCs */
/* ASVCLIST points to DMKSVCNS */
/* Displacements from DMKSVCNS (defined in DMKSVC) */
#define NEXTSAVE  0        /* Pointer to next Save Area */
#define SLCADDR   4        /* V=R Start */
#define DMKSVCHI  8        /* DMKFREHI */
#define DMKSVCLO 12        /* DMKFRELO - SAVEAREA LENGTH */


/* VM ASSIST LISTS */
/* ENTRYPOINT TO VARIOUS PRIVOP SIM FASTPATH */
/* AVMALIST points to DMKPRVMA */
/* Displacements from DMKPRVMA (defined in DMKPRV) */
#define VSIVS 0     /* EP To DMKVSIVS (Fastpath SIO/SIOF) */
#define VSIEX 4     /* Base addr for VSIVS */
#define DSPCH 8     /* Scheduler - Fast path for LPSW/SSM/STNSM/STOSM */
#define TMRCC 12    /* SCKC EP */
#define TMR   16    /* Timer ops base */
#define TMRSP 20    /* SPT EP */
#define VATAT 24    /* ARCHITECT */
#define DSPB  28    /* Slow Path Dispatcher - PSW Revalidate required */
#define PRVVS 32    /* VSIVS COUNT */
#define PRVVL 36    /* LPSW Count */
#define PRVVM 40    /* SSM/STxSM COUNT */
#define PRVVC 44    /* SCKC COUNT */
#define RESERVED 48;
#define PRVVP 52;   /* SPT COUNT */


/* CP 'SAVEAREA' block definitions */
#define SAVENEXT  0
#define SAVERETN  0
#define SAVEREGS  0x10
#define SAVER12   0x04
#define SAVER13   0x08
#define SAVER8    0x30
#define SAVER10   0x38

/* Fields for CCW translation (part of SAVEAREA dsect in DMKCCW) */

#define PRVCOMND  0x0C
#define PRVFLAG   0x0D
/* Bits defined in PRVFLAG */
#define SMCOM     0x01
#define FWDTIC    0x04

#define VIRCOMND  0x0E
#define VIRFLAG   0x0F
#define MEMO1     0x41
/* Bits defined in MEMO1 */
#define HADISAM   0x01
#define HADUTIC   0x02

#define MEMO2     0x42
/* Bits defined in MEMO2 */
#define STRTNEW   0x40

#define FIRSTRCW  0x44
#define THISRCW   0x58
#define DEVTABLE  0x50

/* CCW related bits */
#define CD        0x80
#define CC        0x40
#define IDA       0x04
#define CDTIC     0x18

/* RCWCCW block definitions */
#define RCWPNT    0x00
#define RCWCOMND  0x00
#define RCWVCAW   0x04
#define RCWFLAG   0x04
#define RCWCTL    0x05
/* Bits in RCWCTL */
#define RCWIO     0x80
#define RCWGEN    0x40
#define RCWHMR    0x20
#define RCW2311   0x04
#define RCWSHR    0x01

#define RCWCNT    0x06
#define RCWVCNT   0x08
#define RCWRCNT   0x0A
#define RCWCCNT   0x0E
#define RCWHSIZ   0x10

/* CORTABLE definitions */
#define CORFLAG   0x08
/* Bits defined in CORFLAG */
#define CORSHARE  0x08

/* IOBLOK Displacements */
#define IOBCAW    0x20
#define IOBSPEC2  0x39
/* Flags in IOBSPEC2 */
#define IOBUNREL  0x20
#define IOBCLN    0x40

/* VMBLOK Displacements */
#define VMQFPNT         0x000
#define VMQBPNT         0x004
#define VMPNT           0x008
#define VMECEXT         0x00C
#define VMVCR0  VMECEXT
#define VMSEG           0x010
#define VMSIZE          0x014
#define VMCHSTRT        0x018
#define VMCUSTRT        0x01C
#define VMDVSTRT        0x020
#define VMTERM          0x024
#define VMVTERM         0x028
#define VMTRMID         0x02A
#define VMTLEND         0x02C
#define VMTLDEL         0x02D
#define VMTCDEL         0x02E
#define VMTESCP         0x02F
#define VMCHCNT         0x030
#define VMCUCNT         0x032
#define VMDVCNT         0x034
#define VMIOACTV        0x036
#define VMCHTBL         0x038
#define VMRSTAT         0x058
    /* Flags defined in VMRSTAT */
#define VMCFWAIT 0x80
#define VMPGWAIT 0x40
#define VMIOWAIT 0x20
#define VMPSWAIT 0x10
#define VMEXWAIT 0x08
#define VMLOGON  0x04
#define VMLOGOFF 0x02
#define VMIDLE   0x01
#define VMCPWAIT (VMCFWAIT|VMPGWAIT|VMIOWAIT|VMEXWAIT|VMLOGOFF|VMLOGON)
#define VMNORUN  (VMCPWAIT|VMPSWAIT)
#define VMLONGWT (VMCFWAIT|VMLOGON|VMLOGOFF|VMIDLE)

#define VMDSTAT         0x059
    /* Flags defined in VMDSTAT */
#define VMDSP   0x80
#define VMTSEND 0x40
#define VMQSEND 0x20
#define VMTIO   0x10
#define VMRUN   0x08
#define VMINQ   0x04
#define VMELIG  0x02
#define VMDSPCH 0x01

#define VMOSTAT         0x05A
    /* Flags defined in VMOSTAT */
#define VMSYSOP 0x80
#define VMSHR   0x40
#define VMSLEEP 0x20
#define VMDISC  0x10
#define VMCFRUN 0x08
#define VMVIRCF 0x04
#define VMCF    0x02
#define VMKILL  0x01

#define VMQSTAT         0x05B
    /* Flags defined in VMQSTAT */
#define VMPRIDSP 0x80
#define VMAUTOLOG 0x40
#define VMWSERNG 0x20
#define VMDLDRP 0x10
#define VMWSCHG 0x08
#define VMINHMIG 0x04
#define VMCFREAD 0x02
#define VMPA2APL 0x01

#define VMPSTAT         0x05C
    /* Flags defined in VMPSTAT */
#define VMISAM 0x80
#define VMV370R 0x40
#define VMRPAGE 0x20
#define VMREAL 0x10
#define VMNOTRAN 0x08
#define VMNSHR 0x04
#define VMACCOUN 0x02
#define VMPAGEX 0x01

#define VMESTAT         0x05D
    /* Flags defined in VMESTAT */
#define VMSHADT 0x80
#define VMPERCM 0x40
#define VMBADCR0 0x20
#define VMMICSVC 0x10
#define VMEXTCM 0x08
#define VMNEWCR0 0x04
#define VMINVSEG 0x02
#define VMINVPAG 0x01
#define VMECZAP ~VMMICSVC

#define VMTRCTL         0x05E
    /* Bits defined in VMTRCTL */
#define VMTRPER 0x80
#define VMTRSVC 0x40
#define VMTRPRG 0x20
#define VMTRIO  0x10
#define VMTREX  0x08
#define VMTRPRV 0x04
#define VMTRSIO 0x02
#define VMTRBRIN 0x01
#define VMTRINT (VMTRSVC|VMTRPRG|VMTRIO|VMTREX)

#define VMMLEVEL        0x05F
#define VMQLEVEL        0x060
    /* Bits defined in VMQLEVEL */
#define VMQ1    0x80
#define VMCOMP  0x40
#define VMHIPRI 0x20
#define VMLOPRI 0x10
#define VMAEX   0x08
#define VMAEXP  0x04
#define VMAQ3   0x02
#define VMDROP1 0x02
#define VMFS    0x01

#define VM_RESERVED1    0x061
#define VMTLEVEL        0x062
   /* Flags defined for VMTLEVEL */
#define VMTON   0x80
#define VMRON   0x40
#define VMCPUTMR 0x20
#define VMSTMPI 0x08
#define VMSTMPT 0x04
#define VMTMRINT 0x01

#define VMPEND          0x063
   /* Flags defined in VMPEND */
#define VMDEFSTK 0x80
#define VMPERPND 0x40
#define VMPRGPND 0x20
#define VMSVCPND 0x10
#define VMPGPND  0x08
#define VMIOPND  0x02
#define VMEXTPND 0x01

#define VMLOCKER        0x064
#define VMFSTAT         0x068
#define VMMLVL2         0x069
#define VMIOINT         0x06A
#define VMTIMER         0x06C
#define VMVTIME         0x070
#define VMTMOUTQ        0x078
#define VMTTIME         0x080
#define VMTMINQ         0x088
#define VMTSOUTQ VMTMINQ
#define VMTODINQ        0x090
#define VMINST          0x098
#define VMUPRIOR        0x09E
#define VMPSWCT         0x09F
#define VMTREXT         0x0A0
#define VMADSTOP        0x0A4
#define VMPSW           0x0A8
#define VMGPRS          0x0B0
#define VMFPRS          0x0F0
#define VMUSER          0x110
#define VMACCNT         0x118
#define VMDIST          0x120
#define VMPRGIL         0x156

#define VMMICRO         0x17C
#define VMMCR6 VMMICRO
#define VMMADDR VMMICRO+1

#define VMPXINT         0x184

#define VMNOECPS        0x1A7

#define VMSTKCNT        0x1CC

/* Virtual Device Blocks Key Fields */
#define VCHSTAT  0x06
   /* Flags defined in VCHSTAT */
#define VCHBUSY  0x80
#define VCHCEPND 0x40

#define VCHTYPE  0x07
   /* Flags defined in VCHTYPE */
#define VCHSEL   0x80

#define VCUINTS  0x04
   /* Flags defined in VCUINTS */
#define CUE      0x20

#define VCUSTAT  0x06
   /* Flags defined in VCUSTAT */
#define VCUCHBSY 0x80
#define VCUCEPND 0x40
#define VCUBUSY  0x20
#define VCUCUEPN 0x08

#define VCUTYPE  0x07
   /* Flags defined in VCUTYPE */
#define VCUSHRD  0x80
#define VCUCTCA  0x40

#define VDEVINTS  0x02
#define VDEVTYPC  0x04
/* Flags defined in VDEVTYPC */
#define CLASDASD  0x04
#define CLASGRAF  0x40

#define VDEVTYPE  0x05
/* Flags defined in VDEVTYPE */
#define TYP3277   0x04
#define TYP3278   0x01

#define VDEVSTAT  0x06
   /* Flags defined in VDEVSTAT */
#define VDEVCHBS  0x80
#define VDEVCHAN  0x40
#define VDEVBUSY  0x20
#define VDEVDED   0x01

#define VDEVFLAG  0x07
   /* Flags defined in VDEVFLAG */
#define VDEVUC    0x01

#define VDEVRELN  0x10
#define VDEVFLG2  0x38
   /* Flags defined in VDEVFLG2 */
#define VDEVRRF   0x80



/* ECBLOK Specifics */
#define EXTSHCR0 0x40
#define EXTSHCR1 0x44
#define EXTSHLEN 0x48

/* ARCHTECT field displacements  (see ARCHTECT in DMKVAT)  */
#define PINVBIT 0x08
#define PAGTLEN 0x12

/* Field displacements to support CP trace entries by relevant CP assists */
#define TRACSTRT 0x0C
#define TRACEND  0x10
#define TRACCURR 0x14
#define TRACFLG1 0x400
#define TRAC02   0x40
#define TRAC67   0x04
#define TRACFLG2 0x401
#define TRAC0A   0x80
#define TRCSVC   0x82
#define TRCFREE  0x86
#define TRCFRET  0x87
#define TRCRUN   0x8A
#define FREESAVE 0x280
#define FREER0   0x280
#define FREER1   0x284
#define FREER14  0x2b8



typedef struct _ECPSVM_STAT
{
    char *name;
    U64   call;
    U64   hit;
    u_int support:1;
    u_int enabled:1;
    u_int debug:1;
    u_int total:1;
} ECPSVM_STAT;

/* THE FOLLOWING ARE C99 INITIALISATION OF THE ECPSVM INSTRUCTION STATE STRUCTURES */
/* SINCE MSVC SEEMS TO NOT LIKE THOSE, THEY ARE REPLACED FOR THE TIME BEING        */
#if 0
#define ECPSVM_STAT_DCL(_name) ECPSVM_STAT _name
#define ECPSVM_STAT_DEF(_name) ._name = { .name = ""#_name"" ,.call=0,.hit=0,.support=1,.total=0,.enabled=1,.debug=0}
#define ECPSVM_STAT_DEFU(_name) ._name = { .name = ""#_name"" ,.call=0,.hit=0,.support=0,.total=0,.enabled=1,.debug=0}
#define ECPSVM_STAT_DEFM(_name) ._name = { .name = ""#_name"" ,.call=0,.hit=0,.support=1,.total=1,.enabled=1,.debug=0}
#endif

/* BELOW ARE NON C99 STRUCTURE INITIALIZERS KEEP THE ABOVE IN SYNC PLEASE */
#define ECPSVM_STAT_DCL(_name) ECPSVM_STAT _name
#define ECPSVM_STAT_DEF(_name) { ""#_name"" ,0,0,1,1,0,0}
#define ECPSVM_STAT_DEFU(_name) {""#_name"" ,0,0,0,1,0,0}
#define ECPSVM_STAT_DEFM(_name) {""#_name"" ,0,0,1,1,0,1}

typedef struct _ECPSVM_CMDENT
{
    char *name;
    int  abbrev;
    void (*fun)(int argc,char **av);
    char *expl;
    char *help;
} ECPSVM_CMDENT;

#endif
