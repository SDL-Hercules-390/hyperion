#----------------------------------------------------------------------
#             Test S/370 AWS tape BSF into Load Point
#----------------------------------------------------------------------
#
#  PROGRAMMING NOTE: the bug originally reported only occurred when
#  .het files were used but did not occur when .aws files were used.
#  Nevertheless for thoroughness we test both types and other types
#  may be added in the future.
#
#-------------------------------------------------------------------

*Testcase S/370 AWS tape BSF into Load Point

defsym  tapecuu     590                 # just a device number
defsym  ftype       aws                 # tape filename filetype
defsym  tapefile    "$(testpath)/$(ftype)bsf.$(ftype)"

script "$(testpath)/tapebsf.subtst"

*Done

#-------------------------------------------------------------------
