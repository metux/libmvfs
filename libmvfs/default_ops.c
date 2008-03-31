
#define _LARGEFILE64_SOURCE

#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <malloc.h>

#include <mvfs/mvfs.h>
#include <mvfs/default_ops.h>

/* release the file's data */
int mvfs_default_fileops_free  (MVFS_FILE* file)
{
    // call the close() handler just for sure
    if (!(file->ops.close == NULL))
	file->ops.close(file);

    mvfs_fs_unref(file->fs);
    return 0;
}

int mvfs_default_fileops_close (MVFS_FILE* file)
{
    fprintf(stderr,"mvfs_default_fileops_close() DUMMY\n");
    return 0;
}

int mvfs_default_fileops_open    (MVFS_FILE* fp, mode_t mode)
{
    fprintf(stderr,"mvfs_default_fileops_open() DUMMY\n");
    fp->errcode = 0;
    return 0;
}

off64_t mvfs_default_fileops_seek (MVFS_FILE* fp, off64_t offset, int whence)
{
    fprintf(stderr,"mvfs_default_fileops_seek() seeking not supported\n");
    fp->errcode = ESPIPE;
    return (off64_t) -1;
}

ssize_t mvfs_default_fileops_read    (MVFS_FILE* fp, void* buf, size_t count)
{
    fprintf(stderr,"mvfs_default_fileops_pread() read not supported\n");
    fp->errcode = EINVAL;
    return (ssize_t) -1;
}

ssize_t mvfs_default_fileops_write   (MVFS_FILE* fp, const void* buf, size_t count)
{
    fprintf(stderr,"mvfs_default_fileops_pwrite() write not supported\n");
    fp->errcode = EINVAL;
    return (ssize_t) -1;
}

ssize_t mvfs_default_fileops_pread    (MVFS_FILE* fp, void* buf, size_t count, off64_t offset)
{
    fprintf(stderr,"mvfs_default_fileops_pread() read not supported\n");
    fp->errcode = EINVAL;
    return (ssize_t) -1;
}

ssize_t mvfs_default_fileops_pwrite   (MVFS_FILE* fp, const void* buf, size_t count, off64_t offset)
{
    fprintf(stderr,"mvfs_default_fileops_pwrite() write not supported\n");
    fp->errcode = EINVAL;
    return (ssize_t) -1;
}

static inline const char* __mvfs_flag2str(MVFS_FILE_FLAG f)
{
    switch (f)
    {
	case NONBLOCK:		return "NONBLOCK";
	case READ_TIMEOUT:	return "READ_TIMEOUT";
	case WRITE_TIMEOUT:	return "WRITE_TIMEOUT";
	case READ_AHEAD:	return "READ_AHEAD";
	case WRITE_ASYNC:	return "WRITE_ASYNNC";
	default:		return "UNKNOWN";
    }
}

int mvfs_default_fileops_setflag (MVFS_FILE* fp, MVFS_FILE_FLAG flag, long value)
{
    fprintf(stderr,"mvfs_default_fileops_setflag() %s not supported\n", __mvfs_flag2str(flag));
    fp->errcode = EINVAL;
    return -1;
}

int mvfs_default_fileops_getflag (MVFS_FILE* fp, MVFS_FILE_FLAG flag, long* value)
{
    fprintf(stderr,"mvfs_default_fileops_getflag() %s not supported\n", __mvfs_flag2str(flag));
    fp->errcode = EINVAL;
    return -1;
}

MVFS_STAT* mvfs_default_fileops_stat(MVFS_FILE* fp)
{
    fprintf(stderr,"mvfs_default_fileops_stat() not supported\n");
    fp->errcode = EINVAL;
    return NULL;
}

MVFS_FILE* mvfs_default_fsops_openfile(MVFS_FILESYSTEM* fs, const char* name, mode_t mode)
{
    fprintf(stderr,"mvfs_default_fsops_open() DUMMY\n");
    if (fs!=NULL)
	fs->errcode = EINVAL;
    return NULL;
}

MVFS_STAT* mvfs_default_fsops_stat(MVFS_FILESYSTEM* fs, const char* name)
{
    fprintf(stderr,"mvfs_default_fsops_stat() DUMMY\n");
    if (fs!=NULL)
	fs->errcode = EINVAL;
    return NULL;
}

int mvfs_default_fsops_unlink(MVFS_FILESYSTEM* fs, const char* name)
{
    fprintf(stderr,"mvfs_default_fsops_unlink() DUMMY\n");
    if (fs!=NULL)
	fs->errcode = EINVAL;
    return -EINVAL;
}

int mvfs_default_fileops_eof(MVFS_FILE* file)
{
    fprintf(stderr,"mvfs_default_fileops_eof() DUMMY\n");
    return 0;
}

MVFS_FILE* mvfs_default_fileops_lookup(MVFS_FILE* file, const char* name)
{
    return NULL;
}

MVFS_STAT* mvfs_default_fileops_scan(MVFS_FILE* file)
{
    return NULL;
}

int mvfs_default_fileops_reset(MVFS_FILE* file)
{
    return -EINVAL;
}
