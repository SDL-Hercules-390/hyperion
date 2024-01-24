/* HTTPSERV.C   (C) Copyright Jan Jaeger, 2002-2012                  */
/*              (C) Copyright TurboHercules, SAS 2010-2011           */
/*              Hercules HTTP Server for Console Ops                 */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/* This file contains all code required for the HTTP server,         */
/* when the http_server thread is started it will listen on          */
/* the HTTP port specified on the HTTPPORT config statement.         */
/*                                                                   */
/* When a request comes in a http_request thread is started,         */
/* which will handle the request.                                    */
/*                                                                   */
/* When authentification is required (auth parm on the HTTPPORT      */
/* statement) then the user will be authenticated based on the       */
/* userid and password supplied on the HTTPPORT statement, if        */
/* these where not supplied then the userid and password of the      */
/* that hercules is running under will be used. In the latter case   */
/* the root userid/password will also be accepted.                   */
/*                                                                   */
/* If the request is for a /cgi-bin/ path, then the cgibin           */
/* directory in cgibin.c will be searched, for any other request     */
/* the http_serv.httproot (/usr/local/hercules) will be used as the  */
/* root to find the specified file.                                  */
/*                                                                   */
/* As realpath() is used to verify that the files are from within    */
/* the http_serv.httproot tree symbolic links that refer to files    */
/* outside the http_serv.httproot tree are not supported.            */
/*                                                                   */
/*                                           Jan Jaeger - 28/03/2002 */
/*-------------------------------------------------------------------*/

#include "hstdinc.h"

#define _HTTPSERV_C_
#define _HENGINE_DLL_

#include "hercules.h"
#include "httpmisc.h"
#include "hostinfo.h"

// #define DEBUG_HTTPSERV

/*-------------------------------------------------------------------*/
/* External reference to the cgi-bin directory in cgibin.c           */
/*-------------------------------------------------------------------*/
extern CGITAB cgidir[];

/*-------------------------------------------------------------------*/

static MIMETAB mime_types[] =
{
    { NULL,    NULL },                  /* No suffix entry           */

    { "txt",   "text/plain" },
    { "jcl",   "text/plain" },
    { "gif",   "image/gif"  },
    { "jpg",   "image/jpeg" },
    { "css",   "text/css"   },
    { "html",  "text/html"  },
    { "htm",   "text/html"  },
    { "js",    "text/javascript"  },

    /*----------------------------------------*/
    /*  This one SHOULD be:                   */
    /* { "ico", "image/vnd.microsoft.icon" }, */
    /* but Apache 2 sets it as:               */

    { "ico",   "image/x-icon" },

    /* so we'll go with what's in use. --JRM  */
    /*----------------------------------------*/

    { NULL,    NULL }                   /* Default suffix entry      */
};

/*-------------------------------------------------------------------*/

struct HTTP_SERV
{
    U16     httpport;                   /* HTTP port number or zero  */
    char*   httpuser;                   /* HTTP userid               */
    char*   httppass;                   /* HTTP password             */
    char*   httproot;                   /* HTTP root                 */
    BYTE    httpauth:1;                 /* HTTP auth required flag   */
    BYTE    httpbinddone:1;             /* HTTP waiting for bind     */
    BYTE    httpshutdown:1;             /* HTTP Flag to signal shut  */
    BYTE    httpstmtold:1;              /* HTTP old command type     */
    COND    http_wait_shutdown;         /* HTTP Shutdown condition   */
    LOCK    http_lock_shutdown;         /* HTTP Shutdown lock        */
};
typedef struct HTTP_SERV    HTTP_SERV;

/*-------------------------------------------------------------------*/

static LOCK       http_lock_root;       /* HTTP Root Change Lock     */
static HTTP_SERV  http_serv;
static BYTE       http_struct_init = FALSE;

/*-------------------------------------------------------------------*/
/*                        html_include                               */
/*-------------------------------------------------------------------*/
DLL_EXPORT int html_include(WEBBLK *webblk, char *filename)
{
    FILE *inclfile;
    char fullname[HTTP_PATH_LENGTH];
    char buffer[HTTP_PATH_LENGTH];
    int ret;

    STRLCPY( fullname, http_serv.httproot );
    STRLCAT( fullname, filename );

    inclfile = fopen(fullname,"rb");

    if (!inclfile)
    {
        WRMSG(HHC01800,"E","fopen()",strerror(errno));
        hprintf(webblk->sock,MSG(HHC01800,"E","fopen()",strerror(errno)));
        return FALSE;
    }

    while (!feof(inclfile))
    {
        ret = (int)fread(buffer, 1, sizeof(buffer), inclfile);
        if (ret <= 0) break;
        hwrite(webblk->sock,buffer, ret);
    }

    fclose(inclfile);
    return TRUE;
}

/*-------------------------------------------------------------------*/
/*                         html_header                               */
/*-------------------------------------------------------------------*/
DLL_EXPORT void html_header(WEBBLK *webblk)
{
    if (webblk->request_type != REQTYPE_POST)
        hprintf(webblk->sock,"Expires: 0\n");

    hprintf(webblk->sock,"Content-type: text/html\n\n");

    if (!html_include(webblk,HTML_HEADER))
        hprintf(webblk->sock,"<HTML>\n<HEAD>\n<TITLE>Hercules</TITLE>\n</HEAD>\n<BODY>\n\n");
}

