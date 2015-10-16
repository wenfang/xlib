#include "xmysql.h"
#include "xmalloc.h"
#include "xsock.h"

#define XMYSQL_FREE 1

typedef struct server_handshake_msg {
  char    version;
  xstring version_info;
} server_handshake_msg;

static void _handshake(void *arg1, void *arg2) {
  xmysql *mysql = arg1;
  printf("%d, %d\n", xstring_len(mysql->_conn->buf), (int)*mysql->_conn->buf);
}

static void _conn_cb(void *arg1, void *arg2) {
  printf("call _conn_cb\n");
  xmysql *mysql = arg1;
  mysql->_conn->post_rtask.handler = XHANDLER(_handshake, mysql, NULL);
  xconn_read(mysql->_conn);
}

static void _conn(xmysql *mysql) {
  int cfd = xsock_tcp();
  mysql->_conn = xconn_newfd(cfd);
  mysql->_conn->post_rtask.handler = XHANDLER(_conn_cb, mysql, NULL);
  xconn_connect(mysql->_conn, mysql->_addr, mysql->_port);
}

void xmysql_query(xmysql *mysql) {
  if (mysql->_status == XMYSQL_FREE) {
    _conn(mysql);
    return;
  }
}

xmysql* xmysql_new(const char *addr, const char *port) {
  xmysql *mysql = xcalloc(sizeof(xmysql));
  mysql->_addr = addr;
  mysql->_port = port;
  mysql->_status = XMYSQL_FREE;
  return mysql;
}

void xmysql_free(void *arg) {
  if (arg == NULL) return;

  xmysql *mysql = arg;
  if (mysql->_status != XMYSQL_FREE) {
    xconn_free(mysql->_conn);
  } 
  xfree(mysql);
}
