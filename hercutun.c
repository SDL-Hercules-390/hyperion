/* HERCUTUN.C                                                        */
/*              Hercules utun (macOS) Interface Helper Program       */

/*
 * This program creates a utun network interface, as supported by
 * recent versions of the XNU kernel on macOS; configures it for IPv4,
 * IPv6, or both; and sends an open file descriptor and the unit
 * number of the interface (which may have been selected dynamically)
 * to its parent process, which will use the file descriptor to send
 * and receive packets.
 *
 * Since the utun kernel control and the SIOCAIFADDR and
 * SIOCAIFADDR_IN6 ioctl requests are available only to privileged
 * processes, this program will normally be installed set-user-ID
 * root, so the rest of Hercules need not run with special privileges.
 * File mode 4750 (-rwsr-x---) is suggested, with the group set to
 * "admin" or a locally-defined group of authorized Hercules users.
 *
 * This program is intended to be invoked only from utun.c in
 * Hercules, not interactively from a shell.  It accepts three, four,
 * or six command-line arguments:
 *
 * hercutun unit IPv6-addr dst-IPv6-addr
 * hercutun unit IPv4-addr dst-IPv4-addr IPv4-mask
 * hercutun unit IPv4-addr dst-IPv4-addr IPv4-mask IPv6-addr dst-IPv6-addr
 *
 * where:
 *
 * - unit is the utun interface number (e.g., 0 for "utun0") or -1 to
 *   specify that the first available utun interface should be used;
 *
 * - IPv4-addr and dst-IPv4-addr are the IPv4 addresses of the host
 *   (driving) system and Hercules guest, respectively;
 *
 * - IPv4-mask is the IPv4 subnet mask for the virtual point-to-point
 *   link containing IPv4-addr and dst-IPv4-addr; and
 *
 * - IPv6-addr and dst-IPv6-addr are the IPv6 addresses of the host
 *   (driving) system and Hercules guest, respectively.
 *
 * The IPv6 prefix length is always 128 bits for point-to-point
 * interfaces.  If the IPv4 or IPv6 addresses are not specified, the
 * interface is not configured for IPv4 or IPv6, respectively.
 *
 * For security reasons, this program opens no files and invokes no other
 * programs.  Errors are communicated to the parent Hercules process
 * through the exit status, defined in hercutun.h.
 *
 * The utun interface on macOS isn't covered in Apple's developer
 * documentation, but it is described in the system header file
 * /usr/include/net/if_utun.h and in the XNU kernel source code:
 *
 * https://github.com/apple/darwin-xnu/blob/master/bsd/net/if_utun.c
 * https://github.com/apple/darwin-xnu/blob/master/bsd/net/if_utun.h
 *
 * This code is inspired by Jonathan Levin's example:
 *
 * http://newosxbook.com/src.jl?tree=listings&file=17-15-utun.c
 *
 * The technique for passing file descriptors is described in Advanced
 * Programming in the UNIX Environment, 3rd ed., pp. 642-669.
 */

#include "config.h"

#if defined(BUILD_HERCUTUN)

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/sys_domain.h>
#include <sys/kern_control.h>
#include <net/if.h>
#include <net/if_utun.h>
#include <netinet/in.h>
#include <netinet6/in6_var.h>
#include <netinet6/nd6.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "hercutun.h"

int open_utun(int *);
void ifconfig_ipv4(const int, const char *, const char *, const char *);
void ifconfig_ipv6(const int, const char *, const char *);
void send_fd(int, int);
void print_usage_and_exit(void);

int
open_utun(int *unit)
{
    int fd;
    struct ctl_info ci;
    struct sockaddr_ctl sc;

    memset(&ci, 0, sizeof(ci));
    if (strlcpy(ci.ctl_name, UTUN_CONTROL_NAME, sizeof(ci.ctl_name)) >=
        sizeof(ci.ctl_name)) {
        fprintf(stderr, "HHCXU111E: Too many digits in utun unit\n");
        exit(HERCUTUN_UTUN_ERROR);
    }

    fd = socket(PF_SYSTEM, SOCK_DGRAM, SYSPROTO_CONTROL);
    if (fd < 0) {
        fprintf(stderr, "HHCXU112E: utun socket() failed: %s\n",
                strerror(errno));
        exit(HERCUTUN_UTUN_ERROR);
    }

    if (ioctl(fd, CTLIOCGINFO, &ci) < 0) {
        fprintf(stderr, "HHCXU113E: utun ioctl() failed: %s\n",
                strerror(errno));
        exit(HERCUTUN_UTUN_ERROR);
    }

    sc.sc_id = ci.ctl_id;
    sc.sc_len = sizeof(sc);
    sc.sc_family = AF_SYSTEM;
    sc.ss_sysaddr = AF_SYS_CONTROL;

    if (*unit < 0) {
        sc.sc_unit = 1;
    } else {
        sc.sc_unit = (uint32_t)(*unit + 1);
    }

    while (connect(fd, (struct sockaddr *)&sc, sizeof(sc)) < 0) {
        if (*unit < 0 && sc.sc_unit < 256 && errno == EBUSY) {
            sc.sc_unit++;
        } else {
            fprintf(stderr, "HHCXU114E: utun connect() failed: %s\n",
                    strerror(errno));
            exit(HERCUTUN_UTUN_ERROR);
        }
    }

    *unit = (int)(sc.sc_unit - 1);
    return fd;
}

