#ifndef SCHED_H
#define SCHED_H

#include "list.h"
#include "signal.h"
#include "stddef.h"
#include "thread.h"

// https://stackoverflow.com/a/6294133
// linux 32 bit 32768
#define PID_MAX        32768
#define THREAD_RUNNING 0x0000
#define THREAD_ZOMBIE  0x0001
#define THREAD_UNUSED  0x0002

#define USTACK_SIZE 0x4000
#define KSTACK_SIZE 0x4000

int nxt_pid;
thread_t threads[PID_MAX + 1];
thread_t *curr_thread;
list_head_t *rq;

void init_threads();
void idle();
void schedule();
void init_tasks();
void kill_zombies();
void exec_thread(byte *data, uint32_t textsize);
// thread_t *thread_create(void *start, uint32_t text_size);
thread_t *thread_create(void *start, uint32_t filesize);
void thread_exit();

extern void switch_to(void *curr_cxt, void *next_cxt);
extern void store_cxt(void *curr_cxt);
extern void load_cxt(void *curr_cxt);
extern void *get_current();
void schedule_timer_task(char *args);

#endif