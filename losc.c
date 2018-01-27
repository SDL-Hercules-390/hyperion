/* LOSC.C       (C) Copyright Jan Jaeger, 2009-2012                  */
/*              Hercules Licensed Operating System Check             */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#include "hstdinc.h"

#define _LOSC_C_
#define _HENGINE_DLL_

#include "hercules.h"

static char* licensed_os[] =
{
      "MVS",    /* Generic name for MVS, OS/390, z/OS       */
      "VM",     /* Generic name for VM, VM/XA, VM/ESA, z/VM */
      "VSE",
      "TPF",
      NULL
};

static int   os_licensed  = 0;
static bool  check_done   = false;

void losc_set( int license_status )
{
    os_licensed = license_status;
    check_done  = false;
}

void losc_check( char* ostype )
{
    char**  lictype;

    if (check_done)
        return;

    check_done = true;

    for (lictype = licensed_os; *lictype; lictype++)
    {
        if (!strncasecmp( ostype, *lictype, strlen( *lictype )))
        {
            if (os_licensed == PGM_PRD_OS_LICENSED)
            {
                // "PGMPRDOS LICENSED specified and a licenced program product operating system is running"
                WRMSG( HHC00130, "W" );
            }
            else// (os_licensed == PGM_PRD_OS_RESTRICTED)
            {
                // "A licensed program product operating system detected, all processors have been stopped"
                WRMSG( HHC00131, "A" );

                stop_all_cpus_intlock_held();
            }
        }
    }
}
