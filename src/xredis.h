#ifndef __XREDIS_H
#define __XREDIS_H

#include "xconn.h"
#include "xtask.h"

typedef struct xredis_s {
  xstring     *req;
  xstring     *rsp;
  int         reqCnt;
  int         rspCnt;
  xtask       task;

  const char    *_addr;
  const char    *_port;
  xconn         *_conn;
  unsigned int  _timeout;
  int           _status;
} xredis;

void xredis_do(xredis *rds, xstring *cmd, int cnt);

xredis* xredis_new(const char *addr, const char *port);
void xredis_free(xredis *rds);

#endif
