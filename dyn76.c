/* DYN76.C      (C) Copyright Harold Grovesteen, 2010-2012           */
/*              Diagnose Instr for file transfers                    */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/* DYN76   (C) Copyright Jason Paul Winter, 2010                      */
/*             See license_dyn76.txt for licensing                    */

#include "hstdinc.h"

#if !defined(_HENGINE_DLL_)
#define _HENGINE_DLL_
#endif /*_HENGINE_DLL_*/

#if !defined(_DYN76_C)
#define _DYN76_C
#endif /* !defined(_DYN76_C) */

#include "hercules.h"
#include "opcode.h"
#include "inline.h"
#include "hdiagf18.h"

#if !defined(_DYN76_H)
#define _DYN76_H


/*-------------------------------------------------------------------*/
/* Internal macro definitions                                        */
/*-------------------------------------------------------------------*/

/* These macros allow retrieving and setting of register contents from/to
   the Compatibility Parameter Block, which is in essence a R0-R15 register
   save area.  These macros ensure storage keys and accesses are checked for
   the parameter block.
*/

#define set_reg(_r, _v) \
    ARCH_DEP(vstore4)((U32)(_v), (VADR)cmpb+(_r * 4), USE_REAL_ADDR, regs)
#define get_reg(_v, _r) \
    _v = ARCH_DEP(vfetch4)((VADR)(cmpb+(_r * 4)), USE_REAL_ADDR, regs)

/* Compile with debugging */
//#define DYN76_DEBUG
//#define DYN76_DEBUG_FKEEPER

/* Keep track of open files, a utility can then be used to close them if needed (fn3) */
struct fkeeper
{
    struct fkeeper *next; /* May or may not end up in the global list */
    U32  SaveArea;        /* Space to save a work register */
    U32  id;              /* Used to identify this fkeeper to the guest */
    int  handle;          /* Host file system handle */
    int  mode;            /* Text/Binary-Mode */
    int  data;            /* filenames index and for read/write */
    char oldname  [260];
    char filename [260];  /* Also used for 256 byte read/write buffer */
};

/* File Keeper lists.  New entries are added at the top of the list
   Because both lists are treated a single shared resource they utilize
   the same lock, nfile_lock, to control CPU (thread) access */

static struct fkeeper *fkpr_head = NULL;  /* Open file status list */
static struct fkeeper *rst_head = NULL;   /* Restart list */

static unsigned char DCCascii_to_ebcdic[] =
{
    "\x00\x01\x02\x03\x37\x2D\x2E\x2F\x16\x05\x15\x0B\x0C\x0D\x0E\x0F"
    "\x10\x11\x12\x13\x3C\x3D\x32\x26\x18\x19\x3F\x27\x1C\x1D\x1E\x1F"
    "\x40\x5A\x7F\x7B\x5B\x6C\x50\x7D\x4D\x5D\x5C\x4E\x6B\x60\x4B\x61"
    "\xF0\xF1\xF2\xF3\xF4\xF5\xF6\xF7\xF8\xF9\x7A\x5E\x4C\x7E\x6E\x6F"
    "\x7C\xC1\xC2\xC3\xC4\xC5\xC6\xC7\xC8\xC9\xD1\xD2\xD3\xD4\xD5\xD6"
    "\xD7\xD8\xD9\xE2\xE3\xE4\xE5\xE6\xE7\xE8\xE9\xAD\xE0\xBD\x5F\x6D"
    "\x79\x81\x82\x83\x84\x85\x86\x87\x88\x89\x91\x92\x93\x94\x95\x96"
    "\x97\x98\x99\xA2\xA3\xA4\xA5\xA6\xA7\xA8\xA9\xC0\x4F\xD0\xA1\x07"
    "\x20\x21\x22\x23\x24\x25\x06\x17\x28\x29\x2A\x2B\x2C\x09\x0A\x1B"
    "\x30\x31\x1A\x33\x34\x35\x36\x08\x38\x39\x3A\x3B\x04\x14\x3E\xFF"
    "\x41\xAA\x4A\xB1\x9F\xB2\x6A\xB5\xBB\xB4\x9A\x8A\xB0\xCA\xAF\xBC"
    "\x90\x8F\xEA\xFA\xBE\xA0\xB6\xB3\x9D\xDA\x9B\x8B\xB7\xB8\xB9\xAB"
    "\x64\x65\x62\x66\x63\x67\x9E\x68\x74\x71\x72\x73\x78\x75\x76\x77"
    "\xAC\x69\xED\xEE\xEB\xEF\xEC\xBF\x80\xFD\xFE\xFB\xFC\xBA\xAE\x59"
    "\x44\x45\x42\x46\x43\x47\x9C\x48\x54\x51\x52\x53\x58\x55\x56\x57"
    "\x8C\x49\xCD\xCE\xCB\xCF\xCC\xE1\x70\xDD\xDE\xDB\xDC\x8D\x8E\xDF"
};

