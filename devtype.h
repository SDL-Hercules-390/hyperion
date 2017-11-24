/* DEVTYPE.H    (c) Copyright Jan Jaeger, 1999-2012                  */
/*              Hercules Device Definitions                          */

#if !defined(_DEVICES_H)

#define _DEVICES_H

#ifndef _FBADASD_C_
#ifndef _HDASD_DLL_
#define FBA_DLL_IMPORT DLL_IMPORT
#else   /* _HDASD_DLL_ */
#define FBA_DLL_IMPORT extern
#endif  /* _HDASD_DLL_ */
#else
#define FBA_DLL_IMPORT DLL_EXPORT
#endif

#ifndef _CKDDASD_C_
#ifndef _HDASD_DLL_
#define CKD_DLL_IMPORT DLL_IMPORT
#else   /* _HDASD_DLL_ */
#define CKD_DLL_IMPORT extern
#endif  /* _HDASD_DLL_ */
#else
#define CKD_DLL_IMPORT DLL_EXPORT
#endif


struct DEVHND {
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

#define BEGIN_DEVICE_CLASS_QUERY( _classname, _dev, _class, _buflen, _buffer ) \
    if (_class) *_class = _classname; \
    if (!_dev || !_class || !_buflen || !_buffer) return


#if !defined(OPTION_DYNAMIC_LOAD)
extern DEVHND constty_device_hndinfo;
extern DEVHND loc3270_device_hndinfo;
extern DEVHND comadpt_device_hndinfo;
extern DEVHND cardrdr_device_hndinfo;
extern DEVHND cardpch_device_hndinfo;
extern DEVHND printer_device_hndinfo;
extern DEVHND prt3203_device_hndinfo
extern DEVHND tape_other_devhnd;
extern DEVHND tape_3590_devhnd;
extern DEVHND qeth_device_hndinfo;
extern DEVHND zfcp_device_hndinfo;
#endif /*!defined(OPTION_DYNAMIC_LOAD)*/
CKD_DLL_IMPORT DEVHND ckddasd_device_hndinfo;
FBA_DLL_IMPORT DEVHND fbadasd_device_hndinfo;
extern DEVHND ctci_device_hndinfo;
extern DEVHND ctce_device_hndinfo;
extern DEVHND lcs_device_hndinfo;
extern DEVHND ptp_device_hndinfo;


#endif /*!defined(_DEVICES_H)*/
