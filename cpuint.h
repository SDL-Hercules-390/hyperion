/* CPUINT.H     (C) Copyright Jan Jaeger, 2001-2012                  */
/*              Hercules Interrupt State and Mask Definitions        */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/**********************************************************************
 Interrupts_State & Interrupts_Mask bits definition (Initial_Mask=800E)
 Machine check, PER and external interrupt subclass bit positions
 are fixed by the architecture and cannot be changed
 Floating interrupts are made pending to all CPUs, and are
 recorded in the sysblk structure, CPU specific interrupts
 are recorded in the regs structure.
 hi0m mmmm pppp pppp xxxx xxxx xxxx h0hs : type U32
 || | |||| |||| |||| |||| |||| |||| |-||   h:mask  is always '1'
 || | |||| |||| |||| |||| |||| |||| | ||   s:state is always '1'
 || | |||| |||| |||| |||| |||| |||| | |+--> '1' : PSW_WAIT
 || | |||| |||| |||| |||| |||| |||| | +---> '1' : RESTART
 || | |||| |||| |||| |||| |||| |||| |             (available)
 || | |||| |||| |||| |||| |||| |||| +-----> '1' : STORSTAT
 || | |||| |||| |||| |||| |||| ||||
 || | |||| |||| |||| |||| |||| |||+-------> '1' : ETR
 || | |||| |||| |||| |||| |||| ||+--------> '1' : EXTSIG
 || | |||| |||| |||| |||| |||| |+---------> '1' : INTKEY
 || | |||| |||| |||| |||| |||| +----------> '1' : ITIMER
 || | |||| |||| |||| |||| ||||
 || | |||| |||| |||| |||| |||+------------> '1' : ECPS VTIMER
 || | |||| |||| |||| |||| ||+-------------> '1' : SERVSIG
 || | |||| |||| |||| |||| |+--------------> '1' : PTIMER
 || | |||| |||| |||| |||| +---------------> '1' : CLKC
 || | |||| |||| |||| ||||
 || | |||| |||| |||| |||+-----------------> '1' : TODSYNC
 || | |||| |||| |||| ||+------------------> '1' : EXTCALL
 || | |||| |||| |||| |+-------------------> '1' : EMERSIG
 || | |||| |||| |||| +--------------------> '1' : MALFALT
 || | |||| |||| ||||
 || | |||| |||| |||+----------------------> '1' : PER IFNUL
 || | |||| |||| ||+-----------------------> '1' : PER TEND
 || | |||| |||| |+------------------------> '1' : PER ZEROADDR
 || | |||| |||| +-------------------------> '1' : PER STURA
 || | |||| ||||
 || | |||| |||+---------------------------> '1' : PER GRA or SKEY
 || | |||| ||+----------------------------> '1' : PER SA
 || | |||| |+-----------------------------> '1' : PER IF
 || | |||| +------------------------------> '1' : PER SB
 || | ||||
 || | |||+--------------------------------> '1' : WARNING
 || | ||+---------------------------------> '1' : XDMGRPT
 || | |+----------------------------------> '1' : DGRDRPT
 || | +-----------------------------------> '1' : RCVYRPT
 || |
 || +-------------------------------------> '1' : CHANRPT
 ||
 |+---------------------------------------> '1' : IO
 +----------------------------------------> '1' : INTERRUPT possible
**********************************************************************/

// Hercules internal bit# macro: bit numbers are referenced counting
// from the RIGHT to left (i.e. 31 <-- 0) and thus the numerical value
// of a set bit is equal to two raised to the power of the bit position.

// While this is the COMPLETE OPPOSITE from the way bit numbers are
// numbered in IBM architectural reference manuals, we do it this way
// anyway since that's the way Intel numbers their bits, thus allowing
// us to easily replace bit referencing statements with corresponding
// Intel assembler bit manipulation instructions (since Intel is still
// the predominant host architecture platform for Hercules)

/* WARNING! The BIT() macro should only be used for bit numbers
   which are strictly less than 32!
*/
#ifndef    BIT
  #define  BIT(nr)  (((U32)1) << (nr))  // (bit# counting from the right)
#endif

/* The CPU_BIT macro is solely for manipulating
   the CPU number as a CPU bit in a CPU_BITMAP */
#ifndef CPU_BIT
  #define CPU_BIT(nr) ( ((  CPU_BITMAP ) ( 1 ) ) << ( nr ) )
#endif

