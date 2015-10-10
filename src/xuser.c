#include "xuser.h"
#include "cJSON.h"

static xserver *server1;
static xserver *server2;

static void _conn_exit(void *arg1, void *arg2) {
  xconn *conn = arg1;
  xconn_free(conn);
}

static void mainHandler(void *arg1, void *arg2) {
  xconn *conn = arg1;
  conn->post_wtask.handler = XHANDLER(_conn_exit, conn, NULL);
  xconn_writes(conn, "OK\r\n");
  xconn_flush(conn);
}

static void monitorHandler(void *arg1, void *arg2) {
  xconn *conn = arg1;
  cJSON *json = cJSON_CreateObject();
  cJSON_AddItemToObject(json, "name", cJSON_CreateString("wenfang"));
  conn->post_wtask.handler = XHANDLER(_conn_exit, conn, NULL);
  xconn_writes(conn, cJSON_PrintUnformatted(json));
  xconn_flush(conn);
  cJSON_Delete(json);
}

static bool _init(void) {
  server1 = xserver_register("127.0.0.1", 7879, mainHandler, NULL);
  if (server1 == NULL) {
    XLOG_ERR("xserver_register error");
    return false;
  }
  server2 = xserver_register("127.0.0.1", 7880, monitorHandler, NULL);
  if (server2 == NULL) {
    XLOG_ERR("xserver_register error");
    return false;
  }
  return true;
}

static void _deinit(void) {
  xserver_unregister(server1); 
  xserver_unregister(server2); 
}

xmodule xuser_module = {
  "xuser",
  XUSER_MODULE,
  _init,
  NULL,
  NULL,
  _deinit,
};
