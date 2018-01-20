/* CHSC.H       (C) Copyright Jan Jaeger, 1999-2012                  */
/*              Channel Subsystem interface fields                   */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/* This module implements channel subsystem interface functions      */
/* for the Hercules ESA/390 emulator.                                */
/*                                                                   */
/* This implementation is based on the S/390 Linux implementation    */

#ifndef _CHSC_H
#define _CHSC_H


#define _CHSC_AI(_array, _bitno) ((_array)[((_bitno)/32)][(((_bitno)%32)/8)])
#define _CHSC_BI(_bitno) (0x80 >> (((_bitno)%32)%8))
#define CHSC_SB(_array, _bitno)                                       \
  do {                                                                \
      _CHSC_AI((_array), (_bitno)) |= _CHSC_BI((_bitno));             \
  } while (0)


/*-------------------------------------------------------------------*/
/* Pack all structures to byte boundary...                           */
/*-------------------------------------------------------------------*/

#undef ATTRIBUTE_PACKED
#if defined(_MSVC_)
 #pragma pack(push)
 #pragma pack(1)
 #define ATTRIBUTE_PACKED
#else
 #define ATTRIBUTE_PACKED __attribute__((packed))
#endif


/*-------------------------------------------------------------------*/
/* CHSC_REQ: Generic Channel Subsystem Call request                  */
/*-------------------------------------------------------------------*/
struct _CHSC_REQ;
typedef struct _CHSC_REQ CHSC_REQ;
struct _CHSC_REQ {
/*000*/ HWORD   length;                 /* Offset to response field  */
/*002*/ HWORD   req;                    /* Request code              */
/* The following request codes are in use, but the request name      */
/* isn't known for the majority of the codes.                        */
// fine CHSC_REQ_RESETCU        0x0001  /* Reset Control Unit        */
#define CHSC_REQ_CHPDESC        0x0002  /* Store Channel-Path Description */
#define CHSC_REQ_SCHDESC        0x0004  /* Store Subchannel Description Data */
#define CHSC_REQ_CUDESC         0x0006  /* Store Subchannel Control-Unit Data */
//      CHSC_REQ_???????        0x000C  /* ???                       */
//      CHSC_REQ_???????        0x000E  /* ???                       */
#define CHSC_REQ_CSSINFO        0x0010  /* Store Channel-Subsystem Characteristics */
#define CHSC_REQ_CNFINFO        0x0012  /* Store Configuration Information */
//      CHSC_REQ_???????        0x0016  /* ???                       */
//      CHSC_REQ_???????        0x001C  /* ???                       */
// fine CHSC_REQ_CCHCOMP        0x001D  /* Compare Channel Components */
//      CHSC_REQ_???????        0x001E  /* ???                       */
#define CHSC_REQ_SETSSSI        0x0021  /* Set Subchannel Indicator  */
#define CHSC_REQ_GETSSQD        0x0024  /* Store Subchannel QDIO Data */
//      CHSC_REQ_???????        0x0026  /* ???                       */
//      CHSC_REQ_???????        0x002A  /* ???                       */
//      CHSC_REQ_???????        0x002B  /* ???                       */
//      CHSC_REQ_???????        0x0030  /* ???                       */
#define CHSC_REQ_ENFACIL        0x0031  /* Enable Facility           */
// fine CHSC_REQ_CNFCOMP        0x0032  /* Store Configuration Component */
//      CHSC_REQ_???????        0x0033  /* ???                       */
//      CHSC_REQ_???????        0x0034  /* ???                       */
//      CHSC_REQ_???????        0x0038  /* ???                       */
//      CHSC_REQ_???????        0x0046  /* ???                       */
//      CHSC_REQ_???????        0x1008  /* ???                       */
//      CHSC_REQ_???????        0x1027  /* ???                       */
//      CHSC_REQ_???????        0x102E  /* ???                       */
//      CHSC_REQ_???????        0x1036  /* ???                       */
// fine CHSC_REQ_EVENTIN        0x200E  /* Store Event Information   */
//      CHSC_REQ_???????        0x400F  /* ???                       */
// fine CHSC_REQ_CHGPATH        0x4011  /* Change Channel-Path Configuration */
// fine CHSC_REQ_CHGUNIT        0x4013  /* Change Control-Unit Configuration */
// fine CHSC_REQ_CHGDEVI        0x4015  /* Change I/O-Device Configuration */
//      CHSC_REQ_???????        0x4023  /* ???                       */
//      CHSC_REQ_???????        0x4025  /* ???                       */
//      CHSC_REQ_???????        0x4029  /* ???                       */
/* The following request names are in use, but the request code      */
/* isn't known for any of the names.                                 */
// fine CHSC_REQ_CHGMODE                /* Change Configuration Mode */
// fine CHSC_REQ_SDCLQUE                /* Store Shared-Device-Cluster Queues */
// fine CHSC_REQ_SUBPAIN                /* Store Subchannel Path Information */
// fine CHSC_REQ_CONCOLI                /* Store Configuration Component List */
// fine CHSC_REQ_DOMCOLI                /* Store Domain Component List */
// fine CHSC_REQ_DOMATTR                /* Set Domain Attributes */
// fine CHSC_REQ_DCOATLI                /* Store Domain Configuration Attributes List */
// fine CHSC_REQ_MEACHAT                /* Store Channel Measurement Attributes */
// fine CHSC_REQ_EXTCHME                /* Set Extended Channel Measurement */
/*004*/ FWORD   resv[3];
/*010*/ } ATTRIBUTE_PACKED;


