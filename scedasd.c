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

/*-------------------------------------------------------------------*/
/*                non-ARCH_DEP helper functions                      */
/*              compiled only once, the first time                   */
/*-------------------------------------------------------------------*/

#ifndef _SCEDASD_C
#define _SCEDASD_C

static TID    scedio_tid;             /* Thread id of the i/o driver */
static char*  sce_basedir = NULL;     /* base directory for SCE load */

/*-------------------------------------------------------------------*/

char *get_sce_dir()
{
    return sce_basedir;
}

/*-------------------------------------------------------------------*/

void set_sce_dir(char *path)
{
char realdir[MAX_PATH];
char tempdir[MAX_PATH];

    if(sce_basedir)
    {
        free(sce_basedir);
        sce_basedir = NULL;
    }

    if(!path)
        sce_basedir = NULL;
    else
        if(!realpath(path,tempdir))
        {
            WRMSG(HHC00600, "E", path, "realpath()", strerror(errno));
            sce_basedir = NULL;
        }
        else
        {
            hostpath(realdir, tempdir, sizeof(realdir));
            STRLCAT( realdir, PATHSEPS );
            sce_basedir = strdup(realdir);
        }
}

/*-------------------------------------------------------------------*/

static char *set_sce_basedir(char *path)
{
char *basedir;
char realdir[MAX_PATH];
char tempdir[MAX_PATH];

    if(sce_basedir)
    {
        free(sce_basedir);
        sce_basedir = NULL;
    }

    if(!realpath(path,tempdir))
    {
        WRMSG(HHC00600,"E",path,"realpath()",strerror(errno));
        sce_basedir = NULL;
        return NULL;
    }
    hostpath(realdir, tempdir, sizeof(realdir));

    if((basedir = strrchr(realdir,PATHSEPC)))
    {
        *(++basedir) = '\0';
        sce_basedir = strdup(realdir);
        return (basedir = strrchr(path,PATHSEPC)) ? ++basedir : path;
    }
    else
    {
        sce_basedir = NULL;
        return path;
    }
}

/*-------------------------------------------------------------------*/

