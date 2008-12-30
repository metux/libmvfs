/*
    libmvfs - metux Virtual Filesystem Library

    Fish filesystem API

    Copyright (C) 2008 Enrico Weigelt, metux IT service <weigelt@metux.de>
    This code is published under the terms of the GNU Public License 2.0
*/

#ifndef __LIBMVFS_FISH_FS_H
#define __LIBMVFS_FISH_FS_H

#include <mvfs/mvfs.h>

#ifdef __cplusplus
extern "C" {
#endif

MVFS_FILESYSTEM* mvfs_fishfs_create_args(MVFS_ARGS* args);

#ifdef __cplusplus
}
#endif

#endif
