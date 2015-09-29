#ifndef __XEPOLL_H
#define __XEPOLL_H

#include "xtask.h"
#include <stdbool.h>

#define XEPOLL_NONE   0
#define XEPOLL_LISTEN 1
#define XEPOLL_READ   1
#define XEPOLL_WRITE  2

bool xepoll_init(void);
void xepoll_deinit(void);

bool xepoll_enable(unsigned fd, unsigned mask, xtask *task);
bool xepoll_disable(unsigned fd, unsigned mask);

void xepoll_process(int timeout);

#endif
