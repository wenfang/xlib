#ifndef __XTASK_H
#define __XTASK_H

#include "list.h"
#include "rbtree.h"
#include "xhandler.h"
#include <stdbool.h>

#define XTASK_FAST    1
#define XTASK_TIMEOUT 2

typedef struct xtask_s {
  xhandler          handler;
  unsigned          flags;
  struct rb_node    _timer_node;
  struct list_head  _task_node;
  unsigned          _deadline;
  unsigned          _status;
} xtask_t __attribute__((aligned(sizeof(long))));

extern void
xtask_init(xtask_t *task);

extern bool
xtask_empty(void);

extern void
xtask_enqueue(xtask_t *task);

extern void
xtask_enqueue_timeout(xtask_t *task, unsigned long ms);

extern void
xtask_dequeue(xtask_t *task);

extern void
xtask_process(void);

#endif
