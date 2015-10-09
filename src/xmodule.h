#ifndef __XMODULE_H
#define __XMODULE_H

#include <stdbool.h>

#define XCORE_MODULE 1
#define XUSER_MODULE 2

typedef struct xmodule_s {
  const char* name;
  int         type;

  bool  (*init_master)(void);
  bool  (*init_worker)(void);
  void  (*deinit_worker)(void);
  void  (*deinit_master)(void);
} xmodule;

bool xmodule_master_init(int type);
void xmodule_master_deinit(int type);

bool xmodule_worker_init(int type);
void xmodule_worker_deinit(int type);

#endif
