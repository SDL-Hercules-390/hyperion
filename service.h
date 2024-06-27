/* SERVICE.H    (C) Copyright Roger Bowler, 1999-2012                */
/*              (C) Copyright Jan Jaeger, 1999-2012                  */
/*              Service Processor Architectured fields               */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/* Interpretive Execution - (C) Copyright Jan Jaeger, 1999-2012      */
/* z/Architecture support - (C) Copyright Jan Jaeger, 1999-2012      */

#ifndef _SERVICE_H
#define _SERVICE_H

/*-------------------------------------------------------------------*/
/*    Service Call Logical Processor command word definitions        */
/*-------------------------------------------------------------------*/
#define SCLP_READ_SCP_INFO      0x00020001
#define SCLP_READ_IFL_INFO      0x00120001
#define SCLP_READ_CHP_INFO      0x00030001
#define SCLP_READ_CSI_INFO      0x001C0001

#define SCLP_READ_XST_MAP       0x00250001

#define SCLP_WRITE_EVENT_DATA   0x00760005
#define SCLP_READ_EVENT_DATA    0x00770005
#define SCLP_WRITE_EVENT_MASK   0x00780005

#define SCLP_DECONFIGURE_CPU    0x00100001
#define SCLP_CONFIGURE_CPU      0x00110001

#define SCLP_DISCONNECT_VF      0x001A0001
#define SCLP_CONNECT_VF         0x001B0001

#define SCLP_COMMAND_MASK       0xFFFF00FF
#define SCLP_COMMAND_CLASS      0x000000FF
#define SCLP_RESOURCE_MASK      0x0000FF00
#define SCLP_RESOURCE_SHIFT     8

/*-------------------------------------------------------------------*/
/*     Service Call Control Block (SCCB) header layout               */
/*-------------------------------------------------------------------*/
typedef struct _SCCB_HEADER
{
/*00*/  HWORD   length;                 /* Total length of SCCB      */
/*02*/  BYTE    flag;                   /* Flag byte                 */
/*03*/  BYTE    resv1[2];               /* Reserved                  */
/*05*/  BYTE    type;                   /* Request type              */
/*06*/  BYTE    reas;                   /* Reason code               */
/*07*/  BYTE    resp;                   /* Response class code       */
}
SCCB_HEADER; /* size: 0x08 */

/* Bit definitions for SCCB header flag byte */
#define SCCB_FLAG_SYNC          0x80    /* Synchronous request       */

/* Bit definitions for SCCB header request type */
#define SCCB_TYPE_VARIABLE      0x80    /* Variable request          */

/* Bit definitions for SCCB header reason code */
#define SCCB_REAS_NONE          0x00    /* No reason                 */
#define SCCB_REAS_NOT_PGBNDRY   0x01    /* SCCB crosses page boundary*/
#define SCCB_REAS_ODD_LENGTH    0x02    /* Length not multiple of 8  */
#define SCCB_REAS_TOO_SHORT     0x03    /* Length is inadequate      */
#define SCCB_REAS_NOACTION      0x02    /* Resource in req. state    */
#define SCCB_REAS_STANDBY       0x04    /* Resource in standby state */
#define SCCB_REAS_INVALID_CMD   0x01    /* Invalid SCLP command code */
#define SCCB_REAS_INVALID_RSCP  0x03    /* Invalid resource in parm  */
#define SCCB_REAS_IMPROPER_RSC  0x05    /* Resource in improper state*/
#define SCCB_REAS_INVALID_RSC   0x09    /* Invalid resource          */

/* Bit definitions for SCCB header response class code */
#define SCCB_RESP_BLOCK_ERROR   0x00    /* Data block error          */
#define SCCB_RESP_INFO          0x10    /* Information returned      */
#define SCCB_RESP_COMPLETE      0x20    /* Command complete          */
#define SCCB_RESP_BACKOUT       0x40    /* Command backed out        */
#define SCCB_RESP_REJECT        0xF0    /* Command reject            */

