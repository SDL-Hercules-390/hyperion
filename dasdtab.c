/* DASDTAB.C    (C) Copyright Roger Bowler, 1999-2012                */
/*              (C) and others 2013-2023                             */
/*              Hercules Supported DASD definitions                  */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* This module contains the tables that define the attributes of     */
/* each DASD device and control unit supported by Hercules.          */
/* Routines are also provided to perform table lookup and build the  */
/* device identifier and characteristics areas.                      */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* Reference information:                                            */
/* GA32-0099 IBM 3990 Storage Control Reference (Models 1, 2, and 3) */
/* GA32-0274 IBM 3990,9390 Storage Control Reference                 */
/* GC26-7006 IBM RAMAC Array Subsystem Reference                     */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* Note: source for most CKD/FBA device capacities take from SDI's   */
/* device capacity page at: http://www.sdisw.com/dasd_capacity.html  */
/* (used with permission)                                            */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

#define _DASDTAB_C_
#define _HDASD_DLL_

#include "hercules.h"

/*-------------------------------------------------------------------*/
/*             CKD control unit definitions                          */
/*-------------------------------------------------------------------*/
/*                                                                   */
/*           ----------------------------------                      */
/*           3880, 3990, 2105 and higher coding                      */
/*           ----------------------------------                      */
/*                                                                   */
/*                                                                   */
/* Model coding bits (RDC byte 2):                                   */
/*                                                                   */
/*   11.. .... 3880 Speed Matching Buffer (LR support)               */
/*   ..1. .... Non-Gap-Synchronous mode                              */
/*   .... 1... Cached                                                */
/*   .... .011 3880-3                                                */
/*   .... .101 3880-3     Feature 3005 - 3380 AJ4/AK4 Attachment     */
/*   .... .010 3990-1/2   Nocache/Synchronous                        */
/*   .... .100 3990-3/6/7 Basic Operation Mode                       */
/*   .... .001 3990-6/7   Enhanced Mode                              */
/*                                                                   */
/*                                                                   */
/* Features (RDC byte 6):                                            */
/*                                                                   */
/*   1... .... Multiple Burst ECC (ignored, 3380 devices)            */
/*   .1.. .... Locate Record, Read Tracks                            */
/*   ..1. .... Reserved                                              */
/*   ...1 .... Locate Record, Read                                   */
/*   .... 1... Set System Characteristics is supported  (*)          */
/*   .... .1.. Set System Characteristics               (*)          */
/*             has been received for this Path Group    (*)          */
/*   .... ..1. Prefix CCW supported & enabled           (*)          */
/*   .... ...1 Reserved for VM minidisk use                          */
/*             (testing this bit for zero is not reliable as an      */
/*             indicator that this is not a minidisk; can fail       */
/*             when minidisk is a fullpack minidisk)                 */
/*                                                                   */
/*        (*)  https://www.vm.ibm.com/pubs/cp720/RDCBK.HTML          */
/*                                                                   */
/*                                                                   */
/* Levels defining byte 8 format (RDC byte 7):                       */
/*                                                                   */
/*   0000 0000 Not supported by Hercules, must be zero               */
/*                                                                   */
/*                                                                   */
/* Added feature support (RDC byte 8):                               */
/*                                                                   */
/*   1... .... RAMAC: During dynamic sparing operations,             */
/*             a defective primary track will be written             */
/*             as defective on the secondary                         */
/*   ...1 .... RAMAC: Data striping and compaction is                */
/*             supported on parallel channels                        */
/*   .00. 0000 Reserved, set to zeros                                */
/*                                                                   */
/*                                                                   */
/* Subsystem Program Visible Facilities (RDC byte 9):                */
/*                                                                   */
/*   1... .... Cache Fast Write supported (not supported, yet!)      */
/*             (non-retentive data)                                  */
/*   .1.. .... Lock Facility supported    (not supported, yet!)      */
/*   ..1. .... Record Cache supported                                */
/*   ...1 .... Track Cache supported                                 */
/*   .... 1... Dual Copy supported        (not supported)            */
/*   .... .1.. DASD Fast Write supported  (not supported)            */
/*   .... ..1. Reset Allegiance           (not supported, yet!)      */
/*   .... ...1 24 Byte Compatibility Sense Format                    */
/*             (set by code when 3380 on 3390 controller)            */
/*                                                                   */
/*                                                                   */
/*-------------------------------------------------------------------*/
static CKDCU ckdcutab[] = {
/*                              func/ type                                        */
/* name          type model code feat code features   ciws ---------  senselength */
 {"2314",       0x2314,0x00,0x00,0x00,0x00,0x00000000,0,0,0,0,0,0,0,0,6},
 {"2835",       0x2835,0x00,0x00,0x00,0x00,0x00000000,0,0,0,0,0,0,0,0,6},
 {"2841",       0x2841,0x00,0x00,0x00,0x00,0x00000000,0,0,0,0,0,0,0,0,6},
 {"3830",       0x3830,0x02,0x00,0x00,0x00,0x00000000,0,0,0,0,0,0,0,0,24},
 {"3880",       0x3880,0x05,0x09,0x00,0x00,0x80000000,0,0,0,0,0,0,0,0,24},
/*
    PROGRAMMING NOTE: the 00001000 features bit (RAMAC Data striping
    and compaction on parallel channels) is apparently needed in order
    to allow SMS to create VSAM Extended Format Datasets (as well as
    RDC bytes 51-53 needing to be set properly too). We set them even
    though we do not directly support RAID since the user may provide
    such support themselves by placing their Hercules dasd file on a
    RAID array and it's important to be able to create VSAM Extended
    Format datasets since doing so is common in the real world so we
    need to "support" allowing them to do so.
*/
 {"3990",       0x3990,0xc2,0x10,0x00,0x00,0xd2000000,0x40fa0100,0,0,0,0,0,0,0,32},
 {"3990-3",     0x3990,0xec,0x06,0x00,0x00,0xd2001010,0x40fa0100,0x41270004,0x423e0040,0,0,0,0,0,32},
 {"3990-6",     0x3990,0xe9,0x15,0x48,0x15,0x52001010,0x40fa0100,0x41270004,0x423e0060,0,0,0,0,0,32},
 {"9343",       0x9343,0xe0,0x11,0x00,0x00,0x80000000,0,0,0,0,0,0,0,0,32}
/*"2105",       0x2105,0xe8,0x15,0x48,0x15,0x50000037,0x40fa0100,0x41270004,0x423e01a0,0x433e0008,0,0,0,0,32}, */
} ;
#define CKDCU_NUM (sizeof(ckdcutab)/CKDCU_SIZE)