/* Interrupt bit numbers */
#define IC_INTERRUPT       31 /* 0x80000000 */
#define IC_IO              30 /* 0x40000000 */
#define IC_UNUSED_29       29 /* 0x20000000 */
#define IC_CHANRPT         28 /* 0x10000000 - Architecture dependent (CR14) */
#define IC_RCVYRPT         27 /* 0x08000000 - Architecture dependent (CR14) */
#define IC_DGRDRPT         26 /* 0x04000000 - Architecture dependent (CR14) */
#define IC_XDMGRPT         25 /* 0x02000000 - Architecture dependent (CR14) */
#define IC_WARNING         24 /* 0x01000000 - Architecture dependent (CR14) */
#define IC_PER_SB          23 /* 0x00800000 - Architecture dependent (CR9 >> 8) */
#define IC_PER_IF          22 /* 0x00400000 - Architecture dependent (CR9 >> 8) */
#define IC_PER_SA          21 /* 0x00200000 - Architecture dependent (CR9 >> 8) */
#define IC_PER_GRA         20 /* 0x00100000 - Architecture dependent (CR9 >> 8) */
#define IC_PER_SKEY        20 /* 0x00100000 - Architecture dependent (CR9 >> 8) */
#define IC_PER_STURA       19 /* 0x00080000 - Architecture dependent (CR9 >> 8) */
#define IC_PER_ZEROADDR    18 /* 0x00040000 - Architecture dependent (CR9 >> 8) */
#define IC_PER_TEND        17 /* 0x00020000 - Architecture dependent (CR9 >> 8) */
#define IC_PER_IFNUL       16 /* 0x00010000 - Architecture dependent (CR9 >> 8) */
#define IC_MALFALT         15 /* 0x00008000 - Architecture dependent (CR0) */
#define IC_EMERSIG         14 /* 0x00004000 - Architecture dependent (CR0) */
#define IC_EXTCALL         13 /* 0x00002000 - Architecture dependent (CR0) */
#define IC_TODSYNC         12 /* 0x00001000 - Architecture dependent (CR0) */
#define IC_CLKC            11 /* 0x00000800 - Architecture dependent (CR0) */
#define IC_PTIMER          10 /* 0x00000400 - Architecture dependent (CR0) */
#define IC_SERVSIG          9 /* 0x00000200 - Architecture dependent (CR0) */
#define IC_ECPSVTIMER       8 /* 0x00000100 - Not Architecture dependent */
#define IC_ITIMER           7 /* 0x00000080 - Architecture dependent (CR0) */
#define IC_INTKEY           6 /* 0x00000040 - Architecture dependent (CR0) */
#define IC_EXTSIG           5 /* 0x00000020 - Architecture dependent (CR0) */
#define IC_ETR              4 /* 0x00000010 - Architecture dependent (CR0) */
#define IC_STORSTAT         3 /* 0x00000008 */
#define IC_UNUSED_2         2 /* 0x00000004 */
#define IC_RESTART          1 /* 0x00000002 */
#define IC_PSW_WAIT         0 /* 0x00000001 */

/* Initial values */
#define IC_INITIAL_STATE   BIT(IC_PSW_WAIT)
#define IC_INITIAL_MASK  ( BIT(IC_INTERRUPT) \
                         | BIT(IC_RESTART) \
                         | BIT(IC_STORSTAT) \
                         )

#define SET_IC_INITIAL_MASK(_regs)  (_regs)->ints_mask = IC_INITIAL_MASK
#define SET_IC_INITIAL_STATE(_regs) (_regs)->ints_state = IC_INITIAL_STATE

/* I/O interrupt subclasses */
#define IC_IOPENDING     ( BIT(IC_IO) )

/* External interrupt subclasses in CR 0 */
#define IC_EXT_SCM_CR0   ( BIT(IC_MALFALT) \
                         | BIT(IC_EMERSIG) \
                         | BIT(IC_EXTCALL) \
                         | BIT(IC_TODSYNC) \
                         | BIT(IC_CLKC) \
                         | BIT(IC_PTIMER) \
                         | BIT(IC_SERVSIG) \
                         | BIT(IC_ITIMER) \
                         | BIT(IC_INTKEY) \
                         | BIT(IC_EXTSIG) \
                         | BIT(IC_ETR) \
                         )

/* External interrupt subclasses */

/*
 * Adds ECPS:VM Vtimer which has no individual
 * subclass mask in CR0
 */
#define IC_EXTPENDING    ( BIT(IC_MALFALT) \
                         | BIT(IC_EMERSIG) \
                         | BIT(IC_EXTCALL) \
                         | BIT(IC_TODSYNC) \
                         | BIT(IC_CLKC) \
                         | BIT(IC_PTIMER) \
                         | BIT(IC_SERVSIG) \
                         | BIT(IC_ECPSVTIMER) \
                         | BIT(IC_ITIMER) \
                         | BIT(IC_INTKEY) \
                         | BIT(IC_EXTSIG) \
                         | BIT(IC_ETR) \
                         )

/* Machine check subclasses */
#define IC_MCKPENDING    ( BIT(IC_CHANRPT) \
                         | BIT(IC_RCVYRPT) \
                         | BIT(IC_DGRDRPT) \
                         | BIT(IC_XDMGRPT) \
                         | BIT(IC_WARNING) \
                         )

/* Not disabled mask */
#define IC_OPEN_MASK     ( IC_MCKPENDING \
                         | IC_EXTPENDING \
                         | IC_IOPENDING \
                         )

