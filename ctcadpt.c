/* CTCADPT.C    (C) Copyright James A. Pierson, 2002-2012            */
/*              (C) Copyright Roger Bowler, 2000-2012                */
/*              (c) Copyright Vic Cross, 2001-2009                   */
/*              (C) Copyright Peter J. Jansen, 2014-2020             */
/*              (C) and others 2013-2021                             */
/*              Hercules Channel-to-Channel Emulation Support        */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

// Hercules Channel-to-Channel Emulation Support
// ====================================================================

// Notes:
//   This module contains the remaining CTC emulation modes that
//   have not been moved to seperate modules. There is also logic
//   to allow old style 3088 device definitions for compatibility
//   and may be removed in a future release.
//
//   Please read README.NETWORKING for more info.
//

#include "hstdinc.h"

#define _CTCADPT_C_
#define _HENGINE_DLL_

#include "hercules.h"
#include "devtype.h"
#include "ctcadpt.h"
#include "tuntap.h"
#include "opcode.h"
#include "devtype.h"

// --------------------------------------------------------------------
// The following macro's attempt to maximize source commonality between
// different Hercules versions, whilst adhering to different styles.
// --------------------------------------------------------------------

#define CTCE_DEVNUM(p)          SSID_TO_LCSS(p->ssid), p->devnum
#define CTCX_DEVNUM(p)          CTCE_DEVNUM(p)
#define CCWC                    U32              // sCount, Residual

// --------------------------------------------------------------------
// CTCE_FSM_CELL for the Finite State Machine (FSM) table cells.
// --------------------------------------------------------------------

typedef struct _CTCE_FSM_CELL {
   BYTE new_state;
   BYTE x_unit_stat;
   BYTE y_unit_stat;
   BYTE actions;
}
CTCE_FSM_CELL;

// --------------------------------------------------------------------
// CTCE_INFO also for use by CTCE Tracing when requested
// --------------------------------------------------------------------

typedef struct _CTCE_INFO
{
    CTCE_FSM_CELL      fsm;            /* Current FSM table cell     */
    BYTE               state_x_prev;   /* This  side previous state  */
    BYTE               state_y_prev;   /* Other side previous state  */
    BYTE               actions;        /* Triggered by CCW received  */
    BYTE               state_new;      /* The updated FSM state      */
    BYTE               x_unit_stat;    /* Resulting device unit stat */
    BYTE               scb;            /* Last SCB returned          */
    BYTE               sas[2];         /* Last SAS 2 SENSE bytes ret.*/
    BYTE               busy_waits;     /* Number of times waited for */
                                       /* a Busy condition to end    */
    BYTE               de_ready;       /* Device-End status          */
                                       /* indicating ready to be     */
                                       /* presented, yielding ...    */
    u_int              sent : 1;       /* = 1 : CTCE_Send done       */
    u_int              attn_can : 1;   /* = 1 : Atttention Cancelled */
    u_int              con_lost : 1;   /* = 1 : contention lost      */
    u_int              con_won  : 1;   /* = 1 : contention won       */
    u_int              sel_reset : 1;  /* = 1 : selective reset      */
    int                wait_rc;        /* CTCE_Send Wait RC if used  */
                                       /* from transition to         */
                                       /* "Working(D)" state.        */
    int                de_ready_attn_rc;   /* device_attention RC    */
    int                working_attn_rc;    /* device_attention RC    */
}
CTCE_INFO;

// --------------------------------------------------------------------
// CTCE_Cmd_Xfr enumeration type used by CTCE_Trace
// --------------------------------------------------------------------

enum CTCE_Cmd_Xfr
{
    CTCE_LCL,                          /* Cmd remains Local only     */
    CTCE_SND,                          /* Cmd Sent to y-side         */
    CTCE_RCV,                          /* Cmd Received from y-side   */
    CTCE_SND_NS,                       /* Cmd Send not possible      */
    CTCE_SND_NSR                       /* Cmd Send not poasible and  */
                                       /*     also not receiving     */
};

// --------------------------------------------------------------------
// CTCE_Sok_Use enumeration type used by CTCE_Get_Socket
// ---------------------------------------------------------------------------

enum CTCE_Sok_Use
{
    CTCE_SOK_LIS,                      /* Socket for listen()        */
    CTCE_SOK_CON                       /* Socket for connect()       */
};

// ====================================================================
// Declarations
// ====================================================================

static int      CTCT_Init( DEVBLK *dev, int argc, char *argv[] );

static void     CTCT_Read( DEVBLK* pDEVBLK,   CCWC  sCount,
                           BYTE*   pIOBuf,    BYTE* pUnitStat,
                           CCWC*   pResidual, BYTE* pMore );

static void     CTCT_Write( DEVBLK* pDEVBLK,   CCWC  sCount,
                            BYTE*   pIOBuf,    BYTE* pUnitStat,
                            CCWC*   pResidual );

static void*    CTCT_ListenThread( void* argp );

static void     CTCE_ExecuteCCW( DEVBLK* pDEVBLK, BYTE  bCode,
                                 BYTE    bFlags,  BYTE  bChained,
                                 CCWC    sCount,  BYTE  bPrevCode,
                                 int     iCCWSeq, BYTE* pIOBuf,
                                 BYTE*   pMore,   BYTE* pUnitStat,
                                 CCWC*   pResidual );

static int      CTCE_Init( DEVBLK *dev, int argc, char *argv[] );

static int      CTCE_Start_Listen_Connect_Threads( DEVBLK *dev );

static int      CTCE_Start_ConnectThread( DEVBLK *dev );

static void     CTCE_Send(        DEVBLK*             pDEVBLK,
                            const CCWC                sCount,
                                  BYTE*               pIOBuf,
                                  BYTE*               pUnitStat,
                                  CCWC*               pResidual,
                                  CTCE_INFO*          pCTCE_Info );

static void*    CTCE_RecvThread( void* argp );

static void*    CTCE_ListenThread( void* argp );

static void     CTCE_Reset( DEVBLK* pDEVBLK );

static U32      CTCE_ChkSum( const BYTE* pBuf, const U16 BufLen );

static void     CTCE_Trace(       DEVBLK*             pDEVBLK,
                            const enum CTCE_Cmd_Xfr   eCTCE_Cmd_Xfr,
                            const CTCE_INFO*          pCTCE_Info,
                            const BYTE*               pUnitStat );

static void*    CTCE_ConnectThread( void* argp );

static int      CTCE_Get_Socket( DEVBLK*                   dev,
                                 const enum CTCE_Sok_Use   eCTCE_Sok_Use );

static int      CTCE_Write_Init( DEVBLK*                   dev,
                                 const int                 fd );

static int      CTCE_Recovery( DEVBLK*                     dev );

static int      CTCE_Build_RCD(  DEVBLK*                   dev,
                                 BYTE*                     buffer,
                                 int                       bufsz );

// --------------------------------------------------------------------
// Definitions for CTC general data blocks
// --------------------------------------------------------------------

typedef struct _CTCG_PARMBLK
{
    int                 listenfd;
    struct sockaddr_in  addr;
    DEVBLK*             dev;
}
CTCG_PARMBLK;

// --------------------------------------------------------------------
// CTCE Send-Receive Socket Prefix at the start of the DEVBLK buf
// --------------------------------------------------------------------

typedef struct _CTCE_SOKPFX
{
    union
    {
        struct
        {
            BYTE        CmdReg;        /* CTCE command register      */
            BYTE        FsmSta;        /* CTCE FSM state             */
            U16         sCount;        /* CTCE sCount copy           */
            U16         PktSeq;        /* CTCE Packet Sequence ID    */
        };
        struct                         /* Overlay used on 1st R/W by */
        {                              /* Hercules (NOT guest OS) :  */
            U16         ctce_herc;     /* CTCE other (y-)side info   */
            U16         ctce_lport;    /* CTCE y-side listening port */
            struct in_addr
                        ctce_ipaddr;   /* CTCE our ipaddr for y-side */
        };
    };
    U16                 SndLen;        /* CTCE Packet Sent Length    */
    U16                 devnum;        /* CTCE Sender's devnum       */
    U16                 ssid;          /* CTCE Sender's ssid         */
}
CTCE_SOKPFX;

// --------------------------------------------------------------------
// CTCE Equivalent of CTCG_PARMBLK (used to pass thread arguments)
// --------------------------------------------------------------------

typedef struct _CTCE_PARMBLK
{
    int                 fd;            /* socket file descriptor     */
    struct sockaddr_in  addr;
    DEVBLK*             dev;
}
CTCE_PARMBLK;

// --------------------------------------------------------------------
// CTCE Constants (generated by a small REXX script)
// --------------------------------------------------------------------

#define CTCE_PREPARE                0
#define CTCE_CONTROL                1
#define CTCE_READ                   2
#define CTCE_WRITE                  3
#define CTCE_SENSE_COMMAND_BYTE     4
#define CTCE_READ_BACKWARD          6
#define CTCE_WRITE_END_OF_FILE      7
#define CTCE_NO_OPERATION           8
#define CTCE_SET_EXTENDED_MODE      9
#define CTCE_SENSE_ADAPTER_STATE    10
#define CTCE_SENSE_ID               11
#define CTCE_READ_CONFIG_DATA       12
#define CTCE_SET_BASIC_MODE         15

static char *CTCE_CmdStr[16] = {
    "PRE" , //  0 = 00 = Prepare
    "CTL" , //  1 = 01 = Control
    "RED" , //  2 = 02 = Read
    "WRT" , //  3 = 03 = Write
    "SCB" , //  4 = 04 = Sense Command Byte
    "???" , //  5 = 05 = Not Used
    "RBK" , //  6 = 06 = Read Backward
    "WEF" , //  7 = 07 = Write End Of File
    "NOP" , //  8 = 10 = No Operation
    "SEM" , //  9 = 11 = Set Extended Mode
    "SAS" , // 10 = 12 = Sense Adapter State
    "SID" , // 11 = 13 = Sense ID
    "RCD" , // 12 = 14 = Read Configuration Data
    "INV" , // 13 = 15 = Invalid Command Code
    "RST" , // 14 = 16 = Invalid Command Code Used to Report SCB 0 after a Reset
    "SBM"   // 15 = 17 = Set Basic Mode
};

static BYTE CTCE_command[256] = {
    14, 3, 2, 8,10, 3, 2, 1,13, 3, 2, 8, 6, 3, 2, 1,
    13, 3, 2, 8, 4, 3, 2, 1,13, 3, 2, 8, 6, 3, 2, 1,
    13, 3, 2, 8,13, 3, 2, 1,13, 3, 2, 8, 6, 3, 2, 1,
    13, 3, 2, 8, 4, 3, 2, 1,13, 3, 2, 8, 6, 3, 2, 1,
    13, 3, 2,15,13, 3, 2, 1,13, 3, 2,15, 6, 3, 2, 1,
    13, 3, 2,15, 4, 3, 2, 1,13, 3, 2,15, 6, 3, 2, 1,
    13, 3, 2,15,13, 3, 2, 1,13, 3, 2,15, 6, 3, 2, 1,
    13, 3, 2,15, 4, 3, 2, 1,13, 3, 2,15, 6, 3, 2, 1,
    13, 7, 2, 8,13, 7, 2, 1,13, 7, 2, 8, 6, 7, 2, 1,
    13, 7, 2, 8, 4, 7, 2, 1,13, 7, 2, 8, 6, 7, 2, 1,
    13, 7, 2, 8,13, 7, 2, 1,13, 7, 2, 8, 6, 7, 2, 1,
    13, 7, 2, 8, 4, 7, 2, 1,13, 7, 2, 8, 6, 7, 2, 1,
    13, 7, 2, 9,12, 7, 2, 1,13, 7, 2,13, 6, 7, 2, 1,
    13, 7, 2,13, 4, 7, 2, 1,13, 7, 2,13, 6, 7, 2, 1,
    13, 7, 2, 0,11, 7, 2, 1,13, 7, 2,13, 6, 7, 2, 1,
    13, 7, 2,13, 4, 7, 2, 1,13, 7, 2,13, 6, 7, 2, 1
};

/* In base (non-extended) mode the WEOF (WEF) */
/* command does not exist but classifies as   */
/* a regular WRITE command.  The WEOF-to-WRT  */
/* mapping is performed with this macro:      */

/* fine CTCE_CMD(c)             (pDEVBLK->ctcxmode == 1 ?   (CTCE_command[c]) : \ */
#define CTCE_CMD( c )           ( pDEVBLK->ctcxmode || pDEVBLK->ctce_remote_xmode ?  (CTCE_command[c]) : \
                                ((CTCE_command[c])==7 ? 3 : (CTCE_command[c])))

#define IS_CTCE_CCW_PRE(c)      ((CTCE_command[c]==0))
#define IS_CTCE_CCW_CTL(c)      ((CTCE_command[c]==1))
#define IS_CTCE_CCW_RED(c)      ((CTCE_command[c]==2))
#define IS_CTCE_CCW_WRT(c)      ((CTCE_CMD( c) ==3))
#define IS_CTCE_CCW_SCB(c)      ((CTCE_command[c]==4))
#define IS_CTCE_CCW_RBK(c)      ((CTCE_command[c]==6))
#define IS_CTCE_CCW_WEF(c)      ((CTCE_CMD( c )==7))
#define IS_CTCE_CCW_NOP(c)      ((CTCE_command[c]==8))
#define IS_CTCE_CCW_SEM(c)      ((CTCE_command[c]==9))
#define IS_CTCE_CCW_SBM(c)      ((CTCE_command[c]==15))
#define IS_CTCE_CCW_SAS(c)      ((CTCE_command[c]==10))
#define IS_CTCE_CCW_SID(c)      ((CTCE_command[c]==11))
#define IS_CTCE_CCW_RCD(c)      ((CTCE_command[c]==12))
#define IS_CTCE_CCW_DEP(c)      ((CTCE_CMD( c )<7))           /* Any Dependent Command */
#define IS_CTCE_CCW_RDA(c)      (((CTCE_command[c]&0xFB)==2)) /* Read or Read Backward */
#define IS_CTCE_CCW_WRA(c)      (((CTCE_command[c]&0xFB)==3)) /* Write or Write EOF    */

/* CTCE devices can be Reset (RST), not with  */
/* a CCW command, but via the device handler  */
/* CTCE_Halt, which gets called following a   */
/* HSCH or CSCH instruction.  It is encoded   */
/* via a synthetic CCW command "bCode_reset"  */
/* which is 0x00 as an RST needs to result in */
/* a zero y command register setting.         */
/* A zero CCW input to CTCE_ExecuteCCW gets   */
/* converted to a CCW of 8 which is invalid.  */
/* Only when received from the other (y-)side */
/* will a zero CCW be accepted, as an RST.    */
#define IS_CTCE_RST(c)          ((CTCE_command[c]==14))
#define bCode_reset   (0x00)
#define bCode_invalid (0x08)

/* Macros for classifying CTC states follow.  */
/* These are numbered 0 thru 7 as per the     */
/* column numbers 0-3 and 4-7 in the table    */
/* in section 2.13 in SA22-7203-00 by IBM,    */
/* which is (alomost) the same as the table   */
/* in section 3.15 in SA22-7901-01 by IBM.    */
/*                                            */
/* But in base (non-extended) mode, the table */
/* in section 2.13 in SA77-7901-01 applies,   */
/* omitting column 5 for the Not-Ready state: */
/* base (non-extended) mode considers this    */
/* the same as Available.  We perform this    */
/* Base-Not-Ready mapping into Available with */
/* this macro:                                */

/* fine CTCE_STATE(c)           (pDEVBLK->ctcxmode == 1 ?  ((c)&0x07) : \ */
#define CTCE_STATE( c )         ( pDEVBLK->ctcxmode || pDEVBLK->ctce_remote_xmode ?  ((c)&0x07) : \
                                (((c)&0x07)==0x05 ? 0x04 : ((c)&0x07)))

#define IS_CTCE_YWP(c)          (((c)&0x07)==0x00)
#define IS_CTCE_YWC(c)          (((c)&0x07)==0x01)
#define IS_CTCE_YWR(c)          (((c)&0x07)==0x02)
#define IS_CTCE_YWW(c)          (((c)&0x07)==0x03)
#define IS_CTCE_YAV(c)          ((CTCE_STATE(c))==0x04)
#define IS_CTCE_YNR(c)          ((CTCE_STATE(c))==0x05)

/* These two are useful combinations :        */
/* - The 0 (YWP) or 4 (YAV) states READY      */
#define IS_CTCE_YAP(c)          (((CTCE_STATE(c))&0x03)==0x00)
/* - Any Y working state: YWP, YWC, YWR or YWW */
#define IS_CTCE_YWK(c)          (((c)&0x04)==0x00)
/* - Any of the states Cntl, Read, or Write   */
#define IS_CTCE_CRW(c)         ((((c)&0x04)==0x00) && (((c)&0x07)!=0x00))

/* A special one is "X available" (XAV) which */
/* includes the not ready state.              */
#define IS_CTCE_XAV(c)          (((c)<6))

/* Useful SET macros for the above.           */
#define SET_CTCE_YAV(c)         (c=(((c)&0xF8)|0x04))
#define SET_CTCE_YNR(c)         (c=(((c)&0xF8)|0x05))

/* One letter CTC state abbreviations         */
static char *CTCE_StaStr[8] = {"P", "C", "R", "W", "A", "N", "X", "I"};

/* The CTCE CCW command will trigger actions  */
/* which are dependent on the CTCE state.     */
/* These different action flags are :         */
#define CTCE_WEOF               (0x80)
#define CTCE_SEND               (0x40)
#define CTCE_WAIT               (0x20)
#define CTCE_ATTN               (0x10)
#define CTCE_MATCH              (0x08)

/* Corresponding macros to test for these     */
#define IS_CTCE_WEOF(c)         (((c)&CTCE_WEOF)==CTCE_WEOF)
#define IS_CTCE_SEND(c)         (((c)&CTCE_SEND)==CTCE_SEND)
#define IS_CTCE_WAIT(c)         (((c)&CTCE_WAIT)==CTCE_WAIT)
#define IS_CTCE_ATTN(c)         (((c)&CTCE_ATTN)==CTCE_ATTN)
#define IS_CTCE_MATCH(c)        (((c)&CTCE_MATCH)==CTCE_MATCH)

/* And the corresponding SET macros for these */
#define SET_CTCE_WEOF(c)        (c|=CTCE_WEOF)
#define SET_CTCE_SEND(c)        (c|=CTCE_SEND)
#define SET_CTCE_WAIT(c)        (c|=CTCE_WAIT)
#define SET_CTCE_ATTN(c)        (c|=CTCE_ATTN)
#define SET_CTCE_MATCH(c)       (c|=CTCE_MATCH)

/* And the corresponding CLeaR macros         */
#define CLR_CTCE_WEOF(c)        (c&=~CTCE_WEOF)
#define CLR_CTCE_SEND(c)        (c&=~CTCE_SEND)
#define CLR_CTCE_WAIT(c)        (c&=~CTCE_WAIT)
#define CLR_CTCE_ATTN(c)        (c&=~CTCE_ATTN)
#define CLR_CTCE_MATCH(c)       (c&=~CTCE_MATCH)

/* To CLeaR all flags                         */
#define CLR_CTCE_ALLF(c)        (c&=~CTCE_WEOF)

/* Enhanced CTC processing is selected by     */
/* omitting default MTU bufsize CTCE_MTU_MIN, */
/* or by specifying a larger number.  The     */
/* default is equal to 62552, calculated as   */
/*    sizeof(CTCE_SOKPFX==16) +               */
/*    sizeof(U16=pSokBuf->sCount==2) +        */
/*    62534 (==0xF446)                        */
/* the latter number is the largest data      */
/* sCount seen used by  CTC programs to date. */
/* If that number would be too small one day, */
/* a severe error message will instruct the   */
/* user to specify an increased MTU bufsize   */
/* in the device configuration statement.     */
#define CTCE_MTU_MIN ( (int)( 62534 + sizeof(CTCE_SOKPFX) + sizeof(U16 /* sCount */) ) )

#define CTCE_RESET_TYPE                                                                                             \
      (     ( pDEVBLK->scsw.flag2 & ( SCSW2_FC_HALT                  ) ) )                ? "Halt Reset"      : ""  \
    , (     ( pDEVBLK->scsw.flag2 & (                 SCSW2_FC_CLEAR ) ) )                ? "Clear Reset"     : ""  \
    , ( ( ! ( pDEVBLK->scsw.flag2 & ( SCSW2_FC_HALT | SCSW2_FC_CLEAR ) ) ) &&                                       \
          ! ( IS_CTCE_CRW( pDEVBLK->ctcexState ) | IS_CTCE_CRW( pDEVBLK->ctceyState ) ) ) ? "System Reset"    : ""  \
    , ( ( ! ( pDEVBLK->scsw.flag2 & ( SCSW2_FC_HALT | SCSW2_FC_CLEAR ) ) ) &&                                       \
            ( IS_CTCE_CRW( pDEVBLK->ctcexState ) | IS_CTCE_CRW( pDEVBLK->ctceyState ) ) ) ? "Selective Reset" : ""

