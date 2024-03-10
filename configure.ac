###############################################################################
# CONFIGURE.AC: Process this file with AutoConf to produce a configure script.
###############################################################################
#
#    AC is AutoConfigure, AM is AutoMake, and AH is AutoHeader. The
#    'HC_XXXX..' macros are custom Hercules autoconf macros defined
#    in the 'hercules.m4' file in the 'autoconf' subdirectory.
#
###############################################################################

AC_INIT(hercules.h)           # (package, version, bugreport email, etc)
AC_REVISION($Revision$)       # (the version of this configure.ac)
AC_CONFIG_AUX_DIR(autoconf)   # (directory containing auxillary build tools)

# -----------------------------------------------------------------------------
#  PROGRAMMING NOTE: The below VERS_MAJ, VERS_INT and VERS_MIN assignment
#  statements must retain their existing variable names (VERS_MAJ, VERS_INT
#  and VERS_MIN) since the *Nix "_dynamic_version" script and the Windows
#  "_dynamic_version.cmd" script both expect those names. (They both parse
#  this file (configure.ac) and extract VERS_MAJ, VERS_INT and VERS_MIN.)
# -----------------------------------------------------------------------------
#  PLEASE ALSO NOTE that each of them must be numeric values and must NOT
#  be defined with any leading zeros as doing so would cause the C compiler
#  to interpret them as octal values leading to a compiler error whenever
#  any of them reaches '08' or '09' both of which are invalid octal values.
# -----------------------------------------------------------------------------

VERS_MAJ=4                    # first  digit of version
VERS_INT=8                    # second digit of version
VERS_MIN=0                    # third  digit of version
VERS_DEV=1                    # flag: 1 = between releases development version
                              #       0 = official formal release version

AM_INIT_AUTOMAKE(hercules,${VERS_MAJ}.${VERS_INT}.${VERS_MIN})
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

# -----------------------------------------------------------------------------

AC_CONFIG_HEADERS(config.h)   # (the file the resulting configure script will produce)
AM_MAINTAINER_MODE()
AC_CANONICAL_HOST()           # (sets $host_cpu, $host_vendor, and $host_os)
AC_CANONICAL_TARGET()         # (sets $target_cpu, $target_vendor, and $target_os)

# FORTRAN detection scanning can goof up our shared libraries.
# So we diable checking for it.  And C++, too, while we're at it.
#
# Idea from https://stackoverflow.com/questions/4292683
#
m4_defun([_LT_AC_LANG_CXX_CONFIG], [:])
m4_defun([_LT_AC_LANG_F77_CONFIG], [:])

###############################################################################
#   Programs section...
###############################################################################

HC_PROG_CC()                  # (REQUIRED due to our auto-optimization logic!)

###############################################################################
#   OS-specific settings that we can't figure out any other way (yet)
###############################################################################

case "$target_os" in

    aix*)
        hc_cv_is_aix=yes
        hc_cv_is_nix=no
        hc_cv_is_windows=no
        hc_cv_is_mingw=no
        hc_cv_is_apple=no
        ;;
    linux*)
        hc_cv_is_aix=no
        hc_cv_is_nix=yes
        hc_cv_is_windows=no
        hc_cv_is_mingw=no
        hc_cv_is_apple=no
        ;;
    mingw*)
        hc_cv_is_aix=no
        hc_cv_is_nix=no
        hc_cv_is_windows=yes
        hc_cv_is_mingw=yes
        hc_cv_is_apple=no
        ;;
    cygwin*)
        hc_cv_is_aix=no
        hc_cv_is_nix=no
        hc_cv_is_windows=yes
        hc_cv_is_mingw=no
        hc_cv_is_apple=no
        ;;
    darwin*)
        hc_cv_is_darwin=yes
        hc_cv_is_aix=no
        hc_cv_is_nix=no
        hc_cv_is_windows=no
        hc_cv_is_mingw=no
        if test $target_vendor = apple; then
            hc_cv_is_apple=yes
        else
            hc_cv_is_apple=no
        fi

        # Added for Apple Mac M1
        if test $target_cpu = aarch64; then
            hc_cv_build_apple_m1=yes
        else
            hc_cv_build_apple_m1=no
        fi
        ;;
    *bsd*)
        hc_cv_is_aix=no
        hc_cv_is_nix=yes
        hc_cv_is_windows=no
        hc_cv_is_mingw=no
        hc_cv_is_apple=no
        ;;
    *)
        hc_cv_is_aix=no
        hc_cv_is_nix=no
        hc_cv_is_windows=no
        hc_cv_is_mingw=no
        hc_cv_is_apple=no
        ;;
esac

#--------------------------------------------------------------#
#  Determine target CPU architecture  (x86, mips, sparc, etc)  #
#--------------------------------------------------------------#

AC_MSG_CHECKING( [target CPU architecture] )

case "$target_cpu" in

    i*86|x86*)
        hc_cv_cpu_arch=x86
        hc_cv_pkg_lib_subdir=""
        ;;

    aarch64*)
        hc_cv_cpu_arch=aarch64
        hc_cv_pkg_lib_subdir="/aarch64"
        ;;

    arm*)
        hc_cv_cpu_arch=arm
        hc_cv_pkg_lib_subdir="/arm"

        case "$target_os" in
          *darwin*)
            hc_cv_cpu_arch=aarch64
            hc_cv_pkg_lib_subdir="/aarch64"
            ;;
        esac
        ;;

    e2k*)
        hc_cv_cpu_arch=e2k
        hc_cv_pkg_lib_subdir="/e2k"
        ;;

    mips*)
        hc_cv_cpu_arch=mips
        hc_cv_pkg_lib_subdir="/mips"
        ;;

    ppc*|powerpc*)
        hc_cv_cpu_arch=ppc
        hc_cv_pkg_lib_subdir="/ppc"
        ;;

    riscv64*)
        hc_cv_cpu_arch=riscv64
        hc_cv_pkg_lib_subdir="/riscv64"
        ;;

    sparc*)
        hc_cv_cpu_arch=sparc
        hc_cv_pkg_lib_subdir="/sparc"
        ;;

    s390x*)
        hc_cv_cpu_arch=s390x
        hc_cv_pkg_lib_subdir="/s390x"
        ;;

    xscale*)
        hc_cv_cpu_arch=xscale
        hc_cv_pkg_lib_subdir="/xscale"
        ;;

    *)
        hc_cv_cpu_arch=unknown
        hc_cv_pkg_lib_subdir="/unknown"
        ;;
esac

AC_MSG_RESULT( [${hc_cv_cpu_arch}] )

#----------------------------------------------------#
#  Determine target CPU bitness  (32-bit or 64-bit)  #
#----------------------------------------------------#

AC_MSG_CHECKING( [target CPU bitness] )

case "$target_cpu" in

    *64|*64le)
        hc_cv_cpu_bits=64
        ;;
    s390x)
        hc_cv_cpu_bits=64
        ;;
    *)
        hc_cv_cpu_bits=32
        ;;
esac

case "$target_os" in

    *linux*)
        if test $target_cpu = e2k; then
            # e2k-mcst-linux-gnu
            hc_cv_cpu_bits=64
        fi
        ;;
    *darwin*)
        if test $target_cpu = arm; then
            # arm-apple-darwin20.6.0
            target_cpu=aarch64
            hc_cv_cpu_bits=64
        fi
        ;;
esac

AC_MSG_RESULT( [${hc_cv_cpu_bits}] )

#----------------------------------------------------------------------#
#  Determine GCC level                                                 #
#----------------------------------------------------------------------#
hc_gcc_level="`${CC} -dumpversion`"
AC_MSG_NOTICE( [Detected GCC equivalent level for ${CC} : ${hc_gcc_level}] )

#----------------------------------------------------------------------#
#                                                                      #
#                         Initialize CFLAGS                            #
#                                                                      #
#  Note that we specify our flags BEFORE any already existing CFLAGS   #
#  in case the user specified their own CFLAGS=.  This ensures we do   #
#  not override their flags and allows them to easily override ours.   #
#                                                                      #
#----------------------------------------------------------------------#

if test "$GCC" = "yes"; then


  # We would like to use some GNU extensions if they're available

  CFLAGS="-D_GNU_SOURCE  ${CFLAGS}"


  # We require the '-pthread' option for threading to work properly

  CFLAGS="-pthread  ${CFLAGS}"


  # We would like to see EXTRA warnings too (i.e. as many as possible).
  # Note that in newer versions of gcc this option is called '-Wextra'
  # but we use '-W' instead (which is still supported) to support older
  # versions of gcc too.

  CFLAGS="-W  ${CFLAGS}"


  # We would like to see more than the usual number of warnings please
  # (Note that '-Wall' also enables '-Wformat')

  CFLAGS="-Wall  ${CFLAGS}"

fi

#-------------------------------------------------------#
#  Hard-coded target-operating-system-specific settings #
#  that we have no other/easy way to figure out...      #
#-------------------------------------------------------#

if test "$hc_cv_is_nix"        = "yes"; then

    hc_cv_build_hercifc=yes
    hc_cv_non_unique_gettimeofday=no

elif test "$hc_cv_is_windows"  = "yes"; then

    hc_cv_build_hercifc=no
    hc_cv_non_unique_gettimeofday=yes

elif test "$hc_cv_is_apple"    = "yes"; then

    hc_cv_build_hercifc=yes
    hc_cv_non_unique_gettimeofday=no

else
    hc_cv_build_hercifc=no
    hc_cv_non_unique_gettimeofday=no
fi

#----------------------#
#  AIX-specific stuff  #
#----------------------#

if test "$hc_cv_is_aix" = "yes"; then

  if test "$target_os" = "$host_os"; then

    # Not cross-compiling (likely case); use same architecture
    # as that of the actual hardware (CPU) as reported by the
    # prtconf -c  ==> "CPU Type: 32-bit" or "CPU Type: 64-bit"

    if prtconf -c | grep '64' 1>/dev/null 2>&1
    then
      hc_cv_build_aix64=yes
    else
      hc_cv_build_aix64=no
    fi
  else
    # Cross-compiling (unlikely case); rely on value of OBJECT_MODE
    # variable to tell us for which architecture they want to build...

    if echo $OBJECT_MODE | grep '64' 1>/dev/null 2>&1
    then
      hc_cv_build_aix64=yes
    else
      hc_cv_build_aix64=no
    fi
  fi
  if test "$hc_cv_build_aix64" = "yes"; then
    if test "$GCC" = "yes"; then
      CC="$CC -maix64"
      CXX="$CXX -maix64"
    else
      # VisualAge presumed...
      CC="$CC -q64"
      CXX="$CXX -q64"
    fi
    test "$AR" = "" && AR="ar"
    test "$NM" = "" && NM="nm"
    AR="$AR -X64"
    NM="$NM -X64 -B"
  fi
  LDFLAGS="$LDFLAGS -L/usr/local/lib"
  CPPFLAGS="$CPPFLAGS -I/usr/local/include -D_LARGE_FILES -D_LFS_LARGEFILE -D_LINUX_SOURCE_COMPAT"
fi

# -----------------------------------------------------------------------------
#  PROGRAMMING NOTE: The below 'AC_SUBST' macro causes AC_OUTPUT to replace
#  all instances of "@xxxxxx@" in input files with the value that the shell
#  variable "xxxxxx" has when AC_OUTPUT is called. However, we setup a define
#  for source files (MODULESDIR) and a variable for make ($(modexecdir)).  Any
#  other usage should be avoided.
# -----------------------------------------------------------------------------

modexecdir='$(libdir)/$(PACKAGE)'
AC_SUBST(modexecdir)


# -----------------------------------------------------------------------------
#
#  AC_LIBTOOL_DLOPEN
#
#       Enable checking for dlopen support. This macro should be used if the
#       package makes use of the '-dlopen' and '-dlpreopen' flags, otherwise
#       libtool will assume that the system does not support dlopening. The
#       macro must be called before AC_PROG_LIBTOOL.
#
# -----------------------------------------------------------------------------

AC_LIBTOOL_DLOPEN()                     # (we need libtool's dlopen support)


# -----------------------------------------------------------------------------
#
#  AC_LIBTOOL_WIN32_DLL
#
#       This macro should be used if the package has been ported to build
#       clean dlls on win32 platforms. Usually this means that any library
#       data items are exported with __declspec(dllexport) and imported with
#       __declspec(dllimport). If this macro is not used, libtool will assume
#       that the package libraries are not dll clean and will build only static
#       libraries on win32 hosts.
#
#       This macro must be called before AC_PROG_LIBTOOL, and provision must
#       be made to pass '-no-undefined' to libtool in link mode from the package
#       Makefile. Naturally, if you pass '-no-undefined', you must ensure that
#       all the library symbols really are defined at link time!
#
# -----------------------------------------------------------------------------

AC_LIBTOOL_WIN32_DLL()                  # (we need Win32 support in libtool)

# -----------------------------------------------------------------------------
#  This is a hack:
#
#    On NetBSD, override how libtool decides that a library can be linked into
#    a shared library. The libSoftFloat.a library is not permissible by the
#    usual rule, which allows *.so and *_pic.a (because all code in a shared
#    library must be Position Independent Code).
#
#    The result is that the shared libraries are not built and hercules
#    does not work.
#
#    Linux commonly seems to have "pass_all" for this setting.
#    We want to affect the line deplibs_check_method=... in the copied
#    libtool script.
#
#    Since we know that External Packages static libs are always built as PIC,
#    the following should workaround the problem.
#
# -----------------------------------------------------------------------------

case $host_os in
    freebsd1 | freebsd1.* | freebsd2 | freebsd2.* | freebsd3 | freebsd3.*)
      lt_cv_deplibs_check_method="match_pattern /lib[^/]+(\\.so|_pic\\.a|\\.a)\$"
      ;;

    freebsd*)
      lt_cv_deplibs_check_method="pass_all"
      ;;

    netbsd*)
      #lt_cv_deplibs_check_method="match_pattern /lib[^/]+(\\.so|_pic\\.a|\\.a)\$"
      lt_cv_deplibs_check_method="pass_all"
      ;;
esac

# -----------------------------------------------------------------------------
#  See: 'AC_PROG_LIBTOOL' below.
# -----------------------------------------------------------------------------

AC_DISABLE_STATIC()                     # (forces libtool to build shared
                                        #  libraries instead of static ones)

# -----------------------------------------------------------------------------
#  AC_PROG_LIBTOOL
#
#       Add support for the '--enable-shared' and '--disable-shared'
#       configure flags. By default, this macro turns on shared libraries
#       if they are available, and also enables static libraries if they
#       don't conflict with the shared libraries. You can modify these
#       defaults by calling either the AC_DISABLE_SHARED or AC_DISABLE_STATIC
#       macros.
#
#       Hercules REQUIRES shared libraries (i.e. DLLs), so we do indeed use
#       the AC_DISABLE_STATIC macro above.
#
# -----------------------------------------------------------------------------

AC_PROG_LIBTOOL()                       # (we build libtool for ourselves)


# -----------------------------------------------------------------------------
#
#  AC_LIB_LTDL
#
#       Even though libltdl is installed together with libtool, you may wish
#       to include libltdl in the distribution of your package, for the convenience
#       of users of your package that don't have libtool or libltdl installed.
#
#       The most simplistic way to add libltdl to your package is to copy the
#       source files, 'ltdl.c' and 'ltdl.h', to a source directory withing your
#       package and to build and link them along with the rest of your sources.
#
#       To do this, you must add a call to the 'AC_LIB_LTDL' macro to your package's
#       'configure.in' to perform the required configure time checks in order that
#       'ltdl.o' is built correctly.
#
#       This method does have its problems though: if you try to link the package
#       binaries with an installed libltdl, or a library which depends on libltdl,
#       you may have problems with duplicate symbol definitions.
#
#       In order to enable this flavor of libltdl, you should add the line
#       'AC_LIBLTDL_CONVENIENCE' to your `configure.in', before 'AC_PROG_LIBTOOL'.
#
#       In order to select the installable version of libltdl, you should add a
#       call of the macro 'AC_LIBLTDL_INSTALLABLE' to your 'configure.in' before
#       'AC_PROG_LIBTOOL'. This macro will check whether libltdl is already
#       installed and, if not, request the libltdl embedded in your package to be
#       built and installed.
#
#       Whatever macro you use, it is up to you to ensure that your 'configure.in'
#       will configure libltdl, using 'AC_CONFIG_SUBDIRS', and that your 'Makefile's
#       will start sub-makes within libltdl's directory, using automake's SUBDIRS,
#       for example. Both macros define the shell variables LIBLTDL, to the link flag
#       that you should use to link with libltdl, and LTDLINCL, to the preprocessor
#       flag that you should use to compile with programs that include 'ltdl.h'. It
#       is up to you to use 'AC_SUBST' to ensure that this variable will be available
#       in 'Makefile's, or add them to variables that are 'AC_SUBST'ed by default,
#       such as LIBS and CPPFLAGS.
#
#       So, when you want to link a program with libltdl, be it a convenience,
#       installed or installable library, just compile with '$(LTDLINCL)' and link
#       it with '$(LIBLTDL)', using libtool.
#
#       You should probably also add 'AC_LIBTOOL_DLOPEN' to your 'configure.in' before
#       'AC_PROG_LIBTOOL', otherwise libtool will assume no dlopening mechanism is
#       supported, and revert to dlpreopening, which is probably not what you want.
#
#       The following example shows you how to embed the convenience libltdl
#       in your package. In order to use the installable variant just replace
#       'AC_LIBLTDL_CONVENIENCE' with 'AC_LIBLTDL_INSTALLABLE'. We assume that libltdl
#       was embedded using 'libtoolize --ltdl':
#
#           configure.in:
#
#               ...
#               dnl Enable building of the convenience library
#               dnl and set LIBLTDL accordingly
#               AC_LIBLTDL_CONVENIENCE
#               dnl Substitute LTDLINCL and LIBLTDL in the Makefiles
#               AC_SUBST(LTDLINCL)
#               AC_SUBST(LIBLTDL)
#               dnl Check for dlopen support
#               AC_LIBTOOL_DLOPEN
#               dnl Configure libtool
#               AC_PROG_LIBTOOL
#               dnl Configure libltdl
#               AC_CONFIG_SUBDIRS(libltdl)
#               ...
#
#           Makefile.am:
#
#               ...
#               SUBDIRS = libltdl
#
#               INCLUDES = $(LTDLINCL)
#
#               myprog_LDFLAGS = -export-dynamic
#               # The quotes around -dlopen below fool automake <= 1.4 into accepting it
#               myprog_LDADD = $(LIBLTDL) "-dlopen" self "-dlopen" foo1.la
#               myprog_DEPENDENCIES = $(LIBLTDL) foo1.la
#               ...
#
# -----------------------------------------------------------------------------

