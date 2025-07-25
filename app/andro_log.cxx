#include <cerrno>
#include <fcntl.h>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <sys/time.h>
#include <ctime>
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
    u_char *log_name = nullptr;

    auto config = CConfig::GetInstance();
    const char* log_name_str = config->GetString("Log");
    if (log_name_str == nullptr) {
        log_name_str = ANDRO_ERROR_LOG_PATH;
    }

    andro_log.log_level = config->GetIntDefault("LogLevel", ANDRO_LOG_NOTICE);
    andro_log.fd = open(log_name_str, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (andro_log.fd == -1) {
        log_stderr(errno, "[alert] could not open error log file: open() \"%s\" failed", log_name_str);
        andro_log.fd = STDERR_FILENO;
    }

}

void log_stderr(int err, const char *fmt, ...) {
    va_list args;
    u_char errstr[ANDRO_MAX_ERROR_STR + 1];
    u_char *p, *last;

    // Use explicit_bzero if available, otherwise memset
    memset(errstr, 0, sizeof(errstr));
    last = errstr + ANDRO_MAX_ERROR_STR;

    p = Andro_Cpy_Mem(errstr, "andromeda: ", 11);

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
    
    // Securely clear the buffer after use
    memset(errstr, 0, sizeof(errstr));
}

u_char *log_errno(u_char *buffer, u_char *last, int err) {
    char *errorInfo = strerror(err);
    if (!errorInfo) {
        return buffer; // Safety check
    }
    
    // Use strnlen for safety
    size_t len = strnlen(errorInfo, ANDRO_MAX_ERROR_STR);

    char leftstr[10] = {0};
    snprintf(leftstr, sizeof(leftstr), " (%d: ", err);
    size_t leftlen = strnlen(leftstr, sizeof(leftstr) - 1);

    char rightstr[] = ") ";
    size_t rightlen = sizeof(rightstr) - 1; // Compile-time constant

    size_t extralen = leftlen + rightlen;
    if ((buffer + len + extralen) < last) {
        buffer = Andro_Cpy_Mem(buffer, leftstr, leftlen);
        buffer = Andro_Cpy_Mem(buffer, errorInfo, len);
        buffer = Andro_Cpy_Mem(buffer, rightstr, rightlen);
    }
    return buffer;
}

void log_error_core(int level, int err, const char *fmt, ...) {
    u_char *last;
    u_char errstr[ANDRO_MAX_ERROR_STR + 1];

    // Use explicit_bzero if available, otherwise memset
    memset(errstr, 0, sizeof(errstr));
    last = errstr + ANDRO_MAX_ERROR_STR;

    struct timeval tv{};
    struct tm tm{};
    time_t sec;
    u_char *p;
    va_list args;

    // These are not sensitive data, but following best practices
    memset(&tv, 0, sizeof(timeval));
    memset(&tm, 0, sizeof(tm));

    gettimeofday(&tv, NULL);

    sec = tv.tv_sec;
    localtime_r(&sec, &tm);
    tm.tm_mon++;
    tm.tm_year += 1900;

    u_char strcurrtime[40] = {0};
    slprintf(strcurrtime, (u_char *)-1,
             "%4d/%02d/%02d %02d:%02d:%02d",
             tm.tm_year, tm.tm_mon,
             tm.tm_mday, tm.tm_hour,
             tm.tm_min, tm.tm_sec);

    // Use strnlen without C-style cast - strcurrtime is already u_char*
    size_t time_len = strnlen(reinterpret_cast<const char*>(strcurrtime), sizeof(strcurrtime) - 1);
    p = Andro_Cpy_Mem(errstr, strcurrtime, time_len);
    p = slprintf(p, last, " [%s] ", err_levels[level]);
    p = slprintf(p, last, "%P: ", andro_pid);

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

    while (true) {
        if (level > andro_log.log_level) {
            break;
        }

        ssize_t n = write(andro_log.fd, errstr, p - errstr);
        if (n == -1) {
            if (errno == ENOSPC) {
                // maybe no space.
            } else {
                if (andro_log.fd != STDERR_FILENO) {
                    // Write to stderr as fallback, but don't need to store return value
                    (void)write(STDERR_FILENO, errstr, p - errstr);
                }
            }
        }
        break;
    }

    // Securely clear the buffer after use
    memset(errstr, 0, sizeof(errstr));
}