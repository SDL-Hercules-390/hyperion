![test image](images/image_header_herculeshyperionSDL.png)
[Return to master README.md](../README.md)

# Hercules Dynamic Loader

## Contents

1. [About](#About)
2. [Commands](#Commands)
3. [Module load and unload](#Module-load-and-unload)
4. [Resolving Symbols](#Resolving-Symbols)
5. [Some additional notes](#Some-additional-notes)
6. [Important Hercules internals build information relating to HDL](#Important-Hercules-internals-build-information-relating-to-HDL)

## About

The dynamic loader is intended to supply a loading and linking mechanism, whereby routines, commands, instructions and functions can be dynamically added to Hercules, without the need to rebuild or even restart Hercules.

## Commands

The loader can be controlled by the following Hercules commands:  
```
ldmod <module list>    - Load modules named in module list
rmmod <module list>    - Unload modules named in list
lsmod                  - List all modules and entry points
lsdep                  - List all dependencies
```

The ldmod statement may also appear in the Hercules configuration file.  
```
configuration statement:
modpath <pathname>     - Specifies where modules are loaded from
```

## Module load and unload

The loader has two basic functions: module load and module unload.

###  Module load


    int hdl_load(char *name, int flags);


Where name is the module name, this name may include the path.  If no path is given then the module is loaded from the default library search order.  Note that this is different from the standard search order.

Flags may be one of the following:

         HDL_LOAD_DEFAULT or 0  -  Default load
         HDL_LOAD_MAIN          -  Reserved for Hercules use
         HDL_LOAD_NOUNLOAD      -  Module cannot be unloaded
         HDL_LOAD_FORCE         -  Override dependency check
         HDL_LOAD_NOMSG         -  Do not issue any error messages

This function returns a zero value when the load is successful.

### Module unload


    int hdl_dele(char *name);


Where name is the name of the module that is to be unloaded.
This function returns a zero value when the unload is successful.

## Resolving Symbols


    void * HDL_FINDSYM(char *symbolname);


This function will return the entry point of symbolname or zero when the symbol cannot be resolved.


    void * HDL_FINDNXT(current_entry point);
    
    
This function will return the previous entry point.
That is, the entry point which was current before the entry point as identified by current_entry point was registered.

This function is intended to allow a module to call the original routine.  An example of this is given in the `panel_command` entry as listed below.

There are some special considerations for systems that do not support the concept of back-linking.  Back-linking is the operating system support of dynamically resolving unresolved external references in a dynamic module, with the main module, or other loaded modules. Cygwin does not support back-linking and Cygwin specials are listed in this example with `#if defined(WIN32)`.

## Some additional notes

Unload will remove all references to a specific module, but currently it will not actually remove the loaded module from memory.  This is because there is no safe way (yet) to synchronize unloading of code and, besides, it may still be in use.  This should however pose no practical limitations.

When a module lists a new dependency, that dependency will be registered.  Unloading the module does not remove the dependency, this is to be consistent with the previous note about unloading.

```
#include "hercules.h"
#include "devtype.h"
#include "opcode.h"

/*   Local definitions   */

static void *gui_cpu_state(REGS *regs)
{
    void *(*prev_cpu_state)(REGS *);

    /* CPU status update processing */

    /* Call higher level routine if one exists */
    if((prev_cpu_state = HDL_FINDNXT(gui_cpu_state)))
        return prev_cpu_state(regs);

    return NULL;
}


void  *ProcessCommand (char *command)
{
void * (*prev_panel_command)(char *);

    if (strncasecmp(command,"ourcmd",6) == 0)
    {
        logmsg ("This is our command\n");
    }
    else
        /* Call higher level command handler */
        if((prev_panel_command = HDL_FINDNXT(ProcessCommand)))
            return prev_panel_command(command);

    return NULL;
}
```

The dependency section is, for all intents and purposes, called before the module is loaded.  Its purpose is to check that there are no incompatibilities between this module and the version of Hercules that we are running.  Dependencies are identified by name, this name is given on the `HDL_DEPENDENCY` statement.

Each dependency then has a version code, and a size code, where the version code is a character string, and the size code an integer value.  If the version or size codes do not match with those in the Hercules main module, the module cannot be loaded.
The version is usually a character string that identifies the version of the component, and the size is to be the size of the component in the case of structures or unions.

Version and size should be coded as following:

```
#define HDL_VERS_SOMETHING  "1.0"
#define HDL_SIZE_SOMETHING  sizeof(SOMETHING)
```

where "SOMETHING" can be a structure or other component.

The associated dependency statement:
```
HDL_DEPENDENCY(SOMETHING);
````

When a dependency is given that has not yet been registered, it will be registered, such that it can be checked in subsequent module loads.

**The dependency section is mandatory**.

```
HDL_DEPENDENCY_SECTION;
{
     /* Define version dependencies that this module requires */
     HDL_DEPENDENCY ( HERCULES );
     HDL_DEPENDENCY ( SYSBLK   );
     HDL_DEPENDENCY ( REGS     );
     HDL_DEPENDENCY ( DEVBLK   );
}
END_DEPENDENCY_SECTION;
```

The registration exports labels and their associated entry points to Hercules, such that the symbols and associated entry points may    be known to Hercules and any other module that may have been loaded. The registration section is called once during module load.

If we have registered a function that is also called from this DLL, then it must also be listed in the resolver section.  This to ensure that the symbol is properly resolved when other modules are loaded.

The registration section is optional.

```
HDL_REGISTER_SECTION;
{
    /* These are the entry points we export to Hercules
       All functions and labels used this dll must be static
       and non exportable, this to ensure that no foreign
       names are included by the system loader on systems
       that provide back-link support (mostly *nix systems)
    */

    HDL_REGISTER ( daemon_task, external_gui_interface );
    HDL_REGISTER ( debug_cpu_state, gui_cpu_state );
    HDL_REGISTER ( panel_command, ProcessCommand );
}
END_REGISTER_SECTION;
```

The resolver section imports the entry points of symbols that have been previously registered.

When a symbol is requested that has not been previously registered then the resolve function will search the loaded modules for that symbol, and register it implicitly.  This latter function is mainly provided to support systems that do not have back-link support (most notably Cygwin).

Entry points that are resolved should be indirect pointers, for example the panel_command routine is defined as:


       void *(*panel_command)(char *)


The resolver may be called multiple times, the first time it is called is during module load, immediately after the registration section is called.  It is subsequently called when other modules are loaded or unloaded.

When a symbol cannot be resolved it will be set to NULL.

The resolver section is optional.

```
HDL_RESOLVER_SECTION;
{
    /* These are Hercules's entry points that we need access to
       these may be updated by other loadable modules, so we need
       to resolve them here.
    */

    HDL_RESOLVE ( panel_command );
    HDL_RESOLVE ( debug_cpu_state );

    HDL_RESOLVE_PTRVAR ( my_sysblk_ptr, sysblk );
}
END_RESOLVER_SECTION;
```

The device section is to register device drivers with Hercules. It associates device types with device handlers.

If a device handler is not registered for a specific device type then and a loadable mode with the name of "hdtxxxx" exists (where xxxx is the device type), then that module is loaded.

**Search order:**

1. The most recently registered (i.e. loaded) device of the requested device type.
2. Device driver in external loadable module, where the module name is hdtxxxx (where xxxx is the device type i.e. module name `hdtlcs` for device type LCS or `hdt2703` for device type 2703)
3. If the device is listed in the alias table [hdteq.c](../hdteq.c) then external module hdtyyyy will be loaded, where yyyy is the base name as listed in hdteq.c.

The device name is always mapped to lower case when searching for loadable modules.

The device section is optional.
*/
```
HDL_DEVICE_SECTION;
{
    HDL_DEVICE(1052,constty_device_hndinfo);
    HDL_DEVICE(3215,constty_device_hndinfo);
}
END_DEVICE_SECTION;
```

The instruction section registers inserts optional instructions, or modifies existing instructions.

Instructions are generally defined with `DEF_INST(instname)` which results in an external reference of `s370_instname`, `s390_instname` and `z900_instname`. If an instruction is not defined for a certain architecture mode then `UNDEF_INST(instname)` must be used for that given architecture mode.

The instruction section is optional.

```
HDL_INSTRUCTION_SECTION;
{
    HDL_DEF_INST( HDL_INSTARCH_370, 0xB2FE, new_B2FE_inst_doing_something );
    HDL_DEF_INST( HDL_INSTARCH_390 | HDL_INSTARCH_900, 0xB2FD, new_B2FD_inst_doing_something_else );
}
END_INSTRUCTION_SECTION;
```

The final section is called once, when the module is unloaded or when Hercules terminates.

A dll can reject being unloaded by returning a non-zero value in the final section.

The final section is intended to be used to perform cleanup or indicate cleanup action to be taken.  It may set a shutdown flag that is used within this dll that all local functions must now terminate.

The final section is optional.

```
HDL_FINAL_SECTION;
{

}
END_FINAL_SECTION;
```

Below is Fish's sample code...

```
/*   Define version dependencies that this module requires...
**
** The following are the various Hercules structures whose layout your
** module depends on. The layout of the following structures (size and
** version) MUST match the layout that was used to build Hercules with.
** If the size/version of any of the following structures changes (and
** a new version of Hercules is built using the new layout), then YOUR
** module must also be built with the new layout as well. The layout of
** the structures as they were when your module is built MUST MATCH the
** layout as it was when the version of Hercules you're using was built.
** Further note that the below HDL_DEPENDENCY_SECTION is actually just
** a function that the hdl logic calls, and thus allows you to insert
** directly into the below section any specialized 'C' code you need.
*/
HDL_DEPENDENCY_SECTION;
{
     HDL_DEPENDENCY(HERCULES);
     HDL_DEPENDENCY(REGS);
     HDL_DEPENDENCY(DEVBLK);
     HDL_DEPENDENCY(SYSBLK);
     HDL_DEPENDENCY(WEBBLK);
}
END_DEPENDENCY_SECTION;


/*  Register re-bindable entry point with resident version, or UNRESOLVED
**
** The following section defines the entry points within Hercules that
** your module is overriding (replacing). Your module's functions will
** be called by Hercules instead of the normal Hercules function (if any).
** The functions defined below thus provide additional/new functionality
** above/beyond the functionality normally provided by Hercules. Be aware
** however that it is entirely possible for other dlls to subsequently
** override the same functions that you've overridden such that they end
** up being called before your override does and your override may thus
** not get called at all (depending on how their override is written).
** Note that the "entry-point name" does not need to correspond to any
** existing variable or function (i.e. the entry-point name is just that:
** a name, and nothing more. There does not need to be a variable defined
** anywhere in your module with that name). Further note that the below
** HDL_REGISTER_SECTION is actually just a function that the hdl logic
** calls, thus allowing you to insert directly into the below section
** any specialized 'C' code that you may need.
*/
HDL_REGISTER_SECTION;
{
    /*            register this       as the address of
                  entry-point name,   this var or func
    */
    HDL_REGISTER( panel_command,      my_panel_command );
    HDL_REGISTER( panel_display,      my_panel_display );
    HDL_REGISTER( some_exitpoint,     UNRESOLVED       );
}
END_REGISTER_SECTION;


/*   Resolve re-bindable entry point on module load or unload...
**
** The following entries "resolve" entry points that your module
** needs. These entries define the names of registered entry points
** that you need "imported" into your dll so that you may call them
** directly yourself. The HDL_RESOLVE_PTRVAR macro is used to auto-
** matically set one of your own pointer variables to the registered
** entry point's currently registered value (usually an address of
** a function or variable). Note that the HDL_RESOLVER_SECTION is
** actually just a function that the hdl logic calls, thus allowing
** you to insert directly into the below section any specialized 'C'
** code that you may need.
*/
HDL_RESOLVER_SECTION;
{
    /*           Herc's registered
                 entry points that
                 you need to call
                 directly yourself
    */
    HDL_RESOLVE( system_command          );
    HDL_RESOLVE( some_exitpoint          );
    HDL_RESOLVE( debug_cpu_state         );
    HDL_RESOLVE( debug_program_interrupt );
    HDL_RESOLVE( debug_diagnose          );

    /* The following illustrates how to use HDL_RESOLVE_PTRVAR
       macro to retrieve the address of one of Herc's registered
       entry points.

                         Your pointer-   Herc's registered
                         variable name   entry-point name
    */
    HDL_RESOLVE_PTRVAR(  my_sysblk_ptr,  sysblk         );
}
END_RESOLVER_SECTION;


/* The following section defines what should be done just before
** your module is unloaded. It is nothing more than a function that
** is called by hdl logic just before your module is unloaded, and
** nothing more. Thus you can place any 'C' code here that you want.
*/
HDL_FINAL_SECTION;
{
    my_cleanup();
}
END_FINAL_SECTION;



/* DYNCGI.C     (c)Copyright Jan Jaeger, 2002-2003                   */
/*              HTTP cgi-bin routines                                */

/* This file contains cgi routines that may be executed on the      */
/* server (ie under control of a Hercules thread)                    */
/*                                                                   */
/*                                                                   */
/* Dynamically loaded cgi routines must be registered under the      */
/* pathname that they are accessed with (ie /cgi-bin/test)           */
/* All cgi pathnames must start with /cgi-bin/                       */
/*                                                                   */
/*                                                                   */
/* The cgi-bin routines may call the following HTTP service routines */
/*                                                                   */
/* char *cgi_variable(WEBBLK *webblk, char *name);                   */
/*   This call returns a pointer to the cgi variable requested       */
/*   or a NULL pointer if the variable is not found                  */
/*                                                                   */
/* char *cgi_cookie(WEBBLK *webblk, char *name);                     */
/*   This call returns a pointer to the cookie requested             */
/*   or a NULL pointer if the cookie is not found                    */
/*                                                                   */
/* char *cgi_username(WEBBLK *webblk);                               */
/*   Returns the username for which the user has been authenticated  */
/*   or NULL if not authenticated (refer to auth/noauth parameter    */
/*   on the HTTPPORT configuration statement)                        */
/*                                                                   */
/* char *cgi_baseurl(WEBBLK *webblk);                                */
/*   Returns the url as requested by the user                        */
/*                                                                   */
/* void html_header(WEBBLK *webblk);                                 */
/*   Sets up the standard html header, and includes the              */
/*   html/header.htmlpart file.                                      */
/*                                                                   */
/* void html_footer(WEBBLK *webblk);                                 */
/*   Sets up the standard html footer, and includes the              */
/*   html/footer.htmlpart file.                                      */
/*                                                                   */
/* int html_include(WEBBLK *webblk, char *filename);                 */
/*   Includes an html file                                           */
/*                                                                   */
/*                                                                   */
/*                                           Jan Jaeger - 28/03/2002 */

#include "hstdinc.h"
#include "hercules.h"
#include "devtype.h"
#include "opcode.h"
#include "httpmisc.h"

#if defined(OPTION_HTTP_SERVER)

void cgibin_test(WEBBLK *webblk)
{
    html_header(webblk);
    hprintf(webblk->hsock, "<H2>Sample cgi routine</H2>\n");
    html_footer(webblk);
}


HDL_DEPENDENCY_SECTION;
{
     HDL_DEPENDENCY(HERCULES);
//   HDL_DEPENDENCY(REGS);
//   HDL_DEPENDENCY(DEVBLK);
//   HDL_DEPENDENCY(SYSBLK);
     HDL_DEPENDENCY(WEBBLK);
}
END_DEPENDENCY_SECTION;


HDL_REGISTER_SECTION;
{
    HDL_REGISTER( /cgi-bin/test, cgibin_test );
}
END_REGISTER_SECTION;


HDL_RESOLVER_SECTION;
{
}
END_RESOLVER_SECTION;


HDL_FINAL_SECTION;
{
}
END_FINAL_SECTION;

#endif /*defined(OPTION_HTTP_SERVER)*/



/* TESTINS.C    Test instruction                                     */

#include "hercules.h"

#include "opcode.h"

/*-------------------------------------------------------------------*/
/* 0000 BARF  - Barf                                            [RR] */
/*-------------------------------------------------------------------*/
DEF_INST(barf)
{
int r1, r2;                                     /* register values   */

    RR(inst, regs, r1, r2)

    logmsg("Barf\n");

    ARCH_DEP(program_interrupt)(regs, PGM_OPERATION_EXCEPTION);
}


#if !defined(_GEN_ARCH)

#if defined(_ARCH_NUM_1)
 #define  _GEN_ARCH _ARCH_NUM_1
 #include "testins.c"
#endif

#if defined(_ARCH_NUM_2)
 #undef   _GEN_ARCH
 #define  _GEN_ARCH _ARCH_NUM_2
 #include "testins.c"
#endif


HDL_DEPENDENCY_SECTION;
{

} END_DEPENDENCY_SECTION;


HDL_INSTRUCTION_SECTION;
{
    HDL_DEF_INST(HDL_INSTARCH_ALL,0x00,barf);

} END_INSTRUCTION_SECTION;

#endif /*!defined(_GEN_ARCH)*/
```

### IMPORTANT HERCULES INTERNALS BUILD INFORMATION RELATING TO HDL

(our `DLL_EXPORT` and `DLL_IMPORT` design)

Here's the poop. Any function that needs to be exported/imported to another MODULE (i.e. 'module' is defined as .DLL or .SO, etc), _**MUST**_ have its functon declaration defined in the [hexterns.h](../hexterns.h) header and _ONLY_ in the [hexterns.h](../hexterns.h) header!

That is to say, you _must **NOT**_ declare the function in a separate header file! _(That might be the way you normally do things for a normal project, but that is NOT the way you do it with Hercules)_


#### _STEP 1:_

You need to ensure your .c _source_ member always begins with the following very specific header file #include sequence:

 ```
    /*  XXXXXX.C    (C) Copyright XXXXXXXXXXX & Others, yyyy-2011        */
    /*              Module description goes here...                      */
    /*                                                                   */
    /*   Released under "The Q Public License Version 1"                 */
    /*   (http://www.hercules-390.org/herclic.html) as modifications     */
    /*   to Hercules.                                                    */


    #include "hstdinc.h"

    #define _XXXXXXX_C_
    #define _ZZZZZZZ_DLL_

    #include "hercules.h"
    ...(other #includes go here)...


    DLL_EXPORT  int myfunction1 (DEVBLK*, int myarg, int otherarg)
    {
      ... function body ...
    }


    DLL_EXPORT  int myfunction2 (DEVBLK*, int myarg, int otherarg)
    {
      ... function body ...
    }


    DLL_EXPORT  int myfunction3 (DEVBLK*, int myarg, int otherarg)
    {
      ... function body ...
    }
```

where 'XXXXX' is the name of your source member, 'ZZZZZ' is the name of the MODULE (.dll or .so) that your code will be a part of, and `DLL_EXPORT` is added to the beginning of each function that needs to be exported.

Refer to the `OBJ_CODE.msvc` and/or [Makefile.am](../Makefile.am) members to see how all of Hercules code is divided into separate loadable modules (DLLs) so you know which module (DLL) your function should be a part of.


#### _STEP 2:_

Add a new entry at the _beginning_ of [hexterns.h](../hexterns.h) as follows:

```
#ifndef _XXXXXXX_C_
#ifndef _ZZZZZZZ_DLL_
#define MMMM_DLL_IMPORT  DLL_IMPORT
#else
#define MMMM_DLL_IMPORT  extern
#endif
#else
#define MMMM_DLL_IMPORT  DLL_EXPORT
#endif
```

where 'MMMM' is a unique 2-4 character prefix of your own choosing that identifies your source member export (e.g. `HUTL_DLL_IMPORT`).


#### _STEP 3:_

Add your exported function declarations to _end_ of [hexterns.h](../hexterns.h):

```
    /* Functions in module xxxxxxx.c */
    MMMM_DLL_IMPORT int myfunction1 (DEVBLK*, int myarg, int otherarg);
    MMMM_DLL_IMPORT int myfunction2 (DEVBLK*, int myarg, int otherarg);
    MMMM_DLL_IMPORT int myfunction3 (DEVBLK*, int myarg, int otherarg);
    ...etc...
```


#### _STEP 4:_

Update _both_ the `OBJ_CODE.msvc` and [Makefile.am](../Makefile.am) members with your new source member. Be sure to update _BOTH_ files, e.g.:

```
    ---(Makefile.am)---


      libhercu_la_SOURCES = version.c    \
                            hscutl.c     \
                            codepage.c   \
                            logger.c     \
                            logmsg.c     \
                            hdl.c        \
                            hostinfo.c   \
                            hsocket.c    \
                            memrchr.c    \
                            parser.c     \
                            pttrace.c    \
                            xxxxxxxx.c   \
                            $(FTHREADS)  \
                            $(LTDL)


    ---(OBJ_CODE.msvc)---


        hutil_OBJ = \
            $(O)codepage.obj \
            $(O)fthreads.obj \
            $(O)getopt.obj   \
            $(O)hdl.obj      \
            $(O)hostinfo.obj \
            $(O)hscutl.obj   \
            $(O)logger.obj   \
            $(O)logmsg.obj   \
            $(O)memrchr.obj  \
            $(O)parser.obj   \
            $(O)pttrace.obj  \
            $(O)version.obj  \
            $(O)hsocket.obj  \
            $(O)w32util.obj  \
            $(O)xxxxxxxx.obj
```

That's it. That's all you should have to do.

Again, you _may_ use a separate #include header file for your new source member with _NON-EXPORTED_ functions declared within it, but any function you need to export to another module _must **not**_ be declared there. The function must instead be declared in [hexterns.h](../hexterns.h) as described above.
