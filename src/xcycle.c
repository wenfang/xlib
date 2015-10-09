#include "xcycle.h"
#include "xopt.h"

#include <stdlib.h>

xcycle cycle;

bool xcycle_init(const char *config_file) {
  cycle.daemon = xopt_int("global", "daemon", 0);
  if ((cycle.procs = xopt_int("global", "procs", 1)) <= 0) return false;
  if ((cycle.maxfd = xopt_int("global", "maxfd", 10000)) <= 0) return false;
  cycle.pidfile = xopt_string("global", "pidfile", "/tmp/xlib.pid");
  return true;
}

void xcycle_deinit(void) {
}
