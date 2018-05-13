/* HDL.H        (C) Copyright Jan Jaeger, 2003-2012                  */
/*              Hercules Dynamic Loader                              */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#ifndef _HDL_H
#define _HDL_H

#include "hercules.h"

/*-------------------------------------------------------------------*/
/*                           Flags                                   */
/*-------------------------------------------------------------------*/

#define HDL_LOAD_DEFAULT     0x00000000
#define HDL_LOAD_MAIN        0x80000000 /* Hercules MAIN module flag */
#define HDL_LOAD_NOUNLOAD    0x40000000 /* Module cannot be unloaded */
#define HDL_LOAD_FORCE       0x20000000 /* Override dependency check */
#define HDL_LOAD_NOMSG       0x10000000 /* Do not issue not found msg*/
#define HDL_LOAD_WAS_FORCED  0x08000000 /* Module load was forced    */

#define HDL_INSTARCH_370     0x80000000
#define HDL_INSTARCH_390     0x40000000
#define HDL_INSTARCH_900     0x20000000
#define HDL_INSTARCH_ALL     (HDL_INSTARCH_370 | HDL_INSTARCH_390 | HDL_INSTARCH_900)

#define HDL_LIST_DEFAULT     0x00000000
#define HDL_LIST_ALL         0x80000000 /* list all references       */

/*-------------------------------------------------------------------*/
/*                   HDL callback functions                          */
/*-------------------------------------------------------------------*/
/*   These are the HDL functions the loadable modules call into.     */
/*-------------------------------------------------------------------*/

typedef void  SHUTDN( void* );
typedef int   DEPCHK( const char* depname, const char* depvers, int depsize );
typedef void  REGSYM( const char* symname, void* symaddr );
typedef void* GETSYM( const char* symname );
typedef void  DEFDEV( const char* typname, DEVHND* devhnd );
typedef void  DEFINS( U32 hdl_arch, int opcode, const char* name, void* func );
typedef int   MODFIN();

/*-------------------------------------------------------------------*/
/*                 Module entry-point functions                      */
/*-------------------------------------------------------------------*/
/*      These are the module functions that HDL calls into.          */
/*-------------------------------------------------------------------*/

typedef int         DEPSEC( DEPCHK* depchk );
typedef void        REGSEC( REGSYM* regsym );
typedef void        RESSEC( GETSYM* getsym );
typedef void        DEVSEC( DEFDEV* defdev );
typedef const char* DEVEQU( const char* typname );
typedef void        INSSEC( DEFINS* defins );
typedef int         FINSEC();

/*-------------------------------------------------------------------*/
/*                HDLSHUT  --  Shutdown handling                     */
/*-------------------------------------------------------------------*/

struct HDLSHUT; typedef struct HDLSHUT HDLSHUT;
struct HDLSHUT                          /* Thread shutdown control   */
{
    HDLSHUT*     next;                  /* Next entry                */
    const char*  shutname;              /* Function name             */
    SHUTDN*      shutfunc;              /* Function pointer          */
    void*        shutarg;               /* Function argument         */
};

/*-------------------------------------------------------------------*/
/*                  HDLPRE  --  Module Preloading                    */
/*-------------------------------------------------------------------*/

struct HDLPRE; typedef struct HDLPRE HDLPRE;
struct HDLPRE                           /* Preload list entry        */
{
    const char*  name;                  /* Module name               */
    int          flag;                  /* Load flags                */
};

/*-------------------------------------------------------------------*/
/*                  HDLDEP  --  Dependency entry                     */
/*-------------------------------------------------------------------*/

struct HDLDEP; typedef struct HDLDEP HDLDEP;
struct HDLDEP                           /* Dependency entry          */
{
    const char*  name;                  /* Structure name            */
    const char*  version;               /* Structure version         */
    int          size;                  /* Structure size            */
    HDLDEP*      next;                  /* Next entry                */
};

/*-------------------------------------------------------------------*/
/*                  HDLSYM  --  External Symbol entry                */
/*-------------------------------------------------------------------*/

struct HDLSYM; typedef struct HDLSYM HDLSYM;
struct HDLSYM                           /* External Symbol entry     */
{
    const char*  name;                  /* Symbol name               */
    const char*  owner;                 /* Owning module             */
    void*        symbol;                /* Symbol address            */
    int          refcnt;                /* Symbol reference count    */
    HDLSYM*      next;                  /* Next entry in chain       */
};

/*-------------------------------------------------------------------*/
/*                  HDLDEV  --  Device entry                         */
/*-------------------------------------------------------------------*/

