#------------------------------------------------------------------------------
# Makefile for Hercules S/370, ESA/390 and z/Architecture emulator
# (C) Copyright Roger Bowler, 1999-2012
# (C) and others 2013-2023
# Process this file with 'automake' to produce Makefile.in
#------------------------------------------------------------------------------

AUTOMAKE_OPTIONS = foreign 1.5

#------------------------------------------------------------------------------
extpkg_incdirs = @EXTPKG_INCDIRS@
extpkg_headers = @EXTPKG_HEADERS@
extpkg_ldadd   = @EXTPKG_LDADD@
hqa_inc        = @HQA_INC@
lns            = @LN_S@
ldadd          = @LIBS@

#------------------------------------------------------------------------------
# Note! Backslash line continuation is disallowed by newer versions of aclocal
#------------------------------------------------------------------------------

ACLOCAL_AMFLAGS = -I autoconf -I m4

#------------------------------------------------------------------------------

AM_CPPFLAGS =       \
  -I $(top_srcdir)  \
  -I $(hqa_inc)

AM_CPPFLAGS += $(extpkg_incdirs)

#------------------------------------------------------------------------------
# Note! Due to inter-module dependencies the sequence
#       (ordering) of the SUBDIRS list is important!
#------------------------------------------------------------------------------

SUBDIRS =     \
  m4          \
  util        \
  html        \
  man         \
  .

#------------------------------------------------------------------------------
# Invoke shell script to dynamically define version variables
#------------------------------------------------------------------------------

VERS_MAJ := $(shell $(top_srcdir)/_dynamic_version $(top_srcdir) VERS_MAJ)
VERS_INT := $(shell $(top_srcdir)/_dynamic_version $(top_srcdir) VERS_INT)
VERS_MIN := $(shell $(top_srcdir)/_dynamic_version $(top_srcdir) VERS_MIN)
VERS_BLD := $(shell $(top_srcdir)/_dynamic_version $(top_srcdir) VERS_BLD)

  # PROGRAMMING NOTE: Due to autotool's insistance of defining 'VERSION'
  # for us via a #define within the 'config.h' header (the value of which
  # is derives from the 'configure.ac's "AM_INIT_AUTOMAKE" statement)
  # instead of letting us define it ourselves (which we would prefer),
  # we must undefine it here and then redefine it to the value that our
  # '_dynamic_version' script determied it should be set to (which it saved
  # for us in the 'DYNAMIC_VERSION' variable for obvious reasons).

DYNAMIC_VERSION := $(shell $(top_srcdir)/_dynamic_version $(top_srcdir) VERSION )

# Now simply pass those version defines on the command-line...

AM_CPPFLAGS += -D DYNAMIC_VERSION=\"$(DYNAMIC_VERSION)\" -D VERS_MAJ=$(VERS_MAJ) -D VERS_INT=$(VERS_INT) -D VERS_MIN=$(VERS_MIN) -D VERS_BLD=$(VERS_BLD)

#------------------------------------------------------------------------------

if BUILD_HERCIFC
HERCIFC = hercifc
else
HERCIFC =
endif

HERCLIN = herclin

fthreads_SRC  = fthreads.c

if BUILD_FTHREADS
FTHREADS = $(fthreads_SRC)
else
FTHREADS =
endif

#------------------------------------------------------------------------------
#  We need to still include the source for the dynamic modules
#  in with the distribution regardless of whether or not the dyanmic
#  modules themselves are to be built so if dynamic load not possible,
#  then we need to include the source on the EXTRA_hercules_SOURCES
#  statement so even though they won't/don't actually get built, they
#  still nonetheless get included into distribution tarball. This is
#  *similar* to (bit not identical to) the way the fthreads sources
#  are handled. With the fthreads sources though, they can at least always
#  be built (even though they may not always generate any actual code
#  whenever they do) whereas the dynamic module source CANNOT (MUST not)
#  even be *attempted* to be "built" if support for building dynamic modules
#  doesn't even exist on the system doing the building.
#------------------------------------------------------------------------------

dynamic_SRC = \
  dyncrypt.c  \
  dyngui.c    \
  hdteq.c

dyndev_SRC =  \
  awstape.c   \
  cardpch.c   \
  cardrdr.c   \
  comm3705.c  \
  commadpt.c  \
  tcpnje.c    \
  con1052c.c  \
  console.c   \
  ctc_ctci.c  \
  ctc_lcs.c   \
  ctc_ptp.c   \
  ctcadpt.c   \
  faketape.c  \
  ftlib.c     \
  hchan.c     \
  hetlib.c    \
  hettape.c   \
  mpc.c       \
  netsupp.c   \
  omatape.c   \
  printer.c   \
  qeth.c      \
  resolve.c   \
  scsitape.c  \
  sllib.c     \
  sockdev.c   \
  tapeccws.c  \
  tapedev.c   \
  tuntap.c    \
  dummydev.c  \
  zfcp.c

#-----------------------------------------------------------------
# -module         :  create a dlopen'able module
# -no-undefined   :  required on all platform that do not allow
#                    shared modules to have undefined symbols
# $(ldadd)        :  Misc Linker flags set by autoconf
# -export-dynamic :  so dlsym works (to be verified -
#                    not sure whether it is necessary)
# -avoid-version  :  needed.. Otherwise libtool gives crazy names
#                    to Windows DLLs
#-----------------------------------------------------------------

XSTATIC =
DYNSRC  =

DYNMOD_LD_FLAGS =   \
  -module           \
  -no-undefined     \
  $(XSTATIC)        \
  -export-dynamic   \
  -avoid-version

DYNMOD_LD_ADD =     \
  libherc.la        \
  libhercs.la       \
  libhercu.la       \
  $(extpkg_ldadd)   \
  $(ldadd)

LIB_LD_FLAGS =      \
  -export-dynamic   \
   $(XSTATIC)       \
   -no-undefined    \
   -avoid-version

#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
# List of Libtool shared libraries & loadable modules
#------------------------------------------------------------------------------

HERCLIBS =

HERCLIBS2 =     \
  libhercs.la   \
  libhercu.la   \
  libherct.la   \
  libhercd.la   \
  libherc.la    \
  libhdt3420_not_mod.la

