/* DASDLOAD2.H  (C) Copyright Roger Bowler, 1999-2012                */
/*              (C) Copyright TurboHercules, SAS 2010-2011           */
/*              Hercules DASD Utilities: DASD image loader           */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* This program creates a virtual DASD volume from a list of         */
/* datasets previously unloaded using the TSO XMIT command.          */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* Additional credits:                                               */
/*      Corrections to CVOL initialization logic by Jay Maynard      */
/*      IEBCOPY native dataset support by Ronen Tzur                 */
/*      Standardized Messages by P. Gorlinsky                        */
/*-------------------------------------------------------------------*/


/*********************************************************************/
/*                                                                   */
/*       THIS SOURCE FILE IS DESIGNED TO BE #INCLUDED                */
/*       FROM DASDLOAD.C and DASDLOAD64.C CONTROLLED BY              */
/*       DASDLOAD64 #DEFINE EITHER 0 or 1.                           */
/*                                                                   */
/*********************************************************************/


/*-------------------------------------------------------------------*/
/* Internal table sizes                                              */
/*-------------------------------------------------------------------*/
#define DEF_MAXDBLK 10000               /* Maximum number of directory
                                           blocks per dataset        */
#define DEF_MAXTTR  50000               /* Maximum number of TTRs
                                           per dataset               */
#define DEF_MAXDSCB 5000                /* Maximum number of DSCBs   */

/*-------------------------------------------------------------------*/
/* Internal macro definitions                                        */
/*-------------------------------------------------------------------*/
#define CASERET(s)      case s: return (#s)
#define XMINF           info_msg
#define XMINFF          info_msg
#define XMERR           printf
#define XMERRF          printf

#define EBCDIC_END      "\xC5\xD5\xC4"
#define EBCDIC_TXT      "\xE3\xE7\xE3"

/*-------------------------------------------------------------------*/
/* Definition of LOGREC header record                                */
/*-------------------------------------------------------------------*/
typedef struct _DIPHDR {
        HWORD   recid;                  /* Record identifier (0xFFFF)*/
        HWORD   bcyl;                   /* Extent begin cylinder     */
        HWORD   btrk;                   /* Extent begin track        */
        HWORD   ecyl;                   /* Extent end cylinder       */
        HWORD   etrk;                   /* Extent end track          */
        BYTE    resv;                   /* Unused                    */
        BYTE    restart[7];             /* Restart area BBCCHHR      */
        HWORD   trkbal;                 /* Bytes remaining on track  */
        HWORD   trklen;                 /* Total bytes on track      */
        BYTE    reused[7];              /* Last reused BBCCHHR       */
        HWORD   lasthead;               /* Last track on cylinder    */
        HWORD   trklen90;               /* 90% of track length       */
        BYTE    devcode;                /* Device type code          */
        BYTE    cchh90[4];              /* 90% full track CCHH       */
        BYTE    switches;               /* Switches                  */
        BYTE    endid;                  /* Check byte (0xFF)         */
    } DIPHDR;

/*-------------------------------------------------------------------*/
/* Definition of internal extent descriptor array entry              */
/*-------------------------------------------------------------------*/
typedef struct _EXTDESC {
        U16     bcyl;                   /* Begin cylinder            */
        U16     btrk;                   /* Begin track               */
        U16     ecyl;                   /* End cylinder              */
        U16     etrk;                   /* End track                 */
        U16     ntrk;                   /* Number of tracks          */
    } EXTDESC;

/*-------------------------------------------------------------------*/
/* Definition of internal TTR conversion table array entry           */
/*-------------------------------------------------------------------*/
typedef struct _TTRCONV {
        BYTE    origttr[3];             /* TTR in original dataset   */
        BYTE    outpttr[3];             /* TTR in output dataset     */
    } TTRCONV;

/*-------------------------------------------------------------------*/
/* Definitions for dataset initialization methods                    */
/*-------------------------------------------------------------------*/
#define METHOD_EMPTY    0
#define METHOD_XMIT     1
#define METHOD_DIP      2
#define METHOD_CVOL     3
#define METHOD_VTOC     4
#define METHOD_VS       5
#define METHOD_SEQ      6

/*-------------------------------------------------------------------*/
/* Static data areas                                                 */
/*-------------------------------------------------------------------*/
BYTE twelvehex00[]  = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
BYTE cvol_low_key[] = {0, 0, 0, 0, 0, 0, 0, 1};

/* Information message level: 0=None, 1=File name, 2=File information,
   3=Member information, 4=Text units, record headers, 5=Dump data */

int  infolvl = 1;

int  nMaxDBLK = DEF_MAXDBLK;
int  nMaxTTR  = DEF_MAXTTR;
int  nMaxDSCB = DEF_MAXDSCB;

/*-------------------------------------------------------------------*/
/* Subroutine to display command syntax and exit                     */
/*-------------------------------------------------------------------*/
static void
argexit ( int code, char *pgm )
{
    // "HHC02496I Usage: ..."

#ifdef CCKD_COMPRESS_ZLIB
    char *bufz =  "  -z     compress using zlib [default]\n";
#else
    char *bufz = "";
#endif

#define MSG_NUM     "HHC02496I "

#ifdef CCKD_COMPRESS_BZIP2
  #ifdef CCKD_COMPRESS_ZLIB
    char *bufbz = MSG_NUM "  -bz2   compress using bzip2\n";
  #else
    char *bufbz =         "  -bz2   compress using bzip2\n";
  #endif
#else
    char *bufbz = "";
#endif

    char*  buflfs = "";

    if (sizeof(off_t) > 4)
#if defined( CCKD_COMPRESS_ZLIB ) || defined( CCKD_COMPRESS_BZIP2 )
        buflfs = MSG_NUM "  -lfs   create single large output file\n";
#else
        buflfs =         "  -lfs   create single large output file\n";
#endif

    FWRMSG( stderr, HHC02496, "I", pgm, bufz, bufbz, buflfs );

    exit(code);
} /* end function argexit */

/*-------------------------------------------------------------------*/
/* Subroutine to display an informational message                    */
/*-------------------------------------------------------------------*/
static void
info_msg (int lvl, char *msg, ...)
{
va_list vl;

    if (infolvl >= lvl)
    {
        va_start(vl, msg);
        vprintf (msg, vl);
    }
} /* end function info_msg */

/*-------------------------------------------------------------------*/
/* Subroutine to load a S/390 integer value from a buffer            */
/*-------------------------------------------------------------------*/
static int
make_int (BYTE *src, int srclen)
{
int             result = 0;             /* Result accumulator        */
int             i;                      /* Array subscript           */

    for (i=0; i < srclen; i++)
    {
        result <<= 8;
        result |= src[i];
    }

    return result;

} /* end function make_int */

/*-------------------------------------------------------------------*/
/* Subroutine to return the name of a dataset organization           */
/*-------------------------------------------------------------------*/
static char *
dsorg_name (BYTE  *dsorg)
{
static char     name[8];                /* Name of dsorg             */

    if (dsorg[0] & DSORG_IS)
        strcpy (name, "IS");
    else if (dsorg[0] & DSORG_PS)
        strcpy (name, "PS");
    else if (dsorg[0] & DSORG_DA)
        strcpy (name, "DA");
    else if (dsorg[0] & DSORG_PO)
        strcpy (name, "PO");

    if (dsorg[0] & DSORG_U) STRLCAT( name, "U" );

    return name;
} /* end function dsorg_name */

/*-------------------------------------------------------------------*/
/* Subroutine to return the name of a record format                  */
/*-------------------------------------------------------------------*/
static char *
recfm_name (BYTE *recfm)
{
static char     name[8];                /* Name of record format     */

    switch (recfm[0] & RECFM_FORMAT) {
    case RECFM_FORMAT_V:
        strcpy (name, "V"); break;
    case RECFM_FORMAT_F:
        strcpy (name, "F"); break;
    case RECFM_FORMAT_U:
        strcpy (name, "U"); break;
    default:
        strcpy (name,"??");
    } /* end switch */

    if (recfm[0] & RECFM_TRKOFLOW) STRLCAT( name, "T" );
    if (recfm[0] & RECFM_BLOCKED)  STRLCAT( name, "B" );
    if (recfm[0] & RECFM_SPANNED)  STRLCAT( name, "S" );

    switch (recfm[0] & RECFM_CTLCHAR) {
    case RECFM_CTLCHAR_A:
        strcpy (name, "A"); break;
    case RECFM_CTLCHAR_M:
        strcpy (name, "M"); break;
    } /* end switch */

    return name;
} /* end function recfm_name */

/*-------------------------------------------------------------------*/
/* Subroutine to return the name of a DASD device from the UCB type  */
/*-------------------------------------------------------------------*/
static char *
dasd_name (FWORD ucbtype)
{
    if (ucbtype[2] != 0x20) return "????";

    switch (ucbtype[3]) {
    case 0x01: return "2311";
    case 0x02: return "2301";
    case 0x03: return "2303";
    case 0x04: if (ucbtype[1] == 0x00) return "2302";
               else return "9345";
    case 0x05: return "2321";
    case 0x06: return "2305-1";
    case 0x07: return "2305-2";
    case 0x08: return "2314";
    case 0x09: return "3330";
    case 0x0A: return "3340";
    case 0x0B: return "3350";
    case 0x0C: return "3375";
    case 0x0D: return "3330-11";
    case 0x0E: return "3380";
    case 0x0F: return "3390";
    } /* end switch(key) */

    return "????";

} /* end function dasd_name */

/*-------------------------------------------------------------------*/
/* Subroutine to return the UCBTYPE of a DASD device                 */
/*-------------------------------------------------------------------*/
static U32
ucbtype_code (U16 devtype)
{
    switch (devtype) {
    case 0x2311: return 0x30002001;
    case 0x2301: return 0x30402002;
    case 0x2303: return 0x30002003;
    case 0x2302: return 0x30002004;
    case 0x2321: return 0x30002005;
    case 0x2305: return 0x30002006;
    case 0x2314: return 0x30C02008;
    case 0x3330: return 0x30502009;
    case 0x3340: return 0x3050200A;
    case 0x3350: return 0x3050200B;
    case 0x3375: return 0x3050200C;
    case 0x3380: return 0x3050200E;
    case 0x3390: return 0x3050200F;
    case 0x9345: return 0x30502004;
    } /* end switch(key) */

    return 0;

} /* end function ucbtype_code */

/*-------------------------------------------------------------------*/
/* Subroutine to calculate relative track address                    */
/* Input:                                                            */
/*      cyl     Cylinder number                                      */
/*      head    Head number                                          */
/*      heads   Number of heads per cylinder                         */
/*      numext  Number of extents                                    */
/*      xarray  Array containing 1-16 extent descriptions            */
/* Output:                                                           */
/*      The return value is the relative track number,               */
/*      or -1 if an error occurred.                                  */
/*-------------------------------------------------------------------*/
static int
calculate_ttr (int cyl, int head, int heads, int numext,
                EXTDESC xarray[])
{
int     i;                              /* Array subscript           */
int     track;                          /* Relative track number     */

    /* Search the extent descriptor array */
    for (i = 0, track = 0; i < numext; track += xarray[i++].ntrk)
    {
        if (cyl < xarray[i].bcyl || cyl > xarray[i].ecyl)
            continue;

        if (cyl == xarray[i].bcyl && head < xarray[i].btrk)
            continue;

        if (cyl == xarray[i].ecyl && head > xarray[i].etrk)
            continue;

        track += (cyl - xarray[i].bcyl) * heads
                - xarray[i].btrk + head;
        break;
    } /* end for(i) */

    /* Error if track was not found in extent table */
    if (i == numext)
    {
        // "CCHH[%04X%04X] not found in extent table"
        XMERRF ( MSG( HHC02505, "E", cyl, head ) );
        return -1;
    }

    /* Return relative track number */
    return track;
} /* end function calculate_ttr */

/*-------------------------------------------------------------------*/
/* Subroutine to read IPL text from an EBCDIC object file            */
/* Input:                                                            */
/*      iplfnm  Name of EBCDIC card image object file                */
/*      iplbuf  Address of buffer in which to build IPL text record  */
/*      buflen  Length of buffer                                     */
/* Output:                                                           */
/*      The return value is the length of the IPL text built         */
/*      in the buffer if successful, or -1 if error                  */
/* Note:                                                             */
/*      Only TXT records are processed; ESD and RLD records are      */
/*      ignored because the IPL text is non-relocatable and is       */
/*      assumed to have zero origin.  An END card must be present.   */
/*-------------------------------------------------------------------*/
static int
read_ipl_text (char *iplfnm, BYTE *iplbuf, int buflen)
{
int             rc;                     /* Return code               */
int             ipllen = 0;             /* Length of IPL text        */
int             txtlen;                 /* Byte count from TXT card  */
int             txtadr;                 /* Address from TXT card     */
int             tfd;                    /* Object file descriptor    */
BYTE            objrec[80];             /* Object card image         */
char            pathname[MAX_PATH];     /* iplfnm in host path format*/

    /* Open the object file */
    hostpath(pathname, iplfnm, sizeof(pathname));
    tfd = HOPEN (pathname, O_RDONLY|O_BINARY);
    if (tfd < 0)
    {
        // "Cannot %s file %s; error %s"
        XMERRF ( MSG( HHC02506, "E", "open", iplfnm, strerror( errno ) ) );
        return -1;
    }

    /* Read the object file */
    while (1)
    {
        /* Read a card image from the object file */
        rc = read (tfd, objrec, 80);
        if (rc < 80)
        {
            // "Cannot %s file %s; error %s"
            XMERRF ( MSG( HHC02506, "E", "read", iplfnm, strerror( errno ) ) );
            close (tfd);
            return -1;
        }

        /* Column 1 of each object card must contain X'02' */
        if (objrec[0] != 0x02)
        {
            // "File %s: invalid object file"
            XMERRF ( MSG( HHC02507, "E", iplfnm ) );
            close (tfd);
            return -1;
        }

        /* Exit if END card has been read */
        if (memcmp(objrec+1, EBCDIC_END, 3) == 0)
            break;

        /* Ignore any cards which are not TXT cards */
        if (memcmp(objrec+1, EBCDIC_TXT, 3) != 0)
            continue;

        /* Load the address from TXT card columns 6-8 */
        txtadr = (objrec[5] << 16) | (objrec[6] << 8) | objrec[7];

        /* Load the byte count from TXT card columns 11-12 */
        txtlen = (objrec[10] << 8) | objrec[11];

        // "IPL text address=%06X length=%04X"
        XMINFF (5, MSG( HHC02522, "I", txtadr, txtlen ) );

        /* Check that the byte count is valid */
        if (txtlen > 56)
        {
            // "File %s: TXT record has invalid count of %d"
            XMERRF ( MSG( HHC02508, "E", iplfnm, txtlen ) );
            close (tfd);
            return -1;
        }

        /* Check that the text falls within the buffer */
        if (txtadr + txtlen > buflen)
        {
            // "File %s: IPL text exceeds %d bytes"
            XMERRF ( MSG( HHC02509, "E", iplfnm, buflen ) );
            close (tfd);
            return -1;
        }

        /* Copy the IPL text to the buffer */
        memcpy (iplbuf + txtadr, objrec+16, txtlen);

        /* Update the total size of the IPL text */
        if (txtadr + txtlen > ipllen)
            ipllen = txtadr + txtlen;

    } /* end while */

    return ipllen;
} /* end function read_ipl_text */

/*-------------------------------------------------------------------*/
/* Subroutine to initialize the output track buffer                  */
/* Input:                                                            */
/*      trklen  Track length of virtual output device                */
/*      trkbuf  Pointer to track buffer                              */
/*      cyl     Cylinder number on output device                     */
/*      head    Head number on output device                         */
/* Output:                                                           */
/*      usedv   Number of bytes written to track of virtual device   */
/*-------------------------------------------------------------------*/
static void
init_track (int trklen, BYTE *trkbuf, int cyl, int head, int *usedv)
{
    CKD_TRKHDR *trkhdr;                 /* -> Track header           */
    CKD_RECHDR *rechdr;                 /* -> Record header          */

    /* Clear the track buffer to zeroes */
    memset (trkbuf, 0, trklen);

    /* Build the home address in the track buffer */
    trkhdr = (CKD_TRKHDR*) trkbuf;
    trkhdr->bin = 0;
    store_hw( trkhdr->cyl,  cyl  );
    store_hw( trkhdr->head, head );

    /* Build a standard record zero in the track buffer */
    rechdr = (CKD_RECHDR*) (trkbuf + CKD_TRKHDR_SIZE);
    store_hw( rechdr->cyl,  cyl  );
    store_hw( rechdr->head, head );
              rechdr->rec  = 0;
              rechdr->klen = 0;
    store_hw( rechdr->dlen, CKD_R0_DLEN );

    /* Set number of bytes used in track buffer */
    *usedv = CKD_TRKHDR_SIZE + CKD_RECHDR_SIZE + CKD_R0_DLEN;

    /* Build end of track marker at end of buffer */
    memcpy( trkbuf + *usedv, &CKD_ENDTRK, CKD_ENDTRK_SIZE );

} /* end function init_track */

/*-------------------------------------------------------------------*/
/* Subroutine to write track buffer to output file                   */
/* Input:                                                            */
/*      cif     -> CKD image file descriptor                         */
/*      ofname  Output file name                                     */
/*      heads   Number of tracks per cylinder on output device       */
/*      trklen  Track length of virtual output device                */
/* Input/output:                                                     */
/*      usedv   Number of bytes written to track of virtual device   */
/*      reltrk  Relative track number on output device               */
/*      cyl     Cylinder number on output device                     */
/*      head    Head number on output device                         */
/* Output:                                                           */
/*      The return value is 0 if successful, -1 if error occurred.   */
/*-------------------------------------------------------------------*/
static int
write_track (CIFBLK *cif, char *ofname, int heads, int trklen,
             int *usedv, int *reltrk, int *cyl, int *head)
{
int             rc;                     /* Return code               */

    UNREFERENCED(ofname);
    UNREFERENCED(trklen);

    /* Don't overwrite HA */
    if (*usedv == 0)
        *usedv = CKD_TRKHDR_SIZE;

    /* Build end of track marker at end of buffer */
    memcpy( cif->trkbuf + *usedv, &CKD_ENDTRK, CKD_ENDTRK_SIZE );
    cif->trkmodif = 1;

    /* Reset values for next track */
    (*reltrk)++;
    (*head)++;
    if (*head >= heads)
    {
        (*cyl)++;
        *head = 0;
    }
    *usedv = 0;

    /* Read the next track */
    if (*cyl < (int)cif->devblk.ckdcyls)
    {
        rc = read_track( cif, *cyl, (U8) *head );
        if (rc < 0) return -1;
    }

    return 0;
} /* end function write_track */

/*-------------------------------------------------------------------*/
/* Subroutine to add a data block to the output track buffer         */
/* Input:                                                            */
/*      cif     -> CKD image file descriptor                         */
/*      ofname  Output file name                                     */
/*      blk     Pointer to data block                                */
/*      keylen  Key length                                           */
/*      datalen Data length                                          */
/*      devtype Output device type                                   */
/*      heads   Number of tracks per cylinder on output device       */
/*      trklen  Track length of virtual output device                */
/*      maxtrk  Maximum number of tracks to be written               */
/* Input/output:                                                     */
/*      usedv   Number of bytes written to track of virtual device   */
/*      usedr   Number of bytes written to track, calculated         */
/*              according to the formula for a real device           */
/*      trkbal  Number of bytes remaining on track, calculated       */
/*              according to the formula for a real device           */
/*      reltrk  Relative track number on output device               */
/*      cyl     Cylinder number on output device                     */
/*      head    Head number on output device                         */
/*      rec     Record number on output device                       */
/* Output:                                                           */
/*      The return value is 0 if successful, -1 if error occurred.   */
/*-------------------------------------------------------------------*/
static int
write_block( CIFBLK *cif, char *ofname, DATABLK *blk, int keylen,
             int datalen, U16 devtype, int heads, int trklen,
             int maxtrk, int *usedv, int *usedr,
             int *trkbal, int *reltrk, int *cyl, int *head, int *rec )
{
int             rc;                     /* Return code               */
int             cc;                     /* Capacity calculation code */
CKD_RECHDR     *rechdr;                 /* -> Record header          */

    UNREFERENCED(devtype);

    /* Determine whether record will fit on current track */
    cc = capacity_calc( cif, *usedr, keylen, datalen,
                        usedr, trkbal, NULL, NULL, NULL, NULL, NULL,
                        NULL, NULL, NULL, NULL, NULL );
    if (cc < 0) return -1;

    /* Move to next track if record will not fit */
    if (cc > 0 && *usedr > 0)
    {
        /* Write current track to output file */
        rc = write_track( cif, ofname, heads, trklen,
                          usedv, reltrk, cyl, head );
        if (rc < 0) return -1;

        /* Clear bytes used and record number for new track */
        *usedr = 0;
        *rec = 0;

        /* Determine whether record will fit on new track */
        cc = capacity_calc( cif, *usedr, keylen, datalen,
                            usedr, trkbal, NULL, NULL, NULL, NULL,
                            NULL, NULL, NULL, NULL, NULL, NULL );
        if (cc < 0) return -1;

    } /* end if */

    /* Error if record will not even fit on an empty track */
    if (cc > 0)
    {
        // "Input record CCHHR[%02X%02X%02X%02X%02X] exceeds output device track size"
        XMERRF ( MSG( HHC02510, "E", blk->cyl[0], blk->cyl[1],
                blk->head[0], blk->head[1], blk->rec ) );
        return -1;
    }

    /* Determine whether end of extent has been reached */
    if (*reltrk >= maxtrk)
    {
        // "Dataset exceeds extent size: reltrk=%d, maxtrk=%d"
        XMERRF ( MSG( HHC02511, "E", *reltrk, maxtrk ) );
        return -1;
    }

    /* Build home address and record 0 if new track */
    if (*usedv == 0)
        init_track( trklen, cif->trkbuf, *cyl, *head, usedv );

    /* Double check that record will not exceed virtual track size */
    if (*usedv + CKD_RECHDR_SIZE + keylen + datalen + 8 > trklen)
    {
        // "Input record CCHHR[%02X%02X%02X%02X%02X] exceeds virtual device track size"
        XMERRF ( MSG( HHC02512, "E", blk->cyl[0], blk->cyl[1],
                blk->head[0], blk->head[1], blk->rec ) );
        return -1;
    }

    /* Add data block to virtual track buffer */
    (*rec)++;
    rechdr = (CKD_RECHDR*)(cif->trkbuf + *usedv);
    store_hw( rechdr->cyl,   (U16)  *cyl  );
    store_hw( rechdr->head,  (U16)  *head );
              rechdr->rec  = (BYTE) *rec;
              rechdr->klen = (BYTE)  keylen;
    store_hw( rechdr->dlen,  (U16)   datalen );
    *usedv += CKD_RECHDR_SIZE;
    memcpy( cif->trkbuf + *usedv, blk->kdarea, keylen + datalen );
    *usedv += keylen + datalen;
    cif->trkmodif = 1;

    return 0;
} /* end function write_block */

