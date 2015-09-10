#ifndef __XEPOLL_H
#define __XEPOLL_H

#include "xtask.h"
#include <stdbool.h>

#define XEPOLL_NONE   0
#define XEPOLL_LISTEN 1
#define XEPOLL_READ   1
#define XEPOLL_WRITE  2

extern bool
xepoll_init();

extern bool
xepoll_deinit(void);

extern bool
xepoll_enable(unsigned fd, unsigned mask, xtask *task);

extern bool
xepoll_disable(unsigned fd, unsigned mask);

extern void
xepoll_process(int timeout);

#endif
