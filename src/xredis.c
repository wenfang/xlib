#include "xredis.h"
#include "xsock.h"
#include "xmalloc.h"

#define XREDIS_FREE     1
#define XREDIS_START    2
#define XREDIS_CONN     3
#define XREDIS_TIMEOUT  4
#define XREDIS_ERROR    5

static void _send(xredis *rds) {
  xstring buf = xstring_empty();
  while (xlistLength(rds->reqList) > 0) {
    xlistNode *node = xlistFirst(rds->reqList);
    xredisMsg *req = xlistNodeValue(node);
    buf = xstring_catprintf(buf, "*%d\r\n", req->size);
    for (int i=0; i<req->size; i++) {
      buf = xstring_catprintf(buf, "$%d\r\n%s\r\n", xstring_len(req->data[i]), req->data[i]);
    }
    xconn_write(rds->_conn, buf, xstring_len(buf));
    xstring_clean(buf);
    xlist_delNode(rds->reqList, node);
  }
  xstring_free(buf);
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
      xredisMsg *rsp = xredisMsg_new(1);
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
      int pos = xstring_search(rds->_conn->buf, "\r\n");
      if (pos < 0) break;
      int bulkLen = atoi(rds->_conn->buf+1);
      if (xstring_len(rds->_conn->buf)-pos-4 < bulkLen) break;

      if (rds->rspBuf == NULL) { 
        xredisMsg *rsp = xredisMsg_new(1);
        rsp->data[0] = xstring_cpylen(rsp->data[0], rds->_conn->buf+pos+2, bulkLen);
        rsp->len = 1;
        rsp->type = XREDIS_BULK;
        xlist_addNodeTail(rds->rspList, rsp);
        ret = 1;
        xstring_range(rds->_conn->buf, pos+4+bulkLen, -1);
        continue;
      } else {
        xredisMsg *rsp = rds->rspBuf;
        rsp->data[rsp->len] = xstring_cpylen(rsp->data[rsp->len], rds->_conn->buf+pos+2, bulkLen);
        rsp->len++;
        if (rsp->len == rsp->size) {
          rsp->type = XREDIS_ARRAY;
          xlist_addNodeTail(rds->rspList, rsp);
          ret = 1;
          rds->rspBuf = NULL;
        }
        xstring_range(rds->_conn->buf, pos+4+bulkLen, -1);
        continue;
      }
    }
    if (*rds->_conn->buf == '*') {
      int pos = xstring_search(rds->_conn->buf, "\r\n");
      if (pos < 0) break;
      int bulkCnt = atoi(rds->_conn->buf+1);
      rds->rspBuf = xredisMsg_new(bulkCnt);
      xstring_range(rds->_conn->buf, pos+2, -1);
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
  if (xlistLength(rds->reqList) > 0) _send(rds);
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

void xredis_do(xredis *rds, xredisMsg *req) {
  xlist_addNodeTail(rds->reqList, req);
  if (rds->_status == XREDIS_FREE) {
    _conn(rds);
    return;
  }
  _send(rds);
}

xredis* xredis_new(const char *addr, const char *port) {
  xredis *rds = xcalloc(sizeof(xredis));
  rds->reqList = xlist_new();
  rds->reqList->free = xredisMsg_free;
  rds->rspList = xlist_new();
  rds->rspList->free = xredisMsg_free; 
  xtask_init(&rds->task);

  rds->_addr = addr;
  rds->_port = port;
  rds->_status = XREDIS_FREE;
  return rds;
}

void xredis_free(void *arg) {
  if (arg == NULL) return;

  xredis *rds = arg;
  xlist_free(rds->reqList);
  xlist_free(rds->rspList);
  xredisMsg_free(rds->rspBuf);
  if (rds->_status != XREDIS_FREE) xconn_free(rds->_conn);
  xfree(rds);
}

xredisMsg* xredisMsg_new(unsigned size) {
  xredisMsg *rsp = xmalloc(sizeof(xredisMsg));
  rsp->data = xmalloc(sizeof(xstring)*size);
  for (int i=0; i<size; i++) rsp->data[i] = xstring_empty();
  rsp->len  = 0;
  rsp->size = size;
  return rsp;
}

void xredisMsg_free(void *arg) {
  if (arg == NULL) return;

  xredisMsg *rsp = arg;
  for (int i=0; i<rsp->size; i++) xstring_free(rsp->data[i]);
  xfree(rsp->data);
  xfree(rsp);
}
