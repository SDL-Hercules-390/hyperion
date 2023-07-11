/* VM.C         (C) Copyright Roger Bowler, 2000-2012                */
/*              ESA/390 VM Diagnose calls and IUCV instruction       */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/* Interpretive Execution - (C) Copyright Jan Jaeger, 1999-2012      */

/*-------------------------------------------------------------------*/
/* This module implements miscellaneous diagnose functions           */
/* described in SC24-5670 VM/ESA CP Programming Services             */
/* and SC24-5855 VM/ESA CP Diagnosis Reference                       */
/* and SC24-6084 z/VM 5.4 CP Programming Services.                   */
/*      Modifications for Interpretive Execution (SIE) by Jan Jaeger */
/* z/Architecture support - (C) Copyright Jan Jaeger, 1999-2012      */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

#define _VM_C_
#define _HENGINE_DLL_

#include "hercules.h"
#include "opcode.h"
#include "inline.h"
#include "commadpt.h"

#if defined(FEATURE_EMULATE_VM)

#if !defined(_VM_C)
#define _VM_C

/*-------------------------------------------------------------------*/
/* Internal macro definitions                                        */
/*-------------------------------------------------------------------*/
#define DEV024(_type,_cls,_typ) \
       { _type,_cls,_typ,0xC0 }
#define DEV210(_type,_cls,_typ) \
       { _type,_cls,_typ,0x40 }

/*-------------------------------------------------------------------*/
/* Synchronous Block I/O Parameter List                              */
/*-------------------------------------------------------------------*/
typedef struct _HCPSBIOP {
        HWORD   devnum;                 /* Device number             */
        BYTE    akey;                   /* Bits 0-3=key, 4-7=zeroes  */
        BYTE    type;                   /* I/O request type          */
        FWORD   blksize;                /* Fixed block size          */
        FWORD   sbiaddr;                /* Address of SBILIST        */
        FWORD   sbicount;               /* Number of SBILIST entries */
        FWORD   blkcount;               /* Number of blocks processed*/
        BYTE    unitstat;               /* Device status             */
        BYTE    chanstat;               /* Subchannel status         */
        HWORD   residual;               /* Residual byte count       */
        BYTE    lpm;                    /* Logical path mask         */
        BYTE    resv1[5];               /* Reserved bytes, must be 0 */
        HWORD   sensecount;             /* Number of sense bytes     */
        BYTE    resv2[24];              /* Reserved bytes, must be 0 */
        BYTE    sense[32];              /* Sense bytes               */
    } HCPSBIOP;

/* Definitions for I/O request type */
#define HCPSBIOP_WRITE        0x01
#define HCPSBIOP_READ         0x02

/*-------------------------------------------------------------------*/
/* Synchronous General I/O Parameter List                            */
/*-------------------------------------------------------------------*/
typedef struct _HCPSGIOP {
        HWORD   devnum;                 /* Device number             */
        BYTE    akey;                   /* Bits 0-3=key, 4-7=zeroes  */
        BYTE    flag;                   /* Flags                     */
        FWORD   resv1;                  /* Reserved word, must be 0  */
        FWORD   ccwaddr;                /* Address of channel program*/
        FWORD   resv2;                  /* Reserved word, must be 0  */
        FWORD   lastccw;                /* CCW address at interrupt  */
        BYTE    unitstat;               /* Device status             */
        BYTE    chanstat;               /* Subchannel status         */
        HWORD   residual;               /* Residual byte count       */
        BYTE    lpm;                    /* Logical path mask         */
        BYTE    resv3[5];               /* Reserved bytes, must be 0 */
        HWORD   sensecount;             /* Number of sense bytes     */
        BYTE    resv4[24];              /* Reserved bytes, must be 0 */
        BYTE    sense[32];              /* Sense bytes               */
    } HCPSGIOP;

/* Bit definitions for flags */
#define HCPSGIOP_FORMAT1_CCW  0x80      /* 1=Format-1 CCW            */
#define HCPSGIOP_FLAG_RESV    0x7F      /* Reserved bits, must be 0  */

/*-------------------------------------------------------------------*/
/* DIAGNOSE X'24' and DIAGNOSE X'210' Structures and Table           */
/*-------------------------------------------------------------------*/

/* VM Device Class Definitions */
#define DC_TERM 0x80
#define DC_GRAF 0x40
#define DC_URI  0x20
#define DC_URO  0x10
#define DC_TAPE 0x08
#define DC_DASD 0x04
#define DC_SPEC 0x02
#define DC_FBA  0x01

/* VM Device Type Definitions */
#define DT_CTCA 0x80
#define DT_FBA  0x00
#define DT_OSA  0x20 /* Not yet supported */
#define DT_UNKN 0x01
#define DT_0671 0x20
#define DT_1052 0x00
#define DT_1403 0x41
#define DT_1442 0x88
#define DT_2305 0x02
#define DT_2311 0x80
#define DT_2314 0x40
#define DT_2501 0x81
#define DT_2703 0x40
#define DT_3211 0x42
#define DT_3215 0x00
#define DT_3277 0x04
#define DT_3287 0x02
#define DT_3310 0x01
#define DT_3330 0x10
#define DT_3340 0x01
#define DT_3350 0x08
#define DT_3370 0x02
#define DT_3375 0x04
#define DT_3380 0x20
#define DT_3390 0x82
#define DT_3410 0x08
#define DT_3420 0x10
#define DT_3422 0x82
#define DT_3430 0x02
#define DT_3480 0x01
#define DT_3490 0x81
#define DT_3505 0x84
#define DT_3525 0x84
#define DT_3590 0x83
#define DT_370x 0x40
#define DT_8809 0x04
#define DT_9332 0x08
#define DT_9335 0x04
#define DT_9336 0x40
#define DT_9345 0x81
#define DT_9347 0x84

/* VM Virtual Device Status Definitions */
#define DS_DED    0x01    /* Dedicated device          */
#define DS_BUSY   0x20    /* Device is busy            */

/* VM Virtual Device Flag Definitions */
#define DF_ENA     0x80   /* 270x line enabled         */
#define DF_CONN    0x40   /* 270x line connected       */
#define DF_RSRL    0x02   /* Reserve/Release supported */
#define DF_MIDAW   0x01   /* MIDAW's supported         */

/* VM Real Device Features */
#define DRF_RPS    0x80  /* Device has RPS             */
#define DRF_EXTSNS 0x40  /* Extended Sense             */
#define DRF_CTCA   0x40  /* CTCA device                */
#define DRF_35M    0x08  /* 3340 has 35M data module   */
#define DRF_70M    0x04  /* 3340 has 70M data module   */
#define DRF_RSRL   0x02  /* Reserve/Release valid      */

/*-------------------------------------------------------------------*/
/* Hercules-to-VM Device Table                                       */
/*-------------------------------------------------------------------*/
typedef struct _VMDEVTBL {
        U16     vmhtype;                /* Hercules device type      */
        BYTE    vmdevcls;               /* VM Device Class           */
        BYTE    vmdevtyp;               /* VM Device Type            */
        BYTE    vmdiags;                /* DIAGS recognizing device  */
#define VMDIAG024 0x80   /* Device recognized by DIAGNOSE X'24'      */
#define VMDIAG210 0x40   /* Device recognized by DIAGNOSE X'210'     */
    } VMDEVTBL;
#define VMDEV_SIZE sizeof(VMDEVTBL)

