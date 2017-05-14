#----------------------------------------------------------------------
#             Test S/370 HET tape BSF into Load Point
#----------------------------------------------------------------------
#
#  PROGRAMMING NOTE: the bug originally reported only occurred when
#  .het files were used but did not occur when .aws files were used.
#  Nevertheless for thoroughness we test both types and other types
#  may be added in the future.
#
#-------------------------------------------------------------------

*Testcase S/370 HET (ZLIB) tape BSF into Load Point

*If $ZLIB

    defsym  tapecuu     590                 # just a device number
    defsym  ftype       het                 # tape filename filetype

    defsym  tapefile    "$(testpath)/$(ftype)bsf.$(ftype)"    # ZLIB

    script "$(testpath)/tapebsf.subtst"

*Else
    *Message SKIPPING: Testcase S/370 HET (ZLIB) tape BSF into Load Point
    *Message REASON:   No ZLIB support
*Fi

*Done

#-------------------------------------------------------------------

*Testcase S/370 HET (BZIP2) tape BSF into Load Point

*If $HET_BZIP2

    defsym  tapecuu     590                 # just a device number
    defsym  ftype       het                 # tape filename filetype

    defsym  tapefile    "$(testpath)/$(ftype)bsf-bzip2.$(ftype)"    # BZIP2

    script "$(testpath)/tapebsf.subtst"

*Else
    *Message SKIPPING: Testcase S/370 HET (BZIP2) tape BSF into Load Point
    *Message REASON:   No BZIP2 support
*Fi

*Done

#-------------------------------------------------------------------
