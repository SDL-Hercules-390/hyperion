/* IMPEXP.H     (C) "Fish" (David B. Trout), 2018-2021               */
/*              DLL_IMPORT/DLL_EXPORT/extern control                 */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

/*-------------------------------------------------------------------*/
/*  This header #defines a function/variable attribute depending on  */
/*  which source file is being compiled and which module it is for,  */
/*  allowing modules to access functions/variables in other modules  */
/*  by declaring them as being imported or exported or just extern.  */
/*                                                                   */
/*  Each source file MUST start with a #include for "hstdinc.h", be  */
/*  following by a #define for both the source file being compiled   */
/*  as well as the module (DLL) it is a part of, and then #include   */
/*  for header "hercules.h".  This is a requirement for any source   */
/*  file that needs to export any of its functions to other modules. */
/*                                                                   */
/*  Refer to the "hscutl.c" source file as a simple example of this. */
/*  If you need to #include other headers do so AFTER "hercules.h".  */
/*                                                                   */
/*  As always, please try to keep the below in alphabetical order.   */
/*-------------------------------------------------------------------*/

#ifndef _IMPEXP_H_
#define _IMPEXP_H_

/*********************************************************************/
/*                          _HDASD_DLL_                              */
/*********************************************************************/

#ifndef    _CACHE_C_
  #ifndef  _HDASD_DLL_
    #define CCH_DLL_IMPORT          DLL_IMPORT
  #else
    #define CCH_DLL_IMPORT          extern
  #endif
#else
  #define   CCH_DLL_IMPORT          DLL_EXPORT
#endif

/*----------------------------------------------------*/

#ifndef    _CCKDDASD_C_
  #ifndef  _HDASD_DLL_
    #define CCKD_DLL_IMPORT         DLL_IMPORT
  #else
    #define CCKD_DLL_IMPORT         extern
  #endif
#else
  #define   CCKD_DLL_IMPORT         DLL_EXPORT
#endif

/*----------------------------------------------------*/

#ifndef    _CCKDDASD64_C_
  #ifndef  _HDASD_DLL_
    #define CCKD64_DLL_IMPORT       DLL_IMPORT
  #else
    #define CCKD64_DLL_IMPORT       extern
  #endif
#else
  #define   CCKD64_DLL_IMPORT       DLL_EXPORT
#endif

/*----------------------------------------------------*/

#ifndef    _CCKDUTIL_C_
  #ifndef  _HDASD_DLL_
    #define CCDU_DLL_IMPORT         DLL_IMPORT
  #else
    #define CCDU_DLL_IMPORT         extern
  #endif
#else
  #define   CCDU_DLL_IMPORT         DLL_EXPORT
#endif

/*----------------------------------------------------*/

#ifndef    _CCKDUTIL64_C_
  #ifndef  _HDASD_DLL_
    #define CCDU64_DLL_IMPORT       DLL_IMPORT
  #else
    #define CCDU64_DLL_IMPORT       extern
  #endif
#else
  #define   CCDU64_DLL_IMPORT       DLL_EXPORT
#endif

/*----------------------------------------------------*/

#ifndef    _CKDDASD_C_
  #ifndef  _HDASD_DLL_
    #define CKD_DLL_IMPORT          DLL_IMPORT
  #else
    #define CKD_DLL_IMPORT          extern
  #endif
#else
  #define   CKD_DLL_IMPORT          DLL_EXPORT
#endif

/*----------------------------------------------------*/

#ifndef    _CKDDASD64_C_
  #ifndef  _HDASD_DLL_
    #define CKD64_DLL_IMPORT        DLL_IMPORT
  #else
    #define CKD64_DLL_IMPORT        extern
  #endif
#else
  #define   CKD64_DLL_IMPORT        DLL_EXPORT
#endif

/*----------------------------------------------------*/

#ifndef    _DASDTAB_C_
  #ifndef  _HDASD_DLL_
    #define DTB_DLL_IMPORT          DLL_IMPORT
  #else
    #define DTB_DLL_IMPORT          extern
  #endif
#else
  #define   DTB_DLL_IMPORT          DLL_EXPORT
#endif

/*----------------------------------------------------*/

