#ifndef SYSCALL_H
#define SYSCALL_H
#include "except.h"
#include "stddef.h"

#define SYSNUM_getpid     0
#define SYSNUM_uart_read  1
#define SYSNUM_uart_write 2
#define SYSNUM_exec       3
#define SYSNUM_fork       4
#define SYSNUM_exit       5
#define SYSNUM_mbox_call  6
#define SYSNUM_kill_pid   7
#define SYSNUM_signal     8
#define SYSNUM_kill       9
#define SYSNUM_sigret     10

void syscall_handler(trapframe_t* tf);
// signal need
void sys_kill_pid(trapframe_t *tf, int pid);
#endif