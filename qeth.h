/* QETH.H       (c) Copyright Jan Jaeger,   2010-2012                */
/*              OSA Express                                          */

/* This implementation is based on the S/390 Linux implementation    */


#ifndef _QETH_H
#define _QETH_H

#include "esa390io.h"       /* Need ND/NQ and NED/NEQ structures     */


/*-------------------------------------------------------------------*/
/* Maximum number of supported Read or Write Queues and the          */
/* actual default number of supported Queues (Read and Write)        */
/*-------------------------------------------------------------------*/
#define QDIO_MAXQ               32    /* Maximum number of queues    */
#define QETH_QDIO_READQ          1    /* Number of read queues       */
#define QETH_QDIO_WRITEQ         4    /* Number of write queues      */


/*-------------------------------------------------------------------*/
/* Other miscellanous constants                                      */
/*-------------------------------------------------------------------*/
#define OSA_GROUP_SIZE          3     /* Devices per OSA Adapter     */
#define OSA_PORTNO              0     /* OSA Port Number             */
#define OSA_MAXIPV4             1     /* Max supported IPv4 addresses*/
#define OSA_MAXIPV6            32     /* Max supported IPv6 addresses*/
#define OSA_MAXMAC             32     /* Max supported MAC addresses */
#define OSA_TIMEOUTUS       50000     /* Read select timeout (usecs) */

#define QTOKEN1        0xD8C5E3F1     /* QETH token 1 (QET1 ebcdic)  */
#define QTOKEN2        0xD8C5E3F2     /* QETH token 2 (QET2 ebcdic)  */
#define QTOKEN3        0xD8C5E3F3     /* QETH token 3 (QET3 ebcdic)  */
#define QTOKEN4        0xD8C5E3F4     /* QETH token 4 (QET4 ebcdic)  */
#define QTOKEN5        0xD8C5E3F5     /* QETH token 5 (QET5 ebcdic)  */
#define QUCLEVEL       0xC8D9C3F1     /* Microcode level ("HRC1")    */


/*-------------------------------------------------------------------*/
/* OSA CCW Assignments                                               */
/*-------------------------------------------------------------------*/
#define OSA_RCD              0xFA     /* Read Configuration Data     */
#define OSA_SII              0x81     /* Set Interface ID            */
#define OSA_RNI              0x82     /* Read Node Identifier        */
#define OSA_EQ               0x1B     /* Establish Queues            */
#define OSA_AQ               0x1F     /* Activate Queues             */


/*-------------------------------------------------------------------*/
/* Convert Subchannel Token to IO ID (LCSS & SSID)                   */
/*-------------------------------------------------------------------*/
#if 1  /* 1 means token != ioid, or 0 means token == ioid */
#define TKN2IOID(_token)  (~(_token))
#define IOID2TKN(_ioid)   (~(_ioid))
#else
#define TKN2IOID(_token)  (_token)
#define IOID2TKN(_ioid)   (_ioid)
#endif


/*-------------------------------------------------------------------*/
/* OSA Configuration Data Constants                                  */
/*-------------------------------------------------------------------*/
#define  OSA_RCD_CIW            MAKE_CIW( CIW_TYP_RCD, OSA_RCD, sizeof(configuration_data) )
#define  OSA_SII_CIW            MAKE_CIW( CIW_TYP_SII, OSA_SII, SII_SIZE )
#define  OSA_RNI_CIW            MAKE_CIW( CIW_TYP_RNI, OSA_RNI, sizeof(node_data) )
#define  OSA_EQ_CIW             MAKE_CIW( CIW_TYP_3,   OSA_EQ,  4096 )
#define  OSA_AQ_CIW             MAKE_CIW( CIW_TYP_4,   OSA_AQ,  0 )

#define  OSA_SNSID_1730_01      0x17, 0x30, 0x01    // 1730 model 1
#define  OSA_SNSID_1731_01      0x17, 0x31, 0x01    // 1731 model 1
#define  OSA_SNSID_1732_01      0x17, 0x32, 0x01    // 1732 model 1

