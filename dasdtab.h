/* DASDTAB.H    (C) Copyright Roger Bowler, 1999-2012                */
/*              (C) and others 2013-2023                             */
/*              DASD table structures                                */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* This header file contains defines the table entries that          */
/* describe all DASD devices supported by Hercules.                  */
/* It also contains function prototypes for the DASD table utilities.*/
/*-------------------------------------------------------------------*/

#ifndef _DASDTAB_H
#define _DASDTAB_H

#include "hercules.h"

#define CKD_CONFIG_DATA_SIZE 256
#if 0
#define myssid (dev->devnum & 0xffe0)   /* Storage subsystem identifier
                                           32 devices per subsystem  */
#else
#define myssid SSID(dev)
#endif

/* 32 devices per subsystem */
#define DEVICES_PER_SUBSYS_SHIFT 5
#define DEVICES_PER_SUBSYS (1 << DEVICES_PER_SUBSYS_SHIFT)
#define SSID(_dev) ((_dev)->devnum & ~(DEVICES_PER_SUBSYS-1))
#define IFID(_dev) ((SSID((_dev)) >> DEVICES_PER_SUBSYS_SHIFT) & 0x7)

/* Test for 3990-3 or 3990-6 control unit */
#define MODEL3(_cu) ((_cu)->devt == 0x3990 && (_cu)->model == 0xec)
#define MODEL6(_cu) ((_cu)->devt == 0x3990 && (_cu)->model == 0xe9)

/*-------------------------------------------------------------------*/
/* Definition of a CKD DASD device entry                             */
/*-------------------------------------------------------------------*/
typedef struct _CKDDEV {                /* CKD Device table entry    */
  const char   *name;                   /* Device name               */
        U16     devt;                   /* Device type               */
        BYTE    model;                  /* Device model              */
        BYTE    devclass;               /* Device class: X'20'=DASD  */
#define DEVCLASS_DASD       0x20
        BYTE    code;                   /* Device code               */
        U16     cyls;                   /* Number primary cylinders  */
        U16     altcyls;                /* Number alternate cylinders*/
        U16     heads;                  /* Number heads (trks/cyl)   */
        U16     r0;                     /* R0 max size               */
        U16     r1;                     /* R1 max size               */
        U16     har0;                   /* HA/R0 overhead size       */
        U16     len;                    /* Max length                */
        U16     sectors;                /* Number sectors            */
        U16     rpscalc;                /* RPS calculation factor    */
        S16     formula;                /* Space calculation formula */
        U16     f1,f2,f3,f4,f5,f6;      /* Space calculation factors */
        char   *cu;                     /* Default control unit name */
      } CKDDEV;
#define CKDDEV_SIZE sizeof(CKDDEV)

/*-------------------------------------------------------------------*/
/* Definition of a CKD DASD control unit entry                       */
/*-------------------------------------------------------------------*/
typedef struct _CKDCU {                 /* CKD Control Unit entry    */
        char   *name;                   /* Control Unit name         */
        U16     devt;                   /* Control Unit type         */
        BYTE    model;                  /* Control Unit model        */
        BYTE    code;                   /* Control Unit code         */
        BYTE    funcfeat;               /* Functions/Features        */
        BYTE    typecode;               /* CU Type Code              */
        U32     sctlfeat;               /* Control Unit features     */
        U32     ciw1;                   /* CIW 1                     */
        U32     ciw2;                   /* CIW 2                     */
        U32     ciw3;                   /* CIW 3                     */
        U32     ciw4;                   /* CIW 4                     */
        U32     ciw5;                   /* CIW 5                     */
        U32     ciw6;                   /* CIW 6                     */
        U32     ciw7;                   /* CIW 7                     */
        U32     ciw8;                   /* CIW 8                     */
        U8      senselength;            /* Sense length              */
      } CKDCU;
#define CKDCU_SIZE sizeof(CKDCU)

/*-------------------------------------------------------------------*/
/* Definition of a FBA DASD device entry                             */
/*-------------------------------------------------------------------*/
typedef struct _FBADEV {                /* FBA Device entry          */
        char   *name;                   /* Device name               */
        U16     devt;                   /* Device type               */
        BYTE    devclass;               /* Device class              */
        BYTE    type;                   /* Type                      */
        BYTE    model;                  /* Model                     */
        U32     bpg;                    /* Blocks per cyclical group */
        U32     bpp;                    /* Blocks per access position*/
        U32     size;                   /* Block size                */
        U32     blks;                   /* Number of blocks          */
        U16     cu;                     /* Default control unit type */
      } FBADEV;
#define FBADEV_SIZE sizeof(FBADEV)

#if defined(FEATURE_VM_BLOCKIO)
/*-------------------------------------------------------------------*/
/* Device Standard Block Size Information Table                      */
/*-------------------------------------------------------------------*/
typedef struct _BLKTAB {
        char  *name;         /* Device name                          */
        U16    devt;         /* Hercules supported device Type       */
        int    darch;        /* FBA (0) or CKD (1) device            */
#define VMDEVFBA 0           /* Fixed-Block Architecture device      */
#define VMDEVCKD 1           /* (Extended-)Count-Key-Data device     */
   /* sectors per block or blocks per track for standardblock sizes  */
        int    phys512;      /* Block size 512  */
        int    phys1024;     /* Block size 1024 */
        int    phys2048;     /* Block size 2048 */
        int    phys4096;     /* Block size 4096 */
    } BLKTAB;
#define BLKTAB_SIZE sizeof(BLKTAB)

/* Macros that define a table entry */
#define CKDIOT(_name,_type,_bs512,_bs1024,_bs2048,_bs4096) \
       { _name, _type, 1, _bs512, _bs1024, _bs2048, _bs4906 }
#define FBAIOT(_name,_type) \
       { _name, _type, 0, 1, 2, 4, 8 }
#endif /* defined(FEATURE_VM_BLOCKIO) */

/*-------------------------------------------------------------------*/
/* Request types for dasd_lookup                                     */
/*-------------------------------------------------------------------*/
#define DASD_CKDDEV 1                   /* Lookup CKD device         */
#define DASD_CKDCU  2                   /* Lookup CKD control unit   */
#define DASD_FBADEV 3                   /* Lookup FBA device         */
#if defined(FEATURE_VM_BLOCKIO)
#define DASD_STDBLK 4       /* Lookup device standard block/physical */
#endif /* defined(FEATURE_VM_BLOCKIO) */

/*-------------------------------------------------------------------*/
/* Dasd table function prototypes                                    */
/*-------------------------------------------------------------------*/
DTB_DLL_IMPORT void   *dasd_lookup (int, char *, U32   , U32   );
int     dasd_build_ckd_devid (CKDDEV *, CKDCU *, BYTE *);
int     dasd_build_ckd_devchar (CKDDEV *, CKDCU *, BYTE *, int);
DTB_DLL_IMPORT int     dasd_build_ckd_config_data (DEVBLK *, BYTE *, int);
DTB_DLL_IMPORT int     dasd_build_ckd_subsys_status (DEVBLK *, BYTE *, int);
int     dasd_build_fba_devid (FBADEV *, BYTE *);
int     dasd_build_fba_devchar (FBADEV *, BYTE *, int);

#endif // _DASDTAB_H
