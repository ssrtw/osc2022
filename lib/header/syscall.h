#ifndef SYSCALL_H
#define SYSCALL_H
#include "except.h"
#include "stddef.h"

void syscall_handler(trapframe_t* tf);

#endif