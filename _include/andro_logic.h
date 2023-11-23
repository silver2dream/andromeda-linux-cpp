#ifndef __ANDRO_LOGIC_H__
#define __ANDRO_LOGIC_H__

#include <sys/socket.h>

#include "andro_socket.h"

class CLogic : public CSocket {
   public:
    CLogic();
    ~CLogic() override;
    bool Init() override;

   public:
    bool HandleRegister(lp_connection_t conn_ptr, lp_message_header_t msg_header, char *pkg_body, unsigned short pkg_body_len);
    bool HandleLogin(lp_connection_t conn_ptr, lp_message_header_t msg_header, char *pkg_body, unsigned short pkg_body_len);

   public:
    void ThreadRecvProcFunc(char *msg_buffer) override;
};

#endif