#define IC_PER_MASK      ( BIT(IC_PER_SB) \
                         | BIT(IC_PER_IF) \
                         | BIT(IC_PER_SA) \
                         | BIT(IC_PER_GRA) \
                         | BIT(IC_PER_SKEY) \
                         | BIT(IC_PER_STURA) \
                         | BIT(IC_PER_ZEROADDR) \
                         | BIT(IC_PER_TEND) \
                         | BIT(IC_PER_IFNUL) \
                         )

/* SIE & Assist supported events */
#define IC_SIE_INT       ( BIT(IC_IO) \
                         | BIT(IC_CLKC) \
                         | BIT(IC_PTIMER) \
                         | BIT(IC_ITIMER) \
                         )

#define IC_CR9_SHIFT 8

/* Mask bits are ONLY set by the associated cpu thread and
 * therefore don't have to be serialized.  The mask bits
 * indicate which interrupts the CPU is willing to take at
 * this time.  We set the mask bits below in one fell swoop
 * to avoid multiple updates to `ints_mask'.
 */

#undef IC_CR0_TO_INTMASK
#if defined(FEATURE_ECPSVM)

#define IC_CR0_TO_INTMASK(_regs) \
(  ( (_regs)->CR(0) & IC_EXT_SCM_CR0) \
  | (((_regs)->CR(0) & BIT(IC_ITIMER)) ? BIT(IC_ECPSVTIMER) : 0) )
#else

#define IC_CR0_TO_INTMASK(_regs) \
 ( (_regs)->CR(0) & IC_EXT_SCM_CR0)

#endif /* FEATURE_ECPSVM */

#define IC_MASK(_regs) \
 ( ( IC_INITIAL_MASK ) \
 | ( ECMODE(&(_regs)->psw) \
     ? ( ((_regs)->psw.sysmask & PSW_IOMASK) ? BIT(IC_IO) : 0 ) \
     : ( ((_regs)->psw.sysmask & 0xFE) ? BIT(IC_IO) : 0 ) \
   ) \
 | ( MACHMASK(&(_regs)->psw) ? ((_regs)->CR(14) & IC_MCKPENDING) : 0 ) \
 | ( PER_MODE((_regs)) ? ((_regs)->ints_mask & IC_PER_MASK) : 0 ) \
 | ( ((_regs)->psw.sysmask & PSW_EXTMASK) ? (IC_CR0_TO_INTMASK((_regs))) : 0 ) \
 | ( WAITSTATE(&(_regs)->psw) ? BIT(IC_PSW_WAIT) : 0 ) \
 )

#define IC_ECMODE_MASK(_regs) \
 ( ( IC_INITIAL_MASK ) \
 | ( ((_regs)->psw.sysmask & PSW_IOMASK) ? BIT(IC_IO) : 0 ) \
 | ( MACHMASK(&(_regs)->psw) ? ((_regs)->CR(14) & IC_MCKPENDING) : 0 ) \
 | ( PER_MODE((_regs)) ? ((_regs)->ints_mask & IC_PER_MASK) : 0 ) \
 | ( ((_regs)->psw.sysmask & PSW_EXTMASK) ? (IC_CR0_TO_INTMASK((_regs))) : 0 ) \
 | ( WAITSTATE(&(_regs)->psw) ? BIT(IC_PSW_WAIT) : 0 ) \
 )

#define IC_BCMODE_MASK(_regs) \
 ( ( IC_INITIAL_MASK ) \
 | ( ((_regs)->psw.sysmask & 0xFE) ? BIT(IC_IO) : 0 ) \
 | ( MACHMASK(&(_regs)->psw) ? ((_regs)->CR(14) & IC_MCKPENDING) : 0 ) \
 | ( PER_MODE((_regs)) ? ((_regs)->ints_mask & IC_PER_MASK) : 0 ) \
 | ( ((_regs)->psw.sysmask & PSW_EXTMASK) ? (IC_CR0_TO_INTMASK((_regs))) : 0 ) \
 | ( WAITSTATE(&(_regs)->psw) ? BIT(IC_PSW_WAIT) : 0 ) \
 )

/* Note: if PER mode, invalidate the AIA to force instfetch to be called */
#define SET_IC_ECMODE_MASK(_regs) \
 do { \
   (_regs)->ints_mask = IC_ECMODE_MASK((_regs)); \
   if ( ( (_regs)->permode = PER_MODE((_regs)) ) ) \
    INVALIDATE_AIA((_regs)); \
 } while (0)

#define SET_IC_BCMODE_MASK(_regs) \
 do { \
   (_regs)->ints_mask = IC_BCMODE_MASK((_regs)); \
   if ( ( (_regs)->permode = PER_MODE((_regs)) ) ) \
    INVALIDATE_AIA((_regs)); \
 } while (0)

#undef SET_IC_MASK
#ifdef FEATURE_BCMODE
 #define SET_IC_MASK(_regs) \
  do { \
    (_regs)->ints_mask = IC_MASK((_regs)); \
    if ( ( (_regs)->permode = PER_MODE((_regs)) ) ) \
     INVALIDATE_AIA((_regs)); \
  } while (0)
#else
 #define SET_IC_MASK(_regs) SET_IC_ECMODE_MASK(_regs)
