#----------------------------------------------------------------------
#                     STIDP  --  Store CPU ID
#----------------------------------------------------------------------
# Fixed test values

cpuverid    AA
cpumodel    4444
cpuserial   666666

#----------------------------------------------------------------------
# Serial number:        (ESA390 and z/Arch)

#   N.....  BASIC mode            N     CPU
#   NL....  LPAR  mode  FMT-0     NL    CPU, LPAR
#   LL....  LPAR  mode  FMT-1     LL    2-digit LPAR

#----------------------------------------------------------------------
#                           S/370
#----------------------------------------------------------------------

*If $can_s370_mode
  *If $max_cpu_engines > 0

#----------------------------------------------------------------------

defsym      test_name         "S/370  BASIC  FMT-0  CPUs = 1"
defsym      test_archlvl       S/370
defsym      test_lparnum              BASIC
defsym      test_cpuidfmt                        0
defsym      test_numcpu                                    1
defsym      panel_cpu                                      0
defsym      cf_1st                                    "cpu 1 cf off"
defsym      cf_2nd                                    "cpu 0 cf on"

defsym      cpuid_ver         AA
defsym      cpuid_ser         666666
defsym      cpuid_mcel        0000

defsym      want_cpuid        "$(cpuid_ver)$(cpuid_ser) 4444$(cpuid_mcel)"

script "$(testpath)/stidp-s370.subtst"

#----------------------------------------------------------------------

defsym      test_name         "S/370  BASIC  FMT-1  CPUs = 1"
defsym      test_archlvl       S/370
defsym      test_lparnum              BASIC
defsym      test_cpuidfmt                        1
defsym      test_numcpu                                    1
defsym      panel_cpu                                      0
defsym      cf_1st                                    "cpu 1 cf off"
defsym      cf_2nd                                    "cpu 0 cf on"

defsym      cpuid_ver         AA
defsym      cpuid_ser         666666
defsym      cpuid_mcel        0000

defsym      want_cpuid        "$(cpuid_ver)$(cpuid_ser) 4444$(cpuid_mcel)"

script "$(testpath)/stidp-s370.subtst"

#----------------------------------------------------------------------

defsym      test_name         "S/370  LPAR  FMT-0  CPUs = 1"
defsym      test_archlvl       S/370
defsym      test_lparnum              3
defsym      test_cpuidfmt                       0
defsym      test_numcpu                                   1
defsym      panel_cpu                                     0
defsym      cf_1st                                   "cpu 1 cf off"
defsym      cf_2nd                                   "cpu 0 cf on"

defsym      cpuid_ver         AA
defsym      cpuid_ser         666666
defsym      cpuid_mcel        0000

defsym      want_cpuid        "$(cpuid_ver)$(cpuid_ser) 4444$(cpuid_mcel)"

script "$(testpath)/stidp-s370.subtst"

#----------------------------------------------------------------------

defsym      test_name         "S/370  LPAR  FMT-1  CPUs = 1"
defsym      test_archlvl       S/370
defsym      test_lparnum              3
defsym      test_cpuidfmt                       1
defsym      test_numcpu                                   1
defsym      panel_cpu                                     0
defsym      cf_1st                                   "cpu 1 cf off"
defsym      cf_2nd                                   "cpu 0 cf on"

defsym      cpuid_ver         AA
defsym      cpuid_ser         666666
defsym      cpuid_mcel        0000

defsym      want_cpuid        "$(cpuid_ver)$(cpuid_ser) 4444$(cpuid_mcel)"

script "$(testpath)/stidp-s370.subtst"

#----------------------------------------------------------------------

  *Else
    *Message SKIPPING: MAXCPU < 1
  *Fi
  *If $max_cpu_engines > 1

#----------------------------------------------------------------------

defsym      test_name         "S/370  BASIC  FMT-0  CPUs = 2"
defsym      test_archlvl       S/370
defsym      test_lparnum              BASIC
defsym      test_cpuidfmt                        0
defsym      test_numcpu                                    2
defsym      panel_cpu                                      1
defsym      cf_1st                                    "cpu 0 cf off"
defsym      cf_2nd                                    "cpu 1 cf on"

