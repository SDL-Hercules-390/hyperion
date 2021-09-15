/*-------------------------------------------------------------------*/
/* TCPIP.C (C) Copyright Jason Paul Winter, 2003,2010                */
/*         Minor adaptions for SDL Hyperion, Juergen Winkelmann 2019 */
/*                                                                   */
/* C Socket Interface for the Hercules System/370, ESA/390,          */
/* ------------------         and z/Architecture Emulator            */
/*                                                                   */
/* This program was written by Jason Paul Winter.                    */
/*                                                                   */
/* Copyright (c) 2003-2010, Jason Paul Winter                        */
/* All rights reserved.                                              */
/*                                                                   */
/* Redistribution and use in source and binary forms, with           */
/* or without modification, are permitted provided that              */
/* the following conditions are met:                                 */
/*                                                                   */
/* Redistributions of source code must retain the above              */
/* copyright notice, this list of conditions and the                 */
/* following disclaimer.                                             */
/*                                                                   */
/* Redistributions in binary form must reproduce the above           */
/* copyright notice, this list of conditions and the following       */
/* disclaimer in the documentation and/or other materials            */
/* provided with the distribution.                                   */
/*                                                                   */
/* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND            */
/* CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,       */
/* INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF          */
/* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE          */
/* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR             */
/* CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,      */
/* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,          */
/* BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR            */
/* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS              */
/* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,      */
/* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING         */
/* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE        */
/* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH      */
/* DAMAGE.                                                           */
/*                                                                   */
/*-------------------------------------------------------------------*/
/* This module implements the backend to the TCPIP instruction by    */
/* mapping calls from the guest's socket library (EZASOKET or        */
/* equivalent) to native socket calls on the host.                   */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

#define _TCPIP_C_
#define _HENGINE_DLL_

#include "hercules.h"

#if defined( __sun__ )
#include <sys/filio.h>      // (need FIONBIO and FIONREAD)
#endif

#include "tcpip.h"

static u_int find_slot ( U_LONG_PTR address ) {
    u_int  i;
    i = 0;
    obtain_lock (&tcpip_lock);
    while ((((signed) map32[i]) != -1) && (i < Ccom-1)) i++;
    map32[i] = address;
    release_lock (&tcpip_lock);
    return (i);
}

static void tcpip_init () {
    u_int  i;

    initialize_lock(&tcpip_lock);

    CerrGen = 0;

    i = 0;
    while (i < Ccom) {
        map32[i] = -1;

        Cselect [i] = NULL;

        Cerr [i] = 0;
        Ccom_blk [i] = 1;
        Ccom_han [i] = -1;
        Ccom_opn [i++] = 0;
    }
}

/* This is the error translation routine for whatever host to Hercules: */
static int Get_errno () {
    u_int  i;

#ifndef _MSVC_
    i = errno;
#else
    i = WSAGetLastError ();
#endif

    switch (i) {
    case fEMFILE:
        return (hEMFILE);
/*    case EAGAIN:
        return (hEAGAIN);  SAME AS EWOULDBLOCK*/
    case EWOULDBLOCK:
        return (hEWOULDBLOCK);
    case EINPROGRESS:
        return (hEINPROGRESS);
    case EALREADY:
        return (hEALREADY);
    case ENOTSOCK:
        return (hENOTSOCK);
    case EDESTADDRREQ:
        return (hEDESTADDRREQ);
    case EMSGSIZE:
        return (hEMSGSIZE);
    case EPROTOTYPE:
        return (hEPROTOTYPE);
    case ENOPROTOOPT:
        return (hENOPROTOOPT);
    case EPROTONOSUPPORT:
        return (hEPROTONOSUPPORT);
    case ESOCKTNOSUPPORT:
        return (hESOCKTNOSUPPORT);
    case EOPNOTSUPP:
        return (hEOPNOTSUPP);
    case EPFNOSUPPORT:
        return (hEPFNOSUPPORT);
    case EAFNOSUPPORT:
        return (hEAFNOSUPPORT);
    case EADDRINUSE:
        return (hEADDRINUSE);
    case EADDRNOTAVAIL:
        return (hEADDRNOTAVAIL);
    case ENETDOWN:
        return (hENETDOWN);
    case ENETUNREACH:
        return (hENETUNREACH);
    case ENETRESET:
        return (hENETRESET);
    case ECONNABORTED:
        return (hECONNABORTED);
    case ECONNRESET:
        return (hECONNRESET);
    case ENOBUFS:
        return (hENOBUFS);
    case EISCONN:
        return (hEISCONN);
    case ENOTCONN:
        return (hENOTCONN);
    case ESHUTDOWN:
        return (hESHUTDOWN);
    case ETOOMANYREFS:
        return (hETOOMANYREFS);
    case ETIMEDOUT:
        return (hETIMEDOUT);
    case ECONNREFUSED:
        return (hECONNREFUSED);
    case fEINVAL :
        return (hEINVAL);
    case ELOOP :
        return (hELOOP);
    case fENAMETOOLONG :
        return (hENAMETOOLONG);
    case EHOSTDOWN :
        return (hEHOSTDOWN);
    case EHOSTUNREACH :
        return (hEHOSTUNREACH);
    case fENOTEMPTY :
        return (hENOTEMPTY);
#ifdef EPROCLIM // Some *nixs don't have this error
    case EPROCLIM :
        return (hEPROCLIM);
#endif
    case EUSERS :
        return (hEUSERS);
    case EDQUOT :
        return (hEDQUOT);
    case ESTALE :
        return (hESTALE);
    case EREMOTE :
        return (hEREMOTE);
    }

    return (i);
}

