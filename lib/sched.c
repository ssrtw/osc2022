#include "sched.h"

#include "except.h"
#include "malloc.h"
#include "string.h"
#include "timer.h"
#include "uart.h"

void init_threads() {
    lock();
    curr_pid = 0;
    rq = kmalloc(sizeof(list_head_t));
    INIT_LIST_HEAD(rq);
    for (int i = 0; i <= PID_MAX; i++) {
        threads[i].pid = i;
        threads[i].state = THREAD_UNUSED;
    }
    curr_thread = thread_create(idle);
    asm volatile("msr tpidr_el1, %0" ::"r"(kmalloc(sizeof(thread_context_t))));
    unlock();
}

void idle() {
    for (;;) {
        kill_zombies();
        schedule();
    }
}

void schedule() {
    lock();
    do {
        curr_thread = list_next_entry(curr_thread, lh);
    } while (list_is_head(&curr_thread->lh, rq) || curr_thread->state == THREAD_ZOMBIE);
    unlock();
    switch_to(get_current(), &curr_thread->cxt);
}

void kill_zombies() {
    lock();
    thread_t *curr;
    list_for_each_entry(curr, rq, lh) {
        if (curr->state == THREAD_ZOMBIE) {
            list_del_entry(&curr->lh);
            kfree(curr->ustack_ptr);
            kfree(curr->kstack_ptr);
            curr->state = THREAD_UNUSED;
        }
    }
    unlock();
}

void exec_thread(byte *data, uint32_t textsize) {
    thread_t *t = thread_create(data);
    t->data = kmalloc(textsize);
    // 填入text段
    // uart_printf("copy text from:%x\n", data);
    memcpy(t->data, (char *)data, textsize);
    t->cxt.lr = (uint64_t)t->data;

    curr_thread = t;
    add_timer_task(TIMER_BY_SESC, 1, schedule_timer_task, "");
    // uart_printf("will return:%x\n", data);
    asm volatile(
        "msr tpidr_el1, %0\n\t"  // thread info addr
        "msr elr_el1, %1\n\t"    // ereturn addr, back to el0 start address
        "msr spsr_el1, xzr\n\t"  // enable interrupt in EL0. You can do it by setting spsr_el1 to 0 before returning to EL0.
        "msr sp_el0, %2\n\t"     // el0的stack(user stack)
        "mov sp, %3\n\t"         // el1的stack(kernel stack)
        "eret\n\t" ::"r"(&t->cxt),
        "r"(t->cxt.lr), "r"(t->cxt.sp), "r"(t->kstack_ptr + KSTACK_SIZE));
}

// thread_t *thread_create(void *start, uint32_t textsize) {
thread_t *thread_create(void *start) {
    lock();
    thread_t *use_thread;
    // 先檢查是否已經拿完一輪了，不然直接從下個pid找起
    if (unlikely(curr_pid > PID_MAX)) {
        int i;
        for (i = 0; i <= PID_MAX; i++) {
            if (threads[i].state == THREAD_UNUSED) {
                use_thread = &threads[i];
                break;
            }
        }
        if (i > PID_MAX) {
            uart_puts("error, haven't any can use PID\n");
            return NULL;
        }
    } else {
        use_thread = &threads[curr_pid++];
    }
    use_thread->state = THREAD_RUNNING;
    use_thread->kstack_ptr = kmalloc(KSTACK_SIZE);
    use_thread->ustack_ptr = kmalloc(USTACK_SIZE);
    // uart_printf("kstack addr=%x\nustack addr=%x\n", use_thread->kstack_ptr, use_thread->ustack_ptr);
    use_thread->cxt.lr = (uint64_t)start;
    use_thread->cxt.sp = (uint64_t)use_thread->ustack_ptr + USTACK_SIZE;
    use_thread->cxt.fp = use_thread->cxt.sp;
    // sig about
    use_thread->sigcheck = 0;
    for (int i = 0; i < SIG_MAX; i++) {
        use_thread->sig_handler[i] = default_sig_handler;
        use_thread->sigcount[i] = 0;
    }
    list_add(&(use_thread->lh), rq);
    unlock();
    return use_thread;
}

void thread_exit() {
    lock();
    curr_thread->state = THREAD_ZOMBIE;
    unlock();
    schedule();
}

void schedule_timer_task(char *args) {
    unsigned long long cntfrq_el0;
    asm volatile("mrs %0, cntfrq_el0\n\t"
                 : "=r"(cntfrq_el0));  // tick frequency
    // add a new schedule task
    // Set the expired time as core timer frequency shift right 5 bits.
    add_timer_task(TIMER_BY_TICK, cntfrq_el0 >> 5, schedule_timer_task, "");
}