/*-------------------------------------------------------------------*/
/* CHSC_RSP: Generic Channel Subsystem Call response                 */
/*-------------------------------------------------------------------*/
struct _CHSC_RSP;
typedef struct _CHSC_RSP CHSC_RSP;
struct _CHSC_RSP {
/*000*/ HWORD   length;                 /* Length of response field  */
/*002*/ HWORD   rsp;                    /* Reponse code              */
#define CHSC_REQ_OK             0x0001  /* No error                  */
#define CHSC_REQ_INVALID        0x0002  /* Invalid request           */
#define CHSC_REQ_ERRREQ         0x0003  /* Error in request block    */
#define CHSC_REQ_NOTSUPP        0x0004  /* Request not supported     */
#define CHSC_REQ_FACILITY       0x0101  /* Unknown Facility          */
                                        /* Response codes 0101-0111  */
                                        /* are returned by Change    */
                                        /* Configuration requests,   */
                                        /* the precise meaning is    */
                                        /* dependant on the request. */
/*004*/ FWORD   info;
/*008*/ } ATTRIBUTE_PACKED;


/*-------------------------------------------------------------------*/
/* PROGRAMMING NOTE: as far as we know the maximum size of a CHSC    */
/* instruction's request + response buffer can never exceed 0x1000   */
/* and the response buffer immediately follows the request itself.   */
/*-------------------------------------------------------------------*/
#define CHSC_REQRSP_SIZE    0x1000      /* Max reqest + response len */


/*-------------------------------------------------------------------*/
/* chsc_max_rsp: return maximum possible number of response entries  */
/*  req_len   =  request length FETCH_HW(req_len, chsc_req->length)  */
/*  rsp_size  =  size of one response entry, sizeof(CHSC_RSPxx)      */
/* returned value:  maximum possible response entries, or 0 if error */
/*-------------------------------------------------------------------*/
static INLINE U16 chsc_max_rsp( U16 req_len, size_t rsp_size )
{
register U16 max_rsp;
    if (0
        || rsp_size > (CHSC_REQRSP_SIZE - sizeof(CHSC_REQ) - sizeof(CHSC_RSP))
        || req_len  > (CHSC_REQRSP_SIZE - sizeof(CHSC_RSP) - rsp_size)
    )
        return 0;
    if (!rsp_size)
        max_rsp = (CHSC_REQRSP_SIZE - req_len) / sizeof(CHSC_RSP);
    else
        max_rsp = (CHSC_REQRSP_SIZE - req_len - sizeof(CHSC_RSP)) / rsp_size;
    return max_rsp;
}


/*-------------------------------------------------------------------*/
/* chsc_req_ok: respond CHSC_REQ_OK to good CHSC request             */
/*-------------------------------------------------------------------*/
static INLINE int chsc_req_ok( CHSC_RSP *chsc_rsp, U16 rsp_len, U32 info )
{
    STORE_HW( chsc_rsp->length, rsp_len);
    STORE_HW( chsc_rsp->rsp, CHSC_REQ_OK);
    STORE_FW( chsc_rsp->info, info);
    return 0;
}