/*-------------------------------------------------------------------*/
/*                        html_footer                                */
/*-------------------------------------------------------------------*/
DLL_EXPORT void html_footer(WEBBLK *webblk)
{
    if (!html_include(webblk,HTML_FOOTER))
        hprintf(webblk->sock,"\n</BODY>\n</HTML>\n");
}


/*-------------------------------------------------------------------*/
/*                         json_header                               */
/*-------------------------------------------------------------------*/
DLL_EXPORT void json_header(WEBBLK *webblk)
{
    hprintf(webblk->sock,"Content-type: application/json\n\n");
}


/*-------------------------------------------------------------------*/
/*                          http_exit                                */
/*-------------------------------------------------------------------*/
static void http_exit(WEBBLK *webblk)
{
CGIVAR *cgivar;
int rc;
    if(webblk)
    {
        /* MS SDK docs state:

            "To assure that all data is sent and received on a connected
             socket before it is closed, an application should use shutdown
             to close connection before calling closesocket. For example,
             to initiate a graceful disconnect:

                1, Call WSAAsyncSelect to register for FD_CLOSE notification.
                2. Call shutdown with how=SD_SEND.
                3. When FD_CLOSE received, call recv until zero returned,
                   or SOCKET_ERROR.
                4. Call closesocket.

            Note: The shutdown function does not block regardless of the
            SO_LINGER setting on the socket."
        */

        // Notify other end of connection to not expect any more data from us.
        // They should detect this via their own 'recv' returning zero bytes
        // (thus letting them know they've thus received all the data from us
        // they're ever going to receive). They should then do their own
        // 'shutdown(s,SHUT_WR)' at their end letting US know we're also not
        // going to be receiving any more data from THEM. This is called a
        // "graceful close" of the connection...

        shutdown( webblk->sock, SHUT_WR );

        // Now wait for them to shudown THEIR end of the connection (i.e. wait
        // for them to do their own 'shutdown(s,SHUT_WR)') by "hanging" on a
        // 'recv' call until we either eventually detect they've shutdown their
        // end of the connection (0 bytes received) or else an error occurs...

        do
        {
            BYTE c;
            rc = read_socket( webblk->sock, &c, 1 );
        }
        while ( rc > 0 );

        // NOW we can SAFELY close the socket since we now KNOW for CERTAIN
        // that they've received ALL of the data we previously sent to them...
        // (otherwise they wouldn't have close their connection on us!)

        close_socket( webblk->sock );

        if(webblk->user) free(webblk->user);
        if(webblk->request) free(webblk->request);
        cgivar = webblk->cgivar;
        while(cgivar)
        {
            CGIVAR *tmpvar = cgivar->next;
            free(cgivar->name);
            free(cgivar->value);
            free(cgivar);
            cgivar = tmpvar;
        }
        free(webblk);
    }
    exit_thread(NULL);
}

/*-------------------------------------------------------------------*/
/*                         http_error                                */
/*-------------------------------------------------------------------*/
static void http_error(WEBBLK *webblk, char *err, char *header, char *info)
{
    hprintf(webblk->sock,"HTTP/1.0 %s\n%sConnection: close\n"
                          "Content-Type: text/html\n\n"
                          "<HTML><HEAD><TITLE>%s</TITLE></HEAD>"
                          "<BODY><H1>%s</H1><P>%s</BODY></HTML>\n\n",
                          err, header, err, err, info);
    http_exit(webblk);
}

/*-------------------------------------------------------------------*/
/*                      http_timestring                              */
/*-------------------------------------------------------------------*/
static char *http_timestring(char *time_buff,int buff_size, time_t t)
{
    struct tm *tm = localtime(&t);
    strftime(time_buff, buff_size, "%a, %d %b %Y %H:%M:%S %Z", tm);
    return time_buff;
}

/*-------------------------------------------------------------------*/
/*                     http_decode_base64                            */
/*-------------------------------------------------------------------*/
static void http_decode_base64(char *s)
{
    char *b64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    int bit_o, byte_o, idx, i, n;
    unsigned char *d = (unsigned char *)s;
    char *p;

    n = i = 0;

    while (*s && (p = strchr(b64, *s)))
    {
        idx = (int)(p - b64);
        byte_o = (i*6)/8;
        bit_o = (i*6)%8;
        d[byte_o] &= ~((1<<(8-bit_o))-1);
        if (bit_o < 3)
        {
            d[byte_o] |= (idx << (2-bit_o));
            n = byte_o+1;
        }
        else
        {
            d[byte_o] |= (idx >> (bit_o-2));
            d[byte_o+1] = 0;
            d[byte_o+1] |= (idx << (8-(bit_o-2))) & 0xFF;
            n = byte_o+2;
        }
            s++; i++;
    }
    /* null terminate */
    d[n] = 0;
}