AC_LIB_LTDL()                           # (we need the ltdl libtool library)
AC_SUBST([LIBTOOL_DEPS])                # (see PROGRAMMING NOTE above)

# -----------------------------------------------------------------------------
# (See comments in the 'AC_CHECK_LIB' Libraries section further below)
# -----------------------------------------------------------------------------
AC_MSG_NOTICE( [(use of lt_dlopen forced by Hercules Dynamic Loader requirement)] )

AS_IF([test "x${with_included_ltdl}" = "xno"],
    [hc_cv_have_lt_dlopen=no],
    [hc_cv_have_lt_dlopen=yes])

AS_IF([test "x${ac_cv_func_dlopen}" = "xyes" -o "x${ac_cv_lib_dl_dlopen}" = "xyes"],
    [hc_cv_have_dlopen=yes],
    [hc_cv_have_dlopen=no])

HC_LD_DISALLOWDUPS()                    # Add linker options to LDFLAGS variable
                                        # to warn about duplicate libtool symbols,
                                        # if needed.

###############################################################################
#  The following is a "global error" flag used to defer aborting configure
#  until after ALL errors have been detected/reported.
###############################################################################

hc_error=no

###############################################################################
#   Autoheader templates
###############################################################################

#  All AC_DEFINE() macros used within autoconf (to define pre-processor vars
#  used during the actual build process) must have corresponding AH_TEMPLATE
#  statements coded somewhere. We place them all here simply for convenience.

AH_TEMPLATE( [CUSTOM_BUILD_STRING],     [Define to provide additional information about this build] )
AH_TEMPLATE( [MAX_CPU_ENGS],            [Defines the maximum number of emulated CPU engines] )
AH_TEMPLATE( [HOST_ARCH],               [Define as quoted 'target_cpu' value = target host CPU architecture] )
AH_TEMPLATE( [DEBUG],                   [Define to enable extra debugging code (TRACE/VERIFY/ASSERT macros)] )
AH_TEMPLATE( [C99_FLEXIBLE_ARRAYS],     [Define if your gcc properly supports C99 flexible arrays] )
AH_TEMPLATE( [HAVE_ATTR_PRINTF],        [Define if your gcc properly supports __attribute__((format(printf,n,n)))] )
AH_TEMPLATE( [HAVE_ATTR_REGPARM],       [Define if your gcc properly supports __attribute__((regparm(n)))] )
AH_TEMPLATE( [NO_ASM_BYTESWAP],         [Define to disable assembler routines for byte swapping] )
AH_TEMPLATE( [NO_IEEE_SUPPORT],         [Define to disable IEEE floating point support] )
AH_TEMPLATE( [NO_SETUID],               [Define to disable setuid operation] )
AH_TEMPLATE( [NO_SIGABEND_HANDLER],     [Define to disable sigabend_handler (please describe this better)] )
AH_TEMPLATE( [HAVE_SIGNAL_HANDLING],    [Define to if signal handling is available] )
AH_TEMPLATE( [NON_UNIQUE_GETTIMEOFDAY], [Define if 'gettimeofday' returns non-unique values] )
AH_TEMPLATE( [NEED_GETOPT_WRAPPER],     [Define to indicate a wrapper for getopt is needed] )
AH_TEMPLATE( [NEED_GETOPT_OPTRESET],    [Define to indicate optreset exists] )
AH_TEMPLATE( [HAVE_U_INT],              [Define if your system uses u_int8_t, etc, instead of uint8_t] )
AH_TEMPLATE( [OPTION_ENHANCED_CONFIG_INCLUDE], [Define to enable enhanced-mode 'include' file support in configuration file] )
AH_TEMPLATE( [OPTION_HAO],              [Define to enable Hercules Automatic Operator feature] )
AH_TEMPLATE( [HDL_USE_LIBTOOL],         [Define to cause dynamic loader to use libtool instead of dlopen] )
AH_TEMPLATE( [BUILD_HERCIFC],           [Define if hercifc program is to be built] )
AH_TEMPLATE( [WIN32],                   [Define when building under Win32 (MinGW or Cygwin)] )
AH_TEMPLATE( [BUILD_APPLE_M1],          [Define when building for Apple Darwin M1 ARM] )
AH_TEMPLATE( [OPTION_FTHREADS],         [Define to use included threads implementation (fthreads)] )
AH_TEMPLATE( [TIMESPEC_IN_TIME_H],      [Define if 'struct timespec' defined in <time.h>] )
AH_TEMPLATE( [TIMESPEC_IN_SYS_TYPES_H], [Define if 'struct timespec' defined in <sys/types.h>] )
AH_TEMPLATE( [_BSD_SOCKLEN_T_],         [Define missing macro on apple darwin (osx) platform] )
AH_TEMPLATE( [HAVE_ZLIB],               [Define to enable zlib compression in emulated DASDs] )
AH_TEMPLATE( [CCKD_BZIP2],              [Define to enable bzip2 compression in emulated DASDs] )
AH_TEMPLATE( [HET_BZIP2],               [Define to enable bzip2 compression in emulated tapes] )
AH_TEMPLATE( [OPTION_CAPABILITIES],     [Define to enable posix draft 1003.1e capabilities] )
AH_TEMPLATE( [HAVE_OBJECT_REXX],        [Define to enable OORexx support] )
AH_TEMPLATE( [HAVE_REGINA_REXX],        [Define to enable Regina Rexx support] )
AH_TEMPLATE( [ENABLE_IPV6],             [Define to enable IPV6 support] )
AH_TEMPLATE( [HAVE_HQA_H],              [Define to if header "hqa.h" exists in HQA dir] )
AH_TEMPLATE( [HAVE_FULL_KEEPALIVE],     [Define if symbols 'TCP_KEEPIDLE', 'TCP_KEEPINTVL' and 'TCP_KEEPCNT' are ALL available ] )
AH_TEMPLATE( [HAVE_PARTIAL_KEEPALIVE],  [Define if symbol  'TCP_KEEPIDLE', 'TCP_KEEPINTVL' or 'TCP_KEEPCNT' is available ] )
AH_TEMPLATE( [HAVE_BASIC_KEEPALIVE],    [Define if symbol  'SO_KEEPALIVE' or 'TCP_KEEPALIVE' is available] )
AH_TEMPLATE( [DISABLE_IAF2],            [Define if enable_interlocked_access_facility_2 is to be forced off] )
AH_TEMPLATE( [HAVE_GCC_DIAG_PRAGMA],         [Define if _Pragma GCC diagnostic is supported] )
AH_TEMPLATE( [HAVE_GCC_DIAG_PUSHPOP],        [Define if _Pragma GCC diagnostic push/pop is supported] )
AH_TEMPLATE( [HAVE_GCC_UNUSED_FUNC_WARNING], [Define if GCC issues 'defined but not used' warnings] )
AH_TEMPLATE( [HAVE_GCC_SET_UNUSED_WARNING],  [Define if GCC issues 'variable set but not used' warnings] )
AH_TEMPLATE( [C11_ATOMICS_AVAILABLE],        [Define if C11 atomics are available] )
AH_TEMPLATE( [C11_ATOMIC_BOOL_LOCK_FREE],    [Define to 'stdatomics.h' ATOMIC_*_LOCK_FREE value] )
AH_TEMPLATE( [C11_ATOMIC_CHAR_LOCK_FREE],    [Define to 'stdatomics.h' ATOMIC_*_LOCK_FREE value] )
AH_TEMPLATE( [C11_ATOMIC_CHAR16_T_LOCK_FREE],[Define to 'stdatomics.h' ATOMIC_*_LOCK_FREE value] )
AH_TEMPLATE( [C11_ATOMIC_CHAR32_T_LOCK_FREE],[Define to 'stdatomics.h' ATOMIC_*_LOCK_FREE value] )
AH_TEMPLATE( [C11_ATOMIC_WCHAR_T_LOCK_FREE], [Define to 'stdatomics.h' ATOMIC_*_LOCK_FREE value] )
AH_TEMPLATE( [C11_ATOMIC_SHORT_LOCK_FREE],   [Define to 'stdatomics.h' ATOMIC_*_LOCK_FREE value] )
AH_TEMPLATE( [C11_ATOMIC_INT_LOCK_FREE],     [Define to 'stdatomics.h' ATOMIC_*_LOCK_FREE value] )
AH_TEMPLATE( [C11_ATOMIC_LONG_LOCK_FREE],    [Define to 'stdatomics.h' ATOMIC_*_LOCK_FREE value] )
AH_TEMPLATE( [C11_ATOMIC_LLONG_LOCK_FREE],   [Define to 'stdatomics.h' ATOMIC_*_LOCK_FREE value] )
AH_TEMPLATE( [C11_ATOMIC_POINTER_LOCK_FREE], [Define to 'stdatomics.h' ATOMIC_*_LOCK_FREE value] )
AH_TEMPLATE( [HAVE_ATOMIC_INTRINSICS],       [Define if '__atomic_xor_fetch' and friends are available] )
AH_TEMPLATE( [HAVE_SWAP_BUILTINS],           [Define if '__builtin_bswap32' and friends are available] )
AH_TEMPLATE( [HAVE_SYNC_BUILTINS],           [Define if '__sync_xor_and_fetch' and friends are available] )
AH_TEMPLATE( [HAVE_PTHREAD_SETNAME_NP],      [Define if 'pthread_setname_np()' is available] )
AH_TEMPLATE( [PTHREAD_SET_NAME_ONLY],        [Define if 'pthread_setname_np()' takes only name argument] )
AH_TEMPLATE( [PTHREAD_SET_NAME_3ARGS],       [Define if 'pthread_setname_np()' takes 3 arguments like NetBSD] )
AH_TEMPLATE( [HAVE_PTHREAD_RWLOCKATTR_SETPSHARED], [Define if 'pthread_rwlockattr_setpshared()' is available] )

###############################################################################
#   Checks for REQUIRED (non-optional) header files...
###############################################################################

#  PROGRAMMING NOTE: We use 'AC_CHECK_HEADER' here (singular) since we don't
#  care whether 'HAVE_XXX' gets #defined or not since, because these are re-
#  quired headers, if any of them are not found, we abort and thus we don't
#  need to have any 'HAVE_XXX' pre-processor #defined entered into config.h
#  (because we can't build Herc at all if any of them don't happen to exist)

AC_CHECK_HEADER( ctype.h,      [], [ AC_MSG_RESULT( [ERROR: Required header 'ctype.h' not found]      ); hc_error=yes ] )
AC_CHECK_HEADER( errno.h,      [], [ AC_MSG_RESULT( [ERROR: Required header 'errno.h' not found]      ); hc_error=yes ] )
AC_CHECK_HEADER( fcntl.h,      [], [ AC_MSG_RESULT( [ERROR: Required header 'fcntl.h' not found]      ); hc_error=yes ] )
AC_CHECK_HEADER( limits.h,     [], [ AC_MSG_RESULT( [ERROR: Required header 'limits.h' not found]     ); hc_error=yes ] )
AC_CHECK_HEADER( setjmp.h,     [], [ AC_MSG_RESULT( [ERROR: Required header 'setjmp.h' not found]     ); hc_error=yes ] )
AC_CHECK_HEADER( stdarg.h,     [], [ AC_MSG_RESULT( [ERROR: Required header 'stdarg.h' not found]     ); hc_error=yes ] )
AC_CHECK_HEADER( stdio.h,      [], [ AC_MSG_RESULT( [ERROR: Required header 'stdio.h' not found]      ); hc_error=yes ] )
AC_CHECK_HEADER( stdlib.h,     [], [ AC_MSG_RESULT( [ERROR: Required header 'stdlib.h' not found]     ); hc_error=yes ] )
AC_CHECK_HEADER( string.h,     [], [ AC_MSG_RESULT( [ERROR: Required header 'string.h' not found]     ); hc_error=yes ] )
AC_CHECK_HEADER( sched.h,      [], [ AC_MSG_RESULT( [ERROR: Required header 'sched.h' not found]      ); hc_error=yes ] )
AC_CHECK_HEADER( time.h,       [], [ AC_MSG_RESULT( [ERROR: Required header 'time.h' not found]       ); hc_error=yes ] )
AC_CHECK_HEADER( unistd.h,     [], [ AC_MSG_RESULT( [ERROR: Required header 'unistd.h' not found]     ); hc_error=yes ] )
AC_CHECK_HEADER( sys/stat.h,   [], [ AC_MSG_RESULT( [ERROR: Required header 'sys/stat.h' not found]   ); hc_error=yes ] )
AC_CHECK_HEADER( sys/time.h,   [], [ AC_MSG_RESULT( [ERROR: Required header 'sys/time.h' not found]   ); hc_error=yes ] )
AC_CHECK_HEADER( sys/types.h,  [], [ AC_MSG_RESULT( [ERROR: Required header 'sys/types.h' not found]  ); hc_error=yes ] )
AC_CHECK_HEADER( sys/mman.h,   [], [ AC_MSG_RESULT( [ERROR: Required header 'sys/mman.h' not found]   ); hc_error=yes ] )

#  PROGRAMMING NOTE: the pthread.h header only required if this is not
#  an fthreads build. Thus we delay aborting until later once we know
#  (if this is a windows build; otherwise we abort right away)

AC_CHECK_HEADER( pthread.h, [hc_cv_have_pthread_h=yes],
    [
        if test "$hc_cv_is_windows" = "yes"; then

            hc_cv_alt_pthread_location=/usr/Pthreads

            AC_MSG_NOTICE( [looking for pthread.h in ${hc_cv_alt_pthread_location}] )

            hc_temp=$CFLAGS
            CFLAGS="$CFLAGS -I${hc_cv_alt_pthread_location}"

            AC_CHECK_HEADER( pthread.h,
                [hc_cv_have_pthread_h=yes],
                [hc_cv_have_pthread_h=no]
            )

            CFLAGS=$hc_temp

        else
            AC_MSG_RESULT( [ERROR: Required header 'pthread.h' not found] )
            hc_error=yes
        fi
    ]
)

AC_CACHE_SAVE()

###############################################################################
#   Checks for optional (non-required) header files...
###############################################################################

#  PROGRAMMING NOTE: We use 'AC_CHECK_HEADERS' here (plural) to cause autoconf
#  to automatically add a #define/#undef 'HAVE_XXX' statement into config.h to
#  let us know whether the header exists on this system or not (since, because
#  these are optional headers, we are still able to successfully build Herc if
#  they don't happen to exist). The 'hc_cv_have_xxx' variables are only defined
#  in case other parts of configure.ac need to  know whether the header exists
#  or not without having to do their own AC_CHECK_HEADERS (since we've already
#  done it).

HC_CHECK_HQA_DIR()                # (Hercules Build Configuration QA Testing)

#------------------------------------------------------------------------------
#  PROGRAMMING NOTE: on Darwin and *BSD sys/socket.h must be included before
#  net/if.h, net/route.h, or netinet/in.h can be #included, and on OS X 10.3
#  (but not 10.4) sys/types.h must be #included before sys/socket.h. Thus
#  the below four header checks are treated specially. If we ever drop support
#  for OS X 10.3, a lot of this cruft can be removed, not just here but
#  anywhere we find ourselves manually including sys/types.h.

