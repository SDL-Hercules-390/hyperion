/* cckdmap.c   (C) Copyright "Fish" (David B. Trout), 2019           */
/*                 Compressed dasd file map                          */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/*   This program is a CCKD64 debugging tool that reads a dasd       */
/*   file and reports the physical sequence of all of its areas,     */
/*   including free space.  The report is in physical file offset    */
/*   sequence.  It allows you to determine not only how many (and    */
/*   where) all free space gaps are, but also where each track's     */
/*   data resides, the size of the track data area, where both the   */
/*   device header and compressed device headers, L1 table and all   */
/*   L2 tables are located.  This allows you to determine whether    */
/*   the preferred L2 table area (which immediately follows the L1   */
/*   table) contains only L2 tables, or whether is also contains     */
/*   track data or free space, and whether or not all of a file's    */
/*   L2 tables are where they should be (within the preferred L2     */
/*   area).  It allows us to determine whether dasdinit created      */
/*   the file correctly or not and whether cckdcomp compressed       */
/*   the file correctly or not, and thus whether the cckd garbage    */
/*   collector's cckd_gc_l2 (Reposition level 2 tables) function     */
/*   will likely be invoked or not (which is known to be buggy).     */
/*                                                                   */
/*       Usage:    cckdmap.exe  [ -t | -v ]   infile                 */
/*                                                                   */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"
#include "hercules.h"
#include "dasdblks.h"
#include "devtype.h"
#include "opcode.h"
#include "ccwarn.h"

#define UTILITY_NAME    "cckdmap"
#define UTILITY_DESC    "Compressed dasd file map"

/*-------------------------------------------------------------------*/
/* Global variables                                                  */
/*-------------------------------------------------------------------*/
char*       pgm              = NULL;    /* less any extension (.ext) */
SPCTAB64*   spacetab         = NULL;    /* Spaces table              */
int         numspace         = 0;       /* Number of spaces in table */
S32         num_L1tab        = 0;       /* Number of L1 tab entries  */
bool        cckd64           = false;   /* true = CCKD64 input       */
bool        swaps_needed     = false;   /* true = endian swap needed */
bool        fba              = false;   /* true = FBA, false = CKD   */
bool        shadow           = false;   /* true = shadow, else base  */
bool        data             = false;   /* Consolidated tracks report*/
bool        verbose          = false;   /* Report detailed track info*/

CCKD_DEVHDR     cdevhdr32    = {0};     /* CCKD   device header      */
CCKD_L1ENT*     L1tab32      = NULL;    /* CCKD   Level 1 table      */
CCKD_L2ENT      L2tab32[256] = {0};     /* CCKD   Level 2 table      */

CCKD64_DEVHDR   cdevhdr      = {0};     /* CCKD64 device header      */
CCKD64_L1ENT*   L1tab        = NULL;    /* CCKD64 Level 1 table      */
CCKD64_L2ENT    L2tab[256]   = {0};     /* CCKD64 Level 2 table      */

/*-------------------------------------------------------------------*/
/* Display command syntax and return -1                              */
/*-------------------------------------------------------------------*/
static int syntax()
{
    // Usage: %s [ -t | -v ] infile
    //   infile   Input file
    // options:
    //   -t       report consolidated track data
    //   -v       verbose (report detailed track data)
    WRMSG( HHC03000, "I", pgm );
    return -1;
}

/*-------------------------------------------------------------------*/
/* Helper macros                                                     */
/*-------------------------------------------------------------------*/
#define SWAP_CCKD64_DEVHDR(p) do if (swaps_needed) cckd64_swapend_chdr( p );          while (0)
#define SWAP_CCKD64_L1TAB(p)  do if (swaps_needed) cckd64_swapend_l1( p, num_L1tab ); while (0)
#define SWAP_CCKD64_L2TAB(p)  do if (swaps_needed) cckd64_swapend_l2( p );            while (0)