/*-------------------------------------------------------------------*/
/* chsc_req_errreq: respond CHSC_REQ_ERRREQ to bad CHSC request      */
/*-------------------------------------------------------------------*/
static INLINE int chsc_req_errreq( CHSC_RSP *chsc_rsp, U32 info )
{
    STORE_HW( chsc_rsp->length, sizeof(CHSC_RSP));
    STORE_HW( chsc_rsp->rsp, CHSC_REQ_ERRREQ);
    STORE_FW( chsc_rsp->info, info);
    return 0;
}


/*-------------------------------------------------------------------*/
/* CHSC_REQ2: Store Channel Path Description request                 */
/*-------------------------------------------------------------------*/
struct _CHSC_REQ2;
typedef struct _CHSC_REQ2 CHSC_REQ2;
struct _CHSC_REQ2 {                     /* Store Channel Path Desc   */
/*000*/ HWORD   length;                 /* Offset to response field  */
/*002*/ HWORD   req;                    /* Request code              */
/*004*/ BYTE    flags1;
#define CHSC_REQ2_F1_M          0x20
#define CHSC_REQ2_F1_C          0x10
#define CHSC_REQ2_F1_FMT        0x0F
/*005*/ BYTE    cssid;
/*006*/ BYTE    rfmt;
#define CHSC_REQ2_RFMT_MSK      0x0F
/*007*/ BYTE    first_chpid;
/*008*/ BYTE    resv1[3];
/*00B*/ BYTE    last_chpid;
/*00C*/ FWORD   resv2;
/*010*/ } ATTRIBUTE_PACKED;


/*-------------------------------------------------------------------*/
/* CHSC_RSP2: Store Channel Path Description format 0 response       */
/*-------------------------------------------------------------------*/
/* CHSC_RSP followed by one or more CHSC_RSP2 */
struct _CHSC_RSP2;
typedef struct _CHSC_RSP2 CHSC_RSP2;
struct _CHSC_RSP2 {                     /* Store Channel Path Desc   */
/*000*/ BYTE    flags1;                 /* Flags                     */
#define CHSC_RSP2_F1_CHPID_VALID  0x80  /*                           */
#define CHSC_RSP2_F1_CHLA_VALID   0x40  /*                           */
#define CHSC_RSP2_F1_LSN_VALID    0x20  /*                           */
#define CHSC_RSP2_F1_SWLA_VALID   0x10  /*                           */
/*001*/ BYTE    lsn;                    /* Logical Switch Number     */
/*002*/ BYTE    chp_type;               /* Channel path description  */
/*003*/ BYTE    chpid;                  /* CHPID                     */
/*004*/ BYTE    swla;                   /* Switch Link Address       */
/*005*/ BYTE    resv5;
/*006*/ BYTE    chla;                   /* Channel Link Address      */
/*007*/ BYTE    resv7;
/*008*/ } ATTRIBUTE_PACKED;


/*-------------------------------------------------------------------*/
/* CHSC_RSP2F1: Store Channel Path Description format 1 response     */
/*-------------------------------------------------------------------*/
/* CHSC_RSP followed by one or more CHSC_RSP2F1 */
struct _CHSC_RSP2F1;
typedef struct _CHSC_RSP2F1 CHSC_RSP2F1;
struct _CHSC_RSP2F1 {                   /* Store Channel Path Desc   */
/*000*/ BYTE    flags1;                 /* Flags                     */
#define CHSC_RSP2F1_F1_CHPID_VALID 0x80 /*                           */
#define CHSC_RSP2F1_F1_LSN_VALID   0x20 /*                           */
/*001*/ BYTE    lsn;                    /* Logical Switch Number     */
/*002*/ BYTE    chp_type;               /* Channel path description  */
/*003*/ BYTE    chpid;                  /* CHPID                     */
/*004*/ BYTE    resv1[3];
/*007*/ BYTE    chpp;
/*008*/ FWORD   resv2[3];
/*00C*/ HWORD   mdc;
/*00E*/ HWORD   flags2;
#define CHSC_RSP2F1_F2_R        0x0004
#define CHSC_RSP2F1_F2_S        0x0002
#define CHSC_RSP2F1_F2_F        0x0001
/*010*/ FWORD   resv3[2];
/*018*/ } ATTRIBUTE_PACKED;


