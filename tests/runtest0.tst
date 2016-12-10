
msglvl +verbose +emsgloc
msglvl +time +usecs

* ----------------------------------------------------------------------------
*Testcase runtest0: all cpus 0 seconds
* ----------------------------------------------------------------------------
*
numcpu      4           #  Total CPUs needed for this test...
*
* ----------------------------------------------------------------------------
*
defsym  secs0     0     #  Loop duration in seconds for each CPU...
defsym  secs1     0
defsym  secs2     0
defsym  secs3     0
*
defsym  maxdur   0.4    #  Expected test duration, give or take.
*
* ----------------------------------------------------------------------------
*
script "$(testpath)/runtest.subtst"
*
* ----------------------------------------------------------------------------
numcpu      1           #  Clean up own mess
