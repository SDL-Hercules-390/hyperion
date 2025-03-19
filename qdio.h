/* QDIO.H       (C) Copyright Jan Jaeger,   2010-2012                */
/*              Queued Direct I/O                                    */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/* This implementation is based on the S/390 Linux implementation    */

#ifndef _QDIO_H
#define _QDIO_H

#include "hercules.h"

/*-------------------------------------------------------------------*/
/*           Optional debug trace callback function                  */
/*-------------------------------------------------------------------*/
typedef void QDIOTRC( DEVBLK* dev, char* fmt, ... );

/*-------------------------------------------------------------------*/
/* OSA Device Structure                                              */
/*-------------------------------------------------------------------*/
typedef struct _QDIO_DEV {
    unsigned rxcnt;             /* Packets read                      */
    unsigned txcnt;             /* Packets written                   */
    unsigned dropcnt;           /* Packets dropped                   */

    int     idxstate;           /* IDX state                         */
#define MPC_IDX_STATE_INACTIVE  0x00
#define MPC_IDX_STATE_ACTIVE    0x01
#define MPC_IDX_STATE_HALTING   0x02

    int     acqstate;           /* ACTIVATE QUEUES state             */
#define ACQ_STATE_INACTIVE  0x00
#define ACQ_STATE_ACTIVE    0x01

    int     thinint;            /* Thin Interrupts on PCI            */

    int   i_qcnt;               /* Input Queue Count                 */
    int   i_qpos;               /*   Current Queue Position          */
    int   i_bpos[QDIO_MAXQ];    /*     Current Buffer Position       */

    int   o_qcnt;               /* Output Queue Count                */
    int   o_qpos;               /*   Current Queue Position          */
    int   o_bpos[QDIO_MAXQ];    /*     Current Buffer Position       */

    U32   i_qmask;              /* Input Queue Mask                  */
    U32   o_qmask;              /* Output Queue Mask                 */

    BYTE  i_slibk[QDIO_MAXQ];   /* Input SLIB Key                    */
    BYTE  i_slk[QDIO_MAXQ];     /* Input Storage List Key            */
    BYTE  i_sbalk[QDIO_MAXQ];   /* Input SBAL Key                    */
    BYTE  i_slsblk[QDIO_MAXQ];  /* Input SLSB Key                    */

    U64   i_sliba[QDIO_MAXQ];   /* Input SLIB Address                */
    U64   i_sla[QDIO_MAXQ];     /* Input Storage List Address        */
    U64   i_slsbla[QDIO_MAXQ];  /* Input SLSB Address                */

    BYTE  o_slibk[QDIO_MAXQ];   /* Output SLIB Key                   */
    BYTE  o_slk[QDIO_MAXQ];     /* Output Storage List Key           */
    BYTE  o_sbalk[QDIO_MAXQ];   /* Output SBAL Key                   */
    BYTE  o_slsblk[QDIO_MAXQ];  /* Output SLSB Key                   */

    U64   o_sliba[QDIO_MAXQ];   /* Output SLIB Address               */
    U64   o_sla[QDIO_MAXQ];     /* Output Storage List Address       */
    U64   o_slsbla[QDIO_MAXQ];  /* Output SLSB Address               */

    BYTE  qibk;                 /* Queue Information Block Key       */
    U64   qiba;                 /* Queue Information Block Address   */

    U64   alsi;                 /* Adapter Local Summary Indicator   */
#define ALSI_ERROR      0x80    /* ZZ TO BE CONFIRMED                */
    U64   dsci;                 /* Device status change Indicator    */
#define DSCI_IOCOMP     0x01    /* ZZ TO BE CONFIRMED                */
    BYTE  ks;                   /* alsi storage key                  */
    BYTE  kc;                   /* dsci storage key                  */
    } QDIO_DEV;


