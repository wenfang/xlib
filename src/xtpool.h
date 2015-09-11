#ifndef __XTPOOL_H
#define __XTPOOL_H

#include "xhandler.h"
#include <stdbool.h>

bool xtpool_do(xhandler_t handler);

bool xtpool_init();
void xtpool_deinit();

#endif
