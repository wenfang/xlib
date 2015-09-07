#ifndef __XDICT_H
#define __XDICT_H

typedef struct xdictEntry {
  void* key;
  union {
    void *val;
    uint64_t u64;
    int64_t s64;
    double d;
  } v;
  struct xdictEntry *next;
} xdictEntry;

typedef struct xdictht {
  xdictEntry **table;
  unsigned long size;
  unsigned long sizemask;
  unsigned long used; 
} xdictht;

typedef struct xdict {
  xdictht ht[2];
  long rehashidx;
  int iterators;
} xdict;

xdict* xdict_new(void);
void xdict_free(xdict* dict);

int xdict_add(xdict* d, void* key, void* value);

#endif
