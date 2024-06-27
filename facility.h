/* FACILITY.H   (C) Copyright "Fish" (David B. Trout), 2018-2019     */
/*                  Facility bit structures and functions            */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#ifndef _FACILITY_H_
#define _FACILITY_H_

/*-------------------------------------------------------------------*/
/*                   Facility Table typedefs                         */
/*-------------------------------------------------------------------*/

typedef bool FACMODCHK( bool         enable,
                        int          bitno,
                        int          archnum,
                        const char*  action,
                        const char*  actioning,
                        const char*  opp_actioning,
                        const char*  target_facname );

typedef void FACUPDINS( int hdl_arch, bool enable );

/*-------------------------------------------------------------------*/
/*                  Facility Table structure                         */
/*-------------------------------------------------------------------*/

struct FACTAB
{
    FACMODCHK*   modokfunc;         /* Modification Check function   */
    FACUPDINS*   updinstrs;         /* Update Opcode Table function  */
    const char*  name;              /* Short facility name           */
    const char*  long_name;         /* Long name = Description       */
          int    bitno;             /* Bit number                    */
          int    supmask;           /* Which archs support it        */
          int    defmask;           /* Default for which archs       */
          int    reqmask;           /* Which archs require it        */
};
typedef struct FACTAB   FACTAB;

/*-------------------------------------------------------------------*/
/*                    Facility Table macros                          */
/*-------------------------------------------------------------------*/

#define FT( _sup, _def, _req, _name )                               \
{                                                                   \
    NULL,                                                           \
    NULL,                                                           \
    QSTR( _name ),                                                  \
    NULL,                                                           \
    STFL_ ## _name,   /* stfl.h 'STFL_XXX' bit number #define */    \
    _sup,                                                           \
    _def,                                                           \
    _req                                                            \
},

/*-------------------------------------------------------------------*/

#define FT2( _modokfunc, _updinstrs, _name, _desc )                 \
{                                                                   \
    _modokfunc,                                                     \
    _updinstrs,                                                     \
    QSTR( _name ),                                                  \
    _desc,                                                          \
    STFL_ ## _name,   /* stfl.h 'STFL_XXX' bit number #define */    \
    0,                                                              \
    0,                                                              \
    0,                                                              \
},

/*-------------------------------------------------------------------*/
/*                  Facility Table functions                         */
/*-------------------------------------------------------------------*/

extern bool init_facilities_lists();
extern void init_cpu_facilities( REGS* regs );
extern bool facility_query( int argc, char* argv[] );
extern int  facility_enable_disable( int argc, char* argv[] );

#endif // _FACILITY_H_
