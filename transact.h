/* TRANSACT.H   (C) Copyright Bob Wood, 2019                         */
/*                  Transactional-Execution consts and structs       */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#ifndef _TRANSACT_H_
#define _TRANSACT_H_

/*-------------------------------------------------------------------*/
/*          Transactional-Execution Facility constants               */
/*-------------------------------------------------------------------*/
#define  MAX_TXF_LEVEL           15   /* Max nesting depth           */
#define  MAX_TXF_CONTRAN_INSTR   32   /* Max CONSTRAINED instr.      */
#define  MAX_TXF_PAGES           64   /* Max num of modified pages   */
#define  MAX_TXF_NTSTG          128   /* Max nontransactional stores */
#define  MAX_CAPTURE_TRIES      128   /* Max clean copy attempts     */

#define  ZPAGEFRAME_PAGESIZE   4096   /* IBM z page size (4K)        */
#define  ZPAGEFRAME_BYTEMASK   0x00000FFF
#define  ZPAGEFRAME_PAGEMASK   0xFFFFFFFFFFFFF000ULL

#define  ZCACHE_LINE_SIZE       256   /* IBM z cache line size       */
#define  ZCACHE_LINE_SHIFT        8   /* Cache line size shift value */
#define  ZCACHE_LINE_PAGE           (ZPAGEFRAME_PAGESIZE/ZCACHE_LINE_SIZE)
                                      /* Cache lines per 4K page     */

/*-------------------------------------------------------------------*/
/*        Transactional-Execution Facility Condition Codes           */
/*-------------------------------------------------------------------*/
#define  TXF_CC_SUCCESS         0   /* Trans. successfully initiated */

#define  TXF_CC_INDETERMINATE   1   /* Indeterminate condition;
                                       successful retry unlikely.    */

#define  TXF_CC_TRANSIENT       2   /* Transient condition;
                                       successful retry likely.      */

#define  TXF_CC_PERSISTENT      3   /* Persistent condition;
                                       successful retry NOT likely
                                       under current conditions. If
                                       conditions change, then retry
                                       MIGHT be more productive.     */

/*-------------------------------------------------------------------*/
/*          abort_transaction function 'retry' code                  */
/*-------------------------------------------------------------------*/
#define  ABORT_RETRY_RETURN     0     /* Return to caller. Used when */
                                      /* abort called from e.g. ext
                                         or I/O interrupt processing */
#define  ABORT_RETRY_CC         1     /* Set condition code and then */
                                      /* longjmp to progjmp.         */
#define  ABORT_RETRY_PGMCHK     2     /* PGMCHK if CONSTRAINED mode, */
                                      /* else longjmp to progjmp.    */

/*-------------------------------------------------------------------*/
/*               Non-transactional store table                       */
/*-------------------------------------------------------------------*/
/* The non-transactional store table tracks non-transactional stores */
/* that occur during an unconstrained transaction.  It is needed to  */
/* commit the changes when a transaction abort occurs.               */
/*-------------------------------------------------------------------*/
struct NTRANTBL
{
    U64     effective_addr;     /* virtual address of the store      */
    int     arn;                /* access register value             */
    BYTE    skey;               /* storage key in use                */
    BYTE    amodebits;          /* addressing mode (see below)       */

#define AMODE_64    3           /* 64-bit: psw.amode64==1            */
#define AMODE_31    1           /* 31-bit: psw.amode64==0, amode==1  */
#define AMODE_24    0           /* 24-bit: psw.amode64==0, amode==0  */

    BYTE    resv[2];            /* reserved                          */
    U64     ntran_data;         /* non-transactional data already
                                   in guest big-endian format.       */
};
typedef struct NTRANTBL  NTRANTBL;   // Non-transactional store table

/*-------------------------------------------------------------------*/
/*                   Transaction Page Map                            */
/*-------------------------------------------------------------------*/
struct TPAGEMAP
{
    U64     virtpageaddr;       /* virtual address of mapped page    */
    BYTE*   mainpageaddr;       /* address of main page being mapped */
    BYTE*   altpageaddr;        /* addesss of alternate & save pages */
    BYTE    cachemap[ ZCACHE_LINE_PAGE ];  /* cache line indicators  */

#define CM_CLEAN    0           /* clean cache line (init default)   */
#define CM_FETCHED  1           /* cache line was fetched            */
#define CM_STORED   2           /* cache line was stored into        */
};
typedef struct TPAGEMAP  TPAGEMAP;   // Transaction Page Map table

/*-------------------------------------------------------------------*/
/*                Transaction Diagnostic Block                       */
/*-------------------------------------------------------------------*/
struct TDB
{
    BYTE    tdbformat;          /* format, 0 = invalid, 1 = valid    */
    BYTE    tdbflags;           /* flags                             */

#define TDB_CTV     0x80        /* Conflict-Token Validity           */
#define TDB_CTI     0x40        /* Constrained-Transaction Indicator */

    BYTE    tdbresv1[4];        /* reserved                          */
    HWORD   tdbtnd;             /* TND (Transaction Nesting Depth)   */

    DBLWRD  tdbabortcode;       /* transaction abort code; see below */
    DBLWRD  tdbconfict;         /* conflict token                    */
    DBLWRD  tdbinstaddr;        /* transaction abort inst addr       */

    BYTE    tdbeaid;            /* exception access id               */
    BYTE    tdbdxc;             /* data exception code               */
    BYTE    tdbresv2[2];        /* reserved                          */
    FWORD   tdbpgmintid;        /* program interrupt identification  */

    DBLWRD  tdbtranexcid;       /* transaction exception ident       */
    DBLWRD  tdbbreakaddr;       /* breaking event address            */  
    DBLWRD  tdbresv3[9];        /* reserved                          */

    DBLWRD  tdbgpr[16];         /* general purpose register array    */

#define ABORT_CODE_EXT           2  /* External interruption         */
#define ABORT_CODE_UPGM          4  /* PGM interruption (Unfiltered) */
#define ABORT_CODE_MCK           5  /* Machine-check interruption    */
#define ABORT_CODE_IO            6  /* I/O interruption              */
#define ABORT_CODE_FETCH_OVF     7  /* Fetch overflow                */
#define ABORT_CODE_STORE_OVF     8  /* Store overflow                */
#define ABORT_CODE_FETCH_CNF     9  /* Fetch conflict                */
#define ABORT_CODE_STORE_CNF    10  /* Store conflict                */
#define ABORT_CODE_INSTR        11  /* Restricted instruction        */
#define ABORT_CODE_FPGM         12  /* PGM interruption (Filtered)   */
#define ABORT_CODE_NESTING      13  /* Nesting depth exceeded        */
#define ABORT_CODE_FETCH_OTHER  14  /* Cache -- fetch related        */
#define ABORT_CODE_STORE_OTHER  15  /* Cache -- store related        */
#define ABORT_CODE_CACHE_OTHER  16  /* Cache -- other                */
#define ABORT_CODE_GUARDED      19  /* Guarded-storage event related */
#define ABORT_CODE_MISC        255  /* Miscellaneous condition       */
#define ABORT_CODE_TABORT      256  /* TABORT instruction            */
};
typedef struct TDB  TDB;             // Transaction Dianostic Block

CASSERT( sizeof( TDB ) == 256, transact_h );

#endif // _TRANSACT_H_