AC_CHECK_HEADERS( sys/socket.h,   [hc_cv_have_sys_socket_h=yes],   [hc_cv_have_sys_socket_h=no],
[
    #include <sys/types.h>
] )
AC_CHECK_HEADERS( netinet/in.h,   [hc_cv_have_netinet_in_h=yes],   [hc_cv_have_netinet_in_h=no],
[
    #include <sys/types.h>
    #ifdef HAVE_SYS_SOCKET_H
      #include <sys/socket.h>
    #endif
] )
AC_CHECK_HEADERS( net/if.h,       [hc_cv_have_net_if_h=yes],       [hc_cv_have_net_if_h=no],
[
    #include <sys/types.h>
    #ifdef HAVE_SYS_SOCKET_H
      #include <sys/socket.h>
    #endif
] )
AC_CHECK_HEADERS( net/route.h,    [hc_cv_have_net_route_h=yes],    [hc_cv_have_net_route_h=no],
[
    #include <sys/types.h>
    #ifdef HAVE_SYS_SOCKET_H
      #include <sys/socket.h>
    #endif
] )
AC_CHECK_HEADERS( net/tcp.h,      [hc_cv_have_net_tcp_h=yes],      [hc_cv_have_net_tcp_h=no],
[
    #include <sys/types.h>
    #ifdef HAVE_SYS_SOCKET_H
      #include <sys/socket.h>
    #endif
] )
AC_CHECK_HEADERS( netinet/tcp.h,  [hc_cv_have_netinet_tcp_h=yes],  [hc_cv_have_netinet_tcp_h=no],
[
    #include <sys/types.h>
    #ifdef HAVE_SYS_SOCKET_H
      #include <sys/socket.h>
    #endif
] )

#------------------------------------------------------------------------------

AC_CHECK_HEADERS( arpa/inet.h,    [hc_cv_have_arpa_inet_h=yes],    [hc_cv_have_arpa_inet_h=no]    )
AC_CHECK_HEADERS( linux/if_tun.h, [hc_cv_have_linux_if_tun_h=yes], [hc_cv_have_linux_if_tun_h=no] )
AC_CHECK_HEADERS( sys/ioctl.h,    [hc_cv_have_sys_ioctl_h=yes],    [hc_cv_have_sys_ioctl_h=no]    )

#------------------------------------------------------------------------------
#  PROGRAMMING NOTE: On Linux struct in6_ifreq is defined in linux/ipv6.h.
#  On other systems it may be defined in netinet6/in6_var.h or elsewhere.
#
#  in6_*.h files are strictly for library separation and should not normally
#  be directly #included. This is enforced on FreeBSD and to a lesser degree
#  on OS/X where you you can get away with it by #including netinet/in_var.h
#  first.

AC_CHECK_HEADERS( linux/ipv6.h,   [hc_cv_have_linux_ipv6_h=yes],   [hc_cv_have_linux_ipv6_h=no],
[
    #include <sys/types.h>
    #ifdef HAVE_SYS_SOCKET_H
      #include <sys/socket.h>
    #endif
] )
AC_CHECK_HEADERS( net/if_var.h,   [hc_cv_have_net_if_var_h=yes],   [hc_cv_have_net_if_var_h=no],
[
    #include <sys/types.h>
    #ifdef HAVE_SYS_SOCKET_H
      #include <sys/socket.h>
    #endif
    #ifdef HAVE_NET_IF_H
      #include <net/if.h>
    #endif
] )
AC_CHECK_HEADERS( netinet/in_var.h,  [hc_cv_have_netinet_in_var_h=yes],  [hc_cv_have_netinet_in_var_h=no],
[
    #include <sys/types.h>
    #ifdef HAVE_SYS_SOCKET_H
      #include <sys/socket.h>
    #endif
    #ifdef HAVE_NET_IF_H
      #include <net/if.h>
    #endif
    #ifdef HAVE_NET_IF_VAR_H
      #include <net/if_var.h>
    #endif
    #ifdef HAVE_NETINET_IN_H
      #include <netinet/in.h>
    #endif
] )
AC_CHECK_HEADERS( netinet6/in6_var.h,  [hc_cv_have_netinet6_in6_var_h=yes],  [hc_cv_have_netinet6_in6_var_h=no],
[
    #include <sys/types.h>
    #ifdef HAVE_SYS_SOCKET_H
      #include <sys/socket.h>
    #endif
    #ifdef HAVE_NET_IF_H
      #include <net/if.h>
    #endif
    #ifdef HAVE_NET_IF_VAR_H
      #include <net/if_var.h>
    #endif
    #ifdef HAVE_NETINET_IN_VAR_H
      #include <netinet/in.h>
      #include <netinet/in_var.h>
    #endif
] )

#------------------------------------------------------------------------------
#  PROGRAMMING NOTE: on *BSD systems sys/param.h must be #included before
#  sys/mount.h as it contains the #define of NGROUPS.

AC_CHECK_HEADERS( sys/param.h,    [hc_cv_have_sys_param_h=yes],    [hc_cv_have_sys_param_h=no]    )
AC_CHECK_HEADERS( sys/mount.h,    [hc_cv_have_sys_mount_h=yes],    [hc_cv_have_sys_mount_h=no],
[
    #ifdef HAVE_SYS_PARAM_H
      #include <sys/param.h>
    #endif
] )

#------------------------------------------------------------------------------
AC_CHECK_HEADERS_ONCE(stdatomic.h sys/sysctl.h assert.h)

AC_CHECK_HEADERS( sys/mtio.h,       [hc_cv_have_sys_mtio_h=yes],       [hc_cv_have_sys_mtio_h=no]       )
AC_CHECK_HEADERS( sys/resource.h,   [hc_cv_have_sys_resource_h=yes],   [hc_cv_have_sys_resource_h=no]   )
AC_CHECK_HEADERS( sys/uio.h,        [hc_cv_have_sys_uio_h=yes],        [hc_cv_have_sys_uio_h=no]        )
AC_CHECK_HEADERS( sys/utsname.h,    [hc_cv_have_sys_utsname_h=yes],    [hc_cv_have_sys_utsname_h=no]    )
AC_CHECK_HEADERS( sys/wait.h,       [hc_cv_have_sys_wait_h=yes],       [hc_cv_have_sys_wait_h=no]       )
AC_CHECK_HEADERS( sys/un.h,         [hc_cv_have_sys_un_h=yes],         [hc_cv_have_sys_un_h=no]         )
AC_CHECK_HEADERS( byteswap.h,       [hc_cv_have_byteswap_h=yes],       [hc_cv_have_byteswap_h=no]       )
AC_CHECK_HEADERS( bzlib.h,          [hc_cv_have_bzlib_h=yes],          [hc_cv_have_bzlib_h=no]          )

AC_CHECK_HEADERS( dirent.h,         [hc_cv_have_dirent_h=yes],         [hc_cv_have_dirent_h=no]         )

AC_CHECK_HEADERS( dlfcn.h,          [hc_cv_have_dlfcn_h=yes],          [hc_cv_have_dlfcn_h=no]          )
AC_CHECK_HEADERS( fenv.h,           [hc_cv_have_fenv_h=yes],           [hc_cv_have_fenv_h=no]           )
AC_CHECK_HEADERS( inttypes.h,       [hc_cv_have_inttypes_h=yes],       [hc_cv_have_inttypes_h=no]       )
AC_CHECK_HEADERS( stdint.h,         [hc_cv_have_stdint_h=yes],         [hc_cv_have_stdint_h=no]         )
AC_CHECK_HEADERS( stdbool.h,        [hc_cv_have_stdbool_h=yes],        [hc_cv_have_stdbool_h=no]        )
AC_CHECK_HEADERS( locale.h,         [hc_cv_have_locale_h=yes],         [hc_cv_have_locale_h=no]         )
AC_CHECK_HEADERS( ltdl.h,           [hc_cv_have_ltdl_h=yes],           [hc_cv_have_ltdl_h=no]           )
AC_CHECK_HEADERS( malloc.h,         [hc_cv_have_malloc_h=yes],         [hc_cv_have_malloc_h=no]         )
AC_CHECK_HEADERS( math.h,           [hc_cv_have_math_h=yes],           [hc_cv_have_math_h=no]           )
AC_CHECK_HEADERS( netdb.h,          [hc_cv_have_netdb_h=yes],          [hc_cv_have_netdb_h=no]          )
AC_CHECK_HEADERS( pwd.h,            [hc_cv_have_pwd_h=yes],            [hc_cv_have_pwd_h=no]            )
AC_CHECK_HEADERS( regex.h,          [hc_cv_have_regex_h=yes],          [hc_cv_have_regex_h=no]          )
AC_CHECK_HEADERS( signal.h,         [hc_cv_have_signal_h=yes],         [hc_cv_have_signal_h=no]         )
AC_CHECK_HEADERS( termios.h,        [hc_cv_have_termios_h=yes],        [hc_cv_have_termios_h=no]        )
AC_CHECK_HEADERS( time.h,           [hc_cv_have_time_h=yes],           [hc_cv_have_time_h=no]           )
AC_CHECK_HEADERS( zlib.h,           [hc_cv_have_zlib_h=yes],           [hc_cv_have_zlib_h=no]           )
AC_CHECK_HEADERS( sys/capability.h, [hc_cv_have_sys_capa_h=yes],       [hc_cv_have_sys_capa_h=no]       )
AC_CHECK_HEADERS( sys/prctl.h,      [hc_cv_have_sys_prctl_h=yes],      [hc_cv_have_sys_prctl_h=no]      )
AC_CHECK_HEADERS( sys/syscall.h,    [hc_cv_have_syscall_h=yes],        [hc_cv_have_syscall_h=no]        )
AC_CHECK_HEADERS( rexx.h,           [hc_cv_have_rexx_h=yes],           [hc_cv_have_rexx_h=no]           )
AC_CHECK_HEADERS( oorexxapi.h,      [hc_cv_have_oorexxapi_h=yes],      [hc_cv_have_oorexxapi_h=no]      )
AC_CHECK_HEADERS( regina/rexxsaa.h, [hc_cv_have_regina_rexxsaa_h=yes], [hc_cv_have_regina_rexxsaa_h=no] )
AC_CHECK_HEADERS( rexxsaa.h,        [hc_cv_have_rexxsaa_h=yes],        [hc_cv_have_rexxsaa_h=no]        )

AC_CACHE_SAVE()

###############################################################################
#   Checks for declarations...
###############################################################################

#  PROGRAMMING NOTE: For declaration checks, you need to be careful to use the
#  following test in your program:
#
#      #if defined(HAVE_DECL_XXXX) && !HAVE_DECL_XXXXX
#      ...code to handle not declared case...
#      #endif
#
#  This is because UNLIKE other 'AC_CHECK' macros, when a SYMBOL isn't DECLared,
#  "HAVE_DECL_XXXX" is #defined to '0' instead of leaving "HAVE_DECL_XXXX" #undefined.
#  (e.g. #defined to 1 if you have the declaration and #defined to 0 if you don't)

