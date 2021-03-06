#ifndef __XSTRING_H
#define __XSTRING_H

#include <stdio.h>
#include <stdarg.h>

typedef char* xstring;

typedef struct xstring_hdr_s {
  unsigned int  len;
  unsigned int  size;
  char          data[];
} xstring_hdr_t;

static inline size_t xstring_len(xstring s) {
  xstring_hdr_t* sh = (void*)(s - sizeof(xstring_hdr_t));
  return sh->len;
}

xstring xstring_newlen(const void* init, size_t initlen);
xstring xstring_new(const char* init);
xstring xstring_empty(void);
xstring xstring_dup(xstring s);
void xstring_free(xstring s);

xstring xstring_makeroom(xstring s, size_t addlen);

xstring xstring_catlen(xstring s, const void* t, size_t len);
xstring xstring_cat(xstring s, const char* t);
xstring xstring_catxs(xstring s, xstring t);
xstring xstring_catfd(xstring s, int fd, unsigned len, int *res);
xstring xstring_catvprintf(xstring s, const char *fmt, va_list ap);
xstring xstring_catprintf(xstring s, const char *fmt, ...);

xstring xstring_cpylen(xstring s, const void* t, size_t len);
xstring xstring_cpy(xstring s, const char* t);
xstring xstring_cpyxs(xstring s, xstring t);
xstring xstring_cpyfd(xstring s, int fd, unsigned len, int *res);

void xstring_clean(xstring s);
void xstring_strim(xstring s, const char* cset);
void xstring_range(xstring s, int start, int end);

int xstring_search(xstring s, const char *key);

xstring* xstring_split(xstring s, const char* sep, int* count);
void xstrings_free(xstring* s, int count);

#endif