/*-------------------------------------------------------------------*/
/* Subroutine to write track zero                                    */
/* Input:                                                            */
/*      cif     -> CKD image file descriptor                         */
/*      ofname  Output file name                                     */
/*      volser  Volume serial number (ASCIIZ)                        */
/*      devtype Output device type                                   */
/*      heads   Number of tracks per cylinder on output device       */
/*      trklen  Track length of virtual output device                */
/*      iplfnm  Name of file containing IPL text object deck         */
/*      flagECmode        1  set EC mode bit in wait PSW             */
/*                        0  don't set EC mode bit in wait PSW       */
/*      flagMachinecheck  1  set machine-check-enabled flag          */
/*                           in wait PSW                             */
/*                        0  don't set machine-check-enabled flag    */
/*                           in wait PSW                             */
/* Output:                                                           */
/*      reltrk  Next relative track number on output device          */
/*      outcyl  Cylinder number of next track on output device       */
/*      outhead Head number of next track on output device           */
/*      The return value is 0 if successful, -1 if error occurred.   */
/*-------------------------------------------------------------------*/
static int
write_track_zero( CIFBLK *cif, char *ofname, char *volser, U16 devtype,
            int heads, int trklen, char *iplfnm,
            int *reltrk, int *outcyl, int *outhead,
            int flagECmode, int flagMachinecheck )
{
int             rc;                     /* Return code               */
int             outusedv = 0;           /* Output bytes used on track
                                           of virtual device         */
int             outusedr = 0;           /* Output bytes used on track
                                           of real device            */
int             outtrkbr = 0;           /* Output bytes remaining on
                                           track of real device      */
int             outtrk = 0;             /* Output relative track     */
int             outrec = 0;             /* Output record number      */
int             keylen;                 /* Key length                */
int             datalen;                /* Data length               */
int             maxtrks = 1;            /* Maximum track count       */
DATABLK        *datablk;                /* -> data block             */
BYTE            buf[sizeof(DATABLK)];   /* Buffer for data block     */

    /* For 2311, the complete IPL text doesn't fit on track 0 record 4
       so the IPL2 text is adjusted further below to load from track 1
       record 1 thus requiring us to use 2 tracks instead of just 1. */
    if (devtype == 0x2311)
        maxtrks = 2;

    /* Read track 0 */
    rc = read_track( cif, 0, 0 );
    if (rc < 0) return -1;

    /* Initialize the track buffer */
    *outcyl = 0; *outhead = 0;
    init_track( trklen, cif->trkbuf, *outcyl, *outhead, &outusedv );
    cif->trkmodif = 1;

    /* Build the IPL1 record */
    memset( buf, 0, sizeof( buf ));
    datablk = (DATABLK*) buf;
    memcpy( datablk->kdarea, IPL1_KEY, IPL1_KEYLEN ); // "IPL1"

    /* Build IPL PSW and CCWs in IPL1 record */
    if (iplfnm != NULL)
    {
        /* Copy model IPL PSW and CCWs for IPLable volume */
        memcpy( datablk->kdarea +  4, iplpsw,  8 );
        memcpy( datablk->kdarea + 12, iplccw1, 8 );
        memcpy( datablk->kdarea + 20, iplccw2, 8 );
    }
    else
    {
        /* Copy model IPL PSW and CCWs for non-IPLable volume */
        memcpy( datablk->kdarea +  4, noiplpsw,  8 );
        memcpy( datablk->kdarea + 12, noiplccw1, 8 );
        memcpy( datablk->kdarea + 20, noiplccw2, 8 );

        /* Set EC mode flag in wait PSW if requested */
        if (flagECmode)
            *(datablk->kdarea+5) = 0x08 | *(datablk->kdarea+5);

        /* Set machine-check-enabled mask in PSW if requested */
        if (flagMachinecheck)
            *(datablk->kdarea+5) = 0x04 | *(datablk->kdarea+5);
    }

    keylen  = IPL1_KEYLEN;
    datalen = IPL1_DATALEN;

    rc = write_block( cif, ofname, datablk, keylen, datalen,
                devtype, heads, trklen, maxtrks,
                &outusedv, &outusedr, &outtrkbr,
                &outtrk, outcyl, outhead, &outrec );
    if (rc < 0) return -1;

    /* Build the IPL2 record */
    memset (buf, 0, sizeof(buf));
    datablk = (DATABLK*)buf;
    memcpy( datablk->kdarea, IPL2_KEY, IPL2_KEYLEN ); // "IPL2"

    if (iplfnm != NULL)
    {
        /* For 2311 the IPL text won't fit on only track 0 record 4,
           so adjust the IPL2 so that it loads from track 1 record 1 */
        if (devtype == 0x2311)
        {
            memcpy( datablk->kdarea + IPL2_KEYLEN, ipl2data, 32 );
            memcpy( datablk->kdarea + IPL2_KEYLEN + 32, "\x00\x00\x00\x00\x00\x01", 6 ); // BBCCHH
            memcpy( datablk->kdarea + IPL2_KEYLEN + 38, "\x00\x00\x00\x01\x01",     5 ); // CCHHR
        }
        else
            memcpy( datablk->kdarea + IPL2_KEYLEN, ipl2data, 43 );
    }

    keylen  = IPL2_KEYLEN;
    datalen = IPL2_DATALEN;

    rc = write_block (cif, ofname, datablk, keylen, datalen,
                devtype, heads, trklen, maxtrks,
                &outusedv, &outusedr, &outtrkbr,
                &outtrk, outcyl, outhead, &outrec);
    if (rc < 0) return -1;

    /* Build the VOL1 record */
    memset( buf, 0x40, sizeof( buf ));
    datablk = (DATABLK*) buf;
    memcpy( datablk->kdarea, VOL1_KEY, VOL1_KEYLEN );
    build_vol1( datablk->kdarea + VOL1_KEYLEN, volser, HERC_OWNERA, true );

    keylen  = VOL1_KEYLEN;
    datalen = VOL1_DATALEN;

    rc = write_block( cif, ofname, datablk, keylen, datalen,
                devtype, heads, trklen, maxtrks,
                &outusedv, &outusedr, &outtrkbr,
                &outtrk, outcyl, outhead, &outrec );
    if (rc < 0) return -1;

    /* Build the IPL text from the object file */
    if (iplfnm != NULL)
    {
        memset( buf, 0, sizeof( buf ));
        datalen = read_ipl_text( iplfnm, buf + 12, sizeof( buf ) - 12 );
        if (datalen < 0) return -1;

        datablk = (DATABLK*) buf;
        keylen = 0;

        rc = write_block( cif, ofname, datablk, keylen, datalen,
                    devtype, heads, trklen, maxtrks,
                    &outusedv, &outusedr, &outtrkbr,
                    &outtrk, outcyl, outhead, &outrec );
        if (rc < 0) return -1;
    }

    /* Write track zero to the output file */
    rc = write_track( cif, ofname, heads, trklen,
                    &outusedv, reltrk, outcyl, outhead );
    if (rc < 0) return -1;

    return 0;
} /* end function write_track_zero */

/*-------------------------------------------------------------------*/
/* Subroutine to update a data block in the output file              */
/* Input:                                                            */
/*      cif     -> CKD image file descriptor                         */
/*      ofname  Output file name                                     */
/*      blk     Pointer to data block structure                      */
/*      cyl     Cylinder number                                      */
/*      head    Head number                                          */
/*      rec     Record number                                        */
/*      keylen  Key length                                           */
/*      datalen Data length                                          */
/*      heads   Number of tracks per cylinder on output device       */
/*      trklen  Track length of virtual output device                */
/* Output:                                                           */
/*      The return value is 0 if successful, -1 if error occurred.   */
/*-------------------------------------------------------------------*/
static int
update_block (CIFBLK *cif, char *ofname, DATABLK *blk, int cyl,
    int head, int rec, int keylen, int datalen, int heads, int trklen)
{
int             rc;                     /* Return code               */
int             curcyl;                 /* Original cylinder         */
int             curhead;                /* Original head             */
int             klen;                   /* Record key length         */
int             dlen;                   /* Record data length        */
int             skiplen;                /* Number of bytes to skip   */
int             offset;                 /* Offset into trkbuf        */
CKD_TRKHDR      trkhdr;                 /* Track header              */
CKD_RECHDR      rechdr;                 /* Record header             */

    UNREFERENCED(heads);
    UNREFERENCED(trklen);

    /* Save the current position in the output file */
    curcyl = cif->curcyl;
    curhead = cif->curhead;

    /* Read the requested track */
    rc = read_track( cif, cyl, (U8) head );
    if (rc < 0)
    {
        // "File %s: read error at cyl[%04X/%d] head[%04X/%d]"
        XMERRF ( MSG( HHC02513, "E", ofname, cyl, cyl, head, head ) );
        return -1;
    }

    /* Copy the track header */
    memcpy (&trkhdr, cif->trkbuf, CKD_TRKHDR_SIZE);
    offset = CKD_TRKHDR_SIZE;

    /* Validate the track header */
    if (0
        ||           trkhdr.bin    != 0
        || fetch_hw( trkhdr.cyl  ) != cyl
        || fetch_hw( trkhdr.head ) != head
    )
    {
        // "File %s: cyl[%04X/%d] head[%04X/%d]; invalid track header %02X%02X%02X%02X%02X"
        XMERRF ( MSG( HHC02514, "E", ofname, cyl, cyl, head, head,
                                     trkhdr.bin, trkhdr.cyl[0], trkhdr.cyl[1],
                                     trkhdr.head[0], trkhdr.head[1] ) );
        return -1;
    }

    /* Search for the record to be updated */
    while (1)
    {
        /* Copy the next record header */
        memcpy (&rechdr, cif->trkbuf + offset, CKD_RECHDR_SIZE);
        offset += CKD_RECHDR_SIZE;

        /* Check for end of track */
        if (memcmp( &rechdr, &CKD_ENDTRK, CKD_ENDTRK_SIZE ) == 0)
        {
            // "File %s: cyl[%04X/%d] head[%04X/%d] rec[%02X/%d] record not found"
            XMERRF ( MSG( HHC02515, "E", ofname, cyl, cyl, head, head, rec, rec ) );
            return -1;
        }

        /* Extract record key length and data length */
        klen = rechdr.klen;
        dlen = (rechdr.dlen[0] << 8) | rechdr.dlen[1];

        /* Exit loop if matching record number */
        if (rechdr.rec == rec)
            break;

        /* Skip the key and data areas */
        skiplen = klen + dlen;
        offset += skiplen;
    } /* end while */

    /* Check for attempt to change key length or data length */
    if (keylen != klen || datalen != dlen)
    {
        // "File %s: cyl[%04X/%d] head[%04X/%d] rec[%02X/%d] unmatched KL/DL"
        XMERRF ( MSG( HHC02516, "E", ofname, cyl, cyl, head, head, rec, rec ) );
        return -1;
    }

    /* Copy the updated block to the trkbuf */
    memcpy (cif->trkbuf + offset, blk->kdarea, keylen + datalen);
    cif->trkmodif = 1;

    /* Restore original track */
    rc = read_track( cif, curcyl, (U8) curhead );
    if (rc < 0)
    {
        // "File %s: read error at cyl[%04X/%d] head[%04X/%d]"
        XMERRF ( MSG( HHC02513, "E", ofname, curcyl, curcyl, curhead, curhead ) );
        return -1;
    }

    // "Updating cyl[%04X/%d] head[%04X/%d] rec[%02X/%d]; kl[%d] dl[%d]"
    XMINFF (4, MSG( HHC02523, "I", cyl, cyl, head, head, rec, rec, keylen, datalen ) );

    return 0;
} /* end function update_block */

/*-------------------------------------------------------------------*/
/* Subroutine to build a format 1 DSCB                               */
/* Input:                                                            */
/*      dscbtab Array of pointers to DSCB data blocks                */
/*      dscbnum Number of entries in dscbtab array                   */
/*      dsname  Dataset name (ASCIIZ)                                */
/*      volser  Volume serial number (ASCIIZ)                        */
/*      dsorg   1st byte of dataset organization bits                */
/*      recfm   1st byte of record format bits                       */
/*      lrecl   Logical record length                                */
/*      blksz   Block size                                           */
/*      keyln   Key length                                           */
/*      dirblu  Bytes used in last directory block                   */
/*      lasttrk Relative track number of last-used track of dataset  */
/*      lastrec Record number of last-used block of dataset          */
/*      trkbal  Bytes remaining on last-used track                   */
/*      units   Allocation units (C=CYL, T=TRK)                      */
/*      spsec   Secondary allocation quantity                        */
/*      bcyl    Extent begin cylinder number                         */
/*      bhead   Extent begin head number                             */
/*      ecyl    Extent end cylinder number                           */
/*      ehead   Extent end head number                               */
/* Output:                                                           */
/*      The return value is 0 if successful, or -1 if error          */
/*                                                                   */
/* This subroutine allocates a DATABLK structure, builds a DSCB      */
/* within the structure, and adds the structure to the DSCB array.   */
/*-------------------------------------------------------------------*/
static int
build_format1_dscb (DATABLK *dscbtab[], int dscbnum,
                char *dsname, char *volser,
                BYTE dsorg, BYTE recfm, int lrecl, int blksz,
                int keyln, int dirblu, int lasttrk, int lastrec,
                int trkbal, BYTE units, int spsec,
                int bcyl, int bhead, int ecyl, int ehead)
{
DATABLK        *datablk;                /* -> Data block structure   */
FORMAT1_DSCB   *f1dscb;                 /* -> DSCB within data block */
int             blklen;                 /* Size of data block        */
struct tm      *tmptr;                  /* -> Date and time structure*/
time_t          timeval;                /* Current time value        */

    /* Obtain the current time */
    time(&timeval);
    tmptr = localtime(&timeval);

    /* Allocate storage for a DATABLK structure */
    blklen = 12 + sizeof(FORMAT1_DSCB);
    datablk = (DATABLK*)malloc(blklen);
    if (datablk == NULL)
    {
        // "Error obtaining storage for %sDSCB: %s"
        XMERRF ( MSG( HHC02517, "E", "Format 1 ", strerror(errno) ) );
        return -1;
    }

    /* Check that there is room in the DSCB pointer array */
    if (dscbnum >= nMaxDSCB)
    {
        // "Internal error: DSCB count exceeds MAXDSCB of %d"
        XMERRF ( MSG( HHC02518, "E", nMaxDSCB ) );
        return -1;
    }

    /* Clear the data block and save its address in the DSCB array */
    memset (datablk, 0, blklen);
    dscbtab[dscbnum] = datablk;

    /* Point to the DSCB within the data block */
    f1dscb = (FORMAT1_DSCB*)(datablk->kdarea);

    /* Build the format 1 DSCB */
    convert_to_ebcdic (f1dscb->ds1dsnam, 44, dsname);
    f1dscb->ds1fmtid = 0xF1;
    convert_to_ebcdic (f1dscb->ds1dssn, 6, volser);
    store_hw( f1dscb->ds1volsq, 1 );
    f1dscb->ds1credt[0] = (BYTE) tmptr->tm_year;
    f1dscb->ds1credt[1] = (tmptr->tm_yday >> 8) & 0xFF;
    f1dscb->ds1credt[2] = tmptr->tm_yday & 0xFF;
    f1dscb->ds1expdt[0] = 0;
    f1dscb->ds1expdt[1] = 0;
    f1dscb->ds1expdt[2] = 0;
    f1dscb->ds1noepv = 1;
    f1dscb->ds1bodbd = (BYTE) dirblu;
    convert_to_ebcdic (f1dscb->ds1syscd, 13, "HERCULES");
    f1dscb->ds1dsorg[0] = dsorg;
    f1dscb->ds1dsorg[1] = 0;
    f1dscb->ds1recfm = recfm;
    f1dscb->ds1optcd = 0;
    store_hw( f1dscb->ds1blkl,  blksz );
    store_hw( f1dscb->ds1lrecl, lrecl );
    f1dscb->ds1keyl = (BYTE) keyln;
    store_hw( f1dscb->ds1rkp, 0 );
    f1dscb->ds1dsind = DS1DSIND_LASTVOL;
    if ((blksz & 0x07) == 0)
        f1dscb->ds1dsind |= DS1DSIND_BLKSIZ8;
    store_fw( f1dscb->ds1scalo, spsec );
    f1dscb->ds1scalo[0] =
        (units == 'C' ? DS1SCALO_UNITS_CYL : DS1SCALO_UNITS_TRK);
    f1dscb->ds1lstar[0] = (lasttrk >> 8) & 0xFF;
    f1dscb->ds1lstar[1] = lasttrk & 0xFF;
    f1dscb->ds1lstar[2] = (BYTE) lastrec;
    store_hw( f1dscb->ds1trbal, trkbal );
    f1dscb->ds1ext1.xttype =
        (units == 'C' ? XTTYPE_CYLBOUND : XTTYPE_DATA);
    f1dscb->ds1ext1.xtseqn = 0;
    store_hw( f1dscb->ds1ext1.xtbcyl, bcyl );
    store_hw( f1dscb->ds1ext1.xtbtrk, bhead );
    store_hw( f1dscb->ds1ext1.xtecyl, ecyl );
    store_hw( f1dscb->ds1ext1.xtetrk, ehead );
    return 0;
} /* end function build_format1_dscb */

/*-------------------------------------------------------------------*/
/* Subroutine to build a format 4 DSCB                               */
/* Input:                                                            */
/*      dscbtab Array of pointers to DSCB data blocks                */
/*      dscbnum Number of entries in dscbtab array                   */
/*      devtype Output device type                                   */
/* Output:                                                           */
/*      The return value is 0 if successful, or -1 if error          */
/*                                                                   */
/* This subroutine allocates a DATABLK structure, builds a DSCB      */
/* within the structure, and adds the structure to the DSCB array.   */
/*                                                                   */
/* Note: The VTOC extent descriptor, the highest F1 DSCB address,    */
/* and the number of unused DSCBs, are set to zeroes here and must   */
/* be updated later when the VTOC size and location are known.       */
/* The device size in cylinders is set to the normal size for the    */
/* device type and must be updated when the actual total number of   */
/* cylinders written to the volume is known.                         */
/*-------------------------------------------------------------------*/
static int
build_format4_dscb (DATABLK *dscbtab[], int dscbnum, CIFBLK *cif)
{
DATABLK        *datablk;                /* -> Data block structure   */
FORMAT4_DSCB   *f4dscb;                 /* -> DSCB within data block */
int             blklen;                 /* Size of data block        */
int             numdscb;                /* Number of DSCBs per track */
int             numdblk;                /* Number of dir blks/track  */
int             physlen;                /* Physical track length     */
int             numcyls;                /* Device size in cylinders  */
int             numheads;               /* Number of heads/cylinder  */
int             kbconst;                /* Keyed block constant      */
int             lbconst;                /* Last keyed block constant */
int             nkconst;                /* Non-keyed block constant  */
BYTE            devflag;                /* Device flags for VTOC     */
int             tolfact;                /* Device tolerance          */

    /* Calculate the physical track length, block overheads, device
       size, and the number of DSCBs and directory blocks per track */
    capacity_calc (cif, 0, 44, 96, NULL, NULL, &physlen, &kbconst,
                    &lbconst, &nkconst, &devflag, &tolfact, NULL,
                    &numdscb, &numheads, &numcyls);
    capacity_calc (cif, 0, 8, 256, NULL, NULL, NULL,
                    NULL, NULL, NULL, NULL, NULL, NULL,
                    &numdblk, NULL, NULL);

    /* Allocate storage for a DATABLK structure */
    blklen = 12 + sizeof(FORMAT4_DSCB);
    datablk = (DATABLK*)malloc(blklen);
    if (datablk == NULL)
    {
        // "Error obtaining storage for %sDSCB: %s"
        XMERRF ( MSG( HHC02517, "E", "Format 4 ", strerror(errno) ) );
        return -1;
    }

    /* Check that there is room in the DSCB pointer array */
    if (dscbnum >= nMaxDSCB)
    {
        // "Internal error: DSCB count exceeds MAXDSCB of %d"
        XMERRF ( MSG( HHC02518, "E", nMaxDSCB ) );
        return -1;
    }

    /* Clear the data block and save its address in the DSCB array */
    memset (datablk, 0, blklen);
    dscbtab[dscbnum] = datablk;

    /* Point to the DSCB within the data block */
    f4dscb = (FORMAT4_DSCB*)(datablk->kdarea);

    /* Build the format 4 DSCB */
    memset (f4dscb->ds4keyid, 0x04, 44);
    f4dscb->ds4fmtid = 0xF4;
    f4dscb->ds4hcchh[0] = (numcyls >> 8) & 0xFF;
    f4dscb->ds4hcchh[1] = numcyls & 0xFF;
    f4dscb->ds4hcchh[2] = 0;
    f4dscb->ds4hcchh[3] = 0;
    store_hw( f4dscb->ds4noatk, 0 );
    f4dscb->ds4vtoci = DS4VTOCI_DOS;
    f4dscb->ds4noext = 1;
    f4dscb->ds4devsz[0] = (numcyls >> 8) & 0xFF;
    f4dscb->ds4devsz[1] = numcyls & 0xFF;
    f4dscb->ds4devsz[2] = (numheads >> 8) & 0xFF;
    f4dscb->ds4devsz[3] = numheads & 0xFF;
    store_hw( f4dscb->ds4devtk, physlen );
    f4dscb->ds4devi = kbconst;
    f4dscb->ds4devl = lbconst;
    f4dscb->ds4devk = nkconst;
    f4dscb->ds4devfg = devflag;
    store_hw( f4dscb->ds4devtl, tolfact );
    f4dscb->ds4devdt = numdscb;
    f4dscb->ds4devdb = numdblk;

    return 0;
} /* end function build_format4_dscb */