/*-------------------------------------------------------------------*/
/*                    CKD device definitions                         */
/*-------------------------------------------------------------------*/
static CKDDEV ckdtab[] = {
/* name         type model clas code prime a hd    r0    r1 har0   len sec    rps  f f1  f2   f3   f4 f5 f6  cu */
 {"2305-1",    0x2305,0x00,0x20,0x00,   48,6, 8,14568,14136, 432,14568, 90,0x0000,-1,202,432,  0,   0,  0,0,"2835"},
 {"2305",      0x2305,0x00,0x20,0x00,   48,6, 8,14568,14136, 432,14568, 90,0x0000,-1,202,432,  0,   0,  0,0,"2835"},
 {"2305-2",    0x2305,0x02,0x20,0x00,   96,12,8,14858,14660, 198,14858, 90,0x0000,-1, 91,198,  0,   0,  0,0,"2835"},
 {"2305-x",    0x2305,0x02,0x20,0x00,65535,0, 8,14858,14660, 198,14858, 90,0x0000,-1, 91,198,  0,   0,  0,0,"2835"},

/* name         type model clas code prime a hd    r0    r1 har0   len sec    rps  f f1  f2   f3   f4 f5 f6  cu */
 {"2311-1",    0x2311,0x00,0x20,0x00,  200,3,10,    0, 3625,   0, 3625,  0,0x0000,-2,20, 61, 537, 512,  0,0,"2841"},
 {"2311",      0x2311,0x00,0x20,0x00,  200,3,10,    0, 3625,   0, 3625,  0,0x0000,-2,20, 61, 537, 512,  0,0,"2841"},
 {"2311-x",    0x2311,0x00,0x20,0x00,65535,0,10,    0, 3625,   0, 3625,  0,0x0000,-2,20, 61, 537, 512,  0,0,"2841"},

/* name         type model clas code prime a hd    r0    r1 har0   len sec    rps  f f1  f2   f3   f4 f5 f6  cu */
 {"2314-1",    0x2314,0x00,0x20,0x00,  200,3,20,    0, 7294,   0, 7294,  0,0x0000,-2,45,101,2137,2048,  0,0,"2314"},
 {"2314",      0x2314,0x00,0x20,0x00,  200,3,20,    0, 7294,   0, 7294,  0,0x0000,-2,45,101,2137,2048,  0,0,"2314"},
 {"2314-x",    0x2314,0x00,0x20,0x00,65535,0,20,    0, 7294,   0, 7294,  0,0x0000,-2,45,101,2137,2048,  0,0,"2314"},

/* name         type model clas code prime a hd    r0    r1 har0   len sec    rps  f f1  f2   f3   f4 f5 f6  cu */
 {"3330-1",    0x3330,0x01,0x20,0x00,  404,7,19,13165,13030, 135,13165,128,0x0000,-1,56,135,   0,   0,  0,0,"3830"},
 {"3330",      0x3330,0x01,0x20,0x00,  404,7,19,13165,13030, 135,13165,128,0x0000,-1,56,135,   0,   0,  0,0,"3830"},
 {"3330-11",   0x3330,0x11,0x20,0x00,  808,7,19,13165,13030, 135,13165,128,0x0000,-1,56,135,   0,   0,  0,0,"3830"},
 {"3330-2",    0x3330,0x11,0x20,0x00,  808,7,19,13165,13030, 135,13165,128,0x0000,-1,56,135,   0,   0,  0,0,"3830"},
 {"3330-x",    0x3330,0x11,0x20,0x00,65535,0,19,13165,13030, 135,13165,128,0x0000,-1,56,135,   0,   0,  0,0,"3830"},

/* name         type model clas code prime a hd    r0    r1 har0   len sec    rps  f f1  f2   f3   f4 f5 f6  cu */
 {"3340-35",   0x3340,0x01,0x20,0x00,  348,1,12, 8535, 8368, 167, 8535, 64,0x0000,-1,75,167,   0,   0,  0,0,"3830"},
 {"3340-1",    0x3340,0x01,0x20,0x00,  348,1,12, 8535, 8368, 167, 8535, 64,0x0000,-1,75,167,   0,   0,  0,0,"3830"},
 {"3340",      0x3340,0x01,0x20,0x00,  348,1,12, 8535, 8368, 167, 8535, 64,0x0000,-1,75,167,   0,   0,  0,0,"3830"},
 {"3340-70",   0x3340,0x02,0x20,0x00,  696,2,12, 8535, 8368, 167, 8535, 64,0x0000,-1,75,167,   0,   0,  0,0,"3830"},
 {"3340-2",    0x3340,0x02,0x20,0x00,  696,2,12, 8535, 8368, 167, 8535, 64,0x0000,-1,75,167,   0,   0,  0,0,"3830"},
 {"3340-x",    0x3340,0x02,0x20,0x00,65535,0,12, 8535, 8368, 167, 8535, 64,0x0000,-1,75,167,   0,   0,  0,0,"3830"},

/* name         type model clas code prime a hd    r0    r1 har0   len sec    rps  f f1  f2   f3   f4 f5 f6  cu */
 {"3350-1",    0x3350,0x00,0x20,0x00,  555,5,30,19254,19069, 185,19254,128,0x0000,-1,82,185,   0,   0,  0,0,"3830"},
 {"3350",      0x3350,0x00,0x20,0x00,  555,5,30,19254,19069, 185,19254,128,0x0000,-1,82,185,   0,   0,  0,0,"3830"},
 {"3350-x",    0x3350,0x00,0x20,0x00,65535,0,30,19254,19069, 185,19254,128,0x0000,-1,82,185,   0,   0,  0,0,"3830"},

/* name         type model clas code prime a hd    r0    r1 har0   len sec    rps  f f1  f2   f3   f4 f5 f6  cu */
 {"3375-1",    0x3375,0x02,0x20,0x0e,  959,3,12,36000,35616, 832,36000,196,0x5007, 1, 32,384,160,   0,  0,0,"3880"},
 {"3375",      0x3375,0x02,0x20,0x0e,  959,3,12,36000,35616, 832,36000,196,0x5007, 1, 32,384,160,   0,  0,0,"3880"},
 {"3375-x",    0x3375,0x02,0x20,0x0e,65535,0,12,36000,35616, 832,36000,196,0x5007, 1, 32,384,160,   0,  0,0,"3880"},

/* name         type model clas code prime a hd    r0    r1 har0   len sec    rps  f f1  f2   f3   f4 f5 f6  cu */
 {"3380-A",    0x3380,0x02,0x20,0x0e,  885,1,15,47988,47476,1088,47968,222,0x5007, 1, 32,492,236,   0,  0,0,"3880"},
 {"3380-B",    0x3380,0x02,0x20,0x0e,  885,1,15,47988,47476,1088,47968,222,0x5007, 1, 32,492,236,   0,  0,0,"3880"},
 {"3380-1",    0x3380,0x02,0x20,0x0e,  885,1,15,47988,47476,1088,47968,222,0x5007, 1, 32,492,236,   0,  0,0,"3880"},
 {"3380",      0x3380,0x02,0x20,0x0e,  885,1,15,47988,47476,1088,47968,222,0x5007, 1, 32,492,236,   0,  0,0,"3880"},
 {"3380-D",    0x3380,0x06,0x20,0x0e,  885,1,15,47988,47476,1088,47968,222,0x5007, 1, 32,492,236,   0,  0,0,"3880"},
 {"3380-J",    0x3380,0x16,0x20,0x0e,  885,1,15,47988,47476,1088,47968,222,0x5007, 1, 32,492,236,   0,  0,0,"3880"},
 {"3380-E",    0x3380,0x0a,0x20,0x0e, 1770,2,15,47988,47476,1088,47968,222,0x5007, 1, 32,492,236,   0,  0,0,"3880"},
 {"3380-2",    0x3380,0x0a,0x20,0x0e, 1770,2,15,47988,47476,1088,47968,222,0x5007, 1, 32,492,236,   0,  0,0,"3880"},
 {"3380-K",    0x3380,0x1e,0x20,0x0e, 2655,3,15,47988,47476,1088,47968,222,0x5007, 1, 32,492,236,   0,  0,0,"3880"},
 {"3380-3",    0x3380,0x1e,0x20,0x0e, 2655,3,15,47988,47476,1088,47968,222,0x5007, 1, 32,492,236,   0,  0,0,"3880"},
 {"EMC3380K+", 0x3380,0x1e,0x20,0x0e, 3339,3,15,47988,47476,1088,47968,222,0x5007, 1, 32,492,236,   0,  0,0,"3880"},
 {"EMC3380K++",0x3380,0x1e,0x20,0x0e, 3993,3,15,47988,47476,1088,47968,222,0x5007, 1, 32,492,236,   0,  0,0,"3880"},
 {"3380-x",    0x3380,0x1e,0x20,0x0e,65535,0,15,47988,47476,1088,47968,222,0x5007, 1, 32,492,236,   0,  0,0,"3880"},

/* name         type model clas code prime a hd    r0    r1 har0   len sec    rps  f f1  f2   f3   f4 f5 f6  cu */
 {"3390-1",    0x3390,0x02,0x20,0x26, 1113,1,15,57326,56664,1428,58786,224,0x7708, 2, 34,19,   9,   6,116,6,"3990"},
 {"3390",      0x3390,0x02,0x20,0x26, 1113,1,15,57326,56664,1428,58786,224,0x7708, 2, 34,19,   9,   6,116,6,"3990"},
 {"3390-2",    0x3390,0x06,0x20,0x27, 2226,1,15,57326,56664,1428,58786,224,0x7708, 2, 34,19,   9,   6,116,6,"3990"},
 {"3390-3",    0x3390,0x0a,0x20,0x24, 3339,1,15,57326,56664,1428,58786,224,0x7708, 2, 34,19,   9,   6,116,6,"3990"},
 {"3390-9",    0x3390,0x0c,0x20,0x32,10017,3,15,57326,56664,1428,58786,224,0x7708, 2, 34,19,   9,   6,116,6,"3990"},
 {"3390-27",   0x3390,0x0c,0x20,0x32,32760,3,15,57326,56664,1428,58786,224,0x7708, 2, 34,19,   9,   6,116,6,"3990"},
 {"3390-J",    0x3390,0x0c,0x20,0x32,32760,3,15,57326,56664,1428,58786,224,0x7708, 2, 34,19,   9,   6,116,6,"3990"},
 {"3390-54",   0x3390,0x0c,0x20,0x32,65520,3,15,57326,56664,1428,58786,224,0x7708, 2, 34,19,   9,   6,116,6,"3990"},
 {"3390-JJ",   0x3390,0x0c,0x20,0x32,65520,3,15,57326,56664,1428,58786,224,0x7708, 2, 34,19,   9,   6,116,6,"3990"},
 {"3390-x",    0x3390,0x0c,0x20,0x32,65535,0,15,57326,56664,1428,58786,224,0x7708, 2, 34,19,   9,   6,116,6,"3990"},

/* name         type model clas code prime a hd    r0    r1 har0   len sec    rps  f f1  f2   f3   f4 f5 f6  cu */
 {"9345-1",    0x9345,0x04,0x20,0x04, 1440,0,15,48174,46456,1184,48280,213,0x8b07, 2, 34,18,   7,   6,116,6,"9343"},
 {"9345",      0x9345,0x04,0x20,0x04, 1440,0,15,48174,46456,1184,48280,213,0x8b07, 2, 34,18,   7,   6,116,6,"9343"},
 {"9345-2",    0x9345,0x04,0x20,0x04, 2156,0,15,48174,46456,1184,48280,213,0x8b07, 2, 34,18,   7,   6,116,6,"9343"},
 {"9345-x",    0x9345,0x04,0x20,0x04,65535,0,15,48174,46456,1184,48280,213,0x8b07, 2, 34,18,   7,   6,116,6,"9343"}
} ;
#define CKDDEV_NUM (sizeof(ckdtab)/CKDDEV_SIZE)

