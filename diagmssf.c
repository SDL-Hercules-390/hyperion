/* DIAGMSSF.C   (c) Copyright Jan Jaeger, 1999-2012                  */
/*              ESA/390 Diagnose Functions                           */

/*-------------------------------------------------------------------*/
/* This module implements various diagnose functions                 */
/* MSSF call as described in SA22-7098-0                             */
/* SCPEND as described in GC19-6215-0 which is also used with PR/SM  */
/* LPAR RMF interface call                                           */
/*                                                                   */
/*                                             04/12/1999 Jan Jaeger */
/* z/Architecture support - (c) Copyright Jan Jaeger, 1999-2012      */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

#if !defined(_HENGINE_DLL_)
#define _HENGINE_DLL_
#endif

#if !defined(_DIAGMSSF_C_)
#define _DIAGMSSF_C_
#endif

#include "hercules.h"

#include "opcode.h"

#include "inline.h"

#include "service.h"

#if !defined(_DIAGMSSF_C)

#define _DIAGMSSF_C

/*-------------------------------------------------------------------*/
/* MSSF Call service processor commands                              */
/*-------------------------------------------------------------------*/
#define MSSF_READ_CONFIG_INFO   0x00020001
#define MSSF_READ_CHP_STATUS    0x00030001

/*-------------------------------------------------------------------*/
/* Service Processor Command Control Block                           */
/*-------------------------------------------------------------------*/
typedef struct _SPCCB_HEADER {
        HWORD   length;                 /* Total length of SPCCB     */
        BYTE    resv1[4];               /* Reserved                  */
        HWORD   resp;                   /* Response code             */
    } SPCCB_HEADER;

#define SPCCB_REAS_COMPLETE   0x00
#define SPCCB_RESP_COMPLETE   0x10
#define SPCCB_REAS_CHECK      0x00
#define SPCCB_RESP_CHECK      0x40
#define SPCCB_REAS_BADLENGTH  0x01
#define SPCCB_RESP_BADLENGTH  0xF0
#define SPCCB_REAS_NOT2KALIGN 0x01
#define SPCCB_RESP_NOT2KALIGN 0x00
#define SPCCB_REAS_UNASSIGNED 0x06
#define SPCCB_RESP_UNASSIGNED 0xF0

/*-------------------------------------------------------------------*/
/* Service Processor Command Control Block                           */
/*-------------------------------------------------------------------*/
typedef struct _SPCCB_CONFIG_INFO {
        BYTE    totstori;               /* Total number of installed
                                           storage increments.       */
        BYTE    storisiz;               /* Storage increment size in
                                           units of one megabyte.    */
        BYTE    hex04;                  /* This field contains the
                                           value of 04 hex.          */
        BYTE    hex01;                  /* This field contains the
                                           value of either 01 hex or
                                           02 hex.                   */
        FWORD   reserved;               /* Reserved.                 */
        HWORD   toticpu;                /* Total number of installed
                                           CPUs.  This number also
                                           specifies the total number
                                           of entries in the CPU-
                                           description list.         */
        HWORD   officpu;                /* Offset in the SPCCB to the
                                           first entry in the CPU-
                                           description list.         */
        HWORD   tothsa;                 /* Total number of machine-
                                           storage areas reserved for
                                           the requesting
                                           configuration.  This number
                                           also specifies the total
                                           number of entries in the
                                           machine-storage-area-
                                           description list.        */
        HWORD   offhsa;                 /* Offset in the SPCCB to the
                                           first entry in the
                                           machine-storage-area-
                                           description list.        */
        BYTE    loadparm[8];            /* Load parameter.  This field
                                           conains eight bytes of
                                           EBCDIC information specified
                                           by the operator for the
                                           manually initiated load
                                           function.  If the operator
                                           specifies fewer then eight
                                           characters,  they are left-
                                           justified and padded on the
                                           right with 40 hex (preferred),
                                           or F0 hex,  or 00 hex to
                                           form an eight byte load
                                           parameter.  If the operator
                                           does not specify a load
                                           parameter,  a default load
                                           parameter is formed,
                                           consisting of F1 hex (preferred),
                                           or 40 hex,  or 00 hex,  padded
                                           on the right with 40 hex
                                           (preferred), or F0 hex,  of
                                           00 hex.  The load parameter
                                           is set to this default value
                                           by the activation of the
                                           system-reset-clear key or by
                                           performing certain operator
                                           functions.                */
    } SPCCB_CONFIG_INFO;

