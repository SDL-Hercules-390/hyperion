/* MPC.H        (C) Copyright Jan Jaeger,  2010-2012                 */
/*              (C) Copyright Ian Shorter, 2011-2012                 */
/*              (C) Copyright Harold Grovesteen, 2011-2012           */
/*              (C) and others 2013-2021                             */
/*              MPC (Multi-Path Channel) functions                   */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/* This implementation is based on the S/390 Linux implementation    */


#if !defined(_MPC_H)
#define _MPC_H

/*********************************************************************/
/* Structures                                                        */
/*********************************************************************/

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
/* Constants                                                         */
/*-------------------------------------------------------------------*/
#define MPC_TOKEN_X5       0x05
#define MPC_TOKEN_LENGTH   4

#define PROTOCOL_02        0x02
#define PROTOCOL_LAYER2    0x08
#define PROTOCOL_LAYER3    0x03
#define PROTOCOL_NCP       0x0A
#define PROTOCOL_UNKNOWN   0x7E

#define FROM_GUEST         '<'
#define TO_GUEST           '>'
#define NO_DIRECTION       ' '

/*===================================================================*/
/* Structures used by multiple devices                               */
/*===================================================================*/

/*-------------------------------------------------------------------*/
/* Transport Header (See Note 1)                                     */
/*-------------------------------------------------------------------*/
struct _MPC_TH                     /* Transport Header               */
{                                  /*                                */
/*000*/  FWORD  first4;            /* The organisation of the first  */
                                   /* 4-bytes isn't clear. The only  */
                                   /* contents that have been seen   */
                                   /* are 0x00E00000.                */
#define MPC_TH_FIRST4  0x00E00000  /*                                */
/*004*/  FWORD  seqnum;            /* Sequence number.               */
/*008*/  FWORD  offrrh;            /* Offset from the start of the   */
                                   /* MPC_TH to the first (or only)  */
                                   /* MPC_RRH.                       */
                                   /* (Or, if you prefer, the        */
                                   /* length of the MPC_TH.)         */
/*00C*/  FWORD  length;            /* Total length of the MPC_TH     */
                                   /* and the data transported with  */
                                   /* the MPC_TH. (See Note 2)       */
/*010*/  HWORD  unknown10;         /* This field always seems to     */
                                   /* contain 0x1000 for OSA, or     */
                                   /* 0x0FFC for PTP. Why?           */
/*012*/  HWORD  numrrh;            /* The number of MPC_RRHs         */
                                   /* transported with the MPC_TH.   */
/*014*/                            /* End of the MPC_TH.             */
                                   /* (Or, if you prefer, the start  */
                                   /* of the following MPC_RRH.)     */
} ATTRIBUTE_PACKED;                /*                                */
#define SIZE_TH  0x0014            /* Size of MPC_TH.                */
typedef struct _MPC_TH MPC_TH, *PMPC_TH;

/*-------------------------------------------------------------------*/
/* Request/Response Header (See Note 1)                              */
/*-------------------------------------------------------------------*/
struct _MPC_RRH                    /* Request/Response Header        */
{                                  /*                                */
/*000*/  FWORD  offrrh;            /* Offset from the start of the   */
                                   /* MPC_TH to the next MPC_RRH.    */
                                   /* Contains zero for the last     */
                                   /* (or only) MPC_RRH.             */
/*004*/  BYTE   type;              /* Type (See Note 3)              */
#define RRH_TYPE_CM     0x81       /*                                */
#define RRH_TYPE_ULP    0x41       /*                                */
#define RRH_TYPE_IPA    0xC1       /*                                */
/*005*/  BYTE   proto;             /* Protocol (See Note 3)          */
/*006*/  HWORD  numph;             /* The number of MPC_PHs          */
                                   /* following the MPC_RRH.         */
/*008*/  FWORD  seqnum;            /* Sequence number.               */
/*00C*/  FWORD  ackseq;            /* Ack sequence number.           */
/*010*/  HWORD  offph;             /* Offset from the start of       */
                                   /* the MPC_RRH to the first       */
                                   /* (or only) MPC_PH.              */
                                   /* (Or, if you prefer, the        */
                                   /* length of the MPC_RRH).        */
/*012*/  HWORD  lenfida;           /* Length of the data             */
                                   /* referenced by the first        */
                                   /* MPC_PH. (See Note 2)           */
/*014*/  BYTE   lenalda[3];        /* Length of the data             */
                                   /* referenced by the MPC_PHs.     */
                                   /* Note: a 3-byte length field.   */
/*017*/  BYTE   tokenx5;           /* Token length or type or ...    */
/*018*/  FWORD  token;             /* Token.                         */
/*01C*/                            /* End of short MPC_RRH.          */
/*01C*/  BYTE   unknown1C;         /* ???.                           */
/*01D*/  BYTE   unknown1D[3];      /* ???.                           */
/*020*/  FWORD  unknown20;         /* ???. Contains zero in the      */
                                   /* first MPC_RRH, or a number     */
                                   /* in the second MPC_RRH.         */
/*024*/                            /* End of the MPC_RRH.            */
                                   /* (Or, if you prefer, the start  */
                                   /* of the following MPC_PH.)      */
} ATTRIBUTE_PACKED;                /*                                */
#define SIZE_RRH 0x0024            /* Size of MPC_RRH.               */
#define SIZE_RRH_1 0x001C          /* Size of short MPC_RRH.         */
typedef struct _MPC_RRH MPC_RRH, *PMPC_RRH;

/*-------------------------------------------------------------------*/
/* Protocol Data Unit Header                                         */
/*-------------------------------------------------------------------*/
struct _MPC_PH                     /* Protocol Data Unit Header      */
{                                  /*                                */
/*000*/  BYTE   locdata;           /* The location of the data       */
                                   /* referenced by this MPC_PH.     */
                                   /* (See Note 2)                   */
#define PH_LOC_1  1                /*                                */
#define PH_LOC_2  2                /*                                */
/*001*/  BYTE   lendata[3];        /* Length of the data             */
                                   /* referenced by this MPC_PH.     */
                                   /* Note: a 3-byte length field.   */
/*004*/  FWORD  offdata;           /* Offset from the start of the   */
                                   /* MPC_TH to the data referenced  */
                                   /* by this MPC_PH.                */
/*008*/                            /* End of the MPC_PH.             */
                                   /* (Or, if you prefer, the start  */
                                   /* of the following MPC_PH, or    */
                                   /* data, or whatever.)            */
} ATTRIBUTE_PACKED;                /*                                */
#define SIZE_PH  0x0008            /* Size of MPC_PH.                */
typedef struct _MPC_PH MPC_PH, *PMPC_PH;


/*-------------------------------------------------------------------*/
/* Protocol Data Unit                                                */
/*-------------------------------------------------------------------*/
/* For PTP devices:-                                                 */
/* - when MPC_RRH->type contains 0x81 and                            */
/*        MPC_RRH->proto contains 0x08 (0x8108)                      */
/*   the PDU is one or more IP packets.                              */
/* - when MPC_RRH->type contains 0xC1 and                            */
/*        MPC_RRH->proto contains 0x08 (0xC108)                      */
/*   the PDU is an MPC_PIX                                           */
/* - when MPC_RRH->type contains 0x41 or 0xC1 and                    */
/*        MPC_RRH->proto contains 0x7E (0x417E or 0xC17E)            */
/*   the PDU is an MPC_PUK with one or more MPC_PUS.                 */


