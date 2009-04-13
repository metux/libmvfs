/*
    libmvfs - metux Virtual Filesystem Library

    This is an simple auto-connecting overlay fs.
    It takes in URLs and automatically loads/connects backend fs'es on demand,
    so an client doesn't have to cope with individual filesystems anymore

    The functionality is a little bit like MC's vfs

    Copyright (C) 2008 Enrico Weigelt, metux IT service <weigelt@metux.de>
    This code is published under the terms of the GNU Public License 2.0
*/

#define __DEBUG

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <mvfs/mvfs.h>
#include <mvfs/autoconnect_ops.h>
#include <mvfs/_utils.h>

#define FS_MAGIC	"metux/autoconnect-fs-1.0"

#define __FILEOPS_HEAD(err);					\
	if (file==NULL)						\
	{							\
	    ERRMSG("NULL file handle");				\
	    return err;						\
	}							\
	METACACHE_FILE_PRIV* priv = (file->priv.ptr);		\
	if (priv == NULL)					\
	{							\
	    ERRMSG("corrupt file handle");			\
	    return err;						\
	}							\
	if (file->fs==NULL)					\
	{							\
	    ERRMSG("NULL file handle");				\
	    return err;						\
	}							\
	ACFS_FS_PRIV* fspriv = (file->fs->priv.ptr);		\
	if (fspriv == NULL)					\
	{							\
	    ERRMSG("corrupt fs handle");			\
	    return err;						\
	}

#define __FSOPS_HEAD(err);					\
	if (fs==NULL)						\
	{							\
	    ERRMSG("NULL file handle");				\
	    return err;						\
	}							\
	if (!_mvfs_check_magic(fs,FS_MAGIC, "autoconnectfs"))	\
	{							\
	    ERRMSG("fs magic mismatch");			\
	    return err;						\
	}							\
	ACFS_FS_PRIV* fspriv = (fs->priv.ptr);			\
	if (fspriv == NULL)					\
	{							\
	    ERRMSG("corrupt fs handle");			\
	    return err;						\
	}

static MVFS_STAT*   _autoconnectfs_fsop_stat     (MVFS_FILESYSTEM* fs, const char* name);
static MVFS_FILE*   _autoconnectfs_fsop_open     (MVFS_FILESYSTEM* fs, const char* name, mode_t mode);
static int          _autoconnectfs_fsop_unlink   (MVFS_FILESYSTEM* fs, const char* name);
static int          _autoconnectfs_fsop_chmod    (MVFS_FILESYSTEM* fs, const char* name, mode_t mode);
static MVFS_SYMLINK _autoconnectfs_fsop_readlink (MVFS_FILESYSTEM* fs, const char* name);

static MVFS_FILESYSTEM_OPS _fsops = 
{
    .openfile	= _autoconnectfs_fsop_open,
    .unlink	= _autoconnectfs_fsop_unlink,
    .stat	= _autoconnectfs_fsop_stat,
    .chmod      = _autoconnectfs_fsop_chmod,
    .readlink   = _autoconnectfs_fsop_readlink
};

typedef struct _FSENT		FSENT;
typedef struct _LOOKUP		LOOKUP;

// should use an hashtable
struct _FSENT
{
    MVFS_FILESYSTEM* fs;
    char*            url;
    FSENT*           next;
};

typedef struct
{
    FSENT* filesystems;
} ACFS_FS_PRIV;

struct _LOOKUP
{
    MVFS_FILESYSTEM* fs;
    char*            filename;
};

static LOOKUP _lookup_fs(ACFS_FS_PRIV* priv, const char* file)
{
    LOOKUP ret = { .fs = NULL, .filename = NULL };
    MVFS_ARGS* args = mvfs_args_from_url(file);

    const char* type = mvfs_args_get(args,"type");
    const char* host = mvfs_args_get(args,"host");
    const char* port = mvfs_args_get(args,"port");
    const char* path = mvfs_args_get(args,"path");

    DEBUGMSG("path=\"%s\" host=\"%s\" type=\"%s\"", path, host, type);

    if (!path) path = "/";
    if (!host) host = "";
    if (!type) type = "file";

    char buffer[8194];
    if ((port) && strlen(port))
	sprintf(buffer,"%s://%s:%s/", type, host, port);
    else
	sprintf(buffer,"%s://%s/", type, host);

    // FIXME: the whole of this could reside in an URI->Plan9 fs layer, which does the connection handling automatically
    DEBUGMSG("looking for fs for: \"%s\" (key: %s)", file, buffer);

    // now try to find a matching fs -- fixme !!!!
    FSENT* p;
    for (p=priv->filesystems; p; p=p->next)
    {
	if (!strcmp(p->url,buffer))
	{
	    DEBUGMSG("found an existing connection for: %s (key %s)", file, buffer);
	    ret.fs       = p->fs;
	    ret.filename = strdup(path);
	    goto out;
	}
    }

    DEBUGMSG("Dont have an connection for %s (%s) yet - trying to connect ...", file, buffer);

    // this has to be done already here, since the path string will be overwritten!
    ret.filename = strdup(path);

    // prevent attempting to chroot
    mvfs_args_set(args, "path", "");
    MVFS_FILESYSTEM* fs = mvfs_fs_create_args(args);
    if (fs == NULL)
    {
	ERRMSG("Couldnt connect to service: %s", buffer);
	goto err;
    }

    FSENT* newfs = calloc(1,sizeof(FSENT));
    newfs->fs   = fs;
    newfs->url  = strdup(buffer);
    newfs->next = priv->filesystems;
    priv->filesystems = newfs;

    DEBUGMSG("Now opening file: %s via fs", file);

    ret.fs       = fs;

err:

out:
    mvfs_args_free(args);
    return ret;
}

