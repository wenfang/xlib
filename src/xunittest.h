#ifndef __XUNITTEST_H
#define __XUNITTEST_H

#include <stdio.h>
#include <string.h>

#define XTEST_ASSERT(cond)                                  \
  do {                                                      \
    if (!(cond)) {                                          \
      printf("[XTEST ERROR] %s:%d\n", __FILE__, __LINE__);  \
    }                                                       \
  } while(0)  

#define XTEST_EQ(x, y) XTEST_ASSERT(x == y)
#define XTEST_NOT_EQ(x, y) XTEST_ASSERT(!(x == y))

#define XTEST_LT(x, y) XTEST_ASSERT(x > y)
#define XTEST_ST(x, y) XTEST_ASSERT(x < y)

#define XTEST_STRING_EQ(x, y)           \
  if (x != NULL && y != NULL) {         \
    XTEST_ASSERT(!strcmp(x, y));        \
  } else if (x == NULL && y == NULL) {  \
    XTEST_ASSERT(1);                    \
  } else {                              \
    XTEST_ASSERT(0);                    \
  }

#endif
