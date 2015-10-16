#ifndef __XMYSQL_H
#define __XMYSQL_H

#include "xconn.h"

typedef struct xmysql_s {
  const char  *_addr;
  const char  *_port;
  xconn       *_conn;
  int         _status;
} xmysql;

void xmysql_query(xmysql *mysql);

xmysql* xmysql_new(const char *addr, const char *port);
void xmysql_free(void *arg);

#endif
