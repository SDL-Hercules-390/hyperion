/* CONSPAWN.C   (c) "Fish" (David B. Trout), 2005-2012               */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* CONSPAWN.C   (c) "Fish" (David B. Trout), 2005-2006               */
/*       This program is spawned by Hercules as a result of          */
/*       the 'sh' (shell) command. It's purpose is to simply         */
/*       call the host's shell (command interpreter) program         */
/*       with the arguments supplied (usually to invoke yet          */
/*       another program), redirecting the results back to           */
/*       Hercules for display. NOTE that this program does           */
/*       not perform the actual stdio redirection itself, but        */
/*       rather relies on Hercules to have set that up before        */
/*       invoking this program.                                      */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"
#include "hercules.h"

#define PGMNAME  "conspawn"

int main(int argc, char* argv[])
{
    int i, rc;
    size_t k;
    char* p;

#ifdef _MSVC_
#pragma comment(lib,"shell32.lib")   // (need ShellExecute)
    // --------------------------------------------------------
    // PROGRAMMING NOTE: We MUST use "ShellExecute" for Windows
    // GUI programs since: 1) GUI programs don't use stdio,
    // 2) they never exit until the user manually closes them.
    //
    // Erroneously using the 'system()' function to start a GUI
    // program causes HercGUI to hang at PowerOff as it waits
    // for its child process to close its stdio handles which
    // GUI programs never do until they exit (which they never
    // do until the user manually closes them).
    //
    // The reason this phenomenon occurs even though Hercules
    // does indeed close ITS OWN stdio handles when it ends is
    // because its child processes that it creates ALSO inherit
    // the same stdio handles! (I.e. the GUI programs that Herc
    // starts end up never closing "Herc's" [inherited] stdio
    // handles, which are the same handles that HercGUI waits
    // on! Thus GUI programs started by Herc using the 'system'
    // API end up hanging HercGUI! (during Herc PowerOff))
    //
    // Thus, for GUI apps, we MUST use "ShellExecute" here
    // to prevent HercGUI from hanging at PowerOff. Also note
    // that this hang obviously does not occur when Hercules
    // is run in command-line (non-HercGUI) mode even when
    // the 'system()' API is erroneously used, since Herc is
    // obviously (duh!) not being run under the control of an
    // external GUI in such a situation.   -- Fish, Aug. 2006
    // --------------------------------------------------------

    if (argc >= 2 && strcasecmp(argv[1],"startgui") == 0)
    {
        ////////////////////////////////////////////////////////
        // Windows GUI program; no stdio; use 'ShellExecute'...
        ////////////////////////////////////////////////////////

        // REFERENCE: upon entry:

        //   argv[0]       "conspawn"
        //   argv[1]       "startgui"
        //   argv[2]       (program to start)
        //   argv[3..n]    (arguments for program)

        HWND     hwnd          = NULL;
        LPCTSTR  lpOperation   = NULL;
        LPCTSTR  lpFile        = argv[2];
        LPCTSTR  lpParameters  = NULL;
        LPCTSTR  lpDirectory   = NULL;
        INT      nShowCmd      = SW_SHOWNORMAL;
        char*    pszErrMsg     = NULL;

        // Build arguments string from passed args...

        for (i=3, k=0; i < argc; i++)
        {
            k += strlen(argv[i]) + 1;
            if (strchr(argv[i],' ')) k += 2;
        }

        if (k)
        {
            // (allocate room for arguments string)

            if (!(p = malloc(sizeof(char)*k)))
            {
                errno = ENOMEM;
                perror( PGMNAME );
                return -1;
            }
            *p = 0;

            // (build arguments string from args)

            for (i=3; i < argc; ++i)
            {
                if (strchr(argv[i],' ')) strcat(p,"\"");
                strcat(p,argv[i]);
                if (strchr(argv[i],' ')) strcat(p,"\"");
                if (i != (argc-1)) strcat(p," ");
            }

            lpParameters = p;
        }
        else
            p = NULL;

        rc = (intptr_t) ShellExecute( hwnd, lpOperation, lpFile, lpParameters, lpDirectory, nShowCmd );

        if (p)
            free(p);

        if ( rc > 32)
            return 0;       // rc > greater than 32 == success...

        // rc <= less than or equal 32 == error...

        switch (rc)
        {
            case 0:                      pszErrMsg = "The operating system is out of memory or resources"; break;
            case SE_ERR_FNF:             pszErrMsg = "The specified file was not found"; break;
            case SE_ERR_PNF:             pszErrMsg = "The specified path was not found"; break;
            case SE_ERR_ACCESSDENIED:    pszErrMsg = "The operating system denied access to the specified file"; break;
            case ERROR_BAD_FORMAT:       pszErrMsg = "The .exe file is invalid (non-Microsoft Win32 .exe or error in .exe image)"; break;
            case SE_ERR_OOM:             pszErrMsg = "There was not enough memory to complete the operation"; break;
            case SE_ERR_DLLNOTFOUND:     pszErrMsg = "The specified dynamic-link library (DLL) was not found"; break;
            case SE_ERR_SHARE:           pszErrMsg = "A sharing violation occurred"; break;
            case SE_ERR_ASSOCINCOMPLETE: pszErrMsg = "The file name association is incomplete or invalid"; break;
            case SE_ERR_DDETIMEOUT:      pszErrMsg = "The DDE transaction could not be completed because the request timed out"; break;
            case SE_ERR_DDEFAIL:         pszErrMsg = "The DDE transaction failed"; break;
            case SE_ERR_DDEBUSY:         pszErrMsg = "The Dynamic Data Exchange (DDE) transaction could not be completed because other DDE transactions were being processed"; break;
            case SE_ERR_NOASSOC:         pszErrMsg = "There is no application associated with the given file name extension. This error will also be returned if you attempt to print a file that is not printable"; break;

            default:
                printf(PGMNAME": ShellExecute(\"%s\", \"%s\",...) failed: Unknown error; rc=%d (0x%8.8X).\n",
                    lpFile, lpParameters, rc, rc );
                return -1;
        }

        printf( PGMNAME": ShellExecute(\"%s\", \"%s\",...) failed: %s.\n",
            lpFile, lpParameters, pszErrMsg );

        return -1;
    }
#endif // _MSVC_

    ////////////////////////////////////////////////////////
    // Command line program using stdio; use 'system()'...
    ////////////////////////////////////////////////////////

    // Re-build a complete command line from passed args...

    for (i=1, k=0; i < argc; i++)
    {
        k += strlen(argv[i]) + 1;
        if (strchr(argv[i],' ')) k += 2;
    }

    if (!k)
    {
        errno = EINVAL;
        perror( PGMNAME );
        printf( PGMNAME": Usage: command [args]\n");
        return -1;
    }

    // (allocate room for command line)

    if (!(p = malloc(sizeof(char)*k)))
    {
        errno = ENOMEM;
        perror( PGMNAME );
        return -1;
    }
    *p = 0;

    // (rebuild command-line from args)

    for (i=1; i < argc; ++i)
    {
        if (strchr(argv[i],' ')) strcat(p,"\"");
        strcat(p,argv[i]);
        if (strchr(argv[i],' ')) strcat(p,"\"");
        if (i != (argc-1)) strcat(p," ");
    }

    // Ask system() to process command line...

    // NOTE: the below call WILL NOT RETURN
    // until the program being called exits!

    rc = system(p);

    free(p);

    // --------------------------------------------------------
    // PROGRAMMING NOTE: only rc == -1 need be reported since,
    // if the command interpreter called a batch/cmd file for
    // example, it could have set its own custom return code.
    //
    // Only -1 means the system() call itself failed, which
    // is the only thing that actually needs to be reported.
    // --------------------------------------------------------

    if ( -1 == rc )
        perror( PGMNAME );

    return rc;
}
