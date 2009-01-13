/*
    libmvfs - metux Virtual Filesystem Library

    Some little helpers inlines for unix environments

    Copyright (C) 2008 Enrico Weigelt, metux IT service <weigelt@metux.de>
    This code is published under the terms of the GNU Public License 2.0
*/

#ifndef __LIBMVFS_UNIX_HLP_H
#define __LIBMVFS_UNIX_HELP_H

// FIXME !
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <inttypes.h>

#include <mvfs/stat.h>

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
}
#endif

#endif
