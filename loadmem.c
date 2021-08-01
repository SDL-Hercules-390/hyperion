/* LOADMEM.C    (C) Copyright Roger Bowler, 1999                     */
/*              (C) Copyright Paul Leisy, 2001                       */
/*              (C) Copyright TurboHercules SAS, 2010-2012           */
/*              Load memory functions                                */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#include "hstdinc.h"

#define _LOADMEM_C_
#define _HENGINE_DLL_

#include "hercules.h"

/*-------------------------------------------------------------------*/
/* loadcore filename command - load a core image file                */
/*-------------------------------------------------------------------*/
int loadcore_cmd(int argc, char *argv[], char *cmdline)
{
REGS *regs;

    char   *fname;                      /* -> File name (ASCIIZ)     */
    struct stat statbuff;               /* Buffer for file status    */
    char   *loadaddr;                   /* loadcore memory address   */
    U64     work64;                     /* 64-bit work variable      */
    RADR    aaddr;                      /* Absolute storage address  */
    char    pathname[MAX_PATH];         /* file in host path format  */

    UNREFERENCED(cmdline);

    if (argc < 2)
    {
        WRMSG(HHC02202, "E", argv[0] );
        return -1;
    }

    fname = argv[1];
    hostpath(pathname, fname, sizeof(pathname));

    if (stat(pathname, &statbuff) < 0)
    {
        WRMSG(HHC02219, "E", "stat()", strerror(errno));
        return -1;
    }

    if (argc < 3) aaddr = 0;
    else
    {
        loadaddr = argv[2];

        if (sscanf(loadaddr, "%"SCNx64, &work64) !=1)
        {
            WRMSG(HHC02205, "E", loadaddr, ": invalid address" );
            return -1;
        }
        aaddr = (RADR) work64;
    }

    obtain_lock(&sysblk.cpulock[sysblk.pcpu]);

    if (!IS_CPU_ONLINE(sysblk.pcpu))
    {
        release_lock(&sysblk.cpulock[sysblk.pcpu]);
        WRMSG(HHC00816, "W", PTYPSTR(sysblk.pcpu), sysblk.pcpu, "online");
        return 0;
    }
    regs = sysblk.regs[sysblk.pcpu];

    /* Command is valid only when CPU is stopped */
    if (CPUSTATE_STOPPED != regs->cpustate)
    {
        release_lock(&sysblk.cpulock[sysblk.pcpu]);
        WRMSG(HHC02247, "E");
        return -1;
    }

    // "Loading file %s to location %s"
    {
        char buf1[32];
        MSGBUF( buf1, "%"PRIX64, (U64) aaddr );
        WRMSG(HHC02250, "I", fname, buf1 );
    }

    (void)load_main(fname, aaddr, 1);

    release_lock(&sysblk.cpulock[sysblk.pcpu]);

    // "Operation complete"
    WRMSG(HHC02249, "I");

    return 0;
}


