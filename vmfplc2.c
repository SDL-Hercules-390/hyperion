/* VMFPLC2.C  (C) Copyright Ivan S. Warren 2010-2012                 */
/*            (C) Copyright "Fish" (David B. Trout), 2019            */
/*                                                                   */
/*                Hercules VM/CMS VMFPLC2/TAPE Utility               */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*   For usage instructions, refer to either the 'README.VMFPLC2'    */
/*   document or the "vmfplc2.1" man page.                           */

/**********************************************************************

                            CAVEATS

  EDF FST support has not been thoroughly tested. I have not tested
  dumping files (via the CMS vmfplc2 command) from CMS "EDF" format
  disks and loading them onto the host system, nor vice-versa (dump-
  ing from the host system and then loading onto a CMS "EDF" disk).

  There exists evidence (based on review of tools written by others)
  that the CMS 'TAPE' utility also sometimes creates (DUMP command)
  blocks with a "CMSF" or "CMSV" block header depending on whether
  the file being dumped is Fixed or Variable.  I have not seen this
  on VM/370 so I did not add any logic to handle these block types.

  The use of a control file for LOADing tapes DUMPed by CMS onto
  the host system is required in order to know whether or not the
  file being loaded is Text or Binary since CMS does not provide
  such information in its FST.  Knowing this critical as it allows
  vmfplc2 to know whether or not to translate the data from tape
  to ASCII (from EBCDIC) before writing it to the host system and
  allows an *exact* copy of binary files to be transferred between
  the CMS guest and host system.  Text files however, might not be
  transferred exactly due to padding/trimming of trailing blanks
  as well as the ASCII<->EBCDIC translation codepage that is used.

**********************************************************************/

#include "hstdinc.h"

#include "hercules.h"
#include "tapedev.h"
#include "ccwarn.h"

#define UTILITY_NAME    "vmfplc2"
#define UTILITY_DESC    "VM/CMS VMFPLC2/TAPE Utility"

static char* pgm;       /* (initialized by INITIALIZE_UTILITY macro) */

/*-------------------------------------------------------------------*/
/*            Global constants and I/O buffers, etc...               */
/*-------------------------------------------------------------------*/

/*  VMFPLC2  PLC'H'  (FST)   tape block header     .PLCH             */
/*  VMFPLC2  PLC'D'  (data)  tape block header     .PLCD             */
/*  TAPE     CMS' '  (data)  tape block header     .CMS              */
/*  TAPE     CMS'N'  (name)  tape block header     .CMSN             */

#define                      BLK_HDRSIZE   5
static const BYTE  PLCH_hdr[ BLK_HDRSIZE ] = { 0x02, 0xD7, 0xD3, 0xC3, 0xC8 };
static const BYTE  PLCD_hdr[ BLK_HDRSIZE ] = { 0x02, 0xD7, 0xD3, 0xC3, 0xC4 };
static const BYTE  CMS__hdr[ BLK_HDRSIZE ] = { 0x02, 0xC3, 0xD4, 0xE2, 0x40 };
static const BYTE  CMSN_hdr[ BLK_HDRSIZE ] = { 0x02, 0xC3, 0xD4, 0xE2, 0xD5 };

/* NOTE:  Tape blocks will always be 5 bytes more than the
          below values to account for above block headers            */

#define PLC2_BLOCKSIZE        4000    /* size expected by VMFPLC2    */
#define CMS_BLOCKSIZE          800    /* size expected by CMS TAPE   */
#define MAX_PLC2_BLKSIZE     65535    /* max supported tape blksize  */

CASSERT( !(PLC2_BLOCKSIZE % CMS_BLOCKSIZE), vmfplc2_c );

static BYTE bfr [ MAX_PLC2_BLKSIZE ];     /* Primary I/O buffer      */
static BYTE wrk [ MAX_PLC2_BLKSIZE ];     /* host -> guest translate */

/* Valid character for CMS file-names and file-types                 */
static const char* validchars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                "0123456789"
                                "$#@+-:_";

/* Valid CMS file-mode numbers                                       */
static const char* validfmnum = "0123456";

/* Default code page                                                 */
static const char* defaultcp = NULL;

/*-------------------------------------------------------------------*/
/*        Processing options and runtime status information          */
/*-------------------------------------------------------------------*/
struct OPTIONS
{
    const char*  cmd;         /* "vmfplc2.exe"                       */
    const char*  ctlfile;     /* input DUMP control file             */
    const char*  tapefile;    /* input or output (depends on verb)   */

    bool   cms;               /* true == 'TAPE' mode, else 'VMFPLC2' */
    bool   quiet;             /* true == suppress informational msgs */
    bool   comp;              /* true == .het compression, else .aws */
    bool   zlib;              /* true == compress w/zlib, else bzip2 */
    BYTE   level;             /* .het compression level (0..9)       */
    BYTE   verb;              /* LOAD/SCAN/DUMP (see #defines below) */

#define   DUMP_VERB     0
#define   LOAD_VERB     1
#define   SCAN_VERB     2

    DEVBLK devblk;            /* DEVBLK for tape I/O support         */
    BYTE   unitstat;          /* Dummy unitstat for tape I/O support */
};
typedef struct OPTIONS  OPTIONS;

/*-------------------------------------------------------------------*/
/*                    Return verb string                             */
/*-------------------------------------------------------------------*/
static const char* verb( OPTIONS* opts )
{
    if (DUMP_VERB == opts->verb) return "DUMP";
    if (LOAD_VERB == opts->verb) return "LOAD";
    if (SCAN_VERB == opts->verb) return "SCAN";
    return "???";
}

/*-------------------------------------------------------------------*/
/*              Display usage syntax and return -1                   */
/*-------------------------------------------------------------------*/
static int syntax()
{
    WRMSG( HHC02620, "I", pgm );    // "Usage: %s ...
    return -1;
}

/*-------------------------------------------------------------------*/
/*                    Parse the command line                         */
/*-------------------------------------------------------------------*/
/*                                                                   */
/*   Usage: %s [options] VERB <infile> | <ctlfile> <outfile>         */
/*                                                                   */
/*     VERB      desired action               (DUMP/LOAD/SCAN)       */
/*     ctlfile   filename of control file     (DUMP/LOAD only)       */
/*     tapein    filename of input tape file  (LOAD/SCAN only)       */
/*     tapeout   filename of output tape file (DUMP only)            */
/*                                                                   */
/*   Options:                                                        */
/*                                                                   */
/*     -c cp     desired translation codepage                        */
/*     -t        read/write tape in CMS 'TAPE' DUMP format           */
/*     -v        don't suppress certain informational messages       */
/*                                                                   */
/*     -u        create uncompressed .aws output                     */
/*     -z        create compressed .het output using zlib            */
/*     -b        create compressed .het output using bzip2           */
/*     -4        desired compression level (-1, -2 ... -9)           */
/*                                                                   */
/*-------------------------------------------------------------------*/
static int parse_parms( int ac, char* av[], OPTIONS* opts )
{
    int i, argc;

    /* Set default options */
    opts->cmd   = basename( av[0] );
    opts->quiet = true;
    opts->comp  = true;
    opts->zlib  = true;
    opts->level = HETDFLT_LEVEL;
    defaultcp   = query_codepage();

    if (opts->quiet) sysblk.msglvl &= ~MLVL_VERBOSE;
    else             sysblk.msglvl |=  MLVL_VERBOSE;

    /* Parse the options */
    for (argc=ac, i=1; i < argc && '-' == av[i][0]; i++)
    {
        switch (av[i][1])
        {
            case 't':

                opts->cms = true;
                break;

            case 'v':

                opts->quiet = false;
                sysblk.msglvl |= MLVL_VERBOSE;
                break;

            case 'u':

                opts->comp  = false;
                break;

            case 'z':

                opts->comp  = true;
                opts->zlib  = true;
                break;

            case 'b':

                opts->comp  = true;
                opts->zlib  = false;
                break;

            case 'c':

                if (!av[i+1])
                {
                    // "%s not specified"
                    FWRMSG( stderr, HHC02622, "E", "code page" );
                    return syntax();
                }

                if (!valid_codepage_name( av[i+1] ))
                {
                    // "Invalid %s \"%s\""
                    FWRMSG( stderr, HHC02621, "E", "code page", av[i+1] );
                    return syntax();
                }

                defaultcp = av[ ++i ];
                break;

            default:

                if (1
                    &&  av[i][1]
                    &&  av[i][1] >= '1'
                    &&  av[i][1] <= '9'
                    && !av[i][2]
                )
                {
                    opts->comp  = true;
                    opts->level = (av[i][1] & 0x0F);
                    break;
                }

                // "Invalid %s \"%s\""
                FWRMSG( stderr, HHC02621, "E", "option", av[i] );
                return syntax();
        }
    }

    /* Account for options just parsed */
    argc -= (i-1);

    /* Parse the action verb */

    if (argc < 2)
    {
        // "%s not specified"
        FWRMSG( stderr, HHC02622, "E", "VERB" );
        return syntax();
    }

         if (strcasecmp( av[i], "DUMP" ) == 0) opts->verb = DUMP_VERB;
    else if (strcasecmp( av[i], "SCAN" ) == 0) opts->verb = SCAN_VERB;
    else if (strcasecmp( av[i], "LOAD" ) == 0) opts->verb = LOAD_VERB;
    else
    {
        // "Invalid %s \"%s\""
        FWRMSG( stderr, HHC02621, "E", "function", av[i] );
        return syntax();
    }

    i++; /* (get past verb argument) */

    /* Each verb has its own set of required arguments */
    switch (opts->verb)
    {
        case SCAN_VERB:

            if (argc < 3)
            {
                // "%s not specified"
                FWRMSG( stderr, HHC02622, "E", "tape file" );
                return syntax();
            }

            opts->tapefile = av[i++];

            /* Force use of het handler which can read het or aws */
            opts->comp = true;
            break;

        case DUMP_VERB:
        case LOAD_VERB:

            if (argc < 3)
            {
                // "%s not specified"
                FWRMSG( stderr, HHC02622, "E", "ctlfile" );
                return syntax();
            }

            if (argc < 4)
            {
                // "%s not specified"
                FWRMSG( stderr, HHC02622, "E",
                    DUMP_VERB == opts->verb ? "outfile" : "infile" );
                return syntax();
            }

            opts->ctlfile  = av[i++];
            opts->tapefile = av[i++];
            break;
    }

    /* Check for any extraneous (leftover) arguments */
    if (av[i])
    {
        // "Invalid %s \"%s\""
        FWRMSG( stderr, HHC02621, "E", "argument", av[i] );
        return syntax();
    }

    // "%s tape format set to %s"
    WRMSG( HHC02637, "I",
        DUMP_VERB == opts->verb ? "Output" : "Input",
        opts->cms ? "CMS TAPE" : "VMFPLC2" );

    return 0;
}

/*********************************************************************/
/*********************************************************************/
/**                                                                 **/
/**           TAPE MANAGEMENT STRUCTURES AND FUNCTIONS              **/
/**                                                                 **/
/*********************************************************************/
/*********************************************************************/

/*-------------------------------------------------------------------*/
/* the TAPE_BLOCK structure represents a single tape block */
/*-------------------------------------------------------------------*/
typedef struct TAPE_BLOCK   TAPE_BLOCK;

struct TAPE_BLOCK
{
    BYTE*        data;
    size_t       sz;
    TAPE_BLOCK*  next;
};

/*-------------------------------------------------------------------*/
/* The TAPE_BLOCKS structure represents a collection of tape blocks  */
/*-------------------------------------------------------------------*/
struct TAPE_BLOCKS
{
    const BYTE*  hdr;           /* Header to append to every record  */
    size_t       hdrsz;         /* Size of header                    */
    size_t       blksz;         /* Maximum block size                */
    size_t       blk_modulo;    /* Must be a multiple of this        */
    size_t       blk_count;     /* Number of blocks in chain         */
    TAPE_BLOCK*  first;         /* First block in chain              */
    TAPE_BLOCK*  current;       /* Working block                     */
};
typedef struct TAPE_BLOCKS  TAPE_BLOCKS;

