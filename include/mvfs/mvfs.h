/*
    libmvfs - metux Virtual Filesystem Library

    Main include file

    Copyright (C) 2008 Enrico Weigelt, metux IT service <weigelt@metux.de>
    This code is published under the terms of the GNU Public License 2.0
*/

#ifndef __LIBMVFS_MVFS_H
#define __LIBMVFS_MVFS_H

#include <mvfs/types.h>
#include <mvfs/stat.h>
#include <mvfs/fileops.h>
#include <fcntl.h>

// experimental functions which run in an separate background process
// might be totally unstable
int mvfs_fs_read_fd(MVFS_FILESYSTEM* fs, const char* name);
int mvfs_fs_ls_fd(MVFS_FILESYSTEM* fs, const char* name);

#endif
