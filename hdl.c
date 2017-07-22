/* HDL.C        (c) Copyright Jan Jaeger, 2003-2012                  */
/*              Hercules Dynamic Loader                              */

#include "hstdinc.h"

#define _HDL_C_
#define _HUTIL_DLL_

#include "hercules.h"
#include "opcode.h"

#if defined(OPTION_DYNAMIC_LOAD)
HDLPRE hdl_preload[] =
{
    { "hdteq",              HDL_LOAD_NOMSG                     },
    { "dyncrypt",           HDL_LOAD_NOMSG                     },
#if 0
    { "dyn_test1",          HDL_LOAD_DEFAULT                   },
    { "/foo/dyn_test2",     HDL_LOAD_NOMSG                     },
    { "../bar/dyn_test3",   HDL_LOAD_NOMSG | HDL_LOAD_NOUNLOAD },
#endif
    { NULL,                 0                                  },
};

static DLLENT *hdl_dll;                  /* dll chain                */
static LOCK   hdl_lock;                  /* loader lock              */
static DLLENT *hdl_cdll;                 /* current dll (hdl_lock)   */

static HDLDEP *hdl_depend;               /* Version codes in hdlmain */

static char *hdl_modpath = NULL;
static int   hdl_arg_p = FALSE;

static void hdl_didf (int, int, char *, void *);
static void hdl_modify_opcode(int, HDLINS *);

#endif

static HDLSHD *hdl_shdlist;              /* Shutdown call list       */
static int   hdl_sdip = FALSE;           /* hdl shutdown in progesss */

/* Global hdl_device_type_equates */

DLL_EXPORT char *(*hdl_device_type_equates)(const char *);

/* hdl_adsc - add shutdown call
 */
DLL_EXPORT void hdl_adsc (char* shdname, void * shdcall, void * shdarg)
{
HDLSHD *newcall;
HDLSHD **tmpcall;

    if(hdl_sdip)
        return;

    /* avoid duplicates - keep the first one */
    for(tmpcall = &(hdl_shdlist); *tmpcall; tmpcall = &((*tmpcall)->next) )
        if( (*tmpcall)->shdcall == shdcall
          && (*tmpcall)->shdarg == shdarg )
            return;

    newcall = malloc(sizeof(HDLSHD));
    newcall->shdname = shdname;
    newcall->shdcall = shdcall;
    newcall->shdarg = shdarg;
    newcall->next = hdl_shdlist;
    hdl_shdlist = newcall;

}

/* hdl_rmsc - remove shutdown call
 */
DLL_EXPORT int hdl_rmsc (void *shdcall, void *shdarg)
{
HDLSHD **tmpcall;
int rc = -1;

    if(hdl_sdip)
        return rc;

    for(tmpcall = &(hdl_shdlist); *tmpcall; tmpcall = &((*tmpcall)->next) )
    {
        if( (*tmpcall)->shdcall == shdcall
          && (*tmpcall)->shdarg == shdarg )
        {
        HDLSHD *frecall;
            frecall = *tmpcall;
            *tmpcall = (*tmpcall)->next;
            free(frecall);
            rc = 0;
        }
    }
    return rc;
}


/* hdl_shut - call all shutdown call entries in LIFO order
 */
DLL_EXPORT void hdl_shut (void)
{
HDLSHD *shdent;

    if(MLVL(DEBUG))
        // "HDL: begin shutdown sequence"
        WRMSG( HHC01500, "I" );

    hdl_sdip = TRUE;

    for(shdent = hdl_shdlist; shdent; shdent = hdl_shdlist)
    {
        /* Remove shutdown call entry to ensure it is called once */
        hdl_shdlist = shdent->next;

        {
            if(MLVL(DEBUG))
                // "HDL: calling %s"
                WRMSG( HHC01501, "I", shdent->shdname );

            (shdent->shdcall) (shdent->shdarg);

            if(MLVL(DEBUG))
                // "HDL: %s complete"
                WRMSG( HHC01502, "I", shdent->shdname );
        }
        free(shdent);
    }

        if(MLVL(DEBUG))
            // "HDL: shutdown sequence complete"
            WRMSG( HHC01504, "I" );
}

#if defined(OPTION_DYNAMIC_LOAD)


/* hdl_setpath - set path for module load
 * If path is NULL, then return the current path
 * If path length is greater than MAX_PATH, send message and return NULL
 *     indicating an error has occurred.
 * If flag is TRUE, then only set new path if not already defined
 * If flag is FALSE, then always set the new path.
 */