#define SCCB_REAS_NO_EVENTS     0x60    /* No outstanding EVENTs     */
#define SCCB_RESP_NO_EVENTS     0xF0
#define SCCB_REAS_EVENTS_SUP    0x62    /* All events suppressed     */
#define SCCB_RESP_EVENTS_SUP    0xF0
#define SCCB_REAS_INVALID_MASK  0x70    /* Invalid events mask       */
#define SCCB_RESP_INVALID_MASK  0xF0
#define SCCB_REAS_MAX_BUFF      0x71    /* Buffer exceeds maximum    */
#define SCCB_RESP_MAX_BUFF      0xF0
#define SCCB_REAS_BUFF_LEN_ERR  0x72    /* Buffer len verification   */
#define SCCB_RESP_BUFF_LEN_ERR  0xF0
#define SCCB_REAS_SYNTAX_ERROR  0x73    /* Buffer syntax error       */
#define SCCB_RESP_SYNTAX_ERROR  0xF0
#define SCCB_REAS_INVALID_MSKL  0x74    /* Invalid mask length       */
#define SCCB_RESP_INVALID_MSKL  0xF0
#define SCCB_REAS_EXCEEDS_SCCB  0x75    /* Exceeds SCCB max capacity */
#define SCCB_RESP_EXCEEDS_SCCB  0xF0

/*-------------------------------------------------------------------*/
/* The following maximum value constants define the architectural    */
/* limit for the amount of real storage based on the realinum and    */
/* realiszm field widths in the below _SCCB_SCP_INFO structure.      */
/*-------------------------------------------------------------------*/
#define MAX_STORINCR        0xFFFFULL    /* SCP realinum field limit */
#define MAX_INCRSIZE_MB     0xFFULL      /* SCP realiszm field limit */
#define MAX_1MINCR_STORSIZE (MAX_STORINCR * (       1        * _1M))
#define MAX_SCP_STORSIZE    (MAX_STORINCR * (MAX_INCRSIZE_MB * _1M))

/*-------------------------------------------------------------------*/
/*   SCCB System Control Program (SCP) information block layout      */
/*-------------------------------------------------------------------*/
typedef struct _SCCB_SCP_INFO
{
        HWORD   realinum;               /* Number of real storage
                                           increments installed      */
        BYTE    realiszm;               /* Size of each real storage
                                           increment in MB           */
        BYTE    realbszk;               /* Size of each real storage
                                           block in KB               */
        HWORD   realiint;               /* Real storage increment
                                           block interleave interval */
        HWORD   resv2;                  /* Reserved                  */
        HWORD   numcpu;                 /* Number of CPUs installed  */
        HWORD   offcpu;                 /* Offset from start of SCCB
                                           to CPU information array  */
        HWORD   numhsa;                 /* Number of HSAs            */
        HWORD   offhsa;                 /* Offset from start of SCCB
                                           to HSA information array  */
        BYTE    loadparm[8];            /* Load parameter            */
        FWORD   xpndinum;               /* Number of expanded storage
                                           increments installed      */
        FWORD   xpndsz4K;               /* Number of 4KB blocks in an
                                           expanded storage increment*/
        HWORD   xpndenum;               /* Number of expanded storage
                                           elements installed        */
        HWORD   resv3;                  /* Reserved                  */
        HWORD   vectssiz;               /* Vector section size       */
        HWORD   vectpsum;               /* Vector partial sum number */
        BYTE    ifm[8];                 /* Installed facilities      */
        BYTE    resv4[8];               /* Reserved                  */
        HWORD   maxresgp;               /* Maximum resource group    */
        BYTE    resv5[6];               /* Reserved                  */
        HWORD   nummpf;                 /* Number of entries in MPF
                                           information array         */
        HWORD   offmpf;                 /* Offset from start of SCCB
                                           to MPF information array  */
        BYTE    resv6[4];               /* Reserved                  */
        BYTE    cfg[6];                 /* Config characteristics    */
        FWORD   rcci;                   /* Capacity                  */
        BYTE    cfg11;                  /* Config char. byte 11      */
        BYTE    numcrl;                 /* Max #of copy and reassign
                                           list elements allowed     */
        FWORD   etrtol;                 /* ETR sync check tolerance  */
        BYTE    resv60[3];
        BYTE    maxvm;                  /* Max guest storage size
                                           >= 31 and <= 64 (2**pow)-1
                                           is the max supported
                                           guest real size. 0 means
                                           not constrained.          */
        FWORD   grzm;                   /* Addess increment size in
                                           units of 1M, valid only
                                           if realiszm is zero       */
        DBLWRD  grnmx;                  /* Maximum increment number
                                           when it is larger than
                                           64K or when ESAME is on   */
        BYTE    resv8[16];              /* Reserved                  */
}
SCCB_SCP_INFO;