/*-------------------------------------------------------------------*/
/* PDU Unknown                                                       */
/*-------------------------------------------------------------------*/
struct _MPC_PUK                    /*                                */
{                                  /*                                */
/*000*/  HWORD  length;            /* Length of the MPC_PUK.         */
/*002*/  BYTE   what;              /* What (See Note 4)              */
#define PUK_WHAT_41  0x41          /*                                */
#define PUK_WHAT_43  0x43          /*                                */
#define PUK_WHAT_45  0x45          /*                                */
/*003*/  BYTE   type;              /* Type (See Note 4)              */
#define PUK_TYPE_01        0x01    /*                                */
#define PUK_TYPE_ENABLE    0x02    /*                                */
#define PUK_TYPE_DISABLE   0x03    /*                                */
#define PUK_TYPE_SETUP     0x04    /*                                */
#define PUK_TYPE_TAKEDOWN  0x05    /*                                */
#define PUK_TYPE_CONFIRM   0x06    /*                                */
#define PUK_TYPE_ACTIVE    0x60    /*                                */
/*004*/  HWORD  lenpus;            /* Total length of the MPC_PUSs   */
                                   /* following the MPC_PUK.         */
/*006*/  BYTE   unknown06;         /* ???, only ever seen null.      */
/*007*/  BYTE   unknown07[5];      /* ???, only ever seen nulls.     */
/*00C*/                            /* End of the MPC_PUK.            */
} ATTRIBUTE_PACKED;                /*                                */
#define SIZE_PUK  0x000C           /* Size of MPC_PUK                */
typedef struct _MPC_PUK MPC_PUK, *PMPC_PUK;

/*-------------------------------------------------------------------*/
/* PDU Unknown structured fields                                     */
/*-------------------------------------------------------------------*/
struct _MPC_PUS                    /*                                */
{                                  /*                                */
/*000*/  HWORD  length;            /* Length of the MPC_PUS.         */
/*002*/  BYTE   what;              /* What (See Note 5)              */
#define PUS_WHAT_04  0x04          /*                                */
/*003*/  BYTE   type;              /* Type (See Note 5)              */
#define PUS_TYPE_01  0x01          /*                                */
#define PUS_TYPE_02  0x02          /*                                */
#define PUS_TYPE_03  0x03          /*                                */
#define PUS_TYPE_04  0x04          /*                                */
#define PUS_TYPE_05  0x05          /*                                */
#define PUS_TYPE_06  0x06          /*                                */
#define PUS_TYPE_07  0x07          /*                                */
#define PUS_TYPE_08  0x08          /*                                */
#define PUS_TYPE_09  0x09          /*                                */
#define PUS_TYPE_0A  0x0A          /*                                */
#define PUS_TYPE_0B  0x0B          /*                                */
#define PUS_TYPE_0C  0x0C          /*                                */
/*004*/  union _vc                 /* Variable contents start        */
    {

         struct _pus_01 {          /* PUS_01 contents                */
/*004*/    BYTE   proto;           /* Protocol.                      */
/*005*/    BYTE   unknown05;       /* ?, PTP uses 0x01               */
                                   /*    QETH uses 0x04              */
/*006*/    BYTE   tokenx5;         /* Token length or type or ...    */
/*007*/    BYTE   token[4];        /* Filter token                   */
         } pus_01;                 /*                                */
#define SIZE_PUS_01  0x000B        /* Size of MPC_PUS_01             */

         union  _pus_02 {          /* PUS_02 contents start          */
           struct _a {             /* PUS_02_A contents              */
/*004*/      BYTE   clock[8];      /* STCK value                     */
           } a;                    /*                                */
#define SIZE_PUS_02_A  0x000C      /* Size of MPC_PUS_02_A           */
           struct _b {             /* PUS_02_B contents              */
/*004*/      BYTE   unknown04;     /* ???, only seen 0x02.           */
/*005*/      BYTE   flags;         /* Flags                          */
                                   /*   0xF0  Only seen 0x90 in      */
                                   /*         first 4-bits.          */
                                   /*   0x08  One, ipaddr contains   */
                                   /*         an IPv6 address.       */
                                   /*         Zero, ipaddr contains  */
                                   /*         an IPv4 address.       */
                                   /*   0x07  Only seen zeros.       */
/*006*/      BYTE   unknown06[4];  /* ???, only seen nulls.          */
/*00A*/      BYTE   unknown0A;     /* ???, only seen 0x40.           */
/*00B*/      BYTE   unknown0B;     /* ???, only seen null.           */
/*00C*/      BYTE   ipaddr[16];    /* IP address. (IPv4 address in   */
                                   /* first four bytes, remaining    */
                                   /* bytes contain nulls.)          */
/*01C*/      BYTE   unknown1C[16]; /* ???, only seen nulls.          */
           } b;                    /*                                */
#define SIZE_PUS_02_B  0x002C      /* Size of MPC_PUS_02_B           */
           struct _c {             /* PUS_02_C contents              */
/*004*/      BYTE   nulls[8];      /* Nulls                          */
           } c;                    /*                                */
#define SIZE_PUS_02_C  0x000C      /* Size of MPC_PUS_02_C           */
         } pus_02;                 /* PUS_02 contents end            */

         struct _pus_03 {          /* PUS_03 contents                */
/*004*/    BYTE   tokenx5;         /* Token length or type or ...    */
/*005*/    BYTE   token[4];        /* Filter token                   */
         } pus_03;                 /*                                */
#define SIZE_PUS_03  0x0009        /* Size of MPC_PUS_03             */

         struct _pus_04 {          /* PUS_04 contents                */
/*004*/    BYTE   tokenx5;         /* Token length or type or ...    */
/*005*/    BYTE   token[4];        /* Connection token               */
         } pus_04;                 /*                                */
#define SIZE_PUS_04  0x0009        /* Size of MPC_PUS_04             */

         struct _pus_05 {          /* PUS_05 contents                */
/*004*/    BYTE   tokenx5;         /* Token length or type or ...    */
/*005*/    BYTE   token[4];        /* Filter token                   */
         } pus_05;                 /*                                */
#define SIZE_PUS_05  0x0009        /* Size of MPC_PUS_05             */

         struct _pus_06 {          /* PUS_06 contents                */
/*004*/    BYTE   unknown04[2];    /* ???, only seen 0x4000          */
                                   /*             or 0xC800          */
         } pus_06;                 /*                                */
#define SIZE_PUS_06  0x0006        /* Size of MPC_PUS_06             */

         struct _pus_07 {          /* PUS_07 contents                */
/*004*/    BYTE   unknown04[4];    /* ???, only seen 0x40000000      */
                                   /*             or 0xC8000000      */
         } pus_07;                 /*                                */
#define SIZE_PUS_07  0x0008        /* Size of MPC_PUS_07             */

         struct _pus_08 {          /* PUS_08 contents                */
/*004*/      BYTE   tokenx5;       /* Token length or type or ...    */
/*005*/      BYTE   token[4];      /* Connection token               */
         } pus_08;                 /*                                */
#define SIZE_PUS_08  0x0009        /* Size of MPC_PUS_08             */

         struct _pus_09 {          /* PUS_09 contents                */
/*004*/      FWORD  SeqNum[4];     /* Sequence number                */
/*008*/      FWORD  AckSeqNum[4];  /* Ack Sequence number            */
         } pus_09;                 /*                                */
#define SIZE_PUS_09  0x000C        /* Size of MPC_PUS_09             */

         struct _pus_0A {          /* PUS_0A contents                */
/*004*/      BYTE   unknown04[4];  /* ???                            */
/*008*/      HWORD  mtu;           /* MTU                            */
/*00A*/      BYTE   linknum;       /* Link number                    */
/*00B*/      BYTE   lenportname;   /* Length of the port name        */
/*00C*/      BYTE   portname[8];   /* Port name                      */
/*014*/      BYTE   linktype;      /* Link type                      */
#define QETH_LINK_TYPE_FAST_ETH       0x01   /*                      */
// fine QETH_LINK_TYPE_HSTR           0x02   /*                      */
#define QETH_LINK_TYPE_GBIT_ETH       0x03   /*                      */
// fine QETH_LINK_TYPE_OSN            0x04   /*                      */
#define QETH_LINK_TYPE_10GBIT_ETH     0x10   /*                      */
// fine QETH_LINK_TYPE_LANE_ETH100    0x81   /*                      */
// fine QETH_LINK_TYPE_LANE_TR        0x82   /*                      */
// fine QETH_LINK_TYPE_LANE_ETH1000   0x83   /*                      */
// fine QETH_LINK_TYPE_LANE           0x88   /*                      */
// fine QETH_LINK_TYPE_ATM_NATIVE     0x90   /*                      */
/*015*/      BYTE   unknown15[3];  /* ???                            */
         } pus_0A;                 /*                                */
#define SIZE_PUS_0A_A  0x0014      /* Size of MPC_PUS_0A             */
#define SIZE_PUS_0A_B  0x0018      /* Size of MPC_PUS_0A             */

         struct _pus_0B {          /* PUS_0B contents                */
                                   /* The contents match those in    */
                                   /* one of the 4-byte structures   */
                                   /* at the end of the MPC_IEA/IEAR */
/*004*/      BYTE   ddev[2];       /* Data Device Number             */
/*006*/      BYTE   ddcua;         /* Data Device Control Unit Addr  */
/*007*/      BYTE   ddua;          /* Data Device Unit Address       */
         } pus_0B;                 /*                                */
#define SIZE_PUS_0B  0x0008        /* Size of MPC_PUS_0B             */

         struct _pus_0C {          /* PUS_0C contents                */
/*004*/      BYTE   unknown04[9];  /* ???, only seen                 */
                                   /*      0x000900060401030408      */
                                   /*      Is the 0x0009 a length?   */
                                   /*      If so, does this mean     */
                                   /*      there could be other      */
                                   /*      stuctures?                */
         } pus_0C;                 /*                                */
#define SIZE_PUS_0C  0x000D        /* Size of MPC_PUS_0C             */

    } vc;                          /* Variable contents end          */
} ATTRIBUTE_PACKED;                /*                                */
typedef struct _MPC_PUS MPC_PUS, *PMPC_PUS;

