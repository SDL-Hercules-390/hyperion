/* HREXXAPI.C   (C) Copyright Enrico Sorichetti, 2012                */
/*              (C) Copyright "Fish" (David B. Trout), 2017          */
/*              Rexx Interpreter Support                             */
/*                                                                   */
/*  Released under "The Q Public License Version 1"                  */
/*  (http://www.hercules-390.org/herclic.html) as modifications to   */
/*  Hercules.                                                        */

/*  inspired by the previous Rexx implementation by Jan Jaeger       */

/*-------------------------------------------------------------------*/
/*                      PROGRAMMING NOTE                             */
/*-------------------------------------------------------------------*/
/*          THIS SOURCE MODULE IS NOT DIRECTLY COMPILED              */
/*-------------------------------------------------------------------*/
/* This source module should be considered an integral part of both  */
/* the "hRexx_o.c" and "hRexx_r.c" source modules. Both "hRrexx_o.c" */
/* and "hRexx_r.c" #include this source file at the very end of each */
/* of their respective source files. By using #defines each of them  */
/* ensures each of the below support functions is assigned a unique  */
/* function name. Our primary Rexx support module ("hRexx.c") uses   */
/* generic function pointers to choose at run time which package's   */
/* functions will actually be called: OORexx's or Regina's.          */
/*-------------------------------------------------------------------*/

#ifdef REXX_PKG                 // Support for Rexx package
                                // Note: REXX_PKG is #defined ONLY by
                                // source files hRexx_o.c and hRexx_r.c
                                // See PROGRAMMING NOTE above.

/*-------------------------------------------------------------------*/
/* Constant (unmodifiable) RXSTRING type                             */
/*-------------------------------------------------------------------*/
typedef const RXSTRING          CRXSTRING, *PCRXSTRING;

/*-------------------------------------------------------------------*/
/* Better replacement for default rexx.h "MAKERXSTRING" macro        */
/*-------------------------------------------------------------------*/
/* Object Rexx's "rexx.h" header defines MAKERXSTRING as multiple    */
/* statements enclosed within {} braces, making its use within an    */
/* if/else construct problematic.  Regina Rexx's definition of the   */
/* MAKERXSTRING macro doesn't have this problem.  The below replace- */
/* ment macro is the workaround.  r:RXSTRING, p:strptr, l:strlength  */
/*-------------------------------------------------------------------*/
#define HMAKE_RXSTRING(r,p,l)   do { MAKERXSTRING(r,p,l); } while (0)

/*-------------------------------------------------------------------*/
/* Private global variables                                          */
/*-------------------------------------------------------------------*/
static char*  REXX_DEP( PackageName      )  = QSTR( REXX_PKG );
static char*  REXX_DEP( PackageVersion   )  = NULL;
static char*  REXX_DEP( PackageSource    )  = NULL;
static char   REXX_DEP( PackageMajorVers )  = '0';

static char*  LibName       = REXX_LIBNAME;
static char*  ApiLibName    = REXX_APILIBNAME;
static void*  LibHandle     = NULL;
static void*  ApiLibHandle  = NULL;

/*-------------------------------------------------------------------*/
/* Library function names                                            */
/*-------------------------------------------------------------------*/
#define REXX_START                  "RexxStart"
#define REXX_REGISTER_FUNCTION      "RexxRegisterFunctionExe"
#define REXX_DEREGISTER_FUNCTION    "RexxDeregisterFunction"
#define REXX_REGISTER_SUBCOM        "RexxRegisterSubcomExe"
#define REXX_DEREGISTER_SUBCOM      "RexxDeregisterSubcom"
#define REXX_REGISTER_EXIT          "RexxRegisterExitExe"
#define REXX_DEREGISTER_EXIT        "RexxDeregisterExit"
#define REXX_ALLOCATE_MEMORY        "RexxAllocateMemory"
#define REXX_FREE_MEMORY            "RexxFreeMemory"
#define REXX_VARIABLE_POOL          "RexxVariablePool"
#define REXX_SETHALT                "RexxSetHalt"

/*-------------------------------------------------------------------*/
/* Generic Rexx Library function typedefs                            */
/*-------------------------------------------------------------------*/
typedef HR_REXXRC_T (HR_ENTRY *PFNHRSTART)               ( HR_ARGC_T, HR_ARGV_T, HR_PCSZ_T, PRXSTRING, HR_PCSZ_T, HR_CALLTYPE_T, PRXSYSEXIT, short*, PRXSTRING );
typedef HR_REXXRC_T (HR_ENTRY *PFNHRREGISTERFUNCTIONEXE) ( HR_PCSZ_T, HR_PFN_T );
typedef HR_REXXRC_T (HR_ENTRY *PFNHRDEREGISTERFUNCTION)  ( HR_PCSZ_T );
typedef HR_REXXRC_T (HR_ENTRY *PFNHRREGISTERSUBCOMEXE)   ( HR_PCSZ_T, HR_PFN_T, HR_USERINFO_T );
typedef HR_REXXRC_T (HR_ENTRY *PFNHRDEREGISTERSUBCOM)    ( HR_PCSZ_T, HR_PCSZ_T );
typedef HR_REXXRC_T (HR_ENTRY *PFNHRREGISTEREXITEXE)     ( HR_PCSZ_T, HR_PFN_T, HR_USERINFO_T );
typedef void*       (HR_ENTRY *PFNHRALLOCATEMEMORY)      ( HR_MEMSIZE_T );
typedef HR_REXXRC_T (HR_ENTRY *PFNHRFREEMEMORY)          ( void* );
typedef HR_REXXRC_T (HR_ENTRY *PFNHRVARIABLEPOOL)        ( PSHVBLOCK );
typedef HR_REXXRC_T (HR_ENTRY *PFNHRREXXSETHALT)         ( HR_PROCESS_ID_T, HR_THREAD_ID_T );

/*-------------------------------------------------------------------*/
/* Generic Rexx Library function pointers                            */
/*-------------------------------------------------------------------*/
static PFNHRSTART                 REXX_DEP( RexxStart        ) = NULL;
static PFNHRREGISTERFUNCTIONEXE   REXX_DEP( RegisterFunction ) = NULL;
static PFNHRREGISTERSUBCOMEXE     REXX_DEP( RegisterSubcom   ) = NULL;
static PFNHRREGISTEREXITEXE       REXX_DEP( RegisterExit     ) = NULL;
static PFNHRALLOCATEMEMORY        REXX_DEP( AllocateMemory   ) = NULL;
static PFNHRFREEMEMORY            REXX_DEP( FreeMemory       ) = NULL;
static PFNHRVARIABLEPOOL          REXX_DEP( VariablePool     ) = NULL;
static PFNHRREXXSETHALT           REXX_DEP( SetHalt          ) = NULL;

/*-------------------------------------------------------------------*/
/* Generic Rexx registered handler/function signatures               */
/*-------------------------------------------------------------------*/
#define HR_EXITHAND_T             HR_EXITHAND_RC_T  HR_ENTRY
#define HR_SUBCOMHAND_T           HR_SUBCOM_RC_T    HR_ENTRY
#define HR_EXTFUNC_T              HR_EXTFUNC_RC_T   HR_ENTRY

