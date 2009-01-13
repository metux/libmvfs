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

MVFS_FILESYSTEM* mvfs_autoconnectfs_create();

// retrieve a list of open connections as an (malloc()'ed) text buffer
// each connection is represented by an string with newline (\n)
char*            mvfs_autoconnectfs_getconnections(MVFS_FILESYSTEM* fs);

#ifdef __cplusplus
}
#endif

#endif