static VMDEVTBL vmdev[] = {
   DEV024(0x0671,DC_FBA, DT_0671),
   DEV024(0x1052,DC_TERM,DT_1052),
   DEV024(0x1403,DC_URO, DT_1403),
   DEV024(0x1442,DC_URI, DT_1442),
   DEV024(0x2305,DC_DASD,DT_2305),
   DEV024(0x2311,DC_DASD,DT_2311),
   DEV024(0x2314,DC_DASD,DT_2314),
   DEV024(0x2501,DC_URI, DT_2501),
   DEV024(0x2703,DC_TERM,DT_2703),
   DEV024(0x3088,DC_SPEC,DT_CTCA),
   DEV024(0x3211,DC_URI, DT_3211),
   DEV024(0x3215,DC_TERM,DT_3215),
   DEV024(0x3270,DC_GRAF,DT_3277),
   DEV024(0x3287,DC_GRAF,DT_3287),
   DEV024(0x3310,DC_FBA, DT_3310),
   DEV024(0x3330,DC_DASD,DT_3330),
   DEV024(0x3340,DC_DASD,DT_3340),
   DEV024(0x3350,DC_DASD,DT_3350),
   DEV024(0x3370,DC_FBA, DT_3370),
   DEV024(0x3375,DC_DASD,DT_3375),
   DEV024(0x3380,DC_DASD,DT_3380),
   DEV210(0x3390,DC_DASD,DT_3390),
   DEV024(0x3410,DC_TAPE,DT_3410),
   DEV024(0x3420,DC_TAPE,DT_3420),
   DEV024(0x3422,DC_TAPE,DT_3422),
   DEV024(0x3430,DC_TAPE,DT_3430),
   DEV024(0x3480,DC_TAPE,DT_3480),
   DEV210(0x3490,DC_TAPE,DT_3490),
   DEV024(0x3505,DC_URI, DT_3505),
   DEV024(0x3525,DC_URO, DT_3525),
   DEV024(0x3590,DC_TAPE,DT_3590),
   DEV024(0x3705,DC_SPEC,DT_370x),
   DEV024(0x8809,DC_TAPE,DT_8809),
   DEV024(0x9332,DC_FBA, DT_9332),
   DEV024(0x9335,DC_FBA, DT_9335),
   DEV024(0x9336,DC_FBA, DT_9336),
   DEV210(0x9345,DC_DASD,DT_9345),
   DEV024(0x9347,DC_TAPE,DT_9347)
};
#define VMDEV_NUM (sizeof(vmdev)/VMDEV_SIZE)

/*-------------------------------------------------------------------*/
/* Virtual Device Data                                               */
/*-------------------------------------------------------------------*/
typedef struct _VRDCVDAT {
        BYTE    vdevcls;                /* Virtual device class      */
        BYTE    vdevtyp;                /* Virtual device type       */
        BYTE    vdevstat;               /* Virtual device status     */
        BYTE    vdevflag;               /* Virtual device flag       */
    } VRDCVDAT;

/*-------------------------------------------------------------------*/
/* Real Device Data                                                  */
/*-------------------------------------------------------------------*/
typedef struct _VRDCRCDT {
        BYTE    rdevcls;                /* Real device class         */
        BYTE    rdevtyp;                /* Real device type          */
        BYTE    rdevmodl;               /* Real device model         */
        BYTE    rdevfeat;               /* Real device features      */
    } VRDCRCDT;

/*-------------------------------------------------------------------*/
/* Virtual/Real Device Characteristics Block                         */
/*-------------------------------------------------------------------*/
typedef struct _VRDCBLOK {
/*00*/  HWORD    vrdcdvno;       /* Device number                    */
/*02*/  HWORD    vrdclen;        /* VRDCBLOK length                  */
/*04*/  VRDCVDAT vrdcvdat;       /* Virtual device data              */
/*08*/  VRDCRCDT vrdcrcdt;       /* Real device data                 */
/*0C*/  BYTE     vrdcundv;       /* Real underlying device           */
/*0D*/  BYTE     vrdcrdaf;       /* Real device additional features  */
#define VRDCEMRD 0x02            /* No emulated real device          */
/*0E*/  HWORD    vrdcrsvd;       /* Reserved - must be zeros         */
/*10*/  BYTE     vrdcrdc[64];    /* READ DEVICE CHARACTERISTICS data */
/*50*/  BYTE     vrdcpgid[11];   /* Path Group Identifier            */
/*5B*/  BYTE     resv5[5];       /* reserved                         */
/*60*/  BYTE     vrdcvers;       /* version                          */
/*61*/  BYTE     vrdcrsio[31];   /* reserved for Input/Output        */
/*80*/  HWORD    vrdcrdev;       /* Real device number               */
/*82*/  BYTE     vrdcrsve[126];  /* reserverd                        */
/*100*/
   } VRDCBLOK;
#define VRDCBLOK_SIZE sizeof(VRDCBLOK)

/*-------------------------------------------------------------------*/
/* Check if command is a "shell" type command   (boolean function)   */
/*-------------------------------------------------------------------*/
static BYTE is_shell_command( const char* cmd )
{
    char* cmd_work  = strdup( cmd );
    BYTE  is_shcmd  = FALSE;

    /* Remove leading/trailing blanks */
    TRIM( cmd_work );

    /* Check for possible "shell" type command */
    if (0
        || strcasecmp( cmd_work, "sh"   ) == 0  // (shell command)
        || strcasecmp( cmd_work, "exec" ) == 0  // (rexx command)
    )
        is_shcmd = TRUE;  /* Looks like a 'sh' shell command! */

    free( cmd_work );
    return is_shcmd;
}

#endif /*!defined(_VM_C)*/

/*-------------------------------------------------------------------*/
/* Internal Function Prototypes                                      */
/*-------------------------------------------------------------------*/
DEVBLK* ARCH_DEP(vmdevice_data)(int, U16, VRDCVDAT *, VRDCRCDT *);

/*-------------------------------------------------------------------*/
/* Provide VM Virtual and Real Device Data based upon device number  */
/*-------------------------------------------------------------------*/
DEVBLK* ARCH_DEP(vmdevice_data)(int code, U16 devnum, VRDCVDAT *vdat, VRDCRCDT *rdat)
{
U32      i;                      /* loop index                      */
VMDEVTBL *vmentry;               /* -> VMDEVTBL entry found         */
DEVBLK   *dev;                   /* -> DEVBLK                       */

    /* Clear vdat and rdat */
    memset (vdat, 0, sizeof(*vdat));
    memset (rdat, 0, sizeof(*rdat));

    /* Locate the device block */
    dev = find_device_by_devnum (0,devnum);

    /* Return 0 if device is not found */
    if (!dev)
       return 0;

    /* Indicate the device is dedicated - all Hercules devices are */
    vdat->vdevstat = DS_DED;

    /* Find the device in the VM table */
    vmentry=NULL;
    for (i = 0; i < (int)VMDEV_NUM; i++)
       {
#if 0 // debug
           LOGMSG( "vmdevice_data: i=%i %4.4X %2.2X %2.2X %2.2X\n",i,
                   vmdev[i].vmhtype, vmdev[i].vmdevcls, vmdev[i].vmdevtyp, vmdev[i].vmdiags );
#endif
           if (dev->devtype == vmdev[i].vmhtype)
           {
               vmentry = &vmdev[i];
               break;
           }
       }
#if 0 // debug
    LOGMSG( "FOUND: %4.4X %2.2X %2.2X %2.2X\n",
            vmentry->vmhtype, vmentry->vmdevcls, vmentry->vmdevtyp, vmentry->vmdiags );
#endif

    /* If device is not in the table or it isn't recognized by DIAG X'24' */
    if ( !vmentry || ( code==0x24 && !(vmentry->vmdiags & VMDIAG024 ) ) )
    {
        /* Set the real and virtual data to an unsupported device */
        vdat->vdevcls = DC_SPEC;
        vdat->vdevtyp = DT_UNKN;
        rdat->rdevcls = DC_SPEC;
        rdat->rdevtyp = DT_UNKN;
        return dev;
    }

    /* Set the virtual and real data to the device's VM class and type */
    vdat->vdevcls = vmentry->vmdevcls;
    vdat->vdevtyp = vmentry->vmdevtyp;
    rdat->rdevcls = vmentry->vmdevcls;
    rdat->rdevtyp = vmentry->vmdevtyp;

    /* Indicate if the device is busy */
    if (0
        || dev->startpending
        || (1
            && dev->busy
#if defined( OPTION_SHARED_DEVICES )
            && dev->shioactive == DEV_SYS_LOCAL
#endif // defined( OPTION_SHARED_DEVICES )
           )
    )
        vdat->vdevstat |= DS_BUSY;

    /* Set virtual device flags, and real device model and features */
    vdat->vdevflag = 0x00;
    rdat->rdevmodl = 0x00;
    rdat->rdevfeat = 0x00;

    if (dev->hnd->reserve)           /* Indicate if RESERVE/RELEASE supported */
        vdat->vdevflag |= DF_RSRL;

#if defined(FEATURE_MIDAW_FACILITY)
    /* If DIAGNOSE X'210', indicate if MIDAW's are supported */
    if (code==0x210)
        vdat->vdevflag |= DF_MIDAW;
#endif /* FEATURE_MIDAW_FACILITY */

    switch (rdat->rdevcls) {
    case DC_DASD:
         if (dev->hnd->reserve)
            rdat->rdevfeat |= DRF_RSRL;
         if (dev->numsense==24)
            rdat->rdevfeat |= DRF_EXTSNS;
         if (dev->ckdtab->sectors)
            rdat->rdevfeat |= DRF_RPS;
         if (dev->devtype == 0x3340)
         {
            if (dev->ckdtab->model==0x01)
               rdat->rdevfeat |= DRF_35M;
            else
               rdat->rdevfeat |= DRF_70M;
         }
         if ( dev->devtype == 0x3380  && code == 0x24)
            rdat->rdevmodl = (dev->ckdtab->model & 0x0F) | (dev->ckdcu->model & 0xF0);
         else
            rdat->rdevmodl = dev->ckdtab->model;
         break;
     case DC_FBA:
         rdat->rdevmodl = dev->fbatab->model;
         break;
     case DC_TERM:
         if (dev->devtype==0x3215)
         {
            rdat->rdevfeat = 0x50;
            /* Note: 0x50 is carried forward from the previous version of       */
            /* DIAGNOSE X'24'. The actual meaning was not previously documented */
         }
         else
         {
            if (dev->devtype==0x2703 && dev->commadpt)
            {
                if (dev->commadpt->enabled)
                   vdat->vdevflag |= DF_ENA;
                if (dev->commadpt->connect)
                   vdat->vdevflag |= DF_CONN;
            }
         }
         break;
     case DC_SPEC:
         if (rdat->rdevtyp==DT_CTCA)
            rdat->rdevfeat = DRF_CTCA;
     }

     /* Return the located DEVBLK to the caller */
     return dev;

} /* end function vmdevice_data */

