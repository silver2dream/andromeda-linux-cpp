#ifndef __ADNROD_GBLDEF_H__
#define __ADNROD_GBLDEF_H__

#include <sys/types.h>

typedef struct {
    char ItemName[50];
    char ItemContent[500];
} CConfItem, *LPCConfItem;

typedef struct {
    int log_level;
    int fd;
} andro_log_t;

extern size_t G_ARGV_NEED_MEM;
extern size_t G_ENV_NEED_MEM;
extern int G_OS_ARGC;
extern char **G_OS_ARGV;
extern char *G_ENVMEM;
extern int G_ENVIRON_LEN;

extern pid_t andro_pid;
extern pid_t andro_ppid;
extern andro_log_t andro_log;

#endif