/*-------------------------------------------------------------------*/
/* Subroutine to build a format 5 DSCB                               */
/* Input:                                                            */
/*      dscbtab Array of pointers to DSCB data blocks                */
/*      dscbnum Number of entries in dscbtab array                   */
/* Output:                                                           */
/*      The return value is 0 if successful, or -1 if error          */
/*                                                                   */
/* This subroutine allocates a DATABLK structure, builds a DSCB      */
/* within the structure, and adds the structure to the DSCB array.   */
/*                                                                   */
/* Note: The format 5 DSCB is built with no free space extents.      */
/* The DOS bit which is set in ds4vtoci forces the operating system  */
/* VTOC conversion routine to calculate the free space and update    */
/* the format 5 DSCB the first time the volume is accessed.          */
/*-------------------------------------------------------------------*/
static int
build_format5_dscb (DATABLK *dscbtab[], int dscbnum)
{
DATABLK        *datablk;                /* -> Data block structure   */
FORMAT5_DSCB   *f5dscb;                 /* -> DSCB within data block */
int             blklen;                 /* Size of data block        */

    /* Allocate storage for a DATABLK structure */
    blklen = 12 + sizeof(FORMAT5_DSCB);
    datablk = (DATABLK*)malloc(blklen);
    if (datablk == NULL)
    {
        // "Error obtaining storage for %sDSCB: %s"
        XMERRF ( MSG( HHC02517, "E", "Format 5 ", strerror(errno) ) );
        return -1;
    }

    /* Check that there is room in the DSCB pointer array */
    if (dscbnum >= nMaxDSCB)
    {
        // "Internal error: DSCB count exceeds MAXDSCB of %d"
        XMERRF ( MSG( HHC02518, "E", nMaxDSCB ) );
        return -1;
    }

    /* Clear the data block and save its address in the DSCB array */
    memset (datablk, 0, blklen);
    dscbtab[dscbnum] = datablk;

    /* Point to the DSCB within the data block */
    f5dscb = (FORMAT5_DSCB*)(datablk->kdarea);

    /* Build the format 5 DSCB */
    memset (f5dscb->ds5keyid, 0x05, 4);
    f5dscb->ds5fmtid = 0xF5;

    return 0;
} /* end function build_format5_dscb */

/*-------------------------------------------------------------------*/
/* Subroutine to write the VTOC                                      */
/* Input:                                                            */
/*      dscbtab Array of pointers to DSCB data blocks                */
/*      numdscb Number of DSCBs including format 4 and format 5      */
/*      cif     -> CKD image file descriptor                         */
/*      ofname  Output file name                                     */
/*      devtype Output device type                                   */
/*      reqcyls Requested device size in cylinders, or zero          */
/*      heads   Number of tracks per cylinder on output device       */
/*      trklen  Track length of virtual output device                */
/*      vtoctrk Starting relative track number for VTOC, or zero     */
/*      vtocext Number of tracks in VTOC, or zero                    */
/* Input/output:                                                     */
/*      nxtcyl  Starting cylinder number for next dataset            */
/*      nxthead Starting head number for next dataset                */
/* Output:                                                           */
/*      volvtoc VTOC starting CCHHR (5 bytes)                        */
/*      The return value is 0 if successful, or -1 if error          */
/*                                                                   */
/* If vtoctrk and vtocext are non-zero, then the VTOC is written     */
/* into the space previously reserved at the indicated location.     */
/* Otherwise, the VTOC is written at the next available dataset      */
/* location, using as many tracks as are necessary, and nextcyl      */
/* and nexthead are updated to point past the end of the VTOC.       */
/*-------------------------------------------------------------------*/
static int
write_vtoc (DATABLK *dscbtab[], int numdscb, CIFBLK *cif, char *ofname,
            U16 devtype, int reqcyls, int heads, int trklen,
            int vtoctrk, int vtocext,
            int *nxtcyl, int *nxthead, BYTE volvtoc[])
{
int             rc;                     /* Return code               */
int             i;                      /* Array subscript           */
DATABLK        *datablk;                /* -> Data block structure   */
FORMAT1_DSCB   *f1dscb;                 /* -> Format 1 DSCB          */
FORMAT4_DSCB   *f4dscb;                 /* -> Format 4 DSCB          */
int             dscbpertrk;             /* Number of DSCBs per track */
int             mintrks;                /* Minimum VTOC size (tracks)*/
int             numtrks;                /* Actual VTOC size (tracks) */
int             highcyl;                /* Last used cylinder number */
int             highhead;               /* Last used head number     */
int             highrec;                /* Last used record number   */
int             numf0dscb;              /* Number of unused DSCBs    */
int             abstrk;                 /* Absolute track number     */
int             endcyl;                 /* VTOC end cylinder number  */
int             endhead;                /* VTOC end head number      */
int             numcyls;                /* Volume size in cylinders  */
int             outusedv = 0;           /* Bytes used in track buffer*/
int             outusedr = 0;           /* Bytes used on real track  */
int             outtrkbr;               /* Bytes left on real track  */
int             outcyl;                 /* Output cylinder number    */
int             outhead;                /* Output head number        */
int             outtrk = 0;             /* Relative track number     */
int             outrec = 0;             /* Output record number      */
int             prealloc = 0;           /* 1=VTOC is preallocated    */
BYTE            blankblk[152];          /* Data block for blank DSCB */
int             curcyl;                 /* Current cylinder in file  */
int             curhead;                /* Current head in file      */
char            dsnama[45];             /* Dataset name (ASCIIZ)     */

    /* Determine if the VTOC is preallocated */
    prealloc = (vtoctrk != 0 && vtocext != 0);

    /* Point to the format 4 DSCB within the first data block */
    f4dscb = (FORMAT4_DSCB*)(dscbtab[0]->kdarea);

    /* Calculate the minimum number of tracks required for the VTOC */
    dscbpertrk = f4dscb->ds4devdt;
    mintrks = (numdscb + dscbpertrk - 1) / dscbpertrk;

    /* Save the current position in the output file */
    curcyl = cif->curcyl;
    curhead = cif->curhead;

    /* Obtain the VTOC starting location and size */
    if (prealloc)
    {
        /* Use preallocated VTOC location */
        outcyl = vtoctrk / heads;
        outhead = vtoctrk % heads;
        numtrks = vtocext;
    }
    else
    {
        /* Use next available dataset location for VTOC */
        outcyl = *nxtcyl;
        outhead = *nxthead;
        numtrks = mintrks;
    }

    /* Check that VTOC extent size is sufficient */
    if (numtrks < mintrks)
    {
        // "VTOC requires %d track%s; current allocation is too small"
        XMERRF ( MSG( HHC02519, "E", mintrks, mintrks == 1 ? "" : "s" ) );
        return -1;
    }

    /* Read the first track of the VTOC */
    rc = read_track( cif, outcyl, (U8) outhead );
    if (rc < 0)
    {
        // "Error reading %s at cyl[%04X/%d] head[%04X/%d]"
        XMERRF ( MSG( HHC02530, "E", "VTOC", outcyl, outcyl, outhead, outhead ) );
        return -1;
    }

    /* Calculate the CCHHR of the last format 1 DSCB */
    abstrk = (outcyl * heads) + outhead;
    abstrk += mintrks - 1;
    highcyl = abstrk / heads;
    highhead = abstrk % heads;
    highrec = ((numdscb - 1) % dscbpertrk) + 1;

    /* Update the last format 1 CCHHR in the format 4 DSCB */
    f4dscb->ds4hpchr[0] = (highcyl >> 8) & 0xFF;
    f4dscb->ds4hpchr[1] = highcyl & 0xFF;
    f4dscb->ds4hpchr[2] = (highhead >> 8) & 0xFF;
    f4dscb->ds4hpchr[3] = highhead & 0xFF;
    f4dscb->ds4hpchr[4] = (BYTE) highrec;

    /* Build the VTOC start CCHHR */
    volvtoc[0] = (outcyl >> 8) & 0xFF;
    volvtoc[1] = outcyl & 0xFF;
    volvtoc[2] = (outhead >> 8) & 0xFF;
    volvtoc[3] = outhead & 0xFF;
    volvtoc[4] = 1;

    // "VTOC starts at cyl[%04X/%d] head[%04X/%d] and is %d track%s"
    XMINFF (1, MSG( HHC02524, "I", outcyl, outcyl, outhead, outhead, numtrks, numtrks == 1 ? "" : "s" ) );

    /* Calculate the number of format 0 DSCBs required to
       fill out the unused space at the end of the VTOC */
    numf0dscb = (numtrks * dscbpertrk) - numdscb;

    /* Update the format 0 DSCB count in the format 4 DSCB */
    store_hw( f4dscb->ds4dsrec, numf0dscb );

    /* Calculate the CCHH of the last track of the VTOC */
    abstrk = (outcyl * heads) + outhead;
    abstrk += numtrks - 1;
    endcyl = abstrk / heads;
    endhead = abstrk % heads;

    /* Update the VTOC extent descriptor in the format 4 DSCB */
    f4dscb->ds4vtoce.xttype =
        (endhead == heads - 1 ? XTTYPE_CYLBOUND : XTTYPE_DATA);
    f4dscb->ds4vtoce.xtseqn = 0;
    store_hw( f4dscb->ds4vtoce.xtbcyl, outcyl );
    store_hw( f4dscb->ds4vtoce.xtbtrk, outhead );
    store_hw( f4dscb->ds4vtoce.xtecyl, endcyl );
    store_hw( f4dscb->ds4vtoce.xtetrk, endhead );

    /* Calculate the mimimum volume size */
    if (prealloc)
    {
        /* The VTOC was preallocated, so the minimum volume
           size equals the next available cylinder number */
        numcyls = *nxtcyl;
        if (*nxthead != 0) numcyls++;
    }
    else
    {
        /* The VTOC will be written into the available space,
           so the minimum volume size is one more than the
           ending cylinder number of the VTOC */
        numcyls = endcyl + 1;
    }

    /* If the minimum volume size is less than the requested
       size then use the requested size as the actual size */
    if (numcyls < reqcyls) numcyls = reqcyls;

    /* Update the volume size in cylinders in the format 4 DSCB */
    f4dscb->ds4devsz[0] = (numcyls >> 8) & 0xFF;
    f4dscb->ds4devsz[1] = numcyls & 0xFF;

    /* Format the track buffer */
    init_track (trklen, cif->trkbuf, outcyl, outhead, &outusedv);
    cif->trkmodif = 1;

    /* Write the format 4, format 5, and format 1 DSCBs to the VTOC */
    for (i = 0; i < numdscb; i++)
    {
        /* Load the data block pointer from the DSCB table */
        datablk = dscbtab[i];

        /* Extract the dataset name from the format 1 DSCB */
        memset (dsnama, 0, sizeof(dsnama));
        f1dscb = (FORMAT1_DSCB*)(datablk->kdarea);
        if (f1dscb->ds1fmtid == 0xF1)
        {
            make_asciiz (dsnama, sizeof(dsnama), f1dscb->ds1dsnam,
                        sizeof(f1dscb->ds1dsnam));
        }

        /* Add next DSCB to the track buffer */
        rc = write_block (cif, ofname, datablk, 44, 96,
                    devtype, heads, trklen, numtrks,
                    &outusedv, &outusedr, &outtrkbr,
                    &outtrk, &outcyl, &outhead, &outrec);
        if (rc < 0) return -1;

        // "Format %d DSCB CCHHR[%04X%04X%02X] (TTR[%04X%02X]) %s"
        XMINFF (4, MSG( HHC02525, "I", datablk->kdarea[0] == 0x04 ? 4 :
                                       datablk->kdarea[0] == 0x05 ? 5 : 1,
                                       outcyl, outhead, outrec, outtrk, outrec, dsnama ) );
        if (infolvl >= 5) data_dump (datablk, 152);

    } /* end for(i) */

    /* Fill the remainder of the VTOC with format 0 DSCBs */
    for (i = 0; i < numf0dscb; i++)
    {
        /* Add a format 0 DSCB to the track buffer */
        memset (blankblk, 0, sizeof(blankblk));
        datablk = (DATABLK*)blankblk;
        rc = write_block (cif, ofname, datablk, 44, 96,
                    devtype, heads, trklen, numtrks,
                    &outusedv, &outusedr, &outtrkbr,
                    &outtrk, &outcyl, &outhead, &outrec);
        if (rc < 0) return -1;

        // "Format %d DSCB CCHHR[%04X%04X%02X] (TTR[%04X%02X]) %s"
        XMINFF (4,  MSG( HHC02525, "I", 0, outcyl, outhead, outrec, outtrk, outrec, "" ) );

    } /* end for(i) */

    /* Write data remaining in last track buffer */
    rc = write_track (cif, ofname, heads, trklen,
                    &outusedv, &outtrk, &outcyl, &outhead);
    if (rc < 0) return -1;

    /* Restore original file position if VTOC was preallocated */
    if (prealloc)
    {
        /* Read the original track again */
        rc = read_track( cif, curcyl, (U8) curhead );
        if (rc < 0)
        {
            // "Error reading %s at cyl[%04X/%d] head[%04X/%d]"
            XMERRF ( MSG( HHC02530, "E", "track", curcyl, curcyl, curhead, curhead ) );
            return -1;
        }
    }
    else
    {
        /* Update next cyl and head if VTOC not preallocated */
        *nxtcyl = outcyl;
        *nxthead = outhead;
    }

    return 0;
} /* end function write_vtoc */

/*-------------------------------------------------------------------*/
/* Subroutine to return the name of a text unit                      */
/*-------------------------------------------------------------------*/
static char *
tu_name (U16 key)
{
    switch (key) {
    CASERET(INMDDNAM);
    CASERET(INMDSNAM);
    CASERET(INMMEMBR);
    CASERET(INMDIR  );
    CASERET(INMEXPDT);
    CASERET(INMTERM );
    CASERET(INMBLKSZ);
    CASERET(INMDSORG);
    CASERET(INMLRECL);
    CASERET(INMRECFM);
    CASERET(INMTNODE);
    CASERET(INMTUID );
    CASERET(INMFNODE);
    CASERET(INMFUID );
    CASERET(INMLREF );
    CASERET(INMLCHG );
    CASERET(INMCREAT);
    CASERET(INMFVERS);
    CASERET(INMFTIME);
    CASERET(INMTTIME);
    CASERET(INMFACK );
    CASERET(INMERRCD);
    CASERET(INMUTILN);
    CASERET(INMUSERP);
    CASERET(INMRECCT);
    CASERET(INMSIZE );
    CASERET(INMFFM  );
    CASERET(INMNUMF );
    CASERET(INMTYPE );
    } /* end switch(key) */

    return "????????";

} /* end function tu_name */

/*-------------------------------------------------------------------*/
/* Subroutine to extract next text unit from buffer                  */
/* Input:                                                            */
/*      xbuf    Pointer to start of buffer                           */
/*      bufpos  Position of next text unit within buffer             */
/*      bufrem  Number of bytes remaining in buffer                  */
/*      pkey    Pointer to field to receive text unit key            */
/*      pnum    Pointer to field to receive number of data items     */
/*      maxnum  Maximum number of data items expected                */
/*      plen    Pointer to array to receive data item lengths        */
/*      pdata   Pointer to array to receive data item pointers       */
/* Output:                                                           */
/*      The function return value is the total length of the         */
/*      text unit, or -1 if error.                                   */
/*                                                                   */
/* Text units are listed if infolvl is 4 or greater.                 */
/*-------------------------------------------------------------------*/
static int
next_tu (BYTE *xbuf, int bufpos, int bufrem, U16 *pkey, U16 *pnum,
        U16 maxnum, U16 plen[], BYTE *pdata[])
{
int     i, j;                           /* Array subscripts          */
U16     key, num;                       /* Text unit header          */
int     field;                          /* Field number              */
int     offset;                         /* Offset into text unit     */
U16     len;                            /* Field length              */
char   *name;                           /* Text unit name            */
BYTE    c, chars[9];                    /* Character work areas      */
char    hex[17];                        /* Character work areas      */

    /* Error if remaining length is insufficient for header */
    if (bufrem < 4)
    {
        // "Incomplete text unit"
        XMERR ( MSG( HHC02531, "E" ) );
        return -1;
    }

    /* Load the key and field count from the first 4 bytes */
    key = (xbuf[bufpos] << 8) | xbuf[bufpos+1];
    num = (xbuf[bufpos+2] << 8) | xbuf[bufpos+3];

    /* Obtain the text unit name */
    name = tu_name(key);

    /* Print the text unit name and field count */

    // "\t+%04X %-8.8s %04X %04X"
    XMINFF (4, MSG_C( HHC02526, "I", bufpos, name, key, num ) );

    /* Error if number of fields exceeds maximum */
    if (num > maxnum)
    {
        XMINF (4, "\n");

        // "Too many fields in text unit"
        XMERR ( MSG( HHC02532, "E" ) );
        return -1;
    }

    /* Point to first field */
    offset = 4;
    bufrem -= 4;

    /* Process each field in text unit */
    for (field = 0; field < num; field++)
    {
        /* Error if remaining length is insufficient for length */
        if (bufrem < 2)
        {
            XMINF (4, "\n");
            // "Incomplete text unit"
            XMERR ( MSG( HHC02531, "E" ) );
            return -1;
        }

        /* Load field length from next 2 bytes */
        len = (xbuf[bufpos+offset] << 8) | xbuf[bufpos+offset+1];
        offset += 2;
        bufrem -= 2;

        /* Error if remaining length is insufficient for data */
        if (bufrem < len)
        {
            XMINF (4, "\n");
            // "Incomplete text unit"
            XMERR ( MSG( HHC02531, "E" ) );
            return -1;
        }

        /* Print field length and data */
        if (field > 0) XMINF (4, "\n\t\t\t\t ");
        XMINFF (4, "%04X ", len);
        memset (hex, 0, sizeof(hex));
        memset (chars, 0, sizeof(chars));
        for (i = 0, j = 0; i < len; i++, j++)
        {
            if (i > 0 && (i & 0x07) == 0)
            {
                XMINFF (4, "%-16.16s %-8.8s\n\t\t\t\t      ",
                    hex, chars);
                memset (hex, 0, sizeof(hex));
                memset (chars, 0, sizeof(chars));
                j = 0;
            }
            sprintf(hex+2*j, "%2.2X", xbuf[bufpos+offset+i]);
            c = guest_to_host(xbuf[bufpos+offset+i]);
            if (!isprint(c)) c = '.';
            chars[j] = c;
        } /* end for(i) */
        XMINFF (4, "%-16.16s %-8.8s", hex, chars);

        /* Save field length and pointer in array */
        plen[field] = len;
        pdata[field] = xbuf + bufpos + offset;

        /* Get offset of next field in text unit */
        offset += len;
        bufrem -= len;

    } /* end for */

    /* Print newline at end of text unit */
    XMINF (4, "\n");

    /* Return key, number of fields, and total length */
    *pkey = key;
    *pnum = num;
    return offset;

} /* end function next_tu */

/*-------------------------------------------------------------------*/
/* Subroutine to assemble a logical record from segments             */
/* Input:                                                            */
/*      xfd     Input file descriptor                                */
/*      xfname  Input file name                                      */
/*      xbuf    Pointer to buffer to receive logical record          */
/* Output:                                                           */
/*      ctl     Zero=data record, non-zero=control record            */
/*      The return value is the logical record length,               */
/*      or -1 if an error occurred.                                  */
/*-------------------------------------------------------------------*/
static int
read_xmit_rec (int xfd, char *xfname, BYTE *xbuf, BYTE *ctl)
{
int             rc;                     /* Return code               */
int             xreclen = 0;            /* Cumulative record length  */
int             segnum;                 /* Segment counter           */
int             seglen;                 /* Segment data length       */
BYTE            ctlind = 0x00;          /* 0x20=Control record       */
BYTE            seghdr[2];              /* Segment length and flags  */

    for (segnum = 0; ; segnum++)
    {
        /* Read the segment length and flags */
        rc = read (xfd, seghdr, 2);
        if (rc < 2)
        {
            // "File %s: read error: %s"
            XMERRF ( MSG( HHC02533, "E", xfname, rc < 0 ? strerror(errno) : "Unexpected end of file" ) );
            return -1;
        }

        /* Check for valid segment header */
        if (seghdr[0] < 2 || (seghdr[1] & 0x1F) != 0)
        {
            // "File %s: invalid segment header: %02X%02X"
            XMERRF ( MSG( HHC02534, "E", xfname, seghdr[0], seghdr[1] ) );
            return -1;
        }

        /* Check flags for first segment */
        if (segnum == 0)
        {
            /* Check that first segment indicator is set */
            if ((seghdr[1] & 0x80) == 0)
            {
                // "File %s: first segment indicator expected"
                XMERRF ( MSG( HHC02535, "E", xfname ) );
                return -1;
            }

            /* Save the control record indicator */
            ctlind = (seghdr[1] & 0x20);
        }

        /* Check flags for subsequent segments */
        if (segnum > 0)
        {
            /* Check that first segment indicator is not set */
            if (seghdr[1] & 0x80)
            {
                // "File %s: first segment indicator not expected"
                XMERRF ( MSG( HHC02536, "E", xfname ) );
                return -1;
            }

            /* Check if ctlrec indicator matches first segment */
            if ((seghdr[1] & 0x20) != ctlind)
            {
                // "File %s: control record indicator mismatch"
                XMERRF ( MSG( HHC02537, "E", xfname ) );
                return -1;
            }
        }

        /* Read segment data into buffer */
        seglen = seghdr[0] - 2;
        rc = read (xfd, xbuf + xreclen, seglen);
        if (rc < seglen)
        {
            // "File %s: read error: %s"
            XMERRF ( MSG( HHC02533, "E", xfname, rc < 0 ? strerror(errno) : "Unexpected end of file" ) );
            return -1;
        }

        /* Accumulate total record length */
        xreclen += seglen;

        /* Exit if last segment of record */
        if (seghdr[1] & 0x40)
            break;

    } /* end for(segnum) */

    /* Return record length and control indicator */
    *ctl = ctlind;
    return xreclen;

} /* end function read_xmit_rec */

