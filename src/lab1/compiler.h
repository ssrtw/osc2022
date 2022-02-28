#ifndef _COMPILER_H
#define _COMPILER_H

typedef unsigned char uchar;
typedef unsigned char byte;
typedef unsigned int uint;
typedef unsigned long size_t;
typedef unsigned long ulong;

#define likely(x) __builtin_expect(!!(x), 1)    // x很可能為真
#define unlikely(x) __builtin_expect(!!(x), 0)  // x很可能為假

#endif