/*-------------------------------------------------------------------*/
/*  Forward references                                               */
/*-------------------------------------------------------------------*/

static HR_EXITHAND_T    ExitHandler
(
    HR_FCODE_T  ExitNumber,     // Code defining the exit function
    HR_FCODE_T  SubFunction,    // Code defining the exit subfunction
    PEXIT       ParmBlock       // Function-dependent control block
);

static HR_SUBCOMHAND_T  SubComHandler
(
    PRXSTRING        Command,   // Command string from Rexx
    unsigned short*  Flags,     // Returned Error/Failure Flags
    PRXSTRING        RetValue   // Returned RC string
);

static HR_EXTFUNC_T     AWSCmd
(
    HR_PCSZ_T   Name,           // Name of the function
    HR_ARGC_T   argc,           // Number of arguments
    HR_ARGV_T   argv,           // List of argument strings
    HR_PCSZ_T   Queuename,      // Current queue name
    PRXSTRING   RetValue        // Returned result string
);

    static BYTE RegisterRexxHandlersAndFunctions();

/*-------------------------------------------------------------------*/
/* Helper function to 'sprintf' to an RXSTRING                       */
/*-------------------------------------------------------------------*/
static int rx_sprintf( PRXSTRING prx, const char* fmt, ... )
{
    HR_MEMSIZE_T  len;
    va_list       vargs;

    static const HR_MEMSIZE_T maxlen = (64 * 1024);     // (64K)

    len = 0;
    va_start( vargs, fmt );

    if (RXVALIDSTRING( *prx ))
        len = vsnprintf( prx->strptr, prx->strlength, fmt, vargs );

    /* PROGRAMMING NOTE: we use '>=' comparison to ensure there
       will always be room for a terminating NULL, even though
       RXSTRING strings are never necessarily null terminated.
    */
    if (!RXVALIDSTRING( *prx ) || len >= prx->strlength)
    {
        do
        {
            if (RXVALIDSTRING( *prx ))
                REXX_DEP( FreeMemory )( prx->strptr );

            if (!(prx->strptr = REXX_DEP( AllocateMemory )( prx->strlength += DEF_RXSTRING_BUFSZ )))
            {
                // "REXX(%s) %s: return code 0x%8.8"PRIX32" from %s"
                WRMSG( HHC17512, "E", REXX_DEP( PackageName ),
                    "rx_sprintf", (U32) RXSHV_MEMFL,
                    QSTR( REXX_DEP( AllocateMemory )));
                len = 0; // (forced since strptr is now NULL)
                break;
            }

            len = vsnprintf( prx->strptr, prx->strlength, fmt, vargs );
        }
        while (len >= prx->strlength && prx->strlength < maxlen);

        if (len > maxlen)   // (if we gave up due to max exceeded)
            len = maxlen;   // (then set final len to the maximum)
    }

    va_end( vargs );
    prx->strlength = len;
    return len;
}

/*-------------------------------------------------------------------*/
/* Helper function to 'strdup' an RXSTRING                           */
/*-------------------------------------------------------------------*/
static char* rx_strdup( PCRXSTRING prx )
{
    /* PROGRAMMING NOTE: this function exists because Rexx strings
       are NOT necessarily NULL terminated. Thus a simple strdup()
       should NEVER be used on a Rexx RXSTRING string.
    */
    char* dup;

    /* Allocate room for copy of str + null terminator */
    if (0
        || !prx
        || !RXVALIDSTRING( *prx )
        || !(dup = malloc( prx->strlength + 1 ))
    )
        return NULL;

    /* Copy string to buffer and null terminate */
    strlcpy( dup, prx->strptr, prx->strlength + 1 );

    return dup;
}

/*-------------------------------------------------------------------*/
/* Call Rexx VariablePool function                                   */
/*-------------------------------------------------------------------*/
static HR_REXXRC_T  VariablePool( SHVBLOCK* pShvBlock )
{
    /* PROGRAMMING NOTE: calling Rexx's VariablePool function is
       made a separate function unto itself to make it easier to
       debug every call made to VariablePool without having to set
       many separate breakpoints all over the place.
    */
    HR_REXXRC_T  rc;
    rc = REXX_DEP( VariablePool )( pShvBlock );
    return rc;
}

/*-------------------------------------------------------------------*/
/* Create or update a Rexx variable                                  */
/*-------------------------------------------------------------------*/
static HR_REXXRC_T  SetVar
(
    char*  Name,                // The name of Rexx variable
    char*  Value                // The value to assign to it
)
{
    SHVBLOCK     RxVarBlock;    // Shared Variable Request Block
    HR_REXXRC_T  RexxRC;        // Rexx API return code

    RxVarBlock.shvcode  =  RXSHV_SYSET;     // SET
    RxVarBlock.shvret   =  RXSHV_OK;
    RxVarBlock.shvnext  =  NULL;

    HMAKE_RXSTRING( RxVarBlock.shvname,  Name,  strlen( Name  ));
    HMAKE_RXSTRING( RxVarBlock.shvvalue, Value, strlen( Value ));

    RxVarBlock.shvnamelen  = RxVarBlock.shvname.strlength;
    RxVarBlock.shvvaluelen = RxVarBlock.shvvalue.strlength;

    /* Create or update the variable */
    RexxRC = VariablePool( &RxVarBlock );

    /* If the variable we wanted to update didn't exist before our
       call (but was successfully created as a result of our call),
       then it's not really an error. Our objective was achieved.
    */
    if (RexxRC == RXSHV_NEWV)   // (if variable was newly created)
        RexxRC =  RXSHV_OK;     // (then we condider that success)

    /* Check for error */
    if (RexxRC != RXSHV_OK && MLVL( DEBUG ))
    {
        // "REXX(%s) %s: return code 0x%8.8"PRIX32" from %s"
        WRMSG( HHC17512, "D", REXX_DEP( PackageName ),
            "SetVar", (U32) RexxRC,
            QSTR( REXX_DEP( VariablePool )));
    }

    return RexxRC;
}

/*-------------------------------------------------------------------*/
/* Delete a Rexx variable                                            */
/*-------------------------------------------------------------------*/
static HR_REXXRC_T  DropVar
(
    char*  Name                 // Name of Rexx variable to drop
)
{
    SHVBLOCK     RxVarBlock;    // Shared Variable Request Block
    HR_REXXRC_T  RexxRC;        // Rexx API return code

    RxVarBlock.shvcode  =  RXSHV_SYDRO;     // DROP
    RxVarBlock.shvret   =  RXSHV_OK;
    RxVarBlock.shvnext  =  NULL;

    HMAKE_RXSTRING( RxVarBlock.shvname, Name, strlen( Name ));

    RxVarBlock.shvnamelen = RxVarBlock.shvname.strlength;

    /* Drop the variable */
    RexxRC = VariablePool( &RxVarBlock );

    /* If the variable we wanted to delete didn't (doesn't) exist,
       then it's not really an error. Our objective was achieved.
    */
    if (RexxRC == RXSHV_BADN)   // (if variable didn't exist before)
        RexxRC =  RXSHV_OK;     // (then our objective was achieved)

    /* Check for error */
    if (RexxRC != RXSHV_OK && MLVL( DEBUG ))
    {
        // "REXX(%s) %s: return code 0x%8.8"PRIX32" from %s"
        WRMSG( HHC17512, "D", REXX_DEP( PackageName ),
            "DropVar", (U32) RexxRC,
            QSTR( REXX_DEP( VariablePool )));
    }

    return RexxRC;
}

