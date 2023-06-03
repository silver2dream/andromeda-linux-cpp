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

#include "andro_func.h"
#include "andro_global.h"
#include "andro_lockmutex.h"
#include "andro_macro.h"
#include "andro_memory.h"
#include "andro_socket.h"

void CSocket::wait_requset_handler(lp_connection_t conn_ptr) {
    ssize_t result = recv_proc(conn_ptr, conn_ptr->packet_recv_buf_ptr, conn_ptr->packet_recv_len);
    if (result <= 0) {
        return;
    }

    if (conn_ptr->packet_stat == ANDRO_PKG_STAT_HEAD_INIT) {
        if (result == ANDRO_PKG_HEADER_LEN) {
            proc_header_handler(conn_ptr);
        } else {
            conn_ptr->packet_stat = ANDRO_PKG_STAT_HEAD_RECVING;
            conn_ptr->DataOffset(result);
        }
    } else if (conn_ptr->packet_stat == ANDRO_PKG_STAT_HEAD_RECVING) {
        if (conn_ptr->packet_recv_len == result) {
            proc_header_handler(conn_ptr);
        } else {
            conn_ptr->DataOffset(result);
        }
    } else if (conn_ptr->packet_stat == ANDRO_PKG_STAT_BODY_INIT) {
        if (result == conn_ptr->packet_recv_len) {
            proc_data_handler(conn_ptr);
        } else {
            conn_ptr->packet_stat = ANDRO_PKG_STAT_BODY_RECVING;
            conn_ptr->DataOffset(result);
        }
    } else if (conn_ptr->packet_stat == ANDRO_PKG_STAT_BODY_RECVING) {
        if (result == conn_ptr->packet_recv_len) {
            proc_data_handler(conn_ptr);
        } else {
            conn_ptr->DataOffset(result);
        }
    }

    return;
}

ssize_t CSocket::recv_proc(lp_connection_t conn_ptr, char* buffer, ssize_t buf_len) {
    ssize_t n;

    n = recv(conn_ptr->fd, buffer, buf_len, 0);
    if (n == 0) {
        log_stderr(errno, "the connection is closed through a proper four-way handshake, fd=%d", conn_ptr->fd);
        close_connection(conn_ptr);
        return -1;
    }

    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            log_stderr(errno, "the condition errno == EAGAIN || errno == EWOULDBLOCK is true in the CSocekt::recv_proc()");
            return -1;
        }

        /*
            The occurrence of the `EINTR` error:
            When a process, which is blocked in a slow system call,
            captures a signal and the corresponding signal handler returns,
            the system call may return an `EINTR` error.

            For example, in a socket server, if a signal capture mechanism is set up and there are child processes,
            when the parent process is blocked in a slow system call and captures a valid signal,
            the kernel will cause `accept` to return an `EINTR` error (interrupted system call).
        */
        if (errno == EINTR) {
            log_stderr(errno, "the condition errno == EINTR is true in the CSocekt::recv_proc()");
            return -1;
        }

        if (errno == ECONNRESET) {
            // If the client abruptly closes the entire running program without properly closing the socket connection, it can result in the ECONNRESET error.
            // Do nothing.
        } else {
            log_stderr(errno, "ouccur error in the CSocekt::recv_proc()");
        }

        log_stderr(errno, " the connection is closed by the client in an abnormal manner, fd=%d", conn_ptr->fd);
        close_connection(conn_ptr);
        return -1;
    }

    return n;
}

void CSocket::proc_header_handler(lp_connection_t conn_ptr) {
    CMemory* memory = CMemory::GetInstance();

    lp_packet_header_t pkg_header_ptr;
    pkg_header_ptr = (lp_packet_header_t)(conn_ptr->packet_head_info);

    unsigned short pkg_len;
    pkg_len = ntohs(pkg_header_ptr->pkg_len);

    if (pkg_len < ANDRO_PKG_HEADER_LEN) {
        conn_ptr->ResetRecv();
    } else if (pkg_len > (ANDRO_PKG_MAX_LEN - 1000)) {
        conn_ptr->ResetRecv();
    } else {
        char* tmp_buff_ptr = (char*)memory->AllocMemory(ANDRO_MSG_HEADER_LEN + pkg_len, false);
        conn_ptr->is_memory_allocated_for_packet = true;
        conn_ptr->allocated_packet_mem_ptr = tmp_buff_ptr;

        lp_message_header_t msg_header_ptr = (lp_message_header_t)tmp_buff_ptr;
        msg_header_ptr->conn_ptr = conn_ptr;
        msg_header_ptr->sequence = conn_ptr->sequence;

        tmp_buff_ptr += ANDRO_MSG_HEADER_LEN;
        memcpy(tmp_buff_ptr, pkg_header_ptr, ANDRO_PKG_HEADER_LEN);
        if (pkg_len == ANDRO_PKG_HEADER_LEN) {
            proc_data_handler(conn_ptr);
        } else {
            conn_ptr->packet_stat = ANDRO_PKG_STAT_BODY_INIT;
            conn_ptr->packet_recv_buf_ptr = tmp_buff_ptr + ANDRO_PKG_HEADER_LEN;
            conn_ptr->packet_recv_len = pkg_len - ANDRO_PKG_HEADER_LEN;
        }
    }

    return;
}

void CSocket::proc_data_handler(lp_connection_t conn_ptr) {
    int msg_count = 0;
    push_to_msg_queue(conn_ptr->allocated_packet_mem_ptr, msg_count);

    G_THREAD_POOL.Call(msg_count);

    conn_ptr->is_memory_allocated_for_packet = false;
    conn_ptr->allocated_packet_mem_ptr = nullptr;
    conn_ptr->ResetRecv();
}

void CSocket::push_to_msg_queue(char* buffer, int& msg_count) {
    CLock lock(&msg_queue_mutex);
    msg_queue.push_back(buffer);
    ++msg_queue_size;
    msg_count = msg_queue_size;
}

char* CSocket::PopFromMsgQueue() {
    CLock lock(&msg_queue_mutex);

    if (msg_queue.empty()) {
        return nullptr;
    }

    char* msg_buffer = msg_queue.front();
    msg_queue.pop_front();
    --msg_queue_size;
    return msg_buffer;
}

void CSocket::ThreadRecvProcFunc(char* msg_buffer) {
    return;
}