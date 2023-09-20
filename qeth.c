/* QETH.C       (C) Copyright Jan Jaeger,   1999-2012                */
/*              (C) and others 2013-2021                             */
/*              OSA Express                                          */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/* This module contains device handling functions for the            */
/* OSA Express emulated card                                         */
/*                                                                   */
/* This implementation is based on the S/390 Linux implementation    */
/*                                                                   */
/* Device module hdtqeth                                             */
/*                                                                   */
/* hercules.cnf:                                                     */
/* 0A00-0A02 QETH <optional parameters>                              */
/* Default parm:   iface /dev/net/tun                                */
/* Optional parms: ifname  <name of interface>                       */
/*                 hwaddr  <MAC address>                             */
/*                 ipaddr  <IPv4 address and prefix length>          */
/*                 netmask <IPv4 netmask>                            */
/*                 ipaddr6 <IPv6 address and prefix length>          */
/*                 mtu     <MTU>                                     */
/*                 chpid   <channel path id>                         */
/*                 debug                                             */
/*                                                                   */
/* When using a bridged configuration no parameters are required     */
/* on the QETH device statement.  The tap device will in that case   */
/* need to be  bridged to another virtual or real ethernet adapter.  */
/* e.g.                                                              */
/* 0A00.3 QETH                                                       */
/* The tap device will need to be bridged e.g.                       */
/* brctl addif <bridge> tap0                                         */
/*                                                                   */
/* When using a routed configuration the tap device needs to have    */
/* an IP address assigned in the same subnet as the guests virtual   */
/* eth adapter.                                                      */
/* e.g.                                                              */
/* 0A00.3 QETH ipaddr 192.168.10.1                                   */
/* where the guest can then use any other IP address in the          */
/* 192.168.10 range                                                  */
/*                                                                   */
/*                                                                   */
/* zLinux defines which three devices addresses are used for what    */
/* purpose depending on on the distribution.                         */
/* Debian, in a file named, for example, config-ccw-0.0.0800, in the */
/* /etc/sysconfig/hardware directory, has the CCWGROUP_CHANS         */
/* statement. An example CCWGROUP_CHANS statement is:-               */
/*   CCWGROUP_CHANS=(0.0.0800 0.0.0801 0.0.0802)                     */
/* Similarly, Fedora, in a file named, for example,                  */
/* ifcfg-enccw0.0.0800, in the /etc/sysconfig/network-scripts        */
/* directory, has the SUBCHANNELS statement. An example SUBCHANNELS  */
/* statement is:-                                                    */
/*   SUBCHANNELS="0.0.0800,0.0.0801,0.0.0802"                        */
/* With the example SUBCHANNELS statement the first device address,  */
/* i.e. 800, would be used for the read device, the second device    */
/* address, i.e. 801, would be used for the write device, and the    */
/* third device address, i.e. 802, would be used for the data        */
/* device. However, if the SUBCHANNELS statement were specified as:- */
/*   SUBCHANNELS="0.0.0802,0.0.0800,0.0.0801"                        */
/* then device address 802 would be used for the read device, device */
/* address 800 would be used for the write device, and device        */
/* address 801 would be used for the data device.                    */
/* (It's not clear whether the group of device addresses even needs  */
/* to be consecutive?)                                               */
/*                                                                   */
/* z/VM defines which devices addresses are used for what purpose in */
/* the 'DEVICE and LINK Statements for OSD Devices' (see 'z/VM:      */
/* TCP/IP Planning and Customization'). A single device address      */
/* specifies the first of three consecutive virtual device numbers   */
/* to be grouped for the OSA-Express Adapter. TCP/IP uses the first  */
/* device address for the data device, the second device address for */
/* the read device, and the third device address for the write       */
/* device.                                                           */
/*                                                                   */
/* z/OS defines which devices addresses are used for what purpose in */
/* a VTAM 'Transport resource list major node' (a TRLE, see 'z/OS    */
/* Communication Server: SNA Reource Definition Guide'). One read    */
/* device address, one write device address, and up to 238 data      */
/* device addresses must be specified. The read device address must  */
/* be an even number, and the write device address must be an odd    */
/* number that is one greater than the read device address. Each     */
/* TCP/IP in the same logical partition instance that starts an      */
/* OSA-Express in QDIO mode is assigned one of the data device       */
/* addresses by VTAM. Each TCP/IP in the same logical partition      */
/* instance that starts an OSA-Express network traffic analyzer      */
/* (OSAENTA) trace is also assigned one of the DATAPATH channels by  */
/* VTAM. A sufficient number of data device addresses must be coded  */
/* for the number of concurrent instances that will be using an      */
/* OSA-Express port. For example, if you wanted three instances of   */
/* TCP/IP in the same logical partition, you could code the DATAPATH */
/* operand in the TRLE as follows:                                   */
/*   DATAPATH=(402,403,404), or DATAPATH=(402-404).                  */
/*                                                                   */
/* The OSA-Express Implementation Guide says:-                       */
/* - Any OSD, OSX, or OSM CHPID (QDIO mode) requires at least three  */
/* devices for each TCP/IP stack: read, write, and data path. For    */
/* z/OS, only the first TCP/IP stack requires three devices. Any     */
/* additional TCP/IP stack requires only one device (data path). If  */
/* you define both IPv4 and IPv6 with INTERFACE statements, z/OS     */
/* will use two data devices, one for each protocol.                 */
/* - If you plan to use the OSA-Express Network Traffic Analyzer,    */
/* you need one more data device, which is used to transmit the      */
/* captured trace data to TCP/IP.                                    */
/* - Any OSE CHPID requires two devices (read and write) for each    */
/* TCP/IP. SNA requires one device.                                  */
/*                                                                   */

#include "hstdinc.h"

DISABLE_GCC_UNUSED_FUNCTION_WARNING;

#include "hercules.h"
#include "devtype.h"
#include "chsc.h"
#include "mpc.h"
#include "tuntap.h"
#include "resolve.h"
#include "ctcadpt.h"
#include "hercifc.h"
#include "qeth.h"
#include "opcode.h"
#include "inline.h"


/*-------------------------------------------------------------------*/
/* QETH Debugging                                                    */
/*-------------------------------------------------------------------*/

//#define ENABLE_QETH_DEBUG   1   // 1:enable, 0:disable, #undef:default
#define QETH_PTT_TRACING        // #define to enable PTT debug tracing
#define QETH_DUMP_DATA          // #undef to suppress i/o buffers dump


/*-------------------------------------------------------------------*/
/* QETH Debugging                                                    */
/*-------------------------------------------------------------------*/

/* (enable debugging if needed/requested) */
#if (!defined(ENABLE_QETH_DEBUG) && defined(DEBUG)) ||  \
    ( defined(ENABLE_QETH_DEBUG) && ENABLE_QETH_DEBUG)
  #define QETH_DEBUG
#endif

/* (activate debugging if debugging is enabled) */
#if defined(QETH_DEBUG)
  #define  ENABLE_TRACING_STMTS   1     // (Fish: DEBUGGING)
  #include "dbgtrace.h"                 // (Fish: DEBUGGING)
  #define  NO_QETH_OPTIMIZE             // (Fish: DEBUGGING) (MSVC only)
  #define  QETH_PTT_TRACING             // (Fish: DEBUG speed/debugging)
#endif

/* (disable optimizations if debugging) */
#if defined( _MSVC_ ) && defined( NO_QETH_OPTIMIZE )
  #pragma optimize( "", off )           // disable optimizations for reliable breakpoints
#endif

/* (QETH speed/performance/debugging) */
#if defined( QETH_PTT_TRACING )
  #define PTT_QETH_TRACE(   _string, _tr1, _tr2, _tr3 ) \
          PTT( PTT_CL_QETH, _string, _tr1, _tr2, _tr3 )
#else
  #define PTT_QETH_TRACE(...)   // (do nothing)
#endif

/* DBGTRC statements controlled by dev stmt "debug" option */
static void dbgtrc( const char* file, int line, const char* func,
                    DEVBLK* dev, char* fmt, ... )
{
  DEVGRP *devgrp = dev->group;
  if (devgrp)
  {
    OSA_GRP* grp = devgrp->grp_data;
    if (grp && grp->debugmask)
    {
      char buf[256];
      va_list   vargs;
      va_start( vargs, fmt );
      vsnprintf( buf, sizeof(buf), fmt, vargs );
      // HHC03991D "%1d:%04X %s: %s"
      fwritemsg( file, line, func, WRMSG_NORMAL, stdout,
        "HHC03991D " HHC03991 "\n", LCSS_DEVNUM, dev->typname, buf );
      va_end( vargs );
    }
  }
}
#define DBGTRC( ... ) dbgtrc( __FILE__, __LINE__, __FUNCTION__, ## __VA_ARGS__ )

/* (trace I/O data buffers if debugging) */
#if defined( QETH_DUMP_DATA )
  #define MPC_DUMP_DATA(_str,_adr,_len,_dir)  \
    net_data_trace( dev, _adr, _len, _dir, 'D', _str, 0 );
#else
  #define MPC_DUMP_DATA(...)    // (do nothing)
#endif

/* DBGUPD statements controlled by "debugupdown" option */
static void dbgupd( const char* file, int line, const char* func,
                    DEVBLK* dev, int what, void* adr, int len, BYTE dir, char* fmt, ... )
{
  DEVGRP *devgrp = dev->group;
  if (devgrp)
  {
    OSA_GRP* grp = devgrp->grp_data;
    if (grp && (grp->debugmask & DBGQETHUPDOWN))
    {
      char buf[256];
      va_list   vargs;
      va_start( vargs, fmt );
      vsnprintf( buf, sizeof(buf), fmt, vargs );
      // HHC03991D "%1d:%04X %s: %s"
      fwritemsg( file, line, func, WRMSG_NORMAL, stdout,
        "HHC03991D " HHC03991 "\n", LCSS_DEVNUM, dev->typname, buf );
      va_end( vargs );
      if (what == 3) {
        mpc_display_osa_iear( dev, adr, dir, len );
      } else if (what == 2) {
        mpc_display_osa_iea( dev, adr, dir, len );
      } else if (what == 1) {
        mpc_display_osa_th_etc( dev, adr, dir, 0 );
      } else {
        mpc_display_stuff( dev, "", adr, len, dir );
      }
    }
  }
}
#define DBGUPD( ... ) dbgupd( __FILE__, __LINE__, __FUNCTION__, ## __VA_ARGS__ )

/*-------------------------------------------------------------------*/
/* Hercules Dynamic Loader (HDL)                                     */
/*-------------------------------------------------------------------*/
#if defined( WIN32 ) && !defined( _MSVC_ ) && !defined( HDL_USE_LIBTOOL )
  SYSBLK *psysblk;
  #define sysblk (*psysblk)
#endif


/*-------------------------------------------------------------------*/
/* Helper functions, entirely internal to qeth.c                     */
/*-------------------------------------------------------------------*/
static OSA_BHR* process_cm_enable( DEVBLK*, MPC_TH*, MPC_RRH*, MPC_PUK* );
static OSA_BHR* process_cm_setup( DEVBLK*, MPC_TH*, MPC_RRH*, MPC_PUK* );
static OSA_BHR* process_cm_takedown( DEVBLK*, MPC_TH*, MPC_RRH*, MPC_PUK* );
static OSA_BHR* process_cm_disable( DEVBLK*, MPC_TH*, MPC_RRH*, MPC_PUK* );
static int      process_ulp_enable_extract( DEVBLK*, MPC_TH*, MPC_RRH*, MPC_PUK* );
static OSA_BHR* process_ulp_enable( DEVBLK*, MPC_TH*, MPC_RRH*, MPC_PUK* );
static OSA_BHR* process_ulp_setup( DEVBLK*, MPC_TH*, MPC_RRH*, MPC_PUK* );
static OSA_BHR* process_dm_act( DEVBLK*, MPC_TH*, MPC_RRH*, MPC_PUK* );
static OSA_BHR* process_ulp_takedown( DEVBLK*, MPC_TH*, MPC_RRH*, MPC_PUK* );
static OSA_BHR* process_ulp_disable( DEVBLK*, MPC_TH*, MPC_RRH*, MPC_PUK* );
static OSA_BHR* alloc_buffer( DEVBLK*, int );
static void     add_buffer_to_chain( OSA_BAN*, OSA_BHR* );
static OSA_BHR* remove_buffer_from_chain( OSA_BAN* );
static void     remove_and_free_any_buffers_on_chain( OSA_BAN* );
static void     signal_idx_event( OSA_GRP* );
static void     InitMACAddr( DEVBLK* dev, OSA_GRP* grp );
static void     InitMTU    ( DEVBLK* dev, OSA_GRP* grp );
static int      netmask2prefix( char* ttnetmask, char** ttpfxlen  );
static int      prefix2netmask( char* ttpfxlen,  char** ttnetmask );
static U32      makepfxmask4( char* ttpfxlen );
static void     makepfxmask6( char* ttpfxlen6, BYTE* pfxmask6 );
static void     qeth_init_queues(DEVBLK *dev);
static void     qeth_init_queue(DEVBLK *dev, int output);
#if defined( ENABLE_IPV6 )
static void     process_l3_icmpv6_packet(DEVBLK*, OSA_GRP*, IP6FRM*);
static void     calculate_icmpv6_checksum(IP6FRM*, BYTE*, int);
#endif

/*-------------------------------------------------------------------*/
/* Configuration Data Constants                                      */
/*-------------------------------------------------------------------*/
static const NED  osa_device_ned[]  = {OSA_DEVICE_NED};
static const NED  osa_ctlunit_ned[] = {OSA_CTLUNIT_NED};
static const NED  osa_token_ned[]   = {OSA_TOKEN_NED};
static const NEQ  osa_general_neq[] = {OSA_GENERAL_NEQ};

static NED configuration_data[4]; // (initialized by HDL_DEPENDENCY_SECTION)

static const ND  osa_nd[] = {OSA_ND};
static const NQ  osa_nq[] = {OSA_NQ};

static ND node_data[2]; // (initialized by HDL_DEPENDENCY_SECTION)

#define SII_SIZE    sizeof(U32)

static const BYTE sense_id_bytes[] =
{
    0xFF,                               /* Always 0xFF               */
    OSA_SNSID_1731_01,                  /* Control Unit type/model   */
    OSA_SNSID_1732_01,                  /* I/O Device   type/model   */
    0x00,                               /* Always 0x00               */
    OSA_RCD_CIW,                        /* Read Config. Data CIW     */
    OSA_SII_CIW,                        /* Set Interface Id. CIW     */
    OSA_RNI_CIW,                        /* Read Node Identifier CIW  */
    OSA_EQ_CIW,                         /* Establish Queues CIW      */
    OSA_AQ_CIW                          /* Activate Queues CIW       */
};


static BYTE qeth_immed_commands [256] =
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
/* Internal socket pipe signals used to request something            */
/*-------------------------------------------------------------------*/
#define QDSIG_RESET     0       /* Used to reset signal flag         */
#define QDSIG_HALT      1       /* Halt Device signalling            */
#define QDSIG_SYNC      2       /* SIGA Synchronize                  */
#define QDSIG_READ      3       /* SIGA Initiate Input               */
#define QDSIG_RDMULT    4       /* SIGA Initiate Input Multiple      */
#define QDSIG_WRIT      5       /* SIGA Initiate Output              */
#define QDSIG_WRMULT    6       /* SIGA Initiate Output Multiple     */
#define QDSIG_WAKEUP    7       /* Wakeup signalling                 */

static const char* qsig2str( BYTE sig ) {
    static const char* sigstr[] = {
    /*0*/ "QDSIG_RESET",
    /*1*/ "QDSIG_HALT",
    /*2*/ "QDSIG_SYNC",
    /*3*/ "QDSIG_READ",
    /*4*/ "QDSIG_RDMULT",
    /*5*/ "QDSIG_WRIT",
    /*6*/ "QDSIG_WRMULT",
    /*7*/ "QDSIG_WAKEUP",
    }; static char buf[16];
    if (sig < _countof( sigstr ))
        return sigstr[ sig ];
    MSGBUF(buf,"QDSIG_0x%02X",sig);
    return buf;
}

/*-------------------------------------------------------------------*/
/* The following is the original macro comment - kept for historical */
/* value                                                             */
/*-------------------------------------------------------------------*/
/* STORCHK macro: check storage access & update ref & change bits.   */
/* Returns 0 if successful or CSW_PROGC or CSW_PROTC if error.       */
/* Storage key ref & change bits are only updated if successful.     */
/*-------------------------------------------------------------------*/

/* Make the macro unto an inline function - same perf, less arcane.  */
/* NOTE : This should probably go into qdio.h */
static inline int qeth_storage_access_check(U64 addr, size_t len,int key,int acc, DEVBLK *dev)
{
  if(addr+len>dev->mainlim)
  {
    DBGTRC(dev,"Address %llx above main storage",addr);
    return CSW_PROGC;  /* Outside storage */
  }
  if(dev->orb.flag5 & ORB5_A)     /* Address limit checking enabled ?*/
  {
    if(dev->pmcw.flag5 & PMCW5_LM_LOW)  /* Low address defined ? */
    {
      if(addr<sysblk.addrlimval)
      {
        DBGTRC(dev,"Address %llx below limit of %llx",addr,sysblk.addrlimval);
        return CSW_PROGC;
      }
    }
    if(dev->pmcw.flag5 & PMCW5_LM_HIGH) /* High address defined ? */
    {
      if((addr+len)>sysblk.addrlimval)
      {
        DBGTRC(dev,"Address %llx above limit of %llx",addr+len,sysblk.addrlimval);
        return CSW_PROGC;
      }
    }
  }
  /* key 0 always right */
  if(key==0) return 0;

  /* This may not be described anywhere - and could be wrong  */
  /* But apparently z/VM TC/IP expects Key 14 to allow access to all */
  /* including storage frames with key 0.... */
  /* and apparently z/OS TCP/IP expects Key 6 to allow access to all */
  /* including storage frames with key 0.... */
  if((key & 0Xf0)==0xe0) return 0; /* Special case for key 14 ? */
  if((key & 0Xf0)==0x60) return 0; /* Special case for key 6 ? */

  /* Key match check if keys match we're good */
  if((ARCH_DEP( get_dev_4K_storage_key )( dev, addr ) & STORKEY_KEY) == key) return 0;

  /* Ok for fetch (i.e. ref) when key mismatches */
  /* (as long as not fetch-protected) */
  if(!((ARCH_DEP( get_dev_4K_storage_key )( dev, addr )) & STORKEY_FETCH) && acc==STORKEY_REF) return 0;

  /* Ok for write (i.e. chg) or fetch (i.e. ref) when write allowed even on key mismatch */
  /* (i.e. when storage already changed we presume writes are allowed?) */
  if(((ARCH_DEP( get_dev_4K_storage_key )( dev, addr )) & STORKEY_CHANGE)) return 0;
  DBGTRC(dev,"Key mismatch protection exception : requested key : %x, storage key : %x access type %x",key,ARCH_DEP( get_dev_4K_storage_key )( dev, addr ),acc);

  return CSW_PROTC;
}

/* The following function updates the storage key on access */
static inline int qeth_storage_access_check_and_update(U64 addr, size_t len,int key,int acc, DEVBLK *dev)
{
  int rc;
  rc=qeth_storage_access_check(addr,len,key,acc,dev);
  if(rc==0)
  {
    /* Update the REF/CHANGE flag in the storage key */
    ARCH_DEP( or_dev_4K_storage_key )( dev, addr, (acc & (STORKEY_REF | STORKEY_CHANGE)) );
  }
  return rc;
}

/* Macro wrapper */
#define STORCHK(_addr,_len,_key,_acc,_dev) qeth_storage_access_check_and_update(_addr,_len,_key,_acc,_dev)

#if defined( _FEATURE_QDIO_THININT )
/*-------------------------------------------------------------------*/
/* Set Adapter Local Summary Indicator bits                          */
/*-------------------------------------------------------------------*/
static inline void set_alsi( DEVBLK* dev, BYTE bits )
{
    if (dev->qdio.alsi)
    {
        BYTE* alsi = dev->mainstor + dev->qdio.alsi;

        obtain_lock( &sysblk.mainlock );
        {
            *alsi |= bits;
            ARCH_DEP( or_dev_4K_storage_key )( dev, dev->qdio.alsi, (STORKEY_REF | STORKEY_CHANGE) );
        }
        release_lock( &sysblk.mainlock );
    }
}

#define SET_ALSI(_dev,_bits)    set_alsi((_dev),(_bits))


/*-------------------------------------------------------------------*/
/* Set Device State Change Indicator bits                            */
/*-------------------------------------------------------------------*/
static inline void set_dsci( DEVBLK* dev, BYTE bits )
{
    if (dev->qdio.dsci)
    {
        BYTE* dsci = dev->mainstor + dev->qdio.dsci;
        BYTE* alsi = dev->mainstor + dev->qdio.alsi;

        obtain_lock( &sysblk.mainlock );
        {
            *dsci |= bits;
            ARCH_DEP( or_dev_4K_storage_key )( dev, dev->qdio.dsci, (STORKEY_REF | STORKEY_CHANGE) );

            *alsi |= bits;
            ARCH_DEP( or_dev_4K_storage_key )( dev, dev->qdio.alsi, (STORKEY_REF | STORKEY_CHANGE) );
        }
        release_lock( &sysblk.mainlock );
    }
}
#define SET_DSCI(_dev,_bits)    set_dsci((_dev),(_bits))

#else /* !defined( _FEATURE_QDIO_THININT ) */

#define SET_ALSI(_dev,_bits)    /* (do nothing) */
#define SET_DSCI(_dev,_bits)    /* (do nothing) */

#endif /* defined( _FEATURE_QDIO_THININT ) */


/*-------------------------------------------------------------------*/
/* Register local MAC address                                        */
/*-------------------------------------------------------------------*/
/* The returned values are:-                                         */
/*   -1  The table is full                                           */
/*    0  The MAC address was added to the table                      */
/*    1  The MAC address was already in the table                    */
static int register_mac(OSA_GRP *grp, DEVBLK* dev, BYTE *mac, int type)
{
int i;
char charmac[24];

    /* Check whether the MAC address is already registered. */
    for (i = 0; i < OSA_MAXMAC; i++)
    {
        if (grp->mac[i].type &&
            memcmp(grp->mac[i].addr, mac, IFHWADDRLEN) == 0)
        {
            return 1;
        }
    }
    /* Register the previously unknown MAC address. */
    for (i = 0; i < OSA_MAXMAC; i++)
    {
        if (!grp->mac[i].type)
        {
            memcpy(grp->mac[i].addr, mac, IFHWADDRLEN);
            grp->mac[i].type = type;
            MSGBUF( charmac,
                      "%02x:%02x:%02x:%02x:%02x:%02x",
                      mac[0], mac[1], mac[2], mac[3], mac[4], mac[5] );
            // HHC03801 "%1d:%04X %s: %s: Register guest MAC address %s"
            WRMSG(HHC03801, "I", LCSS_DEVNUM, dev->typname, grp->ttifname,
                      charmac );
            return 0;
        }
    }
    /* Oh dear, the MAC address table is full. */
    MSGBUF( charmac,
              "%02x:%02x:%02x:%02x:%02x:%02x",
              mac[0], mac[1], mac[2], mac[3], mac[4], mac[5] );
    // HHC03802 "%1d:%04X %s: %s: Cannot register guest MAC address %s"
    WRMSG(HHC03802, "E", LCSS_DEVNUM, dev->typname, grp->ttifname,
              charmac );
    return -1;
}

/*-------------------------------------------------------------------*/
/* Unregister local MAC address                                      */
/*-------------------------------------------------------------------*/
/* The returned values are:-                                         */
/*    0  The MAC address was removed from the table                  */
/*    1  The MAC address was not in the table                        */
static int unregister_mac(OSA_GRP *grp, DEVBLK* dev, BYTE *mac, int type, BYTE warnmsg )
{
int i;
char charmac[24];
UNREFERENCED(type);
    /* Check whether the MAC address is registered. */
    for (i = 0; i < OSA_MAXMAC; i++)
    {
        if (grp->mac[i].type &&
            memcmp(grp->mac[i].addr, mac, IFHWADDRLEN) == 0)
        {
            grp->mac[i].type = MAC_TYPE_NONE;
            memset(grp->mac[i].addr, 0, IFHWADDRLEN);
            MSGBUF( charmac,
                      "%02x:%02x:%02x:%02x:%02x:%02x",
                      mac[0], mac[1], mac[2], mac[3], mac[4], mac[5] );
            // HHC03803 "%1d:%04X %s: %s: Unregistered guest MAC address %s"
            WRMSG(HHC03803, "I", LCSS_DEVNUM, dev->typname, grp->ttifname,
                      charmac );
            return 0;
        }
    }
    /* Oh dear, the MAC address wasn't registered. */
    if (warnmsg)
    {
        MSGBUF( charmac,
                  "%02x:%02x:%02x:%02x:%02x:%02x",
                  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5] );
        // HHC03804 "%1d:%04X %s: %s: Cannot unregister guest MAC address %s"
        WRMSG(HHC03804, "W", LCSS_DEVNUM, dev->typname, grp->ttifname,
                  charmac );
    }
    return 1;
}

/*-------------------------------------------------------------------*/
/* Unregister all local MAC address                                  */
/*-------------------------------------------------------------------*/
/* The returned value is:-                                           */
/*    0  All MAC addresses were removed from the table               */
static int unregister_all_mac(OSA_GRP *grp)
{
int i;
    for(i = 0; i < OSA_MAXMAC; i++)
    {
        grp->mac[i].type = MAC_TYPE_NONE;
        memset(grp->mac[i].addr, 0, IFHWADDRLEN);
    }
    return MAC_TYPE_NONE;
}

/*-------------------------------------------------------------------*/
/* Validate MAC address and return MAC type                          */
/* In promiscuous mode, all frames are accepted                      */
/* a Broadcast frame is always valid (unless we do not seek broadcast*/
/*  frames                                                           */
/*-------------------------------------------------------------------*/
static int validate_mac(BYTE *mac, int type, OSA_GRP *grp)
{
int i;  /* Utility variable */

    /* Always accept broadcast frames */
    /* Search for all FFs             */

    for(i=0;i<IFHWADDRLEN;i++)
    {
       if(mac[i]!=0xff) break;
    }
    /* If we reached end of MAC address, all FF then it is a broadcast */
    /* Return it is a broadcast unless broadcast isn't sought for      */
    /* unless the interface is in promiscuous mode                     */

    if(i==(IFHWADDRLEN)) return (MAC_TYPE_BRDCST & type) | grp->promisc;

    /* Find a matching MAC (Unicast or Multicast) */
    for(i = 0; i < OSA_MAXMAC; i++)
    {
        /* Find a type matching the search mask with a matching MAC addr */
        if((grp->mac[i].type & type) && !memcmp(grp->mac[i].addr,mac,IFHWADDRLEN))
            return grp->mac[i].type | grp->promisc;
    }

    /* Not match found - but accept it if the interface is in promiscuous mode */
    /* Otherwise, ignore the frame                                             */

    return grp->promisc;
}


#if defined( OPTION_W32_CTCI )
/*-------------------------------------------------------------------*/
/*         Check if CTCI-WIN supports multiple IP addresses          */
/*-------------------------------------------------------------------*/
static bool tt32_multiple_ip_support()
{
    static bool didthis = false;       // (only need to do this once)
    static bool multiple_ip_support;   // (boolean we're determining)

    if (!didthis)
    {
        int major, inter, minor, build;
        tt32_version_numbers( &major, &inter, &minor, &build );
        // CTCI-WIN v3.7 or greater?
        multiple_ip_support = (((major * 100) + inter) >= 307);
        didthis = true;
    }
    return multiple_ip_support;
}
#endif


/*-------------------------------------------------------------------*/
/* Register local IPv4 address                                       */
/*-------------------------------------------------------------------*/
/* The returned values are:-                                         */
/*   -1  The table is full                                           */
/*    0  The IPv4 address was added to the table                     */
/*    1  The IPv4 address was already in the table                   */
static int register_ipv4(OSA_GRP* grp, DEVBLK* dev, BYTE *ipaddr4)
{
int i;
char charip4[48];
    /* Check whether the IPv4 address is already registered. */
    for (i = 0; i < OSA_MAXIPV4; i++)
    {
        if (grp->ipaddr4[i].type == IPV4_TYPE_INUSE &&
            memcmp(grp->ipaddr4[i].addr, ipaddr4, 4) == 0)
        {
            return 1;
        }
    }
    /* Register the previously unknown IPv4 address. */
    for (i = 0; i < OSA_MAXIPV4; i++)
    {
        if (grp->ipaddr4[i].type == IPV4_TYPE_NONE)
        {
#if defined( OPTION_W32_CTCI )
            if (tt32_multiple_ip_support())
            {
                if (TT32_AddIPAddr( grp->ttfd, ipaddr4 ) < 0)
                    break; // (fail)
            }
#endif
            memcpy(grp->ipaddr4[i].addr, ipaddr4, 4);
            grp->ipaddr4[i].type = IPV4_TYPE_INUSE;
            hinet_ntop( AF_INET, ipaddr4, charip4, sizeof(charip4) );
            // HHC03805 "%1d:%04X %s: %s: Registered guest IP address %s"
            WRMSG(HHC03805, "I", LCSS_DEVNUM, dev->typname, grp->ttifname,
                    charip4 );
            return 0;
        }
    }
    /* Oh dear, the table of IPv4 addresses is full. */
    hinet_ntop( AF_INET, ipaddr4, charip4, sizeof(charip4) );
    // HHC03806 "%1d:%04X %s: %s: Cannot register guest IP address %s"
    WRMSG(HHC03806, "E", LCSS_DEVNUM, dev->typname, grp->ttifname,
            charip4 );
    return -1;
}

/*-------------------------------------------------------------------*/
/* Unregister local IPv4 address                                     */
/*-------------------------------------------------------------------*/
/* The returned values are:-                                         */
/*    0  The IPv4 address was removed from the table                 */
/*    1  The IPv4 address was not in the table                       */
static int unregister_ipv4(OSA_GRP* grp, DEVBLK* dev, BYTE *ipaddr4)
{
int i;
char charip4[48];
    /* Check whether the IPv4 address is registered. */
    for (i = 0; i < OSA_MAXIPV4; i++)
    {
        if (grp->ipaddr4[i].type == IPV4_TYPE_INUSE &&
            memcmp(grp->ipaddr4[i].addr, ipaddr4, 4) == 0)
        {
#if defined( OPTION_W32_CTCI )
            if (tt32_multiple_ip_support())
            {
                if (TT32_DelIPAddr( grp->ttfd, ipaddr4 ) < 0)
                    break; // (fail)
            }
#endif
            grp->ipaddr4[i].type = IPV4_TYPE_NONE;
            memset(grp->ipaddr4[i].addr, 0, 4);
            hinet_ntop( AF_INET, ipaddr4, charip4, sizeof(charip4) );
            // HHC03807 "%1d:%04X %s: s: Unregister guest IP address %s"
            WRMSG(HHC03807, "I", LCSS_DEVNUM, dev->typname, grp->ttifname,
                    charip4 );
            return 0;
        }
    }
    /* Oh dear, the IPv4 address wasn't registered. */
    hinet_ntop( AF_INET, ipaddr4, charip4, sizeof(charip4) );
    // HHC03808 "%1d:%04X %s: s: Cannot unregister guest IP address %s"
    WRMSG(HHC03808, "W", LCSS_DEVNUM, dev->typname, grp->ttifname,
          charip4 );
    return 1;
}

/*-------------------------------------------------------------------*/
/* Unregister all local IPv4 addresses                               */
/*-------------------------------------------------------------------*/
/* The returned value is:-                                           */
/*    0  All IPv4 addresses were removed from the table              */
static int unregister_all_ipv4(OSA_GRP* grp)
{
int i;
    for (i = 0; i < OSA_MAXIPV4; i++)
    {
        grp->ipaddr4[i].type = IPV4_TYPE_NONE;
        memset(grp->ipaddr4[i].addr, 0, 4);
    }
    return 0;
}


/*-------------------------------------------------------------------*/
/* Register local IPv6 address                                       */
/*-------------------------------------------------------------------*/
/* The returned values are:-                                         */
/*   -1  The table is full                                           */
/*    0  The IPv6 address was added to the table                     */
/*    1  The IPv6 address was already in the table                   */
static int register_ipv6(OSA_GRP* grp, DEVBLK* dev, BYTE *ipaddr6)
{
int i;
char charip6[48];
    /* Check whether the IPv6 address is already registered. */
    for (i = 0; i < OSA_MAXIPV6; i++)
    {
        if (grp->ipaddr6[i].type == IPV6_TYPE_INUSE &&
            memcmp(grp->ipaddr6[i].addr, ipaddr6, 16) == 0)
        {
            return 1;
        }
    }
    /* Register the previously unknown IPv6 address. */
    for (i = 0; i < OSA_MAXIPV6; i++)
    {
        if (grp->ipaddr6[i].type == IPV6_TYPE_NONE)
        {
            memcpy(grp->ipaddr6[i].addr, ipaddr6, 16);
            grp->ipaddr6[i].type = IPV6_TYPE_INUSE;
            hinet_ntop( AF_INET6, ipaddr6, charip6, sizeof(charip6) );
            // HHC03805 "%1d:%04X %s: s: Registered guest IP address %s"
            WRMSG(HHC03805, "I", LCSS_DEVNUM, dev->typname, grp->ttifname,
                    charip6 );
            return 0;
        }
    }
    /* Oh dear, the IPv6 address table is full. */
    hinet_ntop( AF_INET6, ipaddr6, charip6, sizeof(charip6) );
    // HHC03806 "%1d:%04X %s: s: Cannot register guest IP address %s"
    WRMSG(HHC03806, "E", LCSS_DEVNUM, dev->typname, grp->ttifname,
          charip6 );
    return -1;
}

