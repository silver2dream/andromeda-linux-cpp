#ifndef _ANDRO_PACKET_H
#define _ANDRO_PACKET_H

#define ANDRO_PKG_MAX_LEN 30000

#define ANDRO_PKG_STAT_HEAD_INIT 0     // Ready for receviing head packet.
#define ANDRO_PKG_STAT_HEAD_RECVING 1  // Receiving head packet and keep receiving.
#define ANDRO_PKG_STAT_BODY_INIT 2     // Ready for receviing body packet.
#define ANDRO_PKG_STAT_BODY_RECVING 3  // Receiving body packet and keep receiving.

#define ANDRO_DATA_BUFF_SIZE 20
#define ANDRO_PKG_HEADER_LEN sizeof(andro_packet_header_s)

typedef struct andro_packet_header_s packet_header_t, *lp_packet_header_t;

#pragma pack(1)  // Alignment method: 1-byte alignment.
                 // This means that there is no byte alignment between members of the structure: they are tightly packed together.
struct andro_packet_header_s {
    unsigned short pkg_len;   // Total message length [header + body] -- 2 bytes. The maximum number that can be represented by 2 bytes is over 60,000.
                              // We define ANDRO_PKG_MAX_LEN as 30,000, so pkg_Len is enough to store the entire package length recorded in the header [header + body].
    unsigned short msg_code;  // Message type code -- 2 bytes. This is used to distinguish between different commands [different messages].
    int crc32;                // CRC32 check -- 4 bytes. This is introduced as a basic verification to prevent situations where the received content and the sent content are not consistent.
};
#pragma pack()

#endif