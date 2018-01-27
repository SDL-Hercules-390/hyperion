/* FTLIB.H      (C) Copyright TurboHercules, SAS 2010-2011           */
/*             Header files for FAKETAPE.                            */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#if !defined( _FTLIB_H_ )
#define _FTLIB_H_

#include "hercules.h"

/*-------------------------------------------------------------------*/
/* Structure definition for Flex FakeTape block headers              */
/*-------------------------------------------------------------------*/
/*
 * The character length fields in a Flex FakeTape header are in BIG
 * endian ASCII hex. That is to say, when the length field is ASCII
 * "0123" (i.e. 0x30, 0x31, 0x32, 0x33), the length of the block is
 * decimal 291 bytes (0x0123 == 291).
 *
 * The two block length fields are followed by an XOR "check" field
 * calculated as the XOR of the two preceding length fields and is
 * used to verify the integrity of the header.
 *
 * The Flex FakeTape tape format does not support any flag fields
 * in its header and thus does not support any type of compression.
 */
typedef struct _FAKETAPE_BLKHDR
{  /*
    * PROGRAMMING NOTE: note that for Flex FakeTapes, the "previous
    * chunk size" comes FIRST, followed by the "current chunk size"
    * second. This is the complete opposite from the way it is for
    * AWS tape files. Also note that for Flex FakeTape the size fields
    * are in BIG endian ASCII hex-string whereas for AWS tapes
    * they're LITTLE endian binary.
    */
    char  sprvblkl[4];                  /* length of previous block  */
    char  scurblkl[4];                  /* length of this block      */
    char  sxorblkl[4];                  /* XOR both lengths together */
}
FAKETAPE_BLKHDR;

/*
|| Control block for FakeTape Emulated Tape files
*/
typedef struct _fetb
{
    FILE           *fh;                 /* Tape file handle                 */
    int             fd;                 /* Tape file descriptor             */
    char            filename[MAX_PATH]; /* filename                         */

    off_t           nxtblkpos;          /* Offset from start of file
                                           to next block                    */
    off_t           prvblkpos;          /* Offset from start of file
                                           to previous block                */
    off_t           eotmargin;          /* Amount of space left before
                                           reporting EOT (in bytes)         */
    off_t           maxsize;            /* Maximum allowed TAPE file
                                           size                             */
    U16             curfilen;           /* Current file number              */
    U32             blockid;            /* Current device block ID          */
    FAKETAPE_BLKHDR chdr;               /* Current block header             */
    u_int           writeprotect:1;     /* TRUE=write protected             */
    u_int           readlast:1;         /* TRUE=last i/o was read           */
    u_int           truncated:1;        /* TRUE=file truncated              */
    u_int           eotwarning:1;       /* TRUE=EOT warning area reached    */
    u_int           created:1;          /* TRUE = CREATED                   */

} FETB;

/*
|| Flags for fet_open()
*/
#define FETOPEN_CREATE          0x01    /* Create NL tape, if file missing  */
#define FETOPEN_READONLY        0x02    /* Force the open read-only         */

/*
|| Error definitions       NOTE: Keep these in sync with hetlib.h
*/
#define FETE_OK                 0       /* No error                         */
#define FETE_ERROR              -1      /* File error (check errno)         */
#define FETE_TAPEMARK           -2      /* Tapemark read                    */
#define FETE_BOT                -3      /* Beginning of tape                */
#define FETE_EOT                -4      /* End of tape                      */
#define FETE_BADBOR             -5      /* BOR not found                    */
#define FETE_BADEOR             -6      /* EOR not found                    */
#define FETE_BADMARK            -7      /* Unexpected tapemark              */
#define FETE_OVERFLOW           -8      /* Buffer not big enough            */
#define FETE_PREMEOF            -9      /* Premature EOF                    */
#define FETE_DECERR             -10     // unused
#define FETE_UNKMETH            -11     // unused
#define FETE_COMPERR            -12     // unused
#define FETE_BADLEN             -13     /* Specified length to big          */
#define FETE_PROTECTED          -14     /* Write protected                  */
#define FETE_BADFUNC            -15     /* Bad function code passed         */
#define FETE_BADMETHOD          -16     // unused
#define FETE_BADLEVEL           -17     // unused
#define FETE_BADMSIZE           -18     // unused
#define FETE_BADDIR             -19     /* Invalid direction specified      */
#define FETE_NOMEM              -20     /* Insufficient memory              */
#define FETE_BADHDR             -21     /* Couldn't read block header       */
#define FETE_BADCOMPRESS        -22     // unused
#define FETE_BADBLOCK           -23     /* Block is short                   */
#define FETE_BADLOC             -24     /* Location error                   */

// Function Prototypes

FET_DLL_IMPORT int fet_open(            FETB **fetb,    char *filename,     int flags );
FET_DLL_IMPORT int fet_close(           FETB **fetb );
FET_DLL_IMPORT int fet_read_header(     FETB *fetb,     off_t blkpos,       U16* pprvblkl,  U16* pcurblkl );
FET_DLL_IMPORT int fet_read(            FETB *fetb,     void *sbuf );
FET_DLL_IMPORT int fet_write_header(    FETB *fetb,     off_t blkpos,       U16 prvblkl,    U16 curblkl );
FET_DLL_IMPORT int fet_write(           FETB *fetb,     void *sbuf,         U16 slen );
FET_DLL_IMPORT int fet_tapemark(        FETB *fetb );
FET_DLL_IMPORT int fet_sync(            FETB *fetb );
FET_DLL_IMPORT int fet_bsb(             FETB *fetb );
FET_DLL_IMPORT int fet_fsb(             FETB *fetb );
FET_DLL_IMPORT int fet_bsf(             FETB *fetb );
FET_DLL_IMPORT int fet_fsf(             FETB *fetb );
FET_DLL_IMPORT int fet_rewind(          FETB *fetb );
FET_DLL_IMPORT int fet_passedeot(       FETB *fetb );
FET_DLL_IMPORT const char *fet_error(   int rc );
#endif /* defined( _FTLIB_H_ ) */
