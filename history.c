/* HISTORY.C    (C) Copyright Roger Bowler, 1999-2012                */
/*               Hercules Command History Processes                  */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#include "hstdinc.h"

#include "hercules.h"
#include "history.h"

/*-------------------------------------------------------------------*/
/*                      HISTORY structure                            */
/*-------------------------------------------------------------------*/
typedef struct HISTORY  HISTORY;
struct HISTORY
{
  int       number;
  char*     cmdline;
  HISTORY*  prev;
  HISTORY*  next;
};

/*-------------------------------------------------------------------*/
/*                     Global variables                              */
/*-------------------------------------------------------------------*/
HISTORY*  history_lines     = NULL; /* list begin (oldest cmd)       */
HISTORY*  history_lines_end = NULL; /* list end (most recent cmd)    */
HISTORY*  history_ptr       = NULL; /* last key press retrieved cmd  */
HISTORY*  backup            = NULL; /* saved last removed cmd        */
int       history_count     = 0;    /* for line numbering            */

/*-------------------------------------------------------------------*/
/*                     Public variables                              */
/*-------------------------------------------------------------------*/
/* used in panel.c to see if there was history command requested     */
/* and returns that command                                          */
/*-------------------------------------------------------------------*/
char*   historyCmdLine     = NULL;
int     history_requested  = 0;

/*-------------------------------------------------------------------*/
/*                   copy_to_historyCmdLine                          */
/*-------------------------------------------------------------------*/
void copy_to_historyCmdLine( char* cmdline )
{
    size_t size = strlen( cmdline ) + 1;
    free( historyCmdLine );
    historyCmdLine = malloc( size );
    strlcpy( historyCmdLine, cmdline, size );
}

/*-------------------------------------------------------------------*/
/*           history_init   -   initialize environment               */
/*-------------------------------------------------------------------*/
int history_init()
{
    history_lines      = NULL;
    history_lines_end  = NULL;
    history_ptr        = NULL;
    backup             = NULL;
    history_count      = 0;

    historyCmdLine     = (char*) malloc( CMD_SIZE + 1 );
    history_requested  = 0;
    return 0;
}

/*-------------------------------------------------------------------*/
/*           history_add   -   add commandline to history list       */
/*-------------------------------------------------------------------*/
int history_add( char* cmdline )
{
    HISTORY*  tmp;
    size_t    size;

    /* if there is some backup line remaining, remove it */
    if (backup)
    {
        free( backup->cmdline );
        free( backup );
        backup = NULL;
    }

    /* If last line is exactly the same as this line
       ignore and return to caller
    */
    if (history_lines && !strcmp( cmdline, history_lines_end->cmdline ))
    {
        history_ptr = NULL;
        return 0;
    }

    /* Allocate space and copy string */
    tmp  = (HISTORY*) malloc( sizeof( HISTORY ));
    size = strlen( cmdline ) + 1;
    tmp->cmdline = (char*) malloc(size);
    strlcpy( tmp->cmdline, cmdline, size );
    tmp->next = NULL;
    tmp->prev = NULL;
    tmp->number = ++history_count;

    if (!history_lines)
    {
        /* first in list */
        history_lines     = tmp;
        history_lines_end = tmp;
    }
    else
    {
        tmp->prev               = history_lines_end;
        history_lines_end->next = tmp;
        history_lines_end       = tmp;
    }

    history_ptr = NULL;

    if (history_count > HISTORY_MAX)
    {
        /* If we are over maximum number of lines in history list,
           oldest one should be deleted, but we don't know whether
           history_remove will be called or not, so the oldest line
           is just saved instead, to be deleted on the next call.
        */
        backup              = history_lines;
        history_lines       = history_lines->next;
        backup->next        = NULL;
        history_lines->prev = NULL;
    }
    return 0;
}