DLL_EXPORT char *hdl_setpath( const char *path, int flag )
{
    char    pathname[MAX_PATH];         /* pathname conversion  */
    char    abspath[MAX_PATH];          /* pathname conversion  */

    if (!path)
        return hdl_modpath;             /* return module path to caller */

    if ( strlen(path) > MAX_PATH )
    {
        // "HDL: path name length %d exceeds maximum of %d"
        WRMSG( HHC01505, "E", (int)strlen(path), MAX_PATH );
        return NULL;
    }

    // Convert path to host format
    hostpath( pathname, path, sizeof (pathname ));

    // Convert path to absolute path if it's relative
    abspath[0] = 0;
    if ('.' == pathname[0] && !realpath( pathname, abspath ))
    {
        char buf[MAX_PATH+128];
        MSGBUF( buf,  "\"%s\": %s", pathname, strerror( errno ) );
        // "HDL: error in function %s: %s"
        WRMSG( HHC01511, "W", "realpath()", buf );
        abspath[0] = 0;
    }
    if (!abspath[0])
        STRLCPY( abspath, pathname );

    // Check flag: TRUE == conditional, FALSE == unconditional
    if (flag)
    {
        if (hdl_modpath)
        {
            if (!hdl_arg_p)
            {
                free( hdl_modpath );
            }
            else
            {
                // "HDL: change request of directory to %s is ignored"
                WRMSG( HHC01506, "W", pathname );
                // "HDL: directory remains %s; taken from startup"
                WRMSG( HHC01507, "W", hdl_modpath );
                return hdl_modpath;
            }
        }
    }
    else
    {
        hdl_arg_p = TRUE;

        if (hdl_modpath)
            free( hdl_modpath );
    }

    hdl_modpath = strdup( abspath );

    if (MLVL( VERBOSE ))
    {
        struct stat statbuf;
#ifdef _MSVC_
        int len; char c;
#endif

        // "HDL: loadable module directory is %s"
        WRMSG( HHC01508, "I", hdl_modpath );
#ifdef _MSVC_
        // stat: If path contains the location of a directory,
        // it cannot contain a trailing backslash. If it does,
        // -1 will be returned and errno will be set to ENOENT.
        c = 0;
        len = strlen( hdl_modpath );
        if (0
            || hdl_modpath[len-1] == '/'
            || hdl_modpath[len-1] == '\\'
        )
        {
            c = hdl_modpath[len-1];
            hdl_modpath[len-1] = 0;
        }
#endif
        if (stat( hdl_modpath, &statbuf ) != 0 || !S_ISDIR( statbuf.st_mode ))
        {
            // "HDL: %s is not a valid directory"
            WRMSG( HHC01536, "W", hdl_modpath );
        }
#ifdef _MSVC_
        if (c)
            hdl_modpath[len-1] = c;
#endif
    }

    return hdl_modpath;
}

/*
 * Check in this order:
 * 1) filename as passed
 * 2) filename with extension if needed
 * 3) modpath added if basename(filename)
 * 4) extension added to #3
 */
static void * hdl_dlopen(char *filename, int flag _HDL_UNUSED)
{
char   *fullname;
void   *ret;
size_t  fulllen = 0;

    if ( (ret = dlopen(filename,flag)) )       /* try filename as is first */
        return ret;

    fulllen = strlen(filename) + strlen(hdl_modpath) + 2 + HDL_SUFFIX_LENGTH;
    fullname = (char *)calloc(1,fulllen);

    if ( fullname == NULL )
        return NULL;

#if defined(HDL_MODULE_SUFFIX)
    strlcpy(fullname,filename,fulllen);
    strlcat(fullname,HDL_MODULE_SUFFIX,fulllen);

    if ( (ret = dlopen(fullname,flag)) )       /* try filename with suffix next */
    {
        free(fullname);
        return ret;
    }
#endif

    if (hdl_modpath && *hdl_modpath)
    {
        char* filenamecopy = strdup( filename );
        strlcpy( fullname, hdl_modpath,              fulllen );
        strlcat( fullname, PATHSEPS,                 fulllen );
        strlcat( fullname, basename( filenamecopy ), fulllen );
        free( filenamecopy );
    }
    else
        strlcpy(fullname,filename,fulllen);

    if ( (ret = dlopen(fullname,flag)) )
    {
        free(fullname);
        return ret;
    }

#if defined(HDL_MODULE_SUFFIX)
    strlcat(fullname,HDL_MODULE_SUFFIX,fulllen);

    if((ret = dlopen(fullname,flag)))
    {
        free(fullname);
        return ret;
    }
#endif

    return NULL;
}