defsym      cpuid_ver         AA
defsym      cpuid_ser         666666
defsym      cpuid_mcel        0000

defsym      want_cpuid        "$(cpuid_ver)$(cpuid_ser) 4444$(cpuid_mcel)"

script "$(testpath)/stidp-s370.subtst"

#----------------------------------------------------------------------

defsym      test_name         "S/370  BASIC  FMT-1  CPUs = 2"
defsym      test_archlvl       S/370
defsym      test_lparnum              BASIC
defsym      test_cpuidfmt                        1
defsym      test_numcpu                                    2
defsym      panel_cpu                                      1
defsym      cf_1st                                    "cpu 0 cf off"
defsym      cf_2nd                                    "cpu 1 cf on"

defsym      cpuid_ver         AA
defsym      cpuid_ser         666666
defsym      cpuid_mcel        0000

defsym      want_cpuid        "$(cpuid_ver)$(cpuid_ser) 4444$(cpuid_mcel)"

script "$(testpath)/stidp-s370.subtst"

#----------------------------------------------------------------------

defsym      test_name         "S/370  LPAR  FMT-0  CPUs = 2"
defsym      test_archlvl       S/370
defsym      test_lparnum              3
defsym      test_cpuidfmt                       0
defsym      test_numcpu                                   2
defsym      panel_cpu                                     1
defsym      cf_1st                                   "cpu 0 cf off"
defsym      cf_2nd                                   "cpu 1 cf on"

defsym      cpuid_ver         AA
defsym      cpuid_ser         666666
defsym      cpuid_mcel        0000

defsym      want_cpuid        "$(cpuid_ver)$(cpuid_ser) 4444$(cpuid_mcel)"

script "$(testpath)/stidp-s370.subtst"

#----------------------------------------------------------------------

defsym      test_name         "S/370  LPAR  FMT-1  CPUs = 2"
defsym      test_archlvl       S/370
defsym      test_lparnum              3
defsym      test_cpuidfmt                       1
defsym      test_numcpu                                   2
defsym      panel_cpu                                     1
defsym      cf_1st                                   "cpu 0 cf off"
defsym      cf_2nd                                   "cpu 1 cf on"

defsym      cpuid_ver         AA
defsym      cpuid_ser         666666
defsym      cpuid_mcel        0000

defsym      want_cpuid        "$(cpuid_ver)$(cpuid_ser) 4444$(cpuid_mcel)"

script "$(testpath)/stidp-s370.subtst"

#----------------------------------------------------------------------

  *Else
    *Message SKIPPING: MAXCPU < 2
  *Fi
*Else
  *Message SKIPPING: S/370 not supported
*Fi

#----------------------------------------------------------------------
#                           ESA390
#----------------------------------------------------------------------

*If $can_esa390_mode
  *If $max_cpu_engines > 0

#----------------------------------------------------------------------

defsym      test_name         "ESA/390  BASIC  FMT-0  CPUs = 1"
defsym      test_archlvl       ESA/390
defsym      test_lparnum                BASIC
defsym      test_cpuidfmt                          0
defsym      test_numcpu                                      1
defsym      panel_cpu                                        0
defsym      cf_1st                                      "cpu 1 cf off"
defsym      cf_2nd                                      "cpu 0 cf on"

defsym      cpuid_ver         AA
defsym      cpuid_ser         066666
defsym      cpuid_mcel        0000

defsym      want_cpuid        "$(cpuid_ver)$(cpuid_ser) 4444$(cpuid_mcel)"

script "$(testpath)/stidp-esa390.subtst"

#----------------------------------------------------------------------

defsym      test_name         "ESA/390  BASIC  FMT-1  CPUs = 1"
defsym      test_archlvl       ESA/390
defsym      test_lparnum                BASIC
defsym      test_cpuidfmt                          1
defsym      test_numcpu                                      1
defsym      panel_cpu                                        0
defsym      cf_1st                                      "cpu 1 cf off"
defsym      cf_2nd                                      "cpu 0 cf on"

