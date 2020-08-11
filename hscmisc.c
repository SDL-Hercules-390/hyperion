/* HSCMISC.C    (C) Copyright Roger Bowler, 1999-2012                */
/*              (C) Copyright Jan Jaeger, 1999-2012                  */
/*              Miscellaneous System Command Routines                */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#include "hstdinc.h"

#define _HSCMISC_C_
#define _HENGINE_DLL_

#include "hercules.h"
#include "devtype.h"
#include "opcode.h"
#include "inline.h"
#include "hconsole.h"
#include "esa390io.h"
#include "hexdumpe.h"

/*-------------------------------------------------------------------*/
/*   ARCH_DEP section: compiled multiple times, once for each arch.  */
/*-------------------------------------------------------------------*/

#ifndef COMPILE_THIS_ONLY_ONCE
#define COMPILE_THIS_ONLY_ONCE

//-------------------------------------------------------------------
//                     Helper macro
//-------------------------------------------------------------------

#define LIMIT_RANGE( _start, _end, _limit )                           \
    do {                                                              \
        if ((_end) > (_limit) && ((_end) - (_limit) + 1) > (_start))  \
            (_end) = ((_start) + (_limit) - 1);                       \
    } while(0)

#define RANGE_LIMIT  _64_KILOBYTE  // (default limit for LIMIT_RANGE)

//-------------------------------------------------------------------
//         (static helper function forward references)
//-------------------------------------------------------------------

static int  display_inst_regs ( REGS* regs, BYTE* inst, BYTE opcode, char* buf, int buflen );
static int  parse_range       ( char* operand, U64 maxadr, U64* sadrp, U64* eadrp, BYTE* newval );

#endif /* COMPILE_THIS_ONLY_ONCE */

//-------------------------------------------------------------------
//                      ARCH_DEP() code
//-------------------------------------------------------------------
// ARCH_DEP (build-architecture / FEATURE-dependent) functions here.
// All BUILD architecture dependent (ARCH_DEP) function are compiled
// multiple times (once for each defined build architecture) and each
// time they are compiled with a different set of FEATURE_XXX defines
// appropriate for that architecture. Use #ifdef FEATURE_XXX guards
// to check whether the current BUILD architecture has that given
// feature #defined for it or not. WARNING: Do NOT use _FEATURE_XXX.
// The underscore feature #defines mean something else entirely. Only
// test for FEATURE_XXX. (WITHOUT the underscore)
//-------------------------------------------------------------------

/*-------------------------------------------------------------------*/
/*                       virt_to_real                                */
/*-------------------------------------------------------------------*/
/* Convert virtual address to real address                           */
/*                                                                   */
/* Input:                                                            */
/*      vaddr   Virtual address to be translated                     */
/*      arn     Access register number                               */
/*      regs    CPU register context                                 */
/*      acctype Type of access (ACCTYPE_INSTFETCH, ACCTYPE_READ,     */
/*              ACCTYPE_WRITE or ACCTYPE_LRA)                        */
/* Output:                                                           */
/*      raptr   Points to word in which real address is returned     */
/*      siptr   Points to word to receive indication of which        */
/*              STD or ASCE was used to perform the translation      */
/* Return value:                                                     */
/*      0 = translation successful, non-zero = exception code        */
/*                                                                   */
/* Note:                                                             */
/*      To avoid unwanted alteration of the CPU register context     */
/*      during translation (e.g. regs->dat fields are updated and    */
/*      the TEA is updated too if a translation exception occurs),   */
/*      the translation is performed using a temporary copy of the   */
/*      CPU registers. While inefficient, this is a utility function */
/*      not meant to be used by executing CPUs. It is only designed  */
/*      to be called by other utility functions like 'display_virt'  */
/*      (v_vmd), 'alter_display_virt' (v_cmd), 'disasm_stor' (u_cmd) */
/*      and 'display_inst'.                                          */
/*                                                                   */
/*      PLEASE NOTE HOWEVER, that since "logical_to_main" IS called, */
/*      the storage key reference and change bits ARE updated when   */
/*      the translation is successful.                               */
/*                                                                   */
/*-------------------------------------------------------------------*/
int ARCH_DEP( virt_to_real )( U64* raptr, int* siptr, U64 vaddr,
                              int arn, REGS* iregs, int acctype )
{
    int icode;
    REGS* regs = copy_regs( iregs );    /* (temporary working copy) */

    if (!(icode = setjmp( regs->progjmp )))
    {
        int temp_arn = arn;     /* (bypass longjmp clobber warning) */

        if (acctype == ACCTYPE_INSTFETCH)
            temp_arn = USE_INST_SPACE;

        if (SIE_MODE( regs ))
            memcpy( HOSTREGS->progjmp, regs->progjmp, sizeof( jmp_buf ));

        ARCH_DEP( logical_to_main )( (VADR)vaddr, temp_arn, regs, acctype, 0 );
    }

    *siptr = regs->dat.stid;
    *raptr = (U64) HOSTREGS->dat.raddr;

    free_aligned( regs );   /* (discard temporary REGS working copy) */

    return icode;

} /* end function virt_to_real */


/*-------------------------------------------------------------------*/
/* Display real storage (up to 16 bytes, or until end of page)       */
/* Prefixes display by Rxxxxx: if draflag is 1                       */
/* Returns number of characters placed in display buffer             */
/*-------------------------------------------------------------------*/
static int ARCH_DEP(display_real) (REGS *regs, RADR raddr, char *buf, size_t bufl,
                                    int draflag, char *hdr)
{
RADR    aaddr;                          /* Absolute storage address  */
int     i, j;                           /* Loop counters             */
int     n = 0;                          /* Number of bytes in buffer */
char    hbuf[64];                       /* Hexadecimal buffer        */
BYTE    cbuf[17];                       /* Character buffer          */
BYTE    c;                              /* Character work area       */

#if defined(FEATURE_INTERVAL_TIMER)
    if(ITIMER_ACCESS(raddr,16))
        ARCH_DEP(store_int_timer)(regs);
#endif

    n = snprintf(buf, bufl, "%s", hdr);
    if (draflag)
    {
        n += snprintf (buf+n, bufl-n, "R:"F_RADR":", raddr);
    }

    aaddr = APPLY_PREFIXING (raddr, regs->PX);
    if (SIE_MODE(regs))
    {
        if (HOSTREGS->mainlim == 0 || aaddr > HOSTREGS->mainlim)
        {
            n += snprintf (buf+n, bufl-n,
                "A:"F_RADR" Guest real address is not valid", aaddr);
            return n;
        }
        else
        {
            n += snprintf (buf+n, bufl-n, "A:"F_RADR":", aaddr);
        }
    }
    else
    if (regs->mainlim == 0 || aaddr > regs->mainlim)
    {
        n += snprintf (buf+n, bufl-n, "%s", " Real address is not valid");
        return n;
    }

    n += snprintf (buf+n, bufl-n, "K:%2.2X=", STORAGE_KEY(aaddr, regs));

    memset (hbuf, SPACE, sizeof(hbuf));
    memset (cbuf, SPACE, sizeof(cbuf));

    for (i = 0, j = 0; i < 16; i++)
    {
        c = regs->mainstor[aaddr++];
        j += snprintf (hbuf+j, sizeof(hbuf)-j, "%2.2X", c);
        if ((aaddr & 0x3) == 0x0) hbuf[j++] = SPACE;
        c = guest_to_host(c);
        if (!isprint(c)) c = '.';
        cbuf[i] = c;
        if ((aaddr & PAGEFRAME_BYTEMASK) == 0x000) break;
    } /* end for(i) */

    n += snprintf (buf+n, bufl-n, "%36.36s %16.16s", hbuf, cbuf);
    return n;

} /* end function display_real */


/*-------------------------------------------------------------------*/
/* Display virtual storage (up to 16 bytes, or until end of page)    */
/* Returns number of characters placed in display buffer             */
/*-------------------------------------------------------------------*/
static int ARCH_DEP(display_virt) (REGS *regs, VADR vaddr, char *buf, size_t bufl,
                                    int ar, int acctype, char *hdr, U16* xcode)
{
RADR    raddr;                          /* Real address              */
int     n;                              /* Number of bytes in buffer */
int     stid;                           /* Segment table indication  */

    n = snprintf (buf, bufl, "%s%c:"F_VADR":", hdr,
                 ar == USE_REAL_ADDR ? 'R' : 'V', vaddr);
    *xcode = ARCH_DEP(virt_to_real) (&raddr, &stid,
                                     vaddr, ar, regs, acctype);
    if (*xcode == 0)
        n += ARCH_DEP(display_real) (regs, raddr, buf+n, bufl-n, 0, "");
    else
        n += snprintf( buf+n, bufl-n,
                       " Translation exception %4.4hX (%s)",
                       *xcode, PIC2Name( *xcode ));
    return n;

} /* end function display_virt */


/*-------------------------------------------------------------------*/
/*               Hexdump absolute storage page                       */
/*-------------------------------------------------------------------*/
/*                                                                   */
/*   regs     CPU register context                                   */
/*   aaddr    Absolute address of start of page to be dumped         */
/*   adr      Cosmetic address of start of page                      */
/*   offset   Offset from start of page where to begin dumping       */
/*   amt      Number of bytes to dump                                */
/*   vra      0 = alter_display_virt; 'R' real; 'A' absolute         */
/*   wid      Width of addresses in bits (32 or 64)                  */
/*                                                                   */
/* Message number HHC02290 used if vra != 0, otherwise HHC02291.     */
/* aaddr must be page aligned. offset must be < pagesize. amt must   */
/* be <= pagesize - offset. Results printed directly via WRMSG.      */
/* Returns 0 on success, otherwise -1 = error.                       */
/*-------------------------------------------------------------------*/
static int ARCH_DEP( dump_abs_page )( REGS *regs, RADR aaddr, RADR adr,
                                       size_t offset, size_t amt,
                                       char vra, BYTE wid )
{
    char*   msgnum;                 /* "HHC02290" or "HHC02291"      */
    char*   dumpdata;               /* pointer to data to be dumped  */
    char*   dumpbuf = NULL;         /* pointer to hexdump buffer     */
    char    pfx[64];                /* string prefixed to each line  */

    msgnum = vra ? "HHC02290" : "HHC02291";

    if (0
        || aaddr  &  PAGEFRAME_BYTEMASK     /* not page aligned      */
        || adr    &  PAGEFRAME_BYTEMASK     /* not page aligned      */
        || offset >= PAGEFRAME_PAGESIZE     /* offset >= pagesize    */
        || amt    > (PAGEFRAME_PAGESIZE - offset)/* more than 1 page */
        || (wid != 32 && wid != 64)         /* invalid address width */
    )
    {
        // "Error in function %s: %s"
        WRMSG( HHC02219, "E", "dump_abs_page()", "invalid parameters" );
        return -1;
    }

    /* Flush interval timer value to storage */
    ITIMER_SYNC( adr + offset, amt, regs );

    /* Check for addressing exception */
    if (aaddr > regs->mainlim)
    {
        MSGBUF( pfx, "%c:"F_RADR"  Addressing exception",
            vra ? vra : 'V', adr );
        if (vra)
            WRMSG( HHC02290, "E", pfx );
        else
            WRMSG( HHC02291, "E", pfx );
        return -1;
    }

    /* Format string each dump line should be prefixed with */
    MSGBUF( pfx, "%sI %c:", msgnum, vra ? vra : 'V' );

    /* Point to first byte of actual storage to be dumped */
    dumpdata = (char*) regs->mainstor + aaddr + offset;

    /* Adjust cosmetic starting address of first line of dump */
    adr += offset;                  /* exact cosmetic start address  */
    adr &= ~0xF;                    /* align to 16-byte boundary     */
    offset &= 0xF;                  /* offset must be < (bpg * gpl)  */

    /* Use hexdump to format 16-byte aligned absolute storage dump   */

    hexdumpew                       /* afterwards dumpbuf --> dump   */
    (
        pfx,                        /* string prefixed to each line  */
        &dumpbuf,                   /* ptr to hexdump buffer pointer */
                                    /* (if NULL hexdump will malloc) */
        dumpdata,                   /* pointer to data to be dumped  */
        offset,                     /* bytes to skip on first line   */
        amt,                        /* amount of data to be dumped   */
        adr,                        /* cosmetic dump address of data */
        wid,                        /* width of dump address in bits */
        4,                          /* bpg value (bytes per group)   */
        4                           /* gpl value (groups per line)   */
    );

    /* Check for internal hexdumpew error */
    if (!dumpbuf)
    {
        // "Error in function %s: %s"
        WRMSG( HHC02219, "E", "dump_abs_page()", "hexdumpew failed" );
        return -1;
    }

    /* Display the dump and free the buffer hexdump malloc'ed for us */

    /* Note: due to WRMSG requirements for multi-line messages, the
       first line should not have a message number. Thus we skip past
       it via +1 for "I" in message number +1 for blank following it.
       We also remove the last newline since WRMSG does that for us. */

    *(dumpbuf + strlen( dumpbuf ) - 1) = 0; /* (remove last newline) */

    if (vra)
        WRMSG( HHC02290, "I", dumpbuf + strlen( msgnum ) + 1 + 1 );
    else
        WRMSG( HHC02291, "I", dumpbuf + strlen( msgnum ) + 1 + 1 );

    free( dumpbuf );
    return 0;

} /* end function dump_abs_page */


