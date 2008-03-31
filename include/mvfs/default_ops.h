
#ifndef __LIBMVFS_DEFAULT_OPS_H
#define __LIBMVFS_DEFAULT_OPS_H

#include <mvfs/mvfs.h>

#ifdef __cplusplus
extern "C" {
#endif

int        mvfs_default_fileops_reopen  (MVFS_FILE* fp, mode_t mode);
off64_t    mvfs_default_fileops_seek    (MVFS_FILE* fp, off64_t offset, int whence);
ssize_t    mvfs_default_fileops_pread   (MVFS_FILE* fp, void* buf, size_t count, off64_t offset);
ssize_t    mvfs_default_fileops_pwrite  (MVFS_FILE* fp, const void* buf, size_t count, off64_t offset);
ssize_t    mvfs_default_fileops_read    (MVFS_FILE* fp, void* buf, size_t count);
ssize_t    mvfs_default_fileops_write   (MVFS_FILE* fp, const void* buf, size_t count);
int        mvfs_default_fileops_setflag (MVFS_FILE* fp, MVFS_FILE_FLAG flag, long value);
int        mvfs_default_fileops_getflag (MVFS_FILE* fp, MVFS_FILE_FLAG flag, long* value);
MVFS_STAT* mvfs_default_fileops_stat    (MVFS_FILE* fp);
int        mvfs_default_fileops_close   (MVFS_FILE* fp);
int        mvfs_default_fileops_eof     (MVFS_FILE* fp);
int        mvfs_default_fileops_free    (MVFS_FILE* fp);
MVFS_STAT* mvfs_default_fileops_scan    (MVFS_FILE* fp);
int        mvfs_default_fileops_reset   (MVFS_FILE* fp);
MVFS_FILE* mvfs_default_fileops_lookup  (MVFS_FILE* fp, const char* name);

MVFS_FILE* mvfs_default_fsops_openfile (MVFS_FILESYSTEM* fs, const char* name, mode_t mode);
MVFS_STAT* mvfs_default_fsops_stat     (MVFS_FILESYSTEM* fs, const char* name);
int        mvfs_default_fsops_unlink   (MVFS_FILESYSTEM* fs, const char* name);
int        mvfs_default_fsops_free     (MVFS_FILESYSTEM* fs);

#ifdef __cplusplus
}
#endif

#endif
