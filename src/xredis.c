#include "xredis.h"
#include "xmalloc.h"

#define XREDIS_FREE 1;
#define XREDIS_CONN 2;

xredis* xredis_new(const char *addr, const char *port) {
  xredis *rds = xcalloc(sizeof(xredis));
  rds._addr = addr;
  rds._port = port;
  rds._status = XREDIS_FREE;
  xtask_init(&rds->post_task);
  return rds;
}

void xredis_free(xredis *rds) {
  if (rds._status != XREDIS_FREE) {
    xconn_close(rds._conn);
  }
  xstrings_free(rds.buf, rds.cnt);
  xfree(rds);
}
