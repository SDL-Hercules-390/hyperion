/* SCEDASD.C    (C) Copyright Jan Jaeger, 1999-2012                  */
/*              Service Control Element DASD I/O functions           */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#include "hstdinc.h"

#define _HENGINE_DLL_

#include "hercules.h"
#include "opcode.h"
#include "inline.h"
#include "service.h"

/*-------------------------------------------------------------------*/
/*                non-ARCH_DEP helper functions                      */
/*              compiled only once, the first time                   */
/*-------------------------------------------------------------------*/

#ifndef _SCEDASD_C
#define _SCEDASD_C

static TID    scedio_tid;             /* Thread id of the i/o driver */
static char*  sce_basedir = NULL;     /* base directory for SCE load */

/*-------------------------------------------------------------------*/

char* get_sce_dir()
{
    return sce_basedir;
}

/*-------------------------------------------------------------------*/

void set_sce_dir( char* path )
{
char realdir[ MAX_PATH ];
char tempdir[ MAX_PATH ];

    if (sce_basedir)
    {
        free( sce_basedir );
        sce_basedir = NULL;
    }

    if (!path)
        sce_basedir = NULL;
    else
    {
        if (!realpath( path, tempdir ))
        {
            // "SCE file %s: error in function %s: %s"
            WRMSG( HHC00600, "E", path, "realpath()", strerror( errno ));
            sce_basedir = NULL;
        }
        else
        {
            hostpath( realdir, tempdir, sizeof( realdir ));
            STRLCAT( realdir, PATHSEPS );
            sce_basedir = strdup( realdir );
        }
    }
}

/*-------------------------------------------------------------------*/

static char* set_sce_basedir( char* path )
{
char* basedir;
char  realdir[ MAX_PATH ];
char  tempdir[ MAX_PATH ];

    if (sce_basedir)
    {
        free( sce_basedir );
        sce_basedir = NULL;
    }

    if (!realpath( path, tempdir ))
    {
        // "SCE file %s: error in function %s: %s"
        WRMSG( HHC00600, "E", path, "realpath()", strerror( errno ));
        sce_basedir = NULL;
        return NULL;
    }

    hostpath( realdir, tempdir, sizeof( realdir ));

    if ((basedir = strrchr( realdir, PATHSEPC )))
    {
        *(++basedir) = '\0';
        sce_basedir = strdup( realdir );
        return (basedir = strrchr( path, PATHSEPC )) ? ++basedir : path;
    }
    else
    {
        sce_basedir = NULL;
        return path;
    }
}

/*-------------------------------------------------------------------*/

static char* check_sce_filepath( const char* path, char* fullpath )
{
char temppath[ MAX_PATH ];
char tempreal[ MAX_PATH ];

    /* Return file access error if no basedir has been set */
    if (!sce_basedir)
    {
        strlcpy( fullpath, path, sizeof( temppath ));
        errno = EACCES;
        return NULL;
    }

    /* Establish the full path of the file we are trying to access */
    STRLCPY( temppath, sce_basedir );
    STRLCAT( temppath, path );

    if (!realpath( temppath, tempreal ))
    {
        hostpath( fullpath, tempreal, sizeof( temppath ));
        if (strncmp( sce_basedir, fullpath, strlen( sce_basedir )))
            errno = EACCES;
        return NULL;
    }

    hostpath( fullpath, tempreal, sizeof( temppath ));

    if (strncmp( sce_basedir, fullpath, strlen( sce_basedir )))
    {
        errno = EACCES;
        return NULL;
    }

    return fullpath;
}

/*-------------------------------------------------------------------*/

#define    CHUNKSIZE   (64 * 1024 * 1024)
CASSERT(   CHUNKSIZE < (INT_MAX - PAGEFRAME_PAGESIZE), scedasd_c );
CASSERT( !(CHUNKSIZE &            PAGEFRAME_BYTEMASK), scedasd_c );

