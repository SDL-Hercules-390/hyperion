/* CONVTO64.c (C) Copyright "Fish" (David B. Trout), 2018            */
/*                Convert cckd file to cckd64 format                 */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/*   This program copies a cckd base or shadow file to the           */
/*   new cckd64 format.  The input file must be a compressed         */
/*   cckd format base or shadow file and the resulting output        */
/*   will be a corresponding compressed cckd64 format base or        */
/*   shadow file.                                                    */
/*                                                                   */
/*   Usage:       convto64  [-q] [-v]  infile  outfile               */
/*                                                                   */
/*   PLEASE ALSO NOTE that this is a one-way conversion.  It         */
/*   can only convert 32-bit cckd format dasds to cckd64 format,     */
/*   but it does NOT (cannot) convert the new cckd64 format back     */
/*   into the old 32-bit cckd format!                                */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"
#include "hercules.h"
#include "dasdblks.h"
#include "devtype.h"
#include "opcode.h"
#include "ccwarn.h"

#define UTILITY_NAME    "convto64"
#define UTILITY_DESC    "Convert cckd file to cckd64 format"

/*-------------------------------------------------------------------*/
/* Helper macros                                                     */
/*-------------------------------------------------------------------*/
#define SWAP_CCKD_DEVHDR(p)   do if (swaps_needed) cckd_swapend_chdr  ( p );          while (0)
#define SWAP_CCKD64_DEVHDR(p) do if (swaps_needed) cckd64_swapend_chdr( p );          while (0)

#define SWAP_CCKD_L1TAB(p)    do if (swaps_needed) cckd_swapend_l1  ( p, num_L1tab ); while (0)
#define SWAP_CCKD64_L1TAB(p)  do if (swaps_needed) cckd64_swapend_l1( p, num_L1tab ); while (0)

#define SWAP_CCKD_L2TAB(p)    do if (swaps_needed) cckd_swapend_l2  ( p );            while (0)
#define SWAP_CCKD64_L2TAB(p)  do if (swaps_needed) cckd64_swapend_l2( p );            while (0)

/*-------------------------------------------------------------------*/
/* Function forward references                                       */
/*-------------------------------------------------------------------*/
int  process_L2_tab( int trkblk, int ifd, CCKD_L2ENT*   iL2,
                                 int ofd, CCKD64_L2ENT* oL2 );

/*-------------------------------------------------------------------*/
/* Global variables                                                  */
/*-------------------------------------------------------------------*/
S32   num_L1tab         = 0;            /* Number of L1 tab entries  */
bool  fba               = false;        /* true = FBA, false = CKD   */
bool  quiet             = false;        /* true = no track progress  */
bool  verbose           = false;        /* true = report each track  */
bool  swaps_needed      = false;        /* true = endian swap needed */
BYTE  trkbuf[64*1024]   = {0};          /* Track image data buffer   */

/*-------------------------------------------------------------------*/
/* Display command syntax                                            */
/*-------------------------------------------------------------------*/
int syntax( const char* pgm, const char* msgfmt, ... )
{
    // HHC02950:
    //
    //    Usage: %s [-q] [-v] infile outfile
    //      infile    input file
    //      outfile   output file
    //    options:
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
            if (!(msgbuf = malloc( buffsize += chunksize )))
                BREAK_INTO_DEBUGGER();

            va_end(   vargs );
            va_start( vargs, msgfmt );

            rc = vsnprintf( msgbuf, buffsize, msgfmt, vargs );
        }
        while (rc < 0 || rc >= buffsize);

        // "Syntax error: %s"
        FWRMSG( stderr, HHC02594, "E", msgbuf );
        free( msgbuf );
    }

    // "Usage: %s [-q] [-v] infile outfile"
    WRMSG( HHC02950, "I", pgm );
    return -1;
}

