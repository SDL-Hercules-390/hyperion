/* HEXTERNS.H   (C) Copyright Roger Bowler, 1999-2012                */
/*              (C) and others 2013-2023                             */
/*                    Hercules function prototypes...                */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

//      This header auto-#included by 'hercules.h'...
//
//      The <config.h> header and other required headers are
//      presumed to have already been #included ahead of it...

#ifndef _HEXTERNS_H
#define _HEXTERNS_H

#include "hercules.h"
#include "cckd.h"

#if !defined(HAVE_STRSIGNAL)
  const char* strsignal(int signo);    // (ours is in 'strsignal.c')
#endif

#if defined(HAVE_SETRESUID)
/* (the following missing from SUSE 7.1) */
int getresuid(uid_t *ruid, uid_t *euid, uid_t *suid);
int getresgid(gid_t *rgid, gid_t *egid, gid_t *sgid);
int setresuid(uid_t ruid, uid_t euid, uid_t suid);
int setresgid(gid_t rgid, gid_t egid, gid_t sgid);
#endif

/* Function used to compare filenames */
#if defined(MIXEDCASE_FILENAMES_ARE_UNIQUE)
  #define strfilenamecmp   strcmp
  #define strnfilenamecmp  strncmp
#else
  #define strfilenamecmp   strcasecmp
  #define strnfilenamecmp  strncasecmp
#endif

/* Global data areas in module hsys.c */
HSYS_DLL_IMPORT SYSBLK   sysblk;        /* System control block      */
HSYS_DLL_IMPORT int      extgui;        /* true = extgui active      */

/* Functions in module bldcfg.c */
int build_config (const char *fname);
BLDC_DLL_IMPORT const char* init_sysblk_netdev();

/* Functions in module script.c */
SCRI_DLL_IMPORT int process_config (const char *fname);

/* Functions in module config.c */
void release_config ( void* );
CONF_DLL_IMPORT DEVBLK *find_device_by_devnum (U16 lcss, U16 devnum);
DEVBLK *find_device_by_subchan (U32 ioid);
int  attach_device (U16 lcss, U16 devnum, const char *devtype, int addargc,
        char *addargv[], int numconfdev);
int  detach_device (U16 lcss, U16 devnum);
int  define_device (U16 lcss, U16 olddev, U16 newdev);
CONF_DLL_IMPORT int  group_device(DEVBLK *dev, int members);
CONF_DLL_IMPORT BYTE free_group(DEVGRP *group, int locked, const char *msg, DEVBLK *errdev);
int  configure_cpu (int cpu);
int  deconfigure_cpu (int cpu);
int  configure_numcpu (int numcpu);
int  configure_maxcpu (int maxcpu);
int  configure_memlock(int);
int  configure_memfree(int);
int  configure_storage( U64 /* number of 4K pages */ );
int  configure_xstorage(U64);
U64  adjust_mainsize( int archnum, U64 mainsize );

int  configure_shrdport(U16 shrdport);
SHR_DLL_IMPORT void shutdown_shared_server       ( void* unused );
SHR_DLL_IMPORT void shutdown_shared_server_locked( void* unused );
#define MAX_ARGS  1024                  /* Max argv[] array size     */
int parse_and_attach_devices(const char *devnums,const char *devtype,int ac,char **av);
CONF_DLL_IMPORT int parse_single_devnum(const char *spec, U16 *lcss, U16 *devnum);
int parse_single_devnum_silent(const char *spec, U16 *lcss, U16 *devnum);
struct DEVARRAY
{
    U16 cuu1;
    U16 cuu2;
};
typedef struct DEVARRAY DEVARRAY;
struct DEVNUMSDESC
{
    BYTE lcss;
    DEVARRAY *da;
};
typedef struct DEVNUMSDESC DEVNUMSDESC;
CONF_DLL_IMPORT size_t parse_devnums(const char *spec,DEVNUMSDESC *dd);
CONF_DLL_IMPORT int readlogo(char *fn);
CONF_DLL_IMPORT void clearlogo(void);
CONF_DLL_IMPORT int parse_conkpalv(char* s, int* idle, int* intv, int* cnt );
CONF_DLL_IMPORT bool is_diag_instr();
CONF_DLL_IMPORT bool are_cpu_thread( int* cpunum );