/*-------------------------------------------------------------------*/
/* Subroutine to assemble a logical record from DSORG=VS file        */
/* Input:                                                            */
/*      xfd     Input file descriptor                                */
/*      xfname  Input file name                                      */
/*      xbuf    Pointer to buffer to receive logical record          */
/*      recnum  Relative number for the record to be read            */
/* Output:                                                           */
/*      The return value is the logical record length,               */
/*      or -1 if an error occurred.                                  */
/*-------------------------------------------------------------------*/
static int
read_vs_rec (int xfd, char *xfname, BYTE *xbuf, int recnum)
{
int             rc;                     /* Return code               */
int             xreclen;                /* Cumulative record length  */
DATABLK        *datablk;                /* Data block                */

    if (recnum == 0) {
       xreclen = read(xfd, xbuf, 56);   /* read COPYR1 plus some extras */
       if (xreclen < 56)
        {
            // "File %s: read error: %s"
            XMERRF ( MSG( HHC02533, "E", xfname, xreclen < 0 ? strerror(errno) : "Unexpected end of file" ) );
            return -1;
        }
     }

    else if (recnum == 1) {
       xreclen = read(xfd, xbuf, sizeof(COPYR2));  /* read COPYR2 */
       if (xreclen < (int)sizeof(COPYR2))
        {
            // "File %s: read error: %s"
            XMERRF ( MSG( HHC02533, "E", xfname, xreclen < 0 ? strerror(errno) : "Unexpected end of file" ) );
            return -1;
        }
     }

    else {
       rc = read(xfd, xbuf, 12);        /* read header of DATABLK */
       if (rc == 0)                     /* read nothing? */
          return 0;
       if (rc < 12)
        {
            // "File %s: read error: %s"
            XMERRF ( MSG( HHC02533, "E", xfname, rc < 0 ? strerror(errno) : "Unexpected end of file" ) );
            return -1;
        }
       datablk = (DATABLK *)xbuf;
       xreclen = ((datablk->dlen[0] << 8) | datablk->dlen[1])
               + datablk->klen;
       rc = read(xfd, xbuf + 12, xreclen);  /* read kdarea of DATABLK */
       if (rc < xreclen)
        {
            // "File %s: read error: %s"
            XMERRF ( MSG( HHC02533, "E", xfname, rc < 0 ? strerror(errno) : "Unexpected end of file" ) );
            return -1;
        }
       xreclen += 12;                   /* also count the header */
    }

    /* Return record length */
    return xreclen;

} /* end function read_vs_rec */

/*-------------------------------------------------------------------*/
/* Subroutine to process an INMR02 control record                    */
/* Input:                                                            */
/*      xbuf    Pointer to buffer containing control record          */
/*      xreclen Length of control record                             */
/*      filen   Pointer to field to receive file sequence number     */
/*      dsorg   Pointer to byte to receive dataset organization      */
/*      recfm   Pointer to byte to receive record format             */
/*      lrecl   Pointer to integer to receive logical record length  */
/*      blksz   Pointer to integer to receive block size             */
/*      keyln   Pointer to integer to receive key length             */
/*      dirnm   Pointer to integer to number of directory blocks     */
/* Output:                                                           */
/*      If the record contains the text unit INMUTILN=IEBCOPY        */
/*      then the dataset attributes are returned and the function    */
/*      return value is 1.  Otherwise the return value is 0          */
/*      and the dataset attributes remain unchanged.                 */
/*      If an error occurs then the return value is -1.              */
/*                                                                   */
/* File information is listed if infolvl is 2 or greater.            */
/*-------------------------------------------------------------------*/
static int
process_inmr02 (BYTE *xbuf, int xreclen, int *filen,
                BYTE *dsorg, BYTE *recfm, U16 *lrecl, U16 *blksz,
                U16 *keyln, U16 *dirnm)
{
int             rc;                     /* Return code               */
int             i;                      /* Array subscript           */
int             len;                    /* String length             */
U32             filenum;                /* File number               */
int             bufpos;                 /* Position of TU in buffer  */
int             bufrem;                 /* Bytes remaining in buffer */
char            tuutiln[9];             /* Utility name              */
BYTE            tukeyln;                /* Key length                */
HWORD           tudsorg;                /* Data set organization     */
HWORD           turecfm;                /* Record format             */
U16             tulrecl;                /* Logical record length     */
U16             tublksz;                /* Block size                */
int             tudirct;                /* Number of directory blocks*/
U16             tukey;                  /* Text unit key             */
U16             tunum;                  /* Number of text unit fields*/
char            tudsnam[45];            /* Data set name             */
#define MAXNUM  20                      /* Maximum number of fields  */
U16             fieldlen[MAXNUM];       /* Array of field lengths    */
BYTE           *fieldptr[MAXNUM];       /* Array of field pointers   */

    /* Extract the file number which follows the record name */
    filenum = (xbuf[6] << 24) | (xbuf[7] << 16)
            | (xbuf[8] << 8) | xbuf[9];

    // "File number: %d"
    XMINFF (4, MSG( HHC02527, "I", filenum ) );

    /* Point to the first text unit */
    bufpos = 10;
    bufrem = xreclen-10;

    /* Clear values to be loaded from text units */
    memset (tudsnam, 0, sizeof(tudsnam));
    memset (tuutiln, 0, sizeof(tuutiln));
    memset (tudsorg, 0, sizeof(tudsorg));
    memset (turecfm, 0, sizeof(turecfm));
    tulrecl = 0;
    tublksz = 0;
    tukeyln = 0;
    tudirct = 0;

    /* Process each text unit */
    while (bufrem > 0)
    {
        /* Extract the next text unit */
        rc = next_tu (xbuf, bufpos, bufrem, &tukey, &tunum,
                MAXNUM, fieldlen, fieldptr);
        if (rc < 0)
        {
            // "Invalid text unit at offset %04X"
            XMERRF ( MSG( HHC02538, "E", bufpos + 2 ) );
            return -1;
        }
        bufpos += rc;
        bufrem -= rc;

        /* Save the values from selected text units */
        switch (tukey) {
        case INMUTILN:
            make_asciiz (tuutiln, sizeof(tuutiln),
                        fieldptr[0], fieldlen[0]);
            break;
        case INMDSORG:
            if (fieldlen[0] > sizeof(tudsorg))
                fieldlen[0] = sizeof(tudsorg);
            memcpy (tudsorg, fieldptr[0], fieldlen[0]);
            break;
        case INMRECFM:
            if (fieldlen[0] > sizeof(turecfm))
                fieldlen[0] = sizeof(turecfm);
            memcpy (turecfm, fieldptr[0], fieldlen[0]);
            break;
        case INMLRECL:
            tulrecl = (U16) make_int (fieldptr[0], fieldlen[0]);
            break;
        case INMBLKSZ:
            tublksz = (U16) make_int (fieldptr[0], fieldlen[0]);
            break;
        case INMTYPE:
            tukeyln = (BYTE) make_int (fieldptr[0], fieldlen[0]);
            break;
        case INMDIR:
            tudirct = make_int (fieldptr[0], fieldlen[0]);
            break;
        case INMDSNAM:
            memset (tudsnam, 0, sizeof(tudsnam));
            for (i = 0; i < tunum; i++)
            {
#if defined( _MSVC_ )
                len = (int)strlen(tudsnam);
#else
                len = strlen(tudsnam);
#endif
                if (i > 0 && len < (int)(sizeof(tudsnam) - 1))
                    tudsnam[len++] = '.';
                make_asciiz (tudsnam + len, sizeof(tudsnam) - len,
                                fieldptr[i], fieldlen[i]);
            } /* end for(i) */
        } /* end switch(tukey) */
    } /* end while(bufrem) */

    /* Return the dataset values if this is the IEBCOPY record */
    if (strcmp(tuutiln, "IEBCOPY") == 0)
    {
        // "File[%04u]: DSNAME=%s"
        XMINFF (2, MSG( HHC02528, "I", filenum, tudsnam ) );

        // "            DSORG=%s RECFM=%s LRECL=%d BLKSIZE=%d KEYLEN=%d DIRBLKS=%d"
        XMINFF (2, MSG( HHC02529, "I", dsorg_name(tudsorg), recfm_name(turecfm),
                                       tulrecl, tublksz, tukeyln, tudirct ) );
        *filen = filenum;
        *dsorg = tudsorg[0];
        *recfm = turecfm[0];
        *lrecl = tulrecl;
        *blksz = tublksz;
        *keyln = tukeyln;
        *dirnm = (U16) tudirct;
    }

    return 0;
} /* end function process_inmr02 */

/*-------------------------------------------------------------------*/
/* Subroutine to process a control record other than INMR02          */
/* Input:                                                            */
/*      xbuf    Pointer to buffer containing control record          */
/*      xreclen Length of control record                             */
/* Output:                                                           */
/*      The return value is 0 if successful, or -1 if error.         */
/*-------------------------------------------------------------------*/
static int
process_inmrxx (BYTE *xbuf, int xreclen)
{
int             rc;                     /* Return code               */
int             bufpos;                 /* Position of TU in buffer  */
int             bufrem;                 /* Bytes remaining in buffer */
U16             tukey;                  /* Text unit key             */
U16             tunum;                  /* Number of text unit fields*/
#define MAXNUM  20                      /* Maximum number of fields  */
U16             fieldlen[MAXNUM];       /* Array of field lengths    */
BYTE           *fieldptr[MAXNUM];       /* Array of field pointers   */

    /* Point to the first text unit */
    bufpos = 6;
    bufrem = xreclen-6;

    /* Process each text unit */
    while (bufrem > 0)
    {
        /* Extract the next text unit */
        rc = next_tu (xbuf, bufpos, bufrem, &tukey, &tunum,
                MAXNUM, fieldlen, fieldptr);
        if (rc < 0)
        {
            // "Invalid text unit at offset %04X"
            XMERRF ( MSG( HHC02538, "E", bufpos + 2 ) );
            return -1;
        }

        bufpos += rc;
        bufrem -= rc;

    } /* end while(bufrem) */

    return 0;
} /* end function process_inmrxx */

/*-------------------------------------------------------------------*/
/* Subroutine to process a COPYR1 header record                      */
/* Input:                                                            */
/*      xbuf    Pointer to buffer containing header record           */
/*      xreclen Length of header record                              */
/* Output:                                                           */
/*      The return value is the number of tracks per cylinder,       */
/*      or -1 if an error occurred.                                  */
/*-------------------------------------------------------------------*/
static int
process_copyr1 (BYTE *xbuf, int xreclen)
{
COPYR1 *copyr1 = (COPYR1*)xbuf;         /* -> COPYR1 header record   */
U16     blksize;                        /* Block size                */
U16     lrecl;                          /* Logical record length     */
BYTE    keylen;                         /* Key length                */
U16     cyls;                           /* Number of cylinders       */
U16     heads;                          /* Number of tracks/cylinder */

    /* Check COPYR1 record for correct length */
    if (xreclen != sizeof(COPYR1)
        && xreclen != sizeof(COPYR1) - 4)
    {
        // "COPYR%d record length is invalid"
        XMERR ( MSG( HHC02539, "E", 1 ) );
        return -1;
    }

    /* Check that COPYR1 header identifier is correct */
    if (memcmp(copyr1->hdrid, COPYR1_HDRID, 3) != 0)
    {
        // "COPYR%d header identifier not correct"
        XMERR ( MSG( HHC02540, "E", 1 ) );
        return -1;
    }

    /* Check that the dataset is an old format unload */
    if ((copyr1->uldfmt & COPYR1_ULD_FORMAT)
            != COPYR1_ULD_FORMAT_OLD)
    {
        // "COPYR%d unload format is unsupported"
        XMERR ( MSG( HHC02541, "E", 1 ) );
        return -1;
    }

    blksize = (copyr1->ds1blkl[0] << 8) | copyr1->ds1blkl[1];
    lrecl = (copyr1->ds1lrecl[0] << 8) | copyr1->ds1lrecl[1];
    keylen = copyr1->ds1keyl;

    /* Display original dataset information */

    // "Original dataset: DSORG=%s RECFM=%s LRECL=%d BLKSIZE=%d KEYLEN=%d"
    XMINFF (2, MSG( HHC02550, "I",
            dsorg_name(copyr1->ds1dsorg),
            recfm_name(&copyr1->ds1recfm),
            lrecl, blksize, keylen ) );

    // "Dataset was unloaded from device type %02X%02X%02X%02X (%s)"
    XMINFF (2, MSG( HHC02551, "I",
            copyr1->ucbtype[0], copyr1->ucbtype[1],
            copyr1->ucbtype[2], copyr1->ucbtype[3],
            dasd_name(copyr1->ucbtype) ) );

    cyls  = fetch_hw( copyr1->cyls  );
    heads = fetch_hw( copyr1->heads );

    // "Original device was cyls[%04X/%d], heads[%04X/%d]"
    XMINFF (2, MSG( HHC02552, "I", cyls, cyls, heads, heads ) );

    return heads;
} /* end function process_copyr1 */

/*-------------------------------------------------------------------*/
/* Subroutine to process a COPYR2 header record                      */
/* Input:                                                            */
/*      xbuf    Pointer to buffer containing header record           */
/*      xreclen Length of header record                              */
/*      xarray  Pointer to array to receive 1-16 extent descriptions */
/* Output:                                                           */
/*      The return value is the number of extents,                   */
/*      or -1 if an error occurred.                                  */
/*                                                                   */
/* Extent information is listed if infolvl is 4 or greater.          */
/*-------------------------------------------------------------------*/
static int
process_copyr2 (BYTE *xbuf, int xreclen, EXTDESC xarray[])
{
COPYR2 *copyr2 = (COPYR2*)xbuf;         /* -> COPYR2 header record   */
int     numext;                         /* Number of extents         */
int     i;                              /* Array subscript           */

    /* Check COPYR2 record for correct length */
    if (xreclen != sizeof(COPYR2))
    {
        // "COPYR%d record length is invalid"
        XMERR ( MSG( HHC02539, "E", 2 ) );
        return -1;
    }

    /* Get number of extents from DEB basic section */
    numext = copyr2->debbasic[0];
    if (numext < 1 || numext > 16)
    {
        // "Invalid number of extents %d"
        XMERRF ( MSG( HHC02542, "E", numext ) );
        return -1;
    }

    /* Copy each extent descriptor into the array */
    for (i = 0; i < numext; i++)
    {
        xarray[i].bcyl = (copyr2->debxtent[i][6] << 8)
                        | copyr2->debxtent[i][7];
        xarray[i].btrk = (copyr2->debxtent[i][8] << 8)
                        | copyr2->debxtent[i][9];
        xarray[i].ecyl = (copyr2->debxtent[i][10] << 8)
                        | copyr2->debxtent[i][11];
        xarray[i].etrk = (copyr2->debxtent[i][12] << 8)
                        | copyr2->debxtent[i][13];
        xarray[i].ntrk = (copyr2->debxtent[i][14] << 8)
                        | copyr2->debxtent[i][15];

        // "Extent %d: Begin CCHH[%04X%04X] End CCHH[%04X%04X] Tracks=[%04X/%d]"
        XMINFF (4, MSG( HHC02553, "I", i, xarray[i].bcyl, xarray[i].btrk,
                                          xarray[i].ecyl, xarray[i].etrk,
                                          xarray[i].ntrk, xarray[i].ntrk ) );

    } /* end for(i) */

    /* Return number of extents */
    return numext;
} /* end function process_copyr2 */

/*-------------------------------------------------------------------*/
/* Subroutine to process a directory block record                    */
/* Input:                                                            */
/*      xbuf    Pointer to directory block                           */
/*      blklen  Length of directory block                            */
/*      cyl     Cylinder number of directory block in output file    */
/*      head    Head number of directory block in output file        */
/*      rec     Record number of directory block in output file      */
/*      dirblka Array of pointers to directory blocks                */
/*      dirblkn Number of directory blocks in dirblka array          */
/* Output:                                                           */
/*      dirblu  Number of bytes used in directory block              */
/*      The return value is 0 if successful, 1 if end of directory,  */
/*      or -1 if an error occurred.                                  */
/*                                                                   */
/* A pointer to the directory block is saved in the dirblka array.   */
/* The copy of the data block is updated with the cylinder, head,    */
/* and record number of the directory block in the output file.      */
/* Directory information is listed if infolvl is 3 or greater.       */
/*-------------------------------------------------------------------*/
static int
process_dirblk (DATABLK *xbuf, int blklen, int cyl, int head, int rec,
                DATABLK *dirblka[], int dirblkn, int *dirblu)
{
int             size;                   /* Size of directory entry   */
int             i, j;                   /* Array subscripts          */
int             k;                      /* Userdata halfword count   */
DATABLK        *blkp;                   /* -> Copy of directory block*/
BYTE           *dirptr;                 /* -> Next byte within block */
int             dirrem;                 /* Number of bytes remaining */
PDSDIR         *dirent;                 /* -> Directory entry        */
char            memname[9];             /* Member name (ASCIIZ)      */
BYTE            c, chars[25];           /* Character work areas      */
char            hex[49];                /* Character work areas      */

    /* Check for end of directory */
    if (blklen == 12 && memcmp(xbuf, twelvehex00, 12) == 0)
    {
        // "End of directory"
        XMINF (3, MSG( HHC02554, "I" ) );
        return 1;
    }

    /* Check directory block record for correct length */
    if (blklen != 276)
    {
        // "Invalid directory block record length"
        XMERR ( MSG( HHC02543, "E" ) );
        return -1;
    }

    /* Obtain storage for a copy of the directory block */
    blkp = (DATABLK*)malloc(blklen);
    if (blkp == NULL)
    {
        // "Storage allocation for %s using %s failed: %s"
        XMERRF ( MSG( HHC02544, "E", "a directory block", "malloc()", strerror( errno ) ) );
        return -1;
    }

    /* Copy the directory block */
    memcpy (blkp, xbuf, blklen);

    /* Check that there is room in the directory block pointer array */
    if (dirblkn >= nMaxDBLK)
    {
        // "Internal error: Number of directory blocks exceeds MAXDBLK of %d"
        XMERRF ( MSG( HHC02545, "E", nMaxDBLK ) );
        return -1;
    }

    /* Add the directory block to the pointer array */
    dirblka[dirblkn] = blkp;

    /* Update the CCHHR in the copy of the directory block */
    store_hw( blkp->cyl,  cyl  );
    store_hw( blkp->head, head );
              blkp->rec = rec;

    /* Load number of bytes in directory block */
    dirptr = xbuf->kdarea + 8;
    dirrem = (dirptr[0] << 8) | dirptr[1];
    if (dirrem < 2 || dirrem > 256)
    {
        // "Invalid directory block byte count"
        XMERR (MSG( HHC02546, "E" ) );
        return -1;
    }

    /* Return number of bytes used in directory block */
    *dirblu = dirrem;

    /* Point to first directory entry */
    dirptr += 2;
    dirrem -= 2;

    /* Process each directory entry */
    while (dirrem > 0)
    {
        /* Point to next directory entry */
        dirent = (PDSDIR*)dirptr;

        /* Test for end of directory */
        if (memcmp( dirent->pds2name, &CKD_ENDTRK, CKD_ENDTRK_SIZE ) == 0)
            break;

        /* Extract the member name */
        make_asciiz (memname, sizeof(memname), dirent->pds2name, 8);

        /* Display the directory entry */

        // "%s %-8.8s TTR[%02X%02X%02X] "
        XMINFF (3, MSG_C( HHC02555, "I", (dirent->pds2indc & PDS2INDC_ALIAS) ? " Alias" : "Member",
                                        memname, dirent->pds2ttrp[0],
                                        dirent->pds2ttrp[1], dirent->pds2ttrp[2] ) );

        /* Load the user data halfword count */
        k = dirent->pds2indc & PDS2INDC_LUSR;

        /* Print the user data */
        if (k > 0) XMINF (3, "Userdata=");
        memset (hex, 0, sizeof(hex));
        memset (chars, 0, sizeof(chars));
        for (i = 0, j = 0; i < k*2; i++, j++)
        {
            if (i == 8 || i == 32 || i == 56)
            {
                if (i == 8)
                    XMINFF (3, "%-16.16s %-8.8s\n  ", hex, chars);
                else
                    XMINFF (3, "%-16.16s %-16.16s %16.16s %-24.24s\n  ",
                        hex, hex+16, hex+32, chars);
                memset (hex, 0, sizeof(hex));
                memset (chars, 0, sizeof(chars));
                j = 0;
            }
            sprintf(hex+2*j, "%2.2X", dirent->pds2usrd[i]);
            c = guest_to_host(dirent->pds2usrd[i]);
            if (!isprint(c)) c = '.';
            chars[j] = c;
        } /* end for(i) */
        if (i <= 8)
            XMINFF (3, "%-16.16s %-8.8s\n", hex, chars);
        else
            XMINFF (3, "%-16.16s %-16.16s %-16.16s %-24.24s\n",
                hex, hex+16, hex+32, chars);

        /* Point to next directory entry */
        size = 12 + k*2;
        dirptr += size;
        dirrem -= size;
    }

    return 0;
} /* end function process_dirblk */

/*-------------------------------------------------------------------*/
/* Subroutine to replace a TTR in a PDS directory                    */
/* Input:                                                            */
/*      memname Member name (ASCIIZ)                                 */
/*      ttrptr  Pointer to TTR to be replaced                        */
/*      ttrtab  Pointer to TTR conversion table                      */
/*      numttr  Number of entries in TTR conversion table            */
/* Output:                                                           */
/*      The TTR is replaced using the TTR conversion table.          */
/*                                                                   */
/* Return value is 0 if successful, or -1 if TTR not in table.       */
/* Directory information is listed if infolvl is 3 or greater.       */
/*-------------------------------------------------------------------*/
static int
replace_ttr (char *memname, BYTE *ttrptr, TTRCONV *ttrtab, int numttr)
{
int             i;                      /* Array subscript           */

    /* Search for the TTR in the conversion table */
    for (i = 0; i < numttr; i++)
    {
        if (memcmp(ttrptr, ttrtab[i].origttr, 3) == 0)
        {
            // "Member %s TTR[%02X%02X%02X] replaced by TTR[%02X%02X%02X]"
            XMINFF (4, MSG( HHC02556, "I", memname, ttrptr[0], ttrptr[1], ttrptr[2],
                    ttrtab[i].outpttr[0], ttrtab[i].outpttr[1],
                    ttrtab[i].outpttr[2] ) );
            memcpy (ttrptr, ttrtab[i].outpttr, 3);
            return 0;
        }
    }

    /* Return error if TTR not found in conversion table */

    // "Member %s TTR[%02X%02X%02X] not found in dataset"
    XMERRF ( MSG( HHC02557, "I", memname, ttrptr[0], ttrptr[1], ttrptr[2] ) );
    return -1;

} /* end function replace_ttr */