#endif

/*
 * State bits indicate what interrupts are possibly pending
 * for a CPU.  These bits can be set by any thread and therefore
 * are serialized by the `intlock'.
 * For PER, the state bits are set when CR9 is loaded and the mask
 * bits are set when a PER event occurs
 */

#define SET_IC_TRACE \
 do { \
   int i; \
   CPU_BITMAP mask = sysblk.started_mask; \
   for (i = 0; mask; i++) { \
     if (mask & 1) \
       sysblk.regs[i]->ints_state |= BIT(IC_INTERRUPT); \
     mask >>= 1; \
   } \
 } while (0)

/*  "SET_IC_PER": whenever LCTL/LCTLG instruction is executed that
    sets a new CR9 (PER control register) value, the following macro
    is issued which sets the new ints_state value indicating which
    PER events have been enabled and resets ints_mask appropriately.
*/
#define SET_IC_PER(_regs) \
 do { \
  (_regs)->ints_state &= (~IC_PER_MASK); \
  (_regs)->ints_state |= (((_regs)->CR(9) >> IC_CR9_SHIFT) & IC_PER_MASK); \
  (_regs)->ints_mask  &= (~IC_PER_MASK | (_regs)->ints_state); \
 } while (0)

  /* * * * * * * * * * * * * *
   * Set state bit to '1'    *
   * * * * * * * * * * * * * */

#define ON_IC_INTERRUPT(_regs) \
 do { \
   (_regs)->ints_state |=  BIT(IC_INTERRUPT); \
 } while (0)

#define ON_IC_RESTART(_regs) \
 do { \
   (_regs)->ints_state |= BIT(IC_INTERRUPT) | BIT(IC_RESTART); \
 } while (0)

#define ON_IC_STORSTAT(_regs) \
 do { \
   (_regs)->ints_state |= BIT(IC_INTERRUPT) | BIT(IC_STORSTAT); \
 } while (0)

#define ON_IC_IOPENDING \
 do { \
   REGS *regs; \
   CPU_BITMAP mask = sysblk.started_mask; \
   CPU_BITMAP wake; \
   int i; \
   if ( !(sysblk.ints_state & BIT(IC_IO)) ) \
   { \
     sysblk.ints_state |= BIT(IC_IO); \
     wake = mask; \
     for (i = 0; mask; mask >>= 1, ++i) \
     { \
       if (mask & 1) \
       { \
         regs = sysblk.regs[i]; \
         if ( regs->ints_mask & BIT(IC_IO) ) \
           regs->ints_state |= BIT(IC_INTERRUPT) | BIT(IC_IO); \
         else \
           regs->ints_state |= BIT(IC_IO), \
           wake ^= regs->cpubit; \
       } \
     } \
     WAKEUP_CPU_MASK(wake); \
   } \
 } while (0)

#define ON_IC_CHANRPT \
 do { \
   int i; CPU_BITMAP mask; \
   if ( !(sysblk.ints_state & BIT(IC_CHANRPT)) ) { \
     sysblk.ints_state |=  BIT(IC_CHANRPT); \
     mask = sysblk.started_mask; \
     for (i = 0; mask; i++) { \
       if (mask & 1) { \
         if ( sysblk.regs[i]->ints_mask & BIT(IC_CHANRPT) ) \
           sysblk.regs[i]->ints_state |= BIT(IC_INTERRUPT) | BIT(IC_CHANRPT); \
         else \
           sysblk.regs[i]->ints_state |= BIT(IC_CHANRPT); \
       } \
       mask >>= 1; \
     } \
   } \
 } while (0)

#define ON_IC_INTKEY \
 do { \
   int i; CPU_BITMAP mask; \
   if ( !(sysblk.ints_state & BIT(IC_INTKEY)) ) { \
     sysblk.ints_state |= BIT(IC_INTKEY); \
     mask = sysblk.started_mask; \
     for (i = 0; mask; i++) { \
       if (mask & 1) { \
         if ( sysblk.regs[i]->ints_mask & BIT(IC_INTKEY) ) \
           sysblk.regs[i]->ints_state |= BIT(IC_INTERRUPT) | BIT(IC_INTKEY); \
         else \
           sysblk.regs[i]->ints_state |=  BIT(IC_INTKEY); \
       } \
       mask >>= 1; \
     } \
   } \
 } while (0)

#define ON_IC_SERVSIG \
 do { \
   int i; CPU_BITMAP mask; \
   if ( !(sysblk.ints_state & BIT(IC_SERVSIG)) ) { \
     sysblk.ints_state |= BIT(IC_SERVSIG); \
     mask = sysblk.started_mask; \
     for (i = 0; mask; i++) { \
       if (mask & 1) { \
         if ( sysblk.regs[i]->ints_mask & BIT(IC_SERVSIG) ) \
           sysblk.regs[i]->ints_state |= BIT(IC_INTERRUPT) | BIT(IC_SERVSIG); \
         else \
           sysblk.regs[i]->ints_state |= BIT(IC_SERVSIG); \
         } \
       mask >>= 1; \
     } \
   } \
 } while (0)

