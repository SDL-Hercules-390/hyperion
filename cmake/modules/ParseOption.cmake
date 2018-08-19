#   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#
macro( parse_option _opt _arg )

    set(_tlist  "1" "ON"  "Y" "YES" "TRUE" )
    set(_flist  "0" "OFF" "N" "NO" "FALSE" "IGNORE" "NOTFOUND" )

    if( "${_arg}" STREQUAL "")
        set("${_opt}_F" FALSE )
        set("${_opt}_V" ""   )
    endif()

    string( TOUPPER "${_arg}" _tmp )

    if( 0 )

    #   check for true
    elseif( ${_tmp} IN_LIST "1" "ON"  "Y" "YES" "TRUE" )
        set("${_opt}_F" TRUE )
        set("${_opt}_V" ""   )

    #   check for false
    elseif( ${_tmp} IN_LIST "0" "OFF" "N" "NO" "FALSE" "IGNORE" "NOTFOUND" )
        set("${_opt}_F" FALSE )
        set("${_opt}_V" ""   )

    #   valid data
    else( )
        set("${_opt}_F" TRUE )
        set("${_opt}_V" "${_arg}" )

    endif()

endmacro()
