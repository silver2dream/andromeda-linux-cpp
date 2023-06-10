
#include "andro_logic.h"

#include <arpa/inet.h>
#include <errno.h>  //errno
#include <fcntl.h>  //open
#include <pthread.h>
#include <stdarg.h>  //va_start....
#include <stdint.h>  //uintptr_t
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>  //ioctl
#include <sys/time.h>   //gettimeofday
#include <time.h>       //localtime_r
#include <unistd.h>     //STDERR_FILENO等

#include "andro_crc32.h"
#include "andro_func.h"
#include "andro_global.h"
#include "andro_macro.h"
#include "andro_memory.h"
#include "andro_tmp_proto.h"

typedef bool (CLogic::*handler)(lp_connection_t conn_ptr, lp_message_header_t msg_header, char* pkg_body, unsigned short pkg_body_len);

static const handler status_handler[] = {
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,

    &CLogic::HandleRegister,
    &CLogic::HandleLogin,
};
#define ANDRO_AUTH_TOTAL_COMMANDS sizeof(status_handler) / sizeof(handler)

CLogic::CLogic() {
}

CLogic::~CLogic() {
}

bool CLogic::Init() {
    bool base = CSocket::Init();
    return base;
}

void CLogic::ThreadRecvProcFunc(char* msg_bffer) {
    lp_message_header_t msg_header_ptr = (lp_message_header_t)(msg_bffer);
    lp_packet_header_t pkg_header_ptr = (lp_packet_header_t)(msg_bffer + ANDRO_MSG_HEADER_LEN);
    void* pkg_body = nullptr;
    unsigned short pkg_len = ntohs(pkg_header_ptr->pkg_len);

    if (ANDRO_PKG_HEADER_LEN == pkg_len) {
        if (pkg_header_ptr->crc32 != 0) {
            return;
        }
        pkg_body = nullptr;
    } else {
        pkg_header_ptr->crc32 = ntohl(pkg_header_ptr->crc32);
        pkg_body = (void*)(msg_bffer + ANDRO_MSG_HEADER_LEN + ANDRO_PKG_HEADER_LEN);

        int calc_crc = CCRC32::GetInstance()->GetCRC((unsigned char*)pkg_body, pkg_len - ANDRO_PKG_HEADER_LEN);
        if (calc_crc != pkg_header_ptr->crc32) {
            log_stderr(0, "CRC failed in CLogic::ThreadRecvProcFunc(), discarding packet");
            return;
        }
    }

    unsigned short msg_code = ntohs(pkg_header_ptr->msg_code);
    lp_connection_t conn_ptr = msg_header_ptr->conn_ptr;

    // The connection in this pool has been occupied by other TCP connections [other sockets],
    // which indicates that the original connection between the client and this server has been disconnected.
    // In such case, this packet should be directly discarded without processing.
    if (conn_ptr->sequence != msg_header_ptr->sequence) {
        return;
    }

    if (msg_code >= ANDRO_AUTH_TOTAL_COMMANDS) {
        log_stderr(0, "msg_code[%d] invalid in CLogic::ThreadRecvProcFunc()", msg_code);
        return;
    }

    if (status_handler[msg_code] == nullptr) {
        log_stderr(0, "msg_code[%d] can't find handle function in CLogic::ThreadRecvProcFunc()", msg_code);
        return;
    }

    (this->*status_handler[msg_code])(conn_ptr, msg_header_ptr, (char*)pkg_body, pkg_len - ANDRO_PKG_HEADER_LEN);
    return;
}

bool CLogic::HandleRegister(lp_connection_t conn_ptr, lp_message_header_t msg_header, char* pkg_body, unsigned short pkg_body_len) {
    LPSTRUCT_REGISTER reg = (LPSTRUCT_REGISTER)pkg_body;
    log_stderr(0, "CLogic::HandleRegister,username=%s,pwd=%s", reg->username, reg->password);
    return true;
}

bool CLogic::HandleLogin(lp_connection_t conn_ptr, lp_message_header_t msg_header, char* pkg_body, unsigned short pkg_body_len) {
    LPSTRUCT_LOGIN login = (LPSTRUCT_LOGIN)pkg_body;
    log_stderr(0, "CLogic::HandleLogin,username=%s,pwd=%s", login->username, login->password);
    return true;
}