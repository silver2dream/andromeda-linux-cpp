
#include "andro_socket.h"

#include <arpa/inet.h>
#include <errno.h>    //errno
#include <fcntl.h>    //open
#include <pthread.h>  //
#include <stdarg.h>   //va_start....
#include <stdint.h>   //uintptr_t
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>  //ioctl
#include <sys/time.h>   //gettimeofday
#include <time.h>       //localtime_r
#include <unistd.h>     //STDERR_FILENO

#include <string>

#include "andro_conf.h"
#include "andro_func.h"
#include "andro_global.h"
#include "andro_lockmutex.h"
#include "andro_macro.h"
#include "andro_memory.h"

CSocket::CSocket() {
    worker_max_connections = 1;
    listen_port_count = 1;
    recy_connection_wait_time = 60;

    epoll_handler = -1;

    msg_send_queue_size = 0;
    total_recy_connection_size = 0;

    return;
}

bool CSocket::Init() {
    read_conf();
    bool result = open_listening_sockets();
    return result;
}

bool CSocket::InitSubProc() {
    if (pthread_mutex_init(&send_msg_queue_mutex, NULL) != 0) {
        log_stderr(0, "pthread_mutex_init(&send_msg_queue_mutex) failed in CSocket::InitSubProc");
        return false;
    }

    if (pthread_mutex_init(&connection_mutex, NULL) != 0) {
        log_stderr(0, "pthread_mutex_init(&connection_mutex) failed in CSocket::InitSubProc");
        return false;
    }

    if (pthread_mutex_init(&recy_conn_queue_mutex, NULL) != 0) {
        log_stderr(0, "pthread_mutex_init(&recy_conn_queue_mutex) failed in CSocket::InitSubProc");
        return false;
    }

    if (sem_init(&sem_event_send_queue, 0, 0) == -1) {
        log_stderr(0, "sem_init(&sem_event_send_queue, 0, 0) failed in CSocket::InitSubProc");
        return false;
    }

    int err;
    lp_thread_t send_queue_ptr;
    thread_container.push_back(send_queue_ptr = new thread_t(this));
    err = pthread_create(&send_queue_ptr->handle, NULL, ServerSendQueueThread, send_queue_ptr);
    if (err != 0) {
        return false;
    }

    lp_thread_t recy_conn_ptr;
    thread_container.push_back(recy_conn_ptr = new thread_t(this));
    err = pthread_create(&recy_conn_ptr->handle, NULL, ServerRecyConnectionThread, recy_conn_ptr);
    if (err != 0) {
        return false;
    }
    return true;
}

CSocket::~CSocket() {
    std::vector<lp_listening_t>::iterator pos;
    for (pos = listen_socket_list.begin(); pos != listen_socket_list.end(); ++pos) {
        delete (*pos);
    }
    listen_socket_list.clear();

    return;
}

void CSocket::ShutdownSubProc() {
    std::vector<lp_thread_t>::iterator iter;
    for (iter = thread_container.begin(); iter != thread_container.end(); iter++) {
        pthread_join((*iter)->handle, NULL);
    }

    for (iter = thread_container.begin(); iter != thread_container.end(); iter++) {
        if (*iter)
            delete *iter;
    }
    thread_container.clear();

    clear_msg_send_queue();
    clear_connetion_pool();

    pthread_mutex_destroy(&connection_mutex);
    pthread_mutex_destroy(&send_msg_queue_mutex);
    pthread_mutex_destroy(&recy_conn_queue_mutex);
    sem_destroy(&sem_event_send_queue);
}

void CSocket::clear_msg_send_queue() {
    char *tmp_ptr;
    CMemory *memory = CMemory::GetInstance();

    while (!msg_send_queue.empty()) {
        tmp_ptr = msg_send_queue.front();
        msg_send_queue.pop_front();
        memory->FreeMemeory(tmp_ptr);
    }
}

