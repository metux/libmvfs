
//
//	FD private data layout
//
//	id	fd
//	name	filename
//	ptr	DIR* pointer

#define PRIV_FD(file)			(file->priv.id)
#define PRIV_NAME(file)			(file->priv.name)
#define PRIV_DIRP(file)			((DIR*)(file->priv.ptr))

#define PRIV_SET_FD(file,fd)	 	file->priv.id = fd;
#define PRIV_SET_NAME(file,name)	file->priv.name = strdup(name)
#define PRIV_SET_DIRP(file,dirp)	file->priv.ptr = dirp;

#include <mvfs/mvfs.h>
#include <mvfs/default_ops.h>
#include <mvfs/hostfs.h>

#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define FS_MAGIC 	"hostfs"

static int        mvfs_hostfs_fileops_open    (MVFS_FILE* file, mode_t mode);
static off64_t    mvfs_hostfs_fileops_seek    (MVFS_FILE* file, off64_t offset, int whence);
static ssize_t    mvfs_hostfs_fileops_read    (MVFS_FILE* file, void* buf, size_t count);
static ssize_t    mvfs_hostfs_fileops_write   (MVFS_FILE* file, const void* buf, size_t count);
static ssize_t    mvfs_hostfs_fileops_pread   (MVFS_FILE* file, void* buf, size_t count, off64_t offset);
static ssize_t    mvfs_hostfs_fileops_pwrite  (MVFS_FILE* file, const void* buf, size_t count, off64_t offset);
static int        mvfs_hostfs_fileops_setflag (MVFS_FILE* file, MVFS_FILE_FLAG flag, long value);
static int        mvfs_hostfs_fileops_getflag (MVFS_FILE* file, MVFS_FILE_FLAG flag, long* value);
static MVFS_STAT* mvfs_hostfs_fileops_stat    (MVFS_FILE* file);
static int        mvfs_hostfs_fileops_close   (MVFS_FILE* file);
static int        mvfs_hostfs_fileops_eof     (MVFS_FILE* file);
static MVFS_FILE* mvfs_hostfs_fileops_lookup  (MVFS_FILE* file, const char* name);
static MVFS_STAT* mvfs_hostfs_fileops_scan    (MVFS_FILE* file);
static int        mvfs_hostfs_fileops_reset   (MVFS_FILE* file);

static MVFS_FILE_OPS hostfs_fileops = 
{
    .seek	= mvfs_hostfs_fileops_seek,
    .read	= mvfs_hostfs_fileops_read,
    .write	= mvfs_hostfs_fileops_write,
    .pread	= mvfs_hostfs_fileops_pread,
    .pwrite	= mvfs_hostfs_fileops_pwrite,
    .close	= mvfs_hostfs_fileops_close,
    .eof        = mvfs_hostfs_fileops_eof,
    .lookup     = mvfs_hostfs_fileops_lookup,
    .reset      = mvfs_hostfs_fileops_reset,
    .scan       = mvfs_hostfs_fileops_scan
};

static MVFS_FILE* mvfs_hostfs_fsops_open    (MVFS_FILESYSTEM* fs, const char* name, mode_t mode);
static MVFS_STAT* mvfs_hostfs_fsops_stat    (MVFS_FILESYSTEM* fs, const char* name);
static int        mvfs_hostfs_fsops_unlink  (MVFS_FILESYSTEM* fs, const char* name);
static int        mvfs_hostfs_fsops_free    (MVFS_FILESYSTEM* fs);
    
static MVFS_FILESYSTEM_OPS hostfs_fsops = 
{
    .openfile	= mvfs_hostfs_fsops_open,
    .unlink	= mvfs_hostfs_fsops_unlink
};

static off64_t mvfs_hostfs_fileops_seek (MVFS_FILE* file, off64_t offset, int whence)
{
    off_t ret = lseek(PRIV_FD(file), offset, whence);
    file->errcode = errno;
    return ret;
}

static ssize_t mvfs_hostfs_fileops_read (MVFS_FILE* file, void* buf, size_t count)
{
    memset(buf, 0, count);
    ssize_t s = read(PRIV_FD(file), buf, count);
    file->errcode = errno;
    if (s==0)
	file->priv.status = 1;
    return s;
}

static ssize_t mvfs_hostfs_fileops_pread (MVFS_FILE* file, void* buf, size_t count, off64_t offset)
{
    mvfs_hostfs_fileops_seek(file, offset, SEEK_SET);
    return mvfs_hostfs_fileops_read(file, buf, count);
}

static ssize_t mvfs_hostfs_fileops_write (MVFS_FILE* file, const void* buf, size_t count)
{
    ssize_t s = write(PRIV_FD(file), buf, count);
    file->errcode = errno;
    return s;
}