static unsigned char DCCebcdic_to_ascii[] =
{
    "\x00\x01\x02\x03\x9C\x09\x86\x7F\x97\x8D\x8E\x0B\x0C\x0D\x0E\x0F"
    "\x10\x11\x12\x13\x9D\x0A\x08\x87\x18\x19\x92\x8F\x1C\x1D\x1E\x1F"
    "\x80\x81\x82\x83\x84\x85\x17\x1B\x88\x89\x8A\x8B\x8C\x05\x06\x07"
    "\x90\x91\x16\x93\x94\x95\x96\x04\x98\x99\x9A\x9B\x14\x15\x9E\x1A"
    "\x20\xA0\xE2\xE4\xE0\xE1\xE3\xE5\xE7\xF1\xA2\x2E\x3C\x28\x2B\x7C"
    "\x26\xE9\xEA\xEB\xE8\xED\xEE\xEF\xEC\xDF\x21\x24\x2A\x29\x3B\x5E"
    "\x2D\x2F\xC2\xC4\xC0\xC1\xC3\xC5\xC7\xD1\xA6\x2C\x25\x5F\x3E\x3F"
    "\xF8\xC9\xCA\xCB\xC8\xCD\xCE\xCF\xCC\x60\x3A\x23\x40\x27\x3D\x22"
    "\xD8\x61\x62\x63\x64\x65\x66\x67\x68\x69\xAB\xBB\xF0\xFD\xFE\xB1"
    "\xB0\x6A\x6B\x6C\x6D\x6E\x6F\x70\x71\x72\xAA\xBA\xE6\xB8\xC6\xA4"
    "\xB5\x7E\x73\x74\x75\x76\x77\x78\x79\x7A\xA1\xBF\xD0\x5B\xDE\xAE"
    "\xAC\xA3\xA5\xB7\xA9\xA7\xB6\xBC\xBD\xBE\xDD\xA8\xAF\x5D\xB4\xD7"
    "\x7B\x41\x42\x43\x44\x45\x46\x47\x48\x49\xAD\xF4\xF6\xF2\xF3\xF5"
    "\x7D\x4A\x4B\x4C\x4D\x4E\x4F\x50\x51\x52\xB9\xFB\xFC\xF9\xFA\xFF"
    "\x5C\xF7\x53\x54\x55\x56\x57\x58\x59\x5A\xB2\xD4\xD6\xD2\xD3\xD5"
    "\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\xB3\xDB\xDC\xD9\xDA\x9F"
};

static void StrConverter (char * s, unsigned char * cct)
{
    int i = 0;
    while (s [i])
    {
        s [i] = (signed char)cct [(unsigned char)s [i]];
        i++;
    }
}

static void MemConverter (char * s, unsigned char * cct, int len)
{
    int i = 0;
    while (i < len)
    {
        s [i] = (signed char)cct [(unsigned char)s [i]];
        i++;
    }
}

#ifdef _MSVC_

/* When using MSVC, just use the normal critical section functions: */

#undef initialize_lock
#undef LOCK
#undef obtain_lock
#undef release_lock

#define initialize_lock InitializeCriticalSection
#define LOCK CRITICAL_SECTION
#define obtain_lock EnterCriticalSection
#define release_lock LeaveCriticalSection

#endif

/* File keeper list controls */
static LOCK nfile_lock;
static long nfile_init_req = 1;
static U32 nfile_id = 0;
static U32 restart_id = 0;
#define DO_FREE 1
#define NO_FREE 0

static void nfile_init ()
{
    initialize_lock(&nfile_lock);
}
#define dolock(l) obtain_lock (&l); /* This ensures we use the correct fn */
#define unlock(l) release_lock (&l);

#ifdef _MSVC_
#include <io.h>
#define _open w32_hopen
#else
#include <fcntl.h>
#include <unistd.h>

#define _open hopen
#define _read read
#define _write write
#define _lseek lseek
#define _close close

#define _O_TEXT   0x04
#define _O_BINARY 0x08

#define _O_WRONLY O_WRONLY
#define _O_RDWR   O_RDWR
#define _O_RDONLY O_RDONLY
#define _O_APPEND O_APPEND
#define _O_TRUNC  O_TRUNC
#define _O_CREAT  O_CREAT
#define _O_EXCL   O_EXCL
#endif

/* WARNING: This function MUST be called with nfile_lock already locked */
static void AddFKByID (U32 id, struct fkeeper *item, struct fkeeper **list)
{
    /* struct fkeeper * head = list; */
    item->id = id;
    item->next = *list;
    *list = item;
#ifdef DYN76_DEBUG_FKEEPER
    logmsg("DF18: AddFKByID added id=%d at %X with next item at %X to list head=%X at %X\n",
           item->id, item, item->next, *list, list);
#endif
}

