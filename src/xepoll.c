#include "xepoll.h"
#include "xcycle.h"
#include "xmalloc.h"
#include "xutil.h"
#include "xlog.h"
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <errno.h>

typedef struct xepoll_s {
  xtask     *_rtask;
  xtask     *_wtask;
  unsigned  _mask;          // mask set in epoll
} xepoll __attribute__((aligned(sizeof(long))));

static int        				epfd;
static int        				maxfd;
static xepoll 				    *epolls;
static struct epoll_event *epoll_events;

static bool change_epoll(unsigned fd, xepoll *epoll_t, unsigned newmask) {
  if (epoll_t->_mask == newmask) return true;
  // set epoll_event 
  struct epoll_event ee;
  ee.data.u64 = 0;
  ee.data.fd  = fd;
  ee.events   = 0;
  if (newmask & XEPOLL_READ) ee.events |= EPOLLIN;
  if (newmask & XEPOLL_WRITE) ee.events |= EPOLLOUT;
  // set op type 
  int op = EPOLL_CTL_MOD; 
  if (epoll_t->_mask == XEPOLL_NONE) {
    op = EPOLL_CTL_ADD;
  } else if (newmask == XEPOLL_NONE) {
    op = EPOLL_CTL_DEL; 
  }
  if (epoll_ctl(epfd, op, fd, &ee) == -1) {
    XLOG_ERR("epoll_ctl error: fd %d, %s", fd, strerror(errno));
    return false;
  }
  epoll_t->_mask = newmask;
  return true;
}

bool xepoll_enable(unsigned fd, unsigned mask, xtask *task) {
  ASSERT(task);
  if (fd >= maxfd) {
    XLOG_ERR("fd is too large : %d", fd);
    return false;
  }
  xepoll *epoll_t = &epolls[fd];
  if (mask & XEPOLL_READ) epoll_t->_rtask = task;
  if (mask & XEPOLL_WRITE) epoll_t->_wtask = task;
  return change_epoll(fd, epoll_t, epoll_t->_mask | mask);
}

bool xepoll_disable(unsigned fd, unsigned mask) {
  if (fd >= maxfd) {
    XLOG_ERR("fd is too large : %d", fd);
    return false;
  }
  xepoll *epoll_t = &epolls[fd];
  if (mask & XEPOLL_READ) epoll_t->_rtask = NULL;
  if (mask & XEPOLL_WRITE) epoll_t->_wtask = NULL;
  return change_epoll(fd, epoll_t, epoll_t->_mask & (~mask));
}

void xepoll_process(int timeout) {
  int events_n = epoll_wait(epfd, epoll_events, maxfd, timeout);
  if (unlikely(events_n < 0)) {
    if (errno == EINTR) return;
    XLOG_ERR("epoll_wait error: %s", strerror(errno));
    return;
  }
  if (events_n == 0) return;
  // check events
  for (int i=0; i<events_n; i++) {
    struct epoll_event *e = &epoll_events[i];
    xepoll *epoll_t = &epolls[e->data.fd];
    if ((e->events & EPOLLIN) && (epoll_t->_mask & XEPOLL_READ)) {
      xtask_enqueue(epoll_t->_rtask);
    }
    if ((e->events & EPOLLOUT) && (epoll_t->_mask & XEPOLL_WRITE)) {
      xtask_enqueue(epoll_t->_wtask);
    }
  }
}

static bool _init(void) {
  epfd = epoll_create(10240);
  if (epfd < 0) {
    XLOG_ERR("epoll_create error: %s", strerror(errno));
    return false;
  }

  maxfd = cycle.maxfd;
  epolls = xcalloc(sizeof(xepoll)*maxfd);
	epoll_events = xcalloc(sizeof(struct epoll_event)*maxfd);
  return true;
}

static void _deinit(void) {
  close(epfd);
  xfree(epolls);
	xfree(epoll_events);
}

xmodule xepoll_module = {
  "xepoll",
  XCORE_MODULE,
  _init,
  NULL,
  NULL,
  _deinit,
};
