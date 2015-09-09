#include "xutil.h"
#include <fcntl.h>

int 
xdaemon() {
  switch (fork()) {
  case -1:
    return -1;
  case 0:   // child return here
    break;
  default:  // parent return and exit
    exit(EXIT_SUCCESS);
  }

  if (setsid() == -1) return -1;
  if (chdir("/") != 0) return -1;

  int fd;
  if ((fd = open("/dev/null", O_RDWR, 0)) == -1) return -1;
  if (dup2(fd, STDIN_FILENO) < 0) return -1;
  if (dup2(fd, STDOUT_FILENO) < 0) return -1;
  if (dup2(fd, STDERR_FILENO) < 0) return -1;
  if (fd > STDERR_FILENO) close(fd);
  return 0;
}

bool
xsave_pid(const char* pid_file) {
  ASSERT(pid_file);
  FILE *fp;
  if (!(fp = fopen(pid_file, "w"))) return false;
  fprintf(fp, "%ld\n", (long)getpid());
  if (fclose(fp) == -1) return false;
  return true;
}

pid_t
xget_pid(const char* pid_file) {
  ASSERT(pid_file);
  long pid;
  FILE *fp;
  if (!(fp = fopen(pid_file, "r"))) return 0;
  fscanf(fp, "%ld\n", &pid);
  fclose(fp);
  return pid;
}

bool
xremove_pid(const char* pid_file) {
  ASSERT(pid_file);
  if (unlink(pid_file)) return false;
  return true;
}

extern char** environ;

static char** xargv;
static char* xargv_last = NULL;

bool
xinit_proc_title(int argc, char** argv) {
	xargv = argv;

	size_t size = 0;
	for (int i=0; environ[i]; i++) {
		size += strlen(environ[i]) + 1;
	}

	char* p = calloc(1, size);
	if (p == NULL) return false;

	xargv_last = xargv[0];
	for (int i=0; xargv[i]; i++) {
		if (xargv_last == xargv[i]) xargv_last = xargv[i] + strlen(xargv[i]) + 1;
	}

	for (int i=0; environ[i]; i++) {
		if (xargv_last == environ[i]) {
			size = strlen(environ[i]) + 1;
			xargv_last = environ[i] + size;
			strncpy(p, environ[i], size);
			environ[i] = p;
			p += size;
		}
	}

	xargv_last--;

	return true;
}

void
xset_proc_title(char* title) {
	xargv[1] = NULL;
	strncpy(xargv[0], title, xargv_last - xargv[0]);
}
