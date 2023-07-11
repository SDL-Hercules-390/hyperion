/* VMD250.C     (C) Copyright Harold Grovesteen, 2009-2012           */
/*              z/VM 5.4 DIAGNOSE code X'250'                        */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* This module implements DIAGNOSE code X'250' as described in:      */
/* SC24-6084    z/VM 5.4 CP Programming Services                     */
/* SC24-6179-04 z/VM 6.3 CP Programming Services                     */
/* SC24-6272-03 z/VM 7.1 CP Programming Services                     */
/*-------------------------------------------------------------------*/
/*                                                                   */
/* Hercules extends the use to System/370 with 4K pages and 2K pages */
/* with recommended constraints                                      */
/*                                                                   */
/* Unconditional log messages generated                              */
/*                                                                   */
/*  - Could not allocate storage for Block I/O environment           */
/*  - Invalid list processor status code returned                    */
/*  - Error creating asynchronous thread                             */
/*  - Could not allocate storage for asynchronous I/O request        */
/*                                                                   */
/* Conditional log messages when device tracing is enabled           */
/*                                                                   */
/*  - BLKTAB information used when device tracing is enabled         */
/*  - Established environment when device tracing is enabled         */
/*  - Block I/O owns device                                          */
/*  - Block I/O returned device                                      */
/*  - BIOE status returned                                           */
/*  - List processor parameters                                      */
/*  - BIOE operation being performed                                 */
/*  - List processor status code                                     */
/*  - Read/Write I/O operation                                       */
/*  - Syncronous or asynchronous request information                 */
/*  - Address checking results                                       */
/*  - Device driver results                                          */
/*  - Block I/O environment removed                                  */
/*  - Triggered Block I/O interrupt                                  */
/*                                                                   */
/*-------------------------------------------------------------------*/
/*                                                                   */
/* The following structure exists between the different functions    */
/*                                                                   */
/* From diagnose.c                                                   */
/*                                                                   */
/* AD:vm_blockio                                                     */
/*    |                                                              */
/*   INIT:                                                           */
/*    +---> d250_init32---+                                          */
/*    |                   +---> d250_init                            */
/*    +---> d250_init64---+                                          */
/*    |                                                              */
/*   IOREQ:                                                          */
/*    +-> AD:d250_iorq32--+---SYNC----> d250_list32--+               */
/*    |                   V                   ^      |               */
/*    |               ASYNC Thread            |      |    d250_read  */
/*    |                   +-> AD:d250_async32-+      +--> d250_write */
/*    |                       d250_bio_interrupt     |    (calls     */
/*    |                                              |    drivers)   */
/*    +-> AD:d250_iorq64--+----SYNC---> d250_list64--+               */
/*    |                   V                   ^                      */
/*    |               ASYNC Thread            |                      */
/*    |                   +-> AD:d250_async64-+                      */
/*    |                       d250_bio_interrupt                     */
/*   REMOVE:                                                         */
/*    +---> d250_remove                                              */
/*                                                                   */
/*  Function         ARCH_DEP   On CPU   On Async    Owns            */
/*                              thread    thread     device          */
/*                                                                   */
/*  vm_blockio          Yes      Yes        No         No            */
/*  d250_init32/64      No       Yes        No         No            */
/*  d250_init           No       Yes        No         No            */
/*  d250_iorq32/64      Yes      Yes        No         No            */
/*  d250_async32/64     Yes      No         Yes        No            */
/*  d250_list32/64      Yes      Yes        Yes       Yes            */
/*  d250_bio_interrup   No       No         Yes       N/A            */
/*  d250_read           No       Yes        Yes       Yes            */
/*  d250_write          No       Yes        Yes       Yes            */
/*  d250_remove         No       Yes        No         No            */
/*  d250_addrck         Yes      Yes        Yes        No            */
/*                                                                   */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

#define _VMD250_C_
#define _HENGINE_DLL_

#include "hercules.h"
#include "opcode.h"
#include "inline.h"
#include "vmd250.h"

#define FEATURE_VM_BLOCKIO
#if defined(FEATURE_VM_BLOCKIO)

#if !defined(_VMD250_C)
#define _VMD250_C

#if !defined(_VMD250_H)
#define _VMD250_H
/*-------------------------------------------------------------------*/
/* Internal macro definitions                                        */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* Block I/O Function, Condition and Return Codes                    */
/*-------------------------------------------------------------------*/

/* Function Codes (Ry)                                               */
#define INIT   0x00              /* Initialize Block I/O environment */
#define IOREQ  0x01              /* Perform I/O request              */
#define REMOVE 0x02              /* Remove the Block I/O environment */

/* Condition Codes (cc)                                              */
#define CC_SUCCESS 0             /* Function completed successfully  */
#define CC_PARTIAL 1             /* Function partially completed     */
#define CC_FAILED  2             /* Function failed                  */
/* Note: cc 3 is not returned by Block I/O                           */

/* General Return Codes (Rx+1)- All functions                        */
#define RC_SUCCESS   0           /* Function competed successfully   */
#define RC_NODEV    16           /* Device not defined               */
#define RC_STATERR  28           /* Env. state error                 */
#define RC_ERROR   255           /* Irrecoverable error              */
/* Note: RC_ERROR is used to indicate an internal Hercules error     */
/*       and it should be accompanied by a log message               */

/* INIT Return Codes (Rx+1)                                          */
#define RC_READONLY  4           /* Successful for R/O device        */
#define RC_NOSUPP   20           /* Not a supported DASD device      */
#define RC_BADBLKSZ 24           /* Unsupported block size           */

/* IOREQ Return Codes (Rx+1)                                         */
#define RC_ASYNC     8           /* Asynchronous request initiated   */
#define RC_SYN_PART 12  /* Synchronous request partially successful  */
#define RC_CNT_ERR  36  /* Block count not between 1 and 256         */
#define RC_ALL_BAD  40  /* All blocks unsuccessful                   */
#define RC_REM_PART 44  /* Remove aborted request - should not occur */
#define RC_EXI_PART 48  /* Exigent aborted req. - should not occur   */
/* Note: Because the DEVBLK lock is held RC_REM_PART and RC_EXI_PART */
/*       should not occur in Hercules                                */


/*-------------------------------------------------------------------*/
/* Block I/O Parameter List Formats (BIOPL)                          */
/*-------------------------------------------------------------------*/
#define INIT32R1_LEN 21
#define INIT32R2_LEN 24
typedef struct _BIOPL_INIT32 {
        HWORD   devnum;              /* Device number                */
        BYTE    flaga;               /* Addressing mode flag         */
        BYTE    resv1[INIT32R1_LEN]; /* reserved - must be zeros     */
        FWORD   blksize;             /* Block size                   */
        FWORD   offset;              /* Physical block 1 offset      */
        FWORD   startblk;            /* First physical block number  */
        FWORD   endblk;              /* Last physical block number   */
        BYTE    resv2[INIT32R2_LEN]; /* reserved - must be zeros     */
    } BIOPL_INIT32;

#define INIT64R1_LEN 21
#define INIT64R2_LEN 4
#define INIT64R3_LEN 8
typedef struct _BIOPL_INIT64 {
        HWORD   devnum;              /* Device number                */
        BYTE    flaga;               /* Addressing mode flag         */
        BYTE    resv1[INIT64R1_LEN]; /* reserved - must be zeros     */
        FWORD   blksize;             /* Block size                   */
        BYTE    resv2[INIT64R2_LEN]; /* Physical block 1 offset      */
        DBLWRD  offset;              /* Physical block 1 offset      */
        DBLWRD  startblk;            /* First physical block number  */
        DBLWRD  endblk;              /* Last physical block number   */
        BYTE    resv3[INIT64R3_LEN]; /* reserved - must be zeros     */
    } BIOPL_INIT64;

#define IORQ32R1_LEN 21
#define IORQ32R2_LEN 2
#define IORQ32R3_LEN 20
typedef struct _BIOPL_IORQ32 {
        HWORD   devnum;              /* Device number                */
        BYTE    flaga;               /* Addressing mode flag         */
        BYTE    resv1[IORQ32R1_LEN]; /* reserved - must be zeros     */
        BYTE    key;                 /* Storage key                  */
        BYTE    flags;               /* I/O request flags            */
        BYTE    resv2[IORQ32R2_LEN]; /* reserved - must be zeros     */
        FWORD   blkcount;            /* Block count of request       */
        FWORD   bioealet;            /* BIOE list ALET               */
        FWORD   bioeladr;            /* BIOE list address            */
        FWORD   intparm;             /* Interrupt parameter          */
        BYTE    resv3[IORQ32R3_LEN]; /* reserved - must be zeros     */
    } BIOPL_IORQ32;

#define IORQ64R1_LEN 21
#define IORQ64R2_LEN 2
#define IORQ64R3_LEN 4
#define IORQ64R4_LEN 8
typedef struct _BIOPL_IORQ64 {
        HWORD   devnum;              /* Device number                */
        BYTE    flaga;               /* Addressing mode flag         */
        BYTE    resv1[IORQ64R1_LEN]; /* reserved - must be zeros     */
        BYTE    key;                 /* Storage key                  */
        BYTE    flags;               /* I/O request flags            */
        BYTE    resv2[IORQ64R2_LEN]; /* reserved - must be zeros     */
        FWORD   blkcount;            /* Block count of request       */
        FWORD   bioealet;            /* BIOE list ALET               */
        BYTE    resv3[IORQ64R3_LEN]; /* reserved - must be zeros     */
        DBLWRD  intparm;             /* Interrupt parameter          */
        DBLWRD  bioeladr;            /* BIOE list address            */
        BYTE    resv4[IORQ64R4_LEN]; /* reserved - must be zeros     */
    } BIOPL_IORQ64;

#define REMOVER1_LEN 62
typedef struct _BIOPL_REMOVE {
        HWORD   devnum;              /* Device number                */
        BYTE    resv1[REMOVER1_LEN]; /* reserved - must be zeros     */
    } BIOPL_REMOVE;

typedef struct _BIOPL {
        HWORD devnum;                /* Device number                */
        BYTE  flaga;                 /* Addressing mode flag         */
        BYTE resv1[61];              /* Additional Parameters        */
    } BIOPL;

