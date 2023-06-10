#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "andro_func.h"
#include "andro_global.h"
#include "andro_macro.h"

typedef struct {
    int signo;
    const char *signame;

    void (*handler)(int signo, siginfo_t *siginfo, void *ucontext);
} signal_t;

static void signal_handler(int signo, siginfo_t *siginfo, void *ucontext);
static void process_get_status(void);

signal_t signals[] = {
    {SIGHUP, "SIGHUP", signal_handler},
    {SIGINT, "SIGINT", signal_handler},
    {SIGTERM, "SIGTERM", signal_handler},
    {SIGCHLD, "SIGCHLD", signal_handler},
    {SIGQUIT, "SIGQUIT", signal_handler},
    {SIGIO, "SIGIO", signal_handler},
    {SIGSYS, "SIGSYS, SIG_IGN", NULL},

    {0, NULL, NULL}

};

int init_signals() {
    signal_t *sig;
    struct sigaction sa;

    for (sig = signals; sig->signo != 0; sig++) {
        memset(&sa, 0, sizeof(struct sigaction));

        if (sig->handler != nullptr) {
            sa.sa_sigaction = sig->handler;
            sa.sa_flags = SA_SIGINFO;
        } else {
            sa.sa_handler = SIG_IGN;
        }

        sigemptyset(&sa.sa_mask);

        if (sigaction(sig->signo, &sa, NULL) == -1) {
            log_error_core(ANDRO_LOG_EMERG, errno, "sigaction(%s) failed", sig->signame);
            return -1;
        } else {
            // log_stderr(0, "sigaction(%s) succed!", sig->signame);
        }
    }

    return 0;
}

static void signal_handler(int signo, siginfo_t *siginfo, void *ucontext) {
    printf("signal[%d] is coming. \n", signo);
    signal_t *sig;
    char *action;

    for (sig = signals; sig->signo != 0; sig++) {
        if (sig->signo == signo) {
            break;
        }
    }

    action = (char *)"";

    if (process_type == ANDRO_PROCESS_MASTER) {
        switch (signo) {
            case SIGCHLD:
                andro_reap = 1;
                break;
            default:
                break;
        }
    } else if (process_type == ANDRO_PROCESS_WORKER) {
        // ToDo
    } else {
        // ToDo
    }

    if (siginfo && siginfo->si_pid) {
        log_error_core(ANDRO_LOG_NOTICE, 0, "signal %d (%s) received from %P%s", signo, sig->signame, siginfo->si_pid, action);
    } else {
        log_error_core(ANDRO_LOG_NOTICE, 0, "signal %d (%s) received %s", signo, sig->signame, action);
    }

    if (signo == SIGCHLD) {
        process_get_status();
    }

    return;
}

static void process_get_status(void) {
    pid_t pid;
    int status;
    int err;
    int one = 0;

    for (;;) {
        pid = waitpid(-1, &status, WNOHANG);

        if (pid == 0) {
            return;
        }
        if (pid == -1) {
            err = errno;
            if (err == EINTR) {
                continue;
            }

            if (err == ECHILD && one) {
                return;
            }

            if (err == ECHILD) {
                log_error_core(ANDRO_LOG_INFO, err, "waitpid() failed!");
                return;
            }

            log_error_core(ANDRO_LOG_ALERT, err, "waitpid() failed!");
            return;
        }

        one = 1;
        if (WTERMSIG(status)) {
            log_error_core(ANDRO_LOG_ALERT, 0, "pid = %P exited on signal %d!", pid, WTERMSIG(status));
        } else {
            log_error_core(ANDRO_LOG_NOTICE, 0, "pid = %P exited with code %d!", pid, WEXITSTATUS(status));
        }
    }
    return;
}