#include "xmalloc.h"

#include <stdlib.h>

void* xmalloc(size_t size) {
  void* ptr = malloc(size);
  return ptr;
}

void* xcalloc(size_t size) {
  void* ptr = calloc(1, size);
  return ptr;
}

void* xrealloc(void* ptr, size_t size) {
  void* realptr = realloc(ptr, size);
  return realptr;
}

void xfree(void* ptr) {
  free(ptr);
}