//      PUS_TYPE 01 02 03 04 05 06 07 08 09 0A 0B 0C
//
// CM_ENABLE      x  x                             x
// CM_SETUP                x  x  x
// CM_CONFIRM              x       x   x
// CM_TAKEDOWN             x
// CM_DISABLE           x
//
// ULP_ENABLE     x  x
// ULP_SETUP         x     x  x  x              x
// ULP_CONFIRM       x     x       x   x        x
// ULP_TAKEDOWN            x
// ULP_DISABLE          x
//
// DM_ACT                  x
//
// 4501                    x              x
//

/*===================================================================*/
/* Structures used by OSA devices                                    */
/*===================================================================*/

/*-------------------------------------------------------------------*/
/* Identification Exchange Activate                                  */
/*-------------------------------------------------------------------*/
typedef struct _MPC_IEA {
/*000*/ HWORD   resv000;        /*                                   */
/*002*/ HWORD   ddc;            /* Device Directed Command           */
#define IDX_ACT_DDC     0x8000
/*004*/ FWORD   thsn;           /* Transport Header Sequence Number  */
/* The following 2-bytes are almost certainly not a half word.
   The first byte is probably 2 or more bit fields. If there are
   2 bit fields, the first field is 6-bits long, the second field
   is 2-bits long. If there are 3 (or more!) bit fields, things
   are less certain. The last 2 bits (bits  6-7) are a field,
   and the preceeding 2 bits (bits 4-5) are probably a field,
   10 indicating read and 01 indicating write. What the first
   4 bits (bits 0-3, only seen 0001) signify is unknown. What
   the second byte (only seen 0x01) signifies is also unknown.       */
/*008*/ HWORD   type;           /* IDX_ACT type (read or write)      */
#define IDX_ACT_TYPE_READ       0x1901
#define IDX_ACT_TYPE_WRITE      0x1501
/*00A*/ BYTE    datapath;       /* Number of data paths              */
/*00B*/ BYTE    port;           /* Port number (bit 0 set)           */
#define IDX_ACT_PORT_ACTIVATE   0x80
#define IDX_ACT_PORT_STANDBY    0x40
#define IDX_ACT_PORT_MASK       0x3F
/*00C*/ FWORD   token;          /* Issuer token                      */
/*010*/ HWORD   flevel;         /* Function level                    */
#define IDX_ACT_FLEVEL_READ     0x0000
#define IDX_ACT_FLEVEL_WRITE    0xFFFF
/*012*/ FWORD   uclevel;        /* Microcode level                   */
/*016*/ BYTE    dataset[8];     /* Name                              */
/* The following 4-bytes are repeated for each data path */
/*01E*/ struct {                /* The contents of one of these      */
                                /* 4-byte structures matches the     */
                                /* contents of the PUS_0B structure, */
                                /* used in QETH PUK_TYPE_SETUP and   */
                                /* PUK_TYPE_CONFIRM for a data path. */
/*01E*/     HWORD   ddev;       /* Data Device Number                */
/*020*/     BYTE    ddcua;      /* Data Device Control Unit Address  */
/*021*/     BYTE    ddua;       /* Data Device Unit Address          */
/*022*/   } data;
/*var*/ } MPC_IEA;
#define MPC_IEA_FIRST4 0x00008000  /*                                */

#define MPC_END_FIRST4 0x0000C000  /* Adapter shutdown/close?        */
/* 8-bytes received: those seen are 0000C000 80000000                */

/* Are the first 8-bytes of an IEA and IEAR an XID0? They look very
   similar to those used in PTP, see structure PTPHX0.               */

/*-------------------------------------------------------------------*/
/* Identification Exchange Activate Response                         */
/*-------------------------------------------------------------------*/
typedef struct _MPC_IEAR {
/*000*/ HWORD   resv000;        /*                                   */
/*002*/ HWORD   ddc;            /* Device Directed Command           */
#define IDX_RSP_DDC             0x8000
#define IDX_RSP_CMD_MASK        0xC000
#define IDX_RSP_CMD_TERM        0xC000 /* IDX_TERMINATE received     */
/*004*/ BYTE    reason;         /* Reason code                       */
#define IDX_RSP_REASON_INVPORT  0x22
/*005*/ BYTE    resv005[3];     /*                                   */
/*008*/ BYTE    resp;           /* Response code                     */
#define IDX_RSP_RESP_MASK       0x03
#define IDX_RSP_RESP_OK         0x02
/*009*/ BYTE    cause;          /* Negative response cause code      */
#define IDX_RSP_CAUSE_INUSE     0x19
/*00A*/ BYTE    datapath;       /* Number of data paths              */
/*00B*/ BYTE    flags;          /* Flags                             */
#define IDX_RSP_FLAGS_NOPORTREQ 0x80
#define IDX_RSP_FLAGS_40        0x40  /* Significance unknown */
/*00C*/ FWORD   token;          /* QETHs Issuer token                */
/*010*/ HWORD   flevel;         /* Function level                    */
#define IDX_RSP_FLEVEL_0201     0x0201
/*012*/ FWORD   uclevel;        /* Microcode level                   */
/*016*/ BYTE    dataset[8];     /* Name                              */
/* The following 4-bytes are repeated for each data path */
/*01E*/ struct {
/*01E*/     HWORD   ddev;       /* Data Device Number                */
/*020*/     BYTE    ddcua;      /* Data Device Control Unit Address  */
/*021*/     BYTE    ddua;       /* Data Device Unit Address          */
/*022*/   } data;
/*var*/ } MPC_IEAR;


