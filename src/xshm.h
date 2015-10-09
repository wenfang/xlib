#ifndef __XSHM_H
#define __XSHM_H

#include <pthread.h>

typedef struct xshm_s {
  void      *addr;
  unsigned  _size;
} xshm __attribute__((aligned(sizeof(long))));

xshm* xshm_new(unsigned size);
void xshm_free(xshm *shm);

pthread_mutex_t* xshm_mutex_new(void);
void xshm_mutex_free(pthread_mutex_t* shmux);

#endif
