
#----------------------------------------------------------
# In addition to the below 3211.core file this test
# also uses an associated "3211.rexx" script as well.
#----------------------------------------------------------

*If \$rexx_supported
    *Message SKIPPING: Testcase 3211 printer
    *Message REASON:   No Hercules Rexx support.
*Else
    *If $rexx_VERSION = ''
        *Message SKIPPING: Testcase 3211 printer
        *Message REASON:   Rexx is not installed.
    *Else

        *If $rexx_package = 'Regina'

          *Testcase 3211 printer (Regina)

          # Prepare test environment

          mainsize  1
          numcpu    1
          sysclear
          archlvl   390
          loadcore  "$(testpath)/3211.core"
          detach    00f
          attach    00f 3211 "3211.txt"
          diag8cmd  enable noecho     # need diag8 to exec rexx script
          shcmdopt  enable diag8      # rexx script needs shell access

          # Set needed Hercules Rexx processing options

          rexx mode subroutine msglevel off msgprefix off errprefix off resolver on syspath on extensions .rexx start auto

          #---------------------------------------------------
          #  Define the tests we want to be run
          #  Skip tests 3, 4 and 9 because Regina Rexx sucks.
          #---------------------------------------------------

          r ff0=00000000000000000000000000000000    # (start with none)
          r ff0=00f1f20000f5f6f7f800000000000000    # (skip 3, 4 and 9)

          # Run the tests...
          runtest   1                 # (plenty of time)

          # Clean up afterwards
          detach    000f              # (no longer needed)
          diag8cmd  disable noecho    # (no longer needed)
          shcmdopt  disable nodiag8   # (no longer needed)

          *Compare
          r 1000.10
          *Want "Return Code flags" 00000000 00000000 00000000 00000000

          *Done

        *Else

          *If $rexx_SOURCE = 'DARWIN'

            *Testcase 3211 printer (Apple)

            # Prepare test environment

            mainsize  1
            numcpu    1
            sysclear
            archlvl   390
            loadcore  "$(testpath)/3211.core"
            detach    00f
            attach    00f 3211 "3211.txt"
            diag8cmd  enable noecho     # need diag8 to exec rexx script
            shcmdopt  enable diag8      # rexx script needs shell access

            # Set needed Hercules Rexx processing options

            rexx mode subroutine msglevel off msgprefix off errprefix off resolver on syspath on extensions .rexx start auto

            #----------------------------------------------------------
            #  Define the tests we want to be run
            #  Skip tests 3, 4 and 9 because the only OORexx available
            #  for Apple Mac is OORexx 5.0, and it apparently has the
            #  same or similar Rexx bug as Regina! (i.e. for reasons
            #  that are unknown tests 3, 4 and 9 seem to always fail)
            #----------------------------------------------------------

            r ff0=00000000000000000000000000000000    # (start with none)
            r ff0=00f1f20000f5f6f7f800000000000000    # (skip 3, 4 and 9)

            # Run the tests...
            runtest   1                 # (plenty of time)

            # Clean up afterwards
            detach    000f              # (no longer needed)
            diag8cmd  disable noecho    # (no longer needed)
            shcmdopt  disable nodiag8   # (no longer needed)

            *Compare
            r 1000.10
            *Want "Return Code flags" 00000000 00000000 00000000 00000000

            *Done

          *Else

            *Testcase 3211 printer (OORexx)

            # Prepare test environment
            mainsize  1
            numcpu    1
            sysclear
            archlvl   390
            loadcore  "$(testpath)/3211.core"
            detach    00f
            attach    00f 3211 "3211.txt"
            diag8cmd  enable noecho     # need diag8 to exec rexx script
            shcmdopt  enable diag8      # rexx script needs shell access

            # Set needed Hercules Rexx processing options
            rexx mode subroutine msglevel off msgprefix off errprefix off resolver on syspath on extensions .rexx start auto

            # Define the tests we want to be run
            r ff0=00000000000000000000000000000000    # (start with none)
            r ff0=00f1f2f3f4f5f6f7f8f9000000000000    # (default = all)

            # Run the tests...
            runtest   1                 # (plenty of time)

            # Clean up afterwards
            detach    000f              # (no longer needed)
            diag8cmd  disable noecho    # (no longer needed)
            shcmdopt  disable nodiag8   # (no longer needed)

            *Explain
            *Explain If the above is an "unexpected wait state" of 000100D8
            *Explain it means that the DIAG8 instruction completed with a non-
            *Explain zero condition code.  The likely reason for this is the
            *Explain results buffer wasn't large enough because the Hercules
            *Explain command the test issued resulted in an unexpected error
            *Explain message (which couldn't fit into DIAG8's response buffer).
            *Explain
            *Explain If any of the below test completion flags are non-zero
            *Explain it means that particular test has failed.  For example,
            *Explain if the completion flags are 000000F3 F4F5F6F7 F8F90000...
            *Explain it means that tests 3 thru 9 have failed (F3 = '3' etc).
            *Explain

            *Compare
            r 1000.10
            *Want "Return Code flags" 00000000 00000000 00000000 00000000

            * *************************************************
            *  The following should all have valid file sizes.
            *  (i.e. they should all be numbers, e.g. 38, etc)
            *  If they are not numbers (e.g. still "aa"), then
            *  it means the Rexx being used is SEVERLY BROKEN!
            * *************************************************
            *
            *   Test  03   sizes should be the same
            *   Test  04     "      "   "  DIFFERENT
            *   Test  09     "      "   "  the same
            *
            * *************************************************
            r 2100.100  #  SIZ03A  (before)
            r 2200.100  #  SIZ03B  (after) should be SAME
            * *************************************************
            r 2300.100  #  SIZ04A  (before)
            r 2400.100  #  SIZ04B  (after) should be DIFFERENT
            * *************************************************
            r 2500.100  #  SIZ09A  (before)
            r 2600.100  #  SIZ09B  (after) should be SAME
            * *************************************************

            *Done

          *Fi
        *Fi
    *Fi
*Fi