/* hdl_dvad - register device type
 */
DLL_EXPORT void hdl_dvad (char *devname, DEVHND *devhnd)
{
HDLDEV *newhnd;

    newhnd = malloc(sizeof(HDLDEV));
    newhnd->name = strdup(devname);
    newhnd->hnd = devhnd;
    newhnd->next = hdl_cdll->hndent;
    hdl_cdll->hndent = newhnd;
}


/* hdl_fhnd - find registered device handler
 */
static DEVHND * hdl_fhnd (const char *devname)
{
DLLENT *dllent;
HDLDEV *hndent;

    for(dllent = hdl_dll; dllent; dllent = dllent->dllnext)
    {
        for(hndent = dllent->hndent; hndent; hndent = hndent->next)
        {
            if(!strcasecmp(devname,hndent->name))
            {
                return hndent->hnd;
            }
        }
    }

    return NULL;
}


/* hdl_bdnm - build device module name
 */
static char * hdl_bdnm (const char *ltype)
{
char        *dtname;
unsigned int n;
size_t       m;

    /* Don't forget the extra +1 for the \0 ending.             @PJJ */
    m = strlen(ltype) + sizeof(HDL_HDTP_Q) + 1;
    dtname = malloc(m);
    strlcpy(dtname,HDL_HDTP_Q,m);
    strlcat(dtname,ltype,m);

    for(n = 0; n < strlen(dtname); n++)
        if(isupper(dtname[n]))
            dtname[n] = tolower(dtname[n]);

    return dtname;
}


/* hdl_ghnd - obtain device handler
 */
DLL_EXPORT DEVHND * hdl_ghnd (const char *devtype)
{
DEVHND *hnd;
char *hdtname;
char *ltype;

    if((hnd = hdl_fhnd(devtype)))
        return hnd;

    hdtname = hdl_bdnm(devtype);

    if(hdl_load(hdtname,HDL_LOAD_NOMSG) || !hdl_fhnd(devtype))
    {
        if(hdl_device_type_equates)
        {
            if((ltype = hdl_device_type_equates(devtype)))
            {
                free(hdtname);

                hdtname = hdl_bdnm(ltype);

                hdl_load(hdtname,HDL_LOAD_NOMSG);
            }
        }
    }

    free(hdtname);

    return hdl_fhnd(devtype);
}


/* hdl_list - list all entry points
 */
DLL_EXPORT void hdl_list (int flags)
{
DLLENT *dllent;
MODENT *modent;
char buf[256];
int len;

    for(dllent = hdl_dll; dllent; dllent = dllent->dllnext)
    {
        // "HDL: dll type = %s, name = %s, flags = (%s, %s)"
        WRMSG( HHC01531, "I"
            ,(dllent->flags & HDL_LOAD_MAIN)       ? "main"     : "load"
            ,dllent->name
            ,(dllent->flags & HDL_LOAD_NOUNLOAD)   ? "not unloadable" : "unloadable"
            ,(dllent->flags & HDL_LOAD_WAS_FORCED) ? "forced"   : "not forced" );

        for(modent = dllent->modent; modent; modent = modent->modnext)
            if((flags & HDL_LIST_ALL)
              || !((dllent->flags & HDL_LOAD_MAIN) && !modent->fep))
            {
                // "HDL:  symbol = %s, loadcount = %d%s, owner = %s"
                WRMSG( HHC01532, "I"
                    ,modent->name
                    ,modent->count
                    ,modent->fep ? "" : ", unresolved"
                    ,dllent->name );
            }

        if(dllent->hndent)
        {
        HDLDEV *hndent;
            len = 0;
            for(hndent = dllent->hndent; hndent; hndent = hndent->next)
                len += snprintf(buf + len, sizeof(buf) - len, " %s",hndent->name);
            // "HDL:  devtype(s) =%s"
            WRMSG( HHC01533, "I", buf );

        }

        if(dllent->insent)
        {
        HDLINS *insent;
            for(insent = dllent->insent; insent; insent = insent->next)
            {
                len = 0;
#if defined(_370)
                if(insent->archflags & HDL_INSTARCH_370)
                    len += snprintf(buf + len, sizeof(buf) - len, ", archmode = " _ARCH_370_NAME);
#endif
#if defined(_390)
                if(insent->archflags & HDL_INSTARCH_390)
                    len += snprintf(buf + len, sizeof(buf) - len, ", archmode = " _ARCH_390_NAME);
#endif
#if defined(_900)
                if(insent->archflags & HDL_INSTARCH_900)
                    len += snprintf(buf + len, sizeof(buf) - len, ", archmode = " _ARCH_900_NAME);
#endif
                // "HDL:  instruction = %s, opcode = %4.4X%s"
                WRMSG( HHC01534, "I"
                    ,insent->instname
                    ,insent->opcode
                    ,buf );
            }
        }
    }
}


