/*
    libmvfs - metux Virtual Filesystem Library

    Mountable filesystem API

    Copyright (C) 2008 Enrico Weigelt, metux IT service <weigelt@metux.de>
    This code is published under the terms of the GNU Public License 2.0
*/

#ifndef __LIBMVFS_MOUNTFS_H
#define __LIBMVFS_MOUNTFS_H

#include <mvfs/mvfs.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct 
{
    const char* options;
} MVFS_MOUNTFS_PARAM;

MVFS_FILESYSTEM* mvfs_mountfs_create(MVFS_MOUNTFS_PARAM param);
int              mvfs_mountfs_mount(MVFS_FILESYSTEM* fs, MVFS_FILESYSTEM* newfs, const char* mountpoint);

#ifdef __cplusplus
}
#endif

#endif