/*-------------------------------------------------------------------*/
/* Unregister local IPv6 address                                     */
/*-------------------------------------------------------------------*/
/* The returned values are:-                                         */
/*    0  The IPv6 address was removed from the table                 */
/*    1  The IPv6 address was not in the table                       */
static int unregister_ipv6(OSA_GRP* grp, DEVBLK* dev, BYTE *ipaddr6)
{
int i;
char charip6[48];
    /* Check whether the IPv6 address is registered. */
    for (i = 0; i < OSA_MAXIPV6; i++)
    {
        if (grp->ipaddr6[i].type == IPV6_TYPE_INUSE &&
            memcmp(grp->ipaddr6[i].addr, ipaddr6, 16) == 0)
        {
            grp->ipaddr6[i].type = IPV6_TYPE_NONE;
            memset(grp->ipaddr6[i].addr, 0, 16);
            hinet_ntop( AF_INET6, ipaddr6, charip6, sizeof(charip6) );
            // HHC03807 "%1d:%04X %s: %s: Unregistered guest IP address %s"
            WRMSG(HHC03807, "I", LCSS_DEVNUM, dev->typname, grp->ttifname,
                    charip6 );
            return 0;
        }
    }
    /* Oh dear, the IPv6 address wasn't registered. */
    hinet_ntop( AF_INET6, ipaddr6, charip6, sizeof(charip6) );
    // HHC03808 "%1d:%04X %s: %s: Cannot unregister guest IP address %s"
    WRMSG(HHC03808, "W", LCSS_DEVNUM, dev->typname, grp->ttifname,
            charip6 );
    return 1;
}

/*-------------------------------------------------------------------*/
/* Unregister all local IPv6 addresses                               */
/*-------------------------------------------------------------------*/
/* The returned value is:-                                           */
/*    0  All IPv6 addresses were removed from the table              */
static int unregister_all_ipv6(OSA_GRP* grp)
{
int i;
    for (i = 0; i < OSA_MAXIPV6; i++)
    {
        grp->ipaddr6[i].type = IPV6_TYPE_NONE;
        memset(grp->ipaddr6[i].addr, 0, 16);
    }
    return 0;
}


/*-------------------------------------------------------------------*/
/* QETH pipe read/write/select...                                    */
/*-------------------------------------------------------------------*/
#define  QETH_TEMP_PIPE_ERROR( errnum )  ( errnum == HSO_EINTR    ||  \
                                           errnum == HSO_EAGAIN   ||  \
                                           errnum == HSO_EALREADY ||  \
                                           errnum == HSO_EWOULDBLOCK )

static int qeth_select (int nfds, fd_set* rdset, struct timeval* tv)
{
    int rc, errnum;
    PTT_QETH_TRACE( "b4 select", 0,0,0 );
    for (;;)
    {
        /* Do the select */
        rc = select( nfds, rdset, NULL, NULL, tv );
        /* Get error code */
        errnum = HSO_errno;
        /* Return if successful */
        if (rc >= 0)
            break;
        /* Return if non-temporary error */
        if (!QETH_TEMP_PIPE_ERROR( errnum ))
            break;
        /* Otherwise pause before retrying */
        sched_yield();
    }
    if (rc <= 0)
        errno = errnum;
    PTT_QETH_TRACE( "af select", 0,0,0 );
    return rc;
}
static int qeth_read_pipe (int fd, BYTE *sig)
{
    int rc, errnum;
    PTT_QETH_TRACE( "b4 rdpipe", 0,0,*sig );
    for (;;) {
        rc = read_pipe( fd, sig, 1 );
        if (rc > 0)
            break;
        errnum = HSO_errno;
        if (!QETH_TEMP_PIPE_ERROR( errnum ))
            break;
        sched_yield();
    }
    if (rc <= 0)
        errno = errnum;
    PTT_QETH_TRACE( "af rdpipe", 0,0,*sig );
    return rc;
}
static int qeth_write_pipe (int fd, BYTE *sig)
{
    int rc, errnum;
    PTT_QETH_TRACE( "b4 wrpipe", 0,0,*sig );
    for (;;) {
        rc = write_pipe( fd, sig, 1 );
        if (rc > 0)
            break;
        errnum = HSO_errno;
        if (!QETH_TEMP_PIPE_ERROR( errnum ))
            break;
        sched_yield();
    }
    if (rc <= 0)
        errno = errnum;
    PTT_QETH_TRACE( "af wrpipe", 0,0,*sig );
    return rc;
}


/*-------------------------------------------------------------------*/
/*  Helper macro to call "qeth_errnum_msg()" function                */
/*-------------------------------------------------------------------*/
#define QERRMSG( dev, grp, errnum, msgcode, errmsg )  \
    qeth_errnum_msg( (dev), (grp), (errnum), (msgcode), (errmsg), \
        __FUNCTION__, __FILE__, __LINE__ )

/*-------------------------------------------------------------------*/
/* Issue generic error message with return code and strerror msg.    */
/* Returns the same errnum value that was passed.                    */
/*-------------------------------------------------------------------*/
static int qeth_errnum_msg( DEVBLK* dev, OSA_GRP* grp,
                            int errnum, char* msgcode, char* errmsg,
                            const char* func, const char* filename, int line )
{
    char strerr[256] = {0};
    char msgbuf[256] = {0};
    char msgloc[256] = {0};

    if (errnum >= 0)
        STRLCPY( strerr, strerror( errnum ));
    else
        STRLCPY( strerr, "An unidentified error has occurred" );

    /* Manually identify ACTUAL source of error if emsgloc is enabled,
       since normal emsgloc identifies US as source which doesn't help.
    */
    if (MLVL( EMSGLOC ) && (0
        || 'S' == *msgcode
        || 'E' == *msgcode
        || 'W' == *msgcode
    ))
        MSGBUF( msgloc, "%s() at %s(%d): ",
            func, TRIMLOC( filename ), line );

    /* "function() failed, rc=99 (0x00000063): an error occurred" */
    MSGBUF( msgbuf, "%s%s, rc=%d (0x%08X): %s",
        msgloc, errmsg, errnum, errnum, strerr );

    // HHC03996 "%1d:%04X %s: %s: %s"
    if (str_caseless_eq( "E", msgcode ))
        WRMSG( HHC03996, "E", LCSS_DEVNUM,
            dev->typname, grp->ttifname, msgbuf );
    else if (str_caseless_eq( "W", msgcode ))
        WRMSG( HHC03996,      "W", LCSS_DEVNUM,
            dev->typname, grp->ttifname, msgbuf );
    else
        WRMSG( HHC03996, "I", LCSS_DEVNUM,
            dev->typname, grp->ttifname, msgbuf );

    return errnum;
}


/*-------------------------------------------------------------------*/
/* Report what values we are using                                   */
/*-------------------------------------------------------------------*/
static void qeth_report_using( DEVBLK *dev, OSA_GRP *grp )
{
    char not[8];
    STRLCPY( not, grp->enabled ? "" : "not " );

    // HHC03997 "%1d:%04X %s: Interface %s %susing %s %s"
    WRMSG( HHC03997, "I", LCSS_DEVNUM, dev->typname,
        grp->ttifname, not, "MAC address", grp->tthwaddr );

    if (grp->l3 && grp->ttipaddr)
    {
        WRMSG( HHC03997, "I", LCSS_DEVNUM, dev->typname,
             grp->ttifname, not, "IP address", grp->ttipaddr );

        if(grp->ttnetmask)
        {
            WRMSG( HHC03997, "I", LCSS_DEVNUM, dev->typname,
                grp->ttifname, not, "subnet mask", grp->ttnetmask );
        }
    }

    if (grp->l3 && grp->ttipaddr6)
    {
        WRMSG( HHC03997, "I", LCSS_DEVNUM, dev->typname,
            grp->ttifname, not, "IP address", grp->ttipaddr6 );

        if(grp->ttpfxlen6)
        {
            WRMSG( HHC03997, "I", LCSS_DEVNUM, dev->typname,
                grp->ttifname, not, "prefix length", grp->ttpfxlen6 );
        }
    }

    if (grp->ttmtu)
    {
        WRMSG( HHC03997, "I", LCSS_DEVNUM, dev->typname,
            grp->ttifname, not, "MTU", grp->ttmtu );
    }

#if defined( ENABLE_IPV6 )
    if (grp->l3 && grp->enabled)
    {
        // HHC03997 "%1d:%04X %s: Interface %s %susing %s %s"
        WRMSG( HHC03997, "I", LCSS_DEVNUM, dev->typname,
            grp->ttifname, not, "drive MAC address", grp->szDriveMACAddr );
        // HHC03997 "%1d:%04X %s: Interface %s %susing %s %s"
        WRMSG( HHC03997, "I", LCSS_DEVNUM, dev->typname,
            grp->ttifname, not, "drive IP address", grp->szDriveLLAddr6 );
    }
#endif
}


/*-------------------------------------------------------------------*/
/* Enable the TUNTAP interface  (set IFF_UP flag)                    */
/*-------------------------------------------------------------------*/
static int qeth_enable_interface (DEVBLK *dev, OSA_GRP *grp)
{
    int rc;
    int flags;

    if (grp->enabled)
        return 0;

    flags = ( 0
            | IFF_UP
            | IFF_ALLMULTI
            | IFF_BROADCAST
#if defined( TUNTAP_IFF_RUNNING_NEEDED )
            | IFF_RUNNING
#endif
#if defined( OPTION_W32_CTCI )
            | (grp->debugmask ? IFF_DEBUG : 0)
#endif
            | (grp->promisc ? IFF_PROMISC : 0)
            );

    rc = TUNTAP_SetFlags( grp->ttifname, flags );
    if (rc != 0)
    {
        QERRMSG( dev, grp, errno,
            "E", "qeth_enable_interface() failed" );
        return rc;
    }

    grp->enabled = 1;
    qeth_report_using( dev, grp );

    return 0;
}


/*-------------------------------------------------------------------*/
/* Disable the TUNTAP interface  (clear IFF_UP flag)                 */
/*-------------------------------------------------------------------*/
/* Clearing the IFF_UP flag is only done on Windows, as, on *nix,    */
/* clearing and then setting the IFF_UP flag causes interface        */
/* IP addresses to be lost.                                          */
/*-------------------------------------------------------------------*/
static int qeth_disable_interface (DEVBLK *dev, OSA_GRP *grp)
{
    if (!grp->enabled)
        return 0;

#if defined( OPTION_W32_CTCI )
    {
        int rc;
        int flags;

        flags = ( 0
                | IFF_ALLMULTI
                | IFF_BROADCAST
#if defined( TUNTAP_IFF_RUNNING_NEEDED )
                | IFF_RUNNING
#endif
                | (grp->debugmask ? IFF_DEBUG : 0)
                | (grp->promisc ? IFF_PROMISC : 0)
                );

        rc = TUNTAP_SetFlags( grp->ttifname, flags );
        if (rc != 0)
        {
            QERRMSG( dev, grp, errno,
                "E", "qeth_disable_interface() failed" );
            return rc;
        }
    }
#endif /* defined( OPTION_W32_CTCI ) */

    grp->enabled = 0;
    qeth_report_using( dev, grp );

    return 0;
}


/*-------------------------------------------------------------------*/
/* Create the TUNTAP interface                                       */
/*-------------------------------------------------------------------*/
static int qeth_create_interface (DEVBLK *dev, OSA_GRP *grp)
{
    int i, rc;
    char buf[64];

    /* We should only ever be called ONCE */
    if (grp->ttfd >= 0)
    {
        DBGTRC( dev, "ERROR: TUNTAP Interface already exists!" );
        ASSERT(0);              /* (Oops!) */
        return -1;              /* Return failure */
    }

    /* Create the new interface by opening the TUNTAP device */
    if ((rc = TUNTAP_CreateInterface
    (
        grp->ttdev,
        0
            | IFF_NO_PI
            | IFF_OSOCK
            | (grp->l3 ? IFF_TUN : IFF_TAP)
        ,
        &grp->ttfd,
        grp->ttifname

    )) != 0)
        return QERRMSG( dev, grp, errno,
            "E", "TUNTAP_CreateInterface() failed" );

    /* Update DEVBLK file descriptors */
    for (i=0; i < dev->group->acount; i++)
        dev->group->memdev[i]->fd = grp->ttfd;

    // HHC00901 "%1d:%04X %s: interface %s, type %s opened"
    WRMSG( HHC00901, "I", SSID_TO_LCSS(dev->ssid),
                         dev->devnum,
                         dev->typname,
                         grp->ttifname,
                         (grp->l3 ? "TUN" : "TAP"));

    /* Set NON-Blocking mode by disabling Blocking mode */
    if ((rc = socket_set_blocking_mode(grp->ttfd,0)) != 0)
        QERRMSG( dev, grp, rc,
            "W", "socket_set_blocking_mode() failed" );

    /* Set the interface's MTU size, if possible */
    {
        /* Save original requested value, if any */
        char*  ttmtu  = grp->ttmtu ? strdup( grp->ttmtu ) : NULL;
        U16    uMTU   = ttmtu ? (U16) atoi( ttmtu ) : 0;

        /* Retrieve the interface's actual MTU */
        free( grp->ttmtu );
        grp->ttmtu = NULL;
        grp->uMTU  = 0;
        InitMTU( dev, grp );  /* get interface's mtu value */
        ASSERT( grp->ttmtu );
        ASSERT( grp->uMTU  );

        /* Decrease their requested MTU if neccessary */
        if (ttmtu)
        {
            if (uMTU > grp->uMTU)
            {
                // "%1d:%04X %s: %s: Requested MTU %s too large; decreasing to %s bytes"
                WRMSG( HHC03809, "W", LCSS_DEVNUM,
                    dev->typname, grp->ttifname, ttmtu, grp->ttmtu );
                free( ttmtu );
                ttmtu = NULL;
            }
            else /* Use their requested value */
            {
                free( grp->ttmtu );
                grp->ttmtu = ttmtu;
                grp->uMTU  = uMTU;
            }

            MSGBUF( buf, "TUNTAP_SetMTU(%s) failed", grp->ttmtu );
            if ((rc = TUNTAP_SetMTU( grp->ttifname, grp->ttmtu )) != 0)
                return QERRMSG( dev, grp, errno, "E", buf );
        }
    }

#if defined( OPTION_TUNTAP_SETMACADDR )
    /* Make sure the interface has a valid MAC address.      */
    /* TUN's of course don't have MAC addresses, only TAP's. */
    if (grp->tthwaddr) {
        if (!grp->l3)
        {
            if ((rc = TUNTAP_SetMACAddr( grp->ttifname, grp->tthwaddr )) != 0)
            {
                MSGBUF( buf, "TUNTAP_SetMACAddr(%s) failed", grp->tthwaddr );
                return QERRMSG( dev, grp, errno, "E", buf );
            }
        }
    }
#endif

    /* Make sure the interface has a valid MAC address.      */
    /* TUN's of course don't have MAC addresses, only TAP's, */
    /* but we need to keep up a pretence.                    */
    InitMACAddr( dev, grp );

    /* If possible, assign an IPv4 address to the guest interface */
    if (1
#if defined( OPTION_W32_CTCI )
        && grp->l3
#endif
        && grp->ttipaddr
    )
    {
#if defined( OPTION_W32_CTCI )
        if ((rc = TUNTAP_SetDestAddr( grp->ttifname,grp->ttipaddr )) != 0)
        {
            char buf[64];
            MSGBUF( buf, "TUNTAP_SetDestAddr(\"%s\") failed", grp->ttipaddr );
            return QERRMSG( dev, grp, errno, "E", buf );
        }
#else /* Linux */
        if ((rc = TUNTAP_SetIPAddr( grp->ttifname, grp->ttipaddr )) != 0)
        {
            char buf[64];
            MSGBUF( buf, "TUNTAP_SetIPAddr(\"%s\") failed", grp->ttipaddr );
            return QERRMSG( dev, grp, errno, "E", buf );
        }
#endif /* CTCI-WIN or Linux */
    }

    /* Same thing with the IPv4 subnet mask */
#if defined( OPTION_TUNTAP_SETNETMASK )
    if(
#if defined( OPTION_W32_CTCI )
       grp->l3 &&
#endif
                  grp->ttnetmask)
    {
        if ((rc = TUNTAP_SetNetMask( grp->ttifname, grp->ttnetmask )) != 0)
        {
            MSGBUF( buf, "TUNTAP_SetNetMask(%s) failed", grp->ttnetmask );
            return QERRMSG( dev, grp, errno, "E", buf );
        }
    }
#endif /* defined( OPTION_TUNTAP_SETNETMASK ) */

    /* Assign it an IPv6 address too, if possible */
#if defined( ENABLE_IPV6 )
    if(
#if defined( OPTION_W32_CTCI )
       grp->l3 &&
#endif
                  grp->ttipaddr6)
    {
        if((rc = TUNTAP_SetIPAddr6(grp->ttifname, grp->ttipaddr6, grp->ttpfxlen6)) != 0)
        {
            MSGBUF( buf, "TUNTAP_SetIPAddr6(%s) failed", grp->ttipaddr6 );
            return QERRMSG( dev, grp, errno, "E", buf );
        }
    }
#endif /* defined( ENABLE_IPV6 ) */

    return 0;
}


