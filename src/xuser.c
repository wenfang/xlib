#include "xuser.h"
#include "xmalloc.h"
#include "cJSON.h"
#include "http_parser.h"

static xserver *server1;
static xserver *server2;
static xserver *server3;

static void _conn_exit(void *arg1, void *arg2) {
  xconn *conn = arg1;
  xconn_free(conn);
}

/*
static void _read_command(void *arg1, void *arg2) {
  xconn *conn = arg1;
  xconn_writes(conn, "HTTP/1.1 200 OK\r\n");
  printf("%ld\n", http_parser_version());
  xconn_flush(conn);
}
*/

static void _main_done(void *arg1, void *arg2) {
  xconn *conn = arg1;
  xredis *rds = arg2;
  xredis_free(rds);
  xconn_free(conn);
}

static void _main_call(void *arg1, void *arg2) {
  xconn *conn = arg1;
  xredis *rds = xredis_new("127.0.0.1", "6379");
  rds->task.handler = XHANDLER(_main_done, conn, rds);
  xstring *cmd = xmalloc(sizeof(xstring)*2);
  cmd[0] = xstring_new("incr");
  cmd[1] = xstring_new("mytestabc");
  xredis_do(rds, cmd, 2);
}

static void mainHandler(void *arg1, void *arg2) {
  xconn *conn = arg1;
  conn->post_rtask.handler = XHANDLER(_main_call, conn, NULL);
  xconn_readuntil(conn, "\r\n\r\n");
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

typedef struct httpconn_s {
  http_parser *parser;
  xconn       *conn;  
} httpconn;

static int _on_url(http_parser *parser, const char *at, size_t length) {
  xstring url = xstring_newlen(at, length);
  printf("_on_url called: %s\n", url);
  xstring_free(url);
  return 0;
}

static void _parse_header(void *arg1, void *arg2) {
  http_parser_settings settings;
  http_parser_settings_init(&settings);
  settings.on_url = _on_url;

  httpconn *httpconn = arg1;
  http_parser_execute(httpconn->parser, &settings, httpconn->conn->buf, xstring_len(httpconn->conn->buf));
  xconn_free(httpconn->conn);
  xfree(httpconn->parser);
  xfree(httpconn);
}

static void httpHandler(void *arg1, void *arg2) {
  httpconn *httpconn = xmalloc(sizeof(httpconn));
  httpconn->parser = xmalloc(sizeof(http_parser));
  httpconn->conn = arg1;
  http_parser_init(httpconn->parser, HTTP_REQUEST); 
  httpconn->conn->post_rtask.handler = XHANDLER(_parse_header, httpconn, NULL);
  xconn_readuntil(httpconn->conn, "\r\n\r\n");
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
  server3 = xserver_register("127.0.0.1", 7881, httpHandler, NULL);
  if (server3 == NULL) {
    XLOG_ERR("xserver_register error");
    return false;
  }
  return true;
}

static void _deinit(void) {
  xserver_unregister(server1); 
  xserver_unregister(server2);
  xserver_unregister(server3);
}

xmodule xuser_module = {
  "xuser",
  XUSER_MODULE,
  _init,
  NULL,
  NULL,
  _deinit,
};