/* BIOPL flaga field related values                                  */
#define BIOPL_FLAGARSV 0x7F         /* Reserved bits must be zero    */
#define BIOPL_FLAGAMSK 0x80         /* Addressing flag               */
#define BIOPL_32BIT    0x00         /* 32-bit BIOPL/BIOE Formats     */
#define BIOPL_64BIT    0x80         /* 64-bit BIOPL/BIOE Formats     */

/* BIOPL flags field related values                                  */
#define BIOPL_FLAGSRSV 0xFC         /* Reserved bits must be zero    */
#define BIOPL_ASYNC    0x02         /* Asynchronous request          */
#define BIOPL_CACHE    0x01         /* Not used - z/VM specific      */

/* BIOPL key field values                                            */
#define BIOPL_KEYRSV   0x0F         /* Reserved bits must be zero    */

/*-------------------------------------------------------------------*/
/* Block I/O Entry Formats (BIOE)                                    */
/*-------------------------------------------------------------------*/

typedef struct _BIOE32 {
        BYTE    type;               /* Type of I/O request           */
        BYTE    status;             /* Status of I/O request         */
        BYTE    resv1[2];           /* reserved - must be zeros      */
        FWORD   blknum;             /* Requested block number        */
        FWORD   bufalet;            /* Data buffer ALET              */
        FWORD   bufaddr;            /* Data buffer absolute address  */
    } BIOE32;

typedef struct _BIOE64 {
        BYTE    type;               /* Type of I/O request           */
        BYTE    status;             /* Status of I/O request         */
        BYTE    resv1[2];           /* reserved - must be zeros      */
        FWORD   bufalet;            /* Data buffer ALET              */
        DBLWRD  blknum;             /* Requested block number        */
        DBLWRD  bufaddr;            /* Data buffer absolute address  */
    } BIOE64;

/* BIOE type field values                                            */
#define BIOE_WRITE     0x01         /* Write block request type      */
#define BIOE_READ      0x02         /* Read block request type       */

/* BIOE status field values                                          */
#define BIOE_SUCCESS   0x00     /* Request successful                */
#define BIOE_BADBLOCK  0x01     /* Invalid block specified           */
#define BIOE_ADDREXC   0x02     /* Data buffer address exception     */
#define BIOE_DASDRO    0x03     /* Write request to read-only device */
#define BIOE_CKDRECL   0x04     /* CKD record size not block size    */
#define BIOE_IOERROR   0x05     /* I/O error on device               */
#define BIOE_BADREQ    0x06     /* Request type invalid              */
#define BIOE_PROTEXC   0x07     /* A protection exception occurred   */
#define BIOE_ADRCAP    0x08     /* Not used - z/VM specific          */
#define BIOE_ALENEXC   0x09     /* Not used - z/VM specific          */
#define BIOE_ALETEXC   0x0A     /* Not used - z/VM specific          */
#define BIOE_NOTZERO   0x0B     /* Reserved fields not zero          */
#define BIOE_ABORTED   0x0C     /* Request aborted                   */

/*-------------------------------------------------------------------*/
/* I/O Request Control Structures and Flags                          */
/*-------------------------------------------------------------------*/

#define SYNC   0  /* Synchronous request being processed  */
#define ASYNC  1  /* Asynchronous request being processed */

/* Synchronous or asynchronous processing status codes     */
#define PSC_SUCCESS 0x00  /* Successful processing         */
#define PSC_PARTIAL 0x01  /* Partial success               */
#define PSC_STGERR  0x02  /* Error on storage of BIOE      */
#define PSC_REMOVED 0x03  /* Block I/O environment removed */

/* Structure passed to the asynchronous thread */
typedef struct _IOCTL32 {
        /* Hercules structures involved in the request */
        REGS   *regs;                /* CPU register structure       */
        DEVBLK *dev;                 /* Device block                 */
        /* External interrupt related values */
        BYTE    subintcod;           /* Sub-interruption code        */
        BYTE    statuscod;           /* Interruption status code     */
        U32     intrparm;            /* 32-bit interrupt parm        */
        /* List control structure */
        S32     blkcount;            /* Block count                  */
        U32     listaddr;            /* BIOE list address            */
        BYTE    key;                 /* Storage Key                  */
        int     goodblks;            /* Number of successful I/O's   */
        int     badblks;             /* Number of unsuccessful I/O's */
    } IOCTL32;

/* Structure passed to the asynchronous thread */
typedef struct _IOCTL64 {
        /* Hercules structures involved in the request */
        REGS   *regs;                /* CPU register structure       */
        DEVBLK *dev;                 /* Device block                 */
        /* External interrupt related values related values */
        BYTE    subintcod;           /* Sub-interruption code        */
        BYTE    statuscod;           /* Interruption status code     */
        U64     intrparm;            /* 64-bit interrupt parm        */
        /* List control structure */
        S64     blkcount;            /* Block count                  */
        U64     listaddr;            /* BIOE list address            */
        BYTE    key;                 /* Storage Key                  */
        int     goodblks;            /* Number of successful I/O's   */
        int     badblks;             /* Number of unsuccessful I/O's */
    } IOCTL64;

#endif /* !defined(_VMD250_H) */

/*-------------------------------------------------------------------*/
/* Internal Architecture Independent Function Prototypes             */
/*-------------------------------------------------------------------*/
/* Initialization Functions */
static int  d250_init32(DEVBLK *, int *, BIOPL_INIT32 *, REGS *);
static int  d250_init64(DEVBLK *, int *, BIOPL_INIT64 *, REGS *);
static struct VMBIOENV* d250_init(DEVBLK *, U32, S64, int *, int *);

/* Input/Output Request Functions */
static void d250_preserve(DEVBLK *);
static void d250_restore(DEVBLK *);
static int d250_read(DEVBLK *, S64, S32, void *);
static int d250_write(DEVBLK *, S64, S32, void *);
/* Note: some I/O request functions are architecture dependent */

/* Removal Function */
static int  d250_remove(DEVBLK *, int *, BIOPL_REMOVE *, REGS *);

/* Asynchronous Interrupt Generation */
static void d250_bio_interrupt(DEVBLK *, U64 intparm, BYTE status, BYTE code);

/*-------------------------------------------------------------------*/
/*  Trigger Block I/O External Interrupt                             */
/*-------------------------------------------------------------------*/
static void d250_bio_interrupt(DEVBLK *dev, U64 intparm, BYTE status, BYTE subcode)
{

   OBTAIN_INTLOCK(NULL);

   /* This is inspired by sclp_attn_thread function in service.c */
   /* Make sure a service signal interrupt is not pending */
   while(IS_IC_SERVSIG)
   {
       RELEASE_INTLOCK(NULL);
       sched_yield();
       OBTAIN_INTLOCK(NULL);
   }

   /* Can now safely store my interrupt information */
   sysblk.bioparm  = intparm;   /* Trigger with the interrupt parameter */
   sysblk.biostat  = status;    /* Trigger with the status */
   sysblk.biosubcd = subcode;   /* Trigger with the subcode */
   sysblk.biodev   = dev;       /* Trigger for this device  */
   sysblk.servcode = EXT_BLOCKIO_INTERRUPT;

   /* The Block I/O external interrupt is enabled by the same CR0 bit */
   /* as are service signal interrupts.  This means the guest will    */
   /* be interrupted when it has enabled service signals.  For this   */
   /* reason the Block I/O is being treated like a service signal and */
   /* handled by the service signal handling logic in external.c      */

   /* Make the "service signal" interrupt pending                     */
   ON_IC_SERVSIG;
   /* Wake up any waiters */
   WAKEUP_CPUS_MASK (sysblk.waiting_mask);

   if (dev->ccwtrace)
   {
      WRMSG (HHC01905, "I",
              sysblk.biodev->devnum,
              sysblk.servcode,
              sysblk.bioparm,
              sysblk.biostat,
              sysblk.biosubcd
           );
   }

   /* Free the interrupt lock */
   RELEASE_INTLOCK(NULL);
}

/*-------------------------------------------------------------------*/
/*  Initialize Environment - 32-bit Addressing                       */
/*-------------------------------------------------------------------*/
static int  d250_init32(DEVBLK *dev, int *diag_rc, BIOPL_INIT32 *biopl,
                 REGS *regs)
{
BIOPL_INIT32   bioplx00;             /* Use to check reserved fields */

/* Passed to the generic INIT function */
U32     blksize;                     /* Blocksize                    */
S32     offset;                      /* Offset                       */

/* Returned by generic INIT function */
struct VMBIOENV *bioenv;            /* -->allocated environement     */
int     rc;                         /* return code                   */
int     cc;                         /* Condition code to return      */

   /* Clear the reserved BIOPL */
   memset(&bioplx00,0,sizeof(BIOPL_INIT32));

   /* Make sure reserved fields are binary zeros                     */
   if ((memcmp(&biopl->resv1,&bioplx00,INIT32R1_LEN)!=0) ||
       (memcmp(&biopl->resv2,&bioplx00,INIT32R2_LEN)!=0))
   {
       ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
   }

   /* Fetch the block size from the BIOPL */
   FETCH_FW(blksize,&biopl->blksize);

   /* Fetch the offset from the BIOPL provided by the guest */
   FETCH_FW(offset,&biopl->offset);

   /* Call the addressing independent initialization function */
   bioenv=d250_init(dev, blksize, (S64)offset, &cc, &rc);

   if (bioenv)
   {
      /* Save the values in the BIOPL for return to guest */
      STORE_FW(&biopl->startblk,(U32)bioenv->begblk);
      STORE_FW(&biopl->endblk,(U32)bioenv->endblk);
      if (dev->ccwtrace)
      {
         WRMSG (HHC01906, "I",
              dev->devnum,
              blksize,
              (S64)offset,
              bioenv->begblk,
              bioenv->endblk
             );
      }
   }
   *diag_rc = rc;
   return cc;
} /* end function d250_init32 */

