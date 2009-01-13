/*
    libmvfs - metux Virtual Filesystem Library

    Metadata-caching filesystem API

    Copyright (C) 2008 Enrico Weigelt, metux IT service <weigelt@metux.de>
    This code is published under the terms of the GNU Public License 2.0
*/

#ifndef __MVFS_METACACHE_OPS_H
#define __MVFS_METACACHE_OPS_H

#include <mvfs/mvfs.h>

#ifdef __cplusplus
extern "C" {
#endif

MVFS_FILESYSTEM* mvfs_metacachefs_create_1(MVFS_FILESYSTEM* fs);

#ifdef __cplusplus
}
#endif

#endif
