#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "andro_conf.h"
#include "andro_func.h"
#include "andro_global.h"
#include "andro_macro.h"
#include "andro_memory.h"
#include "andro_socket.h"

static void free_resource();

size_t G_ARGV_NEED_MEM = 0;
size_t G_ENV_NEED_MEM = 0;
int G_OS_ARGC = 0;
char **G_OS_ARGV;
char *G_ENVMEM = nullptr;
int G_ENVIRON_LEN = 0;
int G_DAEMONIZED = 0;

pid_t andro_pid;
pid_t andro_ppid;
int process_type;

sig_atomic_t andro_reap;
CSocket G_SOCKET;
CThreadPool G_THREAD_POOL;

int main(int argc, char *const *argv) {
    int exitcode = 0;

    andro_pid = getpid();
    andro_ppid = getppid();
    G_ARGV_NEED_MEM = 0;
    for (int i = 0; i < argc; i++) {
        G_ARGV_NEED_MEM += strlen(argv[i]) + 1;
    }

    for (int i = 0; environ[i]; i++) {
        G_ENV_NEED_MEM += strlen(environ[i]) + 1;
    }

    G_OS_ARGC = argc;
    G_OS_ARGV = (char **)argv;

    andro_log.fd = -1;
    process_type = ANDRO_PROCESS_MASTER;
    andro_reap = 0;

    CConfig *config = CConfig::GetInstance();
    if (config->Load(ANDRO_CONF_FILE) == false) {
        log_init();
        log_stderr(0, "can't load config[%s] doc", ANDRO_CONF_FILE);
        exitcode = 2;
        goto lblexit;
    }

    CMemory::GetInstance();

    log_init();

    if (init_signals() != 0) {
        exitcode = 1;
        goto lblexit;
    }

    if (G_SOCKET.Init() == false) {
        exitcode = 1;
        goto lblexit;
    }

    init_proctitle();

    if (config->GetIntDefault(ANDRO_CONF_DAEMON_MODE, 0) == 1) {
        int daemon_reuslt = daemon();
        if (daemon_reuslt == -1) {  // Fork failed.
            exitcode = 1;
            goto lblexit;
        }

        if (daemon_reuslt == 1) {  // Original parenet process.
            free_resource();
            exitcode = 0;
            return exitcode;
        }

        G_DAEMONIZED = 1;
    }

    master_process_cycle();

lblexit:
    log_stderr(0, "process exit!, pid=%P", getpid());
    free_resource();
    return exitcode;
}

void free_resource() {
    if (G_ENVMEM) {
        delete[] G_ENVMEM;
        G_ENVMEM = nullptr;
    }

    if (andro_log.fd != STDERR_FILENO && andro_log.fd != -1) {
        close(andro_log.fd);
        andro_log.fd = -1;
    }
}