#ifndef    _DASDUTIL_C_
  #ifndef  _HDASD_DLL_
    #define DUT_DLL_IMPORT          DLL_IMPORT
  #else
    #define DUT_DLL_IMPORT          extern
  #endif
#else
  #define   DUT_DLL_IMPORT          DLL_EXPORT
#endif

/*----------------------------------------------------*/

#ifndef    _DASDUTIL64_C_
  #ifndef  _HDASD_DLL_
    #define DUT64_DLL_IMPORT        DLL_IMPORT
  #else
    #define DUT64_DLL_IMPORT        extern
  #endif
#else
  #define   DUT64_DLL_IMPORT        DLL_EXPORT
#endif

/*----------------------------------------------------*/

#ifndef    _FBADASD_C_
  #ifndef  _HDASD_DLL_
    #define FBA_DLL_IMPORT          DLL_IMPORT
  #else
    #define FBA_DLL_IMPORT          extern
  #endif
#else
  #define   FBA_DLL_IMPORT          DLL_EXPORT
#endif

/*----------------------------------------------------*/

#ifndef    _FBADASD64_C_
  #ifndef  _HDASD_DLL_
    #define FBA64_DLL_IMPORT        DLL_IMPORT
  #else
    #define FBA64_DLL_IMPORT        extern
  #endif
#else
  #define   FBA64_DLL_IMPORT        DLL_EXPORT
#endif

/*----------------------------------------------------*/

#ifndef    _SHARED_C_
  #ifndef  _HDASD_DLL_
    #define SHR_DLL_IMPORT          DLL_IMPORT
  #else
    #define SHR_DLL_IMPORT          extern
  #endif
#else
  #define   SHR_DLL_IMPORT          DLL_EXPORT
#endif

/*********************************************************************/
/*                        _HENGINE_DLL_                              */
/*********************************************************************/

#ifndef    _BLDCFG_C_
  #ifndef  _HENGINE_DLL_
    #define BLDC_DLL_IMPORT         DLL_IMPORT
  #else
    #define BLDC_DLL_IMPORT         extern
  #endif
#else
  #define   BLDC_DLL_IMPORT         DLL_EXPORT
#endif

/*----------------------------------------------------*/

#ifndef    _CHANNEL_C_
  #ifndef  _HENGINE_DLL_
    #define CHAN_DLL_IMPORT         DLL_IMPORT
  #else
    #define CHAN_DLL_IMPORT         extern
  #endif
#else
  #define   CHAN_DLL_IMPORT         DLL_EXPORT
#endif

/*----------------------------------------------------*/

#ifndef    _CLOCK_C_
  #ifndef  _HENGINE_DLL_
    #define CLOCK_DLL_IMPORT        DLL_IMPORT
  #else
    #define CLOCK_DLL_IMPORT        extern
  #endif
#else
  #define   CLOCK_DLL_IMPORT        DLL_EXPORT
#endif

/*----------------------------------------------------*/

#ifndef    _CMDTAB_C_
  #ifndef  _HENGINE_DLL_
    #define CMDT_DLL_IMPORT         DLL_IMPORT
  #else
    #define CMDT_DLL_IMPORT         extern
  #endif
#else
  #define   CMDT_DLL_IMPORT         DLL_EXPORT
#endif

/*----------------------------------------------------*/

#ifndef    _CONFIG_C_
  #ifndef  _HENGINE_DLL_
    #define CONF_DLL_IMPORT         DLL_IMPORT
  #else
    #define CONF_DLL_IMPORT         extern
  #endif
#else
  #define   CONF_DLL_IMPORT         DLL_EXPORT
#endif

/*----------------------------------------------------*/

#ifndef    _CPU_C_
  #ifndef  _HENGINE_DLL_
    #define CPU_DLL_IMPORT          DLL_IMPORT
  #else
    #define CPU_DLL_IMPORT          extern
  #endif
#else
  #define   CPU_DLL_IMPORT          DLL_EXPORT
#endif

/*----------------------------------------------------*/

#ifndef    _DAT_C
  #ifndef  _HENGINE_DLL_
    #define DAT_DLL_IMPORT          DLL_IMPORT
  #else
    #define DAT_DLL_IMPORT          extern
  #endif
#else
  #define   DAT_DLL_IMPORT          DLL_EXPORT
#endif

