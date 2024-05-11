/* DASDCAT.C    (C) Copyright Roger Bowler, 1999-2012                */
/*              DASD "cat" functions                                 */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*
 * dasdcat
 *
 * Vast swathes copied from dasdpdsu.c (C) Copyright Roger Bowler, 1999-2009
 * Changes and additions Copyright 2000-2009 by Malcolm Beattie
 *
 *
 *
 */

#include "hstdinc.h"
#include "hercules.h"
#include "dasdblks.h"

#ifdef WIN32
#include <io.h> // (setmode)
#endif

#define UTILITY_NAME    "dasdcat"
#define UTILITY_DESC    "DASD cat program"

/* Option flags */
#define OPT_ASCIIFY         0x01
#define OPT_CARDS           0x02
#define OPT_PDS_WILDCARD    0x04
#define OPT_PDS_LISTONLY    0x08
#define OPT_SEQNO           0x10
#define OPT_MEMINFO_ONLY    0x20

/* function prototypes */
int do_cat          (CIFBLK *cif, char *file);
int get_volser      (CIFBLK *cif);
int do_cat_cards    (BYTE *buf, int len, unsigned long optflags);
int do_cat_pdsmember(CIFBLK *cif, DSXTENT *extent, int noext,
                     char* dsname, char *pdsmember, unsigned long optflags);
int process_member  (CIFBLK *cif, int noext, DSXTENT extent[],
                     BYTE *ttr, unsigned long optflags,
                     char* dsname, char* memname);
int process_dirblk  (CIFBLK *cif, int noext, DSXTENT extent[], BYTE *dirblk,
                     char* dsname, char *pdsmember, unsigned long optflags);

static int found = 0;
static char volser[7];

int main(int argc, char **argv)
{
 char           *pgm;                    /* less any extension (.ext) */
 int             rc = 0;
 CIFBLK         *cif = 0;
 char           *fn;
 char           *sfn;

    INITIALIZE_UTILITY( UTILITY_NAME, UTILITY_DESC, &pgm );

    if (argc < 2)
    {
        // "Usage: dasdcat..."
        WRMSG( HHC02405, "I", pgm );
        return 1;
    }

 /*
 * If your version of Hercules doesn't have support in its
 * dasdutil.c for turning off verbose messages, then remove
 * the following line but you'll have to live with chatty
 * progress output on stdout.
 */
 set_verbose_util(0);

 while (*++argv)
 {
     if (!strcmp(*argv, "-i"))
     {
         fn = *++argv;
         if (*(argv+1) && strlen (*(argv+1)) > 3 && !memcmp(*(argv+1), "sf=", 3))
             sfn = *++argv;
         else sfn = NULL;
         if (cif)
         {
             close_ckd_image(cif);
             cif = 0;
         }
         cif = open_ckd_image(fn, sfn, O_RDONLY | O_BINARY, IMAGE_OPEN_NORMAL);
         if (!cif)
         {
             // "Failed opening %s"
             FWRMSG( stderr, HHC02403, "E", *argv );
             rc = 1;
             break;
         }
     }
     else if (cif)
     {
         if ((rc = do_cat( cif, *argv )) != 0)
         {
             rc = 1;
             break;
         }
     }
 }

 if (cif)
 close_ckd_image(cif);

 return rc;
}

int do_cat_cards(BYTE *buf, int len, unsigned long optflags)
{
    if (len % 80 != 0)
    {
        // "Can't make 80 column card images from block length %d"
        FWRMSG( stderr, HHC02404, "E", len );
        return -1;
    }

    while (len)
    {
     char card[81];
     int srclen = (optflags & OPT_SEQNO) ? 80 : 72;
        make_asciiz(card, sizeof(card), buf, srclen);

        if (optflags & OPT_PDS_WILDCARD)
        {
            putchar('|');
            putchar(' ');
        }

        puts(card);
        len -= 80;
        buf += 80;
    }
    return 0;
}