/*-------------------------------------------------------------------*/
/* CHSC_REQ4: Store Subchannel Description Data request              */
/*-------------------------------------------------------------------*/
struct _CHSC_REQ4;
typedef struct _CHSC_REQ4 CHSC_REQ4;
struct _CHSC_REQ4 {                     /* Store Subchann Descr Data */
/*000*/ HWORD   length;                 /* Offset to response field  */
/*002*/ HWORD   req;                    /* Request code              */
/*004*/ HWORD   ssidfmt;
#define CHSC_REQ4_SSID          0x0030
#define CHSC_REQ4_FMT           0x000f
/*006*/ HWORD   f_sch;                  /* First subchannel          */
/*008*/ HWORD   resv2;
/*00A*/ HWORD   l_sch;                  /* Last subchannel           */
/*00C*/ FWORD   resv3;
/*010*/ } ATTRIBUTE_PACKED;


/*-------------------------------------------------------------------*/
/* CHSC_RSP4: Store Subchannel Description Data response             */
/*-------------------------------------------------------------------*/
/* CHSC_RSP followed by zero, one or more CHSC_RSP4 */
struct _CHSC_RSP4;
typedef struct _CHSC_RSP4 CHSC_RSP4;
struct _CHSC_RSP4 {                     /* Store Subchann Descr Data */
/*000*/ BYTE    flags1;                 /* Flags                     */
#define CHSC_RSP4_F1_SCH_VALID    0x80  /* Subchannel valid          */
#define CHSC_RSP4_F1_DEV_VALID    0x40  /* Device number valid       */
#define CHSC_RSP4_F1_ST           0x38  /* Subchannel type           */
#define CHSC_RSP4_F1_ST_IO        0x00  /* I/O Subchannel; all fields
                                           have a meaning            */
#define CHSC_RSP4_F1_ST_CHSC      0x08  /* CHSC Subchannel only sch_val
                                           st and sch have a meaning */
#define CHSC_RSP4_F1_ST_MSG       0x10  /* MSG Subchannel; all fields
                                           except unit_addr have a
                                           meaning                   */
#define CHSC_RPS4_F1_ST_ADM       0x18  /* ADM Subchannel; Only sch_val
                                           st and sch have a meaning */
/*001*/ BYTE    unit_addr;              /* Unit address              */
/*002*/ HWORD   devno;                  /* Device number             */
/*004*/ BYTE    path_mask;              /* Valid path mask           */
/*005*/ BYTE    fla_valid_mask;         /* Valid link mask           */
/*006*/ HWORD   sch;                    /* Subchannel number         */
/*008*/ BYTE    chpid[8];               /* Channel path array        */
/*010*/ HWORD   fla[8];                 /* Full link address array   */
/*020*/ } ATTRIBUTE_PACKED;


/*-------------------------------------------------------------------*/
/* CHSC_REQ6: Store Subchannel Control-Unit Data request             */
/*-------------------------------------------------------------------*/
struct _CHSC_REQ6;
typedef struct _CHSC_REQ6 CHSC_REQ6;
struct _CHSC_REQ6 {                     /* Store Subchann. CU Data   */
/*000*/ HWORD   length;                 /* Offset to response field  */
/*002*/ HWORD   req;                    /* Request code              */
/*004*/ HWORD   ssidfmt;
#define CHSC_REQ6_M             0x2000
#define CHSC_REQ6_FMT           0x00f0
#define CHSC_REQ6_SSID          0x0003
/*006*/ HWORD   f_sch;                  /* First subchannel          */
/*008*/ BYTE    resv2;
/*009*/ BYTE    cssid;
/*00A*/ HWORD   l_sch;                  /* Last subchannel           */
/*00C*/ FWORD   resv3;
/*010*/ } ATTRIBUTE_PACKED;


/*-------------------------------------------------------------------*/
/* CHSC_RSP6: Store Subchannel Control-Unit Data response            */
/*-------------------------------------------------------------------*/
/* CHSC_RSP followed by zero, one or more CHSC_RSP6 */
struct _CHSC_RSP6;
typedef struct _CHSC_RSP6 CHSC_RSP6;
struct _CHSC_RSP6 {                     /* Store Subchann. CU Data   */
/*000*/ BYTE    flags1;                 /* Flags                     */
#define CHSC_RSP6_F1_SCH_VALID    0x80  /* Subchannel valid          */
#define CHSC_RSP6_F1_DEV_VALID    0x40  /* Device number valid       */
#define CHSC_RSP6_F1_ST           0x38  /* Subchannel type           */
#define CHSC_RSP6_F1_ST_IO        0x00  /* I/O Subchannel; all fields
                                           have a meaning            */
#define CHSC_RSP6_F1_ST_CHSC      0x08  /* CHSC Subchannel only sch_val
                                           st and sch have a meaning */
#define CHSC_RSP6_F1_ST_MSG       0x10  /* MSG Subchannel; all fields
                                           except unit_addr have a
                                           meaning                   */
#define CHSC_RPS6_F1_ST_ADM       0x18  /* ADM Subchannel; Only sch_val
                                           st and sch have a meaning */
/*001*/ BYTE    path_mask;              /* Path mask                 */
/*002*/ HWORD   devnum;                 /* Device number             */
/*004*/ HWORD   lcucb;                  /* LCUCB queue number        */
/*006*/ HWORD   sch;                    /* Subchannel number         */
/*007*/ BYTE    chpid[8];               /* Channel path array        */
/*010*/ HWORD   cun[8];                 /* Control unit number array */
/*020*/ } ATTRIBUTE_PACKED;


