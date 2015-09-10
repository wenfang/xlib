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

static int readcommon(xio_t* io, xstring* s) {
  int res;
  char buf[BUF_LEN];

  for (;;) {
    if (io->_rtype == XIO_READ) {
      unsigned len = xstring_len(io->_rbuf);
      if (len > 0) {
        *s = xstring_catxs(*s, io->_rbuf);
        xstring_clean(io->_rbuf);
        io->_rtype = XIO_READNONE;
        return len;
      }
    } else if (io->_rtype == XIO_READBYTES) {
      if (io->_rbytes <= xstring_len(io->_rbuf)) {
        *s = xstring_catlen(*s, io->_rbuf, io->_rbytes);
        xstring_range(*s, io->_rbytes, -1);
        io->_rtype = XIO_READNONE;
        return io->_rbytes;
      }
    } else if (io->_rtype == XIO_READUNTIL) {
      char* pos = strstr(io->_rbuf, io->_delim);
      if (pos != NULL) {
        unsigned len = pos - io->_rbuf;
        *s = xstring_catlen(*s, io->_rbuf, len);
        xstring_range(io->_rbuf, len + strlen(io->_delim), -1);
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
    io->_rbuf = xstring_catlen(io->_rbuf, buf, res);
  }
  // read error copy data
  *s = xstring_catxs(*s, io->_rbuf);
  xstring_clean(io->_rbuf);
  io->_rtype = XIO_READNONE;
  return res;
}

int xio_read(xio_t* io, xstring* s) {
  ASSERT(io && s);
  if (io->_closed) return XIO_CLOSED;
  if (io->_error) return XIO_ERROR;

  io->_rtype  = XIO_READ;
  return readcommon(io, s);
}

int xio_readbytes(xio_t* io, unsigned len, xstring* s) {
  ASSERT(io && len && s);
  if (io->_closed) return XIO_CLOSED;
  if (io->_error) return XIO_ERROR;

  io->_rtype  = XIO_READBYTES;
  io->_rbytes = len;
  return readcommon(io, s);
}

int xio_readuntil(xio_t* io, const char* delim, xstring* s) {  
  ASSERT(io && delim && s);
  if (io->_closed) return XIO_CLOSED;
  if (io->_error) return XIO_ERROR;

  io->_rtype = XIO_READUNTIL;
  io->_delim = delim;
  return readcommon(io, s);
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
  xio_t* io = xio_newfile("testdata/xio_test");
  if (io == NULL) XTEST_EQ(0, 1);
  xstring s = xstring_new("this is  a test");
  xio_write(io, s);
  xio_flush(io);
  xio_free(io);

  io = xio_newfile("testdata/xio_test");
  if (io == NULL) XTEST_EQ(0, 1);

  xstring_clean(s);
  xio_readuntil(io, " ", &s);
  XTEST_STRING_EQ(s, "this");

  xstring_clean(s);
  xio_readuntil(io, " ", &s);
  XTEST_STRING_EQ(s, "is");

  xstring_clean(s);
  xio_readuntil(io, " ", &s);
  XTEST_STRING_EQ(s, "");

  xstring_clean(s);
  xio_readuntil(io, " ", &s);
  XTEST_STRING_EQ(s, "a");

  xstring_clean(s);
  xio_readuntil(io, " ", &s);
  XTEST_STRING_EQ(s, "test");

  xstring_free(s);
  xio_free(io);
  return 0;
}

#endif