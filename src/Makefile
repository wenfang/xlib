
.c.o:
	gcc -DDEBUG -g -fPIC -std=gnu99 -I/usr/local/include -pthread -Wall -c $< -o $@

all:$(OBJS) $(LUA_DIR)/src/liblua.a
	gcc -g $(OBJS) $(LUA_DIR)/src/liblua.a -ldl -lm -lpthread -o spe

xstring: xstring.c xmalloc.c xstring.h xmalloc.h
	gcc -g -D__XSTRING_TEST xstring.c xmalloc.c -o xstring

xlist: xlist.c xlist.h xmalloc.c xmalloc.h
	gcc -g -D__XLIST_TEST xlist.c xmalloc.c -o xlist

clean:
	rm -rf *.o
	rm -rf xstring xlist

.PHONY: all xstring xlist clean
