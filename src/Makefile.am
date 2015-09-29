AUTOMAKE_OPTIONS=foreign

bin_PROGRAMS=xstring xio xopt xtask xepoll xlist xtpool xdict xshm

xstring_SOURCES = xstring.c xmalloc.c xstring.h xmalloc.h
xstring_CPPFLAGS = -D__XSTRING_TEST -Wall

xio_SOURCES = xio.c xstring.c xmalloc.c xio.h xstring.h xmalloc.h
xio_CPPFLAGS = -D__XIO_TEST -Wall

xopt_SOURCES = xopt.c xio.c xstring.c xmalloc.c xopt.h xio.h xstring.h xmalloc.h
xopt_CPPFLAGS = -D__XOPT_TEST -Wall

xtask_SOURCES = xtask.c rbtree.c xtask.h rbtree.h
xtask_CPPFLAGS = -D__XTASK_TEST -Wall

xepoll_SOURCES = xepoll.c xconf.c xopt.c xio.c xstring.c xmalloc.c xtask.c xlog.c rbtree.c xepoll.h xconf.h xopt.h xio.h xstring.h xmalloc.h xtask.h
xepoll_CPPFLAGS = -D__XEPOLL_TEST -Wall

xlist_SOURCES = xlist.c xmalloc.c xlist.h xmalloc.h
xlist_CPPFLAGS = -D__XLIST_TEST -Wall

xtpool_SOURCES = xtpool.c xmalloc.c xtpool.h xmalloc.h
xtpool_CPPFLAGS = -D__XTPOOL_TEST -Wall
xtpool_LDFLAGS = -lpthread

xdict_SOURCES = xdict.c xmalloc.c xdict.h xmalloc.h
xdict_CPPFLAGS = -D__XDICT_TEST -Wall

xshm_SOURCES = xshm.c xmalloc.c xlog.c
xshm_CPPFLAGS = -D__XSHM_TEST -Wall
xshm_LDFLAGS = -lpthread