/*-------------------------------------------------------------------*/
/*                         http_unescape                             */
/*-------------------------------------------------------------------*/
static char *http_unescape(char *buffer)
{
    char *pointer = buffer;

    while ( (pointer = strchr(pointer,'+')) )
        *pointer = ' ';

    pointer = buffer;

    while (pointer && *pointer && (pointer = strchr(pointer,'%')))
    {
        int highnibble = pointer[1];
        int lownibble = pointer[2];

        if (highnibble >= '0' && highnibble <= '9')
            highnibble = highnibble - '0';
        else if (highnibble >= 'A' && highnibble <= 'F')
            highnibble = 10 + highnibble - 'A';
        else if (highnibble >= 'a' && highnibble <= 'f')
            highnibble = 10 + highnibble - 'a';
        else
        {
            pointer++;
            continue;
        }

        if (lownibble >= '0' && lownibble <= '9')
            lownibble = lownibble - '0';
        else if (lownibble >= 'A' && lownibble <= 'F')
            lownibble = 10 + lownibble - 'A';
        else if (lownibble >= 'a' && lownibble <= 'f')
            lownibble = 10 + lownibble - 'a';
        else
        {
            pointer++;
            continue;
        }

        *pointer = (highnibble<<4) | lownibble;

        memmove(pointer+1, pointer+3, strlen(pointer+3)+1);

        pointer++;
    }

    return buffer;
}

/*-------------------------------------------------------------------*/
/*                http_interpret_variable_string                     */
/*-------------------------------------------------------------------*/
static void http_interpret_variable_string(WEBBLK *webblk, char *qstring, int type)
{
char *name;
char *value;
char *strtok_str = NULL;
CGIVAR **cgivar;

    for (cgivar = &(webblk->cgivar);
        *cgivar;
         cgivar = &((*cgivar)->next));

    for (name = strtok_r(qstring,"&; ",&strtok_str);
         name;
         name = strtok_r(NULL,"&; ",&strtok_str))
    {
        if(!(value = strchr(name,'=')))
            continue;

        *value++ = '\0';

        (*cgivar) = malloc(sizeof(CGIVAR));
        (*cgivar)->next = NULL;
        (*cgivar)->name = strdup(http_unescape(name));
        (*cgivar)->value = strdup(http_unescape(value));
        (*cgivar)->type = type;
        cgivar = &((*cgivar)->next);
    }
}

/*-------------------------------------------------------------------*/

#ifdef DEBUG_HTTPSERV
static void http_dump_cgi_variables(WEBBLK *webblk)
{
    CGIVAR* cv;
    for (cv = webblk->cgivar; cv; cv = cv->next)
        LOGMSG( "cgi_var_dump: pointer(%p) name(%s) value(%s) type(%d)\n",
            cv, cv->name, cv->value, cv->type );
}
#endif

/*-------------------------------------------------------------------*/
/*                        http_variable                              */
/*-------------------------------------------------------------------*/
DLL_EXPORT char *http_variable(WEBBLK *webblk, char *name, int type)
{
    CGIVAR *cv;
    for(cv = webblk->cgivar; cv; cv = cv->next)
        if((cv->type & type) && !strcmp(name,cv->name))
            return cv->value;
    return NULL;
}

/*-------------------------------------------------------------------*/
/*                      http_verify_path                             */
/*-------------------------------------------------------------------*/
static void http_verify_path(WEBBLK *webblk, char *path)
{
    char resolved_path[HTTP_PATH_LENGTH];
    char pathname[HTTP_PATH_LENGTH];

#ifdef DEBUG_HTTPSERV
    int i;

    for (i = 0; path[i]; i++)
        if (!isalnum((unsigned char)path[i]) && !strchr("/.-_", path[i]))
            http_error(webblk, "404 File Not Found","",
                               "Illegal character in filename");
#endif

    if (!realpath( path, resolved_path ))
        http_error(webblk, "404 File Not Found","",
                           "Invalid pathname");

    hostpath(pathname, resolved_path, sizeof(pathname));

    // The following verifies the specified file does not lie
    // outside the specified httproot (Note: http_serv.httproot
    // was previously resolved to an absolute path by config.c)

    if (strncmp( http_serv.httproot, pathname, strlen(http_serv.httproot)))
        http_error(webblk, "404 File Not Found","",
                           "Invalid pathname");
}

/*-------------------------------------------------------------------*/
/*                       http_authenticate                           */
/*-------------------------------------------------------------------*/
static int http_authenticate(WEBBLK *webblk, char *type, char *userpass)
{
    char *pointer ,*user, *passwd;

    if (!strcasecmp(type,"Basic"))
    {
        if(userpass)
        {
            http_decode_base64(userpass);

            /* the format is now user:password */
            if ((pointer = strchr(userpass,':')))
            {
                *pointer = 0;
                user = userpass;
                passwd = pointer+1;

                /* Hardcoded userid and password in configuration file */
                if(http_serv.httpuser && http_serv.httppass)
                {
                    if(!strcmp(user,http_serv.httpuser)
                      && !strcmp(passwd,http_serv.httppass))
                    {
                        webblk->user = strdup(user);
                        return TRUE;
                    }
                }
#if !defined(WIN32)
                else
                {
                    struct passwd *pass = NULL;

                    /* unix userid and password check, the userid
                       must be the same as that hercules is
                       currently running under */
// ZZ INCOMPLETE
// ZZ No password check is being performed yet...
                    if((pass = getpwnam(user))
                      &&
                       (pass->pw_uid == 0
                          || pass->pw_uid == getuid()))
                    {
                        webblk->user = strdup(user);
                        return TRUE;
                    }
                }
#endif /*!defined(WIN32)*/
            }
        }
    }

    webblk->user = NULL;

    return FALSE;
}

