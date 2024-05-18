/* DASDLS.C    (C) Copyright Roger Bowler, 1999-2012                 */
/*             (C) and others 2013-2023                              */
/*              Hercules DASD Utilities: DASD list program           */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*********************************************************************
 *                          dasdls
 *
 * Copyright 2000-2009 by Malcolm Beattie
 * Based on code copyright by Roger Bowler, 1999-2009
 * Decode of F1 DSCB by Chris Cheney, 2013, based on
 * P.E. Havercan's VTOCLIST mainframe utility
 * Copied by Erwin Marschalk from 3.12 to 4.0 release
 * Further tweaks and fixes by Fish
 * Sorted report by Fish
 * Support for multiple extents by Ian Shorter
 *********************************************************************/

#include "hstdinc.h"
#include "hercules.h"
#include "dasdblks.h"

#define UTILITY_NAME    "dasdls"
#define UTILITY_DESC    "List DASD image file contents"

/*********************************************************************/
/* function prototypes                                               */

void pdate               (BYTE* value, int runflgs);
void pdatex              (BYTE* value, int runflgs);
void pbyte               (BYTE* value);
void phword              (BYTE* value);
int  hword               (HWORD value);

int  extent_size         (DSXTENT *ext, int heads);
int  extents_array       (DSXTENT extents[], int max, int *count, int heads);
int  chainf3             (int *size, BYTE *ptr, int *count );
int  ordday_to_calday    (int year, int ordinalday, int *month, int *day);

int  end_of_track        (BYTE *p);
int  list_contents       (CIFBLK *cif, char *sfile, char *volser, DSXTENT *extent );
int  do_ls_cif           (CIFBLK *cif, char *sfile);
int  do_ls               (char *file,  char *sfile);

/*********************************************************************/
/* globals                                                           */

static CIFBLK* cif   = NULL;
static int yroffs    = 0;       /* year offset                       */
#define                DEFAULT_DSNLEN   44
static int dsnlen    = DEFAULT_DSNLEN;
static int runflgs   = 0;       /* run flags set from command line   */
                                /* run flags:                        */
#define rf_caldate   0x01       /*     dates in yyyymmmdd format     */
#define rf_expdate   0x02       /*     show expiry dates             */
#define rf_refdate   0x04       /*     show last-referenced dates    */
#define rf_header    0x08       /*     show header                   */
#define rf_info      0x10       /*     show F1 info                  */
#define rf_cchh      0x20       /*     show extent cchh info         */
#define rf_nosort    0x40       /*     do NOT sort results           */

/*********************************************************************/
/* sort by dsname support                                            */

static char** lcchhtab     = NULL;      /* Parts of a full line      */
static char** lsegstab     = NULL;      /* Parts of a full line      */
static char** linestab     = NULL;      /* Complete lines            */
static int    numcchh      = 0;         /* #of segments in lcchhtab  */
static int    numsegs      = 0;         /* #of segments in lsegstab  */
static int    numlines     = 0;         /* #of lines in linestab     */
static char   segbuf[256]  = {0};       /* Line segment buffer       */
static char   linebuf[256*256] = {0};   /* Full line buffer          */

#define    CCHH_COLHDR  " Ext#  (BEG)      (END)    "
static int cchh_colhdr_width;

static void prtcchhseg( const char* fmt, ... )
{
    va_list   vl;
    va_start( vl, fmt );

    vsnprintf( segbuf, sizeof( segbuf ), fmt, vl );

    lcchhtab = realloc( lcchhtab, (numcchh + 1) * sizeof( char* ));
    lcchhtab[ numcchh++ ] = strdup( segbuf );
}

static void prtseg( const char* fmt, ... )
{
    va_list   vl;
    va_start( vl, fmt );

    vsnprintf( segbuf, sizeof( segbuf ), fmt, vl );

    lsegstab = realloc( lsegstab, (numsegs + 1) * sizeof( char* ));
    lsegstab[ numsegs++ ] = strdup( segbuf );
}

