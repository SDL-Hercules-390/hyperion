/* CTCADPT.C    (C) Copyright James A. Pierson, 2002-2012            */
/*              (C) Copyright Roger Bowler, 2000-2012                */
/*              (c) Copyright Vic Cross, 2001-2009                   */
/*              (C) Copyright Peter J. Jansen, 2014-2017             */
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
#include "opcode.h"
#include "devtype.h"

// --------------------------------------------------------------------
// CTCE Info also for use by CTCE Tracing when requested
// --------------------------------------------------------------------

typedef struct _CTCE_INFO
{
    BYTE               state_x_prev;   /* This  side previous state  */
    BYTE               state_y_prev;   /* Other side previous state  */
    BYTE               actions;        /* Triggered by CCW received  */
    BYTE               state_new;      /* The updated FSM state      */
    BYTE               x_unit_stat;    /* Resulting device unit stat */
    BYTE               scb;            /* Last SCB returned          */
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
    int                de_ready_attn_rc;   /* device_attention RC    */
    int                working_attn_rc;    /* device_attention RC    */
                                       /* from transition to         */
                                       /* "Working(D)" state         */
    int                sok_buf_len;    /* socket buffer length       */
}
CTCE_INFO;

// --------------------------------------------------------------------
// CTCE_Cmd_Xfr enumeration type used by CTCE_Trace
// --------------------------------------------------------------------

enum CTCE_Cmd_Xfr
{
    CTCE_LCL,                          /* Cmd remains Local only     */
    CTCE_SND,                          /* Cmd Send to y-side         */
    CTCE_RCV                           /* Cmd Received from y-side   */
};

// ====================================================================
// Declarations
// ====================================================================

static int      CTCT_Init( DEVBLK *dev, int argc, char *argv[] );

static void     CTCT_Read( DEVBLK* pDEVBLK,   U32   sCount,
                           BYTE*   pIOBuf,    BYTE* pUnitStat,
                           U32*    pResidual, BYTE* pMore );

static void     CTCT_Write( DEVBLK* pDEVBLK,   U32   sCount,
                            BYTE*   pIOBuf,    BYTE* pUnitStat,
                            U32*    pResidual );

static void*    CTCT_ListenThread( void* argp );

static void     CTCE_ExecuteCCW( DEVBLK* pDEVBLK, BYTE  bCode,
                                 BYTE    bFlags,  BYTE  bChained,
                                 U32     sCount,  BYTE  bPrevCode,
                                 int     iCCWSeq, BYTE* pIOBuf,
                                 BYTE*   pMore,   BYTE* pUnitStat,
                                 U32*    pResidual );

static int      CTCE_Init( DEVBLK *dev, int argc, char *argv[] );

static void     CTCE_Send( DEVBLK* pDEVBLK,   U32        sCount,
                            BYTE*  pIOBuf,    BYTE*      pUnitStat,
                            U32*   pResidual, CTCE_INFO* pCTCE_Info );

static void*    CTCE_RecvThread( void* argp );

static void*    CTCE_ListenThread( void* argp );

static void     CTCE_Reset( DEVBLK* pDEVBLK );

static U32      CTCE_ChkSum( const BYTE* pBuf, const U16 BufLen );

static void     CTCE_Trace( const DEVBLK*             pDEVBLK,
                            const U32                 sCount,
                            const enum CTCE_Cmd_Xfr   eCTCE_Cmd_Xfr,
                            const CTCE_INFO*          pCTCE_Info,
                            const BYTE*               pCTCE_Buf,
                            const BYTE*               pUnitStat );

static int      CTCE_Connect_Timeout( int                    sockfd,
                                      const struct sockaddr* saptr,
                                      const socklen_t        salen,
                                      const int              usec,
                                      const DEVBLK*          pDEVBLK );

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
    BYTE                CmdReg;        /* CTCE command register      */
    BYTE                FsmSta;        /* CTCE FSM state             */
    U16                 sCount;        /* CTCE sCount copy           */
    U16                 PktSeq;        /* CTCE Packet Sequence ID    */
    U16                 SndLen;        /* CTCE Packet Sent Length    */
    U16                 DevNum;        /* CTCE Sender's devnum       */
    U16                 ssid;          /* CTCE Sender's ssid         */
}
CTCE_SOKPFX;

// --------------------------------------------------------------------
// CTCE Equivalent of CTCG_PARMBLK
// --------------------------------------------------------------------

typedef struct _CTCE_PARMBLK
{
    int                 listenfd[2];   /* [0] = read, [1] = write    */
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
    "???" , // 13 = 15 = Invalid Command Code
    "CB0" , // 14 = 16 = Invalid Command Code Used to Report SCB 0
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
    13, 7, 2, 9,13, 7, 2, 1,13, 7, 2,13, 6, 7, 2, 1,
    13, 7, 2,13, 4, 7, 2, 1,13, 7, 2,13, 6, 7, 2, 1,
    13, 7, 2, 0,11, 7, 2, 1,13, 7, 2,13, 6, 7, 2, 1,
    13, 7, 2,13, 4, 7, 2, 1,13, 7, 2,13, 6, 7, 2, 1
};

/* In base (non-extended) mode the WEOF (WEF) */
/* command does not exist but classifies as   */
/* a regular WRITE command.  The WEOF-to-WRT  */
/* mapping is performed with this macro:      */
#define CTCE_CMD(c)             (pDEVBLK->ctcxmode == 1 ?   (CTCE_command[c]) : \
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
#define CTCE_STATE(c)           (pDEVBLK->ctcxmode == 1 ?  ((c)&0x07) : \
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
/* default is equal to 61592, calculated as   */
/*    sizeof(CTCE_SOKPFX) +                   */
/*    sizeof(U16=pSokBuf->sCount=2) +         */
/*    61578 (=0xF08A)                         */
/* the latter number is the largest data      */
/* sCount seen used by  CTC programs to date. */
/* If that number would be too small one day, */
/* a severe error message will instruct the   */
/* user to specify an increased MTU bufsize   */
/* in the device configuration statement.     */
#define CTCE_MTU_MIN ( (int)( 61578 + sizeof(CTCE_SOKPFX) + sizeof(U16 /* sCount */) ) )

#define CTCE_RESET_TYPE                                                                                             \
      (     ( pDEVBLK->scsw.flag2 & ( SCSW2_FC_HALT                  ) ) )                ? "Halt Reset"      : ""  \
    , (     ( pDEVBLK->scsw.flag2 & (                 SCSW2_FC_CLEAR ) ) )                ? "Clear Reset"     : ""  \
    , ( ( ! ( pDEVBLK->scsw.flag2 & ( SCSW2_FC_HALT | SCSW2_FC_CLEAR ) ) ) &&                                       \
          ! ( IS_CTCE_YWK( pDEVBLK->ctcexState ) | IS_CTCE_YWK( pDEVBLK->ctceyState ) ) ) ? "System Reset"    : ""  \
    , ( ( ! ( pDEVBLK->scsw.flag2 & ( SCSW2_FC_HALT | SCSW2_FC_CLEAR ) ) ) &&                                       \
            ( IS_CTCE_YWK( pDEVBLK->ctcexState ) | IS_CTCE_YWK( pDEVBLK->ctceyState ) ) ) ? "Selective Reset" : ""

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
/*      |||         . CD = CE + DE                                    */
/*      |||         . C  = CE                                         */
/*      |||         . BA = BUSY + ATTN                                */
/*      |||         . B  = BUSY                                       */
/*      |||         . UC = Unit Check                                 */
/*      |||+------- S = Send this commands also to the other (y-)side */
/*      ||||+------ M = a Matching command for the other (y-)side     */
/*      |||||+----- W = our (x-)side must Wait for a matching command */
/*      ||||||+---- A = cause Attention interrupt at the other y-side */
/*      |||||||                                                       */
#define PC_S_W  { P, CSW_CE             , 0, CTCE_SEND              | CTCE_WAIT             }
#define C__S_WA { C, 0                  , 0, CTCE_SEND              | CTCE_WAIT | CTCE_ATTN }
#define R__S_WA { R, 0                  , 0, CTCE_SEND              | CTCE_WAIT | CTCE_ATTN }
#define W__S_WA { W, 0                  , 0, CTCE_SEND              | CTCE_WAIT | CTCE_ATTN }
#define CC_SMW  { C, CSW_CE             , 0, CTCE_SEND | CTCE_MATCH | CTCE_WAIT             }
#define R__SMW  { R, 0                  , 0, CTCE_SEND | CTCE_MATCH | CTCE_WAIT             }
#define W__SMW  { W, 0                  , 0, CTCE_SEND | CTCE_MATCH | CTCE_WAIT             }
#define ACDSM   { A, CSW_CE   | CSW_DE  , 0, CTCE_SEND | CTCE_MATCH                         }
#define ACDS    { A, CSW_CE   | CSW_DE  , 0, CTCE_SEND                                      }
#define AUCS    { A,       CSW_UC       , 0, CTCE_SEND                                      }
#define  CDS    { U, CSW_CE   | CSW_DE  , 0, CTCE_SEND                                      }
#define  CD     { U, CSW_CE   | CSW_DE  , 0, 0                                              }
#define  B      { U, CSW_BUSY           , 0, 0                                              }
#define  BA     { U, CSW_BUSY | CSW_ATTN, 0, 0                                              }
#define  UC     { U,       CSW_UC       , 0, 0                                              }
#define  UCS    { U,       CSW_UC       , 0, CTCE_SEND                                      }

