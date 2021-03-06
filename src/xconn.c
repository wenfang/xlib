#include "xconn.h"
#include "xcycle.h"
#include "xmalloc.h"
#include "xtask.h"
#include "xsock.h"
#include "xutil.h"
#include <errno.h>
#include <netdb.h>

#define BUF_SIZE  1024

#define XCONN_READNONE   0
#define XCONN_READ       1
#define XCONN_READUNTIL  2
#define XCONN_READBYTES  3

#define XCONN_WRITENONE  0
#define XCONN_WRITE      1

#define XCONN_CONNECT    1

static xconn *conns;
static int maxfd;
static int usedfd;

static void _connect_normal(void *arg, void *nop) {
  xconn *conn = arg;
  // connect timeout
  if (conn->_rtimeout && XTASK_IS_TIMEOUT(conn->_rtask)) {
    conn->flags |= XCONN_CTIMEOUT;
    goto end_out;
  }
  int err = 0;
  socklen_t errlen = sizeof(err);
  if (getsockopt(conn->_fd, SOL_SOCKET, SO_ERROR, &err, &errlen) == -1) conn->flags |= XCONN_ERROR;
  if (err) conn->flags |= XCONN_ERROR;

end_out:
  xepoll_disable(conn->_fd, XEPOLL_READ|XEPOLL_WRITE);
  if (conn->_rtimeout) xtask_dequeue(&conn->_rtask);
  conn->_rtype  = XCONN_READNONE;
  conn->_wtype  = XCONN_WRITENONE;
  xtask_enqueue(&conn->post_rtask);
}

