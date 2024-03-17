/* TAPEDEV.C    (C) Copyright Roger Bowler, 1999-2012                */
/*              Hercules Tape Device Handler                         */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/* Original Author: Roger Bowler                                     */
/* Prime Maintainer: Ivan Warren                                     */
/* Secondary Maintainer: "Fish" (David B. Trout)                     */

/*-------------------------------------------------------------------*/
/* This module contains device handling functions for emulated       */
/* magnetic tape devices for the Hercules ESA/390 emulator.          */
/*-------------------------------------------------------------------*/
/* Messages issued by the TAPEDEV.C module are prefixed HHCTA0nn     */
/* CCW processing functions have been moved to module TAPECCW.C      */
/*                                                                   */
/* Five emulated tape formats are supported:                         */
/*                                                                   */
/* 1. AWSTAPE   This is the format used by the P/390.                */
/*              The entire tape is contained in a single flat file.  */
/*              A tape block consists of one or more block segments. */
/*              Each block segment is preceded by a 6-byte header.   */
/*              Files are separated by tapemarks, which consist      */
/*              of headers with zero block length.                   */
/*              AWSTAPE files are readable and writable.             */
/*                                                                   */
/*              Support for AWSTAPE is in the "AWSTAPE.C" member.    */
/*                                                                   */
/* 2. OMATAPE   This is the Optical Media Attach device format.      */
/*              Each physical file on the tape is represented by     */
/*              a separate flat file.  The collection of files that  */
/*              make up the physical tape is obtained from an ASCII  */
/*              text file called the "tape description file", whose  */
/*              file name is always tapes/xxxxxx.tdf (where xxxxxx   */
/*              is the volume serial number of the tape).            */
/*              Three formats of tape files are supported:           */
/*              * FIXED files contain fixed length EBCDIC blocks     */
/*                with no headers or delimiters. The block length    */
/*                is specified in the TDF file.                      */
/*              * TEXT files contain variable length ASCII blocks    */
/*                delimited by carriage return line feed sequences.  */
/*                The data is translated to EBCDIC by this module.   */
/*              * HEADER files contain variable length blocks of     */
/*                EBCDIC data prefixed by a 16-byte header.          */
/*              The TDF file and all of the tape files must reside   */
/*              reside under the same directory which is normally    */
/*              on CDROM but can be on disk.                         */
/*              OMATAPE files are supported as read-only media.      */
/*                                                                   */
/*              OMATAPE tape Support is in the "OMATAPE.C" member.   */
/*                                                                   */
/* 3. SCSITAPE  This format allows reading and writing of 4mm or     */
/*              8mm DAT tape, 9-track open-reel tape, or 3480-type   */
/*              cartridge on an appropriate SCSI-attached drive.     */
/*              All SCSI tapes are processed using the generalized   */
/*              SCSI tape driver (st.c) which is controlled using    */
/*              the MTIOCxxx set of IOCTL commands.                  */
/*              PROGRAMMING NOTE: the 'tape' portability macros for  */
/*              physical (SCSI) tapes MUST be used for all tape i/o! */
/*                                                                   */
/*              SCSI tape Support is in the "SCSITAPE.C" member.     */
/*                                                                   */
/* 4. HET       This format is based on the AWSTAPE format but has   */
/*              been extended to support compression.  Since the     */
/*              basic file format has remained the same, AWSTAPEs    */
/*              can be read/written using the HET routines.          */
/*                                                                   */
/*              Support for HET is in the "HETTAPE.C" member.        */
/*                                                                   */
/* 5. FAKETAPE  This is the format used by Fundamental Software      */
/*              on their FLEX-ES systems. It it similar to the AWS   */
/*              format. The entire tape is contained in a single     */
/*              flat file. A tape block is preceded by a 12-ASCII-   */
/*              hex-characters header which indicate the size of     */
/*              the previous and next blocks. Files are separated    */
/*              by tapemarks which consist of headers with a zero    */
/*              current block length. FakeTapes are both readable    */
/*              and writable.                                        */
/*                                                                   */
/*              Support for FAKETAPE is in the "FAKETAPE.C" member.  */
/*                                                                   */
/*-------------------------------------------------------------------*/
/*-------------------------------------------------------------------*/
/* Additional credits:                                               */
/*      3480 commands contributed by Jan Jaeger                      */
/*      Sense byte improvements by Jan Jaeger                        */
/*      3480 Read Block ID and Locate CCWs by Brandon Hill           */
/*      Unloaded tape support by Brandon Hill                    v209*/
/*      HET format support by Leland Lucius                      v209*/
/*      JCS - minor changes by John Summerfield                  2003*/
/*      PERFORM SUBSYSTEM FUNCTION / CONTROL ACCESS support by       */
/*      Adrian Trenkwalder (with futher enhancements by Fish)        */
/*      **INCOMPLETE** 3590 support by Fish (David B. Trout)         */
/*-------------------------------------------------------------------*/
/*-------------------------------------------------------------------*/
/* Reference information:                                            */
/* SC53-1200 S/370 and S/390 Optical Media Attach/2 User's Guide     */
/* SC53-1201 S/370 and S/390 Optical Media Attach/2 Technical Ref    */
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

#define _TAPEDEV_C_
#define _HTAPE_DLL_

#include "hercules.h"  /* need Hercules control blocks               */
#include "tapedev.h"   /* Main tape handler header file              */

//#define  ENABLE_TRACING_STMTS   1       // (Fish: DEBUGGING)
//#include "dbgtrace.h"                   // (Fish: DEBUGGING)

/*-------------------------------------------------------------------*/

#if defined(WIN32) && !defined(HDL_USE_LIBTOOL) && !defined(_MSVC_)
  SYSBLK *psysblk;
  #define sysblk (*psysblk)
#endif

/*-------------------------------------------------------------------*/

DEVHND  tape_other_devhnd   =
{
        &tapedev_init_handler,         /* Device Initialization      */
        &tapedev_execute_ccw,          /* Device CCW execute         */
        &tapedev_close_device,         /* Device Close               */
        &tapedev_query_device,         /* Device Query               */
        NULL,                          /* Device Extended Query      */
        NULL,                          /* Device Start channel pgm   */
        NULL,                          /* Device End channel pgm     */
        NULL,                          /* Device Resume channel pgm  */
        NULL,                          /* Device Suspend channel pgm */
        NULL,                          /* Device Halt channel pgm    */
        NULL,                          /* Device Read                */
        NULL,                          /* Device Write               */
        NULL,                          /* Device Query used          */
        NULL,                          /* Device Reserve             */
        NULL,                          /* Device Release             */
        NULL,                          /* Device Attention           */
        TapeImmedOther,                /* Immediate CCW Codes        */
        NULL,                          /* Signal Adapter Input       */
        NULL,                          /* Signal Adapter Output      */
        NULL,                          /* Signal Adapter Sync        */
        NULL,                          /* Signal Adapter Output Mult */
        NULL,                          /* QDIO subsys desc           */
        NULL,                          /* QDIO set subchan ind       */
        NULL,                          /* Hercules suspend           */
        NULL                           /* Hercules resume            */
};

DEVHND  tape_3590_devhnd   =
{
        &tapedev_init_handler,         /* Device Initialization      */
        &tapedev_execute_ccw,          /* Device CCW execute         */
        &tapedev_close_device,         /* Device Close               */
        &tapedev_query_device,         /* Device Query               */
        NULL,                          /* Device Extended Query      */
        NULL,                          /* Device Start channel pgm   */
        NULL,                          /* Device End channel pgm     */
        NULL,                          /* Device Resume channel pgm  */
        NULL,                          /* Device Suspend channel pgm */
        NULL,                          /* Device Halt channel pgm    */
        NULL,                          /* Device Read                */
        NULL,                          /* Device Write               */
        NULL,                          /* Device Query used          */
        NULL,                          /* Device Reserve             */
        NULL,                          /* Device Release             */
        NULL,                          /* Device Attention           */
        TapeImmed3590,                 /* Immediate CCW Codes        */
        NULL,                          /* Signal Adapter Input       */
        NULL,                          /* Signal Adapter Output      */
        NULL,                          /* Signal Adapter Sync        */
        NULL,                          /* Signal Adapter Output Mult */
        NULL,                          /* QDIO subsys desc           */
        NULL,                          /* QDIO set subchan ind       */
        NULL,                          /* Hercules suspend           */
        NULL                           /* Hercules resume            */
};

/*-------------------------------------------------------------------*/
/* Libtool static name colision resolution...                        */
/* Note: lt_dlopen will look for symbol & modulename_LTX_symbol      */
/*-------------------------------------------------------------------*/

#if defined( HDL_USE_LIBTOOL )

  #define  hdl_ddev   hdt3420_LTX_hdl_ddev
  #define  hdl_depc   hdt3420_LTX_hdl_depc
  #define  hdl_reso   hdt3420_LTX_hdl_reso
  #define  hdl_init   hdt3420_LTX_hdl_init
  #define  hdl_fini   hdt3420_LTX_hdl_fini

#endif

/*-------------------------------------------------------------------*/

HDL_DEPENDENCY_SECTION;
{
    HDL_DEPENDENCY ( HERCULES );
    HDL_DEPENDENCY ( DEVBLK   );
    HDL_DEPENDENCY ( SYSBLK   );
}
END_DEPENDENCY_SECTION

/*-------------------------------------------------------------------*/

HDL_DEVICE_SECTION;
{
    HDL_DEVICE ( 3410, tape_other_devhnd );
    HDL_DEVICE ( 3411, tape_other_devhnd );
    HDL_DEVICE ( 3420, tape_other_devhnd );
    HDL_DEVICE ( 3422, tape_other_devhnd );
    HDL_DEVICE ( 3430, tape_other_devhnd );
    HDL_DEVICE ( 3480, tape_other_devhnd );
    HDL_DEVICE ( 3490, tape_other_devhnd );
    HDL_DEVICE ( 3590, tape_3590_devhnd );
    HDL_DEVICE ( 8809, tape_other_devhnd );
    HDL_DEVICE ( 9347, tape_other_devhnd );
    HDL_DEVICE ( 9348, tape_other_devhnd );
}
END_DEVICE_SECTION

/*-------------------------------------------------------------------*/

#if defined(WIN32) && !defined(HDL_USE_LIBTOOL) && !defined(_MSVC_)

  #undef sysblk

  HDL_RESOLVER_SECTION;
  {
    HDL_RESOLVE_SYMPTR ( psysblk, sysblk );
  }
  END_RESOLVER_SECTION

#endif // defined(WIN32) && !defined(HDL_USE_LIBTOOL) && !defined(_MSVC_)

/*-------------------------------------------------------------------*/
/*  (see 'tapedev.h' for layout of TAPEMEDIA_HANDLER structure)      */
/*-------------------------------------------------------------------*/

TAPEMEDIA_HANDLER  tmh_aws  =
{
    &generic_tmhcall,
    &open_awstape,
    &close_awstape,
    &read_awstape,
    &write_awstape,
    &rewind_awstape,
    &bsb_awstape,
    &fsb_awstape,
    &bsf_awstape,
    &fsf_awstape,
    &write_awsmark,
    &sync_awstape,
    &no_operation,              // (DSE)    ZZ FIXME: not coded yet
    &no_operation,              // (ERG)
    &is_tapeloaded_filename,
    &passedeot_awstape,
    &readblkid_virtual,
    &locateblk_virtual
};

/*-------------------------------------------------------------------*/

TAPEMEDIA_HANDLER  tmh_het   =
{
    &generic_tmhcall,
    &open_het,
    &close_het,
    &read_het,
    &write_het,
    &rewind_het,
    &bsb_het,
    &fsb_het,
    &bsf_het,
    &fsf_het,
    &write_hetmark,
    &sync_het,
    &no_operation,              // (DSE)    ZZ FIXME: not coded yet
    &no_operation,              // (ERG)
    &is_tapeloaded_filename,
    &passedeot_het,
    &readblkid_virtual,
    &locateblk_virtual
};

/*-------------------------------------------------------------------*/

TAPEMEDIA_HANDLER  tmh_fake  =
{
    &generic_tmhcall,
    &open_faketape,
    &close_faketape,
    &read_faketape,
    &write_faketape,
    &rewind_faketape,
    &bsb_faketape,
    &fsb_faketape,
    &bsf_faketape,
    &fsf_faketape,
    &write_fakemark,
    &sync_faketape,
    &no_operation,              // (DSE)    ZZ FIXME: not coded yet
    &no_operation,              // (ERG)
    &is_tapeloaded_filename,
    &passedeot_faketape,
    &readblkid_virtual,
    &locateblk_virtual
};

/*-------------------------------------------------------------------*/

TAPEMEDIA_HANDLER  tmh_oma   =
{
    &generic_tmhcall,
    &open_omatape,
    &close_omatape,
    &read_omatape,
    &write_READONLY5,           // WRITE
    &rewind_omatape,
    &bsb_omatape,
    &fsb_omatape,
    &bsf_omatape,
    &fsf_omatape,
    &write_READONLY,            // WTM
    &write_READONLY,            // SYNC
    &write_READONLY,            // DSE
    &write_READONLY,            // ERG
    &is_tapeloaded_filename,
    &return_false1,             // passedeot
    &readblkid_virtual,
    &locateblk_virtual
};

/*-------------------------------------------------------------------*/

#if defined(OPTION_SCSI_TAPE)

TAPEMEDIA_HANDLER  tmh_scsi   =
{
    &generic_tmhcall,
    &open_scsitape,
    &close_scsitape,
    &read_scsitape,
    &write_scsitape,
    &rewind_scsitape,
    &bsb_scsitape,
    &fsb_scsitape,
    &bsf_scsitape,
    &fsf_scsitape,
    &write_scsimark,
    &sync_scsitape,
    &dse_scsitape,
    &erg_scsitape,
    &is_tape_mounted_scsitape,
    &passedeot_scsitape,
    &readblkid_scsitape,
    &locateblk_scsitape
};