/*-------------------------------------------------------------------*/
/* Subroutine to update a note list record                           */
/* Input:                                                            */
/*      cif     -> CKD image file descriptor                         */
/*      ofname  Output file name                                     */
/*      heads   Number of tracks per cylinder on output device       */
/*      trklen  Track length of virtual output device                */
/*      dsstart Relative track number of start of dataset            */
/*      memname Member name (ASCIIZ)                                 */
/*      ttrn    Pointer to TTRN of note list record                  */
/*      ttrtab  Pointer to TTR conversion table                      */
/*      numttr  Number of entries in TTR conversion table            */
/* Output:                                                           */
/*      Each original TTR in the note list record is replaced by the */
/*      corresponding output TTR from the TTR conversion table.      */
/*                                                                   */
/* Return value is 0 if successful, or -1 if error.                  */
/*-------------------------------------------------------------------*/
static int
update_note_list (CIFBLK *cif, char *ofname, int heads, int trklen,
                int dsstart, char *memname, BYTE *ttrn,
                TTRCONV *ttrtab, int numttr)
{
int             rc;                     /* Return code               */
int             i;                      /* Loop counter              */
int             trk;                    /* Relative track number     */
int             cyl;                    /* Cylinder number           */
int             head;                   /* Head number               */
int             rec;                    /* Record number             */
int             klen;                   /* Record key length         */
int             dlen;                   /* Record data length        */
int             numnl;                  /* Number of note list TTRs  */
int             nllen;                  /* Note list length          */
BYTE           *ttrptr;                 /* -> Note list TTR          */
int             curcyl;                 /* Current cylinder          */
int             curhead;                /* Current head              */
int             offset;                 /* Offset into track buffer  */
int             skiplen;                /* Number of bytes to skip   */
CKD_TRKHDR      trkhdr;                 /* Track header              */
CKD_RECHDR      rechdr;                 /* Record header             */
BYTE            notelist[1024];         /* Note list                 */

    UNREFERENCED(trklen);

    /* Load the TTR of the note list record */
    trk = (ttrn[0] << 8) | ttrn[1];
    rec = ttrn[2];

    /* Load number of note list TTRs and calculate note list length */
    numnl = ttrn[3];
    nllen = numnl * 4;

    /* Calculate the CCHHR of the note list record */
    cyl = (dsstart + trk) / heads;
    head = (dsstart + trk) % heads;

    // "Member %s Updating note list for member at TTR[%04X%02X] CCHHR[%04X%04X%02X]"
    XMINFF (4, MSG( HHC02558, "I", memname, trk, rec, cyl, head, rec ) );

    /* Save the current position in the output file */
    curcyl = cif->curcyl;
    curhead = cif->curhead;

    /* Seek to start of track header */
    rc = read_track( cif, cyl, (U8) head );
    if (rc < 0)
    {
        // "File %s: read error at cyl[%04X/%d] head[%04X/%d]"
        XMERRF ( MSG( HHC02547, "E", ofname, cyl, cyl, head, head ) );
        return -1;
    }

    /* Copy the track header */
    memcpy (&trkhdr, cif->trkbuf, CKD_TRKHDR_SIZE);
    offset = CKD_TRKHDR_SIZE;

    /* Validate the track header */
    if (0
        ||           trkhdr.bin    != 0
        || fetch_hw( trkhdr.cyl  ) != cyl
        || fetch_hw( trkhdr.head ) != head
    )
    {
        // "File %s: cyl[%04X/%d] head[%04X/%d] invalid track header %02X%02X%02X%02X%02X"
        XMERRF ( MSG( HHC02548, "E", ofname, cyl, cyl, head, head,
                                     trkhdr.bin, trkhdr.cyl[0], trkhdr.cyl[1],
                                     trkhdr.head[0], trkhdr.head[1] ) );
        return -1;
    }

    /* Search for the note list record */
    while (1)
    {
        /* Copy the next record header */
        memcpy (&rechdr, cif->trkbuf + offset, CKD_RECHDR_SIZE);
        offset += CKD_RECHDR_SIZE;

        /* Check for end of track */
        if (memcmp( &rechdr, &CKD_ENDTRK, CKD_ENDTRK_SIZE ) == 0)
        {
            // "File %s: cyl[%04X/%d] head[%04X/%d] rec[%02X/%d] note list record not found"
            XMERRF ( MSG( HHC02549, "E", ofname, cyl, cyl, head, head, rec, rec ) );
            return -1;
        }

        /* Extract record key length and data length */
        klen = rechdr.klen;
        dlen = (rechdr.dlen[0] << 8) | rechdr.dlen[1];

        /* Exit loop if matching record number */
        if (rechdr.rec == rec)
            break;

        /* Skip the key and data areas */
        skiplen = klen + dlen;
        offset += skiplen;
    } /* end while */

    /* Check that the data length is sufficient */
    if (dlen < nllen)
    {
        // "Member %s Note list at cyl[%04X/%d] head[%04X/%d] rec[%02X/%d] dlen[%d] is too short for %d TTRs"
        XMERRF ( MSG( HHC02559, "E", memname, cyl, cyl, head, head, rec, rec, dlen, numnl ) );
        return -1;
    }

    /* Skip the key area if present */
    offset += klen;

    /* Copy the note list from the data area */
    memcpy (&notelist, cif->trkbuf + offset, nllen);

    /* Replace the TTRs in the note list record */
    ttrptr = notelist;
    for (i = 0; i < numnl; i++)
    {
        rc = replace_ttr (memname, ttrptr, ttrtab, numttr);
        if (rc < 0) return -1;
        ttrptr += 4;
    } /* end for(i) */

    /* Copy the updated note list to the buffer */
    memcpy (cif->trkbuf + offset, &notelist, nllen);
    cif->trkmodif = 1;

    /* Restore original file position */
    rc = read_track( cif, curcyl, (U8) curhead );
    if (rc < 0)
    {
        // "File %s: read error at cyl[%04X/%d] head[%04X/%d]"
        XMERRF ( MSG( HHC02547, "E", ofname, curcyl, curcyl, curhead, curhead ) );
        return -1;
    }

    // "Updating cyl[%04X/%d] head[%04X/%d] rec[%02X/%d]; kl[%d] dl[%d]"
    XMINFF (4, MSG( HHC02523, "I", cyl, cyl, head, head, rec, rec, klen, dlen ) );

    return 0;
} /* end function update_note_list */

/*-------------------------------------------------------------------*/
/* Subroutine to update a directory block record                     */
/* Input:                                                            */
/*      cif     -> CKD image file descriptor                         */
/*      ofname  Output file name                                     */
/*      heads   Number of tracks per cylinder on output device       */
/*      trklen  Track length of virtual output device                */
/*      dsstart Relative track number of start of dataset            */
/*      xbuf    Pointer to directory block                           */
/*      ttrtab  Pointer to TTR conversion table                      */
/*      numttr  Number of entries in TTR conversion table            */
/* Output:                                                           */
/*      Each original TTR in the directory block is replaced by the  */
/*      corresponding output TTR from the TTR conversion table.      */
/*      For any module which has note list entries, the note list    */
/*      TTRs in the note list record are also updated.               */
/*                                                                   */
/* Return value is 0 if successful, or -1 if any directory entry     */
/* contains a TTR which is not found in the TTR conversion table.    */
/*-------------------------------------------------------------------*/
static int
update_dirblk (CIFBLK *cif, char *ofname, int heads, int trklen,
                int dsstart, DATABLK *xbuf,
                TTRCONV *ttrtab, int numttr)
{
int             rc;                     /* Return code               */
int             size;                   /* Size of directory entry   */
int             k;                      /* Userdata halfword count   */
BYTE           *dirptr;                 /* -> Next byte within block */
int             dirrem;                 /* Number of bytes remaining */
PDSDIR         *dirent;                 /* -> Directory entry        */
BYTE           *ttrptr;                 /* -> User TTR               */
int             n;                      /* Number of user TTRs       */
int             i;                      /* Loop counter              */
char            memname[9];             /* Member name (ASCIIZ)      */

    /* Load number of bytes in directory block */
    dirptr = xbuf->kdarea + 8;
    dirrem = (dirptr[0] << 8) | dirptr[1];
    if (dirrem < 2 || dirrem > 256)
    {
        // "Invalid directory block byte count"
        XMERR ( MSG( HHC02546, "E" ) );
        return -1;
    }

    /* Point to first directory entry */
    dirptr += 2;
    dirrem -= 2;

    /* Process each directory entry */
    while (dirrem > 0)
    {
        /* Point to next directory entry */
        dirent = (PDSDIR*)dirptr;

        /* Test for end of directory */
        if (memcmp( dirent->pds2name, &CKD_ENDTRK, CKD_ENDTRK_SIZE ) == 0)
            break;

        /* Extract the member name */
        make_asciiz (memname, sizeof(memname), dirent->pds2name, 8);

        /* Replace the member TTR */
        rc = replace_ttr (memname, dirent->pds2ttrp, ttrtab, numttr);
        if (rc < 0) return -1;

        /* Load the number of user TTRs */
        n = (dirent->pds2indc & PDS2INDC_NTTR) >> PDS2INDC_NTTR_SHIFT;

        /* Replace the user TTRs */
        ttrptr = dirent->pds2usrd;
        for (i = 0; i < n; i++)
        {
            rc = replace_ttr (memname, ttrptr, ttrtab, numttr);
            if (rc < 0) return -1;
            ttrptr += 4;
        } /* end for(i) */

        /* Update the note list record if note list TTRs exist */
        if ((dirent->pds2indc & PDS2INDC_ALIAS) == 0
            && n >= 2 && dirent->pds2usrd[7] != 0)
        {
            rc = update_note_list (cif, ofname, heads, trklen,
                                dsstart, memname, dirent->pds2usrd+4,
                                ttrtab, numttr);
            if (rc < 0) return -1;
        }

        /* Load the user data halfword count */
        k = dirent->pds2indc & PDS2INDC_LUSR;

        /* Point to next directory entry */
        size = 12 + k*2;
        dirptr += size;
        dirrem -= size;
    }

    return 0;
} /* end function update_dirblk */

/*-------------------------------------------------------------------*/
/* Subroutine to read an IEBCOPY file and write to DASD image file   */
/* Input:                                                            */
/*      xfname  XMIT input file name                                 */
/*      ofname  DASD image file name                                 */
/*      cif     -> CKD image file descriptor                         */
/*      devtype Output device type                                   */
/*      heads   Output device number of tracks per cylinder          */
/*      trklen  Output device virtual track length                   */
/*      outcyl  Output starting cylinder number                      */
/*      outhead Output starting head number                          */
/*      maxtrks Maximum extent size in tracks                        */
/*      method  METHOD_XMIT or METHOD_VS                             */
/* Output:                                                           */
/*      odsorg  Dataset organization                                 */
/*      orecfm  Record format                                        */
/*      olrecl  Logical record length                                */
/*      oblksz  Block size                                           */
/*      okeyln  Key length                                           */
/*      dirblu  Bytes used in last directory block                   */
/*      lastrec Record number of last block written                  */
/*      trkbal  Number of bytes remaining on last track              */
/*      numtrks Number of tracks written                             */
/*      nxtcyl  Starting cylinder number for next dataset            */
/*      nxthead Starting head number for next dataset                */
/*-------------------------------------------------------------------*/
static int
process_iebcopy_file (char *xfname, char *ofname, CIFBLK *cif,
                U16 devtype, int heads, int trklen,
                int outcyl, int outhead, int maxtrks,
                BYTE method,
                BYTE *odsorg, BYTE *orecfm,
                int *olrecl, int *oblksz, int *okeyln,
                int *dirblu, int *lastrec, int *trkbal,
                int *numtrks, int *nxtcyl, int *nxthead)
{
int             rc = 0;                 /* Return code               */
int             i;                      /* Array subscript           */
int             xfd;                    /* XMIT file descriptor      */
int             dsstart;                /* Relative track number of
                                           start of output dataset   */
BYTE           *xbuf;                   /* -> Logical record buffer  */
int             xreclen;                /* Logical record length     */
BYTE            xctl;                   /* 0x20=Control record       */
char            xrecname[8];            /* XMIT control record name  */
int             datarecn = 0;           /* Data record counter       */
int             datafiln = 0;           /* Data file counter         */
int             copyfiln = 0;           /* Seq num of file to copy   */
BYTE            dsorg=0;                /* Dataset organization      */
BYTE            recfm=0;                /* Dataset record format     */
U16             lrecl=0;                /* Dataset record length     */
U16             blksz=0;                /* Dataset block size        */
U16             keyln=0;                /* Dataset key length        */
U16             dirnm;                  /* Number of directory blocks*/
int             enddir = 0;             /* 1=End of directory found  */
BYTE           *blkptr;                 /* -> Data block in record   */
DATABLK        *datablk;                /* -> Data block             */
int             blktrk;                 /* Data block relative track */
int             blkcyl;                 /* Data block cylinder number*/
int             blkhead;                /* Data block head number    */
int             blkrec;                 /* Data block record number  */
int             keylen;                 /* Key length of data block  */
int             datalen;                /* Data length of data block */
int             blklen;                 /* Total length of data block*/
int             origheads = 0;          /* Number of tracks/cylinder
                                           on original dataset       */
int             numext = 0;             /* Number of extents         */
EXTDESC         xarray[16];             /* Extent descriptor array   */
DATABLK       **dirblka;                /* -> Directory block array  */
int             dirblkn = 0;            /* #of directory blocks read */
int             outusedv = 0;           /* Output bytes used on track
                                           of virtual device         */
int             outusedr = 0;           /* Output bytes used on track
                                           of real device            */
int             outtrkbr = 0;           /* Output bytes remaining on
                                           track of real device      */
int             outtrk = 0;             /* Output relative track     */
int             outrec = 0;             /* Output record number      */
TTRCONV        *ttrtab;                 /* -> TTR conversion table   */
int             numttr = 0;             /* TTR table array index     */
COPYR1         *copyr1;                 /* -> header record 1        */
char            pathname[MAX_PATH];     /* xfname in host path format*/

    /* Open the input file */
    hostpath(pathname, xfname, sizeof(pathname));
    xfd = HOPEN (pathname, O_RDONLY|O_BINARY);
    if (xfd < 0)
    {
        // "File %s; %s error: %s"
        XMERRF ( MSG( HHC02468, "E", xfname, "open()", strerror( errno ) ) );
        return -1;
    }

    /* Obtain the input logical record buffer */
    xbuf = malloc (65536);
    if (xbuf == NULL)
    {
        // "Storage allocation for %s using %s failed: %s"
        XMERRF ( MSG( HHC02544, "E", "input buffer", "malloc()", strerror(errno) ) );
        close (xfd);
        return -1;
    }

    /* Obtain storage for the directory block array */
    dirblka = (DATABLK**)malloc (sizeof(DATABLK*) * nMaxDBLK);
    if (dirblka == NULL)
    {
        // "Storage allocation for %s using %s failed: %s"
        XMERRF ( MSG( HHC02544, "E", "directory block array", "malloc()", strerror(errno) ) );
        free (xbuf);
        close (xfd);
        return -1;
    }

    /* Obtain storage for the TTR conversion table */
    ttrtab = (TTRCONV*)malloc (sizeof(TTRCONV) * nMaxTTR);
    if (ttrtab == NULL)
    {
        // "Storage allocation for %s using %s failed: %s"
        XMERRF ( MSG( HHC02544, "E", "TTR table", "malloc()", strerror(errno) ) );
        free (xbuf);
        free (dirblka);
        close (xfd);
        return -1;
    }

    /* Calculate the relative track number of the dataset */
    dsstart = (outcyl * heads) + outhead;

    /* Display the file information message */

    // "File %s: Processing"
    XMINFF (1, MSG( HHC02560, "I", xfname ) );

    /* Read each logical record */
    while (1)
    {
        xctl=0;
        if (method == METHOD_XMIT)
           rc = read_xmit_rec (xfd, xfname, xbuf, &xctl);
        else if (method == METHOD_VS) {
           rc = read_vs_rec (xfd, xfname, xbuf, datarecn);
           if (rc == 0)                 /* end-of-file */
              break;
        } else
           rc = -1;
        if (rc < 0) return -1;
        xreclen = rc;

        /* Process control records */
        if (method == METHOD_XMIT && xctl)
        {
            /* Extract the control record name */
            make_asciiz (xrecname, sizeof(xrecname), xbuf, 6);

            // "Control record: %s len[%d]"
            XMINFF (4, MSG( HHC02561, "I", xrecname, xreclen ) );

            /* Exit if control record is a trailer record */
            if (strcmp(xrecname, "INMR06") == 0)
                break;

            /* Process control record according to type */
            if (strcmp(xrecname, "INMR02") == 0)
            {
                rc = process_inmr02 (xbuf, xreclen, &copyfiln,
                                     &dsorg, &recfm, &lrecl, &blksz,
                                     &keyln, &dirnm);
                if (rc < 0) return -1;
            }
            else
            {
                rc = process_inmrxx (xbuf, xreclen);
                if (rc < 0) return -1;
            }

            /* Reset the data counter if data control record */
            if (strcmp(xrecname, "INMR03") == 0)
            {
                datafiln++;
                datarecn = 0;

                // "File number:    %d %s"
                XMINFF (4, MSG( HHC02562, "I", datafiln, datafiln == copyfiln ? "(selected)" : "(not selected)" ) );
            }

            /* Loop to get next record */
            continue;

        } /* end if(xctl) */

        /* Process data records */
        datarecn++;

        // "Data record:    len[%d]"
        XMINFF (4, MSG( HHC02563, "I", xreclen ) );
        if (infolvl >= 5) data_dump (xbuf, xreclen);

        /* If this is not the IEBCOPY file then ignore data record */
        if (method == METHOD_XMIT && datafiln != copyfiln)
        {
            continue;
        }

        /* Process IEBCOPY header record 1 */
        if (datarecn == 1)
        {
            origheads = process_copyr1 (xbuf, xreclen);
            if (origheads < 0) exit(1);
            if (method == METHOD_VS) {
               copyr1 = (COPYR1 *)xbuf;
               dsorg = copyr1->ds1dsorg[0];
               recfm = copyr1->ds1recfm;
               lrecl = (copyr1->ds1lrecl[0] << 8) | copyr1->ds1lrecl[1];
               blksz = (copyr1->ds1blkl[0] << 8) | copyr1->ds1blkl[1];
               keyln = copyr1->ds1keyl;
            }
            continue;
        }

        /* Process IEBCOPY header record 2 */
        if (datarecn == 2)
        {
            numext = process_copyr2 (xbuf, xreclen, xarray);
            if (numext < 0) exit(1);
            continue;
        }

        /* Process each data block in data record */
        blkptr = xbuf;
        while (xreclen > 0)
        {
            /* Compute the length of the block */
            datablk = (DATABLK*)blkptr;
            blkcyl = (datablk->cyl[0] << 8)
                    | datablk->cyl[1];
            blkhead = (datablk->head[0] << 8)
                    | datablk->head[1];
            blkrec = datablk->rec;
            keylen = datablk->klen;
            datalen = (datablk->dlen[0] << 8)
                    | datablk->dlen[1];
            blklen = 12 + keylen + datalen;

            /* Calculate the TTR in the original dataset */
            blktrk = (enddir == 0) ? 0 :
                        calculate_ttr (blkcyl, blkhead,
                                origheads, numext, xarray);

            /* Write the data block to the output file */
            rc = write_block (cif, ofname, datablk, keylen, datalen,
                        devtype, heads, trklen, maxtrks,
                        &outusedv, &outusedr, &outtrkbr,
                        &outtrk, &outcyl, &outhead, &outrec);
            if (rc < 0)
            {
                // "File %s: %s error: input record CCHHR[%04X%04X%02X] (TTR[%04X%02X]) kl[%d] dl[%d]"
                XMERRF ( MSG( HHC02570, "E", ofname, "write_block()",
                        blkcyl, blkhead, blkrec,
                        blktrk, blkrec, keylen, datalen ) );
                return -1;
            }

            // "Input record:   CCHHR[%04X%04X%02X] (TTR[%04X%02X]) kl[%d] dl[%d]\n"
            //       "HHC02564I relocated to:   CCHHR[%04X%04X%02X] (TTR[%04X%02X])"
            XMINFF (4, MSG( HHC02564, "I", blkcyl, blkhead, blkrec, blktrk, blkrec, keylen, datalen,
                                           outcyl, outhead, outrec, outtrk, outrec ) );

            /* Process directory block or member block */
            if (enddir == 0)
            {
                rc = process_dirblk (datablk, blklen,
                                    outcyl, outhead, outrec,
                                    dirblka, dirblkn, dirblu);
                if (rc < 0) return -1;
                enddir = rc;

                /* Count the number of directory blocks read */
                if (enddir == 0) dirblkn++;
            }
            else /* Not a directory block */
            {
                /* Check that TTR conversion table is not full */
                if (numttr >= nMaxTTR)
                {
                    // "Internal error: TTR count exceeds MAXTTR of %d"
                    XMERRF ( MSG( HHC02571, "E", nMaxTTR ) );
                    return -1;
                }

                /* Add an entry to the TTR conversion table */
                ttrtab[numttr].origttr[0] = (blktrk >> 8) & 0xFF;
                ttrtab[numttr].origttr[1] = blktrk & 0xFF;
                ttrtab[numttr].origttr[2] = (BYTE) blkrec;
                ttrtab[numttr].outpttr[0] = (outtrk >> 8) & 0xFF;
                ttrtab[numttr].outpttr[1] = outtrk & 0xFF;
                ttrtab[numttr].outpttr[2] = (BYTE) outrec;
                numttr++;
            }

            /* Point to next data block in data record */
            xreclen -= blklen;
            blkptr += blklen;

        } /* end while(xreclen) */

    } /* end while(1) */

    /* Check for unsupported xmit utility */
    if (method == METHOD_XMIT && copyfiln == 0)
    {
        // "File %s: XMIT utility file is not in IEBCOPY format; file is not loaded"
        XMERRF ( MSG( HHC02572, "W", xfname ) );
    }

    /* Return the last record number and track balance */
    *lastrec = outrec;
    *trkbal = outtrkbr;

    /* Write any data remaining in track buffer */
    rc = write_track (cif, ofname, heads, trklen,
                    &outusedv, &outtrk, &outcyl, &outhead);
    if (rc < 0) return -1;

    /* Update the directory and rewrite to output file */
    for (i = 0; i < dirblkn; i++)
    {
        /* Obtain the directory block pointer from the array */
        datablk = dirblka[i];

        /* Update TTR pointers in this directory block */
        rc = update_dirblk (cif, ofname, heads, trklen, dsstart,
                            datablk, ttrtab, numttr);
        if (rc < 0) return -1;

        /* Rewrite the updated directory block */
        blkcyl = (datablk->cyl[0] << 8) | datablk->cyl[1];
        blkhead = (datablk->head[0] << 8) | datablk->head[1];
        blkrec = datablk->rec;
        keylen = datablk->klen;
        datalen = (datablk->dlen[0] << 8) | datablk->dlen[1];

        rc = update_block (cif, ofname, datablk, blkcyl, blkhead,
                        blkrec, keylen, datalen, heads, trklen);
        if (rc < 0) return -1;

    } /* end for(i) */

    /* Close input file and release buffers */
    close (xfd);
    for (i = 0; i < dirblkn; i++)
        free (dirblka[i]);
    free (dirblka);
    free (xbuf);
    free (ttrtab);

    /* Return the dataset attributes */
    *odsorg = dsorg;
    *orecfm = recfm;
    *olrecl = lrecl;
    *oblksz = blksz;
    *okeyln = keyln;

    /* Return number of tracks and starting address of next dataset */
    *numtrks = outtrk;
    *nxtcyl = outcyl;
    *nxthead = outhead;
    return 0;

} /* end function process_iebcopy_file */

