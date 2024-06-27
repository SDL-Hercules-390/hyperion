/* ZFCP.C       (C) Copyright Jan Jaeger,   1999-2012                */
/*              Fibre Channel Protocol attached DASD emulation       */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/* This module contains device handling functions for the            */
/* ZFCP Fibre Channel Protocol interface, and will translate all     */
/* ZFCP requests to iSCSI requests                                   */
/*                                                                   */
/* This implementation is based on the S/390 Linux implementation    */
/*                                                                   */
/* Device module hdtzfcp                                             */
/*                                                                   */
/* hercules.cnf:                                                     */
/*   0C00-0C02 ZFCP <optional parameters>                            */
/* Parameters:                                                       */
/* Optional parms:                                                   */
/*                                                                   */
/*                                                                   */
/*                                                                   */
/* Related commands:                                                 */
/*                                                                   */
/* To specify the location of the SCSI bootstrap loader to be        */
/* used for the IPL                                                  */
/*   HWLDR  SCSIBOOT /usr/local/share/hercules/scsiboot              */
/*                                                                   */
/* Specify WWPN, LUN and other optional SCSI IPL parameters          */
/*   LOADDEV PORTNAME 8001020304050607 LUN 08090A0B0C0D0E0F          */
/*                                                                   */
/*                                                                   */

#include "hstdinc.h"

DISABLE_GCC_UNUSED_SET_WARNING;
DISABLE_GCC_UNUSED_FUNCTION_WARNING;

#include "hercules.h"
#include "devtype.h"
#include "chsc.h"
#include "zfcp.h"
#include "opcode.h"
#include "inline.h"

/*-------------------------------------------------------------------*/
/* ZFCP Debugging                                                    */
/*-------------------------------------------------------------------*/

#define ENABLE_ZFCP_DEBUG   1    // 1:always, 0:never, #undef:maybe

#if (!defined(ENABLE_ZFCP_DEBUG) && defined(DEBUG)) || \
    (defined(ENABLE_ZFCP_DEBUG) && ENABLE_ZFCP_DEBUG)
 #define ZFCP_DEBUG
#endif

#if defined(ZFCP_DEBUG)
 #define  ENABLE_TRACING_STMTS   1       // (Fish: DEBUGGING)
 #include "dbgtrace.h"                   // (Fish: DEBUGGING)
 #define  NO_ZFCP_OPTIMIZE               // (Fish: DEBUGGING) (MSVC only)
#endif

#if defined( _MSVC_ ) && defined( NO_ZFCP_OPTIMIZE )
  #pragma optimize( "", off )           // disable optimizations for reliable breakpoints
#endif

#if defined( WIN32 ) && !defined( _MSVC_ ) && !defined( HDL_USE_LIBTOOL )
  SYSBLK *psysblk;
  #define sysblk (*psysblk)
#endif


/*-------------------------------------------------------------------*/
/* Configuration Data Constants                                      */
/*-------------------------------------------------------------------*/
static const NED  zfcp_device_ned[]  = {ZFCP_DEVICE_NED};
static const NED  zfcp_ctlunit_ned[] = {ZFCP_CTLUNIT_NED};
static const NED  zfcp_token_ned[]   = {ZFCP_TOKEN_NED};
static const NEQ  zfcp_general_neq[] = {ZFCP_GENERAL_NEQ};

static NED  configuration_data[4]; // (initialized by HDL_DEPENDENCY_SECTION)

static const ND  zfcp_nd[] = {ZFCP_ND};
static const NQ  zfcp_nq[] = {ZFCP_NQ};

static ND  node_data[2]; // (initialized by HDL_DEPENDENCY_SECTION)

#define SII_SIZE    sizeof(U32)

static const BYTE sense_id_bytes[] =
{
    0xFF,                               /* Always 0xFF               */
    ZFCP_SNSID_1731_03,                 /* Control Unit type/model   */
    ZFCP_SNSID_1732_03,                 /* I/O Device   type/model   */
    0x00,                               /* Always 0x00               */
    ZFCP_RCD_CIW,                       /* Read Config. Data CIW     */
    ZFCP_SII_CIW,                       /* Set Interface Id. CIW     */
    ZFCP_RNI_CIW,                       /* Read Node Identifier CIW  */
    ZFCP_EQ_CIW,                        /* Establish Queues CIW      */
    ZFCP_AQ_CIW,                        /* Activate Queues CIW       */
};


static BYTE zfcp_immed_commands [256] =
{
/* 0 1 2 3 4 5 6 7 8 9 A B C D E F */
   0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0, /* 00 */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1, /* 10 */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 20 */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 30 */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 40 */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 50 */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 60 */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 70 */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 80 */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* 90 */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* A0 */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* B0 */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* C0 */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* D0 */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, /* E0 */
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0  /* F0 */
};


/*-------------------------------------------------------------------*/
/*   STORCHK  --  check storage access & update ref & change bits    */
/*-------------------------------------------------------------------*/
/*  Returns 0 if successful or CSW_PROGC or CSW_PROTC if error.      */
/*  Storage key ref & change bits are only updated if successful.    */
/*-------------------------------------------------------------------*/
BYTE STORCHK
(
    U64      addr,              /* Storage address being accessed    */
    size_t   len,               /* Length of storage being accessed  */
    BYTE     key,               /* Storage access key                */
    BYTE     acc,               /* Access type (STORKEY_REF/_CHANGE) */
    DEVBLK*  dev                /* Pointer to device block           */
)
{
    /* Validate address limits */
    if (0
        || (addr + len) > dev->mainlim   // (outside main storage)
        || (dev->orb.flag5 & ORB5_A      // (or outside address limits)
            && ((dev->pmcw.flag5 & PMCW5_LM_LOW  && (addr +  0 ) < sysblk.addrlimval) ||
                (dev->pmcw.flag5 & PMCW5_LM_HIGH && (addr + len) > sysblk.addrlimval)))
    )
        return CSW_PROGC;

    /* Validate keyed access */
    if (key && key != (ARCH_DEP( get_dev_4K_storage_key )( dev, addr ) & STORKEY_KEY) // (key doesn't match)
        && (0
            || ARCH_DEP( get_dev_4K_storage_key )( dev, addr ) & STORKEY_FETCH        // (and fetch protected)
            || acc == STORKEY_CHANGE                                                  // (or trying to update)
           )
    )
        return CSW_PROTC;

    if (STORKEY_CHANGE == acc)
        ARCH_DEP( or_dev_4K_storage_key )( dev, addr, (STORKEY_REF | STORKEY_CHANGE) );
    else
        ARCH_DEP( or_dev_4K_storage_key )( dev, addr, STORKEY_REF );

    return 0;  // (0 == success)
}


#if defined( ZFCP_DEBUG )
#define DBGTRC( _dev, ... )                             \
do {                                                    \
  if (((ZFCP_GRP*)((_dev)->group->grp_data))->debug)    \
        TRACE( __VA_ARGS__ );                           \
} while(0)
#else
 #define DBGTRC( _dev, ... )