#endif /* defined(OPTION_SCSI_TAPE) */

/*-------------------------------------------------------------------*/
/* Device-Type Initialization Table    (DEV/CU MODEL, FEATURES, ETC) */
/*-------------------------------------------------------------------*/
/*
   PROGRAMMING NOTE: the MDR/OBR code (Device Characteristics bytes
   40-41) are apparently CRITICALLY IMPORTANT for proper tape drive
   functioning for certain operating systems. If the bytes are not
   provided (set to zero) or are set incorrectly, certain operating
   systems end up using unusual/undesirable Mode Set values in their
   Channel Programs (such as x'20' Write Immediate for example). I
   only note it here because these two particular bytes are rather
   innocuous looking based upon their name and sparsely documented
   and largely unexplained values, thereby possibly misleading one
   into believing they weren't important and thus could be safely
   set to zero if their values were unknown. Rest assured they are
   NOT unimportant! Quite the opposite: the are, for some operating
   systems, CRITICALLY IMPORTANT and must NOT be returned as zeros.

   The following were obtained from "EREP Release 3.5.0 Reference"
   (GC35-0152-03):

       Model                        MDR          OBR
      -------                      -----        -----

       3480                         0x41         0x80
       3490                         0x42         0x81

       3590                         0x46         0x83
       3590 (3591/3490 EMU)         0x47         0x84
       3590 (3590/3490 EMU)         0x48         0x85


   NOTE: only models 3480, 3490 and 3590 support the RDC (Read
   Device Characteristics) channel command, and thus they're the
   only ones we must know the MDR/OBR codes for (since the MDR/OBR
   codes are only used in the RDC CCW and not anwhere else). That
   is to say, NONE of the Channel Commands (CCWs) that all the
   OTHER models happen to support have an MDR/OBR code anywhere
   in their data. Only models 3480, 3490 and 3590 have MDR/OBR
   codes buried in their CCW data (specifically the RDC CCW data).

   Also note that, at the moment, we do not support emulating 3590's
   or 3591's running in 3490 Emulation Mode (i.e. 3591/3490 EMU or
   3590/3490 EMU). The user is free to use such a device with Herc-
   ules however, but if they do, it should be specified as a 3490.

---------------------------------------------------------------------*/

typedef struct DEVINITTAB               /* Initialization values     */
{
    U16         devtype;                /* Device type               */
    BYTE        devmodel;               /* Device model number       */
    U16         cutype;                 /* Control unit type         */
    BYTE        cumodel;                /* Control unit model number */
    U32         feats1;                 /* Storage control features  */
    U32         feats2;                 /* Storage control features  */
    U32         maxblk;                 /* Maximum supported blksize */
    U32         recblk;                 /* Recommended blksize       */
    BYTE        MDR;                    /* Misc. Data Record ID      */
    BYTE        OBR;                    /* Outboard Recorder ID      */
    BYTE        mvscode;                /* MVS Dev Code/Class/Type   */
    BYTE        devclass;               /* Device class code         */
    BYTE        devtcode;               /* Device type code          */
    int         numdevid;               /* #of SNSID bytes (see NOTE)*/
    int         numsense;               /* #of SENSE bytes           */
    int         haverdc;                /* RDC Supported             */
    int         displayfeat;            /* Has LCD display           */
}
DEVINITTAB;
DEVINITTAB      DevInitTab[]  =         /* Initialization table      */
{
// PROGRAMMING NOTE: we currently do not support a #of Sense-ID bytes
// value (numdevid) greater than 7 since our current channel-subsystem
// design does not support the concept of hardware physical attachment
// "Nodes" (i.e. separate control-unit, device and interface elements).
// Supporting more than 7 bytes of Sense-ID information would require
// support for Node Descriptors (ND) and Node Element Descriptors (NED)
// and the associated commands (CCWs) to query them (Read Configuration
// Data (0xFA) , Set Interface Identifier (0x73) and associated support
// in the Read Subsystem Data (0x3E) command), which is vast overkill
// and a complete waste of time given our current overly-simple channel
// subsystem design.
//
//--------------------------------------------------------------------
//            3410/3411/3420/3422/3430/8809/9347/9348
//--------------------------------------------------------------------
//
// devtype/mod  cutype/mod     feats1      feats2   maxblk recblk MDR OBR mvs cls typ sid sns rdc dsp
 { 0x3410,0x01, 0x3115,0x01, 0x00000000, 0x00000000, BLK64, BLK64, 0,  0,  0,  0,  0,  0,  9,  0,  0 },
 { 0x3411,0x01, 0x3115,0x01, 0x00000000, 0x00000000, BLK64, BLK64, 0,  0,  0,  0,  0,  0,  9,  0,  0 },
 { 0x3420,0x06, 0x3803,0x02, 0x00000000, 0x00000000, BLK64, BLK64, 0,  0,  0,  0,  0,  0, 24,  0,  0 }, // (DEFAULT: 3420)
 { 0x3422,0x01, 0x3422,0x01, 0x00000000, 0x00000000, BLK64, BLK64, 0,  0,  0,  0,  0,  7, 32,  0,  0 },
 { 0x3430,0x01, 0x3422,0x01, 0x00000000, 0x00000000, BLK64, BLK64, 0,  0,  0,  0,  0,  7, 32,  0,  0 },
 { 0x8809,0x01, 0x8809,0x01, 0x00000000, 0x00000000, BLK64, BLK64, 0,  0,  0,  0,  0,  0, 32,  0,  0 },
 { 0x9347,0x01, 0x9347,0x01, 0x00000000, 0x00000000, BLK64, BLK64, 0,  0,  0,  0,  0,  7, 32,  0,  0 },
 { 0x9348,0x01, 0x9348,0x01, 0x00000000, 0x00000000, BLK64, BLK64, 0,  0,  0,  0,  0,  7, 32,  0,  0 },

//--------------------------------------------------------------------
//                          3480/3490/3590
//--------------------------------------------------------------------
//
// PROGRAMMING NOTE: we currently do not support a #of Sense-ID bytes
// value (numdevid) greater than 7 since our current channel-subsystem
// design does not support the concept of hardware physical attachment
// "Nodes" (i.e. separate control-unit, device and interface elements).
// Supporting more than 7 bytes of Sense-ID information would require
// support for Node Descriptors (ND) and Node Element Descriptors (NED)
// and the associated commands (CCWs) to query them (Read Configuration
// Data (0xFA) , Set Interface Identifier (0x73) and associated support
// in the Read Subsystem Data (0x3E) command), which is vast overkill
// and a complete waste of time given our current overly-simple channel
// subsystem design.
//
// PROGRAMMING NOTE: if you change the below devtype/mod or cutype/mod
// values, be sure to ALSO change the same corresponding values in the
// tapeccws.c's READ CONFIGURATION DATA (CCW opcode 0xFA) logic too!
// (i.e. the "tape_read_configuration_data()" function further below)
//
// For the layout of the 'feats1' and 'feats2' fields, please refer to
// the 'TAPERDC' struct defined in the tapedev.h header file.
//
// devtype/mod  cutype/mod     feats1      feats2    maxblk  recblk  MDR  OBR   mvs   cls  typ  sid sns rdc dsp
 { 0x3480,0x31, 0x3480,0x31, FEAT1_3480, FEAT2_3480,  BLK64,  BLK64, M48, O48, 0x00, 0x80,0x80,  7, 24,  1,  1 },  // 0x31=D31
 { 0x3490,0x50, 0x3490,0x50, FEAT1_3490, FEAT2_3490,  BLK64,  BLK64, M49, O49, 0x00, 0x80,0x80,  7, 32,  1,  1 },  // 0x50=C10
 { 0x3590,0x60, 0x3590,0x11, FEAT1_3590, FEAT2_3590, BLK256, BLK256, M59, O59, 0x80, 0x00,0x00,  7, 32,  1,  1 },  // 0x60=A50, 0x11=B1A
 { 0xFFFF,0xFF, 0xFFFF,0xFF, 0xFFFFFFFF, 0xFFFFFFFF,   -1,     -1,  0xFF,0xFF, 0xFF, 0xFF,0xFF, -1, -1, -1, -1 },  //**** END OF TABLE ****
 { 0x3420,0x06, 0x3803,0x02, 0x00000000, 0x00000000,  BLK64,  BLK64,  0,   0,    0,    0,   0,   0, 24,  0,  0 },  // (DEFAULT: 3420)
};

/*-------------------------------------------------------------------*/
/* static template/prototype configuration data                      */
/*-------------------------------------------------------------------*/
/*                                                                   */
/*             GA32-0127 IBM 3490E Hardware Reference                */
/*                                                                   */
/* Read Configuration Data (X'FA')                                   */
/*                                                                   */
/* A Read Configuration Data command causes 160 bytes of data to     */
/* be transferred from the control unit to the channel.  The data    */
/* transferred by this command is referred to as a configuration     */
/* record and is associated with the addressed device-path pair.     */
/* The configuration record from each device-path pair provides the  */
/* host with identifiers of node elements internal to the subsystem. */
/*                                                                   */
/*-------------------------------------------------------------------*/
static const BYTE cfgdata[] =       // (prototype data)
{
// ---------------- Device NED ---------------------------------------------------
0xCC,                               // 0:      NED code
0x01,                               // 1:      Type  (X'01' = I/O Device)
0x02,                               // 2:      Class (X'02' = Magnetic Tape)
0x00,                               // 3:      (Reserved)
0xF0,0xF0,0xF3,0xF4,0xF9,0xF0,      // 4-9:    Type  ('003490')
0xC3,0xF1,0xF0,                     // 10-12:  Model ('C10')
0xC8,0xD9,0xC3,                     // 13-15:  Manufacturer ('HRC' = Hercules)
0xE9,0xE9,                          // 16-17:  Plant of Manufacture ('ZZ' = Herc)
0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,      // 18-29:  Sequence Number
0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,      //
0x00, 0x00,                         // 30-31: Tag (x'000n', n = Logical Drive Address)
// ---------------- Control Unit NED ---------------------------------------------
0xC4,                               // 32:     NED code
0x02,                               // 33:     Type  (X'02' = Control Unit)
0x00,                               // 34:     Class (X'00' = Undefined)
0x00,                               // 35:     (Reserved)
0xF0,0xF0,0xF3,0xF4,0xF9,0xF0,      // 36-41:  Type  ('003490')
0xC3,0xF1,0xF0,                     // 42-44:  Model ('C10')
0xC8,0xD9,0xC3,                     // 45-47:  Manufacturer ('HRC' = Hercules)
0xE9,0xE9,                          // 48-49:  Plant of Manufacture ('ZZ' = Herc)
0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,      // 50-61:  Sequence Number
0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,      //
0x00, 0x00,                         // 62-63:  Tag (x'0000')
// ---------------- Library NED --------------------------------------------------
0x00,                               // 64:     NED code   (x'00' = Not Used)
0x00,                               // 65:     Type
0x00,                               // 66:     Class
0x00,                               // 67:     (Reserved)
0x00,0x00,0x00,0x00,0x00,0x00,      // 68-73:  Type
0x00,0x00,0x00,                     // 74-76:  Model
0x00,0x00,0x00,                     // 77-79:  Manufacturer
0x00,0x00,                          // 80-81:  Plant of Manufacture
0x00,0x00,0x00,0x00,0x00,0x00,      // 82-93:  Sequence Number
0x00,0x00,0x00,0x00,0x00,0x00,      //
0x00, 0x00,                         // 94-95:  Tag
// ---------------- Token NED ---------------------------------------------------
0xEC,                               // 96:       NED code
0x00,                               // 97:       Type  (X'00' = Unspecified)
0x00,                               // 98:       Class (X'00' = Undefined)
0x00,                               // 99:       (Reserved)
0xF0,0xF0,0xF3,0xF4,0xF9,0xF0,      // 100-105:  Type  ('003490')
0xC3,0xF1,0xF0,                     // 106-108:  Model ('C10')
0xC8,0xD9,0xC3,                     // 109-111:  Manufacturer ('HRC' = Hercules)
0xE9,0xE9,                          // 112-113:  Plant of Manufacture ('ZZ' = Herc)
0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,      // 114-125:  Sequence Number
0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,      //
0x00, 0x00,                         // 126-127:  Tag (x'0000')
// ---------------- General NEQ --------------------------------------------------
0x80,                               // 128:      NED code
0x80,                               // 129:      Record Selector:
                                    //           x'80' = Control Unit 0
                                    //           x'81' = Control Unit 1
0x00,0x80,                          // 130-131:  Interface Id:
                                    //           x'0080' = CU Channel Adapter A
                                    //           x'0040' = CU Channel Adapter B
0x00,                               // 132:      Device-Dependent Timeout
0x00,0x00,0x00,                     // 133-135:  (Reserved)
0x00,                               // 136:      Extended Information:
                                    //           x'00' for Logical Drive Addresses 0-7
                                    //           x'01' for Logical Drive Addresses 8-F
0x00,0x00,0x00,0x00,0x00,0x00,0x00, // 137-159:  (Reserved)
0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,
};

CASSERT( sizeof(cfgdata) == 160, tapedev_c );