typedef struct _SPCCB_CPU_INFO {
        BYTE    cpuaddr;                /* Rightmost eight bits of the
                                           CPU address               */
        BYTE    todid;                  /* Identity of the TOD clock
                                           accessed by this CPU      */
    } SPCCB_CPU_INFO;

typedef struct _SPCCB_HSA_INFO {
        BYTE    hsasize;                /* Bits 1-15 of this field
                                           specify the size of the
                                           area in units of 4K bytes
                                           or 32K bytes.  When bit 0
                                           of byte 0 is zero,  the
                                           size is in units of 32K
                                           bytes;  otherwise,  the
                                           size is in units of 4K
                                           bytes.                    */
        BYTE    hsaaddr;                /* Absolute address of the
                                           start of the machine-
                                           storage area.             */
    } SPCCB_HSA_INFO;

typedef struct _SPCCB_CHP_STATUS {
        BYTE    installed[32];          /* Installed-channel-path bit
                                           map.                      */
        BYTE    assigned[32];           /* Assigned-channel-path bit
                                           map.                      */
        BYTE    configured[32];         /* Configured-channel-path bit
                                           map.                      */
        BYTE    reserved[152];          /* Reserved.                 */
    } SPCCB_CHP_STATUS;


/* Diagnose 204 Field Descriptions from http://lwn.net/Articles/180938/
 */

typedef struct _DIAG204_HDR {
        BYTE    numpart;                /* Number of partitions      */
        BYTE    flags;                  /* Flag Byte                 */
#define DIAG204_PHYSICAL_PRESENT        0x80
        HWORD   resv;                   /* Unknown , 0 on 2003,
                                           0x0005 under VM           */
        HWORD   physcpu;                /* Number of phys CP's       */
        HWORD   offown;                 /* Offset to own partition   */
        DBLWRD  diagstck;               /* TOD of last diag204       */
    } DIAG204_HDR;

typedef struct _DIAG204_PART {
        BYTE    partnum;                /* Logical partition number
                                           starts with 1             */
        BYTE    virtcpu;                /* Number of virt CP's       */
        HWORD   resv1[3];
        BYTE    partname[8];            /* Partition name            */
    } DIAG204_PART;

typedef struct _DIAG204_PART_CPU {
        HWORD   cpaddr;                 /* CP address                */
        BYTE    resv1[2];
        BYTE    index;                  /* Index into diag224 area   */
        BYTE    cflag;                  /*   ???                     */
        HWORD   weight;                 /* Weight                    */
        DBLWRD  totdispatch;            /* Total dispatch time in    */
                                        /* ... microseconds          */
                                        /* ... (Accumulated CPU      */
                                        /* ... time)                 */
        DBLWRD  effdispatch;            /* Effective dispatch time   */
                                        /* ... in microseconds       */
                                        /* ... (Accumulated          */
                                        /* ... effective CPU time;   */
                                        /* ... that is, time not     */
                                        /* ... spent in overhead     */
                                        /* ... activities)           */
    } DIAG204_PART_CPU;

typedef struct _DIAG204_X_HDR {
        BYTE    numpart;                /* Number of partitions      */
        BYTE    flags;                  /* Flag Byte                 */
#define DIAG204_X_PHYSICAL_PRESENT      0x80
        HWORD   resv1;                  /* Unknown , 0 on 2003,
                                           0x0005 under VM           */
        HWORD   physcpu;                /* Number of phys CP's       */
        HWORD   offown;                 /* Offset to own partition   */
        DBLWRD  diagstck1;              /* TOD of last diag204       */
        DBLWRD  diagstck2;              /*   in STCKE format         */
        BYTE    resv2[40];
    } DIAG204_X_HDR;

typedef struct _DIAG204_X_PART {
        BYTE    partnum;                /* Logical partition number
                                           starts with 1             */
        BYTE    virtcpu;                /* Number of virt CP's       */
        BYTE    realcpu;                /* Number of real CP's       */
        BYTE    pflag;
        FWORD   mlu;
        BYTE    partname[8];            /* Partition name            */
        BYTE    cpcname[8];             /* CPC name                  */
        BYTE    osname[8];              /* Operating system type     */
        DBLWRD  cssize;                 /* Central storage size      */
        DBLWRD  essize;                 /* Expanded Storage size     */
        BYTE    upid;
        BYTE    resv1[3];
        FWORD   gr_mlu;
        BYTE    gr_name[8];             /* Sysplex name?             */
        BYTE    resv2[32];
    } DIAG204_X_PART;

