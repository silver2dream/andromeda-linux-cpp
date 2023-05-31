
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
    worker_max_connections = 1;
    listen_port_count = 1;

    epoll_handler = -1;
    connections_ptr = nullptr;
    free_connections_ptr = nullptr;
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
    read_conf();
    bool result = open_listening_sockets();
    return result;
}

void CSocket::read_conf() {
    CConfig *config = CConfig::GetInstance();
    worker_max_connections = config->GetIntDefault(ANDRO_CONF_WORK_MAX_CONNECTIONS, worker_max_connections);
    listen_port_count = config->GetIntDefault(ANDRO_CONF_LISTEN_PORT_COUNT, listen_port_count);
    return;
}

bool CSocket::open_listening_sockets() {
    int socket_fd;
    struct sockaddr_in serv_addr;
    int port;

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    CConfig *config = CConfig::GetInstance();
    for (int i = 0; i < listen_port_count; i++) {
        socket_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (socket_fd == -1) {
            log_stderr(errno, "socket() occur failed in CSocket::Init(), i=%d", i);
        }

        int reuse_addr = 1;
        if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, (const void *)&reuse_addr, sizeof(reuse_addr)) == -1) {
            log_stderr(errno, "setsockopt(SO_REUSEADDR) failed in in CSocket::Init(), i=%d", i);
            close(socket_fd);
            return false;
        }

        if (set_nonblocking(socket_fd) == false) {
            log_stderr(errno, "set_nonblocking failed in in CSocket::Init(), i=%d", i);
            close(socket_fd);
            return false;
        }

        port = config->GetIntDefault(Andro_Str_Combine(ANDRO_CONF_PORT_PRIFIX, i), 10000);
        serv_addr.sin_port = htons(in_port_t(port));

        if (bind(socket_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1) {
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

    if (listen_socket_list.size() <= 0) return false;

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

int CSocket::EpollInit() {
    epoll_handler = epoll_create(worker_max_connections);
    if (epoll_handler == -1) {
        log_stderr(errno, "epoll_create() failed in EpollInit()");
        exit(2);
    }

    connection_num = worker_max_connections;
    connections_ptr = new connection_t[connection_num];

    int i = connection_num;
    lp_connection_t next = nullptr;
    lp_connection_t conn = connections_ptr;
    do {
        i--;

        conn[i].next_conn = next;
        conn[i].fd = -1;
        conn[i].instance = 1;
        conn[i].sequence = 0;
        next = &conn[i];
    } while (i);

    free_connections_ptr = next;
    free_connection_num = connection_num;

    std::vector<lp_listening_t>::iterator pos;
    for (pos = listen_socket_list.begin(); pos != listen_socket_list.end(); ++pos) {
        conn = get_connection((*pos)->fd);
        if (conn == nullptr) {
            log_stderr(errno, "get_free_connection() failed in EpollInit()");
            exit(2);
        }

        conn->listening = (*pos);
        (*pos)->connection = conn;

        conn->rhandler = &CSocket::event_accept;

        if (EpollAddEvent((*pos)->fd, 1, 0, 0, EPOLL_CTL_ADD, conn) == -1) {
            exit(2);
        }
    }

    return 1;
}

int CSocket::EpollAddEvent(int fd, int read_event, int write_event, uint32_t other_flag, uint32_t event_type, lp_connection_t conn_ptr) {
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));

    if (read_event == 1) {
        ev.events = EPOLLIN | EPOLLRDHUP;
    }

    if (other_flag != 0) {
        ev.events |= other_flag;
    }

    ev.data.ptr = (void *)((uintptr_t)conn_ptr | conn_ptr->instance);

    if (epoll_ctl(epoll_handler, event_type, fd, &ev) == -1) {
        log_stderr(errno, "epoll_ctl(%d,%d,%d,%u,%u) failed in EpollAddEvent()", fd, read_event, write_event, other_flag, event_type);
        return -1;
    }
    return 1;
}

int CSocket::EpollProcessEvents(int timer) {
    int events = epoll_wait(epoll_handler, epoll_events, ANDRO_MAX_EPOLL_WAIT_EVENTS, timer);
    if (events == -1) {
        if (errno == EINTR) {
            log_error_core(ANDRO_LOG_INFO, errno, "epoll_wait() failed in CSocket::EpollProcessEvents()");
            return 1;
        } else {
            log_error_core(ANDRO_LOG_ALERT, errno, "epoll_wait() failed in CSocket::EpollProcessEvents()");
            return 0;
        }
    }

    if (events == 0) {
        if (timer != -1) {
            return 1;
        }
        log_error_core(ANDRO_LOG_ALERT, 0, "epoll_wait() is not timeout and have no events return, in CSocket::EpollProcessEvents()");
        return 0;
    }

    lp_connection_t conn;
    uintptr_t instance;
    uint32_t revents;
    for (int i = 0; i < events; ++i) {
        conn = (lp_connection_t)(epoll_events[i].data.ptr);
        instance = (uintptr_t)conn & 1;
        conn = (lp_connection_t)((uintptr_t)conn & (uintptr_t)~1);

        if (conn->fd == -1) {
            log_error_core(ANDRO_LOG_DEBUG, 0, "the connection[%p]'s fd = -1 that is expired event in EpollProcessEvents()", conn);
            continue;
        }

        if (conn->instance != instance) {
            log_error_core(ANDRO_LOG_DEBUG, 0, "the connection[%p]'s instance has changed that is expired event in EpollProcessEvents()", conn);
            continue;
        }

        revents = epoll_events[i].events;
        if (revents & (EPOLLERR | EPOLLHUP)) {
            revents |= EPOLLIN | EPOLLOUT;
        }
        if (revents & EPOLLIN) {
            (this->*(conn->rhandler))(conn);
        }

        if (revents & EPOLLOUT) {
            log_stderr(errno, "11111111111");
        }
    }
    return 1;
}