/* hdl_dlst - list all dependencies
 */
DLL_EXPORT void hdl_dlst (void)
{
HDLDEP *depent;

    for(depent = hdl_depend;
      depent;
      depent = depent->next)
        // "HDL: dependency %s version %s size %d"
        WRMSG( HHC01535,"I",depent->name,depent->version,depent->size );
}


/* hdl_dadd - add dependency
 */
static int hdl_dadd (char *name, char *version, int size)
{
HDLDEP **newdep;

    for (newdep = &(hdl_depend);
        *newdep;
         newdep = &((*newdep)->next));

    (*newdep) = malloc(sizeof(HDLDEP));
    (*newdep)->next    = NULL;
    (*newdep)->name    = strdup(name);
    (*newdep)->version = strdup(version);
    (*newdep)->size    = size;

    return 0;
}


/* hdl_dchk - dependency check
 */
static int hdl_dchk (char *name, char *version, int size)
{
HDLDEP *depent;

    for(depent = hdl_depend;
      depent && strcmp(name,depent->name);
      depent = depent->next);

    if(depent)
    {
        if(strcmp(version,depent->version))
        {
            // "HDL: dependency check failed for %s, version %s expected %s"
            WRMSG( HHC01509, "I",name, version, depent->version );
            return -1;
        }

        if(size != depent->size)
        {
            // "HDL: dependency check failed for %s, size %d expected %d"
            WRMSG( HHC01510, "I", name, size, depent->size );
            return -1;
        }
    }
    else
    {
        hdl_dadd(name,version,size);
    }

    return 0;
}


/* hdl_fent - find entry point
 */
DLL_EXPORT void * hdl_fent (char *name)
{
DLLENT *dllent;
MODENT *modent;
void *fep;

    /* Find entry point and increase loadcount */
    for(dllent = hdl_dll; dllent; dllent = dllent->dllnext)
    {
        for(modent = dllent->modent; modent; modent = modent->modnext)
        {
            if(!strcmp(name,modent->name))
            {
                modent->count++;
                return modent->fep;
            }
        }
    }

    /* If not found then lookup as regular symbol */
    for(dllent = hdl_dll; dllent; dllent = dllent->dllnext)
    {
        if((fep = dlsym(dllent->dll,name)))
        {
            if(!(modent = malloc(sizeof(MODENT))))
            {
                // "HDL: error in function %s: %s"
                WRMSG( HHC01511, "E", "malloc()", strerror(errno) );
                return NULL;
            }

            modent->fep = fep;
            modent->name = strdup(name);
            modent->count = 1;

            /* Insert current entry as first in chain */
            modent->modnext = dllent->modent;
            dllent->modent = modent;

            return fep;
        }
    }

    /* No entry point found */
    return NULL;
}


/* hdl_nent - find next entry point in chain
 */
DLL_EXPORT void * hdl_nent (void *fep)
{
DLLENT *dllent;
MODENT *modent = NULL;
char   *name;

    for(dllent = hdl_dll; dllent; dllent = dllent->dllnext)
    {
        for(modent = dllent->modent; modent; modent = modent->modnext)
        {
            if(modent->fep == fep)
                break;
        }

        if(modent && modent->fep == fep)
            break;
    }

    if(!modent)
        return NULL;

    name = modent->name;

    if(!(modent = modent->modnext))
    {
        if((dllent = dllent->dllnext))
            modent = dllent->modent;
        else
            return NULL;
    }

    /* Find entry point */
    for(; dllent; dllent = dllent->dllnext, modent = dllent->modent)
    {
        for(; modent; modent = modent->modnext)
        {
            if(!strcmp(name,modent->name))
            {
                return modent->fep;
            }
        }
    }

    return NULL;
}


/* hdl_regi - register entry point
 */
static void hdl_regi (char *name, void *fep)
{
MODENT *modent;

    modent = malloc(sizeof(MODENT));

    modent->fep = fep;
    modent->name = strdup(name);
    modent->count = 0;

    modent->modnext = hdl_cdll->modent;
    hdl_cdll->modent = modent;

}


