#include "xconf.h"
#include "xopt.h"
#include <stdlib.h>

xconf global_conf;

bool 
xconf_load() {
  cycle.daemon = spe_opt_int("global", "daemon", 0);
  if ((cycle.procs = spe_opt_int("global", "procs", 1)) <= 0) return false;
  if ((cycle.maxfd = spe_opt_int("global", "maxfd", 1000000)) <= 0) return false;
  cycle.pidfile = spe_opt_string("global", "pidfile", "/tmp/spe.pid");
  return true;
}
