
.c.o:
	gcc -DDEBUG -g -fPIC -std=gnu99 -I/usr/local/include -pthread -Wall -c $< -o $@

all:$(OBJS) $(LUA_DIR)/src/liblua.a
	gcc -g $(OBJS) $(LUA_DIR)/src/liblua.a -ldl -lm -lpthread -o spe

xstring: xstring.c xmalloc.c xstring.h xmalloc.h
	gcc -D__XSTRING_TEST xstring.c xmalloc.c -o xstring

clean:
	rm -rf *.o
	rm -rf xstring

.PHONY: all xstring clean