/*-------------------------------------------------------------------*/
/* Retrieve a Rexx variable's value                                  */
/*-------------------------------------------------------------------*/
static HR_REXXRC_T  FetchVar
(
    char*   Name,           // Name of Rexx variable to be retrieved
    char**  Value           // strdup()'ed copy of variable's value
)
{
    SHVBLOCK     RxVarBlock;   // Shared Variable Request Block
    HR_REXXRC_T  RexxRC;       // Rexx API return code

    RxVarBlock.shvcode  =  RXSHV_SYFET;     // FETCH
    RxVarBlock.shvret   =  RXSHV_OK;
    RxVarBlock.shvnext  =  NULL;

    /* Set 'shvvalue' to RXNULLSTRING so Rexx does allocation */

    HMAKE_RXSTRING( RxVarBlock.shvname,  Name, strlen( Name ));
    HMAKE_RXSTRING( RxVarBlock.shvvalue, NULL,    0          );

    RxVarBlock.shvnamelen  = RxVarBlock.shvname.strlength;
    RxVarBlock.shvvaluelen = RxVarBlock.shvvalue.strlength;

    /* Retrieve the variable */
    RexxRC = VariablePool( &RxVarBlock );

    /* Check for error */
    if (RexxRC != RXSHV_OK)
    {
        if (MLVL( DEBUG ))
        {
            // "REXX(%s) %s: return code 0x%8.8"PRIX32" from %s"
            WRMSG( HHC17512, "D", REXX_DEP( PackageName ),
                "FetchVar", (U32) RexxRC,
                QSTR( REXX_DEP( VariablePool )));
        }
        *Value = NULL;
        return RexxRC;
    }

    /* Return a copy of the variable's value to the caller.
       Caller is responsible for eventually calling free().
       If the variable isn't found RxVarBlock.shvvalue will
       simply be a RXNULLSTRING (RxVarBlock.shvvalue.strptr
       NULL and RxVarBlock.shvvalue.strlength=0).  In such
       situations rx_strdup() function simply returns NULL.
    */
    *Value = rx_strdup( &RxVarBlock.shvvalue );

    /* Rexx string no longer needed. Free it to prevent leak. */
    if (RXVALIDSTRING( RxVarBlock.shvvalue ))
        REXX_DEP( FreeMemory )( RxVarBlock.shvvalue.strptr );

    return RexxRC;
}

/*-------------------------------------------------------------------*/
/* Exit Handler Parameter Block                                      */
/*-------------------------------------------------------------------*/
struct HR_PARMBLK
{
    union
    {
        RXSIOSAY_PARM  say;         // Say
        RXSIOTRC_PARM  trc;         // Trace
        RXSIOTRD_PARM  trd;         // Terminal Read
        RXSIODTR_PARM  dtr;         // Debug Terminal Read
    } u;
};
typedef  struct HR_PARMBLK  HR_PARMBLK;

#define  u_say       u.say.rxsio_string
#define  u_trc       u.trc.rxsio_string
#define  u_trd       u.trd.rxsiotrd_retc
#define  u_dtr       u.dtr.rxsiodtr_retc

/*-------------------------------------------------------------------*/
/* Exit handler                                                      */
/*-------------------------------------------------------------------*/
static HR_EXITHAND_T  ExitHandler
(
    HR_FCODE_T  ExitNumber,     // code defining the exit function
    HR_FCODE_T  SubFunction,    // code defining the exit SubFunction
    PEXIT       ParmBlock       // function-dependent control block
)
{
    HR_EXITHAND_RC_T    rc;
    HR_PARMBLK*         pb     = (HR_PARMBLK*) ParmBlock;

    // Session I/O (RXSIO) is the only thing our exit handler handles

    if (ExitNumber != RXSIO)
        return RXEXIT_NOT_HANDLED;

    rc  = RXEXIT_HANDLED;   // think positively

    switch (SubFunction)
    {
        case RXSIOSAY:      // Perform SAY Clause
        {
            if (MsgPrefix) WRMSG( HHC17540, "I", pb->u_say.strptr );
            else           LOGMSG( "%s\n",       pb->u_say.strptr );
        }
        break;

        case RXSIOTRC:      // Write Trace Output
        {
            if (ErrPrefix) WRMSG( HHC17541, "D", pb->u_trc.strptr );
            else           LOGMSG( "%s\n",       pb->u_trc.strptr );
        }
        break;

        case RXSIOTRD:      // Read Input from the Terminal
        {
            HMAKE_RXSTRING( pb->u_trd, NULL, 0 );
        }
        break;

        case RXSIODTR:      // Read Debug Input from the Terminal
        {
            HMAKE_RXSTRING( pb->u_dtr, NULL, 0 );
        }
        break;

        default:
        {
            rc = RXEXIT_NOT_HANDLED;
        }
        break;
    }

    return rc;
}

/*-------------------------------------------------------------------*/
/* Convert Variable Pool return code to Hercules Rexx error code     */
/*-------------------------------------------------------------------*/
static HR_ERR_T  Shv2Err( void* shvrc )     // void* so it's generic
{
    switch ((size_t)shvrc)          // (must be integral value)
    {
        case RXSHV_OK:              // Execution was OK
        case RXSHV_NEWV:            // Variable was created
        case RXSHV_LVAR:            // Last var trans via SHVNEXTV
        {
            return HRERR_OK;
        }

        case RXSHV_MEMFL:           // Out of memory failure
        {
            return HRERR_NOMEM;
        }

        case RXSHV_TRUNC:           // Truncation occurred (Fetch)
        case RXSHV_BADN:            // Invalid variable name
        case RXSHV_BADF:            // Invalid func code (shvcode)
        {
            return HRERR_ERROR;
        }

        case RXSHV_NOAVL:           // Interface is not available
        default:
        {
            break;                  // (catch-all)
        }
    }
    return HRERR_FAILURE;           // return 'FAILURE'
}
#define SHV2ERR( rc )               Shv2Err( INT2VOIDP( rc ))

