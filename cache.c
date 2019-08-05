/* CACHE.C    (C) Copyright Greg Smith, 2002-2012                    */
/*            Dynamic cache manager for multi-threaded applications  */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

//FIXME ?? Dynamic resizing is disabled

#include "hstdinc.h"

#define _CACHE_C_
#define _HDASD_DLL_

#include "hercules.h"

static int  cache_create (int ix);
static int  cache_destroy (int ix);
static int  cache_check_ix(int ix);
static int  cache_check_cache(int ix);
static int  cache_check(int ix, int i);
static int  cache_isbusy(int ix, int i);
static int  cache_isempty(int ix, int i);
static void cache_allocbuf(int ix, int i, int len);

DISABLE_GCC_UNUSED_FUNCTION_WARNING;

/*-------------------------------------------------------------------*/
/* The master cache blocks array controlled by sysblk.dasdcache_lock */
/*-------------------------------------------------------------------*/
static CACHEBLK  cacheblk[ CACHE_MAX_INDEX ] = {0};

#define OBTAIN_GLOBAL_CACHE_LOCK()   obtain_lock(  &sysblk.dasdcache_lock )
#define RELEASE_GLOBAL_CACHE_LOCK()  release_lock( &sysblk.dasdcache_lock )

/*-------------------------------------------------------------------*/
/* Public functions                                                  */
/*-------------------------------------------------------------------*/

int cache_nbr (int ix)
{
    if (cache_check_ix(ix)) return -1;
    return cacheblk[ix].nbr;
}

int cache_busy (int ix)
{
    if (cache_check_ix(ix)) return -1;
    return cacheblk[ix].busy;
}

int cache_empty (int ix)
{
    if (cache_check_ix(ix)) return -1;
    return cacheblk[ix].empty;
}

int cache_waiters (int ix)
{
    if (cache_check_ix(ix)) return -1;
    return cacheblk[ix].waiters;
}

S64 cache_size (int ix)
{
    if (cache_check_ix(ix)) return -1;
    return cacheblk[ix].size;
}

S64 cache_hits (int ix)
{
    if (cache_check_ix(ix)) return -1;
    return cacheblk[ix].hits;
}

S64 cache_misses (int ix)
{
    if (cache_check_ix(ix)) return -1;
    return cacheblk[ix].misses;
}

int cache_busy_percent (int ix)
{
    if (cache_check_ix(ix)) return -1;
    return (cacheblk[ix].busy * 100) / cacheblk[ix].nbr;
}

int cache_empty_percent (int ix)
{
    if (cache_check_ix(ix)) return -1;
    return (cacheblk[ix].empty * 100) / cacheblk[ix].nbr;
}

int cache_hit_percent (int ix)
{
    S64 total;
    if (cache_check_ix(ix)) return -1;
    total = cacheblk[ix].hits + cacheblk[ix].misses;
    if (total == 0) return -1;
    return (int)((cacheblk[ix].hits * 100) / total);
}

int cache_lookup (int ix, U64 key, int *oldest_entry)
{
    int i,p;

    if (oldest_entry)
        *oldest_entry = -1;
    if (cache_check_ix(ix))
        return -1;
    /* `p' is the preferred index */
    p = (int)(key % cacheblk[ix].nbr);

    if (cacheblk[ix].cache[p].key == key)
    {
        i = p;
        if (!cache_isempty(ix, i))
        {
            cacheblk[ix].fasthits++;
        }
    }
    else
    {
        if (cache_isbusy(ix, p) || cacheblk[ix].age - cacheblk[ix].cache[p].age < 20)
            p = -2;
        for (i = 0; i < cacheblk[ix].nbr; i++)
        {
            if (cacheblk[ix].cache[i].key == key) break;
            if (oldest_entry && !cache_isbusy(ix, i)
             && (*oldest_entry < 0 || i == p || cacheblk[ix].cache[i].age < cacheblk[ix].cache[*oldest_entry].age))
                if (*oldest_entry != p) *oldest_entry = i;
        }
    }

    if (i >= cacheblk[ix].nbr)
    {
        i = -1;
    }
    else if ( i >= 0 && cache_isempty(ix, i) )
    {
        if ( oldest_entry && *oldest_entry < 0 )
        {
            *oldest_entry = i;
        }
        i = -1;
    }
    if ( i >= 0 )
    {
        cacheblk[ix].hits++;
    }
    else
    {
        cacheblk[ix].misses++;
    }
    return i;
}

int cache_scan (int ix, CACHE_SCAN_RTN rtn, void *data)
{
int      i;                             /* Cache index               */
int      rc;                            /* Return code               */
int      answer = -1;                   /* Answer from routine       */

    if (cache_check_ix(ix)) return -1;
    for (i = 0; i < cacheblk[ix].nbr; i++) {
        rc = (rtn)(&answer, ix, i, data);
        if (rc != 0) break;
    }
    return answer;
}

