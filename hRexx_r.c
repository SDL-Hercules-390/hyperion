/* HREXX_R.C    (c)Copyright Enrico Sorichetti, 2012                 */
/*              Rexx Interpreter Support ( Regina Rexx )             */
/*                                                                   */
/*  Released under "The Q Public License Version 1"                  */
/*  (http://www.hercules-390.org/herclic.html) as modifications to   */
/*  Hercules.                                                        */

/*  inspired by the previous Rexx implementation by Jan Jaeger       */

#include "hstdinc.h"

#ifndef _HREXX_R_C_
#define _HREXX_R_C_

#if defined(HAVE_REGINA_REXX)

#define HREXXDROPVAR            ReginaRexxDropVar
#define HREXXFETCHVAR           ReginaRexxFetchVar
#define HREXXSETVAR             ReginaRexxSetVar
#define HREXXEXECCMD            ReginaRexxExecCmd
#define HREXXEXECINSTORECMD     ReginaRexxExecInstoreCmd
#define HREXXEXECSUB            ReginaRexxExecSub

#define HREXXREGISTERFUNCTIONS  ReginaRexxRegisterFunctions
#define HREXXEXTERNALFUNCTION_T ULONG APIENTRY

#define HREXXAWSCMD             ReginaRexxawscmd

#define HREXXREGISTERHANDLERS   ReginaRexxRegisterHandlers
#define HREXXPFN                PFN
#define HREXXSTRING             RXSTRING

#define HREXXEXITHANDLER_T      LONG APIENTRY
#define HREXXLONG               LONG

#define HREXXSUBCOMHANDLER_T    APIRET APIENTRY
#define HREXXPSTRING            PRXSTRING

#define _HENGINE_DLL_
#include "hercules.h"

#include "hRexx.h"

#define INCL_REXXSAA
#if defined(HAVE_REGINA_REXXSAA_H)
 #include "regina/rexxsaa.h"
#else
 #include "rexxsaa.h"
#endif

#define RXAPI_MEMFAIL   1002

extern void *hRexxLibHandle;        /* Library handle */
extern void *hRexxApiLibHandle;     /* Api Library handle */

extern char *RexxPackage;

extern char *RexxLibrary;
extern char *RexxApiLibrary;

extern int   MessageLevel;
extern char *MessagePrefix;
extern char *ErrorPrefix;

extern int (*RexxDynamicLoader)();
extern int (*RexxRegisterFunctions)();
extern int (*RexxRegisterHandlers)();
extern int (*RexxExecCmd)();
extern int (*RexxExecInstoreCmd)();
extern int (*RexxExecSub)();

typedef APIRET APIENTRY PFNREXXSTART( LONG, PRXSTRING, PCSZ, PRXSTRING, PCSZ, LONG, PRXSYSEXIT, PSHORT, PRXSTRING );
typedef APIRET APIENTRY PFNREXXREGISTERFUNCTIONEXE( PCSZ, PFN);
typedef APIRET APIENTRY PFNREXXDEREGISTERFUNCTION( PCSZ );
typedef APIRET APIENTRY PFNREXXREGISTERSUBCOMEXE( PCSZ, PFN, PUCHAR);
typedef APIRET APIENTRY PFNREXXDEREGISTERSUBCOM( PCSZ, PCSZ);
typedef APIRET APIENTRY PFNREXXREGISTEREXITEXE( PSZ, PFN , PUCHAR );
typedef APIRET APIENTRY PFNREXXDEREGISTEREXIT( PCSZ, PCSZ);
typedef PVOID  APIENTRY PFNREXXALLOCATEMEMORY(ULONG);
typedef APIRET APIENTRY PFNREXXFREEMEMORY( PVOID);

typedef APIRET APIENTRY PFNREXXVARIABLEPOOL( PSHVBLOCK );

static PFNREXXSTART                 *hRexxStart;
static PFNREXXREGISTERFUNCTIONEXE   *hRexxRegisterFunction;
static PFNREXXDEREGISTERFUNCTION    *hRexxDeregisterFunction;
static PFNREXXREGISTERSUBCOMEXE     *hRexxRegisterSubcom;
static PFNREXXDEREGISTERSUBCOM      *hRexxDeregisterSubcom;
static PFNREXXREGISTEREXITEXE       *hRexxRegisterExit;
static PFNREXXDEREGISTEREXIT        *hRexxDeregisterExit;
static PFNREXXALLOCATEMEMORY        *hRexxAllocateMemory;
static PFNREXXFREEMEMORY            *hRexxFreeMemory;
static PFNREXXVARIABLEPOOL          *hRexxVariablePool;

int ReginaRexxDynamicLoader()
{
    HDLOPEN( hRexxLibHandle, REGINA_LIBRARY, RTLD_LAZY);

    HDLSYM ( hRexxStart, hRexxLibHandle, REXX_START);

    HDLSYM ( hRexxRegisterFunction, hRexxLibHandle, REXX_REGISTER_FUNCTION);

    HDLSYM ( hRexxDeregisterFunction, hRexxLibHandle, REXX_DEREGISTER_FUNCTION);

    HDLSYM ( hRexxRegisterSubcom, hRexxLibHandle, REXX_REGISTER_SUBCOM);

    HDLSYM ( hRexxDeregisterSubcom, hRexxLibHandle, REXX_DEREGISTER_SUBCOM);

    HDLSYM ( hRexxRegisterExit, hRexxLibHandle, REXX_REGISTER_EXIT);

    HDLSYM ( hRexxDeregisterExit, hRexxLibHandle, REXX_DEREGISTER_EXIT);

    HDLSYM ( hRexxAllocateMemory, hRexxLibHandle, REXX_ALLOCATE_MEMORY);

    HDLSYM ( hRexxFreeMemory, hRexxLibHandle, REXX_FREE_MEMORY);

    HDLSYM ( hRexxFreeMemory, hRexxLibHandle, REXX_FREE_MEMORY);

    HDLSYM ( hRexxVariablePool, hRexxLibHandle, REXX_VARIABLE_POOL);

    return 0;
}

#include "hRexxapi.h"

#endif /* defined(HAVE_REGINA_REXX) */
#endif /* #ifndef _HREXX_R_C_  */
