/*
    libmvfs - metux Virtual Filesystem Library

    Local filesystem API

    Copyright (C) 2008 Enrico Weigelt, metux IT service <weigelt@metux.de>
    This code is published under the terms of the GNU Public License 2.0
*/

#ifndef __LIBMVFS_HOSTFS_H
#define __LIBMVFS_HOSTFS_H

#include <mvfs/mvfs.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mvfs_hostfs_param
{
    const char* chroot;
    const char* maskuser;
    int         readonly;
} MVFS_HOSTFS_PARAM;

MVFS_FILESYSTEM* mvfs_hostfs_create(MVFS_HOSTFS_PARAM param);
MVFS_FILESYSTEM* mvfs_hostfs_create_args(MVFS_ARGS* args);

#ifdef __cplusplus
}
#endif

#endif
