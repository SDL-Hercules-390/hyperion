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

archmode s/370

mainsize -1
*Error HHC01451E Invalid value -1 specified for mainsize

mainsize 0
*Error HHC01451E Invalid value 0 specified for mainsize

mainsize 1
*Info HHC17003I MAIN     storage is 1M (mainsize); storage is not locked

mainsize 1B
*Info HHC17003I MAIN     storage is 4K (mainsize); storage is not locked

mainsize 2K
*Info HHC17003I MAIN     storage is 4K (mainsize); storage is not locked

mainsize 65535B
*Info HHC17003I MAIN     storage is 64K (mainsize); storage is not locked

mainsize 65536B
*Info HHC17003I MAIN     storage is 64K (mainsize); storage is not locked

mainsize 16777215B
*Info HHC17003I MAIN     storage is 16M (mainsize); storage is not locked

mainsize 16777216B
*Info HHC17003I MAIN     storage is 16M (mainsize); storage is not locked

mainsize 16777217B
*Info HHC17003I MAIN     storage is 16388K (mainsize); storage is not locked

mainsize 16m
*Info HHC17003I MAIN     storage is 16M (mainsize); storage is not locked

mainsize 16M
*Info HHC17003I MAIN     storage is 16M (mainsize); storage is not locked

mainsize 1g
*Error HHC01451E Invalid value 1g specified for mainsize

mainsize 1G
*Error HHC01451E Invalid value 1G specified for mainsize

mainsize 4K
*Info HHC17003I MAIN     storage is 4K (mainsize); storage is not locked

#-------------------------------------------------------------------------------

archmode esa/390

*Info HHC17003I MAIN     storage is 1M (mainsize); storage is not locked

mainsize 1b
*Info HHC17003I MAIN     storage is 1M (mainsize); storage is not locked

#-------------------------------------------------------------------------------

mainsize 2g

*If $ptrsize = 4
    *If $platform = "Windows"
        *Error 1 HHC01430S Error in function configure_storage(2G): Not enough space
    *Else # e.g. Linux
        *Error 1 HHC01430S Error in function configure_storage(2G): Cannot allocate memory
    *Fi
    *Error   HHC02388E Configure storage error -1
*Else # 64 bit
    *Info    HHC17003I MAIN     storage is 2G (mainsize); storage is not locked
*Fi

#-------------------------------------------------------------------------------

mainsize 2147483647B

*If $ptrsize = 4
    *If $platform = "Windows"
        *Error 1 HHC01430S Error in function configure_storage(2G): Not enough space
    *Else # e.g. Linux
        *Error 1 HHC01430S Error in function configure_storage(2G): Cannot allocate memory
    *Fi
    *Error   HHC02388E Configure storage error -1
*Else # 64 bit
    *Info    HHC17003I MAIN     storage is 2G (mainsize); storage is not locked
*Fi

#-------------------------------------------------------------------------------

mainsize 2147483648b

*If $ptrsize = 4
    *If $platform = "Windows"
        *Error 1 HHC01430S Error in function configure_storage(2G): Not enough space
    *Else # e.g. Linux
        *Error 1 HHC01430S Error in function configure_storage(2G): Cannot allocate memory
    *Fi
    *Error   HHC02388E Configure storage error -1
*Else # 64 bit
    *Info    HHC17003I MAIN     storage is 2G (mainsize); storage is not locked
*Fi

#-------------------------------------------------------------------------------

mainsize 2147483649B
*Error HHC01451E Invalid value 2147483649B specified for mainsize

#-------------------------------------------------------------------------------

archmode z/Arch

mainsize 16e

*If $ptrsize = 4
    *Error HHC01451E Invalid value 16e specified for mainsize
*Else # 64 bit
    *Error HHC02388E Configure storage error -1
*Fi

#-------------------------------------------------------------------------------

mainsize 17E
*Error HHC01451E Invalid value 17E specified for mainsize

#-------------------------------------------------------------------------------

archmode s/370

mainsize 4k
*Info HHC17003I MAIN     storage is 4K (mainsize); storage is not locked

#-------------------------------------------------------------------------------

archmode z/Arch

# PROGRAMMING NOTE: below test must be "*Info 2 ..." because
# switching from archmode S/370 to z/Arch or vice versa now
# causes the LPARNUM and CPUIDFMT commands to also be issued
# resulting in 2 extra messages. See "Release Notes".

*Info 2 HHC17003I MAIN     storage is 1M (mainsize); storage is not locked

*Done nowait

#-------------------------------------------------------------------------------