/*-------------------------------------------------------------------*/
/* Read Configuration Data function                                  */
/*-------------------------------------------------------------------*/
static int tape_read_configuration_data( DEVBLK* dev, BYTE* buffer, int bufsz )
{
    int   copylen;
    BYTE  work[ sizeof( cfgdata ) ];

    /* Copy prototype Configuration Data to work area */
    memcpy( work, cfgdata, sizeof( work ));

    /* Fixup values for this particular device/type...  NOTE: we
       only fixup the Device and Control Unit NEDs here. The Token
       NED's type/model values come from the Device NED's values.
    */
    if (0x3480 == dev->devtype)
    {
        memcpy (&work[7],  "\xF4\xF8",     2);      // '48'
        memcpy (&work[39], "\xF4\xF8",     2);      // '48'

        memcpy (&work[10], "\xC4\xF3\xF1", 3);      // 'D31'
        memcpy (&work[42], "\xC4\xF3\xF1", 3);      // 'D31'
    }
    else if (0x3490 == dev->devtype)
    {
//      memcpy (&work[7],  "\xF4\xF9",     2);      // '49'
//      memcpy (&work[39], "\xF4\xF9",     2);      // '49'

//      memcpy (&work[10], "\xC3\xF1\xF0", 3);      // 'C10'
//      memcpy (&work[42], "\xC3\xF1\xF0", 3);      // 'C10'
    }
    else if (0x3590 == dev->devtype)
    {
        memcpy (&work[7],  "\xF5\xF9",     2);      // '59'
        memcpy (&work[39], "\xF5\xF9",     2);      // '59'

        memcpy (&work[10], "\xC2\xF1\xC1", 3);      // 'B1A'
        memcpy (&work[42], "\xC1\xF5\xF0", 3);      // 'A50'
    }

    memcpy (&work[100], &work[4], 9);       // (set Token NED Type/Model from Device NED)

    work[31] |= (dev->devnum & 0x0F);       // (set Logical Drive Address)

    if ((dev->devnum & 0x0F) > 7)
        work[136] = 0x01;                   // (set Extended Information)

    /* Finally, copy the work area into the caller's buffer */
    copylen = bufsz < (int) sizeof( work ) ? bufsz : (int) sizeof( work );
    memcpy( buffer, work, copylen );

    /* Return to them the number of bytes we provided */
    return copylen;
}

/*-------------------------------------------------------------------*/
/* Initialize the device handler                                     */
/* NOTE: always called with dev->lock held                           */
/*-------------------------------------------------------------------*/
static int tapedev_init_handler (DEVBLK *dev, int argc, char *argv[])
{
int             rc;
DEVINITTAB*     pDevInitTab;
TAPERDC*        rdc = (TAPERDC*) dev->devchar;

    dev->rcd = &tape_read_configuration_data;

    /* Close current tape */
    if (dev->fd >= 0)
    {
        /* Prevent accidental re-init'ing of already loaded tape drives */
        if (sysblk.nomountedtapereinit)
        {
            char*  devclass;

            tapedev_query_device( dev, &devclass, 0, NULL );

            if (1
                && strcmp( devclass,"TAPE" ) == 0
                && (0
                    || TAPEDEVT_SCSITAPE == dev->tapedevt
                    || (argc >= 3 && strcmp( argv[2], TAPE_UNLOADED ) != 0)
                   )
            )
            {
                ASSERT( dev->tmh && dev->tmh->tapeloaded );
                if (dev->tmh->tapeloaded( dev, NULL, 0 ))
                {
                    // "%1d:%04X reinit rejected; drive not empty"
                    WRMSG( HHC02243, "E", LCSS_DEVNUM );
                    return -1;
                }
            }
        }

        dev->tmh->close( dev );
        dev->fd = -1;
    }

    autoload_close( dev );
    dev->tdparms.displayfeat = 0;

    /* reset excps count */
    dev->excps = 0;

    /* Determine the control unit type and model number */
    /* Support for 3490/3422/3430/8809/9347, etc.. */
    if (!sscanf( dev->typname, "%hx", &dev->devtype ))
        dev->devtype = 0x3420;

    // PROGAMMING NOTE: we use hard-coded values from our DevInitTab
    // for virtual (non-SCSI) devices and, for the time being, for non-
    // virtual (SCSI) devices too. Once we add direct SCSI I/O support
    // we will need to add code to get this information directly from
    // the actual SCSI device itself.
    for
    (
        pDevInitTab = &DevInitTab[0];
        pDevInitTab->devtype != 0xFFFF && pDevInitTab->devtype != dev->devtype;
        pDevInitTab++
    );

    if (pDevInitTab->devtype == 0xFFFF)         /* (entry not found?) */
    {
        // "Unsupported tape device type '%04X' specified"
        WRMSG(HHC00225, "E", dev->devtype );

        pDevInitTab++;                          /* (default entry; s/b same as 0x3420) */
        pDevInitTab->devtype = dev->devtype;    /* (don't know what else to do really) */
        pDevInitTab->cutype  = dev->devtype;    /* (don't know what else to do really) */
    }

    /* Allow SENSE ID for certain specific legacy devices if requested */

    dev->numdevid = pDevInitTab->numdevid;  // (default == from table)

    if (1
        && sysblk.legacysenseid             // (if option requested, AND is)
        && (0                               // (for allowable legacy device)
            || 0x3410 == dev->devtype
            || 0x3411 == dev->devtype
            || 0x3420 == dev->devtype
            || 0x8809 == dev->devtype
           )
    )
    {
        dev->numdevid = 7;      // (allow only this much for legacy devices)
    }

    /* Initialize the Sense-Id bytes if needed... */
    if (dev->numdevid > 0)
    {
                   dev->devid[0] = 0xFF;
        STORE_HW( &dev->devid[1],  pDevInitTab->cutype  );
                   dev->devid[3] = pDevInitTab->cumodel;
        STORE_HW( &dev->devid[4],  pDevInitTab->devtype );
                   dev->devid[6] = pDevInitTab->devmodel;

        /* Initialize the CIW information if needed... */
        if (dev->numdevid > 7)
        {
            // PROGRAMMING NOTE: see note near 'DEVINITTAB'
            // struct definition regarding requirements for
            // supporting more than 7 bytes of SNSID info.

            memcpy( &dev->devid[8],  "\x40\xFA\x00\xA0", 4 ); // CIW Read Configuration Data  (0xFA)
            memcpy( &dev->devid[12], "\x41\x73\x00\x04", 4 ); // CIW Set Interface Identifier (0x73)
            memcpy( &dev->devid[16], "\x42\x3E\x00\x60", 4 ); // CIW Read Subsystem Data      (0x3E)
        }
    }

    /* Initialize the Read Device Characteristics (RDC) bytes... */
    if (pDevInitTab->haverdc)
    {
        dev->numdevchar = sizeof( dev->devchar );
        memset( dev->devchar, 0, dev->numdevchar );

        /* Initialize feature bytes and other values */
        STORE_FW( rdc->feats1,    pDevInitTab->feats1 );
        STORE_FW( rdc->feats2,    pDevInitTab->feats2 );
        STORE_HW( rdc->cutype,    pDevInitTab->cutype  );
        STORE_HW( rdc->ncutype,   pDevInitTab->cutype  );
        STORE_HW( rdc->tutype,    pDevInitTab->devtype );
        STORE_HW( rdc->ntutype,   pDevInitTab->devtype );
                  rdc->cumodel  = pDevInitTab->cumodel;
                  rdc->ncumodel = pDevInitTab->cumodel;
                  rdc->tumodel  = pDevInitTab->devmodel;
                  rdc->ntumodel = pDevInitTab->devmodel;
                  rdc->devclass = pDevInitTab->devclass;
                  rdc->devtcode = pDevInitTab->devtcode;
                  rdc->mdr      = pDevInitTab->MDR;
                  rdc->obr      = pDevInitTab->OBR;
                  rdc->mvscode  = pDevInitTab->mvscode;

        /* Set maximum blocksize and default blocksize values */
        STORE_FW( rdc->maxblk, pDevInitTab->maxblk );
        STORE_FW( rdc->recblk, pDevInitTab->recblk );
    }

    /* Initialize other fields */
//  dev->numdevid            = pDevInitTab->numdevid;   // (we already did this)
    dev->numsense            = pDevInitTab->numsense;
    dev->tdparms.displayfeat = pDevInitTab->displayfeat;
    dev->fenced              = 0;   // (always, initially)
    dev->SIC_active          = 0;   // (always, initially)
    dev->forced_logging      = 0;   // (always, initially)
    dev->noautomount         = 0;   // (always, initially)

    /* Clear the DPA */
    memset (dev->pgid, 0, sizeof(dev->pgid));
    /* Clear Drive password - Adrian */
    memset (dev->drvpwd, 0, sizeof(dev->drvpwd));

    /* Request the channel to merge data chained write CCWs into
       a single buffer before passing data to the device handler */
    dev->cdwmerge = 1;

    /* Request a maximum sized device I/O buffer */
    dev->bufsize = MAX_TAPE_BLKSIZE;

    /* ISW */
    /* Build a 'clear' sense */
    memset (dev->sense, 0, sizeof(dev->sense));
    dev->sns_pending = 0;

    // Initialize the [non-SCSI] auto-loader...

    //-----------------------------------------------------------------
    //                IMPORTANT PROGRAMMING NOTE!
    //-----------------------------------------------------------------
    //    "dev->tapedevt" is still uninitialized at this point!
    //-----------------------------------------------------------------
    // We don't know yet what type of tape device we're dealing with
    // (SCSI (non-virtual) or non-SCSI (virtual)) since 'mountnewtape'
    // hasn't been called yet (which determines the media handler to
    // be used and initializes dev->tapedevt).  The only thing we know
    // (or WILL know once 'autoload_init' is called) is whether or not
    // there was a [non-SCSI] auto-loader defined for the device.
    //-----------------------------------------------------------------

    autoload_init( dev, argc, argv );

    // Was an auto-loader defined for this device?
    if ( !dev->als )
    {
        // No. Just mount whatever tape there is (if any)...
        rc = mountnewtape( dev, argc, argv );
    }
    else
    {
        // Yes. Try mounting the FIRST auto-loader slot...
        if ( (rc = autoload_mount_first( dev )) != 0 )
        {
            // If that doesn't work, try subsequent slots...
            while
            (
                dev->als
                &&
                (rc = autoload_mount_next( dev )) != 0
            )
            {
                ;  // (nop; just go on to next slot)
            }
            rc = dev->als ? rc : -1;
        }
    }

#if defined(OPTION_SCSI_TAPE)
    /* Initialize SCSI tape status field (must not do
       this until AFTER mountnewtape has been called
       since dev->stape_online is otherwise undefined) */
    dev->sstat = dev->stape_online ? 0 : GMT_DR_OPEN( -1 );
#endif

    /* Request a maximum sized device I/O buffer */
    dev->bufsize = MAX_TAPE_BLKSIZE;

    /* Make attention pending if necessary */
    if (!dev->reinit || rc < 0)
    {
        TRACE( "+++ NOT calling device_attention( 0x%04.4X, CSW_DE )\n", dev->devnum );
    }
    else // (dev->reinit && rc >= 0)
    {
        TRACE( "+++ CALLING device_attention( 0x%04.4X, CSW_DE )\n", dev->devnum );

        RELEASE_DEVLOCK( dev );
        {
            rc = device_attention( dev, CSW_DE );
        }
        OBTAIN_DEVLOCK( dev );
    }

    return rc;

} /* end function tapedev_init_handler */


/*-------------------------------------------------------------------*/
/* Close the device                                                  */
/*-------------------------------------------------------------------*/
static int tapedev_close_device ( DEVBLK *dev )
{
    autoload_close(dev);
    dev->tmh->close(dev);
    ASSERT( dev->fd < 0 );

    dev->curfilen  = 1;
    dev->nxtblkpos = 0;
    dev->prvblkpos = -1;
    dev->curblkrem = 0;
    dev->curbufoff = 0;
    dev->blockid   = 0;
    dev->fenced = 0;

    return 0;
} /* end function tapedev_close_device */


/*-------------------------------------------------------------------*/
/*  Tape format determination REGEXPS. Used by gettapetype below     */
/*-------------------------------------------------------------------*/

struct tape_format_entry                    /*   (table layout)      */
{
    char*               fmtreg;             /* A regular expression  */
    int                 fmtcode;            /* the device code       */
    TAPEMEDIA_HANDLER*  tmh;                /* The media dispatcher  */
    char*               descr;              /* readable description  */
    char*               short_descr;        /* (same but shorter)    */
};

/*-------------------------------------------------------------------*/
/*  Tape format determination REGEXPS. Used by gettapetype below     */
/*-------------------------------------------------------------------*/

struct  tape_format_entry   fmttab   [] =   /*    (table itself)     */
{
    /* This entry matches a filename ending with .aws    */
#define  AWSTAPE_FMTENTRY   0
#define  DEFAULT_FMTENTRY   AWSTAPE_FMTENTRY
    {
        "\\.aws$",
        TAPEDEVT_AWSTAPE,
        &tmh_aws,
        "AWS Format tape file",
        "AWS tape"
    },

    /* This entry matches a filename ending with .het    */
#define  HETTAPE_FMTENTRY   1
    {
        "\\.het$",
        TAPEDEVT_HETTAPE,
        &tmh_het,
        "Hercules Emulated Tape file",
        "HET tape"
    },

    /* This entry matches a filename ending with .tdf    */
#define  OMATAPE_FMTENTRY   2
    {
        "\\.tdf$",
        TAPEDEVT_OMATAPE,
        &tmh_oma,
        "Optical Media Attachment (OMA) tape",
        "OMA tape"
    },

    /* This entry matches a filename ending with .fkt    */
#define  FAKETAPE_FMTENTRY  3
    {
        "\\.fkt$",
        TAPEDEVT_FAKETAPE,
        &tmh_fake,
        "Flex FakeTape file",
        "FakeTape"
    },

#if defined(OPTION_SCSI_TAPE)

