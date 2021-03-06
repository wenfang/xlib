#ifndef __XLIST_H
#define __XLIST_H

typedef struct xlistNode {
  struct xlistNode  *prev;
  struct xlistNode  *next;
  void *value;
} xlistNode;

typedef struct xlistIter {
  xlistNode *next;
  int direction;
} xlistIter;

typedef struct xlist {
  xlistNode *head;
  xlistNode *tail;
  void (*free)(void *ptr);
  unsigned long len;
} xlist;

#define xlistLength(I) ((I)->len)
#define xlistFirst(I) ((I)->head)
#define xlistLast(I) ((I)->tail)
#define xlistPrevNode(n) ((n)->prev)
#define xlistNextNode(n) ((n)->next)
#define xlistNodeValue(n) ((n)->value)

xlist* xlist_new(void);
void xlist_free(xlist *list);

xlist* xlist_addNodeHead(xlist *list, void *value);
xlist* xlist_addNodeTail(xlist *list, void *value);
xlist* xlist_insertNode(xlist *list, xlistNode *old_node, void *value, int after);
void xlist_delNode(xlist *list, xlistNode *node);

xlistIter* xlist_newIterator(xlist *list, int direction);
xlistNode* xlist_next(xlistIter *iter);
void xlist_freeIterator(xlistIter *iter);
void xlist_rewind(xlist *list, xlistIter *li);
void xlist_rewindTail(xlist *list, xlistIter *li);

#define XLIST_START_HEAD  0
#define XLIST_START_TAIL  1

#endif
