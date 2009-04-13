/*
    libmvfs - metux Virtual Filesystem Library

    Argument list handling

    Copyright (C) 2008 Enrico Weigelt, metux IT service <weigelt@metux.de>
    This code is published under the terms of the GNU Public License 2.0
*/

#include "mvfs-internal.h"

#include <string.h>
#include <malloc.h>
#include <hash.h>
#include <stdio.h>
#include <errno.h>

#include <mvfs/mvfs.h>
#include <mvfs/url.h>
#include <mvfs/_utils.h>

struct __mvfs_args
{
    hash hashtable;
};

// not very efficient yet, but should work for now

MVFS_ARGS* mvfs_args_alloc()
{
    MVFS_ARGS* args = calloc(1,sizeof(MVFS_ARGS));
    if (!hash_initialise(&(args->hashtable), 997U, hash_hash_string, hash_compare_string, hash_copy_string, free, free))
    {
	ERRMSG("hash init failed");
	free(args);
	return NULL;
    }
    return args;
}

const char* mvfs_args_get(MVFS_ARGS* args, const char* name)
{
    if (args==NULL)
	return NULL;

    const char* str = NULL;
    if (hash_retrieve(&(args->hashtable), (char*)name, (void**)&str))
	return str;

    return NULL;
}

int mvfs_args_set(MVFS_ARGS* args, const char* name, const char* value)
{
    if (args == NULL)
	return -1;

    hash_delete(&(args->hashtable), (char*)name);
    if (value==NULL)
	return 0;
	
    if (!hash_insert(&(args->hashtable), strdup(name), strdup(value)))
	ERRMSG("failed for key %s", name);

    return 1;
}

int mvfs_args_setn(MVFS_ARGS* args, const char* name, const char* value, int sz)
{
    if (args == NULL)
	return -1;

    hash_delete(&(args->hashtable), (char*)name);
    if (value==NULL)
	return 0;
	
    if (!hash_insert(&(args->hashtable), strdup(name), strndup(value,sz)))
	ERRMSG("failed for key %s", name);

    return 1;
}

int mvfs_args_free(MVFS_ARGS* args)
{
    if (args==NULL)
	return -EFAULT;

    hash_deinitialise(&(args->hashtable));
    return 0;
}

MVFS_ARGS* mvfs_args_from_url(const char* url)
{
    if (url==NULL)
	return NULL;

    MVFS_URL* u = mvfs_url_parse(url);
    MVFS_ARGS* args = mvfs_args_alloc();

    if ((!(u->type)) || (!(u->type[0])))
	u->type = strdup("local");

    size_t s = strlen(url)*2;
    char* buffer = (char*)malloc(strlen(url)*2);

    strcpy(buffer, u->type);
    strcat(buffer, "://");
    if (u->username)
    {
	strcat(buffer, u->username);
	if (u->secret)
	{
	    strcat(buffer, ":");
	    strcat(buffer, u->secret);
	}
	strcat(buffer, "@");
    }
    if (u->hostname)
    {
	strcat(buffer, u->hostname);
	if (u->port)
	{
	    strcat(buffer, ":");
	    strcat(buffer, u->port);
	}
    }
    strcat(buffer, "/");
    if (u->pathname)
    {
	const char* p = u->pathname;
	while (p[0] == '/')
	    p++;
	strcat(buffer, p);
    }

    mvfs_args_set(args,"url",              url);
    mvfs_args_set(args,MVFS_ARGS_PATH,     u->pathname);
    mvfs_args_set(args,MVFS_ARGS_HOSTNAME, u->hostname);
    mvfs_args_set(args,MVFS_ARGS_PORT,     u->port);
    mvfs_args_set(args,MVFS_ARGS_USERNAME, u->username);
    mvfs_args_set(args,MVFS_ARGS_SECRET,   u->secret);
    mvfs_args_set(args,MVFS_ARGS_TYPE,     u->type);
    mvfs_args_set(args,"normalized-url",   buffer);
    return args;
}

char* mvfs_args_to_url(MVFS_ARGS* args)
{
    const char* type   = mvfs_args_get_def(args, MVFS_ARGS_TYPE,     "file");
    const char* host   = mvfs_args_get_def(args, MVFS_ARGS_HOSTNAME, "");
    const char* user   = mvfs_args_get_def(args, MVFS_ARGS_USERNAME, "");
    const char* port   = mvfs_args_get_def(args, MVFS_ARGS_PORT,     "");
    const char* secret = mvfs_args_get_def(args, MVFS_ARGS_SECRET,   "");
    const char* path   = mvfs_args_get_def(args, MVFS_ARGS_PATH,     "");

    size_t sz = strlen(type)+strlen(host)+strlen(user)+strlen(secret)+strlen(port)+strlen(path);
    char* buffer = (char*)malloc(strlen+20);

    strcpy(buffer, type);
    strcat(buffer, "://");
    if (user[0])
    {
	strcat(buffer, user);
	if (secret[0])
	{
	    strcat(buffer, ":");
	    strcat(buffer, secret);
	}
	strcat(buffer, "@");
    }
    if (host[0])
    {
	strcat(buffer, host);
	if (port[0])
	{
	    strcat(buffer, ":");
	    strcat(buffer, port);
	}
    }
    strcat(buffer, "/");
    while (path[0]=='/')
	path++;

    strcat(buffer, path);

    return buffer;
}
