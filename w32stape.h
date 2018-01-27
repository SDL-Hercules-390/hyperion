/* W32STAPE.H   (C) Copyright "Fish" (David B. Trout), 2005-2012     */
/*               Hercules Win32 SCSI Tape handling module            */
/*                                                                   */
/*   Released under "The Q Public License Version 1"                 */
/*   (http://www.hercules-390.org/herclic.html) as modifications to  */
/*   Hercules.                                                       */

////////////////////////////////////////////////////////////////////////////////////
//
//  This module contains only MSVC support for SCSI tapes.
//  Primary SCSI Tape support is in module 'scsitape.c'...
//
////////////////////////////////////////////////////////////////////////////////////

#ifndef _W32STAPE_H_
#define _W32STAPE_H_

#ifdef _MSVC_

#include "w32mtio.h"        // Win32 version of 'mtio.h'

#define  WIN32_TAPE_DEVICE_NAME    "\\\\.\\Tape0"

W32ST_DLL_IMPORT int     w32_open_tape  ( const char* path, int oflag,   ... );
W32ST_DLL_IMPORT int     w32_define_BOT ( int fd, U32 msk, U32 bot );
W32ST_DLL_IMPORT int     w32_ioctl_tape ( int fd,       int request, ... );
W32ST_DLL_IMPORT int     w32_close_tape ( int fd );
W32ST_DLL_IMPORT ssize_t w32_read_tape  ( int fd,       void* buf, size_t nbyte );
W32ST_DLL_IMPORT ssize_t w32_write_tape ( int fd, const void* buf, size_t nbyte );

#endif // _MSVC_

#endif // _W32STAPE_H_