#endif /* _SCEDASD_C */

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
/* Function to Load (read) specified file into absolute main storage */
/* Return value == 0 (success) or non-zero (failure)                 */
/*-------------------------------------------------------------------*/
int ARCH_DEP( load_main )( char* fname, RADR startloc, int noisy )
{
U64     loaded;
RADR    aaddr;
RADR    pageaddr;
int     fd;
int     pages;
size_t  chunk;
int     bytes;
time_t  begtime, curtime;
char    fmt_mem[8];

    fd = HOPEN( fname, O_RDONLY | O_BINARY );

    if (fd < 0)
    {
        if (errno != ENOENT)
            // "SCE file %s: error in function %s: %s"
            WRMSG( HHC00600, "E", fname, "open()", strerror( errno ));
        return fd;
    }

    /* Calculate size of first chunk to reach page boundary */
    chunk = PAGEFRAME_PAGESIZE - (startloc & PAGEFRAME_BYTEMASK);
    aaddr = startloc;

    if (noisy)
    {
        loaded = 0;
        time( &begtime );
    }

    /* Read file into storage until end of file or end of storage */
    for (;;)
    {
        if (chunk > (sysblk.mainsize - aaddr))
            chunk = (sysblk.mainsize - aaddr);

        bytes = read( fd, sysblk.mainstor + aaddr, chunk );

        chunk = CHUNKSIZE;

        /* Check for I/O error */
        if (bytes < 0)
        {
            // "SCE file %s: error in function %s: %s"
            WRMSG( HHC00600, "E", fname, "read()",strerror( errno ));
            close( fd );
            return -1;
        }

        /* Check for end-of-file */
        if (bytes == 0)
        {
            close( fd );
            return 0;
        }

        /* Update the storage keys for all of the pages we just read */
        pages = ROUND_UP( bytes, PAGEFRAME_PAGESIZE ) / PAGEFRAME_PAGESIZE;
        pageaddr = aaddr;

        do
        {
            ARCH_DEP( or_storage_key )( pageaddr, STORKEY_REF | STORKEY_CHANGE );
            pageaddr += PAGEFRAME_PAGESIZE;
        }
        while (--pages);

        aaddr += bytes;
        aaddr &= PAGEFRAME_PAGEMASK;

        /* Check if end of storge reached */
        if (aaddr >= sysblk.mainsize)
        {
            int rc;

            if (read( fd, &rc, 1 ) > 0)
            {
                rc = +1;

                if (noisy)
                {
                    // "SCE file %s: load main terminated at end of mainstor"
                    WRMSG( HHC00603, "W", fname );
                }
            }
            else /* ignore any error; we're at end of storage anyway */
                rc = 0;

            close( fd );
            return rc;
        }

        /* Issue periodic progress messages */
        if (noisy)
        {
            loaded += bytes;
            time( &curtime );

            if (difftime( curtime, begtime ) > 2.0)
            {
                begtime = curtime;

                // "%s bytes %s so far..."
                WRMSG( HHC02317, "I",
                    fmt_memsize( loaded, fmt_mem, sizeof( fmt_mem )),
                        "loaded" );
            }
        }
    } /* end for (;;) */

} /* end function load_main */