AC_CHECK_DECLS( SIGPIPE,              [hc_cv_have_sigpipe=yes],             [hc_cv_have_sigpipe=no],             [#include <signal.h>] )
AC_CHECK_DECLS( SIGBUS,               [hc_cv_have_sigbus=yes],              [hc_cv_have_sigbus=no],              [#include <signal.h>] )
AC_CHECK_DECLS( IFNAMSIZ,             [hc_cv_have_ifnamsiz=yes],            [hc_cv_have_ifnamsiz=no],
[
    #include <sys/types.h>
    #ifdef HAVE_SYS_SOCKET_H
      #include <sys/socket.h>
    #endif
] )
AC_CHECK_DECLS( LOGIN_NAME_MAX,       [hc_cv_have_login_name_max=yes],      [hc_cv_have_login_name_max=no],      [#include <limits.h>] )
AC_CHECK_DECLS( _SC_NPROCESSORS_CONF, [hc_cv_have_sc_nprocessors_conf=yes], [hc_cv_have_sc_nprocessors_conf=no], [#include <unistd.h>] )
AC_CHECK_DECLS( _SC_NPROCESSORS_ONLN, [hc_cv_have_sc_nprocessors_onln=yes], [hc_cv_have_sc_nprocessors_onln=no], [#include <unistd.h>] )
AC_CHECK_DECLS( SIOCSIFNETMASK,       [hc_cv_have_siocsifnetmask=yes],      [hc_cv_have_siocsifnetmask=no],      [#include <linux/sockios.h>] )
AC_CHECK_DECLS( SIOCSIFBRDADDR,       [hc_cv_have_siocsifbrdaddr=yes],      [hc_cv_have_siocsifbrdaddr=no],      [#include <linux/sockios.h>] )
AC_CHECK_DECLS( SIOCSIFHWADDR,        [hc_cv_have_siocsifhwaddr=yes],       [hc_cv_have_siocsifhwaddr=no],       [#include <linux/sockios.h>] )
AC_CHECK_DECLS( SIOCGIFHWADDR,        [hc_cv_have_siocgifhwaddr=yes],       [hc_cv_have_siocgifhwaddr=no],       [#include <linux/sockios.h>] )
AC_CHECK_DECLS( SIOCADDRT,            [hc_cv_have_siocaddrt=yes],           [hc_cv_have_siocaddrt=no],           [#include <linux/sockios.h>] )
AC_CHECK_DECLS( SIOCDELRT,            [hc_cv_have_siocdelrt=yes],           [hc_cv_have_siocdelrt=no],           [#include <linux/sockios.h>] )
AC_CHECK_DECLS( SIOCDIFADDR,          [hc_cv_have_siocdifaddr=yes],         [hc_cv_have_siocdifaddr=no],         [#include <linux/sockios.h>] )

if test "$hc_cv_have_sys_mtio_h" = "yes"; then
    AC_CHECK_DECLS( MTEWARN,          [hc_cv_have_mtewarn=yes],             [hc_cv_have_mtewarn=no],             [#include <sys/mtio.h>] )
else
    hc_cv_have_mtewarn=no
fi

#------------------------------------------------------------------------------
#                  Check for TCP Keepalive support
#------------------------------------------------------------------------------

AC_MSG_NOTICE( [Checking for TCP keepalive support...] )

AC_CHECK_DECLS( SO_KEEPALIVE, [hc_cv_have_basic_keepalive=yes], [hc_cv_have_basic_keepalive=no],
[
    #include <sys/types.h>
    #ifdef HAVE_SYS_SOCKET_H
      #include <sys/socket.h>
    #endif
] )

if test "$hc_cv_have_basic_keepalive" != "yes"; then
  AC_CHECK_DECLS( TCP_KEEPALIVE, [hc_cv_have_basic_keepalive=yes], [hc_cv_have_basic_keepalive=no],
  [
      #include <sys/types.h>
      #ifdef HAVE_SYS_SOCKET_H
        #include <sys/socket.h>
      #endif
      #if defined(HAVE_NET_TCP_H)
        #include <net/tcp.h>
      #elif defined(HAVE_NETINET_TCP_H)
        #include <netinet/tcp.h>
      #endif
  ] )
fi

if test "$hc_cv_have_basic_keepalive" != "yes"; then
  hc_cv_have_partial_keepalive=no
  hc_cv_have_full_keepalive=no
else
  AC_CHECK_DECLS( TCP_KEEPIDLE, [hc_cv_have_tcp_keepidle=yes], [hc_cv_have_tcp_keepidle=no],
  [
    #include <sys/types.h>
    #ifdef HAVE_SYS_SOCKET_H
      #include <sys/socket.h>
    #endif
    #if defined(HAVE_NET_TCP_H)
      #include <net/tcp.h>
    #elif defined(HAVE_NETINET_TCP_H)
      #include <netinet/tcp.h>
    #endif
  ] )
  AC_CHECK_DECLS( TCP_KEEPINTVL, [hc_cv_have_tcp_keepintvl=yes], [hc_cv_have_tcp_keepintvl=no],
  [
    #include <sys/types.h>
    #ifdef HAVE_SYS_SOCKET_H
      #include <sys/socket.h>
    #endif
    #if defined(HAVE_NET_TCP_H)
      #include <net/tcp.h>
    #elif defined(HAVE_NETINET_TCP_H)
      #include <netinet/tcp.h>
    #endif
  ] )
  AC_CHECK_DECLS( TCP_KEEPCNT, [hc_cv_have_tcp_keepcnt=yes], [hc_cv_have_tcp_keepcnt=no],
  [
    #include <sys/types.h>
    #ifdef HAVE_SYS_SOCKET_H
      #include <sys/socket.h>
    #endif
    #if defined(HAVE_NET_TCP_H)
      #include <net/tcp.h>
    #elif defined(HAVE_NETINET_TCP_H)
      #include <netinet/tcp.h>
    #endif
  ] )
  if test "$hc_cv_have_tcp_keepidle"  = "yes" &&
     test "$hc_cv_have_tcp_keepintvl" = "yes" &&
     test "$hc_cv_have_tcp_keepcnt"   = "yes"; then
     hc_cv_have_full_keepalive=yes
     hc_cv_have_partial_keepalive=yes
  else
    hc_cv_have_full_keepalive=no
    if test "$hc_cv_have_tcp_keepidle"  = "yes" ||
       test "$hc_cv_have_tcp_keepintvl" = "yes" ||
       test "$hc_cv_have_tcp_keepcnt"   = "yes"; then
       hc_cv_have_partial_keepalive=yes
    else
       hc_cv_have_partial_keepalive=no
    fi
  fi
fi

if test "$hc_cv_have_full_keepalive" = "yes"; then
  AC_MSG_NOTICE( [TCP keepalive support...  FULL] )
elif test "$hc_cv_have_partial_keepalive" = "yes"; then
  AC_MSG_NOTICE( [TCP keepalive support...  PARTIAL] )
elif test "$hc_cv_have_basic_keepalive" = "yes"; then
  AC_MSG_NOTICE( [TCP keepalive support...  BASIC] )
else
  AC_MSG_NOTICE( [TCP keepalive support...  NONE] )
fi

AC_CACHE_SAVE()

###############################################################################
#   Checks for types...
###############################################################################

AC_CHECK_TYPES(__int128_t,  [hc_cv_have___int128_t=yes], [hc_cv_have___int128_t=no] )
AC_CHECK_TYPES( u_int8_t,   [hc_cv_have_u_int8_t=yes],   [hc_cv_have_u_int8_t=no]   )
AC_CHECK_TYPES( useconds_t, [hc_cv_have_useconds_t=yes], [hc_cv_have_useconds_t=no] )
AC_CHECK_TYPES( id_t,       [hc_cv_have_id_t=yes],       [hc_cv_have_id_t=no]       )

AC_CHECK_TYPES( u_char,     [hc_cv_have_u_char=yes],     [hc_cv_have_u_char=no],
[
    #include <sys/types.h>
    #ifdef HAVE_SYS_SOCKET_H
      #include <sys/socket.h>
    #endif
] )
AC_CHECK_TYPES( u_short,    [hc_cv_have_u_short=yes],    [hc_cv_have_u_short=no],
[
    #include <sys/types.h>
    #ifdef HAVE_SYS_SOCKET_H
      #include <sys/socket.h>
    #endif
] )
AC_CHECK_TYPES( u_int,      [hc_cv_have_u_int=yes],      [hc_cv_have_u_int=no],
[
    #include <sys/types.h>
    #ifdef HAVE_SYS_SOCKET_H
      #include <sys/socket.h>
    #endif
] )
AC_CHECK_TYPES( u_long,     [hc_cv_have_u_long=yes],     [hc_cv_have_u_long=no],
[
    #include <sys/types.h>
    #ifdef HAVE_SYS_SOCKET_H
      #include <sys/socket.h>
    #endif
] )
AC_CHECK_TYPES( in_addr_t,  [hc_cv_have_in_addr_t=yes],  [hc_cv_have_in_addr_t=no],
[
    #include <sys/types.h>
    #ifdef HAVE_SYS_SOCKET_H
      #include <sys/socket.h>
    #endif
    #ifdef HAVE_NETINET_IN_H
      #include <netinet/in.h>
    #endif
] )
AC_CHECK_TYPES( socklen_t,  [hc_cv_have_socklen_t=yes],  [hc_cv_have_socklen_t=no],
[
    #include <sys/types.h>
    #ifdef HAVE_SYS_SOCKET_H
      #include <sys/socket.h>
    #endif
] )

AC_CACHE_SAVE()

###############################################################################
#   Checks for libraries...
###############################################################################

#  PROGRAMMING NOTE: we require libtool (or, optionally, dlltool (a less power-
#  ful (flexible) libtool-like tool for Windows platforms) in order to support
#  loadable modules. This is a relatively safe requirement since we provide
#  a version of libtool with Hercules (and build it as part of the preliminary
#  autoconf processing; see the 'Programs' section above). However, we need to
#  keep the below check for 'dlopen' anyway since we prefer that libtool use it
#  instead of its own equivalent (lt_dlopen) if it's available.

AC_CHECK_LIB( msvcrt, _pipe         )
AC_CHECK_LIB( dl,     dlopen        )
AC_CHECK_LIB( m,      sqrt          )
AC_CHECK_LIB( socket, connect       )
AC_CHECK_LIB( nsl,    gethostbyname )
AC_CHECK_LIB( resolv, inet_aton     )

AC_CHECK_LIB( z,   uncompress,                 [ hc_cv_have_libz=yes ],
                                               [ hc_cv_have_libz=no  ] )

AC_CHECK_LIB( bz2, BZ2_bzBuffToBuffDecompress, [ hc_cv_have_libbz2=yes ],
                                               [ hc_cv_have_libbz2=no  ] )

# jbs 10/15/2003 Solaris requires -lrt for sched_yield() and fdatasync()
AC_CHECK_LIB( rt, sched_yield )

AC_CACHE_SAVE()

###############################################################################
#   Checks for library functions...
###############################################################################
#   PROGRAMMING NOTE: AC_CHECK_LIB should be called first for the below
#   library function checks to ensure the library where the function is
#   defined gets added to the LIBS library search variable...
###############################################################################

AC_CHECK_FUNCS( pthread_setname_np,
    [hc_cv_have_pthread_setname_np=yes],
    [hc_cv_have_pthread_setname_np=no] )
AC_CHECK_FUNCS( pthread_rwlockattr_setpshared,
    [hc_cv_have_pthread_rwlockattr_setpshared=yes],
    [hc_cv_have_pthread_rwlockattr_setpshared=no] )
AC_CHECK_FUNCS( memrchr )
AC_CHECK_FUNCS( getopt_long )
AC_CHECK_FUNCS( sqrtl ldexpl fabsl fmodl frexpl )
AC_CHECK_FUNCS( ldexpf frexpf fabsf rint )
AC_CHECK_FUNCS( strlcpy strlcat )
AC_CHECK_FUNCS( strerror_r )
AC_CHECK_FUNCS( strsignal, [hc_cv_have_strsignal=yes], [hc_cv_have_strsignal=no] )
AC_CHECK_FUNCS( sys_siglist )
AC_CHECK_FUNCS( InitializeCriticalSectionAndSpinCount )
AC_CHECK_FUNCS( sleep usleep nanosleep )
AC_CHECK_FUNCS( sched_yield )
AC_CHECK_FUNCS( strtok_r )
AC_CHECK_FUNCS( pipe )
AC_CHECK_FUNCS( gettimeofday )
AC_CHECK_FUNCS( getpgrp )
AC_CHECK_FUNCS( scandir alphasort )
AC_CHECK_FUNCS( getlogin getlogin_r )
AC_CHECK_FUNCS( realpath )
AC_CHECK_FUNCS( fdatasync fsync ftruncate )
AC_CHECK_FUNCS( inet_aton )
AC_CHECK_FUNCS( fork socketpair )
AC_CHECK_FUNCS( sysconf )
AC_CHECK_FUNCS( mlock )
AC_CHECK_FUNCS( mlockall )
AC_CHECK_FUNCS( pvalloc )
AC_CHECK_FUNCS( valloc )
AC_CHECK_FUNCS( posix_memalign )

AC_CHECK_FUNCS( vsscanf,
    [hc_cv_have_vsscanf=yes],
    [hc_cv_have_vsscanf=no] )

AC_CHECK_FUNCS( setresuid getresuid,
              [hc_cv_have_getsetuid=yes],
              [hc_cv_have_getsetuid=no] )

if test "$hc_cv_have_getsetuid" != "yes"; then
    AC_CHECK_FUNCS( setreuid geteuid getuid,
                 [hc_cv_have_getsetuid=yes],
                 [hc_cv_have_getsetuid=no] )
fi


#  FIXME: Disabled because some builtin ffs seem to be causing a problem.
#         (gcc 3.4 barfs on certain 'march=' settings?)
#AC_CHECK_FUNCS( ffs )


# For OS X 10.6 autoconf defines HAVE_FDATASYNC even though there is
# no function prototype declared for fdatasync() and unistd.h contains
# define _POSIX_SYNCHRONIZED_IO (-1) which indicates that fdatasync is
# not supported. So to decide whether fdatasync really can be used, we
# create a new symbol HAVE_FDATASYNC_SUPPORTED which is defined only if
# HAVE_FDATASYNC is defined and _POSIX_SYNCHRONIZED_IO is not negative.

AC_CACHE_CHECK([whether fdatasync is supported],[ac_cv_func_fdatasync_supported],[
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <unistd.h>
    ]],[[
#if !defined(HAVE_FDATASYNC)
  #error fdatasync is not defined on this platform
#endif
#if defined(_POSIX_SYNCHRONIZED_IO) && (_POSIX_SYNCHRONIZED_IO+0 < 0)
  #error fdatasync is not supported on this platform
#endif
    ]])],
    [ac_cv_func_fdatasync_supported=yes],
    [ac_cv_func_fdatasync_supported=no])
])
AS_IF([test "x${ac_cv_func_fdatasync_supported}" = "xyes"],
    [AC_DEFINE([HAVE_FDATASYNC_SUPPORTED],[1],[Define to 1 if the fdatasync function is supported.])])


AC_CACHE_SAVE()

###############################################################################
#   Checks for structures and structure members...
###############################################################################

AC_CHECK_MEMBERS( [struct sockaddr_in.sin_len],
                  [hc_cv_have_sockaddr_in_sin_len=yes ],
                  [hc_cv_have_sockaddr_in_sin_len=no  ],
[
    #include <sys/types.h>
    #ifdef HAVE_SYS_SOCKET_H
      #include <sys/socket.h>
    #endif
    #ifdef HAVE_NETINET_IN_H
      #include <netinet/in.h>
    #endif
] )

AC_CHECK_MEMBERS( [struct in_addr.s_addr],
                  [hc_cv_have_in_addr_s_addr=yes  ],
                  [hc_cv_have_in_addr_s_addr=no   ],
[
    #include <sys/types.h>
    #ifdef HAVE_SYS_SOCKET_H
      #include <sys/socket.h>
    #endif
    #ifdef HAVE_NETINET_IN_H
      #include <netinet/in.h>
    #endif
] )

# This test works for Linux and OS/X

AC_CHECK_MEMBERS( [struct in6_ifreq.ifr6_addr],
                  [hc_cv_have_in6_ifreq_ifr6_addr=yes  ],
                  [hc_cv_have_in6_ifreq_ifr6_addr=no   ],
[
    #ifdef HAVE_LINUX_IPV6_H
      #include <linux/ipv6.h>
    #endif
    #ifdef HAVE_NETINET6_IN6_VAR_H
      #include <netinet6/in6_var.h>
    #endif
] )

# FreeBSD naming scheme

AC_CHECK_MEMBERS( [struct in6_ifreq.ifr_ifru.ifru_flags],
                  [hc_cv_have_in6_ifreq_ifru_addr=yes  ],
                  [hc_cv_have_in6_ifreq_ifru_addr=no   ],
[
    #ifdef HAVE_LINUX_IPV6_H
      #include <linux/ipv6.h>
    #endif
    #ifdef HAVE_NET_IF_H
      #include <net/if.h>
    #endif
    #ifdef HAVE_NET_IF_VAR_H
      #include <net/if_var.h>
    #endif
    #ifdef HAVE_NETINET_IN_H
      #include <netinet/in.h>
    #endif
    #ifdef HAVE_NETINET6_IN6_VAR_H
      #include <netinet/in_var.h>  // Not the ipv6 file, please
    #endif
] )

AC_CHECK_MEMBERS( [struct sigaction.sa_sigaction],
                  [hc_cv_have_sa_sigaction=yes  ],
                  [hc_cv_have_sa_sigaction=no   ],
                  [#include <signal.h>          ] )

AC_CHECK_MEMBERS( [struct timespec.tv_nsec],
    [
        hc_cv_timespec_in_sys_types_h=yes
        hc_cv_timespec_in_time_h=no
    ],
    [
        AC_CHECK_MEMBERS( [struct timespec.tv_nsec],
            [
                hc_cv_timespec_in_sys_types_h=no
                hc_cv_timespec_in_time_h=yes
            ],
            [
                hc_cv_timespec_in_sys_types_h=no
                hc_cv_timespec_in_time_h=no
            ],
            [#include <time.h>]
        )
    ],
    [#include <sys/types.h>]
)

if test "$hc_cv_have_sys_mtio_h" = "yes"; then

  AC_CHECK_MEMBERS( [struct mtget.mt_gstat],
                    [hc_cv_have_mtget_mt_gstat=yes  ],
                    [hc_cv_have_mtget_mt_gstat=no   ],
                    [#include <sys/mtio.h>] )
fi

AC_CACHE_SAVE()

###############################################################################
#   Checks for compiler characteristics...
###############################################################################

AC_C_BIGENDIAN()

#  PROGRAMMING NOTE: Okay, this is stupid. If there are any trailing spaces
#  following the type we're checking the size of, then they're converted to
#  underscores in the 'SIZEOF_XXXX' that gets #defined! For example, doing a
#  AC_CHECK_SIZEOF( int    ) yields: #define SIZEOF_INT____ 4  !!!!!!!!!!!!!
#  So... the below AC_CHECK_SIZEOF macros must NOT have any spaces in them!!

AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(long)
AC_CHECK_SIZEOF(long long)
AC_CHECK_SIZEOF(size_t)
AC_CHECK_SIZEOF(int *)
AC_CHECK_SIZEOF(off_t)

if test "$hc_cv_have_pthread_h" = "yes"; then
  AC_CHECK_SIZEOF(pthread_t)
fi

#----------------------------------#
#  Structure alignment/size test   #
#----------------------------------#

AC_MSG_NOTICE( [begin check: whether byte structs are aligned/rounded by default...] )

if test "$GCC" = "yes"; then

cat > conftest.h << __EOF
#include <stdio.h>
struct bytestruct
{
    unsigned char a;
} __attribute__((packed));
__EOF

else

cat > conftest.h << __EOF
#include <stdio.h>
struct bytestruct
{
    unsigned char a;
};
__EOF

fi

AC_CHECK_SIZEOF( struct bytestruct, [], [#include "conftest.h"] )

if test "$ac_cv_sizeof_struct_bytestruct" = "1"; then

    hc_cv_byte_structs_aligned_and_rounded_by_default=no
    hc_cv_byte_structs_always_aligned_and_rounded=no

else

    #  The sizeof our test structure is not '1'.
    #  The compiler is rounding the size of the
    #  structure upward to some predefined value.

    hc_cv_byte_structs_aligned_and_rounded_by_default=yes

    #  If there's no way to request the compiler
    #  to not do that, then we can't build Herc.

    case "$target_cpu-$GCC" in

        aarch64*-yes|arm*-yes|xscale*-yes|sh*-yes|pxa*-yes)

            hc_cv_byte_structs_always_aligned_and_rounded=no
            ;;

        *)
            hc_cv_byte_structs_always_aligned_and_rounded=yes
            ;;
    esac
fi

AC_MSG_NOTICE( [results: byte structs are aligned/rounded by default... ${hc_cv_byte_structs_aligned_and_rounded_by_default}] )

if test "$hc_cv_byte_structs_always_aligned_and_rounded" = "yes"; then

    AC_MSG_RESULT( [ERROR: Size of structures are aligned/rounded and we don't know how to tell the compiler otherwise] )
    hc_error=yes
fi

#------------------------------#
#  Check if this is GCC 2.96   #
#------------------------------#

AC_CACHE_CHECK( [if this is the broken 2.96 version of GCC],

    [hc_cv_is_gcc_2_96],
    [
        if test "$GCC" = "yes"; then

            AC_COMPILE_IFELSE(
                [
                    #if __GNUC__ == 2 && __GNUC_MINOR__ == 96
                      yes;
                    #else
                      int no;
                    #endif
                ],
                [hc_cv_is_gcc_2_96=no],
                [hc_cv_is_gcc_2_96=yes]
            )

        else
            hc_cv_is_gcc_2_96=no
        fi
    ]
)

#---------------------------------------------------#
#  Check if 'bswapxx' builtins are available        #
#---------------------------------------------------#

# Note: We purposely do *NOT* check for GCC. We don't
# care if the compiler is GCC or CLANG. Either is ok.

AC_CACHE_CHECK( [if '_bswap' builtins are available],

    [hc_cv_swap_builtins_available],
    [
        AC_COMPILE_IFELSE(
        [
            static unsigned short     _16 = 0;
            static unsigned long      _32 = 0;
            static unsigned long long _64 = 0;
            void main()
            {
                _16 = __builtin_bswap16( _16 );
                _32 = __builtin_bswap32( _32 );
                _64 = __builtin_bswap64( _64 );
            }
        ],
        [hc_cv_swap_builtins_available=yes],
        [hc_cv_swap_builtins_available=no] )
    ]
)

#---------------------------------------------------#
#  Check if atomic '__sync' builtins are available  #
#---------------------------------------------------#

# Note: We purposely do *NOT* check for GCC. We don't
# care if the compiler is GCC or CLANG. Either is ok.

AC_CACHE_CHECK( [if atomic '_sync' builtins are available],

    [hc_cv_sync_builtins_available],
    [
        AC_COMPILE_IFELSE(
        [
            static unsigned long long ll = 0;
            static unsigned long long *pll = &ll;
            static unsigned char  b = 0;
            static unsigned char *p = &b;
            void main()
            {
                __sync_or_and_fetch  ( p, 0xFF );
                __sync_and_and_fetch ( p, 0x00 );
                __sync_xor_and_fetch ( p, 0xAC );
                __sync_fetch_and_add ( pll, 88 );
            }
        ],
        [hc_cv_sync_builtins_available=yes],
        [hc_cv_sync_builtins_available=no] )
    ]
)

#------------------------------------------------#
#  Check if '__atomic' intrinsics are available  #
#------------------------------------------------#

# Note: We purposely do *NOT* check for GCC. We don't
# care if the compiler is GCC or CLANG. Either is ok.

AC_CACHE_CHECK( [if '__atomic' intrinsics are available],

    [hc_cv_atomic_intrinsics_available],
    [
        AC_COMPILE_IFELSE(
        [
            static unsigned char  b = 0;
            static unsigned char *p = &b;
            void main()
            {
                __atomic_or_fetch  ( p, 0xFF, __ATOMIC_SEQ_CST );
                __atomic_and_fetch ( p, 0x00, __ATOMIC_SEQ_CST );
                __atomic_xor_fetch ( p, 0xAC, __ATOMIC_SEQ_CST );
            }
        ],
        [hc_cv_atomic_intrinsics_available=yes],
        [hc_cv_atomic_intrinsics_available=no] )
    ]
)

#---------------------------------------#
#  Check if C11 atomics are available   #
#---------------------------------------#

# Note: We purposely do *NOT* check for GCC. We don't
# care if the compiler is GCC or CLANG. Either is ok.

AC_CACHE_CHECK( [if C11 atomics are available],

    [hc_cv_c11_atomics_available],
    [
        AC_COMPILE_IFELSE(
        [
            #if !defined(HAVE_STDATOMIC_H) || defined(__STDC_NO_ATOMICS__)
              #error C11 atomics are not available
            #else
              #include <stdatomic.h>
              #if defined(ATOMIC_CHAR_LOCK_FREE)
                #define C11_ATOMICS_ARE_AVAILABLE
              #else
                #error C11 atomics are not available
              #endif
            #endif
        ],
        [hc_cv_c11_atomics_available=yes],
        [hc_cv_c11_atomics_available=no] )
    ]
)

#-----------------------------------------------#
#  Determine 'safe' ATOMIC_*_LOCK_FREE values   #
#-----------------------------------------------#

if test "$hc_cv_c11_atomics_available" = "yes"; then

  HC_GET_C11_LOCK_FREE_VALUE(ATOMIC_BOOL_LOCK_FREE)
                   hc_cv_c11_atomic_bool_lock_free="$c11_lock_free_value"

  HC_GET_C11_LOCK_FREE_VALUE(ATOMIC_CHAR_LOCK_FREE)
                   hc_cv_c11_atomic_char_lock_free="$c11_lock_free_value"

  HC_GET_C11_LOCK_FREE_VALUE(ATOMIC_CHAR16_T_LOCK_FREE)
                   hc_cv_c11_atomic_char16_t_lock_free="$c11_lock_free_value"

  HC_GET_C11_LOCK_FREE_VALUE(ATOMIC_CHAR32_T_LOCK_FREE)
                   hc_cv_c11_atomic_char32_t_lock_free="$c11_lock_free_value"

  HC_GET_C11_LOCK_FREE_VALUE(ATOMIC_WCHAR_T_LOCK_FREE)
                   hc_cv_c11_atomic_wchar_t_lock_free="$c11_lock_free_value"

  HC_GET_C11_LOCK_FREE_VALUE(ATOMIC_SHORT_LOCK_FREE)
                   hc_cv_c11_atomic_short_lock_free="$c11_lock_free_value"

  HC_GET_C11_LOCK_FREE_VALUE(ATOMIC_INT_LOCK_FREE)
                   hc_cv_c11_atomic_int_lock_free="$c11_lock_free_value"

  HC_GET_C11_LOCK_FREE_VALUE(ATOMIC_LONG_LOCK_FREE)
                   hc_cv_c11_atomic_long_lock_free="$c11_lock_free_value"

  HC_GET_C11_LOCK_FREE_VALUE(ATOMIC_LLONG_LOCK_FREE)
                   hc_cv_c11_atomic_llong_lock_free="$c11_lock_free_value"

  HC_GET_C11_LOCK_FREE_VALUE(ATOMIC_POINTER_LOCK_FREE)
                   hc_cv_c11_atomic_pointer_lock_free="$c11_lock_free_value"

  if test "$hc_error" != "no"; then
    AC_MSG_ERROR( [Please correct the above error(s) and try again] )
  fi

else

  hc_cv_c11_atomic_bool_lock_free="0"
  hc_cv_c11_atomic_char_lock_free="0"
  hc_cv_c11_atomic_char16_t_lock_free="0"
  hc_cv_c11_atomic_char32_t_lock_free="0"
  hc_cv_c11_atomic_wchar_t_lock_free="0"
  hc_cv_c11_atomic_short_lock_free="0"
  hc_cv_c11_atomic_int_lock_free="0"
  hc_cv_c11_atomic_long_lock_free="0"
  hc_cv_c11_atomic_llong_lock_free="0"
  hc_cv_c11_atomic_pointer_lock_free="0"

fi

#----------------------------------------------#
#  Check if C99 flexible arrays are supported  #
#----------------------------------------------#
#  The logic to test whether C99 flexible      #
#  arrays are supported is defined in the      #
#  'HC_C99_FLEXIBLE_ARRAYS' macro in the       #
#  'hercules.m4' file in the 'autoconf' sub-   #
#   directory, and issues the AC_DEFINE for    #
#  'C99_FLEXIBLE_ARRAYS' if it's supported     #
#  and also sets '$hc_cv_c99_flexible_array'.  #
#----------------------------------------------#

HC_C99_FLEXIBLE_ARRAYS()

#----------------------------------------------------------------#
#  Check if GCC supports '__attribute__ ((format(printf,n,n)))'  #
#----------------------------------------------------------------#

if test "$GCC" = "yes"; then

    AC_CACHE_CHECK( [whether '__attribute__ ((format(printf,n,n)))' is supported],

        [hc_cv_attr_printf_supported],
        [
            hc_temp="$CFLAGS"
            CFLAGS="-Wall -Werror"

            AC_COMPILE_IFELSE(
                [
                    void logmsg(char*,...) __attribute__ ((format(printf,1,2)));
                ],
                [hc_cv_attr_printf_supported=yes],
                [hc_cv_attr_printf_supported=no]
            )

            CFLAGS="$hc_temp"
        ]
    )
else
    hc_cv_attr_printf_supported=no
fi

#--------------------------------------------------------#
#  Check if GCC supports '__attribute__ ((regparm(n)))'  #
#--------------------------------------------------------#

#  Note: even though at the moment GCC only supports regparm
#  on i386 or greater machines, that could change at any time
#  in the future so we don't bother checking for it.

if test "$GCC" = "yes"; then

    AC_CACHE_CHECK( [whether '__attribute__ ((regparm(n)))' is supported],

        [hc_cv_regparm_attr_supported],
        [
            hc_temp="$CFLAGS"
            CFLAGS="-Wall -Werror"

            AC_COMPILE_IFELSE(
                [
                    void conftest() __attribute__ ((regparm(1)));
                ],
                [hc_cv_regparm_attr_supported=yes],
                [hc_cv_regparm_attr_supported=no]
            )

            CFLAGS="$hc_temp"
        ]
    )
else
    hc_cv_regparm_attr_supported=no
fi

#--------------------------------------------------------#
#  Check if GCC supports diagnostic pragma               #
#--------------------------------------------------------#

if test "$GCC" = "yes"; then

    AC_CACHE_CHECK( [whether GCC supports diagnostic pragma],

        [hc_cv_have_gcc_diag_pragma],
        [
            hc_temp="$CFLAGS"
            CFLAGS="-Wall -Werror"

            AC_COMPILE_IFELSE(
                [
                    /* Test an ancient diagnostic */
                    #pragma GCC diagnostic ignored "-Wformat"
                ],
                [hc_cv_have_gcc_diag_pragma=yes],
                [hc_cv_have_gcc_diag_pragma=no]
            )

            CFLAGS="$hc_temp"
        ]
    )
else
    hc_cv_have_gcc_diag_pragma=no
fi

#--------------------------------------------------------#
#  Check if GCC supports diagnostic push/pop pragma      #
#--------------------------------------------------------#

if test "$hc_cv_have_gcc_diag_pragma" = "yes"  &&
   test "$GCC"                        = "yes"; then

    AC_CACHE_CHECK( [whether GCC supports diagnostic push/pop pragma],

        [hc_cv_have_gcc_diag_pushpop],
        [
            hc_temp="$CFLAGS"
            CFLAGS="-Wall -Werror"

            AC_COMPILE_IFELSE(
                [
                    #pragma GCC diagnostic push
                ],
                [hc_cv_have_gcc_diag_pushpop=yes],
                [hc_cv_have_gcc_diag_pushpop=no]
            )

            CFLAGS="$hc_temp"
        ]
    )
else
    hc_cv_have_gcc_diag_pushpop=no
fi

#--------------------------------------------------------#
#  Check if GCC issues func defined but not used warning #
#--------------------------------------------------------#

if test "$GCC" = "yes"; then

    # Note: success/failure is reversed for this test.

    AC_CACHE_CHECK( [whether GCC issues 'function defined but not used' warnings],

        [hc_cv_have_gcc_unused_func_warning],
        [
            hc_temp="$CFLAGS"
            CFLAGS="-Wall -Werror"

            AC_COMPILE_IFELSE(
                [
                    static int unused() {return 1;}
                    int main(int argc, char **argv)
                    {
                        return 0;
                    }
                ],
                [hc_cv_have_gcc_unused_func_warning=no],
                [hc_cv_have_gcc_unused_func_warning=yes]
            )

            CFLAGS="$hc_temp"
        ]
    )
else
    hc_cv_have_gcc_unused_func_warning=no
fi

#--------------------------------------------------------#
#  Check if GCC issues variable set but not used warning #
#--------------------------------------------------------#

if test "$GCC" = "yes"; then

    # Note: success/failure is reversed for this test.

    AC_CACHE_CHECK( [whether GCC issues 'variable set but not used' warnings],

        [hc_cv_have_gcc_set_unused_warning],
        [
            hc_temp="$CFLAGS"
            CFLAGS="-Wall -Werror"

            AC_COMPILE_IFELSE(
                [
                    int main(int argc, char **argv)
                    {
                        int rc;
                         rc = 2;
                        return 0;
                    }
                ],
                [hc_cv_have_gcc_set_unused_warning=no],
                [hc_cv_have_gcc_set_unused_warning=yes]
            )

            CFLAGS="$hc_temp"
        ]
    )
else
    hc_cv_have_gcc_set_unused_warning=no
fi

#---------------------------------------------------#
#  Test for GCC '__attribute__ ((regparm(3)))' bug  #
#---------------------------------------------------#

if test "$GCC"                          = "yes"  &&
   test "$hc_cv_regparm_attr_supported" = "yes"; then

    AC_CACHE_CHECK( [whether '__attribute__ ((regparm(3)))' is broken],

        [hc_cv_regparm_attr_broke],
        [
            hc_temp="$CFLAGS"
            CFLAGS="-O3 -fomit-frame-pointer"
            AC_TRY_RUN(
                [
                    /*
                        Fish: Test for reparms bug caused by alloca bug# 8750
                        Ref: <http://gcc.gnu.org/bugzilla/show_bug.cgi?id=8750>
                    */
                    struct REGS
                    {
                        int a, b, c, d;
                        char e[50000];
                    };
                    typedef struct REGS REGS;

                    #define ATTR_REGPARM __attribute__ (( regparm(3) ))

                    int func1 ( int a, int b, int c, REGS *regs ) ATTR_REGPARM;
                    int func2 ( int a, int b, int c, REGS *regs ) ATTR_REGPARM;

                    REGS global_regs;

                    int main()
                    {
                        return func1( 1, 2, 3, &global_regs );
                    }

                    int ATTR_REGPARM func1 ( int a, int b, int c, REGS *regs )
                    {
                        REGS stack_regs;
                        regs=regs; /* (quiet compiler warning) */
                        if ( func2( a, b, c, &stack_regs ) == 0 ) return 0; /* pass */
                        return 1; /* fail */
                    }

                    int ATTR_REGPARM func2 ( int a, int b, int c, REGS *regs )
                    {
                        regs=regs; /* (quiet compiler warning) */
                        if ( 1==a && 2==b && 3==c ) return 0; /* pass */
                        return 1; /* fail */
                    }
                ],
                [hc_cv_regparm_attr_broke=no],
                [hc_cv_regparm_attr_broke=yes],
                [hc_cv_regparm_attr_broke=yes]
            )

            CFLAGS="$hc_temp"
        ]
    )
else
    hc_cv_regparm_attr_broke=no
fi

#------------------------------------------------------#
#  Test for GCC builtin alloca bug# 8750               #
#  <http://gcc.gnu.org/bugzilla/show_bug.cgi?id=8750>  #
#------------------------------------------------------#

if test "$GCC" = "yes"; then

    AC_CACHE_CHECK( [whether '__builtin_alloca' is broken],

        [hc_cv_builtin_alloca_broke],
        [
            hc_temp=$CFLAGS
            CFLAGS="-g -O2 -fomit-frame-pointer"

            AC_TRY_RUN(
                [
                    /*
                        Fish: Test for gcc builtin alloca bug# 8750
                        <http://gcc.gnu.org/bugzilla/show_bug.cgi?id=8750>

                        Required(?!) (not sure) compiler options:

                            -g -O2 -fomit-frame-pointer
                    */

                    int foo ()
                    {
                        char a[50000+16];
                        memset(a,0xCD,50000);
                        a[50000]=0;
                        return strlen(a);
                    }

                    int main()
                    {
                        return ( foo() != 50000 );
                    }
                ],
                [hc_cv_builtin_alloca_broke=no],
                [hc_cv_builtin_alloca_broke=yes],
                [hc_cv_builtin_alloca_broke=yes]
            )

            CFLAGS=$hc_temp
        ]
    )
else
    hc_cv_builtin_alloca_broke=no
fi

#------------------------------------------------------------#
#  Check for OS X gcc preprocessor macro argument count bug  #
#------------------------------------------------------------#

if test "$GCC" = "yes"; then

    AC_CACHE_CHECK( [whether preprocessor macro argument counting broken],

        [hc_cv_pp_macro_arg_counting_broke],
        [
            hc_temp="$CFLAGS"
            CFLAGS="-Wall -Werror"

            AC_COMPILE_IFELSE(
                [
                    #include <stdio.h>
                    #define MACRO(_x,_args...) printf(_x, ## _args)
                    int  main( int argc, char **argv, char **arge )
                    {
                        MACRO( "bare printf\n" );
                        return 0;
                    }
                ],
                [hc_cv_pp_macro_arg_counting_broke=no],
                [hc_cv_pp_macro_arg_counting_broke=yes]
            )

            CFLAGS="$hc_temp"
        ]
    )
else
    hc_cv_pp_macro_arg_counting_broke=no
fi

#------------------------------------------#
#  Check for broken '-O3' version of GCC   #
#------------------------------------------#

if test "$GCC" = "yes"; then

    AC_CACHE_CHECK( [if this is GCC with broken '-O3' option],

        [hc_cv_broken_gcc_O3],
        [
            AS_VERSION_COMPARE( $hc_gcc_level, "4.9.2",
                                [hc_cv_broken_gcc_O3=no],
                                [hc_cv_broken_gcc_O3=yes],
                                [hc_cv_broken_gcc_O3=no] )
        ]
    )
else
    hc_cv_broken_gcc_O3=no
fi

#-------------------------------------------------#
#  Check if 'pthread_setname_np' takes only name  #
#-------------------------------------------------#

if test "$hc_cv_have_pthread_setname_np" = "yes"; then

  AC_CACHE_CHECK( [if 'pthread_setname_np' takes only name],

      [hc_cv_pthread_set_name_only],
      [
          AC_COMPILE_IFELSE(
          [
              #include <pthread.h>
              void main()
              {
                  pthread_setname_np("foo");
              }
          ],
          [hc_cv_pthread_set_name_only=yes],
          [hc_cv_pthread_set_name_only=no] )
      ]
  )

fi

#-------------------------------------------------#
# Check if 'pthread_setname_np' takes 3 arguments #
#-------------------------------------------------#

if test "$hc_cv_have_pthread_setname_np" = "yes"; then

  AC_CACHE_CHECK( [if 'pthread_setname_np' takes 3 arguments],

      [hc_cv_pthread_set_name_3args],
      [
          AC_COMPILE_IFELSE(
          [
              #include <pthread.h>
              void main()
              {
                  pthread_setname_np( pthread_self(), "%s", "main" );
              }
          ],
          [hc_cv_pthread_set_name_3args=yes],
          [hc_cv_pthread_set_name_3args=no] )
      ]
  )

fi

#------------------------------------------------------#
#  Test for GCC strict aliasing insanity               #
#------------------------------------------------------#

if test "$GCC" = "yes"; then

    AC_CACHE_CHECK( [for gcc strict aliasing insanity],

        [hc_cv_gcc_strict_aliasing_insanity],
        [
            hc_temp=$CFLAGS
            CFLAGS="-O3 -fstrict-aliasing"

            AC_TRY_RUN(
                [
                    #include <stdio.h>
                    #include <stdlib.h>

                    #ifdef WORDS_BIGENDIAN
                    #define I 1
                    #else
                    #define I 0
                    #endif

                    int main()
                    {
                        unsigned long a;
                        a = 111;
                        *((unsigned short*)&a + I) = 222;
                        return (a == 222 ? 0 : 1);
                    }
                ],
                [hc_cv_gcc_strict_aliasing_insanity=no],
                [hc_cv_gcc_strict_aliasing_insanity=yes],
                [hc_cv_gcc_strict_aliasing_insanity=yes]
            )

            CFLAGS=$hc_temp
        ]
    )
else
    hc_cv_gcc_strict_aliasing_insanity=no
fi

#------------------------------------------------------------#
#  Check if traditional preprocessor is the K&R C type...    #
#------------------------------------------------------------#
#
# Apple's latest GCC documentation reveals:
#
#     ... the -traditional-cpp option has changed.
#    In Apple GCC 3.1 and earlier Apple GCC compilers,
#    -traditional-cpp was used to toggle between the
#    standard GNU GCC preprocessor and Apple's own
#    preprocessor, "cpp-precomp". The GNU GCC compiler
#    interpreted -traditional-cpp differently on all
#    other platforms. Since cpp-precomp has been removed
#    for Apple's GCC 3.3 compiler, the standard GNU
#    meaning of -traditional-cpp has been restored. By
#    default, the GCC 3.3 preprocessor conforms to the
#    ISO C standard. Using the -tradtional-cpp option
#    means the C preprocessor should instead try to
#    emulate the old "K&R C".
#
#------------------------------------------------------------#

if test "$GCC" = "yes"; then

    AC_CACHE_CHECK( [whether '-traditional-cpp' is K&R C preprocessor],

        [hc_cv_traditional_cpp_is_K_AND_R_C_type],
        [
            hc_temp="$CFLAGS"
            CFLAGS="-Wall -Werror -traditional-cpp"

# Note: The test program MUST start in column 1! Otherwise, the compilation
# will fail when it's not supposed to.
            AC_COMPILE_IFELSE(
                [
/* If the following gets an error, then the
   "traditional" preprocessor is K&R C type.
   Otherwise if it compiles WITHOUT error
   then the "traditional" preprocessor is
   NOT the K&R C type.
*/
#if 1
  #include <stdio.h>  // comment/etc...
#endif
int main( int, char**, char** )
{
  return 0;
}
                ],
                [hc_cv_traditional_cpp_is_K_AND_R_C_type=no],
                [hc_cv_traditional_cpp_is_K_AND_R_C_type=yes]
            )

            CFLAGS="$hc_temp"
        ]
    )
else
    hc_cv_traditional_cpp_is_K_AND_R_C_type=no
fi

#-----------------------------------------------------------#
#  Check whether byte-swapping can be done using assembler  #
#-----------------------------------------------------------#

AC_MSG_CHECKING( [whether byte-swapping can be done using assembler] )

# Use the system's byteswap routines if they're present.
# Use our own byteswap assembler routines for i486+ only.

case "$hc_cv_cpu_arch" in

    x86)
        hc_cv_asm_byteswap=yes
        ;;
    *)
        hc_cv_asm_byteswap=$hc_cv_have_byteswap_h
        ;;
esac

AC_MSG_RESULT( [$hc_cv_asm_byteswap] )

#------------------------------------------------------------------
#  The logic to test whether optreset is needed for getopt use is
#  defined in the 'HC_CHECK_NEED_GETOPT_OPTRESET' macro in the
#  'hercules.m4' file in the autoconf directory, and issues the
#  AC_DEFINE for 'NEED_GETOPT_OPTRESET' if it's needed (and also
#  sets the '$hc_cv_need_getopt_optreset' variable appropriately).
#------------------------------------------------------------------

HC_CHECK_NEED_GETOPT_OPTRESET()

AC_CACHE_SAVE()

###############################################################################
#   Checks for system services...
###############################################################################

AC_SYS_LARGEFILE()
AC_TYPE_OFF_T()
AC_FUNC_FSEEKO()

AC_CACHE_SAVE()

###############################################################################
#   AC_CONFIG_FILES( [file...] )...
###############################################################################

AC_CACHE_SAVE()

###############################################################################
#   Set flags according to user-specified --enable-xxxxx build options
###############################################################################

#  PROGRAMMING NOTE: some of these values default to previously determined
#  values (e.g. cckd-bzip2 for example defaults to whether libbz2 exists),
#  so this section MUST [unfortunately] come AFTER the above sections. This
#  does have the unfortunate side effect of not detecting invalid options
#  right away like one would normally expect/want. The only way around that
#  would be to perform two checks (one at the beginning and then one again
#  later on), but that approach was rejected since it would tend to make our
#  configure.ac script less clean (simple and straightforward).

AC_ARG_ENABLE( extpkgs,

    AC_HELP_STRING( [--enable-extpkgs=DIR],

        [define base External Packages installation directory]
    ),
    [
        hc_cv_extpkg_dir="${enableval}"

        if test -d "${hc_cv_extpkg_dir}"; then
          AC_MSG_NOTICE( [Using External Packages override directory ${hc_cv_extpkg_dir}] )
        else
          AC_MSG_RESULT( [ERROR: --enable-extpkgs directory not found: ${hc_cv_extpkg_dir}] )
          hc_error=yes
          hc_cv_extpkg_dir=""
        fi
    ],
    [hc_cv_extpkg_dir=""]
)

AC_ARG_ENABLE( hqa,

    AC_HELP_STRING( [--enable-hqa=DIR],

        [define HQA_DIR override directory]
    ),
    [
        hc_cv_hqa_opt=${enableval}

        if test -d "$hc_cv_hqa_opt"; then
          if test -e "$hc_cv_hqa_opt/hqa.h"; then
            hc_cv_have_hqa_h=yes
          else
            hc_cv_have_hqa_h=no
          fi
        else
          AC_MSG_RESULT( [ERROR: --enable-hqa directory not found: $hc_cv_hqa_opt] )
          hc_error=yes
          hc_cv_have_hqa_h=no
        fi

        if test "$hc_cv_have_hqa_h" = "yes"; then
          if test "$hc_cv_hqa_inc" != "."; then
            AC_MSG_RESULT( [WARNING: --enable-hqa value overrides existing HAQ_DIR value] )
          fi
          hc_cv_hqa_inc=$hc_cv_hqa_opt
        else
          hc_cv_hqa_inc=.
        fi
    ]
)

AC_ARG_ENABLE( object-rexx,

    AC_HELP_STRING( [--enable-object-rexx],

        [enable OORexx support]
    ),
    [
        case "${enableval}" in
        yes) hc_cv_ena_object_rexx=yes                      ;;
        no)  hc_cv_ena_object_rexx=no                       ;;
        *)   AC_MSG_RESULT( [ERROR: invalid 'object-rexx' option] )
             hc_error=yes
             ;;
        esac
    ],
    [
     if test "$hc_cv_have_rexx_h" = "yes"; then
         hc_cv_ena_object_rexx=yes
     else
         hc_cv_ena_object_rexx=no
     fi
    ]
)

AC_ARG_ENABLE( regina-rexx,

    AC_HELP_STRING( [--enable-regina-rexx],

        [enable regina rexx support]
    ),
    [
        case "${enableval}" in
        yes) hc_cv_ena_regina_rexx=yes                      ;;
        no)  hc_cv_ena_regina_rexx=no                       ;;
        *)   AC_MSG_RESULT( [ERROR: invalid 'regina-rexx' option] )
             hc_error=yes
             ;;
        esac
    ],
    [
        hc_cv_ena_regina_rexx=no
    ]
)

AC_ARG_ENABLE( ipv6,

    AC_HELP_STRING( [--enable-ipv6],

        [enable ipv6 support]
    ),
    [
        case "${enableval}" in
        yes) hc_cv_ena_ipv6=yes                      ;;
        no)  hc_cv_ena_ipv6=no                       ;;
        *)   AC_MSG_RESULT( [ERROR: invalid 'ipv6' option] )
             hc_error=yes
             ;;
        esac
    ],
    [hc_cv_ena_ipv6=yes]
)

AC_ARG_ENABLE( cckd-bzip2,

    AC_HELP_STRING( [--enable-cckd-bzip2],

        [enable bzip2 compression for emulated dasd]
    ),
    [
        case "${enableval}" in
        yes) hc_cv_opt_cckd_bzip2=yes                      ;;
        no)  hc_cv_opt_cckd_bzip2=no                       ;;
        *)   AC_MSG_RESULT( [ERROR: invalid 'cckd-bzip2' option] )
             hc_error=yes
             ;;
        esac
    ],
    [hc_cv_opt_cckd_bzip2=$hc_cv_have_libbz2]
)

AC_ARG_ENABLE( het-bzip2,

    AC_HELP_STRING( [--enable-het-bzip2],

        [enable bzip2 compression for emulated tapes]
    ),
    [
        case "${enableval}" in
        yes) hc_cv_opt_het_bzip2=yes                      ;;
        no)  hc_cv_opt_het_bzip2=no                       ;;
        *)   AC_MSG_RESULT( [ERROR: invalid 'het-bzip2' option] )
             hc_error=yes
             ;;
        esac
    ],
    [hc_cv_opt_het_bzip2=$hc_cv_have_libbz2]
)

AC_ARG_ENABLE( debug,

    AC_HELP_STRING( [--enable-debug],

        [enable unoptimized debug code (and TRACE/VERIFY/ASSERT macros)]
    ),
    [
        case "${enableval}" in
        yes) hc_cv_opt_debug=yes                      ;;
        no)  hc_cv_opt_debug=no                       ;;
        *)   AC_MSG_RESULT( [ERROR: invalid 'debug' option] )
             hc_error=yes
             ;;
        esac
    ],
    [hc_cv_opt_debug=no]
)

AC_ARG_ENABLE( optimization,

    AC_HELP_STRING( [--enable-optimization=yes|no|FLAGS],

        [enable automatic optimization, or specify flags]
    ),
    [
        hc_cv_opt_optimization=${enableval}
    ],
    [hc_cv_opt_optimization=yes]
)

AC_ARG_ENABLE( enhanced-configincludes,

    AC_HELP_STRING( [--disable-enhanced-configincludes],

        [disable enhanced-mode 'include' file support in configuration file]
    ),
    [
        case "${enableval}" in
        yes) hc_cv_opt_enhanced_configincludes=yes                      ;;
        no)  hc_cv_opt_enhanced_configincludes=no                       ;;
        *)   AC_MSG_RESULT( [ERROR: invalid 'enhanced-configincludes' option] )
             hc_error=yes
             ;;
        esac
    ],
    [hc_cv_opt_enhanced_configincludes=yes]
)

AC_ARG_ENABLE( automatic-operator,

    AC_HELP_STRING( [--disable-automatic-operator],

        [disable Hercules Automatic Operator feature]
    ),
    [
        case "${enableval}" in
        yes) hc_cv_opt_auto_oper=yes                      ;;
        no)  hc_cv_opt_auto_oper=no                       ;;
        *)   AC_MSG_RESULT( [ERROR: invalid 'automatic-operator' option] )
             hc_error=yes
             ;;
        esac
    ],
    [hc_cv_opt_auto_oper=$hc_cv_have_regex_h]
)

AC_ARG_ENABLE( fthreads,

    AC_HELP_STRING( [--disable-fthreads],

        [disable use of fish threads instead of posix threads]
    ),
    [
        case "${enableval}" in
        yes) hc_cv_opt_fthreads=yes                      ;;
        no)  hc_cv_opt_fthreads=no                       ;;
        *)   AC_MSG_RESULT( [ERROR: invalid 'fthreads' option] )
             hc_error=yes
             ;;
        esac
    ],
    [hc_cv_opt_fthreads=$hc_cv_is_windows]
)