/*-------------------------------------------------------------------*/
/* Adapter Command Routine                                           */
/*-------------------------------------------------------------------*/
static void osa_adapter_cmd(DEVBLK *dev, MPC_TH *req_th, int datalen)
{
OSA_GRP *grp = (OSA_GRP*)dev->group->grp_data;

OSA_BHR *rsp_bhr;

MPC_RRH *req_rrh;
/* MPC_PH  *req_ph; */

U32 offrrh;
U16 offph;

    /* Point to request MPC_RRH and MPC_PH. */
    FETCH_FW(offrrh,req_th->offrrh);
    req_rrh = (MPC_RRH*)((BYTE*)req_th+offrrh);
    FETCH_HW(offph,req_rrh->offph);
    /* req_ph = (MPC_PH*)((BYTE*)req_rrh+offph); */

    switch(req_rrh->type) {

    case RRH_TYPE_CM:
        strcat( dev->dev_data, "RRH_TYPE_CM" );  /* Prepare the contentstring */
        {
            MPC_PUK *req_puk;

            req_puk = mpc_point_puk( dev, req_th, req_rrh );

            switch(req_puk->type) {

            case PUK_TYPE_ENABLE:
                strcat( dev->dev_data, ": PUK_TYPE_ENABLE (CM_ENABLE)" );  /* Prepare the contentstring */
                rsp_bhr = process_cm_enable( dev, req_th, req_rrh, req_puk );
                break;

            case PUK_TYPE_SETUP:
                strcat( dev->dev_data, ": PUK_TYPE_SETUP (CM_SETUP)" );  /* Prepare the contentstring */
                rsp_bhr = process_cm_setup( dev, req_th, req_rrh, req_puk );
                break;

            case PUK_TYPE_TAKEDOWN:
                strcat( dev->dev_data, ": PUK_TYPE_TAKEDOWN (CM_TAKEDOWN)" );  /* Prepare the contentstring */
                rsp_bhr = process_cm_takedown( dev, req_th, req_rrh, req_puk );
                break;

            case PUK_TYPE_DISABLE:
                strcat( dev->dev_data, ": PUK_TYPE_DISABLE (CM_DISABLE)" );  /* Prepare the contentstring */
                rsp_bhr = process_cm_disable( dev, req_th, req_rrh, req_puk );
                break;

            default:
                // HHC03991 "%1d:%04X %s: %s"
                WRMSG( HHC03991, "W", LCSS_DEVNUM, dev->typname,
                    "Unknown PUK_TYPE_xxx (CM Unknown)" );
                net_data_trace( dev, (BYTE*)req_th, datalen, FROM_GUEST, 'I', "???", 0 );
                rsp_bhr = NULL;

            }

            // Add response buffer to chain.
            add_buffer_to_chain( &grp->idx, rsp_bhr );
            signal_idx_event( grp );

        }
        break;

    /*  The RRH_TYPE_ULP requests enable/disable a specific data device. The  */
    /*  data device being enabled is identified by the ULP_SETUP PUS_0B       */
    /*  contents. The data device being disabled is identified by the token.  */
    /*  Note: The ULP_ENABLE, ULP_SETUP & ULP_ACTIVE exchanges also prepares  */
    /*  the token that identifies the data device in subsequent RRH_TYPE_IPA  */
    /*  requests & responses. QETH currently uses the constant 'QET5' for     */
    /*  the token, I wonder if it should be unique?                           */
    case RRH_TYPE_ULP:
        strcat( dev->dev_data, "RRH_TYPE_ULP" );  /* Prepare the contentstring */
        {
            MPC_PUK *req_puk;

            req_puk = mpc_point_puk(dev,req_th,req_rrh);

            switch(req_puk->type) {

            case PUK_TYPE_ENABLE:
                strcat( dev->dev_data, ": PUK_TYPE_ENABLE (ULP_ENABLE)" );  /* Prepare the contentstring */
                if (process_ulp_enable_extract( dev, req_th, req_rrh, req_puk ) != 0)
                {
                    rsp_bhr = NULL;
                    break;
                }
                if (grp->ttfd < 0)
                    if (qeth_create_interface( dev, grp ) != 0)
                        QERRMSG( dev, grp, -1,
                            "E", "qeth_create_interface() failed" );
                rsp_bhr = process_ulp_enable( dev, req_th, req_rrh, req_puk );
                break;

            case PUK_TYPE_SETUP:
                strcat( dev->dev_data, ": PUK_TYPE_SETUP (ULP_SETUP)" );  /* Prepare the contentstring */
                rsp_bhr = process_ulp_setup( dev, req_th, req_rrh, req_puk );
                break;

            case PUK_TYPE_ACTIVE:
                strcat( dev->dev_data, ": PUK_TYPE_ACTIVE (ULP_ACTIVE)" );  /* Prepare the contentstring */
                rsp_bhr = process_dm_act( dev, req_th, req_rrh, req_puk );
                break;

            case PUK_TYPE_TAKEDOWN:
                strcat( dev->dev_data, ": PUK_TYPE_TAKEDOWN (ULP_TAKEDOWN)" );  /* Prepare the contentstring */
                rsp_bhr = process_ulp_takedown( dev, req_th, req_rrh, req_puk );
                break;

            case PUK_TYPE_DISABLE:
                strcat( dev->dev_data, ": PUK_TYPE_DISABLE (ULP_DISABLE)" );  /* Prepare the contentstring */
                rsp_bhr = process_ulp_disable( dev, req_th, req_rrh, req_puk );
                break;

            default:
                // HHC03991 "%1d:%04X %s: %s"
                WRMSG( HHC03991, "W", LCSS_DEVNUM, dev->typname,
                    "Unknown PUK_TYPE_xxx (ULP Unknown)" );
                net_data_trace( dev, (BYTE*)req_th, datalen, FROM_GUEST, 'I', "???", 0 );
                rsp_bhr = NULL;
            }

            // Add response buffer to chain.
            add_buffer_to_chain( &grp->idx, rsp_bhr );
            signal_idx_event( grp );
        }
        break;

    /*  The RRH_TYPE_IPA requests configure a specific data device            */
    /*  identified by the MPC_RRH->token variable. As QETH only supports a    */
    /*  group with a single data device, this isn't a problem (yet!).         */
    case RRH_TYPE_IPA:
        strcat( dev->dev_data, "RRH_TYPE_IPA" );  /* Prepare the contentstring */
        {
            MPC_TH  *rsp_th;
            MPC_RRH *rsp_rrh;
            MPC_PH  *rsp_ph;
            MPC_IPA *ipa;

            U32      rqsize;
            U32      offdata;
            U32      lendata;
//          U32      ackseq;

            U16      proto;
            char     protoc[10];

            int      ipasize = SIZE_IPA;
            int      ipadatasize = -1;

            /* Allocate a buffer to which the request will be copied */
            /* and then modified, to become the response.            */
            FETCH_FW(rqsize,req_th->length);
            rsp_bhr = alloc_buffer( dev, rqsize+100 );
            if (!rsp_bhr)
                break;
            rsp_bhr->datalen = rqsize;

            /* Point to response MPC_TH. */
            rsp_th = (MPC_TH*)((BYTE*)rsp_bhr + SizeBHR);

            /* Copy request to response buffer. */
            memcpy(rsp_th,req_th,rqsize);

            /* Point to response MPC_RRH and MPC_PH. */
            rsp_rrh = (MPC_RRH*)((BYTE*)rsp_th+offrrh);
            rsp_ph = (MPC_PH*)((BYTE*)rsp_rrh+offph);

            /* Get the length of and point to response MPC_IPA and associated command. */
            FETCH_F3( lendata, rsp_ph->lendata );
            FETCH_FW( offdata, rsp_ph->offdata );
            ipa = (MPC_IPA*)((BYTE*)rsp_th + offdata);

            /* Modify the response MPC_TH and MPC_RRH. */
            STORE_FW( rsp_th->seqnum, 0 );
            STORE_HW( rsp_th->unknown10, 0x0FFC );        /* !!! */
            rsp_rrh->proto = PROTOCOL_UNKNOWN;
            memcpy( rsp_rrh->token, grp->gtulpconn, MPC_TOKEN_LENGTH );

            /* Make a note of the protocol the request relates to. */
            FETCH_HW(proto,ipa->proto);
            if (proto == IPA_PROTO_IPV4) {
                strcpy( protoc, " (IPv4)" );
            } else if (proto == IPA_PROTO_IPV6) {
                strcpy( protoc, " (IPv6)" );
            } else {
                strcpy( protoc, "" );
            }

            switch(ipa->cmd) {

            case IPA_CMD_STARTLAN:  /* 0x01 : Start LAN operations */
                /* Note: the request MPC_IPA may be 16 or 20-bytes in length  */
                /* with nothing following, or may be 20-byte in length with   */
                /* an unpredictable number of bytes following. The response   */
                /* MPC_IPA we return is always 16-bytes in length with        */
                /* nothing following. There was a reason for this, I just     */
                /* wish, I could remember what it was! Something to do with   */
                /* z/VM TCPIP I think, but...                                 */
                {

                    strcat( dev->dev_data, ": IPA_CMD_STARTLAN" );  /* Prepare the contentstring */
                    rsp_bhr->content = strdup( dev->dev_data );

                    /* Display the request MPC_TH etc., maybe. */
                    DBGUPD( dev, 1, req_th, 0, FROM_GUEST, "%s: Request", dev->dev_data );

                    STORE_HW(ipa->rc,IPA_RC_OK);
                    grp->ipae0 |= IPA_SETADAPTERPARMS;
                    grp->ipae4 |= IPA_SETADAPTERPARMS;
                    grp->ipae6 |= IPA_SETADAPTERPARMS;

                    ipasize = SIZE_IPA_SHORT;
                    ipadatasize = 0;

                    /* Enable the TUN or TAP interface */
                    VERIFY( qeth_enable_interface( dev, grp ) == 0);
                }
                /* end case IPA_CMD_STARTLAN:  0x01 */
                break;

            case IPA_CMD_STOPLAN:  /* 0x02 : Stop LAN operations */
                {
                    strcat( dev->dev_data, ": IPA_CMD_STOPLAN" );  /* Prepare the contentstring */
                    rsp_bhr->content = strdup( dev->dev_data );

                    /* Display the request MPC_TH etc., maybe. */
                    DBGUPD( dev, 1, req_th, 0, FROM_GUEST, "%s: Request", dev->dev_data );

                    STORE_HW(ipa->rc,IPA_RC_OK);
                    grp->ipae0 &= ~IPA_SETADAPTERPARMS;
                    grp->ipae4 &= ~IPA_SETADAPTERPARMS;
                    grp->ipae6 &= ~IPA_SETADAPTERPARMS;

                    /* Disable the TUN or TAP interface */
                    VERIFY( qeth_disable_interface( dev, grp ) == 0);
                }
                /* end case IPA_CMD_STOPLAN:  0x02 */
                break;

            case IPA_CMD_SETVMAC:  /* 0x21 : Set Layer-2 MAC address */
                {
                MPC_IPA_MAC *ipa_mac = (MPC_IPA_MAC*)(ipa+1);
                int rc = 0;
#if defined( OPTION_W32_CTCI )
                char tthwaddr[32] = {0}; // 11:22:33:44:55:66
                BYTE was_enabled;
#endif

                    strcat( dev->dev_data, ": IPA_CMD_SETVMAC" );  /* Prepare the contentstring */
                    rsp_bhr->content = strdup( dev->dev_data );

                    /* Display the request MPC_TH etc., maybe. */
                    DBGUPD( dev, 1, req_th, 0, FROM_GUEST, "%s: Request", dev->dev_data );

                    /* Register the MAC address */
                    rc = register_mac(grp, dev, ipa_mac->macaddr, MAC_TYPE_UNICST);
                    if (rc == -1) {          /* MAC table full */
                        STORE_HW(ipa->rc, IPA_RC_L2_ADDR_TABLE_FULL);
                    } else if (rc == 1) {    /* MAC address in table */
                        STORE_HW(ipa->rc, IPA_RC_L2_DUP_MAC);
                    } else {                 /* MAC address added to table */
                        STORE_HW(ipa->rc, IPA_RC_SUCCESS);
                    }

#if defined( OPTION_W32_CTCI )
#if defined( OPTION_TUNTAP_SETMACADDR )
                    // PROGRAMMING NOTE: normally one should not change
                    // tuntap's MAC but due to the way Windows CTCI-WIN
                    // is implemented (designed) we MUST change the MAC
                    // for Windows. This is ONLY for Windows. For Linux
                    // we should leave the tuntap MAC alone (as-is).

                    /* We only set the interface MAC address if the MAC    */
                    /* address supplied by the guest is different to the   */
                    /* interface MAC address that is currently being used. */
                    if (memcmp( grp->iMAC, ipa_mac->macaddr, IFHWADDRLEN ) != 0)
                    {

                        /* Save guest MAC address */
                        MSGBUF( tthwaddr, "%02x:%02x:%02x:%02x:%02x:%02x"
                            ,ipa_mac->macaddr[0]
                            ,ipa_mac->macaddr[1]
                            ,ipa_mac->macaddr[2]
                            ,ipa_mac->macaddr[3]
                            ,ipa_mac->macaddr[4]
                            ,ipa_mac->macaddr[5]
                        );

                        was_enabled = grp->enabled ? TRUE : FALSE;

                        if (was_enabled)
                            VERIFY( qeth_disable_interface( dev, grp ) == 0);

                        rc = TUNTAP_SetMACAddr( grp->ttifname, tthwaddr );

                        if (rc == 0)
                        {
                            free( grp->tthwaddr );
                            grp->tthwaddr = strdup( tthwaddr );
                            memcpy( grp->iMAC, ipa_mac->macaddr, IFHWADDRLEN );
                        }

                        if (was_enabled)
                            VERIFY( qeth_enable_interface( dev, grp ) == 0);

                        if (rc != 0)
                        {
                            char msgbuf[256];
                            MSGBUF( msgbuf,
                                "IPA_CMD_SETVMAC(%s,%s) failed",
                                grp->ttifname, tthwaddr );
                            QERRMSG( dev, grp, errno, "E", msgbuf );
                            STORE_HW( ipa->rc, IPA_RC_FFFF );
                        }

                    }
#endif /* defined( OPTION_TUNTAP_SETMACADDR ) */
#endif /* defined( OPTION_W32_CTCI ) */

                }
                /* end case IPA_CMD_SETVMAC:  0x21 */
                break;

            case IPA_CMD_DELVMAC:  /* 0x22 : Delete Layer-2 MAC address */
                {
                MPC_IPA_MAC *ipa_mac = (MPC_IPA_MAC*)(ipa+1);
                int  rc;

                    strcat( dev->dev_data, ": IPA_CMD_DELVMAC" );  /* Prepare the contentstring */
                    rsp_bhr->content = strdup( dev->dev_data );

                    /* Display the request MPC_TH etc., maybe. */
                    DBGUPD( dev, 1, req_th, 0, FROM_GUEST, "%s: Request", dev->dev_data );

                    rc = unregister_mac(grp, dev, ipa_mac->macaddr, MAC_TYPE_UNICST, TRUE);
                    if (rc == 1) {           /* MAC address not in table */
                        STORE_HW(ipa->rc, IPA_RC_L2_MAC_NOT_FOUND);
                    } else {                 /* MAC address removed from table */
                        STORE_HW(ipa->rc, IPA_RC_SUCCESS);
                    }
                }
                /* end case IPA_CMD_DELVMAC:  0x22 */
                break;

            case IPA_CMD_SETGMAC:  /* 0x23 : Set Layer-2 Group Multicast address */
                {
                MPC_IPA_MAC *ipa_mac = (MPC_IPA_MAC*)(ipa+1);
                int  rc;

                    strcat( dev->dev_data, ": IPA_CMD_SETGMAC" );  /* Prepare the contentstring */
                    rsp_bhr->content = strdup( dev->dev_data );

                    /* Display the request MPC_TH etc., maybe. */
                    DBGUPD( dev, 1, req_th, 0, FROM_GUEST, "%s: Request", dev->dev_data );

                    rc = register_mac(grp, dev, ipa_mac->macaddr, MAC_TYPE_MLTCST);
                    if (rc == -1) {          /* MAC table full */
                        STORE_HW(ipa->rc, IPA_RC_L2_ADDR_TABLE_FULL);
                    } else if (rc == 1) {    /* MAC address in table */
                        STORE_HW(ipa->rc, IPA_RC_L2_DUP_MAC);
                    } else {                 /* MAC address added to table */
                        STORE_HW(ipa->rc, IPA_RC_SUCCESS);
                    }
                }
                /* end case IPA_CMD_SETGMAC:  0x23 */
                break;

            case IPA_CMD_DELGMAC:  /* 0x24 : Delete Layer-2 Group Multicast address */
                {
                MPC_IPA_MAC *ipa_mac = (MPC_IPA_MAC*)(ipa+1);
                int  rc;

                    strcat( dev->dev_data, ": IPA_CMD_DELGMAC" );  /* Prepare the contentstring */
                    rsp_bhr->content = strdup( dev->dev_data );

                    /* Display the request MPC_TH etc., maybe. */
                    DBGUPD( dev, 1, req_th, 0, FROM_GUEST, "%s: Request", dev->dev_data );

                    rc = unregister_mac(grp, dev, ipa_mac->macaddr, MAC_TYPE_MLTCST, TRUE);
                    if (rc == 1) {           /* MAC address not in table */
                        STORE_HW(ipa->rc, IPA_RC_L2_GMAC_NOT_FOUND);
                    } else {                 /* MAC address removed from table */
                        STORE_HW(ipa->rc, IPA_RC_SUCCESS);
                    }
                }
                /* end case IPA_CMD_DELGMAC:  0x24 */
                break;

            case IPA_CMD_SETVLAN:  /* 0x25 : Set Layer-2 VLAN */
                {
                    strcat( dev->dev_data, ": IPA_CMD_SETVLAN" );  /* Prepare the contentstring */
                    strcat( dev->dev_data, protoc );               /* Prepare the contentstring */
                    rsp_bhr->content = strdup( dev->dev_data );

                    /* Display the request MPC_TH etc., maybe. */
                    DBGUPD( dev, 1, req_th, 0, FROM_GUEST, "%s: Request", dev->dev_data );

                    STORE_HW(ipa->rc,IPA_RC_OK);

                    ipadatasize = 2;
                }
                /* end case IPA_CMD_SETVLAN:  0x25 */
                break;

            case IPA_CMD_DELVLAN:  /* 0x26 : Delete Layer-2 VLAN */
                {
                    strcat( dev->dev_data, ": IPA_CMD_DELVLAN" );  /* Prepare the contentstring */
                    strcat( dev->dev_data, protoc );               /* Prepare the contentstring */
                    rsp_bhr->content = strdup( dev->dev_data );

                    /* Display the request MPC_TH etc., maybe. */
                    DBGUPD( dev, 1, req_th, 0, FROM_GUEST, "%s: Request", dev->dev_data );

                    STORE_HW(ipa->rc,IPA_RC_OK);

                    ipadatasize = 2;
                }
                /* end case IPA_CMD_DELVLAN:  0x26 */
                break;

            case IPA_CMD_SETIP:  /* 0xB1 : Set Layer-3 IP unicast address */
                {
                MPC_IPA_SIP *ipa_sip = (MPC_IPA_SIP*)(ipa+1);
                U16  retcode;
                int  rc;
                U32  flags;
                char ipaddr[16] = {0};
                char ipmask[16] = {0};
#if defined( OPTION_W32_CTCI )
                BYTE was_enabled;
#endif

                    strcat( dev->dev_data, ": IPA_CMD_SETIP" );  /* Prepare the contentstring */
                    strcat( dev->dev_data, protoc );             /* Prepare the contentstring */
                    rsp_bhr->content = strdup( dev->dev_data );

                    /* Display the request MPC_TH etc., maybe. */
                    DBGUPD( dev, 1, req_th, 0, FROM_GUEST, "%s: Request", dev->dev_data );

                    retcode = IPA_RC_OK;

                    if (proto == IPA_PROTO_IPV4)
                    {
                      /* Save guest IPv4 address and netmask.        */
                      /* Note: ipa_sip->data.ip4.mask often contains */
                      /* 0xFFFFFF00, irrespective of the subnet mask */
                      /* the guest is actually using.                */
                      MSGBUF(ipaddr,"%d.%d.%d.%d",ipa_sip->data.ip4.addr[0],
                                                  ipa_sip->data.ip4.addr[1],
                                                  ipa_sip->data.ip4.addr[2],
                                                  ipa_sip->data.ip4.addr[3]);
                      MSGBUF(ipmask,"%d.%d.%d.%d",ipa_sip->data.ip4.mask[0],
                                                  ipa_sip->data.ip4.mask[1],
                                                  ipa_sip->data.ip4.mask[2],
                                                  ipa_sip->data.ip4.mask[3]);
                      FETCH_FW(flags,ipa_sip->data.ip4.flags);
                      /* Note: whether an address is flagged as default */
                      /* or vipa appear to depend on what the guest is, */
                      /* and how the inferface is defined in the guest. */
                      if (0
                          || flags == IPA_SIP_DEFAULT
                          || flags == IPA_SIP_VIPA
                          || flags == IPA_SIP_TAKEOVER
                      )
                      {
                        /* Register the IPv4 address */
                        rc = register_ipv4(grp, dev, (BYTE*)ipa_sip->data.ip4.addr);
                        if (rc == -1) {          /* IP table full */
                            retcode = IPA_RC_IP_ADDR_TABLE_FULL;
                        } else if (rc == 1) {    /* IP address in table */
                            retcode = IPA_RC_IP_ADDR_ALREADY_USED;
                        } else {                 /* IP address added to table */
                            retcode = IPA_RC_SUCCESS;
                        }

                        /* We only set the interface IPv4 address if the address */
                        /* provided by the guest is different from the address   */
                        /* specified on the configuration statement.             */
                        /* Note: grp->confipaddr4 contains nulls if no address   */
                        /* was specified on the configuration statement.         */
                        if (memcmp( grp->confipaddr4, ipa_sip->data.ip4.addr, 4 ) != 0)
                        {
#if defined( OPTION_W32_CTCI )
                          if (tt32_multiple_ip_support())
                              rc = 0; // 'register_ipv4' did it for us
                          else
                          {
                              /* If the interface is already enabled/up we need to temporarily */
                              /* bring it down (disable it) so we can change the IP address    */
                              /* and then afterwards bring it back up again (enable it).       */
                              was_enabled = grp->enabled ? TRUE : FALSE;

                              if (was_enabled)
                                  VERIFY( qeth_disable_interface( dev, grp ) == 0);
#endif
                              rc = TUNTAP_SetDestAddr( grp->ttifname, ipaddr );
#if defined( OPTION_W32_CTCI )
                              if (was_enabled)
                                  VERIFY( qeth_enable_interface( dev, grp ) == 0);
                          }
#endif
                          if (rc != 0)
                          {
                              char buf[64];
                              MSGBUF( buf, "TUNTAP_SetDestAddr(%s) failed", ipaddr );
                              QERRMSG( dev, grp, errno, "E", buf );
                              retcode = IPA_RC_FFFF;
                          }
                        }

                        ipadatasize = (4 + 4 + 4);
                      }
                      else
                      {
                        retcode = IPA_RC_UNSUPPORTED_SUBCMD;
                      }
                    }
#if defined( ENABLE_IPV6 )
                    else if (proto == IPA_PROTO_IPV6)
                    {
                      FETCH_FW(flags,ipa_sip->data.ip6.flags);
                      if (flags == IPA_SIP_DEFAULT)
                      {
                        /* Register the IPv6 address */
                        rc = register_ipv6(grp, dev, (BYTE*)ipa_sip->data.ip6.addr);
                        if (rc == -1) {          /* IP table full */
                            retcode = IPA_RC_IP_ADDR_TABLE_FULL;
                        } else if (rc == 1) {    /* IP address in table */
                            retcode = IPA_RC_IP_ADDR_ALREADY_USED;
                        } else {                 /* IP address added to table */
                            retcode = IPA_RC_SUCCESS;
                        }

                        // TUNTAP_SetDestAddr issues an ioctl SIOCSIFDSTADDR request,
                        // for which there doesn't appear to be an IPv6 equivalent.
                        // The concept of peer IPv6 addresses doesn't seem to exist,
                        // presumably one is supposed to use routing.

                        ipadatasize = (16 + 16 + 4);
                      }
                    }
#endif /* defined( ENABLE_IPV6 ) */
                    else
                    {
                        retcode = IPA_RC_INVALID_IP_VERSION;
                    }

                    STORE_HW(ipa->rc,retcode);
                }
                /* end case IPA_CMD_SETIP:  0xB1 */
                break;

            case IPA_CMD_QIPASSIST:  /* 0xB2 : Query Layer-3 IP assist capability */
                {

                    strcat( dev->dev_data, ": IPA_CMD_QIPASSIST" );  /* Prepare the contentstring */
                    strcat( dev->dev_data, protoc );                 /* Prepare the contentstring */
                    rsp_bhr->content = strdup( dev->dev_data );

                    /* Display the request MPC_TH etc., maybe. */
                    DBGUPD( dev, 1, req_th, 0, FROM_GUEST, "%s: Request", dev->dev_data );

                    grp->ipae0 |= IPA_SETADAPTERPARMS;
                    grp->ipae4 |= IPA_SETADAPTERPARMS;
                    grp->ipae6 |= IPA_SETADAPTERPARMS;

                    STORE_HW(ipa->rc,IPA_RC_OK);

                    ipadatasize = 0;
                }
                /* end case IPA_CMD_QIPASSIST:  0xB2 */
                break;

            case IPA_CMD_SETASSPARMS:  /* 0xB3 : Set Layer-3 IP assist parameters */
                {
                MPC_IPA_SAS *ipa_sas = (MPC_IPA_SAS*)(ipa+1);
                U32 ano;
                U16 cmd;
                U16 len;
                char anoc[16];

                    FETCH_FW(ano,ipa_sas->hdr.ano);    /* Assist number */
                    FETCH_HW(cmd,ipa_sas->hdr.cmd);    /* Command code */
                    FETCH_HW(len,ipa_sas->hdr.len);    /* Length */
                    MSGBUF(anoc, " 0x%08X", ano);      /* Assist number in hex character */

                    strcat( dev->dev_data, ": IPA_CMD_SETASSPARMS" );  /* Prepare the contentstring */
                    strcat( dev->dev_data, protoc );                   /* Prepare the contentstring */
                    switch(cmd) {
                    case IPA_SAS_CMD_START:      /* 0x0001 */
                        strcat( dev->dev_data, ": START" );            /* Prepare the contentstring */
                        strcat( dev->dev_data, anoc );                 /* Prepare the contentstring */
                        break;
                    case IPA_SAS_CMD_STOP:       /* 0x0002 */
                        strcat( dev->dev_data, ": STOP" );             /* Prepare the contentstring */
                        strcat( dev->dev_data, anoc );                 /* Prepare the contentstring */
                        break;
                    case IPA_SAS_CMD_CONFIGURE:  /* 0x0003 */
                        strcat( dev->dev_data, ": CONFIGURE" );        /* Prepare the contentstring */
                        strcat( dev->dev_data, anoc );                 /* Prepare the contentstring */
                        break;
                    case IPA_SAS_CMD_ENABLE:     /* 0x0004 */
                        strcat( dev->dev_data, ": ENABLE" );           /* Prepare the contentstring */
                        strcat( dev->dev_data, anoc );                 /* Prepare the contentstring */
                        break;
                    case IPA_SAS_CMD_0005:       /* 0x0005 */
                        strcat( dev->dev_data, ": CMD_0005" );         /* Prepare the contentstring */
                        strcat( dev->dev_data, anoc );                 /* Prepare the contentstring */
                        break;
                    case IPA_SAS_CMD_0006:       /* 0x0006 */
                        strcat( dev->dev_data, ": CMD_0006" );         /* Prepare the contentstring */
                        strcat( dev->dev_data, anoc );                 /* Prepare the contentstring */
                        break;
                    case IPA_SAS_CMD_0008:       /* 0x0008 */
                        strcat( dev->dev_data, ": CMD_0008" );         /* Prepare the contentstring */
                        strcat( dev->dev_data, anoc );                 /* Prepare the contentstring */
                        break;
                    default:
                        {
                        char not_supp[12];
                            MSGBUF( not_supp, " (0x%04X)", cmd );
                            strcat( dev->dev_data, ": NOT SUPPORTED" );  /* Prepare the contentstring */
                            strcat( dev->dev_data, not_supp );           /* Prepare the contentstring */
                        }
                    }
                    rsp_bhr->content = strdup( dev->dev_data );

                    /* Display the request MPC_TH etc., maybe. */
                    DBGUPD( dev, 1, req_th, 0, FROM_GUEST, "%s: Request", dev->dev_data );

                    if (proto == IPA_PROTO_IPV4) {
                        if (!(ano & grp->ipas4)) {
                            STORE_HW(ipa->rc,IPA_RC_NOTSUPP);
                            break;
                        }
                    } else if (proto == IPA_PROTO_IPV6) {
                        if (!(ano & grp->ipas6)) {
                            STORE_HW(ipa->rc,IPA_RC_NOTSUPP);
                            break;
                        }
                    } else {
                        if (!(ano & grp->ipas4)) {
                            STORE_HW(ipa->rc,IPA_RC_NOTSUPP);
                            break;
                        }
                    }

                    switch(cmd) {

                    case IPA_SAS_CMD_START:      /* 0x0001 */
                        grp->ipae4 |= ano;
                        grp->ipae6 |= ano;
                        grp->ipae0 |= ano;
                        STORE_HW(ipa_sas->hdr.rc,IPA_RC_OK);
                        STORE_HW(ipa->rc,IPA_RC_OK);
                        break;

                    case IPA_SAS_CMD_STOP:       /* 0x0002 */
                        grp->ipae4 &= (0xFFFFFFFF - ano);
                        grp->ipae6 &= (0xFFFFFFFF - ano);
                        grp->ipae0 &= (0xFFFFFFFF - ano);
                        STORE_HW(ipa_sas->hdr.rc,IPA_RC_OK);
                        STORE_HW(ipa->rc,IPA_RC_OK);
                        break;

                    case IPA_SAS_CMD_CONFIGURE:  /* 0x0003 */
                    case IPA_SAS_CMD_ENABLE:     /* 0x0004 */
                    case IPA_SAS_CMD_0005:       /* 0x0005 */
                    case IPA_SAS_CMD_0006:       /* 0x0006 */
                    case IPA_SAS_CMD_0008:       /* 0x0008 */
                        STORE_HW(ipa_sas->hdr.rc,IPA_RC_OK);
                        STORE_HW(ipa->rc,IPA_RC_OK);
                        break;

                    default:
                    /*  STORE_HW(ipa_sas->hdr.rc,IPA_RC_UNSUPPORTED_SUBCMD);  */
                        STORE_HW(ipa->rc,IPA_RC_UNSUPPORTED_SUBCMD);
                    }

                    ipadatasize = (len + 4);
                }
                /* end case IPA_CMD_SETASSPARMS:  0xB3 */
                break;

            case IPA_CMD_SETIPM:  /* 0xB4 : Set Layer-3 IP multicast address */
                {
                    strcat( dev->dev_data, ": IPA_CMD_SETIPM" );  /* Prepare the contentstring */
                    strcat( dev->dev_data, protoc );              /* Prepare the contentstring */
                    rsp_bhr->content = strdup( dev->dev_data );

                    /* Display the request MPC_TH etc., maybe. */
                    DBGUPD( dev, 1, req_th, 0, FROM_GUEST, "%s: Request", dev->dev_data );

                    STORE_HW(ipa->rc,IPA_RC_OK);

                    ipadatasize = (8 + 16);
                }
                /* end case IPA_CMD_SETIPM:  0xB4 */
                break;

            case IPA_CMD_DELIPM:  /* 0xB5 : Delete Layer-3 IP multicast address */
                {
                    strcat( dev->dev_data, ": IPA_CMD_DELIPM" );  /* Prepare the contentstring */
                    strcat( dev->dev_data, protoc );              /* Prepare the contentstring */
                    rsp_bhr->content = strdup( dev->dev_data );

                    /* Display the request MPC_TH etc., maybe. */
                    DBGUPD( dev, 1, req_th, 0, FROM_GUEST, "%s: Request", dev->dev_data );

                    STORE_HW(ipa->rc,IPA_RC_OK);
                }
                /* end case IPA_CMD_DELIPM:  0xB5 */
                break;

            case IPA_CMD_SETRTG:  /* 0xB6 : Set Layer-3 routing information */
                {
                BYTE *ipa_rtg = (BYTE*)(ipa+1);

                    strcat( dev->dev_data, ": IPA_CMD_SETRTG" );  /* Prepare the contentstring */
                    strcat( dev->dev_data, protoc );              /* Prepare the contentstring */
                    rsp_bhr->content = strdup( dev->dev_data );

                    /* Display the request MPC_TH etc., maybe. */
                    DBGUPD( dev, 1, req_th, 0, FROM_GUEST, "%s: Request", dev->dev_data );

                    ipa_rtg[0] = 0;
                    STORE_HW(ipa->rc,IPA_RC_OK);
                    ipadatasize = 1;
                }
                /* end case IPA_CMD_DELIPM:  0xB6 */
                break;

            case IPA_CMD_DELIP:  /* 0xB7  Delete Layer-3 IP unicast address */
                {
                MPC_IPA_SIP *ipa_sip = (MPC_IPA_SIP*)(ipa+1);
                U16  retcode;
                int  rc;
                U32  flags;

                    strcat( dev->dev_data, ": IPA_CMD_DELIP" );  /* Prepare the contentstring */
                    strcat( dev->dev_data, protoc );             /* Prepare the contentstring */
                    rsp_bhr->content = strdup( dev->dev_data );

                    /* Display the request MPC_TH etc., maybe. */
                    DBGUPD( dev, 1, req_th, 0, FROM_GUEST, "%s: Request", dev->dev_data );

                    retcode = IPA_RC_OK;

                    if (proto == IPA_PROTO_IPV4)
                    {
                      FETCH_FW(flags,ipa_sip->data.ip4.flags);
                      if (flags == IPA_SIP_DEFAULT)
                      {

                        /* Unregister the IPv4 address */
                        rc = unregister_ipv4(grp, dev, (BYTE*)ipa_sip->data.ip4.addr);
                        if (rc == 1) {           /* IP address not in table */
                          retcode = IPA_RC_UNREGISTERED_ADDR;
                        } else {                 /* IP address removed from to table */
                          retcode = IPA_RC_SUCCESS;
                        }

                      }
                    }
#if defined( ENABLE_IPV6 )
                    else if (proto == IPA_PROTO_IPV6)
                    {
                      FETCH_FW(flags,ipa_sip->data.ip6.flags);
                      if (flags == IPA_SIP_DEFAULT)
                      {

                        /* Register the IPv6 address */
                        rc = unregister_ipv6(grp, dev, (BYTE*)ipa_sip->data.ip6.addr);
                        if (rc == 1) {           /* IP address not in table */
                          retcode = IPA_RC_UNREGISTERED_ADDR;
                        } else {                 /* IP address removed from to table */
                          retcode = IPA_RC_SUCCESS;
                        }

                      }
                    }
#endif /* defined( ENABLE_IPV6 ) */
                    else
                    {
                      retcode = IPA_RC_INVALID_IP_VERSION;
                    }

                    STORE_HW(ipa->rc,retcode);
                }
                /* end case IPA_CMD_DELIP:  0xB7 */
                break;

            case IPA_CMD_SETADPPARMS:  /* 0xB8 : Set Adapter Parameters */
                {
                MPC_IPA_SAP *ipa_sap = (MPC_IPA_SAP*)(ipa+1);
                U32 cmd;

                    strcat( dev->dev_data, ": IPA_CMD_SETADPPARMS" );    /* Prepare the contentstring */
                    strcat( dev->dev_data, protoc );                     /* Prepare the contentstring */
                    FETCH_FW(cmd,ipa_sap->cmd);
                    switch(cmd) {
                    case IPA_SAP_QUERY:      /* 0x00000001 */
                        strcat( dev->dev_data, ": QUERY" );              /* Prepare the contentstring */
                        break;
                    case IPA_SAP_SETMAC:     /* 0x00000002 */
                        strcat( dev->dev_data, ": SETMAC" );             /* Prepare the contentstring */
                        {
                        SAP_SMA *sma = (SAP_SMA*)(ipa_sap+1);
                        U32 cmd;
                            FETCH_FW(cmd,sma->cmd);
                            switch(cmd) {
                            case IPA_SAP_SMA_CMD_READ:  /* 0 */
                                strcat( dev->dev_data, ": READ" );       /* Prepare the contentstring */
                                break;
//                          case IPA_SAP_SMA_CMD_REPLACE:  /* 1 */
//                              strcat( dev->dev_data, ": REPLACE" );    /* Prepare the contentstring */
//                              break;
//                          case IPA_SAP_SMA_CMD_ADD:  /* 2 */
//                              strcat( dev->dev_data, ": ADD" );        /* Prepare the contentstring */
//                              break;
//                          case IPA_SAP_SMA_CMD_DEL:  /* 4 */
//                              strcat( dev->dev_data, ": DELETE" );     /* Prepare the contentstring */
//                              break;
//                          case IPA_SAP_SMA_CMD_RESET:  /* 8 */
//                              strcat( dev->dev_data, ": RESET" );      /* Prepare the contentstring */
//                              break;
                            default:
                                {
                                char not_supp[16];
                                    MSGBUF( not_supp, " (0x%08X)", cmd );
                                    strcat( dev->dev_data, ": NOT SUPPORTED" );  /* Prepare the contentstring */
                                    strcat( dev->dev_data, not_supp );           /* Prepare the contentstring */
                                }
                            }
                        }
                        break;
//                  case IPA_SAP_SETGADR:    /* 0x00000004 */
//                      strcat( dev->dev_data, ": SETGADR" );            /* Prepare the contentstring */
//                      break;
//                  case IPA_SAP_SETFADR:    /* 0x00000008 */
//                      strcat( dev->dev_data, ": SETFADR" );            /* Prepare the contentstring */
//                      break;
//                  case IPA_SAP_SETAMODE:   /* 0x00000010 */
//                      strcat( dev->dev_data, ": SETAMODE" );           /* Prepare the contentstring */
//                      break;
//                  case IPA_SAP_SETCFG:     /* 0x00000020 */
//                      strcat( dev->dev_data, ": SETCFG" );             /* Prepare the contentstring */
//                      break;
//                  case IPA_SAP_SETCFGE:    /* 0x00000040 */
//                      strcat( dev->dev_data, ": SETCFGE" );            /* Prepare the contentstring */
//                      break;
//                  case IPA_SAP_BRDCST:     /* 0x00000080 */
//                      strcat( dev->dev_data, ": BRDCST" );             /* Prepare the contentstring */
//                      break;
//                  case IPA_SAP_OSAMSG:     /* 0x00000100 */
//                      strcat( dev->dev_data, ": OSAMSG" );             /* Prepare the contentstring */
//                      break;
//                  case IPA_SAP_SETSNMP:    /* 0x00000200 */
//                      strcat( dev->dev_data, ": SETSNMP" );            /* Prepare the contentstring */
//                      break;
                    case IPA_SAP_CARDINFO:   /* 0x00000400 */
                        strcat( dev->dev_data, ": CARDINFO" );           /* Prepare the contentstring */
                        break;
                    case IPA_SAP_PROMISC:    /* 0x00000800 */
                        strcat( dev->dev_data, ": PROMISC" );            /* Prepare the contentstring */
                        break;
//                  case IPA_SAP_SETDIAG:    /* 0x00002000 */
//                      strcat( dev->dev_data, ": SETDIAG" );            /* Prepare the contentstring */
//                      break;
                    case IPA_SAP_SETACCESS:  /* 0x00010000 */
                        strcat( dev->dev_data, ": SETACCESS" );          /* Prepare the contentstring */
                        break;
                    default:
                        {
                        char not_supp[16];
                            MSGBUF( not_supp, " (0x%08X)", cmd );
                            strcat( dev->dev_data, ": NOT SUPPORTED" );  /* Prepare the contentstring */
                            strcat( dev->dev_data, not_supp );           /* Prepare the contentstring */
                        }
                    }
                    rsp_bhr->content = strdup( dev->dev_data );

                    /* Display the request MPC_TH etc., maybe. */
                    DBGUPD( dev, 1, req_th, 0, FROM_GUEST, "%s: Request", dev->dev_data );

                    switch(cmd) {

                    case IPA_SAP_QUERY:  /*0x00000001 */
                        {
                        SAP_QRY *qry = (SAP_QRY*)(ipa_sap+1);

                            STORE_FW(qry->nlan,0x00000001);
                            qry->lan_type = QETH_LINK_TYPE_FAST_ETH;
                            STORE_FW(qry->suppcm,IPA_SAP_SUPP);
                            STORE_FW(ipa_sap->suppcm,IPA_SAP_SUPP);
                            STORE_HW(ipa_sap->cmdlen,(sizeof(SAP_QRY)+sizeof(MPC_IPA_SAP)-8));
                            STORE_HW(ipa_sap->rc,IPA_RC_OK);
                            STORE_HW(ipa->rc,IPA_RC_OK);
                            ipadatasize = (sizeof(SAP_QRY)+sizeof(MPC_IPA_SAP));
                        }
                        break;

                    case IPA_SAP_SETMAC:  /* 0x00000002 */
                        {
                        SAP_SMA *sma = (SAP_SMA*)(ipa_sap+1);
                        U32 cmd;

                            FETCH_FW(cmd,sma->cmd);
                            switch(cmd) {

                            case IPA_SAP_SMA_CMD_READ:  /* 0 */
                                STORE_FW(sma->asize,IFHWADDRLEN);
                                STORE_FW(sma->nomacs,1);
                                memcpy(sma->addr, grp->iMAC, IFHWADDRLEN);
                                STORE_FW(ipa_sap->suppcm,IPA_SAP_SUPP);
                                STORE_FW(ipa_sap->resv004,IPA_SAP_SUPP);  /* !!!! */
                                STORE_HW(ipa_sap->cmdlen,(sizeof(SAP_SMA)+sizeof(MPC_IPA_SAP)-8));
                                STORE_HW(ipa_sap->rc,IPA_RC_OK);
                                STORE_HW(ipa->rc,IPA_RC_OK);
                                ipadatasize = (sizeof(SAP_SMA)+sizeof(MPC_IPA_SAP));
                                break;

//                          case IPA_SAP_SMA_CMD_REPLACE:  /* 1 */
//                          case IPA_SAP_SMA_CMD_ADD:  /* 2 */
//                          case IPA_SAP_SMA_CMD_DEL:  /* 4 */
//                          case IPA_SAP_SMA_CMD_RESET:  /* 8 */

                            default:
                                STORE_HW(ipa_sap->rc,IPA_RC_UNSUPPORTED_SUBCMD);
                                STORE_HW(ipa->rc,IPA_RC_UNSUPPORTED_SUBCMD);
                            }
                        }
                        break;

                    case IPA_SAP_CARDINFO:  /* 0x00000400 */
                        {
                        SAP_SCI *sci = (SAP_SCI*)(ipa_sap+1);
                            sci->card_type = QETH_CARD_TYPE_OSD;
                            STORE_HW(sci->port_mode,QETH_PORT_MODE_FULLDUPLEX);
                            STORE_FW(sci->port_speed,QETH_PORT_SPEED_10M);
                            STORE_FW(ipa_sap->suppcm,IPA_SAP_SUPP);
                            STORE_HW(ipa_sap->cmdlen,(sizeof(SAP_SCI)+sizeof(MPC_IPA_SAP)-8));
                            STORE_HW(ipa_sap->rc,IPA_RC_OK);
                            STORE_HW(ipa->rc,IPA_RC_OK);
                            ipadatasize = (sizeof(SAP_SCI)+sizeof(MPC_IPA_SAP));
                        }
                        break;

                    case IPA_SAP_PROMISC:  /* 0x00000800 */
                        {
                        SAP_SPM *spm = (SAP_SPM*)(ipa_sap+1);
                        U32 promisc;
                            FETCH_FW(promisc,spm->promisc);
                            grp->promisc = promisc ? MAC_TYPE_PROMISC : 0;
                            DBGTRC(dev, "  IPA_SAP_PROMISC %s",grp->promisc ? "On" : "Off");
                            STORE_HW(ipa_sap->rc,IPA_RC_OK);
                            STORE_HW(ipa->rc,IPA_RC_OK);
                        }
                        break;

                    case IPA_SAP_SETACCESS:  /* 0x00010000 */
                        STORE_HW(ipa_sap->rc,IPA_RC_OK);
                        STORE_HW(ipa->rc,IPA_RC_OK);
                        break;

                    default:
                        STORE_HW(ipa_sap->rc,IPA_RC_UNSUPPORTED_SUBCMD);
                        STORE_HW(ipa->rc,IPA_RC_UNSUPPORTED_SUBCMD);
                    }
                }
                /* end case IPA_CMD_SETADPPARMS:  0xB8 */
                break;

            case IPA_CMD_SETDIAGASS:  /* 0xB9 : Set Layer-3 diagnostic assists */
                {
                    strcat( dev->dev_data, ": IPA_CMD_SETDIAGASS" );  /* Prepare the contentstring */
                    strcat( dev->dev_data, protoc );                  /* Prepare the contentstring */
                    rsp_bhr->content = strdup( dev->dev_data );

                    /* Display the request MPC_TH etc., maybe. */
                    DBGUPD( dev, 1, req_th, 0, FROM_GUEST, "%s: Request", dev->dev_data );

                    STORE_HW(ipa->rc,IPA_RC_OK);
                }
                /* end case IPA_CMD_SETDIAGASS:  0xB9 */
                break;

            case IPA_CMD_CREATEADDR:  /* 0xC3 : Create Layer-3 IPv6 address from Layer-2 MAC address */
                {
                BYTE *ip6 = (BYTE*)(ipa+1);

                    strcat( dev->dev_data, ": IPA_CMD_CREATEADDR" );  /* Prepare the contentstring */
                    strcat( dev->dev_data, protoc );                  /* Prepare the contentstring */
                    rsp_bhr->content = strdup( dev->dev_data );

                    /* Display the request MPC_TH etc., maybe. */
                    DBGUPD( dev, 1, req_th, 0, FROM_GUEST, "%s: Request", dev->dev_data );

                    /* Return the values that the guest wiil use to create   */
                    /* the low-order 64-bits of the IPv6 link local address. */
                    memcpy( ip6+0, grp->iMAC, IFHWADDRLEN );
                    ip6[6] = 0xFF;
                    ip6[7] = 0xFE;
                    ip6[0] |= 0x02; // FIXME: IPA_CMD_CREATEADDR: is this needed?

                    STORE_HW(ipa->rc,IPA_RC_OK);

                    ipadatasize = 8;
                }
                /* end case IPA_CMD_CREATEADDR:  0xC3 */
                break;

            default:
                {
                char cmd_not_supp[10];

                    MSGBUF( cmd_not_supp, " (0x%02X)", ipa->cmd );
                    strcat( dev->dev_data, ": NOT SUPPORTED" );  /* Prepare the contentstring */
                    strcat( dev->dev_data, cmd_not_supp );       /* Prepare the contentstring */
                    rsp_bhr->content = strdup( dev->dev_data );

                    /* Display the request MPC_TH etc., maybe. */
                    DBGUPD( dev, 1, req_th, 0, FROM_GUEST, "%s: Request", dev->dev_data );

                    STORE_HW(ipa->rc,IPA_RC_NOTSUPP);
                }
            }
            /* end switch(ipa->cmd) */

            {
            U32  uLoselen;
            U32  uLength1;
            U32  uLength3;

                if (ipasize != SIZE_IPA || ipadatasize != -1) {
                    uLength3 = ipasize + ipadatasize;
                    uLoselen = lendata - uLength3;
                    uLength1 = rqsize - uLoselen;
                    rsp_bhr->datalen = uLength1;
                    STORE_FW( rsp_th->length, uLength1 );
                    STORE_HW( rsp_rrh->lenfida, (U16)uLength3 );
                    STORE_F3( rsp_rrh->lenalda, uLength3 );
                    STORE_F3( rsp_ph->lendata, uLength3 );
                }

            }

//          ipa->iid = IPA_IID_ADAPTER | IPA_IID_REPLY;
            ipa->iid = IPA_IID_HOST;
            if (proto == IPA_PROTO_IPV4) {
                STORE_FW(ipa->ipas,grp->ipas4);
                grp->ipae4 &= grp->ipas4;
                if (lendata >= SIZE_IPA) {
                    STORE_FW(ipa->ipae,grp->ipae4);
                }
            } else if (proto == IPA_PROTO_IPV6) {
                STORE_FW(ipa->ipas,grp->ipas6);
                grp->ipae6 &= grp->ipas6;
                if (lendata >= SIZE_IPA) {
                    STORE_FW(ipa->ipae,grp->ipae6);
                }
            } else {
                STORE_FW(ipa->ipas,grp->ipas4);
                grp->ipae0 &= grp->ipas4;
                if (lendata >= SIZE_IPA) {
                    STORE_FW(ipa->ipae,grp->ipae0);
                }
            }

            // Add response buffer to chain.
            add_buffer_to_chain( &grp->idx, rsp_bhr );
            signal_idx_event( grp );
        }
        /* end case RRH_TYPE_IPA: */
        break;

    default:
        // HHC03991 "%1d:%04X %s: %s"
        WRMSG( HHC03991, "W", LCSS_DEVNUM, dev->typname,
               "Unknown RHH_TYPE_xxx" );
        net_data_trace( dev, (BYTE*)req_th, datalen, FROM_GUEST, 'I', "???", 0 );

    }
    /* end switch(req_rrh->type) */
}
/* end osa_adapter_cmd */


/*-------------------------------------------------------------------*/
/* Device Command Routine                                            */
/*-------------------------------------------------------------------*/
static void osa_device_cmd(DEVBLK *dev, MPC_IEA *iea, int ieasize)
{
OSA_GRP *grp = (OSA_GRP*)dev->group->grp_data;
OSA_BHR *rsp_bhr;
MPC_IEAR *iear;
U16 reqtype;

    /* Allocate a buffer to which the IEA will be copied */
    /* and then modified, to become the IEAR.            */
    rsp_bhr = alloc_buffer( dev, ieasize+10 );
    if (!rsp_bhr)
        return;
    rsp_bhr->datalen = ieasize;

    /* Point to response IEAR. */
    iear = (MPC_IEAR*)((BYTE*)rsp_bhr + SizeBHR);

    /* Copy request to response buffer. */
    memcpy(iear, iea, ieasize);

    /*  The IDX_ACT_TYPE_READ and IDX_ACT_TYPE_WRITE requests & responses     */
    /*  tell the OSA adapter which of its 256 potential device addresses      */
    /*  make up the, in QETH terms, group. The IDX_ACT_TYPE_READ request      */
    /*  identifies the read device, and the IDX_ACT_TYPE_WRITE request        */
    /*  identifies the write device. Each of the requests contains a list of  */
    /*  device(s) that will be the data device(s). As QETH only supports a    */
    /*  group with a single data device, we only ever see a list with a       */
    /*  single entry. (VTAM supports up to 238 data devices in a group.) The  */
    /*  requests also contain the iea->port variable containing the OSA port  */
    /*  number, something that is of great importance to a real OSA, but      */
    /*  QETH completely ignores. The requests also contains the iea->dataset  */
    /*  variable, which I believe contains the OSA port name for non-QDIO     */
    /*  OSA's (see PUS_0A).                                                   */
    /*  Note: I said earlier that QETH ignores the iea->port variable. What   */
    /*  I should have said is that QETH ignores the variable after checking   */
    /*  that the port number is zero. The check probably should be removed.   */

    FETCH_HW(reqtype, iea->type);
    switch(reqtype) {

    case IDX_ACT_TYPE_READ:

        strcat( dev->dev_data, "IDX_ACT_TYPE_READ" );  /* Prepare the contentstring */
        rsp_bhr->content = strdup( dev->dev_data );

        /* Display the request MPC_IEA, maybe. */
        DBGUPD( dev, 2, iea, ieasize, FROM_GUEST, "%s: Request", dev->dev_data );

        if((iea->port & IDX_ACT_PORT_MASK) != OSA_PORTNO)
        {
            DBGTRC(dev, "IDX_ACT_TYPE_READ: Invalid OSA Port %d",
                (iea->port & IDX_ACT_PORT_MASK));
            dev->qdio.idxstate = MPC_IDX_STATE_INACTIVE;
            break;
        }

        iear->resp &= (0xFF - IDX_RSP_RESP_MASK);
        iear->resp |= IDX_RSP_RESP_OK;
        iear->flags = (IDX_RSP_FLAGS_NOPORTREQ + IDX_RSP_FLAGS_40);
        STORE_FW(iear->token, QTOKEN1);
        STORE_HW(iear->flevel, IDX_RSP_FLEVEL_0201);
        STORE_FW(iear->uclevel, QUCLEVEL);
        dev->qdio.idxstate = MPC_IDX_STATE_ACTIVE;
        dev->qtype = QTYPE_READ;

        break;

    case IDX_ACT_TYPE_WRITE:

        strcat( dev->dev_data, "IDX_ACT_TYPE_WRITE" );  /* Prepare the contentstring */
        rsp_bhr->content = strdup( dev->dev_data );

        /* Display the request MPC_IEA, maybe. */
        DBGUPD( dev, 2, iea, ieasize, FROM_GUEST, "%s: Request", dev->dev_data );

        memcpy( grp->gtissue, iea->token, MPC_TOKEN_LENGTH );  /* Remember guest token issuer */
        grp->ipas4 = IPA_SUPP_IPv4;
#if defined( ENABLE_IPV6 )
        grp->ipas6 = IPA_SUPP_IPv6;
#else
        grp->ipas6 = 0;
#endif
        grp->ipae0 = 0;
        grp->ipae4 = 0;
        grp->ipae6 = 0;

        if((iea->port & IDX_ACT_PORT_MASK) != OSA_PORTNO)
        {
            DBGTRC(dev, "IDX_ACT_TYPE_WRITE: Invalid OSA Port %d",
                (iea->port & IDX_ACT_PORT_MASK));
            dev->qdio.idxstate = MPC_IDX_STATE_INACTIVE;
            break;
        }

        iear->resp &= (0xFF - IDX_RSP_RESP_MASK);
        iear->resp |= IDX_RSP_RESP_OK;
        iear->flags = (IDX_RSP_FLAGS_NOPORTREQ + IDX_RSP_FLAGS_40);
        STORE_FW(iear->token, QTOKEN1);
        STORE_HW(iear->flevel, IDX_RSP_FLEVEL_0201);
        STORE_FW(iear->uclevel, QUCLEVEL);
        dev->qdio.idxstate = MPC_IDX_STATE_ACTIVE;
        dev->qtype = QTYPE_WRITE;

        break;

    default:

        // HHC03991 "%1d:%04X %s: %s"
        WRMSG( HHC03991, "W", LCSS_DEVNUM, dev->typname,
               "Unknown IDX_ACT_TYPE_xxx" );
        net_data_trace( dev, (BYTE*)iea, ieasize, FROM_GUEST, 'I', "IEA?", 0 );

        dev->qdio.idxstate = MPC_IDX_STATE_INACTIVE;

        // Free the response buffer.
        if (rsp_bhr->content) free( rsp_bhr->content );
        free( rsp_bhr );
        rsp_bhr = NULL;

        break;
    }

    // Add response buffer to chain.
    add_buffer_to_chain( &grp->idx, rsp_bhr );
    signal_idx_event( grp );
}


