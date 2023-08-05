/* HAO.C        (C) Copyright Bernard van der Helm, 2002-2012        */
/*             Hercules Automatic Operator Implementation            */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*---------------------------------------------------------------------------*/
/* file: hao.c                                                               */
/*                                                                           */
/* Implementation of the automatic operator within the Hercules emulator.    */
/*                                                                           */
/*                            (C) Copyright Bernard van der Helm, 2002-2011  */
/*                            Noordwijkerhout, The Netherlands.              */
/*---------------------------------------------------------------------------*/

#include "hstdinc.h"

#define _HAO_C_
#define _HENGINE_DLL_

#include "hercules.h"

#if defined(OPTION_HAO)

/*---------------------------------------------------------------------------*/
/* constants                                                                 */
/*---------------------------------------------------------------------------*/
#define HAO_WKLEN    256    /* (maximum message length able to tolerate) */
#define HAO_MAXRULE  64     /* (purely arbitrary and easily increasable) */
#define HAO_MAXCAPT  9      /* (maximum number of capturing groups)      */

/*---------------------------------------------------------------------------*/
/* local variables                                                           */
/*---------------------------------------------------------------------------*/
static TID      haotid;                         /* Herc Auto-Oper thread-id  */
static LOCK     ao_lock;
static regex_t  ao_preg[HAO_MAXRULE];
static char    *ao_cmd[HAO_MAXRULE];
static char    *ao_tgt[HAO_MAXRULE];
static char     ao_msgbuf[LOG_DEFSIZE+1];   /* (plus+1 for NULL termination) */

/*---------------------------------------------------------------------------*/
/* function prototypes                                                       */
/*---------------------------------------------------------------------------*/
DLL_EXPORT void  hao_command(char *cmd);
static     int   hao_initialize();
static     void  hao_message(char *buf);
static     void  hao_clear();
static     void  hao_cmd(char *arg);
static     void  hao_cpstrp(char *dest, char *src);
static     void  hao_del(char *arg);
static     void  hao_list(char *arg);
static     void  hao_tgt(char *arg);
static     void* hao_thread(void* dummy);

/*---------------------------------------------------------------------------*/
/* void hao_initialize(void)                                                 */
/*                                                                           */
/* This function is called by the 'hao_command()' function. It initializes   */
/* all global variables. PLEASE NOTE: This is a BOOLEAN function, returning  */
/* TRUE or FALSE indicating success or failure.                              */
/*---------------------------------------------------------------------------*/
static int hao_initialize()
{
    static int already_did_this = FALSE;
    static int rc;
    int i = 0;

    /* PROGRAMMING NOTE: this is a ONE TIME initialization function.
     * If initialization fails for any reason we DO NOT try again.
     */
    if (already_did_this)       /* if we already attempted this, then */
        return rc;              /* return same return code as before. */
    already_did_this = TRUE;    /* So we don't try doing this again.  */

    initialize_lock( &ao_lock );

    /* serialize */
    obtain_lock( &ao_lock );

    /* initialize variables */
    for(i=0; i < HAO_MAXRULE; i++)
    {
        ao_cmd[i] = NULL;
        ao_tgt[i] = NULL;
    }

    /* initialize message buffer */
    memset( ao_msgbuf, 0, sizeof( ao_msgbuf ));

    /* Start message monitoring thread */
    rc = create_thread( &haotid, JOINABLE, hao_thread, NULL, HAO_THREAD_NAME );
    if (rc)
    {
        rc = FALSE;
        // "Error in function create_thread(): %s"
        WRMSG( HHC00102, "E", strerror( rc ));
        haotid = 0;
    }
    else
    {
        rc = TRUE;
        ASSERT( haotid != 0 );
    }

    release_lock( &ao_lock );

    return rc; /* TRUE/FALSE */
}