void CSocket::read_conf() {
    CConfig *config = CConfig::GetInstance();
    worker_max_connections = config->GetIntDefault(ANDRO_CONF_WORK_MAX_CONNECTIONS, worker_max_connections);
    listen_port_count = config->GetIntDefault(ANDRO_CONF_LISTEN_PORT_COUNT, listen_port_count);
    recy_connection_wait_time = config->GetIntDefault(ANDRO_CONF_RECY_WAIT_TIME, recy_connection_wait_time);
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

        auto port_str = ANDRO_CONF_PORT_PRIFIX + std::to_string(i);
        port = config->GetIntDefault(port_str.c_str(), 10000);
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

void CSocket::msg_send(char *send_buffer) {
    CLock lock(&send_msg_queue_mutex);
    msg_send_queue.push_back(send_buffer);
    ++msg_send_queue_size;

    if (sem_post(&sem_event_send_queue) == -1) {
        log_stderr(0, "sem_post(&sem_event_send_queue) failed in CSocket::msg_send");
    }
    return;
}

int CSocket::EpollInit() {
    epoll_handler = epoll_create(worker_max_connections);
    if (epoll_handler == -1) {
        log_stderr(errno, "epoll_create() failed in EpollInit()");
        exit(2);
    }

    init_connetion_pool();

    std::vector<lp_listening_t>::iterator pos;
    for (pos = listen_socket_list.begin(); pos != listen_socket_list.end(); ++pos) {
        lp_connection_t conn = get_connection((*pos)->fd);
        if (conn == nullptr) {
            log_stderr(errno, "get_free_connection() failed in EpollInit()");
            exit(2);
        }

        conn->listening = (*pos);
        (*pos)->connection = conn;

        conn->rhandler = &CSocket::event_accept;

        if (EpollOperateEvent((*pos)->fd, EPOLL_CTL_ADD, EPOLLIN | EPOLLRDHUP, 0, conn) == -1) {
            exit(2);
        }
    }

    return 1;
}

int CSocket::EpollOperateEvent(int fd, uint32_t event_type, uint32_t other_flag, int bcation, lp_connection_t conn_ptr) {
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));

    if (event_type == EPOLL_CTL_ADD) {
        ev.data.ptr = (void *)conn_ptr;
        ev.events = other_flag;
        conn_ptr->events = other_flag;
    } else if (event_type == EPOLL_CTL_MOD) {
        // Do nothing.
    } else {
        // Do nothing.
        return 1;
    }

    if (epoll_ctl(epoll_handler, event_type, fd, &ev) == -1) {
        log_stderr(errno, "epoll_ctl(%d,%ud,%ud,%d) failed in EpollOperateEvent()", fd, event_type, other_flag, bcation);
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
        revents = epoll_events[i].events;

        if (revents & EPOLLIN) {
            (this->*(conn->rhandler))(conn);
        }

        if (revents & EPOLLOUT) {
            log_stderr(errno, "11111111111");
        }
    }
    return 1;
}

