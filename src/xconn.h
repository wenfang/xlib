#ifndef __XCONN_H
#define __XCONN_H

#include "xepoll.h"
#include "xmodule.h"
#include "xtask.h"
#include "xstring.h"
#include "xutil.h"
#include <string.h>

// conn flags
#define XCONN_CTIMEOUT  1
#define XCONN_RTIMEOUT  1
#define XCONN_WTIMEOUT  2
#define XCONN_CLOSED    4
#define XCONN_ERROR     8

typedef struct {
  xstring   buf;
  xtask     post_rtask;
  xtask     post_wtask;
  unsigned  flags;

  int       _fd;
  xtask     _rtask;
  xtask     _wtask;
  unsigned  _rtimeout;
  unsigned  _wtimeout;
  xstring   _rbuf;
  xstring   _wbuf;

  const char  *_delim;
  unsigned    _rbytes;
  unsigned    _rtype:2;
  unsigned    _wtype:1;
} xconn __attribute__((aligned(sizeof(long))));

bool xconn_connect(xconn *conn, const char *addr, const char *port);
bool xconn_read(xconn *conn);
bool xconn_readbytes(xconn *conn, unsigned len);
bool xconn_readuntil(xconn *conn, const char *delim);

static inline bool xconn_write(xconn *conn, const char *buf, unsigned len) {
  ASSERT(conn && buf && len);
  if (conn->flags & (XCONN_CLOSED | XCONN_ERROR)) return false;
  conn->_wbuf = xstring_catlen(conn->_wbuf, buf, len);
  return true;
}

static inline bool xconn_writes(xconn *conn, const char *buf) {
  return xconn_write(conn, buf, strlen(buf));
}

bool xconn_flush(xconn *conn);
void xconn_set_timeout(xconn *conn, unsigned rtimeout, unsigned wtimeout);

xconn* xconn_newfd(unsigned fd);
void xconn_free(xconn *conn);

extern xmodule xconn_module;

#endif
