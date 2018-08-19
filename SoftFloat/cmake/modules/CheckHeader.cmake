#------------------------------------------------------------------------------
#                           CheckHeader.cmake
#------------------------------------------------------------------------------

include( CMakeParseArguments )

#------------------------------------------------------------------------------

function( check_header )

    cmake_parse_arguments( args "REQ;OPT" "F" "" ${ARGN} )

    if( NOT "${args_F}" STREQUAL "" )
        string( TOLOWER "${args_F}" _file )
        set( _file "${CMAKE_BINARY_DIR}/${_file}.h.in" )
    endif()

    foreach( _memb IN LISTS args_UNPARSED_ARGUMENTS )

        set( _flag "HAVE_${_memb}")
        string( REGEX REPLACE "[^a-zA-Z0-9_]" "_" _flag "${_flag}" )
        string( TOUPPER "${_flag}" _flag)

        if( DEFINED _file )
            file( APPEND ${_file} "#cmakedefine ${_flag}\n" )
        endif()

        check_include_file( "${_memb}" "${_flag}" )

        if ( args_REQ )
            if( NOT ${_flag} )
                message( "++ Error processing header ${_memb}" )
            endif()
        endif()

    endforeach()

endfunction()