/**********************************************************************/
/* Now finally the CTCE FSM table:                                    */
/**********************************************************************/
static struct CTCE_FsmEnt {
   BYTE new_state;
   BYTE x_unit_stat;
   BYTE y_unit_stat;
   BYTE actions;
}
const CTCE_Fsm[16][8] = {
/* cmd/stat P       C       R       W       A       N       X       I     */
/* PRE */ {ACDSM  , CD    , CD    , CD    ,PC_S_W , UCS   , B     , B     },
/* CTL */ {CC_SMW , BA    , BA    , BA    ,C__S_WA, UCS   , B     , B     },
/* RED */ {R__SMW , BA    , BA    ,ACDSM  ,R__S_WA, UCS   , B     , B     },
/* WRT */ {W__SMW , BA    ,ACDSM  , BA    ,W__S_WA, UCS   , B     , B     },
/* SCB */ { CD    ,ACDSM  , CD    , CD    ,ACDS   , UCS   , B     , B     },
/* nus */ { UC    , UC    , UC    , UC    , UC    , UC    , B     , B     },
/* RBK */ {R__SMW , BA    , BA    ,ACDSM  ,R__S_WA, UCS   , B     , B     },
/* WEF */ { CDS   , BA    ,ACDSM  , BA    , CDS   , UCS   , B     , B     },
/* NOP */ { CD    , BA    , BA    , BA    , CD    , UC    , B     , B     },
/* SEM */ { CDS   , CDS   , CDS   , CDS   ,ACDS   ,AUCS   , B     , B     }, /* C, R, W imply Selective Reset */

/* SAS */ { CD    , CD    , CD    , CD    , CD    , CD    , B     , B     },
/* SID */ { CD    , CD    , CD    , CD    , CD    , CD    , B     , B     },
/* RCD */ { CD    , CD    , CD    , CD    , CD    , CD    , B     , B     },

/* inv */ { UC    , UC    , UC    , UC    , UC    , UC    , B     , B     },
/* CB0 */ { UC    , UC    , UC    , UC    , UC    , UC    , B     , B     },
/* SBM */ { CDS   , CDS   , CDS   , CDS   ,ACDS   ,AUCS   , B     , B     }  /* C, R, W imply Selective Reset */
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
#undef ACDSM
#undef ACDS
#undef AUCS
#undef  CDS
#undef  CD
#undef  B
#undef  BA
#undef  UC
#undef  UCS

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
#define CTCE_UDP

/* The following macro's attempt to maximize source commonality between  */
/* different Hercules versions, whilst adhering to different styles.     */
#define CTCX_DEVNUM(p)          SSID_TO_LCSS(p->ssid), p->devnum
#define CTCE_FILENAME           pDEVBLK->filename + 0

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
                       U32     sCount,  BYTE  bPrevCode,
                       int     iCCWSeq, BYTE* pIOBuf,
                       BYTE*   pMore,   BYTE* pUnitStat,
                       U32*    pResidual )
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
        snprintf(str,sizeof(str),"CTCT %4.4X ListenThread",dev->devnum);
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
    snprintf( dev->filename, sizeof(dev->filename), "%s:%s", remaddr, remotep );
    dev->filename[sizeof(dev->filename)-1] = '\0';
    return 0;
}

//
// CTCT_Write
//

static void  CTCT_Write( DEVBLK* pDEVBLK,   U32   sCount,
                         BYTE*   pIOBuf,    BYTE* pUnitStat,
                         U32*    pResidual )
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
        if( pDEVBLK->ccwtrace || pDEVBLK->ccwstep )
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

static void  CTCT_Read( DEVBLK* pDEVBLK,   U32   sCount,
                        BYTE*   pIOBuf,    BYTE* pUnitStat,
                        U32*    pResidual, BYTE* pMore )
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
    if( pDEVBLK->ccwtrace || pDEVBLK->ccwstep )
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

    snprintf( pBuffer, iBufLen, "%s IO[%"PRIu64"]", filename, pDEVBLK->excps );
}

// -------------------------------------------------------------------
// Close the device (Generic)
// -------------------------------------------------------------------

int  CTCE_Close( DEVBLK* pDEVBLK )
{
    // Close the device file (if not already closed)
    if (pDEVBLK->fd >= 0)
    {
        if (socket_is_socket( pDEVBLK->fd ))
            close_socket( pDEVBLK->fd );
        else
            close( pDEVBLK->fd );
        pDEVBLK->fd = -1;           // indicate we're now closed
    }
    return 0;
}

// ====================================================================
// CTCE Support
// ====================================================================

// CTC Enhanced
// ============
//   Enhanced CTC functionality is designed to emulate real
//   3088 CTC Adapter hardware, using a pair of TCP sockets
//   with a likewise configured Hercules instance on a
//   different PC (or same PC).  The new device type is CTCE.

//   The implementation is based mostly on IBM publications,
//   "ESCON Channel-to-Channel Adapter", SA22-7203-00, and
//   also  "Channel-to-Channel Adapter", SA22-7091-01, although
//   no claim for completeness of this implemenation is feasible.

//   The CTCE configuration is similar to the CTCT device.  The
//   MTU bufsize parameter is optional, but when specified must be
//   >= CTCE_MTU_MIN (=61592). This is the default value when omitted.
//   (Please note that 61592 = sizeof(CTCE_SOKPFX) + sizeof(sCount) + 0xF08A
//   the latter being the maximum sCount experienced in CTC CCW programs.)

//   CTCE requires an even-odd pair of port numbers per device side
//   but only the even port numbers are to be configured; the odd
//   numbers are just derived by adding 1 to the (configured) even
//   port numbers.  The socket connection pairs cross-connect, the
//   arrows showing the send->receive direction :
//
//      x-lport-even -> y-rport-odd
//      x-lport-odd  <- y-rport-even
//
//   A sample CTCE device configuration is shown below:
//
//      Hercules PC Host A with IP address 192.168.1.100 :
//
//         0E40  CTCE  30880  192.168.1.200  30880
//         0E41  CTCE  30882  192.168.1.200  30882
//
//      Hercules PC Host B with IP address 192.168.1.200 :
//
//         0E40  CTCE  30880  192.168.1.100  30880
//         0E41  CTCE  30882  192.168.1.100  30882

// -------------------------------------------------------------------
// Execute a Channel Command Word (CTCE)
// -------------------------------------------------------------------

