![header image](images/image_header_herculeshyperionSDL.png)
[Return to master README.md](../README.md)

# Building Hercules-390 for AIX 5.3      *PRELIMINARY
## Contents
1. [About](#About)
2. [Process](#Process)
3. [ToDo](#ToDo)

## About
This document gives instructions and information on how to build Hercules-390 for AIX v5.3.

## Process
1. Standard AIX systems do not have autoconf, automake, zlib or bzip2 libraries and headers; these can be downloaded from <http://www.oss4aix.org/download/latest/aix53/> and installed using rpm (rpm is part of AIX 5.3).
Autoconf/automake require some pre-requisites, such as gettext, info, m4, libsigsegv and expat; all these could be downloaded and installed from the same place.

2. Enter the configure and make commands to build/install Hercules:
```
./configure  &&  make  &&  make install
```
(or you can run each command separately if you want).

3. That's it! Enjoy your private mainframe. :)

The above is based on efforts done by Alexey Bozrikov [bozy@fgm.com.cy], Harold Grovesteen [h.grovsteen@tx.rr.com] and Fish [fish@softdevlabs.com] on the [Hercules-390 Yahoo group](https://groups.yahoo.com/neo/groups/hercules-390) list during the month of October 2009.

## ToDo:
The followings items are still remaining to do: 
  - Get SCSI tape working: resolve `<sys/mtio.h>` header & `struct mtget` issue(s).
```
#if defined( HAVE_STRUCT_MTGET_MT_GSTAT )
#else // !defined( HAVE_STRUCT_MTGET_MT_GSTAT )
#endif // defined( HAVE_STRUCT_MTGET_MT_GSTAT )
```
- Test networking (did you do this yet Alexey?).
