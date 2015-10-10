#define _GNU_SOURCE
#include <sched.h>

#include "xworker.h"
#include "xhandler.h"
#include "xcycle.h"
#include "xmodule.h"
#include "xserver.h"
#include "xtask.h"
#include "xepoll.h"
#include "xsignal.h"

#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>

// command list
#define WORKER_STOP  1

// for master
#define MAX_WORKER 128

typedef struct xworker_s {
  pid_t pid;
  int   notify_fd;
} xworker;

static xworker workers[MAX_WORKER];

// for worker
static int control_fd;
static xtask control_task;
static bool _stop;

// for master
static int get_slot(void) {
  for (int i=0; i<MAX_WORKER; i++) {
    if (workers[i].pid == 0) return i;
  }
  return -1;
}

int xworker_fork(void) {
  int proc_slot;
  if ((proc_slot = get_slot()) == -1) return -1;

  int channel[2];
  if (socketpair(AF_UNIX, SOCK_STREAM, 0, channel)) return -1;

  pid_t pid = fork();
  if (pid < 0) return -1;
  // for worker
  if (!pid) {
    // set cpu affinity
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(proc_slot%xcpu_count(), &set);
    sched_setaffinity(0, sizeof(cpu_set_t), &set);
    // set control_fd
    control_fd = channel[1];
    close(channel[0]);
    return 0;
  }
  // for master
  workers[proc_slot].pid        = pid;
  workers[proc_slot].notify_fd  = channel[0];
  close(channel[1]);
  return 1;
}

int xworker_reset(pid_t pid) {
  for (int i=0; i<MAX_WORKER; i++) {
    if (workers[i].pid == pid) {
      workers[i].pid = 0;
      close(workers[i].notify_fd);
      return 0;
    }
  }
  return -1;
}

void xworker_stop(void) {
  int command = WORKER_STOP;
  for (int i=0; i<MAX_WORKER; i++) {
    if (workers[i].pid == 0) continue;
    write(workers[i].notify_fd, &command, sizeof(int));
    workers[i].pid = 0;
    close(workers[i].notify_fd);
  }
}

// for worker
static void _control_handler(void *nop1, void *nop2) {
  int command;
  int res = read(control_fd, &command, sizeof(int));
  if (res < 0) {
    XLOG_ERR("worker read control_fd error");
    return;
  }
  if (res == 0) { // master exit, worker exit
    _stop = true;
    return;
  }
  if (command == WORKER_STOP) {
    _stop = true;
    return;
  }
  XLOG_ERR("invalid command: %d", command);
}

void xworker_process(void) {
	xproctitle_set(cycle.worker_name);
  xsignal_register(SIGPIPE, SIG_IGN);
  xsignal_register(SIGHUP, SIG_IGN);
  xsignal_register(SIGCHLD, SIG_IGN);
  xsignal_register(SIGUSR1, SIG_IGN);
  // init worker
  xmodule_worker_init(XCORE_MODULE);
  xmodule_worker_init(XUSER_MODULE);
  // enable control task
  xtask_init(&control_task);
  control_task.handler = XHANDLER(_control_handler, NULL, NULL);
  xepoll_enable(control_fd, XEPOLL_READ, &control_task);
  // event loop
  unsigned timeout;
  while (!_stop) {
    timeout = !xtask_empty() ? 0 : cycle.ticks;
    xserver_preloop();
    xepoll_process(timeout);
    xserver_postloop();
    xtask_process();
    xsignal_process();
  }
  // disable control task
  xepoll_disable(control_fd, XEPOLL_READ);
  close(control_fd);
  // exit worker
  xmodule_worker_deinit(XUSER_MODULE);
  xmodule_worker_deinit(XCORE_MODULE);
}
