#ifndef __ADNROD_SOCKET_H__
#define __ADNROD_SOCKET_H__

#include <sys/epoll.h>
#include <sys/socket.h>

#include <vector>

#define ANDRO_LISTEN_BACKLOG 511
#define ANDRO_MAX_EPOLL_WAIT_EVENTS 512

typedef struct andro_listening_s listening_t, *lp_listening_t;
typedef struct andro_connection_s connection_t, *lp_connection_t;
typedef class CSocket CSocket;

typedef void (CSocket::*event_handler)(lp_connection_t c);

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

    lp_connection_t next_conn;  // This is a pointer and assign to element of singal way linked-list
};

class CSocket {
   public:
    CSocket();
    virtual ~CSocket();

   public:
    virtual bool Init();

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
    void close_accepted_connection(lp_connection_t conn_ptr);

    size_t sock_ntop(struct sockaddr* sock_addr, int port, u_char* text, size_t len);

    lp_connection_t get_connection(int socket_fd);
    void free_connection(lp_connection_t conn_ptr);

   private:
    int worker_max_connections;  // The epoll max connections.
    int listen_port_count;
    int epoll_handler;

    lp_connection_t connections_ptr;
    lp_connection_t free_connections_ptr;

    int connection_num;       // The connection pool size.
    int free_connection_num;  // The number for free connection in connection pool.

    std::vector<lp_listening_t> listen_socket_list;

    struct epoll_event epoll_events[ANDRO_MAX_EPOLL_WAIT_EVENTS];
};

#endif