/*-------------------------------------------------------------------*/
/*                      Close tape                                   */
/*-------------------------------------------------------------------*/
static void tape_close( OPTIONS* opts )
{
    if (opts->comp)
        close_het( &opts->devblk );
    else
        close_awstape( &opts->devblk );
}

/*-------------------------------------------------------------------*/
/*                       Open tape                                   */
/*-------------------------------------------------------------------*/
static int tape_open( OPTIONS* opts )
{
    int rc;

    /* Initialize SYSBLK and DEVBLK fields for open of tape device */

    sysblk.auto_tape_create = (DUMP_VERB == opts->verb) ? 1 : 0;

    opts->devblk.tdparms.auto_create      =  sysblk.auto_tape_create;
    opts->devblk.tdparms.logical_readonly = !sysblk.auto_tape_create;
    opts->devblk.tdparms.compress         = opts->comp;
    opts->devblk.tdparms.method           = opts->zlib ? HETMETH_ZLIB : HETMETH_BZLIB;
    opts->devblk.tdparms.level            = opts->level;
    opts->devblk.tdparms.chksize          = HETDFLT_CHKSIZE;
    opts->devblk.devtype                  = 0x3590;
    opts->devblk.batch                    = 1; // enable quiet flag
    opts->devblk.quiet                    = opts->quiet;

    STRLCPY( opts->devblk.filename, opts->tapefile );

    if (opts->comp)
    {
        if ((rc = open_het( &opts->devblk, &opts->unitstat, 0 )) < 0)
            // "Error in function %s: %s"
            FWRMSG( stderr, HHC02624, "E", "open_het()", het_error( rc ));
    }
    else
    {
        if ((rc = open_awstape( &opts->devblk, &opts->unitstat, 0 )) < 0)
            // "Error in function %s: %s"
            FWRMSG( stderr, HHC02624, "E", "open_awstape()", strerror( errno ));
    }

    return rc;
}

/*-------------------------------------------------------------------*/
/*                       Rewind tape                                 */
/*-------------------------------------------------------------------*/
static int tape_rewind( OPTIONS* opts )
{
    int rc;

    if (opts->comp)
    {
        if ((rc = rewind_het( &opts->devblk, &opts->unitstat, 0 )) < 0)
            // "Error in function %s: %s"
            FWRMSG( stderr, HHC02624, "E", "rewind_het()", het_error( rc ));
    }
    else
    {
        if ((rc = rewind_awstape( &opts->devblk, &opts->unitstat, 0 )) < 0)
            // "Error in function %s: %s"
            FWRMSG( stderr, HHC02624, "E", "rewind_awstape()", strerror( errno ));
    }

    return rc;
}

/*-------------------------------------------------------------------*/
/*                   Forward Space Block tape                        */
/*-------------------------------------------------------------------*/
static int tape_fsb( OPTIONS* opts )
{
    int rc;

    if (opts->comp)
    {
        if ((rc = fsb_het( &opts->devblk, &opts->unitstat, 0 )) < 0)
            // "Error in function %s: %s"
            FWRMSG( stderr, HHC02624, "E", "fsb_het()", het_error( rc ));
    }
    else
    {
        if ((rc = fsb_awstape( &opts->devblk, &opts->unitstat, 0 )) < 0)
            // "Error in function %s: %s"
            FWRMSG( stderr, HHC02624, "E", "fsb_awstape()", strerror( errno ));
    }

    return rc;
}

/*-------------------------------------------------------------------*/
/*                Retrieve current tape position                     */
/*-------------------------------------------------------------------*/
static int tape_get_position( OPTIONS* opts )
{
    return opts->devblk.blockid;
}

/*-------------------------------------------------------------------*/
/*               Position tape to specific block                     */
/*-------------------------------------------------------------------*/
static int tape_set_position( OPTIONS* opts, U32 blockid )
{
    int rc;

    if ((rc = tape_rewind( opts )) >= 0)
    {
        opts->devblk.curfilen   =  1;
        opts->devblk.nxtblkpos  =  0;
        opts->devblk.prvblkpos  = -1;
        opts->devblk.blockid    =  0;

        while (opts->devblk.blockid < blockid && rc >= 0)
            rc = tape_fsb( opts );
    }

    return rc;
}

/*-------------------------------------------------------------------*/
/*                       Read tape                                   */
/*-------------------------------------------------------------------*/
static int tape_read( OPTIONS* opts )
{
    int rc;

    if (opts->comp)
    {
        if ((rc = read_het( &opts->devblk, bfr, &opts->unitstat, 0 )) < 0)
            // "Error in function %s: %s"
            FWRMSG( stderr, HHC02624, "E", "read_het()", het_error( rc ));
    }
    else
    {
        if ((rc = read_awstape( &opts->devblk, bfr, &opts->unitstat, 0 )) < 0)
            // "Error in function %s: %s"
            FWRMSG( stderr, HHC02624, "E", "read_awstape()", strerror( errno ));
    }

    return rc;
}

/*-------------------------------------------------------------------*/
/*                    Write a single tape mark                       */
/*-------------------------------------------------------------------*/
static int tape_write_mark( OPTIONS* opts )
{
    int rc;

    if (opts->comp)
    {
        if ((rc = write_hetmark( &opts->devblk, &opts->unitstat, 0 )) < 0)
            // "Error in function %s: %s"
            FWRMSG( stderr, HHC02624, "E", "write_hetmark()", het_error( rc ));
    }
    else
    {
        if ((rc = write_awsmark( &opts->devblk, &opts->unitstat, 0 )) < 0)
            // "Error in function %s: %s"
            FWRMSG( stderr, HHC02624, "E", "write_awsmark()", strerror( errno ));
    }

    return rc;
}

/*-------------------------------------------------------------------*/
/*                 Write a block of data to tape                     */
/*-------------------------------------------------------------------*/
int tape_write_block( OPTIONS* opts, const BYTE* bfr, int sz )
{
    int rc;

    if (opts->comp)
    {
        if ((rc =  write_het( &opts->devblk, bfr, sz, &opts->unitstat, 0 )) < 0)
            // "Error in function %s: %s"
            FWRMSG( stderr, HHC02624, "E", "write_het()", het_error( rc ));
    }
    else
    {
        if ((rc = write_awstape( &opts->devblk, bfr, sz, &opts->unitstat, 0 )) < 0)
            // "Error in function %s: %s"
            FWRMSG( stderr, HHC02624, "E", "write_awstape()", strerror( errno ));
    }

    return rc;
}

/*-------------------------------------------------------------------*/
/*    Write TAPE_BLOCK and adjust the block size to a modulo         */
/*                 (without counting the header)                     */
/*-------------------------------------------------------------------*/
static int write_TAPE_BLOCK( OPTIONS* opts, const TAPE_BLOCK* tb, size_t mod, size_t hdrsz )
{
    size_t  sz;

    sz = (tb->sz - hdrsz) / mod;
    if ( (tb->sz - hdrsz) % mod)
        sz++;

    sz *= mod;
    sz += hdrsz;

    return tape_write_block( opts, tb->data, (int) sz );
}

/*-------------------------------------------------------------------*/
/*             Write a collection of blocks to tape                  */
/*-------------------------------------------------------------------*/
static int write_TAPE_BLOCKS( OPTIONS* opts, const TAPE_BLOCKS* tbs)
{
    TAPE_BLOCK* tb;
    int rc;

    tb = tbs->first;

    while (tb)
    {
        if ((rc = write_TAPE_BLOCK( opts, tb, tbs->blk_modulo, tbs->hdrsz )) < 0)
            return rc;
        tb = tb->next;
    }

    return 0;
}

/*-------------------------------------------------------------------*/
/*            Initialize a collection of tape blocks                 */
/*-------------------------------------------------------------------*/
static TAPE_BLOCKS* init_blocks( size_t blksz, size_t modulo, const BYTE* hdr, int hdrsz )
{
    TAPE_BLOCKS*  tbs;

    if (!(tbs = malloc( sizeof( TAPE_BLOCKS ))))
        return NULL;

    tbs->hdr        = hdr;
    tbs->hdrsz      = hdrsz;
    tbs->blksz      = blksz;
    tbs->blk_count  = 0;
    tbs->blk_modulo = modulo;
    tbs->first      = NULL;
    tbs->current    = NULL;

    return tbs;
}

/*-------------------------------------------------------------------*/
/*       Add a chunk of data to a collection of tape blocks          */
/*-------------------------------------------------------------------*/
static int append_data( TAPE_BLOCKS* tbs, const BYTE* bfr, size_t bfr_bytes )
{
    TAPE_BLOCK* tb;
    int bfr_offset;
    int cpy_amount;

    bfr_offset = 0;

    while (bfr_bytes)   /* while bytes remain to be appended */
    {
        /* Allocate a new tape block if we don't have one yet,
           or if the current block is full (no room remains).
        */
        if (0
            || !tbs->current
            ||  tbs->current->sz >= (tbs->hdrsz + tbs->blksz)
        )
        {
            /* Allocate a new TAPE_BLOCK structure */
            if (!(tb = malloc( sizeof( TAPE_BLOCK ))))
                return -1;

            tb->next = NULL;
            tb->sz   = 0;

            /* First one? */
            if (!tbs->first)
            {
                tbs->first   = tb;
                tbs->current = tb;
            }
            else /* No, chain to previous */
            {
                tbs->current->next = tb;
                tbs->current       = tb;
            }

            /* Track number of blocks in chain */
            tbs->blk_count++;

            /* Allocate room for the actual tape block */
            if (!(tb->data = malloc( tbs->hdrsz + tbs->blksz )))
                return -1;

            /* Initialize block header and clear remainder */
            memcpy( tb->data, tbs->hdr, tbs->hdrsz );
            memset( tb->data + tbs->hdrsz, 0, tbs->blksz );

            /* Initialize the current tape blocks size (i.e. how much
               actual data it currently holds: just hdr at the moment)
            */
            tb->sz = tbs->hdrsz; /* (just the header at the moment) */
        }
        else
        {
            /* Otherwise append data to end of current block */
            tb = tbs->current;
        }

        /* Calculate how much room remains in the current tape block */
        cpy_amount = (int) MIN( bfr_bytes, (tbs->hdrsz + tbs->blksz) - tb->sz );

        /* Append as much data as possible to end of current block */
        memcpy( &tb->data[ tb->sz ], &bfr[ bfr_offset ], cpy_amount );

        /* Update current block size, bfr offset and bytes remaining */
        tb->sz     += cpy_amount;
        bfr_offset += cpy_amount;
        bfr_bytes  -= cpy_amount;
    }

    return 0;
}

/*-------------------------------------------------------------------*/
/*              Release a collection of tape blocks                  */
/*-------------------------------------------------------------------*/
static void free_blocks( TAPE_BLOCKS* tbs )
{
    TAPE_BLOCK *tb, *ntb;

    if (!tbs)
        return;

    tb = tbs->first;

    while (tb)
    {
        free( tb->data );
        ntb = tb->next;
        free( tb );
        tb = ntb;
    }

    free( tbs );
}

/*********************************************************************/
/*********************************************************************/
/**                                                                 **/
/**         FILE RECORDS MANAGEMENT STRUCTURES & FUNCTIONS          **/
/**                                                                 **/
/*********************************************************************/
/*********************************************************************/

/*-------------------------------------------------------------------*/
/*    RECS struct:  a file with its collection of tape blocks        */
/*-------------------------------------------------------------------*/
struct RECS
{
    TAPE_BLOCKS*  blocks;
    size_t        filesz;
    int           reccount;
    int           reclen;
    char          recfm;
};
typedef struct RECS     RECS;

