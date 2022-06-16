#include "dtb.h"

#include "cpio.h"
#include "string.h"
#include "uart.h"
#include "util.h"

#define initram_name "linux,initrd-"

uint32_t big2little(uint32_t data) {
    return ((data >> 24) & 0xff) |
           ((data >> 8) & 0xff00) |
           ((data << 8) & 0xff0000) |
           ((data << 24) & 0xff000000);
}

uint64_t big2little64(uint64_t data) {
    return ((data >> 56) & 0xff) |
           ((data >> 40) & 0xff00) |
           ((data >> 24) & 0xff0000) |
           ((data >> 8) & 0xff000000) |
           ((data << 8) & 0xff00000000) |
           ((data << 24) & 0xff0000000000) |
           ((data << 40) & 0xff000000000000) |
           ((data << 56) & 0xff00000000000000);
}

void fdt_traverse(fdt_callback callback) {
    fdt_header *header = dtb_addr;

    if (big2little(header->magic) != FDT_MAGIC) {
        uart_puts("[Error] dtb header magic is error value!\n");
        return;
    }
    uint32_t size = big2little(header->size_dt_struct);
    byte *ptr = (byte *)((byte *)header + big2little(header->off_dt_struct));
    byte *name_start = (byte *)((byte *)header + big2little(header->off_dt_strings));

    byte *end = (byte *)(ptr + size);
    while (ptr < end) {
        uint32_t token = big2little(*(uint32_t *)ptr);
        ptr += 4;
        if (token == FDT_BEGIN_NODE) {
            callback(token, ptr, 0, 0);
            //如果字串不是4個byte對齊，就要算一下位置(Doc:The node name is followed by zeroed padding bytes, if necessary for alignment)
            ptr += strlen(ptr) + 1;                        //+1 with \0
            ptr = align_up((uint64_t)ptr, FDT_ALIGNMENT);  // 如果字串長度不是4byte對齊的話，要對齊
        } else if (token == FDT_PROP) {
            uint32_t len = big2little(*(uint32_t *)ptr);
            ptr += 4;
            byte *name = (byte *)(name_start + big2little(*(uint32_t *)ptr));
            ptr += 4;
            callback(token, name, ptr, len);
            ptr += len;
            ptr = align_up((uint64_t)ptr, FDT_ALIGNMENT);  // 如果字串長度不是4byte對齊的話，要對齊
        } else if (token == FDT_END_NODE || token == FDT_NOP || token == FDT_END) {
            callback(token, 0, 0, 0);  // nothing after this token
        } else {
            uart_printf("[Error] token value error: %x\n", token);
        }
    }
}

void fdt_callback_initramfs(uint32_t token, char *name, void *value, uint32_t value_len) {
    if (token != FDT_PROP) return;
    if (!strncmp(name, initram_name "start", sizeof(initram_name "start"))) {
        cpio_ramfs = (void *)(size_t)big2little(*(uint32_t *)value);
    }
    if (!strncmp(name, initram_name "end", sizeof(initram_name "end"))) {
        cpio_ramfs_end = (void *)(size_t)big2little(*(uint32_t *)value);
    }
}

void dtb_reserve_mem(void (*reserve_page_ptr)(size_t, size_t)) {
    fdt_header *header = dtb_addr;

    if (big2little(header->magic) != FDT_MAGIC) {
        uart_puts("[Error] dtb header magic is error value!\n");
        return;
    }
    // Devicetree Specification 5.3 Memory Reservation Block
    fdt_reserve_entry_t *fdt_rsv_entry = (fdt_reserve_entry_t *)((byte *)header + big2little(header->off_mem_rsvmap));
    uint64_t reserve_mem_start = big2little64(fdt_rsv_entry->address);
    uint64_t reserve_mem_end = reserve_mem_start + big2little64(fdt_rsv_entry->size);
    reserve_page_ptr(reserve_mem_start, reserve_mem_end);
}