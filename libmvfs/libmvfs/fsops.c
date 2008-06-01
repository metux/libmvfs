
#include <mvfs/types.h>
#include <mvfs/default_ops.h>
#include <mvfs/hostfs.h>
#include <mvfs/mixpfs.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>

MVFS_FILE* mvfs_fs_openfile(MVFS_FILESYSTEM* fs, const char* name, mode_t mode)
{
    if (fs==NULL)
	return NULL;
    if (fs->ops.openfile == NULL)
	return mvfs_default_fsops_openfile(fs, name, mode);
    return fs->ops.openfile(fs, name, mode);
}

MVFS_FILESYSTEM* mvfs_fs_alloc(MVFS_FILESYSTEM_OPS ops, const char* magic)
{
    MVFS_FILESYSTEM* fs = calloc(sizeof(MVFS_FILESYSTEM),1);
    fs->refcount = 1;
    fs->ops      = ops;
    fs->magic    = strdup(magic);
    return fs;
}

MVFS_STAT* mvfs_fs_statfile(MVFS_FILESYSTEM* fs, const char* filename)
{
    if (fs==NULL)
	return NULL;
    return ((fs->ops.stat==NULL) ?
	    mvfs_default_fsops_stat(fs, filename) : 
	    fs->ops.stat(fs, filename));
}

int mvfs_fs_unlink(MVFS_FILESYSTEM* fs, const char* filename)
{
    if (fs==NULL)
	return -EFAULT;
    return ((fs->ops.unlink==NULL) ? 
	mvfs_default_fsops_unlink(fs,filename) :
	fs->ops.unlink(fs, filename));
}

int mvfs_fs_ref(MVFS_FILESYSTEM* fs)
{
    if (fs==NULL)
	return -EFAULT;

    fs->refcount++;
    return fs->refcount;
}

int mvfs_fs_unref(MVFS_FILESYSTEM* fs)
{
    if (fs==NULL)
	return -EFAULT;

    fs->refcount--;
    if (fs->refcount>0)
	return fs->refcount;

    fprintf(stderr,"Free'ing filesystem\n");

    if (!(fs->ops.free == NULL))
	fs->ops.free(fs);
	
    free(fs);
    return 0;
}

MVFS_FILESYSTEM* mvfs_fs_create_args(MVFS_ARGS* args)
{
    if (args==NULL)
    {
	fprintf(stderr,"mvfs_fs_create() NULL args ... defaulting to file:/");
	return mvfs_hostfs_create_args(args);
    }

    const char* type = mvfs_args_get(args,"type");
    if (type==NULL)
    {
	fprintf(stderr,"mvfs_fs_create() missing type ... defaulting to \"file\"\n");
	return mvfs_hostfs_create_args(args);
    }

    if (strcmp(type,"file")==0)
	return mvfs_hostfs_create_args(args);
    if (strcmp(type,"local")==0)
	return mvfs_hostfs_create_args(args);
    if ((strcmp(type,"ninep")==0) || (strcmp(type,"9p")==0))
	return mvfs_mixpfs_create_args(args);
    
    fprintf(stderr,"mvfs_fs_create() unsupported type \"%s\"\n", type);
    return NULL;
}

MVFS_SYMLINK mvfs_fs_readlink(MVFS_FILESYSTEM* fs, const char* name)
{
    if (fs==NULL)
	return ((MVFS_SYMLINK){.errcode = -EFAULT, .target=""});

    if (fs->ops.readlink == NULL)
	return ((MVFS_SYMLINK){.errcode = -EOPNOTSUPP, .target=""});

    return fs->ops.readlink(fs, name);
}

int mvfs_fs_symlink(MVFS_FILESYSTEM* fs, const char* n1, const char* n2)
{
    if (fs==NULL)
	return -EFAULT;

    if (fs->ops.symlink == NULL)
	return -EOPNOTSUPP;

    return fs->ops.symlink(fs, n1, n2);
}

int mvfs_fs_rename(MVFS_FILESYSTEM* fs, const char* n1, const char* n2)
{
    if (fs==NULL)
	return -EFAULT;

    if (fs->ops.rename == NULL)
	return -EOPNOTSUPP;
    
    return fs->ops.rename(fs, n1, n2);
}

int mvfs_fs_chmod(MVFS_FILESYSTEM* fs, const char* filename, mode_t mode)
{
    if (fs==NULL)
	return -EFAULT;
    if (fs->ops.chmod == NULL)
	return -EOPNOTSUPP;
    return fs->ops.chmod(fs, filename, mode);
}

int mvfs_fs_chown(MVFS_FILESYSTEM* fs, const char* filename, const char* uid, const char* gid)
{
    if (fs==NULL)
	return -EFAULT;
    if (fs->ops.chown == NULL)
	return -EOPNOTSUPP;
    return fs->ops.chown(fs, filename, uid, gid);
}

int mvfs_fs_mkdir(MVFS_FILESYSTEM* fs, const char* filename, mode_t mode)
{
    if (fs==NULL)
	return -EFAULT;
    if (fs->ops.mkdir == NULL)
	return -EOPNOTSUPP;
    return fs->ops.mkdir(fs, filename, mode);
}