/*-------------------------------------------------------------------*/
/*              Initialize a new RECS structure                      */
/*-------------------------------------------------------------------*/
static RECS* initrecs( OPTIONS* opts, char recfm, int recl, const BYTE* hdr, int hdrsz )
{
    RECS*  recs;
    size_t blksz = opts->cms ? CMS_BLOCKSIZE : PLC2_BLOCKSIZE;

    if (!(recs = malloc( sizeof( RECS ))))
        return NULL;

    recs->blocks   = init_blocks( blksz, CMS_BLOCKSIZE, hdr, hdrsz );
    recs->filesz   = 0;
    recs->reccount = 0;
    recs->reclen   = recl;
    recs->recfm    = recfm;

    return recs;
}

/*-------------------------------------------------------------------*/
/* Add 1 (recfm V) or multiple (recfm F) records to a RECS structure */
/* and to the related tape block collection.                         */
/*-------------------------------------------------------------------*/
static int addrecs( RECS* recs, const BYTE* bfr, int sz )
{
    BYTE recd[2];   // (guest BIG-ENDIAN format)
    int rc;

    recs->filesz += sz;

    switch (recs->recfm)
    {
        case 'V': // (variable)

            store_hw( recd, (U16) sz );

            if (0
                || (rc = append_data( recs->blocks, recd,  2 )) != 0
                || (rc = append_data( recs->blocks, bfr,  sz )) != 0
            )
                return rc;

            recs->reccount++;
            recs->reclen = MAX( recs->reclen, sz );
            break;

        case 'F': // (fixed)

            if ((rc = append_data( recs->blocks, bfr, sz )) != 0)
                return rc;
            break;
    }

    return 0;
}

/*-------------------------------------------------------------------*/
/*                    Close a RECS structure.                        */
/*-------------------------------------------------------------------*/
/* The RECS structue is released and the collection of tape blocks   */
/* is returned for subsequent writing.  For empty files, a dummy     */
/* record is created since CMS does not support empty files.  The    */
/* last record of a RECFM F file is also padded if necessary.        */
/*-------------------------------------------------------------------*/
static TAPE_BLOCKS*  flushrecs( RECS* recs,
                                int*     recl,
                                int*     recc,
                                size_t*  filesz,
                                BYTE     pad )
{
    TAPE_BLOCKS* blks;
    int residual;
    int rc;

    if (!recs->filesz)
    {
        if ((rc = append_data( recs->blocks, &pad, 1 )) != 0)
            return NULL;

        recs->filesz = 1;

        if (recs->recfm == 'V')     // (variable)
        {
            recs->reccount = 1;
            recs->reclen   = 1;
        }
    }

    if (recs->recfm == 'F')         // (fixed)
    {
        /* Check if we need to do any padding */
        residual  = recs->reclen - (recs->filesz % recs->reclen);
        residual %= recs->reclen;

        if (residual)  /* (do we need to pad?) */
        {
            BYTE* padbfr;

            if (!(padbfr = malloc( residual )))
                return NULL;

            /* Fill work buffer with requested padding character */
            memset( padbfr, pad, residual );

            /* Add padding to reach proper fixed record size */
            if ((rc = append_data( recs->blocks, padbfr, residual )) != 0)
                return NULL;

            free( padbfr );

            recs->filesz += residual;   /* (fix the file size) */
        }

        recs->reccount = (int) recs->filesz / recs->reclen;
    }

    /* Pass updated values back to caller */

    *recl   = recs->reclen;
    *recc   = recs->reccount;
    *filesz = recs->filesz;

    blks = recs->blocks;

    free( recs );

    return blks;
}

/*-------------------------------------------------------------------*/
/*            Process a binary file for DUMP function                */
/*-------------------------------------------------------------------*/
static TAPE_BLOCKS* dump_binary_file( OPTIONS* opts,
                                      const char* infile, char recfm,
                                      int* recl, int* recc, size_t* filesz )
{
    TAPE_BLOCKS*  blks;
    FILE*         ifile;
    RECS*         recs;
    const BYTE*   hdr;
    int           rsz;

    /* Allocate and initialize a new RECS collection for this file */
    hdr = opts->cms ? CMS__hdr : PLCD_hdr;
    if (!(recs = initrecs( opts, recfm, *recl, hdr, BLK_HDRSIZE )))
        return NULL;

    /* Open this file for reading */
    if (!(ifile = fopen( infile, "rb" )))
    {
        // "File open error: \"%s\": %s"
        FWRMSG( stderr, HHC02623, "E", infile, strerror( errno ));
        return NULL;
    }

    /* Add all of this file's data to our RECS collection */
    while ((rsz = (int) fread( bfr, 1, MAX_PLC2_BLKSIZE, ifile )) != 0)
        if (addrecs( recs, bfr, rsz) != 0)
            return NULL;

    /* Check for I/O error */
    if (ferror( ifile ))
    {
        // "I/O error on file \"%s\": %s"
        FWRMSG( stderr, HHC02627, "E", infile, strerror( errno ));
        VERIFY( fclose( ifile ) == 0 );
        return NULL;
    }

    /* Close the file */
    VERIFY( fclose( ifile ) == 0 );

    /* Create a TAPE_BLOCKS collection and discard RECS collection */
    blks = flushrecs( recs, recl, recc, filesz, 0x40 );

    return blks;
}

/*-------------------------------------------------------------------*/
/*            Process a textual file for DUMP function               */
/*-------------------------------------------------------------------*/
static TAPE_BLOCKS* dump_text_file( OPTIONS* opts,
                                    const char* infile, char recfm,
                                    int* recl, int* recc, size_t* filesz )
{
    TAPE_BLOCKS*  blks;
    FILE*         ifile;
    RECS*         recs;
    const BYTE*   hdr;
    char*         rec;
    int           rsz, maxrsz = 0;
    bool          truncated = false;

    /* Allocate and initialize a new RECS collection for this file */
    hdr = opts->cms ? CMS__hdr : PLCD_hdr;
    if (!(recs = initrecs( opts, recfm, *recl, hdr, BLK_HDRSIZE )))
        return NULL;

    /* Open this file for reading */
    if (!(ifile = fopen( infile, "r" )))
    {
        // "File open error: \"%s\": %s"
        FWRMSG( stderr, HHC02623, "E", infile, strerror( errno ));
        return NULL;
    }

    /* Add all of this file's data to our RECS collection */
    while ((rec = fgets( (char*)bfr, sizeof( bfr ), ifile )) != NULL)
    {
        /* Get length */
        rsz = (int) strlen( rec );

        /* Remove newline (if present) and adjust length */
        if (rsz && rec[ rsz - 1 ] == '\n')
            rec[ --rsz ] = 0;

        /* If fixed, pad or truncate record to specified length */
        if (recfm == 'F')
        {
            /* Pad or truncate the record as needed */
            if (rsz < *recl )
                memset( &rec[rsz], ' ', *recl - rsz );  // (pad)
            else if (rsz > *recl)
            {
                maxrsz = MAX( rsz, maxrsz );
                rec[ *recl ] = 0;  // (truncate)
                truncated = true;
            }
            rsz = *recl;
        }
        else // (recfm == 'V')
        {
            /* If empty line, change it to a single blank */
            if (!rsz)
            {
                rec[0] = ' ';
                rec[ rsz = 1 ] =  0;
            }
        }

        /* Translate textual data from ASCII to EBCDIC */
        str_host_to_guest( rec, wrk, rsz );

        /* Add this record to our RECS collection */
        if (addrecs( recs, wrk, rsz ) != 0)
            return NULL;
    }

    /* Check for I/O error */
    if (ferror( ifile ))
    {
        // "I/O error on file \"%s\": %s"
        FWRMSG( stderr, HHC02627, "E", infile, strerror( errno ));
        VERIFY( fclose( ifile ) == 0 );
        return NULL;
    }

    /* Close the file */
    VERIFY( fclose( ifile ) == 0 );

    /* Create a TAPE_BLOCKS collection and discard RECS collection */
    blks = flushrecs( recs, recl, recc, filesz, 0x40 );

    if (truncated)
        // "lrecl %u less than %u; one or more records truncated"
        FWRMSG( stderr, HHC02628, "W", *recl, maxrsz );

    return blks;
}

/*-------------------------------------------------------------------*/
/*           Process a structured file for DUMP function             */
/*-------------------------------------------------------------------*/
static TAPE_BLOCKS* dump_structured_file( OPTIONS* opts,
                                          const char* infile, char recfm,
                                          int* recl, int* recc, size_t* filesz )
{
    TAPE_BLOCKS*  blks;
    FILE*         ifile;
    RECS*         recs;
    const BYTE*   hdr;
    size_t        num;
    int           rsz;
    uint16_t      rlbfr;

    /* Allocate and initialize a new RECS collection for this file */
    hdr = opts->cms ? CMS__hdr : PLCD_hdr;
    if (!(recs = initrecs( opts, recfm, *recl, hdr, BLK_HDRSIZE )))
        return NULL;

    /* Open this file for reading */
    if (!(ifile = fopen( infile, "rb" )))
    {
        // "File open error: \"%s\": %s"
        FWRMSG( stderr, HHC02623, "E", infile, strerror( errno ));
        return NULL;
    }

    /* Read the structured record's length */
    while ((num = fread( &rlbfr, 1, sizeof( rlbfr ), ifile )) == sizeof( rlbfr ))
    {
        rsz = fetch_hw( &rlbfr );

        /* Now that we know how big the record is,
           read the actual structure record itself. */
        if ((int)(num = fread( bfr, 1, rsz, ifile )) != rsz)
        {
            if (ferror( ifile ))
                // "I/O error on file \"%s\": %s"
                FWRMSG( stderr, HHC02627, "E", infile, strerror( errno ));
            else
                // "Expected %d bytes from file \"%s\", but only %d bytes read"
                FWRMSG( stderr, HHC02629, "E", rsz, infile, (int) num );

            VERIFY( fclose( ifile ) == 0 );
            return NULL;
        }

        /* Add this record to this file's RECS collection */
        if (addrecs( recs, bfr, rsz ) != 0)
            return NULL;
    }

    /* Short read or I/O error? */
    if (num || ferror( ifile ))
    {
        if (num)
            // "Expected %d bytes from file \"%s\", but only %d bytes read"
            FWRMSG( stderr, HHC02629, "E", (int) sizeof( rlbfr ), infile, (int) num );
        else
            // "I/O error on file \"%s\": %s"
            FWRMSG( stderr, HHC02627, "E", infile, strerror( errno ));

        VERIFY( fclose( ifile ) == 0 );
        return NULL;
    }

    /* Close the file */
    VERIFY( fclose( ifile ) == 0 );

    /* Create a TAPE_BLOCKS collection and discard RECS collection */
    blks = flushrecs( recs, recl, recc, filesz, 0x40 );

    return blks;
}

/*-------------------------------------------------------------------*/
/*                 Process file for DUMP function                    */
/*-------------------------------------------------------------------*/
static TAPE_BLOCKS* dump_file( OPTIONS* opts,
                               const char* infile, char filefmt, char recfm,
                               int* recl, int* recc, size_t* filesz )
{
    TAPE_BLOCKS* blks;

    switch (filefmt)
    {
        case 'B': blks = dump_binary_file     ( opts, infile, recfm, recl, recc, filesz ); break;
        case 'T': blks = dump_text_file       ( opts, infile, recfm, recl, recc, filesz ); break;
        case 'S': blks = dump_structured_file ( opts, infile, recfm, recl, recc, filesz ); break;
        default:
            // "INTERNAL ERROR %s"
            FWRMSG( stderr, HHC02634, "E", "0001" );
            return NULL;
    }

    return blks;
}

