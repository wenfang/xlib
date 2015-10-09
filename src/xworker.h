#ifndef __XWORKER_H
#define __XWORKER_H

#include <sys/types.h>

int xworker_fork(void);
int xworker_reset(pid_t pid);

void xworker_stop(void);
void xworker_process(void);

#endif
