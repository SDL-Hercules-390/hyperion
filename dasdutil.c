/* DASDUTIL.C   (c) Copyright Roger Bowler, 1999-2012                */
/*              Hercules DASD Utilities: Common subroutines          */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* This module contains common subroutines used by DASD utilities    */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

#define _DASDUTIL_C_
#define _HDASD_DLL_

#include "hercules.h"
#include "dasdblks.h"
#include "devtype.h"
#include "opcode.h"

/*-------------------------------------------------------------------*/
/* External references         (defined in ckddasd.c)                */
/*-------------------------------------------------------------------*/
extern DEVHND ckddasd_device_hndinfo;

/*-------------------------------------------------------------------*/
/* Internal macro definitions                                        */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* Static data areas                                                 */
/*-------------------------------------------------------------------*/
static int verbose = 0;                /* Be chatty about reads etc. */
static BYTE eighthexFF[] = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
static BYTE iplpsw[8]    = {0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x0F};
static BYTE iplccw1[8]   = {0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x01};
static BYTE iplccw2[8]   = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static int  nextnum = 0;

#if 0
SYSBLK sysblk; /* Currently required for shared.c */
#endif

/*-------------------------------------------------------------------*/
/* Subroutine to convert a null-terminated string to upper case      */
/*-------------------------------------------------------------------*/
DLL_EXPORT void string_to_upper (char *source)
{
int     i;                              /* Array subscript           */

    for (i = 0; source[i] != '\0'; i++)
        source[i] = toupper(source[i]);

} /* end function string_to_upper */

/*-------------------------------------------------------------------*/
/* Subroutine to convert a null-terminated string to lower case      */
/*-------------------------------------------------------------------*/
DLL_EXPORT void string_to_lower (char *source)
{
int     i;                              /* Array subscript           */

    for (i = 0; source[i] != '\0'; i++)
        source[i] = tolower(source[i]);

} /* end function string_to_lower */

/*-------------------------------------------------------------------*/
/* Subroutine to convert a string to EBCDIC and pad with blanks      */
/*-------------------------------------------------------------------*/
DLL_EXPORT void convert_to_ebcdic (BYTE *dest, int len, char *source)
{
int     i;                              /* Array subscript           */

    set_codepage(NULL);

    for (i = 0; i < len && source[i] != '\0'; i++)
        dest[i] = host_to_guest(source[i]);

    while (i < len)
        dest[i++] = 0x40;

} /* end function convert_to_ebcdic */

/*-------------------------------------------------------------------*/
/* Subroutine to convert an EBCDIC string to an ASCIIZ string.       */
/* Removes trailing blanks and adds a terminating null.              */
/* Returns the length of the ASCII string excluding terminating null */
/*-------------------------------------------------------------------*/
DLL_EXPORT int make_asciiz (char *dest, int destlen, BYTE *src, int srclen)
{
int             len;                    /* Result length             */

    set_codepage(NULL);

    for (len=0; len < srclen && len < destlen-1; len++)
        dest[len] = guest_to_host(src[len]);
    while (len > 0 && dest[len-1] == SPACE) len--;
    dest[len] = '\0';

    return len;

} /* end function make_asciiz */

/*-------------------------------------------------------------------*/
/* Subroutine to print a data block in hex and character format.     */
/*-------------------------------------------------------------------*/
DLL_EXPORT void data_dump ( void *addr, int len )
{
unsigned int    maxlen = 2048;
unsigned int    i, xi, offset, startoff = 0;
BYTE            c;
BYTE           *pchar;
char            print_chars[17];
char            hex_chars[64];
char            prev_hex[64] = "";
int             firstsame = 0;
int             lastsame = 0;

    set_codepage(NULL);

    pchar = (BYTE *)addr;

    for (offset=0; ; )
    {
        if (offset >= maxlen && offset <= len - maxlen)
        {
            offset += 16;
            pchar += 16;
            prev_hex[0] = '\0';
            continue;
        }
        if ( offset > 0 )
        {
            if ( strcmp ( hex_chars, prev_hex ) == 0 )
            {
                if ( firstsame == 0 ) firstsame = startoff;
                lastsame = startoff;
            }
            else
            {
                if ( firstsame != 0 )
                {
                    if ( lastsame == firstsame )
                        printf ("Line %4.4X same as above\n",
                                firstsame );
                    else
                        printf ("Lines %4.4X to %4.4X same as above\n",
                                firstsame, lastsame );
                    firstsame = lastsame = 0;
                }
                printf ("+%4.4X %s %s\n",
                        startoff, hex_chars, print_chars);
                strlcpy ( prev_hex, hex_chars, sizeof(prev_hex) );
            }
        }

        if ( offset >= (U32)len ) break;

        memset( print_chars, 0, sizeof(print_chars) );
        memset( hex_chars, SPACE, sizeof(hex_chars) );
        startoff = offset;
        for (xi=0, i=0; i < 16; i++)
        {
            c = *pchar++;
            if (offset < (U32)len) {
                sprintf(hex_chars+xi, "%2.2X", c);
                print_chars[i] = '.';
                if (isprint(c)) print_chars[i] = c;
                c = guest_to_host(c);
                if (isprint(c)) print_chars[i] = c;
            }
            offset++;
            xi += 2;
            hex_chars[xi] = SPACE;
            if ((offset & 3) == 0) xi++;
        } /* end for(i) */
        hex_chars[xi] = '\0';

    } /* end for(offset) */

} /* end function data_dump */

/*-------------------------------------------------------------------*/
/* Subroutine to read a track from the CKD DASD image                */
/* Input:                                                            */
/*      cif     -> CKD image file descriptor structure               */
/*      cyl     Cylinder number                                      */
/*      head    Head number                                          */
/* Output:                                                           */
/*      The track is read into trkbuf, and curcyl and curhead        */
/*      are set to the cylinder and head number.                     */
/*                                                                   */
/* Return value is 0 if successful, -1 if error                      */
/*-------------------------------------------------------------------*/
DLL_EXPORT int read_track (CIFBLK *cif, U32 cyl, U8 head)
{
int             rc;                     /* Return code               */
U32             trk;                    /* Track number              */
DEVBLK         *dev;                    /* -> CKD device block       */
BYTE            unitstat;               /* Unit status               */

    /* Exit if required track is already in buffer */
    if (cif->curcyl == cyl && cif->curhead == head)
        return 0;

    dev = &cif->devblk;

    if (cif->trkmodif)
    {
        cif->trkmodif = 0;
        if (verbose) /* Issue progress message */
           fprintf (stdout, MSG(HHC00445, "I", SSID_TO_LCSS(cif->devblk.ssid), cif->devblk.devnum, cif->fname,
                    cif->curcyl, cif->curhead));
        trk = (cif->curcyl * cif->heads) + cif->curhead;
        rc = (dev->hnd->write)(dev, trk, 0, NULL, cif->trksz, &unitstat);
        if (rc < 0)
        {
            fprintf (stderr, MSG(HHC00446, "E", SSID_TO_LCSS(cif->devblk.ssid), cif->devblk.devnum, cif->fname,
                    unitstat));
            return -1;
        }
    }

    if (verbose) /* Issue progress message */
       fprintf (stdout, MSG(HHC00447, "I", SSID_TO_LCSS(cif->devblk.ssid), cif->devblk.devnum, cif->fname,
                            cyl, head));

    trk = (cyl * cif->heads) + head;
    rc = (dev->hnd->read)(dev, trk, &unitstat);
    if (rc < 0)
    {
        fprintf (stderr, MSG(HHC00448, "E", SSID_TO_LCSS(cif->devblk.ssid), cif->devblk.devnum, cif->fname,
                unitstat));
        return -1;
    }

    /* Set current buf, cylinder and head */
    cif->trkbuf = dev->buf;
    cif->curcyl = cyl;
    cif->curhead = head;

    return 0;
} /* end function read_track */

/*-------------------------------------------------------------------*/
/* Subroutine to read a block from the CKD DASD image                */
/* Input:                                                            */
/*      cif     -> CKD image file descriptor structure               */
/*      cyl     Cylinder number of requested block                   */
/*      head    Head number of requested block                       */
/*      rec     Record number of requested block                     */
/* Output:                                                           */
/*      keyptr  Pointer to record key                                */
/*      keylen  Actual key length                                    */
/*      dataptr Pointer to record data                               */
/*      datalen Actual data length                                   */
/*                                                                   */
/* Return value is 0 if successful, +1 if end of track, -1 if error  */
/*-------------------------------------------------------------------*/
DLL_EXPORT int read_block (CIFBLK *cif, U32 cyl, U8 head, U8 rec, BYTE **keyptr,
                U8 *keylen, BYTE **dataptr, U16 *datalen)
{
int             rc;                     /* Return code               */
BYTE           *ptr;                    /* -> byte in track buffer   */
CKDDASD_RECHDR *rechdr;                 /* -> Record header          */
U8              kl;                     /* Key length                */
U16             dl;                     /* Data length               */

    /* Read the required track into the track buffer if necessary */
    rc = read_track (cif, cyl, head);
    if (rc < 0) return -1;

    /* Search for the requested record in the track buffer */
    ptr = cif->trkbuf;
    ptr += CKDDASD_TRKHDR_SIZE;

    while (1)
    {
        /* Exit with record not found if end of track */
        if (memcmp(ptr, eighthexFF, 8) == 0)
            return +1;

        /* Extract key length and data length from count field */
        rechdr = (CKDDASD_RECHDR*)ptr;
        kl = rechdr->klen;
        dl = (rechdr->dlen[0] << 8) | rechdr->dlen[1];

        /* Exit if requested record number found */
        if (rechdr->rec == rec)
            break;

        /* Issue progress message */
//      fprintf (stdout,
//              "Skipping CCHHR=%2.2X%2.2X%2.2X%2.2X"
//              "%2.2X KL=%2.2X DL=%2.2X%2.2X\n",
//              rechdr->cyl[0], rechdr->cyl[1],
//              rechdr->head[0], rechdr->head[1],
//              rechdr->rec, rechdr->klen,
//              rechdr->dlen[0], rechdr->dlen[1]);

        /* Point past count key and data to next block */
        ptr += CKDDASD_RECHDR_SIZE + kl + dl;
    }

    /* Return key and data pointers and lengths */
    if (keyptr != NULL) *keyptr = ptr + CKDDASD_RECHDR_SIZE;
    if (keylen != NULL) *keylen = kl;
    if (dataptr != NULL) *dataptr = ptr + CKDDASD_RECHDR_SIZE + kl;
    if (datalen != NULL) *datalen = dl;
    return 0;

} /* end function read_block */

