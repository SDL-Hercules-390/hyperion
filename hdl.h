/* HDL.H        (C) Copyright Jan Jaeger, 2003-2012                  */
/*              Hercules Dynamic Loader                              */

#ifndef _HDL_H
#define _HDL_H

#include "hercules.h"

/*-------------------------------------------------------------------*/

#ifndef _HDL_C_
  #ifndef _HUTIL_DLL_
    #define HDL_DLL_IMPORT    DLL_IMPORT
  #else
    #define HDL_DLL_IMPORT    extern
  #endif
#else
  #define   HDL_DLL_IMPORT    DLL_EXPORT
#endif

/*-------------------------------------------------------------------*/

#ifndef _HDLMAIN_C_
  #ifndef _HENGINE_DLL_
    #define HDM_DLL_IMPORT    DLL_IMPORT
  #else
    #define HDM_DLL_IMPORT    extern
  #endif
#else
  #define   HDM_DLL_IMPORT    DLL_EXPORT
#endif

/*-------------------------------------------------------------------*/
/*                       Shutdown handling                           */
/*-------------------------------------------------------------------*/

struct HDLSHUT;
typedef struct HDLSHUT   HDLSHUT;
typedef void   SHUTFUNC( void* );

struct HDLSHUT
{
    HDLSHUT*   next;            // Point to next entry in chain
    char*      shutname;        // function name
    SHUTFUNC*  shutfunc;        // function pointer
    void*      shutarg;         // function argument
};

/*-------------------------------------------------------------------*/

HDL_DLL_IMPORT void     hdl_atexit   ( void );                  /* Call all shutdown routines*/
HDL_DLL_IMPORT void     hdl_addshut  ( char*, void*, void* );   /* Add shutdown routine      */
HDL_DLL_IMPORT int      hdl_delshut  ( void*, void* );          /* Remove shutdown routine   */

/*-------------------------------------------------------------------*/

DLL_EXPORT int          hdl_main     ();                        /* Main initialization rtn   */
DLL_EXPORT void         hdl_initpath ( const char* );           /* Initialize module path    */
DLL_EXPORT int          hdl_setpath  ( const char* );           /* Set module path (-1/+1/0) */
DLL_EXPORT const char*  hdl_getpath  ();                        /* Return module path        */
DLL_EXPORT int          hdl_load     ( char*, int );            /* load dll                  */
DLL_EXPORT int          hdl_unload   ( char* );                 /* Unload dll                */
DLL_EXPORT void         hdl_listmods ( int );                   /* list all loaded modules   */
DLL_EXPORT void         hdl_listdeps ();                        /* list all dependencies     */
DLL_EXPORT DEVHND*      hdl_getdev   ( const char* devtype );   /* Get device handler        */
DLL_EXPORT void*        hdl_findsym  ( char* );                 /* Find entry name           */
DLL_EXPORT void*        hdl_next     ( void* );                 /* Find next entry in chain  */

/*-------------------------------------------------------------------*/
#if !defined( OPTION_DYNAMIC_LOAD )
/*-------------------------------------------------------------------*/

