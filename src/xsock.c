#include "xsock.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>

#define DEFAULT_BACKLOG	10240

static bool 
sock_addr_valid(const char* addr) {
  if (!addr) return false;
  int dot_cnt = 0;
  int addrLen = strlen(addr);
  for (int i=0; i<addrLen; i++) {
    if (addr[i] >='0' && addr[i]<='9') continue;
    if (addr[i] == '.') {
      dot_cnt++;
      continue;
    }
    return false;
  }
  if (dot_cnt != 3) return false;
  return true;
}

int 
xsock_tcp_server(const char* addr, int port) {
  int sfd;
  if ((sfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) return -1;
  //set socket option
  int flags = 1;
  if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, (void*)&flags, sizeof(flags)) < 0) goto error_out;
  if (setsockopt(sfd, SOL_SOCKET, SO_KEEPALIVE, (void*)&flags, sizeof(flags)) < 0) goto error_out;
  flags = 5;
  if (setsockopt(sfd, IPPROTO_TCP, TCP_DEFER_ACCEPT, &flags, sizeof(flags)) < 0) goto error_out;
  // set linger
  struct linger ling = {0, 0};
  if (setsockopt(sfd, SOL_SOCKET, SO_LINGER, (void*)&ling, sizeof(ling)) < 0) goto error_out;
  //set socket address
  struct sockaddr_in saddr;
  bzero(&saddr, sizeof(saddr));
  saddr.sin_family = AF_INET;
  if (sock_addr_valid(addr)) {
    if (inet_aton(addr, &saddr.sin_addr) == 0) goto error_out;
  } else {
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);
  }
  saddr.sin_port = htons(port);
  //bind and listen
  if (bind(sfd, (struct sockaddr *)&saddr, sizeof(saddr)) != 0) goto error_out;
  if (listen(sfd, DEFAULT_BACKLOG) != 0) goto error_out;
  return sfd;

error_out:
  xsock_close(sfd);
  return -1;
}

int 
xsock_udp_server(const char* addr, int port) {
  int sfd;
  if ((sfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) return -1;

  struct sockaddr_in saddr;
  bzero(&saddr, sizeof(saddr));
  saddr.sin_family = AF_INET;
  if (sock_addr_valid(addr)) {
    if (inet_aton(addr, &saddr.sin_addr) == 0) goto error_out;
  } else {
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);
  }
  saddr.sin_port = htons(port);

  if (bind(sfd, (struct sockaddr *)&saddr, sizeof(saddr)) != 0) goto error_out;
  return sfd;

error_out:
  xsock_close(sfd);
  return -1;
}

int 
xsock_accept(int sfd) {
  struct sockaddr_in caddr;
  socklen_t caddr_len = 0;
  bzero(&caddr, sizeof(caddr));
  return accept(sfd, (struct sockaddr*)&caddr, &caddr_len);
}

int 
xsock_accept_timeout(int sfd, int timeout) {
  struct sockaddr_in caddr;
  socklen_t caddr_len = 0;
  bzero(&caddr, sizeof(caddr));
  // init poll struct
  struct pollfd pfd;
  for (;;) {
    pfd.fd = sfd;
    pfd.events = POLLIN;
    int res= poll(&pfd, 1, timeout);
    // poll error or timeout
    if (res < 0) {
      if (errno == EINTR) continue;
      return res;
    }
    if (res == 0) return 0;
    // poll success accept
    int cfd = accept(sfd, (struct sockaddr*)&caddr, &caddr_len);
    if (cfd < 0) {
      if (errno == EINTR) continue;
      if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
    }
    return cfd;
  }
}

bool 
xsock_set_block(int fd, int block) {
  int flags;
  if ((flags = fcntl(fd, F_GETFL, 0)) < 0) return false;
  // set new flags;
  if (block) {
    flags &= ~O_NONBLOCK;
  } else {
    flags |= O_NONBLOCK;
  }
  if (fcntl(fd, F_SETFL, flags) < 0) return false;
  return true;
}
