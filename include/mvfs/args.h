/*

    mVFS API
    
*/

#ifndef __LIBMVFS_ARGS_H
#define __LIBMVFS_ARGS_H

#include <hash.h>

typedef struct __mvfs_args	MVFS_ARGS;

struct __mvfs_args
{
    hash hashtable;
};

MVFS_ARGS*  mvfs_args_alloc();
MVFS_ARGS*  mvfs_args_from_url(const char* s);
int         mvfs_args_parse(const char* s);
int         mvfs_args_free(MVFS_ARGS* args);
int         mvfs_args_set(MVFS_ARGS* args, const char* name, const char* value);
const char* mvfs_args_get(MVFS_ARGS* args, const char* name);

#endif