/*-------------------------------------------------------------------*/
/* Disassemble real                                                  */
/*-------------------------------------------------------------------*/
static void ARCH_DEP(disasm_stor) (REGS *regs, int argc, char *argv[], char *cmdline)
{
char*   opnd;                           /* Range/alteration operand  */
U64     saddr, eaddr;                   /* Range start/end addresses */
U64     maxadr;                         /* Highest real storage addr */
RADR    raddr;                          /* Real storage address      */
RADR    aaddr;                          /* Absolute storage address  */
int     stid = -1;                      /* How translation was done  */
int     len;                            /* Number of bytes to alter  */
int     ilc;                            /* Instruction length counter*/
BYTE    inst[6];                        /* Storage alteration value  */
BYTE    opcode;                         /* Instruction opcode        */
U16     xcode;                          /* Exception code            */
char    type;                           /* Address space type        */
char    buf[512];                       /* MSGBUF work buffer        */

    UNREFERENCED(cmdline);

    /* We require only one operand */
    if (argc != 1)
    {
        // "Missing or invalid argument(s)"
        WRMSG( HHC17000, "E" );
        return;
    }

    /* Parse optional address-space prefix */
    opnd = argv[0];
    type = toupper( *opnd );

    if (0
        || type == 'R'
        || type == 'V'
        || type == 'P'
        || type == 'H'
    )
        opnd++;
    else
        type = REAL_MODE( &regs->psw ) ? 'R' : 'V';

    /* Set limit for address range */
  #if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
    maxadr = 0xFFFFFFFFFFFFFFFFULL;
  #else /*!defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/
    maxadr = 0x7FFFFFFF;
  #endif /*!defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/

    /* Parse the range or alteration operand */
    len = parse_range (opnd, maxadr, &saddr, &eaddr, NULL);
    if (len < 0) return;

    if (regs->mainlim == 0)
    {
        WRMSG(HHC02289, "I", "Real address is not valid");
        return;
    }

    /* Limit the amount to be displayed to a reasonable value */
    LIMIT_RANGE( saddr, eaddr, RANGE_LIMIT );

    /* Display real storage */
    while (saddr <= eaddr)
    {
        if(type == 'R')
            raddr = saddr;
        else
        {
            /* Convert virtual address to real address */
            if((xcode = ARCH_DEP(virt_to_real) (&raddr, &stid, saddr, 0, regs, ACCTYPE_INSTFETCH) ))
            {
                MSGBUF( buf, "R:"F_RADR"  Storage not accessible code = %4.4X (%s)",
                    saddr, xcode, PIC2Name( xcode ));
                WRMSG( HHC02289, "I", buf );
                return;
            }
        }

        /* Convert real address to absolute address */
        aaddr = APPLY_PREFIXING (raddr, regs->PX);
        if (aaddr > regs->mainlim)
        {
            MSGBUF( buf, "R:"F_RADR"  Addressing exception", raddr );
            WRMSG( HHC02289, "I", buf );
            return;
        }

        /* Determine opcode and check for addressing exception */
        opcode = regs->mainstor[aaddr];
        ilc = ILC(opcode);

        if (aaddr + ilc > regs->mainlim)
        {
            MSGBUF( buf, "R:"F_RADR"  Addressing exception", aaddr );
            WRMSG( HHC02289, "I", buf );
            return;
        }

        /* Copy instruction to work area and hex print it */
        memcpy(inst, regs->mainstor + aaddr, ilc);
        len = sprintf(buf, "%c:"F_RADR"  %2.2X%2.2X",
          stid == TEA_ST_PRIMARY ? 'P' :
          stid == TEA_ST_HOME ? 'H' :
          stid == TEA_ST_SECNDRY ? 'S' : 'R',
          raddr, inst[0], inst[1]);

        if(ilc > 2)
        {
            len += snprintf(buf + len, sizeof(buf)-len, "%2.2X%2.2X", inst[2], inst[3]);
            if(ilc > 4)
                len += snprintf(buf + len, sizeof(buf)-len, "%2.2X%2.2X ", inst[4], inst[5]);
            else
                len += snprintf(buf + len, sizeof(buf)-len, "     ");
        }
        else
            len += snprintf(buf + len, sizeof(buf)-len, "         ");

        /* Disassemble the instruction and display the results */
        PRINT_INST(inst, buf + len);
        WRMSG(HHC02289, "I", buf);

        /* Go on to the next instruction */
        saddr += ilc;

    } /* end while (saddr <= eaddr) */

} /* end function disasm_stor */


/*-------------------------------------------------------------------*/
/* Process alter or display real or absolute storage command         */
/*-------------------------------------------------------------------*/
static void ARCH_DEP(alter_display_real_or_abs) (REGS *regs, int argc, char *argv[], char *cmdline)
{
char*   opnd;                           /* range/alteration operand  */
U64     saddr, eaddr;                   /* Range start/end addresses */
U64     maxadr;                         /* Highest real storage addr */
RADR    raddr;                          /* Real storage address      */
RADR    aaddr;                          /* Absolute storage address  */
size_t  totamt;                         /* Total amount to be dumped */
int     len;                            /* Number of bytes to alter  */
int     i;                              /* Loop counter              */
BYTE    newval[32];                     /* Storage alteration value  */
char    buf[64];                        /* MSGBUF work buffer        */
char    absorr[8];                      /* Uppercase command         */

    UNREFERENCED(argc);
    UNREFERENCED(cmdline);

    /* We require only one operand */
    if (argc != 2)
    {
        // "Missing or invalid argument(s)"
        WRMSG( HHC17000, "E" );
        return;
    }

    /* Convert command to uppercase */
    for (i = 0; argv[0][i]; i++)
        absorr[i] = toupper(argv[0][i]);
    absorr[i] = 0;
    opnd = argv[1];

    /* Set limit for address range */
  #if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
    maxadr = 0xFFFFFFFFFFFFFFFFULL;
  #else /*!defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/
    maxadr = 0x7FFFFFFF;
  #endif /*!defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/

    /* Parse the range or alteration operand */
    len = parse_range (opnd, maxadr, &saddr, &eaddr, newval);
    if (len < 0) return;

    if (regs->mainlim == 0)
    {
        // "%c:"F_RADR"  Storage address is not valid"
        WRMSG( HHC02327, "E", absorr[0], saddr );
        return;
    }

    /* Alter real or absolute storage */
    if (len > 0)
    {
        for (i=0; i < len; i++)
        {
            /* Address of next byte */
            raddr = saddr + i;

            /* Convert real address to absolute address */
            if ('R' == absorr[0])
                aaddr = APPLY_PREFIXING (raddr, regs->PX);
            else
                aaddr = raddr; /* (is already absolute) */

            /* Check for addressing exception */
            if (aaddr > regs->mainlim)
            {
                // "%c:"F_RADR"  Addressing exception"
                WRMSG( HHC02328, "E", 'A', aaddr );
                return;
            }

            /* Update absolute storage */
            regs->mainstor[aaddr] = newval[i];
            STORAGE_KEY(aaddr, regs) |= (STORKEY_REF | STORKEY_CHANGE);
        } /* end for(i) */
    }

    /* Limit the amount to be displayed to a reasonable value */
    LIMIT_RANGE( saddr, eaddr, RANGE_LIMIT );

    /* Display real or absolute storage */
    if ((totamt = (eaddr - saddr) + 1) > 0)
    {
        RADR    pageadr  = saddr & PAGEFRAME_PAGEMASK;
        size_t  pageoff  = saddr - pageadr;
        size_t  pageamt  = PAGEFRAME_PAGESIZE - pageoff;
        BYTE    addrwid  = (ARCH_900_IDX == sysblk.arch_mode) ? 64: 32;

        /* Dump absolute storage one whole page at a time */

        for (;;)
        {
            /* Next page to be dumped */
            raddr = pageadr;

            /* Make sure we don't dump too much */
            if (pageamt > totamt)
                pageamt = totamt;

            /* Convert real address to absolute address */
            if ('R' == absorr[0])
                aaddr = APPLY_PREFIXING( raddr, regs->PX );
            else
                aaddr = raddr; /* (is already absolute) */

            /* Check for addressing exception */
            if (aaddr > regs->mainlim)
            {
                // "%c:"F_RADR"  Addressing exception"
                WRMSG( HHC02328, "E", 'A', aaddr );
                break;
            }

            /* Display storage key for this page */
            MSGBUF( buf, "A:"F_RADR"  K:%2.2X",
                aaddr, STORAGE_KEY( aaddr, regs ));
            WRMSG( HHC02290, "I", buf );

            /* Now hexdump that absolute page */
            VERIFY( ARCH_DEP( dump_abs_page )( regs, aaddr, raddr,
                pageoff, pageamt, absorr[0], addrwid ) == 0);

            /* Check if we're done */
            if (!(totamt -= pageamt))
                break;

            /* Go on to the next page */
            pageoff =  0; // (from now on)
            pageamt =  PAGEFRAME_PAGESIZE;
            pageadr += PAGEFRAME_PAGESIZE;
        }
    }

} /* end function alter_display_real_or_abs */


/*-------------------------------------------------------------------*/
/* HELPER for virtual storage alter or display command               */
/*-------------------------------------------------------------------*/
static void ARCH_DEP( bldtrans )(REGS *regs, int arn, int stid,
                                 char *trans, size_t size)
{
    /* Build string indicating how virtual address was translated    */

    char    buf[16];  /* Caller's buffer should be at least this big */

         if (REAL_MODE( &regs->psw )) MSGBUF( buf, "%s", "(dat off)"   );
    else if (stid == TEA_ST_PRIMARY)  MSGBUF( buf, "%s", "(primary)"   );
    else if (stid == TEA_ST_SECNDRY)  MSGBUF( buf, "%s", "(secondary)" );
    else if (stid == TEA_ST_HOME)     MSGBUF( buf, "%s", "(home)"      );
    else                              MSGBUF( buf, "(AR%2.2d)", arn    );

    strlcpy( trans, buf, size);
}


/*-------------------------------------------------------------------*/
/* Process virtual storage alter or display command                  */
/*-------------------------------------------------------------------*/
static void ARCH_DEP(alter_display_virt) (REGS *regs, int argc, char *argv[], char *cmdline)
{
char*   opnd;                           /* range/alteration operand  */
U64     saddr, eaddr;                   /* Range start/end addresses */
U64     maxadr;                         /* Highest virt storage addr */
VADR    vaddr;                          /* Virtual storage address   */
RADR    raddr;                          /* Real storage address      */
RADR    aaddr;                          /* Absolute storage address  */
int     stid;                           /* Segment table indication  */
int     len;                            /* Number of bytes to alter  */
int     i;                              /* Loop counter              */
int     arn = 0;                        /* Access register number    */
U16     xcode;                          /* Exception code            */
char    trans[16];                      /* Address translation mode  */
BYTE    newval[32];                     /* Storage alteration value  */
char    buf[96];                        /* Message buffer            */
char    type;                           /* optional addr-space type  */
size_t  totamt;                         /* Total amount to be dumped */

    UNREFERENCED(cmdline);

    /* We require only one operand */
    if (argc != 1)
    {
        // "Missing or invalid argument(s)"
        WRMSG( HHC17000, "E" );
        return;
    }

    /* Parse optional address-space prefix */
    opnd = argv[0];
    type = toupper( *opnd );

    if (1
        && type != 'P'
        && type != 'S'
        && type != 'H'
    )
        arn = 0;
    else
    {
        switch (type)
        {
            case 'P': arn = USE_PRIMARY_SPACE;   break;
            case 'S': arn = USE_SECONDARY_SPACE; break;
            case 'H': arn = USE_HOME_SPACE;      break;
        }
        opnd++;
    }

    /* Set limit for address range */
  #if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
    maxadr = 0xFFFFFFFFFFFFFFFFULL;
  #else /*!defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/
    maxadr = 0x7FFFFFFF;
  #endif /*!defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/

    /* Parse the range or alteration operand */
    len = parse_range (opnd, maxadr, &saddr, &eaddr, newval);
    if (len < 0) return;

    if (regs->mainlim == 0)
    {
        // "%c:"F_RADR"  Storage address is not valid"
        WRMSG( HHC02327, "E", 'V', saddr );
        return;
    }

    /* Alter virtual storage */
    if (len > 0
        && ARCH_DEP(virt_to_real) (&raddr, &stid, saddr, arn, regs, ACCTYPE_LRA) == 0
        && ARCH_DEP(virt_to_real) (&raddr, &stid, eaddr, arn, regs, ACCTYPE_LRA) == 0
    )
    {
        for (i=0; i < len; i++)
        {
            /* Address of next byte */
            vaddr = saddr + i;

            /* Convert virtual address to real address */
            xcode = ARCH_DEP(virt_to_real) (&raddr, &stid, vaddr,
                arn, regs, ACCTYPE_LRA);
            ARCH_DEP( bldtrans )(regs, arn, stid, trans, sizeof(trans));

            /* Check for Translation Exception */
            if (0 != xcode)
            {
                // "%c:"F_RADR"  Translation exception %4.4hX (%s)  %s"
                WRMSG( HHC02329, "E", 'V', vaddr, xcode, PIC2Name( xcode ),
                    trans );
                return;
            }

            /* Convert real address to absolute address */
            aaddr = APPLY_PREFIXING (raddr, regs->PX);

            /* Check for addressing exception */
            if (aaddr > regs->mainlim)
            {
                // "%c:"F_RADR"  Addressing exception"
                WRMSG( HHC02328, "E", 'R', raddr );
                return;
            }

            /* Update absolute storage */
            regs->mainstor[aaddr] = newval[i];
            STORAGE_KEY(aaddr, regs) |= (STORKEY_REF | STORKEY_CHANGE);
        }
    }

    /* Limit the amount to be displayed to a reasonable value */
    LIMIT_RANGE( saddr, eaddr, RANGE_LIMIT );

    /* Display virtual storage */
    if ((totamt = (eaddr - saddr) + 1) > 0)
    {
        RADR    pageadr  = saddr & PAGEFRAME_PAGEMASK;
        size_t  pageoff  = saddr - pageadr;
        size_t  pageamt  = PAGEFRAME_PAGESIZE - pageoff;
        BYTE    addrwid  = (ARCH_900_IDX == sysblk.arch_mode) ? 64: 32;

        /* Dump absolute storage one whole page at a time */

        for (;;)
        {
            /* Next page to be dumped */
            vaddr = pageadr;

            /* Make sure we don't dump too much */
            if (pageamt > totamt)
                pageamt = totamt;

            /* Convert virtual address to real address */
            xcode = ARCH_DEP( virt_to_real )( &raddr, &stid, vaddr,
                arn, regs, ACCTYPE_LRA );
            ARCH_DEP( bldtrans )(regs, arn, stid, trans, sizeof(trans));

            /* Check for Translation Exception */
            if (0 != xcode)
            {
                // "%c:"F_RADR"  Translation exception %4.4hX (%s)  %s"
                WRMSG( HHC02329, "E", 'V', vaddr, xcode, PIC2Name( xcode ),
                    trans );
            }
            else
            {
                /* Convert real address to absolute address */
                aaddr = APPLY_PREFIXING (raddr, regs->PX);

                /* Check for addressing exception */
                if (aaddr > regs->mainlim)
                {
                    // "%c:"F_RADR"  Addressing exception"
                    WRMSG( HHC02328, "E", 'R', raddr );
                    break;  /* (no sense in continuing) */
                }

                /* Display storage key for page and how translated */
                MSGBUF( buf, "R:"F_RADR"  K:%2.2X  %s",
                    raddr, STORAGE_KEY( aaddr, regs ), trans );
                WRMSG( HHC02291, "I", buf );

                /* Now hexdump that absolute page */
                VERIFY( ARCH_DEP( dump_abs_page )( regs, aaddr, vaddr,
                    pageoff, pageamt, 0, addrwid ) == 0);
            }

            /* Check if we're done */
            if (!(totamt -= pageamt))
                break;

            /* Go on to the next page */
            pageoff =  0; // (from now on)
            pageamt =  PAGEFRAME_PAGESIZE;
            pageadr += PAGEFRAME_PAGESIZE;
        }
    }

} /* end function alter_display_virt */


