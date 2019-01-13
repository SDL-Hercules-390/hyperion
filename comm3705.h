/* COMM3705.H   (C) Copyright Roger Bowler, 2008-2012                */
/*              (C) Copyright MHP, 2007 (see below)                  */
/*              Hercules 3705 communications controller              */
/*              running NCP                                          */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#ifndef __COMM3705_H__
#define __COMM3705_H__

#include "hercules.h"

struct COMMADPT
{
    DEVBLK *dev;                /* the devblk to which this CA is attched   */
    TID  cthread;               /* Thread used to control the socket        */
    TID  tthread;               /* Thread used to control the socket        */
    U16  lport;                 /* Local listening port                     */
    in_addr_t lhost;            /* Local listening address                  */
    int sfd;                    /* Communication socket FD                  */
    int lfd;                    /* Listen socket for DIAL=IN, INOUT & NO    */
    COND ipc;                   /* I/O <-> thread IPC condition EVB         */
    COND ipc_halt;              /* I/O <-> thread IPC HALT special EVB      */
    LOCK lock;                  /* COMMADPT lock                            */
    int pipe[2];                /* pipe used for I/O to thread signaling    */
    char locncpnm[9],           /* name of local NCP (in EBCDIC)            */
         rmtncpnm[9];           /* name of remote NCP (in EBCDIC)           */
    U16  devnum,                /* devnum copy from DEVBLK                  */
         locsuba,               /* local NCP or local 3791 node subarea number */
         rmtsuba;               /* remote NCP subarea number                   */
    U32
        have_cthread:1,         /* the comm thread is running               */
        haltpending:1,          /* A request has been issued to halt current*/
                                /* CCW                                      */
        bindflag:1,
        telnet_opt:1,           /* expecting telnet option char             */
        telnet_iac:1,           /* expecting telnet command char            */
        telnet_int:1,           /* telnet interrupt received                */
        hangup:1,               /* host initated shutdown                   */
        is_3270:1,              /* 0=tty 1=3270                             */
        eol_flag:1,             /* 1 = CR has been received                 */
        debug_sna:1,            /* 1 = write debug messages                 */
        emu3791:1,              /* mode (0=default=3705;1=3791)             */
        idblk,                  /* IDBLK of switched PU (default=0x017)     */
        idnum;                  /* IDNUM of switched PU (default=0x00017)   */
    U32 rlen3270;               /* amt of data in 3270 recv buf             */
    BYTE telnet_cmd;            /* telnet command */

    int read_ccw_count;
    int write_ccw_count;
    int unack_attn_count;

    int ncpa_sscp_seqn;
    int ncpb_sscp_seqn;
    int lu_sscp_seqn;
    int lu_lu_seqn;

    BYTE inpbuf[65536];
    int inpbufl,
        unitsz,                 /* I/O blocksize (default=256)              */
        ackspeed;               /* slow down factor for unacknowledged attn */

    void * freeq;
    void * sendq;
    BYTE * poolarea;

    BYTE sscp_addr0;
    BYTE sscp_addr1;
    BYTE ncp_addr0;
    BYTE ncp_addr1;
    BYTE pu_addr0;
    BYTE pu_addr1;
    BYTE lu_addr0;
    BYTE lu_addr1;
    BYTE tso_addr0;
    BYTE tso_addr1;
};

#endif