/*-------------------------------------------------------------------*/
/* Subroutine to search a dataset for a specified key                */
/* Input:                                                            */
/*      cif     -> CKD image file descriptor structure               */
/*      key     Key value                                            */
/*      keylen  Key length                                           */
/*      noext   Number of extents                                    */
/*      extent  Dataset extent array                                 */
/* Output:                                                           */
/*      cyl     Cylinder number of requested block                   */
/*      head    Head number of requested block                       */
/*      rec     Record number of requested block                     */
/*                                                                   */
/* Return value is 0 if successful, +1 if key not found, -1 if error */
/*-------------------------------------------------------------------*/
DLL_EXPORT int search_key_equal (CIFBLK *cif, BYTE *key, U8 keylen, u_int noext,
                    DSXTENT extent[], U32 *cyl, U8 *head, U8 *rec)
{
int             rc;                     /* Return code               */
U32             ccyl;                   /* Cylinder number           */
U8              chead;                  /* Head number               */
u_int           cext;                   /* Extent sequence number    */
U32             ecyl;                   /* Extent end cylinder       */
U32             ehead;                  /* Extent end head           */
BYTE           *ptr;                    /* -> byte in track buffer   */
CKDDASD_RECHDR *rechdr;                 /* -> Record header          */
U8              kl;                     /* Key length                */
U16             dl;                     /* Data length               */

    /* Start at first track of first extent */
    cext = 0;
    ccyl = (extent[cext].xtbcyl[0] << 8) | extent[cext].xtbcyl[1];
    chead = (extent[cext].xtbtrk[0] << 8) | extent[cext].xtbtrk[1];
    ecyl = (extent[cext].xtecyl[0] << 8) | extent[cext].xtecyl[1];
    ehead = (extent[cext].xtetrk[0] << 8) | extent[cext].xtetrk[1];

    if (verbose)
    {
       fprintf (stdout, MSG(HHC00449, "I", SSID_TO_LCSS(cif->devblk.ssid), cif->devblk.devnum, cif->fname,
               cext, ccyl, chead, ecyl, ehead));
    }

    while (1)
    {
        /* Read the required track into the track buffer */
        rc = read_track (cif, ccyl, chead);
        if (rc < 0) return -1;

        /* Search for the requested record in the track buffer */
        ptr = cif->trkbuf;
        ptr += CKDDASD_TRKHDR_SIZE;

        while (1)
        {
            /* Exit loop at end of track */
            if (memcmp(ptr, eighthexFF, 8) == 0)
                break;

            /* Extract key length and data length from count field */
            rechdr = (CKDDASD_RECHDR*)ptr;
            kl = rechdr->klen;
            dl = (rechdr->dlen[0] << 8) | rechdr->dlen[1];

            /* Return if requested record key found */
            if (kl == keylen
                && memcmp(ptr + CKDDASD_RECHDR_SIZE, key, 44) == 0)
            {
                *cyl = ccyl;
                *head = chead;
                *rec = rechdr->rec;
                return 0;
            }

            /* Issue progress message */
//          fprintf (stdout,
//                  "Skipping CCHHR=%2.2X%2.2X%2.2X%2.2X"
//                  "%2.2X KL=%2.2X DL=%2.2X%2.2X\n",
//                  rechdr->cyl[0], rechdr->cyl[1],
//                  rechdr->head[0], rechdr->head[1],
//                  rechdr->rec, rechdr->klen,
//                  rechdr->dlen[0], rechdr->dlen[1]);

            /* Point past count key and data to next block */
            ptr += CKDDASD_RECHDR_SIZE + kl + dl;

        } /* end while */

        /* Point to the next track */
        chead++;
        if (chead >= cif->heads)
        {
            ccyl++;
            chead = 0;
        }

        /* Loop if next track is within current extent */
        if (ccyl < ecyl || (ccyl == ecyl && chead <= ehead))
            continue;

        /* Move to next extent */
        cext++;
        if (cext >= noext) break;
        ccyl = (extent[cext].xtbcyl[0] << 8) | extent[cext].xtbcyl[1];
        chead = (extent[cext].xtbtrk[0] << 8) | extent[cext].xtbtrk[1];
        ecyl = (extent[cext].xtecyl[0] << 8) | extent[cext].xtecyl[1];
        ehead = (extent[cext].xtetrk[0] << 8) | extent[cext].xtetrk[1];

       if (verbose)
       {
           fprintf (stdout, MSG(HHC00449, "I", SSID_TO_LCSS(cif->devblk.ssid), cif->devblk.devnum, cif->fname,
                   cext, ccyl, chead, ecyl, ehead));
       }

    } /* end while */

    /* Return record not found at end of extents */
    return +1;

} /* end function search_key_equal */

/*-------------------------------------------------------------------*/
/* Subroutine to convert relative track to cylinder and head         */
/* Input:                                                            */
/*      tt      Relative track number                                */
/*      noext   Number of extents in dataset                         */
/*      extent  Dataset extent array                                 */
/*      heads   Number of tracks per cylinder                        */
/* Output:                                                           */
/*      cyl     Cylinder number                                      */
/*      head    Head number                                          */
/*                                                                   */
/* Return value is 0 if successful, or -1 if error                   */
/*-------------------------------------------------------------------*/
DLL_EXPORT int convert_tt (u_int tt, u_int noext, DSXTENT extent[], U8 heads,
                U32 *cyl, U8 *head)
{
u_int           i;                      /* Extent sequence number    */
u_int           trk;                    /* Relative track number     */
u_int           bcyl;                   /* Extent begin cylinder     */
u_int           btrk;                   /* Extent begin head         */
u_int           ecyl;                   /* Extent end cylinder       */
u_int           etrk;                   /* Extent end head           */
u_int           start;                  /* Extent begin track        */
u_int           end;                    /* Extent end track          */
u_int           extsize;                /* Extent size in tracks     */

    for (i = 0, trk = tt; i < noext; i++)
    {
        bcyl = (extent[i].xtbcyl[0] << 8) | extent[i].xtbcyl[1];
        btrk = (extent[i].xtbtrk[0] << 8) | extent[i].xtbtrk[1];
        ecyl = (extent[i].xtecyl[0] << 8) | extent[i].xtecyl[1];
        etrk = (extent[i].xtetrk[0] << 8) | extent[i].xtetrk[1];

        start = (bcyl * heads) + btrk;
        end = (ecyl * heads) + etrk;
        extsize = end - start + 1;

        if (trk < extsize)
        {
            trk += start;
            *cyl = trk / heads;
            *head = trk % heads;
            return 0;
        }

        trk -= extsize;

    } /* end for(i) */

    fprintf (stderr, MSG(HHC00450, "E", tt));
    return -1;

} /* end function convert_tt */

/*-------------------------------------------------------------------*/
/* Subroutine to open a CKD image file                               */
/* Input:                                                            */
/*      fname    CKD image file name                                 */
/*      sfname   Shadow-File option string  (e.g. "sf=shadow_*.xxx") */
/*      omode    Open mode: O_RDONLY or O_RDWR                       */
/*      option   IMAGE_OPEN_NORMAL, IMAGE_OPEN_DASDCOPY, etc.        */
/*                                                                   */
/* The CKD image file is opened, a track buffer is obtained,         */
/* and a CKD image file descriptor structure is built.               */
/* Return value is a pointer to the CKD image file descriptor        */
/* structure if successful, or NULL if unsuccessful.                 */
/*-------------------------------------------------------------------*/
DLL_EXPORT CIFBLK* open_ckd_image (char *fname, char *sfname, int omode,
                       int option)
{
int             fd;                     /* File descriptor           */
int             rc;                     /* Return code               */
int             iLen;                   /* Record length             */
CKDDASD_DEVHDR  devhdr;                 /* CKD device header         */
CIFBLK         *cif;                    /* CKD image file descriptor */
DEVBLK         *dev;                    /* CKD device block          */
CKDDEV         *ckd;                    /* CKD DASD table entry      */
char           *rmtdev;                 /* Possible remote device    */
char           *argv[2];                /* Arguments to              */
int             argc=0;                 /*                           */
char            sfxname[FILENAME_MAX*2];/* Suffixed file name        */
char            typname[64];
char            pathname[MAX_PATH];     /* file path in host format  */

    /* Obtain storage for the file descriptor structure */
    cif = (CIFBLK*) calloc (1,sizeof(CIFBLK));
    if (cif == NULL)
    {
        char buf[40];
        MSGBUF(buf, "calloc(1,%d)", (int)sizeof(CIFBLK));
        fprintf (stderr, MSG(HHC00404, "E", SSID_TO_LCSS(cif->devblk.ssid), cif->devblk.devnum, fname,
                             buf, strerror(errno)));
        return NULL;
    }

    /* Initialize the devblk */
    dev = &cif->devblk;
    if ((omode & O_RDWR) == 0) dev->ckdrdonly = 1;
    dev->fd = -1;
    dev->batch = 1;
    dev->dasdcopy  = (option & IMAGE_OPEN_DASDCOPY) ? 1 : 0;
    dev->showdvol1 = (option & IMAGE_OPEN_DVOL1)    ? 1 : 0;
    dev->quiet     = (option & IMAGE_OPEN_QUIET)    ? 1 : 0;

    /* If the filename has a `:' then it may be a remote device */
    if ((rmtdev = strchr(fname, ':')))
    {
        /* Verify port number follows colon */
        char *p;
        for (p = rmtdev + 1; *p && *p != ':'; p++)
        {
            if (!isdigit(*p))  /* (port numbers are always numeric) */
            {
                /* Not a port number ==> not really a remote device */
                rmtdev = NULL;
                break;
            }
        }
    }

    /* Read the device header so we can determine the device type */
    strlcpy( sfxname, fname, sizeof(sfxname));
    hostpath(pathname, sfxname, sizeof(pathname));
    fd = HOPEN (pathname, omode);
    if (fd < 0)
    {
        /* If no shadow file name was specified, then try opening the
           file with the file sequence number in the name */
        if (sfname == NULL)
        {
            int i;
            char *s,*suffix;

            /* Look for last slash marking end of directory name */
            s = strrchr (fname, PATHSEPC);
            if (s == NULL) s = fname;

            /* Insert suffix before first dot in file name, or
               append suffix to file name if there is no dot.
               If the filename already has a place for the suffix
               then use that. */
            s = strchr (s, '.');
            if (s != NULL)
            {
                i = s - fname;
                if (i > 2 && fname[i-2] == '_')
                    suffix = sfxname + i - 1;
                else
                {
                    strlcpy( sfxname + i, "_1", sizeof(sfxname)-(size_t)i );
                    strlcat( sfxname, fname + i, sizeof(sfxname) );
                    suffix = sfxname + i + 1;
                }
            }
            else
            {
                if (strlen(sfxname) < 2 || sfxname[strlen(sfxname)-2] != '_')
                    strlcat (sfxname, "_1", sizeof(sfxname) );
                suffix = sfxname + strlen(sfxname) - 1;
            }
            *suffix = '1';
            hostpath(pathname, sfxname, sizeof(pathname));
            fd = HOPEN (pathname, omode);
        }
        if (fd < 0 && rmtdev == NULL)
        {
            fprintf (stderr, MSG(HHC00404, "E", SSID_TO_LCSS(cif->devblk.ssid), cif->devblk.devnum, cif->fname,
                                 "open()", strerror(errno)));
            free (cif);
            return NULL;
        }
        else if (fd < 0) strlcpy( sfxname, fname, sizeof(sfxname) );
    }

    /* If not a possible remote device, check the dasd header
       and set the device type */
    if (fd >= 0)
    {

        iLen = read(fd, &devhdr, CKDDASD_DEVHDR_SIZE);
        if (iLen < 0)
        {
            fprintf (stderr, MSG(HHC00404, "E", SSID_TO_LCSS(cif->devblk.ssid), cif->devblk.devnum, cif->fname,
                     "read()", strerror(errno)));
            close (fd);
            free (cif);
            return NULL;
        }
        close (fd);

        if (iLen < CKDDASD_DEVHDR_SIZE
         || (memcmp(devhdr.devid, "CKD_P370", 8)
          && memcmp(devhdr.devid, "CKD_C370", 8)))
        {
            fprintf (stderr, MSG(HHC00406, "E", SSID_TO_LCSS(cif->devblk.ssid), cif->devblk.devnum, cif->fname));
            free (cif);
            return NULL;
        }

        /* Set the device type */
        ckd = dasd_lookup (DASD_CKDDEV, NULL, devhdr.devtype, 0);
        if (ckd == NULL)
        {
            fprintf(stderr, MSG(HHC00451, "E", SSID_TO_LCSS(cif->devblk.ssid), cif->devblk.devnum, cif->fname,
                    devhdr.devtype));
            free (cif);
            return NULL;
        }
        dev->devtype = ckd->devt;
        MSGBUF(typname, "%4.4X", dev->devtype);
        dev->typname=typname;   /* Makes HDL Happy */
    }

    /* Set the device handlers */
    dev->hnd = &ckddasd_device_hndinfo;

    /* Set the device number */
    dev->devnum = ++nextnum;

    /* Build arguments for ckddasd_init_handler */
    argv[0] = sfxname;
    argc++;
    if (sfname != NULL)
    {
        argv[1] = sfname;
        argc++;
    }

    /* Call the device handler initialization function */
    rc = (dev->hnd->init)(dev, argc, argv);
    if (rc < 0)
    {
        fprintf (stderr, MSG(HHC00452, "E", SSID_TO_LCSS(cif->devblk.ssid), cif->devblk.devnum, cif->fname));
        free (cif);
        return NULL;
    }

    /* Call the device start exit */
    if (dev->hnd->start) (dev->hnd->start) (dev);

    /* Set CIF fields */
    cif->fname = fname;
    cif->fd = dev->fd;

    /* Extract the number of heads and the track size */
    cif->heads = dev->ckdheads;
    cif->trksz = ((U32)(devhdr.trksize[3]) << 24)
                | ((U32)(devhdr.trksize[2]) << 16)
                | ((U32)(devhdr.trksize[1]) << 8)
                | (U32)(devhdr.trksize[0]);
    if (verbose)
    {
       fprintf (stdout, MSG(HHC00453, "I", SSID_TO_LCSS(cif->devblk.ssid), cif->devblk.devnum, cif->fname,
               cif->heads, cif->trksz));
    }

    /* Indicate that the track buffer is empty */
    cif->curcyl = -1;
    cif->curhead = -1;
    cif->trkmodif = 0;

    return cif;
} /* end function open_ckd_image */