/*-------------------------------------------------------------------*/
/* Subroutine to initialize a SYSCTLG dataset as an OS CVOL          */
/* Input:                                                            */
/*      ofname  DASD image file name                                 */
/*      cif     -> CKD image file descriptor                         */
/*      volser  Volume serial number                                 */
/*      devtype Output device type                                   */
/*      heads   Output device number of tracks per cylinder          */
/*      trklen  Output device virtual track length                   */
/*      outcyl  Output starting cylinder number                      */
/*      outhead Output starting head number                          */
/*      extsize Extent size in tracks                                */
/* Output:                                                           */
/*      lastrec Record number of last block written                  */
/*      trkbal  Number of bytes remaining on last track              */
/*      numtrks Number of tracks written                             */
/*      nxtcyl  Starting cylinder number for next dataset            */
/*      nxthead Starting head number for next dataset                */
/* Note:                                                             */
/*      This subroutine builds a minimal SYSCTLG containing only     */
/*      the entries required on an OS/360 IPL volume.                */
/*-------------------------------------------------------------------*/
static int
cvol_initialize (char *ofname, CIFBLK *cif, char *volser,
                U16 devtype, int heads, int trklen,
                int outcyl, int outhead, int extsize,
                int *lastrec, int *trkbal,
                int *numtrks, int *nxtcyl, int *nxthead)
{
int             rc;                     /* Return code               */
int             i;                      /* Array subscript           */
int             keylen;                 /* Key length of data block  */
int             datalen;                /* Data length of data block */
int             outusedv = 0;           /* Output bytes used on track
                                           of virtual device         */
int             outusedr = 0;           /* Output bytes used on track
                                           of real device            */
int             outtrkbr = 0;           /* Output bytes remaining on
                                           track of real device      */
int             outtrk = 0;             /* Output relative track     */
int             outrec = 0;             /* Output record number      */
int             blkptrk;                /* Number of blocks per track*/
int             totblks;                /* Number of blocks in CVOL  */
int             bytes;                  /* Bytes used in this block  */
U32             ucbtype;                /* UCB device type           */
PDSDIR         *catent;                 /* -> Catalog entry          */
DATABLK         datablk;                /* Data block                */
#define NUM_SYS1_DATASETS       8       /* Number of SYS1 datasets   */
static char    *sys1name[NUM_SYS1_DATASETS] =
                {"DUMP", "IMAGELIB", "LINKLIB", "NUCLEUS",
                "PARMLIB", "PROCLIB", "SAMPLIB", "SYSJOBQE"};

    /* Set the key length and data length for SYSCTLG dataset */
    keylen = 8;
    datalen = 256;

    /* Obtain the number of blocks which will fit on a track */
    capacity_calc (cif, 0, keylen, datalen, NULL, NULL,
                    NULL, NULL, NULL, NULL, NULL,
                    NULL, NULL, &blkptrk, NULL, NULL);

    /* Calculate the total number of blocks in the catalog */
    totblks = extsize * blkptrk;

    /* Get the UCB device type */
    ucbtype = ucbtype_code (devtype);

    /*-----------------------------------*/
    /* Initialize the volume index block */
    /*-----------------------------------*/
    memset (datablk.kdarea, 0, keylen + datalen);

    /* The key field contains all X'FF' */
    memcpy( datablk.kdarea, &CKD_ENDTRK, CKD_ENDTRK_SIZE );

    /* The first entry begins after the 2 byte count field */
    bytes = 2;
    catent = (PDSDIR*)(datablk.kdarea + keylen + bytes);

    /* Build the volume index control entry (VICE) */

    /* The VICE name is X'0000000000000001' */
    memcpy (catent->pds2name, cvol_low_key, 8);

    /* Set TTR to highest block in volume index, i.e. X'000001' */
    catent->pds2ttrp[0] = 0;
    catent->pds2ttrp[1] = 0;
    catent->pds2ttrp[2] = 1;

    /* Indicator byte X'05' means 5 user halfwords follow, and
       uniquely identifies this catalog entry as a VICE */
    catent->pds2indc = 5;

    /* Set the TTR of the last block of the catalog */
    catent->pds2usrd[0] = ((extsize - 1) >> 8) & 0xFF;
    catent->pds2usrd[1] = (extsize - 1) & 0xFF;
    catent->pds2usrd[2] = (BYTE) blkptrk;
    catent->pds2usrd[3] = 0;

    /* Set the TTR of the first unused block (X'000003') */
    catent->pds2usrd[4] = 0;
    catent->pds2usrd[5] = 0;
    catent->pds2usrd[6] = 3;

    /* Remainder of user data is 0 */
    catent->pds2usrd[7] = 0;
    catent->pds2usrd[8] = 0;
    catent->pds2usrd[9] = 0;

    /* Increment bytes used by the length of the VICE */
    bytes += 22;
    catent = (PDSDIR*)(datablk.kdarea + keylen + bytes);

    /* Build the index pointer for SYS1 */
    convert_to_ebcdic (catent->pds2name, 8, "SYS1");

    /* Set TTR of the SYS1 index block, i.e. X'000002' */
    catent->pds2ttrp[0] = 0;
    catent->pds2ttrp[1] = 0;
    catent->pds2ttrp[2] = 2;

    /* Indicator byte X'00' means no user halfwords follow, and
       uniquely identifies this catalog entry as an index pointer */
    catent->pds2indc = 0;

    /* Increment bytes used by the length of the index pointer */
    bytes += 12;
    catent = (PDSDIR*)(datablk.kdarea + keylen + bytes);

    /* Set the last entry in block marker */
    memcpy( catent->pds2name, &CKD_ENDTRK, CKD_ENDTRK_SIZE );

    /* Increment bytes used by the last entry marker */
    bytes += 12;
    catent = (PDSDIR*)(datablk.kdarea + keylen + bytes);

    /* Set the number of bytes used in this block */
    store_hw( &datablk.kdarea[ keylen+0 ], bytes );

    /* Write the volume index block to the output file */
    rc = write_block (cif, ofname, &datablk, keylen, datalen,
                devtype, heads, trklen, extsize,
                &outusedv, &outusedr, &outtrkbr,
                &outtrk, &outcyl, &outhead, &outrec);
    if (rc < 0) return -1;

    // "Catalog block:  cyl[%04X/%d] head[%04X/%d] rec[%02X/%d]"
    XMINFF (4, MSG( HHC02565, "I", outcyl, outcyl, outhead, outhead, outrec, outrec ) );

    if (infolvl >= 5)
        data_dump (datablk.kdarea, keylen + datalen);

    /* Count number of blocks written */
    totblks--;

    /*---------------------------------*/
    /* Initialize the SYS1 index block */
    /*---------------------------------*/
    memset (datablk.kdarea, 0, keylen + datalen);

    /* The key field contains all X'FF' */
    memcpy( datablk.kdarea, &CKD_ENDTRK, CKD_ENDTRK_SIZE );

    /* The first entry begins after the 2 byte count field */
    bytes = 2;
    catent = (PDSDIR*)(datablk.kdarea + keylen + bytes);

    /* Build the index control entry (ICE) */

    /* The ICE name is X'0000000000000001' */
    memcpy (catent->pds2name, cvol_low_key, 8);

    /* Set TTR to highest block in this index, i.e. X'000002' */
    catent->pds2ttrp[0] = 0;
    catent->pds2ttrp[1] = 0;
    catent->pds2ttrp[2] = 2;

    /* Indicator byte X'03' means 3 user halfwords follow, and
       uniquely identifies this catalog entry as an ICE */
    catent->pds2indc = 3;

    /* Set the TTR of this block */
    catent->pds2usrd[0] = 0;
    catent->pds2usrd[1] = 0;
    catent->pds2usrd[2] = 2;

    /* The next byte contains the alias count */
    catent->pds2usrd[3] = 0;

    /* The remaining 2 bytes of userdata are zeroes */
    catent->pds2usrd[4] = 0;
    catent->pds2usrd[5] = 0;

    /* Increment bytes used by the length of the ICE */
    bytes += 18;

    /* Build the dataset pointers for SYS1.xxxxxxxx datasets */
    for (i = 0; i < NUM_SYS1_DATASETS; i++)
    {
        /* Point to next dataset pointer entry */
        catent = (PDSDIR*)(datablk.kdarea + keylen + bytes);

        /* Set the name of the dataset pointer entry */
        convert_to_ebcdic (catent->pds2name, 8, sys1name[i]);

        /* Set the TTR to zero */
        catent->pds2ttrp[0] = 0;
        catent->pds2ttrp[1] = 0;
        catent->pds2ttrp[2] = 0;

        /* Indicator byte X'07' means 7 user halfwords follow, and
           uniquely identifies the entry as a dataset pointer */
        catent->pds2indc = 7;

        /* The next two bytes contain the volume count (X'0001') */
        catent->pds2usrd[0] = 0;
        catent->pds2usrd[1] = 1;

        /* The next four bytes contain the UCB type */
        catent->pds2usrd[2] = (ucbtype >> 24) & 0xFF;
        catent->pds2usrd[3] = (ucbtype >> 16) & 0xFF;
        catent->pds2usrd[4] = (ucbtype >> 8) & 0xFF;
        catent->pds2usrd[5] = ucbtype & 0xFF;

        /* The next six bytes contain the volume serial number */
        convert_to_ebcdic (catent->pds2usrd+6, 6, volser);

        /* The next two bytes contain the volume seq.no. (X'0000') */
        catent->pds2usrd[12] = 0;
        catent->pds2usrd[13] = 0;

        /* Increment bytes used by the length of the dataset pointer */
        bytes += 26;

    } /* end for(i) */

    /* Point to last entry in block */
    catent = (PDSDIR*)(datablk.kdarea + keylen + bytes);

    /* Set the last entry in block marker */
    memcpy( catent->pds2name, &CKD_ENDTRK, CKD_ENDTRK_SIZE );

    /* Increment bytes used by the last entry marker */
    bytes += 12;
    catent = (PDSDIR*)(datablk.kdarea + keylen + bytes);

    /* Set the number of bytes used in this block */
    store_hw( &datablk.kdarea[ keylen+0 ], bytes );

    /* Write the index block to the output file */
    rc = write_block (cif, ofname, &datablk, keylen, datalen,
                devtype, heads, trklen, extsize,
                &outusedv, &outusedr, &outtrkbr,
                &outtrk, &outcyl, &outhead, &outrec);
    if (rc < 0) return -1;

    // "Catalog block:  cyl[%04X/%d] head[%04X/%d] rec[%02X/%d]"
    XMINFF (4, MSG( HHC02565, "I", outcyl, outcyl, outhead, outhead, outrec, outrec ) );

    if (infolvl >= 5) data_dump (datablk.kdarea, keylen + datalen);

    /* Count number of blocks written */
    totblks--;

    /*--------------------------------------------*/
    /* Initialize remaining unused catalog blocks */
    /*--------------------------------------------*/
    while (totblks > 0)
    {
        memset (datablk.kdarea, 0, keylen + datalen);

        /* Write the volume index block to the output file */
        rc = write_block (cif, ofname, &datablk, keylen, datalen,
                    devtype, heads, trklen, extsize,
                    &outusedv, &outusedr, &outtrkbr,
                    &outtrk, &outcyl, &outhead, &outrec);
        if (rc < 0) return -1;

        // "Catalog block:  cyl[%04X/%d] head[%04X/%d] rec[%02X/%d]"
        XMINFF (4, MSG( HHC02565, "I", outcyl, outcyl, outhead, outhead, outrec, outrec ) );

        if (infolvl >= 5) data_dump (datablk.kdarea, keylen + datalen);

        /* Count number of blocks written */
        totblks--;

    } /* end while(totblks) */

    /* Set the last record number to X'FF' so that OS/360 catalog
       management routines can recognize that the CVOL has been
       initialized by detecting X'FF' at ds1lstar+2 in the VTOC */
    *lastrec = 0xFF;

    /* Return the track balance */
    *trkbal = outtrkbr;

    /* Write data remaining in track buffer */
    rc = write_track (cif, ofname, heads, trklen,
                    &outusedv, &outtrk, &outcyl, &outhead);
    if (rc < 0) return -1;

    /* Return number of tracks and starting address of next dataset */
    *numtrks = outtrk;
    *nxtcyl = outcyl;
    *nxthead = outhead;
    return 0;

} /* end function cvol_initialize */

/*-------------------------------------------------------------------*/
/* Subroutine to initialize a LOGREC dataset with IFCDIP00 header    */
/* Input:                                                            */
/*      ofname  DASD image file name                                 */
/*      cif     -> CKD image file descriptor                         */
/*      devtype Output device type                                   */
/*      heads   Output device number of tracks per cylinder          */
/*      trklen  Output device virtual track length                   */
/*      outcyl  Output starting cylinder number                      */
/*      outhead Output starting head number                          */
/*      extsize Extent size in tracks                                */
/* Output:                                                           */
/*      lastrec Record number of last block written                  */
/*      trkbal  Number of bytes remaining on last track              */
/*      numtrks Number of tracks written                             */
/*      nxtcyl  Starting cylinder number for next dataset            */
/*      nxthead Starting head number for next dataset                */
/*-------------------------------------------------------------------*/
static int
dip_initialize (char *ofname, CIFBLK *cif,
                U16 devtype, int heads, int trklen,
                int outcyl, int outhead, int extsize,
                int *lastrec, int *trkbal,
                int *numtrks, int *nxtcyl, int *nxthead)
{
int             rc;                     /* Return code               */
int             keylen;                 /* Key length of data block  */
int             datalen;                /* Data length of data block */
int             outusedv = 0;           /* Output bytes used on track
                                           of virtual device         */
int             outusedr = 0;           /* Output bytes used on track
                                           of real device            */
int             outtrkbr = 0;           /* Output bytes remaining on
                                           track of real device      */
int             outtrk = 0;             /* Output relative track     */
int             outrec = 0;             /* Output record number      */
int             remlen;                 /* Bytes remaining on 1st trk*/
int             physlen;                /* Physical track length     */
int             lasthead;               /* Highest head on cylinder  */
int             endcyl;                 /* Extent end cylinder       */
int             endhead;                /* Extent end head           */
int             trklen90;               /* 90% of track length       */
int             cyl90;                  /* 90% full cylinder number  */
int             head90;                 /* 90% full head number      */
int             reltrk90;               /* 90% full relative track   */
DIPHDR         *diphdr;                 /* -> Record in data block   */
DATABLK         datablk;                /* Data block                */

    /* Set the key length and data length for the header record */
    keylen = 0;
    datalen = sizeof(DIPHDR);

    /* Obtain the physical track size and the track balance
       remaining on the first track after the header record */
    capacity_calc (cif, 0, keylen, datalen, NULL, &remlen,
                    &physlen, NULL, NULL, NULL, NULL, NULL,
                    NULL, NULL, NULL, NULL);

    /* Calculate the end of extent cylinder and head */
    lasthead = heads - 1;
    endcyl = outcyl;
    endhead = outhead + extsize - 1;
    while (endhead >= heads)
    {
        endhead -= heads;
        endcyl++;
    }

    /* Calculate the 90% full cylinder and head */
    trklen90 = physlen * 9 / 10;
    reltrk90 = extsize * trklen90 / physlen;
    if (reltrk90 == 0) reltrk90 = 1;
    cyl90 = outcyl;
    head90 = outhead + reltrk90 - 1;
    while (head90 >= heads)
    {
        head90 -= heads;
        cyl90++;
    }

    /* Initialize the DIP header record */
    diphdr = (DIPHDR*) (datablk.kdarea);
    memset( diphdr, 0, sizeof( DIPHDR ));

    store_hw( diphdr->recid, 0xFFFF );
    store_hw( diphdr->bcyl, outcyl  );
    store_hw( diphdr->btrk, outhead );
    store_hw( diphdr->ecyl, endcyl  );
    store_hw( diphdr->etrk, endhead );

    diphdr->restart[2] = (outcyl >> 8) & 0xFF;
    diphdr->restart[3] = outcyl & 0xFF;
    diphdr->restart[4] = (outhead >> 8) & 0xFF;
    diphdr->restart[5] = outhead & 0xFF;
    diphdr->restart[6] = 1;

    store_hw( diphdr->trkbal, remlen  );
    store_hw( diphdr->trklen, physlen );

    diphdr->reused[2] = (outcyl >> 8) & 0xFF;
    diphdr->reused[3] = outcyl & 0xFF;
    diphdr->reused[4] = (outhead >> 8) & 0xFF;
    diphdr->reused[5] = outhead & 0xFF;
    diphdr->reused[6] = 1;

    store_hw( diphdr->lasthead, lasthead );
    store_hw( diphdr->trklen90, trklen90 );

    diphdr->devcode = (ucbtype_code (devtype ) & 0x0F) | 0xF0;
    diphdr->cchh90[0] = (cyl90  >> 8) & 0xFF;
    diphdr->cchh90[1] =  cyl90        & 0xFF;
    diphdr->cchh90[2] = (head90 >> 8) & 0xFF;
    diphdr->cchh90[3] =  head90       & 0xFF;
    diphdr->endid = 0xFF;

    /* Write the data block to the output file */
    rc = write_block (cif, ofname, &datablk, keylen, datalen,
                devtype, heads, trklen, extsize,
                &outusedv, &outusedr, &outtrkbr,
                &outtrk, &outcyl, &outhead, &outrec);
    if (rc < 0) return -1;

    // "DIP complete:   cyl[%04X/%d] head[%04X/%d] rec[%02X/%d]"
    XMINFF (3, MSG( HHC02566, "I", outcyl, outcyl, outhead, outhead, outrec, outrec ) );

    if (infolvl >= 5) data_dump (diphdr, sizeof(DIPHDR));

    /* Return the last record number and track balance */
    *lastrec = outrec;
    *trkbal = outtrkbr;

    /* Write data remaining in track buffer */
    rc = write_track (cif, ofname, heads, trklen,
                    &outusedv, &outtrk, &outcyl, &outhead);
    if (rc < 0) return -1;

    /* Return number of tracks and starting address of next dataset */
    *numtrks = outtrk;
    *nxtcyl = outcyl;
    *nxthead = outhead;
    return 0;

} /* end function dip_initialize */

