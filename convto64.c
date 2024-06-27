/* CONVTO64.c   (C) Copyright "Fish" (David B. Trout), 2018-2019     */
/*              (C) and others 2020-2023                             */
/*              Convert compressed ckd to cckd64 format              */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/*   This program copies a 32-bit cckd or 64-bit cckd64 base image   */
/*   or shadow file to the new cckd64 format.  The input file must   */
/*   be either a compressed 32-bit cckd format base or shadow file   */
/*   or a compressed 64-bit cckd64 format base or shadow file, and   */
/*   the resulting output will always be a compressed cckd64 format  */
/*   base or shadow file.                                            */
/*                                                                   */
/*   The reason we support converting cckd64 to cckd64 is because    */
/*   a convenient side effect of our conversion is the resulting     */
/*   file ends up being completely compressed, with all of its L2    */
/*   tables in their preferred location.                             */
/*                                                                   */
/*   Another reason is to allow re-running the conversion again      */
/*   in case a newer version of ourselves does something different   */
/*   that the older version didn't and only cckd64 input happens     */
/*   to be available (because you deleted the original 32-bit cckd   */
/*   files after doing your original conversion the first time with  */
/*   the older version of this program).                             */
/*                                                                   */
/*      Usage:  convto64  [-r] [-c] [-q] [-v]  infile  outfile       */
/*                                                                   */
/*   PLEASE NOTE HOWEVER that this is a ONE-WAY CONVERSION! It can   */
/*   only convert 32-bit cckd format or 64-bit cckd64 format dasds   */
/*   to the new cckd64 format, but it CANNOT convert any new cckd64  */
/*   format dasds back to their original 32-bit cckd format!         */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"
#include "hercules.h"
#include "dasdblks.h"
#include "devtype.h"
#include "opcode.h"
#include "ccwarn.h"

#define UTILITY_NAME    "convto64"
#define UTILITY_DESC    "Convert compressed ckd to cckd64 format"

/*-------------------------------------------------------------------*/
/* Helper macros                                                     */
/*-------------------------------------------------------------------*/
#define SWAP_CCKD64_DEVHDR(p) do if (swaps_needed) cckd64_swapend_chdr( p );          while (0)
#define SWAP_CCKD64_L1TAB(p)  do if (swaps_needed) cckd64_swapend_l1( p, num_L1tab ); while (0)
#define SWAP_CCKD64_L2TAB(p)  do if (swaps_needed) cckd64_swapend_l2( p );            while (0)

/*-------------------------------------------------------------------*/
/* Global variables                                                  */
/*-------------------------------------------------------------------*/
S32   num_L1tab         = 0;            /* Number of L1 tab entries  */
S32   tracks_copied     = 0;            /* Number of tracks copied   */
bool  replace           = false;        /* true = replace output     */
bool  contigL2          = false;        /* true = contiguous L2s     */
bool  cckd64            = false;        /* true = CCKD64 input       */
bool  fba               = false;        /* true = FBA, false = CKD   */
bool  quiet             = false;        /* true = no track progress  */
bool  verbose           = false;        /* true = report each track  */
bool  swaps_needed      = false;        /* true = endian swap needed */
BYTE  trkbuf[64*1024]   = {0};          /* Track image data buffer   */

CCKD_DEVHDR     icdevhdr32    = {0};     /* CCKD   device header      */
CCKD_L1ENT*     iL1tab32      = NULL;    /* CCKD   Level 1 table      */
CCKD_L2ENT*     iL2tab32      = NULL;    /* CCKD   Level 2 tables     */

CCKD64_DEVHDR   icdevhdr      = {0};     /* CCKD64 device header      */
CCKD64_L1ENT*   iL1tab        = NULL;    /* CCKD64 Level 1 table      */
CCKD64_L2ENT*   iL2tab        = NULL;    /* CCKD64 Level 2 tables     */

CCKD64_DEVHDR   ocdevhdr      = {0};     /* Compressed Device Header  */
CCKD64_L1ENT*   oL1tab        = NULL;    /* Level 1 table             */
CCKD64_L2ENT*   oL2tab        = NULL;    /* Level 2 tables            */