/*-------------------------------------------------------------------*/
/*       FBA device definitions - courtesy of Tomas Masek            */
/*-------------------------------------------------------------------*/
static FBADEV fbatab[] = {
/* name          devt class type mdl  bpg bpp size   blks   cu     */
 {"3310-1",     0x3310,0x21,0x01,0x01, 32,352,512, 125664,0x4331},
 {"3310",       0x3310,0x21,0x01,0x01, 32,352,512, 125664,0x4331},
 {"3310-x",     0x3310,0x21,0x01,0x01, 32,352,512,      0,0x4331},

/* name          devt class type mdl  bpg bpp size   blks   cu     */
 {"3370-A1",    0x3370,0x21,0x02,0x00, 62,744,512, 558000,0x3880},
 {"3370-B1",    0x3370,0x21,0x02,0x00, 62,744,512, 558000,0x3880},
 {"3370-1",     0x3370,0x21,0x02,0x00, 62,744,512, 558000,0x3880},
 {"3370",       0x3370,0x21,0x02,0x00, 62,744,512, 558000,0x3880},
 {"3370-A2",    0x3370,0x21,0x05,0x04, 62,744,512, 712752,0x3880},
 {"3370-B2",    0x3370,0x21,0x05,0x04, 62,744,512, 712752,0x3880},
 {"3370-2",     0x3370,0x21,0x05,0x04, 62,744,512, 712752,0x3880},
 {"3370-x",     0x3370,0x21,0x05,0x04, 62,744,512,      0,0x3880},

/* name          devt class type mdl  bpg bpp size   blks   cu     */
 {"9313-1",     0x9313,0x21,0x08,0x00, 96,480,512, 246240,0x6310},
 {"9313",       0x9313,0x21,0x08,0x00, 96,480,512, 246240,0x6310},
 {"9313-x",     0x9313,0x21,0x08,0x00, 96,480,512,      0,0x6310},

/* name          devt class type mdl  bpg bpp size   blks   cu     */
 {"9332-200",   0x9332,0x21,0x07,0x00, 73,292,512, 360036,0x6310},
 {"9332-400",   0x9332,0x21,0x07,0x00, 73,292,512, 360036,0x6310},
 {"9332-1",     0x9332,0x21,0x07,0x00, 73,292,512, 360036,0x6310},
 {"9332",       0x9332,0x21,0x07,0x00, 73,292,512, 360036,0x6310},
 {"9332-600",   0x9332,0x21,0x07,0x01, 73,292,512, 554800,0x6310},
 {"9332-x",     0x9332,0x21,0x07,0x01, 73,292,512,      0,0x6310},

/* name          devt class type mdl  bpg bpp size   blks   cu     */
 {"9335-1",     0x9335,0x21,0x06,0x01, 71,426,512, 804714,0x6310},
 {"9335",       0x9335,0x21,0x06,0x01, 71,426,512, 804714,0x6310},
 {"9335-x",     0x9335,0x21,0x06,0x01, 71,426,512,      0,0x6310},

/* name          devt class type mdl  bpg bpp size   blks   cu     */
 {"9336-10",    0x9336,0x21,0x11,0x00, 63,315,512, 920115,0x6310},
 {"9336-1",     0x9336,0x21,0x11,0x00, 63,315,512, 920115,0x6310},
 {"9336",       0x9336,0x21,0x11,0x00, 63,315,512, 920115,0x6310},
 {"9336-20",    0x9336,0x21,0x11,0x10,111,777,512,1672881,0x6310},
 {"9336-25",    0x9336,0x21,0x11,0x10,111,777,512,1672881,0x6310},
 {"9336-x",     0x9336,0x21,0x11,0x10,111,777,512,      0,0x6310},

/* name          devt class type mdl  bpg bpp size   blks   cu     */
 {"0671-1",     0x0671,0x21,0x12,0x00, 63,504,512, 574560,0x6310},
 {"0671",       0x0671,0x21,0x12,0x00, 63,504,512, 574560,0x6310},
 {"0671-04",    0x0671,0x21,0x12,0x04, 63,504,512, 624456,0x6310},
 {"0671-08",    0x0671,0x21,0x12,0x08, 63,504,512, 513072,0x6310},
 {"0671-x",     0x0671,0x21,0x12,0x04, 63,504,512,      0,0x6310}
} ;
#define FBADEV_NUM (sizeof(fbatab)/FBADEV_SIZE)

