/*
    libmvfs - metux Virtual Filesystem Library

    9P filesystem (via libmixp) API

    Copyright (C) 2008 Enrico Weigelt, metux IT service <weigelt@metux.de>
    This code is published under the terms of the GNU Public License 2.0
*/

#ifndef __LIBMVFS_MIXPFS_H
#define __LIBMVFS_MIXPFS_H

#include <mvfs/mvfs.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mvfs_mixpfs_param
{
//    const char* chroot;
//    const char* maskuser;
//    int         readonly;
    const char*	address;
} MVFS_MIXPFS_PARAM;

// MVFS_FILESYSTEM* mvfs_mixpfs_create(MVFS_MIXPFS_PARAM param);
MVFS_FILESYSTEM* mvfs_mixpfs_create_args(MVFS_ARGS* args);

#ifdef __cplusplus
}
#endif

#endif