/*-------------------------------------------------------------------*/
/*                    display_inst_adj                               */
/*-------------------------------------------------------------------*/
static void ARCH_DEP( display_inst_adj )( REGS* iregs, BYTE* inst, bool pgmint )
{
QWORD   qword;                          /* Doubleword work area      */
BYTE    opcode;                         /* Instruction operation code*/
int     ilc;                            /* Instruction length        */
int     b1=-1, b2=-1, x1;               /* Register numbers          */
U16     xcode = 0;                      /* Exception code            */
VADR    addr1 = 0, addr2 = 0;           /* Operand addresses         */
char    buf[2048];                      /* Message buffer            */
char    buf2[512];
int     n;                              /* Number of bytes in buffer */
REGS*   regs;                           /* Copied regs               */

char    psw_inst_msg[160]   = {0};
char    op1_stor_msg[128]   = {0};
char    op2_stor_msg[128]   = {0};
char    regs_msg_buf[4*512] = {0};

    /* Ensure storage exists to attempt the display */
    if (iregs->mainlim == 0)
    {
        WRMSG( HHC02267, "I", "Real address is not valid" );
        return;
    }

    n = 0;
    buf[0] = '\0';

    if (iregs->ghostregs)
        regs = iregs;
    else if (!(regs = copy_regs( iregs )))
        return;

#if defined( _FEATURE_SIE )
    if (SIE_MODE( regs ))
        n += snprintf( buf + n, sizeof( buf )-n, "SIE: " );
#endif

    /* Display the PSW */
    memset( qword, 0, sizeof( qword ));
    copy_psw( regs, qword );

    if (sysblk.cpus > 1)
        n += snprintf( buf + n, sizeof( buf )-n, "%s%02X: ", PTYPSTR( regs->cpuad ), regs->cpuad );

    n += snprintf( buf + n, sizeof( buf )-n,
                "PSW=%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X ",
                qword[0], qword[1], qword[2], qword[3],
                qword[4], qword[5], qword[6], qword[7] );

#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
    n += snprintf (buf + n, sizeof(buf)-n,
                "%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X ",
                qword[8], qword[9], qword[10], qword[11],
                qword[12], qword[13], qword[14], qword[15]);
#endif

    /* Exit if instruction is not valid */
    if (!inst)
    {
        size_t len;
        MSGBUF( psw_inst_msg, "%s Instruction fetch error\n", buf );
        display_gregs( regs, regs_msg_buf, sizeof(regs_msg_buf)-1, "HHC02269I " );
        /* Remove unwanted extra trailing newline from regs_msg_buf */
        len = strlen( regs_msg_buf );
        if (len)
            regs_msg_buf[ len-1 ] = 0;
        // "%s%s" // (instruction fetch error + regs)
        WRMSG( HHC02325, "E", psw_inst_msg, regs_msg_buf );
        if (!iregs->ghostregs)
            free_aligned( regs );
        return;
    }

    /* Extract the opcode and determine the instruction length */
    opcode = inst[0];
    ilc = ILC( opcode );

    /* Format instruction line */
                 n += snprintf( buf + n, sizeof( buf )-n, "INST=%2.2X%2.2X", inst[0], inst[1] );
    if (ilc > 2) n += snprintf( buf + n, sizeof( buf )-n, "%2.2X%2.2X",      inst[2], inst[3] );
    if (ilc > 4) n += snprintf( buf + n, sizeof( buf )-n, "%2.2X%2.2X",      inst[4], inst[5] );
                 n += snprintf( buf + n, sizeof( buf )-n, " %s", (ilc < 4) ? "        " :
                                                                 (ilc < 6) ? "    " : "" );
    n += PRINT_INST( inst, buf + n );
    MSGBUF( psw_inst_msg, MSG( HHC02324, "I", buf ));

    n = 0;
    buf[0] = '\0';

    /* Process the first storage operand */
    if (1
        && ilc > 2
        && opcode != 0x84   // BRXH
        && opcode != 0x85   // BRXLE
        && opcode != 0xA5   // RI-x     (relative)
        && opcode != 0xA7   // RI-x     (relative)
        && opcode != 0xB3   // RRE/RRF
        && opcode != 0xC0   // RIL-x    (relative)
        && opcode != 0xC4   // RIL-x    (relative)
        && opcode != 0xC6   // RIL-x    (relative)
        && opcode != 0xEC   // RIE-x
    )
    {
        /* Calculate the effective address of the first operand */
        b1 = inst[2] >> 4;
        addr1 = ((inst[2] & 0x0F) << 8) | inst[3];
        if (b1 != 0)
        {
            addr1 += regs->GR( b1 );
            addr1 &= ADDRESS_MAXWRAP( regs );
        }

        /* Apply indexing for RX/RXE/RXF instructions */
        if (0
            || (opcode >= 0x40 && opcode <= 0x7F)
            || opcode == 0xB1   // LRA
            || opcode == 0xE3   // RXY-x
            || opcode == 0xED   // RXE-x, RXF-x, RXY-x, RSL-x
        )
        {
            x1 = inst[1] & 0x0F;
            if (x1 != 0)
            {
                addr1 += regs->GR( x1 );
                addr1 &= ADDRESS_MAXWRAP( regs );
            }
        }
    }

    /* Process the second storage operand */
    if (1
        && ilc > 4
        && opcode != 0xC0   // RIL-x    (relative)
        && opcode != 0xC4   // RIL-x    (relative)
        && opcode != 0xC6   // RIL-x    (relative)
        && opcode != 0xE3   // RXY-x
        && opcode != 0xEB   // RSY-x, SIY-x
        && opcode != 0xEC   // RIE-x
        && opcode != 0xED   // RXE-x, RXF-x, RXY-x, RSL-x
    )
    {
        /* Calculate the effective address of the second operand */
        b2 = inst[4] >> 4;
        addr2 = ((inst[4] & 0x0F) << 8) | inst[5];
        if (b2 != 0)
        {
            addr2 += regs->GR( b2 );
            addr2 &= ADDRESS_MAXWRAP( regs );
        }
    }

    /* Calculate the operand addresses for MVCL(E) and CLCL(E) */
    if (0
        || opcode == 0x0E   // MVCL
        || opcode == 0x0F   // CLCL
        || opcode == 0xA8   // MVCLE
        || opcode == 0xA9   // CLCLE
    )
    {
        b1 = inst[1] >> 4;   addr1 = regs->GR( b1 ) & ADDRESS_MAXWRAP( regs );
        b2 = inst[1] & 0x0F; addr2 = regs->GR( b2 ) & ADDRESS_MAXWRAP( regs );
    }

    /* Calculate the operand addresses for RRE instructions */
    if (0
        || (opcode == 0xB2 &&
            (0
             || (inst[1] >= 0x20 && inst[1] <= 0x2F)
             || (inst[1] >= 0x40 && inst[1] <= 0x6F)
             || (inst[1] >= 0xA0 && inst[1] <= 0xAF)
            )
           )
        || (opcode == 0xB9 &&
            (0
             || (inst[1] == 0x05)   // LURAG
             || (inst[1] == 0x25)   // STURG
             || (inst[1] >= 0x31)   // FIXME: Needs more specifics!
            )
           )
    )
    {
        b1 = inst[3] >> 4;
        addr1 = regs->GR( b1 ) & ADDRESS_MAXWRAP( regs );
        b2 = inst[3] & 0x0F;
        if (inst[1] >= 0x29 && inst[1] <= 0x2C)
            addr2 = regs->GR( b2 ) & ADDRESS_MAXWRAP_E( regs );
        else
            addr2 = regs->GR( b2 ) & ADDRESS_MAXWRAP( regs );
    }

    /* Calculate the operand address for RIL-x (relative) instructions */
    if (0
        || (opcode == 0xC0 &&
            (0
             || (inst[1] & 0x0F) == 0x00    // LARL   (relative)
             || (inst[1] & 0x0F) == 0x04    // BRCL   (relative)
             || (inst[1] & 0x0F) == 0x05    // BRASL  (relative)
            )
           )
        || opcode == 0xC4   // RIL-x  (relative)
        || opcode == 0xC6   // RIL-x  (relative)
    )
    {
        S64 offset = 2LL * (S32) (fetch_fw( inst+2 ));
        addr1 = (!regs->execflag) ? PSW_IA( regs, offset )
            : (regs->ET + offset) & ADDRESS_MAXWRAP( regs );
        b1 = 0;

        /* If we were called to display the instruction that
           program checked, then since the PSW's IA is pointing
           PAST the instruction (and not at it) and the operand
           is relative to the instruction, then we need to make
           a minor adjustment to our calculated operand address.
        */
        if (pgmint)         // ("display_pgmint_inst" call?)
            addr1 -= ilc;   // (yes, adjust operand address)
    }

    /* Format storage at first storage operand location */
    if (b1 >= 0)
    {
        n = 0;
        buf2[0] = '\0';

#if defined( _FEATURE_SIE )
        if (SIE_MODE( regs ))
            n += snprintf( buf2 + n, sizeof( buf2 )-n, "SIE: " );
#endif
        if (sysblk.cpus > 1)
            n += snprintf( buf2 + n, sizeof( buf2 )-n, "%s%02X: ",
                          PTYPSTR( regs->cpuad ), regs->cpuad );

        if (REAL_MODE( &regs->psw ))
            ARCH_DEP( display_virt )( regs, addr1, buf2+n, sizeof( buf2 )-n-1,
                                      USE_REAL_ADDR, ACCTYPE_READ, "", &xcode );
        else
            ARCH_DEP( display_virt )( regs, addr1, buf2+n, sizeof( buf2 )-n-1,
                                      b1, (opcode == 0x44
#if defined( FEATURE_035_EXECUTE_EXTN_FACILITY )
                                 || (opcode == 0xc6 && !(inst[1] & 0x0f))
#endif
                                                ? ACCTYPE_INSTFETCH :
                                 opcode == 0xB1 ? ACCTYPE_LRA :
                                                  ACCTYPE_READ ), "", &xcode );

        MSGBUF( op1_stor_msg, MSG( HHC02326, "I", buf2 ));
    }

    /* Format storage at second storage operand location */
    if (b2 >= 0)
    {
        int ar = b2;
        n = 0;
        buf2[0] = '\0';

#if defined(_FEATURE_SIE)
        if (SIE_MODE( regs ))
            n += snprintf( buf2 + n, sizeof( buf2 )-n, "SIE: " );
#endif
        if (sysblk.cpus > 1)
            n += snprintf( buf2 + n, sizeof( buf2 )-n, "%s%02X: ",
                           PTYPSTR( regs->cpuad ), regs->cpuad );
        if (0
            || REAL_MODE( &regs->psw )
            || (opcode == 0xB2 && inst[1] == 0x4B)  /*LURA*/
            || (opcode == 0xB2 && inst[1] == 0x46)  /*STURA*/
            || (opcode == 0xB9 && inst[1] == 0x05)  /*LURAG*/
            || (opcode == 0xB9 && inst[1] == 0x25)  /*STURG*/
        )
            ar = USE_REAL_ADDR;

        ARCH_DEP( display_virt )( regs, addr2, buf2+n, sizeof( buf2 )-n-1,
                                  ar, ACCTYPE_READ, "", &xcode );

        MSGBUF( op2_stor_msg, MSG( HHC02326, "I", buf2 ));
    }

    /* Format registers associated with the instruction */
    if (!sysblk.showregsnone)
        display_inst_regs( regs, inst, opcode, regs_msg_buf, sizeof( regs_msg_buf )-1 );

    if (sysblk.showregsfirst)
    {
        /* Remove unwanted extra trailing newline from regs_msg_buf */
        size_t len = strlen( regs_msg_buf );
        if (len)
            regs_msg_buf[ len-1 ] = 0;
    }

    /* Now display all instruction tracing messages all at once */
    if (sysblk.showregsfirst)
         LOGMSG( "%s%s%s%s", regs_msg_buf,
                             psw_inst_msg, op1_stor_msg, op2_stor_msg );
    else LOGMSG( "%s%s%s%s", psw_inst_msg, op1_stor_msg, op2_stor_msg,
                             regs_msg_buf );

    if (!iregs->ghostregs)
        free_aligned( regs );

} /* end function display_inst */

/*-------------------------------------------------------------------*/
/*                    display_inst                                   */
/*-------------------------------------------------------------------*/
void ARCH_DEP( display_inst )( REGS* iregs, BYTE* inst )
{
    ARCH_DEP( display_inst_adj )( iregs, inst, false );
}

/*-------------------------------------------------------------------*/
/*                    display_pgmint_inst                            */
/*-------------------------------------------------------------------*/
void ARCH_DEP( display_pgmint_inst )( REGS* iregs, BYTE* inst )
{
    ARCH_DEP( display_inst_adj )( iregs, inst, true );
}

/*-------------------------------------------------------------------*/
/*          (delineates ARCH_DEP from non-arch_dep)                  */
/*-------------------------------------------------------------------*/

#if !defined( _GEN_ARCH )

  #if defined(              _ARCH_NUM_1 )
    #define   _GEN_ARCH     _ARCH_NUM_1
    #include "hscmisc.c"
  #endif

  #if defined(              _ARCH_NUM_2 )
    #undef    _GEN_ARCH
    #define   _GEN_ARCH     _ARCH_NUM_2
    #include "hscmisc.c"
  #endif

