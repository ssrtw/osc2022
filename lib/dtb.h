#ifndef _DTB_H
#define _DTB_H
#include "stddef.h"

#define FDT_MAGIC      0xD00DFEED
#define FDT_BEGIN_NODE 0x00000001
#define FDT_END_NODE   0x00000002
#define FDT_PROP       0x00000003
#define FDT_NOP        0x00000004
#define FDT_END        0x00000009
#define FDT_ALIGNMENT  4

typedef void (*fdt_callback)(uint32_t token, char* name, void* value, uint32_t value_len);

void* dtb_addr;

typedef struct {
    uint32_t magic;
    uint32_t totalsize;
    uint32_t off_dt_struct;
    uint32_t off_dt_strings;
    uint32_t off_mem_rsvmap;
    uint32_t version;
    uint32_t last_comp_version;
    uint32_t boot_cpuid_phys;
    uint32_t size_dt_strings;
    uint32_t size_dt_struct;
} fdt_header;

uint32_t big2little(uint32_t data);

void fdt_traverse(fdt_callback func);

void fdt_callback_initramfs(uint32_t token, char* name, void* value, uint32_t value_len);

#endif