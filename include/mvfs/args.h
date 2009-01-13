/*
    libmvfs - metux Virtual Filesystem Library

    Arguments/Parameters handling

    Copyright (C) 2008 Enrico Weigelt, metux IT service <weigelt@metux.de>
    This code is published under the terms of the GNU Public License 2.0
*/

#ifndef __LIBMVFS_ARGS_H
#define __LIBMVFS_ARGS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct __mvfs_args	MVFS_ARGS;

/* allocate an empty args structure */
MVFS_ARGS*  mvfs_args_alloc();

/* free an given args structure */
int         mvfs_args_parse(const char* s);

/* set an argument value */
int         mvfs_args_set(MVFS_ARGS* args, const char* name, const char* value);

/* get an argument value - not that the result is readonly and only of temporary lifetime - strdup() asap */
const char* mvfs_args_get(MVFS_ARGS* args, const char* name);

/* parse an URL into an argument list object */
MVFS_ARGS*  mvfs_args_from_url(const char* s);

#ifdef __cplusplus
}
#endif

#endif