/*-------------------------------------------------------------------*/
/* CHSC_REQ10: Store Channel-Subsystem Characteristics request       */
/*-------------------------------------------------------------------*/
struct _CHSC_REQ10;
typedef struct _CHSC_REQ10 CHSC_REQ10;
struct _CHSC_REQ10 {                    /* Store Channel-Subsystem Characteristics */
/*000*/ HWORD   length;                 /* Offset to response field  */
/*002*/ HWORD   req;                    /* Request code              */
/*004*/ FWORD   resv[3];                /* Must be zero              */
/*010*/ } ATTRIBUTE_PACKED;


/*-------------------------------------------------------------------*/
/* CHSC_RSP10: Store Channel-Subsystem Characteristics reponse       */
/*-------------------------------------------------------------------*/
/* CHSC_RSP followed by CHSC_RSP10 */
struct _CHSC_RSP10;
typedef struct _CHSC_RSP10 CHSC_RSP10;
struct _CHSC_RSP10 {                    /* Store Channel-Subsystem Characteristics */
/*000*/ FWORD   general_char[510];
/*7F8*/ FWORD   chsc_char[508];         /* ZZ: Linux/390 code indicates
                                           this field has a length of
                                           518, however, that would
                                           mean that the entire CHSC
                                           request would be 4K + 16
                                           in length which is probably
                                           an error -    *JJ/10/10/04*/
/*FE8*/ } ATTRIBUTE_PACKED;


/*-------------------------------------------------------------------*/
/* CHSC_REQ12: Store Configuration Information request               */
/*-------------------------------------------------------------------*/
struct _CHSC_REQ12;
typedef struct _CHSC_REQ12 CHSC_REQ12;
struct _CHSC_REQ12 {                    /* Store Config. Information */
/*000*/ HWORD   length;                 /* Offset to response field  */
/*002*/ HWORD   req;                    /* Request code              */
/*004*/ BYTE    flags;
#define CHSC_REQ12_FLAGS_M      0x20
#define CHSC_REQ12_FLAGS_FMT    0x0F
/*005*/ BYTE    cssid;
/*006*/ BYTE    ssid;
#define CHSC_REQ12_SSID_MASK    0x03
/*007*/ BYTE    resv007;
/*008*/ DBLWRD  resv008;
/*010*/ } ATTRIBUTE_PACKED;


/*-------------------------------------------------------------------*/
/* CHSC_RSP12: Store Configuration Information response              */
/*-------------------------------------------------------------------*/
/* CHSC_RSP followed by CHSC_RSP12 */
struct _CHSC_RSP12;
typedef struct _CHSC_RSP12 CHSC_RSP12;
struct _CHSC_RSP12 {                     /* Store Config. Information */
/*000*/ BYTE    resv008;
/*001*/ BYTE    flags;                  /* Flags                     */
#define CHSC_RSP12_F1_CM   0x80         /* Configuration Mode        */
#define CHSC_RSP12_F1_CV   0x40         /* Configuration Valid       */
#define CHSC_RSP12_F1_CC   0x20         /* Configuration Changed     */
#define CHSC_RSP12_F1_TP   0x10         /* Token Present             */
#define CHSC_RSP12_F1_PPV  0x08         /* Program-Parameter Valid   */
/*002*/ BYTE    unknow00A;              /* ???                       */
/*003*/ BYTE    pnum;                   /* Partition Number          */
/*004*/ FWORD   pp[4];                  /* Program Parameter         */
/*014*/ FWORD   rse;                    /* Remaining Subchannel Elements */
/*018*/ FWORD   rcue;                   /* Remaining Control-Unit Elements */
/*01C*/ FWORD   rsce;                   /* Remaining Shared-Cluster Elements */
/*020*/ FWORD   unknow028;
/*024*/ FWORD   unknow02C;
/*028*/ FWORD   cct[16];                /* Current-Configuration Token */
/*068*/ FWORD   tct[16];                /* Target-Configuration Token */
/*0A8*/ FWORD   resv0B0;
/*0AC*/ HWORD   pnv;                    /* Partition-Names Valid mask */
/*0AE*/ HWORD   resv0B6;
/*0B0*/ DBLWRD  pn[16];                 /* Partition Names           */
/*130*/ FWORD   unknow138[488];
/*8D0*/ } ATTRIBUTE_PACKED;