defsym      cpuid_ver         AA
defsym      cpuid_ser         066666
defsym      cpuid_mcel        0000

defsym      want_cpuid        "$(cpuid_ver)$(cpuid_ser) 4444$(cpuid_mcel)"

script "$(testpath)/stidp-esa390.subtst"

#----------------------------------------------------------------------

defsym      test_name         "ESA/390  LPAR  FMT-0  CPUs = 1"
defsym      test_archlvl       ESA/390
defsym      test_lparnum                3
defsym      test_cpuidfmt                         0
defsym      test_numcpu                                     1
defsym      panel_cpu                                       0
defsym      cf_1st                                     "cpu 1 cf off"
defsym      cf_2nd                                     "cpu 0 cf on"

defsym      cpuid_ver         AA
defsym      cpuid_ser         036666
defsym      cpuid_mcel        0000

defsym      want_cpuid        "$(cpuid_ver)$(cpuid_ser) 4444$(cpuid_mcel)"

script "$(testpath)/stidp-esa390.subtst"

#----------------------------------------------------------------------

defsym      test_name         "ESA/390  LPAR  FMT-1  CPUs = 1"
defsym      test_archlvl       ESA/390
defsym      test_lparnum                3
defsym      test_cpuidfmt                         1
defsym      test_numcpu                                     1
defsym      panel_cpu                                       0
defsym      cf_1st                                     "cpu 1 cf off"
defsym      cf_2nd                                     "cpu 0 cf on"

defsym      cpuid_ver         AA
defsym      cpuid_ser         036666
defsym      cpuid_mcel        8000

defsym      want_cpuid        "$(cpuid_ver)$(cpuid_ser) 4444$(cpuid_mcel)"

script "$(testpath)/stidp-esa390.subtst"

#----------------------------------------------------------------------

  *Else
    *Message SKIPPING: MAXCPU < 1
  *Fi
  *If $max_cpu_engines > 1

#----------------------------------------------------------------------

defsym      test_name         "ESA/390  BASIC  FMT-0  CPUs = 2"
defsym      test_archlvl       ESA/390
defsym      test_lparnum                BASIC
defsym      test_cpuidfmt                          0
defsym      test_numcpu                                      2
defsym      panel_cpu                                        1
defsym      cf_1st                                      "cpu 0 cf off"
defsym      cf_2nd                                      "cpu 1 cf on"

defsym      cpuid_ver         AA
defsym      cpuid_ser         166666
defsym      cpuid_mcel        0000

defsym      want_cpuid        "$(cpuid_ver)$(cpuid_ser) 4444$(cpuid_mcel)"

script "$(testpath)/stidp-esa390.subtst"

#----------------------------------------------------------------------

defsym      test_name         "ESA/390  BASIC  FMT-1  CPUs = 2"
defsym      test_archlvl       ESA/390
defsym      test_lparnum                BASIC
defsym      test_cpuidfmt                          1
defsym      test_numcpu                                      2
defsym      panel_cpu                                        1
defsym      cf_1st                                      "cpu 0 cf off"
defsym      cf_2nd                                      "cpu 1 cf on"

defsym      cpuid_ver         AA
defsym      cpuid_ser         166666
defsym      cpuid_mcel        0000

defsym      want_cpuid        "$(cpuid_ver)$(cpuid_ser) 4444$(cpuid_mcel)"

script "$(testpath)/stidp-esa390.subtst"

#----------------------------------------------------------------------

defsym      test_name         "ESA/390  LPAR  FMT-0  CPUs = 2"
defsym      test_archlvl       ESA/390
defsym      test_lparnum                3
defsym      test_cpuidfmt                         0
defsym      test_numcpu                                     2
defsym      panel_cpu                                       1
defsym      cf_1st                                     "cpu 0 cf off"
defsym      cf_2nd                                     "cpu 1 cf on"

defsym      cpuid_ver         AA
defsym      cpuid_ser         136666
defsym      cpuid_mcel        0000

defsym      want_cpuid        "$(cpuid_ver)$(cpuid_ser) 4444$(cpuid_mcel)"

script "$(testpath)/stidp-esa390.subtst"

