AUTOMAKE_OPTIONS=foreign

bin_PROGRAMS=xstring xio xopt xtask xepoll

xstring_SOURCES = xstring.c xmalloc.c
xstring_CPPFLAGS = -D__XSTRING_TEST

xio_SOURCES = xio.c xstring.c xmalloc.c
xio_CPPFLAGS = -D__XIO_TEST

xopt_SOURCES = xopt.c xio.c xstring.c xmalloc.c
xopt_CPPFLAGS = -D__XOPT_TEST

xtask_SOURCES = xtask.c rbtree.c
xtask_CPPFLAGS = -D__XTASK_TEST

xepoll_SOURCES = xepoll.c xconf.c xopt.c xio.c xstring.c xmalloc.c xtask.c xlog.c rbtree.c
xepoll_CPPFLAGS = -D__XEPOLL_TEST