/*-------------------------------------------------------------------*/
/*          (delineates ARCH_DEP from non-arch_dep)                  */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/*  non-ARCH_DEP section: compiled only ONCE after last arch built   */
/*-------------------------------------------------------------------*/
/*  Note: the last architecture has been built so the normal non-    */
/*  underscore FEATURE values are now #defined according to the      */
/*  LAST built architecture just built (usually zarch = 900). This   */
/*  means from this point onward (to the end of file) you should     */
/*  ONLY be testing the underscore _FEATURE values to see if the     */
/*  given feature was defined for *ANY* of the build architectures.  */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/*                System Shutdown Processing                         */
/*-------------------------------------------------------------------*/
/*                                                                   */
/* The following 'sigq' functions are responsible for ensuring all   */
/* of the CPUs are stopped ("quiesced") before continuing with the   */
/* Hercules shutdown processing and should NEVER be called directly. */
/*                                                                   */
/* They are instead called by 'do_shutdown' or 'do_shutdown_wait'    */
/* (defined further below), as needed and/or as appropriate.         */
/*                                                                   */
/*-------------------------------------------------------------------*/

static int wait_sigq_pending = 0;

static int is_wait_sigq_pending()
{
int pending;

    OBTAIN_INTLOCK(NULL);
    pending = wait_sigq_pending;
    RELEASE_INTLOCK(NULL);

    return pending;
}

static void wait_sigq_resp()
{
int pending;
    /* Wait for all CPU's to stop */
    do
    {
        OBTAIN_INTLOCK(NULL);
        wait_sigq_pending = 0;
        if (!are_all_cpus_stopped_intlock_held())
            wait_sigq_pending = 1;
        pending = wait_sigq_pending;
        RELEASE_INTLOCK(NULL);

        if(pending)
            SLEEP(1);
    }
    while(is_wait_sigq_pending());
}

static void cancel_wait_sigq()
{
    OBTAIN_INTLOCK(NULL);
    wait_sigq_pending = 0;
    RELEASE_INTLOCK(NULL);
}


/*-------------------------------------------------------------------*/
/*                       do_shutdown_now                             */
/*-------------------------------------------------------------------*/
/*                                                                   */
/*  This is the main shutdown processing function. It is NEVER       */
/*  called directly, but is instead ONLY called by either the        */
/*  'do_shutdown' or 'do_shutdown_wait' functions after all CPUs     */
/*  have been stopped.                                               */
/*                                                                   */
/*  It is responsible for releasing the device configuration and     */
/*  then calling the Hercules Dynamic Loader "hdl_atexit" function   */
/*  to invoke all registered Hercules at-exit/termination functions  */
/*  (similar to 'atexit' but unique to Hercules) to perform any      */
/*  other needed miscellaneous shutdown related processing.          */
/*                                                                   */
/*  Only after the above three tasks have been completed (stopping   */
/*  the CPUs, releasing the device configuration, calling registered */
/*  termination routines/functions) can Hercules then safely exit.   */
/*                                                                   */
/*  Note too that, *technically*, this function *should* wait for    */
/*  ALL other threads to finish terminating first before either      */
/*  exiting or returning back to the caller, but we currently don't  */
/*  enforce that (since that's REALLY what hdl_addshut + hdl_atexit  */
/*  are actually designed for!).                                     */
/*                                                                   */
/*  At the moment, as long as the three previously mentioned three   */
/*  most important shutdown tasks have been completed (stop cpus,    */
/*  release device config, call term funcs), then we consider the    */
/*  brunt of our shutdown processing to be completed and thus exit   */
/*  (or return back to the caller to let them exit instead).         */
/*                                                                   */
/*  If there are any stray threads still running when that happens,  */
/*  they will be automatically terminated by the operating sytem as  */
/*  is normal whenever a process exits.                              */
/*                                                                   */
/*  So if there are any threads that must be terminated completely   */
/*  and cleanly before Hercules can safely terminate, you BETTER     */
/*  add code to this function to ENSURE your thread is terminated    */
/*  properly! (and/or add a call to 'hdl_addshut' at the appropriate */
/*  place in your startup sequence). For this purpose, the use of    */
/*  "join_thread" is STRONGLY encouraged as it ENSURES that your     */
/*  thread will not continue until the thread in question has first  */
/*  completely exited beforehand.                                    */
/*                                                                   */
/*-------------------------------------------------------------------*/

static void do_shutdown_now()
{
    // "Begin Hercules shutdown"
    WRMSG( HHC01420, "I" );

    // (hack to prevent minor message glitch during shutdown)
    fflush( stdout );
    fflush( stderr );
    usleep( 10000 );

    ASSERT( !sysblk.shutfini );   // (sanity check)
    sysblk.shutfini = FALSE;      // (shutdown NOT finished yet)
    sysblk.shutdown = TRUE;       // (system shutdown initiated)

    /* Wakeup I/O subsystem to start I/O subsystem shutdown */
    {
        int  n;
        for (n=0; sysblk.devtnbr && n < 100; ++n)
        {
            signal_condition( &sysblk.ioqcond );
            usleep( 10000 );
        }
    }

    // "Calling termination routines"
    WRMSG( HHC01423, "I" );

    // (hack to prevent minor message glitch during shutdown)
    fflush( stdout );
    fflush( stderr );
    usleep( 10000 );

#if !defined( _MSVC_ )
    logger_unredirect();
#endif

    hdl_atexit();

    // "All termination routines complete"
    fprintf( stdout, MSG( HHC01424, "I" ));

    /*
    logmsg("Terminating threads\n");
    {
        // (none we really care about at the moment...)
    }
    logmsg("Threads terminations complete\n");
    */

    // "Hercules shutdown complete"
    fprintf( stdout, MSG( HHC01425, "I" ));

    sysblk.shutfini = TRUE;    // (shutdown is now complete)

    // "Hercules terminated"
    fprintf( stdout, MSG( HHC01412, "I" ));

    //                     PROGRAMMING NOTE

    // If we're NOT in "daemon_mode" (i.e. panel_display in control),
    // -OR- if a daemon_task DOES exist, then THEY are in control of
    // shutdown; THEY are responsible for exiting the system whenever
    // THEY feel it's proper to do so (by simply returning back to the
    // caller thereby allowing 'main' to return back to the operating
    // system).

    // OTHEWRWISE we ARE in "daemon_mode", but a daemon_task does NOT
    // exist, which means the main thread (tail end of 'impl.c') is
    // stuck in a loop reading log messages and writing them to the
    // logfile, so we need to do the exiting here since it obviously
    // cannot.

    if (sysblk.daemon_mode && !daemon_task)
    {
#ifdef _MSVC_
        socket_deinit();
#endif
        fflush( stdout );
        exit(0);
    }
}


/*-------------------------------------------------------------------*/
/*                     do_shutdown_wait                              */
/*-------------------------------------------------------------------*/
/*                                                                   */
/* This function simply waits for the CPUs to stop and then calls    */
/* the above do_shutdown_now function to perform the actual shutdown */
/* (which releases the device configuration, etc)                    */
/*                                                                   */
/*-------------------------------------------------------------------*/
static void* do_shutdown_wait(void* arg)
{
    UNREFERENCED( arg );
    WRMSG(HHC01426, "I");
    wait_sigq_resp();
    do_shutdown_now();
    return NULL;
}


/*-------------------------------------------------------------------*/
/*                       do_shutdown                                 */
/*-------------------------------------------------------------------*/
/*                                                                   */
/* This is the main system shutdown function, and the ONLY function  */
/* that should EVER be called to shut the system down. It calls one  */
/* or more of the above static helper functions as needed.           */
/*                                                                   */
/*-------------------------------------------------------------------*/
void do_shutdown()
{
TID tid;
    if ( sysblk.shutimmed )
        do_shutdown_now();
    else
    {
        if(is_wait_sigq_pending())
            cancel_wait_sigq();
        else
            if(can_signal_quiesce() && !signal_quiesce(0,0))
                create_thread(&tid, DETACHED, do_shutdown_wait,
                              NULL, "do_shutdown_wait");
            else
                do_shutdown_now();
    }
}


/*-------------------------------------------------------------------*/
/*                      display_regs32                               */
/*                      display_regs64                               */
/*-------------------------------------------------------------------*/
/* The following 2 routines display an array of 32/64 registers      */
/* 1st parameter is the register type (GR, CR, AR, etc..)            */
/* 2nd parameter is the CPU Address involved                         */
/* 3rd parameter is an array of 32/64 bit regs                       */
/* NOTE : 32 bit regs are displayed 4 by 4, while 64 bit regs are    */
/*        displayed 2 by 2. Change the modulo if to change this      */
/*        behaviour.                                                 */
/* These routines are intended to be invoked by display_gregs,       */
/* display_cregs and display_aregs                                   */
/* Ivan Warren 2005/11/07                                            */
/*-------------------------------------------------------------------*/
static int display_regs32(char *hdr,U16 cpuad,U32 *r,int numcpus,char *buf,int buflen,char *msghdr)
{
    int i;
    int len=0;
    for(i=0;i<16;i++)
    {
        if(!(i%4))
        {
            if(i)
            {
                len+=snprintf(buf+len, buflen-len, "%s", "\n");
            }
            len+=snprintf(buf+len, buflen-len, "%s", msghdr);
            if(numcpus>1)
            {
                len+=snprintf(buf+len,buflen-len,"%s%02X: ", PTYPSTR(cpuad), cpuad);
            }
        }
        if(i%4)
        {
            len+=snprintf(buf+len,buflen-len,"%s", " ");
        }
        len+=snprintf(buf+len,buflen-len,"%s%2.2d=%8.8"PRIX32,hdr,i,r[i]);
    }
    len+=snprintf(buf+len,buflen-len,"%s","\n");
    return(len);
}

#if defined(_900)

static int display_regs64(char *hdr,U16 cpuad,U64 *r,int numcpus,char *buf,int buflen,char *msghdr)
{
    int i;
    int rpl;
    int len=0;
    if(numcpus>1 && !(sysblk.insttrace || sysblk.inststep) )
    {
        rpl=2;
    }
    else
    {
        rpl=4;
    }
    for(i=0;i<16;i++)
    {
        if(!(i%rpl))
        {
            if(i)
            {
                len+=snprintf(buf+len,buflen-len,"%s", "\n");
            }
            len+=snprintf(buf+len,buflen-len, "%s", msghdr);
            if(numcpus>1)
            {
                len+=snprintf(buf+len,buflen-len,"%s%02X: ", PTYPSTR(cpuad), cpuad);
            }
        }
        if(i%rpl)
        {
            len+=snprintf(buf+len,buflen-len,"%s"," ");
        }
        len+=snprintf(buf+len,buflen-len,"%s%1.1X=%16.16"PRIX64,hdr,i,r[i]);
    }
    len+=snprintf(buf+len,buflen-len,"%s","\n");
    return(len);
}

#endif // _900


/*-------------------------------------------------------------------*/
/*        Display registers for the instruction display              */
/*-------------------------------------------------------------------*/
static int display_inst_regs (REGS *regs, BYTE *inst, BYTE opcode, char *buf, int buflen )
{
    int len=0;

    /* Display the general purpose registers */
    if (!(opcode == 0xB3 || (opcode >= 0x20 && opcode <= 0x3F))
        || (opcode == 0xB3 && (
                (inst[1] >= 0x80 && inst[1] <= 0xCF)
                || (inst[1] >= 0xE1 && inst[1] <= 0xFE)
           )))
    {
        len += display_gregs (regs, buf + len, buflen - len - 1, "HHC02269I " );
    }

    /* Display control registers if appropriate */
    if (!REAL_MODE(&regs->psw) || opcode == 0xB2 || opcode == 0xB6 || opcode == 0xB7)
    {
        len += display_cregs (regs, buf + len, buflen - len - 1, "HHC02271I ");
    }

    /* Display access registers if appropriate */
    if (!REAL_MODE(&regs->psw) && ACCESS_REGISTER_MODE(&regs->psw))
    {
        len += display_aregs (regs, buf + len, buflen - len - 1, "HHC02272I ");
    }

    /* Display floating point control register if AFP enabled */
    if ((regs->CR(0) & CR0_AFP) && (
                                (opcode == 0x01 && inst[1] == 0x0A)          /* PFPO Perform Floating Point Operation  */
                                || (opcode == 0xB2 && inst[1] == 0x99)       /* SRNM   Set BFP Rounding mode 2-bit     */
                                || (opcode == 0xB2 && inst[1] == 0x9C)       /* STFPC  Store FPC                       */
                                || (opcode == 0xB2 && inst[1] == 0x9D)       /* LFPC   Load FPC                        */
                                || (opcode == 0xB2 && inst[1] == 0xB8)       /* SRNMB  Set BFP Rounding mode 3-bit     */
                                || (opcode == 0xB2 && inst[1] == 0xB9)       /* SRNMT  Set DFP Rounding mode           */
                                || (opcode == 0xB2 && inst[1] == 0xBD)       /* LFAS   Load FPC and Signal             */
                                || (opcode == 0xB3 && (inst[1] <= 0x1F))                       /* RRE BFP arithmetic   */
                                || (opcode == 0xB3 && (inst[1] >= 0x40 && inst[1] <= 0x5F))    /* RRE BFP arithmetic   */
                                || (opcode == 0xB3 && (inst[1] >= 0x84 && inst[1] <= 0x8C))    /* SFPC, SFASR, EFPC    */
                                || (opcode == 0xB3 && (inst[1] >= 0x90 && inst[1] <= 0xAF))    /* RRE BFP arithmetic   */
                                || (opcode == 0xB3 && (inst[1] >= 0xD0))/*inst[1] <= 0xFF)) */ /* RRE DFP arithmetic   */
                                || (opcode == 0xB9 && (inst[1] >= 0x41 && inst[1] <= 0x43))    /* DFP Conversions      */
                                || (opcode == 0xB9 && (inst[1] >= 0x49 && inst[1] <= 0x5B))    /* DFP Conversions      */
                                || (opcode == 0xED && (inst[1] <= 0x1F))                       /* RXE BFP arithmetic   */
                                || (opcode == 0xED && (inst[1] >= 0x40 && inst[1] <= 0x59))    /* RXE DFP shifts, tests*/
                                || (opcode == 0xED && (inst[1] >= 0xA8 && inst[1] <= 0xAF)))   /* RXE DFP conversions  */
        )
    {
        len += snprintf(buf + len, buflen - len, MSG(HHC02276,"I", regs->fpc));
    }

    /* Display floating-point registers if appropriate */
    if ( (opcode == 0xB3 && !((inst[1] == 0x84) || (inst[1] == 0x85) || (inst[1] == 0x8C)))  /* exclude FPC-only instrs  */
        || (opcode == 0xED)
        || (opcode >= 0x20 && opcode <= 0x3F)  /* HFP Arithmetic and load/store  */
        || (opcode >= 0x60 && opcode <= 0x70)  /* HFP Arithmetic and load/store  */
        || (opcode >= 0x78 && opcode <= 0x7F)  /* HFP Arithmetic and load/store  */
        || (opcode == 0xB2 && inst[1] == 0x2D) /* DXR  Divide HFP extended               */
        || (opcode == 0xB2 && inst[1] == 0x44) /* SQDR Square Root HFP long              */
        || (opcode == 0xB2 && inst[1] == 0x45) /* SQER Square Root HFP short             */
        || (opcode == 0xB9 && (inst[1] >= 0x41 && inst[1] <= 0x43)) /* DFP Conversions*/
        || (opcode == 0xB9 && (inst[1] >= 0x49 && inst[1] <= 0x5B)) /* DFP Conversions*/
        || (opcode == 0x01 && inst[1] == 0x0A) /* PFPO Perform Floating Point Operation  */
        )
    {
        len += display_fregs (regs, buf + len, buflen - len - 1, "HHC02270I ");
    }

    if (len && sysblk.showregsfirst)
        len += snprintf( buf + len, buflen - len, "\n" );

    return len;
}