HERCMODS =      \
  dyncrypt.la   \
  dyngui.la     \
  hdteq.la      \
  hdtptp.la     \
  hdtqeth.la    \
  hdtzfcp.la    \
  hdt1052c.la   \
  hdt1403.la    \
  hdt2703.la    \
  hdttcpnje.la  \
  hdt3705.la    \
  hdt3088.la    \
  hdt3270.la    \
  hdt3420.la    \
  hdt3505.la    \
  hdt2880.la    \
  hdtdummy.la   \
  hdt3525.la

modexec_LTLIBRARIES = $(HERCMODS)
noinst_LTLIBRARIES  = $(HERCLIBS)
lib_LTLIBRARIES     = $(HERCLIBS2)

#------------------------------------------------------------------------------
# The following causes the dynamic modules to not even get built
# unless support for building dynamic modules exists on the build system
#------------------------------------------------------------------------------

bin_PROGRAMS = \
  cckdcdsk     \
  cckdcomp     \
  cckddiag     \
  cckdswap     \
  cckdcdsk64   \
  cckdcomp64   \
  cckddiag64   \
  cckdswap64   \
  convto64     \
  cckdmap      \
  dasdcat      \
  dasdconv     \
  dasdcopy     \
  dasdinit     \
  dasdconv64   \
  dasdcopy64   \
  dasdinit64   \
  dasdisup     \
  dasdload     \
  dasdload64   \
  dasdls       \
  dasdpdsu     \
  dasdseq      \
  dasdser      \
  dmap2hrc     \
  hercules     \
  hetget       \
  hetinit      \
  hetmap       \
  hetupd       \
  maketape     \
  tapecopy     \
  tapemap      \
  tapesplt     \
  tfprint      \
  tfswap       \
  vmfplc2      \
  $(HERCIFC)   \
  $(HERCLIN)

EXTRA_PROGRAMS = hercifc

#----------------------------------------------------------------------------
# For each module:
#
#   ModuleName_la_SOURCES      = <list of source files>
#   ModuleName_la_LDFLAGS      = $(DYNMOD_LD_FLAGS) (see above)
#   ModuleName_la_LIBADD       = libherc.la (the Core Hercules Shared Library)
#   ModuleName_la_DEPENDENCIES = libherc.la (may not be necessary)
#----------------------------------------------------------------------------

dyncrypt_la_SOURCES = dyncrypt.c
dyncrypt_la_LDFLAGS = $(DYNMOD_LD_FLAGS)
dyncrypt_la_LIBADD  = $(DYNMOD_LD_ADD)

dyngui_la_SOURCES   = dyngui.c
dyngui_la_LDFLAGS   = $(DYNMOD_LD_FLAGS)
dyngui_la_LIBADD    = $(DYNMOD_LD_ADD)

hdteq_la_SOURCES    = hdteq.c
hdteq_la_LDFLAGS    = $(DYNMOD_LD_FLAGS)
hdteq_la_LIBADD     = $(DYNMOD_LD_ADD)

hdt1403_la_SOURCES  = printer.c sockdev.c
hdt1403_la_LDFLAGS  = $(DYNMOD_LD_FLAGS)
hdt1403_la_LIBADD   = $(DYNMOD_LD_ADD)

hdt2880_la_SOURCES  = hchan.c
hdt2880_la_LDFLAGS  = $(DYNMOD_LD_FLAGS)
hdt2880_la_LIBADD   = $(DYNMOD_LD_ADD)

hdt2703_la_SOURCES  = commadpt.c
hdt2703_la_LDFLAGS  = $(DYNMOD_LD_FLAGS)
hdt2703_la_LIBADD   = $(DYNMOD_LD_ADD)

hdttcpnje_la_SOURCES = tcpnje.c
hdttcpnje_la_LDFLAGS = $(DYNMOD_LD_FLAGS)
hdttcpnje_la_LIBADD  = $(DYNMOD_LD_ADD)

hdt3705_la_SOURCES  = comm3705.c
hdt3705_la_LDFLAGS  = $(DYNMOD_LD_FLAGS)
hdt3705_la_LIBADD   = $(DYNMOD_LD_ADD)

hdt3088_la_SOURCES  = ctc_lcs.c ctc_ctci.c ctcadpt.c tuntap.c netsupp.c
hdt3088_la_LDFLAGS  = $(DYNMOD_LD_FLAGS)
hdt3088_la_LIBADD   = $(DYNMOD_LD_ADD)

hdt3270_la_SOURCES  = console.c
hdt3270_la_LDFLAGS  = $(DYNMOD_LD_FLAGS)
hdt3270_la_LIBADD   = $(DYNMOD_LD_ADD)

hdt3420_la_SOURCES  = tapedev.c tapeccws.c awstape.c faketape.c hettape.c omatape.c scsitape.c scsiutil.c
hdt3420_la_LDFLAGS  = $(DYNMOD_LD_FLAGS)
hdt3420_la_LIBADD   = $(DYNMOD_LD_ADD) libherct.la

hdt3505_la_SOURCES  = cardrdr.c sockdev.c
hdt3505_la_LDFLAGS  = $(DYNMOD_LD_FLAGS)
hdt3505_la_LIBADD   = $(DYNMOD_LD_ADD)

hdt3525_la_SOURCES  = cardpch.c
hdt3525_la_LDFLAGS  = $(DYNMOD_LD_FLAGS)
hdt3525_la_LIBADD   = $(DYNMOD_LD_ADD)

hdtqeth_la_SOURCES  = qeth.c mpc.c resolve.c tuntap.c
hdtqeth_la_LDFLAGS  = $(DYNMOD_LD_FLAGS)
hdtqeth_la_LIBADD   = $(DYNMOD_LD_ADD)

hdtzfcp_la_SOURCES  = zfcp.c
hdtzfcp_la_LDFLAGS  = $(DYNMOD_LD_FLAGS)
hdtzfcp_la_LIBADD   = $(DYNMOD_LD_ADD)

hdt1052c_la_SOURCES = con1052c.c
hdt1052c_la_LDFLAGS = $(DYNMOD_LD_FLAGS)
hdt1052c_la_LIBADD  = $(DYNMOD_LD_ADD)

hdtptp_la_SOURCES   = ctc_ptp.c mpc.c resolve.c tuntap.c
hdtptp_la_LDFLAGS   = $(DYNMOD_LD_FLAGS)
hdtptp_la_LIBADD    = $(DYNMOD_LD_ADD)

hdtdummy_la_SOURCES = dummydev.c
hdtdummy_la_LDFLAGS = $(DYNMOD_LD_FLAGS)
hdtdummy_la_LIBADD  = $(DYNMOD_LD_ADD)

