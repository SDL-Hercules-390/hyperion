/* HERC_GETOPT.H (C) Copyright Roger Bowler, 2006-2012               */
/*              Hercules getopt interface                            */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#if !defined(__HERC_GETOPT_H__)
#    define  __HERC_GETOPT_H__

#include "hercules.h"
#include "getopt.h"

#if defined(NEED_GETOPT_OPTRESET)
  #define OPTRESET() optreset=1
#else
  #define OPTRESET()
#endif

#if defined(NEED_GETOPT_WRAPPER)

  // The following series of defines end up causing the source file
  // that happens to include "herc_getopt.h" to end up calling HERC's
  // version of getopt instead of the normal system getopt.

  #define  getopt         herc_getopt
  #define  optarg         herc_optarg
  #define  optind         herc_optind
  #define  optopt         herc_optopt
  #define  optreset       herc_optreset

  int herc_getopt(int,char * const *,const char *);

  #if defined(HAVE_GETOPT_LONG)
    #define  getopt_long    herc_getopt_long
    struct option; // (fwd ref)
    int herc_getopt_long(int,char * const *,const char *,const struct option *,int *);
  #endif

  extern char *herc_optarg;
  extern int   herc_optind;
  extern int   herc_opterr;
  extern int   herc_optopt;
  extern int   herc_optreset;

#endif /* defined(NEED_GETOPT_WRAPPER) */

#endif /* __HERC_GETOPT_H__ */