/*-------------------------------------------------------------------*/
/* Raise Adapter Interrupt                                           */
/*-------------------------------------------------------------------*/
static void raise_adapter_interrupt( DEVBLK* dev )
{
    OSA_GRP* grp = (OSA_GRP*) dev->group->grp_data;

    /* Don't waste time queuing interrupts during power off sequence */
    if (sysblk.shutdown)
        return;

    /* Keep trying to OBTAIN_INTLOCK until we either succeed
       or we detect that a halt/clear subchannel was requested
       (which would cause a deadlock if we just blindly tried
       to obtain it).  Note that we test for the halt or clear
       subchannel request WITHOUT first obtaining dev->lock to
       prevent a deadlock that would occur for the same reason
       (since channel.c holds BOTH during HSCH/CSCH processing).
    */
    while (!(dev->scsw.flag2 & (SCSW2_FC_HALT | SCSW2_FC_CLEAR)))
    {
        /* Try to obtain the interrupt lock (OBTAIN_INTLOCK) */
        if (TRY_OBTAIN_INTLOCK( NULL ) == 0)
        {
            /* Interrupt lock obtained; queue the interrupt */
            OBTAIN_DEVLOCK( dev );
            {
                if (grp->debugmask & DBGQETHINTRUPT)
                    DBGTRC( dev, "Adapter Interrupt" );

                dev->pciscsw.flag2 |= SCSW2_Q | SCSW2_FC_START;
                dev->pciscsw.flag3 |= SCSW3_SC_INTER | SCSW3_SC_PEND;
                dev->pciscsw.chanstat = CSW_PCI;

                OBTAIN_IOINTQLK();
                {
                    QUEUE_IO_INTERRUPT_QLOCKED( &dev->pciioint, FALSE );
                    UPDATE_IC_IOPENDING_QLOCKED();
                }
                RELEASE_IOINTQLK();
            }
            RELEASE_DEVLOCK( dev );

            RELEASE_INTLOCK( NULL );
            return;
        }

        /* Yield to hopefully allow current lock owner a chance
           to finish using it and release it before we try again.
        */
        sched_yield();
    }

    /* Halt/Clear Subchannel was requested for device.
       Return immediately without queuing any interrupt.
    */
}


/*-------------------------------------------------------------------*/
/* Internal function return code flags                               */
/*-------------------------------------------------------------------*/

typedef short QRC;              /* Internal function return code     */

#define QRC_SUCCESS      0      /* Successful completion             */
#define QRC_EIOERR      -1      /* Device i/o error reading/writing  */
#define QRC_ESTORCHK    -2      /* STORCHK failure (Prot Key Chk)    */
#define QRC_ENOSPC      -3      /* Out of Storage Blocks             */
#define QRC_EPKEOF      -4      /* EOF while looking for packets     */
#define QRC_EPKTYP      -5      /* Unsupported output packet type    */
#define QRC_EPKSIZ      -6      /* Output packet/frame too large     */
#define QRC_EZEROBLK    -7      /* Zero Length Storage Block         */
#define QRC_EPKSBLEN    -8      /* Packet length <-> SBALE mismatch  */
#define QRC_ESBPKCPY    -9      /* Packet copy wrong ending SBALE    */
#define QRC_ESBNOEOF   -10      /* No last Last Storage Block flag   */


/*-------------------------------------------------------------------*/
/* Helper function to report errors associated with an SBALE.        */
/*-------------------------------------------------------------------*/
static QRC SBALE_Error( char* msg, QRC qrc, DEVBLK* dev,
                        QDIO_SBAL *sbal, BYTE sbalk, int sb )
{
    char errmsg[256] = {0};
    U64 sbala = (U64)((BYTE*)sbal - dev->mainstor);
    U64 sba;
    U32 sblen;

    FETCH_DW( sba,   sbal->sbale[sb].addr   );
    FETCH_FW( sblen, sbal->sbale[sb].length );

    MSGBUF( errmsg, msg, sb, sbala, sbalk, sba, sblen,
        sbal->sbale[sb].flags[0],
        sbal->sbale[sb].flags[3]);

    // HHC03985 "%1d:%04X %s: %s"
    WRMSG( HHC03985, "E", LCSS_DEVNUM,
        dev->typname, errmsg );

    return qrc;
}
/*-------------------------------------------------------------------*/
/* Helper macro to call above helper function.                       */
/*-------------------------------------------------------------------*/
#define SBALE_ERROR(_qrc,_dev,_sbal,_sbalk,_sb)                     \
    SBALE_Error( "** " #_qrc " **: SBAL(%d) @ %llx [%02X]:"         \
        " Addr: %llx Len: %d flags[0,3]: %2.2X %2.2X",              \
        (_qrc), (_dev), (_sbal), (_sbalk), (_sb))


/*-------------------------------------------------------------------*/
/* Helper macro to check if last SBALE fragment                      */
/*-------------------------------------------------------------------*/
#define IS_LAST_SBALE_FRAGMENT( _flag0 )                            \
    (((_flag0) & SBALE_FLAG0_FRAG_LAST) == SBALE_FLAG0_FRAG_LAST )


/*-------------------------------------------------------------------*/
/* Helper macro to check if absolutely the last SBALE                */
/*-------------------------------------------------------------------*/
#define IS_LAST_SBALE_ENTRY( _flag0 )                               \
    ((_flag0) & SBALE_FLAG0_LAST_ENTRY)


/*-------------------------------------------------------------------*/
/* Helper macro to check for logically last SBALE                    */
/*-------------------------------------------------------------------*/
#define LOGICALLY_LAST_SBALE( _flag0, _pack )                       \
    ((_pack) ? (IS_LAST_SBALE_FRAGMENT( _flag0 )                    \
            ||  IS_LAST_SBALE_ENTRY( _flag0 ))                      \
             :  IS_LAST_SBALE_ENTRY( _flag0 ))


/*-------------------------------------------------------------------*/
/* Helper macro to check for logically last SBALE    (O/P only)      */
/*-------------------------------------------------------------------*/
#define WR_LOGICALLY_LAST_SBALE( _flag0 )                           \
    LOGICALLY_LAST_SBALE( (_flag0), grp->wrpack )


/*-------------------------------------------------------------------*/
/* Helper macro to set the SBALE fragment flags                      */
/*-------------------------------------------------------------------*/
#define SET_SBALE_FRAG( _flag0, _frag )                             \
  do {                                                              \
    (_flag0) &= ~(SBALE_FLAG0_LAST_ENTRY | SBALE_FLAG0_FRAG_MASK);  \
    (_flag0) |= (_frag);                                            \
  } while (0)


/*-------------------------------------------------------------------*/
/* Determine Layer 3 IPv4 cast type                                  */
/*-------------------------------------------------------------------*/
static inline int l3_cast_type_ipv4( BYTE* dest_addr, OSA_GRP* grp )
{
U32 dest_hbo;
U32 mask_hbo;
int i;
    FETCH_FW(dest_hbo, dest_addr);
    FETCH_FW(mask_hbo, grp->confpfxmask4);
    /* Note: why check for the loopback address? */
    if (!dest_hbo)
        return HDR3_FLAGS_NOCAST;
    if ((dest_hbo & 0xF0000000) == 0xE0000000) // 224-239 (E0-EF)
        return HDR3_FLAGS_MULTICAST;
    if ((dest_hbo & mask_hbo) == mask_hbo)
        return HDR3_FLAGS_BROADCAST;

    /* Check whether the IPv4 address is in the table. */
    for (i = 0; i < OSA_MAXIPV4; i++)
    {
        if (grp->ipaddr4[i].type == IPV4_TYPE_INUSE &&
            memcmp(grp->ipaddr4[i].addr, dest_addr, 4) == 0)
        {
            return HDR3_FLAGS_UNICAST;
        }
    }
    return HDR3_FLAGS_NOTFORUS;
}


/*-------------------------------------------------------------------*/
/* Determine Layer 3 IPv6 cast type                                  */
/*-------------------------------------------------------------------*/
static inline int l3_cast_type_ipv6( BYTE* dest_addr, OSA_GRP* grp )
{
    static const BYTE dest_zero[16] = {0};
    int i;

    /* Note: why check for the any address? */
    if (memcmp( dest_addr, dest_zero, 16 ) == 0)
        return HDR3_FLAGS_NOCAST;

    if (dest_addr[0] == 0xFF)
        return HDR3_FLAGS_MULTICAST;

    /* Check whether the IPv6 address is in the table. */
    for (i = 0; i < OSA_MAXIPV6; i++)
    {
        if (grp->ipaddr6[i].type == IPV6_TYPE_INUSE &&
            memcmp(grp->ipaddr6[i].addr, dest_addr, 16) == 0)
        {
            return HDR3_FLAGS_UNICAST;
        }
    }

    /* Oh dear, the IPv6 address isn't in the table. */
    return HDR3_FLAGS_NOTFORUS;
}


/*-------------------------------------------------------------------*/
/* Determine if TUN/TAP device has more packets waiting for us.      */
/* Does a 'select' on the TUN/TAP device using a zero timeout        */
/* and returns 1 if more packets are waiting or 0 (false) otherwise. */
/* Note: boolean function. Does not report errors. If the select     */
/* call fails then this function simply returns 0 = false (EOF).     */
/*-------------------------------------------------------------------*/
static BYTE more_packets( DEVBLK* dev )
{
    fd_set readset;
    struct timeval tv = {0,0};
    FD_ZERO( &readset );
    FD_SET( dev->fd, &readset );
    return (qeth_select( dev->fd+1, &readset, &tv ) > 0);
}


/*-------------------------------------------------------------------*/
/* Read one packet/frame from TUN/TAP device into dev->buf.          */
/* dev->buflen updated with length of packet/frame just read.        */
/*-------------------------------------------------------------------*/
static QRC read_packet( DEVBLK* dev, OSA_GRP *grp )
{
    int errnum;

    PTT_QETH_TRACE( "rdpack entr", dev->bufsize, 0, 0 );
    dev->buflen = TUNTAP_Read( dev->fd, dev->buf, dev->bufsize );
    errnum = errno;

    if (unlikely(dev->buflen < 0))
    {
        if (errnum == EAGAIN)
        {
            errno = EAGAIN;
            PTT_QETH_TRACE( "rdpack exit", dev->bufsize, dev->buflen, QRC_EPKEOF );
            return QRC_EPKEOF;
        }
        else
        {
            // HHC00912 "%1d:%04X %s: error reading from device %s: %d %s"
            WRMSG(HHC00912, "E", LCSS_DEVNUM,
                dev->typname, grp->ttifname, errnum, strerror( errnum ));
            errno = errnum;
            PTT_QETH_TRACE( "rdpack exit", dev->bufsize, dev->buflen, QRC_EIOERR );
            return QRC_EIOERR;
        }
    }

    if (unlikely(dev->buflen == 0))
    {
        errno = EAGAIN;
        PTT_QETH_TRACE( "rdpack exit", dev->bufsize, dev->buflen, QRC_EPKEOF );
        return QRC_EPKEOF;
    }

    /* Count packets received */
    dev->qdio.rxcnt++;

    PTT_QETH_TRACE( "rdpack exit", dev->bufsize, dev->buflen, QRC_SUCCESS );
    return QRC_SUCCESS;
}


/*-------------------------------------------------------------------*/
/* Write one L2/L3 packet/frame to the TUN/TAP device.               */
/*-------------------------------------------------------------------*/
static QRC write_packet( DEVBLK* dev, OSA_GRP *grp,
                         BYTE* pkt, int pktlen )
{
    int wrote, errnum;

    PTT_QETH_TRACE( "wrpack entr", 0, pktlen, 0 );
    wrote = TUNTAP_Write( dev->fd, pkt, pktlen );
    errnum = errno;

    if (likely(wrote == pktlen))
    {
        dev->qdio.txcnt++;
        PTT_QETH_TRACE( "wrpack exit", 0, pktlen, QRC_SUCCESS );
        return QRC_SUCCESS;
    }

    // HHC00911 "%1d:%04X %s: error writing to device %s: %d %s"
    WRMSG(HHC00911, "E", LCSS_DEVNUM,
        dev->typname, grp->ttifname, errnum, strerror( errnum ));
    errno = errnum;
    PTT_QETH_TRACE( "wrpack exit", 0, pktlen, QRC_EIOERR );
    return QRC_EIOERR;
}


/*-------------------------------------------------------------------*/
/* Copy data fragment into OSA queue buffer storage.                 */
/* Uses the entries from the passed Storage Block Address List to    */
/* split the fragment across several Storage Blocks as needed.       */
/*-------------------------------------------------------------------*/
/* src points to the data to be copied into the Storage Block.       */
/* rem is the src data length (how much remains to be copied).       */
/* sbal points to the Storage Block Address List for the buffer.     */
/* sbalk is the Storage Block's associated protection key.           */
/* sb is a ptr to the current SBAL Storage Block number.             */
/* frag0 is the SBALE's current first/middle fragment flag.          */
/* sboff is a ptr to the offset into the current Storage Block.      */
/* sbrem is a ptr to how much of the current Storage Block remains.  */
/*-------------------------------------------------------------------*/
static QRC copy_fragment_to_storage( DEVBLK* dev, QDIO_SBAL *sbal,
                                     BYTE sbalk, int* sb, BYTE* frag0,
                                     U32* sboff, U32* sbrem,
                                     BYTE* src, int rem )
{
    U64 sba;                            /* Storage Block Address     */
    BYTE *dst = NULL;                   /* Destination address       */
    int len;                            /* Copy length (work)        */

    /* While src bytes remain to be copied */
    while (rem > 0)
    {
        /* End of current Storage Block? */
        if (!*sbrem && *sboff)
        {
            /* Done using this Storage Block */
            STORE_FW( sbal->sbale[*sb].length, *sboff );
            STORE_FW( sbal->sbale[*sb].flags,     0   );
            SET_SBALE_FRAG( sbal->sbale[*sb].flags[0], *frag0 );

            /* Go on to next Storage Block */
            if (*sb >= (QMAXSTBK-1))
                return SBALE_ERROR( QRC_ENOSPC, dev,sbal,sbalk,*sb);
            *sb = *sb + 1;
            *frag0 = SBALE_FLAG0_FRAG_MIDDLE;
            *sboff = 0;
        }

        /* Starting a new Storage Block? */
        if (!*sboff || !dst)
        {
            /* Check the Storage Block's length and key */
            FETCH_DW(  sba,   sbal->sbale[*sb].addr   );
            FETCH_FW( *sbrem, sbal->sbale[*sb].length );
            if (!*sbrem)
                return SBALE_ERROR( QRC_EZEROBLK, dev,sbal,sbalk,*sb);
            if (STORCHK(sba,(*sbrem)-1,sbalk,STORKEY_CHANGE,dev))
                return SBALE_ERROR( QRC_ESTORCHK, dev,sbal,sbalk,*sb);
            *sbrem -= *sboff;

            /* Calculate new destination address */
            dst = (BYTE*)(dev->mainstor + sba + *sboff);
        }

        /* Continue copying data to Storage Block */
        len = min( *sbrem, (U32)rem );
        memcpy( dst, src, len );

        dst    += len;
        src    += len;
        rem    -= len;
        *sboff += len;
        *sbrem -= len;
    }

    /* PROGRAMMING NOTE: the CALLER will mark last fragment! */
    return QRC_SUCCESS;
}


/*-------------------------------------------------------------------*/
/* Copy packet/frame data from one/more OSA queue storage buffers    */
/* into DEVBLK device buffer. Uses the entries from the passed       */
/* Storage Block Address List to copy the packet/frame data which    */
/* might be split across several Storage Blocks. Stops when either   */
/* the entire output packet has been consolidated into the DEVBLK    */
/* buffer or the ending Storage Block is reached and consumed.       */
/*-------------------------------------------------------------------*/
/* sbal points to the Storage Block Address List for the buffer.     */
/* sbalk is the associated protection key for the queue buffer.      */
/* sb is a ptr to the current SBAL Storage Block number.             */
/* sbsrc is where in the first Storage Block to begin copying from.  */
/* sblen is the length of the first Storage Block minus the OSA hdr. */
/* dev->bufres holds the expected packet/frame size and dev->buflen  */
/* starts at zero. Both are updated as the copying proceeds.         */
/*-------------------------------------------------------------------*/
static QRC copy_storage_fragments( DEVBLK* dev, OSA_GRP *grp,
                                   QDIO_SBAL *sbal, BYTE sbalk,
                                   int* sb, BYTE* sbsrc, U32 sblen )
{
    U64 sba;                            /* Storage Block Address     */
    BYTE *dst;                          /* Destination address       */
    U32 len;                            /* Copy length               */

    dst = dev->buf + dev->buflen;       /* Build destination pointer */

    /* Copy data from each Storage Block in turn until the entire
       packet/frame has been copied or we reach the ending Block. */
    while (dev->bufres > 0)
    {
        /* End of current storage block? */
        if (!sblen)
        {
            /* Is this the last storage block? */
            if (WR_LOGICALLY_LAST_SBALE( sbal->sbale[*sb].flags[0] ))
            {
                /* We have copied as much data as we possibly can but
                dev->bufres is not zero so the Storage Blocks Entries
                are wrong. THIS SHOULD NEVER OCCUR since our device
                buffer size is always set to the maximum size of 64K. */
                return SBALE_ERROR( QRC_EPKSBLEN, dev,sbal,sbalk,*sb);
            }

            /* Request interrupt if needed */
            if (sbal->sbale[*sb].flags[3] & SBALE_FLAG3_PCI_REQ)
            {
                SET_DSCI(dev,DSCI_IOCOMP);
                grp->oqPCI = TRUE;
            }

            /* Retrieve the next storage block entry */
            if (*sb >= (QMAXSTBK-1))
                return SBALE_ERROR( QRC_ENOSPC, dev,sbal,sbalk,*sb);
            *sb = *sb + 1;
            FETCH_DW( sba,   sbal->sbale[*sb].addr   );
            FETCH_FW( sblen, sbal->sbale[*sb].length );
            if (!sblen)
                return SBALE_ERROR( QRC_EZEROBLK, dev,sbal,sbalk,*sb);
            if (STORCHK( sba, sblen-1, sbalk, STORKEY_CHANGE, dev))
                return SBALE_ERROR( QRC_ESTORCHK, dev,sbal,sbalk,*sb);

            /* Point to new data source */
            sbsrc = (BYTE*)(dev->mainstor + sba);
        }

        /* Copying packet/frame to device from this storage block */
        len = min( (U32)dev->bufres, sblen );
        memcpy( dst, sbsrc, len );

        dst         += len;
        dev->buflen += len;
        dev->bufres -= len;
        sbsrc       += len;
        sblen       -= len;
    }

    return QRC_SUCCESS;
}


/*-------------------------------------------------------------------*/
/* Copy packet/frame from dev->buf into OSA queue buffer storage.    */
/* Uses the entries from the passed Storage Block Address List to    */
/* split the packet/frame across several Storage Blocks as needed.   */
/* dev->buflen should be set to the length of the packet/frame.      */
/*-------------------------------------------------------------------*/
/* sbal points to the Storage Block Address List for the buffer.     */
/* sb is the Storage Block number to begin processing with.          */
/* sbalk is the associated protection key for the queue buffer.      */
/* hdr points to pre-built OSA_HDR2/OSA_HDR3 and hdrlen is its size. */
/*-------------------------------------------------------------------*/
static QRC copy_packet_to_storage( DEVBLK* dev, OSA_GRP *grp,
                                   QDIO_SBAL *sbal, int sb, BYTE sbalk,
                                   BYTE* hdr, int hdrlen,
                                   BYTE* frm, int frmlen )
{
    int ssb = sb;                       /* Starting Storage Block    */
    U32 sboff = 0;                      /* Storage Block offset      */
    U32 sbrem = 0;                      /* Storage Block remaining   */
    BYTE frag0;                         /* SBALE fragment flag       */
    QRC qrc;                            /* Internal return code      */

    /* Start with the header first */
    frag0 = SBALE_FLAG0_FRAG_FIRST;
    if ((qrc = copy_fragment_to_storage( dev, sbal, sbalk,
        &sb, &frag0, &sboff, &sbrem, hdr, hdrlen )) < 0 )
        return qrc;

    /* Then copy the packet/frame */
    if ((qrc = copy_fragment_to_storage( dev, sbal, sbalk,
        &sb, &frag0, &sboff, &sbrem, frm, frmlen )) < 0 )
        return qrc;

    /* Mark last fragment */
    frag0 = SBALE_FLAG0_FRAG_LAST;
    STORE_FW( sbal->sbale[sb].length, sboff );
    STORE_FW( sbal->sbale[sb].flags,     0   );
    SET_SBALE_FRAG( sbal->sbale[sb].flags[0], frag0 );

    /* Dump the SBALE's we consumed */
    if (grp->debugmask & DBGQETHSBALE)
    {
        int  i;
        for (i=ssb; i <= sb; i++)
        {
            FETCH_FW( sbrem, sbal->sbale[i].length );
            frag0 = sbal->sbale[i].flags[0];
            DBGTRC( dev, "Input SBALE(%d): flag: %02X Len: %04X (%d)",
                i, frag0, sbrem, sbrem );
        }
    }

    return QRC_SUCCESS;
}


/*-------------------------------------------------------------------*/
/* Read one L2 frame from TAP device into queue buffer storage.      */
/*-------------------------------------------------------------------*/
static QRC read_L2_packets( DEVBLK* dev, OSA_GRP *grp,
                            QDIO_SBAL *sbal, BYTE sbalk )
{
    OSA_HDR2 o2hdr;
    ETHFRM* eth;
    int mactype;
    QRC qrc;
    int sb = 0;     /* Start with Storage Block zero */
    U16  hwEthernetType;
    char cPktType[8];

    do {
        /* Find (another) frame for our MAC */
        eth = (ETHFRM*)dev->buf;
        for(;;)
        {
            if ((qrc = read_packet( dev, grp )) < 0)
                return qrc; /*(probably EOF)*/

            /* Verify the frame is being sent to us */
            if (!(mactype = validate_mac( eth->bDestMAC, MAC_TYPE_ANY, grp )))
                continue; /* (try next packet) */
            /* We found a frame being sent to our MAC */
            break;
        }

        /* Build the Layer 2 OSA header */
        memset( &o2hdr, 0, sizeof( OSA_HDR2 ));
        STORE_HW( o2hdr.pktlen, dev->buflen );
        o2hdr.id = HDR_ID_LAYER2;

        switch( mactype & MAC_TYPE_ANY ) {
        case MAC_TYPE_UNICST:
            o2hdr.flags[2] |= HDR2_FLAGS2_UNICAST;
            break;
        case MAC_TYPE_BRDCST:
            o2hdr.flags[2] |= HDR2_FLAGS2_BROADCAST;
            break;
        case MAC_TYPE_MLTCST:
            o2hdr.flags[2] |= HDR2_FLAGS2_MULTICAST;
            break;
        }

        /* Debugging */
        if (grp->debugmask & DBGQETHPACKET)
        {
            FETCH_HW( hwEthernetType, eth->hwEthernetType );

                 if (ETH_TYPE_IP   == hwEthernetType) STRLCPY( cPktType, "IPv4"    );
            else if (ETH_TYPE_IPV6 == hwEthernetType) STRLCPY( cPktType, "IPv6"    );
            else if (ETH_TYPE_ARP  == hwEthernetType) STRLCPY( cPktType, "ARP"     );
            else if (ETH_TYPE_RARP == hwEthernetType) STRLCPY( cPktType, "RARP"    );
            else if (ETH_TYPE_SNA  == hwEthernetType) STRLCPY( cPktType, "SNA"     );
            else                                      STRLCPY( cPktType, "unknown" );

            // "%1d:%04X %s: Receive frame of size %d bytes (with %s packet) from device %s"
            WRMSG( HHC00986, "D", LCSS_DEVNUM,
                dev->typname, dev->buflen, cPktType, grp->ttifname );
            net_data_trace( dev, (BYTE*) &o2hdr, sizeof( o2hdr ), TO_GUEST, 'D', "L2 hdr", 0 );
            net_data_trace( dev,    dev->buf,     dev->buflen,    TO_GUEST, 'D', "Frame ", 0 );
        }

        /* Copy header and frame to buffer storage block(s) */
        qrc = copy_packet_to_storage( dev, grp, sbal, sb, sbalk,
                                      (BYTE*) &o2hdr, sizeof( o2hdr ),
                                      dev->buf, dev->buflen );
    }
    while (qrc >= 0 && grp->rdpack && more_packets( dev ) && ++sb < QMAXSTBK);

    /* Mark end of buffer */
    if (sb >= QMAXSTBK) sb--;
    sbal->sbale[sb].flags[0] |= SBALE_FLAG0_LAST_ENTRY;

    return qrc;
}


/*-------------------------------------------------------------------*/
/* Read one L3 packet from TUN device into queue buffer storage.     */
/*-------------------------------------------------------------------*/
static QRC read_L3_packets( DEVBLK* dev, OSA_GRP *grp,
                            QDIO_SBAL *sbal, BYTE sbalk )
{
    IP4FRM* ip4;
    IP6FRM* ip6;
    OSA_HDR3 o3hdr;
    QRC qrc;
    BYTE udp = 17;
    int sb = 0;     /* Start with Storage Block zero */
    int   iPktVer;
    char  cPktType[8];

    do
    {
        /* Read another packet into the device buffer */
        if ((qrc = read_packet( dev, grp )) != 0)
            return qrc; /*(probably EOF)*/

        /* Build the Layer 3 OSA header */
        memset( &o3hdr, 0, sizeof( OSA_HDR3 ));
        STORE_HW( o3hdr.length, dev->buflen );
        o3hdr.id = HDR_ID_LAYER3;
//      STORE_HW( o3hdr.frame_offset, ???? ); // TSO only?
//      STORE_FW( o3hdr.token, ???? );

        /* Check the IP packet version. The first 4-bits of the     */
        /* first byte of the IP header contains the version number. */
        iPktVer = ( ( dev->buf[0] & 0xF0 ) >> 4 );
        if (iPktVer == 4)
        {
            ip4 = (IP4FRM*)dev->buf;
            STRLCPY( cPktType, " IPv4" );
            memcpy( &o3hdr.dest_addr[12], &ip4->lDstIP, 4 );
            memcpy( o3hdr.in_cksum, ip4->hwChecksum, 2 );
            o3hdr.flags = l3_cast_type_ipv4( &o3hdr.dest_addr[12], grp );
            if (o3hdr.flags == HDR3_FLAGS_NOTFORUS)
                return QRC_EPKEOF; /* Not our packet */
            o3hdr.ext_flags = (ip4->bProtocol == udp) ? HDR3_EXFLAG_UDP : 0;
        }
        else if (iPktVer == 6)
        {
            ip6 = (IP6FRM*)dev->buf;
            STRLCPY( cPktType, " IPv6" );
            memcpy( o3hdr.dest_addr, ip6->bDstAddr, 16 );
            o3hdr.flags = l3_cast_type_ipv6( o3hdr.dest_addr, grp );
            if (o3hdr.flags == HDR3_FLAGS_NOTFORUS)
                return QRC_EPKEOF; /* Not our packet */
/* ????     o3hdr.flags |= HDR3_FLAGS_PASSTHRU | HDR3_FLAGS_IPV6;    */
            o3hdr.flags |= HDR3_FLAGS_IPV6;
            o3hdr.ext_flags = (ip6->bNextHeader == udp) ? HDR3_EXFLAG_UDP : 0;
        }
        else
        {
            /* Err... not IPv4 or IPv6! */
            STRLCPY( cPktType, "" );
        }

        /* Debugging */
        if (grp->debugmask & DBGQETHPACKET)
        {
            // HHC00913 "%1d:%04X %s: Receive%s packet of size %d bytes from device %s"
            WRMSG(HHC00913, "D", LCSS_DEVNUM, dev->typname,
                            cPktType, dev->buflen, grp->ttifname );
/*          net_data_trace( dev, (BYTE*)&o3hdr, sizeof(o3hdr), TO_GUEST, 'D', "L3 hdr", 0 );        */
            net_data_trace( dev, dev->buf, dev->buflen, TO_GUEST, 'D', "Packet", 0 );
        }

        /* Copy header and packet to buffer storage block(s) */
        qrc = copy_packet_to_storage( dev, grp, sbal, sb, sbalk,
                                      (BYTE*) &o3hdr, sizeof( o3hdr ),
                                      dev->buf, dev->buflen );
    }
    while (qrc >= 0 && grp->rdpack && more_packets( dev ) && ++sb < QMAXSTBK);

    /* Mark end of buffer */
    if (sb >= QMAXSTBK) sb--;
    sbal->sbale[sb].flags[0] |= SBALE_FLAG0_LAST_ENTRY;

    return qrc;
}


/*-------------------------------------------------------------------*/
/* Read one L3 packet from response buffer storage.                  */
/*-------------------------------------------------------------------*/
static QRC read_l3r_buffers( DEVBLK* dev, OSA_GRP *grp,
                             QDIO_SBAL *sbal, BYTE sbalk )
{
    OSA_BHR* bhr;
    BYTE*    bufdata;
    IP4FRM*  ip4;
    IP6FRM*  ip6;
    U32      datalen, length;
    OSA_HDR3 o3hdr;
    QRC      qrc;
    BYTE     udp = 17;
    int      sb = 0;     /* Start with Storage Block zero */
    int      iPktVer;
    char     cPktType[8];

    do
    {
        /* Remove layer 3 response buffer from chain. */
        bhr = remove_buffer_from_chain( &grp->l3r );
        if (!bhr) return QRC_EPKEOF;

        /* Point to response data and get its length. */
        bufdata = (BYTE*)bhr + SizeBHR;
        datalen = bhr->datalen;
        length = bhr->datalen;

        /* Build the Layer 3 OSA header */
        memset( &o3hdr, 0, sizeof( OSA_HDR3 ));
        STORE_HW( o3hdr.length, datalen );
        o3hdr.id = HDR_ID_LAYER3;

        /* Check the IP packet version. The first 4-bits of the     */
        /* first byte of the IP header contains the version number. */
        iPktVer = ( ( bufdata[0] & 0xF0 ) >> 4 );
        if (iPktVer == 4)
        {
            ip4 = (IP4FRM*)bufdata;
            STRLCPY( cPktType, " IPv4" );
            memcpy( &o3hdr.dest_addr[12], &ip4->lDstIP, 4 );
            memcpy( o3hdr.in_cksum, ip4->hwChecksum, 2 );
            o3hdr.flags = l3_cast_type_ipv4( &o3hdr.dest_addr[12], grp );
            if (o3hdr.flags == HDR3_FLAGS_NOTFORUS)
                length = 0; /* Not our packet */
            o3hdr.ext_flags = (ip4->bProtocol == udp) ? HDR3_EXFLAG_UDP : 0;
        }
        else if (iPktVer == 6)
        {
            ip6 = (IP6FRM*)bufdata;
            STRLCPY( cPktType, " IPv6" );
            memcpy( o3hdr.dest_addr, ip6->bDstAddr, 16 );
            o3hdr.flags = l3_cast_type_ipv6( o3hdr.dest_addr, grp );
            if (o3hdr.flags == HDR3_FLAGS_NOTFORUS)
                length = 0; /* Not our packet */
            o3hdr.flags |= HDR3_FLAGS_IPV6;
            o3hdr.ext_flags = (ip6->bNextHeader == udp) ? HDR3_EXFLAG_UDP : 0;
        }
        else
        {
            /* Err... not IPv4 or IPv6! */
            STRLCPY( cPktType, "" );
            length = 0;
        }

        if (length)
        {
            /* Debugging */
            if (grp->debugmask & DBGQETHPACKET)
            {
                // HHC00913 "%1d:%04X %s: Receive%s packet of size %d bytes from device %s"
                WRMSG(HHC00913, "D", LCSS_DEVNUM, dev->typname,
                                cPktType, datalen, "layer 3 response" );
/*              net_data_trace( dev, (BYTE*)&o3hdr, sizeof(o3hdr), TO_GUEST, 'D', "L3 hdr", 0 );    */
                net_data_trace( dev, bufdata, datalen, TO_GUEST, 'D', "Packet", 0 );
            }

            /* Copy header and packet to buffer storage block(s) */
            qrc = copy_packet_to_storage( dev, grp, sbal, sb, sbalk,
                                          (BYTE*)&o3hdr, sizeof(o3hdr),
                                          bufdata, datalen );
        }
        else
            qrc = 0;

        /* Free layer 3 response buffer. Read is complete. */
        if (bhr->content) free( bhr->content );
        free( bhr );
    }
    while (qrc >= 0 && grp->rdpack && grp->l3r.firstbhr && ++sb < QMAXSTBK);

    /* Mark end of buffer */
    if (sb >= QMAXSTBK) sb--;
    sbal->sbale[sb].flags[0] |= SBALE_FLAG0_LAST_ENTRY;

    return qrc;
}


