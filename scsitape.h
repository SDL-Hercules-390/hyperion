/* SCSITAPE.H   (C) Copyright "Fish" (David B. Trout), 2005-2012     */
/*              Hercules SCSI tape handling module                   */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/* Original Author: "Fish" (David B. Trout)                          */
/* Prime Maintainer: "Fish" (David B. Trout)                         */
/* Secondary Maintainer: Ivan Warren                                 */

/*-------------------------------------------------------------------*/
/* This module only contains support for SCSI tapes. Please see the  */
/* 'tapedev.c' (and possibly other) source module(s) for information */
/* regarding support for other emulated tape/media formats.          */
/*-------------------------------------------------------------------*/

#ifndef _SCSITAPE_H_
#define _SCSITAPE_H_

#if defined(OPTION_SCSI_TAPE)

#include "tapedev.h"
#include "w32stape.h"
#include "scsiutil.h"

/*-------------------------------------------------------------------*/
/*                    External TMH-vector calls                      */
/*-------------------------------------------------------------------*/
extern int   update_status_scsitape   ( DEVBLK *dev ); // (via "generic" vector)
extern int   open_scsitape            ( DEVBLK *dev,                           BYTE *unitstat, BYTE code );
extern int   finish_scsitape_open     ( DEVBLK *dev,                           BYTE *unitstat, BYTE code );
extern void  close_scsitape           ( DEVBLK *dev );
extern int   read_scsitape            ( DEVBLK *dev,       BYTE *buf,          BYTE *unitstat, BYTE code );
extern int   write_scsitape           ( DEVBLK *dev, const BYTE *buf, U32 len, BYTE *unitstat, BYTE code );
extern int   rewind_scsitape          ( DEVBLK *dev,                           BYTE *unitstat, BYTE code );
extern int   bsb_scsitape             ( DEVBLK *dev,                           BYTE *unitstat, BYTE code );
extern int   fsb_scsitape             ( DEVBLK *dev,                           BYTE *unitstat, BYTE code );
extern int   bsf_scsitape             ( DEVBLK *dev,                           BYTE *unitstat, BYTE code );
extern int   fsf_scsitape             ( DEVBLK *dev,                           BYTE *unitstat, BYTE code );
extern int   write_scsimark           ( DEVBLK *dev,                           BYTE *unitstat, BYTE code );
extern int   sync_scsitape            ( DEVBLK *dev,                           BYTE *unitstat, BYTE code );
extern int   dse_scsitape             ( DEVBLK *dev,                           BYTE *unitstat, BYTE code );
extern int   erg_scsitape             ( DEVBLK *dev,                           BYTE *unitstat, BYTE code );
extern int   is_tape_mounted_scsitape ( DEVBLK *dev,                           BYTE *unitstat, BYTE code );
extern int   passedeot_scsitape       ( DEVBLK *dev );
extern int   readblkid_scsitape       ( DEVBLK* dev, BYTE* logical, BYTE* physical );
extern int   locateblk_scsitape       ( DEVBLK* dev, U32 blockid,              BYTE *unitstat, BYTE code );

/*-------------------------------------------------------------------*/
/*                     Internal functions                            */
/*-------------------------------------------------------------------*/
extern void  int_scsi_rewind_unload ( DEVBLK *dev, BYTE *unitstat, BYTE code );
extern void  int_scsi_status_update ( DEVBLK *dev, int mountstat_only );
extern int   int_scsi_status_mounted( DEVBLK *dev, int update_status );
extern int   int_write_scsimark     ( DEVBLK *dev );

extern void  blockid_emulated_to_actual( DEVBLK *dev, BYTE *emu_blkid, BYTE *act_blkid );
extern void  blockid_actual_to_emulated( DEVBLK *dev, BYTE *act_blkid, BYTE *emu_blkid );

extern void  blockid_32_to_22( BYTE *in_32blkid, BYTE *out_22blkid );
extern void  blockid_22_to_32( BYTE *in_22blkid, BYTE *out_32blkid );

extern void  create_automount_thread( DEVBLK* dev );
extern void *scsi_tapemountmon_thread( void* notused );
extern void  define_BOT_pos( DEVBLK *dev );

/*-------------------------------------------------------------------*/
/*                 SCSI drive status query timeout                   */
/*-------------------------------------------------------------------*/
/*                                                                   */
/*  PROGRAMMING NOTE: I'm not sure of what the the actual/proper     */
/*  value should be (or is) for the following value but I've coded   */
/*  what seems to me to be a reasonable value for it.                */
/*                                                                   */
/*  As you can probably guess based on its [rather verbose] name,    */
/*  it's the maximum amount of time that a SCSI tape drive status    */
/*  query should take (i.e. asking the system for the drive status   */
/*  should not, under normal circumstances, take any longer than     */
/*  the below defined amount of time, specified in microseconds).    */
/*                                                                   */
/*  It should be set to the most reasonable pessimistic value that   */
/*  we can tolerate, and should probably be at least as long as the  */
/*  host operating system's thread scheduling time-slice quantum.    */
/*                                                                   */
/*                                            --  Fish, April 2006   */
/*-------------------------------------------------------------------*/
/*                                                                   */
/*  August, 2006: further testing/experimentation has revealed that  */
/*  the "proper" value (i.e. one that causes the fewest potential    */
/*  problems) for the below timeout setting varies greatly from one  */
/*  system to another, with different host CPU speeds and different  */
/*  hardware (SCSI adapter cards, etc) being the largest factors.    */
/*                                                                   */
/*  Thus, in order to try and define it to a value likely to cause   */
/*  the LEAST number of problems on the largest number of systems,   */
/*  I am changing it from its original 25 millisecond value to the   */
/*  EXTREMELY PESSIMISTIC (but nonetheless safe(?), since it is      */
/*  afterall only a timeout setting) value of 250 milliseconds.      */
/*                                                                   */
/*  I've decided to do this because I happened to notice that on     */
/*  some systems (Windows) with moderate host workload, querying     */
/*  the status of the tape drive, while *usually* only taking 4-6    */
/*  milliseconds maximum, would sometimes take up to 113 or more     */
/*  milliseconds! (thus sometimes causing the guest to experience    */
/*  intermittent/sporadic unsolicited ATTN interrupts on the tape    */
/*  drive as their tape jobs ran (since "not mounted" status was     */
/*  thus getting set as a result of the status query taking longer   */
/*  than expected, causing the auto-mount thread to kick-in and      */
/*  then immediately exit again (with an ATTN interrupt of course)   */
/*  whenever it eventually noticed a tape was indeed still mounted   */
/*  on the drive)).                                                  */
/*                                                                   */
/*  Thus, even though such unsolicited ATTN interrupts occuring in   */
/*  the middle of (i.e. during) an already running tape job should   */
/*  NOT, under ordinary circumstances, cause any problems (as long   */
/*  as auto-scsi-mount is enabled of course), in order to reduce     */
/*  the chance of it happening, I am increasing the below timeout    */
/*  setting to a value that, ideally, *should* work on most *all*    */
/*  systems, even under the most pessimistic of host workloads.      */
/*                                                                   */
/*                                            -- Fish, August 2006.  */
/*-------------------------------------------------------------------*/

// ZZ FIXME: should we maybe make this a config file option??

#define MAX_NORMAL_SCSI_DRIVE_QUERY_RESPONSE_TIMEOUT_USECS  (250*1000)

#endif // defined( OPTION_SCSI_TAPE )

#endif // _SCSITAPE_H_