int process_member(CIFBLK *cif, int noext, DSXTENT extent[],
                    BYTE *ttr, unsigned long optflags,
                    char* dsname, char* memname)
{
 int   rc;
 bool  memtitle;
 u_int trk;
 U16   len;
 U32   cyl;
 U8    head;
 U8    rec;
 BYTE *buf;

 U32   beg_cyl  = 0;
 U8    beg_head = 0;
 U8    beg_rec  = 0;
 U64   tot_len  = 0;

    trk = (ttr[0] << 8) | ttr[1];
    rec = ttr[2];

    if (optflags & OPT_CARDS)
        memtitle = false;

    while (1)
    {
        rc = convert_tt(trk, noext, extent, cif->heads, &cyl, &head);
        if (rc < 0)
            return -1;

        rc = read_block(cif, cyl, head, rec, 0, 0, &buf, &len);
        if (rc < 0)
            return -1;

        if (rc > 0)  /* end of track */
        {
            trk++;
            rec = 1;
            continue;
        }

        if (len == 0) /* end of member */
            break;

        if (optflags & OPT_MEMINFO_ONLY)
        {
            if (!beg_cyl)
            {
                beg_cyl  = cyl;
                beg_head = head;
                beg_rec  = rec;
            }
            tot_len += len;
        }
        else if (optflags & OPT_CARDS)
        {
            if (!memtitle)
            {
                memtitle = true;

                if (extgui || isatty( fileno( stdout )))
                {
                    char title[80];
                    MSGBUF( title, "                %s(%s):", dsname, memname );

                    puts("");
                    puts( title );
                    puts("");
                }
            }

            /* Formatted 72 or 80 column ASCII card images */
            if ((rc = do_cat_cards(buf, len, optflags)) != 0)
                return -1; // (len not multiple of 80 bytes)
        }
        else if (optflags & OPT_ASCIIFY)
        {
            /* Unformatted ASCII text */
            BYTE *p;
            for (p = buf; len--; p++)
                putchar(guest_to_host(*p));
        }
        else
        {
            /* Output member in binary exactly as-is */
#if O_BINARY != 0
            setmode(fileno(stdout),O_BINARY);
#endif
            fwrite(buf, len, 1, stdout);
        }

        rec++;
    }

    if (optflags & OPT_MEMINFO_ONLY)
    {
        char memsize[128];
        fmt_memsize( tot_len, memsize, sizeof( memsize ));

        // "%s/%s/%-8s %8s bytes from %4.4"PRIX32"%2.2"PRIX32"%2.2"PRIX32" to %4.4"PRIX32"%2.2"PRIX32"%2.2"PRIX32
        FWRMSG( stdout, HHC02407, "I",
            volser, dsname, memname, memsize,
            (U32) beg_cyl, (U32) beg_head, (U32) beg_rec,
            (U32)     cyl, (U32)     head, (U32)     rec );
    }

    return 0;
}

int process_dirblk(CIFBLK *cif, int noext, DSXTENT extent[], BYTE *dirblk,
 char* dsname, char *pdsmember, unsigned long optflags)
{
 int rc;
 int dirrem;
 char memname[9];
 static const BYTE endofdir[8] = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};

    /* Load number of bytes in directory block */
    dirrem = (dirblk[0] << 8) | dirblk[1];
    if (dirrem < 2 || dirrem > 256)
    {
        // "Directory block byte count is invalid"
        FWRMSG( stderr, HHC02400, "E" );
        return -1;
    }

    /* Point to first directory entry */
    dirblk += 2;
    dirrem -= 2;

    while (dirrem > 0)
    {
     PDSDIR *dirent = (PDSDIR*)dirblk;
     int k, size;

        /* Check for end of directory */
        if (memcmp( endofdir, dirent->pds2name, 8 ) == 0)
            return +1; /* (logical EOF) */

        /* Extract this member's name */
        make_asciiz(memname, sizeof(memname), dirent->pds2name, 8);

        if (optflags & OPT_PDS_LISTONLY)
        {
         char memname_lc[9];
            /* List just the member names in this PDS */
            memcpy(memname_lc, memname, sizeof(memname));
            string_to_lower(memname_lc);
            puts(memname_lc);
        }
        else
        {
            /* Are we interested in this specific member? */
            if (0
                || (optflags & OPT_PDS_WILDCARD)
                || strcmp( pdsmember, memname ) == 0
            )
            {
                if (optflags & OPT_PDS_WILDCARD && !(optflags & OPT_MEMINFO_ONLY))
                    printf("> Member %s\n", memname);
                else if (1
                    && !(optflags & OPT_MEMINFO_ONLY)
                    && !isatty( fileno( stdout ))
                )
                {
                    /* Delete any existing o/p file contents */
                    rewind( stdout );
                    if (ftruncate( fileno( stdout ), 0 ) < 0)
                    {
                        // "Error in function %s: %s"
                        FWRMSG( stderr, HHC02408, "E", "ftruncate", strerror( errno ));
                        return -1;
                    }
                }

                rc = process_member(cif, noext, extent, dirent->pds2ttrp, optflags, dsname, memname);
                if (rc < 0)
                    return -1;

                /* If not ALL members then we're done */
                if (!(optflags & OPT_PDS_WILDCARD))
                {
                    found = 1;
                    return +1; /* (logical EOF) */
                }
            }
        }

        /* Load the user data halfword count */
        k = dirent->pds2indc & PDS2INDC_LUSR;

        /* Point to next directory entry */
        size = 12 + k*2;
        dirblk += size;
        dirrem -= size;
    }
    return 0;
}