/* Functions in module panel.c */
void expire_kept_msgs(int unconditional);
void set_console_title(char * status);
HPAN_DLL_IMPORT U32    maxrates_rpt_intvl;  // (reporting interval)
HPAN_DLL_IMPORT U32    curr_high_mips_rate; // (high water mark for current interval)
HPAN_DLL_IMPORT U32    curr_high_sios_rate; // (high water mark for current interval)
HPAN_DLL_IMPORT U32    prev_high_mips_rate; // (saved high water mark for previous interval)
HPAN_DLL_IMPORT U32    prev_high_sios_rate; // (saved high water mark for previous interval)
HPAN_DLL_IMPORT time_t curr_int_start_time; // (start time of current interval)
HPAN_DLL_IMPORT time_t prev_int_start_time; // (start time of previous interval)
HPAN_DLL_IMPORT void update_maxrates_hwm(); // (update high-water-mark values)
HPAN_DLL_IMPORT void set_panel_colors();    // (set panel message colors)

/* Functions in module hao.c (Hercules Automatic Operator) */
#if defined(OPTION_HAO)
HAO_DLL_IMPORT void hao_command(char *command); /* process hao command */
#endif /* defined(OPTION_HAO) */

/* Functions in module hsccmd.c (so PTT debugging patches can access them) */
extern int quit_cmd(     int argc, char* argv[], char* cmdline );
extern int quitmout_cmd( int argc, char* argv[], char* cmdline );
HCMD_DLL_IMPORT int devinit_cmd( int argc, char* argv[], char* cmdline ); // used by CTCE_Recovery()
extern int qproc_cmd( int argc, char* argv[], char* cmdline );

/* Functions in module hscpufun.c (so PTT debugging patches can access them) */
HCPU_DLL_IMPORT int stopall_cmd   ( int argc, char* argv[], char* cmdline );
extern          int start_cmd_cpu ( int argc, char* argv[], char* cmdline );
extern          int stop_cmd_cpu  ( int argc, char* argv[], char* cmdline );
extern          int restart_cmd   ( int argc, char* argv[], char* cmdline );

/* Functions in module hscemode.c (so PTT debugging patches can access them) */
HCEM_DLL_IMPORT int aia_cmd       ( int argc, char* argv[], char* cmdline );

/* CPU ID related functions in module hscemode.c */
extern U64  createCpuId      ( const U64 model, const U64 version, const U64 serial, const U64 MCEL );
extern BYTE setAllCpuIds     ( const S32 model, const S16 version, const S32 serial, const S32 MCEL, bool force );
extern BYTE setAllCpuIds_lock( const S32 model, const S16 version, const S32 serial, const S32 MCEL, bool force );
extern void setCpuIdregs     ( REGS* regs, S32 arg_model, S16 arg_version, S32 arg_serial, S32 arg_MCEL, bool force );
extern void setCpuId         ( const unsigned int cpu, S32 arg_model, S16 arg_version, S32 arg_serial, S32 arg_MCEL, bool force );
extern BYTE resetAllCpuIds();
extern void setOperationMode();
extern void enable_lparmode( const bool enable );

/* Functions in module cmdtab.c */
CMDT_DLL_IMPORT int InternalHercCmd(char *cmdline); /* (NEVER for guest) */
CMDT_DLL_IMPORT int HercCmdLine (char *cmdline);    /* (maybe guest cmd) */
/* Note: ALL arguments -- including argument 3 (cmdline) -- are REQUIRED */
CMDT_DLL_IMPORT int CallHercCmd (int argc, char **argv, char *cmdline);

/* Functions in module losc.c */
void losc_set  ( int license_status );
void losc_check( char* ostype );