/*-------------------------------------------------------------------*/
/* Subroutine to close a CKD image file                              */
/* Input:                                                            */
/*      cif     -> CKD image file descriptor structure               */
/*                                                                   */
/* The track buffer is flushed and released, the CKD image file      */
/* is closed, and the file descriptor structure is released.         */
/* Return value is 0 if successful, -1 if error                      */
/*-------------------------------------------------------------------*/
DLL_EXPORT int close_ckd_image (CIFBLK *cif)
{
int             rc;                     /* Return code               */
U32             trk;                    /* Track number              */
DEVBLK         *dev;                    /* -> CKD device block       */
BYTE            unitstat;               /* Unit status               */

    dev = &cif->devblk;

    /* Write the last track if modified */
    if (cif->trkmodif)
    {
        if (verbose) /* Issue progress message */
           fprintf (stdout, MSG(HHC00445, "I", SSID_TO_LCSS(cif->devblk.ssid), cif->devblk.devnum, cif->fname,
                    cif->curcyl, cif->curhead));
        trk = (cif->curcyl * cif->heads) + cif->curhead;
        rc = (dev->hnd->write)(dev, trk, 0, NULL, cif->trksz, &unitstat);
        if (rc < 0)
        {
            fprintf (stderr, MSG(HHC00446, "E", SSID_TO_LCSS(cif->devblk.ssid), cif->devblk.devnum, cif->fname,
                    unitstat));
        }
    }

    /* Call the END exit */
    if (dev->hnd->end) (dev->hnd->end) (dev);

    /* Close the CKD image file */
    (dev->hnd->close)(dev);

    /* Release the file descriptor structure */
    free (cif);

    return 0;
} /* end function close_ckd_image */

/*-------------------------------------------------------------------*/
/* Subroutine to open a FBA image file                               */
/* Input:                                                            */
/*      fname    FBA image file name                                 */
/*      sfname   Shadow-File option string  (e.g. "sf=shadow_*.xxx") */
/*      omode    Open mode: O_RDONLY or O_RDWR                       */
/*      option   IMAGE_OPEN_NORMAL, IMAGE_OPEN_DASDCOPY, etc.        */
/*                                                                   */
/* The FBA image file is opened, a track buffer is obtained,         */
/* and a FBA image file descriptor structure is built.               */
/* Return value is a pointer to the FBA image file descriptor        */
/* structure if successful, or NULL if unsuccessful.                 */
/*-------------------------------------------------------------------*/
DLL_EXPORT CIFBLK* open_fba_image (char *fname, char *sfname, int omode,
                        int option)
{
int             rc;                     /* Return code               */
CIFBLK         *cif;                    /* FBA image file descriptor */
DEVBLK         *dev;                    /* FBA device block          */
FBADEV         *fba;                    /* FBA DASD table entry      */
char           *argv[2];                /* Arguments to              */
int             argc=0;                 /*  device open              */

    /* Obtain storage for the file descriptor structure */
    cif = (CIFBLK*) calloc( 1, sizeof(CIFBLK) );
    if (cif == NULL)
    {
        char buf[40];
        MSGBUF(buf, "calloc(1,%d)", (int)sizeof(CIFBLK));
        fprintf (stderr, MSG(HHC00404, "E", SSID_TO_LCSS(cif->devblk.ssid), cif->devblk.devnum, fname,
                buf, strerror(errno)));
        return NULL;
    }

    /* Initialize the devblk */
    dev = &cif->devblk;
    if ((omode & O_RDWR) == 0) dev->ckdrdonly = 1;
    dev->batch = 1;
    dev->dasdcopy  = (option & IMAGE_OPEN_DASDCOPY) ? 1 : 0;
    dev->showdvol1 = (option & IMAGE_OPEN_DVOL1)    ? 1 : 0;
    dev->quiet     = (option & IMAGE_OPEN_QUIET)    ? 1 : 0;

    /* Set the device type */
    fba = dasd_lookup (DASD_FBADEV, NULL, DEFAULT_FBA_TYPE, 0);
    if (fba == NULL)
    {
        fprintf(stderr, MSG(HHC00451, "E", SSID_TO_LCSS(cif->devblk.ssid), cif->devblk.devnum, fname,
                DEFAULT_FBA_TYPE));
        free (cif);
        return NULL;
    }
    dev->devtype = fba->devt;

    /* Set the device handlers */
    dev->hnd = &fbadasd_device_hndinfo;

    /* Set the device number */
    dev->devnum = ++nextnum;

    /* Build arguments for fbadasd_init_handler */
    argv[0] = fname;
    argc++;
    if (sfname != NULL)
    {
        argv[1] = sfname;
        argc++;
    }

    /* Call the device handler initialization function */
    rc = (dev->hnd->init)(dev, argc, argv);
    if (rc < 0)
    {
        fprintf (stderr, MSG(HHC00452, "E", SSID_TO_LCSS(cif->devblk.ssid), cif->devblk.devnum, fname));
        free (cif);
        return NULL;
    }

    /* Set CIF fields */
    cif->fname = fname;
    cif->fd = dev->fd;

    /* Extract the number of sectors and the sector size */
    cif->heads = dev->fbanumblk;
    cif->trksz = dev->fbablksiz;
    if (verbose)
    {
       fprintf (stdout, MSG(HHC00454, "I", SSID_TO_LCSS(cif->devblk.ssid), cif->devblk.devnum, fname,
               cif->heads, cif->trksz));
    }

    /* Indicate that the track buffer is empty */
    cif->curcyl = -1;
    cif->curhead = -1;
    cif->trkmodif = 0;

    return cif;
} /* end function open_fba_image */