typedef struct _DIAG204_X_PART_CPU {
        HWORD   cpaddr;                 /* CP address                */
        BYTE    resv1[2];
        BYTE    index;                  /* Index into diag224 area   */
        BYTE    cflag;                  /*   ???                     */
        HWORD   weight;                 /* Weight                    */
        DBLWRD  totdispatch;            /* Total dispatch time in    */
                                        /* ... microseconds          */
                                        /* ... (Accumulated CPU      */
                                        /* ... time)                 */
        DBLWRD  effdispatch;            /* Effective dispatch time   */
                                        /* ... in microseconds       */
                                        /* ... (Accumulated          */
                                        /* ... effective CPU time;   */
                                        /* ... that is, time not     */
                                        /* ... spent in overhead     */
                                        /* ... activities)           */
        HWORD   minweight;
        HWORD   curweight;
        HWORD   maxweight;
        BYTE    resv2[2];
        DBLWRD  onlinetime;             /* Accumulated microseconds  */
                                        /* ... logical CPU defined   */
        DBLWRD  waittime;               /* Accumulated microseconds  */
                                        /* ... logical CPU not       */
                                        /* ... executing (wait)      */
        FWORD   pmaweight;
        FWORD   polarweight;
        BYTE    resv3[40];
    } DIAG204_X_PART_CPU;

#endif /*!defined(_DIAGMSSF_C)*/


/*-------------------------------------------------------------------*/
/* Process SCPEND call (Function code 0x044)                         */
/*-------------------------------------------------------------------*/
void ARCH_DEP(scpend_call) (void)
{
        sched_yield();                  /* Just go to the dispatcher
                                           for a minimum delay       */
} /* end function scpend_call */

#ifdef FEATURE_MSSF_CALL
/*-------------------------------------------------------------------*/
/* Process MSSF call (Function code 0x080)                           */
/*-------------------------------------------------------------------*/
int ARCH_DEP(mssf_call) (int r1, int r2, REGS *regs)
{
U32     spccb_absolute_addr;            /* Absolute addr of SPCCB    */
U32     mssf_command;                   /* MSSF command word         */
U32               spccblen;            /* Length of SPCCB            */
SPCCB_HEADER      *spccb;              /* -> SPCCB header            */
SPCCB_CONFIG_INFO *spccbconfig;        /* -> SPCCB CONFIG info       */
SPCCB_CPU_INFO    *spccbcpu;           /* -> SPCCB CPU information   */
SPCCB_CHP_STATUS  *spccbchp;           /* -> SPCCB channel path info */
U16               offset;              /* Offset from start of SPCCB */
int               i;                   /* loop counter               */
DEVBLK            *dev;                /* Device block pointer       */

    /* R1 contains the real address of the SPCCB */
    spccb_absolute_addr = APPLY_PREFIXING (regs->GR_L(r1), regs->PX);

    /* R2 contains the service-processor-command word */
    mssf_command = regs->GR_L(r2);

    /* Program check if SPCCB is not on a doubleword boundary */
    if ( spccb_absolute_addr & 0x00000007 )
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);

    /* Program check if SPCCB is outside main storage */
    if ( spccb_absolute_addr > regs->mainlim )
        ARCH_DEP(program_interrupt) (regs, PGM_ADDRESSING_EXCEPTION);

