#------------------------------------------------------------------------------
#                              Vdump.cmake
#------------------------------------------------------------------------------
#
#  This macro dumps all CMake variables to a debug output file skipping
#  all variables deemed to be "uninteresting".  You can call this macro
#  in either one of two different ways:
#
#     vdump(  "${CURRENT_LIST_FILE}"  "${CMAKE_CURRENT_LIST_LINE}"  )
#     vdump(  "${CURRENT_LIST_FILE}"  "custom string"               )
#
#  The first variation will create an output file with the name:
#
#     "CMakeList_txt_12345.txt"
#
#  where "CMakeList_txt" of the name of the CMake list file where vdump
#  was called from and "12345" is of course the line number of the CMake
#  list file where vdump was called from.  The second variation creates
#  a file called:
#
#     "CMakeList_txt_custom_string.txt"
#
#  where "custom_string" is the second argument passed to this macro.
#  Both variations of course have all non-alphanumerc characters in
#  the resulting filename replaced with underscores.
#
#------------------------------------------------------------------------------

macro( vdump _listname _linenum )

    #--------------------------------------------------------------------------
    #  Zero pad the passed line number     (e.g. "42" becomes "0042")
    #--------------------------------------------------------------------------

    if( ${_linenum} MATCHES ^[0-9] )      # if they passed a line number

        set( _where "0000${_linenum}" )
        string( LENGTH "${_where}" _n )
        math( EXPR _n "${_n} - 4" )
        string( SUBSTRING ${_where} ${_n} -1 _where )

    else()
        set( _where "${_linenum}" )       # they passed a custom string
    endif()


    #--------------------------------------------------------------------------
    #  Construct a preliminary output filename
    #--------------------------------------------------------------------------

    set( _suffix    "_vars_at_${_where}"      )
    set( _dumpfile  "${_listname}${_suffix}"  )

    unset( _where )
    unset( _suffix )


    #--------------------------------------------------------------------------
    #  Replace non-alphanumeric characters in the filename with underscores,
    #  convert it to a full path filename and then delete any existing file.
    #--------------------------------------------------------------------------

    string( REGEX REPLACE "[^a-zA-Z0-9_]" "_" _dumpfile "${_dumpfile}")
    set( _dumpfile "${CMAKE_BINARY_DIR}/${_dumpfile}.txt" )
    file( REMOVE ${_dumpfile} )


    #--------------------------------------------------------------------------
    #  Get a LIST of all CMake variables and process them one by one...
    #--------------------------------------------------------------------------

    unset( _dumpdata )
    set( _dumpdata "vdump of interesting \"_listname\" variables...\n\n" )

    get_cmake_property( _varlist VARIABLES )

    foreach( _varname IN LISTS _varlist )

        #--------------------------------------------------------------
        #  Skip variables whose names are less than 4 characters long
        #--------------------------------------------------------------

        string( LENGTH "${_varname}" _n )
        if( _n LESS 4 )
            continue()
        endif()


        #--------------------------------------------------------------
        #  Skip variables that start with underscore since they are,
        #  by common practice, usually only temporary work variables.
        #--------------------------------------------------------------

        string( SUBSTRING "${_varname}" 0 1 _c )
        if( "${_c}" STREQUAL "_" )
            continue()
        endif()


        #--------------------------------------------------------------
        #  Skip any variable whose name is "OUTPUT"
        #--------------------------------------------------------------

        if( "${_varname}" STREQUAL "OUTPUT" )
            continue()
        endif()


        #--------------------------------------------------------------
        #  Skip any variable whose name ends with "_CONTENT"
        #--------------------------------------------------------------

        if( "${_varname}" MATCHES "(_CONTENT)$" )
            continue()
        endif()


        #--------------------------------------------------------------
        #  Accumulate variable name and value in our work variable
        #--------------------------------------------------------------

        set( _varvalue "${${_varname}}" )
        set( _dumpdata "${_dumpdata}++ ${_varname}='${_varvalue}'\n" )


    endforeach()


    #--------------------------------------------------------------------------
    #  Now write everything to our debug file at once with one 'file' call
    #--------------------------------------------------------------------------

    file( WRITE ${_dumpfile} "${_dumpdata}" )

    unset( _dumpdata )
    unset( _dumpfile )

endmacro()
