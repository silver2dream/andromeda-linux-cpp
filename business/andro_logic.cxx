#include "andro_logic.h"

#include <arpa/inet.h>
#include <cstring>

#include "andro_cmd.h"
#include "andro_crc32.h"
#include "andro_func.h"
#include "andro_lockmutex.h"
#include "andro_memory.h"
#include "auth.pb.h"

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

CLogic::CLogic() = default;

CLogic::~CLogic() = default;

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

    // The connection in this pool has been occupied by other TCP connection_pool [other sockets],
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
    if (pkg_body == nullptr) {
        return false;
    }

    auth::C2SRegister req;

    CLock lock(&conn_ptr->logic_proc_mutex);

    req.ParseFromArray(pkg_body, pkg_body_len);
    log_stderr(0, "usesrname:%s", req.username().c_str());
    log_stderr(0, "password:%s", req.password().c_str());

    CCRC32* crc32 = CCRC32::GetInstance();

    auth::S2CRegister resp;
    resp.set_username(req.username());
    resp.set_password(req.password());
    size_t send_len = resp.ByteSizeLong();

    char* send_buffer = (char*)CMemory::AllocMemory(ANDRO_MSG_HEADER_LEN + ANDRO_PKG_HEADER_LEN + send_len, false);
    memcpy(send_buffer, msg_header, ANDRO_MSG_HEADER_LEN);
    lp_packet_header_t pkg_header_ptr;
    pkg_header_ptr = (lp_packet_header_t)(send_buffer + ANDRO_MSG_HEADER_LEN);
    pkg_header_ptr->msg_code = ANDRO_BUSINESS_CMD_REGISTER;
    pkg_header_ptr->msg_code = htons(pkg_header_ptr->msg_code);
    pkg_header_ptr->pkg_len = htons(ANDRO_PKG_HEADER_LEN + send_len);

    char* send_info = (send_buffer + ANDRO_MSG_HEADER_LEN + ANDRO_PKG_HEADER_LEN);

    resp.SerializeToArray(send_info, send_len);

    pkg_header_ptr->crc32 = crc32->GetCRC((unsigned char*)send_info, send_len);
    pkg_header_ptr->crc32 = htonl(pkg_header_ptr->crc32);

    msg_send(send_buffer);
    return true;
}

bool CLogic::HandleLogin(lp_connection_t conn_ptr, lp_message_header_t msg_header, char* pkg_body, unsigned short pkg_body_len) {
    return true;
}