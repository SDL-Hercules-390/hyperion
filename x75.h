/*-------------------------------------------------------------------*/
/* X75.H   (C) Copyright Jason Paul Winter, 2003,2010                */
/*         Minor adaptions for SDL Hyperion, Juergen Winkelmann 2019 */
/*-------------------------------------------------------------------*/

extern int lar_tcpip (DW * regs); /* function in tcpip.c             */
extern unsigned int lar_offset (DW * regs); /* ditto: resume point of */
                                  /* a copy into/out of the host buffer */
extern U_LONG_PTR map32[Ccom];    /* map 64-bit host addresses       */
                                  /* to 32-bit guest registers       */
