
#include <mvfs/types.h>
#include <mvfs/default_ops.h>

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <malloc.h>

/*
MVFS_DIR* mvfs_dir_alloc(MVFS_FILESYSTEM* fs, MVFS_DIR_OPS ops)
{
    MVFS_DIR* dir = calloc(sizeof(MVFS_DIR),1);
    dir->fs = fs;
    dir->refcount = 1;
    dir->ops = ops;
    mvfs_fs_ref(fs);    
    return dir;
}

int mvfs_dir_ref(MVFS_DIR* dir)
{
    if (dir==NULL)
	return -EFAULT;
    dir->refcount++;
    return dir->refcount;
}

int mvfs_dir_unref(MVFS_DIR* dir)
{
    if (dir==NULL)
	return -EFAULT;

    dir->refcount--;

    if (dir->refcount>0)
	return dir->refcount;
    
    if (!(dir->ops.close == NULL))
	dir->ops.close(dir);

    if (dir->ops.free == NULL)
	mvfs_default_dirops_free(dir);
    else
	dir->ops.free(dir);

    return 0;
}

int mvfs_dir_scan(MVFS_DIR* dir)
{
    if (dir==NULL)
	return -EFAULT;

    if (dir->ops.scan == NULL)
	return mvfs_default_dirops_scan(dir);
    else
	return dir->ops.scan(dir);
}
*/
