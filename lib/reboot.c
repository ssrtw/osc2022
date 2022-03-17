#include "reboot.h"

#include "gpio.h"

#define PM_PASSWORD 0x5a000000
#define PM_RSTC     ((volatile uint32_t*)(MMIO_BASE + 0x10001c))
#define PM_WDOG     ((volatile uint32_t*)(MMIO_BASE + 0x100024))

void reboot(uint32_t tick) {              // reboot after watchdog timer expire
    *PM_RSTC = (PM_PASSWORD | 0x20);  // full reset
    *PM_WDOG = (PM_PASSWORD | tick);  // number of watchdog tick
}

void cancel_reboot() {
    *PM_RSTC = (PM_PASSWORD | 0);  // no reset
    *PM_WDOG = (PM_PASSWORD | 0);  // number of watchdog tick
}