//  /*debug*/logmsg("MSSF call %8.8X SPCCB=%8.8X\n",
//  /*debug*/       mssf_command, spccb_absolute_addr);

    /* Point to Service Processor Command Control Block */
    spccb = (SPCCB_HEADER*)(regs->mainstor + spccb_absolute_addr);

    /* Load SPCCB length from header */
    FETCH_HW(spccblen,spccb->length);

    /* Mark page referenced */
    STORAGE_KEY(spccb_absolute_addr, regs) |= STORKEY_REF;

    /* Program check if end of SPCCB falls outside main storage */
    if ( sysblk.mainsize - spccblen < spccb_absolute_addr )
        ARCH_DEP(program_interrupt) (regs, PGM_ADDRESSING_EXCEPTION);

    /* Obtain the interrupt lock */
    OBTAIN_INTLOCK(regs);

    /* If a service signal is pending then we cannot process the request */
    if( IS_IC_SERVSIG && (sysblk.servparm & SERVSIG_ADDR)) {
        RELEASE_INTLOCK(regs);
        return 2;   /* Service Processor Busy */
    }

    if( spccb_absolute_addr & 0x7ffff800 ) {
        spccb->resp[0] = SPCCB_REAS_NOT2KALIGN;
        spccb->resp[1] = SPCCB_RESP_NOT2KALIGN;
    } else
        /* Test MSSF command word */
        switch (mssf_command) {

        case MSSF_READ_CONFIG_INFO:

            /* Set response code X'01F0' if SPCCB length
               is insufficient to contain CONFIG info */
            if ( spccblen < 64 )
            {
                spccb->resp[0] = SPCCB_REAS_BADLENGTH;
                spccb->resp[1] = SPCCB_RESP_BADLENGTH;
            break;
            }

            /* Point to SPCCB data area following SPCCB header */
            spccbconfig = (SPCCB_CONFIG_INFO*)(spccb+1);
            memset (spccbconfig, 0, sizeof(SPCCB_CONFIG_INFO));

            /* Set main storage size in SPCCB */
            spccbconfig->totstori = sysblk.mainsize >> 20;
            spccbconfig->storisiz = 1;
            spccbconfig->hex04 = 0x04;
            spccbconfig->hex01 = 0x01;

            /* Set CPU array count and offset in SPCCB */
            STORE_HW(spccbconfig->toticpu,sysblk.maxcpu);
            offset = sizeof(SPCCB_HEADER) + sizeof(SPCCB_CONFIG_INFO);
            STORE_HW(spccbconfig->officpu,offset);

            /* Set HSA array count and offset in SPCCB */
            STORE_HW(spccbconfig->tothsa,0);
            offset += (U16)(sizeof(SPCCB_CPU_INFO) * sysblk.maxcpu);
            STORE_HW(spccbconfig->offhsa,offset);

            /* Move IPL load parameter to SPCCB */
            get_loadparm (spccbconfig->loadparm);

            /* Build the CPU information array after the SCP info */
            spccbcpu = (SPCCB_CPU_INFO*)(spccbconfig+1);
            for (i = 0; i < sysblk.maxcpu; i++, spccbcpu++)
            {
                memset( spccbcpu, 0, sizeof(SPCCB_CPU_INFO) );
                spccbcpu->cpuaddr = i;
                spccbcpu->todid = 0;
            }

            /* Set response code X'0010' in SPCCB header */
            spccb->resp[0] = SPCCB_REAS_COMPLETE;
            spccb->resp[1] = SPCCB_RESP_COMPLETE;

            break;

        case MSSF_READ_CHP_STATUS:

            /* Set response code X'0300' if SPCCB length
               is insufficient to contain channel path info */
            if ( spccblen < sizeof(SPCCB_HEADER) + sizeof(SPCCB_CHP_STATUS))
            {
                spccb->resp[0] = SPCCB_REAS_BADLENGTH;
                spccb->resp[1] = SPCCB_RESP_BADLENGTH;
                break;
            }

            /* Point to SPCCB data area following SPCCB header */
            spccbchp = (SPCCB_CHP_STATUS*)(spccb+1);
            memset( spccbchp, 0, sizeof(SPCCB_CHP_STATUS) );

            /* Identify CHPIDs used */
            for (dev = sysblk.firstdev; dev != NULL; dev = dev->nextdev)
            {
                for (i = 0; i < 8; i++)
                {
                    if( ((0x80 >> i) & dev->pmcw.pim) )
                    {
                    BYTE chpid;
                        chpid = dev->pmcw.chpid[i];
                        spccbchp->installed[chpid / 8] |= 0x80 >> (chpid % 8);
                        spccbchp->assigned[chpid / 8] |= 0x80 >> (chpid % 8);
                        spccbchp->configured[chpid / 8] |= 0x80 >> (chpid % 8);
                    }
               }
            }

            /* Set response code X'0010' in SPCCB header */
            spccb->resp[0] = SPCCB_REAS_COMPLETE;
            spccb->resp[1] = SPCCB_RESP_COMPLETE;

            break;

        default:
            PTT_ERR("*DIAG080",regs->GR_L(r1),regs->GR_L(r2),regs->psw.IA_L);
            /* Set response code X'06F0' for invalid MSSF command */
            spccb->resp[0] = SPCCB_REAS_UNASSIGNED;
            spccb->resp[1] = SPCCB_RESP_UNASSIGNED;

            break;

        } /* end switch(mssf_command) */

    /* Mark page changed */
    STORAGE_KEY(spccb_absolute_addr, regs) |= STORKEY_CHANGE;

    /* Set service signal external interrupt pending */
    sysblk.servparm &= ~SERVSIG_ADDR;
    sysblk.servparm |= spccb_absolute_addr;
    ON_IC_SERVSIG;

    /* Release the interrupt lock */
    RELEASE_INTLOCK(regs);

    /* Return condition code 0: Command initiated */
    return 0;

} /* end function mssf_call */
#endif /* FEATURE_MSSF_CALL */