/*-------------------------------------------------------------------*/
/*  Initialize Environment - 64-bit Addressing                       */
/*-------------------------------------------------------------------*/
static int d250_init64(DEVBLK *dev, int *diag_rc, BIOPL_INIT64 *biopl,
                REGS *regs)
{
BIOPL_INIT64   bioplx00;             /* Use to check reserved fields */

/* Passed to the generic INIT function */
U32     blksize;                     /* Blocksize                    */
S64     offset;                      /* Offset                       */

/* Returned by generic INIT function */
struct VMBIOENV *bioenv;             /* -->allocated environement    */
int     rc;                          /* return code                  */
int     cc;                          /* condition code               */

   /* Clear the reserved BIOPL */
   memset(&bioplx00,0,sizeof(BIOPL_INIT64));

   /* Make sure reserved fields are binary zeros  */
   if ((memcmp(&biopl->resv1,&bioplx00,INIT64R1_LEN)!=0) ||
       (memcmp(&biopl->resv2,&bioplx00,INIT64R2_LEN)!=0) ||
       (memcmp(&biopl->resv3,&bioplx00,INIT64R3_LEN)!=0))
   {
       ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
   }

   /* Fetch the block size from the BIOPL */
   FETCH_FW(blksize,&biopl->blksize);

   /* Fetch the offset from the BIOPL provided by the guest */
   FETCH_DW(offset,&biopl->offset);

   bioenv=d250_init(dev, blksize, offset, &cc, &rc);

   if (bioenv)
   {
      /* Save the values in the BIOPL for return to guest */
      STORE_DW(&biopl->startblk,bioenv->begblk);
      STORE_DW(&biopl->endblk,bioenv->endblk);
      if (dev->ccwtrace)
      {
         WRMSG (HHC01906, "I",
              dev->devnum,
              blksize,
              offset,
              bioenv->begblk,
              bioenv->endblk
             );
      }
   }
   *diag_rc = rc;
   return cc;
} /* end function d250_init64 */

/*-------------------------------------------------------------------*/
/*  Initialize Environment - Addressing Independent                  */
/*-------------------------------------------------------------------*/
static struct VMBIOENV* d250_init(DEVBLK *dev, U32 blksize, S64 offset,
                    int *cc, int *rc )
{
int      isCKD;         /* Flag for CKD device                       */
int      isRO;          /* Flag for readonly device                  */
int      seccyl;/* Number of sectors/block or records/track for dev. */
int      numblks;       /* Number of blocks on the device            */
S32      begblk;        /* Starting block number                     */
S32      endblk;        /* Ending block number                       */
BLKTAB  *blktab;     /* Pointer to device std block-to-physical info */
/* Established environement                                          */
struct VMBIOENV *bioenv;  /* -->allocated environement               */

   /* Return with an error if the device does not exist */
   if (!dev)
   {
      *rc = RC_NODEV;  /* Set the return code for no device */
      *cc = CC_FAILED; /* Indicate the function failed      */
      return 0;
   }

   /* Look up the block-to-phyical mapping information */
   blktab=dasd_lookup(DASD_STDBLK,NULL,(U32)dev->devtype,0);
   if (!blktab)
   {
       *rc = RC_NOSUPP; /* Set the return code for unsuppored device */
       *cc = CC_FAILED; /* Indicate the function failed      */
       return NULL;
   }

   if (dev->ccwtrace)
   {
      WRMSG (HHC01907, "I",
              dev->devnum,
              blktab->devt,
              blktab->darch,
              blktab->phys512,
              blktab->phys1024,
              blktab->phys2048,
              blktab->phys4096
             );
   }

   /* Save the device architecture */
   isCKD = blktab->darch;

   /* Determine if the blocksize is valid and its physical/block value */
   switch(blksize)
   {
      case 4096:
         seccyl=blktab->phys4096;
         break;
      case 512:
         seccyl=blktab->phys512;
         break;
      case 1024:
         seccyl=blktab->phys1024;
         break;
      case 2048:
         seccyl=blktab->phys2048;
         break;
      default:
         *rc = RC_BADBLKSZ;
         *cc = CC_FAILED;
         return NULL;
   }

   isRO = 0;  /* Assume device is read-write */
   if (isCKD)
   {
      /* Number of standard blocks is based upon number of primary */
      /* cylinders                                                 */
      numblks=(dev->ckdtab->cyls * dev->ckdtab->heads * seccyl);
      if (dev->ckdrdonly)
      {
         isRO = 1;
      }
   }
   else
   {
      numblks=(dev->fbanumblk*dev->fbablksiz)/blksize;
      /* FBA devices are never read only */
   }

   /* Establish the environment's beginning and ending block numbers */
   /* Block numbers in the environment are relative to 1, not zero   */
   begblk=1-offset;
   endblk=numblks-offset;

   if (!(bioenv=(struct VMBIOENV *)malloc(sizeof(struct VMBIOENV))))
   {
      char buf[40];
      MSGBUF(buf, "malloc(%d)",(int)sizeof(struct VMBIOENV));
      WRMSG (HHC01908, "E", buf, strerror(errno));
      *rc = RC_ERROR;  /* Indicate an irrecoverable error occurred */
      *cc = CC_FAILED;
      return NULL;
   }

   /* Set the fields in the environment structure                */
   bioenv->dev     = dev     ; /* Set device block pointer       */
   bioenv->blksiz  = blksize ; /* Pass the block size            */
   bioenv->offset  = offset  ; /* Pass the offset                */
   bioenv->begblk  = begblk  ; /* Save the starting block        */
   bioenv->endblk  = endblk  ; /* Save the ending block          */
   bioenv->isCKD   = isCKD   ; /* Save the device type           */
   bioenv->isRO    = isRO    ; /* Save the read/write status     */
   bioenv->blkphys = seccyl  ; /* Save the block-to-phys mapping */

   /* Attach the environment to the DEVBLK */
   /* Lock the DEVBLK in case another thread wants it */
   OBTAIN_DEVLOCK( dev );
   if (dev->vmd250env == NULL)
   {
       /* If an environment does not exist, establish it */
       dev->vmd250env = bioenv ;
       /* No need to hold the device lock now, environment is set */
       RELEASE_DEVLOCK( dev );

       /* Set the appropriate successful return and condition codes */
       if (isRO)
       {
           *rc = RC_READONLY ; /* Set READ ONLY success return code */
       }
       else
       {
           *rc = RC_SUCCESS  ; /* Set the READ/WRITE success return code */
       }
       *cc = CC_SUCCESS ; /* Set that the function has succeeded */
   }
   else
   {
       /* If an environment already exists, this is an error:    */
       /*   1. Release the device block                          */
       /*   2. free the environment just built                   */
       /*   3. Reset the retuned environment to NULL and         */
       /*   4. Reset return and condition codes to reflect       */
       /*      the error condition                               */
       RELEASE_DEVLOCK( dev );
       free(bioenv);
       bioenv = NULL ;
       *rc = RC_STATERR;
       *cc = CC_FAILED;
   }

   /* Return the bioenv so that the start and end blocks can     */
   /* be returned to the guest in the addressing mode specific   */
   /* BIOPL format                                               */
   return bioenv ;

} /* end function d250_init */

/*-------------------------------------------------------------------*/
/* Preserve Device Status                                            */
/*-------------------------------------------------------------------*/
/* WARNING: This function must NOT be called with the device lock held */

/* From the perspective of Hercules, Block I/O is occurring at the */
/* level of the device, NOT the channel or channel subsystem.  For */
/* the channel subsytem or other CPU's, the device is made to      */
/* appear busy and reserved if shared.                             */
/*                                                                 */
/* It is possible for block I/O to take control between the        */
/* completion of a CCW chain and the presentation by the device of */
/* related sense information.  The device related sense information*/
/* is therefore stored in the Block I/O environment and restored   */
/* upon completion.  This also allows the device drivers to set    */
/* sense information during Block I/O operations, which they do.   */
/* A pointer is used by the device drivers for unit status, so     */
/* this can be redirected to a Block I/O location.                 */
/*                                                                 */
/* A cleaner but more intrusive change would be to have the        */
/* the drivers use a pointer for where sense is stored rather than */
/* assume the device block.  This is deferred for a possible future*/
/* enhancement.                                                    */
static void d250_preserve(DEVBLK *dev)
{
    OBTAIN_DEVLOCK( dev );

#if defined( OPTION_SHARED_DEVICES )
    /* Wait for the device to become available      */
    /* Note: this logic comes from the beginning of */
    /* channel.c ARCH_DEP(execute_ccw_chain)        */
    shared_iowait( dev );
    dev->shioactive = DEV_SYS_LOCAL;
#endif // defined( OPTION_SHARED_DEVICES )

    dev->busy = 1;
    dev->startpending = 0;
    if (dev->sns_pending)
    {
       /* Save the pending sense */
       memcpy(&dev->vmd250env->sense,&dev->sense,sizeof(dev->sense));
       if (dev->ccwtrace)
       {  WRMSG(HHC01909, "I", dev->devnum);
       }
    }

    /* Both fbadasd.c and ckddasd.c set the local reserved flag before  */
    /* calling the shared device client                                 */
    dev->reserved = 1;
    if (dev->hnd->reserve)
    {
       RELEASE_DEVLOCK( dev );
       (dev->hnd->reserve)(dev);
    }
    else
    {
       RELEASE_DEVLOCK( dev );
    }
}

/*-------------------------------------------------------------------*/
/* Restore Device Status                                             */
/*-------------------------------------------------------------------*/
/* WARNING: This function MUST not be called with the device lock held */
static void d250_restore(DEVBLK *dev)
{
    OBTAIN_DEVLOCK( dev );
    {
        if (dev->hnd->release)
        {
            RELEASE_DEVLOCK( dev );
            {
                (dev->hnd->release)( dev );
            }
            OBTAIN_DEVLOCK( dev );
        }

        /* Both fbadasd.c and ckddasd.c reset the local reserved flag     */
        /* after calling the shared device client                         */
        dev->reserved = 0;

        if (dev->sns_pending)
        {
            /* Restore the pending sense */
            memcpy(&dev->sense,&dev->vmd250env->sense,sizeof(dev->sense));

            if (dev->ccwtrace)
                // %04X d250_restore pending sense restored"
                WRMSG (HHC01920, "I", dev->devnum);
        }
#if defined( OPTION_SHARED_DEVICES )
        dev->shioactive = DEV_SYS_NONE;
#endif
        dev->busy = 0;
    }
    RELEASE_DEVLOCK( dev );
}