#define  OSA_SDC_TYPE_1730      _SDC_TYP( 0xF0, 0xF0, 0xF1, 0xF7, 0xF3, 0xF0 )
#define  OSA_SDC_TYPE_1731      _SDC_TYP( 0xF0, 0xF0, 0xF1, 0xF7, 0xF3, 0xF1 )
#define  OSA_SDC_TYPE_1732      _SDC_TYP( 0xF0, 0xF0, 0xF1, 0xF7, 0xF3, 0xF2 )
#define  OSA_SDC_TYPE_9676      _SDC_TYP( 0xF0, 0xF0, 0xF9, 0xF6, 0xF7, 0xF6 )

#define  OSA_DEVICE_SDC         MAKE_SDC( OSA_SDC_TYPE_1732, MODEL_001, MFR_HRC, PLANT_ZZ, SEQ_000000000000 )
#define  OSA_CTLUNIT_SDC        MAKE_SDC( OSA_SDC_TYPE_1730, MODEL_001, MFR_HRC, PLANT_ZZ, SEQ_000000000000 )
#define  OSA_TOKEN_SDC          MAKE_SDC( OSA_SDC_TYPE_1730, MODEL_002, MFR_HRC, PLANT_ZZ, SEQ_000000000000 )

#define  OSA_DEVICE_NED         MAKE_NED( FIELD_IS_NED, NED_NORMAL_NED, NED_SN_NODE, NED_REAL, NED_TYP_DEVICE,  NED_DEV_COMM,   NED_RELATED, OSA_DEVICE_SDC,  TAG_00 )
#define  OSA_CTLUNIT_NED        MAKE_NED( FIELD_IS_NED, NED_NORMAL_NED, NED_SN_NODE, NED_REAL, NED_TYP_CTLUNIT, NED_DEV_UNSPEC, NED_RELATED, OSA_CTLUNIT_SDC, TAG_00 )
#define  OSA_TOKEN_NED          MAKE_NED( FIELD_IS_NED, NED_TOKEN_NED,  NED_SN_NODE, NED_REAL, NED_TYP_UNSPEC,  NED_DEV_COMM,   NED_RELATED, OSA_TOKEN_SDC,   TAG_00 )
#define  OSA_GENERAL_NEQ        NULL_GENEQ

#define  OSA_ND                 MAKE_ND( ND_TYP_DEVICE, ND_DEV_COMM, ND_CHPID_FF, OSA_CTLUNIT_SDC, TAG_00 )
#define  OSA_NQ                 NULL_MODEP_NQ


/*-------------------------------------------------------------------*/
/* OSA Buffer Header                                                 */
/*-------------------------------------------------------------------*/
struct _OSA_BHR;                        /* OSA Buffer Header         */
typedef struct _OSA_BHR   OSA_BHR,  *POSA_BHR;
struct _OSA_BHR {                       /* OSA Buffer Header         */
    OSA_BHR*  next;                     /* Pointer to next OSA_BHR   */
    char*     content;                  /* Pointer to content string */
    int       arealen;                  /* Data area length          */
    int       datalen;                  /* Data length               */
};                                      /*                           */
#define SizeBHR  sizeof(OSA_BHR)        /* Size of OSA_BHR           */


/*-------------------------------------------------------------------*/
/* OSA Buffer Anchor                                                 */
/*-------------------------------------------------------------------*/
typedef struct _OSA_BAN {
    LOCK      lockbhr;          /* Lock for buffer chain             */
    OSA_BHR*  firstbhr;         /* First OSA_BHR in chain            */
    OSA_BHR*  lastbhr;          /* Last OSA_BHR in chain             */
    int       numbhr;           /* Number of OSA_BHRs on chain       */
} OSA_BAN;