/*-------------------------------------------------------------------*/
/*         File Status Table (FST) as expected by VMFPLC2            */
/*-------------------------------------------------------------------*/
struct FST                  /* File Status Table (FST)               */
{
/*      Basic (short) FST = 40 bytes                                 */

/*000*/ BYTE   fn[8];       /* File name                             */
/*008*/ BYTE   ft[8];       /* File type                             */
/*010*/ BYTE   dt[4];       /* date: MMDDhhmm                  (DCB) */
/*014*/ HWORD  wp;          /* Write ptr                             */
/*016*/ HWORD  rp;          /* Read ptr                              */
/*018*/ BYTE   fm[2];       /* File mode                             */
/*01A*/ HWORD  reccount;    /* 16 bit record count                   */
/*01C*/ HWORD  fcl;         /* First chain link                      */
/*01E*/ BYTE   recfm;       /* Record fmt                   (F or V) */
/*01F*/ BYTE   flag1;       /* Flags                                 */
#define FSTCNTRY  0x08      /* Century for year and adt: 0=19, 1=20  */
/*020*/ FWORD  lrecl;       /* 32 bit record length                  */
/*024*/ HWORD  dbc;         /* 800-byte Data Block Count             */
/*026*/ BYTE   year[2];     /* Last 2 digits of year (YY)   (EBCDIC) */
/*028*/                     /* Size of Basic (short) FST = 40 bytes  */

/*      The following two fields are VMFPLC2 specific                */

/*028*/ FWORD  lastsz;      /* num 800 byte blocks in last record    */
/*02C*/ FWORD  numblk;      /* num complete 4000 byte records        */

/*      Extended EDF FST (BSEPP and later) = 64 bytes                */

/*030*/ FWORD  afop;        /* Alt File Origin Ptr                   */
/*034*/ FWORD  adbc;        /* Alt Data Block Count                  */
/*038*/ FWORD  aic;         /* Alt Item Count                        */
/*03C*/ BYTE   levels;      /* Levels of indirection                 */
/*03D*/ BYTE   ptrsz;       /* Size of Pointers                      */
/*03E*/ BYTE   adt[6];      /* Alt Date: YYMMDDHHMMSS          (DCB) */
/*044*/ BYTE   extra[4];    /* Reserved                              */
/*048*/                     /* Size of Extended EDF FST = 64 bytes   */
};
typedef struct FST  FST;    /* Just a shorter name than "struct FST" */
CASSERT( sizeof( FST ) == 72, vmfplc2_c );

/* Empty FST Alternate Date field */
#define SIZEOF_STRUCT_MEM( s, m )   sizeof(((s*)0)->m)
static const BYTE empty_adt[ SIZEOF_STRUCT_MEM( FST, adt )] = {0};

/*-------------------------------------------------------------------*/
/*  CMS 'TAPE DUMP' command '.CMSN' "name" tape block header format  */
/*-------------------------------------------------------------------*/
struct CMS                  /* CMS TAPE DUMP block layout            */
{
/*000*/ HWORD  wp;          /* Write ptr                             */
/*002*/ HWORD  rp;          /* Read ptr                              */
/*004*/ HWORD  mode;        /* Mode                                  */
/*006*/ HWORD  reccount;    /* 16 bit record count                   */
/*008*/ HWORD  fcl;         /* First chain link                      */
/*00A*/ BYTE   recfm;       /* Record fmt                   (F or V) */
/*00B*/ BYTE   flag1;       /* Flags                                 */
/*00C*/ FWORD  lrecl;       /* 32 bit record length                  */
/*010*/ HWORD  dbc;         /* 800-byte Data Block Count             */
/*012*/ BYTE   year[2];     /* Last 2 digits of year (YY)   (EBCDIC) */
/*014*/ BYTE   dt[4];       /* date: MMDDhhmm                  (DCB) */

    /* Beginning of alternate counts (added with EDF file system)    */

/*018*/ FWORD  afop;        /* Alt File Origin Ptr                   */
/*01C*/ FWORD  adbc;        /* Alt Data Block Count                  */
/*020*/ FWORD  aic;         /* Alt Item Count                        */
/*024*/ BYTE   levels;      /* Levels of indirection                 */
/*025*/ BYTE   ptrsz;       /* Size of Pointers                      */
/*026*/ BYTE   adt[6];      /* Alt Date: YYMMDDHHMMSS          (DCB) */
/*02C*/ BYTE   extra[4];    /* Reserved                              */
/*030*/ BYTE   pad[16];     /* (pad to to 64 bytes)                  */

    /* Filename appended following FST                               */

/*040*/ BYTE   fn[8];       /* File name                             */
/*048*/ BYTE   ft[8];       /* File type                             */
/*050*/ BYTE   fm[2];       /* File mode                             */
/*052*/                     /* Total length of structure             */
};
typedef struct CMS  CMS;    /* Just a shorter name than "struct CMS" */
CASSERT( sizeof( CMS ) == 82, vmfplc2_c );

/*-------------------------------------------------------------------*/
/* Convert single byte decimal value to DCB (Decimal Coded Binary)   */
/*-------------------------------------------------------------------*/
static BYTE to_dcb( BYTE v )
{
    return ((v/10) * 16) + (v % 10);        // 0x1C (28) ==> 0x28
}

/*-------------------------------------------------------------------*/
/* Convert single byte DCB (Decimal Coded Binary) value to binary    */
/*-------------------------------------------------------------------*/
static BYTE from_dcb( BYTE v )
{
    return ((v >> 4) * 10) + (v & 0x0F);    // 0x28 (28) ==> 0x1C
}

/*-------------------------------------------------------------------*/
/* Convert DCB (Decimal Coded Binary) bytes to string                */
/*-------------------------------------------------------------------*/
static const char* dcb_to_str( const BYTE* dcb, size_t dcbsz, char* str, size_t strsz )
{
    size_t i;
    char buf[4] = {0};

    /* (valid arguments check) */
    ASSERT( dcb && dcbsz && str && strsz > (2 * dcbsz));

    if (!str || !dcbsz || !str || !strsz)
        return NULL;

    *str = 0;

    for (i=0; i < dcbsz; i++)
    {
        MSGBUF( buf, "%02u", from_dcb( dcb[i] ));
        strlcat( str, buf, strsz );
    }

    return str;
}

/*-------------------------------------------------------------------*/
/*             FST tape record  (with VMFPLC2 header)                */
/*-------------------------------------------------------------------*/
struct FST_BLOCK
{
    BYTE  hdr[ BLK_HDRSIZE ];   /* .PLCH hdr  */
    FST   fst;                  /* FST record */
};
typedef struct FST_BLOCK    FST_BLOCK;

/*-------------------------------------------------------------------*/
/*              Create the VMFPLC2 Header record                     */
/*-------------------------------------------------------------------*/
static struct FST_BLOCK* build_fst_block( const char* fn, const char* ft,
                                          const char* fm, char recfm,
                                          int lrecl, int reccount,
                                          int filesz, time_t dt,
                                          size_t blk_count )
{
    FST_BLOCK*   fstb;
    struct  tm*  ttm;
    int          numfull;
    int          lastsz;
    char         bfr[3];

    if (!(fstb = malloc( sizeof( FST_BLOCK ))))
        return NULL;

    memset( fstb, 0, sizeof( FST_BLOCK ));
    memcpy( fstb->hdr, PLCH_hdr, BLK_HDRSIZE );

    memset( fstb->fst.fn, 0x40, sizeof( fstb->fst.fn ));
    memset( fstb->fst.ft, 0x40, sizeof( fstb->fst.ft ) );
    memset( fstb->fst.fm, 0x40, sizeof( fstb->fst.fm ) );

    str_host_to_guest( fn, fstb->fst.fn, sizeof( fstb->fst.fn ));
    str_host_to_guest( ft, fstb->fst.ft, sizeof( fstb->fst.ft ));
    str_host_to_guest( fm, fstb->fst.fm, sizeof( fstb->fst.fm ));

    ttm = localtime( &dt );

    fstb->fst.dt[0] = to_dcb( ttm->tm_mon  + 1 ); // Months since January! (0..11)
    fstb->fst.dt[1] = to_dcb( ttm->tm_mday + 0 );
    fstb->fst.dt[2] = to_dcb( ttm->tm_hour + 0 );
    fstb->fst.dt[3] = to_dcb( ttm->tm_min  + 0 );

    MSGBUF( bfr, "%2.2u", ttm->tm_year % 100 );
    str_host_to_guest( bfr, fstb->fst.year, sizeof( fstb->fst.year ));

    fstb->fst.adt[0] = to_dcb( ttm->tm_year % 100 );
    fstb->fst.adt[1] = to_dcb( ttm->tm_mon  + 1 ); // Months since January! (0..11)
    fstb->fst.adt[2] = to_dcb( ttm->tm_mday + 0 );
    fstb->fst.adt[3] = to_dcb( ttm->tm_hour + 0 );
    fstb->fst.adt[4] = to_dcb( ttm->tm_min  + 0 );
    fstb->fst.adt[5] = to_dcb( ttm->tm_sec  + 0 );

    fstb->fst.recfm = host_to_guest( recfm );

    fstb->fst.flag1 = FSTCNTRY;   // current year is >= 2000!

    store_fw( fstb->fst.lrecl,    (U32) lrecl        );
    store_fw( fstb->fst.aic,      (U32) reccount     );

    store_hw( fstb->fst.reccount, (U16) reccount     );
    store_hw( fstb->fst.wp,       (U16) reccount + 1 );
    store_hw( fstb->fst.rp,       (U16)     1        );
    store_hw( fstb->fst.dbc,      (U16) blk_count    );

    if (recfm == 'V')
        filesz += (reccount * sizeof( HWORD ));

    numfull = filesz / PLC2_BLOCKSIZE;
    lastsz  = filesz % PLC2_BLOCKSIZE;
    if (lastsz)
        lastsz = ROUND_UP( lastsz, CMS_BLOCKSIZE );
    lastsz /= CMS_BLOCKSIZE;

    store_fw( fstb->fst.numblk, (U32) numfull );
    store_fw( fstb->fst.lastsz, (U32) lastsz  );

    return  fstb;
}

/*-------------------------------------------------------------------*/
/*             CMS tape record  (with CMSN header)                   */
/*-------------------------------------------------------------------*/
struct CMS_BLOCK
{
    BYTE  hdr[ BLK_HDRSIZE ];       /* .CMSN hdr    */
    CMS   cms;                      /* CMSN record  */
    BYTE  rsrvd[ CMS_BLOCKSIZE - sizeof( CMS ) ];
};
typedef struct CMS_BLOCK    CMS_BLOCK;

CASSERT( sizeof( CMS_BLOCK ) == (BLK_HDRSIZE + CMS_BLOCKSIZE), vmfplc2_c );