#endif


#if defined(_FEATURE_QDIO_THININT)
/*-------------------------------------------------------------------*/
/* Set Adapter Local Summary Indicator bits                          */
/*-------------------------------------------------------------------*/
static inline void set_alsi(DEVBLK *dev, BYTE bits)
{
    if(dev->qdio.alsi)
    {
    BYTE *alsi = dev->mainstor + dev->qdio.alsi;

        obtain_lock(&sysblk.mainlock);
        *alsi |= bits;
        ARCH_DEP( or_dev_4K_storage_key )( dev, dev->qdio.alsi, (STORKEY_REF | STORKEY_CHANGE) );
        release_lock(&sysblk.mainlock);
    }
}


/*-------------------------------------------------------------------*/
/* Set Device State Change Indicator bits                            */
/*-------------------------------------------------------------------*/
static inline void set_dsci(DEVBLK *dev, BYTE bits)
{
    if(dev->qdio.dsci)
    {
    BYTE *dsci = dev->mainstor + dev->qdio.dsci;
    BYTE *alsi = dev->mainstor + dev->qdio.alsi;

        obtain_lock(&sysblk.mainlock);
        *dsci |= bits;
        ARCH_DEP( or_dev_4K_storage_key )( dev, dev->qdio.dsci, (STORKEY_REF | STORKEY_CHANGE) );
        *alsi |= bits;
        ARCH_DEP( or_dev_4K_storage_key )( dev, dev->qdio.alsi, (STORKEY_REF | STORKEY_CHANGE) );
        release_lock(&sysblk.mainlock);
    }
}
#endif /*defined(_FEATURE_QDIO_THININT)*/


/*-------------------------------------------------------------------*/
/* Raise Adapter Interrupt                                           */
/*-------------------------------------------------------------------*/
static void raise_adapter_interrupt(DEVBLK *dev)
{
    DBGTRC( dev, "Adapter Interrupt dev(%4.4x)\n", dev->devnum );

    OBTAIN_DEVLOCK( dev );
    {
        dev->pciscsw.flag2 |= SCSW2_Q | SCSW2_FC_START;
        dev->pciscsw.flag3 |= SCSW3_SC_INTER | SCSW3_SC_PEND;
        dev->pciscsw.chanstat = CSW_PCI;
        QUEUE_IO_INTERRUPT( &dev->pciioint, FALSE );
    }
    RELEASE_DEVLOCK( dev );

    /* Update interrupt status */
    OBTAIN_INTLOCK( NULL );
    UPDATE_IC_IOPENDING();
    RELEASE_INTLOCK( NULL );
}


// We must go through the queues/buffers in a round robin manner
// so that buffers are re-used on a LRU (Least Recently Used) basis.
// When no buffers are available we must keep our current position.
// When a buffer becomes available we will advance to that location.
// When we reach the end of the buffer queue we will advance to the
// next available queue.
// When a queue is newly enabled then we will start at the beginning
// of the queue (this is handled in signal adapter).

/*-------------------------------------------------------------------*/
/* Process Input Queue                                               */
/*-------------------------------------------------------------------*/
static void process_input_queue(DEVBLK *dev)
{
ZFCP_GRP *grp = (ZFCP_GRP*)dev->group->grp_data;
int iq = dev->qdio.i_qpos;
int mq = dev->qdio.i_qcnt;

    DBGTRC(dev, "Input Qpos(%d) Bpos(%d)\n",dev->qdio.i_qpos,dev->qdio.i_bpos[dev->qdio.i_qpos]);

    while (mq--)
        if(dev->qdio.i_qmask & (0x80000000 >> iq))
        {
        int ib = dev->qdio.i_bpos[iq];
        QDIO_SLSB *slsb;
        int mb = 128;
            slsb = (QDIO_SLSB*)(dev->mainstor + dev->qdio.i_slsbla[iq]);

            while(mb--)
                if(slsb->slsbe[ib] == SLSBE_INPUT_EMPTY)
                {
                QDIO_SL *sl = (QDIO_SL*)(dev->mainstor + dev->qdio.i_sla[iq]);
                U64 sa; U32 len; BYTE *buf;
                U64 la;
                QDIO_SBAL *sbal;
                int tlen = 0;
                int ns;

                    DBGTRC( dev, "Input Queue(%d) Buffer(%d)\n", iq, ib );

                    FETCH_DW(sa,sl->sbala[ib]);
                    if(STORCHK(sa,sizeof(QDIO_SBAL)-1,dev->qdio.i_slk[iq],STORKEY_REF,dev))
                    {
                        slsb->slsbe[ib] = SLSBE_ERROR;
                        ARCH_DEP( or_dev_4K_storage_key )( dev, dev->qdio.i_slsbla[iq], (STORKEY_REF | STORKEY_CHANGE) );
#if defined(_FEATURE_QDIO_THININT)
                        set_alsi(dev,ALSI_ERROR);
#endif /*defined(_FEATURE_QDIO_THININT)*/
                        grp->reqpci = TRUE;
                        DBGTRC( dev, "STORCHK ERROR sa(%16.16"PRIx64"), key(%2.2x)\n", sa, dev->qdio.i_slk[ iq ]);
                        return;
                    }
                    sbal = (QDIO_SBAL*)(dev->mainstor + sa);

                    for(ns = 0; ns < 16; ns++)
                    {
                        FETCH_DW(la,sbal->sbale[ns].addr);
                        FETCH_FW(len,sbal->sbale[ns].length);
                        if(!len)
                            break;  // Or should this be continue - ie a discontiguous sbal???
                        if(STORCHK(la,len-1,dev->qdio.i_sbalk[iq],STORKEY_CHANGE,dev))
                        {
                            slsb->slsbe[ib] = SLSBE_ERROR;
                            ARCH_DEP( or_dev_4K_storage_key )( dev, dev->qdio.i_slsbla[iq], (STORKEY_REF | STORKEY_CHANGE) );
#if defined(_FEATURE_QDIO_THININT)
                            set_alsi(dev,ALSI_ERROR);
#endif /*defined(_FEATURE_QDIO_THININT)*/
                            grp->reqpci = TRUE;
                            DBGTRC( dev, "STORCHK ERROR la(%16.16"PRIx64"), len(%d), key(%2.2x)\n", la, len, dev->qdio.i_sbalk[ iq ]);
                            return;
                        }
                        buf = (BYTE*)(dev->mainstor + la);

#if 0
                        // ADD CODE TO READ BLOCKS AND SAVE TO THE QUEUES
#endif
                    }

                    if(tlen > 0)
                    {
#if defined(_FEATURE_QDIO_THININT)
                        set_dsci(dev,DSCI_IOCOMP);
#endif /*defined(_FEATURE_QDIO_THININT)*/
                        grp->reqpci = TRUE;
                        slsb->slsbe[ib] = SLSBE_INPUT_COMPLETED;
                        ARCH_DEP( or_dev_4K_storage_key )( dev, dev->qdio.i_slsbla[iq], (STORKEY_REF | STORKEY_CHANGE) );
                        if(++ib >= 128)
                        {
                            ib = 0;
                            dev->qdio.i_bpos[iq] = ib;
                            if(++iq >= dev->qdio.i_qcnt)
                                iq = 0;
                            dev->qdio.i_qpos = iq;
                            mq = dev->qdio.o_qcnt;
                        }
                        dev->qdio.i_bpos[iq] = ib;
                        mb = 128;
                    }
                    else
                    {
                        if(ns)
                            sbal->sbale[ns-1].flags[0] = SBALE_FLAG0_LAST_ENTRY;
                        return;
                    }
                    if(ns)
                        sbal->sbale[ns-1].flags[0] = SBALE_FLAG0_LAST_ENTRY;
                }
                else /* Buffer not empty */
                {
                    if(++ib >= 128)
                    {
                        ib = 0;
                        dev->qdio.i_bpos[iq] = ib;
                        if(++iq >= dev->qdio.i_qcnt)
                            iq = 0;
                        dev->qdio.i_qpos = iq;
                    }
                    dev->qdio.i_bpos[iq] = ib;
                }
        }
        else
            if(++iq >= dev->qdio.i_qcnt)
                iq = 0;

}


