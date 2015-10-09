#ifndef __XSIGNAL_H
#define __XSIGNAL_H

typedef void (*xsignalHandlerFunc)(int sig);

void xsignal_register(int sig, xsignalHandlerFunc handlerFunc);
void xsignal_process(void);

void xsignal_init(void);

#endif