/*----------------------------------------------------*/

#ifndef    _HAO_C_
  #ifndef  _HENGINE_DLL_
    #define HAO_DLL_IMPORT          DLL_IMPORT
  #else
    #define HAO_DLL_IMPORT          extern
  #endif
#else
  #define   HAO_DLL_IMPORT          DLL_EXPORT
#endif

/*----------------------------------------------------*/

#ifndef    _HSCCMD_C_
  #ifndef  _HENGINE_DLL_
    #define HCMD_DLL_IMPORT         DLL_IMPORT
  #else
    #define HCMD_DLL_IMPORT         extern
  #endif
#else
  #define   HCMD_DLL_IMPORT         DLL_EXPORT
#endif

/*----------------------------------------------------*/

#ifndef    _HSCEMODE_C_
  #ifndef  _HENGINE_DLL_
    #define HCEM_DLL_IMPORT         DLL_IMPORT
  #else
    #define HCEM_DLL_IMPORT         extern
  #endif
#else
  #define   HCEM_DLL_IMPORT         DLL_EXPORT
#endif

/*----------------------------------------------------*/

#ifndef    _HSCLOC_C_
  #ifndef  _HENGINE_DLL_
    #define HCML_DLL_IMPORT         DLL_IMPORT
  #else
    #define HCML_DLL_IMPORT         extern
  #endif
#else
  #define   HCML_DLL_IMPORT         DLL_EXPORT
#endif

/*----------------------------------------------------*/

#ifndef    _HSCMISC_C_
  #ifndef  _HENGINE_DLL_
    #define HMISC_DLL_IMPORT        DLL_IMPORT
  #else
    #define HMISC_DLL_IMPORT        extern
  #endif
#else
  #define HMISC_DLL_IMPORT          DLL_EXPORT
#endif

/*----------------------------------------------------*/

#ifndef    _HSCPUFUN_C_
  #ifndef  _HENGINE_DLL_
    #define HCPU_DLL_IMPORT         DLL_IMPORT
  #else
    #define HCPU_DLL_IMPORT         extern
  #endif
#else
  #define   HCPU_DLL_IMPORT         DLL_EXPORT
#endif

/*----------------------------------------------------*/

#ifndef    _HTTPSERV_C_
  #ifndef  _HENGINE_DLL_
    #define HTTP_DLL_IMPORT         DLL_IMPORT
  #else
    #define HTTP_DLL_IMPORT         extern
  #endif
#else
  #define   HTTP_DLL_IMPORT         DLL_EXPORT
#endif

/*----------------------------------------------------*/

#ifndef    _IMPL_C_
  #ifndef  _HENGINE_DLL_
    #define IMPL_DLL_IMPORT         DLL_IMPORT
  #else
    #define IMPL_DLL_IMPORT         extern
  #endif
#else
  #define   IMPL_DLL_IMPORT         DLL_EXPORT
#endif

/*----------------------------------------------------*/

#ifndef    _INLINE_C_
  #ifndef  _HENGINE_DLL_
    #define INLINE_DLL_IMPORT       DLL_IMPORT
  #else
    #define INLINE_DLL_IMPORT       extern
  #endif
#else
  #define   INLINE_DLL_IMPORT       DLL_EXPORT
#endif

#ifndef    _INLINE_C_
  #define   INLINE_INL_DLL_IMPORT
#else
  #define   INLINE_INL_DLL_IMPORT   extern
#endif

/*----------------------------------------------------*/

#ifndef    _LOADPARM_C_
  #ifndef  _HENGINE_DLL_
    #define LOADPARM_DLL_IMPORT     DLL_IMPORT
  #else
    #define LOADPARM_DLL_IMPORT     extern
  #endif
#else
  #define   LOADPARM_DLL_IMPORT     DLL_EXPORT
#endif

/*----------------------------------------------------*/

#ifndef    _MPC_C_
  #ifndef  _HENGINE_DLL_
    #define MPC_DLL_IMPORT          DLL_IMPORT
  #else
    #define MPC_DLL_IMPORT          extern
  #endif
#else
  #define   MPC_DLL_IMPORT          DLL_EXPORT
#endif

/*----------------------------------------------------*/

