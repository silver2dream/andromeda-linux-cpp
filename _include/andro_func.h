#ifndef __ANDRO_FUNC_H__
#define __ANDRO_FUNC_H__

void Rtrim(char *string);
void Ltrim(char *string);

// Related to process initialization.
void init();
void setproctitle(const char *title);

// Related to log.
void log_init();
void log_stderr(int err, const char *fmt, ...);
void log_error_core(int level, int err, const char *fmt, ...);

u_char *log_errno(u_char *buf, u_char *last, int err);
u_char *slprintf(u_char *buf, u_char *last, const char *fmt, ...);
u_char *vslprintf(u_char *buf, u_char *last, const char *fmt, va_list args);

#endif