/*-------------------------------------------------------------------*/
/* Device Type and Features (Function code 0x024)                    */
/*-------------------------------------------------------------------*/
int ARCH_DEP(diag_devtype) (int r1, int r2, REGS *regs)
{
DEVBLK         *dev;                    /* -> Device block           */
U16             devnum;                 /* Device number             */
VRDCVDAT        vdat;                   /* Virtual device data       */
VRDCRCDT        rdat;                   /* Real device data          */

#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
     /* Program check if 64-bit addressing is being used. */
     if (regs->psw.amode64)
     {
         ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
     }
#endif /* FEATURE_001_ZARCH_INSTALLED_FACILITY */

    /* Return console information if R1 register is all ones */
    if (regs->GR_L(r1) == 0xFFFFFFFF)
    {
        for (dev = sysblk.firstdev; dev != NULL; dev = dev->nextdev)
            if ( dev->allocated
                 && ( dev->devtype == 0x3215 || dev->devtype == 0x1503 )
               )
            {
                regs->GR_L(r1) = dev->devnum;
                break;
            }
    }

    /* Extract the device number from the R1 register */
    devnum = regs->GR_L(r1);

    /* Locate the device block and set the virtual and real device information */
    dev = ARCH_DEP(vmdevice_data) (0x24,devnum,&vdat,&rdat);

    /* Return condition code 3 if device does not exist */
    if (!dev)
        return 3;

    /* Return virtual device information in the R2 register */
    FETCH_FW(regs->GR_L(r2),&vdat);

    /* Return real device information in the R2+1 register */
    if (r2 != 15)
        FETCH_FW(regs->GR_L(r2+1),&rdat);
#if 0 // debug
    LOGMSG( "Diagnose X'024':devnum=%4.4X VRDCVDAT=%8.8X VRDCRCDT=%8.8X\n",
            devnum, vdat, rdat );
#endif

    /* Return condition code 0 */
    return 0;

} /* end function diag_devtype */

