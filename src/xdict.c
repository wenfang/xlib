#include "xdict.h"
#include "xmalloc.h"

static void _clear(xdict *d) {
  for (unsigned long i=0; i<d->size && d->used>0; i++) {
    xdictEntry *he, *nextHe;
    if ((he = d->table[i]) == NULL) continue;
    while (he) {
      nextHe = he->next;
      xdictFreeEntryKey(d, he);
      xdictFreeEntryVal(d, he);
      xfree(he);
      d->used--;
      he = nextHe;   
    }
  }
}

static unsigned long _nextPower(unsigned long size) {
  unsigned long i = XDICT_INITIAL_SIZE;
  while (1) {
    if (i >= size) return i;
    i *= 2;
  }
}

static int _expand_real(xdict *d, unsigned long size) {
  if (d->used > size) return XDICT_ERR;

  unsigned long realsize = _nextPower(size);
  xdict n;
  n.table = xcalloc(realsize * sizeof(xdictEntry*));
  n.type = d->type;
  n.size = realsize;
  n.sizemask = realsize - 1;
  n.used = d->used;
  n.privdata = d->privdata;

  for(int i=0; i<d->size && d->used>0; i++) {
    xdictEntry *he, *nextHe;
    if (d->table[i] == NULL) continue;
    he = d->table[i];
    while (he) {
      unsigned int h;
      nextHe = he->next;

      h = xdictHashKey(d, he->key) & n.sizemask;
      he->next = n.table[h];
      d->used--;
      he = nextHe;
    }
  }
  xfree(d->table);
  *d = n;
  return XDICT_OK;
}

static int _expand(xdict *d) {
  if (d->size == 0) {
    return _expand_real(d, XDICT_INITIAL_SIZE);
  }
  if (d->used == d->size) {
    return _expand_real(d, d->size*2);
  }
  return XDICT_OK;
}

static int _keyIndex(xdict *d, const void *key) {
  unsigned int h;
  xdictEntry *he;

  if (_expand(d) == XDICT_ERR) return -1;
  h = xdictHashKey(d, key) & d->sizemask;
  he = d->table[h];
  while (he) {
    if (xdictCompareHashKeys(d, key, he->key)) return -1;
    he = he->next;
  }
  return h;
}

int xdict_add(xdict *d, void *key, void *val) {
  int index;
  xdictEntry *entry;

  if ((index = _keyIndex(d, key)) == -1) return XDICT_ERR;
  entry = xmalloc(sizeof(*entry));
  entry->next = d->table[index];
  d->table[index] = entry;

  xdictSetHashKey(d, entry, key);
  xdictSetHashVal(d, entry, val);
  d->used++;
  return XDICT_OK; 
}

int xdict_set(xdict *d, void *key, void *val) {
  if (xdict_add(d, key, val) == XDICT_OK) return 1;
  xdictEntry *entry = xdict_find(d, key);
  xdictEntry auxentry = *entry;
  xdictSetHashVal(d, entry, val);
  xdictFreeEntryVal(d, &auxentry);
  return 0; 
}

int xdict_del(xdict *d, void *key) {
  if (d->size == 0) return XDICT_ERR;
  unsigned int h = xdictHashKey(d, key) & d->sizemask;
  xdictEntry *de = d->table[h];
  xdictEntry *prevde = NULL;

  while (de) {
    if (xdictCompareHashKeys(d, key, de->key)) {
      if (prevde) {
        prevde->next = de->next;
      } else {
        d->table[h] = de->next;
      }
      xdictFreeEntryKey(d, de);
      xdictFreeEntryVal(d, de);
      xfree(de);
      d->used--;
      return XDICT_OK;
    }
    prevde = de;
    de = de->next;
  }
  return XDICT_ERR;
}

xdictEntry* xdict_find(xdict *d, const void *key) {
  if (d->size == 0) return NULL; 
  unsigned int h = xdictHashKey(d, key) & d->sizemask;
  xdictEntry *he = d->table[h];
  while (he) {
    if (xdictCompareHashKeys(d, key, he->key)) return he;
    he = he->next;
  }
  return NULL;
}

xdictIterator* xdict_newIterator(xdict *d) {
  xdictIterator *iter = xmalloc(sizeof(*iter));
  iter->d = d;
  iter->index = -1;
  iter->entry = NULL;
  iter->nextEntry = NULL;
  return iter;
}

xdictEntry* xdict_next(xdictIterator *iter) {
  while (1) {
    if (iter->entry == NULL) {
      iter->index++;
      if (iter->index >= (signed)iter->d->size) break;
      iter->entry = iter->d->table[iter->index];
    } else {
      iter->entry = iter->nextEntry;
    }
    if (iter->entry) {
      iter->nextEntry = iter->entry->next;
      return iter->entry;
    }
  }
  return NULL;
}

void xdict_freeIterator(xdictIterator *iter) {
  xfree(iter);
}

unsigned int xdict_genHashFunction(const unsigned char *buf, int len) {
  unsigned int hash = 5381;
  
  while (len--) hash = ((hash << 5) + hash) + (*buf++);
  return hash;
}

xdict* xdict_new(xdictType *type, void *privDataPtr) {
  xdict *d = xmalloc(sizeof(xdict));
  d->table = NULL;
  d->type = type;
  d->size = 0;
  d->sizemask = 0;
  d->used = 0;
  d->privdata = privDataPtr;
  return d;
}

void xdict_free(xdict *d) {
  _clear(d);
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