#ifndef    _OPCODE_C_
  #ifndef  _HENGINE_DLL_
    #define OPCD_DLL_IMPORT         DLL_IMPORT
  #else
    #define OPCD_DLL_IMPORT         extern
  #endif
#else
  #define   OPCD_DLL_IMPORT         DLL_EXPORT
#endif

/*----------------------------------------------------*/

#ifndef    _PANEL_C_
  #ifndef  _HENGINE_DLL_
    #define HPAN_DLL_IMPORT         DLL_IMPORT
  #else
    #define HPAN_DLL_IMPORT         extern
  #endif
#else
  #define   HPAN_DLL_IMPORT         DLL_EXPORT
#endif

/*----------------------------------------------------*/

#ifndef    _QDIO_C_
  #ifndef  _HENGINE_DLL_
    #define QDIO_DLL_IMPORT         DLL_IMPORT
  #else
    #define QDIO_DLL_IMPORT         extern
  #endif
#else
  #define   QDIO_DLL_IMPORT         DLL_EXPORT
#endif

/*----------------------------------------------------*/

#ifndef    _SCRIPT_C_
  #ifndef  _HENGINE_DLL_
    #define SCRI_DLL_IMPORT         DLL_IMPORT
  #else
    #define SCRI_DLL_IMPORT         extern
  #endif
#else
  #define   SCRI_DLL_IMPORT         DLL_EXPORT
#endif

/*----------------------------------------------------*/

#ifndef    _SERVICE_C_
  #ifndef  _HENGINE_DLL_
    #define SERV_DLL_IMPORT         DLL_IMPORT
  #else
    #define SERV_DLL_IMPORT         extern
  #endif
#else
  #define   SERV_DLL_IMPORT         DLL_EXPORT
#endif

/*----------------------------------------------------*/

#ifndef    _SKEY_C_
  #ifndef  _HENGINE_DLL_
    #define SKEY_DLL_IMPORT         DLL_IMPORT
  #else
    #define SKEY_DLL_IMPORT         extern
  #endif
#else
  #define   SKEY_DLL_IMPORT         DLL_EXPORT
#endif

#ifndef    _SKEY_C_
  #define   SKEY_INL_DLL_IMPORT
#else
  #define   SKEY_INL_DLL_IMPORT     extern
#endif

/*----------------------------------------------------*/

#ifndef    _TRANSACT_C_
  #ifndef  _HENGINE_DLL_
    #define TRANS_DLL_IMPORT        DLL_IMPORT
  #else
    #define TRANS_DLL_IMPORT        extern
  #endif
#else
  #define   TRANS_DLL_IMPORT        DLL_EXPORT
#endif

/*----------------------------------------------------*/

#ifndef    _VECTOR_C_
  #ifndef  _HENGINE_DLL_
    #define VECT_DLL_IMPORT         DLL_IMPORT
  #else
    #define VECT_DLL_IMPORT         extern
  #endif
#else
  #define   VECT_DLL_IMPORT         DLL_EXPORT
#endif

/*----------------------------------------------------*/

#ifndef    _ZVECTOR_C_
  #ifndef  _HENGINE_DLL_
    #define ZVECT_DLL_IMPORT        DLL_IMPORT
  #else
    #define ZVECT_DLL_IMPORT        extern
  #endif
#else
  #define   ZVECT_DLL_IMPORT        DLL_EXPORT
#endif

/*----------------------------------------------------*/

#ifndef    _VSTORE_C_
  #ifndef  _HENGINE_DLL_
    #define VSTORE_DLL_IMPORT       DLL_IMPORT
  #else
    #define VSTORE_DLL_IMPORT       extern
  #endif
#else
  #define   VSTORE_DLL_IMPORT       DLL_EXPORT
#endif

#ifndef    _VSTORE_C_
  #define   VSTORE_INL_DLL_IMPORT
#else
  #define   VSTORE_INL_DLL_IMPORT   extern
#endif

/*********************************************************************/
/*                          _HTAPE_DLL_                              */
/*********************************************************************/

#ifndef    _FTLIB_C_
  #ifndef  _HTAPE_DLL_
    #define FET_DLL_IMPORT          DLL_IMPORT
  #else
    #define FET_DLL_IMPORT          extern
  #endif
#else
  #define   FET_DLL_IMPORT          DLL_EXPORT