/*-------------------------------------------------------------------*/
/* Process Output Queue                                              */
/*-------------------------------------------------------------------*/
static void process_output_queue(DEVBLK *dev)
{
ZFCP_GRP *grp = (ZFCP_GRP*)dev->group->grp_data;
int oq = dev->qdio.o_qpos;
int mq = dev->qdio.o_qcnt;

    while (mq--)
        if(dev->qdio.o_qmask & (0x80000000 >> oq))
        {
        int ob = dev->qdio.o_bpos[oq];
        QDIO_SLSB *slsb;
        int mb = 128;
            slsb = (QDIO_SLSB*)(dev->mainstor + dev->qdio.o_slsbla[oq]);

            while(mb--)
                if(slsb->slsbe[ob] == SLSBE_OUTPUT_PRIMED)
                {
                QDIO_SL *sl = (QDIO_SL*)(dev->mainstor + dev->qdio.o_sla[oq]);
                U64 sa; U32 len; BYTE *buf;
                U64 la;
                QDIO_SBAL *sbal;
                int ns;

                    DBGTRC( dev, "Output Queue(%d) Buffer(%d)\n", oq, ob );

                    FETCH_DW(sa,sl->sbala[ob]);
                    if(STORCHK(sa,sizeof(QDIO_SBAL)-1,dev->qdio.o_slk[oq],STORKEY_REF,dev))
                    {
                        slsb->slsbe[ob] = SLSBE_ERROR;
                        ARCH_DEP( or_dev_4K_storage_key )( dev, dev->qdio.o_slsbla[oq], (STORKEY_REF | STORKEY_CHANGE) );
#if defined(_FEATURE_QDIO_THININT)
                        set_alsi(dev,ALSI_ERROR);
#endif /*defined(_FEATURE_QDIO_THININT)*/
                        grp->reqpci = TRUE;
                        DBGTRC( dev, "STORCHK ERROR sa(%16.16"PRIx64"), key(%2.2x)\n", sa, dev->qdio.o_slk[ oq ]);
                        return;
                    }
                    sbal = (QDIO_SBAL*)(dev->mainstor + sa);

                    for(ns = 0; ns < 16; ns++)
                    {
                        FETCH_DW(la,sbal->sbale[ns].addr);
                        FETCH_FW(len,sbal->sbale[ns].length);
                        if(!len)
                            break;  // Or should this be continue - ie a discontiguous sbal???
                        if(STORCHK(la,len-1,dev->qdio.o_sbalk[oq],STORKEY_REF,dev))
                        {
                            slsb->slsbe[ob] = SLSBE_ERROR;
                            ARCH_DEP( or_dev_4K_storage_key )( dev, dev->qdio.o_slsbla[oq], (STORKEY_REF | STORKEY_CHANGE) );
#if defined(_FEATURE_QDIO_THININT)
                            set_alsi(dev,ALSI_ERROR);
#endif /*defined(_FEATURE_QDIO_THININT)*/
                            grp->reqpci = TRUE;
                            DBGTRC( dev, "STORCHK ERROR la(%16.16"PRIx64"), len(%d), key(%2.2x)\n", la, len, dev->qdio.o_sbalk[ oq ]);
                            return;
                        }
                        buf = (BYTE*)(dev->mainstor + la);

#if 0
                        // ADD CODE TO TAKE BLOCKS OF THE QUEUE AND WRITE
#endif

                        if((sbal->sbale[ns].flags[3] & SBALE_FLAG3_PCI_REQ))
                        {
#if defined(_FEATURE_QDIO_THININT)
                            set_dsci(dev,DSCI_IOCOMP);
#endif /*defined(_FEATURE_QDIO_THININT)*/
                            grp->reqpci = TRUE;
                        }
                    }

                    slsb->slsbe[ob] = SLSBE_OUTPUT_COMPLETED;
                    ARCH_DEP( or_dev_4K_storage_key )( dev, dev->qdio.o_slsbla[oq], (STORKEY_REF | STORKEY_CHANGE) );
                    if(++ob >= 128)
                    {
                        ob = 0;
                        dev->qdio.o_bpos[oq] = ob;
                        if(++oq >= dev->qdio.o_qcnt)
                            oq = 0;
                        dev->qdio.o_qpos = oq;
                        mq = dev->qdio.o_qcnt;
                    }
                    dev->qdio.o_bpos[oq] = ob;
                    mb = 128;
                }
                else
                    if(++ob >= 128)
                    {
                        ob = 0;
                        if(++oq >= dev->qdio.o_qcnt)
                            oq = 0;
                    }

        }
        else
            if(++oq >= dev->qdio.o_qcnt)
                oq = 0;
}


/*-------------------------------------------------------------------*/
/* Halt device handler                                               */
/*-------------------------------------------------------------------*/
static void zfcp_halt_or_clear( DEVBLK* dev )
{
    ZFCP_GRP* grp = (ZFCP_GRP*) dev->group->grp_data;

    /* Signal QDIO end if QDIO is active */
    if (dev->scsw.flag2 & SCSW2_Q)
    {
        dev->scsw.flag2 &= ~SCSW2_Q;
        VERIFY( 1 == write_pipe( grp->ppfd[1], "*", 1 ));
    }
    else
        if (dev->group->acount == ZFCP_GROUP_SIZE)
            signal_condition( &grp->qcond );
}