/*-------------------------------------------------------------------*/
/* Queue Descriptor Entry (Format 0)                                 */
/*-------------------------------------------------------------------*/
typedef struct _QDIO_QDES0 {
/*000*/ DBLWRD  sliba;          /* Storage List Info Block Address   */
/*008*/ DBLWRD  sla;            /* Storage List Address              */
/*010*/ DBLWRD  slsba;          /* Storage List State Block Address  */
/*018*/ FWORD   resv018;
/*01C*/ BYTE    keyp1;          /* Access keys for SLIB and SL       */
#define QDES_KEYP1_A_SLIB 0xF0
#define QDES_KEYP1_A_SL   0x0F
/*01D*/ BYTE    keyp2;          /* Access keys for SBALs ad SLSB     */
#define QDES_KEYP2_A_SBAL 0xF0
#define QDES_KEYP2_A_SLSB 0x0F
/*01E*/ HWORD   resv01e;
    } QDIO_QDES0;


/*-------------------------------------------------------------------*/
/* Queue Descriptor Record (QDR)                                     */
/*-------------------------------------------------------------------*/
typedef struct _QDIO_QDR {
/*000*/ BYTE    qfmt;           /* Queue Format                      */
/*001*/ BYTE    pfmt;           /* Model Dependent Parameter Format  */
/*002*/ BYTE    resv002;
/*003*/ BYTE    ac;             /* Adapter Characteristics           */
/*004*/ BYTE    resv004;
/*005*/ BYTE    iqdcnt;         /* Input Queue Descriptor Count      */
/*006*/ BYTE    resv006;
/*007*/ BYTE    oqdcnt;         /* Output Queue Descriptor Count     */
/*008*/ BYTE    resv008;
/*009*/ BYTE    iqdsz;          /* Input Queue Descriptor Size       */
/*00A*/ BYTE    resv00a;
/*00B*/ BYTE    oqdsz;          /* Output Queue Descriptor Size      */
/*00C*/ FWORD   resv00c[9];
/*030*/ DBLWRD  qiba;           /* Queue Information Block Address   */
/*038*/ FWORD   resv038;
/*03C*/ BYTE    qkey;           /* Queue Information Block Key       */
/*03D*/ BYTE    resv03d[3];
/*040*/ QDIO_QDES0 qdf0[126];   /* Format 0 Queue Descriptors        */
    } QDIO_QDR;


/*-------------------------------------------------------------------*/
/* Queue Information Block (QIB)                                     */
/*-------------------------------------------------------------------*/
typedef struct _QDIO_QIB {
/*000*/ BYTE    qfmt;           /* Queue Format                      */
/*001*/ BYTE    pfmt;           /* Parameter Format                  */
#define QID_PFMT_QETH   0x00
/*002*/ BYTE    rflags;         /* Flags                             */
#define QIB_RFLAGS_QEBSM  0x80
/*003*/ BYTE    ac;             /* Adapter Characteristics           */
#define QIB_AC_PCI      0x40
/*004*/ FWORD   resv004;
/*008*/ DBLWRD  isliba;         /* Input SLIB queue address          */
/*010*/ DBLWRD  osliba;         /* Output SLIB queue address         */
/*018*/ FWORD   resv018;
/*01C*/ FWORD   resv01c;
/*020*/ BYTE    ebcnam[8];      /* Adapter ID in EBCDIC              */
/*028*/ BYTE    resv028[88];
/*080*/ BYTE    parm[128];      /* Model Dependent Parameters        */
    } QDIO_QIB;

/*-------------------------------------------------------------------*/
/* Maximum number of Storage array entries                           */
/*-------------------------------------------------------------------*/
#define QMAXBUFS   128          /* Maximum Number of Buffers         */
#define QMAXSTBK    16          /* Maximum Storage block entries     */

/*-------------------------------------------------------------------*/
/* Storage List Information Block (SLIB)                             */
/*-------------------------------------------------------------------*/
typedef struct _QDIO_SLIB {
/*000*/ DBLWRD  nsliba;         /* Next SLIB in queue                */
/*008*/ DBLWRD  sla;            /* Storage List Address              */
/*010*/ DBLWRD  slsba;          /* Storage List State Block Address  */
/*018*/ BYTE    resv018[1000];
/*400*/ DBLWRD  slibe[QMAXBUFS];/* Storage List Info. Block Entries  */
    } QDIO_SLIB;