#if defined(FEATURE_VM_BLOCKIO)
static BLKTAB blktab[] = {
#if 0 /* Remove conditional compilation when CKD devices supported */
   CKDIOT("2305",0x2305,15,10,5,3),
   CKDIOT("2311",0x2311,6,3,1,0),
   CKDIOT("2314",0x2314,11,6,3,1),
   CKDIOT("3330",0x3330,20,11,6,3),
   CKDIOT("3340",0x3340,12,7,3,2),
   CKDIOT("3350",0x3350,27,15,8,4),
   CKDIOT("3375",0x3375,40,25,14,8),
   CKDIOT("3380",0x3380,35,23,14,7),
   CKDIOT("3390",0x3390,49,33,21,12),
   CKDIOT("9345",0x9345,41,28,17,9),
#endif
   FBAIOT("0671",0x0671),
   FBAIOT("3310",0x3310),
   FBAIOT("3370",0x3370),
   FBAIOT("9332",0x9332),
   FBAIOT("9335",0x9335),
   FBAIOT("9336",0x9336)
};
#define BLKTAB_NUM (sizeof(blktab)/BLKTAB_SIZE)

#endif /* defined(FEATURE_VM_BLOCKIO) */


/*-------------------------------------------------------------------*/
/* Lookup a table entry either by name or type                       */
/*-------------------------------------------------------------------*/
DLL_EXPORT void *dasd_lookup (int dtype, char *name, U32 devt, U32 size)
{
U32 i;                                  /* Loop Index                */

    switch (dtype) {

    case DASD_CKDDEV:
        for (i = 0; i < (int)CKDDEV_NUM; i++)
        {
            if ((name && !strcmp(name, ckdtab[i].name))
             || (((U32)devt == (U32)ckdtab[i].devt || (U32)devt == (U32)(ckdtab[i].devt & 0xff))
              && (U32)size <= (U32)(ckdtab[i].cyls + ckdtab[i].altcyls)))
                return &ckdtab[i];
        }
        return NULL;

    case DASD_CKDCU:
        for (i = 0; i < (int)CKDCU_NUM; i++)
        {
            if ((name != NULL && strcmp(name, ckdcutab[i].name) == 0)
             || devt == ckdcutab[i].devt)
                return &ckdcutab[i];
        }
        return NULL;

    case DASD_FBADEV:
        for (i = 0; i < (int)FBADEV_NUM; i++)
        {
            if ((name && !strcmp(name, fbatab[i].name))
             || (((U32)devt == (U32)fbatab[i].devt || (U32)devt == (U32)(fbatab[i].devt & 0xff))
              && ((size <= fbatab[i].blks) || (fbatab[i].blks == 0))))
                return &fbatab[i];
        }
        return NULL;
#if defined(FEATURE_VM_BLOCKIO)
    case DASD_STDBLK:
        for (i = 0; i < (int)BLKTAB_NUM; i++)
        {
            if ((name && !strcmp(name, blktab[i].name)) ||
                (U32)devt == (U32)blktab[i].devt ||
                (U32)devt == (U32)(blktab[i].devt & 0xff))
                return &blktab[i];
        }
        return NULL;
#endif /* defined(FEATURE_VM_BLOCKIO) */
    default:
        return NULL;
    }

    UNREACHABLE_CODE( return NULL );
}

