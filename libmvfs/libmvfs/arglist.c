#include <string.h>
#include <malloc.h>
#include <hash.h>
#include <stdio.h>
#include <errno.h>

#include <mvfs/mvfs.h>
#include <mvfs/url.h>

// not very efficient yet, but should work for now

MVFS_ARGS* mvfs_args_alloc()
{
    MVFS_ARGS* args = calloc(1,sizeof(MVFS_ARGS));
    if (!hash_initialise(&(args->hashtable), 997U, hash_hash_string, hash_compare_string, hash_copy_string, free, free))
    {
	fprintf(stderr,"mvfs_args_alloc() hash init failed\n");
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
	fprintf(stderr,"mvfs_args_set failed for key %s\n", name);

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
	fprintf(stderr,"mvfs_args_setn failed for key %s\n", name);

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
    
    mvfs_args_set(args,"url",      url);
    mvfs_args_set(args,"path",     u->pathname);
    mvfs_args_set(args,"hostname", u->hostname);
    mvfs_args_set(args,"port",     u->port);
    mvfs_args_set(args,"username", u->username);
    mvfs_args_set(args,"secret",   u->secret);

    return args;
}
