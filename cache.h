/* CACHE.H    (c)Copyright Greg Smith, 2002-2012                     */
/*            Buffer Cache Manager                                   */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------
  Description:

    Manages multiple caches in a multi-threaded environment.  A cache
    is dynamically created and destroyed.  It's size or number of
    entries is also dynamically determined.  A cache entry contains
    an identifying `key', `flags' which indicate whether an entry is
    busy or not, and a `buf' which is a pointer to the cached object.

  Cache entry:

    The structure of a cache entry is:

      U64       key;
      U32       flag;
      int       len;
      void     *buf;
      int       value;
      U64       age;

    The first 8 bits of the flag indicates if the entry is `busy' or
    not.  If any of the first 8 bits are non-zero then the entry is
    considered `busy' and will not be stolen or otherwise reused.

  APIs:

    General query functions:

      int         cache_nbr(int ix); [0]
                  Number of entries

      int         cache_busy(int ix);
                  Number of busy entries

      int         cache_empty(int ix);
                  Number of empty entries [1]

      int         cache_waiters(int ix);
                  Number of waiters for a non-busy cache entry

      S64         cache_size(int ix);
                  Size of all allocated objects

      S64         cache_hits(int ix);
                  Number of successful lookups

      S64         cache_misses(int ix);
                  Number of unsuccessful lookups

      int         cache_busy_percent(int ix);
                  Percentage (0 .. 100) of entries that are busy

      int         cache_empty_percent(int ix);
                  Percentage of entries that are empty [1]

      int         cache_hit_percent(int ix);
                  Percentage of successful lookups to total lookups

     Notes        [0] `ix' identifies the cache.  This is an integer
                      and is reserved in `cache.h'
                  [1] An empty entry contains a zero key value.
                      A valid key should not be all zeroes
                      (0x0000000000000000) or all ones
                      (0xffffffffffffffff).   All ones is used to
                      indicate an error circumstance.

    Entry specific functions:

      U64         cache_getkey(int ix, int i); [0]
                  Return key for the specified cache entry

      U64         cache_setkey(int ix, int i, U64 key);
                  Set the key for the specified cache entry;
                  the old key is returned

      U32         cache_getflag(int ix, int i);
                  Return the flag for the specified cache entry

      U32         cache_setflag(int ix, int i, U32 andbits, U32 orbits);
                  Set the flag for the specified cache entry; first
                  the `andbits' value is `and'ed against the entry then
                  the `orbits' value is `or'ed against the entry.  The
                  old flag is returned.

      U64         cache_getage(int ix, int i); [1]
                  Return age for the specified cache entry

      U64         cache_setage(int ix, int i);
                  Set age for the specified cache entry

      void       *cache_getbuf(int ix, int i, int len);
                  Return address of the object buf for the cache entry.
                  If `len' is non-zero, then if the current object
                  is null or `len' is greater than the current object
                  length then the old object is freed and a new
                  object is obtained.

      void       *cache_setbuf(int ix, int i, void *buf, int len);
                  The old object address and length is replaced.
                  The address of the old object is returned and
                  can be freed using `free()'.

      int         cache_getlen(int ix, int i);
                  Return the length of the current object

       Notes      [0] `i' is the index of the entry in cache `ix'
                  [1] `age' is a sequentially incremented value and
                      does not correspond to date or time

    Locking functions:

      int         cache_lock(int ix);
                  Obtain the lock for cache `ix'.  If the cache does
                  not exist then it will be created.  Generally, the
                  lock should be obtained when referencing cache
                  entries and must be held when a cache entry status
                  may change from `busy' to `not busy' or vice versa.
                  Likewise, the lock must be held when a cache entry
                  changes from `empty' to `not empty' or vice versa.

      int         cache_unlock(int ix);
                  Release the cache lock

    Search functions:

      int         cache_lookup(int ix, U64 key, int *o);
                  Search cache `ix' for entry matching `key'.
                  If a non-NULL pointer `o' is provided, then the
                  oldest or preferred cache entry index is returned
                  that is available to be stolen.

      int         cache_scan (int ix, int (rtn)(), void *data);
                  Scan a cache routine entry by entry calling routine
                  `rtn'.  Parameters passed to the routine are
                  `(int *answer, int ix, int i, void *data)'  where
                  `ix' is the cache index, `i' is the cache entry
                  index and `data' is the value passed to cache_scan.
                  `*answer' is initialized to -1 and can be set by
                  the scan subroutine.  This will be the value returned
                  by cache_scan.  If the routine returns a non-zero
                  value then the scan is terminated.

    Other functions:

      int         cache_wait(int ix);
                  Wait for a non-busy cache entry to become available.
                  Typically called after `cache_lookup' was
                  unsuccessful and `*o' is -1.

      int         cache_release(int ix, int i, int flag);
                  Release the cache entry.  If flag is CACHE_FREEBUF
                  then the object buffer is also freed.

  -------------------------------------------------------------------*/

