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
#include "andro_memory.h"
#include "andro_socket.h"

lp_connection_t CSocket::get_connection(int socket_fd) {
    lp_connection_t conn_ptr = free_connections_ptr;
    if (conn_ptr == nullptr) {
        log_stderr(0, "free connections list was empty that is error in CSocket::get_connection");
        return nullptr;
    }

    free_connections_ptr = conn_ptr->next_conn;
    free_connection_num--;

    uintptr_t instance = conn_ptr->instance;
    uint64_t sequence = conn_ptr->sequence;

    memset(conn_ptr, 0, sizeof(connection_t));
    conn_ptr->fd = socket_fd;
    conn_ptr->ResetRecv();
    conn_ptr->is_memory_allocated_for_packet = false;
    conn_ptr->allocated_packet_mem_ptr = nullptr;

    conn_ptr->instance = !instance;
    conn_ptr->sequence = sequence;
    ++conn_ptr->sequence;

    return conn_ptr;
}

void CSocket::free_connection(lp_connection_t conn_ptr) {
    if (conn_ptr->is_memory_allocated_for_packet == true) {
        CMemory::GetInstance()->FreeMemeory(conn_ptr->allocated_packet_mem_ptr);
        conn_ptr->allocated_packet_mem_ptr = nullptr;
        conn_ptr->is_memory_allocated_for_packet = false;
    }

    conn_ptr->next_conn = free_connections_ptr;
    ++conn_ptr->sequence;

    free_connections_ptr = conn_ptr;
    ++free_connection_num;
    return;
}

void CSocket::close_connection(lp_connection_t conn_ptr) {
    if (close(conn_ptr->fd) == -1) {
        log_error_core(ANDRO_LOG_ALERT, errno, "close(%d) failed in CSocket::close_connection", conn_ptr->fd);
    }
    conn_ptr->fd = -1;
    free_connection(conn_ptr);
    return;
}