/*-------------------------------------------------------------------*/
/* Build CKD devid field                                             */
/*-------------------------------------------------------------------*/
int dasd_build_ckd_devid (CKDDEV *ckd, CKDCU *cu, BYTE *devid)
{
int len;

    memset( devid, 0, 256 );

    store_fw (devid + 0, 0xFF000000 | (cu->devt << 8) | cu->model);
    store_fw (devid + 4, (ckd->devt << 16) | (ckd->model << 8) | 0x00);
    store_fw (devid + 8, cu->ciw1);
    store_fw (devid +12, cu->ciw2);
    store_fw (devid +16, cu->ciw3);
    store_fw (devid +20, cu->ciw4);
    store_fw (devid +24, cu->ciw5);
    store_fw (devid +28, cu->ciw6);
    store_fw (devid +32, cu->ciw7);
    store_fw (devid +36, cu->ciw8);

    /* Set length */
    if (cu->ciw8 != 0) len = 40;
    else if (cu->ciw7 != 0) len = 36;
    else if (cu->ciw6 != 0) len = 32;
    else if (cu->ciw5 != 0) len = 28;
    else if (cu->ciw4 != 0) len = 24;
    else if (cu->ciw3 != 0) len = 20;
    else if (cu->ciw2 != 0) len = 16;
    else if (cu->ciw1 != 0) len = 12;
    else len = 7;

    /* If LEGACYSENSEID ENABLE is not set and device is a 2311
       or 2314, set devid length to zero to force command reject
       response to 0xE4 Sense ID command */
    if (1
        && !sysblk.legacysenseid
        && (0
            || 0x2311 == ckd->devt
            || 0x2314 == ckd->devt
           )
        )
    {
      len = 0;
    }

    return len;
}