/*-------------------------------------------------------------------*/
/* The following functions are never called directly. They instead   */
/* are called via the function pointers in hsys.c (which point to    */
/* the real functions or elsewhere if overridden by an HDL module).  */
/*-------------------------------------------------------------------*/

HPAN_DLL_IMPORT  void   the_real_panel_display  ();
CMDT_DLL_IMPORT  void*  the_real_panel_command  ( char* cmdline );
OPCD_DLL_IMPORT  void*  the_real_replace_opcode ( int arch, INSTR_FUNC inst, int opcode1, int opcode2 );

/*-------------------------------------------------------------------*/
/* The following functions pointers are defined in hsys.c and will   */
/* either be NULL, point to the above real functions or else point   */
/* somewhere else if they are overridden by an HDL module.           */
/*-------------------------------------------------------------------*/

HSYS_DLL_IMPORT  PANDISP*       panel_display;
HSYS_DLL_IMPORT  PANDISP*       daemon_task;
HSYS_DLL_IMPORT  PANCMD*        panel_command;
HSYS_DLL_IMPORT  SYSTEMCMD*     system_command;
HSYS_DLL_IMPORT  REPOPCODE*     replace_opcode;

#if defined( OPTION_W32_CTCI )
HSYS_DLL_IMPORT  DBGT32ST*      debug_tt32_stats;
HSYS_DLL_IMPORT  DBGT32TR*      debug_tt32_tracing;
#endif

/*-------------------------------------------------------------------*/
/* The following overridable debugging hook function pointers are    */
/* defined in hsys.c and called via the "HDC1", "HDC2", etc, macros  */
/*-------------------------------------------------------------------*/

HSYS_DLL_IMPORT  HDLDBGCD*      debug_cd_cmd;
HSYS_DLL_IMPORT  HDLDBGCPU*     debug_cpu_state;
HSYS_DLL_IMPORT  HDLDBGCPU*     debug_watchdog_signal;
HSYS_DLL_IMPORT  HDLDBGPGMI*    debug_program_interrupt;
HSYS_DLL_IMPORT  HDLDBGDIAG*    debug_diagnose;

HSYS_DLL_IMPORT  HDLDBGSCLPUC*  debug_sclp_unknown_command;
HSYS_DLL_IMPORT  HDLDBGSCLPUE*  debug_sclp_unknown_event;
HSYS_DLL_IMPORT  HDLDBGSCLPUE*  debug_sclp_unknown_event_mask;
HSYS_DLL_IMPORT  HDLDBGSCLPUE*  debug_sclp_event_data;
HSYS_DLL_IMPORT  HDLDBGSCLPUE*  debug_chsc_unknown_request;

/*-------------------------------------------------------------------*/

/* Functions in module httpserv.c */
int http_command(int argc, char *argv[]);
int http_startup(int isconfigcalling);
char *http_get_root();
char *http_get_port();
char *http_get_portauth();

/* Functions in module loadparm.c */
void set_loadparm(char *name);
void get_loadparm(BYTE *dest);
char *str_loadparm();
void set_lparname(char *name);
void get_lparname(BYTE *dest);
LOADPARM_DLL_IMPORT char *str_lparname();
int set_manufacturer(char *name);
LOADPARM_DLL_IMPORT char *str_manufacturer();
int set_plant(char *name);
LOADPARM_DLL_IMPORT char *str_plant();
int set_model(char *m1, char* m2, char* m3, char* m4);
char *str_modelhard();
char *str_modelcapa();
char *str_modelperm();
char *str_modeltemp();
void get_manufacturer(BYTE *name);
void get_plant(BYTE *name);
void get_model(BYTE *name);
void get_modelcapa(BYTE *name);
void get_modelperm(BYTE *name);
void get_modeltemp(BYTE *name);
unsigned int get_RealCPCount();
void get_sysname(BYTE *name);
void get_systype(BYTE *name);
void get_sysplex(BYTE *name);
void set_sysname(BYTE *name);
void set_systype(BYTE *name);
void set_sysplex(BYTE *name);
LOADPARM_DLL_IMPORT char *str_sysname();
LOADPARM_DLL_IMPORT char *str_sysplex();
LOADPARM_DLL_IMPORT char *str_systype();
void get_vmid(BYTE *name);
void set_vmid(BYTE *name);
LOADPARM_DLL_IMPORT char *str_vmid();
void get_cpid(BYTE *name);
void set_cpmid(BYTE *name);
LOADPARM_DLL_IMPORT char *str_cpid();
void bld_sysib_sequence( BYTE* seqc );
void get_mpfactors(BYTE *dest);

