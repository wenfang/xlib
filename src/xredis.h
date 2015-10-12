#ifndef __XREDIS_H
#define __XREDIS_H

#include "xconn.h"
#include "xtask.h"

typedef struct xredis_s {
  xtask       post_task;
  xstring     *buf;
  int         cnt;

  const char    *_addr;
  const char    *_port;
  xconn         *_conn;
  unsigned int  _cmd_timeout;
  int           _status;
} xredis;

xredis* xredis_new(const char *addr, const char *port);
void xredis_free(xredis *rds);

#endif