/*-------------------------------------------------------------------*/
/* Subroutine to build extent array for specified dataset            */
/* Input:                                                            */
/*      cif     -> CKD image file descriptor structure               */
/*      dsnama  -> Dataset name (ASCIIZ)                             */
/* Output:                                                           */
/*      extent  Extent array (up to 16 entries)                      */
/*      noext   Number of extents                                    */
/*                                                                   */
/* Return value is 0 if successful, or -1 if error                   */
/*-------------------------------------------------------------------*/
DLL_EXPORT int build_extent_array (CIFBLK *cif, char *dsnama, DSXTENT extent[],
                        int *noext)
{
int             rc;                     /* Return code               */
U16             len;                    /* Record length             */
U8              keylen;                 /* Key length                */
U32             cyl;                    /* Cylinder number           */
U8              head;                   /* Head number               */
U8              rec;                    /* Record number             */
BYTE           *vol1data;               /* -> Volume label           */
FORMAT1_DSCB   *f1dscb;                 /* -> Format 1 DSCB          */
FORMAT3_DSCB   *f3dscb;                 /* -> Format 3 DSCB          */
FORMAT4_DSCB   *f4dscb;                 /* -> Format 4 DSCB          */
BYTE            dsname[44];             /* Dataset name (EBCDIC)     */
char            volser[7];              /* Volume serial (ASCIIZ)    */

    /* Convert the dataset name to EBCDIC */
    convert_to_ebcdic (dsname, sizeof(dsname), dsnama);

    /* Read the volume label */
    rc = read_block (cif, 0, 0, 3, NULL, NULL, &vol1data, &len);
    if (rc < 0) return -1;
    if (rc > 0)
    {
        fprintf (stderr, MSG(HHC00455, "E", SSID_TO_LCSS(cif->devblk.ssid), cif->devblk.devnum, cif->fname, "VOL1"));
        return -1;
    }

    /* Extract the volume serial and the CCHHR of the format 4 DSCB */
    make_asciiz (volser, sizeof(volser), vol1data+4, 6);
    cyl = (vol1data[11] << 8) | vol1data[12];
    head = (vol1data[13] << 8) | vol1data[14];
    rec = vol1data[15];

    if (verbose)
    {
       fprintf (stdout, MSG(HHC00456, "I", SSID_TO_LCSS(cif->devblk.ssid), cif->devblk.devnum, cif->fname,
                volser, cyl, head, rec));
    }

    /* Read the format 4 DSCB */
    rc = read_block (cif, cyl, head, rec,
                    (void *)&f4dscb, &keylen, NULL, NULL);
    if (rc < 0) return -1;
    if (rc > 0)
    {
        fprintf (stderr, MSG(HHC00455, "E", SSID_TO_LCSS(cif->devblk.ssid), cif->devblk.devnum, cif->fname, "F4DSCB"));
        return -1;
    }

    if (verbose)
    {
       fprintf (stdout, MSG(HHC00457, "I", SSID_TO_LCSS(cif->devblk.ssid), cif->devblk.devnum, cif->fname,
               f4dscb->ds4vtoce.xtbcyl[0], f4dscb->ds4vtoce.xtbcyl[1],
               f4dscb->ds4vtoce.xtbtrk[0], f4dscb->ds4vtoce.xtbtrk[1],
               f4dscb->ds4vtoce.xtecyl[0], f4dscb->ds4vtoce.xtecyl[1],
               f4dscb->ds4vtoce.xtetrk[0], f4dscb->ds4vtoce.xtetrk[1]));
    }

    /* Search for the requested dataset in the VTOC */
    rc = search_key_equal (cif, dsname, sizeof(dsname),
                            1, &(f4dscb->ds4vtoce),
                            &cyl, &head, &rec);
    if (rc < 0) return -1;
    if (rc > 0)
    {
        fprintf (stderr, MSG(HHC00458, "E", SSID_TO_LCSS(cif->devblk.ssid), cif->devblk.devnum, cif->fname, dsnama));
        return -1;
    }

    if (verbose)
    {
       fprintf (stdout, MSG(HHC00459, "I", SSID_TO_LCSS(cif->devblk.ssid), cif->devblk.devnum, cif->fname,
               dsnama, cyl, head, rec));
    }

    /* Read the format 1 DSCB */
    rc = read_block (cif, cyl, head, rec,
                    (void *)&f1dscb, &keylen, NULL, NULL);
    if (rc < 0) return -1;
    if (rc > 0)
    {
        fprintf (stderr, MSG(HHC00455, "E", SSID_TO_LCSS(cif->devblk.ssid), cif->devblk.devnum, cif->fname, "F1DSCB"));
        return -1;
    }

    /* Extract number of extents and first 3 extent descriptors */
    *noext = f1dscb->ds1noepv;
    extent[0] = f1dscb->ds1ext1;
    extent[1] = f1dscb->ds1ext2;
    extent[2] = f1dscb->ds1ext3;

    /* Obtain additional extent descriptors */
    if (f1dscb->ds1noepv > 3)
    {
        /* Read the format 3 DSCB */
        cyl = (f1dscb->ds1ptrds[0] << 8) | f1dscb->ds1ptrds[1];
        head = (f1dscb->ds1ptrds[2] << 8) | f1dscb->ds1ptrds[3];
        rec = f1dscb->ds1ptrds[4];
        rc = read_block (cif, cyl, head, rec,
                        (void *)&f3dscb, &keylen, NULL, NULL);
        if (rc < 0) return -1;
        if (rc > 0)
        {
            fprintf (stderr, MSG(HHC00455, "E", SSID_TO_LCSS(cif->devblk.ssid), cif->devblk.devnum, cif->fname, "F3DSCB"));
            return -1;
        }

        /* Extract the next 13 extent descriptors */
        extent[3] = f3dscb->ds3extnt[0];
        extent[4] = f3dscb->ds3extnt[1];
        extent[5] = f3dscb->ds3extnt[2];
        extent[6] = f3dscb->ds3extnt[3];
        extent[7] = f3dscb->ds3adext[0];
        extent[8] = f3dscb->ds3adext[1];
        extent[9] = f3dscb->ds3adext[2];
        extent[10] = f3dscb->ds3adext[3];
        extent[11] = f3dscb->ds3adext[4];
        extent[12] = f3dscb->ds3adext[5];
        extent[13] = f3dscb->ds3adext[6];
        extent[14] = f3dscb->ds3adext[7];
        extent[15] = f3dscb->ds3adext[8];
    }

    return 0;
} /* end function build_extent_array */

/*-------------------------------------------------------------------*/
/* Subroutine to calculate physical device track capacities          */
/* Input:                                                            */
/*      cif     -> CKD image file descriptor structure               */
/*      used    Number of bytes used so far on track,                */
/*              excluding home address and record 0                  */
/*      keylen  Key length of proposed new record                    */
/*      datalen Data length of proposed new record                   */
/* Output:                                                           */
/*      newused Number of bytes used including proposed new record   */
/*      trkbaln Number of bytes remaining on track                   */
/*      physlen Number of bytes on physical track (=ds4devtk)        */
/*      kbconst Overhead for non-last keyed block (=ds4devi)         */
/*      lbconst Overhead for last keyed block (=ds4devl)             */
/*      nkconst Overhead difference for non-keyed block (=ds4devk)   */
/*      devflag Device flag byte for VTOC (=ds4devfg)                */
/*      tolfact Device tolerance factor (=ds4devtl)                  */
/*      maxdlen Maximum data length for non-keyed record 1           */
/*      numrecs Number of records of specified length per track      */
/*      numhead Number of tracks per cylinder                        */
/*      numcyls Number of cylinders per volume                       */
/* Note:                                                             */
/*      A NULL address may be specified for any of the output        */
/*      fields if the output value is not required.                  */
/*      The return value is 0 if the record will fit on the track,   */
/*      +1 if record will not fit on track, or -1 if unknown devtype */
/* Note:                                                             */
/*      Although the virtual DASD image file contains no interrecord */
/*      gaps, this subroutine performs its calculations taking into  */
/*      account the gaps that would exist on a real device, so that  */
/*      the track capacities of the real device are not exceeded.    */
/*-------------------------------------------------------------------*/
DLL_EXPORT int capacity_calc (CIFBLK *cif, int used, int keylen, int datalen,
                int *newused, int *trkbaln, int *physlen, int *kbconst,
                int *lbconst, int *nkconst, BYTE*devflag, int *tolfact,
                int *maxdlen, int *numrecs, int *numhead, int *numcyls)
{
CKDDEV         *ckd;                    /* -> CKD device table entry */
U8              heads;                  /* Number of tracks/cylinder */
U32             cyls;                   /* Number of cyls/volume     */
U16             trklen;                 /* Physical track length     */
U16             maxlen;                 /* Maximum data length       */
int             devi, devl, devk;       /* Overhead fields for VTOC  */
BYTE            devfg;                  /* Flag field for VTOC       */
int             devtl;                  /* Tolerance field for VTOC  */
int             b1;                     /* Bytes used by new record
                                           when last record on track */
int             b2;                     /* Bytes used by new record
                                           when not last on track    */
int             nrecs;                  /* Number of record/track    */
int             c, d1, d2, x;           /* 23xx/3330/3350 factors    */
int             f1, f2, f3, f4, f5, f6; /* 3380/3390/9345 factors    */
int             fl1, fl2, int1, int2;   /* 3380/3390/9345 calculation*/

    ckd = cif->devblk.ckdtab;
    trklen = ckd->len;
    maxlen = ckd->r1;
    heads = ckd->heads;
    cyls = ckd->cyls;

    switch (ckd->formula) {

    case -2:  /* 2311, 2314 */
        c = ckd->f1; x = ckd->f2; d1 = ckd->f3; d2 = ckd->f4;
        b1 = keylen + datalen + (keylen == 0 ? 0 : c);
        b2 = ((keylen + datalen) * d1 / d2)
                + (keylen == 0 ? 0 : c) + x;
        nrecs = (trklen - b1)/b2 + 1;
        devi = c + x; devl = c; devk = c; devtl = d1 / (d2/512);
        devfg = 0x01;
        break;

    case -1:  /* 3330, 3340, 3350 */
        c = ckd->f1; x = ckd->f2;
        b1 = b2 = keylen + datalen + (keylen == 0 ? 0 : c) + x;
        nrecs = trklen / b2;
        devi = c + x; devl = c + x; devk = c; devtl = 512;
        devfg = 0x01;
        break;

    case 1:  /* 3375, 3380 */
        f1 = ckd->f1; f2 = ckd->f2; f3 = ckd->f3;
        fl1 = datalen + f2;
        fl2 = (keylen == 0 ? 0 : keylen + f3);
        fl1 = ((fl1 + f1 - 1) / f1) * f1;
        fl2 = ((fl2 + f1 - 1) / f1) * f1;
        b1 = b2 = fl1 + fl2;
        nrecs = trklen / b2;
        devi = 0; devl = 0; devk = 0; devtl = 0; devfg = 0x30;
        break;

    case 2:  /* 3390, 9345 */
        f1 = ckd->f1; f2 = ckd->f2; f3 = ckd->f3;
        f4 = ckd->f4; f5 = ckd->f5; f6 = ckd->f6;
        int1 = ((datalen + f6) + (f5*2-1)) / (f5*2);
        int2 = ((keylen + f6) + (f5*2-1)) / (f5*2);
        fl1 = (f1 * f2) + datalen + f6 + f4*int1;
        fl2 = (keylen == 0 ? 0 : (f1 * f3) + keylen + f6 + f4*int2);
        fl1 = ((fl1 + f1 - 1) / f1) * f1;
        fl2 = ((fl2 + f1 - 1) / f1) * f1;
        b1 = b2 = fl1 + fl2;
        nrecs = trklen / b2;
        devi = 0; devl = 0; devk = 0; devtl = 0; devfg = 0x30;
        break;

    default:
        return -1;
    } /* end switch(ckd->formula) */

    /* Return VTOC fields and maximum data length */
    if (physlen != NULL) *physlen = trklen;
    if (kbconst != NULL) *kbconst = devi;
    if (lbconst != NULL) *lbconst = devl;
    if (nkconst != NULL) *nkconst = devk;
    if (devflag != NULL) *devflag = devfg;
    if (tolfact != NULL) *tolfact = devtl;
    if (maxdlen != NULL) *maxdlen = maxlen;

    /* Return number of records per track */
    if (numrecs != NULL) *numrecs = nrecs;

    /* Return number of tracks per cylinder
       and usual number of cylinders per volume */
    if (numhead != NULL) *numhead = heads;
    if (numcyls != NULL) *numcyls = cyls;

    /* Return if record will not fit on the track */
    if (used + b1 > trklen)
        return +1;

    /* Calculate number of bytes used and track balance */
    if (newused != NULL)
        *newused = used + b2;
    if (trkbaln != NULL)
        *trkbaln = (used + b2 > trklen) ? 0 : trklen - used - b2;

    return 0;
} /* end function capacity_calc */