/*-------------------------------------------------------------------*/
/* Display command syntax                                            */
/*-------------------------------------------------------------------*/
static int syntax( const char* pgm, const char* msgfmt, ... )
{
    // HHC02950:
    //
    //    Usage: %s [-r] [-c] [-q] [-v] infile outfile
    //      infile    input file
    //      outfile   output file
    //    options:
    //      -r     Replace output file
    //      -c     Write contiguous L2 tables
    //      -q     Minimal progress messages
    //      -v     Report every track copied

    if (msgfmt)
    {
        const int  chunksize  = 128;
        int        rc         = -1;
        int        buffsize   =  0;
        char*      msgbuf     = NULL;
        va_list    vargs;

        do
        {
            if (msgbuf) free( msgbuf );
            if (!(msgbuf = calloc( 1, buffsize += chunksize )))
                BREAK_INTO_DEBUGGER();

            va_end(   vargs );
            va_start( vargs, msgfmt );

            rc = vsnprintf( msgbuf, buffsize, msgfmt, vargs );
        }
        while (rc < 0 || rc >= buffsize);

        // "Syntax error: %s"
        FWRMSG( stderr, HHC02959, "E", msgbuf );
        free( msgbuf );
    }

    // "Usage: %s [-r] [-c] [-q] [-v] infile outfile"
    WRMSG( HHC02950, "I", pgm );
    return -1;
}

/*-------------------------------------------------------------------*/
/*       Convert 32-bit CCKD control block to 64-bit CCKD64          */
/*-------------------------------------------------------------------*/
static void cdevhdr_to_64()
{
    if (!cckd64)
    {
        memcpy( icdevhdr.cdh_vrm,  icdevhdr32.cdh_vrm,  sizeof( icdevhdr.cdh_vrm  ));
        memcpy( icdevhdr.cdh_cyls, icdevhdr32.cdh_cyls, sizeof( icdevhdr.cdh_cyls ));

        icdevhdr.num_L1tab    = icdevhdr32.num_L1tab;
        icdevhdr.num_L2tab    = icdevhdr32.num_L2tab;

        icdevhdr.cdh_opts     = icdevhdr32.cdh_opts;
        icdevhdr.cdh_size     = icdevhdr32.cdh_size;
        icdevhdr.cdh_used     = icdevhdr32.cdh_used;
        icdevhdr.cdh_nullfmt  = icdevhdr32.cdh_nullfmt;

        icdevhdr.free_off     = icdevhdr32.free_off;
        icdevhdr.free_total   = icdevhdr32.free_total;
        icdevhdr.free_largest = icdevhdr32.free_largest;
        icdevhdr.free_num     = icdevhdr32.free_num;
        icdevhdr.free_imbed   = icdevhdr32.free_imbed;

        icdevhdr.cmp_algo     = icdevhdr32.cmp_algo;
        icdevhdr.cmp_parm     = icdevhdr32.cmp_parm;
    }
}
static void L1tab_to_64()
{
    if (!cckd64)
    {
        CCKD_L1ENT*    L32  = iL1tab32;     // input 32
        CCKD64_L1ENT*  L64  = iL1tab;       // input 64

        S32  i;
        for (i=0; i < num_L1tab; ++i)
        {
            if (L32[i] == CCKD_MAXSIZE)
                L64[i] = CCKD64_MAXSIZE;
            else
                L64[i] = L32[i];
        }
    }
}
static void L2tabs_to_64()
{
    if (!cckd64)
    {
        int  n, i;
        for (n=0; n < num_L1tab; ++n)
        {
            for (i=0; i < 256; ++i)
            {
                if (iL2tab32[(n*256)+i].L2_trkoff == CCKD_MAXSIZE)
                    iL2tab[(n*256)+i].L2_trkoff = CCKD64_MAXSIZE;
                else
                    iL2tab[(n*256)+i].L2_trkoff = iL2tab32[(n*256)+i].L2_trkoff;

                iL2tab[(n*256)+i].L2_len  = iL2tab32[(n*256)+i].L2_len;
                iL2tab[(n*256)+i].L2_size = iL2tab32[(n*256)+i].L2_size;
            }
        }
    }
}