/* Functions in module impl.c */
IMPL_DLL_IMPORT int impl(int,char **);
typedef void (*LOGCALLBACK)( const char*, size_t );
typedef void *(*COMMANDHANDLER)(char *);
IMPL_DLL_IMPORT void registerLogCallback(LOGCALLBACK);
IMPL_DLL_IMPORT COMMANDHANDLER getCommandHandler(void);

/* Functions in module timer.c */
void* timer_thread( void* argp );
#if defined( _FEATURE_073_TRANSACT_EXEC_FACILITY )
void* rubato_thread( void* argp );
#endif

/* Functions in module clock.c */
void update_TOD_clock (void);
int configure_epoch(int);
int configure_yroffset(int);
int configure_tzoffset(int);

/* Functions in module service.c */
int scp_command( const char* command, bool priomsg, bool echo, bool mask );
int can_signal_quiesce ();
int can_send_command ();
int signal_quiesce (U16 count, BYTE unit);
void sclp_attention(U16 type);
void sclp_reset();
SERV_DLL_IMPORT void sclp_sysg_attention();
int servc_hsuspend(void *file);
int servc_hresume(void *file);

#if defined( _FEATURE_073_TRANSACT_EXEC_FACILITY )
/* Functions in module transact.c */
TRANS_DLL_IMPORT BYTE* txf_maddr_l( const U64  vaddr,   const size_t  len,
                                    const int  arn,     REGS*         regs,
                                    const int  acctype, BYTE*         maddr );
TRANS_DLL_IMPORT void s370_abort_transaction( REGS* regs, int retry, int txf_tac, const char* loc );
TRANS_DLL_IMPORT void s390_abort_transaction( REGS* regs, int retry, int txf_tac, const char* loc );
TRANS_DLL_IMPORT void z900_abort_transaction( REGS* regs, int retry, int txf_tac, const char* loc );
TRANS_DLL_IMPORT void z900_txf_do_pi_filtering( REGS* regs, int code );
void alloc_txfmap( REGS* regs );
void free_txfmap( REGS* regs );
void txf_abort_all( U16 cpuad, int why, const char* location );
#endif

/* Functions in module ckddasd.c */
void ckd_build_sense ( DEVBLK *, BYTE, BYTE, BYTE, BYTE, BYTE);
int ckd_dasd_init_handler   ( DEVBLK *dev, int argc, char *argv[]);
int ckd64_dasd_init_handler ( DEVBLK *dev, int argc, char *argv[]);
void ckd_dasd_execute_ccw ( DEVBLK *dev, BYTE code, BYTE flags,
        BYTE chained, U32 count, BYTE prevcode, int ccwseq,
        BYTE *iobuf, BYTE *more, BYTE *unitstat, U32 *residual );
int ckd_dasd_close_device ( DEVBLK *dev );
void ckd_dasd_query_device (DEVBLK *dev, char **devclass,
                int buflen, char *buffer);
int ckd_dasd_hsuspend ( DEVBLK *dev, void *file );
int ckd_dasd_hresume  ( DEVBLK *dev, void *file );

/* Functions in module fbadasd.c */
FBA_DLL_IMPORT void fbadasd_syncblk_io (DEVBLK *dev, BYTE type, int blknum,
        int blksize, BYTE *iobuf, BYTE *unitstat, U32 *residual);
