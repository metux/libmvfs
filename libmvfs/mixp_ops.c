/*
    libmvfs - metux Virtual Filesystem Library

    Filesystem driver: 9P via libmixp

    Copyright (C) 2008 Enrico Weigelt, metux IT service <weigelt@metux.de>
    This code is published under the terms of the GNU Public License 2.0
*/

#define __DEBUG

#define _LARGEFILE64_SOURCE

#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <malloc.h>

#include <mvfs/mvfs.h>
#include <mvfs/default_ops.h>
#include <mvfs/mixpfs.h>
#include <mvfs/_utils.h>

#include <9p-mixp/mixp.h>

#define MIXP_FS_CLIENT(fs)	((MIXP_CLIENT*)(fs->priv.ptr))

#define	FS_MAGIC	"metux/mixp-fs-1"

static inline char* SSTRDUP(const char* str)
{
    if (str==NULL)
	return strdup("");
    else
	return strdup(str);
}

static off64_t    mvfs_mixpfs_fileops_seek   (MVFS_FILE* file, off64_t offset, int whence);
static ssize_t    mvfs_mixpfs_fileops_pread  (MVFS_FILE* file, void* buf, size_t count, off64_t offset);
static ssize_t    mvfs_mixpfs_fileops_pwrite (MVFS_FILE* file, const void* buf, size_t count, off64_t offset);
static ssize_t    mvfs_mixpfs_fileops_read   (MVFS_FILE* file, void* buf, size_t count);
static ssize_t    mvfs_mixpfs_fileops_write  (MVFS_FILE* file, const void* buf, size_t count);
static int        mvfs_mixpfs_fileops_close  (MVFS_FILE* file);
static int        mvfs_mixpfs_fileops_free   (MVFS_FILE* file);
static int        mvfs_mixpfs_fileops_eof    (MVFS_FILE* file);
static MVFS_STAT* mvfs_mixpfs_fileops_stat   (MVFS_FILE* file);
static MVFS_FILE* mvfs_mixpfs_fileops_lookup (MVFS_FILE* file, const char* name);
static MVFS_STAT* mvfs_mixpfs_fileops_scan   (MVFS_FILE* file);
static int        mvfs_mixpfs_fileops_reset  (MVFS_FILE* file);

static MVFS_FILE_OPS mixpfs_fileops = 
{
    .seek	= mvfs_mixpfs_fileops_seek,
    .read       = mvfs_mixpfs_fileops_read,
    .write      = mvfs_mixpfs_fileops_write,
    .pread	= mvfs_mixpfs_fileops_pread,
    .pwrite	= mvfs_mixpfs_fileops_pwrite,
    .close	= mvfs_mixpfs_fileops_close,
    .free	= mvfs_mixpfs_fileops_free,
    .eof        = mvfs_mixpfs_fileops_eof,
    .lookup     = mvfs_mixpfs_fileops_lookup,
    .scan       = mvfs_mixpfs_fileops_scan,
    .reset      = mvfs_mixpfs_fileops_reset,
    .stat       = mvfs_mixpfs_fileops_stat
};

static MVFS_STAT* mvfs_mixpfs_fsops_stat   (MVFS_FILESYSTEM* fs, const char* name);
static MVFS_FILE* mvfs_mixpfs_fsops_open   (MVFS_FILESYSTEM* fs, const char* name, mode_t mode);
static int        mvfs_mixpfs_fsops_unlink (MVFS_FILESYSTEM* fs, const char* name);

static MVFS_FILESYSTEM_OPS mixpfs_fsops = 
{
    .openfile	= mvfs_mixpfs_fsops_open,
    .unlink	= mvfs_mixpfs_fsops_unlink,
    .stat       = mvfs_mixpfs_fsops_stat
};

typedef struct _mixp_dirent MIXP_DIRENT;
struct _mixp_dirent
{
    MIXP_STAT* stat; 
    MIXP_DIRENT* next;
};

typedef struct 
{
    MIXP_CFID*   cfid;
    char*        pathname;
    int          eof;
    off64_t	 pos;
    MIXP_DIRENT* dirents;
    MIXP_DIRENT* dirptr;
} MIXP_FILE_PRIV;

#define __FILEOPS_HEAD(err);				\
	if (file==NULL)					\
	{						\
	    ERRMSG("NULL file handle");			\
	    return err;					\
	}						\
	MIXP_FILE_PRIV* priv = (file->priv.ptr);	\
	if (priv == NULL)				\
	{						\
	    ERRMSG("corrupt file handle");		\
	    return err;					\
	}