/*-------------------------------------------------------------------*/
/* Process Synchronous Fixed Block I/O call (Function code 0x0A4)    */
/*-------------------------------------------------------------------*/
int ARCH_DEP(syncblk_io) (int r1, int r2, REGS *regs)
{
U32             i;                      /* Array subscript           */
U32             numsense;               /* Number of sense bytes     */
U32             iopaddr;                /* Address of HCPSBIOP       */
HCPSBIOP        ioparm;                 /* I/O parameter list        */
DEVBLK         *dev;                    /* -> Device block           */
U16             devnum;                 /* Device number             */
U32             residual;               /* Residual byte count       */
U32             blksize;                /* Fixed block size          */
U32             sbiaddr;                /* Addr of SBILIST           */
U32             sbicount;               /* Number of SBILIST entries */
U32             blkcount;               /* Number of blocks processed*/
U32             blknum;                 /* Block number              */
U32             absadr;                 /* Absolute storage address  */
BYTE            accum;                  /* Work area                 */
BYTE            unitstat = 0;           /* Device status             */
BYTE            chanstat = 0;           /* Subchannel status         */
BYTE            skey1, skey2;           /* Storage keys of first and
                                           last byte of I/O buffer   */
//FIXME: code not right for shared devices

    UNREFERENCED(r2);

    /* Register R1 contains the real address of the parameter list */
    iopaddr = regs->GR_L(r1);

    /* Program check if parameter list not on fullword boundary */
    if (iopaddr & 0x00000003)
    {
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
    }

    /* Ensure that parameter list operand is addressable */
    ARCH_DEP(validate_operand) (iopaddr, USE_REAL_ADDR, sizeof(ioparm)-1,
                        ACCTYPE_WRITE, regs);

    /* Fetch the parameter list from real storage */
    ARCH_DEP(vfetchc) (&ioparm, sizeof(ioparm)-1, iopaddr, USE_REAL_ADDR, regs);

    /* Load numeric fields from the parameter list */
    devnum = (ioparm.devnum[0] << 8) | ioparm.devnum[1];
    blksize = (ioparm.blksize[0] << 24)
                | (ioparm.blksize[1] << 16)
                | (ioparm.blksize[2] << 8)
                | ioparm.blksize[3];
    sbiaddr = (ioparm.sbiaddr[0] << 24)
                | (ioparm.sbiaddr[1] << 16)
                | (ioparm.sbiaddr[2] << 8)
                | ioparm.sbiaddr[3];
    sbicount = (ioparm.sbicount[0] << 24)
                | (ioparm.sbicount[1] << 16)
                | (ioparm.sbicount[2] << 8)
                | ioparm.sbicount[3];

    /* Locate the device block */
    dev = find_device_by_devnum (0,devnum);

    /* Set return code 2 and cond code 1 if device does not exist
       or does not support the synchronous I/O call */
    if (dev == NULL || dev->devtype != 0x3370)
    {
        regs->GR_L(15) = 2;
        return 1;
    }

    /* Program check if protect key bits 4-7 are not zero
       or if I/O request type is not read or write */
    if ((ioparm.akey & 0x0F)
        || !(ioparm.type == HCPSBIOP_WRITE
            || ioparm.type == HCPSBIOP_READ))
    {
        ARCH_DEP(program_interrupt) (regs, PGM_OPERAND_EXCEPTION);
    }

    /* Set return code 8 and cond code 2 if blocksize is invalid */
    if (!(blksize == 512 || blksize == 1024
            || blksize == 2048 || blksize == 4096))
    {
        regs->GR_L(15) = 8;
        return 2;
    }

    /* Program check if SBILIST is not on a doubleword boundary */
    if (sbiaddr & 0x00000007)
    {
        ARCH_DEP(program_interrupt) (regs, PGM_OPERAND_EXCEPTION);
    }

    /* Program check if reserved fields are not zero */
    for (accum = 0, i = 0; i < sizeof(ioparm.resv1); i++)
        accum |= ioparm.resv1[i];
    for (i = 0; i < sizeof(ioparm.resv2); i++)
        accum |= ioparm.resv2[i];
    if (accum != 0)
    {
        ARCH_DEP(program_interrupt) (regs, PGM_OPERAND_EXCEPTION);
    }

    /* Set return code 11 and cond code 2 if SBI count is invalid */
    if (sbicount < 1 || sbicount > 500)
    {
        regs->GR_L(15) = 11;
        return 2;
    }

    OBTAIN_DEVLOCK( dev );
    {
#if defined( FEATURE_CHANNEL_SUBSYSTEM )
        /* Return code 5 and condition code 1 if status pending */
        if ((dev->scsw.flag3 & SCSW3_SC_PEND)
            || (dev->pciscsw.flag3 & SCSW3_SC_PEND))
        {
            RELEASE_DEVLOCK( dev );
            regs->GR_L(15) = 5;
            return 1;
        }
#endif

        /* Return code 5 and condition code 1 if device is busy */
        if (dev->busy || IOPENDING(dev))
        {
            RELEASE_DEVLOCK( dev );
            regs->GR_L(15) = 5;
            return 1;
        }

        /* Set the device busy indicator */
        dev->busy = 1;
    }
    RELEASE_DEVLOCK( dev );

    /* Process each entry in the SBILIST */
    for (blkcount = 0; blkcount < sbicount; blkcount++)
    {
        /* Return code 10 and cond code 2 if SBILIST entry
           is outside main storage or is fetch protected.
           Note that the SBI address is an absolute address
           and is not subject to fetch-protection override
           or storage-protection override mechanisms, and
           an SBILIST entry cannot cross a page boundary */
        if (sbiaddr > regs->mainlim
            || ((ARCH_DEP( get_storage_key )( sbiaddr ) & STORKEY_FETCH)
                && (ARCH_DEP( get_storage_key )( sbiaddr ) & STORKEY_KEY) != ioparm.akey
                && ioparm.akey != 0))
        {
            regs->GR_L(15) = 10;
            return 2;
        }

        /* Load block number and data address from SBILIST */
        blknum = ARCH_DEP(fetch_fullword_absolute)(sbiaddr, regs);
        absadr = ARCH_DEP(fetch_fullword_absolute)(sbiaddr+4, regs);

        if (dev->ccwtrace)
        {
            WRMSG(HHC01952, "I",
                    LCSS_DEVNUM,
                    (ioparm.type == HCPSBIOP_WRITE ? "WRITE" : "READ"),
                    blknum, absadr, blksize);
        }

        /* Return code 12 and cond code 2 if buffer exceeds storage */
        if (absadr > regs->mainlim - blksize)
        {
            regs->GR_L(15) = 12;
            return 2;
        }

        /* Channel protection check if access key does not match
           storage keys of buffer.  Note that the buffer address is
           an absolute address, the buffer cannot span more than two
           pages, and the access is not subject to fetch-protection
           override, storage-protection override, or low-address
           protection */
        skey1 = ARCH_DEP( get_storage_key )( absadr );
        skey2 = ARCH_DEP( get_storage_key )( absadr + blksize - 1 );
        if (ioparm.akey != 0
            && (
                   ((skey1 & STORKEY_KEY) != ioparm.akey
                    && ((skey1 & STORKEY_FETCH)
                        || ioparm.type == HCPSBIOP_READ))
                || ((skey2 & STORKEY_KEY) != ioparm.akey
                    && ((skey2 & STORKEY_FETCH)
                        || ioparm.type == HCPSBIOP_READ))
            ))
        {
            chanstat |= CSW_PROTC;
            break;
        }

        /* Call device handler to read or write one block */
        fbadasd_syncblk_io (dev, ioparm.type, blknum, blksize,
                            regs->mainstor + absadr,
                            &unitstat, &residual);

        /* Set incorrect length if residual count is non-zero */
        if (residual != 0)
            chanstat |= CSW_IL;

        /* Exit if any unusual status */
        if (unitstat != (CSW_CE | CSW_DE) || chanstat != 0)
            break;

        /* Point to next SBILIST entry */
        sbiaddr += 8;

    } /* end for(blkcount) */

    /* Reset the device busy indicator */
    dev->busy = 0;

    /* Store the block count in the parameter list */
    ioparm.blkcount[0] = (blkcount >> 24) & 0xFF;
    ioparm.blkcount[1] = (blkcount >> 16) & 0xFF;
    ioparm.blkcount[2] = (blkcount >> 8) & 0xFF;
    ioparm.blkcount[3] = blkcount & 0xFF;

    /* Store the device and subchannel status in the parameter list */
    ioparm.unitstat = unitstat;
    ioparm.chanstat = chanstat;

    /* Store the residual byte count in the parameter list */
    ioparm.residual[0] = (residual >> 8) & 0xFF;
    ioparm.residual[1] = residual & 0xFF;

    /* Return sense data if unit check occurred */
    if (unitstat & CSW_UC)
    {
        numsense = dev->numsense;
        if (numsense > sizeof(ioparm.sense))
            numsense = sizeof(ioparm.sense);
        ioparm.sensecount[0] = (numsense >> 8) & 0xFF;
        ioparm.sensecount[1] = numsense & 0xFF;
        memcpy (ioparm.sense, dev->sense, numsense);
    }

    /* Store the updated parameter list in real storage */
    ARCH_DEP(vstorec) (&ioparm, sizeof(ioparm)-1, iopaddr, USE_REAL_ADDR, regs);

    /* If I/O error occurred, set return code 13 and cond code 3 */
    if (unitstat != (CSW_CE | CSW_DE) || chanstat != 0)
    {
        regs->GR_L(15) = 13;
        return 3;
    }

    /* Set return code 0 and cond code 0 */
    regs->GR_L(15) = 0;
    return 0;

} /* end function syncblk_io */

