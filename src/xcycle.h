#ifndef __XCYCLE_H
#define __XCYCLE_H

#include <stdbool.h>

typedef struct xcycle_s {
  int         daemon;
  int         procs;
  int         maxfd;
  int         ticks;
  const char  *pidfile;
  const char  *master_name;
  const char  *worker_name;
} xcycle;


bool xcycle_load(void);

extern xcycle cycle;

#endif