static void prtline( const char* fmt, ... )
{
    va_list   vl;
    va_start( vl, fmt );

    vsnprintf( linebuf, sizeof( linebuf ), fmt, vl );

    linestab = realloc( linestab, (numlines + 1) * sizeof( char* ));
    linestab[ numlines++ ] = strdup( linebuf );
}

static void print_line_from_segs()
{
    int  cchhnum, segnum;
    char buffer[256*256] = {0};

    if (runflgs & rf_cchh)
    {
        /* For each extent that the dataset has... */
        for (cchhnum=0; cchhnum < numcchh; ++cchhnum)
        {
            /* Print this extent's BEG/END CCHH */
            STRLCPY( buffer, lcchhtab[ cchhnum ]);
            free(            lcchhtab[ cchhnum ]);

            /* Print the remainder of the information... */
            for (segnum=0; segnum < numsegs; ++segnum)
            {
                /* (very first segment is always dsname) */
                STRLCAT( buffer, lsegstab[ segnum ]);

                /* (if this is extent #2, then we're done) */
                if (cchhnum > 0)
                {
                    STRLCAT( buffer, "\n" );
                    break;
                }
            }

            prtline( "%s", buffer );
        }
    }
    else
    {
        /* Print the remainder of the information... */
        for (segnum=0; segnum < numsegs; ++segnum)
            STRLCAT( buffer, lsegstab[ segnum ]);

        prtline( "%s", buffer );
    }

    free( lcchhtab );
    lcchhtab = NULL;
    numcchh = 0;

    for (segnum=0; segnum < numsegs; ++segnum)
        free( lsegstab[ segnum ]);

    free( lsegstab );
    lsegstab = NULL;
    numsegs = 0;
}

static int sort_linestab( const void* a, const void* b )
{
    const char* line_a = *(const char**)a;
    const char* line_b = *(const char**)b;

    // (we always sort by dataset name...)

    if (runflgs & rf_cchh)
    {
        int  rc;
        if ((rc = strncmp( line_a + cchh_colhdr_width,
                           line_b + cchh_colhdr_width, dsnlen )) != 0)
            return rc;
    }

    return strcmp( line_a, line_b );
}

/*********************************************************************/

int end_of_track( BYTE *p )
{
    return p[0] == 0xff && p[1] == 0xff && p[2] == 0xff && p[3] == 0xff
        && p[4] == 0xff && p[5] == 0xff && p[6] == 0xff && p[7] == 0xff;
}

/*********************************************************************/
/* ordinalday is 1..365 (366 in leap year)                           */
/* often wrongly called Julian day                                   */

#define JAN_FEB_DAYS    (31 + 28)

int ordday_to_calday( int year, int ordinalday, int *month, int *day )
{
  int d, m, offset;
  int leap = (((year % 4) == 0) &&
             (((year % 100) != 0) || ((year % 400) == 0)) ? 1 : 0);

  /* offset the months so that March is month 0 to deal
     with the anomalies of February (short month, leap day) */

  if ((ordinalday <= 0) || (ordinalday > (365 + leap)))
      return -1;

  offset = JAN_FEB_DAYS + leap;     /* 31 (Jan) + {28, 29} (Feb) */

  if (ordinalday <= offset)
      offset = - (365 - JAN_FEB_DAYS);

  d = ordinalday - 1 - offset;      /* ordinal day to index day and offset to 1 March */

  /* the months from March follow 5-month cycles of 31, 30, 31, 30, 31 days each month */

  m = d / 153;                      /* which 5-month cycle? */
  d -= (153 * m);                   /* day within 5-month cycle */
  m *= 5;                           /* month that starts the 5-month cycle */

  /* day of month = ((d mod (31 + 30)) mod 31 */

  while (d >= 61)                   /* body is executed 0, 1, or 2 times */
  {
      d -= 61;
      m += 2;
  }

  if (d >= 31)
  {
      d -= 31;
      m += 1;
  }

  *day = d + 1;                     /* ordinal day of month */

  /* convert back to January start of year and ordinal month */

  if (m >= 10)
      m -= 12;

  *month = m + 2;                   /* NB _index_ month, not ordinal month */
  return 0;
}