/*-------------------------------------------------------------------*/
/* Convert Hercules Rexx error code to SubComm/ExtFunc return code   */
/*-------------------------------------------------------------------*/
static HR_SUBCOM_RC_T    Err2SubComm( HR_ERR_T         err,
                                      unsigned short*  Flags )
{
    HR_SUBCOM_RC_T  rc;

    switch (err)
    {
        case HRERR_OK:      rc = RXSUBCOM_OK;       break;
        case HRERR_BADARGS: rc = RXSUBCOM_BADENTRY; break;
        case HRERR_NOMEM:   rc = RXSUBCOM_NOEMEM;   break;
        case HRERR_ERROR:   rc = RXSUBCOM_ERROR;    break;
        case HRERR_FAILURE: rc = RXSUBCOM_FAILURE;  break;
        default:            rc = RXSUBCOM_FAILURE;  break;
    }

    *Flags = rc;        // (update Rexx Flags)
    return rc;
}
/*-------------------------------------------------------------------*/
static HR_EXTFUNC_RC_T   Err2ExtFunc( HR_ERR_T  err )
{
    HR_EXTFUNC_RC_T  rc;

    switch (err)
    {
        case HRERR_OK:      rc = RXFUNC_OK;       break;
        case HRERR_BADARGS: rc = RXFUNC_BADENTRY; break;
        case HRERR_NOMEM:   rc = RXFUNC_NOEMEM;   break;
        case HRERR_ERROR:   rc = RXFUNC_ERROR;    break;
        case HRERR_FAILURE: rc = RXFUNC_FAILURE;  break;
        default:            rc = RXFUNC_FAILURE;  break;
    }

    return rc;
}

/*-------------------------------------------------------------------*/
/* Format a SubComm/ExtFunc return value                             */
/*-------------------------------------------------------------------*/
static void FmtSubCommRV
(
    HR_SUBCOM_RC_T   rc,            // SubComm return code
    int              panelrc,       // panel_command return code
    PRXSTRING        RetValue       // Rexx Return Value
)
{
    if (RXSUBCOM_OK == rc)
        rx_sprintf( RetValue, "%d", panelrc );
    else

#if REXX_PKGNUM == OOREXX_PKGNUM

    rx_sprintf( RetValue, "%d", rc );       // (RexxReturnCode = int)

#else // REXX_PKGNUM == REGINA_PKGNUM

    rx_sprintf( RetValue, "%u", rc );       // (APIRET = ULONG)

#endif
}
/*-------------------------------------------------------------------*/
static void FmtExtFuncRV
(
    HR_EXTFUNC_RC_T  rc,            // ExtFunc return code
    int              panelrc,       // panel_command return code
    PRXSTRING        RetValue       // Rexx Return Value
)
{
    if (RXFUNC_OK == rc)
        rx_sprintf( RetValue, "%d", panelrc );
    else

#if REXX_PKGNUM == OOREXX_PKGNUM

    rx_sprintf( RetValue, "%"PRIu64, (U64) rc );   // (size_t)

#else // REXX_PKGNUM == REGINA_PKGNUM

    rx_sprintf( RetValue, "%u", rc );              // (APIRET = ULONG)

#endif
}

/*-------------------------------------------------------------------*/
/* Return from SubComm/ExtFunc                                       */
/*-------------------------------------------------------------------*/
static HR_SUBCOM_RC_T   SubCommRC
(
    HR_ERR_T         err,
    int              panelrc,
    PRXSTRING        RetValue,
    unsigned short*  Flags
)
{
    HR_SUBCOM_RC_T  rc   = Err2SubComm( err, Flags );
    FmtSubCommRV(   rc,  panelrc,  RetValue );
    return          rc;
}
/*-------------------------------------------------------------------*/
static HR_EXTFUNC_T   ExtFuncRC
(
    HR_ERR_T         err,
    int              panelrc,
    PRXSTRING        RetValue
)
{
    HR_EXTFUNC_RC_T  rc   = Err2ExtFunc( err );
    FmtExtFuncRV(    rc,  panelrc,  RetValue );
    return           rc;
}

/*-------------------------------------------------------------------*/
/* Issue Hercules Command and save response in Rexx stem variable.   */
/*-------------------------------------------------------------------*/
static HR_ERR_T   HerculesCommand
(
    char*  herccmd,         // Hercules command to be issued
    char*  stemname,        // Response stem variable name
    int*   panelrc          // panel_command return code
)
{
    HR_ERR_T  err;          // Hercules Rexx error code
    char*     resp;         // Response from panel_command
    bool      quiet;        // don't show response on console if true

    /* Validate arguments and trim Hercules command line */
    if (!herccmd || !*TRIM( herccmd ) || !panelrc)
        return HRERR_BADARGS;

    /* if response is wanted, ask Hercules to be quiet on the console */
    if (stemname) quiet = true;
    else          quiet = false;

    /* Issue the Hercules command and capture the response */
    *panelrc = panel_command_capture( herccmd, &resp, quiet );

    /* Format response string stem values if response is wanted */
    if (stemname)
    {
        char*   line;                 // single line of response
        char*   vname;                // stem variable name buffer
        size_t  vnamesize, stemlen;   // stem buffer size and strlen

        int     n;                          // (counts #of stems)
        char    buf[ NUM_DIGITS_32 + 1 ];   // (prints #of stems)

        /* Remove any trailing dot/period from stem name */
        if (!(stemlen = strlen( rtrim( stemname, "." ))))
        {
            free( resp );
            return HRERR_BADARGS;
        }

        /* Allocate buffer for constructing stem variable names.
           +1 is for dot/period, another +1 for null terminator.
        */
        if (!(vname = malloc( vnamesize = stemlen + 1 + NUM_DIGITS_32 + 1 )))
        {
            free( resp );
            return HRERR_NOMEM;     // (out of memory)
        }

        /* Parse response lines and assign to Rexx stem variable */
        if (resp)
        {
            for (n=1, line = strtok( resp, "\n" ); line;
                 n++, line = strtok( NULL, "\n" ))
            {
                snprintf( vname, vnamesize, "%s.%d", stemname, n );

                if (HRERR_OK != (err = SHV2ERR( SetVar( vname, line ))))
                {
                    free( resp );
                    free( vname );
                    return err;
                }
            }
        }
        else
            n = 0;  // (no response == no stems)

        /* Tell Rexx how many stem values there are (stemname.0) */
        snprintf( vname, vnamesize, "%s.%d", stemname, 0 );
        MSGBUF( buf, "%d", n ); // (number of stems)

        if (HRERR_OK != (err = SHV2ERR( SetVar( vname, buf ))))
        {
            free( resp );
            free( vname );
            return err;
        }

        /* Free stem variable name buffer */
        free( vname );
    }

    /* Free panel_command response strings */
    free( resp );

    /* Return success */
    return HRERR_OK;
}

