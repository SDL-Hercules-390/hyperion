/* UTUN.C:                                                           */
/*              Hercules CTCI with utun (macOS) Interface            */

/*
 * macOS hosts don't have built-in tun(4) or tap(4) devices, and
 * recent versions are picky about kernel extensions, so it may not
 * always be convenient to use Mattias Nissler's tuntaposx kext.
 *
 * The XNU kernel does provide a utun network interface, however, with
 * similar functionality but a different API.  See hercutun.c, the
 * helper program that sets up the interface, for details.
 *
*/

#include "hstdinc.h"

#if defined(HAVE_NET_IF_UTUN_H)

#include <net/if_utun.h>
#include "hercules.h"
#include "devtype.h"
#include "ctcadpt.h"
#include "utun.h"
#include "hercutun.h"

int
UTUN_Initialize(int *pUnit,
                const char *pszDriveIPAddr,
                const char *pszGuestIPAddr,
                const char *pszNetMask,
                int *pfd)
{
    int fd[2], nr, status = -1;
    char *hercutun;
    char pszUnit[IF_NAMESIZE-sizeof(HERCUTUN_IF_NAME_PREFIX)+1];
    pid_t pid;
    struct cmsghdr *cmsg;
    struct iovec iov[1];
    struct msghdr msg;

    if (snprintf(pszUnit, sizeof(pszUnit), "%d", *pUnit) >
        (long)sizeof(pszUnit)-1) {
        WRMSG( HHCXU001E, "E", *pUnit);
        return(-1);
    }

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fd) < 0) {
        WRMSG( HHCXU002E, "E", strerror(errno));
        return(-1);
    }

    if (!(hercutun = getenv("HERCULES_UTUN"))) {
        hercutun = HERCUTUN_CMD;
    }

    WRMSG(HHCXU901I, "I",hercutun, pszUnit, pszDriveIPAddr, pszGuestIPAddr, pszNetMask);

    if ((pid = fork()) < 0) {
        WRMSG(HHCXU003E, "E", strerror(errno));
        return(-1);
    } else if (pid == 0) {
        /* in child process*/
        close(fd[0]);
        if (fd[1] != STDIN_FILENO &&
            dup2(fd[1], STDIN_FILENO) != STDIN_FILENO) {
            exit(HERCUTUN_IPC_ERROR);
        }
        if (fd[1] != STDOUT_FILENO &&
            dup2(fd[1], STDOUT_FILENO) != STDOUT_FILENO) {
            exit(HERCUTUN_IPC_ERROR);
        }
        if (execlp(hercutun, hercutun, pszUnit, pszDriveIPAddr,
                  pszGuestIPAddr, pszNetMask, (char *)0) < 0) {
            exit(HERCUTUN_IPC_ERROR);
        }
    }

    /* in parent process */
    close(fd[1]);

    cmsg = malloc(CMSG_LEN(sizeof(int)));
    if (cmsg == NULL) {
        WRMSG(HHCXU004E, "E");
        goto err2;
    }

    iov[0].iov_base = pUnit;
    iov[0].iov_len = sizeof(*pUnit);

    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    msg.msg_control = cmsg;
    msg.msg_controllen = CMSG_LEN(sizeof(int));
    msg.msg_flags = 0;

    if ((nr = recvmsg(fd[0], &msg, 0)) < 0) {
        WRMSG(HHCXU005E, "E", strerror(errno));
        goto err1;
    }

    if (nr == 0) {
        WRMSG(HHCXU006E, "E");
        goto err1;
    }

    if (msg.msg_controllen != CMSG_LEN(sizeof(int))) {
        WRMSG(HHCXU007E, "E");
        goto err1;
    }

    *pfd = *(int *)CMSG_DATA(cmsg);
    free(cmsg);
    waitpid(pid, &status, 0);

    return 0;

 err1:
    free(cmsg);
 err2:
    waitpid(pid, &status, 0);
    switch (WEXITSTATUS(status)) {
    case HERCUTUN_OK:
        WRMSG(HHCXU020I, "I");
        break;
    case HERCUTUN_ARG_ERROR:
        WRMSG(HHCXU021E, "E");
        break;
    case HERCUTUN_UTUN_ERROR:
        WRMSG(HHCXU022E, "E");
        break;
    case HERCUTUN_IFCONFIG_ERROR:
        WRMSG(HHCXU023E, "E");
        break;
    case HERCUTUN_IPC_ERROR:
        WRMSG(HHCXU024E, "E");
        break;
    }
    return -1;
}

/* same prototype as read(2) */
ssize_t
UTUN_Read(int fildes, void *buf, size_t nbyte)
{
    struct iovec iov[2];
    uint32_t header;
    ssize_t rc;

    iov[0].iov_base = &header;
    iov[0].iov_len = sizeof(header);

    iov[1].iov_base = buf;
    iov[1].iov_len = nbyte;

    for (;;) {
        rc = readv(fildes, iov, 2);
        if (rc <= 0) {
            return rc;
        } else if (rc >= (long)sizeof(header) &&
                   header == htonl(AF_INET)) { /* Ignore everything but
                                                  IPv4 datagrams; need to
                                                  fix this for IPv6 */
            return rc - sizeof(header);
        }
    }
}

/* same prototype as write(2) */
ssize_t
UTUN_Write(int fildes, void *buf, size_t nbyte)
{
    struct iovec iov[2];
    uint32_t header = htonl(AF_INET); /* Assume it's an IPv4 datagram;
                                         need to fix this for IPv6 */

    iov[0].iov_base = &header;
    iov[0].iov_len = sizeof(header);

    iov[1].iov_base = buf;
    iov[1].iov_len = nbyte;

    return writev(fildes, iov, 2);
}

#endif /* defined(HAVE_NET_IF_UTUN_H) */