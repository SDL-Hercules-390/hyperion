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


# ------------------------
#  Hercules shared library
# ------------------------

# Add all modules to one big shared library to avoid the interdependency and reduce moving parts
add_library ( hercules_engine SHARED
# hsys

  hsys.c

# utils

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

# dasd
  cache.c
  cckddasd.c
  cckdutil.c
  ckddasd.c
  dasdtab.c
  dasdutil.c
  fbadasd.c
  shared.c

# core

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

# hetlib

  hetlib.c

# ftlib

  ftlib.c

# sllib

  sllib.c

# win32 support
  w32util.c
  fthreads.c
  w32ctca.c

)

target_link_libraries(hercules_engine decNumber)
target_link_libraries(hercules_engine SoftFloat)
target_link_libraries(hercules_engine crypto)
target_link_libraries(hercules_engine telnet)
target_link_libraries(hercules_engine ${CMAKE_DL_LIBS})
target_link_libraries(hercules_engine ${CMAKE_THREAD_LIBS_INIT})

install(TARGETS hercules_engine DESTINATION lib)

if(IS_WINDOWS)
  target_link_libraries(hercules_engine ws2_32)
  target_link_libraries(hercules_engine bcrypt)
  target_link_libraries(hercules_engine winmm)
endif()


if(LIBM_REQUIRED)
  target_link_libraries(hercules_engine m)
endif()

if (BZIP2_FOUND)
  target_link_libraries(hercules_engine ${BZIP2_LIBRARIES})
endif()

if (ZLIB_FOUND)
  message(STATUS "ZLIB LIBS: ${ZLIB_LIBRARIES}")
  target_link_libraries(hercules_engine ${ZLIB_LIBRARIES})
endif()

# --------------
# Device drivers
# --------------

add_library( hdteq    SHARED hdteq.c)
add_library( hdt1403  SHARED printer.c sockdev.c)
add_library( hdt2880  SHARED hchan.c)
add_library( hdt2703  SHARED commadpt.c)
add_library( hdt3705  SHARED comm3705.c)
add_library( hdt3088  SHARED ctc_lcs.c ctc_ctci.c ctcadpt.c tuntap.c netsupp.c)
add_library( hdt3270  SHARED console.c)
add_library( hdt3420  SHARED tapedev.c tapeccws.c awstape.c faketape.c hettape.c omatape.c scsitape.c scsiutil.c)
add_library( hdt3505  SHARED cardrdr.c sockdev.c)
add_library( hdt3525  SHARED cardpch.c)
add_library( hdtqeth  SHARED qeth.c mpc.c resolve.c tuntap.c)
add_library( hdtzfcp  SHARED zfcp.c)
add_library( hdt1052c SHARED con1052c.c)
add_library( hdtptp   SHARED ctc_ptp.c mpc.c resolve.c tuntap.c)

target_link_libraries( hdteq    hercules_engine )
target_link_libraries( hdt1403  hercules_engine )
target_link_libraries( hdt2880  hercules_engine )
target_link_libraries( hdt2703  hercules_engine )
target_link_libraries( hdt3705  hercules_engine )
target_link_libraries( hdt3088  hercules_engine )
target_link_libraries( hdt3270  hercules_engine )
target_link_libraries( hdt3420  hercules_engine )
target_link_libraries( hdt3505  hercules_engine )
target_link_libraries( hdt3525  hercules_engine )
target_link_libraries( hdtqeth  hercules_engine )
target_link_libraries( hdtzfcp  hercules_engine )
target_link_libraries( hdt1052c hercules_engine )
target_link_libraries( hdtptp   hercules_engine )

install(TARGETS hdteq    DESTINATION lib)
install(TARGETS hdt1403  DESTINATION lib)
install(TARGETS hdt2880  DESTINATION lib)
install(TARGETS hdt2703  DESTINATION lib)
install(TARGETS hdt3705  DESTINATION lib)
install(TARGETS hdt3088  DESTINATION lib)
install(TARGETS hdt3270  DESTINATION lib)
install(TARGETS hdt3420  DESTINATION lib)
install(TARGETS hdt3505  DESTINATION lib)
install(TARGETS hdt3525  DESTINATION lib)
install(TARGETS hdtqeth  DESTINATION lib)
install(TARGETS hdtzfcp  DESTINATION lib)
install(TARGETS hdt1052c DESTINATION lib)
install(TARGETS hdtptp   DESTINATION lib)


