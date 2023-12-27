#include <cstdio>
#include <arpa/inet.h>

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