/*---------------------------------------------------------------------------*/
/* void hao_command(char *cmd)                                               */
/*                                                                           */
/* Within panel this function is called when a command is given that starts  */
/* with the string hao. Here we check if a correct hao command is requested, */
/* otherwise a help is printed.                                              */
/*---------------------------------------------------------------------------*/
DLL_EXPORT void hao_command(char *cmd)
{
    char work[HAO_WKLEN];
    char work2[HAO_WKLEN];

    /* Perform one-time initialization */
    if (!hao_initialize())
    {
        static int didthis = FALSE;
        if (!didthis)
        {
            didthis = TRUE;
            // "Could not create the Automatic Operator thread"
            WRMSG(HHC01404, "S");
        }
    }

    /* copy and strip spaces */
    hao_cpstrp(work, cmd);

    /* again without starting hao */
    hao_cpstrp(work2, &work[3]);

    if(!strncasecmp(work2, "tgt", 3))
    {
        /* again without starting tgt */
        hao_cpstrp(work, &work2[3]);
        hao_tgt(work);
        return;
    }

    if(!strncasecmp(work2, "cmd", 3))
    {
        /* again without starting cmd */
        hao_cpstrp(work, &work2[3]);
        hao_cmd(work);
        return;
    }

    if(!strncasecmp(work2, "del", 3))
    {
        /* again without starting del */
        hao_cpstrp(work, &work2[3]);
        hao_del(work);
        return;
    }

    if(!strncasecmp(work2, "list", 4))
    {
        /* again without starting list */
        hao_cpstrp(work, &work2[4]);
        hao_list(work);
        return;
    }

    if(!strncasecmp(work2, "clear", 4))
    {
        hao_clear();
        return;
    }

    // "Unknown hao command, valid commands are: ..."
    WRMSG(HHC00070, "E");
}

/*---------------------------------------------------------------------------*/
/* hao_cpstrp(char *dest, char *src)                                         */
/*                                                                           */
/* This function copies the string from src to dest, without the leading     */
/* or trailing spaces.                                                       */
/*---------------------------------------------------------------------------*/
static void hao_cpstrp(char *dest, char *src)
{
    int i;

    /* Find first non space */
    for(i = 0; src[i] == ' '; i++);
    /* Check if we've hit the wall */
    if(!src[i])
    {
        /* Just blanks or empty string */
        dest[0]=0;
        return;
    }
    /* Copy from that point but no more than our work area */
    /* No need to copy more than HAO_WKLEN-1 since the last character */
    /* will be set to 0 anyway */
    strncpy(dest, &src[i], HAO_WKLEN-1);
    /* Ensure there is a trailing 0 */
    dest[HAO_WKLEN-1] = 0;
    /* Find the last non space character from the end */
    for(i = (int)strlen(dest); i && dest[i - 1] == ' '; i--);
    /* Terminate the string at the last space or empty string */
    dest[i] = 0;
}

/*---------------------------------------------------------------------------*/
/* void hao_tgt(char *arg)                                                   */
/*                                                                           */
/* This function is given when the hao tgt command is given. A free slot is  */
/* to be found and filled with the rule. There will be loop checking.        */
/*---------------------------------------------------------------------------*/
static void hao_tgt(char *arg)
{
    int i;
    int j;
    int rc;
    char work[HAO_WKLEN];

    /* serialize */
    obtain_lock(&ao_lock);

    /* find a free slot */
    for(i = 0; ao_tgt[i] && i < HAO_MAXRULE; i++)

    /* check for table full */
    if(i == HAO_MAXRULE)
    {
        release_lock(&ao_lock);
        // "The %s was not added because table is full; table size is %02d"
        WRMSG(HHC00071, "E", "target", HAO_MAXRULE);
        return;
    }

    /* check if not command is expected */
    for(j = 0; j < HAO_MAXRULE; j++)
    {
        if(ao_tgt[j] && !ao_cmd[j])
        {
            release_lock(&ao_lock);
            // "The command %s given, but the command %s was expected"
            WRMSG(HHC00072, "E", "tgt", "cmd");
            return;
        }
    }

    /* check for empty target */
    if(!strlen(arg))
    {
        release_lock(&ao_lock);
        // "Empty %s specified"
        WRMSG(HHC00073, "E", "target");
        return;
    }

    /* check for duplicate targets */
    for(j = 0; j < HAO_MAXRULE; j++)
    {
        if(ao_tgt[j] && !strcmp(arg, ao_tgt[j]))
        {
            release_lock(&ao_lock);
            // "The target was not added because a duplicate was found in the table at %02d"
            WRMSG(HHC00074, "E", j);
            return;
        }
    }

    /* compile the target string */
    rc = regcomp(&ao_preg[i], arg, REG_EXTENDED);

    /* check for error */
    if(rc)
    {
        release_lock(&ao_lock);

        /* place error in work */
        regerror(rc, (const regex_t *) &ao_preg[i], work, HAO_WKLEN);
        // "Error in function %s: %s"
        WRMSG(HHC00075, "E", "regcomp()", work);
        return;
    }

    /* check for possible loop */
    for(j = 0; j < HAO_MAXRULE; j++)
    {
        if(ao_cmd[j] && !regexec(&ao_preg[i], ao_cmd[j], 0, NULL, 0))
        {
            release_lock(&ao_lock);
            regfree(&ao_preg[i]);
            // "The %s was not added because it causes a loop with the %s at index %02d"
            WRMSG(HHC00076, "E", "target", "command", i);
            return;
        }
    }

    /* duplicate the target */
    ao_tgt[i] = strdup(arg);

    /* check duplication */
    if(!ao_tgt[i])
    {
        release_lock(&ao_lock);
        regfree(&ao_preg[i]);
        // "Error in function %s: %s"
        WRMSG(HHC00075, "E", "strdup()", strerror(ENOMEM));
        return;
    }

    release_lock(&ao_lock);

    // "The %s was placed at index %d"
    WRMSG(HHC00077, "I", "target", i);
}

