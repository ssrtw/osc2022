#include "util.h"

uint64_t align_up(uint64_t n, uint64_t align) {
    return (n + align - 1) & (~(align - 1));
}