int cache_lock(int ix)
{
    if (cache_check_cache(ix)) return -1;
    obtain_lock(&cacheblk[ix].lock);
    return 0;
}

int cache_unlock(int ix)
{
    if (cache_check_ix(ix)) return -1;
    release_lock(&cacheblk[ix].lock);
    if (cacheblk[ix].empty == cacheblk[ix].nbr)
        cache_destroy(ix);
    return 0;
}

int cache_wait(int ix)
{
    if (cache_check_ix(ix)) return -1;
    if (cacheblk[ix].busy < cacheblk[ix].nbr)
        return 0;

    cacheblk[ix].waiters++; cacheblk[ix].waits++;

#if FALSE
    {
    struct timeval  now;
    struct timespec tm;
        gettimeofday (&now, NULL);
        tm.tv_sec = now.tv_sec;
        tm.tv_nsec = (now.tv_usec + CACHE_WAITTIME) * 1000;
        tm.tv_sec += tm.tv_nsec / 1000000000;
        tm.tv_nsec = tm.tv_nsec % 1000000000;
        timed_wait_condition(&cacheblk[ix].waitcond, &cacheblk[ix].lock, &tm);
    }
#else
    wait_condition(&cacheblk[ix].waitcond, &cacheblk[ix].lock);
#endif
    cacheblk[ix].waiters--;
    return 0;
}

U64 cache_getkey(int ix, int i)
{
    if (cache_check(ix,i)) return (U64)-1;
    return cacheblk[ix].cache[i].key;
}

U64 cache_setkey(int ix, int i, U64 key)
{
    U64 oldkey;
    int empty;

    if (cache_check(ix,i)) return (U64)-1;
    empty = cache_isempty(ix, i);
    oldkey = cacheblk[ix].cache[i].key;
    cacheblk[ix].cache[i].key = key;
    if (empty && !cache_isempty(ix, i))
        cacheblk[ix].empty--;
    else if (!empty && cache_isempty(ix, i))
        cacheblk[ix].empty++;
    return oldkey;
}

U32 cache_getflag(int ix, int i)
{
    if (cache_check(ix,i)) return (U32)-1;
    return cacheblk[ix].cache[i].flag;
}

U32 cache_setflag(int ix, int i, U32 andbits, U32 orbits)
{
    U32 oldflags;
    int empty;
    int busy;

    if (cache_check(ix,i)) return (U32)-1;

    empty = cache_isempty(ix, i);
    busy = cache_isbusy(ix, i);
    oldflags = cacheblk[ix].cache[i].flag;

    cacheblk[ix].cache[i].flag &= andbits;
    cacheblk[ix].cache[i].flag |= orbits;

    if (!cache_isbusy(ix, i) && cacheblk[ix].waiters > 0)
        signal_condition(&cacheblk[ix].waitcond);
    if (busy && !cache_isbusy(ix, i))
        cacheblk[ix].busy--;
    else if (!busy && cache_isbusy(ix, i))
        cacheblk[ix].busy++;
    if (empty && !cache_isempty(ix, i))
        cacheblk[ix].empty--;
    else if (!empty && cache_isempty(ix, i))
        cacheblk[ix].empty++;
    return oldflags;
}

U64 cache_getage(int ix, int i)
{
    if (cache_check(ix,i)) return (U64)-1;
    return cacheblk[ix].cache[i].age;
}

U64 cache_setage(int ix, int i)
{
    U64 oldage;
    int empty;

    if (cache_check(ix,i)) return (U64)-1;
    empty = cache_isempty(ix, i);
    oldage = cacheblk[ix].cache[i].age;
    cacheblk[ix].cache[i].age = ++cacheblk[ix].age;
    if (empty) cacheblk[ix].empty--;
    return oldage;
}

void *cache_getbuf(int ix, int i, int len)
{
    if (cache_check(ix,i)) return NULL;
    if (len > 0
     && cacheblk[ix].cache[i].buf != NULL
     && cacheblk[ix].cache[i].len < len) {
        cacheblk[ix].size -= cacheblk[ix].cache[i].len;
        free (cacheblk[ix].cache[i].buf);
        cacheblk[ix].cache[i].buf = NULL;
        cacheblk[ix].cache[i].len = 0;
    }
    if (cacheblk[ix].cache[i].buf == NULL && len > 0)
        cache_allocbuf (ix, i, len);
    return cacheblk[ix].cache[i].buf;
}

void *cache_setbuf(int ix, int i, void *buf, int len)
{
    void *oldbuf;
    if (cache_check(ix,i)) return NULL;
    oldbuf = cacheblk[ix].cache[i].buf;
    cacheblk[ix].size -= cacheblk[ix].cache[i].len;
    cacheblk[ix].cache[i].buf = buf;
    cacheblk[ix].cache[i].len = len;
    cacheblk[ix].size += len;
    return oldbuf;
}