void  CTCE_ExecuteCCW( DEVBLK* pDEVBLK, BYTE  bCode,
                       BYTE    bFlags,  BYTE  bChained,
                       U32     sCount,  BYTE  bPrevCode,
                       int     iCCWSeq, BYTE* pIOBuf,
                       BYTE*   pMore,   BYTE* pUnitStat,
                       U32*    pResidual )
{
    int             iNum;               // Number of bytes to move
    CTCE_PARMBLK    parm;               // Parameters for the server
                                        //   as in CTCE_Init, used here for the client side
    int             optval;             // Argument for setsockopt
    char*           remaddr;            // Remote IP address
    char            address[20]="";     // temp space for IP address
    int             rc;                 // Return code
    CTCE_INFO       CTCE_Info;          // CTCE information (also for tracing)

    UNREFERENCED( bChained  );
    UNREFERENCED( bPrevCode );
    UNREFERENCED( iCCWSeq   );
    UNREFERENCED( pMore     );

    // Initialise our CTCE_Info and save the previous x- and y-states in it.
    CTCE_Info.wait_rc            = 0;
    CTCE_Info.de_ready           = 0;
    CTCE_Info.de_ready_attn_rc   = 0;
    CTCE_Info.working_attn_rc    = 0;
    CTCE_Info.busy_waits         = 0;
    CTCE_Info.sent               = 0;
    CTCE_Info.con_lost           = 0;
    CTCE_Info.con_won            = 0;
    CTCE_Info.sel_reset          = 0;
    CTCE_Info.sok_buf_len        = 0;
    CTCE_Info.state_x_prev       = pDEVBLK->ctcexState;
    CTCE_Info.state_y_prev       = pDEVBLK->ctceyState;

    // Connect to the partner CTCE device if the device file is not open
    if (pDEVBLK->fd < 0)
    {

        // It's a little confusing, but we're using a couple of the
        // members of the server paramter structure to initiate the
        // outgoing connection.  Saves a couple of variable declarations,
        // though.  If we feel strongly about it, we can declare separate
        // variables...

        // make a TCP socket
        parm.listenfd[0] = socket(AF_INET, SOCK_STREAM, 0);
        if( parm.listenfd[0] < 0 )
        {
            WRMSG( HHC05050, "E",  /* CTCE: Error creating socket: %s */
                CTCX_DEVNUM( pDEVBLK ), strerror( HSO_errno ) );
            *pUnitStat = CSW_CE | CSW_DE | CSW_UC;
            return;
        }

        // Allow previous instance of socket to be reused
        optval = 1;
        setsockopt( parm.listenfd[0], SOL_SOCKET, SO_REUSEADDR,
            ( GETSET_SOCKOPT_T* )&optval, sizeof( optval ) );

#if defined(CTCE_DISABLE_NAGLE)
        optval = 1;
        rc = setsockopt( parm.listenfd[0], IPPROTO_TCP, TCP_NODELAY,
            ( char* )&optval, sizeof( optval ) );
        if( rc < 0 )
        {
            WRMSG( HHC05051, "E",  /* CTCE: TCP_NODELAY error for socket (port %d): %s */
                CTCX_DEVNUM( pDEVBLK ), pDEVBLK->ctce_lport, strerror(HSO_errno));
            *pUnitStat = CSW_CE | CSW_DE | CSW_UC;
            return;
        }
#endif

        // bind socket to our local port
        // (might seem like overkill, and usually isn't done, but doing this
        // bind() to the local port we configure gives the other end a chance
        // at validating the connection request)
        memset( &( parm.addr ), 0, sizeof( parm.addr ) );
        parm.addr.sin_family = AF_INET;
        parm.addr.sin_port = htons( pDEVBLK->ctce_lport );
        parm.addr.sin_addr.s_addr = htonl( INADDR_ANY );

        rc = bind( parm.listenfd[0],
            ( struct sockaddr * )&parm.addr,
            sizeof( parm.addr ) );
        if( rc < 0 )
        {
            WRMSG( HHC05052, "E",  /* CTCE: Error binding to socket (port %d): %s */
                CTCX_DEVNUM( pDEVBLK ), pDEVBLK->ctce_lport, strerror( HSO_errno ) );
            *pUnitStat = CSW_CE | CSW_DE | CSW_UC;
            return;
        }
        strcpy( address, inet_ntoa( pDEVBLK->ctce_ipaddr ) );
        remaddr = address;

        // initiate a connection to the other end
        memset( &( parm.addr ), 0, sizeof( parm.addr ) );
        parm.addr.sin_family = AF_INET;

        // the even (=read) port must connect to the odd (=write) port
        // at the other side and vice-versa
        parm.addr.sin_port = htons(pDEVBLK->ctce_rport + 1 );
        parm.addr.sin_addr = pDEVBLK->ctce_ipaddr;

        // We connect() but with a timeout value of 500000 usec = 0.5 sec
        rc = CTCE_Connect_Timeout( parm.listenfd[0],
            ( struct sockaddr * )&parm.addr,
            sizeof( parm.addr), 500000, pDEVBLK );

        // if connection was not successful, then we may retry later on when the other side becomes ready.
        if( rc < 0 )
        {

            // We issue the "retry is possible" message only once, which is when
            // the ctce_contention_loser variable is still in its initial =1 state.
            if( pDEVBLK->ctce_contention_loser == 1 )
            {
                WRMSG( HHC05053, "I",  /* CTCE: Connect error :%d -> %s:%d, retry is possible */
                    CTCX_DEVNUM( pDEVBLK ), pDEVBLK->ctce_lport, remaddr, pDEVBLK->ctce_rport + 1 );
            }
        }
        else  // successfully connected to the other end
        {
            WRMSG( HHC05054, "I",  /* CTCE: Started outbound connection :%d -> %s:%d */
                CTCX_DEVNUM( pDEVBLK ), pDEVBLK->ctce_lport, remaddr, pDEVBLK->ctce_rport + 1 );

            // The even local port (form the config) is for writing
            pDEVBLK->fd = parm.listenfd[0];
        }
    }

    // Intervention required if the device file is not open
    if( ( pDEVBLK->fd < 0 ) || ( pDEVBLK->ctcefd < 0 ) )
    {
        if( !IS_CCW_SENSE( bCode ) &&
            !IS_CCW_CONTROL( bCode ) )
        {
            pDEVBLK->sense[0] = SENSE_IR;
            *pUnitStat = CSW_CE | CSW_DE | CSW_UC;
            return;
        }
    }

    // Changes to DEVBLK are lock protected as the CTCE_RecvThread
    // might update as well.
    obtain_lock( &pDEVBLK->lock );

    // The CCW Flags Command Chaining indicator being set indicates
    // that a CCW Program is in progress.  The last CCW in the chain
    // has this flag turned off.
    pDEVBLK->ctce_ccw_flags_cc = ( ( bFlags & CCW_FLAGS_CC ) != 0 );

    // Copy control command byte in x command register
    pDEVBLK->ctcexCmd = bCode;

    // A valid Set Extended / Base Mode (SEM / SBM) command implies a
    // Reset took place immediately prior to it, for which currently
    // no explicit device handler exit capability exists (i.e. in
    // "channel.c").  Hence that we implement this here, as well
    // when a SEM / SBM instruction is received at the other (y-)side.
    // Because the 'extended mode' influences the FSM state, the SEM
    // command immediately needs to result in the 'available' state.
    if( IS_CTCE_CCW_SEM( pDEVBLK->ctcexCmd ) ||
        IS_CTCE_CCW_SBM( pDEVBLK->ctcexCmd ) )
    {
        CTCE_Info.sel_reset = ( ( IS_CTCE_YWK( pDEVBLK->ctcexState ) | IS_CTCE_YWK( pDEVBLK->ctceyState ) ) ? 1 : 0 ) ;
        CTCE_Reset( pDEVBLK );
        if( IS_CTCE_CCW_SEM( pDEVBLK->ctcexCmd ) )
        {
            pDEVBLK->ctcxmode = 1;
            SET_CTCE_YAV( pDEVBLK->ctcexState );
        }
        else
        {
            pDEVBLK->ctcxmode = 0;
        }
    }

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

        // Process depending on the CCW command.
        switch ( CTCE_CMD( pDEVBLK->ctcexCmd ) )
        {

        // Most of the CTCE commands processing (if any at all)
        // takes place in CTCE_Send and CTCE_RECV down below.
        case CTCE_PREPARE:
        case CTCE_CONTROL:
        case CTCE_READ:
        case CTCE_WRITE:
        case CTCE_READ_BACKWARD:
        case CTCE_WRITE_END_OF_FILE:
        case CTCE_NO_OPERATION:
        case CTCE_SET_EXTENDED_MODE:
        case CTCE_SET_BASIC_MODE:
            break;

        case CTCE_SENSE_COMMAND_BYTE:

            // In y-state available we return 0 otherwise the last y-side command.
            *pIOBuf = ( IS_CTCE_YAV( pDEVBLK->ctceyState ) ) ?
                0 : pDEVBLK->ctceyCmdSCB;
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

        // Invalid commands
        // (or never experienced / tested / supported ones)
        case CTCE_READ_CONFIG_DATA:
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

            // In case we sent a matching SCB command, this side becomes
            // the contention winner side; the receiver updates accordingly.
            if( IS_CTCE_CCW_SCB( pDEVBLK->ctcexCmd ) && CTCE_Info.sent )
            {
                pDEVBLK->ctce_contention_loser = 0;
            }
        }

        // This (x-)side will leave the Not Ready state.
        if( IS_CTCE_YNR( pDEVBLK->ctcexState ) )
        {
            SET_CTCE_YAV( pDEVBLK->ctcexState );
        }

    } // if( !( CTCE_Info.x_unit_stat & CSW_BUSY ) )

    // We merge a Unit Check in case the Y state is Not Ready.
    // But only when pUnitStat is still 0 or Unit Check or Busy (no Attn).
    // SENSE bit 1 for Intervention Required will be set as well.
    if( IS_CTCE_YNR( pDEVBLK -> ctceyState ) &&
        ( ( *pUnitStat & (~ ( CSW_BUSY | CSW_UC ) ) ) == 0 ) )
    {
        *pUnitStat |= CSW_UC;
        pDEVBLK->sense[0] = SENSE_IR;
    }

    // Produce a CTCE Trace logging if requested, noting that for the
    // IS_CTCE_WAIT cases such a logging is produced prior to the WAIT,
    // and CTCE_Recv will produce a logging for the matching command.
    if( ( pDEVBLK->ccwtrace || pDEVBLK->ccwstep ) &&
        ( !( IS_CTCE_WAIT( CTCE_Info.actions ) ) ||
          !CTCE_Info.sent ) )
    {
        CTCE_Trace( pDEVBLK, sCount, CTCE_Info.sent ? CTCE_SND : CTCE_LCL,
                    &CTCE_Info, pDEVBLK->buf, pUnitStat );
    }

    // The contention winning CTCE side initially is the first one to
    // attempt commands; each matching SCB command sent sets this as well.
    if( ( pDEVBLK->fd < 0 ) || ( pDEVBLK->ctcefd < 0 ) )
    {
        pDEVBLK->ctce_contention_loser = 0;
    }

    release_lock( &pDEVBLK->lock );
}

//
// CTCE_Init
//

