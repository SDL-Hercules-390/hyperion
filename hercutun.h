/* HERCUTUN.H                                                        */
/*              Hercules utun (macOS) Interface Helper Program       */

#define HERCUTUN_CMD "hercutun"
#define HERCUTUN_IF_NAME_PREFIX "utun"

enum HERCUTUN_EXIT_STATUS {
    HERCUTUN_OK = 0,            /* success */
    HERCUTUN_ARG_ERROR,         /* error parsing arguments */
    HERCUTUN_UTUN_ERROR,        /* error opening the utun interface */
    HERCUTUN_IFCONFIG_ERROR,    /* error setting IP addresses or masks */
    HERCUTUN_IPC_ERROR          /* error passing file descriptor to parent */
};