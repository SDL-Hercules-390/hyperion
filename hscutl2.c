/* HSCUTL2.C   (C) Copyright Mark L. Gaubatz and others, 2003-2012   */
/*             (C) Copyright "Fish" (David B. Trout), 2013           */
/*              Host-specific functions for Hercules                 */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/**********************************************************************/
/*                                                                    */
/*  HSCUTL2.C                                                         */
/*                                                                    */
/*  Implementation of functions used in Hercules that may be missing  */
/*  on some platform ports.                                           */
/*                                                                    */
/*  This file is portion of the HERCULES S/370, S/390 and             */
/*  z/Architecture emulator.                                          */
/*                                                                    */
/**********************************************************************/

#include "hstdinc.h"

#define _HSCUTL2_C_
#define _HUTIL_DLL_

#include "hercules.h"

#if defined( WIN32 )

/**********************************************************************/
/*                                                                    */
/*                        Windows Patches                             */
/*                                                                    */
/*  The following functional equivalents are provided for Windows:    */
/*                                                                    */
/*      int set_herc_nice( int which, id_t who, int nice );           */
/*                                                                    */
/*  Limitations:                                                      */
/*                                                                    */
/*      1. The only 'which' value supported is PRIO_PROCESS.          */
/*         The PRIO_PGRP and PRIO_USER values are not supported.      */
/*                                                                    */
/*      2. The only 'who' value supported is 0 (current process).     */
/*                                                                    */
/*      3. The 'nice' value is translated from Unix to Windows        */
/*         and vice-versa according to the following table:           */
/*                                                                    */
/*                                      Windows   Unix                */
/*         REALTIME_PRIORITY_CLASS        15       -20                */
/*         HIGH_PRIORITY_CLASS             2       -15                */
/*         ABOVE_NORMAL_PRIORITY_CLASS     1        -8                */
/*         NORMAL_PRIORITY_CLASS           0         0                */
/*         BELOW_NORMAL_PRIORITY_CLASS    -1         8                */
/*         IDLE_PRIORITY_CLASS            -2        15                */
/*                                                                    */
/**********************************************************************/

DLL_EXPORT int set_herc_nice( int which , id_t who , int nice )
{
    HANDLE hProcess;
    DWORD  dwClass;

    if (PRIO_PROCESS != which || 0 != who)
        return EINVAL;

    hProcess = (HANDLE) -1;      /* I.e. current process */

    if      (nice < -15) dwClass = REALTIME_PRIORITY_CLASS;
    else if (nice <  -8) dwClass = HIGH_PRIORITY_CLASS;
    else if (nice <   0) dwClass = ABOVE_NORMAL_PRIORITY_CLASS;
    else if (nice <   8) dwClass = NORMAL_PRIORITY_CLASS;
    else if (nice <  15) dwClass = BELOW_NORMAL_PRIORITY_CLASS;
    else                 dwClass = IDLE_PRIORITY_CLASS;

    if (!SetPriorityClass( hProcess, dwClass ))
    {
        errno = EACCES;     /* (presumed) */
        return -1;
    }
    return 0;
}

#else // !defined( WIN32 )

DLL_EXPORT int set_herc_nice( int which , id_t who , int nice )
{
    int rc;
    SETMODE( ROOT );
    {
        rc = setpriority( PRIO_PROCESS, 0, nice );
    }
    SETMODE( USER );
    return rc;
}

#endif // defined( WIN32 )