/*-------------------------------------------------------------------*/
/* Process LPAR DIAG 204 call                                        */
/*-------------------------------------------------------------------*/
void ARCH_DEP(diag204_call) (int r1, int r2, REGS *regs)
{
DIAG204_HDR       *hdrinfo;            /* Header                     */
DIAG204_PART      *partinfo;           /* Partition info             */
DIAG204_PART_CPU  *cpuinfo;            /* CPU info                   */
#if defined(FEATURE_EXTENDED_DIAG204)
DIAG204_X_HDR      *hdrxinfo;          /* Header                     */
DIAG204_X_PART     *partxinfo;         /* Partition info             */
DIAG204_X_PART_CPU *cpuxinfo;          /* CPU info                   */
#endif /*defined(FEATURE_EXTENDED_DIAG204)*/
RADR              abs;                 /* abs addr of data area      */
int               i;                   /* loop counter               */
struct timespec   cputime;             /* to obtain thread time data */
ETOD              ETOD;                /* Extended TOD clock         */
U64               uCPU[MAX_CPU_ENGINES];    /* User CPU time    (us) */
U64               tCPU[MAX_CPU_ENGINES];    /* Total CPU time   (us) */

#if defined(FEATURE_PHYSICAL_DIAG204)
static BYTE       physical[8] =
              {0xD7,0xC8,0xE8,0xE2,0xC9,0xC3,0xC1,0xD3}; /* PHYSICAL */
#endif /*defined(FEATURE_PHYSICAL_DIAG204)*/

#if defined(FEATURE_EXTENDED_DIAG204)
U64               oCPU[MAX_CPU_ENGINES];    /* Online CPU time  (us) */
U64               wCPU[MAX_CPU_ENGINES];    /* Wait CPU time    (us) */
#endif /*defined(FEATURE_EXTENDED_DIAG204)*/

    /* Test DIAG204 command word */
    switch (regs->GR_L(r2)) {

    case 0x04:

        abs = APPLY_PREFIXING (GR_A(r1,regs), regs->PX);

        /* Program check if RMF data is not on a page boundary */
        if ( (abs & PAGEFRAME_BYTEMASK) != 0x000)
            ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);

        /* Program check if RMF data area is outside main storage */
        if ( abs > regs->mainlim )
            ARCH_DEP(program_interrupt) (regs, PGM_ADDRESSING_EXCEPTION);

        /* Point to DIAG 204 data area */
        hdrinfo = (DIAG204_HDR*)(regs->mainstor + abs);

        /* Mark page referenced */
        STORAGE_KEY(abs, regs) |= STORKEY_REF | STORKEY_CHANGE;

        /* Retrieve the TOD clock value */
        etod_clock(regs, &ETOD, ETOD_extended);

        /* Get processor time(s) and leave out non-CPU processes and
         * threads
         */
        for(i = 0; i < sysblk.maxcpu; ++i)
        {
            if (IS_CPU_ONLINE(i))
            {
                /* Get CPU times in microseconds */
                if( clock_gettime(sysblk.cpuclockid[i], &cputime) == 0 )
                {
                    uCPU[i] = timespec2us(&cputime);
                    tCPU[i] = uCPU[i] + etod2us(sysblk.regs[i]->waittime_accumulated
                                              + sysblk.regs[i]->waittime ) ;
                }
            }
        }

        memset(hdrinfo, 0, sizeof(DIAG204_HDR));
        hdrinfo->numpart = 1;
#if defined(FEATURE_PHYSICAL_DIAG204)
        hdrinfo->flags = DIAG204_PHYSICAL_PRESENT;
#endif /*defined(FEATURE_PHYSICAL_DIAG204)*/
        STORE_HW(hdrinfo->physcpu,sysblk.cpus);
        STORE_HW(hdrinfo->offown,sizeof(DIAG204_HDR));
        STORE_DW(hdrinfo->diagstck,ETOD2tod(ETOD));

        /* hercules partition */
        partinfo = (DIAG204_PART*)(hdrinfo + 1);
        memset(partinfo, 0, sizeof(DIAG204_PART));
        partinfo->partnum = sysblk.lparnum;     /* Hercules partition */
        partinfo->virtcpu = sysblk.cpus;
        get_lparname(partinfo->partname);

        /* hercules cpu's */
        cpuinfo = (DIAG204_PART_CPU*)(partinfo + 1);
        for(i = 0; i < sysblk.maxcpu; i++)
          if (IS_CPU_ONLINE(i))
          {
              memset(cpuinfo, 0, sizeof(DIAG204_PART_CPU));
              STORE_HW(cpuinfo->cpaddr,sysblk.regs[i]->cpuad);
              cpuinfo->index=sysblk.ptyp[i];
              STORE_HW(cpuinfo->weight,100);
              STORE_DW(cpuinfo->totdispatch,tCPU[i]);
              STORE_DW(cpuinfo->effdispatch, uCPU[i]);
              cpuinfo += 1;
          }

#if defined(FEATURE_PHYSICAL_DIAG204)
        /* LPAR management */
        /* FIXME: This section should report on the real CPUs, appearing
         *        and transformed for reporting purposes. This should
         *        also be properly reflected in STSI information.
         */
        partinfo = (DIAG204_PART*)cpuinfo;
        memset(partinfo, 0, sizeof(DIAG204_PART));
        partinfo->partnum = 0; /* Physical machine */
        partinfo->virtcpu = sysblk.cpus;
        memcpy(partinfo->partname,physical,sizeof(physical));

        /* report all emulated physical cpu's */
        cpuinfo = (DIAG204_PART_CPU*)(partinfo + 1);
        for(i = 0; i < sysblk.maxcpu; i++)
          if (IS_CPU_ONLINE(i))
          {
              memset(cpuinfo, 0, sizeof(DIAG204_PART_CPU));
              STORE_HW(cpuinfo->cpaddr,sysblk.regs[i]->cpuad);
              cpuinfo->index = sysblk.ptyp[i];
              STORE_HW(cpuinfo->weight,100);
              STORE_DW(cpuinfo->totdispatch, tCPU[i]);
              STORE_DW(cpuinfo->effdispatch, uCPU[i]);
              cpuinfo += 1;
          }
#endif /*defined(FEATURE_PHYSICAL_DIAG204)*/

        regs->GR_L(r2) = 0;

        break;

#if defined(FEATURE_EXTENDED_DIAG204)
    /* Extended subcode 5 returns the size of the data areas provided by extended subcodes 6 and 7 */
    case 0x00010005:
        i = sizeof(DIAG204_X_HDR) + ((sizeof(DIAG204_X_PART) + (sysblk.maxcpu * sizeof(DIAG204_X_PART_CPU))) * 2);
        regs->GR_L(r2+1) = (i + PAGEFRAME_BYTEMASK) / PAGEFRAME_PAGESIZE;
        regs->GR_L(r2) = 0;

        break;

    /* Provide extended information */
    case 0x00010006:
        /* We fall through as we do not have any secondary cpus (that we know of) */

    /* Provide extended information, including information about secondary CPUs */
    case 0x00010007:
        /* Program check if RMF data is not on a page boundary */
        if ( (regs->GR_L(r1) & PAGEFRAME_BYTEMASK) != 0x000)
            ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);

        /* Obtain absolute address of main storage block,
           check protection, and set reference and change bits */
        hdrxinfo = (DIAG204_X_HDR*)MADDR (GR_A(r1,regs), r1, regs, ACCTYPE_WRITE, regs->psw.pkey);

        /* Retrieve the TOD clock value */
        etod_clock(regs, &ETOD, ETOD_extended);

        /* Get processor time(s) and leave out non-CPU processes and
         * threads
         */
        for(i = 0; i < sysblk.maxcpu; ++i)
        {
            if (IS_CPU_ONLINE(i))
            {
                oCPU[i] = etod2us(ETOD.high - regs->tod_epoch - sysblk.cpucreateTOD[i]);
                /* Get CPU times in microseconds */
                if( clock_gettime(sysblk.cpuclockid[i], &cputime) == 0 )
                {
                    uCPU[i] = timespec2us(&cputime);
                    tCPU[i] = uCPU[i] + etod2us(sysblk.regs[i]->waittime_accumulated
                                              + sysblk.regs[i]->waittime ) ;
                }
                wCPU[i] = tCPU[i] - uCPU[i];
            }
        }

        memset(hdrxinfo, 0, sizeof(DIAG204_X_HDR));
        hdrxinfo->numpart = 1;
