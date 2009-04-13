/*
    libmvfs - metux Virtual Filesystem Library

    Auto-connecting filesystem API

    Copyright (C) 2008 Enrico Weigelt, metux IT service <weigelt@metux.de>
    This code is published under the terms of the GNU Public License 2.0
*/

#ifndef __MVFS_AUTOCONNECT_OPS_H
#define __MVFS_AUTOCONNECT_OPS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MVFS_CONNECTOR_PREFIXMAP MVFS_CONNECTOR_PREFIXMAP;

struct MVFS_CONNECTOR_PREFIXMAP
{
    MVFS_CONNECTOR_PREFIXMAP* next;

    const char* prefix;		/* the prefix to match on, eg. URL schema */
    const char* newprefix;	/* a prefix to map to (optional) */
};

MVFS_FILESYSTEM* mvfs_connector_create_args(MVFS_ARGS* args);

#ifdef __cplusplus
}
#endif

#endif
