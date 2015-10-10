AUTOMAKE_OPTIONS=foreign

bin_PROGRAMS=xstring xio xopt xtask xlist xdict spex

XMALLOC = xmalloc.c xmalloc.h xlog.h xlog.c
XSTRING = xstring.c xstring.h $(XMALLOC)
XIO = xio.c xio.h $(XSTRING)
XOPT = xopt.c xopt.h $(XIO)
XTASK = xtask.c xtask.h rbtree.c rbtree.h
XEPOLL = xepoll.c xepoll.h xcycle.c xcycle.h xmodule.h xmodule.c $(XOPT) $(XTASK)
XCONN = xconn.c xconn.h xsock.c xsock.h $(XEPOLL)

xstring_SOURCES = $(XSTRING)
xstring_CPPFLAGS = -D__XSTRING_TEST -Wall

xio_SOURCES = $(XIO)
xio_CPPFLAGS = -D__XIO_TEST -Wall

xopt_SOURCES = $(XOPT)
xopt_CPPFLAGS = -D__XOPT_TEST -Wall

xtask_SOURCES = $(XTASK)
xtask_CPPFLAGS = -D__XTASK_TEST -Wall

xlist_SOURCES = xlist.c xlist.h $(XMALLOC)
xlist_CPPFLAGS = -D__XLIST_TEST -Wall

xdict_SOURCES = xdict.c xdict.h $(XMALLOC)
xdict_CPPFLAGS = -D__XDICT_TEST -Wall

spex_SOURCES = xmaster.c xmaster.h xserver.c xserver.h xutil.c xutil.h xshm.c xshm.h xworker.c xworker.h xsignal.c xsignal.h xuser.c xuser.h cJSON.c cJSON.h xtpool.c xtpool.h $(XCONN)
spex_CPPFLAGS = -Wall -std=gnu99
spex_LDFLAGS = -lm -lpthread
