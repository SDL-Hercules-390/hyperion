/* UTUN.H                                                            */
/*              Hercules CTCI with utun (macOS) Interface            */

#ifndef __UTUN_H_
#define __UTUN_H_

#if defined(HAVE_NET_IF_UTUN_H)

#include "hercules.h"

extern int UTUN_Initialize(int *pUnit,
                           const char *pszDriveIPAddr,
                           const char *pszGuestIPAddr,
                           const char *pszNetMask,
                           int *pfd);

extern ssize_t UTUN_Read(int fildes, void *buf, size_t nbyte);

extern ssize_t UTUN_Write(int fildes, void *buf, size_t nbyte);

#endif /* defined(HAVE_NET_IF_UTUN_H) */

#endif /* __UTUN_H_ */