#ifndef __XMALLOC_H
#define __XMALLOC_H

#include <stdio.h>

void* xmalloc(size_t size);
void* xcalloc(size_t size);
void* xrealloc(void* ptr, size_t size);
void xfree(void* ptr);

#endif