/*-------------------------------------------------------------------*/
/*  Environment Remove - Any Addressing                              */
/*-------------------------------------------------------------------*/
static int d250_remove(DEVBLK *dev, int *rc, BIOPL_REMOVE * biopl, REGS *regs)
{
BIOPL_REMOVE bioplx00;               /* Use to check reserved fields */
struct VMBIOENV *bioenv;             /* -->allocated environement    */
int       cc;                        /* Condition code to return     */

   /* Clear the reserved BIOPL */
   memset(&bioplx00,0,sizeof(BIOPL_REMOVE));


   /* Make sure reserved fields are binary zeros  */
   if (memcmp(&biopl->resv1,&bioplx00,REMOVER1_LEN)!=0)
   {
       ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
   }

   /* Return with an error if the device does not exist */
   if (!dev)
   {
      *rc = RC_NODEV;  /* Set the return code for no device */
      return CC_FAILED; /* Indicate the function failed     */
   }

   /* Attach the environment to the DEVBLK */
   /* Lock the DEVBLK in case another CPU is trying something */
   OBTAIN_DEVLOCK( dev );
   bioenv=dev->vmd250env;
   if (dev->vmd250env == NULL)
   {
       /* If an environment does not exist, it should be */
       /* Return the environment state error return code */
       /* and failed condition code                      */
       RELEASE_DEVLOCK( dev );
       *rc = RC_STATERR;
       cc = CC_FAILED;
   }
   else
   {
       /*   1. Restore pending sense, if any                     */
       /*   2. Free the environment previously established       */
       /*   3. Set the device environment pointer to NULL,       */
       /*      telling others that the environment doesn't exist */
       /*   4. Release the device block                          */
       /*   5. Free the environment previously established       */
       /*   6. Return successful return and condition codes      */
       if (dev->sns_pending)
       {
           memcpy(&dev->sense,dev->vmd250env->sense,sizeof(dev->sense));
       }
       dev->vmd250env = NULL ;
       /* No need to hold the device lock while freeing the environment */
       RELEASE_DEVLOCK( dev );
       free(bioenv);
       if (dev->ccwtrace)
       {
           WRMSG(HHC01921, "I", dev->devnum);
       }
       *rc = RC_SUCCESS;
       cc = CC_SUCCESS ; /* Set that the function has succeeded */
   }
   return cc ;
} /* end function d250_remove */

/*-------------------------------------------------------------------*/
/*  Device Independent Read Block                                    */
/*-------------------------------------------------------------------*/
static int d250_read(DEVBLK *dev, S64 pblknum, S32 blksize, void *buffer)
{
BYTE unitstat;     /* Device unit status */
U32  residual;     /* Residual byte count */

/* Note: Not called with device lock held */

    OBTAIN_DEVLOCK( dev );
    if (dev->ccwtrace)
    {
       WRMSG(HHC01922, "I", dev->devnum, blksize, pblknum);
    }

    if (dev->vmd250env->isCKD)
    {
       RELEASE_DEVLOCK( dev );
       /* Do CKD I/O */

       /* CKD to be supplied */

       return BIOE_IOERROR;
    }
    else
    {
       /* Do FBA I/O */

       /* Call the I/O start exit */
       if (dev->hnd->start) (dev->hnd->start) (dev);

       unitstat = 0;

       /* Call the FBA driver's read standard block routine */
       fbadasd_read_block(dev, (int)pblknum, (int)blksize,
                          dev->vmd250env->blkphys,
                          buffer, &unitstat, &residual );

       if (dev->ccwtrace)
       {
          WRMSG(HHC01923, "I", dev->devnum, unitstat, residual );
       }

       /* Call the I/O end exit */
       if (dev->hnd->end) (dev->hnd->end) (dev);

       RELEASE_DEVLOCK( dev );
    }
    /* If an I/O error occurred, return status of I/O Error */
    if ( unitstat != ( CSW_CE | CSW_DE ) )
    {
       return BIOE_IOERROR;
    }

    /* If there was a residual count, block size error, return status of 2 */
    /* Note: This can only happen for CKD devices                          */
    if ( residual != 0 )
    {
       return BIOE_CKDRECL;
    }

    /* Success! return 0 status */
    return BIOE_SUCCESS;
}

/*-------------------------------------------------------------------*/
/*  Device Independent Write Block                                   */
/*-------------------------------------------------------------------*/
static int d250_write(DEVBLK *dev, S64 pblknum, S32 blksize, void *buffer)
{
BYTE unitstat;     /* Device unit status */
U32  residual;     /* Residual byte count */

    OBTAIN_DEVLOCK( dev );
    if (dev->ccwtrace)
    {
       WRMSG(HHC01922, "I", dev->devnum, blksize, pblknum);
    }

    if (!dev->vmd250env)
    {
       RELEASE_DEVLOCK( dev );
       return BIOE_ABORTED;
    }
    if (dev->vmd250env->isCKD)
    {
        RELEASE_DEVLOCK( dev );
        /* Do CKD I/O */

        /* CKD to be supplied */

        return BIOE_IOERROR;
    }
    else
    {
       /* Do FBA I/O */

       /* Call the I/O start exit */
       if (dev->hnd->start) (dev->hnd->start) (dev);

       unitstat = 0;

       /* Call the FBA driver's write standard block routine */
       fbadasd_write_block(dev, (int)pblknum, (int)blksize,
                           dev->vmd250env->blkphys,
                           buffer, &unitstat, &residual );
       if (dev->ccwtrace)
       {
          WRMSG(HHC01923, "I", dev->devnum, unitstat, residual );
       }

       /* Call the I/O end exit */
       if (dev->hnd->end) (dev->hnd->end) (dev);

       RELEASE_DEVLOCK( dev );
    }

    /* If an I/O error occurred, return status of 1 */
    if ( unitstat != ( CSW_CE | CSW_DE ) )
    {
       return BIOE_IOERROR;
    }

    /* If there was a residual count, block size error, return status of 2 */
    /* Note: This can only happen for CKD devices                          */
    if ( residual != 0 )
    {
       return BIOE_CKDRECL;
    }

    /* Success! */
    return BIOE_SUCCESS;
}

#endif /*!defined(_VMD250_C)*/

/*-------------------------------------------------------------------*/
/* Internal Architecture Dependent Function Prototypes               */
/*-------------------------------------------------------------------*/
/* Input/Output Request Functions */
static int   ARCH_DEP(d250_iorq32)(DEVBLK *, int *, BIOPL_IORQ32 *, REGS *);
static int   ARCH_DEP(d250_list32)(IOCTL32 *, int);
static U16   ARCH_DEP(d250_addrck)(RADR, RADR, int, BYTE, REGS *);

#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
static int   ARCH_DEP(d250_iorq64)(DEVBLK *, int *, BIOPL_IORQ64 *, REGS *);
/* void *ARCH_DEP(d250_async64)(void *); */
static int   ARCH_DEP(d250_list64)(IOCTL64 *, int);
#endif /* defined(FEATURE_001_ZARCH_INSTALLED_FACILITY) */

/*-------------------------------------------------------------------*/
/* Process Standard Block I/O (Function code 0x250)                  */
/*-------------------------------------------------------------------*/
int ARCH_DEP(vm_blockio) (int r1, int r2, REGS *regs)
{
/* Guest related paramters and values                                */
RADR    biopaddr;                      /* BIOPL address              */

union   parmlist{                      /* BIOPL formats that         */
        BIOPL biopl;                   /* May be supplied by the     */
        BIOPL_INIT32 init32;           /* guest                      */
        BIOPL_IORQ32 iorq32;
        BIOPL_REMOVE remove;
#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
        BIOPL_INIT64 init64;
        BIOPL_IORQ64 iorq64;
#endif /* defined(FEATURE_001_ZARCH_INSTALLED_FACILITY) */
        };
union   parmlist bioplin;              /* BIOPL from/to guest        */

U16     devnum;                        /* Device number              */
DEVBLK *dev;                           /* --> Device block           */
int     rc;                            /* return code in Rx+1        */
int     cc;                            /* condition code             */

    rc = RC_ERROR; /* Initialize the return code to error */
    cc = CC_FAILED; /* Failure assumed unless otherwise successful */

#if 0 // debug
    if (sizeof(BIOPL) != 64)
    {
            LOGMSG( "BIOPL size not 64: %d\n", sizeof( BIOPL ));
    }
    if (sizeof(BIOPL_INIT32) != 64)
    {
            LOGMSG( "BIOPL_INIT32 size not 64: %d\n", sizeof( BIOPL_INIT32 ));
    }
    if (sizeof(BIOPL_INIT64) != 64)
    {
            LOGMSG( "BIOPL_INIT64 size not 64: %d\n", sizeof( BIOPL_INIT64 ));
    }
    if (sizeof(BIOPL_IORQ32) != 64)
    {
            LOGMSG( "BIOPL_IORQ32 size not 64: %d\n", sizeof( BIOPL_IORQ32 ));
    }
    if (sizeof(BIOPL_REMOVE) != 64)
    {
            LOGMSG( "BIOPL_REMOVE size not 64: %d\n", sizeof( BIOPL_REMOVE ));
    }
    if (sizeof(BIOPL_IORQ64) != 64)
    {
            LOGMSG( "BIOPL_IORQ64 size not 64: %d\n", sizeof( BIOPL_IORQ64 ));
    }
    if (sizeof(BIOE32) != 16)
    {
            LOGMSG( "BIOE32 size not 16: %d\n", sizeof( BIOE32 ));
    }
    if (sizeof(BIOE64) != 24)
    {
            LOGMSG( "BIOE64 size not 24: %d\n", sizeof( BIOE64 ));
    }
#endif

    /* Retrieve the BIOPL address from R1 */
    biopaddr = regs->GR(r1);

    /* Specification exception if the BIOPL is not on a doubleword boundary */
    if (biopaddr & 0x00000007)
    {
        ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
    }

    /* Fetch the BIOPL from guest storage */
    ARCH_DEP(wfetchc) (&bioplin, sizeof(bioplin)-1,
                       biopaddr, USE_REAL_ADDR, regs);

    /* Access the targeted device number from the BIOPL*/
    FETCH_HW(devnum,&bioplin.biopl.devnum);

    /* Locate the device by the number */
    dev = find_device_by_devnum (0,devnum);
    /* Device not found will be checked by the called function */

    switch(regs->GR_L(r2))
    {

/*--------------------------------------------------------*/
/* Initialize the Block I/O Device Environment            */
/*--------------------------------------------------------*/
    case INIT:

#if !defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
         /* 64-bit formats not supported for S/370 or ESA/390 */
         /* and bits 1-7 must be zero                         */
         if (bioplin.biopl.flaga != 0x00)
         {
             ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
         }

         /* Call the 32-bit addressing function */
         cc = d250_init32(dev,&rc,&bioplin.init32,regs);
#else
         /* Bits 1-7 must be zero for z/Architecture */
         if (bioplin.biopl.flaga & BIOPL_FLAGARSV)
         {
             ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
         }
         /* Call the addressing sensitive function */
         if (bioplin.biopl.flaga & BIOPL_FLAGAMSK)
         {
            cc = d250_init64(dev,&rc,&bioplin.init64,regs);
         }
         else
         {
            cc = d250_init32(dev,&rc,&bioplin.init32,regs);
         }
#endif /* !FEATURE_001_ZARCH_INSTALLED_FACILITY */
         break;

/*--------------------------------------------------------*/
/* Perform block I/O read/write requests to the device    */
/*--------------------------------------------------------*/
    case IOREQ:

#if !defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)
         /* 64-bit formats not supported for S/370 or ESA/390 */
         /* and bits 1-7 must be zero                         */
         if (bioplin.biopl.flaga != 0x00)
         {
             ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
         }
         cc = ARCH_DEP(d250_iorq32)(dev,&rc,&bioplin.iorq32,regs);
#else
         /* Bits 1-7 must be zero for z/Architecture */
         if (bioplin.biopl.flaga & BIOPL_FLAGARSV)
         {
             ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
         }

         if (bioplin.biopl.flaga & BIOPL_FLAGAMSK)
         {
            cc = ARCH_DEP(d250_iorq64)(dev,&rc,&bioplin.iorq64,regs);
         }
         else
         {
            cc = ARCH_DEP(d250_iorq32)(dev,&rc,&bioplin.iorq32,regs);
         }
#endif /* !FEATURE_001_ZARCH_INSTALLED_FACILITY */
         break;

/*--------------------------------------------------------*/
/* Remove the Block I/O Device Environment                */
/*--------------------------------------------------------*/
    case REMOVE:
         cc = d250_remove(dev,&rc,&bioplin.remove,regs);
         break;
    default:
         ARCH_DEP(program_interrupt)(regs, PGM_SPECIFICATION_EXCEPTION);
    } /* end switch(regs->GR_L(r2)) */

    /* Update the BIOPL in main storage */
    ARCH_DEP(wstorec) (&bioplin, sizeof(bioplin)-1,
                       biopaddr, USE_REAL_ADDR, regs);

    /* Set the return code in Rx+1 */
    regs->GR_L((r1+1)&0xF) = rc;

    /* Return the condition code */
    return cc;

} /* end function vm_blockio */