    /* This entry matches a filename starting with /dev/ */
#define  SCSITAPE_FMTENTRY  4
    {
        "^/dev/",
        TAPEDEVT_SCSITAPE,
        &tmh_scsi,
        "SCSI attached tape drive",
        "SCSI tape"
    },

#if defined(_MSVC_)

    /* (same idea but for Windows SCSI tape device names) */
#undef   SCSITAPE_FMTENTRY
#define  SCSITAPE_FMTENTRY  5
    {
        "^((\\\\\\\\\\.\\\\)|(//\\./))\\w",
        TAPEDEVT_SCSITAPE,
        &tmh_scsi,
        "SCSI attached tape drive",
        "SCSI tape"
    },

#endif // _MSVC_
#endif // OPTION_SCSI_TAPE
};


/*-------------------------------------------------------------------*/
/*  gettapetype_byname        determine tape device type by filename */
/*-------------------------------------------------------------------*/
/* returns fmttab entry# on success, -1 on error/unable to determine */
/*-------------------------------------------------------------------*/
int gettapetype_byname (DEVBLK *dev)
{
#if defined(HAVE_REGEX_H) || defined(HAVE_PCRE)
    regex_t     regwrk;                 /* REGEXP work area          */
    regmatch_t  regwrk2;                /* REGEXP match area         */
    char        errbfr[1024];           /* Working storage           */
    int         i;                      /* Loop control              */
#endif // HAVE_REGEX_H
    int         rc;                     /* various rtns return codes */

    /* Use the file name to determine the device type */

#if defined(HAVE_REGEX_H) || defined(HAVE_PCRE)

    for (i=0; i < (int) _countof( fmttab ); i++)
    {
        rc = regcomp (&regwrk, fmttab[i].fmtreg, REG_ICASE);
        if (rc < 0)
        {
            regerror (rc, &regwrk, errbfr, 1024);
            // "%1d:%04X Tape file '%s', type '%s': error in function '%s': '%s'"
            WRMSG(HHC00205, "E", LCSS_DEVNUM, dev->filename, TTYPSTR(dev->tapedevt), "regcomp()", errbfr);
            return -1;
        }

        rc = regexec (&regwrk, dev->filename, 1, &regwrk2, 0);
        if (rc < 0)
        {
            regerror (rc, &regwrk, errbfr, 1024);
            regfree ( &regwrk );
            // "%1d:%04X Tape file '%s', type '%s': error in function '%s': '%s'"
            WRMSG(HHC00205, "E", LCSS_DEVNUM, dev->filename, TTYPSTR(dev->tapedevt), "regexec()", errbfr);
            return -1;
        }

        regfree (&regwrk);

        if (rc == 0)  /* MATCH? */
            return i;

        ASSERT( rc == REG_NOMATCH );
    }

#else // !HAVE_REGEX_H

    if (1
        && (rc = strlen(dev->filename)) > 4
        && (rc = strcasecmp( &dev->filename[rc-4], ".aws" )) == 0
    )
    {
        return AWSTAPE_FMTENTRY;
    }

    if (1
        && (rc = strlen(dev->filename)) > 4
        && (rc = strcasecmp( &dev->filename[rc-4], ".het" )) == 0
    )
    {
        return HETTAPE_FMTENTRY;
    }

    if (1
        && (rc = strlen(dev->filename)) > 4
        && (rc = strcasecmp( &dev->filename[rc-4], ".tdf" )) == 0
    )
    {
        return OMATAPE_FMTENTRY;
    }

    if (1
        && (rc = strlen(dev->filename)) > 4
        && (rc = strcasecmp( &dev->filename[rc-4], ".fkt" )) == 0
    )
    {
        return FAKETAPE_FMTENTRY;
    }

#if defined(OPTION_SCSI_TAPE)
    if (1
        && (rc = strlen(dev->filename)) > 5
        && (rc = strncasecmp( dev->filename, "/dev/", 5 )) == 0
    )
    {
        if (strncasecmp( dev->filename+5, "st", 2 ) == 0)
            dev->stape_close_rewinds = 1; // (rewind at close)
        else
            dev->stape_close_rewinds = 0; // (otherwise don't)

        return SCSITAPE_FMTENTRY;
    }
#if defined(_MSVC_)
    if (1
        && strncasecmp(dev->filename, "\\\\.\\", 4) == 0
        &&           *(dev->filename        +    4) != 0
    )
    {
        return SCSITAPE_FMTENTRY;
    }
#endif // _MSVC_
#endif // OPTION_SCSI_TAPE
#endif // HAVE_REGEX_H

    return -1;      /* -1 == "unable to determine" */

} /* end function gettapetype_byname */


/*-------------------------------------------------------------------*/
/*  gettapetype_bydata       determine tape device type by file data */
/*-------------------------------------------------------------------*/
/* returns fmttab entry# on success, -1 on error/unable to determine */
/*-------------------------------------------------------------------*/
int gettapetype_bydata (DEVBLK *dev)
{
    char        pathname[MAX_PATH];     /* file path in host format  */
    int         rc;                     /* various rtns return codes */

    /* Try to determine the type based on actual file contents */
    hostpath( pathname, dev->filename, sizeof(pathname) );
    rc = HOPEN ( pathname, O_RDONLY | O_BINARY );
    if (rc >= 0)
    {
        BYTE hdr[6];                    /* block header i/o buffer   */
        int fd = rc;                    /* save file descriptor      */

        /* Read the header. If bytes 0-3 are ASCII "0000", then the
         * tape is likely a Flex FakeTape. Otherwise if bytes 2-3 are
         * binary zero (x'0000'), it's likely an AWS type tape. If byte
         * 4 (first flag byte) has either of the ZLIB or BZIP2 flags on,
         * then it's a HET tape. Otherwise it's just an ordinary AWS tape.
         */
        rc = read (fd, hdr, sizeof(hdr));
             close(fd);
        if (rc >= 6)
        {
            /* Use the data to make the possible determination */
            if (memcmp(hdr, "@TDF", 4) == 0)
                return OMATAPE_FMTENTRY;

            if (1
                && hdr[0] == 0x30           /* "ASCII"-zero len prev block? */
                && hdr[1] == 0x30
                && hdr[2] == 0x30
                && hdr[3] == 0x30
            )
                return FAKETAPE_FMTENTRY;   /* Then obviously Flex FakeTape */

            if (hdr[2] == 0 && hdr[3] == 0)             /* 0 len prev blk?  */
            {
                if (hdr[4] & HETHDR_FLAGS1_TAPEMARK)    /* If tapemark then */
                    return -1;                          /* can't tell type. */
                if (hdr[4] & HETHDR_FLAGS1_COMPRESS ||  /* ZLIB or BZIP2 or */
                    hdr[5] & HETHDR_FLAGS2_COMPRESS)    /* Bus-Tech ZLIB?   */
                    return HETTAPE_FMTENTRY;            /* Then HET format. */
                else
                    return AWSTAPE_FMTENTRY;            /* Else default AWS */
            }
        }
    }
    return -1;      /* -1 == "unable to determine" */

} /* end function gettapetype_bydata */


/*-------------------------------------------------------------------*/
/*  gettapetype              determine tape device type              */
/*-------------------------------------------------------------------*/
/* returns fmttab entry# on success, -1 on error/unable to determine */
/*-------------------------------------------------------------------*/
int gettapetype (DEVBLK *dev, char **short_descr)
{
    char*       descr;                  /* Device descr from fmttab  */
    int         i;                      /* fmttab entry#             */

    i = gettapetype_byname( dev );      /* Get type based on name    */

#if defined(OPTION_SCSI_TAPE)
    if (i != SCSITAPE_FMTENTRY)         /* If not SCSI tape...       */
#endif
    {
        if ( !strcasecmp( dev->filename, TAPE_UNLOADED ) )
        {
            i = DEFAULT_FMTENTRY;
        }
        else
        {
            int i2 = gettapetype_bydata( dev ); // Get type based on data..

            if (i2 >= 0 &&                      // If valid type by data, AND
                ( i2 != AWSTAPE_FMTENTRY ||     // *not* AWS by data (or if it
                  i  != HETTAPE_FMTENTRY )      // is, if it's not HET by name)..
               )
                i = i2;                         // ..Use type based on data.
        }
    }

    /* If file type still unknown, use a reasonable default value... */
    if (i < 0)
    {
        i = DEFAULT_FMTENTRY;
        if (strcmp (dev->filename, TAPE_UNLOADED) != 0)
        {
            // "%1d:%04X Tape file '%s', type '%s': format type is not determinable, presumed '%s'"
            WRMSG(HHC00220, "W", LCSS_DEVNUM, dev->filename, TTYPSTR(dev->tapedevt), fmttab[i].short_descr );
        }
    }

    dev->tapedevt = fmttab[i].fmtcode;
    dev->tmh      = fmttab[i].tmh;
    descr         = fmttab[i].descr;
    *short_descr  = fmttab[i].short_descr;

    if (strcmp (dev->filename, TAPE_UNLOADED) != 0)
    {
        // "%1d:%04X Tape file '%s', type '%s': format type '%s'"
        WRMSG(HHC00221, "I", LCSS_DEVNUM, dev->filename, TTYPSTR(dev->tapedevt), descr);
    }

    return 0;   // (success)

} /* end function gettapetype */


/*-------------------------------------------------------------------*/
/*  The following table goes hand-in-hand with the 'enum' values     */
/*  that immediately follow.  Used by 'mountnewtape' function.       */
/*-------------------------------------------------------------------*/

PARSER  ptab  [] =
{
    { "-d",         NULL },
    { "awstape",    NULL },
    { "idrc",       "%d" },
    { "compress",   "%d" },
    { "method",     "%d" },
    { "level",      "%d" },
    { "chunksize",  "%d" },
    { "maxsize",    PARSER_STR_TYPE },
    { "maxsizeK",   "%d" },
    { "maxsizeM",   "%d" },
    { "eotmargin",  PARSER_STR_TYPE },
    { "strictsize", "%d" },
    { "readonly",   "%d" },
    { "ro",         NULL },
    { "noring",     NULL },
    { "rw",         NULL },
    { "ring",       NULL },
    { "deonirq",    "%d" },
    { "noautomount",NULL },
    { "--blkid-22", NULL },
    { "--blkid-24", NULL },   /* (synonym for --blkid-22) */
    { "--blkid-32", NULL },
    { "--no-erg",   NULL },
    { "--online",   NULL },
    { NULL,         NULL },   /* (end of table) */
};

/*-------------------------------------------------------------------*/
/*  The following table goes hand-in-hand with the 'ptab' PARSER     */
/*  table immediately above. Used by 'mountnewtape' function.        */
/*-------------------------------------------------------------------*/

enum
{
    TDPARM_NONE,
    TDPARM_DEBUG,
    TDPARM_AWSTAPE,
    TDPARM_IDRC,
    TDPARM_COMPRESS,
    TDPARM_METHOD,
    TDPARM_LEVEL,
    TDPARM_CHKSIZE,
    TDPARM_MAXSIZE,
    TDPARM_MAXSIZEK,
    TDPARM_MAXSIZEM,
    TDPARM_EOTMARGIN,
    TDPARM_STRICTSIZE,
    TDPARM_READONLY,
    TDPARM_RO,
    TDPARM_NORING,
    TDPARM_RW,
    TDPARM_RING,
    TDPARM_DEONIRQ,
    TDPARM_NOAUTOMOUNT,
    TDPARM_BLKID22,
    TDPARM_BLKID24,
    TDPARM_BLKID32,
    TDPARM_NOERG,
    TDPARM_ONLINE
};

