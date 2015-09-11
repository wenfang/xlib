#include "xtpool.h"
#include "xmalloc.h"
#include "xutil.h"
#include "list.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define MAX_THREAD  64

typedef struct xthread_s {
  pthread_t         id;
  pthread_mutex_t   lock;
  pthread_cond_t    ready;
  xhandler_t        handler;
  struct list_head  node;
  unsigned          stop:1;
} xthread_t;

typedef struct xtpool_s {
  unsigned          total;
  unsigned          threadRun;
  struct list_head  freeList;
  pthread_mutex_t   freeListLock;
  xthread_t         threads[MAX_THREAD];
} xtpool_t;

static xtpoolt* pool;

typedef void *(*routiner)(void*);

static void* routineOnce(void *arg) {
  xthread_t *thread = arg;

  XHANDLER_CALL(thread->handler);

  __sync_sub_and_fetch(&pool->threadRun, 1);
  pthread_mutex_destroy(&thread->lock);
  pthread_cond_destroy(&thread->ready);
  xfree(thread);
  return NULL;
}

static void* routineNormal(void *arg) {
  xthread_t *thread = arg;
  // run thread
  while (1) {
    pthread_mutex_lock(&thread->lock);
    while (!thread->handler.handler && !thread->stop) {
      pthread_cond_wait(&thread->ready, &thread->lock);
    }
    pthread_mutex_unlock(&thread->lock);
    if (unlikely(thread->stop)) break;
    // run handler
    XHANDLER_CALL(thread->handler);
    thread->handler = XHANDLER_NULL;
    // add to free list
    pthread_mutex_lock(&pool->freeListLock);
    list_add_tail(&thread->node, &pool->freeList);
    pthread_mutex_unlock(&pool->freeListLock);
  }
  // quit thread
  __sync_sub_and_fetch(&pool->threadRun, 1);
  pthread_mutex_destroy(&thread->lock);
  pthread_cond_destroy(&thread->ready);
  xfree(thread);
  return NULL;
}

static void
_thread_init(xthread_t *thread, routiner routine, xhandler_t handler) {
  pthread_mutex_init(&thread->lock, NULL);
  pthread_cond_init(&thread->ready, NULL);
  thread->handler  = handler;
  thread->stop     = 0;   
  // put into all 
  __sync_add_and_fetch(&pool->threadRun, 1);
  // create thread
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  pthread_create(&thread->id, &attr, routine, thread); 
  pthread_attr_destroy(&attr);
}

bool xtpool_do(xhandler_t handler) {
  if (!pool) return false;
  xthread_t* thread = NULL;
  if (!list_empty(&pool->freeList)) {
    // get free thread
    pthread_mutex_lock(&pool->freeListLock);
    if ((thread = list_first_entry(&pool->freeList, xthread_t, node))) {
      list_del_init(&thread->node);
    }
    pthread_mutex_unlock(&pool->freeListLock);
    // set handler to thread
    if (likely(thread != NULL)) {
      pthread_mutex_lock(&thread->lock);
      thread->handler = handler;
      pthread_mutex_unlock(&thread->lock);
      pthread_cond_signal(&thread->ready);
      return true;
    }
  }
  thread = xcalloc(sizeof(xthread_t));
  if (!thread) return false;
  _thread_init(thread, routineOnce, handler);
  return true;
}

bool xtpool_init() {
  if (pool) return false;
  pool = xcalloc(1, sizeof(xtpoolt));
  if (!pool) return false;

  pool->total = xcpu_count() * 2;
  if (pool->total > MAX_THREAD) pool->total = MAX_THREAD;
  INIT_LIST_HEAD(&pool->freeList);
  pthread_mutex_init(&pool->freeListLock, NULL);
  // enable thread
  int i;
  for (i=0; i<pool->total; i++) {
    xthread_t *thread = &pool->threads[i];
    INIT_LIST_HEAD(&thread->node);
    list_add_tail(&thread->node, &pool->freeList);
    _thread_init(thread, routineNormal, XHANDLER_NULL);
  }
  return true;
}

void xtpool_deinit() {
  if (!pool) return;
  // set stop flag
  int i;
  for (i=0; i<pool->total; i++) {
    xthread_t *thread = &pool->threads[i];
    pthread_mutex_lock(&thread->lock);
    thread->stop = 1;
    pthread_mutex_unlock(&thread->lock);
    pthread_cond_signal(&thread->ready);
  }
  // wait all quit
  int sleep_cnt = 0;
  while (sleep_cnt > 10) {
    int all = __sync_fetch_and_add(&pool->threadRun, 0);
    if (!all) break;
    usleep(50000);
    sleep_cnt++;
  }
  pthread_mutex_destroy(&pool->freeListLock);
  free(pool);
  pool = NULL;
}
