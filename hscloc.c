/* HSCLOC.C     (C) Copyright TurboHercules SAS, 2010-2011           */
/*              Locate debugging functions                           */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#include "hstdinc.h"

#define _HSCLOC_C_
#define _HENGINE_DLL_

#include "hercules.h"

static char *fmt_decimal( const U64 number )
{
    static  char    fmt_dec[64];
    double  num  = (double)number;
    BYTE    size;
    int     i;

    memset(fmt_dec, 0, sizeof(fmt_dec));

    if ( num > 0 )
    {
        if ( num >= (double)ONE_TRILLION )
        {
            num /= (double)ONE_TRILLION;
            size = 'T';
        }
        else if ( num >= ONE_BILLION )
        {
            num /= (double)ONE_BILLION;
            size = 'G';
        }
        else if ( num >= (double)ONE_MILLION )
        {
            num /= (double)ONE_MILLION;
            size = 'M';
        }
        else
        {
            num /= (double)ONE_THOUSAND;
            size = 'K';
        }

        MSGBUF( fmt_dec, "%7.3f", num );

        i = (int)strlen(fmt_dec);

        if ( i > 0 )
        {
            for ( i--; i > 0; i-- )
            {
                if      ( fmt_dec[i] == '0' ) fmt_dec[i] = '\0';
                else if ( fmt_dec[i] == '.' ) { fmt_dec[i] = '\0'; break; }
                else break;
            }
        }

        fmt_dec[strlen(fmt_dec)] = '\0';
        fmt_dec[strlen(fmt_dec)+1] = '\0';
        fmt_dec[strlen(fmt_dec)+2] = '\0';
        fmt_dec[strlen(fmt_dec)] = ' ';
        fmt_dec[strlen(fmt_dec)] = size;
    }
    else
    {
        MSGBUF( fmt_dec, "%3d ", 0 );
        size = ' ';
    }

    return fmt_dec;
}

void fmt_line( unsigned char *tbl, char *name, int start, int length)
{
    int     i, j, k, l, o;
    char    hbuf[128];
    char    cbuf[64];
    char    fmtline[256];
    BYTE    c;

    l = length < 32 ? length : 32;

    for( o = start; o < (start+length); o += l )
    {
        memset( hbuf, 0, sizeof(hbuf) );
        memset( cbuf, 0, sizeof(cbuf) );

        for (i = 0, j = 0, k = 0; i < l; i++)
        {
            c = tbl[o+i];
            if ( (i & 0x3) == 0x0 ) hbuf[j++] = SPACE;
            if ( (i & 0xf) == 0x0 ) { hbuf[j++] = SPACE; cbuf[k++] = SPACE; }

            j += idx_snprintf( j, hbuf, sizeof(hbuf), "%2.2X", c );
            cbuf[k++] = ( !isprint((unsigned char)c) ? '.' : c );

        } /* end for(i) */
        MSGBUF( fmtline, "%s+0x%04x%-74.74s %-34.34s", name, o, hbuf, cbuf );
        WRMSG( HHC90000, "D", fmtline );
    }

}