#endif

/*----------------------------------------------------*/

#ifndef    _HETLIB_C_
  #ifndef  _HTAPE_DLL_
    #define HET_DLL_IMPORT          DLL_IMPORT
  #else
    #define HET_DLL_IMPORT          extern
  #endif
#else
  #define   HET_DLL_IMPORT          DLL_EXPORT
#endif

/*----------------------------------------------------*/

#ifndef    _SLLIB_C_
  #ifndef  _HTAPE_DLL_
    #define SLL_DLL_IMPORT          DLL_IMPORT
  #else
    #define SLL_DLL_IMPORT          extern
  #endif
#else
  #define   SLL_DLL_IMPORT          DLL_EXPORT
#endif

/*----------------------------------------------------*/

#ifndef    _W32STAPE_C_
  #ifndef  _HTAPE_DLL_
    #define W32ST_DLL_IMPORT        DLL_IMPORT
  #else
    #define W32ST_DLL_IMPORT        extern
  #endif
#else
  #define   W32ST_DLL_IMPORT        DLL_EXPORT
#endif

/*********************************************************************/
/*                          _HUTIL_DLL_                              */
/*********************************************************************/

#ifndef    _CODEPAGE_C_
  #ifndef  _HUTIL_DLL_
    #define COD_DLL_IMPORT          DLL_IMPORT
  #else
    #define COD_DLL_IMPORT          extern
  #endif
#else
  #define   COD_DLL_IMPORT          DLL_EXPORT
#endif

/*----------------------------------------------------*/

#ifndef    _FTHREADS_C_
  #ifndef  _HUTIL_DLL_
    #define FT_DLL_IMPORT           DLL_IMPORT
  #else
    #define FT_DLL_IMPORT           extern
  #endif
#else
  #define   FT_DLL_IMPORT           DLL_EXPORT
#endif

/*----------------------------------------------------*/

#ifndef    _GETOPT_C_
  #ifndef  _HUTIL_DLL_
    #define GOP_DLL_IMPORT          DLL_IMPORT
  #else
    #define GOP_DLL_IMPORT          extern
  #endif
#else
  #define   GOP_DLL_IMPORT          DLL_EXPORT
#endif

/*----------------------------------------------------*/

#ifndef    _HDL_C_
  #ifndef  _HUTIL_DLL_
    #define HDL_DLL_IMPORT          DLL_IMPORT
  #else
    #define HDL_DLL_IMPORT          extern
  #endif
#else
  #define   HDL_DLL_IMPORT          DLL_EXPORT
#endif

/*----------------------------------------------------*/

#ifndef    _HEXDUMPE_C_
  #ifndef  _HUTIL_DLL_
    #define HEXDMP_DLL_IMPORT       DLL_IMPORT
  #else
    #define HEXDMP_DLL_IMPORT       extern
  #endif
#else
  #define   HEXDMP_DLL_IMPORT       DLL_EXPORT
#endif

/*----------------------------------------------------*/

#ifndef    _HOSTINFO_C_
  #ifndef  _HUTIL_DLL_
    #define HI_DLL_IMPORT           DLL_IMPORT
  #else
    #define HI_DLL_IMPORT           extern
  #endif
#else
  #define   HI_DLL_IMPORT           DLL_EXPORT
#endif

/*----------------------------------------------------*/

#ifndef    _HSCUTL_C_
  #ifndef  _HUTIL_DLL_
    #define HUT_DLL_IMPORT          DLL_IMPORT
  #else
    #define HUT_DLL_IMPORT          extern
  #endif
#else
  #define   HUT_DLL_IMPORT          DLL_EXPORT
#endif

/*----------------------------------------------------*/

#ifndef    _HSCUTL2_C_
  #ifndef  _HUTIL_DLL_
    #define HU2_DLL_IMPORT          DLL_IMPORT
  #else
    #define HU2_DLL_IMPORT          extern
  #endif
#else
  #define   HU2_DLL_IMPORT          DLL_EXPORT
#endif

/*----------------------------------------------------*/

#ifndef    _HSOCKET_C_
  #ifndef  _HUTIL_DLL_
    #define HSOCK_DLL_IMPORT        DLL_IMPORT
  #else
    #define HSOCK_DLL_IMPORT        extern
  #endif