/*-------------------------------------------------------------------*/
/* Process Synchronous General I/O call (Function code 0x0A8)        */
/*-------------------------------------------------------------------*/
int ARCH_DEP(syncgen_io) (int r1, int r2, REGS *regs)
{
U32             i;                      /* Array subscript           */
U32             numsense;               /* Number of sense bytes     */
U32             iopaddr;                /* Address of HCPSGIOP       */
HCPSGIOP        ioparm;                 /* I/O parameter list        */
DEVBLK         *dev;                    /* -> Device block           */
U16             devnum;                 /* Device number             */
U16             residual;               /* Residual byte count       */
U32             ccwaddr;                /* Address of channel program*/
U32             lastccw;                /* CCW address at interrupt  */
BYTE            accum;                  /* Work area                 */
BYTE            unitstat = 0;           /* Device status             */
BYTE            chanstat = 0;           /* Subchannel status         */

//FIXME: code not right for shared devices

    UNREFERENCED(r2);

    /* Register R1 contains the real address of the parameter list */
    iopaddr = regs->GR_L(r1);

    /* Program check if parameter list not on fullword boundary */
    if (iopaddr & 0x00000003)
    {
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
    }

    /* Ensure that parameter list operand is addressable */
    ARCH_DEP(validate_operand) (iopaddr, USE_REAL_ADDR, sizeof(ioparm)-1,
                        ACCTYPE_WRITE, regs);

    /* Fetch the parameter list from real storage */
    ARCH_DEP(vfetchc) (&ioparm, sizeof(ioparm)-1, iopaddr, USE_REAL_ADDR, regs);

    /* Load numeric fields from the parameter list */
    devnum = (ioparm.devnum[0] << 8) | ioparm.devnum[1];
    ccwaddr = (ioparm.ccwaddr[0] << 24)
                | (ioparm.ccwaddr[1] << 16)
                | (ioparm.ccwaddr[2] << 8)
                | ioparm.ccwaddr[3];

    /* Locate the device block */
    dev = find_device_by_devnum (0,devnum);

    /* Set return code 1 and cond code 1 if device does not exist */
    if (dev == NULL)
    {
        regs->GR_L(15) = 1;
        return 1;
    }

    /* Program check if protect key bits 4-7 are not zero
       or if the reserved bits in the flag byte are not zero */
    if ((ioparm.akey & 0x0F) || (ioparm.flag & HCPSGIOP_FLAG_RESV))
    {
        ARCH_DEP(program_interrupt) (regs, PGM_OPERAND_EXCEPTION);
    }

#ifdef FEATURE_S370_CHANNEL
    /* Program check if flag byte specifies format-1 CCW */
    if (ioparm.flag & HCPSGIOP_FORMAT1_CCW)
    {
        ARCH_DEP(program_interrupt) (regs, PGM_OPERAND_EXCEPTION);
    }
#endif /*FEATURE_S370_CHANNEL*/

    /* Program check if CCW is not on a doubleword boundary,
       or if CCW address exceeds maximum according to CCW format */
    if ((ccwaddr & 0x00000007) || ccwaddr >
           ((ioparm.flag & HCPSGIOP_FORMAT1_CCW) ?
                        (U32)0x7FFFFFFF : (U32)0x00FFFFFF))
    {
        ARCH_DEP(program_interrupt) (regs, PGM_OPERAND_EXCEPTION);
    }

    /* Program check if reserved fields are not zero */
    for (accum = 0, i = 0; i < sizeof(ioparm.resv1); i++)
        accum |= ioparm.resv1[i];
    for (i = 0; i < sizeof(ioparm.resv2); i++)
        accum |= ioparm.resv2[i];
    for (i = 0; i < sizeof(ioparm.resv3); i++)
        accum |= ioparm.resv3[i];
    for (i = 0; i < sizeof(ioparm.resv4); i++)
        accum |= ioparm.resv4[i];
    if (accum != 0)
    {
        ARCH_DEP(program_interrupt) (regs, PGM_OPERAND_EXCEPTION);
    }

    OBTAIN_DEVLOCK( dev );
    {
#if defined( FEATURE_CHANNEL_SUBSYSTEM )
        /* Return code 5 and condition code 1 if status pending */
        if ((dev->scsw.flag3 & SCSW3_SC_PEND)
            || (dev->pciscsw.flag3 & SCSW3_SC_PEND))
        {
            RELEASE_DEVLOCK( dev );
            regs->GR_L(15) = 5;
            return 1;
        }
#endif

        /* Return code 5 and condition code 1 if device is busy */
        if (dev->busy || IOPENDING(dev))
        {
            RELEASE_DEVLOCK( dev );
            regs->GR_L(15) = 5;
            return 1;
        }

        /* Set the device busy indicator */
        dev->busy = 1;
    }
    RELEASE_DEVLOCK( dev );

    /* Build the operation request block */
    memset (&dev->orb, 0, sizeof(ORB));
    STORE_FW(dev->orb.ccwaddr, ccwaddr);
    dev->orb.flag4 = ioparm.akey & ORB4_KEY;
    if (ioparm.flag & HCPSGIOP_FORMAT1_CCW)
        dev->orb.flag5 |= ORB5_F;

    /* Execute the channel program synchronously */
    ARCH_DEP(execute_ccw_chain) (dev);

    /* Obtain status, CCW address, and residual byte count */
    lastccw = (dev->scsw.ccwaddr[0] << 24)
                | (dev->scsw.ccwaddr[1] << 16)
                | (dev->scsw.ccwaddr[2] << 8)
                | dev->scsw.ccwaddr[3];
    unitstat = dev->scsw.unitstat;
    chanstat = dev->scsw.chanstat;
    residual = (dev->scsw.count[0] << 8) | dev->scsw.count[1];

    /* Clear the interrupt pending and device busy conditions */
    OBTAIN_DEVLOCK( dev );
    {
        dev->busy = dev->pending = 0;
        dev->scsw.flag2 = 0;
        dev->scsw.flag3 = 0;
    }
    RELEASE_DEVLOCK( dev );

    /* Store the last CCW address in the parameter list */
    ioparm.lastccw[0] = (lastccw >> 24) & 0xFF;
    ioparm.lastccw[1] = (lastccw >> 16) & 0xFF;
    ioparm.lastccw[2] = (lastccw >> 8) & 0xFF;
    ioparm.lastccw[3] = lastccw & 0xFF;

    /* Store the device and subchannel status in the parameter list */
    ioparm.unitstat = unitstat;
    ioparm.chanstat = chanstat;

    /* Store the residual byte count in the parameter list */
    ioparm.residual[0] = (residual >> 8) & 0xFF;
    ioparm.residual[1] = residual & 0xFF;

    /* Return sense data if unit check occurred */
    if (unitstat & CSW_UC)
    {
        numsense = dev->numsense;
        if (numsense > sizeof(ioparm.sense))
            numsense = sizeof(ioparm.sense);
        ioparm.sensecount[0] = (numsense >> 8) & 0xFF;
        ioparm.sensecount[1] = numsense & 0xFF;
        memcpy (ioparm.sense, dev->sense, numsense);
    }

    /* Store the updated parameter list in real storage */
    ARCH_DEP(vstorec) (&ioparm, sizeof(ioparm)-1, iopaddr, USE_REAL_ADDR, regs);

    /* If I/O error occurred, set return code 13 and cond code 3 */
    if (unitstat != (CSW_CE | CSW_DE) || chanstat != 0)
    {
        regs->GR_L(15) = 13;
        return 3;
    }

    /* Return with condition code 0 and register 15 unchanged */
    return 0;

} /* end function syncgen_io */

/*-------------------------------------------------------------------*/
/* Store Extended Identification Code (Function code 0x000)          */
/*-------------------------------------------------------------------*/
void ARCH_DEP(extid_call) (int r1, int r2, REGS *regs)
{
int        i;                           /* Array subscript           */
int        ver, rel;                    /* Version and release number*/
int        tzdiff;                      /* Time zone differential    */
U32        idaddr;                      /* Address of storage operand*/
U32        idlen;                       /* Length of storage operand */
BYTE       buf[40];                     /* Extended identification   */
#if defined( HAVE_GETLOGIN_R )
  #if !defined(LOGIN_NAME_MAX)
    #define LOGIN_NAME_MAX 100
  #endif
char       unam[LOGIN_NAME_MAX+1];      /* User name                 */
#endif
char      *puser;                       /* Pointer to user name      */
BYTE       c;                           /* Character work area       */

    /* Load storage operand address from R1 register */
    idaddr = regs->GR_L(r1);

    /* Program check if operand is not on a doubleword boundary */
    if (idaddr & 0x00000007)
    {
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
    }

    /* Load storage operand length from R2 register */
    idlen = regs->GR_L(r2);

    /* Program check if operand length is invalid */
    if (idlen < 1)
    {
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
    }

    /* Bytes 0-7 contain the system name ("HERCULES" in EBCDIC) */
    get_lparname(buf);

    /* Bytes 8-9 contain the execution environment bits */
    buf[8] = 0x00;
    buf[9] = 0x00;

    /* Byte 10 contains the system product version number */
    sscanf (QSTR(VERSION), "%d.%d", &ver, &rel);
    buf[10] = ver & 0xff;

    /* Byte 11 contains version number from STIDP */
    buf[11] = regs->cpuversion;

    /* Bytes 12-13 contain MCEL length from STIDP */
    buf[12] = (regs->cpuid >> 8) & 0xFF;
    buf[13] = regs->cpuid & 0xFF;

    /* Bytes 14-15 contain the CP address */
    buf[14] = (regs->cpuad >> 8) & 0xFF;
    buf[15] = regs->cpuad & 0xFF;

    /* Bytes 16-23 contain the userid in EBCDIC */
#if defined( HAVE_GETLOGIN_R )
    memset( unam, 0, sizeof(unam) );
    VERIFY( getlogin_r ( unam, sizeof(unam) ) == 0 );
    puser = unam;
#else
    puser = "";
#endif
    for (i = 0; i < 8; i++)
    {
        c = (*puser == '\0' ? SPACE : *(puser++));
        buf[16+i] = host_to_guest(toupper((unsigned char)c));
    }

    /* Bytes 24-31 contain the program product bitmap */
    memcpy (buf+24, "\x7F\xFE\x00\x00\x00\x00\x00\x00", 8);

    /* Bytes 32-35 contain the time zone differential */
    tzdiff = query_tzoffset();   /* returns +/-HHMM as an integer */
    tzdiff = ((tzdiff/100)*3600)+((tzdiff%100)*60);
    STORE_FW(buf+32,tzdiff);

    /* Bytes 36-39 contain version, level, and service level */
    buf[36] = ver & 0xff;
    buf[37] = rel & 0xff;
    buf[38] = 0x00;
    buf[39] = 0x00;

#if 0 // debug
    LOGMSG( "Diagnose X'000':"

            "%2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X "
            "%2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X\n\t\t"

            "%2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X "
            "%2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X\n\t\t"

            "%2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X\n"

            , buf[ 0], buf[ 1], buf[ 2], buf[ 3], buf[ 4], buf[ 5], buf[ 6], buf[ 7]
            , buf[ 8], buf[ 9], buf[10], buf[11], buf[12], buf[13], buf[14], buf[15]

            , buf[16], buf[17], buf[18], buf[19], buf[20], buf[21], buf[22], buf[23]
            , buf[24], buf[25], buf[26], buf[27], buf[28], buf[29], buf[30], buf[31]

            , buf[32], buf[33], buf[34], buf[35], buf[36], buf[37], buf[38], buf[39]
        );
#endif

    /* Enforce maximum length to store */
    if (idlen > sizeof(buf))
        idlen = sizeof(buf);

    /* Store the extended identification code at operand address */
    ARCH_DEP(vstorec) (buf, idlen-1, idaddr, USE_REAL_ADDR, regs);

    /* Deduct number of bytes from the R2 register */
    regs->GR_L(r2) -= idlen;

} /* end function extid_call */