struct HDLDEV; typedef struct HDLDEV HDLDEV;
struct HDLDEV                           /* Device entry              */
{
    const char*  name;                  /* Device type name          */
    DEVHND*      hnd;                   /* Device handlers           */
    HDLDEV*      next;                  /* Next entry                */
};

/*-------------------------------------------------------------------*/
/*                  EQUTAB  --  Device-Equate table                  */
/*-------------------------------------------------------------------*/

struct EQUTAB; typedef struct EQUTAB EQUTAB;
struct EQUTAB                           /* Device Equate table entry */
{
    const char*  alias;                 /* typname from device stmt  */
    const char*  name;                  /* hdt module typname        */
};

/*-------------------------------------------------------------------*/
/*                  HDLINS  --  Instruction entry                    */
/*-------------------------------------------------------------------*/

struct HDLINS; typedef struct HDLINS HDLINS;
struct HDLINS                           /* Instruction entry         */
{
    const char*  instname;              /* Instruction name          */
    void*        instfunc;              /* Instruction function      */
    void*        previous;              /* Previous function         */
    int          opcode;                /* Opcode                    */
    U32          hdl_arch;              /* HDL architecture          */
    HDLINS*      next;                  /* Next entry                */
};

/*-------------------------------------------------------------------*/
/*                  HDLMOD  --  module entry                         */
/*-------------------------------------------------------------------*/

struct HDLMOD; typedef struct HDLMOD HDLMOD;
struct HDLMOD                           /* module entry              */
{
    const char*  name;                  /* module name               */
    void*        handle;                /* module handle (dlopen)    */
    int          flags;                 /* load flags                */

    DEPSEC*      depsec_ep;             /* hdl_check_depends_ep      */
    REGSEC*      regsec_ep;             /* hdl_register_symbols_ep   */
    RESSEC*      ressec_ep;             /* hdl_resolve_symbols_ep    */
    DEVSEC*      devsec_ep;             /* hdl_define_devtypes_ep    */
    INSSEC*      inssec_ep;             /* hdl_define_instructs_ep   */
    FINSEC*      finsec_ep;             /* hdl_on_module_unload_ep   */

    HDLSYM*      symbols;               /* First symbol entry        */
    HDLDEV*      devices;               /* First device entry        */
    HDLINS*      instructs;             /* First instruction entry   */
    HDLMOD*      next;                  /* Next entry in chain       */
};

/*-------------------------------------------------------------------*/
/*                  HDL Function declarations                        */
/*-------------------------------------------------------------------*/
HDL_DLL_IMPORT int hdl_main             /* Main HDL init function    */
(
    PANDISP*    real_pandisp,           /*  ->  real panel_display   */
    PANCMD*     real_pancmd,            /*  ->  real panel_command   */
    REPOPCODE*  real_repopcode,         /*  ->  real replace_opcode  */
    DEVHND*     real_ckd_DEVHND,        /*  ->  CKD devices DEVHND   */
    DEVHND*     real_fba_DEVHND         /*  ->  FBA devices DEVHND   */

);
/*                          hdl_atexit:    Call all shutdown routines*/
/*                          hdl_addshut:   Add shutdown routine      */
/*                          hdl_delshut:   Remove shutdown routine   */
/*                          hdl_initpath:  Initialize module path    */
/*                          hdl_setpath:   Set module path (-1/+1/0) */
/*                          hdl_getpath:   Return module path        */
/*                          hdl_loadmod:   Load HDL module           */
/*                          hdl_freemod:   Unload HDL module         */
/*                          hdl_listmods:  List all HDL modules      */
/*                          hdl_listdeps:  List all HDL dependencies */
/*                          hdl_listequs:  List device equates       */
/*                          hdl_DEVHND:    Get device-type handler   */
/*                          hdl_devequ:    Device-type equates func  */
/*                          hdl_getsym:    Retrieve symbol address   */
/*                          hdl_next:      Find next entry in chain  */
/*                          hdl_repins:    Update opcode table entry */

