#ifndef __SPE_OPT_H
#define __SPE_OPT_H

#include <stdbool.h>

extern int
spe_opt_int(char* section, char* key, int def);

extern const char*
spe_opt_string(char* section, char* key, const char* def);

extern bool 
spe_opt_create(const char* config_file);

extern void
spe_opt_destroy();

#endif