/*-------------------------------------------------------------------*/
/* IP Assist Header                                                  */
/*-------------------------------------------------------------------*/
typedef struct _MPC_IPA {
/*000*/ BYTE    cmd;            /* Command                           */
/*001*/ BYTE    iid;            /* Initator Identifier               */
#define IPA_IID_HOST    0x00
#define IPA_IID_ADAPTER 0x01
#define IPA_IID_REPLY   0x80
/*002*/ HWORD   seq;            /* Sequence number                   */
/*004*/ HWORD   rc;             /* Return Code                       */
#define IPA_RC_OK       IPA_RC_SUCCESS
/*006*/ BYTE    at;             /* Adapter Type                      */
#define IPA_AT_ANY      0x01
/*007*/ BYTE    port;           /* OSA Port Number                   */
/*008*/ BYTE    lvl;            /* OSA Level                         */
#define IPA_LVL_ANY     0x01
#define IPA_LVL_L2      0x02
/*009*/ BYTE    cnt;            /* Parameter Count                   */
/*00A*/ HWORD   proto;          /* Protocol                          */
#define IPA_PROTO_ANY   0x0000
#define IPA_PROTO_IPV4  0x0004
#define IPA_PROTO_IPV6  0x0006
/*00C*/ FWORD   ipas;           /* Supported IP Assist mask          */
/*010*/ FWORD   ipae;           /* Enabled IP Assist mask            */
/*014*/ } MPC_IPA;
#define SIZE_IPA        0x0014  /* Size of MPC_IPA                   */
#define SIZE_IPA_SHORT  0x0010  /* Size of short (16-byte) MPC_IPA   */


#define IPA_ARP_PROCESSING      0x00000001L  /*  *  *                */
#define IPA_INBOUND_CHECKSUM    0x00000002L  /*                      */
#define IPA_OUTBOUND_CHECKSUM   0x00000004L  /*                      */
#define IPA_IP_FRAGMENTATION    0x00000008L  /*                      */
#define IPA_FILTERING           0x00000010L  /*  *  *                */
#define IPA_IPV6                0x00000020L  /*  *  *                */
#define IPA_MULTICASTING        0x00000040L  /*  *  *                */  /* Must be on for IPv6 */
#define IPA_IP_REASSEMBLY       0x00000080L  /*                      */
#define IPA_QUERY_ARP_COUNTERS  0x00000100L  /*  *  *                */
#define IPA_QUERY_ARP_ADDR_INFO 0x00000200L  /*     *                */
#define IPA_SETADAPTERPARMS     0x00000400L  /*  *  *                */
#define IPA_VLAN_PRIO           0x00000800L  /*  *  *                */
#define IPA_PASSTHRU            0x00001000L  /*  *  *                */
#define IPA_FLUSH_ARP_SUPPORT   0x00002000L  /*     *                */
#define IPA_FULL_VLAN           0x00004000L  /*  *  *                */
#define IPA_INBOUND_PASSTHRU    0x00008000L  /*     *                */
#define IPA_SOURCE_MAC          0x00010000L  /*     *                */
#define IPA_OSA_MC_ROUTER       0x00020000L  /*     *                */
#define IPA_QUERY_ARP_ASSIST    0x00040000L  /*     *                */
#define IPA_INBOUND_TSO         0x00080000L  /*                      */
#define IPA_OUTBOUND_TSO        0x00100000L  /*                      */
                                             /*  |  |                */
                                             /*  |  A zPDT claims    */
                                             /*  |  ..to support     */
                                             /*  |  ..these assists. */
                                             /*  |                   */
                                             /*  Hercules claims     */
                                             /*  ..to support        */
                                             /*  ..these assists.    */

/*  Above is an almost certainly incomplete list of assists that an  */
/*  OSA-device can claim to support. In Hercules the use of these    */
/*  assists has only been seen to be requested by guests using       */
/*  layer 3. Whether any of the assists can be requested by guests   */
/*  using layer 2 is unknown. A guest requests the use of an assist  */
/*  with an IPA_CMD_SETADPPARMS (0xB8) command with an               */
/*  IPA_SAS_CMD_START (0x0001) subcommand. Assist requests are       */
/*  generally propocol dependant, e.g. IPA_ARP_PROCESSING is only    */
/*  requested for IPv4.                                              */

/*  Below is the assists that Hercules claims to support.            */
#if defined(ENABLE_IPV6)
#define IPA_SUPP_IPv4 ( 0 \
                      | IPA_ARP_PROCESSING \
                      | IPA_FILTERING \
                      | IPA_IPV6 \
                      | IPA_MULTICASTING \
                      | IPA_QUERY_ARP_COUNTERS \
                      | IPA_SETADAPTERPARMS \
                      | IPA_PASSTHRU \
                      )
#define IPA_SUPP_IPv6 ( 0 \
                      | IPA_IPV6 \
                      | IPA_MULTICASTING \
                      | IPA_SETADAPTERPARMS \
                      | IPA_PASSTHRU \
                      )
#else
#define IPA_SUPP_IPv4 ( 0 \
                      | IPA_ARP_PROCESSING \
                      | IPA_FILTERING \
                      | IPA_MULTICASTING \
                      | IPA_QUERY_ARP_COUNTERS \
                      | IPA_SETADAPTERPARMS \
                      | IPA_PASSTHRU \
                      )
#endif /*defined(ENABLE_IPV6)*/

#define IPA_CMD_STARTLAN 0x01   /* Start LAN operations              */
#define IPA_CMD_STOPLAN 0x02    /* Stop LAN operations               */
#define IPA_CMD_SETVMAC 0x21    /* Set Layer-2 MAC address           */
#define IPA_CMD_DELVMAC 0x22    /* Delete Layer-2 MAC address        */
#define IPA_CMD_SETGMAC 0x23    /* Set Layer-2 Group Multicast addr  */
#define IPA_CMD_DELGMAC 0x24    /* Del Layer-2 Group Multicast addr  */
#define IPA_CMD_SETVLAN 0x25    /* Set Layer-2 VLAN                  */
#define IPA_CMD_DELVLAN 0x26    /* Delete Layer-2 VLAN               */
#define IPA_CMD_SETBRIDGEPORT 0x2B /*                                */
#define IPA_CMD_SETCCID 0x41    /*                                   */
#define IPA_CMD_DELCCID 0x42    /*                                   */
#define IPA_CMD_MODCCID 0x43    /*                                   */
#define IPA_CMD_SETIP   0xB1    /* Set Layer-3 IP unicast address    */
#define IPA_CMD_QIPASSIST 0xB2  /* Query Layer-3 IP assist capability*/
#define IPA_CMD_SETASSPARMS 0xB3 /* Set Layer-3 IP assist parameters */
#define IPA_CMD_SETIPM  0xB4    /* Set Layer-3 IP multicast address  */
#define IPA_CMD_DELIPM  0xB5    /* Delete Layer-3 IP multicast addr  */
#define IPA_CMD_SETRTG  0xB6    /* Set Layer-3 routing information   */
#define IPA_CMD_DELIP   0xB7    /* Delete Layer-3 IP unicast addr    */
#define IPA_CMD_SETADPPARMS 0xB8 /* Various adapter directed sub-cmds*/
#define IPA_CMD_SETDIAGASS 0xB9 /* Set Layer-3 diagnostic assists    */
#define IPA_CMD_SETBRIDGEPORTIQD 0xBE /*                             */
#define IPA_CMD_CREATEADDR 0xC3 /* Create L3 IPv6 addr from L2 MAC   */
#define IPA_CMD_DESTROYADDR 0xC4 /* Destroy L3 IPv6 addr from L2 MAC */
#define IPA_CMD_REGLCLADDR 0xD1 /*                                   */
#define IPA_CMD_UNREGLCLADDR 0xD2 /*                                 */
#define IPA_CMD_ADDRCHANGENOTIF 0xD3 /*                              */

