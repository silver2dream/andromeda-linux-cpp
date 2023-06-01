#include <arpa/inet.h>
#include <errno.h>   //errno
#include <fcntl.h>   //open
#include <stdarg.h>  //va_start....
#include <stdint.h>  //uintptr_t
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>  //ioctl
#include <sys/time.h>   //gettimeofday
#include <time.h>       //localtime_r
#include <unistd.h>     //STDERR_FILENO

#include "andro_func.h"
#include "andro_macro.h"
#include "andro_socket.h"

void CSocket::event_accept(lp_connection_t old_conn_ptr) {
    struct sockaddr client_sock_addr;
    socklen_t sock_len;
    int err;
    int level;
    int conn_fd;
    static int use_accept4 = 1;
    lp_connection_t new_conn_ptr;

    sock_len = sizeof(client_sock_addr);
    do {
        if (use_accept4) {
            conn_fd = accept4(old_conn_ptr->fd, &client_sock_addr, &sock_len, SOCK_NONBLOCK);
        } else {
            conn_fd = accept(old_conn_ptr->fd, &client_sock_addr, &sock_len);
        }

        if (conn_fd == -1) {
            err = errno;

            if (err == EAGAIN) {
                return;
            }

            level = ANDRO_LOG_ALERT;
            if (err == ECONNABORTED) {
                level = ANDRO_LOG_ERR;
            } else if (err == EMFILE || err == ENFILE) {
                level = ANDRO_LOG_CRIT;
            }
            log_error_core(level, errno, "accept4() failed in CSocket::event_accept()");

            if (use_accept4 && err == ENOSYS) {
                use_accept4 = 0;
                continue;
            }

            if (err == ECONNABORTED) {
            }

            if (err == EMFILE || err == ENFILE) {
            }
            return;
        }

        new_conn_ptr = get_connection(conn_fd);
        if (new_conn_ptr == nullptr) {
            if (close(conn_fd) == -1) {
                log_error_core(ANDRO_LOG_ALERT, errno, "close(%d) failed in CSocket::event_accept", conn_fd);
            }
            return;
        }

        memcpy(&new_conn_ptr->socket_addr, &client_sock_addr, sock_len);

        u_char ip_addr[100];
        memset(ip_addr, 0, sizeof(ip_addr));
        sock_ntop(&new_conn_ptr->socket_addr, 1, ip_addr, sizeof(ip_addr) - 10);
        log_stderr(0, "client ip=%s", ip_addr);

        if (!use_accept4) {
            if (set_nonblocking(conn_fd) == false) {
                close_accepted_connection(new_conn_ptr);
                return;
            }
        }

        new_conn_ptr->listening = old_conn_ptr->listening;
        new_conn_ptr->w_ready = 1;
        new_conn_ptr->rhandler = &CSocket::wait_requset_handler;

        if (EpollAddEvent(conn_fd, 1, 0, EPOLLET, EPOLL_CTL_ADD, new_conn_ptr) == -1) {
            close_accepted_connection(new_conn_ptr);
            return;
        }
        break;
    } while (1);
}

void CSocket::close_accepted_connection(lp_connection_t conn_ptr) {
    int fd = conn_ptr->fd;
    free_connection(conn_ptr);
    conn_ptr->fd = -1;
    if (close(fd) == -1) {
        log_error_core(ANDRO_LOG_ALERT, errno, "close(%d) failed in CSocket::close_accepted_connection", fd);
    }
    return;
}