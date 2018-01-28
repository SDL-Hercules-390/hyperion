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

DLL_EXPORT SYSBLK  sysblk = {0};
DLL_EXPORT int     extgui =  0;

/*-------------------------------------------------------------------*/
/*         The following functions are called directly               */
/*-------------------------------------------------------------------*/

DLL_EXPORT PANDISP*       panel_display       = NULL;
DLL_EXPORT PANDISP*       daemon_task         = NULL;
DLL_EXPORT PANCMD*        panel_command       = NULL;
DLL_EXPORT SYSTEMCMD*     system_command      = NULL;
DLL_EXPORT REPOPCODE*     replace_opcode      = NULL;

#if defined( OPTION_W32_CTCI )
DLL_EXPORT DBGT32ST*      debug_tt32_stats    = NULL;
DLL_EXPORT DBGT32TR*      debug_tt32_tracing  = NULL;
#endif

/*-------------------------------------------------------------------*/
/*   The following are called via the "HDC1", "HDC2", etc, macros    */
/*-------------------------------------------------------------------*/

DLL_EXPORT HDLDBGCD*      debug_cd_cmd                   = NULL;
DLL_EXPORT HDLDBGCPU*     debug_cpu_state                = NULL;
DLL_EXPORT HDLDBGCPU*     debug_watchdog_signal          = NULL;
DLL_EXPORT HDLDBGPGMI*    debug_program_interrupt        = NULL;
DLL_EXPORT HDLDBGDIAG*    debug_diagnose                 = NULL;

DLL_EXPORT HDLDBGSCLPUC*  debug_sclp_unknown_command     = NULL;
DLL_EXPORT HDLDBGSCLPUE*  debug_sclp_unknown_event       = NULL;
DLL_EXPORT HDLDBGSCLPUE*  debug_sclp_unknown_event_mask  = NULL;
DLL_EXPORT HDLDBGSCLPUE*  debug_sclp_event_data          = NULL;
DLL_EXPORT HDLDBGSCLPUE*  debug_chsc_unknown_request     = NULL;

/*-------------------------------------------------------------------*/