/* Bit definitions for installed facilities */
#define SCCB_IFM0_CHANNEL_PATH_INFORMATION              0x80
#define SCCB_IFM0_CHANNEL_PATH_SUBSYSTEM_COMMAND        0x40
#define SCCB_IFM0_CHANNEL_PATH_RECONFIG                 0x20
#define SCCB_IFM0_CPU_INFORMATION                       0x08
#define SCCB_IFM0_CPU_RECONFIG                          0x04

#define SCCB_IFM1_SIGNAL_ALARM                          0x80
#define SCCB_IFM1_WRITE_OPERATOR_MESSAGE                0x40
#define SCCB_IFM1_STORE_STATUS_ON_LOAD                  0x20
#define SCCB_IFM1_RESTART_REASONS                       0x10
#define SCCB_IFM1_INSTRUCTION_ADDRESS_TRACE_BUFFER      0x08
#define SCCB_IFM1_LOAD_PARAMETER                        0x04
#define SCCB_IFM1_READ_AND_WRITE_DATA                   0x02

#define SCCB_IFM2_REAL_STORAGE_INCREMENT_RECONFIG       0x80
#define SCCB_IFM2_REAL_STORAGE_ELEMENT_INFO             0x40
#define SCCB_IFM2_REAL_STORAGE_ELEMENT_RECONFIG         0x20
#define SCCB_IFM2_COPY_AND_REASSIGN_STORAGE             0x10
#define SCCB_IFM2_EXTENDED_STORAGE_USABILITY_MAP        0x08
#define SCCB_IFM2_EXTENDED_STORAGE_ELEMENT_INFO         0x04
#define SCCB_IFM2_EXTENDED_STORAGE_ELEMENT_RECONFIG     0x02
#define SCCB_IFM2_COPY_AND_REASSIGN_STORAGE_LIST        0x01
#define SCCB_IFM3_VECTOR_FEATURE_RECONFIG               0x80

#define SCCB_IFM3_READ_WRITE_EVENT_FEATURE              0x40
#define SCCB_IFM3_EXTENDED_STORAGE_USABILITY_MAP_EXT    0x20
#define SCCB_IFM3_READ_RESOURCE_GROUP_INFO              0x08
#define SCCB_IFM4_READ_STORAGE_STATUS                   0x80

/* Bit definitions for configuration characteristics */
#define SCCB_CFG0_LOGICALLY_PARTITIONED                 0x80
#define SCCB_CFG0_SUPPRESSION_ON_PROTECTION             0x20
#define SCCB_CFG0_INITIATE_RESET                        0x10
#define SCCB_CFG0_STORE_CHANNEL_SUBSYS_CHARACTERISTICS  0x08
#define SCCB_CFG0_FAST_SYNCHRONOUS_DATA_MOVER           0x01
#define SCCB_CFG0_MVPG_FOR_ALL_GUESTS                   0x04
#define SCCB_CFG0_UNKNOWN_BUT_SET_UNDER_VM              0x02

#define SCCB_CFG1_CSLO                                  0x40

#define SCCB_CFG2_DEVICE_ACTIVE_ONLY_MEASUREMENT        0x40
#define SCCB_CFG2_CALLED_SPACE_IDENTIFICATION           0x02
#define SCCB_CFG2_CHECKSUM_INSTRUCTION                  0x01

#define SCCB_CFG3_RESUME_PROGRAM                        0x80
#define SCCB_CFG3_PERFORM_LOCKED_OPERATION              0x40
#define SCCB_CFG3_IMMEDIATE_AND_RELATIVE                0x10
#define SCCB_CFG3_COMPARE_AND_MOVE_EXTENDED             0x08
#define SCCB_CFG3_BRANCH_AND_SET_AUTHORITY              0x04
#define SCCB_CFG3_EXTENDED_FLOATING_POINT               0x02
#define SCCB_CFG3_EXTENDED_LOGICAL_COMPUTATION_FACILITY 0x01

#define SCCB_CFG4_EXTENDED_TOD_CLOCK                    0x80
#define SCCB_CFG4_EXTENDED_TRANSLATION                  0x40
#define SCCB_CFG4_LOAD_REVERSED_FACILITY                0x20
#define SCCB_CFG4_EXTENDED_TRANSLATION_FACILITY2        0x10
#define SCCB_CFG4_STORE_SYSTEM_INFORMATION              0x08
#define SCCB_CFG4_LPAR_CLUSTERING                       0x02
#define SCCB_CFG4_IFA_FACILITY                          0x01

