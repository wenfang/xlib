#include "xio.h"
#include "xopt.h"
#include "xmalloc.h"
#include "list.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>

#define SEC_MAXLEN  16
#define KEY_MAXLEN  16
#define VAL_MAXLEN  128

typedef struct xopt_s {
	char sec[SEC_MAXLEN];
 	char key[KEY_MAXLEN];      // string key
	char val[VAL_MAXLEN];      // string value
	struct list_head  node;
} xopt_t;

static LIST_HEAD(options);

static bool xopt_set(const char* sec, const char* key, const char* val) {
  xopt_t* opt = xcalloc(sizeof(xopt_t));
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

int xopt_int(const char* sec, const char* key, int def) {
  if (key == NULL) return def;
  if (sec == NULL) sec = "global";
  xopt_t* entry = NULL;
  list_for_each_entry(entry, &options, node) {
    if (!strcmp(sec, entry->sec) && !strcmp(key, entry->key)) {
      return atoi(entry->val);
    }
  }
  return def;
}

const char* xopt_string(const char* sec, const char* key, const char* def) {
  if (key == NULL) return def;
  if (sec == NULL) sec = "global";
  xopt_t* entry = NULL;
  list_for_each_entry(entry, &options, node) {
    if (!strcmp(sec, entry->sec) && !strcmp(key, entry->key)) {
      return entry->val;
    }
  }
  return def;
}

bool xopt_new(const char* config_file) {
  char sec[SEC_MAXLEN];
  char key[KEY_MAXLEN];
  char val[VAL_MAXLEN];
  int res;

  xio_t* io = xio_newfile(config_file, 0);
  if (io == NULL) return false;
  
  xstring line = xstring_empty();
  if (line == NULL) return false;

  // set default section
  strcpy(sec, "global");
 
  res = xio_readuntil(io, "\n", &line); 
  while ((res > 0) || (res <=0 && xstring_len(line) > 0)) {
    // get one line from file
    xstring_strim(line, " \r\t\n");
    if (xstring_len(line) == 0 || *line == '#') goto next;
    // section line, get section
    if (*line == '[' && *(line+xstring_len(line)) == ']') {
      xstring_strim(line, " []\t");
      // section can't be null
      if (xstring_len(line) == 0) {
        xstring_free(line);
        xio_free(io);
        return false;
      }
      strncpy(sec, line, SEC_MAXLEN);
      sec[SEC_MAXLEN-1] = 0;
      goto next;
    }
    // split key and value
    int count;
    xstring* tokens = xstring_split(line, "=", &count);
    if (count != 2) {
      xstrings_free(tokens, count);
      xstring_free(line);
      xio_free(io);
      return false;
    }
    xstring_strim(tokens[0], " ");
    strncpy(key, tokens[0], KEY_MAXLEN);
    key[KEY_MAXLEN-1] = 0;

    xstring_strim(tokens[1], " ");
    strncpy(val, tokens[1], VAL_MAXLEN);
    val[VAL_MAXLEN-1] = 0;
    xstrings_free(tokens, count);
    // set option value
    xopt_set(sec, key, val);

next:
    if (res <= 0) break;
    xstring_clean(line);
    res = xio_readuntil(io, "\n", &line); 
  };
  xstring_free(line);
  xio_free(io);
  return true;
}

void xopt_free(void) {
  xopt_t *entry, *tmp;
  list_for_each_entry_safe(entry, tmp, &options, node) {
    list_del_init(&entry->node);
    xfree(entry);
  }
}

#ifdef __XOPT_TEST

#include "xunittest.h"

int main(void) {
  xio_t *io = xio_newfile("testdata/test.conf", 1);
  XTEST_NOT_EQ(io, NULL);

  xstring s = xstring_new("name= wenfang\n# test\n age = 12 \nlocation=beijing");
  xio_write(io, s);
  xio_flush(io);

  xstring_free(s);
  xio_free(io);
  
  bool res = xopt_new("testdata/test.conf");
  XTEST_EQ(res, true);
  XTEST_STRING_EQ(xopt_string(NULL, "name", NULL), "wenfang");
  XTEST_EQ(xopt_int(NULL, "age", 0), 12);
  XTEST_STRING_EQ(xopt_string(NULL, "location", NULL), "beijing");
  xopt_free();
}

#endif
