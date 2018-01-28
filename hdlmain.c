/* HDLMAIN.C    (C) Copyright Jan Jaeger, 2003-2012                  */
/*              Hercules Dynamic Loader                              */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#include "hstdinc.h"

#define _HDLMAIN_C_
#define _HERCULES_EXE_

#include "hercules.h"
#include "httpmisc.h"

/*-------------------------------------------------------------------*/

#if !defined( _GEN_ARCH )

  #if defined(              _ARCH_NUM_1 )
    #define   _GEN_ARCH     _ARCH_NUM_1
    #include "hdlmain.c"
  #endif

  #if defined(              _ARCH_NUM_2 )
    #undef    _GEN_ARCH
    #define   _GEN_ARCH     _ARCH_NUM_2
    #include "hdlmain.c"
  #endif

/*-------------------------------------------------------------------*/

HDL_DEPENDENCY_SECTION;
{
     HDL_DEPENDENCY( HERCULES );
     HDL_DEPENDENCY( REGS     );
     HDL_DEPENDENCY( DEVBLK   );
     HDL_DEPENDENCY( SYSBLK   );
     HDL_DEPENDENCY( WEBBLK   );
}
END_DEPENDENCY_SECTION;

/*-------------------------------------------------------------------*/

static void** unresolved = NULL;
#define UNRESOLVED *unresolved

/*-------------------------------------------------------------------*/

HDL_REGISTER_SECTION;
{
    HDL_REGISTER( panel_display,                 panel_display_r  );
    HDL_REGISTER( panel_command,                 panel_command_r  );
    HDL_REGISTER( replace_opcode,                replace_opcode_r );
    HDL_REGISTER( parse_args,                    parse_args       );

    HDL_REGISTER( daemon_task,                   UNRESOLVED       );
    HDL_REGISTER( system_command,                UNRESOLVED       );
    HDL_REGISTER( hdl_devequ,                    UNRESOLVED       );

    HDL_REGISTER( debug_cpu_state,               UNRESOLVED       );
    HDL_REGISTER( debug_cd_cmd,                  UNRESOLVED       );
    HDL_REGISTER( debug_device_state,            UNRESOLVED       );
    HDL_REGISTER( debug_program_interrupt,       UNRESOLVED       );
    HDL_REGISTER( debug_diagnose,                UNRESOLVED       );
    HDL_REGISTER( debug_sclp_unknown_command,    UNRESOLVED       );
    HDL_REGISTER( debug_sclp_unknown_event,      UNRESOLVED       );
    HDL_REGISTER( debug_sclp_unknown_event_mask, UNRESOLVED       );
    HDL_REGISTER( debug_sclp_event_data,         UNRESOLVED       );
    HDL_REGISTER( debug_chsc_unknown_request,    UNRESOLVED       );
    HDL_REGISTER( debug_watchdog_signal,         UNRESOLVED       );

#if defined( OPTION_W32_CTCI )

    HDL_REGISTER( debug_tt32_stats,              UNRESOLVED       );
    HDL_REGISTER( debug_tt32_tracing,            UNRESOLVED       );
#endif
}
END_REGISTER_SECTION;

/*-------------------------------------------------------------------*/

HDL_RESOLVER_SECTION;
{
    HDL_RESOLVE( panel_display                 );
    HDL_RESOLVE( daemon_task                   );
    HDL_RESOLVE( panel_command                 );
    HDL_RESOLVE( system_command                );
    HDL_RESOLVE( replace_opcode                );
    HDL_RESOLVE( hdl_devequ                    );

    HDL_RESOLVE( debug_cpu_state               );
    HDL_RESOLVE( debug_cd_cmd                  );
    HDL_RESOLVE( debug_device_state            );
    HDL_RESOLVE( debug_program_interrupt       );
    HDL_RESOLVE( debug_diagnose                );
    HDL_RESOLVE( debug_sclp_unknown_command    );
    HDL_RESOLVE( debug_sclp_unknown_event      );
    HDL_RESOLVE( debug_sclp_unknown_event_mask );
    HDL_RESOLVE( debug_sclp_event_data         );
    HDL_RESOLVE( debug_chsc_unknown_request    );

#if defined( OPTION_W32_CTCI )

    HDL_RESOLVE( debug_tt32_stats              );
    HDL_RESOLVE( debug_tt32_tracing            );
#endif
}
END_RESOLVER_SECTION;

/*-------------------------------------------------------------------*/

HDL_DEVICE_SECTION;
{
    HDL_DEVICE ( 2305, ckddasd_device_hndinfo );
    HDL_DEVICE ( 2311, ckddasd_device_hndinfo );
    HDL_DEVICE ( 2314, ckddasd_device_hndinfo );
    HDL_DEVICE ( 3330, ckddasd_device_hndinfo );
    HDL_DEVICE ( 3340, ckddasd_device_hndinfo );
    HDL_DEVICE ( 3350, ckddasd_device_hndinfo );
    HDL_DEVICE ( 3375, ckddasd_device_hndinfo );
    HDL_DEVICE ( 3380, ckddasd_device_hndinfo );
    HDL_DEVICE ( 3390, ckddasd_device_hndinfo );
    HDL_DEVICE ( 9345, ckddasd_device_hndinfo );

    HDL_DEVICE ( 0671, fbadasd_device_hndinfo );
    HDL_DEVICE ( 3310, fbadasd_device_hndinfo );
    HDL_DEVICE ( 3370, fbadasd_device_hndinfo );
    HDL_DEVICE ( 9313, fbadasd_device_hndinfo );
    HDL_DEVICE ( 9332, fbadasd_device_hndinfo );
    HDL_DEVICE ( 9335, fbadasd_device_hndinfo );
    HDL_DEVICE ( 9336, fbadasd_device_hndinfo );
}
END_DEVICE_SECTION;

/*-------------------------------------------------------------------*/

#endif /* !defined( _GEN_ARCH ) */