/*-------------------------------------------------------------------*/
/* Determine if running on a big endian system or not                */
/*-------------------------------------------------------------------*/
bool are_big_endian()
{
    static union
    {
        uint32_t  ui32;
        char      b[4];
    }
    test = {0x01020304};

    return (0x01 == test.b[0]);
}

/*-------------------------------------------------------------------*/
/* Determine if endian swaps are going to be needed or not           */
/*-------------------------------------------------------------------*/
bool are_swaps_needed( CCKD_DEVHDR* icdevhdr )
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
/* Convert a cckd shadow file to cckd64 format                       */
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
U32             size;                   /* Work: I/O size            */
S64             savepos;                /* Work: saved file position */
S64             newpos;                 /* Work: lseek return code   */

CKD_DEVHDR      devhdr;                 /* Device header             */

CCKD_DEVHDR     icdevhdr;               /* Compressed Device Header  */
CCKD_L1ENT*     iL1tab;                 /* Level 1 table             */
CCKD_L2ENT*     iL2tab;                 /* Level 2 table             */

CCKD64_DEVHDR   ocdevhdr  = {0};        /* Compressed Device Header  */
CCKD64_L1ENT*   oL1tab    = NULL;       /* Level 1 table             */
CCKD64_L2ENT*   oL2tab    = NULL;       /* Level 2 table             */

    INITIALIZE_UTILITY( UTILITY_NAME, UTILITY_DESC, &pgm );

    /* Process the arguments */
    for (argc--, argv++ ; argc > 0 ; argc--, argv++)
    {
        /* End of options? */
        if (argv[0][0] != '-')
            break;

        if (0
            || strcmp( argv[0], "-q"     ) == 0
            || strcmp( argv[0], "-quiet" ) == 0
        )
        {
            quiet = true;       /* Suppress track progress */
        }
        else if (0
            || strcmp( argv[0], "-v"       ) == 0
            || strcmp( argv[0], "-verbose" ) == 0
        )
        {
            verbose = true;     /* report each track's progress */
        }
        else
            return syntax( pgm, "unrecognized/unsupported option: %s",
                argv[0] );
    }

    /* Get the file names */
    if (argc < 2)
        return syntax( pgm, "%s", "missing input-file specification" );
    else if (argc > 3)
        return syntax( pgm, "extraneous parameter: %s", argv[2] );
    else
    {
        ifile = argv[0];
        ofile = argv[1];
    }

    /* Open input file and verify correct format */
    hostpath( pathname, ifile, sizeof( pathname ));
    if ((ifd = HOPEN( pathname, O_RDONLY | O_BINARY )) < 0)
    {
        // "Error in function %s: %s"
        FWRMSG( stderr, HHC02412, "E", "open()", strerror( errno ));
        return -1;
    }

    /* Read the input file's device header */
    size = (U32) sizeof( devhdr );
    if ((rc = read( ifd, &devhdr, size )) < (int) size)
    {
        // "Error in function %s: %s"
        FWRMSG( stderr, HHC02412, "E", "read()", strerror( errno ));
        return -1;
    }

    /* Input must be a cckd type */
    if (!((imgtyp = dh_devid_typ( devhdr.dh_devid )) & ANY32_CMP_OR_SF_TYP))
    {
        // "Dasd image file format unsupported or unrecognized: %s"
        FWRMSG( stderr, HHC02424, "E", ifile );
        return syntax( pgm, NULL );
    }

    /* Open ouput file */
    hostpath( pathname, ofile, sizeof( pathname ));
    if ((ofd = HOPEN( pathname,
        O_CREAT | O_WRONLY | O_BINARY | O_EXCL,
        S_IRUSR | S_IWUSR | S_IRGRP)) < 0)
    {
        // "Error in function %s: %s"
        FWRMSG( stderr, HHC02412, "E", "open()", strerror( errno ));
        return -1;
    }

    /* Convert the output file's device header */
    if      (imgtyp & CKD_C370_TYP) memcpy(   devhdr.dh_devid,
        dh_devid_str( CKD_C064_TYP ), sizeof( devhdr.dh_devid ));

    else if (imgtyp & CKD_S370_TYP) memcpy(   devhdr.dh_devid,
        dh_devid_str( CKD_S064_TYP ), sizeof( devhdr.dh_devid ));

    else if (imgtyp & FBA_S370_TYP) memcpy(   devhdr.dh_devid,
        dh_devid_str( FBA_S064_TYP ), sizeof( devhdr.dh_devid ));

    else if (imgtyp & FBA_S370_TYP) memcpy(   devhdr.dh_devid,
        dh_devid_str( FBA_S064_TYP ), sizeof( devhdr.dh_devid ));

    /* Remember whether FBA or CKD */
    if (imgtyp & FBA32_CMP_OR_SF_TYP)
        fba = true;

    /* Write the output file's device header */
    size = (U32) sizeof( devhdr );
    if ((rc = write( ofd, &devhdr, size )) < (int) size)
    {
        // "Error in function %s: %s"
        FWRMSG( stderr, HHC02412, "E", "write()", strerror( errno ));
        return -1;
    }

    /* Read the input file's compressed device header */
    size = (U32) sizeof( icdevhdr );
    if ((rc = read( ifd, &icdevhdr, size )) < (int) size)
    {
        // "Error in function %s: %s"
        FWRMSG( stderr, HHC02412, "E", "read()", strerror( errno ));
        return -1;
    }

    /* Determine whether endian swaps are going to be needed or not */
    swaps_needed = are_swaps_needed( &icdevhdr );

    /* Swap input file's compressed device header before processing */
    SWAP_CCKD_DEVHDR( &icdevhdr );

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
        FWRMSG( stderr, HHC02412, "E", "write()", strerror( errno ));
        return -1;
    }

    /* Save the number of L1 table entries */
    num_L1tab = icdevhdr.num_L1tab;

    /* Allocate room for the L1 tables */
    size = (U32) (num_L1tab * sizeof( CCKD_L1ENT ));
    if (!(iL1tab = (CCKD_L1ENT*) malloc( size )))
    {
        // "Error in function %s: %s"
        FWRMSG( stderr, HHC02412, "E", "malloc()", strerror( errno ));
        return -1;
    }
    size = (U32) (num_L1tab * sizeof( CCKD64_L1ENT ));
    if (!(oL1tab = (CCKD64_L1ENT*) malloc( size )))
    {
        // "Error in function %s: %s"
        FWRMSG( stderr, HHC02412, "E", "malloc()", strerror( errno ));
        return -1;
    }

    /* Read the input file's L1 table */
    size = (U32) (num_L1tab * sizeof( CCKD_L1ENT ));
    if ((rc = read( ifd, iL1tab, size )) < (int) size)
    {
        // "Error in function %s: %s"
        FWRMSG( stderr, HHC02412, "E", "read()", strerror( errno ));
        return -1;
    }

    /* Swap the input file's L1 table before processing it */
    SWAP_CCKD_L1TAB( iL1tab );

    /* Write out (make room for) the output file's L1 table.
       We'll fix it up and rewrite it later when we're done.
    */
    size = (U32) (num_L1tab * sizeof( CCKD64_L1ENT ));
    if ((rc = write( ofd, oL1tab, size )) < (int) size )
    {
        // "Error in function %s: %s"
        FWRMSG( stderr, HHC02412, "E", "write()", strerror( errno ));
        return -1;
    }

    /* Allocate storage for the L2 tables */
    size = (U32) (num_L1tab * (256 * sizeof( CCKD_L2ENT )));
    if (!(iL2tab = (CCKD_L2ENT*) malloc( size )))
    {
        // "Error in function %s: %s"
        FWRMSG( stderr, HHC02412, "E", "malloc()", strerror( errno ));
        return -1;
    }
    size = (U32) (num_L1tab * (256 * sizeof( CCKD64_L2ENT )));
    if (!(oL2tab = (CCKD64_L2ENT*) malloc( size )))
    {
        // "Error in function %s: %s"
        FWRMSG( stderr, HHC02412, "E", "malloc()", strerror( errno ));
        return -1;
    }

    // "Gathering L2 tables..."
    WRMSG( HHC02951, "I" );

    /* Read in all of the input file's L2 tables */
    size = (U32) (256 * sizeof( CCKD_L2ENT ));  // (i/p L2 table size)
    for (i=0; i < num_L1tab; i++)
    {
        /* Skip non-existent L2 tables */
        if (iL1tab[i] == 0)
        {
            oL1tab[i] = 0;
            continue;
        }
        if (iL1tab[i] == ULONG_MAX)
        {
            oL1tab[i] = ULLONG_MAX;
            continue;
        }

        if ((newpos = lseek( ifd, iL1tab[i], SEEK_SET )) < 0)
        {
            // "Error in function %s: %s"
            FWRMSG( stderr, HHC02412, "E", "lseek()", strerror( errno ));
            return -1;
        }
        if ((rc = read( ifd, &iL2tab[i*256], size )) < (int) size)
        {
            // "Error in function %s: %s"
            FWRMSG( stderr, HHC02412, "E", "read()", strerror( errno ));
            return -1;
        }
    }

    // "Copying L2 tables and associated track data..."
    WRMSG( HHC02952, "I" );

    /* Process all of the L2 tables... */
    size = (U32) (256 * sizeof( CCKD64_L2ENT )); // (o/p L2 table size)
    for (i=0; i < num_L1tab; i++)
    {
        if (0
            || iL1tab[i] == 0
            || iL1tab[i] == ULONG_MAX
        )
            continue;   /* (skip L2 tables that don't exist) */

        /* Make room for this L2 table in the output file.
           We'll re-write it with the correct values after
           it's been processed.
        */
        if ((oL1tab[i] = lseek( ofd, 0, SEEK_CUR )) < 0)
        {
            // "Error in function %s: %s"
            FWRMSG( stderr, HHC02412, "E", "lseek()", strerror( errno ));
            return -1;
        }
        if ((rc = write( ofd, &oL2tab[i*256], size )) < (int) size )
        {
            // "Error in function %s: %s"
            FWRMSG( stderr, HHC02412, "E", "write()", strerror( errno ));
            return -1;
        }

        /* Report progress... */
        if (!quiet && !verbose)
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
            FWRMSG( stderr, HHC02412, "E", "process_L2_tab()", strerror( errno ));
            return -1;
        }

        /* Save the output file position after L2 processing */
        if ((savepos = lseek( ofd, 0, SEEK_CUR )) < 0)
        {
            // "Error in function %s: %s"
            FWRMSG( stderr, HHC02412, "E", "lseek()", strerror( errno ));
            return -1;
        }

        /* Swap the output L2 table before writing it out */
        SWAP_CCKD64_L2TAB( oL2tab );

        /* Re-write output file's L2 table with now accurate values */
        if ((newpos = lseek( ofd, oL1tab[i], SEEK_SET )) < 0)
        {
            // "Error in function %s: %s"
            FWRMSG( stderr, HHC02412, "E", "lseek()", strerror( errno ));
            return -1;
        }
        if ((rc = write( ofd, &oL2tab[i*256], size )) < (int) size )
        {
            // "Error in function %s: %s"
            FWRMSG( stderr, HHC02412, "E", "write()", strerror( errno ));
            return -1;
        }

        /* Reset the output file's position back to where it was */
        if ((newpos = lseek( ofd, savepos, SEEK_SET )) < 0)
        {
            // "Error in function %s: %s"
            FWRMSG( stderr, HHC02412, "E", "lseek()", strerror( errno ));
            return -1;
        }

        /* Flush what we've got so far */
        fdatasync( ofd );
    }
    /* (end process L2 tables) */

    /* Now that all of the output file's data has been written,
       save the current file position as the output file's size.
    */
    if ((savepos = lseek( ofd, 0, SEEK_CUR )) < 0)
    {
        // "Error in function %s: %s"
        FWRMSG( stderr, HHC02412, "E", "lseek()", strerror( errno ));
        return -1;
    }

    /* Fix up the output file's compressed device header */
    ocdevhdr.cdh_size = savepos;
    ocdevhdr.cdh_used = savepos;

    /* Swap the output compressed device header before writing it */
    SWAP_CCKD64_DEVHDR( &ocdevhdr );

    // "Writing cckd64 compressed device header..."
    WRMSG( HHC02954, "I" );

    /* Re-write output file's compressed device header */
    if ((newpos = lseek( ofd, CCKD64_DEVHDR_POS, SEEK_SET )) < 0)
    {
        // "Error in function %s: %s"
        FWRMSG( stderr, HHC02412, "E", "lseek()", strerror( errno ));
        return -1;
    }
    size = (U32) sizeof( ocdevhdr );
    if ((rc = write( ofd, &ocdevhdr, size )) < (int) size)
    {
        // "Error in function %s: %s"
        FWRMSG( stderr, HHC02412, "E", "write()", strerror( errno ));
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
        FWRMSG( stderr, HHC02412, "E", "write()", strerror( errno ));
        return -1;
    }

    /* We're done! Close our input and output files and exit! */
    close( ofd );
    close( ifd );

    // "DASD operation completed"
    WRMSG( HHC02423, "I" );
    return 0;
}

