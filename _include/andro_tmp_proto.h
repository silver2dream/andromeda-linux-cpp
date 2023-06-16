#ifndef __ANDRO_TMP_PROTO_H__
#define __ANDRO_TMP_PROTO_H__

#define CMD_START 0
#define CMD_REGISTER CMD_START + 5
#define CMD_LOGIN CMD_START + 6

#pragma pack(1)
typedef struct _STRUCT_REGISTER {
    int iType;
    char username[56];
    char password[40];

} STRUCT_REGISTER, *LPSTRUCT_REGISTER;

typedef struct _STRUCT_LOGIN {
    char username[56];
    char password[40];

} STRUCT_LOGIN, *LPSTRUCT_LOGIN;
#pragma pack()
#endif