#define IPA_RC_SUCCESS                   0x0000
#define IPA_RC_NOTSUPP                   0x0001
#define IPA_RC_IP_TABLE_FULL             0x0002
#define IPA_RC_UNKNOWN_ERROR             0x0003
#define IPA_RC_UNSUPPORTED_COMMAND       0x0004
#define IPA_RC_TRACE_ALREADY_ACTIVE      0x0005
#define IPA_RC_INVALID_FORMAT            0x0006
#define IPA_RC_DUP_IPV6_REMOTE           0x0008
#define IPA_RC_DUP_IPV6_HOME             0x0010
#define IPA_RC_UNREGISTERED_ADDR         0x0011
#define IPA_RC_NO_ID_AVAILABLE           0x0012
#define IPA_RC_ID_NOT_FOUND              0x0013
#define IPA_RC_INVALID_IP_VERSION        0x0020
#define IPA_RC_LAN_FRAME_MISMATCH        0x0040
#define IPA_RC_L2_UNSUPPORTED_CMD        0x2003
#define IPA_RC_L2_DUP_MAC                0x2005
#define IPA_RC_L2_ADDR_TABLE_FULL        0x2006
#define IPA_RC_L2_DUP_LAYER3_MAC         0x200a
#define IPA_RC_L2_GMAC_NOT_FOUND         0x200b
#define IPA_RC_L2_MAC_NOT_AUTH_BY_HYP    0x200c
#define IPA_RC_L2_MAC_NOT_AUTH_BY_ADP    0x200d
#define IPA_RC_L2_MAC_NOT_FOUND          0x2010
#define IPA_RC_L2_INVALID_VLAN_ID        0x2015
#define IPA_RC_L2_DUP_VLAN_ID            0x2016
#define IPA_RC_L2_VLAN_ID_NOT_FOUND      0x2017
#define IPA_RC_DATA_MISMATCH             0xe001
#define IPA_RC_INVALID_MTU_SIZE          0xe002
#define IPA_RC_INVALID_LANTYPE           0xe003
#define IPA_RC_INVALID_LANNUM            0xe004
#define IPA_RC_DUPLICATE_IP_ADDRESS      0xe005
#define IPA_RC_IP_ADDR_TABLE_FULL        0xe006
#define IPA_RC_LAN_PORT_STATE_ERROR      0xe007
#define IPA_RC_SETIP_NO_STARTLAN         0xe008
#define IPA_RC_SETIP_ALREADY_RECEIVED    0xe009
#define IPA_RC_IP_ADDR_ALREADY_USED      0xe00a
#define IPA_RC_MC_ADDR_NOT_FOUND         0xe00b
#define IPA_RC_SETIP_INVALID_VERSION     0xe00d
#define IPA_RC_UNSUPPORTED_SUBCMD        0xe00e
#define IPA_RC_ARP_ASSIST_NO_ENABLE      0xe00f
#define IPA_RC_PRIMARY_ALREADY_DEFINED   0xe010
#define IPA_RC_SECOND_ALREADY_DEFINED    0xe011
#define IPA_RC_INVALID_SETRTG_INDICATOR  0xe012
#define IPA_RC_MC_ADDR_ALREADY_DEFINED   0xe013
#define IPA_RC_LAN_OFFLINE               0xe080
#define IPA_RC_VEPA_TO_VEB_TRANSITION    0xe090
#define IPA_RC_INVALID_IP_VERSION2       0xf001
#define IPA_RC_NOMEM                     0xfffe
#define IPA_RC_FFFF                      0xffff


/*-------------------------------------------------------------------*/
/* MAC request                                                       */
/*-------------------------------------------------------------------*/
typedef struct _MPC_IPA_MAC {
/*000*/ FWORD   maclen;         /* Length of MAC Address             */
/*004*/ BYTE    macaddr[FLEXIBLE_ARRAY];  /* MAC Address             */
    } MPC_IPA_MAC;


/*-------------------------------------------------------------------*/
/* Set Adapter Parameters                                            */
/*-------------------------------------------------------------------*/
typedef struct _MPC_IPA_SAP {
/*000*/ FWORD   suppcm;         /* Supported subcommand mask         */
#define IPA_SAP_QUERY     0x00000001L  /* Query Command Supported */
#define IPA_SAP_SETMAC    0x00000002L  /* Alter MAC Address */
#define IPA_SAP_SETGADR   0x00000004L  /* Add/Delete Group Address */
#define IPA_SAP_SETFADR   0x00000008L  /* Add/Delete Functional Address */
#define IPA_SAP_SETAMODE  0x00000010L  /* Set Addressing Mode */
#define IPA_SAP_SETCFG    0x00000020L  /* Set Config Parms */
#define IPA_SAP_SETCFGE   0x00000040L  /* Set Config Parms Extended */
#define IPA_SAP_BRDCST    0x00000080L  /* Set Broadcast Mode */
#define IPA_SAP_OSAMSG    0x00000100L  /* Send OSA Message */
#define IPA_SAP_SETSNMP   0x00000200L  /* Set SNMP Control */
#define IPA_SAP_CARDINFO  0x00000400L  /* Query Card Info */
#define IPA_SAP_PROMISC   0x00000800L  /* Set Promisc Mode */
#define IPA_SAP_SETDIAG   0x00002000L  /* Set Diag Assist */
#define IPA_SAP_SETACCESS 0x00010000L  /* Set Access Control */
#define IPA_SAP_QOAT      0x00080000L  /* Query OAT */
#define IPA_SAP_QSWTATTR  0x00100000L  /* Query Switch Attrubutes */

#define IPA_SAP_SUPP ( 0 \
                     | IPA_SAP_QUERY \
                     | IPA_SAP_SETMAC \
                     | IPA_SAP_CARDINFO \
                     | IPA_SAP_PROMISC \
                     )

/*004*/ FWORD   resv004;        /*                                   */
/*008*/ HWORD   cmdlen;         /* Subcommand length                 */
/*00A*/ HWORD   resv00a;        /*                                   */
/*00C*/ FWORD   cmd;            /* Subcommand                        */
/*010*/ HWORD   rc;             /* Return Code                       */
/*012*/ BYTE    used;           /*                                   */
/*013*/ BYTE    seqno;          /*                                   */
/*014*/ FWORD   resv014;        /*                                   */
    } MPC_IPA_SAP;

/*-------------------------------------------------------------------*/
/* SAP ADP Query                                                     */
/*-------------------------------------------------------------------*/
typedef struct _SAP_QRY {
/*000*/ FWORD   nlan;           /* Number of Lan types supported     */
/*004*/ BYTE    lan_type;       /* LAN type response                 */
/*005*/ BYTE    resv005[3];
/*008*/ FWORD   suppcm;         /* Supported commands bit map        */
/*00C*/ BYTE    resv00c[8];
    } SAP_QRY;

