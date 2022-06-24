#ifndef _SIGNAL_H
#define _SIGNAL_H

#define USER_SIG_WRAPPER_VIRT_ADDR 0xffffffff9000L

#define SIG_MAX 32
typedef void (*sig_handler_t)();

#include "stddef.h"
#include "trapframe.h"

void check_sig(trapframe_t *tf);
void run_sig(trapframe_t *tf, uint32_t signum);
void default_sig_handler();

void sig_handler_wrapper();

#endif