#define SCCB_CFG5_SENSE_RUNNING_STATUS                  0x08
#define SCCB_CFG5_ESAME                                 0x01

#define SCCB_CFGB_PER_3                                 0x04
#define SCCB_CFGB_LOAD_WITH_DUMP                        0x02
#define SCCB_CFGB_LIST_DIRECTED_IPL                     0x01

/*-------------------------------------------------------------------*/
/*            CPU information array entry                            */
/*-------------------------------------------------------------------*/
typedef struct _SCCB_CPU_INFO
{
        BYTE    cpa;                    /* CPU address               */
        BYTE    tod;                    /* TOD clock number          */
        BYTE    cpf[12];                /* RCPU facility map         */
        BYTE    ptyp;                   /* Processor type            */
        BYTE    ksid;                   /* Crypto unit identifier    */
}
SCCB_CPU_INFO;

/* Bit definitions for CPU installed features */
#define SCCB_CPF0_SIE_370_MODE                          0x80
#define SCCB_CPF0_SIE_XA_MODE                           0x40
#define SCCB_CPF0_SIE_SET_II_370_MODE                   0x20
#define SCCB_CPF0_SIE_SET_II_XA_MODE                    0x10
#define SCCB_CPF0_SIE_NEW_INTERCEPT_FORMAT              0x08
#define SCCB_CPF0_STORAGE_KEY_ASSIST                    0x04
#define SCCB_CPF0_MULTIPLE_CONTROLLED_DATA_SPACE        0x02

#define SCCB_CPF1_IO_INTERPRETATION_LEVEL_2             0x40
#define SCCB_CPF1_GUEST_PER_ENHANCED                    0x20
#define SCCB_CPF1_SIGP_INTERPRETATION_ASSIST            0x08
#define SCCB_CPF1_RCP_BYPASS_FACILITY                   0x04
#define SCCB_CPF1_REGION_RELOCATE_FACILITY              0x02
#define SCCB_CPF1_EXPEDITE_TIMER_PROCESSING             0x01

#define SCCB_CPF2_VECTOR_FEATURE_INSTALLED              0x80
#define SCCB_CPF2_VECTOR_FEATURE_CONNECTED              0x40
#define SCCB_CPF2_VECTOR_FEATURE_STANDBY_STATE          0x20
#define SCCB_CPF2_CRYPTO_FEATURE_ACCESSED               0x10
#define SCCB_CPF2_EXPEDITE_RUN_PROCESSING               0x04

#define SCCB_CPF3_PRIVATE_SPACE_FEATURE                 0x80
#define SCCB_CPF3_FETCH_ONLY_BIT                        0x40
#define SCCB_CPF3_PER2_INSTALLED                        0x01

#define SCCB_CPF4_OMISSION_GR_ALTERATION_370            0x80

#define SCCB_CPF5_GUEST_WAIT_STATE_ASSIST               0x40

/*-------------------------------------------------------------------*/
/*       Processor type codes  (mostly for diag 224)                 */
/*-------------------------------------------------------------------*/
/* PROGRAMMING NOTE: if you define/add a new value to the below,     */
/* be sure to ALSO update the 'ptypes' table in 'hsccmd.c'!          */
/*-------------------------------------------------------------------*/
#define SCCB_PTYP_CP                                    0
#define SCCB_PTYP_UNKNOWN                               1
#define SCCB_PTYP_ZAAP                                  2
#define SCCB_PTYP_IFL                                   3
#define SCCB_PTYP_ICF                                   4
#define SCCB_PTYP_ZIIP                                  5
#define MAX_SCCB_PTYP /*(see above PROGRAMMING NOTE!)*/ 5

/*-------------------------------------------------------------------*/
/*                Processor type macro                               */
/*-------------------------------------------------------------------*/

/* WARNING! Uses 'ptyp' array defined in 'sysblk'! This macro
   will thus *NOT* work correctly for utilities, whose sysblk
   is NEVER fully initialized! Call the 'ptyp2short' function
   directly yourself instead!
*/
#define PTYPSTR(cpu_num)        ptyp2short( sysblk.ptyp[(cpu_num)] )

