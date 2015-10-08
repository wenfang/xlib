#include "xdict.h"
#include "xmalloc.h"

static void xdict_reset(xdict *d) {
  d->table = NULL;
  d->size = 0;
  d->sizemask = 0;
  d->used = 0;
}

static void xdict_clear(xdict *d) {
  unsigned long i;

  for (i=0; i<d->size && d->used>0; i++) {
    xdictEntry *he, *nextHe;
    if ((he = d->table[i]) == NULL) continue;
    while(he) {
      nextHe = he->next;
      xdictFreeEntryKey(d, he);
      xdictFreeEntryVal(d, he);
      xfree(he);
      d->used--;
      he = nextHe;   
    }
  }
}

static int xdict_expandReal(xdict *d, unsigned long size) {
}

static int xdict_expand(xdict *d) {
  if (d->size == 0) {
    return xdict_expandReal(d, XDICT_INITIAL_SIZE);
  }
  if (d->used == d->size) {
    return xdict_expandReal(d, d->size*2);
  }
  return XDICT_OK;
}

static void xdict_keyIndex(xdict *d, const void *key) {
  unsigned int h;
  xdictEntry *he;

}

int xdict_add(xdict *d, void *key, void *value) {
  int index;
  xdictEntry *entry;

  return XDICT_OK; 
}


xdict* xdict_new(xdictType *type, void *privDataPtr) {
  xdict* d = xmalloc(sizeof(xdict));
  if (d == NULL) return NULL;

  xdict_reset(d);
  d->type = type;
  d->privdata = privDataPtr;
  return d;
}

void xdict_free(xdict *d) {
  xdict_clear(d);
  xfree(d);
}

#ifdef __XDICT_TEST

#include "xunittest.h"

int main(void) {
  xdict *dict = xdict_new(NULL, NULL);
  xdict_free(dict);
  return 0;
}

#endif