/*-------------------------------------------------------------------*/
/*                Create the CMS name record                         */
/*-------------------------------------------------------------------*/
static struct CMS_BLOCK* build_cms_block( const char* fn, const char* ft,
                                          const char* fm, char recfm,
                                          int lrecl, int reccount,
                                          int filesz, time_t dt,
                                          size_t blk_count )
{
    CMS_BLOCK*   cmsb;
    struct  tm*  ttm;
    char         bfr[3];

    UNREFERENCED( filesz );

    if (!(cmsb = malloc( sizeof( CMS_BLOCK ))))
        return NULL;

    memset( cmsb, 0, sizeof( CMS_BLOCK ));
    memcpy( cmsb->hdr, CMSN_hdr, BLK_HDRSIZE );

    memset( cmsb->cms.fn, 0x40, sizeof( cmsb->cms.fn ));
    memset( cmsb->cms.ft, 0x40, sizeof( cmsb->cms.ft ) );
    memset( cmsb->cms.fm, 0x40, sizeof( cmsb->cms.fm ) );

    str_host_to_guest( fn, cmsb->cms.fn, sizeof( cmsb->cms.fn ));
    str_host_to_guest( ft, cmsb->cms.ft, sizeof( cmsb->cms.ft ));
    str_host_to_guest( fm, cmsb->cms.fm, sizeof( cmsb->cms.fm ));

    memcpy( cmsb->cms.mode, cmsb->cms.fm, sizeof( HWORD ));

    ttm = localtime( &dt );

    cmsb->cms.dt[0] = to_dcb( ttm->tm_mon  + 1 ); // Months since January! (0..11)
    cmsb->cms.dt[1] = to_dcb( ttm->tm_mday + 0 );
    cmsb->cms.dt[2] = to_dcb( ttm->tm_hour + 0 );
    cmsb->cms.dt[3] = to_dcb( ttm->tm_min  + 0 );

    MSGBUF( bfr, "%2.2u", ttm->tm_year % 100 );
    str_host_to_guest( bfr, cmsb->cms.year, sizeof( cmsb->cms.year ));

    cmsb->cms.recfm = host_to_guest( recfm );

    store_fw( cmsb->cms.lrecl,    (U32) lrecl        );
    store_hw( cmsb->cms.reccount, (U16) reccount     );
    store_hw( cmsb->cms.wp,       (U16) reccount + 1 );
    store_hw( cmsb->cms.rp,       (U16)     1        );
    store_hw( cmsb->cms.dbc,      (U16) blk_count    );

    return  cmsb;
}

/*-------------------------------------------------------------------*/
/* Format a consistent date/time string from FST data                */
/*-------------------------------------------------------------------*/
static const char* format_fst_datetime( const FST* fst, char* str, size_t strsz )
{
    char  cc [4]  = {0};    // "YY" century
    char  yy [4]  = {0};    // "YY" year
    char  mo [4]  = {0};    // "MM" month
    char  dd [4]  = {0};    // "DD" day
    char  hh [4]  = {0};    // "HH" hour
    char  mi [4]  = {0};    // "MM" minute
    char  ss [4]  = {0};    // "SS" second

    STRLCPY( cc, (fst->flag1 & FSTCNTRY) ? "20" : "19" );

    /* Use Alt Date (YYMMDDHHMMSS) if it's available (non-zero) */

    if (memcmp( fst->adt, empty_adt, sizeof( fst->adt )) != 0)
    {
        dcb_to_str( fst->adt + 0, 1, yy, sizeof( yy ));
        dcb_to_str( fst->adt + 1, 1, mo, sizeof( mo ));
        dcb_to_str( fst->adt + 2, 1, dd, sizeof( dd ));
        dcb_to_str( fst->adt + 3, 1, hh, sizeof( hh ));
        dcb_to_str( fst->adt + 4, 1, mi, sizeof( mi ));
        dcb_to_str( fst->adt + 5, 1, ss, sizeof( ss ));
    }
    else /* (use shorter default (MMDDhhmm) date) */
    {
        str_guest_to_host( fst->year, yy, sizeof( fst->year ));

        dcb_to_str( fst->dt + 0, 1, mo, sizeof( mo ));
        dcb_to_str( fst->dt + 1, 1, dd, sizeof( dd ));
        dcb_to_str( fst->dt + 2, 1, hh, sizeof( hh ));
        dcb_to_str( fst->dt + 3, 1, mi, sizeof( mi ));
        STRLCPY( ss, "00" );
    }

    // "YYYY-MM-DD HH:MM:SS"    // 19 chars + null = 20 bytes

    snprintf( str, strsz, "%2.2s%2.2s-%2.2s-%2.2s %2.2s:%2.2s:%2.2s",
                           cc, yy, mo, dd, hh, mi, ss );
    return str;
}

/*-------------------------------------------------------------------*/
/* Format a consistent date/time string from FST data                */
/*-------------------------------------------------------------------*/
static const char* format_cms_datetime( const CMS* cms, char* str, size_t strsz )
{
    char  yy [4]  = {0};    // "YY" year
    char  mo [4]  = {0};    // "MM" month
    char  dd [4]  = {0};    // "DD" day
    char  hh [4]  = {0};    // "HH" hour
    char  mi [4]  = {0};    // "MM" minute

    str_guest_to_host( cms->year, yy, sizeof( cms->year ));

    dcb_to_str( cms->dt + 0, 1, mo, sizeof( mo ));
    dcb_to_str( cms->dt + 1, 1, dd, sizeof( dd ));
    dcb_to_str( cms->dt + 2, 1, hh, sizeof( hh ));
    dcb_to_str( cms->dt + 3, 1, mi, sizeof( mi ));

    // "MM/DD/YY HH:MM"     // 14 chars + null = 15 bytes

    snprintf( str, strsz, "%s/%s/%s %s:%s",
                           mo, dd, yy, hh, mi );
    return str;
}

/*-------------------------------------------------------------------*/
/* Format FST information line                                       */
/*-------------------------------------------------------------------*/
static const char* format_fst_info( const FST* fst, char* str, size_t strsz )
{
    int  recl, recs, blks;

    char fn [ sizeof( fst->fn ) + 1 ]  = {0};
    char ft [ sizeof( fst->ft ) + 1 ]  = {0};
    char fm [ sizeof( fst->fm ) + 1 ]  = {0};
    char dt [ 19                + 1 ]  = {0};
    char recfm;

    str_guest_to_host( fst->fn, fn, sizeof( fst->fn ));
    str_guest_to_host( fst->ft, ft, sizeof( fst->ft ));
    str_guest_to_host( fst->fm, fm, sizeof( fst->fm ));

    recfm = (char) guest_to_host( fst->recfm );

    format_fst_datetime( fst, dt, sizeof( dt ));

    recl = fetch_fw( fst->lrecl );

    if (!(recs = fetch_fw( fst->aic )))
        recs = fetch_hw( fst->reccount );

    blks  = fetch_fw( fst->numblk );
    blks *= (PLC2_BLOCKSIZE / CMS_BLOCKSIZE);
    blks += fetch_fw( fst->lastsz );

    snprintf( str, strsz, "%-8s %-8s %-2s %c %-5u %s recs %4u blks %u",
        fn , ft, fm, recfm, recl, dt, recs, blks );

    return str;
}

/*-------------------------------------------------------------------*/
/* Format CMS information line                                       */
/*-------------------------------------------------------------------*/
static const char* format_cms_info( const CMS* cms, char* str, size_t strsz )
{
    int  recl, recs, blks;

    char fn [ sizeof( cms->fn ) + 1 ]  = {0};
    char ft [ sizeof( cms->ft ) + 1 ]  = {0};
    char fm [ sizeof( cms->fm ) + 1 ]  = {0};
    char dt [ 19                + 1 ]  = {0};
    char recfm;

    str_guest_to_host( cms->fn, fn, sizeof( cms->fn ));
    str_guest_to_host( cms->ft, ft, sizeof( cms->ft ));
    str_guest_to_host( cms->fm, fm, sizeof( cms->fm ));

    recfm = (char) guest_to_host( cms->recfm );

    format_cms_datetime( cms, dt, sizeof( dt ));

    recl = fetch_fw( cms->lrecl );
    recs = fetch_hw( cms->reccount );
    blks = fetch_hw( cms->dbc );

    snprintf( str, strsz, "%-8s %-8s %-2s %c %-5u %s recs %4u blks %u",
        fn , ft, fm, recfm, recl, dt, recs, blks );

    return str;
}

/*-------------------------------------------------------------------*/
/*      This function validates a CMS File Name or File Type         */
/*   Returns 1-based position of invalid character or 0 is valid.    */
/*-------------------------------------------------------------------*/
static int validate_fnft( const char *fnft )
{
    int i, j;

    for (i=0; fnft[i]; i++)     /* For each char in FN/FT... */
    {
        for (j=0; validchars[j] && fnft[i] != validchars[j]; j++)
            ; // (do nothing)

        if (!validchars[j])     /* If end of valid string reached */
            return i+1;         /* Then an invalid char was found */
    }
    return 0;
}

/*-------------------------------------------------------------------*/
/*              Validate a CMS FN FT FM construct                    */
/* Returns error message if invalid, otherwise NULL if valid         */
/*-------------------------------------------------------------------*/
static const char* validate_cmsfile( const char* fn, const char* ft, const char* fm )
{
    int i;
    static char msg[512];

    if (strlen( fn ) > 8) return "CMS File name too long";
    if (strlen( ft ) > 8) return "CMS File type too long";
    if (strlen( fm ) > 2) return "CMS File mode too long";

    if ((i = validate_fnft( fn )) != 0)
    {
        MSGBUF( msg,
            "Invalid character in CMS file %s at position %d",
            "name", i );
        return msg;
    }

    if ((i = validate_fnft( ft )) != 0)
    {
        // "Invalid character in CMS file %s at position %d"
        MSGBUF( msg,
            "Invalid character in CMS file %s at position %d",
            "type", i );
        return msg;
    }

    if (!isalpha( (unsigned char)fm[0] ))
        return "CMS File mode must start with a letter";

    if (strlen( fm ) > 1)
    {
        for (i=0; validfmnum[i] && fm[1] != validfmnum[i]; i++)
            ; // (do nothing)

        if (!validfmnum[i])
            return "CMS File mode number must be 0-6";
    }

    return NULL;
}

/*-------------------------------------------------------------------*/
/*                         CTLTAB                                    */
/*-------------------------------------------------------------------*/
struct CTLTAB
{
    const char*  orec;          // (original control file record)
    const char*  codepage;
    const char*  fn;            // (tapemark entry if "@TM")
    const char*  ft;
    const char*  fm;
    const char*  hostfile;
    time_t       mtime;
    int          reclen;        // i.e. lrecl
    char         recfm;         // 'F'ixed or 'V'ariable
    char         filefmt;       // 'T'ext, 'B'inary or 'S'truct
    bool         loaded;        // entry has been LOADed
};
typedef struct CTLTAB   CTLTAB;

static CTLTAB*  ctltab  = NULL;     // (pointer to table)
static int      numctl  = 0;        // (number of entries)

/*-------------------------------------------------------------------*/
/*              Add a new CTLTAB entry to the table                  */
/*-------------------------------------------------------------------*/
static bool add_ctltab_entry( CTLTAB* ctl )
{
    CTLTAB* tab;
    int     num;

    /* Make room for new entry */
    if (!(tab = realloc( ctltab, sizeof( CTLTAB ) * (num = numctl + 1))))
        return false; // (out of memory)

    ctltab = tab;
    numctl = num;

    /* Append new entry to end of table */
    memcpy( &ctltab[ numctl - 1 ], ctl, sizeof( CTLTAB ));
    return true;
}

/*-------------------------------------------------------------------*/
/*                 Find and return CTLTAB entry                      */
/*-------------------------------------------------------------------*/
static CTLTAB* find_ctltab_entry( const char* fn, const char* ft, char recfm )
{
    int  i;
    for (i=0; i < numctl; i++)
        if (1
            && strcasecmp( ctltab[i].fn,    "@TM"   ) != 0  // not tapemark entry
            && strcasecmp( ctltab[i].fn,      fn    ) == 0  // file name matches
            && strcasecmp( ctltab[i].ft,      ft    ) == 0  // file type matches
            &&             ctltab[i].recfm == recfm         // recfm matches
            &&            !ctltab[i].loaded                 // not already loaded
        )
            return &ctltab[i];  // (found)
    return NULL;                // (not found)
}