/*********************************************************************/
/* printing helper functions                                         */

void pdate( BYTE* value, int runflgs )
{
    char *fmt;
    static char *mths[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                            "Jul", "Aug", "Sep", "Oct", "Nov", "Dec", "" };
    int y = value[0] + yroffs;
    int m = 12;           /* 0..11 = Jan..Dec, 12 indexes empty string */
    int d = (value[1] << 8) | value[2];

    if (runflgs & rf_caldate)
    {
        /* calendar date (yyyymmmdd) */
        if (ordday_to_calday( y += 1900, d, &m, &d ))
            fmt = " *********"; /* (ordday_to_calday failed!) */
        else
            fmt = " %4.4d%s%2.2d";
    }
    else /* ordinal day (yyddd) */
    {
        y %= 100;
        fmt = " %2.2d%s%3.3d";
    }
    prtseg( fmt, y, mths[m], d );
}

void pdatex( BYTE* value, int runflgs )
{
    (value[0] | value[1] ? pdate( value, runflgs )
                         : prtseg( runflgs & rf_caldate ? " ---------"
                                                        : " -----" ));
}

void pbyte( BYTE* value )
{
    prtseg(" %3d", value[0]);
}

void phword( BYTE* value )
{
    prtseg(" %5d", (value[0] << 8) | value[1]);
}

/*********************************************************************/
/* dataset extent processing                                         */

int hword( HWORD value )
{
    return (value[0] << 8) + value[1];
}

int extent_size( DSXTENT *ext, int heads )
{
    return heads * ( hword( ext->xtecyl ) - hword( ext->xtbcyl ) ) +
                   ( hword( ext->xtetrk ) - hword( ext->xtbtrk ) ) + 1;
}

int extents_array( DSXTENT extents[], int max, int *count, int heads )
{
    int i;
    int size = 0;

    for (i=0; *count > 0 && i < max; i++)
    {
        if (extents[i].xttype)
        {
            size += extent_size( &extents[i], heads );
            *count -= 1;

            if (runflgs & rf_cchh)
            {
                prtcchhseg("%3d  %4.4X/%4.4X  %4.4X/%4.4X  ",
                            numcchh + 1,
                            hword( extents[i].xtbcyl ), hword( extents[i].xtbtrk ),
                            hword( extents[i].xtecyl ), hword( extents[i].xtetrk ) );
            }
        }
        else
        {
          // "%s record not found"
          FWRMSG( stderr, HHC02471, "E", "Extents_array DSXTENT" );
        }
    }
    return size;
}

/*********************************************************************/

int chainf3( int *size, BYTE *ptr, int *count )
{
    FORMAT3_DSCB *f3dscb = NULL;
    int rc = 0; /* prime for success */

    while (*count > 0 && (ptr[0] || ptr[1] || ptr[2] || ptr[3] || ptr[4]))
    {
//*debug*/fprintf(stderr, "*** %d %.2x%.2x %.2x%.2x %.2x\n",
//*debug*/        *count, ptr[0], ptr[1], ptr[2], ptr[3], ptr[4]);

        if (read_block( cif, hword( &ptr[0] ), hword( &ptr[2] ), ptr[4],
                        (BYTE**) &f3dscb, NULL, NULL, NULL ) == 0)
        {
            switch (f3dscb->ds3fmtid)
            {
                case 0xf3:
                {
                    if (0
                        || f3dscb->ds3keyid[0] != 0x03
                        || f3dscb->ds3keyid[1] != 0x03
                        || f3dscb->ds3keyid[2] != 0x03
                        || f3dscb->ds3keyid[3] != 0x03
                    )
                    {
                        break;  /* Error: break out of switch */
                    }
                    else
                    {
                        *size += extents_array( &f3dscb->ds3extnt[0], 4, count, cif->heads );
                        *size += extents_array( &f3dscb->ds3adext[0], 9, count, cif->heads );
                    }
                }
                /* Fall through to below 0xf2 case */
                /* GCC hint */
                /* FALLTHRU */
                case 0xf2:
                {
                    /* Note: same offset for both F2 and F3 DSCBs */
                    ptr = &f3dscb->ds3ptrds[0];
                    continue;   /* continue while loop */
                }
            }
        }

        /* Error */
        rc = -1;
//*debug*/fprintf(stderr, "*** DSCB id=0x%.2x\n", f3dscb->ds3fmtid);
        break;

    } /* end of while loop */

    return rc;
}