/*-------------------------------------------------------------------*/
/*  Function load_hmc simulates the load from the service processor  */
/*-------------------------------------------------------------------*/
/*                                                                   */
/*   The filename pointed to is a descriptor file which has the      */
/*   following format:                                               */
/*                                                                   */
/*   '*' in col 1 is comment                                         */
/*   core image file followed by address where it should be loaded   */
/*                                                                   */
/* For example:                                                      */
/*                                                                   */
/*    * Linux/390 cdrom boot image                                   */
/*    boot_images/tapeipl.ikr 0x00000000                             */
/*    boot_images/initrd 0x00800000                                  */
/*    boot_images/parmfile 0x00010480                                */
/*                                                                   */
/* The location of the image files is relative to the location of    */
/* the descriptor file.                         Jan Jaeger 10-11-01  */
/*                                                                   */
/*-------------------------------------------------------------------*/
int ARCH_DEP( load_hmc )( char* fname, int cpu, int clear )
{
REGS*   regs;                           /* -> Regs                   */
FILE*   fp;
char    inputbuff[MAX_PATH];
char*   inputline;
char    filename[MAX_PATH+1];           /* filename of image file    */
char    pathname[MAX_PATH];             /* pathname of image file    */
U32     fileaddr;
int     rc = 0;                         /* Return codes (work)       */

static const bool noisy =
#if defined(_DEBUG) || defined(DEBUG)
    true;
#else
    false;
#endif

    /* Get started */
    if (ARCH_DEP( common_load_begin )( cpu, clear ) != 0)
        return -1;

    /* The actual IPL proper starts here... */

    regs = sysblk.regs[ cpu ];          /* Point to IPL CPU's regs   */

    if (fname == NULL)                  /* Default ipl from DASD     */
        fname = "HERCULES.ins";         /*   from HERCULES.ins       */

    hostpath( pathname, fname, sizeof( pathname ));

    if (!(fname = set_sce_basedir( pathname )))
        return -1;

    /* Construct and check full pathname */
    if (!check_sce_filepath( fname, filename ))
    {
        // "SCE file %s: load from file failed: %s"
        WRMSG( HHC00601,"E", fname, strerror( errno ));
        return -1;
    }

    fp = fopen( filename, "r" );
    if (fp == NULL)
    {
        // "SCE file %s: error in function %s: %s"
        WRMSG( HHC00600,"E", fname,"fopen()", strerror( errno ));
        return -1;
    }

    do
    {
        inputline = fgets( inputbuff, sizeof( inputbuff ), fp );

#if !defined(_MSVC_)
        if (inputline && *inputline == 0x1a)
            inputline = NULL;
#endif
        if (inputline)
        {
            rc = sscanf( inputline,"%" QSTR( MAX_PATH ) "s %i", filename, &fileaddr );
        }

        /* If no load address was found load to location zero */
        if (inputline && rc < 2)
            fileaddr = 0;

        if (inputline && rc > 0 && *filename != '*' && *filename != '#')
        {
            hostpath( pathname, filename, sizeof( pathname ));

            /* Construct and check full pathname */
            if (!check_sce_filepath( pathname, filename ))
            {
                // "SCE file %s: load from path failed: %s"
                WRMSG( HHC00602,"E",pathname,strerror( errno ));
                return -1;
            }

            if (ARCH_DEP( load_main )( filename, fileaddr, noisy ) < 0)
            {
                // Error!
                fclose( fp );
                HDC1( debug_cpu_state, regs );
                return -1;
            }

            sysblk.main_clear = sysblk.xpnd_clear = 0;
        }
    }
    while (inputline);

    fclose( fp );

    /* Finish up... */
    return ARCH_DEP( common_load_finish )( regs );

} /* end function load_hmc */

#if defined( FEATURE_SCEDIO )

