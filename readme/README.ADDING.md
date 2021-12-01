![header image](images/image_header_herculeshyperionSDL.png)
[Return to master README.md](../README.md)

# Adding New Source Files to Hercules

## Contents

1. [Overview](#Overview)
2. [Repository](#Repository)
3. [Build Files](#Build-Files)
4. [Visual Studio](#Visual-Studio)
5. [Example commits](#Example-commits)

## Overview

Adding new files to the Hercules emulator project would certainly seem like a simple
straightforward thing. Unfortunately there are some additional details which far too
many Hercules developers frequently overlook _(including myself sometimes!)_.

This document attempts to describe these oversights and should be used as internal
reference documentation that all Hercules developers should review on occasion.

Adding new files to the repository is only the _first_ (and most important) step
in the process. But the other steps, which too many developers tend to forget about,
are just as important.

Those additional steps (which many developers tend to overlook) fall into two major areas:

1. _Updating the **build files** so that the new files are included in the build process._
2. _Adding the files to the **Visual Studio project files** (user interface)._

## Repository

Adding a new source file to our git repository is a simple and straightforward process
so I won't bother going into any detail. If you don't already know how to add new files
to the git repository, then you probably have no business being a Hercules developer!

## Build Files

After adding the new source file to the repository you need to update one of the
`Makefile.am` files (non-Windows) _**and**_ one or more of the `msvc.makefile.includes`
files (Windows). Which makefile or msvc.makefile.includes file you need to update
depends on what type of file you are adding.

The easiest way to determine which Makefile (or msvc.makefile.includes file) that you
need to update _(as well as the change you need to make to it)_, is to simply do a
`grep` for the filename of a _similar_ file that already exists in Hercules.

For example, if you are adding a new .c source file pertaining to instruction emulation
`hengine.dll`, you might do a grep for `general1` _(without the extension!)_ and make
your change to all of the same files in the same manner. If you are adding a new html
file, you might do your grep for `hercconf`, etc.

#### _Linux:_

If you are adding a new **`.c` source file**, you need to update the project's main (primary)
`Makefile.am` file in the root directory of the repository:

* [`Makefile.am`](https://github.com/SDL-Hercules-390/hyperion/blob/master/Makefile.am)

If you are adding a new **runtest** test file to the `tests` subdirectory, you need to
update the `Makefile.am` file in _that_ directory:

* [`tests/Makefile.am`](https://github.com/SDL-Hercules-390/hyperion/blob/master/tests/Makefile.am)

If you are adding a new `.html` documentation **webpage** to the `html` subdirectory,
then you need to add your new file to _that_ subdirectory's `Makefile.am` file:

* [`html/Makefile.am`](https://github.com/SDL-Hercules-390/hyperion/blob/master/html/Makefile.am)

#### _MSVC:_

If you are adding a new `.c` source file, you first need to determine which "module"
(DLL) that file is a part of. The collection of object code pertaining to each of our
modules (shared libraries or DLLs) is controlled via the `OBJ_CODE.msvc`, `MODULES.msvc`
and `MOD_RULES2.msvc` makefile fragments in the `msvc.makefile.includes` directory:

* [`OBJ_CODE.msvc`](https://github.com/SDL-Hercules-390/hyperion/blob/master/msvc.makefile.includes/OBJ_CODE.msvc)
(maybe, depending on what type of file you are adding)
* [`MODULES.msvc`](https://github.com/SDL-Hercules-390/hyperion/blob/master/msvc.makefile.includes/MODULES.msvc)
(maybe, depending on what type of file you are adding)
* [`MOD_RULES2.msvc`](https://github.com/SDL-Hercules-390/hyperion/blob/master/msvc.makefile.includes/MOD_RULES2.msvc)
(maybe, depending on what type of file you are adding)

If you are adding a new **runtest** test file to the `tests` subdirectory, you do _not_
need to update any build file. You only need to update one of the **Visual Studio project
files** as discussed in the "Visual Studio" section just below.

If you are adding a new `.html` documentation **webpage** to the `html` subdirectory,
then once again, you _only_ need to update one of the **Visual Studio project files**
as discussed in the next section immediately below.

## Visual Studio

As part of the process of adding a new file to Hercules, it is also important to add
that file to the following Visual Studio project files:

* [Hercules_VS2008.vcproj](https://github.com/SDL-Hercules-390/hyperion/blob/master/Hercules_VS2008.vcproj)

* [Hercules_VS2015.vcxproj](https://github.com/SDL-Hercules-390/hyperion/blob/master/Hercules_VS2015.vcxproj)
* [Hercules_VS2015.vcxproj.filters](https://github.com/SDL-Hercules-390/hyperion/blob/master/Hercules_VS2015.vcxproj.filters)

* [Hercules_VS2017.vcxproj](https://github.com/SDL-Hercules-390/hyperion/blob/master/Hercules_VS2017.vcxproj)
* [Hercules_VS2017.vcxproj.filters](https://github.com/SDL-Hercules-390/hyperion/blob/master/Hercules_VS2017.vcxproj.filters)

* [Hercules_VS2019.vcxproj](https://github.com/SDL-Hercules-390/hyperion/blob/master/Hercules_VS2019.vcxproj)
* [Hercules_VS2019.vcxproj.filters](https://github.com/SDL-Hercules-390/hyperion/blob/master/Hercules_VS2019.vcxproj.filters)

* [Hercules_VS2022.vcxproj](https://github.com/SDL-Hercules-390/hyperion/blob/master/Hercules_VS2022.vcxproj)
* [Hercules_VS2022.vcxproj.filters](https://github.com/SDL-Hercules-390/hyperion/blob/master/Hercules_VS2022.vcxproj.filters)

The change that you need to make to each of the above files should hopefully be obvious,
and you should follow the same format that already exists.

> _**NOTE:** &nbsp; The changes that need to be made to the above Visual Studio
> 2015, 2017, 2019 and 2022 project files are all exactly identical. The easiest
> way to make your changes is to first change the two 2015 files first, and then
> simply copy those exact same changes over into the same 2017, 2019 and 2022 files.
> Please also note that the VS2008 project file is an exception to the rule.
> Its format is completely different, so be sure to make your  changes in the 
> right place._

The easiest way to determine how to make your change is to simply search for
the filename of a _similar_ file that already exists in Hercules.

### Example commits:

Here are some sample commits so you can see the changes that need to be made
when adding a new file to Hercules:

* [New "cckdmap" utility](https://github.com/SDL-Hercules-390/hyperion/commit/ae0313a371fd5c7189dcba4939700a0d3692e007)

* [Add TCPIP (X'75') instruction; see README.TCPIP for details.](https://github.com/SDL-Hercules-390/hyperion/commit/7714b835ebcc8ceeb0a8bd473f7694d2e427b1f8)

But again, the perhaps easiest way is to simple make the same changes that
were made when a _similar_ type file was added in the past.

___

Please let me know if there is anything I can do to explain the above process
more clearly for you.

I know it seems somewhat complicated, but it's not.

_**You basically just need to remember to:**_

* Add the file to the correct autoconf `Makefile.am` file
* Add the file to the correct MSVC makefile fragment _(`OBJ_CODE.msvc` etc)_
* Add the file to all of the Visual Studio project files (`Hercules_VS2008.vcproj`, `Hercules_VS2015.vcxproj`, `Hercules_VS2015.vcxproj.filters`, etc)

And the easiest way to do that is, like I said, by doing it just like
others have done it previously. Do a grep for a similar type file and
review the files and the changes that were made when that file was
added.


_**"Fish"** (David B. Trout)<br>
&nbsp;&nbsp;&nbsp;&nbsp;October 5, 2010_