/*-------------------------------------------------------------------*/
/* CHSC_REQ21: Set Subchannel Indicator.  Response is CHSC_RSP.      */
/*-------------------------------------------------------------------*/
struct _CHSC_REQ21;
typedef struct _CHSC_REQ21 CHSC_REQ21;
struct _CHSC_REQ21 {                    /* Set Subchannel Indicator  */
/*000*/ HWORD   length;                 /* Offset to response field  */
/*002*/ HWORD   req;                    /* Request code              */
/*004*/ HWORD   opcode;                 /* Operation Code            */
/*006*/ HWORD   resv1;
/*008*/ FWORD   resv2;
/*00C*/ FWORD   resv3;
/*010*/ DBLWRD  alsi;                   /* Adapter Summary Indicator */
/*018*/ DBLWRD  dsci;                   /* Device State Change Ind   */
/*020*/ BYTE    sk;                     /* Storage keys: alsi, dsci  */
#define CHSC_REQ21_KS         0xF0      /* Storage key alsi          */
#define CHSC_REQ21_KC         0x0F      /* Storage key dsci          */
/*021*/ BYTE    resv4[2];
/*023*/ BYTE    isc;                    /* Interrupt SubClass        */
#define CHSC_REQ21_VISC_MASK  0x70
#define CHSC_REQ21_ISC_MASK   0x07
/*024*/ FWORD   flags;                  /* Flags                     */
#define CHSC_REQ21_FLAGS_D 0x10000000   /* Time Delay Enablement     */
/*028*/ FWORD   resv5;
/*02C*/ FWORD   ssid;                   /* Subchannel Id             */
/*030*/ FWORD   resvx[1004];
/*FE0*/ } ATTRIBUTE_PACKED;


/*-------------------------------------------------------------------*/
/* CHSC_REQ24: Store Subchannel QDIO Data request                    */
/*-------------------------------------------------------------------*/
struct _CHSC_REQ24;
typedef struct _CHSC_REQ24 CHSC_REQ24;
struct _CHSC_REQ24 {                    /* Store Subchann. QDIO Data */
/*000*/ HWORD   length;                 /* Offset to response field  */
/*002*/ HWORD   req;                    /* Request code              */
/*004*/ HWORD   ssidfmt;
#define CHSC_REQ24_SSID         0x0030
#define CHSC_REQ24_FMT          0x000f
/*006*/ HWORD   f_sch;                  /* First subchannel number   */
/*008*/ HWORD   resv1;
/*00A*/ HWORD   l_sch;                  /* Last subchannel number    */
/*00C*/ FWORD   resv2;
/*010*/ } ATTRIBUTE_PACKED;


