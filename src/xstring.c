#include "xstring.h"
#include "xmalloc.h"

#include <stdlib.h>
#include <string.h>

xstring
xstring_newlen(const void* init, size_t initlen) {
  xstring_hdr *sh;
  if (init) {
    sh = xmalloc(sizeof(xstring_hdr) + initlen + 1);
  } else {
    sh = xcalloc(sizeof(xstring_hdr) + initlen + 1);
  }
  if (sh == NULL) return NULL;
  sh->len = initlen;
  sh->size = initlen;
  if (init && initlen) {
    memcpy(sh->data, init, initlen);
  }
  sh->data[sh->len] = '\0';
  return (char*)sh->data;
}

xstring
xstring_new(const char* init) {
  size_t initlen = (init == NULL) ? 0 : strlen(init);
  return xstring_newlen(init, initlen);
}

xstring
xstring_empty(void) {
  return xstring_newlen("", 0);
}

xstring
xstring_dup(xstring s) {
  return xstring_newlen(s, xstring_len(s));
}

void
xstring_free(xstring s) {
  if (s == NULL) return;
  xfree(s - sizeof(xstring_hdr));
}

static inline size_t xstring_avail(xstring s) {
  xstring_hdr* sh = (void*)(s - sizeof(xstring_hdr));
  return sh->size - sh->len;
}

static xstring
grow(xstring s, size_t addlen) {
  size_t free = xstring_avail(s);
  if (free >= addlen) return s;

  xstring_hdr* sh = (void*)(s - sizeof(xstring_hdr));
  size_t newsize = 25 * (xstring_len(s) + addlen) / 16;
  xstring_hdr* newsh = xrealloc(sh, sizeof(xstring_hdr) + newsize + 1);
  if (newsh == NULL) return NULL;
  newsh->size = newsize;
  return (char*)newsh->data;
}

xstring xstring_catlen(xstring s, const void* t, size_t len) {
  s = grow(s, len);
  if (s == NULL) return NULL;

  xstring_hdr* sh = (void*)(s - sizeof(xstring_hdr));
  memcpy(s + sh->len, t, len); 
  sh->len += len;
  sh->data[sh->len] = '\0';
  return s;
}

#ifdef __XSTRING_TEST

#include "xunittest.h"

int main(void) {
  xstring s = xstring_new("init");
  XTEST_EQ(xstring_len(s), 4);  

  xstring_catlen(s, "other", 5);
  XTEST_EQ(xstring_len(s), 9);
  XTEST_STRING_EQ(s, "initother"); 

  return 0;
}

#endif
