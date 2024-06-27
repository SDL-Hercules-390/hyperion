/* TAPECCWS.C   (C) Copyright Roger Bowler, 1999-2012                */
/*              (C) Copyright TurboHercules, SAS 2011                */
/*              Hercules Tape Device Handler CCW Processing          */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* This module contains the CCW handling functions for tape devices. */
/* The subroutines in this module are called by the general tape     */
/* device handler (tapedev.c) when the tape format is AWSTAPE.       */
/*-------------------------------------------------------------------*/
/*-------------------------------------------------------------------*/
/* Reference information:                                            */
/* SG24-2506 IBM 3590 Tape Subsystem Technical Guide                 */
/* GA32-0331 IBM 3590 Hardware Reference                             */
/* GA32-0329 IBM 3590 Introduction and Planning Guide                */
/* SG24-2594 IBM 3590 Multiplatform Implementation                   */
/* ANSI INCITS 131-1994 (R1999) SCSI-2 Reference                     */
/* GA32-0127 IBM 3490E Hardware Reference                            */
/* GC35-0152 EREP Release 3.5.0 Reference                            */
/* SA22-7204 ESA/390 Common I/O-Device Commands                      */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"
#include "hercules.h"  /* need Hercules control blocks               */
#include "tapedev.h"   /* Main tape handler header file              */

//#define  ENABLE_TRACING_STMTS   1       // (Fish: DEBUGGING)
//#include "dbgtrace.h"                   // (Fish: DEBUGGING)

/*-------------------------------------------------------------------*/
/*         (forward declarations needed by below tables)             */
/*-------------------------------------------------------------------*/

extern  BYTE           TapeCommands3410 [];
extern  BYTE           TapeCommands3420 [];
extern  BYTE           TapeCommands3422 [];
extern  BYTE           TapeCommands3430 [];
extern  BYTE           TapeCommands3480 [];
extern  BYTE           TapeCommands3490 [];
extern  BYTE           TapeCommands3590 [];
extern  BYTE           TapeCommands9347 [];

extern  TapeSenseFunc  build_sense_3410;
extern  TapeSenseFunc  build_sense_3420;
        #define        build_sense_3422         build_sense_3420
        #define        build_sense_3430         build_sense_3420
extern  TapeSenseFunc  build_sense_3480_etal;
extern  TapeSenseFunc  build_sense_3490;
extern  TapeSenseFunc  build_sense_3590;
extern  TapeSenseFunc  build_sense_Streaming;

/*-------------------------------------------------------------------*/
/*                     TapeDevtypeList                               */
/* Format:                                                           */
/*                                                                   */
/*    A:    Supported Device Type,                                   */
/*    B:      Command table index, (TapeCommandTable)                */
/*    C:      UC on RewUnld,   (1/0 = true/false)                    */
/*    D:      CUE on RewUnld,  (1/0 = true/false)                    */
/*    E:      Sense Build Function table index (TapeSenseTable)      */
/*                                                                   */
/*-------------------------------------------------------------------*/

int  TapeDevtypeList [] =
{
   /*   A   B  C  D  E  */
    0x3410, 0, 1, 0, 0,
    0x3411, 0, 1, 0, 0,
    0x3420, 1, 1, 1, 1,
    0x3422, 2, 0, 0, 2,
    0x3430, 3, 0, 0, 3,
    0x3480, 4, 0, 0, 4,
    0x3490, 5, 0, 0, 5,
    0x3590, 6, 0, 0, 6,
    0x9347, 7, 0, 0, 7,
    0x9348, 7, 0, 0, 7,
    0x8809, 7, 0, 0, 7,
    0x0000, 0, 0, 0, 0      /* (end of table marker) */
};

/*-------------------------------------------------------------------*/
/*                       TapeCommandTable                            */
/*                                                                   */
/*  Specific supported CCW codes for each device type. Index is      */
/*  fetched by TapeCommandIsValid from "TapeDevtypeList[ n+1 ]".     */
/*                                                                   */
/*-------------------------------------------------------------------*/

BYTE*  TapeCommandTable [] =
{
     TapeCommands3410,      /*  0   3410/3411                        */
     TapeCommands3420,      /*  1   3420                             */
     TapeCommands3422,      /*  2   3422                             */
     TapeCommands3430,      /*  3   3430                             */
     TapeCommands3480,      /*  4   3480 (Maybe all 38K Tapes)       */
     TapeCommands3490,      /*  5   3490                             */
     TapeCommands3590,      /*  6   3590                             */
     TapeCommands9347,      /*  7   9347 (Maybe all streaming tapes) */
     NULL
};

/*-------------------------------------------------------------------*/
/*                       TapeSenseTable                              */
/*                                                                   */
/* SENSE function routing table. Index is fetched by 'build_senseX'  */
/* function from table entry "TapeDevtypeList[ i+4 ]".               */
/*-------------------------------------------------------------------*/

TapeSenseFunc*  TapeSenseTable  [] =
{
    build_sense_3410,       /*  0   3410/3411                        */
    build_sense_3420,       /*  1   3420                             */
    build_sense_3422,       /*  2   3422                             */
    build_sense_3430,       /*  3   3430                             */
    build_sense_3480_etal,  /*  4   3480 (Maybe all 38K Tapes)       */
    build_sense_3490,       /*  5   3490                             */
    build_sense_3590,       /*  6   3590                             */
    build_sense_Streaming,  /*  7   9347 (Maybe all streaming tapes) */
    NULL
};

/*-------------------------------------------------------------------*/
/*           CCW opcode Validity Tables by Device Type               */
/*-------------------------------------------------------------------*/
/*                                                                   */
/* The below tables are used by 'TapeCommandIsValid' to determine    */
/* if a CCW code is initially valid or not for the given device.     */
/*                                                                   */
/*    0: Command is NOT valid                                        */
/*  * 1: Command is Valid, Tape MUST be loaded                       */
/*  * 2: Command is Valid, Tape NEED NOT be loaded                   */
/*    3: Command is Valid, But is a NO-OP (return CE+DE now)         */
/*    4: Command is Valid, But is a NO-OP (for virtual tapes)        */
/*  * 5: Command is Valid, Tape MUST be loaded (add DE to status)    */
/*                                                                   */
/*  * Note that CCWs codes marked as valid might still get rejected  */
/*    upon more stringent validity testing done by the actual CCW    */
/*    processing function itself.                                    */
/*                                                                   */
/* SOURCES:                                                          */
/*                                                                   */
/*   GX20-1850-2 "S/370 Reference Summary"  (3410/3411/3420)         */
/*   GX20-0157-1 "370/XA Reference Summary" (3420/3422/3430/3480)    */
/*   GA33-1510-0 "S/370 Model 115 FC"       (3410/3411)              */
/*                                                                   */
/* Ivan Warren, 2003-02-24                                           */
/*-------------------------------------------------------------------*/

BYTE  TapeCommands3410 [256] =
{
/* 0 1 2 3 4 5 6 7 8 9 A B C D E F */
   0,1,1,1,2,0,0,5,0,0,0,0,1,0,0,5, /* 00 */
   0,0,0,4,0,0,0,1,0,0,0,1,0,0,0,1, /* 10 */
   0,0,0,4,0,0,0,1,0,0,0,4,0,0,0,1, /* 20 */
   0,0,0,4,0,0,0,1,0,0,0,4,0,0,0,1, /* 30 */
   0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,0, /* 40 */
   0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0, /* 50 */
   0,0,0,4,0,0,0,0,0,0,0,4,0,0,0,0, /* 60 */
   0,0,0,4,0,0,0,0,0,0,0,4,0,0,0,0, /* 70 */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 80 */
   0,0,0,4,0,0,0,1,0,0,0,0,0,0,0,0, /* 90 */
   0,0,0,4,0,0,0,0,0,0,0,4,0,0,0,0, /* A0 */
   0,0,0,4,0,0,0,0,0,0,0,4,0,0,0,0, /* B0 */
   0,0,0,4,0,0,0,0,0,0,0,4,0,0,0,0, /* C0 */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* D0 */
   0,0,0,0,2,0,0,0,0,0,0,3,0,0,0,0, /* E0 */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0  /* F0 */
};

BYTE  TapeCommands3420 [256] =
{
/* 0 1 2 3 4 5 6 7 8 9 A B C D E F */
   0,1,1,1,2,0,0,5,0,0,0,2,1,0,0,5, /* 00 */
   0,0,0,4,0,0,0,1,0,0,0,1,0,0,0,1, /* 10 */
   0,0,0,4,0,0,0,1,0,0,0,4,0,0,0,1, /* 20 */
   0,0,0,4,0,0,0,1,0,0,0,4,0,0,0,1, /* 30 */
   0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,0, /* 40 */
   0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0, /* 50 */
   0,0,0,4,0,0,0,0,0,0,0,4,0,0,0,0, /* 60 */
   0,0,0,4,0,0,0,0,0,0,0,4,0,0,0,0, /* 70 */
   0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,0, /* 80 */
   0,0,0,4,0,0,0,1,0,0,0,0,0,0,0,0, /* 90 */
   0,0,0,4,0,0,0,0,0,0,0,4,0,0,0,0, /* A0 */
   0,0,0,4,0,0,0,0,0,0,0,4,0,0,0,0, /* B0 */
   0,0,0,4,0,0,0,0,0,0,0,4,0,0,0,0, /* C0 */
   0,0,0,4,4,0,0,0,0,0,0,0,0,0,0,0, /* D0 */
   0,0,0,0,2,0,0,0,0,0,0,3,0,0,0,0, /* E0 */
   0,0,0,2,4,0,0,0,0,0,0,0,0,2,0,0  /* F0 */
};

BYTE  TapeCommands3422 [256] =
{
/* 0 1 2 3 4 5 6 7 8 9 A B C D E F */
   0,1,1,1,2,0,0,5,0,0,0,2,1,0,0,5, /* 00 */
   0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1, /* 10 */
   0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1, /* 20 */
   0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1, /* 30 */
   0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,0, /* 40 */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 50 */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 60 */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 70 */
   0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0, /* 80 */
   0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0, /* 90 */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* A0 */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* B0 */
   0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0, /* C0 */
   0,0,0,4,4,0,0,0,0,0,0,0,0,0,0,0, /* D0 */
   0,0,0,0,2,0,0,0,0,0,0,3,0,0,0,0, /* E0 */
   0,0,0,2,4,0,0,0,0,0,0,0,0,2,0,0  /* F0 */
};

BYTE  TapeCommands3430 [256] =
{
/* 0 1 2 3 4 5 6 7 8 9 A B C D E F */
   0,1,1,1,2,0,0,5,0,0,0,2,1,0,0,5, /* 00 */
   0,0,0,0,0,0,0,1,0,0,0,1,0,0,0,1, /* 10 */
   0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1, /* 20 */
   0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1, /* 30 */
   0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,0, /* 40 */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 50 */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 60 */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 70 */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 80 */
   0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0, /* 90 */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* A0 */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* B0 */
   0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0, /* C0 */
   0,0,0,4,4,0,0,0,0,0,0,0,0,0,0,0, /* D0 */
   0,0,0,0,2,0,0,0,0,0,0,3,0,0,0,0, /* E0 */
   0,0,0,2,4,0,0,0,0,0,0,0,0,2,0,0  /* F0 */
};

BYTE  TapeCommands3480 [256] =
{
/* 0 1 2 3 4 5 6 7 8 9 A B C D E F */
   3,1,1,1,2,0,0,5,0,0,0,0,1,0,0,5, /* 00 */
   0,0,1,3,0,0,0,1,0,0,0,0,0,0,0,1, /* 10 */
   0,0,1,3,2,0,0,1,0,0,0,3,0,0,0,1, /* 20 */
   0,0,0,3,2,0,0,1,0,0,0,3,0,0,0,1, /* 30 */
   0,0,0,1,0,0,0,0,0,0,0,2,0,0,0,1, /* 40 */
   0,0,0,3,0,0,0,0,0,0,0,3,0,0,0,0, /* 50 */
   0,0,0,3,2,0,0,0,0,0,0,3,0,0,0,0, /* 60 */
   0,0,0,3,0,0,0,2,0,0,0,3,0,0,0,0, /* 70 */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 80 */
   0,0,0,3,0,0,0,1,0,0,0,0,0,0,0,2, /* 90 */
   0,0,0,3,0,0,0,0,0,0,0,3,0,0,0,2, /* A0 */
   0,0,0,3,0,0,0,2,0,0,0,3,0,0,0,0, /* B0 */
   0,0,0,2,0,0,0,2,0,0,0,3,0,0,0,0, /* C0 */
   0,0,0,3,0,0,0,0,0,0,0,2,0,0,0,0, /* D0 */
   0,0,0,2,2,0,0,0,0,0,0,0,0,0,0,0, /* E0 */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0  /* F0 */
};

BYTE  TapeCommands3490 [256] =
{
/* 0 1 2 3 4 5 6 7 8 9 A B C D E F */
   3,1,1,1,2,0,0,5,0,0,0,0,1,0,0,5, /* 00 */
   0,0,1,3,0,0,0,1,0,0,0,0,0,0,0,1, /* 10 */
   0,0,1,3,2,0,0,1,0,0,0,3,0,0,0,1, /* 20 */
   0,0,0,3,2,0,0,1,0,0,0,3,0,0,2,1, /* 30 */
   0,0,0,1,0,0,0,0,0,0,0,2,0,0,2,1, /* 40 */
   0,0,0,3,0,0,0,0,0,0,0,2,0,0,0,0, /* 50 */
   0,0,0,3,2,0,0,0,0,0,0,3,0,0,0,0, /* 60 */
   0,0,0,3,0,0,0,2,0,0,0,3,0,0,0,0, /* 70 */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 80 */
   0,0,0,3,0,0,0,1,0,0,0,0,0,0,0,2, /* 90 */
   0,0,0,3,0,0,0,0,0,0,0,3,0,0,0,2, /* A0 */
   0,0,0,3,0,0,0,2,0,0,0,3,0,0,0,0, /* B0 */
   0,0,0,2,0,0,0,2,0,0,0,0,0,0,0,0, /* C0 */
   0,0,0,3,0,0,0,0,0,0,0,2,0,0,0,0, /* D0 */
   0,0,0,2,2,0,0,0,0,0,0,0,0,0,0,0, /* E0 */
   0,0,0,0,0,0,0,0,0,0,2,0,0,0,0,0  /* F0 */
};

BYTE  TapeCommands3590 [256] =
{
/* 0 1 2 3 4 5 6 7 8 9 A B C D E F */
   3,1,1,1,2,0,1,5,0,0,1,0,1,0,0,5, /* 00 */
   0,0,1,3,0,0,0,1,0,0,0,0,0,0,0,1, /* 10 */
   0,0,1,3,2,0,0,1,0,0,0,3,0,0,0,1, /* 20 */
   0,0,0,3,2,0,0,1,0,0,0,3,0,0,2,1, /* 30 */
   0,0,0,1,0,0,0,0,0,0,0,2,0,0,2,1, /* 40 */
   0,0,0,3,0,0,0,0,0,0,0,2,0,0,0,0, /* 50 */
   0,0,1,3,2,0,0,0,0,0,0,3,0,0,0,0, /* 60 */
   0,0,0,3,0,0,0,2,0,0,0,3,0,0,0,0, /* 70 */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 80 */
   0,0,0,3,0,0,0,1,0,0,0,0,0,0,0,2, /* 90 */
   0,0,0,3,0,0,0,0,0,0,0,3,0,0,0,2, /* A0 */
   0,0,0,3,0,0,0,2,0,0,0,3,0,0,0,0, /* B0 */
   0,0,2,2,0,0,0,2,0,0,0,3,0,0,0,3, /* C0 */  // CB=? CF=?
   0,0,0,3,0,0,0,0,0,0,0,2,0,0,0,0, /* D0 */
   0,0,0,2,2,0,0,0,0,0,0,0,0,0,0,0, /* E0 */
   0,0,0,0,0,0,0,0,0,0,2,0,0,0,0,0  /* F0 */
};

BYTE  TapeCommands9347 [256] =
{
/* 0 1 2 3 4 5 6 7 8 9 A B C D E F */
   0,1,1,1,2,0,0,5,0,0,0,2,1,0,0,5, /* 00 */
   0,0,0,4,0,0,0,1,0,0,0,1,0,0,0,1, /* 10 */
   0,0,0,4,0,0,0,1,0,0,0,4,0,0,0,1, /* 20 */
   0,0,0,4,0,0,0,1,0,0,0,4,0,0,0,1, /* 30 */
   0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,0, /* 40 */
   0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0, /* 50 */
   0,0,0,4,0,0,0,0,0,0,0,4,0,0,0,0, /* 60 */
   0,0,0,4,0,0,0,0,0,0,0,4,0,0,0,0, /* 70 */
   0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,0, /* 80 */
   0,0,0,4,0,0,0,1,0,0,0,0,0,0,0,0, /* 90 */
   0,0,0,4,2,0,0,0,0,0,0,4,0,0,0,0, /* A0 */
   0,0,0,4,0,0,0,0,0,0,0,4,0,0,0,0, /* B0 */
   0,0,0,4,0,0,0,0,0,0,0,4,0,0,0,0, /* C0 */
   0,0,0,4,4,0,0,0,0,0,0,0,0,0,0,0, /* D0 */
   0,0,0,0,2,0,0,0,0,0,0,3,0,0,0,0, /* E0 */
   0,0,0,2,4,0,0,0,0,0,0,0,0,2,0,0  /* F0 */
};

/*-------------------------------------------------------------------*/
/* Ivan Warren 20040227                                              */
/*                                                                   */
/* This table is used by channel.c to determine if a CCW code        */
/* is an immediate command or not.                                   */
/*                                                                   */
/* The table is addressed in the DEVHND structure as 'DEVIMM immed'  */
/*                                                                   */
/*     0:  ("false")  Command is *NOT* an immediate command          */
/*     1:  ("true")   Command *IS* an immediate command              */
/*                                                                   */
/* Note: An immediate command is defined as a command which returns  */
/* CE (channel end) during initialization (that is, no data is       */
/* actually transfered). In this case, IL is not indicated for a     */
/* Format 0 or Format 1 CCW when IL Suppression Mode is in effect.   */
/*                                                                   */
/*-------------------------------------------------------------------*/

BYTE  TapeImmedOther [256] =
{
/* 0 1 2 3 4 5 6 7 8 9 A B C D E F */
   0,0,0,1,0,0,0,1,0,0,0,0,0,0,0,1, /* 00 */
   0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1, /* 10 */
   0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1, /* 20 */
   0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1, /* 30 */
   0,0,0,1,0,0,0,1,0,0,0,0,0,0,0,0, /* 40 */
   0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1, /* 50 */
   0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1, /* 60 */
   0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,1, /* 70 */ /* Adrian Trenkwalder - 77 was 1 */
   0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1, /* 80 */
   0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0, /* 90 */
   0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0, /* A0 */
   0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,1, /* B0 */
   0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,1, /* C0 */
   0,0,0,1,0,0,0,1,0,0,0,0,0,0,0,1, /* D0 */
   0,0,0,0,0,0,0,1,0,0,0,1,0,0,0,1, /* E0 */
   0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1  /* F0 */
};

BYTE  TapeImmed3590 [256] =               /* (modified copy of the above) */
{
/* 0 1 2 3 4 5 6 7 8 9 A B C D E F */
   0,0,0,1,0,0,0,1,0,0,0,0,0,0,0,1, /* 00 */
   0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1, /* 10 */
   0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1, /* 20 */
   0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1, /* 30 */
   0,0,0,1,0,0,0,1,0,0,0,0,0,0,0,0, /* 40 */
   0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1, /* 50 */
   0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1, /* 60 */
   0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,1, /* 70 */
   0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1, /* 80 */
   0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0, /* 90 */
   0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0, /* A0 */
   0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,1, /* B0 */
   0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0, /* C0 */    // CB=? CF=?
   0,0,0,1,0,0,0,1,0,0,0,0,0,0,0,1, /* D0 */
   0,0,0,0,0,0,0,1,0,0,0,1,0,0,0,1, /* E0 */
   0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,1  /* F0 */
};