/*********************************************************************/
/* list_contents partly based on dasdutil.c:search_key_equal         */

int list_contents( CIFBLK *cif, char *sfile, char *volser, DSXTENT *extent )
{
    u_int cext  = 0;
    u_int ccyl  = (extent[cext].xtbcyl[0] << 8) | extent[cext].xtbcyl[1];
    u_int chead = (extent[cext].xtbtrk[0] << 8) | extent[cext].xtbtrk[1];
    u_int ecyl  = (extent[cext].xtecyl[0] << 8) | extent[cext].xtecyl[1];
    u_int ehead = (extent[cext].xtetrk[0] << 8) | extent[cext].xtetrk[1];

    EXTGUIMSG( "ETRK=%d\n", (ecyl * cif->heads) + ehead );

    if (runflgs & rf_header)
    {
        /* display column headers allowing for optional columns */

        char cchhbuf[32];

        if (runflgs & rf_cchh)
            STRLCPY( cchhbuf, CCHH_COLHDR );
        else
            cchhbuf[0] = 0;

        LOGMSG("%s%*s%s", cchhbuf, -dsnlen, "Dsname",
                                       runflgs & rf_caldate ? "  Created " : " CREDT");
        LOGMSG(runflgs & rf_refdate ? (runflgs & rf_caldate ? " Last Ref." : " REFDT") : "");
        LOGMSG(runflgs & rf_expdate ? (runflgs & rf_caldate ? " Exp. Date" : " EXPDT") : "");

        LOGMSG(" ORG RECFM LRECL BLKSZ Key  Trks%%Use#Ext 2ndry_alloc\n");
    }

    do {
        BYTE *ptr;
        int rc = read_track( cif, ccyl, chead );

        EXTGUIMSG( "CTRK=%d\n", (ccyl * cif->heads) + chead );

        if (rc < 0)
            return -1;

        ptr = cif->trkbuf + CKD_TRKHDR_SIZE;

        while (!end_of_track( ptr ))
        {

            CKD_RECHDR *rechdr = (CKD_RECHDR*)ptr;

            int kl = rechdr->klen;
            int dl = (rechdr->dlen[0] << 8) | rechdr->dlen[1];

            FORMAT1_DSCB *f1dscb = (FORMAT1_DSCB*) (ptr + CKD_RECHDR_SIZE);

//*debug*/   LOGMSG("F1DSCB\n");
//*debug*/   data_dump(f1dscb, sizeof(FORMAT1_DSCB));

            char dsname[ sizeof( f1dscb->ds1dsnam ) + 1 ];
            char txtrecfm[5] = "";                    /* recfm text */
            char *tmpstr;

            int lrecl;
            int numext;
            int space;

            double value;

            make_asciiz( dsname, sizeof( dsname ), ptr + CKD_RECHDR_SIZE, kl );

            if (valid_dsname( dsname ))
            {
                if (runflgs == 0 && dsnlen == DEFAULT_DSNLEN)
                {
                    prtseg("%s", dsname);
                }
                else
                {
                    prtseg("%*s", -dsnlen, dsname);

                    if (runflgs & rf_info)
                    {
                        /* CREDT */

                        pdate( f1dscb->ds1credt, runflgs );

                        /* REFDT */

                        if (runflgs & rf_refdate)
                            pdatex( f1dscb->ds1refdt, runflgs );

                        /* EXPDT */

                        if (runflgs & rf_expdate)
                            pdatex( f1dscb->ds1expdt, runflgs );

                        /* DSORG */

                        tmpstr = "??";

                        if (f1dscb->ds1dsorg[0] == 0 ||
                            f1dscb->ds1dsorg[0] == DSORG_U)
                        {
                            if (f1dscb->ds1dsorg[1] == DSORG_AM)
                                tmpstr = "VS";
                        }

                        if (f1dscb->ds1dsorg[1] == 0)
                        {
                            switch (f1dscb->ds1dsorg[0] & (DSORG_PS | DSORG_DA | DSORG_PO))
                            {
                                case DSORG_PS: tmpstr = "PS"; break;
                                case DSORG_DA: tmpstr = "DA"; break;
                                case DSORG_PO: tmpstr = "PO"; break;
                                case 0:        tmpstr = "  "; break; /* none of the above */
                                default: /* don't change? */  break; /* multiple bits set */
                            }
                        }
                        prtseg(" %s%s", tmpstr, f1dscb->ds1dsorg[0] & DSORG_U ? "U" : " ");

                        /* RECFM */

                        switch (f1dscb->ds1recfm & RECFM_FORMAT)
                        {
                            case RECFM_FORMAT_F: tmpstr = "F"; break;
                            case RECFM_FORMAT_V: tmpstr = "V"; break;
                            default:             tmpstr = "U"; break;
                        }
                        STRLCPY( txtrecfm, tmpstr );

                        if (f1dscb->ds1recfm & RECFM_BLOCKED)  STRLCAT( txtrecfm, "B" );
                        if (f1dscb->ds1recfm & RECFM_SPANNED)  STRLCAT( txtrecfm, "S" );

                        switch (f1dscb->ds1recfm & RECFM_CTLCHAR)
                        {
                            case RECFM_CTLCHAR_A:                   tmpstr = "A"; break;
                            case RECFM_CTLCHAR_M:                   tmpstr = "M"; break;
                            case RECFM_CTLCHAR_A | RECFM_CTLCHAR_M: tmpstr = "?"; break; /* both ?! */
                            default:                                tmpstr = "";  break; /* neither */
                        }
                        STRLCAT( txtrecfm, tmpstr );

                        if (f1dscb->ds1recfm & RECFM_TRKOFLOW)  STRLCAT( txtrecfm, "T" );

                        prtseg(" %-5s", txtrecfm);

                        /* LRECL */

                        lrecl = (f1dscb->ds1lrecl[0] << 8) | f1dscb->ds1lrecl[1];

                        if (lrecl) prtseg(" %5d", lrecl);
                        else       prtseg("      ");

                        /* BLKSZ, KEYLN */

                        phword( f1dscb->ds1blkl );     /* BLKSZ */
                        pbyte( &f1dscb->ds1keyl );     /* KEYLN */

                        /* space allocated */

                        numext = f1dscb->ds1noepv;
    //*debug*/           LOGMSG("NUMEXT = %5d", numext);

                        space = extents_array( &f1dscb->ds1ext1, 3, &numext, cif->heads );
    //*debug*/           LOGMSG("SPACE nach extents_array = %d", space);

                        chainf3( &space, &f1dscb->ds1ptrds[0], &numext );
    //*debug*/           LOGMSG("SPACE nach chainf3 = %d", space);

                        prtseg(" %5d", space);

                        /* % of allocated spaced used */

                        /* fraction of last track used = 1 - ds1trbal / trkzize */
                        value = 1.0 - (double)hword( &f1dscb->ds1trbal[0] ) / (cif->trksz);

                        /* add in the number of full tracks used */
                        value += hword( &f1dscb->ds1lstar[0] );

                        if (!space)
                            prtseg("    ");
                        else
                        {
                            value = value * 100 / space; /* % space used */
                            prtseg(" %3.0f", value);
                        }

                        /* Number of extents */

                        pbyte( &f1dscb->ds1noepv );  /* #EXT */

                        /* SCALO */

                        if (DS1SCALO_UNITS_ABSTR
                            == (f1dscb->ds1scalo[0] & DS1SCALO_UNITS))
                        {
                            prtseg(" %-11s", "ABSTR");
                        }
                        else
                        {
                            tmpstr = "CYL";

                            switch (f1dscb->ds1scalo[0] & DS1SCALO_UNITS)
                            {
                                case DS1SCALO_UNITS_BLK: tmpstr = "BLK"; break;
                                case DS1SCALO_UNITS_TRK: tmpstr = "TRK"; break;
                                case DS1SCALO_UNITS_CYL:                 break;
                            }
                            prtseg(" %3s%8d", tmpstr,
                                (((f1dscb->ds1scalo[1]  << 8) +
                                   f1dscb->ds1scalo[2]) << 8) +
                                   f1dscb->ds1scalo[3]);
                        }

                    } /* end if (runflgs & rf_info) */
                }

                prtseg("\n");
                print_line_from_segs();

            } /* end if (valid_dsname( dsname )) */

            ptr += CKD_RECHDR_SIZE + kl + dl;

        } /* end while (!end_of_track( ptr )) */

        chead++;

        if (chead >= cif->heads)
        {
            ccyl++;
            chead = 0;
        }

    } while (ccyl < ecyl || (ccyl == ecyl && chead <= ehead));

    /* If we "printed" any lines... */
    if (numlines)
    {
        int i;

        /* Sort them into ascending sequence by dsname */
        if (!(runflgs & rf_nosort))
            qsort( linestab, numlines, sizeof( linestab ), sort_linestab );

        /* NOW actually print them all for real */
        for (i=0; i < numlines; ++i)
        {
            LOGMSG( "%s\n", RTRIM( linestab[i] ));
            free( (linestab[i]) );
        }

        free( linestab );
        linestab = NULL;
        numlines = 0;
    }

    return 0;
}

