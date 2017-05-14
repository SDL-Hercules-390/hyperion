/* HREXXAPI.H   (c)Copyright Enrico Sorichetti, 2012                  */
/*              Rexx Interpreter Support                              */
/*                                                                    */
/*  Released under "The Q Public License Version 1"                   */
/*  (http://www.hercules-390.org/herclic.html) as modifications to    */
/*  Hercules.                                                         */

/*  inspired by the previous Rexx implementation by Jan Jaeger        */

#ifndef _HREXXAPI_H_
#define _HREXXAPI_H_

#define LOG_CAPTURE( _RETC_ , _RESP_, _FCNM_, _BUFF_) do { \
_RESP_ = log_capture( _FCNM_ , _BUFF_ ); \
_RETC_ = 0; \
} while (0)

/*--------------------------------------------------------------------*/
/* Drop a Variable form the Rexx variable pool                        */
/*--------------------------------------------------------------------*/
int HREXXDROPVAR (
    char *    pszVar )          /* Variable name                      */
{
SHVBLOCK      RxVarBlock;
unsigned long ulRetc;

    /* initialize the RxVarBlock */
    RxVarBlock.shvnext = NULL;
    RxVarBlock.shvname.strptr = pszVar;
    RxVarBlock.shvname.strlength = strlen(pszVar);
    RxVarBlock.shvnamelen = RxVarBlock.shvname.strlength;
    RxVarBlock.shvvalue.strptr = NULL;
    RxVarBlock.shvvalue.strlength = 0;
    RxVarBlock.shvvaluelen = 0;
    RxVarBlock.shvcode = RXSHV_SYDRO;
    RxVarBlock.shvret = RXSHV_OK;

    /* issue the request */
    ulRetc = (*hRexxVariablePool)(&RxVarBlock);

    /* test return code */
    if (ulRetc == RXSHV_OK || ulRetc != RXSHV_BADN) {
        ulRetc = 0;
    }

    return (int)ulRetc;
}

/*--------------------------------------------------------------------*/
/* Fetch a Variable Value from the Rexx variable pool                 */
/*--------------------------------------------------------------------*/
int HREXXFETCHVAR (
    char *    pszVar,            /* Variable name                     */
    PRXSTRING pValue)            /* Variable value                    */
{
SHVBLOCK      RxVarBlock;
unsigned long ulRetc;
char * zValue ;

    /* initialize the RxVarBlock */
    RxVarBlock.shvnext = NULL;
    RxVarBlock.shvname.strptr = pszVar;
    RxVarBlock.shvname.strlength = strlen(pszVar);
    RxVarBlock.shvnamelen = RxVarBlock.shvname.strlength;
    RxVarBlock.shvvalue.strptr = NULL;
    RxVarBlock.shvvalue.strlength = 0;
    RxVarBlock.shvvaluelen = 0;
    RxVarBlock.shvcode = RXSHV_SYFET;
    RxVarBlock.shvret = RXSHV_OK;

    /* fetch variable from pool */
    ulRetc = (*hRexxVariablePool)(&RxVarBlock);

    /* test return code */
    if (ulRetc != RXSHV_OK ) {
        pValue->strptr = NULL;
        pValue->strlength = 0;
    }
    else {
        /* allocate a new buffer for the Rexx variable pool value */
        zValue = (char *) (*hRexxAllocateMemory)(RxVarBlock.shvvalue.strlength + 1);
        if ( !zValue ) {
            pValue->strptr = NULL;
            pValue->strlength = 0;
            ulRetc = RXSHV_MEMFL;
        }
        else {
            /* copy to new buffer and zero-terminate */
            memmove( zValue, RxVarBlock.shvvalue.strptr, RxVarBlock.shvvalue.strlength);
            *(zValue + RxVarBlock.shvvalue.strlength) = '\0';
            pValue -> strptr = zValue;
            pValue -> strlength = RxVarBlock.shvvalue.strlength;
        }
        // free memory returned from RexxVariablePool API
        (*hRexxFreeMemory)(RxVarBlock.shvvalue.strptr);
    }
    return (int)ulRetc;
}