/*-------------------------------------------------------------------*/
/*   Function to write to a file on the service processor disk       */
/*   Returns -1 == error, or total number of bytes written (>= 0)    */
/*-------------------------------------------------------------------*/
static S64 ARCH_DEP( write_file )( char* fname, int mode, CREG sto, S64 size )
{
int  fd, nwrite;
U64  totwrite = 0;

    fd = HOPEN( fname, mode | O_WRONLY | O_BINARY,
            S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH );

    if (fd < 0)
    {
        // "SCE file %s: error in function %s: %s"
        WRMSG( HHC00600, "E", fname, "open()", strerror( errno ));
        return -1;
    }

#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
    sto &= ASCE_TO;
#else
    sto &= STD_STO;
#endif

    for ( ; size > 0 ; sto += sizeof( sto ))
    {
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
    DBLWRD* ste;
#else
    FWORD*  ste;
#endif
    CREG    pto, pti;

        /* Fetch segment table entry and calc Page Table Origin */
        if (sto >= sysblk.mainsize)
            goto eof;

#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
        ste = (DBLWRD*)(sysblk.mainstor + sto);
#else
        ste = (FWORD*) (sysblk.mainstor + sto);
#endif
        FETCH_W( pto, ste );
        ARCH_DEP( or_storage_key )( sto, STORKEY_REF );

        if (pto & SEGTAB_INVALID)
            goto eof;

#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
        pto &= ZSEGTAB_PTO;
#else
        pto &= SEGTAB_PTO;
#endif
        for (pti = 0; pti < 256 && size > 0; pti++, pto += sizeof( pto ))
        {
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
        DBLWRD* pte;
#else
        FWORD*  pte;
#endif
        CREG    pgo;
        BYTE*   page;

            /* Fetch Page Table Entry to get page origin */
            if (pto >= sysblk.mainsize)
                goto eof;

#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
            pte = (DBLWRD*)(sysblk.mainstor + pto);
#else
            pte = (FWORD*) (sysblk.mainstor + pto);
#endif
            FETCH_W( pgo, pte );
            ARCH_DEP( or_storage_key )( pto, STORKEY_REF );

            if (!(pgo & PAGETAB_INVALID))
            {
#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
                pgo &= ZPGETAB_PFRA;
#else
                pgo &= PAGETAB_PFRA;
#endif
                /* Write page to SCE disk */
                if (pgo >= sysblk.mainsize)
                    goto eof;

                page = sysblk.mainstor + pgo;
                nwrite = write( fd, page, STORAGE_KEY_PAGESIZE );
                totwrite += nwrite;

                if (nwrite != STORAGE_KEY_PAGESIZE)
                    goto eof;

                ARCH_DEP( or_storage_key )( pgo, STORKEY_REF );
            }
            size -= STORAGE_KEY_PAGESIZE;
        }
    }
eof:
    close( fd );
    return totwrite;
}

/*-------------------------------------------------------------------*/
/*   Function to read from a file on the service processor disk      */
/*   Returns -1 == error, or total number of bytes read (>= 0)       */
/*-------------------------------------------------------------------*/
static S64 ARCH_DEP( read_file )( char* fname, CREG sto, S64 seek, S64 size )
{
int  fd, nread;
U64  totread = 0;

    fd = HOPEN( fname, O_RDONLY | O_BINARY );

    if (fd < 0)
    {
        if (errno != ENOENT)
            // "SCE file %s: error in function %s: %s"
            WRMSG( HHC00600, "E", fname, "open()", strerror( errno ));
        return -1;
    }

    if (lseek( fd, (off_t)seek, SEEK_SET ) == (off_t) seek)
    {
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
        sto &= ASCE_TO;
#else
        sto &= STD_STO;
#endif
        for ( ; size > 0 ; sto += sizeof( sto ))
        {
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
        DBLWRD* ste;
#else
        FWORD*  ste;
#endif
        CREG    pto, pti;

            /* Fetch segment table entry and calc Page Table Origin */
            if (sto >= sysblk.mainsize)
                goto eof;

#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
            ste = (DBLWRD*)(sysblk.mainstor + sto);
#else
            ste = (FWORD*) (sysblk.mainstor + sto);
#endif
            FETCH_W( pto, ste );
            ARCH_DEP( or_storage_key )( sto, STORKEY_REF );

            if (pto & SEGTAB_INVALID)
                goto eof;

#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
            pto &= ZSEGTAB_PTO;
#else
            pto &= SEGTAB_PTO;
#endif
            for (pti = 0; pti < 256 && size > 0; pti++, pto += sizeof( pto ))
            {
#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
            DBLWRD* pte;
#else
            FWORD*  pte;
#endif
            CREG    pgo;
            BYTE*   page;

                /* Fetch Page Table Entry to get page origin */
                if (pto >= sysblk.mainsize)
                    goto eof;

#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY)
                pte = (DBLWRD*)(sysblk.mainstor + pto );
#else
                pte = (FWORD*) (sysblk.mainstor + pto);
#endif
                FETCH_W( pgo, pte );
                ARCH_DEP( or_storage_key )( pto, STORKEY_REF );

                if (pgo & PAGETAB_INVALID)
                    goto eof;

#if defined( FEATURE_001_ZARCH_INSTALLED_FACILITY )
                pgo &= ZPGETAB_PFRA;
#else
                pgo &= PAGETAB_PFRA;
#endif
                /* Read page into main storage */
                if (pgo >= sysblk.mainsize)
                    goto eof;

                page = sysblk.mainstor + pgo;
                nread = read( fd, page, STORAGE_KEY_PAGESIZE );
                totread += nread;
                size -= nread;

                if (nread != STORAGE_KEY_PAGESIZE)
                    goto eof;

                ARCH_DEP( or_storage_key )( pgo, (STORKEY_REF | STORKEY_CHANGE) );
            }
        }
    }
eof:
    close( fd );
    return totread;
}