/*-------------------------------------------------------------------*/
/* Storage List (SL)                                                 */
/*-------------------------------------------------------------------*/
typedef struct _QDIO_SL {
/*000*/ DBLWRD  sbala[QMAXBUFS];/* Storage Block Address List address*/
    } QDIO_SL;


/*-------------------------------------------------------------------*/
/* Storage Block Address List Entry (SBALE)                          */
/*-------------------------------------------------------------------*/
typedef struct _QDIO_SBALE {
/*000*/ BYTE    flags[4];       /* Flags                             */
#define SBALE_FLAG0_LAST_ENTRY  0x40
#define SBALE_FLAG0_CONTIGUOUS  0x20
#define SBALE_FLAG0_FRAG_MASK   0x0C
#define SBALE_FLAG0_FRAG_FIRST  0x04
#define SBALE_FLAG0_FRAG_MIDDLE 0x08
#define SBALE_FLAG0_FRAG_LAST   0x0C
#define SBALE_FLAG3_PCI_REQ     0x40
/*004*/ FWORD   length;         /* Storage length                    */
/*008*/ DBLWRD  addr;           /* Storage Address                   */
    } QDIO_SBALE;


/*-------------------------------------------------------------------*/
/* Storage Block Address List (SBAL)                                 */
/*-------------------------------------------------------------------*/
typedef struct _QDIO_SBAL {
/*000*/ QDIO_SBALE sbale[QMAXSTBK];/* Storage Block Addr. List entry */
    } QDIO_SBAL;


/*-------------------------------------------------------------------*/
/* Storage List State Block (SLSB)                                   */
/*-------------------------------------------------------------------*/
typedef struct _QDIO_SLSB {
/*000*/ BYTE    slsbe[QMAXBUFS];/* Storage Block Address List entry  */
#define SLSBE_OWNER             0xC0 /* Owner Mask                   */
#define SLSBE_OWNER_OS          0x80 /* Control Program is owner     */
#define SLSBE_OWNER_CU          0x40 /* Control Unit is owner        */
#define SLSBE_TYPE              0x20 /* Buffer type mask             */
#define SLSBE_TYPE_INPUT        0x00 /* Input Buffer                 */
#define SLSBE_TYPE_OUTPUT       0x20 /* Output Buffer                */
#define SLSBE_VALID             0x10 /* Buffer Valid                 */
#define SLSBE_STATE             0x0F /* Buffer state mask            */
#define SLSBE_STATE_NOTINIT     0x00 /* Not initialised              */
#define SLSBE_STATE_EMPTY       0x01 /* Buffer empty                 */
#define SLSBE_STATE_PRIMED      0x02 /* Buffer ready                 */
#define SLSBE_STATE_PENDING     0x03 /* Buffer pending               */
#define SLSBE_STATE_HALTED      0x0E /* I/O halted                   */
#define SLSBE_STATE_ERROR       0x0F /* I/O Error                    */
#define SLSBE_ERROR             0xFF /* Addressing Error             */

#define SLSBE_OUTPUT_PRIMED     ( 0 \
                                | SLSBE_OWNER_CU                    \
                                | SLSBE_TYPE_OUTPUT                 \
                                | SLSBE_STATE_PRIMED                \
                                )
#define SLSBE_OUTPUT_COMPLETED  ( 0 \
                                | SLSBE_OWNER_OS                    \
                                | SLSBE_TYPE_OUTPUT                 \
                                | SLSBE_STATE_EMPTY                 \
                                )
#define SLSBE_INPUT_EMPTY       ( 0 \
                                | SLSBE_OWNER_CU                    \
                                | SLSBE_TYPE_INPUT                  \
                                | SLSBE_STATE_EMPTY                 \
                                )
#define SLSBE_INPUT_ACKED       ( 0 \
                                | SLSBE_OWNER_OS                    \
                                | SLSBE_TYPE_INPUT                  \
                                | SLSBE_STATE_EMPTY                 \
                                )
#define SLSBE_INPUT_COMPLETED   ( 0 \
                                | SLSBE_OWNER_OS                    \
                                | SLSBE_TYPE_INPUT                  \
                                | SLSBE_STATE_PRIMED                \
                                )
    } QDIO_SLSB;


#endif /* _QDIO_H */
