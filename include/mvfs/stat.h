/*

    mVFS API
    
*/

#ifndef __LIBMVFS_STAT_H
#define __LIBMVFS_STAT_H

// FIXME !
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <sys/types.h>
#include <inttypes.h>

typedef struct __mvfs_stat      MVFS_STAT;

struct __mvfs_stat
{
    const char* name;
    const char* uid;
    const char* gid;
    mode_t      mode;
    long        size;
    time_t      atime;
    time_t      mtime;
    time_t      ctime;
};

int        mvfs_stat_free  (MVFS_STAT* st);
MVFS_STAT* mvfs_stat_alloc (const char* name, const char* uid, const char* gid);
MVFS_STAT* mvfs_stat_dup   (MVFS_STAT* st);

#endif