/*-------------------------------------------------------------------*/
/*                         http_download                             */
/*-------------------------------------------------------------------*/
static void http_download(WEBBLK *webblk, char *filename)
{
    char buffer[HTTP_PATH_LENGTH];
    char tbuf[80];
    int fd, length;
    char *filetype;
    char fullname[HTTP_PATH_LENGTH];
    struct stat st;
    MIMETAB *mime_type = mime_types;

    STRLCPY( fullname, http_serv.httproot );
    STRLCAT( fullname, filename );

    http_verify_path(webblk,fullname);

    if(stat(fullname,&st))
        http_error(webblk, "404 File Not Found","",
                           strerror(errno));

    if(!S_ISREG(st.st_mode))
        http_error(webblk, "404 File Not Found","",
                           "The requested file is not a regular file");

    fd = HOPEN(fullname,O_RDONLY|O_BINARY,0);
    if (fd == -1)
        http_error(webblk, "404 File Not Found","",
                           strerror(errno));

    hprintf(webblk->sock,"HTTP/1.0 200 OK\n");
    if ((filetype = strrchr(filename,'.')))
        for(mime_type++;mime_type->suffix
          && strcasecmp(mime_type->suffix,filetype + 1);
          mime_type++);
    if(mime_type->type)
        hprintf(webblk->sock,"Content-Type: %s\n", mime_type->type);

    hprintf(webblk->sock,"Expires: %s\n",
      http_timestring(tbuf,sizeof(tbuf),time(NULL)+HTML_EXPIRE_SECS));

    hprintf(webblk->sock,"Content-Length: %d\n\n", (int)st.st_size);
    while ((length = read(fd, buffer, sizeof(buffer))) > 0)
            hwrite(webblk->sock,buffer, length);
    close(fd);
    http_exit(webblk);
}

/*-------------------------------------------------------------------*/
/*                          http_request                             */
/*-------------------------------------------------------------------*/
static void *http_request(void* arg)
{
    WEBBLK *webblk;
    int authok = !http_serv.httpauth;
    char line[HTTP_PATH_LENGTH];
    char *url = NULL;
    char *pointer;
    char *strtok_str = NULL;
    CGITAB *cgient;
    int content_length = 0;
    int sock = (int) (uintptr_t) arg;

    if(!(webblk = malloc(sizeof(WEBBLK))))
        http_exit(webblk);

    memset(webblk,0,sizeof(WEBBLK));
    webblk->sock = sock;

    while (hgets(line, sizeof(line), webblk->sock))
    {
        if (*line == '\r' || *line == '\n')
            break;

        if((pointer = strtok_r(line," \t\r\n",&strtok_str)))
        {
            if(!strcasecmp(pointer,"GET"))
            {
                if((pointer = strtok_r(NULL," \t\r\n",&strtok_str)))
                {
                    webblk->request_type = REQTYPE_GET;
                    url = strdup(pointer);
                }
            }
            else
            if(!strcasecmp(pointer,"POST"))
            {
                if((pointer = strtok_r(NULL," \t\r\n",&strtok_str)))
                {
                    webblk->request_type = REQTYPE_POST;
                    url = strdup(pointer);
                }
            }
            else
            if(!strcasecmp(pointer,"PUT"))
            {
                http_error(webblk,"400 Bad Request", "",
                                  "This server does not accept PUT requests");
            }
            else
            if(!strcasecmp(pointer,"Authorization:"))
            {
                if((pointer = strtok_r(NULL," \t\r\n",&strtok_str)))
                    authok = http_authenticate(webblk,pointer,
                                  strtok_r(NULL," \t\r\n",&strtok_str));
            }
            else
            if(!strcasecmp(pointer,"Cookie:"))
            {
                if((pointer = strtok_r(NULL,"\r\n",&strtok_str)))
                    http_interpret_variable_string(webblk, pointer, VARTYPE_COOKIE);
            }
            else
            if(!strcasecmp(pointer,"Content-Length:"))
            {
                if((pointer = strtok_r(NULL," \t\r\n",&strtok_str)))
                    content_length = atoi(pointer);
            }
        }
    }
    webblk->request = url;

    if(webblk->request_type == REQTYPE_POST
      && content_length != 0)
    {
    char *post_arg;
        if((pointer = post_arg = malloc(content_length + 1)))
        {
        int i;
            for(i = 0; i < content_length; i++)
            {
                *pointer = hgetc(webblk->sock);
                if(*pointer != '\n' && *pointer != '\r')
                    pointer++;
            }
            *pointer = '\0';
            http_interpret_variable_string(webblk, post_arg, VARTYPE_POST);
            free(post_arg);
        }
    }

    if (!authok)
    {
        http_error(webblk, "401 Authorization Required",
                           "WWW-Authenticate: Basic realm=\"HERCULES\"\n",
                           "You must be authenticated to use this service");
    }

    if (!url)
    {
        http_error(webblk,"400 Bad Request", "",
                          "You must specify a GET or POST request");
    }

    /* anything following a ? in the URL is part of the get arguments */
    if ((pointer=strchr(url,'?'))) {
        *pointer++ = 0;
        http_interpret_variable_string(webblk, pointer, VARTYPE_GET);
    }

    while(url[0] == '/' && url[1] == '/')
        url++;

    webblk->baseurl = url;

    if(!strcasecmp("/",url))
        url = HTTP_WELCOME;

    if(strncasecmp("/cgi-bin/",url,9))
        http_download(webblk,url);
    else
        url += 9;

    while(*url == '/')
        url++;

#ifdef DEBUG_HTTPSERV
    http_dump_cgi_variables(webblk);
#endif

    for(cgient = cgidir; cgient->path; cgient++)
    {
        if(!strcmp(cgient->path, url))
        {
        char tbuf[80];
            hprintf(webblk->sock,"HTTP/1.0 200 OK\nConnection: close\n");
            hprintf(webblk->sock,"Date: %s\n",
              http_timestring(tbuf,sizeof(tbuf),time(NULL)));
            (cgient->cgibin) (webblk);
            http_exit(webblk);
        }
    }

    {
        cgibin_func* dyncgi;

        if( (dyncgi = (cgibin_func*) hdl_getsym( webblk->baseurl )) )
        {
        char tbuf[80];
            hprintf(webblk->sock,"HTTP/1.0 200 OK\nConnection: close\n");
            hprintf(webblk->sock,"Date: %s\n",
              http_timestring(tbuf,sizeof(tbuf),time(NULL)));
            dyncgi(webblk);
            http_exit(webblk);
        }
    }

    http_error(webblk, "404 File Not Found","",
                       "The requested file was not found");
    return NULL;
}