/*-------------------------------------------------------------------*/
/* Build CKD devchar field                                           */
/*-------------------------------------------------------------------*/
int dasd_build_ckd_devchar (CKDDEV *ckd, CKDCU *cu, BYTE *devchar,
                            int cyls)
{
int altcyls;                            /* Number alternate cyls     */

    if (cyls > ckd->cyls) altcyls = cyls - ckd->cyls;
    else altcyls = 0;

    memset( devchar, 0, 64 );
    store_hw(devchar+0, cu->devt);              // Storage control type
    devchar[2]  = cu->model;                    // CU model
    store_hw(devchar+3, ckd->devt);             // Device type
    devchar[5]  = ckd->model;                   // Device model
    store_fw(devchar+6, cu->sctlfeat |          // Device and SD facilities
      (cu->devt == 0x3990 &&                    // ... or in 24-byte sense
       ckd->devt == 0x3380));                   // ... compatibility for 3380
                                                // ... hosted on 3990 controller
    devchar[10] = ckd->devclass;                // Device class code:
                                                // (always X'20'=DASD)
    devchar[11] = ckd->code;                    // Device type code
    store_hw(devchar+12, cyls - altcyls);       // Primary cylinders
    store_hw(devchar+14, ckd->heads);           // Tracks per cylinder
    devchar[16] = (BYTE)(ckd->sectors);         // Number of sectors
    store_hw(devchar+18, ckd->len);             // Track length
    store_hw(devchar+20, ckd->har0);            // Length of HA and R0
    if (ckd->formula == 1)
    {
        devchar[22] = (BYTE)(ckd->formula);     // Track capacity formula
        devchar[23] = (BYTE)(ckd->f1);          // Factor F1
        store_hw(devchar+24, ckd->f2);          // Factor F2
        store_hw(devchar+26, ckd->f3);          // Factor F3
    }
    else if (ckd->formula == 2)
    {
        devchar[22] = (BYTE)(ckd->formula);     // Track capacity formula
        devchar[23] = (BYTE)(ckd->f1);          // Factor F1
        devchar[24] = (BYTE)(ckd->f2);          // Factor F2
        devchar[25] = (BYTE)(ckd->f3);          // Factor F3
        devchar[26] = (BYTE)(ckd->f4);          // Factor F4
        devchar[27] = (BYTE)(ckd->f5);          // Factor F5
    }
    if (altcyls > 0)
    {
        store_hw(devchar+28, cyls - altcyls);   // Alternate cylinder & tracks
        store_hw(devchar+30, altcyls * ckd->heads);
    }
    devchar[40] = ckd->code;                    // MDR record ID
    devchar[41] = ckd->code;                    // OBR record ID
    devchar[42] = cu->code;                     // CU Type Code
    devchar[43] = 0x02;                         // Parameter length
    store_hw(devchar+44, ckd->r0);              // Record 0 length
    devchar[47] = 0x01;                         // Track set
    devchar[48] = (BYTE)(ckd->f6);              // F6
    store_hw(devchar+49, ckd->rpscalc);         // RPS factor
    devchar[51] =  0x00;                        // reserved byte 51
    /*---------------------------------------------------------------*/
    /* 2013/01/09 Fish                                               */
    /* RDC bytes 51-53 are reserved for model-3, model-6 and higher  */
    /* control units and define a feature buffer lower/upper limit.  */
    /* The lower limit might not need to be precisely 0x0f, but it   */
    /* cannot be zero or else VSAM Extended Format Datasets cannot   */
    /* be created by SMS. The precise value is unknown but was 0x0f  */
    /* in previous versions of Hercules so we use that value. Bytes  */
    /* 52-53 were previously always zero but defining them to be a   */
    /* value larger than byte 51 seems to make logical sense since   */
    /* we know it's an upper limit. I was told 0x7f was reasonable.  */
    /*---------------------------------------------------------------*/
    if (MODEL3(cu) || MODEL6(cu))               // Model-3/-6/greater?
    {
        devchar[51] =  0x0f;                    // reserved (feature buffer lower limit)

        if (MODEL3(cu))                         // 3990-3
        {
            devchar[52] =  0x00;                // reserved (always zero)
            devchar[53] =  0x3f;                // reserved (feature buffer upper limit)
        }
        else                                    // 3990-6 or greater
        {
            devchar[52] =  0x00;                // reserved (high byte feature buffer upper limit)
            devchar[53] =  0x7f;                // reserved (low byte  feature buffer upper limit)
        }
    }
    devchar[54] = cu->funcfeat;                 // device/CU functions/features
    devchar[56] = cu->typecode;                 // Real CU type code
    /*---------------------------------------------------------------*/
    /* 2007/05/04 @kl                                                */
    /* The following line to set devchar[57] to 0xff was restored    */
    /* to circumvent a command reject when ICKDSF issues a Read      */
    /* Special Home Address (0x0a) to an alternate track.            */
    /* According to the IBM 3880 Storage Control Reference,          */
    /* GA16-1661-09, and the 3990/9330 Reference, GA32-0274-05,      */
    /* it should be 0x00 for real 3380 and 3390 devices.  Setting    */
    /* it to 0xff makes the underlying real DASD look like a         */
    /* disk array (whose virtual 3380/3390 disks have no alternate   */
    /* tracks).  This causes DSF to skip issuing the 0x0a channel    */
    /* command, which Hercules does not currently support, for       */
    /* alternate tracks.                                             */
    /*---------------------------------------------------------------*/
    devchar[57] = 0xff;                         // real device type code

    /*---------------------------------------------------------------*/
    /*  https://www.vm.ibm.com/pubs/cp720/RDCBK.HTML                 */
    /*                                                               */
    /*  NAME       : HCPRDCBK                                        */
    /*  DESCRIPTION: Real Device Characteristics Block               */
    /*  DSECT      : RDCBK                                           */
    /*   ...                                                         */
    /*  003A  58 Bitstring  2 RDC5859    DASD SPECIFIC RESERVED      */
    /*  003C  60 Signed     4 RDCMCYL32  NUMBER OF PRIMARY CYLINDERS */
    /*                                                               */
    /*---------------------------------------------------------------*/

    store_fw( devchar+60, cyls - altcyls );     // Primary cylinders

    return 64;
}