/*---------------------------------------------------------------------------*/
/* void hao_cmd(char *arg)                                                   */
/*                                                                           */
/* This function is called when the hao cmd command is given. It searches    */
/* the index of the last given hao tgt command. Does some checking and fills */
/* the entry with the given command. There will be loop checking             */
/*---------------------------------------------------------------------------*/
static void hao_cmd(char *arg)
{
    int i;
    int j;

    /* serialize */
    obtain_lock(&ao_lock);

    /* find the free slot */
    for(i = 0; ao_cmd[i] && i < HAO_MAXRULE; i++);

    /* check for table full -> so tgt cmd expected */
    if(i == HAO_MAXRULE)
    {
        release_lock(&ao_lock);
        // "The %s was not added because table is full; table size is %02d"
        WRMSG(HHC00071, "E", "command", HAO_MAXRULE);
        return;
    }

    /* check if target is given */
    if(!ao_tgt[i])
    {
        release_lock(&ao_lock);
        // "The command %s given, but the command %s was expected"
        WRMSG(HHC00072, "E", "cmd", "tgt");
        return;
    }

    /* check for empty cmd string */
    if(!strlen(arg))
    {
        release_lock(&ao_lock);
        // "Empty %s specified"
        WRMSG(HHC00073, "E", "command");
        return;
    }

    /* check for hao command, prevent deadlock */
    for(j = 0; !strncasecmp(&arg[j], "herc ", 4); j += 5);
    if(!strcasecmp(&arg[j], "hao") || !strncasecmp(&arg[j], "hao ", 4))
    {
        release_lock(&ao_lock);
        // "The command was not added because it may cause dead locks"
        WRMSG(HHC00078, "E");
        return;
    }

    /* check for possible loop */
    for(j = 0; j < HAO_MAXRULE; j++)
    {
        if(ao_tgt[j] && !regexec(&ao_preg[j], arg, 0, NULL, 0))
        {
            release_lock(&ao_lock);
            // "The %s was not added because it causes a loop with the %s at index %02d"
            WRMSG(HHC00076, "E", "command", "target", j);
            return;
        }
    }

    /* duplicate the string */
    ao_cmd[i] = strdup(arg);

    /* check duplication */
    if(!ao_cmd[i])
    {
        release_lock(&ao_lock);
        // "Error in function %s: %s"
        WRMSG(HHC00075, "E", "strdup()", strerror(ENOMEM));
        return;
    }

    release_lock(&ao_lock);

    // "The %s was placed at index %d"
    WRMSG(HHC00077, "I", "command", i);
}

