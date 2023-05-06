#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "andro_conf.h"
#include "andro_func.h"
#include "andro_global.h"
#include "andro_macro.h"

static u_char err_levels[][20] = {
    {"stderr"},
    {"emerg"},
    {"alert"},
    {"crit"},
    {"error"},
    {"warn"},
    {"notice"},
    {"info"},
    {"debug"}};

andro_log_t andro_log;

void log_init() {
    u_char *logName = nullptr;
    size_t nLen;

    auto config = CConfig::GetInstance();
    logName = (u_char *)config->GetString("Log");
    if (logName == nullptr) {
        logName = (u_char *)ANDRO_ERROR_LOG_PATH;
    }

    andro_log.log_level = config->GetIntDefault("LogLevel", ANDRO_LOG_NOTICE);
    andro_log.fd = open((const char *)logName, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (andro_log.fd == -1) {
        log_stderr(errno, "[alert] could not open error log file: open() \"%s\" failed", logName);
        andro_log.fd = STDERR_FILENO;
    }

    return;
}

void log_stderr(int err, const char *fmt, ...) {
    va_list args;
    u_char errstr[ANDRO_MAX_ERROR_STR + 1];
    u_char *p, *last;

    memset(errstr, 0, sizeof(errstr));
    last = errstr + ANDRO_MAX_ERROR_STR;

    p = andro_cpymem(errstr, "andromeda: ", 11);

    va_start(args, fmt);
    p = vslprintf(p, last, fmt, args);
    va_end(args);

    if (err) {
        p = log_errno(p, last, err);
    }

    if (p >= (last - 1)) {
        p = (last - 1) - 1;
    }
    *p++ = '\n';

    write(STDERR_FILENO, errstr, p - errstr);
    return;
}

u_char *log_errno(u_char *buf, u_char *last, int err) {
    char *errorInfo = strerror(err);
    size_t len = strlen(errorInfo);

    char leftstr[10] = {0};
    sprintf(leftstr, " (%d: ", err);
    size_t leftlen = strlen(leftstr);

    char rightstr[] = ") ";
    size_t rightlen = strlen(rightstr);

    size_t extralen = leftlen + rightlen;
    if ((buf + len + extralen) < last) {
        buf = andro_cpymem(buf, leftstr, leftlen);
        buf = andro_cpymem(buf, errorInfo, len);
        buf = andro_cpymem(buf, rightstr, rightlen);
    }
    return buf;
}

void log_error_core(int level, int err, const char *fmt, ...) {
    u_char *last;
    u_char errstr[ANDRO_MAX_ERROR_STR + 1];

    memset(errstr, 0, sizeof(errstr));
    last = errstr + ANDRO_MAX_ERROR_STR;

    struct timeval tv;
    struct tm tm;
    time_t sec;
    u_char *p;
    va_list args;

    memset(&tv, 0, sizeof(timeval));
    memset(&tm, 0, sizeof(tm));

    gettimeofday(&tv, NULL);

    sec = tv.tv_sec;
    localtime_r(&sec, &tm);
    tm.tm_mon++;
    tm.tm_year += 1900;

    u_char strcurrtime[40] = {0};
}