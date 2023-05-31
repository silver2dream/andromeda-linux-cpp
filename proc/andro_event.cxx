#include <errno.h>  //errno
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "andro_conf.h"
#include "andro_func.h"
#include "andro_macro.h"
#include "andro_socket.h"

void process_events_and_timers() {
    G_SOCKET.EpollProcessEvents(-1);
}