#ifndef _CPIO_H
#define _CPIO_H

#include "stddef.h"

#define CPIO_HEADER_MAGIC "070701"
#define CPIO_FOOTER_MAGIC "TRAILER!!!"
#define CPIO_ALIGNMENT    4

void *cpio_ramfs;

struct cpio_header {
    char c_magic[6];    /* Magic header '070701'. */
    char c_ino[8];      /* "i-node" number. */
    char c_mode[8];     /* Permisions. */
    char c_uid[8];      /* User ID. */
    char c_gid[8];      /* Group ID. */
    char c_nlink[8];    /* Number of hard links. */
    char c_mtime[8];    /* Modification time. */
    char c_filesize[8]; /* File size. */
    char c_devmajor[8]; /* Major dev number. */
    char c_devminor[8]; /* Minor dev number. */
    char c_rdevmajor[8];
    char c_rdevminor[8];
    char c_namesize[8]; /* Length of filename in bytes. */
    char c_check[8];    /* Checksum. */
};

struct cpio_info {
    /// The number of files in the CPIO archive
    uint32_t file_count;
    /// The maximum size of a file name
    uint32_t max_path_sz;
};

void *cpio_get_file(const char *name, uint64_t *size);

void cpio_ls();

#endif
