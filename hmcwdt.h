/* HMCWDT.H    (C) Copyright Jan Jaeger, 1999-2012                   */
/*              (C) Copyright Roger Bowler, 1999-2012                */
/*                                                                   */
/*              Watchdog Timer - Diagnose 0x288                      */
/*            (Control Virtual Machine Time Bomb)                    */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#ifndef _HMCWDT_H_
#define _HMCWDT_H_

/*-----------------------------------------------------------*/
/*  Diagnose 288: Watchdog Timer  support                    */
/*-----------------------------------------------------------*/

/* Diag 288 : watchdog timer headers                         */
/*-----------------------------------------------------------*/
/* Start of headers and defines copied from                  */
/* https://github.com/torvalds/linux                         */
/*      file 'arch/s390/include/asm/diag288.h' and           */
/*      file 'drivers/watchdog/diag288_wdt.c                 */
/* The copied headers have been modified                     */
/*-----------------------------------------------------------*/

/* constants */
#define HMCWDT_MIN_INTERVAL 15	    /* Minimal time supported by diag288 */
#define HMCWDT_MAX_INTERVAL 3600    /* One hour should be enough - pure estimation */

#define HMCWDT_MAX_CMDLEN   240     /* Max length of cmds string */

#define HMCWDT_DEFAULT_TIMEOUT 30
#define HMCWDT_US_SLEEP     250000

/* Diag 0x288 Function codes - open, reset, cancel */
#define HMCWDT_FUNC_OPEN    0
#define HMCWDT_FUNC_RESET   1
#define HMCWDT_FUNC_CLOSE  2

/* Helpers */
#define HMCWDT_IS_DISABLED          ( sysblk.hmcwdt_enabled == 0 )
#define HMCWDT_IS_ENABLED           ( sysblk.hmcwdt_enabled == 1 )
#define HMCWDT_IS_ENABLED_INACTIVE  ( sysblk.hmcwdt_enabled == 1 && sysblk.hmcwdt_active == 0 )
#define HMCWDT_IS_ENABLED_ACTIVE    ( sysblk.hmcwdt_enabled == 1 && sysblk.hmcwdt_active == 1 )
#define HMCWDT_IS_SHUTDOWN          ( sysblk.hmcwdt_canceled == 1 )
#define HMCWDT_STATE_STR            ( HMCWDT_IS_ENABLED_ACTIVE ? "enabled-active"    :               \
                                      HMCWDT_IS_ENABLED        ? "enabled-inactive"  : "disabled"    \
                                    )

#define OBTAIN_HMCWDT_LOCK()        obtain_lock(  &sysblk.hmcwdt_lock )
#define RELEASE_HMCWDT_LOCK()       release_lock( &sysblk.hmcwdt_lock )

/* Functions in module hmcwdt.c */
/* ---------------------------- */
/* used in Diag 0x288 and       */
/* used in hsccmd.c for hmcwdt  */
/* ---------------------------- */
int hmcwdt_diag288_open( U32 timeout );
int hmcwdt_diag288_reset( U32 timeout );
int hmcwdt_diag288_close( U32 timeout );

U64 hmcwdt_get_expire_time( U32 timeout );
U32 hmcwdt_get_timeout( );
int hmcwdt_set_cmdsep( char );
int hmcwdt_set_disabled( );
int hmcwdt_set_enabled( );
int hmcwdt_set_cmds( char* );
int hmcwdt_show_status( );
int hmcwdt_test_expire( U32 timeout );

#endif /* _HMCWDT_H_ */