/*-------------------------------------------------------------------*/
/* Subroutine to create a CKD DASD image file                        */
/* Input:                                                            */
/*      fname    DASD image file name                                */
/*      fseqn    Sequence number of this file (1=first)              */
/*      devtype  Device type                                         */
/*      heads    Number of heads per cylinder                        */
/*      trksize  DADS image track length                             */
/*      buf      -> Track image buffer                               */
/*      start    Starting cylinder number for this file              */
/*      end      Ending cylinder number for this file                */
/*      volcyls  Total number of cylinders on volume                 */
/*      volser   Volume serial number                                */
/*      comp     Compression algorithm for a compressed device.      */
/*               Will be 0xff if device is not to be compressed.     */
/*      dasdcopy xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx      */
/*      nullfmt  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx      */
/*      rawflag  create raw image (skip special track 0 handling)    */
/*      flagECmode        1  set EC mode bit in wait PSW             */
/*                        0  don't set EC mode bit in wait PSW       */
/*      flagMachinecheck  1  set machine-check-enabled flag          */
/*                           in wait PSW                             */
/*                        0  don't set machine-check-enabled flag    */
/*                           in wait PSW                             */
/*-------------------------------------------------------------------*/
static int
create_ckd_file (char *fname, int fseqn, U16 devtype, U32 heads,
                U32 trksize, BYTE *buf, U32 start, U32 end,
                U32 volcyls, char *volser, BYTE comp, int dasdcopy,
                int nullfmt, int rawflag,
                int flagECmode, int flagMachinecheck)
{
int             rc;                     /* Return code               */
off_t           rcoff;                  /* Return value from lseek() */
int             fd;                     /* File descriptor           */
int             i;                      /* Loop counter              */
int             n;                      /* Loop delimiter            */
CKDDASD_DEVHDR  devhdr;                 /* Device header             */
CCKDDASD_DEVHDR cdevhdr;                /* Compressed device header  */
CCKD_L1ENT     *l1=NULL;                /* -> Primary lookup table   */
CCKD_L2ENT      l2[256];                /* Secondary lookup table    */
CKDDASD_TRKHDR *trkhdr;                 /* -> Track header           */
CKDDASD_RECHDR *rechdr;                 /* -> Record header          */
U32             cyl;                    /* Cylinder number           */
U32             head;                   /* Head number               */
U32             trk = 0;                /* Track number              */
U32             trks;                   /* Total number tracks       */
BYTE            r;                      /* Record number             */
BYTE           *pos;                    /* -> Next position in buffer*/
U32             cpos = 0;               /* Offset into cckd file     */
u_int           len = 0;                /* Length used in track      */
u_int           keylen = 4;             /* Length of keys            */
u_int           ipl1len = 24;           /* Length of IPL1 data       */
u_int           ipl2len = 144;          /* Length of IPL2 data       */
u_int           vol1len = 80;           /* Length of VOL1 data       */
u_int           rec0len = 8;            /* Length of R0 data         */
u_int           fileseq;                /* CKD header sequence number*/
u_int           highcyl;                /* CKD header high cyl number*/
int             x=O_EXCL;               /* Open option               */
CKDDEV         *ckdtab;                 /* -> CKD table entry        */
char            pathname[MAX_PATH];     /* file path in host format  */

    /* Locate the CKD dasd table entry */
    ckdtab = dasd_lookup (DASD_CKDDEV, NULL, devtype, 0);
    if (ckdtab == NULL)
    {
        fprintf (stderr, MSG(HHC00415, "E", 0, 0, fname, devtype));
        return -1;
    }

    /* Set file sequence number to zero if this is the only file */
    if (fseqn == 1 && end + 1 == volcyls)
        fileseq = 0;
    else
        fileseq = fseqn;

    /* Set high cylinder number to zero if this is the last file */
    if (end + 1 == volcyls)
        highcyl = 0;
    else
        highcyl = end;
    cyl = end - start + 1;

    /* Special processing for ckd and dasdcopy */
    if (comp == 0xFF && dasdcopy)
    {
        highcyl = end;
        if (end + 1 == volcyls)
            fileseq = 0xff;
    }

    trks = volcyls * heads;

    /* if `dasdcopy' > 1 then we can replace the existing file */
    if (dasdcopy > 1) x = 0;

    /* Create the DASD image file */
    hostpath(pathname, fname, sizeof(pathname));
    fd = HOPEN (pathname, O_WRONLY | O_CREAT | x | O_BINARY,
                S_IRUSR | S_IWUSR | S_IRGRP);
    if (fd < 0)
    {
        fprintf (stderr, MSG(HHC00404,"E", 0, 0, fname, "open()", strerror(errno)));
        return -1;
    }

    /* Create the device header */
    memset(&devhdr, 0, CKDDASD_DEVHDR_SIZE);
    if (comp == 0xff)
        memcpy(devhdr.devid, "CKD_P370", 8);
    else
        memcpy(devhdr.devid, "CKD_C370", 8);
    devhdr.heads[3] = (heads >> 24) & 0xFF;
    devhdr.heads[2] = (heads >> 16) & 0xFF;
    devhdr.heads[1] = (heads >> 8) & 0xFF;
    devhdr.heads[0] = heads & 0xFF;
    devhdr.trksize[3] = (trksize >> 24) & 0xFF;
    devhdr.trksize[2] = (trksize >> 16) & 0xFF;
    devhdr.trksize[1] = (trksize >> 8) & 0xFF;
    devhdr.trksize[0] = trksize & 0xFF;
    devhdr.devtype = devtype & 0xFF;
    devhdr.fileseq = fileseq;
    devhdr.highcyl[1] = (highcyl >> 8) & 0xFF;
    devhdr.highcyl[0] = highcyl & 0xFF;

    /* Write the device header */
    rc = write (fd, &devhdr, CKDDASD_DEVHDR_SIZE);
    if (rc < (int)CKDDASD_DEVHDR_SIZE)
    {
        fprintf (stderr, MSG(HHC00404, "E", 0, 0, fname, "write()",
                errno ? strerror(errno) : "incomplete"));
        return -1;
    }

    /* Build a compressed CKD file */
    if (comp != 0xff)
    {
        /* Create the compressed device header */
        memset(&cdevhdr, 0, CCKDDASD_DEVHDR_SIZE);
        cdevhdr.vrm[0] = CCKD_VERSION;
        cdevhdr.vrm[1] = CCKD_RELEASE;
        cdevhdr.vrm[2] = CCKD_MODLVL;
        if (cckd_endian())  cdevhdr.options |= CCKD_BIGENDIAN;
        cdevhdr.options |= CCKD_ORDWR;
        cdevhdr.numl1tab = (volcyls * heads + 255) / 256;
        cdevhdr.numl2tab = 256;
        cdevhdr.cyls[3] = (volcyls >> 24) & 0xFF;
        cdevhdr.cyls[2] = (volcyls >> 16) & 0xFF;
        cdevhdr.cyls[1] = (volcyls >>    8) & 0xFF;
        cdevhdr.cyls[0] = volcyls & 0xFF;
        cdevhdr.compress = comp;
        cdevhdr.compress_parm = -1;
        cdevhdr.nullfmt = nullfmt;

        /* Write the compressed device header */
        rc = write (fd, &cdevhdr, CCKDDASD_DEVHDR_SIZE);
        if (rc < (int)CCKDDASD_DEVHDR_SIZE)
        {
            fprintf (stderr, MSG(HHC00404, "E", 0, 0, fname,
                                 "write()", errno ? strerror(errno) : "incomplete"));
            return -1;
        }

        /* Create the primary lookup table */
        l1 = calloc (cdevhdr.numl1tab, CCKD_L1ENT_SIZE);
        if (l1 == NULL)
        {
            char buf[40];
            MSGBUF( buf, "calloc(%d,%d)", (int)cdevhdr.numl1tab, (int)CCKD_L1ENT_SIZE);
            fprintf (stderr, MSG(HHC00404, "E", 0, 0, fname, buf, strerror(errno)));
            return -1;
        }
        l1[0] = CCKD_L1TAB_POS + cdevhdr.numl1tab * CCKD_L1ENT_SIZE;

        /* Write the primary lookup table */
        rc = write (fd, l1, cdevhdr.numl1tab * CCKD_L1ENT_SIZE);
        if (rc < (int)(cdevhdr.numl1tab * CCKD_L1ENT_SIZE))
        {
            fprintf (stderr, MSG(HHC00404, "E", 0, 0, fname,
                                 "write()", errno ? strerror(errno) : "incomplete"));
            return -1;
        }

        /* Create the secondary lookup table */
        memset (&l2, 0, CCKD_L2TAB_SIZE);

        /* Write the seondary lookup table */
        rc = write (fd, &l2, CCKD_L2TAB_SIZE);
        if (rc < (int)CCKD_L2TAB_SIZE)
        {
            fprintf (stderr, MSG(HHC00404, "E", 0, 0, fname,
                                 "write()", errno ? strerror(errno) : "incomplete"));
            return -1;
        }

        cpos = l1[0] + CCKD_L2TAB_SIZE;
    }

    if (!dasdcopy)
    {
        /* Write each cylinder */
        for (cyl = start; cyl <= end; cyl++)
        {
            /* Display progress message every 10 cylinders */
            if (cyl && !(cyl % 10))
            {
#ifdef EXTERNALGUI
                if (extgui)
                    fprintf (stderr, "CYL=%u\n", cyl);
                else
#endif /*EXTERNALGUI*/
                fprintf (stderr, "Writing cylinder %u\r", cyl);
            }

            for (head = 0; head < heads; head++)
            {
                /* Clear the track to zeroes */
                memset (buf, 0, trksize);

                /* Build the track header */
                trkhdr = (CKDDASD_TRKHDR*)buf;
                trkhdr->bin = 0;
                store_hw(&trkhdr->cyl, cyl);
                store_hw(&trkhdr->head, head);
                pos = buf + CKDDASD_TRKHDR_SIZE;

                /* Build record zero */
                r = 0;
                rechdr = (CKDDASD_RECHDR*)pos;
                pos += CKDDASD_RECHDR_SIZE;
                store_hw(&rechdr->cyl, cyl);
                store_hw(&rechdr->head, head);
                rechdr->rec = r;
                rechdr->klen = 0;
                store_hw(&rechdr->dlen, rec0len);
                pos += rec0len;
                r++;

                /* Track 0 contains IPL records and volume label */
                if (!rawflag && fseqn == 1 && trk == 0)
                {
                    /* Build the IPL1 record */
                    rechdr = (CKDDASD_RECHDR*)pos;
                    pos += CKDDASD_RECHDR_SIZE;

                    store_hw(&rechdr->cyl, cyl);
                    store_hw(&rechdr->head, head);
                    rechdr->rec = r;
                    rechdr->klen = keylen;
                    store_hw(&rechdr->dlen, ipl1len);
                    r++;

                    convert_to_ebcdic (pos, keylen, "IPL1");
                    pos += keylen;

                    /* Copy model IPL PSW and CCWs */
                    memcpy (pos, iplpsw, 8);
                    memcpy (pos+8, iplccw1, 8);
                    memcpy (pos+16, iplccw2, 8);

                    /* Set EC mode flag in wait PSW if requested */
                    if (flagECmode)
                    {
                        *(pos+1) = 0x08 | *(pos+1);
                    }

                    /* Set machine-check-enabled mask in PSW if requested */
                    if (flagMachinecheck)
                    {
                        *(pos+1) = 0x04 | *(pos+1);
                    }

                    pos += ipl1len;

                    /* Build the IPL2 record */
                    rechdr = (CKDDASD_RECHDR*)pos;
                    pos += CKDDASD_RECHDR_SIZE;

                    store_hw(&rechdr->cyl, cyl);
                    store_hw(&rechdr->head, head);
                    rechdr->rec = r;
                    rechdr->klen = keylen;
                    store_hw(&rechdr->dlen, ipl2len);
                    r++;

                    convert_to_ebcdic (pos, keylen, "IPL2");
                    pos += keylen;

                    pos += ipl2len;

                    /* Build the VOL1 record */
                    rechdr = (CKDDASD_RECHDR*)pos;
                    pos += CKDDASD_RECHDR_SIZE;

                    store_hw(&rechdr->cyl, cyl);
                    store_hw(&rechdr->head, head);
                    rechdr->rec = r;
                    rechdr->klen = keylen;
                    store_hw(&rechdr->dlen, vol1len);
                    r++;

                    convert_to_ebcdic (pos, keylen, "VOL1");
                    pos += keylen;

                    convert_to_ebcdic (pos, 4, "VOL1");             //VOL1
                    convert_to_ebcdic (pos+4, 6, volser);           //volser
                    pos[10] = 0x40;                                 //security
                    store_hw(pos+11,0);                             //vtoc CC
                    store_hw(pos+13,1);                             //vtoc HH
                    pos[15] = 0x01;                                 //vtoc R
                    memset(pos+16, 0x40, 21);                       //reserved
                    convert_to_ebcdic (pos+37, 14, "    HERCULES"); //ownerid
                    memset(pos+51, 0x40, 29);                       //reserved
                    pos += vol1len;

                    /* 9 4096 data blocks for linux volume */
                    if (nullfmt == CKDDASD_NULLTRK_FMT2)
                    {
                        for (i = 0; i < 9; i++)
                        {
                            rechdr = (CKDDASD_RECHDR*)pos;
                            pos += CKDDASD_RECHDR_SIZE;

                            store_hw(&rechdr->cyl, cyl);
                            store_hw(&rechdr->head, head);
                            rechdr->rec = r;
                            rechdr->klen = 0;
                            store_hw(&rechdr->dlen, 4096);
                            pos += 4096;
                            r++;
                        }
                    }
                } /* end if(trk == 0) */

                /* Track 1 for linux contains an empty VTOC */
                else if (fseqn == 1 && trk == 1 && nullfmt == CKDDASD_NULLTRK_FMT2)
                {
                    /* build format 4 dscb */
                    rechdr = (CKDDASD_RECHDR*)pos;
                    pos += CKDDASD_RECHDR_SIZE;

                    /* track 1 record 1 count */
                    store_hw(&rechdr->cyl, cyl);
                    store_hw(&rechdr->head, head);
                    rechdr->rec = r;
                    rechdr->klen = 44;
                    store_hw(&rechdr->dlen, 96);
                    r++;

                    /* track 1 record 1 key */
                    memset (pos, 0x04, 44);
                    pos += 44;

                    /* track 1 record 1 data */
                    memset (pos, 0, 96);
                    pos[0] = 0xf4;                            // DS4IDFMT
                    store_hw(pos + 6, 10);                    // DS4DSREC
                    pos[14] = trks > 65535 ? 0xa0 : 0;        // DS4VTOCI
                    pos[15] = 1;                              // DS4NOEXT
                    store_hw(pos+18, volcyls);                // DS4DSCYL
                    store_hw(pos+20, heads);                  // DS4DSTRK
                    store_hw(pos+22, ckdtab->len);            // DS4DEVTK
                    pos[27] = 0x30;                           // DS4DEVFG
                    pos[30] = 0x0c;                           // DS4DEVDT
                    pos[61] = 0x01;                           // DS4VTOCE + 00
                    pos[66] = 0x01;                           // DS4VTOCE + 05
                    pos[70] = 0x01;                           // DS4VTOCE + 09
                    pos[81] = trks > 65535 ? 7 : 0;           // DS4EFLVL
                    pos[85] = trks > 65535 ? 1 : 0;           // DS4EFPTR + 03
                    pos[86] = trks > 65535 ? 3 : 0;           // DS4EFPTR + 04
                    pos += 96;

                    /* build format 5 dscb */
                    rechdr = (CKDDASD_RECHDR*)pos;
                    pos += CKDDASD_RECHDR_SIZE;

                    /* track 1 record 1 count */
                    store_hw(&rechdr->cyl, cyl);
                    store_hw(&rechdr->head, head);
                    rechdr->rec = r;
                    rechdr->klen = 44;
                    store_hw(&rechdr->dlen, 96);
                    r++;

                    /* track 1 record 2 key */
                    memset (pos, 0x05, 4);                    // DS5KEYID
                    memset (pos+4, 0, 40);
                    if (trks <= 65535)
                    {
                        store_hw(pos+4, 2);                   // DS5AVEXT + 00
                        store_hw(pos+6, volcyls - 1);         // DS5AVEXT + 02
                        pos[8] = heads - 2;                   // DS5AVEXT + 04
                    }
                    pos += 44;

                    /* track 1 record 2 data */
                    memset (pos, 0, 96);
                    pos[0] = 0xf5;                            // DS5FMTID
                    pos += 96;

                    /* build format 7 dscb */
                    if (trks > 65535)
                    {
                        rechdr = (CKDDASD_RECHDR*)pos;
                        pos += CKDDASD_RECHDR_SIZE;

                        /* track 1 record 3 count */
                        store_hw(&rechdr->cyl, cyl);
                        store_hw(&rechdr->head, head);
                        rechdr->rec = r;
                        rechdr->klen = 44;
                        store_hw(&rechdr->dlen, 96);
                        r++;

                        /* track 1 record 2 key */
                        memset (pos, 0x07, 4);                // DS7KEYID
                        memset (pos+4, 0, 40);
                        store_fw(pos+4, 2);                   // DS7EXTNT + 00
                        store_fw(pos+8, trks - 1);            // DS7EXTNT + 04
                        pos += 44;

                        /* track 1 record 2 data */
                        memset (pos, 0, 96);
                        pos[0] = 0xf7;                        // DS7FMTID
                        pos += 96;
                    }

                    n = 12 - r + 1;
                    for (i = 0; i < n; i++)
                    {
                        rechdr = (CKDDASD_RECHDR*)pos;
                        pos += CKDDASD_RECHDR_SIZE;

                        store_hw(&rechdr->cyl, cyl);
                        store_hw(&rechdr->head, head);
                        rechdr->rec = r;
                        rechdr->klen = 44;
                        store_hw(&rechdr->dlen, 96);
                        pos += 140;
                        r++;
                    }
                }

                /* Specific null track formatting */
                else if (nullfmt == CKDDASD_NULLTRK_FMT0)
                {
                    rechdr = (CKDDASD_RECHDR*)pos;
                    pos += CKDDASD_RECHDR_SIZE;

                    store_hw(&rechdr->cyl, cyl);
                    store_hw(&rechdr->head, head);
                    rechdr->rec = r;
                    rechdr->klen = 0;
                    store_hw(&rechdr->dlen, 0);
                    r++;
                }
                else if (nullfmt == CKDDASD_NULLTRK_FMT2)
                {
                    /* Other linux tracks have 12 4096 data records */
                    for (i = 0; i < 12; i++)
                    {
                        rechdr = (CKDDASD_RECHDR*)pos;
                        pos += CKDDASD_RECHDR_SIZE;
                        store_hw(&rechdr->cyl, cyl);
                        store_hw(&rechdr->head, head);
                        rechdr->rec = r;
                        rechdr->klen = 0;
                        store_hw(&rechdr->dlen, 4096);
                        pos += 4096;
                        r++;
                    }
                }

                /* End-of-track marker */
                memcpy (pos, eighthexFF, 8);
                pos += 8;

                /* Calculate length to write */
                if (comp == 0xff)
                    len = (int)trksize;
                else
                {
                    len = (int)(pos - buf);
                    l2[trk].pos = cpos;
                    l2[trk].len = l2[trk].size = len;
                    cpos += len;
                }

                /* Write the track to the file */
                rc = write (fd, buf, len);
                if (rc != (int)len)
                {
                    fprintf (stderr, MSG(HHC00404, "E", 0, 0, fname,
                                 "write()", errno ? strerror(errno) : "incomplete"));
                    return -1;
                }

                /* Exit if compressed disk and current track is 1 */
                if (comp != 0xff && trk == 1) break;

                trk++;

            } /* end for(head) */

            /* Exit if compressed disk */
            if (comp != 0xff) break;

        } /* end for(cyl) */

    } /* `dasdcopy' bit is off */
    else
        cyl = end + 1;

    /* Complete building the compressed file */
    if (comp != 0xff)
    {
        cdevhdr.size = cdevhdr.used = cpos;

        /* Rewrite the compressed device header */
        rcoff = lseek (fd, CKDDASD_DEVHDR_SIZE, SEEK_SET);
        if (rcoff == -1)
        {
            fprintf (stderr, MSG(HHC00404, "E", 0, 0, fname,
                                 "lseek()", strerror(errno)));
            return -1;
        }
        rc = write (fd, &cdevhdr, CCKDDASD_DEVHDR_SIZE);
        if (rc < (int)CCKDDASD_DEVHDR_SIZE)
        {
          fprintf (stderr, MSG(HHC00404, "E", 0, 0, fname,
                                 "write()", errno ? strerror(errno) : "incomplete"));
            return -1;
        }

        /* Rewrite the secondary lookup table */
        rcoff = lseek (fd, (off_t)l1[0], SEEK_SET);
        if (rcoff == -1)
        {
            fprintf (stderr, MSG(HHC00404, "E", 0, 0, fname,
                                 "lseek()", strerror(errno)));
            return -1;
        }
        rc = write (fd, &l2, CCKD_L2TAB_SIZE);
        if (rc < (int)CCKD_L2TAB_SIZE)
        {
          fprintf (stderr, MSG(HHC00404, "E", 0, 0, fname,
                                 "write()", errno ? strerror(errno) : "incomplete"));
            return -1;
        }
        rc = ftruncate(fd, (off_t)cdevhdr.size);

        free (l1);
        cyl = volcyls;
    }

    /* Close the DASD image file */
    rc = close (fd);
    if (rc < 0)
    {
        fprintf (stderr, MSG(HHC00404, "E", 0, 0, fname,
                                 "close()", strerror(errno)));
        return -1;
    }

    /* Display completion message */
    fprintf (stderr, MSG(HHC00460, "I", 0, 0, fname,
             cyl - start, "cylinders"));
    return 0;

} /* end function create_ckd_file */

