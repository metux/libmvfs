
#include <string.h>
#include <malloc.h>
#include <hash.h>
#include <stdio.h>

#include <mvfs/mvfs.h>
#include <errno.h>

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

    if (url[0] == '/')
    {
	// url starts with an slash -> local pathname
	MVFS_ARGS* args = mvfs_args_alloc();
	mvfs_args_set(args, "url",  url);
	mvfs_args_set(args, "type", "local");
	mvfs_args_set(args, "path", url);
	return args;
    }
    
    const char* dotpos = strchr(url,':');
    if (!dotpos)
    {
	fprintf(stderr,"mvfs_args_from_url() cannot parse url \"%s\" no : found\n", url);
	return NULL;
    }

    if (dotpos==url)
    {
	fprintf(stderr,"mvfs_args_from_url() cannot parse url \"%s\" empty proto\n", url);
	return NULL;
    }

    MVFS_ARGS* args = mvfs_args_alloc();
    mvfs_args_set (args, "url",  url);
    mvfs_args_setn(args, "type", url, (dotpos-url));

    url = dotpos+1;						// "//localhost:9999/foo/bar
    if (url[0]=='/')
    {
	url++;
	if (url[0]=='/') url++;
    
	if (url[0] == 0)			// url ends here. no host and no pathname
	{
	    fprintf(stderr,"mvfs_args_from_url() missing host and/or pathname\n");
	    return NULL;
	}
	
	if (url[0] == '/') 			// no host spec - evrything from this slash is localname
	{
	    mvfs_args_set(args,"path", url);
	    return args;
	}

	const char* pathname;
	if (!(pathname=strchr(url,'/')))
	    pathname=url+strlen(url);
	else
	    mvfs_args_set(args,"path",pathname);
	
	// okay, now we know where the pathname starts (may be 0-len, but not NULL)
	// and that the hostspec spans from [url..pathname-1]
	// BTW: url is already proven to be non-0len
	if ((dotpos=strchr(url,':')) && (dotpos<pathname))
	{
	    // we've got an host name spanning [url..dotpos-1]
	    // port spans [dotpos+1..pathname-1]
	    mvfs_args_setn(args,"host", url, dotpos-url);
	    mvfs_args_setn(args,"port", dotpos+1, pathname-dotpos-1);
	}
	else
	{
	    // we only got hostname (w/o port) spanning [url..pathname-1]
	    mvfs_args_setn(args,"host", url, pathname-url);
	}

	return args;
    }

    fprintf(stderr,"mvfs_args_from_url() cannot parse url \"%s\"\n", url);
    return NULL;
}