/**********************************************************************/
/* A summary of the Channel-to-Channel command operations this CTCE   */
/* device emulates can be found in IBM publications SA22-7203-00 in   */
/* section 2.13, and in SA22-7091-01 sections 2.13 and 3.15.  The     */
/* tables show the device states of both sides, and the influence of  */
/* CCW commands depending on this state.  Our CTCE implemention is    */
/* assisted by a Finite State Machine (FSM) table closely matching    */
/* the figures in these prublications.                                */
/*                                                                    */
/* Eeach CTCE side is in a given state at any point in time, which    */
/* corresponds to the columns in the FSM table, matching columns 0    */
/* through 7 in the publications mentionned.  Each CCW command has a  */
/* row in the FSM table.  A CCW command received will (1) trigger a   */
/* transition to a new_state, (2) cause a Unit Status update, and (3) */
/* cause a number of actions to be carried out.                       */
/*                                                                    */
/* The FSM table coding is assisted with macro's for the state each   */
/* CTCE side (x=local, y=remote) can have, matching the FSM column    */
/* column numbers 0-7: Prepare, Control, Read, Write, Available,      */
/* Not-ready, X-working (=P/C/R/W) or Int-pending, all represented by */
/* a single letter: P, C, R, W, A, N, X, I.  Additionally, the CTCE   */
/* FSM table uses U for Unchanged to cover the case of no state       */
/* change whatsoever, e.g. for CCW commands SAS, SID, RCD and others. */
/* Please see macro's CTCE_NEW_X_STATE & CTCE_NEW_Y_STATE down below. */
/**********************************************************************/
#define P    0
#define C    1
#define R    2
#define W    3
#define A    4
#define N    5
#define X    6
#define I    7
#define U  255

/**********************************************************************/
/* Each CTCE FSM table entry contains a macro up to 7 letters long:   */
/*                                                                    */
/*      +---------- new_state = P, C, R, W, A or U                    */
/*      |++-------- Unit Status bits encoded with up to two letters:  */
/*      |||         . UC = Unit Check                                 */
/*      |||         . C_ = CE                                         */
/*      |||         . E_ = CE + DE                                    */
/*      |||         . EU = CD + CE + UC                               */
/*      |||         . BA = BUSY + ATTN                                */
/*      |||         . B  = BUSY                                       */
/*      |||+------- S = Send this commands also to the other (y-)side */
/*      ||||+------ M = a Matching command for the other (y-)side     */
/*      |||||+----- W = our (x-)side must Wait for a matching command */
/*      ||||||+---- A = cause Attention interrupt at the other y-side */
/*      |||||||                                                       */
#define PC_S_W  { P, CSW_CE                    , 0, CTCE_SEND              | CTCE_WAIT             }
#define C__S_WA { C, 0                         , 0, CTCE_SEND              | CTCE_WAIT | CTCE_ATTN }
#define R__S_WA { R, 0                         , 0, CTCE_SEND              | CTCE_WAIT | CTCE_ATTN }
#define W__S_WA { W, 0                         , 0, CTCE_SEND              | CTCE_WAIT | CTCE_ATTN }
#define CC_SMW  { C, CSW_CE                    , 0, CTCE_SEND | CTCE_MATCH | CTCE_WAIT             }
#define R__SMW  { R, 0                         , 0, CTCE_SEND | CTCE_MATCH | CTCE_WAIT             }
#define W__SMW  { W, 0                         , 0, CTCE_SEND | CTCE_MATCH | CTCE_WAIT             }
#define AE_SM   { A, CSW_CE   | CSW_DE         , 0, CTCE_SEND | CTCE_MATCH                         }
#define NEUSM   { N, CSW_CE   | CSW_DE | CSW_UC, 0, CTCE_SEND | CTCE_MATCH                         }
#define NEUS    { N, CSW_CE   | CSW_DE | CSW_UC, 0, CTCE_SEND                                      }
#define AE_S    { A, CSW_CE   | CSW_DE         , 0, CTCE_SEND                                      }
#define AUCS    { A,                     CSW_UC, 0, CTCE_SEND                                      }
#define  E_S    { U, CSW_CE   | CSW_DE         , 0, CTCE_SEND                                      }
#define  E_     { U, CSW_CE   | CSW_DE         , 0, 0                                              }
#define  B      { U, CSW_BUSY                  , 0, 0                                              }
#define  BA     { U, CSW_BUSY | CSW_ATTN       , 0, 0                                              }
#define  UC     { U,                     CSW_UC, 0, 0                                              }
#define  UCS    { U,                     CSW_UC, 0, CTCE_SEND                                      }
#define N__S    { N, 0                         , 0, CTCE_SEND                                      }

/**********************************************************************/
/* Now finally the CTCE FSM table:                                    */
/**********************************************************************/
static const CTCE_FSM_CELL CTCE_Fsm[16][8] = {

/* cmd/stat P       C       R       W       A       N       X       I     */
/* PRE */ {AE_SM  , E_    , E_    , E_    ,PC_S_W ,AUCS   , B     , B     },
/* CTL */ {CC_SMW , BA    , BA    , BA    ,C__S_WA,AUCS   , B     , B     },
/* RED */ {R__SMW , BA    , BA    ,AE_SM  ,R__S_WA,AUCS   , B     , B     },
/* WRT */ {W__SMW , BA    ,AE_SM  , BA    ,W__S_WA,AUCS   , B     , B     },
/* SCB */ { E_    ,AE_SM  , E_    , E_    , E_    ,AUCS   , B     , B     },
/* nus */ { UC    , UC    , UC    , UC    , UC    , UC    , B     , B     },
/* RBK */ {R__SMW , BA    , BA    ,AE_SM  ,R__S_WA,AUCS   , B     , B     },
/* WEF */ { E_S   , BA    ,AE_SM  , BA    , E_S   ,AUCS   , B     , B     },
/* NOP */ { E_S   , BA    , BA    , BA    ,AE_S   ,AUCS   , B     , B     },
/* SEM */ { E_S   , BA    , BA    , BA    ,AE_S   ,AUCS   , B     , B     },

/* SAS */ { E_    , E_    , E_    , E_    , E_    , E_    , B     , B     },
/* SID */ { E_    , E_    , E_    , E_    , E_    , E_    , B     , B     },
/* RCD */ { E_    , E_    , E_    , E_    , E_    , E_    , B     , B     },

/* inv */ { UC    , UC    , UC    , UC    , UC    , UC    , B     , B     },
/* RST */ {NEUSM  ,NEUSM  ,NEUSM  ,NEUSM  ,N__S   ,N__S   ,NEUS   ,NEUS   },
/* SBM */ { E_S   , BA    , BA    , BA    ,AE_S   ,AUCS   , B     , B     }
};

#undef P
#undef C
#undef R
#undef W
#undef A
#undef N
#undef X
#undef I
#undef U

#undef PC_S_W
#undef C__S_WA
#undef R__S_WA
#undef W__S_WA
#undef CC_SMW
#undef R__SMW
#undef W__SMW
#undef AE_SM
#undef NEUSM
#undef NEU
#undef ACDS
#undef AUCS
#undef  E_S
#undef  E_
#undef  B
#undef  BA
#undef  UC
#undef  UCS
#undef N__S

#define CTCE_ACTIONS_PRT(s)     IS_CTCE_WEOF(s)  ? " WEOF"  : ""  \
                              , IS_CTCE_WAIT(s)  ? " WAIT"  : ""  \
                              , IS_CTCE_MATCH(s) ? " MATCH" : ""  \
                              , IS_CTCE_ATTN(s)  ? " ATTN"  : ""

#define CTCE_X_STATE_FSM_IDX                                                \
    ( ( ( pDEVBLK->ctcexState & 0x04 ) == 0x00 ) ? 0x06 : CTCE_STATE( pDEVBLK->ctceyState ) )

#define CTCE_Y_STATE_FSM_IDX                                                \
    ( ( ( pDEVBLK->ctceyState & 0x04 ) == 0x00 ) ? 0x06 : CTCE_STATE( pDEVBLK->ctcexState ) )

#define CTCE_NEW_X_STATE(c)                                                 \
    ( ( CTCE_Fsm[CTCE_CMD( c )][CTCE_X_STATE_FSM_IDX].new_state != 255 ) ?  \
      ( CTCE_Fsm[CTCE_CMD( c )][CTCE_X_STATE_FSM_IDX].new_state )        :  \
      ( pDEVBLK->ctcexState & 0x07 ) )

#define CTCE_NEW_Y_STATE(c)                                                 \
    ( ( CTCE_Fsm[CTCE_CMD( c )][CTCE_Y_STATE_FSM_IDX].new_state != 255 ) ?  \
      ( CTCE_Fsm[CTCE_CMD( c )][CTCE_Y_STATE_FSM_IDX].new_state )        :  \
      ( pDEVBLK->ctceyState & 0x07 ) )

#define CTCE_DISABLE_NAGLE

#define CTCE_DEFAULT_RPORT       ( 3088 )
#define CTCE_DEFAULT_LISTEN_PORT ( 3088 )

/* CTCE CCW tracing shows all CTC CCW commands as well as Reset actions  */
/* (which are processed as synthetic commands) on the local (x-)side, as */
/* well as received from the remote (y-)side of the CTC connection.      */
/* The Hercules generic CCW tracing (t+ and t- commands) includes this.  */
/* The "ctc debug { on | off | startup } <devnum>" comamnd can be used   */
/* to limit the tracing to the CTC commands avoiding the channel traces. */
/* The startup option produces CTCE command traces during startup only,  */
/* disabling itself after a limited number of CTC commands, or after a   */
/* matching read / write command pair.  Unexpected errors (re-)trigger   */
/* the limited startup trace automatically in any case.                  */
/* This temporary CTCE CCW tracing is handy for debugging CTCE startup,  */
/* or intermittent connectivity problems which may re-trigger startup.   */
/* Permanent tracing is when dev->ctce_trace_cntr == CTCE_TRACE_ON, and  */
/* dev->ctce_trace_cntr > 0 causes temporary tracing until the counter   */
/* is decremented reaching zero.                                         */
#define CTCE_CCWTRACE( dev )                                                \
        ( dev->ccwtrace ||                                                  \
        ( dev->ctce_trace_cntr == CTCE_TRACE_ON  ) ||                       \
        ( ( dev->ctce_trace_cntr >  0 ) && ( dev->ctce_trace_cntr-- ) ) )
#define CTCE_RESTART_CCWTRACE( dev )     ( ( dev->ctce_trace_cntr >= 0 ) && \
        ( dev->ctce_trace_cntr = CTCE_TRACE_STARTUP ) )
#define CTCE_ERROR_CCWTRACE( dev )                                          \
        ( dev->ctce_trace_cntr = CTCE_TRACE_STARTUP )

/* CTCE_HERC_... items are communicated in ctce_herc in the first send   */
/* following a socket connect().  It contains Hercules and NOT guest OS  */
/* status information.                                                   */
#define CTCE_HERC_ONLY          ( 0x8000 )
#define CTCE_HERC_RECV          ( 0x8001 )


/**********************************************************************/
/* This table is used by channel.c to determine if a CCW code is an   */
/* immediate command or not                                           */
/* The table is addressed in the DEVHND structure as 'DEVIMM immed'   */
/* 0 : Command is NOT an immediate command                            */
/* 1 : Command is an immediate command                                */
/* Note : An immediate command is defined as a command which returns  */
/* CE (channel end) during initialisation (that is, no data is        */
/* actually transfered). In this case, IL is not indicated for a CCW  */
/* Format 0 or for a CCW Format 1 when IL Suppression Mode is in      */
/* effect                                                             */
/**********************************************************************/

static BYTE CTCE_immed_commands[256] =
{
/* 0 1 2 3 4 5 6 7 8 9 A B C D E F */
   0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1, /* 0x */
   0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1, /* 1x */
   0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1, /* 2x */
   0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1, /* 3x */
   0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1, /* 4x */
   0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1, /* 5x */
   0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1, /* 6x */
   0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1, /* 7x */
   0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1, /* 8x */
   0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1, /* 9x */
   0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1, /* Ax */
   0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1, /* Bx */
   0,0,0,1,0,0,0,1,0,0,0,0,0,0,0,1, /* Cx */
   0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1, /* Dx */
   0,0,0,1,0,0,0,1,0,0,0,0,0,0,0,1, /* Ex */
   0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1  /* Fx */
};

//  X0XX X011  No Operation
//  MMMM M111  Control
//  1100 0011  Set Extended Mode
//  10XX X011  Set Basic Mode
//  1110 0011  Prepare
//  1XXX XX01  Write EOF (but not treated as such !)

// --------------------------------------------------------------------
// Device Handler Information Block
// --------------------------------------------------------------------

DEVHND ctct_device_hndinfo =
{
        &CTCT_Init,                    /* Device Initialization      */
        &CTCX_ExecuteCCW,              /* Device CCW execute         */
        &CTCX_Close,                   /* Device Close               */
        &CTCX_Query,                   /* Device Query               */
        NULL,                          /* Device Extended Query      */
        NULL,                          /* Device Start channel pgm   */
        NULL,                          /* Device End channel pgm     */
        NULL,                          /* Device Resume channel pgm  */
        NULL,                          /* Device Suspend channel pgm */
        NULL,                          /* Device Halt channel pgm    */
        NULL,                          /* Device Read                */
        NULL,                          /* Device Write               */
        NULL,                          /* Device Query used          */
        NULL,                          /* Device Reserve             */
        NULL,                          /* Device Release             */
        NULL,                          /* Device Attention           */
        NULL,                          /* Immediate CCW Codes        */
        NULL,                          /* Signal Adapter Input       */
        NULL,                          /* Signal Adapter Output      */
        NULL,                          /* Signal Adapter Sync        */
        NULL,                          /* Signal Adapter Output Mult */
        NULL,                          /* QDIO subsys desc           */
        NULL,                          /* QDIO set subchan ind       */
        NULL,                          /* Hercules suspend           */
        NULL                           /* Hercules resume            */
};

DEVHND ctce_device_hndinfo =
{
        &CTCE_Init,                    /* Device Initialization      */
        &CTCE_ExecuteCCW,              /* Device CCW execute         */
        &CTCE_Close,                   /* Device Close               */
        &CTCE_Query,                   /* Device Query               */
        NULL,                          /* Device Extended Query      */
        NULL,                          /* Device Start channel pgm   */
        NULL,                          /* Device End channel pgm     */
        NULL,                          /* Device Resume channel pgm  */
        NULL,                          /* Device Suspend channel pgm */
        &CTCE_Reset,                   /* Device Halt channel pgm    */
        NULL,                          /* Device Read                */
        NULL,                          /* Device Write               */
        NULL,                          /* Device Query used          */
        NULL,                          /* Device Reserve             */
        NULL,                          /* Device Release             */
        NULL,                          /* Device Attention           */
        CTCE_immed_commands,           /* Immediate CCW Codes        */
        NULL,                          /* Signal Adapter Input       */
        NULL,                          /* Signal Adapter Output      */
        NULL,                          /* Signal Adapter Sync        */
        NULL,                          /* Signal Adapter Output Mult */
        NULL,                          /* QDIO subsys desc           */
        NULL,                          /* QDIO set subchan ind       */
        NULL,                          /* Hercules suspend           */
        NULL                           /* Hercules resume            */
};

extern DEVHND ctci_device_hndinfo;
extern DEVHND lcs_device_hndinfo;

// ====================================================================
// Primary Module Entry Points
// ====================================================================

// --------------------------------------------------------------------
// Device Initialization Handler (Generic)
// --------------------------------------------------------------------

// -------------------------------------------------------------------
// Query the device definition (Generic)
// -------------------------------------------------------------------

void  CTCX_Query( DEVBLK* pDEVBLK,
                  char**  ppszClass,
                  int     iBufLen,
                  char*   pBuffer )
{
    char  filename[ PATH_MAX + 1 ];     /* full path or just name    */

    BEGIN_DEVICE_CLASS_QUERY( "CTCA", pDEVBLK, ppszClass, iBufLen, pBuffer );

    snprintf( pBuffer, iBufLen, "%s IO[%"PRIu64"]", filename, pDEVBLK->excps );
}

// -------------------------------------------------------------------
// Close the device (Generic)
// -------------------------------------------------------------------

int  CTCX_Close( DEVBLK* pDEVBLK )
{

    // Close the device file (if not already closed)
    if( pDEVBLK->fd >= 0 )
    {
        if (socket_is_socket( pDEVBLK->fd ))
            close_socket( pDEVBLK->fd );
        else
            close( pDEVBLK->fd );
        pDEVBLK->fd = -1;           // indicate we're now closed
    }
    return 0;
}

// -------------------------------------------------------------------
// Execute a Channel Command Word (Generic)
// -------------------------------------------------------------------