/*-------------------------------------------------------------------*/
// (returns boolean true/false success/failure)

static bool ARCH_DEP( scedio_ior )( SCCB_SCEDIO_R_BK* scedio_r_bk )
{
U32           origin;
char          image[9];
unsigned int  i;
char          filename[MAX_PATH];
bool          success;

static const bool noisy =
#if defined(_DEBUG) || defined(DEBUG)
    true;
#else
    false;
#endif

    FETCH_FW( origin, scedio_r_bk->origin );

    /* Convert image filename to null terminated ascii string */
    for (i=0; i < sizeof( image ) - 1 && scedio_r_bk->imagein[i] != 0x40; i++)
        image[i] = guest_to_host( (int)  scedio_r_bk->imagein[i]);
    image[i] = '\0';

    /* Ensure file access is allowed and within specified directory */
    if (!check_sce_filepath( image, filename ))
    {
        if (errno != ENOENT)
            // "SCE file %s: access error on image %s: %s"
            WRMSG( HHC00604, "E", filename, image, strerror( errno ));
        return false;
    }

    success = ARCH_DEP( load_main )( filename, origin, noisy ) == 0;
    return success;
}

/*-------------------------------------------------------------------*/
// (returns boolean true/false success/failure)

static bool ARCH_DEP( scedio_iov )( SCCB_SCEDIO_V_BK* scedio_v_bk )
{
S64     seek;
S64     length;
S64     totread, totwrite;
U64     sto;
char    fname[MAX_PATH];

    switch (scedio_v_bk->type)
    {
    case SCCB_SCEDIOV_TYPE_INIT:

        return true;

    case SCCB_SCEDIOV_TYPE_READ:

        /* Ensure file access is allowed and within specified directory */
        if (!check_sce_filepath( (char*)scedio_v_bk->filename, fname ))
        {
            if (errno != ENOENT)
                // "SCE file %s: access error: %s"
                WRMSG( HHC00605, "E", fname, strerror( errno ));
            return false;
        }

        FETCH_DW( sto,    scedio_v_bk->sto );
        FETCH_DW( seek,   scedio_v_bk->seek );
        FETCH_DW( length, scedio_v_bk->length );

        totread = ARCH_DEP( read_file )( fname, sto, seek, length );

        if (totread > 0)
        {
            STORE_DW( scedio_v_bk->length, totread );

            if (totread == length)
                STORE_DW( scedio_v_bk->ncomp, 0 );
            else
                STORE_DW( scedio_v_bk->ncomp, seek + totread );

            return true;
        }
        else
            return false;

    case SCCB_SCEDIOV_TYPE_CREATE:
    case SCCB_SCEDIOV_TYPE_APPEND:

        /* Ensure file access is allowed and within specified directory */
        if (!check_sce_filepath( (char*)scedio_v_bk->filename, fname ))
        {
            if (errno != ENOENT)
                // "SCE file %s: access error: %s"
                WRMSG( HHC00605, "E", fname, strerror( errno ));

            /* A file not found error may be expected for a create request */
            if (!(errno == ENOENT && scedio_v_bk->type == SCCB_SCEDIOV_TYPE_CREATE))
                return false;
        }

        FETCH_DW( sto,    scedio_v_bk->sto );
        FETCH_DW( length, scedio_v_bk->length );

        totwrite = ARCH_DEP( write_file ) (fname,
            scedio_v_bk->type == SCCB_SCEDIOV_TYPE_CREATE ? (O_CREAT | O_TRUNC) : O_APPEND, sto, length );

        if(totwrite >= 0)
        {
            STORE_DW( scedio_v_bk->ncomp, totwrite );
            return true;
        }
        else
            return false;

    default:
        PTT_ERR("*SERVC", (U32)scedio_v_bk->type, (U32)scedio_v_bk->flag1, scedio_v_bk->flag2 );
        return false;
    }
}