/*-------------------------------------------------------------------*/
/* Subroutine to initialize a sequential dataset                     */
/* Input:                                                            */
/*      sfname  SEQ input file name                                  */
/*      ofname  DASD image file name                                 */
/*      cif     -> CKD image file descriptor                         */
/*      devtype Output device type                                   */
/*      heads   Output device number of tracks per cylinder          */
/*      trklen  Output device virtual track length                   */
/*      outcyl  Output starting cylinder number                      */
/*      outhead Output starting head number                          */
/*      extsize Extent size in tracks                                */
/*      dsorg   Dataset organization  (DA or PS)                     */
/*      recfm   Record Format (F or FB)                              */
/*      lrecl   Record length                                        */
/*      blksz   Block size                                           */
/*      keyln   Key length                                           */
/* Output:                                                           */
/*      lastrec Record number of last block written                  */
/*      trkbal  Number of bytes remaining on last track              */
/*      numtrks Number of tracks written                             */
/*      nxtcyl  Starting cylinder number for next dataset            */
/*      nxthead Starting head number for next dataset                */
/*-------------------------------------------------------------------*/
static int
seq_initialize (char *sfname, char *ofname, CIFBLK *cif, U16 devtype,
                int heads, int trklen, int outcyl, int outhead,
                int extsize, BYTE dsorg, BYTE recfm,
                int lrecl, int blksz, int keyln,
                int *lastrec, int *trkbal,
                int *numtrks, int *nxtcyl, int *nxthead)
{
int             rc;                     /* Return code               */
int             sfd;                    /* Input seq file descriptor */
int             size;                   /* Size left in input file   */
int             outusedv = 0;           /* Output bytes used on track
                                           of virtual device         */
int             outusedr = 0;           /* Output bytes used on track
                                           of real device            */
int             outtrkbr = 0;           /* Output bytes remaining on
                                           track of real device      */
int             outtrk = 0;             /* Output relative track     */
int             outrec = 0;             /* Output record number      */
struct stat     st;                     /* Data area for fstat()     */
DATABLK         datablk;                /* Data block                */
char            pathname[MAX_PATH];     /* sfname in host path format*/

    /* Perform some checks */
    if (!(dsorg & DSORG_PS) && !(dsorg & DSORG_DA))
    {
        // "File %s: invalid dsorg[0x%02X] for SEQ processing; dsorg must be PS or DA"
        XMERRF ( MSG( HHC02573, "E", sfname, dsorg ) );
        return -1;
    }
    if (recfm != RECFM_FORMAT_F && recfm != (RECFM_FORMAT_F|RECFM_BLOCKED))
    {
        // "File %s: invalid recfm[0x%02X] for SEQ processing; recfm must be F or FB"
        XMERRF ( MSG( HHC02574, "E", sfname, recfm ) );
        return -1;
    }
    if (blksz == 0) blksz = lrecl;
    if (lrecl == 0) lrecl = blksz;
    if (lrecl == 0 || blksz % lrecl != 0
     || (blksz != lrecl && recfm == RECFM_FORMAT_F))
    {
        // "File %s: invalid lrecl[%d] or blksize[%d] for SEQ processing"
        XMERRF ( MSG( HHC02575, "E", sfname, lrecl, blksz ) );
        return -1;
    }
    if (keyln > 0 && blksz > lrecl)
    {
        // "File %s: invalid keyln[%d] for SEQ processing; keyln must be 0 for blocked"
        XMERR ( MSG( HHC02576, "E", sfname, keyln ) );
        return -1;
    }

    /* Open the input file */
    hostpath(pathname, sfname, sizeof(pathname));
    sfd = HOPEN (pathname, O_RDONLY|O_BINARY);
    if (sfd < 0)
    {
        // "File %s; %s error: %s"
        XMERRF ( MSG( HHC02468, "E", sfname, "open()", strerror(errno) ) );
        return -1;
    }

    /* Get input file status */
    rc = fstat(sfd, &st);
    if (rc < 0)
    {
        // "File %s; %s error: %s"
        XMERRF ( MSG( HHC02468, "E", sfname, "fstat()", strerror(errno) ) );
        close (sfd);
        return -1;
    }
    size = (int) st.st_size;

    /* Read the first track */
    rc = read_track( cif, *nxtcyl, (U8) *nxthead );
    if (rc < 0)
    {
        // "File %s: read error at cyl[%04X/%d] head[%04X/%d]"
        XMERRF ( MSG( HHC02547, "E", ofname, *nxtcyl, *nxtcyl, *nxthead, *nxthead ) );
        close (sfd);
        return -1;
    }

    while (size > 0)
    {
        /* Read a block of data from the input file */
        rc = read (sfd, &datablk.kdarea, blksz < size ? blksz : size);
        if (rc < (blksz < size ? blksz : size))
        {
            // "File %s; %s error: %s"
            XMERRF ( MSG( HHC02468, "E", sfname, "read()", strerror(errno) ) );
            close (sfd);
            return -1;
        }
        size -= rc;

        /* Pad the block if necessary */
        if (rc < blksz)
        {
            /* Adjust blksize down to next
               highest multiple of lrecl */
            blksz = (((rc-1) / lrecl) + 1) * lrecl;
            memset (&datablk.kdarea[rc], 0, blksz - rc);
        }

        rc = write_block (cif, ofname, &datablk, keyln, blksz - keyln,
                        devtype, heads, trklen, extsize,
                        &outusedv, &outusedr, &outtrkbr,
                        &outtrk, &outcyl, &outhead, &outrec);
        if (rc < 0)
        {
            close (sfd);
            return -1;
        }
    }

    /* Close the input file */
    close (sfd);

    /* Create the end of file record */
    rc = write_block (cif, ofname, &datablk, 0, 0,
                devtype, heads, trklen, extsize,
                &outusedv, &outusedr, &outtrkbr,
                &outtrk, &outcyl, &outhead, &outrec);
    if (rc < 0) return -1;

    /* Return the last record number and track balance */
    *lastrec = outrec;
    *trkbal = outtrkbr;

    /* Write data remaining in track buffer */
    rc = write_track (cif, ofname, heads, trklen,
                    &outusedv, &outtrk, &outcyl, &outhead);
    if (rc < 0) return -1;

    /* Return number of tracks and starting address of next dataset */
    *numtrks = outtrk;
    *nxtcyl = outcyl;
    *nxthead = outhead;
    return 0;

} /* end function seq_initialize */


/*-------------------------------------------------------------------*/
/* Subroutine to initialize an empty dataset                         */
/* Input:                                                            */
/*      ofname  DASD image file name                                 */
/*      cif     -> CKD image file descriptor                         */
/*      devtype Output device type                                   */
/*      heads   Output device number of tracks per cylinder          */
/*      trklen  Output device virtual track length                   */
/*      outcyl  Output starting cylinder number                      */
/*      outhead Output starting head number                          */
/*      extsize Extent size in tracks                                */
/*      dsorg   Dataset organization                                 */
/*      dirblks Number of directory blocks                           */
/* Output:                                                           */
/*      dirblu  Bytes used in last directory block                   */
/*      lastrec Record number of last block written                  */
/*      trkbal  Number of bytes remaining on last track              */
/*      numtrks Number of tracks written                             */
/*      nxtcyl  Starting cylinder number for next dataset            */
/*      nxthead Starting head number for next dataset                */
/*-------------------------------------------------------------------*/
static int
empty_initialize (char *ofname, CIFBLK *cif, U16 devtype,
                int heads, int trklen, int outcyl, int outhead,
                int extsize, BYTE dsorg, int dirblks,
                int *dirblu, int *lastrec, int *trkbal,
                int *numtrks, int *nxtcyl, int *nxthead)
{
int             rc;                     /* Return code               */
int             i;                      /* Loop counter              */
int             keylen;                 /* Key length of data block  */
int             datalen;                /* Data length of data block */
int             outusedv = 0;           /* Output bytes used on track
                                           of virtual device         */
int             outusedr = 0;           /* Output bytes used on track
                                           of real device            */
int             outtrkbr = 0;           /* Output bytes remaining on
                                           track of real device      */
int             outdblu = 0;            /* Output bytes used in last
                                           directory block           */
int             outtrk = 0;             /* Output relative track     */
int             outrec = 0;             /* Output record number      */
DATABLK         datablk;                /* Data block                */

    /* Initialize the directory if dataset is a PDS */
    if (dsorg & DSORG_PO)
    {
        /* Build the first directory block */
        keylen = 8;
        datalen = 256;
        outdblu = 14;
        memset( datablk.kdarea, 0, keylen + datalen);
        memcpy( datablk.kdarea, &CKD_ENDTRK, CKD_ENDTRK_SIZE );
        store_hw( &datablk.kdarea[ keylen+0 ], outdblu );
        memcpy( datablk.kdarea + keylen + 2, &CKD_ENDTRK, CKD_ENDTRK_SIZE );

        /* Write directory blocks to output dataset */
        for (i = 0; i < dirblks; i++)
        {
            /* Write a directory block */
            rc = write_block (cif, ofname, &datablk, keylen, datalen,
                        devtype, heads, trklen, extsize,
                        &outusedv, &outusedr, &outtrkbr,
                        &outtrk, &outcyl, &outhead, &outrec);
            if (rc < 0) return -1;

            /* Clear subsequent directory blocks to zero */
            memset (datablk.kdarea, 0, keylen + datalen);

        } /* end for(i) */

    } /* end if(DSORG_PO) */

    /* Create the end of file record */
    keylen = 0;
    datalen = 0;
    rc = write_block (cif, ofname, &datablk, keylen, datalen,
                devtype, heads, trklen, extsize,
                &outusedv, &outusedr, &outtrkbr,
                &outtrk, &outcyl, &outhead, &outrec);
    if (rc < 0) return -1;

    /* Return number of bytes used in last directory block */
    *dirblu = outdblu;

    /* Return the last record number and track balance */
    *lastrec = outrec;
    *trkbal = outtrkbr;

    /* Write data remaining in track buffer */
    rc = write_track (cif, ofname, heads, trklen,
                    &outusedv, &outtrk, &outcyl, &outhead);
    if (rc < 0) return -1;

    /* Return number of tracks and starting address of next dataset */
    *numtrks = outtrk;
    *nxtcyl = outcyl;
    *nxthead = outhead;
    return 0;

} /* end function empty_initialize */

/*-------------------------------------------------------------------*/
/* Subroutine to read a statement from the control file              */
/* Input:                                                            */
/*      cfp     Control file pointer                                 */
/*      cfname  Control file name                                    */
/*      stmt    Buffer to receive control statement                  */
/*      sbuflen Length of statement buffer                           */
/* Output:                                                           */
/*      pstmtno Statement number                                     */
/*      The return value is 0 if a statement was successfully read,  */
/*      +1 if end of file, or -1 if error                            */
/*-------------------------------------------------------------------*/
static int
read_ctrl_stmt (FILE *cfp, char *cfname, char *stmt, int sbuflen,
                int *pstmtno)
{
    int         stmtlen;                /* Length of input statement */
    static int  stmtno = 0;             /* Statement number          */

    while (1)
    {
        /* Read next record from control file */
        stmtno++;
        *pstmtno = stmtno;

        if (!fgets( stmt, sbuflen, cfp ))
        {
            /* Return +1 when end of control file is reached.
               Else return -1 indicating input file error.
            */
            if (feof( cfp ))
                return +1;

            // (error)
            {
                char msgbuf[64];
                MSGBUF( msgbuf, "line[%d] fgets()", stmtno );

                // "File %s; %s error: %s"
                XMERRF( MSG( HHC02468, "E", cfname, msgbuf, strerror( errno )));
            }
            return -1;
        }

        /* Indicate input file progess */
        EXTGUIMSG( "IPOS=%"PRId64"\n", (U64) ftell( cfp ));

        /* Check for DOS end of file character */
        if (stmt[0] == '\x1A')
            return +1;

        /* Check that end of statement has been read */
        stmtlen = (int) strlen( stmt );
        if (!stmtlen || stmt[ stmtlen-1 ] != '\n')
        {
            // "File %s: line[%04d] error: line length is invalid"
            XMERRF( MSG( HHC02577, "E", cfname, stmtno ));
            return -1;
        }

        /* Remove trailing whitespace */
        RTRIM( stmt );
        stmtlen = (int) strlen( stmt );

        /* Print the input statement */

        // "File %s[%04d]: %s"
        XMINFF( 0, MSG( HHC02567, "I", cfname, stmtno, stmt ));

        /* Ignore comment statements */
        if (!stmtlen || stmt[0] == '#' || stmt[0] == '*')
            continue;

        break;
    } /* end while */

    return 0;
} /* end function read_ctrl_stmt */

/*-------------------------------------------------------------------*/
/* Subroutine to parse a dataset statement from the control file     */
/* Input:                                                            */
/*      stmt    Control statement                                    */
/* Output:                                                           */
/*      dsname  ASCIIZ dataset name (1-44 bytes + terminator)        */
/*      method  Processing method (see METHOD_xxx defines)           */
/*                                                                   */
/*      The following field is returned only for the XMIT method:    */
/*      ifptr   Pointer to XMIT initialization file name             */
/*                                                                   */
/*      The following fields are returned for non-XMIT methods:      */
/*      units   Allocation units (C=CYL, T=TRK)                      */
/*      sppri   Primary allocation quantity                          */
/*      spsec   Secondary allocation quantity                        */
/*      spdir   Directory allocation quantity                        */
/*      dsorg   1st byte of dataset organization bits                */
/*      recfm   1st byte of record format bits                       */
/*      lrecl   Logical record length                                */
/*      blksz   Block size                                           */
/*      keyln   Key length                                           */
/*      The return value is 0 if successful, or -1 if error.         */
/* Control statement format:                                         */
/*      dsname method [initfile] [space [dcbattrib]]                 */
/*      The method can be:                                           */
/*      XMIT = load PDS from initfile containing an IEBCOPY unload   */
/*             dataset created using the TSO TRANSMIT command        */
/*      EMPTY = create empty dataset (do not specify initfile)       */
/*      DIP = initialize LOGREC dataset with IFCDIP00 header record  */
/*      CVOL = initialize SYSCTLG dataset as an OS CVOL              */
/*      VTOC = reserve space for the VTOC (dsname is ignored)        */
/*      The space allocation can be:                                 */
/*      CYL [pri [sec [dir]]]                                        */
/*      TRK [pri [sec [dir]]]                                        */
/*      If primary quantity is omitted then the dataset will be      */
/*      allocated the minimum number of tracks or cylinders needed   */
/*      to contain the data loaded from the initfile.                */
/*      Default allocation is in tracks.                             */
/*      The dcb attributes can be:                                   */
/*      dsorg recfm lrecl blksize keylen                             */
/*      For the XMIT method the dcb attributes are taken from the    */
/*      initialization file and need not be specified.               */
/*      Examples:                                                    */
/*      SYS1.PARMLIB XMIT /cdrom/os360/reslibs/parmlib.xmi           */
/*      SYS1.NUCLEUS XMIT /cdrom/os360/reslibs/nucleus.xmi CYL       */
/*      SYS1.SYSJOBQE EMPTY CYL 10 0 0 DA F 176 176 0                */
/*      SYS1.DUMP EMPTY CYL 10 2 0 PS FB 4104 4104 0                 */
/*      SYS1.OBJPDS EMPTY CYL 10 2 50 PO FB 80 3120 0                */
/*      SYSVTOC VTOC CYL 1                                           */
/*      SYSCTLG CVOL TRK 10                                          */
/*      SYS1.LOGREC DIP CYL 1                                        */
/*-------------------------------------------------------------------*/
static int
parse_ctrl_stmt (char *stmt, char *dsname, BYTE *method, char **ifptr,
                BYTE *units, int *sppri, int *spsec, int *spdir,
                BYTE *dsorg, BYTE *recfm,
                int *lrecl, int *blksz, int *keyln)
{
char           *pdsnam;                 /* -> dsname in input stmt   */
char           *punits;                 /* -> allocation units       */
char           *psppri;                 /* -> primary space quantity */
char           *pspsec;                 /* -> secondary space qty.   */
char           *pspdir;                 /* -> directory space qty.   */
char           *pdsorg;                 /* -> dataset organization   */
char           *precfm;                 /* -> record format          */
char           *plrecl;                 /* -> logical record length  */
char           *pblksz;                 /* -> block size             */
char           *pkeyln;                 /* -> key length             */
char           *pimeth;                 /* -> initialization method  */
char           *pifile;                 /* -> initialization filename*/
BYTE            c;                      /* Character work area       */
char           *strtok_str = NULL;      /* last token                */

    /* Parse the input statement */
    pdsnam = strtok_r (stmt, " \t", &strtok_str);
    pimeth = strtok_r (NULL, " \t", &strtok_str);

    /* Check that all mandatory fields are present */
    if (pdsnam == NULL || pimeth == NULL)
    {
        if (pdsnam == NULL)
            // "%s missing"
            XMERR ( MSG(HHC02578, "E", "DSNAME" ) );
        if (pimeth == NULL)
            // "%s missing"
            XMERR ( MSG(HHC02578, "E", "Initialization method" ) );
        return -1;
    }

    /* Return the dataset name in EBCDIC and ASCII */
    string_to_upper (pdsnam);
    memset (dsname, 0, 45);
    strncpy (dsname, pdsnam, 44);

    /* Set default dataset attribute values */
    *units = 'T';
    *sppri = 1;
    *spsec = 0;
    *spdir = 0;
    *dsorg = 0x00;
    *recfm = 0x00;
    *lrecl = 0;
    *blksz = 0;
    *keyln = 0;
    *ifptr = NULL;

    /* Test for valid initialization method */
    if (strcasecmp(pimeth, "XMIT") == 0)
        *method = METHOD_XMIT;
    else if (strcasecmp(pimeth, "VS") == 0)
        *method = METHOD_VS;
    else if (strcasecmp(pimeth, "EMPTY") == 0)
        *method = METHOD_EMPTY;
    else if (strcasecmp(pimeth, "DIP") == 0)
        *method = METHOD_DIP;
    else if (strcasecmp(pimeth, "CVOL") == 0)
        *method = METHOD_CVOL;
    else if (strcasecmp(pimeth, "VTOC") == 0)
        *method = METHOD_VTOC;
    else if (strcasecmp(pimeth, "SEQ") == 0)
        *method = METHOD_SEQ;
    else
    {
        // "Initialization method %s is invalid"
        XMERRF ( MSG( HHC02579, "E", pimeth ) );
        return -1;
    }

    /* Locate the initialization file name */
    if (*method == METHOD_XMIT || *method == METHOD_VS || *method == METHOD_SEQ)
    {
        pifile = strtok_r (NULL, " \t", &strtok_str);
        if (pifile == NULL)
        {
            // "Initialization file name missing"
            XMERR ( MSG( HHC02580, "E" ) );
            return -1;
        }
        *ifptr = pifile;
    }

    /* Determine the space allocation units */
    punits = strtok_r (NULL, " \t", &strtok_str );
    if (punits == NULL) return 0;

    string_to_upper (punits);
    if (strcmp(punits, "CYL") == 0)
        *units = 'C';
    else if (strcmp(punits, "TRK") == 0)
        *units = 'T';
    else
    {
        // "Unit of allocation %s is invalid"
        XMERRF ( MSG( HHC02581, "E", punits ) );
        return -1;
    }

    /* Determine the primary space allocation quantity */
    psppri = strtok_r (NULL, " \t", &strtok_str);
    if (psppri == NULL) return 0;

    if (sscanf(psppri, "%u%c", sppri, &c) != 1)
    {
        // "%s allocation of %s %sS is invalid"
        XMERRF ( MSG( HHC02582, "E", "Primary", psppri, punits ) );
        return -1;
    }

    /* Determine the secondary space allocation quantity */
    pspsec = strtok_r (NULL, " \t", &strtok_str);
    if (pspsec == NULL) return 0;

    if (sscanf(pspsec, "%u%c", spsec, &c) != 1)
    {
        // "%s allocation of %s %sS is invalid"
        XMERRF ( MSG( HHC02582, "E", "Secondary", pspsec, punits ) );
        return -1;
    }

    /* Determine the directory space allocation quantity */
    pspdir = strtok_r (NULL, " \t", &strtok_str);
    if (pspdir == NULL) return 0;

    if (sscanf(pspdir, "%u%c", spdir, &c) != 1)
    {
        // "Directory space %s is invalid"
        XMERRF ( MSG( HHC02583, "E", pspdir ) );
        return -1;
    }

    /* Determine the dataset organization */
    pdsorg = strtok_r (NULL, " \t", &strtok_str);
    if (pdsorg == NULL) return 0;

    string_to_upper (pdsorg);
    if (strcmp(pdsorg, "IS") == 0)
        *dsorg = DSORG_IS;
    else if (strcmp(pdsorg, "PS") == 0)
        *dsorg = DSORG_PS;
    else if (strcmp(pdsorg, "DA") == 0)
        *dsorg = DSORG_DA;
    else if (strcmp(pdsorg, "PO") == 0)
        *dsorg = DSORG_PO;
    else
    {
        // "Dataset organization (DSORG) %s is invalid"
        XMERRF ( MSG( HHC02584, "E", pdsorg ) );
        return -1;
    }

    /* Determine the record format */
    precfm = strtok_r (NULL, " \t", &strtok_str);
    if (precfm == NULL) return 0;

    string_to_upper (precfm);
    if (strcmp(precfm, "F") == 0)
        *recfm = RECFM_FORMAT_F;
    else if (strcmp(precfm, "FB") == 0)
        *recfm = RECFM_FORMAT_F | RECFM_BLOCKED;
    else if (strcmp(precfm, "FBS") == 0)
        *recfm = RECFM_FORMAT_F | RECFM_BLOCKED | RECFM_SPANNED;
    else if (strcmp(precfm, "V") == 0)
        *recfm = RECFM_FORMAT_V;
    else if (strcmp(precfm, "VB") == 0)
        *recfm = RECFM_FORMAT_V | RECFM_BLOCKED;
    else if (strcmp(precfm, "VBS") == 0)
        *recfm = RECFM_FORMAT_V | RECFM_BLOCKED | RECFM_SPANNED;
    else if (strcmp(precfm, "U") == 0)
        *recfm = RECFM_FORMAT_U;
    else
    {
        // "Record format (RECFM) %s is invalid"
        XMERRF ( MSG( HHC02585, "E", precfm ) );
        return -1;
    }

    /* Determine the logical record length */
    plrecl = strtok_r (NULL, " \t", &strtok_str);
    if (plrecl == NULL) return 0;

    if (sscanf(plrecl, "%u%c", lrecl, &c) != 1
        || *lrecl > MAX_DATALEN)
    {
        // "Logical record length (LRECL) %s is invalid"
        XMERRF ( MSG( HHC02586, "E", plrecl ) );
        return -1;
    }

    /* Determine the block size */
    pblksz = strtok_r (NULL, " \t", &strtok_str);
    if (pblksz == NULL) return 0;

    if (sscanf(pblksz, "%u%c", blksz, &c) != 1
        || *blksz > MAX_DATALEN)
    {
        // "Block size (BLKSIZE) %s is invalid"
        XMERRF ( MSG( HHC02587, "E", pblksz ) );
        return -1;
    }

    /* Determine the key length */
    pkeyln = strtok_r (NULL, " \t", &strtok_str);
    if (pkeyln == NULL) return 0;

    if (sscanf(pkeyln, "%u%c", keyln, &c) != 1
        || *keyln > 255)
    {
        // "Key Length (KEYLEN) %s is invalid"
        XMERRF ( MSG( HHC02588, "E", pkeyln ) );
        return -1;
    }

    return 0;
} /* end function parse_ctrl_stmt */