/*-------------------------------------------------------------------*/
/* SAP SETMAC                                                        */
/*-------------------------------------------------------------------*/
typedef struct _SAP_SMA {
/*000*/ FWORD   cmd;            /* Command                           */
#define IPA_SAP_SMA_CMD_READ     0
#define IPA_SAP_SMA_CMD_REPLACE  1
#define IPA_SAP_SMA_CMD_ADD      2
#define IPA_SAP_SMA_CMD_DEL      4
#define IPA_SAP_SMA_CMD_RESET    8
/*004*/ FWORD   asize;          /* Address size                      */
/*008*/ FWORD   nomacs;         /* Number of MAC addresses           */
/*00C*/ BYTE    addr[6];        /* MAC address                       */
    } SAP_SMA;

/*-------------------------------------------------------------------*/
/* SAP Set Promiscuous Mode on/off                                   */
/*-------------------------------------------------------------------*/
typedef struct _SAP_SPM {
/*000*/ FWORD   promisc;        /* Promiscuous mode                  */
#define SAP_PROMISC_ON  0x00000001
#define SAP_PROMISC_OFF 0x00000000
    } SAP_SPM;

/*-------------------------------------------------------------------*/
/* SAP CARDINFO                                                      */
/*-------------------------------------------------------------------*/
typedef struct _SAP_SCI {
/*000*/ BYTE    card_type;      /*                                   */
// fine QETH_CARD_TYPE_UNKNOWN      0x00     /*                      */
#define QETH_CARD_TYPE_OSD          0x01     /*                      */
// fine QETH_CARD_TYPE_OSX          0x02     /*                      */
// fine QETH_CARD_TYPE_OSM          0x03     /*                      */
// fine QETH_CARD_TYPE_IQD          0x05     /*                      */
// fine QETH_CARD_TYPE_OSN          0x06     /*                      */
/*001*/ BYTE    resv001[1];     /*                                   */
/*002*/ HWORD   port_mode;      /*                                   */
// fine QETH_PORT_MODE_HALFDUPLEX   0x0002   /*                      */
#define QETH_PORT_MODE_FULLDUPLEX   0x0003   /*                      */
/*004*/ FWORD   port_speed;     /*                                   */
#define QETH_PORT_SPEED_10M       0x00000005 /*                      */
// fine QETH_PORT_SPEED_100M      0x00000006 /*                      */
// fine QETH_PORT_SPEED_1G        0x00000007 /*                      */
// fine QETH_PORT_SPEED_10G       0x00000008 /*                      */
/*008*/ BYTE    resv008[4];     /*                                   */
    } SAP_SCI;

/*-------------------------------------------------------------------*/
/* Set Assist Parameters                                             */
/*-------------------------------------------------------------------*/
struct MPC_IPA_SAS_HDR {
/*000*/ FWORD   ano;            /* Assist number                     */
/*004*/ HWORD   len;            /* Length                            */
                                /* Note: The length value is the     */
                                /* total length of the length field  */
                                /* and the following fields. It does */
                                /* not include the length of the     */
                                /* preceeding assist number field.   */
/*006*/ HWORD   cmd;            /* Command code                      */
#define IPA_SAS_CMD_START      0x0001
#define IPA_SAS_CMD_STOP       0x0002
#define IPA_SAS_CMD_CONFIGURE  0x0003
#define IPA_SAS_CMD_ENABLE     0x0004
#define IPA_SAS_CMD_0005       0x0005   /* I wonder what this does?  */
#define IPA_SAS_CMD_0006       0x0006   /* And this?                 */
#define IPA_SAS_CMD_0008       0x0008   /* And this?                 */
/*008*/ HWORD   rc;             /* Return code                       */
/*00A*/ BYTE    norep;          /* Number of replies                 */
/*00B*/ BYTE    seqno;          /* Sequence number                   */
    };

typedef struct _MPC_IPA_SAS {
/*000*/ struct MPC_IPA_SAS_HDR hdr;
/*00C*/ union {
            U32    flags_32;
            BYTE   ip[16];
            /* There are other things that are part of the union. */
        } data;
    } MPC_IPA_SAS;

/*-------------------------------------------------------------------*/
/* Set or Delete IP Address                                          */
/*-------------------------------------------------------------------*/
typedef struct _MPC_IPA_SIP {
/*000*/   union {
            struct {
/*000*/       BYTE   addr[4];
/*004*/       BYTE   mask[4];   /* This field should be a subnet     */
                                /* mask, but it usually contains     */
                                /* 0xFFFFFF00, irrespective of the   */
                                /* guests subnet mask. Why?          */
/*008*/       BYTE   flags[4];
            } ip4;
            struct {
/*000*/       BYTE   addr[16];
/*010*/       BYTE   mask[16];  /* This field should be a subnet     */
                                /* mask, but it usually contains     */
                                /* nulls, irrespective of the        */
                                /* guests prefix length. Why?        */
                                /* On zLinux the answer is because   */
                                /* the field copied here is never    */
                                /* initialize with a value.          */
/*020*/       BYTE   flags[4];
            } ip6;
          } data;
        } MPC_IPA_SIP;

#define IPA_SIP_DEFAULT             0x00000000
#define IPA_SIP_VIPA                0x00000001
#define IPA_SIP_TAKEOVER            0x00000002
#define IPA_DIP_ADDR_2_B_TAKEN_OVER 0x00000020
#define IPA_DIP_VIPA                0x00000040
#define IPA_DIP_ADDR_NEEDS_SETIP    0x00000080

/*===================================================================*/
/* Structures used by PTP devices                                    */
/*===================================================================*/

/*-------------------------------------------------------------------*/
/* PDU IP information exchange                                       */
/*-------------------------------------------------------------------*/
struct _MPC_PIX                    /*                                */
{                                  /*                                */
/*000*/  BYTE   action;            /* Action                         */
#define PIX_START    0x01          /*                                */
#define PIX_STOP     0x02          /*                                */
#define PIX_ADDRESS  0x11          /*                                */
/*001*/  BYTE   askans;            /* Ask/Answer                     */
#define PIX_ASK      0x80          /*                                */
#define PIX_ANSWER   0x01          /*                                */
/*002*/  BYTE   numaddr;           /* The number of IP addresses?    */
#define PIX_ONEADDR  0x01          /* (Always contains 1, but there  */
                                   /* has only ever been 1 address.) */
/*003*/  BYTE   iptype;            /* IP address type                */
#define PIX_IPV4     0x04          /*                                */
#define PIX_IPV6     0x06          /*                                */
/*004*/  HWORD  idnum;             /* An identifcation number.       */
/*006*/  HWORD  rcode;             /* Return code.                   */
                                   /*    0  Successful               */
                                   /*    4  Not supported            */
                                   /*   16  Same address             */
/*008*/  BYTE   ipaddr[16];        /* IP address. (IPv4 address in   */
                                   /* first four bytes, remaining    */
                                   /* bytes contain nulls.)          */
/*018*/                            /* End of the MPC_PIX.            */
} ATTRIBUTE_PACKED;                /*                                */
#define SIZE_PIX  0x0018           /* Size of MPC_PIX.               */
typedef struct _MPC_PIX MPC_PIX, *PMPC_PIX;

/* ----------------------------------------------------------------- */
/* PTP Handshake                                   (host byte order) */
/* ----------------------------------------------------------------- */
/* Exchange Identification (XID) Information Fields (XID I-fields).  */
/*                                                 (host byte order) */
/* See the IBM manual 'Systems Network Architecture Formats'.        */
/* ----------------------------------------------------------------- */

