
#ifndef __MVFS_ROSTRING_FILE_H
#define __MVFS_ROSTRING_FILE_H

#include <mvfs/mvfs.h>

/* -- file access to an in-memory string (read-only) -- */
int        mvfs_rostring_ops_init  (MVFS_FILE* f, const char* filename);
int        mvfs_rostring_ops_reopen(MVFS_FILE* f, mode_t mode);
ssize_t    mvfs_rostring_ops_read  (MVFS_FILE* f, void* buf, size_t size);
ssize_t    mvfs_rostring_ops_pread (MVFS_FILE* f, void* buf, size_t size, off64_t offset);
off64_t    mvfs_rostring_ops_size  (MVFS_FILE* f);
MVFS_FILE* mvfs_rostring_ops_create(const char* str);

static MVFS_FILE_OPS mvfs_rostring_ops = {
    .reopen 	= mvfs_rostring_ops_reopen,
    .read 	= mvfs_rostring_ops_read,
    .pread      = mvfs_rostring_ops_pread
//    .size 	= mvfs_rostring_ops_size,
//    .classname 	= "rostring"
};

#endif