/*-------------------------------------------------------------------*/
/* Write all packets/frames in this primed buffer. Automatically     */
/* handles output packing mode by continuing to next storage Block.  */
/* sbal points to the Storage Block Address List for the buffer.     */
/* sbalk is the associated protection key for the queue buffer.      */
/*-------------------------------------------------------------------*/
static QRC write_buffered_packets( DEVBLK* dev, OSA_GRP *grp,
                                   QDIO_SBAL *sbal, BYTE sbalk )
{
    U64 sba;                            /* Storage Block Address     */
    BYTE* hdr;                          /* Ptr to OSA packet header  */
    OSA_HDR2* o2hdr;                    /* Ptr to OSA layer2 header  */
    OSA_HDR3* o3hdr;                    /* Ptr to OSA layer3 header  */
    BYTE* pkt;                          /* Ptr to packet or frame    */
    U32 sblen;                          /* Length of Storage Block   */
    int hdrlen;                         /* Length of OSA header      */
    int pktlen;                         /* Packet or frame length    */
    int sb;                             /* Storage Block number      */
    int ssb;                            /* Starting Storage Block    */
    QRC qrc;                            /* Internal return code      */
    BYTE hdr_id;                        /* OSA Header Block Id       */
    BYTE flag0;                         /* Storage Block Flag        */

    ETHFRM* eth;                        /* Ethernet frame header     */
    U16  hwEthernetType;
    int iPktVer;
    int iTraceLen;
    char cPktType[8];


    sb = 0;                             /* Start w/Storage Block 0   */

    do
    {
        /* Save starting Storage Block number */
        ssb = sb;

        /* Retrieve the (next) Storage Block and check its key */
        FETCH_DW( sba,   sbal->sbale[sb].addr   );
        FETCH_FW( sblen, sbal->sbale[sb].length );
        if (!sblen)
            return SBALE_ERROR( QRC_EZEROBLK, dev,sbal,sbalk,sb);
        if (STORCHK( sba, sblen-1, sbalk, STORKEY_REF, dev ))
            return SBALE_ERROR( QRC_ESTORCHK, dev,sbal,sbalk,sb);

        /* Get pointer to OSA header */
        hdr = (BYTE*)(dev->mainstor + sba);

        /* Verify Block is long enough to hold the full OSA header.
           FIXME: there is nothing in the specs that requires the
           header to not span multiple Storage Blocks so we should
           should probably support it, but at the moment we do not. */
        if (sblen < max(sizeof(OSA_HDR2),sizeof(OSA_HDR3)))
            WRMSG( HHC03983, "W", LCSS_DEVNUM,
                dev->typname, "** FIXME ** OSA_HDR spans multiple storage blocks." );

        /* Debugging */
        if (grp->debugmask & DBGQETHDATA)
        {
          iTraceLen = sblen;

          // HHC00981 "%1d:%04X %s: Accept data of size %d bytes from guest"
          WRMSG(HHC00981, "D", LCSS_DEVNUM, dev->typname, iTraceLen);
          if (iTraceLen > 256)
          {
            iTraceLen = 256;
            // HHC00980 "%1d:%04X %s: Data of size %d bytes displayed, data of size %d bytes not displayed"
            WRMSG(HHC00980, "D", LCSS_DEVNUM, dev->typname,
                                 iTraceLen, (sblen - iTraceLen) );
          }
          net_data_trace( dev, hdr, iTraceLen, FROM_GUEST, 'D', "data", 0 );
        }

        /* Determine if Layer 2 Ethernet frame or Layer 3 IP packet */
        hdr_id = hdr[0];
        switch (hdr_id)
        {
        U16 length;
        case HDR_ID_LAYER2:
        {
         /* ETHFRM* eth; */
            o2hdr = (OSA_HDR2*)hdr;
            hdrlen = sizeof(OSA_HDR2);
            pkt = hdr + hdrlen;
            FETCH_HW( length, o2hdr->pktlen );
            pktlen = length;
         /* eth = (ETHFRM*)pkt; */
            break;
        }
        case HDR_ID_LAYER3:
        {
            o3hdr = (OSA_HDR3*)hdr;
            hdrlen = sizeof(OSA_HDR3);
            pkt = hdr + hdrlen;
            FETCH_HW( length, o3hdr->length );
            pktlen = length;
            break;
        }
        case HDR_ID_TSO:
        case HDR_ID_OSN:
        default:
            return SBALE_ERROR( QRC_EPKTYP, dev,sbal,sbalk,sb);
        }


        /* Make sure the packet/frame fits in the device buffer */
        if (pktlen > dev->bufsize)
            return SBALE_ERROR( QRC_EPKSIZ, dev,sbal,sbalk,sb);

        /* Copy the actual packet/frame into the device buffer */
        sblen -= hdrlen;
        dev->bufres = pktlen;
        dev->buflen = 0;

        if ((qrc = copy_storage_fragments( dev, grp, sbal, sbalk,
                                           &sb, pkt, sblen )) < 0)
            return qrc;

        /* Save ending flag */
        flag0 = sbal->sbale[sb].flags[0];

        /* Trace the pack/frame if debugging is enabled */
        if (grp->debugmask & DBGQETHSBALE)
            DBGTRC( dev, "Output SBALE(%d-%d): Len: %04X (%d)",
                ssb, sb, dev->buflen, dev->buflen );

        /* Initialize packet pointer and packet length */
        pkt    = dev->buf;
        pktlen = dev->buflen;

        /* I know the following looks pretty weird but it seems to be         */
        /* necessary when using IPv6 over layer 3. IPv6 uses ICMPv6 Neighbor  */
        /* Solicitation (NS) & Neighbor Advertisment (NA) to determine the    */
        /* link layer address (i.e. Ethernet MAC) for an IPv6 address. When   */
        /* the guest sends out a NS, this function sends the packet over the  */
        /* tun to the host, whereupon the host duly ignores the packet. Hence */
        /* this function, and function process_l3_icmpv6_packet, responds to  */
        /* the NS with a NA. It seems that the NA must contain a link local   */
        /* address, otherwise the guest ignores the packet. However, after    */
        /* the guest has received an NA with a link local address every IPv6  */
        /* packet from the guest is an Ethernet frame containing the IPv6     */
        /* packet. Perhaps that's how OSD's actually work? IPv6 packets to    */
        /* the guest don't need to be Ethernet frames.                        */
        /*                                                                    */
        /* Originally the whole of this section was surrounded by             */
        /* '#if defined(ENABLE_IPV6)'/'#endif'. However, implementing zLinux  */
        /* layer 3 support showed that zLinux emmitted IPv4 ARP frames, which */
        /* if sent to the tun interface upset the tun. Subsequent exposure to */
        /* layer 3 VSWITCH prompted a revamp to make checks less specific.    */
        /*                                                                    */
        if (hdr_id == HDR_ID_LAYER3)
        {
            /* Might this be a L2 Ethernet Frame? (and not an L3 IP packet?)  */
            o3hdr = (OSA_HDR3*)hdr;
            if (o3hdr->flags & HDR3_FLAGS_PASSTHRU)
            {
                eth = (ETHFRM*)pkt;
                FETCH_HW( hwEthernetType, eth->hwEthernetType );
                /* */
                if (!(o3hdr->flags & HDR3_FLAGS_IPV6))
                {
                    /* It probably is an L2 Ethernet Frame! */
                    if (hwEthernetType != ETH_TYPE_IP)
                    {
                        /* Can't write L2 Ethernet frame to L3 tun device! */
                        dev->qdio.dropcnt++;
                        if (grp->debugmask & DBGQETHDROP)
                        {
                            // "%1d:%04X %s: %s: Output dropped: %s"
                            WRMSG( HHC03811, "W", LCSS_DEVNUM,
                                dev->typname, grp->ttifname, "L2 frame on tun device" );
                            iTraceLen = pktlen;
                            if (iTraceLen > 128)
                            {
                              iTraceLen = 128;
                              // HHC00980 "%1d:%04X %s: Data of size %d bytes displayed, data of size %d bytes not displayed"
                              WRMSG(HHC00980, "D", LCSS_DEVNUM, dev->typname,
                                                   iTraceLen, (pktlen - iTraceLen) );
                            }
                            net_data_trace( dev, pkt, iTraceLen, FROM_GUEST, 'D', "drop", 0 );
                        }
                        continue; /* (don't write this, probably ARP, packet) */
                    }
                }
                /* Bump past L2 Ethernet header to actual L3 IP packet */
                pkt += sizeof(ETHFRM);
                pktlen -= sizeof(ETHFRM);
                /* Bump past 802.1Q header to actual L3 IP packet */
                if (hwEthernetType == ETH_TYPE_VLANTAG)
                {
                    pkt += 4;
                    pktlen -= 4;
                }
            }
        }

        /* Debugging */
        if (grp->debugmask & DBGQETHPACKET)
        {
            if (!grp->l3)
            {
                eth = (ETHFRM*) pkt;
                FETCH_HW( hwEthernetType, eth->hwEthernetType );

                     if (ETH_TYPE_IP   == hwEthernetType) STRLCPY( cPktType, "IPv4"    );
                else if (ETH_TYPE_IPV6 == hwEthernetType) STRLCPY( cPktType, "IPv6"    );
                else if (ETH_TYPE_ARP  == hwEthernetType) STRLCPY( cPktType, "ARP"     );
                else if (ETH_TYPE_RARP == hwEthernetType) STRLCPY( cPktType, "RARP"    );
                else if (ETH_TYPE_SNA  == hwEthernetType) STRLCPY( cPktType, "SNA"     );
                else                                      STRLCPY( cPktType, "unknown" );

                // HHC00985 "%1d:%04X %s: Send frame of size %d bytes (with %s packet) to device %s"
                WRMSG(HHC00985, "D", LCSS_DEVNUM, dev->typname,
                                     pktlen, cPktType, grp->ttifname );
                net_data_trace( dev, pkt, pktlen, FROM_GUEST, 'D', "Frame ", 0 );
            }
            else
            {
                iPktVer = ((pkt[0] & 0xF0) >> 4);

                     if (iPktVer == 4) STRLCPY( cPktType, " IPv4" );
                else if (iPktVer == 6) STRLCPY( cPktType, " IPv6" );
                else                   STRLCPY( cPktType, " ????" );

                // HHC00910 "%1d:%04X %s: Send%s packet of size %d bytes to device %s"
                WRMSG( HHC00910, "D", LCSS_DEVNUM,
                    dev->typname, cPktType, pktlen, grp->ttifname );
                net_data_trace( dev, pkt, pktlen, FROM_GUEST, 'D', "Packet", 0 );
            }
        }

        /* Write the packet */
        qrc = write_packet( dev, grp, pkt, pktlen );

#if defined( ENABLE_IPV6 )

        /* Layer 3 mode? */
        if (grp->l3)
        {
            if ((iPktVer = ((pkt[0] & 0xF0) >> 4)) == 6)
            {
                IP6FRM* ip6;
                BYTE    icmpv6 = 58;  /* = 0x3A */
                ip6 = (IP6FRM*) pkt;

                if (ip6->bNextHeader == icmpv6)
                    process_l3_icmpv6_packet( dev, grp, ip6 );
            }
        }
#endif /* defined( ENABLE_IPV6 ) */

    }
    while (qrc >= 0 && !IS_LAST_SBALE_ENTRY( flag0 ) && ++sb < QMAXSTBK);

    if (sb < QMAXSTBK)
        return qrc;

    return SBALE_ERROR( QRC_ESBNOEOF, dev,sbal,sbalk,sb-1);
}


/*-------------------------------------------------------------------*/
/*                   Process Input Queues                            */
/*-------------------------------------------------------------------*/
/* We must go through the queues/buffers in a round robin manner     */
/* so that buffers are re-used on a LRU (Least Recently Used) basis. */
/* When no buffers are available we must keep our current position.  */
/* When a buffer becomes available we will advance to that location. */
/* When we reach the end of the buffer queue we will advance to the  */
/* next available queue. When a queue is newly enabled we start at   */
/* the beginning of the queue (this is handled in signal adapter).   */
/*-------------------------------------------------------------------*/
static void process_input_queues( DEVBLK *dev )
{
OSA_GRP *grp = (OSA_GRP*)dev->group->grp_data;
int sqn = dev->qdio.i_qpos;             /* Starting queue number     */
int mq = dev->qdio.i_qcnt;              /* Maximum number of queues  */
int qn = sqn;                           /* Working queue number      */
int did_read = 0;                       /* Indicates some data read  */

    PTT_QETH_TRACE( "prinq entr", 0,0,0 );
    do
    {
        if(dev->qdio.i_qmask & (0x80000000 >> qn))
        {
        QDIO_SLSB *slsb = (QDIO_SLSB*)(dev->mainstor + dev->qdio.i_slsbla[qn]);
        int sbn = dev->qdio.i_bpos[qn]; /* Starting buffer number    */
        int mb = QMAXBUFS;              /* Maximum number of buffers */
        int bn = sbn;                   /* Working buffer number     */
        QRC qrc;                        /* Internal return code      */

            do
            {
                if(slsb->slsbe[bn] == SLSBE_INPUT_EMPTY)
                {
                QDIO_SL *sl = (QDIO_SL*)(dev->mainstor + dev->qdio.i_sla[qn]);
                U64 sbala;              /* Storage Block Address List*/
                BYTE sk;                /* Storage Key               */

                    sk = dev->qdio.i_slk[qn];
                    FETCH_DW( sbala, sl->sbala[bn] );

                    /* Verify Storage Block Address List is accessible */
                    if(STORCHK( sbala, sizeof(QDIO_SBAL)-1, sk, STORKEY_REF, dev ))
                    {
                        DBGTRC(dev, "STORCHK Error SBALA(%llx), Key(%2.2X)", sbala, sk);
                        qrc = QRC_ESTORCHK;
                    }
                    else
                    {
                        /* Read packets into this empty buffer */
                        QDIO_SBAL *sbal = (QDIO_SBAL*)(dev->mainstor + sbala);
                        sk = dev->qdio.i_sbalk[qn];
                        did_read = 1;

                        if (grp->l3)
                        {
                            if (grp->l3r.firstbhr)
                                qrc = read_l3r_buffers( dev, grp, sbal, sk );
                            else
                                qrc = read_L3_packets( dev, grp, sbal, sk );
                        }
                        else
                            qrc = read_L2_packets( dev, grp, sbal, sk );

                        /* Mark the buffer as having been completed */
                        if (qrc >= 0)
                        {
                            if (grp->debugmask & DBGQETHQUEUES)
                                DBGTRC(dev, "Input Queue(%d) Buffer(%d)", qn, bn);

                            slsb->slsbe[bn] = SLSBE_INPUT_COMPLETED;
                            ARCH_DEP( or_dev_4K_storage_key )( dev, dev->qdio.i_slsbla[qn], (STORKEY_REF | STORKEY_CHANGE) );
                            SET_DSCI(dev,DSCI_IOCOMP);
                            grp->iqPCI = TRUE;
                            PTT_QETH_TRACE( "prinq OK", qn,bn,qrc );
                            return;
                        }
                        else if (qrc == QRC_EPKEOF)
                        {
                            /* We didn't find any packets/frames meant
                            for us (perhaps the destination MAC or IP
                            addresses didn't match) so just return. */
                            PTT_QETH_TRACE( "*prcinq EOF", qn,bn,qrc );
                            return;   /* (nothing for us to do) */
                        }
                    }

                    /* Handle errors here since both read_L2_packets
                       and read_L3_packets may also return an error */
                    if (qrc < 0)
                    {
                        slsb->slsbe[bn] = SLSBE_ERROR;
                        ARCH_DEP( or_dev_4K_storage_key )( dev, dev->qdio.i_slsbla[qn], (STORKEY_REF | STORKEY_CHANGE) );
                        SET_ALSI(dev,ALSI_ERROR);
                        grp->iqPCI = TRUE;
                        PTT_QETH_TRACE( "*prcinq ERR", qn,bn,qrc );
                        return;
                    }

                } /* end if (SLSBE_INPUT_EMPTY) */

                /* Go on to the next buffer... */
                if(++bn >= mb)
                    bn = 0;
            }
            while ((dev->qdio.i_bpos[qn] = bn) != sbn);

        } /* end if(dev->qdio.i_qmask & (0x80000000 >> qn)) */

        /* Go on to the next queue... */
        if(++qn >= mq)
            qn = 0;
    }
    while ((dev->qdio.i_qpos = qn) != sqn);

    /* PROGRAMMING NOTE: If we did not actually perform a read
       from the tuntap device in the logic above, then we need
       to do so here at this time. We were called for a reason.
       There are supposedly packets to be read but since we did
       not attempt to read any of them we need to do so at this
       time. Failure to do this causes ACTIVATE QUEUES to call
       us continuously, over and over and over again and again
       and again, because its 'select' function still indicates
       that the socket still has unread data waiting to be read.
    */
    if (!did_read && more_packets( dev ))
    {
        char buff[4096];
        int packet_len = TUNTAP_Read( grp->ttfd, buff, sizeof( buff ));
        if (packet_len)
        {
            dev->qdio.dropcnt++;
            PTT_QETH_TRACE( "*prcinq drop", dev->qdio.i_qmask, 0, 0 );
            if (grp->debugmask & DBGQETHDROP)
            {
                // "%1d:%04X %s: %s: Input dropped: %s"
                WRMSG( HHC03810, "W", LCSS_DEVNUM,
                    dev->typname, grp->ttifname, "No available buffers" );
            }
        }
        /* No available/empty Input Queues were to be found */
        /* Wake up the program so it can process its queues */
        grp->iqPCI = TRUE;
    }
    PTT_QETH_TRACE( "prinq exit", 0,0,0 );
}
/* end process_input_queues */


/*-------------------------------------------------------------------*/
/*                  Process Output Queues                            */
/*-------------------------------------------------------------------*/
static void process_output_queues( DEVBLK *dev )
{
OSA_GRP *grp = (OSA_GRP*)dev->group->grp_data;
int sqn = dev->qdio.o_qpos;             /* Starting queue number     */
int mq = dev->qdio.o_qcnt;              /* Maximum number of queues  */
int qn = sqn;                           /* Working queue number      */
int found_buff = 0;                     /* Found primed O/P buffer   */

    PTT_QETH_TRACE( "proutq entr", 0,0,0 );
    do
    {
        if(dev->qdio.o_qmask & (0x80000000 >> qn))
        {
        QDIO_SLSB *slsb = (QDIO_SLSB*)(dev->mainstor + dev->qdio.o_slsbla[qn]);
        int sbn = dev->qdio.o_bpos[qn]; /* Starting buffer number    */
        int mb = QMAXBUFS;              /* Maximum number of buffers */
        int bn = sbn;                   /* Working buffer number     */

            do
            {
                if(slsb->slsbe[bn] == SLSBE_OUTPUT_PRIMED)
                {
                QDIO_SL *sl = (QDIO_SL*)(dev->mainstor + dev->qdio.o_sla[qn]);
                U64 sbala;              /* Storage Block Address List*/
                BYTE sk;                /* Storage Key               */
                QRC qrc;                /* Internal return code      */

                    if (grp->debugmask & DBGQETHQUEUES)
                        DBGTRC(dev, "Output Queue(%d) Buffer(%d)", qn, bn);

                    found_buff = 1;
                    sk = dev->qdio.o_slk[qn];
                    FETCH_DW( sbala, sl->sbala[bn] );

                    /* Verify Storage Block Address List is accessible */
                    if(STORCHK( sbala, sizeof(QDIO_SBAL)-1, sk, STORKEY_REF, dev ))
                    {
                        DBGTRC(dev, "STORCHK Error SBALA(%llx), Key(%2.2X)", sbala, sk);
                        qrc = QRC_ESTORCHK;
                    }
                    else
                    {
                        /* Write packets from this primed buffer */
                        QDIO_SBAL *sbal = (QDIO_SBAL*)(dev->mainstor + sbala);

                        sk = dev->qdio.o_sbalk[qn];

                        if ((qrc = write_buffered_packets( dev, grp, sbal, sk )) >= 0)
                            slsb->slsbe[bn] = SLSBE_OUTPUT_COMPLETED;
                    }

                    /* Packets written or an error has ocurred */
                    ARCH_DEP( or_dev_4K_storage_key )( dev, dev->qdio.o_slsbla[qn], (STORKEY_REF | STORKEY_CHANGE) );

                    /* Handle errors */
                    if (qrc < 0)
                    {
                        slsb->slsbe[bn] = SLSBE_ERROR;
                        SET_ALSI(dev,ALSI_ERROR);
                        grp->oqPCI = TRUE;
                        PTT_QETH_TRACE( "*proutq ERR", qn,bn,qrc );
                        return;
                    }

                } /* end if(SLSBE_OUTPUT_PRIMED) */

                /* Go on to the next buffer... */
                if(++bn >= mb)
                    bn = 0;
            }
            while ((dev->qdio.o_bpos[qn] = bn) != sbn);

        } /* end if(dev->qdio.o_qmask & (0x80000000 >> qn)) */

        /* Go on to the next queue... */
        if(++qn >= mq)
            qn = 0;
    }
    while ((dev->qdio.o_qpos = qn) != sqn);

    if (!found_buff)
        PTT_QETH_TRACE( "*proutq EOF", dev->qdio.o_qmask,0,0 );

    PTT_QETH_TRACE( "proutq exit", 0,0,0 );
}
/* end process_output_queues */


/*-------------------------------------------------------------------*/
/* Halt device and Clear Subchannel related functions...             */
/*-------------------------------------------------------------------*/
static void qeth_halt_read_device( DEVBLK* dev, OSA_GRP* grp )
{
    obtain_lock( &grp->qlock );
    {
        /* Is read device still active? */
        if (dev->busy && dev->qdio.idxstate == MPC_IDX_STATE_ACTIVE)
        {
            DBGTRC( dev, "Halting read device" );
            {
                /* Ask, then wait for, the READ CCW loop to exit */
                PTT_QETH_TRACE( "b4 halt read", 0,0,0 );
                dev->qdio.idxstate = MPC_IDX_STATE_HALTING;
                signal_condition( &grp->qrcond );
                wait_condition( &grp->qrcond, &grp->qlock );
                PTT_QETH_TRACE( "af halt read", 0,0,0 );
            }
            DBGTRC( dev, "Read device halted" );
        }
    }
    release_lock( &grp->qlock );
}

static void qeth_halt_data_device( DEVBLK* dev, OSA_GRP* grp )
{
    obtain_lock( &grp->qlock );
    {
        /* Is data device still active? */
        if (dev->busy && dev->scsw.flag2 & SCSW2_Q)
        {
            BYTE  sig  = QDSIG_HALT;

            DBGTRC( dev, "Halting data device" );
            {
                /* Ask, then wait for, the Activate Queues loop to exit */
                PTT_QETH_TRACE( "b4 halt data", 0,0,0 );
                VERIFY( qeth_write_pipe( grp->ppfd[1], &sig ) == 1);
                wait_condition( &grp->qdcond, &grp->qlock );
                dev->scsw.flag2 &= ~SCSW2_Q;
                PTT_QETH_TRACE( "af halt data", 0,0,0 );
            }
            DBGTRC( dev, "Data device halted" );
        }
    }
    release_lock (&grp->qlock );
}

/*-------------------------------------------------------------------*/
/*                  QETH Halt or Clear Subchannel                    */
/*-------------------------------------------------------------------*/
/* This function is called by channel.c in response to a HSCH (Halt  */
/* Subchannel) or CSCH (Clear Subchannel) instruction.  Upon entry,  */
/* both INTLOCK (sysblk.intlock) and dev->lock are held.             */
/*-------------------------------------------------------------------*/
static void qeth_halt_or_clear( DEVBLK* dev )
{
    OSA_GRP* grp = (OSA_GRP*) dev->group->grp_data;

    if (QTYPE_READ == dev->qtype)
        qeth_halt_read_device( dev, grp );
    else if (QTYPE_DATA == dev->qtype)
        qeth_halt_data_device( dev, grp );
    else
    {
        DBGTRC( dev, "qeth_halt_or_clear: noop!" );
        PTT_QETH_TRACE( "*halt noop", dev->devnum, 0,0 );
    }
}

/*-------------------------------------------------------------------*/
/* Read Configuration Data function                                  */
/*-------------------------------------------------------------------*/
static int qeth_read_configuration_data( DEVBLK* dev, BYTE* buffer, int bufsz )
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
static int qeth_init_handler( DEVBLK *dev, int argc, char *argv[] )
{
OSA_GRP *grp;
int groupsize = OSA_GROUP_SIZE;
int grouped = 0;
int i;
int retcode = 0;
U32 mask4;

//  if (dev->numconfdev > groupsize)
//      groupsize = dev->numconfdev;

    if (!dev->group)
    {
        /* This code is executed for each device in the group. */


        memset( dev->sense,        0,       sizeof( dev->sense     ));
        memcpy( dev->devid, sense_id_bytes, sizeof( sense_id_bytes ));

        dev->rcd         =  &qeth_read_configuration_data;
        dev->numsense    =  32;
        dev->numdevid    =  sizeof( sense_id_bytes );
        dev->devtype     =  dev->devid[1] << 8 | dev->devid[2];
        dev->chptype[0]  =  CHP_TYPE_OSD;
        dev->pmcw.flag4 |=  PMCW4_Q;
        dev->fd          =  -1;

        /* Setting dev->bufsize = 0xFFFF causes, on return to attach_device */
        /* in config.c, storage of size 0xFFFF bytes to be obtained, and    */
        /* the storage address to be placed in dev->buf. The storage is     */
        /* used to receive data from the TUNTAP interface, so 0xFFFF is     */
        /* effectively the largest MTU that could ever be used by QETH.     */

        dev->bufsize = 0xFFFF;    /* maximum packet/frame size */

        if (!(grouped = group_device( dev, groupsize )) && !dev->member)
        {
            char buf[32];

            /* This code is executed for the first device in the group. */

            dev->group->grp_data = grp = malloc( sizeof( OSA_GRP ));
            memset( grp, 0, sizeof( OSA_GRP ));

            initialize_condition( &grp->qrcond );
            initialize_condition( &grp->qdcond );

            initialize_lock( &grp->qlock );
            MSGBUF( buf,    "&grp->qlock %1d:%04X",       LCSS_DEVNUM );
            set_lock_name(   &grp->qlock, buf );

            initialize_lock( &grp->idx.lockbhr );
            MSGBUF( buf,    "&grp->idx.lockbhr %1d:%04X", LCSS_DEVNUM );
            set_lock_name(   &grp->idx.lockbhr, buf );

            initialize_lock( &grp->l3r.lockbhr );
            MSGBUF( buf,    "&grp->l3r.lockbhr %1d:%04X", LCSS_DEVNUM );
            set_lock_name(   &grp->l3r.lockbhr, buf );

            /* Create ACTIVATE QUEUES signalling pipe */
            /* Check your return codes, Jan. */

            VERIFY( !create_pipe( grp->ppfd ));

            /* Set NON-Blocking mode (i.e. disable blocking mode) */

            VERIFY( socket_set_blocking_mode( grp->ppfd[0], 0 ) == 0);
            VERIFY( socket_set_blocking_mode( grp->ppfd[1], 0 ) == 0);

            /* Set defaults */

            grp->ttdev = strdup( DEF_NETDEV );
            grp->ttfd  = -1;
        }
        else
            /* This code is executed for the second and subsequent devices in the group. */
            grp = dev->group->grp_data;
    }
    else
        /* This code is not executed. ??? */
        grp = dev->group->grp_data;

    /*-----------------------------------------------------------*/
    /* PROCESS ALL COMMAND LINE OPTIONS HERE.                    */
    /* Each device in the group will execute this loop.          */
    /*-----------------------------------------------------------*/
    /*                                                           */
    /* NOTE: The following configuration statement:              */
    /*                                                           */
    /*    0800-0802 QETH mtu 1492 ipaddr 192.168.2.1 debug       */
    /*                                                           */
    /* results in exactly the same QETH group as the following   */
    /* configuration statements:                                 */
    /*                                                           */
    /*    0802 QETH mtu 1492                                     */
    /*    0800 QETH ipaddr 192.168.2.1                           */
    /*    0801 QETH debug                                        */
    /*                                                           */
    /* This is either a bug or a design feature, depending       */
    /* on your point of view.                                    */
    /*                                                           */
    /*-----------------------------------------------------------*/

    /* PROGRAMMING NOTE: argument validation is deferred until the
       group is complete. Thus the below argument processing loop
       just saves the argument values but doesn't validate them. */

    for (i=0; i < argc; i++)
    {
        if (!strcasecmp("iface",argv[i]) && (i+1) < argc)
        {
            free( grp->ttdev );
            grp->ttdev = strdup(argv[++i]);
            continue;
        }
#if !defined( OPTION_W32_CTCI )
        else if(!strcasecmp("ifname",argv[i]) && (i+1) < argc)
        {
            STRLCPY( grp->ttifname, argv[++i] );
            continue;
        }
#endif
        else if(!strcasecmp("hwaddr",argv[i]) && (i+1) < argc)
        {
            free( grp->tthwaddr );
            grp->tthwaddr = strdup(argv[++i]);
            continue;
        }
        else if(!strcasecmp("ipaddr",argv[i]) && (i+1) < argc)
        {
            char  *slash, *prfx;
            slash = strchr( argv[i+1], '/' );  /* Point to slash character */
            if (slash) {                       /* If there is a slash      */
                slash[0] = 0;                  /* Replace slash with null  */
                prfx = slash + 1;              /* Point to prefix size     */
                free( grp->ttpfxlen );
                grp->ttpfxlen = strdup(prfx);
            }
            free( grp->ttipaddr );
            grp->ttipaddr = strdup(argv[++i]);
            continue;
        }
        else if(!strcasecmp("netmask",argv[i]) && (i+1) < argc)
        {
            free( grp->ttnetmask );
            grp->ttnetmask = strdup(argv[++i]);
            continue;
        }
#if defined( ENABLE_IPV6 )
        else if(!strcasecmp("ipaddr6",argv[i]) && (i+1) < argc)
        {
            char  *slash, *prfx;
            free( grp->ttipaddr6 );
            free( grp->ttpfxlen6 );
            slash = strchr( argv[i+1], '/' );  /* Point to slash character */
            if (slash) {                       /* If there is a slash      */
                prfx = slash + 1;              /* Point to prefix size     */
                slash[0] = 0;                  /* Replace slash with null  */
            }
            else {
                prfx = "128";
            }
            grp->ttpfxlen6 = strdup(prfx);
            grp->ttipaddr6 = strdup(argv[++i]);
            continue;
        }
#endif /* defined( ENABLE_IPV6 ) */
        else if(!strcasecmp("mtu",argv[i]) && (i+1) < argc)
        {
            free( grp->ttmtu );
            grp->ttmtu = strdup(argv[++i]);
            continue;
        }
        else if(!strcasecmp("chpid",argv[i]) && (i+1) < argc)
        {
            free( grp->ttchpid );
            grp->ttchpid = strdup(argv[++i]);
            continue;
        }
        else if (!strcasecmp("debug",argv[i]))
        {
            grp->debugmask = DBGQETHPACKET+DBGQETHDATA+DBGQETHUPDOWN;
            continue;
        }
        else if (!strcasecmp("debugpacket",argv[i]))
        {
            grp->debugmask |= DBGQETHPACKET;
            continue;
        }
        else if (!strcasecmp("debugdata",argv[i]))
        {
            grp->debugmask |= DBGQETHDATA;
            continue;
        }
        else if (!strcasecmp("debugupdown",argv[i]))
        {
            grp->debugmask |= DBGQETHUPDOWN;
            continue;
        }
        else if(!strcasecmp("nodebug",argv[i]))
        {
            grp->debugmask = 0;
            continue;
        }
        else
        {
            // HHC00918 "%1d:%04X %s: option %s unknown or specified incorrectly"
            WRMSG(HHC00918, "E", LCSS_DEVNUM, dev->typname, argv[i] );
        }
    }

    if (grouped)
    {
        /* Validate the arguments now that the group is complete. */

        DEVBLK  *cua;
        U16      destlink;
        int      i, work_rc, pfxlen, chpid;
        MAC      mac;
        HRB      hrb;
        char     c;
        char    *p;
        static const BYTE zeromac[IFHWADDRLEN] = {0};

#if defined( OPTION_W32_CTCI )
        /* The 'ipaddr' and 'netmask' are REQUIRED parameters */
        if (!grp->ttipaddr)
        {
            // "%1d:%04X %s: Required parameter '%s' missing"
            WRMSG( HHC00917, "E", LCSS_DEVNUM, dev->typname, "ipaddr" );
            retcode = -1;
        }
        if (!grp->ttnetmask && !grp->ttpfxlen)
        {
            // "%1d:%04X %s: Required parameter '%s' missing"
            WRMSG( HHC00917, "E", LCSS_DEVNUM, dev->typname, "netmask" );
            retcode = -1;
        }
#endif

        /* If no tuntap device was specified use the defined default */
        if (!grp->ttdev)
            grp->ttdev = strdup( DEF_NETDEV );

        /* Check the grp->tthwaddr value */
        if (grp->tthwaddr)
        {
            if (ParseMAC( grp->tthwaddr, mac ) != 0 ||
                memcmp( mac, zeromac, IFHWADDRLEN ) == 0)
            {
                // HHC00916 "%1d:%04X %s: option %s value %s invalid"
                WRMSG(HHC00916, "E", LCSS_DEVNUM, dev->typname,
                                     "hwaddr", grp->tthwaddr );
                retcode = -1;
                free(grp->tthwaddr);
                grp->tthwaddr = NULL;
            }
        }

        /* Check the grp->ttipaddr and grp->ttpfxlen values */
        if (grp->ttipaddr)
        {
            // Check whether a numeric IPv4 address has been specified.
            memset( &hrb, 0, sizeof(hrb) );
            hrb.wantafam = AF_INET;
            hrb.numeric = TRUE;
            memcpy( hrb.host, grp->ttipaddr, strlen(grp->ttipaddr) );
            work_rc = resolve_host( &hrb);
            if (work_rc == 0)
            {
                hinet_pton( AF_INET, grp->ttipaddr, grp->confipaddr4 );
            }
            else
            {
                // HHC00916 "%1d:%04X %s: option %s value %s invalid"
                WRMSG(HHC00916, "E", LCSS_DEVNUM, dev->typname,
                                     "ipaddr", grp->ttipaddr );
                retcode = -1;
                free(grp->ttipaddr);
                grp->ttipaddr = NULL;
                if(grp->ttpfxlen)
                {
                    free(grp->ttpfxlen);
                    grp->ttpfxlen = NULL;
                }
                if(grp->ttnetmask)
                {
                    free(grp->ttnetmask);
                    grp->ttnetmask = NULL;
                }
            }
        }

#if defined( OPTION_W32_CTCI )
        if (!grp->ttnetmask && !grp->ttpfxlen)
        {
            grp->ttnetmask = strdup("255.255.255.255");
            grp->ttpfxlen = strdup("32");
        }
#endif

        if (grp->ttnetmask)
        {
            char *new_ttpfxlen = NULL;
            /* Build new prefix length based on netmask */
            if (netmask2prefix( grp->ttnetmask, &new_ttpfxlen ) != 0)
            {
                // HHC00916 "%1d:%04X %s: option %s value %s invalid"
                WRMSG(HHC00916, "E", LCSS_DEVNUM, dev->typname,
                                     "netmask", grp->ttnetmask );
                retcode = -1;
                free( grp->ttnetmask );
                free( grp->ttipaddr  );
                free( grp->ttpfxlen  );
                grp->ttnetmask = NULL;
                grp->ttipaddr  = NULL;
                grp->ttpfxlen  = NULL;
            }
            else if (grp->ttpfxlen)
            {
                /* Check netmask value (via newly built prefix)
                   for consistency with existing prefix length */
                if (grp->ttpfxlen &&
                    strcmp( new_ttpfxlen, grp->ttpfxlen ) != 0)
                {
                    // HHC03998 "%1d:%04X %s: %s inconsistent with %s"
                    WRMSG(HHC03998, "W", LCSS_DEVNUM, dev->typname,
                        "prefix length", "netmask" );
                }
                /* Use consistent prefix length */
                free( grp->ttpfxlen );
                grp->ttpfxlen = new_ttpfxlen;
            }
        }
        if (grp->ttpfxlen)
        {
            char *new_ttnetmask = NULL;
            /* Build new netmask based on prefix length */
            if (prefix2netmask( grp->ttpfxlen, &new_ttnetmask ) != 0)
            {
                // HHC00916 "%1d:%04X %s: option %s value %s invalid"
                WRMSG(HHC00916, "E", LCSS_DEVNUM, dev->typname,
                                     "ipaddr", grp->ttipaddr );
                retcode = -1;
                free( grp->ttpfxlen  );
                free( grp->ttipaddr  );
                free( grp->ttnetmask );
                grp->ttpfxlen  = NULL;
                grp->ttipaddr  = NULL;
                grp->ttnetmask = NULL;
            }
            else if (grp->ttnetmask)
            {
                /* Check prefix length (via newly built netmask)
                   for consistency with existing netmask value */
                if (grp->ttnetmask &&
                    strcmp( new_ttnetmask, grp->ttnetmask ) != 0)
                {
                    // HHC03998 "%1d:%04X %s: %s inconsistent with %s"
                    WRMSG(HHC03998, "W", LCSS_DEVNUM, dev->typname,
                        "netmask", "prefix length" );
                }
                /* Use consistent netmask */
                free( grp->ttnetmask );
                grp->ttnetmask = new_ttnetmask;
            }
        }

#if defined( ENABLE_IPV6 )
        /* Check the grp->ttipaddr6 and grp->ttpfxlen6 values */
        if (grp->ttipaddr6)
        {
            // Check whether a numeric IPv6 address has been specified.
            memset( &hrb, 0, sizeof(hrb) );
            hrb.wantafam = AF_INET6;
            hrb.numeric = TRUE;
            memcpy( hrb.host, grp->ttipaddr6, strlen(grp->ttipaddr6) );
            work_rc = resolve_host( &hrb);
            if (work_rc == 0)
            {
                hinet_pton( AF_INET6, grp->ttipaddr6, grp->confipaddr6 );
            }
            else
            {
                // HHC00916 "%1d:%04X %s: option %s value %s invalid"
                WRMSG(HHC00916, "E", LCSS_DEVNUM, dev->typname,
                                     "ipaddr6", grp->ttipaddr6 );
                retcode = -1;
                free( grp->ttipaddr6 );
                free( grp->ttpfxlen6 );
                grp->ttipaddr6 = NULL;
                grp->ttpfxlen6 = NULL;
            }
        }
        if (grp->ttpfxlen6)
        {
            // Check whether a numeric prefix in the range 1 to 128 has been specified.
            work_rc = 0;
            for (p = grp->ttpfxlen6; isdigit((unsigned char)*p); p++) { }
            if (*p != '\0' || !strlen(grp->ttpfxlen6))
                work_rc = -1;
            pfxlen = atoi(grp->ttpfxlen6);
            if (work_rc != 0 || pfxlen > 128 )
            {
                // HHC00916 "%1d:%04X %s: option %s value %s invalid"
                WRMSG(HHC00916, "E", LCSS_DEVNUM, dev->typname,
                                     "ipaddr6", grp->ttpfxlen6 );
                retcode = -1;
                free( grp->ttpfxlen6 );
                free( grp->ttipaddr6 );
                grp->ttpfxlen6 = NULL;
                grp->ttipaddr6 = NULL;
            }
        }
#endif /* defined( ENABLE_IPV6 ) */

        /* Check the MTU value */
        if (grp->ttmtu)
        {
            U16 uMTU = (U16) atoi( grp->ttmtu );
            if (uMTU < QETH_MIN_MTU)
            {
                // HHC00916 "%1d:%04X %s: option %s value %s invalid"
                WRMSG( HHC00916, "E", LCSS_DEVNUM,
                    dev->typname, "mtu", grp->ttmtu );
                retcode = -1;
                free( grp->ttmtu );
                grp->ttmtu = NULL;
            }
        }

        /* Check the grp->ttchpid value */
        if (grp->ttchpid)
        {
            if(sscanf(grp->ttchpid, "%x%c", &chpid, &c) != 1 || chpid < 0x00 || chpid > 0xFF)
            {
                // HHC00916 "%1d:%04X %s: option %s value %s invalid"
                WRMSG(HHC00916, "E", LCSS_DEVNUM, dev->typname,
                                     "chpid", grp->ttchpid );
                retcode = -1;
                free(grp->ttchpid);
                grp->ttchpid = NULL;
            }
            else
            {
                for (i = 0; i < dev->group->members; i++)
                {
                    dev->group->memdev[i]->pmcw.chpid[0] = chpid;
                }
            }
        }

        /* Initialize each device's Full Link Address array */
        cua = dev->group->memdev[0];
        destlink = 0x000D; // ZZ FIXME: where should this come from?
        for(i = 0; i < dev->group->members; i++) {
            dev->group->memdev[i]->fla[0] =
                (destlink << 8) | (cua->devnum & 0x00FF);
        }

        /* Initialize mask fields */
        if(grp->ttpfxlen) {
            mask4 = makepfxmask4( grp->ttpfxlen );
            STORE_FW( grp->confpfxmask4, mask4 );
        }
        if(grp->ttpfxlen6)
            makepfxmask6( grp->ttpfxlen6, grp->confpfxmask6 );
    }

    return retcode;

} /* end function qeth_init_handler */


