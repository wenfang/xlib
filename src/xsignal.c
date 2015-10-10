#include "xsignal.h"

#include <string.h>
#include <signal.h>

#define MAX_SIGNAL 256

typedef struct xsignal_s {
  unsigned            cnt;
  xsignalHandlerFunc  handlerFunc;
} xsignal;

static int      queue_len;
static int      queue[MAX_SIGNAL];
static xsignal  state[MAX_SIGNAL];
static sigset_t blocked;


static void _common_handler(int sig) {
	// not handler this signal, ignore
  if (sig < 0 || sig > MAX_SIGNAL || !state[sig].handlerFunc) {
    signal(sig, SIG_IGN);
    return;
  }
	// put signal in queue
  if (!state[sig].cnt && (queue_len<MAX_SIGNAL)) {
    queue[queue_len++] = sig;
  }
	// add signal cnt
  state[sig].cnt++;
  signal(sig, _common_handler);
}

void xsignal_register(int sig, xsignalHandlerFunc handlerFunc) {
	if (sig < 0 || sig > MAX_SIGNAL) return;
	state[sig].cnt = 0;
 	if (handlerFunc == NULL) handlerFunc = SIG_IGN; 
	// set signal handler
	if (handlerFunc != SIG_IGN && handlerFunc != SIG_DFL) {
		state[sig].handlerFunc = handlerFunc;
 		signal(sig, _common_handler);
 	} else {                        
   	state[sig].handlerFunc = NULL;
 		signal(sig, handlerFunc);
	}
}

void xsignal_process(void) {
  if (queue_len == 0) return;

	sigset_t old;
	sigprocmask(SIG_SETMASK, &blocked, &old);
 	// check signal queue	
	for (int i=0; i<queue_len; i++) {
		int sig = queue[i];
 		xsignal *desc = &state[sig];
		if (desc->cnt) {
   		if (desc->handlerFunc) desc->handlerFunc(sig);
   		desc->cnt = 0;
 		}
 	}
	queue_len = 0;
	sigprocmask(SIG_SETMASK, &old, NULL);  
}

static bool _init(void) {
  queue_len = 0;
  memset(queue, 0, sizeof(queue));
  memset(state, 0, sizeof(state));
  sigfillset(&blocked);
  return true;
}

xmodule xsignal_module = {
  "xsignal",
  XCORE_MODULE,
  _init,
  NULL,
  NULL,
  NULL,
};
