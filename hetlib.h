/* HETLIB.C    (C) Copyright Leland Lucius, 2000-2012                */
/*             Header for the Hercules Emulated Tape library         */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#if !defined( _HETLIB_H_ )
#define _HETLIB_H_

/*
|| ----------------------------------------------------------------------------
||
|| HETLIB.H     (C) Copyright Leland Lucius, 2000-2009
||              Released under terms of the Q Public License.
||
|| Header for the Hercules Emulated Tape library.
||
|| ----------------------------------------------------------------------------
*/

#if !defined( TRUE )
#define TRUE                    (1L)
#endif

#if !defined( FALSE )
#define FALSE                   (0L)
#endif

/*
|| Chunk header for an HET.  Physically compatible with AWSTAPE format.
*/
typedef struct _hethdr
{
    uint8_t        clen[ 2 ];          /* Length of current block           */
    uint8_t        plen[ 2 ];          /* Length of previous block          */
    uint8_t        flags1;             /* Flags byte 1                      */
    uint8_t        flags2;             /* Flags byte 2                      */
} HETHDR;

/*
|| Macros for accessing current and previous sizes - accept ptr to HETHDR
*/
#define HETHDR_CLEN( h ) ( ( (h)->chdr.clen[ 1 ] << 8 ) + (h)->chdr.clen[ 0 ] )
#define HETHDR_PLEN( h ) ( ( (h)->chdr.plen[ 1 ] << 8 ) + (h)->chdr.plen[ 0 ] )

/*
|| Definitions for HETHDR flags byte 1 (compression incompatible with AWSTAPE)
*/
#define HETHDR_FLAGS1_BOR       0x80    /* Start of new record              */
#define HETHDR_FLAGS1_TAPEMARK  0x40    /* Tape mark                        */
#define HETHDR_FLAGS1_EOR       0x20    /* End of record                    */
#define HETHDR_FLAGS1_COMPRESS  0x03    /* Compression method mask          */
#define HETHDR_FLAGS1_BZLIB     0x02    /* BZLIB compression                */
#define HETHDR_FLAGS1_ZLIB      0x01    /* ZLIB compression                 */

/*
|| Definitions for HETHDR flags byte 2 (incompatible with AWSTAPE and HET)
*/
#define HETHDR_FLAGS2_COMPRESS     0x80 /* Compression method mask          */
#define HETHDR_FLAGS2_ZLIB_BUSTECH 0x80 /* Bus-Tech ZLIB compression        */

/*
|| Control block for Hercules Emulated Tape files
*/
typedef struct _hetb
{
    FILE           *fh;                 /* Tape file handle                 */
    int             fd;                 /* Tape file descriptor             */
    uint32_t        chksize;            /* Size of output chunks            */
    uint32_t        ublksize;           /* Current block compressed size    */
    uint32_t        cblksize;           /* Current block uncompressed size  */
    uint32_t        cblk;               /* Current block number             */
    HETHDR          chdr;               /* Current block header             */
    u_int           writeprotect:1;     /* TRUE=write protected             */
    u_int           readlast:1;         /* TRUE=last i/o was read           */
    u_int           truncated:1;        /* TRUE=file truncated              */
    u_int           compress:1;         /* TRUE=compress written data       */
    u_int           decompress:1;       /* TRUE=decompress read data        */
    u_int           method:2;           /* 1=ZLIB, 2=BZLIB compresion       */
    u_int           level:4;            /* 1=<n<=9 compression level        */
    u_int           created:1;          /* TRUE = CREATED                   */
} HETB;

/*
|| Compression methods
*/
#define HETMETH_ZLIB            1       /* ZLIB compression                 */
#define HETMETH_BZLIB           2       /* BZLIB compression                */

/*
|| Limits
*/
#define HETMIN_METHOD           1       /* Minimum compression method       */
#if defined( HET_BZIP2 )
#define HETMAX_METHOD           2       /* Maximum compression method       */
#else
#define HETMAX_METHOD           1       /* Maximum compression method       */
#endif /* defined( HET_BZIP2 ) */
#define HETMIN_LEVEL            1       /* Minimum compression level        */
#define HETMAX_LEVEL            9       /* Maximum compression level        */
#define HETMIN_CHUNKSIZE        4096    /* Minimum chunksize                */
#define HETMAX_CHUNKSIZE        65535   /* Maximum chunksize                */
#define HETMIN_BLOCKSIZE        1       /* Minimum blocksize                */
#define HETMAX_BLOCKSIZE        2097152 /* Maximum blocksize = 2MB          */

