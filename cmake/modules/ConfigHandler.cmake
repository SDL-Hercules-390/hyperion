#   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#
include( CMakeParseArguments )

#   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#
function( config_open )

    cmake_parse_arguments( args "REQ;OPT" "F" "" ${ARGN} )

    if( NOT "${args_F}" STREQUAL "" )
        string( TOLOWER "${args_F}" _file )
        set( _file "${CMAKE_BINARY_DIR}/${_file}.h.in" )
        message( "++ at config_open _file '${_file}' " )
        set( _flag "IN_${args_F}_H")
        string( REGEX REPLACE "[^a-zA-Z0-9_]" "_" _flag "${_flag}" )
        string( TOUPPER "${_flag}" _flag)

        file( REMOVE "${_file}" )
	    file( APPEND "${_file}" "#ifndef ${_flag}\n")
	    file( APPEND "${_file}" "#define ${_flag}\n")
    endif()
    
endfunction()

#   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#
function( config_close )

    cmake_parse_arguments( args "REQ;OPT" "F" "" ${ARGN} )

    if( NOT "${args_F}" STREQUAL "" )
        string( TOLOWER "${args_F}" _file )
        set( _file "${CMAKE_BINARY_DIR}/${_file}.h.in" )

        set( _flag "IN_${args_F}_H")
        string( REGEX REPLACE "[^a-zA-Z0-9_]" "_" _flag "${_flag}" )
        string( TOUPPER "${_flag}" _flag)
	    file( APPEND "${_file}" "\n#endif /* #ifndef ${_flag} */ \n")
    endif()
    
endfunction()

#   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#
function( config_spacer )

    cmake_parse_arguments( args "REQ;OPT" "F" "" ${ARGN} )
    
    if( NOT "${args_F}" STREQUAL "" )
        string( TOLOWER "${args_F}" _file )
        set( _file "${CMAKE_BINARY_DIR}/${_file}.h.in" )
        file( APPEND ${_file} "
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
*/
" )
    endif()

endfunction()    

#   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#
function( config_flag  )

    cmake_parse_arguments( args "REQ;OPT" "F" "" ${ARGN} )
    message( "++ at config_flags_F '${args_F}' " ) 
    
    if( NOT "${args_F}" STREQUAL "" )
        string( TOLOWER "${args_F}" _file )
        set( _file "${CMAKE_BINARY_DIR}/${_file}.h.in" )

        foreach( _flag IN LISTS args_UNPARSED_ARGUMENTS )
            set( _flag "HAVE_${_flag}")
            string( REGEX REPLACE "[^a-zA-Z0-9_]" "_" _flag "${_flag}" )
            string( TOUPPER "${_flag}" _flag)
            file( APPEND ${_file} "#cmakedefine ${_flag}\n" )
        endforeach()
    endif()
    
endfunction()

#   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#
function( config_value )

    cmake_parse_arguments( args "REQ;OPT" "F" "" ${ARGN} )
    message( "++ at config_vars_F '${args_F}' " )
    
    if( NOT "${args_F}" STREQUAL "" )
        string( TOLOWER "${args_F}" _file )
        set( _file "${CMAKE_BINARY_DIR}/${_file}.h.in" )

        foreach( _var IN LISTS args_UNPARSED_ARGUMENTS )
            set( _var "HAVE_${_var}")
            string( REGEX REPLACE "[^a-zA-Z0-9_]" "_" _var "${_var}" )
            string( TOUPPER "${_var}" _var)
            file( APPEND ${_file} "#cmakedefine ${_var}\n" )
        endforeach()
    endif()
    
endfunction()
