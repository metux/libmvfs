/*
    libmvfs - metux Virtual Filesystem Library

    Client-side filesystem operations frontend

    Copyright (C) 2008 Enrico Weigelt, metux IT service <weigelt@metux.de>
    This code is published under the terms of the GNU Public License 2.0
*/

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <mvfs/types.h>
#include <mvfs/default_ops.h>
#include <mvfs/hostfs.h>
#include <mvfs/mixpfs.h>
#include <mvfs/fishfs.h>
#include <mvfs/_utils.h>

#define __CHECK_FS(ret)					\
    {							\
	if (fs==NULL)					\
	{						\
	    DEBUGMSG("NULL fs descriptor passed");	\
	    return ret;					\
	}						\
    }

#define __FSOP_STD_VAL(opname,faultret,stdret,param...)	\
    __CHECK_FS(faultret);				\
    return ((fs->ops.opname) ? (fs->ops.opname( fs, ##param )) : (stdret))

#define __FSOP_STD_CALL(opname,faultret,param...)		\
    __CHECK_FS(faultret);				\
    return ((fs->ops.opname) ? (fs->ops.opname( fs, ##param )) : mvfs_default_fsops_##opname(fs, ##param))

MVFS_FILESYSTEM* mvfs_fs_alloc(MVFS_FILESYSTEM_OPS ops, const char* magic)
{
    MVFS_FILESYSTEM* fs = calloc(sizeof(MVFS_FILESYSTEM),1);
    fs->refcount = 1;
    fs->ops      = ops;
    fs->magic    = strdup(magic);
    return fs;
}

MVFS_FILE* mvfs_fs_openfile(MVFS_FILESYSTEM* fs, const char* name, mode_t mode)
{
    __FSOP_STD_CALL(openfile,NULL,name,mode);
}

MVFS_STAT* mvfs_fs_statfile(MVFS_FILESYSTEM* fs, const char* filename)
{
    __FSOP_STD_CALL(stat,NULL,filename);
}

int mvfs_fs_unlink(MVFS_FILESYSTEM* fs, const char* filename)
{
    __FSOP_STD_CALL(unlink,-EFAULT,filename);
}

int mvfs_fs_ref(MVFS_FILESYSTEM* fs)
{
    __CHECK_FS(-EFAULT);
    fs->refcount++;
    return fs->refcount;
}

int mvfs_fs_unref(MVFS_FILESYSTEM* fs)
{
    __CHECK_FS(-EFAULT);

    fs->refcount--;
    if (fs->refcount>0)
	return fs->refcount;

    DEBUGMSG("Free'ing filesystem");

    if (!(fs->ops.free == NULL))
	fs->ops.free(fs);
	
    free(fs);
    return 0;
}

MVFS_FILESYSTEM* mvfs_fs_create_args(MVFS_ARGS* args)
{
    if (args==NULL)
    {
	DEBUGMSG("NULL args ... defaulting to file:/");
	return mvfs_hostfs_create_args(args);
    }

    const char* type = mvfs_args_get(args,"type");
    if (type==NULL)
    {
	DEBUGMSG("missing type ... defaulting to \"file\"");
	return mvfs_hostfs_create_args(args);
    }

    if (strcmp(type,"file")==0)
	return mvfs_hostfs_create_args(args);
    if (strcmp(type,"local")==0)
	return mvfs_hostfs_create_args(args);
    if (strcmp(type,"ninep")==0)
	return mvfs_hostfs_create_args(args);
    if (strcmp(type,"9p")==0)
	return mvfs_mixpfs_create_args(args);
    if (strcmp(type,"fish")==0)
	return mvfs_fishfs_create_args(args);

    ERRMSG("unsupported type \"%s\"", type);
    return NULL;
}

MVFS_SYMLINK mvfs_fs_readlink(MVFS_FILESYSTEM* fs, const char* name)
{
    __FSOP_STD_VAL(readlink,
	((MVFS_SYMLINK){.errcode = -EFAULT, .target=""}),
	((MVFS_SYMLINK){.errcode = -EOPNOTSUPP, .target=""}),
	name);
}

int mvfs_fs_symlink(MVFS_FILESYSTEM* fs, const char* n1, const char* n2)
{
    __FSOP_STD_VAL(symlink, -EFAULT, -EOPNOTSUPP, n1, n2);
}

int mvfs_fs_rename(MVFS_FILESYSTEM* fs, const char* n1, const char* n2)
{
    __FSOP_STD_VAL(rename, -EFAULT, -EOPNOTSUPP, n1, n2);
}

int mvfs_fs_chmod(MVFS_FILESYSTEM* fs, const char* filename, mode_t mode)
{
    __FSOP_STD_VAL(chmod, -EFAULT, -EOPNOTSUPP, filename, mode);
}

int mvfs_fs_chown(MVFS_FILESYSTEM* fs, const char* filename, const char* uid, const char* gid)
{
    __FSOP_STD_VAL(chown, -EFAULT, -EOPNOTSUPP, filename, uid, gid);
}

int mvfs_fs_mkdir(MVFS_FILESYSTEM* fs, const char* filename, mode_t mode)
{
    __FSOP_STD_VAL(mkdir, -EFAULT, -EOPNOTSUPP, filename, mode);
}