/* Macro converts event type to event mask */
#define EVDMASK(_type) \
        (0x80000000 >> (SCCB_EVD_TYPE_ ## _type - 1))

#define SCLP_RECV_ENABLED(_type) \
    (servc_cp_recv_mask & EVDMASK(_type))

/* Definitions for crypto unit identifier */
#define SCCB_KSID_CRYPTO_UNIT_ID                        0x01

/*-------------------------------------------------------------------*/
/*               HSA information array entry                         */
/*-------------------------------------------------------------------*/
typedef struct _SCCB_HSA_INFO
{
        HWORD   hssz;                   /* Size of HSA in 4K blocks  */
        FWORD   ahsa;                   /* Address of HSA            */
}
SCCB_HSA_INFO;

/*-------------------------------------------------------------------*/
/*               MPF information array entry                         */
/*-------------------------------------------------------------------*/
typedef struct _SCCB_MPF_INFO
{
        HWORD   mpfy;                   /* MPF info array entry      */
}
SCCB_MPF_INFO;

/*-------------------------------------------------------------------*/
/*           Channel path information data area                      */
/*-------------------------------------------------------------------*/
typedef struct _SCCB_CHP_INFO
{
        BYTE    installed[32];          /* Channels installed bits   */
        BYTE    standby[32];            /* Channels standby bits     */
        BYTE    online[32];             /* Channels online bits      */
}
SCCB_CHP_INFO;

/*-------------------------------------------------------------------*/
/*           Channel path information data area                      */
/*-------------------------------------------------------------------*/
typedef struct _SCCB_CHSET
{
        BYTE    chanset0a[32];          /* 370 channel set 0A        */
        BYTE    chanset1a[32];          /* 370 channel set 1A        */
        BYTE    chanset0b[32];          /* 370 channel set 0B        */
        BYTE    chanset1b[32];          /* 370 channel set 1B        */
        BYTE    csconfig;               /* Channel set configuration */
        BYTE    resv[23];               /* Reserved, set to zero     */
}
SCCB_CHSET_INFO;

/*-------------------------------------------------------------------*/
/*        Read Channel Subsystem Information data area               */
/*-------------------------------------------------------------------*/
typedef struct _SCCB_CSI_INFO
{
        BYTE    csif[8];                /* Channel Subsystem installed
                                           facility field            */
        BYTE    resv[48];
}
SCCB_CSI_INFO;

/* Bit definitions for channel subsystem installed facilities */
#define SCCB_CSI0_CANCEL_IO_REQUEST_FACILITY            0x02
#define SCCB_CSI0_CONCURRENT_SENSE_FACILITY             0x01

/*-------------------------------------------------------------------*/
/*                    Write Event Mask                               */
/*-------------------------------------------------------------------*/
typedef struct _SCCB_EVT_MASK
{
/*00*/  HWORD   reserved;
/*02*/  HWORD   length;                 /* Event mask length         */
/*04*/  BYTE    masks[32];              /* Event masks               */
/*??*///FWORD   cp_recv_mask;           /* These mask fields have    */
/*??*///FWORD   cp_send_mask;           /* the length defined by     */
/*??*///FWORD   sclp_recv_mask;         /* the length halfword       */
/*??*///FWORD   sclp_send_mask;
}
SCCB_EVT_MASK;

#define SCCB_EVENT_CONS_RECV_MASK ( \
        (0x80000000 >> (SCCB_EVD_TYPE_MSG-1))   | \
        (0x80000000 >> (SCCB_EVD_TYPE_PRIOR-1)) )

#define SCCB_EVENT_CONS_SEND_MASK ( \
        (0x80000000 >> (SCCB_EVD_TYPE_OPCMD-1)) | \
        (0x80000000 >> (SCCB_EVD_TYPE_PRIOR-1)) | \
        (0x80000000 >> (SCCB_EVD_TYPE_CPCMD-1)) )

