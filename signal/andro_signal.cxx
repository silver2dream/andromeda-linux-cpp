#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "andro_func.h"
#include "andro_macro.h"

typedef struct {
    int signo;
    const char *signame;

    void (*handler)(int signo, siginfo_t *siginfo, void *ucontext);
} signal_t;

static void signal_handler(int signo, siginfo_t *siginfo, void *ucontext);

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
            log_stderr(0, "sigaction(%s) succed!", sig->signame);
        }
    }

    return 0;
}

static void signal_handler(int signo, siginfo_t *siginfo, void *ucontext) {
    printf("signal[%d] is coming. \n", signo);
}