#   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#

#   commit count
execute_process( COMMAND ${GIT_EXECUTABLE} rev-list --all --count
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    RESULT_VARIABLE _r
    ERROR_VARIABLE  _e
    OUTPUT_VARIABLE _o
    OUTPUT_STRIP_TRAILING_WHITESPACE )
if( NOT ${_r} EQUAL 0 )
    set( GIT_WC_COMMITS "invalid" )
    message( "Command \"${GIT_EXECUTABLE}\" in directory ${path} failed with error:\n${error}" )
else()
    string( STRIP ${_o} GIT_WC_COMMITS )
endif()

#   hash
execute_process( COMMAND ${GIT_EXECUTABLE} log -1 --pretty=format:%H
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    RESULT_VARIABLE _r
    ERROR_VARIABLE  _e
    OUTPUT_VARIABLE _o
    OUTPUT_STRIP_TRAILING_WHITESPACE )
if( NOT ${_r} EQUAL 0 )
    set( GIT_WC_HASH "invalid" )
    message( "Command \"${GIT_EXECUTABLE}\" in directory ${path} failed with error:\n${error}" )
else()
    string( SUBSTRING ${_o} 0  12 GIT_WC_HASH )
endif()

#   pending changes
execute_process( COMMAND ${GIT_EXECUTABLE} diff-index --name-only HEAD -- 
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    RESULT_VARIABLE _r
    ERROR_VARIABLE  _e
    OUTPUT_VARIABLE _o
    OUTPUT_STRIP_TRAILING_WHITESPACE )
if( NOT ${_r} EQUAL 0 )
    set( GIT_WC_CHANGES "invalid" )
    message( "Command \"${GIT_EXECUTABLE}\" in directory ${path} failed with error:\n${error}" )
else()
    if( "${_o}" STREQUAL "" )
        set( GIT_WC_CHANGES "" )
    else()
        set( GIT_WC_CHANGES "+" )
    endif()
endif()

set( GIT_WC_REVISION "${GIT_WC_COMMITS}-[${GIT_WC_HASH}]${GIT_WC_CHANGES}" )