/*-------------------------------------------------------------------*/
/*     Thread to perform service processor I/O functions             */
/*-------------------------------------------------------------------*/
static void* ARCH_DEP( scedio_thread )( void* arg )
{
SCCB_SCEDIO_V_BK* scedio_v_bk;
SCCB_SCEDIO_R_BK* scedio_r_bk;
SCCB_SCEDIO_BK*   scedio_bk  = (SCCB_SCEDIO_BK*) arg;

    switch (scedio_bk->flag1)
    {
    case SCCB_SCEDIO_FLG1_IOV:

        scedio_v_bk = (SCCB_SCEDIO_V_BK*)(scedio_bk + 1);

        if (ARCH_DEP( scedio_iov )( scedio_v_bk ))
            scedio_bk->flag3 |= SCCB_SCEDIO_FLG3_COMPLETE;
        else
            scedio_bk->flag3 &= ~SCCB_SCEDIO_FLG3_COMPLETE;
        break;

    case SCCB_SCEDIO_FLG1_IOR:

        scedio_r_bk = (SCCB_SCEDIO_R_BK*)(scedio_bk + 1);

        if (ARCH_DEP( scedio_ior )( scedio_r_bk ))
        {
            /* Indicate what SCE dasd file we finished reading? */
            memcpy( scedio_r_bk->imageout, scedio_r_bk->imagein, SCCB_SCEDIO_R_IMAGE_LEN );
            scedio_bk->flag3 |= SCCB_SCEDIO_FLG3_COMPLETE;
        }
        else
            scedio_bk->flag3 &= ~SCCB_SCEDIO_FLG3_COMPLETE;
        break;

    default:
        PTT_ERR("*SERVC", (U32)scedio_bk->flag0, (U32)scedio_bk->flag1, scedio_bk->flag3 );
    }

    OBTAIN_INTLOCK( NULL );
    {
        while (IS_IC_SERVSIG)
        {
            RELEASE_INTLOCK( NULL );
            {
                sched_yield();
            }
            OBTAIN_INTLOCK( NULL );
        }

        sclp_attention( SCCB_EVD_TYPE_SCEDIO );
        scedio_tid = 0;
    }
    RELEASE_INTLOCK( NULL );
    return NULL;
}

