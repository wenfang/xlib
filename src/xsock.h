#ifndef __XSOCK_H 
#define __XSOCK_H

#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

int xsock_accept(int fd);
int xsock_accept_timeout(int fd, int timeout);

bool xsock_set_block(int fd, int block);

int xsock_tcp_server(const char* addr, int port);
int xsock_udp_server(const char* addr, int port);

static inline int xsock_tcp(void) {
  return socket(AF_INET, SOCK_STREAM, 0);
}

static inline int xsock_udp(void) {
  return socket(AF_INET, SOCK_DGRAM, 0);
}

static inline int xsock_close(int fd) {
  return close(fd);
}

#endif
