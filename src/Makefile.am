AUTOMAKE_OPTIONS=foreign

bin_PROGRAMS=xstring xio xopt xtask xepoll xlist xtpool xdict xconn xserver

XMALLOC = xmalloc.c xmalloc.h xlog.h xlog.c
XSTRING = xstring.c xstring.h $(XMALLOC)
XIO = xio.c xio.h $(XSTRING)
XOPT = xopt.c xopt.h $(XIO)
XTASK = xtask.c xtask.h rbtree.c rbtree.h
XEPOLL = xepoll.c xepoll.h xconf.c xconf.h $(XOPT) $(XTASK)
XCONN = xconn.c xconn.h xsock.c xsock.h $(XEPOLL)

xstring_SOURCES = $(XSTRING)
xstring_CPPFLAGS = -D__XSTRING_TEST -Wall

xio_SOURCES = $(XIO)
xio_CPPFLAGS = -D__XIO_TEST -Wall

xopt_SOURCES = $(XOPT)
xopt_CPPFLAGS = -D__XOPT_TEST -Wall

xtask_SOURCES = $(XTASK)
xtask_CPPFLAGS = -D__XTASK_TEST -Wall

xepoll_SOURCES = $(XEPOLL)
xepoll_CPPFLAGS = -D__XEPOLL_TEST -Wall

xlist_SOURCES = xlist.c xlist.h $(XMALLOC)
xlist_CPPFLAGS = -D__XLIST_TEST -Wall

xtpool_SOURCES = xtpool.c xtpool.h $(XMALLOC)
xtpool_CPPFLAGS = -D__XTPOOL_TEST -Wall
xtpool_LDFLAGS = -lpthread

xdict_SOURCES = xdict.c xdict.h $(XMALLOC)
xdict_CPPFLAGS = -D__XDICT_TEST -Wall

xconn_SOURCES = $(XCONN)
xconn_CPPFLAGS = -D__XCONN_TEST -Wall

xserver_SOURCES = xserver.c xserver.h xshm.c xshm.h $(XCONN)
xserver_CPPFLAGS = -D__XSERVER_TEST -Wall
xserver_LDFLAGS = -lpthread