static struct fkeeper * FindFK (U32 id, struct fkeeper **list)
{
    struct fkeeper *fk;
#ifdef DYN76_DEBUG_FKEEPER
    logmsg("DF18: FindFK id=%d in list head at=%X\n", id, list);
#endif
    dolock (nfile_lock); /* Take ownership of the list */

    fk = *list;          /* Search the list */

#ifdef DYN76_DEBUG_FKEEPER
    logmsg("DF18: FindFK list head points to first entry at %X\n", fk);
#endif
    while (fk)
    {
#ifdef DYN76_DEBUG_FKEEPER
        logmsg("DF18: FindFK id=%d (0x%X) is at: %X\n", fk->id, fk->id, fk);
#endif
        if (fk->id == id)
        {   /* Found the entry */
            unlock (nfile_lock);
            return fk;
        }
        fk = fk->next;
    }
    unlock (nfile_lock); /* Release ownership of the list */
    /* Reached the end of the list without finding the id */
    return 0;
}

static void RemoveFKByID (U32 id, struct fkeeper **list, int free_entry)
{
    struct fkeeper *pfk;
    struct fkeeper *fk;
#ifdef DYN76_DEBUG_FKEEPER
    logmsg("DF18: RemoveFKByID id %d from list head at=%X\n", id, list);
#endif
    dolock (nfile_lock); /* Take ownership of the list */
    pfk = NULL;
    fk = *list;
    while (fk)
    {
#ifdef DYN76_DEBUG_FKEEPER
        logmsg("DF18: RemoveFKByID id=%d (0x%X) is at: %X\n", fk->id, fk->id, fk);
#endif
        if (fk->id == id)
        {   /* Found the entry */
            if (pfk) /* Wasn't the head entry */
            {
                pfk->next = fk->next;
            }
            else     /* Was the head entry */
            {
                *list = fk->next;
            }
            if (free_entry)
            {
#ifdef DYN76_DEBUG_FKEEPER
                logmsg("DF18: RemoveFKByID freeing id=%d at: %X\n", fk->id, fk);
#endif
                free (fk);
            }
            break;
        }
        pfk = fk;
        fk = fk->next;
    }
    unlock (nfile_lock); /* Release ownership of the list */
}

static int RemoveFKByName (char * filename)
{
    int i = -1 * EBADF;
    struct fkeeper *pfk;
    struct fkeeper *fk;

    dolock (nfile_lock); /* Take ownership of the list */
    pfk = NULL;
    fk = fkpr_head; /* Search the list */
    while (fk)
    {
        if (strcmp (fk->filename, filename) == 0)
        {   /* Found the entry! */
#ifdef DYN76_DEBUG
        logmsg("DF18: CFILE - orphan _close(%d)\n", fk->handle);
#endif
            i = _close (fk->handle); /* Additional step here is to close the file too */
#ifdef DYN76_DEBUG
            logmsg("DF18: CFILE - orphan _close result: %d\n", i);
#endif
            if (i == -1)
            {
                i = errno;
#ifdef DYN76_DEBUG
            logmsg("DF18: CFILE - orphan _close errno: %d\n", i);
#endif
                i = -1 * i;
            }
            else
            {
                if (pfk) /* Wasn't the head entry */
                {
                    pfk->next = fk->next;
                }
                else     /* Was the head entry */
                {
                    fkpr_head = fk->next;
                }
                free (fk);
            }
            break;
        }
        pfk = fk;
        fk = fk->next;
    }
    unlock (nfile_lock); /* Release ownership of the list */
    return (i);
}

#endif /* !defined(_DYN76_H) */
/*-------------------------------------------------------------------*/
/* 76xx NFL  - NFILE Ra,yyy(Rb,Rc)  Ra=0,yyy=0,Rb=0,Rc=R2            */
/*-------------------------------------------------------------------*/

/*
  R0  = Set to 0 prior to call,
        but due to possible instruction restart, >=0 on return
  R1  = Function number:
        0/ int rename (char * oldname - char * newname);
               -special 1 parm points to dual nul-term-strings
        1/ int unlink (char * filename);
        2/ int open   (char * filename, int oflg, int pmode);
        3/ int close  (char * filename);
               -special version to allow abended pgm cleanups "by hand"
        4/ int read   (char * buf, int h, int count);
        5/ int write  (char * buf, int h, int count);
        6/ int seek   (int h, int offset, int origin);
        7/ int commit (int h);
        8/ int close  (int h);
        9/ int setmode (int h, int mode);
  R2..R4 = Parameters, depending on function number
           All may be destroyed on return
  R5  = Work structure pointer (not set prior to call and safely restored on return)
  R15 = Return Code (0 = ok/handle, +ve = handle, -ve = errno)
*/

