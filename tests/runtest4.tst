
msglvl +verbose +emsgloc

* ----------------------------------------------------------------------------
*Testcase runtest4: all cpus 1-4 seconds
* ----------------------------------------------------------------------------
*
numcpu      4           #  Total CPUs needed for this test...
*
* ----------------------------------------------------------------------------
*
defsym  secs0     1     #  Loop duration in seconds for each CPU...
defsym  secs1     2
defsym  secs2     3
defsym  secs3     4
*
defsym  maxdur   10.5   #  Pessimistic test duration
*
* ----------------------------------------------------------------------------
*
script "$(testpath)/runtest.subtst"
*
* ----------------------------------------------------------------------------
numcpu      1           #  Clean up own mess
