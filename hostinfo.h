/* HOSTINFO.H   (C) Copyright "Fish" (David B. Trout), 2002-2012     */
/*              (C) Copyright TurboHercules, SAS 2010-2011           */
/*            Header file contains host system information           */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#ifndef _HOSTINFO_H_
#define _HOSTINFO_H_

#include "hercules.h"

/*-------------------------------------------------------------------*/
/* Host System Information block                                     */
/*-------------------------------------------------------------------*/
typedef struct HOST_INFO
{
#define HDL_NAME_HOST_INFO  "HOST_INFO"
#define HDL_VERS_HOST_INFO  "SDL 4.00"  /* Internal Version Number   */
#define HDL_SIZE_HOST_INFO  sizeof(HOST_INFO)
        BLOCK_HEADER;                   /* Name of block             */
/*-------------------- HDR /\ ---------------------------------------*/

        char    sysname[64];
        char    nodename[64];
        char    release[64];
        char    curbuild[64];
        char    version[64];
        char    machine[64];
        char    cpu_brand[64];          /* x86/x64 cpu brand string  */

        int     trycritsec_avail;       /* 1=TryEnterCriticalSection */
        int     maxfilesopen;           /* Max num of open files     */

        int     num_procs;              /* #of processors            */
        int     num_physical_cpu;       /* #of cores                 */
        int     num_logical_cpu;        /* #of of hyperthreads       */
        int     num_packages;           /* #of physical CPUS         */

        int     vector_unit;            /* CPU has vector processor  */
        int     fp_unit;                /* CPU has Floating Point    */
        int     cpu_64bits;             /* CPU is 64 bit             */
        int     cpu_aes_extns;          /* CPU supports aes extension*/
        int     valid_cache_nums;       /* Cache nums are obtained   */

        U64     bus_speed;              /* Motherboard BUS Speed   Hz*/
        U64     cpu_speed;              /* Maximum CPU speed       Hz*/

        U64     cachelinesz;            /* cache line size           */
        U64     L1Icachesz;             /* cache size L1 Inst        */
        U64     L1Dcachesz;             /* cache size L1 Data        */
        U64     L1Ucachesz;             /* cache size L1 Unified     */
        U64     L2cachesz;              /* cache size L2             */
        U64     L3cachesz;              /* cache size L3             */

        U64     hostpagesz;             /* Host page size            */
        U64     AllocationGranularity;  /*                           */
        U64     TotalPhys;              /* Installed Real Memory     */
        U64     AvailPhys;              /* Available Read Memory     */
        U64     TotalPageFile;          /* Size of Swap/Page         */
        U64     AvailPageFile;          /* Free Amt of Swap/Page     */
        U64     TotalVirtual;           /* Virtual Space max         */
        U64     AvailVirtual;           /* Virtual Space in use      */

/*-------------------- TLR \/ ---------------------------------------*/
        BLOCK_TRAILER;                  /* eye-end                   */

} HOST_INFO;

HI_DLL_IMPORT HOST_INFO     hostinfo;
HI_DLL_IMPORT void     init_hostinfo ( HOST_INFO* pHostInfo );
HI_DLL_IMPORT char*  format_hostinfo ( HOST_INFO* pHostInfo,
                                       char*      pszHostInfoStrBuff,
                                       size_t     nHostInfoStrBuffSiz );

/* Hercules Host Information structure  (similar to utsname struct)  */


#endif // _HOSTINFO_H_
