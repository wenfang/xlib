AUTOMAKE_OPTIONS=foreign

bin_PROGRAMS=xstring xio xopt xtask xepoll xlist xtpool xdict xshm

XMALLOC = xmalloc.c xmalloc.h xlog.h xlog.c
XSTRING = xstring.c xstring.h $(XMALLOC)
XIO = xio.c xio.h $(XSTRING)

xstring_SOURCES = $(XSTRING)
xstring_CPPFLAGS = -D__XSTRING_TEST -Wall

xio_SOURCES = $(XIO)
xio_CPPFLAGS = -D__XIO_TEST -Wall

xopt_SOURCES = xopt.c xopt.h $(XIO)
xopt_CPPFLAGS = -D__XOPT_TEST -Wall

xtask_SOURCES = xtask.c rbtree.c xtask.h rbtree.h
xtask_CPPFLAGS = -D__XTASK_TEST -Wall

xepoll_SOURCES = xepoll.c xconf.c xopt.c xio.c xstring.c xmalloc.c xtask.c xlog.c rbtree.c xepoll.h xconf.h xopt.h xio.h xstring.h xmalloc.h xtask.h
xepoll_CPPFLAGS = -D__XEPOLL_TEST -Wall

xlist_SOURCES = xlist.c xlist.h $(XMALLOC)
xlist_CPPFLAGS = -D__XLIST_TEST -Wall

xtpool_SOURCES = xtpool.c xtpool.h $(XMALLOC)
xtpool_CPPFLAGS = -D__XTPOOL_TEST -Wall
xtpool_LDFLAGS = -lpthread

xdict_SOURCES = xdict.c xdict.h $(XMALLOC)
xdict_CPPFLAGS = -D__XDICT_TEST -Wall

xshm_SOURCES = xshm.c xshm.h $(XMALLOC)
xshm_CPPFLAGS = -D__XSHM_TEST -Wall
xshm_LDFLAGS = -lpthread

xconn_SOURCES = xconn.c xtask.c
xconn_CPPFLAGS = -D__XCONN_TEST -Wall