/*-------------------------------------------------------------------*/
/* Process CP command (Function code 0x008)                          */
/*-------------------------------------------------------------------*/
int ARCH_DEP(cpcmd_call) (int r1, int r2, REGS *regs)
{
U32     i;                              /* Array subscript           */
U32     cc;                             /* Condition code            */
U32     cmdaddr;                        /* Address of command string */
U32     cmdlen;                         /* Length of command string  */
U32     respadr;                        /* Address of response buffer*/
U32     maxrlen;                        /* Length of response buffer */
BYTE    cmdflags;                       /* Command flags             */

#define CMDFLAGS_REJPASSW     0x80      /* Reject password in command*/
#define CMDFLAGS_RESPONSE     0x40      /* Return response in buffer */
#define CMDFLAGS_REQPASSW     0x20      /* Prompt for password       */
#define CMDFLAGS_RESERVED     0x1F      /* Reserved bits, must be 0  */

char    msg[256];                       /* Message work area         */
char    cmd[256+1];                     /* Input command +1 for NULL */
char*   resp;                           /* Output response           */
int     freeresp;                       /* Flag to free resp         */

    /* Obtain address of command from R1 register
       and command length and flags from R2 register
    */
    cmdaddr  = regs->GR_L( r1 );
    cmdflags = regs->GR_L( r2 ) >> 24;
    cmdlen   = regs->GR_L( r2 ) & 0x00FFFFFF;

    /* Program check if invalid flags, or if command string
       is too long, or if response buffer is specified and
       registers are consecutive or either register specifies
       register 15
    */
    if (0
        || (cmdflags & CMDFLAGS_RESERVED)
        || (cmdlen > (sizeof(cmd)-1))   // (room for NULL terminator!)
        || (1
            && (cmdflags & CMDFLAGS_RESPONSE)
            && (0
                || r1 == r2 + 1
                || r2 == r1 + 1
                || r1 == 15
                || r2 == 15
               )
           )
    )
    {
        ARCH_DEP( program_interrupt )( regs, PGM_SPECIFICATION_EXCEPTION );
    }

    /* Put machine into stopped state if command length is zero */
    if (cmdlen == 0)
    {
        regs->opinterv = 0;
        regs->cpustate = CPUSTATE_STOPPED;
        ON_IC_INTERRUPT( regs );
        return 0;
    }

    /* Obtain the EBCDIC command string from guest storage */
    ARCH_DEP( vfetchc )( cmd, cmdlen-1, cmdaddr, USE_REAL_ADDR, regs );

    /* Translate guest EBCDIC command to host ASCII format */
    for (i=0; i < cmdlen; i++)
        cmd[i] = guest_to_host( cmd[i] );
    cmd[i] = 0; // (null terminate!)

    /* Execute the Hercules emulator (hypervisor) command */
    resp = "";
    freeresp = 0;

#if !defined( FEATURE_HERCULES_DIAGCALLS )
        // "Hercules-specific DIAG instructions support not included in engine build"
        MSGBUF( msg, MSG( HHC01954, "E" ));
        resp = &msg[0];
#else
    /************************************************************
     *  SECURITY CHECK: Reject attempts to issue "shell" type
     *  commands unless specifically allowed by SHCMDOPT option.
     ************************************************************/
    if (1
        && is_shell_command( cmd )
        && (sysblk.shcmdopt & (SHCMDOPT_ENABLE + SHCMDOPT_DIAG8))
                           != (SHCMDOPT_ENABLE + SHCMDOPT_DIAG8)
    )
    {
        // "DIAG8 access to shell disallowed by SHCMDOPT setting"
        MSGBUF( msg, MSG( HHC01953, "E" ));
        resp = &msg[0];
    }
    else
    {
        /* It's either not a "shell" type command or the
           SHCMDOPT option is allowing them to be issued
           so we can go ahead and execute the command. */

        // "%s guest issued panel command: %s"
        if (sysblk.diag8opt & DIAG8CMD_ECHO)
            PWRMSG( WRMSG_PANEL, HHC01950, "I", "Starting", RTRIM( cmd ));

        /* Issue the command and capture the response */
        if (cmdflags & CMDFLAGS_RESPONSE)
        {
            panel_command_capture( cmd, &resp, false );

            if (resp)
                freeresp = 1;
            else
                resp = "";
        }
        else /* Issue command normally (no response wanted) */
        {
            panel_command( cmd );
        }

        // "%s guest issued panel command: %s"
        if (sysblk.diag8opt & DIAG8CMD_ECHO)
            PWRMSG( WRMSG_PANEL, HHC01950, "I", "Completed", RTRIM( cmd ));
    }
#endif // defined( FEATURE_HERCULES_DIAGCALLS )

    /* Copy response to guest storage if response requested */
    if (cmdflags & CMDFLAGS_RESPONSE)
    {
        U32 resplen, chunk, rem;

        /* Translate host ASCII response to guest EBCDIC format */
        for (i=0, resplen = (U32) strlen( resp ); i < resplen; i++)
            resp[i] = host_to_guest( resp[i] );

        /* Retrieve guest's response parameters */
        respadr = regs->GR_L( r1+1 );
        maxrlen = regs->GR_L( r2+1 );

        /* Copy the response to guest storage in 256 byte chunks */
        for (i=0, chunk=256, rem = MIN( resplen, maxrlen ); rem; rem -= chunk, i += chunk)
        {
            if (rem < chunk) chunk = rem;
            ARCH_DEP( vstorec )( &resp[i], chunk-1, respadr+i, USE_REAL_ADDR, regs );
        }

        /* Update guest response register and set condition code */
        if (resplen <= maxrlen)
        {
            /* They have the complete response */
            regs->GR_L( r2+1 ) = resplen;
            cc = 0;
        }
        else
        {
            /* Their response buffer is too small */
            regs->GR_L( r2+1 ) = (resplen - maxrlen);
            cc = 1;
        }
    }
    else
        cc = 0;

    /* Set R2 register to CP completion code */
    regs->GR_L( r2 ) = 0;

    /* Free memory malloc'ed by panel_command_capture */
    if (freeresp)
        free( resp );

    /* Return condition code */
    return cc;

} /* end function cpcmd_call */

/*-------------------------------------------------------------------*/
/* Access Re-IPL data (Function code 0x0B0)                          */
/*-------------------------------------------------------------------*/
void ARCH_DEP(access_reipl_data) (int r1, int r2, REGS *regs)
{
U32     bufadr;                         /* Real addr of data buffer  */
U32     buflen;                         /* Length of data buffer     */

    /* Obtain buffer address and length from R1 and R2 registers */
    bufadr = regs->GR_L(r1);
    buflen = regs->GR_L(r2);

    /* Program check if buffer length is negative */
    if ((S32)buflen < 0)
    {
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
    }

    /* Store IPL information if buffer length is non-zero */
    if (buflen > 0)
    {
        /* Store one byte of zero to indicate no IPL information */
        ARCH_DEP(vstoreb) (0, bufadr, USE_REAL_ADDR, regs);
    }

    PTT_ERR("*DIAG0B0",regs->GR_L(r1),regs->GR_L(r2),regs->psw.IA_L);

    /* Return code 4 means no re-IPL information available */
    regs->GR_L(r2) = 4;

} /* end function access_reipl_data */