#define ON_IC_ITIMER(_regs) \
 do { \
   if ( (_regs)->ints_mask & BIT(IC_ITIMER) ) \
     (_regs)->ints_state |= BIT(IC_INTERRUPT) | BIT(IC_ITIMER); \
   else \
     (_regs)->ints_state |= BIT(IC_ITIMER); \
 } while (0)

#define ON_IC_PTIMER(_regs) \
 do { \
   if ( (_regs)->ints_mask & BIT(IC_PTIMER) ) \
     (_regs)->ints_state |= BIT(IC_INTERRUPT) | BIT(IC_PTIMER); \
   else \
     (_regs)->ints_state |= BIT(IC_PTIMER); \
 } while (0)

#define ON_IC_ECPSVTIMER(_regs) \
 do { \
   if ( (_regs)->ints_mask & BIT(IC_ECPSVTIMER) ) \
     (_regs)->ints_state |= BIT(IC_INTERRUPT) | BIT(IC_ECPSVTIMER); \
   else \
     (_regs)->ints_state |= BIT(IC_ECPSVTIMER); \
 } while (0)

#define ON_IC_CLKC(_regs) \
 do { \
   if ( (_regs)->ints_mask & BIT(IC_CLKC) ) \
     (_regs)->ints_state |= BIT(IC_INTERRUPT) | BIT(IC_CLKC); \
   else \
     (_regs)->ints_state |= BIT(IC_CLKC); \
 } while (0)

#define ON_IC_EXTCALL(_regs) \
 do { \
   if ( (_regs)->ints_mask & BIT(IC_EXTCALL) ) \
     (_regs)->ints_state |= BIT(IC_INTERRUPT) | BIT(IC_EXTCALL); \
   else \
     (_regs)->ints_state |= BIT(IC_EXTCALL); \
 } while (0)

#define ON_IC_MALFALT(_regs) \
 do { \
   if ( (_regs)->ints_mask & BIT(IC_MALFALT) ) \
     (_regs)->ints_state |= BIT(IC_INTERRUPT) | BIT(IC_MALFALT); \
   else \
     (_regs)->ints_state |= BIT(IC_MALFALT); \
 } while (0)

#define ON_IC_EMERSIG(_regs) \
 do { \
   if ( (_regs)->ints_mask & BIT(IC_EMERSIG) ) \
     (_regs)->ints_state |= BIT(IC_INTERRUPT) | BIT(IC_EMERSIG); \
   else \
     (_regs)->ints_state |= BIT(IC_EMERSIG); \
 } while (0)

    /*
     * When a PER event occurs we set the bit in ints_mask instead of
     * ints_state; therefore intlock does not need to be held.
     * The ints_state bits are set when CR9 is loaded.
     */

#define ON_IC_PER_SB(_regs) \
 do { \
   (_regs)->ints_mask |= BIT(IC_PER_SB); \
 } while (0)

#define ON_IC_PER_IF(_regs) \
 do { \
   (_regs)->ints_mask |= BIT(IC_PER_IF); \
 } while (0)

#define ON_IC_PER_SA(_regs) \
 do { \
     (_regs)->ints_mask |= BIT(IC_PER_SA); \
 } while (0)

#define ON_IC_PER_GRA(_regs) \
 do { \
     (_regs)->ints_mask |= BIT(IC_PER_GRA); \
 } while (0)

#define ON_IC_PER_SKEY(_regs) \
 do { \
     (_regs)->ints_mask |= BIT(IC_PER_SKEY); \
 } while (0)

#define ON_IC_PER_STURA(_regs) \
 do { \
     (_regs)->ints_mask |= BIT(IC_PER_STURA); \
 } while (0)

#define ON_IC_PER_ZEROADDR(_regs) \
 do { \
     (_regs)->ints_mask |= BIT(IC_PER_ZEROADDR); \
 } while (0)

#define ON_IC_PER_TEND(_regs) \
 do { \
     (_regs)->ints_mask |= BIT(IC_PER_TEND); \
 } while (0)

#define ON_IC_PER_IFNUL(_regs) \
 do { \
   (_regs)->ints_mask |= BIT(IC_PER_IFNUL); \
 } while (0)


  /* * * * * * * * * * * * * *
   * Set state bit to '0'    *
   * * * * * * * * * * * * * */

#define OFF_IC_INTERRUPT(_regs) \
 do { \
   (_regs)->ints_state &= ~BIT(IC_INTERRUPT); \
 } while (0)

#define OFF_IC_RESTART(_regs) \
 do { \
   (_regs)->ints_state &= ~BIT(IC_RESTART); \
 } while (0)

#define OFF_IC_STORSTAT(_regs) \
 do { \
   (_regs)->ints_state &= ~BIT(IC_STORSTAT); \
 } while (0)

