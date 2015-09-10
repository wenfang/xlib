#ifndef __XOPT_H
#define __XOPT_H

#include <stdbool.h>

int xopt_int(const char* sec, const char* key, int def);
const char* xopt_string(const char* sec, const char* key, const char* def);

bool xopt_new(const char* config_file);
void xopt_free(void);

#endif