/*-------------------------------------------------------------------*/
/* locate - display sysblk [offset [ length ] ]                      */
/*-------------------------------------------------------------------*/
int locate_sysblk( int argc, char* argv[], char* cmdline )
{
    int      rc             = 0;
    char     msgbuf[256];
    int      start          = 0;
    int      start_adj      = 0;
    int      length         = 512;
    u_char*  tbl            = (u_char *)&sysblk;

    UNREFERENCED( cmdline );

    if (argc == 2)
    {
        int ok = TRUE;
        U64 loc = CSWAP64( sysblk.blkloc );

        /* verify head, tail, length and address */

        if (loc != (U64)((uintptr_t) &sysblk ))
        {
            MSGBUF( msgbuf, "SYSBLK moved; was 0x%16.16"PRIX64", is 0x%p",
                loc, &sysblk );
            WRMSG( HHC90000, "D", msgbuf );
            ok = FALSE;
        }

        if (CSWAP32( sysblk.blksiz ) != (U32) sizeof( SYSBLK ))
        {
            MSGBUF( msgbuf, "SYSBLK size wrong; is %u, should be %u",
                CSWAP32( sysblk.blksiz ), (U32) sizeof( SYSBLK ));
            WRMSG( HHC90000, "D", msgbuf );
            ok = FALSE;
        }

        /* verify header */
        {
            char     header[32];
            memset(  header, SPACE, sizeof( header ));
            STRLCPY( header, HDL_NAME_SYSBLK );

            if (strcmp( sysblk.blknam, header ) != 0)
            {
                MSGBUF( msgbuf, "SYSBLK header wrong; is \"%s\", should be \"%s\"",
                    sysblk.blknam, header );
                WRMSG( HHC90000, "D", msgbuf );
                ok = FALSE;
            }
        }

        /* verify version */
        {
            char     version[32];
            memset(  version, SPACE, sizeof( version ));
            STRLCPY( version, HDL_VERS_SYSBLK );

            if (strcmp( sysblk.blkver, version ) != 0)
            {
                MSGBUF( msgbuf, "SYSBLK version wrong; is \"%s\", should be \"%s\"",
                    sysblk.blkver, version );
                WRMSG( HHC90000, "D", msgbuf );
                ok = FALSE;
            }
        }

        /* verify trailer */
        {
            char     trailer[32];
            memset(  trailer, SPACE, sizeof( trailer ));
            STRLCPY( trailer, "END " );
            STRLCAT( trailer, HDL_NAME_SYSBLK );

            if (strcmp( sysblk.blkend, trailer ) != 0)
            {
                MSGBUF( msgbuf, "SYSBLK trailer wrong; is \"%s\", should be \"%s\"",
                    sysblk.blkend, trailer);
                WRMSG( HHC90000, "D", msgbuf );
                ok = FALSE;
            }
        }

        MSGBUF( msgbuf, "SYSBLK @ 0x%p - %sVerified", &sysblk, ok ? "" : "Not " );
        WRMSG( HHC90000, "D", msgbuf );
    }

    if ( argc > 2 )
    {
        /* start offset */
        int     x;
        BYTE    c;

        if ( sscanf(argv[2], "%x%c", &x, &c) != 1  )
        {
            return -1;
        }
        if ( x > (int)sizeof(SYSBLK) )
        {
            return -1;
        }
        start_adj = x % 16;
        start = x - ( x % 16 );      /* round to a 16 byte boundry */
        if ( start + length + start_adj > (int)sizeof(SYSBLK) )
            length = (int)sizeof(SYSBLK) - start;
    }

    if ( argc > 3 )
    {
        /* length */
        int     x;
        BYTE    c;

        if ( sscanf(argv[3], "%x%c", &x, &c) != 1  )
        {
            return -1;
        }
        if ( x > 4096 )
        {
            return -1;
        }
        length = x;
    }

    length += start_adj;
    if ( start + length > (int)sizeof(SYSBLK) )
        length = (int)sizeof(SYSBLK) - start;

    fmt_line( tbl, "sysblk", start, length);

    return rc;
}