# "There is no support in GCC for expressing an integer constant of type
#  __int128 for targets with long long integer less than 128 bits wide."

if test "$hc_cv_have___int128_t" != "yes"; then
  hc_cv_max_cpu_engines=64
  AC_MSG_NOTICE( [NOTICE: __int128_t NOT supported; max CPU engines = ${hc_cv_max_cpu_engines}] )
else
  AC_TRY_RUN(
      [
          #include <stdio.h>
          #include <stdlib.h>

          int main()
          {
              __int128_t  foo;
              if (sizeof( foo ) >= 16)
                return 0;
              else
                return -1;
          }
      ],
      [hc_cv_max_cpu_engines=128],
      [hc_cv_max_cpu_engines=64],
      [hc_cv_max_cpu_engines=64]
  )
  if test "${hc_cv_max_cpu_engines}" = "128"; then
    AC_MSG_NOTICE( [NOTICE: sizeof(__int128_t) >= 16; max CPU engines = ${hc_cv_max_cpu_engines}] )
  else
    AC_MSG_NOTICE( [NOTICE: sizeof(__int128_t) < 16; max CPU engines = ${hc_cv_max_cpu_engines}] )
  fi
fi

hc_cv_default_num_cpu_engines=8

AC_ARG_ENABLE( multi-cpu,

    AC_HELP_STRING( [--enable-multi-cpu=yes|no|NUMBER],

        [enable/disable multi-cpu support (yes=8,no=1,NUMBER=1-64/128,default=64/128)]
    ),
    [
        case "${enableval}" in
        yes)             hc_cv_opt_num_cpu_engines=$hc_cv_default_num_cpu_engines  ;;
        no)              hc_cv_opt_num_cpu_engines=1                               ;;
        *)
            if test "${enableval}" -le 0; then
                AC_MSG_RESULT( [ERROR: 'multi-cpu' value must be > 0] )
                hc_error=yes
            else
              if test "${enableval}" -gt "$hc_cv_max_cpu_engines"; then
                AC_MSG_RESULT( [ERROR: 'multi-cpu' value must be <= ${hc_cv_max_cpu_engines}] )
                hc_error=yes
              else
                hc_cv_opt_num_cpu_engines=${enableval}
              fi
            fi
            ;;
        esac
    ],
    [hc_cv_opt_num_cpu_engines=$hc_cv_max_cpu_engines]
)

