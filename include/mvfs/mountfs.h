
#ifndef __LIBMVFS_MOUNTFS_H
#define __LIBMVFS_MOUNTFS_H

#include <mvfs/mvfs.h>

typedef struct 
{
    const char* options;
} MVFS_MOUNTFS_PARAM;

MVFS_FILESYSTEM* mvfs_mountfs_create(MVFS_MOUNTFS_PARAM param);
int              mvfs_mountfs_mount(MVFS_FILESYSTEM* fs, MVFS_FILESYSTEM* newfs, const char* mountpoint);

#endif