/*********************************************************************/
/* do_ls_cif based on dasdutil.c:build_extent_array                  */

int do_ls_cif( CIFBLK *cif, char *sfile )
{
    int rc;

    U32 cyl;
    U8  head;
    U8  rec;

    U16 rlen;
    U8  klen;

    unsigned char *vol1data;
    FORMAT4_DSCB  *f4dscb;

    char volser[7];

    rc = read_block( cif, 0, 0, 3, 0, 0, &vol1data, &rlen );

    if (rc < 0)
        return -1;

    if (rc > 0)
    {
        // "%s record not found"
        FWRMSG( stderr, HHC02471, "E", "VOL1" );
        return -1;
    }

    make_asciiz( volser, sizeof( volser ), vol1data + 4, 6 );

    if (sfile)
        LOGMSG( "\nVOLSER:  %-6s    \"%s\" sf=\"%s\"\n\n", volser, cif->fname, &sfile[3] );
    else
        LOGMSG( "\nVOLSER:  %-6s    \"%s\"\n\n", volser, cif->fname );

    cyl  = (vol1data[11] << 8) | vol1data[12];
    head = (vol1data[13] << 8) | vol1data[14];
    rec  =  vol1data[15];

    rc = read_block( cif, cyl, head, rec, (void *)&f4dscb, &klen, 0, 0 );

    if (rc < 0)
        return -1;

    if (rc > 0)
    {
        // "%s record not found"
        FWRMSG( stderr, HHC02471, "E", "Format 4 DSCB" );
        return -1;
    }

    return list_contents( cif, sfile, volser, &f4dscb->ds4vtoce );
}

