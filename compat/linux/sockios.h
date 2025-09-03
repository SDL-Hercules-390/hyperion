/* linux/sockios.h - compatibility shim for FreeBSD
 *
 * Drop this into your include path when porting Linux code that expects
 * <linux/sockios.h>. It maps to <sys/sockio.h> and defines missing constants
 * where possible. Some ioctls have no direct FreeBSD equivalent and are
 * left undefined (you may need to rewrite code that uses them).
 */

#ifndef _LINUX_SOCKIOS_H

#define _LINUX_SOCKIOS_H
#include <sys/sockio.h>

#endif
