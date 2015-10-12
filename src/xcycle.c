#include "xcycle.h"
#include "xopt.h"

xcycle cycle;

bool xcycle_load(void) {
  cycle.daemon = xopt_int("global", "daemon", 0);
  if ((cycle.procs = xopt_int("global", "procs", 1)) <= 0) return false;
  if ((cycle.maxfd = xopt_int("global", "maxfd", 10000)) <= 0) return false;
  if ((cycle.ticks = xopt_int("global", "ticks", 300)) <= 0) return false;
  cycle.pidfile = xopt_string("global", "pidfile", "/tmp/spex.pid");
  cycle.master_name = xopt_string("global", "master_name", "spex: master");
  cycle.worker_name = xopt_string("global", "worker_name", "spex: worker");
  return true;
}
