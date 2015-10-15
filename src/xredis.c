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

static int _parse_data(xredis *rds) {
  int ret = 0;
  for (;;) {
    if (xstring_len(rds->_conn->buf) == 0) break;
    if (*rds->_conn->buf == ':' ||
        *rds->_conn->buf == '+' ||
        *rds->_conn->buf == '-') {
      // find \r\n in buffer
      int pos = xstring_search(rds->_conn->buf, "\r\n");
      if (pos < 0) break;
      // \r\n is found 
      xredisRsp *rsp = xredisRsp_new(1);
      rsp->data[0] = xstring_cpylen(rsp->data[0], rds->_conn->buf+1, pos-1);
      if (*rds->_conn->buf == '+') {
        rsp->type = XREDIS_STR;
      } else if (*rds->_conn->buf == '-') {
        rsp->type = XREDIS_ERR;
      } else if (*rds->_conn->buf == ':') {
        rsp->type = XREDIS_INT;
      };
      xlist_addNodeTail(rds->rspList, rsp);
      xstring_range(rds->_conn->buf, pos+2, -1);
      ret = 1;
      continue;
    }
    if (*rds->_conn->buf == '$') {
      int dataStart = xstring_search(rds->_conn->buf, "\r\n");
      if (dataStart < 0) break;
      dataStart += 2;
      int dataLen = xstring_search(rds->_conn->buf+dataStart, "\r\n");
      if (dataLen < 0) break;
      dataLen += 2;
      xredisRsp *rsp = xredisRsp_new(1);
      rsp->data[0] = xstring_cpylen(rsp->data[0], rds->_conn->buf+dataStart, dataLen);
      rsp->type = XREDIS_BULK;
      xlist_addNodeTail(rds->rspList, rsp);
      xstring_range(rds->_conn->buf, dataStart+dataLen+2, -1);
      ret = 1;
      continue;
    }
  }
  return ret;
}

// recv data callback
static void _recv_cb(void *arg1, void *arg2) {
  xredis *rds = arg1;
  if (_parse_data(rds)) xtask_enqueue(&rds->task);
  xconn_read(rds->_conn);
}

// conn callback
static void _conn_cb(void *arg1, void *arg2) {
  xredis *rds = arg1;
  if (XCONN_IS_ERROR(rds->_conn)) {
    // TODO: connnect error
    printf("connect error\n");
    return;
  }
  rds->_status = XREDIS_CONN;
  rds->_conn->post_rtask.handler = XHANDLER(_recv_cb, rds, NULL);
  if (rds->reqCnt != 0) _send(rds);
  xconn_read(rds->_conn);
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
