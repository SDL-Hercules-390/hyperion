#------------------------------------------------------------------------------
#                         CapitalizeWord.cmake
#------------------------------------------------------------------------------

macro ( capitalize_word _word _caps)

    string( SUBSTRING ${_word} 0 1 _head )
    string( TOUPPER   ${_head} _head )
    string( SUBSTRING ${_word} 1 -1 _tail )
    string( TOLOWER   ${_tail} _tail )
    set( "${_caps}" "${_head}${_tail}" )

endmacro ()