/*-------------------------------------------------------------------*/
/*        mountnewtape     --     mount a tape in the drive          */
/*-------------------------------------------------------------------*/
/*                                                                   */
/*  Syntax:       filename [options]                                 */
/*                                                                   */
/*  where options are any of the entries in the 'ptab' PARSER        */
/*  table defined further above. Some commonly used options are:     */
/*                                                                   */
/*    awstape          sets the HET parms to be compatible with the  */
/*                     R|P/390|'s tape file Format (HET files)       */
/*                                                                   */
/*    idrc|compress    0|1: Write tape blocks with compression       */
/*                     (std deviation: Read backward allowed on      */
/*                     compressed HET tapes while it is not on       */
/*                     IDRC formated 3480 tapes)                     */
/*                                                                   */
/*    --no-erg         for SCSI tape only, means the hardware does   */
/*                     not support the "Erase Gap" command and all   */
/*                     such i/o's should return 'success' instead.   */
/*                                                                   */
/*    --blkid-32       for SCSI tape only, means the hardware        */
/*                     only supports full 32-bit block-ids.          */
/*                                                                   */
/*    --online         for SCSI tape only, means the drive does not  */
/*                     use the clearing/setting of the GMT_DR_OPEN   */
/*                     flag to indicate when a tape is mounted but   */
/*                     rather uses the GMT_ONLINE flag instead.      */
/*                                                                   */
/*-------------------------------------------------------------------*/
int  mountnewtape ( DEVBLK *dev, int argc, char **argv )
{
    char*       short_descr;            /* Short descr from fmttab   */
    char        msg[80];
    int         i;                      /* Loop control              */
    int         rc, optrc;              /* various rtns return codes */
    bool        lock_obtained = false;  /* true if WE obtained lock  */

    union {                                 /* Parser results        */
        U32  num;                           /* Parser results        */
        BYTE str[ MAX_PARSER_STRLEN + 1 ];  /* Parser results        */
    } res;                                  /* Parser results        */

    /* Acquire the device lock before proceeding */
    if (!(lock_obtained = ((rc = try_obtain_lock( &dev->lock )) == 0)))
    {
        /* If some other thread (not us) owns it, then wait for it */
        if (rc != EBUSY)
        {
            OBTAIN_DEVLOCK( dev );
            lock_obtained = true; /* (so we know to release it) */
        }
        /* (else: presume we already owned it) */
    }

    /* Release the previous OMA descriptor array if allocated */
    if (dev->omadesc != NULL)
    {
        free (dev->omadesc);
        dev->omadesc = NULL;
    }

    /* The first argument is the file name */
    if (argc == 0 || strlen(argv[0]) >= sizeof(dev->filename))
        STRLCPY( dev->filename, TAPE_UNLOADED );
    else
    {
        /* Save the file name in the device block */
        hostpath(dev->filename, argv[0], sizeof(dev->filename));
    }

    /* Determine tape device type... (initializes "dev->tapedevt") */
    VERIFY( gettapetype( dev, &short_descr ) == 0 );

    /* (sanity check) */
    ASSERT(dev->tapedevt != TAPEDEVT_UNKNOWN);
    ASSERT(dev->tmh != NULL);
    ASSERT(short_descr != NULL);
    MSGBUF(msg, "not valid for %s", short_descr);

    /* Initialize device dependent fields */
    dev->fd                = -1;
    dev->omadesc           = NULL;
    dev->omafiles          = 0;
    dev->curfilen          = 1;
    dev->nxtblkpos         = 0;
    dev->prvblkpos         = -1;
    dev->curblkrem         = 0;
    dev->curbufoff         = 0;
    dev->readonly          = 0;
    dev->hetb              = NULL;
    dev->tdparms.compress  = HETDFLT_COMPRESS;
    dev->tdparms.method    = HETDFLT_METHOD;
    dev->tdparms.level     = HETDFLT_LEVEL;
    dev->tdparms.chksize   = HETDFLT_CHKSIZE;
    dev->tdparms.maxsize   = 0;        // no max size     (default)
    dev->eotmargin         = 128*1024; // 128K EOT margin (default)
    dev->tdparms.logical_readonly = 0; // read/write      (default)
    dev->noautomount       = 0;

#if defined(OPTION_SCSI_TAPE)
    // Real 3590's support Erase Gap and use 32-bit blockids.
    // Note that these are just defaults and may be changed
    // further below after processing our remaining options.

    if (TAPEDEVT_SCSITAPE == dev->tapedevt
        &&     0x3590     == dev->devtype)
    {
        dev->stape_no_erg   = 0;        // (default for 3590 SCSI)
        dev->stape_blkid_32 = 1;        // (default for 3590 SCSI)
    }
#endif

// "%1d:%04X Tape file '%s', type '%s': option '%s' rejected: '%s'"
#define  _HHC00223E() WRMSG(HHC00223, "E", LCSS_DEVNUM, dev->filename, TTYPSTR(dev->tapedevt), argv[i], msg)

    /* Process remaining options */
    rc = 0;
    for (i = 1; i < argc; i++)
    {
        optrc = 0;
        switch (parser( &ptab[0], argv[i], &res ))
        {
        case TDPARM_NONE:
            // "%1d:%04X Tape file '%s', type '%s': option '%s' rejected: '%s'"
            WRMSG(HHC00223, "E", LCSS_DEVNUM, dev->filename, TTYPSTR(dev->tapedevt), argv[i], "unrecognized");
            optrc = -1;
            break;

        case TDPARM_DEBUG:
            dev->debug = 1;
            break;

        case TDPARM_AWSTAPE:
            if (0
                || TAPEDEVT_SCSITAPE == dev->tapedevt
                || TAPEDEVT_FAKETAPE == dev->tapedevt
            )
            {
                // "%1d:%04X Tape file '%s', type '%s': option '%s' rejected: '%s'"
                _HHC00223E(); optrc = -1; break;
            }
            dev->tdparms.compress = FALSE;
            dev->tdparms.chksize = 4096;
            break;

        case TDPARM_IDRC:
        case TDPARM_COMPRESS:
            if (0
                || TAPEDEVT_SCSITAPE == dev->tapedevt
                || TAPEDEVT_FAKETAPE == dev->tapedevt
            )
            {
                // "%1d:%04X Tape file '%s', type '%s': option '%s' rejected: '%s'"
                _HHC00223E(); optrc = -1; break;
            }
            dev->tdparms.compress = (res.num ? TRUE : FALSE);
            break;

        case TDPARM_METHOD:
            if (0
                || TAPEDEVT_SCSITAPE == dev->tapedevt
                || TAPEDEVT_FAKETAPE == dev->tapedevt
            )
            {
                // "%1d:%04X Tape file '%s', type '%s': option '%s' rejected: '%s'"
                _HHC00223E(); optrc = -1; break;
            }
            if (res.num < HETMIN_METHOD || res.num > HETMAX_METHOD)
            {
                // "%1d:%04X Tape file '%s', type '%s': option '%s' rejected: '%s'"
                WRMSG(HHC00223, "E", LCSS_DEVNUM, dev->filename, TTYPSTR(dev->tapedevt), argv[i], "method out of range");
                optrc = -1;
                break;
            }
            dev->tdparms.method = res.num;
            break;

        case TDPARM_LEVEL:
            if (0
                || TAPEDEVT_SCSITAPE == dev->tapedevt
                || TAPEDEVT_FAKETAPE == dev->tapedevt
            )
            {
                // "%1d:%04X Tape file '%s', type '%s': option '%s' rejected: '%s'"
               _HHC00223E(); optrc = -1; break;
            }
            if (res.num < HETMIN_LEVEL || res.num > HETMAX_LEVEL)
            {
                // "%1d:%04X Tape file '%s', type '%s': option '%s' rejected: '%s'"
                WRMSG(HHC00223, "E", LCSS_DEVNUM, dev->filename, TTYPSTR(dev->tapedevt), argv[i], "level out of range");
                optrc = -1;
                break;
            }
            dev->tdparms.level = res.num;
            break;

        case TDPARM_CHKSIZE:
            if (0
                || TAPEDEVT_SCSITAPE == dev->tapedevt
                || TAPEDEVT_FAKETAPE == dev->tapedevt
            )
            {
                // "%1d:%04X Tape file '%s', type '%s': option '%s' rejected: '%s'"
                _HHC00223E(); optrc = -1; break;
            }
            if (res.num > 0 && (res.num < HETMIN_CHUNKSIZE || res.num > HETMAX_CHUNKSIZE))
            {
                // "%1d:%04X Tape file '%s', type '%s': option '%s' rejected: '%s'"
                WRMSG(HHC00223, "E", LCSS_DEVNUM, dev->filename, TTYPSTR(dev->tapedevt), argv[i], "chunksize out of range");
                optrc = -1;
                break;
            }
            dev->tdparms.chksize = (res.num == 0 ? HETDFLT_CHKSIZE : res.num);
            break;

        case TDPARM_MAXSIZE:
            if (TAPEDEVT_SCSITAPE == dev->tapedevt)
            {
                // "%1d:%04X Tape file '%s', type '%s': option '%s' rejected: '%s'"
                _HHC00223E(); optrc = -1; break;
            }
            else
            {
                int     rc      = 0;
                U64     maxsize = 0;
                BYTE    f       = '\0';
                BYTE    c       = '\0';

                rc = sscanf((const char*)res.str, "%"SCNu64"%c%c", &maxsize, &f, &c);
                if ( rc < 1 || rc > 2 )
                {
                    WRMSG( HHC01451, "E", res.str, "maxsize" );
                    optrc = -1;
                }
                else if ( rc == 2 )
                {
                    switch (toupper((unsigned char)f))
                    {
                        case 'K':
                            maxsize <<= SHIFT_KIBIBYTE;
                            break;
                        case 'M':
                            maxsize <<= SHIFT_MEBIBYTE;
                            break;
                        case 'G':
                            maxsize <<= SHIFT_GIBIBYTE;
                            break;
#if SIZEOF_SIZE_T >= 8
                        case 'T':
                            maxsize <<= SHIFT_TEBIBYTE;
                            break;
#endif
                        default:
                            WRMSG( HHC01451, "E", res.str, "maxsize" );
                            maxsize = 0;
                            optrc = -1;
                            break;
                    }
                    if ( optrc != -1 )
                        dev->tdparms.maxsize = maxsize;
                }
                else
                    dev->tdparms.maxsize = maxsize;
            }
            break;

        case TDPARM_MAXSIZEK:
            if (TAPEDEVT_SCSITAPE == dev->tapedevt)
            {
                // "%1d:%04X Tape file '%s', type '%s': option '%s' rejected: '%s'"
                _HHC00223E(); optrc = -1; break;
            }
            dev->tdparms.maxsize=res.num*1024;
            break;

        case TDPARM_MAXSIZEM:
            if (TAPEDEVT_SCSITAPE == dev->tapedevt)
            {
                // "%1d:%04X Tape file '%s', type '%s': option '%s' rejected: '%s'"
                _HHC00223E(); optrc = -1; break;
            }
            dev->tdparms.maxsize=res.num*1024*1024;
            break;

        case TDPARM_EOTMARGIN:
            {
                int     rc          = 0;
                U64     eotmargin   = 0;
                BYTE    f           = '\0';
                BYTE    c           = '\0';

                rc = sscanf((const char*)res.str, "%"SCNu64"%c%c", &eotmargin, &f, &c);
                if ( rc < 1 || rc > 2 )
                {
                    WRMSG( HHC01451, "E", res.str, "eotmargin" );
                    optrc = -1;
                }
                else if ( rc == 2 )
                {
                    switch (toupper((unsigned char)f))
                    {
                        case 'K':
                            eotmargin <<= SHIFT_KIBIBYTE;
                            break;
                        case 'M':
                            eotmargin <<= SHIFT_MEBIBYTE;
                            break;
                        case 'G':
                            eotmargin <<= SHIFT_GIBIBYTE;
                            break;
#if SIZEOF_SIZE_T >= 8
                        case 'T':
                            eotmargin <<= SHIFT_TEBIBYTE;
                            break;
#endif
                        default:
                            WRMSG( HHC01451, "E", res.str, "eotmargin" );
                            eotmargin = 0;
                            optrc = -1;
                            break;
                    }
                    if ( optrc != -1 )
                        dev->eotmargin = eotmargin;
                }
                else
                    dev->eotmargin = eotmargin;
            }
            break;

        case TDPARM_STRICTSIZE:
            if (TAPEDEVT_SCSITAPE == dev->tapedevt)
            {
                // "%1d:%04X Tape file '%s', type '%s': option '%s' rejected: '%s'"
                _HHC00223E(); optrc = -1; break;
            }
            dev->tdparms.strictsize=res.num;
            break;

        case TDPARM_READONLY:
            if (TAPEDEVT_SCSITAPE == dev->tapedevt)
            {
                // "%1d:%04X Tape file '%s', type '%s': option '%s' rejected: '%s'"
                _HHC00223E(); optrc = -1; break;
            }
            dev->tdparms.logical_readonly=(res.num ? 1 : 0 );
            break;

        case TDPARM_RO:
        case TDPARM_NORING:
            if (TAPEDEVT_SCSITAPE == dev->tapedevt)
            {
                // "%1d:%04X Tape file '%s', type '%s': option '%s' rejected: '%s'"
                _HHC00223E(); optrc = -1; break;
            }
            dev->tdparms.logical_readonly=1;
            break;

        case TDPARM_RW:
        case TDPARM_RING:
            if (TAPEDEVT_SCSITAPE == dev->tapedevt)
            {
                // "%1d:%04X Tape file '%s', type '%s': option '%s' rejected: '%s'"
                _HHC00223E(); optrc = -1; break;
            }
            dev->tdparms.logical_readonly=0;
            break;

        case TDPARM_DEONIRQ:
            if (TAPEDEVT_SCSITAPE == dev->tapedevt)
            {
                // "%1d:%04X Tape file '%s', type '%s': option '%s' rejected: '%s'"
                _HHC00223E(); optrc = -1; break;
            }
            dev->tdparms.deonirq=(res.num ? 1 : 0 );
            break;

        case TDPARM_NOAUTOMOUNT:
            if (TAPEDEVT_SCSITAPE == dev->tapedevt)
            {
                // "%1d:%04X Tape file '%s', type '%s': option '%s' rejected: '%s'"
                _HHC00223E(); optrc = -1; break;
            }
            dev->noautomount = 1;
            break;

#if defined(OPTION_SCSI_TAPE)
        case TDPARM_BLKID22:
        case TDPARM_BLKID24:
            if (TAPEDEVT_SCSITAPE != dev->tapedevt)
            {
                // "%1d:%04X Tape file '%s', type '%s': option '%s' rejected: '%s'"
                _HHC00223E(); optrc = -1; break;
            }
            dev->stape_blkid_32 = 0;
            break;

        case TDPARM_BLKID32:
            if (TAPEDEVT_SCSITAPE != dev->tapedevt)
            {
                // "%1d:%04X Tape file '%s', type '%s': option '%s' rejected: '%s'"
                _HHC00223E(); optrc = -1; break;
            }
            dev->stape_blkid_32 = 1;
            break;

        case TDPARM_NOERG:
            if (TAPEDEVT_SCSITAPE != dev->tapedevt)
            {
                // "%1d:%04X Tape file '%s', type '%s': option '%s' rejected: '%s'"
                _HHC00223E(); optrc = -1; break;
            }
            dev->stape_no_erg = 1;
            break;

        case TDPARM_ONLINE:
            if (TAPEDEVT_SCSITAPE != dev->tapedevt)
            {
                // "%1d:%04X Tape file '%s', type '%s': option '%s' rejected: '%s'"
                _HHC00223E(); optrc = -1; break;
            }
            dev->stape_online = 1;
            break;
#endif /* defined(OPTION_SCSI_TAPE) */

        default:
            // "%1d:%04X Tape file '%s', type '%s': option '%s' rejected: '%s'"
            WRMSG(HHC00223, "E", LCSS_DEVNUM, dev->filename, TTYPSTR(dev->tapedevt), argv[i], "parse error");
            optrc = -1;
            break;

        } // end switch (parser (&ptab[0], argv[i], &res))

        if (optrc < 0)
            rc = -1;
        else
        {
            // "%1d:%04X Tape file '%s', type '%s': option '%s' accepted"
            WRMSG(HHC00222, "I", LCSS_DEVNUM, dev->filename, TTYPSTR(dev->tapedevt), argv[i]);
        }

    } // end for (i = 1; i < argc; i++)

    if (0 != rc)
    {
        if (lock_obtained) // (release lock only if WE obtained it)
            RELEASE_DEVLOCK( dev );
        return -1;
    }

#if defined(OPTION_SCSI_TAPE)
    if (dev->tapedevt == TAPEDEVT_SCSITAPE)
        dev->sstat = dev->stape_online ? 0 : GMT_DR_OPEN( -1 );
#endif

    /* Adjust the display if necessary */
    if(dev->tdparms.displayfeat)
    {
        if(strcmp(dev->filename,TAPE_UNLOADED)==0)
        {
            /* NO tape is loaded */
            if(TAPEDISPTYP_UMOUNTMOUNT == dev->tapedisptype)
            {
                /* A new tape SHOULD be mounted */
                dev->tapedisptype   = TAPEDISPTYP_MOUNT;
                dev->tapedispflags |= TAPEDISPFLG_REQAUTOMNT;
                STRLCPY( dev->tapemsg1, dev->tapemsg2 );
            }
            else if(TAPEDISPTYP_UNMOUNT == dev->tapedisptype)
            {
                dev->tapedisptype = TAPEDISPTYP_IDLE;
            }
        }
        else
        {
            /* A tape IS already loaded */
            dev->tapedisptype = TAPEDISPTYP_IDLE;
        }
    }

    if (lock_obtained) // (release lock only if WE obtained it)
        RELEASE_DEVLOCK( dev );

    UpdateDisplay(dev);
    rc = ReqAutoMount(dev);

    return rc;

} /* end function mountnewtape */