/*-------------------------------------------------------------------*/
/* The core of the whole shebang: the SubCommand handler             */
/*-------------------------------------------------------------------*/
static HR_SUBCOMHAND_T  SubComHandler
(
    PRXSTRING        Command,       // Command string from Rexx
    unsigned short*  Flags,         // Rexx Error/Failure Flags
    PRXSTRING        RetValue       // Returned results string
)
{
    HR_SUBCOM_RC_T   rc         = RXSUBCOM_OK;
    HR_ERR_T         err        = HRERR_OK;
    char*            stemname   = NULL;
    char*            herccmd    = NULL;
    int              panelrc    = 0;
    BYTE             RetUsr     = TRUE;

    /* Verify a Hercules command was passed to us */
    if (!RXVALIDSTRING( *Command ))
    {
        err = HRERR_BADARGS;
        goto quick_exit;
    }

    /* Make a modifiable copy of the Hercules command to be executed */
    if (!(herccmd = rx_strdup( Command )))
    {
        err = HRERR_NOMEM;
        goto quick_exit;
    }

    /* Examine the value of our errorhandler variable to see if
       the user wants to handle any errors themselves or whether
       they prefer that the system handle errors automatically.
    */
    {
        char* erropt;
        if (HRERR_OK != (err = SHV2ERR( FetchVar( HR_ERRHNDLR_VNAME, &erropt ))))
            goto quick_exit;
        if (erropt)
        {
            if (strcasecmp( erropt, "system" ) == 0)
                RetUsr = FALSE;
            free( erropt );
        }
    }

    /* Retrieve what stem name they want to use */
    if (HRERR_OK != (err = SHV2ERR( FetchVar( HR_RESPSTEM_VNAME, &stemname ))))
        goto quick_exit;

    if (stemname)
    {
        /* Not persistent so drop it */
        if (HRERR_OK != (err = SHV2ERR( DropVar( HR_RESPSTEM_VNAME ))))
            goto quick_exit;
    }
    else // HR_RESPSTEM_VNAME not found. Try HR_PERSISTRESPSTEM_VNAME.
    {
        if (HRERR_OK != (err = SHV2ERR( FetchVar( HR_PERSISTRESPSTEM_VNAME, &stemname ))))
            goto quick_exit;
    }

    /* Issue the Hercules command and save the results */
    err = HerculesCommand( herccmd, stemname, &panelrc );

quick_exit:

    /* Free acquired storage */
    free( herccmd );
    free( stemname );

    /* Return from SubCommand */
    rc = SubCommRC( err, panelrc, RetValue, Flags );

    // Return to Rexx
    return RetUsr ? RXSUBCOM_OK : rc;
}

/*-------------------------------------------------------------------*/
/* Hercules AWSCMD external function                                 */
/*-------------------------------------------------------------------*/
static HR_EXTFUNC_T  AWSCmd
(
    HR_PCSZ_T   Name,           // Name of the function ("AWSCMD")
    HR_ARGC_T   argc,           // Number of arguments
    HR_ARGV_T   argv,           // Array of argument RXSTRINGs
    HR_PCSZ_T   Queuename,      // Current queue name
    PRXSTRING   RetValue        // Returned results string
)
{
    HR_EXTFUNC_RC_T  rc         = RXFUNC_OK;
    HR_ERR_T         err        = HRERR_OK;
    char*            stemname   = NULL;
    char*            herccmd    = NULL;
    int              panelrc    = 0;
    BYTE             RetUsr     = TRUE;

    // argv[0] = (required) Hercules command to be issued
    // argv[1] = (optional) Response stem variable name (empty = not
    //                      interested in response, i.e. not wanted)
    // argv[2] = (optional) Error Handling option (default = user)

    UNREFERENCED( Name );
    UNREFERENCED( Queuename );

    /* Need at least 1 argument: the Hercules command to be issued. */
    if (0
        || argc < 1
        || argc > 3
        || !RXVALIDSTRING( argv[0] )
    )
    {
        err = HRERR_BADARGS;
        goto quick_exit;
    }

    /* Make a modifiable copy of the Hercules command to be executed */
    if (!(herccmd = rx_strdup( (PCRXSTRING) &argv[0] )))
    {
        err = HRERR_NOMEM;
        goto quick_exit;
    }

    if (argc >= 2)
    {
        /* Save the stem name to be used to return their results in */
        if (RXVALIDSTRING( argv[1] ))
        {
            if (!(stemname = rx_strdup( (PCRXSTRING) &argv[1] )))
            {
                err = HRERR_NOMEM;
                goto quick_exit;
            }
        }

        /* Check if they specified an error handling option */
        if (argc >= 3)
        {
            ASSERT( argc == 3 );

            if (RXVALIDSTRING( argv[2] ))
            {
                /* PROGRAMMING NOTE: Rexx strings are NOT necessarily
                   null terminated. Thus we cannot directly compare a
                   Rexx string using e.g. strcasecmp(). We must make
                   a careful copy of the string instead and use that.
                */
                char* erropt;

                if (!(erropt = rx_strdup( (PCRXSTRING) &argv[2] )))
                {
                    err = HRERR_NOMEM;
                    goto quick_exit;
                }

                if (strcasecmp( erropt, "system" ) == 0)
                    RetUsr = FALSE;

                free( erropt );
            }
        }
    }

    /* Issue the Hercules command and save the results */
    err = HerculesCommand( herccmd, stemname, &panelrc );

quick_exit:

    /* Free acquired storage */
    free( herccmd );
    free( stemname );

    /* Return from External Function */
    rc = ExtFuncRC( err, panelrc, RetValue );

    // Return to Rexx
    return RetUsr ? RXFUNC_OK : rc;
}

/*-------------------------------------------------------------------*/
/* Call Rexx RexxStart function                                      */
/*-------------------------------------------------------------------*/
static HR_REXXRC_T  HRexxStart
(
    HR_ARGC_T       ArgCount,           // Number of arguments
    HR_ARGV_T       ArgList,            // Array of arguments
    HR_PCSZ_T       ProgramName,        // Name of Rexx file
    PRXSTRING       Instore,            // In-Storage Script
    HR_PCSZ_T       EnvName,            // Command env. name
    HR_CALLTYPE_T   CallType,           // Code for how invoked
    PRXSYSEXIT      Exits,              // EXITs on this call
    short*          RetRC,              // Converted return code
    PRXSTRING       Result              // Rexx program output
)
{
    /* PROGRAMMING NOTE: calling a Rexx package's RexxStart function
       is made a separate function unto itself to make it easier to
       debug every call made to RexxStart without having to set many
       separate breakpoints all over the place.
    */
#if REXX_PKGNUM == REGINA_PKGNUM
    /* When running a Regina script in background, handlers need to be re-registered. */
    if (!equal_threads( thread_id(), sysblk.impltid )) RegisterRexxHandlersAndFunctions();
#endif
    HR_REXXRC_T  rc  = REXX_DEP( RexxStart )
    (
        ArgCount,                       // Number of arguments
        ArgList,                        // Array of arguments
        ProgramName,                    // Name of Rexx file
        Instore,                        // In-Storage Script
        EnvName,                        // Command env. name
        CallType,                       // Code for how invoked
        Exits,                          // EXITs on this call
        RetRC,                          // Converted return code
        Result                          // Rexx program output
    );
    return rc;
}