/*-------------------------------------------------------------------*/
/* loadtext filename command - load a text deck file                 */
/*-------------------------------------------------------------------*/
int loadtext_cmd(int argc, char *argv[], char *cmdline)
{
    const char blanks[8]     = {"\x40\x40\x40\x40\x40\x40\x40\x40"};


    /* Object deck record format headers                             */
    /* (names chosen not to conflict with future GOFF support)       */

    const char c_ObjectESD[9] = {"\xC5\xE2\xC4\x40\x40\x40\x40\x40\x40"};       // ESD - External Symbol Dictionary
    const char c_ObjectTXT[4]  = {"\xE3\xE7\xE3\x40"};                          // TXT - Text
    const char c_ObjectRLD[9] = {"\xD9\xD3\xC4\x40\x40\x40\x40\x40\x40"};       // RLD - Relocation Dictionary
    const char c_ObjectEND[4]  = {"\xC5\xD5\xC4\x40"};                          // END - End
    const char c_ObjectSYM[9] = {"\xE2\xE8\xD4\x40\x40\x40\x40\x40\x40"};       // SYM - Symbol


    /* Special control directives from VM                            */

    const char c_ObjectCPB[3]  = {"\xC3\xD7\xC2"};                              // CPB - Conditional Page Boundary
    const char c_ObjectDEL[3]  = {"\xC4\xD5\xD3"};                              // DEL - Delete
    const char c_ObjectICS[3]  = {"\xC9\xC3\xE2"};                              // ICS - Include Control Section
    const char c_ObjectLDT[3]  = {"\xD3\xC4\xE3"};                              // LDT - Loader Termination
    const char c_ObjectPAD[3]  = {"\xD7\xC1\xC4"};                              // PAD - Padding
    const char c_ObjectPRT[3]  = {"\xD7\xD8\xE3"};                              // PRT - Printer
    const char c_ObjectPRM[3]  = {"\xD7\xD8\xD4"};                              // PRM - Parameter
    const char c_ObjectREP[3]  = {"\xD9\xC5\xD7"};                              // REP - Replace
    const char c_ObjectSLC[3]  = {"\xE2\xD3\xC3"};                              // SLC - Set Location Counter
    const char c_ObjectSPB[3]  = {"\xE1\xD7\xC2"};                              // SPB - Set Page Boundary
    const char c_ObjectSYS[3]  = {"\xE1\xE8\xE1"};                              // SYS - Subsystem
    const char c_ObjectUPB[3]  = {"\xE4\xD7\xC2"};                              // UPB - Unconditional Page Boundary
    const char c_ObjectVER[3]  = {"\xE5\xC5\xD8"};                              // VER - Verify


    /* Define byte tests for general record types                    */

#define TextRecord              0x02
#define GOFFRecord              0x03
#define CommentRecord           0x5C
#define LoaderBinderRecord      0x40


    /* Define directive tests                                        */

#define ObjectTest(n) (NCMP(c_Object##n, buf+1, sizeof(c_Object##n)))

#define ObjectESD (ObjectTest(ESD) && \
                   NCMP(buf+12, blanks, 2) && \
                   NCMP(buf+64, blanks, 8))

#define ObjectTXT (ObjectTest(TXT) && \
                   NCMP(buf+8, blanks, 2) && \
                   NCMP(buf+12, blanks, 2))

#define ObjectRLD (ObjectTest(RLD) && \
                   NCMP(buf+12, blanks, 4))

#define ObjectEND (ObjectTest(END) && \
                   NCMP(buf+8, blanks, 6) && \
                   NCMP(buf+24, blanks, 4) && \
                   NCMP(buf+71, blanks, 1))

#define ObjectSYM (ObjectTest(SYM) && \
                   NCMP(buf+12, blanks, 4))