/*-------------------------------------------------------------------*/
/*              Read/Write Event Data Header                         */
/*-------------------------------------------------------------------*/
typedef struct _SCCB_EVD_HDR
{
/*00*/  HWORD   totlen;                 /* Event Data Buffer total
                                           length                    */
/*02*/  BYTE    type;

#define SCCB_EVD_TYPE_OPCMD     0x01    /* 80 Operator command       */
#define SCCB_EVD_TYPE_MSG       0x02    /* 40 Msg from Control Pgm   */
//efine SCCB_EVD_TYPE_???       0x03    /* 20 ???                    */
//efine SCCB_EVD_TYPE_???       0x04    /* 10 ???                    */
//efine SCCB_EVD_TYPE_???       0x05    /* 08 ???                    */
//efine SCCB_EVD_TYPE_???       0x06    /* 04 ???                    */
#define SCCB_EVD_TYPE_SCEDIO    0x07    /* 02 SCE DASD I/O           */
#define SCCB_EVD_TYPE_STATECH   0x08    /* 01 State Change           */

#define SCCB_EVD_TYPE_PRIOR     0x09    /* 80 Priority msg/cmd       */
//efine SCCB_EVD_TYPE_???       0x0A    /* 40 ???                    */
#define SCCB_EVD_TYPE_CPIDENT   0x0B    /* 20 CntlProgIdent          */
#define SCCB_EVD_TYPE_HWL       0x0C    /* 10 Hardware Loader        */
//efine SCCB_EVD_TYPE_???       0x0D    /* 08 ???                    */
//efine SCCB_EVD_TYPE_???       0x0E    /* 04 ???                    */
//efine SCCB_EVD_TYPE_???       0x0F    /* 02 ???                    */
//efine SCCB_EVD_TYPE_???       0x10    /* 01 ???                    */

//efine SCCB_EVD_TYPE_???       0x11    /* 80 ???                    */
//efine SCCB_EVD_TYPE_???       0x12    /* 40 ???                    */
//efine SCCB_EVD_TYPE_???       0x13    /* 20 ???                    */
//efine SCCB_EVD_TYPE_???       0x14    /* 10 ???                    */
//efine SCCB_EVD_TYPE_???       0x15    /* 08 ???                    */
//efine SCCB_EVD_TYPE_???       0x16    /* 04 ???                    */
//efine SCCB_EVD_TYPE_???       0x17    /* 02 ???                    */
//efine SCCB_EVD_TYPE_???       0x18    /* 01 ???                    */

//efine SCCB_EVD_TYPE_???       0x19    /* 80 ???                    */
#define SCCB_EVD_TYPE_VT220     0x1A    /* 40 VT220 Msg              */
#define SCCB_EVD_TYPE_SYSG      0x1B    /* 20 3270 Msg (SYSG cons)   */
#define SCCB_EVD_TYPE_SDIAS     0x1C    /* 10 Dump retrieval         */
#define SCCB_EVD_TYPE_SIGQ      0x1D    /* 08 SigQuiesce             */
//efine SCCB_EVD_TYPE_???       0x1E    /* 04 ???                    */
//efine SCCB_EVD_TYPE_???       0x1F    /* 02 ???                    */
#define SCCB_EVD_TYPE_CPCMD     0x20    /* 01 CntlProgOpCmd          */

/*03*/  BYTE    flag;
#define SCCB_EVD_FLAG_PROC      0x80    /* Event successful          */
/*04*/  HWORD   resv;                   /* Reserved for future use   */
}
SCCB_EVD_HDR; /* size: 0x06 */

/*-------------------------------------------------------------------*/
/*              Read/Write Event Data Buffer                         */
/*-------------------------------------------------------------------*/
typedef struct _SCCB_EVD_BK
{
        HWORD   msglen;
        BYTE    const1[51];
        HWORD   cplen;                  /* CP message length         */
        BYTE    const2[24];
        HWORD   tdlen;                  /* Text Data length          */
        BYTE    const3[2];
        BYTE    sdtlen;
        BYTE    const4;                 /* Self defining tag         */
        BYTE    tmlen;
        BYTE    const5;                 /* Text Message format       */
//      BYTE    txtmsg[n];
}
SCCB_EVD_BK;

/*-------------------------------------------------------------------*/
/*              Message Control Data Block                           */
/*-------------------------------------------------------------------*/
typedef struct _SCCB_MCD_BK
{
        HWORD   length;                 /* Total length of MCD       */
        HWORD   type;                   /* Type must be 0x0001       */
        FWORD   tag;                    /* Tag must be 0xD4C4C240    */
        FWORD   revcd;                  /* Revision code 0x00000001  */
}
SCCB_MCD_BK;