#ifndef _HERCULES_CACHE_H
#define _HERCULES_CACHE_H 1

#include "hercules.h"

/*-------------------------------------------------------------------*/
/* Reserve cache indexes here                                        */
/*-------------------------------------------------------------------*/
#define  CACHE_MAX_INDEX              8 /* Max number caches [0..7]  */

#define  CACHE_DEVBUF                 0 /* Device Buffer cache       */
#define  CACHE_L2                     1 /* L2 cache                  */
#define  CACHE_2                      2 /*      (available)          */
#define  CACHE_3                      3 /*      (available)          */
#define  CACHE_4                      4 /*      (available)          */
#define  CACHE_5                      5 /*      (available)          */
#define  CACHE_6                      6 /*      (available)          */
#define  CACHE_7                      7 /*      (available)          */

/*-------------------------------------------------------------------*/
/* Cache entry                                                       */
/*-------------------------------------------------------------------*/
typedef struct _CACHE {                 /* Cache entry               */
      U64       key;                    /* Key                       */
      U32       flag;                   /* Flags                     */
      int       len;                    /* Buffer length             */
      void     *buf;                    /* Buffer address            */
      int       value;                  /* Arbitrary value           */
      U64       age;                    /* Age                       */
    } CACHE;

/*-------------------------------------------------------------------*/
/* Cache header                                                      */
/*-------------------------------------------------------------------*/
typedef struct _CACHEBLK {              /* Cache header              */
      int       magic;                  /* Magic number              */
      int       nbr;                    /* Number entries            */
      int       busy;                   /* Number busy entries       */
      int       empty;                  /* Number empty entries      */
      int       waiters;                /* Number waiters            */
      int       waits;                  /* Number times waited       */
      S64       size;                   /* Allocated buffer size     */
      S64       hits;                   /* Number lookup hits        */
      S64       fasthits;               /* Number fast lookup hits   */
      S64       misses;                 /* Number lookup misses      */
      U64       age;                    /* Age counter               */
      LOCK      lock;                   /* Lock                      */
      COND      waitcond;               /* Wait for available entry  */
      CACHE    *cache;                  /* Cache table address       */
      time_t    atime;                  /* Time last adjustment      */
      time_t    wtime;                  /* Time last wait            */
      int       adjusts;                /* Number of adjustments     */
    } CACHEBLK;

/*-------------------------------------------------------------------*/
/* Flag definitions                                                  */
/*-------------------------------------------------------------------*/
#define CACHE_BUSY           0xFF000000 /* Busy bits                 */
#define CACHE_TYPE           0x000000FF /* Type bits                 */

#define CACHE_FREEBUF                 1 /* Free buf on release       */

#define CACHE_MAGIC          0x01CACE10 /* Magic number              */
#define CACHE_DEFAULT_NBR           229 /* Initial entries (prime)   */
//FIXME the line below increases the size for CACHE_L2.  Since each
//      cckd device always has an active l2 entry this number
//      actually limits the number of cckd devices that can be
//      attached.
//      This is a workaround to increase the max number of devices
#define CACHE_DEFAULT_L2_NBR       1031 /* Initial entries for L2    */

