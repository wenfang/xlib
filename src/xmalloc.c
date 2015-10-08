#include "xmalloc.h"
#include "xlog.h"

#include <stdlib.h>

void* xmalloc(size_t size) {
  void* ptr = malloc(size);
  if (ptr == NULL) {
    XLOG_ERR("malloc size: %d error", size);
    abort();
  }
  return ptr;
}

void* xcalloc(size_t size) {
  void* ptr = calloc(1, size);
  if (ptr == NULL) {
    XLOG_ERR("calloc size: %d error", size);
    abort();
  }
  return ptr;
}

void* xrealloc(void* ptr, size_t size) {
  void* realptr = realloc(ptr, size);
  if (realptr == NULL) {
    XLOG_ERR("realptr size: %d error", size);
    abort();
  }
  return realptr;
}

void xfree(void* ptr) {
  free(ptr);
}
