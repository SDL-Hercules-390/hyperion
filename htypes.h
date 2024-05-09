/* HTYPES.H     (C) Copyright Roger Bowler, 1999-2016                */
/*              (C) and others 2017-2023                             */
/*              Hercules Type Definitions                            */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* Fixed size integer, boolean and Hercules types (int32_t, bool,    */
/* BOOL, U32, U64, FWORD, DBLWRD, etc). Try to pull in as many of    */
/* the values as possible from the available system headers.         */
/*-------------------------------------------------------------------*/

#ifndef _HTYPES_H_
#define _HTYPES_H_

#ifdef HAVE_CONFIG_H
  #include <config.h>
#endif

/*-------------------------------------------------------------------*/
/*         Standard fixed size integer and boolean types             */
/*-------------------------------------------------------------------*/

#if defined( HAVE_INTTYPES_H )
  #include <inttypes.h>
#elif defined( HAVE_STDINT_H )
  #include <stdint.h>
#else

  typedef char                  int8_t;
  typedef short                 int16_t;
  typedef int                   int32_t;
  typedef long long             int64_t;
  typedef unsigned char         uint8_t;
  typedef unsigned short        uint16_t;
  typedef unsigned int          uint32_t;
  typedef unsigned long long    uint64_t;
  typedef char                  int_fast8_t;
  typedef int                   int_fast16_t;
  typedef int                   int_fast32_t;
  typedef long long             int_fast64_t;
  typedef unsigned char         uint_least8_t;
  typedef unsigned char         uint_fast8_t;
  typedef unsigned int          uint_fast16_t;
  typedef unsigned int          uint_fast32_t;
  typedef unsigned long long    uint_fast64_t;

#endif

#ifndef int32_t
#define int32_t                 int32_t         /* (used by extpkgs) */
#endif

#ifndef HAVE_U_INT8_T
  typedef uint8_t               u_int8_t;
  typedef uint16_t              u_int16_t;
  typedef uint32_t              u_int32_t;
  typedef uint64_t              u_int64_t;
#endif

#ifndef   _BSDTYPES_DEFINED
  #define _BSDTYPES_DEFINED
  #ifndef HAVE_U_CHAR
    typedef unsigned char       u_char;
  #endif
  #ifndef HAVE_U_SHORT
    typedef unsigned short      u_short;
  #endif
  #ifndef HAVE_U_INT
    typedef unsigned int        u_int;
  #endif
  #ifndef HAVE_U_LONG
    typedef unsigned long       u_long;
  #endif
#endif

#ifdef _MSVC_
  typedef    int32_t            pid_t;
  typedef    int32_t            mode_t;
  typedef   uint32_t            in_addr_t;
  #ifndef   _SSIZE_T_DEFINED
    #define _SSIZE_T_DEFINED
    typedef  SSIZE_T            ssize_t;
  #endif
#endif

#ifdef HAVE_STDBOOL_H
  #include <stdbool.h>
#else
  #define                       _Bool    int
  #define bool                  _Bool
  #define true                  1
  #define false                 0
  #define __bool_true_false_are_defined  1
#endif

/*-------------------------------------------------------------------*/
/*              Hercules fixed size integer types                    */
/*-------------------------------------------------------------------*/

typedef  int8_t     S8;         // signed 8-bits
typedef  int16_t    S16;        // signed 16-bits
typedef  int32_t    S32;        // signed 32-bits
typedef  int64_t    S64;        // signed 64-bits

typedef  uint8_t    U8;         // unsigned 8-bits
typedef  uint16_t   U16;        // unsigned 16-bits
typedef  uint32_t   U32;        // unsigned 32-bits
typedef  uint64_t   U64;        // unsigned 64-bits

#ifndef  _MSVC_                 // (MSVC typedef's it too)
typedef  uint8_t    BYTE;       // unsigned byte       (1 byte)
#endif
typedef  uint8_t    HWORD[2];   // unsigned halfword   (2 bytes)
typedef  uint8_t    FWORD[4];   // unsigned fullword   (4 bytes)
typedef  uint8_t    DBLWRD[8];  // unsigned doubleword (8 bytes)
typedef  uint8_t    QWORD[16];  // unsigned quadword   (16 bytes)

#ifndef  BOOL
#define  BOOL       int         // (same as Windows)
#endif

#if defined( _MSVC_ )           // (some code needs the following)
  #if defined( _WIN64 )
    typedef unsigned __int64  U_LONG_PTR;   // name unique to Hercules
  #else // 32-bit x86
    typedef unsigned __int32  U_LONG_PTR;   // name unique to Hercules
  #endif
#else // Linux, etc
  typedef unsigned long       U_LONG_PTR;   // name unique to Hercules
#endif

/*-------------------------------------------------------------------*/
/*                       Socket stuff                                */
/*-------------------------------------------------------------------*/

#ifndef HAVE_SOCKLEN_T
  typedef  unsigned int     socklen_t;
#endif

#ifndef HAVE_IN_ADDR_T
  typedef  unsigned int     in_addr_t;
#endif

#ifndef HAVE_USECONDS_T
  typedef  unsigned int     useconds_t;
#endif

#if !defined( HAVE_STRUCT_IN_ADDR_S_ADDR ) && !defined( _WINSOCK_H )
  struct in_addr
  {
    in_addr_t  s_addr;
  };
#endif

  // (The following are simply to silence some compile time warnings)
#ifdef _MSVC_
  typedef  char               GETSET_SOCKOPT_T;
  typedef  const char *const *EXECV_ARG2_ARGV_T;
#else
  typedef  void               GETSET_SOCKOPT_T;
  typedef  char *const       *EXECV_ARG2_ARGV_T;
#endif

/*-------------------------------------------------------------------*/
/*                   Magnetic Tape stuff                             */
/*-------------------------------------------------------------------*/

#if defined( OPTION_SCSI_TAPE ) && !defined( HAVE_SYS_MTIO_H )
  struct mt_tape_info
  {
     long   t_type;         /* device type id (mt_type) */
     char  *t_name;         /* descriptive name */
  };
  #define MT_TAPE_INFO      { { 0, NULL } }
#endif

/*-------------------------------------------------------------------*/
/*            Primary Hercules Control Structures                    */
/*-------------------------------------------------------------------*/

typedef struct SYSBLK    SYSBLK;    // System configuration block
typedef struct REGS      REGS;      // CPU register context
typedef struct VFREGS    VFREGS;    // Vector Facility Registers
typedef struct ZPBLK     ZPBLK;     // Zone Parameter Block
typedef struct TELNET    TELNET;    // Telnet Control Block
typedef struct DEVBLK    DEVBLK;    // Device configuration block
typedef struct CHPBLK    CHPBLK;    // Channel Path config block
typedef struct IOINT     IOINT;     // I/O interrupt queue

typedef struct GSYSINFO  GSYSINFO;  // Ebcdic machine information

typedef struct DEVDATA   DEVDATA;   // xxxxxxxxx
typedef struct DEVGRP    DEVGRP;    // xxxxxxxxx
typedef struct DEVHND    DEVHND;    // xxxxxxxxx

typedef struct GUISTAT   GUISTAT;   // EXTERNALGUI Device Status Ctl

/*-------------------------------------------------------------------*/
/*      Secondary Device and I/O Control Related Structures          */
/*-------------------------------------------------------------------*/

typedef struct COMMADPT         COMMADPT;         // Comm Adapter
typedef struct bind_struct      bind_struct;      // Socket Device Ctl
typedef struct TCPNJE           TCPNJE;           // TCPNJE communications

typedef struct TAPEMEDIA_HANDLER  TAPEMEDIA_HANDLER;  // (see tapedev.h)
typedef struct TAPEAUTOLOADENTRY  TAPEAUTOLOADENTRY;  // (see tapedev.h)
typedef struct TAMDIR             TAMDIR;             // (see tapedev.h)

/*-------------------------------------------------------------------*/
/*             Device handler function prototypes                    */
/*-------------------------------------------------------------------*/

typedef int   DEVIF  (DEVBLK *dev, int argc, char *argv[]);
typedef void  DEVQF  (DEVBLK *dev, char **devclass, int buflen,
                                   char *buffer);
typedef void  DEVXF  (DEVBLK *dev, BYTE code, BYTE flags,
                                   BYTE chained, U32 count,
                                   BYTE prevcode, int ccwseq,
                                   BYTE *iobuf, BYTE *more,
                                   BYTE *unitstat, U32 *residual);
typedef void  DEVHF  (DEVBLK *dev);
typedef int   DEVCF  (DEVBLK *dev);
typedef void  DEVSF  (DEVBLK *dev);
typedef int   DEVRF  (DEVBLK *dev, int ix, BYTE *unitstat);
typedef int   DEVWF  (DEVBLK *dev, int rcd, int off, BYTE *buf,
                                   int len, BYTE *unitstat);
typedef int   DEVUF  (DEVBLK *dev);
typedef void  DEVRR  (DEVBLK *dev);
typedef int   DEVSA  (DEVBLK *dev, U32 qmask);
typedef int   DEVSAS (DEVBLK *dev, U32 oqmask, U32 iqmask);
typedef int   DEVQD  (DEVBLK *dev, void *desc);
typedef int   DEVSR  (DEVBLK *dev, void *file);

/*-------------------------------------------------------------------*/
/*           Device handler description structures                   */
/*-------------------------------------------------------------------*/
typedef BYTE *DEVIM;                    /* Immediate CCW Codes Table */

/*-------------------------------------------------------------------*/
/*              Read Configuration Data function                     */
/*-------------------------------------------------------------------*/
typedef int DEVRCD( DEVBLK *dev, BYTE *buffer, int bufsz );

/*-------------------------------------------------------------------*/
/*                Format Sense bytes function                        */
/*-------------------------------------------------------------------*/
typedef void DEVSNS( const DEVBLK* dev, char* buf, size_t bufsz );

/*-------------------------------------------------------------------*/
/*            typedefs for HDL overridable functions                 */
/*-------------------------------------------------------------------*/

typedef void (ATTR_REGPARM(2) *INSTR_FUNC)( BYTE inst[], REGS* regs );

/* The following functions are called directly */

typedef void   PANDISP      ();
typedef void*  PANCMD       ( char* command );
typedef int    SYSTEMCMD    ( int argc, char* argv[], char* cmdline );
typedef void*  REPOPCODE    ( int arch, INSTR_FUNC inst, int opcode1, int opcode2 );
typedef int    DBGT32ST     ( int fd );
typedef bool   DBGT32TR     ( int fd );

/* The following are called via the "HDC1", "HDC2", etc, macros */

typedef void*  HDLDBGCD     ( char* cwd );
typedef void*  HDLDBGCPU    ( REGS* regs );
typedef void*  HDLDBGPGMI   ( REGS* regs , int ruptcode );
typedef void*  HDLDBGDIAG   ( U32 code, int r1, int r2, REGS* regs );
typedef void*  HDLDBGSCLPUC ( U32 sclp_command, void* sccb, REGS* regs );
typedef void*  HDLDBGSCLPUE ( void* evd_hdr, void* sccb, REGS* regs );

/*-------------------------------------------------------------------*/
/*                          crypto                                   */
/*-------------------------------------------------------------------*/

#if defined( _WIN32 )
typedef BCRYPT_ALG_HANDLE   HRANDHAND;  /* secure random api handle  */
#else
typedef int                 HRANDHAND;  /* secure random api handle  */
#endif

/*-------------------------------------------------------------------*/
/*            Trace File helper function typedefs                    */
/*-------------------------------------------------------------------*/

typedef BYTE TFGSK( U64 );                  // get_storage_key
typedef S64  TFGCT( REGS* );                // get_cpu_timer
typedef void TFSIT( REGS* );                // store_int_timer
typedef int  TFVTR( U64*, int*, U64, int,   // virt_to_real
                    REGS*, int );

/*-------------------------------------------------------------------*/

#endif // _HTYPES_H_
