/* IMPEXP.H     (C) "Fish" (David B. Trout), 2018                    */
/*              DLL_IMPORT/DLL_EXPORT/extern control                 */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#ifndef _IMPEXP_H_
#define _IMPEXP_H_

// Define all DLL Imports depending on current file

#ifndef _HSYS_C_
#define HSYS_DLL_IMPORT DLL_IMPORT
#else   /* _HSYS_C_ */
#define HSYS_DLL_IMPORT DLL_EXPORT
#endif  /* _HSYS_C_ */

#ifndef _CCKDDASD_C_
#ifndef _HDASD_DLL_
#define CCKD_DLL_IMPORT DLL_IMPORT
#else   /* _HDASD_DLL_ */
#define CCKD_DLL_IMPORT extern
#endif  /* _HDASD_DLL_ */
#else
#define CCKD_DLL_IMPORT DLL_EXPORT
#endif

#ifndef _HDL_C_
#ifndef _HUTIL_DLL_
#define HHDL_DLL_IMPORT DLL_IMPORT
#else   /* _HUTIL_DLL_ */
#define HHDL_DLL_IMPORT extern
#endif  /* _HUTIL_DLL_ */
#else
#define HHDL_DLL_IMPORT DLL_EXPORT
#endif

#ifndef _GETOPT_C_
#ifndef _HUTIL_DLL_
#define GOP_DLL_IMPORT DLL_IMPORT
#else   /* _HUTIL_DLL_ */
#define GOP_DLL_IMPORT extern
#endif  /* _HUTIL_DLL_ */
#else
#define GOP_DLL_IMPORT DLL_EXPORT
#endif

#ifndef _HSCCMD_C_
#ifndef _HENGINE_DLL_
#define HCMD_DLL_IMPORT DLL_IMPORT
#else   /* _HENGINE_DLL_ */
#define HCMD_DLL_IMPORT extern
#endif  /* _HENGINE_DLL_ */
#else
#define HCMD_DLL_IMPORT DLL_EXPORT
#endif

#ifndef _HSCMISC_C_
#ifndef _HENGINE_DLL_
#define HMISC_DLL_IMPORT DLL_IMPORT
#else   /* _HENGINE_DLL_ */
#define HMISC_DLL_IMPORT extern
#endif  /* _HENGINE_DLL_ */
#else
#define HMISC_DLL_IMPORT DLL_EXPORT
#endif

#ifndef _HSCEMODE_C_
#ifndef _HENGINE_DLL_
#define HCEM_DLL_IMPORT DLL_IMPORT
#else   /* _HENGINE_DLL_ */
#define HCEM_DLL_IMPORT extern
#endif  /* _HENGINE_DLL_ */
#else
#define HCEM_DLL_IMPORT DLL_EXPORT
#endif

#ifndef _HSCLOC_C_
#ifndef _HENGINE_DLL_
#define HCML_DLL_IMPORT DLL_IMPORT
#else   /* _HENGINE_DLL_ */
#define HCML_DLL_IMPORT extern
#endif  /* _HENGINE_DLL_ */
#else
#define HCML_DLL_IMPORT DLL_EXPORT
#endif

#ifndef _HSCPUFUN_C_
#ifndef _HENGINE_DLL_
#define HCPU_DLL_IMPORT DLL_IMPORT
#else   /* _HENGINE_DLL_ */
#define HCPU_DLL_IMPORT extern
#endif  /* _HENGINE_DLL_ */
#else
#define HCPU_DLL_IMPORT DLL_EXPORT
#endif

#ifndef _CMDTAB_C_
#ifndef _HENGINE_DLL_
#define CMDT_DLL_IMPORT DLL_IMPORT
#else   /* _HENGINE_DLL_ */
#define CMDT_DLL_IMPORT extern
#endif  /* _HENGINE_DLL_ */
#else
#define CMDT_DLL_IMPORT DLL_EXPORT
#endif

#ifndef _HAO_C_
#ifndef _HENGINE_DLL_
#define HAO_DLL_IMPORT DLL_IMPORT
#else   /* _HENGINE_DLL_ */
#define HAO_DLL_IMPORT extern
#endif  /* _HENGINE_DLL_ */
#else
#define HAO_DLL_IMPORT DLL_EXPORT
#endif

#ifndef _PANEL_C_
#ifndef _HENGINE_DLL_
#define HPAN_DLL_IMPORT DLL_IMPORT
#else   /* _HENGINE_DLL_ */
#define HPAN_DLL_IMPORT extern
#endif  /* _HENGINE_DLL_ */
#else
#define HPAN_DLL_IMPORT DLL_EXPORT
#endif

#ifndef _IMPL_C_
#ifndef _HENGINE_DLL_
#define IMPL_DLL_IMPORT DLL_IMPORT
#else   /* _HENGINE_DLL_ */
#define IMPL_DLL_IMPORT extern
#endif  /* _HENGINE_DLL_ */
#else
#define IMPL_DLL_IMPORT DLL_EXPORT
#endif

#ifndef _CCKDUTIL_C_
#ifndef _HDASD_DLL_
#define CCDU_DLL_IMPORT DLL_IMPORT
#else   /* _HDASD_DLL_ */
#define CCDU_DLL_IMPORT extern
#endif  /* _HDASD_DLL_ */
#else
#define CCDU_DLL_IMPORT DLL_EXPORT
#endif

#ifndef _CONFIG_C_
#ifndef _HENGINE_DLL_
#define CONF_DLL_IMPORT DLL_IMPORT
#else   /* _HENGINE_DLL_ */
#define CONF_DLL_IMPORT extern
#endif  /* _HENGINE_DLL_ */
#else
#define CONF_DLL_IMPORT DLL_EXPORT
#endif

