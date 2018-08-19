#------------------------------------------------------------------------------
#                           Version.cmake
#------------------------------------------------------------------------------

set( VERS_MAJOR "${PROJECT_VERSION_MAJOR}" )  # First  digit
set( VERS_INTER "${PROJECT_VERSION_MINOR}" )  # Second digit
set( VERS_MINOR "${PROJECT_VERSION_PATCH}" )  # Third  digit

#------------------------------------------------------------------------------

find_package( Git )

if( GIT_FOUND AND EXISTS ${CMAKE_SOURCE_DIR}/.git )

    include( GIT_Revision )
    set( VERS_BUILD "${GIT_COMMIT_COUNT}" )
    set( VERS_EXTRA "-g${GIT_HASH7}${GIT_MODIFIED}" )

endif()

#------------------------------------------------------------------------------


# ...Add support for other repository types (e.g. SVN) here... 


#------------------------------------------------------------------------------

if( NOT "${VERS_BUILD}" )
    set( VERS_BUILD "0" )
endif()

set( VERS_STRING "${VERS_MAJOR}.${VERS_INTER}.${VERS_MINOR}.${VERS_BUILD}${VERS_EXTRA}" )

message( "Building ${FULLNAME} version ${VERS_STRING}" )

#------------------------------------------------------------------------------
