#------------------------------------------------------------------------------
#                           GIT_Revision.cmake
#------------------------------------------------------------------------------

# Output values:  GIT_COMMIT_COUNT, GIT_HASH7 and GIT_MODIFIED

#-----------------
#  Commit count
#-----------------

if( WIN32 )

    # PROGRAMMING NOTE: Windows does not come with "wc" so we have to
    # use a different technique. Luckily most Windows versions of git
    # are more modern than the gits that come with older Linux distros
    # and thus very likely support the newer rev-list "--count" option.

    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-list --count HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        RESULT_VARIABLE _r
        ERROR_VARIABLE  _e
        ERROR_STRIP_TRAILING_WHITESPACE
        OUTPUT_VARIABLE _o
        OUTPUT_STRIP_TRAILING_WHITESPACE )

    if( NOT ${_r} EQUAL 0 )
        message( FATAL_ERROR "Command \"git rev-list --count HEAD\" failed with rc=${_r}: ${_e}" )
    endif()

else()

    # PROGRAMMING NOTE: because some older Linux distros don't come with
    # more recent versions of git that understand the rev-list "--count"
    # option, we use the git 'log' command instead and pipe the results
    # into wc to get the count. Also note that when we do this we need to
    # be careful to use "tformat" and not "format" so that the last line
    # gets properly terminated with a newline so that wc then returns the
    # correct count. (Without "tformat" wc returns a count one less than
    # the actual number of commits).

    execute_process(
        COMMAND ${GIT_EXECUTABLE} log --pretty=tformat:''
        COMMAND wc -l
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        RESULT_VARIABLE _r
        ERROR_VARIABLE  _e
        ERROR_STRIP_TRAILING_WHITESPACE
        OUTPUT_VARIABLE _o
        OUTPUT_STRIP_TRAILING_WHITESPACE )

    if( NOT ${_r} EQUAL 0 )
        message( FATAL_ERROR "Command \"git log --pretty=tformat:'' | wc -l\" failed with rc=${_r}: ${_e}" )
    endif()

endif()


string( STRIP ${_o} GIT_COMMIT_COUNT )
trace( GIT_COMMIT_COUNT )

#-----------------
#     Hash
#-----------------

execute_process( COMMAND ${GIT_EXECUTABLE} log -1 --pretty=format:%H
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    RESULT_VARIABLE _r
    ERROR_VARIABLE  _e
    ERROR_STRIP_TRAILING_WHITESPACE
    OUTPUT_VARIABLE _o
    OUTPUT_STRIP_TRAILING_WHITESPACE )

if( NOT ${_r} EQUAL 0 )
    message( FATAL_ERROR "Command \"git log -1 --pretty=format:%H\" failed with rc=${_r}: ${_e}" )
endif()

string( SUBSTRING ${_o} 0 7 GIT_HASH7 )
trace( GIT_HASH7 )

#-----------------
# Pending changes
#-----------------

execute_process( COMMAND ${GIT_EXECUTABLE} diff-index --name-only HEAD --
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    RESULT_VARIABLE _r
    ERROR_VARIABLE  _e
    ERROR_STRIP_TRAILING_WHITESPACE
    OUTPUT_VARIABLE _o
    OUTPUT_STRIP_TRAILING_WHITESPACE )

if( NOT ${_r} EQUAL 0 )
    message( FATAL_ERROR "Command \"git diff-index --name-only HEAD --\" failed with rc=${_r}: ${_e}" )
endif()

if( "${_o}" STREQUAL "" )
    set( GIT_MODIFIED "" )
else()
    set( GIT_MODIFIED "-modified" )
endif()
trace( GIT_MODIFIED )

#------------------------------------------------------------------------------
