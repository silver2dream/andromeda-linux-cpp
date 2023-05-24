#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "andro_conf.h"
#include "andro_func.h"
#include "andro_macro.h"

static void start_worker_processes(int num);
static int spawn_process(int num, const char *processname);
static void worker_process_init(int num);
static void worker_process_cycle(int num, const char *processname);

void master_process_cycle() {
    sigset_t set;
    sigemptyset(&set);

    sigaddset(&set, SIGCHLD);
    sigaddset(&set, SIGALRM);
    sigaddset(&set, SIGIO);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGHUP);
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGUSR2);
    sigaddset(&set, SIGWINCH);
    sigaddset(&set, SIGTERM);
    sigaddset(&set, SIGQUIT);

    if (sigprocmask(SIG_BLOCK, &set, NULL) == -1) {
        log_error_core(ANDRO_LOG_ALERT, errno, "sigprocmask() occur fail in master_process_cycle()!");
    }

    size_t size;
    int i;

    CConfig *config = CConfig::GetInstance();
    auto master_process = config->GetString(ANDRO_CONF_MASTER_PROCESSES_NAME);
    size = sizeof(master_process);
    size += G_ARGV_NEED_MEM;
    if (size < ANDRO_MAX_TITLE_STR) {
        char title[ANDRO_MAX_TITLE_STR] = {0};
        strcpy(title, (const char *)master_process);
        strcat(title, " ");

        for (i = 0; i < G_OS_ARGC; i++) {
            strcat(title, G_OS_ARGV[i]);
        }
        set_proctitle(title);
    }

    // loaded num of sub-process from conf.

    int workprocess = config->GetIntDefault(ANDRO_CONF_WORK_PROCESSES, 1);
    start_worker_processes(workprocess);

    sigemptyset(&set);
    for (;;) {
        log_error_core(0, 0, "this is master process, pid=%P", andro_pid);
        sleep(1);
    }
    // log_error_core(0, 0, "this is master process, pid=%P", andro_pid);
    return;
}

static void start_worker_processes(int num) {
    int i;
    CConfig *config = CConfig::GetInstance();
    const char *workerTitle = config->GetString(ANDRO_CONF_WORK_PROCESSES_NAME);
    for (i = 0; i < num; i++) {
        spawn_process(i, workerTitle);
    }
    return;
}

static int spawn_process(int num, const char *processname) {
    pid_t pid;
    pid = fork();
    switch (pid) {
        case -1:
            log_error_core(ANDRO_LOG_ALERT, errno, "spawn_process() fork() num=%d, title=\"%s\" fail!", num, processname);
            return -1;
        case 0:
            andro_ppid = andro_pid;
            andro_pid = getpid();
            worker_process_cycle(num, processname);
            break;
        default:
            break;
    }

    return pid;
}

static void worker_process_cycle(int num, const char *processname) {
    worker_process_init(num);
    set_proctitle(processname);

    for (;;) {
        log_error_core(0, 0, "good--this is num=%d,pid=%P!", num, andro_pid);
        sleep(1);
    }
    // log_error_core(0, 0, "good--this is num=%d,pid=%P！", num, andro_pid);
    return;
}

static void worker_process_init(int num) {
    sigset_t set;
    sigemptyset(&set);
    if (sigprocmask(SIG_SETMASK, &set, NULL) == -1) {
        log_error_core(ANDRO_LOG_ALERT, errno, "sigprocmask() occur faill in worker_process_init()!");
    }

    return;
}