/*-------------------------------------------------------------------*/
/*                          http_root                                */
/*-------------------------------------------------------------------*/
char *http_root()
{
    obtain_lock( &http_lock_root );

    /* If the HTTP root directory is not specified,
       use a reasonable default */
    if (!http_serv.httproot)
    {
#if defined(_MSVC_)
        char process_dir[HTTP_PATH_LENGTH];
        if (get_process_directory(process_dir,HTTP_PATH_LENGTH) > 0)
        {
            STRLCAT( process_dir, "\\html" );
            http_serv.httproot = strdup(process_dir);
        }
        else
#endif /*defined(WIN32)*/
        http_serv.httproot = strdup(HTTP_ROOT);
    }

    /* Convert the specified HTTPROOT value to an absolute path
       ending with a '/' and save in http_serv.httproot. */
    {
        char absolute_httproot_path[HTTP_PATH_LENGTH];
        int  rc;
#if defined(_MSVC_)
        /* Expand any embedded %var% environ vars */
        rc = expand_environ_vars( http_serv.httproot, absolute_httproot_path,
            sizeof(absolute_httproot_path) );
        if (rc == 0)
        {
            char *p = strdup(absolute_httproot_path);;
            if ( http_serv.httproot != NULL )
                free(http_serv.httproot);

            http_serv.httproot = p;
        }
#endif /* defined(_MSVC_) */
        /* Convert to absolute path */
        if (!realpath(http_serv.httproot, absolute_httproot_path))
        {
            char msgbuf[MAX_PATH+3] = { 0 };
            char *p = msgbuf;

            if ( strchr( http_serv.httproot, SPACE ) == NULL )
                p = http_serv.httproot;
            else
                MSGBUF(msgbuf, "'%s'", http_serv.httproot);

            // "HTTP server: invalid root directory: %s: %s"
            WRMSG(HHC01801, "E", p, strerror(errno));

            release_lock( &http_lock_root );

            return NULL;
        }
        /* Verify that the absolute path is valid */
        // mode: 0 = exist only, 2 = write, 4 = read, 6 = read/write
        // rc: 0 = success, -1 = error (errno = cause)
        // ENOENT = File name or path not found.
        if (access( absolute_httproot_path, R_OK ) != 0)
        {
            char msgbuf[MAX_PATH+3];
            char *p = absolute_httproot_path;

            if ( strchr( absolute_httproot_path, SPACE ) != NULL )
            {
                MSGBUF(msgbuf, "'%s'", absolute_httproot_path);
                p = msgbuf;
            }

            // "HTTP server: invalid root directory: %s: %s"
            WRMSG(HHC01801, "E", p, strerror(errno));

            release_lock( &http_lock_root );

            return NULL;
        }

        /* Append trailing [back]slash, but only if needed */
        rc = (int)strlen(absolute_httproot_path);

        if (absolute_httproot_path[rc-1] != *HTTP_PS)
            STRLCAT( absolute_httproot_path, HTTP_PS );

        if (strlen(absolute_httproot_path) >= MAX_PATH )
        {
            char msgbuf[MAX_PATH+3] = { 0 };
            char *p = msgbuf;

            if ( strchr( absolute_httproot_path, SPACE ) == NULL )
                p = absolute_httproot_path;
            else
                MSGBUF(msgbuf, "'%s'", absolute_httproot_path);

            // "HTTP server: invalid root directory: %s: %s"
            WRMSG(HHC01801, "E", p, "path length too long");

            release_lock( &http_lock_root );

            return NULL;
        }
        else
        {
            char pathname[MAX_PATH];    /* working pathname          */
            char msgbuf[MAX_PATH+3];
            char *p = msgbuf;

            memset(msgbuf,0,sizeof(msgbuf));

            /* Save the absolute path */
            hostpath(pathname, absolute_httproot_path, sizeof(pathname));
            free(http_serv.httproot);
            http_serv.httproot = strdup(pathname);

            if ( strchr( http_serv.httproot, SPACE ) == NULL )
                p = http_serv.httproot;
            else
                MSGBUF(msgbuf, "'%s'", http_serv.httproot);

            // "HTTP server using root directory %s"
            WRMSG(HHC01802, "I", p);
        }
    }

    release_lock( &http_lock_root );

    return http_serv.httproot;
}