#--------------------------------------------
# Common data areas (shared) library
#--------------------------------------------

libhercs_la_SOURCES = hsys.c
libhercs_la_LDFLAGS = $(LIB_LD_FLAGS)
libhercs_la_LIBADD  = $(ldadd)

#--------------------------------------------
# Tape utility subroutines (shared) library
#--------------------------------------------

libherct_la_SOURCES = \
  hetlib.c            \
  ftlib.c             \
  sllib.c

libherct_la_LDFLAGS = $(LIB_LD_FLAGS)

libherct_la_LIBADD = \
  $(ldadd)           \
  libhercs.la        \
  libhercu.la

#--------------------------------------------
# DASD utility subroutines (shared) library
#--------------------------------------------

libhercd_la_SOURCES = \
  cache.c             \
  cckddasd.c          \
  cckdutil.c          \
  ckddasd.c           \
  cckddasd64.c        \
  cckdutil64.c        \
  ckddasd64.c         \
  dasdtab.c           \
  dasdutil.c          \
  fbadasd.c           \
  dasdutil64.c        \
  fbadasd64.c         \
  shared.c

libhercd_la_LDFLAGS = $(LIB_LD_FLAGS)

libhercd_la_LIBADD = \
  $(ldadd)           \
  libhercs.la        \
  libhercu.la

#--------------------------------------------
# Pure Utility functions
#--------------------------------------------

libhercu_la_SOURCES = \
  codepage.c          \
  hdl.c               \
  hexdumpe.c          \
  hostinfo.c          \
  hscutl.c            \
  hsocket.c           \
  hthreads.c          \
  logger.c            \
  logmsg.c            \
  machdep.c           \
  memrchr.c           \
  parser.c            \
  pttrace.c           \
  version.c           \
  $(FTHREADS)

libhercu_la_LDFLAGS = $(LIB_LD_FLAGS)
libhercu_la_LIBADD =  \
  $(extpkg_ldadd)     \
  $(ldadd)            \
  libhercs.la

libhdt3420_not_mod_la_SOURCES  =  \
  tapedev.c                       \
  tapeccws.c                      \
  awstape.c                       \
  faketape.c                      \
  hettape.c                       \
  omatape.c                       \
  scsitape.c                      \
  scsiutil.c
libhdt3420_not_mod_la_LDFLAGS  =  $(LIB_LD_FLAGS)
libhdt3420_not_mod_la_LIBADD   =  \
  $(ldadd)                        \
  libherct.la                     \
  libhercs.la                     \
  libhercu.la                     \
  libherc.la                      \
  libherct.la

#--------------------------------------------
# Core Hercules (shared) library
#--------------------------------------------

libherc_la_SOURCES = \
  _archdep_templ.c   \
  archlvl.c          \
  assist.c           \
  bldcfg.c           \
  cgibin.c           \
  channel.c          \
  chsc.c             \
  clock.c            \
  cmdtab.c           \
  cmpsc_2012.c       \
  cmpscdbg.c         \
  cmpscdct.c         \
  cmpscget.c         \
  cmpscmem.c         \
  cmpscput.c         \
  config.c           \
  control.c          \
  cpu.c              \
  crypto.c           \
  dat.c              \
  decimal.c          \
  dfp.c              \
  diagmssf.c         \
  diagnose.c         \
  dyn76.c            \
  ecpsvm.c           \
  esame.c            \
  external.c         \
  facility.c         \
  fillfnam.c         \
  float.c            \
  general1.c         \
  general2.c         \
  general3.c         \
  hao.c              \
  hbyteswp.c         \
  hconsole.c         \
  hdiagf18.c         \
  history.c          \
  hRexx.c            \
  hRexx_o.c          \
  hRexx_r.c          \
  hsccmd.c           \
  hscemode.c         \
  hscloc.c           \
  hscmisc.c          \
  hscpufun.c         \
  httpserv.c         \
  ieee.c             \
  impl.c             \
  inline.c           \
  io.c               \
  ipl.c              \
  loadmem.c          \
  loadparm.c         \
  losc.c             \
  machchk.c          \
  machdep.c          \
  opcode.c           \
  panel.c            \
  pfpo.c             \
  plo.c              \
  qdio.c             \
  scedasd.c          \
  scescsi.c          \
  script.c           \
  service.c          \
  sie.c              \
  skey.c             \
  sr.c               \
  stack.c            \
  strsignal.c        \
  tcpip.c            \
  timer.c            \
  trace.c            \
  transact.c         \
  vector.c           \
  vm.c               \
  vmd250.c           \
  vstore.c           \
  x75.c              \
  xstore.c           \
  $(DYNSRC)

EXTRA_libherc_la_SOURCES = \
  memrchr.c          \
  $(dynamic_SRC)     \
  $(dyndev_SRC)      \
  $(extra_SRC)       \
  $(fthreads_SRC)

libherc_la_LDFLAGS = $(LIB_LD_FLAGS)

libherc_la_LIBADD = \
  libhercs.la       \
  libhercu.la       \
  libherct.la       \
  libhercd.la       \
  $(extpkg_ldadd)   \
  $(ldadd)

#--------------------------------------------------------
#  THIS is the hercules executable.
#--------------------------------------------------------

HLDFLAGS = -dlopen self
HDEPS    = $(extpkg_ldadd)

hercules_SOURCES = \
  bootstrap.c

hercules_LDADD =  \
  libherc.la      \
  libhercs.la     \
  $(extpkg_ldadd) \
  $(ldadd)

hercules_LDFLAGS = $(HLDFLAGS)

hercules_DEPENDENCIES = \
  libherc.la            \
  libhercs.la           \
  $(HDEPS)

herclin_SOURCES = \
  herclin.c

herclin_LDADD =   \
  libherc.la      \
  libhercs.la     \
  $(extpkg_ldadd) \
  $(ldadd)

herclin_LDFLAGS = $(HLDFLAGS)

herclin_DEPENDENCIES = \
  libherc.la           \
  libhercs.la          \
  $(HDEPS)

#------------------------------------------------------------------------------
# Side binaries...  (i.e. our hercules utilities)
#------------------------------------------------------------------------------

tools_LD_FLAGS =

tools_ADDLIBS = $(HERCLIBS2) $(ldadd)

if BUILD_HERCIFC
hercifc_SOURCES    = hercifc.c
hercifc_LDADD      = $(tools_ADDLIBS)
hercifc_LDFLAGS    = $(tools_LD_FLAGS)
endif

