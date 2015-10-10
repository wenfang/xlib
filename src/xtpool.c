#include "xtpool.h"
#include "xopt.h"
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
  xhandler          handler;
  struct list_head  node;
  unsigned          stop;
} xthread;

typedef struct xtpool_s {
  unsigned          size;
  unsigned          avail;
  struct list_head  free;
  pthread_mutex_t   freeLock;
  xthread           threads[];
} xtpool;

static xtpool *pool;

typedef void *(*routiner)(void*);

static void* _once(void *arg) {
  xthread *thread = arg;

  XHANDLER_CALL(thread->handler);

  pthread_mutex_destroy(&thread->lock);
  pthread_cond_destroy(&thread->ready);
  xfree(thread);
  __sync_sub_and_fetch(&pool->avail, 1);
  return NULL;
}

static void* _normal(void *arg) {
  xthread *thread = arg;
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
    thread->handler = XHANDLER_EMPTY;
    // add to free list
    pthread_mutex_lock(&pool->freeLock);
    list_add_tail(&thread->node, &pool->free);
    pthread_mutex_unlock(&pool->freeLock);
  }
  // quit thread
  pthread_mutex_destroy(&thread->lock);
  pthread_cond_destroy(&thread->ready);
  __sync_sub_and_fetch(&pool->avail, 1);
  return NULL;
}

static void _thread_init(xthread *thread, routiner routine, xhandler handler) {
  pthread_mutex_init(&thread->lock, NULL);
  pthread_cond_init(&thread->ready, NULL);
  thread->handler  = handler;
  thread->stop     = 0;   
  // create thread
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  pthread_create(&thread->id, &attr, routine, thread); 
  pthread_attr_destroy(&attr);
}

bool xtpool_do(xhandler handler) {
  if (!pool) return false;
  xthread *thread = NULL;
  if (!list_empty(&pool->free)) {
    // get free thread
    pthread_mutex_lock(&pool->freeLock);
    if ((thread = list_first_entry(&pool->free, xthread, node))) {
      list_del_init(&thread->node);
    }
    pthread_mutex_unlock(&pool->freeLock);
    // set handler to thread
    if (likely(thread != NULL)) {
      pthread_mutex_lock(&thread->lock);
      thread->handler = handler;
      pthread_mutex_unlock(&thread->lock);
      pthread_cond_signal(&thread->ready);
      return true;
    }
  }
  thread = xcalloc(sizeof(xthread));
  __sync_add_and_fetch(&pool->avail, 1);
  _thread_init(thread, _once, handler);
  return true;
}

static bool _init(void) {
  int size = xopt_int("tpool", "size", -1);
  if (size <= 0) return true;
  if (size == 0) size = xcpu_count() * 2;
  if (size > MAX_THREAD) size = MAX_THREAD;

  pool = xcalloc(sizeof(xtpool) + size*sizeof(xthread));
  pool->size = size;
  INIT_LIST_HEAD(&pool->free);
  pthread_mutex_init(&pool->freeLock, NULL);
  // enable thread
  __sync_add_and_fetch(&pool->avail, pool->size);
  for (int i=0; i<pool->size; i++) {
    xthread *thread = &pool->threads[i];
    INIT_LIST_HEAD(&thread->node);
    list_add_tail(&thread->node, &pool->free);
    _thread_init(thread, _normal, XHANDLER_EMPTY);
  }
  return true;
}

static void _deinit(void) {
  if (!pool) return;
  // set stop flag
  for (int i=0; i<pool->size; i++) {
    xthread *thread = &pool->threads[i];
    pthread_mutex_lock(&thread->lock);
    thread->stop = 1;
    pthread_mutex_unlock(&thread->lock);
    pthread_cond_signal(&thread->ready);
  }
  // wait all quit
  int sleep_cnt = 0;
  while (sleep_cnt < 300) {
    int all = __sync_fetch_and_add(&pool->avail, 0);
    if (!all) break;
    usleep(100000);
    sleep_cnt++;
  }
  pthread_mutex_destroy(&pool->freeLock);
  xfree(pool);
  pool = NULL;
}

xmodule xtpool_module = {
  "xtpool",
  XCORE_MODULE,
  NULL,
  _init,
  _deinit,
  NULL,
};