#define OFF_IC_IOPENDING \
 do { \
   int i; CPU_BITMAP mask; \
   if ( sysblk.ints_state & BIT(IC_IO) ) { \
     sysblk.ints_state &= ~BIT(IC_IO); \
     mask = sysblk.started_mask; \
     for (i = 0; mask; i++) { \
       if (mask & 1) \
         sysblk.regs[i]->ints_state &= ~BIT(IC_IO); \
       mask >>= 1; \
     } \
   } \
 } while (0)

#define OFF_IC_CHANRPT \
 do { \
   int i; CPU_BITMAP mask; \
   if ( sysblk.ints_state & BIT(IC_CHANRPT) ) { \
     sysblk.ints_state &= ~BIT(IC_CHANRPT); \
     mask = sysblk.started_mask; \
     for (i = 0; mask; i++) { \
       if (mask & 1) \
         sysblk.regs[i]->ints_state &= ~BIT(IC_CHANRPT); \
       mask >>= 1; \
     } \
   } \
 } while (0)

#define OFF_IC_INTKEY \
 do { \
   int i; CPU_BITMAP mask; \
   if ( sysblk.ints_state & BIT(IC_INTKEY) ) { \
     sysblk.ints_state &= ~BIT(IC_INTKEY); \
     mask = sysblk.started_mask; \
     for (i = 0; mask; i++) { \
       if (mask & 1) \
         sysblk.regs[i]->ints_state &= ~BIT(IC_INTKEY); \
       mask >>= 1; \
     } \
   } \
 } while (0)

#define OFF_IC_SERVSIG \
 do { \
   int i; CPU_BITMAP mask; \
   if ( sysblk.ints_state & BIT(IC_SERVSIG) ) { \
     sysblk.ints_state &= ~BIT(IC_SERVSIG); \
     mask = sysblk.started_mask; \
     for (i = 0; mask; i++) { \
       if (mask & 1) \
         sysblk.regs[i]->ints_state &= ~BIT(IC_SERVSIG); \
       mask >>= 1; \
     } \
   } \
 } while (0)

#define OFF_IC_ITIMER(_regs) \
 do { \
   (_regs)->ints_state &= ~BIT(IC_ITIMER); \
 } while (0)

#define OFF_IC_PTIMER(_regs) \
 do { \
   (_regs)->ints_state &= ~BIT(IC_PTIMER); \
 } while (0)

#define OFF_IC_ECPSVTIMER(_regs) \
 do { \
   (_regs)->ints_state &= ~BIT(IC_ECPSVTIMER); \
 } while (0)

#define OFF_IC_CLKC(_regs) \
 do { \
   (_regs)->ints_state &= ~BIT(IC_CLKC); \
 } while (0)

#define OFF_IC_EXTCALL(_regs) \
 do { \
   (_regs)->ints_state &= ~BIT(IC_EXTCALL); \
 } while (0)

#define OFF_IC_MALFALT(_regs) \
 do { \
   (_regs)->ints_state &= ~BIT(IC_MALFALT); \
 } while (0)

#define OFF_IC_EMERSIG(_regs) \
 do { \
   (_regs)->ints_state &= ~BIT(IC_EMERSIG); \
 } while (0)

#define OFF_IC_PER(_regs) \
 do { \
   (_regs)->ints_mask &= ~IC_PER_MASK; \
 } while (0)

#define OFF_IC_PER_SB(_regs) \
 do { \
   (_regs)->ints_mask &= ~BIT(IC_PER_SB); \
 } while (0)

#define OFF_IC_PER_IF(_regs) \
 do { \
   (_regs)->ints_mask &= ~BIT(IC_PER_IF); \
 } while (0)

#define OFF_IC_PER_SA(_regs) \
 do { \
   (_regs)->ints_mask &= ~BIT(IC_PER_SA); \
 } while (0)

#define OFF_IC_PER_GRA(_regs) \
 do { \
   (_regs)->ints_mask &= ~BIT(IC_PER_GRA); \
 } while (0)

#define OFF_IC_PER_SKEY(_regs) \
 do { \
   (_regs)->ints_mask &= ~BIT(IC_PER_SKEY); \
 } while (0)

#define OFF_IC_PER_STURA(_regs) \
 do { \
   (_regs)->ints_mask &= ~BIT(IC_PER_STURA); \
 } while (0)

#define OFF_IC_PER_ZEROADDR(_regs) \
 do { \
   (_regs)->ints_mask &= ~BIT(IC_PER_ZEROADDR); \
 } while (0)

#define OFF_IC_PER_TEND(_regs) \
 do { \
   (_regs)->ints_mask &= ~BIT(IC_PER_TEND); \
 } while (0)

#define OFF_IC_PER_IFNUL(_regs) \
 do { \
   (_regs)->ints_mask &= ~BIT(IC_PER_IFNUL); \
 } while (0)


  /* * * * * * * * * * * * * *
   * Test interrupt state    *
   * * * * * * * * * * * * * */