dasdinit_SOURCES   = dasdinit.c
dasdinit_LDADD     = $(tools_ADDLIBS)
dasdinit_LDFLAGS   = $(tools_LD_FLAGS)

dasdinit64_SOURCES = dasdinit64.c
dasdinit64_LDADD   = $(tools_ADDLIBS)
dasdinit64_LDFLAGS = $(tools_LD_FLAGS)

dasdisup_SOURCES   = dasdisup.c
dasdisup_LDADD     = $(tools_ADDLIBS)
dasdisup_LDFLAGS   = $(tools_LD_FLAGS)

dasdload_SOURCES   = dasdload.c
dasdload_LDADD     = $(tools_ADDLIBS)
dasdload_LDFLAGS   = $(tools_LD_FLAGS)

dasdload64_SOURCES = dasdload64.c
dasdload64_LDADD   = $(tools_ADDLIBS)
dasdload64_LDFLAGS = $(tools_LD_FLAGS)

dasdconv_SOURCES   = dasdconv.c
dasdconv_LDADD     = $(tools_ADDLIBS)
dasdconv_LDFLAGS   = $(tools_LD_FLAGS)

dasdconv64_SOURCES = dasdconv64.c
dasdconv64_LDADD   = $(tools_ADDLIBS)
dasdconv64_LDFLAGS = $(tools_LD_FLAGS)

dasdls_SOURCES     = dasdls.c
dasdls_LDADD       = $(tools_ADDLIBS)
dasdls_LDFLAGS     = $(tools_LD_FLAGS)

dasdcat_SOURCES    = dasdcat.c
dasdcat_LDADD      = $(tools_ADDLIBS)
dasdcat_LDFLAGS    = $(tools_LD_FLAGS)

dasdpdsu_SOURCES   = dasdpdsu.c
dasdpdsu_LDADD     = $(tools_ADDLIBS)
dasdpdsu_LDFLAGS   = $(tools_LD_FLAGS)

dasdseq_SOURCES    = dasdseq.c
dasdseq_LDADD      = $(tools_ADDLIBS)
dasdseq_LDFLAGS    = $(tools_LD_FLAGS)

dasdser_SOURCES    = dasdser.c
dasdser_LDADD      = $(tools_ADDLIBS)
dasdser_LDFLAGS    = $(tools_LD_FLAGS)

maketape_SOURCES   = maketape.c
maketape_LDADD     = $(tools_ADDLIBS)
maketape_LDFLAGS   = $(tools_LD_FLAGS)

tapecopy_SOURCES   = tapecopy.c scsiutil.c
tapecopy_LDADD     = $(tools_ADDLIBS)
tapecopy_LDFLAGS   = $(tools_LD_FLAGS)
tapecopy_CFLAGS    = $(AM_CFLAGS)

tapemap_SOURCES    = tapemap.c
tapemap_LDADD      = $(tools_ADDLIBS)
tapemap_LDFLAGS    = $(tools_LD_FLAGS)

tapesplt_SOURCES   = tapesplt.c
tapesplt_LDADD     = $(tools_ADDLIBS)
tapesplt_LDFLAGS   = $(tools_LD_FLAGS)

cckdcdsk_SOURCES   = cckdcdsk.c
cckdcdsk_LDADD     = $(tools_ADDLIBS)
cckdcdsk_LDFLAGS   = $(tools_LD_FLAGS)

cckdcomp_SOURCES   = cckdcomp.c
cckdcomp_LDADD     = $(tools_ADDLIBS)
cckdcomp_LDFLAGS   = $(tools_LD_FLAGS)

cckddiag_SOURCES   = cckddiag.c
cckddiag_LDADD     = $(tools_ADDLIBS)
cckddiag_LDFLAGS   = $(tools_LD_FLAGS)

dasdcopy_SOURCES   = dasdcopy.c
dasdcopy_LDADD     = $(tools_ADDLIBS)
dasdcopy_LDFLAGS   = $(tools_LD_FLAGS)

cckdswap_SOURCES   = cckdswap.c
cckdswap_LDADD     = $(tools_ADDLIBS)
cckdswap_LDFLAGS   = $(tools_LD_FLAGS)

cckdcdsk64_SOURCES = cckdcdsk64.c
cckdcdsk64_LDADD   = $(tools_ADDLIBS)
cckdcdsk64_LDFLAGS = $(tools_LD_FLAGS)

cckdcomp64_SOURCES = cckdcomp64.c
cckdcomp64_LDADD   = $(tools_ADDLIBS)
cckdcomp64_LDFLAGS = $(tools_LD_FLAGS)

cckddiag64_SOURCES = cckddiag64.c
cckddiag64_LDADD   = $(tools_ADDLIBS)
cckddiag64_LDFLAGS = $(tools_LD_FLAGS)

dasdcopy64_SOURCES = dasdcopy64.c
dasdcopy64_LDADD   = $(tools_ADDLIBS)
dasdcopy64_LDFLAGS = $(tools_LD_FLAGS)

cckdswap64_SOURCES = cckdswap64.c
cckdswap64_LDADD   = $(tools_ADDLIBS)
cckdswap64_LDFLAGS = $(tools_LD_FLAGS)

cckdmap_SOURCES    = cckdmap.c
cckdmap_LDADD      = $(tools_ADDLIBS)
cckdmap_LDFLAGS    = $(tools_LD_FLAGS)

convto64_SOURCES   = convto64.c
convto64_LDADD     = $(tools_ADDLIBS)
convto64_LDFLAGS   = $(tools_LD_FLAGS)

hetget_SOURCES     = hetget.c
hetget_LDADD       = $(tools_ADDLIBS)
hetget_LDFLAGS     = $(tools_LD_FLAGS)

hetinit_SOURCES    = hetinit.c
hetinit_LDADD      = $(tools_ADDLIBS)
hetinit_LDFLAGS    = $(tools_LD_FLAGS)

hetmap_SOURCES     = hetmap.c
hetmap_LDADD       = $(tools_ADDLIBS)
hetmap_LDFLAGS     = $(tools_LD_FLAGS)

hetupd_SOURCES     = hetupd.c
hetupd_LDADD       = $(tools_ADDLIBS)
hetupd_LDFLAGS     = $(tools_LD_FLAGS)

dmap2hrc_SOURCES   = dmap2hrc.c
dmap2hrc_LDADD     = $(tools_ADDLIBS)
dmap2hrc_LDFLAGS   = $(tools_LD_FLAGS)