/*-------------------------------------------------------------------*/
/*             Display general purpose registers                     */
/*-------------------------------------------------------------------*/
int display_gregs (REGS *regs, char *buf, int buflen, char *hdr)
{
    int i;
    U32 gprs[16];
#if defined(_900)
    U64 ggprs[16];
#endif

#if defined(_900)
    if(regs->arch_mode != ARCH_900_IDX)
    {
#endif
        for(i=0;i<16;i++)
        {
            gprs[i]=regs->GR_L(i);
        }
        return(display_regs32("GR",regs->cpuad,gprs,sysblk.cpus,buf,buflen,hdr));
#if defined(_900)
    }
    else
    {
        for(i=0;i<16;i++)
        {
            ggprs[i]=regs->GR_G(i);
        }
        return(display_regs64("R",regs->cpuad,ggprs,sysblk.cpus,buf,buflen,hdr));
    }
#endif

} /* end function display_gregs */


/*-------------------------------------------------------------------*/
/*                  Display control registers                        */
/*-------------------------------------------------------------------*/
int display_cregs (REGS *regs, char *buf, int buflen, char *hdr)
{
    int i;
    U32 crs[16];
#if defined(_900)
    U64 gcrs[16];
#endif

#if defined(_900)
    if(regs->arch_mode != ARCH_900_IDX)
    {
#endif
        for(i=0;i<16;i++)
        {
            crs[i]=regs->CR_L(i);
        }
        return(display_regs32("CR",regs->cpuad,crs,sysblk.cpus,buf,buflen,hdr));
#if defined(_900)
    }
    else
    {
        for(i=0;i<16;i++)
        {
            gcrs[i]=regs->CR_G(i);
        }
        return(display_regs64("C",regs->cpuad,gcrs,sysblk.cpus,buf,buflen,hdr));
    }
#endif

} /* end function display_cregs */


/*-------------------------------------------------------------------*/
/*                    Display access registers                       */
/*-------------------------------------------------------------------*/
int display_aregs (REGS *regs, char *buf, int buflen, char *hdr)
{
    int i;
    U32 ars[16];

    for(i=0;i<16;i++)
    {
        ars[i]=regs->AR(i);
    }
    return(display_regs32("AR",regs->cpuad,ars,sysblk.cpus,buf,buflen,hdr));

} /* end function display_aregs */


/*-------------------------------------------------------------------*/
/*               Display floating point registers                    */
/*-------------------------------------------------------------------*/
int display_fregs (REGS *regs, char *buf, int buflen, char *hdr)
{
char cpustr[32] = "";

    if(sysblk.cpus>1)
        MSGBUF(cpustr, "%s%s%02X: ", hdr, PTYPSTR(regs->cpuad), regs->cpuad);
    else
        MSGBUF(cpustr, "%s", hdr);

    if(regs->CR(0) & CR0_AFP)
        return(snprintf(buf,buflen,
            "%sFPR0=%8.8X%8.8X FPR2=%8.8X%8.8X\n"
            "%sFPR1=%8.8X%8.8X FPR3=%8.8X%8.8X\n"
            "%sFPR4=%8.8X%8.8X FPR6=%8.8X%8.8X\n"
            "%sFPR5=%8.8X%8.8X FPR7=%8.8X%8.8X\n"
            "%sFPR8=%8.8X%8.8X FP10=%8.8X%8.8X\n"
            "%sFPR9=%8.8X%8.8X FP11=%8.8X%8.8X\n"
            "%sFP12=%8.8X%8.8X FP14=%8.8X%8.8X\n"
            "%sFP13=%8.8X%8.8X FP15=%8.8X%8.8X\n"
            ,cpustr, regs->fpr[0],  regs->fpr[1],  regs->fpr[4],  regs->fpr[5]
            ,cpustr, regs->fpr[2],  regs->fpr[3],  regs->fpr[6],  regs->fpr[7]
            ,cpustr, regs->fpr[8],  regs->fpr[9],  regs->fpr[12], regs->fpr[13]
            ,cpustr, regs->fpr[10], regs->fpr[11], regs->fpr[14], regs->fpr[15]
            ,cpustr, regs->fpr[16], regs->fpr[17], regs->fpr[20], regs->fpr[21]
            ,cpustr, regs->fpr[18], regs->fpr[19], regs->fpr[22], regs->fpr[23]
            ,cpustr, regs->fpr[24], regs->fpr[25], regs->fpr[28], regs->fpr[29]
            ,cpustr, regs->fpr[26], regs->fpr[27], regs->fpr[30], regs->fpr[31]
        ));
    else
        return(snprintf(buf,buflen,
            "%sFPR0=%8.8X%8.8X FPR2=%8.8X%8.8X\n"
            "%sFPR4=%8.8X%8.8X FPR6=%8.8X%8.8X\n"
            ,cpustr, regs->fpr[0], regs->fpr[1], regs->fpr[2], regs->fpr[3]
            ,cpustr, regs->fpr[4], regs->fpr[5], regs->fpr[6], regs->fpr[7]
        ));

} /* end function display_fregs */


/*-------------------------------------------------------------------*/
/*                     Display subchannel                            */
/*-------------------------------------------------------------------*/
int display_subchannel (DEVBLK *dev, char *buf, int buflen, char *hdr)
{
    static const char*  status_type[3] = {"Device Status    ",
                                          "Unit Status      ",
                                          "Subchannel Status"};

    struct BITS { U8 b7:1; U8 b6:1; U8 b5:1; U8 b4:1; U8 b3:1; U8 b2:1; U8 b1:1; U8 b0:1; };
    union ByteToBits { struct BITS b; U8 status; } u;
    int len = 0;

    len+=snprintf(buf+len,buflen-len,
        "%s%1d:%04X D/T%04X\n",
        hdr, LCSS_DEVNUM, dev->devtype);

    if (ARCH_370_IDX == sysblk.arch_mode)
    {
        len+=snprintf(buf+len,buflen-len,
            "%s  CSW Flags:%2.2X CCW:%2.2X%2.2X%2.2X            Flags\n"
            "%s         US:%2.2X  CS:%2.2X Count:%2.2X%2.2X       (Key) Subchannel key          %1.1X\n"
            "%s                                       (S)   Suspend control         %1.1X\n"
            "%s                                       (L)   Extended format         %1.1X\n"
            "%s  Subchannel Internal Management       (CC)  Deferred condition code %1.1X\n",
            hdr, dev->scsw.flag0,
                 dev->scsw.ccwaddr[1], dev->scsw.ccwaddr[2], dev->scsw.ccwaddr[3],
            hdr, dev->scsw.unitstat, dev->scsw.chanstat,
                 dev->scsw.count[0], dev->scsw.count[1],
                 (dev->scsw.flag0 & SCSW0_KEY)      >> 4,
            hdr, (dev->scsw.flag0 & SCSW0_S)        >> 3,
            hdr, (dev->scsw.flag0 & SCSW0_L)        >> 2,
            hdr, (dev->scsw.flag0 & SCSW0_CC));
    }

    len+=snprintf(buf+len,buflen-len,
        "%s  Subchannel Number[%04X]\n"
        "%s    Path Management Control Word (PMCW)\n"
        "%s  IntParm:%2.2X%2.2X%2.2X%2.2X\n"
        "%s    Flags:%2.2X%2.2X        Dev:%2.2X%2.2X\n"
        "%s      LPM:%2.2X PNOM:%2.2X LPUM:%2.2X PIM:%2.2X\n"
        "%s      MBI:%2.2X%2.2X        POM:%2.2X PAM:%2.2X\n"
        "%s  CHPID 0:%2.2X    1:%2.2X    2:%2.2X   3:%2.2X\n"
        "%s        4:%2.2X    5:%2.2X    6:%2.2X   7:%2.2X\n"
        "%s     Misc:%2.2X%2.2X%2.2X%2.2X\n",
        hdr, dev->subchan,
        hdr,
        hdr, dev->pmcw.intparm[0], dev->pmcw.intparm[1],
        dev->pmcw.intparm[2], dev->pmcw.intparm[3],
        hdr, dev->pmcw.flag4, dev->pmcw.flag5,
        dev->pmcw.devnum[0], dev->pmcw.devnum[1],
        hdr, dev->pmcw.lpm, dev->pmcw.pnom, dev->pmcw.lpum, dev->pmcw.pim,
        hdr, dev->pmcw.mbi[0], dev->pmcw.mbi[1],
        dev->pmcw.pom, dev->pmcw.pam,
        hdr, dev->pmcw.chpid[0], dev->pmcw.chpid[1],
        dev->pmcw.chpid[2], dev->pmcw.chpid[3],
        hdr, dev->pmcw.chpid[4], dev->pmcw.chpid[5],
        dev->pmcw.chpid[6], dev->pmcw.chpid[7],
        hdr,dev->pmcw.zone, dev->pmcw.flag25,
        dev->pmcw.flag26, dev->pmcw.flag27);

    len+=snprintf(buf+len,buflen-len,
        "%s  Subchannel Status Word (SCSW)\n"
        "%s    Flags: %2.2X%2.2X  Subchan Ctl: %2.2X%2.2X     (FC)  Function Control\n"
        "%s      CCW: %2.2X%2.2X%2.2X%2.2X                          Start                   %1.1X\n"
        "%s       DS: %2.2X  SS: %2.2X  Count: %2.2X%2.2X           Halt                    %1.1X\n"
        "%s                                             Clear                   %1.1X\n"
        "%s    Flags                              (AC)  Activity Control\n"
        "%s      (Key) Subchannel key          %1.1X        Resume pending          %1.1X\n"
        "%s      (S)   Suspend control         %1.1X        Start pending           %1.1X\n"
        "%s      (L)   Extended format         %1.1X        Halt pending            %1.1X\n"
        "%s      (CC)  Deferred condition code %1.1X        Clear pending           %1.1X\n"
        "%s      (F)   CCW-format control      %1.1X        Subchannel active       %1.1X\n"
        "%s      (P)   Prefetch control        %1.1X        Device active           %1.1X\n"
        "%s      (I)   Initial-status control  %1.1X        Suspended               %1.1X\n"
        "%s      (A)   Address-limit control   %1.1X  (SC)  Status Control\n"
        "%s      (U)   Suppress-suspend int.   %1.1X        Alert                   %1.1X\n"
        "%s    Subchannel Control                       Intermediate            %1.1X\n"
        "%s      (Z)   Zero condition code     %1.1X        Primary                 %1.1X\n"
        "%s      (E)   Extended control (ECW)  %1.1X        Secondary               %1.1X\n"
        "%s      (N)   Path not operational    %1.1X        Status pending          %1.1X\n"
        "%s      (Q)   QDIO active             %1.1X\n",
        hdr,
        hdr, dev->scsw.flag0, dev->scsw.flag1, dev->scsw.flag2, dev->scsw.flag3,
        hdr, dev->scsw.ccwaddr[0], dev->scsw.ccwaddr[1],
             dev->scsw.ccwaddr[2], dev->scsw.ccwaddr[3],
             (dev->scsw.flag2 & SCSW2_FC_START) >> 6,
        hdr, dev->scsw.unitstat, dev->scsw.chanstat,
             dev->scsw.count[0], dev->scsw.count[1],
             (dev->scsw.flag2 & SCSW2_FC_HALT)  >> 5,
        hdr, (dev->scsw.flag2 & SCSW2_FC_CLEAR) >> 4,
        hdr,
        hdr, (dev->scsw.flag0 & SCSW0_KEY)      >> 4,
             (dev->scsw.flag2 & SCSW2_AC_RESUM) >> 3,
        hdr, (dev->scsw.flag0 & SCSW0_S)        >> 3,
             (dev->scsw.flag2 & SCSW2_AC_START) >> 2,
        hdr, (dev->scsw.flag0 & SCSW0_L)        >> 2,
             (dev->scsw.flag2 & SCSW2_AC_HALT)  >> 1,
        hdr, (dev->scsw.flag0 & SCSW0_CC),
             (dev->scsw.flag2 & SCSW2_AC_CLEAR),
        hdr, (dev->scsw.flag1 & SCSW1_F)        >> 7,
             (dev->scsw.flag3 & SCSW3_AC_SCHAC) >> 7,
        hdr, (dev->scsw.flag1 & SCSW1_P)        >> 6,
             (dev->scsw.flag3 & SCSW3_AC_DEVAC) >> 6,
        hdr, (dev->scsw.flag1 & SCSW1_I)        >> 5,
             (dev->scsw.flag3 & SCSW3_AC_SUSP)  >> 5,
        hdr, (dev->scsw.flag1 & SCSW1_A)        >> 4,
        hdr, (dev->scsw.flag1 & SCSW1_U)        >> 3,
             (dev->scsw.flag3 & SCSW3_SC_ALERT) >> 4,
        hdr, (dev->scsw.flag3 & SCSW3_SC_INTER) >> 3,
        hdr, (dev->scsw.flag1 & SCSW1_Z)        >> 2,
             (dev->scsw.flag3 & SCSW3_SC_PRI)   >> 2,
        hdr, (dev->scsw.flag1 & SCSW1_E)        >> 1,
             (dev->scsw.flag3 & SCSW3_SC_SEC)   >> 1,
        hdr, (dev->scsw.flag1 & SCSW1_N),
             (dev->scsw.flag3 & SCSW3_SC_PEND),
        hdr, (dev->scsw.flag2 & SCSW2_Q)        >> 7);

    u.status = (U8)dev->scsw.unitstat;
    len+=snprintf(buf+len,buflen-len,
        "%s    %s %s%s%s%s%s%s%s%s%s\n",
        hdr, status_type[(sysblk.arch_mode == ARCH_370_IDX)],
        u.status == 0 ? "is Normal" : "",
        u.b.b0 ? "Attention " : "",
        u.b.b1 ? "SM " : "",
        u.b.b2 ? "CUE " : "",
        u.b.b3 ? "Busy " : "",
        u.b.b4 ? "CE " : "",
        u.b.b5 ? "DE " : "",
        u.b.b6 ? "UC " : "",
        u.b.b7 ? "UE " : "");

    u.status = (U8)dev->scsw.chanstat;
    len+=snprintf(buf+len,buflen-len,
        "%s    %s %s%s%s%s%s%s%s%s%s\n",
        hdr, status_type[2],
        u.status == 0 ? "is Normal" : "",
        u.b.b0 ? "PCI " : "",
        u.b.b1 ? "IL " : "",
        u.b.b2 ? "PC " : "",
        u.b.b3 ? "ProtC " : "",
        u.b.b4 ? "CDC " : "",
        u.b.b5 ? "CCC " : "",
        u.b.b6 ? "ICC " : "",
        u.b.b7 ? "CC " : "");

    // PROGRAMMING NOTE: the following ugliness is needed
    // because 'snprintf' is a macro on Windows builds and
    // you obviously can't use the preprocessor to select
    // the arguments to be passed to a preprocessor macro.

#if defined( OPTION_SHARED_DEVICES )
  #define BUSYSHAREABLELINE_PATTERN     "%s    busy             %1.1X    shareable     %1.1X\n"
  #define BUSYSHAREABLELINE_VALUE       hdr, dev->busy, dev->shareable,
#else // !defined( OPTION_SHARED_DEVICES )
  #define BUSYSHAREABLELINE_PATTERN     "%s    busy             %1.1X\n"
  #define BUSYSHAREABLELINE_VALUE       hdr, dev->busy,
#endif // defined( OPTION_SHARED_DEVICES )

    len+=snprintf(buf+len,buflen-len,
        "%s  DEVBLK Status\n"
        BUSYSHAREABLELINE_PATTERN
        "%s    suspended        %1.1X    console       %1.1X    rlen3270 %5d\n"
        "%s    pending          %1.1X    connected     %1.1X\n"
        "%s    pcipending       %1.1X    readpending   %1.1X\n"
        "%s    attnpending      %1.1X    connecting    %1.1X\n"
        "%s    startpending     %1.1X    localhost     %1.1X\n"
        "%s    resumesuspended  %1.1X    reserved      %1.1X\n"
        "%s    tschpending      %1.1X    locked        %1.1X\n",
        hdr,
        BUSYSHAREABLELINE_VALUE
        hdr, dev->suspended,          dev->console,     dev->rlen3270,
        hdr, dev->pending,            dev->connected,
        hdr, dev->pcipending,         dev->readpending,
        hdr, dev->attnpending,        dev->connecting,
        hdr, dev->startpending,       dev->localhost,
        hdr, dev->resumesuspended,    dev->reserved,
        hdr, dev->tschpending,        test_lock(&dev->lock) ? 1 : 0);

    return(len);

} /* end function display_subchannel */