HDL_DLL_IMPORT void         hdl_atexit   ( void );
HDL_DLL_IMPORT void         hdl_addshut  ( const char* shutname, SHUTDN* shutfunc, void* shutarg );
HDL_DLL_IMPORT int          hdl_delshut  ( SHUTDN* shutfunc, void* shutarg );
HDL_DLL_IMPORT void         hdl_initpath ( const char* path );
HDL_DLL_IMPORT int          hdl_setpath  ( const char* path );
HDL_DLL_IMPORT const char*  hdl_getpath  ();
HDL_DLL_IMPORT int          hdl_loadmod  ( const char* name, int flags );
HDL_DLL_IMPORT int          hdl_freemod  ( const char* name );
HDL_DLL_IMPORT void         hdl_listmods ( int flags );
HDL_DLL_IMPORT void         hdl_listdeps ();
HDL_DLL_IMPORT int          hdl_listequs ();
HDL_DLL_IMPORT DEVHND*      hdl_DEVHND   ( const char* typname );
HDL_DLL_IMPORT DEVEQU*      hdl_devequ;
HDL_DLL_IMPORT void*        hdl_getsym   ( const char* symname );
HDL_DLL_IMPORT void*        hdl_next     ( const void* symbol );
HDL_DLL_IMPORT void         hdl_repins   ( bool replace, HDLINS* ins );

/*-------------------------------------------------------------------*/
/*                     HDL_USE_LIBTOOL                               */
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
/*                     HDL_MODULE_SUFFIX                             */
/*-------------------------------------------------------------------*/

/*      NOTE:  SHLIBEXT defined by ISW in configure.ac/config.h      */

#if defined( LTDL_SHLIB_EXT )
  #define   HDL_MODULE_SUFFIX       LTDL_SHLIB_EXT
#else
  #if defined( LT_MODULE_EXT )
    #define HDL_MODULE_SUFFIX       LT_MODULE_EXT
  #elif defined( _MSVC_ )
    #define HDL_MODULE_SUFFIX       ".dll"
  #else
    #define HDL_MODULE_SUFFIX       ".la"
  #endif
#endif

#if defined( HDL_MODULE_SUFFIX )
 #define HDL_SUFFIX_LENGTH          (sizeof( HDL_MODULE_SUFFIX ) - 1)
#else
 #define HDL_SUFFIX_LENGTH          0
#endif

/*-------------------------------------------------------------------*/
/*                   HDL_DEPENDENCY_SECTION                          */
/*-------------------------------------------------------------------*/

#define HDL_DEPENDENCY_SECTION                                      \
                                                                    \
    DLL_EXPORT int hdl_check_depends_ep( DEPCHK* depchk )           \
    {                                                               \
        int depchk_rc = 0;

/*-------------------------------------------------------------------*/
/*                   END_DEPENDENCY_SECTION                          */
/*-------------------------------------------------------------------*/

#define END_DEPENDENCY_SECTION                                      \
                                                                    \
        return depchk_rc;                                           \
    }

/*-------------------------------------------------------------------*/
/*                      HDL_DEPENDENCY                               */
/*-------------------------------------------------------------------*/

