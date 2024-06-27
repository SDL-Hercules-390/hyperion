![test image](images/image_header_herculeshyperionSDL.png)
[Return to master README.md](../README.md)

# Hercules CMPSC "Compression Call" instruction support
## Contents
1. [Notes](#Notes)
## Notes

The z/Architecture Compression Call instruction (CMPSC opcode B263) had at one time two different implementations within Hercules 4.0 Hyperion.

The original (legacy) CMPSC implementation was lacking support for the z/Architecture CMPSC-enhancement facility, failed to pass some cmpsctst testing tool **`(*)`** architecural compliance tests, and was not as fast.

The replacement "cmpsc_2012" implementation not only passes all cmpsctst testing tool architectural compliance tests, but also fully supports the z/Architecture CMPSC-enhancement facility, and is about 10% faster as well. Because of this the old legacy implementation was removed and the new 2012 implementation is now the _only_ implementation in Hercules 4.0 Hyperion.

The CMPSC Compression Call instruction is fully described in painstaking detail in publication "ESA/390 Data Compression" (SA22-7208) as well as in the [z/Architecture Principles of Operation SA22-7832](http://publibfi.boulder.ibm.com/epubs/pdf/dz9zr011.pdf) manual.

The new default cmpsc_2012 implementation logic is spread across several different source files, each handling just one aspect of the instructions overall functionality, and closely follows IBM's flow charts published in each of those manuals:

      cmpsc_2012.c        Primary implementation functions
      cmpsc.h             Implementation-wide common header
      cmpscbit.h          Bit Extraction Helper Macros
      cmpscget.h          Get Next Index Functions
      cmpscget.c          Get Next Index Functions
      cmpscput.h          Put Next Index Functions
      cmpscput.c          Put Next Index Functions
      cmpscdct.h          Get Dictionary Entry Functions
      cmpscdct.c          Get Dictionary Entry Functions
      cmpscmem.h          Memory Access Functions
      cmpscmem.c          Memory Access Functions
      cmpscdbg.h          Debugging Functions (not implemented)
      cmpscdbg.c          Debugging Functions (not implemented)


The original legacy cmpsc implementation was written by Bernard van der Helm of Noordwijkerhout, The Netherlands. The new/improved cmpsc_2012 implementation was written by Fish (David B. Trout) of the United States of America, and borrows some of the techniques that Bernard pioneered.

**`(*)`** <small><i>The CMPSCTST instruction testing tool is maintained separately from Hercules in its own source code repository on GitHub, ships with a pre-built set of ready-to-run binaries (executable files and scripts) for both Windows and Linux (CentOS 6.4 with gcc 4.4), a default set of test files, and contains a detailed README explaining how to not only build the tool for yourself but also how to run its many tests. The repository is located **[here](https://github.com/Fish-Git/cmpsctst)**.</i></small>
