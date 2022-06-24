#ifndef _MBOX_H
#define _MBOX_H

#include "stddef.h"

extern volatile uint32_t mbox[64];

int mbox_call(uint32_t ch);
void get_board_revision(uint32_t *res);
void get_arm_memory(uint32_t *addr, uint32_t *size);

#endif