#define IS_IC_INTERRUPT(_regs)     ( (_regs)->ints_state & BIT(IC_INTERRUPT) )
#define IS_IC_RESTART(_regs)       ( (_regs)->ints_state & BIT(IC_RESTART)   )
#define IS_IC_STORSTAT(_regs)      ( (_regs)->ints_state & BIT(IC_STORSTAT)  )
#define IS_IC_IOPENDING            ( sysblk.ints_state   & BIT(IC_IO)        )
#define IS_IC_MCKPENDING(_regs)    ( (_regs)->ints_state &     IC_MCKPENDING )
#define IS_IC_CHANRPT              ( sysblk.ints_state   & BIT(IC_CHANRPT)   )
#define IS_IC_INTKEY               ( sysblk.ints_state   & BIT(IC_INTKEY)    )
#define IS_IC_SERVSIG              ( sysblk.ints_state   & BIT(IC_SERVSIG)   )
#define IS_IC_ITIMER(_regs)        ( (_regs)->ints_state & BIT(IC_ITIMER)    )
#define IS_IC_PTIMER(_regs)        ( (_regs)->ints_state & BIT(IC_PTIMER)    )
#define IS_IC_ECPSVTIMER(_regs)    ( (_regs)->ints_state & BIT(IC_ECPSVTIMER))
#define IS_IC_CLKC(_regs)          ( (_regs)->ints_state & BIT(IC_CLKC)      )
#define IS_IC_EXTCALL(_regs)       ( (_regs)->ints_state & BIT(IC_EXTCALL)   )
#define IS_IC_MALFALT(_regs)       ( (_regs)->ints_state & BIT(IC_MALFALT)   )
#define IS_IC_EMERSIG(_regs)       ( (_regs)->ints_state & BIT(IC_EMERSIG)   )
#define IS_IC_PER(_regs)           ( (_regs)->ints_mask  &     IC_PER_MASK   )
#define IS_IC_PER_SB(_regs)        ( (_regs)->ints_mask  & BIT(IC_PER_SB)    )
#define IS_IC_PER_IF(_regs)        ( (_regs)->ints_mask  & BIT(IC_PER_IF)    )
#define IS_IC_PER_SA(_regs)        ( (_regs)->ints_mask  & BIT(IC_PER_SA)    )
#define IS_IC_PER_GRA(_regs)       ( (_regs)->ints_mask  & BIT(IC_PER_GRA)   )
#define IS_IC_PER_SKEY(_regs)      ( (_regs)->ints_mask  & BIT(IC_PER_SKEY)  )
#define IS_IC_PER_STURA(_regs)     ( (_regs)->ints_mask  & BIT(IC_PER_STURA) )
#define IS_IC_PER_ZEROADDR(_regs)  ( (_regs)->ints_mask  & BIT(IC_PER_ZEROADDR) )
#define IS_IC_PER_TEND(_regs)      ( (_regs)->ints_mask  & BIT(IC_PER_TEND) )
#define IS_IC_PER_IFNUL(_regs)     ( (_regs)->ints_mask  & BIT(IC_PER_IFNUL) )


  /* * * * * * * * * * * * * *
   * Disabled wait check     *
   * * * * * * * * * * * * * */

#define IS_IC_DISABLED_WAIT_PSW(_regs) \
                                ( ((_regs)->ints_mask & IC_OPEN_MASK) == 0 )


  /* * * * * * * * * * * * * *
   * Test PER mask bits      *
   * * * * * * * * * * * * * */

#define EN_IC_PER(_regs)           unlikely( (_regs)->permode )
#define EN_IC_PER_SB(_regs)        ( EN_IC_PER(_regs) && ((_regs)->ints_state & BIT(IC_PER_SB))    )
#define EN_IC_PER_IF(_regs)        ( EN_IC_PER(_regs) && ((_regs)->ints_state & BIT(IC_PER_IF))    )
#define EN_IC_PER_SA(_regs)        ( EN_IC_PER(_regs) && ((_regs)->ints_state & BIT(IC_PER_SA))    )
#define EN_IC_PER_GRA(_regs)       ( EN_IC_PER(_regs) && ((_regs)->ints_state & BIT(IC_PER_GRA))   )
#define EN_IC_PER_SKEY(_regs)      ( EN_IC_PER(_regs) && ((_regs)->ints_state & BIT(IC_PER_SKEY))  )
#define EN_IC_PER_STURA(_regs)     ( EN_IC_PER(_regs) && ((_regs)->ints_state & BIT(IC_PER_STURA)) )
#define EN_IC_PER_ZEROADDR(_regs)  ( EN_IC_PER(_regs) && ((_regs)->ints_state & BIT(IC_PER_ZEROADDR)) )
#define EN_IC_PER_TEND(_regs)      ( EN_IC_PER(_regs) && ((_regs)->ints_state & BIT(IC_PER_TEND)) )
#define EN_IC_PER_IFNUL(_regs)     ( EN_IC_PER(_regs) && ((_regs)->ints_state & BIT(IC_PER_IFNUL)) )


  /* * * * * * * * * * * * * * * * * * * * * * * * *
   * Check for specific enabled pending interrupt  *
   * * * * * * * * * * * * * * * * * * * * * * * * */

#define OPEN_IC_MCKPENDING(_regs) \
                        ( (_regs)->ints_state & (_regs)->ints_mask & IC_MCKPENDING )

