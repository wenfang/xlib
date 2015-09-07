#ifndef __XSTRING_H
#define __XSTRING_H

#include <stdio.h>

typedef char* xstring;

typedef struct xstring_hdr {
  unsigned int  len;
  unsigned int  size;
  char          data[];
} xstring_hdr;

static inline size_t xstring_len(xstring s) {
  xstring_hdr* sh = (void*)(s - sizeof(xstring_hdr));
  return sh->len;
}

xstring xstring_newlen(const void* init, size_t initlen);
xstring xstring_new(const char* init);
xstring xstring_empty(void);
xstring xstring_dup(xstring s);
void xstring_free(xstring s);

xstring xstring_catlen(xstring s, const void* t, size_t len);

#endif
