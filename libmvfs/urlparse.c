/*
    libmvfs - metux Virtual Filesystem Library

    tiny URL parsing function

    Copyright (C) 2008 Enrico Weigelt, metux IT service <weigelt@metux.de>
    This code is published under the terms of the GNU Public License 2.0
*/

#include <string.h>
#include <malloc.h>
#include <stdio.h>
#include <mvfs/url.h>
#include <mvfs/_utils.h>

#define RET_OK		\
    {			\
	url->error = 0;	\
	return url;	\
    }

#define RET_ERR(err)					\
    {							\
	url->error = err;				\
	DEBUGMSG("(%s) error: %d \n", url->url, err);	\
	return url;					\
    }

MVFS_URL* mvfs_url_parse(const char* u)
{
    MVFS_URL* url;
    url = (MVFS_URL*)calloc(1,sizeof(MVFS_URL));
    url->error = MVFS_ERR_URL_MISSING_TYPE;

    char* ptr1 = (char*)&(url->buffer);
    char* ptr2 = NULL;
    char* ptr3 = NULL;

    strcpy(url->buffer, u);
    strcpy(url->url,    u);

    // check if this might be an abolute local filename
    if ((*ptr1)=='/')
    {
	url->pathname = ptr1;
	RET_OK;
    }

    // first try to get the url type - it's separated by a :
    if (!(ptr2 = strchr(ptr1, ':')))
	RET_ERR(MVFS_ERR_URL_MISSING_TYPE);

    *ptr2 = 0;		// terminate and set the type field
    url->type = ptr1;
    ptr1 = ptr2+1;	// jump right after the :

    // now we expect two slashes
    if (!((ptr1[0] == '/') && (ptr1[1] == '/')))
	RET_ERR(MVFS_ERR_URL_MISSING_SLASHSLASH);
    ptr1+=2;

    // if the url ends here, we've got something like "file://"
    // quite strange, but maybe correct ?
    if (!(*ptr1))
	RET_OK;

    // if we have have an / here, we've got no hostspec, just path
    if ((*ptr1)=='/')
    {
	url->pathname = ptr1;
	RET_OK;
    }

    // parse the host spec ...
    // do we have some @ ? - then cut out the username(+secret)
    if (ptr2 = strchr(ptr1,'@'))
    {
	(*ptr2) = 0;
	if (url->secret = strchr(ptr1,':'))	// got username + secret
	{
	    (*url->secret) = 0;
	    url->secret++;
	}
	url->username = ptr1;
	ptr1 = ptr2+1;
    }

    {
	char* ptr_slash = strchr(ptr1, '/');
	char* ptr_colon = strchr(ptr1, ':');

	if (ptr_slash)
	{
	    // pathname found. if we found a colon behind it, ignore
	    if (ptr_colon > ptr_slash)
		ptr_colon = 0;
	    // it's a bit tricky - we must make at least 1 byte more room ;-o
	    char pathname[MVFS_URL_MAX];
	    strcpy(pathname, ptr_slash);
	    *ptr_slash = 0;
	    ptr_slash++;
	    *ptr_slash = 0;
	    ptr_slash++;
	    strcpy(ptr_slash, pathname);
	    url->pathname = ptr_slash;
	}
	
	if (ptr_colon)
	{
	
	    *ptr_colon = 0;
	    ptr_colon++;
	    url->port = ptr_colon;
	}
	
	url->hostname = ptr1;
    }

    RET_OK;
}
