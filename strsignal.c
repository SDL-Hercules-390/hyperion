/* STRSIGNAL.C  (C) Copyright Roger Bowler, 1999-2012                */
/*                 Originally written by Fred Fish fnf@cygnus.com    */
/*      Implements strsignal() for architectures without it          */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#include "hstdinc.h"

#include "hercules.h"

#if !defined(HAVE_STRSIGNAL)

/* We need to declare sys_siglist, because even if the system provides
   it we can't assume that it is declared in <signal.h> (for example,
   SunOS provides sys_siglist, but it does not declare it in any
   header file).  fHowever, we can't declare sys_siglist portably,
   because on some systems it is declared with const and on some
   systems it is declared without const.  If we were using autoconf,
   we could work out the right declaration.  Until, then we just
   ignore any declaration in the system header files, and always
   declare it ourselves.  With luck, this will always work.  */
#define sys_siglist no_such_symbol

/*  Routines imported from standard C runtime libraries. */

#ifdef __STDC__
#include <stddef.h>
extern void *malloc (size_t size);                  /* 4.10.3.3 */
extern void *memset (void *s, int c, size_t n);     /* 4.11.6.1 */
#else   /* !__STDC__ */
extern char *malloc ();     /* Standard memory allocater */
extern char *memset ();
#endif  /* __STDC__ */

/* Undefine the macro we used to hide the definition of sys_siglist
   found in the system header files.  */
#undef sys_siglist

#ifndef NULL
#  ifdef __STDC__
#    define NULL (void *) 0
#  else
#    define NULL 0
#  endif
#endif

#ifndef MAX
#  define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

/* Translation table for signal values.

   Note that this table is generally only accessed when it is used at runtime
   to initialize signal name and message tables that are indexed by signal
   value.

   Not all of these signals will exist on all systems.  This table is the only
   thing that should have to be updated as new signal numbers are introduced.
   It's sort of ugly, but at least its portable. */

struct signal_info
{
  int value;        /* The numeric value from <signal.h> */
  const char *name; /* The equivalent symbolic value */
#ifndef HAVE_SYS_SIGLIST
  const char *msg;  /* Short message about this value */
#endif
};

#ifndef HAVE_SYS_SIGLIST
#   define ENTRY(value, name, msg)  {value, name, msg}
#else
#   define ENTRY(value, name, msg)  {value, name}
#endif