/*---------------------------------------------------------------------------*/
/* void hao_del(char *arg)                                                   */
/*                                                                           */
/* This function is called when the command hao del is given. the rule in    */
/* the given index is cleared.                                               */
/*---------------------------------------------------------------------------*/
static void hao_del(char *arg)
{
    int i;
    int rc;

    /* read the index number to delete */
    rc = sscanf(arg, "%d", &i);
    if(!rc || rc == -1)
    {
        // "The command 'del' was given without a valid index"
        WRMSG(HHC00083, "E");
        return;
    }

    /* check if index is valid */
    if(i < 0 || i >= HAO_MAXRULE)
    {
        // "Invalid index; index must be between 0 and %02d"
        WRMSG(HHC00084, "E", HAO_MAXRULE - 1);
        return;
    }

    /* serialize */
    obtain_lock(&ao_lock);

    /* check if entry exists */
    if(!ao_tgt[i])
    {
        release_lock(&ao_lock);
        // "Rule at index %d not deleted, already empty"
        WRMSG(HHC00085, "E", i);
        return;
    }

    /* delete the entry */
    free(ao_tgt[i]);
    ao_tgt[i] = NULL;
    regfree(&ao_preg[i]);

    if(ao_cmd[i])
    {
        free(ao_cmd[i]);
        ao_cmd[i] = NULL;
    }

    release_lock(&ao_lock);

    // "Rule at index %d successfully deleted"
    WRMSG(HHC00086, "I", i);
}

/*---------------------------------------------------------------------------*/
/* void hao_list(char *arg)                                                  */
/*                                                                           */
/* this function is called when the hao list command is given. It lists all  */
/* rules. When given an index, only that index will be showed.               */
/*---------------------------------------------------------------------------*/
static void hao_list(char *arg)
{
    int i;
    int rc;
    int size;

    rc = sscanf(arg, "%d", &i);
    if(!rc || rc == -1)
    {
        /* list all rules */
        size = 0;

        /* serialize */
        obtain_lock(&ao_lock);

        for(i = 0; i < HAO_MAXRULE; i++)
        {
            if(ao_tgt[i])
            {
                if(!size)
                {
                    // "The defined Hercules Automatic Operator rule(s) are:"
                    WRMSG(HHC00087, "I");
                }
                // "Index %02d: target %s -> command %s"
                WRMSG(HHC00088, "I", i, ao_tgt[i], (ao_cmd[i] ? ao_cmd[i] : "not specified"));
                size++;
            }
        }
        release_lock(&ao_lock);

        if(!size)
        {
            // "The are no HAO rules defined"
            WRMSG(HHC00089, "I");
        }
        else
        {
            // "%d rule(s) displayed"
            WRMSG(HHC00082, "I", size);
        }
    }
    else
    {
        /* list specific index */
        if(i < 0 || i >= HAO_MAXRULE)
        {
            // "Invalid index; index must be between 0 and %02d"
            WRMSG(HHC00084, "E", HAO_MAXRULE - 1);
        }
        else
        {
            /* serialize */
            obtain_lock(&ao_lock);

            if(!ao_tgt[i])
            {
                // "No rule defined at index %02d"
                WRMSG(HHC00079, "E", i);
            }
            else
            {
                // "Index %02d: target %s -> command %s"
                WRMSG(HHC00088, "I", i, ao_tgt[i], (ao_cmd[i] ? ao_cmd[i] : "not specified"));
            }

            release_lock(&ao_lock);
        }
    }
}

/*---------------------------------------------------------------------------*/
/* void hao_clear(void)                                                      */
/*                                                                           */
/* This function is called when the hao clear command is given. This         */
/* function just clears all defined rules. Handy command for panic           */
/* situations.                                                               */
/*---------------------------------------------------------------------------*/
static void hao_clear(void)
{
    int i;

    /* serialize */
    obtain_lock(&ao_lock);

    /* clear all defined rules */
    for(i = 0; i < HAO_MAXRULE; i++)
    {
        if(ao_tgt[i])
        {
            free(ao_tgt[i]);
            ao_tgt[i] = NULL;
            regfree(&ao_preg[i]);
        }
        if(ao_cmd[i])
        {
            free(ao_cmd[i]);
            ao_cmd[i] = NULL;
        }
    }

    release_lock(&ao_lock);

    // "All HAO rules are cleared"
    WRMSG(HHC00080, "I");
}