static ssize_t mvfs_hostfs_fileops_pwrite (MVFS_FILE* file, const void* buf, size_t count, off64_t offset)
{
    mvfs_hostfs_fileops_seek(file,offset,SEEK_SET);
    return mvfs_hostfs_fileops_write(file,buf,count);
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

static int mvfs_hostfs_fileops_setflag (MVFS_FILE* fp, MVFS_FILE_FLAG flag, long value)
{
    fprintf(stderr,"mvfs_hostfs_fileops_setflag() %s not supported\n", __mvfs_flag2str(flag));
    fp->errcode = EINVAL;
    return -1;
}

static int mvfs_hostfs_fileops_getflag (MVFS_FILE* fp, MVFS_FILE_FLAG flag, long* value)
{
    fprintf(stderr,"mvfs_hostfs_fileops_getflag() %s not supported\n", __mvfs_flag2str(flag));
    fp->errcode = EINVAL;
    return -1;
}

static MVFS_STAT* mvfs_hostfs_fileops_stat(MVFS_FILE* fp)
{
    fprintf(stderr,"mvfs_hostfs_fileops_stat() not supported\n");
    fp->errcode = EINVAL;
    return NULL;
}

static MVFS_FILE* mvfs_hostfs_fsops_open(MVFS_FILESYSTEM* fs, const char* name, mode_t mode)
{
    int fd = open(name, mode);
    if (fd<0)
    {
	fs->errcode = errno;
	return NULL;
    }

    MVFS_FILE* file = mvfs_file_alloc(fs,hostfs_fileops);
    file->priv.name = strdup(name);
    file->priv.id   = fd;
    
    return file;
}

static MVFS_STAT* mvfs_hostfs_fsops_stat(MVFS_FILESYSTEM* fs, const char* name)
{
    fprintf(stderr,"mvfs_hostfs_fsops_stat() DUMMY\n");
    if (fs!=NULL)
	fs->errcode = EINVAL;
    return NULL;
}

static int mvfs_hostfs_fsops_unlink(MVFS_FILESYSTEM* fs, const char* name)
{
    int ret = unlink(name);
    fs->errcode = errno;
    return errno;
}

MVFS_FILESYSTEM* mvfs_hostfs_create(MVFS_HOSTFS_PARAM par)
{
    fprintf(stderr,"mvfs_hostfs_create() params currently ignored 2!\n");
    MVFS_FILESYSTEM* fs = mvfs_fs_alloc(hostfs_fsops,FS_MAGIC);
    return fs;
}

MVFS_FILESYSTEM* mvfs_hostfs_create_args(MVFS_ARGS* args)
{
    const char* chroot = mvfs_args_get(args,"chroot");
    if ((chroot) && (strlen(chroot)) && (!strcmp(chroot,"/")))
    {
	fprintf(stderr,"mvfs_hostfs: chroot not supported yet !\n");
	return NULL;
    }
    
    MVFS_FILESYSTEM* fs = mvfs_fs_alloc(hostfs_fsops, FS_MAGIC);
    return fs;
}

static int mvfs_hostfs_fileops_close(MVFS_FILE* file)
{
    int ret = close(PRIV_FD(file));
    file->priv.id = -1;
    return ret;
}

static int mvfs_hostfs_fileops_eof(MVFS_FILE* file)
{
    return ((file->priv.status) ? 1 : 0);
}

static MVFS_FILE* mvfs_hostfs_fileops_lookup  (MVFS_FILE* file, const char* name)
{
    int fd = openat(PRIV_FD(file), name, O_RDONLY);
    if (fd<0)
	return NULL;

    MVFS_FILE* f2 = mvfs_file_alloc(file->fs,hostfs_fileops);
    f2->priv.name = strdup(name);
    f2->priv.id   = fd;

    return file;
}

static DIR* mvfs_hostfs_fileops_init_dir(MVFS_FILE* file)
{
    DIR* dir = PRIV_DIRP(file);
    if (dir != NULL)
	return dir;
    
    dup(PRIV_FD(file));

    dir = fdopendir(1);

    dir = fdopendir(dup(PRIV_FD(file)));
    PRIV_SET_DIRP(file,dir);
    return dir;
}

static MVFS_STAT* mvfs_hostfs_fileops_scan(MVFS_FILE* file)
{
    DIR* dir = mvfs_hostfs_fileops_init_dir(file);
    if (dir==NULL)
    {
	fprintf(stderr,"mvfs_hostfs_fileops_scan() cannot get DIR* ptr\n");
	return NULL;
    }

    struct dirent * ent = readdir(dir);
    if (ent == NULL)
	return 0;
    
    if ((strcmp(ent->d_name,".")==0) || (strcmp(ent->d_name,"..")==0))
	return mvfs_hostfs_fileops_scan(file);

    return mvfs_stat_alloc(ent->d_name, "none", "none");	// FIXME !
}

static int mvfs_hostfs_fileops_reset(MVFS_FILE* file)
{
    DIR* dir = mvfs_hostfs_fileops_init_dir(file);
    if (dir==NULL)
    {
	fprintf(stderr,"mvfs_hostfs_fileops_reset() cannot get DIR* ptr\n");
	return 0;
    }
    
    rewinddir(dir);
    return 1;
}

#define _DIR_CLEAN_CURRENT		\
{					\
    if (dir->current.name != NULL)	\
    {					\
	free(dir->current.name);	\
	dir->current.name = NULL;	\
    }					\
}

#define _DIR_PTR	\
	((dir==NULL) ? NULL : ((DIR*)dir->priv.ptr))
	
#define _DIR_SANITY(fn)	\
    if (dir == NULL)							\
    {									\
	fprintf(stderr,"hostfs::dirops::%s: NULL dir passed\n");	\
	return -EFAULT;							\
    }									\
    if (dir->priv.ptr == NULL)						\
    {									\
	fprintf(stderr,"hostfs::dirops::%s: dir already closed\n");	\
	return -EINVAL;							\
    }
