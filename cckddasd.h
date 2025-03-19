/* CCKDDASD.H   (C) Copyright Roger Bowler, 1999-2012                */
/*              (C) Copyright Greg Smith, 2002-2012                  */
/*                                                                   */
/*              CCKD (Compressed CKD) Device Handler                 */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* This module contains device functions for compressed emulated     */
/* count-key-data direct access storage devices.                     */
/*-------------------------------------------------------------------*/

#ifndef _CCKDDASD_H_
#define _CCKDDASD_H_

//#define DEBUG_FREESPACE                 /* (freespace debugging opt) */

/*-------------------------------------------------------------------*/

#if defined( DEBUG_FREESPACE )
  #define CCKD_CHK_SPACE(_dev)      cckd_chk_space(_dev)  /* (debug) */
  void cckd_chk_space  ( DEVBLK* dev );
  void cckd64_chk_space( DEVBLK* dev );
#else
  #define CCKD_CHK_SPACE(_dev)      /* (do nothing) */
#endif

/*-------------------------------------------------------------------*/
/*                       Global Variables                            */
/*-------------------------------------------------------------------*/

CCKD_DLL_IMPORT  CCKDBLK  cckdblk;
CCKD_DLL_IMPORT  int      gctab[5];

extern char*         compname   [];
extern CCKD_L2ENT    empty_l2   [ CKD_NULLTRK_FMTMAX + 1 ][256];
extern CCKD64_L2ENT  empty64_l2 [ CKD_NULLTRK_FMTMAX + 1 ][256];

/*-------------------------------------------------------------------*/
/* Internal functions                                                */
/*-------------------------------------------------------------------*/
int     cckd_dasd_init(int argc, BYTE *argv[]);
void    cckd_dasd_term_if_appropriate();
/*-------------------------------------------------------------------*/
int     cckd_dasd_init_handler( DEVBLK *dev, int argc, char *argv[] );
int     cckd_dasd_close_device(DEVBLK *dev);
void    cckd_dasd_start(DEVBLK *dev);
void    cckd_dasd_end(DEVBLK *dev);
/*-------------------------------------------------------------------*/
int     cckd64_dasd_init_handler( DEVBLK *dev, int argc, char *argv[] );
int     cckd64_dasd_close_device(DEVBLK *dev);
void    cckd64_dasd_start(DEVBLK *dev);
void    cckd64_dasd_end(DEVBLK *dev);
/*-------------------------------------------------------------------*/
int     cckd_open (DEVBLK *dev, int sfx, int flags, mode_t mode);
int     cckd_close (DEVBLK *dev, int sfx);
int     cckd_read (DEVBLK *dev, int sfx, off_t off, void *buf, unsigned int len);
int     cckd_write (DEVBLK *dev, int sfx, off_t off, void *buf, unsigned int len);
int     cckd_ftruncate(DEVBLK *dev, int sfx, off_t off);
/*-------------------------------------------------------------------*/
int     cckd64_open (DEVBLK *dev, int sfx, int flags, mode_t mode);
int     cckd64_close (DEVBLK *dev, int sfx);
int     cckd64_read (DEVBLK *dev, int sfx, U64 off, void *buf, unsigned int len);
int     cckd64_write (DEVBLK *dev, int sfx, U64 off, void *buf, unsigned int len);
int     cckd64_ftruncate(DEVBLK *dev, int sfx, U64 off);
/*-------------------------------------------------------------------*/
void   *cckd_malloc(DEVBLK *dev, char *id, size_t size);
void   *cckd_calloc(DEVBLK *dev, char *id, size_t n, size_t size);
void   *cckd_realloc( DEVBLK *dev, char *id, void* p, size_t size );
void   *cckd_free(DEVBLK *dev, char *id,void *p);
int     cckd_read_track(DEVBLK *dev, int trk, BYTE *unitstat);
int     cckd_update_track(DEVBLK *dev, int trk, int off,
                         BYTE *buf, int len, BYTE *unitstat);