/*---------------------------------------------------------------------------*/
/* void* hao_thread(void* dummy)                                             */
/*                                                                           */
/* This thread is created by hao_initialize. It examines every message       */
/* printed. Here we check if a rule applies to the message. If so we fire    */
/* the command within the rule.                                              */
/*---------------------------------------------------------------------------*/
static void* hao_thread(void* dummy)
{
    static int did_waiting_msg = FALSE;
    char*  msgbuf  = NULL;
    int    msgidx  = -1;
    int    msgamt  = 0;
    char*  msgend  = NULL;
    char   svchar  = 0;
    int    bufamt  = 0;

    UNREFERENCED(dummy);

    // "Thread id "TIDPAT", prio %d, name '%s' started"
    LOG_THREAD_BEGIN( HAO_THREAD_NAME  );

    /* PROGRAMMING NOTE: because we are dependent on the logger thread (to
     * feed us log messages) we must NOT proceed until the logger facility
     * becomes active.
     */
    // ZZ FIXME: The below is an WORKAROUND that needs redesigned.
    // It works for now but is NOT the proper way to accomplish this.
    while (!sysblk.shutdown && !logger_isactive())
    {
        if (!did_waiting_msg)
        {
            did_waiting_msg = TRUE;
            // "HAO thread waiting for logger facility to become active"
            WRMSG( HHC00090, "W" );
        }
        USLEEP( 50 * 1000 );    /* (wait for a bit) */
    }

    if (!sysblk.shutdown && did_waiting_msg)
    {
        // "Logger facility now active; HAO thread proceeding"
        WRMSG( HHC00091, "I" );
        did_waiting_msg = FALSE;
    }

    /* Do until shutdown */
    while(!sysblk.shutdown && msgamt >= 0)
    {
        /* wait for message data */
        msgamt = log_read(&msgbuf, &msgidx, LOG_BLOCK);
        if(msgamt > 0)
        {
            /* append to existing data */
            if (msgamt > (int)((sizeof(ao_msgbuf) - 1) - bufamt))
                msgamt = (int)((sizeof(ao_msgbuf) - 1) - bufamt);

            strncpy( &ao_msgbuf[bufamt], msgbuf, msgamt );
            ao_msgbuf[bufamt += msgamt] = 0;
            msgbuf = ao_msgbuf;

            /* process only complete messages */
            msgend = strchr(msgbuf,'\n');
            while(msgend)
            {
                /* null terminate message */
                svchar = *(msgend+1);
                *(msgend+1) = 0;

                /* process message */
                hao_message(msgbuf);

                /* restore destroyed byte */
                *(msgend+1) = svchar;
                msgbuf = msgend+1;
                msgend = strchr(msgbuf,'\n');
            }

            /* shift message buffer */
            memmove( ao_msgbuf, msgbuf, bufamt -= (msgbuf - ao_msgbuf) );
        }
    }

    // "Thread id "TIDPAT", prio %d, name '%s' ended"
    LOG_THREAD_END( HAO_THREAD_NAME  );
    return NULL;
}

/*---------------------------------------------------------------------------*/
/* int hao_ignoremsg(char *msg)                                              */
/*                                                                           */
/* This function determines whether the message being processed should be    */
/* ignored or not (so we don't react to it).  Returns 1 (TRUE) if message    */
/* should be ignored, or 0 == FALSE if message should be processed.          */
/*---------------------------------------------------------------------------*/
static int hao_ignoremsg(const char *msg)
{
    int msglen;
    const char* msgnum;

    msglen = strlen( msg );

    if (MLVL( DEBUG ) && msglen >= MLVL_DEBUG_PFXIDX)
    {
        msgnum = msg + MLVL_DEBUG_PFXIDX;
        msglen = strlen( msgnum );
    }
    else
        msgnum = msg;

    /* Ignore our own messages (HHC0007xx, HHC0008xx and HHC0009xx
       are reserved so that hao.c can recognize its own messages) */

    if (0
        || !strncasecmp( msgnum, "HHC0007", 7 )
        || !strncasecmp( msgnum, "HHC0008", 7 )
        || !strncasecmp( msgnum, "HHC0009", 7 )
    )
        return TRUE;  /* (it's one of our hao messages; ignore it) */

    /* To be extra safe, ignore any messages with the string "hao" in them */
    if (0
        || !strncasecmp( msgnum, "HHC00013I Herc command: 'hao ",      29 )
        || !strncasecmp( msgnum, "HHC00013I Herc command: 'herc hao ", 34 )
        || !strncasecmp( msgnum, "HHC01603I hao ",                     14 )
        || !strncasecmp( msgnum, "HHC01603I herc hao ",                19 )
    )
        return TRUE;  /* (it's one of our hao messages; ignore it) */

    /* Same idea but for messages logged as coming from the .rc file */
    if (!strncasecmp( msgnum, "> hao ", 6 ))
        return TRUE;

    return FALSE;   /* (message appears to be one we should process) */
}

