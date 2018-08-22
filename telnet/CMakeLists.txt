#------------------------------------------------------------------------------
#   Global settings
#------------------------------------------------------------------------------

cmake_minimum_required( VERSION 2.8 )

cmake_policy( SET CMP0048 NEW )
cmake_policy( SET CMP0054 OLD )

#------------------------------------------------------------------------------
#   Load some handy CMake modules
#------------------------------------------------------------------------------

if( NOT EXISTS "${CMAKE_SOURCE_DIR}/cmake/modules" )
    message( FATAL_ERROR "CMake modules directory not found! ${CMAKE_SOURCE_DIR}/cmake/modules" )
else()

    set( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules" )

    include( Trace )

endif()

#------------------------------------------------------------------------------
#   Add current directory to include path.
#------------------------------------------------------------------------------

set(CMAKE_INCLUDE_CURRENT_DIR ON)


#------------------------------------------------------------------------------
#   Define the project
#------------------------------------------------------------------------------

include( project.txt )


#------------------------------------------------------------------------------
#   Define package VERSION strings
#------------------------------------------------------------------------------

include( Version )


#------------------------------------------------------------------------------
#   Construct a list of this project's public headers
#------------------------------------------------------------------------------

include( headers.txt )


#------------------------------------------------------------------------------
#   Construct a list of this project's source files
#------------------------------------------------------------------------------

include( sources.txt )


#------------------------------------------------------------------------------
#   Required headers
#------------------------------------------------------------------------------

include( CheckIncludeFile )
include( CheckHeader )

check_header( stdbool.h )       # defines HAVE_STDBOOL_H
check_header( stdint.h )        # defines HAVE_STDINT_H


#------------------------------------------------------------------------------
#   Always generate the platform.h header
#------------------------------------------------------------------------------

if( NOT EXISTS ${CMAKE_SOURCE_DIR}/telnet/platform.h.in )
    message( FATAL_ERROR "Unable to find platform.h.in!" )
else()
    configure_file( ${CMAKE_SOURCE_DIR}/telnet/platform.h.in
                    ${CMAKE_BINARY_DIR}/telnet/platform.h )
endif()


#------------------------------------------------------------------------------
#   Update the INCLUDE directories search order
#------------------------------------------------------------------------------

include( includes.txt )


#------------------------------------------------------------------------------
#   Adjust needed compile flags for this project/package
#------------------------------------------------------------------------------

include( cflags.txt )


#------------------------------------------------------------------------------
#   Set the build architecture      (32 bit or 64- bit)
#------------------------------------------------------------------------------

if( WIN32 )
  # (the toolchain being used (i.e. vstools.cmd 32/64) defines this on Windows)
else()
  if( BITNESS STREQUAL "32" )
    set( CMAKE_C_FLAGS_DEBUG          "${CMAKE_C_FLAGS_DEBUG}          -m32 -fPIC" )
    set( CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -m32 -fPIC" )
  else()
    set( CMAKE_C_FLAGS_DEBUG          "${CMAKE_C_FLAGS_DEBUG}          -m64 -fPIC" )
    set( CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -m64 -fPIC" )
  endif()
endif()

trace( CMAKE_C_FLAGS_DEBUG )
trace( CMAKE_C_FLAGS_RELWITHDEBINFO )


#------------------------------------------------------------------------------
#   Define the package's build and install targets
#------------------------------------------------------------------------------

add_library( ${FULLNAME}  STATIC  ${${PROJECT_NAME}_SRCS} )

set_target_properties( ${FULLNAME} PROPERTIES
                       POSITION_INDEPENDENT_CODE  TRUE
                       PUBLIC_HEADER              "${PUBLIC_HEADERS}"
                       OUTPUT_NAME                 ${FULLNAME}
                       COMPILE_PDB_NAME            ${FULLNAME} )

install( TARGETS ${FULLNAME}
         PUBLIC_HEADER DESTINATION include
         ARCHIVE       DESTINATION lib )


#------------------------------------------------------------------------------
#   Define additional files to be installed
#------------------------------------------------------------------------------

include( extra.txt )