#else
  #define   HSOCK_DLL_IMPORT        DLL_EXPORT
#endif

/*----------------------------------------------------*/

#ifndef    _HTHREAD_C_
  #ifndef  _HUTIL_DLL_
    #define HT_DLL_IMPORT           DLL_IMPORT
  #else
    #define HT_DLL_IMPORT           extern
  #endif
#else
  #define   HT_DLL_IMPORT           DLL_EXPORT
#endif

/*----------------------------------------------------*/

#ifndef    _LOGGER_C_
  #ifndef  _HUTIL_DLL_
    #define LOGR_DLL_IMPORT         DLL_IMPORT
  #else
    #define LOGR_DLL_IMPORT         extern
  #endif
#else
  #define   LOGR_DLL_IMPORT         DLL_EXPORT
#endif

/*----------------------------------------------------*/

#ifndef    _LOGMSG_C_
  #ifndef  _HUTIL_DLL_
    #define LOGM_DLL_IMPORT         DLL_IMPORT
  #else
    #define LOGM_DLL_IMPORT         extern
  #endif
#else
  #define   LOGM_DLL_IMPORT         DLL_EXPORT
#endif

/*----------------------------------------------------*/

#ifndef    _MEMRCHR_C_
  #ifndef  _HUTIL_DLL_
    #define MEM_DLL_IMPORT          DLL_IMPORT
  #else
    #define MEM_DLL_IMPORT          extern
  #endif
#else
  #define   MEM_DLL_IMPORT          DLL_EXPORT
#endif

/*----------------------------------------------------*/

#ifndef    _PARSER_C_
  #ifndef  _HUTIL_DLL_
    #define PAR_DLL_IMPORT          DLL_IMPORT
  #else
    #define PAR_DLL_IMPORT          extern
  #endif
#else
  #define   PAR_DLL_IMPORT          DLL_EXPORT
#endif

/*----------------------------------------------------*/

#ifndef    _PTTRACE_C_
  #ifndef  _HUTIL_DLL_
    #define PTT_DLL_IMPORT          DLL_IMPORT
  #else
    #define PTT_DLL_IMPORT          extern
  #endif
#else
  #define   PTT_DLL_IMPORT          DLL_EXPORT
#endif

/*----------------------------------------------------*/

#ifndef    _VERSION_C_
  #ifndef  _HUTIL_DLL_
    #define VER_DLL_IMPORT          DLL_IMPORT
  #else
    #define VER_DLL_IMPORT          extern
  #endif
#else
  #define   VER_DLL_IMPORT          DLL_EXPORT
#endif

/*----------------------------------------------------*/

#ifndef    _W32UTIL_C_
  #ifndef  _HUTIL_DLL_
    #define W32_DLL_IMPORT          DLL_IMPORT
  #else
    #define W32_DLL_IMPORT          extern
  #endif
#else
  #define   W32_DLL_IMPORT          DLL_EXPORT
#endif

/*********************************************************************/
/*                          _HSYS_DLL_                               */
/*********************************************************************/

#ifndef    _HSYS_C_
  #ifndef  _HSYS_DLL_
    #define HSYS_DLL_IMPORT         DLL_IMPORT
  #else
    #define HSYS_DLL_IMPORT         extern
  #endif
#else
  #define   HSYS_DLL_IMPORT         DLL_EXPORT
#endif

/*********************************************************************/
/*                        _HDT3420_DLL_                              */
/*********************************************************************/

#ifndef    _AWSTAPE_C_
  #ifndef  _HDT3420_DLL_
    #define AWSTAPE_DLL_IMPORT      DLL_IMPORT
  #else
    #define AWSTAPE_DLL_IMPORT      extern
  #endif
#else
  #define   AWSTAPE_DLL_IMPORT      DLL_EXPORT
#endif

/*----------------------------------------------------*/

#ifndef    _HETTAPE_C_
  #ifndef  _HDT3420_DLL_
    #define HETTAPE_DLL_IMPORT      DLL_IMPORT
  #else
    #define HETTAPE_DLL_IMPORT      extern
  #endif
#else
  #define   HETTAPE_DLL_IMPORT      DLL_EXPORT
#endif

/*----------------------------------------------------*/

#endif // _IMPEXP_H_