/*-------------------------------------------------------------------*/
/* Query the device definition                                       */
/*-------------------------------------------------------------------*/
static void tapedev_query_device ( DEVBLK *dev, char **devclass, int buflen, char *buffer )
{
    char filename[ PATH_MAX + 1 ];     /* full path or just name    */
    char devparms[ MAX_PATH + 1 + 128 ];
    char dispmsg [ 256 ];
    char fmt_mem [ 128 ];    // Max of 21 bytes used for U64
    char fmt_eot [ 128 ];    // Max of 21 bytes used for U64

    BEGIN_DEVICE_CLASS_QUERY( "TAPE", dev, devclass, buflen, buffer );

    memset(buffer, 0, buflen);
    memset(devparms, 0, sizeof(devparms));
    memset(dispmsg, 0, sizeof(dispmsg));
    memset(fmt_mem, 0, sizeof(fmt_mem));
    memset(fmt_eot, 0, sizeof(fmt_eot));

    GetDisplayMsg( dev, dispmsg, sizeof(dispmsg) );

    if (strchr(filename,' ')) STRLCAT( devparms, "\""     );
                              STRLCAT( devparms, filename );
    if (strchr(filename,' ')) STRLCAT( devparms, "\""     );

    if (dev->noautomount)
        STRLCAT( devparms, " noautomount" );

#if defined(OPTION_SCSI_TAPE)
    if ( TAPEDEVT_SCSITAPE != dev->tapedevt )
#endif
    {
        const  char suffix[9] = {0x00, 'K', 'M', 'G', 'T', 'P', 'E', 'Z', 'Y'};
        u_int   i = 0;

        U64 mem = (U64)dev->tdparms.maxsize;

        if (mem)
        {
            for (i = 0; i < sizeof(suffix); i++)
            {
                if (mem & 0x3FF)
                    break;
                mem >>= 10;
            }
        }
        MSGBUF( fmt_mem, " maxsize=%"PRIu64"%c", mem, suffix[i]);

        mem = (U64)dev->eotmargin;

        i = 0;
        if (mem)
        {
            for (i = 0; i < sizeof(suffix); i++)
            {
                if (mem & 0x3FF)
                    break;
                mem >>= 10;
            }
        }
        MSGBUF( fmt_eot, " eotmargin=%"PRIu64"%c", mem, suffix[i]);
    }

    if ( strcmp( filename, TAPE_UNLOADED ) == 0 )
    {
#if defined(OPTION_SCSI_TAPE)
        if ( TAPEDEVT_SCSITAPE == dev->tapedevt )
        {
            if (0x3590 == dev->devtype) // emulating 3590
            {
                if (!dev->stape_blkid_32 ) STRLCAT( devparms, " --blkid-22" );
            }
            else // emulating 3480, 3490
            {
                if ( dev->stape_blkid_32 ) STRLCAT( devparms, " --blkid-32" );
            }
            if ( dev->stape_no_erg ) STRLCAT( devparms, " --no-erg" );
        }
#endif
        snprintf(buffer, buflen, "%s%s%s IO[%"PRIu64"]%s%s deonirq=%c",
            devparms,
            dev->tdparms.displayfeat ? ", Display: " : "",
            dev->tdparms.displayfeat ?    dispmsg    : "",
            dev->excps,
            fmt_mem, fmt_eot, dev->tdparms.deonirq == 0 ? 'N' : 'Y' );
        buffer[buflen-1] = '\0';
    }
    else // (filename was specified)
    {
        char tapepos[64];

        memset(tapepos, 0, sizeof(tapepos));

        if ( TAPEDEVT_SCSITAPE != dev->tapedevt )
        {
            MSGBUF( tapepos, "[%d:%08"PRIX64"] ", dev->curfilen, dev->nxtblkpos );
        }
#if defined(OPTION_SCSI_TAPE)
        else // (this is a SCSI tape drive)
        {
            if (STS_BOT( dev ))
            {
                dev->eotwarning = 0;
                STRLCAT( tapepos, "*BOT* " );
            }

            // If tape has a display, then GetDisplayMsg already
            // appended *FP* for us. Otherwise we need to do it.

            if ( !dev->tdparms.displayfeat )
                if (STS_WR_PROT( dev ))
                    STRLCAT( tapepos, "*FP* " );

            if (0x3590 == dev->devtype) // emulating 3590
            {
                if (!dev->stape_blkid_32 ) STRLCAT( devparms, " --blkid-22" );
            }
            else // emulating 3480, 3490
            {
                if ( dev->stape_blkid_32 ) STRLCAT( devparms, " --blkid-32" );
            }
            if ( dev->stape_no_erg ) STRLCAT( devparms, " --no-erg" );
        }
#endif

        if ( TAPEDEVT_SCSITAPE != dev->tapedevt
#if defined(OPTION_SCSI_TAPE)
            || STS_MOUNTED(dev)
#endif
        )
        {
            // Not a SCSI tape,  -or-  mounted SCSI tape...

            snprintf( buffer, buflen, "%s%s %s%s%s IO[%"PRIu64"]",
                devparms, (dev->readonly ? " ro" : ""),
                tapepos,
                dev->tdparms.displayfeat ? "Display: " : "",
                dev->tdparms.displayfeat ?  dispmsg    : "",
                dev->excps );
            buffer[buflen-1] = '\0';
        }
        else /* ( TAPEDEVT_SCSITAPE == dev->tapedevt && STS_NOT_MOUNTED(dev) ) */
        {
            // UNmounted SCSI tape...

            snprintf( buffer, buflen, "%s%s (%sNOTAPE)%s%s IO[%"PRIu64"]",
                devparms, (dev->readonly ? " ro" : ""),
                dev->fd < 0              ?   "closed; "  : "",
                dev->tdparms.displayfeat ? ", Display: " : "",
                dev->tdparms.displayfeat ?    dispmsg    : "",
                dev->excps );
            buffer[buflen-1] = '\0';
        }
    }

    return;

} /* end function tapedev_query_device */


/*-------------------------------------------------------------------*/
/* Issue a message on the console indicating the display status      */
/*-------------------------------------------------------------------*/
void UpdateDisplay( DEVBLK *dev )
{
    if ( dev->tdparms.displayfeat )
    {
        char msgbfr[256];

        GetDisplayMsg( dev, msgbfr, sizeof(msgbfr) );

        if ( dev->prev_tapemsg )
        {
            if ( strcmp( msgbfr, dev->prev_tapemsg ) == 0 )
                return;
            free( dev->prev_tapemsg );
            dev->prev_tapemsg = NULL;
        }

        dev->prev_tapemsg = strdup( msgbfr );

        // "%1d:%04X Tape file '%s', type '%s': display '%s'"
        WRMSG(HHC00224, "I", LCSS_DEVNUM, dev->filename, TTYPSTR(dev->tapedevt), msgbfr );
    }
#if defined(OPTION_SCSI_TAPE)
    else
        if (TAPEDEVT_SCSITAPE == dev->tapedevt)
            int_scsi_status_update( dev, 1 );
#endif
}