void  CTCX_ExecuteCCW( DEVBLK* pDEVBLK, BYTE  bCode,
                       BYTE    bFlags,  BYTE  bChained,
                       CCWC    sCount,  BYTE  bPrevCode,
                       int     iCCWSeq, BYTE* pIOBuf,
                       BYTE*   pMore,   BYTE* pUnitStat,
                       CCWC*   pResidual )
{
    int             iNum;               // Number of bytes to move
    BYTE            bOpCode;            // CCW opcode with modifier
                                        //   bits masked off

    UNREFERENCED( bFlags    );
    UNREFERENCED( bChained  );
    UNREFERENCED( bPrevCode );
    UNREFERENCED( iCCWSeq   );

    // Intervention required if the device file is not open
    if( pDEVBLK->fd < 0 &&
        !IS_CCW_SENSE( bCode ) &&
        !IS_CCW_CONTROL( bCode ) )
    {
        pDEVBLK->sense[0] = SENSE_IR;
        *pUnitStat = CSW_CE | CSW_DE | CSW_UC;
        return;
    }

    // Mask off the modifier bits in the CCW bOpCode
    if( ( bCode & 0x07 ) == 0x07 )
        bOpCode = 0x07;
    else if( ( bCode & 0x03 ) == 0x02 )
        bOpCode = 0x02;
    else if( ( bCode & 0x0F ) == 0x0C )
        bOpCode = 0x0C;
    else if( ( bCode & 0x03 ) == 0x01 )
        bOpCode = pDEVBLK->ctcxmode ? ( bCode & 0x83 ) : 0x01;
    else if( ( bCode & 0x1F ) == 0x14 )
        bOpCode = 0x14;
    else if( ( bCode & 0x47 ) == 0x03 )
        bOpCode = 0x03;
    else if( ( bCode & 0xC7 ) == 0x43 )
        bOpCode = 0x43;
    else
        bOpCode = bCode;

    // Process depending on CCW bOpCode
    switch (bOpCode)
    {
    case 0x01:  // 0MMMMM01  WRITE
        //------------------------------------------------------------
        // WRITE
        //------------------------------------------------------------

        // Return normal status if CCW count is zero
        if( sCount == 0 )
        {
            *pUnitStat = CSW_CE | CSW_DE;
            break;
        }

        // Write data and set unit status and residual byte count
        switch( pDEVBLK->ctctype )
        {
        case CTC_CTCT:
            CTCT_Write( pDEVBLK, sCount, pIOBuf, pUnitStat, pResidual );
            break;
        }
        break;

    case 0x81:  // 1MMMMM01  WEOF
        //------------------------------------------------------------
        // WRITE EOF
        //------------------------------------------------------------

        // Return normal status
        *pUnitStat = CSW_CE | CSW_DE;
        break;

    case 0x02:  // MMMMMM10  READ
    case 0x0C:  // MMMM1100  RDBACK
        // -----------------------------------------------------------
        // READ & READ BACKWARDS
        // -----------------------------------------------------------

        // Read data and set unit status and residual byte count
        switch( pDEVBLK->ctctype )
        {
        case CTC_CTCT:
            CTCT_Read( pDEVBLK, sCount, pIOBuf, pUnitStat, pResidual, pMore );
            break;
        }
        break;

    case 0x07:  // MMMMM111  CTL
        // -----------------------------------------------------------
        // CONTROL
        // -----------------------------------------------------------

        *pUnitStat = CSW_CE | CSW_DE;
        break;

    case 0x03:  // M0MMM011  NOP
        // -----------------------------------------------------------
        // CONTROL NO-OPERATON
        // -----------------------------------------------------------

        *pUnitStat = CSW_CE | CSW_DE;
        break;

    case 0x43:  // 00XXX011  SBM
        // -----------------------------------------------------------
        // SET BASIC MODE
        // -----------------------------------------------------------

        // Command reject if in basic mode
        if( pDEVBLK->ctcxmode == 0 )
        {
            pDEVBLK->sense[0] = SENSE_CR;
            *pUnitStat        = CSW_CE | CSW_DE | CSW_UC;

            break;
        }

        // Reset extended mode and return normal status
        pDEVBLK->ctcxmode = 0;

        *pResidual = 0;
        *pUnitStat = CSW_CE | CSW_DE;

        break;

    case 0xC3:  // 11000011  SEM
        // -----------------------------------------------------------
        // SET EXTENDED MODE
        // -----------------------------------------------------------

        pDEVBLK->ctcxmode = 1;

        *pResidual = 0;
        *pUnitStat = CSW_CE | CSW_DE;

        break;

    case 0xE3:  // 11100011
        // -----------------------------------------------------------
        // PREPARE (PREP)
        // -----------------------------------------------------------

        *pUnitStat = CSW_CE | CSW_DE;

        break;

    case 0x14:  // XXX10100  SCB
        // -----------------------------------------------------------
        // SENSE COMMAND BYTE
        // -----------------------------------------------------------

        *pUnitStat = CSW_CE | CSW_DE;
        break;

    case 0x04:  // 00000100  SENSE
      // -----------------------------------------------------------
      // SENSE
      // -----------------------------------------------------------

        // Command reject if in basic mode
        if( pDEVBLK->ctcxmode == 0 )
        {
            pDEVBLK->sense[0] = SENSE_CR;
            *pUnitStat        = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        // Calculate residual byte count
        iNum = ( sCount < pDEVBLK->numsense ) ?
            sCount : pDEVBLK->numsense;

        *pResidual = sCount - iNum;

        if( sCount < pDEVBLK->numsense )
            *pMore = 1;

        // Copy device sense bytes to channel I/O buffer
        memcpy( pIOBuf, pDEVBLK->sense, iNum );

        // Clear the device sense bytes
        memset( pDEVBLK->sense, 0, sizeof( pDEVBLK->sense ) );

        // Return unit status
        *pUnitStat = CSW_CE | CSW_DE;

        break;

    case 0xE4:  //  11100100  SID
        // -----------------------------------------------------------
        // SENSE ID
        // -----------------------------------------------------------

        // Calculate residual byte count
        iNum = ( sCount < pDEVBLK->numdevid ) ?
            sCount : pDEVBLK->numdevid;

        *pResidual = sCount - iNum;

        if( sCount < pDEVBLK->numdevid )
            *pMore = 1;

        // Copy device identifier bytes to channel I/O buffer
        memcpy( pIOBuf, pDEVBLK->devid, iNum );

        // Return unit status
        *pUnitStat = CSW_CE | CSW_DE;

        break;

    default:
        // ------------------------------------------------------------
        // INVALID OPERATION
        // ------------------------------------------------------------

        // Set command reject sense byte, and unit check status
        pDEVBLK->sense[0] = SENSE_CR;
        *pUnitStat        = CSW_CE | CSW_DE | CSW_UC;
    }
}

// ====================================================================
// CTCT Support
// ====================================================================

//
// CTCT_Init
//

static int  CTCT_Init( DEVBLK *dev, int argc, char *argv[] )
{
    char           str[80];            // Thread name
    int            rc;                 // Return code
    int            mtu;                // MTU size (binary)
    int            lport;              // Listen port (binary)
    int            rport;              // Destination port (binary)
    char*          listenp;            // Listening port number
    char*          remotep;            // Destination port number
    char*          mtusize;            // MTU size (characters)
    char*          remaddr;            // Remote IP address
    struct in_addr ipaddr;             // Work area for IP address
    BYTE           c;                  // Character work area
    TID            tid;                // Thread ID for server
    CTCG_PARMBLK   parm;               // Parameters for the server
    char           address[20]="";     // temp space for IP address

    dev->devtype = 0x3088;

    dev->excps = 0;

    dev->ctctype = CTC_CTCT;

    SetSIDInfo( dev, 0x3088, 0x08, 0x3088, 0x01 );

    // Check for correct number of arguments
    if (argc != 4)
    {
        WRMSG (HHC00915, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, "CTC");
        return -1;
    }

    // The first argument is the listening port number
    listenp = *argv++;

    if( strlen( listenp ) > 5 ||
        sscanf( listenp, "%u%c", &lport, &c ) != 1 ||
        lport < 1024 || lport > 65534 )
    {
        WRMSG(HHC00916, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, "CTC", "port number", listenp);
        return -1;
    }

    // The second argument is the IP address or hostname of the
    // remote side of the point-to-point link
    remaddr = *argv++;

    if (!inet_aton( remaddr, &ipaddr ))
    {
        struct hostent *hp;

        if( ( hp = gethostbyname( remaddr ) ) != NULL )
        {
            memcpy( &ipaddr, hp->h_addr, hp->h_length );
            STRLCPY( address, inet_ntoa( ipaddr ) );
            remaddr = address;
        }
        else
        {
            WRMSG(HHC00916, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, "CTC", "IP address", remaddr);
            return -1;
        }
    }

    // The third argument is the destination port number
    remotep = *argv++;

    if( strlen( remotep ) > 5 ||
        sscanf( remotep, "%u%c", &rport, &c ) != 1 ||
        rport < 1024 || rport > 65534 )
    {
        WRMSG(HHC00916, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, "CTC", "port number", remotep);
        return -1;
    }

    // The fourth argument is the maximum transmission unit (MTU) size
    mtusize = *argv;

    if( strlen( mtusize ) > 5 ||
        sscanf( mtusize, "%u%c", &mtu, &c ) != 1 ||
        mtu < 46 || mtu > 65536 )
    {
        WRMSG(HHC00916, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, "CTC", "MTU size", mtusize);
        return -1;
    }

    // Set the device buffer size equal to the MTU size
    dev->bufsize = mtu;

    // Initialize the file descriptor for the socket connection

    // It's a little confusing, but we're using a couple of the
    // members of the server paramter structure to initiate the
    // outgoing connection.  Saves a couple of variable declarations,
    // though.  If we feel strongly about it, we can declare separate
    // variables...

    // make a TCP socket
    parm.listenfd = socket( AF_INET, SOCK_STREAM, 0 );

    if( parm.listenfd < 0 )
    {
        WRMSG (HHC00900, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, "CTC", "socket()", strerror( HSO_errno ) );
        CTCX_Close( dev );
        return -1;
    }

    // bind socket to our local port
    // (might seem like overkill, and usually isn't done, but doing this
    // bind() to the local port we configure gives the other end a chance
    // at validating the connection request)
    memset( &(parm.addr), 0, sizeof( parm.addr ) );
    parm.addr.sin_family      = AF_INET;
    parm.addr.sin_port        = htons(lport);
    parm.addr.sin_addr.s_addr = htonl(INADDR_ANY);

    rc = bind( parm.listenfd,
               (struct sockaddr *)&parm.addr,
               sizeof( parm.addr ) );
    if( rc < 0 )
    {
        WRMSG( HHC00900, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, "CTC", "bind()", strerror( HSO_errno ) );
        CTCX_Close( dev );
        return -1;
    }

    // initiate a connection to the other end
    memset( &(parm.addr), 0, sizeof( parm.addr ) );
    parm.addr.sin_family = AF_INET;
    parm.addr.sin_port   = htons(rport);
    parm.addr.sin_addr   = ipaddr;
    rc = connect( parm.listenfd,
                  (struct sockaddr *)&parm.addr,
                  sizeof( parm.addr ) );

    // if connection was not successful, start a server
    if( rc < 0 )
    {
        // used to pass parameters to the server thread
        CTCG_PARMBLK* arg;

        WRMSG(HHC00971, "I", SSID_TO_LCSS(dev->ssid), dev->devnum, remaddr, remotep );

        // probably don't need to do this, not sure...
        close_socket( parm.listenfd );

        parm.listenfd = socket( AF_INET, SOCK_STREAM, 0 );

        if( parm.listenfd < 0 )
        {
            WRMSG(HHC00900, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, "CTC", "socket()", strerror( HSO_errno ) );
            CTCX_Close( dev );
            return -1;
        }

        // set up the listening port
        memset( &(parm.addr), 0, sizeof( parm.addr ) );

        parm.addr.sin_family      = AF_INET;
        parm.addr.sin_port        = htons(lport);
        parm.addr.sin_addr.s_addr = htonl(INADDR_ANY);

        if( bind( parm.listenfd,
                  (struct sockaddr *)&parm.addr,
                  sizeof( parm.addr ) ) < 0 )
        {
            WRMSG(HHC00900, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, "CTC", "bind()", strerror( HSO_errno ) );
            CTCX_Close( dev );
            return -1;
        }

        if( listen( parm.listenfd, 1 ) < 0 )
        {
            WRMSG(HHC00900, "E", SSID_TO_LCSS(dev->ssid), dev->devnum, "CTC", "listen()", strerror( HSO_errno ) );
            CTCX_Close( dev );
            return -1;
        }

        // we are listening, so create a thread to accept connection
        arg = malloc( sizeof( CTCG_PARMBLK ) );
        memcpy( arg, &parm, sizeof( parm ) );
        arg->dev = dev;
        MSGBUF(str, "CTCT %4.4X ListenThread",dev->devnum);
        str[sizeof(str)-1]=0;
        rc = create_thread( &tid, JOINABLE, CTCT_ListenThread, arg, str );
        if(rc)
           WRMSG(HHC00102, "E", strerror(rc));
    }
    else  // successfully connected (outbound) to the other end
    {
        WRMSG(HHC00972, "I", SSID_TO_LCSS(dev->ssid), dev->devnum, remaddr, remotep );
        dev->fd = parm.listenfd;
    }

    // for cosmetics, since we are successfully connected or serving,
    // fill in some details for the panel.
    MSGBUF( dev->filename, "%s:%s", remaddr, remotep );
    dev->filename[sizeof(dev->filename)-1] = '\0';
    return 0;
}

//
// CTCT_Write
//

static void  CTCT_Write( DEVBLK* pDEVBLK,   CCWC  sCount,
                         BYTE*   pIOBuf,    BYTE* pUnitStat,
                         CCWC*   pResidual )
{
    PCTCIHDR   pFrame;                  // -> Frame header
    PCTCISEG   pSegment;                // -> Segment in buffer
    U16        sOffset;                 // Offset of next frame
    U16        sSegLen;                 // Current segment length
    U16        sDataLen;                // Length of IP Frame data
    int        iPos;                    // Offset into buffer
    U16        i;                       // Array subscript
    int        rc;                      // Return code
    BYTE       szStackID[33];           // VSE IP stack identity
    U32        iStackCmd;               // VSE IP stack command

    // Check that CCW count is sufficient to contain block header
    if( sCount < sizeof( CTCIHDR ) )
    {
        WRMSG(HHC00906, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, sCount );

        pDEVBLK->sense[0] = SENSE_DC;
        *pUnitStat        = CSW_CE | CSW_DE | CSW_UC;
        return;
    }

    // Fix-up frame pointer
    pFrame = (PCTCIHDR)pIOBuf;

    // Extract the frame length from the header
    FETCH_HW( sOffset, pFrame->hwOffset );


    // Check for special VSE TCP/IP stack command packet
    if( sOffset == 0 && sCount == 40 )
    {
        // Extract the 32-byte stack identity string
        for( i = 0;
             i < sizeof( szStackID ) - 1 && i < sCount - 4;
             i++)
            szStackID[i] = guest_to_host( pIOBuf[i+4] );

        szStackID[i] = '\0';

        // Extract the stack command word
        FETCH_FW( iStackCmd, *((FWORD*)&pIOBuf[36]) );

        // Display stack command and discard the packet
        WRMSG(HHC00907, "I", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, szStackID, iStackCmd );

        *pUnitStat = CSW_CE | CSW_DE;
        *pResidual = 0;
        return;
    }

    // Check for special L/390 initialization packet
    if( sOffset == 0 )
    {
        // Return normal status and discard the packet
        *pUnitStat = CSW_CE | CSW_DE;
        *pResidual = 0;
        return;
    }

    // Adjust the residual byte count
    *pResidual -= sizeof( CTCIHDR );

    // Process each segment in the buffer
    for( iPos  = sizeof( CTCIHDR );
         iPos  < sOffset;
         iPos += sSegLen )
    {
        // Check that the segment is fully contained within the block
        if( iPos + sizeof( CTCISEG ) > sOffset )
        {
            WRMSG(HHC00908, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, iPos );

            pDEVBLK->sense[0] = SENSE_DC;
            *pUnitStat        = CSW_CE | CSW_DE | CSW_UC;
            return;
        }

        // Fix-up segment header in the I/O buffer
        pSegment = (PCTCISEG)(pIOBuf + iPos);

        // Extract the segment length from the segment header
        FETCH_HW( sSegLen, pSegment->hwLength );

        // Check that the segment length is valid
        if( ( sSegLen        < sizeof( CTCISEG ) ) ||
            ( (U32)iPos + sSegLen > sOffset      ) ||
            ( (U32)iPos + sSegLen > sCount       ) )
        {
            WRMSG(HHC00909, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, sSegLen, iPos );

            pDEVBLK->sense[0] = SENSE_DC;
            *pUnitStat        = CSW_CE | CSW_DE | CSW_UC;
            return;
        }

        // Calculate length of IP frame data
        sDataLen = sSegLen - sizeof( CTCISEG );

        // Trace the IP packet before sending
        if (pDEVBLK->ccwtrace)
        {
            WRMSG(HHC00934, "I", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->filename );
            if( pDEVBLK->ccwtrace )
                packet_trace( pSegment->bData, sDataLen, '>' );
        }

        // Write the IP packet
        rc = write_socket( pDEVBLK->fd, pSegment->bData, sDataLen );

        if( rc < 0 )
        {
            WRMSG(HHC00936, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->filename,
                    strerror( HSO_errno ) );

            pDEVBLK->sense[0] = SENSE_EC;
            *pUnitStat        = CSW_CE | CSW_DE | CSW_UC;
            return;
        }

        // Adjust the residual byte count
        *pResidual -= sSegLen;

        // We are done if current segment satisfies CCW count
        if( (U32)iPos + sSegLen == sCount )
        {
            *pResidual -= sSegLen;
            *pUnitStat = CSW_CE | CSW_DE;
            return;
        }
    }

    // Set unit status and residual byte count
    *pUnitStat = CSW_CE | CSW_DE;
    *pResidual = 0;
}

//
// CTCT_Read
//

static void  CTCT_Read( DEVBLK* pDEVBLK,   CCWC  sCount,
                        BYTE*   pIOBuf,    BYTE* pUnitStat,
                        CCWC*   pResidual, BYTE* pMore )
{
    PCTCIHDR    pFrame   = NULL;       // -> Frame header
    PCTCISEG    pSegment = NULL;       // -> Segment in buffer
    fd_set      rfds;                  // Read FD_SET
    int         iRetVal;               // Return code from 'select'
    int         iLength  = 0;

    static struct timeval tv;          // Timeout time for 'select'


    // Limit how long we should wait for data to come in
    FD_ZERO( &rfds );
    FD_SET( pDEVBLK->fd, &rfds );

    tv.tv_sec  = DEF_NET_READ_TIMEOUT_SECS;
    tv.tv_usec = 0;

    iRetVal = select( pDEVBLK->fd + 1, &rfds, NULL, NULL, &tv );

    switch( iRetVal )
    {
    case 0:
        *pUnitStat = CSW_CE | CSW_DE | CSW_UC | CSW_SM;
        pDEVBLK->sense[0] = 0;
        return;

    case -1:
        if( HSO_errno == HSO_EINTR )
            return;

        WRMSG(HHC00973, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->filename,
              strerror( HSO_errno ) );

        pDEVBLK->sense[0] = SENSE_EC;
        *pUnitStat = CSW_CE | CSW_DE | CSW_UC;
        return;

    default:
        break;
    }

    // Read an IP packet from the TUN device
    iLength = read_socket( pDEVBLK->fd, pDEVBLK->buf, pDEVBLK->bufsize );

    // Check for other error condition
    if( iLength < 0 )
    {
        WRMSG(HHC00973, "E", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, pDEVBLK->filename,
              strerror( HSO_errno ) );
        pDEVBLK->sense[0] = SENSE_EC;
        *pUnitStat = CSW_CE | CSW_DE | CSW_UC;
        return;
    }

    // Trace the packet received from the TUN device
    if (pDEVBLK->ccwtrace)
    {
        // "%1d:%04X %s: receive%s packet of size %d bytes from device %s"
        WRMSG(HHC00913, "I", SSID_TO_LCSS(pDEVBLK->ssid), pDEVBLK->devnum, "CTC",
                             "", iLength, "TUN" );
        packet_trace( pDEVBLK->buf, iLength, '<' );
    }

    // Fix-up Frame pointer
    pFrame = (PCTCIHDR)pIOBuf;

    // Fix-up Segment pointer
    pSegment = (PCTCISEG)( pIOBuf + sizeof( CTCIHDR ) );

    // Initialize segment
    memset( pSegment, 0, iLength + sizeof( CTCISEG ) );

    // Update next frame offset
    STORE_HW( pFrame->hwOffset,
              (U16)(iLength + sizeof( CTCIHDR ) + sizeof( CTCISEG )) );

    // Store segment length
    STORE_HW( pSegment->hwLength, (U16)(iLength + sizeof( CTCISEG )) );

    // Store Frame type
    STORE_HW( pSegment->hwType, ETH_TYPE_IP );

    // Copy data
    memcpy( pSegment->bData, pDEVBLK->buf, iLength );

    // Fix-up frame pointer and terminate block
    pFrame = (PCTCIHDR)( pIOBuf + sizeof( CTCIHDR ) +
                         sizeof( CTCISEG ) + iLength );
    STORE_HW( pFrame->hwOffset, 0x0000 );

    // Calculate #of bytes returned including two slack bytes
    iLength += sizeof( CTCIHDR ) + sizeof( CTCISEG ) + 2;

    if( sCount < (U32)iLength )
    {
        *pMore     = 1;
        *pResidual = 0;

        iLength    = sCount;
    }
    else
    {
        *pMore      = 0;
        *pResidual -= iLength;
    }

    // Set unit status
    *pUnitStat = CSW_CE | CSW_DE;
}

//
// CTCT_ListenThread
//

static void*  CTCT_ListenThread( void* argp )
{
    int          connfd;
    socklen_t    servlen;
    char         str[80];
    CTCG_PARMBLK parm;

    // set up the parameters passed via create_thread
    parm = *((CTCG_PARMBLK*) argp);
    free( argp );

    for( ; ; )
    {
        servlen = sizeof(parm.addr);

        // await a connection
        connfd = accept( parm.listenfd,
                         (struct sockaddr *)&parm.addr,
                         &servlen );

        MSGBUF( str, "%s:%d",
                 inet_ntoa( parm.addr.sin_addr ),
                 ntohs( parm.addr.sin_port ) );

        if( strcmp( str, parm.dev->filename ) != 0 )
        {
            WRMSG(HHC00974, "E", SSID_TO_LCSS(parm.dev->ssid), parm.dev->devnum,
                    parm.dev->filename, str);
            close_socket( connfd );
        }
        else
        {
            parm.dev->fd = connfd;
        }

        // Ok, so having done that we're going to loop back to the
        // accept().  This was meant to handle the connection failing
        // at the other end; this end will be ready to accept another
        // connection.  Although this will happen, I'm sure you can
        // see the possibility for bad things to occur (eg if another
        // Hercules tries to connect).  This will also be fixed RSN.
    }

    UNREACHABLE_CODE( return NULL );
}
// -------------------------------------------------------------------
// Query the device definition (Generic)
// -------------------------------------------------------------------

void  CTCE_Query( DEVBLK* pDEVBLK,
                  char**  ppszClass,
                  int     iBufLen,
                  char*   pBuffer )
{

    char  filename[ PATH_MAX + 1 ];     /* full path or just name    */

    BEGIN_DEVICE_CLASS_QUERY( "CTCA", pDEVBLK, ppszClass, iBufLen, pBuffer );

    snprintf( pBuffer, iBufLen, "CTCE %05d/%d %s%s%s %s IO[%"PRIu64"]",
        pDEVBLK->ctce_lport, pDEVBLK->ctce_connect_lport,
        ( pDEVBLK->ctcefd > 0           ) ? "<" : "!",
        ( pDEVBLK->ctce_contention_loser) ? "-" : "=",
        ( pDEVBLK->fd     > 0           ) ? ">" : "!",
        filename     , pDEVBLK->excps );
}

// -------------------------------------------------------------------
// Close the device
// -------------------------------------------------------------------

int  CTCE_Close( DEVBLK* pDEVBLK )
{

    // Close the device file (if not already closed)
    if ( pDEVBLK->fd >= 0 )
    {
        {
            shutdown( pDEVBLK->fd, SHUT_RDWR );
            close_socket( pDEVBLK->fd );
        }
        pDEVBLK->fd = -1;           // indicate we're now closed
    }

    // And the same for the receiving socket read device file
    if ( pDEVBLK->ctcefd >= 0 )
    {
        {
            shutdown( pDEVBLK->ctcefd, SHUT_RDWR );
            close_socket( pDEVBLK->ctcefd );
        }
        pDEVBLK->ctcefd = -1;
    }

    return 0;
}

// ====================================================================
// CTCE Support
// ====================================================================
//
// CTC Enhanced
// ============
//   Enhanced CTC functionality is designed to emulate real
//   3088 CTC Adapter hardware, using a pair of TCP sockets
//   with a likewise configured Hercules instance on a
//   different PC (or same PC).  The new device type is CTCE.
//
//   The implementation is based mostly on IBM publications,
//   "ESCON Channel-to-Channel Adapter", SA22-7203-00, and
//   also  "Channel-to-Channel Adapter", SA22-7091-01, although
//   no claim for completeness of this implemenation is feasible.
//
//   The CTCE configuration is similar to the CTCT device.  The
//   MTU bufsize parameter is optional, but when specified must be
//   >= CTCE_MTU_MIN (=62552). This is the default value when omitted.
//   (Please note that 62552 = sizeof(CTCE_SOKPFX) + sizeof(sCount) + 0xF446
//   the latter being the maximum sCount experienced in CTC CCW programs.)
//
//   CTCE requires a pair of port numbers on each device side.  In the
//   previous CTCE version these had to be an even-odd pair of port
//   numbers, whereby only the even port numbers had to be specified
//   in the CTCE configuration.  This restriction has now been removed.
//   Any port number > 1024 and < 65534 is allowed.  The CTCE configuration
//   specifies the port number at the receiving end, the sender side port
//   number is a free randomly chosen one.  The resulting socket pairs
//   cross-connect, the arrows showing the send->receive direction :
//
//      x-lport-random -> y-rport-config
//      x-lport-config <- y-rport-random
//
//   The configuration statement for CTCE is as follows, choosing one of 2
//   possible formats (noting that items between [] brackets are optional, and
//   the items between <> brackets require actual values to be given):
//
//      <ldevnum>     CTCE <lport> [<rdevnum>=]<raddress>  <rport>  [[<mtu>] <sml>] [FICON]
//      <ldevnum>[.n] CTCE <lport> [<rdevnum>]=<raddress> [<rport>] [[<mtu>] <sml>] [FICON]
//
//   where:
//
//      <ldevnum>    is the address of the CTCE device at the local system.
//      <rdevnum>    is the address of the CTCE device at the remote system.
//      <lport>      is the receiving TCP/IP port on the local system,
//                   which defaults to 3088.
//      <rport>      is the receiving TCP/IP port on the remote system,
//                   which also defaults to 3088.
//      <raddress>   is the host IP address on the remote system.
//      <mtu>        optional mtu buffersize, defaults to 62552.
//      <sml>        optional small minimum for mtu, defaults to 16.
//      n            the number of CTCE devices being configured, with
//                   consecutive addresses starting with <ldevnum>.
//                   (Only possible in the 2nd format, which implies the
//                   equal sign (=) in front of <raddress>.)
//      FICON        optional parameter specifying a FICON Channel-to-Channel adapter
//                   to be emulated (i.e. a FCTC instead of a CTCA)
//
//   A sample CTCE device configuration is shown below:
//
//      Hercules PC Host A with IP address 192.168.1.100 :
//
//         0E40  CTCE  30880  192.168.1.200  30880
//         0E41  CTCE  30881  192.168.1.200  30881
//
//      Hercules PC Host B with IP address 192.168.1.200 :
//
//         0E40  CTCE  30880  192.168.1.100  30880
//         0E41  CTCE  30881  192.168.1.100  30881
//
//   The 2nd format CTCE device configuration is easier to specify by omitting
//   the <lport> and <rport> numbers, but specifying the <rdevnum> instead.
//   The above sample configuration can then become :
//
//      # Hercules PC Host A with IP address 192.168.1.100:
//
//      0E40  CTCE  0E40=192.168.1.200
//      0E41  CTCE  0E41=192.168.1.200
//
//      # Hercules PC Host B with IP address 192.168.1.200:
//
//      0E40  CTCE  0E40=192.168.1.100
//      0E41  CTCE  0E41=192.168.1.100
//
//   This can even be further simplified by (1) omitting the <rdevnum> as its
//   default is being equal to <ldevnum>, and by (2) specifying the ".n"
//   qualifier as ".2", meaning that 2 CTCE devices starting at "0E40" are
//   being defined :
//
//      # Hercules PC Host A with IP address 192.168.1.100:
//
//      0E40.2  CTCE  =192.168.1.200
//
//      # Hercules PC Host B with IP address 192.168.1.200:
//
//      0E40.2  CTCE  =192.168.1.100
//
//   The above example omits <rdevnum>, and values for this
//   below 0100 = 0x0100 = 256 are not allowed.  But single hex
//   digit <rdevnum> values have a special menaing.  They will be used to
//   construct <rdevnum>'s by exclusive-or-ing the <ldevnum> with such
//   value.  As an example :
//
//      0E40.4  CTCE       1=192.168.1.200
//
//   The above is the same as :
//
//      0E40    CTCE    0E41=192.168.1.200
//      0E41    CTCE    0E40=192.168.1.200
//      0E42    CTCE    0E43=192.168.1.200
//      0E43    CTCE    0E42=192.168.1.200
//
//   The above example could be used to establish a redundant pair of
//   read/write CTC links, where each Hercules side uses the even devnum
//   addresses for reading, and the odd ones for writing (or the other
//   way around).  That way, the operating system definitions on each
//   side can be identical, e.g. for a VTAM MPC CTC link :
//
//      CTCATRL  VBUILD TYPE=TRL
//      C0E40TRL TRLE  LNCTL=MPC,READ=(0E40,0E42),WRITE=(0E41,0E43)
//
//      CTCALCL  VBUILD TYPE=LOCAL
//      C0E40LCL PU    TRLE=C0E40TRL,XID=YES,CONNTYPE=APPN,CPCP=YES,TGP=CHANNEL
//
//   Please also note the optional trailing keyword FICON.  When specified,
//   a fiber channel CTC adapter (FCTC) is being emulated, instead of a
//   regular CTCA.
//
//   CTCE connected Hercules instances can be hosted on any Hercules supported
//   platform (Windows, Linux, MacOS ...).  Both sides do not need to be the same.
//
// ---------------------------------------------------------------------
// Execute a Channel Command Word (CTCE)
// ---------------------------------------------------------------------

void  CTCE_ExecuteCCW( DEVBLK* pDEVBLK, BYTE  bCode,
                       BYTE    bFlags,  BYTE  bChained,
                       CCWC    sCount,  BYTE  bPrevCode,
                       int     iCCWSeq, BYTE* pIOBuf,
                       BYTE*   pMore,   BYTE* pUnitStat,
                       CCWC*   pResidual )
{
    int             iNum;               // Number of bytes to move
    CTCE_INFO       CTCE_Info = { 0 };  // CTCE information (also for tracing)

    UNREFERENCED( bChained  );
    UNREFERENCED( bPrevCode );
    UNREFERENCED( iCCWSeq   );
    UNREFERENCED( pMore     );

    // Initialise our CTCE_Info previous x- and y-states.
    CTCE_Info.state_x_prev = pDEVBLK->ctcexState;
    CTCE_Info.state_y_prev = pDEVBLK->ctceyState;

    // Intervention required if the device file is not open
    if( ( pDEVBLK->fd < 0 ) || ( pDEVBLK->ctcefd < 0 ) )
    {
        if( !IS_CCW_SENSE( bCode ) &&
            !IS_CCW_CONTROL( bCode ) )
        {
            pDEVBLK->sense[0] = ( SENSE_IR | SENSE_OC );
            *pUnitStat = CSW_CE | CSW_DE | CSW_UC;

            // A state mismatch tracing does not apply in this case.         
            CTCE_Info.state_new = pDEVBLK->ctcexState;

            // Produce a CTCE Trace logging if requested.
            CTCE_RESTART_CCWTRACE( pDEVBLK ) ;
            if( CTCE_CCWTRACE( pDEVBLK ) )
            {
                CTCE_Trace( pDEVBLK, ( pDEVBLK->ctcefd < 0 ) ?
                    CTCE_SND_NSR : CTCE_SND_NS,  &CTCE_Info, pUnitStat );
            }
            return;
        }
    }

    // Changes to DEVBLK are lock protected as the CTCE_RecvThread
    // might update as well.
    OBTAIN_DEVLOCK( pDEVBLK );

    // The CCW Flags Command Chaining indicator being set indicates
    // that a CCW Program is in progress.  The last CCW in the chain
    // has this flag turned off.
    pDEVBLK->ctce_ccw_flags_cc = ( ( bFlags & CCW_FLAGS_CC ) != 0 );

    // Copy control command byte in x command register
    pDEVBLK->ctcexCmd = ( bCode == 0 ) ? bCode_invalid : bCode;

    // The new X-state and transition actions are derived from the FSM table.

    CTCE_Info.state_new   = CTCE_NEW_X_STATE( pDEVBLK->ctcexCmd );
    CTCE_Info.actions     = CTCE_Fsm[CTCE_CMD( pDEVBLK->ctcexCmd )][CTCE_X_STATE_FSM_IDX].actions;
    CTCE_Info.x_unit_stat = CTCE_Fsm[CTCE_CMD( pDEVBLK->ctcexCmd )][CTCE_X_STATE_FSM_IDX].x_unit_stat;

    *pUnitStat            = CTCE_Fsm[CTCE_CMD( pDEVBLK->ctcexCmd )][CTCE_X_STATE_FSM_IDX].x_unit_stat;

    // If a READ or READ_BACKWARD command is received whilst the WEOF
    // bit is set then the sole case for a Unit Exception applies.
    if( IS_CTCE_WEOF( pDEVBLK->ctcexState ) &&
        IS_CTCE_CCW_RDA( pDEVBLK->ctcexCmd ) )
    {
        CLR_CTCE_WEOF( pDEVBLK->ctcexState );
        *pResidual = 0;
        *pUnitStat = CSW_CE | CSW_DE | CSW_UX;
    }

    // Otherwise in case the CTCE device is not busy actions may result.
    else if( !( CTCE_Info.x_unit_stat & CSW_BUSY ) )
    {
        CLR_CTCE_WEOF( pDEVBLK->ctcexState );
        pDEVBLK->ctcexState = CTCE_NEW_X_STATE( pDEVBLK->ctcexCmd );

        // Process depending on the CCW command.
        switch ( CTCE_CMD( pDEVBLK->ctcexCmd ) )
        {

        // Most of the CTCE commands processing (if any at all) takes
        // place in CTCE_Send and CTCE_RECV down below, except this :
        case CTCE_PREPARE:
        case CTCE_CONTROL:
        case CTCE_READ:
        case CTCE_WRITE:
        case CTCE_READ_BACKWARD:
        case CTCE_WRITE_END_OF_FILE:
        case CTCE_NO_OPERATION:
            break;

        case CTCE_SET_EXTENDED_MODE:
            pDEVBLK->ctcxmode = 1;
            break;

        case CTCE_SET_BASIC_MODE:
            pDEVBLK->ctcxmode = 0;
            break;

        case CTCE_SENSE_COMMAND_BYTE:

            // In y-state available we return 0 otherwise the last y-side command.
            *pIOBuf = ( IS_CTCE_YAV( pDEVBLK->ctceyState ) ) ?
                0 : pDEVBLK->ctceyCmd;
            CTCE_Info.scb = *pIOBuf;
            *pResidual = sCount - 1;
            break;

        case CTCE_SENSE_ADAPTER_STATE:

            // Calculate residual byte count
            iNum = ( sCount < pDEVBLK->numsense ) ?
                sCount : pDEVBLK->numsense;
            *pResidual = sCount - iNum;

            // Copy device sense bytes to channel I/O buffer
            memcpy( pIOBuf, pDEVBLK->sense, iNum );
            memcpy( &CTCE_Info.sas, pDEVBLK->sense, 2 );

            // Clear the device sense bytes
            memset( pDEVBLK->sense, 0, sizeof( pDEVBLK->sense ) );
            break;

        case CTCE_SENSE_ID:

            // Calculate residual byte count
            iNum = ( sCount < pDEVBLK->numdevid ) ?
                sCount : pDEVBLK->numdevid;
            *pResidual = sCount - iNum;

            // Copy device identifier bytes to channel I/O buffer
            memcpy( pIOBuf, pDEVBLK->devid, iNum );
            break;

        case CTCE_READ_CONFIG_DATA:

            // Build the RCD bytes into the devid.
            iNum = CTCE_Build_RCD( pDEVBLK,
                pDEVBLK->devid + pDEVBLK->numdevid,
                sizeof( pDEVBLK->devid ) - pDEVBLK->numdevid );
            if ( (int) sCount < iNum )
            {
                iNum = sCount;
            }
            *pResidual = sCount - iNum;

            // Copy the RCD bytes now in devid to the channel I/O buffer.
            memcpy( pIOBuf, pDEVBLK->devid + pDEVBLK->numdevid, iNum );
            break;

        // Invalid commands
        // (or never experienced / tested / supported ones)
        default:

            // Signalling invalid commands using Unit Check with a
            // Command Reject sense code for this CTCE device failed.
            // (MVS results were a WAIT 064 RSN 9 during NIP.)
            // An Interface Control Check would probably be needed but
            // we do not know how to generate that, so we use SENSE_EC.
            //
            //    pDEVBLK->sense[0] = SENSE_CR;

            pDEVBLK->sense[0] = SENSE_EC;
            *pUnitStat        = CSW_CE | CSW_DE | CSW_UC;

        } // switch ( CTCE_CMD( pDEVBLK->ctcexCMD ) )

        // In most cases we need to inform the other (y-)side so we SEND
        // our command (and data) to the other side.  During this process
        // and any response received, all other actions take place.
        if( IS_CTCE_SEND( CTCE_Info.actions ) )
        {
            CTCE_Send( pDEVBLK, sCount, pIOBuf, pUnitStat, pResidual, &CTCE_Info );
        }

    } // if( !( CTCE_Info.x_unit_stat & CSW_BUSY ) )

    // Sense byte 0 bits 2 thru 6 are reset, effectively all but 1 and 7.
    pDEVBLK->sense[0] &= ( SENSE_IR | SENSE_OC );

    // We merge a Unit Check in case the Y state is Not Ready.
    // But only when pUnitStat is still 0 or Unit Check or Busy (no Attn).
    // sense byte 0 bit 1 (Intervention Required) will be set,
    // and also bit 7 (Interface Discconect / Operation Check).    
    if( IS_CTCE_YNR( pDEVBLK -> ctceyState ) &&
        ( ( *pUnitStat & (~ ( CSW_BUSY | CSW_UC ) ) ) == 0 ) )
    {
        *pUnitStat |= CSW_UC;
        pDEVBLK->sense[0] = ( SENSE_IR | SENSE_OC );
    }

    // Produce a CTCE Trace logging if requested, noting that for the
    // IS_CTCE_WAIT cases such a logging is produced prior to the WAIT,
    // and CTCE_Recv will produce a logging for the matching command.
    if( ( CTCE_CCWTRACE( pDEVBLK ) ) &&
        ( !( IS_CTCE_WAIT( CTCE_Info.actions ) ) ||
          !CTCE_Info.sent ) )
    {
        CTCE_Trace( pDEVBLK, ( ( !IS_CTCE_SEND( CTCE_Info.actions ) ) ? CTCE_LCL :
            ( CTCE_Info.sent ? CTCE_SND : ( ( pDEVBLK->ctcefd < 0 ) ? CTCE_SND_NSR :
            CTCE_SND_NS ) ) ), &CTCE_Info, pUnitStat );
    }

    RELEASE_DEVLOCK( pDEVBLK );

} // CTCE_ExecuteCCW

// ---------------------------------------------------------------------
// CTCE_Init
// ---------------------------------------------------------------------

static int  CTCE_Init( DEVBLK *dev, int argc, char *argv[] )
{
    int            mtu;                // MTU size (binary)
    char*          listenp;            // Listening port number
    char*          remotep;            // Destination port number
    char*          mtusize;            // MTU size (characters)
    char*          remaddr;            // Remote IP address
    BYTE           c;                  // Character work area
    int            ctceSmlBin;         // Small size (binary)
    char*          ctceSmlChr;         // Small size (characters)
    char           address[20]="";     // temp space for IP address
    int            next_arg = 0;       // Proceeds upto argc - 1
    char*          equal_sign;         // CCUU - IP address separator
    int            argc_updated = argc;
    char*          attndelay;          // Optional ATTNDELAY parm keyword

    // In case of a "devinit" command prior to a CTCE connection,
    // we want to close down the connect() thread.  We simulate a
    // "detach" command followed by a 0.7 sec sleep during which
    // we unlock the device to accomplish this.
    if( dev->reinit )
    {
        if( dev->fd < 0 )
        {
            dev->allocated = 0;
            RELEASE_DEVLOCK( dev );
            {
                USLEEP( 700000 );
            }
            OBTAIN_DEVLOCK( dev );
            dev->allocated = 1;
        }
        else
        {

            // Otherwise "devinit" closes the CTCE connection.
            CTCE_Close( dev );
        }
    }

    dev->devtype = 0x3088;

    dev->ctctype = CTC_CTCE;

//  SetSIDInfo( dev, 0x3088, 0x08, 0x0000, 0x00 ); CTCA, Extended Mode
//  SetSIDInfo( dev, 0x3088, 0x08, 0x0000, 0x01 ); CTCA, Basic    Mode
//  SetSIDInfo( dev, 0x3088, 0x1F, 0x0000, 0x00 ); ESCON CTC, Extended Mode, i.e. SCTC
//  SetSIDInfo( dev, 0x3088, 0x1F, 0x0000, 0x01 ); ESCON CTC, Basic    Mode, i.e. BCTC
//  SetSIDInfo( dev, 0x3088, 0x1E, 0x0000, 0x00 ); FICON CTC
//  SetSIDInfo( dev, 0x3088, 0x01, ...          ); P390 OSA emulation
//  SetSIDInfo( dev, 0x3088, 0x60, ...          ); OSA/2 adapter
//  SetSIDInfo( dev, 0x3088, 0x61, ...          ); CISCO 7206 CLAW protocol ESCON connected
//  SetSIDInfo( dev, 0x3088, 0x62, ...          ); OSA/D device
//  But the orignal CTCX_init had this :
//  SetSIDInfo( dev, 0x3088, 0x08, 0x3088, 0x01 );
//  Which is what we used until we made the VM TSAF connection work as well which needed :
    SetSIDInfo( dev, 0x3088, 0x08, 0x0000, 0x01 );    

    dev->numsense = 2;
    // A version 4 only feature ...
    dev->excps = 0;

    // The halt_device exit is established; in version 4 this is in DEVHND in the ctce_device_hndinfo.
//  dev->halt_device = &CTCE_Reset;

    // Mark both socket file descriptors as not yet connected.
    dev->fd = -1;       // For send / write to the other (y-) side
    dev->ctcefd = -1;   // For receive / read from the other (y-)side

    // We begin by checking the trailing parameter for the optional keyword FICON.
    // FCTC's support for the RCD command must be supplied via Sense ID.
    dev->ctce_ficon = ( strcasecmp( argv[argc - 1], "FICON" ) == 0 );
    argc_updated = argc - dev->ctce_ficon;
    if ( dev->ctce_ficon )
    {
        SetSIDInfo( dev, 0x3088, 0x1E, 0x0000, 0x00 );
        SetCIWInfo( dev, 0, 0, 0xC4, 0x0080 );
    }

    // We check for the next trailing optional parameter ATTNDELAY <nnn> which
    // can be used to insert a delay of <nnn> msec prior to ATTN interrupts.
    // This was found to be needed for circumventing a probable VM/SP VTAM 3 bug.
    dev->ctce_attn_delay = 0;                    // Defaults to 0
    if ( argc_updated > 2 )
    {
        if ( strcasecmp( argv[argc_updated - 2], "ATTNDELAY" ) == 0 )
        {
            attndelay = argv[argc_updated - 1];
            if ( strlen( attndelay ) > 3 ||
                sscanf( attndelay, "%u%c", &dev->ctce_attn_delay, &c ) != 1 )
            {
                dev->ctce_attn_delay = 0;        // Defaults to 0
                WRMSG( HHC05085, "W",  // CTCE: Invalid ATTNDELAY value %s ignored"
                    CTCX_DEVNUM( dev ), attndelay );
            }
            argc_updated -= 2;
        }
    }
    dev->ctce_attn_delay *= 1000;                // msec -> micro seconds

    // At least the remote CTCE IP address needs to be specified.
    if ( argc_updated < 1 )
    {
        WRMSG( HHC05055, "E",  // CTCE: Missing (at least) remote CTCE IP address parameter"
            CTCX_DEVNUM( dev ) );
        return -1;
    }

    // The first argument is the listening port number, which is now optional.
    listenp = argv[next_arg];
    if ( strlen( listenp ) > 5 ||
        sscanf( listenp, "%u%c", &dev->ctce_lport, &c ) != 1 )
    {
        dev->ctce_lport = CTCE_DEFAULT_LISTEN_PORT;
    }
    else
    {
        next_arg++;
        if (dev->ctce_lport < 1024 || dev->ctce_lport > 65534 )
        {
            WRMSG( HHC05056, "E",  // CTCE: Local port number outside range 1024-65534: %s"
                CTCX_DEVNUM( dev ), listenp );
            return -1;
        }
    }

    // The next argument is the IP address or hostname of the
    // remote side of the point-to-point link
    remaddr = argv[next_arg++];

    // The remote IP address can be optionally preceeded by the
    // CCUU address of the CTCE at the remote side followed by an
    // equal sign without intervening blanks, e.g. 0C40=192.168.1.230
    if ( ( equal_sign = strchr( remaddr, '=' ) ) )
    {
        dev->ctce_rccuu = strtoul( remaddr, &equal_sign, 16 );
        if ( dev->ctce_rccuu > 255 )
        {
            if ( dev->ctce_rccuu > 65534 )
            {
                WRMSG( HHC05057, "E",  // CTCE: Remote CCUU address outside range 0001-FFFF: %4X"
                    CTCX_DEVNUM( dev ), dev->ctce_rccuu );
                return -1;
            }
            dev->ctce_rccuu += dev->numconfdev - 1;
        }

        // Remote CCUU addresses < 256 are not supported, but such values
        // are used to compute the actual remote CCUU address from the
        // local CCUU address by flipping certain bits in it.  The special
        // interesting values are 0, 1, 3, 5 and 9.  0 yields remote equal
        // local.  1 flips the least signigicant bit so that local / remote
        // are always odd / even (or vice versa), as do values 3, 5, and 9,
        // except that another bit in the least significant hex digit is
        // flipped as well.  This may be useful when defining multiple
        // CTCE devices, e.g. : 0C40.16 CTCE 9=192.168.100.10
        else
        {
            dev->ctce_rccuu = dev->devnum ^ dev->ctce_rccuu;
        }

        remaddr = equal_sign + 1;
    }
    else
    {
        dev->ctce_rccuu = 0;
    }

    if ( !inet_aton( remaddr, &dev->ctce_ipaddr ) )
    {
        struct hostent *hp;

        if ( ( hp = gethostbyname( remaddr ) ) != NULL )
        {
            memcpy( &dev->ctce_ipaddr, hp->h_addr, hp->h_length );
            strcpy( address, inet_ntoa( dev->ctce_ipaddr ) );
            remaddr = address;
        }
        else
        {
            WRMSG( HHC05058, "E",  // CTCE: Invalid IP address %s
                CTCX_DEVNUM( dev ), remaddr );
            return -1;
        }
    }

    // The next argument is the remote destination port number.
    if ( next_arg < argc_updated )
    {
        remotep = argv[next_arg++];
        if ( strlen( remotep ) > 5 ||
            sscanf( remotep, "%u%c", &dev->ctce_rport, &c ) != 1 ||
            dev->ctce_rport < 1024 || dev->ctce_rport > 65534 )
        {
            WRMSG( HHC05059, "E",  // CTCE: Invalid port number: %s"
                CTCX_DEVNUM( dev ), remotep );
            return -1;
        }
    }

    // Please note that the default for this optional parm is 3088,
    // but it is only optional in case the remote CCUU is specified.
    else
    {
        if ( dev->ctce_rccuu == 0 )
        {
            WRMSG( HHC05060, "E",  // CTCE: Both remote listening port and remote CCUU are missing; at least one is required."
                CTCX_DEVNUM( dev ) );
            return -1;
        }
        else
        {
            dev->ctce_rport = CTCE_DEFAULT_RPORT;
        }
    }

    // Enhanced CTC default MTU bufsize is CTCE_MTU_MIN.
    if ( next_arg < argc_updated )
    {
        mtusize = argv[next_arg++];
        // The next argument is the maximum transmission unit (MTU) size
        if ( strlen( mtusize ) > 5 ||
            sscanf( mtusize, "%u%c", &mtu, &c ) != 1 ||
            mtu < CTCE_MTU_MIN || mtu > 65536 )
        {
            WRMSG( HHC05061, "E",  // CTCE: Invalid MTU size %s, allowed range is %d to 65536"
                CTCX_DEVNUM( dev ), mtusize, CTCE_MTU_MIN );
            return -1;
        }
    }
    else
    {
        mtu = CTCE_MTU_MIN;
    }

    // Set the device buffer size equal to the MTU size times 2, as
    // 2 such buffers are needed in parallel to cater for receiving
    // WRITE commands data prior to matching them with READ commands.
    // (Please see CTCE_RecvThread and CTCE_Send for more details.)
    dev->bufsize = mtu * 2;

    // Enhanced CTC only supports an optional extra parameter,
    // the Small MTU size, which defaults to the minimum size
    // of the TCP/IP packets exchanged: CTCE_SOKPFX.
    ctceSmlBin = sizeof(CTCE_SOKPFX);
    if ( next_arg < argc_updated )
    {
        ctceSmlChr = argv[next_arg++];
        if ( strlen( ctceSmlChr ) > 5 ||
            sscanf( ctceSmlChr, "%u%c", &ctceSmlBin, &c ) != 1 ||
            ctceSmlBin < (int)sizeof(CTCE_SOKPFX) || ctceSmlBin > mtu )
        {
            ctceSmlBin = sizeof(CTCE_SOKPFX);
            WRMSG( HHC05062, "W",  // CTCE: Invalid Small MTU size %s ignored"
                CTCX_DEVNUM( dev ), ctceSmlChr );
        }
    }
    dev->ctceSndSml = ctceSmlBin;

    // Check if there are any extraneous parameters to ignore.
    if ( next_arg < argc_updated )
    {
        WRMSG( HHC05064, "W",  // CTCE: Extraneous parameters ignored : %s ..."
            CTCX_DEVNUM( dev ), argv[next_arg++] );
    }

    // We're now ready to start the Listen and Connect threads to
    // asynchronously connect to the partner CTCE when present.
    return CTCE_Start_Listen_Connect_Threads( dev );

} // CTCE_Init

// ---------------------------------------------------------------------
// CTCE_Start_Listen_Connect_Threads
// ---------------------------------------------------------------------

static int  CTCE_Start_Listen_Connect_Threads( DEVBLK* dev )
{
    char           str[80];                 // Temp string
    CTCE_PARMBLK   parm_listen;             // Parameters for the server
    CTCE_PARMBLK*  arg;                     // used to pass parameters to the server thread
    char*          remaddr;                 // Remote IP address
    char           address[20];             // String for IP address
    char           rccuu_addr_rport[30];    // String for rcuu and IP address
    DEVBLK*        dev_srch;                // Device block being searched

    // for cosmetics, since we are successfully serving,
    // fill in some details for the panel.
    strcpy( address, inet_ntoa( dev->ctce_ipaddr ) );
    remaddr = address;
    MSGBUF( rccuu_addr_rport, "%1d:%04X=%s:%d/*",
        SSID_TO_LCSS( dev->ssid ), dev->ctce_rccuu, remaddr, dev->ctce_rport );
    strcpy(  dev->filename, rccuu_addr_rport );

    // We only need to start a listen thread provided no such thread
    // listening on that port number is already running.  We search
    // the device blocks to see if none has been started yet.
    for ( dev_srch = sysblk.firstdev; dev_srch; dev_srch = dev_srch->nextdev )
    {
        if ( 1
            && dev_srch->ctctype == CTC_CTCE
            && dev_srch->ctce_listen_tid != 0
            && dev_srch->ctce_lport ==  dev->ctce_lport
        )
        {
            break;
        }
    }

    // OK if none has been started yet.
    if ( !dev_srch )
    {

        // Get a socket for the listening port.
        memset( &(parm_listen.addr), 0, sizeof( parm_listen.addr ) );
        parm_listen.addr.sin_family      = AF_INET;
        parm_listen.addr.sin_port        = htons( dev->ctce_lport );
        parm_listen.addr.sin_addr.s_addr = htonl( INADDR_ANY );
        if ( ( parm_listen.fd = CTCE_Get_Socket( dev, CTCE_SOK_LIS ) ) < 0 )
        {
            return -1;
        }
        else
        {

            // The backlog parameter is set to 128 to avoid connection refused
            // situations when high numbers of CTCE devices are defined.
            if ( listen( parm_listen.fd, 128 ) > -1 )
            {

                // We are listening, so create a thread to accept connections.
                arg = malloc( sizeof( CTCE_PARMBLK ) );
                memcpy( arg, &parm_listen, sizeof( parm_listen ) );
                arg->dev = dev;
                MSGBUF( str, "CTCE %4.4X ListenThread", dev->devnum );
                str[sizeof( str ) - 1] = 0;
                if ( create_thread( &dev->ctce_listen_tid, DETACHED, CTCE_ListenThread, arg, str ) == 0 )
                {
                    WRMSG( HHC05063, "I",  // CTCE: Awaiting inbound connection :%5d <- %s"
                        CTCX_DEVNUM( dev ), dev->ctce_lport, rccuu_addr_rport );
                }
                else
                {
                    WRMSG( HHC05080, "E",  // CTCE: create listen thread %s error: %s"
                        CTCX_DEVNUM( dev ), str, strerror( HSO_errno ) );
                    return -1;
                }
            }
            else
            {
                WRMSG( HHC05066, "E",  /* CTCE: Error on call to listen (port=%d): %s */
                    CTCX_DEVNUM( dev ), dev->ctce_lport, strerror( HSO_errno ) );
                close_socket( parm_listen.fd );
                return -1;
            }
        }
    }

    // If we did not have to start a CTCE_ListenThread, then we report that.
    if ( dev_srch )
    {
        WRMSG( HHC05081, "I",  // CTCE: Already awaiting connection :%5d <- %s"
            CTCX_DEVNUM( dev ), dev->ctce_lport, rccuu_addr_rport );
    }

    // We're now ready to start the Connect thread to
    // asynchronously connect to the partner CTCE when present.
    return CTCE_Start_ConnectThread( dev );

} // CTCE_Start_Listen_Connect_Threads

// ---------------------------------------------------------------------
// CTCE_ListenThread
// ---------------------------------------------------------------------

PUSH_GCC_WARNINGS()
DISABLE_GCC_UNUSED_SET_WARNING; // (because rc only referenced if full keepalive)

static void*  CTCE_ListenThread( void* argp )
{
    DEVBLK        *pDEVBLK;                      // device block pointer
    int            connect_fd;
    int            iLength;                      // length of 1st read_socket
    socklen_t      servlen;
    char           str[80];
    CTCE_PARMBLK   parm_listen;
    TID            tid2;                         // Thread ID for read thread
#if defined( CTCE_DISABLE_NAGLE )
    const int      so_value_1 = 1;               // Argument for setsockopt
#endif
    CTCE_SOKPFX   *pSokBuf;                      // Overlay for buf inside DEVBLK
    BYTE          *buf;                          //-> Device 1st recv data buffer
    DEVBLK        *dev;                          // device block pointer to search for
    char*          remaddr;                      // Remote IP address
    char           address[20]="";               // temp space for IP address
    BYTE           renewing;                     // When renewing a CTCE connection
#if defined( HAVE_BASIC_KEEPALIVE )
    int            rc;                           // set_socket_keepalive Return Code
#endif // defined( HAVE_BASIC_KEEPALIVE )

    // Set up the parameters passed via create_thread.
    parm_listen = *( ( CTCE_PARMBLK* ) argp );
    free( argp );
    pDEVBLK = parm_listen.dev;
    servlen = sizeof( parm_listen.addr );

    // We keep on listening, i.e. waiting to accept() incoming connect's, until shutdown time.
    for( ; ; )
    {
        if ( ( connect_fd = accept( parm_listen.fd,
            ( struct sockaddr * )&parm_listen.addr, &servlen ) ) > -1 )
        {

            // A bonafide connect() to us also provides an initial record
            // with a specific length of data for verification purposes.
#if defined( CTCE_DISABLE_NAGLE )
            if ( setsockopt( connect_fd, IPPROTO_TCP, TCP_NODELAY,
                ( GETSET_SOCKOPT_T* )&so_value_1, sizeof( so_value_1 ) ) > -1 )
#endif
            {
                buf = malloc( pDEVBLK->ctceSndSml );
                memset( buf, 0, pDEVBLK->ctceSndSml );
                pSokBuf = ( CTCE_SOKPFX* )buf;
                iLength = read_socket( connect_fd, buf, pDEVBLK->ctceSndSml );
                if ( iLength == pDEVBLK->ctceSndSml )
                {

                    // We search the CTCE devices for one with a matching remote IP address,
                    // and either a matching remote CCUU address when one is specified, or
                    // a matching remote listening port number (the CTCE v. 1 case).
                    for ( dev = sysblk.firstdev; dev; dev = dev->nextdev )
                    {
                        if  ( 1
                            && dev->allocated
                            && dev->ctctype == CTC_CTCE
                            && dev->ctce_ipaddr.s_addr == parm_listen.addr.sin_addr.s_addr
                            && dev->ctce_rport == pSokBuf->ctce_lport
                            &&  ( 0
                                ||   ( dev->ctce_rccuu == 0 )
                                || ( ( dev->ctce_rccuu != 0 ) && ( dev->ctce_rccuu == pSokBuf->devnum     ) )
                                )
                            )
                        {
                            OBTAIN_DEVLOCK( dev );

                            // We might need to re-initialise our TID following a CTCE recovery.
                            dev->ctce_listen_tid = hthread_self();

                            // At any point in time will one of the two sides of a CTCE link be
                            // the "ctce_contention_loser".  Both sides receiving a dependent
                            // command "simultaneously" may detect this only upon receiving it from
                            // the other side.  The ctce_contention_loser side will retroactively
                            // back out as if the other side was first, so that the winner can ignore
                            // reception of the conflicting command.  Here we merely initialize
                            // the ctce_contention_loser side, so that the side with the highest IP
                            // address or CCUU address will be the initial winner.
                            if      ( pSokBuf->ctce_ipaddr.s_addr > dev->ctce_ipaddr.s_addr )
                                dev->ctce_contention_loser = 0;
                            else if ( pSokBuf->ctce_ipaddr.s_addr < dev->ctce_ipaddr.s_addr )
                                dev->ctce_contention_loser = 1;
                            else if ( pSokBuf->ssid               > dev->ssid               )
                                dev->ctce_contention_loser = 0;
                            else if ( pSokBuf->ssid               < dev->ssid               )
                                dev->ctce_contention_loser = 1;
                            else if ( pSokBuf->devnum             > dev->devnum             )
                                dev->ctce_contention_loser = 0;
                            else // ( pSokBuf->devnum             < dev->devnum             )
                                dev->ctce_contention_loser = 1;

                            // Show the actual remote listening and connecting ports in filename.
                            strcpy( address, inet_ntoa( dev->ctce_ipaddr ) );
                            remaddr = address;
                            MSGBUF( dev->filename, "%1d:%04X=%s:%d/%d", CTCE_DEVNUM( pSokBuf ),
                                remaddr, pSokBuf->ctce_lport, ntohs( parm_listen.addr.sin_port ) );

                            // In case our side believed we were already connected, the other
                            // side must have issued a re-connect for whatever reason, e.g.
                            // when recovering a lost connection or completely restarting.
                            if( dev->ctcefd > 0 )
                            {
                                renewing = 1;
                            }
                            else
                            {
                                renewing = 0;
                            }

#if defined( HAVE_BASIC_KEEPALIVE )
                            // We try improving recoverability by enabling TCP keepalives.
                            // We re-use the retry interval and count values from the console values,
                            // (CONKPALV), but the idle interval will be set to 1200 seconds, as
                            // inactivity periods of up to 15 minutes are normal.
                            // We only report failure if the system is expected to support it all.
#define CTCE_KEEPALIVE_IDLE 1200
                            rc = set_socket_keepalive( connect_fd, CTCE_KEEPALIVE_IDLE,
                                                       sysblk.kaintv, sysblk.kacnt );
  #if defined( HAVE_FULL_KEEPALIVE )
                            if( rc != 0 )
                            {
                                WRMSG( HHC05082, "W",  // CTCE: TCP set_socket_keepalive RC=%d"
                                    CTCX_DEVNUM( dev ), rc);
                            }
  #endif // defined( HAVEHAVE_FULL_KEEPALIVE )
#endif // defined( HAVE_BASIC_KEEPALIVE )

                            // The all-important connect socket descriptor is now established.
                            dev->ctcefd = connect_fd;

                            // This side is ready to start receiving and sending so we
                            // start a read thread to do the receiving part;
                            MSGBUF( str, "CTCE %04X RecvThread", dev->devnum );
                            str[sizeof( str ) - 1] = 0;
                            if( create_thread( &tid2, DETACHED, CTCE_RecvThread, dev, str ) != 0 )
                            {
                                WRMSG( HHC05069, "E",  // CTCE: create_thread %s error: %s"
                                    CTCX_DEVNUM( dev ), str, strerror( HSO_errno ) );
                            }
                            else
                            {
                                WRMSG( HHC05070, "I",  // CTCE: %s inbound connection :%5d <- %1d:%04X=%s:%5d (bufsize=%d,%d)"
                                    CTCX_DEVNUM( dev ), ( renewing ? "Renewing" : "Accepted" ), dev->ctce_lport,
                                    CTCE_DEVNUM( pSokBuf ), remaddr, ntohs( parm_listen.addr.sin_port),
                                    dev->bufsize / 2, dev->ctceSndSml );
                            }

                            // If we are not yet or no longer connected to the other side,
                            // then we must re-initiate the CTCE_ConnectThread.
                            if ( ( dev->fd != -1 ) && ( ! ( pSokBuf->ctce_herc & CTCE_HERC_RECV ) ) )
                            {
                                dev->reinit = 1;
                                CTCE_Start_ConnectThread( dev ) ;
                            }

                            // We break out of the device search loop.
                            RELEASE_DEVLOCK( dev );
                            break ;
                        } // matching device found
                    } // device search loop
                } // initial read_socket() after connect() completed

                // A TCP connect() which did not match any of our CTCE devices is reported.
                if ( ( ( iLength != pDEVBLK->ctceSndSml ) || ( !dev ) ) )
                {
                    WRMSG( HHC05067, "W",  // CTCE: Ignoring non matching connection from %1d:%04X=%s:%d"
                         CTCX_DEVNUM( pDEVBLK ), CTCE_DEVNUM( pSokBuf ),
                         inet_ntoa( parm_listen.addr.sin_addr ), ntohs( parm_listen.addr.sin_port ) );
                    close_socket( connect_fd );
                }
                free ( buf ) ;
            }
#if defined( CTCE_DISABLE_NAGLE )
            else
            {
                WRMSG( HHC05068, "E",  // CTCE: TCP_NODELAY error for listening socket %d (port %d): %s"
                    CTCX_DEVNUM( pDEVBLK ), connect_fd, pDEVBLK->ctce_lport, strerror( HSO_errno ) );
                close_socket( connect_fd );
            }
#endif
        }
        else
        {
            WRMSG( HHC05083, "E",  // CTCE: Error on accept() for listening socket %d (port %d): %s"
                CTCX_DEVNUM( pDEVBLK ), connect_fd, pDEVBLK->ctce_lport, strerror( HSO_errno ) );
            close_socket( connect_fd );
        }
    } // do forever loop until shutdown

    UNREACHABLE_CODE( return NULL );

} // CTCE_ListenThread

POP_GCC_WARNINGS()

// ---------------------------------------------------------------------
// CTCE_Send
// ---------------------------------------------------------------------

static void     CTCE_Send(        DEVBLK*             pDEVBLK,
                            const CCWC                sCount,
                                  BYTE*               pIOBuf,
                                  BYTE*               pUnitStat,
                                  CCWC*               pResidual,
                                  CTCE_INFO*          pCTCE_Info )
{
    CTCE_SOKPFX   *pSokBuf;                 // overlay for buf in the device block
    CTCE_SOKPFX   *pSokBuf_written;         // ... and the alternate buf in the same
    int            rc;                      // Return code

    if( ! IS_CTCE_SEND( pCTCE_Info->actions ) )
    {
        WRMSG( HHC05071, "S",  // CTCE: Internal error, SEND status incorrectly encoded !
            CTCX_DEVNUM( pDEVBLK ) );
    }

    // We only ever Send if the sockets are connected.
    if( ( pDEVBLK->fd < 0 ) || ( pDEVBLK->ctcefd < 0 ) )
    {
        CTCE_RESTART_CCWTRACE( pDEVBLK );

        if ( pDEVBLK->filename[1] != '?' )
        {
            pDEVBLK->filename[1] = '?';
        }
        if( !IS_CTCE_CCW_SCB( pDEVBLK->ctcexCmd ) )
        {
            *pUnitStat = 0;
        }
        return ;
    }
    pCTCE_Info->sent = 1;

    pDEVBLK->ctce_UnitStat = *pUnitStat;

    // We select the device block buffer used for sending CTC commands
    // to be different from the one used for receiving CTC commands.
    pSokBuf = ( CTCE_SOKPFX* ) ( pDEVBLK->buf + ( pDEVBLK->ctce_buf_next_read ? pDEVBLK->bufsize / 2 : 0 ) );

    pSokBuf->CmdReg = pDEVBLK->ctcexCmd;
    pSokBuf->FsmSta = pDEVBLK->ctcexState;
    pSokBuf->sCount = sCount;
    pSokBuf->PktSeq = ++pDEVBLK->ctcePktSeq;
    pSokBuf->SndLen = pDEVBLK->ctceSndSml;
    pSokBuf->devnum = pDEVBLK->devnum;
    pSokBuf->ssid   = pDEVBLK->ssid;

    // Only a (non-WEOF) write command data includes sending the IOBuf.
    if( IS_CTCE_CCW_WRT( pDEVBLK->ctcexCmd ) )
    {
        memcpy( ( BYTE * ) pSokBuf + sizeof( CTCE_SOKPFX ), pIOBuf, sCount );

        // Increase the SndLen if the sCount is too large.
        if( pSokBuf->SndLen < ( sCount + sizeof( CTCE_SOKPFX ) ) )
            pSokBuf->SndLen = ( sCount + sizeof( CTCE_SOKPFX ) );

        // If bufsize (init from the MTU parameter) is not large enough
        // then we will have a severe error as the CTC will not connect.
        if( ( pDEVBLK->bufsize / 2 ) < pSokBuf->SndLen )
        {
            WRMSG( HHC05073, "S",  /* CTCE: bufsize parameter %d is too small; increase at least to %d */
                CTCX_DEVNUM( pDEVBLK ), pDEVBLK->bufsize / 2, pSokBuf->SndLen );
        }
    }

    // Write all of this to the other (y-)side.
    rc = write_socket( pDEVBLK->fd, ( BYTE * ) pSokBuf, pSokBuf->SndLen );

    if( rc < 0 )
    {
        WRMSG( HHC05074, "E",  /* CTCE: Error writing to %s: %s */
            CTCX_DEVNUM( pDEVBLK ), pDEVBLK->filename, strerror( HSO_errno ) );

        CTCE_ERROR_CCWTRACE( pDEVBLK );

        // We will try a CTCE Recovery in case of the send error when not shutting down.
        if( !sysblk.shutdown )
        {
            (void) CTCE_Recovery( pDEVBLK ) ;
        }

        // This looks like a 'broken connection' situation so we
        // set intervention required.  (An equipment check and
        // returning to the not ready state was found to not work.)
        pDEVBLK->sense[0] = ( SENSE_IR | SENSE_OC );
        *pUnitStat = CSW_CE | CSW_DE | CSW_UC;
        return;
    }

    // If this command is a matching one for the other (y-)side
    // Working(D) state, then that (y-)side becomes available.
    if IS_CTCE_MATCH( pCTCE_Info->actions )
    {
        SET_CTCE_YAV( pDEVBLK->ctceyState );
    }

    // If we received a command that is going to put our (x-)side
    // in a Working(D) state, then we will need to wait until a
    // matching command arrives from the other (y-)side.  The WAIT
    // timeout is chosen to be long enough to not timeout over periods
    // if inactivity; we experienced up to exactly 15 minutes following
    // (non-matching) PREPARE CCW commands, so we set it to 1000 sec.
    if( IS_CTCE_WAIT( pCTCE_Info->actions ) )
    {

        // Produce a CTCE Trace logging if requested.
        if( CTCE_CCWTRACE( pDEVBLK ) )
        {
            CTCE_Trace( pDEVBLK, CTCE_SND, pCTCE_Info, pUnitStat );
        }

        obtain_lock( &pDEVBLK->ctceEventLock );
        RELEASE_DEVLOCK( pDEVBLK );

        pCTCE_Info->wait_rc = timed_wait_condition_relative_usecs(
            &pDEVBLK->ctceEvent,
            &pDEVBLK->ctceEventLock,
            1000000000,
            NULL );

        OBTAIN_DEVLOCK( pDEVBLK );
        release_lock( &pDEVBLK->ctceEventLock );

        // Trace the non-zero WAIT RC (e.g. timeout, RC=138 (windows) or 110 (unix)).
        if( pCTCE_Info->wait_rc != 0 )
        {
            CTCE_ERROR_CCWTRACE( pDEVBLK );
            CTCE_Trace( pDEVBLK, CTCE_SND, pCTCE_Info, pUnitStat );
        }

        // A WRITE EOF command from the other side will have resulted
        // in the WEOF flag being set.  If this was a matching command
        // for a READ then unit exception needs to be included.
        else if( IS_CTCE_WEOF( pDEVBLK->ctcexState ) &&
                 IS_CTCE_CCW_RDA( pDEVBLK->ctcexCmd ) )
        {
            *pResidual = 0;
            *pUnitStat  = CSW_CE | CSW_DE | CSW_UX;

            // Produce a trace logging if requested.
            if( CTCE_CCWTRACE( pDEVBLK ) )
            {
                CTCE_Trace( pDEVBLK, CTCE_SND, pCTCE_Info, pUnitStat );
            }
            return;
        }
    } // if( IS_CTCE_WAIT( pCTCE_Info->actions ) )

    // Command collisions or resets (CSW_UC) never return data.
    if( 0
        || ( pDEVBLK->ctce_UnitStat == (CSW_BUSY | CSW_ATTN) )
        || IS_CTCE_RST( pDEVBLK->ctceyCmd )
        || ( pDEVBLK->ctce_UnitStat & CSW_UC ) )
    {
        *pResidual = sCount;
    }

    // If the command (by now matched) was a READ command, then the
    // other (y-)side data is available in the alternate device
    // buffer.  We copy that data into the IO buffer, but only
    // transfer the minimum of the current READ sCount and the
    // original WRITE sCount we received from the other (y-)side.
    else if( IS_CTCE_CCW_RED( pDEVBLK->ctcexCmd ) )
    {

        // We switch to the alternate device block buffer for the
        // next CTCE_Send to use; CTCE_Recv already did this.
        pDEVBLK->ctce_buf_next_read ^= 1;

        // We point to the device block buffer holding the matching
        // WRITE command data which CTCE_Recv received.
        pSokBuf_written = ( CTCE_SOKPFX* ) ( pDEVBLK->buf +
            ( pDEVBLK->ctce_buf_next_read ? pDEVBLK->bufsize / 2 : 0 ) );

        // We copy the correct amount of WRITE command data from the
        // device block buffer to the IO buffer and compute Residual.
        memcpy( pIOBuf, ( BYTE * ) pSokBuf_written + sizeof(CTCE_SOKPFX),
            ( sCount <= pSokBuf_written->sCount )
            ? sCount :  pSokBuf_written->sCount );
        *pResidual = sCount - (
            ( sCount <= pSokBuf_written->sCount )
            ? sCount :  pSokBuf_written->sCount );
    }
    else
    {
        *pResidual = 0;
    }

    // The final UnitStat may have been amended by CTCE_Recv like when
    // it received a matching command (typically resulting in CE + DE).
    // We need to merge this.
    *pUnitStat |= pDEVBLK->ctce_UnitStat;
    pDEVBLK->ctce_UnitStat = 0;

    return;

} // CTCE_Send

// ---------------------------------------------------------------------
// CTCE_RecvThread
// ---------------------------------------------------------------------

static void*  CTCE_RecvThread( void* argp )
{
    DEVBLK        *pDEVBLK = (DEVBLK*) argp;     // argp is the device block pointer
    CTCE_SOKPFX   *pSokBuf;                      // overlay for buf inside DEVBLK
    CTCE_INFO      CTCE_Info;                    // CTCE information (also for tracing)
    int            iLength = 0;
    U64            ctcePktCnt = 0;               // Recvd Packet Count
    U64            ctceBytCnt = 0;               // Recvd Byte Count
    BYTE           ctce_recv_mods_UnitStat;      // UnitStat modifications
    int            i = 0;                        // temporary variable

    // When the receiver thread is (re-)started, the CTCE devblk is (re-)initialized
    OBTAIN_DEVLOCK( pDEVBLK );

    // Enhanced CTC adapter intiialization for y-side command register.
    pDEVBLK->ctceyCmd = 0x00;

    // We initialize the device buffer selector variables.
    pDEVBLK->ctce_buf_next_write = pDEVBLK->ctce_buf_next_read = 0;

    // We select the initial device block buffer used for receiving CTC
    // commands to be different from the one used for sending CTC commands.
    pSokBuf = ( CTCE_SOKPFX* ) ( pDEVBLK->buf + ( pDEVBLK->ctce_buf_next_write ? 0 : pDEVBLK->bufsize / 2 ) );

    // CTCE DEVBLK (re-)initialisation completed.
    RELEASE_DEVLOCK( pDEVBLK );

    // Initialise our CTCE_Info as needed.
    CTCE_Info.de_ready_attn_rc   = 0;
    CTCE_Info.working_attn_rc    = 0;
    CTCE_Info.busy_waits         = 0;

    // This thread will loop until we receive a zero-length packet caused by CTCE_Close from the other side.
    for( ; ; )
    {
        // We read whatever the other (y-)side of the CTC has sent us,
        // which by now won't block until the complete buffer is received.
        iLength = read_socket( pDEVBLK->ctcefd, ( BYTE * ) pSokBuf, pDEVBLK->ctceSndSml );

        // Followed by the receiving the rest if the default SndLen was too small.
        if( ( pDEVBLK->ctceSndSml < pSokBuf->SndLen ) && ( iLength != 0 ) )
            iLength += read_socket( pDEVBLK->ctcefd, ( BYTE * ) pSokBuf + pDEVBLK->ctceSndSml,
                pSokBuf->SndLen - pDEVBLK->ctceSndSml );

        // Commands sent by the other (y-)side most likely cause DEVBLK
        // changes to our (x-)side and thus need to be lock protected.
        OBTAIN_DEVLOCK( pDEVBLK );

        // An iLength==0 means the other end has closed down the connection,
        // an iLength<0  means a receive error, which in a non-abnormal case
        // can be caused by a DETACH of the CTCE device.  In both cases we
        // close down this thread.
        if( iLength <= 0 )
        {
            if( iLength < 0 )
            {
                WRMSG( HHC05077, "E",  /* CTCE: Error reading from %s: %s */
                    CTCX_DEVNUM( pDEVBLK ), pDEVBLK->filename, strerror ( HSO_errno ) );
                CTCE_ERROR_CCWTRACE( pDEVBLK );
            }

            // When this was not caused by a read error, or when shutting down, then we close the CTCE device.
            if( ( iLength == 0 ) || ( sysblk.shutdown ) )
            {
                // We report some statistics.
                WRMSG( HHC05076, "I",  // CTCE: Connection closed; %"PRIu64" MB received in %"PRIu64" packets from %s; shutdown=%d"
                    CTCX_DEVNUM( pDEVBLK ), ctceBytCnt >> SHIFT_MEGABYTE , ctcePktCnt, pDEVBLK->filename, sysblk.shutdown );

                CTCE_Close( pDEVBLK );
            }

            // A CTCE Recovery (i.e. a devinit) is done in either case, error
            // or partner shutdown, as long as we are not shutting down.
            if( !sysblk.shutdown )
            {
                (void) CTCE_Recovery( pDEVBLK ) ;
                CTCE_RESTART_CCWTRACE( pDEVBLK );
            }
            RELEASE_DEVLOCK( pDEVBLK );
            return NULL;    // make compiler happy
        } // if( iLength <= 0 )

        // As CTCE Reset received from the other (y-)side will cause our (x-)side
        // sense byte 0 bit 1 (Intervention Required) to be set,
        // and also bit 7 (Interface Discconect / Operation Check).
        if( IS_CTCE_RST( pSokBuf->CmdReg ) )
        {
            pDEVBLK->sense[0] |= ( SENSE_IR | SENSE_OC );

            // Any WRITE data in the reception buffer won't be used anymore.
            pDEVBLK->ctce_buf_next_read = pDEVBLK->ctce_buf_next_write;

            // The remote extended mode setting was also reset to its initial value.
            pDEVBLK->ctce_remote_xmode = (*(pDEVBLK->devid+6) == 0x00 ) ? 1 : 0 ;

        }

        // As SAS / SID commands are never sent accross, anything else we receive
        // from the y-side will cause a reset of sense byte 0 bits 1 and 7.
        else
        {
           pDEVBLK->sense[0] &= ~( SENSE_IR | SENSE_OC );
           pDEVBLK->ctce_system_reset = 0;
        }

        // Update the Receive statistics counters.
        ctcePktCnt += 1 ;
        ctceBytCnt += iLength ;

        // Initialise the UnitStat modifications.
        ctce_recv_mods_UnitStat = 0;

        // Save the previous CTCE states both on
        // our (x-)side as well as the other (y-)side.
        CTCE_Info.state_x_prev = pDEVBLK->ctcexState;
        CTCE_Info.state_y_prev = pDEVBLK->ctceyState;

        // Assume no contention loser or winner.
        CTCE_Info.con_lost = 0;
        CTCE_Info.con_won = 0;

        // If the other (y-)side sent us a dependent command that would
        // result in a BUSY+ATTN device status, then we have a command
        // collision, caused by the crossing of the transmissions.
        // (Please note that a non-dependent command is just executed,
        // i.e. treated as if it came just ahead of our dependent one.)
        // We use the FSM table to make the decision :
        if( ( CSW_BUSY | CSW_ATTN ) ==  CTCE_Fsm[CTCE_CMD( pSokBuf->CmdReg )]
                                                [CTCE_Y_STATE_FSM_IDX].x_unit_stat
            && IS_CTCE_CCW_DEP( pSokBuf->CmdReg ) )
        {

            // We will cancel our (x-)side dependent command provided
            // the (y-)side command was NOT a Write EOF, and that we
            // are currently the contention losing side.  This avoids
            // the deadlock when both sides would wait for a matching
            // command that could never arrive.
            if( pDEVBLK->ctce_contention_loser )
            {

                // This is done by signaling our awaiting (x-)side as
                // if a matching command was received after having
                // reset our state to the prior Available state and
                // and ensuring a resulting BUSY+ATTN device status.
                SET_CTCE_YAV( pDEVBLK->ctcexState );
                pDEVBLK->ctce_UnitStat = CSW_BUSY | CSW_ATTN;
                obtain_lock( &pDEVBLK->ctceEventLock );
                signal_condition( &pDEVBLK->ctceEvent );
                release_lock( &pDEVBLK->ctceEventLock );

                // For CTCE_Trace purposes ...
                CTCE_Info.con_lost = 1;
            }
            else
            {
                CTCE_Info.con_won = 1;
            }
        }

        // When no command collision occurred or our (x-)side is not the
        // contention winner, the other (y-)side command received will be acted upon.
        if( CTCE_Info.con_won != 1 )
        {

            // The command received from the other (y-)side may cause a
            // state transition on our (x-)side, as well as some actions.
            // Both depend on our current (x-)side state and are encoded
            // within the FSM table.
            CTCE_Info.state_new = CTCE_NEW_Y_STATE( pSokBuf->CmdReg );
            CTCE_Info.actions     = CTCE_Fsm[CTCE_CMD( pSokBuf->CmdReg )]
                                            [CTCE_Y_STATE_FSM_IDX].actions;
            CTCE_Info.x_unit_stat = CTCE_Fsm[CTCE_CMD( pSokBuf->CmdReg )]
                                            [CTCE_Y_STATE_FSM_IDX].x_unit_stat;

            // SEM / SBM commands need to be acted upon.
            if( IS_CTCE_CCW_SEM( pSokBuf->CmdReg ) )
            {
                pDEVBLK->ctce_remote_xmode = 1;
            }
            else if( IS_CTCE_CCW_SBM( pSokBuf->CmdReg ) )
            {
                pDEVBLK->ctce_remote_xmode = 0;
            }

            // Device-End status indicating ready will be presented
            // if the y-side has just now become ready.
            CTCE_Info.de_ready = ( IS_CTCE_YNR( pDEVBLK->ctceyState ) &&
                                  !IS_CTCE_YNR( pSokBuf->FsmSta ) ) ? 1 : 0;

            // Our (x-)side knowledge from the other (y-)side is updated.
            pDEVBLK->ctceyState = pSokBuf->FsmSta;
            pDEVBLK->ctceyCmd =  pSokBuf->CmdReg;

            // If the other (y-)side sent us a WRITE command then we
            // retain the device block buffer to be able to pass the
            // transferred data to the matching READ command.  Hence
            // that we switch to the alternate device block buffer.
            if( IS_CTCE_CCW_WRT( pDEVBLK->ctceyCmd ) )
            {
                pDEVBLK->ctce_buf_next_write ^= 1;
                pSokBuf = ( CTCE_SOKPFX* ) ( pDEVBLK->buf +
                    ( pDEVBLK->ctce_buf_next_write ? 0 : pDEVBLK->bufsize / 2 ) );
            }

            // If the other side sent us a WRITE EOF command
            // then we just set the WEOF flag on our side.
            else if( IS_CTCE_CCW_WEF( pDEVBLK->ctceyCmd ) )
            {
                SET_CTCE_WEOF( pDEVBLK->ctcexState );
            }

            // If the other side sent us a READ or READBK command whilst the
            // previous command at our (x-) side was a WRITE EOF command then
            // the other side will have generated a Unit Exception to the WEOF
            // setting, effectively discarding that READ command.  We therefore
            // ignore this READ command, but we need to set the resulting
            // state to Available.  We clear the Wait + Attention actions.
            else if( IS_CTCE_CCW_RDA( pDEVBLK->ctceyCmd ) &&
                     IS_CTCE_CCW_WEF( pDEVBLK->ctcexCmd ) &&
                     IS_CTCE_ATTN( CTCE_Info.actions ) )
            {
                SET_CTCE_YAV( pDEVBLK->ctceyState );
                CLR_CTCE_WAIT( CTCE_Info.actions );
                CLR_CTCE_ATTN( CTCE_Info.actions );
            }

            // If the other (y-)side sent us a matching command for our
            // (x-)side Working(D) state, then we need to signal that
            // condition so that CTCE_Send no longer needs to wait.
            if( IS_CTCE_MATCH( CTCE_Info.actions ) )
            {

                // Our (x-)side returns to the available state, but the
                // other (y-)side is decided for via the FSM table entry.
                SET_CTCE_YAV( pDEVBLK->ctcexState );
                pDEVBLK->ctceyState = CTCE_Info.state_new;

                // All matching commands result in a final UnitStat
                // CE + DE stat at the local device end.
                ctce_recv_mods_UnitStat = CTCE_Info.x_unit_stat;

                // We now signal CTCE_Send that the MATCHing command has been received.
                obtain_lock( &pDEVBLK->ctceEventLock );
                signal_condition( &pDEVBLK->ctceEvent );
                release_lock( &pDEVBLK->ctceEventLock );
            } // if( IS_CTCE_MATCH( CTCE_Info.actions ) )

            // If the other (y-)side sent us a Device-End status
            // indicating Ready then this has to be presented on this side.
            else if( CTCE_Info.de_ready )
            {
                RELEASE_DEVLOCK( pDEVBLK );
                {
                    ctce_recv_mods_UnitStat = CSW_DE;

                    // We may receive a de_ready from the other y-side before
                    // our x-side is ready for it, in which case we retry.
                    do
                    {
                        CTCE_Info.de_ready_attn_rc = device_attention( pDEVBLK, CSW_DE );
                    }
                    while ((CTCE_Info.de_ready_attn_rc == 3) && (USLEEP( 100 ) == 0));
                }
                OBTAIN_DEVLOCK( pDEVBLK );

                // Another attention would be harmful and is not needed.
                CLR_CTCE_ATTN( CTCE_Info.actions );
            }

            // If the other (y-)side sent us a command that may require
            // us to signal attention then we will do so provided no
            // program chain is in progress (SA22-7203-00, item 2.1.1,
            // second paragraph).  We test for that condition using the
            // Command Chaining flag on the last received CCW.
            CTCE_Info.attn_can = 0;
            if( IS_CTCE_ATTN( CTCE_Info.actions )
                && ( !pDEVBLK->ctce_ccw_flags_cc )
                && ( CTCE_Info.con_lost == 0 ) )
            {

                // Produce a CTCE Trace logging if requested.
                if( CTCE_CCWTRACE( pDEVBLK ) )
                {

                    // Disable ATTN RC reporting this time.
                    CTCE_Info.working_attn_rc = -1;
                    CTCE_Trace( pDEVBLK, CTCE_RCV, &CTCE_Info, &ctce_recv_mods_UnitStat );
                }

                // The device_attention might not work on the first
                // attempt due to the fact that we need to release
                // the device lock around it, merely because that
                // routine obtains and releases the device lock.
                // During that short period, one or more commands
                // may have come in between, causing a device busy
                // and a possible other (y-)side status update. So
                // we may need to re-try the ATTN if needed at all.
                RELEASE_DEVLOCK( pDEVBLK );
                CTCE_Info.working_attn_rc = 1;
                for( CTCE_Info.busy_waits = 0;
                     ( CTCE_Info.working_attn_rc == 1 ) &&
                     ( CTCE_Info.attn_can == 0 ) &&
                     ( CTCE_Info.busy_waits <= 20 ) ;
                     CTCE_Info.busy_waits++ )
                {

                    // To circumvent a bug in VM/SP causing SIO timeout errors, we can
                    // insert a 200 msec delay or so allowing the previous CCW to complete,
                    // which needs to be configured using the CTCE option ATTNDELAY <nnn>
                    if( pDEVBLK->ctce_attn_delay && CTCE_Info.busy_waits == 0 )
                    {
                        USLEEP( pDEVBLK->ctce_attn_delay );
                    }

                    CTCE_Info.working_attn_rc = device_attention( pDEVBLK, CSW_ATTN );

                    // ATTN RC=1 means a device busy status did
                    // appear so that the signal did not work.
                    // We will retry after some (increasingly)
                    // small amount of time.
                    if( CTCE_Info.working_attn_rc == 1 )
                    {
                        if( CTCE_Info.busy_waits == 0 )
                        {
                            i = 10;
                        }
                        else
                        {
                            i = i * 2;
                        }
                        USLEEP(i);

                        // Cancel the ATTN in case a CCW program
                        // has started in the mean time.
                        if ( pDEVBLK->ctce_ccw_flags_cc )
                        {
                            CTCE_Info.attn_can = 1;
                        }
                    }
                }
                OBTAIN_DEVLOCK( pDEVBLK );

                // We will show the ATTN status if it was signalled.
                if( CTCE_Info.working_attn_rc == 0 )
                {
                    ctce_recv_mods_UnitStat = CSW_ATTN;
                }
                CTCE_Info.busy_waits -= 1;
            } // if( IS_CTCE_ATTN( CTCE_Info.actions ) && ... /* Attention Needed */
            else if( IS_CTCE_ATTN( CTCE_Info.actions ) )
            {
                CTCE_Info.busy_waits = 0;
                CTCE_Info.attn_can = 1;
            }
        }

        // Merge any UnitStat modifications into the final one.
        pDEVBLK->ctce_UnitStat |= ctce_recv_mods_UnitStat;

        // Produce a CTCE Trace logging if requested.
        if( CTCE_CCWTRACE( pDEVBLK )
            || ( ctce_recv_mods_UnitStat == ( CSW_BUSY | CSW_ATTN ) )
            || ( CTCE_Info.de_ready_attn_rc != 0 )
            || ( ( CTCE_Info.working_attn_rc  != 0 ) && ( CTCE_Info.attn_can == 0 ) )
            || ( CTCE_Info.busy_waits       >= 3 ) )
        {
            if( ctce_recv_mods_UnitStat != 0 )
            {
                ctce_recv_mods_UnitStat = pDEVBLK->ctce_UnitStat;
            }
            CTCE_Trace( pDEVBLK, CTCE_RCV, &CTCE_Info, &ctce_recv_mods_UnitStat );
        }
        CTCE_Info.de_ready_attn_rc = 0;
        CTCE_Info.working_attn_rc  = 0;
        CTCE_Info.busy_waits       = 0;

        RELEASE_DEVLOCK( pDEVBLK );
    }

} // CTCE_RecvThread

// ---------------------------------------------------------------------
// CTCE_Reset -- Selective Reset or System Reset a CTCE adapter device
// ---------------------------------------------------------------------

static void CTCE_Reset( DEVBLK* pDEVBLK )
{
    BYTE            UnitStat = 0;           // Only used as CTCE_Send needs it.
    CCWC            Residual = 0;           // Only used as CTCE_Send needs it.
    CTCE_INFO       CTCE_Info = { 0 };      // CTCE information (also for tracing)

    // The caller already did an OBTAIN_DEVLOCK( pDEVBLK ).

    // Initialise our CTCE_Info previous x- and y-states for CTCE_Trace.
    CTCE_Info.state_x_prev       = pDEVBLK->ctcexState;
    CTCE_Info.state_y_prev       = pDEVBLK->ctceyState;

    // A system reset at power up initialisation must result in
    // sense byte 0 bit 1 (Intervention Required) to be set,
    // and also bit 7 (Interface Discconect / Operation Check).    
    if( pDEVBLK->ctce_system_reset )
    {
        pDEVBLK->sense[0] = ( SENSE_IR | SENSE_OC );
    }

    // Reset the y-command register to 0, clear any WEOF state,
    // and reset our (x-)side (only) to initial extended mode.
    pDEVBLK->ctceyCmd = bCode_reset;
    CLR_CTCE_WEOF( pDEVBLK->ctcexState );
    pDEVBLK->ctcxmode = (*(pDEVBLK->devid+6) == 0x00 ) ? 1 : 0 ;

    // We cancel any Working(D) wait state at our (x-)side.
    if( IS_CTCE_YWK( pDEVBLK->ctcexState ) )
    {
        obtain_lock( &pDEVBLK->ctceEventLock );
        signal_condition( &pDEVBLK->ctceEvent );
        release_lock( &pDEVBLK->ctceEventLock );

        pDEVBLK->ctce_UnitStat = CSW_CE | CSW_DE;
    }
    if( IS_CTCE_YWK( pDEVBLK->ctceyState ) )
    {
        SET_CTCE_YAV( pDEVBLK->ctceyState );
    }

    // The CTCE_Reset is being signaled to our (x-)side as the result of
    // a Halt or Clear subchannel instruction.  We procees this using a
    // synthetic reset (RST) command, which is encdoded as CCW command 0x00
    // because that's what the y command register needs to be set to.  This 0x00
    // comamnd however would be an invalid "bCode" argument for CTCE_ExecuteCCW.
    // Our FSM table however caters for (part of) the work to be done :
    pDEVBLK->ctcexCmd = bCode_reset;
    CTCE_Info.fsm         = CTCE_Fsm[CTCE_CMD( bCode_reset )][CTCE_X_STATE_FSM_IDX];

    CTCE_Info.state_new   = CTCE_NEW_X_STATE(  bCode_reset ); /* Silly : only need for CTCE_Trace MISMATCH avoidance */
    pDEVBLK->ctcexState   = CTCE_NEW_X_STATE(  bCode_reset );

    CTCE_Info.actions      = CTCE_Fsm[CTCE_CMD( bCode_reset )][CTCE_X_STATE_FSM_IDX].actions;
    UnitStat               = CTCE_Fsm[CTCE_CMD( bCode_reset )][CTCE_X_STATE_FSM_IDX].x_unit_stat;

    // Any WRITE data in the reception buffer won't be used anymore.
    pDEVBLK->ctce_buf_next_read = pDEVBLK->ctce_buf_next_write;

    // In most cases we need to inform the other (y-)side.
    if( IS_CTCE_SEND( CTCE_Info.actions ) )
    {
        CTCE_Send( pDEVBLK, 0, NULL, &UnitStat, &Residual, &CTCE_Info );
    }

    // We restart startup tracing if applicable.
    CTCE_RESTART_CCWTRACE( pDEVBLK ) ;
    if( CTCE_CCWTRACE( pDEVBLK ) )
    {
        CTCE_Trace( pDEVBLK, ( CTCE_Info.sent ? CTCE_SND :
            ( ( pDEVBLK->ctcefd < 0 ) ? CTCE_SND_NSR : CTCE_SND_NS ) ),
            &CTCE_Info, &UnitStat );
    }
} // CTCE_Reset

// ---------------------------------------------------------------------
// CTCE_ChkSum
// ---------------------------------------------------------------------
//
// Subroutine to compute a XOR-based checksum for use in debug messages.
//

U32 CTCE_ChkSum(const BYTE* pBuf, const U16 BufLen)
{
    U32            i;
    U32            XORChk = 0;                   // XOR of buffer for checking
    BYTE          *pXOR = (BYTE*)&XORChk;        // -> XORChk


    // We initialize the result with the buffer length so that
    // different length zero buffers yield a different checksum.
    XORChk = BufLen;
    for(i = 0; i < BufLen; i++)
    {
        if( (i % 4) == 0 )
        {
           pXOR = (BYTE*)&XORChk;
        }
        *pXOR++ ^= *pBuf++;
    }
    return XORChk;

} // CTCE_ChkSum

// ---------------------------------------------------------------------
// CTCE_Trace
// ---------------------------------------------------------------------
//
// Subroutine to produce a CTCE trace logging when requested.
//

void            CTCE_Trace(       DEVBLK*             pDEVBLK,
                            const enum CTCE_Cmd_Xfr   eCTCE_Cmd_Xfr,
                            const CTCE_INFO*          pCTCE_Info,
                            const BYTE*               pUnitStat )
{
    static char *CTCE_XfrStr[5] = {
        "--" ,  //  0 = CTCE_LCL
        "->" ,  //  1 = CTCE_SND
        "<-" ,  //  2 = CTCE_RCV
        "-|" ,  //  3 = CTCE_SND_NS
        "||"    //  4 = CTCE_SND_NSR
    };
    BYTE           ctce_Cmd;                   // CTCE command being traced
    BYTE           ctce_PktSeq;                // Packet Sequence number traced
    CTCE_SOKPFX   *pSokBuf;                    // overlay for buf inside DEVBLK
    BYTE           ctce_state_verify;          // CTCE state to be verfified
    char           ctce_state_l_xy[2];         // CTCE X+Y states, left
    char           ctce_state_r_xy[2];         // CTCE X+Y stares, right
    char           ctce_trace_stat[24];        // to contain " Stat=.. CC=. w=.,r=."
    char           ctce_trace_xtra[256];       // to contain extra info when tracing
    char           ctce_trace_xtra_temp[256];  // temporary work area for the above
    char           ctce_devnum[8];             // for ccwtrace packet trace information

    // The source for reporting dependings on the Command X-fer
    // direction.  The CTCE states are reported in lower case,
    // but a changed state is highlighted in upper case.
    if( eCTCE_Cmd_Xfr == CTCE_RCV )
    {
        pSokBuf = ( CTCE_SOKPFX* ) ( pDEVBLK->buf +
            ( pDEVBLK->ctce_buf_next_write ^ IS_CTCE_CCW_WRT( pDEVBLK->ctceyCmd )
            ? 0 : pDEVBLK->bufsize / 2 ) );
        ctce_Cmd    = pSokBuf->CmdReg;
        ctce_PktSeq = pSokBuf->PktSeq;
        ctce_state_r_xy[0] = 32 + *CTCE_StaStr[CTCE_STATE( pCTCE_Info->state_x_prev )];
        ctce_state_r_xy[1] = 32 + *CTCE_StaStr[CTCE_STATE( pCTCE_Info->state_y_prev )];
        if( ( pDEVBLK->ctcexState & 0x07 ) == ( pCTCE_Info->state_x_prev & 0x07 ) )
        {
            ctce_state_l_xy[0] = 32 + *CTCE_StaStr[CTCE_STATE( pDEVBLK->ctcexState )];
        }
        else
        {
            ctce_state_l_xy[0] = *CTCE_StaStr[CTCE_STATE( pDEVBLK->ctcexState )];
        }
        if( ( pDEVBLK->ctceyState & 0x07 ) == ( pCTCE_Info->state_y_prev & 0x07 ) )
        {
            ctce_state_l_xy[1] = 32 + *CTCE_StaStr[CTCE_STATE( pDEVBLK->ctceyState )];
        }
        else
        {
            ctce_state_l_xy[1] = *CTCE_StaStr[CTCE_STATE( pDEVBLK->ctceyState )];
        }
        ctce_state_verify = pDEVBLK->ctceyState & 0x07;
    }
    else
    {
        pSokBuf = ( CTCE_SOKPFX* ) ( pDEVBLK->buf +
            ( pDEVBLK->ctce_buf_next_read ^ ( IS_CTCE_CCW_RED( pDEVBLK->ctcexCmd )
            && ( !( IS_CTCE_WAIT( pCTCE_Info->actions ) ) ) )
            ? pDEVBLK->bufsize / 2 : 0 ) );
        ctce_Cmd    = pDEVBLK->ctcexCmd;
        ctce_PktSeq = pDEVBLK->ctcePktSeq;
        ctce_state_l_xy[0] = 32 + *CTCE_StaStr[CTCE_STATE( pCTCE_Info->state_x_prev )];
        ctce_state_l_xy[1] = 32 + *CTCE_StaStr[CTCE_STATE( pCTCE_Info->state_y_prev )];
        if( pDEVBLK->ctcexState == pCTCE_Info->state_x_prev )
        {
            ctce_state_r_xy[0] = 32 + *CTCE_StaStr[CTCE_STATE( pDEVBLK->ctcexState )];
        }
        else
        {
            ctce_state_r_xy[0] = *CTCE_StaStr[CTCE_STATE( pDEVBLK->ctcexState )];
        }
        if( pDEVBLK->ctceyState == pCTCE_Info->state_y_prev )
        {
            ctce_state_r_xy[1] = 32 + *CTCE_StaStr[CTCE_STATE( pDEVBLK->ctceyState )];
        }
        else
        {
            ctce_state_r_xy[1] = *CTCE_StaStr[CTCE_STATE( pDEVBLK->ctceyState )];
        }
        ctce_state_verify = pDEVBLK->ctcexState & 0x07;
    }

    // Report on the device status and CCW Command Chaining flag.
    if( 0
        || ( eCTCE_Cmd_Xfr != CTCE_RCV && ( !IS_CTCE_RST( ctce_Cmd ) ) )
        || ( *pUnitStat != 0 )
        || ( IS_CTCE_MATCH( pCTCE_Info->actions ) ) )
    {
        MSGBUF( ctce_trace_stat,
            "Stat=%02X", *pUnitStat );
    }
    else
    {
        MSGBUF( ctce_trace_stat,
            "       " );
    }
    if( !IS_CTCE_RST( ctce_Cmd ) )
    {
        MSGBUF( ctce_trace_xtra_temp,
            " CC=%d", pDEVBLK->ctce_ccw_flags_cc );
        STRLCAT( ctce_trace_stat, ctce_trace_xtra_temp );
    }
    else
    {
        STRLCAT( ctce_trace_stat, "     " );
    }

    // Report on the alternating device block buffer usage.
    MSGBUF( ctce_trace_xtra_temp,
        " w=%d,r=%d", pDEVBLK->ctce_buf_next_write, pDEVBLK->ctce_buf_next_read );
    STRLCAT( ctce_trace_stat, ctce_trace_xtra_temp );

    ctce_trace_xtra[0] = '\0' ;

    // The other side's entering a "Working" state may
    // require an Attention or not, which will be shown.
    // Please note that the CTCE_ACTIONS_PRT macro in
    // that case will show "ATTN" at the rightmost end.
    if( IS_CTCE_ATTN( pCTCE_Info->actions ) && ( eCTCE_Cmd_Xfr == CTCE_RCV ) )
    {
        if( pCTCE_Info->attn_can )
        {
            STRLCAT( ctce_trace_xtra, "->NONE" );
        }
        else if( pCTCE_Info->working_attn_rc > -1 )
        {
            MSGBUF( ctce_trace_xtra_temp,
                "->RC=%d", pCTCE_Info->working_attn_rc );
            STRLCAT( ctce_trace_xtra, ctce_trace_xtra_temp );
        }
    }

    // The other side's "DE Ready" signalling to be shown.
    if( pCTCE_Info->de_ready )
    {
        MSGBUF( ctce_trace_xtra_temp,
            " DE_READY->RC=%d", pCTCE_Info->de_ready_attn_rc );
        STRLCAT( ctce_trace_xtra, ctce_trace_xtra_temp );
    }

    // "WEOF" means that the "Write End of File" bit is or was set.
    // "WEOF->SET" means it just got set right now, in which case
    // "WEOF->SET->UX" means an Unit Exception (UX) will follow because
    // it got set because of a WEOF command matching a Read command
    // (which actually will clear the WEOF immediately thereafter).
    // "WEOF->CLR" indicates the WEOF bit just got reset.
    if(  IS_CTCE_WEOF( pCTCE_Info->state_x_prev ) ||
         IS_CTCE_WEOF( pDEVBLK->ctcexState      ) )
    {
        STRLCAT( ctce_trace_xtra, " WEOF" );
    }
    if( !IS_CTCE_WEOF( pCTCE_Info->state_x_prev ) &&
         IS_CTCE_WEOF( pDEVBLK->ctcexState      ) )
    {
        STRLCAT( ctce_trace_xtra, "->SET" );
        if( IS_CTCE_MATCH( pCTCE_Info->actions ) )
        {
            STRLCAT( ctce_trace_xtra, "->UX" );
        }
    }
    if(  IS_CTCE_WEOF( pCTCE_Info->state_x_prev ) &&
        !IS_CTCE_WEOF( pDEVBLK->ctcexState      ) )
    {
        STRLCAT( ctce_trace_xtra, "->CLR" );
    }

    // Report on the SCB returned if applicable.
    if( ( eCTCE_Cmd_Xfr != CTCE_RCV ) && IS_CTCE_CCW_SCB( ctce_Cmd ) )
    {
        MSGBUF( ctce_trace_xtra_temp,
            " SCB=%02X=%s", pCTCE_Info->scb, CTCE_CmdStr[CTCE_CMD( pCTCE_Info->scb )] );
        STRLCAT( ctce_trace_xtra, ctce_trace_xtra_temp );
    }

    // Report on the device status.
    if( pCTCE_Info->busy_waits != 0 )
    {
        MSGBUF( ctce_trace_xtra_temp,
            " Busy_Waits=%d", pCTCE_Info->busy_waits );
        STRLCAT( ctce_trace_xtra, ctce_trace_xtra_temp );
    }

    // Report on the WAIT RC if needed.
    if( ( eCTCE_Cmd_Xfr == CTCE_SND ) && ( pCTCE_Info->wait_rc != 0 ) )
    {
        MSGBUF( ctce_trace_xtra_temp,
            " WAIT->RC=%d", pCTCE_Info->wait_rc );
        STRLCAT( ctce_trace_xtra, ctce_trace_xtra_temp );
    }

    // Report on the SENSE byte 1 and 2 if needed.
    if( ( pDEVBLK->sense[0] != 0 ) || ( pDEVBLK->sense[1] != 0 ) )
    {
        MSGBUF( ctce_trace_xtra_temp,
            " SENSE=%02X%02X", pDEVBLK->sense[0], pDEVBLK->sense[1] );
        STRLCAT( ctce_trace_xtra, ctce_trace_xtra_temp );
    }
    else if( IS_CTCE_CCW_SAS( ctce_Cmd ) )
    {
        MSGBUF( ctce_trace_xtra_temp,
            " SENSE=%02X%02X", pCTCE_Info->sas[0], pCTCE_Info->sas[1] );
        STRLCAT( ctce_trace_xtra, ctce_trace_xtra_temp );
    }

    // The "state mismatch" was used for debugging purposes
    // which would show logic errors.
    if( ( pCTCE_Info->state_new != ctce_state_verify )
        && !( ( !pCTCE_Info->sent ) && ( IS_CTCE_SEND( pCTCE_Info->actions ) ) ) )
    {
        MSGBUF( ctce_trace_xtra_temp,
            " CTCE_STATE MISMATCH %s!=%s(:FSM) !",
            CTCE_StaStr[ctce_state_verify],
            CTCE_StaStr[pCTCE_Info->state_new] );
        STRLCAT( ctce_trace_xtra, ctce_trace_xtra_temp );
    }

    // The unit "State mismatch" was used for debugging purposes
    // which would show logic errors.
    if( ( *pUnitStat !=
        ( ( ( eCTCE_Cmd_Xfr == CTCE_RCV ) && ( IS_CTCE_MATCH( pCTCE_Info->actions ) ) )
        ? ( CSW_CE | CSW_DE ) : ( pCTCE_Info->x_unit_stat ) ) )
        && !( *pUnitStat & ( CSW_UC | CSW_UX | CSW_SM ) )
        && !( ( eCTCE_Cmd_Xfr == CTCE_RCV ) && ( IS_CTCE_WAIT( pCTCE_Info->actions ) ) )
        &&  ( ( eCTCE_Cmd_Xfr != CTCE_RCV ) || ( *pUnitStat != 0 ) )
        && !( pCTCE_Info->de_ready ) )
    {
        MSGBUF( ctce_trace_xtra_temp,
            " Stat MISMATCH %02X!=%02X(:FSM) !",
            *pUnitStat, pCTCE_Info->x_unit_stat );
        STRLCAT( ctce_trace_xtra, ctce_trace_xtra_temp );
    }

    // Report a contention loser situation.
    if( pCTCE_Info->con_lost )
    {
        STRLCAT( ctce_trace_xtra, " CON_LOSER" );
    }

    // Report a contention winner situation.
    if( pCTCE_Info->con_won )
    {
        STRLCAT( ctce_trace_xtra, " IGNORED / CON_WINNER" );
    }

    // Report a Halt or Clear causing a Reset.
    if( pDEVBLK->scsw.flag2 & SCSW2_FC_HALT )
    {
        STRLCAT( ctce_trace_xtra, " HALT" );
    }
    if( pDEVBLK->scsw.flag2 & SCSW2_FC_CLEAR )
    {
        STRLCAT( ctce_trace_xtra, " CLEAR" );
    }
    if( IS_CTCE_RST( ctce_Cmd ) &&
       !( pDEVBLK->scsw.flag2 & ( SCSW2_FC_HALT | SCSW2_FC_CLEAR ) ) )
    {
        STRLCAT( ctce_trace_xtra, " RESET" );
    }

    // Temporarily triggered trace will end prematureley on a matching
    // Read or Write command.
    if( ( pDEVBLK->ctce_trace_cntr > 0 ) && IS_CTCE_MATCH( pCTCE_Info->actions ) &&
        ( IS_CTCE_CCW_RED( ctce_Cmd ) || IS_CTCE_CCW_WRT( ctce_Cmd ) ) )
    {
        pDEVBLK->ctce_trace_cntr = 0;
    }

    // Add an ending dot if this ends a temporarily triggered trace.
    if( !pDEVBLK->ccwtrace && ( pDEVBLK->ctce_trace_cntr == 0 ) )
    {
        STRLCAT( ctce_trace_xtra, "." );
    }

/*

HHC05079I <src_dev> CTCE: <direction> <dst_dev> <seq#> cmd=<cmd>=<cmd_hex>
          xy=<x_src><y_src><direction><x_dst><y_dst> l=<length> k=<chksum>
          Stat=<stat> <extra_msgs>

Explanation
        The CTCE device <local_dev> processes a <cmd> (hex value <cmd_hex>).
        The <direction> shows whether it originates locally (the x-side),
        and if it needs to be sent (->) to the remote <remote_dev> device
        (the y-side), or if the command was received (<-) from the y-side.
        The command causes a state transition shown in <x_local><y_local>
        <direction> <x_remote><y_remote>, using single-letter presentations
        for these (p, c, r, w, a, n); the change is highlighted in uppercase.
        (p=Prepare, c=Control, r=Read, w=Write, a=Available, n=Not-Ready).
        The resulting unit device status is shown in hex <stat>.  Extra
        action indications are given in <extra_msgs>, .e.g. WAIT for a
        matching command from the other y-side, raise ATTN at the other
        side which results in ATTN->RC=rc or is canceled ATTN->NONE,
        DE_READY->RC=rc showing DE singalling READY from the other side,
        and End-of-File being set (WEOF->SET) or cleared (WEOF->CLR) or
        just found to be set (WEOF).  WEOF->UX shows when it generates a
        device Unit Exception.  Other <extra_msgs> may appear.

Action
        None.

*/

    WRMSG( HHC05079, "I",  // CTCE: %s %.6s #%04X cmd=%s=%02X xy=%.2s%s%.2s l=%04X k=%08X %s%s%s%s%s%s
        CTCX_DEVNUM( pDEVBLK ), CTCE_XfrStr[eCTCE_Cmd_Xfr],
        pDEVBLK->filename, ctce_PktSeq,
        CTCE_CmdStr[CTCE_CMD( ctce_Cmd )], ctce_Cmd,
        ctce_state_l_xy, CTCE_XfrStr[eCTCE_Cmd_Xfr],
        ctce_state_r_xy, pSokBuf->sCount,
        CTCE_ChkSum( ( BYTE * ) pSokBuf, pSokBuf->SndLen ),
        ctce_trace_stat,
        CTCE_ACTIONS_PRT( pCTCE_Info->actions ),
        ctce_trace_xtra );
    if( pDEVBLK->ccwtrace )
    {
        MSGBUF( ctce_devnum, "%1d:%04X", CTCE_DEVNUM( pDEVBLK ) );
        net_data_trace( pDEVBLK, ( BYTE * ) pSokBuf, pSokBuf->SndLen ,
            ( eCTCE_Cmd_Xfr == CTCE_RCV ) ? '<' : '>', 'D', ctce_devnum, 0 );
    }
    return;

} // CTCE_Trace

// ---------------------------------------------------------------------
// CTCE_Start_ConnectThread
// ---------------------------------------------------------------------

static int  CTCE_Start_ConnectThread( DEVBLK *dev )
{
    char           str_connect[80];    // Thread name for client
    TID            tid_connect;        // Thread ID for client

    // The other (y-)side will only be initialised in case of regular
    // initialisations, not so for recovery-style re-initializations.
    if( !dev->reinit )
    {

        // The initial CTCA Extended Mode setting can be derived from the SetSIDInfo
        // DevMod byte setting (for both sides), which ended up in dev->devid+6 :
        dev->ctcxmode = (*(dev->devid+6) == 0x00 ) ? 1 : 0 ;
        dev->ctce_remote_xmode = dev->ctcxmode;

        // Enhanced CTC adapter intiialization for y-side command register.
        dev->ctceyCmd = 0x00;

        // Enhanced CTC adapter sides are state-aware, with initial
        // state (1) "Available" and (2) "Not Ready" as in column 6 in
        // the table 2.13 in SA22-7203-00, i.e. we consider both
        // sides being in state YNR. ALL Flags are cleared.
        CLR_CTCE_ALLF( dev->ctceyState );
        SET_CTCE_YNR ( dev->ctceyState );

        // Until we are successfully contacted by the other side,
        // we need to mark the "other side Intervention Required".
        // However, device reset actions in "channel.c" will zero the
        // sense bytes, hence that we set dev->ctce_system_reset here
        // which we use in CTCE_Reset afterwards to correct that.
        dev->ctce_system_reset = 1;
        dev->sense[0] = ( SENSE_IR | SENSE_OC );

        // Initialize the CTC lock and condition used to signal
        // reception of a command matching the dependent one.
        initialize_lock( &dev->ctceEventLock );
        initialize_condition( &dev->ctceEvent );

        // Initialize the CTCE Trace counter for no tracing at all.
        // Use "ctc debug { on | off startup } <devnum>" to change this.
        dev->ctce_trace_cntr = CTCE_TRACE_OFF;
    }


    // Enhanced CTC adapter intiialization for command register and CB.
    dev->ctcexCmd = 0x00;

    // Enhanced CTC adapter sides are state-aware, with initial
    // state (1) "Available" and (2) "Not Ready" as in column 6 in
    // the table 2.13 in SA22-7203-00, i.e. we consider both
    // sides being in state YNR. ALL Flags are cleared.
    CLR_CTCE_ALLF( dev->ctcexState );
    SET_CTCE_YNR ( dev->ctcexState );

    // Initialize the 12 bits Send->Recv packet sequence ID with
    // bits 13-12 of the CCUU devnum in bits 15-14, and
    // bits 01-00 of the CCUU devnum in bits 13-12.  This helps
    // distinguishing same-host traffic if the Send-Recv side
    // CCUU's are sufficiently different (e.g. when under VM).
    dev->ctcePktSeq = ((dev->devnum <<  2) & 0xC000) |
                      ((dev->devnum << 12) & 0x3000) ;

    MSGBUF( str_connect, "CTCE %4.4X ConnectThread", dev->devnum);
    str_connect[sizeof( str_connect )-1]=0;
    if( create_thread( &tid_connect, DETACHED, CTCE_ConnectThread, dev, str_connect ) != 0 )
    {
        WRMSG( HHC05069, "E",  // CTCE: create_thread %s error: %s"
            CTCX_DEVNUM( dev ), str_connect, strerror( HSO_errno ) );

        return -1;
    }

    // This completes the CTCE initialisation, which might have been a
    // recovery-style re-initialisation, which we can now mrk as done.
    dev->reinit = 0;

    return 0;

} // CTCE_Start_ConnectThread

// ---------------------------------------------------------------------
// CTCE_ConnectThread
// ---------------------------------------------------------------------

static void*  CTCE_ConnectThread( void* argp )
{
    DEVBLK             *dev = (DEVBLK*) argp;         // argp is the device block pointer
    int                 fd;                           // socket file descriptor
    struct sockaddr_in  addr;                         // socket destination address info
    int                 rc = 0;                       // Return Code
    const int           connect_retry_interval = 500; // in msec
    const int           connect_msg_interval = 600;   // in sec, e,g, 10 minutes
    int                 attempts_counter = 0;         // rotates up to a maximum of ...
    const int           attempts_counter_max =        // results in a msg every 10 min
                            connect_msg_interval *
                            1000 / connect_retry_interval;
    BYTE                renewed;                      // When renewing a CTCE connection
    char*               remaddr;                      // Remote IP address
    char                address[20]="";               // temp space for IP address


    // Obtain a socket to connect to the other end.
    OBTAIN_DEVLOCK( dev );

    // We must keep on trying to connect until it is successful,
    // which is why we do this in a separate dedicated thread.
    do
    {
        if ( ( fd = CTCE_Get_Socket( dev, CTCE_SOK_CON ) ) < 0 )
        {
            RELEASE_DEVLOCK( dev );
            return NULL;
        }

        // Initialise the destination IP address.
        strcpy( address, inet_ntoa( dev->ctce_ipaddr ) );
        remaddr = address;
        memset( &addr, 0, sizeof( addr ) );
        addr.sin_family = AF_INET;
        addr.sin_addr = dev->ctce_ipaddr;
        addr.sin_port = htons( dev->ctce_rport );

        // During the blocking connect() and a possible retry wait interval we release the device lock.
        RELEASE_DEVLOCK( dev );
        rc = connect( fd, ( struct sockaddr * )&addr, sizeof( addr ) );
        if ( rc < 0 )
        {
            close_socket( fd );
            USLEEP(connect_retry_interval * 1000) ;
        }
        OBTAIN_DEVLOCK( dev );

        // A successful connect() is immediately followed by a initial write.
        if ( rc == 0 )
        {
            rc = CTCE_Write_Init( dev, fd );
        }

        // The first message appears just once after (each re-)initialisation,
        // attempt, the second one every connect_message_interval. (= 1200*0.5 sec).
        attempts_counter = ( attempts_counter > attempts_counter_max ) ? 2 : ( attempts_counter + 1 ) ;
        if( ( attempts_counter == 1 ) && ( dev->fd < 0 ) && ( rc < 0 ) )
        {
            WRMSG( HHC05065, "I",  // CTCE: Attempt outbound connection :%5d -> %1d:%04X=%s:%d"
                CTCX_DEVNUM( dev ), dev->ctce_lport, SSID_TO_LCSS(dev->ssid),
                dev->ctce_rccuu, remaddr, dev->ctce_rport );
        }
        else if( ( attempts_counter == attempts_counter_max ) && ( dev->ctce_trace_cntr > 0 ) )
        {
            WRMSG( HHC05072, "I",  // CTCE: Still attempting connection :%5d -> %1d:%04X=%s:%d%s"
                CTCX_DEVNUM( dev ), dev->ctce_lport, SSID_TO_LCSS(dev->ssid),
                dev->ctce_rccuu, remaddr, dev->ctce_rport,
                --dev->ctce_trace_cntr ? "" : "." );
        }

    // We connect to the other side, but this will keep on failing
    // until the other side has started listening with accept(),
    // provided no DETACH or DEVINIT command was issued before.
    } while ( ( rc < 0 ) && dev->allocated);

    // If we connected OK then send/write socket fd is now known.
    if ( rc == 0 )
    {
        renewed = ( dev->fd != -1 ) ? 1 : 0;
        dev->fd = fd;

        WRMSG( HHC05054, "I",  // CTCE: %s outbound connection :%5d -> %1d:%04X=%s:%d"
            CTCX_DEVNUM( dev ), ( renewed ? "Renewed" : "Started" ),
            dev->ctce_connect_lport, SSID_TO_LCSS( dev->ssid ),
            dev->ctce_rccuu, remaddr, dev->ctce_rport );
    }

    // If the connection was not successful then we either encountered an
    // error, or the user decided to quit before starting the other side.
    else
    {
        WRMSG( HHC05078, "I",  // CTCE: Aborted outbound connection :%5d -> %1d:%04X=%s:%d"
            CTCX_DEVNUM( dev ), dev->ctce_lport, SSID_TO_LCSS(dev->ssid),
            dev->ctce_rccuu, remaddr, dev->ctce_rport );
        shutdown( fd, SHUT_RDWR );
        close_socket( fd );
    }

    RELEASE_DEVLOCK( dev );
    return NULL;

} // CTCE_ConnectThread

// ---------------------------------------------------------------------
// CTCE_Get_Socket
// ---------------------------------------------------------------------

static int      CTCE_Get_Socket( DEVBLK*                   dev,
                                 const enum CTCE_Sok_Use   eCTCE_Sok_Use)
{
    int                 fd;                           // socket file descriptor
    const int           so_value_1 = 1;               // argument for setsockopt
    struct sockaddr_in  addr;                         // socket address info
    socklen_t           addrlen;                      // as needed for getsockname() etc.


    // Obtain a new socket.
    if( ( fd = socket(AF_INET, SOCK_STREAM, 0) ) < 0 )
    {
        WRMSG( HHC05050, "E",  // CTCE: Error creating %s socket: %s"
            CTCX_DEVNUM( dev ), (eCTCE_Sok_Use == CTCE_SOK_LIS) ? "listen" : "connect",
            strerror( HSO_errno ) );
        close_socket( fd );
        return -1;
    }

    // Allow previous instance of the socket address to be reused.
    if ( setsockopt( fd, SOL_SOCKET, SO_REUSEADDR,
        ( GETSET_SOCKOPT_T* )&so_value_1, sizeof( so_value_1 ) ) < 0 )
    {
        WRMSG( HHC05051, "E",  // CTCE: %s error for %s socket (port %d): %s"
            CTCX_DEVNUM( dev ), "SO_REUSEADDR",
            (eCTCE_Sok_Use == CTCE_SOK_LIS) ? "listen" : "connect",
            dev->ctce_lport, strerror( HSO_errno ) );
        close_socket( fd );
        return -1;
    }

#if defined(CTCE_DISABLE_NAGLE)
    if ( eCTCE_Sok_Use == CTCE_SOK_CON )
    {

        // Disable the NAGLE protocol as we need responsiveness more than throughput.
        if ( setsockopt( fd, IPPROTO_TCP, TCP_NODELAY,
            ( GETSET_SOCKOPT_T* )&so_value_1, sizeof( so_value_1 ) ) < 0 )
        {
            WRMSG( HHC05051, "E",  // CTCE: %s error for %s socket (port %d): %s"
                CTCX_DEVNUM( dev ), "TCP_NODELAY",
                (eCTCE_Sok_Use == CTCE_SOK_LIS) ? "listen" : "connect",
                dev->ctce_lport, strerror( HSO_errno ) );
            close_socket( fd );
            return -1;
        }
    }
#endif

    // We bind the socket to a local port, which for a listening port
    // is this CTCE device's receiving port, otherwise we bind to a
    // random port (i.e. = 0).
    memset( &( addr ), 0, sizeof( addr ) );
    addr.sin_family = AF_INET;
    if ( eCTCE_Sok_Use == CTCE_SOK_LIS )
    {
        addr.sin_port = htons( dev->ctce_lport );
    }
    else
    {
        addr.sin_port = htons( 0 );
    }

    addr.sin_addr.s_addr = htonl( INADDR_ANY );
    if ( bind( fd, ( struct sockaddr * )&addr, sizeof( addr ) ) < 0 )
    {
        WRMSG( HHC05052, "E",  // CTCE: Error binding to %s socket (port %d): %s"
            CTCX_DEVNUM( dev ),
            ( eCTCE_Sok_Use == CTCE_SOK_LIS ) ? "listen" : "connect",
            dev->ctce_lport, strerror( HSO_errno ) );
        close_socket( fd );
        return -1;
    }

    // A connect socket is always bound to a random lport, which we determine.
    if ( eCTCE_Sok_Use == CTCE_SOK_CON )
    {
        addrlen = sizeof( addr );
        if ( 1
            && getsockname ( fd, ( struct sockaddr * )&addr, &addrlen ) == 0
            && addr.sin_family == AF_INET
            && addrlen == sizeof( addr ) )
        {
            dev->ctce_connect_lport = ntohs( addr.sin_port );
        }
        else
        {
            WRMSG( HHC05053, "E",  // CTCE: Error on getsockname for %s socket (port %d): %s"
                CTCX_DEVNUM( dev ), (eCTCE_Sok_Use == CTCE_SOK_LIS) ? "listen" : "connect",
                dev->ctce_lport, strerror( HSO_errno ) );
            close_socket( fd );
            return -1;
        }
    }

    // By now all possible error cases have been reported and acted upon.
    return fd;

} // CTCE_Get_Socket

// ---------------------------------------------------------------------
// CTCE_Write_Init
// ---------------------------------------------------------------------

static int      CTCE_Write_Init( DEVBLK*                   dev,
                                 const int                 fd )
{
    int                 rc;                           // Return Code
    CTCE_SOKPFX*        pSokBuf;                      // The buffer to be written

    pSokBuf = (CTCE_SOKPFX*) dev->buf;
    pSokBuf->ctce_lport  = dev->ctce_lport;
    pSokBuf->ctce_ipaddr = dev->ctce_ipaddr;
    pSokBuf->SndLen      = dev->ctceSndSml;
    pSokBuf->devnum      = dev->devnum;
    pSokBuf->ssid        = dev->ssid;
    pSokBuf->ctce_herc   = ( dev->ctcefd > 0 ) ? CTCE_HERC_RECV : 0 ; // 0 = we're not yet receiving
    if ( ( rc = write_socket( fd, pSokBuf, pSokBuf->SndLen ) ) == pSokBuf->SndLen )
    {
        rc = 0;
    }
    else
    {
        close_socket( fd );
        WRMSG( HHC05075, "E",  // CTCE: Initial write_socket :%d -> %1d:%04X=%s:%d ; rc=%d!=%d ; error = %s"
            CTCX_DEVNUM( dev ), dev->ctce_lport, SSID_TO_LCSS( dev->ssid ),
            dev->ctce_rccuu, inet_ntoa( dev->ctce_ipaddr ), dev->ctce_rport, rc, pSokBuf->SndLen, strerror( HSO_errno ) );
        rc = -1;
        CTCE_ERROR_CCWTRACE( dev );
    }
    return rc;

} // CTCE_Write_Init

// ---------------------------------------------------------------------
// CTCE_Recovery
// ---------------------------------------------------------------------

static int    CTCE_Recovery( DEVBLK* dev )
{
    char      devnum[7];                                   // devnum to be recovered
    char     *argv[] = { "DEVINIT", ( char* )&devnum };    // to be passed to  devinit_cmd
    int       rc;                                          // Return Code from devinit_cmd

    MSGBUF( devnum, "%1d:%04X", CTCE_DEVNUM( dev ) );

    WRMSG( HHC05086, "I",  // CTCE: Recovery is about to issue Hercules command: %s %s"
        CTCX_DEVNUM( dev ), argv[0], argv[1] );

    RELEASE_DEVLOCK( dev );
    {
        rc = devinit_cmd( sizeof( argv ) / sizeof( argv[0] ), argv, NULL );
    }
    OBTAIN_DEVLOCK( dev );
    return rc;

} // CTCE_Recovery

// ---------------------------------------------------------------------
// CTCE_Build_RCD
// ---------------------------------------------------------------------
static int      CTCE_Build_RCD(  DEVBLK*                   dev,
                                 BYTE*                     buffer,
                                 int                       bufsz )
{

/*-------------------------------------------------------------------*/
/* static template/prototype configuration data                      */
/*-------------------------------------------------------------------*/
/*                                                                   */
/*    SB10-7034-05  ESCON and FICON Channel-to-Channel Reference     */
/*                                                                   */
/* Read Configuration Data (X'C4')                                   */
/*                                                                   */
/* A Read Configuration Data command for a CTC device was only ever  */
/* encountered for FICON CTC (FCTC), never for regular CTC Adapters. */
/* (ESCON CTC adapters so far have not been tested yet.)  The RCD    */
/* command was then implemented similar to the code in tapedev.c.    */
/*                                                                   */
/* A Read Configuration Data command for a FCTC causes 132 bytes of  */
/* be transferred from the control unit to the channel.              */
/*                                                                   */
/*-------------------------------------------------------------------*/
static const BYTE cfgdata[] =       // (prototype data)
{
// ---------------- Local NED -----------------------------------------
0xC8,                               // 0:      NED code
0x01,                               // 1:      Type  (X'01' = I/O Device)
0x09,                               // 2:      Class (X'09' = CTCA)
0x00,                               // 3:      (Reserved)
0xF0,0xF0,0xF3,0xF0,0xF8,0xF8,      // 4-9:    Type  ('003088')
0xC3,0xE3,0xC3,                     // 10-12:  Model ('CTC')
0xC8,0xD9,0xC3,                     // 13-15:  Manufacturer ('HRC' = Hercules)
0xE9,0xE9,                          // 16-17:  Plant of Manufacture ('ZZ' = Herc)
0xF0,0xF0,0xF0,0xF0,                // 18-21:  Sequence Number Bytes 0-3
0xC1,                               // 22:     Sequence Number Byte  4
0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0, // 23-29:  Sequence Number Bytes 5-11
0x00, 0x00,                         // 30-31:  Tag
// ---------------- Specific NED --------------------------------------
0x40,                               // 32:     Flags
0x00,0x00,0x00,                     // 33-35:  (Reserved)
0xF0,0xF0,0xF3,0xF0,0xF8,0xF8,      // 36-41:  Type  ('003088')
0xC3,0xE3,0xC3,                     // 42-44:  Model ('CTC')
0xC8,0xD9,0xC3,                     // 45-47:  Manufacturer ('HRC' = Hercules)
0xE9,0xE9,                          // 48-49:  Plant of Manufacture ('ZZ' = Herc)
0xF0,0xF0,0xF0,0xF0,                // 50-53:  Sequence Number Bytes 0-3
0xC2,                               // 54:     Sequence Number Byte  4
0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0, // 55-61:  Sequence Number Bytes 5-11
0x00, 0x00,                         // 62-63:  Tag
// ---------------- Token NED -----------------------------------------
0xE8,                               // 64:     NED code
0x01,                               // 65:     Type  (X'01' = I/O Device)
0x09,                               // 66:     Class (X'09' = CTCA)
0x00,                               // 67:     (Reserved)
0xF0,0xF0,0xF2,0xF8,0xF1,0xF7,      // 68-73:  Type  ('002817') (z196)
0xC3,0xE3,0xC3,                     // 74-76:  Model ('CTC')
0xC8,0xD9,0xC3,                     // 77-79:  Manufacturer ('HRC' = Hercules)
0xE9,0xE9,                          // 80-81:  Plant of Manufacture ('ZZ' = Herc)
0xF0,0xF0,0xF0,0xF0,                // 82-85:  Sequence Number Bytes 0-3
0xF0,                               // 86:     Sequence Number Byte  4
0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0, // 87-93:  Sequence Number Bytes 5-11
0x00, 0xE1,                         // 94-95:  Tag
// ---------------- General NED ---------------------------------------
0x80,                               // 96:       NED code
0x00,                               // 97:       Record Selector
0x00,0x01,                          // 98-99:    Interface ID
                                    //           x'0001' = local config record
                                    //           x'0002' = remote config record
0x00,0x00,0x00,0x00,0x00,0x00,0x00, // 100-131:  (Reserved)
0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,
};

CASSERT( sizeof(cfgdata) == 132, ctcadpt_c );

    int   RCD_len;
    BYTE  work[ sizeof( cfgdata ) ];

    // Copy prototype Configuration Data to work area.
    memcpy( work, cfgdata, sizeof( work ));

    // Fixup values for this particular FICON CTCE device.
    work[19]    = (dev->devnum      & 0x0FF);   // (set Inbound  IID)
    work[21]    = (dev->ctce_rccuu  & 0x0FF);   // (set Outbound IID)
    work[31]    = (dev->devnum      & 0x0FF);   // (set Unit Address)

    work[19+32] = (dev->ctce_rccuu  & 0x0FF);   // (set Inbound  IID)
    work[21+32] = (dev->devnum      & 0x0FF);   // (set Outbound IID)
    work[31+32] = (dev->ctce_rccuu  & 0x0FF);   // (set Unit Address)

    // Finally, copy the work area into the caller's buffer.
    RCD_len = bufsz < (int) sizeof( work ) ? bufsz : (int) sizeof( work );
    memcpy( buffer, work, RCD_len );

    // Return the number of bytes we provided.
    return RCD_len;

} // CTCE_Build_RCD
