/*
    libmvfs - metux Virtual Filesystem Library

    Client-side file operation frontend

    Copyright (C) 2008 Enrico Weigelt, metux IT service <weigelt@metux.de>
    This code is published under the terms of the GNU Public License 2.0
*/

#include <mvfs/types.h>
#include <mvfs/default_ops.h>

#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <malloc.h>

off64_t mvfs_file_seek    (MVFS_FILE* fp, off64_t offset, int whence)
{
    if (fp==NULL)
	return (off64_t) -EFAULT;

    if (fp->ops.seek == NULL)
	return mvfs_default_fileops_seek(fp, offset, whence);

    return fp->ops.seek(fp, offset, whence);
}

ssize_t mvfs_file_read    (MVFS_FILE* fp, void* buf, size_t count)
{
    if (fp==NULL)
	return (ssize_t) -EFAULT;
    if (fp->ops.read == NULL)
	return mvfs_default_fileops_read(fp, buf, count);

    return fp->ops.read(fp, buf, count);
}

ssize_t mvfs_file_write   (MVFS_FILE* fp, const void* buf, size_t count)
{
    if (fp==NULL)
	return (ssize_t) -EFAULT;
    if (fp->ops.write == NULL)
	return mvfs_default_fileops_write(fp, buf, count);
    
    return fp->ops.write(fp, buf, count);
}

ssize_t mvfs_file_pread    (MVFS_FILE* fp, void* buf, size_t count, off64_t offset)
{
    if (fp==NULL)
	return (ssize_t) -EFAULT;
    if (fp->ops.read == NULL)
	return mvfs_default_fileops_pread(fp, buf, count, offset);

    return fp->ops.pread(fp, buf, count, offset);
}

ssize_t mvfs_file_pwrite  (MVFS_FILE* fp, const void* buf, size_t count, off64_t offset)
{
    if (fp==NULL)
	return (ssize_t) -EFAULT;
    if (fp->ops.write == NULL)
	return mvfs_default_fileops_pwrite(fp, buf, count, offset);
    
    return fp->ops.pwrite(fp, buf, count, offset);
}

int mvfs_file_setflag (MVFS_FILE* fp, MVFS_FILE_FLAG flag, long value)
{
    if (fp==NULL)
	return -EFAULT;
    if (fp->ops.setflag == NULL)
	return mvfs_default_fileops_setflag(fp, flag, value);
    return fp->ops.setflag(fp, flag, value);
}

int mvfs_file_getflag (MVFS_FILE* fp, MVFS_FILE_FLAG flag, long* value)
{
    if (fp==NULL)
	return -EFAULT;
    if (fp->ops.getflag == NULL)
	return mvfs_default_fileops_getflag(fp, flag, value);
    return fp->ops.getflag(fp, flag, value);
}

int mvfs_stat_free(MVFS_STAT*st)
{
    if (st == NULL)
	return -EFAULT;
    if (st->name)	free((char*)st->name);
    if (st->uid)	free((char*)st->uid);
    if (st->gid)	free((char*)st->gid);
    free(st);
    return 0;
}

MVFS_STAT* mvfs_stat_dup(MVFS_STAT* oldst)
{
    if (oldst == NULL)
	return NULL;

    MVFS_STAT* newst = mvfs_stat_alloc(oldst->name, oldst->uid, oldst->gid);
    newst->mode  = oldst->mode;
    newst->size  = oldst->size;
    newst->atime = oldst->atime;
    newst->mtime = oldst->mtime;
    newst->ctime = oldst->ctime;
    return newst;
}

MVFS_STAT* mvfs_stat_alloc(const char* name, const char* uid, const char* gid)
{
    MVFS_STAT* stat = (MVFS_STAT*)calloc(1,sizeof(MVFS_STAT));
    stat->name = strdup(name ? name : "");
    stat->uid  = strdup(uid  ? uid : "");	
    stat->gid  = strdup(gid  ? gid : "");
    return stat;
}

MVFS_STAT* mvfs_file_stat(MVFS_FILE* fp)
{
    if (fp==NULL)
	return NULL;
    if (fp->ops.stat == NULL)
	return mvfs_default_fileops_stat(fp);
    return fp->ops.stat(fp);
}

int mvfs_file_close(MVFS_FILE* file)
{
    if (file==NULL)
	return -EFAULT;

    int ret;
    if (file->ops.close == NULL)
	    ret = mvfs_default_fileops_close(file);
    else
	    ret = file->ops.close(file);
    mvfs_file_unref(file);
    return ret;
}

MVFS_FILE* mvfs_file_alloc(MVFS_FILESYSTEM* fs, MVFS_FILE_OPS ops)
{
    MVFS_FILE* fp = calloc(sizeof(MVFS_FILE),1);
    fp->fs = fs;
    fp->refcount = 1;
    fp->ops = ops;
    mvfs_fs_ref(fs);
    
    return fp;
}

int mvfs_file_unref(MVFS_FILE* file)
{
    if (file==NULL)
	return -EFAULT;

    file->refcount--;
    if (file->refcount>0)
	return file->refcount;
	
    // file is not referenced anymore - call the free handler
    // the free handler is also responsible for unref'ing the fs
    if (file->ops.free == NULL)
	mvfs_default_fileops_free(file);
    else 
	file->ops.free(file);
	
    // now we can assume, all additional data has been free()'d and fs ins unref'ed
    free(file);
}

int mvfs_file_ref(MVFS_FILE* file)
{
    if (file==NULL)
	return -EFAULT;
    file->refcount++;
    return file->refcount;
}

int mvfs_file_eof(MVFS_FILE* file)
{
    if (file==NULL)
	return -EFAULT;
    if (file->ops.eof == NULL)
	return mvfs_default_fileops_eof(file);
    return file->ops.eof(file);
}

MVFS_STAT* mvfs_file_scan(MVFS_FILE* file)
{
    if (file == NULL)
	return NULL;

    if (file->ops.scan == NULL)
	return mvfs_default_fileops_scan(file);
	
    return file->ops.scan(file);
}

MVFS_FILE* mvfs_file_lookup(MVFS_FILE* file, const char* name)
{
    if (file == NULL)
	return NULL;
    
    if (file->ops.lookup == NULL)
	return mvfs_default_fileops_lookup(file, name);

    return file->ops.lookup(file,name);
}

int mvfs_file_reset(MVFS_FILE* file)
{
    if (file==NULL)
	return -EFAULT;
    
    if (file->ops.reset == NULL)
	return mvfs_default_fileops_reset(file);

    return file->ops.reset(file);
}
