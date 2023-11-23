#ifndef __ADNROD_GBLDEF_H__
#define __ADNROD_GBLDEF_H__

#include <csignal>

#include "andro_logic.h"
#include "andro_socket.h"
#include "andro_threadpool.h"

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
extern int G_DAEMONIZED;
extern CLogic G_SOCKET;
extern CThreadPool G_THREAD_POOL;
extern int G_STOP_EVENT;

extern pid_t andro_pid;
extern pid_t andro_ppid;
extern int process_type;
extern sig_atomic_t andro_reap;

extern andro_log_t andro_log;

#endif