FBA_DLL_IMPORT void fbadasd_read_block
      ( DEVBLK *dev, int blknum, int blksize, int blkfactor,
        BYTE *iobuf, BYTE *unitstat, U32 *residual );
FBA_DLL_IMPORT void fbadasd_write_block
      ( DEVBLK *dev, int blknum, int blksize, int blkfactor,
        BYTE *iobuf, BYTE *unitstat, U32 *residual );
int fba_dasd_init_handler   ( DEVBLK *dev, int argc, char *argv[]);
int fba64_dasd_init_handler ( DEVBLK *dev, int argc, char *argv[]);
void fba_dasd_execute_ccw ( DEVBLK *dev, BYTE code, BYTE flags,
        BYTE chained, U32 count, BYTE prevcode, int ccwseq,
        BYTE *iobuf, BYTE *more, BYTE *unitstat, U32 *residual );
int fba_dasd_close_device ( DEVBLK *dev );
void fba_dasd_query_device (DEVBLK *dev, char **devclass,
                int buflen, char *buffer);
int fba_dasd_hsuspend ( DEVBLK *dev, void *file );
int fba_dasd_hresume  ( DEVBLK *dev, void *file );

/* Functions in module cckddasd.c/cckddasd64.c */

DEVIF   cckd_dasd_init_handler;
int     cckd_dasd_close_device (DEVBLK *);
int     cckd_read_track (DEVBLK *, int, BYTE *);
int     cckd_update_track (DEVBLK *, int, int, BYTE *, int, BYTE *);
int     cfba_read_block (DEVBLK *, int, BYTE *);
int     cfba_write_block (DEVBLK *, int, int, BYTE *, int, BYTE *);

DEVIF   cckd64_dasd_init_handler;
int     cckd64_dasd_close_device (DEVBLK *);
int     cckd64_read_track (DEVBLK *, int, BYTE *);
int     cckd64_update_track (DEVBLK *, int, int, BYTE *, int, BYTE *);
int     cfba64_read_block (DEVBLK *, int, BYTE *);
int     cfba64_write_block (DEVBLK *, int, int, BYTE *, int, BYTE *);

/* Functions in module cckdutil.c/cckdutil64.c */
CCDU_DLL_IMPORT const char* comp_to_str( BYTE comp );
CCDU_DLL_IMPORT const char* spc_typ_to_str( BYTE spc_typ );

CCDU_DLL_IMPORT   int   cckd_swapend (DEVBLK *);
CCDU_DLL_IMPORT   void  cckd_swapend_chdr ( CCKD_DEVHDR* );
CCDU_DLL_IMPORT   void  cckd_swapend_l1   ( CCKD_L1ENT*, int num_L1tab );
CCDU_DLL_IMPORT   void  cckd_swapend_l2   ( CCKD_L2ENT* );
CCDU_DLL_IMPORT   void  cckd_swapend_free ( CCKD_FREEBLK* );

CCDU64_DLL_IMPORT int   cckd64_swapend (DEVBLK *);
CCDU64_DLL_IMPORT void  cckd64_swapend_chdr ( CCKD64_DEVHDR* );
CCDU64_DLL_IMPORT void  cckd64_swapend_l1   ( CCKD64_L1ENT*, int );
CCDU64_DLL_IMPORT void  cckd64_swapend_l2   ( CCKD64_L2ENT* );
CCDU64_DLL_IMPORT void  cckd64_swapend_free ( CCKD64_FREEBLK* );
CCDU64_DLL_IMPORT int   cckd64_comp (DEVBLK *);
CCDU64_DLL_IMPORT int   cckd64_chkdsk (DEVBLK *, int);

CCDU_DLL_IMPORT   int   cckd_def_opt_bigend ();
CCDU_DLL_IMPORT   int   cckd_comp (DEVBLK *);
CCDU_DLL_IMPORT   int   cckd_chkdsk (DEVBLK *, int);

/* Functions in module hscmisc.c */
int herc_system (char* command);
void do_shutdown();
bool insttrace_all();

