#ifndef __SPE_CONN_H
#define __SPE_CONN_H

#include "spe_module.h"
#include "spe_epoll.h"
#include "xtask.h"
#include "spe_buf.h"
#include "spe_handler.h"
#include "spe_util.h"
#include <string.h>

#define XCONN_CONNECT_TIMEOUT 1
#define XCONN_READ_TIMEOUT    1
#define XCONN_WRITE_TIMEOUT   2
#define XCONN_CLOSED          4
#define XCONN_ERROR           8

typedef struct {
  int       _fd;
  xtask_t   _rtask;
  xtask_t   _wtask;
  xtask_t   post_rtask;
  xtask_t   post_wtask;
  unsigned  _rtimeout;
  unsigned  _wtimeout;
  xstring   buf;
  xstring   _rbuf;
  xstring   _wbuf;

  char*     _delim;
  unsigned  _rbytes;
  unsigned  _rtype:2;
  unsigned  _wtype:1;
  unsigned  ctimeout:1;
  unsigned  rtimeout:1;
  unsigned  wtimeout:1;
  unsigned  closed:1;
  unsigned  error:1;
  unsigned  flags;
} xconn_t __attribute__((aligned(sizeof(long))));

bool xconn_connect(xconn_t *conn, const char *addr, const char *port);
bool xconn_read(xconn_t *conn);
bool xconn_readbytes(xconn_t *conn, unsigned len);
bool xconn_readuntil(xconn_t *conn, const char* delim);

static inline bool xconn_write(xconn_t* conn, char* buf, unsigned len) {
  ASSERT(conn && buf && len);
  if (conn->closed || conn->error) return false;
  return spe_buf_append(conn->_write_buffer, buf, len);
}

bool xconn_flush(xconn_t* conn);
void xconn_set_timeout(xconn_t* conn, unsigned read_expire_time, unsigned write_expire_time);

xconn_t* xconn_newfd(unsigned fd);
void xconn_free(xconn_t *conn);

bool xconn_init(void);
bool xconn_deinit(void);

#endif