/*-------------------------------------------------------------------*/
/*                        http_shutdown                              */
/*-------------------------------------------------------------------*/
static void http_shutdown(void * unused)
{
    UNREFERENCED(unused);

    obtain_lock(&http_serv.http_lock_shutdown);

    http_serv.httpshutdown = TRUE;      /* signal shutdown */

    if ( sysblk.httptid != 0 )
        timed_wait_condition_relative_usecs(&http_serv.http_wait_shutdown,&http_serv.http_lock_shutdown,2*1000*1000,NULL);

    release_lock(&http_serv.http_lock_shutdown);
}

/*-------------------------------------------------------------------*/
/*                        http_server                                */
/*-------------------------------------------------------------------*/
void *http_server (void *arg)
{
int                 rc;                 /* Return code               */
int                 lsock;              /* Socket for listening      */
int                 csock;              /* Socket for conversation   */
struct sockaddr_in  server;             /* Server address structure  */
fd_set              selset;             /* Read bit map for select   */
int                 optval;             /* Argument for setsockopt   */
TID                 httptid;            /* Negotiation thread id     */
struct timeval      timeout;            /* timeout value             */


    UNREFERENCED(arg);

    http_serv.httpshutdown = TRUE;

    hdl_addshut("http_shutdown",http_shutdown, NULL);

    /* Set server thread priority; ignore any errors */
    SET_THREAD_PRIORITY( sysblk.srvprio, sysblk.qos_user_interactive );

    /* Display thread started message on control panel */
    LOG_THREAD_BEGIN( HTTP_SRVR_THREAD_NAME  );

    /* make sure root path is built */
    if ( http_root() == NULL )
        goto http_server_stop;

    /* Obtain a socket */
    lsock = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (lsock < 0)
    {
        WRMSG(HHC01800,"E", "socket()", strerror(HSO_errno));
        goto http_server_stop;
    }

    /* Allow previous instance of socket to be reused */
    optval = 1;
    setsockopt (lsock, SOL_SOCKET, SO_REUSEADDR,
                (void*)&optval, sizeof(optval));

    /* Prepare the sockaddr structure for the bind */
    memset (&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = http_serv.httpport;
    server.sin_port = htons(server.sin_port);

    http_serv.httpbinddone = FALSE;
    /* Attempt to bind the socket to the port */
    while (TRUE)
    {
        rc = bind (lsock, (struct sockaddr *)&server, sizeof(server));

        if (rc == 0 || HSO_errno != HSO_EADDRINUSE) break;

        WRMSG(HHC01804, "W", http_serv.httpport);
        SLEEP(10);
    } /* end while */

    if (rc != 0)
    {
        WRMSG(HHC01800,"E", "bind()", strerror(HSO_errno));
        goto http_server_stop;
    }
    else
        http_serv.httpbinddone = TRUE;

    /* Put the socket into listening state */
    rc = listen (lsock, 32);

    if (rc < 0)
    {
        WRMSG(HHC01800,"E", "listen()", strerror(HSO_errno));
        http_serv.httpbinddone = FALSE;
        goto http_server_stop;
    }

    http_serv.httpshutdown = FALSE;

    WRMSG(HHC01803, "I", http_serv.httpport);

    /* Handle http requests */
    while ( !http_serv.httpshutdown )
    {

        /* Initialize the select parameters */
        FD_ZERO (&selset);
        FD_SET (lsock, &selset);

        timeout.tv_sec  = 0;
        timeout.tv_usec = 10000;

        /* until a better way to implement this use standard windows */
#undef select
        /* Wait for a file descriptor to become ready  use NON-BLOCKING select()*/
        rc = select ( lsock+1, &selset, NULL, NULL, &timeout );

        if ( rc == 0 || http_serv.httpshutdown ) continue;

        if (rc < 0 )
        {
            if (HSO_errno == HSO_EINTR) continue;
            WRMSG(HHC01800, "E", "select()", strerror(HSO_errno));
            break;
        }

        /* If a http request has arrived then accept it */
        if (FD_ISSET(lsock, &selset))
        {
            /* Accept the connection and create conversation socket */
            csock = accept (lsock, NULL, NULL);

            if (csock < 0)
            {
                WRMSG(HHC01800, "E", "accept()", strerror(HSO_errno));
                continue;
            }

            /* Create a thread to execute the http request */
            rc = create_thread( &httptid, DETACHED,
                                http_request, (void *)(uintptr_t)csock,
                                HTTP_REQ_THREAD_NAME );
            if(rc)
            {
                WRMSG(HHC00102, "E", strerror(rc));
                close_socket (csock);
            }

        } /* end if(lsock) */

    } /* end while */

    /* Close the listening socket */
    close_socket (lsock);

http_server_stop:
    if ( !sysblk.shutdown )
        hdl_delshut(http_shutdown, NULL);

    /* Display thread ended message on control panel */
    LOG_THREAD_END( HTTP_SRVR_THREAD_NAME  );

    sysblk.httptid = 0;

    http_serv.httpbinddone = FALSE;

    signal_condition(&http_serv.http_wait_shutdown);

    return NULL;

} /* end function http_server */

/*-------------------------------------------------------------------*/
/*                         http_startup                              */
/*-------------------------------------------------------------------*/
int http_startup(int isconfigcalling)
{
    int rc = 0;
    static int first_call = TRUE;

    if ( first_call )
    {
        if ( !http_struct_init )
        {
            memset(&http_serv,0,sizeof(HTTP_SERV));
            initialize_condition( &http_serv.http_wait_shutdown );
            initialize_lock( &http_serv.http_lock_shutdown );
            initialize_lock( &http_lock_root );
            http_struct_init = TRUE;
        }
        first_call = FALSE;
    }

    if ( http_serv.httpport == 0 )
    {
        rc = -1;
    }
    else if ( isconfigcalling )
    {
        if ( !http_serv.httpstmtold )
        {
            rc = 1;
        }
    }

    if ( rc == 0 )
    {
        if ( sysblk.httptid == 0 )
        {
            int rc_ct;

            rc_ct = create_thread( &sysblk.httptid, DETACHED,
                http_server, NULL, HTTP_SRVR_THREAD_NAME );
            if ( rc_ct )
            {
                WRMSG(HHC00102, "E", strerror(rc));
                rc = -1;
            }
            else
            {
                WRMSG( HHC01807, "I" );
                rc = 0;
            }
        }
        else
        {
            WRMSG( HHC01806, "W", "already started" );
            rc = 0;
        }
    }

    return rc;
}

/*-------------------------------------------------------------------*/
/*                          http_command                             */
/*-------------------------------------------------------------------*/
int http_command( int argc, char* argv[] )
{
    int rc = 0;

    UPPER_ARGV_0( argv );

    if (!http_struct_init)
    {
        memset( &http_serv, 0 , sizeof( HTTP_SERV ));

        initialize_condition( &http_serv.http_wait_shutdown );
        initialize_lock( &http_serv.http_lock_shutdown );
        initialize_lock( &http_lock_root );

        http_struct_init = TRUE;
    }

    http_serv.httpstmtold = FALSE;

    if (1
        && argc == 2
        && CMD( argv[0], ROOTX, 4 )
        && (0
            || strlen( argv[0] ) == 4
            || (1
                && strlen( argv[0] ) == 5
                && argv[2]
                && strcmp( argv[2], "httproot" ) == 0
               )
           )
    )
    {
        if (strlen( argv[0] ) == 5)
        {
            http_serv.httpstmtold = TRUE;
        }

        obtain_lock( &http_lock_root );
        {
            if (http_serv.httproot)
            {
                free( http_serv.httproot );
                http_serv.httproot = NULL;
            }

            if (strlen( argv[1] ) > 0)
            {
                char pathname[ MAX_PATH ];

                hostpath( pathname, argv[1], sizeof( pathname ));

                if (pathname[ strlen( pathname ) - 1 ] != PATHSEPC)
                    STRLCAT( pathname, PATHSEPS );

                http_serv.httproot = strdup( pathname );
            }
        }
        release_lock( &http_lock_root );

        http_root();

        if (MLVL( VERBOSE ))
            // "%-14s set to %s"
            WRMSG( HHC02204, "I", http_serv.httpstmtold ? "HTTPROOT": "ROOT",
                        http_serv.httproot ? http_serv.httproot : "<not specified>");

        if (http_serv.httpstmtold)
            http_startup( TRUE );

        rc = 0;
    }
    else if (1
             && (0
                 || argc == 2
                 || argc == 3
                 || argc == 5
                )
             && CMD( argv[0], PORTX, 4 )
             && (0
                 || strlen(argv[0]) == 4
                 || (1
                     && strlen( argv[0] ) == 5
                     && argv[5]
                     && strcmp( argv[5], "httpport" ) == 0
                    )
                )
    )
    {
        if (strlen( argv[0] ) == 5)
        {
            http_serv.httpstmtold = TRUE;
        }

        if (sysblk.httptid)
        {
            // "HTTP server must be stopped for this command"
            WRMSG( HHC01812, "E" );
            rc = -1;
        }
        else
        {
            char c;

            if (0
                || sscanf( argv[1], "%hu%c", &http_serv.httpport, &c ) != 1
                || !http_serv.httpport
                || (http_serv.httpport < 1024 && http_serv.httpport != 80)
            )
            {
                rc = -1;
            }
            if (rc >= 0 && argc == 3 && CMD( argv[2], NOAUTH, 6 ))
            {
                http_serv.httpauth = 0;
            }
            else if (rc >=0 && argc == 5 && CMD( argv[2], AUTH, 4 ))
            {
                if (strlen( argv[3] ) < 1 || strlen( argv[4] ) < 1)
                {
                    // "HTTP server: auth requires valid userid and password operands"
                    WRMSG( HHC01814, "E" );
                    rc = -1;
                }
                else
                {
                    if (http_serv.httpuser)
                        free( http_serv.httpuser );

                    http_serv.httpuser = strdup( argv[3] );

                    if (http_serv.httppass)
                        free( http_serv.httppass );

                    http_serv.httppass = strdup( argv[4] );

                    http_serv.httpauth = 1;
                }
            }
            else if (argc != 2 || rc < 0)
            {
                // "Invalid command usage. Type 'help %s' for assistance."
                WRMSG( HHC02299, "E", "http" );
                rc = -1;
            }

            if (rc >= 0 && MLVL( VERBOSE ))
            {
                char msgbuf[128];

                if ( http_serv.httpauth == 1 )
                {
                    MSGBUF( msgbuf, "port=%hu auth userid=\"%s\" password=(suppressed)",
                            http_serv.httpport,
                          ( http_serv.httpuser == NULL || strlen(http_serv.httpuser) == 0 ) ?
                                "" : http_serv.httpuser );
                }
                else
                    MSGBUF( msgbuf, "port=%hu noauth", http_serv.httpport );

                // "%-14s set to %s"
                WRMSG( HHC02204, "I", http_serv.httpstmtold ? "HTTPPORT" : "PORT", msgbuf );

                if (http_serv.httpstmtold)
                    http_startup( TRUE );

            }   /* VERBOSE */
        }
    }
    else if ( argc == 1 && CMD(argv[0],start,3) )
    {
        if (!http_serv.httpport)
        {
            // "HTTP server port is %s"
            WRMSG( HHC01815, "E", "not valid");
            rc = -1;
        }
        else
            rc = http_startup( FALSE );
    }
    else if (argc == 1 && CMD(argv[0],stop,4))
    {
        if (sysblk.httptid)
        {
            http_shutdown( NULL );

            // "HTTP server signaled to stop"
            WRMSG( HHC01805, "I" );
            rc = 1;
        }
        else
        {
            http_serv.httpshutdown = TRUE;

            // "HTTP server is %s"
            WRMSG( HHC01806, "W", "already stopped" );
            rc = 1;
        }
    }
    else if (!argc)
    {
        if (sysblk.httptid)
        {
            if (http_serv.httpbinddone)
            {
                // "HTTP server is waiting for requests"
                WRMSG( HHC01809, "I" );
                rc = 0;
            }
            else
            {
                // "HTTP server waiting for bind to complete; port in use by another server"
                WRMSG( HHC01813, "I" );
                rc = 1;
            }
        }
        else
        {
            // "HTTP server is stopped"
            WRMSG( HHC01810, "I" );
            rc = 1;
        }

        // "HTTP server root directory %s"
        WRMSG( HHC01811, "I", http_get_root());

        // "HTTP server port is %s with %s"
        WRMSG( HHC01808, "I", http_get_port(), http_get_portauth());
    }
    else
    {
        // "Invalid command usage. Type 'help %s' for assistance."
        WRMSG( HHC02299, "E", "http" );
        rc = -1;
    }
    return rc;
}

/*-------------------------------------------------------------------*/
/*                        http_get_port                              */
/*-------------------------------------------------------------------*/
char *http_get_port()
{
static char msgbuf[128];

    MSGBUF( msgbuf, "%hu", http_serv.httpport );

    return msgbuf;

}

/*-------------------------------------------------------------------*/
/*                      http_get_portauth                            */
/*-------------------------------------------------------------------*/
char *http_get_portauth()
{
static char msgbuf[128];

    if ( http_serv.httpauth == 1)
    {
        MSGBUF( msgbuf, "auth userid=\"%s\" password=(suppressed)",
                ( http_serv.httpuser == NULL || strlen(http_serv.httpuser) == 0 ) ?
                                "" : http_serv.httpuser );
    }
    else
    {
        MSGBUF( msgbuf, "%s", "noauth" );
    }

    return msgbuf;

}

/*-------------------------------------------------------------------*/
/*                       http_get_root                               */
/*-------------------------------------------------------------------*/
char *http_get_root()
{
        char *p;
static  char msgbuf[FILENAME_MAX+3];

    if ( http_serv.httproot == NULL )
        p = "is <not specified>";

    else if ( strchr(http_serv.httproot, SPACE) != NULL )
    {
        MSGBUF( msgbuf, "'%s'", http_serv.httproot );
        p = msgbuf;
    }
    else
    {
        p = http_serv.httproot;
    }

    return p;
}

/*-------------------------------------------------------------------*/