int     cckd_used(DEVBLK *dev);
/*-------------------------------------------------------------------*/
//id   *cckd64_free(DEVBLK *dev, char *id,void *p);
int     cckd64_read_track(DEVBLK *dev, int trk, BYTE *unitstat);
int     cckd64_update_track(DEVBLK *dev, int trk, int off,
                         BYTE *buf, int len, BYTE *unitstat);
int     cckd64_used(DEVBLK *dev);
/*-------------------------------------------------------------------*/
int     cfba_read_block(DEVBLK *dev, int blkgrp, BYTE *unitstat);
int     cfba_write_block(DEVBLK *dev, int blkgrp, int off,
                         BYTE *buf, int wrlen, BYTE *unitstat);
int     cfba_used(DEVBLK *dev);
/*-------------------------------------------------------------------*/
int     cfba64_read_block(DEVBLK *dev, int blkgrp, BYTE *unitstat);
int     cfba64_write_block(DEVBLK *dev, int blkgrp, int off,
                         BYTE *buf, int wrlen, BYTE *unitstat);
int     cfba64_used(DEVBLK *dev);
/*-------------------------------------------------------------------*/
int     cckd_read_trk(DEVBLK *dev, int trk, int ra, BYTE *unitstat);
void    cckd_readahead(DEVBLK *dev, int trk);
int     cckd_readahead_scan(int *answer, int ix, int i, void *data);
void*   cckd_ra(void* arg);
void    cckd_flush_cache(DEVBLK *dev);
int     cckd_flush_cache_scan(int *answer, int ix, int i, void *data);
void    cckd_flush_cache_all();
void    cckd_purge_cache(DEVBLK *dev);
int     cckd_purge_cache_scan(int *answer, int ix, int i, void *data);
void*   cckd_writer(void *arg);
int     cckd_writer_scan(int *o, int ix, int i, void *data);
void    cckd_writer_write( int writer, int o );
off_t   cckd_get_space(DEVBLK *dev, int *size, int flags);
void    cckd_rel_space(DEVBLK *dev, off_t pos, int len, int size);
void    cckd_flush_space(DEVBLK *dev);
int     cckd_read_chdr(DEVBLK *dev);
int     cckd_write_chdr(DEVBLK *dev);
int     cckd_read_l1(DEVBLK *dev);
int     cckd_write_l1(DEVBLK *dev);
int     cckd_write_l1ent(DEVBLK *dev, int L1idx);
int     cckd_read_init(DEVBLK *dev);
int     cckd_read_fsp(DEVBLK *dev);
int     cckd_write_fsp(DEVBLK *dev);
int     cckd_read_l2(DEVBLK *dev, int sfx, int L1idx);
void    cckd_purge_l2(DEVBLK *dev);
int     cckd_purge_l2_scan(int *answer, int ix, int i, void *data);
int     cckd_steal_l2();
int     cckd_steal_l2_scan(int *answer, int ix, int i, void *data);
int     cckd_write_l2(DEVBLK *dev);
int     cckd_read_l2ent(DEVBLK *dev, CCKD_L2ENT *l2, int trk);
int     cckd_write_l2ent(DEVBLK *dev,   CCKD_L2ENT *l2, int trk);
int     cckd_read_trkimg(DEVBLK *dev, BYTE *buf, int trk, BYTE *unitstat);
int     cckd_write_trkimg(DEVBLK *dev, BYTE *buf, int len, int trk, int flags);
int     cckd_harden(DEVBLK *dev);
int     cckd_trklen(DEVBLK *dev, BYTE *buf);
int     cckd_null_trk(DEVBLK *dev, BYTE *buf, int trk, int nullfmt);
int     cckd_check_null_trk (DEVBLK *dev, BYTE *buf, int trk, int len);
int     cckd_cchh(DEVBLK *dev, BYTE *buf, int trk);
int     cckd_validate(DEVBLK *dev, BYTE *buf, int trk, int len);
void    cckd_sf_parse_sfn( DEVBLK* dev, char* sfn );
char   *cckd_sf_name(DEVBLK *dev, int sfx);
int     cckd_sf_init(DEVBLK *dev);
int     cckd_sf_new(DEVBLK *dev);
void    cckd_lock_devchain(int flag);
void    cckd_unlock_devchain();
void    cckd_dhstart(int by_cmdline);
void*   cckd_dh(void* arg);
void    cckd_gcstart();
void*   cckd_gcol(void* arg);
void    cckd_gcol_dev( DEVBLK* dev, struct timeval* tv_now );
int     cckd_gc_state( DEVBLK* dev );
void    cckd_gc_rpt_state( DEVBLK* dev );
int     cckd_gc_percolate( DEVBLK* dev, U64 size );
int     cckd_gc_l2(DEVBLK *dev, BYTE *buf);
DEVBLK *cckd_find_device_by_devnum (U16 devnum);
/*-------------------------------------------------------------------*/
int     cckd64_read_trk(DEVBLK *dev, int trk, int ra, BYTE *unitstat);
//id    cckd64_readahead(DEVBLK *dev, int trk);
//t     cckd64_readahead_scan(int *answer, int ix, int i, void *data);
//id*   cckd64_ra(void* arg);
void    cckd64_flush_cache(DEVBLK *dev);
int     cckd64_flush_cache_scan(int *answer, int ix, int i, void *data);
void    cckd64_flush_cache_all();
void    cckd64_purge_cache(DEVBLK *dev);
int     cckd64_purge_cache_scan(int *answer, int ix, int i, void *data);
//id*   cckd64_writer(void *arg);
//t     cckd64_writer_scan(int *o, int ix, int i, void *data);
void    cckd64_writer_write( int writer, int o );
S64     cckd64_get_space(DEVBLK *dev, int *size, int flags);
void    cckd64_rel_space(DEVBLK *dev, U64 pos, int len, int size);
void    cckd64_flush_space(DEVBLK *dev);
int     cckd64_read_chdr(DEVBLK *dev);
int     cckd64_write_chdr(DEVBLK *dev);
int     cckd64_read_l1(DEVBLK *dev);
int     cckd64_write_l1(DEVBLK *dev);
int     cckd64_write_l1ent(DEVBLK *dev, int L1idx);
int     cckd64_read_init(DEVBLK *dev);
int     cckd64_read_fsp(DEVBLK *dev);
int     cckd64_write_fsp(DEVBLK *dev);
int     cckd64_read_l2(DEVBLK *dev, int sfx, int L1idx);
void    cckd64_purge_l2(DEVBLK *dev);
int     cckd64_purge_l2_scan(int *answer, int ix, int i, void *data);
int     cckd64_steal_l2();
//t     cckd64_steal_l2_scan(int *answer, int ix, int i, void *data);
int     cckd64_write_l2(DEVBLK *dev);
int     cckd64_read_l2ent(DEVBLK *dev, CCKD64_L2ENT *l2, int trk);
int     cckd64_write_l2ent(DEVBLK *dev,   CCKD64_L2ENT *l2, int trk);
int     cckd64_read_trkimg(DEVBLK *dev, BYTE *buf, int trk, BYTE *unitstat);
int     cckd64_write_trkimg(DEVBLK *dev, BYTE *buf, int len, int trk, int flags);
int     cckd64_harden(DEVBLK *dev);
//t     cckd64_trklen(DEVBLK *dev, BYTE *buf);
int     cckd64_null_trk(DEVBLK *dev, BYTE *buf, int trk, int nullfmt);
int     cckd64_check_null_trk (DEVBLK *dev, BYTE *buf, int trk, int len);
int     cckd64_cchh(DEVBLK *dev, BYTE *buf, int trk);
int     cckd64_validate(DEVBLK *dev, BYTE *buf, int trk, int len);
//id    cckd64_sf_parse_sfn( DEVBLK* dev, char* sfn );
//ar   *cckd64_sf_name(DEVBLK *dev, int sfx);
int     cckd64_sf_init(DEVBLK *dev);
int     cckd64_sf_new(DEVBLK *dev);
//id    cckd64_lock_devchain(int flag);
//id    cckd64_unlock_devchain();
void    cckd64_gcstart();
//id*   cckd64_gcol(void* arg);
void    cckd64_gcol_dev( DEVBLK* dev, struct timeval* tv_now );
int     cckd64_gc_state( DEVBLK* dev );
void    cckd64_gc_rpt_state( DEVBLK* dev );
int     cckd64_gc_percolate( DEVBLK* dev, U64 size );
int     cckd64_gc_l2(DEVBLK *dev, BYTE *buf);
//VBLK *cckd64_find_device_by_devnum (U16 devnum);
/*-------------------------------------------------------------------*/
BYTE   *cckd_uncompress(DEVBLK *dev, BYTE *from, int len, int maxlen, int trk);
int     cckd_uncompress_zlib(DEVBLK *dev, BYTE *to, BYTE *from, int len, int maxlen);
int     cckd_uncompress_bzip2(DEVBLK *dev, BYTE *to, BYTE *from, int len, int maxlen);
int     cckd_compress(DEVBLK *dev, BYTE **to, BYTE *from, int len, int comp, int parm);
int     cckd_compress_none(DEVBLK *dev, BYTE **to, BYTE *from, int len, int parm);
int     cckd_compress_zlib(DEVBLK *dev, BYTE **to, BYTE *from, int len, int parm);
int     cckd_compress_bzip2(DEVBLK *dev, BYTE **to, BYTE *from, int len, int parm);
/*-------------------------------------------------------------------*/
BYTE   *cckd64_uncompress(DEVBLK *dev, BYTE *from, int len, int maxlen, int trk);
//t     cckd64_uncompress_zlib(DEVBLK *dev, BYTE *to, BYTE *from, int len, int maxlen);
//t     cckd64_uncompress_bzip2(DEVBLK *dev, BYTE *to, BYTE *from, int len, int maxlen);
//t     cckd64_compress(DEVBLK *dev, BYTE **to, BYTE *from, int len, int comp, int parm);
//t     cckd64_compress_none(DEVBLK *dev, BYTE **to, BYTE *from, int len, int parm);
//t     cckd64_compress_zlib(DEVBLK *dev, BYTE **to, BYTE *from, int len, int parm);
//t     cckd64_compress_bzip2(DEVBLK *dev, BYTE **to, BYTE *from, int len, int parm);
/*-------------------------------------------------------------------*/
CCKD_DLL_IMPORT   int     cckd_command(char *op, int cmd);
                  void    cckd_command_help();
                  void    cckd_command_opts();
                  void    cckd_command_stats();
                  void    cckd_trace( const char* func, int line,
                                      DEVBLK* dev, char* fmt, ...);
