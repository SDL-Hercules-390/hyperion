/* HEXTERNS.H   (C) Copyright Roger Bowler, 1999-2012                */
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

#if defined( _MSC_VER ) && (_MSC_VER < VS2005)
//  '_ftol'   is defined in MSVCRT.DLL
//  '_ftol2'  we define ourselves in "w32ftol2.c"
extern long _ftol ( double dblSource );
extern long _ftol2( double dblSource );
#endif

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

/* Global data areas in module config.c                              */
HSYS_DLL_IMPORT SYSBLK   sysblk;                /* System control block      */
CCKD_DLL_IMPORT CCKDBLK  cckdblk;               /* CCKD global block         */

#ifdef EXTERNALGUI
HSYS_DLL_IMPORT int extgui;             // __attribute__ ((deprecated));
/* The external gui interface is now external and now uses the
   HDC(debug_cpu_state, regs) interface */
#endif /*EXTERNALGUI*/

/* Functions in module bldcfg.c */
int build_config (const char *fname);
BLDC_DLL_IMPORT const char* init_sysblk_netdev();

/* Functions in module script.c */
SCRI_DLL_IMPORT int process_config (const char *fname);
SCRI_DLL_IMPORT int parse_args (char* p, int maxargc, char** pargv, int* pargc);

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
int  configure_storage(U64);
int  configure_xstorage(U64);

int  configure_herc_priority(int prio);
int  configure_cpu_priority(int prio);
int  configure_dev_priority(int prio);
int  configure_tod_priority(int prio);
int  configure_srv_priority(int prio);

int  configure_shrdport(U16 shrdport);
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
CONF_DLL_IMPORT BYTE is_diag_instr();
CONF_DLL_IMPORT BYTE are_cpu_thread( int* cpunum );

/* Functions in module archlvl.c */
extern void init_default_archmode();
extern bool init_facilities_lists();
extern void init_cpu_facilities( REGS* regs );
extern const char* get_arch_name( REGS* regs );

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

/* Functions in module hao.c (Hercules Automatic Operator) */
#if defined(OPTION_HAO)
HAO_DLL_IMPORT void hao_command(char *command); /* process hao command */
#endif /* defined(OPTION_HAO) */

/* Functions in module hsccmd.c (so PTT debugging patches can access them) */
HCMD_DLL_IMPORT const char* ptyp2long ( BYTE ptyp );       // diag224_call()
HCMD_DLL_IMPORT const char* ptyp2short( BYTE ptyp );       // PTYPSTR()
HCMD_DLL_IMPORT BYTE short2ptyp( const char* shortname );  // engines_cmd()
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
extern BYTE setAllCpuIds     ( const S32 model, const S16 version, const S32 serial, const S32 MCEL );
extern BYTE setAllCpuIds_lock( const S32 model, const S16 version, const S32 serial, const S32 MCEL );
extern void setCpuIdregs     ( REGS* regs,
                               S32 arg_model, S16 arg_version, S32 arg_serial, S32 arg_MCEL );
extern void setCpuId         ( const unsigned int cpu,
                               S32 arg_model, S16 arg_version, S32 arg_serial, S32 arg_MCEL );
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

#if defined( OPTION_DYNAMIC_LOAD )

HSYS_DLL_IMPORT void  (*panel_display)  ();
HPAN_DLL_IMPORT void    panel_display_r ();
HSYS_DLL_IMPORT void* (*panel_command)  ( void* cmdline );
CMDT_DLL_IMPORT void*   panel_command_r ( void* cmdline );
HSYS_DLL_IMPORT void* (*replace_opcode) ( int arch, INSTR_FUNC inst, int opcode1, int opcode2 );
OPCD_DLL_IMPORT void*   replace_opcode_r( int arch, INSTR_FUNC inst, int opcode1, int opcode2 );
HSYS_DLL_IMPORT int   (*system_command) ( int argc, char* argv[], char* cmdline );
HSYS_DLL_IMPORT void  (*daemon_task)    ();