#define HDL_DEPENDENCY( dep )                                       \
                                                                    \
    do                                                              \
    {                                                               \
        if (depchk( QSTR( dep ), HDL_VERS_ ## dep, HDL_SIZE_ ## dep ))  \
            depchk_rc = 1;                                          \
    }                                                               \
    while (0)

/*-------------------------------------------------------------------*/
/*                    HDL_REGISTER_SECTION                           */
/*-------------------------------------------------------------------*/

#define HDL_REGISTER_SECTION                                        \
                                                                    \
    DLL_EXPORT void hdl_register_symbols_ep( REGSYM* regsym )       \
    {

/*-------------------------------------------------------------------*/
/*                   END_REGISTER_SECTION                            */
/*-------------------------------------------------------------------*/

#define END_REGISTER_SECTION                                        \
                                                                    \
    }

/*-------------------------------------------------------------------*/
/*                     HDL_REGISTER                                  */
/*-------------------------------------------------------------------*/

#define HDL_REGISTER( epname, varname )                             \
                                                                    \
    regsym( QSTR( epname ), (void*) &(varname) );

/*-------------------------------------------------------------------*/
/*                   HDL_RESOLVER_SECTION                            */
/*-------------------------------------------------------------------*/

#define HDL_RESOLVER_SECTION                                        \
                                                                    \
    DLL_EXPORT void hdl_resolve_symbols_ep( GETSYM* getsym )        \
    {

/*-------------------------------------------------------------------*/
/*                    END_RESOLVER_SECTION                           */
/*-------------------------------------------------------------------*/

#define END_RESOLVER_SECTION                                        \
                                                                    \
    }

/*-------------------------------------------------------------------*/
/*                       HDL_RESOLVE                                 */
/*-------------------------------------------------------------------*/

#define HDL_RESOLVE( sym )                                          \
                                                                    \
    sym = getsym( QSTR( sym ));

/*-------------------------------------------------------------------*/
/*                     HDL_RESOLVE_SYMPTR                            */
/*-------------------------------------------------------------------*/

#define HDL_RESOLVE_SYMPTR( psym, sym )                             \
                                                                    \
    psym = getsym( QSTR( sym ));

/*-------------------------------------------------------------------*/
/*                    HDL_DEVICE_SECTION                             */
/*-------------------------------------------------------------------*/

#define HDL_DEVICE_SECTION                                          \
                                                                    \
    DLL_EXPORT void hdl_define_devtypes_ep( DEFDEV* defdev )        \
    {

/*-------------------------------------------------------------------*/
/*                    END_DEVICE_SECTION                             */
/*-------------------------------------------------------------------*/

#define END_DEVICE_SECTION                                          \
                                                                    \
    }

/*-------------------------------------------------------------------*/
/*                       HDL_DEVICE                                  */
/*-------------------------------------------------------------------*/

#define HDL_DEVICE( typname, devhnd )                               \
                                                                    \
    defdev( QSTR( typname ), &(devhnd) );

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
/*                  HDL_INSTRUCTION_SECTION                          */
/*-------------------------------------------------------------------*/

#define HDL_INSTRUCTION_SECTION                                     \
                                                                    \
    DLL_EXPORT void hdl_define_instructs_ep( DEFINS* defins )       \
    {

/*-------------------------------------------------------------------*/
/*                    END_INSTRUCTION_SECTION                        */
/*-------------------------------------------------------------------*/

#define END_INSTRUCTION_SECTION                                     \
                                                                    \
    }

/*-------------------------------------------------------------------*/
/*                      HDL_370_DEF_INST                             */
/*-------------------------------------------------------------------*/

#if defined(_370)
 #define HDL_370_DEF_INST( hdl_arch, opcode, instrfunc )            \
                                                                    \
    do                                                              \
        if (hdl_arch == HDL_INSTARCH_370) defins( hdl_arch, opcode, \
            QSTR( instrfunc ),     &s370_ ## instrfunc );           \
    while (0)

#else
 #define HDL_370_DEF_INST( hdl_arch, opcode, instrfunc)
#endif

/*-------------------------------------------------------------------*/
/*                      HDL_390_DEF_INST                             */
/*-------------------------------------------------------------------*/

#if defined(_390)
 #define HDL_390_DEF_INST( hdl_arch, opcode, instrfunc )            \
                                                                    \
    do                                                              \
        if (hdl_arch == HDL_INSTARCH_390) defins( hdl_arch, opcode, \
            QSTR( instrfunc ),     &s390_ ## instrfunc );           \
    while (0)

#else
 #define HDL_390_DEF_INST( hdl_arch, opcode, instrfunc)
#endif

/*-------------------------------------------------------------------*/
/*                      HDL_900_DEF_INST                             */
/*-------------------------------------------------------------------*/

#if defined(_900)
 #define HDL_900_DEF_INST( hdl_arch, opcode, instrfunc)             \
                                                                    \
    do                                                              \
        if (hdl_arch == HDL_INSTARCH_900) defins( hdl_arch, opcode, \
            QSTR( instrfunc ),     &z900_ ## instrfunc );           \
    while (0)

#else
 #define HDL_900_DEF_INST( hdl_arch, opcode, instrfunc)
#endif

/*-------------------------------------------------------------------*/
/*                       HDL_DEF_INST                                */
/*-------------------------------------------------------------------*/

#define HDL_DEF_INST( hdl_arch, opcode, instrfunc )                             \
                                                                                \
    do                                                                          \
    {                                                                           \
        HDL_370_DEF_INST( ((hdl_arch) & HDL_INSTARCH_370), opcode, instrfunc ); \
        HDL_390_DEF_INST( ((hdl_arch) & HDL_INSTARCH_390), opcode, instrfunc ); \
        HDL_900_DEF_INST( ((hdl_arch) & HDL_INSTARCH_900), opcode, instrfunc ); \
    }                                                                           \
    while (0)

/*-------------------------------------------------------------------*/
/*                       FINAL_SECTION                               */
/*-------------------------------------------------------------------*/

#define HDL_FINAL_SECTION                                           \
                                                                    \
    DLL_EXPORT int hdl_on_module_unload_ep()                        \
    {                                                               \
        int rc = 0;

/*-------------------------------------------------------------------*/
/*                    END_FINAL_SECTION                              */
/*-------------------------------------------------------------------*/

#define END_FINAL_SECTION                                           \
                                                                    \
        return rc;                                                  \
    }

#endif /* _HDL_H */
