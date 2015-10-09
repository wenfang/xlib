#include "xuser.h"

static xserver *server;

static void _conn_exit(void *arg1, void *arg2) {
  xconn *conn = arg1;
  xconn_free(conn);
}

static void mainHandler(void *arg1, void *arg2) {
  xconn *conn = arg1;
  conn->post_wtask.handler = XHANDLER(_conn_exit, conn, NULL);
  xconn_write(conn, "OK\r\n", 4);
  xconn_flush(conn);
}

static bool _init(void) {
  server = xserver_register("127.0.0.1", 7879, mainHandler, NULL);
  if (server == NULL) {
    XLOG_ERR("xserver_register error");
    return false;
  }
  return true;
}

static void _deinit(void) {
  xserver_unregister(server); 
}

xmodule xuser_module = {
  "xuser",
  XUSER_MODULE,
  _init,
  NULL,
  NULL,
  _deinit,
};
