#ifndef __XTASK_H
#define __XTASK_H

#include "list.h"
#include "rbtree.h"
#include "xhandler.h"
#include <stdbool.h>

#define XTASK_FAST    1
#define XTASK_TIMEOUT 2

typedef struct xtask_s {
  xhandler_t        handler;
  unsigned          flags;
  struct rb_node    _timer_node;
  struct list_head  _task_node;
  unsigned          _deadline;
  unsigned          _status;
} xtask __attribute__((aligned(sizeof(long))));

void xtask_init(xtask *task);
bool xtask_empty(void);

void xtask_enqueue(xtask *task);
void xtask_enqueue_timeout(xtask *task, unsigned long ms);
void xtask_dequeue(xtask *task);

void xtask_process(void);

#endif