int do_cat_pdsmember(CIFBLK *cif, DSXTENT *extent, int noext,
                    char* dsname, char *pdsmember, unsigned long optflags)
{
 int   rc;
 u_int trk;
 U8    rec;

    /* Point to the start of the directory */
    trk = 0;
    rec = 1;

    /* Read the directory */
    while (1)
    {
     BYTE *blkptr;
     BYTE dirblk[256];
     U32 cyl;
     U8  head;
     U16 len;
        EXTGUIMSG( "CTRK=%d\n", trk );
        rc = convert_tt(trk, noext, extent, cif->heads, &cyl, &head);
        if (rc < 0)
            return -1;

        rc = read_block(cif, cyl, head, rec, 0, 0, &blkptr, &len);
        if (rc < 0)
            return -1;

        if (rc > 0)     /* end of track */
        {
            trk++;
            rec = 1;
            continue;
        }

        if (len == 0)   /* physical end of file */
            return 0;

        memcpy(dirblk, blkptr, sizeof(dirblk));

        rc = process_dirblk(cif, noext, extent, dirblk, dsname, pdsmember, optflags);
        if (rc < 0)
            return -1;

        if (rc > 0)     /* logical end of file */
            return 0;

        rec++;
    }
    UNREACHABLE_CODE( return -1 );
}

int do_cat(CIFBLK *cif, char *file)
{
 int rc = 0;
 DSXTENT extent[16];
 int noext;
 char buff[100]; /* must fit max length DSNAME/MEMBER..OPTS */
 char dsname[45];
 unsigned long optflags = 0;
 char *p;
 char *pdsmember = 0;

    if ((rc = get_volser( cif )) != 0)
        return rc;

    strncpy(buff, file, sizeof(buff));
    buff[sizeof(buff)-1] = 0;

    p = strchr(buff, ':');
    if (p)
    {
        *p++ = 0;
        for (; *p; p++)
        {
            if (*p == 'a')
                optflags |= OPT_ASCIIFY;
            else if (*p == 'c')
                optflags |= OPT_CARDS;
            else if (*p == 's')
                optflags |= OPT_SEQNO;
            else if (*p == '?')
                optflags |= OPT_MEMINFO_ONLY;
            else
            {
                char buf[2];
                buf[0] = *p;
                buf[1] = 0;
                // "Unknown 'member:flags' formatting option %s"
                FWRMSG( stderr, HHC02402, "E", buf );
                rc = -1;
            }
        }
    }

    if (rc != 0)
        return rc;

    p = strchr(buff, '/');
    if (p)
    {
        *p = 0;
        pdsmember = p + 1;
        string_to_upper(pdsmember);
    }

    strncpy(dsname, buff, sizeof(dsname));
    dsname[sizeof(dsname)-1] = 0;
    string_to_upper(dsname);

    rc = build_extent_array(cif, dsname, extent, &noext);
    if (rc < 0)
        return -1;

    /* Calculate ending relative track */
    if (extgui)
    {
        int bcyl;  /* Extent begin cylinder     */
        int btrk;  /* Extent begin head         */
        int ecyl;  /* Extent end cylinder       */
        int etrk;  /* Extent end head           */
        int trks;  /* total tracks in dataset   */
        int i;     /* loop control              */

        for (i = 0, trks = 0; i < noext; i++)
        {
            bcyl = (extent[i].xtbcyl[0] << 8) | extent[i].xtbcyl[1];
            btrk = (extent[i].xtbtrk[0] << 8) | extent[i].xtbtrk[1];
            ecyl = (extent[i].xtecyl[0] << 8) | extent[i].xtecyl[1];
            etrk = (extent[i].xtetrk[0] << 8) | extent[i].xtetrk[1];
            trks += (((ecyl*cif->heads)+etrk)-((bcyl*cif->heads)+btrk))+1;
        }
        EXTGUIMSG( "ETRK=%d\n", trks-1 );
    }

    if (pdsmember)
    {
        if      (!strcmp(pdsmember, "*")) optflags |= OPT_PDS_WILDCARD;
        else if (!strcmp(pdsmember, "?")) optflags |= OPT_PDS_LISTONLY;

        rc = do_cat_pdsmember(cif, extent, noext, dsname, pdsmember, optflags);

        if (!(optflags & (OPT_PDS_LISTONLY | OPT_PDS_WILDCARD)) && !found)
        {
            // "Member '%s' not found in dataset '%s' on volume '%s'"
            FWRMSG( stderr, HHC02406, "E", pdsmember, dsname, volser );
            rc = 1;
        }
    }
    else
    {
        // "Non-PDS-members not yet supported"
        FWRMSG( stderr, HHC02401, "E" );
        rc = 1;
    }

    return rc;
}

int get_volser( CIFBLK *cif )
{
    unsigned char *vol1data;
    int rc;
    U16 rlen;

    rc = read_block( cif, 0, 0, 3, 0, 0, &vol1data, &rlen );

    if (rc < 0)
        return -1;

    if (rc > 0)
    {
        // "%s record not found"
        FWRMSG( stderr, HHC02471, "E", "VOL1" );
        return -1;
    }

    make_asciiz( volser, sizeof(volser), vol1data+4, 6 );
    return 0;
}