#define OPEN_IC_IOPENDING(_regs) \
                        ( (_regs)->ints_state & (_regs)->ints_mask & IC_IOPENDING )

#define OPEN_IC_CHANRPT(_regs) \
                        ( (_regs)->ints_state & (_regs)->ints_mask & BIT(IC_CHANRPT) )

#define OPEN_IC_EXTPENDING(_regs) \
                        ( (_regs)->ints_state & (_regs)->ints_mask & IC_EXTPENDING )

#define OPEN_IC_ITIMER(_regs) \
                        ( (_regs)->ints_state & (_regs)->ints_mask & BIT(IC_ITIMER) )

#define OPEN_IC_PTIMER(_regs) \
                        ( (_regs)->ints_state & (_regs)->ints_mask & BIT(IC_PTIMER) )

#define OPEN_IC_ECPSVTIMER(_regs) \
                        ( (_regs)->ints_state & (_regs)->ints_mask & BIT(IC_ECPSVTIMER) )

#define OPEN_IC_CLKC(_regs) \
                        ( (_regs)->ints_state & (_regs)->ints_mask & BIT(IC_CLKC) )

#define OPEN_IC_INTKEY(_regs) \
                        ( (_regs)->ints_state & (_regs)->ints_mask & BIT(IC_INTKEY) )

#define OPEN_IC_SERVSIG(_regs) \
                        ( (_regs)->ints_state & (_regs)->ints_mask & BIT(IC_SERVSIG) )

#define OPEN_IC_EXTCALL(_regs) \
                        ( (_regs)->ints_state & (_regs)->ints_mask & BIT(IC_EXTCALL) )

#define OPEN_IC_MALFALT(_regs) \
                        ( (_regs)->ints_state & (_regs)->ints_mask & BIT(IC_MALFALT) )

#define OPEN_IC_EMERSIG(_regs) \
                        ( (_regs)->ints_state & (_regs)->ints_mask & BIT(IC_EMERSIG) )

#define OPEN_IC_PER(_regs) \
                        ( (_regs)->ints_state & (_regs)->ints_mask & IC_PER_MASK )
#define OPEN_IC_PER_SB(_regs) \
                        ( (_regs)->ints_state & (_regs)->ints_mask & BIT(IC_PER_SB) )
#define OPEN_IC_PER_IF(_regs) \
                        ( (_regs)->ints_state & (_regs)->ints_mask & BIT(IC_PER_IF) )
#define OPEN_IC_PER_SA(_regs) \
                        ( (_regs)->ints_state & (_regs)->ints_mask & BIT(IC_PER_SA) )
#define OPEN_IC_PER_GRA(_regs) \
                        ( (_regs)->ints_state & (_regs)->ints_mask & BIT(IC_PER_GRA) )
#define OPEN_IC_PER_SKEY(_regs) \
                        ( (_regs)->ints_state & (_regs)->ints_mask & BIT(IC_PER_SKEY) )
#define OPEN_IC_PER_STURA(_regs) \
                        ( (_regs)->ints_state & (_regs)->ints_mask & BIT(IC_PER_STURA) )
#define OPEN_IC_PER_ZEROADDR(_regs) \
                        ( (_regs)->ints_state & (_regs)->ints_mask & BIT(IC_PER_ZEROADDR) )
#define OPEN_IC_PER_TEND(_regs) \
                        ( (_regs)->ints_state & (_regs)->ints_mask & BIT(IC_PER_TEND) )
#define OPEN_IC_PER_IFNUL(_regs) \
                        ( (_regs)->ints_state & (_regs)->ints_mask & BIT(IC_PER_IFNUL) )

  /* * * * * * * * * * * * * * * * * * * * * * * * *
   * Check for general enabled pending interrupt   *
   * * * * * * * * * * * * * * * * * * * * * * * * */

#define IC_INTERRUPT_CPU(_regs) \
   ( (_regs)->ints_state & (_regs)->ints_mask )

#define INTERRUPT_PENDING(_regs) IC_INTERRUPT_CPU((_regs))

#define SIE_IC_INTERRUPT_CPU(_regs) \
   (((_regs)->ints_state|(HOST(_regs)->ints_state&IC_SIE_INT)) & (_regs)->ints_mask)

#define SIE_INTERRUPT_PENDING(_regs) SIE_IC_INTERRUPT_CPU((_regs))


   /* * * * * * * * * * * * * * * * * * * * * * * * *
    *            PER macros summary                 *
    * * * * * * * * * * * * * * * * * * * * * * * * *

  SET_IC_PER        indicates which PER events are wanted

  ON_IC_PER_xxx     indicates a PER event has happened
  OFF_IC_PER_xxx    resets PER event to has NOT happened

  EN_IC_PER_xxx     tests whether PER event is wanted
  IS_IC_PER_xxx     tests whether PER event has happened

  OPEN_IC_PER_xxx   tests whether PER interrupt is pending
                    (i.e. whether interrupt SHOULD occur)
*/