/*-------------------------------------------------------------------*/
/* Determine if endian swaps are going to be needed or not           */
/*-------------------------------------------------------------------*/
static bool are_swaps_needed( CCKD64_DEVHDR* icdevhdr )
{
    bool  running_on_big_endian_system  = are_big_endian();

    return
    (0
        || (1
            && running_on_big_endian_system
            && !(icdevhdr->cdh_opts & CCKD_OPT_BIGEND)
           )
        || (1
            && !running_on_big_endian_system
            && (icdevhdr->cdh_opts & CCKD_OPT_BIGEND)
           )
    );
}

/*-------------------------------------------------------------------*/
/* Function forward references                                       */
/*-------------------------------------------------------------------*/
static int  process_L2_tab( int trkblk, int ifd, CCKD64_L2ENT* iL2,
                                        int ofd, CCKD64_L2ENT* oL2 );

/*-------------------------------------------------------------------*/
/* Convert a cckd base or shadow file image to cckd64 format         */
/*-------------------------------------------------------------------*/
int main( int argc, char* argv[] )
{
char           *pgm;                    /* less any extension (.ext) */
char           *ifile, *ofile;          /* -> Input/Output file names*/
int             ifd,    ofd;            /* File descriptor integers  */
int             i, cyls;                /* Work variables            */
int             rc;                     /* Return code               */
char            pathname[MAX_PATH];     /* file path in host format  */
U32             imgtyp;                 /* Dasd image format         */
U32             size32;                 /* Work: CCKD   I/O size     */
U32             size;                   /* Work: CCKD64 I/O size     */
off_t           curpos;                 /* Work: saved file position */
off_t           tmppos;                 /* Work: lseek return code   */

CKD_DEVHDR      devhdr;                 /* Device header             */

    INITIALIZE_UTILITY( UTILITY_NAME, UTILITY_DESC, &pgm );

    /* Parse options */
    for (argc--, argv++ ; argc > 0 ; argc--, argv++)
    {
        /* End of options? */
        if (argv[0][0] != '-')
            break;

        if (0
            || strcasecmp( argv[0], "-q"      ) == 0
            || strcasecmp( argv[0], "--quiet" ) == 0
        )
        {
            quiet = true;       /* Suppress track progress */
        }
        else if (0
            || strcasecmp( argv[0], "-v"        ) == 0
            || strcasecmp( argv[0], "--verbose" ) == 0
        )
        {
            verbose = true;     /* report each track's progress */
        }
        else if (0
            || strcasecmp( argv[0], "-r"        ) == 0
            || strcasecmp( argv[0], "--replace" ) == 0
        )
        {
            replace = true;     /* replace output file */
        }
        else if (0
            || strcasecmp( argv[0], "-c"       ) == 0
            || strcasecmp( argv[0], "--contig" ) == 0
        )
        {
            contigL2 = true;    /* contiguous L2s */
        }
        else
            return syntax( pgm, "unrecognized option: %s",
                argv[0] );
    }

    /* Parse arguments */
    if (argc < 1)
        return syntax( pgm, "%s", "missing input-file specification" );
    else if (argc > 2)
        return syntax( pgm, "extraneous parameter: %s", argv[2] );
    else
    {
        ifile = argv[0];
        if (argc < 2)
            return syntax( pgm, "%s", "missing output-file specification" );
        ofile = argv[1];
    }

    /* Open input file and verify correct format */
    hostpath( pathname, ifile, sizeof( pathname ));
    if ((ifd = HOPEN( pathname, O_RDONLY | O_BINARY )) < 0)
    {
        // "Error in function %s: %s"
        FWRMSG( stderr, HHC02958, "E", "open()", strerror( errno ));
        return -1;
    }

    // "Converting \"%s\" to CCKD64 file format..."
    WRMSG( HHC02962, "I", pathname );

    /* Read the input file's device header */
    size = (U32) sizeof( devhdr );
    if ((rc = read( ifd, &devhdr, size )) < (int) size)
    {
        // "Error in function %s: %s"
        FWRMSG( stderr, HHC02958, "E", "read()", strerror( errno ));
        return -1;
    }

    /* Input must be a compressed type */
    if (!((imgtyp = dh_devid_typ( devhdr.dh_devid )) & ANY_CMP_OR_SF_TYP))
    {
        // "Dasd image file format unsupported or unrecognized: %s"
        FWRMSG( stderr, HHC02960, "E", ifile );
        return syntax( pgm, NULL );
    }

    /* Remember whether input is CCKD or CCKD64 */
    if (imgtyp & ANY64_CMP_OR_SF_TYP)
        cckd64 = true;

    /* Remember whether input is FBA or CKD */
    if (imgtyp & FBA32_CMP_OR_SF_TYP)
        fba = true;

    /* Open ouput file */
    hostpath( pathname, ofile, sizeof( pathname ));
    if ((ofd = HOPEN( pathname,
        O_CREAT | O_WRONLY | O_BINARY | (replace ? 0 : O_EXCL),
        S_IRUSR | S_IWUSR | S_IRGRP)) < 0)
    {
        // "Error in function %s: %s"
        FWRMSG( stderr, HHC02958, "E", "open()", strerror( errno ));
        return -1;
    }

    /* Convert the output file's device header */
    if      (imgtyp & CKD_C370_TYP) memcpy(   devhdr.dh_devid,
        dh_devid_str( CKD_C064_TYP ), sizeof( devhdr.dh_devid ));

    else if (imgtyp & CKD_S370_TYP) memcpy(   devhdr.dh_devid,
        dh_devid_str( CKD_S064_TYP ), sizeof( devhdr.dh_devid ));

    else if (imgtyp & FBA_C370_TYP) memcpy(   devhdr.dh_devid,
        dh_devid_str( FBA_C064_TYP ), sizeof( devhdr.dh_devid ));

    else if (imgtyp & FBA_S370_TYP) memcpy(   devhdr.dh_devid,
        dh_devid_str( FBA_S064_TYP ), sizeof( devhdr.dh_devid ));

    /* Write the output file's device header */
    size = (U32) sizeof( devhdr );
    if ((rc = write( ofd, &devhdr, size )) < (int) size)
    {
        // "Error in function %s: %s"
        FWRMSG( stderr, HHC02958, "E", "write()", strerror( errno ));
        return -1;
    }

    /* Read the input file's compressed device header */
    if (cckd64)
    {
        size = (U32) sizeof( icdevhdr );
        if ((rc = read( ifd, &icdevhdr, size )) < (int) size)
        {
            // "Error in function %s: %s"
            FWRMSG( stderr, HHC02958, "E", "read()", strerror( errno ));
            return -1;
        }
    }
    else // (32-bit CCKD)
    {
        size32 = (U32) sizeof( icdevhdr32 );
        if ((rc = read( ifd, &icdevhdr32, size32 )) < (int) size32)
        {
            // "Error in function %s: %s"
            FWRMSG( stderr, HHC02958, "E", "read()", strerror( errno ));
            return -1;
        }

        /* Convert 32-bit CCKD compressed device header to 64-bit CCKD64 */
        cdevhdr_to_64();
    }

    /* Determine whether endian swaps are going to be needed or not */
    swaps_needed = are_swaps_needed( &icdevhdr );

    /* Swap input file's compressed device header before processing */
    SWAP_CCKD64_DEVHDR( &icdevhdr );

    /* Copy the input file's compressed device header fields
       to the output file's compressed device header fields.
    */
    memcpy( ocdevhdr.cdh_vrm, icdevhdr.cdh_vrm, sizeof( ocdevhdr.cdh_vrm ));

    ocdevhdr.cdh_opts   =  icdevhdr.cdh_opts;
    ocdevhdr.num_L1tab  =  icdevhdr.num_L1tab;
    ocdevhdr.num_L2tab  =  icdevhdr.num_L2tab;

    ocdevhdr.cdh_size = 0;      // (we'll fill this in later)
    ocdevhdr.cdh_used = 0;      // (we'll fill this in later)

    FETCH_LE_FW( cyls, icdevhdr.cdh_cyls );
    STORE_LE_FW( ocdevhdr.cdh_cyls, cyls );

    ocdevhdr.cdh_nullfmt = icdevhdr.cdh_nullfmt;
    ocdevhdr.cmp_algo    = icdevhdr.cmp_algo;
    ocdevhdr.cmp_parm    = icdevhdr.cmp_parm;

    /* We don't copy free space so all of these will be zero */

    ocdevhdr.free_off     = 0;
    ocdevhdr.free_total   = 0;
    ocdevhdr.free_largest = 0;
    ocdevhdr.free_num     = 0;
    ocdevhdr.free_imbed   = 0;

    /* Write the output file's compressed device header.
       We'll fix it up and rewrite it when we're done.
    */
    size = (U32) sizeof( ocdevhdr );
    if ((rc = write( ofd, &ocdevhdr, size )) < (int) size)
    {
        // "Error in function %s: %s"
        FWRMSG( stderr, HHC02958, "E", "write()", strerror( errno ));
        return -1;
    }

    /* Save the number of L1 table entries */
    num_L1tab = icdevhdr.num_L1tab;
    EXTGUIMSG( "TRKS=%d\n", num_L1tab * 256 );

    /* Allocate room for the output file's L1 table */
    size = (U32) (num_L1tab * sizeof( CCKD64_L1ENT ));
    if (!(oL1tab = (CCKD64_L1ENT*) calloc( 1, size )))
    {
        // "Error in function %s: %s"
        FWRMSG( stderr, HHC02958, "E", "calloc()", strerror( errno ));
        return -1;
    }

    /* Write out (make room for) the output file's L1 table.
       We'll fix it up and rewrite it later when we're done. */
    if ((rc = write( ofd, oL1tab, size )) < (int) size )
    {
        // "Error in function %s: %s"
        FWRMSG( stderr, HHC02958, "E", "write()", strerror( errno ));
        return -1;
    }

    /* Allocate room for input file's 64-bit L1 table */
    size = (U32) (num_L1tab * sizeof( CCKD64_L1ENT ));
    if (!(iL1tab = (CCKD64_L1ENT*) calloc( 1, size )))
    {
        // "Error in function %s: %s"
        FWRMSG( stderr, HHC02958, "E", "calloc()", strerror( errno ));
        return -1;
    }

    /* Read the input file's L1 table */
    if (cckd64)
    {
        if ((rc = read( ifd, iL1tab, size )) < (int) size)
        {
            // "Error in function %s: %s"
            FWRMSG( stderr, HHC02958, "E", "read()", strerror( errno ));
            return -1;
        }
    }
    else // (32-bit CCKD)
    {
        /* Allocate room for input file's 32-bit L1 table */
        size32 = (U32) (num_L1tab * sizeof( CCKD_L1ENT ));
        if (!(iL1tab32 = (CCKD_L1ENT*) calloc( 1, size32 )))
        {
            // "Error in function %s: %s"
            FWRMSG( stderr, HHC02958, "E", "calloc()", strerror( errno ));
            return -1;
        }

        /* Read the input file's 32-bit L1 table */
        if ((rc = read( ifd, iL1tab32, size32 )) < (int) size32)
        {
            // "Error in function %s: %s"
            FWRMSG( stderr, HHC02958, "E", "read()", strerror( errno ));
            return -1;
        }

        /* Convert the input file's 32-bit L1 table to 64-bit L1 table */
        L1tab_to_64();
        free( iL1tab32 );
        iL1tab32 = NULL;
    }

    /* Swap the input file's L1 table before processing it */
    SWAP_CCKD64_L1TAB( iL1tab );

    /* Allocate storage for the input and output file's L2 tables */
    size = (U32) (num_L1tab * (256 * sizeof( CCKD64_L2ENT )));
    if (!(iL2tab = (CCKD64_L2ENT*) malloc( size )))
    {
        // "Error in function %s: %s"
        FWRMSG( stderr, HHC02958, "E", "malloc()", strerror( errno ));
        return -1;
    }
    memset( iL2tab, 0xFF, size );
    if (!(oL2tab = (CCKD64_L2ENT*) malloc( size )))
    {
        // "Error in function %s: %s"
        FWRMSG( stderr, HHC02958, "E", "malloc()", strerror( errno ));
        return -1;
    }
    memset( oL2tab, 0xFF, size );
    if (!cckd64)
    {
        size32 = (U32) (num_L1tab * (256 * sizeof( CCKD_L2ENT )));
        if (!(iL2tab32 = (CCKD_L2ENT*) malloc( size32 )))
        {
            // "Error in function %s: %s"
            FWRMSG( stderr, HHC02958, "E", "malloc()", strerror( errno ));
            return -1;
        }
        memset( iL2tab32, 0xFF, size32 );
    }

    // "Gathering L2 tables..."
    WRMSG( HHC02951, "I" );

    /* Read in all of the input file's L2 tables */
    if (cckd64)
        size   = (U32) (256 * sizeof( CCKD64_L2ENT ));  // (i/p L2 table size)
    else
        size32 = (U32) (256 * sizeof( CCKD_L2ENT   ));  // (i/p L2 table size)
    for (i=0; i < num_L1tab; i++)
    {
        /* Skip non-existent L2 tables */
        if (iL1tab[i] == 0)
        {
            oL1tab[i] = 0;
            continue;
        }
        if (iL1tab[i] == ULLONG_MAX)
        {
            oL1tab[i] = ULLONG_MAX;
            continue;
        }
        if ((tmppos = lseek( ifd, iL1tab[i], SEEK_SET )) < 0)
        {
            // "Error in function %s: %s"
            FWRMSG( stderr, HHC02958, "E", "lseek()", strerror( errno ));
            return -1;
        }
        if (cckd64)
        {
            if ((rc = read( ifd, &iL2tab[i*256], size )) < (int) size)
            {
                // "Error in function %s: %s"
                FWRMSG( stderr, HHC02958, "E", "read()", strerror( errno ));
                return -1;
            }
        }
        else // (32-bit CCKD)
        {
            if ((rc = read( ifd, &iL2tab32[i*256], size32 )) < (int) size32)
            {
                // "Error in function %s: %s"
                FWRMSG( stderr, HHC02958, "E", "read()", strerror( errno ));
                return -1;
            }
        }
    }

    /* Convert 32-bit L2 tables to 64-bit CCKD64 */
    if (!cckd64)
    {
        L2tabs_to_64();
        free( iL2tab32 );
        iL2tab32 = NULL;
    }

    /* Set output size = size of 64-bit CCKD64 L2 table */
    size = (U32) (256 * sizeof( CCKD64_L2ENT ));

    if (contigL2)
    {
        /* Write out all L2 tables to the output file (even empty ones).
           We'll rewrite them all with their correct values later, once
           their track data has been copied in our loop further below.
           This ensures that none of the L2 tables will ever need to be
           created or relocated since: 1) they'll always already exist,
           and 2) they'll always exist in their preferred file location
           (i.e. immediately following the L1 table).
        */
        for (i=0; i < num_L1tab; ++i)
        {
            if ((off_t)(oL1tab[i] = lseek( ofd, 0, SEEK_CUR )) < 0)
            {
                // "Error in function %s: %s"
                FWRMSG( stderr, HHC02958, "E", "lseek()", strerror( errno ));
                return -1;
            }
            if ((rc = write( ofd, &oL2tab[i*256], size )) < (int) size )
            {
                // "Error in function %s: %s"
                FWRMSG( stderr, HHC02958, "E", "write()", strerror( errno ));
                return -1;
            }
        }
    }

    // "Copying L2 tables and associated track data..."
    WRMSG( HHC02952, "I" );

    /* Process all of the L2 tables... */
    for (i=0; i < num_L1tab; i++)
    {
        if (!contigL2)
        {
            if (0
                || iL1tab[i] == 0
                || iL1tab[i] == ULLONG_MAX
            )
                continue;   /* (skip L2 tables that don't exist) */

            /* Make room for this L2 table in the output file.
               We'll re-write it with the correct values after
               it's been processed.
            */
            if ((off_t)(oL1tab[i] = lseek( ofd, 0, SEEK_CUR )) < 0)
            {
                // "Error in function %s: %s"
                FWRMSG( stderr, HHC02958, "E", "lseek()", strerror( errno ));
                return -1;
            }
            if ((rc = write( ofd, &oL2tab[i*256], size )) < (int) size )
            {
                // "Error in function %s: %s"
                FWRMSG( stderr, HHC02958, "E", "write()", strerror( errno ));
                return -1;
            }
        }

        /* Report progress... */
        if (!quiet && !verbose) // (!verbose since verbose reports each track)
        {
            if (fba)
                // "Copying %s %d - %d ..."
                WRMSG( HHC02953, "I", "blocks",
                    (CFBA_BLKS_PER_GRP * ( i    * 256)),
                    (CFBA_BLKS_PER_GRP * ((i+1) * 256)) - 1 );
            else
                // "Copying %s %d - %d ..."
                WRMSG( HHC02953, "I", "tracks", (i*256), ((i+1)*256) - 1 );
        }

        /* Copy all of this L2 table's track data... */
        if (process_L2_tab( i*256, ifd, &iL2tab[i*256], ofd, &oL2tab[i*256] ) < 0)
        {
            // "Error in function %s: %s"
            FWRMSG( stderr, HHC02958, "E", "process_L2_tab()", strerror( errno ));
            return -1;
        }

        /* Save the current output file position */
        if ((curpos = lseek( ofd, 0, SEEK_CUR )) < 0)
        {
            // "Error in function %s: %s"
            FWRMSG( stderr, HHC02958, "E", "lseek()", strerror( errno ));
            return -1;
        }

        /* Swap the output L2 table before writing it out */
        SWAP_CCKD64_L2TAB( oL2tab );

        /* Re-write output file's L2 table with now accurate values */
        if ((tmppos = lseek( ofd, oL1tab[i], SEEK_SET )) < 0)
        {
            // "Error in function %s: %s"
            FWRMSG( stderr, HHC02958, "E", "lseek()", strerror( errno ));
            return -1;
        }
        ASSERT( tmppos == oL1tab[i] );
        if ((rc = write( ofd, &oL2tab[i*256], size )) < (int) size )
        {
            // "Error in function %s: %s"
            FWRMSG( stderr, HHC02958, "E", "write()", strerror( errno ));
            return -1;
        }

        /* Restore output file position back to where it was */
        if ((tmppos = lseek( ofd, curpos, SEEK_SET )) < 0)
        {
            // "Error in function %s: %s"
            FWRMSG( stderr, HHC02958, "E", "lseek()", strerror( errno ));
            return -1;
        }
        ASSERT( tmppos == curpos );

        /* Flush what we've got so far */
        fdatasync( ofd );
    }
    /* (end process L2 tables) */

    // "%"PRIu32" tracks copied"
    WRMSG( HHC02957, "I", tracks_copied );

    /* Now that all of the output file's data has been written,
       save the current file position as the output file's size.
    */
    if ((curpos = lseek( ofd, 0, SEEK_CUR )) < 0)
    {
        // "Error in function %s: %s"
        FWRMSG( stderr, HHC02958, "E", "lseek()", strerror( errno ));
        return -1;
    }

    /* Fix up the output file's compressed device header */
    ocdevhdr.cdh_size = curpos;
    ocdevhdr.cdh_used = curpos;

    /* Swap the output compressed device header before writing it */
    SWAP_CCKD64_DEVHDR( &ocdevhdr );

    // "Writing cckd64 compressed device header..."
    WRMSG( HHC02954, "I" );

    /* Re-write output file's compressed device header */
    if ((tmppos = lseek( ofd, CCKD64_DEVHDR_POS, SEEK_SET )) < 0)
    {
        // "Error in function %s: %s"
        FWRMSG( stderr, HHC02958, "E", "lseek()", strerror( errno ));
        return -1;
    }
    size = (U32) sizeof( ocdevhdr );
    if ((rc = write( ofd, &ocdevhdr, size )) < (int) size)
    {
        // "Error in function %s: %s"
        FWRMSG( stderr, HHC02958, "E", "write()", strerror( errno ));
        return -1;
    }

    /* Swap the output L1 table before writing it out */
    SWAP_CCKD64_L1TAB( oL1tab );

    // "Writing L1 table..."
    WRMSG( HHC02955, "I" );

    /* Re-write the output file's now accurate L1 table */
    size = (U32) (num_L1tab * sizeof( CCKD64_L1ENT ));
    if ((rc = write( ofd, oL1tab, size )) < (int) size )
    {
        // "Error in function %s: %s"
        FWRMSG( stderr, HHC02958, "E", "write()", strerror( errno ));
        return -1;
    }

    /* We're done! Close our input and output files and exit! */
    close( ofd );
    close( ifd );

    // "DASD operation completed"
    WRMSG( HHC02961, "I" );

    return 0;
}

