
#ifndef __LIBMVFS_MIXPFS_H
#define __LIBMVFS_MIXPFS_H

#include <mvfs/mvfs.h>

typedef struct mvfs_mixpfs_param
{
//    const char* chroot;
//    const char* maskuser;
//    int         readonly;
    const char*	address;
} MVFS_MIXPFS_PARAM;

// MVFS_FILESYSTEM* mvfs_mixpfs_create(MVFS_MIXPFS_PARAM param);
MVFS_FILESYSTEM* mvfs_mixpfs_create_args(MVFS_ARGS* args);

#endif
