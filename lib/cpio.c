#include "cpio.h"

#include "malloc.h"
#include "sched.h"
#include "string.h"
#include "uart.h"
#include "util.h"

/* Parse an ASCII hex string into an integer. */
static uint64_t parse_hex_str(char *s, uint32_t max_len) {
    uint64_t r = 0;
    uint64_t i;

    for (i = 0; i < max_len; i++) {
        r <<= 4;
        if (s[i] >= '0' && s[i] <= '9') {
            r += s[i] - '0';
        } else if (s[i] >= 'a' && s[i] <= 'f') {
            r += s[i] - 'a' + 10;
        } else if (s[i] >= 'A' && s[i] <= 'F') {
            r += s[i] - 'A' + 10;
        } else {
            return r;
        }
        continue;
    }
    return r;
}

int cpio_parse_header(void *now, const char **filename, uint64_t *_filesize, void **data,
                      struct cpio_header **next) {
    uint64_t filesize;
    struct cpio_header *archive = now;
    /* Ensure magic header exists. */
    if (strncmp(archive->c_magic, CPIO_HEADER_MAGIC,
                sizeof(archive->c_magic)) != 0)
        return -1;

    /* Get filename and file size. */
    filesize = parse_hex_str(archive->c_filesize, sizeof(archive->c_filesize));
    *filename = ((char *)archive) + sizeof(struct cpio_header);

    /* Ensure filename is not the trailer indicating EOF. */
    if (strncmp(*filename, CPIO_FOOTER_MAGIC, sizeof(CPIO_FOOTER_MAGIC)) == 0)
        return 1;

    // 解檔名長度
    uint64_t filename_length = parse_hex_str(archive->c_namesize, sizeof(archive->c_namesize));
    *data = (void *)align_up(((uint64_t)archive) + sizeof(struct cpio_header) + filename_length, CPIO_ALIGNMENT);
    *next = (struct cpio_header *)align_up(((uint64_t)*data) + filesize, CPIO_ALIGNMENT);
    if (_filesize) {
        *_filesize = filesize;
    }
    return 0;
}

cpio_file_info *cpio_traverse(const char *name, cpio_callback func) {
    uint64_t size;
    struct cpio_header *header = cpio_ramfs;
    uint32_t input_len, len;
    input_len = strlen(name);
    for (;;) {
        struct cpio_header *next;
        void *result;
        const char *current_filename;

        int error = cpio_parse_header(header, &current_filename, &size, &result, &next);
        if (error)
            return -1;
        len = strlen(current_filename);
        if (input_len == len && strncmp(current_filename, name, len) == 0) {
            if (func != NULL) {
                func(result, size);
                return NULL;
            } else {
                cpio_file_info *cfi = kmalloc(sizeof(cpio_file_info));
                cfi->start = (uint64_t)result;
                cfi->size = size;
                return cfi;
            }
        }
        header = next;
    }
    return -1;
}

void cpio_catfile(void *data, size_t size) {
    uart_putn(data, size);
}

void cpio_exec(void *data, size_t size) {
    exec_thread((byte *)data, size);
}

void cpio_ls() {
    const char *current_filename;
    struct cpio_header *header, *next;
    void *result;
    int error;
    uint64_t size;

    header = cpio_ramfs;
    while (1) {
        error = cpio_parse_header(header, &current_filename, &size, &result, &next);
        if (error) break;
        uart_printf("%s\n", current_filename);
        header = next;
    }
}

void cpio_get_data() {
    const char *current_filename;
    struct cpio_header *header, *next;
    void *result;
    int error;
    uint64_t size;

    header = cpio_ramfs;
    while (1) {
        error = cpio_parse_header(header, &current_filename, &size, &result, &next);
        if (error) break;
        uart_printf("%s\n", current_filename);
        header = next;
    }
}