static int __mixp_flushdir(MVFS_FILE* file)
{
    __FILEOPS_HEAD(-EFAULT);
    MIXP_DIRENT* walk;
    for (walk=priv->dirents; walk; walk=priv->dirents)
    {
	mixp_stat_free(walk->stat);
	free(walk);
    }
    priv->dirents = priv->dirptr = NULL;
    return 0;
}

static int __mixp_readdir(MVFS_FILE* file)
{
    __FILEOPS_HEAD(-EFAULT);    
    
    // directory already loaded ?
    if (priv->dirents != NULL)
	return 0;
	
    MIXP_CFID*    fid;
    char* buf;
    int lflag; 
    int dflag;
    int i;

    {
	MIXP_STAT* stat = mixp_stat(MIXP_FS_CLIENT(file->fs), priv->pathname);
	if (stat==NULL)
	{
	    DEBUGMSG("couldnt stat dir: \"%s\"", priv->pathname);
	    return -ENOENT;
	}
	if ((stat->mode & P9_DMDIR) == 0)
	{
	    DEBUGMSG("file \"%s\" is not an directory", priv->pathname);
	    mixp_stat_free(stat);
	    return -1;
	}
	mixp_stat_free(stat);
    }
    
    fid = mixp_open(MIXP_FS_CLIENT(file->fs), priv->pathname, P9_OREAD);
    if (fid == NULL)
    {
	DEBUGMSG("couldnt open file \"%s\"", priv->pathname);
	return -ENOENT;
    }
    DEBUGMSG("successfully opened dir: \"%s\"", priv->pathname);
    fprintf(stderr,"mixp:readdir() opened dir: \"%s\"\n", priv->pathname);

    {
	int count;
	MIXP_STAT* newstat;
	MIXP_MESSAGE m;
	MIXP_DIRENT* entries = NULL;
	MIXP_DIRENT* walk = NULL;
	buf = calloc(1,fid->iounit);
	while ((count = mixp_read(fid, buf, fid->iounit))>0)
	{
	    m = mixp_message(buf, count, MsgUnpack);
	    while (m.pos < m.end)
	    {
		newstat = calloc(1,sizeof(MIXP_STAT));
		mixp_pstat(&m, newstat);
		if (entries == NULL)
		{
		    entries = walk = (MIXP_DIRENT*)calloc(1,sizeof(MIXP_DIRENT));
		}
		else
		{
		    walk->next = (MIXP_DIRENT*)calloc(1,sizeof(MIXP_DIRENT));
		    walk = walk->next;
		}
		walk->stat = newstat;
		newstat = NULL;
	    }
	}
	priv->dirents = priv->dirptr = entries;
    }
}

off64_t mvfs_mixpfs_fileops_seek (MVFS_FILE* file, off64_t offset, int whence)
{
    __FILEOPS_HEAD((off64_t)-1);

    // FIXME: should check if the position is valid
    switch (whence)
    {
	case SEEK_SET:	priv->pos = offset;	break;
	case SEEK_CUR:	priv->pos += offset;	break;
	case SEEK_END:	
	    DEBUGMSG("WARN: 9p/mixp: SEEK_END not implemented yet");
	    file->errcode = EINVAL;
	    return (off64_t)-1;
	default:
	    DEBUGMSG("WARN: mixp::seek() unknown whence %d", whence);
	    file->errcode = EINVAL;
	    return (off64_t)-1;
    }
}

ssize_t mvfs_mixpfs_fileops_pread (MVFS_FILE* file, void* buf, size_t count, off64_t offset)
{
    __FILEOPS_HEAD((ssize_t)-1);

    memset(buf, 0, count);
    ssize_t ret = mixp_pread(priv->cfid, buf, count, offset);
    if (ret<1)		// 0 and -1 signal EOF ;-o
    {
	priv->eof=1;
	return 0;
    }

    priv->pos+=ret;
    return ret;
}

ssize_t mvfs_mixpfs_fileops_read (MVFS_FILE* file, void* buf, size_t count)
{
    __FILEOPS_HEAD((ssize_t)-1);
    memset(buf, 0, count);
    ssize_t ret = mixp_read(priv->cfid, buf, count);
    if (ret<1)		// 0 and -1 signal EOF ;-o
    {
	priv->eof=1;
	return 0;
    }

    priv->pos+=ret;
    return ret;
}

ssize_t mvfs_mixpfs_fileops_write (MVFS_FILE* file, const void* buf, size_t count)
{
    __FILEOPS_HEAD((ssize_t)-1);
    ssize_t s = mixp_write(priv->cfid, buf, count);
    priv->pos+=s;
    return s;
}