#if defined(FEATURE_PHYSICAL_DIAG204)
        hdrxinfo->flags = DIAG204_X_PHYSICAL_PRESENT;
#endif /*defined(FEATURE_PHYSICAL_DIAG204)*/
        STORE_HW(hdrxinfo->physcpu,sysblk.cpus);
        STORE_HW(hdrxinfo->offown,sizeof(DIAG204_X_HDR));
        STORE_DW(hdrxinfo->diagstck1,ETOD.high);
        STORE_DW(hdrxinfo->diagstck2,ETOD.low);

        /* hercules partition */
        partxinfo = (DIAG204_X_PART*)(hdrxinfo + 1);
        memset(partxinfo, 0, sizeof(DIAG204_PART));
        partxinfo->partnum = sysblk.lparnum;    /* Hercules partition */
        partxinfo->virtcpu = sysblk.cpus;
        partxinfo->realcpu = hostinfo.num_procs;
        get_lparname(partxinfo->partname);
        get_sysname(partxinfo->cpcname);
        get_systype(partxinfo->osname);
        STORE_DW( partxinfo->cssize, sysblk.mainsize >> SHIFT_MEGABYTE );
        STORE_DW( partxinfo->essize, sysblk.xpndsize >> SHIFT_MEGABYTE );
        get_sysplex(partxinfo->gr_name);

        /* hercules cpu's */
        cpuxinfo = (DIAG204_X_PART_CPU*)(partxinfo + 1);
        for(i = 0; i < sysblk.maxcpu; i++)
          if (IS_CPU_ONLINE(i))
          {
              memset(cpuxinfo, 0, sizeof(DIAG204_X_PART_CPU));
              STORE_HW(cpuxinfo->cpaddr,sysblk.regs[i]->cpuad);
              cpuxinfo->index = sysblk.ptyp[i];
              STORE_HW(cpuxinfo->weight,100);

              STORE_DW(cpuxinfo->totdispatch, tCPU[i]);
              STORE_DW(cpuxinfo->effdispatch, uCPU[i]);

              STORE_HW(cpuxinfo->minweight,1000);
              STORE_HW(cpuxinfo->curweight,1000);
              STORE_HW(cpuxinfo->maxweight,1000);

              STORE_DW(cpuxinfo->onlinetime, oCPU[i]);
              STORE_DW(cpuxinfo->waittime,   wCPU[i]);

              STORE_HW(cpuxinfo->pmaweight,1000);
              STORE_HW(cpuxinfo->polarweight,1000);

              cpuxinfo += 1;
          }