/*-------------------------------------------------------------------*/
/*      Parse a storage range or storage alteration operand          */
/*-------------------------------------------------------------------*/
/*                                                                   */
/* Valid formats for a storage range operand are:                    */
/*      startaddr                                                    */
/*      startaddr-endaddr                                            */
/*      startaddr.length                                             */
/* where startaddr, endaddr, and length are hexadecimal values.      */
/*                                                                   */
/* Valid format for a storage alteration operand is:                 */
/*      startaddr=hexstring (up to 32 pairs of digits)               */
/*                                                                   */
/* Return values:                                                    */
/*      0  = operand contains valid storage range display syntax;    */
/*           start/end of range is returned in saddr and eaddr       */
/*      >0 = operand contains valid storage alteration syntax;       */
/*           return value is number of bytes to be altered;          */
/*           start/end/value are returned in saddr, eaddr, newval    */
/*      -1 = error message issued                                    */
/*-------------------------------------------------------------------*/
static int parse_range (char *operand, U64 maxadr, U64 *sadrp,
                        U64 *eadrp, BYTE *newval)
{
U64     opnd1, opnd2;                   /* Address/length operands   */
U64     saddr, eaddr;                   /* Range start/end addresses */
int     rc;                             /* Return code               */
int     n;                              /* Number of bytes altered   */
int     h1, h2;                         /* Hexadecimal digits        */
char    *s;                             /* Alteration value pointer  */
BYTE    delim;                          /* Operand delimiter         */
BYTE    c;                              /* Character work area       */

    if (!operand)
    {
        // "Missing or invalid argument(s)"
        WRMSG( HHC17000, "E" );
        return -1;
    }

    rc = sscanf(operand, "%"SCNx64"%c%"SCNx64"%c",
                &opnd1, &delim, &opnd2, &c);

    /* Process storage alteration operand */
    if (rc > 2 && delim == '=' && newval)
    {
        s = strchr (operand, '=');
        n = 0;
        while (1)
        {
            h1 = *(++s);
            if (h1 == '\0'  || h1 == '#' ) break;
            if (h1 == SPACE || h1 == '\t') continue;
            h1 = toupper(h1);
            h1 = (h1 >= '0' && h1 <= '9') ? h1 - '0' :
                 (h1 >= 'A' && h1 <= 'F') ? h1 - 'A' + 10 : -1;
            if (h1 < 0)
            {
                WRMSG(HHC02205, "E", s, ": invalid hex digit");
                return -1;
            }
            h2 = *(++s);
            h2 = toupper(h2);
            h2 = (h2 >= '0' && h2 <= '9') ? h2 - '0' :
                 (h2 >= 'A' && h2 <= 'F') ? h2 - 'A' + 10 : -1;
            if (h2 < 0)
            {
                WRMSG(HHC02205, "E", --s, ": invalid hex pair");
                return -1;
            }
            if (n >= 32)
            {
                WRMSG(HHC02205, "E", --s, ": only a maximum of 32 bytes may be altered");
                return -1;
            }
            newval[n++] = (h1 << 4) | h2;
        } /* end for(n) */
        saddr = opnd1;
        eaddr = saddr + n - 1;
    }
    else
    {
        /* Process storage range operand */
        saddr = opnd1;
        if (rc == 1)
        {
            /* If only starting address is specified, default to
               64 byte display, or less if near end of storage */
            eaddr = saddr + 0x3F;
            if (eaddr > maxadr) eaddr = maxadr;
        }
        else
        {
            /* Ending address or length is specified */
            if (rc != 3 || !(delim == '-' || delim == '.'))
            {
                WRMSG(HHC02205, "E", operand, "");
                return -1;
            }
            eaddr = (delim == '.') ? saddr + opnd2 - 1 : opnd2;
        }
        /* Set n=0 to indicate storage display only */
        n = 0;
    }

    /* Check for valid range */
    if (saddr > maxadr || eaddr > maxadr || eaddr < saddr)
    {
        WRMSG(HHC02205, "E", operand, ": invalid range");
        return -1;
    }

    /* Return start/end addresses and number of bytes altered */
    *sadrp = saddr;
    *eadrp = eaddr;
    return n;

} /* end function parse_range */


/*-------------------------------------------------------------------*/
/*  get_connected_client   return IP address and hostname of the     */
/*                         client that is connected to this device   */
/*-------------------------------------------------------------------*/
void get_connected_client (DEVBLK* dev, char** pclientip, char** pclientname)
{
    *pclientip   = NULL;
    *pclientname = NULL;

    obtain_lock (&dev->lock);

    if (dev->bs             /* if device is a socket device,   */
        && dev->fd != -1)   /* and a client is connected to it */
    {
        *pclientip   = strdup(dev->bs->clientip);
        *pclientname = strdup(dev->bs->clientname);
    }

    release_lock (&dev->lock);
}

/*-------------------------------------------------------------------*/
/*  Return the address of a regs structure to be used for address    */
/*  translation.  This address should be freed by the caller.        */
/*-------------------------------------------------------------------*/
DLL_EXPORT REGS* copy_regs( REGS* regs )
{
 REGS  *newregs, *hostregs;
 size_t size;

    size = (SIE_MODE( regs ) || SIE_ACTIVE( regs )) ? 2 * sizeof( REGS )
                                                    :     sizeof( REGS );
    if (!(newregs = malloc_aligned( size, 4096 )))
    {
        char buf[64];
        MSGBUF( buf, "malloc(%d)", (int)size );
        // "Error in function %s: %s"
        WRMSG( HHC00075, "E", buf, strerror( errno ));
        return NULL;
    }

    /* Perform partial copy and clear the TLB */
    memcpy(  newregs, regs, sysblk.regs_copy_len );
    memset( &newregs->tlb.vaddr, 0, TLBN * sizeof( DW ));

    newregs->tlbID      = 1;
    newregs->ghostregs  = 1;
    HOST(  newregs )    = newregs;
    GUEST( newregs )    = NULL;
    newregs->sie_active = 0;

    /* Copy host regs if in SIE mode (newregs is SIE guest regs) */
    if (SIE_MODE( newregs ))
    {
        hostregs = newregs + 1;

        memcpy(  hostregs, HOSTREGS, sysblk.regs_copy_len );
        memset( &hostregs->tlb.vaddr, 0, TLBN * sizeof( DW ));

        hostregs->tlbID     = 1;
        hostregs->ghostregs = 1;

        HOST(  hostregs )   = hostregs;
        GUEST( hostregs )   = newregs;

        HOST(  newregs  )   = hostregs;
        GUEST( newregs  )   = newregs;
    }

    return newregs;
}


/*-------------------------------------------------------------------*/
/*      Format Channel Report Word (CRW) for display                 */
/*-------------------------------------------------------------------*/
const char* FormatCRW( U32 crw, char* buf, size_t bufsz )
{
    static const char* rsctab[] =
    {
        "0",
        "1",
        "MONIT",
        "SUBCH",
        "CHPID",
        "5",
        "6",
        "7",
        "8",
        "CAF",
        "10",
        "CSS",
    };
    static const BYTE  numrsc  =  _countof( rsctab );

    static const char* erctab[] =
    {
        "NULL",
        "AVAIL",
        "INIT",
        "TEMP",
        "ALERT",
        "ABORT",
        "ERROR",
        "RESET",
        "MODFY",
        "9",
        "RSTRD",
    };
    static const BYTE  numerc  =  _countof( erctab );

    if (!buf)
        return NULL;
    if (bufsz)
        *buf = 0;
    if (bufsz <= 1)
        return buf;

    if (crw)
    {
        U32     flags   =  (U32)    ( crw & CRW_FLAGS_MASK );
        BYTE    erc     =  (BYTE) ( ( crw & CRW_ERC_MASK   ) >> 16 );
        BYTE    rsc     =  (BYTE) ( ( crw & CRW_RSC_MASK   ) >> 24 );
        U16     rsid    =  (U16)    ( crw & CRW_RSID_MASK  );

        snprintf( buf, bufsz,

            "RSC:%d=%s, ERC:%d=%s, RSID:%d=0x%4.4X Flags:%s%s%s%s%s%s%s"

            , rsc
            , rsc < numrsc ? rsctab[ rsc ] : "???"

            , erc
            , erc < numerc ? erctab[ erc ] : "???"

            , rsid
            , rsid

            , ( flags & CRW_FLAGS_MASK ) ? ""            : "0"
            , ( flags & 0x80000000     ) ? "0x80000000," : ""
            , ( flags & CRW_SOL        ) ? "SOL,"        : ""
            , ( flags & CRW_OFLOW      ) ? "OFLOW,"      : ""
            , ( flags & CRW_CHAIN      ) ? "CHAIN,"      : ""
            , ( flags & CRW_AR         ) ? "AR,"         : ""
            , ( flags & 0x00400000     ) ? "0x00400000," : ""
        );

        rtrim( buf, "," );              // (remove trailing comma)
    }
    else
        strlcpy( buf, "(end)", bufsz ); // (end of channel report)

    return buf;
}


/*-------------------------------------------------------------------*/
/*      Format Operation-Request Block (ORB) for display             */
/*-------------------------------------------------------------------*/
const char* FormatORB( ORB* orb, char* buf, size_t bufsz )
{
    if (!buf)
        return NULL;

    if (bufsz)
        *buf = 0;

    if (bufsz <= 1 || !orb)
        return buf;

    snprintf( buf, bufsz,

        "IntP:%2.2X%2.2X%2.2X%2.2X Key:%d LPM:%2.2X "
        "Flags:%X%2.2X%2.2X %c%c%c%c%c%c%c%c%c%c%c%c %c%c.....%c "
        "%cCW:%2.2X%2.2X%2.2X%2.2X"

        , orb->intparm[0], orb->intparm[1], orb->intparm[2], orb->intparm[3]
        , (orb->flag4 & ORB4_KEY) >> 4
        , orb->lpm

        , (orb->flag4 & ~ORB4_KEY)
        , orb->flag5
        , orb->flag7

        , ( orb->flag4 & ORB4_S ) ? 'S' : '.'
        , ( orb->flag4 & ORB4_C ) ? 'C' : '.'
        , ( orb->flag4 & ORB4_M ) ? 'M' : '.'
        , ( orb->flag4 & ORB4_Y ) ? 'Y' : '.'

        , ( orb->flag5 & ORB5_F ) ? 'F' : '.'
        , ( orb->flag5 & ORB5_P ) ? 'P' : '.'
        , ( orb->flag5 & ORB5_I ) ? 'I' : '.'
        , ( orb->flag5 & ORB5_A ) ? 'A' : '.'

        , ( orb->flag5 & ORB5_U ) ? 'U' : '.'
        , ( orb->flag5 & ORB5_B ) ? 'B' : '.'
        , ( orb->flag5 & ORB5_H ) ? 'H' : '.'
        , ( orb->flag5 & ORB5_T ) ? 'T' : '.'

        , ( orb->flag7 & ORB7_L ) ? 'L' : '.'
        , ( orb->flag7 & ORB7_D ) ? 'D' : '.'
        , ( orb->flag7 & ORB7_X ) ? 'X' : '.'

        , ( orb->flag5 & ORB5_B ) ? 'T' : 'C'  // (TCW or CCW)

        , orb->ccwaddr[0], orb->ccwaddr[1], orb->ccwaddr[2], orb->ccwaddr[3]
    );

    return buf;
}


