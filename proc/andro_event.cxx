#include "andro_conf.h"
#include "andro_func.h"

void process_events_and_timers() {
  G_SOCKET.EpollProcessEvents(-1);

  G_SOCKET.PrintThreadInfo();
}