static int  CTCE_Init( DEVBLK *dev, int argc, char *argv[] )
{
    char           str[80];            // Thread name
    int            mtu;                // MTU size (binary)
    char*          listenp;            // Listening port number
    char*          remotep;            // Destination port number
    char*          mtusize;            // MTU size (characters)
    char*          remaddr;            // Remote IP address
    BYTE           c;                  // Character work area
    TID            tid;                // Thread ID for server
    int            ctceSmlBin;         // Small size (binary)
    char*          ctceSmlChr;         // Small size (characters)
    CTCE_PARMBLK   parm;               // Parameters for the server
    CTCE_PARMBLK*  arg;                // used to pass parameters to the server thread
    char           address[20]="";     // temp space for IP address
    int            optval;             // Argument for setsockopt

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
    SetSIDInfo( dev, 0x3088, 0x08, 0x3088, 0x01 );

    dev->numsense = 2;

    // A version 4 only feature ...
    dev->excps = 0;

    // The halt_device exit is established; in version 4 this is in DEVHND in the ctce_device_hndinfo.
//  dev->halt_device = &CTCE_Reset;

    // Mark both socket file descriptors as not yet connected.
    dev->fd = -1;
    dev->ctcefd = -1;

    // Check for correct number of arguments
    if( (argc < 3) && (argc > 5) )
    {
        WRMSG( HHC05055, "E",  /* CTCE: Incorrect number of parameters */
            CTCX_DEVNUM( dev ) );
        return -1;
    }

    // The first argument is the listening port number
    // which for CTCE must be an even port number.
    listenp = *argv++;

    if( strlen( listenp ) > 5 ||
        sscanf( listenp, "%u%c", &dev->ctce_lport, &c ) != 1 ||
        dev->ctce_lport < 1024 || dev->ctce_lport > 65534 )
    {
        WRMSG( HHC05056, "E",  /* CTCE: Invalid port number: %s */
            CTCX_DEVNUM( dev ), listenp );
        return -1;
    }
    if( dev->ctce_lport % 2 )
    {
        WRMSG( HHC05057, "E",  /* CTCE: Local port number not even: %s */
            CTCX_DEVNUM( dev ), listenp );
        return -1;
    }

    // The second argument is the IP address or hostname of the
    // remote side of the point-to-point link
    remaddr = *argv++;

    if (!inet_aton( remaddr, &dev->ctce_ipaddr ))
    {
        struct hostent *hp;

        if( ( hp = gethostbyname( remaddr ) ) != NULL )
        {
            memcpy( &dev->ctce_ipaddr, hp->h_addr, hp->h_length );
            strcpy( address, inet_ntoa( dev->ctce_ipaddr ) );
            remaddr = address;
        }
        else
        {
            WRMSG( HHC05058, "E",  /* CTCE: Invalid IP address %s */
                CTCX_DEVNUM( dev ), remaddr );
            return -1;
        }
    }

    // The third argument is the destination port number
    // which for CTCE must be an even port number.
    remotep = *argv++;

    if( strlen( remotep ) > 5 ||
        sscanf( remotep, "%u%c", &dev->ctce_rport, &c ) != 1 ||
        dev->ctce_rport < 1024 || dev->ctce_rport > 65534 )
    {
        WRMSG( HHC05059, "E",  /* CTCE: Invalid port number: %s */
            CTCX_DEVNUM( dev ), remotep );
        return -1;
    }
    if( dev->ctce_rport % 2 )
    {
        WRMSG( HHC05060, "E",  /* CTCE: Remote port number not even: %s */
            CTCX_DEVNUM( dev ), remotep );
        return -1;
    }

    // Enhanced CTC default MTU bufsize is CTCE_MTU_MIN.
    if( argc < 4 )
    {
        mtu = CTCE_MTU_MIN;
    }
    else
    {

        // The fourth argument is the maximum transmission unit (MTU) size
        mtusize = *argv;

        if( strlen( mtusize ) > 5 ||
            sscanf( mtusize, "%u%c", &mtu, &c ) != 1 ||
            mtu < CTCE_MTU_MIN || mtu > 65536 )
        {
            WRMSG( HHC05061, "E",  /* CTCE: Invalid MTU size %s, allowed range is %d to 65536 */
                CTCX_DEVNUM( dev ), mtusize, CTCE_MTU_MIN );
            return -1;
        }
    }

    // Enhanced CTC only supports an optional 5th parameter,
    // the Small MTU size, which defaults to the minimum size
    // of the TCP/IP packets exchanged: CTCE_SOKPFX.
    ctceSmlBin = sizeof(CTCE_SOKPFX);
    if( argc == 5 )
    {
        ctceSmlChr = *(++argv);

        if( strlen( ctceSmlChr ) > 5 ||
            sscanf( ctceSmlChr, "%u%c", &ctceSmlBin, &c ) != 1 ||
            ctceSmlBin < (int)sizeof(CTCE_SOKPFX) || ctceSmlBin > mtu )
        {
            ctceSmlBin = sizeof(CTCE_SOKPFX);
            WRMSG( HHC05062, "W",  /* CTCE: Invalid Small MTU size %s ignored */
                CTCX_DEVNUM( dev ), ctceSmlChr );
        }
    }
    dev->ctceSndSml = ctceSmlBin;

    // Set the device buffer size equal to the MTU size
    dev->bufsize = mtu;

    WRMSG( HHC05063, "I",  /* CTCE: Awaiting inbound connection :%d <- %s:%d */
        CTCX_DEVNUM( dev ), dev->ctce_lport + 1, remaddr, dev->ctce_rport );

    // Initialize the file descriptor for the socket connection
    parm.listenfd[1] = socket(AF_INET, SOCK_STREAM, 0);
    if( parm.listenfd[1] < 0 )
    {
        WRMSG( HHC05064, "E",  /* CTCE: Error creating socket: %s */
            CTCX_DEVNUM( dev ), strerror( HSO_errno ) );
        CTCE_Close( dev );
        return -1;
    }

    // Allow previous instance of socket to be reused
    optval = 1;
    setsockopt(parm.listenfd[1], SOL_SOCKET, SO_REUSEADDR,
        (GETSET_SOCKOPT_T*)&optval, sizeof(optval));

    // set up the listening port
    memset( &(parm.addr), 0, sizeof( parm.addr ) );

    parm.addr.sin_family      = AF_INET;
    parm.addr.sin_port        = htons(dev->ctce_lport + 1) ;
    parm.addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if( bind( parm.listenfd[1],
              ( struct sockaddr * )&parm.addr,
              sizeof( parm.addr ) ) < 0 )
    {
        WRMSG( HHC05065, "E",  /* CTCE: Error binding to socket (port=%d): %s */
            CTCX_DEVNUM( dev ), dev->ctce_lport + 1, strerror( HSO_errno ) );
        CTCE_Close( dev );
        return -1;
    }

    if( listen( parm.listenfd[1], 1 ) < 0 )
    {
        WRMSG( HHC05066, "E",  /* CTCE: Error on call to listen (port=%d): %s */
            CTCX_DEVNUM( dev ), dev->ctce_lport + 1, strerror( HSO_errno ) );
        CTCE_Close( dev );
        return -1;
    }

    // we are listening, so create a thread to accept connection
    arg = malloc( sizeof( CTCE_PARMBLK ) );
    memcpy( arg, &parm, sizeof( parm ) );
    arg->dev = dev;
    snprintf( str, sizeof( str ), "CTCE %4.4X ListenThread", dev->devnum);
    str[sizeof( str )-1]=0;
    create_thread( &tid, JOINABLE, CTCE_ListenThread, arg, str );

    // for cosmetics, since we are successfully serving,
    // fill in some details for the panel.
    // Also used for connection verification in CTCE_ListenThread.
    // The other side's CTCE device number will be completed
    // upon the first receive, we initialize here with dots.
    sprintf( dev->filename, ".:....=%s:%d", remaddr, dev->ctce_rport );

    // Enhanced CTC adapter intiialization for command register and CB.
    dev->ctcexCmd = 0x00;
    dev->ctceyCmd = 0x00;
    dev->ctceyCmdSCB = 0x00;

    // Enhanced CTC adapter sides are state-aware, with initial
    // state (1) "Available" and (2) "Not Ready" as in column 6 in
    // the table 2.13 in SA22-7203-00, i.e. we consider both
    // sides being in state YNR. ALL Flags are cleared.
    CLR_CTCE_ALLF( dev->ctcexState );
    SET_CTCE_YNR ( dev->ctcexState );
    CLR_CTCE_ALLF( dev->ctceyState );
    SET_CTCE_YNR ( dev->ctceyState );

    // Until we are successfully contacted by the other side,
    // we mark the "other side Intervention Required".
    dev->sense[0] = SENSE_IR;

    // Initialize the 12 bits Send->Recv packet sequence ID with
    // bits 13-12 of the CCUU devnum in bits 15-14, and
    // bits 01-00 of the CCUU devnum in bits 13-12.  This helps
    // distinguishing same-host traffic if the Send-Recv side
    // CCUU's are sufficiently different (e.g. when under VM).
    dev->ctcePktSeq = ((dev->devnum <<  2) & 0xC000) |
                      ((dev->devnum << 12) & 0x3000) ;

    // Initialize the CTC lock and condition used to signal
    // reception of a command matching the dependent one.
    initialize_lock( &dev->ctceEventLock );
    initialize_condition( &dev->ctceEvent );

    // The ctce_contention_loser side of a CTCE connection will act as
    // if a colliding dependent command arrived following the one at
    // the other side.  The CTC side connecting 1st will reset this,
    // and matching SCB commands may alter it also.
    dev->ctce_contention_loser = 1;

    return 0;
}

//
// CTCE_ListenThread
//

