![test image](images/image_header_herculeshyperionSDL.png)
[Return to master README.md](../README.md)
# Notes for FreeBSD and to some extent also OS/X
## Contents
1. [Notes](#Notes)
# Notes
Hercules has been built on FreeBSD and on its derivative, OS/X.  While untested, there should be more than a sporting chance that it builds on NetBSD and OpenBSD too.

You need to install the ports of the gnu m4, make, and sed commands in addition to the packages mentioned in the INSTALL file as Hercules will not build with the original BSD version of these utilities.

To avoid modifying a plethora of files, you should add these aliases to the .bashrc file of the user that builds Hercules:
```
alias make=gmake
alias sed=gsed
alias m4=gm4
```

Also, beware that many of the autoxxxx ports are rather old in the tooth in the FreeBSD ports collection.

In particular, the message `missing: Unknown '--is-lightweight'` indicates that someone has been a little too eager.

John P. Hartmann 2014-08-04.