/*-------------------------------------------------------------------*/
/* CHSC_RSP24: Store Subchannel QDIO Data response                   */
/*-------------------------------------------------------------------*/
/* CHSC_RSP followed by zero, one or more CHSC_RSP24 */
struct _CHSC_RSP24;
typedef struct _CHSC_RSP24 CHSC_RSP24;
struct _CHSC_RSP24 {                    /* Store Subchann. QDIO Data */
/*000*/ BYTE flags;                     /* Flags for st QDIO Sch Data */
#define CHSC_FLAG_QDIO_CAPABILITY       0x80
#define CHSC_FLAG_VALIDITY              0x40
#define CHSC_FLAG_UNKNOWN20             0x20    /* ??? */
/*001*/ BYTE    resv1;
/*002*/ HWORD   sch;
/*004*/ BYTE    qfmt;
/*005*/ BYTE    parm;
/*006*/ BYTE    qdioac1;                        /* QDIO adapter-characteristics-1 flag */
#define AC1_UNKNOWN80                   0x80    /* ??? */
#define AC1_SIGA_INPUT_NEEDED           0x40    /* Process input queues */
#define AC1_SIGA_OUTPUT_NEEDED          0x20    /* Process output queues */
#define AC1_SIGA_SYNC_NEEDED            0x10    /* Ask hypervisor to sync */
#define AC1_AUTOMATIC_SYNC_ON_THININT   0x08    /* Set by hypervisor */
#define AC1_AUTOMATIC_SYNC_ON_OUT_PCI   0x04    /* Set by hypervisor */
#define AC1_SC_QEBSM_AVAILABLE          0x02    /* Available for subchannel */
#define AC1_SC_QEBSM_ENABLED            0x01    /* Enabled for subchannel */
/*007*/ BYTE    sch_class;
/*008*/ BYTE    pcnt;
/*009*/ BYTE    icnt;
/*00A*/ BYTE    resv2;
/*00B*/ BYTE    ocnt;
/*00C*/ BYTE    resv3;
/*00D*/ BYTE    mbccnt;
/*00E*/ HWORD   qdioac2;                        /* QDIO adapter-characteristics-2 flag */
#define QETH_AC2_UNKNOWN4000            0x4000  /* ??? */
#define QETH_AC2_UNKNOWN2000            0x2000  /* ??? */
#define QETH_SNIFF_AVAIL                0x0008  /* Promisc mode avail */
#define QETH_AC2_DATA_DIV_AVAILABLE     0x0010
#define QETH_AC2_DATA_DIV_ENABLED       0x0002
/*010*/ DBLWRD  sch_token;
/*018*/ BYTE    mro;
/*019*/ BYTE    mri;
/*01A*/ HWORD   qdioac3;
#define QETH_AC3_FORMAT2_CQ_AVAILABLE   0x8000
/*01C*/ HWORD   resv5;
/*01E*/ BYTE    resv6;
/*01F*/ BYTE    mmwc;
/*020*/ } ATTRIBUTE_PACKED;


/*-------------------------------------------------------------------*/
/* CHSC_REQ31: Enable Facility request. The response is a CHSC_RSP.  */
/*-------------------------------------------------------------------*/
struct _CHSC_REQ31;
typedef struct _CHSC_REQ31 CHSC_REQ31;
struct _CHSC_REQ31 {                    /* Enable Facility           */
/*000*/ HWORD   length;                 /* Offset to response field  */
/*002*/ HWORD   req;                    /* Request code              */
/*004*/ HWORD   resv1;
/*006*/ HWORD   facility;               /* Operation Code            */
#define CHSC_REQ31_MSS          0x0002  /* MSS Facility              */
/*008*/ FWORD   resv2;
/*00C*/ FWORD   resv3;
/*010*/ FWORD   opdata[252];            /* Operation Code Data       */
/*400*/ } ATTRIBUTE_PACKED;


/*-------------------------------------------------------------------*/
/* Some requests that aren't supported yet.                          */
/*-------------------------------------------------------------------*/

/* Reset Control Unit request. The response is a CHSC_RSP            */
struct _CHSC_REQ1;
typedef struct _CHSC_REQ1  CHSC_REQ1;
struct _CHSC_REQ1 {                     /* Reset Control Unit        */
/*000*/ HWORD   length;                 /* Offset to response field  */
/*002*/ HWORD   req;                    /* Request code              */
/*004*/ BYTE    resv1[3];
/*007*/ BYTE    chpid;
/*008*/ HWORD   resv2;
/*00A*/ HWORD   sch;                    /* Subchannel number         */
/*00C*/ FWORD   resv3;
/*010*/ } ATTRIBUTE_PACKED;

struct _CHSC_REQC;
typedef struct _CHSC_REQC  CHSC_REQC;
struct _CHSC_REQC  {
/*000*/ HWORD   length;                 /* Offset to response field  */
/*002*/ HWORD   req;                    /* Request code              */
/*004*/ FWORD   unknown4[3];
/*010*/ } ATTRIBUTE_PACKED;

struct _CHSC_REQ26;
typedef struct _CHSC_REQ26 CHSC_REQ26;
struct _CHSC_REQ26 {
/*000*/ HWORD   length;                 /* Offset to response field  */
/*002*/ HWORD   req;                    /* Request code              */
/*004*/ HWORD   unknown4;
/*006*/ HWORD   first_cun;              /* First control unit number */
/*008*/ HWORD   unknown8;
/*00A*/ HWORD   last_cun;               /* Last control unit number  */
/*00C*/ FWORD   unknownc;
/*010*/ } ATTRIBUTE_PACKED;

