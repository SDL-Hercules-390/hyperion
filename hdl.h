/* HDL.H        (c) Copyright Jan Jaeger, 2003-2012                  */
/*              Hercules Dynamic Loader                              */

#ifndef _HDL_H
#define _HDL_H

#include "hercules.h"

#if !defined(_MSVC_)
  #define _HDL_UNUSED __attribute__ ((unused))
#else
  #define _HDL_UNUSED
#endif

#ifndef _HDL_C_
#ifndef _HUTIL_DLL_
#define HDL_DLL_IMPORT DLL_IMPORT
#else   /* _HUTIL_DLL_ */
#define HDL_DLL_IMPORT extern
#endif  /* _HUTIL_DLL_ */
#else
#define HDL_DLL_IMPORT DLL_EXPORT
#endif

#ifndef _HDLMAIN_C_
#ifndef _HENGINE_DLL_
#define HDM_DLL_IMPORT DLL_IMPORT
#else   /* _HENGINE_DLL_ */
#define HDM_DLL_IMPORT extern
#endif  /* _HENGINE_DLL_ */
#else
#define HDM_DLL_IMPORT DLL_EXPORT
#endif

/*********************************************************************/

struct _HDLSHD;
typedef struct _HDLSHD {
    struct _HDLSHD *next;
    char* shdname;                      /* identifying name          */
    void (*shdcall) (void *);           /* Entry to be called        */
    void *shdarg;                       /* Optional argument         */
} HDLSHD;


HDL_DLL_IMPORT void    hdl_adsc(char*, void *, void *);/* Add shutdown routine      */
HDL_DLL_IMPORT int     hdl_rmsc(void *, void *);       /* Remove shutdown routine   */
HDL_DLL_IMPORT void    hdl_shut(void);                 /* Call all shutdown routines*/

DLL_EXPORT DEVHND *hdl_ghnd(const char *devname);        /* Get device handler        */

/*********************************************************************/

#if !defined(OPTION_DYNAMIC_LOAD)

#define HDL_DEVICE_SECTION                          \
DLL_EXPORT DEVHND *hdl_ghnd(const char *devtype)    \
{

#define HDL_DEVICE( _devname, _devhnd )             \
    if(!strcasecmp( QSTR(_devname), devtype ))      \
        return &(_devhnd);

#define END_DEVICE_SECTION                          \
    return NULL;                                    \
}

#else /* defined(OPTION_DYNAMIC_LOAD) */

/*********************************************************************/

#if !defined(HDL_USE_LIBTOOL)
  #define dlinit()
#else
  #define dlinit()                lt_dlinit()
  #define dlopen(_name, _flags)   lt_dlopen(_name)
  #define dlsym(_handle, _symbol) lt_dlsym(_handle, _symbol)
  #define dlclose(_handle)        lt_dlclose(_handle)
  #define dlerror()               lt_dlerror()
  #define RTLD_NOW                0
#endif

// extern char *(*hdl_device_type_equates)(char *);

typedef struct _HDLDEV {                /* Device entry              */
    char *name;                         /* Device type name          */
    DEVHND *hnd;                        /* Device handlers           */
    struct _HDLDEV *next;               /* Next entry                */
} HDLDEV;


typedef struct _HDLINS {                /* Instruction entry         */
    int  opcode;                        /* Opcode                    */
    int  archflags;                     /* Architecture flags        */
    char *instname;                     /* Instruction name          */
    void *instruction;                  /* Instruction routine       */
    void *original;                     /* Original instruction      */
    struct _HDLINS *next;               /* Next entry                */
} HDLINS;


struct _HDLDEP;
typedef struct _HDLDEP {                /* Dependency entry          */
    char *name;                         /* Dependency name           */
    char *version;                      /* Version                   */
    int  size;                          /* Structure/module size     */
    struct _HDLDEP *next;               /* Next entry                */
} HDLDEP;


typedef struct _HDLPRE {                /* Preload list entry        */
    char *name;                         /* Module name               */
    int  flag;                          /* Load flags                */
} HDLPRE;


struct _MODENT;
typedef struct _MODENT {                /* External Symbol entry     */
    void (*fep)();                      /* Function entry point      */
    char *name;                         /* Function symbol name      */
    int  count;                         /* Symbol load count         */
    struct _MODENT *modnext;            /* Next entry in chain       */
} MODENT;