vmfplc2_SOURCES    = vmfplc2.c
vmfplc2_LDADD      = $(tools_ADDLIBS) libhdt3420_not_mod.la
vmfplc2_LDFLAGS    = $(tools_LD_FLAGS)

tfprint_SOURCES    = tfprint.c
tfprint_LDADD      = $(tools_ADDLIBS)
tfprint_LDFLAGS    = $(tools_LD_FLAGS)

tfswap_SOURCES     = tfswap.c
tfswap_LDADD       = $(tools_ADDLIBS)
tfswap_LDFLAGS     = $(tools_LD_FLAGS)

#------------------------------------------------------------------------------
# Files that are not 'built' per-se
# Also contains some WIN32 only source files
#------------------------------------------------------------------------------

EXTRA_DIST =                  \
  .gitattributes              \
  .gitignore                  \
  .travis.yml                 \
  _dynamic_version            \
  _dynamic_version.cmd        \
  _TODO.txt                   \
  all-all.jobs                \
  all-all.msbuild.jobs        \
  CODEOWNERS                  \
  debug-all.msbuild.jobs      \
  retail-all.msbuild.jobs     \
  vstools.cmd                 \
  autoconf/config.guess       \
  autoconf/config.rpath       \
  autoconf/config.sub         \
  autoconf/depcomp            \
  autoconf/hercules.m4        \
  autoconf/install-sh         \
  autoconf/libtool.m4         \
  autoconf/ltargz.m4          \
  autoconf/ltdl.m4            \
  autoconf/ltmain.sh          \
  autoconf/ltoptions.m4       \
  autoconf/ltsugar.m4         \
  autoconf/ltversion.m4       \
  autoconf/lt~obsolete.m4     \
  autoconf/missing            \
  autoconf/mkinstalldirs      \
  autoconf/README             \
  autogen.sh                  \
  bldlvlck                    \
  build_pch.c                 \
  BUILDING                    \
  CHANGES                     \
  configure.ac                \
  conspawn.c                  \
  COPYRIGHT                   \
  debug-all.jobs              \
  dynmake.bat                 \
  getopt.c                    \
  hercdasd.ico                \
  hercdasd.rc                 \
  herclogo.txt                \
  hercmisc.ico                \
  hercmisc.rc                 \
  hercprod.rc                 \
  herctape.ico                \
  herctape.rc                 \
  hercules.cnf                \
  hercules.cnf.rexx           \
  hercules.ico                \
  hercules.m4                 \
  Hercules_VS2008.sln         \
  Hercules_VS2008.vcproj      \
  Hercules_VS2015.sln              \
  Hercules_VS2015.vcxproj          \
  Hercules_VS2015.vcxproj.filters  \
  Hercules_VS2017.sln              \
  Hercules_VS2017.vcxproj          \
  Hercules_VS2017.vcxproj.filters  \
  Hercules_VS2019.sln              \
  Hercules_VS2019.vcxproj          \
  Hercules_VS2019.vcxproj.filters  \
  Hercules_VS2022.sln              \
  Hercules_VS2022.vcxproj          \
  Hercules_VS2022.vcxproj.filters  \
  hercver.rc2                 \
  INSTALL                     \
  license_dyn76.txt           \
  makefile-dllmod.msvc        \
  Makefile.am                 \
  makefile.bat                \
  makefile.msvc               \
  msvc.makefile.includes/VSVERS.msvc          \
  msvc.makefile.includes/TARGETVER.msvc       \
  msvc.makefile.includes/CONFIG.msvc          \
  msvc.makefile.includes/OUTPUT_DIRS.msvc     \
  msvc.makefile.includes/MODULES.msvc         \
  msvc.makefile.includes/OBJ_CODE.msvc        \
  msvc.makefile.includes/ZLIB_DIR.msvc        \
  msvc.makefile.includes/BZIP2_DIR.msvc       \
  msvc.makefile.includes/PCRE_DIR.msvc        \
  msvc.makefile.includes/REXX_DIRS.msvc       \
  msvc.makefile.includes/HQA_DIR.msvc         \
  msvc.makefile.includes/DEBUG_RETAIL.msvc    \
  msvc.makefile.includes/ZLIB_FLAGS.msvc      \
  msvc.makefile.includes/BZIP2_FLAGS.msvc     \
  msvc.makefile.includes/PCRE_FLAGS.msvc      \
  msvc.makefile.includes/REXX_FLAGS.msvc      \
  msvc.makefile.includes/IPV6_FLAGS.msvc      \
  msvc.makefile.includes/HQA_FLAGS.msvc       \
  msvc.makefile.includes/HERC_FLAGS.msvc      \
  msvc.makefile.includes/VERSION.msvc         \
  msvc.makefile.includes/EXT_PKGS.msvc        \
  msvc.makefile.includes/PRIM_RULES.msvc      \
  msvc.makefile.includes/OUTDIR_RULES.msvc    \
  msvc.makefile.includes/MOD_RULES1.msvc      \
  msvc.makefile.includes/MOD_RULES2.msvc      \
  msvc.makefile.includes/ZLIB_RULES.msvc      \
  msvc.makefile.includes/BZIP2_RULES.msvc     \
  msvc.makefile.includes/PCRE_RULES.msvc      \
  msvc.makefile.includes/REXX_RULES.msvc      \
  README.md                       \
  readme/images/herclogo.jpg      \
  readme/images/image_header_herculeshyperionSDL.png  \
  readme/README.ADDING.md         \
  readme/README.AIX.md            \
  readme/README.APL360.md         \
  readme/README.BSD.md            \
  readme/README.CCKD64.md         \
  readme/README.CMPSC.md          \
  readme/README.COMMADPT.md       \
  readme/README.DYNMOD.md         \
  readme/README.ECPSVM.md         \
  readme/README.EXTPKG.md         \
  readme/README.FACILITIES.md     \
  readme/README.HAO.md            \
  readme/README.HDL.md            \
  readme/README.HERCLOGO.md       \
  readme/README.HQA.md            \
  readme/README.HRAF.md           \
  readme/README.ISSUES.md         \
  readme/README.MAKETAPE.md       \
  readme/README.MINGW.md          \
  readme/README.MSVC.md           \
  readme/README.NETWORKING.md     \
  readme/README.OSX.md            \
  readme/README.PTT.md            \
  readme/README.REXX.md           \
  readme/README.RUNTEST.md        \
  readme/README.RXVT4APL.md       \
  readme/README.S37X.md           \
  readme/README.SETUID.md         \
  readme/README.SNA.md            \
  readme/README.SUN.md            \
  readme/README.TAPE.md           \
  readme/README.TCPIP.md          \
  readme/README.TCPNJE.md         \
  readme/README.VMFPLC2.md        \
  readme/README.WIN64.md          \
  RELEASE.NOTES               \
  retail-all.jobs             \
  scripts/cmpsc.cnf           \
  scripts/cmpsc.rexx          \
  scripts/hbasics             \
  scripts/hbasic1.rexx        \
  scripts/hbasic2.rex         \
  scripts/hbasic3.cmd         \
  scripts/hbasic4.rx          \
  scripts/hbasic5.ext1        \
  scripts/hcommand.rexx       \
  scripts/hexecio.rexx        \
  scripts/hlinein.rexx        \
  scripts/hrecurs.rexx        \
  crypto/crypto.LICENSE.txt               \
  crypto/crypto.README.txt                \
  crypto/include/crypto.h                 \
  crypto/include/crypto_version.h         \
  crypto/include/rijndael.h               \
  crypto/include/sha1.h                   \
  crypto/include/sha2.h                   \
  crypto/include/sshdes.h                 \
  crypto/lib/crypto32.lib                 \
  crypto/lib/crypto32.pdb                 \
  crypto/lib/crypto32d.lib                \
  crypto/lib/crypto32d.pdb                \
  crypto/lib/crypto64.lib                 \
  crypto/lib/crypto64.pdb                 \
  crypto/lib/crypto64d.lib                \
  crypto/lib/crypto64d.pdb                \
  crypto/lib/libcrypto32.a                \
  crypto/lib/libcrypto32d.a               \
  crypto/lib/libcrypto64.a                \
  crypto/lib/libcrypto64d.a               \
  decNumber/decnumber.ERRATA              \
  decNumber/decnumber.ICU-license.html    \
  decNumber/decnumber.pdf                 \
  decNumber/decnumber.readme.txt          \
  decNumber/include/decContext.h          \
  decNumber/include/decNumber.h           \
  decNumber/include/decPacked.h           \
  decNumber/include/decQuad.h             \
  decNumber/include/decimal128.h          \
  decNumber/include/decimal32.h           \
  decNumber/include/decimal64.h           \
  decNumber/include/decnumber_version.h   \
  decNumber/lib/decNumber32.lib           \
  decNumber/lib/decNumber32.pdb           \
  decNumber/lib/decNumber32d.lib          \
  decNumber/lib/decNumber32d.pdb          \
  decNumber/lib/decNumber64.lib           \
  decNumber/lib/decNumber64.pdb           \
  decNumber/lib/decNumber64d.lib          \
  decNumber/lib/decNumber64d.pdb          \
  decNumber/lib/libdecNumber32.a          \
  decNumber/lib/libdecNumber32d.a         \
  decNumber/lib/libdecNumber64.a          \
  decNumber/lib/libdecNumber64d.a         \
  SoftFloat/doc/SoftFloat-history.html    \
  SoftFloat/doc/SoftFloat-source.html     \
  SoftFloat/doc/SoftFloat.html            \
  SoftFloat/include/softfloat.h           \
  SoftFloat/include/softfloat_types.h     \
  SoftFloat/include/softfloat_version.h   \
  SoftFloat/lib/SoftFloat32.lib           \
  SoftFloat/lib/SoftFloat32.pdb           \
  SoftFloat/lib/SoftFloat32d.lib          \
  SoftFloat/lib/SoftFloat32d.pdb          \
  SoftFloat/lib/SoftFloat64.lib           \
  SoftFloat/lib/SoftFloat64.pdb           \
  SoftFloat/lib/SoftFloat64d.lib          \
  SoftFloat/lib/SoftFloat64d.pdb          \
  SoftFloat/lib/libSoftFloat32.a          \
  SoftFloat/lib/libSoftFloat32d.a         \
  SoftFloat/lib/libSoftFloat64.a          \
  SoftFloat/lib/libSoftFloat64d.a         \
  SoftFloat/softfloat.LICENSE.txt         \
  SoftFloat/softfloat.README.html         \
  SoftFloat/softfloat.README.txt          \
  telnet/include/telnet.h                 \
  telnet/include/telnet_version.h         \
  telnet/lib/libtelnet32.a                \
  telnet/lib/libtelnet32d.a               \
  telnet/lib/libtelnet64.a                \
  telnet/lib/libtelnet64d.a               \
  telnet/lib/telnet32.lib                 \
  telnet/lib/telnet32.pdb                 \
  telnet/lib/telnet32d.lib                \
  telnet/lib/telnet32d.pdb                \
  telnet/lib/telnet64.lib                 \
  telnet/lib/telnet64.pdb                 \
  telnet/lib/telnet64d.lib                \
  telnet/lib/telnet64d.pdb                \
  telnet/telnet.LICENSE.txt               \
  telnet/telnet.README.txt                \
  $(extpkg_headers)