/*---------------------------------------------------------------------------*/
/* size_t hao_subst(char *str, size_t soff, size_t eoff,                     */
/*              char *cmd, size_t coff, size_t csize)                        */
/*                                                                           */
/* This function copies a substring of the original string into              */
/* the command buffer. The input parameters are:                             */
/*      str     the original string                                          */
/*      soff    starting offset of the substring in the original string      */
/*      eoff    offset of the first character following the substring        */
/*      cmd     the destination command buffer                               */
/*      coff    offset in the command buffer to copy the substring           */
/*      csize   size of the command buffer (including terminating zero)      */
/* The return value is the number of characters copied.                      */
/*---------------------------------------------------------------------------*/
static size_t hao_subst(char *str, size_t soff, size_t eoff,
        char *cmd, size_t coff, size_t csize)
{
    size_t len = eoff - soff;

    if (soff + len > strlen(str))
        len = strlen(str) - soff;

    if (coff + len > csize-1)
        len = csize-1 - coff;

    memcpy(cmd + coff, str + soff, len);
    return len;
}

/*---------------------------------------------------------------------------*/
/* void hao_message(char *buf)                                               */
/*                                                                           */
/* This function is called by hao_thread whenever a message is about to be   */
/* printed. Here we check if a rule applies to the message. If so we fire    */
/* the command within the rule.                                              */
/*---------------------------------------------------------------------------*/
static void hao_message(char *buf)
{
    char work[HAO_WKLEN];
    char cmd[HAO_WKLEN];
    regmatch_t rm[HAO_MAXCAPT+1];
    int i, j, k, numcapt;
    size_t n;
    char *p;

    /* copy and strip spaces */
    hao_cpstrp(work, buf);

    /* strip the herc prefix */
    while(!strncmp(work, "herc", 4))
        hao_cpstrp(work, &work[4]);

    /* Ignore the message if we should (e.g. if one of our own!) */
    if (hao_ignoremsg( work ))
    return;

    /* serialize */
    obtain_lock(&ao_lock);

    /* check all defined rules */
    for(i = 0; i < HAO_MAXRULE; i++)
    {
        if(ao_tgt[i] && ao_cmd[i])  /* complete rule defined in this slot? */
        {
            /* does this rule match our message? */
            if (regexec(&ao_preg[i], work, HAO_MAXCAPT+1, rm, 0) == 0)
            {
                /* count the capturing group matches */
                for (j = 0; j <= HAO_MAXCAPT && rm[j].rm_so >= 0; j++);
                numcapt = j - 1;

                /* copy the command and process replacement patterns */
                for (n=0, p=ao_cmd[i]; *p && n < sizeof(cmd)-1; )
                {
                    /* replace $$ by $ */
                    if (*p == '$' && p[1] == '$')
                    {
                        cmd[n++] = '$';
                        p += 2;
                        continue;
                    }
                    /* replace $` by characters to the left of the match */
                    if (*p == '$' && p[1] == '`')
                    {
                        n += hao_subst(work, 0, rm[0].rm_so, cmd, n, sizeof(cmd));
                        p += 2;
                        continue;
                    }
                    /* replace $' by characters to the right of the match */
                    if (*p == '$' && p[1] == '\'')
                    {
                        n += hao_subst(work, rm[0].rm_eo, strlen(work), cmd, n, sizeof(cmd));
                        p += 2;
                        continue;
                    }
                    /* replace $1..$99 by the corresponding capturing group */
                    if (*p == '$' && isdigit((unsigned char)p[1]))
                    {
                        if (isdigit((unsigned char)p[2]))
                        {
                            j = (p[1]-'0') * 10 + (p[2]-'0');
                            k = 3;
                        }
                        else
                        {
                            j = p[1]-'0';
                            k = 2;
                        }
                        if (j > 0 && j <= numcapt)
                        {
                            n += hao_subst(work, rm[j].rm_so, rm[j].rm_eo, cmd, n, sizeof(cmd));
                            p += k;
                            continue;
                        }
                    }
                    /* otherwise copy one character */
                    cmd[n++] = *p++;
                }
                cmd[n] = '\0';

                // "Match at index %02d, executing command %s"
                WRMSG(HHC00081, "I", i, cmd);
                panel_command(cmd);
            }
        }
    }
    release_lock(&ao_lock);
}

#endif /* defined(OPTION_HAO) */
