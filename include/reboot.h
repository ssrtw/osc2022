#ifndef _REBOOT_H
#define _REBOOT_H

#include "stddef.h"

void reboot(uint32_t tick);
void cancel_reboot();

#endif