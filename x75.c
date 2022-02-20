/*-------------------------------------------------------------------*/
/* X75.C   (C) Copyright Jason Paul Winter, 2003,2010                */
/*         Minor adaptions for SDL Hyperion, Juergen Winkelmann 2019 */
/*                                                                   */
/* This program was written by Jason Paul Winter. It was originally  */
/* named DYN75.C.                                                    */
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
/* This module implements the TCPIP instruction, which allows guest  */
/* access to the host's TCP/IP stack. Register usage is as follows:  */
/*                                                                   */
/* R0  = 0 (Initially, but turns to > 0 after the native call.       */
/* R1  = Byte Counter                                                */
/* R2  = Source/Destination of PC buffer.  32bits.                   */
/* R3  = Direction (0 = to Host PC, 1 = from Host PC)                */
/* R4  = Returned Bytes                                              */
/*                                                                   */
/* R7 \                                                              */
/* R8 |= Used by the lar_tcpip function (but not here.)              */
/* R9 /                                                              */
/*                                                                   */
/* R14 = Identifier (returned & passed back for conversations.)      */
/* R15 = Work Variable / Return Code                                 */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

#define _X75_C_
#define _HENGINE_DLL_

#include "hercules.h"
#include "opcode.h"
#include "inline.h"

#include "tcpip.h"
#include "x75.h"

#if defined( FEATURE_TCPIP_EXTENSION )
/*-------------------------------------------------------------------*/
/* 75xx TCPIP Ra,yyy(Rb,Rc) Ra=anything, Rc>4<14, Rb=0/ditto  [RX-a] */
/*-------------------------------------------------------------------*/
DEF_INST( tcpip )
{
    int     r1;              /* Value of R field       */
    int     x2;              /* Index register         */
    int     b2;              /* Base of effective addr */
    VADR    effective_addr2; /* Effective address      */
    int     i;
    unsigned char * s;

    /*  vv---vv---------------- input variables to TCPIP */
    RX(inst, regs, r1, x2, b2, effective_addr2);
    /*             ^^-------------- becomes to-store register a */
    /*                 ^^---------- becomes index register b */
    /*                     ^^------ becomes access register c */
    /*                         ^^-- becomes yyy+gr[b]+gr[c] */
    UNREFERENCED(r1);

    if (!FACILITY_ENABLED( HERC_TCPIP_PROB_STATE, regs )) PRIV_CHECK(regs);

    if (regs->GR_L(0) == 0) { /* Only run when R0 = 0, (restart) */

        if (lar_tcpip (&(regs->gr [0])) == 0) { /* Get PC buffer */
            regs->GR_L(15) = -1; /* Error */
            return;
        }

        regs->GR_L(0) = 1;    /* Do not call native routine again */
    }

    if (regs->GR_L(1) != 0) s = (unsigned char *)(map32[regs->GR_L(2)]);

    while (regs->GR_L(1) != 0) { /* Finished > */

        i = regs->GR_L(1) - 1;
        if (i > 255) i = 255;

        if (regs->GR_L(3) == 0) { /* Going to host */
            /* Load bytes from operand address */
            ARCH_DEP(vfetchc) ( s, (unsigned char)i, effective_addr2, b2, regs );
        } else {                  /* Going from host */
            /* Store bytes at operand address */
            ARCH_DEP(vstorec) ( s, (unsigned char)i, effective_addr2, b2, regs );
        }

        i++;
        effective_addr2 += i;  /* No exception, quick copy without calc's */
        (regs->GR_L(b2)) += i; /* Exception, can recalculate if/when restart */
        s += i;                /* Next PC byte segment location */
        (regs->GR_L(1)) -= i;  /* One less segment to copy next time */
    }

    if (lar_tcpip (&(regs->gr [0])) == 0) { /* Run! */
        regs->GR_L(15) = -1; /* Error */
        return;
    }

    regs->GR_L(15) = 0; /* No error */
}
#endif /*defined( FEATURE_TCPIP_EXTENSION )*/

#if !defined( _GEN_ARCH )
  #if defined(           _ARCH_NUM_1 )
    #define   _GEN_ARCH  _ARCH_NUM_1
    #include "x75.c"
  #endif
  #if defined(           _ARCH_NUM_2 )
    #undef    _GEN_ARCH
    #define   _GEN_ARCH  _ARCH_NUM_2
    #include "x75.c"
  #endif
#endif /*!defined( _GEN_ARCH )*/
