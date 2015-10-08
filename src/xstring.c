#include "xstring.h"
#include "xmalloc.h"
#include "xutil.h"

#include <stdlib.h>
#include <string.h>

xstring xstring_newlen(const void* init, size_t initlen) {
  xstring_hdr_t *sh;
  if (init) {
    sh = xmalloc(sizeof(xstring_hdr_t)+initlen+1);
  } else {
    sh = xcalloc(sizeof(xstring_hdr_t)+initlen+1);
  }
  sh->len = initlen;
  sh->size = initlen;
  if (init && initlen) {
    memcpy(sh->data, init, initlen);
  }
  sh->data[sh->len] = '\0';
  return (char*)sh->data;
}

xstring xstring_new(const char* init) {
  size_t initlen = (init == NULL) ? 0 : strlen(init);
  return xstring_newlen(init, initlen);
}

xstring xstring_empty(void) {
  return xstring_newlen("", 0);
}

xstring xstring_dup(xstring s) {
  return xstring_newlen(s, xstring_len(s));
}

void xstring_free(xstring s) {
  if (s == NULL) return;
  xfree(s-sizeof(xstring_hdr_t));
}

xstring xstring_makeroom(xstring s, size_t addlen) {
  xstring_hdr_t* sh = (void*)(s-sizeof(xstring_hdr_t));
  size_t free = sh->size - sh->len;
  if (free >= addlen) return s;

  size_t newsize = 25 * (xstring_len(s) + addlen) / 16;
  xstring_hdr_t* newsh = xrealloc(sh, sizeof(xstring_hdr_t) + newsize + 1);
  newsh->size = newsize;
  return (char*)newsh->data;
}

xstring xstring_catlen(xstring s, const void* t, size_t len) {
  s = xstring_makeroom(s, len);
  xstring_hdr_t* sh = (void*)(s-sizeof(xstring_hdr_t));
  memcpy(s + sh->len, t, len); 
  sh->len += len;
  sh->data[sh->len] = '\0';
  return s;
}

xstring xstring_cat(xstring s, const char* t) {
  return xstring_catlen(s, t, strlen(t));
}

xstring xstring_catxs(xstring s, xstring t) {
  return xstring_catlen(s, t, xstring_len(t));
}

xstring xstring_catfd(xstring s, int fd, unsigned len, int *res) {
  s = xstring_makeroom(s, len);
  xstring_hdr_t* sh = (void*)(s-sizeof(xstring_hdr_t));
  *res = read(fd, s + sh->len, len);
  if (*res <= 0) {
    sh->data[sh->len] = '\0';
    return s;
  }
  sh->len += *res;
  sh->data[sh->len] = '\0';
  return s;
}

xstring xstring_cpylen(xstring s, const void* t, size_t len) {
  xstring_hdr_t* sh = (void*)(s-sizeof(xstring_hdr_t));
  if (sh->size < len) {
    s = xstring_makeroom(s, len-sh->len);
    sh = (void*)(s-sizeof(xstring_hdr_t));
  }
  memcpy(s, t, len);
  sh->len = len;
  sh->data[sh->len] = '\0';
  return s;
}

xstring xstring_cpy(xstring s, const char* t) {
  return xstring_cpylen(s, t, strlen(t));
}

xstring xstring_cpyxs(xstring s, xstring t) {
  return xstring_cpylen(s, t, xstring_len(t));
}

xstring xstring_cpyfd(xstring s, int fd, unsigned len, int *res) {
  xstring_hdr_t* sh = (void*)(s-sizeof(xstring_hdr_t));
  if (sh->size < len) {
    s = xstring_makeroom(s, len-sh->len);
    sh = (void*)(s-sizeof(xstring_hdr_t));
  }
  *res = read(fd, s, len);
  if (*res <= 0) {
    xstring_clean(s);
    return s;
  }
  sh->len = *res;
  sh->data[sh->len] = '\0';
  return s;
}

void xstring_clean(xstring s) {
  xstring_hdr_t* sh = (void*)(s-sizeof(xstring_hdr_t));
  sh->len = 0;
  sh->data[sh->len] = '\0';
}

void xstring_strim(xstring s, const char* cset) {
  xstring_hdr_t* sh = (void*)(s-sizeof(xstring_hdr_t));
  char *start, *end, *sp, *ep;
  size_t len;

  sp = start = s;
  ep = end = s + xstring_len(s) - 1;
  while (sp <= end && strchr(cset, *sp)) sp++;
  while (ep > start && strchr(cset, *ep)) ep--;
  len = (sp > ep) ? 0 : ((ep-sp)+1);
  if (sh->data != sp) memmove(sh->data, sp, len);
  sh->len = len;
  sh->data[sh->len] = '\0';
}

void xstring_range(xstring s, int start, int end) {
  xstring_hdr_t* sh = (void*)(s - sizeof(xstring_hdr_t));
  size_t newlen, len = xstring_len(s);
  
  if (len == 0) return;
  if (start < 0) {
    start = len + start;
    if (start < 0) start = 0;
  }
  if (end < 0) {
    end = len + end;
    if (end < 0) end = 0;
  }
  newlen = (start > end) ? 0 : (end-start)+1;
  if (newlen != 0) {
    if (start >= (signed)len) {
      newlen = 0;
    } else if (end >= (signed)len) {
      end = len-1;
      newlen = (start > end) ? 0 : (end-start)+1;
    }
  } else {
    start = 0;
  }
  if (start && newlen) memmove(sh->data, sh->data+start, newlen);
  sh->data[newlen] = 0;
  sh->len = newlen;
}

int xstring_search(xstring s, const char *key) {
  ASSERT(key);
  if (!xstring_len(s)) return -1;
  char *point = strstr(s, key);
  if (!point) return -1;
  return point - s;
}

xstring* xstring_split(xstring s, const char* sep, int* count) {
  int start = 0, slots = 5;
  size_t total = xstring_len(s);
  xstring* token = xmalloc(sizeof(xstring)*slots);

  *count = 0;
  while (start < total) {
    char* point = strstr(s + start, sep);
    if (!point) break;
    if (point != s + start) {
      token[*count] = xstring_newlen(s + start, point - s - start);
      (*count)++;
      if (*count >= slots) {
        slots = slots * 3 / 2;
        token = xrealloc(token, sizeof(xstring)*slots);
      }
    }
    start = point - s + strlen(sep);
  }

  if (total != start) {
    token[*count] = xstring_newlen(s + start, total - start);
    (*count)++;
    if (*count >= slots) {
        slots = slots * 3 / 2;
        token = xrealloc(token, sizeof(xstring)*slots);
    }
  }
  return token;
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

  xstring_strim(s, "inc ");
  XTEST_STRING_EQ(s, "totherab");

  s = xstring_cpy(s, "abcdefghijklmnopqrst");
  XTEST_EQ(xstring_len(s), 20);
  XTEST_STRING_EQ(s, "abcdefghijklmnopqrst");

  xstring_range(s, 3, 4);
  XTEST_STRING_EQ(s, "de");

  s = xstring_catxs(s, s);
  XTEST_STRING_EQ(s, "dede");

  xstring_clean(s);
  XTEST_STRING_EQ(s, "");

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
