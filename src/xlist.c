#include "xlist.h"
#include "xmalloc.h"

xlist* xlist_new(void) {
  xlist *list = xmalloc(sizeof(xlist));
  if (list == NULL) return NULL;
  list->head = list->tail = NULL;
  list->len = 0;
  return list;
}

void xlist_free(xlist *list) {
  unsigned int len = list->len;
  xlistNode *current, *next;
  
  current = list->head;
  while (len--) {
    next = current->next; 
    if (list->free) list->free(current->value);
    current = next;
  }
  xfree(list);
}

#ifdef __XLIST_TEST

#include "xunittest.h"

int main(void) {
  return 0;
}

#endif