CCKD_DLL_IMPORT   void    cckd_print_itrace();
CCKD_DLL_IMPORT   bool    cckd_dtax(); // Dump Table At Exit
/*-------------------------------------------------------------------*/
//KD64_DLL_IMPORT int     cckd64_command(char *op, int cmd);
//                void    cckd64_command_help();
//                void    cckd64_command_opts();
//                void    cckd64_command_stats();
//                void    cckd64_trace(DEVBLK *dev, char *msg, ...);
//KD64_DLL_IMPORT void    cckd64_print_itrace();
/*-------------------------------------------------------------------*/
CCKD_DLL_IMPORT   void   *cckd_sf_add(void *data);
CCKD_DLL_IMPORT   void   *cckd_sf_remove(void *data);
CCKD_DLL_IMPORT   void   *cckd_sf_comp(void *data);
CCKD_DLL_IMPORT   void   *cckd_sf_chk(void *data);
CCKD_DLL_IMPORT   void   *cckd_sf_stats(void *data);
CCKD_DLL_IMPORT   void    cckd_gc_rpt_states();
/*-------------------------------------------------------------------*/
CCKD64_DLL_IMPORT void   *cckd64_sf_add(void *data);
CCKD64_DLL_IMPORT void   *cckd64_sf_remove(void *data);
CCKD64_DLL_IMPORT void   *cckd64_sf_comp(void *data);
CCKD64_DLL_IMPORT void   *cckd64_sf_chk(void *data);
CCKD64_DLL_IMPORT void   *cckd64_sf_stats(void *data);

#endif // _CCKDDASD_H_