/*-------------------------------------------------------------------*/
/*                Parse a control file statement                     */
/*-------------------------------------------------------------------*/
static int parse_ctlfile_stmt( OPTIONS* opts, const char* orec, int recno )
{
    int            rc;
    int            argc;
    int            reclen;

    struct  stat   stt                  = {0};
    CTLTAB         ctl                  = {0};
    char*          argv[ MAX_ARGS ]     = {0};
    char*          fn                   = NULL;
    char*          ft                   = NULL;
    char*          fm                   = NULL;
    char*          recfm                = NULL;
    char*          lrecl                = NULL;
    char*          filefmt              = NULL;
    char*          infile               = NULL;
    char*          rec                  = NULL;
    char*          endptr               = NULL;
    const char*    msg                  = NULL;
    const char*    codepage             = NULL;

    /* Parse the ctlfile statement */
    if (0
        || parse_args((rec = strdup( orec )), MAX_ARGS, argv, &argc ) == 0
        || argv[0][0] == '*'
        || argv[0][0] == '#'
        || argv[0][0] == ';'
    )
    {
        /* Blank line or comment line; ignore */
        free( rec );
        return 0;
    }

    /*     0        1     2     3       4        5       6         7      */
    /* [codepage]  <FN>  <FT>  <FM>  <recfm>  <lrecl>  <type>  <hostfile> */

    codepage = argc >= 1 ? argv[0] : NULL;
    fn       = argc >= 2 ? argv[1] : NULL;
    ft       = argc >= 3 ? argv[2] : NULL;
    fm       = argc >= 4 ? argv[3] : NULL;
    recfm    = argc >= 5 ? argv[4] : NULL;
    lrecl    = argc >= 6 ? argv[5] : NULL;
    filefmt  = argc >= 7 ? argv[6] : NULL;
    infile   = argc >= 8 ? argv[7] : NULL;

    /* Was codepage specified? */
    if (codepage && strchr( codepage, '/' ))
    {
        if (!valid_codepage_name( codepage ))
        {
            msg = "Invalid codepage";
            goto process_entry_error;
        }
    }
    else /* codepage NOT specified; adjust parameters */
    {
        infile   = filefmt;
        filefmt  = lrecl;
        lrecl    = recfm;
        recfm    = fm;
        fm       = ft;
        ft       = fn;
        fn       = (char*) codepage;
        codepage = defaultcp;
    }

    /* Validate each argument... */

    if (!fn)
    {
        msg = "File name missing";
        goto process_entry_error;
    }

    if (strcasecmp( fn, "@TM" ) == 0)
    {
        ctl.fn = strdup( "@TM" );
        ctl.loaded = false;
        VERIFY( add_ctltab_entry( &ctl ));
        return 0;
    }

    if (!ft)
    {
        msg = "File type missing";
        goto process_entry_error;
    }

    if (!fm)
    {
        msg = "File mode missing";
        goto process_entry_error;
    }

    strupper( fn, fn );
    strupper( ft, ft );
    strupper( fm, fm );

    if ((msg = validate_cmsfile( fn, ft, fm )) != NULL)
        goto process_entry_error;

    if (!recfm)
    {
        msg = "Record format missing";
        goto process_entry_error;
    }

    if (1
        && !CMD( recfm, FIXED,    1 )
        && !CMD( recfm, VARIABLE, 1 )
    )
    {
        msg = "Record format must be 'F'ixed or 'V'ariable";
        goto process_entry_error;
    }

    /* For recfm 'V'ariable files the lrecl should not be specified. */
    if (recfm[0] == 'V')
    {
        infile  = filefmt;
        filefmt = lrecl;
        reclen  = 0;     // (recln meaningless for recfm V = variable)
    }
    else /* (recfm[0]) */
    {
        if (!lrecl)
        {
            msg = "Logical Record Length missing for 'F'ixed record format file";
            goto process_entry_error;
        }

        errno  = 0;
        reclen = strtoul( lrecl, &endptr, 0 );

        if (errno || endptr[0] != 0)
        {
            msg = "Logical Record Length must be a numeric value";
            goto process_entry_error;
        }

        if (reclen < 1 || reclen >= MAX_PLC2_BLKSIZE)
        {
            static char buf[64];
            MSGBUF( buf, "Logical Record Length must be between 1 and %u",
                MAX_PLC2_BLKSIZE );
            msg = buf;
            goto process_entry_error;
        }
    }

    if (!filefmt)
    {
        msg = "File format is missing";
        goto process_entry_error;
    }

    if (1
        && !CMD( filefmt, BINARY,     1 )
        && !CMD( filefmt, TEXTUAL,    1 )
        && !CMD( filefmt, TXT,        1 )
        && !CMD( filefmt, STRUCTURED, 1 )
    )
    {
        msg = "File format must be either 'B'inary, 'T'extual or 'S'tructured";
        goto process_entry_error;
    }

    if (!infile)
    {
        msg = "Input file name missing";
        goto process_entry_error;
    }

    /* Structured files are only for RECFM V for the time being */
    if (filefmt[0] == 'S' && recfm[0] != 'V')
    {
        msg = "Dumping structured input to anything other than RECFM V is not supported";
        goto process_entry_error;
    }

    /* If 'DUMP'ing, verify host file exists */
    if (1
        && DUMP_VERB == opts->verb
        && (rc = stat( infile, &stt )) != 0
    )
    {
        msg = strerror( errno );
        goto process_entry_error;
    }

    /* Add this entry to the table */
    ctl.orec     = strdup( orec );
    ctl.codepage = strdup( codepage );
    ctl.fn       = strdup( fn );
    ctl.ft       = strdup( ft );
    ctl.fm       = strdup( fm );
    ctl.hostfile = strdup( infile );
    ctl.mtime    = stt.st_mtime;
    ctl.reclen   = reclen;
    ctl.recfm    = toupper( (unsigned char)recfm  [0] );
    ctl.filefmt  = toupper( (unsigned char)filefmt[0] );
    ctl.loaded   = false;

    VERIFY( add_ctltab_entry( &ctl ));

    free( rec );
    return 0;

process_entry_error:

    // ">>> %s"
    // "    Bad entry at line %d in file \"%s\""
    // "    %s"

    FWRMSG( stderr, HHC02626, "E", orec );
    FWRMSG( stderr, HHC02632, "E", recno, opts->ctlfile );
    FWRMSG( stderr, HHC02633, "E", msg );

    free( rec );
    return -1;
}

/*-------------------------------------------------------------------*/
/*                Read and parse the control file                    */
/*-------------------------------------------------------------------*/
static int parse_ctlfile( OPTIONS* opts )
{
    FILE*   cfile;
    char*   rec;
    int     recno = 0;
    int     errs  = 0;

    /* Open the control file */
    if (!(cfile = fopen( opts->ctlfile, "r" )))
    {
        // "File open error: \"%s\": %s"
        FWRMSG( stderr, HHC02623, "E", opts->ctlfile, strerror( errno ));
        return -1;
    }

    /* Parse and save each control file statement */
    while ((rec = fgets( (char*) bfr, sizeof( bfr ), cfile )) != NULL)
    {
        recno++;
        rec[ strlen( rec ) - 1 ] = 0;  // (remove newline)

        if (parse_ctlfile_stmt( opts, rec, recno ) != 0)
            errs++;
    }

    /* Check for ctlfile I/O error */
    if (ferror( cfile ))
    {
        // "I/O error on file \"%s\": %s"
        FWRMSG( stderr, HHC02627, "E", opts->ctlfile, strerror( errno ));
        errs++;
    }

    /* Close the control file */
    VERIFY( fclose( cfile ) == 0 );

    if (errs)
        // "%d errors encountered"
        FWRMSG( stderr, HHC02630, "W", errs );

    return errs ? -1 : 0;
}

/*-------------------------------------------------------------------*/
/*                    Perform DUMP operation                         */
/*-------------------------------------------------------------------*/
static int dodump( OPTIONS* opts )
{
    TAPE_BLOCKS*  blks = NULL;
    FST_BLOCK*    fstb = NULL;
    CMS_BLOCK*    cmsb = NULL;
    size_t        filesz;
    int           i, rc, reccount;
    char          info[80];

    // "%s"
    WRMSG( HHC02625, "I", "DUMPING..." );

    /* Parse the DUMP control file */
    if ((rc = parse_ctlfile( opts )) != 0)
        return rc;

    /* Open the output tape file */
    if ((rc = tape_open( opts )) != 0)
        return rc;

    /* Process each entry in the parsed CTLTAB */
    for (i=0, rc=0; rc == 0 && i < numctl; i++)
    {
        /* Write tapemark if requested */
        if (strcasecmp( ctltab[i].fn, "@TM" ) == 0)
        {
            if ((rc = tape_write_mark( opts )) == 0)
                // ">>> %s"
                WRMSG( HHC02626, "I", "@TM" );
            continue;
        }

        /* Set translation codepage if file is TEXT format */
        if (ctltab[i].filefmt == 'T')
            if (strcasecmp( ctltab[i].codepage, query_codepage()) != 0)
                set_codepage( ctltab[i].codepage );

        // ">>> %s"
        WRMSG( HHC02626, "I", ctltab[i].orec );

        /* DUMP file, creating TAPE_BLOCKS */
        if (!(blks = dump_file( opts,
                                ctltab[i].hostfile, ctltab[i].filefmt,
                                ctltab[i].recfm, &ctltab[i].reclen,
                                &reccount, &filesz )))
        {
            rc = -1;
            break;
        }

        /* Build FST_BLOCK or CMS_BLOCK for this file */
        if (opts->cms)
            cmsb = build_cms_block( ctltab[i].fn, ctltab[i].ft, ctltab[i].fm,
                                    ctltab[i].recfm, ctltab[i].reclen, reccount,
                                    (int) filesz, ctltab[i].mtime, blks->blk_count );
        else // (vmfplc2 format)
            fstb = build_fst_block( ctltab[i].fn, ctltab[i].ft, ctltab[i].fm,
                                    ctltab[i].recfm, ctltab[i].reclen, reccount,
                                    (int) filesz, ctltab[i].mtime, blks->blk_count );

        /* (check for success/failure) */
        if (0
            || ( opts->cms && !cmsb)
            || (!opts->cms && !fstb)
        )
        {
            free_blocks( blks );
            rc = -1;
            break;
        }

        /* Create the DUMP tape */
        if (opts->cms)
        {
            /* Write all TAPE_BLOCKS and the CMS_BLOCK for this file */
            if ((rc = write_TAPE_BLOCKS( opts, blks )) == 0)
                if ((rc = tape_write_block( opts, (BYTE*) cmsb, sizeof( CMS_BLOCK ))) == 0)
                    format_cms_info( &cmsb->cms, info, sizeof( info ));
        }
        else // (vmfplc2 format)
        {
            /* Write the FST_BLOCK and all TAPE_BLOCKS for this file */
            if ((rc = tape_write_block( opts, (BYTE*) fstb, sizeof( FST_BLOCK ))) == 0)
                if ((rc = write_TAPE_BLOCKS( opts, blks )) == 0)
                    format_fst_info( &fstb->fst, info, sizeof( info ));
        }

        if (!opts->quiet)
            // "    %s"
            WRMSG( HHC02633, "I", info );

        if (opts->cms) free( cmsb );
        else           free( fstb );

        free_blocks( blks );

        fstb = NULL;
        cmsb = NULL;
        blks = NULL;
    }

    if (rc == 0)
        if ((rc = tape_write_mark( opts )) == 0)
            // ">>> %s"
            WRMSG( HHC02626, "I", "@TM" );

    if (rc == 0)
        if ((rc = tape_write_mark( opts )) == 0)
            // ">>> %s"
            WRMSG( HHC02626, "I", "@TM" );

    /* Close output tape and issue completion message */
    tape_close( opts );

    if (rc == 0)
    {
        // "%s complete"
        WRMSG( HHC02631, "I", verb( opts ));

        if (!opts->quiet)
            // "Tape \"%s\" created"
            WRMSG( HHC02635, "I", opts->tapefile );
    }

    return rc;
}

