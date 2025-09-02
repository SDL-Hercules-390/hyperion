![test image](images/image_header_herculeshyperionSDL.png)
[Return to master README.md](../README.md)
# Notes for FreeBSD
## Contents
1. [Notes](#Notes)
# Notes
Hercules has been built on FreeBSD 14.3 on amd64 using hercules-helper. This port has experimental (but, seemingly, working) QETH TUN support.


The following commandline was used:

CFLAGS='-I compat -I /usr/local/include' ~/hercules-helper/hercules-buildall.sh -v -p --flavor=sdl-hercules --git-branch=develop


Standard utilities as available in pkg were used. The build was tested as working using both clang 19, clang 20 and gcc 15.

 

Lennert Van Alboom 2025-09-02.