/*-------------------------------------------------------------------*/
/* Build CKD configuration data                                      */
/*-------------------------------------------------------------------*/
DLL_EXPORT int dasd_build_ckd_config_data (DEVBLK *dev, BYTE *iobuf, int count)
{
int  i;
BYTE buf[256];

    /* Clear the configuration data area */
    memset( buf, 0, 256 );

    /* NOTE: ckdtab and/or ckdcu could be NULL if this is a shared
       device that hasn't connected to the server yet (which would
       cause a crash), so we have to check.
    */
    if (dev->ckdtab && dev->ckdcu)
    {
        /* Bytes 0-31: NED 1  Node element descriptor for the device */
        store_fw (buf, 0xdc010100);
        sprintf ((char *)&buf[4], "  %4.4X0%2.2XHRCZZ",
                            dev->ckdtab->devt, dev->ckdtab->model);
        memcpy( &buf[18], dev->serial, 12 );
        for (i = 4; i < 30; i++)
            buf[i] = host_to_guest( isalnum( (unsigned char)buf[i] ) ? buf[i] : '0' );
        store_hw(buf + 30, dev->devnum);        /* Uniquely tag within system */

        /* Bytes 32-63: NED 2  Node element descriptor for the string */
        store_fw (buf + 32, 0xd4020000);
        sprintf ((char *)&buf[36], "  %4.4X0%2.2XHRCZZ000000000003",
                            dev->ckdtab->devt, dev->ckdtab->model);
        for (i = 36; i < 62; i++)
            buf[i] = host_to_guest(buf[i]);
        store_hw (buf + 62, 0x0000);

        /* Bytes 64-95: NED 3  Node element descriptor for the storage director */
        store_fw (buf + 64, 0xd0000000);
        sprintf ((char *)&buf[68], "  %4.4X0%2.2XHRCZZ000000000002",
                            dev->ckdcu->devt, dev->ckdcu->model);
        for (i = 68; i < 94; i++)
            buf[i] = host_to_guest(buf[i]);
        buf[94] = 0x00;
        buf[95] = (dev->devnum >> 8) & 0xFF;

        /* Bytes 96-127: NED 4  Node element descriptor for the subsystem */
        store_fw (buf + 96, 0xF0000001);
        sprintf ((char *)&buf[100], "  %4.4X   HRCZZ000000000001",
                            dev->ckdcu->devt);
        for (i = 100; i < 126; i++)
            buf[i] = host_to_guest(buf[i]);
        store_hw (buf + 126, 0x0000);

        /* Bytes 128-223: zeroes */

        /* Bytes 224-255: NEQ  Node Element Qualifier */
        buf[224] = 0x80;                  // flags (general NEQ)
        buf[225] = 0;                     // record selector
        store_hw (buf + 226, IFID(dev));  // interface id
        store_hw (buf + 228, 0);          // must be zero
        buf[230] = 0x1E;                  // primary missing interrupt timer interval
        buf[231] = 0x00;                  // secondary missing interrupt timer interval
        store_hw (buf + 232, SSID(dev));  // subsystem id
        buf[234] = 0x80;                  // path/cluster id
        buf[235] = (dev->devnum & 0xFF);  // unit address
        buf[236] = (dev->devnum & 0xFF);  // physical device id
        buf[237] = (dev->devnum & 0xFF);  // physical device address
        buf[238] = buf[227];              // SA ID (same as interface ID, byte 227)
        store_hw (buf + 239, 0);          // escon link address
        buf[241] = 0x80;                  // interface protocol type (parallel)
//      buf[241] = 0x40;                  // interface protocol type (escon)
        buf[242] = 0x80;                  // NEQ format flags
        buf[243] = (dev->devnum & 0xFF);  // logical device address (LDA)
                                          // bytes 244-255 must be zero
    }

    /* Copy data characteristics to the I/O buf */
    count = count > 256 ? 256 : count;
    memcpy (iobuf, buf, count);

    return 256;
}


