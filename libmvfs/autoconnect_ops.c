//
// this is an simple auto-connecting overlay fs.
// it takes in URLs and automatically loads/connects backend fs'es on demand
// so an client doesn't have to cope with individual filesystems anymore
//
// the functionality is a little bit like MC's vfs
//

// #define _DEBUG

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <mvfs/mvfs.h>
#include <mvfs/autoconnect_ops.h>

#define FS_MAGIC	"metux/autoconnect-fs-1.0"

#define __FILEOPS_HEAD(err);						\
	if (file==NULL)							\
	{								\
	    fprintf(stderr,"%s() NULL file handle\n", __FUNCTION__);	\
	    return err;							\
	}								\
	METACACHE_FILE_PRIV* priv = (file->priv.ptr);			\
	if (priv == NULL)						\
	{								\
	    fprintf(stderr,"%s() corrupt file handle\n", __FUNCTION__);	\
	    return err;							\
	}								\
	if (file->fs==NULL)						\
	{								\
	    fprintf(stderr,"%s() NULL file handle\n", __FUNCTION__);	\
	    return err;							\
	}								\
	ACFS_FS_PRIV* fspriv = (file->fs->priv.ptr);			\
	if (fspriv == NULL)						\
	{								\
	    fprintf(stderr,"%s() corrupt fs handle\n", __FUNCTION__);	\
	    return err;							\
	}

#define __FSOPS_HEAD(err);						\
	if (fs==NULL)							\
	{								\
	    fprintf(stderr,"%s() NULL file handle\n", __FUNCTION__);	\
	    return err;							\
	}								\
	if (!_mvfs_check_magic(fs,FS_MAGIC, "autoconnectfs"))		\
	{								\
	    fprintf(stderr,"%s() fs magic mismatch\n", __FUNCTION__);	\
	    return err;							\
	}								\
	ACFS_FS_PRIV* fspriv = (fs->priv.ptr);				\
	if (fspriv == NULL)						\
	{								\
	    fprintf(stderr,"%s() corrupt fs handle\n", __FUNCTION__);	\
	    return err;							\
	}

static MVFS_STAT* _acfs_fsops_stat   (MVFS_FILESYSTEM* fs, const char* name);
static MVFS_FILE* _acfs_fsops_open   (MVFS_FILESYSTEM* fs, const char* name, mode_t mode);
static int        _acfs_fsops_unlink (MVFS_FILESYSTEM* fs, const char* name);

static MVFS_FILESYSTEM_OPS _fsops = 
{
    .openfile	= _acfs_fsops_open,
    .unlink	= _acfs_fsops_unlink,
    .stat	= _acfs_fsops_stat
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
    const char* path = strdup(mvfs_args_get(args,"path"));

    if (!path)	path = "/";
    if (!host)  host = "";
    if (!type)  type = "file";

    char buffer[8194];
    if ((port) && strlen(port))
	sprintf(buffer,"%s://%s:%s/", type, host, port);
    else
	sprintf(buffer,"%s://%s/", type, host);

    // FIXME: the whole of this could reside in an URI->Plan9 fs layer, which does the connection handling automatically
#ifdef _DEBUG
    fprintf(stderr,"Autoconnect-FS: looking for fs for: \"%s\" (key: %s)\n", file, buffer);
#endif
    // now try to find a matching fs -- fixme !!!!
    FSENT* p;
    for (p=priv->filesystems; p; p=p->next)
    {
	if (!strcmp(p->url,buffer))
	{
#ifdef _DEBUG
	    fprintf(stderr,"Autoconnect-FS: found an existing connection for: %s (%s)\n", file, buffer);
#endif
	    ret.fs       = p->fs;
	    ret.filename = path;
	    goto out;
	}
    }

#ifdef _DEBUG
    fprintf(stderr,"Autoconnect-FS: Dont have an connection for %s (%s) yet - trying to connect ...\n", file, buffer);
#endif
    // prevent attempting to chroot
    mvfs_args_set(args, "path", "");
    MVFS_FILESYSTEM* fs = mvfs_fs_create_args(args);
    if (fs == NULL)
    {
	fprintf(stderr,"Autoconnect-FS: Couldnt connect to service: %s\n", buffer);
	goto err;
    }

    FSENT* newfs = calloc(1,sizeof(FSENT));
    newfs->fs   = fs;
    newfs->url  = strdup(buffer);
    newfs->next = priv->filesystems;
    priv->filesystems = newfs;

#ifdef _DEBUG    
    fprintf(stderr,"Autoconnect-FS: Now opening file: %s via fs\n", file);
#endif
    ret.fs       = fs;
    ret.filename = strdup(path);

err:
    if (path)
	free(path);
out:
    mvfs_args_free(args);
    return ret;
}

static MVFS_FILE* _acfs_fsops_open(MVFS_FILESYSTEM* fs, const char* name, mode_t mode)
{
    __FSOPS_HEAD(NULL);
#ifdef _DEBUG
    fprintf(stderr,"autoconnect_ops: openfile: \"%s\"\n", name);
#endif
    LOOKUP lu = _lookup_fs(fspriv,name);
    if (lu.fs == NULL)
    {
	fprintf(stderr,"autoconnect_ops: couldnt allocate fs for: %s\n");
	return NULL;
    }

    MVFS_FILE* f = mvfs_fs_openfile(lu.fs, lu.filename, mode);
    free(lu.filename);
    return f;
}

static MVFS_STAT* _acfs_fsops_stat(MVFS_FILESYSTEM* fs, const char* name)
{
    __FSOPS_HEAD(NULL);
#ifdef _DEBUG
    fprintf(stderr,"autoconnect_ops: statfile: \"%s\"\n", name);
#endif
    LOOKUP lu = _lookup_fs(fspriv, name);
    if (lu.fs == NULL)
    {
	fprintf(stderr,"autoconnect_ops: couldnt allocate fs for: %s\n");
	return NULL;
    }

    MVFS_STAT* st = mvfs_fs_statfile(lu.fs, lu.filename);
    free(lu.filename);
    return st;
}

static int _acfs_fsops_unlink(MVFS_FILESYSTEM* fs, const char* name)
{
    __FSOPS_HEAD(-EFAULT);
#ifdef _DEBUG
    fprintf(stderr,"autoconnect_ops: unlinkfile: \"%s\"\n", name);
#endif
    LOOKUP lu = _lookup_fs(fspriv, name);
    if (lu.fs == NULL)
    {
	fprintf(stderr,"autoconnect_ops: couldnt allocate fs for: %s\n");
	return 0;
    }

    int ret = mvfs_fs_unlink(lu.fs, lu.filename);
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