#----------------------------------------------------------------------

defsym      test_name         "ESA/390  LPAR  FMT-1  CPUs = 2"
defsym      test_archlvl       ESA/390
defsym      test_lparnum                3
defsym      test_cpuidfmt                         1
defsym      test_numcpu                                     2
defsym      panel_cpu                                       1
defsym      cf_1st                                     "cpu 0 cf off"
defsym      cf_2nd                                     "cpu 1 cf on"

defsym      cpuid_ver         AA
defsym      cpuid_ser         036666
defsym      cpuid_mcel        8000

defsym      want_cpuid        "$(cpuid_ver)$(cpuid_ser) 4444$(cpuid_mcel)"

script "$(testpath)/stidp-esa390.subtst"

#----------------------------------------------------------------------

  *Else
    *Message SKIPPING: MAXCPU < 2
  *Fi
*Else
  *Message SKIPPING: ESA/390 not supported
*Fi

#----------------------------------------------------------------------
#                           z/Arch
#----------------------------------------------------------------------

*If $can_zarch_mode
  *If $max_cpu_engines > 0

#----------------------------------------------------------------------

defsym      test_name         "z/Arch  BASIC  FMT-0  CPUs = 1"
defsym      test_archlvl       z/Arch
defsym      test_lparnum               BASIC
defsym      test_cpuidfmt                         0
defsym      test_numcpu                                     1
defsym      panel_cpu                                       0
defsym      cf_1st                                     "cpu 1 cf off"
defsym      cf_2nd                                     "cpu 0 cf on"

defsym      cpuid_ver         00
defsym      cpuid_ser         066666
defsym      cpuid_mcel        0000

defsym      want_cpuid        "$(cpuid_ver)$(cpuid_ser) 4444$(cpuid_mcel)"

script "$(testpath)/stidp-zarch.subtst"

#----------------------------------------------------------------------

defsym      test_name         "z/Arch  BASIC  FMT-1  CPUs = 1"
defsym      test_archlvl       z/Arch
defsym      test_lparnum               BASIC
defsym      test_cpuidfmt                         1
defsym      test_numcpu                                     1
defsym      panel_cpu                                       0
defsym      cf_1st                                     "cpu 1 cf off"
defsym      cf_2nd                                     "cpu 0 cf on"

defsym      cpuid_ver         00
defsym      cpuid_ser         066666
defsym      cpuid_mcel        0000

defsym      want_cpuid        "$(cpuid_ver)$(cpuid_ser) 4444$(cpuid_mcel)"

script "$(testpath)/stidp-zarch.subtst"

#----------------------------------------------------------------------

defsym      test_name         "z/Arch  LPAR  FMT-0  CPUs = 1"
defsym      test_archlvl       z/Arch
defsym      test_lparnum               3
defsym      test_cpuidfmt                        0
defsym      test_numcpu                                    1
defsym      panel_cpu                                      0
defsym      cf_1st                                    "cpu 1 cf off"
defsym      cf_2nd                                    "cpu 0 cf on"

defsym      cpuid_ver         00
defsym      cpuid_ser         036666
defsym      cpuid_mcel        0000

defsym      want_cpuid        "$(cpuid_ver)$(cpuid_ser) 4444$(cpuid_mcel)"

script "$(testpath)/stidp-zarch.subtst"

#----------------------------------------------------------------------

defsym      test_name         "z/Arch  LPAR  FMT-1  CPUs = 1"
defsym      test_archlvl       z/Arch
defsym      test_lparnum               3
defsym      test_cpuidfmt                        1
defsym      test_numcpu                                    1
defsym      panel_cpu                                      0
defsym      cf_1st                                    "cpu 1 cf off"
defsym      cf_2nd                                    "cpu 0 cf on"

defsym      cpuid_ver         00
defsym      cpuid_ser         036666
defsym      cpuid_mcel        8000

defsym      want_cpuid        "$(cpuid_ver)$(cpuid_ser) 4444$(cpuid_mcel)"

script "$(testpath)/stidp-zarch.subtst"