/*-------------------------------------------------------------------*/
/*       Convert 32-bit CCKD control block to 64-bit CCKD64          */
/*-------------------------------------------------------------------*/
static void cdevhdr_to_64()
{
    if (!cckd64)
    {
        memcpy( cdevhdr.cdh_vrm,  cdevhdr32.cdh_vrm,  sizeof( cdevhdr.cdh_vrm  ));
        memcpy( cdevhdr.cdh_cyls, cdevhdr32.cdh_cyls, sizeof( cdevhdr.cdh_cyls ));

        cdevhdr.num_L1tab    = cdevhdr32.num_L1tab;
        cdevhdr.num_L2tab    = cdevhdr32.num_L2tab;

        cdevhdr.cdh_opts     = cdevhdr32.cdh_opts;
        cdevhdr.cdh_size     = cdevhdr32.cdh_size;
        cdevhdr.cdh_used     = cdevhdr32.cdh_used;
        cdevhdr.cdh_nullfmt  = cdevhdr32.cdh_nullfmt;

        cdevhdr.free_off     = cdevhdr32.free_off;
        cdevhdr.free_total   = cdevhdr32.free_total;
        cdevhdr.free_largest = cdevhdr32.free_largest;
        cdevhdr.free_num     = cdevhdr32.free_num;
        cdevhdr.free_imbed   = cdevhdr32.free_imbed;

        cdevhdr.cmp_algo     = cdevhdr32.cmp_algo;
        cdevhdr.cmp_parm     = cdevhdr32.cmp_parm;
    }
}
static void L1tab_to_64()
{
    if (!cckd64)
    {
        CCKD_L1ENT*    L32  = L1tab32;      // input 32
        CCKD64_L1ENT*  L64  = L1tab;        // input 64

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
static void L2tab_to_64()
{
    if (!cckd64)
    {
        int  i;
        for (i=0; i < 256; ++i)
        {
            if (L2tab32[i].L2_trkoff == CCKD_MAXSIZE)
                L2tab[i].L2_trkoff = CCKD64_MAXSIZE;
            else
                L2tab[i].L2_trkoff = L2tab32[i].L2_trkoff;

            L2tab[i].L2_len  = L2tab32[i].L2_len;
            L2tab[i].L2_size = L2tab32[i].L2_size;
        }
    }
}

/*-------------------------------------------------------------------*/
/* Determine if running on a big endian system or not                */
/*-------------------------------------------------------------------*/
static bool are_big_endian()
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
static bool are_swaps_needed( const CCKD64_DEVHDR* cdevhdr64 )
{
    bool running_on_big_endian_system = are_big_endian();

    return
    (0
        || (1
            && running_on_big_endian_system
            && !(cdevhdr64->cdh_opts & CCKD_OPT_BIGEND)
           )
        || (1
            && !running_on_big_endian_system
            && (cdevhdr64->cdh_opts & CCKD_OPT_BIGEND)
           )
    );
}

/*-------------------------------------------------------------------*/
/* Append new entry to spaces table (array)                          */
/*-------------------------------------------------------------------*/
#define ADD_SPACE( spc ) \
            add_spc_to_table( &spc )

static void add_spc_to_table( SPCTAB64* spc )
{
    spacetab = realloc( spacetab, (numspace + 1) * sizeof( SPCTAB64 ));
    memcpy( &spacetab[ numspace++ ], spc, sizeof( SPCTAB64 ));
}

/*-------------------------------------------------------------------*/
/* Sort the spaces table                                             */
/*-------------------------------------------------------------------*/
static int sort_spacetab_by_file_offset( const void* a, const void* b )
{
    const SPCTAB64 *s1 = a,
                   *s2 = b;

    if (s1->spc_off < s2->spc_off) return -1;
    if (s1->spc_off > s2->spc_off) return +1;

    if (s1->spc_siz < s2->spc_siz) return -1;
    if (s1->spc_siz > s2->spc_siz) return +1;

    if (s1->spc_typ < s2->spc_typ) return -1;
    if (s1->spc_typ > s2->spc_typ) return +1;

    if (s1->spc_val < s2->spc_val) return -1;
    if (s1->spc_val > s2->spc_val) return +1;

    return 0;
}

/*-------------------------------------------------------------------*/
/*                          MAIN                                     */
/*-------------------------------------------------------------------*/
int main( int argc, char* argv[] )
{
char*           file;                   /* File name                 */
char            pathname[MAX_PATH];     /* File path in host format  */

char            str_size    [64];       /* Work area for fmt_S64     */
char            str_used    [64];       /* Work area for fmt_S64     */
char            str_total   [64];       /* Work area for fmt_S64     */
char            str_largest [64];       /* Work area for fmt_S64     */
char            track_range [32];       /* Work area for fmt_S64     */

off_t           curpos;                 /* Work: saved file position */

CKDDEV*         ckdtab     = NULL;      /* Device table entry        */
CKD_DEVHDR      devhdr     = {0};       /* CKD device header         */
SPCTAB64        spc        = {0};       /* Space table entry         */
char            ser[12+1]  = {0};       /* Serial number             */

int             rc;                     /* Return code               */
int             fd;                     /* File descriptor integer   */
int             i, k;                   /* Loop variables            */
int             tracks;                 /* Total tracks              */
int             L1track;                /* L1 table track number     */
size_t          len_size,  len_used;    /* Return value from fmt_S64 */
size_t          len_total, len_largest; /* Return value from fmt_S64 */

U64             seek_total;             /* Distance from L2 to track */
U32             active_tracks;          /* Total active tracks       */
U32             cyls;                   /* Total cylinders           */
U32             heads;                  /* Heads per cylinder        */
U32             trksize;                /* Track size                */
U32             imgtyp;                 /* Dasd image format         */
U32             size32;                 /* Work: CCKD   I/O size     */
U32             size;                   /* Work: CCKD64 I/O size     */
U16             devtype;                /* Device type (e.g. 0x3390) */

    INITIALIZE_UTILITY( UTILITY_NAME, UTILITY_DESC, &pgm );

    /* Parse options */
    for (argc--, argv++; argc > 0; argc--, argv++)
    {
        /* End of options? */
        if (argv[0][0] != '-')
            break;

        if (0
            || strcasecmp( argv[0], "-t"       ) == 0
            || strcasecmp( argv[0], "--tracks" ) == 0
        )
        {
            data    = true;     /* Consolidated tracks report */
            verbose = false;
        }
        else if (0
            || strcasecmp( argv[0], "-v"        ) == 0
            || strcasecmp( argv[0], "--verbose" ) == 0
        )
        {
            verbose = true;     /* Report detailed track info */
            data    = false;
        }
        else
        {
            // "Unrecognized option: %s"
            FWRMSG( stderr, HHC03001, "E", argv[0] );
            return syntax();
        }
    }

    /* Parse arguments */
    if (argc < 1)
    {
        // "Missing input-file specification"
        FWRMSG( stderr, HHC03002, "E" );
        return syntax();
    }

    if (argc > 1)
    {
        // "Extraneous parameter: %s"
        FWRMSG( stderr, HHC03003, "E", argv[1] );
        return syntax();
    }

    file = argv[0];

    // ""
    // "%s of:     \"%s\""
    // ""
    WRMSG( HHC03020, "I" );
    WRMSG( HHC03021, "I", pgm, file );

    /* Open file and verify correct format */
    hostpath( pathname, file, sizeof( pathname ));
    if ((fd = HOPEN( pathname, O_RDONLY | O_BINARY )) < 0)
    {
        // "%s error: %s"
        FWRMSG( stderr, HHC03006, "E", "open()", strerror( errno ));
        return -1;
    }

    /* Add an EOF record right away */
    if ((curpos = lseek( fd, 0, SEEK_END )) < 0)
    {
        // "%s error: %s"
        FWRMSG( stderr, HHC03006, "E", "lseek()", strerror( errno ));
        return -1;
    }

    spc.spc_typ  = SPCTAB_EOF;
    spc.spc_off  = curpos;
    spc.spc_siz  = curpos;
    spc.spc_len  = curpos;
    spc.spc_val  = -1;
    spc.spc_val2 = -1;
    ADD_SPACE( spc );

    /* Read the device header */
    size = (U32) sizeof( devhdr );
    if ((curpos = lseek( fd, 0, SEEK_SET )) < 0)
    {
        // "%s error: %s"
        FWRMSG( stderr, HHC03006, "E", "lseek()", strerror( errno ));
        return -1;
    }
    if ((rc = read( fd, &devhdr, size )) < (int) size)
    {
        // "%s error: %s"
        FWRMSG( stderr, HHC03006, "E", "read()", strerror( errno ));
        return -1;
    }

    /* Input must be a compressed type */
    if (!((imgtyp = dh_devid_typ( devhdr.dh_devid )) & ANY_CMP_OR_SF_TYP))
    {
        // "Unsupported dasd image file format"
        FWRMSG( stderr, HHC03004, "E" );
        return -1;
    }

    /* Remember whether input is CCKD or CCKD64 */
    if (imgtyp & ANY64_CMP_OR_SF_TYP)
        cckd64 = true;

    /* Remember whether input is FBA or CKD */
    if (imgtyp & FBA_CMP_OR_SF_TYP)
        fba = true;

    /* Remember whether input is shadow or not */
    if (imgtyp & ANY_SF_TYP)
        shadow = true;

    /* Add CKD device header to spaces table */
    spc.spc_typ  = SPCTAB_DEVHDR;
    spc.spc_off  = 0;
    spc.spc_siz  = sizeof( CKD_DEVHDR );
    spc.spc_len  = sizeof( CKD_DEVHDR );
    spc.spc_val  = -1;
    spc.spc_val2 = -1;
    ADD_SPACE( spc );

    /* Read the compressed device header */
    if (cckd64)
    {
        size = (U32) sizeof( cdevhdr );
        if ((curpos = lseek( fd, CCKD64_DEVHDR_POS, SEEK_SET )) < 0)
        {
            // "%s error: %s"
            FWRMSG( stderr, HHC03006, "E", "lseek()", strerror( errno ));
            return -1;
        }
        if ((rc = read( fd, &cdevhdr, size )) < (int) size)
        {
            // "%s error: %s"
            FWRMSG( stderr, HHC03006, "E", "read()", strerror( errno ));
            return -1;
        }
    }
    else // (32-bit CCKD)
    {
        size32 = (U32) sizeof( cdevhdr32 );
        if ((curpos = lseek( fd, CCKD_DEVHDR_POS, SEEK_SET )) < 0)
        {
            // "%s error: %s"
            FWRMSG( stderr, HHC03006, "E", "lseek()", strerror( errno ));
            return -1;
        }
        if ((rc = read( fd, &cdevhdr32, size32 )) < (int) size32)
        {
            // "%s error: %s"
            FWRMSG( stderr, HHC03006, "E", "read()", strerror( errno ));
            return -1;
        }

        /* Convert 32-bit CCKD compressed device header to 64-bit CCKD64 */
        cdevhdr_to_64();
    }

    /* Add compressed device header to spaces table */
    spc.spc_typ  = SPCTAB_CDEVHDR;
    spc.spc_off  = curpos;
    spc.spc_siz  = cckd64 ? size : size32;
    spc.spc_len  = cckd64 ? size : size32;
    spc.spc_val  = -1;
    spc.spc_val2 = -1;
    ADD_SPACE( spc );

    /* Determine whether endian swaps are going to be needed or not */
    swaps_needed = are_swaps_needed( &cdevhdr );

    /* Swap compressed device header before processing */
    SWAP_CCKD64_DEVHDR( &cdevhdr );

    /* Save some values for reporting */
    devtype = 0x3300 + devhdr.dh_devtyp;

    FETCH_LE_FW( heads,   devhdr.dh_heads   );
    FETCH_LE_FW( trksize, devhdr.dh_trksize );
    FETCH_LE_FW( cyls,   cdevhdr.cdh_cyls   );

    tracks = cyls * heads;

    /* Locate the CKD dasd table entry */
    if (!(ckdtab = dasd_lookup( DASD_CKDDEV, NULL, devtype, cyls )))
    {
        // "Device type %4.4X not found in dasd table"
        FWRMSG( stderr, HHC03005, "E", devtype );
        return -1;
    }

    /* Report the dasd device header fields */

    // dh_devid:      %s        (%s-bit C%s%s %s)
    // dh_heads:      %u
    // dh_trksize:    %u
    // dh_devtyp:     0x%02.2X            (%s)
    // dh_fileseq:    0x%02.2X
    // dh_highcyl:    %u
    // dh_serial:     %s

    memcpy( ser, devhdr.dh_serial, sizeof( devhdr.dh_serial ));

    WRMSG( HHC03022, "I", dh_devid_str( imgtyp ),
        cckd64 ? "64"          : "32",
        fba    ? "FBA"         : "CKD",
        cckd64 ? "64"          : "",
        shadow ? "shadow file" : "base image",
        heads, trksize,
        devhdr.dh_devtyp, ckdtab->name, devhdr.dh_fileseq,
        fetch_hw( devhdr.dh_highcyl ),
        ser );

    /* Report the compressed device header fields */

    len_size    = fmt_S64( str_size,    (S64) cdevhdr.cdh_size     );
    len_used    = fmt_S64( str_used,    (S64) cdevhdr.cdh_used     );

    len_total   = fmt_S64( str_total,   (S64) cdevhdr.free_total   );
    len_largest = fmt_S64( str_largest, (S64) cdevhdr.free_largest );

    // cdh_vrm:       %u.%u.%u
    // cdh_opts:      0x%02.2X
    // num_L1tab:     %"PRId32
    // num_L2tab:     %"PRId32
    // cdh_cyls:      %"PRIu32"            (%"PRIu32" tracks)
    // cdh_size:      0x%10.10"PRIX64"    (%*s bytes)
    // cdh_used:      0x%10.10"PRIX64"    (%*s bytes)
    // free_off:      0x%10.10"PRIX64
    // free_total:    0x%10.10"PRIX64"    (%*s bytes)
    // free_largest:  0x%10.10"PRIX64"    (%*s bytes)
    // free_num:      %"PRId64
    // free_imbed:    %"PRIu64
    // cdh_nullfmt:   %u               (%s)
    // cmp_algo:      %u               (%s)
    // cmp_parm:      %"PRId16"              %s(%s)

    WRMSG( HHC03023, "I"
        , (U32) cdevhdr.cdh_vrm[0], (U32) cdevhdr.cdh_vrm[1], (U32) cdevhdr.cdh_vrm[2]
        , cdevhdr.cdh_opts
        , cdevhdr.num_L1tab
        , cdevhdr.num_L2tab
        , cyls, tracks
        , cdevhdr.cdh_size, (int) max( len_size, len_used ), str_size
        , cdevhdr.cdh_used, (int) max( len_used, len_size ), str_used
        , cdevhdr.free_off
        , cdevhdr.free_total,   (int) max( len_total, len_largest ), str_total
        , cdevhdr.free_largest, (int) max( len_total, len_largest ), str_largest
        , cdevhdr.free_num
        , cdevhdr.free_imbed
        , (U32) cdevhdr.cdh_nullfmt, cdevhdr.cdh_nullfmt == CKD_NULLTRK_FMT0 ? "ha r0 EOF" :
                                     cdevhdr.cdh_nullfmt == CKD_NULLTRK_FMT1 ? "ha r0" :
                                     cdevhdr.cdh_nullfmt == CKD_NULLTRK_FMT2 ? "linux" : "???"
        , (U32) cdevhdr.cmp_algo,   !cdevhdr.cmp_algo                        ? "none"  :
                                    (cdevhdr.cmp_algo & CCKD_COMPRESS_ZLIB)  ? "zlib"  :
                                    (cdevhdr.cmp_algo & CCKD_COMPRESS_BZIP2) ? "bzip2" : "INVALID"
        , cdevhdr.cmp_parm
        , cdevhdr.cmp_parm <  0 ? ""        : " "
        , cdevhdr.cmp_parm <  0 ? "default" :
          cdevhdr.cmp_parm == 0 ? "none"    :
          cdevhdr.cmp_parm <= 3 ? "low"     :
          cdevhdr.cmp_parm <= 6 ? "medium"  : "high"
    );

    /* Save the number of L1 table entries */
    num_L1tab = cdevhdr.num_L1tab;

    /* Save the size of the L1 table */
    size   = (U32) (num_L1tab * sizeof( CCKD64_L1ENT ));
    size32 = (U32) (num_L1tab * sizeof( CCKD_L1ENT   ));

    /* Allocate room for the L1 table */
    if (!(L1tab = (CCKD64_L1ENT*) calloc( 1, size )))
    {
        // "%s error: %s"
        FWRMSG( stderr, HHC03006, "E", "calloc()", strerror( errno ));
        return -1;
    }

    /* Read the L1 table */
    if (cckd64)
    {
        /* Read the L1 table */
        if ((curpos = lseek( fd, CCKD64_L1TAB_POS, SEEK_SET )) < 0)
        {
            // "%s error: %s"
            FWRMSG( stderr, HHC03006, "E", "lseek()", strerror( errno ));
            return -1;
        }
        if ((rc = read( fd, L1tab, size )) < (int) size)
        {
            // "%s error: %s"
            FWRMSG( stderr, HHC03006, "E", "read()", strerror( errno ));
            return -1;
        }
    }
    else // (32-bit CCKD)
    {
        /* Allocate room for the L1 table */
        if (!(L1tab32 = (CCKD_L1ENT*) calloc( 1, size32 )))
        {
            // "%s error: %s"
            FWRMSG( stderr, HHC03006, "E", "calloc()", strerror( errno ));
            return -1;
        }

        /* Read the L1 table */
        if ((curpos = lseek( fd, CCKD_L1TAB_POS, SEEK_SET )) < 0)
        {
            // "%s error: %s"
            FWRMSG( stderr, HHC03006, "E", "lseek()", strerror( errno ));
            return -1;
        }
        if ((rc = read( fd, L1tab32, size32 )) < (int) size32)
        {
            // "%s error: %s"
            FWRMSG( stderr, HHC03006, "E", "read()", strerror( errno ));
            return -1;
        }

        /* Convert 32-bit L1 table to 64-bit CCKD64 */
        L1tab_to_64();
        free( L1tab32 );
        L1tab32 = NULL;
    }

    /* Swap the L1 table before processing it */
    SWAP_CCKD64_L1TAB( L1tab );

    /* Add the L1 table to spaces table */
    spc.spc_typ  = SPCTAB_L1;
    spc.spc_off  = curpos;
    spc.spc_siz  = cckd64 ? size : size32;
    spc.spc_len  = cckd64 ? size : size32;
    spc.spc_val  = 0;
    spc.spc_val2 = tracks - 1;
    ADD_SPACE( spc );

    /* Get past L1 table to where L2 tables should start */
    curpos += (cckd64 ? size : size32);

    /* Add the calculated L2 tables LOWER bounds to spaces table */
    spc.spc_typ  = SPCTAB_L2LOWER;
    spc.spc_off  = curpos;
    spc.spc_siz  = 0;
    spc.spc_len  = 0;
    spc.spc_val  = -1;
    spc.spc_val2 = -1;
    ADD_SPACE( spc );

    /* Save the size of each L2 table */
    size   = (U32) sizeof( L2tab   );
    size32 = (U32) sizeof( L2tab32 );

    /* Add the calculated L2 tables UPPER bounds to spaces table */
    spc.spc_typ  = SPCTAB_L2UPPER;
    spc.spc_off  = curpos + (num_L1tab * (cckd64 ? size : size32));
    spc.spc_siz  = 0;
    spc.spc_len  = 0;
    spc.spc_val  = -1;
    spc.spc_val2 = -1;
    ADD_SPACE( spc );

    /* Read in all of the L2 tables */
    for (i=0; i < num_L1tab; i++)
    {
        L1track = (i * 256);

        /* Skip non-existent L2 tables */
        if (0
            || L1tab[i] == 0
            || L1tab[i] == ULLONG_MAX
        )
            continue;  // (skip non-existent L2 table)

        /* Read this L2 table */
        if ((curpos = lseek( fd, L1tab[i], SEEK_SET )) < 0)
        {
            // "%s error: %s"
            FWRMSG( stderr, HHC03006, "E", "lseek()", strerror( errno ));
            return -1;
        }

        if (cckd64)
        {
            if ((rc = read( fd, L2tab, size )) < (int) size)
            {
                // "%s error: %s"
                FWRMSG( stderr, HHC03006, "E", "read()", strerror( errno ));
                return -1;
            }
        }
        else // (32-bit CCKD)
        {
            if ((rc = read( fd, L2tab32, size32 )) < (int) size32)
            {
                // "%s error: %s"
                FWRMSG( stderr, HHC03006, "E", "read()", strerror( errno ));
                return -1;
            }

            /* Convert 32-bit L2 table to 64-bit CCKD64 */
            L2tab_to_64();
        }

        /* Swap the L2 table before processing it */
        SWAP_CCKD64_L2TAB( L2tab );

        /* Add this L2 table to the spaces table */
        spc.spc_typ  = SPCTAB_L2;
        spc.spc_off  = curpos;
        spc.spc_siz  = cckd64 ? size : size32;
        spc.spc_len  = cckd64 ? size : size32;
        spc.spc_val  = L1track;
        spc.spc_val2 = L1track + 256 - 1;
        ADD_SPACE( spc );

        /* Add all track data to the table */
        for (k=0; k < 256 && (L1track + k) < tracks; ++k)
        {
            if (0
                || L2tab[k].L2_trkoff == 0
                || L2tab[k].L2_trkoff == ULLONG_MAX
            )
                continue;  // (skip non-existent tracks)

            /* Add this track's offset to the table */
            spc.spc_typ  = fba ? SPCTAB_BLKGRP : SPCTAB_TRK;
            spc.spc_off  = L2tab[k].L2_trkoff;
            spc.spc_siz  = L2tab[k].L2_size;
            spc.spc_len  = L2tab[k].L2_len;
            spc.spc_val  = L1track + k;
            spc.spc_val2 = L1track + k;
            ADD_SPACE( spc );
        }
    }

    /* Close input file */
    close( fd );

    /* Scan for free space and add to table */
    do
    {
        /* Sort table by file offset */
        qsort( spacetab, numspace, sizeof( SPCTAB64 ), sort_spacetab_by_file_offset );

        /* Find gaps */
        for (i=0; i < numspace && spacetab[i].spc_typ != SPCTAB_EOF; ++i)
        {
            if (0
                || !spacetab[i].spc_siz
                || (spacetab[i].spc_off + spacetab[i].spc_siz) >= spacetab[i+1].spc_off
            )
                continue;

            /* Add free space entry */
            spc.spc_typ  = SPCTAB_FREE;
            spc.spc_off  =                         (spacetab[i].spc_off + spacetab[i].spc_siz);
            spc.spc_siz  = spacetab[i+1].spc_off - (spacetab[i].spc_off + spacetab[i].spc_siz);
            spc.spc_len  = spacetab[i+1].spc_off - (spacetab[i].spc_off + spacetab[i].spc_siz);
            spc.spc_val  = -1;
            spc.spc_val2 = -1;
            ADD_SPACE( spc );
            break;
        }
    }
    while (spacetab[i].spc_typ != SPCTAB_EOF);

    /* Consolidate track data unless detailed track report wanted */
    if (!verbose)
    {
        /* Find contiguous track entries... */
        for (i=0; i < numspace && spacetab[i].spc_typ != SPCTAB_EOF; ++i)
        {
            /* Find next unconsolidated track entry... */
            if (spacetab[i].spc_typ != (fba ? SPCTAB_BLKGRP : SPCTAB_TRK))
                continue;

            /* We found a track entry. Consolidate all
               following track entries into this one. */
            spacetab[i].spc_typ = SPCTAB_DATA;
            for (k = i + 1; k < numspace; ++k)
            {
                /* Is this a track entry immediately following the first? */
                if (spacetab[k].spc_typ != (fba ? SPCTAB_BLKGRP : SPCTAB_TRK))
                    break;

                /* Consolidate this track's data into first */
                spacetab[i].spc_siz += spacetab[k].spc_siz;
                spacetab[i].spc_len += spacetab[k].spc_len;

                /* Mark track as having been consolidated */
                spacetab[k].spc_typ = SPCTAB_NONE;
            }
        }
    }

    /* Print the map... */

    // ""
    // "         File offset    Size (hex)    Size (dec)   track(s)"
    // ""
    WRMSG( HHC03020, "I" );
    WRMSG( HHC03020, "I" );
    WRMSG( HHC03040, "I" );
    WRMSG( HHC03020, "I" );

    for (i=0; i < numspace; ++i)
    {
        if (0
            || (spacetab[i].spc_typ == SPCTAB_NONE)
            || (spacetab[i].spc_typ == SPCTAB_DATA && !data)
            || (spacetab[i].spc_typ == (fba ? SPCTAB_BLKGRP : SPCTAB_TRK) && !verbose)
        )
            continue; // (skip this entry)

        if (0
            || spacetab[i].spc_typ == SPCTAB_L2LOWER
            || spacetab[i].spc_typ == SPCTAB_L2UPPER
        )
            // "***********************************************************"
            WRMSG( HHC03041, "I" );

        /* Track number or track range */
        track_range[0] = 0;
        if (spacetab[i].spc_typ != SPCTAB_DATA)
        {
            if (spacetab[i].spc_val != spacetab[i].spc_val2)
                MSGBUF( track_range, "  %"PRId32" - %"PRId32, spacetab[i].spc_val, spacetab[i].spc_val2 );
            else if (spacetab[i].spc_val >= 0)
                MSGBUF( track_range, "  %"PRId32,             spacetab[i].spc_val );
        }

        // "%-8s 0x%10.10"PRIX64"  0x%10.10"PRIX64" %11"PRIu64"%s"
        WRMSG( HHC03042, "I", spc_typ_to_str( spacetab[i].spc_typ ),
            spacetab[i].spc_off, spacetab[i].spc_siz ,spacetab[i].spc_siz,
            RTRIM( track_range ));
    }

    /* Calculate average distance from each L2 table to its tracks */

    seek_total    = 0;
    active_tracks = 0;

    for (i=0; i < numspace && spacetab[i].spc_typ != SPCTAB_EOF; ++i)
    {
        /* Locate next L2 table entry */
        if (spacetab[i].spc_typ != SPCTAB_L2)
            continue;

        /* Save starting track number for this L2 table */
        L1track = spacetab[i].spc_val;

        /* Find all tracks for this L2 table */
        for (k=0; k < numspace && spacetab[k].spc_typ != SPCTAB_EOF; ++k)
        {
            if ((0
                || spacetab[k].spc_typ == SPCTAB_TRK
                || spacetab[k].spc_typ == SPCTAB_BLKGRP
                || spacetab[k].spc_typ == SPCTAB_DATA
                || spacetab[k].spc_typ == SPCTAB_NONE
            )
            && (1
                && spacetab[k].spc_val >= (L1track +   0)
                && spacetab[k].spc_val <  (L1track + 256)
                && spacetab[k].spc_val <  (tracks)
            ))
            {
                /* Count active track */
                active_tracks++;

                /* Calculate this track's distance from its L2 table */
                if (                spacetab[i].spc_off > spacetab[k].spc_off)
                     seek_total += (spacetab[i].spc_off - spacetab[k].spc_off);
                else seek_total += (spacetab[k].spc_off - spacetab[i].spc_off);
            }
        }
    }

    // ""
    // "Total active tracks      = %"PRIu32" tracks"
    // "Average L2-to-track seek = %.3f MB"
    // ""
    WRMSG( HHC03020, "I" );
    WRMSG( HHC03043, "I", active_tracks );
    WRMSG( HHC03044, "I", (((double) seek_total)/((double) active_tracks)) / (1024.0 * 1024.0) );
    WRMSG( HHC03020, "I" );

    /* DONE! */
    return 0;
}