/*-------------------------------------------------------------------*/
/* Read Configuration Data function                                  */
/*-------------------------------------------------------------------*/
static int zfcp_read_configuration_data( DEVBLK* dev, BYTE* buffer, int bufsz )
{
    int   copylen;
    BYTE  work[ sizeof( configuration_data ) ];

    NED *dev_ned = (NED*)&work[0];  /* Device NED is first       */
    NED *ctl_ned = dev_ned + 1;     /* Control Unit NED is next  */
    NED *tkn_ned = ctl_ned + 1;     /* Token NED is last NED     */
    NEQ *gen_neq = (NEQ*)tkn_ned+1; /* General NEQ always last   */
    DEVBLK *cua;                    /* Our Control Unit device   */

    /* Copy configuration data from tempate */
    memcpy (work, configuration_data, sizeof( work ));

    /* The first device in the group is the control unit */
    cua = dev->group->memdev[0];

    /* Insert the Channel Path ID (CHPID) into all of the NEDs */
    dev_ned->tag[0] = dev->pmcw.chpid[0];
    ctl_ned->tag[0] = cua->pmcw.chpid[0];
    tkn_ned->tag[0] = cua->pmcw.chpid[0];

    /* Insert the device's device number into its device NED. */
    dev_ned->tag[1] = dev->devnum & 0xFF;

    /* Insert the control unit address into the General NEQ */
    gen_neq->iid[0] = cua->pmcw.chpid[0];
    gen_neq->iid[1] = cua->devnum & 0xFF;

    /* Finally, copy the work area into the caller's buffer */
    copylen = bufsz < (int) sizeof( work ) ? bufsz : (int) sizeof( work );
    memcpy( buffer, work, copylen );

    /* Return to them the number of bytes we provided */
    return copylen;
}


/*-------------------------------------------------------------------*/
/* Initialize the device handler                                     */
/*-------------------------------------------------------------------*/
static int zfcp_init_handler ( DEVBLK *dev, int argc, char *argv[] )
{
ZFCP_GRP *grp;
int grouped;
int i;

    LOGMSG( "ZFCP Experimental Driver - Incomplete - Work In Progress\n" );

    if(!dev->group)
    {
        dev->rcd = &zfcp_read_configuration_data;

        dev->numdevid = sizeof(sense_id_bytes);
        memcpy(dev->devid, sense_id_bytes, sizeof(sense_id_bytes));

        dev->devtype = dev->devid[1] << 8 | dev->devid[2];

        dev->pmcw.flag4 |= PMCW4_Q;

        if((!(grouped = group_device(dev,ZFCP_GROUP_SIZE)) && !dev->member) || ZFCP_GROUP_SIZE == 1)
        {
            dev->group->grp_data = grp = malloc(sizeof(ZFCP_GRP));
            memset (grp, 0, sizeof(ZFCP_GRP));

            initialize_condition(&grp->qcond);
            initialize_lock(&grp->qlock);

            /* Open write signalling pipe */
            /* Check your retrun codes, Jan.                         */
            VERIFY(!create_pipe(grp->ppfd));
            grp->ttfd = grp->ppfd[0]; // ZZ TEMP

            /* Set Non-Blocking mode */
            socket_set_blocking_mode(grp->ppfd[0],0);

            /* Allocate reponse buffer */
            grp->rspbf = malloc(RSP_BUFSZ);
            grp->rspsz = 0;

            /* Set defaults */
        }
        else
            grp = dev->group->grp_data;
    }
    else
        grp = dev->group->grp_data;

    // process all command line options here
    for(i = 0; i < argc; i++)
    {
        if(!strcasecmp("portname",argv[i]) && (i+1) < argc)
        {
            if(grp->wwpn)
                free(grp->wwpn);
            grp->wwpn = strdup(argv[++i]);
            continue;
        }
        else if(!strcasecmp("lun",argv[i]) && (i+1) < argc)
        {
            if(grp->lun)
                free(grp->lun);
            grp->lun = strdup(argv[++i]);
            continue;
        }
        else if(!strcasecmp("br_lba",argv[i]) && (i+1) < argc)
        {
            if(grp->brlba)
                free(grp->brlba);
            grp->brlba = strdup(argv[++i]);
            continue;
        }
        else if(!strcasecmp("chpid",argv[i]) && (i+1) < argc)
        {
            int chpid;
            char c;
            if(sscanf(argv[++i], "%x%c", &chpid, &c) != 1 || chpid < 0x00 || chpid > 0xFF)
                LOGMSG( "ZFCP: Invalid channel path id %s for device %4.4X\n", argv[i], dev->devnum );

            else
                dev->pmcw.chpid[0] = chpid;
            continue;
        }
        else
#if defined(ZFCP_DEBUG) || defined(IFF_DEBUG)
        if(!strcasecmp("debug",argv[i]))
        {
            grp->debug = 1;
            continue;
        }
        else
        if(!strcasecmp("nodebug",argv[i]))
        {
            grp->debug = 0;
            continue;
        }
        else
#endif
            LOGMSG( "ZFCP: Invalid option %s for device %4.4X\n", argv[i], dev->devnum );

    }

#if ZFCP_GROUP_SIZE > 1
    if(grouped)
#endif
        for(i = 0; i < ZFCP_GROUP_SIZE; i++)
            dev->group->memdev[i]->fla[0] = dev->group->memdev[0]->devnum;

    return 0;
} /* end function zfcp_init_handler */


/*-------------------------------------------------------------------*/
/* Query the device definition                                       */
/*-------------------------------------------------------------------*/
static void zfcp_query_device (DEVBLK *dev, char **devclass,
                int buflen, char *buffer)
{
    char filename[ PATH_MAX + 1 ];      /* full path or just name    */

    BEGIN_DEVICE_CLASS_QUERY( "FCP", dev, devclass, buflen, buffer );

    snprintf( buffer, buflen, "%s%s"
        , (dev->group->acount == ZFCP_GROUP_SIZE) ? "" : "*Incomplete "
        , (dev->scsw.flag2 & SCSW2_Q) ? "QDIO" : ""
        );

} /* end function zfcp_query_device */


/*-------------------------------------------------------------------*/
/* Close the device                                                  */
/*-------------------------------------------------------------------*/
static int zfcp_close_device ( DEVBLK *dev )
{
ZFCP_GRP *grp = (ZFCP_GRP*)dev->group->grp_data;

    if(!dev->member && dev->group->grp_data)
    {
        if(grp->ppfd[0])
            close_pipe(grp->ppfd[0]);
        if(grp->ppfd[1])
            close_pipe(grp->ppfd[1]);

        if(grp->wwpn)
            free(grp->wwpn);
        if(grp->lun)
            free(grp->lun);
        if(grp->brlba)
            free(grp->brlba);

        if(grp->rspbf)
            free(grp->rspbf);

        destroy_condition(&grp->qcond);
        destroy_lock(&grp->qlock);

        free(dev->group->grp_data);
        dev->group->grp_data = NULL;
    }

    return 0;
} /* end function zfcp_close_device */