static MVFS_FILE* _autoconnectfs_fsop_open(MVFS_FILESYSTEM* fs, const char* name, mode_t mode)
{
    __FSOPS_HEAD(NULL);
    LOOKUP lu = _lookup_fs(fspriv,name);
    if (lu.fs == NULL)
    {
	ERRMSG("couldnt allocate fs for: %s", name);
	return NULL;
    }

    MVFS_FILE* f = mvfs_fs_openfile(lu.fs, lu.filename, mode);
    free(lu.filename);
    return f;
}

static MVFS_STAT* _autoconnectfs_fsop_stat(MVFS_FILESYSTEM* fs, const char* name)
{
    __FSOPS_HEAD(NULL);
    LOOKUP lu = _lookup_fs(fspriv, name);
    if (lu.fs == NULL)
    {
	ERRMSG("couldnt allocate fs for: %s", name);
	return NULL;
    }

    MVFS_STAT* st = mvfs_fs_statfile(lu.fs, lu.filename);
    free(lu.filename);
    return st;
}

static int _autoconnectfs_fsop_unlink(MVFS_FILESYSTEM* fs, const char* name)
{
    __FSOPS_HEAD(-EFAULT);
    LOOKUP lu = _lookup_fs(fspriv, name);
    if (lu.fs == NULL)
    {
	ERRMSG("couldnt allocate fs for: %s",name);
	return 0;
    }

    int ret = mvfs_fs_unlink(lu.fs, lu.filename);
    free(lu.filename);
    return ret;
}

static int _autoconnectfs_fsop_chmod(MVFS_FILESYSTEM* fs, const char* name, mode_t mode)
{
    __FSOPS_HEAD(-EFAULT);
    LOOKUP lu = _lookup_fs(fspriv, name);
    if (lu.fs == NULL)
    {
	ERRMSG("couldnt allocate fs for: %s", name);
	return 0;
    }

    int ret = mvfs_fs_chmod(lu.fs, lu.filename, mode);
    free(lu.filename);
    return ret;
}

static MVFS_SYMLINK _autoconnectfs_fsop_readlink(MVFS_FILESYSTEM* fs, const char* name)
{
    __FSOPS_HEAD(((MVFS_SYMLINK){.errcode = -EFAULT}));
    LOOKUP lu = _lookup_fs(fspriv, name);
    if (lu.fs == NULL)
    {
	ERRMSG("couldnt allocate fs for: %s", name);
	return ((MVFS_SYMLINK){.errcode = -ENOENT});
    }

    MVFS_SYMLINK ret = mvfs_fs_readlink(lu.fs, lu.filename);
    free(lu.filename);
    return ret;
}

MVFS_FILESYSTEM* mvfs_autoconnectfs_create()
{
    MVFS_FILESYSTEM* fs = mvfs_fs_alloc(_fsops,FS_MAGIC);
    ACFS_FS_PRIV* priv = (ACFS_FS_PRIV*)calloc(1,sizeof(ACFS_FS_PRIV));
    fs->priv.ptr = priv;
    return fs;
}

char* mvfs_autoconnectfs_getconnections(MVFS_FILESYSTEM* fs)
{
    __FSOPS_HEAD(NULL);
    FSENT* ent = fspriv->filesystems;
    int sz = 0;
    
    for (ent=fspriv->filesystems; ent; ent=ent->next)
	sz += strlen(ent->url)+4;
    
    char* buffer = malloc(sz);
    buffer[0] = 0;
    
    for (ent=fspriv->filesystems; ent; ent=ent->next)
    {
	strcat(buffer,ent->url);
	strcat(buffer,"\n");
    }
    return buffer;
}