/*-------------------------------------------------------------------*/
/*     Format ESW's Subchannel Logout information for display        */
/*-------------------------------------------------------------------*/
const char* FormatSCL( ESW* esw, char* buf, size_t bufsz )
{
static const char* sa[] =
{
    "00",
    "RD",
    "WR",
    "BW",
};
static const char* tc[] =
{
    "HA",
    "ST",
    "CL",
    "11",
};

    if (!buf)
        return NULL;
    if (bufsz)
        *buf = 0;
    if (bufsz <= 1 || !esw)
        return buf;

    snprintf( buf, bufsz,

        "ESF:%c%c%c%c%c%c%c%c%s FVF:%c%c%c%c%c LPUM:%2.2X SA:%s TC:%s Flgs:%c%c%c SC=%d"

        , ( esw->scl0 & 0x80           ) ? '0' : '.'
        , ( esw->scl0 & SCL0_ESF_KEY   ) ? 'K' : '.'
        , ( esw->scl0 & SCL0_ESF_MBPGK ) ? 'G' : '.'
        , ( esw->scl0 & SCL0_ESF_MBDCK ) ? 'D' : '.'
        , ( esw->scl0 & SCL0_ESF_MBPTK ) ? 'P' : '.'
        , ( esw->scl0 & SCL0_ESF_CCWCK ) ? 'C' : '.'
        , ( esw->scl0 & SCL0_ESF_IDACK ) ? 'I' : '.'
        , ( esw->scl0 & 0x01           ) ? '7' : '.'

        , ( esw->scl2 & SCL2_R ) ? " (R)" : ""

        , ( esw->scl2 & SCL2_FVF_LPUM  ) ? 'L' : '.'
        , ( esw->scl2 & SCL2_FVF_TC    ) ? 'T' : '.'
        , ( esw->scl2 & SCL2_FVF_SC    ) ? 'S' : '.'
        , ( esw->scl2 & SCL2_FVF_USTAT ) ? 'D' : '.'
        , ( esw->scl2 & SCL2_FVF_CCWAD ) ? 'C' : '.'

        , esw->lpum

        , sa[  esw->scl2 & SCL2_SA ]

        , tc[ (esw->scl3 & SCL3_TC) >> 6 ]

        , ( esw->scl3 & SCL3_D ) ? 'D' : '.'
        , ( esw->scl3 & SCL3_E ) ? 'E' : '.'
        , ( esw->scl3 & SCL3_A ) ? 'A' : '.'

        , ( esw->scl3 & SCL3_SC )
    );

    return buf;
}


/*-------------------------------------------------------------------*/
/*      Format ESW's Extended-Report Word (ERW) for display          */
/*-------------------------------------------------------------------*/
const char* FormatERW( ESW* esw, char* buf, size_t bufsz )
{
    if (!buf)
        return NULL;
    if (bufsz)
        *buf = 0;
    if (bufsz <= 1 || !esw)
        return buf;

    snprintf( buf, bufsz,

        "Flags:%c%c%c%c%c%c%c%c %c%c SCNT:%d"

        , ( esw->erw0 & ERW0_RSV ) ? '0' : '.'
        , ( esw->erw0 & ERW0_L   ) ? 'L' : '.'
        , ( esw->erw0 & ERW0_E   ) ? 'E' : '.'
        , ( esw->erw0 & ERW0_A   ) ? 'A' : '.'
        , ( esw->erw0 & ERW0_P   ) ? 'P' : '.'
        , ( esw->erw0 & ERW0_T   ) ? 'T' : '.'
        , ( esw->erw0 & ERW0_F   ) ? 'F' : '.'
        , ( esw->erw0 & ERW0_S   ) ? 'S' : '.'

        , ( esw->erw1 & ERW1_C   ) ? 'C' : '.'
        , ( esw->erw1 & ERW1_R   ) ? 'R' : '.'

        , ( esw->erw1 & ERW1_SCNT )
    );

    return buf;
}


/*-------------------------------------------------------------------*/
/*       Format Extended-Status Word (ESW) for display               */
/*-------------------------------------------------------------------*/
const char* FormatESW( ESW* esw, char* buf, size_t bufsz )
{
char scl[64];                               /* Subchannel Logout     */
char erw[64];                               /* Extended-Report Word  */

    if (!buf)
        return NULL;
    if (bufsz)
        *buf = 0;
    if (bufsz <= 1 || !esw)
        return buf;

    snprintf( buf, bufsz,

        "SCL = %s, ERW = %s"

        , FormatSCL( esw, scl, _countof( scl ))
        , FormatERW( esw, erw, _countof( erw ))
    );

    return buf;
}


/*-------------------------------------------------------------------*/
/*      Format SDC (Self Describing Component) information           */
/*-------------------------------------------------------------------*/
static BYTE sdcchar( BYTE c )
{
    /* This  suberfuge  resolved a compiler bug that leads to a slew */
    /* of warnings about c possibly being undefined.                 */
    c = guest_to_host( c );
    return isgraph(c) ? c : '?';
}

const char* FormatSDC( SDC* sdc, char* buf, size_t bufsz )
{
    if (!buf)
        return NULL;
    if (bufsz)
        buf[0] = 0;
    if (bufsz <= 1 || !sdc)
        return buf;

    #define SDCCHAR(fld, n) sdcchar(sdc->fld[n])

    snprintf( buf, bufsz,

        "SDC: type/model:%c%c%c%c%c%c-%c%c%c mfg:%c%c%c plant:%c%c seq/serial:%c%c%c%c%c%c%c%c%c%c%c%c\n"

        , SDCCHAR(type,0),SDCCHAR(type,1),SDCCHAR(type,2),SDCCHAR(type,3),SDCCHAR(type,4),SDCCHAR(type,5)
        , SDCCHAR(model,0),SDCCHAR(model,1),SDCCHAR(model,2)
        , SDCCHAR(mfr,0),SDCCHAR(mfr,1),SDCCHAR(mfr,2)
        , SDCCHAR(plant,0),SDCCHAR(plant,1)
        , SDCCHAR(serial,0),SDCCHAR(serial,1),SDCCHAR(serial,2),SDCCHAR(serial,3),SDCCHAR(serial,4),SDCCHAR(serial,5)
        , SDCCHAR(serial,6),SDCCHAR(serial,7),SDCCHAR(serial,8),SDCCHAR(serial,9),SDCCHAR(serial,10),SDCCHAR(serial,11)
    );

    return buf;
}


/*-------------------------------------------------------------------*/
/*           NEQ (Node-Element Qualifier) type table                 */
/*-------------------------------------------------------------------*/
static const char* NED_NEQ_type[] =
{
    "UNUSED", "NEQ", "GENEQ", "NED",
};


/*-------------------------------------------------------------------*/
/*            Format NED (Node-Element Descriptor)                   */
/*-------------------------------------------------------------------*/
const char* FormatNED( NED* ned, char* buf, size_t bufsz )
{
    const char* typ;
    char bad_typ[4];
    char sdc_info[256];
    static const char* sn_ind[] = { "NEXT", "UNIQUE", "NODE", "CODE3" };
    static const char* ned_type[] = { "UNSPEC", "DEVICE", "CTLUNIT" };
    static const char* dev_class[] =
    {
        "UNKNOWN",
        "DASD",
        "TAPE",
        "READER",
        "PUNCH",
        "PRINTER",
        "COMM",
        "DISPLAY",
        "CONSOLE",
        "CTCA",
        "SWITCH",
        "PROTO",
    };

    if (!buf)
        return NULL;
    if (bufsz)
        buf[0] = 0;
    if (bufsz <= 1 || !ned)
        return buf;

    if (ned->type < _countof( ned_type ))
        typ = ned_type[ ned->type ];
    else
    {
        snprintf( bad_typ, sizeof(bad_typ), "%u", ned->type );
        bad_typ[3] = 0;
        typ = bad_typ;
    }


    if (ned->type == NED_TYP_DEVICE)
    {
        const char* cls;
        char bad_class[4];

        if (ned->cls < _countof( dev_class ))
            cls = dev_class[ ned->cls ];
        else
        {
            snprintf( bad_class, sizeof(bad_class), "%u", ned->cls );
            bad_class[3] = 0;
            cls = bad_class;
        }

        snprintf( buf, bufsz,

            "NED:%s%styp:%s cls:%s lvl:%s sn:%s tag:%02X%02X\n     %s"

            , (ned->flags & 0x20) ? "*" : " "
            , (ned->flags & 0x01) ? "(EMULATED) " : ""
            , typ
            , cls
            , (ned->lvl & 0x01) ? "UNRELATED" : "RELATED"
            , sn_ind[ (ned->flags >> 3) & 0x03 ]
            , ned->tag[0], ned->tag[1]
            , FormatSDC( &ned->info, sdc_info, sizeof(sdc_info))
        );
    }
    else
    {
        snprintf( buf, bufsz,

            "NED:%s%styp:%s lvl:%s sn:%s tag:%02X%02X\n     %s"

            , (ned->flags & 0x20) ? "*" : " "
            , (ned->flags & 0x01) ? "(EMULATED) " : ""
            , typ
            , (ned->lvl & 0x01) ? "UNRELATED" : "RELATED"
            , sn_ind[ (ned->flags >> 3) & 0x03 ]
            , ned->tag[0], ned->tag[1]
            , FormatSDC( &ned->info, sdc_info, sizeof(sdc_info))
        );
    }

    return buf;
}


/*-------------------------------------------------------------------*/
/*            Format NEQ (Node-Element Qualifier)                    */
/*-------------------------------------------------------------------*/
const char* FormatNEQ( NEQ* neq, char* buf, size_t bufsz )
{
    BYTE* byte = (BYTE*) neq;
    U16 iid;

    if (!buf)
        return NULL;
    if (bufsz)
        buf[0] = 0;
    if (bufsz <= 1 || !neq)
        return buf;

    iid = fetch_hw( &neq->iid );

    snprintf( buf, bufsz,

        "NEQ: typ:%s IID:%02X%02X DDTO:%u\n"
        "     %02X%02X%02X%02X %02X%02X%02X%02X\n"
        "     %02X%02X%02X%02X %02X%02X%02X%02X\n"
        "     %02X%02X%02X%02X %02X%02X%02X%02X\n"
        "     %02X%02X%02X%02X %02X%02X%02X%02X\n"

        , NED_NEQ_type[ neq->flags >> 6 ]
        , (BYTE)(iid >> 8), (BYTE)(iid & 0xFF)
        , neq->ddto
        , byte[ 0],byte[ 1],byte[ 2],byte[ 3],  byte[ 4],byte[ 5],byte[ 6],byte[ 7]
        , byte[ 8],byte[ 9],byte[10],byte[11],  byte[12],byte[13],byte[14],byte[15]
        , byte[16],byte[17],byte[18],byte[19],  byte[20],byte[21],byte[22],byte[23]
        , byte[24],byte[25],byte[26],byte[27],  byte[28],byte[29],byte[30],byte[31]
    );

    return buf;
}


/*-------------------------------------------------------------------*/
/*    Helper function to format data as just individual BYTES        */
/*-------------------------------------------------------------------*/
static void FormatBytes( BYTE* data, int len, char* buf, size_t bufsz )
{
    char temp[4];
    int  i;

    for (i=0; i < len; ++i)
    {
        if (i == 4)
            strlcat( buf, " ", bufsz );
        MSGBUF( temp, "%02X", data[i] );
        strlcat( buf, temp, bufsz );
    }
}


/*-------------------------------------------------------------------*/
/*        Format RCD (Read Configuration Data) response              */
/*-------------------------------------------------------------------*/
DLL_EXPORT const char* FormatRCD( BYTE* rcd, int len, char* buf, size_t bufsz )
{
    char temp[256];

    if (!buf)
        return NULL;
    if (bufsz)
        buf[0] = 0;
    if (bufsz <= 1 || !rcd || !len)
        return buf;

    for (; len > 0; rcd += sizeof(NED), len -= sizeof(NED))
    {
        if (len < (int)sizeof(NED))
        {
            FormatBytes( rcd, len, buf, bufsz );
            break;
        }

        switch (rcd[0] >> 6)
        {
        case FIELD_IS_NEQ:
        case FIELD_IS_GENEQ:

            FormatNEQ( (NEQ*)rcd, temp, sizeof(temp)-1);
            break;

        case FIELD_IS_NED:

            FormatNED( (NED*)rcd, temp, sizeof(temp)-1);
            break;

        case FIELD_IS_UNUSED:

            snprintf( temp, sizeof(temp), "n/a\n" );
            break;
        }

        strlcat( buf, temp, bufsz );
    }

    RTRIM( buf );

    return buf;
}


/*-------------------------------------------------------------------*/
/*                 Format ND (Node Descriptor)                       */
/*-------------------------------------------------------------------*/
const char* FormatND( ND* nd, char* buf, size_t bufsz )
{
    const char* val;
    const char* cls;
    const char* by3;
    const char* typ;
    char bad_cls[4];
    char sdc_info[256];
    static const char* css_class[] = { "UNKNOWN", "CHPATH", "CTCA" };
    static const char* val_type[] =
    {
        "VALID", "UNSURE", "INVALID", "3", "4", "5", "6", "7",
    };
    static const char* dev_class[] =
    {
        "UNKNOWN",
        "DASD",
        "TAPE",
        "READER",
        "PUNCH",
        "PRINTER",
        "COMM",
        "DISPLAY",
        "CONSOLE",
        "CTCA",
        "SWITCH",
        "PROTO",
    };

    if (!buf)
        return NULL;
    if (bufsz)
        buf[0] = 0;
    if (bufsz <= 1 || !nd)
        return buf;

    val = val_type[ nd->flags >> 5 ];

    switch (nd->flags >> 5)
    {
    case ND_VAL_VALID:
    case ND_VAL_UNSURE:

        cls = NULL;
        if (nd->flags & 0x01)
        {
            typ = "CSS";
            by3 = "CHPID";
            if (nd->cls < _countof( css_class ))
                cls = css_class[ nd->cls ];
        }
        else
        {
            typ = "DEV";
            by3 = (nd->cls == ND_DEV_PROTO) ? "LINK" : "BYTE3";
            if (nd->cls < _countof( dev_class ))
                cls = dev_class[ nd->cls ];
        }
        if (!cls)
        {
            snprintf( bad_cls, sizeof(bad_cls), "%u", nd->cls );
            bad_cls[3] = 0;
            cls = bad_cls;
        }
        snprintf( buf, bufsz,

            "ND:  val:%s typ:%s cls:%s %s:%02X tag:%02X%02X\n     %s"

            , val
            , typ
            , cls
            , by3, nd->ua
            , nd->tag[0], nd->tag[1]
            , FormatSDC( &nd->info, sdc_info, sizeof(sdc_info))
        );
        break;

    case ND_VAL_INVALID:

        snprintf( buf, bufsz, "ND:  val:INVALID\n" );
        break;

    default:

        snprintf( buf, bufsz, "ND:  val:%u (invalid)\n",
            (int)(nd->flags >> 5) );
        break;
    }

    return buf;
}


