#include "xmaster.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

static int xworker_reap;
static void reap_worker(int sig) {
  xworker_reap = 1;
}

static int worker_stop;
static void stop_worker(int sig) {
  worker_stop = 1;
}

static void xmaster_process() {
  xpid_save(cycle.pidfile);
	xproctitle_set(cycle.master_name);

  xsignal_register(SIGPIPE, SIG_IGN);
  xsignal_register(SIGHUP, SIG_IGN);
  xsignal_register(SIGCHLD, reap_worker);
  xsignal_register(SIGTERM, stop_worker);
  xsignal_register(SIGINT, stop_worker);
  xsignal_register(SIGUSR1, stop_worker);

  sigset_t set;
  sigemptyset(&set);
  for (;;) {
    sigsuspend(&set);
    xsignal_process();
    if (xworker_reap) {
      for (;;) {
        int status;
        int pid = waitpid(-1, &status, WNOHANG);
        if (pid == 0) break;
        if (pid < 0) {
          XLOG_ERR("waipid error");
          break;
        }
        if (xworker_reset(pid) < 0) {
          XLOG_ERR("xworker reset error");
          continue;
        }
        // for new worker
        int res = xworker_fork();
        if (res < 0) {
          XLOG_ERR("xworker fork error");
        } else if (res == 0) {
          xworker_process();
          return;
        }
        XLOG_WARNING("worker restart");
      }
      xworker_reap = 0;
    }
    if (worker_stop) {
      xworker_stop();
      break;
    }
  }
  xpid_remove(cycle.pidfile);
}

int main(int argc, char* argv[]) {
  if (argc > 2) {
    fprintf(stdout, "Usage: %s [configFile]\n", argv[0]);
    return 1;
  }
  if (argc == 2 && !xopt_new(argv[1])) {
    fprintf(stderr, "[ERROR] config file parse error\n");
    return 1;
  }
	xproctitle_init(argc, argv);
  // load xcycle
  if (!xcycle_load()) {
    fprintf(stderr, "[ERROR] xcycle_load error\n");
    return 1;
  }
  // set maxfd
  if (!xmax_open_files(cycle.maxfd)) {
    fprintf(stderr, "[ERROR] xmax_open_files %s\n", strerror(errno));
    return 1;
  }
  // daemonize
  if (cycle.daemon && xdaemon()) {
    fprintf(stderr, "[ERROR] spe_daemon\n");
    return 1;
  }
  if (!xmodule_master_init(XCORE_MODULE)) {
    fprintf(stderr, "[ERROR] xmodule_master_init core error\n");
    return 1;
  }
  if (!xmodule_master_init(XUSER_MODULE)) {
    fprintf(stderr, "[ERROR] xmodule_master_init user error\n");
    return 1;
  }
  // fork worker
  int res = 1;
  for (int i=0; i<cycle.procs; i++) {
    res = xworker_fork();
    if (res <= 0) break;
  }
  if (res == 0) { // for worker
      xworker_process();
  } else if (res == 1) { // for master
      xmaster_process();
  } else {
    fprintf(stderr, "[ERROR] xworker_fork error\n");
  }
  xmodule_master_deinit(XUSER_MODULE);
  xmodule_master_deinit(XCORE_MODULE);
  xopt_free();
  return 0;
}