/*-------------------------------------------------------------------*/
/* Subroutine to create a CKD DASD image                             */
/* Input:                                                            */
/*      fname    DASD image file name                                */
/*      devtype  Device type                                         */
/*      heads    Number of heads per cylinder                        */
/*      maxdlen  Maximum R1 record data length                       */
/*      volcyls  Total number of cylinders on volume                 */
/*      volser   Volume serial number                                */
/*      comp     Compression algorithm for a compressed device.      */
/*               Will be 0xff if device is not to be compressed.     */
/*      lfs      build large (uncompressed) file (if supported)      */
/*      dasdcopy xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx      */
/*      nullfmt  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx      */
/*      rawflag  create raw image (skip special track 0 handling)    */
/*      flagECmode        1  set EC mode bit in wait PSW             */
/*                        0  don't set EC mode bit in wait PSW       */
/*      flagMachinecheck  1  set machine-check-enabled flag          */
/*                           in wait PSW                             */
/*                        0  don't set machine-check-enabled flag    */
/*                           in wait PSW                             */
/*                                                                   */
/* If the total number of cylinders exceeds the capacity of a 2GB    */
/* file, then multiple CKD image files will be created, with the     */
/* suffix _1, _2, etc suffixed to the specified file name.           */
/* Otherwise a single file is created without a suffix.              */
/*-------------------------------------------------------------------*/
DLL_EXPORT int
create_ckd (char *fname, U16 devtype, U32 heads, U32 maxdlen,
           U32 volcyls, char *volser, BYTE comp, int lfs, int dasdcopy,
           int nullfmt, int rawflag,
           int flagECmode, int flagMachinecheck)
{
int             i;                      /* Array subscript           */
int             rc;                     /* Return code               */
char            *s;                     /* String pointer            */
u_int           fileseq;                /* File sequence number      */
char            sfname[FILENAME_MAX];   /* Suffixed name of this file*/
char            *suffix;                /* -> Suffix character       */
U32             endcyl;                 /* Last cylinder of this file*/
U32             cyl;                    /* Cylinder number           */
U32             cylsize;                /* Cylinder size in bytes    */
BYTE           *buf;                    /* -> Track data buffer      */
U32             mincyls;                /* Minimum cylinder count    */
U32             maxcyls;                /* Maximum cylinder count    */
U32             maxcpif;                /* Maximum number of cylinders
                                           in each CKD image file    */
u_int           rec0len = 8;            /* Length of R0 data         */
U32             trksize;                /* DASD image track length   */

    /* Compute the DASD image track length */
    trksize = sizeof(CKDDASD_TRKHDR)
                + sizeof(CKDDASD_RECHDR) + rec0len
                + sizeof(CKDDASD_RECHDR) + maxdlen
                + sizeof(eighthexFF);
    trksize = ROUND_UP(trksize,512);

    /* Compute minimum and maximum number of cylinders */
    cylsize = trksize * heads;
    mincyls = 1;
    if (comp == 0xff && !lfs)
    {
        maxcpif = (0x7fffffff - CKDDASD_DEVHDR_SIZE + 1) / cylsize;
        maxcyls = maxcpif * CKD_MAXFILES;
    }
    else
        maxcpif = maxcyls = volcyls;

    if (maxcyls > 65536)
    {
        maxcyls = 65536;
        fprintf( stderr, MSG(HHC00467, "I", "cylinders", maxcyls) );
    }

    /* Check for valid number of cylinders */
    if (volcyls < mincyls || volcyls > maxcyls)
    {
        if (comp == 0xff && !lfs)
        {
            char    msgbuf[128];

#if defined(HAVE_LIBZ) && defined(CCKD_BZIP2)
            char   *pszcomp     = " or zlib/bzip2 compression";
#elif defined(HAVE_LIBZ)
            char   *pszcomp     = " or zlib compression";
#elif defined(CCKD_BZIP2)
            char   *pszcomp     = " or bzip2 compression";
#else
            char   *pszcomp     = "";
#endif
            char   *pszopt;

            fprintf( stderr, MSG(HHC00466, "I", maxcyls, "cylinders", CKD_MAXFILES) );

            if ( strlen(pszcomp) > 0 )
                pszopt = "related options";
            else
                pszopt = "option";

            MSGBUF(msgbuf, "-lfs%s %s", pszcomp, pszopt );
            fprintf( stderr, MSG(HHC00468, "I", msgbuf) );
        }

        fprintf (stderr, MSG(HHC00461, "E", 0, 0, fname,
                "cylinder", volcyls, mincyls, maxcyls));
        return -1;
    }

    /* Obtain track data buffer */
    buf = malloc(trksize);
    if (buf == NULL)
    {
        char buf[40];
        MSGBUF( buf, "malloc(%u)", trksize);
        fprintf (stderr, MSG(HHC00404, "E", 0, 0, fname,
                buf, strerror(errno)));
        return -1;
    }

    /* Display progress message */
    fprintf( stderr, MSG(HHC00462, "I", 0, 0, fname,
            devtype, rawflag ? "" : volser, volcyls, heads, trksize));

    /* Copy the unsuffixed DASD image file name */
    strlcpy( sfname, fname, sizeof(sfname) );
    suffix = NULL;

    /* Create the suffixed file name if volume will exceed 2GB */
    if (volcyls > maxcpif)
    {
        /* Look for last slash marking end of directory name */
        s = strrchr( fname, PATHSEPC);
        if (s == NULL) s = fname;

        /* Insert suffix before first dot in file name, or
           append suffix to file name if there is no dot.
           If the filename already has a place for the suffix
           then use that. */
        s = strchr (s, '.');
        if (s != NULL)
        {
            i = s - fname;
            if (i > 2 && fname[i-2] == '_')
                suffix = sfname + i - 1;
            else
            {
                strlcpy (sfname + i, "_1", sizeof(sfname)-(size_t)i );
                strlcat (sfname, fname + i, sizeof(sfname) );
                suffix = sfname + i + 1;
            }
        }
        else
        {
            if (strlen(sfname) < 2 || sfname[strlen(sfname)-2] == '_')
                strlcat( sfname, "_1", sizeof(sfname) );
            suffix = sfname + strlen(sfname) - 1;
        }
    }

    /* Create the DASD image files */
    for (cyl = 0, fileseq = 1; cyl < volcyls;
            cyl += maxcpif, fileseq++)
    {
        /* Insert the file sequence number in the file name */
        if (suffix)
        {
            if ( fileseq <= 9 )
                *suffix = '0' + fileseq;
            else
                *suffix = 'A' - 10 + fileseq;
        }

        /* Calculate the ending cylinder for this file */
        if (cyl + maxcpif < volcyls)
            endcyl = cyl + maxcpif - 1;
        else
            endcyl = volcyls - 1;

        /* Create a CKD DASD image file */
        rc = create_ckd_file (sfname, fileseq, devtype, heads,
                    trksize, buf, cyl, endcyl, volcyls, volser,
                    comp, dasdcopy, nullfmt, rawflag,
                    flagECmode, flagMachinecheck);
        if (rc < 0) return -1;
    }

    /* Release data buffer */
    free (buf);

    return 0;
} /* end function create_ckd */