/*-------------------------------------------------------------------*/
/* Process L2 table                                                  */
/*-------------------------------------------------------------------*/
int process_L2_tab( int trkblk, int ifd, CCKD_L2ENT*   iL2,
                                int ofd, CCKD64_L2ENT* oL2 )
{
    int  rc, i, size;                   /* Work variables            */
    S64  newpos;                        /* Work: lseek return code   */

    /* Swap input L2 table if needed before processing */
    SWAP_CCKD_L2TAB( iL2 );

    /* For each L2 table entry... */
    for (i=0; i < 256; i++)
    {
        oL2[i].L2_len  = iL2[i].L2_len;
        oL2[i].L2_size = iL2[i].L2_size;
        oL2[i].L2_pad  = 0;

        if (iL2[i].L2_trkoff == 0)
        {
            oL2[i].L2_trkoff = 0;
            continue;
        }

        if (iL2[i].L2_trkoff == ULONG_MAX)
        {
            oL2[i].L2_trkoff = ULLONG_MAX;
            continue;
        }

        /* Report every track/blkgrp copied if requested... */
        if (verbose && !quiet)
            // "Copying %s %d data..."
            WRMSG( HHC02956, "I", fba ? "blkgrp" : "track", trkblk+i );

        /* Seek to where this track's data resides */
        if ((newpos = lseek( ifd, iL2[i].L2_trkoff, SEEK_SET )) < 0)
        {
            // "Error in function %s: %s"
            FWRMSG( stderr, HHC02412, "E", "lseek()", strerror( errno ));
            return -1;
        }

        /* Read this track's data */
        size = (U32) max( iL2[i].L2_len, iL2[i].L2_size );

        if ((rc = read( ifd, trkbuf, size )) < size)
        {
            // "Error in function %s: %s"
            FWRMSG( stderr, HHC02412, "E", "read()", strerror( errno ));
            return -1;
        }

        /* Retrieve the current output file position */
        if ((oL2[i].L2_trkoff = lseek( ofd, 0, SEEK_CUR )) < 0)
        {
            // "Error in function %s: %s"
            FWRMSG( stderr, HHC02412, "E", "lseek()", strerror( errno ));
            return -1;
        }

        /* Write the track data to the output file */
        if ((rc = write( ofd, trkbuf, size )) < size )
        {
            // "Error in function %s: %s"
            FWRMSG( stderr, HHC02412, "E", "write()", strerror( errno ));
            return -1;
        }
    }

    return 0;
}
