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

static int _read_common(xio* io, xstring* s) {

  int res; 
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
        xstring_range(io->_rbuf, io->_rbytes, -1);
        io->_rtype = XIO_READNONE;
        return io->_rbytes;
      }
    } else if (io->_rtype == XIO_READUNTIL) {
      int pos = xstring_search(io->_rbuf, io->_delim);
      if (pos != -1) {
        unsigned len = pos + strlen(io->_delim);
        *s = xstring_catlen(*s, io->_rbuf, len);
        xstring_range(io->_rbuf, len, -1);
        io->_rtype = XIO_READNONE;
        return len;
      }
    }
 
    io->_rbuf = xstring_catfd(io->_rbuf, io->_fd, BUF_LEN, &res);
    if (res < 0) {
      if (errno == EINTR) continue;
      io->_error = 1;
      break;
    }
    if (res == 0) {
      io->_closed = 1;
      break;
    }
  }
  // read error copy data
  *s = xstring_catxs(*s, io->_rbuf);
  xstring_clean(io->_rbuf);
  io->_rtype = XIO_READNONE;
  return res;
}

int xio_read(xio* io, xstring* s) {
  ASSERT(io && s);
  if (io->_closed) return XIO_CLOSED;
  if (io->_error) return XIO_ERROR;

  io->_rtype  = XIO_READ;
  return _read_common(io, s);
}

int xio_readbytes(xio* io, unsigned len, xstring* s) {
  ASSERT(io && len && s);
  if (io->_closed) return XIO_CLOSED;
  if (io->_error) return XIO_ERROR;

  io->_rtype  = XIO_READBYTES;
  io->_rbytes = len;
  return _read_common(io, s);
}

int xio_readuntil(xio* io, const char* delim, xstring* s) {  
  ASSERT(io && delim && s);
  if (io->_closed) return XIO_CLOSED;
  if (io->_error) return XIO_ERROR;

  io->_rtype = XIO_READUNTIL;
  io->_delim = delim;
  return _read_common(io, s);
}

int xio_write(xio *io, xstring s) {
  ASSERT(io && s);
  if (io->_closed) return XIO_CLOSED;
  if (io->_error) return XIO_ERROR;

  io->_wbuf = xstring_catxs(io->_wbuf, s);
  return xstring_len(s);
}

int xio_flush(xio *io) {
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
  xstring_range(io->_wbuf, total, -1);
  return total;
}

xio* xio_newfile(const char* fname, int create) {
  ASSERT(fname);
  int fd;
  if (create) {
    fd = open(fname, O_RDWR | O_CREAT, 0666);
  } else {
    fd = open(fname, O_RDWR);
  }
  if (fd < 0) return NULL;

  return xio_newfd(fd);
}

xio* xio_newfd(int fd) {
  xio *io = xcalloc(sizeof(xio));
  io->_fd   = fd;
  io->_rbuf = xstring_empty();
  io->_wbuf = xstring_empty();
  return io;
}

void xio_free(xio *io) {
  ASSERT(io);
  close(io->_fd);
  xstring_free(io->_rbuf);
  xstring_free(io->_wbuf);
  xfree(io);
}

#ifdef __XIO_TEST

#include "xunittest.h"

int main(void) {
  xio* io = xio_newfile("testdata/xioest", 1);
  if (io == NULL) XTEST_EQ(0, 1);
  xstring s = xstring_new("this is  a test");
  xio_write(io, s);
  xio_flush(io);
  xio_free(io);

  io = xio_newfile("testdata/xioest", 0);
  if (io == NULL) XTEST_EQ(0, 1);

  xstring_clean(s);
  xio_readuntil(io, " ", &s);
  XTEST_STRING_EQ(s, "this ");

  xstring_clean(s);
  xio_readuntil(io, " ", &s);
  XTEST_STRING_EQ(s, "is ");

  xstring_clean(s);
  xio_readuntil(io, " ", &s);
  XTEST_STRING_EQ(s, " ");

  xstring_clean(s);
  xio_readuntil(io, " ", &s);
  XTEST_STRING_EQ(s, "a ");

  xstring_clean(s);
  xio_readuntil(io, " ", &s);
  XTEST_STRING_EQ(s, "test");
  XTEST_EQ(io->_closed, 1);

  xstring_free(s);
  xio_free(io);
  return 0;
}

#endif