void ARCH_DEP(hdiagf18_FC) (U32 options, VADR cmpb, REGS *regs)
{
    int    space_ctl;           /* This is used to control address space selection */
    int    i;
    int    res;                 /* I/O integer results */
    int    handle = 0;          /* Host file file handle for this file */
    U32    ghandle = 0;         /* Guest file descriptor */
    struct fkeeper *fk = NULL;  /* Host file structure */
    struct fkeeper *rfk = NULL; /* Restart structure */
#ifdef DYN76_DEBUG
    int    io_error = 0;        /* Saves the errno for log messages */
#endif

    /* Pseudo register contents are cached here once retrieved */
    U32    R0;
    U32    R1;
    U32    R2;
    U32    R3;
    U32    R4;
    U32    R5;
    U32    R15;

    /* Initialise the LOCK on our first use */
    if (nfile_init_req)
    {
        nfile_init_req = 0;
        nfile_init ();
    }

#ifdef DYN76_DEBUG
    logmsg("DF18: CFILE Validating FOCPB Address %X\n", cmpb);
#endif

    /* CPB must be on a doubleword and must not cross physical page boundary */
    if ( ((cmpb & 0x7) != 0 ) ||
         (((cmpb + 63) & STORAGE_KEY_PAGEMASK) != (cmpb & STORAGE_KEY_PAGEMASK)) )
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);

#ifdef DYN76_DEBUG
    logmsg("DF18: CFILE Validated FOCPB Address\n");
#endif

    get_reg(R1, 1);     /* Retrieve the function number */
    if (R1 > 9)
    {   /* Invalid Function - generate an exception */
        R0 = 0;
        set_reg(0,R0);  /* don't restart this */
        ARCH_DEP(program_interrupt) (regs, PGM_OPERAND_EXCEPTION);
    }

    get_reg(R2, 2);  /* All functions use parameter 1, so fetch it */

    /* read, write, seek, commit, close and setmode use the file descriptor */
    if (R1 >= 4 && R1 <= 9)
    {
        if (R1 == 4 || R1 == 5)
        {   /* For read and write guest file descriptor is in pseudo R3 */
            get_reg(R3, 3);
            ghandle = R3;
        }
        else
        {   /* For seek, commit, close and setmode, file descriptor is in
               previously fetched pseudo R2 */
            ghandle = R2;
        }

        /* If read, write, seek, commit, close or setmode */
        /* convert the file descriptor into a file handle */
#ifdef DYN76_DEBUG_FKEEPER
        logmsg("DF18: CFILE - looking for guest file descriptor %d\n", ghandle);
#endif
        fk = FindFK(ghandle, &fkpr_head);
#ifdef DYN76_DEBUG_FKEEPER
        logmsg("DF18: CFILE - guest file descriptor %d found at %X\n",
               ghandle, fk);
#endif
        if (!fk)
        {   /* Did not find the guest file descriptor.
               Treat it as if it were a bad host file handle */
#ifdef DYN76_DEBUG
            logmsg("DF18: CFILE - guest file descriptor not found: %d\n", ghandle);
#endif
            R15 = -1 * EBADF;
            set_reg(15,R15);
            R0 = 0;
            set_reg(0,R0); /* No restart on a failure */
            return;
        }
        handle = fk->handle;  /* All host file accesses use this variable */
#ifdef DYN76_DEBUG_FKEEPER
        logmsg("DF18: CFILE - host file handle: %d\n", handle);
#endif
    }

