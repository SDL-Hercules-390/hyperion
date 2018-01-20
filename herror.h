/* HERROR.H     (C) Copyright Jan Jaeger, 2010-2012                  */
/*              Hercules Specfic Error codes                         */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#ifndef _HERROR_H_
#define _HERROR_H_

/* Generic error codes */

#define HNOERROR        (0)         /* OK / NO ERROR                 */
#define HERROR          (-1)        /* Generic Error                 */
#define HERRINVCMD      (-32767)    /* Invalid command  KEEP UNIQUE  */
#define HERRTHREADACT   (-5)        /* Thread is still active        */

/* CPU related error codes */

#define HERRCPUOFF      (-2)        /* CPU Offline: error configuring
                                       a new CPU: either maxcpu CPUs
                                       are already started or the
                                       create_thread() call failed.  */

#define HERRCPUONL      (-3)        /* CPU online: some CPUs are still
                                       running (not all are stopped) */

/* Device related error codes */

#define HERRDEVIDA      (-2)        /* Invalid Device Address        */

#endif // _HERROR_H_