int cache_getlen(int ix, int i)
{
    if (cache_check(ix,i)) return -1;
    return cacheblk[ix].cache[i].len;
}

int cache_getval(int ix, int i)
{
    if (cache_check(ix,i)) return -1;
    return cacheblk[ix].cache[i].value;
}

int cache_setval(int ix, int i, int val)
{
    int rc;
    if (cache_check(ix,i)) return -1;
    rc = cacheblk[ix].cache[i].value;
    cacheblk[ix].cache[i].value = val;
    return rc;
}

int cache_release(int ix, int i, int flag)
{
    void *buf;
    int   len;
    int   empty;
    int   busy;

    if (cache_check(ix,i)) return -1;

    empty = cache_isempty(ix, i);
    busy = cache_isbusy(ix, i);

    buf = cacheblk[ix].cache[i].buf;
    len = cacheblk[ix].cache[i].len;

    memset(&cacheblk[ix].cache[i], 0, sizeof(CACHE));

    if ((flag & CACHE_FREEBUF) && buf != NULL) {
        free (buf);
        cacheblk[ix].size -= len;
        buf = NULL;
        len = 0;
    }

    cacheblk[ix].cache[i].buf = buf;
    cacheblk[ix].cache[i].len = len;

    if (cacheblk[ix].waiters > 0)
        signal_condition(&cacheblk[ix].waitcond);

    if (!empty) cacheblk[ix].empty++;
    if (busy) cacheblk[ix].busy--;

    return 0;
}

DLL_EXPORT int cachestats_cmd(int argc, char *argv[], char *cmdline)
{
    int ix, i;
    char buf[128];

    UNREFERENCED(cmdline);
    UNREFERENCED(argc);
    UNREFERENCED(argv);

    OBTAIN_GLOBAL_CACHE_LOCK();

    for (ix = 0; ix < CACHE_MAX_INDEX; ix++)
    {
        if (cacheblk[ix].magic != CACHE_MAGIC)
        {
            MSGBUF(buf, "Cache[%d] ....... not created", ix);
            WRMSG(HHC02294, "I", buf);
            continue;
        }

        MSGBUF( buf, "Cache............ %10d", ix);
        WRMSG(HHC02294, "I", buf);

        MSGBUF( buf, "nbr ............. %10d", cacheblk[ix].nbr);
        WRMSG(HHC02294, "I", buf);

        MSGBUF( buf, "busy ............ %10d", cacheblk[ix].busy);
        WRMSG(HHC02294, "I", buf);

        MSGBUF( buf, "busy%% ........... %10d",cache_busy_percent(ix));
        WRMSG(HHC02294, "I", buf);

        MSGBUF( buf, "empty ........... %10d", cacheblk[ix].empty);
        WRMSG(HHC02294, "I", buf);

        MSGBUF( buf, "waiters ......... %10d", cacheblk[ix].waiters);
        WRMSG(HHC02294, "I", buf);

        MSGBUF( buf, "waits ........... %10d", cacheblk[ix].waits);
        WRMSG(HHC02294, "I", buf);

        MSGBUF( buf, "buf size ........ %10"PRId64, cacheblk[ix].size);
        WRMSG(HHC02294, "I", buf);

        MSGBUF( buf, "hits ............ %10"PRId64, cacheblk[ix].hits);
        WRMSG(HHC02294, "I", buf);

        MSGBUF( buf, "fast hits ....... %10"PRId64, cacheblk[ix].fasthits);
        WRMSG(HHC02294, "I", buf);

        MSGBUF( buf, "misses .......... %10"PRId64, cacheblk[ix].misses);
        WRMSG(HHC02294, "I", buf);

        MSGBUF( buf, "hit%% ............ %10d", cache_hit_percent(ix));
        WRMSG(HHC02294, "I", buf);

        MSGBUF( buf, "age ............. %10"PRId64, cacheblk[ix].age);
        WRMSG(HHC02294, "I", buf);

        MSGBUF( buf, "last adjusted ... %s", cacheblk[ix].atime == 0 ? "none\n" : ctime(&cacheblk[ix].atime));
        buf[strlen(buf)-1] = '\0';
        WRMSG(HHC02294, "I", buf);

        MSGBUF( buf, "last wait ....... %s", cacheblk[ix].wtime == 0 ? "none\n" : ctime(&cacheblk[ix].wtime));
        buf[strlen(buf)-1] = '\0';
        WRMSG(HHC02294, "I", buf);

        MSGBUF( buf, "adjustments ..... %10d", cacheblk[ix].adjusts);
        WRMSG(HHC02294, "I", buf);

        if (argc > 1)
        {
            for (i = 0; i < cacheblk[ix].nbr; i++)
            {
                MSGBUF( buf, "[%4d] %16.16"PRIx64" %8.8x %10p %6d %10"PRId64,
                    i, cacheblk[ix].cache[i].key, cacheblk[ix].cache[i].flag,
                    cacheblk[ix].cache[i].buf, cacheblk[ix].cache[i].len,
                    cacheblk[ix].cache[i].age);
                WRMSG(HHC02294, "I", buf);
            }
        }
    }

    RELEASE_GLOBAL_CACHE_LOCK();
    return 0;
}