/*-------------------------------------------------------------------*/
/*            Message Control Data Block Header                      */
/*-------------------------------------------------------------------*/
typedef struct _SCCB_OBJ_HDR
{
        HWORD   length;                 /* Total length of OBJ       */
        HWORD   type;                   /* Object type               */
#define SCCB_OBJ_TYPE_GENERAL   0x0001  /* General Object            */
#define SCCB_OBJ_TYPE_CPO       0x0002  /* Control Program Object    */
#define SCCB_OBJ_TYPE_NLS       0x0003  /* NLS data Object           */
#define SCCB_OBJ_TYPE_MESSAGE   0x0004  /* Message Text Object       */
}
SCCB_OBJ_HDR;

/*-------------------------------------------------------------------*/
/*       Message Control Data Block Message Text Object              */
/*-------------------------------------------------------------------*/
typedef struct _SCCB_MTO_BK
{
        BYTE    ltflag[2];              /* Line type flag            */
#define SCCB_MTO_LTFLG0_CNTL    0x80    /* Control text line         */
#define SCCB_MTO_LTFLG0_LABEL   0x40    /* Label text line           */
#define SCCB_MTO_LTFLG0_DATA    0x20    /* Data text line            */
#define SCCB_MTO_LTFLG0_END     0x10    /* Last line of message      */
#define SCCB_MTO_LTFLG0_PROMPT  0x08    /* Prompt line - response
                                           requested (WTOR)          */
#define SCCB_MTO_LTFLG0_DBCS    0x04    /* DBCS text                 */
#define SCCB_MTO_LTFLG0_MIX     0x02    /* Mixed SBCS/DBCS text      */
#define SCCB_MTO_LTFLG1_OVER    0x01    /* Foreground presentation
                                           field override            */
        BYTE    presattr[4];            /* Presentation Attribute
                                           Byte 0 - control
                                           Byte 1 - color
                                           Byte 2 - highlighting
                                           Byte 3 - intensity        */
#define SCCB_MTO_PRATTR0_ALARM  0x80    /* Sound alarm (console)     */
#define SCCB_MTO_PRATTR3_HIGH   0xE8    /* Highlighted               */
#define SCCB_MTO_PRATTR3_NORM   0xE4    /* Normal                    */
}
SCCB_MTO_BK;

/*-------------------------------------------------------------------*/
/*         Message Control Data Block General Object                 */
/*-------------------------------------------------------------------*/
typedef struct _SCCB_MGO_BK
{
        FWORD   seq;                    /* Message DOM ID            */
        BYTE    time[11];               /* C'HH.MM.SS.th'            */
        BYTE    resv1;
        BYTE    date[7];                /* C'YYYYDDD'                */
        BYTE    resv2;
        BYTE    mflag[2];               /* Message Flags             */
#define SCCB_MGO_MFLAG0_DOM     0x80    /* Delete Operator Message   */
#define SCCB_MGO_MFLAG0_ALARM   0x40    /* Sound the SCLP alarm      */
#define SCCB_MGO_MFLAG0_HOLD    0x20    /* Hold message until DOM    */
        BYTE    presattr[4];            /* Presentation Attribute
                                           Byte 0 - control
                                           Byte 1 - color
                                           Byte 2 - highlighting
                                           Byte 3 - intensity        */
#define SCCB_MGO_PRATTR0_ALARM  0x80    /* Sound alarm (console)     */
#define SCCB_MGO_PRATTR3_HIGH   0xE8    /* Highlighted               */
#define SCCB_MGO_PRATTR3_NORM   0xE4    /* Normal                    */
        BYTE    bckattr[4];             /* Background presentation
                                           attributes - covers all
                                           message-test foreground
                                           presentation attribute
                                           field overrides           */
        BYTE    sysname[8];             /* Originating system name   */
        BYTE    jobname[8];             /* Jobname or guestname      */
}
SCCB_MGO_BK;

/*-------------------------------------------------------------------*/
/*           Control Program Information Block                       */
/*-------------------------------------------------------------------*/
typedef struct _SCCB_CPI_BK
{
        BYTE    id_fmt;
        BYTE    resv0;
        BYTE    system_type[8];
        DBLWRD  resv1;
        BYTE    system_name[8];
        DBLWRD  resv2;
        DBLWRD  system_level;
        DBLWRD  resv3;
        BYTE    sysplex_name[8];
        BYTE    resv4[16];
}
SCCB_CPI_BK;