struct _DLLENT;
typedef struct _DLLENT {                /* DLL entry                 */
    char *name;                         /* load module name          */
    void *dll;                          /* DLL handle (dlopen)       */
    int flags;                          /* load flags                */
    int (*hdldepc)(void *);             /* hdl_depc                  */
    int (*hdlreso)(void *);             /* hdl_reso                  */
    int (*hdlinit)(void *);             /* hdl_init                  */
    int (*hdlddev)(void *);             /* hdl_ddev                  */
    int (*hdldins)(void *);             /* hdl_dins                  */
    int (*hdlfini)();                   /* hdl_fini                  */
    struct _MODENT *modent;             /* First symbol entry        */
    struct _HDLDEV *hndent;             /* First device entry        */
    struct _HDLINS *insent;             /* First instruction entry   */
    struct _DLLENT *dllnext;            /* Next entry in chain       */
} DLLENT;


/* SHLIBEXT defined by ISW in configure.ac/config.h */
#if defined( HDL_BUILD_SHARED ) && defined( LTDL_SHLIB_EXT )
  #define   HDL_MODULE_SUFFIX   LTDL_SHLIB_EXT
#else
  #if defined( LT_MODULE_EXT )
    #define HDL_MODULE_SUFFIX   LT_MODULE_EXT
  #elif defined( _MSVC_ )
    #define HDL_MODULE_SUFFIX   ".dll"
  #else
    #define HDL_MODULE_SUFFIX   ".la"
  #endif
#endif


#if defined( HDL_MODULE_SUFFIX )
 #define HDL_SUFFIX_LENGTH (sizeof(HDL_MODULE_SUFFIX) - 1)
#else
 #define HDL_SUFFIX_LENGTH 0
#endif


DLL_EXPORT
int hdl_load(char *, int);              /* load dll                  */
#define HDL_LOAD_DEFAULT     0x00000000
#define HDL_LOAD_MAIN        0x00000001 /* Hercules MAIN module flag */
#define HDL_LOAD_NOUNLOAD    0x00000002 /* Module cannot be unloaded */
#define HDL_LOAD_FORCE       0x00000004 /* Override dependency check */
#define HDL_LOAD_NOMSG       0x00000008 /* Do not issue not found msg*/
#define HDL_LOAD_WAS_FORCED  0x00000010 /* Module load was forced    */

#define HDL_INSTARCH_370     0x00000001
#define HDL_INSTARCH_390     0x00000002
#define HDL_INSTARCH_900     0x00000004
#define HDL_INSTARCH_ALL     (HDL_INSTARCH_370|HDL_INSTARCH_390|HDL_INSTARCH_900)

DLL_EXPORT
int hdl_dele(char *);                   /* Unload dll                */
DLL_EXPORT
void hdl_list(int);                     /* list all loaded modules   */
#define HDL_LIST_DEFAULT     0x00000000
#define HDL_LIST_ALL         0x00000001 /* list all references       */
DLL_EXPORT
void hdl_dlst();                        /* list all dependencies     */

DLL_EXPORT
void hdl_main();                        /* Main initialization rtn   */

DLL_EXPORT
void hdl_initpath( const char* );       /* Initialize module path    */
DLL_EXPORT
int hdl_setpath( const char* );         /* Set module path (-1/+1/0) */
DLL_EXPORT
const char* hdl_getpath();              /* Return module path        */

DLL_EXPORT
void * hdl_fent(char *);                /* Find entry name           */
DLL_EXPORT
void * hdl_nent(void *);                /* Find next in chain        */

/* The following statement should be void *(*unresolved)(void) = NULL*/
static void **unresolved _HDL_UNUSED = NULL;
#define UNRESOLVED *unresolved


#define HDL_DEPC        hdl_depc
#define HDL_RESO        hdl_reso
#define HDL_INIT        hdl_init
#define HDL_FINI        hdl_fini
#define HDL_DDEV        hdl_ddev
#define HDL_DINS        hdl_dins

#define HDL_HDTP        hdt

#define HDL_DEPC_Q      QSTR( HDL_DEPC )
#define HDL_RESO_Q      QSTR( HDL_RESO )
#define HDL_INIT_Q      QSTR( HDL_INIT )
#define HDL_FINI_Q      QSTR( HDL_FINI )
#define HDL_DDEV_Q      QSTR( HDL_DDEV )
#define HDL_DINS_Q      QSTR( HDL_DINS )
#define HDL_HDTP_Q      QSTR( HDL_HDTP )


#define HDL_FINDSYM(_name)      \
    hdl_fent( (_name) )

#define HDL_FINDNXT(_ep)        \
    hdl_nent( &(_ep) )


