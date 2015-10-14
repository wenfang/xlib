#include "xredis.h"
#include "xsock.h"
#include "xmalloc.h"

#define XREDIS_FREE   1
#define XREDIS_START  2
#define XREDIS_CONN   3
#define XREDIS_ERROR  4

static void _send_done(void *arg1, void *arg2) {
  xredis *rds = arg1;
  xtask_enqueue(&rds->task);
}

static void _send(xredis *rds) {
  for (int i=0; i<rds->reqCnt; i++) {
    if (i != 0) xconn_writes(rds->_conn, " ");
    xconn_write(rds->_conn, rds->req[i], xstring_len(rds->req[i]));
  }
  xconn_writes(rds->_conn, "\r\n");
  xconn_flush(rds->_conn);
}

static void _conn_done(void *arg1, void *arg2) {
  xredis *rds = arg1;
  if (XCONN_IS_ERROR(rds->_conn)) {
    printf("connect error\n");
    return;
  }
  rds->_status = XREDIS_CONN;
  rds->_conn->post_wtask.handler = XHANDLER(_send_done, rds, NULL);
  if (rds->reqCnt != 0) _send(rds);
}

static void _conn(xredis *rds) {
  if (rds->_status == XREDIS_CONN) return;
  int cfd = xsock_tcp();
  rds->_conn = xconn_newfd(cfd);
  rds->_status = XREDIS_START;
  rds->_conn->post_rtask.handler = XHANDLER(_conn_done, rds, NULL);
  xconn_connect(rds->_conn, rds->_addr, rds->_port);
}

void xredis_do(xredis *rds, xstring *req, int reqCnt) {
  if (rds->_status == XREDIS_FREE) {
    rds->req = req;
    rds->reqCnt = reqCnt;
    _conn(rds);
    return;
  }
  _send(rds);
}

xredis* xredis_new(const char *addr, const char *port) {
  xredis *rds = xcalloc(sizeof(xredis));
  rds->_addr = addr;
  rds->_port = port;
  rds->_status = XREDIS_FREE;
  xtask_init(&rds->task);
  return rds;
}

void xredis_free(xredis *rds) {
  if (rds->_status != XREDIS_FREE) {
    xconn_free(rds->_conn);
  }
  xstrings_free(rds->req, rds->reqCnt);
  xstrings_free(rds->rsp, rds->rspCnt);
  xfree(rds);
}
