#ifndef __XHANDLER_H
#define __XHANDLER_H

typedef void (*xHandler)(void*, void*);

typedef struct xhandler_s {
  xHandler  handler;
  void      *arg1;
  void      *arg2;
} __attribute__((aligned(sizeof(long)))) xhandler;


#define XHANDLER_EMPTY                (xhandler){NULL, NULL, NULL}
#define XHANDLER(handler, arg1, arg2) (xhandler){handler, arg1, arg2}

#define XHANDLER_CALL(h)                \
  do {                                  \
    if ((h).handler) {                  \
      (h).handler((h).arg1, (h).arg2);  \
    }                                   \
  } while(0)                            

#endif
