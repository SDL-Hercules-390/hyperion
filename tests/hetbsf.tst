#----------------------------------------------------------------------
#             Test S/370 HET tape BSF into Load Point
#----------------------------------------------------------------------

# PROGRAMMING NOTE: the bug originally reported only occurred when
# .het files were used but did not occur when .aws files were used.
# Nevertheless for thoroughness we test both types and other types
# may be added in the future.

*Testcase S/370 ZLIB HET tape BSF into Load Point

*If $ZLIB

defsym  tapecuu     590                 # just a device number
defsym  ftype       het                 # tape filename filetype

defsym  tapefile    "$(testpath)/$(ftype)bsf.$(ftype)"    # ZLIB

script "$(testpath)/tapebsf.subtst"

*Else
*Message No ZLIB HET support; skipping Testcase
*Done
*Fi


*Testcase S/370 BZIP2 HET tape BSF into Load Point
*If $HET_BZIP2

defsym  tapecuu     590                 # just a device number
defsym  ftype       het                 # tape filename filetype

defsym  tapefile    "$(testpath)/$(ftype)bsf-bzip2.$(ftype)"    # BZIP2

script "$(testpath)/tapebsf.subtst"

*Else
*Message No BZIP2 HET support; skipping Testcase
*Done
*Fi