static void*  CTCE_ListenThread( void* argp )
{
    int          connfd;
    socklen_t    servlen;
    char         str[80];
    CTCE_PARMBLK parm;
    TID          tid2;               // Thread ID for read thread
    int          rc;                 // Return code
#if defined( CTCE_DISABLE_NAGLE )
    int          optval;             // Argument for setsockopt
#endif

    // set up the parameters passed via create_thread
    parm = *((CTCE_PARMBLK*) argp);
    free( argp );

    for( ; ; )
    {
        servlen = sizeof( parm.addr );

        // await a connection
        connfd = accept( parm.listenfd[1],
                         (struct sockaddr *)&parm.addr,
                         &servlen );

        sprintf( str, "%s:%d",
                 inet_ntoa( parm.addr.sin_addr ),
                 ntohs( parm.addr.sin_port ) );

        if( strcmp( str, parm.dev->filename + 7 ) != 0 )
        {
            WRMSG( HHC05067, "E",  /* CTCE: Inconsistent config=%s+%d, connecting client=%s */
                CTCX_DEVNUM( parm.dev ), parm.dev->filename + 7, 1, str);
            close_socket( connfd );
        }
        else
        {

#if defined( CTCE_DISABLE_NAGLE )
            optval = 1;
            rc = setsockopt(parm.listenfd[1], IPPROTO_TCP, TCP_NODELAY,
                (char*)&optval, sizeof( optval ) );
            if( rc < 0 )
            {
                WRMSG( HHC05068, "E",  /* CTCE: TCP_NODELAY error for socket (port %d): %s */
                    CTCX_DEVNUM( parm.dev ), parm.dev->ctce_lport + 1, strerror( HSO_errno ) );
                close_socket( connfd );
            }
#endif

            // The next odd local port is for reading
            parm.dev->ctcefd = connfd;

            // This side is ready to start receiving and sending so we
            // start a read thread to do the receiving part;
            snprintf( str, sizeof(str), "CTCE %04X RecvThread",
                parm.dev->devnum );
            str[sizeof(str)-1]=0;
            rc = create_thread( &tid2, JOINABLE, CTCE_RecvThread, parm.dev, str );
            if( rc != 0 )
            {
                WRMSG( HHC05069, "E",  /* CTCE: create_thread error: %s */
                    CTCX_DEVNUM( parm.dev ), strerror(errno));
            }
            else
            {
                WRMSG( HHC05070, "E",  /* CTCE: Accepted inbound connection :%d <- %s (bufsize=%d,%d) */
                    CTCX_DEVNUM( parm.dev ), parm.dev->ctce_lport + 1,
                    parm.dev->filename + 7, parm.dev->bufsize, parm.dev->ctceSndSml );
            }
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

//
// CTCE_Send
//

static void   CTCE_Send( DEVBLK* pDEVBLK,   U32        sCount,
                         BYTE*   pIOBuf,    BYTE*      pUnitStat,
                         U32*    pResidual, CTCE_INFO* pCTCE_Info )
{
    CTCE_SOKPFX   *pSokBuf;                 // overlay for buf inside DEVBLK
    int            rc;                      // Return code

    if( ! IS_CTCE_SEND( pCTCE_Info->actions ) )
    {
        WRMSG( HHC05071, "S",  /* CTCE: SEND status incorrectly encoded ! */
            CTCX_DEVNUM( pDEVBLK ) );
    }

    // We only ever Send if the sockets are connected.
    if( ( pDEVBLK->fd < 0 ) || ( pDEVBLK->ctcefd < 0 ) )
    {

        // We issue the "Not all sockets connected" message only when just
        // one socket is not connected; otherwise just once which is when
        // the ctce_contention_loser variable is still in its initial =1 state.
        if (0
            || pDEVBLK->ctce_contention_loser == 1
            || pDEVBLK->fd                    >= 0
            || pDEVBLK->ctcefd                >= 0
        )
        {
            WRMSG( HHC05072, "E",  /* CTCE: Not all sockets connected: send=%d, receive=%d */
                CTCX_DEVNUM( pDEVBLK ), pDEVBLK->fd, pDEVBLK->ctcefd );
        }
        if( !IS_CTCE_CCW_SCB( pDEVBLK->ctcexCmd ) )
        {
            *pUnitStat = 0;
        }
        return ;
    }
    pCTCE_Info->sent = 1;

    pDEVBLK->ctcexState = CTCE_NEW_X_STATE( pDEVBLK->ctcexCmd );
    pDEVBLK->ctce_UnitStat = *pUnitStat;

    pSokBuf = (CTCE_SOKPFX*) pDEVBLK->buf;
    pSokBuf->CmdReg = pDEVBLK->ctcexCmd;
    pSokBuf->FsmSta = pDEVBLK->ctcexState;
    pSokBuf->sCount = sCount;
    pSokBuf->PktSeq = ++pDEVBLK->ctcePktSeq;
    pSokBuf->SndLen = pDEVBLK->ctceSndSml;
    pSokBuf->DevNum = pDEVBLK->devnum;
    pSokBuf->ssid   = pDEVBLK->ssid;

    // Only a (non-WEOF) write command data includes sending the IOBuf.
    if( IS_CTCE_CCW_WRT( pDEVBLK->ctcexCmd ) )
    {
        memcpy( pDEVBLK->buf + sizeof(CTCE_SOKPFX), pIOBuf, sCount );

        // Increase the SndLen if the sCount is too large.
        if( pSokBuf->SndLen < ( sCount + sizeof(CTCE_SOKPFX) ) )
            pSokBuf->SndLen = ( sCount + sizeof(CTCE_SOKPFX) );

        // Write Data.
        if( pDEVBLK->ccwstep )
            packet_trace( pIOBuf, sCount, '>' );

        // If bufsize (init from the MTU parameter) is not large enough
        // then we will have a severe error as the CTC will not connect.
        if( pDEVBLK->bufsize < pSokBuf->SndLen )
        {
            WRMSG( HHC05073, "S",  /* CTCE: bufsize parameter %d is too small; increase at least to %d */
                CTCX_DEVNUM( pDEVBLK ), pDEVBLK->bufsize, pSokBuf->SndLen );
        }
    }
    pCTCE_Info->sok_buf_len = pSokBuf->SndLen;

    // Write all of this to the other (y-)side.
    rc = write_socket( pDEVBLK->fd, pDEVBLK->buf, pSokBuf->SndLen );

    // The sending of IOBuf for WRT commands has been completed so we need to
    // mark the used device buffer part as free by zeroing the data length field
    // as CTCE_RecvThread will wait for this when it receives a WRT command.
    if( IS_CTCE_CCW_WRT( pDEVBLK->ctcexCmd ) )
    {
        *(U16*)( pDEVBLK->buf + sizeof(CTCE_SOKPFX) ) = 0;
    }

    if( rc < 0 )
    {
        WRMSG( HHC05074, "E",  /* CTCE: Error writing to %s: %s */
            CTCX_DEVNUM( pDEVBLK ), CTCE_FILENAME, strerror( HSO_errno ) );

        pDEVBLK->sense[0] = SENSE_EC;
        *pUnitStat        = CSW_CE | CSW_DE | CSW_UC;

        // For lack of anything better, we return to the not ready state.
        CLR_CTCE_ALLF(pDEVBLK->ctcexState);
        SET_CTCE_YNR(pDEVBLK->ctcexState);
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
        if( pDEVBLK->ccwtrace || pDEVBLK->ccwstep )
        {
            CTCE_Trace( pDEVBLK, sCount, CTCE_SND, pCTCE_Info, pDEVBLK->buf, pUnitStat );
        }

        obtain_lock( &pDEVBLK->ctceEventLock );
        release_lock( &pDEVBLK->lock );

        pCTCE_Info->wait_rc = timed_wait_condition_relative_usecs(
            &pDEVBLK->ctceEvent,
            &pDEVBLK->ctceEventLock,
            1000000000,
            NULL );

        obtain_lock( &pDEVBLK->lock );
        release_lock( &pDEVBLK->ctceEventLock );

        // First we check for Halt or Clear Subchannel
        if( pCTCE_Info->wait_rc == ETIMEDOUT || pCTCE_Info->wait_rc == EINTR )
        {
            // check for halt condition
            if( pDEVBLK->scsw.flag2 & SCSW2_FC_HALT ||
                pDEVBLK->scsw.flag2 & SCSW2_FC_CLEAR )
            {
                if( pDEVBLK->ccwtrace || pDEVBLK->ccwstep )
                {
                    WRMSG( HHC05075, "I",  /* CTCE: Halt or Clear Recognized */
                        CTCX_DEVNUM( pDEVBLK ) );
                }

//???           *pUnitStat = CSW_CE | CSW_DE;
                *pUnitStat = 0;
                *pResidual = sCount;
            }

            // Other timeouts or errors should not occur.
            // But if they do, we try to recover as if the other side
            // was in a working(D) state.
            else
            {
                *pUnitStat = CSW_BUSY | CSW_ATTN ;
                SET_CTCE_YAV( pDEVBLK->ctcexState );
            }

            // Produce a trace logging if requested.
            if( pDEVBLK->ccwtrace || pDEVBLK->ccwstep )
            {
                CTCE_Trace( pDEVBLK, sCount, CTCE_SND, pCTCE_Info, pDEVBLK->buf, pUnitStat );
            }
            return;
        }

        // Trace the non-zero WAIT RC (e.g. timeout, RC=138 (windows) or 110 (unix)).
        else if( pCTCE_Info->wait_rc != 0 )
        {
            CTCE_Trace( pDEVBLK, sCount, CTCE_SND, pCTCE_Info, pDEVBLK->buf, pUnitStat );
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
            if( pDEVBLK->ccwtrace || pDEVBLK->ccwstep )
            {
                CTCE_Trace( pDEVBLK, sCount, CTCE_SND, pCTCE_Info, pDEVBLK->buf, pUnitStat );
            }
            return;
        }
    } // if( IS_CTCE_WAIT( pCTCE_Info->actions ) )

    // In the non-WAIT case the final UnitStat is always
    // CE + DE provided the y-state is not Not_Ready.
    else if( !IS_CTCE_YNR( pDEVBLK -> ctceyState ) )
    {
        pDEVBLK->ctce_UnitStat = CSW_CE | CSW_DE;
    }

    // If the command (by now matched) was a CONTROL command, then this
    // side becomes the contention loser.
    if( IS_CTCE_CCW_CTL( pDEVBLK->ctcexCmd ) )
    {
        pDEVBLK->ctce_contention_loser = 1;
    }

    // Command collisions or resets (CSW_UC) never return data.
    if( ( pDEVBLK->ctce_UnitStat == (CSW_BUSY | CSW_ATTN) ) ||
        ( pDEVBLK->ctce_UnitStat & CSW_UC ) )
    {
        *pResidual = sCount;
    }

    // If the command (by now matched) was a READ command, then the
    // other (y-)side data is available in the DEVBLK buf, so we
    // can copy it into the IO channel buffer and compute residual.
    else if( IS_CTCE_CCW_RED( pDEVBLK->ctcexCmd ) )
    {

        // The actual length of data transferred is the minimum of
        // the current READ sCount, and the original WRITE sCount
        // which is recorded immediately following the CTCE_SOKPFX.
        pSokBuf->sCount =
            ( sCount <= *(U16*)( pDEVBLK->buf + sizeof(CTCE_SOKPFX) ) )
            ? sCount :  *(U16*)( pDEVBLK->buf + sizeof(CTCE_SOKPFX) );

        // Immediately followed by the WRITE data previously received.
        memcpy( pIOBuf, pDEVBLK->buf + sizeof(CTCE_SOKPFX) + sizeof(pSokBuf->sCount),
            pSokBuf->sCount ) ;
        *pResidual = sCount - pSokBuf->sCount;

        // We show the device buffer as being processed by zeroing the data length field.
        *(U16*)( pDEVBLK->buf + sizeof(CTCE_SOKPFX) ) = 0;
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
}

//
// CTCE_RecvThread
//

static void*  CTCE_RecvThread( void* argp )
{
    DEVBLK        *pDEVBLK = (DEVBLK*) argp;     // device block pointer
    CTCE_SOKPFX   *pSokBuf;                      // overlay for buf inside DEVBLK
    CTCE_INFO      CTCE_Info;                    // CTCE information (also for tracing)
    int            iLength  = 0;
    BYTE          *buf;                          //-> Device recv data buffer
    U64            ctcePktCnt = 0;               // Recvd Packet Count
    U64            ctceBytCnt = 0;               // Recvd Byte Count
    BYTE           ctce_recv_mods_UnitStat;      // UnitStat modifications
    int            i = 0;                        // temporary variable

    // When the receiver thread is (re-)started, the CTCE devblk is (re-)initialized
    obtain_lock( &pDEVBLK->lock );

    // Enhanced CTC adapter intiialization for command register and CB
    pDEVBLK->ctceyCmd = 0x00;
    pDEVBLK->ctceyCmdSCB = 0x00;

    // CTCE DEVBLK (re-)initialisation completed.
    release_lock( &pDEVBLK->lock );

    // Avoid having to lock the DEVBLK whilst awaiting data to arrive via read_socket
    buf = malloc( pDEVBLK->bufsize );
    pSokBuf = (CTCE_SOKPFX*)buf;

    // Initialise our CTCE_Info as needed.
    CTCE_Info.de_ready_attn_rc   = 0;
    CTCE_Info.working_attn_rc    = 0;
    CTCE_Info.busy_waits         = 0;

    // This thread will loop until we receive a zero-length packet caused by CTCX_close from the other side.
    for( ; ; )
    {
        // We read whatever the other (y-)side of the CTC has sent us,
        // which by now won't block until the complete bufsize is received.
        iLength = read_socket( pDEVBLK->ctcefd, buf, pDEVBLK->ctceSndSml );

        // Followed by the receiving the rest if the default SndLen was too small.
        if( ( pDEVBLK->ctceSndSml < pSokBuf->SndLen ) && ( iLength != 0 ) )
            iLength += read_socket( pDEVBLK->ctcefd, buf + pDEVBLK->ctceSndSml,
                pSokBuf->SndLen - pDEVBLK->ctceSndSml );

        // In case we are closing down this thread can end.
        if( iLength == 0 )
        {

            // We report some statistics.
            WRMSG( HHC05076, "I",  /* CTCE: Connection closed; %"PRIu64" MB received in %"PRIu64" packets from %s */
                CTCX_DEVNUM( pDEVBLK ), ctceBytCnt >> SHIFT_MEGABYTE , ctcePktCnt, CTCE_FILENAME );

            // And this receiver socket can now be closed.
            close_socket( pDEVBLK->ctcefd );
            pDEVBLK->ctcefd = -1;

            free( buf );
            return NULL;    // make compiler happy
        }

        // Commands sent by the other (y-)side most likely cause DEVBLK
        // changes to our (x-)side and thus need to be lock protected.
        // The write command data in the socket buffer needs to be copied
        // into the device buffer, but we can only do this if that buffer
        // is empty; we may have to wait for that to be the case.
        obtain_lock(&pDEVBLK->lock);
        if( IS_CTCE_CCW_WRT( pSokBuf->CmdReg ) )
        {
            for( CTCE_Info.busy_waits = 0;
                 ( *(U16*)( pDEVBLK->buf + sizeof(CTCE_SOKPFX) ) != 0 ) &&
                 ( CTCE_Info.busy_waits <= 20 ) ;
                 CTCE_Info.busy_waits++ )
            {
                release_lock( &pDEVBLK->lock );
                usleep(  200 );
                obtain_lock( &pDEVBLK->lock );
            }
        }

        // Check for other error condition
        if( iLength < 0 )
        {
            WRMSG( HHC05077, "E",  /* CTCE: Error reading from %s: %s */
                CTCX_DEVNUM( pDEVBLK ), CTCE_FILENAME, strerror ( HSO_errno ) );
            pDEVBLK->sense[0] = SENSE_EC;
            pDEVBLK->scsw.unitstat = CSW_CE | CSW_DE | CSW_UC;
        }
        else
        {

            // Upon the first recv we will fill out the devnum in the filename.
            if( pDEVBLK->filename[2] == '.' )
            {
                snprintf( pDEVBLK->filename, 7, "%1d:%04X",
                    SSID_TO_LCSS( pSokBuf->ssid ), pSokBuf->DevNum );
                pDEVBLK->filename[6] = '=';
            }

            // Update the Receive statistics counters.
            ctcePktCnt += 1 ;
            ctceBytCnt += iLength ;

            // Initialise the UnitStat modifications.
            ctce_recv_mods_UnitStat = 0;

            // Save the previous CTCE states,
            // our (x-)side as well as the other (y-)side.
            CTCE_Info.state_x_prev = pDEVBLK->ctcexState;
            CTCE_Info.state_y_prev = pDEVBLK->ctceyState;

            // SEM / SBM commands imply the other (y-)side performed a
            // Reset (Selective or otherwise), which the (x-)side needs
            // to act on as well.  SEM also requires 'extended' mode
            // to be set on this side as well.
            if( IS_CTCE_CCW_SEM( pSokBuf->CmdReg ) ||
                IS_CTCE_CCW_SBM( pSokBuf->CmdReg ) )
            {
                CTCE_Info.sel_reset = ( ( IS_CTCE_YWK( pDEVBLK->ctcexState ) | IS_CTCE_YWK( pDEVBLK->ctceyState ) ) ? 1 : 0 ) ;
                CTCE_Reset( pDEVBLK );
                if( IS_CTCE_CCW_SEM( pSokBuf->CmdReg ) )
                {
                    pDEVBLK->ctcxmode = 1;
                }
            }

            // The command received from the other (y-)side may cause a
            // state transition on our (x-)side, as well as some actions.
            // Both depend on our current (x-)side state and are encoded
            // within the FSM table.
            CTCE_Info.state_new = CTCE_NEW_Y_STATE( pSokBuf->CmdReg );
            CTCE_Info.actions     = CTCE_Fsm[CTCE_CMD( pSokBuf->CmdReg )]
                                            [CTCE_Y_STATE_FSM_IDX].actions;
            CTCE_Info.x_unit_stat = CTCE_Fsm[CTCE_CMD( pSokBuf->CmdReg )]
                                            [CTCE_Y_STATE_FSM_IDX].x_unit_stat;
            CTCE_Info.con_lost = 0;
            CTCE_Info.con_won = 0;

            // Command collision occurs when both sides receive a
            // (non-matching) DEPendent command at the same time,
            // crossing each other in xfer to the other side (e.g. two
            // READ or WRITE commands).  Both sides would respond with
            // a Busy+Attention device status.
            if( ( CTCE_Info.x_unit_stat == ( CSW_BUSY | CSW_ATTN ) )
                && IS_CTCE_CCW_DEP(  pSokBuf->CmdReg ) )
            {

                // In a real CTC this never occurs, there is always a
                // first and a second side.  CTCE emulates the second
                // side behaviour where ctce_contention_loser==1.
                if( pDEVBLK->ctce_contention_loser )
                {

                    // This is done by signaling this by now awaiting
                    // side as if a matching command was received, but
                    // only after re-instating the original FSM state
                    // and ensuring that the required Busy+Attention
                    // device status will be returned.  Effectively,
                    // this is a contention lost situation.
                    CTCE_Info.con_lost = 1;
                    pDEVBLK->ctcexState = CTCE_Info.state_new;
                    pDEVBLK->ctce_UnitStat = CSW_BUSY | CSW_ATTN;
                    obtain_lock( &pDEVBLK->ctceEventLock );
                    signal_condition( &pDEVBLK->ctceEvent );
                    release_lock( &pDEVBLK->ctceEventLock );

                    // After our (x-)state is reset, we need to
                    // re-compute the FSM state transition effects.
                    CTCE_Info.state_new = CTCE_NEW_Y_STATE( pSokBuf->CmdReg );
                    CTCE_Info.actions = CTCE_Fsm[CTCE_CMD( pSokBuf->CmdReg )]
                                                [CTCE_Y_STATE_FSM_IDX].actions;
                }

                // At the contention winning side, we can simply ignore
                // the CTCE_Recv, as the losing side will effectively
                // behave is if it never happened.
                else
                {
                    CTCE_Info.con_won = 1;
                }
            }
            if( CTCE_Info.con_won != 1 )
            {

                // Device-End status indicating ready will be presented
                // if the y-side has just now become ready.
                CTCE_Info.de_ready = ( IS_CTCE_YNR( pDEVBLK->ctceyState ) &&
                                      !IS_CTCE_YNR( pSokBuf->FsmSta ) ) ? 1 : 0;

                // Our (x-)side knowledge from the other (y-)side is updated.
                pDEVBLK->ctceyState = pSokBuf->FsmSta;
                pDEVBLK->ctceyCmd =  pSokBuf->CmdReg;
                pDEVBLK->ctceyCmdSCB = pSokBuf->CmdReg;

                // Only if the other (y-)side sent us a write command will
                // we copy the socket buffer into the device buffer.
                if( IS_CTCE_CCW_WRT( pDEVBLK->ctceyCmd ) )
                {

                    // We retain the sCount of this WRITE command for later
                    // comparison against the matching READ command, ahead
                    // of the data itself following CTCE_SOKPFX.
                    *(U16*)( pDEVBLK->buf + sizeof(CTCE_SOKPFX) ) = pSokBuf->sCount ;

                    memcpy( pDEVBLK->buf + sizeof(CTCE_SOKPFX) + sizeof(pSokBuf->sCount) ,
                        buf + sizeof(CTCE_SOKPFX), pSokBuf->sCount );
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
                    ctce_recv_mods_UnitStat = CSW_CE | CSW_DE;
                    pDEVBLK->ctce_UnitStat = CSW_CE | CSW_DE;

                    // We now signal CTCE_Send that the MATCHing command has been received.
                    obtain_lock( &pDEVBLK->ctceEventLock );
                    signal_condition( &pDEVBLK->ctceEvent );
                    release_lock( &pDEVBLK->ctceEventLock );
                } // if( IS_CTCE_MATCH( CTCE_Info.actions ) )

                // If the other (y-)side sent us a Device-End status
                // indicating Ready then this has to be presented on this side.
                else if( CTCE_Info.de_ready )
                {
                    release_lock( &pDEVBLK->lock );
                    ctce_recv_mods_UnitStat = CSW_DE;
                    CTCE_Info.de_ready_attn_rc = device_attention( pDEVBLK, CSW_DE );
                    obtain_lock( &pDEVBLK->lock );

                    // Reset sense byte 0 bits 1 and 7.
                    pDEVBLK->sense[0] &= ~( SENSE_IR | SENSE_OC );

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
                    if( pDEVBLK->ccwtrace || pDEVBLK->ccwstep )
                    {

                        // Disable ATTN RC reporting this time.
                        CTCE_Info.working_attn_rc = -1;

                        // In a contention winner situation, the command
                        // received from the other (y-)side still needs
                        // to be reported correctly.
                        pDEVBLK->ctceyCmd =  pSokBuf->CmdReg;
                        CTCE_Info.sok_buf_len = iLength;
                        CTCE_Trace( pDEVBLK, pSokBuf->sCount, CTCE_RCV, &CTCE_Info, buf, &ctce_recv_mods_UnitStat );
                    }

                    // The device_attention might not work on the first
                    // attempt due to the fact that we need to release
                    // the device lock around it, merely because that
                    // routine obtains and releases the device lock.
                    // During that short period, one or more commands
                    // may have come in between, causing a device busy
                    // and a possible other (y-)side status update. So
                    // we may need to re-try the ATTN if needed at all.
                    release_lock( &pDEVBLK->lock );
                    CTCE_Info.working_attn_rc = 1;
                    for( CTCE_Info.busy_waits = 0;
                         ( CTCE_Info.working_attn_rc == 1 ) &&
                         ( CTCE_Info.attn_can == 0 ) &&
                         ( CTCE_Info.busy_waits <= 20 ) ;
                         CTCE_Info.busy_waits++ )
                    {
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
                            usleep(i);

                            // Cancel the ATTN in case a CCW program
                            // has started in the mean time.
                            if ( pDEVBLK->ctce_ccw_flags_cc )
                            {
                                CTCE_Info.attn_can = 1;
                            }
                        }
                    }
                    obtain_lock( &pDEVBLK->lock );

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
            if( pDEVBLK->ccwtrace || pDEVBLK->ccwstep
                || ( ctce_recv_mods_UnitStat == ( CSW_BUSY | CSW_ATTN ) )
                || ( CTCE_Info.de_ready_attn_rc != 0 )
                || ( ( CTCE_Info.working_attn_rc  != 0 ) && ( CTCE_Info.attn_can == 0 ) )
                || ( CTCE_Info.busy_waits       >= 3 ) )
            {

                // In a contention winner situation, the command
                // received from the other (y-)side still needs
                // to be reported correctly.
                pDEVBLK->ctceyCmd =  pSokBuf->CmdReg;

                if( ctce_recv_mods_UnitStat != 0 )
                {
                    ctce_recv_mods_UnitStat = pDEVBLK->ctce_UnitStat;
                }
                CTCE_Info.sok_buf_len = iLength;
                CTCE_Trace( pDEVBLK, pSokBuf->sCount, CTCE_RCV, &CTCE_Info, buf, &ctce_recv_mods_UnitStat );
            }
            CTCE_Info.de_ready_attn_rc = 0;
            CTCE_Info.working_attn_rc  = 0;
            CTCE_Info.busy_waits       = 0;
        }

        release_lock( &pDEVBLK->lock );
    }
}

//
// CTCE_Reset -- Selective Reset or System Reset a CTCE adapter device
//

static void CTCE_Reset( DEVBLK* pDEVBLK )
{
    // The call already did an obtain_lock( &pDEVBLK->lock )
    // which is either in channel.c or earlier in here.
    // Reset the y-command to 0 and set sense bit 1, and
    // clear any WEOF state.
    pDEVBLK->ctceyCmdSCB = 0x00;
    pDEVBLK->sense[0] |= SENSE_IR;
    CLR_CTCE_WEOF( pDEVBLK->ctcexState );

    // Initialize the device buffer by zeroing the data length field.
    *(U16*)( pDEVBLK->buf + sizeof(CTCE_SOKPFX) ) = 0;

    // A selective reset requires cancelling any Working(D) wait state
    // at our (x-)side, and to return to the not ready state.  Also,
    // sense bit 7 needs to be set, and unit status set to CE+DE+UC.
    if( IS_CTCE_YWK( pDEVBLK->ctcexState ) )
    {
        obtain_lock( &pDEVBLK->ctceEventLock );
        {
            signal_condition( &pDEVBLK->ctceEvent );
        }
        release_lock( &pDEVBLK->ctceEventLock );

        CLR_CTCE_ALLF(pDEVBLK->ctcexState);
        SET_CTCE_YNR(pDEVBLK->ctcexState);
        pDEVBLK->sense[0] |= SENSE_OC;
        pDEVBLK->ctce_UnitStat = CSW_CE | CSW_DE | CSW_UC;
    }
    if( IS_CTCE_YWK( pDEVBLK->ctceyState ) )
    {
        SET_CTCE_YAV( pDEVBLK->ctceyState );
    }

    // Produce a CTCE Trace logging if requested.
    if( pDEVBLK->ccwtrace || pDEVBLK->ccwstep )
    {
        WRMSG( HHC05078, "I",  /* CTCE: -| %s%s%s%s x=%s y=%s cmd=%s*/
            CTCX_DEVNUM( pDEVBLK ), CTCE_RESET_TYPE,
            CTCE_StaStr[CTCE_STATE( pDEVBLK->ctcexState )],
            CTCE_StaStr[CTCE_STATE( pDEVBLK->ctceyState )],
            CTCE_CmdStr[CTCE_CMD( pDEVBLK->ctcexCmd )] );
    }
}

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
}

// ---------------------------------------------------------------------
// CTCE_Trace
// ---------------------------------------------------------------------
//
// Subroutine to produce a CTCE trace logging when requested.
//

void            CTCE_Trace( const DEVBLK*             pDEVBLK,
                            const U32                 sCount,
                            const enum CTCE_Cmd_Xfr   eCTCE_Cmd_Xfr,
                            const CTCE_INFO*          pCTCE_Info,
                            const BYTE*               pCTCE_Buf,
                            const BYTE*               pUnitStat )
{
    static char *CTCE_XfrStr[3] = {
        "-|" ,  //  0 = CTCE_LCL
        "->" ,  //  1 = CTCE_SND
        "<-"    //  2 = CTCE_RCV
    };
    BYTE           ctce_Cmd;                   // CTCE command being traced
    BYTE           ctce_PktSeq;                // Packet Sequence number traced
    CTCE_SOKPFX   *pSokBuf;                    // overlay for buf inside DEVBLK
    BYTE           ctce_state_verify;          // CTCE state to be verfified
    char           ctce_state_l_xy[2];         // CTCE X+Y states, left
    char           ctce_state_r_xy[2];         // CTCE X+Y stares, right
    char           ctce_trace_stat[16];        // to contain " Stat=.. CC=."
    char           ctce_trace_xtra[256];       // to contain extra info when tracing
    char           ctce_trace_xtra_temp[256];  // temporary work area for the above

    pSokBuf = (CTCE_SOKPFX*)pCTCE_Buf;

    // Report on the device status and CCW Command Chaining flag.
    if( ( eCTCE_Cmd_Xfr != CTCE_RCV ) || ( *pUnitStat != 0 ) ||
        ( IS_CTCE_MATCH( pCTCE_Info->actions ) ) )
    {
        snprintf( ctce_trace_stat, sizeof( ctce_trace_stat ),
            "Stat=%02X CC=%d", *pUnitStat, pDEVBLK->ctce_ccw_flags_cc );
    }
    else
    {
        snprintf( ctce_trace_stat, sizeof( ctce_trace_stat ),
            "        CC=%d", pDEVBLK->ctce_ccw_flags_cc );
    }

    ctce_trace_xtra[0] = '\0' ;

    // An SEM / SBM command implies a Reset happened immediately prior
    // to it, which we report on.
    if( eCTCE_Cmd_Xfr == CTCE_RCV )
    {
        ctce_Cmd = pDEVBLK->ctceyCmd;
    }
    else
    {
        ctce_Cmd = pDEVBLK->ctcexCmd;
    }
    if( IS_CTCE_CCW_SEM( ctce_Cmd ) ||
        IS_CTCE_CCW_SBM( ctce_Cmd ) )
    {
        if( pDEVBLK->scsw.flag2 & ( SCSW2_FC_HALT ) )
        {
            strlcat( ctce_trace_xtra, " Halt Reset", sizeof( ctce_trace_xtra ) );
        }
        else if ( pDEVBLK->scsw.flag2 & ( SCSW2_FC_CLEAR ) )
        {
            strlcat( ctce_trace_xtra, " Clear Reset", sizeof( ctce_trace_xtra ) );
        }
        else if ( ! pCTCE_Info->sel_reset )
        {
            strlcat( ctce_trace_xtra, " System Reset", sizeof( ctce_trace_xtra ) );
        }
        else if ( pCTCE_Info->sel_reset )
        {
            strlcat( ctce_trace_xtra, " Selective Reset", sizeof( ctce_trace_xtra ) );
        }
        else
        {
            strlcat( ctce_trace_xtra, "??? ", sizeof( ctce_trace_xtra ) );
        }
    }

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
            snprintf( ctce_trace_xtra_temp, sizeof( ctce_trace_xtra_temp ),
                "->RC=%d", pCTCE_Info->working_attn_rc );
            STRLCAT( ctce_trace_xtra, ctce_trace_xtra_temp );
        }
    }

    // The other side's "DE Ready" signalling to be shown.
    if( pCTCE_Info->de_ready )
    {
        snprintf( ctce_trace_xtra_temp, sizeof( ctce_trace_xtra_temp ),
            " DE_READY->RC=%d",pCTCE_Info->de_ready_attn_rc );
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

    // The source for reporting dependings on the Command X-fer
    // direction.  The CTCE states are reported in lower case,
    // but a changed state is highlighted in upper case.
    if( eCTCE_Cmd_Xfr == CTCE_RCV )
    {
        ctce_Cmd    = pDEVBLK->ctceyCmd;
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

        // Report on the SCB returned if applicable.
        if( IS_CTCE_CCW_SCB( ctce_Cmd ) )
        {
            snprintf( ctce_trace_xtra_temp, sizeof( ctce_trace_xtra_temp ),
                " SCB=%02X=%s", pCTCE_Info->scb, CTCE_CmdStr[CTCE_CMD( pCTCE_Info->scb )] );
            STRLCAT( ctce_trace_xtra, ctce_trace_xtra_temp );
        }
    }

    // Report on the device status.
    if( pCTCE_Info->busy_waits != 0 )
    {
        snprintf( ctce_trace_xtra_temp, sizeof( ctce_trace_xtra_temp ),
            " Busy_Waits=%d", pCTCE_Info->busy_waits );
        STRLCAT( ctce_trace_xtra, ctce_trace_xtra_temp );
    }

    // Report on the WAIT RC if needed.
    if( ( eCTCE_Cmd_Xfr == CTCE_SND ) && ( pCTCE_Info->wait_rc != 0 ) )
    {
        snprintf( ctce_trace_xtra_temp, sizeof( ctce_trace_xtra_temp ),
            " WAIT->RC=%d", pCTCE_Info->wait_rc );
        STRLCAT( ctce_trace_xtra, ctce_trace_xtra_temp );
    }

    // The "state mismatch" was used for debugging purposes
    // which would show logic errors.
    if( ( pCTCE_Info->state_new != ctce_state_verify )
        && !( ( !pCTCE_Info->sent ) && ( IS_CTCE_SEND( pCTCE_Info->actions ) ) ) )
    {
        snprintf( ctce_trace_xtra_temp, sizeof( ctce_trace_xtra_temp ),
            " CTCE_STATE MISMATCH %s!=%s(:FSM) !",
            CTCE_StaStr[ctce_state_verify],
            CTCE_StaStr[pCTCE_Info->state_new] );
        STRLCAT( ctce_trace_xtra, ctce_trace_xtra_temp );
    }

    // The unit "Stat mismatch" was used for debugging purposes
    // which would show logic errors.
    if( ( *pUnitStat !=
        ( ( ( eCTCE_Cmd_Xfr == CTCE_RCV ) && ( IS_CTCE_MATCH( pCTCE_Info->actions ) ) )
        ? ( CSW_CE | CSW_DE ) : ( pCTCE_Info->x_unit_stat ) ) )
        && !( *pUnitStat & ( CSW_UC | CSW_UX | CSW_SM ) )
        && !( ( eCTCE_Cmd_Xfr == CTCE_RCV ) && ( IS_CTCE_WAIT( pCTCE_Info->actions ) ) )
        &&  ( ( eCTCE_Cmd_Xfr != CTCE_RCV ) || ( *pUnitStat != 0 ) )
        && !( pCTCE_Info->de_ready ) )
    {
        snprintf( ctce_trace_xtra_temp, sizeof( ctce_trace_xtra_temp ),
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
        STRLCAT( ctce_trace_xtra, " CON_WINNER" );
    }

/*

HHC05079I <src_dev> CTCE: <direction> <dst_dev> <seq#> cmd=<cmd>=<cmd_hex>
          xy=<x_src><y_src><direction><x_dst><y_dst> l=<length> k=<chksum>
          Stat=<stat> <extra_msgs>

Explanation
        The CTCE device <local_dev> processes a <cmd> (hex value <cmd_hex>).
        The <direction> shows whether it originates locally (the x-side),
        and if it needs to be send (->) to the remote <remote_dev> device
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

    WRMSG( HHC05079, "I",  /* CTCE: %s %.6s #%04X cmd=%s=%02X xy=%.2s%s%.2s l=%04X k=%08X %s%s%s%s%s%s */
        CTCX_DEVNUM( pDEVBLK ), CTCE_XfrStr[eCTCE_Cmd_Xfr],
        CTCE_FILENAME, ctce_PktSeq,
        CTCE_CmdStr[CTCE_CMD( ctce_Cmd )], ctce_Cmd,
        ctce_state_l_xy, CTCE_XfrStr[eCTCE_Cmd_Xfr],
        ctce_state_r_xy,
        sCount, IS_CTCE_CCW_WRT( ctce_Cmd )
        ? CTCE_ChkSum( pCTCE_Buf + sizeof(CTCE_SOKPFX), sCount )
        : CTCE_ChkSum( pCTCE_Buf, pCTCE_Info->sok_buf_len ),
        ctce_trace_stat,
        CTCE_ACTIONS_PRT( pCTCE_Info->actions ),
        ctce_trace_xtra );
    if( pDEVBLK->ccwstep )
        packet_trace( (BYTE*)pCTCE_Buf, pCTCE_Info->sok_buf_len, '<' );
    return;
}

// ---------------------------------------------------------------------
// CTCE_connect_timeout
// ---------------------------------------------------------------------
//
// connect but with a timeout parameter.
//

int CTCE_Connect_Timeout(int                    sockfd,
                         const struct sockaddr* saptr,
                         const socklen_t        salen,
                         const int              usec,
                         const DEVBLK*          pDEVBLK)
{
    int                   rc;                    // connect return code
    fd_set                read_set, write_set;   // socket sets for select
    static struct timeval connect_timeout;       // max connect wait time

    // Switch to non-blocking mode during the next connect.
    socket_set_blocking_mode( sockfd, 0 ) ;
    rc = connect( sockfd, saptr, salen );

    // In case connect() did not complete immediately then we prepare
    // the select() to wait for the specified amount of time.
    if( rc < 0 )
    {
        FD_ZERO ( &read_set );
        FD_SET( sockfd, &read_set );
        write_set = read_set ;
        connect_timeout.tv_sec = 0;
        connect_timeout.tv_usec = usec;

        // Then we issue select() until it returns a real error.
        // A zero return code means the select() timed out,
        // a positive one signifies the connect() took place,
        // and a negative one signifies a real select() error occurred.
        do
        {
            rc = select( sockfd + 1, &read_set, &write_set, NULL,
                         usec ? &connect_timeout : NULL );
        }
        while( (rc < 0) && ( HSO_errno == HSO_EINTR ) );

        // In case no connection took place, either due to timeout (rc=0),
        // or due to select() error (rc<0), we will close the socket.
        if( rc <= 0 )
        {
            close_socket( sockfd ) ;

            // A connect timeout (rc=0) will be signalled as non-success with rc=-1.
            if( rc == 0 )
            {
                rc = -1;
            }

            // A real select error (rc<0) will be reported.
            else
            {
                WRMSG( HHC05080, "S",  /* CTCE: Socket select() with %d usec timeout error : %s */
                    CTCX_DEVNUM( pDEVBLK ), usec, strerror( HSO_errno )) ;
            }
        }
    }

    // When connected, switch back to original non-blocking mode and return
    if( rc >= 0 )
    {
        socket_set_blocking_mode( sockfd, 1 ) ;
    }

    return rc;
}