#------------------------------------------------------------------------------
# Source Header files. No 'build' for those
#------------------------------------------------------------------------------

noinst_HEADERS =          \
  archlvl.h               \
  cache.h                 \
  ccfixme.h               \
  cckd.h                  \
  cckd64.h                \
  cckddasd.h              \
  ccnowarn.h              \
  ccwarn.h                \
  chsc.h                  \
  clock.h                 \
  cmdtab.h                \
  cmpsc.h                 \
  cmpscbit.h              \
  cmpscdbg.h              \
  cmpscdct.h              \
  cmpscget.h              \
  cmpscmem.h              \
  cmpscput.h              \
  cnsllogo.h              \
  codepage.h              \
  comm3705.h              \
  commadpt.h              \
  tcpnje.h                \
  cpuint.h                \
  ctc_ptp.h               \
  ctcadpt.h               \
  dasdblks.h              \
  dasdtab.h               \
  dat.h                   \
  dbgtrace.h              \
  devtype.h               \
  ecpsvm.h                \
  esa390.h                \
  esa390io.h              \
  extstring.h             \
  facility.h              \
  feat370.h               \
  feat390.h               \
  feat900.h               \
  featall.h               \
  featchk.h               \
  feature.h               \
  fillfnam.h              \
  fthreads.h              \
  ftlib.h                 \
  getopt.h                \
  hatomic.h               \
  hbyteswp.h              \
  hchan.h                 \
  hconsole.h              \
  hconsts.h               \
  hcrypto.h               \
  hdiagf18.h              \
  hdl.h                   \
  herc_getopt.h           \
  hercifc.h               \
  hercules.h              \
  hercwind.h              \
  herror.h                \
  hetlib.h                \
  hexdumpe.h              \
  hexterns.h              \
  hifr.h                  \
  hinlines.h              \
  history.h               \
  hmacros.h               \
  hmalloc.h               \
  hostinfo.h              \
  hostopts.h              \
  hqadefs.h               \
  hqainc.h                \
  hqawarn.h               \
  hRexx.h                 \
  hRexxapi.c              \
  hscutl.h                \
  hsocket.h               \
  hstdinc.h               \
  hstructs.h              \
  hthreads.h              \
  httpmisc.h              \
  htypes.h                \
  impexp.h                \
  inline.h                \
  instfmts.h              \
  linklist.h              \
  logger.h                \
  machdep.h               \
  memrchr.h               \
  mpc.h                   \
  msgenu.h                \
  netsupp.h               \
  opcode.h                \
  parser.h                \
  printfmt.h              \
  pttrace.h               \
  qdio.h                  \
  qeth.h                  \
  resolve.h               \
  scsitape.h              \
  scsiutil.h              \
  service.h               \
  shared.h                \
  sie.h                   \
  skey.h                  \
  sllib.h                 \
  sockdev.h               \
  sr.h                    \
  stfl.h                  \
  tapedev.h               \
  targetver.h             \
  tcpip.h                 \
  transact.h              \
  tt32api.h               \
  tt32if.h                \
  tuntap.h                \
  version.h               \
  vmd250.h                \
  vstore.h                \
  vsvers.h                \
  w32ctca.c               \
  w32ctca.h               \
  w32dl.h                 \
  w32mtio.h               \
  w32stape.c              \
  w32stape.h              \
  w32util.c               \
  w32util.h               \
  x75.h                   \
  zfcp.h