#if defined(_FEATURE_QDIO_THININT)
/*-------------------------------------------------------------------*/
/* QDIO Set Subchannel Indicator                                     */
/*-------------------------------------------------------------------*/
static int zfcp_set_sci ( DEVBLK *dev, void *desc )
{
CHSC_REQ21 *req21 = (void *)desc;
RADR alsi, dsci;
BYTE ks, kc;
U16 opc;

    FETCH_HW(opc,req21->opcode);

    if(opc)
        return 3; // Invalid operation code

    FETCH_DW(alsi, req21->alsi);
    ks = req21->sk & CHSC_REQ21_KS;

    FETCH_DW(dsci, req21->dsci);
    kc = (req21->sk & CHSC_REQ21_KC) << 4;

    if(alsi && dsci)
    {
        if(STORCHK(alsi,0,ks,STORKEY_CHANGE,dev)
          || STORCHK(dsci,0,kc,STORKEY_CHANGE,dev))
        {
            dev->qdio.thinint = 0;
            return 3;
        }
        else
            dev->qdio.thinint = 1;

    }
    else
        dev->qdio.thinint = 0;

    dev->qdio.alsi = alsi;
    dev->qdio.ks   = ks;

    dev->qdio.dsci = dsci;
    dev->qdio.kc   = kc;

    return 0;
}
#endif /*defined(_FEATURE_QDIO_THININT)*/


/*-------------------------------------------------------------------*/
/* QDIO subsys desc                                                  */
/*-------------------------------------------------------------------*/
static int zfcp_ssqd_desc ( DEVBLK *dev, void *desc )
{
    CHSC_RSP24 *rsp24 = (void *)desc;

    STORE_HW(rsp24->sch, dev->subchan);

    if(dev->pmcw.flag4 & PMCW4_Q)
    {
        rsp24->flags |= ( CHSC_FLAG_QDIO_CAPABILITY | CHSC_FLAG_VALIDITY );

        rsp24->qdioac1 |= ( AC1_SIGA_INPUT_NEEDED | AC1_SIGA_OUTPUT_NEEDED );
        rsp24->qdioac1 |= AC1_AUTOMATIC_SYNC_ON_OUT_PCI;

#if defined(_FEATURE_QEBSM)
        if (FACILITY_ENABLED_DEV( HERC_QEBSM ))
        {
            STORE_DW(rsp24->sch_token, IOID2TKN((dev->ssid << 16) | dev->subchan));
            rsp24->qdioac1 |= ( AC1_SC_QEBSM_AVAILABLE | AC1_SC_QEBSM_ENABLED );
        }
#endif /*defined(_FEATURE_QEBSM)*/

#if defined(_FEATURE_QDIO_THININT)
        if (FACILITY_ENABLED_DEV( HERC_QDIO_THININT ))
            rsp24->qdioac1 |= AC1_AUTOMATIC_SYNC_ON_THININT;
#endif /*defined(_FEATURE_QDIO_THININT)*/

#if 1 // ZZTEST
          rsp24->icnt = QDIO_MAXQ;
          rsp24->ocnt = QDIO_MAXQ;
          rsp24->mbccnt = 0x04;
#endif
    }

    return 0;
}


