#ifndef __XCONF_H
#define __XCONF_H

#include <stdbool.h>

#define XMODULE_MAX  128

typedef struct xconf_s {
  int         daemon;
  int         procs;
  int         maxfd;
  const char  *pidfile;
  void        *ctx[XMODULE_MAX];
} xconf;

extern bool 
xconf_load();

extern xconf global_conf;

#endif
