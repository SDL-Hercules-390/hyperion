/* HSYS.C       (C) Copyright Roger Bowler, 1999-2012                */
/*              Hercules hsys Header                                 */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#include "hstdinc.h"

#define _HSYS_C_

#include "hercules.h"


DLL_EXPORT SYSBLK sysblk;


#if defined(EXTERNALGUI)
DLL_EXPORT int extgui = 0;
#endif

#if defined(OPTION_W32_CTCI)
DLL_EXPORT int (*debug_tt32_stats)(int) = NULL;
DLL_EXPORT void (*debug_tt32_tracing)(int) = NULL;
#endif
#if defined(OPTION_DYNAMIC_LOAD)

DLL_EXPORT void *(*panel_command)   (void *);
DLL_EXPORT void  (*panel_display)   (void);
DLL_EXPORT void *(*replace_opcode)  (int arch, INSTR_FUNC inst, int opcode1, int opcode2);
DLL_EXPORT void  (*daemon_task)     (void);
DLL_EXPORT int   (*system_command)  (int argc, char *argv[], char *cmdline);
DLL_EXPORT void *(*debug_cpu_state) (REGS *);
DLL_EXPORT void *(*debug_cd_cmd)    (char *);
DLL_EXPORT void *(*debug_device_state) (DEVBLK *);
DLL_EXPORT void *(*debug_program_interrupt) (REGS *, int);
DLL_EXPORT void *(*debug_diagnose) (U32, int, int, REGS *);
DLL_EXPORT void *(*debug_iucv) (int, VADR, REGS *);
DLL_EXPORT void *(*debug_sclp_unknown_command) (U32, void *, REGS *);
DLL_EXPORT void *(*debug_sclp_unknown_event) (void *, void *, REGS *);
DLL_EXPORT void *(*debug_sclp_unknown_event_mask) (void *, void *, REGS *);
DLL_EXPORT void *(*debug_sclp_event_data) (void *, void *, REGS *);
DLL_EXPORT void *(*debug_chsc_unknown_request) (void *, void *, REGS *);
DLL_EXPORT void *(*debug_watchdog_signal) (REGS *);

#endif