/* hdl_term - process all "HDL_FINAL_SECTION"s
 */
static void hdl_term (void *unused _HDL_UNUSED)
{
DLLENT *dllent;

    if(MLVL(DEBUG))
        // "HDL: begin termination sequence"
        WRMSG( HHC01512, "I" );

    /* Call all final routines, in reverse load order */
    for(dllent = hdl_dll; dllent; dllent = dllent->dllnext)
    {
        if(dllent->hdlfini)
        {
            if(MLVL(DEBUG))
                // "HDL: calling module cleanup routine %s"
                WRMSG( HHC01513, "I", dllent->name );

            (dllent->hdlfini)();

            if(MLVL(DEBUG))
                // "HDL: module cleanup routine %s complete"
                WRMSG( HHC01514, "I", dllent->name );
        }
    }

    if(MLVL(DEBUG))
        // "HDL: termination sequence complete"
        WRMSG( HHC01515, "I" );
}


#if defined(_MSVC_)
/* hdl_lexe - load exe
 */
static int hdl_lexe ()
{
DLLENT *dllent;
MODENT *modent;

    for(dllent = hdl_dll; dllent; dllent = dllent->dllnext);

    dllent->name = strdup("*Main");

    if(!(dllent->dll = (void*)GetModuleHandle( NULL ) ));
    {
        // "HDL: unable to open dll %s: %s"
        WRMSG( HHC01516, "E", dllent->name, dlerror() );
        free(dllent);
        return -1;
    }

    dllent->flags = HDL_LOAD_MAIN;

    if(!(dllent->hdldepc = dlsym(dllent->dll,HDL_DEPC_Q)))
    {
        // "HDL: no dependency section in %s: %s"
        WRMSG( HHC01517, "E", dllent->name, dlerror() );
        free(dllent);
        return -1;
    }

    dllent->hdlinit = dlsym(dllent->dll,HDL_INIT_Q);

    dllent->hdlreso = dlsym(dllent->dll,HDL_RESO_Q);

    dllent->hdlddev = dlsym(dllent->dll,HDL_DDEV_Q);

    dllent->hdldins = dlsym(dllent->dll,HDL_DINS_Q);

    dllent->hdlfini = dlsym(dllent->dll,HDL_FINI_Q);

    /* No modules or device types registered yet */
    dllent->modent = NULL;
    dllent->hndent = NULL;
    dllent->insent = NULL;

    obtain_lock(&hdl_lock);

    if(dllent->hdldepc)
    {
        if((dllent->hdldepc)(&hdl_dchk))
        {
            // "HDL: dependency check failed for module %s"
            WRMSG( HHC01518, "E", dllent->name );
        }
    }

    hdl_cdll = dllent;

    /* Call initializer */
    if(hdl_cdll->hdlinit)
        (dllent->hdlinit)(&hdl_regi);

    /* Insert current entry as first in chain */
    dllent->dllnext = hdl_dll;
    hdl_dll = dllent;

    /* Reset the loadcounts */
    for(dllent = hdl_dll; dllent; dllent = dllent->dllnext)
        for(modent = dllent->modent; modent; modent = modent->modnext)
            modent->count = 0;

    /* Call all resolvers */
    for(dllent = hdl_dll; dllent; dllent = dllent->dllnext)
    {
        if(dllent->hdlreso)
            (dllent->hdlreso)(&hdl_fent);
    }

    /* register any device types */
    if(hdl_cdll->hdlddev)
        (hdl_cdll->hdlddev)(&hdl_dvad);

    /* register any new instructions */
    if(hdl_cdll->hdldins)
        (hdl_cdll->hdldins)(&hdl_didf);

    hdl_cdll = NULL;

    release_lock(&hdl_lock);

    return 0;
}
#endif


/* hdl_main - initialize hercules dynamic loader
 */
