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

xlist* xlist_AddNodeHead(xlist* list, void *value) {
  xlistNode* node;
  node = xmalloc(sizeof(xlistNode));
  if (node == NULL) return NULL; 
  node->value = value;
  
  if (list->len == 0) {
    list->head = list->tail = node;
    node->prev = node->next = NULL;
  } else {
    node->prev = NULL;
    node->next = list->head;
    list->head->prev = node;
    list->head = node;
  }
  list->len++;
  return list;
}

xlist* xlist_AddNodeTail(xlist* list, void *value) {
  xlistNode* node;
  node = xmalloc(sizeof(xlistNode));
  if (node == NULL) return NULL; 
  node->value = value;

  if (list->len == 0) {
    list->head = list->tail = node;
    node->prev = node->next = NULL;
  } else {
    node->prev = list->tail;
    node->next = NULL;
    list->tail->next = node;
    list->tail = node;
  }
  list->len++;
  return list;
}

#ifdef __XLIST_TEST

#include "xunittest.h"

int main(void) {
  return 0;
}

#endif