/*--------------------------------------------------------------------*/
/* Set/update a Variable in the Rexx variable pool                    */
/*--------------------------------------------------------------------*/
int HREXXSETVAR (
    char *        pszVar,       /* Variable name to be set/updated    */
    char *        pValue,       /* Ptr to value                       */
    size_t        lValue)       /* Value length                       */
{
SHVBLOCK      RxVarBlock;
unsigned long ulRetc;

    /* initialize the RxVarBlock */
    RxVarBlock.shvnext = NULL;
    RxVarBlock.shvname.strptr = pszVar;
    RxVarBlock.shvname.strlength = strlen(pszVar);
    RxVarBlock.shvnamelen = RxVarBlock.shvname.strlength;
    RxVarBlock.shvvalue.strptr = pValue;
    RxVarBlock.shvvalue.strlength = lValue;
    RxVarBlock.shvvaluelen = lValue;
    RxVarBlock.shvcode = RXSHV_SYSET;
    RxVarBlock.shvret = RXSHV_OK;

    /* set variable in pool */
    ulRetc = (*hRexxVariablePool)(&RxVarBlock);

    /* test return code */
    if (ulRetc == RXSHV_NEWV)
        ulRetc = RXSHV_OK;

    return (int)ulRetc;
}

/*--------------------------------------------------------------------*/
/*                                                                    */
/*--------------------------------------------------------------------*/
static int hSubcomError (
    int rc,
    int flag,
    unsigned short *Flags,
    PRXSTRING RetValue )
{
    sprintf( RetValue->strptr, "%d", rc );
    RetValue->strlength = strlen( RetValue->strptr );
    *Flags = flag;
    return rc;
}