/*-------------------------------------------------------------------*/
/* Query the device definition                                       */
/*-------------------------------------------------------------------*/
static void qeth_query_device (DEVBLK *dev, char **devclass,
                               int buflen, char *buffer)
{
char filename[ PATH_MAX + 1 ];          /* full path or just name    */

char qdiostat[80] = {0};
char incomplete[16] = {0};
char status[sizeof(qdiostat)] = {0};
char active[8] = {0};
OSA_GRP *grp;

    BEGIN_DEVICE_CLASS_QUERY( "OSA", dev, devclass, buflen, buffer );

    grp = (OSA_GRP*) dev->group->grp_data;

    if (dev->group->acount == dev->group->members)
    {
        char ttifname[IFNAMSIZ+2];
        char dropped[17] = {0}; // " dr[%u]"

        STRLCPY( ttifname, grp->ttifname );
        if (ttifname[0])
            STRLCAT( ttifname, " " );

        if (grp->debugmask & DBGQETHDROP)
            MSGBUF( dropped, " dr[%u]", dev->qdio.dropcnt );

        MSGBUF( qdiostat, "%stx[%u] rx[%u]%s "
            , ttifname
            , dev->qdio.txcnt
            , dev->qdio.rxcnt
            , dropped
        );
    }

    if (dev->group->acount != dev->group->members)
        STRLCPY( incomplete, "*Incomplete " );

    if (dev->scsw.flag2 & SCSW2_Q)
        STRLCPY( status, qdiostat );

    if (dev->qdio.idxstate == MPC_IDX_STATE_ACTIVE)
        STRLCPY( active, "IDX " );

    snprintf( buffer, buflen, "QDIO %s%s%s%sIO[%"PRIu64"]"
        , incomplete
        , status
        , active
        , grp ? (grp->debugmask ? "debug " : "") : ""
        , dev->excps
    );

} /* end function qeth_query_device */


/*-------------------------------------------------------------------*/
/* Close the device                                                  */
/*-------------------------------------------------------------------*/
static int qeth_close_device ( DEVBLK *dev )
{
DEVGRP *group = (DEVGRP*)dev->group;
OSA_GRP *grp = (OSA_GRP*)(group ? group->grp_data : NULL);

    if (!dev->member && dev->group && dev->group->grp_data)
    {
        int i, ttfd = grp->ttfd;

        PTT_QETH_TRACE( "b4 clos halt", 0,0,0 );
        for (i=0; i < dev->group->members; i++)
        {
            if (QTYPE_READ == group->memdev[i]->qtype)
                qeth_halt_read_device( group->memdev[i], grp );
            else if (QTYPE_DATA == group->memdev[i]->qtype)
                qeth_halt_data_device( group->memdev[i], grp );
        }
        USLEEP( OSA_TIMEOUTUS ); /* give it time to exit */
        PTT_QETH_TRACE( "af clos halt", 0,0,0 );

        PTT_QETH_TRACE( "b4 clos ttfd", 0,0,0 );
        grp->ttfd = -1;
        dev->fd = -1;
        if(ttfd > 0)
            TUNTAP_Close(ttfd);
        PTT_QETH_TRACE( "af clos ttfd", 0,0,0 );

        PTT_QETH_TRACE( "b4 clos pipe", 0,0,0 );
        if(grp->ppfd[0])
            close_pipe(grp->ppfd[0]);
        if(grp->ppfd[1])
            close_pipe(grp->ppfd[1]);
        PTT_QETH_TRACE( "af clos pipe", 0,0,0 );

        PTT_QETH_TRACE( "b4 clos othr", 0,0,0 );

        free( grp->ttdev    );
        free( grp->tthwaddr  );
        free( grp->ttipaddr  );
        free( grp->ttpfxlen  );
        free( grp->ttnetmask );
        free( grp->ttipaddr6 );
        free( grp->ttpfxlen6 );
        free( grp->ttmtu     );
        free( grp->ttchpid   );

        PTT_QETH_TRACE( "af clos othr", 0,0,0 );

        PTT_QETH_TRACE( "b4 clos fbuf", 0,0,0 );
        remove_and_free_any_buffers_on_chain( &grp->idx );
        PTT_QETH_TRACE( "af clos fbuf", 0,0,0 );

        destroy_condition( &grp->qrcond );
        destroy_condition( &grp->qdcond );
        destroy_lock( &grp->qlock );
        destroy_lock( &grp->idx.lockbhr );
        destroy_lock( &grp->l3r.lockbhr );

        PTT_QETH_TRACE( "b4 clos fgrp", 0,0,0 );
        free( group->grp_data );
        group->grp_data = NULL;
        PTT_QETH_TRACE( "af clos fgrp", 0,0,0 );
    }
    else
        dev->fd = -1;

    return 0;
} /* end function qeth_close_device */


#if defined( _FEATURE_QDIO_THININT )
/*-------------------------------------------------------------------*/
/* QDIO Set Subchannel Indicator                                     */
/*-------------------------------------------------------------------*/
static int qeth_set_sci ( DEVBLK *dev, void *desc )
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
#endif /* defined( _FEATURE_QDIO_THININT ) */


/*-------------------------------------------------------------------*/
/* QDIO subsys desc                                                  */
/*-------------------------------------------------------------------*/
static int qeth_ssqd_desc ( DEVBLK *dev, void *desc )
{
    CHSC_RSP24 *rsp24 = (void *)desc;

    STORE_HW(rsp24->sch, dev->subchan);

    if(dev->pmcw.flag4 & PMCW4_Q)
    {
        rsp24->flags |= ( CHSC_FLAG_QDIO_CAPABILITY | CHSC_FLAG_VALIDITY );

        rsp24->qdioac1 |= ( AC1_SIGA_INPUT_NEEDED | AC1_SIGA_OUTPUT_NEEDED );
        rsp24->qdioac1 |= AC1_AUTOMATIC_SYNC_ON_OUT_PCI;

#if defined( _FEATURE_QEBSM )
        if (FACILITY_ENABLED_DEV( HERC_QEBSM ))
        {
            STORE_DW(rsp24->sch_token, IOID2TKN((dev->ssid << 16) | dev->subchan));
            rsp24->qdioac1 |= ( AC1_SC_QEBSM_AVAILABLE | AC1_SC_QEBSM_ENABLED );
        }
#endif

#if defined( _FEATURE_QDIO_THININT )
        if (FACILITY_ENABLED_DEV( HERC_QDIO_THININT ))
            rsp24->qdioac1 |= AC1_AUTOMATIC_SYNC_ON_THININT;
#endif

        rsp24->icnt = QETH_QDIO_READQ;
        rsp24->ocnt = QETH_QDIO_WRITEQ;

        rsp24->qdioac1 |= AC1_UNKNOWN80;
        STORE_HW(rsp24->qdioac2, QETH_AC2_UNKNOWN4000+QETH_AC2_UNKNOWN2000);
    }

    return 0;
}


/*-------------------------------------------------------------------*/
/* Execute a Channel Command Word                                    */
/*-------------------------------------------------------------------*/
static void qeth_execute_ccw ( DEVBLK *dev, BYTE code, BYTE flags,
        BYTE chained, U32 count, BYTE prevcode, int ccwseq,
        BYTE *iobuf, BYTE *more, BYTE *unitstat, U32 *residual )
{
OSA_GRP *grp = (OSA_GRP*)dev->group->grp_data;
U32 num;                                /* Number of bytes to move   */

    UNREFERENCED(flags);
    UNREFERENCED(ccwseq);

    /* Clear the output */
    *more = 0;
    *unitstat = 0;
    *residual = 0;

    /* Command reject if the device group has not been established */
    if((dev->group->acount != dev->group->members)
      && !(IS_CCW_SENSE(code) || IS_CCW_NOP(code) || (code == OSA_RCD)))
    {
        /* Set Intervention required sense, and unit check status */
        dev->sense[0] = SENSE_IR;
        *unitstat = CSW_CE | CSW_DE | CSW_UC;
        return;
    }

    /* Display various information, maybe */
    if (grp->debugmask & DBGQETHCCW)
    {
        // "%1d:%04X %s: Code %02X: Flags %02X: Count %08X: Chained %02X: PrevCode %02X: CCWseq %d"
        WRMSG( HHC03992, "D", LCSS_DEVNUM,
            dev->typname, code, flags, count, chained, prevcode, ccwseq );
    }

    /* Process depending on CCW opcode */
    switch (code) {

    case 0x01:
    /*---------------------------------------------------------------*/
    /* WRITE                                                         */
    /*---------------------------------------------------------------*/
    {
    int      datalen, length;
    U32      first4;
    char     contentstring[256] = {0};

        /* Prepare the contentstring. */
        dev->dev_data = &contentstring;

        /* Get the first 4-bytes of the data. */
        FETCH_FW( first4, iobuf );
        length = datalen = count;

        /* Process the request */
        if (first4 == MPC_TH_FIRST4)
        {
            /* Process the request MPC_TH etc. */
            osa_adapter_cmd(dev,(MPC_TH*)iobuf,datalen);
        }
        else if (first4 == MPC_IEA_FIRST4)
        {
            /* Process the IEA. */
            osa_device_cmd(dev,(MPC_IEA*)iobuf,datalen);
        }
        else if (first4 == MPC_END_FIRST4)
        {
            /* Only ever seen during z/OS shutdown */
            if (grp->debugmask)
            {
                PTT_QETH_TRACE( "shut notify", dev->devnum,0,0 );
                mpc_display_description( dev, "Shutdown Notify" );
                MPC_DUMP_DATA( "END", iobuf, datalen, FROM_GUEST );
            }
        }
        else
        {
            /* Display the unrecognised data. */
            mpc_display_description( dev, "Unrecognised Request" );
            if (length >= 256)
                length = 256;
            MPC_DUMP_DATA( "???", iobuf, length, FROM_GUEST );
        }

        /* Return normal status */
        *unitstat = CSW_CE | CSW_DE;
        *residual = 0;
        *more = 0;

        /* Finished with the contentstring. */
        dev->dev_data = NULL;

        break;
    }


    case 0x02:
    /*---------------------------------------------------------------*/
    /* READ                                                          */
    /*---------------------------------------------------------------*/
    {
    OSA_BHR *bhr;
    BYTE    *iodata;
    U32      datalen, length;
    U32      first4;
        /*
        ** Our purpose is to satisfy the program's request to read
        ** an IDX response. If there is a response queued (chained)
        ** then we return our response thereby satisfying the read,
        ** and exit with normal CSW status.
        **
        ** If we don't have an IDX response queued/chained though,
        ** then how we react depends on whether IDX handshaking is
        ** still active or not.
        **
        ** If IDX is not active (i.e. our dev->qdio.idxstate field
        ** is not MPC_IDX_STATE_ACTIVE) we return with a CSW status
        ** of unit check with status modifier (i.e. the read failed
        ** because there were no response buffers queued/chained to
        ** satisfy their read request with).
        **
        ** Otherwise as long as IDX is still active, we simply wait
        ** until a response is eventually queued (chained) so we can
        ** then use it to satisfy their read request with. That is
        ** to say, we will wait forever for a response to be queued
        ** as long as IDX is still active (we only exit when there
        ** is a response to give them or IDX is no longer active).
        */
        PTT_QETH_TRACE( "read entr", 0,0,0 );

        while (dev->qdio.idxstate == MPC_IDX_STATE_ACTIVE)
        {
            /* Remove IDX response buffer from chain. */
            PTT_QETH_TRACE( "b4 rd rmbuf", 0,0,0 );
            bhr = remove_buffer_from_chain( &grp->idx );
            PTT_QETH_TRACE( "af rd rmbuf", bhr,0,0 );

            if (bhr)
            {
                /* Point to response data and get its length. */
                iodata = (BYTE*)bhr + SizeBHR;
                length = datalen = bhr->datalen;

                /* Set the residual length and normal status. */
                if (count >= datalen)
                {
                    *more     = 0;
                    *residual = count - datalen;
                }
                else
                {
                    datalen   = count;
                    *more     = 1;
                    *residual = 0;
                }
                *unitstat = CSW_CE | CSW_DE;

                /* What type of IDX response is this? */
                FETCH_FW( first4, iodata );

                if (first4 == MPC_TH_FIRST4)
                {
                    MPC_TH *th = (MPC_TH*)iodata;

                    /* Set the transmission header sequence number. */
                    STORE_FW( th->seqnum, ++grp->seqnumth );

                    /* Display the response MPC_TH etc., maybe. */
                    DBGUPD( dev, 1, iodata, 0, TO_GUEST, "%s: Response", bhr->content );
                }
                else if (first4 == MPC_IEA_FIRST4)
                {
                    /* Display the response MPC_IEAR, maybe. */
                    DBGUPD( dev, 3, iodata, datalen, TO_GUEST, "%s: Response", bhr->content );
                }
                else if (first4 == MPC_END_FIRST4)
                {
                    /* Only ever seen during z/OS shutdown */
                    if (grp->debugmask)
                    {
                        PTT_QETH_TRACE( "shut ack", 0,0,0 );
                        mpc_display_description( dev, "Shutdown Acknowledge" );
                        MPC_DUMP_DATA( "END", iobuf, length, TO_GUEST );
                    }
                }
                else
                {
                    /* Display the unrecognised data. */
                    mpc_display_description( dev, "Unrecognised Response" );
                    if (length >= 256)
                        length = 256;
                    MPC_DUMP_DATA( "???", iodata, length, TO_GUEST );
                }

                /* Copy IDX response data to i/o buffer. */
                memcpy( iobuf, iodata, datalen );

                /* Free IDX response buffer. Read is complete. */
                free( bhr->content );
                free( bhr );
                break; /*while*/
            }

            /* There are no IDX response buffers chained. */
            if(dev->qdio.idxstate != MPC_IDX_STATE_ACTIVE)
            {
                /* Return unit check with status modifier. */
                dev->sense[0] = 0;
                *unitstat = CSW_CE | CSW_DE | CSW_UC | CSW_SM;
                break; /*while*/
            }

            /* Wait for an IDX response buffer to be chained. */
            obtain_lock(&grp->qlock);
            PTT_QETH_TRACE( "read wait", 0,0,0 );
            wait_condition( &grp->qrcond, &grp->qlock );
            release_lock(&grp->qlock);

        } /* end while (dev->qdio.idxstate == MPC_IDX_STATE_ACTIVE) */

        if (dev->qdio.idxstate == MPC_IDX_STATE_HALTING)
        {
            obtain_lock( &grp->qlock );
            PTT_QETH_TRACE( "read hlt ack", 0,0,0 );
            dev->qdio.idxstate = MPC_IDX_STATE_INACTIVE;
            signal_condition( &grp->qrcond );
            release_lock( &grp->qlock );
        }

        PTT_QETH_TRACE( "read exit", 0,0,0 );

        break; /*switch*/

    } /* end case 0x02: READ */


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
        /* We currently do not support emulated 3088 CTCA mode */
        dev->sense[0] = SENSE_CR;
        *unitstat = CSW_CE | CSW_DE | CSW_UC;
        break;
    }

    case 0x04:
    /*---------------------------------------------------------------*/
    /* SENSE                                                         */
    /*---------------------------------------------------------------*/

        /* Calculate residual byte count */
        num = (count < dev->numsense) ? count : dev->numsense;
        *residual = count - num;
//???   if (count < dev->numsense) *more = 1;

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
//???   if (count < dev->numdevid) *more = 1;

        /* Copy device identifier bytes to channel I/O buffer */
        memcpy (iobuf, dev->devid, num);

        /* Return unit status */
        *unitstat = CSW_CE | CSW_DE;

        /* Display formatted Sense Id information, maybe */
        if (grp->debugmask & DBGQETHUPDOWN)
        {
            char buf[1024];
            // HHC03995 "%1d:%04X %s: %s:\n%s"
            WRMSG(HHC03995, "D", LCSS_DEVNUM,
                dev->typname, "SID", FormatSID( iobuf, num, buf, sizeof( buf )));
//          MPC_DUMP_DATA( "SID", iobuf, num, ' ' );
        }
        break;


    case OSA_RCD:
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
        if (grp->debugmask & DBGQETHUPDOWN)
        {
            char buf[1024];
            // HHC03995 "%1d:%04X %s: %s:\n%s"
            WRMSG(HHC03995, "D", LCSS_DEVNUM,
                dev->typname, "RCD", FormatRCD( iobuf, num, buf, sizeof( buf )));
//          MPC_DUMP_DATA( "RCD", iobuf, num, ' ' );
        }
        break;
    }


    case OSA_SII:
    /*---------------------------------------------------------------*/
    /* SET INTERFACE IDENTIFIER                                      */
    /*---------------------------------------------------------------*/
    {
        // ZZ FIXME: PROGRAMMING NOTE: z/VM appears to always reject
        // this command so for the time being so will we. Since we're
        // not 100% sure about this however (we may later determine
        // that we actually need it), we shall simply disable it via
        // temporary #if statements. Once we know for certain we can
        // then remove the #ifs and keep the only code we need.

#if 0   // ZZ FIXME: should we be doing this?

        /* z/VM 5.3 always rejects this command so we will too */
        dev->sense[0] = SENSE_CR;
        *unitstat = CSW_CE | CSW_DE | CSW_UC;
        break;

#else   // ZZ FIXME: or should we be doing this instead?

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

        /* Display various information, maybe */
        if (grp->debugmask & DBGQETHUPDOWN) {
            MPC_DUMP_DATA( "SII", iobuf, num, ' ' );
        }

        break;

#endif // ZZ FIXME
    }


    case OSA_RNI:
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
        if (len > (int)sizeof(ND))
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
        if (grp->debugmask & DBGQETHUPDOWN)
        {
            char buf[1024];
            // HHC03995 "%1d:%04X %s: %s:\n%s"
            WRMSG(HHC03995, "D", LCSS_DEVNUM,
                dev->typname, "RNI", FormatRNI( iobuf, num, buf, sizeof( buf )));
//          MPC_DUMP_DATA( "RNI", iobuf, num, ' ' );
        }
        break;
    }


    case OSA_EQ:
    /*---------------------------------------------------------------*/
    /* ESTABLISH QUEUES                                              */
    /*---------------------------------------------------------------*/
    {
        QDIO_QDR *qdr = (QDIO_QDR*)iobuf;
        QDIO_QDES0 *qdes;
        int qdr_len;
        int accerr = 0;
        int i;

        dev->qdio.i_qcnt = qdr->iqdcnt < QDIO_MAXQ ? qdr->iqdcnt : QDIO_MAXQ;
        dev->qdio.o_qcnt = qdr->oqdcnt < QDIO_MAXQ ? qdr->oqdcnt : QDIO_MAXQ;

        DBGTRC( dev, "Establish Queues: Entry");
        PTT_QETH_TRACE( "eq entry", dev->qdio.i_qcnt, dev->qdio.o_qcnt, 0 );

        /* Calculate length of Queue Descriptor Record */
        qdr_len = 0x40;                            /* From start to descriptors */
        qdr_len += (qdr->iqdcnt*(qdr->iqdsz<<2));  /* The input queue descriptors */
        qdr_len += (qdr->oqdcnt*(qdr->oqdsz<<2));  /* The output queue descriptors */

        /* Display Queue Descriptor Record, maybe */
        if (grp->debugmask) {
            net_data_trace( dev, (BYTE*)qdr, qdr_len, ' ', 'D', "QDIO_QDR", 0 );
        }

        /* Check the Queue Information Block storage */
        FETCH_DW(dev->qdio.qiba,qdr->qiba);
        dev->qdio.qibk = qdr->qkey & 0xF0;

        if(!(accerr = STORCHK(dev->qdio.qiba,sizeof(QDIO_QIB)-1,dev->qdio.qibk,STORKEY_CHANGE,dev)))
        {
        QDIO_QIB *qib = (QDIO_QIB*)(dev->mainstor + dev->qdio.qiba);
            qib->ac |= QIB_AC_PCI; // Incidate PCI on output is supported
#if defined( _FEATURE_QEBSM )
            if (FACILITY_ENABLED_DEV( HERC_QEBSM ))
                qib->rflags |= QIB_RFLAGS_QEBSM;
#endif
        }

        /* Check input Queue Descriptor Entry storage */
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

        /* Check output Queue Descriptor Entry storage */
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

        /* Initialize All Queues */
        qeth_init_queues(dev);

        /* Calculate residual byte count */
        num = (count < sizeof(QDIO_QDR)) ? count : sizeof(QDIO_QDR);
        *residual = count - num;
        if (count < sizeof(QDIO_QDR)) *more = 1;

        if(!accerr)
        {
            /* Return unit status */
            *unitstat = CSW_CE | CSW_DE;
            PTT_QETH_TRACE( "eq exit", *unitstat, 0, 0 );
        }
        else
        {
            /* Command reject on invalid or inaccessible storage addresses */
            dev->sense[0] = SENSE_CR;
            *unitstat = CSW_CE | CSW_DE | CSW_UC;
            PTT_QETH_TRACE( "*eq exit", *unitstat, dev->sense[0], accerr );
        }

        DBGTRC( dev, "Establish Queues: Exit");
        break;
    }


    case OSA_AQ:
    /*---------------------------------------------------------------*/
    /* ACTIVATE QUEUES                                               */
    /*---------------------------------------------------------------*/
    {
    fd_set readset;                         /* select read set       */
    struct timeval tv;                      /* select polling        */
    int fd;                                 /* select fd             */
    int rc=0;                               /* select rc (0=timeout) */
    BYTE sig;                               /* thread pipe signal    */

        /*
        ** PROGRAMMING NOTE: we use a relatively short timeout value
        ** for our select so that we can react fairly quickly to the
        ** guest readying (priming) additional output buffers in its
        ** existing Output Queue(s) because a SIGA-w is not required.
        */
        dev->scsw.flag2 |= SCSW2_Q;         /* Indicate QDIO active  */
        dev->qtype = QTYPE_DATA;            /* Identify ourselves    */

        DBGTRC( dev, "Activate Queues: Entry iqm=%8.8x oqm=%8.8x",dev->qdio.i_qmask, dev->qdio.o_qmask);
        PTT_QETH_TRACE( "actq entr", 0,0,0 );

        /* Loop until halt signal is received via notification pipe */
        while (1)
        {
            /* Prepare to wait for additional packets or pipe signal */
            FD_ZERO( &readset );
            FD_SET( grp->ppfd[0], &readset );
            FD_SET( grp->ttfd,    &readset );
            fd = max( grp->ppfd[0], grp->ttfd );
            tv.tv_sec  = 0;
            tv.tv_usec = OSA_TIMEOUTUS;         /* Select timeout usecs  */

            /* Wait (but only very briefly) for more work to arrive */
            rc = qeth_select( fd+1, &readset, &tv );

            /* Read pipe signal if one was sent */
            if (unlikely( rc && FD_ISSET( grp->ppfd[0], &readset )))
            {
                sig = QDSIG_RESET;
                VERIFY( qeth_read_pipe( grp->ppfd[0], &sig ) == 1);
                if (QDSIG_HALT == sig || grp->debugmask & DBGQETHQUEUES)
                    DBGTRC( dev, "Activate Queues: %s received", qsig2str( sig ));

                /* Exit immediately when requested to do so */
                if (QDSIG_HALT == sig)
                    break;

                switch (sig)
                {
                case QDSIG_READ:
                    grp->rdpack = 0;
                    break;
                case QDSIG_RDMULT:
                    grp->rdpack = 1;
                    break;
                case QDSIG_WRIT:
                    grp->wrpack = 0;
                    break;
                case QDSIG_WRMULT:
                    grp->wrpack = 1;
                    break;
                case QDSIG_WAKEUP:
                    break;
                default:
                    ASSERT(0);  /* (should NEVER occur) */
                }
            }

            /* Check if any new packets have arrived */
            if ((rc && FD_ISSET( grp->ttfd, &readset )) || grp->l3r.firstbhr)
            {
                /* Process packets if Queue is available */
                if (likely( dev->qdio.i_qmask ))
                {
                    process_input_queues( dev );

                    /* Present "input available" interrupt if needed */
                    if (grp->iqPCI)
                    {
                        PTT_QETH_TRACE( "actq iqPCI", 0,0,0 );
                        grp->iqPCI = FALSE;
                        raise_adapter_interrupt( dev );
                    }
                }
                else /* (no I/P queues? VERY unlikely!) */
                {
                    if (QRC_SUCCESS == read_packet( dev, grp ))
                    {
                        dev->qdio.dropcnt++;
                        PTT_QETH_TRACE( "*actq drop", dev->qdio.i_qmask, 0, 0 );
                        if (grp->debugmask & DBGQETHDROP)
                        {
                            // "%1d:%04X %s: %s: Input dropped: %s"
                            WRMSG( HHC03810, "W", LCSS_DEVNUM,
                                dev->typname, grp->ttifname, "No available queues" );
                        }
                    }
                }
            }

            /* ALWAYS process all Output Queues each time regardless of
               whether the guest has recently executed a SIGA-w or not
               since most guests expect OSA devices to behave that way.
               (SIGA-w are NOT required to cause processing O/P queues)
            */
            if (likely( dev->qdio.o_qmask ))
            {
                process_output_queues(dev);

                /* Present "output processed" interrupt if needed */
                if (grp->oqPCI)
                {
                    PTT_QETH_TRACE( "actq oqPCI", 0,0,0 );
                    grp->oqPCI = FALSE;
                    raise_adapter_interrupt( dev );
                }
            }
        }
        PTT_QETH_TRACE( "actq break", dev->devnum, 0,0 );

        /* Acknowledge halt signal (how else could we reach here?) */
        if (sig == QDSIG_HALT)
        {
            obtain_lock( &grp->qlock );
            {
                dev->scsw.flag2 &= ~SCSW2_Q;
                signal_condition( &grp->qdcond );
            }
            release_lock( &grp->qlock );
        }

        DBGTRC( dev, "Activate Queues: Exit");
        PTT_QETH_TRACE( "actq exit", 0,0,0 );

        /* Return unit status */
        *unitstat = CSW_CE | CSW_DE;
        break;
    }


    default:
    /*---------------------------------------------------------------*/
    /* INVALID OPERATION                                             */
    /*---------------------------------------------------------------*/
        DBGTRC(dev, "Unknown CCW opcode 0x%02X)",code);
        /* Set command reject sense byte, and unit check status */
        dev->sense[0] = SENSE_CR;
        *unitstat = CSW_CE | CSW_DE | CSW_UC;

    } /* end switch(code) */

    /* Display various information, maybe */
    if (grp->debugmask & DBGQETHCCW)
    {
        // "%1d:%04X %s: Status %02X: Residual %08X: More %02X"
        WRMSG( HHC03993, "D", LCSS_DEVNUM,
            dev->typname, *unitstat, *residual, *more );
    }

} /* end function qeth_execute_ccw */


