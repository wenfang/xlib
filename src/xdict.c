#include "xdict.h"
#include "xmalloc.h"

static void xdict_reset(xdictht *ht) {
  ht->table = NULL;
  ht->size = 0;
  ht->sizemask = 0;
  ht->used = 0;
}

int xdict_add(xdict* d, void* key, void* value) {
}


xdict* xdict_new(void) {
  xdict* d = zmalloc(sizeof(xdict));
  if (d == NULL) return NULL;

  xdict_reset(&d->ht[0]);
  xdict_reset(&d->ht[1]);
  d->rehashidx = -1;
  d->iterators = 0;
  return d;
}