void
ifconfig_ipv4(const int unit, const char *ipv4_addr,
              const char *ipv4_dst_addr, const char *ipv4_mask)
{
    int cfd;
    struct sockaddr_in addr, dstaddr, mask;
    struct ifaliasreq ifra;

    memset(&addr, 0, sizeof(addr));
    addr.sin_len = sizeof(addr);
    addr.sin_family = AF_INET;
    if (inet_pton(AF_INET, ipv4_addr, &addr.sin_addr) != 1) {
        fprintf(stderr, "HHCXU121E: Invalid IPv4-addr\n");
        print_usage_and_exit();
    }

    memset(&dstaddr, 0, sizeof(dstaddr));
    dstaddr.sin_len = sizeof(dstaddr);
    dstaddr.sin_family = AF_INET;
    if (inet_pton(AF_INET, ipv4_dst_addr, &dstaddr.sin_addr) != 1) {
        fprintf(stderr, "HHCXU122E: Invalid dst-IPv4-addr\n");
        print_usage_and_exit();
    }

    memset(&mask, 0, sizeof(mask));
    mask.sin_len = sizeof(mask);
    /* Don't set address family for mask; see Darwin ifconfig(8) source */
    if (inet_pton(AF_INET, ipv4_mask, &mask.sin_addr) != 1) {
        fprintf(stderr, "HHCXU123E: Invalid IPv4-mask\n");
        print_usage_and_exit();
    }

    memset(&ifra, 0, sizeof(ifra));
    if (snprintf(ifra.ifra_name, IF_NAMESIZE,
                 HERCUTUN_IF_NAME_PREFIX "%d", unit) >= IF_NAMESIZE) {
        fprintf(stderr, "HHCXU124E: Too many digits in utun unit\n");
        print_usage_and_exit();
    }
    memcpy(&ifra.ifra_addr, &addr, sizeof(struct sockaddr_in));
    memcpy(&ifra.ifra_broadaddr, &dstaddr, sizeof(struct sockaddr_in));
    memcpy(&ifra.ifra_mask, &mask, sizeof(struct sockaddr_in));

    cfd = socket(PF_INET, SOCK_DGRAM, 0);
    if (cfd < 0) {
        fprintf(stderr, "HHCXU125E: IPv4 socket() failed: %s\n",
                strerror(errno));
        exit(HERCUTUN_IFCONFIG_ERROR);
    }

    if (ioctl(cfd, SIOCAIFADDR, &ifra) < 0) {
        fprintf(stderr, "HHCXU126E: IPv4 ioctl() failed: %s\n",
                strerror(errno));
        exit(HERCUTUN_IFCONFIG_ERROR);
    }

    if (close(cfd) != 0) {
        fprintf(stderr, "HHCXU127E: IPv4 close() failed: %s\n",
                strerror(errno));
        exit(HERCUTUN_IFCONFIG_ERROR);
    }
}

