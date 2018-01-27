/* SLLIB.H      (C) Copyright Leland Lucius, 2000-2012               */
/*             Library for managing Standard Label tapes.            */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#if !defined( _SLLIB_H_ )
#define _SLLIB_H_

#include "hercules.h"

#if !defined( TRUE )
#define TRUE 1
#endif

#if !defined( FALSE )
#define FALSE 0
#endif

/*
|| Raw label structure
*/
typedef struct _sllabel
{
    char            id[      3 ];
    char            num[     1 ];
    union
    {
        struct
        {
            char    volser[  6 ];
            char    access[  1 ];
            char    vtoc[    5 ];
            char    rsvd1[  25 ];
            char    owner[  10 ];
            char    rsvd2[  29 ];
        } vol;

        struct
        {
            char    volser[  6 ];       /* VSN              */
            char    access[  1 ];       /* always space     */
            char    rsvd1[  13 ];       /* always spaces    */
            char    impid[  13 ];       /* implementation ID*/
            char    owner[  14 ];       /* usually spaces   */
            char    rsvd2[  28 ];       /* always spaces    */
            char    one[1      ];       /* always '1'       */
        } avol;                         /* ANSI VOL1        */

        struct
        {
            char    dsid[   17 ];
            char    volser[  6 ];
            char    volseq[  4 ];
            char    dsseq[   4 ];
            char    genno[   4 ];
            char    verno[   2 ];
            char    crtdt[   6 ];
            char    expdt[   6 ];
            char    dssec[   1 ];
            char    blklo[   6 ];
            char    syscd[  13 ];
            char    rsvd1[   3 ];
            char    blkhi[   4 ];
        } ds1;

        struct
        {
            char    dsid[   17 ];       /* File Identifier          */
            char    volser[  6 ];       /* VSN                      */
            char    volseq[  4 ];       /* 0001                     */
            char    dsseq[   4 ];       /* 0001                     */
            char    genno[   4 ];       /* 0001                     */
            char    verno[   2 ];       /* spaces                   */
            char    crtdt[   6 ];       /* cyyddd                   */
            char    expdt[   6 ];       /* cyyddd                   */
            char    dssec[   1 ];       /* space                    */
            char    blkcnt[  6 ];       /* 000000                   */
            char    syscd[  13 ];       /* System Code              */
            char    rsvd1[   7 ];       /* 0000000                  */
        } ads1;                         /* ANSI HDR1, EOF1, EOV1    */

        struct
        {
            char    recfm[   1 ];
            char    blksize[ 5 ];
            char    lrecl[   5 ];
            char    den[     1 ];
            char    dspos[   1 ];
            char    jobid[  17 ];
            char    trtch[   2 ];
            char    ctrl[    1 ];
            char    rsvd1[   1 ];
            char    blkattr[ 1 ];
            char    rsvd2[   2 ];
            char    devser[  6 ];
            char    ckptid[  1 ];
            char    rsvd3[  22 ];
            char    lblkln[ 10 ];
        } ds2;

        struct
        {
            char    data[   76 ];
        } usr;
    } u;
} SLLABEL;

/*
|| Cooked label structure
*/
typedef struct _slfmt
{
    char    *key[ 14 ];
    char    *val[ 14 ];

    char    type[ 4 + 1 ];

    union
    {
        struct
        {
            char    volser[  6 + 1 ];
            char    access[  1 + 1 ];
            char    owner[  14 + 1 ];
        } vol;

        struct
        {
            char    dsid[   17 + 1 ];
            char    volser[  6 + 1 ];
            char    volseq[  4 + 1 ];
            char    dsseq[   4 + 1 ];
            char    genno[   4 + 1 ];
            char    verno[   2 + 1 ];
            char    crtdt[   6 + 1 ];
            char    expdt[   6 + 1 ];
            char    dssec[   1 + 1 ];
            char    blklo[   6 + 1 ];
            char    syscd[  13 + 1 ];
            char    blkhi[   4 + 1 ];
        } ds1;

        struct
        {
            char    recfm[   1 + 1 ];
            char    blksize[ 5 + 1 ];
            char    lrecl[   5 + 1 ];
            char    den[     1 + 1 ];
            char    dspos[   1 + 1 ];
            char    jobid[  17 + 1 ];
            char    trtch[   2 + 1 ];
            char    ctrl[    1 + 1 ];
            char    blkattr[ 1 + 1 ];
            char    devser[  6 + 1 ];
            char    ckptid[  1 + 1 ];
            char    lblkln[ 10 + 1 ];
        } ds2;

        struct
        {
            char    data[   76 + 1 ];
        } usr;
    } u;
} SLFMT;

/*
|| Prettier label structure mappings
*/
#define slvol u.vol
#define slds1 u.ds1
#define slds2 u.ds2
#define slusr u.usr

/*
|| Special dataset name used to generate an IEHINITT HDR1 label
*/
#define SL_INITDSN "_IEHINITT_"

/*
|| Length of SL format date
*/
#define SL_DATELEN  6

/*
|| Label types
*/
#define SLT_UNKNOWN 0
#define SLT_VOL     1
#define SLT_HDR     2
#define SLT_UHL     3
#define SLT_EOF     4
#define SLT_EOV     5
#define SLT_UTL     6

