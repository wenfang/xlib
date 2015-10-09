#ifndef __XSERVER_H
#define __XSERVER_H

#include "xconn.h"
#include "xhandler.h"

#include <stdbool.h>

typedef struct xserver_s {
  unsigned          _sfd;
  xhandlerFunc      _handlerFunc;
  void              *_arg;
  xtask             _listen_task;
  pthread_mutex_t   *_accept_mutex;
  unsigned          _accept_mutex_hold;
  struct list_head  _node;
} xserver __attribute__((aligned(sizeof(long))));

void xserver_preloop(void);
void xserver_postloop(void);

xserver* xserver_register(const char *addr, int port, xhandlerFunc handler, void *arg);
void xserver_unregister(xserver *server);

#endif
