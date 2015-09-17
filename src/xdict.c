#include "xdict.h"
#include "xmalloc.h"

static void xdict_reset(xdict *ht) {
  ht->table = NULL;
  ht->size = 0;
  ht->sizemask = 0;
  ht->used = 0;
}

int xdict_add(xdict* d, void* key, void* value) {
}


xdict* xdict_new(xdictType *type, void *privDataPtr) {
  xdict* d = xmalloc(sizeof(xdict));
  if (d == NULL) return NULL;

  xdict_reset(d);
  d->type = type;
  d->privdate = privDataPtr;
  return d;
}

#ifdef __XDICT_TEST

#include "xunittest.h"

int main(void) {
  return 0;
}

#endif