static const struct signal_info signal_table[] =
{
#if defined (SIGHUP)
  ENTRY(SIGHUP, "SIGHUP", "Hangup"),
#endif
#if defined (SIGINT)
  ENTRY(SIGINT, "SIGINT", "Interrupt"),
#endif
#if defined (SIGQUIT)
  ENTRY(SIGQUIT, "SIGQUIT", "Quit"),
#endif
#if defined (SIGILL)
  ENTRY(SIGILL, "SIGILL", "Illegal instruction"),
#endif
#if defined (SIGTRAP)
  ENTRY(SIGTRAP, "SIGTRAP", "Trace/breakpoint trap"),
#endif
/* Put SIGIOT before SIGABRT, so that if SIGIOT==SIGABRT then SIGABRT
   overrides SIGIOT.  SIGABRT is in ANSI and POSIX.1, and SIGIOT isn't. */
#if defined (SIGIOT)
  ENTRY(SIGIOT, "SIGIOT", "IOT trap"),
#endif
#if defined (SIGABRT)
  ENTRY(SIGABRT, "SIGABRT", "Aborted"),
#endif
#if defined (SIGEMT)
  ENTRY(SIGEMT, "SIGEMT", "Emulation trap"),
#endif
#if defined (SIGFPE)
  ENTRY(SIGFPE, "SIGFPE", "Arithmetic exception"),
#endif
#if defined (SIGKILL)
  ENTRY(SIGKILL, "SIGKILL", "Killed"),
#endif
#if defined (SIGBUS)
  ENTRY(SIGBUS, "SIGBUS", "Bus error"),
#endif
#if defined (SIGSEGV)
  ENTRY(SIGSEGV, "SIGSEGV", "Segmentation fault"),
#endif
#if defined (SIGSYS)
  ENTRY(SIGSYS, "SIGSYS", "Bad system call"),
#endif
#if defined (SIGPIPE)
  ENTRY(SIGPIPE, "SIGPIPE", "Broken pipe"),
#endif
#if defined (SIGALRM)
  ENTRY(SIGALRM, "SIGALRM", "Alarm clock"),
#endif
#if defined (SIGTERM)
  ENTRY(SIGTERM, "SIGTERM", "Terminated"),
#endif
#if defined (SIGUSR1)
  ENTRY(SIGUSR1, "SIGUSR1", "User defined signal 1"),
#endif
#if defined (SIGUSR2)
  ENTRY(SIGUSR2, "SIGUSR2", "User defined signal 2"),
#endif
/* Put SIGCLD before SIGCHLD, so that if SIGCLD==SIGCHLD then SIGCHLD
   overrides SIGCLD.  SIGCHLD is in POXIX.1 */
#if defined (SIGCLD)
  ENTRY(SIGCLD, "SIGCLD", "Child status changed"),
#endif
#if defined (SIGCHLD)
  ENTRY(SIGCHLD, "SIGCHLD", "Child status changed"),
#endif
#if defined (SIGPWR)
  ENTRY(SIGPWR, "SIGPWR", "Power fail/restart"),
#endif
#if defined (SIGWINCH)
  ENTRY(SIGWINCH, "SIGWINCH", "Window size changed"),
#endif
#if defined (SIGURG)
  ENTRY(SIGURG, "SIGURG", "Urgent I/O condition"),
#endif
#if defined (SIGIO)
  /* "I/O pending" has also been suggested, but is misleading since the
     signal only happens when the process has asked for it, not everytime
     I/O is pending. */
  ENTRY(SIGIO, "SIGIO", "I/O possible"),
#endif
#if defined (SIGPOLL)
  ENTRY(SIGPOLL, "SIGPOLL", "Pollable event occurred"),
#endif
#if defined (SIGSTOP)
  ENTRY(SIGSTOP, "SIGSTOP", "Stopped (signal)"),
#endif
#if defined (SIGTSTP)
  ENTRY(SIGTSTP, "SIGTSTP", "Stopped (user)"),
#endif
#if defined (SIGCONT)
  ENTRY(SIGCONT, "SIGCONT", "Continued"),
#endif
#if defined (SIGTTIN)
  ENTRY(SIGTTIN, "SIGTTIN", "Stopped (tty input)"),
#endif
#if defined (SIGTTOU)
  ENTRY(SIGTTOU, "SIGTTOU", "Stopped (tty output)"),
#endif
#if defined (SIGVTALRM)
  ENTRY(SIGVTALRM, "SIGVTALRM", "Virtual timer expired"),
#endif
#if defined (SIGPROF)
  ENTRY(SIGPROF, "SIGPROF", "Profiling timer expired"),
#endif
#if defined (SIGXCPU)
  ENTRY(SIGXCPU, "SIGXCPU", "CPU time limit exceeded"),
#endif
#if defined (SIGXFSZ)
  ENTRY(SIGXFSZ, "SIGXFSZ", "File size limit exceeded"),
#endif
#if defined (SIGWIND)
  ENTRY(SIGWIND, "SIGWIND", "SIGWIND"),
#endif
#if defined (SIGPHONE)
  ENTRY(SIGPHONE, "SIGPHONE", "SIGPHONE"),
#endif
#if defined (SIGLOST)
  ENTRY(SIGLOST, "SIGLOST", "Resource lost"),
#endif
#if defined (SIGWAITING)
  ENTRY(SIGWAITING, "SIGWAITING", "Process's LWPs are blocked"),
#endif
#if defined (SIGLWP)
  ENTRY(SIGLWP, "SIGLWP", "Signal LWP"),
#endif
#if defined (SIGDANGER)
  ENTRY(SIGDANGER, "SIGDANGER", "Swap space dangerously low"),
#endif
#if defined (SIGGRANT)
  ENTRY(SIGGRANT, "SIGGRANT", "Monitor mode granted"),
#endif
#if defined (SIGRETRACT)
  ENTRY(SIGRETRACT, "SIGRETRACT", "Need to relinguish monitor mode"),
#endif
#if defined (SIGMSG)
  ENTRY(SIGMSG, "SIGMSG", "Monitor mode data available"),
#endif
#if defined (SIGSOUND)
  ENTRY(SIGSOUND, "SIGSOUND", "Sound completed"),
#endif
#if defined (SIGSAK)
  ENTRY(SIGSAK, "SIGSAK", "Secure attention"),
#endif
  ENTRY(0, NULL, NULL)
};