// When an MPCPTP/MPCPTP6 link is activated handshaking messages
// are exchanged which are described by the following structures.

struct _PTPHX0;                    // PTP Handshake
struct _PTPHX2;                    // PTP Handshake XID2
struct _PTPHSV;                    // PTP Handshake XID2 CSVcv

typedef struct _PTPHX0 PTPHX0, *PPTPHX0;
typedef struct _PTPHX2 PTPHX2, *PPTPHX2;
typedef struct _PTPHSV PTPHSV, *PPTPHSV;

struct _PTPHX0                     /* PTP Handshake                  */
{                                  /*                                */
/*000*/  BYTE   TH_seg;            /* Only seen 0x00                 */
/*001*/  BYTE   TH_ch_flag;        /* Only seen 0x00 or 0x01         */
#define TH_CH_0x00  0x00           /*                                */
#define TH_CH_0x01  0x01           /*                                */
#define TH_IS_XID   0x01           /*                                */
/*002*/  BYTE   TH_blk_flag;       /* Only seen 0x80 or 0xC0         */
#define TH_DATA_IS_XID  0x80       /*                                */
#define TH_RETRY  0x40             /*                                */
#define TH_DISCONTACT 0xC0         /*                                */
/*003*/  BYTE   TH_is_xid;         /* Only seen 0x01                 */
#define TH_IS_0x01  0x01           /*                                */
/*004*/  BYTE   TH_SeqNum[4];      /* Only seen 0x00050010           */
} ATTRIBUTE_PACKED;                /*                                */
#define SIZE_HX0  0x0008           /* Size of PTPHX0                 */

struct _PTPHX2                     /* PTP Handshake XID2             */
{                                  /*                                */
                                   /* Note: The first 31-bytes       */
                                   /* of the PTPHX2 are an XID2,     */
                                   /* defined in SNA Formats.        */
                                   /*                                */
/*000*/  BYTE   Ft;                /* Format of XID (4-bits),        */
                                   /* Type XID-sending node (4-bits) */
#define XID2_FORMAT_MASK  0xF0     /* Mask out Format from Ft        */
/*001*/  BYTE   Length;            /* Length of the XID2             */
/*002*/  BYTE   NodeID[4];         /* Node identification:           */
                                   /* Block number (12 bits),        */
                                   /* ID number (20-bits)            */
                                   /* Note: the block number is      */
                                   /* always 0xFFF, the ID number is */
                                   /* the high order 5-digits of the */
                                   /* CPU serial number, i.e. if the */
                                   /* serial number is 123456, the   */
                                   /* nodeid will be 0xFFF12345.     */
/*006*/  BYTE   LenXcv;            /* Length of the XID2 exclusive   */
                                   /* of any control vectors         */
/*007*/  BYTE   MiscFlags;         /* Miscellaneous flags            */
/*008*/  BYTE   TGstatus;          /* TG status                      */
/*009*/  BYTE   FIDtypes;          /* FID types supported            */
/*00A*/  BYTE   ULPuse;            /* Upper-layer protocol use       */
/*00B*/  BYTE   LenMaxPIU[2];      /* Length of the maximum PIU that */
                                   /* the XID sender can receive     */
/*00D*/  BYTE   TGNumber;          /* Transmission group number      */
/*00E*/  BYTE   SAaddress[4];      /* Subarea address of the XID     */
                                   /* sender (right-justified        */
                                   /* with leading 0's)              */
/*012*/  BYTE   Flags;             /* Flags                          */
/*013*/  BYTE   CLstatus;          /* CONTACT or load status of      */
                                   /* XID sender                     */
/*014*/  BYTE   IPLname[8];        /* IPL load module name           */
/*01C*/  BYTE   ESAsupp;           /* Extended Subarea Address supp. */
/*01D*/  BYTE   Reserved1D;        /* Reserved                       */
/*01E*/  BYTE   DLCtype;           /* DLC type                       */
#define DLCTYPE_WRITE 0x04         /*   Write path from sender       */
#define DLCTYPE_READ  0x05         /*   Read path from sender        */
                                   /*                                */
                                   /* Note: SNA Formats defines the  */
                                   /* first 31-bytes of an XID2, any */
                                   /* following bytes are DLC type   */
                                   /* specific. SNA Formats defines  */
                                   /* the following bytes for some   */
                                   /* DLC types, but not for DLC     */
                                   /* types 0x04 and 0x05,           */
                                   /* 'Multipath channel to channel; */
                                   /* write path from sender' and    */
                                   /* 'Multipath channel to channel; */
                                   /* read path from sender'.        */
                                   /*                                */
/*01F*/  BYTE   DataLen1[2];       /* ?  Data length?                */
/*021*/  BYTE   MpcFlag;           /* Always contains 0x27           */
/*022*/  BYTE   Unknown22;         /* ?, only seen nulls             */
/*023*/  BYTE   MaxReadLen[2];     /* Maximum read length            */
/*025*/  BYTE   TokenX5;           /* Token length or type or ...    */
/*026*/  BYTE   Token[4];          /* Token                          */
/*02A*/  BYTE   Unknown2A[7];      /* ?, only seen nulls             */
} ATTRIBUTE_PACKED;                /*                                */
#define SIZE_HX2  0x0031           /* Size of PTPHX2                 */

// Call Security Verification (x'56') Control Vector
struct _PTPHSV                     /* PTP Handshake CSVcv            */
{                                  /*                                */
/*000*/  BYTE   Length;            /* Vector length                  */
                                   /* (including this length field)  */
/*001*/  BYTE   Key;               /* Vector key                     */
#define CSV_KEY 0x56               /* CSVcv key                      */
/*002*/  BYTE   reserved02;        /* Reserved                       */
/*003*/  BYTE   LenSIDs;           /* Length of Security IDs         */
                                   /* (including this length field)  */
/*004*/  BYTE   SID1[8];           /* First 8-byte Security ID       */
                                   /* (random data or                */
                                   /* enciphered random data)        */
/*00C*/  BYTE   SID2[8];           /* Second 8-byte Security ID      */
                                   /* (random data or                */
                                   /* enciphered random data or      */
                                   /* space characters)              */
} ATTRIBUTE_PACKED;                /*                                */
#define SIZE_HSV  0x0014           /* Size of PTPHSV                 */


/*********************************************************************/
/* Functions (in mpc.c)                                              */
/*********************************************************************/

