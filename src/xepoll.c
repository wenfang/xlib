#include "xepoll.h"
#include "xconf.h"
#include "xmalloc.h"
#include "xsock.h"
#include "xutil.h"
#include "xlog.h"
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <errno.h>

typedef struct xepoll_s {
  unsigned  mask:2;             // mask set in epoll
  xtask     *read_task;
  xtask     *write_task;
} xepoll __attribute__((aligned(sizeof(long))));

static int        				epfd;
static int        				maxfd;
static xepoll 				    *epolls;
static struct epoll_event *epoll_events;

static bool change_epoll(unsigned fd, xepoll *epoll_t, unsigned newmask) {
  if (epoll_t->mask == newmask) return true;
  // set epoll_event 
  struct epoll_event ee;
  ee.data.u64 = 0;
  ee.data.fd  = fd;
  ee.events   = 0;
  if (newmask & XEPOLL_READ) ee.events |= EPOLLIN;
  if (newmask & XEPOLL_WRITE) ee.events |= EPOLLOUT;
  // set op type 
  int op = EPOLL_CTL_MOD; 
  if (epoll_t->mask == XEPOLL_NONE) {
    op = EPOLL_CTL_ADD;
  } else if (newmask == XEPOLL_NONE) {
    op = EPOLL_CTL_DEL; 
  }
  if (epoll_ctl(epfd, op, fd, &ee) == -1) {
    XLOG_ERR("epoll_ctl error: fd %d, %s", fd, strerror(errno));
    return false;
  }
  epoll_t->mask = newmask;
  return true;
}

bool xepoll_enable(unsigned fd, unsigned mask, xtask *task) {
  ASSERT(task);
  if (fd >= maxfd) return false;
  xepoll *epoll_t = &epolls[fd];
  if (mask & XEPOLL_READ) epoll_t->read_task = task;
  if (mask & XEPOLL_WRITE) epoll_t->write_task = task;
  return change_epoll(fd, epoll_t, epoll_t->mask | mask);
}

bool xepoll_disable(unsigned fd, unsigned mask) {
  if (fd >= maxfd) return false;
  xepoll *epoll_t = &epolls[fd];
  if (mask & XEPOLL_READ) epoll_t->read_task = NULL;
  if (mask & XEPOLL_WRITE) epoll_t->write_task = NULL;
  return change_epoll(fd, epoll_t, epoll_t->mask & (~mask));
}

void xepoll_process(int timeout) {
  struct epoll_event* e;
  xepoll *epoll_t;
  int i, events_n;
  
  events_n = epoll_wait(epfd, epoll_events, maxfd, timeout);
  if (unlikely(events_n < 0)) {
    if (errno == EINTR) return;
    XLOG_ERR("epoll_wait error: %s", strerror(errno));
    return;
  }
  if (events_n == 0) return;
  // check events
  for (i=0; i<events_n; i++) {
    e = &epoll_events[i];
    epoll_t = &epolls[e->data.fd];
    if ((e->events & EPOLLIN) && (epoll_t->mask & XEPOLL_READ)) {
      xtask_enqueue(epoll_t->read_task);
    }
    if ((e->events & EPOLLOUT) && (epoll_t->mask & XEPOLL_WRITE)) {
      xtask_enqueue(epoll_t->write_task);
    }
  }
}

bool xepoll_init(void) {
  epfd = epoll_create(10240);
  if (epfd < 0) {
    XLOG_ERR("epoll_create: %s", strerror(errno));
    return false;
  }

  maxfd = global_conf.maxfd;
  epolls = xcalloc(sizeof(xepoll)*maxfd);
  if (!epolls) {
    XLOG_ERR("xcalloc error");
    goto err_out1;
  }

	epoll_events = xcalloc(sizeof(struct epoll_event)*maxfd);
	if (!epoll_events) {
    XLOG_ERR("xcalloc error");
    goto err_out2;
	}
  return true;

err_out2:
  xfree(epolls);
err_out1:
  close(epfd);
  return false;
}

void xepoll_deinit(void) {
  close(epfd);
  xfree(epolls);
	xfree(epoll_events);
}

#ifdef __XEPOLL_TEST

#include "xunittest.h"

int main(void) {
  xconf_load();
  xepoll_init();
  xepoll_deinit();
  return 0;
}

#endif