HSYS_DLL_IMPORT void* (*debug_device_state)         (DEVBLK *);
HSYS_DLL_IMPORT void* (*debug_cpu_state)            (REGS *);
HSYS_DLL_IMPORT void* (*debug_cd_cmd)               (char *);
HSYS_DLL_IMPORT void* (*debug_watchdog_signal)      (REGS *);
HSYS_DLL_IMPORT void* (*debug_program_interrupt)    (REGS *, int);
HSYS_DLL_IMPORT void* (*debug_diagnose)             (U32, int,  int, REGS *);
HSYS_DLL_IMPORT void* (*debug_iucv)                 (int, VADR, REGS *);
HSYS_DLL_IMPORT void* (*debug_sclp_unknown_command) (U32,    void *, REGS *);
HSYS_DLL_IMPORT void* (*debug_sclp_unknown_event)   (void *, void *, REGS *);
HSYS_DLL_IMPORT void* (*debug_sclp_unknown_event_mask) (void *, void *, REGS *);
HSYS_DLL_IMPORT void* (*debug_chsc_unknown_request) (void *, void *, REGS *);
HSYS_DLL_IMPORT void* (*debug_sclp_event_data)      (void *, void *, REGS *);

#else // !defined( OPTION_DYNAMIC_LOAD )

#define system_command                  NULL
#define daemon_task                     NULL

void  panel_display ();
void* panel_command ( void* cmdline );
void* replace_opcode( int arch, const INSTR_FUNC inst, int opcode1, int opcode2 );

#define debug_cpu_state                 NULL
#define debug_cd_cmd                    NULL
#define debug_device_state              NULL
#define debug_program_interrupt         NULL
#define debug_diagnose                  NULL
#define debug_iucv                      NULL
#define debug_sclp_unknown_command      NULL
#define debug_sclp_unknown_event        NULL
#define debug_sclp_event_data           NULL
#define debug_chsc_unknown_request      NULL
#define debug_watchdog_signal           NULL

#endif // defined( OPTION_DYNAMIC_LOAD )

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
LOADPARM_DLL_IMPORT char **str_model();
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
int quit_cmd(int argc, char *argv[],char *cmdline);
IMPL_DLL_IMPORT void system_cleanup(void);
typedef void (*LOGCALLBACK)( const char*, size_t );
typedef void *(*COMMANDHANDLER)(void *);
IMPL_DLL_IMPORT void registerLogCallback(LOGCALLBACK);
IMPL_DLL_IMPORT COMMANDHANDLER getCommandHandler(void);

/* Functions in module timer.c */
void *timer_update_thread (void *argp);

/* Functions in module clock.c */
void update_TOD_clock (void);
int configure_epoch(int);
int configure_yroffset(int);
int configure_tzoffset(int);

/* Functions in module service.c */
int scp_command (char *command, int priomsg, int echo);
int can_signal_quiesce ();
int can_send_command ();
int signal_quiesce (U16 count, BYTE unit);
void sclp_attention(U16 type);
void sclp_reset();
SERV_DLL_IMPORT void sclp_sysg_attention();
int servc_hsuspend(void *file);
int servc_hresume(void *file);

/* Functions in module ckddasd.c */
void ckd_build_sense ( DEVBLK *, BYTE, BYTE, BYTE, BYTE, BYTE);
int ckddasd_init_handler ( DEVBLK *dev, int argc, char *argv[]);
void ckddasd_execute_ccw ( DEVBLK *dev, BYTE code, BYTE flags,
        BYTE chained, U32 count, BYTE prevcode, int ccwseq,
        BYTE *iobuf, BYTE *more, BYTE *unitstat, U32 *residual );
int ckddasd_close_device ( DEVBLK *dev );
void ckddasd_query_device (DEVBLK *dev, char **devclass,
                int buflen, char *buffer);