MPC_DLL_IMPORT MPC_IPA*  mpc_point_ipa( DEVBLK* pDEVBLK, MPC_TH* pMPC_TH, MPC_RRH* pMPC_RRH );
MPC_DLL_IMPORT MPC_PUK*  mpc_point_puk( DEVBLK* pDEVBLK, MPC_TH* pMPC_TH, MPC_RRH* pMPC_RRH );
MPC_DLL_IMPORT MPC_PUS*  mpc_point_pus( DEVBLK* pDEVBLK, MPC_PUK* pMPC_PUK, BYTE bType );
MPC_DLL_IMPORT void  mpc_display_description( DEVBLK* pDEVBLK, char* pDesc );
MPC_DLL_IMPORT void  mpc_display_stuff( DEVBLK* pDEVBLK, char* cWhat, BYTE* pAddr, int iLen, BYTE bDir );
MPC_DLL_IMPORT void  mpc_display_th( DEVBLK* pDEVBLK, MPC_TH* pMPC_TH, BYTE bDir );
MPC_DLL_IMPORT void  mpc_display_rrh( DEVBLK* pDEVBLK, MPC_RRH* pMPC_RRH, BYTE bDir );
MPC_DLL_IMPORT void  mpc_display_ph( DEVBLK* pDEVBLK, MPC_PH* pMPC_PH, BYTE bDir );
MPC_DLL_IMPORT void  mpc_display_rrh_and_puk( DEVBLK* pDEVBLK, MPC_TH* pMPC_TH, MPC_RRH* pMPC_RRH, BYTE bDir );
MPC_DLL_IMPORT void  mpc_display_rrh_and_pix( DEVBLK* pDEVBLK, MPC_TH* pMPC_TH, MPC_RRH* pMPC_RRH, BYTE bDir );
MPC_DLL_IMPORT void  mpc_display_rrh_and_ipa( DEVBLK* pDEVBLK, MPC_TH* pMPC_TH, MPC_RRH* pMPC_RRH, BYTE bDir );
MPC_DLL_IMPORT void  mpc_display_rrh_and_pkt( DEVBLK* pDEVBLK, MPC_TH* pMPC_TH, MPC_RRH* pMPC_RRH, BYTE bDir, int iLimit );
MPC_DLL_IMPORT void  mpc_display_rrh_and_pdu( DEVBLK* pDEVBLK, MPC_TH* pMPC_TH, MPC_RRH* pMPC_RRH, BYTE bDir, int iLimit );
MPC_DLL_IMPORT void  mpc_display_osa_iea( DEVBLK* pDEVBLK, MPC_IEA* pMPC_IEA, BYTE bDir, int iSize );
MPC_DLL_IMPORT void  mpc_display_osa_iear( DEVBLK* pDEVBLK, MPC_IEAR* pMPC_IEAR, BYTE bDir, int iSize );
MPC_DLL_IMPORT void  mpc_display_osa_th_etc( DEVBLK* pDEVBLK, MPC_TH* pMPC_TH, BYTE bDir, int iLimit );
MPC_DLL_IMPORT void  mpc_display_ptp_th_etc( DEVBLK* pDEVBLK, MPC_TH* pMPC_TH, BYTE bDir, int iLimit );


/*********************************************************************/
/* Notes                                                             */
/*********************************************************************/

/* Note 1.                                                           */
/* The transferred data contains a MPC_TH and one or more MPC_RRHs.  */
/* The first MPC_RRH immediately follows the MPC_TH. When there are  */
/* multiple MPC_RRHs, the second and subsequent MPC_RRHs starts on a */
/* fullword (4-byte) boundary. Each MPC_RRH is immediately followed  */
/* by one or more MPC_PHs. The MPC_PH(s) may be immediately followed */
/* by none, some or all of the data related to the MPC_RRH, e.g.     */
/* just the IP header of a packet or the whole IP packet.            */

/* Note 2.                                                           */
/* Each MPC_RRH is immediately followed by one or more MPC_PHs,      */
/* which refer to the data related to the MPC_RRH. Some or all of    */
/* the data may immediately follow the MPC_PH(s), or the data may be */
/* located some distance away. The location of the data has some     */
/* effect upon the values contained in various length fields.        */
/*                                                                   */
/* When the field MPC_PH->locdata contains a value of 0x01, the data */
/* referenced by the MPC_PH immediately follows the MPC_PH(s),       */
/* usually in the first 4K of the transferred data. When             */
/* MPC_PH->locdata contains a value of 0x02, the data referenced by  */
/* the MPC_PH does not follow the MPC_PH(s), it is located some      */
/* distance away, usually in the second or subsequent 4K of the      */
/* transferred data. (0x01 and 0x02 are the only values that have    */
/* been seen in MPC_PH->locdata, perhaps other values exist.)        */
/*                                                                   */
/* Usually there are only one or two MPC_PHs. When there is a single */
/* MPC_PH, MPC_PH->locdata usually contains a value of 0x01. When    */
/* there are multiple MPC_PHs, MPC_PH->locdata in the first MPC_PH   */
/* contains a value of 0x01, and MPC_PH->locdata in the second       */
/* MPC_PH contains a value of 0x02. Occasionally there is a single   */
/* MPC_PH where MPC_PH->locdata contains a value of 0x02.            */
/*                                                                   */
/* Single MPC_PH, MPC_PH->locdata = 0x01                             */
/*   The whole of the data referenced by the MPC_PH immediately      */
/*   follows the MPC_PH, e.g. an MPC_PUK, or a complete IP packet.   */
/* Multiple MPC_PHs, MPC_PH->locdata = 0x01 and 0x02                 */
/*   The data referenced by the first MPC_PH immediately follows the */
/*   second MPC_PH, e.g. the IP header of an IP packet, and the data */
/*   referenced by the second MPC_PH is located some distance away,  */
/*   e.g. the data of the IP packet.                                 */
/* Single MPC_PH, MPC_PH->locdata = 0x02                             */
/*   The whole of the data referenced by the MPC_PH is located some  */
/*   distance away, e.g. a complete IP packet.                       */
/*                                                                   */
/* Normally MPC_TH->length contains the total length of the MPC_TH   */
/* and all of the data transported with the MPC_TH. However, if      */
/* there is a single MPC_RRH with single MPC_PH and MPC_PH->locdata  */
/* contains 0x02, MPC_TH->length contains only the length of the     */
/* MPC_TH, the MPC_RRH and the MPC_PH.                               */
/*                                                                   */
/* Normally MPC_RRH->lenfida contains the length of the data         */
/* referenced by the first MPC_PH following the MPC_RRH. (Why?)      */
/* However, if there is a single MPC_PH following the MPC_RRH and    */
/* MPC_PH->locdata contains 0x02, MPC_RRH->lenfida contains zero.    */


// Note 3  RRH type & proto
// #define TypeRRH_0x417E  0x417E     /*    0x417E   VTAM/SNA info?      */
// #define TypeRRH_0xC17E  0xC17E     /*    0xC17E   VTAM/SNA info?      */
// #define TypeRRH_0xC108  0xC108     /*    0xC108   IP link/address info? */
// #define TypeRRH_0x8108  0x8108     /*    0x8108   IP packet           */

// Note 4  PUK what & type
// struct _MPC_PUK                    /*                                */
// /*002*/  BYTE   what;              /* What                           */
// /*003*/  BYTE   type;              /* Type                           */
//                                    /* (This is almost certainly      */
//                                    /* two 1-byte (8-bit) values,     */
//                                    /* but until we understand more   */
//                                    /* treat it as a 2-byte value.)   */
//                                    /* 2-byte value.)                 */
// #define PUK_0x4102  0x4102         /*    ENABLE                      */
// #define PUK_0x4103  0x4103         /*                                */
// #define PUK_0x4104  0x4104         /*    SETUP                       */
// #define PUK_0x4105  0x4105         /*                                */
// #define PUK_0x4106  0x4106         /*    confirm                     */
// #define PUK_0x4360  0x4360         /*    complete                    */
// #define PUK_0x4501  0x4501         /*                                */

/* Note 5.                                                              */
/* MPC_PUS->what has only been seen to contain 0x04.                    */
/* MPC_PUS->type shows the type of data in the MPC_PUS. Some types are  */
/* common to multiple device, e.g. 0x02 is used by PTP and QETH, some   */
/* types are unique to a single device, e.g. 0x0C is used by PTP, 0x0A  */
/* is used by QETH. There are presumably other types not yet seen.      */


#if defined(_MSVC_)
 #pragma pack(pop)
#endif


#endif /*!defined(_MPC_H)*/
