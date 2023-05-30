#ifndef __ANDRO_MACRO_H__
#define __ANDRO_MACRO_H__

#define ANDRO_MAX_TITLE_STR 1000

#define ANDRO_MAX_ERROR_STR 2048

#define andro_cpymem(dst, src, n) (((u_char*)memcpy(dst, src, n)) + (n))
#define andro_min(val1, val2) (val1 > val2 ? (val2) : (val1))

#define andro_str_combine(val1, val2) val1 #val2

// The maximum 32-bit unsigned integer.
#define ANDRO_MAX_UINT32_VALUE (uint32_t)0xffffffff

#define ANDRO_INT64_LEN (sizeof("-9223372036854775808") - 1)

// This is the error level of the log, ranging from 0 to 8, with 0 being the highest level error.
#define ANDRO_LOG_STDERR 0
#define ANDRO_LOG_EMERG 1
#define ANDRO_LOG_ALERT 2
#define ANDRO_LOG_CRIT 3
#define ANDRO_LOG_ERR 4
#define ANDRO_LOG_WARN 5
#define ANDRO_LOG_NOTICE 6
#define ANDRO_LOG_INFO 7
#define ANDRO_LOG_DEBUG 8

#define ANDRO_ERROR_LOG_PATH "logs/error.log"

#define ANDRO_PROCESS_MASTER 0
#define ANDRO_PROCESS_WORKER 1

#define ANDRO_PORT_PRIFIX "Port"

#endif