AC_ARG_ENABLE( interlocked-access-facility-2,

    AC_HELP_STRING( [--enable-interlocked-access-facility-2=yes|no],

        [enable/disable Interlocked Access Facility 2 (default yes)]
    ),
    ,
    [enable_interlocked_access_facility_2=yes]
)

AC_ARG_ENABLE( capabilities,

    AC_HELP_STRING([--disable-capabilities],

        [disable fine grained privileges]
    ),
    [
        case "${enableval}" in
        yes) hc_cv_opt_capabilities=yes                      ;;
        no)  hc_cv_opt_capabilities=no                       ;;
        *)   AC_MSG_RESULT( [ERROR: invalid 'capabilities' option] )
             hc_error=yes
             ;;
        esac
    ],
    [hc_cv_opt_capabilities=hc_cv_have_sys_capa_h]
)

# Only include libcap if needed and then force
# disable capabilities support altogether if the
# library or any required headers are missing.

if test "$hc_cv_opt_capabilities" = "yes"; then

    AC_CHECK_LIB(cap,cap_set_proc)

  if test $ac_cv_lib_cap_cap_set_proc = "no" ||
     test $hc_cv_have_sys_capa_h      = "no" ||
     test $hc_cv_have_sys_prctl_h     = "no"; then

     hc_cv_opt_capabilities="no"
  fi
fi

hc_cv_opt_custom_build_str="** The SDL 4.x Hyperion version of Hercules **"

AC_ARG_ENABLE( custom,

    AC_HELP_STRING( [--enable-custom=STRING],

        [override the default custom description of this build]
    ),
    [
        hc_cv_opt_custom_build_str=${enableval}
    ]
)

if test "$hc_cv_build_hercifc" = "yes"; then

    AC_ARG_ENABLE( setuid-hercifc,

        AC_HELP_STRING( [--enable-setuid-hercifc=yes|no|GROUPNAME],

            [install hercifc as setuid root, and allow execution by users in group GROUPNAME]
        ),
        [
            case "${enableval}" in
            yes) hc_cv_opt_setuid_hercifc=yes                      ;;
            no)  hc_cv_opt_setuid_hercifc=no                       ;;
            *)   hc_cv_opt_setuid_hercifc=yes
                 hc_cv_hercifc_groupname=${enableval}
                 ;;
            esac
        ],
        [hc_cv_opt_setuid_hercifc=no]
    )

    hc_cv_setuid_hercifc=$hc_cv_opt_setuid_hercifc

else
    hc_cv_setuid_hercifc=no
fi

#-----------------------------------------------------------------
#  The handling of AC_ARG_ENABLE for "--enable-getoptwrapper"
#  is defined within the 'HC_ARG_ENABLE_GETOPTWRAPPER' macro
#  coded in the 'hercules.m4' file in the autoconf directory
#  and issues the AC_DEFINE for NEED_GETOPT_WRAPPER if needed
#  (and sets the '$hc_cv_need_getopt_wrapper' variable too).
#-----------------------------------------------------------------

HC_ARG_ENABLE_GETOPTWRAPPER()

#----------------------------------------------------------------
#  Note: '$enable_shared' is automatically set by LIBTOOL,
#        unless the user overrides it via --disable-shared.
#----------------------------------------------------------------
hc_cv_hdl_build_shared=$enable_shared

AC_CACHE_SAVE()

