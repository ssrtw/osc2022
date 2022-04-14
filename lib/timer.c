#include "timer.h"

#include "malloc.h"
#include "register.h"
#include "stddef.h"
#include "string.h"
#include "uart.h"

struct list_head *timer_task_head;
void do_timer_task(timer_task_t *task);

void timer_enable() {
    asm volatile(
        "mov x1, 1\n\t"
        "msr cntp_ctl_el0, x1\n\t"  // enable
        "mov x1, 2\n\t"
        "ldr x2, =" XSTR(CORE0_TIMER_IRQ_CTRL)
            "\n\t"
            "str w1, [x2]\n\t"  // unmask timer interrupt
    );
}

void timer_disable() {
    asm volatile(
        "mov x1, 0\n\t"
        "msr cntp_ctl_el0, x1\n\t"  // disable
    );
}

void set_timer_interrupt_by_secs(size_t secs) {
    asm volatile(
        "mrs x1, cntfrq_el0\n\t"     // cntfrq_el0 -> 一秒幾次
        "mul x1, x1, %0\n\t"         // 算出x秒幾tick
        "msr cntp_tval_el0, x1\n\t"  // 設定相對時間(當寫入 TVAL 時，其實是將 CVAL 設為 CNTPCT_EL0 加上寫入 TVAL 的值)
        ::"r"(secs));
}

void set_timer_interrupt_by_tick(size_t tick) {
    asm volatile(
        "msr cntp_cval_el0, %0\n\t"  // 直接設定多少tick一次
        ::"r"(tick));
}

void two_sec_callback(char *args) {
    size_t cntfrq_el0, cntpct_el0;
    asm volatile(
        "mrs %0, cntfrq_el0\n\t"
        "mrs %1, cntpct_el0\n\t"
        : "=r"(cntfrq_el0), "=r"(cntpct_el0));
    uart_printf("is timer interrput, current time: %ds\n", cntpct_el0 / cntfrq_el0);
    uart_printf("%s\n", args);
}

void timer_alert_callback(char *args) {
    size_t cntfrq_el0, cntpct_el0;
    asm volatile(
        "mrs %0, cntfrq_el0\n\t"
        "mrs %1, cntpct_el0\n\t"
        : "=r"(cntfrq_el0), "=r"(cntpct_el0));
    uart_printf("is timer interrput, current time: %ds\n", cntpct_el0 / cntfrq_el0);
    uart_printf("%s\n", args);
}

void timer_handler() {
    if (list_empty(timer_task_head)) {
        set_timer_interrupt_by_secs(500);  // 設定大數值當作不執行timer interrupt
        return;
    }
    do_timer_task((timer_task_t *)timer_task_head->next);
}

void do_timer_task(timer_task_t *task) {
    list_del_entry((struct list_head *)task);
    // exec callback function
    (task->func)(task->args);
    if (!list_empty(timer_task_head)) {
        set_timer_interrupt_by_tick(((timer_task_t *)timer_task_head->next)->expire);
    } else {
        set_timer_interrupt_by_secs(500);  // 設定大數值當作不執行timer interrupt
    }
}

void init_task_list() {
    timer_task_head = malloc_size(sizeof(struct list_head));
    INIT_LIST_HEAD(timer_task_head);
}

size_t get_expire_tick_from_secs(size_t seconds) {
    size_t cntfrq_el0, cntpct_el0, nxt_tick;
    asm volatile(
        "mrs %0, cntfrq_el0\n\t"
        "mrs %1, cntpct_el0\n\t"
        : "=r"(cntfrq_el0), "=r"(cntpct_el0));
    nxt_tick = cntfrq_el0 * seconds + cntpct_el0;
    return nxt_tick;
}

void add_timer_task(timer_callback func, size_t seconds, char *args) {
    timer_task_t *new_task = (timer_task_t *)malloc_size(sizeof(timer_task_t));
    int args_len = strlen(args) + 1;
    new_task->func = func;
    new_task->expire = get_expire_tick_from_secs(seconds);
    new_task->args = malloc_size(args_len);
    new_task->args[args_len] = '\0';
    strncpy(new_task->args, args, args_len);

    INIT_LIST_HEAD(&new_task->list_head);

    struct list_head *curr;
    list_for_each(curr, timer_task_head) {
        // 迴圈當下的task比新task到期時間大，插入到前面
        if (((timer_task_t *)curr)->expire > new_task->expire) {
            list_add(&new_task->list_head, curr->prev);
            break;
        }
    }
    // 前面迴圈跑完，new_task是到期時間最久的
    if (list_is_head(curr, timer_task_head)) {
        list_add_tail((struct list_head *)new_task, timer_task_head);
    }

    set_timer_interrupt_by_tick(((timer_task_t *)(timer_task_head->next))->expire);
}