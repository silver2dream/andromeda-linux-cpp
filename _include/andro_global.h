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

extern char **G_OS_ARGV;
extern char *G_ENVMEM;
extern int G_ENVIRON_LEN;

extern pid_t andro_pid;
extern andro_log_t andro_log;

#endif