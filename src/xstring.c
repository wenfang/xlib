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

xstring xstring_cat(xstring s, const char* t) {
  return xstring_catlen(s, t, strlen(t));
}

xstring xstring_cpylen(xstring s, const void* t, size_t len) {
  xstring_hdr* sh = (void*)(s - sizeof(xstring_hdr));
  if (sh->size < len) {
    s = grow(s, len - sh->len);
    if (s == NULL) return NULL;
    sh = (void*)(s - sizeof(xstring_hdr));
  }
  memcpy(s, t, len);
  sh->len = len;
  sh->data[sh->len] = '\0';
  return s;
}

xstring xstring_cpy(xstring s, const char* t) {
  return xstring_cpylen(s, t, strlen(t));
}

xstring* xstring_split(xstring s, const char* sep, int* count) {
  int slots = 5;
  xstring* token = xmalloc(sizeof(xstring)*slots);
  if (token == NULL) return token;

  xstring* newtoken;
  int i, start = 0;
  size_t total = xstring_len(s);

  *count = 0;
  while (start < total) {
    char* point = strstr(s + start, sep);
    if (!point) break;
    if (point != s + start) {
      token[*count] = xstring_newlen(s + start, point - s - start);
      (*count)++;
      if (*count >= slots) {
        slots = slots * 3 / 2;
        newtoken = xrealloc(token, sizeof(xstring)*slots);
        if (newtoken == NULL) goto err_out;
        token = newtoken;
      }
    }
    start = point - s + strlen(sep);
  }

  if (total != start) {
    token[*count] = xstring_newlen(s + start, total - start);
    (*count)++;
    if (*count >= slots) {
        slots = slots * 3 / 2;
        newtoken = xrealloc(token, sizeof(xstring)*slots);
        if (newtoken == NULL) goto err_out;
        token = newtoken;
    }
  }
  return token;

err_out:
  for (i=0; i<*count; i++) {
    xstring_free(token[*count]);
  }
  xfree(token);
  *count = 0;
  return NULL; 
}

void xstrings_free(xstring* s, int count) {
  int i;
  for (i=0; i<count; i++) {
    xstring_free(s[i]);
  }
  xfree(s);
}

#ifdef __XSTRING_TEST

#include "xunittest.h"

int main(void) {
  xstring s = xstring_new("init");
  XTEST_EQ(xstring_len(s), 4);  

  s = xstring_catlen(s, "other", 5);
  XTEST_EQ(xstring_len(s), 9);
  XTEST_STRING_EQ(s, "initother"); 

  s = xstring_cat(s, "abc");
  XTEST_EQ(xstring_len(s), 12);
  XTEST_STRING_EQ(s, "initotherabc");

  s = xstring_cpy(s, "abcdefghijklmnopqrst");
  XTEST_EQ(xstring_len(s), 20);
  XTEST_STRING_EQ(s, "abcdefghijklmnopqrst");

  s = xstring_cpy(s, " this is    a test another   string");

  xstring* ss;
  int count;
  ss = xstring_split(s, " ", &count);
  XTEST_EQ(count, 6);
  XTEST_STRING_EQ(ss[0], "this");
  XTEST_STRING_EQ(ss[1], "is");
  XTEST_STRING_EQ(ss[2], "a");
  XTEST_STRING_EQ(ss[3], "test");
  XTEST_STRING_EQ(ss[4], "another");
  XTEST_STRING_EQ(ss[5], "string");

  xstrings_free(ss, count);
  xstring_free(s);
  return 0;
}

#endif
