#ifndef __XREDIS_H
#define __XREDIS_H

#include "xconn.h"
#include "xtask.h"
#include "xlist.h"

// for xredisMsg type
#define XREDIS_STR    1
#define XREDIS_ERR    2
#define XREDIS_INT    3
#define XREDIS_BULK   4
#define XREDIS_ARRAY  5

typedef struct xredisMsg_s {
  xstring   *data;
  unsigned  len;
  unsigned  size;
  unsigned  type;
} xredisMsg;

typedef struct xredis_s {
  xlist       *reqList;
  xlist       *rspList;
  xredisMsg   *rspBuf;
  xtask       task;

  const char    *_addr;
  const char    *_port;
  xconn         *_conn;
  unsigned int  _timeout;
  int           _status;
} xredis;

void xredis_do(xredis *rds, xredisMsg *req);

xredis* xredis_new(const char *addr, const char *port);
void xredis_free(void *arg);

xredisMsg* xredisMsg_new(unsigned size);
void xredisMsg_free(void *arg);

#endif
