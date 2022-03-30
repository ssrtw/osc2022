#include "timer.h"

#include "register.h"
#include "stddef.h"
#include "uart.h"

void timer_enable() {
    asm volatile(
        "mov x1, 1\n\t"
        "msr cntp_ctl_el0, x1\n\t"  // enable
        "mov x1, 0x1000000\n\t"
        "msr cntp_tval_el0, x1\n\t"
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

// TODO: 設計成callback
void timer_handler() {
    size_t cntfrq_el0, cntpct_el0;
    asm volatile(
        "mrs %0, cntfrq_el0\n\t"
        "mrs %1, cntpct_el0\n\t" ::"r"(cntfrq_el0),
        "r"(cntpct_el0));
    uart_printf("is timer interrput, current time: %ds\n", cntpct_el0 / cntfrq_el0);
    uart_printf("delay two seconds\n");
    set_timer_interrupt_by_secs(2);
}