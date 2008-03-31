
#ifndef __LIBMVFS_HOSTFS_H
#define __LIBMVFS_HOSTFS_H

#include <mvfs/mvfs.h>

typedef struct mvfs_hostfs_param
{
    const char* chroot;
    const char* maskuser;
    int         readonly;
} MVFS_HOSTFS_PARAM;

MVFS_FILESYSTEM* mvfs_hostfs_create(MVFS_HOSTFS_PARAM param);
MVFS_FILESYSTEM* mvfs_hostfs_create_args(MVFS_ARGS* args);

#endif