/*-------------------------------------------------------------------*/
/* Private functions                                                 */
/*-------------------------------------------------------------------*/
static int cache_destroy_locked (int ix)
{
    int i;
    if (cacheblk[ix].magic == CACHE_MAGIC)
    {
        destroy_lock (&cacheblk[ix].lock);
        destroy_condition (&cacheblk[ix].waitcond);

        if (cacheblk[ix].cache)
        {
            for (i = 0; i < cacheblk[ix].nbr; i++)
                cache_release(ix, i, CACHE_FREEBUF);

            free (cacheblk[ix].cache);
        }
    }
    memset(&cacheblk[ix], 0, sizeof(CACHEBLK));
    return 0;
}

static int cache_create_locked( int ix )
{
    cache_destroy_locked (ix);
    cacheblk[ix].magic = CACHE_MAGIC;

    // FIXME: See the note in cache.h about CACHE_DEFAULT_L2_NBR

    cacheblk[ix].nbr = ix != CACHE_L2 ? CACHE_DEFAULT_NBR
                                      : CACHE_DEFAULT_L2_NBR;

    cacheblk[ix].empty = cacheblk[ix].nbr;

    initialize_lock (&cacheblk[ix].lock);
    initialize_condition (&cacheblk[ix].waitcond);

    cacheblk[ix].cache = calloc (cacheblk[ix].nbr, sizeof(CACHE));

    if (cacheblk[ix].cache == NULL)
    {
        // "Function %s failed; cache %d size %d: [%02d] %s"
        WRMSG (HHC00011, "E", "cache()", ix,
            (int)(cacheblk[ix].nbr * (int)sizeof(CACHE)),
            errno, strerror(errno));
        return -1;
    }
    return 0;
}

static int cache_destroy (int ix)
{
    int rc;
    OBTAIN_GLOBAL_CACHE_LOCK();
    {
        rc = cache_destroy_locked(ix);
    }
    RELEASE_GLOBAL_CACHE_LOCK();
    return rc;
}

static int cache_create (int ix)
{
    int rc;
    OBTAIN_GLOBAL_CACHE_LOCK();
    {
        rc = cache_create_locked(ix);
    }
    RELEASE_GLOBAL_CACHE_LOCK();
    return rc;
}

static int cache_check_ix(int ix)
{
    if (ix < 0 || ix >= CACHE_MAX_INDEX) return -1;
    return 0;
}

static int cache_check_cache(int ix)
{
    int rc;
    OBTAIN_GLOBAL_CACHE_LOCK();
    {
        rc = cache_check_ix(ix) ||
            (cacheblk[ix].magic != CACHE_MAGIC && cache_create_locked(ix));
    }
    RELEASE_GLOBAL_CACHE_LOCK();
    return rc;
}

static int cache_check(int ix, int i)
{
    if (cache_check_ix(ix) || i < 0 || i >= cacheblk[ix].nbr)
        return -1;
    return 0;
}

static int cache_isbusy(int ix, int i)
{
    return ((cacheblk[ix].cache[i].flag & CACHE_BUSY) != 0);
}

static int cache_isempty(int ix, int i)
{
    return (cacheblk[ix].cache[i].key  == 0
         && cacheblk[ix].cache[i].flag == 0
         && cacheblk[ix].cache[i].age  == 0);
}

static void cache_allocbuf(int ix, int i, int len)
{
    cacheblk[ix].cache[i].buf = calloc (len, 1);
    if (cacheblk[ix].cache[i].buf == NULL) {
        WRMSG (HHC00011, "E", "calloc()", ix, len, errno, strerror(errno));
        WRMSG (HHC00012, "W");
        for (i = 0; i < cacheblk[ix].nbr; i++)
            if (!cache_isbusy(ix, i)) cache_release(ix, i, CACHE_FREEBUF);
        cacheblk[ix].cache[i].buf = calloc (len, 1);
        if (cacheblk[ix].cache[i].buf == NULL) {
            WRMSG (HHC00011, "E", "calloc()", ix, len, errno, strerror(errno));
            return;
        }
    }
    cacheblk[ix].cache[i].len = len;
    cacheblk[ix].size += len;
}