int ckddasd_hsuspend ( DEVBLK *dev, void *file );
int ckddasd_hresume  ( DEVBLK *dev, void *file );

/* Functions in module fbadasd.c */
FBA_DLL_IMPORT void fbadasd_syncblk_io (DEVBLK *dev, BYTE type, int blknum,
        int blksize, BYTE *iobuf, BYTE *unitstat, U32 *residual);
FBA_DLL_IMPORT void fbadasd_read_block
      ( DEVBLK *dev, int blknum, int blksize, int blkfactor,
        BYTE *iobuf, BYTE *unitstat, U32 *residual );
FBA_DLL_IMPORT void fbadasd_write_block
      ( DEVBLK *dev, int blknum, int blksize, int blkfactor,
        BYTE *iobuf, BYTE *unitstat, U32 *residual );
int fbadasd_init_handler ( DEVBLK *dev, int argc, char *argv[]);
void fbadasd_execute_ccw ( DEVBLK *dev, BYTE code, BYTE flags,
        BYTE chained, U32 count, BYTE prevcode, int ccwseq,
        BYTE *iobuf, BYTE *more, BYTE *unitstat, U32 *residual );
int fbadasd_close_device ( DEVBLK *dev );
void fbadasd_query_device (DEVBLK *dev, char **devclass,
                int buflen, char *buffer);
int fbadasd_hsuspend ( DEVBLK *dev, void *file );
int fbadasd_hresume  ( DEVBLK *dev, void *file );

/* Functions in module cckddasd.c */
DEVIF   cckddasd_init_handler;
int     cckddasd_close_device (DEVBLK *);
int     cckd_read_track (DEVBLK *, int, BYTE *);
int     cckd_update_track (DEVBLK *, int, int, BYTE *, int, BYTE *);
int     cfba_read_block (DEVBLK *, int, BYTE *);
int     cfba_write_block (DEVBLK *, int, int, BYTE *, int, BYTE *);
CCKD_DLL_IMPORT void   *cckd_sf_add (void *);
CCKD_DLL_IMPORT void   *cckd_sf_remove (void *);
CCKD_DLL_IMPORT void   *cckd_sf_stats (void *);
CCKD_DLL_IMPORT void   *cckd_sf_comp (void *);
CCKD_DLL_IMPORT void   *cckd_sf_chk (void *);
CCKD_DLL_IMPORT int     cckd_command(char *, int);
CCKD_DLL_IMPORT void    cckd_print_itrace ();
CCKD_DLL_IMPORT void    cckd_sf_parse_sfn( DEVBLK* dev, char* sfn );

/* Functions in module cckdutil.c */
CCDU_DLL_IMPORT int     cckd_swapend (DEVBLK *);
CCDU_DLL_IMPORT void    cckd_swapend_chdr (CCKD_DEVHDR *);
CCDU_DLL_IMPORT void    cckd_swapend_l1 (CCKD_L1ENT *, int);
CCDU_DLL_IMPORT void    cckd_swapend_l2 (CCKD_L2ENT *);
CCDU_DLL_IMPORT void    cckd_swapend_free (CCKD_FREEBLK *);
CCDU_DLL_IMPORT void    cckd_swapend4 (char *);
CCDU_DLL_IMPORT void    cckd_swapend2 (char *);
CCDU_DLL_IMPORT int     cckd_endian ();
CCDU_DLL_IMPORT int     cckd_comp (DEVBLK *);
CCDU_DLL_IMPORT int     cckd_chkdsk (DEVBLK *, int);
CCDU_DLL_IMPORT void    cckdumsg (DEVBLK *, int, char *, ...) ATTR_PRINTF(3,4);

/* Functions in module hscmisc.c */
int herc_system (char* command);
void do_shutdown();
void stop_all_cpus();
void stop_all_cpus_intlock_held();
bool are_all_cpus_stopped();
bool are_all_cpus_stopped_intlock_held();
bool are_any_cpus_started();
bool are_any_cpus_started_intlock_held();

