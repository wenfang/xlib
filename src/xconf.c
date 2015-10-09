#include "xconf.h"
#include "xopt.h"
#include <stdlib.h>

xconf global_conf;

bool xconf_load(void) {
  global_conf.daemon = xopt_int("global", "daemon", 0);
  if ((global_conf.procs = xopt_int("global", "procs", 1)) <= 0) return false;
  if ((global_conf.maxfd = xopt_int("global", "maxfd", 10000)) <= 0) return false;
  global_conf.pidfile = xopt_string("global", "pidfile", "/tmp/xlib.pid");
  return true;
}
