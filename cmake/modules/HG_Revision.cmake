#   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#
execute_process( COMMAND ${HG_EXECUTABLE} id -i
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    RESULT_VARIABLE _result
    ERROR_VARIABLE  _error
    OUTPUT_VARIABLE _output
    OUTPUT_STRIP_TRAILING_WHITESPACE )
if( NOT ${_result} EQUAL 0 )
    set( HG_WC_REVISION "invalid" )
    message( "Command \"${HG_EXECUTABLE}\" in directory ${path} failed with error:\n${error}" )
else()
    set( HG_WC_REVISION ${_output} )
    set( REVISION_STRING  "HG_Revision[${_hash}]" )
endif()
