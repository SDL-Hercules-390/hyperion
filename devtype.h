/* DEVTYPE.H    (C) Copyright Jan Jaeger, 1999-2012                  */
/*              Hercules Device Definitions                          */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#ifndef _DEVICES_H
#define _DEVICES_H

struct DEVHND
{
        DEVIF  *init;                  /* Device Initialization      */
        DEVXF  *exec;                  /* Device CCW execute         */
        DEVCF  *close;                 /* Device Close               */
        DEVQF  *query;                 /* Device Query               */
        DEVQF  *ext_query;             /* Extended Device Query      */
        DEVSF  *start;                 /* Device Start channel pgm   */
        DEVSF  *end;                   /* Device End channel pgm     */
        DEVSF  *resume;                /* Device Resume channel pgm  */
        DEVSF  *suspend;               /* Device Suspend channel pgm */
        DEVHF  *halt;                  /* Device Halt/Clear Subchann.*/
        DEVRF  *read;                  /* Device Read                */
        DEVWF  *write;                 /* Device Write               */
        DEVUF  *used;                  /* Device Query used          */
        DEVRR  *reserve;               /* Device Reserve             */
        DEVRR  *release;               /* Device Release             */
        DEVRR  *attention;             /* Device Attention           */
        DEVIM  immed;                  /* Immediate CCW Codes        */
        DEVSA  *siga_r;                /* Signal Adapter Input       */
        DEVSA  *siga_w;                /* Signal Adapter Output      */
        DEVSAS *siga_s;                /* Signal Adapter Sync        */
        DEVSA  *siga_m;                /* Signal Adapter Output Mult */
        DEVQD  *ssqd;                  /* QDIO subsys query desc     */
        DEVQD  *ssci;                  /* QDIO set subchan ind       */
        DEVSR  *hsuspend;              /* Hercules suspend           */
        DEVSR  *hresume;               /* Hercules resume            */
};

#define BEGIN_DEVICE_CLASS_QUERY( _classname, _dev, _class, _buflen, _buffer )  \
                                                                                \
    if (_class) *_class = _classname;                                           \
    if (!_dev || !_class || !_buflen || !_buffer) return;                       \
    if (sysblk.devnameonly && !IS_INTEGRATED_CONS( _dev ))                      \
    {                                                                           \
        STRLCPY( filename, basename( _dev->filename ));                         \
        if (strcmp( filename, "." ) == 0)                                       \
            filename[0] = 0;                                                    \
    }                                                                           \
    else                                                                        \
        STRLCPY( filename, _dev->filename )




CKD_DLL_IMPORT  DEVHND  ckd_dasd_device_hndinfo;
FBA_DLL_IMPORT  DEVHND  fba_dasd_device_hndinfo;

extern          DEVHND  ctci_device_hndinfo;
extern          DEVHND  ctct_device_hndinfo;
extern          DEVHND  ctce_device_hndinfo;
extern          DEVHND  lcs_device_hndinfo;
extern          DEVHND  ptp_device_hndinfo;


#endif // _DEVICES_H