/*-------------------------------------------------------------------*/
/*                     Perform SCAN operation                        */
/*-------------------------------------------------------------------*/
static int doscan( OPTIONS* opts )
{
    int   rc;
    int   blksize;
    int   tapemark_count = 0;
    FST   fst;
    CMS   cms;
    char  info[80];

    // "%s"
    WRMSG( HHC02625, "I", "SCANNING..." );

    /* Open the output tape file */
    if ((rc = tape_open( opts )) != 0)
        return rc;

    /* Scan tape until end-of-tape reached (two tapemarks in a row) */
    while (tapemark_count < 2 && (rc = tape_read( opts )) >= 0)
    {
        blksize = rc;

        if (blksize)
        {
            tapemark_count = 0;

            /* Is this a PLCH header (FST) block? */
            if (1
                && blksize >= BLK_HDRSIZE
                && memcmp( bfr, PLCH_hdr, BLK_HDRSIZE ) == 0
            )
            {
                /* Yes, display file information */
                memcpy( &fst, &bfr[ BLK_HDRSIZE ], sizeof( FST ));

                // ">>> %s"
                WRMSG( HHC02626, "I",
                    format_fst_info( &fst, info, sizeof( info )));
            }
            /* Is this a CMSN header (FST) block? */
            else if (1
                && opts->cms
                && blksize >= BLK_HDRSIZE
                && memcmp( bfr, CMSN_hdr, BLK_HDRSIZE ) == 0
            )
            {
                /* Yes, display file information */
                memcpy( &cms, &bfr[ BLK_HDRSIZE ], sizeof( CMS ));

                // ">>> %s"
                WRMSG( HHC02626, "I",
                    format_cms_info( &cms, info, sizeof( info )));
            }
        }
        else /* (zero length block == tapemark) */
        {
            // "    %s"
            WRMSG( HHC02633, "I", "@TM" );
            tapemark_count++;
        }
    }

    /* Close input tape and issue completion message */
    tape_close( opts );

    if (!(rc < 0))
        // "%s complete"
        WRMSG( HHC02631, "I", verb( opts ));

    return rc;
}

/*-------------------------------------------------------------------*/
/*        Write structured file record size if appropriate           */
/*-------------------------------------------------------------------*/
static int write_siz( FILE* ofile, const BYTE* p, char filefmt, const char* name )
{
    int rc = 0;
    if (filefmt == 'S')  // ('S'tructured)
    {
        if (fwrite( p, 1, sizeof( HWORD ), ofile ) != sizeof( HWORD ))
        {
            // "I/O error on file \"%s\": %s"
            FWRMSG( stderr, HHC02627, "E", name, strerror( errno ));
            rc = -1;
        }
    }
    return rc;
}

/*-------------------------------------------------------------------*/
/*                 write blank trimmed host text                     */
/*-------------------------------------------------------------------*/
static int write_rec( FILE* ofile, BYTE* p, int len,
                      bool trim, char filefmt, const char* name )
{
    int rc = 0;
    if (len)
    {
        if (filefmt == 'T')  // ('T'extual)
        {
            buf_guest_to_host( p, p, len );
            if (trim)
                while (len && ' ' == (char)(*(p + len - 1)))
                    --len;
        }
        if (len && fwrite( p, 1, (size_t) len, ofile ) != (size_t) len)
        {
            // "I/O error on file \"%s\": %s"
            FWRMSG( stderr, HHC02627, "E", name, strerror( errno ));
            rc = -1;
        }
    }
    return rc;
}

/*-------------------------------------------------------------------*/
/*                 Write end-of-line if needed                       */
/*-------------------------------------------------------------------*/
static int endof_rec( FILE* ofile, char filefmt, const char* name )
{
    int rc = 0;
    if (filefmt == 'T')  // ('T'extual)
    {
#if defined( _MSVC_ )
        if (fwrite( "\r\n", 1, 2, ofile ) != 2)
#else
        if (fwrite( "\n",   1, 1, ofile ) != 1)
#endif
        {
            // "I/O error on file \"%s\": %s"
            FWRMSG( stderr, HHC02627, "E", name, strerror( errno ));
            rc = -1;
        }
    }
    return rc;
}

/*-------------------------------------------------------------------*/
/*                   Show them file being LOADed...                  */
/*-------------------------------------------------------------------*/
static void log_loaded( OPTIONS* opts, CTLTAB* ctl, const char* info )
{
    char msgbuf[80+MAX_PATH];
    // ">>> %s"
    MSGBUF( msgbuf, "LOADING:    %s", info );
    WRMSG( HHC02626, "I", msgbuf );
    if (!opts->quiet)
    {
        /* Show them where file is being loaded to... */
        // "    %s"
        MSGBUF( msgbuf, "     to:    %s", ctl->hostfile );
        WRMSG( HHC02633, "I", msgbuf );
    }
}
static void log_cms_loaded( CMS* cms, CTLTAB* ctl, OPTIONS* opts )
{
    char info[80];
    log_loaded( opts, ctl, format_cms_info( cms, info, sizeof( info )));
}
static void log_fst_loaded( FST* fst, CTLTAB* ctl, OPTIONS* opts )
{
    char info[80];
    log_loaded( opts, ctl, format_fst_info( fst, info, sizeof( info )));
}

/*-------------------------------------------------------------------*/
/*                   Show them file being SKIPPED...                 */
/*-------------------------------------------------------------------*/
static void log_skipped( const char* info )
{
    char msgbuf[80+MAX_PATH];
    MSGBUF( msgbuf, "SKIPPED:    %s", info );
    // ">>> %s"
    WRMSG( HHC02626, "I", msgbuf );
    /* Show them why file is being skipped... */
    // "    %s"
    WRMSG( HHC02633, "I", "            (no matching ctlfile entry)");
}
static void log_cms_skipped( CMS* cms, OPTIONS* opts )
{
    char info[80];
    if (!opts->quiet)
        log_skipped( format_cms_info( cms, info, sizeof( info )));
}
static void log_fst_skipped( FST* fst, OPTIONS* opts )
{
    char info[80];
    if (!opts->quiet)
        log_skipped( format_fst_info( fst, info, sizeof( info )));
}

/*-------------------------------------------------------------------*/
/*                   LOAD file onto host system                      */
/*-------------------------------------------------------------------*/
static int load_file
(
    CTLTAB*  ctl,
    FILE*    ofile,
    char     recfm,
    int      blksize,
    int*     recl,
    int*     recs,
    int*     recrem,
    bool*    rszsplit,
    BYTE*    rsz1
)
{
    BYTE*  rec = &bfr[ BLK_HDRSIZE ];
    int    rem = blksize - BLK_HDRSIZE;
    bool   endofrec = false;
    int    recremblk = 0;
    int    rc = 0;

    if (recfm == 'F')   /* Fixed */
    {
        /* Finish writing previous record if incomplete */
        if (*recrem)
        {
            /* How much of record bytes remain in this block */
            recremblk = MIN( *recrem, rem );
            endofrec  = (recremblk >= *recrem);

            if ((rc = write_rec( ofile, rec, recremblk, endofrec,
                ctl->filefmt, ctl->hostfile )) < 0)
                return rc;

            if (endofrec && (rc = endof_rec( ofile,
                ctl->filefmt, ctl->hostfile )) < 0)
                return rc;

            /* Adjust record pointer and bytes remaining */
            rec     +=  recremblk;
            rem     -=  recremblk;
            *recrem -=  recremblk;

            /* Adjust records remaining if record now complete */
            if (*recrem == 0)
                --*recs;
        }

        /* Write fixed length recs until end of block */
        while (rc >= 0 && rem && *recs)
        {
            /* Can we write a complete record? */
            if (rem >= *recl)
            {
                if (1
                    && (rc = write_rec( ofile,
                                        rec, *recl,
                                        true,
                                        ctl->filefmt, ctl->hostfile )) == 0

                    && (rc = endof_rec( ofile,
                                        ctl->filefmt, ctl->hostfile )) == 0
                )
                {
                    /* Adjust record pointer and bytes remaining */
                    rec += *recl;
                    rem -= *recl;

                    /* Adjust records remaining */
                    --*recs;
                }
            }
            else /* (rem < *recl): Write incomplete record */
            {
                if ((rc = write_rec( ofile,
                                     rec, rem,
                                     false,
                                     ctl->filefmt, ctl->hostfile )) == 0)
                {
                    /* Save unwritten bytes in next block */
                    *recrem = *recl - rem;

                    /* Adjust record pointer and bytes remaining */
                    rec += rem;
                    rem -= rem;
                }
            }
        }
    }
    else // (recfm == 'V')  /* Variable */
    {
        /* Finish writing previous record if incomplete */
        if (*recrem)
        {
            /* How much of record bytes remain in this block */
            recremblk = MIN( *recrem, rem );
            endofrec  = (recremblk >= *recrem);

            if ((rc = write_rec( ofile, rec, recremblk, endofrec,
                ctl->filefmt, ctl->hostfile )) < 0)
                return rc;

            if (endofrec && (rc = endof_rec( ofile,
                ctl->filefmt, ctl->hostfile )) < 0)
                return rc;

            /* Adjust record pointer and bytes remaining */
            rec     +=  recremblk;
            rem     -=  recremblk;
            *recrem -=  recremblk;

            /* Adjust records remaining if record now complete */
            if (*recrem == 0)
                --*recs;
        }

        /* Write variable len recs until end of block */
        while (rc >= 0 && *recs && rem >= (int) sizeof( HWORD ))
        {
            HWORD recsize;
            int rsz;

            /* Get size of this record */
            if (*rszsplit)
            {
                recsize[0] = *rsz1;
                recsize[1] = rec[0];
                --rec;
                ++rem;
            }
            else
            {
                recsize[0] = rec[0];
                recsize[1] = rec[1];
            }
            *rszsplit = false;
            rsz = fetch_hw( recsize );
            ASSERT( rsz > 0 && rsz <= *recl );

            if ((rc = write_siz( ofile, recsize, ctl->filefmt, ctl->hostfile )) == 0)
            {
                rec  +=  sizeof( HWORD );
                rem  -=  sizeof( HWORD );

                /* Can we write a complete record? */
                if (rem >= rsz)
                {
                    if (1
                        && (rc = write_rec( ofile,
                                            rec, rsz,
                                            true,
                                            ctl->filefmt, ctl->hostfile )) == 0

                        && (rc = endof_rec( ofile,
                                            ctl->filefmt, ctl->hostfile )) == 0
                    )
                    {
                        /* Adjust record pointer and bytes remaining */
                        rec += rsz;
                        rem -= rsz;

                        /* Adjust records remaining */
                        --*recs;
                    }
                }
                else /* (rem < rsz): Write incomplete record */
                {
                    /* Are ANY record bytes remaining in this block? */
                    if (rem)
                    {
                        if ((rc = write_rec( ofile,
                                             rec, rem,
                                             false,
                                             ctl->filefmt, ctl->hostfile )) == 0)
                        {
                            /* Save unwritten bytes in next block */
                            *recrem = rsz - rem;

                            /* Adjust record pointer and bytes remaining */
                            rec += rem;
                            rem -= rem;
                        }
                    }
                    else // (rem == 0)
                    {
                        /* Save unwritten bytes in next block */
                        *recrem = rsz - rem;
                    }
                }
            }
        }

        /* Did we exit our loop because there weren't
           enough bytes in the buffer to fetch_hw the
           next record's HWORD rsz record size value?
        */
        if ((*rszsplit = (rc >= 0 && *recs && 1 == rem)))
            *rsz1 = *rec;
    }

    return rc;
}