int display_gregs (REGS *regs, char *buf, int buflen, char *hdr);
int display_fregs (REGS *regs, char *buf, int buflen, char *hdr);
int display_vregs (REGS *regs, char *buf, int buflen, char *hdr);
int display_cregs (REGS *regs, char *buf, int buflen, char *hdr);
int display_aregs (REGS *regs, char *buf, int buflen, char *hdr);
int display_subchannel (DEVBLK *dev, char *buf, int buflen, char *hdr);
int display_inst_regs( bool trace2file, REGS* regs, BYTE* inst, BYTE opcode, char* buf, int buflen );

const char* FormatCRW( U32  crw, char* buf, size_t bufsz );
const char* FormatSCL( ESW* esw, char* buf, size_t bufsz );
const char* FormatERW( ESW* esw, char* buf, size_t bufsz );
const char* FormatESW( ESW* esw, char* buf, size_t bufsz );
HMISC_DLL_IMPORT int parse_range (char *operand, U64 maxadr, U64 *sadrp, U64 *eadrp, BYTE *newval);
HMISC_DLL_IMPORT REGS* copy_regs( REGS* regs );
HMISC_DLL_IMPORT const char* FormatSID( BYTE* iobuf, int num, char* buf, size_t bufsz );
HMISC_DLL_IMPORT const char* FormatRCD( BYTE* iobuf, int num, char* buf, size_t bufsz );
HMISC_DLL_IMPORT const char* FormatRNI( BYTE* iobuf, int num, char* buf, size_t bufsz );

HMISC_DLL_IMPORT int s370_virt_to_real( U64* raptr, int* siptr, U64 vaddr, int arn, REGS* regs, int acctype );
HMISC_DLL_IMPORT int s390_virt_to_real( U64* raptr, int* siptr, U64 vaddr, int arn, REGS* regs, int acctype );
HMISC_DLL_IMPORT int z900_virt_to_real( U64* raptr, int* siptr, U64 vaddr, int arn, REGS* regs, int acctype );
void get_connected_client (DEVBLK* dev, char** pclientip, char** pclientname);
void alter_display_real_or_abs (REGS *regs, int argc, char *argv[], char *cmdline);
void alter_display_virt (REGS *regs, int argc, char *argv[], char *cmdline);
void disasm_stor        (REGS *regs, int argc, char *argv[], char *cmdline);
int drop_privileges(int capa);

#if defined(HAVE_OBJECT_REXX) || defined(HAVE_REGINA_REXX)
/* Functions in module hrexx.c */
int rexx_cmd( int argc, char* argv[], char* cmdline );
int exec_cmd( int argc, char* argv[], char* cmdline );
#endif

/* Functions in module sr.c */
int suspend_cmd(int argc, char *argv[],char *cmdline);
int resume_cmd(int argc, char *argv[],char *cmdline);

/* Functions in module ecpsvm.c that are not *direct* instructions   */
/* but rather are instead support functions used by either other     */
/* instruction functions or elsewhere.                               */
#if defined( _FEATURE_ECPSVM )
int  ecpsvm_dosvc(REGS *regs, int svccode);
int  ecpsvm_dossm(REGS *regs,int b,VADR ea);
int  ecpsvm_dolpsw(REGS *regs,int b,VADR ea);
int  ecpsvm_dostnsm(REGS *regs,int b,VADR ea,int imm);
int  ecpsvm_dostosm(REGS *regs,int b,VADR ea,int imm);
int  ecpsvm_dosio(REGS *regs,int b,VADR ea);
int  ecpsvm_dodiag(REGS *regs,int r1,int r3,int b2,VADR effective_addr2);
int  ecpsvm_dolctl(REGS *regs,int r1,int r3,int b2,VADR effective_addr2);
int  ecpsvm_dostctl(REGS *regs,int r1,int r3,int b2,VADR effective_addr2);
int  ecpsvm_doiucv(REGS *regs,int b2,VADR effective_addr2);
int  ecpsvm_virttmr_ext(REGS *regs);
int  ecpsvm_dolra(REGS *regs,int r1,int b2,VADR effective_addr2);
#endif