/*-------------------------------------------------------------------*/
/* Execute a Channel Command Word                                    */
/*-------------------------------------------------------------------*/
static void zfcp_execute_ccw ( DEVBLK *dev, BYTE code, BYTE flags,
        BYTE chained, U32 count, BYTE prevcode, int ccwseq,
        BYTE *iobuf, BYTE *more, BYTE *unitstat, U32 *residual )
{
ZFCP_GRP *grp = (ZFCP_GRP*)dev->group->grp_data;
U32 num;                                /* Number of bytes to move   */

    UNREFERENCED(flags);
    UNREFERENCED(prevcode);
    UNREFERENCED(ccwseq);
    UNREFERENCED(chained);

    /* Command reject if the device group has not been established */
    if((dev->group->acount != ZFCP_GROUP_SIZE)
      && !(IS_CCW_SENSE(code) || IS_CCW_NOP(code) || (code == ZFCP_RCD)))
    {
        /* Set Intervention required sense, and unit check status */
        dev->sense[0] = SENSE_IR;
        *unitstat = CSW_CE | CSW_DE | CSW_UC;
        return;
    }

    /* Process depending on CCW opcode */
    switch (code) {


    case 0x01:
    /*---------------------------------------------------------------*/
    /* WRITE                                                         */
    /*---------------------------------------------------------------*/
    {
        if(1)
        {

            /* Calculate number of bytes to write and set residual count */
            num = (count < RSP_BUFSZ) ? count : RSP_BUFSZ;
            *residual = count - num;
            if (count < RSP_BUFSZ) *more = 1;

            /* Return normal status */
            *unitstat = CSW_CE | CSW_DE;
        }
        else
        {
            /* Command reject if no response buffer available */
            dev->sense[0] = SENSE_CR;
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
        }
        break;
    }


    case 0x02:
    /*---------------------------------------------------------------*/
    /* READ                                                          */
    /*---------------------------------------------------------------*/
    {
        U32 rd_size = 0;

        obtain_lock(&grp->qlock);
        if(grp->rspsz)
        {
            rd_size = grp->rspsz;
            memcpy(iobuf,grp->rspbf,rd_size);
            grp->rspsz = 0;
        }
        else
        {
            if(0)
            {
                wait_condition(&grp->qcond, &grp->qlock);
                if(grp->rspsz)
                {
                    rd_size = grp->rspsz;
                    memcpy(iobuf,grp->rspbf,rd_size);
                    grp->rspsz = 0;
                }
            }
        }
        release_lock(&grp->qlock);

        if(rd_size)
        {
            /* Calculate number of bytes to read and set residual count */
            num = (count < rd_size) ? count : rd_size;
            *residual = count - num;
            if (count < rd_size) *more = 1;

            /* Return normal status */
            *unitstat = CSW_CE | CSW_DE;
        }
        else
        {
            /* Return unit check with status modifier */
            dev->sense[0] = 0;
            *unitstat = CSW_CE | CSW_DE | CSW_UC | CSW_SM;
        }
        break;
    }


    case 0x03:
    /*---------------------------------------------------------------*/
    /* CONTROL NO-OPERATION                                          */
    /*---------------------------------------------------------------*/

        *residual = 0;
        *unitstat = CSW_CE | CSW_DE;
        break;


    case 0x14:
    /*---------------------------------------------------------------*/
    /* SENSE COMMAND BYTE                                            */
    /*---------------------------------------------------------------*/
    {
        // PROGRAMMING NOTE: I'm still not sure about this. The
        // Sense Command Byte command is known to be a 3088 CTCA
        // command, so I suspect we should never be seeing this
        // command because we don't support CTCA emulation mode.

        // I suspect the reason we're currently seeing it MAY be
        // because we still don't have something right and z/OS
        // is thus getting confused into thinking the OSA device
        // is currently configured to emulate a 3088 CTCA device.

        // However, since rejecting it causes z/OS to go into a
        // disabled wait, we are going to temporarily treat it
        // as a valid command until we can positively determine
        // whether or not it is a bonafide valid OSA command.

        /* The Sense Command Byte command returns a single byte
           being the CCW opcode from the other end of the CTCA */

        static const U32 len = 1;               /* cmd length */
        static const BYTE opcode = 0x03;        /* CCW opcode */

        /* Calculate residual byte count */
        num = (count < len) ? count : len;
        *residual = count - num;
        if (count < len) *more = 1;

        /* Copy the CTCA command byte to channel I/O buffer */
        *iobuf = opcode;

        /* Return normal i/o completion status */
        *unitstat = CSW_CE | CSW_DE;
        break;
    }

    case 0x04:
    /*---------------------------------------------------------------*/
    /* SENSE                                                         */
    /*---------------------------------------------------------------*/

        /* Calculate residual byte count */
        num = (count < dev->numsense) ? count : dev->numsense;
        *residual = count - num;
        if (count < dev->numsense) *more = 1;

        /* Copy device sense bytes to channel I/O buffer */
        memcpy (iobuf, dev->sense, num);

        /* Clear the device sense bytes */
        memset (dev->sense, 0, sizeof(dev->sense));

        /* Return unit status */
        *unitstat = CSW_CE | CSW_DE;
        break;


    case 0xE4:
    /*---------------------------------------------------------------*/
    /* SENSE ID                                                      */
    /*---------------------------------------------------------------*/

        /* Calculate residual byte count */
        num = (count < dev->numdevid) ? count : dev->numdevid;
        *residual = count - num;
        if (count < dev->numdevid) *more = 1;

        /* Copy device identifier bytes to channel I/O buffer */
        memcpy (iobuf, dev->devid, num);

        /* Return unit status */
        *unitstat = CSW_CE | CSW_DE;

        /* Display formatted Sense Id information, maybe */
        if( grp->debug )
        {
            char buf[1024];
            // HHC03995 "%1d:%04X %s: %s:\n%s"
            WRMSG(HHC03995, "I", LCSS_DEVNUM,
                dev->typname, "SID", FormatSID( iobuf, num, buf, sizeof( buf )));
        }
        break;


    case ZFCP_RCD:
    /*---------------------------------------------------------------*/
    /* READ CONFIGURATION DATA                                       */
    /*---------------------------------------------------------------*/
    {
        /* Build the configuration data area */
        U32 len = dev->rcd (dev, iobuf, count);

        /* Calculate residual byte count */
        num = (count < len ? count : len);
        *residual = count - num;
        if (count < len) *more = 1;

        /* Return unit status */
        *unitstat = CSW_CE | CSW_DE;

        /* Display formatted Read Configuration Data records, maybe */
        if( grp->debug )
        {
            char buf[1024];
            // HHC03995 "%1d:%04X %s: %s:\n%s"
            WRMSG(HHC03995, "I", LCSS_DEVNUM,
                dev->typname, "RCD", FormatRCD( iobuf, num, buf, sizeof( buf )));
        }
        break;
    }


    case ZFCP_SII:
    /*---------------------------------------------------------------*/
    /* SET INTERFACE IDENTIFIER                                      */
    /*---------------------------------------------------------------*/
    {
        U32 iir;                    /* Work area to validate IIR     */
        FETCH_FW(iir,iobuf);        /* Fetch IIR into work area      */

        /* Command Reject if the Interface ID Record is invalid.
           Note: we only support one interface with an ID of 0. */
        if ((iir & 0xFFFCFFFF) != 0xB0000000 ||
            (iir & 0x00030000) == 0x00030000)
        {
            dev->sense[0] = SENSE_CR;
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* Save the requested Interface ID for later unless it's
           not chained (to a presumably following RNI command) */
        if (chained)
            grp->iir = iir;

        /* Calculate residual byte count */
        num = (count < SII_SIZE) ? count : SII_SIZE;
        *residual = count - num;
        if (count < SII_SIZE) *more = 1;

        /* Return unit status */
        *unitstat = CSW_CE | CSW_DE;

//!     /* Display various information, maybe */
//!     if( grp->debug )
//!     {
//!         mpc_display_stuff( dev, "SII", iobuf, num, ' ' );
//!     }

        break;
    }


    case ZFCP_RNI:
    /*---------------------------------------------------------------*/
    /* READ NODE IDENTIFIER                                          */
    /*---------------------------------------------------------------*/
    {
        U32 len = sizeof(node_data);
        ND *nd = (ND*)iobuf;            /* Node Descriptor pointer   */
        DEVBLK *cua;                    /* Our Control Unit device   */

        /* Command Reject if not chained from Set Interface ID */
        if (!chained || prevcode != OSA_SII)
        {
            dev->sense[0] = SENSE_CR;
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            break;
        }

        /* The first device in the group is the control unit */
        cua = dev->group->memdev[0];

        /* If the Node Selector was zero an ND and one or more
           NQs are returned. Otherwise just the ND is returned. */
        if ((grp->iir & 0x00030000) != 0)
            len = sizeof(ND);

        /* Copy configuration data from tempate */
        memcpy (iobuf, node_data, len);

        /* Insert the CHPID of the node into the Node Descriptor ND */
        nd->tag[0] = dev->pmcw.chpid[0];

        /* Update the Node Qualifier information if they want it */
        if (len > sizeof(ND))
        {
            NQ *nq = (NQ*)nd + 1;       /* Point to Node Qualifier */

            /* Insert the CULA CHPID and device number into the NQ */
            nq->rsrvd[1] = cua->pmcw.chpid[0];
            nq->rsrvd[2] = cua->devnum & 0xFF;
        }

        /* Calculate residual byte count */
        num = (count < len ? count : len);
        *residual = count - num;
        if (count < len) *more = 1;

        /* Return unit status */
        *unitstat = CSW_CE | CSW_DE;

        /* Display formatted Read Node Information, maybe */
        if( grp->debug )
        {
            char buf[1024];
            // HHC03995 "%1d:%04X %s: %s:\n%s"
            WRMSG(HHC03995, "I", LCSS_DEVNUM,
                dev->typname, "RNI", FormatRNI( iobuf, num, buf, sizeof( buf )));
        }
        break;
    }


    case ZFCP_EQ:
    /*---------------------------------------------------------------*/
    /* ESTABLISH QUEUES                                              */
    /*---------------------------------------------------------------*/
    {
        QDIO_QDR *qdr = (QDIO_QDR*)iobuf;
        QDIO_QDES0 *qdes;
        int accerr;
        int i;

        dev->qdio.i_qcnt = qdr->iqdcnt < QDIO_MAXQ ? qdr->iqdcnt : QDIO_MAXQ;
        dev->qdio.o_qcnt = qdr->oqdcnt < QDIO_MAXQ ? qdr->oqdcnt : QDIO_MAXQ;

        FETCH_DW(dev->qdio.qiba,qdr->qiba);
        dev->qdio.qibk = qdr->qkey & 0xF0;

        if(!(accerr = STORCHK(dev->qdio.qiba,sizeof(QDIO_QIB)-1,dev->qdio.qibk,STORKEY_CHANGE,dev)))
        {
        QDIO_QIB *qib = (QDIO_QIB*)(dev->mainstor + dev->qdio.qiba);
            qib->ac |= QIB_AC_PCI; // Incidate PCI on output is supported
#if defined(_FEATURE_QEBSM)
            if (FACILITY_ENABLED_DEV( HERC_QEBSM ))
                qib->rflags |= QIB_RFLAGS_QEBSM;
#endif /*defined(_FEATURE_QEBSM)*/
        }

        qdes = qdr->qdf0;

        for(i = 0; i < dev->qdio.i_qcnt; i++)
        {
            FETCH_DW( dev->qdio.i_sliba[i],  qdes->sliba );
            FETCH_DW( dev->qdio.i_sla[i],    qdes->sla   );
            FETCH_DW( dev->qdio.i_slsbla[i], qdes->slsba );

            dev->qdio.i_slibk[i]  = (qdes->keyp1)      & 0xF0;
            dev->qdio.i_slk[i]    = (qdes->keyp1 << 4) & 0xF0;
            dev->qdio.i_sbalk[i]  = (qdes->keyp2)      & 0xF0;
            dev->qdio.i_slsblk[i] = (qdes->keyp2 << 4) & 0xF0;

            accerr |= STORCHK( dev->qdio.i_slsbla[i], sizeof(QDIO_SLSB) - 1, dev->qdio.i_slsblk[i], STORKEY_CHANGE, dev );
            accerr |= STORCHK( dev->qdio.i_sla[i],    sizeof(QDIO_SL)   - 1, dev->qdio.i_slk[i],    STORKEY_REF,    dev );

            qdes = (QDIO_QDES0*) ((BYTE*)qdes+(qdr->iqdsz << 2));
        }

        for(i = 0; i < dev->qdio.o_qcnt; i++)
        {
            FETCH_DW( dev->qdio.o_sliba[i],  qdes->sliba );
            FETCH_DW( dev->qdio.o_sla[i],    qdes->sla   );
            FETCH_DW( dev->qdio.o_slsbla[i], qdes->slsba );

            dev->qdio.o_slibk[i]  = (qdes->keyp1)      & 0xF0;
            dev->qdio.o_slk[i]    = (qdes->keyp1 << 4) & 0xF0;
            dev->qdio.o_sbalk[i]  = (qdes->keyp2)      & 0xF0;
            dev->qdio.o_slsblk[i] = (qdes->keyp2 << 4) & 0xF0;

            accerr |= STORCHK( dev->qdio.o_slsbla[i], sizeof(QDIO_SLSB) - 1, dev->qdio.o_slsblk[i], STORKEY_CHANGE, dev );
            accerr |= STORCHK( dev->qdio.o_sla[i],    sizeof(QDIO_SL)   - 1, dev->qdio.o_slk[i],    STORKEY_REF,    dev );

            qdes = (QDIO_QDES0*) ((BYTE*)qdes+(qdr->oqdsz << 2));
        }

        /* Calculate residual byte count */
        num = (count < sizeof(QDIO_QDR)) ? count : sizeof(QDIO_QDR);
        *residual = count - num;
        if (count < sizeof(QDIO_QDR)) *more = 1;

        if(!accerr)
        {
            /* Return unit status */
            *unitstat = CSW_CE | CSW_DE;
        }
        else
        {
            /* Command reject on invalid or inaccessible storage addresses */
            dev->sense[0] = SENSE_CR;
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
        }

        break;
    }


    case ZFCP_AQ:
    /*---------------------------------------------------------------*/
    /* ACTIVATE QUEUES                                               */
    /*---------------------------------------------------------------*/
    {
    fd_set readset;
    int rc;

        dev->qdio.i_qmask = dev->qdio.o_qmask = 0;

        FD_ZERO( &readset );

        dev->scsw.flag2 |= SCSW2_Q;


        do {
            /* Process the Input Queue if data has been received */
            if(dev->qdio.i_qmask && FD_ISSET(grp->ttfd,&readset))
            {
                process_input_queue(dev);
            }

            /* Process Output Queue if data needs to be sent */
            if(FD_ISSET(grp->ppfd[0],&readset))
            {
            char c;
                VERIFY(1 == read_pipe(grp->ppfd[0],&c,1));

                if(dev->qdio.o_qmask)
                {
                    process_output_queue(dev);
                }
            }

            if(dev->qdio.i_qmask)
                FD_SET(grp->ttfd, &readset);
            FD_SET(grp->ppfd[0], &readset);

            if(grp->reqpci)
            {
                grp->reqpci = FALSE;
                raise_adapter_interrupt(dev);
            }

#if defined( OPTION_W32_CTCI )
            do {
#endif

                rc = select (((grp->ttfd > grp->ppfd[0]) ? grp->ttfd : grp->ppfd[0]) + 1,
                    &readset, NULL, NULL, NULL);

#if defined( OPTION_W32_CTCI )
            } while (0 == rc || (rc < 0 && HSO_errno == HSO_EINTR));
        } while (rc > 0 && dev->scsw.flag2 & SCSW2_Q);
#else
        } while (dev->scsw.flag2 & SCSW2_Q);
#endif


        /* Return unit status */
        *unitstat = CSW_CE | CSW_DE;
    }
        break;


    default:
    /*---------------------------------------------------------------*/
    /* INVALID OPERATION                                             */
    /*---------------------------------------------------------------*/
        DBGTRC( dev, "Unkown CCW dev(%4.4x) code(%2.2x)\n", dev->devnum, code );
        /* Set command reject sense byte, and unit check status */
        dev->sense[0] = SENSE_CR;
        *unitstat = CSW_CE | CSW_DE | CSW_UC;

    } /* end switch(code) */

} /* end function zfcp_execute_ccw */