#define HDL_DEVICE_SECTION                                          \
                                                                    \
    DLL_EXPORT  DEVHND* hdl_getdev( const char* devtype ) {

/*-------------------------------------------------------------------*/

#define HDL_DEVICE( typename, devhnd )                              \
                                                                    \
    do                                                              \
    {                                                               \
        if (strcasecmp( QSTR( typename ), devtype ) == 0)           \
            return &devhnd;                                         \
    }                                                               \
    while (0)

/*-------------------------------------------------------------------*/

#define END_DEVICE_SECTION          return NULL; }

/*-------------------------------------------------------------------*/
#else /* defined( OPTION_DYNAMIC_LOAD ) */
/*-------------------------------------------------------------------*/

#if !defined( HDL_USE_LIBTOOL )
  #define dlinit()
#else
  #define dlinit()                  lt_dlinit()
  #define dlopen(_name, _flags)     lt_dlopen(_name)
  #define dlsym(_handle, _symbol)   lt_dlsym(_handle, _symbol)
  #define dlclose(_handle)          lt_dlclose(_handle)
  #define dlerror()                 lt_dlerror()
  #define RTLD_NOW                  0
#endif

/*-------------------------------------------------------------------*/

typedef struct _HDLDEV {                /* Device entry              */
    char *name;                         /* Device type name          */
    DEVHND *hnd;                        /* Device handlers           */
    struct _HDLDEV *next;               /* Next entry                */
} HDLDEV;


/*-------------------------------------------------------------------*/

typedef struct _HDLINS {                /* Instruction entry         */
    int  opcode;                        /* Opcode                    */
    int  archflags;                     /* Architecture flags        */
    char *instname;                     /* Instruction name          */
    void *instruction;                  /* Instruction routine       */
    void *original;                     /* Original instruction      */
    struct _HDLINS *next;               /* Next entry                */
} HDLINS;

/*-------------------------------------------------------------------*/

struct _HDLDEP;
typedef struct _HDLDEP {                /* Dependency entry          */
    char *name;                         /* Dependency name           */
    char *version;                      /* Version                   */
    int  size;                          /* Structure/module size     */
    struct _HDLDEP *next;               /* Next entry                */
} HDLDEP;

/*-------------------------------------------------------------------*/

typedef struct _HDLPRE {                /* Preload list entry        */
    char *name;                         /* Module name               */
    int  flag;                          /* Load flags                */
} HDLPRE;

/*-------------------------------------------------------------------*/

struct _MODENT;
typedef struct _MODENT {                /* External Symbol entry     */
    void (*fep)();                      /* Function entry point      */
    char *name;                         /* Function symbol name      */
    int  count;                         /* Symbol load count         */
    struct _MODENT *modnext;            /* Next entry in chain       */
} MODENT;

/*-------------------------------------------------------------------*/

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

/*-------------------------------------------------------------------*/

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

/*-------------------------------------------------------------------*/

#if defined( HDL_MODULE_SUFFIX )
 #define HDL_SUFFIX_LENGTH  (sizeof(HDL_MODULE_SUFFIX) - 1)
#else
 #define HDL_SUFFIX_LENGTH  0
#endif

/*-------------------------------------------------------------------*/

#define HDL_LOAD_DEFAULT     0x00000000
#define HDL_LOAD_MAIN        0x00000001 /* Hercules MAIN module flag */
#define HDL_LOAD_NOUNLOAD    0x00000002 /* Module cannot be unloaded */
#define HDL_LOAD_FORCE       0x00000004 /* Override dependency check */
#define HDL_LOAD_NOMSG       0x00000008 /* Do not issue not found msg*/
#define HDL_LOAD_WAS_FORCED  0x00000010 /* Module load was forced    */

#define HDL_INSTARCH_370     0x00000001
#define HDL_INSTARCH_390     0x00000002
#define HDL_INSTARCH_900     0x00000004
#define HDL_INSTARCH_ALL     (HDL_INSTARCH_370 | HDL_INSTARCH_390 | HDL_INSTARCH_900)

#define HDL_LIST_DEFAULT     0x00000000
#define HDL_LIST_ALL         0x00000001 /* list all references       */

/*-------------------------------------------------------------------*/

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

/*-------------------------------------------------------------------*/
/*                   HDL_DEPENDENCY_SECTION                          */
/*-------------------------------------------------------------------*/

#define HDL_DEPENDENCY_SECTION                                      \
                                                                    \
    DLL_EXPORT int HDL_DEPC                                         \
    (                                                               \
        int (*hdl_depc_vers)( char*, char*, int )                   \
    )                                                               \
    {                                                               \
        int hdl_depc_rc = 0;                                        \
        UNREFERENCED( hdl_depc_vers );

/*-------------------------------------------------------------------*/

#define HDL_DEPENDENCY( dep )                                       \
                                                                    \
    do                                                              \
    {                                                               \
        if (hdl_depc_vers                                           \
        (                                                           \
            QSTR( dep ),                                            \
            HDL_VERS_ ## dep,                                       \
            HDL_SIZE_ ## dep                                        \
        ))                                                          \
        {                                                           \
            hdl_depc_rc = 1;                                        \
        }                                                           \
    }                                                               \
    while (0)

/*-------------------------------------------------------------------*/

#define END_DEPENDENCY_SECTION          return hdl_depc_rc; }

/*-------------------------------------------------------------------*/
/*                    HDL_REGISTER_SECTION                           */
/*-------------------------------------------------------------------*/

#define HDL_REGISTER_SECTION                                        \
                                                                    \
    DLL_EXPORT void HDL_INIT                                        \
    (                                                               \
        int (*hdl_init_regi)( char*, void* )                        \
    )                                                               \
    {                                                               \
        {UNREFERENCED( hdl_init_regi );}

/*-------------------------------------------------------------------*/

#define HDL_REGISTER( epname, varname )                             \
                                                                    \
    hdl_init_regi( QSTR( epname ), &varname );

/*-------------------------------------------------------------------*/

#define END_REGISTER_SECTION            }

/*-------------------------------------------------------------------*/
/*                     HDL_RESOLVER_SECTION                          */
/*-------------------------------------------------------------------*/

#define HDL_RESOLVER_SECTION                                        \
                                                                    \
    DLL_EXPORT void HDL_RESO                                        \
    (                                                               \
        void* (*hdl_reso_fent)( char* )                             \
    )                                                               \
    {                                                               \
        {UNREFERENCED( hdl_reso_fent );}

/*-------------------------------------------------------------------*/

#define HDL_RESOLVE( sym )          sym = hdl_reso_fent( QSTR( sym ));

/*-------------------------------------------------------------------*/

#define HDL_RESOLVE_PTRVAR( psym, sym )                             \
                                                                    \
    psym = hdl_reso_fent( QSTR( sym ));

/*-------------------------------------------------------------------*/

#define END_RESOLVER_SECTION        }

/*-------------------------------------------------------------------*/
/*                    HDL_DEVICE_SECTION                             */
/*-------------------------------------------------------------------*/

#define HDL_DEVICE_SECTION                                          \
                                                                    \
    DLL_EXPORT void HDL_DDEV                                        \
    (                                                               \
        int (*hdl_init_ddev)( char*, void* )                        \
    )                                                               \
    {                                                               \
        {UNREFERENCED( hdl_init_ddev );}

/*-------------------------------------------------------------------*/

#define HDL_DEVICE( typename, devhnd )                              \
                                                                    \
    hdl_init_ddev( QSTR( typename ), &devhnd );

/*-------------------------------------------------------------------*/

#define END_DEVICE_SECTION              }

/*-------------------------------------------------------------------*/
/*                      HDL_370_DEF_INST                             */
/*-------------------------------------------------------------------*/

#if defined(_370)
 #define HDL_370_DEF_INST( arch, opcode, instrfunc )                \
                                                                    \
    do                                                              \
    {                                                               \
        if (arch & HDL_INSTARCH_370)                                \
            hdl_init_dins( arch, opcode,                            \
                QSTR( instrfunc ), &s370_ ## instrfunc );           \
    }                                                               \
    while (0)

#else
 #define HDL_370_DEF_INST( arch, opcode, instrfunc)
#endif

/*-------------------------------------------------------------------*/
/*                      HDL_390_DEF_INST                             */
/*-------------------------------------------------------------------*/

#if defined(_390)
 #define HDL_390_DEF_INST( arch, opcode, instrfunc )                \
                                                                    \
    do                                                              \
    {                                                               \
        if (arch & HDL_INSTARCH_390)                                \
            hdl_init_dins( arch, opcode,                            \
                QSTR( instrfunc ), &s390_ ## instrfunc );           \
    }                                                               \
    while (0)

#else
 #define HDL_390_DEF_INST( arch, opcode, instrfunc)
#endif

/*-------------------------------------------------------------------*/
/*                      HDL_900_DEF_INST                             */
/*-------------------------------------------------------------------*/

#if defined(_900)
 #define HDL_900_DEF_INST( arch, opcode, instrfunc)                 \
                                                                    \
    do                                                              \
    {                                                               \
        if (arch & HDL_INSTARCH_900)                                \
            hdl_init_dins( arch, opcode,                            \
                QSTR( instrfunc ), &z900_ ## instrfunc );           \
    }                                                               \
    while (0)

#else
 #define HDL_900_DEF_INST( arch, opcode, instrfunc)
#endif

/*-------------------------------------------------------------------*/
/*                  HDL_INSTRUCTION_SECTION                          */
/*-------------------------------------------------------------------*/

#define HDL_INSTRUCTION_SECTION                                     \
                                                                    \
    DLL_EXPORT void HDL_DINS                                        \
    (                                                               \
        int (*hdl_init_dins)( int, int, void*, void* )              \
    )                                                               \
    {                                                               \
        {UNREFERENCED( hdl_init_dins );}

/*-------------------------------------------------------------------*/
/*                       HDL_DEF_INST                                */
/*-------------------------------------------------------------------*/

#define HDL_DEF_INST( arch, opcode, instrfunc )                              \
                                                                             \
    do                                                                       \
    {                                                                        \
        HDL_370_DEF_INST( ((arch) & HDL_INSTARCH_370), opcode, instrfunc );  \
        HDL_390_DEF_INST( ((arch) & HDL_INSTARCH_390), opcode, instrfunc );  \
        HDL_900_DEF_INST( ((arch) & HDL_INSTARCH_900), opcode, instrfunc );  \
    }                                                                        \
    while (0)

/*-------------------------------------------------------------------*/
/*                       HDL_UNDEF_INST                              */
/*-------------------------------------------------------------------*/

#define HDL_UNDEF_INST( instrfunc )                                 \
                                                                    \
    DEF_INST( instrfunc )                                           \
    {                                                               \
        INST_UPDATE_PSW( regs, ILC( inst[0] ), ILC( inst[0] ));     \
        regs->program_interrupt( regs, PGM_OPERATION_EXCEPTION );   \
    }

/*-------------------------------------------------------------------*/
/*                    END_INSTRUCTION_SECTION                        */
/*-------------------------------------------------------------------*/

#define END_INSTRUCTION_SECTION     }

/*-------------------------------------------------------------------*/
/*                       FINAL_SECTION                               */
/*-------------------------------------------------------------------*/

#define HDL_FINAL_SECTION           DLL_EXPORT int HDL_FINI() { int rc = 0;

/*-------------------------------------------------------------------*/

#define END_FINAL_SECTION           return rc; }

/*-------------------------------------------------------------------*/
#endif /* defined( OPTION_DYNAMIC_LOAD ) */
/*-------------------------------------------------------------------*/

#endif /* _HDL_H */