/*-------------------------------------------------------------------*/
/* Access Device Information (Function code 0x210)                   */
/*-------------------------------------------------------------------*/
/* Note: This implementation emulates z/VM 5.4                       */
int  ARCH_DEP(device_info) (int r1, int r2, REGS *regs)
{
DEVBLK   *dev;             /* -> Device block                        */
VRDCBLOK vrdc;             /* VRDCBLOK                               */
RADR     blokaddr;         /* Location of the VRDCBLOK               */
U16      bloklen;          /* Length from the VRDCBLOK               */
#if 0
/* Only required if implementation is for the z/VM 5.3 level */
U16      reserved;         /* Bytes 14 and 15                        */
#endif
U16      devnum;           /* Device number from the VRDCBLOK        */


    UNREFERENCED(r2);

    if (regs->GR_L(r1) & 0x3
#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
        || (regs->psw.amode64)
#endif /* FEATURE_001_ZARCH_INSTALLED_FACILITY */
       )
    {
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
    }

    blokaddr = regs->GR_L(r1);

    /* Fetch the first 4 bytes of the VRDCBLOK */
    ARCH_DEP(vfetchc) (&vrdc, 3, blokaddr, USE_REAL_ADDR, regs);

    /* Get the VRDCBLOK length from the working VRDC */
    FETCH_HW(bloklen,&vrdc.vrdclen);

    /* VRDCBLOK length must be at least 8 bytes */
    if (bloklen<8)
    {
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
    }

    /* Fetch remainder of supplied VRDCBLOK, but no more */
    if (bloklen>VRDCBLOK_SIZE)
       bloklen=VRDCBLOK_SIZE;
    ARCH_DEP(vfetchc) (&vrdc.vrdcvdat,bloklen-5,blokaddr+4, USE_REAL_ADDR, regs);

#if 0
    /* If length is 16 or greater, bytes 14 and 15 must be zero on z/VM 5.3.0 or earlier */
    if ( bloklen>=16)
    {
         FETCH_HW(reserved,&vrdc.vrdcrsvd);
         if (reserved != 0)
         {
              ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
         }
    }
#endif

    /* Get the device number from the working VRDC */
    FETCH_HW(devnum,&vrdc.vrdcdvno);

    /* Locate the device block and set the virtual and real device information */
    dev = ARCH_DEP(vmdevice_data) (0x210,devnum,&vrdc.vrdcvdat,&vrdc.vrdcrcdt);

    /* Return condition code 3 if device does not exist */
    if (!dev)
    {
        PTT_ERR("*DIAG210",regs->GR_L(r1),regs->GR_L(r2),regs->psw.IA_L);
        return 3;
    }

    /* Set the underlying device and real device features */
    vrdc.vrdcundv=0x00;
    vrdc.vrdcrdaf=0x00;

    /* Create device dependent mappings */
    if (vrdc.vrdcvdat.vdevcls == DC_DASD)
    {
        memcpy(&vrdc.vrdcrdc,dev->devchar,42);
        switch (dev->devtype)
        {
             case 0x2311:
             case 0x2314:
             case 0x2305:
             case 0x3330:
             case 0x3340:
             case 0x3350:
                  /* Set non-keyed overhead */
                  STORE_HW(&vrdc.vrdcrdc[0x18],dev->ckdtab->f2);
                  /* Set keyed overhead */
                  STORE_HW(&vrdc.vrdcrdc[0x1A],dev->ckdtab->f1);
             /* Note: for all other DASD devices these fields contain bytes 24-27 of the RDC */
        }
        /* Set Control Unit ID */
        vrdc.vrdcrdc[0x2A]=dev->devchar[56];
    }
    else if (vrdc.vrdcvdat.vdevcls == DC_FBA)
        memcpy(&vrdc.vrdcrdc,dev->devchar,32);

    /* Set Path Group ID */
    memcpy(&vrdc.vrdcpgid,dev->pgid,11);

    /* Set version */
    if (bloklen>0x60)
       vrdc.vrdcvers=0x01;

    /* Set underlying real device */
    if (!(vrdc.vrdcrdaf & VRDCEMRD))
       memcpy(&vrdc.vrdcrdev,&vrdc.vrdcdvno,2);

    /* Update the VRDC in main storage */
    ARCH_DEP(vstorec) (&vrdc, bloklen-1, blokaddr, USE_REAL_ADDR, regs);

    /* Return condition code 0 for success */
    return 0;

} /* end function device_info */


/*-------------------------------------------------------------------*/
/* Access Certain Virtual Machine Information (Function code 0x260)  */
/*-------------------------------------------------------------------*/
/* Note: This implementation emulates z/VM 5.4                       */
void ARCH_DEP(vm_info) (int r1, int r2, REGS *regs)
{
DEVBLK  *dev;                          /* -> Device block            */
U16     devnum;                        /* Device number              */
#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
RADR    stgarea;                       /* Storage extent area        */
S64     stglen;                        /* Storage extent area length */
#endif /* FEATURE_001_ZARCH_INSTALLED_FACILITY */

    /* Ry contains the subcode */
    switch(regs->GR_L(r2))
    {
    case 0x00000000: /* Highest addressable byte */
#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)

        /* Program check if running in z/Architecture mode and */
        /* 64-bit addressing is being used.                    */
        if (regs->psw.amode64)
        {
            ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
        }
#endif /* FEATURE_001_ZARCH_INSTALLED_FACILITY */
        regs->GR_L(r1) = regs->mainlim; /* provide highest addressable byte */
        return;

    case 0x00000004: /* Provide BYUSER ID value */

        /* Program check if Rx and Ry are the same registers or        */
        /* or Ry is not an even register or the address provided       */
        /* in Rx is not on a doubleword boundary or if running         */
        /* in z/Architecture mode and 64-bit addressing is being used. */
        if ( r1 == r2 || r2 & 0x1 || regs->GR_L(r1) & 0x7
#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
             || (regs->psw.amode64)
#endif /* FEATURE_001_ZARCH_INSTALLED_FACILITY */
           )
        {
            ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
        }
        regs->GR_L(r2+1)=0x4; /* Indicate no BYUSER ID for Hercules */
        return;

    case 0x00000008: /* Return number of lines per page */
#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)

        /* Program check if running in z/Architecture mode and */
        /* 64-bit addressing is being used.                    */
        if (regs->psw.amode64)
        {
            ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
        }
#endif /* FEATURE_001_ZARCH_INSTALLED_FACILITY */

        /* Get the device number from the Rx register */
        devnum=regs->GR_LHL(r1);

        /* Locate the device block */
        dev = find_device_by_devnum(0,devnum);

        /* Set 0 lines per page for a valid printer or console (meaning SPOOL is OFF) */
        if (dev != NULL &&
              (dev->devtype == 0x1403 ||
               dev->devtype == 0x3211 ||
               dev->devtype == 0x1052 ||
               dev->devtype == 0x3215 )
            )
        {
           regs->GR_L(r1) = 0; /* Set zero lines per page */
           regs->GR_L(r2) = 0; /* Set return code to indicate a valid device */
        }
        else
        {
           regs->GR_L(r2) = 4; /* Set return code to indicate an invalid device */
        }
        return;

#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
    case 0x0000000C: /* Return highest addressable byte for z/Architecture machine */
         regs->GR_G(r1) = regs->mainlim;
         regs->GR_G(r2) = regs->mainlim;
         return;

    case 0x00000010: /* Set storage extent */

         /* Obtain the storage extent area real address from Rx */
         /* and its length from Rx+1                            */
         stgarea=regs->GR_G(r1);
         stglen=regs->GR_G(r1+1); /* Length is treated as a signed value */

         /* Program check if Rx is not an even register or the address */
         /* provided in Rx is not on a quadword boundary or the length */
         /* provided in Rx+1 is not positive or not a multiple of 16   */
         if ( r1 & 1 || stgarea & 0xF || stglen <= 0 || stglen & 0xF )
         {
             ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
         }

         /* Convert real address to absolute address */
         stgarea=APPLY_PREFIXING(stgarea,regs->PX );

         /* Check to ensure extent information can be stored */
         if (stgarea > regs->mainlim - 16)
         {
             regs->program_interrupt (regs, PGM_ADDRESSING_EXCEPTION);
         }
         /* Set start of storage extent to zero */
         ARCH_DEP(store_doubleword_absolute)(0,stgarea,regs);
         /* Set end of storage extent to last addressable byte of main storage */
         ARCH_DEP(store_doubleword_absolute)(regs->mainlim,stgarea+8,regs);
         /* Set number of extents to 1 in Ry */
         regs->GR_G(r2) = 1;
         /* Indicate all extents returned */
         regs->psw.cc = 0;
         return;
#endif /* FEATURE_001_ZARCH_INSTALLED_FACILITY */

    default: /* Invalid subcode */
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
    }
}


