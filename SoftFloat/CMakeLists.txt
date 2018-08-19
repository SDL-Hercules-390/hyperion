#   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#   Global settings
#
cmake_minimum_required( VERSION 2.8 )

cmake_policy(SET CMP0048 NEW)
cmake_policy(SET CMP0054 OLD)

#   set build type to Release by default
set(BUILD_TYPE Release)

#   include current directory as include PATH

set(CMAKE_INCLUDE_CURRENT_DIR ON)

#   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#   define the project
#
project( SoftFloat VERSION 3.1.0 LANGUAGES C )


#   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#   the directory with the CMAKE user modules
if( EXISTS "${CMAKE_SOURCE_DIR}/cmake/modules" )
    set( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules" )
    include( Vdump )
    include( Trace )
else()
	message( FATAL_ERROR "
Cmake modules directory not found '${CMAKE_SOURCE_DIR}/cmake/modules'
" )
endif()

#   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#  	revision information
#
find_package( Git )
if( GIT_FOUND AND EXISTS ${CMAKE_SOURCE_DIR}/.git )

	include( GIT_Revision )
	message( "@@ Building ${PROJECT_NAME} revision ${GIT_WC_REVISION}" )
endif()


set( HERCMODS "../hercsource/" )




set( TRAVIS $ENV{TRAVIS} )
if( TRAVIS )
    message( "@@ Remote build ( Travis ) " )
else()
    message( "@@ Local  build" )

# Prevent an in-source build.  REALPATH resolves any symlinks.  There is
# no way to programatically remove CMakeCache.txt nor CMakeFiles/*
# from within a CMakeLists.txt, so we must just instruct the builder.
# And if the builder fails to clean up the source directory, the stale
# CMakeCache.txt file in the source directory will confound the build
# process until it is removed.  So we will check for it and complain if
# one is found.

get_filename_component( SOURCE_DIR "${CMAKE_SOURCE_DIR}" REALPATH)
get_filename_component( BINARY_DIR "${CMAKE_BINARY_DIR}" REALPATH)

if( (EXISTS ${SOURCE_DIR}/CMakeCache.txt )
        OR (EXISTS ${SOURCE_DIR}/CMakeFiles) )
    if( "{SOURCE_DIR}" STREQUAL ${CMAKE_SOURCE_DIR} )
        set( message_source_dir "${CMAKE_SOURCE_DIR}" )
    elseif( )
        set( message_source_dir "${CMAKE_SOURCE_DIR} = ${SOURCE_DIR}" )
    endif( )
    string( CONCAT error_message
"-------------------------------------------------------------\n"
"Existing file CMakeCache.txt and directory CMakeFiles/ *MUST*\n"
"be removed from the source directory:\n\n"
"    ${CMAKE_SOURCE_DIR}\n\n"
"SoftFloat-3a For Hercules cannot be built until CMakeCache.txt\n"
"and CMakeFiles/. are removed from ${CMAKE_SOURCE_DIR}.\n"
"-------------------------------------------------------------\n"
    )
    message( ${error_message} )
    message( FATAL_ERROR "SoftFloat-3a For Hercules will not be built" )
endif( )

if( "${SOURCE_DIR}" STREQUAL "${BINARY_DIR}" )
    string( CONCAT error_message
"-------------------------------------------------------------\n"
"In-source builds are not allowed for SoftFloat-3a For Hercules\n\n"
"   ${CMAKE_SOURCE_DIR} = ${SOURCE_DIR}\n"
"   ${CMAKE_BINARY_DIR} = ${BINARY_DIR}\n\n"
"Before continuing, remove the file CMakeCache.txt and the\n"
"directory CMakeFiles from the source directory.\n"
"-------------------------------------------------------------"
    )
    message( ${error_message} )
    message( FATAL_ERROR "SoftFloat-3a For Hercules will not be built" )
endif( )

execute_process( COMMAND ${CMAKE_COMMAND} -E remove ${PROJECT_SOURCE_DIRECTORY}/CMakeCache.txt )
set( CMAKE_DISABLE_IN_SOURCE_BUILD ON )
set( CMAKE_DISABLE_SOURCE_CHANGES  ON )



#   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#  	install prefix, and build type
#   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
if( NOT DEFINED INSTALL_PREFIX )
    get_filename_component( BINARY_HLQ ${CMAKE_BINARY_DIR} DIRECTORY )
    get_filename_component( BINARY_DIR ${CMAKE_BINARY_DIR} NAME )
	string( REGEX MATCH "([^\\.]*)" INSTALL_PREFIX ${BINARY_DIR} )
	if(	BINARY_DIR STREQUAL INSTALL_PREFIX )
		message( FATAL_ERROR "
Unable to determine INSTALL_PREFIX from  ${CMAKE_BINARY_DIR}
" )
	endif()
	set( INSTALL_PREFIX  "${BINARY_HLQ}/${INSTALL_PREFIX}")
endif()
set( CMAKE_INSTALL_PREFIX  "${INSTALL_PREFIX}" CACHE PATH "" FORCE )


#   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#
#   Multi-configuration CMake generators (Visual Studio and OS/X)
#   default to four configurations: Release, Debug, MinSizeRel, and
#   RelWithDebInfo.  For now, only the first two are supported.  Remove
#   the other two from consideration.  The configuration to be built is
#   specified at build time, not configure time.
#
#   If building for a single-configuration generator (Makefiles or
#   Ninja), determine the configuration from a) BUILD_TYPE or, if that
#   variable is not defined, the characters following the '.' in the
#   build directory name.  Fail the build if no valid configuration type
#   can be found.
#
#   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

if( "${CMAKE_CONFIGURATION_TYPES}" STREQUAL "" )
    # Generator in use creates build scripts for exactly one configuration.
    # Determine whether this is a release or debug build.
    if( NOT DEFINED BUILD_TYPE )
        get_filename_component( BINARY_HLQ ${CMAKE_BINARY_DIR} DIRECTORY )
        get_filename_component( BINARY_DIR ${CMAKE_BINARY_DIR} NAME )
        string( REGEX MATCH "([^.]*\$)" BUILD_TYPE ${BINARY_DIR} )
        if(	BINARY_DIR STREQUAL BUILD_TYPE )
            message( FATAL_ERROR "Unable to determine CMAKE_BUILD_TYPE from  ${CMAKE_BINARY_DIR}" )
        endif()
    endif()
    include( CapitalizeWord )
    Capitalize_Word( ${BUILD_TYPE} BUILD_TYPE )
    if( NOT BUILD_TYPE MATCHES "Release" AND
        NOT BUILD_TYPE MATCHES "Debug" )
        message( FATAL_ERROR "Invalid Release/Debug BUILD_TYPE ${BUILD_TYPE}" )
    endif()
    set( CMAKE_BUILD_TYPE "${BUILD_TYPE}" )
    message( "@@ Single configuration generator; configuring for ${CMAKE_BUILD_TYPE}" )

else( )
    # For generators that support multiple configurations with build-time
    # configuration selection (Visual Studio or mac OS Xcode), CMake
    # defaults to four configurations: Release, Debug, MinSizeRel, and
    # RelWithDebInfo.  For now, only the first two are needed.  Remove the
    # other two.
    list( REMOVE_ITEM CMAKE_CONFIGURATION_TYPES
                MinSizeRel
                RelWithDebInfo
                )
    message ( "@@ Available configuration Types: ${CMAKE_CONFIGURATION_TYPES}"  )
endif( )


### set( CMAKE_BUILD_TYPE  "${BUILD_TYPE}" CACHE PATH "" FORCE )
set( CMAKE_DEBUG_POSTFIX "_D" )

endif( TRAVIS )

####  set( SHARED_LIBRARY )


#   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#  	headers.   If any header is missing, softfloat_includes.h will address
#   the shortfall
#
include( CheckIncludeFile )
include( CheckHeader )
check_header( stdbool.h )
check_header( stdint.h )
check_header( inttypes.h )

#   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#  	CFLAGS
#
if( NOT WIN32 )
    set( CMAKE_C_FLAGS_DEBUG    "-g -g3 -ggdb3"
        CACHE STRING "" FORCE )
    set( CMAKE_C_FLAGS_RELEASE  "-g -O2"
        CACHE STRING "" FORCE )
endif()

#   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#  	CDEFS
#
if( WIN32 )
    set( CDEFS_DEBUG            "/D HAVE_PLATFORM_H /D _DEBUG" )
    set( CDEFS_RELEASE          "/D HAVE_PLATFORM_H /D _NDEBUG" )

else()
    set( CDEFS_DEBUG            "-D HAVE_PLATFORM_H -D DEBUG" )
    set( CDEFS_RELEASE          "-D HAVE_PLATFORM_H -D NDEBUG" )

endif()

set( CMAKE_C_FLAGS_DEBUG    "${CMAKE_C_FLAGS_DEBUG}   ${CDEFS_DEBUG}" )
set( CMAKE_C_FLAGS_RELEASE  "${CMAKE_C_FLAGS_RELEASE} ${CDEFS_RELEASE}" )

# 	- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#   generate platform.h
#
if( EXISTS ${CMAKE_SOURCE_DIR}/SoftFloat/platform.h.in )
	configure_file( ${CMAKE_SOURCE_DIR}/SoftFloat/platform.h.in
    	            ${CMAKE_BINARY_DIR}/SoftFloat/platform.h )
else()
	message( FATAL_ERROR "** Unable to find platform.h.in" )
endif()





# 	- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#   include directories
#
include_directories(    BEFORE
                    ${CMAKE_BINARY_DIR}
                    ${CMAKE_SOURCE_DIR}/SoftFloat/source/8086-SSE
                    ${CMAKE_SOURCE_DIR}/SoftFloat/source/include
                    )


# 	- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#   SoftFloat Public Headers.  While the list of public headers is
#   assembled, the headers are copied to an include directory in the
#   build tree.  This allows the build tree export target to be self-
#   contained in the build tree, with no references to the source tree.
#   This is particularly useful if the source tree is on a network drive
#   that is not accessable when the downstream build imports the build
#   tree target exported by this script.

#   configure_file( COPYONLY ) is used to copy the headers because it
#   creates a dependency on the source public header; changing the
#   source will cause the file to be copied again.

if( EXISTS ${CMAKE_BINARY_DIR}/include )       # Remove any leftover cruft.
    execute_process( COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/include )
endif( )

execute_process( COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/include )

set(    _l
        softfloat.h
        softfloat_stdTypes.h
        softfloat_types.h
    )
list( SORT _l )
set( public_headers "" )
foreach( _s IN LISTS _l )
    list(   APPEND
            public_headers
            source/include/${_s} )
    configure_file( ${CMAKE_SOURCE_DIR}/SoftFloat/source/include/${_s}  ${CMAKE_BINARY_DIR}/include  COPYONLY )
endforeach()



# 	- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#   the sources
#
# 	- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#   SoftFloat Primitives
set(    _l
        s_add256M.c
        s_approxRecipSqrt32_1.c
        s_countLeadingZeros64.c
        s_countLeadingZeros8.c
        s_mul128To256M.c
        s_mul64To128.c
        s_shiftRightJam128.c
        s_shiftRightJam128Extra.c
        s_shiftRightJam256M.c
        s_sub256M.c
    )
list( SORT _l )
set(    SoftFloat_PRIMITIVES "" )
foreach( _s IN LISTS _l )
    list(   APPEND
            SoftFloat_PRIMITIVES
            source/${_s} )
endforeach()

# 	- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#   SoftFloat Specialised
set(    _l
        s_commonNaNToF128UI.c
        s_commonNaNToF32UI.c
        s_commonNaNToF64UI.c
        s_f128UIToCommonNaN.c
        s_f32UIToCommonNaN.c
        s_f64UIToCommonNaN.c
        ../${HERCMODS}s_propagateNaNF128UI.c
        ../${HERCMODS}s_propagateNaNF32UI.c
        ../${HERCMODS}s_propagateNaNF64UI.c
        specialize.h
    )
list( SORT _l )
set(    SoftFloat_SPECIALISED "" )
foreach( _s IN LISTS _l )
    list(   APPEND
            SoftFloat_SPECIALISED
            source/8086-SSE/${_s} )
endforeach()

# 	- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#   SoftFloat Others
set(    _l
        f128_add.c
        f128_div.c
        f128_eq.c
        f128_isSignalingNaN.c
        f128_lt_quiet.c
        f128_mul.c
        f128_mulAdd.c
        ${HERCMODS}f128_roundToInt.c
        f128_sqrt.c
        f128_sub.c
        f128_to_f32.c
        f128_to_f64.c
        f128_to_i32.c
        f128_to_i64.c
        f128_to_ui32.c
        ${HERCMODS}f128_to_ui64.c
        f32_add.c
        f32_div.c
        f32_eq.c
        f32_isSignalingNaN.c
        f32_lt_quiet.c
        f32_mul.c
        f32_mulAdd.c
        ${HERCMODS}f32_roundToInt.c
        f32_sqrt.c
        f32_sub.c
        f32_to_f128.c
        f32_to_f64.c
        f32_to_i32.c
        f32_to_i64.c
        f32_to_ui32.c
        ${HERCMODS}f32_to_ui64.c
        f64_add.c
        f64_div.c
        f64_eq.c
        f64_isSignalingNaN.c
        f64_lt_quiet.c
        f64_mul.c
        f64_mulAdd.c
        ${HERCMODS}f64_roundToInt.c
        f64_sqrt.c
        f64_sub.c
        f64_to_f128.c
        f64_to_f32.c
        f64_to_i32.c
        f64_to_i64.c
        f64_to_ui32.c
        ${HERCMODS}f64_to_ui64.c
        i32_to_f128.c
        i32_to_f32.c
        i32_to_f64.c
        i64_to_f128.c
        i64_to_f32.c
        i64_to_f64.c
        ${HERCMODS}s_addMagsF128.c
        ${HERCMODS}s_addMagsF32.c
        ${HERCMODS}s_addMagsF64.c
        s_mulAddF128.c
        ${HERCMODS}s_mulAddF32.c
        ${HERCMODS}s_mulAddF64.c
        s_normRoundPackToF128.c
        s_normRoundPackToF32.c
        s_normRoundPackToF64.c
        s_normSubnormalF128Sig.c
        s_normSubnormalF32Sig.c
        s_normSubnormalF64Sig.c
        ${HERCMODS}s_roundPackToF128.c
        ${HERCMODS}s_roundPackToF32.c
        ${HERCMODS}s_roundPackToF64.c
        ${HERCMODS}s_roundPackToI32.c
        ${HERCMODS}s_roundPackToI64.c
        ${HERCMODS}s_roundPackToUI32.c
        ${HERCMODS}s_roundPackToUI64.c
        s_subMagsF128.c
        s_subMagsF32.c
        s_subMagsF64.c
        ${HERCMODS}softfloat_state.c
        ui32_to_f128.c
        ui32_to_f32.c
        ui32_to_f64.c
        ui64_to_f128.c
        ui64_to_f32.c
        ui64_to_f64.c
        include/internals.h
        include/primitives.h
        include/primitiveTypes.h
        include/softfloat.h
        include/softfloat_stdTypes.h
        include/softfloat_types.h
        ../platform.h.in

    )
list( SORT _l )
set(    SoftFloat_OTHERS "" )
foreach( _s IN LISTS _l )
    list(   APPEND
            SoftFloat_OTHERS
            source/${_s} )
endforeach()

# 	- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#   Hercules-required additions to SoftFloat
set(    _l
        f32_scaledResult.c
        f64_scaledResult.c
        f128_scaledResult.c
    )
list( SORT _l )
set(    SoftFloat_HERCMODS "" )
foreach( _s IN LISTS _l )
    list(   APPEND
            SoftFloat_HERCMODS
            hercsource/${_s} )
endforeach()


set(    SoftFloat_SRCS
        ${SoftFloat_PRIMITIVES}
        ${SoftFloat_SPECIALISED}
        ${SoftFloat_OTHERS}
        ${SoftFloat_HERCMODS} )

# 	- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#   Define the target for the static library.

add_library( SoftFloat STATIC ${SoftFloat_SRCS} )
set_target_properties(  SoftFloat PROPERTIES
						OUTPUT_NAME SoftFloat
						PUBLIC_HEADER "${public_headers}" )


# Define the location of the public headers that need to be part of the
# exported targets (build tree and install) for this target.  The
# generator expression $<BUILD_INTERFACE:...> has the expected value
# at build time and is the null string at install time, and vice versa
# for $<INSTALL_INTERFACE:..>.  Without target_include_directories(),
# the public header directory will never appear in the exported targets.

target_include_directories(SoftFloat PUBLIC
                        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
                        $<INSTALL_INTERFACE:include> )


# Because the static library will be included in a shared library,
# Position-independent code is required for non-Windows builds.
if ( NOT WIN32 )
    set_target_properties( SoftFloat PROPERTIES POSITION_INDEPENDENT_CODE ON )
endif ( )


# Install the target.  Include this target in the export of the install
# targets.

install( TARGETS SoftFloat
        EXPORT s3fh
        ARCHIVE DESTINATION lib
        PUBLIC_HEADER DESTINATION include)

# Create the export of the target.  This export can be imported by a user
# of SoftFloat-3a for Hercules (S3FH), such as Hercules, to simplify the
# CMake coding needed to include S3FH.  The install export would be used
# whet a S3FH developer uses S3FH_DIR to point to the S3FH install
# install directory containing the modified code.

install( EXPORT s3fh DESTINATION s3fh-targets )

# Create an export target of the build tree.  This allows the build tree
# to be used as an import target without requiring an install.  The
# build tree target is used by Hercules when SoftFloat-3a For Hercules
# is built as an external package by Hercules.

export( EXPORT s3fh FILE s3fh-targets/s3fh.cmake  )



install(FILES "COPYING.txt" DESTINATION "." )
install(FILES "README.txt"  DESTINATION "." )
install(FILES "README.md"   DESTINATION "." )

# 	- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#   the cpack installers definitions
#
if( WIN32 )
    set( CPACK_GENERATOR "ZIP" )
else()
    set( CPACK_GENERATOR "TGZ" )
endif()

set( CPACK_PACKAGE_NAME         "${PROJECT_NAME}" )
#set( CPACK_PACKAGE_VERSION      "${${PROJECT_NAME}_VERSION}" )
set( CPACK_PACKAGE_FILE_NAME    "${CPACK_PACKAGE_NAME}" )

# 	- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#   the cpack installers
#
include(CPack)

get_filename_component( CURRENT_LIST_FILE "${CMAKE_CURRENT_LIST_FILE}" NAME)
Vdump( "${CURRENT_LIST_FILE}" "at_exit" )
