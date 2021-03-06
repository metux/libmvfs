/*
    libmvfs - metux Virtual Filesystem Library

    File-Status API

    Copyright (C) 2008 Enrico Weigelt, metux IT service <weigelt@metux.de>
    This code is published under the terms of the GNU Public License 2.0
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

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
}
#endif

#endif
