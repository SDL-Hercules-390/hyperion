
#----------------------------------------------------------
# In addition to the below 3211.core file this test
# also uses an associated "3211.rexx" script as well.
#----------------------------------------------------------

*Testcase 3211 printer

*If \$rexx_supported
    *Message SKIPPING: Testcase 3211 printer
    *Message REASON:   No Hercules Rexx support.
*Else
    *If $rexx_VERSION = ''
        *Message SKIPPING: Testcase 3211 printer
        *Message REASON:   Rexx is not installed.
    *Else

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

        # Run the test...
        runtest   0.1               # (plenty of time)

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

    *Fi
*Fi

*Done