/*
|| Default settings
*/
#define HETDFLT_COMPRESS        TRUE    /* Compress written data            */
#define HETDFLT_DECOMPRESS      TRUE    /* Decompress read data             */
#define HETDFLT_METHOD          HETMETH_ZLIB /* Use ZLIB compression        */
#define HETDFLT_LEVEL           4       /* Middle of the road               */
#define HETDFLT_CHKSIZE         HETMAX_CHUNKSIZE /* As big as it gets       */

/*
|| Flags for het_open()
*/
#define HETOPEN_CREATE          0x01    /* Create NL tape, if file missing  */
#define HETOPEN_READONLY        0x02    /* Force the open read-only         */

/*
|| Modes for het_cntl()
*/
#define HETCNTL_GET             0<<8    /* OR with func code to get value   */
#define HETCNTL_SET             1<<8    /* OR with func code to set value   */

/*
|| Function codes for het_cntl()
*/
#define HETCNTL_COMPRESS        1       /* TRUE=write compression on        */
#define HETCNTL_DECOMPRESS      2       /* TRUE=read decomporession on      */
#define HETCNTL_METHOD          3       /* Compression method               */
#define HETCNTL_LEVEL           4       /* Compression level                */
#define HETCNTL_CHUNKSIZE       5       /* Chunk size                       */

/*
|| Error definitions
*/
#define HETE_OK                 0       /* No error                         */
#define HETE_ERROR              -1      /* File error (check errno)         */
#define HETE_TAPEMARK           -2      /* Tapemark read                    */
#define HETE_BOT                -3      /* Beginning of tape                */
#define HETE_EOT                -4      /* End of tape                      */
#define HETE_BADBOR             -5      /* BOR not found                    */
#define HETE_BADEOR             -6      /* EOR not found                    */
#define HETE_BADMARK            -7      /* Unexpected tapemark              */
#define HETE_OVERFLOW           -8      /* Buffer not big enough            */
#define HETE_PREMEOF            -9      /* Premature EOF                    */
#define HETE_DECERR             -10     /* Decompression error              */
#define HETE_UNKMETH            -11     /* Unknown compression method       */
#define HETE_COMPERR            -12     /* Compression error                */
#define HETE_BADLEN             -13     /* Specified length too big         */
#define HETE_PROTECTED          -14     /* Write protected                  */
#define HETE_BADFUNC            -15     /* Bad function code passed         */
#define HETE_BADMETHOD          -16     /* Bad compression method           */
#define HETE_BADLEVEL           -17     /* Bad compression level            */
#define HETE_BADSIZE            -18     /* Bad write chunk size             */
#define HETE_BADDIR             -19     /* Invalid direction specified      */
#define HETE_NOMEM              -20     /* Insufficient memory              */
#define HETE_BADHDR             -21     /* Couldn't read block header       */
#define HETE_BADCOMPRESS        -22     /* Inconsistent compression flags   */
#define HETE_BADBLOCK           -23     // unused
#define HETE_BADLOC             -24     // unused

/*
|| Public functions
*/
HET_DLL_IMPORT int het_open( HETB **hetb, const char *filename, int flags );
HET_DLL_IMPORT int het_close( HETB **hetb );
HET_DLL_IMPORT int het_read_header( HETB *hetb );
HET_DLL_IMPORT int het_read( HETB *hetb, void *sbuf );
HET_DLL_IMPORT int het_write_header( HETB *hetb, int len, int flags1, int flags2 );
HET_DLL_IMPORT int het_write( HETB *hetb, const void *sbuf, int slen );
HET_DLL_IMPORT int het_tapemark( HETB *hetb );
HET_DLL_IMPORT int het_sync( HETB *hetb );
HET_DLL_IMPORT int het_cntl( HETB *hetb, int func, unsigned long val );
HET_DLL_IMPORT int het_locate( HETB *hetb, int block );
HET_DLL_IMPORT int het_bsb( HETB *hetb );
HET_DLL_IMPORT int het_fsb( HETB *hetb );
HET_DLL_IMPORT int het_bsf( HETB *hetb );
HET_DLL_IMPORT int het_fsf( HETB *hetb );
HET_DLL_IMPORT int het_rewind( HETB *hetb );
HET_DLL_IMPORT const char *het_error( int rc );
HET_DLL_IMPORT off_t het_tell ( HETB *hetb );

#endif /* defined( _HETLIB_H_ ) */