/*-------------------------------------------------------------------*/
/* Initialize all Input -AND- Output Queue variables                 */
/*-------------------------------------------------------------------*/
static void qeth_init_queues(DEVBLK *dev)
{
    qeth_init_queue(dev,0);     /* Initialize ALL Input  Queues      */
    qeth_init_queue(dev,1);     /* Initialize ALL Output Queues      */
}
/*-------------------------------------------------------------------*/
/* Initialize all Input -OR- Output Queue variables                  */
/*-------------------------------------------------------------------*/
static void qeth_init_queue(DEVBLK *dev, int output)
{
int i;
U32 qmask;

    PTT_QETH_TRACE( "initq entry", dev->qdio.i_qcnt, dev->qdio.o_qcnt, output );

    if (output)
    {
        dev->qdio.o_qpos = 0;
        for (i=0; i < QDIO_MAXQ; i++)
            dev->qdio.o_bpos[i] = 0;
        qmask = dev->qdio.o_qmask = ~(0xffffffff >> dev->qdio.o_qcnt);
    }
    else /* input */
    {
        dev->qdio.i_qpos = 0;
        for (i=0; i < QDIO_MAXQ; i++)
            dev->qdio.i_bpos[i] = 0;
        qmask = dev->qdio.i_qmask = ~(0xffffffff >> dev->qdio.i_qcnt);
    }

    DBGTRC(dev, "Initialize %s Queue: qmask(0x%08X)",
        output ? "Output" : "Input", qmask );

    PTT_QETH_TRACE( "initq exit", dev->qdio.i_qcnt, dev->qdio.o_qcnt, qmask );
}


/*-------------------------------------------------------------------*/
/* Signal Adapter Initiate Input                                     */
/*-------------------------------------------------------------------*/
static int qeth_initiate_input(DEVBLK *dev, U32 qmask)
{
OSA_GRP *grp = (OSA_GRP*)dev->group->grp_data;
int noselrd, rc = 0;

    if (grp->debugmask & DBGQETHSIGA)
        DBGTRC( dev, "SIGA-r qmask(%8.8x)", qmask );

    PTT_QETH_TRACE( "b4 SIGA-r", qmask, dev->qdio.i_qmask, dev->devnum );

    /* Return CC1 if the device is not QDIO active */
    if(!(dev->scsw.flag2 & SCSW2_Q))
    {
        DBGTRC( dev, "SIGA-r: ERROR: QDIO not active" );
        rc = 1;
    }
    else
    {
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

        /* Send signal to ACTIVATE QUEUES device thread loop */
        if(noselrd && dev->qdio.i_qmask)
        {
            BYTE sig = QDSIG_READ;
            if (grp->debugmask & DBGQETHSIGA)
                DBGTRC( dev, "SIGA-r: sending %s", qsig2str( sig ));
            VERIFY( qeth_write_pipe( grp->ppfd[1], &sig ) == 1);
        }
    }

    PTT_QETH_TRACE( "af SIGA-r", qmask, dev->qdio.i_qmask, dev->devnum );
    return rc;
}


/*-------------------------------------------------------------------*/
/* Signal Adapter Initiate Output/Multiple helper function           */
/*-------------------------------------------------------------------*/
static int qeth_do_initiate_output( DEVBLK *dev, U32 qmask, BYTE sig )
{
OSA_GRP *grp = (OSA_GRP*)dev->group->grp_data;

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

    /* Send signal to ACTIVATE QUEUES device thread loop */
    if(dev->qdio.o_qmask)
    {
        if (grp->debugmask & DBGQETHSIGA)
            DBGTRC( dev, "SIGA-o: sending %s", qsig2str( sig ));
        VERIFY( qeth_write_pipe( grp->ppfd[1], &sig ) == 1);
    }

    return 0;
}


/*-------------------------------------------------------------------*/
/* Signal Adapter Initiate Output                                    */
/*-------------------------------------------------------------------*/
static int qeth_initiate_output( DEVBLK *dev, U32 qmask )
{
    int rc;
    OSA_GRP *grp = (OSA_GRP*)dev->group->grp_data;
    if (grp->debugmask & DBGQETHSIGA)
        DBGTRC( dev, "SIGA-w qmask(%8.8x)", qmask );
    PTT_QETH_TRACE( "b4 SIGA-w", qmask, dev->qdio.o_qmask, dev->devnum );

    if ((rc = qeth_do_initiate_output( dev, qmask, QDSIG_WRIT )) == 1)
        DBGTRC( dev, "SIGA-w: ERROR: QDIO not active");

    PTT_QETH_TRACE( "af SIGA-w", qmask, dev->qdio.o_qmask, dev->devnum );
    return rc;
}


/*-------------------------------------------------------------------*/
/* Signal Adapter Initiate Output Multiple                           */
/*-------------------------------------------------------------------*/
static int qeth_initiate_output_mult( DEVBLK *dev, U32 qmask )
{
    int rc;
    OSA_GRP *grp = (OSA_GRP*)dev->group->grp_data;
    if (grp->debugmask & DBGQETHSIGA)
        DBGTRC( dev, "SIGA-m qmask(%8.8x)", qmask );
    PTT_QETH_TRACE( "b4 SIGA-m", qmask, dev->qdio.o_qmask, dev->devnum );

    if ((rc = qeth_do_initiate_output( dev, qmask, QDSIG_WRMULT )) == 1)
        DBGTRC( dev, "SIGA-m: ERROR: QDIO not active");

    PTT_QETH_TRACE( "af SIGA-m", qmask, dev->qdio.o_qmask, dev->devnum );
    return rc;
}


/*-------------------------------------------------------------------*/
/* Signal Adapter Sync                                               */
/*-------------------------------------------------------------------*/
static int qeth_do_sync( DEVBLK *dev, U32 oqmask, U32 iqmask )
{
    int rc = 0;

    /* Return CC1 if the device is not QDIO active */
    if(!(dev->scsw.flag2 & SCSW2_Q))
        return 1;

    /* Validate Input and Output Masks */
    oqmask &= ~(0xffffffff >> dev->qdio.o_qcnt);
    iqmask &= ~(0xffffffff >> dev->qdio.i_qcnt);

    DBGTRC( dev, "SIGA-s dev(%4.4x) oqmask(%8.8x), iqmask(%8.8x)",
        dev->devnum, oqmask, iqmask );
    PTT_QETH_TRACE( "b4 SIGA-s", oqmask, iqmask, dev->devnum );

    /* Return CC1 if the device is not QDIO active */
    if(!(dev->scsw.flag2 & SCSW2_Q))
    {
        DBGTRC( dev, "SIGA-s: ERROR: QDIO not active");
        rc = 1;
    }
    else
    {
        /*
        ** The Synchronize Function updates either the adapter's SLSB
        ** or program's SLSB with the other's State Block information.
        ** Buffer states in the program's SLSB indicating control unit
        ** ownership will cause the adapter's SLSB to be updated for
        ** that buffer. Buffers in the adapter's SLSB indicating owner-
        ** ship by the program will cause the program's SLSB to be up-
        ** dated.
        */
        /* FIXME Code missing SIGA-Sync functionality */
//      FIXME("Code missing SIGA-Sync functionality");
    }

    PTT_QETH_TRACE( "af SIGA-s", oqmask, iqmask, dev->devnum );
    return rc;
}


/*-------------------------------------------------------------------*/
/* Process the CM_ENABLE request from the guest.                     */
/*-------------------------------------------------------------------*/
static OSA_BHR* process_cm_enable( DEVBLK* dev, MPC_TH* req_th, MPC_RRH* req_rrh, MPC_PUK* req_puk )
{
OSA_GRP *grp = (OSA_GRP*)dev->group->grp_data;

MPC_PUS *req_pus_01;
MPC_PUS *req_pus_02;

OSA_BHR *rsp_bhr;
MPC_TH  *rsp_th;
MPC_RRH *rsp_rrh;
MPC_PH  *rsp_ph;
MPC_PUK *rsp_puk;
MPC_PUS *rsp_pus_01;
MPC_PUS *rsp_pus_02;

U32 uLength1;
U32 uLength2;
U32 uLength3;
U16 uLength4;

    UNREFERENCED(req_rrh);

    /* Display the request MPC_TH etc., maybe. */
    DBGUPD( dev, 1, req_th, 0, FROM_GUEST, "%s: Request", dev->dev_data );

    /* Point to the expected MPC_PUS and check they are present. */
    req_pus_01 = mpc_point_pus( dev, req_puk, PUS_TYPE_01 );
    req_pus_02 = mpc_point_pus( dev, req_puk, PUS_TYPE_02 );
    if( !req_pus_01 || !req_pus_02 )
    {
        /* FIXME Expected pus not present, error message please. */
        DBGTRC(dev, "process_cm_enable: Expected pus not present");
        return NULL;
    }

    /* Copy the guests CM Filter token from request PUS_TYPE_01. */
    memcpy( grp->gtcmfilt, req_pus_01->vc.pus_01.token, MPC_TOKEN_LENGTH );

    // Fix-up various lengths
    uLength4 = SIZE_PUS_01 +                     // first MPC_PUS
               SIZE_PUS_02_C;                    // second MPC_PUS
    uLength3 = SIZE_PUK + uLength4;              // the MPC_PUK and the MPC_PUSs (the data)
    uLength2 = SIZE_TH + SIZE_RRH_1 + SIZE_PH;   // the MPC_TH/MPC_RRH/MPC_PH
    uLength1 = uLength2 + uLength3;              // the MPC_TH/MPC_RRH/MPC_PH and data

    // Allocate a buffer in which the response will be build.
    rsp_bhr = alloc_buffer( dev, uLength1+10 );
    if (!rsp_bhr)
        return NULL;
    rsp_bhr->content = strdup( dev->dev_data );
    rsp_bhr->datalen = uLength1;

    // Fix-up various pointers
    rsp_th = (MPC_TH*)((BYTE*)rsp_bhr + SizeBHR);
    rsp_rrh = (MPC_RRH*)((BYTE*)rsp_th + SIZE_TH);
    rsp_ph = (MPC_PH*)((BYTE*)rsp_rrh + SIZE_RRH_1);
    rsp_puk = (MPC_PUK*)((BYTE*)rsp_ph + SIZE_PH);
    rsp_pus_01 = (MPC_PUS*)((BYTE*)rsp_puk + SIZE_PUK);
    rsp_pus_02 = (MPC_PUS*)((BYTE*)rsp_pus_01 + SIZE_PUS_01);

    // Prepare MPC_TH
    STORE_FW( rsp_th->first4, MPC_TH_FIRST4 );
    STORE_FW( rsp_th->offrrh, SIZE_TH );
    STORE_FW( rsp_th->length, uLength1 );
    STORE_HW( rsp_th->unknown10, 0x0FFC );            /* !!! */
    STORE_HW( rsp_th->numrrh, 1 );

    // Prepare MPC_RRH
    rsp_rrh->type = RRH_TYPE_CM;
    rsp_rrh->proto = PROTOCOL_UNKNOWN;
    STORE_HW( rsp_rrh->numph, 1 );
//  STORE_FW( rsp_rrh->seqnum, ++grp->seqnumis );
    STORE_HW( rsp_rrh->offph, SIZE_RRH_1 );
    STORE_HW( rsp_rrh->lenfida, (U16)uLength3 );
    STORE_F3( rsp_rrh->lenalda, uLength3 );
    rsp_rrh->tokenx5 = MPC_TOKEN_X5;
    memcpy( rsp_rrh->token, grp->gtissue, MPC_TOKEN_LENGTH );

    // Prepare MPC_PH
    rsp_ph->locdata = PH_LOC_1;
    STORE_F3( rsp_ph->lendata, uLength3 );
    STORE_FW( rsp_ph->offdata, uLength2 );

    // Prepare MPC_PUK
    STORE_HW( rsp_puk->length, SIZE_PUK );
    rsp_puk->what = PUK_WHAT_41;
    rsp_puk->type = PUK_TYPE_ENABLE;
    STORE_HW( rsp_puk->lenpus, uLength4 );

    // Prepare first MPC_PUS
    STORE_HW( rsp_pus_01->length, SIZE_PUS_01 );
    rsp_pus_01->what = PUS_WHAT_04;
    rsp_pus_01->type = PUS_TYPE_01;
    rsp_pus_01->vc.pus_01.proto = PROTOCOL_UNKNOWN;
    rsp_pus_01->vc.pus_01.unknown05 = 0x04;           /* !!! */
    rsp_pus_01->vc.pus_01.tokenx5 = MPC_TOKEN_X5;
    STORE_FW( rsp_pus_01->vc.pus_01.token, QTOKEN2 );

    // Prepare second MPC_PUS (which will contain 8-bytes of nulls)
    STORE_HW( rsp_pus_02->length, SIZE_PUS_02_C );
    rsp_pus_02->what = PUS_WHAT_04;
    rsp_pus_02->type = PUS_TYPE_02;

    return rsp_bhr;
}

/*-------------------------------------------------------------------*/
/* Process the CM_SETUP request from the guest.                      */
/*-------------------------------------------------------------------*/
static OSA_BHR* process_cm_setup( DEVBLK* dev, MPC_TH* req_th, MPC_RRH* req_rrh, MPC_PUK* req_puk )
{
OSA_GRP *grp = (OSA_GRP*)dev->group->grp_data;

MPC_PUS *req_pus_04;
MPC_PUS *req_pus_06;

OSA_BHR *rsp_bhr;
MPC_TH  *rsp_th;
MPC_RRH *rsp_rrh;
MPC_PH  *rsp_ph;
MPC_PUK *rsp_puk;
MPC_PUS *rsp_pus_04;
MPC_PUS *rsp_pus_08;
MPC_PUS *rsp_pus_07;

U32 uLength1;
U32 uLength2;
U32 uLength3;
U16 uLength4;

    UNREFERENCED(req_rrh);

    /* Display the request MPC_TH etc., maybe. */
    DBGUPD( dev, 1, req_th, 0, FROM_GUEST, "%s: Request", dev->dev_data );

    /* Point to the expected MPC_PUS and check they are present. */
    req_pus_04 = mpc_point_pus( dev, req_puk, PUS_TYPE_04 );
    req_pus_06 = mpc_point_pus( dev, req_puk, PUS_TYPE_06 );
    if( !req_pus_04 || !req_pus_06 )
    {
        /* FIXME Expected pus not present, error message please. */
        DBGTRC(dev, "process_cm_setup: Expected pus not present");
        return NULL;
    }

    /* Copy the guests CM Connection token from request PUS_TYPE_04. */
    memcpy( grp->gtcmconn, req_pus_04->vc.pus_04.token, MPC_TOKEN_LENGTH );

    // Fix-up various lengths
    uLength4 = SIZE_PUS_04 +                     // first MPC_PUS
               SIZE_PUS_08 +                     // second MPC_PUS
               SIZE_PUS_07;                      // third MPC_PUS
    uLength3 = SIZE_PUK + uLength4;              // the MPC_PUK and the MPC_PUSs (the data)
    uLength2 = SIZE_TH + SIZE_RRH_1 + SIZE_PH;   // the MPC_TH/MPC_RRH/MPC_PH
    uLength1 = uLength2 + uLength3;              // the MPC_TH/MPC_RRH/MPC_PH and data

    // Allocate a buffer in which the response will be build.
    rsp_bhr = alloc_buffer( dev, uLength1+10 );
    if (!rsp_bhr)
        return NULL;
    rsp_bhr->content = strdup( dev->dev_data );
    rsp_bhr->datalen = uLength1;

    // Fix-up various pointers
    rsp_th = (MPC_TH*)((BYTE*)rsp_bhr + SizeBHR);
    rsp_rrh = (MPC_RRH*)((BYTE*)rsp_th + SIZE_TH);
    rsp_ph = (MPC_PH*)((BYTE*)rsp_rrh + SIZE_RRH_1);
    rsp_puk = (MPC_PUK*)((BYTE*)rsp_ph + SIZE_PH);
    rsp_pus_04 = (MPC_PUS*)((BYTE*)rsp_puk + SIZE_PUK);
    rsp_pus_08 = (MPC_PUS*)((BYTE*)rsp_pus_04 + SIZE_PUS_04);
    rsp_pus_07 = (MPC_PUS*)((BYTE*)rsp_pus_08 + SIZE_PUS_08);

    // Prepare MPC_TH
    STORE_FW( rsp_th->first4, MPC_TH_FIRST4 );
    STORE_FW( rsp_th->offrrh, SIZE_TH );
    STORE_FW( rsp_th->length, uLength1 );
    STORE_HW( rsp_th->unknown10, 0x0FFC );            /* !!! */
    STORE_HW( rsp_th->numrrh, 1 );

    // Prepare MPC_RRH
    rsp_rrh->type = RRH_TYPE_CM;
    rsp_rrh->proto = PROTOCOL_UNKNOWN;
    STORE_HW( rsp_rrh->numph, 1 );
//  STORE_FW( rsp_rrh->seqnum, ++grp->seqnumis );
    STORE_HW( rsp_rrh->offph, SIZE_RRH_1 );
    STORE_HW( rsp_rrh->lenfida, (U16)uLength3 );
    STORE_F3( rsp_rrh->lenalda, uLength3 );
    rsp_rrh->tokenx5 = MPC_TOKEN_X5;
    memcpy( rsp_rrh->token, grp->gtissue, MPC_TOKEN_LENGTH );

    // Prepare MPC_PH
    rsp_ph->locdata = PH_LOC_1;
    STORE_F3( rsp_ph->lendata, uLength3 );
    STORE_FW( rsp_ph->offdata, uLength2 );

    // Prepare MPC_PUK
    STORE_HW( rsp_puk->length, SIZE_PUK );
    rsp_puk->what = PUK_WHAT_41;
    rsp_puk->type = PUK_TYPE_CONFIRM;
    STORE_HW( rsp_puk->lenpus, uLength4 );

    // Prepare first MPC_PUS
    STORE_HW( rsp_pus_04->length, SIZE_PUS_04 );
    rsp_pus_04->what = PUS_WHAT_04;
    rsp_pus_04->type = PUS_TYPE_04;
    rsp_pus_04->vc.pus_04.tokenx5 = MPC_TOKEN_X5;
    memcpy( rsp_pus_04->vc.pus_04.token, grp->gtcmconn, MPC_TOKEN_LENGTH );

    // Prepare second MPC_PUS
    STORE_HW( rsp_pus_08->length, SIZE_PUS_08 );
    rsp_pus_08->what = PUS_WHAT_04;
    rsp_pus_08->type = PUS_TYPE_08;
    rsp_pus_08->vc.pus_08.tokenx5 = MPC_TOKEN_X5;
    STORE_FW( rsp_pus_08->vc.pus_08.token, QTOKEN3 );

    // Prepare third MPC_PUS
    STORE_HW( rsp_pus_07->length, SIZE_PUS_07 );
    rsp_pus_07->what = PUS_WHAT_04;
    rsp_pus_07->type = PUS_TYPE_07;
//  memcpy( rsp_pus_07->vc.pus_07.unknown04+0, req_pus_06->vc.pus_06.unknown04, 2 );
    STORE_HW( rsp_pus_07->vc.pus_07.unknown04+0, 0x4000 );
    STORE_HW( rsp_pus_07->vc.pus_07.unknown04+2, 0x0000 );

    return rsp_bhr;
}

/*-------------------------------------------------------------------*/
/* Process the CM_TAKEDOWN request from the guest.                   */
/*-------------------------------------------------------------------*/
static OSA_BHR* process_cm_takedown( DEVBLK* dev, MPC_TH* req_th, MPC_RRH* req_rrh, MPC_PUK* req_puk )
{
OSA_GRP *grp = (OSA_GRP*)dev->group->grp_data;

    UNREFERENCED(grp);
    UNREFERENCED(req_rrh);
    UNREFERENCED(req_puk);

    /* Display the request MPC_TH etc., maybe. */
    DBGUPD( dev, 1, req_th, 0, FROM_GUEST, "%s: Request", dev->dev_data );

    /* There will be no response. */
    return NULL;
}

/*-------------------------------------------------------------------*/
/* Process the CM_DISABLE request from the guest.                    */
/*-------------------------------------------------------------------*/
static OSA_BHR* process_cm_disable( DEVBLK* dev, MPC_TH* req_th, MPC_RRH* req_rrh, MPC_PUK* req_puk )
{
OSA_GRP *grp = (OSA_GRP*)dev->group->grp_data;

    UNREFERENCED(grp);
    UNREFERENCED(req_rrh);
    UNREFERENCED(req_puk);

    /* Display the request MPC_TH etc., maybe. */
    DBGUPD( dev, 1, req_th, 0, FROM_GUEST, "%s: Request", dev->dev_data );

    /* There will be no response. */
    return NULL;
}


/*-------------------------------------------------------------------*/
/* Process the ULP_ENABLE request from the guest to extract values.  */
/*-------------------------------------------------------------------*/
/* There is one ULP_ENABLE request, irrespective of the number of    */
/* data paths, which indicates that all of the data paths will be    */
/* using layer2 or layer3. A ULP_ENABLE response will be returned.   */
static int process_ulp_enable_extract( DEVBLK* dev, MPC_TH* req_th, MPC_RRH* req_rrh, MPC_PUK* req_puk )
{
OSA_GRP *grp = (OSA_GRP*)dev->group->grp_data;

MPC_PUS *req_pus_01;
MPC_PUS *req_pus_0A;

    UNREFERENCED(req_rrh);

    /* Display the request MPC_TH etc., maybe. */
    DBGUPD( dev, 1, req_th, 0, FROM_GUEST, "%s: Request", dev->dev_data );

    /* Point to the expected MPC_PUS and check they are present. */
    req_pus_01 = mpc_point_pus( dev, req_puk, PUS_TYPE_01 );
    req_pus_0A = mpc_point_pus( dev, req_puk, PUS_TYPE_0A );
    if( !req_pus_01 || !req_pus_0A )
    {
        /* FIXME Expected pus not present, error message please. */
        DBGTRC(dev, "process_ulp_enable_extract: Expected pus not present");
        return -1;
    }

    /* Remember whether we are using layer2 or layer3. */
    grp->l3 = (req_pus_01->vc.pus_01.proto == PROTOCOL_LAYER3);

    // HHC03800 "%1d:%04X %s: Adapter mode set to %s"
    WRMSG( HHC03800, "I", LCSS_DEVNUM, dev->typname, grp->l3 ? "Layer 3"
                                                             : "Layer 2" );
    /* Clear any registered IP or MAC addresses. */
    unregister_all_ipv4( grp );
    unregister_all_ipv6( grp );
    unregister_all_mac( grp );

    return 0;
}

/*-------------------------------------------------------------------*/
/* Process the ULP_ENABLE request from the guest.                    */
/*-------------------------------------------------------------------*/
/* There is one ULP_ENABLE request, irrespective of the number of    */
/* data paths, which indicates whether the data paths will be using  */
/* layer2 or layer3. A ULP_ENABLE response is returned.              */
/*-------------------------------------------------------------------*/
static OSA_BHR* process_ulp_enable( DEVBLK* dev, MPC_TH* req_th, MPC_RRH* req_rrh, MPC_PUK* req_puk )
{
OSA_GRP *grp = (OSA_GRP*)dev->group->grp_data;

MPC_PUS *req_pus_01;
MPC_PUS *req_pus_0A;

OSA_BHR *rsp_bhr;
MPC_TH  *rsp_th;
MPC_RRH *rsp_rrh;
MPC_PH  *rsp_ph;
MPC_PUK *rsp_puk;
MPC_PUS *rsp_pus_01;
MPC_PUS *rsp_pus_0A;

U16      len_req_pus_0A;
U16      len_rsp_pus_0A;

U32 uLength1;
U32 uLength2;
U32 uLength3;
U16 uLength4;

    UNREFERENCED(req_th);
    UNREFERENCED(req_rrh);

    /* Point to the expected MPC_PUS and check they are present. */
    req_pus_01 = mpc_point_pus( dev, req_puk, PUS_TYPE_01 );
    req_pus_0A = mpc_point_pus( dev, req_puk, PUS_TYPE_0A );
    if( !req_pus_01 || !req_pus_0A )
    {
        /* FIXME Expected pus not present, error message please. */
        DBGTRC(dev, "process_ulp_enable: Expected pus not present");
        return NULL;
    }

    /* Copy the guests ULP Filter token from request PUS_TYPE_01. */
    memcpy( grp->gtulpfilt, req_pus_01->vc.pus_01.token, MPC_TOKEN_LENGTH );

    /* Determine length of request and response PUS_TYPE_0A. */
    len_rsp_pus_0A = SIZE_PUS_0A_B;
    FETCH_HW( len_req_pus_0A, req_pus_0A->length);
    if( len_req_pus_0A > SIZE_PUS_0A_B )
        len_rsp_pus_0A = len_req_pus_0A;

    // Fix-up various lengths
    uLength4 = SIZE_PUS_01 +                     // first MPC_PUS
               len_rsp_pus_0A;                   // second MPC_PUS
    uLength3 = SIZE_PUK + uLength4;              // the MPC_PUK and the MPC_PUSs (the data)
    uLength2 = SIZE_TH + SIZE_RRH_1 + SIZE_PH;   // the MPC_TH/MPC_RRH/MPC_PH
    uLength1 = uLength2 + uLength3;              // the MPC_TH/MPC_RRH/MPC_PH and data

    // Allocate a buffer in which the response will be build.
    rsp_bhr = alloc_buffer( dev, uLength1+10 );
    if (!rsp_bhr)
        return NULL;
    rsp_bhr->content = strdup( dev->dev_data );
    rsp_bhr->datalen = uLength1;

    // Fix-up various pointers
    rsp_th = (MPC_TH*)((BYTE*)rsp_bhr + SizeBHR);
    rsp_rrh = (MPC_RRH*)((BYTE*)rsp_th + SIZE_TH);
    rsp_ph = (MPC_PH*)((BYTE*)rsp_rrh + SIZE_RRH_1);
    rsp_puk = (MPC_PUK*)((BYTE*)rsp_ph + SIZE_PH);
    rsp_pus_01 = (MPC_PUS*)((BYTE*)rsp_puk + SIZE_PUK);
    rsp_pus_0A = (MPC_PUS*)((BYTE*)rsp_pus_01 + SIZE_PUS_01);

    // Prepare MPC_TH
    STORE_FW( rsp_th->first4, MPC_TH_FIRST4 );
    STORE_FW( rsp_th->offrrh, SIZE_TH );
    STORE_FW( rsp_th->length, uLength1 );
    STORE_HW( rsp_th->unknown10, 0x0FFC );            /* !!! */
    STORE_HW( rsp_th->numrrh, 1 );

    // Prepare MPC_RRH
    rsp_rrh->type = RRH_TYPE_ULP;
//  rsp_rrh->proto = PROTOCOL_UNKNOWN;
    rsp_rrh->proto = PROTOCOL_02;
    STORE_HW( rsp_rrh->numph, 1 );
    STORE_FW( rsp_rrh->seqnum, ++grp->seqnumcm );
    memcpy( rsp_rrh->ackseq, req_rrh->seqnum, 4 );
    STORE_HW( rsp_rrh->offph, SIZE_RRH_1 );
    STORE_HW( rsp_rrh->lenfida, (U16)uLength3 );
    STORE_F3( rsp_rrh->lenalda, uLength3 );
    rsp_rrh->tokenx5 = MPC_TOKEN_X5;
    memcpy( rsp_rrh->token, grp->gtcmconn, MPC_TOKEN_LENGTH );

    // Prepare MPC_PH
    rsp_ph->locdata = PH_LOC_1;
    STORE_F3( rsp_ph->lendata, uLength3 );
    STORE_FW( rsp_ph->offdata, uLength2 );

    // Prepare MPC_PUK
    STORE_HW( rsp_puk->length, SIZE_PUK );
    rsp_puk->what = PUK_WHAT_41;
    rsp_puk->type = PUK_TYPE_ENABLE;
    STORE_HW( rsp_puk->lenpus, uLength4 );

    // Prepare first MPC_PUS
    STORE_HW( rsp_pus_01->length, SIZE_PUS_01 );
    rsp_pus_01->what = PUS_WHAT_04;
    rsp_pus_01->type = PUS_TYPE_01;
    rsp_pus_01->vc.pus_01.proto = req_pus_01->vc.pus_01.proto;
    rsp_pus_01->vc.pus_01.unknown05 = 0x04;           /* !!! */
    rsp_pus_01->vc.pus_01.tokenx5 = MPC_TOKEN_X5;
    STORE_FW( rsp_pus_01->vc.pus_01.token, QTOKEN4 );

    // Prepare second MPC_PUS
    memcpy( rsp_pus_0A, req_pus_0A, len_req_pus_0A );
    STORE_HW( rsp_pus_0A->length, len_rsp_pus_0A );
    STORE_HW( rsp_pus_0A->vc.pus_0A.mtu, grp->uMTU );
    rsp_pus_0A->vc.pus_0A.linktype = QETH_LINK_TYPE_FAST_ETH;

    return rsp_bhr;
}

/*-------------------------------------------------------------------*/
/* Process the ULP_SETUP request from the guest.                     */
/*-------------------------------------------------------------------*/
/* There is one ULP_SETUP request for each of the data paths.        */
/* A ULP_CONFIRM response is returned.                               */
/*-------------------------------------------------------------------*/
static OSA_BHR* process_ulp_setup( DEVBLK* dev, MPC_TH* req_th, MPC_RRH* req_rrh, MPC_PUK* req_puk )
{
OSA_GRP *grp = (OSA_GRP*)dev->group->grp_data;

MPC_PUS *req_pus_04;
//C_PUS *req_pus_05;
MPC_PUS *req_pus_06;
MPC_PUS *req_pus_0B;
U16      len_pus_0B;

OSA_BHR *rsp_bhr;
MPC_TH  *rsp_th;
MPC_RRH *rsp_rrh;
MPC_PH  *rsp_ph;
MPC_PUK *rsp_puk;
MPC_PUS *rsp_pus_04;
MPC_PUS *rsp_pus_08;
MPC_PUS *rsp_pus_07;
MPC_PUS *rsp_pus_0B;

U32 uLength1;
U32 uLength2;
U32 uLength3;
U16 uLength4;

    UNREFERENCED(req_rrh);

    /* Display the request MPC_TH etc., maybe. */
    DBGUPD( dev, 1, req_th, 0, FROM_GUEST, "%s: Request", dev->dev_data );

    /* Point to the expected MPC_PUS and check they are present. */
    req_pus_04 = mpc_point_pus( dev, req_puk, PUS_TYPE_04 );
    req_pus_06 = mpc_point_pus( dev, req_puk, PUS_TYPE_06 );
    req_pus_0B = mpc_point_pus( dev, req_puk, PUS_TYPE_0B );
    if( !req_pus_04 || !req_pus_06 || !req_pus_0B )
    {
        /* FIXME Expected pus not present, error message please. */
        DBGTRC(dev, "process_ulp_setup: Expected pus not present");
        return NULL;
    }
    FETCH_HW( len_pus_0B, req_pus_0B->length);

    /* Copy the guests ULP Connection token from request PUS_TYPE_04. */
    /* FIXME: The ULP Connection token is unique for each data path.  */
    /* FIXME: If multiple data paths are ever supported, the token    */
    /* FIXME: needs to be kept in a data path related block, not grp. */
    memcpy( grp->gtulpconn, req_pus_04->vc.pus_04.token, MPC_TOKEN_LENGTH );

    // Fix-up various lengths
    uLength4 = SIZE_PUS_04 +                     // first MPC_PUS
               SIZE_PUS_08 +                     // second MPC_PUS
               SIZE_PUS_07 +                     // third MPC_PUS
               len_pus_0B;                       // fourth MPC_PUS
    uLength3 = SIZE_PUK + uLength4;              // the MPC_PUK and the MPC_PUSs (the data)
    uLength2 = SIZE_TH + SIZE_RRH_1 + SIZE_PH;   // the MPC_TH/MPC_RRH/MPC_PH
    uLength1 = uLength2 + uLength3;              // the MPC_TH/MPC_RRH/MPC_PH and data

    // Allocate a buffer in which the response will be build.
    rsp_bhr = alloc_buffer( dev, uLength1+10 );
    if (!rsp_bhr)
        return NULL;
    rsp_bhr->content = strdup( dev->dev_data );
    rsp_bhr->datalen = uLength1;

    // Fix-up various pointers
    rsp_th = (MPC_TH*)((BYTE*)rsp_bhr + SizeBHR);
    rsp_rrh = (MPC_RRH*)((BYTE*)rsp_th + SIZE_TH);
    rsp_ph = (MPC_PH*)((BYTE*)rsp_rrh + SIZE_RRH_1);
    rsp_puk = (MPC_PUK*)((BYTE*)rsp_ph + SIZE_PH);
    rsp_pus_04 = (MPC_PUS*)((BYTE*)rsp_puk + SIZE_PUK);
    rsp_pus_08 = (MPC_PUS*)((BYTE*)rsp_pus_04 + SIZE_PUS_04);
    rsp_pus_07 = (MPC_PUS*)((BYTE*)rsp_pus_08 + SIZE_PUS_08);
    rsp_pus_0B = (MPC_PUS*)((BYTE*)rsp_pus_07 + SIZE_PUS_07);

    // Prepare MPC_TH
    STORE_FW( rsp_th->first4, MPC_TH_FIRST4 );
    STORE_FW( rsp_th->offrrh, SIZE_TH );
    STORE_FW( rsp_th->length, uLength1 );
    STORE_HW( rsp_th->unknown10, 0x0FFC );            /* !!! */
    STORE_HW( rsp_th->numrrh, 1 );

    // Prepare MPC_RRH
    rsp_rrh->type = RRH_TYPE_ULP;
//  rsp_rrh->proto = PROTOCOL_UNKNOWN;
    rsp_rrh->proto = PROTOCOL_02;
    STORE_HW( rsp_rrh->numph, 1 );
    STORE_FW( rsp_rrh->seqnum, ++grp->seqnumcm );
    memcpy( rsp_rrh->ackseq, req_rrh->seqnum, 4 );
    STORE_HW( rsp_rrh->offph, SIZE_RRH_1 );
    STORE_HW( rsp_rrh->lenfida, (U16)uLength3 );
    STORE_F3( rsp_rrh->lenalda, uLength3 );
    rsp_rrh->tokenx5 = MPC_TOKEN_X5;
    memcpy( rsp_rrh->token, grp->gtcmconn, MPC_TOKEN_LENGTH );

    // Prepare MPC_PH
    rsp_ph->locdata = PH_LOC_1;
    STORE_F3( rsp_ph->lendata, uLength3 );
    STORE_FW( rsp_ph->offdata, uLength2 );

    // Prepare MPC_PUK
    STORE_HW( rsp_puk->length, SIZE_PUK );
    rsp_puk->what = PUK_WHAT_41;
    rsp_puk->type = PUK_TYPE_CONFIRM;
    STORE_HW( rsp_puk->lenpus, uLength4 );

    // Prepare first MPC_PUS
    STORE_HW( rsp_pus_04->length, SIZE_PUS_04 );
    rsp_pus_04->what = PUS_WHAT_04;
    rsp_pus_04->type = PUS_TYPE_04;
    rsp_pus_04->vc.pus_04.tokenx5 = MPC_TOKEN_X5;
    memcpy( rsp_pus_04->vc.pus_04.token, grp->gtulpconn, MPC_TOKEN_LENGTH );

    // Prepare second MPC_PUS
    /* FIXME: The ULP Connection tokens need to be unique for each data path. */
    /* FIXME: If multiple data paths are ever supported, QTOKEN5 shouldn't    */
    /* FIXME: be used. something unique is required. Something incorporating  */
    /* FIXME: the data paths device address perhaps? It will also need to be  */
    /* FIXME: kept in a data path related block, not grp.                     */
    STORE_HW( rsp_pus_08->length, SIZE_PUS_08 );
    rsp_pus_08->what = PUS_WHAT_04;
    rsp_pus_08->type = PUS_TYPE_08;
    rsp_pus_08->vc.pus_08.tokenx5 = MPC_TOKEN_X5;
    STORE_FW( rsp_pus_08->vc.pus_08.token, QTOKEN5 );

    // Prepare third MPC_PUS
    STORE_HW( rsp_pus_07->length, SIZE_PUS_07 );
    rsp_pus_07->what = PUS_WHAT_04;
    rsp_pus_07->type = PUS_TYPE_07;
    memcpy( rsp_pus_07->vc.pus_07.unknown04+0, req_pus_06->vc.pus_06.unknown04, 2 );
    STORE_HW( rsp_pus_07->vc.pus_07.unknown04+2, 0x0000 );

    // Prepare fourth MPC_PUS
    memcpy( rsp_pus_0B, req_pus_0B, len_pus_0B );

    return rsp_bhr;
}

