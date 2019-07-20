/* ZFCP.H       (C) Copyright Jan Jaeger,   2010-2012                */
/*              Fibre Channel Protocol attached DASD emulation       */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/* This implementation is based on the S/390 Linux implementation    */


#ifndef _ZFCP_H
#define _ZFCP_H

#include "esa390io.h"       /* Need ND/NQ and NED/NEQ structures     */

/*-------------------------------------------------------------------*/
/* Number of Devices ZFCP subchannels per channel                    */
/*-------------------------------------------------------------------*/
#define ZFCP_GROUP_SIZE         1


/*-------------------------------------------------------------------*/
/* Maximum number of supported Queues (Read or Write)                */
/*-------------------------------------------------------------------*/
#define QDIO_MAXQ               32


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
/* ZFCP CCW Assignments                                              */
/*-------------------------------------------------------------------*/
#define ZFCP_RCD        0xFA            /* Read Configuration Data   */
#define ZFCP_SII        0x81            /* Set Interface ID          */
#define ZFCP_RNI        0x82            /* Read Node Identifier      */
#define ZFCP_EQ         0x1B            /* Establish Queues          */
#define ZFCP_AQ         0x1F            /* Activate Queues           */


/*-------------------------------------------------------------------*/
/* OSA Configuration Data Constants                                  */
/*-------------------------------------------------------------------*/

#define  ZFCP_RCD_CIW           MAKE_CIW( CIW_TYP_RCD, ZFCP_RCD, sizeof(configuration_data) )
#define  ZFCP_SII_CIW           MAKE_CIW( CIW_TYP_SII, ZFCP_SII, SII_SIZE )
#define  ZFCP_RNI_CIW           MAKE_CIW( CIW_TYP_RNI, ZFCP_RNI, sizeof(node_data) )
#define  ZFCP_EQ_CIW            MAKE_CIW( CIW_TYP_3,   ZFCP_EQ,  4096 )
#define  ZFCP_AQ_CIW            MAKE_CIW( CIW_TYP_4,   ZFCP_AQ,  0 )

#define  ZFCP_SNSID_1731_03     0x17, 0x31, 0x03    // 1731 model 3
#define  ZFCP_SNSID_1732_03     0x17, 0x32, 0x03    // 1732 model 3

#define  ZFCP_SDC_TYPE_1740     _SDC_TYP( 0xF0, 0xF0, 0xF1, 0xF7, 0xF4, 0xF0 )
#define  ZFCP_SDC_TYPE_1741     _SDC_TYP( 0xF0, 0xF0, 0xF1, 0xF7, 0xF4, 0xF1 )
#define  ZFCP_SDC_TYPE_1742     _SDC_TYP( 0xF0, 0xF0, 0xF1, 0xF7, 0xF4, 0xF2 )
#define  ZFCP_SDC_MODEL_90M     _SDC_MOD( 0xF9, 0xF0, 0xF0 )

#define  ZFCP_DEVICE_SDC        MAKE_SDC( ZFCP_SDC_TYPE_1742, ZFCP_SDC_MODEL_90M, MFR_HRC, PLANT_ZZ, SER_000000000001 )
#define  ZFCP_CTLUNIT_SDC       MAKE_SDC( ZFCP_SDC_TYPE_1741, ZFCP_SDC_MODEL_90M, MFR_HRC, PLANT_ZZ, SER_000000000002 )
#define  ZFCP_TOKEN_SDC         MAKE_SDC( ZFCP_SDC_TYPE_1740, ZFCP_SDC_MODEL_90M, MFR_HRC, PLANT_ZZ, SER_000000000003 )

#define  ZFCP_DEVICE_NED        MAKE_NED( FIELD_IS_NED, NED_NORMAL_NED, NED_SN_NODE, NED_REAL, NED_TYP_DEVICE,  NED_DEV_DASD, NED_RELATED,   ZFCP_DEVICE_SDC,  TAG_00 )
#define  ZFCP_CTLUNIT_NED       MAKE_NED( FIELD_IS_NED, NED_NORMAL_NED, NED_SN_NODE, NED_REAL, NED_TYP_CTLUNIT, NED_DEV_DASD, NED_UNRELATED, ZFCP_CTLUNIT_SDC, TAG_00 )
#define  ZFCP_TOKEN_NED         MAKE_NED( FIELD_IS_NED, NED_TOKEN_NED,  NED_SN_NODE, NED_REAL, NED_TYP_UNSPEC,  NED_DEV_DASD, NED_UNRELATED, ZFCP_TOKEN_SDC,   TAG_00 )
#define  ZFCP_GENERAL_NEQ       NULL_GENEQ

#define  ZFCP_ND                MAKE_ND( ND_TYP_DEVICE, ND_DEV_COMM, ND_CHPID_FF, ZFCP_CTLUNIT_SDC, TAG_00 )
#define  ZFCP_NQ                NULL_MODEP_NQ


/*-------------------------------------------------------------------*/
/* ZFCP Group Structure                                              */
/*-------------------------------------------------------------------*/
typedef struct _ZFCP_GRP {
    COND    qcond;              /* Condition for IDX read thread     */
    LOCK    qlock;              /* Lock for IDX read thread          */

    char *wwpn;                 /*                                   */
    char *lun;                  /*                                   */
    char *brlba;                /*                                   */

    int   ttfd;                 /* ZZ TEMP                           */
    int   ppfd[2];              /* File Descriptor pair write pipe   */

    BYTE   *rspbf;              /* Response Buffer                   */
    int     rspsz;              /* Response Buffer Size              */

    int   reqpci;               /* PCI has been requested            */

    U32   iir;                  /* Interface ID record               */

    int   debug;                /* Adapter in IFF_DEBUG mode         */

    } ZFCP_GRP;


/*-------------------------------------------------------------------*/
/* OSA Layer 2 Header                                                */
/*-------------------------------------------------------------------*/
typedef struct _ZFCP_HDR2 {
/*000*/ BYTE    id;             /*                                   */
#define HDR2_ID_LAYER3  0x01
#define HDR2_ID_LAYER2  0x02
#define HDR2_ID_TSO     0x03
#define HDR2_ID_OSN     0x04
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
    } ZFCP_HDR2;


/*-------------------------------------------------------------------*/
/* Response buffer size                                              */
/*-------------------------------------------------------------------*/
#define RSP_BUFSZ       4096


#endif /* _ZFCP_H */