/*-------------------------------------------------------------------*/
/*   Function to interface with the service processor I/O thread     */
/*-------------------------------------------------------------------*/
/* Returns either zero/non-zero or true/false                        */
/*                                                                   */
/*   SCLP_WRITE_EVENT_DATA:                                          */
/*                                                                   */
/*      zero:      I/O request successfully started                  */
/*      non-zero:  error; request NOT started                        */
/*                                                                   */
/*   SCLP_READ_EVENT_DATA:                                           */
/*                                                                   */
/*      true:      an I/O request WAS pending and has now completed  */
/*      false:     an I/O request was NOT pending                    */
/*                                                                   */
/*-------------------------------------------------------------------*/
static int ARCH_DEP( scedio_request )( U32 sclp_command, SCCB_EVD_HDR* evd_hdr )
{
int rc;
SCCB_SCEDIO_V_BK* scedio_v_bk;
SCCB_SCEDIO_R_BK* scedio_r_bk;
SCCB_SCEDIO_BK*   scedio_bk  = (SCCB_SCEDIO_BK*)(evd_hdr + 1);

static struct
{
    SCCB_SCEDIO_BK  scedio_bk;
    union
    {
        SCCB_SCEDIO_V_BK  v;
        SCCB_SCEDIO_R_BK  r;
    }
    io;
}
static_scedio_bk;

static bool scedio_pending;

    if (sclp_command == SCLP_READ_EVENT_DATA)
    {
        bool pending_req = scedio_pending;
        U16 evd_len;

        /* Return not-completed if the scedio thread is still active */
        if (scedio_tid)
        {
            return 0;
        }

        /* Was there a preceding I/O request to respond to? */
        if (scedio_pending)
        {
            /* Update the scedio_bk copy in the SCCB... */

            /* Zero all event fields */
            memset( evd_hdr, 0, sizeof( SCCB_EVD_HDR ));

            /* Set type in event header */
            evd_hdr->type = SCCB_EVD_TYPE_SCEDIO;

            /* Store copy of original saved SCEDIO header */
            memcpy( scedio_bk, &static_scedio_bk.scedio_bk, sizeof( SCCB_SCEDIO_BK ));

            /* Calculate event response length */
            evd_len = sizeof( SCCB_EVD_HDR ) + sizeof( SCCB_SCEDIO_BK );

            switch (scedio_bk->flag1)
            {
            case SCCB_SCEDIO_FLG1_IOR:

                scedio_r_bk = (SCCB_SCEDIO_R_BK*)(scedio_bk + 1);
                memcpy( scedio_r_bk, &static_scedio_bk.io.r, sizeof( SCCB_SCEDIO_R_BK ));
                evd_len += sizeof( SCCB_SCEDIO_R_BK );
                break;

            case SCCB_SCEDIO_FLG1_IOV:

                scedio_v_bk = (SCCB_SCEDIO_V_BK*)(scedio_bk + 1);
                memcpy( scedio_v_bk, &static_scedio_bk.io.v, sizeof( SCCB_SCEDIO_V_BK ));
                evd_len += sizeof( SCCB_SCEDIO_V_BK );
                break;

            default:
                PTT_ERR("*SERVC",(U32)evd_hdr->type,(U32)scedio_bk->flag1,scedio_bk->flag3);
            }

            /* Set length in event header */
            STORE_HW( evd_hdr->totlen, evd_len );

            /* Indicate I/O request no longer active */
            scedio_pending = false;
        }

        /* Return true if a request was pending */
        return pending_req;
    }

    // else... 'SCLP_WRITE_EVENT_DATA' or 'SCLP_WRITE_EVENT_MASK'

    /* Save copy of original dasd I/O header */
    memcpy( &static_scedio_bk.scedio_bk, scedio_bk, sizeof( SCCB_SCEDIO_BK ));

    switch (scedio_bk->flag1)
    {
    case SCCB_SCEDIO_FLG1_IOR:

        /* Save copy of original dasd I/O block */
        scedio_r_bk = (SCCB_SCEDIO_R_BK*)(scedio_bk + 1);
        memcpy( &static_scedio_bk.io.r, scedio_r_bk, sizeof( SCCB_SCEDIO_R_BK ));
        break;

    case SCCB_SCEDIO_FLG1_IOV:

        /* Save copy of original dasd I/O block */
        scedio_v_bk = (SCCB_SCEDIO_V_BK*)(scedio_bk + 1);
        memcpy( &static_scedio_bk.io.v, scedio_v_bk, sizeof( SCCB_SCEDIO_V_BK ));
        break;

    default:
        PTT_ERR("*SERVC",(U32)evd_hdr->type,(U32)scedio_bk->flag1,scedio_bk->flag3);
    }

    /* Create the scedio thread */
    rc = create_thread( &scedio_tid, &sysblk.detattr,
        ARCH_DEP( scedio_thread ), &static_scedio_bk, "scedio_thread" );

    if (rc)
    {
        // "Error in function create_thread(): %s"
        WRMSG( HHC00102, "E", strerror( rc ));
        return -1;
    }

    /* Remember an I/O request was started and is now running */
    scedio_pending = true;

    return 0;
}

