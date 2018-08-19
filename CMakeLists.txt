# Global settings

cmake_minimum_required( VERSION 2.8 )
cmake_policy(SET CMP0048 NEW)

set(CMAKE_BUILD_TYPE "Release")


# Define the project
project( Hyperion VERSION 4.0 LANGUAGES C )


# Add SoftFloat subdirectory
add_subdirectory(SoftFloat)

# Add crypto subdirectory
add_subdirectory(crypto)

# Add decNumber subdirectory
add_subdirectory(decNumber)

# Add telnet subdirectory
add_subdirectory(telnet)

# ------------------
# CMake Modules
# ------------------

INCLUDE (CheckSymbolExists)
INCLUDE (CheckIncludeFiles)
INCLUDE (CheckFunctionExists)
INCLUDE (CheckStructHasMember)
INCLUDE (CheckTypeSize)

# -------------------
# Set version
# -------------------

execute_process(
  COMMAND git log -1 --format=%h
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_COMMIT_HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

if (GIT_COMMIT_HASH)
  set(DYNAMIC_VERSION "${PROJECT_VERSION}~${GIT_COMMIT_HASH}-${CMAKE_BUILD_TYPE}")
else()
  set(DYNAMIC_VERSION "${PROJECT_VERSION}-${CMAKE_BUILD_TYPE}")
endif()



# ------------------
# Check dependencies
# ------------------

include (CMake_HeaderChecks.txt)
include (CMake_LibraryChecks.txt)
include (CMake_FunctionChecks.txt)
include (CMake_MiscChecks.txt)
include (CMake_OSDep.txt)


if (UNIX)
  set(BUILD_HERCIFC 1)
endif (UNIX)

if (NOT HAVE_SETUID)
    set(NO_SETUID 1)
endif()

if (CMAKE_BUILD_TYPE EQUAL "Debug")
  set(DEBUG_BUILD 1)
endif()




if( NOT EXISTS ${CMAKE_SOURCE_DIR}/config.h.template )
    message( FATAL_ERROR "Unable to find config.h.template!" )
else()
    configure_file( ${CMAKE_SOURCE_DIR}/config.h.template
                ${CMAKE_BINARY_DIR}/config.h )
    set (CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -DHAVE_CONFIG_H")
    include_directories( BEFORE
      ${CMAKE_BINARY_DIR}
    )
endif()


# -------------------
#  Hercules libraries
# -------------------

# SYS subsystem
add_library ( libhercs STATIC
  hsys.c
)

# UTIL subsystem
add_library (libhercu STATIC
  codepage.c
  hdl.c
  hexdumpe.c
  hostinfo.c
  hscutl.c
  hscutl2.c
  hsocket.c
  hthreads.c
  logger.c
  logmsg.c
  memrchr.c
  parser.c
  pttrace.c
  version.c
)
target_link_libraries(libhercu decNumber)
target_link_libraries(libhercu SoftFloat)
target_link_libraries(libhercu crypto)
target_link_libraries(libhercu telnet)



# DASD subsystem
add_library( libhercd STATIC
  cache.c
  cckddasd.c
  cckdutil.c
  ckddasd.c
  dasdtab.c
  dasdutil.c
  fbadasd.c
  shared.c
)
target_link_libraries(libhercd libhercs)
target_link_libraries(libhercd libhercu)

# DYNDEV subsystem

add_library( dyndev STATIC
  awstape.c
  cardpch.c
  cardrdr.c
  comm3705.c
  commadpt.c
  con1052c.c
  console.c
  ctc_ctci.c
  ctc_lcs.c
  ctc_ptp.c
  ctcadpt.c
  faketape.c
  ftlib.c
  hchan.c
  hetlib.c
  hettape.c
  mpc.c
  netsupp.c
  omatape.c
  printer.c
  qeth.c
  resolve.c
  scsitape.c
  sllib.c
  sockdev.c
  tapeccws.c
  tapedev.c
  tuntap.c
  zfcp.c
)

# CORE subsystem

add_library(core STATIC
  _archdep_templ.c
  archlvl.c
  assist.c
  bldcfg.c
  cgibin.c
  channel.c
  chsc.c
  clock.c
  cmdtab.c
  cmpsc_2012.c
  cmpscdbg.c
  cmpscdct.c
  cmpscget.c
  cmpscmem.c
  cmpscput.c
  config.c
  control.c
  cpu.c
  crypto.c
  dat.c
  decimal.c
  dfp.c
  diagmssf.c
  diagnose.c
  dyn76.c
  ecpsvm.c
  esame.c
  external.c
  facility.c
  fillfnam.c
  float.c
  general1.c
  general2.c
  general3.c
  hao.c
  hconsole.c
  hdiagf18.c
  history.c
  hRexx.c
  hRexx_o.c
  hRexx_r.c
  hsccmd.c
  hscemode.c
  hscloc.c
  hscmisc.c
  hscpufun.c
  httpserv.c
  ieee.c
  impl.c
  io.c
  ipl.c
  loadmem.c
  loadparm.c
  losc.c
  machchk.c
  opcode.c
  panel.c
  pfpo.c
  plo.c
  qdio.c
  scedasd.c
  scescsi.c
  script.c
  service.c
  sie.c
  sr.c
  stack.c
  strsignal.c
  timer.c
  trace.c
  transact.c
  vector.c
  vm.c
  vmd250.c
  xstore.c
)

if (BZIP2_FOUND)
  target_link_libraries(libhercd ${BZIP2_LIBRARIES})
endif()

if (ZLIB_FOUND)
  target_link_libraries(libhercd ${ZLIB_LIBRARIES})
endif()

# --------------
# Misc utilities
# --------------

# cckdcdsk
add_executable(cckdcdsk cckdcdsk.c)
target_link_libraries(cckdcdsk libhercd)

# cckdcomp
add_executable(cckdcomp cckdcomp.c)
target_link_libraries(cckdcomp libhercd)

# cckddiag
add_executable(cckddiag cckddiag.c)
target_link_libraries(cckddiag libhercd)

# cckdswap
add_executable(cckdswap cckdswap.c)
target_link_libraries(cckdswap libhercd)

# dasdcat
add_executable(dasdcat dasdcat.c)
target_link_libraries(dasdcat libhercd)

# dasdconv
add_executable(dasdconv dasdconv.c)
target_link_libraries(dasdconv libhercd)

# dasdcopy
add_executable(dasdcopy dasdcopy.c)
target_link_libraries(dasdcopy libhercd)

# dasdinit
add_executable(dasdinit dasdinit.c)
target_link_libraries(dasdinit libhercd)

# dasdisup
add_executable(dasdisup dasdisup.c)
target_link_libraries(dasdisup libhercd)

# dasdload
add_executable(dasdload dasdload.c)
target_link_libraries(dasdload libhercd)

# dasdls
add_executable(dasdls dasdls.c)
target_link_libraries(dasdls libhercd)

# dasdpdsu
add_executable(dasdpdsu dasdpdsu.c)
target_link_libraries(dasdpdsu libhercd)

# dasdseq
add_executable(dasdseq dasdseq.c)
target_link_libraries(dasdseq libhercd)

# dmap2hrc
add_executable(dmap2hrc dmap2hrc.c)
target_link_libraries(dmap2hrc libhercd)

# hetget
add_executable(hetget hetget.c)
target_link_libraries(hetget libhercd)
target_link_libraries(hetget dyndev)

# hetinit
add_executable(hetinit hetinit.c)
target_link_libraries(hetinit libhercd)
target_link_libraries(hetinit dyndev)

# hetmap
add_executable(hetmap hetmap.c)
target_link_libraries(hetmap libhercd)
target_link_libraries(hetmap dyndev)

# hetupd
add_executable(hetupd hetupd.c)
target_link_libraries(hetupd libhercd)
target_link_libraries(hetupd dyndev)

# tapecopy
add_executable(tapecopy tapecopy.c)
target_link_libraries(tapecopy libhercd)

# tapemap
add_executable(tapemap tapemap.c)
target_link_libraries(tapemap libhercd)

# tapesplt
add_executable(tapesplt tapesplt.c)
target_link_libraries(tapesplt libhercd)

# vmfplc2
add_executable(vmfplc2 vmfplc2.c)
target_link_libraries(vmfplc2 libhercd)
target_link_libraries(vmfplc2 dyndev)

# hercules
set (CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -D_GNU_SOURCE")
add_executable(hercules bootstrap.c)
target_link_libraries(hercules core)
target_link_libraries(hercules dyndev)
target_link_libraries(hercules libhercd)

message( STATUS "Hercules Hyperion version ${DYNAMIC_VERSION} ready to build." )