DLL_EXPORT void hdl_main (void)
{
HDLPRE *preload;

    initialize_lock(&hdl_lock);

    hdl_sdip = FALSE;

    if ( hdl_modpath == NULL )
    {
        char *def;
        char pathname[MAX_PATH];

        if (!(def = getenv("HERCULES_LIB")))
        {
            if ( sysblk.hercules_pgmpath == NULL || strlen( sysblk.hercules_pgmpath ) == 0 )
            {
                hostpath(pathname, HDL_DEFAULT_PATH, sizeof(pathname));
                hdl_setpath(pathname, TRUE);
            }
            else
            {
#if !defined (MODULESDIR)
                hdl_setpath(sysblk.hercules_pgmpath, TRUE);
#else
                hostpath(pathname, HDL_DEFAULT_PATH, sizeof(pathname));
                hdl_setpath(pathname, TRUE);
#endif
            }
        }
        else
        {
            hostpath(pathname, def, sizeof(pathname));
            hdl_setpath(pathname, TRUE);
        }
    }

    dlinit();

    if(!(hdl_cdll = hdl_dll = malloc(sizeof(DLLENT))))
    {
        char buf[64];
        MSGBUF( buf,  "malloc(%d)", (int)sizeof(DLLENT));
        fprintf(stderr, MSG(HHC01511, "E", buf, strerror(errno)));
        exit(1);
    }

    hdl_cdll->name = strdup("*Hercules");

/* This was a nice trick. Unfortunately, on some platforms  */
/* it becomes impossible. Some platforms need fully defined */
/* DLLs, some other platforms do not allow dlopen(self)     */

#if 1

    if(!(hdl_cdll->dll = hdl_dlopen(NULL, RTLD_NOW )))
    {
        fprintf(stderr, MSG(HHC01516, "E", "hercules", dlerror()));
        exit(1);
    }

    hdl_cdll->flags = HDL_LOAD_MAIN | HDL_LOAD_NOUNLOAD;

    if(!(hdl_cdll->hdldepc = dlsym(hdl_cdll->dll,HDL_DEPC_Q)))
    {
        fprintf(stderr, MSG(HHC01517, "E", hdl_cdll->name, dlerror()));
        exit(1);
    }

    hdl_cdll->hdlinit = dlsym(hdl_cdll->dll,HDL_INIT_Q);

    hdl_cdll->hdlreso = dlsym(hdl_cdll->dll,HDL_RESO_Q);

    hdl_cdll->hdlddev = dlsym(hdl_cdll->dll,HDL_DDEV_Q);

    hdl_cdll->hdldins = dlsym(hdl_cdll->dll,HDL_DINS_Q);

    hdl_cdll->hdlfini = dlsym(hdl_cdll->dll,HDL_FINI_Q);
#else

    hdl_cdll->flags = HDL_LOAD_MAIN | HDL_LOAD_NOUNLOAD;

    hdl_cdll->hdldepc = &HDL_DEPC;

    hdl_cdll->hdlinit = &HDL_INIT;

    hdl_cdll->hdlreso = &HDL_RESO;

    hdl_cdll->hdlddev = &HDL_DDEV;

    hdl_cdll->hdldins = &HDL_DINS;

    hdl_cdll->hdlfini = &HDL_FINI;
#endif

    /* No modules or device types registered yet */
    hdl_cdll->modent = NULL;
    hdl_cdll->hndent = NULL;
    hdl_cdll->insent = NULL;

    /* No dll's loaded yet */
    hdl_cdll->dllnext = NULL;

    obtain_lock(&hdl_lock);

    if(hdl_cdll->hdldepc)
        (hdl_cdll->hdldepc)(&hdl_dadd);

    if(hdl_cdll->hdlinit)
        (hdl_cdll->hdlinit)(&hdl_regi);

    if(hdl_cdll->hdlreso)
        (hdl_cdll->hdlreso)(&hdl_fent);

    if(hdl_cdll->hdlddev)
        (hdl_cdll->hdlddev)(&hdl_dvad);

    if(hdl_cdll->hdldins)
        (hdl_cdll->hdldins)(&hdl_didf);

    release_lock(&hdl_lock);

    /* Register termination exit */
    hdl_adsc( "hdl_term", hdl_term, NULL);

    for(preload = hdl_preload; preload->name; preload++)
        hdl_load(preload->name, preload->flag);

#if defined(_MSVC_) && 0
    hdl_lexe();
#endif
}


/* hdl_load - load a dll
 */