/*-------------------------------------------------------------------*/
/* OSA MAC structure                                                 */
/*-------------------------------------------------------------------*/
typedef struct _OSA_MAC {
#ifndef              IFHWADDRLEN
  #define            IFHWADDRLEN    6   /* (Mac OSX is missing this) */
#endif
        BYTE    addr[IFHWADDRLEN];
        int     type;
#define MAC_TYPE_NONE     0x00
#define MAC_TYPE_BRDCST   0x01
#define MAC_TYPE_UNICST   0x02
#define MAC_TYPE_MLTCST   0x04
#define MAC_TYPE_ANY      0x0F
#define MAC_TYPE_PROMISC  0x80  /* Adapter is in promiscuous mode    */
} OSA_MAC;


/*-------------------------------------------------------------------*/
/* OSA IPv4 address structure                                        */
/*-------------------------------------------------------------------*/
typedef struct _OSA_IPV4 {
        BYTE    addr[4];
        int     type;
#define IPV4_TYPE_NONE  0
#define IPV4_TYPE_INUSE 1
} OSA_IPV4;


/*-------------------------------------------------------------------*/
/* OSA IPv6 address structure                                        */
/*-------------------------------------------------------------------*/
typedef struct _OSA_IPV6 {
        BYTE    addr[16];
        int     type;
#define IPV6_TYPE_NONE  0
#define IPV6_TYPE_INUSE 1
} OSA_IPV6;


/*-------------------------------------------------------------------*/
/* OSA Group Structure                                               */
/*-------------------------------------------------------------------*/
typedef struct _OSA_GRP {
    COND    qrcond;             /* Condition for IDX read thread     */
    COND    qdcond;             /* Condition for halt data device    */
    LOCK    qlock;              /* Lock for above conditions         */

    OSA_BAN  idx;               /* IDX buffer anchor                 */

    OSA_BAN  l3r;               /* Layer 3 response buffer anchor    */

    char *tuntap;               /* Interface path name               */
    char  ttifname[IFNAMSIZ];   /* Interface network name            */

    char *tthwaddr;             /* MAC address of the interface      */
    char *ttmtu;                /* MTU of the interface              */

    char *ttipaddr;             /* IPv4 address of the interface     */
    char *ttpfxlen;             /* IPv4 Prefix length of interface   */
    char *ttnetmask;            /* IPv4 Netmask of the interface     */

    char *ttipaddr6;            /* IPv6 address of the interface     */
    char *ttpfxlen6;            /* IPv6 Prefix length of interface   */

    char *ttchpid;              /* chpid                             */

    BYTE  confipaddr4[4];       /* IPv4 address of the interface in  */
                                /* host byte order. This variable    */
                                /* contains the binary equivalent of */
                                /* the ttipaddr string.              */
    BYTE  confpfxmask4[4];      /* IPv4 prefix mask (zeroes then ff) */
    BYTE  confipaddr6[16];      /* IPv6 address of the interface in  */
                                /* host byte order. This variable    */
                                /* contains the binary equivalent of */
                                /* the ttipaddr6 string.             */
    BYTE  confpfxmask6[16];     /* IPv6 prefix mask (zeroes then ff) */

 OSA_IPV4 ipaddr4[OSA_MAXIPV4]; /* Locally recognised IPv4 address   */
 OSA_IPV6 ipaddr6[OSA_MAXIPV6]; /* Locally recognised IPv6 addresses */
  OSA_MAC mac[OSA_MAXMAC];      /* Locally recognised MAC addresses  */

    int   promisc;              /* Adapter is in promiscuous mode    */

    int   enabled;              /* Interface is enabled (IFF_UP)     */
    u_int debugmask;            /* Debug mask                        */
#define DBGQETHPACKET   0x00000001  /* Packet                        */
                                    /* (i.e. the Ethernet frames     */
                                    /* or IP packets sent to or      */
                                    /* received from the TAP device  */
                                    /* in network byte order)        */
#define DBGQETHDATA     0x00000002  /* Data                          */
                                    /* (i.e. the messages presented  */
                                    /* to or accepted from the QETH  */
                                    /* devices in network byte order */
                                    /* Note: a maximun of 256 bytes  */
                                    /* is displayed)                 */
#define DBGQETHEXPAND   0x00000004  /* Data expanded                 */
                                    /* (i.e. the messages presented  */
                                    /* to or accepted from the QETH  */
                                    /* devices in network byte order */
                                    /* showing the MPC_TH etc.       */
                                    /* Note: a maximun of 64 bytes   */
                                    /* of data is displayed)         */
#define DBGQETHUPDOWN   0x00000010  /* Connection up and down        */
#define DBGQETHCCW      0x00000020  /* CCWs executed                 */
    int   l3;                   /* Adapter in layer 3 mode           */
    int   rdpack;               /* Adapter in read packing mode      */
    int   wrpack;               /* Adapter in write packing mode     */
    int   iqPCI;                /* Input  Queue PCI was requested    */
    int   oqPCI;                /* Output Queue PCI was requested    */

    int   ttfd;                 /* File Descriptor TUNTAP Device     */
    int   ppfd[2];              /* Thread signalling socket pipe     */

    U32   seqnumth;             /* MPC_TH sequence number            */
    U32   seqnumis;             /* MPC_RRH sequence number issuer    */
    U32   seqnumcm;             /* MPC_RRH sequence number cm        */

    U32   ipas;                 /* Supported IP assist mask          */
    U32   ipae;                 /* Enabled IP assist mask            */
    U32   iir;                  /* Interface ID record               */

    BYTE  iMAC[IFHWADDRLEN];    /* MAC of the interface              */
    U16   uMTU;                 /* MTU of the interface              */
#define QETH_DEF_MTU   "1500"   /* Default MTU size                  */

    BYTE  gtissue[4];           /* Guest token issuer                */
    BYTE  gtcmfilt[4];          /* Guest token cm filter             */
    BYTE  gtcmconn[4];          /* Guest token cm connection         */
    BYTE  gtulpfilt[4];         /* Guest token ulp filter            */
    BYTE  gtulpconn[4];         /* Guest token ulp connection        */

#if defined(ENABLE_IPV6)
    BYTE  iaDriveMACAddr[IFHWADDRLEN];   /* MAC address (Driver)     */
    char  szDriveMACAddr[24];            /* MAC address (Driver)     */
    struct in6_addr iaDriveLLAddr6;      /* IPv6 Link Local address (Driver) */
    char            szDriveLLAddr6[48];  /* IPv6 Link Local address (Driver) */
#endif /* defined(ENABLE_IPV6) */

} OSA_GRP;