#define CACHE_WAITTIME             1000 /* Wait time for entry(usec) */

#define CACHE_ADJUST_INTERVAL        15 /* Adjustment interval (sec) */
#define CACHE_ADJUST_NUMBER         128 /* Uninhibited nbr entries   */
#define CACHE_ADJUST_BUSY1           70 /* Increase when this busy 1 */
#define CACHE_ADJUST_BUSY2           80 /* Increase when this busy 2 */
#define CACHE_ADJUST_RESIZE           8 /* Nbr entries adjusted      */
#define CACHE_ADJUST_EMPTY           16 /* Decrease this many empty  */
#define CACHE_ADJUST_HIT1            60 /* Increase hit% this low  1 */
#define CACHE_ADJUST_HIT2            50 /* Increase hit% this low  2 */
#define CACHE_ADJUST_BUSY3           20 /* Decrease not this busy    */
#define CACHE_ADJUST_HIT3            90 /*      and hit% this high   */
#define CACHE_ADJUST_SIZE  (8*1024*1024)/*      and size this high   */
#define CACHE_ADJUST_WAITTIME        10 /* Increase last wait  (sec) */

/*-------------------------------------------------------------------*/
/* Functions                                                         */
/*-------------------------------------------------------------------*/
CCH_DLL_IMPORT int cachestats_cmd(int argc, char *argv[], char *cmdline);

int         cache_nbr(int ix);
int         cache_busy(int ix);
int         cache_empty(int ix);
int         cache_waiters(int ix);
S64         cache_size(int ix);
S64         cache_hits(int ix);
S64         cache_misses(int ix);
int         cache_busy_percent(int ix);
int         cache_empty_percent(int ix);
int         cache_hit_percent(int ix);
int         cache_lookup(int ix, U64 key, int *o);
typedef int CACHE_SCAN_RTN (int *answer, int ix, int i, void *data);
int         cache_scan (int ix, CACHE_SCAN_RTN rtn, void *data);
int         cache_lock(int ix);
int         cache_unlock(int ix);
int         cache_wait(int ix);
U64         cache_getkey(int ix, int i);
U64         cache_setkey(int ix, int i, U64 key);
U32         cache_getflag(int ix, int i);
U32         cache_setflag(int ix, int i, U32 andbits, U32 orbits);
U64         cache_getage(int ix, int i);
U64         cache_setage(int ix, int i);
void       *cache_getbuf(int ix, int i, int len);
void       *cache_setbuf(int ix, int i, void *buf, int len);
int         cache_getlen(int ix, int i);
int         cache_getval(int ix, int i);
int         cache_setval(int ix, int i, int val);
int         cache_release(int ix, int i, int flag);

/*-------------------------------------------------------------------*/
/* Specific cache definitions (until a better place is found)        */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* Device buffer definitions                                         */
/*-------------------------------------------------------------------*/

#define   CCKD_CACHE_ACTIVE  0x80000000 /* Active entry              */
#define   CCKD_CACHE_READING 0x40000000 /* Entry being read          */
#define   CCKD_CACHE_WRITING 0x20000000 /* Entry being written       */
#define   CCKD_CACHE_IOBUSY  (CCKD_CACHE_READING|CCKD_CACHE_WRITING)
#define   CCKD_CACHE_IOWAIT  0x10000000 /* Waiters for i/o           */
#define   CCKD_CACHE_UPDATED 0x08000000 /* Buffer has been updated   */
#define   CCKD_CACHE_WRITE   0x04000000 /* Entry pending write       */
#define   CCKD_CACHE_USED    0x00800000 /* Entry has been used       */

#define   CKD_CACHE_ACTIVE   0x80000000 /* Active entry              */
#define   FBA_CACHE_ACTIVE   0x80000000 /* Active entry              */
#define   SHRD_CACHE_ACTIVE  0x80000000 /* Active entry              */