#define ObjectCPB (ObjectTest(CPB))
#define ObjectDEL (ObjectTest(DEL))
#define ObjectICS (ObjectTest(ICS))
#define ObjectLDT (ObjectTest(LDT))
#define ObjectPAD (ObjectTest(PAD))
#define ObjectPRT (ObjectTest(PRT))
#define ObjectPRM (ObjectTest(PRM))
#define ObjectREP (ObjectTest(REP))
#define ObjectSLC (ObjectTest(SLC))
#define ObjectSPB (ObjectTest(SPB))
#define ObjectSYS (ObjectTest(SYS))
#define ObjectUPB (ObjectTest(UPB))
#define ObjectVER (ObjectTest(VER))


    char   *fname;                      /* -> File name (ASCIIZ)     */
    char   *loadaddr;                   /* loadcore memory address   */
    U64     work64;                     /* 64-bit work variable      */
    RADR    aaddr;                      /* Absolute storage address  */
    RADR    ahighaddr;                  /* Absolute high address     */
    int     fd;                         /* File descriptor           */
    BYTE    buf[80];                    /* Read buffer               */
    int     recno;                      /* Record number             */
    int     rc = 0;                     /* Return code               */
    int     terminate = 0;              /* Terminate load process    */
    REGS   *regs;
    char    pathname[MAX_PATH];

    UNREFERENCED(cmdline);

    if (argc < 2)
    {
        WRMSG(HHC02202, "E", argv[0] );
        return -1;
    }

    fname = argv[1];

    if (argc < 3)
        aaddr = 0;
    else
    {
        loadaddr = argv[2];

        if (sscanf(loadaddr, "%"SCNx64, &work64) !=1)
        {
            WRMSG(HHC02205, "E", loadaddr, ": invalid address" );
            return -1;
        }
        aaddr = (RADR) work64;

        if (aaddr >= sysblk.mainsize)
        {
            WRMSG( HHC02251, "E" );
            return -1;
        }

        /* Address must be on quadword boundary to maintain alignment */
        if (aaddr & 0x0F)
        {
            WRMSG( HHC02306, "E", argv[0], loadaddr);
            return -1;
        }
    }

    ahighaddr = aaddr;

    obtain_lock(&sysblk.cpulock[sysblk.pcpu]);

    if (!IS_CPU_ONLINE(sysblk.pcpu))
    {
        release_lock(&sysblk.cpulock[sysblk.pcpu]);
        WRMSG(HHC00816, "W", PTYPSTR(sysblk.pcpu), sysblk.pcpu, "online");
        return 0;
    }
    regs = sysblk.regs[sysblk.pcpu];

    if (aaddr > regs->mainlim)
    {
        release_lock(&sysblk.cpulock[sysblk.pcpu]);
        WRMSG(HHC02251, "E");
        return -1;
    }

    /* Command is valid only when CPU is stopped */
    if (CPUSTATE_STOPPED != regs->cpustate)
    {
        release_lock(&sysblk.cpulock[sysblk.pcpu]);
        WRMSG(HHC02247, "E");
        return -1;
    }

    /* Open the specified file name */
    hostpath(pathname, fname, sizeof(pathname));
    if ((fd = HOPEN (pathname, O_RDONLY | O_BINARY)) < 0)
    {
        release_lock(&sysblk.cpulock[sysblk.pcpu]);
        WRMSG(HHC02219,"E", "open()", strerror(errno));
        return -1;
    }

    for ( recno = 1; rc == 0 || terminate; recno++ )
    {
        int rlen;                               /* Record Length */

        /* Read 80 bytes into buffer */
        rlen = read( fd, buf, sizeof(buf) );

        /* Handle file read error conditions */
        if ( rlen < 0 )
        {
            WRMSG(HHC02219,"E", "loadtext read()", strerror(errno));
            rc = -1;
            break;
        }

        /* if record length is not 80; leave */
        else if ( rlen != sizeof(buf) )
        {
            if (rlen > 0)
            {
                WRMSG( HHC02301, "E", argv[0], recno, (int)sizeof(buf) );
                rc = -1;
            }
            break;
        }


        /*************************************************************/
        /*                                                           */
        /*      Process valid Object Deck Records by type (0x02,     */
        /*      0x03, *, and blank)                                  */
        /*                                                           */
        /*************************************************************/

        switch (buf[0])
        {

            case TextRecord:

                /*****************************************************/
                /*                                                   */
                /*   Process valid Object Deck Records by            */
                /*   occurrence priority                             */
                /*                                                   */
                /*   - Columns 73-80 (Deck ID, sequence number,      */
                /*     or both) are ignored for 0x02 compiler        */
                /*     generated records.                            */
                /*                                                   */
                /*****************************************************/

                /* if record is "TXT" then copy bytes to mainstore */
                if (ObjectTXT)
                {
                    u_int n   = ((((buf[5] << 8) | buf[6])<<8) | buf[7]);        // Relative address (positive)
                    u_int len = (buf[10] << 8) | buf[11];                        // Byte count
                    if (len <= 56)                                              // Process if useable count
                    {
                       RADR lastbyte = aaddr + n + len - 1;
                       if (lastbyte >= sysblk.mainsize)                         // Bytes must fit into storage
                       {
                          WRMSG(HHC02307,"E", argv[0], recno, (U64)lastbyte);
                          rc = -1;
                          break;
                       }
                       ahighaddr = MAX(ahighaddr, lastbyte);                    // Keep track of highsest byte used
                       memcpy(regs->mainstor + aaddr + n, &buf[16], len);
                    }
                }

                /* if record is "RLD" then process */
                else if (ObjectRLD)
                {

                    /* Define local macros for relocation tests              */

#define RLD_RELOC_A     ((reloc[4] & 0x30) == 0x00)             // A-constant
#define RLD_RELOC_V     ((reloc[4] & 0x30) == 0x10)             // V-constant
#define RLD_RELOC_Q     ((reloc[4] & 0x30) == 0x20)             // Q-constant
#define RLD_RELOC_CXD   ((reloc[4] & 0x30) == 0x30)             // CXD-constant
#define RLD_RELOC_RI2   ((reloc[4] & 0x7C) == 0x70)             // 2-byte Relative-Immediate reference
#define RLD_RELOC_RI4   ((reloc[4] & 0x7C) == 0x78)             // 4-byte Relative-Immediate reference
#define RLD_RELOC_RI    (RLD_RELOC_RI2 || RLD_RELOC_RI4)        // Relative-Immediate reference
#define RLD_RELOC_NEG   ((reloc[4] & 0x02) == 0x02)             // Negative relocation
#define RLD_RELOC_POS   ((reloc[4] & 0x02) == 0x00)             // Positive relocation


                    U16  count = (buf[10] << 8) | buf[11];
                    BYTE *rstart = buf + 16;            // Start of relocation entries
                    BYTE *rend;                         // End of relocation entries
                    BYTE *reloc;                        // Current relocation entry
                    u_int seglength = 8;                // Set initial segment length
                    U16  resdid;                        // Relocation ESDID
                    U16  pesdid;                        // Position ESDID

                    if (count > 56)
                       continue;                        // Skip if bad count

                    rend = rstart + count;              // End of relocation

                    /* Process Relocation Dictionary Entries                          */

                    /* Note: All relocations are presently considered to only have    */
                    /*       relocation within a single object deck (ESDIDs ignored). */

                    for (reloc = rstart; reloc < rend;)
                    {
                       u_int acl;                       // Address constant length
                       u_int aarel;                     // Load relocation address
                       u_int n;                         // Working length
                       BYTE *vc;                        // Pointer to relocation constant
                       U64 v = 0;                       // Variable contents for relocation


                       /* Load new Relocation ESDID and Position ESDID                      */
                       /* (the first entry on record always contains a R-ESDID and P-ESDID) */

                       if (seglength > 4)
                       {
                          resdid = (reloc[0] << 8) | reloc[1];
                          pesdid = (reloc[2] << 8) | reloc[3];
                       }

                       UNREFERENCED(resdid);  // relocation will be added later
                       UNREFERENCED(pesdid);  // relocation will be added later

                       if (!(RLD_RELOC_Q || RLD_RELOC_CXD))     // Relocate all except Q/CXD-constants
                       {

                          /* Determine address constant length - 1 */

                          if (RLD_RELOC_RI)                        // Relative-Immediate?
                             acl = ((reloc[4] & 0x08) >> 2) + 1;   // Set 2/4-byte Relative-Immediate length
                          else                                     // Else set address constant length
                             acl = ((reloc[4] & 0x40) >> 4) + ((reloc[4] & 0x0C) >> 2);


                          /* Load relocation address */

                          aarel = (((reloc[5] << 8) | reloc[6]) << 8) | reloc[7];


                          /* Relocate address */

                          if ((aaddr + aarel + acl) < sysblk.mainsize)
                          {

                             /* Fetch variable length constant */

                             vc = (BYTE *)regs->mainstor + aaddr + aarel;
                             v = 0;
                             for (n = 0;;)
                             {
                                v |=  vc[n];
                                if (n++ >= acl)
                                   break;
                                v <<= 8;
                             }

                             /* Relocate value */

                             if (RLD_RELOC_POS)         /* Positive relocation */
                                v += aaddr;
                             else                       /* Negative relocation */
                                v = aaddr - v;


                             /* Store variable length constant */

                             for (n = acl;;)
                             {
                                vc[n] = v & 0xFF;
                                if (n-- <= 0)
                                   break;
                                v >>= 8;
                             }
                          }
                          else
                          {
                             WRMSG(HHC02307,"W", argv[0], recno, (RADR)aarel);
                             rc = 4;
                          }
                       }


                       /* Set length based on using last ESDID entries */

                       reloc += seglength = (reloc[4] & 0x01) ? 4 : 8;
                    }

                    /* Undefine local macros for relocation tests            */

#undef RLD_RELOC_A      // A-constant
#undef RLD_RELOC_V      // V-constant
#undef RLD_RELOC_Q      // Q-constant
#undef RLD_RELOC_CXD    // CXD-constant
#undef RLD_RELOC_RI2    // 2-byte Relative-Immediate reference
#undef RLD_RELOC_RI4    // 4-byte Relative-Immediate reference
#undef RLD_RELOC_RI     // Relative-Immediate reference
#undef RLD_RELOC_NEG    // Negative relocation
#undef RLD_RELOC_POS    // Positive relocation

                }

                /* if record is "ESD" then process */
                else if (ObjectESD)
                    continue;    // For now, fill in later

                /* if record is "END" then process */
                else if (ObjectEND)
                {
                    /* Bump to next quadword boundary */
                    aaddr = (ahighaddr + 15) & 0xFFFFF0;

                    // Future processing may be to take start address
                    // from END card and place in PSW

                }

                /* if record is "SYM" then process */
                else if (ObjectSYM)
                    continue;    // For now, fill in later


                /*****************************************************/
                /*                                                   */
                /*      Process VM loader directives                 */
                /*                                                   */
                /*****************************************************/

                /* if record is "SPB" then process */
                else if (ObjectSPB)
                {
                    aaddr = (MAX(aaddr, ahighaddr) + 4093) & 0xFFFFF000;

                    /* Warn if further action may exceed storage */

                    if (aaddr >= sysblk.mainsize)
                    {
                        WRMSG(HHC02307,"W", argv[0], recno, aaddr);
                        rc = 4;
                    }
                }

                /* if record is "LDT" then process and break out of loop */
                else if (ObjectLDT)
                {
                    terminate = 1;      // Terminate process, leave RC alone
                }

                /* If unsupported VM record, just warn */
                else if (ObjectCPB ||
                         ObjectDEL ||
                         ObjectICS ||
                         ObjectPAD ||
                         ObjectPRT ||
                         ObjectPRM ||
                         ObjectREP ||
                         ObjectSLC ||
                         ObjectSYS ||
                         ObjectUPB ||
                         ObjectVER)
                {
                    char msgbuf[4];

                    memset(msgbuf, 0, sizeof(msgbuf));
                    msgbuf[0] = guest_to_host(buf[1]);
                    msgbuf[1] = guest_to_host(buf[2]);
                    msgbuf[2] = guest_to_host(buf[3]);
                    WRMSG( HHC02309, "W", argv[0], recno, msgbuf );
                    rc = 4;
                }


                /*****************************************************/
                /*                                                   */
                /*      Handle unknown loader directive              */
                /*                                                   */
                /*****************************************************/

                /* If unknown object record, complain */
                else
                {
                    char msgbuf[4];

                    memset(msgbuf, 0, sizeof(msgbuf));
                    msgbuf[0] = guest_to_host(buf[1]);
                    msgbuf[1] = guest_to_host(buf[2]);
                    msgbuf[2] = guest_to_host(buf[3]);
                    WRMSG( HHC02302, "W", argv[0], recno, msgbuf );
                    rc = 4;
                }

                /* End of original object format handling */
                break;


            /*********************************************************/
            /*                                                       */
            /*      Process GOFF records                             */
            /*                                                       */
            /*********************************************************/

            case GOFFRecord:

                /* Error if GOFF object (not handled yet) */
                WRMSG( HHC02303, "E", argv[0], recno );
                rc = -1;
                break;


            /*********************************************************/
            /*                                                       */
            /*      Process VM comment record                        */
            /*                                                       */
            /*********************************************************/

            case CommentRecord:
                break;              // Ignore record


            /*********************************************************/
            /*                                                       */
            /*      Process Linkage Editor / Loader / Binder         */
            /*      directives                                       */
            /*                                                       */
            /*********************************************************/

            /* Skip if possible loader card, but complain anyways */
            case LoaderBinderRecord:
                WRMSG( HHC02304, "W", argv[0], recno );
                rc = 4;
                break;


            /*********************************************************/
            /*                                                       */
            /*      Process unknown/unsupported records and          */
            /*      directives                                       */
            /*                                                       */
            /*********************************************************/

            default:
                WRMSG( HHC02305, "E", argv[0], recno );
                rc = -1;
        }

    }

    /* Close file and issue status message */
    close (fd);

    if (rc >= 0 && rc < 8)
    {
        if (rc > 0)
            WRMSG(HHC02308, "W", argv[0] );
        // "Operation complete"
        WRMSG(HHC02249, "I");
    }


    release_lock(&sysblk.cpulock[sysblk.pcpu]);

    return rc;

    /* Undefine macros for routine                                   */

#undef TextRecord
#undef GOFFRecord
#undef CommentRecord
#undef LoaderBinderRecord
#undef ObjectTest
#undef ObjectESD
#undef ObjectTXT
#undef ObjectRLD
#undef ObjectEND
#undef ObjectSYM
#undef ObjectCPB
#undef ObjectDEL
#undef ObjectICS
#undef ObjectLDT
#undef ObjectPAD
#undef ObjectPRT
#undef ObjectPRM
#undef ObjectREP
#undef ObjectSLC
#undef ObjectSPB
#undef ObjectSYS
#undef ObjectUPB
#undef ObjectVER

}