/*-------------------------------------------------------------------*/
/* Common logic to return from executing a Rexx script               */
/*-------------------------------------------------------------------*/
static int ExecRet
(
    const char*  funcname,          // "ExecCmd", "ExecSub", etc...
    const char*  script,            // Rexx script name or script code
    HR_REXXRC_T  rc,                // RexxStart return code
    short*       RetRC,             // Rexx script return code
    PRXSTRING    RetValue           // Rexx script return value
)
{
    const char*  pkgname      =       REXX_DEP( PackageName );
    const char*  failingfunc  = QSTR( REXX_DEP( RexxStart   ));

    if (RXAPI_OK != rc)
    {
        char buf[ 64 ];
        MSGBUF( buf, "%s %s", funcname, failingfunc );
        // "REXX(%s) %s RC(%d)"
        WRMSG( HHC17502, "E", pkgname, buf, (S32) rc );
    }
    else
    {
        if (MsgLevel)
        {
            // "REXX(%s) %s %s RetRC(%hd)"
            WRMSG( HHC17503, "I", pkgname,
                funcname, script, *RetRC );

            if (RetValue->strptr)
            {
                // "REXX(%s) %s %s RetValue(\"%s\")"
                WRMSG( HHC17504, "I", pkgname,
                    funcname, script, RetValue->strptr );
            }
        }
    }

    if (RetValue->strptr)
        REXX_DEP( FreeMemory )( RetValue->strptr );

    return (int) rc;
}

/*-------------------------------------------------------------------*/
/* Execute the specified Rexx script as a COMMAND                    */
/*-------------------------------------------------------------------*/
static int REXX_DEP( ExecCmd )
(
    char*  scriptname,      // Name of Rexx script to be executed
    char*  argstring        // The script's only arguments string
)
{
    HR_REXXRC_T   rc;
    long          argcount;
    RXSTRING      Args[1];
    HR_ARGV_T     ArgList;
    RXSYSEXIT     ExitList[2];
    RXSTRING      RetValue;
    int           calltype;
    short         RetRC;

    /* Build the one and only RXSTRING command arguments string */
    HMAKE_RXSTRING( Args[0], argstring, argstring ? strlen( argstring ) : 0 );

    ExitList[0].sysexit_name  = "HERCSIOE";
    ExitList[0].sysexit_code  = RXSIO;
    ExitList[1].sysexit_code  = RXENDLST;

    RetValue.strptr           = NULL;   // (let Rexx allocate it)
    RetValue.strlength        = 0;      // (let Rexx allocate it)
    argcount                  = 1;
    ArgList                   = (HR_ARGV_T) Args;
    calltype                  = RXCOMMAND;

    rc = HRexxStart( argcount,          // Number of arguments
                     ArgList,           // Array of arguments
                     scriptname,        // Name of Rexx file
                     NULL,              // In-Storage Script
                     "HERCULES",        // Command env. name
                     calltype,          // Code for how invoked
                     ExitList,          // EXITs on this call
                     &RetRC,            // Converted return code
                     &RetValue );       // Rexx program output

    return ExecRet( "ExecCmd", scriptname, rc, &RetRC, &RetValue );
}

/*-------------------------------------------------------------------*/
/* Execute the specified Rexx script as a SUBROUTINE                 */
/*-------------------------------------------------------------------*/
static int REXX_DEP( ExecSub )
(
    char*  scriptname,      // Name of Rexx script to be executed
    int    argc,            // Number of subroutine arguments
    char*  argv[]           // Array of subroutine arguments
)
{
    HR_REXXRC_T   rc;
    long          argcount;
    RXSTRING      Args[ MAX_REXXSTART_ARGS ];
    HR_ARGV_T     ArgList;
    RXSYSEXIT     ExitList[2];
    RXSTRING      RetValue;
    int           calltype;
    short         RetRC;

    /* Build the RXSTRING subroutine arguments array */
    for (argcount=0; argcount < MAX_REXXSTART_ARGS && argcount < argc; argcount++)
    {
        /* Build the RXSTRING for the next argument */
        HMAKE_RXSTRING( Args[ argcount ], argv[ argcount ], strlen( argv[ argcount ]));
    }

    ExitList[0].sysexit_name  = "HERCSIOE";
    ExitList[0].sysexit_code  = RXSIO;
    ExitList[1].sysexit_code  = RXENDLST;

    RetValue.strptr           = NULL;   // (let Rexx allocate it)
    RetValue.strlength        = 0;      // (let Rexx allocate it)
    ArgList                   = (HR_ARGV_T) Args;
    calltype                  = RXSUBROUTINE;

    rc = HRexxStart( argcount,          // Number of arguments
                     ArgList,           // Array of arguments
                     scriptname,        // Name of Rexx file
                     NULL,              // In-Storage Script
                     "HERCULES",        // Command env. name
                     calltype,          // Code for how invoked
                     ExitList,          // EXITs on this call
                     &RetRC,            // Converted return code
                     &RetValue );       // Rexx program output

    return ExecRet( "ExecSub", scriptname, rc, &RetRC, &RetValue );
}

/*-------------------------------------------------------------------*/
/* Execute a Rexx script as a FUNCTION contained entirely in storage */
/*-------------------------------------------------------------------*/
static HR_REXXRC_T  REXX_DEP( ExecInStoreCmd )
(
    char*   sourcecode,     // Source code of Rexx script
    int     argc,           // Number of function arguments
    char*   argv[],         // Array of function arguments
    short*  RetRC,          // Script return code (if Result numeric)
    char*   Result,         // Result string buffer
    size_t  ResultLen       // Size of Result string buffer
)
{
    HR_REXXRC_T   rc;
    long          argcount;
    RXSTRING      Args[ MAX_REXXSTART_ARGS ];
    HR_ARGV_T     ArgList;
    RXSTRING      Instore[2];
    RXSYSEXIT     ExitList[2];
    RXSTRING      RetValue;
    int           calltype;

    const char*   pkgname      =       REXX_DEP( PackageName );
    const char*   failingfunc  = QSTR( REXX_DEP( RexxStart   ));

    /* Build the RXSTRING function arguments array */
    for (argcount=0; argcount < argc && argcount < MAX_REXXSTART_ARGS; argcount++)
    {
        /* Build the RXSTRING for the next argument */
        HMAKE_RXSTRING( Args[ argcount ], argv[ argcount ], strlen( argv[ argcount ]));
    }

    Instore[0].strptr         =         sourcecode;
    Instore[0].strlength      = strlen( sourcecode );
    Instore[1].strptr         = NULL;
    Instore[1].strlength      = 0;

    ExitList[0].sysexit_code  = RXSIO;
    ExitList[0].sysexit_name  = "HERCSIOE";
    ExitList[1].sysexit_code  = RXENDLST;

    RetValue.strptr           = NULL;   // (let Rexx allocate it)
    RetValue.strlength        = 0;      // (let Rexx allocate it)
    ArgList                   = (HR_ARGV_T) Args;
    calltype                  = RXFUNCTION;

    rc = HRexxStart( argcount,          // Number of arguments
                     ArgList,           // Array of arguments
                     "(instore)",       // Name of Rexx file
                     Instore,           // In-Storage Script
                     "HERCULES",        // Command env. name
                     calltype,          // Code for how invoked
                     ExitList,          // EXITs on this call
                     RetRC,             // Converted return code
                     &RetValue );       // Rexx program output

    if (RXAPI_OK != rc)
    {
        char buf[ 64 ];
        MSGBUF( buf, "%s %s", "ExecInStoreCmd", failingfunc );
        // "REXX(%s) %s RC(%d)"
        WRMSG( HHC17502, "E", pkgname, buf, (S32) rc );
        *Result = 0;
    }
    else if (RetValue.strptr)
    {
        /* PROGRAMMING NOTE: the return value returned by Rexx
           is NOT null terminated so we must do that ourselves. */
        strlcpy( Result, RetValue.strptr, MIN( RetValue.strlength+1, ResultLen ));
    }

    /* Done with return value RXSTRING. Free it to prevent leak. */
    if (RetValue.strptr)
        REXX_DEP( FreeMemory )( RetValue.strptr );

    /* Also not interested in Rexx's resulting translated (compiled)
       image of our script so free its memory too to prevent leak. */
    if (Instore[1].strptr)
        REXX_DEP( FreeMemory )( Instore[1].strptr );

    return rc;
}

