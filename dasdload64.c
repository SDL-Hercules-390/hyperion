/* DASDLOAD64.C (C) Copyright Roger Bowler, 1999-2012                */
/*              (C) Copyright TurboHercules, SAS 2010-2011           */
/*              Hercules DASD Utilities: DASD image loader           */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* This program creates a virtual DASD volume from a list of         */
/* datasets previously unloaded using the TSO XMIT command.          */
/*-------------------------------------------------------------------*/

/*-------------------------------------------------------------------*/
/* Additional credits:                                               */
/*      Corrections to CVOL initialization logic by Jay Maynard      */
/*      IEBCOPY native dataset support by Ronen Tzur                 */
/*      Standardized Messages by P. Gorlinsky                        */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"
#include "hercules.h"
#include "dasdblks.h"
#include "ccwarn.h"
#include "cckddasd.h"   // (need cckdblk)

#define UTILITY_NAME    "dasdload64"
#define UTILITY_DESC    "Build 64-bit DASD from TSO XMIT files"

#define DASDLOAD64 1
#include "dasdload2.h"