#define HDL_DEPENDENCY_SECTION \
DLL_EXPORT int HDL_DEPC(int (*hdl_depc_vers)(char *, char *, int) _HDL_UNUSED ) \
{ \
int hdl_depc_rc = 0;

#define HDL_DEPENDENCY(_comp) \
    if (hdl_depc_vers( QSTR(_comp), HDL_VERS_ ## _comp, HDL_SIZE_ ## _comp)) \
        hdl_depc_rc = 1;

#define END_DEPENDENCY_SECTION                          \
return hdl_depc_rc; }


#define HDL_REGISTER_SECTION                            \
DLL_EXPORT void HDL_INIT(int (*hdl_init_regi)(char *, void *) _HDL_UNUSED ) \
{

/*       register this epname, as ep = addr of this var or func...   */
#define HDL_REGISTER( _epname, _varname )               \
    (hdl_init_regi)( QSTR(_epname), &(_varname) );

#define END_REGISTER_SECTION                            \
}


#define HDL_DEVICE_SECTION                              \
DLL_EXPORT void HDL_DDEV(int (*hdl_init_ddev)(char *, void *) _HDL_UNUSED ) \
{

#define HDL_DEVICE( _devname, _devhnd )                 \
    (hdl_init_ddev)( QSTR(_devname), &(_devhnd) );

#define END_DEVICE_SECTION                              \
}


#define HDL_INSTRUCTION_SECTION                         \
DLL_EXPORT void HDL_DINS(int (*hdl_init_dins)(int, int, void *, void *) _HDL_UNUSED ) \
{

#if defined(_370)
 #define HDL_370_DEFINST( _arch, _opcode, _instruction)                               \
do {                                                                                  \
  if( (_arch) & HDL_INSTARCH_370 )                                                    \
    (hdl_init_dins)( _arch, _opcode, QSTR(_instruction), &(s370_ ## _instruction) );  \
} while(0);
#else
 #define HDL_370_DEFINST( _arch, _opcode, _instruction)
#endif

#if defined(_390)
 #define HDL_390_DEFINST( _arch, _opcode, _instruction)                               \
do {                                                                                  \
  if( (_arch) & HDL_INSTARCH_390 )                                                    \
    (hdl_init_dins)( _arch, _opcode, QSTR(_instruction), &(s390_ ## _instruction) );  \
} while(0);
#else
 #define HDL_390_DEFINST( _arch, _opcode, _instruction)
#endif

#if defined(_900)
 #define HDL_900_DEFINST( _arch, _opcode, _instruction)                               \
do {                                                                                  \
  if( (_arch) & HDL_INSTARCH_900 )                                                    \
    (hdl_init_dins)( _arch, _opcode, QSTR(_instruction), &(z900_ ## _instruction) );  \
} while(0);
#else
 #define HDL_900_DEFINST( _arch, _opcode, _instruction)
#endif

#define HDL_DEFINST( _arch, _opcode, _instruction )                        \
do {                                                                       \
    HDL_370_DEFINST(( (_arch) & HDL_INSTARCH_370), _opcode, _instruction); \
    HDL_390_DEFINST(( (_arch) & HDL_INSTARCH_390), _opcode, _instruction); \
    HDL_900_DEFINST(( (_arch) & HDL_INSTARCH_900), _opcode, _instruction); \
} while(0);

#define END_INSTRUCTION_SECTION                         \
}

#define HDL_RESOLVER_SECTION                                          \
DLL_EXPORT void HDL_RESO(void *(*hdl_reso_fent)(char *) _HDL_UNUSED ) \
{

#define HDL_RESOLVE(_name)                              \
    (_name) = (hdl_reso_fent)(QSTR(_name));

/*                  set this ptrvar, to this ep value...             */
#define HDL_RESOLVE_PTRVAR( _ptrvar, _epname )          \
    (_ptrvar) = (hdl_reso_fent)(QSTR(_epname));

#define END_RESOLVER_SECTION                            \
}


#define HDL_FINAL_SECTION                               \
DLL_EXPORT int HDL_FINI()                               \
{                                                       \
    int rc = 0;

#define END_FINAL_SECTION                               \
    return rc;                                          \
}

#endif /* defined(OPTION_DYNAMIC_LOAD) */


#define PROGRAM_INTERRUPT(_regs, _interruption_code) \
        ((_regs)->program_interrupt) ((_regs),(_interruption_code))


/*********************************************************************/

#endif /* _HDL_H */
