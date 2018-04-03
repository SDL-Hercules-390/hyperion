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
# Workaround for unresolved timing issue/bug that only occurs on Linux
# but never on Windows. For some as-yet unknown reason something this
# .tst script seems to cause whichever .tst script follows this one to
# always crash (segfault). Introducing a short delay seems to resolve
# the issue but the reasons why are as-yet unknown.

*If $platform \= "Windows"
  pause 1
*Fi

#----------------------------------------------------------------------