/*-------------------------------------------------------------------*/
/* Subroutine to create an FBA DASD image file                       */
/* Input:                                                            */
/*      fname    DASD image file name                                */
/*      devtype  Device type                                         */
/*      sectsz   Sector size                                         */
/*      sectors  Number of sectors                                   */
/*      volser   Volume serial number                                */
/*      comp     Compression algorithm for a compressed device.      */
/*               Will be 0xff if device is not to be compressed.     */
/*      lfs      build large (uncompressed) file (if supported)      */
/*      dasdcopy xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx      */
/*      rawflag  create raw image (skip sector 1 VOL1 processing)    */
/*-------------------------------------------------------------------*/
DLL_EXPORT int
create_fba (char *fname, U16 devtype, U32 sectsz, U32 sectors,
            char *volser, BYTE comp, int lfs, int dasdcopy, int rawflag)
{
int             rc;                     /* Return code               */
int             fd;                     /* File descriptor           */
U32             sectnum;                /* Sector number             */
BYTE           *buf;                    /* -> Sector data buffer     */
U32             minsect;                /* Minimum sector count      */
U32             maxsect;                /* Maximum sector count      */
int             x=O_EXCL;               /* Open option               */
char            pathname[MAX_PATH];     /* file path in host format  */

    /* Special processing for compressed fba */
    if (comp != 0xff)
    {
        rc = create_compressed_fba (fname, devtype, sectsz, sectors,
                                    volser, comp, lfs, dasdcopy, rawflag);
        return rc;
    }

    /* Compute minimum and maximum number of sectors */
    minsect = 64;
    maxsect = 0x80000000 / sectsz;

    /* Check for valid number of sectors */
    if (sectors < minsect || (!lfs && sectors > maxsect))
    {
        if (!lfs)
            fprintf( stderr, MSG(HHC00521, "I", maxsect, "sectors") );

        fprintf (stderr, MSG(HHC00461, "E", 0, 0, fname,
                "sector", sectors, minsect, maxsect));
        return -1;
    }

    /* Obtain sector data buffer */
    buf = malloc(sectsz);
    if (buf == NULL)
    {
        char buf[40];
        MSGBUF( buf, "malloc(%u)", sectsz);
        fprintf (stderr, MSG(HHC00404, "E", 0, 0, fname,
                buf, strerror(errno)));
        return -1;
    }

    /* Display progress message */
    fprintf (stderr, MSG(HHC00463, "I", 0, 0, fname,
            devtype, rawflag ? "" : volser, sectors, sectsz));

    /* if `dasdcopy' > 1 then we can replace the existing file */
    if (dasdcopy > 1) x = 0;

    /* Create the DASD image file */
    hostpath(pathname, fname, sizeof(pathname));
    fd = HOPEN (pathname, O_WRONLY | O_CREAT | x | O_BINARY,
                S_IRUSR | S_IWUSR | S_IRGRP);
    if (fd < 0)
    {
        fprintf (stderr, MSG(HHC00404, "E", 0, 0, fname,
                "open()", strerror(errno)));
        return -1;
    }

    /* If the `dasdcopy' bit is on then simply allocate the space */
    if (dasdcopy)
    {
        off_t sz = sectors * sectsz;
        rc = ftruncate (fd, sz);
        if (rc < 0)
        {
            fprintf (stderr, MSG(HHC00404, "E", 0, 0, fname,
                "ftruncate()", strerror(errno)));
            return -1;
        }
    }
    /* Write each sector */
    else
    {
        for (sectnum = 0; sectnum < sectors; sectnum++)
        {
            /* Clear the sector to zeroes */
            memset (buf, 0, sectsz);

            /* Sector 1 contains the volume label */
            if (!rawflag && sectnum == 1)
            {
                convert_to_ebcdic (buf, 4, "VOL1");
                convert_to_ebcdic (buf+4, 6, volser);
            } /* end if(sectnum==1) */

            /* Display progress message every 100 sectors */
            if ((sectnum % 100) == 0)
#ifdef EXTERNALGUI
            {
                if (extgui) fprintf (stderr, "BLK=%u\n", sectnum);
                else fprintf (stderr, "Writing sector %u\r", sectnum);
            }
#else /*!EXTERNALGUI*/
            fprintf (stderr, "Writing sector %u\r", sectnum);
#endif /*EXTERNALGUI*/

            /* Write the sector to the file */
            rc = write (fd, buf, sectsz);
            if (rc < (int)sectsz)
            {
                fprintf (stderr, MSG(HHC00404, "E", 0, 0, fname,
                         "write()", errno ? strerror(errno) : "incomplete"));
                return -1;
            }
        } /* end for(sectnum) */
    } /* `dasdcopy' bit is off */

    /* Close the DASD image file */
    rc = close (fd);
    if (rc < 0)
    {
        fprintf (stderr, MSG(HHC00404, "E", 0, 0, fname,
                                 "close()", strerror(errno)));
        return -1;
    }

    /* Release data buffer */
    free (buf);

    /* Display completion message */
    fprintf (stderr, MSG(HHC00460, "I", 0, 0, fname, sectors, "sectors"));

    return 0;
} /* end function create_fba */

