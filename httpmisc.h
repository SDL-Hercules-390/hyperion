/* HTTPMISC.H   (C) Copyright Jan Jaeger, 2002-2012                  */
/*              Hercules HTTP Server for Console Ops                 */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#ifndef _HTTPMISC_H
#define _HTTPMISC_H

/*-------------------------------------------------------------------*/

#if !defined( PKGDATADIR )
 #if !defined( _MSVC_ )
  #define HTTP_ROOT             "/usr/local/share/hercules/"
 #else
  #define HTTP_ROOT             "%ProgramFiles%\\Hercules\\html\\"
 #endif
#else
 #define  HTTP_ROOT             PKGDATADIR "/"
#endif

/*-------------------------------------------------------------------*/

#if !defined( _MSVC_ )
 #define HTTP_PS                "/"
#else
 #define HTTP_PS                "\\"
#endif

/*-------------------------------------------------------------------*/

#if defined( PATH_MAX )
 #define HTTP_PATH_LENGTH       PATH_MAX
#else
 #define HTTP_PATH_LENGTH       1024
#endif

/*-------------------------------------------------------------------*/

#define HTML_EXPIRE_SECS        (60*60*24*7)

#define HTTP_WELCOME "hercules.html"
#define HTML_HEADER  "include/header.htmlpart"
#define HTML_FOOTER  "include/footer.htmlpart"


/*-------------------------------------------------------------------*/

struct CGIVAR
{
    struct CGIVAR*  next;
    char*           name;
    char*           value;
    int             type;

        #define  VARTYPE_NONE       0x00
        #define  VARTYPE_GET        0x01
        #define  VARTYPE_POST       0x02
        #define  VARTYPE_PUT        0x04
        #define  VARTYPE_COOKIE     0x08
};
typedef struct CGIVAR   CGIVAR;

/*-------------------------------------------------------------------*/

struct MIMETAB
{
    char* suffix;
    char* type;
};
typedef struct MIMETAB  MIMETAB;

/*-------------------------------------------------------------------*/

struct WEBBLK
{
    #define HDL_NAME_WEBBLK     "WEBBLK"
    #define HDL_VERS_WEBBLK     "SDL 4.00"
    #define HDL_SIZE_WEBBLK     sizeof( WEBBLK )

    int sock;
    int request_type;

        #define REQTYPE_NONE    0x00
        #define REQTYPE_GET     0x01
        #define REQTYPE_POST    0x02
        #define REQTYPE_PUT     0x04

    char* request;
    char* baseurl;
    char* user;

    CGIVAR* cgivar;
};
typedef struct WEBBLK   WEBBLK;

/*-------------------------------------------------------------------*/

typedef void cgibin_func( WEBBLK* webblk );

struct CGITAB
{
    char*         path;
    cgibin_func*  cgibin;
};
typedef struct CGITAB   CGITAB;

/*-------------------------------------------------------------------*/

#define cgi_variable( _webblk, _varname )   http_variable( (_webblk), (_varname), (VARTYPE_GET | VARTYPE_POST) )
#define cgi_cookie(   _webblk, _varname )   http_variable( (_webblk), (_varname), (VARTYPE_COOKIE) )
#define cgi_username( _webblk )             ( (_webblk)->user    )
#define cgi_baseurl(  _webblk )             ( (_webblk)->baseurl )

/*-------------------------------------------------------------------*/

HTTP_DLL_IMPORT void  html_header   ( WEBBLK* webblk );
HTTP_DLL_IMPORT void  html_footer   ( WEBBLK* webblk );
HTTP_DLL_IMPORT void  json_header   ( WEBBLK* webblk );
HTTP_DLL_IMPORT int   html_include  ( WEBBLK* webblk, char* filename );
HTTP_DLL_IMPORT char* http_variable ( WEBBLK* webblk, char* name, int type );

/*-------------------------------------------------------------------*/

void* http_server( void* arg );
char* http_root();

/*-------------------------------------------------------------------*/

#endif /* _HTTPMISC_H */
