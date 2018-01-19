/* HDIAGF18.H   (C) Copyright, Harold Grovesteen, 2010-2012          */
/*              Hercules DIAGNOSE code X'F18'                        */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#if !defined(__HDIAGF18_H__)
#define __HDIAGF18_H__

/*-------------------------------------------------------------------*/
/*  DIAGNOSE X'F18' Capability Parameter Block (CPB)                 */
/*-------------------------------------------------------------------*/
typedef struct _CPB
    {
        BYTE    ipv6;         /* IPv6 Native Mode Socket Types       */
        BYTE    ipv4;         /* IPv4 Native Mode Socket Types       */
        BYTE    resources;    /* Resources and Modes Supported       */
        BYTE    platform;     /* Platform and Large File Support     */
        HWORD   comptopt;     /* Compatibility Operation Options     */
        HWORD   nativopt;     /* Native Operation Options            */
        HWORD   maxnshsk;     /* Maximum Guest Native Shadow Sockets */
        BYTE    r1;
        BYTE    r2;
        BYTE    r3;
        BYTE    r4;
        BYTE    r5;
        BYTE    r6;
    } CPB;

/* Socket Type Masks */
#define STREAM_SOCKET   0x01
#define DATAGRAM_SOCKET 0x02

/* Option related values */
#define DF18_VER     0x0001     /* Version of DIAGNOSE X'F18'        */
#define VER_MASK     0x000F     /* Mask for version in options       */
#define AMODE_MASK   0x0070     /* Mask for addressing options       */
#define OPTIONS_MASK 0xFFF0     /* Mask for options only             */
#define SPACE_MASK   0x0070
#define NATIVE_OPTIONS 0x0000
#define NATIVE_INVALID (~NATIVE_OPTIONS)
#define COMPAT_OPTIONS 0x0060
#define COMPAT_INVALID (~COMPAT_OPTIONS)
#define DF18_REAL_SPACE    0x0040
#define DF18_PRIMARY_SPACE 0x0020
#define DF18_AR_SPACE      0x0010

/*-------------------------------------------------------------------*/
/*  DIAGNOSE X'F18' Compatibility Mode Parameter Block (CMPB)        */
/*-------------------------------------------------------------------*/
/* Note: The same parameter block is used for socket functions and   */
/*       file operations.  It is always accessed directly from       */
/*       guest storage by the DIAGNOSE to maintain state for restart.*/
/*       Each of the elements of the array correspond to a guest     */
/*       register originally used by NFILE and TCPIP to manage state */
/*       and access guest parameters supplied in registers. This     */
/*       design allows a program converting from NFILE or TCPIP      */
/*       usage to simply use the CMPB as a register save area.       */

typedef struct _CMPB
    {
        FWORD ps_regs[16];     /* Pseudo registers */
    } CMPB;


#endif /* !defined(__HDIAGF18_H__) */