#----------------------------------------------------------------------

  *Else
    *Message SKIPPING: MAXCPU < 1
  *Fi
  *If $max_cpu_engines > 1

#----------------------------------------------------------------------

defsym      test_name         "z/Arch  BASIC  FMT-0  CPUs = 2"
defsym      test_archlvl       z/Arch
defsym      test_lparnum               BASIC
defsym      test_cpuidfmt                         0
defsym      test_numcpu                                     2
defsym      panel_cpu                                       1
defsym      cf_1st                                     "cpu 0 cf off"
defsym      cf_2nd                                     "cpu 1 cf on"

defsym      cpuid_ver         00
defsym      cpuid_ser         166666
defsym      cpuid_mcel        0000

defsym      want_cpuid        "$(cpuid_ver)$(cpuid_ser) 4444$(cpuid_mcel)"

script "$(testpath)/stidp-zarch.subtst"

#----------------------------------------------------------------------

defsym      test_name         "z/Arch  BASIC  FMT-1  CPUs = 2"
defsym      test_archlvl       z/Arch
defsym      test_lparnum               BASIC
defsym      test_cpuidfmt                         1
defsym      test_numcpu                                     2
defsym      panel_cpu                                       1
defsym      cf_1st                                     "cpu 0 cf off"
defsym      cf_2nd                                     "cpu 1 cf on"

defsym      cpuid_ver         00
defsym      cpuid_ser         166666
defsym      cpuid_mcel        0000

defsym      want_cpuid        "$(cpuid_ver)$(cpuid_ser) 4444$(cpuid_mcel)"

script "$(testpath)/stidp-zarch.subtst"

#----------------------------------------------------------------------

defsym      test_name         "z/Arch  LPAR  FMT-0  CPUs = 2"
defsym      test_archlvl       z/Arch
defsym      test_lparnum               3
defsym      test_cpuidfmt                        0
defsym      test_numcpu                                    2
defsym      panel_cpu                                      1
defsym      cf_1st                                    "cpu 0 cf off"
defsym      cf_2nd                                    "cpu 1 cf on"

defsym      cpuid_ver         00
defsym      cpuid_ser         136666
defsym      cpuid_mcel        0000

defsym      want_cpuid        "$(cpuid_ver)$(cpuid_ser) 4444$(cpuid_mcel)"

script "$(testpath)/stidp-zarch.subtst"

#----------------------------------------------------------------------

defsym      test_name         "z/Arch  LPAR  FMT-1  CPUs = 2"
defsym      test_archlvl       z/Arch
defsym      test_lparnum               3
defsym      test_cpuidfmt                        1
defsym      test_numcpu                                    2
defsym      panel_cpu                                      1
defsym      cf_1st                                    "cpu 0 cf off"
defsym      cf_2nd                                    "cpu 1 cf on"

defsym      cpuid_ver         00
defsym      cpuid_ser         036666
defsym      cpuid_mcel        8000

defsym      want_cpuid        "$(cpuid_ver)$(cpuid_ser) 4444$(cpuid_mcel)"

script "$(testpath)/stidp-zarch.subtst"

#----------------------------------------------------------------------

  *Else
    *Message SKIPPING: MAXCPU < 2
  *Fi
*Else
*Message SKIPPING: z/Arch not supported
*Fi

#----------------------------------------------------------------------
# PROGRAMMING NOTE: because this particular test script involves
# configuring CPUs online and offline, it introduces a potential
# race condition between script processing and CPU threads being
# created and starting up and CPU threads being asked to end and
# exit. As a result of this, some test scripts which follow this
# one can potentially crash (segfault) depending on the timing.
#
# This usually occurs (WHEN it does occur; it doesn't always) on
# my Linux VMware virtual machine (which makes sense, since it's
# not running at native host speed and its kernel thread creation
# and destruction logic is completely different from Windows's)
# but has never occurred (yet!) on my Windows host (which again,
# makes sense since it's running at native speed).
#
# So to be safe we introduce a slight pause to give any internal
# processing a chance to complete before we continue on with the
# next test script.

pause 0.75      # (should HOPEFULLY be PLENTY long enough!)

#----------------------------------------------------------------------
