#ifndef __XSIGNAL_H
#define __XSIGNAL_H

#include "xmodule.h"

typedef void (*xsignalHandlerFunc)(int sig);

void xsignal_register(int sig, xsignalHandlerFunc handlerFunc);
void xsignal_process(void);

extern xmodule xsignal_module;

#endif