/*-------------------------------------------------------------------*/
/* OSA Header Id Types                                               */
/*-------------------------------------------------------------------*/
#define HDR_ID_LAYER3    0x01   /* Standard IPv4/IPv6 Packet         */
#define HDR_ID_LAYER2    0x02   /* Ethernet Layer 2 Frame            */
#define HDR_ID_TSO       0x03   /* Layer 3 TCP Segmentation Offload  */
#define HDR_ID_OSN       0x04   /* Channel Data Link Control (CDLC)  */


/*-------------------------------------------------------------------*/
/* Pack below OSA Layer headers to byte boundary...                  */
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
/* OSA Layer 2 Header                                                */
/*-------------------------------------------------------------------*/
struct OSA_HDR2 {
/*000*/ BYTE    id;             /* Packet Id                         */
/*001*/ BYTE    flags[3];       /*                                   */
#define HDR2_FLAGS0_PASSTHRU    0x10
#define HDR2_FLAGS0_IPV6        0x80
#define HDR2_FLAGS0_CASTMASK    0x07
#define HDR2_FLAGS0_ANYCAST     0x07
#define HDR2_FLAGS0_UNICAST     0x06
#define HDR2_FLAGS0_BROADCAST   0x05
#define HDR2_FLAGS0_MULTICAST   0x04
#define HDR2_FLAGS0_NOCAST      0x00
#define HDR2_FLAGS2_MULTICAST   0x01
#define HDR2_FLAGS2_BROADCAST   0x02
#define HDR2_FLAGS2_UNICAST     0x03
#define HDR2_FLAGS2_VLAN        0x04
/*004*/ BYTE    portno;         /*                                   */
/*005*/ BYTE    hdrlen;         /*                                   */
/*006*/ HWORD   pktlen;         /*                                   */
/*008*/ HWORD   seqno;          /*                                   */
/*00A*/ HWORD   vlanid;         /*                                   */
/*00C*/ FWORD   resv00c[5];     /*                                   */
/*020*/ } ATTRIBUTE_PACKED;

