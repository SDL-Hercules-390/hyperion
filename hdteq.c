/* HDTEQ.C      (C) Copyright Jan Jaeger, 2003-2012                  */
/*              Hercules Dynamic Loader                              */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#include "hstdinc.h"
#include "hercules.h"

/*-------------------------------------------------------------------*/
/*                 device-type translation table                     */
/*-------------------------------------------------------------------*/

struct EQUTAB
{
    char*  alias;
    char*  name;
};
typedef struct EQUTAB EQUTAB;

/*-------------------------------------------------------------------*/
/*                 device-type translation table                     */
/*-------------------------------------------------------------------*/

static EQUTAB devtyp_equtab[] =
{
/*
    This table provides aliases for device-types, such that
    various device-types are mapped to a common loadable module.

    The only purpose of this table is to associate the correct
    loadable module with a specific device-type BEFORE the device-
    type in question has been registered.  This table will NOT be
    searched for registered device-types or if the specific loadable
    module exists.

       device-type requested  (second argument of device statement)
       |
       |         base device support  (e.g base=99XY ==> 'hdt99xy.dll')
       |         |
       V         V
*/
//  { "3390",    "3990"  },
//  { "3380",    "3990"  },
               
    { "1052",    "3270"  },
    { "3215",    "3270"  },
    { "3287",    "3270"  },
    { "SYSG",    "3270"  },
               
    { "1052-C",  "1052c" },
    { "3215-C",  "1052c" },
               
    { "1442",    "3505"  },
    { "2501",    "3505"  },
               
    { "3211",    "1403"  },
               
    { "3410",    "3420"  },
    { "3411",    "3420"  },
//  { "3420",    "3420"  },
    { "3480",    "3420"  },
    { "3490",    "3420"  },
    { "3590",    "3420"  },
    { "9347",    "3420"  },
    { "9348",    "3420"  },
    { "8809",    "3420"  },
    { "3422",    "3420"  },
    { "3430",    "3420"  },
               
    { "V3480",   "3480V" },
    { "V3490",   "3480V" },
    { "V3590",   "3480V" },

    { "DW3480",  "3590D" },
    { "DW3490",  "3590D" },
    { "DW3590",  "3590D" },

    { "TH3480",  "3590D" },
    { "TH3490",  "3590D" },
    { "TH3590",  "3590D" },

    { "8232C",   "8232"  },
               
    { "OSA",     "QETH"  },
    { "OSD",     "QETH"  },
               
    { "LCS",     "3088"  },
    { "CTCI",    "3088"  },
    { "CTCT",    "3088"  },
    { "CTCE",    "3088"  },
    { "VMNET",   "3088"  },
               
    { "HCHAN",   "2880"  },
//  { "2880",    "2880"  },
    { "2870",    "2880"  },
    { "2860",    "2880"  },
    { "9032",    "2880"  },
};

/*-------------------------------------------------------------------*/

static char* hdt_translate_device_type( char* typname )
{
    EQUTYP* next_devequ_func;
    size_t  i;

    /* Search device equates table for match */
    for (i=0; i < _countof( devtyp_equtab ); i++)
    {
        /* Is this the device-type they're requesting? */
        if (strcasecmp( devtyp_equtab[i].alias, typname ) == 0)
        {
            /* Yes, then use this device-type name instead */
            return devtyp_equtab[i].name;
        }
    }

    /* Is there another device-type-equates function in the chain? */
    if (!(next_devequ_func = (EQUTYP*) hdl_next( &hdt_translate_device_type )))
        return NULL;

    /* Yes, then maybe it can translate it */
    return next_devequ_func( typname );
}

/*-------------------------------------------------------------------*/
/*             Libtool static name colision resolution               */
/*-------------------------------------------------------------------*/
/* NOTE: lt_dlopen will look for symbol & modulename_LTX_symbol for  */
/*       use in DLREOPEN case only.                                  */
/*-------------------------------------------------------------------*/

#if defined( HDL_USE_LIBTOOL )

#define hdl_ddev    hdteq_LTX_hdl_ddev
#define hdl_depc    hdteq_LTX_hdl_depc
#define hdl_reso    hdteq_LTX_hdl_reso
#define hdl_init    hdteq_LTX_hdl_init
#define hdl_fini    hdteq_LTX_hdl_fini

#endif

/*-------------------------------------------------------------------*/

HDL_DEPENDENCY_SECTION;
{
     HDL_DEPENDENCY( HERCULES );
}
END_DEPENDENCY_SECTION


HDL_REGISTER_SECTION;
{
    HDL_REGISTER( hdl_devequ, hdt_translate_device_type );
}
END_REGISTER_SECTION

/*-------------------------------------------------------------------*/