void
ifconfig_ipv6(const int unit, const char *ipv6_addr,
              const char *ipv6_dst_addr)
{
    int cfd;
    struct sockaddr_in6 addr, dstaddr, mask;
    struct in6_aliasreq ifra;

    memset(&addr, 0, sizeof(addr));
    addr.sin6_len = sizeof(addr);
    addr.sin6_family = AF_INET6;
    if (inet_pton(AF_INET6, ipv6_addr, &addr.sin6_addr) != 1) {
        fprintf(stderr, "HHCXU131E: Invalid IPv6-addr\n");
        print_usage_and_exit();
    }

    memset(&dstaddr, 0, sizeof(dstaddr));
    dstaddr.sin6_len = sizeof(dstaddr);
    dstaddr.sin6_family = AF_INET6;
    if (inet_pton(AF_INET6, ipv6_dst_addr, &dstaddr.sin6_addr) != 1) {
        fprintf(stderr, "HHCXU132E: Invalid dst-IPv6-addr\n");
        print_usage_and_exit();
    }

    memset(&mask, 0, sizeof(mask));
    mask.sin6_len = sizeof(mask);
    /* Don't set address family for mask; see Darwin ifconfig(8) source */
    /* For point-to-point IPv6 interfaces with explicitly-set destination
       addresses, the prefix length must be 128; see in6_update_ifa() in
       https://github.com/apple/darwin-xnu/blob/master/bsd/netinet6/in6.c */
    memset(mask.sin6_addr.s6_addr, 0xFF, 16);

    memset(&ifra, 0, sizeof(ifra));
    if (snprintf(ifra.ifra_name, IF_NAMESIZE,
                 HERCUTUN_IF_NAME_PREFIX "%d", unit) >= IF_NAMESIZE) {
        fprintf(stderr, "HHCXU134E: Too many digits in utun unit\n");
        print_usage_and_exit();
    }
    ifra.ifra_lifetime.ia6t_vltime = ND6_INFINITE_LIFETIME;
    ifra.ifra_lifetime.ia6t_pltime = ND6_INFINITE_LIFETIME;
    memcpy(&ifra.ifra_addr, &addr, sizeof(struct sockaddr_in6));
    memcpy(&ifra.ifra_dstaddr, &dstaddr, sizeof(struct sockaddr_in6));
    memcpy(&ifra.ifra_prefixmask, &mask, sizeof(struct sockaddr_in6));

    cfd = socket(PF_INET6, SOCK_DGRAM, 0);
    if (cfd < 0) {
        fprintf(stderr, "HHCXU135E: IPv6 socket() failed: %s\n",
                strerror(errno));
        exit(HERCUTUN_IFCONFIG_ERROR);
    }

    if (ioctl(cfd, SIOCAIFADDR_IN6, &ifra) < 0) {
        fprintf(stderr, "HHCXU136E: IPv6 ioctl() failed: %s\n",
                strerror(errno));
        exit(HERCUTUN_IFCONFIG_ERROR);
    }

    if (close(cfd) != 0) {
        fprintf(stderr, "HHCXU137E: IPv6 close() failed: %s\n",
                strerror(errno));
        exit(HERCUTUN_IFCONFIG_ERROR);
    }
}

void
send_fd(int fd, int unit) {
    struct cmsghdr *cmsg;
    struct iovec iov[1];
    struct msghdr msg;

    cmsg = malloc(CMSG_LEN(sizeof(int)));
    if (cmsg == NULL) {
        fprintf(stderr, "HHCXU141E: malloc() failed\n");
        exit(HERCUTUN_IPC_ERROR);
    }

    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(int));
    *(int *)CMSG_DATA(cmsg) = fd;

    /* Send the actual unit number, just for the parent's information. */
    iov[0].iov_base = &unit;
    iov[0].iov_len = sizeof(unit);

    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    msg.msg_control = cmsg;
    msg.msg_controllen = CMSG_LEN(sizeof(int));
    msg.msg_flags = 0;

    if (sendmsg(STDOUT_FILENO, &msg, 0) < 0) {
        fprintf(stderr, "HHCXU142E: sendmsg() failed: %s\n", strerror(errno));
        exit(HERCUTUN_IPC_ERROR);
    }
}

void
print_usage_and_exit(void)
{
    fprintf(stderr, "HHCXU101W: hercutun should be invoked only by "
            "Hercules, not interactively.\n\n"
            "Usage: hercutun unit IPv6-addr dst-IPv6-addr\n"
            "       hercutun unit IPv4-addr dst-IPv4-addr IPv4-mask\n"
            "       hercutun unit IPv4-addr dst-IPv4-addr IPv4-mask "
            "IPv6-addr dst-IPv6-addr\n");
    exit(HERCUTUN_ARG_ERROR);
}

int
main(int argc, char **argv)
{
    int fd, unit;
    long unit_arg;
    char *ipv4_addr = NULL, *ipv4_dst_addr, *ipv4_mask,
        *ipv6_addr = NULL, *ipv6_dst_addr;
    char *endptr = NULL;

    if (argc == 4) {
        ipv6_addr = argv[2];
        ipv6_dst_addr = argv[3];
    } else if (argc == 5) {
        ipv4_addr = argv[2];
        ipv4_dst_addr = argv[3];
        ipv4_mask = argv[4];
    } else if (argc == 7) {
        ipv4_addr = argv[2];
        ipv4_dst_addr = argv[3];
        ipv4_mask = argv[4];
        ipv6_addr = argv[5];
        ipv6_dst_addr = argv[6];
    } else {
        print_usage_and_exit();
    }

    unit_arg = strtol(argv[1], &endptr, 10);
    if (*endptr != '\0' || unit_arg > INT_MAX || unit_arg < -1) {
        fprintf(stderr, "HHCXU102E: utun unit out of range\n");
        print_usage_and_exit();
    }
    unit = (int)unit_arg;

    fd = open_utun(&unit);

    if (ipv4_addr) {
        ifconfig_ipv4(unit, ipv4_addr, ipv4_dst_addr, ipv4_mask);
    }

    if (ipv6_addr) {
        ifconfig_ipv6(unit, ipv6_addr, ipv6_dst_addr);
    }

    send_fd(fd, unit);

    exit(HERCUTUN_OK);
}

#endif /* defined(BUILD_HERCUTUN) */