DLL_EXPORT int hdl_load (char *name,int flags)
{
DLLENT *dllent, *tmpdll;
MODENT *modent;
char *modname;

    modname = (modname = strrchr(name,'/')) ? modname+1 : name;

    for(dllent = hdl_dll; dllent; dllent = dllent->dllnext)
    {
        if(strfilenamecmp(modname,dllent->name) == 0)
        {
            // "HDL: module %s already loaded"
            WRMSG( HHC01519, "E", dllent->name );
            return -1;
        }
    }

    if(!(dllent = malloc(sizeof(DLLENT))))
    {
        // "HDL: error in function %s: %s"
        WRMSG( HHC01511, "E", "malloc()", strerror(errno) );
        return -1;
    }

    dllent->name = strdup(modname);

    if(!(dllent->dll = hdl_dlopen(name, RTLD_NOW)))
    {
        if(!(flags & HDL_LOAD_NOMSG))
            // "HDL: unable to open dll %s: %s"
            WRMSG( HHC01516, "E", name, dlerror() );
        free(dllent);
        return -1;
    }

    dllent->flags = (flags & (~HDL_LOAD_WAS_FORCED));

    if(!(dllent->hdldepc = dlsym(dllent->dll,HDL_DEPC_Q)))
    {
        // "HDL: no dependency section in %s: %s"
        WRMSG( HHC01517, "E", dllent->name, dlerror() );
        dlclose(dllent->dll);
        free(dllent);
        return -1;
    }

    for(tmpdll = hdl_dll; tmpdll; tmpdll = tmpdll->dllnext)
    {
        if(tmpdll->hdldepc == dllent->hdldepc)
        {
            // "HDL: dll %s is duplicate of %s"
            WRMSG( HHC01520, "E", dllent->name, tmpdll->name );
            dlclose(dllent->dll);
            free(dllent);
            return -1;
        }
    }


    dllent->hdlinit = dlsym(dllent->dll,HDL_INIT_Q);

    dllent->hdlreso = dlsym(dllent->dll,HDL_RESO_Q);

    dllent->hdlddev = dlsym(dllent->dll,HDL_DDEV_Q);

    dllent->hdldins = dlsym(dllent->dll,HDL_DINS_Q);

    dllent->hdlfini = dlsym(dllent->dll,HDL_FINI_Q);

    /* No modules or device types registered yet */
    dllent->modent = NULL;
    dllent->hndent = NULL;
    dllent->insent = NULL;

    obtain_lock(&hdl_lock);

    if(dllent->hdldepc)
    {
        if((dllent->hdldepc)(&hdl_dchk))
        {
            // "HDL: dependency check failed for module %s"
            WRMSG( HHC01518, "E", dllent->name );
            if(!(flags & HDL_LOAD_FORCE))
            {
                dlclose(dllent->dll);
                free(dllent);
                release_lock(&hdl_lock);
                return -1;
            }
            dllent->flags |= HDL_LOAD_WAS_FORCED;
        }
    }

    hdl_cdll = dllent;

    /* Call initializer */
    if(hdl_cdll->hdlinit)
        (dllent->hdlinit)(&hdl_regi);

    /* Insert current entry as first in chain */
    dllent->dllnext = hdl_dll;
    hdl_dll = dllent;

    /* Reset the loadcounts */
    for(dllent = hdl_dll; dllent; dllent = dllent->dllnext)
        for(modent = dllent->modent; modent; modent = modent->modnext)
            modent->count = 0;

    /* Call all resolvers */
    for(dllent = hdl_dll; dllent; dllent = dllent->dllnext)
    {
        if(dllent->hdlreso)
            (dllent->hdlreso)(&hdl_fent);
    }

    /* register any device types */
    if(hdl_cdll->hdlddev)
        (hdl_cdll->hdlddev)(&hdl_dvad);

    /* register any new instructions */
    if(hdl_cdll->hdldins)
        (hdl_cdll->hdldins)(&hdl_didf);

    hdl_cdll = NULL;

    release_lock(&hdl_lock);

    return 0;
}


/* hdl_dele - unload a dll
 */