/*--------------------------------------------------------------------*/
/* the core of the whole shebang, the subcommand handler              */
/*--------------------------------------------------------------------*/
static HREXXSUBCOMHANDLER_T hSubcomHandler (
    HREXXPSTRING Command,       /* Command string from Rexx           */
    unsigned short *Flags,      /* Returned Error/Failure Flags       */
    PRXSTRING RetValue )        /* Returned RC string                 */
{

short rc;
char *line = NULL;
int   coun;
char *wCommand = NULL;
char *wResp = NULL;

int   ReturnFlag = 1 ; // 1 ==> let the user process the errors
                       // 0 ==> let the system take care of them

char *RespStemName = NULL;
int   RespStemLength = 0;
char *RespStemOffset = NULL;

char temp[WRK_AREA_SIZE];

RXSTRING hErrorHandler;
RXSTRING hPersistentRespStemName ;
RXSTRING hRespStemName ;

    HREXXFETCHVAR( HREXX_ERRORHANDLER_VARNAME, &hErrorHandler );

    if ( RXVALIDSTRING( hErrorHandler ) )
    {
        if ( strcasecmp( RXSTRPTR(hErrorHandler), "retcode" ) == 0 )
            ReturnFlag = 1 ; // let the user process the errors
        else if ( strcasecmp( RXSTRPTR(hErrorHandler), "system" ) == 0 )
            ReturnFlag = 0 ; // let the system take care of them
        else
            ReturnFlag = 1 ; // let the user process the errors
        (*hRexxFreeMemory)( RXSTRPTR( hErrorHandler ) );
        RXSTRPTR(hErrorHandler) = NULL;
    }
    else
        ReturnFlag = 1 ; // let the user process the errors

    HREXXFETCHVAR( HREXX_PERSISTENTRESPSTEMNAME_VARNAME, &hPersistentRespStemName );
    HREXXFETCHVAR( HREXX_RESPSTEMNAME_VARNAME,           &hRespStemName           );

    if ( RXVALIDSTRING( hRespStemName ) )
    {
        RespStemLength = hRespStemName.strlength ;
        RespStemName = (*hRexxAllocateMemory)( RespStemLength + WRK_AREA_SIZE );
        if ( !RespStemName )
            return hSubcomError( -4, RXAPI_MEMFAIL, Flags, RetValue );

        strcpy( RespStemName, RXSTRPTR( hRespStemName ) );

        if  ( RespStemName[RespStemLength-1] != '.' )
        {
            RespStemName[RespStemLength]   = '.';
            RespStemName[RespStemLength+1] = '\0';
        }
        RespStemOffset = RespStemName + strlen( RespStemName );
        sprintf(RespStemOffset,"%d",0);
        sprintf(temp,"%d",0);
        HREXXSETVAR( RespStemName, temp, strlen(temp) );

        (*hRexxFreeMemory)( RXSTRPTR( hRespStemName ) );
        RXSTRPTR( hRespStemName ) = NULL;
        HREXXDROPVAR( HREXX_RESPSTEMNAME_VARNAME );

    }
    else if ( RXVALIDSTRING( hPersistentRespStemName ) )
    {
        RespStemLength = RXSTRLEN( hPersistentRespStemName );
        RespStemName = (*hRexxAllocateMemory)( RespStemLength + WRK_AREA_SIZE );
        if ( !RespStemName )
            return hSubcomError( -4, RXAPI_MEMFAIL, Flags, RetValue );

        strcpy( RespStemName, RXSTRPTR( hPersistentRespStemName ) );

        if  ( RespStemName[RespStemLength-1] != '.' )
        {
            RespStemName[RespStemLength]   = '.';
            RespStemName[RespStemLength+1] = '\0';
        }
        RespStemOffset = RespStemName + strlen( RespStemName );
        sprintf(RespStemOffset,"%d",0);
        sprintf(temp,"%d",0);
        HREXXSETVAR( RespStemName, temp, strlen(temp) );

        (*hRexxFreeMemory)( RXSTRPTR( hPersistentRespStemName ) );
        RXSTRPTR( hPersistentRespStemName ) = NULL;
    }
    else
    {
        RespStemName = NULL;
        RespStemLength = 0;
        RespStemOffset = NULL;
    }

    if ( ! RXVALIDSTRING( *Command ) )
        return hSubcomError( -4, RXSUBCOM_ERROR, Flags, RetValue );

    wCommand = (*hRexxAllocateMemory)( RXSTRLEN( *Command ) + WRK_AREA_SIZE );
    if ( !wCommand )
        return hSubcomError( -4, RXAPI_MEMFAIL, Flags, RetValue );

    strncpy( wCommand, RXSTRPTR( *Command ), RXSTRLEN( *Command ) );
    wCommand[RXSTRLEN(*Command)] = '\0';

    if ( RespStemName )
    {
        rc = panel_command_capture( wCommand, &wResp );
        coun = 0;
        if ( wResp )
        {
            for ( coun=0, line=strtok(wResp, "\n" ); line; line = strtok(NULL, "\n") )
            {
                coun++;
                sprintf( RespStemOffset, "%d", coun );
                HREXXSETVAR( RespStemName, line, strlen(line) );
            }
            (*hRexxFreeMemory)( wResp );
        }
        sprintf( RespStemOffset, "%d", 0 );
        sprintf( temp, "%d", coun );
        HREXXSETVAR( RespStemName, temp, strlen(temp) );
        (*hRexxFreeMemory)( RespStemName );
        if ( ReturnFlag )
            *Flags = RXSUBCOM_OK;
        else
            *Flags = rc < 0 ? RXSUBCOM_ERROR : rc > 0 ? RXSUBCOM_FAILURE : RXSUBCOM_OK;
    }
    else
    {
        rc = (int)(uintptr_t)panel_command( wCommand );
        if ( ReturnFlag )
            *Flags = RXSUBCOM_OK;
        else
            *Flags = rc < 0 ? RXSUBCOM_ERROR : rc > 0 ? RXSUBCOM_FAILURE : RXSUBCOM_OK;
    }

    (*hRexxFreeMemory)( wCommand );

    sprintf( RetValue->strptr, "%d", rc );
    RetValue->strlength = strlen( RetValue->strptr );

    return rc;
}

