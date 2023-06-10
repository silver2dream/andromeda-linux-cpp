#ifndef __ANDRO_LOGIC_H__
#define __ANDRO_LOGIC_H__

#include <sys/socket.h>

#include "andro_socket.h"

class CLogic : public CSocket {
   public:
    CLogic();
    virtual ~CLogic();
    virtual bool Init();

   public:
    bool HandleRegister(lp_connection_t conn_ptr, lp_message_header_t msg_header, char *pkg_body, unsigned short pkg_body_len);
    bool HandleLogin(lp_connection_t conn_ptr, lp_message_header_t msg_header, char *pkg_body, unsigned short pkg_body_len);

   public:
    virtual void ThreadRecvProcFunc(char *msg_buffer);
};

#endif