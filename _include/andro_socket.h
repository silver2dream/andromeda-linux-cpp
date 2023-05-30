#ifndef __ADNROD_SOCKET_H__
#define __ADNROD_SOCKET_H__

#include <vector>

#define ANDRO_LISTEN_BACKLOG 511

typedef struct andro_listening_s {
    int port;
    int fd;
} listening_t, *lp_listening_t;

class CSocket {
   public:
    CSocket();
    virtual ~CSocket();

   public:
    virtual bool Init();

   private:
    bool open_listening_sockets();
    void close_listening_sockets();
    bool set_nonblocking(int socketfd);

   private:
    int listen_port_count;
    std::vector<lp_listening_t> listen_socket_list;
};

#endif