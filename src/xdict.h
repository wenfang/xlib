#ifndef __XDICT_H
#define __XDICT_H

#define XDICT_OK  0
#define XDICT_ERR 1

typedef struct xdictEntry {
  void *key;
  void *val;
  struct xdictEntry *next;
} xdictEntry;

typedef struct xdictType {
  unsigned int (*hashFunction)(const void *key);
  void *(*keyDup)(void *privdata, const void *key);
  void *(*valDup)(void *privdata, const void *obj);
  int (*keyCompare)(void *privdata, const void *key1, const void *key2);
  void (*keyDestructor)(void *privdata, void *key);
  void (*valDestructor)(void *privdata, void *obj); 
} xdictType;

typedef struct xdict {
  xdictEntry **table;
  xdictType *type;
  unsigned long size;
  unsigned long sizemask;
  unsigned long used; 
  void *privdata;
} xdict;

typedef struct xdictIterator {
  xdict *d;
  int index;
  xdictEntry *entry, *nextEntry;
} xdictIterator;

#define XDICT_INITIAL_SIZE 4

#define xdictFreeEntryVal(ht, entry) \
    if ((ht)->type->valDestructor) \
        (ht)->type->valDestructor((ht)->privdata, (entry)->val)

#define xdictSetHashVal(ht, entry, _val_) do { \
    if ((ht)->type->valDup) \
        entry->val = (ht)->type->valDup((ht)->privdata, _val_); \
    else \
        entry->val = (_val_); \
} while(0)

#define xdictFreeEntryKey(ht, entry) \
    if ((ht)->type->keyDestructor) \
        (ht)->type->keyDestructor((ht)->privdata, (entry)->key)

#define xdictSetHashKey(ht, entry, _key_) do { \
    if ((ht)->type->keyDup) \
        entry->key = (ht)->type->keyDup((ht)->privdata, _key_); \
    else \
        entry->key = (_key_); \
} while(0)

#define xdictCompareHashKeys(ht, key1, key2) \
    (((ht)->type->keyCompare) ? \
        (ht)->type->keyCompare((ht)->privdata, key1, key2) : \
        (key1) == (key2))

#define xdictHashKey(ht, key) (ht)->type->hashFunction(key)

#define xdictGetEntryKey(he) ((he)->key)
#define xdictGetEntryVal(he) ((he)->val)
#define xdictSlots(ht) ((ht)->size)
#define xdictSize(ht) ((ht)->used)

unsigned int xdict_genHashFunction(const unsigned char *buf, int len);
xdict* xdict_new(xdictType *type, void *privDataPtr);
void xdict_free(xdict *d);

int xdict_add(xdict *d, void *key, void *val);
int xdict_set(xdict *d, void *key, void *val);
int xdict_del(xdict *d, void *key);

xdictEntry* xdict_find(xdict *d, const void *key);
xdictIterator* xdict_newIterator(xdict *d);
xdictEntry* xdict_next(xdictIterator *iter);
void xdict_freeIterator(xdictIterator *iter);

#endif