/**********************************************************************************/

#if defined(WORDS_BIGENDIAN)

/*
  regs comming in: (Non Intel Format - msb first.)
           /--R0--\          /--R1--\
  00000000 xxxxxxxx 00000000 yyyyyyyy ...
  (Zeros are the 64bit register parts, never used in S370.)
*/

static void set_reg (u_int  * regs, u_int  r, u_int  v) {
    regs [(r * 2) + 1] = v;
}

static u_int  get_reg (u_int  * regs, u_int  r) {
    return (regs [(r * 2) + 1]);
}

#else

/*
  regs comming in: (Intel Format - lsb first.)
  /--R0--\          /--R1--\
  xxxxxxxx 00000000 yyyyyyyy 00000000 ...
  (Zeros are the 64bit register parts, never used in S370.)
*/

static void set_reg (u_int  * regs, u_int  r, u_int  v) {
    regs [r * 2] = v;
}

static u_int  get_reg (u_int  * regs, u_int  r) {
    return (regs [r * 2]);
}

#endif

/**********************************************************************************/

static u_int  check_not_sock (u_int  s, talk_ptr t) {

    if ((s < 1) || (s >= Ccom)) {
        t->ret_cd = -1;
        return (1);
    }

    if (Ccom_han [s] == -1) {
        t->ret_cd = -1;
        return (1);
    }

    return (0);
}