/*-------------------------------------------------------------------*/
/*  Asynchronous Input/Outut 32-bit Driver Thread                    */
/*-------------------------------------------------------------------*/
static void *ARCH_DEP(d250_async32)(void *ctl)
{
IOCTL32 *ioctl;    /* 32-bit IO request controls    */
BYTE    psc;       /* List processing status code   */

   /* Fetch the IO request control structure */
   ioctl=(IOCTL32 *)ctl;

   /* Call the 32-bit BIOE request processor on this async thread*/
   psc=ARCH_DEP(d250_list32)(ioctl, ASYNC);

   /* Trigger the external interrupt here */

   d250_bio_interrupt(ioctl->dev, ioctl->intrparm, psc, 0x03);

   free(ioctl);
   return NULL;
} /* end function ARCH_DEP(d250_async32) */

/*-------------------------------------------------------------------*/
/*  Input/Output Request - 32-bit Addressing                         */
/*-------------------------------------------------------------------*/
static int ARCH_DEP(d250_iorq32)(DEVBLK *dev, int *rc, BIOPL_IORQ32 *biopl,
                REGS *regs)
{
BIOPL_IORQ32   bioplx00;  /* Used to check reserved fields */
IOCTL32 ioctl;            /* Request information */
BYTE    psc;              /* List processing status code */

/* Asynchronous request related fields */
TID     tid;         /* Asynchronous thread ID */
char    tname[32];   /* Thread name */
IOCTL32 *asyncp;     /* Pointer to async thread's storage */
int     rc2;

   /* Clear the reserved BIOPL */
   memset(&bioplx00,0,sizeof(BIOPL_IORQ32));

   /* Make sure reserved fields and bits are binary zeros  */
   if ((memcmp(&biopl->resv1,&bioplx00,IORQ32R1_LEN)!=0) ||
       (memcmp(&biopl->resv2,&bioplx00,IORQ32R2_LEN)!=0) ||
       (memcmp(&biopl->resv3,&bioplx00,IORQ32R3_LEN)!=0) ||
       (biopl->flags & BIOPL_FLAGSRSV) ||
       (biopl->key & BIOPL_KEYRSV)
      )
   {
       ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
   }

   /* Return with an error return code if the device does not exist */
   if (!dev)
   {
      *rc = RC_NODEV;  /* Set the return code for no device */
      return CC_FAILED; /* Indicate the function failed     */
   }

   /* If no environment, return with an error */
   if (!(dev->vmd250env))
   {
      *rc = RC_STATERR;
      return CC_FAILED;
   }

   /* Fetch the block count from the BIOPL */
   FETCH_FW(ioctl.blkcount,&biopl->blkcount);

   /* Block count must be between 1 and 256, inclusive */
   if ((ioctl.blkcount<1) || (ioctl.blkcount>256))
   {
       *rc = RC_CNT_ERR;
       return CC_FAILED;
   }

   /* Fetch the address of the BIO entry list from the BIOPL */
   FETCH_FW(ioctl.listaddr,&biopl->bioeladr);

   /* Extract the storage key from the BIOPL */
   ioctl.key=biopl->key;

   /* Set the structures that are involved in this request */
   ioctl.dev = dev;
   ioctl.regs = regs;

   /* Set I/O success/failure counts to zero */
   ioctl.goodblks = 0;
   ioctl.badblks = 0;

   if (biopl->flags & BIOPL_ASYNC)
   {
       /* Build the request structure */

       /* Extract the 32-bit interrupt parameter from the BIOPL */
       FETCH_FW(ioctl.intrparm,biopl->intparm);

       if (dev->ccwtrace)
       {
          WRMSG(HHC01924, "I",
                   dev->devnum,
                   ioctl.listaddr,
                   ioctl.blkcount,
                   ioctl.key,
                   ioctl.intrparm);
       }

       /* Set the default status code to an aborted list */
       /* Note: This should be set correctly from the returned PSC */
       ioctl.statuscod = PSC_STGERR;

       /* Get the storage for the thread's parameters */
       if (!(asyncp=(IOCTL32 *)malloc(sizeof(IOCTL32))))
       {
          char buf[40];
          MSGBUF(buf, "malloc(%d)", (int)sizeof(IOCTL32));
          WRMSG (HHC01908, "E", buf, strerror(errno));
          *rc = RC_ERROR;
          return CC_FAILED;
       }

       /* Copy the thread's parameters to its own storage */
       memcpy(asyncp,&ioctl,sizeof(IOCTL32));

       /* Launch the asynchronous request on a separate thread */
       MSGBUF(tname,"d250_async %4.4X",dev->devnum);
       tname[sizeof(tname)-1]=0;
       rc2 = create_thread (&tid, DETACHED, ARCH_DEP(d250_async32),
               asyncp, tname);
       if(rc2)
       {
          WRMSG (HHC00102, "E", strerror(rc2));
          RELEASE_DEVLOCK( dev );
          *rc = RC_ERROR;
          return CC_FAILED;
       }
       /* Launched the async request successfully */
       *rc = RC_ASYNC;
       return CC_SUCCESS;
   }
   else
   {
       /* Perform the I/O request synchronously on this thread */
       /* Call the 32-bit BIOE request processor */
       if (dev->ccwtrace)
       {
          WRMSG(HHC01925, "I",
                   dev->devnum,
                   ioctl.listaddr,
                   ioctl.blkcount,
                   ioctl.key);
       }

       psc=ARCH_DEP(d250_list32)(&ioctl, SYNC);

       if (dev->ccwtrace)
       {
          WRMSG(HHC01926, "I", dev->devnum,psc,ioctl.goodblks,ioctl.badblks);
       }

   }

   /* Processor status used to determine return and condition codes */
   switch(psc)
   {
      case PSC_SUCCESS:
         *rc = RC_SUCCESS;
         return CC_SUCCESS;
      case PSC_PARTIAL:
         if (ioctl.goodblks == 0)
         {
            *rc = RC_ALL_BAD;
            return CC_FAILED;
         }
         else
         {
            *rc = RC_SYN_PART;
            return CC_PARTIAL;
         }
      case PSC_REMOVED:
         *rc = RC_REM_PART;
         return CC_PARTIAL;
      default:
         WRMSG (HHC01927, "I", psc);
         *rc = RC_ERROR;
         return CC_FAILED;
   }

} /* end function ARCH_DEP(d250_iorq32) */


/* BIOE Address and Status Handling (32-bit and 64-bit formats)     */
/*                                                                  */
/* The entire BIOE is fetched from storage, but only the status     */
/* field will be updated.  BIOE addresses and block addresses are   */
/* handled the same.  Only key protection and addressing            */
/* exceptions are recognized for these absolute addresses.          */
/* Architecture will determine the maximum address allowed not the  */
/* addressing mode specified in the PSW.  An addressing exception   */
/* will be recognized if a BIOE extends beyond the architecture     */
/* maximum address.                                                 */
/*                                                                  */
/* Setting of reference bit in a storage key is based upon          */
/* inline.h ARCH_DEP(fetch_xxx_absolute) functions                  */
/* Setting of changed bit in a storage key is based upon            */
/* inline.h ARCH_DEP(store_xxx_absolute) functions                  */

