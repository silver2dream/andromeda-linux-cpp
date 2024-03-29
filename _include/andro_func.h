#ifndef __ANDRO_FUNC_H__
#define __ANDRO_FUNC_H__

#include <sys/types.h>
#include <cstdarg>

void Rtrim(char *string);
void Ltrim(char *string);

// Related to process initialization.
void init_proctitle();
void set_proctitle(const char *title);

// Related to log.
void log_init();
void log_stderr(int err, const char *fmt, ...);
void log_error_core(int level, int err, const char *fmt, ...);

u_char *log_errno(u_char *buffer, u_char *last, int err);
u_char *snprintf(u_char *buffer, size_t max, const char *fmt, ...);
u_char *slprintf(u_char *buffer, u_char *last, const char *fmt, ...);
u_char *vslprintf(u_char *buffer, u_char *last, const char *fmt, va_list args);

// Related to signal and master process.
int init_signals();
[[noreturn]] void master_process_cycle();
int daemon();
void process_events_and_timers();

#endif