/*-------------------------------------------------------------------*/
/* Signal Adapter Initiate Input                                     */
/*-------------------------------------------------------------------*/
static int zfcp_initiate_input(DEVBLK *dev, U32 qmask)
{
ZFCP_GRP *grp = (ZFCP_GRP*)dev->group->grp_data;
int noselrd;

    DBGTRC( dev, "SIGA-r dev(%4.4x) qmask(%8.8x)\n", dev->devnum, qmask );

    /* Return CC1 if the device is not QDIO active */
    if(!(dev->scsw.flag2 & SCSW2_Q))
        return 1;

    /* Is there a read select */
    noselrd = !dev->qdio.i_qmask;

    /* Validate Mask */
    qmask &= ~(0xffffffff >> dev->qdio.i_qcnt);

    /* Reset Queue Positions */
    if(qmask != dev->qdio.i_qmask)
    {
    int n;
        for(n = 0; n < dev->qdio.i_qcnt; n++)
            if(!(dev->qdio.i_qmask & (0x80000000 >> n)))
                dev->qdio.i_bpos[n] = 0;
        if(!dev->qdio.i_qmask)
            dev->qdio.i_qpos = 0;

        /* Update Read Queue Mask */
        dev->qdio.i_qmask = qmask;
    }

    /* Send signal to QDIO thread */
    if(noselrd && dev->qdio.i_qmask)
        VERIFY(1 == write_pipe(grp->ppfd[1],"*",1));

    return 0;
}