/*-------------------------------------------------------------------*/
/* Issue Automatic Mount Requests as defined by the display          */
/*-------------------------------------------------------------------*/
int  ReqAutoMount( DEVBLK *dev )
{
    int    rc = 0;
    char   volser[7];
    BYTE   tapeloaded, autoload, mountreq, unmountreq, stdlbled, scratch;
    char*  lbltype;
    char*  tapemsg = "                  ";

    ///////////////////////////////////////////////////////////////////

    // The Automatic Cartridge Loader or "ACL" (sometimes also referred
    // to as an "Automatic Cartridge Feeder" (ACF) too) automatically
    // loads the next cartridge [from the magazine] whenever a tape is
    // unloaded, BUT ONLY IF the 'Index Automatic Load' bit (bit 7) of
    // the FCB (Format Control Byte, byte 0) was on whenever the Load
    // Display ccw was sent to the drive. If the bit was not on when
    // the Load Display ccw was issued, then the requested message (if
    // any) is displayed until the next tape mount/dismount and the ACL
    // is NOT activated (i.e. the next tape is NOT automatically loaded).
    // If the bit was on however, then, as stated, the ACF component of
    // the drive will automatically load the next [specified] cartridge.

    // Whenever the ACL facility is activated (via bit 7 of byte 0 of
    // the Load Display ccw), then only bytes 1-8 of the "Display Until
    // Mounted" message (or bytes 9-17 of a "Display Until Dismounted
    // Then Mounted" message) are displayed to let the operator know
    // which tape is currently being processed by the autoloader and
    // thus is basically for informational purposes only (the operator
    // does NOT need to do anything since the auto-loader is handling
    // tape mounts for them automatically; i.e. the message is NOT an
    // operator mount/dismount request).

    // If the 'Index Automatic Load' bit was not set in the Load Display
    // CCW however, then the specified "Display Until Mounted", "Display
    // Until Unmounted" or "Display Until Unmounted Then Display Until
    // Mounted" message is meant as a mount, unmount, or unmount-then-
    // mount request for the actual [human being] operator, and thus
    // they DO need to take some sort of action (since the ACL automatic
    // loader facility is not active; i.e. the message is a request to
    // the operator to manually unload, load or unload then load a tape).

    // THUS... If the TAPEDISPFLG_AUTOLOADER flag is set (indicating
    // the autoloader is (or should be) active), then the message we
    // issue is simply for INFORMATIONAL purposes only (i.e. "FYI: the
    // following tape is being *automatically* loaded; you don't need
    // to actually do anything")

    // If the TAPEDISPFLG_AUTOLOADER is flag is NOT set however, then
    // we need to issue a message notifying the operator of what they
    // are *expected* to do (e.g. either unload, load or unload/load
    // the specified tape volume).

    // Also please note that while there are no formally established
    // standards regarding the format of the Load Display CCW message
    // text, there are however certain established conventions (estab-
    // lished by IBM naturally). If the first character is an 'M', it
    // means "Please MOUNT the indicated volume". An 'R' [apparently]
    // means "Retain", and, similarly, 'K' means "Keep" (same thing as
    // "Retain"). If the LAST character is an 'S', then it means that
    // a Standard Labeled volume is being requested, whereas an 'N'
    // (or really, anything OTHER than an 'S' (except 'A')) means an
    // unlabeled (or non-labeled) tape volume is being requested. An
    // 'A' as the last character means a Standard Labeled ASCII tape
    // is being requested. If the message is "SCRTCH" (or something
    // similar), then a either a standard labeled or unlabeled scratch
    // tape is obviously being requested (there doesn't seem to be any
    // convention/consensus regarding the format for requesting scratch
    // tapes; some shops for example use 'XXXSCR' to indicate that a
    // scratch tape from tape pool 'XXX' should be mounted).

    ///////////////////////////////////////////////////////////////////

    /* Open the file/drive if needed (kick off auto-mount if needed) */
    if (dev->fd < 0)
    {
        BYTE   unitstat = 0, code = 0;
        BYTE   sensebkup[ _countof( dev->sense )];  // (same size)

        /* Save any pending sense */
        memcpy( sensebkup, dev->sense, sizeof( sensebkup ));

        /* Open the file/drive */
        rc = dev->tmh->open( dev, &unitstat, code );

        /* Ignore open error if no tape being mounted */
        if (strcmp( dev->filename, TAPE_UNLOADED ) == 0)
            rc = 0;

        /* Restore pending sense */
        // FIXME: (but only if the open succeeded?? Is this right??)
        if (rc >= 0)
            memcpy( dev->sense, sensebkup, sizeof( dev->sense ));

#if defined(OPTION_SCSI_TAPE)
        if (TAPEDEVT_SCSITAPE == dev->tapedevt)
        {
            // PROGRAMMING NOTE: it's important to do TWO refreshes here
            // to cause the auto-mount thread to get created. Doing only
            // one doesn't work and doing two shouldn't cause any harm.

            GENTMH_PARMS  gen_parms;

            gen_parms.action  = GENTMH_SCSI_ACTION_UPDATE_STATUS;
            gen_parms.dev     = dev;

            // (refresh potentially stale status)
            VERIFY( dev->tmh->generic( &gen_parms ) == 0 );

            // (force auto-mount thread creation)
            VERIFY( dev->tmh->generic( &gen_parms ) == 0 );
        }
#endif /* defined(OPTION_SCSI_TAPE) */

        /* Return with error if open failed */
        if (rc < 0)
            return rc;
    }

    /* Disabled when [non-SCSI] ACL in use */
    if (dev->als)
        return rc;

    /* Do we actually have any work to do? */
    if (!(dev->tapedispflags & TAPEDISPFLG_REQAUTOMNT))
        return rc;     // (nothing to do!)

    /* Reset work flag */
    dev->tapedispflags &= ~TAPEDISPFLG_REQAUTOMNT;

    /* If the drive doesn't have a display,
       then it can't have an auto-loader either */
    if (!dev->tdparms.displayfeat)
        return rc;

    /* Determine if mount or unmount request
       and get pointer to correct message */

    tapeloaded = dev->tmh->tapeloaded( dev, NULL, 0 ) ? TRUE : FALSE;

    mountreq   = FALSE;     // (default)
    unmountreq = FALSE;     // (default)

    if (tapeloaded)
    {
        // A tape IS already loaded...

        // 1st byte of message1 non-blank, *AND*,
        // unmount request,
        // or unmount+mount request and not message2-only flag?

        tapemsg = dev->tapemsg1;

        if (1
            && ' ' != tapemsg[0]
            && (0
                || TAPEDISPTYP_UNMOUNT == dev->tapedisptype
                || (1
                    && TAPEDISPTYP_UMOUNTMOUNT == dev->tapedisptype
                    && !(dev->tapedispflags & TAPEDISPFLG_MESSAGE2)
                   )
               )
        )
            unmountreq = TRUE;
    }
    else
    {
        // NO TAPE is loaded yet...

        // mount request and 1st byte of msg1 non-blank, *OR*,
        // unmount+mount request and 1st byte of msg2 non-blank?

        if ((1
                && TAPEDISPTYP_MOUNT == dev->tapedisptype
                && ' ' != *(tapemsg = dev->tapemsg1)
            )
            ||
            (1
                && TAPEDISPTYP_UMOUNTMOUNT == dev->tapedisptype
                && ' ' != *(tapemsg = dev->tapemsg2)
            )
        )
            mountreq = TRUE;
    }

    /* Extract volser from message */
    strncpy( volser, tapemsg+1, 6 );
    volser[6] = 0;

    /* Set some boolean flags */
    autoload = ( dev->tapedispflags & TAPEDISPFLG_AUTOLOADER )    ?  TRUE  :  FALSE;
    stdlbled = ( 'S' == tapemsg[7] )                              ?  TRUE  :  FALSE;
//  ascii    = ( 'A' == tapemsg[7] )                              ?  TRUE  :  FALSE;
    scratch  = ( 'S' == tapemsg[0] )                              ?  TRUE  :  FALSE;

    lbltype = stdlbled ? "SL" : "UL";

#if defined(OPTION_SCSI_TAPE)
#if 1
    // ****************************************************************
    // ZZ FIXME: ZZ TODO:   ***  Programming Note  ***

    // Since we currently don't have any way of activating a SCSI tape
    // drive's REAL autoloader mechanism whenever we receive an auto-
    // mount message [from the guest o/s via the Load Display CCW], we
    // leave it to the operator to action the mount message displayed

    // Once ASPI code eventually gets added to Herc (and/or something
    // similar for the Linux world), then the following workaround can
    // be safely removed.

    autoload = FALSE;       // (temporarily forced; see above)

    // ****************************************************************
#endif
#endif /* defined(OPTION_SCSI_TAPE) */

    if (autoload)
    {
        // ZZ TODO: Here is where we'd issue i/o (ASPI?) to the actual
        // hardware autoloader facility (i.e. the SCSI medium changer)
        // to unload and/or load the tape(s) if this were a SCSI auto-
        // loading tape drive.

        if (unmountreq)
        {
            // "%1d:%04X Tape file '%s', type '%s': '%s' tape volume '%s' being auto unloaded"
            WRMSG( HHC00226, "I", LCSS_DEVNUM, dev->filename,
                TTYPSTR( dev->tapedevt ),
                lbltype, scratch ? "<scratch>" : volser );
        }
        if (mountreq)
        {
            // "%1d:%04X Tape file '%s', type '%s': '%s' tape volume '%s' being auto loaded"
            WRMSG( HHC00227, "I", LCSS_DEVNUM, dev->filename,
                TTYPSTR( dev->tapedevt ),
                lbltype, scratch ? "<scratch>" : volser );
        }
    }

    return rc;
} /* end function ReqAutoMount */


/*-------------------------------------------------------------------*/
/*      Get 3480/3490/3590 Display text in 'human' form              */
/* If not a 3480/3490/3590, then just update status if a SCSI tape   */
/*-------------------------------------------------------------------*/
void GetDisplayMsg( DEVBLK *dev, char *msgbfr, size_t  lenbfr )
{
    msgbfr[0]=0;

    if ( !dev->tdparms.displayfeat )
    {
        // (drive doesn't have a display)
#if defined(OPTION_SCSI_TAPE)
        if (TAPEDEVT_SCSITAPE == dev->tapedevt)
            int_scsi_status_update( dev, 1 );
#endif
        return;
    }

    if ( !IS_TAPEDISPTYP_SYSMSG( dev ) )
    {
        // -------------------------
        //   Display Host message
        // -------------------------

        // "When bit 3 (alternate) is set to 1, then
        //  bits 4 (blink) and 5 (low/high) are ignored."

        strlcpy( msgbfr, "\"", lenbfr );

        if ( dev->tapedispflags & TAPEDISPFLG_ALTERNATE )
        {
            char  msg1[9];
            char  msg2[9];

            STRLCPY( msg1,   dev->tapemsg1 );
            STRLCAT( msg1,   "        "    );
            STRLCPY( msg2,   dev->tapemsg2 );
            STRLCAT( msg2,   "        "    );

            strlcat( msgbfr, msg1,             lenbfr );
            strlcat( msgbfr, "\" / \"",        lenbfr );
            strlcat( msgbfr, msg2,             lenbfr );
            strlcat( msgbfr, "\"",             lenbfr );
            strlcat( msgbfr, " (alternating)", lenbfr );
        }
        else
        {
            if ( dev->tapedispflags & TAPEDISPFLG_MESSAGE2 )
                strlcat( msgbfr, dev->tapemsg2, lenbfr );
            else
                strlcat( msgbfr, dev->tapemsg1, lenbfr );

            strlcat ( msgbfr, "\"",          lenbfr );

            if ( dev->tapedispflags & TAPEDISPFLG_BLINKING )
                strlcat ( msgbfr, " (blinking)", lenbfr );
        }

        if ( dev->tapedispflags & TAPEDISPFLG_AUTOLOADER )
            strlcat( msgbfr, " (AUTOLOADER)", lenbfr );

        return;
    }

    // ----------------------------------------------
    //   Display SYS message (Unit/Device message)
    // ----------------------------------------------

    // First, build the system message, then move it into
    // the caller's buffer...

    STRLCPY( dev->tapesysmsg, "\"" );

    switch ( dev->tapedisptype )
    {
    case TAPEDISPTYP_IDLE:
    case TAPEDISPTYP_WAITACT:
    default:
        // Blank display if no tape loaded...
        if ( !dev->tmh->tapeloaded( dev, NULL, 0 ) )
        {
            STRLCAT( dev->tapesysmsg, "        " );
            break;
        }

        // " NT RDY " if tape IS loaded, but not ready...
        // (IBM docs say " NT RDY " means "Loaded but not ready")

        ASSERT( dev->tmh->tapeloaded( dev, NULL, 0 ) );

        if (0
            || dev->fd < 0
#if defined(OPTION_SCSI_TAPE)
            || (1
                && TAPEDEVT_SCSITAPE == dev->tapedevt
                && !STS_ONLINE( dev )
               )
#endif
        )
        {
            STRLCAT( dev->tapesysmsg, " NT RDY " );
            break;
        }

        // Otherwise tape is loaded and ready  -->  "READY"

        ASSERT( dev->tmh->tapeloaded( dev, NULL, 0 ) );

        STRLCAT( dev->tapesysmsg, " READY  " );
        STRLCAT( dev->tapesysmsg, "\"" );

        if (0
            || dev->readonly
#if defined(OPTION_SCSI_TAPE)
            || (1
                &&  TAPEDEVT_SCSITAPE == dev->tapedevt
                &&  STS_WR_PROT( dev )
               )
#endif
        )
            // (append "file protect" indicator)
            STRLCAT( dev->tapesysmsg, " *FP*" );

        // Copy system message to caller's buffer
        strlcpy( msgbfr, dev->tapesysmsg, lenbfr );
        return;

    case TAPEDISPTYP_ERASING:
        STRLCAT( dev->tapesysmsg, " ERASING" );
        break;

    case TAPEDISPTYP_REWINDING:
        STRLCAT( dev->tapesysmsg, "REWINDNG" );
        break;

    case TAPEDISPTYP_UNLOADING:
        STRLCAT( dev->tapesysmsg, "UNLOADNG" );
        break;

    case TAPEDISPTYP_CLEAN:
        STRLCAT( dev->tapesysmsg, "*CLEAN  " );
        break;
    }

    STRLCAT( dev->tapesysmsg, "\"" );

    // Copy system message to caller's buffer
    strlcpy( msgbfr, dev->tapesysmsg, lenbfr );

} /* end function GetDisplayMsg */


/*-------------------------------------------------------------------*/
/*                         IsAtLoadPoint                             */
/*-------------------------------------------------------------------*/
/* Called by the device-type-specific 'build_sense_xxxx' functions   */
/* (indirectly via the 'build_senseX' function) when building sense  */
/* for any i/o error (non-"TAPE_BSENSE_STATUSONLY" type call)       */
/*-------------------------------------------------------------------*/
int IsAtLoadPoint (DEVBLK *dev)
{
int ldpt=0;
    if ( dev->fd >= 0 )
    {
        /* Set load point indicator if tape is at load point */
        switch (dev->tapedevt)
        {
        default:
        case TAPEDEVT_AWSTAPE:
            if (dev->nxtblkpos==0)
            {
                ldpt=1;
            }
            break;

        case TAPEDEVT_HETTAPE:
            if (dev->hetb->cblk == 0)
            {
                ldpt=1;
            }
            break;

#if defined(OPTION_SCSI_TAPE)
        case TAPEDEVT_SCSITAPE:
            int_scsi_status_update( dev, 0 );
            if ( STS_BOT( dev ) )
            {
                dev->eotwarning = 0;
                ldpt=1;
            }
            break;
#endif /* defined(OPTION_SCSI_TAPE) */

        case TAPEDEVT_OMATAPE:
            if (dev->nxtblkpos == 0 && dev->curfilen == 1)
            {
                ldpt=1;
            }
            break;
        } /* end switch(dev->tapedevt) */
    }
    else // ( dev->fd < 0 )
    {
        if ( TAPEDEVT_SCSITAPE == dev->tapedevt )
            ldpt=0; /* (tape cannot possibly be at loadpoint
                        if the device cannot even be opened!) */
        else if ( strcmp( dev->filename, TAPE_UNLOADED ) != 0 )
        {
            /* If the tape has a filename but the tape is not yet */
            /* opened, then we are at loadpoint                   */
            ldpt=1;
        }
    }
    return ldpt;

} /* end function IsAtLoadPoint */


/*********************************************************************/
/*********************************************************************/
/**                                                                 **/
/**                   AUTOLOADER FUNCTIONS                          **/
/**                                                                 **/
/*********************************************************************/
/*********************************************************************/

/*-------------------------------------------------------------------*/
/*                         autoload_init                             */
/*-------------------------------------------------------------------*/
/*  initialise the Autoloader feature                                */
/*-------------------------------------------------------------------*/
void autoload_init( DEVBLK *dev, int ac, char **av )
{
char     bfr[4096];
char    *rec;
FILE    *aldf;
char     pathname[MAX_PATH];
int      argc;
char    *argv[MAX_ARGS];

    autoload_close( dev );

    if (0
        || ac < 1
        || av[0][0] != '@'
    )
        return;

    // "Tape autoloader: file request fn '%s'"
    WRMSG(HHC00228, "I", &av[0][1]);

    hostpath( pathname, &av[0][1], sizeof(pathname) );

    if (!(aldf = fopen( pathname, "r" )))
        return; // open error

    if (ac > 1)
        autoload_global_parms( dev, ac-1, &av[1] );

    while ((rec = fgets( bfr, 4096, aldf )))
    {
        if (0 == parse_args( rec, MAX_ARGS, argv, &argc ))
            continue; // (zero args == blank line; skip..)

        if (strcmp( argv[0], "*" ) == 0)
            autoload_global_parms( dev, argc, argv );
        else
            autoload_tape_entry( dev, argc, argv );
    }

    fclose(aldf);
    return;

} /* end function autoload_init */


