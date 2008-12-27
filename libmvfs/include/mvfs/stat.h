/*

    mVFS API

*/

#ifndef __LIBMVFS_STAT_H
#define __LIBMVFS_STAT_H

// FIXME !
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <unistd.h>
#include <sys/stat.h>
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

/* Free an MVFS_STAT structure. - returns error on NULL ptr passed */
int        mvfs_stat_free  (MVFS_STAT* st);
/* Allocate a new MVFS_STAT structure, initialized w/ given parameters (copied) */
MVFS_STAT* mvfs_stat_alloc (const char* name, const char* uid, const char* gid);
/* Duplicate (copy) an given MVFS_STAT structure */
MVFS_STAT* mvfs_stat_dup   (MVFS_STAT* st);

// convert an MVFS_STAT to unix struct stat
static inline struct stat mvfs_stat_to_unix(MVFS_STAT* st)
{
    if (st==NULL)
	return ((struct stat){});

    return ((struct stat)
    {
	.st_mode  = st->mode,
	.st_atime = st->atime,
	.st_mtime = st->mtime,
	.st_ctime = st->ctime,
	.st_size  = st->size,
	.st_uid   = 0,			// FIXME !!!
	.st_gid   = 0,			// FIXME !!!
	.st_nlink = 1
    });
}

static inline struct stat mvfs_stat_to_unix_unref(MVFS_STAT* stat)
{
    struct stat s2 = mvfs_stat_to_unix(stat);
    mvfs_stat_free(stat);
    return s2;
}

#endif