# --------------
# Misc utilities
# --------------

# cckdcdsk
add_executable(cckdcdsk cckdcdsk.c)
target_link_libraries(cckdcdsk hercules_engine)
install(TARGETS cckdcdsk DESTINATION bin)

# cckdcomp
add_executable(cckdcomp cckdcomp.c)
target_link_libraries(cckdcomp hercules_engine)
install(TARGETS cckdcomp DESTINATION bin)

# cckddiag
add_executable(cckddiag cckddiag.c)
target_link_libraries(cckddiag hercules_engine)
install(TARGETS cckddiag DESTINATION bin)

# cckdswap
add_executable(cckdswap cckdswap.c)
target_link_libraries(cckdswap hercules_engine)
install(TARGETS cckdswap DESTINATION bin)

# dasdcat
add_executable(dasdcat dasdcat.c)
target_link_libraries(dasdcat hercules_engine)
install(TARGETS dasdcat DESTINATION bin)

# dasdconv
add_executable(dasdconv dasdconv.c)
target_link_libraries(dasdconv hercules_engine)
install(TARGETS dasdconv DESTINATION bin)

# dasdcopy
add_executable(dasdcopy dasdcopy.c)
target_link_libraries(dasdcopy hercules_engine)
install(TARGETS dasdcopy DESTINATION bin)

# dasdinit
add_executable(dasdinit dasdinit.c)
target_link_libraries(dasdinit hercules_engine)
install(TARGETS dasdinit DESTINATION bin)

# dasdisup
add_executable(dasdisup dasdisup.c)
target_link_libraries(dasdisup hercules_engine)
install(TARGETS dasdisup DESTINATION bin)

# dasdload
add_executable(dasdload dasdload.c)
target_link_libraries(dasdload hercules_engine)
install(TARGETS dasdload DESTINATION bin)

# dasdls
add_executable(dasdls dasdls.c)
target_link_libraries(dasdls hercules_engine)
install(TARGETS dasdls DESTINATION bin)

# dasdpdsu
add_executable(dasdpdsu dasdpdsu.c)
target_link_libraries(dasdpdsu hercules_engine)
install(TARGETS dasdpdsu DESTINATION bin)

# dasdseq
add_executable(dasdseq dasdseq.c)
target_link_libraries(dasdseq hercules_engine)
install(TARGETS dasdseq DESTINATION bin)

# dmap2hrc
add_executable(dmap2hrc dmap2hrc.c)
target_link_libraries(dmap2hrc hercules_engine)
install(TARGETS dmap2hrc DESTINATION bin)

# hetget
add_executable(hetget hetget.c)
target_link_libraries(hetget hercules_engine)
install(TARGETS hetget DESTINATION bin)

# hetinit
add_executable(hetinit hetinit.c)
target_link_libraries(hetinit hercules_engine)
install(TARGETS hetinit DESTINATION bin)

# hetmap
add_executable(hetmap hetmap.c)
target_link_libraries(hetmap hercules_engine)
install(TARGETS hetmap DESTINATION bin)

# hetupd
add_executable(hetupd hetupd.c)
target_link_libraries(hetupd hercules_engine)
install(TARGETS hetupd DESTINATION bin)

# tapecopy
add_executable(tapecopy tapecopy.c scsiutil.c)
target_link_libraries(tapecopy hercules_engine)
install(TARGETS tapecopy DESTINATION bin)

# tapemap
add_executable(tapemap tapemap.c)
target_link_libraries(tapemap hercules_engine)
install(TARGETS tapemap DESTINATION bin)

# tapesplt
add_executable(tapesplt tapesplt.c)
target_link_libraries(tapesplt hercules_engine)
install(TARGETS tapesplt DESTINATION bin)

# vmfplc2
add_executable(vmfplc2 vmfplc2.c)
target_link_libraries(vmfplc2 hercules_engine)
install(TARGETS vmfplc2 DESTINATION bin)

# --------------------
# Main hercules binary
# --------------------

# hercules
set (CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -D_GNU_SOURCE")
add_executable(hercules bootstrap.c)
target_link_libraries(hercules hercules_engine)
install(TARGETS hercules DESTINATION bin)


message( STATUS "Hercules Hyperion version ${DYNAMIC_VERSION} ready to build." )
