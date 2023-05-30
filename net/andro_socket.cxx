
#include "andro_socket.h"

#include <arpa/inet.h>
#include <errno.h>   //errno
#include <fcntl.h>   //open
#include <stdarg.h>  //va_start....
#include <stdint.h>  //uintptr_t
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>  //ioctl
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>  //gettimeofday
#include <time.h>      //localtime_r
#include <unistd.h>    //STDERR_FILENO

#include "andro_conf.h"
#include "andro_func.h"
#include "andro_macro.h"

CSocket::CSocket() {
    listen_port_count = 1;
    return;
}

CSocket::~CSocket() {
    std::vector<lp_listening_t>::iterator pos;
    for (pos = listen_socket_list.begin(); pos != listen_socket_list.end(); ++pos) {
        delete (*pos);
    }
    listen_socket_list.clear();
    return;
}

bool CSocket::Init() {
    bool result = open_listening_sockets();
    return result;
}

bool CSocket::open_listening_sockets() {
    auto config = CConfig::GetInstance();
    listen_port_count = config->GetIntDefault(ANDRO_CONF_LISTEN_PORT_COUNT, listen_port_count);

    int socket_fd;
    struct sockaddr_in serv_addr;
    int port;

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    for (int i = 0; i < listen_port_count; i++) {
        socket_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (socket_fd == -1) {
            log_stderr(errno, "socket() occur failed in CSocket::Init(), i=%d", i);
        }

        int reuse_addr = 1;
        if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, (const void*)&reuse_addr, sizeof(reuse_addr)) == -1) {
            log_stderr(errno, "setsockopt(SO_REUSEADDR) failed in in CSocket::Init(), i=%d", i);
            close(socket_fd);
            return false;
        }

        if (set_nonblocking(socket_fd) == false) {
            log_stderr(errno, "set_nonblocking failed in in CSocket::Init(), i=%d", i);
            close(socket_fd);
            return false;
        }

        port = config->GetIntDefault(andro_str_combine(ANDRO_PORT_PRIFIX, i), 10000);
        serv_addr.sin_port = htons(in_port_t(port));

        if (bind(socket_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
            log_stderr(errno, "bind() failed in in CSocket::Init(), i=%d", i);
            close(socket_fd);
            return false;
        }

        if (listen(socket_fd, ANDRO_LISTEN_BACKLOG) == -1) {
            log_stderr(errno, "listen() failed in in CSocket::Init(), i=%d", i);
            close(socket_fd);
            return false;
        }

        lp_listening_t listen_socket_item = new listening_t;
        memset(listen_socket_item, 0, sizeof(listening_t));
        listen_socket_item->fd = socket_fd;
        listen_socket_item->port = port;

        log_error_core(ANDRO_LOG_INFO, 0, "liten() successful, port=%d", port);
        listen_socket_list.push_back(listen_socket_item);
    }

    return true;
}

bool CSocket::set_nonblocking(int socket_fd) {
    // 0: clean, 1: setup
    int non_blocking = 1;
    if (ioctl(socket_fd, FIONBIO, &non_blocking) == -1) {
        return false;
    }
    return true;
}

void CSocket::close_listening_sockets() {
    for (int i = 0; i < listen_port_count; i++) {
        close(listen_socket_list[i]->fd);
        log_error_core(ANDRO_LOG_INFO, 0, "close listen port[%d]", listen_socket_list[i]->port);
    }
    return;
}