#define   DEVBUF_TYPE_SHARED 0x00000080 /* Shared entry type         */
#define   DEVBUF_TYPE_COMP   0x00000040 /* CCKD/CFBA entry type      */
#define   DEVBUF_TYPE_CKD    0x00000002 /* CKD entry type            */
#define   DEVBUF_TYPE_FBA    0x00000001 /* FBA entry type            */

#define   DEVBUF_TYPE_CCKD    (DEVBUF_TYPE_COMP|DEVBUF_TYPE_CKD)
#define   DEVBUF_TYPE_CFBA    (DEVBUF_TYPE_COMP|DEVBUF_TYPE_FBA)
#define   DEVBUF_TYPE_SCKD    (DEVBUF_TYPE_SHARED|DEVBUF_TYPE_CKD)
#define   DEVBUF_TYPE_SFBA    (DEVBUF_TYPE_SHARED|DEVBUF_TYPE_FBA)

/*-------------------------------------------------------------------*/

#define CCKD_CACHE_GETKEY(_ix, _devnum, _trk) \
do { \
  (_devnum) = (U16)((cache_getkey(CACHE_DEVBUF,(_ix)) >> 32) & 0xFFFF    ); \
  (_trk)    = (U32)((cache_getkey(CACHE_DEVBUF,(_ix))      ) & 0xFFFFFFFF); \
} while (0)

#define CKD_CACHE_GETKEY(_ix, _devnum, _trk) \
{ \
  (_devnum) = (U16)((cache_getkey(CACHE_DEVBUF,(_ix)) >> 32) & 0xFFFF   ); \
  (_trk)    = (U32)((cache_getkey(CACHE_DEVBUF,(_ix))      )& 0xFFFFFFFF); \
}

#define FBA_CACHE_GETKEY(_ix, _devnum, _blkgrp) \
{ \
  (_devnum) = (U16)((cache_getkey(CACHE_DEVBUF,(_ix)) >> 32) & 0xFFFF    ); \
  (_blkgrp) = (U32)((cache_getkey(CACHE_DEVBUF,(_ix))      ) & 0xFFFFFFFF); \
}

#define SHRD_CACHE_GETKEY(_ix, _devnum, _trk) \
{ \
  (_devnum) = (U16)((cache_getkey(CACHE_DEVBUF,(_ix)) >> 32) & 0xFFFF    ); \
  (_trk)    = (U32)((cache_getkey(CACHE_DEVBUF,(_ix))      ) & 0xFFFFFFFF); \
}

/*-------------------------------------------------------------------*/

#define CCKD_CACHE_SETKEY(_devnum, _trk) \
  ((U64)(((U64)(_devnum) << 32) | (U64)(_trk)))

#define CKD_CACHE_SETKEY(_devnum, _trk) \
  ((U64)(((U64)(_devnum) << 32) | (U64)(_trk)))

#define FBA_CACHE_SETKEY(_devnum, _blkgrp) \
  ((U64)(((U64)(_devnum) << 32) | (U64)(_blkgrp)))

#define SHRD_CACHE_SETKEY(_devnum, _trk) \
  ((U64)(((U64)(_devnum) << 32) | (U64)(_trk)))

/*-------------------------------------------------------------------*/
/* L2 definitions                                                    */
/*-------------------------------------------------------------------*/

#define   L2_CACHE_ACTIVE    0x80000000 /* Active entry              */

#define L2_CACHE_GETKEY(_ix, _sfx, _devnum, _trk) \
do { \
  (_sfx)    = (U16)((cache_getkey(CACHE_L2,(_ix)) >> 48) & 0xFFFF    ); \
  (_devnum) = (U16)((cache_getkey(CACHE_L2,(_ix)) >> 32) & 0xFFFF    ); \
  (_trk)    = (U32)((cache_getkey(CACHE_L2,(_ix))      ) & 0xFFFFFFFF); \
} while (0)

#define L2_CACHE_SETKEY(_sfx, _devnum, _trk) \
  ((U64)(((U64)(_sfx) << 48) | ((U64)(_devnum) << 32) | (U64)(_trk)))

#endif /* _HERCULES_CACHE_H */
