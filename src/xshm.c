#include "xshm.h"
#include "xmalloc.h"
#include "xlog.h"

#include <stdlib.h>
#include <sys/mman.h>

xshm* xshm_new(unsigned size) {
  xshm *shm = xcalloc(sizeof(xshm));
  shm->addr = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, -1, 0);
  if (!shm->addr) {
    XLOG_ERR("xshm mmap error");
    xfree(shm);
    return NULL; 
  } 
  shm->_size = size;
  return shm;
}

void xshm_free(xshm *shm) {
  if (shm == NULL) return;
  if (munmap(shm->addr, shm->_size) == -1) {
    XLOG_ERR("xshm munmap error");
  }
  xfree(shm);
}

pthread_mutex_t* xshm_mutex_new(void) {
  pthread_mutex_t *shmux = mmap(NULL, sizeof(pthread_mutex_t), PROT_READ|PROT_WRITE,
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

void xshm_mutex_free(pthread_mutex_t *shmux) {
  if (shmux == NULL) return;
  pthread_mutex_destroy(shmux);
  if (munmap(shmux, sizeof(pthread_mutex_t)) == -1) {
    XLOG_ERR("xshm_mutex munmap error");
  }
}
