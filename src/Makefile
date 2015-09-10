FLAGS=-g -std=gnu99

TEST_BIN=xstring xlist xtask xepoll xio

.c.o:
	gcc -DDEBUG -g -fPIC -std=gnu99 -I/usr/local/include -pthread -Wall -c $< -o $@

test: $(TEST_BIN)

xstring: xstring.c xmalloc.c xstring.h xmalloc.h
	gcc $(FLAGS) -D__XSTRING_TEST xstring.c xmalloc.c -o xstring

xlist: xlist.c xlist.h xmalloc.c xmalloc.h
	gcc $(FLAGS) -D__XLIST_TEST xlist.c xmalloc.c -o xlist

xtask: xtask.c xtask.h rbtree.c rbtree.h
	gcc $(FLAGS) -D__XTASK_TEST xtask.c rbtree.c -o xtask

xepoll: xepoll.c xepoll.h xlog.c xlog.h xtask.c xtask.h xmalloc.c xmalloc.h
	gcc $(FLAGS) -D__XEPOLL_TEST xepoll.c xlog.c xtask.c -o xepoll

xio: xio.c xio.h xmalloc.c xmalloc.h xstring.c xstring.h
	gcc $(FLAGS) -D__XIO_TEST xio.c xmalloc.c xstring.c -o xio

clean:
	rm -rf *.o
	rm -rf $(TEST_BIN)

.PHONY: test $(TEST_BIN) clean