/* Functions in module w32ctca.c */
#if defined(OPTION_W32_CTCI)
HSYS_DLL_IMPORT int  (*debug_tt32_stats)   (int);
HSYS_DLL_IMPORT bool (*debug_tt32_tracing) (int);
#endif // defined(OPTION_W32_CTCI)

/* Functions in module crypto.c */
#if defined( _FEATURE_076_MSA_EXTENSION_FACILITY_3 )
void renew_wrapping_keys(void);
#endif

/* Functions in module getopt.c */
GOP_DLL_IMPORT int   opterr;    /* if error message should be printed */
GOP_DLL_IMPORT int   optind;    /* index into parent argv vector */
GOP_DLL_IMPORT int   optopt;    /* character checked for validity */
GOP_DLL_IMPORT int   optreset;  /* reset getopt */
GOP_DLL_IMPORT char* optarg;    /* argument associated with option */
GOP_DLL_IMPORT int   getopt      ( int nargc, char * const *nargv, const char *options );
GOP_DLL_IMPORT int   getopt_long ( int nargc, char * const *nargv, const char *options, const struct option *long_options, int *idx );

/* Functions in module channel.c */
                void shared_iowait (DEVBLK *dev);
CHAN_DLL_IMPORT int  device_attention (DEVBLK *dev, BYTE unitstat);
CHAN_DLL_IMPORT int  ARCH_DEP(device_attention) (DEVBLK *dev, BYTE unitstat);
CHAN_DLL_IMPORT void default_sns( char* buf, size_t buflen, BYTE b0, BYTE b1 );

CHAN_DLL_IMPORT void Queue_IO_Interrupt           (IOINT* io, U8 clrbsy, const char* location);
CHAN_DLL_IMPORT void Queue_IO_Interrupt_QLocked   (IOINT* io, U8 clrbsy, const char* location);
CHAN_DLL_IMPORT int  Dequeue_IO_Interrupt         (IOINT* io,            const char* location);
CHAN_DLL_IMPORT int  Dequeue_IO_Interrupt_QLocked (IOINT* io,            const char* location);
CHAN_DLL_IMPORT void Update_IC_IOPENDING          ();
CHAN_DLL_IMPORT void Update_IC_IOPENDING_QLocked  ();

#define QUEUE_IO_INTERRUPT( io, clrbsy )          (void)Queue_IO_Interrupt( (IOINT*)(io), (U8)(clrbsy), PTT_LOC )
#define QUEUE_IO_INTERRUPT_QLOCKED( io, clrbsy )  (void)Queue_IO_Interrupt_QLocked( (IOINT*)(io), (U8)(clrbsy), PTT_LOC )
#define DEQUEUE_IO_INTERRUPT( io )                (int)Dequeue_IO_Interrupt( (IOINT*)(io), PTT_LOC )
#define DEQUEUE_IO_INTERRUPT_QLOCKED( io )        (int)Dequeue_IO_Interrupt_QLocked( (IOINT*)(io), PTT_LOC )
#define UPDATE_IC_IOPENDING()                     (void)Update_IC_IOPENDING()
#define UPDATE_IC_IOPENDING_QLOCKED()             (void)Update_IC_IOPENDING_QLocked()

/* Functions in module dat.c */

void s370_invalidate_aia( REGS* regs );
void s390_invalidate_aia( REGS* regs );
void z900_invalidate_aia( REGS* regs );

void s370_set_ic_mask( REGS* regs );
void s390_set_ic_mask( REGS* regs );
void z900_set_ic_mask( REGS* regs );

void s370_set_aea_mode( REGS* regs );
void s390_set_aea_mode( REGS* regs );
void z900_set_aea_mode( REGS* regs );

void s370_invalidate_guest_aia( REGS* regs );
void s390_invalidate_guest_aia( REGS* regs );
void z900_invalidate_guest_aia( REGS* regs );