/*-------------------------------------------------------------------*/
/* Build CKD subsystem status                                        */
/*-------------------------------------------------------------------*/
DLL_EXPORT int dasd_build_ckd_subsys_status (DEVBLK *dev, BYTE *iobuf, int count)
{
int  num;
BYTE buf[44];

    /* Build the basic subsystem status data in the I/O area */
    memset( buf, 0, 44 );
    buf[1] = dev->devnum & 0xFF;
    buf[2] = DEVICES_PER_SUBSYS - 1;
    store_hw (buf + 38, SSID(dev));
    num = 40;

    /* Build an additional 4 bytes of data for the 3990-6 */
    if (MODEL6(dev->ckdcu))
    {
        buf[0] = 0x01;            /* Set 3990-6 enhanced flag */
        num = 44;
    } /* end if(3990-6) */

    /* Copy subsystem status to the I/O buf */
    count = count > num ? num : count;
    memcpy (iobuf, buf, count);

    return num;
}


/*-------------------------------------------------------------------*/
/* Build FBA devid field                                             */
/*-------------------------------------------------------------------*/
int dasd_build_fba_devid (FBADEV *fba, BYTE *devid)
{
    memset( devid, 0, 256 );

    devid[0] = 0xff;
    store_hw( &devid[1], fba->cu );
    devid[3] = 0x01;                  /* assume model is 1 */
    store_hw( &devid[4], fba->devt );
    devid[6] = fba->model;

    return 7;
}

/*-------------------------------------------------------------------*/
/* Build FBA devchar field                                           */
/*-------------------------------------------------------------------*/
int dasd_build_fba_devchar (FBADEV *fba, BYTE *devchar, int blks)
{
    memset( devchar, 0, 64 );

    devchar[0]  = 0x30;                         // operation modes
    devchar[1]  = 0x08;                         // features
    devchar[2]  = fba->devclass;                // device class
    devchar[3]  = fba->type;                    // unit type

    store_hw( &devchar[ 4], (U16) fba->size );  // block size

    store_fw( &devchar[ 6],       fba->bpg );   // blks per cyclical group
    store_fw( &devchar[10],       fba->bpp );   // blks per access position
    store_fw( &devchar[14],         blks );     // blks under movable heads

    store_fw( &devchar[18],          0 );       // blks under fixed heads
    store_hw( &devchar[22],          0 );       // blks in alternate area
    store_hw( &devchar[24],          0 );       // blks in CE+SA areas
    store_hw( &devchar[26],          0 );       // cyclic period in ms
    store_hw( &devchar[28],          0 );       // min time to change access position in ms
    store_hw( &devchar[30],          0 );       // max time to change access position in ms

    return 32;
}