/*-------------------------------------------------------------------*/
/* Process the ULP_ACTIVE request from the guest.                    */
/*-------------------------------------------------------------------*/
/* There is one ULP_ACTIVE request for each of the data paths.       */
/* A ULP_ACTIVE response is returned.                                */
/*-------------------------------------------------------------------*/
static OSA_BHR* process_dm_act( DEVBLK* dev, MPC_TH* req_th, MPC_RRH* req_rrh, MPC_PUK* req_puk )
{
OSA_GRP *grp = (OSA_GRP*)dev->group->grp_data;

//C_PUS *req_pus_04;

OSA_BHR *rsp_bhr;
MPC_TH  *rsp_th;
MPC_RRH *rsp_rrh;
MPC_PH  *rsp_ph;
MPC_PUK *rsp_puk;
MPC_PUS *rsp_pus_04;

U32 uLength1;
U32 uLength2;
U32 uLength3;
U16 uLength4;

    UNREFERENCED(req_rrh);
    UNREFERENCED(req_puk);

    /* Display the request MPC_TH etc., maybe. */
    DBGUPD( dev, 1, req_th, 0, FROM_GUEST, "%s: Request", dev->dev_data );

    // Fix-up various lengths
    uLength4 = SIZE_PUS_04;                      // first MPC_PUS
    uLength3 = SIZE_PUK + uLength4;              // the MPC_PUK and the MPC_PUSs (the data)
    uLength2 = SIZE_TH + SIZE_RRH_1 + SIZE_PH;   // the MPC_TH/MPC_RRH/MPC_PH
    uLength1 = uLength2 + uLength3;              // the MPC_TH/MPC_RRH/MPC_PH and data

    // Allocate a buffer in which the response will be build.
    rsp_bhr = alloc_buffer( dev, uLength1+10 );
    if (!rsp_bhr)
        return NULL;
    rsp_bhr->content = strdup( dev->dev_data );
    rsp_bhr->datalen = uLength1;

    // Fix-up various pointers
    rsp_th = (MPC_TH*)((BYTE*)rsp_bhr + SizeBHR);
    rsp_rrh = (MPC_RRH*)((BYTE*)rsp_th + SIZE_TH);
    rsp_ph = (MPC_PH*)((BYTE*)rsp_rrh + SIZE_RRH_1);
    rsp_puk = (MPC_PUK*)((BYTE*)rsp_ph + SIZE_PH);
    rsp_pus_04 = (MPC_PUS*)((BYTE*)rsp_puk + SIZE_PUK);

    // Prepare MPC_TH
    STORE_FW( rsp_th->first4, MPC_TH_FIRST4 );
    STORE_FW( rsp_th->offrrh, SIZE_TH );
    STORE_FW( rsp_th->length, uLength1 );
    STORE_HW( rsp_th->unknown10, 0x0FFC );            /* !!! */
    STORE_HW( rsp_th->numrrh, 1 );

    // Prepare MPC_RRH
    rsp_rrh->type = RRH_TYPE_ULP;
//  rsp_rrh->proto = PROTOCOL_UNKNOWN;
    rsp_rrh->proto = PROTOCOL_02;
    STORE_HW( rsp_rrh->numph, 1 );
    STORE_FW( rsp_rrh->seqnum, ++grp->seqnumcm );
    memcpy( rsp_rrh->ackseq, req_rrh->seqnum, 4 );
    STORE_HW( rsp_rrh->offph, SIZE_RRH_1 );
    STORE_HW( rsp_rrh->lenfida, (U16)uLength3 );
    STORE_F3( rsp_rrh->lenalda, uLength3 );
    rsp_rrh->tokenx5 = MPC_TOKEN_X5;
    memcpy( rsp_rrh->token, grp->gtcmconn, MPC_TOKEN_LENGTH );

    // Prepare MPC_PH
    rsp_ph->locdata = PH_LOC_1;
    STORE_F3( rsp_ph->lendata, uLength3 );
    STORE_FW( rsp_ph->offdata, uLength2 );

    // Prepare MPC_PUK
    STORE_HW( rsp_puk->length, SIZE_PUK );
    rsp_puk->what = PUK_WHAT_43;
    rsp_puk->type = PUK_TYPE_ACTIVE;
    STORE_HW( rsp_puk->lenpus, uLength4 );

    // Prepare first MPC_PUS
    STORE_HW( rsp_pus_04->length, SIZE_PUS_04 );
    rsp_pus_04->what = PUS_WHAT_04;
    rsp_pus_04->type = PUS_TYPE_04;
    rsp_pus_04->vc.pus_04.tokenx5 = MPC_TOKEN_X5;
    memcpy( rsp_pus_04->vc.pus_04.token, grp->gtulpconn, MPC_TOKEN_LENGTH );

    return rsp_bhr;
}

/*-------------------------------------------------------------------*/
/* Process the ULP_TAKEDOWN request from the guest.                  */
/*-------------------------------------------------------------------*/
/* There is one ULP_TAKEDOWN request for each of the data paths?     */
/* There is no response. If there is only one data path there is     */
/* often no ULP_TAKEDOWN request, just a ULP_DISABLE request.        */
/*-------------------------------------------------------------------*/
static OSA_BHR* process_ulp_takedown( DEVBLK* dev, MPC_TH* req_th, MPC_RRH* req_rrh, MPC_PUK* req_puk )
{
OSA_GRP *grp = (OSA_GRP*)dev->group->grp_data;

    UNREFERENCED(grp);
    UNREFERENCED(req_rrh);
    UNREFERENCED(req_puk);

    /* Display the request MPC_TH etc., maybe. */
    DBGUPD( dev, 1, req_th, 0, FROM_GUEST, "%s: Request", dev->dev_data );

    /* There will be no response. */
    return NULL;
}

/*-------------------------------------------------------------------*/
/* Process the ULP_DISABLE request from the guest.                   */
/*-------------------------------------------------------------------*/
/* There is one ULP_DISABLE request, irrespective of the number of   */
/* data paths. There is no response.                                 */
/*-------------------------------------------------------------------*/
static OSA_BHR* process_ulp_disable( DEVBLK* dev, MPC_TH* req_th, MPC_RRH* req_rrh, MPC_PUK* req_puk )
{
OSA_GRP *grp = (OSA_GRP*)dev->group->grp_data;

    UNREFERENCED(req_rrh);
    UNREFERENCED(req_puk);

    /* Display the request MPC_TH etc., maybe. */
    DBGUPD( dev, 1, req_th, 0, FROM_GUEST, "%s: Request", dev->dev_data );

    /* There will be a single MPC_PUS_03 containing the grp->gtcmfilt token */

    /* Disable the TUN interface */
    if (grp->l3)
        qeth_disable_interface( dev, grp );

    /* There will be no response. */
    return NULL;
}


/*--------------------------------------------------------------------*/
/* alloc_buffer(): Allocate storage for a OSA_BHR and data            */
/*--------------------------------------------------------------------*/
static OSA_BHR*  alloc_buffer( DEVBLK* dev, int size )
{
    OSA_BHR*   bhr;                        // OSA_BHR
    int        buflen;                     // Buffer length
    char       etext[40];                  // malloc error text

    // Allocate the buffer.
    buflen = SizeBHR + size;
    bhr = malloc( buflen );                // Allocate the buffer
    if (!bhr)                              // if the allocate was not successful...
    {
        // Report the bad news.
        MSGBUF( etext, "malloc(%n)", &buflen );
        // HHC00900 "%1d:%04X %s: error in function %s: %s"
        WRMSG(HHC00900, "E", LCSS_DEVNUM, dev->typname,
                             etext, strerror(errno) );
        return NULL;
    }

    // Clear the buffer.
    memset( bhr, 0, buflen );
    bhr->arealen = size;

    return bhr;
}

/*--------------------------------------------------------------------*/
/* add_buffer_to_chain(): Add OSA_BHR to end of chain.                */
/*--------------------------------------------------------------------*/
static void  add_buffer_to_chain( OSA_BAN* ban, OSA_BHR* bhr )
{
    // Prepare OSA_BHR for adding to chain.
    if (!bhr) return;                      // Any OSA_BHR been passed?
    bhr->next = NULL;                      // Clear the pointer to next OSA_BHR

    // Obtain the buffer chain lock.
    obtain_lock( &ban->lockbhr );

    // Add OSA_BHR to end of chain.
    if (ban->firstbhr)                     // if there are already OSA_BHRs
    {
        ban->lastbhr->next = bhr;          // Add the OSA_BHR to
        ban->lastbhr = bhr;                // the end of the chain
        ban->numbhr++;                     // Increment number of OSA_BHRs
    }
    else
    {
        ban->firstbhr = bhr;               // Make the OSA_BHR
        ban->lastbhr = bhr;                // the only OSA_BHR
        ban->numbhr = 1;                   // on the chain
    }

    // Release the buffer chain lock.
    release_lock( &ban->lockbhr );
}

/*--------------------------------------------------------------------*/
/* remove_buffer_from_chain(): Remove OSA_BHR from start of chain.    */
/*--------------------------------------------------------------------*/
static OSA_BHR*  remove_buffer_from_chain( OSA_BAN* ban )
{
    OSA_BHR*    bhr;                       // OSA_BHR

    // Obtain the buffer chain lock.
    obtain_lock( &ban->lockbhr );

    // Point to first OSA_BHR on the chain.
    bhr = ban->firstbhr;                   // Pointer to first OSA_BHR

    // Remove the first OSA_BHR from the chain, if there is one...
    if (bhr)                               // If there is a OSA_BHR
    {
        ban->firstbhr = bhr->next;         // Make the next the first OSA_BHR
        ban->numbhr--;                     // Decrement number of OSA_BHRs
        bhr->next = NULL;                  // Clear the pointer to next OSA_BHR
        if (!ban->firstbhr)                // if there are no more OSA_BHRs
        {
//          ban->firstbhr = NULL;          // Clear
            ban->lastbhr = NULL;           // the chain
            ban->numbhr = 0;               // pointers and count
        }
    }

    // Release the buffer chain lock.
    release_lock( &ban->lockbhr );

    return bhr;
}

/*--------------------------------------------------------------------*/
/* remove_and_free_any_buffers_on_chain(): Remove and free OSA_BHRs.  */
/*--------------------------------------------------------------------*/
static void  remove_and_free_any_buffers_on_chain( OSA_BAN* ban )
{
    OSA_BHR*    bhr;                       // OSA_BHR

    // Obtain the buffer chain lock.
    obtain_lock( &ban->lockbhr );

    // Remove and free the OSA_BHRs on the chain, if there are any...
    while( ban->firstbhr != NULL )
    {
        bhr = ban->firstbhr;               // Pointer to first OSA_BHR
        ban->firstbhr = bhr->next;         // Make the next the first OSA_BHR
        free( bhr->content );              // Free the buffer content string
        free( bhr );                       // Free the message buffer
    }

    // Reset the chain pointers.
    ban->firstbhr = NULL;                  // Clear
    ban->lastbhr = NULL;                   // the chain
    ban->numbhr = 0;                       // pointers and count

    // Release the buffer chain lock.
    release_lock( &ban->lockbhr );
}


/*--------------------------------------------------------------------*/
/* signal_idx_event():                                                */
/*--------------------------------------------------------------------*/
static void  signal_idx_event( OSA_GRP* grp )
{
    obtain_lock( &grp->qlock );
    signal_condition( &grp->qrcond );
    release_lock( &grp->qlock );
}


/*-------------------------------------------------------------------*/
/* Initialize MAC address                                            */
/*-------------------------------------------------------------------*/
static void InitMACAddr( DEVBLK* dev, OSA_GRP* grp )
{
    static const BYTE zeromac[IFHWADDRLEN] = {0};
    char* tthwaddr = NULL;
    BYTE iMAC[ IFHWADDRLEN ];
    char szMAC[3*IFHWADDRLEN] = {0};
    int rc = 0;
#if defined( ENABLE_IPV6 )
    int j;
    struct in6_addr addr6;
#endif

    /* Do different things for TAP's (layer 2) and TUN's (layer 3) */
    if (!grp->l3) {

        /* Retrieve the MAC Address directly from the TAP interface */
        rc = TUNTAP_GetMACAddr( grp->ttifname, &tthwaddr );

        /* Did we get what we wanted? */
        if (0
            || rc != 0
            || ParseMAC( tthwaddr, iMAC ) != 0
            || memcmp( iMAC, zeromac, IFHWADDRLEN ) == 0
        )
        {
            char* pszMAC;
            UNREFERENCED( dev ); // (unreferenced in non-debug build)
            DBGTRC( dev, "** WARNING ** TUNTAP_GetMACAddr() failed! Using default." );
            build_herc_iface_mac( iMAC, NULL );
            VERIFY( FormatMAC( &pszMAC, iMAC ) == 0);
            free( tthwaddr );
            tthwaddr = pszMAC;
        }

        grp->tthwaddr = strdup( tthwaddr );
        memcpy( grp->iMAC, iMAC, IFHWADDRLEN );
        free( tthwaddr );

#if defined( ENABLE_IPV6 )
        memset( &grp->iaDriveLLAddr6, 0, sizeof(grp->iaDriveLLAddr6) );
        memset( grp->szDriveLLAddr6, 0, sizeof(grp->szDriveLLAddr6) );
#endif

    } else {

        /* If grp->tthwaddr specified a valid MAC address use it, */
        /* otherwise create a MAC address. This MAC address is    */
        /* reported to and used by the guest. The reported MAC    */
        /* is used to create the guests link local IPv6 address.  */
        if (grp->tthwaddr &&
            ParseMAC( grp->tthwaddr, iMAC ) == 0 &&
            memcmp( iMAC, zeromac, IFHWADDRLEN ) != 0)
        {
            memcpy( grp->iMAC, iMAC, IFHWADDRLEN );
        } else {
            free( grp->tthwaddr );
            build_herc_iface_mac( iMAC, NULL );
            MSGBUF( szMAC, "%02x:%02x:%02x:%02x:%02x:%02x",
                    iMAC[0], iMAC[1], iMAC[2], iMAC[3], iMAC[4], iMAC[5] );
            grp->tthwaddr = strdup( szMAC );
            memcpy( grp->iMAC, iMAC, IFHWADDRLEN );
        }

#if defined( ENABLE_IPV6 )
        /* Create a Driver MAC address using pseudo-random numbers */
        for (j = 0; j < 6; j++)
            iMAC[j] = (int)((rand()/(RAND_MAX+1.0))*256);
        iMAC[0] &= 0xFE;  /* Clear multicast bit. */
        iMAC[0] |= 0x02;  /* Set local assignment bit. */
        memcpy( grp->iaDriveMACAddr, iMAC, IFHWADDRLEN );
        MSGBUF( grp->szDriveMACAddr,
                  "%02x:%02x:%02x:%02x:%02x:%02x",
                  iMAC[0], iMAC[1], iMAC[2], iMAC[3], iMAC[4], iMAC[5] );
        /* Create a Driver Link Local address from the Driver MAC address */
        addr6.s6_addr[0] = 0xFE;
        addr6.s6_addr[1] = 0x80;
        memset( &addr6.s6_addr[2], 0, 6 );
        memcpy( &addr6.s6_addr[8], &iMAC[0], 3 );
        addr6.s6_addr[11] = 0xFF;
        addr6.s6_addr[12] = 0xFE;
        memcpy( &addr6.s6_addr[13], &iMAC[3], 3 );
        memcpy( &grp->iaDriveLLAddr6, &addr6, sizeof(grp->iaDriveLLAddr6) );
        hinet_ntop( AF_INET6, &addr6, grp->szDriveLLAddr6, sizeof(grp->szDriveLLAddr6) );
#endif /* defined( ENABLE_IPV6 ) */
    }
}


/*-------------------------------------------------------------------*/
/* Initialize MTU value                                              */
/*-------------------------------------------------------------------*/
static void InitMTU( DEVBLK* dev, OSA_GRP* grp )
{
    char* ttmtu;
    U16 uMTU;
    int rc = 0;

    /* Retrieve the MTU value directly from the TUNTAP interface */
    rc = TUNTAP_GetMTU( grp->ttifname, &ttmtu );

    /* Did we get what we wanted? */
    if (0
        || rc != 0
        || !(uMTU = (U16) atoi( ttmtu ))
        || uMTU < (60    - 14)
        || uMTU > (65535 - 14)
    )
    {
        UNREFERENCED(dev); /*(unreferenced in non-debug build)*/
        DBGTRC(dev, "** WARNING ** TUNTAP_GetMTU() failed! Using default.");
        free( ttmtu );
        ttmtu = strdup( QETH_DEF_MTU );
        uMTU = (U16) atoi( ttmtu );
    }

    grp->ttmtu = strdup( ttmtu );
    grp->uMTU  = uMTU;

    free( ttmtu );
}


/*-------------------------------------------------------------------*/
/* Validate then convert IPv4 ttnetmask value to ttpfxlen value.     */
/* Returns 0 (zero) if valid and conversion successful, else -1.     */
/*-------------------------------------------------------------------*/
static int  netmask2prefix( char* ttnetmask, char** ttpfxlen )
{
    U32 netmask, mask;
    int pfxlen;
    char cbuf[16];
    netmask = ntohl( inet_addr( ttnetmask ));
    if (netmask == ntohl( INADDR_NONE ) &&
        strcmp( ttnetmask, "255.255.255.255" ) != 0)
        return -1;
    for (pfxlen=0, mask=netmask; mask & 0x80000000; mask <<= 1)
        pfxlen++;
    /* we don't support discontiguous subnets */
    if (netmask != (0xFFFFFFFF << (32-pfxlen)))
        return -1;
    MSGBUF( cbuf, "%d", pfxlen );
    free( *ttpfxlen );
    *ttpfxlen = strdup( cbuf );
    return 0;
}


/*-------------------------------------------------------------------*/
/* Validate then convert IPv4 ttpfxlen value to ttnetmask value.     */
/* Returns 0 (zero) if valid and conversion successful, else -1.     */
/*-------------------------------------------------------------------*/
static int  prefix2netmask( char* ttpfxlen, char** ttnetmask )
{
    struct in_addr addr4;
    char* p;
    int pfxlen;
    /* make sure it's a number from 0 to 32 */
    for (p = ttpfxlen; isdigit((unsigned char)*p); p++) { }
    if (*p || !ttpfxlen[0] || (pfxlen = atoi(ttpfxlen)) > 32)
        return -1;
    addr4.s_addr = ~makepfxmask4( ttpfxlen );
    if (!(p = inet_ntoa( addr4 )))
        return -1;
    free( *ttnetmask );
    *ttnetmask = strdup(p);
    return 0;
}


/*-------------------------------------------------------------------*/
/* Convert IPv4 ttpfxlen to a pfxmask4 mask value.  The passed       */
/* ttpfxlen value is presumed to have been previously validated.     */
/*-------------------------------------------------------------------*/
static U32 makepfxmask4( char* ttpfxlen )
{
    U32 pfxmask4;
    int pfxlen = atoi( ttpfxlen );
    if (pfxlen >= 32)
        pfxmask4 = 0x00000000;
    else if (pfxlen <= 0)
        pfxmask4 = 0xFFFFFFFF;
    else
        pfxmask4 = (0xFFFFFFFF >> pfxlen);
    return htonl( pfxmask4 );
}


/*-------------------------------------------------------------------*/
/* Convert IPv6 ttpfxlen6 to a pfxmask6 mask value.  The passed      */
/* ttpfxlen6 value is presumed to have been previously validated.    */
/*-------------------------------------------------------------------*/
static void makepfxmask6( char* ttpfxlen6, BYTE* pfxmask6 )
{
    if (ttpfxlen6)
    {
        int pfxlen = atoi( ttpfxlen6 );
        int quo = pfxlen / 8;
        int rem = pfxlen % 8;
        if (quo)
            memset( &pfxmask6[0], 0x00, quo );
        if (quo < 16)
        {
            memset( &pfxmask6[quo], 0xFF, 16-quo );
            pfxmask6[quo] = (0xFF >> rem);
        }
    }
    else
        memset( &pfxmask6[0], 0xFF, 16 );
}


#if defined( ENABLE_IPV6 )
/*-------------------------------------------------------------------*/
/* Process layer 3 ICMPv6 packet                                     */
/* Note: the ICMPv6 data follows the IPv6 header.                    */
/*-------------------------------------------------------------------*/
static void process_l3_icmpv6_packet(DEVBLK* dev, OSA_GRP* grp, IP6FRM* ip6)
{
    BYTE*      icmp;                // ICMPv6 header
    OSA_BHR*   bhrre;               // Response buffer header
    IP6FRM*    ip6re;               // Response IPv6 header
    BYTE*      icmpre;              // Response ICMPv6 data
    U32        ip6re_packet_size;   // Response IPv6 packet size
    U16        ip6re_header_size;   // Response IPv6 header size
    U16        ip6re_payload_size;  // Response ICMPv6 data size
    char       unspecified[16];
    char       solicitednode[16];
    BYTE       sig;

    // Initialize variables
    memset( unspecified, 0, 16 );
    memset( solicitednode, 0, 16 );
    solicitednode[0] = 0xFF;
    solicitednode[1] = 0x02;
    solicitednode[11] = 0x01;
    solicitednode[12] = 0xFF;

    // Point to the ICMPv6 header
    icmp = (BYTE*)ip6->bPayload;

    // Process Neighbor Solicitation
    if (icmp[0] == 0x87)    /* 0x87 = 135 */
    {                       /* Start of Neighbor Solicitation */

      // Check for a Neighbor Solicitation message sent by the guest
      // to verify that no other node is using the guests IP address.
      // The source address is unspecified, and the destination address
      // is the Solicited Node address, i.e. FF02::1:FFxx:xxxx.

//    if (memcmp( &ip6->bSrcAddr[0], &unspecified, 16 ) == 0 &&
//        memcmp( &ip6->bDstAddr[0], &solicitednode, 13 ) == 0 &&
//        memcmp( &ip6->bDstAddr[13], &icmp[8+13], 3 ) == 0)
//    {
//      return;
//    }

      // Check for a Neighbor Solicitation message sent by the guest
      // to resolve the specified IP address. The source address
      // is the guests IP address, and the destination address is
      // the Solicited Node address, i.e. FF02::1:FFxx:xxxx.

      if (memcmp( &ip6->bSrcAddr[0], &unspecified, 16 ) != 0 &&
          memcmp( &ip6->bDstAddr[0], &solicitednode, 13 ) == 0 &&
          memcmp( &ip6->bDstAddr[13], &icmp[8+13], 3 ) == 0)
      {
        // Prepare various sizes
        ip6re_header_size = sizeof(IP6FRM);
        ip6re_payload_size = 32;
        ip6re_packet_size = ip6re_header_size + ip6re_payload_size;

        // Allocate a buffer in which the ICMPv6 Neighbor Advertisment message
        // will be built. Note: the message will be 72 bytes.
        // The source address is the target address, the destination
        // address is the Link-Local Scope All Nodes multicast address,
        // i.e. FF02:0:0:0:0:0:0:1.

        bhrre = alloc_buffer( dev, (ip6re_packet_size + 10) );
        if (!bhrre) return;
        bhrre->datalen = ip6re_packet_size;
        ip6re = (IP6FRM*)((BYTE*)bhrre + SizeBHR);
        icmpre = (BYTE*)ip6re->bPayload;

        // Prepare response IPv6 header
        ip6re->bVersTCFlow[0] = 0x60;
        STORE_HW( ip6re->bPayloadLength, ip6re_payload_size );
        ip6re->bNextHeader = 0x3A;
        ip6re->bHopLimit = 0xFF;
        memcpy( ip6re->bSrcAddr, &icmp[8], 16 );
        memcpy( ip6re->bDstAddr, ip6->bSrcAddr, 16 );

        // Prepare response ICMPv6 data
        icmpre[0] = 0x88;                    /* Neighbor Advertisment  0x88 = 136 */
        icmpre[4] = 0x60;                    /* Solicited & Override flags */
        memcpy( &icmpre[8], &icmp[8], 16 );  /* Target IP address */
        icmpre[24] = 0x02;                   /* Target link local option */
        icmpre[25] = 0x01;                   /* Length of option in 8-byte units */
        memcpy( &icmpre[26], grp->iaDriveMACAddr, IFHWADDRLEN );

        // Calculate and set the ICMPv6 checksum
        calculate_icmpv6_checksum( ip6re, icmpre, (int)ip6re_payload_size );

        // Add response buffer to chain.
        add_buffer_to_chain( &grp->l3r, bhrre );
        sig = QDSIG_WAKEUP;
        VERIFY( qeth_write_pipe( grp->ppfd[1], &sig ) == 1);
        return;
      }

    } /* End of Neighbor Solicitation */

    // Hmm... an ICMPv6 message sent by the guest
    // that we don't handle.
}
#endif /* defined( ENABLE_IPV6 ) */


#if defined( ENABLE_IPV6 )
/* ------------------------------------------------------------------ */
/*                 calculate_icmpv6_checksum()                        */
/* ------------------------------------------------------------------ */
//
// This is not a general purpose function, it is solely for calculating
// the checksum of ICMPv6 packets sent to the guest on the y-side. The
// following restriction apply:
//
//   1. If the ICMPv6 header and message has a length that is an odd
//      number of bytes they must be followed by a byte containing zero.
//
//   2. Any existing checksum in the ICMPv6 header will be over-written.
//
// The ICMPv6 header and message layout is:
//
//   byte  0    Type: specifies the format of the message.
//   byte  1    Code: further qualifies the message
//   bytes 2-3  Checksum.
//   byte  4-n  Message.
//
// The Hop-by-Hop Options extension header layout is:
//
//   byte  0    Next Header: will be 58 (0x3a), ICMPv6 header.
//   byte  1    Header Extension Length: the header's overall length
//              0 = 8-bytes, 1 = 16-bytes, 2 = 24-bytes, etc.
//   byte  2-n  One or more option fields.
//
void  calculate_icmpv6_checksum( IP6FRM* pIP6FRM, BYTE* pIcmpHdr, int iIcmpLen )
{
    BYTE*      pBytePtr;
    int        i,j;
    U16        uTwobytes;            // Two bytes of data
    U32        uHighhalf;            // High-order half of the checksum
    U32        uChecksum;            // The checksum
    BYTE       bPseudoHeader[40];    //  0-15  Source address
                                     // 16-31  Destination address
                                     // 32-35  Upper-layer packet length
                                     // 36-38  zero
                                     //   39   Next Header (i.e. 58 (0x3a))

    // Clear the checksum in the ICMP header before calculating the checksum.
    STORE_HW( pIcmpHdr+2, 0x0000 );

    // Construct the Psuedo-Header for the checksum calcuation.
    memcpy( bPseudoHeader+0, pIP6FRM->bSrcAddr, 16 );
    memcpy( bPseudoHeader+16, pIP6FRM->bDstAddr, 16 );
    STORE_FW( bPseudoHeader+32, iIcmpLen );
    for( i = 36; i <= 38; i++ )
        bPseudoHeader[i] = 0x00;
    bPseudoHeader[39] = 0x3A;

    // Calculate the checksum.
    uChecksum = 0;
    for( i = 0; i <= 38; i += 2 )
    {
        FETCH_HW( uTwobytes, bPseudoHeader+i );
        uChecksum += uTwobytes;
    }
    pBytePtr = pIcmpHdr;                   // Point to the ICMPv6 header.
    j = iIcmpLen;                          // Get the length of the
    j++;                                   // ICMPv6 packet rounded up
    j &= 0xFFFFFFFE;                       // to the next multiple of two.
    for( i = 0; i <= j - 2; i += 2 )
    {
        FETCH_HW( uTwobytes, pBytePtr );
        uChecksum += uTwobytes;
        pBytePtr += 2;
    }
    uHighhalf = uChecksum >> 16;           // Get the high-order half
                                           // of the checksum value.
    uChecksum &= 0x0000FFFF;               // Get the low-order half.
    uChecksum += uHighhalf;                // Add the high-order half
                                           // to the low-order half.
    uHighhalf = uChecksum >> 16;           // Get the high-order half
                                           // of the checksum value again.
    uChecksum &= 0x0000FFFF;               // Get the low-order half.
    uChecksum += uHighhalf;                // Add the high-order half
                                           // to the low-order half again
                                           // to include any carry.
    uChecksum ^= 0xFFFFFFFF;               // Complement the bits.
    uChecksum &= 0x0000FFFF;               // Get a clean checksum.
    uTwobytes = uChecksum;                 // Copy to a two-byte value.

    // Set the checksum in the ICMP header.
    STORE_HW( pIcmpHdr+2, uTwobytes );

}   /* End function  calculate_icmpv6_checksum() */
#endif /* defined( ENABLE_IPV6 ) */


/*-------------------------------------------------------------------*/
/* Very important things                                             */
/*-------------------------------------------------------------------*/

static DEVHND qeth_device_hndinfo =
{
        &qeth_init_handler,            /* Device Initialization      */
        &qeth_execute_ccw,             /* Device CCW execute         */
        &qeth_close_device,            /* Device Close               */
        &qeth_query_device,            /* Device Query               */
        NULL,                          /* Device Extended Query      */
        NULL,                          /* Device Start channel pgm   */
        NULL,                          /* Device End channel pgm     */
        NULL,                          /* Device Resume channel pgm  */
        NULL,                          /* Device Suspend channel pgm */
        &qeth_halt_or_clear,           /* Device Halt channel pgm    */
        NULL,                          /* Device Read                */
        NULL,                          /* Device Write               */
        NULL,                          /* Device Query used          */
        NULL,                          /* Device Reserve             */
        NULL,                          /* Device Release             */
        NULL,                          /* Device Attention           */
        qeth_immed_commands,           /* Immediate CCW Codes        */
        &qeth_initiate_input,          /* Signal Adapter Input       */
        &qeth_initiate_output,         /* Signal Adapter Output      */
        &qeth_do_sync,                 /* Signal Adapter Sync        */
        &qeth_initiate_output_mult,    /* Signal Adapter Output Mult */
        &qeth_ssqd_desc,               /* QDIO subsys desc           */
#if defined( _FEATURE_QDIO_THININT )
        &qeth_set_sci,                 /* QDIO set subchan ind       */
#else
        NULL,                          /* QDIO set subchan ind       */
#endif
        NULL,                          /* Hercules suspend           */
        NULL                           /* Hercules resume            */
};

/* Libtool static name colision resolution */
/* note : lt_dlopen will look for symbol & modulename_LTX_symbol */
#if defined( HDL_USE_LIBTOOL )
#define hdl_ddev hdtqeth_LTX_hdl_ddev
#define hdl_depc hdtqeth_LTX_hdl_depc
#define hdl_reso hdtqeth_LTX_hdl_reso
#define hdl_init hdtqeth_LTX_hdl_init
#define hdl_fini hdtqeth_LTX_hdl_fini
#endif

HDL_DEPENDENCY_SECTION;
{
    HDL_DEPENDENCY( HERCULES );
    HDL_DEPENDENCY( DEVBLK );
    HDL_DEPENDENCY( SYSBLK );

    memcpy( (NED*)&configuration_data[0], &osa_device_ned [0], sizeof( NED ));
    memcpy( (NED*)&configuration_data[1], &osa_ctlunit_ned[0], sizeof( NED ));
    memcpy( (NED*)&configuration_data[2], &osa_token_ned  [0], sizeof( NED ));
    memcpy( (NED*)&configuration_data[3], &osa_general_neq[0], sizeof( NEQ ));

    memcpy( (ND*)&node_data[0], &osa_nd[0], sizeof( ND ));
    memcpy( (ND*)&node_data[1], &osa_nq[0], sizeof( NQ ));
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


HDL_REGISTER_SECTION;
{
//               Hercules's          Our
//               registered          overriding
//               entry-point         entry-point
//               name                value
#if defined( WIN32 )
  HDL_REGISTER ( debug_tt32_stats,   display_tt32_stats        );
  HDL_REGISTER ( debug_tt32_tracing, enable_tt32_debug_tracing );
#else
  UNREFERENCED( regsym );   // (HDL_REGISTER_SECTION parameter)
#endif
}
END_REGISTER_SECTION


HDL_DEVICE_SECTION;
{
    HDL_DEVICE ( QETH, qeth_device_hndinfo );
    HDL_DEVICE ( OSA,  qeth_device_hndinfo );
    HDL_DEVICE ( OSD,  qeth_device_hndinfo );
}
END_DEVICE_SECTION

#if defined( _MSVC_ ) && defined( NO_QETH_OPTIMIZE )
  #pragma optimize( "", on )            // restore previous settings
#endif