int display_gregs (REGS *regs, char *buf, int buflen, char *hdr);
int display_fregs (REGS *regs, char *buf, int buflen,char *hdr);
int display_cregs (REGS *regs, char *buf, int buflen, char *hdr);
int display_aregs (REGS *regs, char *buf, int buflen, char *hdr);
int display_subchannel (DEVBLK *dev, char *buf, int buflen, char *hdr);
const char* FormatCRW( U32  crw, char* buf, size_t bufsz );
const char* FormatORB( ORB* orb, char* buf, size_t bufsz );
const char* FormatSCL( ESW* esw, char* buf, size_t bufsz );
const char* FormatERW( ESW* esw, char* buf, size_t bufsz );
const char* FormatESW( ESW* esw, char* buf, size_t bufsz );
HMISC_DLL_IMPORT const char* FormatSID( BYTE* iobuf, int num, char* buf, size_t bufsz );
HMISC_DLL_IMPORT const char* FormatRCD( BYTE* iobuf, int num, char* buf, size_t bufsz );
HMISC_DLL_IMPORT const char* FormatRNI( BYTE* iobuf, int num, char* buf, size_t bufsz );
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

/* Functions in ecpsvm.c that are not *direct* instructions */
/* but support functions either used by other instruction   */
/* functions or from somewhere else                         */
#ifdef FEATURE_ECPSVM
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
HSYS_DLL_IMPORT void (*debug_tt32_tracing) (int);
#endif // defined(OPTION_W32_CTCI)

/* Function in crypto.c */
#if defined( _FEATURE_076_MSA_EXTENSION_FACILITY_3 )
void renew_wrapping_keys(void);
#endif

/* Function in getopt.c */
GOP_DLL_IMPORT int   opterr;    /* if error message should be printed */
GOP_DLL_IMPORT int   optind;    /* index into parent argv vector */
GOP_DLL_IMPORT int   optopt;    /* character checked for validity */
GOP_DLL_IMPORT int   optreset;  /* reset getopt */
GOP_DLL_IMPORT char* optarg;    /* argument associated with option */
GOP_DLL_IMPORT int   getopt      ( int nargc, char * const *nargv, const char *options );
GOP_DLL_IMPORT int   getopt_long ( int nargc, char * const *nargv, const char *options, const struct option *long_options, int *idx );

/* Function in channel.c */
                void shared_iowait (DEVBLK *dev);
CHAN_DLL_IMPORT int  device_attention (DEVBLK *dev, BYTE unitstat);
CHAN_DLL_IMPORT int  ARCH_DEP(device_attention) (DEVBLK *dev, BYTE unitstat);

CHAN_DLL_IMPORT void Queue_IO_Interrupt           (IOINT* io, U8 clrbsy);
CHAN_DLL_IMPORT void Queue_IO_Interrupt_QLocked   (IOINT* io, U8 clrbsy);
CHAN_DLL_IMPORT int  Dequeue_IO_Interrupt         (IOINT* io);
CHAN_DLL_IMPORT int  Dequeue_IO_Interrupt_QLocked (IOINT* io);
CHAN_DLL_IMPORT void Update_IC_IOPENDING          ();
CHAN_DLL_IMPORT void Update_IC_IOPENDING_QLocked  ();

#define QUEUE_IO_INTERRUPT              Queue_IO_Interrupt
#define QUEUE_IO_INTERRUPT_QLOCKED      Queue_IO_Interrupt_QLocked
#define DEQUEUE_IO_INTERRUPT            Dequeue_IO_Interrupt
#define DEQUEUE_IO_INTERRUPT_QLOCKED    Dequeue_IO_Interrupt_QLocked
#define UPDATE_IC_IOPENDING             Update_IC_IOPENDING
#define UPDATE_IC_IOPENDING_QLOCKED     Update_IC_IOPENDING_QLocked

#endif // _HEXTERNS_H
