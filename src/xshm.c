#include "xshm.h"
#include "xmalloc.h"
#include "xlog.h"
#include "xutil.h"
#include <stdlib.h>
#include <sys/mman.h>

xshm_t* xshm_new(unsigned size) {
  xshm_t *shm = xcalloc(sizeof(xshm_t));
  if (shm == NULL) {
    XLOG_ERR("xshm xcalloc error");
    return NULL;
  }
  shm->_addr = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, -1, 0);
  if (!shm->_addr) {
    XLOG_ERR("xshm mmap error");
    xfree(shm);
    return NULL; 
  } 
  shm->_size = size;
  return shm;
}

void xshm_free(xshm_t *shm) {
  ASSERT(shm);
  if (munmap(shm->_addr, shm->_size) == -1) {
    XLOG_ERR("xshm munmap error");
  }
  xfree(shm);
}

pthread_mutex_t* xshm_mutex_new(void) {
  pthread_mutex_t* shmux = mmap(NULL, sizeof(pthread_mutex_t), PROT_READ|PROT_WRITE,
      MAP_ANON|MAP_SHARED, -1, 0);
  if (!shmux) {
    XLOG_ERR("xshm_mutex mmap error");
    return NULL;
  }
  pthread_mutexattr_t mattr;
  pthread_mutexattr_init(&mattr);
  pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
  pthread_mutex_init(shmux, &mattr);
  return shmux;
}

void xshm_mutex_free(pthread_mutex_t* shmux) {
  ASSERT(shmux);
  pthread_mutex_destroy(shmux);
  if (munmap(shmux, sizeof(pthread_mutex_t)) == -1) {
    XLOG_ERR("xshm_mutex munmap error");
  }
}

#ifdef __XSHM_TEST

int main(void) {
  return 0;
}

#endif
