#ifndef __XUTIL_H
#define __XUTIL_H

#include "xlog.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/resource.h>

#define likely(x)   __builtin_expect((x), 1)
#define unlikely(x) __builtin_expect((x), 0)

#ifdef DEBUG
  static inline void 
  _Assert(char* file, unsigned line) {
    fprintf(stderr, "Assert Failed [%s:%d]\n", file, line);
    abort();
  }
  #define ASSERT(x) \
    if (!(x)) _Assert(__FILE__, __LINE__)
#else
  #define ASSERT(x)
#endif


static inline unsigned long 
xcurrent_time() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

static inline unsigned
xcpu_count() {
  return sysconf(_SC_NPROCESSORS_ONLN);
}

static inline bool
xmax_open_files(unsigned file_num) {
  struct rlimit r;
  r.rlim_cur = file_num;
  r.rlim_max = file_num;
  if (setrlimit(RLIMIT_NOFILE, &r) < 0) {
    return false;
  }
  return true;
}

extern int
xdaemon();

extern bool
xsave_pid(const char* pid_file);

extern pid_t
xget_pid(const char* pid_file);

extern bool
xremove_pid(const char* pid_file);

extern bool
xinit_proc_title(int argc, char** argv);

extern void
xset_proc_title(char* title);

#endif