/*-------------------------------------------------------------------*/
/*  Input/Outut 32-bit List Processor - Sychronicity Independent     */
/*-------------------------------------------------------------------*/
static int ARCH_DEP(d250_list32)(IOCTL32* ioctl, int async)
/* WARNING: Device Block lock must be released before returning */
{
BIOE32 bioe;      /* 32-bit BIOE fetched from absolute storage */
RADR   bioebeg;   /* Starting address of the BIOE              */
RADR   bioeend;   /* Address of last byte of BIOE              */
U16    xcode;     /* Detected exception condition              */
int    blocks;    /* Number of blocks being processed          */
int    block;     /* counter used in block I/O loop            */
S32    blknum;    /* Block number of the request               */
BYTE   status;    /* Returned BIOE status                      */
/* Passed to generic block I/O function                        */
int    physblk;   /* Physical block number                     */
RADR   bufbeg;    /* Address where the read/write will occur   */
RADR   bufend;    /* Last byte read or written                 */

   xcode = 0;   /* Initialize the address check exception code */
   status = 0;

   /* Preserve pending sense if any and establish my ownership */
   /* of the device by reserving it if shared and locking it   */
   if (ioctl->dev->ccwtrace)
   {
      WRMSG (HHC01928, "I",
               ioctl->dev->devnum,
               ioctl->blkcount,
               (RADR)ioctl->listaddr,
               ioctl->key
               );
   }

   /* Take ownership of the device */
   d250_preserve(ioctl->dev);
   /* Note: the DEVBLK is now locked */

   if (!ioctl->dev->vmd250env)
   {
       d250_restore(ioctl->dev);
       /* Note: the device lock is now released */
       return PSC_REMOVED;
   }

   blocks=(int)ioctl->blkcount;
   bioebeg=ioctl->listaddr & AMASK31 ;

   /* Process each of the BIOE's supplied by the BIOPL count field */
   for ( block = 0 ; block < blocks ; block++ )
   {
      status = 0xFF;  /* Set undefined status */

      bioeend=( bioebeg + sizeof(BIOE32) - 1 ) & AMASK31;
      xcode=ARCH_DEP(d250_addrck)
            (bioebeg,bioeend,ACCTYPE_READ,ioctl->key,ioctl->regs);
      if (ioctl->dev->ccwtrace)
      {
         WRMSG(HHC01929,"I",ioctl->dev->devnum,xcode,bioebeg,bioeend,ioctl->key);
      }
      if ( xcode )
      {
         break;
      }

      /* Fetch the BIOE from storage */
      memcpy(&bioe,ioctl->regs->mainstor+bioebeg,sizeof(BIOE32));
      ARCH_DEP( or_storage_key )( bioebeg, STORKEY_REF );
      ARCH_DEP( or_storage_key )( bioeend, STORKEY_REF );

      /* Process a single BIOE */
      do
      {

         /* Make sure reserved field is zeros */
         if ( bioe.resv1[0]!=0x00 || bioe.resv1[1]!=0x00 )
         {
            status=BIOE_NOTZERO;
            continue;
         }

         /* Fetch and validate block number */
         FETCH_FW(blknum,&bioe.blknum);
         if ( (blknum < ioctl->dev->vmd250env->begblk) ||
              (blknum > ioctl->dev->vmd250env->endblk)
            )
         {
            status=BIOE_BADBLOCK;
            continue;
         }

         /* Fetch the storage address used for I/O */
         FETCH_FW(bufbeg,&bioe.bufaddr);
         bufbeg &= AMASK31;

         /* Ensure the environment still exists */
         if (!ioctl->dev->vmd250env)
         {
            d250_restore(ioctl->dev);
            /* Note: the device lock is now released */
            status=BIOE_ABORTED;
            return PSC_REMOVED;
         }

         /* The I/O handler routines are normally called without the  */
         /* device lock being held.  The device is reserved by the    */
         /* busy status.                                              */

         /* Determine the last byte of the I/O buffer */
         bufend=( bufbeg + ioctl->dev->vmd250env->blksiz -1 ) & AMASK31 ;

         if (ioctl->dev->ccwtrace)
         {
            WRMSG (HHC01930, "I",
                     ioctl->dev->devnum,
                     bioebeg,
                     bioe.type,
                     blknum,
                     bufbeg
                    );
         }

         /* Determine the physical block on the device relative to zero */
         physblk=(S64)blknum+ioctl->dev->vmd250env->offset-1;
         /* The read/write routines will convert this to a physical disk */
         /* location for reading or writing                              */

         if (bioe.type == BIOE_READ)
         {
            xcode=ARCH_DEP(d250_addrck)
                  (bufbeg,bufend,ACCTYPE_READ,ioctl->key,ioctl->regs);
            if (ioctl->dev->ccwtrace)
            {
               WRMSG(HHC01931, "I",
                       ioctl->dev->devnum,xcode,bufbeg,bufend,ioctl->key);
            }
            switch ( xcode )
            {
               case PGM_ADDRESSING_EXCEPTION:
                  status=BIOE_ADDREXC;
                  continue;
               case PGM_PROTECTION_EXCEPTION:
                  status=BIOE_PROTEXC;
                  continue;
            }
            /* At this point, the block number has been validated */
            /* and the buffer is addressable and accessible       */
            status=d250_read(ioctl->dev,
                               physblk,
                               ioctl->dev->vmd250env->blksiz,
                               ioctl->regs->mainstor+bufbeg);

            /* Set I/O storage key references if successful */
            if (!status)
            {
               ARCH_DEP( or_storage_key )( bufbeg, STORKEY_REF );
               ARCH_DEP( or_storage_key )( bufend, STORKEY_REF );

#if defined(FEATURE_2K_STORAGE_KEYS)
               if ( ioctl->dev->vmd250env->blksiz == 4096 )
               {
                  ARCH_DEP( or_storage_key )( bufbeg+2048, STORKEY_REF );
               }
#endif
            }

            continue;
         }  /* end of BIOE_READ */
         else
         {  if (bioe.type == BIOE_WRITE)
            {
               xcode=ARCH_DEP(d250_addrck)
                     (bufbeg,bufend,ACCTYPE_WRITE,ioctl->key,ioctl->regs);
               if (ioctl->dev->ccwtrace)
               {
                  WRMSG(HHC01932, "I",
                           ioctl->dev->devnum,
                           xcode,bufbeg,
                           bufend,
                           ioctl->key);
               }

               switch ( xcode )
               {
                  case PGM_ADDRESSING_EXCEPTION:
                     status=BIOE_ADDREXC;
                     continue;
                  case PGM_PROTECTION_EXCEPTION:
                     status=BIOE_PROTEXC;
                     continue;
               }

               if (ioctl->dev->vmd250env->isRO)
               {
                  status=BIOE_DASDRO;
                  continue;
               }
               status=d250_write(ioctl->dev,
                                   physblk,
                                   ioctl->dev->vmd250env->blksiz,
                                   ioctl->regs->mainstor+bufbeg);

               /* Set I/O storage key references if good I/O */
               if (!status)
               {
                  ARCH_DEP( or_storage_key )( bufbeg, (STORKEY_REF | STORKEY_CHANGE) );
                  ARCH_DEP( or_storage_key )( bufend, (STORKEY_REF | STORKEY_CHANGE) );
#if defined(FEATURE_2K_STORAGE_KEYS)
                  if ( ioctl->dev->vmd250env->blksiz == 4096 )
                  {
                     ARCH_DEP( or_storage_key )( bufbeg+2048, (STORKEY_REF | STORKEY_CHANGE) );
                  }
#endif
               }

               continue;
            } /* end of if BIOE_WRITE */
            else
            {
               status=BIOE_BADREQ;
               continue;
            } /* end of else BIOE_WRITE */
         } /* end of else BIOE_READ */
      }while(0); /* end of do */

      /* Determine if we can store the status in the BIOE */
      xcode=ARCH_DEP(d250_addrck)
            (bioebeg+1,bioebeg+1,ACCTYPE_WRITE,ioctl->key,ioctl->regs);
      if (ioctl->dev->ccwtrace)
      {
         WRMSG(HHC01933, "I",
                  ioctl->dev->devnum,xcode,bioebeg+1,bioebeg+1,ioctl->key);
      }

      /* If the status byte is store protected, give up on processing any */
      /* more BIOE's.  Leave the BIOE list process for loop               */
      if ( xcode )
      {
         break;
      }

      /* Store the status in the BIOE */
      memcpy(ioctl->regs->mainstor+bioebeg+1,&status,1);

      /* Set the storage key change bit */
      ARCH_DEP( or_storage_key )( bioebeg+1, (STORKEY_REF | STORKEY_CHANGE) );

      if (ioctl->dev->ccwtrace)
      {
         WRMSG (HHC01934, "I", ioctl->dev->devnum,bioebeg,status);
      }

      /* Count if this BIOE was a success or failure */
      if ( status )
      {
         ioctl->badblks+=1;
         if ( status == BIOE_ABORTED )
         {
             break;
         }
      }
      else
      {
         ioctl->goodblks+=1;
      }

      /* Determine the address of the next BIOE */
      bioebeg += sizeof(BIOE32);
      bioebeg &= AMASK31;
   } /* end of for loop */

#if 0
   LOGMSG( "(d250_list32) BIOE's processed: %d\n", block );
#endif

   /* Restore device to guest ownership */
   d250_restore(ioctl->dev);
   /* Note: device lock not held */

   /* If an access exception occurred:                                 */
   /*   If this is a synchronous request, generate a program exception */
   /*   or if this is asynchrnous, just return with a storage error    */
   if ( xcode )
   {
      if (async)
          return PSC_STGERR;
      else
          ARCH_DEP(program_interrupt)(ioctl->regs, xcode);
   }

   if ( status == BIOE_ABORTED )
   {
      return PSC_REMOVED;
   }

   /* Determine if we were completely successful or only partially     */
   /* successful.  'Partial' includes none successful.                 */
   /* Synchronous and asynchronous requests handle all failed          */
   /* differently. The good and bad blocks field are used by the       */
   /* caller                                                           */
   if (ioctl->goodblks < blocks)
   {
      return PSC_PARTIAL;
   }
   return PSC_SUCCESS;

} /* end function d250_list32 */

