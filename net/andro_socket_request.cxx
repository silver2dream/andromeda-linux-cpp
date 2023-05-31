#include <arpa/inet.h>
#include <errno.h>   //errno
#include <fcntl.h>   //open
#include <stdarg.h>  //va_start....
#include <stdint.h>  //uintptr_t
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>  //ioctl
#include <sys/time.h>   //gettimeofday
#include <time.h>       //localtime_r
#include <unistd.h>

#include "andro_socket.h"

void CSocket::wait_requset_handler(lp_connection_t conn_ptr) {
    return;
}