/*-------------------------------------------------------------------*/
/*                   TapeCommandIsValid      (Ivan Warren 20030224)  */
/*-------------------------------------------------------------------*/
/*                                                                   */
/* Determine if a CCW code is valid for the Device                   */
/*                                                                   */
/*   rc 0:   is *NOT* valid                                          */
/*   rc 1:   is Valid, tape MUST be loaded                           */
/*   rc 2:   is Valid, tape NEED NOT be loaded                       */
/*   rc 3:   is Valid, But is a NO-OP (Return CE+DE now)             */
/*   rc 4:   is Valid, But is a NO-OP for virtual tapes              */
/*   rc 5:   is Valid, Tape Must be loaded - Add DE to status        */
/*   rc 6:   is Valid, Tape load attempted - but not an error        */
/*           (used for sense and no contingency allegiance exists)   */
/*                                                                   */
/*-------------------------------------------------------------------*/
int TapeCommandIsValid (BYTE code, U16 devtype, BYTE *rustat)
{
int i, rc, tix = 0, devtfound = 0;

    /*
    **  Find the D/T in the table
    **  If not found, treat as invalid CCW code
    */

    *rustat = 0;

    for (i = 0; TapeDevtypeList[i] != 0; i += TAPEDEVTYPELIST_ENTRYSIZE)
    {
        if (TapeDevtypeList[i] == devtype)
        {
           devtfound = 1;
           tix = TapeDevtypeList[i+1];

           if (TapeDevtypeList[i+2])
           {
               *rustat |= CSW_UC;
           }
           if (TapeDevtypeList[i+3])
           {
               *rustat |= CSW_CUE;
           }
           break;
        }
    }

    if (!devtfound)
        return 0;

    rc = TapeCommandTable[tix][code];

    return rc;

} /* end function TapeCommandIsValid */


/*********************************************************************/
/*********************************************************************/
/**                                                                 **/
/**               MAIN TAPE CCW PROCESSING FUNCTION                 **/
/**                                                                 **/
/*********************************************************************/
/*********************************************************************/

static TAMDIR* findtamdir( int rej, int minlen, const char* pszDir );