/*-------------------------------------------------------------------*/
/*  Absolue Address Checking without Reference and Change Recording  */
/*-------------------------------------------------------------------*/
static U16 ARCH_DEP(d250_addrck)
            (RADR beg, RADR end, int acctype, BYTE key, REGS *regs)
/* Note: inline.h and vstore.h functions are not used because they      */
/* will generate program exceptions automatically.  DIAGNOSE X'250' in  */
/* the asynchronous case, must not do that, but rather reflect the      */
/* error in the interrupt status code or BIOE status field.  So this    */
/* function only detects and reports the error.                         */
/* The caller must decide whether to generate a program interrupt or    */
/* just report the encountered error without a program interruption     */
/* and must set the reference and change bit as appropriate             */
{
BYTE   sk1;      /* Storage key of first byte of area         */
BYTE   sk2;      /* Storage key of last byte of area          */
#if defined(FEATURE_2K_STORAGE_KEYS)
BYTE   skmid;    /* Storage key of middle byte of area        */
#endif /* defined(FEATURE_2K_STORAGE_KEYS) */


    if ( (end > regs->mainlim) || (end > MAXADDRESS) || end < beg )
    {
       return PGM_ADDRESSING_EXCEPTION;
    }

    /* Note this logic is inspired by        */
    /* inline.h ARCH_DEP(is_fetch_protected) */
    /* inline.h ARCH_DEP(is_store_protected) */

    if (key == 0)
    {
       return 0;
    }

    sk1 = ARCH_DEP( get_storage_key )( beg );
    sk2 = ARCH_DEP( get_storage_key )( end );

#if defined(FEATURE_2K_STORAGE_KEYS)
    if ( ( end - beg ) > 2048 )
    {
       skmid = ARCH_DEP( get_storage_key )( beg + 2048 );
    }
    else
    {
       skmid = sk2;
    }
#endif /* defined(FEATURE_2K_STORAGE_KEYS) */

    if (acctype == ACCTYPE_READ)
    {  /* Check for fetch protection  */
       if (  ((sk1 & STORKEY_FETCH) && (key != (sk1 & STORKEY_KEY)))
           ||((sk2 & STORKEY_FETCH) && (key != (sk2 & STORKEY_KEY)))
#if defined(FEATURE_2K_STORAGE_KEYS)
           ||((skmid & STORKEY_FETCH) && (key != (skmid & STORKEY_KEY)))
#endif /* defined(FEATURE_2K_STORAGE_KEYS) */
          )
       {
          return PGM_PROTECTION_EXCEPTION;
       }
    }
    else /* assume ACCTYPE_WRITE */
    {  /* Check for store protection */
       if (  (key != (sk1 & STORKEY_KEY))
           ||(key != (sk2 & STORKEY_KEY))
#if defined(FEATURE_2K_STORAGE_KEYS)
           ||(key != (skmid & STORKEY_KEY))
#endif /* defined(FEATURE_2K_STORAGE_KEYS) */
          )
       {
          return PGM_PROTECTION_EXCEPTION;
       }
    }
    return 0;
} /* end of function ARCH_DEP(d250_addrck) */


#if defined(FEATURE_001_ZARCH_INSTALLED_FACILITY)

/*-------------------------------------------------------------------*/
/*  Asynchronous Input/Output 64-bit Driver Thread                   */
/*-------------------------------------------------------------------*/
static void* ARCH_DEP(d250_async64)(void *ctl)
{
IOCTL64 *ioctl;    /* 64-bit IO request controls  */
BYTE     psc;      /* List processing status code */

   /* Fetch the IO request control structure */
   ioctl=(IOCTL64 *)ctl;

   /* Call the 32-bit BIOE request processor on this async thread*/
   psc=ARCH_DEP(d250_list64)(ioctl, ASYNC);

   d250_bio_interrupt(ioctl->dev, ioctl->intrparm, psc, 0x07);

   free(ioctl);
   return NULL;

} /* end function ARCH_DEP(d250_async64) */

/*-------------------------------------------------------------------*/
/*  Input/Output Request - 64-bit Addressing                         */
/*-------------------------------------------------------------------*/
static int ARCH_DEP(d250_iorq64)(DEVBLK *dev, int *rc, BIOPL_IORQ64 *biopl,
             REGS *regs)
{
BIOPL_IORQ64   bioplx00;   /* Used to check reserved fields */
IOCTL64 ioctl;             /* Request information */
BYTE    psc;               /* List processing status code   */

/* Asynchronous request related fields */
TID     tid;         /* Asynchronous thread ID */
char    tname[32];   /* Thread name */
IOCTL64 *asyncp;     /* Pointer to async thread's free standing storage */
int     rc2;

#if 0
   LOGMSG( "(d250_iorq64) Entered\n" );
#endif

   /* Clear the reserved BIOPL */
   memset(&bioplx00,0,sizeof(BIOPL_IORQ64));

   /* Make sure reserved fields are binary zeros  */
   if ((memcmp(&biopl->resv1,&bioplx00,IORQ64R1_LEN)!=0) ||
       (memcmp(&biopl->resv2,&bioplx00,IORQ64R2_LEN)!=0) ||
       (memcmp(&biopl->resv3,&bioplx00,IORQ64R3_LEN)!=0) ||
       (memcmp(&biopl->resv4,&bioplx00,IORQ64R4_LEN)!=0) ||
       (biopl->flags & BIOPL_FLAGSRSV) ||
       (biopl->key & BIOPL_KEYRSV)
      )
   {
       ARCH_DEP(program_interrupt) (regs, PGM_SPECIFICATION_EXCEPTION);
   }

   /* Return with an error return code if the device does not exist */
   if (!dev)
   {
      *rc = RC_NODEV;  /* Set the return code for no device */
      return CC_FAILED; /* Indicate the function failed     */
   }

   /* If no environment, return with an error */
   if (!dev->vmd250env)
   {
      *rc = RC_STATERR;
      return CC_FAILED;
   }

   /* Fetch the block count from the BIOPL */
   FETCH_FW(ioctl.blkcount,&biopl->blkcount);

#if 0
   LOGMSG( "(d250_iorq64) ioctl.blkcount=%d,\n", ioctl.blkcount );
#endif

   /* Block count must be between 1 and 256, inclusive */
   if ((ioctl.blkcount<1) || (ioctl.blkcount>256))
   {
       *rc = RC_CNT_ERR;
       return CC_FAILED;
   }

   /* Fetch the address of the BIO entry list from the BIOPL */
   FETCH_DW(ioctl.listaddr,&biopl->bioeladr);

#if 0
   LOGMSG( "(d250_iorq64) ioctl.listaddr=%16.16X,\n", ioctl.listaddr );
#endif

   /* Extract the storage key from the BIOPL */
   ioctl.key=biopl->key;

   /* Set the structures that are involved in this request */
   ioctl.dev = dev;
   ioctl.regs = regs;

   /* Set I/O success/failure counts to zero */
   ioctl.goodblks = 0;
   ioctl.badblks = 0;

   /* Determine if request is an asynchronous or synchronous */
   if (biopl->flags & BIOPL_ASYNC)
   {
       /* Build the request structure */

       /* Extract the 64-bit interrupt parameter from the BIOPL */
       FETCH_DW(ioctl.intrparm,&biopl->intparm);

       if (dev->ccwtrace)
       {
          WRMSG(HHC01935, "I",
                   dev->devnum,
                   ioctl.listaddr,
                   ioctl.blkcount,
                   ioctl.key,
                   ioctl.intrparm);
       }

       /* Set the default status code to an aborted list */
       /* Note: This should be set correctly from the returned PSC */
       ioctl.statuscod = PSC_STGERR;

       /* Get the storage for the thread's parameters */
       if (!(asyncp=(IOCTL64 *)malloc(sizeof(IOCTL64))))
       {
          char buf[40];
          MSGBUF(buf, "malloc(%d)", (int)sizeof(IOCTL64));
          WRMSG (HHC01908, "E", buf, strerror(errno));
          *rc = RC_ERROR;
          return CC_FAILED;
       }

       /* Copy the thread's parameters to its own storage */
       memcpy(asyncp,&ioctl,sizeof(IOCTL64));

       /* Launch the asynchronous request on a separate thread */
       MSGBUF(tname,"d250_async %4.4X",dev->devnum);
       tname[sizeof(tname)-1]=0;
       rc2 = create_thread (&tid, DETACHED, ARCH_DEP(d250_async64),
               asyncp, tname);
       if(rc2)
       {
          WRMSG (HHC00102, "E", strerror(rc2));
          RELEASE_DEVLOCK( dev );
          *rc = RC_ERROR;
          return CC_FAILED;
       }
       /* Launched the async request successfully */
       *rc = RC_ASYNC;
       return CC_SUCCESS;
   }
   else
   {
       if (dev->ccwtrace)
       {
          WRMSG(HHC01936, "I",
                   dev->devnum,
                   ioctl.listaddr,
                   ioctl.blkcount,
                   ioctl.key);
       }

       psc=ARCH_DEP(d250_list64)(&ioctl, SYNC);

       if (dev->ccwtrace)
       {
          WRMSG(HHC01937, "I", dev->devnum,psc,ioctl.goodblks,ioctl.badblks);
       }
   }

   /* Processor status used to determine return and condition codes */
   switch(psc)
   {
      case PSC_SUCCESS:
         *rc = RC_SUCCESS;
         return CC_SUCCESS;
      case PSC_PARTIAL:
         if (ioctl.goodblks == 0)
         {
            *rc = RC_ALL_BAD;
            return CC_FAILED;
         }
         else
         {
            *rc = RC_SYN_PART;
            return CC_PARTIAL;
         }
      case PSC_REMOVED:
         *rc = RC_REM_PART;
         return CC_PARTIAL;
      default:
         WRMSG (HHC01938, "E", psc);
         *rc = RC_ERROR;
         return CC_FAILED;
   } /* end switch(psc) */

} /* end function d250_iorq64 */

