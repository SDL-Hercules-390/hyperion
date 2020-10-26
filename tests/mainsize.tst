#-------------------------------------------------------------------------------

*Testcase mainsize check storage size

# This file was put into the public domain 2015-11-17 by John P.
# Hartmann. You can use it for anything you like, as long as this
# notice remains.

#-------------------------------------------------------------------------------

msglevel -debug
numcpu 1

*Compare

#-------------------------------------------------------------------------------

archlvl s/370

* Test 1
mainsize -1
*Error HHC01451E Invalid value -1 specified for MAINSIZE

* Test 2
mainsize 0
*Error HHC01451E Invalid value 0 specified for MAINSIZE

* Test 3
mainsize 1
*Info HHC17003I MAIN     storage is 1M (mainsize); storage is not locked

* Test 4
mainsize 1B
*Error HHC01451E Invalid value 1B specified for MAINSIZE

* Test 5
mainsize 2K
*Error HHC01451E Invalid value 2K specified for MAINSIZE

* Test 6
mainsize 65535B
*Info HHC17003I MAIN     storage is 64K (mainsize); storage is not locked

* Test 7
mainsize 65536B
*Info HHC17003I MAIN     storage is 64K (mainsize); storage is not locked

* Test 8
mainsize 16777215B
*Info HHC17003I MAIN     storage is 16M (mainsize); storage is not locked

* Test 9
mainsize 16777216B
*Info HHC17003I MAIN     storage is 16M (mainsize); storage is not locked

* Test 10
mainsize 16777217B
*Info HHC17003I MAIN     storage is 16388K (mainsize); storage is not locked

* Test 11
mainsize 16m
*Info HHC17003I MAIN     storage is 16M (mainsize); storage is not locked

* Test 12  (usually fails with 32-bit build of Hercules due to limited memory)

*If $ptrsize \= 4
  mainsize 1g
  *Info HHC17003I MAIN     storage is 1G (mainsize); storage is not locked
*Fi

* Test 13
mainsize 4K
*Error HHC01451E Invalid value 4K specified for MAINSIZE

#-------------------------------------------------------------------------------

* Test 14

archlvl esa/390

mainsize 1b
*Error HHC01451E Invalid value 1b specified for MAINSIZE

#-------------------------------------------------------------------------------

* Test 15

mainsize 2g

*If $ptrsize = 4
    *If $platform = "Windows"
        *Error 1 HHC01430S Error in function configure_storage( 2G ): Not enough space
    *Else # e.g. Linux
        *Error 1 HHC01430S Error in function configure_storage( 2G ): Cannot allocate memory
    *Fi
    *Error   HHC02388E Configure storage error -1
*Else # 64 bit
    *Info    HHC17003I MAIN     storage is 2G (mainsize); storage is not locked
*Fi

#-------------------------------------------------------------------------------

* Test 16

mainsize 2147483647B

*If $ptrsize = 4
    *If $platform = "Windows"
        *Error 1 HHC01430S Error in function configure_storage( 2G ): Not enough space
    *Else # e.g. Linux
        *Error 1 HHC01430S Error in function configure_storage( 2G ): Cannot allocate memory
    *Fi
    *Error   HHC02388E Configure storage error -1
*Else # 64 bit
    *Info    HHC17003I MAIN     storage is 2G (mainsize); storage is not locked
*Fi

#-------------------------------------------------------------------------------

* Test 17

mainsize 2147483648b

*If $ptrsize = 4
    *If $platform = "Windows"
        *Error 1 HHC01430S Error in function configure_storage( 2G ): Not enough space
    *Else # e.g. Linux
        *Error 1 HHC01430S Error in function configure_storage( 2G ): Cannot allocate memory
    *Fi
    *Error   HHC02388E Configure storage error -1
*Else # 64 bit
    *Info    HHC17003I MAIN     storage is 2G (mainsize); storage is not locked
*Fi

#-------------------------------------------------------------------------------

* Test 18

mainsize 2147483649B
*Error HHC01451E Invalid value 2147483649B specified for MAINSIZE

#-------------------------------------------------------------------------------

* Test 19

archlvl z/Arch

mainsize 16e
*Error HHC01451E Invalid value 16e specified for MAINSIZE

#-------------------------------------------------------------------------------

* Test 20

mainsize 17E
*Error HHC01451E Invalid value 17E specified for MAINSIZE

#-------------------------------------------------------------------------------

* Test 21

archlvl s/370

mainsize 4k
*Error HHC01451E Invalid value 4k specified for MAINSIZE

#-------------------------------------------------------------------------------

* Test 22

archlvl z/Arch

*Info 3 HHC00811I Processor CP00: architecture mode z/Arch
*Info 2 HHC02204I ARCHLVL        set to z/Arch

#-------------------------------------------------------------------------------
# Test automatic MAINSIZE adjustment (raise/lower) on architecture switch

* Test 23

*If $ptrsize \= 4

  archlvl z/Arch
  mainsize 3g

  archlvl s/370
  *Info 1 HHC17006W MAINSIZE decreased to 2G architectural maximum

*Fi

* Test 24

mainsize 64k
*Info HHC17003I MAIN     storage is 64K (mainsize); storage is not locked

* Test 25

archlvl z/Arch
*Info 2 HHC17006W MAINSIZE increased to 1M architectural minimum

*Done nowait

#-------------------------------------------------------------------------------
