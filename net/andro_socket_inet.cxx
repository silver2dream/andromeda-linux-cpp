#include <errno.h>   //errno
#include <fcntl.h>   //open
#include <stdarg.h>  //va_start....
#include <stdint.h>  //uintptr_t
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>  //gettimeofday
#include <time.h>      //localtime_r
#include <unistd.h>    //STDERR_FILENO
// #include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>  //ioctl

#include "andro_func.h"
#include "andro_socket.h"

size_t CSocket::sock_ntop(struct sockaddr *sock_addr, int port, u_char *text, size_t len) {
    struct sockaddr_in *sin;
    u_char *p;

    switch (sock_addr->sa_family) {
        case AF_INET:
            sin = (struct sockaddr_in *)sock_addr;
            p = (u_char *)&sin->sin_addr;
            if (port) {
                p = snprintf(text, len, "%ud.%ud.%ud.%ud:%d", p[0], p[1], p[2], p[3], ntohs(sin->sin_port));
            } else {
                p = snprintf(text, len, "%ud.%ud.%ud.%ud", p[0], p[1], p[2], p[3]);
            }
            return (p - text);
            break;
        default:
            return 0;
            break;
    }
    return 0;
}