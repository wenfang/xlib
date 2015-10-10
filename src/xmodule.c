#include "xmodule.h"
#include "xcycle.h"
#include "xsignal.h"
#include "xepoll.h"
#include "xconn.h"
#include "xserver.h"
#include "xtpool.h"
#include "xuser.h"

xmodule *xmodules[] = {
  &xsignal_module,
  &xepoll_module,
  &xconn_module,
  &xserver_module,
  &xtpool_module,
  &xuser_module,
};

bool xmodule_master_init(int type) {
  int i, module_num = sizeof(xmodules)/sizeof(xmodule*);
  for (i=0; i<module_num; i++) {
    if (xmodules[i]->type != type) continue;
    if (xmodules[i]->init_master && !xmodules[i]->init_master()) {
      XLOG_ERR("xmodule init master error");
      return false;
    }
  } 
  return true;
}

void xmodule_master_deinit(int type) {
  int i, module_num = sizeof(xmodules)/sizeof(xmodule*);
  for (i=module_num-1; i>=0; i--) {
    if (xmodules[i]->type != type) continue;
    if (xmodules[i]->deinit_master) xmodules[i]->deinit_master();
  }
}

bool xmodule_worker_init(int type) {
  int i, module_num = sizeof(xmodules)/sizeof(xmodule*);
  for (i=0; i<module_num; i++) {
    if (xmodules[i]->type != type) continue;
    if (xmodules[i]->init_worker && !xmodules[i]->init_worker()) {
      XLOG_ERR("xmodule init worker error");
      return false;
    }
  } 
  return true;
}

void xmodule_worker_deinit(int type) {
  int i, module_num = sizeof(xmodules)/sizeof(xmodule*);
  for (i=module_num-1; i>=0; i--) {
    if (xmodules[i]->type != type) continue;
    if (xmodules[i]->deinit_worker) xmodules[i]->deinit_worker();
  }
}
