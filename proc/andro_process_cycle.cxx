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
static int spawn_process(int num, const char *process_name);
static void worker_process_init(int num);
static void worker_process_cycle(int num, const char *process_name);

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
        log_error_core(ANDRO_LOG_NOTICE, 0, "this is %s, pid=%P", title, andro_pid);
    }

    int workprocess = config->GetIntDefault(ANDRO_CONF_WORK_PROCESSES, 1);
    start_worker_processes(workprocess);

    sigemptyset(&set);
    for (;;) {
        // log_error_core(0, 0, "this is master process, pid=%P", andro_pid);
        sigsuspend(&set);
        sleep(1);
    }

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

static int spawn_process(int num, const char *process_name) {
    pid_t pid;
    pid = fork();
    switch (pid) {
        case -1:
            log_error_core(ANDRO_LOG_ALERT, errno, "spawn_process() fork() num=%d, title=\"%s\" fail!", num, process_name);
            return -1;
        case 0:
            andro_ppid = andro_pid;
            andro_pid = getpid();
            worker_process_cycle(num, process_name);
            break;
        default:
            break;
    }

    return pid;
}

static void worker_process_cycle(int num, const char *process_name) {
    process_type = ANDRO_PROCESS_WORKER;
    worker_process_init(num);
    set_proctitle(process_name);

    log_error_core(ANDRO_LOG_NOTICE, 0, "good--this is %s, pid=%P！", process_name, andro_pid);
    for (;;) {
        // log_error_core(0, 0, "good--this is num=%d,pid=%P!", num, andro_pid);
        process_events_and_timers();
    }

    G_THREAD_POOL.StopAll();
    return;
}

static void worker_process_init(int num) {
    sigset_t set;
    sigemptyset(&set);
    if (sigprocmask(SIG_SETMASK, &set, NULL) == -1) {
        log_error_core(ANDRO_LOG_ALERT, errno, "sigprocmask() occur faill in worker_process_init()!");
    }

    CConfig *config = CConfig::GetInstance();
    int thread_num = config->GetIntDefault(ANDRO_CONF_WORK_THREAD_COUNT, 5);
    if (G_THREAD_POOL.Create(thread_num) == false) {
        exit(-2);
    }
    sleep(1);

    G_SOCKET.EpollInit();

    return;
}