#include "spe_io.h"
#include "spe_opt.h"
#include "spe_list.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>

#define SEC_MAXLEN  16
#define KEY_MAXLEN  16
#define VAL_MAXLEN  128

typedef struct spe_opt_s {
	char sec[SEC_MAXLEN];
 	char key[KEY_MAXLEN];      // string key
	char val[VAL_MAXLEN];      // string value
	struct list_head  node;
} spe_opt_t;

static LIST_HEAD(options);

/*
===================================================================================================
spe_opt_set
===================================================================================================
*/
static bool 
spe_opt_set(char* sec, char* key, char* val) {
  spe_opt_t* opt = calloc(1, sizeof(spe_opt_t));
  if (opt == NULL) return false;
  // set section and key 
  strncpy(opt->sec, sec, SEC_MAXLEN);
  strncpy(opt->key, key, KEY_MAXLEN);
  strncpy(opt->val, val, VAL_MAXLEN);
  // add to options list
  INIT_LIST_HEAD(&opt->node);
  list_add_tail(&opt->node, &options);
  return true;
}

/*
===================================================================================================
spe_opt_int
===================================================================================================
*/
int
spe_opt_int(char* section, char* key, int def) {
  if (key == NULL) return def;
  if (section == NULL) section = "global";
  spe_opt_t* entry = NULL;
  list_for_each_entry(entry, &options, node) {
    if (!strcmp(section, entry->sec) && !strcmp(key, entry->key)) {
      return atoi(entry->val);
    }
  }
  return def;
}

/*
===================================================================================================
spe_opt_string
===================================================================================================
*/
const char* 
spe_opt_string(char* section, char* key, const char* def) {
  if (key == NULL) return def;
  if (section == NULL) section = "global";
  spe_opt_t* entry = NULL;
  list_for_each_entry(entry, &options, node) {
    if (!strcmp(section, entry->sec) && !strcmp(key, entry->key)) {
      return entry->val;
    }
  }
  return def;
}

/*
===================================================================================================
spe_opt_create
===================================================================================================
*/
bool 
spe_opt_create(const char* config_file) {
  char sec[SEC_MAXLEN];
  char key[KEY_MAXLEN];
  char val[VAL_MAXLEN];

  spe_io_t* io = spe_io_create(config_file);
  if (io == NULL) return false;
  spe_buf_t* line = spe_buf_create();
  if (line == NULL) return false;
  // set default section
  strcpy(sec, "global");
  int res = 1;
  while (res > 0) {
    // get one line from file
    spe_buf_clean(line);
    res = spe_io_readuntil(io, "\n", line);
    spe_buf_strim(line, " \r\t\n");
    if (line->len == 0 || line->data[0] == '#') continue;
    // section line, get section
    if (line->data[0] == '[' && line->data[line->len-1] == ']') {
      spe_buf_strim(line, " []\t");
      // section can't be null
      if (line->len == 0) {
        spe_buf_destroy(line);
        spe_io_destroy(io);
        return false;
      }
      strncpy(sec, line->data, SEC_MAXLEN);
      sec[SEC_MAXLEN-1] = 0;
      continue;
    }
    // split key and value
    spe_bufs_t* bufs = spe_buf_split(line, "=");
    if (bufs->len != 2) {
      spe_bufs_destroy(bufs);
      spe_buf_destroy(line);
      spe_io_destroy(io);
      return false;
    }
    spe_buf_strim(bufs->data[0], " ");
    strncpy(key, bufs->data[0]->data, KEY_MAXLEN);
    key[KEY_MAXLEN-1] = 0;

    spe_buf_strim(bufs->data[1], " ");
    strncpy(val, bufs->data[1]->data, VAL_MAXLEN);
    val[VAL_MAXLEN-1] = 0;
    spe_bufs_destroy(bufs);
    // set option value
    spe_opt_set(sec, key, val);
  }
  spe_buf_destroy(line);
  spe_io_destroy(io);
  return true;
}

/*
===================================================================================================
spe_opt_destroy
===================================================================================================
*/
void
spe_opt_destroy() {
  spe_opt_t *entry, *tmp;
  list_for_each_entry_safe(entry, tmp, &options, node) {
    list_del_init(&entry->node);
    free(entry);
  }
}