/*-------------------------------------------------------------------*/
/*       Function to request service processor I/O                   */
/*-------------------------------------------------------------------*/
void ARCH_DEP( sclp_scedio_request )( SCCB_HEADER* sccb )
{
    SCCB_EVD_HDR*  evd_hdr  = (SCCB_EVD_HDR*)(sccb + 1);

    if (ARCH_DEP( scedio_request )( SCLP_WRITE_EVENT_DATA, evd_hdr ))
    {
        // non-zero: error: I/O request NOT started...

        /* Set response code X'0040' in SCCB header */
        sccb->reas = SCCB_REAS_NONE;
        sccb->resp = SCCB_RESP_BACKOUT;
    }
    else // zero: I/O request WAS successfully started...
    {
        /* Set response code X'0020' in SCCB header */
        sccb->reas = SCCB_REAS_NONE;
        sccb->resp = SCCB_RESP_COMPLETE;

        /* Event write processed and event read now pending */
        evd_hdr->flag |= SCCB_EVD_FLAG_PROC;
    }
}

/*-------------------------------------------------------------------*/
/*     Function to read service processor I/O event data             */
/*-------------------------------------------------------------------*/
void ARCH_DEP( sclp_scedio_event )( SCCB_HEADER* sccb )
{
SCCB_EVD_HDR*  evd_hdr  = (SCCB_EVD_HDR*)(sccb + 1);
U16            evd_len;
U16            sccb_len;

    if (ARCH_DEP( scedio_request )( SCLP_READ_EVENT_DATA, evd_hdr ))
    {
        // true: an I/O request WAS pending and has now completed

        /* Update SCCB length field if variable request */
        if (sccb->type & SCCB_TYPE_VARIABLE)
        {
            FETCH_HW( evd_len, evd_hdr->totlen );
            sccb_len = evd_len + sizeof( SCCB_HEADER );
            STORE_HW( sccb->length, sccb_len );
            sccb->type &= ~SCCB_TYPE_VARIABLE;
        }

        /* Set response code X'0020' in SCCB header */
        sccb->reas = SCCB_REAS_NONE;
        sccb->resp = SCCB_RESP_COMPLETE;

        /* Event read processed and now no events are pending */
        evd_hdr->flag |= SCCB_EVD_FLAG_PROC;
    }
}

#endif /* defined( FEATURE_SCEDIO ) */

/*-------------------------------------------------------------------*/
/*          (delineates ARCH_DEP from non-arch_dep)                  */
/*-------------------------------------------------------------------*/

#if !defined( _GEN_ARCH )

  #if defined(              _ARCH_NUM_1 )
    #define   _GEN_ARCH     _ARCH_NUM_1
    #include "scedasd.c"
  #endif

  #if defined(              _ARCH_NUM_2 )
    #undef    _GEN_ARCH
    #define   _GEN_ARCH     _ARCH_NUM_2
    #include "scedasd.c"
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
/*  Service Processor Load    (load/ipl from the specified file)     */
/*-------------------------------------------------------------------*/

int load_hmc( char* fname, int cpu, int clear )
{
    switch (sysblk.arch_mode)
    {
#if defined(       _370 )
        case   ARCH_370_IDX:
            return s370_load_hmc (fname, cpu, clear);
#endif
#if defined(       _390 )
        case   ARCH_390_IDX:
            return s390_load_hmc (fname, cpu, clear);
#endif
#if defined(       _900 )
        case   ARCH_900_IDX:
            /* z/Arch always starts out in ESA390 mode */
            return s390_load_hmc (fname, cpu, clear);
#endif
        default: CRASH();
    }
    UNREACHABLE_CODE( return -1 );
}

/*-------------------------------------------------------------------*/
/* Load/Read specified file into absolute main storage               */
/*-------------------------------------------------------------------*/
int load_main( char* fname, RADR startloc, int noisy )
{
    switch (sysblk.arch_mode)
    {
#if defined(       _370 )
        case   ARCH_370_IDX:
            return s370_load_main (fname, startloc, noisy);
#endif
#if defined(       _390 )
        case   ARCH_390_IDX:
            return s390_load_main (fname, startloc, noisy);
#endif
#if defined(       _900 )
        case   ARCH_900_IDX:
            return z900_load_main (fname, startloc, noisy);
#endif
        default: CRASH();
    }
    UNREACHABLE_CODE( return -1 );
}

#endif /* !defined( _GEN_ARCH ) */