typedef struct OSA_HDR2 OSA_HDR2;


/*-------------------------------------------------------------------*/
/* OSA Layer 3 Header                                                */
/*-------------------------------------------------------------------*/
struct OSA_HDR3 {
/*000*/ BYTE    id;             /* Packet Id                         */
/*001*/ BYTE    flags;          /* Flags                             */

#define HDR3_FLAGS_IPV6       0x80  /* 1=IPv6, 0=IPv4                */
#define HDR3_FLAGS_PASSTHRU   0x10  /* Pass through packet (IPv6)    */
#define HDR3_FLAGS_CASTMASK   0x07  /* Cast type                     */
#define HDR3_FLAGS_NOTFORUS   0xFF  /* Not meant for us (internal)   */
#define HDR3_FLAGS_NOCAST     0x00  /* No cast                       */
#define HDR3_FLAGS_MULTICAST  0x04  /* Multicast                     */
#define HDR3_FLAGS_BROADCAST  0x05  /* Broadcast                     */
#define HDR3_FLAGS_UNICAST    0x06  /* Unicast                       */
#define HDR3_FLAGS_ANYCAST    0x07  /* Anycast                       */
#define HDR3_FLAGS_UNUSED     0x68  /* Unused bits; must be zero     */

/*002*/ HWORD   in_cksum;       /* Inbound checksum (TSO: sequence#) */
/*004*/ FWORD   token;          /* Token  ????      (TSO: reserved)  */
/*008*/ HWORD   length;         /* Packet size (not including HDR)   */
/*00A*/ BYTE    vlan_prio;      /* (not used)                        */
/*00B*/ BYTE    ext_flags;      /* Extended Buffer flags             */

#define HDR3_EXFLAG_UNUSED    0x80  /* Unused; must be zero          */
#define HDR3_EXFLAG_UDP       0x40  /* 1=UDP packet; 0=TCP           */
#define HDR3_EXFLAG_TPCKSUM   0x20  /* Trnspt cksum; 1=chked, 0=not  */
#define HDR3_EXFLAG_PKCKSUM   0x10  /* PktHdr cksum; 1=chked, 0=not  */
#define HDR3_EXFLAG_SRCMAC    0x08  /* External source MAC present   */
#define HDR3_EXFLAG_VLANTAG   0x04  /* VLAN Tag in dest_addr 12-13   */
#define HDR3_EXFLAG_TOKENID   0x02  /* Token Id present              */
#define HDR3_EXFLAG_VLANID    0x01  /* vlan_id field present         */

/*00C*/ HWORD   vlan_id;        /* VLAN ID (if HDR3_EXFLAG_VLANID)   */
/*00E*/ HWORD   frame_offset;   /* Frame offset (TSO only?)          */
/*010*/ BYTE    dest_addr[16];  /* Destination address: IPv4[12-15]
                                   HDR3_EXFLAG_VLANTAG: VTag[12-13]  */
/*020*/ } ATTRIBUTE_PACKED;     /* Total length: 32 bytes            */

typedef struct OSA_HDR3 OSA_HDR3;


#if defined(_MSVC_)
 #pragma pack(pop)
#endif


#if !defined( OPTION_W32_CTCI )
/*-------------------------------------------------------------------*/
/* Default pathname of the TUNTAP adapter                            */
/*-------------------------------------------------------------------*/
#define TUNTAP_NAME "/dev/net/tun"

#endif /*!defined( OPTION_W32_CTCI )*/


#endif /*_QETH_H*/