#if defined(FEATURE_PHYSICAL_DIAG204)
        /* LPAR management */
        /* FIXME: This section should report on the real CPUs, appearing
         *        and transformed for reporting purposes. This should
         *        also be properly reflected in STSI information.
         */
        partxinfo = (DIAG204_X_PART*)cpuxinfo;
        memset(partxinfo, 0, sizeof(DIAG204_X_PART));
        partxinfo->partnum = 0; /* Physical machine */
        partxinfo->virtcpu = sysblk.cpus;
        partxinfo->realcpu = hostinfo.num_procs;
        memcpy(partxinfo->partname,physical,sizeof(physical));

        /* report all emulated physical cpu's */
        cpuxinfo = (DIAG204_PART_CPU*)(partinfo + 1);
        for(i = 0; i < sysblk.maxcpu; i++)
          if (IS_CPU_ONLINE(i))
          {
              memset(cpuxinfo, 0, sizeof(DIAG204_X_PART_CPU));
              STORE_HW(cpuxinfo->cpaddr,sysblk.regs[i]->cpuad);
              cpuxinfo->index = sysblk.ptyp[i];
              STORE_HW(cpuxinfo->weight,100);

              STORE_DW(cpuxinfo->totdispatch, tCPU[i]);
              STORE_DW(cpuxinfo->effdispatch, uCPU[i]);

              STORE_HW(cpuxinfo->minweight,1000);
              STORE_HW(cpuxinfo->curweight,1000);
              STORE_HW(cpuxinfo->maxweight,1000);

              STORE_DW(cpuxinfo->onlinetime, oCPU[i]);
              STORE_DW(cpuxinfo->waittime,   wCPU[i]);

              STORE_HW(cpuxinfo->pmaweight,1000);
              STORE_HW(cpuxinfo->polarweight,1000);

              cpuxinfo += 1;
          }
