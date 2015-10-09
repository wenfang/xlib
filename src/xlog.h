#ifndef __XLOG_H 
#define __XLOG_H

#include <syslog.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>

#define XLOG_EMERG(fmt, arg...)   xlog_write(LOG_EMERG|LOG_LOCAL6, "[%s:%d][%d][%x][EMERG]: "#fmt, __FILE__, __LINE__, getpid(), pthread_self(), ##arg)
#define XLOG_ALERT(fmt, arg...)   xlog_write(LOG_ALERT|LOG_LOCAL6, "[%s:%d][%d][%x][ALERT]: "#fmt, __FILE__, __LINE__, getpid(), pthread_self(), ##arg)
#define XLOG_CRIT(fmt, arg...)    xlog_write(LOG_CRIT|LOG_LOCAL6, "[%s:%d][%d][%x][CRIT]: "#fmt, __FILE__, __LINE__, getpid(), pthread_self(), ##arg)
#define XLOG_ERR(fmt, arg...)     xlog_write(LOG_ERR|LOG_LOCAL6, "[%s:%d][%d][%x][ERROR]: "#fmt, __FILE__, __LINE__, getpid(), pthread_self(), ##arg)
#define XLOG_WARNING(fmt, arg...) xlog_write(LOG_WARNING|LOG_LOCAL6, "[%s:%d][%d][%x][WARNING]: "#fmt, __FILE__, __LINE__, getpid(), pthread_self(), ##arg)
#define XLOG_NOTICE(fmt, arg...)  xlog_write(LOG_NOTICE|LOG_LOCAL6, "[%s:%d][[%d]%x][NOTICE]: "#fmt, __FILE__, __LINE__, getpid(), pthread_self(), ##arg)
#define XLOG_INFO(fmt, arg...)    xlog_write(LOG_INFO|LOG_LOCAL6, "[%s:%d][%d][%x][INFO]: "#fmt, __FILE__, __LINE__, getpid(), pthread_self(), ##arg)
#define XLOG_DEBUG(fmt, arg...)   xlog_write(LOG_DEBUG|LOG_LOCAL6, "[%s:%d][%d][%x][DEBUG]: "#fmt, __FILE__, __LINE__, getpid(), pthread_self(), ##arg) 

static inline void xlog_init(const char *name) {
  openlog(name, LOG_CONS|LOG_PID, LOG_LOCAL6);
}

static inline void xlog_close() {
  closelog();
}

void xlog_write(int priority, const char *message, ...);

#endif
