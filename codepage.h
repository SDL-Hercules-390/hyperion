/* CODEPAGE.H   (C) Copyright Jan Jaeger, 1999-2012                  */
/*              (C) Copyright TurboHercules, SAS 2010-2011           */
/*              Code Page conversion                                 */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

#ifndef _HERCULES_CODEPAGE_H
#define _HERCULES_CODEPAGE_H

#include "hercules.h"

#ifndef _CODEPAGE_C_
#ifndef _HUTIL_DLL_
#define COD_DLL_IMPORT DLL_IMPORT
#else   /* _HUTIL_DLL_ */
#define COD_DLL_IMPORT extern
#endif  /* _HUTIL_DLL_ */
#else   /* _LOGGER_C_ */
#define COD_DLL_IMPORT DLL_EXPORT
#endif /* _LOGGER_C_ */

COD_DLL_IMPORT char* query_codepage(void);
COD_DLL_IMPORT void set_codepage(char *name);
COD_DLL_IMPORT int update_codepage(int argc, char *argv[], char *table );
COD_DLL_IMPORT unsigned char host_to_guest (unsigned char byte);
COD_DLL_IMPORT unsigned char guest_to_host (unsigned char byte);
COD_DLL_IMPORT unsigned char *h2g_tab();
COD_DLL_IMPORT unsigned char *g2h_tab();

/* helper functions for codepage */
COD_DLL_IMPORT BYTE* buf_guest_to_host( const BYTE *psinbuf, BYTE *psoutbuf, const u_int ilength );
COD_DLL_IMPORT BYTE* str_guest_to_host( const BYTE *psinbuf, BYTE *psoutbuf, const u_int ilength );
COD_DLL_IMPORT BYTE* buf_host_to_guest( const BYTE *psinbuf, BYTE *psoutbuf, const u_int ilength );
COD_DLL_IMPORT BYTE* str_host_to_guest( const BYTE *psinbuf, BYTE *psoutbuf, const u_int ilength );
COD_DLL_IMPORT BYTE* prt_guest_to_host( const BYTE *psinbuf, BYTE *psoutbuf, const u_int ilength );
COD_DLL_IMPORT BYTE* prt_host_to_guest( const BYTE *psinbuf, BYTE *psoutbuf, const u_int ilength );

#endif /* _HERCULES_CODEPAGE_H */