DLL_EXPORT int hdl_dele (char *name)
{
DLLENT **dllent, *tmpdll;
MODENT *modent, *tmpmod;
DEVBLK *dev;
HDLDEV *hnd;
HDLINS *ins;
char *modname;

    modname = (modname = strrchr(name,'/')) ? modname+1 : name;

    obtain_lock(&hdl_lock);

    for(dllent = &(hdl_dll); *dllent; dllent = &((*dllent)->dllnext))
    {
        if(strfilenamecmp(modname,(*dllent)->name) == 0)
        {
            if((*dllent)->flags & (HDL_LOAD_MAIN | HDL_LOAD_NOUNLOAD))
            {
                release_lock(&hdl_lock);
                // "HDL: unloading of module %s not allowed"
                WRMSG( HHC01521, "E", (*dllent)->name );
                return -1;
            }

            for(dev = sysblk.firstdev; dev; dev = dev->nextdev)
                if (IS_DEV( dev ))
                    for(hnd = (*dllent)->hndent; hnd; hnd = hnd->next)
                        if(hnd->hnd == dev->hnd)
                        {
                            release_lock(&hdl_lock);
                            // "HDL: module %s bound to device %1d:%04X"
                            WRMSG( HHC01522, "E",(*dllent)->name, SSID_TO_LCSS(dev->ssid), dev->devnum );
                            return -1;
                        }

            /* Call dll close routine */
            if((*dllent)->hdlfini)
            {
            int rc;

                if((rc = ((*dllent)->hdlfini)()))
                {
                    release_lock(&hdl_lock);
                    // "HDL: unload of module %s rejected by final section"
                    WRMSG( HHC01523, "E", (*dllent)->name );
                    return rc;
                }
            }

            modent = (*dllent)->modent;
            while(modent)
            {
                tmpmod = modent;

                /* remove current entry from chain */
                modent = modent->modnext;

                /* free module resources */
                free(tmpmod->name);
                free(tmpmod);
            }

            tmpdll = *dllent;

            /* remove current entry from chain */
            *dllent = (*dllent)->dllnext;

            for(hnd = tmpdll->hndent; hnd;)
            {
            HDLDEV *nexthnd;
                free(hnd->name);
                nexthnd = hnd->next;
                free(hnd);
                hnd = nexthnd;
            }

            for(ins = tmpdll->insent; ins;)
            {
            HDLINS *nextins;

                hdl_modify_opcode(FALSE, ins);
                free(ins->instname);
                nextins = ins->next;
                free(ins);
                ins = nextins;
            }

//          dlclose(tmpdll->dll);

            /* free dll resources */
            free(tmpdll->name);
            free(tmpdll);

            /* Reset the loadcounts */
            for(tmpdll = hdl_dll; tmpdll; tmpdll = tmpdll->dllnext)
                for(tmpmod = tmpdll->modent; tmpmod; tmpmod = tmpmod->modnext)
                    tmpmod->count = 0;

            /* Call all resolvers */
            for(tmpdll = hdl_dll; tmpdll; tmpdll = tmpdll->dllnext)
            {
                if(tmpdll->hdlreso)
                    (tmpdll->hdlreso)(&hdl_fent);
            }

            release_lock(&hdl_lock);

            return 0;
        }

    }

    release_lock(&hdl_lock);

    // "HDL: module %s not found"
    WRMSG( HHC01524, "E", modname );

    return -1;
}


static void hdl_modify_opcode(int insert, HDLINS *instr)
{
  if(insert)
  {
#ifdef _370
    if(instr->archflags & HDL_INSTARCH_370)
      instr->original = replace_opcode(ARCH_370, instr->instruction, instr->opcode >> 8, instr->opcode & 0x00ff);
#endif
#ifdef _390
    if(instr->archflags & HDL_INSTARCH_390)
      instr->original = replace_opcode(ARCH_390, instr->instruction, instr->opcode >> 8, instr->opcode & 0x00ff);
#endif
#ifdef _900
    if(instr->archflags & HDL_INSTARCH_900)
      instr->original = replace_opcode(ARCH_900, instr->instruction, instr->opcode >> 8, instr->opcode & 0x00ff);
#endif
  }
  else
  {
#ifdef _370
    if(instr->archflags & HDL_INSTARCH_370)
      replace_opcode(ARCH_370, instr->original, instr->opcode >> 8, instr->opcode & 0x00ff);
#endif
#ifdef _390
    if(instr->archflags & HDL_INSTARCH_390)
      replace_opcode(ARCH_390, instr->original, instr->opcode >> 8, instr->opcode & 0x00ff);
#endif
#ifdef _900
    if(instr->archflags & HDL_INSTARCH_900)
      replace_opcode(ARCH_900, instr->original, instr->opcode >> 8, instr->opcode & 0x00ff);
#endif
  }
  return;
}


/* hdl_didf - Define instruction call */
static void hdl_didf (int archflags, int opcode, char *name, void *routine)
{
HDLINS *newins;

    newins = malloc(sizeof(HDLINS));
    newins->opcode = opcode > 0xff ? opcode : (opcode << 8) ;
    newins->archflags = archflags;
    newins->instname = strdup(name);
    newins->instruction = routine;
    newins->next = hdl_cdll->insent;
    hdl_cdll->insent = newins;
    hdl_modify_opcode(TRUE, newins);
}

#endif /*defined(OPTION_DYNAMIC_LOAD)*/