static char *check_sce_filepath(const char *path, char *fullpath)
{
char temppath[MAX_PATH];
char tempreal[MAX_PATH];

    /* Return file access error if no basedir has been set */
    if(!sce_basedir)
    {
        strlcpy(fullpath,path,sizeof(temppath));
        errno = EACCES;
        return NULL;
    }

    /* Establish the full path of the file we are trying to access */
    STRLCPY( temppath, sce_basedir );
    STRLCAT( temppath, path );

    if(!realpath(temppath,tempreal))
    {
        hostpath(fullpath, tempreal, sizeof(temppath));
        if(strncmp( sce_basedir, fullpath, strlen(sce_basedir)))
            errno = EACCES;
        return NULL;
    }

    hostpath(fullpath, tempreal, sizeof(temppath));
    if(strncmp( sce_basedir, fullpath, strlen(sce_basedir)))
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

#endif /* #ifndef _SCEDASD_C */

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
/*-------------------------------------------------------------------*/
int ARCH_DEP(load_main) (char *fname, RADR startloc, int noisy)
{
U64 loaded;
RADR aaddr;
RADR pageaddr;
int fd;
int pages;
size_t chunk;
int bytes;
time_t begtime, curtime;
char fmt_mem[8];

    fd = HOPEN (fname, O_RDONLY|O_BINARY);
    if (fd < 0)
    {
        if(errno != ENOENT)
            WRMSG(HHC00600,"E",fname,"open()",strerror(errno));
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

        bytes = read(fd, sysblk.mainstor + aaddr, chunk);

        chunk = CHUNKSIZE;

        /* Check for I/O error */
        if (bytes < 0)
        {
            // "SCE file %s: error in function %s: %s"
            WRMSG(HHC00600, "E", fname, "read()",strerror(errno));
            close(fd);
            return -1;
        }

        /* Check for end-of-file */
        if (bytes == 0)
        {
            close(fd);
            return 0;
        }

        /* Update the storage keys for all of the pages we just read */
        pages = ROUND_UP( bytes, PAGEFRAME_PAGESIZE ) / PAGEFRAME_PAGESIZE;
        pageaddr = aaddr;

        do
        {
            STORAGE_KEY(pageaddr, &sysblk) |= STORKEY_REF|STORKEY_CHANGE;
            pageaddr += PAGEFRAME_PAGESIZE;
        }
        while (--pages);

        aaddr += bytes;
        aaddr &= PAGEFRAME_PAGEMASK;

        /* Check if end of storge reached */
        if (aaddr >= sysblk.mainsize)
        {
            int rc;
            if (read(fd, &rc, 1) > 0)
            {
                rc = +1;
                if (noisy)
                {
                    // "SCE file %s: load main terminated at end of mainstor"
                    WRMSG(HHC00603, "W", fname);
                }
            }
            else /* ignore any error; we're at end of storage anyway */
                rc = 0;
            close(fd);
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
/* function load_hmc simulates the load from the service processor   */
/*   the filename pointed to is a descriptor file which has the      */
/*   following format:                                               */
/*                                                                   */
/*   '*' in col 1 is comment                                         */
/*   core image file followed by address where it should be loaded   */
/*                                                                   */
/* For example:                                                      */
/*                                                                   */
/* * Linux/390 cdrom boot image                                      */
/* boot_images/tapeipl.ikr 0x00000000                                */
/* boot_images/initrd 0x00800000                                     */
/* boot_images/parmfile 0x00010480                                   */
/*                                                                   */
/* The location of the image files is relative to the location of    */
/* the descriptor file.                         Jan Jaeger 10-11-01  */
/*                                                                   */
/*-------------------------------------------------------------------*/
int ARCH_DEP(load_hmc) (char *fname, int cpu, int clear)
{
REGS   *regs;                           /* -> Regs                   */
FILE   *fp;
char    inputbuff[MAX_PATH];
char   *inputline;
char    filename[MAX_PATH];             /* filename of image file    */
char    pathname[MAX_PATH];             /* pathname of image file    */
U32     fileaddr;
int     rc = 0;                         /* Return codes (work)       */

    /* Get started */
    if (ARCH_DEP(common_load_begin) (cpu, clear) != 0)
        return -1;

    /* The actual IPL proper starts here... */

    regs = sysblk.regs[cpu];    /* Point to IPL CPU's registers */

    if(fname == NULL)                   /* Default ipl from DASD     */
        fname = "HERCULES.ins";         /*   from HERCULES.ins       */

    hostpath(pathname, fname, sizeof(pathname));

    if(!(fname = set_sce_basedir(pathname)))
        return -1;

    /* Construct and check full pathname */
    if(!check_sce_filepath(fname,filename))
    {
        WRMSG(HHC00601,"E",fname,strerror(errno));
        return -1;
    }

    fp = fopen(filename, "r");
    if(fp == NULL)
    {
        WRMSG(HHC00600,"E", fname,"fopen()",strerror(errno));
        return -1;
    }

    do
    {
        inputline = fgets(inputbuff,sizeof(inputbuff),fp);

#if !defined(_MSVC_)
        if(inputline && *inputline == 0x1a)
            inputline = NULL;
#endif /*!defined(_MSVC_)*/

        if(inputline)
        {
            rc = sscanf(inputline,"%" QSTR(MAX_PATH) "s %i",filename,&fileaddr);
        }

        /* If no load address was found load to location zero */
        if(inputline && rc < 2)
            fileaddr = 0;

        if(inputline && rc > 0 && *filename != '*' && *filename != '#')
        {
            hostpath(pathname, filename, sizeof(pathname));

            /* Construct and check full pathname */
            if(!check_sce_filepath(pathname,filename))
            {
                WRMSG(HHC00602,"E",pathname,strerror(errno));
                return -1;
            }

            if( ARCH_DEP(load_main) (filename, fileaddr, 0) < 0 )
            {
                fclose(fp);
                HDC1(debug_cpu_state, regs);
                return -1;
            }
            sysblk.main_clear = sysblk.xpnd_clear = 0;
        }
    } while(inputline);
    fclose(fp);

    /* Finish up... */
    return ARCH_DEP(common_load_finish) (regs);

} /* end function load_hmc */


#if defined( FEATURE_SCEDIO )

/*-------------------------------------------------------------------*/
/* Function to write to a file on the service processor disk         */
/*-------------------------------------------------------------------*/
static S64 ARCH_DEP(write_file)(char *fname, int mode, CREG sto, S64 size)
{
int fd, nwrite;
U64 totwrite = 0;

    fd = HOPEN (fname, mode |O_WRONLY|O_BINARY,
            S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if (fd < 0)
    {
        WRMSG (HHC00600, "E", fname, "open()", strerror(errno));
        return -1;
    }

#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
    sto &= ASCE_TO;
#else /*!defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/
    sto &= STD_STO;
#endif /*!defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/

    for( ; size > 0 ; sto += sizeof(sto))
    {
#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
    DBLWRD *ste;
#else /*!defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/
    FWORD *ste;
#endif /*!defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/
    CREG pto, pti;

        /* Fetch segment table entry and calc Page Table Origin */
        if( sto >= sysblk.mainsize)
            goto eof;
#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
        ste = (DBLWRD*)(sysblk.mainstor + sto);
#else /*!defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/
        ste = (FWORD*)(sysblk.mainstor + sto);
#endif /*!defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/
        FETCH_W(pto, ste);
        STORAGE_KEY(sto, &sysblk) |= (STORKEY_REF);
        if( pto & SEGTAB_INVALID )
            goto eof;
#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
        pto &= ZSEGTAB_PTO;
#else /*!defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/
        pto &= SEGTAB_PTO;
#endif /*!defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/

        for(pti = 0; pti < 256 && size > 0; pti++, pto += sizeof(pto))
        {
#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
        DBLWRD *pte;
#else /*!defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/
        FWORD *pte;
#endif /*!defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/
        CREG pgo;
        BYTE *page;

            /* Fetch Page Table Entry to get page origin */
            if( pto >= sysblk.mainsize)
                goto eof;

#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
            pte = (DBLWRD*)(sysblk.mainstor + pto);
#else /*!defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/
            pte = (FWORD*)(sysblk.mainstor + pto);
#endif /*!defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/
            FETCH_W(pgo, pte);
            STORAGE_KEY(pto, &sysblk) |= (STORKEY_REF);
            if( !(pgo & PAGETAB_INVALID) )
            {
#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
                pgo &= ZPGETAB_PFRA;
#else /*!defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/
                pgo &= PAGETAB_PFRA;
#endif /*!defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/

                /* Write page to SCE disk */
                if( pgo >= sysblk.mainsize)
                    goto eof;

                page = sysblk.mainstor + pgo;
                nwrite = write(fd, page, STORAGE_KEY_PAGESIZE);
                totwrite += nwrite;
                if( nwrite != STORAGE_KEY_PAGESIZE )
                    goto eof;

                STORAGE_KEY(pgo, &sysblk) |= (STORKEY_REF);
            }
            size -= STORAGE_KEY_PAGESIZE;
        }
    }
eof:
    close(fd);
    return totwrite;
}


/*-------------------------------------------------------------------*/
/* Function to read from a file on the service processor disk        */
/*-------------------------------------------------------------------*/
static S64 ARCH_DEP(read_file)(char *fname, CREG sto, S64 seek, S64 size)
{
int fd, nread;
U64 totread = 0;

    fd = HOPEN (fname, O_RDONLY|O_BINARY);
    if (fd < 0)
    {
        if(errno != ENOENT)
            WRMSG (HHC00600, "E", fname, "open()", strerror(errno));
        return -1;
    }

    if(lseek(fd, (off_t)seek, SEEK_SET) == (off_t)seek)
    {
#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
        sto &= ASCE_TO;
#else /*!defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/
        sto &= STD_STO;
#endif /*!defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/

        for( ; size > 0 ; sto += sizeof(sto))
        {
#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
        DBLWRD *ste;
#else /*!defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/
        FWORD *ste;
#endif /*!defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/
        CREG pto, pti;

            /* Fetch segment table entry and calc Page Table Origin */
            if( sto >= sysblk.mainsize)
                goto eof;
#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
            ste = (DBLWRD*)(sysblk.mainstor + sto);
#else /*!defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/
            ste = (FWORD*)(sysblk.mainstor + sto);
#endif /*!defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/
            FETCH_W(pto, ste);
            STORAGE_KEY(sto, &sysblk) |= (STORKEY_REF);
            if( pto & SEGTAB_INVALID )
                goto eof;
#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
            pto &= ZSEGTAB_PTO;
#else /*!defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/
            pto &= SEGTAB_PTO;
#endif /*!defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/

            for(pti = 0; pti < 256 && size > 0; pti++, pto += sizeof(pto))
            {
#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
            DBLWRD *pte;
#else /*!defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/
            FWORD *pte;
#endif /*!defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/
            CREG pgo;
            BYTE *page;

                /* Fetch Page Table Entry to get page origin */
                if( pto >= sysblk.mainsize)
                    goto eof;
#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
                pte = (DBLWRD*)(sysblk.mainstor + pto);
#else /*!defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/
                pte = (FWORD*)(sysblk.mainstor + pto);
#endif /*!defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/
                FETCH_W(pgo, pte);
                STORAGE_KEY(pto, &sysblk) |= (STORKEY_REF);
                if( pgo & PAGETAB_INVALID )
                    goto eof;
#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
                pgo &= ZPGETAB_PFRA;
#else /*!defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/
                pgo &= PAGETAB_PFRA;
#endif /*!defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)*/

                /* Read page into main storage */
                if( pgo >= sysblk.mainsize)
                    goto eof;
                page = sysblk.mainstor + pgo;
                nread = read(fd, page, STORAGE_KEY_PAGESIZE);
                totread += nread;
                size -= nread;
                if( nread != STORAGE_KEY_PAGESIZE )
                    goto eof;
                STORAGE_KEY(pgo, &sysblk) |= (STORKEY_REF|STORKEY_CHANGE);
            }
        }
    }
eof:
    close(fd);
    return totread;
}

/*-------------------------------------------------------------------*/

static int ARCH_DEP(scedio_ior)(SCCB_SCEDIOR_BK *scedior_bk)
{
U32  origin;
char image[9];
S32  size;
unsigned int i;
char filename[MAX_PATH];

    FETCH_FW(origin,scedior_bk->origin);

    /* Convert image filename to null terminated ascii string */
    for(i = 0; i < sizeof(image)-1 && scedior_bk->image[i] != 0x40; i++)
        image[i] = guest_to_host((int)scedior_bk->image[i]);
    image[i] = '\0';

    /* Ensure file access is allowed and within specified directory */
    if(!check_sce_filepath(image,filename))
    {
        if(errno != ENOENT)
            WRMSG (HHC00604, "E", filename, image, strerror(errno));
        return FALSE;
    }

    size = ARCH_DEP(load_main)(filename,origin,0);

    return (size >= 0) ? TRUE : FALSE;
}

/*-------------------------------------------------------------------*/

static int ARCH_DEP(scedio_iov)(SCCB_SCEDIOV_BK *scediov_bk)
{
S64     seek;
S64     length;
S64     totread, totwrite;
U64     sto;
char    fname[MAX_PATH];

    switch(scediov_bk->type) {

    case SCCB_SCEDIOV_TYPE_INIT:
        return TRUE;
        break;


    case SCCB_SCEDIOV_TYPE_READ:
        /* Ensure file access is allowed and within specified directory */
        if(!check_sce_filepath((char*)scediov_bk->filename,fname))
        {
            if(errno != ENOENT)
                WRMSG (HHC00605, "E", fname, strerror(errno));
            return FALSE;
        }
        FETCH_DW(sto,scediov_bk->sto);
        FETCH_DW(seek,scediov_bk->seek);
        FETCH_DW(length,scediov_bk->length);

        totread = ARCH_DEP(read_file)(fname, sto, seek, length);

        if(totread > 0)
        {
            STORE_DW(scediov_bk->length,totread);

            if(totread == length)
                STORE_DW(scediov_bk->ncomp,0);
            else
                STORE_DW(scediov_bk->ncomp,seek+totread);

            return TRUE;
        }
        else
            return FALSE;

        break;


    case SCCB_SCEDIOV_TYPE_CREATE:
    case SCCB_SCEDIOV_TYPE_APPEND:
        /* Ensure file access is allowed and within specified directory */
        if(!check_sce_filepath((char*)scediov_bk->filename,fname))
        {
            if(errno != ENOENT)
                WRMSG (HHC00605, "E", fname, strerror(errno));

            /* A file not found error may be expected for a create request */
            if(!(errno == ENOENT && scediov_bk->type == SCCB_SCEDIOV_TYPE_CREATE))
                return FALSE;
        }
        FETCH_DW(sto,scediov_bk->sto);
        FETCH_DW(length,scediov_bk->length);

        totwrite = ARCH_DEP(write_file)(fname,
          ((scediov_bk->type == SCCB_SCEDIOV_TYPE_CREATE) ? (O_CREAT|O_TRUNC) : O_APPEND), sto, length);

        if(totwrite >= 0)
        {
            STORE_DW(scediov_bk->ncomp,totwrite);
            return TRUE;
        }
        else
            return FALSE;
        break;


    default:
        PTT_ERR("*SERVC",(U32)scediov_bk->type,(U32)scediov_bk->flag1,scediov_bk->flag2);
        return FALSE;

    }
}


/*-------------------------------------------------------------------*/
/* Thread to perform service processor I/O functions                 */
/*-------------------------------------------------------------------*/
static void* ARCH_DEP(scedio_thread)(void* arg)
{
SCCB_SCEDIOV_BK *scediov_bk;
SCCB_SCEDIOR_BK *scedior_bk;
SCCB_SCEDIO_BK  *scedio_bk = (SCCB_SCEDIO_BK*) arg;

    switch(scedio_bk->flag1) {

    case SCCB_SCEDIO_FLG1_IOV:
        scediov_bk = (SCCB_SCEDIOV_BK*)(scedio_bk + 1);
        if( ARCH_DEP(scedio_iov)(scediov_bk) )
            scedio_bk->flag3 |= SCCB_SCEDIO_FLG3_COMPLETE;
        else
            scedio_bk->flag3 &= ~SCCB_SCEDIO_FLG3_COMPLETE;
        break;

    case SCCB_SCEDIO_FLG1_IOR:
        scedior_bk = (SCCB_SCEDIOR_BK*)(scedio_bk + 1);
        if( ARCH_DEP(scedio_ior)(scedior_bk) )
            scedio_bk->flag3 |= SCCB_SCEDIO_FLG3_COMPLETE;
        else
            scedio_bk->flag3 &= ~SCCB_SCEDIO_FLG3_COMPLETE;
        break;

    default:
        PTT_ERR("*SERVC",(U32)scedio_bk->flag0,(U32)scedio_bk->flag1,scedio_bk->flag3);
    }


    OBTAIN_INTLOCK(NULL);

    while(IS_IC_SERVSIG)
    {
        RELEASE_INTLOCK(NULL);
        sched_yield();
        OBTAIN_INTLOCK(NULL);
    }

    sclp_attention(SCCB_EVD_TYPE_SCEDIO);

    scedio_tid = 0;

    RELEASE_INTLOCK(NULL);
    return NULL;
}


/*-------------------------------------------------------------------*/
/* Function to interface with the service processor I/O thread       */
/*-------------------------------------------------------------------*/
static int ARCH_DEP(scedio_request)(U32 sclp_command, SCCB_EVD_HDR *evd_hdr)
{
SCCB_SCEDIO_BK  *scedio_bk = (SCCB_SCEDIO_BK*)(evd_hdr + 1);
SCCB_SCEDIOV_BK *scediov_bk;
SCCB_SCEDIOR_BK *scedior_bk;
int rc;


static struct {
    SCCB_SCEDIO_BK scedio_bk;
    union {
        SCCB_SCEDIOV_BK v;
        SCCB_SCEDIOR_BK r;
        } io;
    } static_scedio_bk ;

static int scedio_pending;

    if(sclp_command == SCLP_READ_EVENT_DATA)
    {
    int pending_req = scedio_pending;
    U16 evd_len;

        /* Return no data if the scedio thread is still active */
        if(scedio_tid)
            return 0;

        /* Update the scedio_bk copy in the SCCB */
        if(scedio_pending)
        {
            /* Zero all fields */
            memset (evd_hdr, 0, sizeof(SCCB_EVD_HDR));

            /* Set type in event header */
            evd_hdr->type = SCCB_EVD_TYPE_SCEDIO;

            /* Store scedio header */
            *scedio_bk = static_scedio_bk.scedio_bk;

            /* Calculate event response length */
            evd_len = sizeof(SCCB_EVD_HDR) + sizeof(SCCB_SCEDIO_BK);

            switch(scedio_bk->flag1) {
            case SCCB_SCEDIO_FLG1_IOR:
                scedior_bk = (SCCB_SCEDIOR_BK*)(scedio_bk + 1);
                *scedior_bk = static_scedio_bk.io.r;
                evd_len += sizeof(SCCB_SCEDIOR_BK);
                break;
            case SCCB_SCEDIO_FLG1_IOV:
                scediov_bk = (SCCB_SCEDIOV_BK*)(scedio_bk + 1);
                *scediov_bk = static_scedio_bk.io.v ;
                evd_len += sizeof(SCCB_SCEDIOV_BK);
                break;
            default:
                PTT_ERR("*SERVC",(U32)evd_hdr->type,(U32)scedio_bk->flag1,scedio_bk->flag3);
            }

            /* Set length in event header */
            STORE_HW(evd_hdr->totlen, evd_len);
        }


        /* Reset the pending flag */
        scedio_pending = 0;

        /* Return true if a request was pending */
        return pending_req;

    }
    else
    {
        /* Take a copy of the scedio_bk in the SCCB */
        static_scedio_bk.scedio_bk = *scedio_bk;
        switch(scedio_bk->flag1) {
        case SCCB_SCEDIO_FLG1_IOR:
            scedior_bk = (SCCB_SCEDIOR_BK*)(scedio_bk + 1);
            static_scedio_bk.io.r = *scedior_bk;
            break;
        case SCCB_SCEDIO_FLG1_IOV:
            scediov_bk = (SCCB_SCEDIOV_BK*)(scedio_bk + 1);
            static_scedio_bk.io.v = *scediov_bk;
            break;
        default:
            PTT_ERR("*SERVC",(U32)evd_hdr->type,(U32)scedio_bk->flag1,scedio_bk->flag3);
        }

        /* Create the scedio thread */
        rc = create_thread(&scedio_tid, &sysblk.detattr,
            ARCH_DEP(scedio_thread), &static_scedio_bk, "scedio_thread");
        if (rc)
        {
            WRMSG(HHC00102, "E", strerror(rc));
            return -1;
        }
        scedio_pending = 1;
    }

    return 0;
}


/*-------------------------------------------------------------------*/
/* Function to request service processor I/O                         */
/*-------------------------------------------------------------------*/
void ARCH_DEP(sclp_scedio_request) (SCCB_HEADER *sccb)
{
SCCB_EVD_HDR    *evd_hdr = (SCCB_EVD_HDR*)(sccb + 1);

    if( ARCH_DEP(scedio_request)(SCLP_WRITE_EVENT_DATA, evd_hdr) )
    {
        /* Set response code X'0040' in SCCB header */
        sccb->reas = SCCB_REAS_NONE;
        sccb->resp = SCCB_RESP_BACKOUT;
    }
    else
    {
        /* Set response code X'0020' in SCCB header */
        sccb->reas = SCCB_REAS_NONE;
        sccb->resp = SCCB_RESP_COMPLETE;
    }

    /* Indicate Event Processed */
    evd_hdr->flag |= SCCB_EVD_FLAG_PROC;

}


/*-------------------------------------------------------------------*/
/* Function to read service processor I/O event data                 */
/*-------------------------------------------------------------------*/
void ARCH_DEP(sclp_scedio_event) (SCCB_HEADER *sccb)
{
SCCB_EVD_HDR    *evd_hdr = (SCCB_EVD_HDR*)(sccb + 1);
U16 sccb_len;
U16 evd_len;

    if( ARCH_DEP(scedio_request)(SCLP_READ_EVENT_DATA, evd_hdr) )
    {
        /* Update SCCB length field if variable request */
        if (sccb->type & SCCB_TYPE_VARIABLE)
        {
            FETCH_HW(evd_len, evd_hdr->totlen);
            sccb_len = evd_len + sizeof(SCCB_HEADER);
            STORE_HW(sccb->length, sccb_len);
            sccb->type &= ~SCCB_TYPE_VARIABLE;
        }

        /* Set response code X'0020' in SCCB header */
        sccb->reas = SCCB_REAS_NONE;
        sccb->resp = SCCB_RESP_COMPLETE;
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

int load_hmc (char *fname, int cpu, int clear)
{
    switch(sysblk.arch_mode) {
#if defined(_370)
        case ARCH_370_IDX:
            return s370_load_hmc (fname, cpu, clear);
#endif
#if defined(_390)
        case ARCH_390_IDX:
            return s390_load_hmc (fname, cpu, clear);
#endif
#if defined(_900)
        case ARCH_900_IDX:
            /* z/Arch always starts out in ESA390 mode */
            return s390_load_hmc (fname, cpu, clear);
#endif
    }
    return -1;
}


/*-------------------------------------------------------------------*/
/* Load/Read specified file into absolute main storage               */
/*-------------------------------------------------------------------*/
int load_main (char *fname, RADR startloc, int noisy)
{
    switch(sysblk.arch_mode) {
#if defined(_370)
        case ARCH_370_IDX:
            return s370_load_main (fname, startloc, noisy);
#endif
#if defined(_390)
        case ARCH_390_IDX:
            return s390_load_main (fname, startloc, noisy);
#endif
#if defined(_900)
        case ARCH_900_IDX:
            return z900_load_main (fname, startloc, noisy);
#endif
    }
    return -1;
}

#endif /*!defined(_GEN_ARCH)*/