void *CSocket::ServerSendQueueThread(void *thread_data) {
    lp_thread_t thread = static_cast<lp_thread_t>(thread_data);
    CSocket *socket_ptr = thread->This;
    int err;
    std::list<char *>::iterator pos, pos2, posend;

    char *msg_buffer;
    lp_message_header_t msg_header_ptr;
    lp_packet_header_t pkg_header_ptr;
    lp_connection_t conn_ptr;
    unsigned short tmp;
    ssize_t send_size;

    CMemory *memory = CMemory::GetInstance();

    while (G_STOP_EVENT == 0) {
        if (sem_wait(&socket_ptr->sem_event_send_queue) == -1) {
            if (errno != EINTR) {
                log_stderr(errno, "sem_wait(&socket_ptr->sem_event_send_queue failed in CSocket::ServerSendQueueThread");
            }
        }

        if (G_STOP_EVENT != 0) {
            break;
        }

        if (socket_ptr->msg_send_queue_size > 0) {
            err = pthread_mutex_lock(&socket_ptr->send_msg_queue_mutex);
            if (err != 0) {
                log_stderr(err, "pthread_mutex_lock(&socket_ptr->send_msg_queue_mutex) failed in CSocket::ServerSendQueueThread, err=%d", err);
            }

            pos = socket_ptr->msg_send_queue.begin();
            posend = socket_ptr->msg_send_queue.end();

            while (pos != posend) {
                msg_buffer = (*pos);
                msg_header_ptr = (lp_message_header_t)msg_buffer;
                pkg_header_ptr = (lp_packet_header_t)(msg_buffer + ANDRO_MSG_HEADER_LEN);
                conn_ptr = msg_header_ptr->conn_ptr;

                if (conn_ptr->sequence != msg_header_ptr->sequence) {
                    pos2 = pos;
                    pos++;
                    socket_ptr->msg_send_queue.erase(pos2);
                    --socket_ptr->msg_send_queue_size;
                    memory->FreeMemeory(msg_buffer);
                    continue;
                }

                if (conn_ptr->throw_send_count > 0) {
                    pos++;
                    continue;
                }

                conn_ptr->allocated_packet_send_mem_ptr = msg_buffer;
                pos2 = pos;
                pos++;
                socket_ptr->msg_send_queue.erase(pos2);
                --socket_ptr->msg_send_queue_size;
                conn_ptr->packet_send_buf_ptr = (char *)pkg_header_ptr;
                tmp = ntohs(pkg_header_ptr->pkg_len);
                conn_ptr->packet_send_len = tmp;

                log_stderr(errno, "about to send data %ud", conn_ptr->packet_send_len);

                send_size = socket_ptr->send_proc(conn_ptr, conn_ptr->packet_send_buf_ptr, conn_ptr->packet_send_len);
                if (send_size > 0) {
                    if (send_size == conn_ptr->packet_send_len) {
                        memory->FreeMemeory(conn_ptr->allocated_packet_send_mem_ptr);
                        conn_ptr->allocated_packet_send_mem_ptr = nullptr;
                        conn_ptr->throw_send_count = 0;
                        log_stderr(0, "sended data finish in CSocket::ServerSendQueueThread");
                    } else {
                        conn_ptr->packet_send_buf_ptr = conn_ptr->packet_send_buf_ptr + send_size;
                        conn_ptr->packet_send_len = conn_ptr->packet_send_len - send_size;
                        ++conn_ptr->throw_send_count;
                        if (socket_ptr->EpollOperateEvent(conn_ptr->fd, EPOLL_CTL_MOD, EPOLLOUT, 0, conn_ptr) == -1) {
                            log_stderr(errno, "EpollOperateEvent() failed in CSocket::ServerSendQueueThread");
                        }
                        log_stderr(errno, "send's buffer was full, part[%d] of total[%d]", send_size, conn_ptr->packet_send_len);
                    }
                    continue;
                } else if (send_size == 0) {
                    memory->FreeMemeory(conn_ptr->allocated_packet_send_mem_ptr);
                    conn_ptr->allocated_packet_send_mem_ptr = nullptr;
                    conn_ptr->throw_send_count = 0;
                    continue;
                } else if (send_size == -1) {
                    ++conn_ptr->throw_send_count;
                    if (socket_ptr->EpollOperateEvent(conn_ptr->fd, EPOLL_CTL_MOD, EPOLLOUT, 0, conn_ptr) == -1) {
                        log_stderr(errno, "EpollOperateEvent() failed in CSocket::ServerSendQueueThread");
                    }
                    continue;
                } else {
                    memory->FreeMemeory(conn_ptr->allocated_packet_send_mem_ptr);
                    conn_ptr->allocated_packet_send_mem_ptr = nullptr;
                    conn_ptr->throw_send_count = 0;
                }
            }

            err = pthread_mutex_unlock(&socket_ptr->send_msg_queue_mutex);
            if (err != 0) {
                log_stderr(err, "pthread_mutex_unlock(&socket_ptr->send_msg_queue_mutex) failed in failed in CSocket::ServerSendQueueThread, err=%d", err);
            }
        }
    }

    return (void *)0;
}