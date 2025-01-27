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
/*                                                                   */
/* This table provides aliases for device-types such that various    */
/* device-types are mapped to a common device-handler module.        */
/*                                                                   */
/* The only purpose of this table is to associate the correct hdt    */
/* loadable module with a specific device-type BEFORE the device-    */
/* type in question has been registered.  This table will NOT be     */
/* searched for registered device-types or if the specific loadable  */
/* module already exists.                                            */
/*                                                                   */
/*-------------------------------------------------------------------*/

static EQUTAB equtab[] =
{
//     alias = device-type (2nd arg of config file device statement)
//     |
//     |         name = hdt loadable module ("99XY" ==> "hdt99xy.dll")
//     |         |
//     V         V

    { "1052",    "3270"  },
    { "3215",    "3270"  },
    { "3287",    "3270"  },
    { "SYSG",    "3270"  },

    { "1052-C",  "1052c" },
    { "3215-C",  "1052c" },

    { "1442",    "3505"  },
    { "2501",    "3505"  },

    { "3203",    "1403"  },
    { "3211",    "1403"  },

    { "3410",    "3420"  },
    { "3411",    "3420"  },
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

    { "HCHAN",   "2880"  },     // 2880 = Block Multiplexer
    { "2870",    "2880"  },     // 2870 = Byte Multiplexor
    { "2860",    "2880"  },     // 2860 = Selector
    { "9032",    "2880"  },     // 9032 = ESCON

    { "AUSC",   "TCPH"   },
    { "UDPH",   "TCPH"   },
    { "TLNT",   "TCPH"   },
};

/*-------------------------------------------------------------------*/
/*          List all of our DEVEQU device-type equates               */
/*-------------------------------------------------------------------*/
static void list_devequs()
{
    char buf[ 80 ] = {0};
    char wrk[ 32 ] = {0};

    size_t  i, n;

    LOGMSG( "HHC01539I HDL:                 typ      mod      typ      mod      typ      mod\n" );
    LOGMSG( "HHC01539I HDL:                ------   -----    ------   -----    ------   -----\n" );

    for (i=0, n=1; i < _countof( equtab ); i++, n++)
    {
        if (n >= 4)
        {
            // "HDL: devtyp/hdlmod: %s"
            WRMSG( HHC01539, "I", RTRIM( buf ));
            n = 1;
            buf[0] = 0;
        }

        MSGBUF( wrk, "%-6s = %-5s    ", equtab[i].alias,
                                        equtab[i].name );
        STRLCAT( buf, wrk );
    }

    // "HDL: devtyp/hdlmod: %s"
    WRMSG( HHC01539, "I", RTRIM( buf ));
}

/*-------------------------------------------------------------------*/
/*             Our DEVEQU device-type equates function               */
/*-------------------------------------------------------------------*/
static const char* devequ_func( const char* typname )
{
    DEVEQU* next_devequ_func;
    size_t  i;

    /* Request to list equates? */
    if (!typname)
        list_devequs();
    else /* Request to translate device-type */
    {
        /* Search device equates table for match */
        for (i=0; i < _countof( equtab ); i++)
        {
            /* Is this the device-type they're requesting? */
            if (strcasecmp( equtab[i].alias, typname ) == 0)
            {
                /* Yes, then use this device-type name instead */
                return equtab[i].name;
            }
        }
    }

    /* Call next device-type-equates function, if any */
    if (!(next_devequ_func = (DEVEQU*) hdl_next( &devequ_func )))
        return NULL;
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
/*       Register our device-type equates module with HDL            */
/*-------------------------------------------------------------------*/

HDL_DEPENDENCY_SECTION;
{
     HDL_DEPENDENCY( HERCULES );
}
END_DEPENDENCY_SECTION

/*-------------------------------------------------------------------*/

HDL_REGISTER_SECTION;
{
    HDL_REGISTER( hdl_devequ, devequ_func );
}
END_REGISTER_SECTION

/*-------------------------------------------------------------------*/