/*-------------------------------------------------------------------*/
/*                         history_remove                            */
/*-------------------------------------------------------------------*/
/* Remove last line from history list.                               */
/* Called only when history command is invoked.                      */
/*-------------------------------------------------------------------*/
int history_remove()
{
    HISTORY* tmp;

    if (!history_lines)
        return 0;

    if (history_lines == history_lines_end)
    {
        ASSERT( !history_lines->next );
        ASSERT( history_count == 1 );

        free( history_lines->cmdline );
        free( history_lines );

        history_lines     = NULL;
        history_lines_end = NULL;

        history_count--;

        return 0;
    }

    tmp = history_lines_end->prev;
    tmp->next = NULL;

    free( history_lines_end->cmdline );
    free( history_lines_end );

    history_count--;
    history_lines_end = tmp;

    if (backup)
    {
        backup->next = history_lines;
        history_lines->prev = backup;
        history_lines = backup;
        backup = NULL;
    }
    return 0;
}

/*-------------------------------------------------------------------*/
/*                          history_next                             */
/*-------------------------------------------------------------------*/
int history_next()
{
    if (!history_ptr)
    {
        if (!(history_ptr = history_lines_end))
            return -1;
        copy_to_historyCmdLine( history_ptr->cmdline );
        return 0;
    }

    if (!history_ptr->next) history_ptr = history_lines;
    else                    history_ptr = history_ptr->next;

    copy_to_historyCmdLine( history_ptr->cmdline );
    return 0;
}

/*-------------------------------------------------------------------*/
/*                          history_prev                             */
/*-------------------------------------------------------------------*/
int history_prev()
{
    if (!history_ptr)
    {
        if (!(history_ptr = history_lines_end))
            return -1;
        copy_to_historyCmdLine( history_ptr->cmdline );
        return 0;
    }

    if (!history_ptr->prev) history_ptr = history_lines_end;
    else                    history_ptr = history_ptr->prev;

    copy_to_historyCmdLine( history_ptr->cmdline );
    return 0;
}

/*-------------------------------------------------------------------*/
/*                     history_relative_line                         */
/*-------------------------------------------------------------------*/
int history_relative_line( int rline )
{
    HISTORY* tmp = history_lines_end;
    char buf[80];

    if (-rline > HISTORY_MAX)
    {
        MSGBUF( buf, "History limited to last %d commands", HISTORY_MAX );
        // "%s"
        WRMSG( HHC02293, "E", buf );
        return -1;
    }

    if (-rline > history_count)
    {
        MSGBUF( buf, "Only %d commands in history", history_count );
        // "%s"
        WRMSG( HHC02293, "E", buf );
        return -1;
    }

    while (rline < -1)
    {
        tmp = tmp->prev;
        rline++;
    }

    copy_to_historyCmdLine( tmp->cmdline );

    history_ptr = NULL;
    return 0;
}

/*-------------------------------------------------------------------*/
/*                     history_absolute_line                         */
/*-------------------------------------------------------------------*/
int history_absolute_line( int aline )
{
    HISTORY* tmp = history_lines_end;
    int lowlimit;
    char buf[80];

    if (!history_count)
    {
        // "%s"
        WRMSG( HHC02293, "E", "History empty" );
        return -1;
    }

    lowlimit = (history_count - HISTORY_MAX);

    if (aline > history_count || aline <= lowlimit)
    {
        MSGBUF( buf, "Only commands %d-%d are in history",
            lowlimit < 0 ? 1 : lowlimit + 1, history_count );
        // "%s"
        WRMSG( HHC02293, "E", buf );
        return -1;
    }

    while (tmp->number != aline)
        tmp = tmp->prev;

    copy_to_historyCmdLine( tmp->cmdline );

    history_ptr = NULL;
    return 0;
}

/*-------------------------------------------------------------------*/
/*           history_show   -   show list of lines in history        */
/*-------------------------------------------------------------------*/
int history_show()
{
    HISTORY* tmp = history_lines;

    while (tmp)
    {
        // "Index %3d: %s"
        WRMSG( HHC02273, "I", tmp->number, tmp->cmdline );
        tmp = tmp->next;
    }
    return 0;
}