/*-------------------------------------------------------------------*/
/*          Message Control Data Block NLS Object                    */
/*-------------------------------------------------------------------*/
typedef struct _SCCB_NLS_BK
{
        HWORD   scpgid;                 /* CPGID for SBCS (def 037)  */
        HWORD   scpsgid;                /* CPSGID for SBCS (def 637) */
        HWORD   dcpgid;                 /* CPGID for DBCS (def 037)  */
        HWORD   dcpsgid;                /* CPSGID for DBCS (def 637) */
}
SCCB_NLS_BK;

/*-------------------------------------------------------------------*/
/*                 Signal Quiesce Block                              */
/*-------------------------------------------------------------------*/
typedef struct _SCCB_SGQ_BK
{
    HWORD   count;                  /* Countdown in units        */
    BYTE    unit;                   /* Unit type                 */
#define SCCB_SGQ_SEC 0
#define SCCB_SGQ_MIN 1
#define SCCB_SGQ_HR  2
}
SCCB_SGQ_BK;

/*-------------------------------------------------------------------*/
/*                   Storage Information                             */
/*-------------------------------------------------------------------*/
typedef struct _SCCB_XST_INFO
{
        HWORD   elmid;                  /* Extended storage element
                                                                id   */
        BYTE    resv1[6];
        FWORD   elmsin;                 /* Starting increment number */
        FWORD   elmein;                 /* Ending increment number   */
        BYTE    elmchar;                /* Element characteristics   */
#define SCCB_XST_INFO_ELMCHAR_REQ 0x80; /* Required element          */
        BYTE    resv2[39];
}
SCCB_XST_INFO;

/*-------------------------------------------------------------------*/
/*                      Storage Map                                  */
/*-------------------------------------------------------------------*/
typedef struct _SCCB_XST_MAP
{
        FWORD   incnum;                 /* Increment number          */
        FWORD   resv;
//      BYTE    map[];                  /* Bitmap of all usable
//                                         expanded storage blocks   */
}
SCCB_XST_MAP;

/*-------------------------------------------------------------------*/
/*               SCE DASD I/O Request Block                          */
/*-------------------------------------------------------------------*/
typedef struct _SCCB_SCEDIO_BK
{
        BYTE    flag0;
        BYTE    flag1;
#define SCCB_SCEDIO_FLG1_IOR       0x03
#define SCCB_SCEDIO_FLG1_IOV       0x04
        BYTE    flag2;
        BYTE    flag3;
#define SCCB_SCEDIO_FLG3_COMPLETE  0x80
}
SCCB_SCEDIO_BK; /* size: 0x04 */

/*-------------------------------------------------------------------*/
/*                SCE DASD I/O VVVV Block                            */
/*-------------------------------------------------------------------*/
// I wish I knew what this control block was!  ('V' = Virtual??)
typedef struct _SCCB_SCEDIO_V_BK
{
        BYTE    type;
#define SCCB_SCEDIOV_TYPE_INIT     0x00
#define SCCB_SCEDIOV_TYPE_READ     0x01
#define SCCB_SCEDIOV_TYPE_CREATE   0x02
#define SCCB_SCEDIOV_TYPE_APPEND   0x03
        BYTE    flag1;
        BYTE    flag2;
        BYTE    flag3;
        DBLWRD  seek;
        DBLWRD  ncomp;
        DBLWRD  length;
        DBLWRD  resv2;
        DBLWRD  resv3;
        DBLWRD  sto;
        BYTE    filename[256];
}
SCCB_SCEDIO_V_BK;

/*-------------------------------------------------------------------*/
/*                SCE DASD I/O RRRR Block                            */
/*-------------------------------------------------------------------*/
// I wish I knew what this control block was!  ('R' = Real??)

typedef struct _SCCB_SCEDIO_R_BK
{
        BYTE    type;
#define SCCB_SCEDIOR_TYPE_INIT     0x00
#define SCCB_SCEDIOR_TYPE_READ     0x01
        BYTE    flag1;
        BYTE    flag2;
        BYTE    flag3;

        FWORD   origin;

#define SCCB_SCEDIO_R_IMAGE_LEN    8

        BYTE    imageout[ SCCB_SCEDIO_R_IMAGE_LEN ];
        BYTE    imagein [ SCCB_SCEDIO_R_IMAGE_LEN ];
}
SCCB_SCEDIO_R_BK;

#endif /* _SERVICE_H */
