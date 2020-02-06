/* HERCIFC.H     (C) Copyright Roger Bowler, 2000-2012               */
/*               (C) Copyright James A. Pierson, 2002-2009           */
/*              Hercules Interface Control Program                   */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#ifndef __HERCIFC_H_
#define __HERCIFC_H_

#include "hifr.h"                       /* need struct hifr */

#define    HERCIFC_CMD  "hercifc"

typedef struct _CTLREQ
{
    long                iType;
    int                 iProcID;
    unsigned long int   iCtlOp;
    char                szIFName[IFNAMSIZ];
    union
    {
        struct hifr     hifr;
#if !defined(__APPLE__) && !defined( FREEBSD_OR_NETBSD ) && !defined(__SOLARIS__)
        struct rtentry  rtentry;
#endif
    }
    iru;
}
CTLREQ, *PCTLREQ;
#define CTLREQ_SIZE     sizeof( CTLREQ )
#define CTLREQ_OP_DONE  0

#endif // __HERCIFC_H_