static void EZASOKET (u_int  func, int  aux1, int  aux2, talk_ptr t) {
    int    i;
    socklen_t  isock;
    int    k;
    int    l;
    int    m;
    int    size;
    struct hostent * hp;
    struct timeval timeout;
    fd_set sockets;
    SOCKADDR_IN Clocal_adx;
    SOCKADDR    Slocal_adx;

    switch (func & 0xFF) {
    case 1:  /* INITAPI */

        t->ret_cd = 0;
        return;

    case 2:  /* GETERRORS */

        if (check_not_sock (aux1, t)) {
            t->ret_cd = hENOTSOCK;
            return;
        }

        t->ret_cd = Cerr [aux1];
        return;

    case 3:  /* GETERROR */

        t->ret_cd = CerrGen;
        return;

    case 4:  /* GETHOSTBYNAME */

        m = strlen (t->buffer_in);
        k = 0;
        while (m) {
            t->buffer_in [k] = DCCebcdic_to_ascii [(unsigned char)(t->buffer_in [k])];
            if ((unsigned char)t->buffer_in [k] == 0x20) {t->buffer_in [k] = 0; break;};
            k++;
            m--;
        }

        if ((hp = gethostbyname (t->buffer_in)) == NULL) {

            CerrGen = Get_errno ();

            t->ret_cd = 0;
            return;
        } else {
            if (hp->h_addr_list [0] == NULL) {
                t->ret_cd = 0;
                return;
            } else {
                t->ret_cd = (ntohl(((u_int  *)(hp->h_addr_list [0])) [0]));
                return;
            }
        }

    case 5:  /* SOCKET */

        obtain_lock (&tcpip_lock);

        m = 1;
        while (m < Ccom) {
            if (Ccom_opn [m] == 0) break;
            m++;
        }
        if (m == Ccom) {

            CerrGen = hEMFILE;

            release_lock (&tcpip_lock);

            t->ret_cd = -1;
            return;  /* None available. */
        }

        i = aux1 >> 16; /* Family: eg. PF_INET */

        if ((Ccom_han [m] = socket (i, (aux1 & 0xFFFF), aux2)) == INVALID_SOCKET) {

            CerrGen = Get_errno ();

            release_lock (&tcpip_lock);

            t->ret_cd = -1;
            return;  /* ERROR */
        }

        Ccom_opn [m] = 1;

        release_lock (&tcpip_lock);

        t->ret_cd = m;
        return;

    case 6:  /* BIND */

        /* FUNC:SOCK&FUNC, AUX1:ADDRESS, AUX2:FAMILY&PORT */

        m = func >> 16; /* SOCKET # */
        i = aux2 >> 16; /* Family */

        if (check_not_sock (m, t)) return;

#if defined(__APPLE__)
        bzero ((LPSOCKADDR)&Clocal_adx, sizeof (Clocal_adx)); /* cleanup address */
#endif

        /* set up socket */
        Clocal_adx.sin_family = (short)(i & 0xFF);

        Clocal_adx.sin_addr.s_addr = htonl (aux1);

        Clocal_adx.sin_port = htons ((unsigned short)(aux2 & 0xFFFF));

        if (bind (Ccom_han [m], (LPSOCKADDR)&Clocal_adx, sizeof (Clocal_adx))) {

            Cerr [m] = Get_errno ();

            t->ret_cd = -1;
            return;  /* ERROR */
        }

        t->ret_cd = 0;
        return;

    case 7:  /* CONNECT */

        m = func >> 16; /* SOCKET # */
        i = aux2 >> 16; /* Family */

        if (check_not_sock (m, t)) return;

        /* set up socket */
        Clocal_adx.sin_family = (short)(i & 0xFF);

        Clocal_adx.sin_addr.s_addr = htonl (aux1);

        Clocal_adx.sin_port = htons ((unsigned short)(aux2 & 0xFFFF));

        k = 1;
        ioctlsocket (Ccom_han [m], FIONBIO, &k);

        i = connect (Ccom_han [m], (LPSOCKADDR)&Clocal_adx, sizeof (Clocal_adx));
        Cerr [m] = Get_errno ();

        k = 0;
        ioctlsocket (Ccom_han [m], FIONBIO, &k);

        if (i == -1) {

            switch (Cerr [m]) {
            case hEISCONN :
                t->ret_cd = 0;  /* Worked! */
                return;

            case hEINVAL :      /* WSAEALREADY in Windows old sockets */
            case hEALREADY :
            case hEWOULDBLOCK :
            case hEINPROGRESS :

                if (Ccom_blk [m] == 0) {
                    t->ret_cd = -1;
                    return;  /* ERROR, But Client Knows... */
                }

                t->ret_cd = -2; /* Must wait. */
                return;
            }

            t->ret_cd = -1;
            return;  /* ERROR */
        }

        t->ret_cd = 0;
        return;

    case 8:  /* LISTEN */

        if (check_not_sock (aux1, t)) return;

        i = aux2;
        if (i == 0) i = SOMAXCONN;

        if (listen (Ccom_han [aux1], i)) {

            Cerr [aux1] = Get_errno ();

            t->ret_cd = -1;
            return;  /* ERROR */
        }

        t->ret_cd = 0;
        return;

    case 9:  /* ACCEPT */

        if (check_not_sock (aux1, t)) return;

        obtain_lock (&tcpip_lock);

        m = 1;
        while (m < Ccom) {
            if (Ccom_opn [m] == 0) break;
            m++;
        }

        if (m == Ccom) {

            Cerr [aux1] = hEMFILE;

            release_lock (&tcpip_lock);

            t->ret_cd = -1;
            return; /* None available. */
        }

        isock = sizeof (Slocal_adx);

        timeout.tv_sec = 0;
        timeout.tv_usec = 0;

        FD_ZERO (&sockets);
        FD_SET (Ccom_han [aux1], &sockets);

        if (select (Ccom_han [aux1] + 1, &sockets, NULL, NULL, &timeout) == 0) {

            release_lock (&tcpip_lock);

            if (Ccom_blk [aux1]) {
                t->ret_cd = -2;
            } else {
                Cerr [aux1] = hEWOULDBLOCK;
                t->ret_cd = -1;
            }
            return;
        }

        l = Ccom_han [m] = accept (Ccom_han [aux1], &Slocal_adx, &isock);

        if (l == INVALID_SOCKET) {

            Cerr [aux1] = Get_errno ();

            release_lock (&tcpip_lock);

            t->ret_cd = -1;
            return;  /* ERROR */
        }

        Ccom_opn [m] = 1;

        release_lock (&tcpip_lock);

        t->len_out = sizeof (Clocal_adx);
        t->buffer_out = (char *)malloc (t->len_out);
        t->buffer_out_slot = find_slot ((U_LONG_PTR)&(t->buffer_out [0]));
        memcpy (t->buffer_out, &Slocal_adx, t->len_out);
        ((SOCKADDR_IN *)(t->buffer_out))->sin_family = htons (((SOCKADDR_IN *)(t->buffer_out))->sin_family);

        t->ret_cd = m;
        return;

    case 10: /* SEND */

        if (check_not_sock (aux1, t)) return;

        if ((l = send (Ccom_han [aux1], t->buffer_in, t->len_in, 0)) == SOCKET_ERROR) {

            Cerr [aux1] = Get_errno ();

            t->ret_cd = -1;
            return;
        }

        t->ret_cd = l;
        return;

    case 11: /* RECV */

        if (check_not_sock (aux1, t)) return;

        timeout.tv_sec = 0;
        timeout.tv_usec = 0;

        FD_ZERO (&sockets);
        FD_SET (Ccom_han [aux1], &sockets);

        if (select (Ccom_han [aux1] + 1, &sockets, NULL, NULL, &timeout) == 0) {

            if (Ccom_blk [aux1]) {
                t->ret_cd = -2;
            } else {
                Cerr [aux1] = hEWOULDBLOCK;
                t->ret_cd = -1;
            }
            return;
        }

        t->buffer_out = (char *)malloc (aux2);
        t->buffer_out_slot = find_slot ((U_LONG_PTR)&(t->buffer_out [0]));

        if ((size = recv (Ccom_han [aux1], t->buffer_out, aux2, 0)) == SOCKET_ERROR) {    /* receive command */

            Cerr [aux1] = Get_errno ();

            t->len_out = 0;
            map32[t->buffer_out_slot] = -1;
            free (t->buffer_out);
            t->buffer_out = NULL;

            t->ret_cd = -1;
            return;
        }

        t->len_out = size;
        t->ret_cd = size;
        return;

    case 12: /* CLOSE */

        if ((aux1 > 0) && (aux1 < Ccom)) {
            /* close connection */
            if (Ccom_opn [aux1])
                closesocket (Ccom_han [aux1]);

            Ccom_opn [aux1] = 0;
            Ccom_han [aux1] = -1;
            Ccom_blk [aux1] = 1;
        }

        t->ret_cd = 0;
        return;

    case 13: /* EBCDIC2ASCII */

        t->len_out = t->len_in;
        t->buffer_out = (char *)malloc (t->len_out);
        t->buffer_out_slot = find_slot ((U_LONG_PTR)&(t->buffer_out [0]));

        m = t->len_out;
        i = 0;
        k = 0;
        while (m) {
            t->buffer_out [k++] = DCCebcdic_to_ascii [(unsigned char)(t->buffer_in [i++])];
            m--;
        }

        t->ret_cd = t->len_out;
        return;

    case 14: /* ASCII2EBCDIC */

        t->len_out = t->len_in;
        t->buffer_out = (char *)malloc (t->len_out);
        t->buffer_out_slot = find_slot ((U_LONG_PTR)&(t->buffer_out [0]));

        m = t->len_out;
        i = 0;
        k = 0;
        while (m) {
            t->buffer_out [k++] = DCCascii_to_ebcdic [(unsigned char)(t->buffer_in [i++])];
            m--;
        }

        t->ret_cd = t->len_out;
        return;

    case 15: /* IOCTL */

        m = func >> 16;

        if (check_not_sock (m, t)) return;

        if (aux1 == 1) {

            if (aux2) {
                Ccom_blk [m] = 0; /* No longer blocking */
            } else {
                Ccom_blk [m] = 1;
            }

            t->ret_cd = 0;

        } else {

            i = ioctlsocket (Ccom_han [m], FIONREAD, &(t->ret_cd));
            if (i == -1) {
                t->ret_cd = -1;
                Cerr [m] = Get_errno ();
            }
        }

        return;

    case 16: /* GETSOCKNAME */

        if (check_not_sock (aux1, t)) return;

        t->len_out = sizeof (Clocal_adx);
        t->buffer_out = (char *)malloc (t->len_out);
        t->buffer_out_slot = find_slot ((U_LONG_PTR)&(t->buffer_out [0]));

        isock = t->len_out;
        t->ret_cd = getsockname (Ccom_han [aux1], (struct sockaddr *)(t->buffer_out), &isock);

        if (t->ret_cd == -1) {
            Cerr [aux1] = Get_errno ();
        } else {
            ((SOCKADDR_IN *)(t->buffer_out))->sin_family = htons (((SOCKADDR_IN *)(t->buffer_out))->sin_family);
        }

        return;

    case 17: /* SELECT */

        /* func>>16 = socket
           aux1 = func subcode
           aux2 = maxsock+1 */

        m = func >> 16;

        if (check_not_sock (m, t)) return;

        switch (aux1 & 0xFF) {
        case 0:  /* Start */
            if (Cselect [m] == NULL) { /* Start-part */

                Cselect [m] = malloc (sizeof (selects));

                Cselect [m]->ri = malloc (sizeof (fd_set));
                Cselect [m]->wi = malloc (sizeof (fd_set));
                Cselect [m]->ei = malloc (sizeof (fd_set));

                Cselect [m]->ro = malloc (sizeof (fd_set));
                Cselect [m]->wo = malloc (sizeof (fd_set));
                Cselect [m]->eo = malloc (sizeof (fd_set));
            }

            FD_ZERO ((fd_set *)(Cselect [m]->ri));
            FD_ZERO ((fd_set *)(Cselect [m]->wi));
            FD_ZERO ((fd_set *)(Cselect [m]->ei));

            Cselect [m]->len = 0;
            Cselect [m]->invalid = 0;

            t->ret_cd = 0;
            return;

        case 1:  /* Read Inputs */

            Cselect [m]->len = t->len_in; /* Copy every time... */

            /* Need to bswap every long in the incoming array... */
            i = t->len_in / 4;
            while (i) {
                i--;
                ((u_int  *)t->buffer_in) [i] = htonl (((u_int  *)t->buffer_in) [i]);
            }

            i = 0;
            while (i < aux2) {
                if (hercFD_ISSET (i, (hercfd_set *)(t->buffer_in))) {
                    if (Ccom_han [i] == -1) {
                        Cselect [m]->invalid = 1;
                    } else {
                        FD_SET (Ccom_han [i], (fd_set *)(Cselect [m]->ri));
                    }
                }
                i++;
            }

            t->ret_cd = 0;
            return;

        case 2:  /* Write Inputs */

            Cselect [m]->len = t->len_in; /* Copy every time... */

            /* Need to bswap every long in the incoming array... */
            i = t->len_in / 4;
            while (i) {
                i--;
                ((u_int  *)t->buffer_in) [i] = htonl (((u_int  *)t->buffer_in) [i]);
            }

            i = 0;
            while (i < aux2) {
                if (hercFD_ISSET (i, (hercfd_set *)(t->buffer_in))) {
                    if (Ccom_han [i] == -1) {
                        Cselect [m]->invalid = 1;
                    } else {
                        FD_SET (Ccom_han [i], (fd_set *)(Cselect [m]->wi));
                    }
                }
                i++;
            }

            t->ret_cd = 0;
            return;

        case 3:  /* Exception Inputs */

            Cselect [m]->len = t->len_in; /* Copy every time... */

            /* Need to bswap every long in the incoming array... */
            i = t->len_in / 4;
            while (i) {
                i--;
                ((u_int  *)t->buffer_in) [i] = htonl (((u_int  *)t->buffer_in) [i]);
            }

            i = 0;
            while (i < aux2) {
                if (hercFD_ISSET (i, (hercfd_set *)(t->buffer_in))) {
                    if (Ccom_han [i] == -1) {
                        Cselect [m]->invalid = 1;
                    } else {
                        FD_SET (Ccom_han [i], (fd_set *)(Cselect [m]->ei));
                    }
                }
                i++;
            }

            t->ret_cd = 0;
            return;

        case 4:  /* Run 'select' (never blocking here, so may loop back.) */

            memcpy (Cselect [m]->ro, Cselect [m]->ri, sizeof (fd_set));
            memcpy (Cselect [m]->wo, Cselect [m]->wi, sizeof (fd_set));
            memcpy (Cselect [m]->eo, Cselect [m]->ei, sizeof (fd_set));

            if (Cselect [m]->invalid) {
                Cerr [m] = hENOTSOCK;
                t->ret_cd = -1;
                return;
            }

            timeout.tv_sec = 0;
            timeout.tv_usec = 0;

            i = select (Ccom_han [aux2 - 1] + 1,
                        (fd_set *)(Cselect [m]->ro),
                        (fd_set *)(Cselect [m]->wo),
                        (fd_set *)(Cselect [m]->eo),
                        &timeout);

            if (i == 0) {
                t->ret_cd = -2; /* Let the caller decide what to do. */
            } else {
                t->ret_cd = i;
            }
            return;

        case 5:  /* Read Outputs */

            t->len_out = Cselect [m]->len;
            t->buffer_out = (char *)malloc (t->len_out);
            t->buffer_out_slot = find_slot ((U_LONG_PTR)&(t->buffer_out [0]));
            memset (t->buffer_out, 0, t->len_out);

            i = 0;
            while (i < aux2) {
                if (Ccom_han [i] != -1)
                    if (FD_ISSET (Ccom_han [i], (fd_set *)(Cselect [m]->ro)))
                        hercFD_SET (i, (hercfd_set *)(t->buffer_out));
                i++;
            }

            /* Need to bswap every long in the outgoing array... */
            i = t->len_out / 4;
            while (i) {
                i--;
                ((u_int  *)t->buffer_out) [i] = htonl (((u_int  *)t->buffer_out) [i]);
            }

            t->ret_cd = 0;
            return;

        case 6:  /* Write Outputs */

            t->len_out = Cselect [m]->len;
            t->buffer_out = (char *)malloc (t->len_out);
            t->buffer_out_slot = find_slot ((U_LONG_PTR)&(t->buffer_out [0]));
            memset (t->buffer_out, 0, t->len_out);

            i = 0;
            while (i < aux2) {
                if (Ccom_han [i] != -1)
                    if (FD_ISSET (Ccom_han [i], (fd_set *)(Cselect [m]->wo)))
                        hercFD_SET (i, (hercfd_set *)(t->buffer_out));
                i++;
            }

            /* Need to bswap every long in the outgoing array... */
            i = t->len_out / 4;
            while (i) {
                i--;
                ((u_int  *)t->buffer_out) [i] = htonl (((u_int  *)t->buffer_out) [i]);
            }

            t->ret_cd = 0;
            return;

        case 7:  /* Exception Outputs */

            t->len_out = Cselect [m]->len;
            t->buffer_out = (char *)malloc (t->len_out);
            t->buffer_out_slot = find_slot ((U_LONG_PTR)&(t->buffer_out [0]));
            memset (t->buffer_out, 0, t->len_out);

            i = 0;
            while (i < aux2) {
                if (Ccom_han [i] != -1)
                    if (FD_ISSET (Ccom_han [i], (fd_set *)(Cselect [m]->eo)))
                        hercFD_SET (i, (hercfd_set *)(t->buffer_out));
                i++;
            }

            /* Need to bswap every long in the outgoing array... */
            i = t->len_out / 4;
            while (i) {
                i--;
                ((u_int  *)t->buffer_out) [i] = htonl (((u_int  *)t->buffer_out) [i]);
            }

            t->ret_cd = 0;
            return;

        case 8:  /* Finish */

            if (Cselect [m] != NULL) { /* Clean-up */

                free (Cselect [m]->ri);
                free (Cselect [m]->wi);
                free (Cselect [m]->ei);

                free (Cselect [m]->ro);
                free (Cselect [m]->wo);
                free (Cselect [m]->eo);

                free (Cselect [m]);
                Cselect [m] = NULL;
            }

            t->ret_cd = 0;
            return;
        }

        return;

    case 18:  /* GETHOSTBYADDR */

        isock = sizeof (Slocal_adx);
        i = htonl (aux1);

#ifdef _MSVC_
        if ((hp = gethostbyaddr ((const char *) &i, isock, AF_INET)) == NULL) {
#else
        if ((hp = gethostbyaddr (&i, isock, AF_INET)) == NULL) {
#endif

            CerrGen = Get_errno ();

            t->ret_cd = 0;
            return;
        } else {
            if (!(t->ret_cd = strlen (hp->h_name) )) {
                t->ret_cd = 0;
                return;
            } else {

                t->len_out = t->ret_cd;
                t->buffer_out = (char *) malloc (t->len_out);
                t->buffer_out_slot = find_slot ((U_LONG_PTR)&(t->buffer_out [0]));

                m = t->ret_cd;
                i = 0;
                k = 0;
                while (m) {
                    t->buffer_out [k++] = DCCascii_to_ebcdic [(unsigned char)(hp->h_name [i++])];
                    m--;
                }
                return;
            }
        }

    case 19:  /* GETPEERNAME */

        if (check_not_sock (aux1, t)) return;

        t->len_out = sizeof (Clocal_adx);
        t->buffer_out = (char *) malloc (t->len_out);
        t->buffer_out_slot = find_slot ((U_LONG_PTR)&(t->buffer_out [0]));

        isock = t->len_out;
        t->ret_cd = getpeername (Ccom_han [aux1], (struct sockaddr *)(t->buffer_out), &isock);

        if (t->ret_cd == -1) {
            Cerr [aux1] = Get_errno ();
        } else {
            ((SOCKADDR_IN *)(t->buffer_out))->sin_family = htons (((SOCKADDR_IN *)(t->buffer_out))->sin_family);
        }

        return;

    }

    t->ret_cd = 0;
    return;
}

/**********************************************************************************/
/*
  R0  = 0 (Initially, but turns to > 0 after the native call.
  R1  = Byte Counter
  R2  = Source/Destination of PC buffer.  32bits.
  R3  = Direction (0 = to Host PC, 1 = from Host PC)
  R4  = Returned Bytes

  R7  = Function
  R8  = Aux. value 1
  R9  = Aux. value 2

  R14 = Identifier (returned & passed back for conversations.)
  R15 = Work Variable / Return Code
*/

u_int  lar_tcpip (u_int  * regs) {
    talk_ptr t;

    if (tcpip_init_req) {
        tcpip_init_req = 0;
        tcpip_init ();
    }

    if (get_reg (regs, 0) == 0) { /* Initial call. */

        if (get_reg (regs, 3) == 0) { /* Alloc memory for this communication. */

            t = (talk_ptr)malloc (sizeof (talk));
            t->slot = find_slot ((U_LONG_PTR)t);

            t->len_in = get_reg (regs, 1);
            t->buffer_in = (char *)malloc (t->len_in + 1);
            t->buffer_in_slot = find_slot ((U_LONG_PTR)&(t->buffer_in [0]));
            t->buffer_in [t->len_in] = 0; /* NULL Terminator */

            t->len_out = 0;
            t->buffer_out = NULL; /* I've got nothing, at the moment */
            t->ret_cd = 0;

            set_reg (regs, 14, (u_int )t->slot);

            set_reg (regs, 2, (u_int )t->buffer_in_slot);

        } else {                      /* They want some return info... */

            t = (talk_ptr)map32[get_reg (regs, 14)];

            set_reg (regs, 1, t->len_out);
            set_reg (regs, 2, (u_int )t->buffer_out_slot);

            set_reg (regs, 4, t->ret_cd);
        }

    } else {                      /* Must need additional processing. */

        t = (talk_ptr)map32[get_reg (regs, 14)];

        if (get_reg (regs, 3) == 0) { /* Run. */

            EZASOKET (get_reg (regs, 7), get_reg (regs, 8), get_reg (regs, 9), t);

        } else {                      /* Dealloc memory for this communication. */

            map32[t->buffer_in_slot] = -1;
            free (t->buffer_in);
            if (t->buffer_out) {
                map32[t->buffer_out_slot] = -1;
                free (t->buffer_out);
            }
            map32[t->slot] = -1;
            free (t);

            set_reg (regs, 14, 0);
        }
    }

    return (1); /* We never return 0, that's for the DLL Loader. */
}

/**********************************************************************************/