/*-------------------------------------------------------------------*/
/*  Input/Outut 64-bit List Processor - Sychronicity Independent     */
/*-------------------------------------------------------------------*/
static int ARCH_DEP(d250_list64)(IOCTL64* ioctl, int async)
/* WARNING: Device Block lock must be released before returning */
{
BIOE64 bioe;      /* 32-bit BIOE fetched from absolute storage */
RADR   bioebeg;   /* Starting address of the BIOE              */
RADR   bioeend;   /* Address of last byte of BIOE              */
U16    xcode;     /* Detected exception condition              */
int    blocks;    /* Number of blocks being processed          */
int    block;     /* counter used in block I/O loop            */
S64    blknum;    /* Block number of the request               */
BYTE   status;    /* Returned BIOE status                      */
/* Passed to generic block I/O function                        */
int    physblk;   /* Physical block number                     */
RADR   bufbeg;    /* Address where the read/write will occur   */
RADR   bufend;    /* Last byte read or written                 */


   xcode = 0;   /* Initialize the address check exception code */
   status = 0;

   /* Preserve pending sense if any and establish my ownership */
   /* of the device by reserving it if shared and locking it   */
   if (ioctl->dev->ccwtrace)
   {
      WRMSG (HHC01939, "I",
               ioctl->dev->devnum,
               ioctl->blkcount,
               (RADR)ioctl->listaddr,
               ioctl->key
               );
   }

   /* Take ownership of the device */
   d250_preserve(ioctl->dev);
   /* Note: the DEVBLK is now locked */

   if (!ioctl->dev->vmd250env)
   {
       d250_restore(ioctl->dev);
       /* Note: the device lock is now released */
       return PSC_REMOVED;
   }

   blocks=(int)ioctl->blkcount;
   bioebeg=ioctl->listaddr & AMASK64 ;

   /* Process each of the BIOE's supplied by the BIOPL count field */
   for ( block = 0 ; block < blocks ; block++ )
   {
      status = 0xFF;  /* Set undefined status */

      bioeend=( bioebeg + sizeof(BIOE32) - 1 ) & AMASK31;
      xcode=ARCH_DEP(d250_addrck)
            (bioebeg,bioeend,ACCTYPE_READ,ioctl->key,ioctl->regs);
      if (ioctl->dev->ccwtrace)
      {
         WRMSG(HHC01940, "I",ioctl->dev->devnum,xcode,bioebeg,bioeend,ioctl->key);
      }
      if ( xcode )
      {
         break;
      }

      /* Fetch the BIOE from storage */
      memcpy(&bioe,ioctl->regs->mainstor+bioebeg,sizeof(BIOE64));
      ARCH_DEP( or_storage_key )( bioebeg, STORKEY_REF );
      ARCH_DEP( or_storage_key )( bioeend, STORKEY_REF );

      /* Process a single BIOE */
      do
      {

         /* Make sure reserved field is zeros */
         if ( bioe.resv1[0]!=0x00 || bioe.resv1[1]!=0x00 )
         {
            status=BIOE_NOTZERO;
            continue;
         }

         /* Fetch and validate block number */
         FETCH_DW(blknum,&bioe.blknum);
         if ( (blknum < ioctl->dev->vmd250env->begblk) ||
              (blknum > ioctl->dev->vmd250env->endblk)
            )
         {
            status=BIOE_BADBLOCK;
            continue;
         }

         /* Fetch the storage address used for I/O */
         FETCH_DW(bufbeg,&bioe.bufaddr);
         bufbeg &= AMASK64;

         /* Ensure the environment still exists */
         if (!ioctl->dev->vmd250env)
         {
            d250_restore(ioctl->dev);
            /* Note: the device lock is now released */
            status=BIOE_ABORTED;
            return PSC_REMOVED;
         }

         /* The I/O handler routines are normally called without the  */
         /* device lock being held.  The device is reserved by the    */
         /* busy status.                                              */

         /* Determine the last byte of the I/O buffer */
         bufend=( bufbeg + ioctl->dev->vmd250env->blksiz -1 ) & AMASK64 ;

         if (ioctl->dev->ccwtrace)
         {
            WRMSG (HHC01941, "I",
                     ioctl->dev->devnum,
                     bioebeg,
                     bioe.type,
                     blknum,
                     bufbeg
                    );
         }

         /* Determine the physical block on the device relative to zero */
         physblk=(S64)blknum+ioctl->dev->vmd250env->offset-1;
         /* The read/write routines will convert this to a physical disk */
         /* location for reading or writing                              */

         if (bioe.type == BIOE_READ)
         {
            xcode=ARCH_DEP(d250_addrck)
                  (bufbeg,bufend,ACCTYPE_READ,ioctl->key,ioctl->regs);
            if (ioctl->dev->ccwtrace)
            {
               WRMSG(HHC01942,"I",
                       ioctl->dev->devnum,xcode,bufbeg,bufend,ioctl->key);
            }
            switch ( xcode )
            {
               case PGM_ADDRESSING_EXCEPTION:
                  status=BIOE_ADDREXC;
                  continue;
               case PGM_PROTECTION_EXCEPTION:
                  status=BIOE_PROTEXC;
                  continue;
            }
            /* At this point, the block number has been validated */
            /* and the buffer is addressable and accessible       */
            status=d250_read(ioctl->dev,
                               physblk,
                               ioctl->dev->vmd250env->blksiz,
                               ioctl->regs->mainstor+bufbeg);

            /* Set I/O storage key references if successful */
            if (!status)
            {
               ARCH_DEP( or_storage_key )( bufbeg, STORKEY_REF );
               ARCH_DEP( or_storage_key )( bufend, STORKEY_REF );
            }

            continue;
         }  /* end of BIOE_READ */
         else
         {  if (bioe.type == BIOE_WRITE)
            {
               xcode=ARCH_DEP(d250_addrck)
                     (bufbeg,bufend,ACCTYPE_WRITE,ioctl->key,ioctl->regs);
               if (ioctl->dev->ccwtrace)
               {
                  WRMSG(HHC01943, "I",
                           ioctl->dev->devnum,
                           xcode,bufbeg,
                           bufend,
                           ioctl->key);
               }

               switch ( xcode )
               {
                  case PGM_ADDRESSING_EXCEPTION:
                     status=BIOE_ADDREXC;
                     continue;
                  case PGM_PROTECTION_EXCEPTION:
                     status=BIOE_PROTEXC;
                     continue;
               }

               if (ioctl->dev->vmd250env->isRO)
               {
                  status=BIOE_DASDRO;
                  continue;
               }
               status=d250_write(ioctl->dev,
                                   physblk,
                                   ioctl->dev->vmd250env->blksiz,
                                   ioctl->regs->mainstor+bufbeg);
               /* Set I/O storage key references if good I/O */
               if (!status)
               {
                  ARCH_DEP( or_storage_key )( bufbeg, (STORKEY_REF | STORKEY_CHANGE) );
                  ARCH_DEP( or_storage_key )( bufend, (STORKEY_REF | STORKEY_CHANGE) );
               }
               continue;
            } /* end of if BIOE_WRITE */
            else
            {
               status=BIOE_BADREQ;
               continue;
            } /* end of else BIOE_WRITE */
         } /* end of else BIOE_READ */
      }while(0); /* end of do */

      /* Determine if we can store the status in the BIOE */
      xcode=ARCH_DEP(d250_addrck)
            (bioebeg+1,bioebeg+1,ACCTYPE_WRITE,ioctl->key,ioctl->regs);
      if (ioctl->dev->ccwtrace)
      {
         WRMSG(HHC01944,"I",ioctl->dev->devnum,xcode,bioebeg+1,bioebeg+1,ioctl->key);
      }

      /* If the status byte is store protected, give up on processing any */
      /* more BIOE's.  Leave the BIOE list process for loop               */
      if ( xcode )
      {
         break;
      }

      /* Store the status in the BIOE */
      memcpy(ioctl->regs->mainstor+bioebeg+1,&status,1);

      /* Set the storage key change bit */
      ARCH_DEP( or_storage_key )( bioebeg+1, (STORKEY_REF | STORKEY_CHANGE) );

      if (ioctl->dev->ccwtrace)
      {
         WRMSG (HHC01945,"I",ioctl->dev->devnum,bioebeg,status);
      }

      /* Count if this BIOE was a success or failure */
      if ( status )
      {
         ioctl->badblks+=1;
         if ( status == BIOE_ABORTED )
         {
             break;
         }
      }
      else
      {
         ioctl->goodblks+=1;
      }

      /* Determine the address of the next BIOE */
      bioebeg += sizeof(BIOE64);
      bioebeg &= AMASK64;
   } /* end of for loop */

#if 0 // remove after testing
   LOGMSG( "(d250_list64) BIOE's processed: %d\n", block );
#endif

   /* Restore device to guest ownership */
   d250_restore(ioctl->dev);
   /* Note: device lock not held */

   /* If an access exception occurred:                                 */
   /*   If this is a synchronous request, generate a program exception */
   /*   or if this is asynchrnous, just return with a storage error    */
   if ( xcode )
   {
      if (async)
          return PSC_STGERR;
      else
          ARCH_DEP(program_interrupt)(ioctl->regs, xcode);
   }

   if ( status == BIOE_ABORTED )
   {
      return PSC_REMOVED;
   }

   /* Determine if we were completely successful or only partially     */
   /* successful.  'Partial' includes none successful.                 */
   /* Synchronous and asynchronous requests handle all failed          */
   /* differently. The good and bad blocks field are used by the       */
   /* caller                                                           */
   if (ioctl->goodblks < blocks)
   {
      return PSC_PARTIAL;
   }
   return PSC_SUCCESS;

} /* end function ARCH_DEP(d250_list64) */

#endif /* defined(FEATURE_001_ZARCH_INSTALLED_FACILITY) */

#endif /*FEATURE_VM_BLOCKIO*/

#if !defined(_GEN_ARCH)

#if defined(_ARCH_NUM_1)
 #define  _GEN_ARCH _ARCH_NUM_1
 #include "vmd250.c"
#endif

#if defined(_ARCH_NUM_2)
 #undef   _GEN_ARCH
 #define  _GEN_ARCH _ARCH_NUM_2
 #include "vmd250.c"
#endif

#endif /*!defined(_GEN_ARCH)*/
