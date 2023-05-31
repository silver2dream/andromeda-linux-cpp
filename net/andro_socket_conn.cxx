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
#include "andro_socket.h"

lp_connection_t CSocket::get_connection(int socket_fd) {
    lp_connection_t conn = free_connections_ptr;
    if (conn == nullptr) {
        log_stderr(0, "free connections list was empty that is error in CSocket::get_connection");
        return nullptr;
    }

    free_connections_ptr = conn->next_conn;
    free_connection_num--;

    uintptr_t instance = conn->instance;
    uint64_t sequence = conn->sequence;

    memset(conn, 0, sizeof(connection_t));
    conn->fd = socket_fd;
    conn->instance = !instance;
    conn->sequence = sequence;
    ++conn->sequence;

    return conn;
}

void CSocket::free_connection(lp_connection_t conn_ptr) {
    conn_ptr->next_conn = free_connections_ptr;
    ++conn_ptr->sequence;

    free_connections_ptr = conn_ptr;
    ++free_connection_num;
    return;
}