/*-------------------------------------------------------------------*/
/* locate - display regs engine [offset [ length ] ] ]              */
/*-------------------------------------------------------------------*/
int locate_regs( int argc, char* argv[], char* cmdline )
{
    int      rc             = 0;
    int      cpu            = sysblk.pcpu;
    char     msgbuf[256];
    int      start          = 0;
    int      start_adj      = 0;
    int      length         = 512;
    REGS*    regs           = sysblk.regs[ cpu ];
    u_char*  tbl            = (u_char*) regs;

    UNREFERENCED( cmdline );

    if (argc == 2)
    {
        int ok = TRUE;
        U64 loc = CSWAP64( regs->blkloc );

        char    blknam[ sizeof( regs->blknam )];
        MSGBUF( blknam, "%-4.4s_%s%02X", HDL_NAME_REGS, PTYPSTR( cpu ), cpu );

        if (!regs)
        {
            WRMSG( HHC90000, "D", "REGS not assigned" );
            return -1;
        }

        /* verify head, tail, length and address */

        if (loc != (U64)((uintptr_t) regs ))
        {
            MSGBUF( msgbuf, "REGS[%2.2X] moved; was 0x%16.16"PRIX64", is 0x%p",
                cpu, loc, regs );
            WRMSG( HHC90000, "D", msgbuf );
            ok = FALSE;
        }

        if (CSWAP32( regs->blksiz ) != (U32) sizeof( REGS ))
        {
            MSGBUF( msgbuf, "REGS[%2.2X] size wrong; is %u, should be %u",
                cpu, CSWAP32( regs->blksiz ), (U32) sizeof( REGS ));
            WRMSG( HHC90000, "D", msgbuf );
            ok = FALSE;
        }

        /* verify header */
        {
            char     header[32];
            memset(  header, SPACE, sizeof( header ));
            STRLCPY( header, blknam );

            if (strcmp( regs->blknam, header ) != 0)
            {
                MSGBUF( msgbuf, "REGS[%2.2X] header wrong; is \"%s\", should be \"%s\"",
                    cpu, regs->blknam, header );
                WRMSG( HHC90000, "D", msgbuf );
                ok = FALSE;
            }
        }

        /* verify version */
        {
            char     version[32];
            memset(  version, SPACE, sizeof( version ));
            STRLCPY( version, HDL_VERS_REGS );

            if (strcmp( regs->blkver, version ) != 0)
            {
                MSGBUF( msgbuf, "REGS[%2.2X] version wrong; is \"%s\", should be \"%s\"",
                    cpu, regs->blkver, version );
                WRMSG( HHC90000, "D", msgbuf );
                ok = FALSE;
            }
        }

        /* verify trailer */
        {
            char     trailer[32];
            memset(  trailer, SPACE, sizeof( trailer ));
            STRLCPY( trailer, "END " );
            STRLCAT( trailer, blknam );

            if (strcmp( regs->blkend, trailer ) != 0)
            {
                MSGBUF( msgbuf, "REGS[%2.2X] trailer wrong; is \"%s\", should be \"%s\"",
                    cpu, regs->blkend, trailer );
                WRMSG( HHC90000, "D", msgbuf );
                ok = FALSE;
            }
        }

        MSGBUF( msgbuf, "REGS[%2.2X] @ 0x%p - %sVerified", cpu, regs, ok ? "" : "Not " );
        WRMSG( HHC90000, "D", msgbuf );
    }

    if ( argc > 2 )
    {
        /* start offset */
        int     x;
        BYTE    c;

        if ( sscanf(argv[2], "%x%c", &x, &c) != 1  )
        {
            return -1;
        }
        if ( x > (int)sizeof(REGS) )
        {
            return -1;
        }
        start_adj = x % 16;
        start = x - ( x % 16 );      /* round to a 16 byte boundry */
        if ( start + length + start_adj > (int)sizeof(REGS) )
            length = (int)sizeof(REGS) - start;
    }

    if ( argc > 3 )
    {
        /* length */
        int     x;
        BYTE    c;

        if ( sscanf(argv[3], "%x%c", &x, &c) != 1  )
        {
            return -1;
        }
        if ( x > 4096 )
        {
            return -1;
        }
        length = x;
    }

    length += start_adj;
    if ( start + length > (int)sizeof(REGS) )
        length = (int)sizeof(REGS) - start;

    fmt_line( tbl, "regs", start, length);

    return rc;
}