/*-------------------------------------------------------------------*/
/* Function to cancel a running Rexx script                          */
/*-------------------------------------------------------------------*/
static int REXX_DEP( HaltExec )( pid_t pid, TID tid )
{
    HR_REXXRC_T  rexx_rc;

    rexx_rc = REXX_DEP( SetHalt )( (HR_PROCESS_ID_T) pid, (HR_THREAD_ID_T) tid );

    if (RXARI_OK == rexx_rc)  // NOTE: "RXARI_OK", not "RXAPI_OK"!!
    {
        // "REXX(%s) Signal HALT "TIDPAT" %s"
        WRMSG( HHC17520, "I", REXX_DEP( PackageName ), TID_CAST( tid ), "success" );
        return 0;
    }

    if (RXARI_NOT_FOUND == rexx_rc)
    {
        // "REXX(%s) %s: The target Rexx procedure was not found"
        WRMSG( HHC17518, "E", REXX_DEP( PackageName ), "HaltExec" );
    }
    else // RXARI_PROCESSING_ERROR
    {
        // "REXX(%s) %s: A failure in Rexx processing has occurred"
        WRMSG( HHC17519, "E", REXX_DEP( PackageName ), "HaltExec" );
    }

    return -1;
}

/*-------------------------------------------------------------------*/
/* Register our exit and subcommand handlers      (boolean function) */
/*-------------------------------------------------------------------*/
static BYTE RegisterRexxHandlersAndFunctions()
{
    static  HR_USERINFO_T  user_info  = NULL;   // No extra info needed
            HR_REXXRC_T    rc;                  // Rexx API return code

    /* Register our Hercules Exit handler */
    rc = REXX_DEP( RegisterExit )
         ( "HERCSIOE", (HR_PFN_T) ExitHandler, user_info );

    if (rc != RXAPI_OK)
    {
        // "REXX(%s) %s: return code 0x%8.8"PRIX32" from %s"
        WRMSG( HHC17512, "E", REXX_DEP( PackageName ),
            "RegisterHandlers", (U32) rc,
            QSTR( REXX_DEP( RegisterExit )));
        return FALSE;
    }

    /* Register our Hercules SubCommand handler */
    rc = REXX_DEP( RegisterSubcom )
         ( "HERCULES", (HR_PFN_T) SubComHandler, user_info );

    if (rc != RXAPI_OK)
    {
        // "REXX(%s) %s: return code 0x%8.8"PRIX32" from %s"
        WRMSG( HHC17512, "E", REXX_DEP( PackageName ),
            "RegisterHandlers", (U32) rc,
            QSTR( REXX_DEP( RegisterSubcom )));
        return FALSE;
    }

    /* Register our Hercules 'AwsCmd' external function */
    rc = REXX_DEP( RegisterFunction )
         ( "AWSCMD", (HR_PFN_T) AWSCmd );

    if (rc != RXAPI_OK)
    {
        // "REXX(%s) %s: return code 0x%8.8"PRIX32" from %s"
        WRMSG( HHC17512, "E", REXX_DEP( PackageName ),
            "RegisterFunctions", (U32) rc,
            QSTR( REXX_DEP( RegisterFunction )));
        return FALSE;
    }

    return TRUE;
}

/*-------------------------------------------------------------------*/
/* Retrieve package version and source strings    (boolean function) */
/*-------------------------------------------------------------------*/
static BYTE GetRexxVersionSource()
{
    char         Result[ 256 ]  = {0};    // Results buffer
    short        RetRC          =  0;     // Script retcode (not used)
    HR_REXXRC_T  rc             =  0;     // Rexx API return code
    char*        source         = NULL;   // Will point to Rexx SOURCE

    /* Free the old values; we'll be retrieving new ones */
    if (REXX_DEP( PackageVersion )) free( REXX_DEP( PackageVersion ));
    if (REXX_DEP( PackageSource  )) free( REXX_DEP( PackageSource  ));

    /* Retrieve Rexx Interpreter version and source value */
    if (0
        || RXAPI_OK != (rc = REXX_DEP( ExecInStoreCmd )
           ( VER_SRC_INSTOR_SCRIPT, 0, NULL, &RetRC, Result, sizeof( Result )))
        || !(source = strchr( Result, '\n' ))  // (parse results too)
    )
    {
        // "REXX(%s) Rexx version/source retrieval failed: RC(%d)"
        WRMSG( HHC17501, "E", REXX_DEP( PackageName ), (S32) rc );

        REXX_DEP( PackageVersion ) = strdup( "unknown" );
        REXX_DEP( PackageSource  ) = strdup( "unknown" );
    }
    else
    {
        *source++ = 0; // (null terminate version and point to source)
        REXX_DEP( PackageVersion ) = strdup( Result );
        REXX_DEP( PackageSource  ) = strdup( source );

        /* Extract the major version number too */

        // "REXX-ooRexx_4.2.0(MT)_32-bit"
        //              ^
        //              ^
        //              ^

        REXX_DEP( PackageMajorVers ) = REXX_DEP( PackageVersion )[12];
    }

    return source ? TRUE : FALSE;
}

/*-------------------------------------------------------------------*/
/* Resolve symbol from given Rexx library         (boolean function) */
/*-------------------------------------------------------------------*/
static BYTE ResolveLibSym ( void** ppfn, char* name, void* lib, BYTE verbose )
{
    if (!(*ppfn = dlsym( lib, name )))
    {
        if (verbose)
        {
            // "REXX(%s) dlsym '%s' failed: %s"
            WRMSG( HHC17533, "E", REXX_DEP( PackageName ),
                name, dlerror());
        }
        return FALSE;
    }
    return TRUE;
}