/*-------------------------------------------------------------------*/
/* Pseudo Timer Extended (Function code 0x270)                       */
/* Pseudo Timer (Function code 0x00C)                                */
/*-------------------------------------------------------------------*/
void ARCH_DEP(pseudo_timer) (U32 code, int r1, int r2, REGS *regs)
{
int     i;                              /* Array subscript           */
time_t  timeval;                        /* Current time              */
struct  tm *tmptr;                      /* -> Current time structure */
U32     bufadr;                         /* Real addr of data buffer  */
U32     buflen;                         /* Length of data buffer     */
char    buf[64];                        /* Response buffer           */
BYTE    dattim[64];                     /* Date and time (EBCDIC)    */
#define DIAG_DATEFMT_SHORT      0x80    /* Date format mm/dd/yy      */
#define DIAG_DATEFMT_FULL       0x40    /* Date format mm/dd/yyyy    */
#define DIAG_DATEFMT_ISO        0x20    /* Date format yyyy-mm-dd    */
#define DIAG_DATEFMT_SYSDFLT    0x10    /* System-wide default format*/
static  char timefmt[]="%m/%d/%y%H:%M:%S%m/%d/%Y%Y-%m-%d";

    /* Get the current date and time in EBCDIC */
    timeval = time(NULL);
    tmptr = localtime(&timeval);
    strftime((char *)dattim, sizeof(dattim), timefmt, tmptr);
    for (i = 0; dattim[i] != '\0'; i++)
        dattim[i] = host_to_guest(dattim[i]);

    /* Obtain buffer address and length from R1 and R2 registers */
    bufadr = regs->GR_L(r1);
    buflen = regs->GR_L(r2);

    /* Use length 32 if R2 is zero or function code is 00C */
    if (r2 == 0 || code == 0x00C)
        buflen = 32;

    /* Program check if R1 and R2 specify the same non-zero
       register number, or if buffer length is less than or
       equal to zero, or if buffer address is zero, or if
       buffer is not on a doubleword boundary */
    if ((r2 != 0 && r2 == r1)
        || (S32)buflen <= 0
        || bufadr == 0
        || (bufadr & 0x00000007))
    {
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
    }

    /* Build the response buffer */
    memset (buf, 0, sizeof(buf));
    /* Bytes 0-7 contain the date as EBCDIC MM/DD/YY */
    memcpy (buf, dattim, 8);
    /* Bytes 8-15 contain the time as EBCDIC HH:MM:SS */
    memcpy (buf+8, dattim+8, 8);
    /* Bytes 16-23 contain the virtual CPU time used in microseconds */
    /* Bytes 24-31 contain the total CPU time used in microseconds */
    /* Bytes 32-41 contain the date as EBCDIC MM/DD/YYYY */
    memcpy (buf+32, dattim+16, 10);
    /* Bytes 42-47 contain binary zeroes */
    /* Bytes 48-57 contain the date as EBCDIC YYYY-MM-DD */
    memcpy (buf+48, dattim+26, 10);
    /* Byte 58 contains the diagnose 270 version code */
    buf[58] = 0x01;
    /* Byte 59 contains the user's default date format */
    buf[59] = DIAG_DATEFMT_ISO;
    /* Byte 60 contains the system default date format */
    buf[60] = DIAG_DATEFMT_ISO;
    /* Bytes 61-63 contain binary zeroes */

#if 0 // debug
    LOGMSG( "Diagnose X'%3.3X':"

            "%2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X "
            "%2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X\n\t\t"

            "%2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X "
            "%2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X\n\t\t"

            "%2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X "
            "%2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X\n\t\t"

            "%2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X "
            "%2.2X%2.2X%2.2X%2.2X %2.2X%2.2X%2.2X%2.2X\n"

            , code

            , buf[ 0], buf[ 1], buf[ 2], buf[ 3], buf[ 4], buf[ 5], buf[ 6], buf[ 7]
            , buf[ 8], buf[ 9], buf[10], buf[11], buf[12], buf[13], buf[14], buf[15]

            , buf[16], buf[17], buf[18], buf[19], buf[20], buf[21], buf[22], buf[23]
            , buf[24], buf[25], buf[26], buf[27], buf[28], buf[29], buf[30], buf[31]

            , buf[32], buf[33], buf[34], buf[35], buf[36], buf[37], buf[38], buf[39]
            , buf[40], buf[41], buf[42], buf[43], buf[44], buf[45], buf[46], buf[47]

            , buf[48], buf[49], buf[50], buf[51], buf[52], buf[53], buf[54], buf[55]
            , buf[56], buf[57], buf[58], buf[59], buf[60], buf[61], buf[63], buf[63]
        );
#endif

    /* Enforce maximum length to store */
    if (buflen > sizeof(buf))
        buflen = sizeof(buf);

    /* Store the response buffer at the operand location */
    ARCH_DEP(vstorec) (buf, buflen-1, bufadr, USE_REAL_ADDR, regs);

} /* end function pseudo_timer */

/*-------------------------------------------------------------------*/
/* Pending Page Release (Function code 0x214)                        */
/*-------------------------------------------------------------------*/
int ARCH_DEP(diag_ppagerel) (int r1, int r2, REGS *regs)
{
U32     abs, start, end;                /* Absolute frame addresses  */
BYTE    skey;                           /* Specified storage key     */
BYTE    func;                           /* Function code...          */
#define DIAG214_EPR             0x00    /* Establish pending release */
#define DIAG214_CPR             0x01    /* Cancel pending release    */
#define DIAG214_CAPR            0x02    /* Cancel all pending release*/
#define DIAG214_CPRV            0x03    /* Cancel and validate       */

    /* Program check if R1 is not an even-numbered register */
    if (r1 & 1)
    {
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
    }

    /* Extract the function code from R1+1 register bits 24-31 */
    func = regs->GR_L(r1+1) & 0xFF;

    /* Extract the start/end addresses from R1 and R1+1 registers */
    start = regs->GR_L(r1) & STORAGE_KEY_PAGEMASK;
    end = regs->GR_L(r1+1) & STORAGE_KEY_PAGEMASK;

    /* Validate start/end addresses if function is not CAPR */
    if (func != DIAG214_CAPR
        && (start > end || end > regs->mainlim))
    {
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
    }

    /* Process depending on function code */
    switch (func)
    {
    case DIAG214_EPR:  /* Establish Pending Release */
        break;

    case DIAG214_CPR:  /* Cancel Pending Release */
    case DIAG214_CPRV: /* Cancel Pending Release and Validate */

        /* Do not set storage keys if R2 is register 0 */
        if (r2 == 0) break;

        /* Obtain key from R2 register bits 24-28 */
        skey = regs->GR_L(r2) & (STORKEY_KEY | STORKEY_FETCH);

        /* Set storage key for each frame within specified range
           without changing existing reference and change bits.
        */
        for (abs = start; abs <= end; abs += STORAGE_KEY_PAGESIZE)
        {
            BYTE refchg = ARCH_DEP( get_storage_key )( abs ) & (STORKEY_REF | STORKEY_CHANGE);
            ARCH_DEP( put_storage_key )( abs, skey | refchg );
        }

        break;

    case DIAG214_CAPR:  /* Cancel All Pending Releases */
        break;

    default:            /* Invalid function code */
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
    } /* end switch(func) */

    /* Return condition code zero */
    return 0;

} /* end function diag_ppagerel */


/*-------------------------------------------------------------------*/
/* B2F0 IUCV  - Inter User Communications Vehicle                [S] */
/*-------------------------------------------------------------------*/
DEF_INST(inter_user_communication_vehicle)
{
int     b2;                             /* Effective addr base       */
VADR    effective_addr2;                /* Effective address         */

    S(inst, regs, b2, effective_addr2);
    PER_ZEROADDR_XCHECK( regs, b2 );

#if defined(FEATURE_ECPSVM)
    if(ecpsvm_doiucv(regs,b2,effective_addr2)==0)
    {
        return;
    }
#endif

    /* Program check if in problem state,
       the IUCV instruction generates an operation exception
       rather then a priviliged operation exception when
       executed in problem state                                 *JJ */
    if ( PROBSTATE(&regs->psw) )
        ARCH_DEP(program_interrupt) (regs, PGM_OPERATION_EXCEPTION);

    SIE_INTERCEPT(regs);

    PTT_ERR("*IUCV",b2,effective_addr2,regs->psw.IA_L);

    /* Set condition code to indicate IUCV not available */
    regs->psw.cc = 3;
}

#endif /*FEATURE_EMULATE_VM*/


#if !defined(_GEN_ARCH)

#if defined(_ARCH_NUM_1)
 #define  _GEN_ARCH _ARCH_NUM_1
 #include "vm.c"
#endif

#if defined(_ARCH_NUM_2)
 #undef   _GEN_ARCH
 #define  _GEN_ARCH _ARCH_NUM_2
 #include "vm.c"
#endif

#endif /*!defined(_GEN_ARCH)*/
