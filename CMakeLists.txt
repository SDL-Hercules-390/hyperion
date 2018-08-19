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


if (BZIP2_FOUND)
  target_link_libraries(libhercd ${BZIP2_LIBRARIES})
endif()

# --------------
# Misc utilities
# --------------

# dasdcat
add_executable(dasdcat dasdcat.c)
target_link_libraries(dasdcat libhercd)

message( STATUS "Hercules Hyperion version ${DYNAMIC_VERSION} ready to build." )