void tapedev_execute_ccw (DEVBLK *dev, BYTE code, BYTE flags,
        BYTE chained, U32 count, BYTE prevcode, int ccwseq,
        BYTE *iobuf, BYTE *more, BYTE *unitstat, U32 *residual)
{
int     rc;                             /* Return code               */
U32     len;     /*(see NOTE below)*/   /* Length of data block      */
U32     num;     /*(see NOTE below)*/   /* Number of bytes to read   */
int     drc;                            /* code disposition          */
BYTE    rustat;                         /* Addl CSW stat on RewUnld  */

    UNREFERENCED( ccwseq );

    /* PROGRAMMING NOTE: len, num, count, *residual and *more are
       used and updated automatically by the RESIDUAL_CALC macro */

    /* Reset flags at start of CCW chain */
    if (dev->ccwseq == 0)
    {
        dev->supvr_inhibit = 0;         /* (reset to default mode)   */
        dev->write_immed   = 0;         /* (reset to default mode)   */
        dev->tapssdlen     = 0;         /* (clear all subsys data)   */
    }

    /* If this is a data-chained READ, then return any data remaining
       in the buffer which was not used by the previous CCW */
    if (chained & CCW_FLAGS_CD)
    {
        /* Calculate number of bytes to read and residual byte count */
        RESIDUAL_CALC (dev->curblkrem);

        if (IS_CCW_RDBACK(code))
        {
            /* We don't need to move anything in this case - just set length */
        }
        else
        {
            memcpy (iobuf, dev->buf + dev->curbufoff, num);
        }

        /* Save size and offset of data not used by this CCW */
        dev->curblkrem -= num;
        dev->curbufoff += num;

        *unitstat = CSW_CE | CSW_DE;
        return;
    }

    /* Command reject if data chaining and command is not a read type */
    if ((flags & CCW_FLAGS_CD) &&
        !(IS_CCW_READ(code) || IS_CCW_RDBACK(code)))
    {
        WRMSG(HHC00212, "E", LCSS_DEVNUM, dev->filename, TTYPSTR(dev->tapedevt), code);
        build_senseX(TAPE_BSENSE_BADCOMMAND,dev,unitstat,code);
        return;
    }

    /* Command reject if command is not Read Subsystem Data command
       if the previous one was a Perform Subsystem Function command
       that prepared some subsystem data for subsequent reading
    */
    if (0x77 == prevcode && dev->tapssdlen && 0x3E != code)
    {
        build_senseX (TAPE_BSENSE_BADCOMMAND, dev, unitstat, code);
        return;
    }

    /* Early determination of CCW validity via TapeCommandTable lookup... */

    drc = TapeCommandIsValid (code, dev->devtype, &rustat);

    switch (drc)
    {
        default:    /* Should NOT occur! */

            ASSERT(0);  // (fall thru to case 0 = unsupported)
            /* FALLTHRU */

        case 0:     /* Unsupported CCW code for given device-type */

            build_senseX (TAPE_BSENSE_BADCOMMAND, dev, unitstat, code);
            return;

        case 1:     /* Valid - Tape MUST be loaded                    */
            break;

        case 2:     /* Valid - Tape NEED NOT be loaded                */
            break;

        case 3:     /* Valid - But is a NO-OP (return CE+DE now) */

            /* Command reject if the volume is currently fenced */
            if (dev->fenced)
            {
                build_senseX (TAPE_BSENSE_FENCED, dev, unitstat, code);
                return;
            }

            build_senseX (TAPE_BSENSE_STATUSONLY, dev, unitstat, code);
            return;

        case 4:     /* Valid, But is a NO-OP (for virtual tapes) */

            /* Command reject if the volume is currently fenced */
            if (dev->fenced)
            {
                build_senseX (TAPE_BSENSE_FENCED, dev, unitstat, code);
                return;
            }

            /* If non-virtual (SCSI) then further processing required */
            if (dev->tapedevt == TAPEDEVT_SCSITAPE)
                break;

            build_senseX (TAPE_BSENSE_STATUSONLY, dev, unitstat, code);
            return;

        case 5:     /* Valid - Tape MUST be loaded (add DE to status) */
            break;
    }
    // end switch (drc)

    /* Verify a tape is loaded if that is required for this CCW... */

    if ((1 == drc || 5 == drc) &&                               // (tape MUST be loaded?)
        (dev->fd < 0 || TAPEDEVT_SCSITAPE == dev->tapedevt))    // (no tape loaded or non-virtual?)
    {
        *residual = count;

        /* Error if tape unloaded */
        if (!strcmp (dev->filename, TAPE_UNLOADED))
        {
            build_senseX (TAPE_BSENSE_TAPEUNLOADED, dev, unitstat, code);
            return;
        }

        /* Open the device file if necessary */
        if (dev->fd < 0)
        {
            rc = dev->tmh->open( dev, unitstat, code );

            if (rc < 0)     /* Did open fail? */
            {
                return;     /* Yes, exit with unit status */
            }
        }

        /* Error if tape is not loaded */
        if (!dev->tmh->tapeloaded( dev, unitstat, code ))
        {
            build_senseX (TAPE_BSENSE_TAPEUNLOADED, dev, unitstat, code);
            return;
        }
    }

    /* Process depending on CCW opcode */
    switch (code) {

    /*---------------------------------------------------------------*/
    /* MODE SET   (pre-3480 and earlier drives)                      */
    /*---------------------------------------------------------------*/
        /* Patch to no-op modeset 1 (7-track) commands -             */
        /*   causes VM problems                                      */
        /* Andy Norrie 2002/10/06                                    */
    case 0x13:
    case 0x23:
    case 0x33:
    case 0x3B:
    case 0x53:
    case 0x63:
    case 0x6B:
//  case 0x73:  // Mode Set (7-track 556/Odd/Normal) for 3420-3/5/7
                // with 7-track feature installed, No-op for 3420-2/4/6
                // and 3480, Invalid for 3422/3430, "Set Interface
                // Identifier" for 3490 and later. NOTE: 3480 and earlier
                // interpretation handled by command-table; 3490 and
                // and later handled further below.
    case 0x7B:
    case 0x93:
    case 0xA3:
    case 0xAB:
    case 0xB3:
    case 0xBB:
//  case 0xC3:  // Mode Set (9-track 1600 bpi) for models earlier than
                // 3480, "Set Tape-Write-Immediate" for 3480 and later.
                // NOTE: handled by command-table for all models earlier
                // than 3480; 3480 and later handled further below.
//  case 0xCB:  // "Mode Set" (9-track 800 bpi) for some models (3410,
                // 3420, etc.) but some other unknown command for other
                // models (3480, 3590, etc) and completely invalid for
                // yet other models (3422, 3430). For all device types
                // except 3590, this code is handled by command-table
                // and not here. For 3590, it is handled further below.
    case 0xD3: /* 9-track 6250 bpi */
    case 0xEB: /* invalid mode set issued by DOS/VS */
    {
        build_senseX(TAPE_BSENSE_STATUSONLY,dev,unitstat,code);
        break;
    }

    /*---------------------------------------------------------------*/
    /* WRITE                                                         */
    /*---------------------------------------------------------------*/
    case 0x01:
    {
        /* Command reject if the volume is currently fenced */
        if (dev->fenced)
        {
            build_senseX (TAPE_BSENSE_FENCED, dev, unitstat, code);
            break;
        }

        /* Unit check if tape is write-protected */
        if (dev->readonly || dev->tdparms.logical_readonly)
        {
            build_senseX (TAPE_BSENSE_WRITEPROTECT, dev, unitstat, code);
            break;
        }

        /* Update matrix display if needed */
        if ( TAPEDISPTYP_WAITACT == dev->tapedisptype )
        {
            dev->tapedisptype = TAPEDISPTYP_IDLE;
            UpdateDisplay( dev );
        }

        /* Assign a unique Message Id for this I/O if needed */
        INCREMENT_MESSAGEID(dev);

        /* Write a block to the tape according to device type */
        if ((rc = dev->tmh->write( dev, iobuf, count, unitstat, code)) < 0)
            break;      // (error)

        *residual = 0;

        /* Perform flush/sync and/or set normal completion status */
        if (0
            || !dev->write_immed
            || (rc = dev->tmh->sync( dev, unitstat, code )) == 0
        )
            build_senseX( TAPE_BSENSE_STATUSONLY, dev, unitstat, code );

        break;
    }

    /*---------------------------------------------------------------*/
    /* READ FORWARD  (3590 only)                                     */
    /*---------------------------------------------------------------*/
    case 0x06:
    {
        if (!RDFWD_SUPPORTED())
        {
            build_senseX (TAPE_BSENSE_BADCOMMAND, dev, unitstat, code);
            break;
        }

        /*   SG24-2506 IBM 3590 Tape Subsystem Technical Guide

        5.2.1 Separate Channel Commands for IPL Read and Normal Read

        On IBM 3480/3490 tape devices there is only one Read Forward
        CCW, the X'02' command code.  This CCW is used to perform
        not only normal read operations but also an IPL Read from
        tape, for example, DFSMSdss Stand-Alone Restore.  When the
        CCW is used as an IPL Read, it is not subject to resetting
        event notification, by definition.  Because there is only
        one Read Forward CCW, it cannot be subject to resetting event
        notification on IBM 3480 and 3490 devices.

        To differentiate between an IPL Read and a normal read
        forward operation, the X'02' command code has been redefined
        to be the IPL Read CCW, and a new X'06' command code has been
        defined to be the Read Forward CCW.  The new Read Forward
        CCW, X'06', is subject to resetting event notification, as
        should be the case for normal read CCWs issued by applications
        or other host software.
        */

        // PROGRAMMING NOTE: I'm not sure what they mean by "resetting
        // event notification" above, but for now we'll just FALL THROUGH
        // to the below IPL READ logic...
    }

    /* (purposely FALL THROUGH to below IPL READ logic for now) */
    /* comment Below to silent GCC warning */
    /* FALLTHRU */

    /*---------------------------------------------------------------*/
    /* IPL READ  (non-3590)                                          */
    /*---------------------------------------------------------------*/
    case 0x02:
    {
        /* Command reject if the volume is currently fenced */
        if (dev->fenced)
        {
            build_senseX (TAPE_BSENSE_FENCED, dev, unitstat, code);
            break;
        }

        /* Update matrix display if needed */
        if ( TAPEDISPTYP_WAITACT == dev->tapedisptype )
        {
            dev->tapedisptype = TAPEDISPTYP_IDLE;
            UpdateDisplay( dev );
        }

        /* Assign a unique Message Id for this I/O if needed */
        INCREMENT_MESSAGEID(dev);

        /* Read a block from the tape according to device type */
        /* Exit with unit check status if read error condition */
        if ((S32)(len = dev->tmh->read( dev, dev->buf, unitstat, code)) < 0)
            break;      // (error)

        /* Calculate number of bytes to read and residual byte count */
        RESIDUAL_CALC (len);

        /* Copy data to I/O buffer */
        memcpy (iobuf, dev->buf, num);

        /* Save size and offset of data not used by this CCW */
        dev->curblkrem = len - num;
        dev->curbufoff = num;
        dev->buflen = len;

        /* Exit with unit exception status if tapemark was read */
        if (len == 0)
            build_senseX (TAPE_BSENSE_READTM, dev, unitstat, code);
        else
            build_senseX (TAPE_BSENSE_STATUSONLY, dev, unitstat, code);

        break;
    }

    /*---------------------------------------------------------------*/
    /* CONTROL NO-OPERATION                                          */
    /*---------------------------------------------------------------*/
    case 0x03:
    {
        /* Command reject if the volume is currently fenced */
        if (dev->fenced)
        {
            build_senseX (TAPE_BSENSE_FENCED, dev, unitstat, code);
            break;
        }

        build_senseX (TAPE_BSENSE_STATUSONLY, dev, unitstat, code);
        break;
    }

    /*---------------------------------------------------------------*/
    /* SENSE                                                         */
    /*---------------------------------------------------------------*/
    case 0x04:
    {
        /* Calculate residual byte count */
        RESIDUAL_CALC (dev->numsense);

        /* If we don't already have some sense already pre-built
           and ready and waiting, then we'll have to build it fresh
           for this call...  Otherwise, we use whatever we already
           have waiting for them pre-built from a previous call...
        */
        if (!dev->sns_pending)
            build_senseX (TAPE_BSENSE_UNSOLICITED, dev, unitstat, code);

        *unitstat = CSW_CE|CSW_DE;  /* Need to do this ourselves as  */
                                    /* we might not have gone thru   */
                                    /* build_senseX...               */

        /* Copy device sense bytes to channel I/O buffer, clear
           them for the next time, and then finally, reset the
           Contengent Allegiance condition... */
        memcpy (iobuf, dev->sense, num);
        memset (dev->sense, 0, sizeof(dev->sense));

        dev->sns_pending = 0;

        break;
    }

    /*---------------------------------------------------------------*/
    /* READ FORWARD  (3590 only)                                     */
    /*---------------------------------------------------------------*/
//  case 0x06:
//  {
        // (handled by case 0x02: IPL READ)
//  }

    /*---------------------------------------------------------------*/
    /* REWIND                                                        */
    /*---------------------------------------------------------------*/
    case 0x07:
    {
        /* Update matrix display if needed */
        if ( TAPEDISPTYP_IDLE    == dev->tapedisptype ||
             TAPEDISPTYP_WAITACT == dev->tapedisptype )
        {
            dev->tapedisptype = TAPEDISPTYP_REWINDING;
            UpdateDisplay( dev );
        }

        /* Assign a unique Message Id for this I/O if needed */
        INCREMENT_MESSAGEID(dev);

        /* Do the rewind */
        rc = dev->tmh->rewind( dev, unitstat, code);

        /* Update matrix display if needed */
        if ( TAPEDISPTYP_REWINDING == dev->tapedisptype )
        {
            dev->tapedisptype = TAPEDISPTYP_IDLE;
            UpdateDisplay( dev );
        }

        /* Check for error */
        if (rc < 0)
        {
            if (!SVF_ENABLED())
                dev->fenced = 1;  // (position lost; fence the volume)
            break;
        }

        dev->eotwarning = 0;
        dev->fenced = 0;

        build_senseX (TAPE_BSENSE_STATUSONLY, dev, unitstat, code);
        break;
    }

    /*---------------------------------------------------------------*/
    /* READ PREVIOUS  (3590)                                         */
    /*---------------------------------------------------------------*/
    case 0x0A:
    {
        /*    SG24-2506 IBM 3590 Tape Subsystem Technical Guide

        5.2.2 Read Previous to Replace Read Backward:

        The ESCON-attached Magstar tape drive does not support the
        Read Backward CCW (command code, X'0C').  It supports a new
        Read Previous CCW that allows processing of an IBM 3590 High
        Performance Tape Cartridge in the backward direction without
        the performance penalties that exist with the Read Backward
        CCW.  IBM 3480 and 3490 devices had to reread the physical
        block from the medium for each request of a logical block.
        The Magstar tape drive retains the physical block in the
        device buffer and satisfies any subsequent Read Previous from
        the buffer, similar to how Read Forward operates.  The Read
        Previous CCW operates somewhat like the Read Backward CCW
        in that it can be used to process the volumes in the backward
        direction.  It is different from the Read Backward, however,
        because the data is transferred to the host in the same order
        in which it was written, rather than in reverse order like
        Read Backward.
        */

        /*   SG24-2594 IBM 3590 Multiplatform Implementation

        5.1.2 New and Changed Read Channel Commands

        [...] That is, the Read Backward command's data address
        will point to the end of the storage area, while a Read
        Previous command points to the beginning of the storage
        area...
        */

        // PROGRAMMING NOTE: luckily, channel.c's buffer handling
        // causes transparent handling of Read Backward/Reverse,
        // so the above buffer alignment and data transfer order
        // is not a concern for us here.

        // PROGRAMMING NOTE: until we can add support to Hercules
        // allowing direct SCSI i/o (so that we can issue the 'Read
        // Reverse' command directly to the SCSI device), we will
        // simply FALL THROUGH to our existing "Read Backward" logic.
    }

    // (purposely FALL THROUGH to the 'READ BACKWARD' logic below)

    /*---------------------------------------------------------------*/
    /* READ BACKWARD                                                 */
    /*---------------------------------------------------------------*/
    case 0x0C:
    {
        /* Update matrix display if needed */
        if ( TAPEDISPTYP_WAITACT == dev->tapedisptype )
        {
            dev->tapedisptype = TAPEDISPTYP_IDLE;
            UpdateDisplay( dev );
        }

        /* Backspace to previous block according to device type */
        /* Exit with unit check status if error condition */
        if ((rc = dev->tmh->bsb( dev, unitstat, code )) < 0)
            break;      // (error)

        /* Exit with unit exception status if tapemark was sensed */
        if (rc == 0)
        {
            *residual = 0;
            build_senseX (TAPE_BSENSE_READTM, dev, unitstat, code);
            break;
        }

        /* Assign a unique Message Id for this I/O if needed */
        INCREMENT_MESSAGEID(dev);

        /* Now read in a forward direction the actual data block
           we just backspaced over, and exit with unit check status
           on any read error condition
        */
        if ((S32)(len = dev->tmh->read( dev, dev->buf, unitstat, code )) < 0)
            break;      // (error)

        /* Calculate number of bytes to read and residual byte count */
        RESIDUAL_CALC (len);

        /* Copy data to I/O buffer */
        memcpy (iobuf, dev->buf, num);

        /* Save size and offset of data not used by this CCW */
        dev->curblkrem = len - num;
        dev->curbufoff = num;
        dev->buflen = len;

        /* Backspace to previous block according to device type,
           and exit with unit check status if error condition */
        if ((rc = dev->tmh->bsb( dev, unitstat, code )) < 0)
            break;      // (error)

        /* Set normal status */
        build_senseX (TAPE_BSENSE_STATUSONLY, dev, unitstat, code);
        break;

    } /* End case 0x0C: READ BACKWARD */

    /*---------------------------------------------------------------*/
    /* REWIND UNLOAD                                                 */
    /*---------------------------------------------------------------*/
    case 0x0F:
    {
        /* Update matrix display if needed */
        if ( dev->tdparms.displayfeat )
        {
            if ( TAPEDISPTYP_UMOUNTMOUNT == dev->tapedisptype )
            {
                dev->tapedisptype   = TAPEDISPTYP_MOUNT;
                dev->tapedispflags |= TAPEDISPFLG_REQAUTOMNT;
                STRLCPY( dev->tapemsg1, dev->tapemsg2 );
            }
            else if ( TAPEDISPTYP_UNMOUNT == dev->tapedisptype )
            {
                dev->tapedisptype = TAPEDISPTYP_IDLE;
            }
        }

        if ( TAPEDISPTYP_IDLE    == dev->tapedisptype ||
             TAPEDISPTYP_WAITACT == dev->tapedisptype )
        {
            dev->tapedisptype = TAPEDISPTYP_UNLOADING;
            UpdateDisplay( dev );
        }

        /* Assign a unique Message Id for this I/O if needed */
        INCREMENT_MESSAGEID(dev);

        /* Do the Rewind-Unload */
#if defined(OPTION_SCSI_TAPE)
        if ( TAPEDEVT_SCSITAPE == dev->tapedevt )
            int_scsi_rewind_unload( dev, unitstat, code );
        else
#endif
        {
            dev->tmh->close(dev);
            *unitstat=0;
        }

        /* Update matrix display if needed */
        if ( TAPEDISPTYP_UNLOADING == dev->tapedisptype )
        {
            dev->tapedisptype = TAPEDISPTYP_IDLE;
            UpdateDisplay( dev );
        }

        if ((*unitstat & CSW_UC) != 0)      // (did it work?)
            break;                          // (no it didn't)

        dev->curfilen = 1;
        dev->nxtblkpos = 0;
        dev->prvblkpos = -1;
        dev->eotwarning = 0;
//      dev->fenced = 0;        // (handler already did this)

        /* Update matrix display */
        UpdateDisplay( dev );

        build_senseX(TAPE_BSENSE_RUN_SUCCESS,dev,unitstat,code);

        if ( dev->als )
        {
            TID dummy_tid;
            char thread_name[64];
            MSGBUF(thread_name,
                "autoload wait for %4.4X tapemount thread",
                dev->devnum);
            thread_name[sizeof(thread_name)-1] = 0;
            rc = create_thread( &dummy_tid, DETACHED,
                autoload_wait_for_tapemount_thread,
                dev, thread_name );
            if (rc)
                WRMSG(HHC00102, "E", strerror(rc));
        }

        ReqAutoMount( dev );
        break;

    } /* End case 0x0F: REWIND UNLOAD */

    /*---------------------------------------------------------------*/
    /* READ BUFFER  (3480 and later)                                 */
    /*---------------------------------------------------------------*/
    case 0x12:
    {
        /*    GA32-0127 IBM 3490E Hardware Reference

        Read Buffer (X'12')

        The Read Buffer command transfers data from the control unit
        to the channel if any buffered write data is in the control
        unit's buffer.  For each Read Buffer command completed, the
        controlling computer retrieves one block of data in last-in/
        first-out (LIFO) sequence until the buffer for the addressed
        tape drive is empty.  The controlling computer usually issues
        this command when the tape drive or subsystem malfunctions
        and cannot write data from the buffer to the tape.
        */

        /* Command reject if the volume is currently fenced */
        if (dev->fenced)
        {
            build_senseX (TAPE_BSENSE_FENCED, dev, unitstat, code);
            break;
        }

        // PROGRAMMING NOTE: until we can add support for performing
        // SCSI i/o directly to the actual real device, we simply do
        // the same thing for non-virtual devices as we do for virtual
        // ones: we force-flush the data to the device (i.e. sync)
        // and then tell the truth: that there's zero bytes of data
        // still buffered (which is true if we just flushed it all)

        // Once we add direct SCSI i/o support though, we can change
        // the below to do an actual read-buffer SCSI command for
        // non-virtual devices. (We will still always need the below
        // for virtual devices though)

        /* Assign a unique Message Id for this I/O if needed */
        INCREMENT_MESSAGEID(dev);

        // Perform flush/sync; exit on error...
        if ((rc = dev->tmh->sync( dev, unitstat, code )) < 0)
            break;      // (i/o error)

        // Flush complete. Our buffer is now empty. Tell them that.
        RESIDUAL_CALC (0);
        dev->curblkrem = 0;
        dev->curbufoff = 0;
        break;
    }

    /*---------------------------------------------------------------*/
    /* ERASE GAP                                                     */
    /*---------------------------------------------------------------*/
    case 0x17:
    {
        /* Command reject if the volume is currently fenced */
        if (dev->fenced)
        {
            build_senseX (TAPE_BSENSE_FENCED, dev, unitstat, code);
            break;
        }

        /* Unit check if tape is write-protected */
        if (dev->readonly || dev->tdparms.logical_readonly)
        {
            build_senseX (TAPE_BSENSE_WRITEPROTECT, dev, unitstat, code);
            break;
        }

        /* Update matrix display if needed */
        if ( TAPEDISPTYP_WAITACT == dev->tapedisptype )
        {
            dev->tapedisptype = TAPEDISPTYP_IDLE;
            UpdateDisplay( dev );
        }

        /* Assign a unique Message Id for this I/O if needed */
        INCREMENT_MESSAGEID(dev);

        /* Do the ERG; exit if error */
        if ((rc = dev->tmh->erg( dev, unitstat, code )) < 0)
            break;      // (error)

        /* Perform flush/sync and/or set normal completion status */
        if (0
            || !dev->write_immed
            || (rc = dev->tmh->sync( dev, unitstat, code )) == 0
        )
            build_senseX( TAPE_BSENSE_STATUSONLY, dev, unitstat, code );

        break;
    }

    /*---------------------------------------------------------------*/
    /* WRITE TAPE MARK                                               */
    /*---------------------------------------------------------------*/
    case 0x1F:
    {
        /* Command reject if the volume is currently fenced */
        if (dev->fenced)
        {
            build_senseX (TAPE_BSENSE_FENCED, dev, unitstat, code);
            break;
        }

        /* Unit check if tape is write-protected */
        if (dev->readonly || dev->tdparms.logical_readonly)
        {
            build_senseX (TAPE_BSENSE_WRITEPROTECT, dev, unitstat, code);
            break;
        }

        /* Update matrix display if needed */
        if ( TAPEDISPTYP_WAITACT == dev->tapedisptype )
        {
            dev->tapedisptype = TAPEDISPTYP_IDLE;
            UpdateDisplay( dev );
        }

        /* Assign a unique Message Id for this I/O if needed */
        INCREMENT_MESSAGEID(dev);

        /* Do the WTM; exit if error */
        if ((rc = dev->tmh->wtm(dev,unitstat,code)) < 0)
            break;      // (error)

        dev->curfilen++;

        /* Perform flush/sync and/or set normal completion status */
        if (0
            || !dev->write_immed
            || (rc = dev->tmh->sync( dev, unitstat, code )) == 0
        )
            build_senseX( TAPE_BSENSE_STATUSONLY, dev, unitstat, code );

        break;
    }

    /*---------------------------------------------------------------*/
    /* READ BLOCK ID                                                 */
    /*---------------------------------------------------------------*/
    case 0x22:
    {
        BYTE  log_blockid  [4];     // (temp; BIG-ENDIAN format)
        BYTE  phys_blockid [4];     // (temp; BIG-ENDIAN format)

        int   errcode   = TAPE_BSENSE_STATUSONLY; // (presume success)

        /* Command reject if the volume is currently fenced */
        if (dev->fenced)
        {
            build_senseX (TAPE_BSENSE_FENCED, dev, unitstat, code);
            break;
        }

        /* Assign a unique Message Id for this I/O if needed */
        INCREMENT_MESSAGEID(dev);

        /* Calculate number of bytes and residual byte count */
        RESIDUAL_CALC( 2 * sizeof(dev->blockid) );

        /* Ask media handler for actual value(s)... */
        if ((rc = dev->tmh->readblkid( dev, log_blockid, phys_blockid )) < 0)
            errcode = TAPE_BSENSE_LOCATEERR;
        else
        {
            /* Copy results to channel I/O buffer... */
            memcpy( &iobuf[0], log_blockid,  4 );
            memcpy( &iobuf[4], phys_blockid, 4 );
        }

        /* Set completion status... */
        build_senseX( errcode, dev, unitstat, code );
        break;
    }

    /*---------------------------------------------------------------*/
    /* READ BUFFERED LOG                                             */
    /*---------------------------------------------------------------*/
    case 0x24:
    {
        /* Calculate residual byte count... */

        // PROGRAMMING NOTE: technically we *should* have up to
        // 64 bytes to give them, but we may not have that many.

        /* How many bytes we SHOULD have depends on whether
           Extended Buffered Log support is enabled or not */
        len = (dev->devchar[8] & 0x01) ? 64 : 32;
        RESIDUAL_CALC (len);

        /* Clear the device sense bytes */
        memset( iobuf, 0, num );

        /* Copy device sense bytes to channel I/O buffer */
        memcpy (iobuf, dev->sense,
                dev->numsense < (U32)num ? dev->numsense : (U32)num);

        /* FIXME: Update for Format-30 Data and others
         *
         * SG24-2594-02 Magstar and IBM 3590 High Performance Tape
         *              Subsystem: Multiplatform Implementation, H.2
         *              Read Buffered Log (RBL, X'24') Format-30 Data,
         *              p. 290
         */

        /* Return unit status */
        build_senseX (TAPE_BSENSE_STATUSONLY, dev, unitstat, code);
        break;
    }

    /*---------------------------------------------------------------*/
    /* BACKSPACE BLOCK                                               */
    /*---------------------------------------------------------------*/
    case 0x27:
    {
        /* Command reject if the volume is currently fenced */
        if (dev->fenced)
        {
            build_senseX (TAPE_BSENSE_FENCED, dev, unitstat, code);
            break;
        }

        /* Update matrix display if needed */
        if ( TAPEDISPTYP_WAITACT == dev->tapedisptype )
        {
            dev->tapedisptype = TAPEDISPTYP_IDLE;
            UpdateDisplay( dev );
        }

        /* Assign a unique Message Id for this I/O if needed */
        INCREMENT_MESSAGEID(dev);

        /* Backspace to previous block according to device type,
           and exit with unit check status on error condition */
        if ((rc = dev->tmh->bsb( dev, unitstat, code )) < 0)
            break;

        /* Exit with unit exception status if tapemark was sensed */
        if (rc == 0)
        {
            build_senseX (TAPE_BSENSE_READTM, dev, unitstat, code);
            break;
        }

        /* Set normal status */
        build_senseX (TAPE_BSENSE_STATUSONLY, dev, unitstat, code);
        break;
    }

    /*---------------------------------------------------------------*/
    /* BACKSPACE FILE                                                */
    /*---------------------------------------------------------------*/
    case 0x2F:
    {
        /* Command reject if the volume is currently fenced */
        if (dev->fenced)
        {
            build_senseX (TAPE_BSENSE_FENCED, dev, unitstat, code);
            break;
        }

        /* Update matrix display if needed */
        if ( TAPEDISPTYP_WAITACT == dev->tapedisptype )
        {
            dev->tapedisptype = TAPEDISPTYP_IDLE;
            UpdateDisplay( dev );
        }

        /* Assign a unique Message Id for this I/O if needed */
        INCREMENT_MESSAGEID(dev);

        /* Backspace to previous file according to device type,
           and exit with unit check status on error condition */
        if ((rc = dev->tmh->bsf( dev, unitstat, code )) < 0)
            break;

        /* Set normal status */
        build_senseX (TAPE_BSENSE_STATUSONLY, dev, unitstat, code);
        break;
    }

    /*---------------------------------------------------------------*/
    /* SENSE PATH GROUP ID                                           */
    /*---------------------------------------------------------------*/
    case 0x34:
    {
        /*    GA32-0127 IBM 3490E Hardware Reference

        Sense Path Group ID (X'34')

        The Sense Path Group ID command transfers 12 bytes of information
        from the control unit to the channel.  The first byte (byte 0)
        is the path state byte, and the remaining 11 bytes (bytes 1-11)
        contain the path-group ID.

        The bit assignments in the path state byte (byte 0) are:

         ________ ________ ____________________________________
        | Bit    |  Value | Description                        |
        |________|________|____________________________________|
        | 0, 1   |        | Pathing Status                     |
        |________|________|____________________________________|
        |        |   00   | Reset                              |
        |________|________|____________________________________|
        |        |   01   | Reserved                           |
        |________|________|____________________________________|
        |        |   10   | Ungrouped                          |
        |________|________|____________________________________|
        |        |   11   | Grouped                            |
        |________|________|____________________________________|
        | 2, 3   |        | Partitioning State                 |
        |________|________|____________________________________|
        |        |   00   | Implicitly Enabled                 |
        |________|________|____________________________________|
        |        |   01   | Reserved                           |
        |________|________|____________________________________|
        |        |   10   | Disabled                           |
        |________|________|____________________________________|
        |        |   11   | Explicitly Enabled                 |
        |________|________|____________________________________|
        | 4      |        | Path Mode                          |
        |________|________|____________________________________|
        |        |    0   | Single path mode.                  |
        |        |    1   | Reserved, invalid for this device. |
        |________|________|____________________________________|
        | 5-7    |   000  | Reserved                           |
        |________|________|____________________________________|
        */

        /* Command Reject if Supervisor-Inhibit */
        if (dev->supvr_inhibit)
        {
            build_senseX (TAPE_BSENSE_BADCOMMAND, dev, unitstat, code);
            break;
        }

        /* Command reject if the command is not the ONLY command
           in the channel program */
        if (chained & CCW_FLAGS_CC)
        {
            build_senseX (TAPE_BSENSE_BADCOMMAND, dev, unitstat, code);
            break;
        }

        /* Calculate residual byte count */
        RESIDUAL_CALC (12);

        /* Byte 0 is the path group state byte */
        iobuf[0] = dev->pgstat;

        /* Bytes 1-11 contain the path group identifier */
        if (num > 1)
            memcpy (iobuf+1, dev->pgid, num-1);

        /* Return unit status */
        build_senseX (TAPE_BSENSE_STATUSONLY, dev, unitstat, code);
        break;

    } /* End case 0x34: SENSE PATH GROUP ID */

    /*---------------------------------------------------------------*/
    /* FORWARD SPACE BLOCK                                           */
    /*---------------------------------------------------------------*/
    case 0x37:
    {
        /* Command reject if the volume is currently fenced */
        if (dev->fenced)
        {
            build_senseX (TAPE_BSENSE_FENCED, dev, unitstat, code);
            break;
        }

        /* Update matrix display if needed */
        if ( TAPEDISPTYP_WAITACT == dev->tapedisptype )
        {
            dev->tapedisptype = TAPEDISPTYP_IDLE;
            UpdateDisplay( dev );
        }

        /* Assign a unique Message Id for this I/O if needed */
        INCREMENT_MESSAGEID(dev);

        /* Forward to next block according to device type  */
        /* Exit with unit check status if error condition  */
        if ((rc = dev->tmh->fsb( dev, unitstat, code )) < 0)
            break;

        /* Exit with unit exception status if tapemark was sensed */
        if (rc == 0)
        {
            build_senseX (TAPE_BSENSE_READTM, dev, unitstat, code);
            break;
        }

        /* Set normal status */
        build_senseX (TAPE_BSENSE_STATUSONLY, dev, unitstat, code);
        break;
    }

    /*---------------------------------------------------------------*/
    /* READ SUBSYSTEM DATA  (3490/3590)                              */
    /*---------------------------------------------------------------*/
    case 0x3E:
    {
        /*       GA32-0127 IBM 3490E Hardware Reference

        Read Subsystem Data (X'3E')

        The Read Subsystem Data command obtains various types of
        information from the 3480/3490 subsystem.  The data presented
        is dependent on the command immediately preceding the Read
        Subsystem Data command in the command chain.

        If the preceding command in the command chain is a Perform
        Subsystem Function command with the Prepare for Read Subsystem
        Data order, the data presented is a function of the sub-order
        in the data transferred with the order.
        */

        /* Command reject if not chained from either a Set Interface
           Identifier or Perform Subsystem Function command */
        if (!((chained & CCW_FLAGS_CC) && (0x77 == prevcode || 0x73 == prevcode)))
        {
            build_senseX(TAPE_BSENSE_BADCOMMAND,dev,unitstat,code);
            break;
        }

        /* Command reject if no subsystem data was prepared
           by a previous Perform Subsystem Function command */
        if (!dev->tapssdlen)      // (any subsystem data?)
        {
            build_senseX (TAPE_BSENSE_BADCOMMAND, dev, unitstat, code);
            break;
        }

        /* Calculate residual byte count */
        RESIDUAL_CALC (dev->tapssdlen);

        /* PROGRAMMING NOTE: the Prepare for Read Subsystem Data
           order of the previous Perform Subsystem Function command
           has already prepared the subsystem data directly in the
           channel buffer itself (iobuf), so there isn't any data
           that actually needs to be moved/copied; the data is
           already sitting in the channel buffer. All we need do
           is return a normal status.
        */
        build_senseX (TAPE_BSENSE_STATUSONLY, dev, unitstat, code);
        break;

    } /* End case 0x3E: READ SUBSYSTEM DATA */

    /*---------------------------------------------------------------*/
    /* FORWARD SPACE FILE                                            */
    /*---------------------------------------------------------------*/
    case 0x3F:
    {
        /* Command reject if the volume is currently fenced */
        if (dev->fenced)
        {
            build_senseX (TAPE_BSENSE_FENCED, dev, unitstat, code);
            break;
        }

        /* Update matrix display if needed */
        if ( TAPEDISPTYP_WAITACT == dev->tapedisptype )
        {
            dev->tapedisptype = TAPEDISPTYP_IDLE;
            UpdateDisplay( dev );
        }

        /* Assign a unique Message Id for this I/O if needed */
        INCREMENT_MESSAGEID(dev);

        /* Forward to next file according to device type  */
        /* Exit with unit check status if error condition */
        if ((rc = dev->tmh->fsf( dev, unitstat, code )) < 0)
            break;

        /* Set normal status */
        build_senseX (TAPE_BSENSE_STATUSONLY, dev, unitstat, code);
        break;
    }

    /*---------------------------------------------------------------*/
    /* SYNCHRONIZE  (3480 or later)                                  */
    /*---------------------------------------------------------------*/
    case 0x43:
    {
        /* Command reject if the volume is currently fenced */
        if (dev->fenced)
        {
            build_senseX (TAPE_BSENSE_FENCED, dev, unitstat, code);
            break;
        }

        /* Update matrix display if needed */
        if ( TAPEDISPTYP_WAITACT == dev->tapedisptype )
        {
            dev->tapedisptype = TAPEDISPTYP_IDLE;
            UpdateDisplay( dev );
        }

        /* Assign a unique Message Id for this I/O if needed */
        INCREMENT_MESSAGEID(dev);

        /* Do the sync */
        if ((rc = dev->tmh->sync( dev, unitstat, code )) == 0)
            build_senseX( TAPE_BSENSE_STATUSONLY, dev, unitstat, code );

        break;
    }

    /*---------------------------------------------------------------*/
    /* SET DIAGNOSE      --  Special AUTOMOUNT support  --           */
    /*---------------------------------------------------------------*/
    case 0x4B:
    {
        int     argc, i;                                     /* work */
        char  **argv;                                        /* work */
        char    newfile [ sizeof(dev->filename) ];           /* work */

        /* Command reject if AUTOMOUNT support not enabled */
        if (0
            || dev->tapedevt == TAPEDEVT_SCSITAPE
            || sysblk.tamdir == NULL
            || dev->noautomount
        )
        {
            build_senseX(TAPE_BSENSE_BADCOMMAND,dev,unitstat,code);
            break;
        }

        /* Command Reject if Supervisor-Inhibit */
        if (dev->supvr_inhibit)
        {
            build_senseX (TAPE_BSENSE_BADCOMMAND, dev, unitstat, code);
            break;
        }

        /* Command Reject if command-chained and i/o length not 1 */
        if ((flags & CCW_FLAGS_CC) && count == 1)
        {
            /* AUTOMOUNT QUERY - part 1 (chained 0xE4 SENSE ID = part 2) */

            /* Set normal status but do nothing else; the next CCW
               should be a SENSE ID (0xE4) which will do the query */
            build_senseX( TAPE_BSENSE_STATUSONLY, dev, unitstat, code );
            break;
        }

        /* AUTOMOUNT MOUNT... */

        /* Calculate residual byte count */
        RESIDUAL_CALC (sizeof(newfile)-1);   /* (minus-1 for NULL) */

        /* Copy the device's new filename from guest storage */
        str_guest_to_host( iobuf, (BYTE *)newfile, num );

        /* Change "OFFLINE" to "*" (tape unloaded) */
        if (strcasecmp (newfile, "OFFLINE") == 0)
            STRLCPY( newfile, TAPE_UNLOADED );

        /* Obtain the device lock */
        OBTAIN_DEVLOCK( dev );

        /* Validate the given path... */
        if ( strcmp( newfile, TAPE_UNLOADED ) != 0 )
        {
            TAMDIR *tamdir = NULL;
            int minlen = 0;
            int rej = 0;

            /* (because i hate typing) */
#define  _HHC00205E(_file,_reason) \
            { \
                WRMSG(HHC00205, "E", LCSS_DEVNUM, _file, TTYPSTR(dev->tapedevt), "auto-mount", _reason); \
                build_senseX (TAPE_BSENSE_TAPELOADFAIL, dev, unitstat, code); \
                RELEASE_DEVLOCK( dev ); \
                break; \
            }

            // Resolve given path...
            {
                char  resolve_in [ MAX_PATH ] = {0};  /* (work) */
                char  resolve_out[ MAX_PATH ] = {0};  /* (work) */

                /* (build path to be resolved...) */
                if (0
#if defined(_MSVC_)
                    || newfile[1] == ':'        /* (fullpath given?) */
#else /* !_MSVC_ */
                    || newfile[0] == '/'        /* (fullpath given?) */
#endif /* _MSVC_ */
                    || newfile[0] == '.'        /* (relative path given?) */
                )
                    resolve_in[0] = 0;          /* (then use just given spec) */
                else                            /* (else prepend with default) */
                    STRLCPY( resolve_in, sysblk.defdir );

                /* (finish building path to be resolved) */
                STRLCAT( resolve_in, newfile );

                /* (fully resolvable path?) */
                if (realpath( resolve_in, resolve_out ) == NULL)
                    _HHC00205E( resolve_in, "unresolvable path" );

                /* Switch to fully resolved path */
                hostpath( newfile, resolve_out, sizeof(newfile) );
            }

            /* Verify file is in an allowable directory... */
            rej = 0; minlen = 0;
            while ((tamdir = findtamdir( rej, minlen, newfile )) != NULL)
            {
                rej = !rej;
                minlen = tamdir->len;
            }

            /* Error if "allowable" directory not found... */
            if (!rej)
                _HHC00205E( newfile, "impermissible directory" );

            /* Verify file exists... */
            if (access( newfile, R_OK ) != 0)
                _HHC00205E( newfile, "file not found" );
        }

        /* Prevent accidental re-init'ing of an already loaded tape drive */
        if (1
            && sysblk.nomountedtapereinit
            && strcmp (newfile,       TAPE_UNLOADED) != 0
            && strcmp (dev->filename, TAPE_UNLOADED) != 0
        )
        {
            WRMSG(HHC00214, "E", LCSS_DEVNUM, newfile, TTYPSTR(dev->tapedevt));
            build_senseX (TAPE_BSENSE_TAPELOADFAIL, dev, unitstat, code);
            RELEASE_DEVLOCK( dev );
            break;
        }

        /* Build re-initialization parameters using new filename */
        argc = dev->argc;
        argv = malloc (dev->argc * sizeof(char*));

        for (i=0; i < argc; i++)
        {
            if (dev->argv[i])
                argv[i] = strdup(dev->argv[i]);
            else
                argv[i] = NULL;
        }

        /* (replace filename argument with new filename) */
        free( argv[0] );
        argv[0] = strdup( newfile );

        /* Attempt reinitializing the device using the new filename... */
        rc = (int)(dev->hnd->init)( dev, argc, argv );

        /* (free temp copy of parms to prevent memory leak) */
        for (i=0; i < argc; i++)
            if (argv[i])
                free(argv[i]);

        /* Issue message and set status based on whether it worked or not... */
        if (0
            || rc < 0
            || strfilenamecmp( dev->filename, newfile ) != 0
        )
        {
            // (failure)

            if (strcmp( newfile, TAPE_UNLOADED ) == 0)
            {
                /* (an error message explaining the reason for the
                    failure should hopefully already have been issued) */
                WRMSG(HHC00205, "E", LCSS_DEVNUM, newfile,
                      TTYPSTR(dev->tapedevt), "auto-unmount", "see previous message");
            }
            else
                _HHC00205E( newfile, "file not found" ); // (presumed)

            /* (the load or unload attempt failed) */
            build_senseX (TAPE_BSENSE_TAPELOADFAIL, dev, unitstat, code);
        }
        else
        {
            // (success)

            if (strcmp( newfile, TAPE_UNLOADED ) == 0)
                WRMSG(HHC00216, "I", LCSS_DEVNUM, newfile, TTYPSTR(dev->tapedevt));
            else
                WRMSG(HHC00215, "I", LCSS_DEVNUM, dev->filename, TTYPSTR(dev->tapedevt));

            /* (save new parms for next time) */
            free( dev->argv[0] );
            dev->argv[0] = strdup( newfile );

            /* (set normal status for this ccw) */
            build_senseX( TAPE_BSENSE_STATUSONLY, dev, unitstat, code );
        }

        /* Release the device lock and exit function... */
        RELEASE_DEVLOCK( dev );
        break;

    } /* End case 0x4B: SET DIAGNOSE */

    /*---------------------------------------------------------------*/
    /* READ MESSAGE ID                                               */
    /*---------------------------------------------------------------*/
    case 0x4E:
    {
        /*       GA32-0127 IBM 3490E Hardware Reference

        Read Message ID (X'4E')

        The Read Message ID command is used to read the message identifier
        that was assigned by the control unit to commands that indicated
        the message-required flag requesting notification when an asynchronous
        operation is complete.  The Read Message ID command must be chained
        directly from the specific command that requested the message
        notification or the command will be presented unit check status
        with associated sense indicating ERA code 27.

        If the Read Message ID command is chained to a specific command
        that requests notification, but the command does not result in an
        asynchronous operation, the message identifier field returned
        will be all zeroes.

        The data returned has the following format:

         ________ ____________________________________________________
        | Byte   | Description                                        |
        |________|____________________________________________________|
        | 0,1    | Length (set to X'000A')                            |
        |________|____________________________________________________|
        | 2      | Format (set to X'02')                              |
        |________|____________________________________________________|
        | 3      | Message Code                                       |
        |        |                                                    |
        |        | Value Description                                  |
        |        |                                                    |
        |        | X'01' Delayed-Response Message                     |
        |________|____________________________________________________|
        | 4-7    | Message ID                                         |
        |        |                                                    |
        |        | This field contains the message identifier         |
        |        | assigned by the control unit to the requested      |
        |        | operation.  If the operation was executed by       |
        |        | the subsystem as an immediate operation, this      |
        |        | field contains all zeroes and a later delayed-     |
        |        | response message is not generated.                 |
        |________|____________________________________________________|
        | 8      | Flags (set to X'00')                               |
        |________|____________________________________________________|
        | 9      | Reserved (set to X'00')                            |
        |________|____________________________________________________|
        */

        /* Command reject if not chained from a write command */
        if (!((chained & CCW_FLAGS_CC) && IS_CCW_WRITE(prevcode)))
        {
            build_senseX (TAPE_BSENSE_BADCOMMAND, dev, unitstat, code);
            break;
        }

        /* Calculate residual byte count */
        RESIDUAL_CALC( 10 );

        // PROGRAMMING NOTE: at the moment all of our i/o's are synchronous.
        // Thus we always return zero indicating the i/o was not asynchronous.

        STORE_HW ( &iobuf[0],   10 );       // 0-1
                    iobuf[2] = 0x02;        // 2
                    iobuf[3] = 0x01;        // 3
        STORE_FW ( &iobuf[4],    0 );       // 4-7  (Message Id)
                    iobuf[8] = 0x00;        // 8
                    iobuf[9] = 0x00;        // 9

        build_senseX (TAPE_BSENSE_STATUSONLY, dev, unitstat,code);
        break;

    } /* End case 0x4E: READ MESSAGE ID */

    /*---------------------------------------------------------------*/
    /* LOCATE BLOCK                                                  */
    /*---------------------------------------------------------------*/
    case 0x4F:
    {
        U32  locblock;                 /* Block Id for Locate Block */
        int  errcode = TAPE_BSENSE_STATUSONLY;  /* Presumed success */

        /* Command reject if the volume is currently fenced */
        if (dev->fenced)
        {
            build_senseX (TAPE_BSENSE_FENCED, dev, unitstat, code);
            break;
        }

        /* Check for minimum count field */
        if (count < sizeof(dev->blockid))
        {
            build_senseX (TAPE_BSENSE_BADCOMMAND, dev, unitstat, code);
            break;
        }

        /* Block to seek */
        ASSERT( count >= sizeof(locblock) );
        FETCH_FW(locblock, iobuf);

        /* Check for invalid/reserved Format Mode bits */
        if (0x3590 != dev->devtype)
        {
            if (0x00C00000 == (locblock & 0x00C00000))
            {
                build_senseX (TAPE_BSENSE_BADCOMMAND, dev, unitstat, code);
                break;
            }

            /* We only want the Block Number in the low-order 22 bits */
            locblock &= 0x003FFFFF;
        }

        /* Calculate residual byte count */
        RESIDUAL_CALC( sizeof(locblock) );

        /* Informative message if tracing */
        if (dev->ccwtrace)
            WRMSG(HHC00217, "I", LCSS_DEVNUM
                ,TAPEDEVT_SCSITAPE == dev->tapedevt ? (char*)dev->filename : ""
                ,TTYPSTR(dev->tapedevt)
                ,locblock
            );

        /* Update display if needed */
        if ( TAPEDISPTYP_IDLE    == dev->tapedisptype ||
             TAPEDISPTYP_WAITACT == dev->tapedisptype )
        {
            dev->tapedisptype = TAPEDISPTYP_LOCATING;
            UpdateDisplay( dev );
        }

        /* Assign a unique Message Id for this I/O if needed */
        INCREMENT_MESSAGEID(dev);

        /* Ask media handler to perform the locate... */
        if ((rc = dev->tmh->locateblk( dev, locblock, unitstat, code )) < 0)
        {
            errcode = TAPE_BSENSE_LOCATEERR;

            if (!SVF_ENABLED())
                dev->fenced = 1; // (position lost; fence the volume)
        }

        /* Update display if needed */
        if ( TAPEDISPTYP_LOCATING == dev->tapedisptype )
        {
            dev->tapedisptype = TAPEDISPTYP_IDLE;
            UpdateDisplay( dev );
        }

        /* Set completion status... */
        build_senseX( errcode, dev, unitstat, code );
        break;

    } /* End case 0x4F: LOCATE BLOCK */

    /*---------------------------------------------------------------*/
    /* SUSPEND MULTIPATH RECONNECTION  (3480 and later)              */
    /*---------------------------------------------------------------*/
    case 0x5B:
    {
        /*       GA32-0127 IBM 3490E Hardware Reference

        Suspend Multipath Reconnection (X'5B')

        The Suspend Multipath Reconnection command performs as a
        No-Operation command because all controlling-computer-to-
        subsystem operations occur in single-path status.
        */

        /* Command Reject if Supervisor-Inhibit */
        if (dev->supvr_inhibit)
        {
            build_senseX (TAPE_BSENSE_BADCOMMAND, dev, unitstat, code);
            break;
        }

        /* Set normal status */
        build_senseX (TAPE_BSENSE_STATUSONLY, dev, unitstat, code);
        break;
    }

    /*---------------------------------------------------------------*/
    /* READ MEDIA CHARACTERISTICS  (3590 only)                       */
    /*---------------------------------------------------------------*/
    case 0x62:
    {
        /*    SG24-2506 IBM 3590 Tape Subsystem Technical Guide

        5.2.3 New Read Media Characteristics

        The new Read Media Characteristics CCW (command code x'62')
        provides up to 256 bytes of information about the media and
        formats supported by the Magstar tape drive."
        */

        // ZZ FIXME: not coded yet.

        /* Set command reject sense byte, and unit check status */
        build_senseX (TAPE_BSENSE_BADCOMMAND, dev, unitstat, code);
        break;
    }

    /*---------------------------------------------------------------*/
    /* READ DEVICE CHARACTERISTICS                                   */
    /*---------------------------------------------------------------*/
    case 0x64:
    {
        /* Command reject if device characteristics not available */
        if (dev->numdevchar == 0)
        {
            build_senseX (TAPE_BSENSE_BADCOMMAND, dev, unitstat, code);
            break;
        }

        /* Calculate residual byte count */
        RESIDUAL_CALC (dev->numdevchar);

        /* Copy device characteristics bytes to channel buffer */
        memcpy (iobuf, dev->devchar, num);

        /* Return unit status */
        build_senseX (TAPE_BSENSE_STATUSONLY, dev, unitstat, code);
        break;
    }

#if 0
    /*---------------------------------------------------------------*/
    /* SET INTERFACE IDENTIFIER  (3490 and later)                    */
    /*---------------------------------------------------------------*/
    case 0x73:
    {
        // PROGRAMMING NOTE: the 3480 and earlier "Mode Set" interpretation
        // of this CCW is handled in the command-table as a no-op; the "Set
        // Interface Identifier" interpretation of this CCW for 3490 and
        // later model tape drives is *ALSO* handled in the command-table
        // as a no-op as well, so there's really no reason for this switch
        // case to even exist until such time as we need to support a model
        // that happens to require special handling (which is unlikely).

        // I'm keeping the code here however for documentation purposes
        // only, but of course disabling it from compilation via #if 0.

        build_senseX (TAPE_BSENSE_STATUSONLY, dev, unitstat, code);
        break;
    }
#endif

    /*---------------------------------------------------------------*/
    /* PERFORM SUBSYSTEM FUNCTION                                    */
    /*---------------------------------------------------------------*/
    case 0x77:
    {
        BYTE  order  = iobuf[0];
        BYTE  flag   = iobuf[1];
        BYTE  parm   = iobuf[2];

        /* Command Reject if Supervisor-Inhibit */
        if (dev->supvr_inhibit)
        {
            build_senseX (TAPE_BSENSE_BADCOMMAND, dev, unitstat, code);
            break;
        }

        /* The flag byte must be zero for all orders because
           none of our supported orders supports a flag byte */
        if (PSF_FLAG_ZERO != flag)
        {
            build_senseX (TAPE_BSENSE_BADCOMMAND, dev, unitstat, code);
            break;
        }

        /* Byte 0 is the PSF order */
        switch (order)
        {
        /*-----------------------------------------------------------*/
        /* Activate/Deactivate Forced Error Logging                  */
        /* 0x8000nn / 0x8100nn                                       */
        /*-----------------------------------------------------------*/
        case PSF_ORDER_AFEL:
        case PSF_ORDER_DFEL:
        {
            BYTE  bEnable  = (PSF_ORDER_AFEL == order) ? 1 : 0;

            /* Calculate residual byte count */
            RESIDUAL_CALC (3);

            /* Control information length must be 3 bytes long */
            /* and the parameter byte must be one or the other */
            if ( (count < len)
                || ((PSF_ACTION_FEL_IMPLICIT != parm) &&
                    (PSF_ACTION_FEL_EXPLICIT != parm))
            )
            {
               build_senseX(TAPE_BSENSE_BADCOMMAND,dev,unitstat,code);
               break;
            }

            /* Enable/Disable Forced Error Logging as requested... */

#if 0 // (implicit enabling for all devices not currently supported; treat as explicit instead)
            if (PSF_ACTION_FEL_IMPLICIT == parm)
            {
                // Implicit: for ALL devices...
                dev->forced_logging = bEnable ? 1 : 0;
            }
            else // (PSF_ACTION_FEL_EXPLICIT == parm)
#endif // (implicit not supported)
            {
                // Explicit: for only THIS device...
                dev->forced_logging = bEnable ? 1 : 0;
            }

            build_senseX(TAPE_BSENSE_STATUSONLY,dev,unitstat,code);
            break;
        }

        /*-----------------------------------------------------------*/
        /* Activate/Deactivate Access Control                        */
        /* 0x8200nn00 / 0x8300nn00                                   */
        /*-----------------------------------------------------------*/
        case PSF_ORDER_AAC:     // (Activate)
        case PSF_ORDER_DAC:     // (Dectivate)
        {
            BYTE  bEnable  = (PSF_ORDER_AAC == order) ? 1 : 0;

            /* Calculate residual byte count */
            RESIDUAL_CALC (4);

            /* Control information length must be 4 bytes long */
            /* and the parameter byte must not be invalid      */
            if (0
                || (count < len)
                || (parm  & ~(PSF_ACTION_AC_LWP | PSF_ACTION_AC_DCD |   // (bits on that shouldn't be)
                              PSF_ACTION_AC_DCR | PSF_ACTION_AC_ER))
                || !(parm &  (PSF_ACTION_AC_LWP | PSF_ACTION_AC_DCD |   // (bits on that should be)
                              PSF_ACTION_AC_DCR | PSF_ACTION_AC_ER))
            )
            {
                build_senseX(TAPE_BSENSE_BADCOMMAND,dev,unitstat,code);
                break;
            }

            /* Enable/Disable Logical Write Protect if requested */
            if (parm & PSF_ACTION_AC_LWP)
                dev->tdparms.logical_readonly = bEnable ? 1 : 0;

            /* Enable/Disable Data Compaction (compression) if requested */
            if (parm & PSF_ACTION_AC_DCD)
            {
                if (TAPEDEVT_HETTAPE == dev->tapedevt)
                {
                    rc = het_cntl( dev->hetb, HETCNTL_SET | HETCNTL_COMPRESS,
                                   bEnable ? TRUE : FALSE );
                }
#if defined(OPTION_SCSI_TAPE)
                else if (TAPEDEVT_SCSITAPE == dev->tapedevt)
                {
                    // ZZ FIXME: future place for direct SCSI i/o
                    // to enable/disable compression for 3480/later.
                }
#endif
            }

            build_senseX(TAPE_BSENSE_STATUSONLY,dev,unitstat,code);
            break;
        }

        /*-----------------------------------------------------------*/
        /* Reset Volume Fenced                                       */
        /* 0x9000                                                    */
        /*-----------------------------------------------------------*/
        case PSF_ORDER_RVF:
        {
            /*       GA32-0127 IBM 3490E Hardware Reference

            Volume Fencing

            When a condition results in a volume integrity exposure,
            the control unit will prevent further access to the volume.
            This process is called Volume Fencing and is primarily
            related to loss of buffered write data, tape positioning,
            or assignment protection.

            The control unit prevents further access to the tape volume
            by conditioning itself to generate deferred unit checks with
            associated sense data indicating ERA code 47, for all commands
            that are eligible to receive the deferred unit check until
            the condition is reset or until the cartridge is unloaded.
            The condition that caused the fencing to occur has already
            been indicated by the previous unit check and associated sense
            data.
            */

            /* Calculate residual byte count */
            RESIDUAL_CALC (2);

            /* Control information length must be 2 bytes long */
            if (count < len)
            {
              build_senseX(TAPE_BSENSE_BADCOMMAND,dev,unitstat,code);
              break;
            }

            dev->fenced = 0;        // (as requested!)

            build_senseX(TAPE_BSENSE_STATUSONLY,dev,unitstat,code);
            break;
        }

        /*-----------------------------------------------------------*/
        /* Pin Device                                                */
        /* 0xA100nn                                                  */
        /*-----------------------------------------------------------*/
        case PSF_ORDER_PIN_DEV:
        {
            /* Calculate residual byte count */
            RESIDUAL_CALC (3);

            /* Control information length must be 3 bytes long
               and the parameter byte must not be invalid */
            if ( (count < len)
                  || ((parm != PSF_ACTION_PIN_CU0) &&
                      (parm != PSF_ACTION_PIN_CU1))
            )
            {
                build_senseX(TAPE_BSENSE_BADCOMMAND,dev,unitstat,code);
                break;
            }

            /* Not currently supported; treat as no-op */
            build_senseX(TAPE_BSENSE_STATUSONLY,dev,unitstat,code);
            break;
        }

        /*-----------------------------------------------------------*/
        /* Unpin Device                                              */
        /* 0xA200                                                    */
        /*-----------------------------------------------------------*/
        case PSF_ORDER_UNPIN_DEV:
        {
            /* Calculate residual byte count */
            RESIDUAL_CALC (2);

            /* Control information length must be 2 bytes long */
            if (count < len)
            {
                build_senseX(TAPE_BSENSE_BADCOMMAND,dev,unitstat,code);
                break;
            }

            /* Not currently supported; treat as no-op */
            build_senseX(TAPE_BSENSE_STATUSONLY,dev,unitstat,code);
            break;
        }

        /*-----------------------------------------------------------*/
        /* Prepare for Read Subsystem Data                           */
        /* 0x180000000000mm00iiiiiiii                                */
        /*-----------------------------------------------------------*/
        case PSF_ORDER_PRSD:
        {
            /*       GA32-0127 IBM 3490E Hardware Reference

            Prepare for Read Subsystem Data (X'18')

            The order transfers 12 bytes of data used for processing a
            Read Subsystem Data command that immediately follows the
            Perform Subsystem Function command specifying this order in
            the command chain.  If a Read Subsystem Data command is not
            issued as the next command in the command chain, the data is
            discarded and no other action is performed.  If a Read Subsystem
            Data command is issued as the next command in the command chain,
            the data determines what type of information is presented to
            the Read Subsystem Data command.

            When the Prepare for Subsystem Data order with the attention
            message sub-order is specified in a Perform Subsystem Function
            command, the command is treated as a global command.  If the
            command is issued while the Special Intercept Condition is
            active, a unit check status is presented with the associated
            sense data indicating ERA code 53.

            The Prepare for Read Subsystem Data order requires an order
            byte (byte 0), a flag byte (byte 1), and parameter bytes.
            The flag byte is set to 0. The parameter bytes are defined
            as follows:

             ________ ___________________________________________________
            | Byte   | Description                                       |
            |________|___________________________________________________|
            | 2-5    | Reserved (X'00')                                  |
            |________|___________________________________________________|
            | 6      | Attention Message (X'03')                         |
            |        |                                                   |
            |        | When active and bytes 8-11 contain X'00000000',   |
            |        | the program is requesting the control unit        |
            |        | to present any pending attention message or       |
            |        | unsolicited unit check condition that is          |
            |        | associated with the addressed device-path pair.   |
            |        | If there is no message or unit check condition    |
            |        | present, the subsystem displays the "No Message"  |
            |        | message.                                          |
            |        |                                                   |
            |        | When active and bytes 8-11 contain anything       |
            |        | other than X'00000000', the program is re-        |
            |        | questing the control unit to present the status   |
            |        | of the asynchronous operation as identified by    |
            |        | the contents of bytes 8-11.                       |
            |________|___________________________________________________|
            | 7      | Reserved (X'00')                                  |
            |________|___________________________________________________|
            | 8-11   | Message ID                                        |
            |________|___________________________________________________|
            */

            /* Calculate residual byte count */
            RESIDUAL_CALC (12);

            /* Control information length must be 12 bytes long the */
            /* parameter must be valid and all reserved bytes zero. */
            /* Also note that the only sub-order we support is the  */
            /* only sub-order that is defined: attention message.   */
            if (0
                || (count < len)
                || (iobuf[6] != PSF_ACTION_SSD_ATNMSG)
                || (memcmp( &iobuf[2], "\00\00\00\00", 4 ) != 0)
                || (iobuf[7] != 0x00)
            )
            {
                build_senseX (TAPE_BSENSE_BADCOMMAND, dev, unitstat, code);
                break;
            }

            /* If the Special Intercept Condition is active, present
               unit check status with sense indicating ERA code 53 */
            if (dev->SIC_active)
            {
                build_senseX (TAPE_BSENSE_BADCOMMAND, dev, unitstat, code);
                dev->SIC_active = 0;
                break;
            }

            // Build the requested Subsystem Data...

            // PROGRAMMING NOTE: note that we build the requested data
            // directly in the channel i/o buffer itself (iobuf). This
            // relieves us from having to allocate/maintain a separate
            // buffer for it somewhere, and relieves the READ SUBSYSTEM
            // DATA command (0x3E) from having to copy the data into
            // the channel buffer from somewhere. Instead it can return
            // immediately since the data is already in the buffer. (See
            // the 0x3E: READ SUBSYSTEM DATA command for information).

            // PROGRAMMING NOTE: since at the moment we don't support
            // asynchronous i/o (all of our i/o's are synchronous), we
            // return either a Format x'00' (No Message) response if the
            // Message Id they specified was x'00000000' or, if they
            // requested the status for a specific Message Id, a format
            // x'02' (Message Id Status) response with x'00' Operation
            // Completion Status (I/O Completed).

            if (memcmp( &iobuf[8], "\00\00\00\00", 4 ) == 0)
            {
                /* Format x'00': "No Message" */
                dev->tapssdlen = 9;                     // (Length)
                STORE_HW ( &iobuf[0], dev->tapssdlen ); // (Length = 9 bytes)
                iobuf[2] = 0x00;                        // (Format = x'00': "No Message")
                iobuf[3] = 0x00;                        // (Message Code = none)
                memcpy( &iobuf[4], &iobuf[8], 4 );      // (Message Id = same as requested)
                iobuf[8] = 0x00;                        // (Flags = none)
            }
            else
            {
                /* Format x'02': "Message Id Status" */
                dev->tapssdlen = 10;                    // (Length)
                STORE_HW ( &iobuf[0], dev->tapssdlen ); // (Length = 10 bytes)
                iobuf[2] = 0x02;                        // (Format = x'01: Message Id Status)
                iobuf[3] = 0x01;                        // (Message Code = Delayed Response)
                memcpy( &iobuf[4], &iobuf[8], 4 );      // (Message Id = same as requested)
                iobuf[8] = 0x00;                        // (Reserved)
                iobuf[9] = 0x00;                        // (Status = "I/O Completed")
            }
            break;

        } /* End case PSF_ORDER_PRSD */

        /*-----------------------------------------------------------*/
        /* Set Special Intercept Condition                           */
        /* 0x1B00                                                    */
        /*-----------------------------------------------------------*/
        case PSF_ORDER_SSIC:
        {
            /*       GA32-0127 IBM 3490E Hardware Reference

            Set Special Intercept Condition (X'1B')

            The order controls the activation or deactivation of the
            special intercept condition associated with the device-path
            group pair to which the command is issued.  The order is
            supported by the model if byte 8 bit 4 is active in the data
            presented to the Read Device Characteristics command.  The
            order requires an order byte (byte 0) and a flag byte (byte 1).
            The flag byte is set to 0.

            When processed, the command activates the special intercept
            condition for the device on each channel path that has the
            same path group ID as the issuing channel path.  The path
            group ID is considered valid on a given channel path if it
            is valid for any device on the channel path.  The special
            intercept condition controls the presentation of attention-
            intercept status.  The sense data associated with the
            attention-intercept status indicates ERA code 57.  The special
            intercept condition also causes the next global command
            issued to the device-path group pair to be presented unit check
            status with associated sense data indicating ERA code 53.

            The special intercept condition is deactivated on a channel
            path if a reset signal is received on the channel path.  The
            special intercept condition is deactivated for the device-group
            pair if a global command is presented unit check status with
            associated sense data indicating ERA code 53, or if the last
            path in the associated set of channel paths (that is, with the
            same valid path group ID) is reset.

            After the Set Special Intercept Condition order is specified
            in a Perform Subsystem Function command, the command is treated
            as a global command. If the command is issued while the special
            intercept condition is active, a unit check status is presented
            with associated sense data indicating ERA code 53.

            If a command is issued to a channel path without a valid path
            group ID (that is, all devices in the reset state), unit check
            status is presented with associated sense data indicating ERA
            code 27.
            */

            /* Command reject if Special Intercept Condition not supported */
            if (!SIC_SUPPORTED())
            {
                build_senseX (TAPE_BSENSE_BADCOMMAND, dev, unitstat, code);
                break;
            }

            /* If the command is issued while the Special Intercept  */
            /* Condition is active, a unit check status is presented */
            /* with associated sense data indicating ERA code 53.    */
            if (dev->SIC_active)        // (already active?)
            {
                build_senseX (TAPE_BSENSE_BADCOMMAND, dev, unitstat, code);
                dev->SIC_active = 0;      // (reset after UC)
                break;
            }

            /* Activate Special Intercept Condition */
            dev->SIC_active = 1;
            break;

        } /* End case PSF_ORDER_SSIC */

        /*-----------------------------------------------------------*/
        /* Message Not Supported                                     */
        /* 0x1C00xxccnnnn0000iiiiii...                               */
        /*-----------------------------------------------------------*/
        case PSF_ORDER_MNS:
        {
            /*       GA32-0127 IBM 3490E Hardware Reference

            Message Not Supported (X'1C')

            The order transfers 20 bytes of data that identify the host
            that does not support a prior attention message containing
            the Notify Nonsupport flag. The order requires an order byte
            (byte 0), a flag byte (byte 1), and parameter bytes.  The
            flag byte is set to 0.  The parameter bytes are defined as
            follows:

             ________ ________ ____________________________________________
            | Byte   | Value  | Description                                |
            |________|________|____________________________________________|
            | 2      |        | Response Code                              |
            |________|________|____________________________________________|
            |        | 0      | Reserved (invalid).                        |
            |________|________|____________________________________________|
            |        | 1      | Message rejected.  Unknown format.         |
            |________|________|____________________________________________|
            |        | 2      | Message rejected.  Function not supported. |
            |________|________|____________________________________________|
            |        | 3-255  | Reserved (invalid).                        |
            |________|________|____________________________________________|
            | 3      |        | Channel Path ID (CHPID)                    |
            |        |        |                                            |
            |        |        | The byte identifies the channel path that  |
            |        |        | received the attention message.            |
            |________|________|____________________________________________|
            | 4, 5   |        | Device Number                              |
            |        |        |                                            |
            |        |        | The bytes identify the device number of    |
            |        |        | the device that received the attention     |
            |        |        | message.                                   |
            |________|________|____________________________________________|
            | 6, 7   |        | Reserved (must be X'00').                  |
            |________|________|____________________________________________|
            | 8-11   |        | Message ID                                 |
            |        |        |                                            |
            |        |        | The field contains the message ID that     |
            |        |        | was presented to the host in the attention |
            |        |        | message.                                   |
            |________|________|____________________________________________|
            | 12-19  |        | System ID                                  |
            |        |        |                                            |
            |        |        | The field contains an 8-byte system ID     |
            |        |        | that identifies the host or host partition |
            |        |        | responding to the attention message.       |
            |________|________|____________________________________________|
            */

            // PROGRAMMING NOTE: none of our responses to the Perform Sub-
            // System Function order Attention Message sub-order (see the
            // PSF_ORDER_PRSD case further above) support any flags. Thus
            // because we never set/request the "Notify Nonsupport" flag
            // in our Attention Message sub-order response, the host should
            // never actually ever be issuing this particular order of the
            // Perform Subsystem Functon command since it shouldn't be
            // trying to tell us what we never asked it to. Nevertheless
            // we should probably support it anyway just in case it does
            // by treating it as a no-op (as long as it's valid of course).

            /* Check for valid data (Note: we don't bother validating the
               Channel Path ID, Device Number, Message ID or System ID) */
            if (0
            //  ||  flag != 0x00                      // (flag byte) (note: already checked)
                || (parm != 0x01 && parm != 0x02)     // (response code)
                ||  iobuf[6] != 0x00                  // (reserved)
                ||  iobuf[7] != 0x00                  // (reserved)
            )
            {
                build_senseX (TAPE_BSENSE_BADCOMMAND, dev, unitstat, code);
                break;
            }

            /* Calculate residual byte count */
            RESIDUAL_CALC (20);

            /* Treat as No-op */
            build_senseX (TAPE_BSENSE_STATUSONLY, dev, unitstat, code);
            break;

        } /* End case PSF_ORDER_MNS */

        /*-----------------------------------------------------------*/
        /* Unknown/Supported PSF order                               */
        /*-----------------------------------------------------------*/
        default:
        {
            build_senseX(TAPE_BSENSE_BADCOMMAND,dev,unitstat,code);
            break;
        }

        } /* End PSF switch (order) */

        break;

    } /* End case 0x77: PERFORM SUBSYSTEM FUNCTION */

    /*---------------------------------------------------------------*/
    /* DATA SECURITY ERASE                                           */
    /*---------------------------------------------------------------*/
    case 0x97:
    {
        /*      GA32-0127 IBM 3490E Hardware Reference

        Data Security Erase (X'97')

        The Data Security Erase command writes a random pattern
        from the position of the tape where the command is issued
        to the physical end of tape.

        The Data Security Erase command must be command-chained
        from an Erase Gap command.  Most operating systems signal
        that the channel program is complete when the channel ending
        status is returned for the final command in the chain.  If
        the Data Security Erase command is the last command in a
        channel program, another command should be chained after the
        Data Security Erase command.  (The No-Operation command is
        appropriate.)  This practice ensures that any error status
        returns with device ending status after the Data Security
        Erase command is completed.
        */

        /* Command reject if not chained from Erase Gap command */
        if (!((chained & CCW_FLAGS_CC) && 0x17 == prevcode))
        {
            build_senseX (TAPE_BSENSE_BADCOMMAND, dev, unitstat, code);
            break;
        }

        /* Command reject if the volume is currently fenced */
        if (dev->fenced)
        {
            build_senseX (TAPE_BSENSE_FENCED, dev, unitstat, code);
            break;
        }

        /* Command reject if tape is write-protected */
        if (dev->readonly || dev->tdparms.logical_readonly)
        {
            build_senseX (TAPE_BSENSE_WRITEPROTECT, dev, unitstat, code);
            break;
        }

        /* Update matrix display if needed */
        if ( TAPEDISPTYP_IDLE    == dev->tapedisptype ||
             TAPEDISPTYP_WAITACT == dev->tapedisptype )
        {
            dev->tapedisptype = TAPEDISPTYP_ERASING;
            UpdateDisplay( dev );
        }

        /* Assign a unique Message Id for this I/O if needed */
        INCREMENT_MESSAGEID(dev);

        /* Do the DSE; exit if error */
        if ((rc = dev->tmh->dse( dev, unitstat, code )) < 0)
            break;      // (error)

        /* Update matrix display if needed */
        if ( TAPEDISPTYP_ERASING == dev->tapedisptype )
        {
            dev->tapedisptype = TAPEDISPTYP_IDLE;
            UpdateDisplay( dev );
        }

        /* Perform flush/sync and/or set normal completion status */
        if (0
            || !dev->write_immed
            || (rc = dev->tmh->sync( dev, unitstat, code )) == 0
        )
            build_senseX( TAPE_BSENSE_STATUSONLY, dev, unitstat, code );

        break;

    } /* End case 0x97: DATA SECURITY ERASE */

    /*---------------------------------------------------------------*/
    /* LOAD DISPLAY                                                  */
    /*---------------------------------------------------------------*/
    case 0x9F:
    {
        /* Command Reject if Supervisor-Inhibit */
        if (dev->supvr_inhibit)
        {
            build_senseX (TAPE_BSENSE_BADCOMMAND, dev, unitstat, code);
            break;
        }

        /* Calculate residual byte count */
        RESIDUAL_CALC (17);

        /* Issue message on 3480 matrix display */
        load_display (dev, iobuf, count);

        /* Return unit status */
        build_senseX (TAPE_BSENSE_STATUSONLY, dev, unitstat, code);
        break;
    }

    /*---------------------------------------------------------------*/
    /* Read and Reset Buffered Log (9347)                            */
    /*---------------------------------------------------------------*/
    case 0xA4:
    {
        /* Calculate residual byte count */
        RESIDUAL_CALC (dev->numsense);

        /* Reset SENSE Data */
        memset (dev->sense, 0, sizeof(dev->sense));
        *unitstat = CSW_CE|CSW_DE;

        /* Copy device Buffered log data (Bunch of 0s for now) */
        memcpy (iobuf, dev->sense, num);

        /* Indicate Contengency Allegiance has been cleared */
        dev->sns_pending = 0;
        break;
    }

    /*---------------------------------------------------------------*/
    /* SET PATH GROUP ID                                             */
    /*---------------------------------------------------------------*/
    case 0xAF:
    {
        /*      GA32-0127 IBM 3490E Hardware Reference

        Set Path Group ID (X'AF')

        The Set Path Group ID command identifies a controlling computer
        and specific channel path to the addressed control unit and
        tape drive.

        The Set Path Group ID command transfers 12 bytes of path group
        ID information to the subsystem.  The first byte (byte 0) is a
        function control byte, and the remaining 11 bytes (bytes 1-11)
        contain the path-group ID.

        The bit assignments in the function control byte (byte 0) are:

         ________ ________ ___________________________________________
        | Bit    |  Value | Description                               |
        |________|________|___________________________________________|
        | 0      |        | Path Mode                                 |
        |________|________|___________________________________________|
        |        |    0   | Single-path Mode                          |
        |________|________|___________________________________________|
        |        |    1   | Multipath Mode (not supported by Models   |
        |        |        | C10, C11, and C22)                        |
        |________|________|___________________________________________|
        | 1, 2   |        | Group Code                                |
        |________|________|___________________________________________|
        |        |   00   | Establish Group                           |
        |________|________|___________________________________________|
        |        |   01   | Disband Group                             |
        |________|________|___________________________________________|
        |        |   10   | Resign from Group                         |
        |________|________|___________________________________________|
        |        |   11   | Reserved                                  |
        |________|________|___________________________________________|
        | 3-7    |  00000 | Reserved                                  |
        |________|________|___________________________________________|


        The final 11 bytes of the Set Path Group ID command identify
        the path group ID.  The path group ID identifies the channel
        paths that belong to the same controlling computer.  Path group
        ID bytes must be the same for all devices in a control unit
        on a given path.  The Path Group ID bytes cannot be all zeroes.
        */

        /* Command Reject if Supervisor-Inhibit */
        if (dev->supvr_inhibit)
        {
            build_senseX (TAPE_BSENSE_BADCOMMAND, dev, unitstat, code);
            break;
        }

        /* Command reject if the command is not the ONLY command
           in the channel program */
        if (chained & CCW_FLAGS_CC)
        {
            build_senseX (TAPE_BSENSE_BADCOMMAND, dev, unitstat, code);
            break;
        }

        /* Calculate residual byte count */
        RESIDUAL_CALC (12);

        /* Control information length must be at least 12 bytes */
        if (count < 12)
        {
            build_senseX (TAPE_BSENSE_BADCOMMAND, dev, unitstat, code);
            break;
        }

        /* Byte 0 is the path group state byte */
        switch((iobuf[0] & SPG_SET_COMMAND))
        {
        case SPG_SET_ESTABLISH:
            /* Only accept the new pathgroup id when
               1) it has not yet been set (ie contains zeros) or
               2) It is set, but we are setting the same value */
            if(memcmp(dev->pgid,
                 "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 11)
              && memcmp(dev->pgid, iobuf+1, 11))
            {
                build_senseX (TAPE_BSENSE_BADCOMMAND, dev, unitstat, code);
                break;
            }

            /* Bytes 1-11 contain the path group identifier */
            memcpy (dev->pgid, iobuf+1, 11); // (set initial value)
            dev->pgstat = SPG_PATHSTAT_GROUPED | SPG_PARTSTAT_IENABLED;
            build_senseX (TAPE_BSENSE_STATUSONLY, dev, unitstat, code);
            break;

        case SPG_SET_DISBAND:
            dev->pgstat = 0;
            build_senseX (TAPE_BSENSE_STATUSONLY, dev, unitstat, code);
            break;

        default:
        case SPG_SET_RESIGN:
            dev->pgstat = 0;
            memset (dev->pgid, 0, 11);  // (reset to zero)
            build_senseX (TAPE_BSENSE_STATUSONLY, dev, unitstat, code);
            break;

        } // end switch((iobuf[0] & SPG_SET_COMMAND))

        break;

    } /* End case 0xAF: SET PATH GROUP ID */

    /*---------------------------------------------------------------*/
    /* ASSIGN                                                        */
    /*---------------------------------------------------------------*/
    case 0xB7:
    {
        /* Command Reject if Supervisor-Inhibit */
        if (dev->supvr_inhibit)
        {
            build_senseX (TAPE_BSENSE_BADCOMMAND, dev, unitstat, code);
            break;
        }

        /* Calculate residual byte count */
        RESIDUAL_CALC (11);

        /* Control information length must be at least 11 bytes */
        if (count < len)
        {
            build_senseX (TAPE_BSENSE_BADCOMMAND, dev, unitstat, code);
            break;
        }

        if((memcmp(iobuf,"\00\00\00\00\00\00\00\00\00\00",11)==0)
            || (memcmp(iobuf,dev->pgid,11)==0))
        {
            dev->pgstat |= SPG_PARTSTAT_XENABLED; /* Set Explicit Partition Enabled */
        }
        else
        {
            build_senseX (TAPE_BSENSE_BADCOMMAND, dev, unitstat, code);
            break;
        }

        /* Return unit status */
        build_senseX (TAPE_BSENSE_STATUSONLY, dev, unitstat, code);
        break;
    }

    /*---------------------------------------------------------------*/
    /* MEDIUM SENSE   (3590)                                         */
    /*---------------------------------------------------------------*/
    case 0xC2:
    {
        /*      GA32-0331 IBM 3590 Hardware Reference

        The 3590 Hardware Reference manual lists many different
        "Mode Sense" Pages that the 3590 supports, with one of
        the supported pages being Mode Page X'23': the "Medium
        Sense" mode page:

           The Medium Sense page provides information about
           the state of the medium currently associated with
           the device, if any.
        */
        if (!MEDSNS_SUPPORTED())
        {
            build_senseX (TAPE_BSENSE_BADCOMMAND, dev, unitstat, code);
            break;
        }

#if 0 //  ZZ FIXME: not coded yet

        // PROGRAMMING NOTE: until we can add support to Hercules
        // allowing direct SCSI i/o (so that we can issue the 10-byte
        // Mode Sense (X'5A') command to ask for Mode Page x'23' =
        // Medium Sense) we have no choice but to reject the command.

        // ZZ FIXME: not written yet.

        build_senseX (TAPE_BSENSE_BADCOMMAND, dev, unitstat, code);

#else //  ++++  BEGIN MEDIUM SENSE HACK  ++++

        /* ZZ FIXME: ***  TEMPORARY(?) HACK  ***

            The following clues were gleaned from Linux 390 source:

                struct tape_3590_med_sense
                {
                    unsigned int macst:4;
                    unsigned int masst:4;

                    char pad[127];
                }

                #define  MSENSE_UNASSOCIATED       0x00
                #define  MSENSE_ASSOCIATED_MOUNT   0x01
                #define  MSENSE_ASSOCIATED_UMOUNT  0x02

                case TO_MSEN:

                    sense = (struct tape_3590_med_sense *) request->cpdata;

                    if (sense->masst == MSENSE_UNASSOCIATED)
                            tape_med_state_set(device, MS_UNLOADED);

                    if (sense->masst == MSENSE_ASSOCIATED_MOUNT)
                            tape_med_state_set(device, MS_LOADED);
                    break;
        */

        /* Calculate residual byte count */
        RESIDUAL_CALC (128);

        /* Return Media Sense data... */

        memset( iobuf, 0, num );          // (init to all zeroes first)

        if (dev->tmh->tapeloaded( dev, unitstat, code ))
            iobuf[0] |= (0x01 & 0x0F);    // MSENSE_ASSOCIATED_MOUNT
//      else
//          iobuf[0] |= (0x00 & 0x0F);    // MSENSE_UNASSOCIATED

        /* Return unit status */
        build_senseX (TAPE_BSENSE_STATUSONLY, dev, unitstat, code);

#endif //  ++++  END MEDIUM SENSE HACK  ++++

        break;

    } /* End case 0xC2: MEDIUM SENSE */

    /*---------------------------------------------------------------*/
    /* SET TAPE-WRITE IMMEDIATE  (3480 and later)                    */
    /*---------------------------------------------------------------*/
    case 0xC3:
    {
        // NOTE: the "Mode Set" interpretation of this CCW for all
        // models earlier than 3480 are handled by the command-table;
        // the "Set Tape-Write Immediate" interpretation of this CCW
        // for 3480 and later models is handled below.

        /* Command reject if the volume is currently fenced */
        if (dev->fenced)
        {
            build_senseX (TAPE_BSENSE_FENCED, dev, unitstat, code);
            break;
        }

        /*      GA32-0127 IBM 3490E Hardware Reference

        Set Tape-Write-Immediate (X'C3')

        The Set Tape-Write-Immediate command causes all subsequent
        Write commands in the channel program to perform as write-
        immediate commands.

        The tape-write-immediate command is explicitly requested by a
        Mode Set or Set Tape-Write-Immediate command.  The subsystem
        forces the tape-write-immediate command while the tape is
        positioned beyond logical end of volume.  This prevents more
        than one record from being in the buffer if the physical end of
        volume is reached.  It may also be forced when load balancing
        is performed or on drives that write the 3480-2 XF format just
        before end of wrap processing.
        */

        /*     GA32-0329 3590 Introduction and Planning Guide

        When data is physically transferred to the tape medium it is
        always immediately reread and verified. The writing of data
        is normally buffered, however, which defers the physical
        transfer of the logical blocks to the tape until the buffer
        conditions require the offloading of the data or until a
        synchronizing command requires the transfer. If immediate
        validation of a successful transfer of data to the tape is
        required at the time that each logical block is written,
        then Tape Write Immediate mode may be programmatically invoked.
        This results in block-by-block synchronization and verification
        of successful transfer all the way to the medium, but at a
        very substantial cost in application performance.
        */

        /* Assign a unique Message Id for this I/O if needed */
        INCREMENT_MESSAGEID(dev);

        /* set write-immedediate mode and perform sync function */
        dev->write_immed = 1;
        if ((rc = dev->tmh->sync( dev, unitstat, code )) == 0)
            build_senseX( TAPE_BSENSE_STATUSONLY, dev, unitstat, code );
        break;

    } /* End case 0xC3: SET TAPE-WRITE IMMEDIATE */

    /*---------------------------------------------------------------*/
    /* UNASSIGN                                                      */
    /*---------------------------------------------------------------*/
    case 0xC7:
    {
        /* Command Reject if Supervisor-Inhibit */
        if (dev->supvr_inhibit)
        {
            build_senseX (TAPE_BSENSE_BADCOMMAND, dev, unitstat, code);
            break;
        }

        /* Calculate residual byte count */
        RESIDUAL_CALC (11);

        /* Control information length must be at least 11 bytes */
        if (count < len)
        {
            build_senseX (TAPE_BSENSE_BADCOMMAND, dev, unitstat, code);
            break;
        }

        /* Reset to All Implicitly enabled */
        dev->pgstat=0;

        /* Reset Path group ID password */
        memset(dev->pgid, 0, 11);

        /* Reset drive password */
        memset(dev->drvpwd, 0, sizeof(dev->drvpwd));

        /* Return unit status */
        build_senseX (TAPE_BSENSE_STATUSONLY, dev, unitstat, code);
        break;
    }

    /*---------------------------------------------------------------*/
    /* ???????????? (3590)       (MODE SELECT maybe?)                */
    /*---------------------------------------------------------------*/
    case 0xCB:
    {
        // ZZ FIXME: not written yet. (No available documentation!)

        /* Not currently supported; treat as no-op */
        build_senseX (TAPE_BSENSE_STATUSONLY, dev, unitstat, code);
        break;
    }

    /*---------------------------------------------------------------*/
    /* ???????????? (3590)       (MODE SENSE maybe?)                 */
    /*---------------------------------------------------------------*/
    case 0xCF:
    {
        /*     ANSI INCITS 131-1994 (R1999) SCSI-2 Reference

        The MODE SENSE command provides a means for a target to
        report parameters to the initiator. It is a complementary
        command to the MODE SELECT command.
        */

        /*      GA32-0331 IBM 3590 Hardware Reference

        The 3590 Hardware Reference manual lists many different
        "Mode Sense" Pages that the 3590 supports.
        */

        // ZZ FIXME: not written yet. (No available documentation!)

        /* Not currently supported; treat as no-op */
        build_senseX (TAPE_BSENSE_STATUSONLY, dev, unitstat, code);
        break;
    }

    /*---------------------------------------------------------------*/
    /* MODE SET  (3480 or later)                                     */
    /*---------------------------------------------------------------*/
    case 0xDB:
    {
        /*          GA32-0127 IBM 3490E Hardware Reference

        Mode Set (X'DB')

        The Mode Set command controls specific aspects of command
        processing within a given command chain.

        The Mode Set command requires one byte of information from the channel.
        The format of the byte is:

         ________ __________________________________________________________
        | Bit    | Description                                              |
        |________|__________________________________________________________|
        | 0,1    | Reserved                                                 |
        |________|__________________________________________________________|
        | 2      | Tape-Write-Immediate Mode                                |
        |        |                                                          |
        |        | If active, any subsequent Write commands within the      |
        |        | current command chain are processed in tape-write-       |
        |        | immediate mode if no other conditions preclude this      |
        |        | mode.  If inactive, Write commands are processed in      |
        |        | buffered mode if no other conditions preclude this       |
        |        | mode.  The default is inactivate.                        |
        |________|__________________________________________________________|
        | 3      | Supervisor Inhibit                                       |
        |        |                                                          |
        |        | If active, any subsequent supervisor command within      |
        |        | the current command chain is presented unit check        |
        |        | status with associated sense data indicating ERA code    |
        |        | 27.  The supervisor inhibit control also determines      |
        |        | if pending buffered log data is reset when a Read        |
        |        | Buffered Log command is issued.  The default is          |
        |        | inactivate.                                              |
        |________|__________________________________________________________|
        | 4      | Improved Data Recording Capability (IDRC)                |
        |        |                                                          |
        |        | If active, IDRC is invoked for any subsequent Write      |
        |        | commands within the current command chain.  See Table    |
        |        | 7 in topic 1.16.6 for the default settings.              |
        |________|__________________________________________________________|
        | 5-7    | Reserved                                                 |
        |________|__________________________________________________________|

        The Mode Set command is a supervisor command and cannot be performed
        if preceded by a Mode Set command that inhibits supervisor commands.
        */

        /* Command reject if the volume is currently fenced */
        if (dev->fenced)
        {
            build_senseX (TAPE_BSENSE_FENCED, dev, unitstat, code);
            break;
        }

        /* Calculate residual byte count */
        RESIDUAL_CALC (1);

        /* Check for count field of at least 1 byte, and that
           supvr-inhibit mode hasn't already been established */
        if (0
            || count < len
            || dev->supvr_inhibit
        )
        {
            build_senseX(TAPE_BSENSE_BADCOMMAND,dev,unitstat,code);
            break;
        }

        /* Assign a unique Message Id for this I/O if needed */
        INCREMENT_MESSAGEID(dev);

        /* Process request */
        if (iobuf[0] & MSET_SUPVR_INHIBIT)
            dev->supvr_inhibit = 1;         /* set supvr-inhibit mode*/

        if (iobuf[0] & MSET_WRITE_IMMED)
            dev->write_immed = 1;           /* set write-immed. mode */

        if (iobuf[0] & MSET_IDRC)           /* set compression mode  */
        {
            if (!IDR_SUPPORTED())           /* Reject if unsupported */
            {
                build_senseX(TAPE_BSENSE_BADCOMMAND,dev,unitstat,code);
                break;
            }
        }

        build_senseX(TAPE_BSENSE_STATUSONLY,dev,unitstat,code);
        break;

    } /* End case 0xDB: MODE SET */

    /*---------------------------------------------------------------*/
    /* CONTROL ACCESS                                                */
    /*---------------------------------------------------------------*/
    case 0xE3:
    {
        /*          GA32-0127 IBM 3490E Hardware Reference

        Control Access (X'E3')

        The Control Access command is used to perform the set-password,
        conditional-enable, and conditional-disable functions of dynamic
        partitioning.

        The command requires 12 bytes of data to be transferred from the
        channel to the control unit which is defined as follows:

         ________ ________ ___________________________________________
        | Byte   | Bit    | Description                               |
        |________|________|___________________________________________|
        | 0      |        | Function Control                          |
        |________|________|___________________________________________|
        |        | 0,1    | 0  (x'00')  Set Password                  |
        |        |        | 1  (x'40')  Conditional Disable           |
        |        |        | 2  (x'80')  Conditional Enable            |
        |        |        | 3  (x'C0')  Reserved (Invalid)            |
        |________|________|___________________________________________|
        |        | 2-7    | Reserved (must be B'0')                   |
        |________|________|___________________________________________|
        | 1-11   |        | Password                                  |
        |________|________|___________________________________________|
        */

        /* Command Reject if Supervisor-Inhibit */
        if (dev->supvr_inhibit)
        {
            build_senseX (TAPE_BSENSE_BADCOMMAND, dev, unitstat, code);
            break;
        }

        /* Calculate residual byte count */
        RESIDUAL_CALC (12);

        /* Control information length must be at least 12 bytes */
        if (count < len)
        {
            build_senseX (TAPE_BSENSE_BADCOMMAND, dev, unitstat, code);
            break;
        }

        /* Byte 0 is the CAC mode-of-use */
        switch (iobuf[0])
        {
        /*-----------------------------------------------------------*/
        /* Set Password                                              */
        /* 0x00nnnnnnnnnnnnnnnnnnnnnn                                */
        /*-----------------------------------------------------------*/
        case CAC_SET_PASSWORD:
        {
            /* Password must not be zero
               and the device path must be Explicitly Enabled */
            if (0
                || memcmp( iobuf+1, "\00\00\00\00\00\00\00\00\00\00\00", 11 ) == 0
                || (dev->pgstat & SPG_PARTSTAT_XENABLED) == 0
            )
            {
                build_senseX (TAPE_BSENSE_BADCOMMAND, dev, unitstat, code);
                break;
            }

            /* Set Password if none set yet */
            if (memcmp( dev->drvpwd, "\00\00\00\00\00\00\00\00\00\00\00", 11 ) == 0)
            {
                memcpy (dev->drvpwd, iobuf+1, 11);
            }
            else /* Password already set - they must match */
            {
                if (memcmp( dev->drvpwd, iobuf+1, 11 ) != 0)
                {
                    build_senseX (TAPE_BSENSE_BADCOMMAND, dev, unitstat, code);
                    break;
                }
            }
            build_senseX (TAPE_BSENSE_STATUSONLY, dev, unitstat, code);
            break;
        }

        /*-----------------------------------------------------------*/
        /* Conditional Enable                                        */
        /* 0x80nnnnnnnnnnnnnnnnnnnnnn                                */
        /*-----------------------------------------------------------*/
        case CAC_COND_ENABLE:
        {
            /* A drive password must be set and it must match the one given as input */
            if (0
                || memcmp( dev->drvpwd, "\00\00\00\00\00\00\00\00\00\00\00", 11 ) == 0
                || memcmp( dev->drvpwd, iobuf+1, 11 ) != 0
            )
            {
                build_senseX (TAPE_BSENSE_BADCOMMAND, dev, unitstat, code);
                break;
            }
            build_senseX (TAPE_BSENSE_STATUSONLY, dev, unitstat, code);
            break;
        }

        /*-----------------------------------------------------------*/
        /* Conditional Disable                                       */
        /* 0x40nnnnnnnnnnnnnnnnnnnnnn                                */
        /*-----------------------------------------------------------*/
        case CAC_COND_DISABLE:
        {
            /* A drive password is set, it must match the one given as input */
            if (1
                && memcmp (dev->drvpwd, "\00\00\00\00\00\00\00\00\00\00\00", 11) != 0
                && memcmp (dev->drvpwd, iobuf+1, 11) != 0
            )
            {
                build_senseX (TAPE_BSENSE_BADCOMMAND, dev, unitstat, code);
                break;
            }

            build_senseX (TAPE_BSENSE_STATUSONLY, dev, unitstat, code);
            break;
        }

        default:    /* Unsupported Control Access Function */
        {
            build_senseX (TAPE_BSENSE_BADCOMMAND, dev, unitstat, code);
            break;
        }

        } /* End switch (iobuf[0]) */

        break;

    } /* End case 0xE3 CONTROL ACCESS */

    /*---------------------------------------------------------------*/
    /* SENSE ID    (3422 and later)                                  */
    /*---------------------------------------------------------------*/
    case 0xE4:
    {
        /* AUTOMOUNT QUERY - part 2 (if command-chained from prior 0x4B) */
        if (1
            && dev->tapedevt != TAPEDEVT_SCSITAPE
            && sysblk.tamdir != NULL
            && !dev->noautomount
            && (chained & CCW_FLAGS_CC)
            && 0x4B == prevcode
        )
        {
            /* Calculate residual byte count */
            RESIDUAL_CALC ((int)strlen(dev->filename));

            /* Copy device filename to guest storage */
            str_host_to_guest( (const BYTE*)dev->filename, iobuf, num );

            /* Return normal status */
            build_senseX (TAPE_BSENSE_STATUSONLY, dev, unitstat, code);
            break;
        }

        /* SENSE ID did not exist on the 3803 */
        /* If numdevid is 0, then 0xE4 not supported */
        if (dev->numdevid==0)
        {
            build_senseX (TAPE_BSENSE_BADCOMMAND, dev, unitstat, code);
            break;
        }

        /* Calculate residual byte count */
        RESIDUAL_CALC (dev->numdevid);

        /* Copy device identifier bytes to channel I/O buffer */
        memcpy (iobuf, dev->devid, num);

        /* Return unit status */
        build_senseX (TAPE_BSENSE_STATUSONLY, dev, unitstat, code);
        break;
    }

    /*---------------------------------------------------------------*/
    /* READ CONFIGURATION DATA   (3490 and later)                    */
    /*---------------------------------------------------------------*/
    case 0xFA:
    {
        /* Build the configuration data area */
        len = dev->rcd (dev, iobuf, count);

        /* Calculate residual byte count */
        RESIDUAL_CALC (len);

        /* Return normal status */
        build_senseX (TAPE_BSENSE_STATUSONLY, dev, unitstat, code);
        break;
    }

    /*---------------------------------------------------------------*/
    /* INVALID OPERATION                                             */
    /*---------------------------------------------------------------*/
    default:
    {
        /* Set command reject sense byte, and unit check status */
        build_senseX (TAPE_BSENSE_BADCOMMAND, dev, unitstat, code);
    }

    } /* end switch (code) */

} /* end function tapedev_execute_ccw */

/*-------------------------------------------------------------------*/
/* Find next more-restrictive TAMDIR subdirectory entry...           */
/*-------------------------------------------------------------------*/
static TAMDIR* findtamdir( int rej, int minlen, const char* pszDir )
{
    char    szDIR[MAX_PATH + 1];
    char    szTAMDIR[MAX_PATH + 1];
    TAMDIR *pTAMDIR;

    hostpath(szDIR, pszDir, sizeof(szDIR));

    pTAMDIR = sysblk.tamdir;    /* always search entire list */

    do
    {
        hostpath(szTAMDIR, pTAMDIR->dir, sizeof(szTAMDIR));
        if (1
            && pTAMDIR->rej == rej
            && pTAMDIR->len > minlen
            && strnfilenamecmp( szDIR, szTAMDIR, pTAMDIR->len ) == 0
        )
            return pTAMDIR;
    }
        while ((pTAMDIR = pTAMDIR->next) != NULL);
    return NULL;
}

/*-------------------------------------------------------------------*/
/* Load Display channel command processing...                        */
/*-------------------------------------------------------------------*/
int  load_display (DEVBLK *dev, BYTE *buf, U16 count)
{
int             rc = 0;                 /* Return code               */
U16             i;                      /* Array subscript           */
char            msg1[9], msg2[9];       /* Message areas (ASCIIZ)    */
BYTE            fcb;                    /* Format Control Byte       */
BYTE            tapeloaded;             /* (boolean true/false)      */
BYTE*           msg;                    /* (work buf ptr)            */

    if (!count)
        return rc;

    /* Pick up format control byte */
    fcb = *buf;

    /* Copy and translate messages... */

    memset( msg1, 0, sizeof(msg1) );
    memset( msg2, 0, sizeof(msg2) );

    msg = buf+1;

    for (i=0; *msg && i < 8 && ((i+1)+0) < count; i++)
        msg1[i] = guest_to_host(*msg++);

    msg = buf+1+8;

    for (i=0; *msg && i < 8 && ((i+1)+8) < count; i++)
        msg2[i] = guest_to_host(*msg++);

    msg1[ sizeof(msg1) - 1 ] = 0;
    msg2[ sizeof(msg2) - 1 ] = 0;

    tapeloaded = dev->tmh->tapeloaded( dev, NULL, 0 );

    switch ( fcb & FCB_FS )  // (high-order 3 bits)
    {
    case FCB_FS_READYGO:     // 0x00

        /*
        || 000b: "The message specified in bytes 1-8 and 9-16 is
        ||       maintained until the tape drive next starts tape
        ||       motion, or until the message is updated."
        */

        dev->tapedispflags = 0;

        STRLCPY( dev->tapemsg1, msg1 );
        STRLCPY( dev->tapemsg2, msg2 );

        dev->tapedisptype  = TAPEDISPTYP_WAITACT;

        break;

    case FCB_FS_UNMOUNT:     // 0x20

        /*
        || 001b: "The message specified in bytes 1-8 is maintained
        ||       until the tape cartridge is physically removed from
        ||       the tape drive, or until the next unload/load cycle.
        ||       If the drive does not contain a cartridge when the
        ||       Load Display command is received, the display will
        ||       contain the message that existed prior to the receipt
        ||       of the command."
        */

        dev->tapedispflags = 0;

        if ( tapeloaded )
        {
            dev->tapedisptype  = TAPEDISPTYP_UNMOUNT;
            dev->tapedispflags = TAPEDISPFLG_REQAUTOMNT;

            STRLCPY( dev->tapemsg1, msg1 );

            if (dev->ccwtrace)
                WRMSG(HHC00218, "I", LCSS_DEVNUM, dev->filename, TTYPSTR(dev->tapedevt), dev->tapemsg1 );
        }

        break;

    case FCB_FS_MOUNT:       // 0x40

        /*
        || 010b: "The message specified in bytes 1-8 is maintained
        ||       until the drive is next loaded. If the drive is
        ||       loaded when the Load Display command is received,
        ||       the display will contain the message that existed
        ||       prior to the receipt of the command."
        */

        dev->tapedispflags = 0;

        if ( !tapeloaded )
        {
            dev->tapedisptype  = TAPEDISPTYP_MOUNT;
            dev->tapedispflags = TAPEDISPFLG_REQAUTOMNT;

            STRLCPY( dev->tapemsg1, msg1 );

            if (dev->ccwtrace)
                WRMSG(HHC00218, "I", LCSS_DEVNUM, dev->filename, TTYPSTR(dev->tapedevt), dev->tapemsg1 );
        }

        break;

    case FCB_FS_NOP:         // 0x60
    default:

        /*
        || 011b: "This value is used to physically access a drive
        ||       without changing the message display. This option
        ||       can be used to test whether a control unit can
        ||       physically communicate with a drive."
        */

        return rc;

    case FCB_FS_RESET_DISPLAY: // 0x80

        /*
        || 100b: "The host message being displayed is cancelled and
        ||       a unit message is displayed instead."
        */

        dev->tapedispflags = 0;
        dev->tapedisptype  = TAPEDISPTYP_IDLE;

        break;

    case FCB_FS_UMOUNTMOUNT: // 0xE0

        /*
        || 111b: "The message in bytes 1-8 is displayed until a tape
        ||       cartridge is physically removed from the tape drive,
        ||       or until the drive is next loaded. The message in
        ||       bytes 9-16 is displayed until the drive is next loaded.
        ||       If no cartridge is present in the drive, the first
        ||       message is ignored and only the second message is
        ||       displayed until the drive is next loaded."
        */

        dev->tapedispflags = 0;

        STRLCPY( dev->tapemsg1, msg1 );
        STRLCPY( dev->tapemsg2, msg2 );

        if ( tapeloaded )
        {
            dev->tapedisptype  = TAPEDISPTYP_UMOUNTMOUNT;
            dev->tapedispflags = TAPEDISPFLG_REQAUTOMNT;

            if (dev->ccwtrace)
                WRMSG(HHC00219, "I", LCSS_DEVNUM, dev->filename, TTYPSTR(dev->tapedevt), dev->tapemsg1, dev->tapemsg2 );
        }
        else
        {
            dev->tapedisptype  = TAPEDISPTYP_MOUNT;
            dev->tapedispflags = TAPEDISPFLG_MESSAGE2 | TAPEDISPFLG_REQAUTOMNT;

            if (dev->ccwtrace)
                WRMSG(HHC00218, "I", LCSS_DEVNUM, dev->filename, TTYPSTR(dev->tapedevt), dev->tapemsg2 );
        }

        break;
    }

    /* Set the flags... */

    /*
        "When bit 7 (FCB_AL) is active and bits 0-2 (FCB_FS) specify
        a Mount Message, then only the first eight characters of the
        message are displayed and bits 3-5 (FCB_AM, FCB_BM, FCB_M2)
        are ignored."
    */
    if (1
        &&   ( fcb & FCB_AL )
        && ( ( fcb & FCB_FS ) == FCB_FS_MOUNT )
    )
    {
        fcb  &=  ~( FCB_AM | FCB_BM | FCB_M2 );
        dev->tapedispflags &= ~TAPEDISPFLG_MESSAGE2;
    }

    /*
        "When bit 7 (FCB_AL) is active and bits 0-2 (FCB_FS) specify
        a Demount/Mount message, then only the last eight characters
        of the message are displayed. Bits 3-5 (FCB_AM, FCB_BM, FCB_M2)
        are ignored."
    */
    if (1
        &&   ( fcb & FCB_AL )
        && ( ( fcb & FCB_FS ) == FCB_FS_UMOUNTMOUNT )
    )
    {
        fcb  &=  ~( FCB_AM | FCB_BM | FCB_M2 );
        dev->tapedispflags |= TAPEDISPFLG_MESSAGE2;
    }

    /*
        "When bit 3 (FCB_AM) is set to 1, then bits 4 (FCB_BM) and 5
        (FCB_M2) are ignored."
    */
    if ( fcb & FCB_AM )
        fcb  &=  ~( FCB_BM | FCB_M2 );

    dev->tapedispflags |= (((fcb & FCB_AM) ? TAPEDISPFLG_ALTERNATE  : 0 ) |
                          ( (fcb & FCB_BM) ? TAPEDISPFLG_BLINKING   : 0 ) |
                          ( (fcb & FCB_M2) ? TAPEDISPFLG_MESSAGE2   : 0 ) |
                          ( (fcb & FCB_AL) ? TAPEDISPFLG_AUTOLOADER : 0 ));

    UpdateDisplay( dev );
    rc = ReqAutoMount( dev );
    return rc;

} /* end function load_display */


/*********************************************************************/
/*********************************************************************/
/**                                                                 **/
/**                 SENSE CCW HANDLING FUNCTIONS                    **/
/**                                                                 **/
/*********************************************************************/
/*********************************************************************/

/*-------------------------------------------------------------------*/
/*                        build_senseX                               */
/*-------------------------------------------------------------------*/
/*  Construct sense bytes and unit status                            */
/*  Note: name changed because semantic changed                      */
/*  ERCode is our internal ERror-type code                           */
/*                                                                   */
/*  Uses the 'TapeSenseTable' table index                            */
/*  from the 'TapeDevtypeList' table to route call to                */
/*  one of the below device-specific sense functions                 */
/*-------------------------------------------------------------------*/
void build_senseX (int ERCode, DEVBLK *dev, BYTE *unitstat, BYTE ccwcode)
{
int i;
BYTE usr;
int sense_built;
    sense_built = 0;
    if(unitstat==NULL)
    {
        unitstat = &usr;
    }
    for(i = 0;TapeDevtypeList[i] != 0; i += TAPEDEVTYPELIST_ENTRYSIZE)
    {
        if (TapeDevtypeList[i] == dev->devtype)
        {
            // Clear old sense if we're going to completely rebuild it...

            if (TAPE_BSENSE_STATUSONLY != ERCode)
            {
                memset( dev->sense, 0, sizeof(dev->sense) );
                dev->sns_pending = 0;
            }

            // Call the primary sense function (e.g. "build_sense_3480_etal")...

            TapeSenseTable[TapeDevtypeList[i+4]](ERCode,dev,unitstat,ccwcode);
            sense_built = 1;

            // Unit-exception s/b signalled for all write operations
            // once the end-of-tape (EOT) reflector has been passed...

            if (1
                && TAPE_BSENSE_STATUSONLY == ERCode
                &&
                (0
                    || 0x01 == ccwcode      // write
                    || 0x17 == ccwcode      // erase gap
                    || 0x1F == ccwcode      // write tapemark
                )
                && dev->tmh->passedeot(dev)
            )
            {
                // We're still in the "Early Warning Zone",
                // so keep warning them...

                *unitstat |= CSW_UX;        // ("Warning!")
            }
            break;
        }
    }
    if (!sense_built)
    {
        memset( dev->sense, 0, sizeof(dev->sense) );
        dev->sense[0]=SENSE_EC;
        *unitstat = CSW_CE|CSW_DE|CSW_UC;
    }
    if (*unitstat & CSW_UC)
    {
        dev->sns_pending = 1;
    }
    return;

} /* end function build_senseX */

/*-------------------------------------------------------------------*/
/*                  build_sense_3410_3420                            */
/*-------------------------------------------------------------------*/
void build_sense_3410_3420 (int ERCode, DEVBLK *dev, BYTE *unitstat, BYTE ccwcode)
{
    // NOTE: caller should have cleared sense area to zeros
    //       if this isn't a 'TAPE_BSENSE_STATUSONLY' call

    UNREFERENCED(ccwcode);

    switch(ERCode)
    {
    case TAPE_BSENSE_TAPEUNLOADED:
        *unitstat = CSW_UC;
        dev->sense[0] = SENSE_IR;
        dev->sense[1] = SENSE1_TAPE_TUB;
        break;
    case TAPE_BSENSE_RUN_SUCCESS: /* RewUnld op */
        /* FIXME : CE Should have been presented before */
        /*         Same as for 348x drives */
        *unitstat = CSW_CE | CSW_UC | CSW_DE | CSW_CUE;
        /*
        *unitstat = CSW_UC | CSW_DE | CSW_CUE;
        */
        dev->sense[0] = SENSE_IR;
        dev->sense[1] = SENSE1_TAPE_TUB;
        break;
    case TAPE_BSENSE_REWINDFAILED:
    case TAPE_BSENSE_FENCED:
    case TAPE_BSENSE_EMPTYTAPE:
    case TAPE_BSENSE_ENDOFTAPE:
    case TAPE_BSENSE_BLOCKSHORT:
        /* On 3411/3420 the tape runs off the reel in that case */
        /* this will cause pressure loss in both columns */
    case TAPE_BSENSE_LOCATEERR:
        /* Locate error: This is more like improperly formatted tape */
        /* i.e. the tape broke inside the drive                       */
        /* So EC instead of DC                                        */
    case TAPE_BSENSE_TAPELOADFAIL:
        *unitstat = CSW_CE|CSW_DE|CSW_UC;
        dev->sense[0] = SENSE_EC;
        dev->sense[1] = SENSE1_TAPE_TUB;
        dev->sense[7] = 0x60;
        break;
    case TAPE_BSENSE_ITFERROR:
        *unitstat = CSW_CE|CSW_DE|CSW_UC;
        dev->sense[0] = SENSE_EC;
        dev->sense[1] = SENSE1_TAPE_TUB;
        dev->sense[4] = 0x80; /* Tape Unit Reject */
        break;
    case TAPE_BSENSE_READFAIL:
    case TAPE_BSENSE_BADALGORITHM:
        *unitstat = CSW_CE|CSW_DE|CSW_UC;
        dev->sense[0] = SENSE_DC;
        dev->sense[3] = 0xC0; /* Vertical CRC check & Multitrack error */
        break;
    case TAPE_BSENSE_WRITEFAIL:
        *unitstat = CSW_CE|CSW_DE|CSW_UC;
        dev->sense[0] = SENSE_DC;
        dev->sense[3] = 0x60; /* Longitudinal CRC check & Multitrack error */
        break;
    case TAPE_BSENSE_BADCOMMAND:
    case TAPE_BSENSE_INCOMPAT:
        *unitstat = CSW_UC;
        dev->sense[0] = SENSE_CR;
        dev->sense[4] = 0x01;
        break;
    case TAPE_BSENSE_WRITEPROTECT:
        *unitstat = CSW_CE|CSW_DE|CSW_UC;
        dev->sense[0] = SENSE_CR;
        break;
    case TAPE_BSENSE_LOADPTERR:
        *unitstat = CSW_CE|CSW_DE|CSW_UC;
        dev->sense[0] = 0;
        break;
    case TAPE_BSENSE_READTM:
        *unitstat = CSW_CE|CSW_DE|CSW_UX;
        break;
    case TAPE_BSENSE_UNSOLICITED:
        *unitstat = CSW_CE|CSW_DE;
        break;
    case TAPE_BSENSE_STATUSONLY:
        *unitstat = CSW_CE|CSW_DE;
        break;
    } // end switch(ERCode)

    if (TAPE_BSENSE_STATUSONLY == ERCode)
        return; // (mission accomplished)

    /* Fill in the common sense information */

    if (strcmp(dev->filename,TAPE_UNLOADED) == 0
        || !dev->tmh->tapeloaded(dev,NULL,0))
    {
        dev->sense[0] |= SENSE_IR;
        dev->sense[1] |= SENSE1_TAPE_FP;
        dev->sense[1] &= ~SENSE1_TAPE_TUA;
        dev->sense[1] |= SENSE1_TAPE_TUB;
    }
    else
    {
        dev->sense[0] &= ~SENSE_IR;
        dev->sense[1] |= IsAtLoadPoint( dev ) ? SENSE1_TAPE_LOADPT : 0;
        dev->sense[1] |= dev->readonly || dev->tdparms.logical_readonly ?
            SENSE1_TAPE_FP : 0;
        dev->sense[1] |= SENSE1_TAPE_TUA;
        dev->sense[1] &= ~SENSE1_TAPE_TUB;
    }
    if (dev->tmh->passedeot(dev))
    {
        dev->sense[4] |= 0x40;
    }

} /* end function build_sense_3410_3420 */

/*-------------------------------------------------------------------*/
/*                     build_sense_3410                              */
/*-------------------------------------------------------------------*/
void build_sense_3410 (int ERCode, DEVBLK *dev, BYTE *unitstat, BYTE ccwcode)
{
    build_sense_3410_3420(ERCode,dev,unitstat,ccwcode);

    dev->sense[5] &= 0x80;
    dev->sense[5] |= 0x40;
    dev->sense[6] = 0x22; /* Dual Dens - 3410/3411 Model 2 */
    dev->numsense = 9;

} /* end function build_sense_3410 */

/*-------------------------------------------------------------------*/
/*                     build_sense_3420                              */
/*-------------------------------------------------------------------*/
void build_sense_3420 (int ERCode, DEVBLK *dev, BYTE *unitstat, BYTE ccwcode)
{
    build_sense_3410_3420(ERCode,dev,unitstat,ccwcode);

    /* Following stripped from original 'build_sense' */
    dev->sense[5] |= 0xC0;
    dev->sense[6] |= 0x03;
    dev->sense[13] = 0x80;
    dev->sense[14] = 0x01;
    dev->sense[15] = 0x00;
    dev->sense[16] = 0x01;
    dev->sense[19] = 0xFF;
    dev->sense[20] = 0xFF;
    dev->numsense = 24;

} /* end function build_sense_3420 */

/*-------------------------------------------------------------------*/
/*                     build_sense_3480_etal                         */
/*-------------------------------------------------------------------*/
void build_sense_3480_etal (int ERCode,DEVBLK *dev,BYTE *unitstat,BYTE ccwcode)
{
int sns4mat = TAPE_SNS7_FMT_20_3480;

    // NOTE: caller should have cleared sense area to zeros
    //       if this isn't a 'TAPE_BSENSE_STATUSONLY' call

    switch(ERCode)
    {
    case TAPE_BSENSE_TAPEUNLOADED:
        dev->sense[0] = TAPE_SNS0_INTVREQ;
        dev->sense[3] = TAPE_ERA_43_DRIVE_NOT_READY; /* ERA 43 = Int Req */
        *unitstat = CSW_UC;
        break;
    case TAPE_BSENSE_RUN_SUCCESS:        /* Not an error */
        /* NOT an error, But according to GA32-0219-02 2.1.2.2
           Rewind Unload always ends with with DE+UC on secondary status */
        /* FIXME! */
        /* Note that Initial status & Secondary statuses are merged here */
        /* when they should be presented separatly */
        *unitstat = CSW_CE|CSW_DE|CSW_UC;
        dev->sense[0] = TAPE_SNS0_INTVREQ;
        dev->sense[3] = TAPE_ERA_2B_ENVIRONMENTAL_DATA_PRESENT;
        sns4mat = TAPE_SNS7_FMT_22_3480_EOV_STATS;
        break;
    case TAPE_BSENSE_TAPELOADFAIL:
        *unitstat = CSW_CE|CSW_DE|CSW_UC;
        dev->sense[0] = TAPE_SNS0_INTVREQ|TAPE_SNS0_DEFUNITCK;
        dev->sense[3] = TAPE_ERA_33_LOAD_FAILURE; /* ERA 33 = Load Failed */
        break;
    case TAPE_BSENSE_READFAIL:
        *unitstat = CSW_CE|CSW_DE|CSW_UC;
        dev->sense[0] = TAPE_SNS0_DATACHK;
        dev->sense[3] = TAPE_ERA_23_READ_DATA_CHECK;
        break;
    case TAPE_BSENSE_WRITEFAIL:
        *unitstat = CSW_CE|CSW_DE|CSW_UC;
        dev->sense[0] = TAPE_SNS0_DATACHK;
        dev->sense[3] = TAPE_ERA_25_WRITE_DATA_CHECK;
        break;
    case TAPE_BSENSE_BADCOMMAND:
        *unitstat = CSW_UC;
        dev->sense[0] = TAPE_SNS0_CMDREJ;
        dev->sense[3] = TAPE_ERA_27_COMMAND_REJECT;
        break;
    case TAPE_BSENSE_INCOMPAT:
        *unitstat = CSW_CE|CSW_DE|CSW_UC;
        dev->sense[0] = TAPE_SNS0_CMDREJ;
        dev->sense[3] = TAPE_ERA_29_FUNCTION_INCOMPATIBLE;
        break;
    case TAPE_BSENSE_WRITEPROTECT:
        *unitstat = CSW_CE|CSW_DE|CSW_UC;
        dev->sense[0] = TAPE_SNS0_CMDREJ;
        dev->sense[3] = TAPE_ERA_30_WRITE_PROTECTED;
        break;
    case TAPE_BSENSE_EMPTYTAPE:
        *unitstat = CSW_CE|CSW_DE|CSW_UC;
        dev->sense[0] = TAPE_SNS0_DATACHK;
        dev->sense[3] = TAPE_ERA_31_TAPE_VOID;
        break;
    case TAPE_BSENSE_ENDOFTAPE:
        *unitstat = CSW_CE|CSW_DE|CSW_UC;
        dev->sense[0] = TAPE_SNS0_EQUIPCHK;
        dev->sense[3] = TAPE_ERA_38_PHYSICAL_END_OF_TAPE;
        break;
    case TAPE_BSENSE_LOADPTERR:
        *unitstat = CSW_CE|CSW_DE|CSW_UC;
        dev->sense[0] = 0;
        dev->sense[3] = TAPE_ERA_39_BACKWARD_AT_BOT;
        break;
    case TAPE_BSENSE_FENCED:
        *unitstat = CSW_CE|CSW_DE|CSW_UC;
        dev->sense[0] = TAPE_SNS0_EQUIPCHK|TAPE_SNS0_DEFUNITCK; /* Deffered UC */
        dev->sense[3] = TAPE_ERA_47_VOLUME_FENCED;
        break;
    case TAPE_BSENSE_BADALGORITHM:
        *unitstat = CSW_CE|CSW_DE|CSW_UC;
        dev->sense[0] = TAPE_SNS0_EQUIPCHK;
        if (dev->devtype==0x3480)
        {
            dev->sense[3] = TAPE_ERA_47_VOLUME_FENCED;   // (volume fenced)
        }
        else // 3490, 3590, etc.
        {
            dev->sense[3] = TAPE_ERA_5E_COMPACT_ALGORITHM_INCOMPAT;   // (bad compaction algorithm)
        }
        break;
    case TAPE_BSENSE_LOCATEERR:
        *unitstat = CSW_CE|CSW_DE|CSW_UC;
        dev->sense[0] = TAPE_SNS0_EQUIPCHK;
        dev->sense[3] = TAPE_ERA_44_LOCATE_BLOCK_FAILED;
        break;
    case TAPE_BSENSE_BLOCKSHORT:
        *unitstat = CSW_CE|CSW_DE|CSW_UC;
        dev->sense[0] = TAPE_SNS0_EQUIPCHK;
        dev->sense[3] = TAPE_ERA_36_END_OF_DATA;
        break;
    case TAPE_BSENSE_ITFERROR:
        *unitstat = CSW_CE|CSW_DE|CSW_UC;
        dev->sense[0] = TAPE_SNS0_EQUIPCHK;
        dev->sense[3] = TAPE_ERA_22_PATH_EQUIPMENT_CHECK;
        break;
    case TAPE_BSENSE_REWINDFAILED:
        *unitstat = CSW_CE|CSW_DE|CSW_UC;
        dev->sense[0] = TAPE_SNS0_EQUIPCHK;
        dev->sense[3] = TAPE_ERA_2C_PERMANENT_EQUIPMENT_CHECK; /* Generic Equipment Malfunction ERP code */
        break;
    case TAPE_BSENSE_READTM:
        *unitstat = CSW_CE|CSW_DE|CSW_UX;
        break;
    case TAPE_BSENSE_UNSOLICITED:
        *unitstat = CSW_CE|CSW_DE;
        dev->sense[3] = TAPE_ERA_00_UNSOLICITED_SENSE;
        break;
    case TAPE_BSENSE_STATUSONLY:
    default:
        if ( ccwcode == 0x24 )      // READ BUFFERED LOG
        {
            if ( dev->tdparms.compress == 0 )
                sns4mat = TAPE_SNS7_FMT_21_3480_READ_BUF_LOG;
            else
                sns4mat = TAPE_SNS7_FMT_30_3480_READ_BUF_LOG;
        }
        *unitstat = CSW_CE|CSW_DE;
        break;
    } // end switch(ERCode)

    if (TAPE_BSENSE_STATUSONLY == ERCode)
        return; // (mission accomplished)

    /* Fill in the common sense information */

    if ( sns4mat == TAPE_SNS7_FMT_20_3480              ||
         sns4mat == TAPE_SNS7_FMT_21_3480_READ_BUF_LOG ||
         sns4mat == TAPE_SNS7_FMT_22_3480_EOV_STATS    ||
         sns4mat == TAPE_SNS7_FMT_30_3480_READ_BUF_LOG )
    {
        dev->sense[7] = sns4mat;
        memset(&dev->sense[8], 0,31-8);

        if ( sns4mat == TAPE_SNS7_FMT_20_3480 )
        {
            dev->sense[25] = 0x06;                  // IDRC Installed & Upgraded Buffer
            if ( sysblk.tamdir != NULL )            // is AUTOLOADER ENABLED
            {
                dev->sense[25] |= 0x01;             // ACL is installed
            }
        }

        if ( dev->devtype == 0x3480 )
        {
            dev->sense[27] = 0xf0;                  // indicate 3480-A22/B22
        }
        else if ( dev->devtype==0x3490 )
        {
            dev->sense[27] = 0xe0;                  // indicate 3490-D31/D32
        }
        else if ( dev->devtype==0x3590 )
        {
            dev->sense[27] = 0xe0;                  // indicate same as 3490 for now
        }

        /* create a serial Number */
        dev->sense[27] |= 0x0C;
        dev->sense[28] = (BYTE)( ( dev->devnum >> 12 ) & 0xFF );
        dev->sense[29] = (BYTE)( ( dev->devnum >>  4 ) & 0xFF );

        dev->sense[30] = (BYTE)( dev->devnum & 0x000F ) | ( (BYTE)((BYTE)( dev->devnum & 0x000F )) << 4 );
    }

    if (0
        || strcmp( dev->filename, TAPE_UNLOADED ) == 0
        || (1
            &&  dev->tmh
            && !dev->tmh->tapeloaded( dev, NULL, 0 )
           )
    )
    {
        dev->sense[0] |= TAPE_SNS0_INTVREQ;
        dev->sense[1] |= TAPE_SNS1_FILEPROT;
    }
    else
    {
        dev->sense[0] &= ~TAPE_SNS0_INTVREQ;
        dev->sense[1] &= ~(TAPE_SNS1_BOT|TAPE_SNS1_FILEPROT);
        dev->sense[1] |= IsAtLoadPoint( dev ) ? TAPE_SNS1_BOT : 0;
        dev->sense[1] |= dev->readonly || dev->tdparms.logical_readonly ?
            TAPE_SNS1_FILEPROT : 0;
    }

    dev->sense[1] |= TAPE_SNS1_ONLINE;
    dev->sense[2] |= TAPE_SNS2_REPORTING_CHAN_A;

} /* end function build_sense_3480_etal */

/*-------------------------------------------------------------------*/
/*                     build_sense_3490                              */
/*-------------------------------------------------------------------*/
void build_sense_3490 (int ERCode, DEVBLK *dev, BYTE *unitstat, BYTE ccwcode)
{
    // Until we know for sure that we have to do something different,
    // we should be able to safely use the 3480 sense function here...

    build_sense_3480_etal( ERCode, dev, unitstat, ccwcode );
}

/*-------------------------------------------------------------------*/
/*                     build_sense_3590                              */
/*-------------------------------------------------------------------*/
void build_sense_3590 (int ERCode, DEVBLK *dev, BYTE *unitstat, BYTE ccwcode)
{
    unsigned char ERA;
    // Until we know for sure that we have to do something different,
    // we should be able to safely use the 3480 sense function here...

    build_sense_3480_etal( ERCode, dev, unitstat, ccwcode );

/* This information is from GA32-0127-03 sections 4.2 and 4.3
   although it is a 3490E reference guide, the values are correct
   for a 3590. For the 3480 and 3490 devices, these actions are
   determined by the OS. For 3590, the device through sense byte
   2 (two) tells the OS what to do.
*/
    ERA = dev->sense[3];

    switch ( ERA )          /* Set Software Recovery */
    {
    case TAPE_ERA_00_UNSOLICITED_SENSE:
        if ( unitstat[0] & CSW_UC )
        {
            dev->sense[2] |= TAPE_SNS2_NTP_BRAC_00_PERM_ERR;
            dev->sense[2] |= TAPE_SNS2_NTP_LOG_CD2_PERM_OBR;
        }
        else
        {
            dev->sense[2] |= TAPE_SNS2_NTP_LOG_CD0_NO_LOG;
        }
        break;
        /* Software Recovery - CONTINUE */
    case TAPE_ERA_2B_ENVIRONMENTAL_DATA_PRESENT:
    case TAPE_ERA_52_END_OF_VOLUME_COMPLETE:
        dev->sense[2] |= TAPE_SNS2_NTP_BRAC_01_CONTINUE;
        break;
        /* Software Recovery - REISSUE */
    case TAPE_ERA_21_DATA_STREAMING_NOT_OPER:
    case TAPE_ERA_2A_UNSOL_ENVIRONMENTAL_DATA:
    case TAPE_ERA_42_DEGRADED_MODE:
    case TAPE_ERA_48_UNSOL_INFORMATIONAL_DATA:
    case TAPE_ERA_4C_RECOVERED_CHECKONE_FAILURE:
    case TAPE_ERA_50_BUFFERED_LOG_OVERFLOW:
    case TAPE_ERA_51_BUFFERED_LOG_END_OF_VOLUME:
    case TAPE_ERA_53_GLOBAL_COMMAND_INTERCEPT:
    case TAPE_ERA_54_TEMP_CHNL_INTERFACE_ERROR:
    case TAPE_ERA_57_GLOBAL_STATUS_INTERCEPT:
        dev->sense[2] |= TAPE_SNS2_NTP_BRAC_10_REISSUE;
        break;
        /* Software Recovery - DEFER REISSUE */
    case TAPE_ERA_33_LOAD_FAILURE:
    case TAPE_ERA_34_UNLOAD_FAILURE:
    case TAPE_ERA_3A_DRIVE_SWITCHED_NOT_READY:
    case TAPE_ERA_43_DRIVE_NOT_READY:
        dev->sense[2] |= TAPE_SNS2_NTP_BRAC_11_DEFER_REISS;
        break;
    default:
        /* Software Recovery - NONE - PERMANENT */
        dev->sense[2] |= TAPE_SNS2_NTP_BRAC_00_PERM_ERR;
        break;
    }

    /* Check for OBR  */
    switch ( ERA )
    {
    case TAPE_ERA_00_UNSOLICITED_SENSE:
        break;
    case TAPE_ERA_24_LOAD_DISPLAY_CHECK:
    case TAPE_ERA_26_READ_OPPOSITE:
    case TAPE_ERA_27_COMMAND_REJECT:
    case TAPE_ERA_29_FUNCTION_INCOMPATIBLE:
    case TAPE_ERA_2A_UNSOL_ENVIRONMENTAL_DATA:
    case TAPE_ERA_2B_ENVIRONMENTAL_DATA_PRESENT:
    case TAPE_ERA_30_WRITE_PROTECTED:
    case TAPE_ERA_38_PHYSICAL_END_OF_TAPE:
    case TAPE_ERA_3A_DRIVE_SWITCHED_NOT_READY:
    case TAPE_ERA_43_DRIVE_NOT_READY:
    case TAPE_ERA_44_LOCATE_BLOCK_FAILED:
    case TAPE_ERA_45_DRIVE_ASSIGNED_ELSEWHERE:
    case TAPE_ERA_46_DRIVE_NOT_ONLINE:
    case TAPE_ERA_4E_MAX_BLOCKSIZE_EXCEEDED:
    case TAPE_ERA_50_BUFFERED_LOG_OVERFLOW:
    case TAPE_ERA_51_BUFFERED_LOG_END_OF_VOLUME:
    case TAPE_ERA_52_END_OF_VOLUME_COMPLETE:
    case TAPE_ERA_53_GLOBAL_COMMAND_INTERCEPT:
    case TAPE_ERA_57_GLOBAL_STATUS_INTERCEPT:
        /* OBR - NO LOG */
        dev->sense[2] |= TAPE_SNS2_NTP_LOG_CD0_NO_LOG;
        break;
    case TAPE_ERA_21_DATA_STREAMING_NOT_OPER:
    case TAPE_ERA_33_LOAD_FAILURE:
    case TAPE_ERA_34_UNLOAD_FAILURE:
    case TAPE_ERA_48_UNSOL_INFORMATIONAL_DATA:
    case TAPE_ERA_4D_RESETTING_EVENT:
    case TAPE_ERA_54_TEMP_CHNL_INTERFACE_ERROR:
        /* OBR - TEMP */
        dev->sense[2] |= TAPE_SNS2_NTP_LOG_CD1_TEMP_OBR;
        break;
    default:
        /* OBR - NONE - PERMANENT */
        if ( ERA == TAPE_ERA_39_BACKWARD_AT_BOT && dev->blockid == 0 )
            break;
        dev->sense[2] |= TAPE_SNS2_NTP_LOG_CD2_PERM_OBR;
        break;

    }
}

/*-------------------------------------------------------------------*/
/*                    build_sense_Streaming                          */
/*                      (8809, 9347, 9348)                           */
/*-------------------------------------------------------------------*/
void build_sense_Streaming (int ERCode, DEVBLK *dev, BYTE *unitstat, BYTE ccwcode)
{
    // NOTE: caller should have cleared sense area to zeros
    //       if this isn't a 'TAPE_BSENSE_STATUSONLY' call

    UNREFERENCED(ccwcode);

    switch(ERCode)
    {
    case TAPE_BSENSE_TAPEUNLOADED:
        *unitstat = CSW_UC;
        dev->sense[0] = SENSE_IR;
        dev->sense[3] = 6;        /* Int Req ERAC */
        break;
    case TAPE_BSENSE_RUN_SUCCESS: /* RewUnld op */
        *unitstat = CSW_UC | CSW_CE | CSW_DE | CSW_CUE;
        /*
        *unitstat = CSW_CE | CSW_UC | CSW_DE | CSW_CUE;
        */
        dev->sense[0] = SENSE_IR;
        dev->sense[3] = 6;        /* Int Req ERAC */
        break;
    case TAPE_BSENSE_REWINDFAILED:
    case TAPE_BSENSE_ITFERROR:
        dev->sense[0] = SENSE_EC;
        dev->sense[3] = 0x03;     /* Perm Equip Check */
        *unitstat = CSW_CE|CSW_DE|CSW_UC;
        break;
    case TAPE_BSENSE_TAPELOADFAIL:
    case TAPE_BSENSE_LOCATEERR:
    case TAPE_BSENSE_ENDOFTAPE:
    case TAPE_BSENSE_EMPTYTAPE:
    case TAPE_BSENSE_FENCED:
    case TAPE_BSENSE_BLOCKSHORT:
    case TAPE_BSENSE_INCOMPAT:
        dev->sense[0] = SENSE_EC;
        dev->sense[3] = 0x10; /* PE-ID Burst Check */
        *unitstat = CSW_CE|CSW_DE|CSW_UC;
        break;
    case TAPE_BSENSE_BADALGORITHM:
    case TAPE_BSENSE_READFAIL:
        dev->sense[0] = SENSE_DC;
        dev->sense[3] = 0x09;     /* Read Data Check */
        *unitstat = CSW_CE|CSW_DE|CSW_UC;
        break;
    case TAPE_BSENSE_WRITEFAIL:
        dev->sense[0] = SENSE_DC;
        dev->sense[3] = 0x07;     /* Write Data Check (Media Error) */
        *unitstat = CSW_CE|CSW_DE|CSW_UC;
        break;
    case TAPE_BSENSE_BADCOMMAND:
        dev->sense[0] = SENSE_CR;
        dev->sense[3] = 0x0C;     /* Bad Command */
        *unitstat = CSW_UC;
        break;
    case TAPE_BSENSE_WRITEPROTECT:
        dev->sense[0] = SENSE_CR;
        dev->sense[3] = 0x0B;     /* File Protect */
        *unitstat = CSW_CE|CSW_DE|CSW_UC;
        break;
    case TAPE_BSENSE_LOADPTERR:
        dev->sense[0] = SENSE_CR;
        dev->sense[3] = 0x0D;     /* Backspace at Load Point */
        *unitstat = CSW_CE|CSW_DE|CSW_UC;
        break;
    case TAPE_BSENSE_READTM:
        *unitstat = CSW_CE|CSW_DE|CSW_UX;
        break;
    case TAPE_BSENSE_UNSOLICITED:
        *unitstat = CSW_CE|CSW_DE;
        break;
    case TAPE_BSENSE_STATUSONLY:
        *unitstat = CSW_CE|CSW_DE;
        break;
    } // end switch(ERCode)

    if (TAPE_BSENSE_STATUSONLY == ERCode)
        return; // (mission accomplished)

    /* Fill in the common sense information */

    if (strcmp(dev->filename,TAPE_UNLOADED) == 0
        || !dev->tmh->tapeloaded(dev,NULL,0))
    {
        dev->sense[0] |= SENSE_IR;
        dev->sense[1] |= SENSE1_TAPE_FP;
        dev->sense[1] &= ~SENSE1_TAPE_TUA;
        dev->sense[1] |= SENSE1_TAPE_TUB;
    }
    else
    {
        dev->sense[0] &= ~SENSE_IR;
        dev->sense[1] |= IsAtLoadPoint( dev ) ? SENSE1_TAPE_LOADPT : 0;
        dev->sense[1] |= dev->readonly || dev->tdparms.logical_readonly ?
            SENSE1_TAPE_FP : 0;
        dev->sense[1] |= SENSE1_TAPE_TUA;
        dev->sense[1] &= ~SENSE1_TAPE_TUB;
    }
    if (dev->tmh->passedeot(dev))
    {
        dev->sense[4] |= 0x40;
    }

} /* end function build_sense_Streaming */
