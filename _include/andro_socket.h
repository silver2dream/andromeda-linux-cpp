#ifndef __ADNROD_SOCKET_H__
#define __ADNROD_SOCKET_H__

#include <sys/epoll.h>
#include <sys/socket.h>

#include <vector>

#include "andro_packet.h"

#define ANDRO_LISTEN_BACKLOG 511
#define ANDRO_MAX_EPOLL_WAIT_EVENTS 512

#define ANDRO_MSG_HEADER_LEN sizeof(andro_message_header_s)

typedef struct andro_message_header_s message_header_t, *lp_message_header_t;
typedef struct andro_listening_s listening_t, *lp_listening_t;
typedef struct andro_connection_s connection_t, *lp_connection_t;
typedef class CSocket CSocket;

typedef void (CSocket::*event_handler)(lp_connection_t c);

struct andro_message_header_s {
    lp_connection_t conn_ptr;
    uint64_t sequence;  // When a data packet is received, the corresponding connection number is recorded.
                        // This can be used in the future to determine whether the connection has been invalidated.
};

struct andro_listening_s {
    int port;
    int fd;
    lp_connection_t connection;  // This is a connection from the connection pool.
};

struct andro_connection_s {
    int fd;
    lp_listening_t listening;

    unsigned instance : 1;  // 0: valid, 1: invalid.
    uint64_t sequence;
    struct sockaddr socket_addr;

    uint8_t w_ready;

    event_handler rhandler;  // This handler for reading.
    event_handler whandler;  // This handler for writing.

    unsigned char packet_stat;                    // The current status of the packet.
    char packet_head_info[ANDRO_DATA_BUFF_SIZE];  // Used to store the header information of the packet.
    char* packet_recv_buf_ptr;                    // The head pointer of the receive buffer, which is particularly useful for handling partially received packets.
    unsigned int packet_recv_len;                 // The amount of data to be received is specified by this variable and is used in conjunction with `packet_recv_buf_ptr`.

    bool is_memory_allocated_for_packet;  // If we successfully receive the packet header,
                                          // we need to allocate memory to store the packet header, message header, and packet body content.
                                          // This flag is used to indicate whether we have allocated memory using `new` because memory allocated with `new` needs to be released.

    char* allocated_packet_mem_ptr;  // The memory address allocated for receiving packets using `new`, used in conjunction with the `is_memory_allocated_for_packet`.

    lp_connection_t next_conn;  // This is a pointer and assign to element of singal way linked-list

    void DataOffset(ssize_t len) {
        packet_recv_buf_ptr = packet_recv_buf_ptr + len;
        packet_recv_len = packet_recv_len - len;
    }

    void ResetRecv() {
        packet_stat = ANDRO_PKG_STAT_HEAD_INIT;
        packet_recv_buf_ptr = packet_head_info;
        packet_recv_len = ANDRO_PKG_HEADER_LEN;
    }
};

class CSocket {
   public:
    CSocket();
    virtual ~CSocket();
    virtual bool Init();

   public:
    virtual void ThreadRecvProcFunc(char* msg_buffer);

   public:
    int EpollInit();
    int EpollAddEvent(int fd, int read_event, int write_event, uint32_t other_flag, uint32_t event_type, lp_connection_t conn_ptr);
    int EpollProcessEvents(int timer);

   private:
    void read_conf();
    bool open_listening_sockets();
    void close_listening_sockets();
    bool set_nonblocking(int socket_fd);

    void event_accept(lp_connection_t old_conn_ptr);      // Build for new connection.
    void wait_requset_handler(lp_connection_t conn_ptr);  // Process data when it comes.
    void close_connection(lp_connection_t conn_ptr);

    ssize_t recv_proc(lp_connection_t conn_ptr, char* buffer, ssize_t buf_len);
    void proc_header_handler(lp_connection_t conn_ptr);
    void proc_data_handler(lp_connection_t conn_ptr);

    size_t sock_ntop(struct sockaddr* sock_addr, int port, u_char* text, size_t len);

    lp_connection_t get_connection(int socket_fd);
    void free_connection(lp_connection_t conn_ptr);

   private:
    int worker_max_connections;  // The epoll max connections.
    int listen_port_count;       // The number of listening ports.
    int epoll_handler;           // The handle returned by epoll_create().

    lp_connection_t connections_ptr;
    lp_connection_t free_connections_ptr;

    int connection_num;       // The connection pool size.
    int free_connection_num;  // The number for free connection in connection pool.
    std::vector<lp_listening_t> listen_socket_list;
    struct epoll_event epoll_events[ANDRO_MAX_EPOLL_WAIT_EVENTS];
};

#endif