struct _CHSC_REQ30;
typedef struct _CHSC_REQ30 CHSC_REQ30;
struct _CHSC_REQ30 {
/*000*/ HWORD   length;                 /* Offset to response field  */
/*002*/ HWORD   req;                    /* Request code              */
/*004*/ BYTE    unknown4;
/*005*/ BYTE    unknown5[3];
/*008*/ FWORD   unknown8[6];
/*020*/ } ATTRIBUTE_PACKED;

struct _CHSC_REQ32;
typedef struct _CHSC_REQ32 CHSC_REQ32;
struct _CHSC_REQ32 {                    /* Store Configur. Component */
/*000*/ HWORD   length;                 /* Offset to response field  */
/*002*/ HWORD   req;                    /* Request code              */
/*004*/ BYTE    ctype;
/*005*/ BYTE    unknown5[3];
/*008*/ FWORD   unknown8[6];
/*020*/ } ATTRIBUTE_PACKED;



/*-------------------------------------------------------------------*/
/* Channel Path Type definitions                                     */
/*-------------------------------------------------------------------*/
#define CHP_TYPE_UNDEF 0x00  /* Unknown                              */
#define CHP_TYPE_BLOCK 0x01  /* Parallel Block Multiplex             */
#define CHP_TYPE_BYTE  0x02  /* Parallel Byte Multiplex              */
#define CHP_TYPE_CNC_P 0x03  /* ESCON Point To Point                 */
#define CHP_TYPE_CNC_U 0x04  /* ESCON Switched or Point To Point     */
#define CHP_TYPE_CNC_S 0x05  /* ESCON Switched Point To Point        */
#define CHP_TYPE_CVC   0x06  /* ESCON Path to a Block Converter      */
#define CHP_TYPE_NTV   0x07  /* Native Interface                     */
#define CHP_TYPE_CTC_P 0x08  /* CTC Point To Point                   */
#define CHP_TYPE_CTC_S 0x09  /* CTC Switched Point To Point          */
#define CHP_TYPE_CTC_U 0x0A  /* CTC Switched or Point To Point       */
#define CHP_TYPE_CFS   0x0B  /* Coupling Facility Sender             */
#define CHP_TYPE_CFR   0x0C  /* Coupling Facility Receiver           */
#define CHP_TYPE_CBY   0x0F  /* ESCON Path to a Byte Converter       */
#define CHP_TYPE_OSE   0x10  /* OSA Express                          */
#define CHP_TYPE_OSD   0x11  /* OSA Direct Express                   */
#define CHP_TYPE_OSA   0x12  /* Open Systems Adapter                 */
#define CHP_TYPE_ISD   0x13  /* Internal System Device               */
#define CHP_TYPE_OSC   0x14  /* OSA Console                          */
#define CHP_TYPE_OSN   0x15  /* OSA NCP                              */
#define CHP_TYPE_CBS   0x16  /* Cluster Bus Sender                   */
#define CHP_TYPE_CBR   0x17  /* Cluster Bus Receiver                 */
#define CHP_TYPE_ICS   0x18  /* Internal Coupling Sender             */
#define CHP_TYPE_ICR   0x19  /* Internal Coupling Receiver           */
#define CHP_TYPE_FC    0x1A  /* FICON Point To Point                 */
#define CHP_TYPE_FC_S  0x1B  /* FICON Switched                       */
#define CHP_TYPE_FCV   0x1C  /* FICON To ESCON Bridge                */
#define CHP_TYPE_FC_U  0x1D  /* FICON Incomplete                     */
#define CHP_TYPE_DSD   0x1E  /* Direct System Device                 */
#define CHP_TYPE_EIO   0x1F  /* Emulated I/O                         */
#define CHP_TYPE_CBP   0x21  /* Integrated Cluster Bus Peer          */
#define CHP_TYPE_CFP   0x22  /* Coupling Facility Peer               */
#define CHP_TYPE_ICP   0x23  /* Internal Coupling Peer               */
#define CHP_TYPE_IQD   0x24  /* Internal Queued Direct Comm          */
#define CHP_TYPE_FCP   0x25  /* FCP Channel                          */
#define CHP_TYPE_CIB   0x26  /* Coupling over Infiniband             */


#if defined(_MSVC_)
 #pragma pack(pop)
#endif

#endif /* _CHSC_H */