/*-------------------------------------------------------------------*/
/*                      autoload_close                               */
/*-------------------------------------------------------------------*/
/*  terminate autoloader operations: release all storage that        */
/*  was allocated by the autoloader facility                         */
/*-------------------------------------------------------------------*/
void autoload_close(DEVBLK *dev)
{
int i;

    if(dev->al_argv!=NULL)
    {
        for(i=0;i<dev->al_argc;i++)
        {
            free(dev->al_argv[i]);
            dev->al_argv[i]=NULL;
        }
        free(dev->al_argv);
        dev->al_argv=NULL;
        dev->al_argc=0;
    }
    dev->al_argc=0;
    if(dev->als!=NULL)
    {
        for(i=0;i<dev->alss;i++)
        {
            autoload_clean_entry(dev,i);
        }
        free(dev->als);
        dev->als=NULL;
        dev->alss=0;
    }
} /* end function autoload_close */


/*-------------------------------------------------------------------*/
/*                    autoload_clean_entry                           */
/*-------------------------------------------------------------------*/
/*  release storage allocated for an autoloader slot                 */
/*  (except the slot itself)                                         */
/*-------------------------------------------------------------------*/
void autoload_clean_entry(DEVBLK *dev,int ix)
{
int i;

    for(i=0;i<dev->als[ix].argc;i++)
    {
        free(dev->als[ix].argv[i]);
        dev->als[ix].argv[i]=NULL;
    }
    dev->als[ix].argc=0;
    if(dev->als[ix].filename!=NULL)
    {
        free(dev->als[ix].filename);
        dev->als[ix].filename=NULL;
    }
} /* end function autoload_clean_entry */


/*-------------------------------------------------------------------*/
/*                    autoload_global_parms                          */
/*-------------------------------------------------------------------*/
/*  Appends an additional list of parameters that will               */
/*  be passed for every tape mounted by the autoloader               */
/*-------------------------------------------------------------------*/
void autoload_global_parms( DEVBLK *dev, int argc, char *argv[] )
{
int i;
char *p;

    if (!dev->al_argv)
    {
        dev->al_argv = calloc( AUTOLOADER_MAX, sizeof(char*));
        dev->al_argc = 0;
        if (!dev->al_argv) return;
    }

    for (i=1; i < argc && dev->al_argc < AUTOLOADER_MAX; ++i)
    {
        p = (char*) strdup( argv[i] );
        if (!p) return;

        // "Tape autoloader: adding '%s' value '%s'"
        WRMSG(HHC00229, "I", "global parm", p);
        dev->al_argv[ dev->al_argc ] = p;
        dev->al_argc++;
    }

} /* end function autoload_global_parms */


/*-------------------------------------------------------------------*/
/*                    autoload_tape_entry                            */
/*-------------------------------------------------------------------*/
/*  populate an autoloader slot  (creates new slot if needed)        */
/*-------------------------------------------------------------------*/
void autoload_tape_entry( DEVBLK *dev, int argc, char *argv[] )
{
int                i;
TAPEAUTOLOADENTRY  tae;

    if (dev->alss >= AUTOLOADER_MAX)
        return;

    if (!dev->als)
    {
        dev->als  = calloc( 1, sizeof(tae) );
        dev->alss = 0;
        if (!dev->als) return;
    }
    else
    {
        TAPEAUTOLOADENTRY *als = (TAPEAUTOLOADENTRY*)
            realloc( dev->als, (dev->alss + 1) * sizeof(tae) );
        if (!als) return;
        dev->als = als;
    }

    tae.argc = 0;
    tae.argv = NULL;
    tae.filename = strdup( argv[0] );
    if (!tae.filename) return;

    // "Tape autoloader: adding '%s' value '%s'"
    WRMSG( HHC00229, "I", "tape entry", tae.filename );

    if (argc > 1)
    {
        tae.argv = calloc( argc-1, sizeof(char*));
        if (!tae.argv)
        {
            free( tae.filename );
            return;
        }

        for (i=1, tae.argc=0; i < argc-1; i++)
        {
            tae.argv[ tae.argc ] = strdup( argv[i] );
            if (!tae.argv[ tae.argc ]) break;
            tae.argc++;
        }
    }

    memcpy( &dev->als[ dev->alss ], &tae, sizeof(tae) );
    dev->alss++;

} /* end function autoload_tape_entry */


/*-------------------------------------------------------------------*/
/*             autoload_wait_for_tapemount_thread                    */
/*-------------------------------------------------------------------*/
void *autoload_wait_for_tapemount_thread(void *db)
{
int      rc   =  -1;
DEVBLK  *dev  =  (DEVBLK*) db;

    OBTAIN_DEVLOCK( dev );
    {
        while
        (
            dev->als
            &&
            (rc = autoload_mount_next( dev )) != 0
        )
        {
            RELEASE_DEVLOCK( dev );
            {
                SLEEP( AUTOLOAD_WAIT_FOR_TAPEMOUNT_INTERVAL_SECS );
            }
            OBTAIN_DEVLOCK( dev );
        }
    }
    RELEASE_DEVLOCK( dev );
    if ( rc == 0 )
        device_attention(dev,CSW_DE);
    return NULL;

} /* end function autoload_wait_for_tapemount_thread */


/*-------------------------------------------------------------------*/
/*                     autoload_mount_first                          */
/*-------------------------------------------------------------------*/
/*  mount in the drive the tape which is                             */
/*  positionned in the 1st autoloader slot                           */
/*-------------------------------------------------------------------*/
int autoload_mount_first(DEVBLK *dev)
{
    dev->alsix=0;
    return(autoload_mount_tape(dev,0));
}


/*-------------------------------------------------------------------*/
/*                     autoload_mount_next                           */
/*-------------------------------------------------------------------*/
/*  mount in the drive the tape whch is                              */
/*  positionned in the slot after the currently mounted tape.        */
/*  if this is the last tape, close the autoloader                   */
/*-------------------------------------------------------------------*/
int autoload_mount_next(DEVBLK *dev)
{
    if(dev->alsix>=dev->alss)
    {
        autoload_close(dev);
        return -1;
    }
    dev->alsix++;
    return(autoload_mount_tape(dev,dev->alsix));
}


/*-------------------------------------------------------------------*/
/*                     autoload_mount_tape                           */
/*-------------------------------------------------------------------*/
/*  mount in the drive the tape which is                             */
/*  positionned in the autoloader slot #alix                         */
/*-------------------------------------------------------------------*/
int autoload_mount_tape( DEVBLK *dev, int alix )
{
int     argc, i, rc;
char  **argv;

    if (alix >= dev->alss)
        return -1;

    argc = 1 + dev->al_argc + dev->als[ alix ].argc;
    argv = calloc( argc, sizeof(BYTE*) );
    if (!argv) return -1;
    argv[0] = dev->als[ alix ].filename;

    for (i=0, argc=1; i < dev->al_argc; i++, argc++)
    {
        argv[ argc ] = strdup( dev->al_argv[i] );
        if (!argv[ argc ]) break;
    }

    for (i=0; i < dev->als[ alix ].argc; i++, argc++)
    {
        argv[ argc ] = strdup( dev->als[ alix ].argv[i] );
        if (!argv[ argc ]) break;
    }

    rc = mountnewtape( dev, argc, argv );

    for (i=1; i < argc; i++)
        free( argv[i] );
    free( argv );

    return rc;

} /* end function autoload_mount_tape */


/*-------------------------------------------------------------------*/
/* is_tapeloaded_filename                                            */
/*-------------------------------------------------------------------*/
int is_tapeloaded_filename ( DEVBLK *dev, BYTE *unitstat, BYTE code )
{
    UNREFERENCED(unitstat);
    UNREFERENCED(code);
    // true 1 == tape loaded, false 0 == tape not loaded
    return strcmp( dev->filename, TAPE_UNLOADED ) != 0 ? 1 : 0;
}

/*-------------------------------------------------------------------*/
/* return_false1                                                     */
/*-------------------------------------------------------------------*/
int return_false1 ( DEVBLK *dev )
{
    UNREFERENCED(dev);
    return 0;
}

/*-------------------------------------------------------------------*/
/* write_READONLY                                                    */
/*-------------------------------------------------------------------*/
int write_READONLY ( DEVBLK *dev, BYTE *unitstat, BYTE code )
{
    build_senseX(TAPE_BSENSE_WRITEPROTECT,dev,unitstat,code);
    return -1;
}

/*-------------------------------------------------------------------*/
/* write_READONLY5                                                   */
/*-------------------------------------------------------------------*/
int write_READONLY5 ( DEVBLK *dev, const BYTE *bfr, U32 blklen, BYTE *unitstat, BYTE code )
{
    UNREFERENCED(bfr);
    UNREFERENCED(blklen);
    build_senseX(TAPE_BSENSE_WRITEPROTECT,dev,unitstat,code);
    return -1;
}

/*-------------------------------------------------------------------*/
/* no_operation                                                      */
/*-------------------------------------------------------------------*/
int no_operation ( DEVBLK *dev, BYTE *unitstat, BYTE code )
{
    build_senseX( TAPE_BSENSE_STATUSONLY, dev, unitstat, code );
    return 0;
}

/*-------------------------------------------------------------------*/
/* readblkid_virtual                                                 */
/*-------------------------------------------------------------------*/
int readblkid_virtual ( DEVBLK* dev, BYTE* logical, BYTE* physical )
{
    // NOTE: returned value is always in guest BIG-ENDIAN format...

    BYTE  blockid[4];

    if (0x3590 == dev->devtype)
    {
        // Full 32-bit block-id...

        blockid[0] = (dev->blockid >> 24) & 0xFF;
        blockid[1] = (dev->blockid >> 16) & 0xFF;
        blockid[2] = (dev->blockid >> 8 ) & 0xFF;
        blockid[3] = (dev->blockid      ) & 0xFF;
    }
    else // (3480 et. al)
    {
        // "22-bit" block-id...

        blockid[0] = 0x01;  // ("wrap" value)
        blockid[1] = (dev->blockid >> 16) & 0x3F;
        blockid[2] = (dev->blockid >> 8 ) & 0xFF;
        blockid[3] = (dev->blockid      ) & 0xFF;
    }

    // NOTE: For virtual tape devices, we return the same value
    // for both the logical "Channel block ID" value as well as
    // the physical "Device block ID" value...

    if (logical)  memcpy( logical,  &blockid[0], 4 );
    if (physical) memcpy( physical, &blockid[0], 4 );

    return 0;
}

/*-------------------------------------------------------------------*/
/* locateblk_virtual                                                 */
/*-------------------------------------------------------------------*/
int locateblk_virtual ( DEVBLK* dev, U32 blockid, BYTE *unitstat, BYTE code )
{
    // NOTE: 'blockid' passed in host (little-endian) format...

    int rc;
    bool is_het = (dev->tmh == &tmh_het);

    /* Do it the hard way: rewind to load-point and then
       keep doing fsb, fsb, fsb... until we find our block
    */
    if ((rc = dev->tmh->rewind( dev, unitstat, code)) >= 0)
    {
        /* Reset position counters to start of file */
        dev->curfilen   =  1;
        dev->nxtblkpos  =  0;
        dev->prvblkpos  = -1;
        dev->blockid    =  0;

        /* Keep doing fsb until we find our block... */
        if (!is_het)
        {
            while (dev->blockid < blockid && (rc >= 0))
                rc = dev->tmh->fsb( dev, unitstat, code );
        }
        else // (special handling for .HET files...)
        {
            while (1)
            {
                // PROGRAMMING NOTE: We need dev->lock to prevent
                // "dev->hetb" from going NULL while we're looping
                // (see GitHub Issue #515 for details), BUT... we
                // also need to also provide a brief window of
                // opportunity for I/O instructions to acquire
                // dev->lock themselves too if they need to, by
                // periodically releasing dev->lock while we loop.
                // Inefficient, yes, but it's the only sure way
                // to prevent GitHub Issue #518 from occurring.

                OBTAIN_DEVLOCK( dev );
                {
                    /* Check for exit condition */
                    if (0
                        || (rc < 0)                 // (error)
                        || !dev->hetb               // (ERROR!!)
                        || dev->blockid >= blockid  // (block located)
                    )
                    {
                        // 'hetb' SHOULDN'T suddenly disappear but
                        // apparently sometimes occassionally does!
                        // See GitHub Issue #515!

                        if (dev->blockid < blockid && !dev->hetb)
                            rc = -1; // (hetb disappeared!)

                        RELEASE_DEVLOCK( dev );
                        break; // (block located or error occurred)
                    }
                }
                RELEASE_DEVLOCK( dev );

                // Between the above release and the below obtain is
                // the window of opportunity for I/O instructions to
                // acquire dev->lock for themselves if they need to.

                OBTAIN_DEVLOCK( dev );
                {
                    if (!dev->hetb)
                        rc = -1;
                    else
                        rc = dev->tmh->fsb( dev, unitstat, code );
                }
                RELEASE_DEVLOCK( dev );

            } // end while
        } // end if special .HET file handling
    } // end if rewind success

    return rc;
}

/*-------------------------------------------------------------------*/
/* generic_tmhcall              generic media-type-handler call...   */
/*-------------------------------------------------------------------*/
int generic_tmhcall ( GENTMH_PARMS* pGenParms )
{
    if (!pGenParms)
    {
        errno = EINVAL;             // (invalid arguments)
        return -1;                  // (return failure)
    }

    switch (pGenParms->action)
    {
#if defined(OPTION_SCSI_TAPE)
        case GENTMH_SCSI_ACTION_UPDATE_STATUS:
        {
            return update_status_scsitape( pGenParms->dev );
        }
#endif /* defined(OPTION_SCSI_TAPE) */

        default:
        {
            errno = EINVAL;         // (invalid arguments)
            return -1;              // (return failure)
        }
    }

    UNREACHABLE_CODE( return -1 );
}
