#ifndef __XIO_H
#define __XIO_H

#include "xstring.h"

#define XIO_CLOSED 0
#define XIO_ERROR  -1

typedef struct xio_s {
  int         _fd;
  xstring     _rbuf;
  xstring     _wbuf;
  const char  *_delim;
  unsigned    _rbytes;
  unsigned    _rtype:2;
  unsigned    _closed:1;
  unsigned    _error:1;
} xio_t __attribute__((aligned(sizeof(long))));

int xio_read(xio_t *io, xstring* s);
int xio_readbytes(xio_t *io, unsigned len, xstring* s);
int xio_readuntil(xio_t *io, const char *delim, xstring* s);

int xio_write(xio_t *io, xstring s);
int xio_flush(xio_t *io);

xio_t* xio_newfile(const char *fname);
xio_t* xio_newfd(int fd);
void xio_free(xio_t *io);

#endif