ssize_t mvfs_mixpfs_fileops_pwrite (MVFS_FILE* file, const void* buf, size_t count, off64_t offset)
{
    __FILEOPS_HEAD(-1);
    ssize_t s = mixp_pwrite(priv->cfid, buf, count, offset);
    priv->pos+=s;
    return s;
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

int mvfs_mixpfs_fileops_setflag (MVFS_FILE* file, MVFS_FILE_FLAG flag, long value)
{
    __FILEOPS_HEAD(-1);
    DEBUGMSG("%s not supported", __mvfs_flag2str(flag));
    file->errcode = EINVAL;
    return -1;
}

int mvfs_mixpfs_fileops_getflag (MVFS_FILE* file, MVFS_FILE_FLAG flag, long* value)
{
    __FILEOPS_HEAD(-1);
    DEBUGMSG("%s not supported", __mvfs_flag2str(flag));
    file->errcode = EINVAL;
    return -1;
}

static inline MVFS_STAT* _convert_stat(MIXP_STAT* st)
{
    if (st == NULL)
    {
	ERRMSG("NULL stat");
	return NULL;
    }

    MVFS_STAT* stat_mvfs = (MVFS_STAT*)calloc(1,sizeof(MVFS_STAT));

    stat_mvfs->name  = SSTRDUP(st->name);
    stat_mvfs->uid   = SSTRDUP(st->uid);
    stat_mvfs->gid   = SSTRDUP(st->gid);

    stat_mvfs->mode  = 0;
    stat_mvfs->size  = st->length;
    stat_mvfs->mtime = st->mtime;
    stat_mvfs->atime = st->atime;
    stat_mvfs->ctime = st->mtime;

    int mode = st->mode;
    
    if (mode & P9_DMDIR)
	stat_mvfs->mode |= S_IFDIR;
    else if (mode & P9_DMSYMLINK)
	stat_mvfs->mode |= S_IFLNK;
    else if (mode & P9_DMDEVICE)
	stat_mvfs->mode |= S_IFCHR;
    else if (mode & P9_DMNAMEDPIPE)
	stat_mvfs->mode |= S_IFIFO;
    else if (mode & P9_DMSOCKET)
	stat_mvfs->mode |= S_IFSOCK;
    else
	stat_mvfs->mode |= S_IFREG;

    if (mode & P9_DMSETUID)
	stat_mvfs->mode |= S_ISUID;
    if (mode & P9_DMSETGID)
	stat_mvfs->mode |= S_ISGID;
    if (mode & S_IRUSR)
	stat_mvfs->mode |= S_IRUSR;
    if (mode & S_IWUSR)
	stat_mvfs->mode |= S_IWUSR;
    if (mode & S_IXUSR)
	stat_mvfs->mode |= S_IXUSR;
    if (mode & S_IRGRP)
	stat_mvfs->mode |= S_IRGRP;
    if (mode & S_IWGRP)
	stat_mvfs->mode |= S_IWGRP;
    if (mode & S_IXGRP)
	stat_mvfs->mode |= S_IXGRP;
    if (mode & S_IROTH)
	stat_mvfs->mode |= S_IROTH;
    if (mode & S_IWOTH)
	stat_mvfs->mode |= S_IWOTH;
    if (mode & S_IXOTH)
	stat_mvfs->mode |= S_IXOTH;

    return stat_mvfs;
}

MVFS_STAT* mvfs_mixpfs_fileops_stat(MVFS_FILE* file)
{
    __FILEOPS_HEAD(NULL);
    MIXP_STAT* mst = mixp_stat(MIXP_FS_CLIENT(file->fs), priv->pathname);
    MVFS_STAT* st = _convert_stat(mst);
    if (st == NULL)
	file->errcode = EINVAL;
    else
	file->errcode = 0;
    mixp_stat_free(mst);
    return st;
}

// FIXME: handle the various file modes !!!
MVFS_FILE* mvfs_mixpfs_fsops_open(MVFS_FILESYSTEM* fs, const char* name, mode_t mode)
{
    int m = 0;
    if (mode & O_RDWR)
	m = P9_ORDWR;
    else if (mode & O_WRONLY)
	m = P9_OWRITE;
    else 
	m = P9_OREAD;

    MIXP_CFID* fid = mixp_open(MIXP_FS_CLIENT(fs), name, m);
    if (fid == NULL)
    {
	DEBUGMSG("couldnt open file: \"%s\"", name);
	fs->errcode = ENOENT;
	return NULL;
    }
    else
	fprintf(stderr,"mvfs_mixpfs_fsops_open() opened file: \"%s\"\n", name);

    MVFS_FILE* file = mvfs_file_alloc(fs,mixpfs_fileops);
    MIXP_FILE_PRIV* priv = calloc(1,sizeof(MIXP_FILE_PRIV));
    file->priv.ptr = priv;
    
    priv->cfid = fid;
    priv->pos  = 0;
    priv->pathname = SSTRDUP(name);

    return file;
}

MVFS_STAT* mvfs_mixpfs_fsops_stat(MVFS_FILESYSTEM* fs, const char* name)
{
    if (fs==NULL)
    {
	ERRMSG("NULL fs passed !");
	return NULL;
    }

    MIXP_STAT* mst = mixp_stat(MIXP_FS_CLIENT(fs), name);
    MVFS_STAT* st = _convert_stat(mst);
    mixp_stat_free(mst);
    if (st == NULL)
    {
	ERRMSG("NULL stat");
	fs->errcode = EINVAL;
	return NULL;
    }

    return st;
}

int mvfs_mixpfs_fsops_unlink(MVFS_FILESYSTEM* fs, const char* name)
{
    DEBUGMSG("FIXME: DUMMY");
    fs->errcode = EPERM;
    return errno;
}

MVFS_FILESYSTEM* mvfs_mixpfs_create_args(MVFS_ARGS* args)
{
    const char* url = mvfs_args_get(args,"url");
    const char* path= mvfs_args_get(args,"path");
    
    if (path && strlen(path) && strcmp("/",path))
    {
	ERRMSG("chroot not supported yet path=\"%s\"", path);
	return NULL;
    }

    MIXP_SERVER_ADDRESS* addr = mixp_srv_addr_parse(url);
    if (addr==NULL)
    {
	ERRMSG("could not parse address \"%s\"", url);
	return NULL;
    }

    MIXP_CLIENT* client = mixp_mount_addr(addr);
    if (client == NULL)
    {
	ERRMSG("could not mount service @ \"%s\"", url);
	return NULL;
    }

    MVFS_FILESYSTEM* fs = mvfs_fs_alloc(mixpfs_fsops,FS_MAGIC);
    fs->priv.ptr=client;

    return fs;
}

int mvfs_mixpfs_fileops_close(MVFS_FILE* file)
{
    __FILEOPS_HEAD(-1);
    if (priv->cfid)
	mixp_close(priv->cfid);
    priv->cfid=NULL;
    priv->eof=0;
    priv->pos=-1;
    if (priv->pathname)
	free(priv->pathname);
    priv->pathname = NULL;
    return 0;
}

int mvfs_mixpfs_fileops_free(MVFS_FILE* file)
{
    __FILEOPS_HEAD(-1);
    mvfs_mixpfs_fileops_close(file);
    free(priv);
    file->priv.ptr = NULL;
    return 0;
}

int mvfs_mixpfs_fileops_eof(MVFS_FILE* file)
{
    __FILEOPS_HEAD(1);
    return ((priv->eof) ? 1 : 0);
}

MVFS_FILE* mvfs_mixpfs_fileops_lookup(MVFS_FILE* file, const char* name)
{
    __FILEOPS_HEAD(NULL);

    char* buffer = (char*)calloc(1,strlen(priv->pathname)+strlen(name)+10);    
    sprintf(buffer,"%s/%s", priv->pathname, name);

    DEBUGMSG("name=\"%s\" oldname=\"%s\" newname=\"%s\"", name, priv->pathname, buffer);

    MVFS_FILE* newfile = mvfs_mixpfs_fsops_open(file->fs, buffer, O_RDONLY);
    free(buffer);
    return newfile;
}

MVFS_STAT* mvfs_mixpfs_fileops_scan(MVFS_FILE* file)
{
    __FILEOPS_HEAD(NULL);
    __mixp_readdir(file);

    if (priv->dirptr == NULL)
	return NULL;
    
    MIXP_STAT* st = priv->dirptr->stat;
    if (st == NULL)
    {
	ERRMSG("UH! got an empty list entry!");
	return NULL;
    }

    MVFS_STAT* stat = _convert_stat(st);
    priv->dirptr = priv->dirptr->next;
    return stat;
}

int mvfs_mixpfs_fileops_reset(MVFS_FILE* file)
{
    __FILEOPS_HEAD(-1);
    __mixp_readdir(file);
    priv->dirptr = priv->dirents;
    return ((priv->dirptr == NULL) ? 0 : 1);
}
