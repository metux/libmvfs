/*
    libmvfs - metux Virtual Filesystem Library

    Client-side filesystem and file access API

    Copyright (C) 2008 Enrico Weigelt, metux IT service <weigelt@metux.de>
    This code is published under the terms of the GNU Public License 2.0
*/

#ifndef __LIBMVFS_FILEOPS_H
#define __LIBMVFS_FILEOPS_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <mvfs/types.h>
#include <mvfs/default_ops.h>
#include <mvfs/args.h>

#ifdef __cplusplus
extern "C" {
#endif

off64_t    mvfs_file_seek    (MVFS_FILE* fp, off64_t offset, int whence);
ssize_t    mvfs_file_read    (MVFS_FILE* fp, void* buf, size_t count);
ssize_t    mvfs_file_write   (MVFS_FILE* fp, const void* buf, size_t count);
ssize_t    mvfs_file_pread   (MVFS_FILE* fp, void* buf, size_t count, off64_t offset);
ssize_t    mvfs_file_pwrite  (MVFS_FILE* fp, const void* buf, size_t count, off64_t offset);
int        mvfs_file_setflag (MVFS_FILE* fp, MVFS_FILE_FLAG flag, long value);
int        mvfs_file_getflag (MVFS_FILE* fp, MVFS_FILE_FLAG flag, long* value);
MVFS_STAT* mvfs_file_stat    (MVFS_FILE* fp);
MVFS_FILE* mvfs_file_alloc   (MVFS_FILESYSTEM* fs, MVFS_FILE_OPS ops);
int        mvfs_file_unref   (MVFS_FILE* file);
int        mvfs_file_ref     (MVFS_FILE* file);
MVFS_FILE* mvfs_file_lookup  (MVFS_FILE* file, const char* name);
MVFS_STAT* mvfs_file_scan    (MVFS_FILE* file);
int        mvfs_file_reset   (MVFS_FILE* file);

MVFS_FILE*       mvfs_fs_openfile (MVFS_FILESYSTEM* fs, const char* name, mode_t mode);
MVFS_STAT*       mvfs_fs_statfile (MVFS_FILESYSTEM* fs, const char* name);
int              mvfs_fs_ref      (MVFS_FILESYSTEM* fs);
int              mvfs_fs_unref    (MVFS_FILESYSTEM* fs);
int              mvfs_fs_unlink   (MVFS_FILESYSTEM* fs, const char* name);
MVFS_SYMLINK     mvfs_fs_readlink (MVFS_FILESYSTEM* fs, const char* name);
int              mvfs_fs_symlink  (MVFS_FILESYSTEM* fs, const char* n1, const char* n2);
int              mvfs_fs_rename   (MVFS_FILESYSTEM* fs, const char* n1, const char* n2);
int              mvfs_fs_chmod    (MVFS_FILESYSTEM* fs, const char* filename, mode_t mode);
int              mvfs_fs_mkdir    (MVFS_FILESYSTEM* fs, const char* filename, mode_t mode);
int              mvfs_fs_chown    (MVFS_FILESYSTEM* fs, const char* filename, const char* uid, const char* gid);
MVFS_FILESYSTEM* mvfs_fs_alloc    (MVFS_FILESYSTEM_OPS ops, const char* magic);

// stolen from BSD ... hope they don't hit me for license incompatibility ;-O
void mvfs_strmode(mode_t mode, char* p);

MVFS_FILESYSTEM* mvfs_fs_create_args(MVFS_ARGS* args);

static inline int _mvfs_check_magic(MVFS_FILESYSTEM* fs, const char* magic, const char* fsname)
{
    if (fs == NULL)
    {
	fprintf(stderr,"_mvfs_check_magic() [%s] NULL fs\n", fsname);
	return 0;
    }
    if (magic == NULL)
    {
	fprintf(stderr,"_mvfs_check_magic() [%s] NULL magic param\n", fsname);
	return 0;
    }
    if (fs->magic == NULL)
    {
	fprintf(stderr,"_mvfs_check_magic() [%s] fs has NULL magic\n", fsname);
	return 0;
    }
    if (strcmp(fs->magic, magic))
    {
	fprintf(stderr,"_mvfs_check_magic() [%s] magic mismatch: IS=\"%s\" REQ=\"%s\"\n", fsname, fs->magic, magic);
	return 0;
    }
    
    return 1;
}

// check whether given filename exists and points to an directory
static inline int mvfs_fs_is_dir(MVFS_FILESYSTEM* fs, const char* filename)
{
    MVFS_STAT* st = mvfs_fs_statfile(fs, filename);
    if (st == NULL)
	return 0;
	
    if (S_ISDIR(st->mode))
    {
	mvfs_stat_free(st);
	return 1;
    }
    else
    {
	mvfs_stat_free(st);
	return 0;
    }
}

// check whether given filename exists and points to an regular file
static inline int mvfs_fs_is_regfile(MVFS_FILESYSTEM* fs, const char* filename)
{
    MVFS_STAT* st = mvfs_fs_statfile(fs, filename);
    if (st == NULL)
	return 0;
	
    if (S_ISREG(st->mode))
    {
	mvfs_stat_free(st);
	return 1;
    }
    else
    {
	mvfs_stat_free(st);
	return 0;
    }
}

// check whether given filename exists
static inline int mvfs_fs_exists(MVFS_FILESYSTEM* fs, const char* filename)
{
    MVFS_STAT* st = mvfs_fs_statfile(fs, filename);
    if (st == NULL)
	return 0;

    mvfs_stat_free(st);
    return 1;
}

#ifdef __cplusplus
}
#endif

#endif
