![test image](images/image_header_herculeshyperionSDL.png)
[Return to master README.md](../README.md)

# Hercules Dynamic Modules (on Windows)

## Contents
1. [Required Files](#Required-Files)
2. [Required makefile format](#Required-makefile-format)
3. [Building (making)](#Building-making)
4. [Pre-Build event and Post-Build event callbacks](#Pre-Build-event-and-Post-Build-event-callbacks)
5. [Resource Compiler](#Resource-Compiler)
6. [The Build Process](#The-Build-Process)
7. [More Information](#More-Information)

## Required files
```
  makefile:       "{modname}.msvc"    defines module name and source file(s)
  resource file:  "{modname}.rc"      the module's version resource file
```

## Required makefile format

    # Module name:

    DYNMOD = {modname}

    # Source files:

    DYNOBJ = \
        $(O){srcfile1}.obj \
        $(O){srcfile2}.obj \
        $(O){srcfile3}.obj
        ... etc...

Your makefile is `!INCLUDEd` as part of Hercules's main makefile and thus your dynamic module gets built along with the rest of Hercules.

## Building (making)

    dynmake.bat  {projdir}  {modname}  {build_type}  {num_cpus}  [-a|clean]

e.g.: `"X:\Hercules\dynmake.bat"  "$(SolutionDir)"  {modname}  RETAIL  32  -a`

The `{projdir}` value you pass MUST be a fully qualified path to your dynamic module's project directory where all of your files are. The `dynamke.bat` command you invoke should also be a fully qualifed path to the desired Hercules `dynmake.bat` file. The other arguments (i.e. `{build_type}`, `{num_cpus}`, etc) are identical to the values normally specified for the main Hercules `makefile.bat` command used to build Hercules with. Refer to [`makefile.bat`](../makefile.bat) for details.

## Pre-Build event and Post-Build event callbacks

Optional files:
```
    prebuild.bat          Called before the main Hercules makefile.bat is called.
    postbld.bat           Called after  the main Hercules makefile.bat is called.
```

During the build process, `dynmake.bat` checks if the file exists in your specified project directory and if it does, calls it with the following parameters:

        {hercdir}  {modname}  {build_type}  {num_cpus}  [-a|clean]

The `{hercdir}` value is the fully qualified path the main Hercules source code directory. The other parameters are the same values that you passed to the [dynmake.bat](../dynmake.bat) command.

## Resource Compiler

For your convenience the following #defines are also passed to the resource compiler on the command-line during the actual build process:
```
    VERSION                     The Hercules version string  (e.g. "3.06-svn-5602")
    TARGETFILENAME              Your module name string      (e.g. "{modname}.dll")
    MAX_CPU_ENGINES_STR         Number of CPUs as a string   (e.g. "32")
```
Use them in your `.rc` resource file's `VERSIONINFO` structure as needed.

## The Build Process

[dynmake.bat](../dynmake.bat) first creates a work subdirectory beneath Hercules's main source code directory using the same name as the `{modname}` you passed to it.

It then calls your `prebuild.bat` file if it exists. Use this callback to perform any pre-build adjustments to your source files that may be necessary before the actual build process begins.

When your `prebuild.bat` returns, it then copys all `*.c`, `*.h`, `*.rc`, `*.rc2` and `*.msvc` files from your project directory into its `{modname}` subdirectory and invokes Hercules's primary [makefile.bat](../makefile.bat) script to perform the actual build.

When the build is done it then calls your `postbld.bat` callback if it exists. You can use this callback to copy the resulting binary from Hercules's output directory to your project directory or whatever other post-build processing your product may require.

## More Information

For additional information regarding dynamic modules please see the [Hercules Dynamic Loader](./README.HDL.md) readme document.
