#include "xredis.h"
#include "xsock.h"
#include "xmalloc.h"

#define XREDIS_FREE     1
#define XREDIS_START    2
#define XREDIS_CONN     3
#define XREDIS_TIMEOUT  4
#define XREDIS_ERROR    5

static void _send(xredis *rds) {
  for (int i=0; i<rds->reqCnt; i++) {
    if (i != 0) xconn_writes(rds->_conn, " ");
    xconn_write(rds->_conn, rds->req[i], xstring_len(rds->req[i]));
  }
  xconn_writes(rds->_conn, "\r\n");
  // flush command to redis server
  xconn_flush(rds->_conn);
}

#define PARSE_START 1

static int _parse_start(xredis *rds) {
  if (*rds->_conn->buf == ':' ||
      *rds->_conn->buf == '+' ||
      *rds->_conn->buf == '-') {
    xredisRsp *rsp = xredisRsp_new(1);
    rsp->data[0] = xstring_cpylen(rsp->data[0], rds->_conn->buf+1, xstring_len(rds->_conn->buf)-3);
    if (*rds->_conn->buf == '+') {
      rsp->type = XREDIS_STR;
    } else if (*rds->_conn->buf == '-') {
      rsp->type = XREDIS_ERR;
    } else if (*rds->_conn->buf == ':') {
      rsp->type = XREDIS_INT;
    };
    xlist_addNodeTail(rds->rspList, rsp);
    return 1;
  }
  return 0;
}

static int _parse_data(xredis *rds) {
  switch (rds->_phase) {
  case PARSE_START:
    return _parse_start(rds);
  }
  return 0;
}

// recv data callback
static void _recv_cb(void *arg1, void *arg2) {
  xredis *rds = arg1;
  if (_parse_data(rds)) xtask_enqueue(&rds->task);
  xconn_readuntil(rds->_conn, "\r\n");
}

// conn callback
static void _conn_cb(void *arg1, void *arg2) {
  xredis *rds = arg1;
  if (XCONN_IS_ERROR(rds->_conn)) {
    printf("connect error\n");
    return;
  }
  rds->_status = XREDIS_CONN;
  rds->_conn->post_rtask.handler = XHANDLER(_recv_cb, rds, NULL);
  if (rds->reqCnt != 0) _send(rds);
  xconn_readuntil(rds->_conn, "\r\n");
}

static void _conn(xredis *rds) {
  if (rds->_status == XREDIS_CONN) return;
  int cfd = xsock_tcp();
  rds->_conn = xconn_newfd(cfd);
  rds->_status = XREDIS_START;
  rds->_conn->post_rtask.handler = XHANDLER(_conn_cb, rds, NULL);
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
  rds->rspList = xlist_new();
  rds->rspList->free = xredisRsp_free; 
  xtask_init(&rds->task);
  rds->_phase = PARSE_START;
  return rds;
}

void xredis_free(xredis *rds) {
  if (rds->_status != XREDIS_FREE) {
    xconn_free(rds->_conn);
  }
  xstrings_free(rds->req, rds->reqCnt);
  xlist_free(rds->rspList);
  xfree(rds);
}

xredisRsp* xredisRsp_new(unsigned size) {
  xredisRsp *rsp = xmalloc(sizeof(xredisRsp));
  rsp->data = xmalloc(sizeof(xstring)*size);
  for (int i=0; i<size; i++) {
    rsp->data[i] = xstring_empty();
  }
  rsp->cnt = size;
  return rsp;
}

void xredisRsp_free(void *arg) {
  xredisRsp *rsp = arg;
  for (int i=0; i<rsp->cnt; i++) {
    xstring_free(rsp->data[i]);
  }
  xfree(rsp->data);
  xfree(rsp);
}