/*-------------------------------------------------------------------*/
/* Subroutine to create a compressed FBA DASD image file             */
/* Input:                                                            */
/*      fname    DASD image file name                                */
/*      devtype  Device type                                         */
/*      sectsz   Sector size                                         */
/*      sectors  Number of sectors                                   */
/*      volser   Volume serial number                                */
/*      comp     Compression algorithm for a compressed device.      */
/*      lfs      build large (uncompressed) file (if supported)      */
/*      dasdcopy xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx      */
/*      rawflag  create raw image (skip sector 1 VOL1 processing)    */
/*-------------------------------------------------------------------*/
int
create_compressed_fba (char *fname, U16 devtype, U32 sectsz,
           U32 sectors, char *volser, BYTE comp, int lfs, int dasdcopy,
           int rawflag)
{
int             rc;                     /* Return code               */
off_t           rcoff;                  /* Return value from lseek() */
int             fd;                     /* File descriptor           */
CKDDASD_DEVHDR  devhdr;                 /* Device header             */
CCKDDASD_DEVHDR cdevhdr;                /* Compressed device header  */
int             blkgrps;                /* Number block groups       */
int             numl1tab, l1tabsz;      /* Level 1 entries, size     */
CCKD_L1ENT     *l1;                     /* Level 1 table pointer     */
CCKD_L2ENT      l2[256];                /* Level 2 table             */
unsigned long   len2;                   /* Compressed buffer length  */
BYTE            buf2[256];              /* Compressed buffer         */
BYTE            buf[65536];             /* Buffer                    */
int             x=O_EXCL;               /* Open option               */
char            pathname[MAX_PATH];     /* file path in host format  */

    UNREFERENCED(lfs);

    /* Calculate the size of the level 1 table */
    blkgrps = (sectors / CFBA_BLOCK_NUM) + 1;
    numl1tab = (blkgrps + 255) / 256;
    l1tabsz = numl1tab * CCKD_L1ENT_SIZE;
    if (l1tabsz > 65536)
    {
        fprintf (stderr, MSG(HHC00464, "E", 0, 0, fname,
                 (U64)(sectors * sectsz), numl1tab));
        return -1;
    }

    /* if `dasdcopy' > 1 then we can replace the existing file */
    if (dasdcopy > 1) x = 0;

    /* Create the DASD image file */
    hostpath(pathname, fname, sizeof(pathname));
    fd = HOPEN (pathname, O_WRONLY | O_CREAT | x | O_BINARY,
                S_IRUSR | S_IWUSR | S_IRGRP);
    if (fd < 0)
    {
        fprintf (stderr, MSG(HHC00404, "E", 0, 0, fname,
                "open()", strerror(errno)));
        return -1;
    }

    /* Display progress message */
    fprintf (stderr, MSG(HHC00465, "I", 0, 0, fname,
            devtype, rawflag ? "" : volser, sectors, sectsz));

    /* Write the device header */
    memset (&devhdr, 0, CKDDASD_DEVHDR_SIZE);
    memcpy (&devhdr.devid, "FBA_C370", 8);
    rc = write (fd, &devhdr, CKDDASD_DEVHDR_SIZE);
    if (rc < (int)CKDDASD_DEVHDR_SIZE)
    {
        fprintf (stderr, MSG(HHC00404, "E", 0, 0, fname,
                         "write()", errno ? strerror(errno) : "incomplete"));
        return -1;
    }

    /* Write the compressed device header */
    memset (&cdevhdr, 0, CCKDDASD_DEVHDR_SIZE);
    cdevhdr.vrm[0] = CCKD_VERSION;
    cdevhdr.vrm[1] = CCKD_RELEASE;
    cdevhdr.vrm[2] = CCKD_MODLVL;
    if (cckd_endian())  cdevhdr.options |= CCKD_BIGENDIAN;
    cdevhdr.options |= CCKD_ORDWR;
    cdevhdr.numl1tab = numl1tab;
    cdevhdr.numl2tab = 256;
    cdevhdr.cyls[3] = (sectors >> 24) & 0xFF;
    cdevhdr.cyls[2] = (sectors >> 16) & 0xFF;
    cdevhdr.cyls[1] = (sectors >>    8) & 0xFF;
    cdevhdr.cyls[0] = sectors & 0xFF;
    cdevhdr.compress = comp;
    cdevhdr.compress_parm = -1;
    rc = write (fd, &cdevhdr, CCKDDASD_DEVHDR_SIZE);
    if (rc < (int)CCKDDASD_DEVHDR_SIZE)
    {
        fprintf (stderr, MSG(HHC00404, "E", 0, 0, fname,
                         "write()", errno ? strerror(errno) : "incomplete"));
        return -1;
    }

    /* Write the level 1 table */
    l1 = (CCKD_L1ENT *)&buf;
    memset (l1, 0, l1tabsz);
    l1[0] = CKDDASD_DEVHDR_SIZE + CCKDDASD_DEVHDR_SIZE + l1tabsz;
    rc = write (fd, l1, l1tabsz);
    if (rc < l1tabsz)
    {
        fprintf (stderr, MSG(HHC00404, "E", 0, 0, fname,
                         "write()", errno ? strerror(errno) : "incomplete"));
        return -1;
    }

    /* Write the 1st level 2 table */
    memset (&l2, 0, CCKD_L2TAB_SIZE);
    l2[0].pos = CKDDASD_DEVHDR_SIZE + CCKDDASD_DEVHDR_SIZE + l1tabsz +
                CCKD_L2TAB_SIZE;
    rc = write (fd, &l2, CCKD_L2TAB_SIZE);
    if (rc < (int)CCKD_L2TAB_SIZE)
    {
        fprintf (stderr, MSG(HHC00404, "E", 0, 0, fname,
                         "write()", errno ? strerror(errno) : "incomplete"));
        return -1;
    }

    /* Write the 1st block group */
    memset (&buf, 0, CKDDASD_DEVHDR_SIZE + CFBA_BLOCK_SIZE);
    if (!rawflag)
    {
        convert_to_ebcdic (&buf[CKDDASD_TRKHDR_SIZE+sectsz], 4, "VOL1");
        convert_to_ebcdic (&buf[CKDDASD_TRKHDR_SIZE+sectsz+4], 6, volser);
    }
    len2 = sizeof(buf2);
#ifdef HAVE_LIBZ
    rc = compress2 (&buf2[0], &len2, &buf[CKDDASD_TRKHDR_SIZE],
                    CFBA_BLOCK_SIZE, -1);
    if (comp && rc == Z_OK)
    {
        buf[0] = CCKD_COMPRESS_ZLIB;
        rc = write (fd, &buf, CKDDASD_TRKHDR_SIZE);
        if (rc < (int)CKDDASD_TRKHDR_SIZE)
        {
            fprintf (stderr, MSG(HHC00404, "E", 0, 0, fname,
                         "write()", errno ? strerror(errno) : "incomplete"));
            return -1;
        }
        rc = write (fd, &buf2, len2);
        if (rc < (int)len2)
        {
            fprintf (stderr, MSG(HHC00404, "E", 0, 0, fname,
                         "write()", errno ? strerror(errno) : "incomplete"));
            return -1;
        }
        l2[0].len = l2[0].size = CKDDASD_TRKHDR_SIZE + len2;
        cdevhdr.size = cdevhdr.used = CKDDASD_DEVHDR_SIZE +
                       CCKDDASD_DEVHDR_SIZE + l1tabsz + CCKD_L2TAB_SIZE +
                       CKDDASD_TRKHDR_SIZE + len2;
    }
    else
#endif // defined(HAVE_LIBZ)
    {
        rc = write (fd, &buf, CKDDASD_TRKHDR_SIZE + CFBA_BLOCK_SIZE);
        if (rc < (int)(CKDDASD_TRKHDR_SIZE + CFBA_BLOCK_SIZE))
        {
            fprintf (stderr, MSG(HHC00404, "E", 0, 0, fname,
                         "write()", errno ? strerror(errno) : "incomplete"));
            return -1;
        }
        l2[0].len = l2[0].size = CKDDASD_TRKHDR_SIZE + CFBA_BLOCK_SIZE;
        cdevhdr.size = cdevhdr.used = CKDDASD_DEVHDR_SIZE +
                       CCKDDASD_DEVHDR_SIZE + l1tabsz + CCKD_L2TAB_SIZE +
                       CKDDASD_TRKHDR_SIZE + CFBA_BLOCK_SIZE;
    }

    /* Re-write the compressed device header */
    rcoff = lseek (fd, CKDDASD_DEVHDR_SIZE, SEEK_SET);
    if (rcoff < 0)
    {
        fprintf (stderr, MSG(HHC00404, "E", 0, 0, fname,
                         "lseek()", strerror(errno)));
        return -1;
    }
    rc = write (fd, &cdevhdr, CCKDDASD_DEVHDR_SIZE);
    if (rc < (int)CCKDDASD_DEVHDR_SIZE)
    {
        fprintf (stderr, MSG(HHC00404, "E", 0, 0, fname,
                         "write()", errno ? strerror(errno) : "incomplete"));
        return -1;
    }

    /* Re-write the 1st level 2 table */
    rcoff = lseek (fd, CKDDASD_DEVHDR_SIZE + CCKDDASD_DEVHDR_SIZE + l1tabsz, SEEK_SET);
    if (rcoff < 0)
    {
      fprintf (stderr, MSG(HHC00404, "E", 0, 0, fname,
                         "lseek()", strerror(errno)));
        return -1;
    }
    rc = write (fd, &l2, CCKD_L2TAB_SIZE);
    if (rc < (int)CCKD_L2TAB_SIZE)
    {
        fprintf (stderr, MSG(HHC00404, "E", 0, 0, fname,
                         "write()", errno ? strerror(errno) : "incomplete"));
        return -1;
    }

    /* Close the DASD image file */
    rc = close (fd);
    if (rc < 0)
    {
      fprintf (stderr, MSG(HHC00404, "E", 0, 0, fname,
                         "close()", strerror(errno)));
        return -1;
    }

    /* Display completion message */
    fprintf (stderr, MSG(HHC00460, "I", 0, 0, fname,
                         sectors, "sectors"));
    return 0;
} /* end function create_compressed_fba */


int get_verbose_util(void)
{
    return verbose;
}

DLL_EXPORT void set_verbose_util(int v)
{
    verbose = v;
}

DLL_EXPORT int valid_dsname( const char *pszdsname )
{
    int i;
    int iLen = (int)strlen(pszdsname);

    if ( iLen > 44 || iLen == 0 ) return FALSE;

    for ( i = 0; i < iLen; i++ )
    {
        BYTE c = pszdsname[i];
        if ( isalnum( c ) )
            continue;
        else if ( c == '$' )
            continue;
        else if ( c == '@' )
            continue;
        else if ( c == '#' )
            continue;
        else if ( c == '-' )
            continue;
        else if ( c == '.' )
            continue;
        else if ( c == '{' )
            continue;
        else if ( i > 1 && c == '\0' )
            break;
        else
            return FALSE;
    }
    return TRUE;
}
