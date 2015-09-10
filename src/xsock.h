#ifndef __XSOCK_H 
#define __XSOCK_H

#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

extern int  
xsock_accept(int fd);

extern int  
xsock_accept_timeout(int fd, int timeout);

extern bool 
xsock_set_block(int fd, int block);

extern int  
xsock_tcp_server(const char* addr, int port);

extern int
xsock_udp_server(const char* addr, int port);

static inline int 
xsock_tcp_socket(void) {
  return socket(AF_INET, SOCK_STREAM, 0);
}

static inline int 
xsock_udp_socket(void) {
  return socket(AF_INET, SOCK_DGRAM, 0);
}

static inline int 
xsock_close(int fd) {
  return close(fd);
}

#endif
