#ifndef _SIGNAL_H
#define _SIGNAL_H

#include "except.h"

typedef void (*sig_handler_t)();

void check_sig(trapframe_t *tf);
void run_sig(trapframe_t *tpf, uint32_t signum);
void default_sig_handler();

#endif