#ifndef _CHANNEL_C_
#ifndef _HENGINE_DLL_
#define CHAN_DLL_IMPORT DLL_IMPORT
#else   /* _HENGINE_DLL_ */
#define CHAN_DLL_IMPORT extern
#endif  /* _HENGINE_DLL_ */
#else
#define CHAN_DLL_IMPORT DLL_EXPORT
#endif

#ifndef _SCRIPT_C_
#ifndef _HENGINE_DLL_
#define SCRI_DLL_IMPORT DLL_IMPORT
#else   /* _HENGINE_DLL_ */
#define SCRI_DLL_IMPORT extern
#endif  /* _HENGINE_DLL_ */
#else
#define SCRI_DLL_IMPORT DLL_EXPORT
#endif

#ifndef _BLDCFG_C_
#ifndef _HENGINE_DLL_
#define BLDC_DLL_IMPORT DLL_IMPORT
#else   /* _HENGINE_DLL_ */
#define BLDC_DLL_IMPORT extern
#endif  /* _HENGINE_DLL_ */
#else
#define BLDC_DLL_IMPORT DLL_EXPORT
#endif

#ifndef _SERVICE_C_
#ifndef _HENGINE_DLL_
#define SERV_DLL_IMPORT DLL_IMPORT
#else   /* _HENGINE_DLL_ */
#define SERV_DLL_IMPORT extern
#endif  /* _HENGINE_DLL_ */
#else
#define SERV_DLL_IMPORT DLL_EXPORT
#endif

#ifndef _LOADPARM_C_
#ifndef _HENGINE_DLL_
#define LOADPARM_DLL_IMPORT DLL_IMPORT
#else   /* _HENGINE_DLL_ */
#define LOADPARM_DLL_IMPORT extern
#endif  /* _HENGINE_DLL_ */
#else
#define LOADPARM_DLL_IMPORT DLL_EXPORT
#endif

#ifndef _OPCODE_C_
#ifndef _HENGINE_DLL_
#define OPCD_DLL_IMPORT DLL_IMPORT
#else   /* _HENGINE_DLL_ */
#define OPCD_DLL_IMPORT extern
#endif  /* _HENGINE_DLL_ */
#else
#define OPCD_DLL_IMPORT DLL_EXPORT
#endif

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

#ifndef _DASDUTIL_C_
#ifndef _HDASD_DLL_
#define DUT_DLL_IMPORT DLL_IMPORT
#else   /* _HDASD_DLL_ */
#define DUT_DLL_IMPORT extern
#endif  /* _HDASD_DLL_ */
#else
#define DUT_DLL_IMPORT DLL_EXPORT
#endif

#ifndef _DASDTAB_C_
#ifndef _HDASD_DLL_
#define DTB_DLL_IMPORT DLL_IMPORT
#else   /* _HDASD_DLL_ */
#define DTB_DLL_IMPORT extern
#endif  /* _HDASD_DLL_ */
#else
#define DTB_DLL_IMPORT DLL_EXPORT
#endif

#ifndef _SHARED_C_
#ifndef _HDASD_DLL_
#define SHR_DLL_IMPORT DLL_IMPORT
#else   /* _HDASD_DLL_ */
#define SHR_DLL_IMPORT extern
#endif  /* _HDASD_DLL_ */
#else
#define SHR_DLL_IMPORT DLL_EXPORT
#endif

#ifndef _FTLIB_C_
#ifndef _HTAPE_DLL_
#define FET_DLL_IMPORT DLL_IMPORT
#else   /* _HTAPE_DLL_ */
#define FET_DLL_IMPORT extern
#endif  /* _HTAPE_DLL_ */
#else
#define FET_DLL_IMPORT DLL_EXPORT
#endif

#ifndef _HETLIB_C_
#ifndef _HTAPE_DLL_
#define HET_DLL_IMPORT DLL_IMPORT
#else   /* _HUTIL_DLL_ */
#define HET_DLL_IMPORT extern
#endif  /* _HUTIL_DLL_ */
#else
#define HET_DLL_IMPORT DLL_EXPORT
#endif

#ifndef _SLLIB_C_
#ifndef _HTAPE_DLL_
#define SLL_DLL_IMPORT DLL_IMPORT
#else   /* _HUTIL_DLL_ */
#define SLL_DLL_IMPORT extern
#endif  /* _HUTIL_DLL_ */
#else
#define SLL_DLL_IMPORT DLL_EXPORT
#endif

#ifndef _W32STAPE_C_
  #ifndef _HTAPE_DLL_
    #define W32ST_DLL_IMPORT  DLL_IMPORT
  #else
    #define W32ST_DLL_IMPORT  extern
  #endif
#else
  #define   W32ST_DLL_IMPORT  DLL_EXPORT
#endif

#ifndef _CACHE_C_
#ifndef _HDASD_DLL_
#define CCH_DLL_IMPORT DLL_IMPORT
#else   /* _HDASD_DLL_ */
#define CCH_DLL_IMPORT extern
#endif  /* _HDASD_DLL_ */
#else
#define CCH_DLL_IMPORT DLL_EXPORT
#endif

#ifndef _CODEPAGE_C_
#ifndef _HUTIL_DLL_
#define COD_DLL_IMPORT DLL_IMPORT
#else
#define COD_DLL_IMPORT extern
#endif
#else 
#define COD_DLL_IMPORT DLL_EXPORT
#endif

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








































































#endif // _IMPEXP_H_