/*********************************************************************/

int do_ls( char *file, char *sfile )
{
    LOGMSG("\n");

    if (!(cif = open_ckd_image( file, sfile, O_RDONLY | O_BINARY, IMAGE_OPEN_NORMAL )))
        return -1;

    if (do_ls_cif( cif, sfile ) != 0)
    {
        close_ckd_image( cif );
        cif = NULL;
        return -1;
    }

    if (close_ckd_image( cif ) != 0)
    {
        cif = NULL;
        return -1;
    }

    cif = NULL;
    return 0;
}

/*********************************************************************/

int main( int argc, char **argv )
{
char           *pgm;            /* less any extension (.ext)         */
int             rc = 0;
char           *fn, *sfn;

    INITIALIZE_UTILITY( UTILITY_NAME, UTILITY_DESC, &pgm );

    if (argc < 2)
    {
        // "Usage: %s [-option [-option ... ]] ckdfile [sf=shadow-file-name]%s"
        WRMSG( HHC02463, "I", pgm );
        exit(2);
    }

    cchh_colhdr_width = (int) strlen( CCHH_COLHDR );

    /*
     * If your version of Hercules doesn't have support in its
     * dasdutil.c for turning off verbose messages, then remove
     * the following line but you'll have to live with chatty
     * progress output on stdout.
     */
    set_verbose_util(0);                    /* (don't be chatty) */

    while (*++argv)
    {
        fn = *argv;

        if (strcmp( fn, "-nosort" ) == 0)   /* do NOT sort results */
        {
            runflgs |= rf_nosort;
            continue;
        }
        if (strcmp( fn, "-info" ) == 0)     /* show F1 info */
        {
            runflgs |= rf_info;
            continue;
        }
        if (strcmp( fn, "-caldt" ) == 0)    /* calendar date format */
        {
            runflgs |= (rf_caldate | rf_info);
            continue;
        }
        if (strcmp( fn, "-expdt" ) == 0)    /* show expiry date */
        {
            runflgs |= (rf_expdate | rf_info);
            continue;
        }
        if (strcmp( fn, "-refdt" ) == 0)    /* show last-reference date */
        {
            runflgs |= (rf_refdate | rf_info);
            continue;
        }
        if (strcmp( fn, "-hdr" ) == 0)      /* show column headers */
        {
            runflgs |= (rf_header | rf_info);
            continue;
        }
        if (strcmp( fn, "-cchh" ) == 0)     /* show extent cchh info */
        {
            runflgs |= (rf_cchh | rf_info);
            continue;
        }
        if (1
            && strlen( *argv )   >    6
            && !memcmp( fn, "-dsnl=", 6 ))  /* restrict dsname width (custom) */
        {
            runflgs |= rf_info;
            dsnlen = atoi( fn + 6 );
            continue;
        }
        if (strcmp( fn, "-dsnl" ) == 0)     /* restrict dsname width (default) */
        {
            runflgs |= rf_info;
            dsnlen = 26;
            continue;
        }
        if (1
            && strlen( *argv )   >      8
            && !memcmp( fn, "-yroffs=", 8 ))/* year offset (custom) */
        {
            runflgs |= rf_info;
            yroffs = atoi( fn + 8 );
            continue;
        }
        if (strcmp( fn, "-yroffs" ) == 0)   /* year offset (default) */
        {
            runflgs |= rf_info;
            yroffs = 28;
            continue;
        }

        /* Check for shadow file */
        if (1
            &&         *(argv+1)
            && strlen( *(argv+1) )   >   3
            && memcmp( *(argv+1), "sf=", 3 ) == 0
        )
             sfn = *++argv;
        else sfn = NULL;

        if (do_ls( fn, sfn ))
            rc = 1;
    }

    LOGMSG("\nEnd of %s; rc=%d\n", UTILITY_NAME, rc );
    return rc;
}

/*********************************************************************/