/* The following 4 functions are always ready to attempt
   and they are not interruptible: CLOSE, COMMIT, SEEK, SETMODE
*/

    /*------------------------*/
    /* SETMODE File Operation */
    /*------------------------*/

    if (R1 == 9)
    {
#ifdef DYN76_DEBUG
        logmsg("DF18: CFILE - setmode file operation\n");
#endif
        get_reg(R3, 3);   /* Fetch parameter two - new file mode */
        if (R3 & 0x04)
            i = _O_TEXT;
        else
            i = _O_BINARY;
#ifdef _MSVC_
        i = _setmode (handle, i); /* Alter the Windows mode */
        if (i == -1)
        {
            R15 = -errno;
            set_reg(15,R15);
            return;
        }
#else
        get_reg(R3, 3);
        /* *nix doesn't need handle updates */
        if (fk->mode)
            i = _O_TEXT;
        else
            i = _O_BINARY;
#endif
        if (i == _O_TEXT) /* Previous mode */
            R15 = 0x04;
        else
            R15 = 0x08;
        set_reg(15,R15);

        if (R3 & 0x04) /* Update translation details: */
            fk->mode = 1;    /* yes, translate */
        else
            fk->mode = 0;    /* no, dont */
        return;
    } else

    /*------------------------*/
    /* CLOSE File Operation   */
    /*------------------------*/

    if (R1 == 8)
    {
#ifdef DYN76_DEBUG
        logmsg("DF18: CFILE - _close(%d)\n", handle);
#endif
        res = _close (handle);
#ifdef DYN76_DEBUG
        logmsg("DF18: CFILE - _close result: %d\n", res);
#endif
        R15 = res;
        set_reg(15,R15);
        if (R15 == 0)
        {   RemoveFKByID (fk->id, &fkpr_head, DO_FREE);
        }
        else
        {   R15 = -errno;
            set_reg(15,R15);
        }
        return;
    } else

    /*------------------------*/
    /* COMMIT File Operation  */
    /*------------------------*/

    if (R1 == 7)
    {
#ifdef DYN76_DEBUG
        logmsg("DF18: CFILE - commit file operation\n");
#endif

#ifdef _MSVC_
#ifdef DYN76_DEBUG
        logmsg("DF18: CFILE - _commit(%d)\n", handle);
#endif
        res =  _commit (handle);
#ifdef DYN76_DEBUG
        logmsg("DF18: CFILE - _commit result: %d\n", res);
#endif
#else /* ifdef __MSVC__ */

#ifdef DYN76_DEBUG
        logmsg("DF18: CFILE - fsync(%d)\n", handle);
#endif
        res = fsync (handle);
#ifdef DYN76_DEBUG
        logmsg("DF18: CFILE - fsync result: %d \n", res);
#endif
#endif /* ifdef __MSVC__ */
        if (res != 0)
        {
            res = errno;
#ifdef DYN76_DEBUG
        logmsg("DF18: CFILE - fsync/_commit errno: %d \n", res);
#endif
            R15 = -1 * res;
        }
        else
        {
            R15 = res;
        }
        set_reg(15,R15);
        return;
    } else

    /*------------------------*/
    /* SEEK File Operation    */
    /*------------------------*/

    if (R1 == 6)
    {
#ifdef DYN76_DEBUG
        logmsg("DF18: CFILE - seek\n");
#endif
        get_reg(R3,3);  /* offset in bytes */
        get_reg(R4,4);  /* origin of the seek */
#ifdef DYN76_DEBUG
        logmsg("DF18: CFILE - _lseek(%d, %d, %d)\n", handle, (long)R3, (int)R4);
#endif
        res = _lseek (handle, (long)R3, (int)R4);
#ifdef DYN76_DEBUG
        logmsg("DF18: CFILE - _lseek result: %d\n", res);
#endif
        if (res == -1)
        {
            res = errno;
#ifdef DYN76_DEBUG
            logmsg("DF18: CFILE - _lseek errno: %d\n", res);
#endif
            R15 = -1 * res;
        }
        else
        {
            R15 = res;
        }
        set_reg(15,R15);
        return;
    }

    /*-------------------------------------------------------------*/
    /* Interruptible operation initilization                       */
    /*-------------------------------------------------------------*/

    if ( (options & SPACE_MASK) == DF18_REAL_SPACE)
        space_ctl = USE_REAL_ADDR;
    else
        space_ctl = USE_PRIMARY_SPACE;

    get_reg(R0,0);        /* Retrieve Restart Stage */

    if (R0 == 0)
    {   /* New operation, not a restart.  Establish new operational state */
        rfk = malloc (sizeof (struct fkeeper));
        if (rfk == NULL)
        {
            R15 = -1 * ENOMEM; /* Error */
            set_reg(15,R15);
            return;
        }
        rfk->mode = -1;      /* mode is not initially set */
        rfk->data = 0;       /* Nothing in the buffer yet */
        rfk->handle = 0;     /* No file handle yet either (could be an open) */
        get_reg(R5,5);       /* Save pseudo R5 */
        rfk->SaveArea = R5;

        /* Set this state's id for the guest and increment
           and link the structure to the list even if not yet open.
           This is required to allow open to be restarted based upon the
           id.  If the file does not successfully open, then the linked entry
           must be removed in stage 3
        */
        dolock (nfile_lock);    /* Take ownership of the list */

        R5 = restart_id++;        /* safely increment the id counter */
#ifdef DYN76_DEBUG_FKEEPER
        logmsg("DF18: CFILE - adding restart fkeeper to rst_head list\n");
#endif
        AddFKByID(R5, rfk, &rst_head);

        unlock (nfile_lock);    /* Release ownership of the list */
        set_reg(5,R5);          /* Set the restart state for this new operation */

        /* For read/write we need this cleared here */
        R15 = 0;
        set_reg(15,R15);

        /* Set the restart stage in case the next stage is interrupted */
        R0 = 1;                /* Set work stage 1 on restart */
        set_reg(0,R0);
    } else

    /*-------------------------------------------------------------*/
    /* Interruptible operation restart                             */
    /*-------------------------------------------------------------*/

    {   /* Must have restarted, refresh fk */
        get_reg(R5,5);    /* R5 contains the previous restart id */
        rfk = FindFK(R5, &rst_head);
        if (!rfk)
        {   /* Did not find the restart - generate an exception */
            R0 = 0;
            set_reg(0,R0); /* restart failed, so don't do it again */
            ARCH_DEP(program_interrupt) (regs, PGM_SPECIAL_OPERATION_EXCEPTION);
        }
    }

    /*-------------------------------------------------------------*/
    /* Work Stage 1 Initial or Restart                             */
    /*-------------------------------------------------------------*/

    if (R0 == 1)
    {
        if (R1 == 0)
        { /* rename - need to get 2 filenames */
            while ((rfk->data == 0) ||                    /* Started? */
                   (rfk->oldname [rfk->data - 1] != 0))
            {   /* Not Finished */

                /* WARNING: This is where interruption might occur */
                ARCH_DEP(wfetchc)
                    (&(rfk->oldname [rfk->data]),
                     0,
                     R2,
                     space_ctl,
                     regs);

                /* Exception, can recalculate if/when restart */
                R2 += 1;
                set_reg(2,R2);
                rfk->data += 1;  /* Next host byte location */
                if (rfk->data >= 259)
                {
                    rfk->oldname [fk->data] = 0;
                    break;
                }
            }
            StrConverter (rfk->oldname, DCCebcdic_to_ascii);
            rfk->data = 0; /* Get ready for newname */
        }
        R0 = 2;               /* Set work stage 2 */
        set_reg(0,R0);
    }

    /*-------------------------------------------------------------*/
    /* Work Stage 2 Initial or Restart                             */
    /*-------------------------------------------------------------*/

    if (R0 == 2)
    {
        if (R1 <= 3)
        {   /* unlink/open/close - need to get 1 filename */
            /* rename - needs to get 2nd filename */
            while ((rfk->data == 0)                        /* Starting */
                   || (rfk->filename [rfk->data - 1] != 0)) /* Not Finished */
            {
                /* WARNING: This is where interruption might occur */
                ARCH_DEP(wfetchc)
                    (&(rfk->filename [rfk->data]),
                     0,
                     R2,
                     space_ctl,
                     regs);

                /* Exception, can recalculate if/when restart */
                R2 += 1;
                set_reg(2,R2);
                rfk->data += 1;  /* Next host byte location */
                if (rfk->data >= 259)
                {
                    rfk->filename [rfk->data] = 0;
                    break;
                }
            }
            StrConverter (rfk->filename, DCCebcdic_to_ascii);
        }
        R0 = 3;               /* Set work stage */
        set_reg(0,R0);
    }

    /*---------------------------------------------------------------------*/
    /* Work Stage 3 Initial or Restart - Interruptible Operation & Results */
    /*---------------------------------------------------------------------*/

    /*------------------------*/
    /* RENAME File Operation  */
    /*------------------------*/

    if (R1 == 0)
    {
#ifdef DYN76_DEBUG
        logmsg("DF18: CFILE - rename\n");
#endif
        /* This is safe to restore early here */
        R5 = rfk->SaveArea;
        set_reg(5,R5);
#ifdef DYN76_DEBUG
        logmsg("DF18: CFILE - rename(from='%s',to='%s')\n",
                rfk->oldname, rfk->filename);
#endif
        res = rename (rfk->oldname, rfk->filename);
#ifdef DYN76_DEBUG
        logmsg("DF18: CFILE - rename result: %d\n", res);
#endif
        if (res != 0)
        {
            res = errno;
#ifdef DYN76_DEBUG
        logmsg("DF18: CFILE - rename errno: %d\n", res);
#endif
            R15 = -1 * res;
        }
        else
        {
            R15 = res;
        }
        set_reg(15,R15);
    } else

    /*------------------------*/
    /* UNLINK File Operation  */
    /*------------------------*/

    if (R1 == 1)
    {
#ifdef DYN76_DEBUG
        logmsg("DF18: CFILE - unlink\n");
#endif
        /* This is safe to restore early here */
        R5 = rfk->SaveArea;
        set_reg(5,R5);

#ifdef _MSVC_
#ifdef DYN76_DEBUG
        logmsg("DF18: CFILE - _unkink('%s')\n", rfk->filename);
#endif
        res = _unlink (rfk->filename);
#ifdef DYN76_DEBUG
        logmsg("DF18: CFILE - _unlink result: %d\n", res);
#endif
#else
#ifdef DYN76_DEBUG
        logmsg("DF18: CFILE - remove('%s')\n", rfk->filename);
#endif
        res = remove (rfk->filename);
#ifdef DYN76_DEBUG
        logmsg("DF18: CFILE - remove result: %d\n", res);
#endif
#endif
        if (res != 0)
        {
            res = errno;
#ifdef DYN76_DEBUG
            logmsg("DF18: CFILE - remove/_unlink errno: %d\n", res);
#endif
            R15 = -1 * res;
        }
        else
        {
            R15 = res;
        }
        set_reg(15,R15);
    } else

    /*------------------------*/
    /* OPEN File Operation    */
    /*------------------------*/

    if (R1 == 2)
    {
#ifdef DYN76_DEBUG
        logmsg("DF18: CFILE - open\n");
#endif
        /* This is safe to restore early here */
        R5 = rfk->SaveArea;
        set_reg(5,R5);

        /* Convert to host platform native open flags */
        i = 0;
        get_reg(R3,3);
        if (R3 & 0x01)
            i |= _O_WRONLY;
        if (R3 & 0x02)
            i |= _O_RDWR;
        if ((R3 & 0x03) == 0)
            i |= _O_RDONLY;
        if (R3 & 0x04)
        {
            rfk->mode = 1; /* Record the desire to translate codepages */
#ifdef _MSVC_
            i |= _O_TEXT; /* Windows can translate newlines to CRLF pairs */
#endif
        }
        else
        {
            rfk->mode = 0; /* Binary mode (untranslated) */
        }
        if (R3 & 0x10)
            i |= _O_APPEND;
        if (R3 & 0x20)
            i |= _O_TRUNC;
        if (R3 & 0x40)
        {
            i |= _O_CREAT;
#ifdef _MSVC_
            get_reg(R4,4);
            if (R4 == 0) /* Pass 0x100 in Windows for readonly */
            {
                /* Set a Windows default */
                R4 = _S_IWRITE;
                set_reg(4,R4);
            }
#endif
        }
        if (R3 & 0x80)
        {
            i |= _O_EXCL;
        }
#ifndef _MSVC_
        get_reg(R4,4);
        if (R4 == 0)
        {   /* Set a *nix default */
            R4 = 0666;  /* Note octal value */
            set_reg(4,R4);
        }
#endif
#ifdef DYN76_DEBUG
        logmsg("DF18: CFILE - _open('%s', 0x%X, 0%o)\n", rfk->filename, i, R4);
#endif
        res = _open (rfk->filename, i, (mode_t)R4);
#ifdef DYN76_DEBUG
        logmsg("DF18: CFILE - _open result: %d\n", res);
#endif
        if (res != -1)
        {   /* Successful host file open */

            /* Save the handle for use in other operations */
            rfk->handle = res;  /* Save the host handle */

            /* Transfer the restart fkeeper to the open file fkeeper list */
#ifdef DYN76_DEBUG_FKEEPER
            logmsg("DF18: CFILE - removing  w/o freeing fkeeper from restart list rst_head\n");
#endif
            RemoveFKByID (rfk->id, &rst_head, NO_FREE);
            dolock(nfile_lock);
#ifdef DYN76_DEBUG_FKEEPER
            logmsg("DF18: CFILE - adding fkeeper to open file list with fkpr_head\n");
#endif
            AddFKByID(nfile_id++, rfk, &fkpr_head);
#ifdef DYN76_DEBUG
            logmsg("DF18: CFILE - opened guest file descriptor %d, host handle: %d\n",
                    rfk->id, rfk->handle);
#endif
            unlock(nfile_lock);

            R15 = rfk->id;
            set_reg(15,R15);
            rfk = NULL;
            /* Note: during the start of the interruptable open operation the
               fkeeper structure was linked to the fkeeper list to allow restart
               of the operation from the restart id.  Now that we have been
               successful in opening the file, we will leave the restart fkeeper
               on the list as the link to the host file from the guest.
            */
        }
        else
        {   /* Failed host file open */

            res = errno;
#ifdef DYN76_DEBUG
            logmsg("DF18: CFILE - _open errno: %X\n", res);
#endif
            R15 = -1 * res;
            set_reg(15,R15);
            /* Note: during start of the interruptable open operation the
               fkeeper structure was linked to the fkeeper list to allow restart
               of the operation from the restart id.  On a failure to actually open
               the file, the structure needs to be removed from the list and that
               will happen below just before returning to hdiagf18.c
            */
        }
    } else

    /*-----------------------------*/
    /* ORPHAN CLOSE File Operation */
    /*-----------------------------*/

    if (R1 == 3)
    {
#ifdef DYN76_DEBUG
        logmsg("DF18: CFILE - orphan close: '%s'\n", rfk->filename);
#endif
        /* This is safe to restore early here */
        R5 = rfk->SaveArea;
        set_reg(5,R5);

        R15 = RemoveFKByName (rfk->filename);
        set_reg(15,R15);
    } else

    /*-----------------------------*/
    /* READ File Operation         */
    /*-----------------------------*/

    if (R1 == 4)
    {
#ifdef DYN76_DEBUG
        logmsg("DF18: CFILE - read\n");
#endif
        get_reg(R4,4);
        /* Note: R15 has been set to zero above during operation initialization */
#ifdef DYN76_DEBUG
        logmsg("DF18: CFILE - read requested bytes: %d\n", R4);
#endif
        while (R4 != 0)
        {
            if (rfk->data == 0)
            { /* Need to fill our buffer with some data? */
                i = R4;

                if (i > 256)
                    i = 256;

                rfk->data = _read (handle, rfk->filename, i);
#ifdef DYN76_DEBUG
                logmsg("DF18: CFILE - host read result: %d\n", rfk->data);
#endif
                if (fk->mode)
                    MemConverter (rfk->filename, DCCascii_to_ebcdic, rfk->data);
            }
            if (rfk->data < 0)
            {
                R15 = -errno;
                set_reg(15,R15);
            }
            if (rfk->data <= 0)
            {   /* All done, we reached EOF (or an error occured) */
                break;
            }

            /* Move the data read to the guest's storage */
            i = rfk->data - 1;
            /* wstorec was designed to operate with storage-to-storage instructions.
               The instruction length field is always one less than the number of
               bytes involved in the instruction.  Hence the number of bytes to be
               moved to storage is decremented by 1 to conform with this behavior. */

            /* WARNING: This is where an interruption might occur.  An exception
               will "nullify" the storing operation.
               On restart, the read above will be bypassed and the entire sequence
               of bytes, upto 256, will be restored */
            ARCH_DEP(wstorec)
                (rfk->filename,  /* This member is used as a buffer */
                 (unsigned char)i,
                 R2,
                 space_ctl,
                 regs);
            i++;

            /* Exception, can recalculate if/when restart */
            R2 += i;
            set_reg(2,R2);

            /* Remember to stop when enough is read */
            R4 -= i;
            set_reg(4,R4);

            /* Return accumulated total */
            R15 += i;
            set_reg(15,R15);

            rfk->data = 0;
        }
        R5 = rfk->SaveArea;
        set_reg(5,R5);
    } else

    /*-----------------------------*/
    /* WRITE File Operation        */
    /*-----------------------------*/

    if (R1 == 5)
    {
#ifdef DYN76_DEBUG
        logmsg("DF18: CFILE - write\n");
#endif
        get_reg(R4,4);
        /* Note: R15 has been set to zero above during operation initialization */

        while (R4 != 0)
        {
            i = R4 - 1;
            /* wfetchc was designed to operate with storage-to-storage instructions.
               The instruction length field is always one less than the number of
               bytes involved in the instruction.  Hence the number of bytes to be
               retrieved from storage  is decremented by 1 to conform with this
               behavior. */

            /* Move guest's write data to the internal buffer */
            if (i > 255)
            {
                i = 255;
            }

            /* WARNING: This is where interruption might occur.  The exception
               "nullifies" the fetch operation.
               On restart, the entire sequence of up to 256 bytes will be refetched
               from storage */
            ARCH_DEP(wfetchc)
                (rfk->filename,
                 (unsigned char)i,
                 R2,
                 space_ctl,
                 regs);
            i++;  /* Fix up number of bytes stored to actual count */

            if (fk->mode)
                MemConverter (rfk->filename, DCCebcdic_to_ascii, i);
                /* rfk->filename is being used as a data buffer here */

            /* Write to the host file from the internal buffer */
#ifdef DYN76_DEBUG
            logmsg("DF18: CFILE - _write(%d, rfk->filename, %d)\n", handle, i);
#endif
            res = _write (handle, rfk->filename, (size_t)i);
#ifdef DYN76_DEBUG
            logmsg("DF18: CFILE - _write result: %d\n", res);
#endif
            if (res < 0)
            {
#ifdef DYN76_DEBUG
                io_error = errno;
                logmsg("DF18: CFILE - write errno: %d\n", io_error);
#endif
                R15 = -errno;
                set_reg(15,R15);
                break;
            }

            /* Not an error, so 'res' is the number of bytes actually written */
            /* Update the address pointer and remaining bytes to write */
            R2 += res;
            set_reg(2,R2);
            R4 -= res;
            set_reg(4,R4);
            /* update the accumlated total */
            R15 += res;
            set_reg(15,R15);
        }
        R5 = rfk->SaveArea;
        set_reg(5,R5);
    }

    if (rfk) /* clean up, unless open already has */
    {
#ifdef DYN76_DEBUG_FKEEPER
        logmsg("DF18: CFILE - removing and freeing restart fkeeper in rst_head list\n");
#endif
        /* Safely remove from the restart state */
        RemoveFKByID (rfk->id, &rst_head, DO_FREE);
    }

    /* Make sure we do not restart this operation */
    R0 = 0;
    set_reg(0,R0);
}

#if !defined(_GEN_ARCH)

#if defined(_ARCH_NUM_1)
#define _GEN_ARCH _ARCH_NUM_1
#include "dyn76.c"
#endif

#if defined(_ARCH_NUM_2)
#undef _GEN_ARCH
#define _GEN_ARCH _ARCH_NUM_2
#include "dyn76.c"
#endif

#endif /*!defined(_GEN_ARCH)*/
