#include "xtask.h"
#include "xutil.h"
#include "xlog.h"
#include <stdlib.h>
#include <pthread.h>

#define XTASK_FREE   0   // task is alloc not in queue
#define XTASK_TIMER  1   // task in timer queue
#define XTASK_QUEUE  2   // task in running queue

static LIST_HEAD(task_head);
static struct rb_root timer_head;

void xtask_init(xtask *task) {
  ASSERT(task);
  task->handler   = XHANDLER_EMPTY;
  task->flags     = 0;
  task->_deadline = 0;
  task->_status   = XTASK_FREE;
  rb_init_node(&task->_timer_node);
  INIT_LIST_HEAD(&task->_task_node);
}

bool xtask_empty(void) {
  return list_empty(&task_head) ? true : false;
}

void xtask_enqueue(xtask *task) {
  ASSERT(task);
  if (task->_status == XTASK_QUEUE) return;
  if (task->_status == XTASK_TIMER) {
    rb_erase(&task->_timer_node, &timer_head);
    rb_init_node(&task->_timer_node);
  }
  if (unlikely(task->flags & XTASK_FAST)) {
    task->_status = XTASK_FREE;
    XHANDLER_CALL(task->handler);
    return;
  }
  list_add_tail(&task->_task_node, &task_head);
  task->_status = XTASK_QUEUE;
}

void xtask_enqueue_timeout(xtask *task, unsigned long ms) {
  ASSERT(task);
  if (task->_status == XTASK_QUEUE) return;
  if (task->_status == XTASK_TIMER) {
    rb_erase(&task->_timer_node, &timer_head);
    rb_init_node(&task->_timer_node);
  }
  task->_deadline = xcurrent_time() + ms;
  task->flags &= ~XTASK_TIMEOUT;
  struct rb_node **new = &timer_head.rb_node, *parent = NULL;
  while (*new) {
    xtask* curr = rb_entry(*new, xtask, _timer_node);
    parent = *new;
    if (task->_deadline < curr->_deadline) {
      new = &((*new)->rb_left);
    } else {
      new = &((*new)->rb_right);
    }
  }
  rb_link_node(&task->_timer_node, parent, new);
  rb_insert_color(&task->_timer_node, &timer_head);
  task->_status = XTASK_TIMER;
}

void xtask_dequeue(xtask* task) {
  ASSERT(task);
  if (task->_status == XTASK_FREE) return;
  if (task->_status == XTASK_QUEUE) {
    list_del_init(&task->_task_node);
  } else if (task->_status == XTASK_TIMER) {
    rb_erase(&task->_timer_node, &timer_head);
    rb_init_node(&task->_timer_node);
  }
  task->_status = XTASK_FREE;
}

void xtask_process(void) {
  // check timer queue
  if (!RB_EMPTY_ROOT(&timer_head)) {
    unsigned long curr_time = xcurrent_time();
    // check timer list
    struct rb_node* first = rb_first(&timer_head);
    while (first) {
      xtask* task = rb_entry(first, xtask, _timer_node);
      if (task->_deadline > curr_time) break;
      ASSERT(task->_status == XTASK_TIMER);
      rb_erase(&task->_timer_node, &timer_head);
      rb_init_node(&task->_timer_node);
      task->flags |= XTASK_TIMEOUT;
      // add to task queue
      list_add_tail(&task->_task_node, &task_head);
      task->_status = XTASK_QUEUE;
      first = rb_first(&timer_head);
    } 
  }
  // run task
  while (!list_empty(&task_head)) {
    xtask* task = list_first_entry(&task_head, xtask, _task_node);
    if (!task) break;
    ASSERT(task->_status == XTASK_QUEUE);
    list_del_init(&task->_task_node);
    task->_status = XTASK_FREE;
    XHANDLER_CALL(task->handler);
  }
}

__attribute__((constructor))
static void __xtask_init(void) {
  timer_head = RB_ROOT;
}

#ifdef __XTASK_TEST

#include "xunittest.h"

void foo(void *arg1, void *arg2) {
  xtask* task = arg1;
  task->flags |= XTASK_TIMEOUT;
  XTEST_EQ(task->flags, XTASK_TIMEOUT); 
  task->flags &= ~XTASK_TIMEOUT;
  XTEST_EQ(task->flags, 0);
}

void bar(void *arg1, void *arg2) {
  xtask* task = arg1;
  XTEST_EQ(task->flags, XTASK_TIMEOUT); 
}

int main(void) {
  xtask task1, task2;
  xtask_init(&task1);
  xtask_init(&task2);
  task1.handler = XHANDLER(foo, &task1, NULL);
  task2.handler = XHANDLER(bar, &task2, NULL);
  xtask_enqueue(&task1);
  xtask_enqueue_timeout(&task2, 200);
  sleep(1);
  xtask_process();
  return 0;
}

#endif