/*-------------------------------------------------------------------*/
/* Process L2 table                                                  */
/*-------------------------------------------------------------------*/
static int process_L2_tab( int trkblk, int ifd, CCKD64_L2ENT* iL2,
                                       int ofd, CCKD64_L2ENT* oL2 )
{
    int  rc, i, size;                   /* Work variables            */
    S64  tmppos;                        /* Work: lseek return code   */

    /* Swap input L2 table if needed before processing */
    SWAP_CCKD64_L2TAB( iL2 );

    /* For each L2 table entry... */
    for (i=0; i < 256; i++)
    {
        EXTGUIMSG( "TRK=%d\n", trkblk+i );

        oL2[i].L2_len  = iL2[i].L2_len;
        oL2[i].L2_size = iL2[i].L2_size;

        /* Skip copying tracks that don't exist */
        if (0
            || iL2[i].L2_trkoff == 0
            || iL2[i].L2_trkoff == ULLONG_MAX
        )
            continue; /* (no track data to copy) */

        /* Report every track/blkgrp copied if requested... */
        if (verbose && !quiet)
            // "Copying %s %d data..."
            WRMSG( HHC02956, "I", fba ? "blkgrp" : "track", trkblk+i );

        tracks_copied++;    /* Count tracks copied */

        /* Seek to where this track's data resides */
        if ((tmppos = lseek( ifd, iL2[i].L2_trkoff, SEEK_SET )) < 0)
        {
            // "Error in function %s: %s"
            FWRMSG( stderr, HHC02958, "E", "lseek()", strerror( errno ));
            return -1;
        }

        /* Read this track's data */
        size = (U32) max( iL2[i].L2_len, iL2[i].L2_size );

        if ((rc = read( ifd, trkbuf, size )) < size)
        {
            // "Error in function %s: %s"
            FWRMSG( stderr, HHC02958, "E", "read()", strerror( errno ));
            return -1;
        }

        /* Retrieve the current output file position */
        if ((off_t)(oL2[i].L2_trkoff = lseek( ofd, 0, SEEK_CUR )) < 0)
        {
            // "Error in function %s: %s"
            FWRMSG( stderr, HHC02958, "E", "lseek()", strerror( errno ));
            return -1;
        }

        /* Write the track data to the output file */
        if ((rc = write( ofd, trkbuf, size )) < size )
        {
            // "Error in function %s: %s"
            FWRMSG( stderr, HHC02958, "E", "write()", strerror( errno ));
            return -1;
        }
    }

    return 0;
}