/*-------------------------------------------------------------------*/
/* locate - display hostinfo block                                   */
/*-------------------------------------------------------------------*/
int locate_hostinfo( int argc, char* argv[], char* cmdline )
{
    int         rc              = 0;
    char        msgbuf[256];
    HOST_INFO*  pHostInfo       = &hostinfo;
    int         ok              = TRUE;
    U64         loc             = CSWAP64( hostinfo.blkloc );
    char        fmt_mem[8];

    UNREFERENCED( argc );
    UNREFERENCED( argv );
    UNREFERENCED( cmdline );

    init_hostinfo( NULL );      // refresh information

    /* verify head, tail, length and address */

    if (loc != (U64)((uintptr_t) &hostinfo ))
    {
        MSGBUF( msgbuf, "HOSTINFO moved; was 0x%16.16"PRIX64", is 0x%p",
            loc, &hostinfo );
        WRMSG( HHC90000, "D", msgbuf );
        ok = FALSE;
    }

    if (CSWAP32( hostinfo.blksiz ) != (U32) sizeof( HOST_INFO ))
    {
        MSGBUF( msgbuf, "HOSTINFO size wrong; is %u, should be %u",
            CSWAP32( hostinfo.blksiz ), (U32) sizeof( HOST_INFO ));
        WRMSG( HHC90000, "D", msgbuf );
        ok = FALSE;
    }

    /* verify header */
    {
        char     header[32];
        memset(  header, SPACE, sizeof( header ));
        STRLCPY( header, HDL_NAME_HOST_INFO );

        if (strcmp( hostinfo.blknam, header ) != 0)
        {
            MSGBUF( msgbuf, "HOSTINFO header wrong; is \"%s\", should be \"%s\"",
                hostinfo.blknam, header );
            WRMSG( HHC90000, "D", msgbuf );
            ok = FALSE;
        }
    }

    /* verify version */
    {
        char     version[32];
        memset(  version, SPACE, sizeof( version ));
        STRLCPY( version, HDL_VERS_HOST_INFO );

        if (strcmp( hostinfo.blkver, version ) != 0)
        {
            MSGBUF( msgbuf, "HOSTINFO version wrong; is %s, should be %s",
                hostinfo.blkver, version );
            WRMSG( HHC90000, "D", msgbuf );
            ok = FALSE;
        }
    }

    /* verify trailer */
    {
        char     trailer[32];
        memset(  trailer, SPACE, sizeof( trailer ));
        STRLCPY( trailer, "END " );
        STRLCAT( trailer, HDL_NAME_HOST_INFO );

        if (strcmp( hostinfo.blkend, trailer ) != 0)
        {
            MSGBUF( msgbuf, "HOSTINFO trailer wrong; is %s, should be %s",
                hostinfo.blkend, trailer );
            WRMSG( HHC90000, "D", msgbuf );
            ok = FALSE;
        }
    }

    MSGBUF( msgbuf, "HOSTINFO @ 0x%p - %sVerified", &hostinfo, ok ? "" : "Not " );
    WRMSG( HHC90000, "D", msgbuf );

    MSGBUF( msgbuf, "%-17s = %s", "sysname", pHostInfo->sysname );
    WRMSG( HHC90000, "D", msgbuf );

    MSGBUF( msgbuf, "%-17s = %s", "nodename", pHostInfo->nodename );
    WRMSG( HHC90000, "D", msgbuf );

    MSGBUF( msgbuf, "%-17s = %s", "release", pHostInfo->release );
    WRMSG( HHC90000, "D", msgbuf );

    MSGBUF( msgbuf, "%-17s = %s", "curbuild", pHostInfo->curbuild );
    WRMSG( HHC90000, "D", msgbuf );

    MSGBUF( msgbuf, "%-17s = %s", "version", pHostInfo->version );
    WRMSG( HHC90000, "D", msgbuf );

    MSGBUF( msgbuf, "%-17s = %s", "machine", pHostInfo->machine );
    WRMSG( HHC90000, "D", msgbuf );

    MSGBUF( msgbuf, "%-17s = %s", "cpu_brand", pHostInfo->cpu_brand );
    WRMSG( HHC90000, "D", msgbuf );

    MSGBUF( msgbuf, "%-17s = %s", "trycritsec_avail", pHostInfo->trycritsec_avail ? "YES" : "NO" );
    WRMSG( HHC90000, "D", msgbuf );

    MSGBUF( msgbuf, "%-17s = %d", "maxfilesopen", pHostInfo->maxfilesopen );
    WRMSG( HHC90000, "D", msgbuf );

    WRMSG( HHC90000, "D", "" );

    MSGBUF( msgbuf, "%-17s = %3d", "num_procs", pHostInfo->num_procs );
    WRMSG( HHC90000, "D", msgbuf );

    MSGBUF( msgbuf, "%-17s = %3d", "num_packages", pHostInfo->num_packages );
    WRMSG( HHC90000, "D", msgbuf );

    MSGBUF( msgbuf, "%-17s = %3d", "num_physical_cpu", pHostInfo->num_physical_cpu );
    WRMSG( HHC90000, "D", msgbuf );

    MSGBUF( msgbuf, "%-17s = %3d", "num_logical_cpu", pHostInfo->num_logical_cpu );
    WRMSG( HHC90000, "D", msgbuf );

    MSGBUF( msgbuf, "%-17s = %sHz", "bus_speed", fmt_decimal(pHostInfo->bus_speed) );
    WRMSG( HHC90000, "D", msgbuf );

    MSGBUF( msgbuf, "%-17s = %sHz", "cpu_speed", fmt_decimal(pHostInfo->cpu_speed) );
    WRMSG( HHC90000, "D", msgbuf );

    MSGBUF( msgbuf, "%-17s = %s", "vector_unit", pHostInfo->vector_unit ? "YES" : " NO" );
    WRMSG( HHC90000, "D", msgbuf );

    MSGBUF( msgbuf, "%-17s = %s", "fp_unit", pHostInfo->fp_unit ? "YES" : " NO" );
    WRMSG( HHC90000, "D", msgbuf );

    MSGBUF( msgbuf, "%-17s = %s", "cpu_64bits", pHostInfo->cpu_64bits ? "YES" : " NO" );
    WRMSG( HHC90000, "D", msgbuf );

    MSGBUF( msgbuf, "%-17s = %s", "cpu_aes_extns", pHostInfo->cpu_aes_extns ? "YES" : " NO" );
    WRMSG( HHC90000, "D", msgbuf );

    WRMSG( HHC90000, "D", "" );

    MSGBUF( msgbuf, "%-17s = %s", "valid_cache_nums", pHostInfo->valid_cache_nums ? "YES" : " NO" );
    WRMSG( HHC90000, "D", msgbuf );

    MSGBUF( msgbuf, "%-17s = %5"PRIu64" B", "cachelinesz", pHostInfo->cachelinesz );
    WRMSG( HHC90000, "D", msgbuf );

    if ( pHostInfo->L1Dcachesz != 0 )
    {
        MSGBUF( msgbuf, "%-17s = %siB", "L1Dcachesz", fmt_memsize(pHostInfo->L1Dcachesz,fmt_mem,sizeof(fmt_mem)) );
        WRMSG( HHC90000, "D", msgbuf );
    }

    if ( pHostInfo->L1Icachesz != 0 )
    {
        MSGBUF( msgbuf, "%-17s = %siB", "L1Icachesz", fmt_memsize(pHostInfo->L1Icachesz,fmt_mem,sizeof(fmt_mem)) );
        WRMSG( HHC90000, "D", msgbuf );
    }

    if ( pHostInfo->L1Ucachesz != 0 )
    {
        MSGBUF( msgbuf, "%-17s = %siB", "L1Ucachesz", fmt_memsize(pHostInfo->L1Ucachesz,fmt_mem,sizeof(fmt_mem)) );
        WRMSG( HHC90000, "D", msgbuf );
    }

    MSGBUF( msgbuf, "%-17s = %siB", "L2cachesz", fmt_memsize(pHostInfo->L2cachesz,fmt_mem,sizeof(fmt_mem)) );
    WRMSG( HHC90000, "D", msgbuf );

    MSGBUF( msgbuf, "%-17s = %siB", "L3cachesz", fmt_memsize(pHostInfo->L3cachesz,fmt_mem,sizeof(fmt_mem)) );
    WRMSG( HHC90000, "D", msgbuf );

    WRMSG( HHC90000, "D", "" );

    MSGBUF( msgbuf, "%-17s = %siB", "hostpagesz", fmt_memsize(pHostInfo->hostpagesz,fmt_mem,sizeof(fmt_mem)) );
    WRMSG( HHC90000, "D", msgbuf );

    MSGBUF( msgbuf, "%-17s = %siB", "AllocGran", fmt_memsize(pHostInfo->AllocationGranularity,fmt_mem,sizeof(fmt_mem)) );
    WRMSG( HHC90000, "D", msgbuf );

    WRMSG( HHC90000, "D", "" );

    MSGBUF( msgbuf, "%-17s = %siB", "TotalPhys", fmt_memsize(pHostInfo->TotalPhys,fmt_mem,sizeof(fmt_mem)) );
    WRMSG( HHC90000, "D", msgbuf );

    MSGBUF( msgbuf, "%-17s = %siB", "AvailPhys", fmt_memsize(pHostInfo->AvailPhys,fmt_mem,sizeof(fmt_mem)) );
    WRMSG( HHC90000, "D", msgbuf );

    MSGBUF( msgbuf, "%-17s = %siB", "TotalPageFile", fmt_memsize(pHostInfo->TotalPageFile,fmt_mem,sizeof(fmt_mem)) );
    WRMSG( HHC90000, "D", msgbuf );

    MSGBUF( msgbuf, "%-17s = %siB", "AvailPageFile", fmt_memsize(pHostInfo->AvailPageFile,fmt_mem,sizeof(fmt_mem)) );
    WRMSG( HHC90000, "D", msgbuf );

    MSGBUF( msgbuf, "%-17s = %siB", "TotalVirtual", fmt_memsize(pHostInfo->TotalVirtual,fmt_mem,sizeof(fmt_mem)) );
    WRMSG( HHC90000, "D", msgbuf );

    MSGBUF( msgbuf, "%-17s = %siB", "AvailVirtual", fmt_memsize(pHostInfo->AvailVirtual,fmt_mem,sizeof(fmt_mem)) );
    WRMSG( HHC90000, "D", msgbuf );

    return rc;
}

/*-------------------------------------------------------------------*/
/* locate   display control blocks by name                           */
/*-------------------------------------------------------------------*/
int locate_cmd(int argc, char *argv[], char *cmdline)
{
    int rc = 0;

    UNREFERENCED(cmdline);

    if (argc > 1 && CMD(argv[1],sysblk,6))
    {
        rc = locate_sysblk(argc, argv, cmdline);
    }
    else
    if (argc > 1 && CMD(argv[1],regs,4))
    {
        rc = locate_regs(argc, argv, cmdline);
    }
    else
    if (argc > 1 && CMD(argv[1],hostinfo,4))
    {
        rc = locate_hostinfo(argc, argv, cmdline);
    }
    else
    {
        WRMSG( HHC02299, "E", argv[0] );
        rc = -1;
    }

    return rc;
}