###############################################################################
#   Final default settings, final sanity / error checks...
###############################################################################

if test "$hc_cv_ena_object_rexx" = "yes" ; then

   if test "$hc_cv_have_rexx_h" != "yes"; then

      AC_MSG_RESULT( [ERROR: OORexx support requested but rexx.h is not available] )
      hc_error=yes
   fi

   if test "$hc_cv_have_oorexxapi_h" != "yes"; then

      AC_MSG_RESULT( [ERROR: OORexx support requested but oorexxapi.h is not available] )
      hc_error=yes
   fi

fi

if test "$hc_cv_ena_regina_rexx" = "yes" ; then

   if test "$hc_cv_have_rexxsaa_h"        != "yes"  &&
      test "$hc_cv_have_regina_rexxsaa_h" != "yes"; then

      AC_MSG_RESULT( [ERROR: Regina Rexx support requested but rexxsaa.h is not available] )
      hc_error=yes
   fi

fi

if test "$hc_cv_ena_ipv6" = "yes" ; then

   if test "$hc_cv_have_in6_ifreq_ifr6_addr" != "yes" &&
      test "$hc_cv_have_in6_ifreq_ifru_addr" != "yes"
   then
      AC_MSG_RESULT( [ERROR: IPv6 support requested but struct in6_ifreq is not available] )
      hc_error=yes
   fi

fi

if test "$hc_cv_build_hercifc" = "yes"; then

   if test "$hc_cv_have_linux_if_tun_h" != "yes"; then

      if test "$hc_cv_have_net_if_h" != "yes"; then

         AC_MSG_RESULT( [ERROR: Required headers 'linux/if_tun.h' or 'net/if.h' not found] )
         hc_error=yes
      fi
   fi

   if test "$hc_cv_have_net_route_h" != "yes"; then

      AC_MSG_RESULT( [ERROR: Required header 'net/route_h' not found] )
      hc_error=yes
   fi
fi

#------------------------------------------------------------------------------
#  signal.h is only required if strsignal function isn't available since if
#  the strsignal function isn't available, we use our own strsignal function
#  which needs it.  The presumption that if the strsignal function is found,
#  signal.h will also be found seems a fairly safe assumption to make IMO.

if test "$hc_cv_have_strsignal" != "yes"  &&
   test "$hc_cv_have_signal_h"  != "yes"; then

   AC_MSG_RESULT( [ERROR: Required header 'signal.h' not found] )
   hc_error=yes
else
   hc_cv_have_signal_handling=yes
fi

#------------------------------------------------------------------------------

if test "$hc_cv_have_vsscanf" != "yes"; then

    AC_MSG_RESULT( [ERROR: Required function 'vsscanf' not found] )
    hc_error=yes
fi

#------------------------------------------------------------------------------

if test "$hc_cv_have_inttypes_h" != "yes"  &&
   test "$hc_cv_have_stdint_h"   != "yes"  &&
   test "$hc_cv_have_u_int8_t"   != "yes"; then

   AC_MSG_RESULT( [ERROR: unable to find fixed-size data types] )
   hc_error=yes
fi

#------------------------------------------------------------------------------

if test "$hc_cv_opt_cckd_bzip2" = "yes"  ||
   test "$hc_cv_opt_het_bzip2"  = "yes"; then

   if test "$hc_cv_have_libbz2" != "yes"; then

      AC_MSG_RESULT( [ERROR: bzip2 compression requested but libbz2 library not found] )
      hc_error=yes
   fi

   if test "$hc_cv_have_bzlib_h" != "yes"; then

      AC_MSG_RESULT( [ERROR: bzip2 compression requested but 'bzlib.h' header not found] )
      hc_error=yes
   fi
fi

#------------------------------------------------------------------------------

if test "$hc_cv_have_lt_dlopen" != "yes"  &&
   test "$hc_cv_have_dlopen"    != "yes"; then

   AC_MSG_RESULT( [ERROR: loadable modules requires libtool or dlltool] )
   hc_error=yes
fi

#------------------------------------------------------------------------------

if test "$hc_cv_opt_auto_oper" = "yes"  &&
   test "$hc_cv_have_regex_h" != "yes"; then

   AC_MSG_RESULT( [ERROR: automatic-operator requested but 'regex.h' header not found] )
   hc_error=yes
fi

#------------------------------------------------------------------------------

if test "$hc_cv_hdl_build_shared" != "yes"; then

   AC_MSG_RESULT( [ERROR: external-gui requires loadable modules (shared libraries = --enable-shared)] )
   hc_error=yes
fi

#------------------------------------------------------------------------------

if test "$hc_cv_opt_fthreads" = "yes"  &&
   test "$hc_cv_is_windows"  != "yes"; then

   AC_MSG_RESULT( [ERROR: fthreads is only for Windows platforms] )
   hc_error=yes
fi

if test "$hc_cv_have_pthread_h" != "yes"  &&
   test "$hc_cv_opt_fthreads"   != "yes"; then

   AC_MSG_RESULT( [ERROR: unable to find pthread.h] )
   hc_error=yes
fi

#------------------------------------------------------------------------------

if test "$hc_cv_is_apple"                          = "yes"  &&
   test "$hc_cv_pp_macro_arg_counting_broke"       = "yes"  &&
   test "$hc_cv_traditional_cpp_is_K_AND_R_C_type" = "yes"; then

    AC_MSG_RESULT( [ERROR: macro argument counting broken and cannot use -traditional-cpp option to work around it] )
    hc_error=yes
fi

#------------------------------------------------------------------------------

if test "$hc_cv_have_full_keepalive"  != "yes"  &&
   test "$hc_cv_have_basic_keepalive" != "yes"; then

   AC_MSG_WARN( [WARNING: Support missing for TCP keepalive] )
fi

###############################################################################
# If any errors have been detected, then abort the configure at this time
###############################################################################

if test "$hc_error" != "no"; then
    AC_MSG_ERROR( [Please correct the above error(s) and try again] )
fi

AC_CACHE_SAVE()

###############################################################################
#   Act on the results of all of the above...
###############################################################################

# Automatic determination of what optimization flags to use...

AC_MSG_CHECKING( [for what optimization flags to use] )

hc_cv_auto_optimize=""
hc_cv_optimization_flags=""

if test "$GCC" = "yes"; then

    # Note: the -g options we start with below will be removed if they specified
    # their own optimization flags via "--enable-optimization=(flags)", but will
    # be kept if they specify an optimized build via "--enable-optimization=yes"

    hc_cv_optimization_flags="$hc_cv_optimization_flags -g"     # Include debugging information
    hc_cv_optimization_flags="$hc_cv_optimization_flags -g3"    # Include module/statement information
    hc_cv_optimization_flags="$hc_cv_optimization_flags -ggdb3" # Include gdb debugger symbols
fi

#  Check their "--enable-optimization" option: Did they say "yes" or "no"
#  or let it default?  Or did they specify their own optimization flags?
#
#  If they specified 'no' then don't optimize.
#  If they specified 'yes' then determine what flags we should use.
#  If they didn't specify the option at all (they want the default),
#  then optimize only if this is NOT a debug build.
#
#  Otherwise use whatever flags they specified as-is.

case "$hc_cv_opt_optimization" in

    no)

        hc_cv_auto_optimize=no
        hc_cv_optimization_flags=""     # (as requested!)
        ;;

    yes)

        hc_cv_auto_optimize=yes
        ;;

    *)
        if test "x$hc_cv_opt_optimization" = "x"; then

            # They did NOT specify any optimization flags.
            # Don't optimize if this is a debugging build.
            # Otherwise we definitely want to optimize.

            if test "$hc_cv_opt_debug" = "yes"; then
                hc_cv_auto_optimize=no
            else
                hc_cv_auto_optimize=yes
            fi

        else
            # They specified their own flags; use them as-is.
            # This removes the -g option if we added it above.

            hc_cv_auto_optimize=no
            hc_cv_optimization_flags="$hc_cv_opt_optimization"
        fi
        ;;
esac

if test "$hc_cv_auto_optimize" = "yes"; then

    #----------------------------------------------------------#
    #                      AUTO-OPTIMIZE                       #
    #----------------------------------------------------------#

    if test "$GCC" = "yes"; then

      # We prefer -O3 (the highest optimization level possible)
      # as long as we can trust GCC to implement it correctly.

      if test "$hc_cv_broken_gcc_O3"        != "yes"  &&
         test "$hc_cv_builtin_alloca_broke" != "yes"
      then
        hc_cv_optimization_flags="$hc_cv_optimization_flags -O3"
      fi
    fi

    hc_cv_is_intel_x86_arch=no

    case "$target_cpu-$GCC" in

        x86_64-yes)

            hc_cv_is_intel_x86_arch=yes
            if test "$hc_cv_is_apple" = "yes"; then
               AS_VERSION_COMPARE( $hc_gcc_level, "4.2.0", [hc_cv_intel_cpu_type=k8],
                                                           [hc_cv_intel_cpu_type=core2],
                                                           [hc_cv_intel_cpu_type=core2] )
            else
               AS_VERSION_COMPARE( $hc_gcc_level, "4.2.0", [hc_cv_intel_cpu_type=k8],
                                                           [hc_cv_intel_cpu_type=native],
                                                           [hc_cv_intel_cpu_type=native] )
            fi
            ;;

        i386-yes|i486-yes|i586-yes|i686-yes|i786-yes)

            hc_cv_is_intel_x86_arch=yes

            if test $target_cpu = i786; then
                hc_cv_intel_cpu_type=pentium4
            else
                if test $target_cpu = i686   &&
                   test "$hc_cv_is_gcc_2_96" = "yes"; then
                    hc_cv_intel_cpu_type=i586
                else
                   if test "$hc_cv_is_apple" = "yes"; then
                      hc_cv_intel_cpu_type=$target_cpu
                   else
                      AS_VERSION_COMPARE( $hc_gcc_level, "4.2.0", [hc_cv_intel_cpu_type=$target_cpu],
                                                                  [hc_cv_intel_cpu_type=native],
                                                                  [hc_cv_intel_cpu_type=native] )
                   fi
                fi
            fi
            ;;

        aarch64-yes)

            hc_cv_is_intel_x86_arch=no
            ;;

        arm-yes)

            hc_cv_is_intel_x86_arch=no
            hc_cv_optimization_flags="$hc_cv_optimization_flags -frename-registers"
            ;;

        xscale-yes|arm*-yes)

            hc_cv_is_intel_x86_arch=no
            hc_cv_optimization_flags="$hc_cv_optimization_flags -mcpu=$target_cpu -mtune=$target_cpu -frename-registers"
            ;;
    esac

    if test "$hc_cv_is_intel_x86_arch" = "yes"; then

        # Set CPU type
        hc_cv_optimization_flags="$hc_cv_optimization_flags -march=$hc_cv_intel_cpu_type"

        # Dynamically inline small block string operations
        # (subroutine calls still made for large area calls)
        # But only if the compiler supports it

        AC_CACHE_CHECK( [for -minline-stringops-dynamically flag],

            [hc_cv_have_inline_stringops_dynamically],
            [
                hc_temp="$CFLAGS"
                CFLAGS="-minline-stringops-dynamically"

                AC_COMPILE_IFELSE(
                [
                    int main(void) { return 0; }
                ],
                [hc_cv_have_inline_stringops_dynamically=yes],
                [hc_cv_have_inline_stringops_dynamically=no] )

                CFLAGS="$hc_temp"
            ]
        )

        if test "$hc_cv_have_inline_stringops_dynamically" = "yes"; then
            hc_cv_optimization_flags="$hc_cv_optimization_flags -minline-stringops-dynamically"
        else
            hc_cv_optimization_flags="$hc_cv_optimization_flags"
        fi

        if test "$hc_cv_builtin_alloca_broke" != "yes" &&
           test "$hc_cv_opt_debug" != "yes"; then
            hc_cv_optimization_flags="$hc_cv_optimization_flags -fomit-frame-pointer"
        else
            hc_cv_optimization_flags="$hc_cv_optimization_flags -fno-omit-frame-pointer"
        fi
    fi

    if test "$hc_cv_gcc_strict_aliasing_insanity" = "yes"; then
      hc_cv_optimization_flags="$hc_cv_optimization_flags -fno-strict-aliasing"
    fi
fi

if test "x$hc_cv_optimization_flags" = "x"; then
    AC_MSG_RESULT( [(none)] )
else
    AC_MSG_RESULT( [$hc_cv_optimization_flags] )
fi

AC_CACHE_SAVE()

###############################################################################
#   DONE! -- Define our OUTPUT values and then exit...
###############################################################################

#--------------------------------------------------------------#
# (place only AC_DEFINE_UNQUOTED here; place AC_DEFINE below)  #
#--------------------------------------------------------------#

if test "x$hc_cv_opt_custom_build_str" != "x"; then
    AC_DEFINE_UNQUOTED( [CUSTOM_BUILD_STRING], "${hc_cv_opt_custom_build_str}" )
fi

AC_DEFINE_UNQUOTED( [MAX_CPU_ENGS],  ${hc_cv_opt_num_cpu_engines} )
AC_DEFINE_UNQUOTED( [HOST_ARCH],    "${target_cpu}" )

AC_DEFINE_UNQUOTED( [C11_ATOMIC_BOOL_LOCK_FREE],     ${hc_cv_c11_atomic_bool_lock_free}     )
AC_DEFINE_UNQUOTED( [C11_ATOMIC_CHAR_LOCK_FREE],     ${hc_cv_c11_atomic_char_lock_free}     )
AC_DEFINE_UNQUOTED( [C11_ATOMIC_CHAR16_T_LOCK_FREE], ${hc_cv_c11_atomic_char16_t_lock_free} )
AC_DEFINE_UNQUOTED( [C11_ATOMIC_CHAR32_T_LOCK_FREE], ${hc_cv_c11_atomic_char32_t_lock_free} )
AC_DEFINE_UNQUOTED( [C11_ATOMIC_WCHAR_T_LOCK_FREE],  ${hc_cv_c11_atomic_wchar_t_lock_free}  )
AC_DEFINE_UNQUOTED( [C11_ATOMIC_SHORT_LOCK_FREE],    ${hc_cv_c11_atomic_short_lock_free}    )
AC_DEFINE_UNQUOTED( [C11_ATOMIC_INT_LOCK_FREE],      ${hc_cv_c11_atomic_int_lock_free}      )
AC_DEFINE_UNQUOTED( [C11_ATOMIC_LONG_LOCK_FREE],     ${hc_cv_c11_atomic_long_lock_free}     )
AC_DEFINE_UNQUOTED( [C11_ATOMIC_LLONG_LOCK_FREE],    ${hc_cv_c11_atomic_llong_lock_free}    )
AC_DEFINE_UNQUOTED( [C11_ATOMIC_POINTER_LOCK_FREE],  ${hc_cv_c11_atomic_pointer_lock_free}  )

AC_CACHE_SAVE()

AC_MSG_NOTICE( [                                         ] )
AC_MSG_NOTICE( [ Package destination directory prefixes: ] )
AC_MSG_NOTICE( [                                         ] )
AC_MSG_NOTICE( [    Libraries:  ${modexecdir}            ] )
AC_MSG_NOTICE( [    Data:       \$(datadir)/hercules     ] )
AC_MSG_NOTICE( [    Locale:     ${localedir}             ] )
AC_MSG_NOTICE( [                                         ] )

CPPFLAGS="$CPPFLAGS"' -DPKGDATADIR=\"$(pkgdatadir)\" -DMODULESDIR=\"$(modexecdir)\" -DHERC_LOCALEDIR=\"$(localedir)\"'

#---------------------------------------------------------------#
#  (place only AC_DEFINE here; place AC_DEFINE_UNQUOTED above)  #
#---------------------------------------------------------------#

