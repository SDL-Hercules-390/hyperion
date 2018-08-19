#------------------------------------------------------------------------------
#                         ParseBinaryDir.cmake
#------------------------------------------------------------------------------

macro( ParseBinaryDir )

    set(  CMAKE_DISABLE_IN_SOURCE_BUILD  ON  )
    set(  CMAKE_DISABLE_SOURCE_CHANGES   ON  )

    #--------------------------------------------------------------------------
    #  Make sure they are not trying to do an "in source" build by making
    #  sure the cmake "binary" (build) directory is not a subdirectory of
    #  the product's main cmake "source" directory.  We use the REALPATH
    #  option on the 'get_filename_component' cmake command to ensure the
    #  user cannot possibly play dirty with symlinks.
    #--------------------------------------------------------------------------
    #  TECHNIQUE: if the first part of the full path binary directory minus
    #  its last directory component matches the full path source directory,
    #  then it is by definition a subdirectory of the source directory.
    #--------------------------------------------------------------------------

    get_filename_component( SOURCE_DIR      "${CMAKE_SOURCE_DIR}" REALPATH )
    get_filename_component( BINARY_DIR      "${CMAKE_BINARY_DIR}" REALPATH )
    get_filename_component( BINARY_DIR_NAME "${CMAKE_BINARY_DIR}" NAME     )

    trace( SOURCE_DIR )
    trace( BINARY_DIR )
    trace( BINARY_DIR_NAME )

    string( LENGTH ${BINARY_DIR}       _len_bindir      )
    string( LENGTH ${BINARY_DIR_NAME}  _len_bindir_name )
    math( EXPR _trunc_bindir_len "${_len_bindir} - ${_len_bindir_name} - 1" )
    string( SUBSTRING ${BINARY_DIR} 0 ${_trunc_bindir_len} TRUNCATED_BINARY_DIR )

    trace( TRUNCATED_BINARY_DIR )

    if( "${TRUNCATED_BINARY_DIR}" STREQUAL "${SOURCE_DIR}" )
        message( FATAL_ERROR "In-source builds are not allowed!
Remove the 'CMakeCache.txt' file and the entire 'CMakeFiles' directory and try again!" )
    endif()

    #--------------------------------------------------------------------------
    #   Enable C/C++ language
    #--------------------------------------------------------------------------
    
    enable_language( C CXX )

    #--------------------------------------------------------------------------
    #   Check if this is a BIG ENDIAN or LITTLE ENDIAN build system.
    #   Some packages needs to know this.
    #--------------------------------------------------------------------------

    include( TestBigEndian )

    TEST_BIG_ENDIAN( IS_BIG_ENDIAN )

    #--------------------------------------------------------------------------
    #   Split the binary build directory into its constituent components.
    #   Refer to the "BUILDING" document for more information.
    #--------------------------------------------------------------------------

    get_filename_component( BINARY_HLQ "${CMAKE_BINARY_DIR}" DIRECTORY )
    get_filename_component( BINARY_DIR "${CMAKE_BINARY_DIR}" NAME )

    trace( BINARY_HLQ )
    trace( BINARY_DIR )

    string( FIND ${BINARY_DIR} " " _n )
    if( NOT ${_n} EQUAL -1 )
        message( FATAL_ERROR "Build directory name cannot have spaces! ${BINARY_DIR}" )
    endif()

    #--------------------------------------------------------------------------
    #  First, split it into two parts: before the dot and after the dot
    #--------------------------------------------------------------------------

    string( REGEX MATCH "([^\\.]*)" _xxxxx ${BINARY_DIR} )
    string( REGEX MATCH "([^.]*\$)" CONFIG ${BINARY_DIR} )

    trace( _xxxxx )
    trace( CONFIG )

    #--------------------------------------------------------------------------
    #  The second part tells us if this is a "Debug" or "Release" build.
    #--------------------------------------------------------------------------

    string( LENGTH ${CONFIG} _n )
    if( ${_n} LESS 1 )
        message( FATAL_ERROR "Invalid Release/Debug build type! ${CONFIG}" )
    endif()

    #--------------------------------------------------------------------------
    #  Capitalize "Debug" and "Release" for Visual Studio compatibility.
    #--------------------------------------------------------------------------

    include( CapitalizeWord )

    Capitalize_Word( ${CONFIG} CONFIG )

    if(( NOT CONFIG STREQUAL "Debug" ) AND (NOT CONFIG STREQUAL "Release" ))
        message( FATAL_ERROR "Invalid Release/Debug build type! ${CONFIG}" )
    endif()

    #--------------------------------------------------------------------------
    #  Define the "Debug" or "Release" build type
    #--------------------------------------------------------------------------

    if( CONFIG STREQUAL "Debug" )
        set( CMAKE_BUILD_TYPE "Debug" CACHE PATH "" FORCE )
        set( DBGCHAR "d" )
    elseif( CONFIG STREQUAL "Release" )
        set( CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE PATH "" FORCE )
        set( DBGCHAR "" )
    endif()

    trace( CMAKE_BUILD_TYPE )

    #--------------------------------------------------------------------------
    #  Now split the first part into the base package/product name
    #  and build architecture (32-bit ot 64-bit).
    #--------------------------------------------------------------------------

    string( LENGTH ${_xxxxx} _n )
    if( ${_n} LESS 3 )
        message( FATAL_ERROR "Invalid base package name! ${_xxxxx}" )
    endif()

    math( EXPR _n "${_n} - 2" )     # (want the last two characters)

    string( SUBSTRING ${_xxxxx} 0 ${_n}    BASENAME )
    string( SUBSTRING ${_xxxxx}   ${_n} -1 BITNESS  )

    if( NOT BITNESS STREQUAL "32" AND
        NOT BITNESS STREQUAL "64" )
        message( FATAL_ERROR "Invalid package architecture! ${BITNESS}" )
    endif()

    #--------------------------------------------------------------------------
    #  Show results
    #--------------------------------------------------------------------------

    set( SUFFIX   "${BITNESS}${DBGCHAR}" )
    set( FULLNAME "${BASENAME}${SUFFIX}" )

    trace( BASENAME )
    trace( BITNESS  )
    trace( CONFIG   )
    trace( DBGCHAR  )
    trace( SUFFIX   )
    trace( FULLNAME )
    trace( CMAKE_BINARY_DIR )

    #--------------------------------------------------------------------------
    #  Define the install directory
    #--------------------------------------------------------------------------

    if( DEFINED INSTALL_PREFIX )

        #  Use the value they specified

        set( CMAKE_INSTALL_PREFIX "${INSTALL_PREFIX}" CACHE PATH "" FORCE )

    else()  # (NOT DEFINED INSTALL_PREFIX)

        if( WIN32 )

            #  Windows: use the full name as the default install directory

            set( CMAKE_INSTALL_PREFIX "${BINARY_HLQ}/${FULLNAME}" CACHE PATH "" FORCE )

        else()

          #  Linux: let it default to the cmake default of '/usr/local'

        endif()

    endif()

    trace( CMAKE_INSTALL_PREFIX )

endmacro()
