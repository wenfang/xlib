#include "xlist.h"
#include "xmalloc.h"

xlist* xlist_new(void) {
  xlist *list = xmalloc(sizeof(xlist));
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

xlist* xlist_addNodeHead(xlist *list, void *value) {
  xlistNode *node;
  node = xmalloc(sizeof(xlistNode));
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

xlist* xlist_addNodeTail(xlist *list, void *value) {
  xlistNode *node;
  node = xmalloc(sizeof(xlistNode));
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

xlist* xlist_insertNode(xlist *list, xlistNode *old_node, void *value, int after) {
  xlistNode *node;
  node = xmalloc(sizeof(xlistNode));
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

void xlist_delNode(xlist *list, xlistNode *node) {
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
  xlistIter *iter = xmalloc(sizeof(xlistIter));
  if (direction == XLIST_START_HEAD) {
    iter->next = list->head;
  } else {
    iter->next = list->tail;
  }
  iter->direction = direction;
  return iter;
}

xlistNode* xlist_next(xlistIter *iter) {
  xlistNode *current = iter->next;
  if (current != NULL) {
    if (iter->direction == XLIST_START_HEAD) {
      iter->next = current->next;
    } else {
      iter->next = current->prev;
    }
  }
  return current;
}

void xlist_freeIterator(xlistIter *iter) {
  xfree(iter);
}

void xlist_rewind(xlist *list, xlistIter *li) {
  li->next = list->head;
  li->direction = XLIST_START_HEAD;
}

void xlist_rewindTail(xlist *list, xlistIter *li) {
  li->next = list->tail;
  li->direction = XLIST_START_TAIL;
}

#ifdef __XLIST_TEST

#include "xunittest.h"

int main(void) {
  xlist *list = xlist_new();
  list = xlist_addNodeHead(list, NULL);
  list = xlist_insertNode(list, xlistFirst(list), NULL, 1);
  xlist_free(list);
  return 0;
}

#endif
