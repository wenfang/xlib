#include "xlist.h"
#include "xmalloc.h"

xlist* xlist_new(void) {
  xlist *list = xmalloc(sizeof(xlist));
  if (list == NULL) return NULL;
  list->head = list->tail = NULL;
  list->len = 0;
  list->free = NULL;
  return list;
}

void xlist_free(xlist *list) {
  unsigned int len = list->len;
  xlistNode *current, *next;
  
  current = list->head;
  while (len--) {
    next = current->next; 
    if (list->free) list->free(current->value);
    xfree(current);
    current = next;
  }
  xfree(list);
}

xlist* xlist_AddNodeHead(xlist *list, void *value) {
  xlistNode *node;
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

xlist* xlist_AddNodeTail(xlist *list, void *value) {
  xlistNode *node;
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

xlist* xlist_InsertNode(xlist *list, xlistNode *old_node, void *value, int after) {
  xlistNode *node;
  node = xmalloc(sizeof(xlistNode));
  if (node == NULL) return NULL;
  node->value = value;

  if (after) {
    node->prev = old_node;
    node->next = old_node->next;
    if (list->tail == old_node) {
      list->tail = node;
    }
  } else {
    node->next = old_node;
    node->prev = old_node->prev;
    if (list->head == old_node) {
      list->head = node;
    }
  }

  if (node->prev != NULL) {
    node->prev->next = node;
  }
  if (node->next != NULL) {
    node->next->prev = node;
  }
  list->len++;
  return list;
}

void xlist_DelNode(xlist *list, xlistNode *node) {
  if (node->prev) {
    node->prev->next = node->next;
  } else {
    list->head = node->next;
  }

  if (node->next) {
    node->next->prev = node->prev;
  } else {
    list->tail = node->prev;
  }
}

xlistIter* xlist_newIterator(xlist *list, int direction) {
}

#ifdef __XLIST_TEST

#include "xunittest.h"

int main(void) {
  xlist *list = xlist_new();
  list = xlist_AddNodeHead(list, NULL);
  list = xlist_InsertNode(list, xlistFirst(list), NULL, 1);
  xlist_free(list);
  return 0;
}

#endif