test "$hc_cv_have_hqa_h"                  = "yes"  &&  AC_DEFINE(HAVE_HQA_H)
test "$hc_cv_opt_debug"                   = "yes"  &&  AC_DEFINE(DEBUG)
test "$hc_cv_have_u_int8_t"               = "yes"  &&  AC_DEFINE(HAVE_U_INT)
test "$hc_cv_opt_enhanced_configincludes" = "yes"  &&  AC_DEFINE(OPTION_ENHANCED_CONFIG_INCLUDE)
test "$hc_cv_opt_auto_oper"               = "yes"  &&  AC_DEFINE(OPTION_HAO)
test "$hc_cv_opt_fthreads"                = "yes"  &&  AC_DEFINE(OPTION_FTHREADS)
test "$hc_cv_have_lt_dlopen"              = "yes"  &&  AC_DEFINE(HDL_USE_LIBTOOL)
test "$hc_cv_is_windows"                  = "yes"  &&  AC_DEFINE(WIN32)
test "$hc_cv_have_libz"                   = "yes"  &&  AC_DEFINE(HAVE_ZLIB)
test "$hc_cv_opt_cckd_bzip2"              = "yes"  &&  AC_DEFINE(CCKD_BZIP2)
test "$hc_cv_opt_het_bzip2"               = "yes"  &&  AC_DEFINE(HET_BZIP2)
test "$hc_cv_timespec_in_sys_types_h"     = "yes"  &&  AC_DEFINE(TIMESPEC_IN_SYS_TYPES_H)
test "$hc_cv_timespec_in_time_h"          = "yes"  &&  AC_DEFINE(TIMESPEC_IN_TIME_H)
test "$hc_cv_have_getsetuid"             != "yes"  &&  AC_DEFINE(NO_SETUID)
test "$hc_cv_asm_byteswap"               != "yes"  &&  AC_DEFINE(NO_ASM_BYTESWAP)
test "$hc_cv_non_unique_gettimeofday"     = "yes"  &&  AC_DEFINE(NON_UNIQUE_GETTIMEOFDAY)
test "$hc_cv_build_hercifc"               = "yes"  &&  AC_DEFINE(BUILD_HERCIFC)
test "$hc_cv_build_apple_m1"              = "yes"  &&  AC_DEFINE(BUILD_APPLE_M1)
test "$hc_cv_opt_capabilities"            = "yes"  &&  AC_DEFINE(OPTION_CAPABILITIES)
test "$hc_cv_ena_object_rexx"             = "yes"  &&  AC_DEFINE(HAVE_OBJECT_REXX)
test "$hc_cv_ena_regina_rexx"             = "yes"  &&  AC_DEFINE(HAVE_REGINA_REXX)
test "$hc_cv_ena_ipv6"                    = "yes"  &&  AC_DEFINE(ENABLE_IPV6)
test "$hc_cv_have_full_keepalive"         = "yes"  &&  AC_DEFINE(HAVE_FULL_KEEPALIVE)
test "$hc_cv_have_partial_keepalive"      = "yes"  &&  AC_DEFINE(HAVE_PARTIAL_KEEPALIVE)
test "$hc_cv_have_basic_keepalive"        = "yes"  &&  AC_DEFINE(HAVE_BASIC_KEEPALIVE)
test "$hc_cv_c11_atomics_available"       = "yes"  &&  AC_DEFINE(C11_ATOMICS_AVAILABLE)
test "$hc_cv_atomic_intrinsics_available" = "yes"  &&  AC_DEFINE(HAVE_ATOMIC_INTRINSICS)
test "$hc_cv_swap_builtins_available"     = "yes"  &&  AC_DEFINE(HAVE_SWAP_BUILTINS)
test "$hc_cv_sync_builtins_available"     = "yes"  &&  AC_DEFINE(HAVE_SYNC_BUILTINS)
test "$hc_cv_have_pthread_setname_np"     = "yes"  &&  AC_DEFINE(HAVE_PTHREAD_SETNAME_NP)
test "$hc_cv_pthread_set_name_only"       = "yes"  &&  AC_DEFINE(PTHREAD_SET_NAME_ONLY)
test "$hc_cv_pthread_set_name_3args"      = "yes"  &&  AC_DEFINE(PTHREAD_SET_NAME_3ARGS)
test "$hc_cv_have_pthread_rwlockattr_setpshared" = "yes" && AC_DEFINE(HAVE_PTHREAD_RWLOCKATTR_SETPSHARED)
test "$enable_interlocked_access_facility_2"     = "no"  && AC_DEFINE(DISABLE_IAF2)
test "$hc_cv_have_gcc_diag_pragma"         = "yes" &&  AC_DEFINE(HAVE_GCC_DIAG_PRAGMA)
test "$hc_cv_have_gcc_diag_pushpop"        = "yes" &&  AC_DEFINE(HAVE_GCC_DIAG_PUSHPOP)
test "$hc_cv_have_gcc_unused_func_warning" = "yes" &&  AC_DEFINE(HAVE_GCC_UNUSED_FUNC_WARNING)
test "$hc_cv_have_gcc_set_unused_warning"  = "yes" &&  AC_DEFINE(HAVE_GCC_SET_UNUSED_WARNING)

if test $hc_cv_have_sa_sigaction    = yes &&
   test $hc_cv_have_signal_handling = yes; then
    AC_DEFINE(HAVE_SIGNAL_HANDLING)
fi

if test "$hc_cv_attr_printf_supported" = "yes"; then

    AC_DEFINE(HAVE_ATTR_PRINTF)
fi

if test "$hc_cv_regparm_attr_supported" = "yes"  &&
   test "$hc_cv_regparm_attr_broke"    != "yes"; then

    AC_DEFINE(HAVE_ATTR_REGPARM)
fi

if test "$hc_cv_is_windows"  != "yes"  &&
   test "$hc_cv_have_fenv_h" != "yes"; then

    AC_DEFINE(NO_IEEE_SUPPORT)
fi

#--------------------------------------------------#
#  CPPFLAGS  (pre-processor flags)                 #
#--------------------------------------------------#

if test "$hc_cv_is_apple"                    = "yes"  &&
   test "$hc_cv_pp_macro_arg_counting_broke" = "yes"; then

    CPPFLAGS="${CPPFLAGS} -traditional-cpp -Wno-endif-labels"
fi

#--------------------------------------------------#
#  CFLAGS  (compiler flags)                        #
#--------------------------------------------------#
#                                                  #
#  Note that for most options we specify our flags #
#  BEFORE any already existing CFLAGS in case the  #
#  user specified their own CFLAGS= option.  This  #
#  ensures we do not override their flags while    #
#  still allowing them to easily override ours.    #
#                                                  #
#--------------------------------------------------#

if test "$hc_cv_is_windows"             = "yes"; then

    if test "$hc_cv_have_pthread_h"         = "yes" &&
       test "x$hc_cv_alt_pthread_location" != "x";  then

      # PROGRAMMING NOTE: the '-I' header include directory
      # option is an exception to the above rule: we must
      # specify our '-I' option AFTER any already existing
      # CFLAGS value since the compiler always searches '-I'
      # directories in the order they are specified on the
      # command line and we want to ensure that any '-I'
      # option they may have specified in their CFLAGS value
      # is always searched ahead of our own '-I' directory.

        CFLAGS="$CFLAGS -I${hc_cv_alt_pthread_location}"
    fi

    CFLAGS="-Wno-format  ${CFLAGS}"
fi

if test "$hc_cv_byte_structs_aligned_and_rounded_by_default" = "yes"; then

    # Request 8-bit (byte) struct boundary alignment

    CFLAGS="-mstructure-size-boundary=8  ${CFLAGS}"
fi

if test  "${hc_cv_optimization_flags}" != ""; then
  CFLAGS="${hc_cv_optimization_flags}  ${CFLAGS}"
fi

#--------------------------------------------------#
#  LIBS  (linker flags)                            #
#--------------------------------------------------#

test  "$hc_cv_have_libbz2" =  "yes"  &&  LIBS="$LIBS -lbz2"
test  "$hc_cv_have_libz"   =  "yes"  &&  LIBS="$LIBS -lz"
test  "$hc_cv_is_mingw"    =  "yes"  &&  LIBS="$LIBS -lmsvcrt"
test  "$hc_cv_is_mingw"    =  "yes"  &&  LIBS="$LIBS -lws2_32"

#------------------------------------------------------#
#  Construct cpu-bitness + build-configuration suffix  #
#------------------------------------------------------#

if test "$hc_cv_opt_debug" = "yes"; then
  hc_cv_pkg_lib_suffix=${hc_cv_cpu_bits}d
else
  hc_cv_pkg_lib_suffix=${hc_cv_cpu_bits}
fi

AC_CACHE_SAVE()

#--------------------------------------------------------------------------------#
#  Pass certain values/settings as makefile variables to automake (makefile.am)  #
#--------------------------------------------------------------------------------#

# Note: Building of shared libraries is forced, and we force use of libtool too.

AM_CONDITIONAL( BUILD_FTHREADS,      [ test "$hc_cv_opt_fthreads"     = "yes" ] )
AM_CONDITIONAL( BUILD_HERCIFC,       [ test "$hc_cv_build_hercifc"    = "yes" ] )
AM_CONDITIONAL( OPTION_CAPABILITIES, [ test "$hc_cv_opt_capabilities" = "yes" ] )
AM_CONDITIONAL( SETUID_HERCIFC,      [ test "$hc_cv_setuid_hercifc"   = "yes" ] )
AM_CONDITIONAL( HERCIFC_GROUPSET,    [ test "x$hc_cv_hercifc_groupname" != "x"] )

if test "x$hc_cv_hercifc_groupname" != "x"; then
    HERCIFC_GROUPNAME=${hc_cv_hercifc_groupname}
    AC_SUBST(HERCIFC_GROUPNAME)
fi

AM_CONDITIONAL( BUILD_APPLE_M1,      [ test "$hc_cv_build_apple_m1"   = "yes" ] )

###############################################################################
#                       External Packages support
###############################################################################
#
#  Our first preference for resolving EXTPKG static lib files is whichever
#  directory was specified by the --enable-extpkgs=DIR configure option. If
#  that option wasn't specified, then our second preference is whichever of
#  the directories defined in the LIBRARY_PATH variable is found to contain
#  our needed EXTPKG static lib file (handled further below).  Our defaults
#  are the static lib files distributed in our source  code repository (which
#  should work just fine for most platforms but not necessarily all platforms
#  which is why we provide multiple different ways to override the default).
#
#------------------------------------------------------------------------------
#                             crypto
#------------------------------------------------------------------------------

crypto_pkgname="crypto" # case matters!
crypto_libname="lib${crypto_pkgname}${hc_cv_pkg_lib_suffix}.a"

if test "${hc_cv_extpkg_dir}" != ""; then
  crypto_pkgdir="${hc_cv_extpkg_dir}"
else
  crypto_pkgdir="${srcdir}/${crypto_pkgname}"
fi

crypto_incdir="${crypto_pkgdir}/include"
crypto_libdir="${crypto_pkgdir}/lib${hc_cv_pkg_lib_subdir}"

crypto_headers="${crypto_incdir}/aes.h      \
                ${crypto_incdir}/crypto.h   \
                ${crypto_incdir}/sha1.h     \
                ${crypto_incdir}/sha256.h   \
                ${crypto_incdir}/sshdes.h"

#------------------------------------------------------------------------------
#                            decNumber
#------------------------------------------------------------------------------

decnumber_pkgname="decNumber" # case matters!
decnumber_libname="lib${decnumber_pkgname}${hc_cv_pkg_lib_suffix}.a"

if test "${hc_cv_extpkg_dir}" != ""; then
  decnumber_pkgdir="${hc_cv_extpkg_dir}"
else
  decnumber_pkgdir="${srcdir}/${decnumber_pkgname}"
fi

decnumber_incdir="${decnumber_pkgdir}/include"
decnumber_libdir="${decnumber_pkgdir}/lib${hc_cv_pkg_lib_subdir}"

decnumber_headers="${decnumber_incdir}/aes.h        \
                   ${decnumber_incdir}/decnumber.h  \
                   ${decnumber_incdir}/sha1.h       \
                   ${decnumber_incdir}/sha256.h     \
                   ${decnumber_incdir}/sshdes.h"

#------------------------------------------------------------------------------
#                             SoftFloat
#------------------------------------------------------------------------------

softfloat_pkgname="SoftFloat" # case matters!
softfloat_libname="lib${softfloat_pkgname}${hc_cv_pkg_lib_suffix}.a"

if test "${hc_cv_extpkg_dir}" != ""; then
  softfloat_pkgdir="${hc_cv_extpkg_dir}"
else
  softfloat_pkgdir="${srcdir}/${softfloat_pkgname}"
fi

softfloat_incdir="${softfloat_pkgdir}/include"
softfloat_libdir="${softfloat_pkgdir}/lib${hc_cv_pkg_lib_subdir}"

softfloat_headers="${softfloat_incdir}/aes.h        \
                   ${softfloat_incdir}/softfloat.h  \
                   ${softfloat_incdir}/sha1.h       \
                   ${softfloat_incdir}/sha256.h     \
                   ${softfloat_incdir}/sshdes.h"

#------------------------------------------------------------------------------
#                             telnet
#------------------------------------------------------------------------------

telnet_pkgname="telnet" # case matters!
telnet_libname="lib${telnet_pkgname}${hc_cv_pkg_lib_suffix}.a"

if test "${hc_cv_extpkg_dir}" != ""; then
  telnet_pkgdir="${hc_cv_extpkg_dir}"
else
  telnet_pkgdir="${srcdir}/${telnet_pkgname}"
fi

telnet_incdir="${telnet_pkgdir}/include"
telnet_libdir="${telnet_pkgdir}/lib${hc_cv_pkg_lib_subdir}"

telnet_headers="${telnet_incdir}/aes.h      \
                ${telnet_incdir}/telnet.h   \
                ${telnet_incdir}/sha1.h     \
                ${telnet_incdir}/sha256.h   \
                ${telnet_incdir}/sshdes.h"

#------------------------------------------------------------------------------
# Allow any LIBRARY_PATH variable directory to override our repository default.
#------------------------------------------------------------------------------

if test -z "${hc_cv_extpkg_dir}"; then

  found_crypto=""
  found_decnumber=""
  found_softfloat=""
  found_telnet=""

  while read -d ':' dir; do
    if test -n "${dir}"; then
      if test -z   "${found_crypto}"; then
        if test -r ${dir}/${crypto_libname}; then
                            crypto_libdir="${dir}"
                      found_crypto="yes"
        fi
      fi

      if test -z   "${found_decnumber}"; then
        if test -r ${dir}/${decnumber_libname}; then
                            decnumber_libdir="${dir}"
                      found_decnumber="yes"
        fi
      fi

      if test -z   "${found_softfloat}"; then
        if test -r ${dir}/${softfloat_libname}; then
                            softfloat_libdir="${dir}"
                      found_softfloat="yes"
        fi
      fi

      if test -z   "${found_telnet}"; then
        if test -r ${dir}/${telnet_libname}; then
                            telnet_libdir="${dir}"
                      found_telnet="yes"
        fi
      fi
    fi

    # Note the trailing ':' in the below LIBRARY_PATH statement
    # to ensure that the last directory in the list is always
    # processed.  Also note that the statements being passed to
    # the while loop (i.e. those between the "<<END" clause and
    # the "END" statement) should not have any # line comments
    # either, since they too would be passed to the while loop.

  done <<END
${LIBRARY_PATH}:
END
fi

LIBS="$LIBS -l${crypto_pkgname}${hc_cv_pkg_lib_suffix}"
LDFLAGS="$LDFLAGS -L${crypto_libdir}"

LIBS="$LIBS -l${decnumber_pkgname}${hc_cv_pkg_lib_suffix}"
LDFLAGS="$LDFLAGS -L${decnumber_libdir}"


LIBS="$LIBS -l${softfloat_pkgname}${hc_cv_pkg_lib_suffix}"
LDFLAGS="$LDFLAGS -L${softfloat_libdir}"

LIBS="$LIBS -l${telnet_pkgname}${hc_cv_pkg_lib_suffix}"
LDFLAGS="$LDFLAGS -L${telnet_libdir}"

###############################################################################
#          Pass values to other subdirectory Makefile.am files
###############################################################################

extpkg_incdirs="-I ${crypto_incdir}     \
                -I ${decnumber_incdir}  \
                -I ${softfloat_incdir}  \
                -I ${telnet_incdir}"

extpkg_headers="${crypto_headers}       \
                ${decnumber_headers}    \
                ${softfloat_headers}    \
                ${telnet_headers}"

# extpkg_ldadd="${crypto_libdir}/${crypto_libname}          \
#               ${decnumber_libdir}/${decnumber_libname}    \
#               ${softfloat_libdir}/${softfloat_libname}    \
#               ${telnet_libdir}/${telnet_libname}"
extpkg_ldadd=

##----------------------------------------------------------------
##                         DEBUGGING
##----------------------------------------------------------------
## AC_MSG_NOTICE( [Using crypto_libdir    = ${crypto_libdir}]    )
## AC_MSG_NOTICE( [Using decnumber_libdir = ${decnumber_libdir}] )
## AC_MSG_NOTICE( [Using softfloat_libdir = ${softfloat_libdir}] )
## AC_MSG_NOTICE( [Using telnet_libdir    = ${telnet_libdir}]    )

## AC_MSG_NOTICE( [LIBS    = ${LIBS}]    )
## AC_MSG_NOTICE( [LDFLAGS = ${LDFLAGS}]    )
## AC_MSG_NOTICE( [extpkg_incdirs = ${extpkg_incdirs}]    )

EXTPKG_INCDIRS="${extpkg_incdirs}"
AC_SUBST(EXTPKG_INCDIRS)

EXTPKG_HEADERS="${extpkg_headers}"
AC_SUBST(EXTPKG_HEADERS)

EXTPKG_LDADD="${extpkg_ldadd}"
AC_SUBST(EXTPKG_LDADD)

#------------------------------------------------------------------------------

HQA_INC="${hc_cv_hqa_inc}"
AC_SUBST(HQA_INC)

#------------------------------------------------------------------------------

AC_CACHE_SAVE()

AC_OUTPUT([  Makefile  util/Makefile  html/Makefile  man/Makefile  m4/Makefile  ])

dnl Kludge to fix Debian/Ubuntu libtool
sed -e 's/^link_all_deplibs=no$/link_all_deplibs=yes/' <libtool >libtool.tmp
mv libtool.tmp libtool

###############################################################################
#   (end-of-file)
###############################################################################