/* Translation table allocated and initialized at runtime.  Indexed by the
   signal value to find the equivalent symbolic value. */

static const char **signal_names;
static int num_signal_names = 0;

/* Translation table allocated and initialized at runtime, if it does not
   already exist in the host environment.  Indexed by the signal value to find
   the descriptive string.

   We don't export it for use in other modules because even though it has the
   same name, it differs from other implementations in that it is dynamically
   initialized rather than statically initialized. */

#ifndef HAVE_SYS_SIGLIST

static int sys_nsig;
static const char **sys_siglist;

#else

#ifdef NSIG
static int sys_nsig = NSIG;
#else
#ifdef _NSIG
static int sys_nsig = _NSIG;
#endif
#endif
extern const char * const sys_siglist[];

#endif


#ifndef HAVE_SYS_SIGLIST
/*

NAME

    init_signal_tables -- initialize the name and message tables

SYNOPSIS

    static void init_signal_tables ();

DESCRIPTION

    Using the signal_table, which is initialized at compile time, generate
    the signal_names and the sys_siglist (if needed) tables, which are
    indexed at runtime by a specific signal value.

BUGS

    The initialization of the tables may fail under low memory conditions,
    in which case we don't do anything particularly useful, but we don't
    bomb either.  Who knows, it might succeed at a later point if we free
    some memory in the meantime.  In any case, the other routines know
    how to deal with lack of a table after trying to initialize it.  This
    may or may not be considered to be a bug, that we don't specifically
    warn about this particular failure mode.

*/

static void
init_signal_tables ()
{
  const struct signal_info *eip;
  int nbytes;

  /* If we haven't already scanned the signal_table once to find the maximum
     signal value, then go find it now. */

  if (num_signal_names == 0)
    {
      for (eip = signal_table; eip -> name != NULL; eip++)
        {
          if (eip -> value >= num_signal_names)
            {
              num_signal_names = eip -> value + 1;
            }
        }
    }

  /* Now attempt to allocate the sys_siglist table, zero it out, and then
     initialize it from the statically initialized signal_table. */

  if (sys_siglist == NULL)
    {
      nbytes = num_signal_names * sizeof (char *);
      if ((sys_siglist = (const char **) malloc (nbytes)) != NULL)
    {
      memset (sys_siglist, 0, nbytes);
      sys_nsig = num_signal_names;
      for (eip = signal_table; eip -> name != NULL; eip++)
        {
          sys_siglist[eip -> value] = eip -> msg;
        }
    }
    }
}
#endif


/*

NAME

    strsignal -- map a signal number to a signal message string

SYNOPSIS

    const char *strsignal (int signo)

DESCRIPTION

    Maps an signal number to an signal message string, the contents of
    which are implementation defined.  On systems which have the external
    variable sys_siglist, these strings will be the same as the ones used
    by psignal().

    If the supplied signal number is within the valid range of indices
    for the sys_siglist, but no message is available for the particular
    signal number, then returns the string "Signal NUM", where NUM is the
    signal number.

    If the supplied signal number is not a valid index into sys_siglist,
    returns NULL.

    The returned string is only guaranteed to be valid only until the
    next call to strsignal.

*/

const char *
strsignal (signo)
  int signo;
{
  const char *msg;
  static char buf[32];

#ifndef HAVE_SYS_SIGLIST

  if (signal_names == NULL)
    {
      init_signal_tables ();
    }

#endif

  if ((signo < 0) || (signo >= sys_nsig))
    {
      /* Out of range, just return NULL */
      msg = NULL;
    }
  else if ((sys_siglist == NULL) || (sys_siglist[signo] == NULL))
    {
      /* In range, but no sys_siglist or no entry at this index. */
      sprintf (buf, "Signal %d", signo);
      msg = (const char *) buf;
    }
  else
    {
      /* In range, and a valid message.  Just return the message. */
      msg = (const char *) sys_siglist[signo];
    }

  return (msg);
}

#endif // !defined(HAVE_STRSIGNAL)