bool xconn_connect(xconn *conn, const char *addr, const char *port) {
  ASSERT(conn && conn->_rtype == XCONN_READNONE && conn->_wtype == XCONN_WRITENONE && addr && port);
  // gen address hints
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family   = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  // get address info into servinfo
  struct addrinfo *servinfo;
  if (getaddrinfo(addr, port, &hints, &servinfo)) {
    XLOG_ERR("getaddrinfo error");
    return false;
  }
  // try the first address
  if (!servinfo) {
    XLOG_ERR("servinfo is null");
    return false;
  }
  if (connect(conn->_fd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
    if (errno == EINPROGRESS) {
      // (async):
      conn->_rtask.handler = XHANDLER(_connect_normal, conn, NULL);
      conn->flags   &= ~XCONN_CTIMEOUT;
      conn->_rtype  = XCONN_CONNECT;
      conn->_wtype  = XCONN_CONNECT;
      xepoll_enable(conn->_fd, XEPOLL_READ|XEPOLL_WRITE, &conn->_rtask);
      if (conn->_rtimeout) {
        xtask_enqueue_timeout(&conn->_rtask, conn->_rtimeout);
      }
      freeaddrinfo(servinfo);
      return true;
    }
    conn->flags |= XCONN_ERROR;
  }
  // (sync): connect success or failed, call handler
  xtask_enqueue(&conn->post_rtask);
  freeaddrinfo(servinfo);
  return true;
}

static void _read_normal(void *arg, void *nop) {
  xconn *conn = arg;
  // check timeout
  if (conn->_rtimeout && XTASK_IS_TIMEOUT(conn->_rtask)) {
    conn->flags |= XCONN_RTIMEOUT;
    goto end_out;
  }
  // read data
  for (;;) {
    int res;
    conn->_rbuf = xstring_catfd(conn->_rbuf, conn->_fd, BUF_SIZE, &res);
    if (res == -1) { // read error
      if (errno == EINTR) continue;
      if (errno == EAGAIN) break;
      XLOG_ERR("conn error: %s", strerror(errno));
      conn->flags |= XCONN_ERROR;
    } else if (res == 0) { // peer close
      conn->flags |= XCONN_CLOSED;
    }
    break;
  }
  // check read type
  if (conn->_rtype == XCONN_READUNTIL) {
    int pos = xstring_search(conn->_rbuf, conn->_delim);
    if (pos != -1) {
      unsigned len = pos + strlen(conn->_delim);
      conn->buf = xstring_catlen(conn->buf, conn->_rbuf, len);
      xstring_range(conn->_rbuf, len, -1);
      goto end_out;
    }
  } else if (conn->_rtype == XCONN_READBYTES) {
    if (conn->_rbytes <= xstring_len(conn->_rbuf)) {
      conn->buf = xstring_catlen(conn->buf, conn->_rbuf, conn->_rbytes);
      xstring_range(conn->_rbuf, conn->_rbytes, -1);
      goto end_out;
    }
  } else if (conn->_rtype == XCONN_READ) {
    if (xstring_len(conn->_rbuf) > 0) { 
      conn->buf = xstring_catxs(conn->buf, conn->_rbuf);
      xstring_clean(conn->_rbuf);
      goto end_out;
    }
  }
  // check error and close 
  if (conn->flags & (XCONN_CLOSED | XCONN_ERROR)) goto end_out;
  return;

end_out:
  xepoll_disable(conn->_fd, XEPOLL_READ);
  if (conn->_rtimeout) xtask_dequeue(&conn->_rtask);
  conn->_rtype = XCONN_READNONE;
  xtask_enqueue(&conn->post_rtask);
}

static bool _readsync(xconn *conn) {
  char buf[BUF_SIZE];
  int res = read(conn->_fd, buf, BUF_SIZE);
  if (res < 0 && errno != EINTR && errno != EAGAIN) conn->flags |= XCONN_ERROR;
  if (res == 0) conn->flags |= XCONN_CLOSED;
  if (res > 0) conn->_rbuf = xstring_catlen(conn->_rbuf, buf, res);
  if (conn->flags & (XCONN_CLOSED | XCONN_ERROR)) {
    conn->buf = xstring_catxs(conn->buf, conn->_rbuf);
    xstring_clean(conn->_rbuf);
    xtask_enqueue(&conn->post_rtask);
    return true;
  }
  return false;
}

static void _readasync(xconn *conn, unsigned rtype) {
  conn->_rtask.handler = XHANDLER(_read_normal, conn, NULL);
  conn->_rtimeout = 0;
  conn->_rtype    = rtype;
  xepoll_enable(conn->_fd, XEPOLL_READ, &conn->_rtask);
  if (conn->_rtimeout) {
    xtask_enqueue_timeout(&conn->_rtask, conn->_rtimeout);
  }
} 

bool xconn_readuntil(xconn *conn, const char *delim) {
  ASSERT(conn && conn->_rtype == XCONN_READNONE && delim);
  if (conn->flags & (XCONN_CLOSED | XCONN_ERROR)) return false;
  // (sync): fast read fast fail
  if (_readsync(conn)) return true;
  // check Buffer for delim
  int pos = xstring_search(conn->_rbuf, delim);
  if (pos != -1) {
    unsigned len = pos + strlen(delim); 
    conn->buf = xstring_catlen(conn->buf, conn->_rbuf, len);
    xstring_range(conn->_rbuf, len, -1);
    xtask_enqueue(&conn->post_rtask);
    return true;
  }
  // (async):
  conn->_delim = delim;
  _readasync(conn, XCONN_READUNTIL);
  return true;
}

bool xconn_readbytes(xconn *conn, unsigned len) {
  ASSERT(conn && conn->_rtype == XCONN_READNONE && len > 0);
  if (conn->flags & (XCONN_CLOSED | XCONN_ERROR)) return false;
  // (sync):
  if (_readsync(conn)) return true;
  // check buffer
  if (len <= xstring_len(conn->_rbuf)) {
    conn->buf = xstring_catxs(conn->buf, conn->_rbuf);
    xstring_range(conn->_rbuf, len, -1);
    xtask_enqueue(&conn->post_rtask);
    return true;
  }
  // (async):
  conn->_rbytes = len;
  _readasync(conn, XCONN_READBYTES);
  return true;
}

bool xconn_read(xconn *conn) {
  ASSERT(conn && conn->_rtype == XCONN_READNONE);
  if (conn->flags & (XCONN_CLOSED | XCONN_ERROR)) return false;
  // (sync): fast out
  if (_readsync(conn)) return true;
  // check buffer
  if (xstring_len(conn->_rbuf) > 0) {
    conn->buf = xstring_catxs(conn->buf, conn->_rbuf);
    xstring_clean(conn->_rbuf);
    xtask_enqueue(&conn->post_rtask);
    return true;
  }
  // (async):
  _readasync(conn, XCONN_READ);
  return true;
}

static void _write_normal(void *arg, void *nop) {
  xconn *conn = arg;
  // check timeout
  if (conn->_wtimeout && XTASK_IS_TIMEOUT(conn->_wtask)) {
    conn->flags |= XCONN_WTIMEOUT;
    goto end_out;
  }
  // write date
  int res = write(conn->_fd, conn->_wbuf, xstring_len(conn->_wbuf));
  if (res < 0) {
    if (errno == EPIPE) {
      conn->flags |= XCONN_CLOSED;
    } else {
      XLOG_ERR("conn write error: %s", strerror(errno));
      conn->flags |= XCONN_ERROR;
    }
    goto end_out;
  }
  xstring_range(conn->_wbuf, res, -1); 
  if (xstring_len(conn->_wbuf) == 0) goto end_out;
  return;

end_out:
  xepoll_disable(conn->_fd, XEPOLL_WRITE);
  if (conn->_wtimeout) xtask_dequeue(&conn->_wtask);
  conn->_wtype = XCONN_WRITENONE;
  xtask_enqueue(&conn->post_wtask);
}

bool xconn_flush(xconn *conn) {
  ASSERT(conn);
  if (conn->_wtype == XCONN_WRITE) return true;
  if (conn->flags & (XCONN_CLOSED | XCONN_ERROR)) return false;
  // sync write
  int res = write(conn->_fd, conn->_wbuf, xstring_len(conn->_wbuf));
  if (res < 0) {
    if (errno == EPIPE) {
      conn->flags |= XCONN_CLOSED;
    } else {
      XLOG_ERR("conn write error: %s", strerror(errno));
      conn->flags |= XCONN_ERROR;
    }
    xtask_enqueue(&conn->post_wtask);
    return true;
  }
  xstring_range(conn->_wbuf, res, -1); 
  if (xstring_len(conn->_wbuf) == 0) {
    xtask_enqueue(&conn->post_wtask);
    return true;
  }
  // async write
  conn->_wtask.handler = XHANDLER(_write_normal, conn, NULL);
  conn->flags   &= ~XCONN_WTIMEOUT;
  conn->_wtype  = XCONN_WRITE;
  xepoll_enable(conn->_fd, XEPOLL_WRITE, &conn->_wtask);
  if (conn->_wtimeout) {
    xtask_enqueue_timeout(&conn->_wtask, conn->_wtimeout);
  }
  return true;
}

void xconn_set_timeout(xconn *conn, unsigned rtimeout, unsigned wtimeout) {
  ASSERT(conn);
  conn->_rtimeout = rtimeout;
  conn->_wtimeout = wtimeout;
}

static void _init_conn(xconn *conn, unsigned fd) {
  if (fd > usedfd) usedfd = fd;
  conn->_fd = fd;
  xtask_init(&conn->_rtask);
  xtask_init(&conn->_wtask);
  xtask_init(&conn->post_rtask);
  xtask_init(&conn->post_wtask);

  conn->buf   = xstring_empty();
  conn->_rbuf = xstring_empty();
  conn->_wbuf = xstring_empty();
}

xconn* xconn_newfd(unsigned fd) {
  if (unlikely(fd >= maxfd)) return NULL;

  xsock_set_block(fd, 0);
  xconn *conn = &conns[fd];
  if (conn->_fd == 0) {
    _init_conn(conn, fd);
  } else {
    xstring_clean(conn->buf);
    xstring_clean(conn->_rbuf);
    xstring_clean(conn->_wbuf);
    xtask_init(&conn->_rtask);
    xtask_init(&conn->_wtask);
    xtask_init(&conn->post_rtask);
    xtask_init(&conn->post_wtask);
  }
  // init conn status
  conn->_rtimeout = 0;
  conn->_wtimeout = 0;
  conn->_rtype    = XCONN_READNONE;
  conn->_wtype    = XCONN_WRITENONE;
  conn->flags     = 0;
  return conn;
}

void xconn_free(xconn *conn) {
  ASSERT(conn);
  xtask_dequeue(&conn->_rtask);
  xtask_dequeue(&conn->_wtask);
  xtask_dequeue(&conn->post_rtask);
  xtask_dequeue(&conn->post_wtask);
  xepoll_disable(conn->_fd, XEPOLL_READ|XEPOLL_WRITE);
  xsock_close(conn->_fd);
}

static bool _init(void) {
  conns = xcalloc(sizeof(xconn)*cycle.maxfd);
  maxfd = cycle.maxfd;
  return true;
}

static void _deinit(void) {
  for(int i=0; i<=usedfd; i++) {
    xconn *conn = &conns[i];
    xstring_free(conn->buf);
    xstring_free(conn->_rbuf);
    xstring_free(conn->_wbuf);
  }
  xfree(conns);
}

xmodule xconn_module = {
  "xconn",
  XCORE_MODULE,
  _init,
  NULL,
  NULL,
  _deinit,
};