###############################################################################
#                         B U I L D   R U L E S
###############################################################################
#
#
#                      ***  PROGRAMMING NOTE!  ***
#
#
#       The assignment statements BEFORE this point MAY use blanks if
#       desired, but the TABS in the BELOW build rules ARE REQUIRED!
#
#
#       Not all make programs accept (handle correctly) make files that
#       use blanks instead of tabs in their build rules. Thus in order to
#       remain compatible with older make programs, the below build rules
#       MUST use TABS and NOT BLANKS!!
#
#
###############################################################################


cckd:                   \
	cckdcdsk$(EXEEXT)   \
	cckdcomp$(EXEEXT)   \
	cckddiag$(EXEEXT)   \
	cckdswap$(EXEEXT)   \
	cckdcdsk64$(EXEEXT) \
	cckdcomp64$(EXEEXT) \
	cckddiag64$(EXEEXT) \
	cckdswap64$(EXEEXT) \
	cckdmap$(EXEEXT)    \
	convto64$(EXEEXT)   \
	cckd2ckd$(EXEEXT)   \
	dasdcopy$(EXEEXT)   \
	dasdcopy64$(EXEEXT)

tar: dist

install-exec-local:

install-exec-hook:
	rm -f $(DESTDIR)$(libdir)/libherc*.a
	rm -f $(DESTDIR)$(modexecdir)/dyn*.a
	rm -f $(DESTDIR)$(modexecdir)/hdt*.a
#
# NOTE : symbolic links point to FINAL destination (not to staged install)
#
	rm -f $(DESTDIR)$(bindir)/fba2cfba$(EXEEXT)
	(cd $(DESTDIR)$(bindir); @LN_S@ ./dasdcopy$(EXEEXT) fba2cfba$(EXEEXT))
	rm -f $(DESTDIR)$(bindir)/ckd2cckd$(EXEEXT)
	(cd $(DESTDIR)$(bindir); @LN_S@ ./dasdcopy$(EXEEXT) ckd2cckd$(EXEEXT))
	rm -f $(DESTDIR)$(bindir)/cfba2fba$(EXEEXT)
	(cd $(DESTDIR)$(bindir); @LN_S@ ./dasdcopy$(EXEEXT) cfba2fba$(EXEEXT))
	rm -f $(DESTDIR)$(bindir)/cckd2ckd$(EXEEXT)
	(cd $(DESTDIR)$(bindir); @LN_S@ ./dasdcopy$(EXEEXT) cckd2ckd$(EXEEXT))
	rm -f $(DESTDIR)$(bindir)/fba2cfba64$(EXEEXT)
	(cd $(DESTDIR)$(bindir); @LN_S@ ./dasdcopy64$(EXEEXT) fba2cfba64$(EXEEXT))
	rm -f $(DESTDIR)$(bindir)/ckd2cckd64$(EXEEXT)
	(cd $(DESTDIR)$(bindir); @LN_S@ ./dasdcopy64$(EXEEXT) ckd2cckd64$(EXEEXT))
	rm -f $(DESTDIR)$(bindir)/cfba642fba$(EXEEXT)
	(cd $(DESTDIR)$(bindir); @LN_S@ ./dasdcopy64$(EXEEXT) cfba642fba$(EXEEXT))
	rm -f $(DESTDIR)$(bindir)/cckd642ckd$(EXEEXT)
	(cd $(DESTDIR)$(bindir); @LN_S@ ./dasdcopy64$(EXEEXT) cckd642ckd$(EXEEXT))
if SETUID_HERCIFC
	chown root $(DESTDIR)$(bindir)/hercifc
if HERCIFC_GROUPSET
	chgrp $(HERCIFC_GROUPNAME) $(DESTDIR)$(bindir)/hercifc
endif
	chmod 0750 $(DESTDIR)$(bindir)/hercifc
	chmod +s   $(DESTDIR)$(bindir)/hercifc
	rm                             hercifc
endif
if OPTION_CAPABILITIES
	(cd $(DESTDIR)$(bindir); setcap 'cap_sys_nice=eip' ./hercules$(EXEEXT))
	(cd $(DESTDIR)$(bindir); setcap 'cap_sys_nice=eip' ./herclin$(EXEEXT))
	(cd $(DESTDIR)$(bindir); setcap 'cap_net_admin+ep' ./hercifc$(EXEEXT))
endif

uninstall-local:

uninstall-hook:
	rm -f $(DESTDIR)$(bindir)/fba2cfba$(EXEEXT)
	rm -f $(DESTDIR)$(bindir)/ckd2cckd$(EXEEXT)
	rm -f $(DESTDIR)$(bindir)/cfba2fba$(EXEEXT)
	rm -f $(DESTDIR)$(bindir)/cckd2ckd$(EXEEXT)
	rm -f $(DESTDIR)$(bindir)/fba2cfba64$(EXEEXT)
	rm -f $(DESTDIR)$(bindir)/ckd2cckd64$(EXEEXT)
	rm -f $(DESTDIR)$(bindir)/cfba642fba$(EXEEXT)
	rm -f $(DESTDIR)$(bindir)/cckd642ckd$(EXEEXT)

%.s: %.c
	$(COMPILE) -S $<

%.i: %.c
	$(COMPILE) -E $< >$@

#---------------------------------------------------------------------------
# PROGRAMMING NOTE: the following rules cause only the affected source
# files to be recompiled whenever any of an external package's headers
# are changed.
#---------------------------------------------------------------------------

.libs/dfp.o:  $(decnumber_headers)
.libs/pfpo.o: $(decnumber_headers)
.libs/ieee.o: $(softfloat_headers)

#---------------------------------------------------------------------------

check:
	$(top_srcdir)/tests/runtest  $(top_srcdir)/tests

.PHONY: diagnostic diagnostic-sysinfo diagnostic-tools diagnostic-dirs diagnostic-vars

diagnostic: diagnostic-sysinfo diagnostic-tools diagnostic-dirs diagnostic-vars

diagnostic-sysinfo:
	@echo 'System information'
	@echo '  /etc/os-release     ' `cat /etc/os-release 2>&1`
	@echo '  uname -a            ' `uname -a`
	@echo '  uname -m            ' `uname -m`
	@echo '  uname -p            ' `uname -p`
	@echo '  uname -s            ' `uname -s`
	@echo ' '

diagnostic-tools:
	@echo 'Build tools versions'
	@echo '  autoconf            ' `autoconf --version 2>&1 | head -n 1`
	@echo '  automake            ' `automake --version 2>&1 | head -n 1`
	@echo '  m4                  ' `m4 --version 2>&1 | head -n 1 | sed "s/.*illegal option.*/BSD version of m4"/`
	@echo '  make                ' `make --version 2>&1 | head -n 1 | sed "s/^usage: make.*/BSD version of make/"`
	@echo '  compiler            ' `$(CC) --version 2>&1 | head -n 1`
	@echo '  linker              ' `$(LD) --version 2>&1 | head -n 1`
	@echo ' '

diagnostic-dirs:
	@echo 'Build directories'
	@echo '  source             ' `(cd $(top_srcdir) && pwd || echo $(top_srcdir))`
	@echo '  build              ' `pwd`
	@echo 'Install directories'
	@echo '  prefix              $(prefix)'
	@echo '    exec_prefix       $(exec_prefix)'
	@echo '      bindir          $(bindir)'
	@echo '      libdir          $(libdir)'
	@echo '    includedir        $(includedir)'
	@echo '    datarootdir       $(datarootdir)'
	@echo '      datadir         $(datadir)'
	@echo '      mandir          $(mandir)'
	@echo '      infodir         $(infodir)'
	@echo '      docdir          $(docdir)'
	@echo ' '

diagnostic-vars:
	@echo 'Values from automake'
if BUILD_HERCIFC
	@echo '  BUILD_HERCIFC        Defined'
else
	@echo '  BUILD_HERCIFC        Undefined'
endif
if BUILD_FTHREADS
	@echo '  BUILD_FTHREADS       Defined'
else
	@echo '  BUILD_FTHREADS       Undefined'
endif
# if BUILD_SHARED
# 	@echo '  BUILD_SHARED         Defined'
# else
# 	@echo '  BUILD_SHARED         Undefined'
# endif
# if OPTION_DYNAMIC_LOAD
# 	@echo '  OPTION_DYNAMIC_LOAD  Defined'
# else
# 	@echo '  OPTION_DYNAMIC_LOAD  Undefined'
# endif
	@echo ' '
	@echo 'Values from make'
	@echo '  $$CC                 $(CC)'
	@echo '  $$CFLAGS             $(CFLAGS)'
	@echo '  $$CXX                $(CXX)'
	@echo '  $$CXXFLAG            $(CXXFLAGS)'
	@echo '  $$LDFLAGS            $(LDFLAGS)'
	@echo '  $$CPPFLAGS           $(CPPFLAGS)'
	@echo ' '
	@echo 'Values from configure'
	@echo '  $$LIBS               @LIBS@'
	@echo '  $$S3FH_INC           @S3FH_INC@'
	@echo '  $$S3FH_LIB           @S3FH_LIB@     (future use)'
	@echo '  $$HQA_INC            @HQA_INC@'
	@echo ' '
	@echo 'Values generated by Makefile.am'
	@echo '  $$AM_CPPFLAGS         $(AM_CPPFLAGS)'
	@echo '  $$DYNAMIC_VERISON     $(DYNAMIC_VERSION)'
	@echo '  $$DYNMOD_LD_ADD       $(DYNMOD_LD_ADD)'
	@echo '  $$DYNMOD_LD_FLAGS     $(DYNMOD_LD_FLAGS)'
	@echo '  $$FTHREADS            $(FTHREADS)'
	@echo '  $$HDEPS               $(HDEPS)'
	@echo '  $$HDLFLAGS            $(HDLFLAGS)'
	@echo '  $$HERCIFC             $(HERCIFC)'
	@echo '  $$HERCLIBS            $(HERCLIBS)'
	@echo '  $$HERCLIBS2           $(HERCLIBS2)'
	@echo '  $$HERCLIN             $(HERCLIN)'
	@echo '  $$LDADD               $(LDADD)'
	@echo '  $$LIB_LD_FLAGS        $(LIB_LD_FLAGS)'
	@echo '  $$LTDL                $(LTDL)'
	@echo '  $$tools_ADDLIBS       $(tools_ADDLIBS)'
	@echo '  $$disktools_ADDLIBS   $(disktools_ADDLIBS)'
	@echo '  $$tapetools_ADDLIBS   $(tapetools_ADDLIBS)'
	@echo '  $$tools_LD_FLAGS      $(tools_LD_FLAGS)'
	@echo '  $$XSTATIC             $(XSTATIC)'





