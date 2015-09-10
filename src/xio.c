#include "xio.h"
#include "xmalloc.h"
#include "xutil.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>

#define BUF_LEN 4096

#define XIO_READNONE     0
#define XIO_READ         1
#define XIO_READBYTES    2
#define XIO_READUNTIL    3

static int read_common(xio_t* io, xstring s) {
  int res;
  char buf[BUF_LEN];

  for (;;) {
    if (io->_rtype == XIO_READ) {
      unsigned len = xstring_len(io->_rbuf);
      if (len > 0) {
        xstring_catxs(s, io->_rbuf);
        xstring_clean(io->_rbuf);
        io->_rtype = XIO_READNONE;
        return len;
      }
    } else if (io->_rtype == XIO_READBYTES) {
      if (io->_rbytes <= xstring_len(io->_rbuf)) {
        xstring_catlen(s, io->_rbuf, io->_rbytes);
        xstring_range(s, io->_rbytes, -1);
        io->_rtype = XIO_READNONE;
        return io->_rbytes;
      }
    } else if (io->_rtype == XIO_READUNTIL) {
      char* pos = strstr(io->_rbuf, io->_delim);
      if (pos != NULL) {
        unsigned len = pos - io->_rbuf + strlen(io->_delim);
        xstring_catlen(s, io->_rbuf, len);
        xstring_range(io->_rbuf, len, -1);
        io->_rtype = XIO_READNONE;
        return len;
      }
    }
    int res = read(io->_fd, buf, BUF_LEN);
    if (res < 0) {
      if (errno == EINTR) continue;
      io->_error = 1;
      break;
    }
    if (res == 0) {
      io->_closed = 1;
      break;
    }
    xstring_catlen(io->_rbuf, buf, res);
  }
  // read error copy data
  xstring_catxs(s, io->_rbuf);
  xstring_clean(io->_rbuf);
  io->_rtype = XIO_READNONE;
  return res;
}

int xio_read(xio_t* io, xstring s) {
  ASSERT(io && s);
  if (io->_closed) return XIO_CLOSED;
  if (io->_error) return XIO_ERROR;

  io->_rtype  = XIO_READ;
  return read_common(io, s);
}

int xio_readbytes(xio_t* io, unsigned len, xstring s) {
  ASSERT(io && len && s);
  if (io->_closed) return XIO_CLOSED;
  if (io->_error) return XIO_ERROR;

  io->_rtype  = XIO_READBYTES;
  io->_rbytes = len;
  return read_common(io, s);
}

int xio_readuntil(xio_t* io, const char* delim, xstring s) {  
  ASSERT(io && delim && s);
  if (io->_closed) return XIO_CLOSED;
  if (io->_error) return XIO_ERROR;

  io->_rtype = XIO_READUNTIL;
  io->_delim = delim;
  return read_common(io, s);
}

int xio_write(xio_t *io, xstring s) {
  ASSERT(io && s);
  if (io->_closed) return XIO_CLOSED;
  if (io->_error) return XIO_ERROR;

  int len = xstring_len(s);
  xstring tmp = xstring_catlen(io->_wbuf, s, len);
  if (tmp == NULL) return 0;
  
  io->_wbuf = tmp;
  return len;
}

int xio_flush(xio_t *io) {
  ASSERT(io);
  if (io->_closed) return XIO_CLOSED;
  if (io->_error) return XIO_ERROR;

  int total = 0, len = xstring_len(io->_wbuf);
  while (total < len) {
    int res = write(io->_fd, io->_wbuf+total, len-total);
    if (res < 0) {
      if (errno == EINTR) continue;
      io->_error = 1;
      break;
    }
    total += res;
  }
  xstring_range(io->_wbuf, total, len-1);
  return total;
}

xio_t* xio_newfile(const char* fname) {
  ASSERT(fname);
  int fd = open(fname, O_RDWR);
  if (fd < 0) return NULL;

  xio_t *io = xio_newfd(fd);
  if (io == NULL) close(fd);
  return io;
}

xio_t* xio_newfd(int fd) {
  xio_t *io = xcalloc(sizeof(xio_t));
  if (io == NULL) return NULL;

  io->_fd   = fd;
  io->_rbuf = xstring_empty();
  if (io->_rbuf == NULL) goto err_out1;
  io->_wbuf = xstring_empty();
  if (io->_wbuf == NULL) goto err_out2;
  return io;

err_out2:
  xstring_free(io->_rbuf);
err_out1:
  xfree(io);
  return NULL;
}

void xio_free(xio_t *io) {
  ASSERT(io);
  close(io->_fd);
  xstring_free(io->_rbuf);
  xstring_free(io->_wbuf);
  xfree(io);
}

#ifdef __XIO_TEST

#include "xunittest.h"

int main(void) {
  return 0;
}

#endif