#endif /*defined(FEATURE_PHYSICAL_DIAG204)*/

        regs->GR_L(r2) = 0;

        break;
#endif /*defined(FEATURE_EXTENDED_DIAG204)*/

    default:
        PTT_ERR("*DIAG204",regs->GR_L(r1),regs->GR_L(r2),regs->psw.IA_L);
        regs->GR_L(r2) = 4;

    } /*switch(regs->GR_L(r2))*/

} /* end function diag204_call */


/*-------------------------------------------------------------------*/
/* Process LPAR DIAG 224 call                                        */
/*-------------------------------------------------------------------*/
void ARCH_DEP(diag224_call) (int r1, int r2, REGS *regs)
{
RADR              abs;                 /* abs addr of data area      */
BYTE             *p;                   /* pointer to the data area   */
unsigned int      len, ptyp, i;        /* work                       */

    // FIXME:  This function is probably incomplete.
    //         See linux/arch/s390/hypfs/hypfs_diag.c

    UNREFERENCED( r1 );

    abs = APPLY_PREFIXING( regs->GR_L( r2 ), regs->PX );

    /* Program check if data area is not on a page boundary */
    if ((abs & PAGEFRAME_BYTEMASK) != 0)
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );

    /* Calculate length of data to be stored */
    len = 16 + ((MAX_SCCB_PTYP + 1) * 16);

    /* Program check if data area is outside main storage */
    if (abs > regs->mainlim || len > PAGEFRAME_PAGESIZE)
        ARCH_DEP( program_interrupt )( regs, PGM_ADDRESSING_EXCEPTION );

    /* Mark page referenced */
    STORAGE_KEY( abs, regs ) |= (STORKEY_REF | STORKEY_CHANGE);

    /* Point to DIAG 224 return area */
    p = regs->mainstor + abs;

    /*
    ** The first byte of the first 16-byte entry contains the total number
    ** of 16-byte entries minus 1 that immediately follows the first entry.
    ** The remaining 15 bytes of the first 16-byte entry are binary zeros.
    ** Each of the remaining entries following the first one contains the
    ** EBCDIC name of each processor type.
    */

    *p = MAX_SCCB_PTYP;         /* (number of entries which follows-1) */
    memset( p+1, 0, 16-1 );     /* (pad first entry with binary zeros) */

    for (ptyp=0; ptyp <= MAX_SCCB_PTYP; ptyp++)
    {
        p += 16;                              /* point to next entry   */
        memcpy( p, ptyp2long( ptyp ), 16 );   /* move in ASCII value   */
        for (i=0; i < 16; i++)
            p[i] = host_to_guest( p[i] );     /* convert it to EBCDIC  */
    }

} /* end function diag224_call */


#if !defined(_GEN_ARCH)

#if defined(_ARCHMODE2)
 #define  _GEN_ARCH _ARCHMODE2
 #include "diagmssf.c"
#endif

#if defined(_ARCHMODE3)
 #undef   _GEN_ARCH
 #define  _GEN_ARCH _ARCHMODE3
 #include "diagmssf.c"
#endif

#endif /*!defined(_GEN_ARCH)*/
