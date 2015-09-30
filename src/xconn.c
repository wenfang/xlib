#include "xconn.h"
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

static xconn_t *conns;
static int conn_maxfd;

static void
connect_normal(void* arg) {
  xconn_t* conn = arg;
  // connect timeout
  if (conn->_read_expire_time && conn->_read_task.timeout) {
    conn->connect_timeout = 1;
    goto end_out;
  }
  int err = 0;
  socklen_t errlen = sizeof(err);
  if (getsockopt(conn->_fd, SOL_SOCKET, SO_ERROR, &err, &errlen) == -1) conn->error = 1;
  if (err) conn->error = 1;

end_out:
  spe_epoll_disable(conn->_fd, SPE_EPOLL_READ|SPE_EPOLL_WRITE);
  if (conn->_read_expire_time) spe_task_dequeue(&conn->_read_task);
  conn->_read_type  = XCONN_READNONE;
  conn->_write_type = XCONN_WRITENONE;
  spe_task_schedule(&conn->post_read_task);
}

/*
===================================================================================================
xconn_connect
===================================================================================================
*/
bool
xconn_connect(xconn_t* conn, const char* addr, const char* port) {
  ASSERT(conn && conn->_read_type == XCONN_READNONE && conn->_write_type == XCONN_WRITENONE && 
      addr && port);
  // gen address hints
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family   = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  // get address info into servinfo
  struct addrinfo *servinfo;
  if (getaddrinfo(addr, port, &hints, &servinfo)) return false;
  // try the first address
  if (!servinfo) return false;
  if (connect(conn->_fd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
    if (errno == EINPROGRESS) {
      // (async):
      spe_task_set_handler(&conn->_read_task, SPE_HANDLER1(connect_normal, conn), 0);
      conn->connect_timeout = 0;
      conn->_read_type      = XCONN_CONNECT;
      conn->_write_type     = XCONN_CONNECT;
      spe_epoll_enable(conn->_fd, SPE_EPOLL_READ|SPE_EPOLL_WRITE, &conn->_read_task);
      if (conn->_read_expire_time) {
        spe_task_schedule_timeout(&conn->_read_task, conn->_read_expire_time);
      }
      freeaddrinfo(servinfo);
      return true;
    }
    conn->error = 1;
  }
  // (sync): connect success or failed, call handler
  spe_task_schedule(&conn->post_read_task);
  freeaddrinfo(servinfo);
  return true;
}

static void
read_normal(void* arg) {
  xconn_t* conn = arg;
  // check timeout
  if (conn->_read_expire_time && conn->_read_task.timeout) {
    conn->read_timeout = 1;
    goto end_out;
  }
  // read data
  for (;;) {
    int res = spe_buf_read_fd_append(conn->_fd, BUF_SIZE, conn->_read_buffer);
    // read error
    if (res == -1) {
      if (errno == EINTR) continue;
      if (errno == EAGAIN) break;
      SPE_LOG_ERR("conn error: %s", strerror(errno));
      conn->error = 1;
      break;
    }
    // peer close
    if (res == 0) {
      conn->closed = 1;
      break;
    }
    break;
  }
  // check read type
  if (conn->_read_type == XCONN_READUNTIL) {
    int pos = spe_buf_search(conn->_read_buffer, conn->_delim);
    if (pos != -1) {
      unsigned len = pos + strlen(conn->_delim);
      spe_buf_append(conn->buffer, conn->_read_buffer->data, len);
      spe_buf_lconsume(conn->_read_buffer, len);
      goto end_out;
    }
  } else if (conn->_read_type == XCONN_READBYTES) {
    if (conn->_rbytes <= conn->_read_buffer->len) {
      spe_buf_append(conn->buffer, conn->_read_buffer->data, conn->_rbytes);
      spe_buf_lconsume(conn->_read_buffer, conn->_rbytes);
      goto end_out;
    }
  } else if (conn->_read_type == XCONN_READ) {
    if (conn->_read_buffer->len > 0) { 
      spe_buf_append(conn->buffer, conn->_read_buffer->data, conn->_read_buffer->len);
      spe_buf_clean(conn->_read_buffer);
      goto end_out;
    }
  }
  // check error and close 
  if (conn->closed || conn->error) goto end_out;
  return;

end_out:
  spe_epoll_disable(conn->_fd, SPE_EPOLL_READ);
  if (conn->_read_expire_time) spe_task_dequeue(&conn->_read_task);
  conn->_read_type = XCONN_READNONE;
  spe_task_schedule(&conn->post_read_task);
}

static bool _readsync(xconn_t* conn) {
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

static void
xconn_read_async(xconn_t* conn, unsigned read_type) {
  spe_task_set_handler(&conn->_read_task, SPE_HANDLER1(read_normal, conn), 0);
  conn->read_timeout  = 0;
  conn->_read_type    = read_type;
  spe_epoll_enable(conn->_fd, SPE_EPOLL_READ, &conn->_read_task);
  if (conn->_read_expire_time) {
    spe_task_schedule_timeout(&conn->_read_task, conn->_read_expire_time);
  }
} 

/*
===================================================================================================
xconn_read_until
===================================================================================================
*/
bool
xconn_read_until(xconn_t* conn, char* delim) {
  ASSERT(conn && conn->_read_type == XCONN_READNONE);
  if (!delim || conn->closed || conn->error) return false;
  // (sync):
  if (_readsync(conn)) return true;
  // check Buffer for delim
  int pos = spe_buf_search(conn->_read_buffer, delim);
  if (pos != -1) {
    unsigned len = pos + strlen(delim); 
    spe_buf_append(conn->buffer, conn->_read_buffer->data, len);
    spe_buf_lconsume(conn->_read_buffer, len);
    spe_task_schedule(&conn->post_read_task);
    return true;
  }
  // (async):
  conn->_delim = delim;
  xconn_read_async(conn, XCONN_READUNTIL);
  return true;
}

/*
===================================================================================================
xconn_readbytes
===================================================================================================
*/
bool
xconn_readbytes(xconn_t* conn, unsigned len) {
  ASSERT(conn && conn->_read_type == XCONN_READNONE);
  if (len == 0 || conn->closed || conn->error ) return false;
  // (sync):
  if (_readsync(conn)) return true;
  // check buffer
  if (len <= conn->_read_buffer->len) {
    spe_buf_append(conn->buffer, conn->_read_buffer->data, len);
    spe_buf_lconsume(conn->_read_buffer, len);
    spe_task_schedule(&conn->post_read_task);
    return true;
  }
  // (async):
  conn->_rbytes = len;
  xconn_read_async(conn, XCONN_READBYTES);
  return true;
}

bool xconn_read(xconn_t *conn) {
  ASSERT(conn && conn->_read_type == XCONN_READNONE);
  if (conn->flags & (XCONN_CLOSED | XCONN_ERROR)) return false;
  // (sync): fast out
  if (!_readsync(conn)) return true;
  // check buffer
  if (xstring_len(conn->_rbuf) > 0) {
    spe_buf_append(conn->buffer, conn->_read_buffer->data, conn->_read_buffer->len);
    spe_buf_clean(conn->_read_buffer);
    spe_task_schedule(&conn->post_read_task);
    return true;
  }
  // (async):
  xconn_read_async(conn, XCONN_READ);
  return true;
}

/*
===================================================================================================
write_normal
===================================================================================================
*/
static void
write_normal(void* arg) {
  xconn_t* conn = arg;
  // check timeout
  if (conn->_write_expire_time && conn->_write_task.timeout) {
    conn->write_timeout = 1;
    goto end_out;
  }
  // write date
  int res = write(conn->_fd, conn->_write_buffer->data, conn->_write_buffer->len);
  if (res < 0) {
    if (errno == EPIPE) {
      conn->closed = 1;
    } else {
      SPE_LOG_ERR("conn write error: %s", strerror(errno));
      conn->error = 1;
    }
    goto end_out;
  }
  spe_buf_lconsume(conn->_write_buffer, res);
  if (conn->_write_buffer->len == 0) goto end_out;
  return;

end_out:
  spe_epoll_disable(conn->_fd, SPE_EPOLL_WRITE);
  if (conn->_write_expire_time) spe_task_dequeue(&conn->_write_task);
  conn->_write_type = XCONN_WRITENONE;
  spe_task_schedule(&conn->post_write_task);
}

/*
===================================================================================================
xconn_flush
===================================================================================================
*/
bool
xconn_flush(xconn_t* conn) {
  ASSERT(conn && conn->_write_type == XCONN_WRITENONE);
  if (conn->closed || conn->error) return false;
  int res = write(conn->_fd, conn->_write_buffer->data, conn->_write_buffer->len);
  if (res < 0) {
    if (errno == EPIPE) {
      conn->closed = 1;
    } else {
      SPE_LOG_ERR("conn write error: %s", strerror(errno));
      conn->error = 1;
    }
    spe_task_schedule(&conn->post_write_task);
    return true;
  }
  spe_buf_lconsume(conn->_write_buffer, res);
  if (conn->_write_buffer->len == 0) {
    spe_task_schedule(&conn->post_write_task);
    return true;
  }
  spe_task_set_handler(&conn->_write_task, SPE_HANDLER1(write_normal, conn), 0);
  conn->write_timeout = 0;
  conn->_write_type   = XCONN_WRITE;
  spe_epoll_enable(conn->_fd, SPE_EPOLL_WRITE, &conn->_write_task);
  if (conn->_write_expire_time) {
    spe_task_schedule_timeout(&conn->_write_task, conn->_write_expire_time);
  }
  return true;
}

void xconn_set_timeout(xconn_t *conn, unsigned rtimeout, unsigned wtimeout) {
  ASSERT(conn);
  conn->_rtimeout = rtimeout;
  conn->_wtimeout = wtimeout;
}

static bool _init_conn(xconn_t *conn, unsigned fd) {
  conn->_fd = fd;
  xtask_init(&conn->_rtask);
  xtask_init(&conn->_wtask);
  xtask_init(&conn->post_rtask);
  xtask_init(&conn->post_wtask);

  conn->buf   = xstring_empty();
  conn->_rbuf = xstring_empty();
  conn->_wbuf = xstring_empty();
  if (!conn->buf || !conn->_rbuf || !conn->_wbuf) {
    xstring_free(conn->buf);
    xstring_free(conn->_rbuf);
    xstring_free(conn->_wbuf);
    return false;
  }
  return true;
}

xconn_t* xconn_newfd(unsigned fd) {
  if (unlikely(fd >= conn_maxfd)) return NULL;

  xsock_set_block(fd, 0);
  xconn_t* conn = &conns[fd];
  if (conn->_fd == 0 && !_init_conn(conn, fd)) return NULL;
  xstring_clean(conn->buf);
  xstring_clean(conn->_rbuf);
  xstring_clean(conn->_wbuf);
  // init conn status
  conn->_rtimeout = 0;
  conn->_wtimeout = 0;
  conn->_rtype    = XCONN_READNONE;
  conn->_wtype    = XCONN_WRITENONE;
  conn->flags     = 0;
  return conn;
}

void xconn_free(xconn_t *conn) {
  ASSERT(conn);
  xtask_dequeue(&conn->_read_task);
  xtask_dequeue(&conn->_write_task);
  xtask_dequeue(&conn->post_read_task);
  xtask_dequeue(&conn->post_write_task);
  xepoll_disable(conn->_fd, XEPOLL_READ|XEPOLL_WRITE);
  xsock_close(conn->_fd);
}

bool xconn_init(void) {
  conns = xcalloc(sizeof(xconn_t)*global_conf.maxfd);
  if (!conns) return false;
  conn_maxfd = global_conf.maxfd;
  return true;
}

bool xconn_deinit(void) {
  free(conns);
  return true;
}
