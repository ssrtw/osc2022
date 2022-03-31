#ifndef _MBOX_H
#define _MBOX_H

#include "stddef.h"

void get_board_revision(uint32_t *res);
void get_arm_memory(uint32_t *addr, uint32_t *size);

#endif