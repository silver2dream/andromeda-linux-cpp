#include <errno.h>  //errno
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "andro_conf.h"
#include "andro_func.h"
#include "andro_macro.h"

int daemon() {
    switch (fork()) {
        case -1:
            log_error_core(ANDRO_LOG_EMERG, errno, "fork() failed in andro_daemon()");
            return -1;
            break;
        case 0:
            break;
        default:
            return 1;
    }

    andro_ppid = andro_pid;
    andro_pid = getpid();

    if (setsid() == -1) {
        log_error_core(ANDRO_LOG_EMERG, errno, "setsid() failed in andro_daemon()");
        return -1;
    }

    // The `umask(0)` function call sets the file mode creation mask to 0, allowing newly created files to have the maximum permissions.
    umask(0);

    int fd = open("/dev/null", O_RDWR);
    if (fd == -1) {
        log_error_core(ANDRO_LOG_EMERG, errno, "opened \"/dev/null\" fail in andro_daemon()");
        return -1;
    }
    if (dup2(fd, STDIN_FILENO) == -1) {
        log_error_core(ANDRO_LOG_EMERG, errno, "dup2(STDIN) failed in andro_daemon()");
        return -1;
    }
    if (dup2(fd, STDOUT_FILENO) == -1) {
        log_error_core(ANDRO_LOG_EMERG, errno, "dup2(STDOUT) failed in andro_daemon()");
        return -1;
    }
    if (fd > STDERR_FILENO) {
        if (close(fd) == -1) {
            log_error_core(ANDRO_LOG_EMERG, errno, "close(fd) failed in andro_daemon()");
            return -1;
        }
    }
    return 0;
}