/*-------------------------------------------------------------------*/
/*                   Perform CMS LOAD operation                      */
/*-------------------------------------------------------------------*/
static int doload_cms( OPTIONS* opts )
{
    // CHALLENGES: the tape block containing the name of the file
    // that was dumped is written FOLLOWING (comes AFTER) all of
    // its data blocks, so as you read the tape during a LOAD op-
    // eration you're reading data blocks but you don't know what
    // file they're for until it's too late! (You need to know what
    // file you're reading BEFOREHAND so you can determine whether
    // it's a Text file or not so you can know whether to translate
    // from EBCDIC to ASCII or not as well as whether to write line
    // endings (LF/CRLF) or trim trailing blanks or not! You don't
    // want to do that for Binary or Structured files, just as you
    // don't want to NOT do that for Text files!)

    // To resolve this dilemma, we save the current tape position
    // whenever we read a file's very first data block and continue
    // reading and ignoring all data blocks we read until we reach
    // the name block. Once the name block is reached (and we have
    // all the information we need), we position the tape back to
    // the file's first data block and THIS time process each one
    // normally.

    // Yes, it's inefficient to keep seeking all over the place but
    // it's really the only way we can hope to accomplish our goal!

    FILE*    ofile = NULL;
    CTLTAB*  ctl = NULL;
    CMS      cms;

    int   i, rc = 0;
    int   blksize;              // (size of current tape block)
    int   recrem = 0;           // (unwritten record bytes remaining)
    int   recl, recs;
    int   tapemark_count = 0;

    char  fn [ sizeof( cms.fn ) + 1 ]  = {0};
    char  ft [ sizeof( cms.ft ) + 1 ]  = {0};
    char  fm [ sizeof( cms.fm ) + 1 ]  = {0};
    char  recfm = 'F';

    bool  rszsplit = false;     // (SPECIAL: rsz split across 2 blocks)
    BYTE  rsz1;                 // (SPECIAL: 1st byte of record size)

    U32   blockid = 0;          // (current file starting data block)
    bool  gotblkid = false;     // (false until first data block read)
    bool  loading  = false;     // (false until name block is read)

    // "%s"
    WRMSG( HHC02625, "I", "LOADING..." );

    /* Parse the DUMP control file */
    if ((rc = parse_ctlfile( opts )) != 0)
        return rc;

    /* Open the input tape file */
    if ((rc = tape_open( opts )) != 0)
        return rc;

    /* Read tape until end-of-tape reached (two tapemarks in a row) */
    while (1
        && rc >= 0
        && tapemark_count < 2
        && (rc = tape_read( opts )) >= 0
    )
    {
        blksize = rc;

        if (blksize)
        {
            tapemark_count = 0;

            /* Is this a CMSN (name) block? */
            if (1
                && blksize >= BLK_HDRSIZE
                && memcmp( bfr, CMSN_hdr, BLK_HDRSIZE ) == 0
            )
            {
                gotblkid = false;  // (reset)

                /* Are we scanning or loading? */
                if (!loading)
                {
                    /* Extract needed CMS information */
                    memcpy( &cms, &bfr[ BLK_HDRSIZE ], sizeof( CMS ));

                    str_guest_to_host( cms.fn, fn, sizeof( cms.fn ));
                    str_guest_to_host( cms.ft, ft, sizeof( cms.ft ));
                    str_guest_to_host( cms.fm, fm, sizeof( cms.fm ));

                    /* Remove trailing blanks from fn and ft */
                    for (i=(int)(sizeof( cms.fn ) - 1); i >= 0 && fn[i] == ' '; fn[i]=0, i--);
                    for (i=(int)(sizeof( cms.ft ) - 1); i >= 0 && ft[i] == ' '; ft[i]=0, i--);

                    recfm = (char) guest_to_host( cms.recfm );

                    recl = fetch_fw( cms.lrecl    );
                    recs = fetch_hw( cms.reccount );

                    /* Find corresponding CTLTAB entry */
                    if (!(ctl = find_ctltab_entry( fn, ft, recfm )))
                    {
                        /* No match in ctlfile; skip loading this file ... */
                        log_cms_skipped( &cms, opts );
                        continue; // (skip this file)
                    }
                }
                else /* We WERE loading, but now we're DONE loading */
                {
                    loading = false;
                    continue;
                }

                /* Show them the DUMPed file on the tape that we are LOADing... */
                log_cms_loaded( &cms, ctl, opts );
                ctl->loaded = true;

                /* Close output file if opened */
                if (ofile)
                {
                    VERIFY( fclose( ofile ) == 0 );
                    ofile = NULL;
                }

                /* Set translation codepage if file is TEXT format */
                if (ctl->filefmt == 'T')
                    if (strcasecmp( ctl->codepage, query_codepage()) != 0)
                        set_codepage( ctl->codepage );

                /* Create the empty host output file */
                if (!(ofile = fopen( ctl->hostfile, "wb" )))
                {
                    // "File open error: \"%s\": %s"
                    FWRMSG( stderr, HHC02623, "E", ctl->hostfile, strerror( errno ));
                    rc = -1;
                    continue;
                }

                /* Reset unwritten record bytes */
                recrem = 0;

                /* Reposition tape and begin loading this file */
                rc = tape_set_position( opts, blockid );
                loading = true;
            }
            /* Is this a CMSb header (DATA) block? */
            else if (1
                && blksize >= BLK_HDRSIZE
                && memcmp( bfr, CMS__hdr, BLK_HDRSIZE ) == 0
            )
            {
                /* Save tape position of first data block */
                if (!gotblkid)
                {
                    blockid = tape_get_position( opts );

                    /* PROGRAMMING NOTE: the blockid is updated after
                       the block is read. Thus to re-read the block
                       we just read, we need to save the blockid of
                       the PREVIOUS block.
                    */
                    ASSERT( blockid >= 1 );  // (sanity check)
                    blockid--;
                    gotblkid = true;
                }

                /* Ignore data blocks if not loading file */
                if (!loading)
                    continue;

                /* Continue writing the file if selected */
                if (ofile)
                    rc = load_file( ctl, ofile, recfm, blksize,
                        &recl, &recs, &recrem, &rszsplit, &rsz1 );
            }
            else
            {
                // "Invalid block encountered @ %d:%d"
                FWRMSG( stderr, HHC02636, "W",
                    opts->devblk.curfilen, opts->devblk.blockid );
                rc = -1;
                continue;
            }
        }
        else
        {
            // "    %s"
            WRMSG( HHC02633, "I", "@TM" );
            tapemark_count++;
        }
    }

    /* Close host output file if opened */
    if (ofile)
        VERIFY( fclose( ofile ) == 0 );

    /* Close input tape and issue completion message */
    tape_close( opts );

    if (rc == 0)
        // "%s complete"
        WRMSG( HHC02631, "I", verb( opts ));

    return rc;
}

/*-------------------------------------------------------------------*/
/*                 Perform VMFPLC2 LOAD operation                    */
/*-------------------------------------------------------------------*/
static int doload( OPTIONS* opts )
{
    FILE*    ofile = NULL;
    CTLTAB*  ctl;
    FST      fst;

    int   i, rc = 0;
    int   blksize;              // (size of current tape block)
    int   recrem = 0;           // (unwritten record bytes remaining)
    int   recl, recs;
    int   tapemark_count = 0;

    char  fn [ sizeof( fst.fn ) + 1 ]  = {0};
    char  ft [ sizeof( fst.ft ) + 1 ]  = {0};
    char  fm [ sizeof( fst.fm ) + 1 ]  = {0};
    char  recfm;

    bool  rszsplit = false;     // (SPECIAL: rsz split across 2 blocks)
    BYTE  rsz1;                 // (SPECIAL: 1st byte of record size)


    /* Process CMS 'TAPE' formatted dump tape if requested */
    if (opts->cms)
        return doload_cms( opts );

    // "%s"
    WRMSG( HHC02625, "I", "LOADING..." );

    /* Parse the DUMP control file */
    if ((rc = parse_ctlfile( opts )) != 0)
        return rc;

    /* Open the input tape file */
    if ((rc = tape_open( opts )) != 0)
        return rc;

    /* Read tape until end-of-tape reached (two tapemarks in a row) */
    while (1
        && rc >= 0
        && tapemark_count < 2
        && (rc = tape_read( opts )) >= 0
    )
    {
        blksize = rc;

        if (blksize)
        {
            tapemark_count = 0;

            /* Is this a PLCH header (FST) block? */
            if (1
                && blksize >= BLK_HDRSIZE
                && memcmp( bfr, PLCH_hdr, BLK_HDRSIZE ) == 0
            )
            {
                /* Close output file if opened */
                if (ofile)
                {
                    VERIFY( fclose( ofile ) == 0 );
                    ofile = NULL;
                }

                /* Extract needed FST information */
                memcpy( &fst, &bfr[ BLK_HDRSIZE ], sizeof( FST ));

                str_guest_to_host( fst.fn, fn, sizeof( fst.fn ));
                str_guest_to_host( fst.ft, ft, sizeof( fst.ft ));
                str_guest_to_host( fst.fm, fm, sizeof( fst.fm ));

                /* Remove trailing blanks from fn and ft */
                for (i=(int)(sizeof( fst.fn ) - 1); i >= 0 && fn[i] == ' '; fn[i]=0, i--);
                for (i=(int)(sizeof( fst.ft ) - 1); i >= 0 && ft[i] == ' '; ft[i]=0, i--);

                recfm = (char) guest_to_host( fst.recfm );

                recl = fetch_fw( fst.lrecl );

                if (!(recs = fetch_fw( fst.aic )))
                    recs = fetch_hw( fst.reccount );

                /* Find corresponding CTLTAB entry */
                if (!(ctl = find_ctltab_entry( fn, ft, recfm )))
                {
                    /* No match in ctlfile; skip loading this file ... */
                    log_fst_skipped( &fst, opts );
                    continue; // (skip this file)
                }

                /* Show them the DUMPed file on the tape that we are LOADing... */
                log_fst_loaded( &fst, ctl, opts );
                ctl->loaded = true;

                /* Set translation codepage if file is TEXT format */
                if (ctl->filefmt == 'T')
                    if (strcasecmp( ctl->codepage, query_codepage()) != 0)
                        set_codepage( ctl->codepage );

                /* Create the empty host output file */
                if (!(ofile = fopen( ctl->hostfile, "wb" )))
                {
                    // "File open error: \"%s\": %s"
                    FWRMSG( stderr, HHC02623, "E", ctl->hostfile, strerror( errno ));
                    rc = -1;
                    continue;
                }

                /* Reset unwritten record bytes */
                recrem = 0;
            }
            /* Is this a PLCD header (DATA) block? */
            else if (1
                && blksize >= BLK_HDRSIZE
                && memcmp( bfr, PLCD_hdr, BLK_HDRSIZE ) == 0
            )
            {
                /* Continue writing the file if selected */
                if (ofile)
                    rc = load_file( ctl, ofile, recfm, blksize,
                        &recl, &recs, &recrem, &rszsplit, &rsz1 );
            }
            else
            {
                // "Invalid block encountered @ %d:%d"
                FWRMSG( stderr, HHC02636, "W",
                    opts->devblk.curfilen, opts->devblk.blockid );
                rc = -1;
                continue;
            }
        }
        else
        {
            // "    %s"
            WRMSG( HHC02633, "I", "@TM" );
            tapemark_count++;
        }
    }

    /* Close host output file if opened */
    if (ofile)
        VERIFY( fclose( ofile ) == 0 );

    /* Close input tape and issue completion message */
    tape_close( opts );

    if (rc == 0)
        // "%s complete"
        WRMSG( HHC02631, "I", verb( opts ));

    return rc;
}

/*-------------------------------------------------------------------*/
/*                          M A I N                                  */
/*-------------------------------------------------------------------*/
int main( int argc, char* argv[] )
{
    int      rc;
    OPTIONS  opts  = {0};

    INITIALIZE_UTILITY( UTILITY_NAME, UTILITY_DESC, &pgm );

    if (parse_parms( argc, argv, &opts ) != 0)
        return -1;

    switch (opts.verb)
    {
        case DUMP_VERB: rc = dodump( &opts ); break;
        case SCAN_VERB: rc = doscan( &opts ); break;
        case LOAD_VERB: rc = doload( &opts ); break;

        default:
            // "INTERNAL ERROR %s"
            FWRMSG( stderr, HHC02634, "E", "0002" );
            rc = -1;
            break;
    }

    return rc;
}
