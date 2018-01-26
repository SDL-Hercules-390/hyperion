/* S37XMOD.C (C) Copyright Ivan S. Warren 2010                       */
/*            Optional S370 Extensions                               */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#include "hstdinc.h"
#include "hercules.h"
#include "opcode.h"

#if defined(_370)

/* This file is only a STUB to separate the module part of s37x
   from the functional code.

   This is necessary because the final location of the functional
   code may depend on the host operating system capabilities.

   The actual functional code resides in s37x.c
*/

#if defined( OPTION_370_EXTENSION )

DLL_IMPORT void s37x_replace_opcode_scan( int set );

HDL_REGISTER_SECTION;
{
    s37x_replace_opcode_scan( TRUE );
}
END_REGISTER_SECTION

/* Module end : restore the instruction functions */
HDL_FINAL_SECTION;
{
    s37x_replace_opcode_scan( FALSE );
}
END_FINAL_SECTION

HDL_DEPENDENCY_SECTION;
{
    HDL_DEPENDENCY (HERCULES);
}
END_DEPENDENCY_SECTION

#else /* !defined( OPTION_370_EXTENSION ) */

HDL_DEPENDENCY_SECTION;
{
    LOGMSG("The S/370 Extension module requires the OPTION_370_EXTENSION build option\n");
    hdl_depc_rc = -1;
}
END_DEPENDENCY_SECTION

#endif /* defined( OPTION_370_EXTENSION ) */

#endif // defined(_370)
