FLAGS=-g -std=gnu99

XMALLOC=xmalloc.c xmalloc.h
XSTRING=xstring.c xstring.h

.c.o:
	gcc -DDEBUG -g -fPIC -std=gnu99 -I/usr/local/include -pthread -Wall -c $< -o $@

all:$(OBJS) $(LUA_DIR)/src/liblua.a
	gcc -g $(OBJS) $(LUA_DIR)/src/liblua.a -ldl -lm -lpthread -o spe

xstring: xstring.c xmalloc.c xstring.h xmalloc.h
	gcc $(FLAGS) -D__XSTRING_TEST xstring.c xmalloc.c -o xstring

xlist: xlist.c xlist.h xmalloc.c xmalloc.h
	gcc $(FLAGS) -D__XLIST_TEST xlist.c xmalloc.c -o xlist

xtask: xtask.c xtask.h rbtree.c rbtree.h
	gcc $(FLAGS) -D__XTASK_TEST xtask.c rbtree.c -o xtask

xepoll: xepoll.c xepoll.h xlog.c xlog.h xtask.c xtask.h xmalloc.c xmalloc.h
	gcc $(FLAGS) -D__XEPOLL_TEST xepoll.c xlog.c xtask.c -o xepoll

clean:
	rm -rf *.o
	rm -rf xstring xlist xtask

.PHONY: all xstring xlist xtask clean
