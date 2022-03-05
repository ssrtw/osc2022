#ifndef _MBOX_H
#define _MBOX_H

#include "stddef.h"

void get_board_revision(uint *res);
void get_arm_memory(uint *addr, uint *size);

#endif