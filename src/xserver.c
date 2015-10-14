#include "xserver.h"
#include "xshm.h"
#include "xmalloc.h"
#include "xsock.h"
#include "xcycle.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// global server list
static LIST_HEAD(_head);

static void _accept(void *arg, void *nop) {
  xserver *server = arg;
  int cfd = xsock_accept(server->_sfd);
  if (cfd <= 0) return;
  if (!server->_handlerFunc) {
    xsock_close(cfd);
    return;
  }
  xconn *conn = xconn_newfd(cfd);
  if (!conn) {
    XLOG_ERR("xconn_newfd error");
    xsock_close(cfd);
    return;
  }
  conn->post_rtask.handler = XHANDLER(server->_handlerFunc, conn, server->_arg);
  xtask_enqueue(&conn->post_rtask);
}

void _xserver_preloop(void) {
  xserver *server = NULL;
  list_for_each_entry(server, &_head, _node) {
    if (!pthread_mutex_trylock(server->_accept_mutex)) {
      if (server->_accept_mutex_hold) continue;
      server->_accept_mutex_hold = 1;
      xepoll_enable(server->_sfd, XEPOLL_LISTEN, &server->_listen_task);
      continue;
    }
    if (server->_accept_mutex_hold) {
      xepoll_disable(server->_sfd, XEPOLL_LISTEN);
      server->_accept_mutex_hold = 0;
    }
  }
}

void _xserver_postloop(void) {
  xserver *server = NULL;
  list_for_each_entry(server, &_head, _node) {
    if (server->_accept_mutex_hold) pthread_mutex_unlock(server->_accept_mutex);
  }
}

xserver* xserver_register(const char *addr, int port, xhandlerFunc handler, void *arg) {
  // create server fd
  int sfd = xsock_tcp_server(addr, port);
  if (sfd < 0) {
    XLOG_ERR("xsock_tcp_server error %s:%d", addr, port);
    return NULL;
  }
  xsock_set_block(sfd, 0);
  // create server
  xserver *server       = xcalloc(sizeof(xserver));
  server->_sfd          = sfd;
  server->_handlerFunc  = handler;
  server->_arg          = arg;
  xtask_init(&server->_listen_task);
  server->_listen_task.handler = XHANDLER(_accept, server, NULL);
  server->_listen_task.flags |= XTASK_FAST;
  server->_accept_mutex = xshm_mutex_new();
  if (!server->_accept_mutex) {
    XLOG_ERR("xshm_mutex_new error");
    xsock_close(sfd);
    xfree(server);
    return NULL;
  }
  INIT_LIST_HEAD(&server->_node);
  list_add_tail(&server->_node, &_head);
  return server;
}

void xserver_unregister(xserver *server) {
  ASSERT(server);
  if (server->_accept_mutex_hold) {
    xepoll_disable(server->_sfd, XEPOLL_LISTEN);
  }
  if (server->_accept_mutex) xshm_mutex_free(server->_accept_mutex);
  xsock_close(server->_sfd);
  list_del(&server->_node);
  xfree(server);
}