/*--------------------------------------------------------------------*/
/* the exit handler                                                   */
/*--------------------------------------------------------------------*/
static HREXXEXITHANDLER_T hExitHandler(
    HREXXLONG ExitNumber,       /* code defining the exit function    */
    HREXXLONG SubFunction,      /* code defining the exit SubFunction */
    PEXIT ParmBlock )           /* function-dependent control block   */
{
    switch( ExitNumber )
    {
        case RXSIO:
            switch( SubFunction ) {
                case RXSIOSAY:
                    if (MessagePrefix)
                        logmsg("%-9s %s\n", MessagePrefix, RXSTRPTR(((RXSIOSAY_PARM *)ParmBlock)->rxsio_string));
                    else
                        logmsg("%s\n", RXSTRPTR(((RXSIOSAY_PARM *)ParmBlock)->rxsio_string));
                    return RXEXIT_HANDLED;
                    break;
                case RXSIOTRC:
                    if (MessagePrefix)
                        logmsg("%-9s %s\n", ErrorPrefix, RXSTRPTR(((RXSIOTRC_PARM *)ParmBlock)->rxsio_string));
                    else
                        logmsg("%s\n", RXSTRPTR(((RXSIOTRC_PARM *)ParmBlock)->rxsio_string));
                    return RXEXIT_HANDLED;
                    break;
                case RXSIOTRD:
                    MAKERXSTRING(((RXSIOTRD_PARM *)ParmBlock)->rxsiotrd_retc, NULL, 0);
                    return RXEXIT_HANDLED;
                    break;
                case RXSIODTR:
                    MAKERXSTRING(((RXSIODTR_PARM *)ParmBlock)->rxsiodtr_retc, NULL, 0);
                    return RXEXIT_HANDLED;
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
    return RXEXIT_NOT_HANDLED;
}

/*--------------------------------------------------------------------*/
/*                                                                    */
/*--------------------------------------------------------------------*/
int HREXXREGISTERHANDLERS()
{
int rc=0;

    if ( !hRexxLibHandle )
    {
        if ( (*RexxDynamicLoader)() != 0 )
           return -1;
    }

    rc = (*hRexxRegisterExit)( hSIOExit, (HREXXPFN)hExitHandler, NULL );
    if (rc != RXEXIT_OK && rc != RXEXIT_NOTREG)
    {
        WRMSG( HHC17534, "E", RexxPackage, "Exit Handler", rc);
        return -1;
    }

    rc = (*hRexxRegisterSubcom)( hSubcom, (HREXXPFN)hSubcomHandler, NULL );
    if (rc != RXSUBCOM_OK && rc != RXSUBCOM_NOTREG)
    {
        WRMSG( HHC17534, "E", RexxPackage, "Subcom Handler", rc);
        return -1;
    }
    return 0;

}

/*--------------------------------------------------------------------*/
/*  Forward reference                                                 */
/*--------------------------------------------------------------------*/
static HREXXEXTERNALFUNCTION_T HREXXAWSCMD(
    const char *Name,
    long argc,
    HREXXSTRING argv[],
    const char *Queuename,
    PRXSTRING RetValue );

/*--------------------------------------------------------------------*/
/*                                                                    */
/*--------------------------------------------------------------------*/
int HREXXREGISTERFUNCTIONS()
{
int rc=0;

    if ( !hRexxLibHandle )
    {
        if ( (*RexxDynamicLoader)() != 0 )
           return -1;
    }

    rc = (*hRexxRegisterFunction)( "AWSCMD", (HREXXPFN) HREXXAWSCMD );
    if (rc == RXFUNC_OK || rc == RXFUNC_DEFINED )
        return 0;
    else
        return -1;


}

/*--------------------------------------------------------------------*/
/* exec Command - execute a rexx script                               */
/*--------------------------------------------------------------------*/
int HREXXEXECCMD (
    char *Command,
    char *args,
    int   argc,
    char *argv[] )
{
int   rc;
short RetRC;
HREXXSTRING    wArgs;
RXSYSEXIT      ExitList[2];
RXSTRING       RetValue;

    UNREFERENCED(argc);

    if ( (*RexxRegisterHandlers)() != 0 )
        return -1;

    if ( (*RexxRegisterFunctions)() != 0 )
        return -1;

    if ( args )
    {
        MAKERXSTRING( wArgs, args, strlen(args) );
    }
    else
    {
        MAKERXSTRING( wArgs, NULL, 0 );
    }

    ExitList[0].sysexit_name = hSIOExit;
    ExitList[0].sysexit_code = RXSIO;
    ExitList[1].sysexit_code = RXENDLST;

    RetValue.strptr = NULL;       /* initialize return-pointer to empty */
    RetValue.strlength = 0;       /* initialize return-length to zero   */

    rc= (*hRexxStart)((args) ? 1 : 0 ,               /* number of arguments   */
                       &wArgs,                       /* array of arguments    */
                       Command,                      /* name of Rexx file     */
                       NULL,                         /* No Instore used       */
                       hSubcom,                      /* Command env. name     */
                       RXCOMMAND,                    /* Code for how invoked  */
                       ExitList,                     /* EXITs on this call    */
                       &RetRC,                       /* converted return code */
                       &RetValue);                   /* Rexx program output   */

    if ( rc !=  0 )
    {
        WRMSG( HHC17502, "E", RexxPackage, RexxPackage, rc);
    }
    else
    {
        if ( MessageLevel > 0 )
        {
            WRMSG( HHC17503, "I", RexxPackage, argv[1], RetRC);
            if (RetValue.strptr != NULL)
            {
                WRMSG( HHC17504, "I", RexxPackage, argv[1], RetValue.strptr);
            }
        }
        if (RetValue.strptr != NULL)
        {
            (*hRexxFreeMemory)(RetValue.strptr);
        }

    }
    return rc;
}

/*-------------------------------------------------------------------*/
/* exec Command - execute an Instore rexx script                     */
/*-------------------------------------------------------------------*/
int HREXXEXECINSTORECMD (
    char *Command,
    char *args,
    short RetRC,
    char *Result )
{
int rc;

HREXXSTRING    wArgs;
RXSTRING       Instore[2];
RXSYSEXIT      ExitList[2];
RXSTRING       RetValue;

    if ( (*RexxRegisterHandlers)() != 0 )
        return -1;

    if ( (*RexxRegisterFunctions)() != 0 )
        return -1;

    if ( args )
    {
        MAKERXSTRING( wArgs, args, strlen(args) );
    }
    else
    {
        MAKERXSTRING( wArgs, NULL, 0 );
    }

    Instore[0].strptr=Command;
    Instore[0].strlength=strlen( Instore[0].strptr );
    Instore[1].strptr=NULL;
    Instore[1].strlength=0;

    ExitList[0].sysexit_name = hSIOExit;
    ExitList[0].sysexit_code = RXSIO;
    ExitList[1].sysexit_code = RXENDLST;

    RetValue.strptr=NULL;
    RetValue.strlength=0;

/*  object rexx note
    the file name is used as an eye catcher even if none needed
    for source look alike with regina rexx;

    regina rexx note
    clumsily needs a rexx file name also for Instore scripts;
    the issue could be fixed with about five lines of code
    it would also fix a wrong message issued, but ...
    */
    rc= (*hRexxStart)((args) ? 1 : 0 ,               /* number of arguments   */
                       &wArgs,                       /* array of arguments    */
                       "Instore",                    /* placeholder, see note */
                       Instore,                      /* Instore used          */
                       hSubcom,                      /* Command env. name     */
                       RXFUNCTION,                   /* Code for how invoked  */
                       ExitList,                     /* EXITs on this call    */
                       &RetRC,                       /* converted return code */
                       &RetValue);                   /* Rexx program output   */

    if ( rc !=  0 )
    {
        WRMSG( HHC17502, "E", RexxPackage, RexxPackage, rc );
        if (RetValue.strptr != NULL)
            (*hRexxFreeMemory)( RetValue.strptr );
    }
    else
    if (RetValue.strptr != NULL)
    {
        strcpy( Result, RetValue.strptr );
        (*hRexxFreeMemory)( RetValue.strptr );
    }
    return rc;

}

/*-------------------------------------------------------------------*/
/* exec Command - execute a rexx script AS SUBROUTINE                */
/*-------------------------------------------------------------------*/
int HREXXEXECSUB (
    char *Command,
    char *args,
    int   argc,
    char *argv[] )
{
int   iarg ;
int   rc;
short RetRC;
long  argcount;

HREXXSTRING    wArgs[MAX_ARGS_TO_REXXSTART + 1];
RXSYSEXIT      ExitList[2];
RXSTRING       RetValue;

    UNREFERENCED( args );
    UNREFERENCED( argc );

    if ( (*RexxRegisterHandlers)() != 0 )
        return -1;

    if ( (*RexxRegisterFunctions)() != 0 )
        return -1;

    for (argcount=0, iarg=2;
         argcount < MAX_ARGS_TO_REXXSTART && iarg < argc;
         argcount++, iarg++)
        MAKERXSTRING( wArgs[argcount], argv[iarg], strlen( argv[iarg] ) );

    ExitList[0].sysexit_name = hSIOExit;
    ExitList[0].sysexit_code = RXSIO;
    ExitList[1].sysexit_code = RXENDLST;

    RetValue.strptr = NULL;       /* initialize return-pointer to empty */
    RetValue.strlength = 0;       /* initialize return-length to zero   */

    rc= (*hRexxStart)( argcount,                     /* number of arguments   */
                       wArgs,                        /* array of arguments    */
                       Command,                      /* name of Rexx file     */
                       NULL,                         /* No Instore used       */
                       hSubcom,                      /* Command env. name     */
                       RXSUBROUTINE,                 /* Code for how invoked  */
                       ExitList,                     /* EXITs on this call    */
                       &RetRC,                       /* converted return code */
                       &RetValue);                   /* Rexx program output   */

    if ( rc !=  0 )
    {
        WRMSG( HHC17502, "E", RexxPackage, RexxPackage, rc );
    }
    else
    {
        if ( MessageLevel > 0)
        {
            WRMSG( HHC17503, "I", RexxPackage, argv[1], RetRC );
            if (RetValue.strptr != NULL)
            {
                WRMSG( HHC17504, "I", RexxPackage, argv[1], RetValue.strptr );
            }
        }
        if (RetValue.strptr != NULL )
        {
            (*hRexxFreeMemory)( RetValue.strptr );
        }

    }
    return rc;
}

/*--------------------------------------------------------------------*/
/*                                                                    */
/*--------------------------------------------------------------------*/
static int hFunctionError (
    int rc,
    PRXSTRING RetValue )
{
    sprintf( RetValue->strptr, "%d", rc );
    RetValue->strlength = strlen( RetValue->strptr );
    return rc;
}

/*--------------------------------------------------------------------*/
/*                                                                    */
/*--------------------------------------------------------------------*/
static HREXXEXTERNALFUNCTION_T HREXXAWSCMD (
    const char *Name,
    long argc,
    HREXXSTRING argv[],
    const char *Queuename,
    PRXSTRING RetValue )
{
short rc;
char *line = NULL;
int   coun;
char *wCommand = NULL;
char *wResp = NULL;

int   ReturnFlag = 1 ; // 1 ==> let the user process the errors
                       // 0 ==> let the system take care of them

char *RespStemName = NULL;
int   RespStemLength = 0;
char *RespStemOffset = NULL;

char temp[WRK_AREA_SIZE];

UNREFERENCED(Name);
UNREFERENCED(Queuename);

//    int iarg;
//    printf ("Name  : %s\n",Name);
//    printf ("argc  : %d\n",(int)argc);
//    for ( iarg = 0; iarg < argc; iarg++)
//        printf("%6d: %s\n",iarg,argv[iarg].strptr);
//    printf ("Qname : %s\n",Queuename);

    if ( argc > 2 )
    {
        if ( RXVALIDSTRING( argv[2] ) )
        {
            if ( strcasecmp( RXSTRPTR( argv[2] ), "retcode" ) == 0 )
                ReturnFlag = 1 ; // let the user process the errors
            else if ( strcasecmp( RXSTRPTR( argv[2] ), "system" ) == 0 )
                ReturnFlag = 0 ; // let the system take care of them
            else
                ReturnFlag = 1 ; // let the user process the errors
        }
        else
            ReturnFlag = 1 ; // let the user process the errors
    }

    if ( argc > 1 )
    {
        if ( RXVALIDSTRING( argv[1] ) )
        {

            RespStemLength = RXSTRLEN( argv[1] ) ;
            RespStemName = (char *)(*hRexxAllocateMemory)( RespStemLength + WRK_AREA_SIZE );
            if ( !RespStemName )
                return hFunctionError( 1, RetValue );
            strcpy( RespStemName, RXSTRPTR( argv[1] ) );

            if  ( RespStemName[RespStemLength-1] != '.' )
            {
                RespStemName[RespStemLength]   = '.';
                RespStemName[RespStemLength+1] = '\0';
            }
            RespStemOffset = RespStemName + strlen( RespStemName );
            sprintf(RespStemOffset,"%d",0);
            sprintf(temp,"%d",0);
            HREXXSETVAR( RespStemName, temp, strlen(temp) );
        }
        else
        {
            RespStemName = NULL;
            RespStemLength = 0;
            RespStemOffset = NULL;
        }
    }
    else
    {
        RespStemName = NULL;
        RespStemLength = 0;
        RespStemOffset = NULL;
    }

    if ( argc <= 0 || ! RXVALIDSTRING( argv[0] ) )
        return hFunctionError( 1, RetValue );

    wCommand = (char *)(*hRexxAllocateMemory)(RXSTRLEN( argv[0] ) + WRK_AREA_SIZE );
    if ( !wCommand )
        return hFunctionError( RXAPI_MEMFAIL, RetValue );

    strncpy( wCommand, RXSTRPTR( argv[0] ), RXSTRLEN( argv[0] ) );
    wCommand[ RXSTRLEN ( argv[0] ) ] = '\0';

    if ( RespStemName )
    {
        rc = panel_command_capture( wCommand, &wResp );
        coun = 0;
        if ( wResp )
        {
            for ( coun=0, line=strtok(wResp, "\n" ); line; line = strtok(NULL, "\n") )
            {
                coun++;
                sprintf( RespStemOffset, "%d", coun );
                HREXXSETVAR( RespStemName, line, strlen(line) );
            }
            free( wResp );
        }
        sprintf( RespStemOffset, "%d", 0 );
        sprintf( temp, "%d", coun );
        HREXXSETVAR( RespStemName, temp, strlen(temp) );
        (*hRexxFreeMemory)( RespStemName );
    }
    else
    {
        rc = (int)(uintptr_t)panel_command( wCommand );
    }

    (*hRexxFreeMemory)( wCommand );

    sprintf( RetValue->strptr, "%d", rc );
    RetValue->strlength = strlen( RetValue->strptr );

    if ( ReturnFlag )
        return 0;
    else
        return rc;
}

#endif /* #ifndef _HREXXAPI_H_  */