/*-------------------------------------------------------------------*/
/* Signal Adapter Initiate Output                                    */
/*-------------------------------------------------------------------*/
static int zfcp_initiate_output(DEVBLK *dev, U32 qmask)
{
ZFCP_GRP *grp = (ZFCP_GRP*)dev->group->grp_data;

    DBGTRC( dev, "SIGA-w dev(%4.4x) qmask(%8.8x)\n", dev->devnum, qmask );

    /* Return CC1 if the device is not QDIO active */
    if(!(dev->scsw.flag2 & SCSW2_Q))
        return 1;

    /* Validate Mask */
    qmask &= ~(0xffffffff >> dev->qdio.o_qcnt);

    /* Reset Queue Positions */
    if(qmask != dev->qdio.o_qmask)
    {
    int n;
        for(n = 0; n < dev->qdio.o_qcnt; n++)
            if(!(dev->qdio.o_qmask & (0x80000000 >> n)))
                dev->qdio.o_bpos[n] = 0;
        if(!dev->qdio.o_qmask)
            dev->qdio.o_qpos = 0;

        /* Update Write Queue Mask */
        dev->qdio.o_qmask = qmask;
    }

    /* Send signal to QDIO thread */
    if(dev->qdio.o_qmask)
        VERIFY(1 == write_pipe(grp->ppfd[1],"*",1));

    return 0;
}


/*-------------------------------------------------------------------*/
/* Signal Adapter Sync                                               */
/*-------------------------------------------------------------------*/
static int zfcp_do_sync(DEVBLK *dev, U32 oqmask, U32 iqmask)
{
    UNREFERENCED(dev);          /* unreferenced for non-DEBUG builds */
    UNREFERENCED(oqmask);       /* unreferenced for non-DEBUG builds */
    UNREFERENCED(iqmask);       /* unreferenced for non-DEBUG builds */

    DBGTRC( dev, "SIGA-s dev(%4.4x) oqmask(%8.8x) iqmask(%8.8x)\n",
        dev->devnum, oqmask, iqmask );

    return 0;
}


/*-------------------------------------------------------------------*/
/* Signal Adapter Initiate Output Multiple                           */
/*-------------------------------------------------------------------*/
static int zfcp_initiate_output_mult(DEVBLK *dev, U32 qmask)
{
    DBGTRC( dev, "SIGA-m dev(%4.4x) qmask(%8.8x)\n", dev->devnum, qmask );

    return zfcp_initiate_output(dev, qmask);
}


static DEVHND zfcp_device_hndinfo =
{
        &zfcp_init_handler,            /* Device Initialization      */
        &zfcp_execute_ccw,             /* Device CCW execute         */
        &zfcp_close_device,            /* Device Close               */
        &zfcp_query_device,            /* Device Query               */
        NULL,                          /* Device Extended Query      */
        NULL,                          /* Device Start channel pgm   */
        NULL,                          /* Device End channel pgm     */
        NULL,                          /* Device Resume channel pgm  */
        NULL,                          /* Device Suspend channel pgm */
        &zfcp_halt_or_clear,           /* Device Halt channel pgm    */
        NULL,                          /* Device Read                */
        NULL,                          /* Device Write               */
        NULL,                          /* Device Query used          */
        NULL,                          /* Device Reserve             */
        NULL,                          /* Device Release             */
        NULL,                          /* Device Attention           */
        zfcp_immed_commands,           /* Immediate CCW Codes        */
        &zfcp_initiate_input,          /* Signal Adapter Input       */
        &zfcp_initiate_output,         /* Signal Adapter Output      */
        &zfcp_do_sync,                 /* Signal Adapter Sync        */
        &zfcp_initiate_output_mult,    /* Signal Adapter Output Mult */
        &zfcp_ssqd_desc,               /* QDIO subsys desc           */
#if defined(_FEATURE_QDIO_THININT)
        &zfcp_set_sci,                 /* QDIO set subchan ind       */
#else /*defined(_FEATURE_QDIO_THININT)*/
        NULL,                          /* QDIO set subchan ind       */
#endif /*defined(_FEATURE_QDIO_THININT)*/
        NULL,                          /* Hercules suspend           */
        NULL                           /* Hercules resume            */
};

/* Libtool static name colision resolution */
/* note : lt_dlopen will look for symbol & modulename_LTX_symbol */
#if defined( HDL_USE_LIBTOOL )
#define hdl_ddev hdtzfcp_LTX_hdl_ddev
#define hdl_depc hdtzfcp_LTX_hdl_depc
#define hdl_reso hdtzfcp_LTX_hdl_reso
#define hdl_init hdtzfcp_LTX_hdl_init
#define hdl_fini hdtzfcp_LTX_hdl_fini
#endif

HDL_DEPENDENCY_SECTION;
{
    HDL_DEPENDENCY( HERCULES );
    HDL_DEPENDENCY( DEVBLK );
    HDL_DEPENDENCY( SYSBLK );

    memcpy( (NED*)&configuration_data[0], &zfcp_device_ned [0], sizeof( NED ));
    memcpy( (NED*)&configuration_data[1], &zfcp_ctlunit_ned[0], sizeof( NED ));
    memcpy( (NED*)&configuration_data[2], &zfcp_token_ned  [0], sizeof( NED ));
    memcpy( (NED*)&configuration_data[3], &zfcp_general_neq[0], sizeof( NEQ ));

    memcpy( (ND*)&node_data[0], &zfcp_nd[0], sizeof( ND ));
    memcpy( (ND*)&node_data[1], &zfcp_nq[0], sizeof( NQ ));
}
END_DEPENDENCY_SECTION


HDL_RESOLVER_SECTION;
{
  #if defined( WIN32 ) && !defined( _MSVC_ ) && !defined( HDL_USE_LIBTOOL )
    #undef sysblk
    HDL_RESOLVE_SYMPTR( psysblk, sysblk );
  #else
    UNREFERENCED( getsym );     // (HDL_RESOLVER_SECTION parameter)
  #endif
}
END_RESOLVER_SECTION


HDL_DEVICE_SECTION;
{
    HDL_DEVICE ( ZFCP, zfcp_device_hndinfo );
}
END_DEVICE_SECTION

#if defined( _MSVC_ ) && defined( NO_ZFCP_OPTIMIZE )
  #pragma optimize( "", on )            // restore previous settings
#endif