/*-------------------------------------------------------------------*/
/* Subroutine to process the control file                            */
/* Input:                                                            */
/*      cfp     Control file pointer                                 */
/*      cfname  Control file name                                    */
/*      ofname  DASD image file name                                 */
/*      cif     -> CKD image file descriptor                         */
/*      volser  Output volume serial number (ASCIIZ)                 */
/*      devtype Output device type                                   */
/*      reqcyls Requested device size in cylinders, or zero          */
/*      heads   Output device number of tracks per cylinder          */
/*      trklen  Output device virtual track length                   */
/*      outcyl  Output starting cylinder number                      */
/*      outhead Output starting head number                          */
/* Output:                                                           */
/*      Datasets are written to the DASD image file as indicated     */
/*      by the control statements.                                   */
/*-------------------------------------------------------------------*/
static int
process_control_file (FILE *cfp, char *cfname, char *ofname,
                CIFBLK *cif, char *volser, U16 devtype, int reqcyls,
                int heads, int trklen, int outcyl, int outhead)
{
int             rc;                     /* Return code               */
int             i;                      /* Array subscript           */
int             n;                      /* Integer work area         */
char            dsname[45];             /* Dataset name (ASCIIZ)     */
BYTE            method;                 /* Initialization method     */
char           *ifname;                 /* ->Initialization file name*/
BYTE            units;                  /* C=CYL, T=TRK              */
int             sppri;                  /* Primary space quantity    */
int             spsec;                  /* Secondary space quantity  */
int             spdir;                  /* Directory space quantity  */
BYTE            dsorg;                  /* Dataset organization      */
BYTE            recfm;                  /* Record format             */
int             lrecl;                  /* Logical record length     */
int             blksz;                  /* Block size                */
int             keyln;                  /* Key length                */
char            stmt[256];              /* Control file statement    */
int             stmtno;                 /* Statement number          */
int             mintrks;                /* Minimum size of dataset   */
int             maxtrks;                /* Maximum size of dataset   */
int             outusedv;               /* Bytes used in track buffer*/
int             tracks = 0;             /* Tracks used in dataset    */
int             numdscb = 0;            /* Number of DSCBs           */
DATABLK       **dscbtab;                /* -> Array of DSCB pointers */
int             dirblu;                 /* Bytes used in last dirblk */
int             lasttrk;                /* Relative track number of
                                           last used track of dataset*/
int             lastrec;                /* Record number of last used
                                           block of dataset          */
int             trkbal;                 /* Bytes unused on last track*/
int             bcyl;                   /* Dataset begin cylinder    */
int             bhead;                  /* Dataset begin head        */
int             ecyl;                   /* Dataset end cylinder      */
int             ehead;                  /* Dataset end head          */
int             vtoctrk = 0;            /* VTOC start relative track */
int             vtocext = 0;            /* VTOC extent size (tracks) */
BYTE            volvtoc[5];             /* VTOC begin CCHHR          */
int             offset = 0;             /* Offset into trkbuf        */
int             fsflag = 0;             /* 1=Free space message sent */

    /* Obtain storage for the array of DSCB pointers */
    dscbtab = (DATABLK**)malloc (sizeof(DATABLK*) * nMaxDSCB);
    if (dscbtab == NULL)
    {
        // "Storage allocation for %s using %s failed: %s"
        XMERRF ( MSG( HHC02544, "E", "DSCB pointer array", "malloc()", strerror(errno) ) );
        return -1;
    }

    /* Initialize the DSCB array with format 4 and format 5 DSCBs */
    rc = build_format4_dscb (dscbtab, numdscb, cif);
    if (rc < 0) return -1;
    numdscb++;

    rc = build_format5_dscb (dscbtab, numdscb);
    if (rc < 0) return -1;
    numdscb++;

    /* Read dataset statements from control file */
    while (1)
    {
        /* Read next statement from control file */
        rc = read_ctrl_stmt (cfp, cfname, stmt, sizeof(stmt), &stmtno);
        if (rc < 0) return -1;

        /* Exit if end of file */
        if (rc > 0)
            break;

        /* Parse dataset statement from control file */
        rc = parse_ctrl_stmt (stmt, dsname, &method, &ifname,
                &units, &sppri, &spsec, &spdir,
                &dsorg, &recfm, &lrecl, &blksz, &keyln);

        /* Exit if error in control file */
        if (rc < 0)
        {
            // "File %s: line[%04d] error: statement is invalid"
            XMERRF ( MSG( HHC02569, "E", cfname, stmtno ) );
            return -1;
        }

        /* Write empty tracks if allocation is in cylinders */
        while (units == 'C' && outhead != 0)
        {
            /* Initialize track buffer with empty track */
            init_track (trklen, cif->trkbuf, outcyl, outhead, &outusedv);

            /* Write track to output file */
            rc = write_track (cif, ofname, heads, trklen,
                            &outusedv, &tracks, &outcyl, &outhead);
            if (rc < 0) break;

        } /* end while */

        // "Creating dataset %-44s at cyl[%04X/%d] head[%04X/%d]"
        XMINFF (1, MSG( HHC02568, "I", dsname, outcyl, outcyl, outhead, outhead ) );
        bcyl = outcyl;
        bhead = outhead;

        /* Calculate minimum size of dataset in tracks */
        mintrks = (units == 'C' ? sppri * heads : sppri);

        /* Create dataset according to method specified */
        switch (method) {

        case METHOD_XMIT:               /* IEBCOPY wrapped in XMIT */
        case METHOD_VS:                 /* "straight" IEBCOPY */
            /* Create dataset using IEBCOPY file as input */
            maxtrks = MAX_TRACKS;
            rc = process_iebcopy_file (ifname, ofname, cif,
                                    devtype, heads, trklen,
                                    outcyl, outhead, maxtrks,
                                    method,
                                    &dsorg, &recfm,
                                    &lrecl, &blksz, &keyln,
                                    &dirblu, &lastrec, &trkbal,
                                    &tracks, &outcyl, &outhead);
            if (rc < 0) return -1;
            break;

        case METHOD_DIP:
            /* Initialize LOGREC dataset */
            rc = dip_initialize (ofname, cif,
                                    devtype, heads, trklen,
                                    outcyl, outhead, mintrks,
                                    &lastrec, &trkbal,
                                    &tracks, &outcyl, &outhead);
            if (rc < 0) return -1;
            break;

        case METHOD_CVOL:
            /* Initialize SYSCTLG dataset */
            rc = cvol_initialize (ofname, cif, volser,
                                    devtype, heads, trklen,
                                    outcyl, outhead, mintrks,
                                    &lastrec, &trkbal,
                                    &tracks, &outcyl, &outhead);
            if (rc < 0) return -1;
            break;

        case METHOD_VTOC:
            /* Reserve space for VTOC */
            vtoctrk = (outcyl * heads) + outhead;
            vtocext = mintrks;
            tracks = 0;
            lastrec = 0;
            trkbal = 0;
            break;

        case METHOD_SEQ:
            /* Create sequential dataset */
            rc = seq_initialize (ifname, ofname, cif,
                                    devtype, heads, trklen,
                                    outcyl, outhead, mintrks,
                                    dsorg, recfm, lrecl, blksz,
                                    keyln, &lastrec, &trkbal,
                                    &tracks, &outcyl, &outhead);
            if (rc < 0) return -1;
            break;

        default:
        case METHOD_EMPTY:
            /* Create empty dataset */
            rc = empty_initialize (ofname, cif,
                                    devtype, heads, trklen,
                                    outcyl, outhead, mintrks,
                                    dsorg, spdir,
                                    &dirblu, &lastrec, &trkbal,
                                    &tracks, &outcyl, &outhead);
            if (rc < 0) return -1;
            break;

        } /* end switch(method) */

        /* Calculate the relative track number of last used track */
        lasttrk = tracks - 1;

        /* Round up space allocation if allocated in cylinders */
        if (units == 'C')
        {
            n = (tracks + heads - 1) / heads * heads;
            if (mintrks < n) mintrks = n;
        }

        /* Fill unused space in dataset with empty tracks */
        while (tracks < mintrks)
        {
            /* Initialize track buffer with empty track */
            init_track (trklen, cif->trkbuf, outcyl, outhead, &outusedv);

            /* Write track to output file */
            rc = write_track (cif, ofname, heads, trklen,
                            &outusedv, &tracks, &outcyl, &outhead);
            if (rc < 0) return -1;

        } /* end while(tracks) */

        /* Print number of tracks written to dataset */
        XMINFF (2, MSG( HHC02589, "I", dsname, tracks, (tracks == 1 ? "" : "s") ) );

        /* Calculate end of extent cylinder and head */
        ecyl = (outhead > 0 ? outcyl : outcyl - 1);
        ehead = (outhead > 0 ? outhead - 1 : heads - 1);

        /* Create format 1 DSCB for the dataset */
        if (method != METHOD_VTOC)
        {
            rc = build_format1_dscb (dscbtab, numdscb, dsname, volser,
                                    dsorg, recfm, lrecl, blksz,
                                    keyln, dirblu, lasttrk, lastrec,
                                    trkbal, units, spsec,
                                    bcyl, bhead, ecyl, ehead);
            if (rc < 0) return -1;
            numdscb++;
        }

    } /* end while */

    /* Write the VTOC */
    rc = write_vtoc (dscbtab, numdscb, cif, ofname, devtype,
                    reqcyls, heads, trklen, vtoctrk, vtocext,
                    &outcyl, &outhead, volvtoc);
    if (rc < 0) return -1;

    /* Write empty tracks up to end of volume */
    while (outhead != 0 || outcyl < reqcyls)
    {
        /* Issue free space information message */
        if (fsflag == 0)
        {
            // "Creating dataset %-44s at cyl[%04X/%d] head[%04X/%d]"
            XMINFF (1, MSG( HHC02568, "I", "****FREE SPACE****", outcyl, outcyl, outhead, outhead ) );
            fsflag = 1;
        }

        /* Indicate output file progess */
        if ((outcyl % 10) == 0)
        {
            EXTGUIMSG( "OUTCYL=%d\n", outcyl );
        }

        /* Initialize track buffer with empty track */
        init_track (trklen, cif->trkbuf, outcyl, outhead, &outusedv);

        /* Write track to output file */
        rc = write_track (cif, ofname, heads, trklen,
                        &outusedv, &tracks, &outcyl, &outhead);
        if (rc < 0) return -1;

    } /* end while */

    if (outcyl > reqcyls && reqcyls != 0)
    {
        // "Volume exceeds %d cylinders"
        XMINFF (0, MSG( HHC02590, "W", reqcyls ) );
    }

    // "File %s: total cylinders written was %d"
    XMINFF (0, MSG( HHC02591, "I", ofname, outcyl ) );

    /* Update the VTOC pointer in the volume label */
    offset = CKD_TRKHDR_SIZE + CKD_R0_SIZE + CKD_R0_DLEN
           + CKD_RECHDR_SIZE + IPL1_KEYLEN + IPL1_DATALEN
           + CKD_RECHDR_SIZE + IPL2_KEYLEN + IPL2_DATALEN
           + CKD_RECHDR_SIZE + VOL1_KEYLEN + offsetof( VOL1_CKD, vtoc_CC );

    // "VTOC pointer [%02X%02X%02X%02X%02X] updated"
    XMINFF (5, MSG( HHC02592, "I", volvtoc[0], volvtoc[1], volvtoc[2], volvtoc[3], volvtoc[4] ) );

    rc = read_track (cif, 0, 0);
    if (rc < 0)
    {
        // "VOL1 record not readable or locatable"
        XMERR ( MSG( HHC02593, "E" ) );
        return -1;
    }

    memcpy (cif->trkbuf + offset, volvtoc, sizeof(volvtoc));
    cif->trkmodif = 1;

    /* Release the DSCB buffers */
    for (i = 0; i < numdscb; i++)
        free (dscbtab[i]);

    /* Release the array of DSCB pointers */
    free (dscbtab);

    return 0;

} /* end function process_control_file */

/*-------------------------------------------------------------------*/
/* DASDLOAD main entry point                                         */
/*-------------------------------------------------------------------*/
int main (int argc, char *argv[])
{
char           *pgm;                    /* less any extension (.ext) */
int             rc = 0;                 /* Return code               */
char           *cfname;                 /* -> Control file name      */
char           *ofname;                 /* -> Output file name       */
FILE           *cfp;                    /* Control file pointer      */
CIFBLK         *cif;                    /* -> CKD image block        */
CKDDEV         *ckd;                    /* -> CKD table entry        */
char           *volser;                 /* -> Volume serial (ASCIIZ) */
char           *sdevtp;                 /* -> Device type (ASCIIZ)   */
char           *sdevsz;                 /* -> Device size (ASCIIZ)   */
char           *iplfnm;                 /* -> IPL text file or NULL  */
BYTE            c;                      /* Character work area       */
U16             devtype;                /* Output device type        */
int             devcyls;                /* Default device size (cyls)*/
int             reqcyls;                /* Requested device size (cyls)
                                           or 0 = use minimum size   */
int             outheads;               /* Output device trks/cyl    */
int             outmaxdl;               /* Output device maximum size
                                           record data length value  */
int             outtrklv;               /* Output device track length
                                           of virtual device         */
int             reltrk;                 /* Output track number       */
int             outcyl;                 /* Output cylinder number    */
int             outhead;                /* Output head number        */
char            stmt[256];              /* Control file statement    */
int             stmtno;                 /* Statement number          */
BYTE            comp = 0xff;            /* Compression algoritm      */
int             altcylflag = 0;         /* Alternate cylinders flag  */
int             flagECmode = 1;         /* IPL PSW mode flag         */
int             flagMachinecheck = 0;   /* IPL PSW machine check flag*/
int             lfs = 0;                /* 1 = Large file            */
char            pathname[MAX_PATH];     /* cfname in host path format*/
char           *strtok_str = NULL;      /* last token position       */

    INITIALIZE_UTILITY( UTILITY_NAME, UTILITY_DESC, &pgm );

    /* Process optional arguments */
    for ( ; argc > 1 && argv[1][0] == '-'; argv++, argc--)
    {
        if (strcmp("0", &argv[1][1]) == 0)
            comp = CCKD_COMPRESS_NONE;
#ifdef CCKD_COMPRESS_ZLIB
        else if (strcmp("z", &argv[1][1]) == 0)
            comp = CCKD_COMPRESS_ZLIB;
#endif
#ifdef CCKD_COMPRESS_BZIP2
        else if (strcmp("bz2", &argv[1][1]) == 0)
            comp = CCKD_COMPRESS_BZIP2;
#endif
        else if (strcmp("a", &argv[1][1]) == 0)
            altcylflag = 1;
        else if (strcmp("lfs", &argv[1][1]) == 0 && sizeof(off_t) > 4)
            lfs = 1;
        else if (strcmp("b", &argv[1][1]) == 0)
            flagECmode = 0;
        else if (strcmp("m", &argv[1][1]) == 0)
            flagMachinecheck = 1;
        else argexit(0, pgm);
    }

    /* Check the number of arguments */
    if (argc < 3 || argc > 7)
        argexit(4, pgm);

    /* The first argument is the control file name */
    cfname = argv[1];
    if (argv[1] == NULL || strlen(argv[1]) == 0)
        argexit(1, pgm);

    /* The second argument is the DASD image file name */
    ofname = argv[2];
    if (argv[2] == NULL || strlen(argv[2]) == 0)
        argexit(2, pgm);

    /* The optional third argument is the message level */
    if (argc > 3 && argv[3] != NULL)
    {
        if (sscanf(argv[3], "%u%c", &infolvl, &c) != 1 || infolvl > 5)
            argexit(3, pgm);

        /* The optional forth argument is the maximum number of DBLK entries */
        if (argc > 4 && argv[4] != NULL)
        {
            if (sscanf( argv[4], "%u%c", &nMaxDBLK, &c) != 1 || nMaxDBLK < 0)
                argexit(5, pgm);

            /* The optional fifth argument is the maximum number of TTR entries */
            if (argc > 5 && argv[5] != NULL)
            {
                if (sscanf( argv[5], "%u%c", &nMaxTTR, &c) != 1 || nMaxTTR < 0)
                    argexit(5, pgm);

                /* The optional sixth argument is the maximum number of DSCB entries */
                if (argc > 6 && argv[6] != NULL)
                {
                    if (sscanf( argv[6], "%u%c", &nMaxDSCB, &c) != 1 || nMaxDSCB < 0)
                        argexit(5, pgm);
                }
            }
        }

        if (!nMaxDBLK) nMaxDBLK = DEF_MAXDBLK;
        if (!nMaxTTR ) nMaxTTR  = DEF_MAXTTR;
        if (!nMaxDSCB) nMaxDSCB = DEF_MAXDSCB;
    }

    /* Open the control file */
    hostpath(pathname, cfname, sizeof(pathname));
    cfp = fopen (pathname, "r");
    if (cfp == NULL)
    {
        // "File %s; %s error: %s"
        XMERRF ( MSG( HHC02468, "E", cfname, "fopen()", strerror( errno ) ) );
        return -1;
    }

    /* Read first statement from control file */
    rc = read_ctrl_stmt (cfp, cfname, stmt, sizeof(stmt), &stmtno);
    if (rc < 0) return -1;

    /* Error if end of file */
    if (rc > 0)
    {
        // "Volume serial statement missing from %s"
        XMERRF ( MSG( HHC02500, "E", cfname ) );
        return -1;
    }

    /* Parse the volume serial statement */
    volser = strtok_r (stmt, " \t", &strtok_str);
    sdevtp = strtok_r (NULL, " \t", &strtok_str);
    sdevsz = strtok_r (NULL, " \t", &strtok_str);
    iplfnm = strtok_r (NULL, " \t", &strtok_str);

    /* Validate the volume serial number */
    if (volser == NULL || strlen(volser) == 0 || strlen(volser) > 6)
    {
        // "Volume serial %s is invalid; %s line %d"
        XMERRF ( MSG( HHC02501, "E", volser, cfname, stmtno ) );
        return -1;
    }
    string_to_upper (volser);

    /* Validate the device type */
    ckd = dasd_lookup (DASD_CKDDEV, sdevtp, 0, 0);
    if (ckd == NULL)
    {
        // "Device type %s is not recognized; %s line %d"
        XMERRF ( MSG( HHC02502, "E", sdevtp, cfname, stmtno ) );
        return -1;
    }
    devtype = ckd->devt;

    /* Obtain number of heads per cylinder, maximum data length per
       track, and default number of cylinders per device */
    outheads = ckd->heads;
    devcyls = ckd->cyls;
    if (altcylflag) devcyls += ckd->altcyls;
    outmaxdl = ckd->r1;

    /* Use default device size if requested size is omitted or
       is zero or is "*" */
    reqcyls = 0;
    if (sdevsz != NULL && strcmp(sdevsz, "*") != 0)
    {
        /* Validate the requested device size in cylinders */
        if (sscanf(sdevsz, "%u%c", &reqcyls, &c) != 1)
        {
            // "Cylinder value %s is invalid; %s line %d"
            XMERRF ( MSG( HHC02503, "E", sdevsz, cfname, stmtno ) );
            return -1;
        }
    }
    if (reqcyls == 0)
        reqcyls = devcyls;

    /* Calculate the track size of the virtual device */
    outtrklv = CKD_TRKHDR_SIZE
             + CKD_R0_SIZE     + CKD_R0_DLEN
             + CKD_RECHDR_SIZE + outmaxdl
             + CKD_ENDTRK_SIZE;
    outtrklv = ROUND_UP( outtrklv, 512 );

    /* Display progress message */

    // "Creating %4.4X volume %s: %u trks/cyl, %u bytes/track"
    XMINFF (0, MSG( HHC02520, "I", devtype, volser, outheads, outtrklv ) );

    /* Create the output file */
    EXTGUIMSG( "REQCYLS=%d\n", reqcyls );

#if DASDLOAD64
    rc = create_ckd64 (ofname, devtype, outheads, outmaxdl, reqcyls,
                     volser, comp, lfs, 0, 0, 0, 1, 0 );
#else
    rc = create_ckd (ofname, devtype, outheads, outmaxdl, reqcyls,
                     volser, comp, lfs, 0, 0, 0, 1, 0 );
#endif

    if (rc < 0)
    {
#if DASDLOAD64
        // "Cannot create %s; %s failed"
        XMERRF ( MSG( HHC02504, "E", ofname, "create_ckd64()" ) );
#else
        // "Cannot create %s; %s failed"
        XMERRF ( MSG( HHC02504, "E", ofname, "create_ckd()" ) );
#endif
        return -1;
    }

    /* Open the output file */
#if DASDLOAD64
    cif = open_ckd64_image (ofname, NULL, O_RDWR | O_BINARY, IMAGE_OPEN_NORMAL);
#else
    cif = open_ckd_image (ofname, NULL, O_RDWR | O_BINARY, IMAGE_OPEN_NORMAL);
#endif
    if (!cif)
    {
#if DASDLOAD64
        // "Cannot create %s; %s failed"
        XMERRF ( MSG( HHC02504, "E", ofname, "open_ckd64_image()" ) );
#else
        // "Cannot create %s; %s failed"
        XMERRF ( MSG( HHC02504, "E", ofname, "open_ckd_image()" ) );
#endif
        return -1;
    }

#if DASDLOAD64
    if (cif->devblk.cckd64 != 1)
#else
    if (cif->devblk.cckd64 != 0)
#endif
    {
        // "Dasd image file format unsupported or unrecognized: %s"
        XMERRF ( MSG( HHC02424, "E", ofname ) );
        return -1;
    }

    if (cif->devblk.cckd_ext)        /* have a compressed ckd extension?  */
        cckdblk.batchml = infolvl;   /* yes, save message level in cckdblk for cckddasd   */

    /* Display progress message */

    // "Loading %4.4X volume %s"
    XMINFF (0, MSG( HHC02521, "I", devtype, volser ) );

    /* Write track zero to the DASD image file */
    rc = write_track_zero (cif, ofname, volser, devtype,
                        outheads, outtrklv, iplfnm,
                        &reltrk, &outcyl, &outhead,
                        flagECmode, flagMachinecheck);
    if (rc < 0)
        return -1;

    /* Process the control file to create the datasets */
    rc = process_control_file (cfp, cfname, ofname, cif, volser,
                        devtype, reqcyls, outheads, outtrklv,
                        outcyl, outhead);

    /* Close files and release buffers */
    fclose (cfp);
    close_ckd_image (cif);

    return rc;

} /* end function main */