void s370_set_guest_ic_mask( REGS* regs );
void s390_set_guest_ic_mask( REGS* regs );
void z900_set_guest_ic_mask( REGS* regs );

void s370_set_guest_aea_mode( REGS* regs );
void s390_set_guest_aea_mode( REGS* regs );
void z900_set_guest_aea_mode( REGS* regs );

void s370_set_aea_common( REGS* regs );
void s390_set_aea_common( REGS* regs );
void z900_set_aea_common( REGS* regs );

void s370_set_guest_aea_common( REGS* regs );
void s390_set_guest_aea_common( REGS* regs );
void z900_set_guest_aea_common( REGS* regs );

void s370_do_purge_tlb( REGS* regs );
void s390_do_purge_tlb( REGS* regs );
void z900_do_purge_tlb( REGS* regs );

void s370_purge_tlb( REGS* regs );
void s390_purge_tlb( REGS* regs );
void z900_purge_tlb( REGS* regs );

void s390_do_purge_alb( REGS* regs );
void z900_do_purge_alb( REGS* regs );

void s390_purge_alb( REGS* regs );
void z900_purge_alb( REGS* regs );

void s370_do_invalidate_tlb( REGS* regs, BYTE mask );
void s390_do_invalidate_tlb( REGS* regs, BYTE mask );
void z900_do_invalidate_tlb( REGS* regs, BYTE mask );

bool s370_is_tlbe_match( REGS* regs, REGS* host_regs, U64 pfra, int i );
bool s390_is_tlbe_match( REGS* regs, REGS* host_regs, U64 pfra, int i );
bool z900_is_tlbe_match( REGS* regs, REGS* host_regs, U64 pfra, int i );

void s370_do_purge_tlbe( REGS* regs, REGS* host_regs, U64 pfra );
void s390_do_purge_tlbe( REGS* regs, REGS* host_regs, U64 pfra );
void z900_do_purge_tlbe( REGS* regs, REGS* host_regs, U64 pfra );

void s370_purge_tlbe( REGS* regs, U64 pfra );
void s390_purge_tlbe( REGS* regs, U64 pfra );
void z900_purge_tlbe( REGS* regs, U64 pfra );

void s370_do_invalidate_tlbe( REGS* regs, BYTE* main );
void s390_do_invalidate_tlbe( REGS* regs, BYTE* main );
void z900_do_invalidate_tlbe( REGS* regs, BYTE* main );

void s370_invalidate_tlbe( REGS* regs, BYTE* main );
void s390_invalidate_tlbe( REGS* regs, BYTE* main );
void z900_invalidate_tlbe( REGS* regs, BYTE* main );

RADR apply_host_prefixing( REGS* regs, RADR raddr );

CPU_DLL_IMPORT void (ATTR_REGPARM(2) s370_program_interrupt)( REGS* regs, int code );
CPU_DLL_IMPORT void (ATTR_REGPARM(2) s390_program_interrupt)( REGS* regs, int code );
CPU_DLL_IMPORT void (ATTR_REGPARM(2) z900_program_interrupt)( REGS* regs, int code );

void s370_display_inst( REGS* iregs, BYTE* inst );
void s390_display_inst( REGS* iregs, BYTE* inst );
void z900_display_inst( REGS* iregs, BYTE* inst );

void s370_display_guest_inst( REGS* iregs, BYTE* inst );
void s390_display_guest_inst( REGS* iregs, BYTE* inst );
void z900_display_guest_inst( REGS* iregs, BYTE* inst );

void s370_update_psw_ia( REGS* regs, int n );
void s390_update_psw_ia( REGS* regs, int n );
void z900_update_psw_ia( REGS* regs, int n );

void s370_update_guest_psw_ia( REGS* regs, int n );
void s390_update_guest_psw_ia( REGS* regs, int n );
void z900_update_guest_psw_ia( REGS* regs, int n );

#endif // _HEXTERNS_H