/*
|| Macros to test label type
*/
#define sl_isvol( s, n ) sl_istype( (s), SLT_VOL, (n) )
#define sl_ishdr( s, n ) sl_istype( (s), SLT_HDR, (n) )
#define sl_isuhl( s, n ) sl_istype( (s), SLT_UHL, (n) )
#define sl_iseof( s, n ) sl_istype( (s), SLT_EOF, (n) )
#define sl_iseov( s, n ) sl_istype( (s), SLT_EOV, (n) )
#define sl_isutl( s, n ) sl_istype( (s), SLT_UTL, (n) )

/*
|| Macros to define specific labels
*/
#define sl_vol1( p1, p2, p3 ) \
        sl_vol( p1, p2, p3 )

#define sl_hdr1( p1, p2, p3, p4, p5, p6, p7 ) \
        sl_ds1( p1, SLT_HDR, p2, p3, p4, p5, p6, p7 )

#define sl_eof1( p1, p2, p3, p4, p5, p6 ) \
        sl_ds1( p1, SLT_EOF, p2, p3, p4, p5, p6 )

#define sl_eov1( p1, p2, p3, p4, p5, p6 ) \
        sl_ds1( p1, SLT_EOV, p2, p3, p4, p5, p6 )

#define sl_hdr2( p1, p2, p3, p4, p5, p6, p7, p8 ) \
        sl_ds2( p1, SLT_HDR, p2, p3, p4, p5, p6, p7, p8 )

#define sl_eof2( p1, p2, p3, p4, p5, p6, p7, p8 ) \
        sl_ds2( p1, SLT_EOF, p2, p3, p4, p5, p6, p7, p8 )

#define sl_eov2( p1, p2, p3, p4, p5, p6, p7, p8 ) \
        sl_ds2( p1, SLT_EOV, p2, p3, p4, p5, p6, p7, p8 )

#define sl_uhl1( p1, p2 ) sl_usr( p1, SLT_UHL, 1, p2 )

#define sl_uhl2( p1, p2 ) sl_usr( p1, SLT_UHL, 2, p2 )

#define sl_uhl3( p1, p2 ) sl_usr( p1, SLT_UHL, 3, p2 )

#define sl_uhl4( p1, p2 ) sl_usr( p1, SLT_UHL, 4, p2 )

#define sl_uhl5( p1, p2 ) sl_usr( p1, SLT_UHL, 5, p2 )

#define sl_uhl6( p1, p2 ) sl_usr( p1, SLT_UHL, 6, p2 )

#define sl_uhl7( p1, p2 ) sl_usr( p1, SLT_UHL, 7, p2 )

#define sl_uhl8( p1, p2 ) sl_usr( p1, SLT_UHL, 8, p2 )

#define sl_utl1( p1, p2 ) sl_usr( p1, SLT_UTL, 1, p2 )

#define sl_utl2( p1, p2 ) sl_usr( p1, SLT_UTL, 2, p2 )

#define sl_utl3( p1, p2 ) sl_usr( p1, SLT_UTL, 3, p2 )

#define sl_utl4( p1, p2 ) sl_usr( p1, SLT_UTL, 4, p2 )

#define sl_utl5( p1, p2 ) sl_usr( p1, SLT_UTL, 5, p2 )

#define sl_utl6( p1, p2 ) sl_usr( p1, SLT_UTL, 6, p2 )

#define sl_utl7( p1, p2 ) sl_usr( p1, SLT_UTL, 7, p2 )

#define sl_utl8( p1, p2 ) sl_usr( p1, SLT_UTL, 8, p2 )

/*
|| Error definitions
*/
#define SLE_BLKSIZE             -1      /* Block size out of range          */
#define SLE_DSSEQ               -2      /* Data set sequence out of range   */
#define SLE_EXPDT               -3      /* Invalid expiration date          */
#define SLE_JOBNAME             -4      /* Missing or invalid job name      */
#define SLE_LRECL               -5      /* Invalid record length            */
#define SLE_OWNER               -6      /* Owner string too long            */
#define SLE_RECFM               -7      /* Missing or invalid record format */
#define SLE_STEPNAME            -8      /* Missing or invalid step name     */
#define SLE_TRTCH               -9      /* Invalid recording technique      */
#define SLE_VOLSEQ              -10     /* Volume sequence out of range     */
#define SLE_VOLSER              -11     /* Missing or invalid volume serial */
#define SLE_DATA                -12     /* User data too long               */
#define SLE_INVALIDTYPE         -13     /* Label type invalid               */
#define SLE_INVALIDNUM          -14     /* Label number invalid             */

/*
|| Public functions/data
*/
SLL_DLL_IMPORT char *sl_atoe( void *, void *, int );
SLL_DLL_IMPORT char *sl_etoa( void *, void *, int );
SLL_DLL_IMPORT char *sl_fmtdate( char *, char *, int );
SLL_DLL_IMPORT void sl_fmtlab( SLFMT *, SLLABEL * );
SLL_DLL_IMPORT int sl_islabel( SLLABEL *, void *, int );
SLL_DLL_IMPORT int sl_istype( void *, int type, int num );
SLL_DLL_IMPORT int sl_vol( SLLABEL *, char *, char * );
SLL_DLL_IMPORT int sl_ds1( SLLABEL *, int type, char *, char *, int, int, char *, int );
SLL_DLL_IMPORT int sl_ds2( SLLABEL *, int type, char *, int, int, char *, char *, char * );
SLL_DLL_IMPORT int sl_usr( SLLABEL *, int type, int num, char * );
SLL_DLL_IMPORT const char *sl_error( int rc );

#endif /* defined( _SLLIB_H_ ) */