/*-------------------------------------------------------------------*/
/* Load EXTRA Rexx Library(s)                     (boolean function) */
/*-------------------------------------------------------------------*/
BYTE REXX_DEP( LoadExtra )( BYTE verbose )
{
    BYTE   bSuccess  = TRUE;
    int    i;
    char*  libname;
    void*  libhandle[ NUM_EXTRALIBS ]  = {0};

    for (i=0; i < (int) NUM_EXTRALIBS; ++i)
    {
        libname = REXX_DEP( ExtraLibs )[i];

        /* ooRexx version 5 and higher doesn't have the rexxutil library anymore */
#if REXX_PKGNUM == OOREXX_PKGNUM
        if (1
            && (REXX_DEP( PackageMajorVers ) >= '5')
    #if defined( _MSVC_ )
            && !strcmp( libname, "rexxutil.dll" )
    #elif defined( __APPLE__ )
            && !strcmp( libname, "librexxutil.dylib" )
    #else
            && !strcmp( libname, "librexxutil.so" )
    #endif
           )
               continue;
#endif

        if (libname && !(libhandle[i] = dlopen( libname, RTLD_NOW )))
        {
            bSuccess = FALSE;

            if (verbose)
            {
                // "REXX(%s) dlopen '%s' failed: %s"
                WRMSG( HHC17531, "W", REXX_DEP( PackageName ),
                    libname, dlerror());
            }
        }
    }

    /* All or nothing! If ANY load fails, then they ALL fail! */

    if (!bSuccess)
        for (i=0; i < (int) NUM_EXTRALIBS; ++i)
            if (libhandle[i])
                dlclose( libhandle[i] );

    return bSuccess;
}

/*-------------------------------------------------------------------*/
/* Load Rexx Library(s) and get function pointers (boolean function) */
/*-------------------------------------------------------------------*/
BYTE REXX_DEP( Load )( BYTE verbose )
{
    /* Load PRIMARY libraries first, and resolve needed entry-points */

    if (LibHandle && ApiLibHandle)
        return TRUE; // (we already did this)

    if (!(LibHandle = dlopen( LibName, RTLD_NOW )))
    {
        if (verbose)
        {
            // "REXX(%s) dlopen '%s' failed: %s"
            WRMSG( HHC17531, "W", REXX_DEP( PackageName ),
                LibName, dlerror());
        }
        LibHandle    = NULL;
        ApiLibHandle = NULL;
        return FALSE;
    }

    if (*ApiLibName)
    {
        if (!(ApiLibHandle = dlopen( ApiLibName, RTLD_NOW )))
        {
            if (verbose)
            {
                // "REXX(%s) dlopen '%s' failed: %s"
                WRMSG( HHC17531, "E", REXX_DEP( PackageName ),
                    ApiLibName, dlerror());
            }
            dlclose( LibHandle );
            LibHandle    = NULL;
            ApiLibHandle = NULL;
            return FALSE;
        }
    }
    else
        ApiLibHandle = LibHandle;

    if (0
        || !ResolveLibSym( (void**) &REXX_DEP( RexxStart        ), REXX_START,                LibHandle, verbose )
        || !ResolveLibSym( (void**) &REXX_DEP( VariablePool     ), REXX_VARIABLE_POOL,        LibHandle, verbose )
        || !ResolveLibSym( (void**) &REXX_DEP( SetHalt          ), REXX_SETHALT,              LibHandle, verbose )

        || !ResolveLibSym( (void**) &REXX_DEP( RegisterFunction ), REXX_REGISTER_FUNCTION, ApiLibHandle, verbose )
        || !ResolveLibSym( (void**) &REXX_DEP( RegisterSubcom   ), REXX_REGISTER_SUBCOM,   ApiLibHandle, verbose )
        || !ResolveLibSym( (void**) &REXX_DEP( RegisterExit     ), REXX_REGISTER_EXIT,     ApiLibHandle, verbose )
        || !ResolveLibSym( (void**) &REXX_DEP( AllocateMemory   ), REXX_ALLOCATE_MEMORY,   ApiLibHandle, verbose )
        || !ResolveLibSym( (void**) &REXX_DEP( FreeMemory       ), REXX_FREE_MEMORY,       ApiLibHandle, verbose )

        || !RegisterRexxHandlersAndFunctions()
        || !GetRexxVersionSource()
    )
    {
        dlclose( LibHandle );

        if (ApiLibHandle != LibHandle)
            dlclose( ApiLibHandle );

        LibHandle    = NULL;
        ApiLibHandle = NULL;

        REXX_DEP( RexxStart        ) = NULL;
        REXX_DEP( VariablePool     ) = NULL;
        REXX_DEP( RegisterFunction ) = NULL;
        REXX_DEP( RegisterSubcom   ) = NULL;
        REXX_DEP( RegisterExit     ) = NULL;
        REXX_DEP( AllocateMemory   ) = NULL;
        REXX_DEP( FreeMemory       ) = NULL;
        REXX_DEP( SetHalt          ) = NULL;

        return FALSE;
    }

    /* Load all EXTRA libraries last... */

    /*---------------------------------------------------------------*/
    /*                                                               */
    /*                     PROGRAMMING NOTE                          */
    /*                                                               */
    /* It is important (at least for Windows OORexx) to also load    */
    /* all of the extra "Extension" libraries too, in case any of    */
    /* the scripts we ask rexx to run for us might require them      */
    /* (such as HOSTEMU.DLL which processes "EXECIO" statements).    */
    /*                                                               */
    /* This is important because rexx's automatic library loading    */
    /* logic might contain a race condition bug in its automatic     */
    /* library load//unload logic (like OORexx does) where if two    */
    /* threads both require the same extension DLL (hostemu.dll),    */
    /* the library ends up being unloaded by the first thread before */
    /* the second thread has had a chance to excecute, leading to a  */
    /* fatal Hercules crash when the second thread tries calling a   */
    /* rexx function within a library that has just been unloaded!   */
    /*                                                               */
    /* GitHub issue #140 "Random Hercules crash using OORexx with    */
    /* HAO": https://github.com/SDL-Hercules-390/hyperion/issues/140 */
    /*                                                               */
    /*---------------------------------------------------------------*/
    if (!REXX_DEP( LoadExtra )( verbose ))
    {
        // A failure to load the ExtraLibs is fatal for
        // version 4.2.0 of ooRexx or earlier. For ooRexx
        // version 5.0.0 and later it is only a warning.

        if (REXX_DEP( PackageMajorVers ) < '5')
        {
            dlclose( LibHandle );
            if (ApiLibHandle != LibHandle)
                dlclose( ApiLibHandle );
            LibHandle    = NULL;
            ApiLibHandle = NULL;
            return FALSE;
        }
    }

    return TRUE;
}

/*-------------------------------------------------------------------*/
/* Enable our Rexx package                       (boolean function ) */
/*-------------------------------------------------------------------*/
BYTE REXX_DEP( Enable )( BYTE verbose )
{
    PackageName       = "";
    PackageVersion    = NULL;
    PackageSource     = NULL;
    ExecCmd           = NULL;
    ExecSub           = NULL;
    HaltExec          = NULL;

    if (!REXX_DEP( Load )( verbose ))
        return FALSE;

    PackageName       = REXX_DEP( PackageName    );
    PackageVersion    = REXX_DEP( PackageVersion );
    PackageSource     = REXX_DEP( PackageSource  );
    ExecCmd           = REXX_DEP( ExecCmd        );
    ExecSub           = REXX_DEP( ExecSub        );
    HaltExec          = REXX_DEP( HaltExec       );

    return TRUE;
}

#endif // REXX_PKG
