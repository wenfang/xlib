#ifndef __XCYCLE_H
#define __XCYCLE_H

#include <stdbool.h>

#define XMODULE_MAX  128

typedef struct xcycle_s {
  int         daemon;
  int         procs;
  int         maxfd;
  const char  *pidfile;
  void        *ctx[XMODULE_MAX];
} xcycle;

bool xcycle_init(const char *config_file);
void xcycle_deinit(void);

extern xcycle cycle;

#endif
