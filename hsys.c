/* HSYS.C       (C) Copyright Roger Bowler, 1999-2012                */
/*              Hercules hsys Header                                 */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#include "hstdinc.h"

#define  _HSYS_C_
#define  _HSYS_DLL_

#include "hercules.h"

/*-------------------------------------------------------------------*/
/*               Hercules Master SYSBLK structure                    */
/*-------------------------------------------------------------------*/
DLL_EXPORT SYSBLK  sysblk  = {0};   /* Hercules-wide SYSBLK struct   */
DLL_EXPORT int     extgui  =  0;    /* 1 == External GUI is active   */

/*-------------------------------------------------------------------*/
/*                      PROGRAMMING NOTE                             */
/*                                                                   */
/* The panel_display, panel_command and replace_opcode function      */
/* pointers are initialized to point to their corresponding "real"   */
/* functions (the_real_panel_display, the_real_panel_command and     */
/* the_real_replace_opcode) by impl.c passing their addresses to     */
/* hdl.c's "hdl_main" function which inits them to that value.       */
/*                                                                   */
/* The other function pointers remain NULL until an HDL module is    */
/* loaded that registers an override for them (which it can also     */
/* do for any of the other functions as well, such as panel_display  */
/* for example; see e.g. dyngui.c's HDL_REGISTER_SECTION)            */
/*                                                                   */
/* Before calling any of the below functions (other than the three   */
/* already mentioned) you should check to see if it actually points  */
/* somewhere by checking to see if it is NULL or not.                */
/*                                                                   */
/* The remaining function pointers (e.g. debug_cpu_state, etc) are   */
/* optional "hook" functions which should be called via one of the   */
/* "HDC1", "HDC2", etc, macros which automatically check for NULL.   */
/*-------------------------------------------------------------------*/

DLL_EXPORT  PANDISP*       daemon_task     = NULL;
DLL_EXPORT  PANDISP*       panel_display   = NULL;    /* (see above) */
DLL_EXPORT  PANCMD*        panel_command   = NULL;    /* (see above) */
DLL_EXPORT  REPOPCODE*     replace_opcode  = NULL;    /* (see above) */
DLL_EXPORT  SYSTEMCMD*     system_command  = NULL;

#if defined( OPTION_W32_CTCI )
DLL_EXPORT  DBGT32ST*      debug_tt32_stats    = NULL;
DLL_EXPORT  DBGT32TR*      debug_tt32_tracing  = NULL;
#endif

/*-------------------------------------------------------------------*/

DLL_EXPORT  HDLDBGCD*      debug_cd_cmd                   = NULL;
DLL_EXPORT  HDLDBGCPU*     debug_cpu_state                = NULL;
DLL_EXPORT  HDLDBGCPU*     debug_watchdog_signal          = NULL;
DLL_EXPORT  HDLDBGPGMI*    debug_program_interrupt        = NULL;
DLL_EXPORT  HDLDBGDIAG*    debug_diagnose                 = NULL;

DLL_EXPORT  HDLDBGSCLPUC*  debug_sclp_unknown_command     = NULL;
DLL_EXPORT  HDLDBGSCLPUE*  debug_sclp_unknown_event       = NULL;
DLL_EXPORT  HDLDBGSCLPUE*  debug_sclp_unknown_event_mask  = NULL;
DLL_EXPORT  HDLDBGSCLPUE*  debug_sclp_event_data          = NULL;
DLL_EXPORT  HDLDBGSCLPUE*  debug_chsc_unknown_request     = NULL;

/*-------------------------------------------------------------------*/