/*-------------------------------------------------------------------*/
/*                Format NQ (Node Qualifier)                         */
/*-------------------------------------------------------------------*/
const char* FormatNQ( NQ* nq, char* buf, size_t bufsz )
{
    BYTE* byte = (BYTE*) nq;
    static const char* type[] =
    {
        "IIL", "MODEP", "2", "3", "4", "5", "6", "7",
    };

    if (!buf)
        return NULL;
    if (bufsz)
        buf[0] = 0;
    if (bufsz <= 1 || !nq)
        return buf;

    snprintf( buf, bufsz,

        "NQ:  %02X%02X%02X%02X %02X%02X%02X%02X  (typ:%s)\n"
        "     %02X%02X%02X%02X %02X%02X%02X%02X\n"
        "     %02X%02X%02X%02X %02X%02X%02X%02X\n"
        "     %02X%02X%02X%02X %02X%02X%02X%02X\n"

        , byte[ 0],byte[ 1],byte[ 2],byte[ 3],  byte[ 4],byte[ 5],byte[ 6],byte[ 7]
        , type[ nq->flags >> 5 ]
        , byte[ 8],byte[ 9],byte[10],byte[11],  byte[12],byte[13],byte[14],byte[15]
        , byte[16],byte[17],byte[18],byte[19],  byte[20],byte[21],byte[22],byte[23]
        , byte[24],byte[25],byte[26],byte[27],  byte[28],byte[29],byte[30],byte[31]
    );

    return buf;
}


/*-------------------------------------------------------------------*/
/*          Format RNI (Read Node Identifier) response               */
/*-------------------------------------------------------------------*/
DLL_EXPORT const char* FormatRNI( BYTE* rni, int len, char* buf, size_t bufsz )
{
    if (!buf)
        return NULL;
    if (bufsz)
        buf[0] = 0;
    if (bufsz <= 1 || !rni || !len)
        return buf;

    if (len >= (int)sizeof(ND))
    {
        char work[256];

        register ND* nd = (ND*) rni;

        FormatND( nd, work, sizeof(work)-1);
        strlcat( buf, work, bufsz );

        len -= sizeof(ND);
        rni += sizeof(ND);

        if (len >= (int)sizeof(NQ))
        {
            register NQ* nq = (NQ*) rni;

            FormatNQ( nq, work, sizeof(work)-1);
            strlcat( buf, work, bufsz );

            len -= sizeof(NQ);
            rni += sizeof(NQ);

            if (len)
                FormatBytes( rni, len, buf, bufsz );
        }
        else
            FormatBytes( rni, len, buf, bufsz );
    }
    else
        FormatBytes( rni, len, buf, bufsz );

    RTRIM( buf );

    return buf;
}


/*-------------------------------------------------------------------*/
/*           Format CIW (Command Information Word)                   */
/*-------------------------------------------------------------------*/
const char* FormatCIW( BYTE* ciw, char* buf, size_t bufsz )
{
    static const char* type[] =
    {
        "RCD", "SII", "RNI", "3  ", "4  ", "5  ", "6  ", "7  ",
        "8  ", "9  ", "10 ", "11 ", "12 ", "13 ", "14 ", "15 ",
    };

    if (!buf)
        return NULL;
    if (bufsz)
        buf[0] = 0;
    if (bufsz <= 1 || !ciw)
        return buf;

    if ((ciw[0] & 0xC0) == 0x40)
    {
        snprintf( buf, bufsz,

            "CIW: %02X%02X%02X%02X  typ:%s op:%02X len:%u\n"

            , ciw[0], ciw[1], ciw[2], ciw[3]
            , type[ ciw[0] & 0x0F ]
            , ciw[1]
            , fetch_hw( ciw+2 )
        );
    }
    else
    {
        snprintf( buf, bufsz,

            "CIW: %02X%02X%02X%02X  not a CIW\n"

            , ciw[0]
            , ciw[1]
            , ciw[2]
            , ciw[3]
        );
    }

    return buf;
}


/*-------------------------------------------------------------------*/
/*              Format SID (Sense ID) response                       */
/*-------------------------------------------------------------------*/
DLL_EXPORT const char* FormatSID( BYTE* ciw, int len, char* buf, size_t bufsz )
{
    char temp[128];

    if (!buf)
        return NULL;
    if (bufsz)
        buf[0] = 0;
    if (bufsz <= 1 || !ciw || !len)
        return buf;

    if (len < 8)
        FormatBytes( ciw, len, buf, bufsz );
    else
    {
        snprintf( buf, bufsz,

            "%02X CU=%02X%02X-%02X DEV=%02X%02X-%02X %02X\n"

            , ciw[0]
            , ciw[1], ciw[2], ciw[3]
            , ciw[4], ciw[5], ciw[6]
            , ciw[7]
        );

        ciw += 8;
        len -= 8;

        for (; len >= 4; ciw += 4, len -= 4)
        {
            FormatCIW( ciw, temp, sizeof(temp)-1);
            strlcat( buf, temp, bufsz );
        }

        if (len)
            FormatBytes( ciw, len, buf, bufsz );

        RTRIM( buf );
    }

    return buf;
}


/*-------------------------------------------------------------------*/
/*              Format Program Interrupt Name                        */
/*-------------------------------------------------------------------*/
DLL_EXPORT const char* PIC2Name( int pcode )
{
    static const char* pgmintname[] =
    {
        /* 01 */    "Operation exception",
        /* 02 */    "Privileged-operation exception",
        /* 03 */    "Execute exception",
        /* 04 */    "Protection exception",
        /* 05 */    "Addressing exception",
        /* 06 */    "Specification exception",
        /* 07 */    "Data exception",
        /* 08 */    "Fixed-point-overflow exception",
        /* 09 */    "Fixed-point-divide exception",
        /* 0A */    "Decimal-overflow exception",
        /* 0B */    "Decimal-divide exception",
        /* 0C */    "HFP-exponent-overflow exception",
        /* 0D */    "HFP-exponent-underflow exception",
        /* 0E */    "HFP-significance exception",
        /* 0F */    "HFP-floating-point-divide exception",
        /* 10 */    "Segment-translation exception",
        /* 11 */    "Page-translation exception",
        /* 12 */    "Translation-specification exception",
        /* 13 */    "Special-operation exception",
        /* 14 */    "Pseudo-page-fault exception",
        /* 15 */    "Operand exception",
        /* 16 */    "Trace-table exception",
        /* 17 */    "ASN-translation exception",
        /* 18 */    "Transaction constraint exception",
        /* 19 */    "Vector/Crypto operation exception",
        /* 1A */    "Page state exception",
        /* 1B */    "Vector processing exception",
        /* 1C */    "Space-switch event",
        /* 1D */    "Square-root exception",
        /* 1E */    "Unnormalized-operand exception",
        /* 1F */    "PC-translation specification exception",
        /* 20 */    "AFX-translation exception",
        /* 21 */    "ASX-translation exception",
        /* 22 */    "LX-translation exception",
        /* 23 */    "EX-translation exception",
        /* 24 */    "Primary-authority exception",
        /* 25 */    "Secondary-authority exception",
        /* 26 */ /* "Page-fault-assist exception",          */
        /* 26 */    "LFX-translation exception",
        /* 27 */ /* "Control-switch exception",             */
        /* 27 */    "LSX-translation exception",
        /* 28 */    "ALET-specification exception",
        /* 29 */    "ALEN-translation exception",
        /* 2A */    "ALE-sequence exception",
        /* 2B */    "ASTE-validity exception",
        /* 2C */    "ASTE-sequence exception",
        /* 2D */    "Extended-authority exception",
        /* 2E */    "LSTE-sequence exception",
        /* 2F */    "ASTE-instance exception",
        /* 30 */    "Stack-full exception",
        /* 31 */    "Stack-empty exception",
        /* 32 */    "Stack-specification exception",
        /* 33 */    "Stack-type exception",
        /* 34 */    "Stack-operation exception",
        /* 35 */    "Unassigned exception",
        /* 36 */    "Unassigned exception",
        /* 37 */    "Unassigned exception",
        /* 38 */    "ASCE-type exception",
        /* 39 */    "Region-first-translation exception",
        /* 3A */    "Region-second-translation exception",
        /* 3B */    "Region-third-translation exception",
        /* 3C */    "Unassigned exception",
        /* 3D */    "Unassigned exception",
        /* 3E */    "Unassigned exception",
        /* 3F */    "Unassigned exception",
        /* 40 */    "Monitor event"
    };
    int ndx, code = (pcode & 0xFF);
    if (code < 1 || code > (int) _countof( pgmintname ))
        return "Unassigned exception";
    ndx = ((code - 1) & 0x3F);
    return (ndx >= 0 && ndx < (int) _countof( pgmintname )) ?
        pgmintname[ ndx ] : "Unassigned exception";
}


/*-------------------------------------------------------------------*/
/*     Wrapper functions to allow calling ARCH_DEP functions         */
/*                      from non-ARCH_DEP code                       */
/*-------------------------------------------------------------------*/
void alter_display_real_or_abs (REGS *regs, int argc, char *argv[], char *cmdline)
{
    switch(sysblk.arch_mode) {
#if defined(_370)
        case ARCH_370_IDX:
            s370_alter_display_real_or_abs (regs, argc, argv, cmdline); break;
#endif
#if defined(_390)
        case ARCH_390_IDX:
            s390_alter_display_real_or_abs (regs, argc, argv, cmdline); break;
#endif
#if defined(_900)
        case ARCH_900_IDX:
            z900_alter_display_real_or_abs (regs, argc, argv, cmdline); break;
#endif
    }

} /* end function alter_display_real_or_abs */


void alter_display_virt (REGS *iregs, int argc, char *argv[], char *cmdline)
{
 REGS *regs;

    if (iregs->ghostregs)
        regs = iregs;
    else if ((regs = copy_regs(iregs)) == NULL)
        return;

    switch(sysblk.arch_mode) {
#if defined(_370)
        case ARCH_370_IDX:
            s370_alter_display_virt (regs, argc, argv, cmdline); break;
#endif
#if defined(_390)
        case ARCH_390_IDX:
            s390_alter_display_virt (regs, argc, argv, cmdline); break;
#endif
#if defined(_900)
        case ARCH_900_IDX:
            z900_alter_display_virt (regs, argc, argv, cmdline); break;
#endif
    }

    if (!iregs->ghostregs)
        free_aligned( regs );
} /* end function alter_display_virt */


void disasm_stor(REGS *iregs, int argc, char *argv[], char *cmdline)
{
 REGS *regs;

    if (iregs->ghostregs)
        regs = iregs;
    else if ((regs = copy_regs(iregs)) == NULL)
        return;

    switch(regs->arch_mode) {
#if defined(_370)
        case ARCH_370_IDX:
            s370_disasm_stor(regs, argc, argv, cmdline);
            break;
#endif
#if defined(_390)
        case ARCH_390_IDX:
            s390_disasm_stor(regs, argc, argv, cmdline);
            break;
#endif
#if defined(_900)
        case ARCH_900_IDX:
            z900_disasm_stor(regs, argc, argv, cmdline);
            break;
#endif
    }

    if (!iregs->ghostregs)
        free_aligned( regs );
}

/*-------------------------------------------------------------------*/
/*              Execute a Unix or Windows command                    */
/*-------------------------------------------------------------------*/
/* Returns the system command status code                            */
/* look at popen for this in the future                              */
/*-------------------------------------------------------------------*/
int herc_system (char* command)
{
#if HOW_TO_IMPLEMENT_SH_COMMAND == USE_ANSI_SYSTEM_API_FOR_SH_COMMAND

    return system(command);

#elif HOW_TO_IMPLEMENT_SH_COMMAND == USE_W32_POOR_MANS_FORK

  #define  SHELL_CMD_SHIM_PGM   "conspawn "

    int rc = (int)(strlen(SHELL_CMD_SHIM_PGM) + strlen(command) + 1);
    char* pszNewCommandLine = malloc( rc );
    strlcpy( pszNewCommandLine, SHELL_CMD_SHIM_PGM, rc );
    strlcat( pszNewCommandLine, command,            rc );
    rc = w32_poor_mans_fork( pszNewCommandLine, NULL );
    free( pszNewCommandLine );
    return rc;

#elif HOW_TO_IMPLEMENT_SH_COMMAND == USE_FORK_API_FOR_SH_COMMAND

extern char **environ;
int pid, status;

    if (command == 0)
        return 1;

    pid = fork();

    if (pid == -1)
        return -1;

    if (pid == 0)
    {
        char *argv[4];

        /* Redirect stderr (screen) to hercules log task */
        dup2(STDOUT_FILENO, STDERR_FILENO);

        /* Drop ROOT authority (saved uid) */
        SETMODE(TERM);
        DROP_ALL_CAPS();

        argv[0] = "sh";
        argv[1] = "-c";
        argv[2] = command;
        argv[3] = 0;
        execve("/bin/sh", argv, environ);

        _exit(127);
    }

    do
    {
        if (waitpid(pid, &status, 0) == -1)
        {
            if (errno != EINTR)
                return -1;
        } else
            return status;
    } while(1);
#else
  #error 'HOW_TO_IMPLEMENT_SH_COMMAND' not #defined correctly
#endif
} /* end function herc_system */

#endif // !defined(_GEN_ARCH)
