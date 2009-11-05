/*
    libmvfs - metux Virtual Filesystem Library

    Filesystem driver: metadata-caching fs

    Copyright (C) 2008 Enrico Weigelt, metux IT service <weigelt@metux.de>
    This code is published under the terms of the GNU Public License 2.0
*/

#include "mvfs-internal.h"

#define _LARGEFILE64_SOURCE

#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <malloc.h>
#include <hash.h>

#include <mvfs/mvfs.h>
#include <mvfs/stat.h>
#include <mvfs/default_ops.h>
#include <mvfs/mixpfs.h>
#include <mvfs/_utils.h>

#define	FS_MAGIC	"metux/metacache-fs-1"

static off64_t    _mvfs_metacache_fileopseek   (MVFS_FILE* file, off64_t offset, int whence);
static ssize_t    _mvfs_metacache_fileoppread  (MVFS_FILE* file, void* buf, size_t count, off64_t offset);
static ssize_t    _mvfs_metacache_fileoppwrite (MVFS_FILE* file, const void* buf, size_t count, off64_t offset);
static ssize_t    _mvfs_metacache_fileopread   (MVFS_FILE* file, void* buf, size_t count);
static ssize_t    _mvfs_metacache_fileopwrite  (MVFS_FILE* file, const void* buf, size_t count);
static int        _mvfs_metacache_fileopclose  (MVFS_FILE* file);
static int        _mvfs_metacache_fileopfree   (MVFS_FILE* file);
static int        _mvfs_metacache_fileopeof    (MVFS_FILE* file);
static MVFS_STAT* _mvfs_metacache_fileopstat   (MVFS_FILE* file);
static MVFS_FILE* _mvfs_metacache_fileoplookup (MVFS_FILE* file, const char* name);
static MVFS_STAT* _mvfs_metacache_fileopscan   (MVFS_FILE* file);
static int        _mvfs_metacache_fileopreset  (MVFS_FILE* file);

static MVFS_FILE_OPS _fileops = 
{
    .seek	= _mvfs_metacache_fileopseek,
    .read       = _mvfs_metacache_fileopread,
    .write      = _mvfs_metacache_fileopwrite,
    .pread	= _mvfs_metacache_fileoppread,
    .pwrite	= _mvfs_metacache_fileoppwrite,
    .close	= _mvfs_metacache_fileopclose,
    .free	= _mvfs_metacache_fileopfree,
    .eof        = _mvfs_metacache_fileopeof,
    .lookup     = _mvfs_metacache_fileoplookup,
    .scan       = _mvfs_metacache_fileopscan,
    .reset      = _mvfs_metacache_fileopreset,
    .stat       = _mvfs_metacache_fileopstat
};

static MVFS_STAT*   _mvfs_metacache_fsop_stat     (MVFS_FILESYSTEM* fs, const char* name);
static MVFS_FILE*   _mvfs_metacache_fsop_open     (MVFS_FILESYSTEM* fs, const char* name, mode_t mode);
static int          _mvfs_metacache_fsop_unlink   (MVFS_FILESYSTEM* fs, const char* name);
static int          _mvfs_metacache_fsop_chmod    (MVFS_FILESYSTEM* fs, const char* name, mode_t mode);
static MVFS_SYMLINK _mvfs_metacache_fsop_readlink (MVFS_FILESYSTEM* fs, const char* name);

static MVFS_FILESYSTEM_OPS _fsops = 
{
    .openfile	= _mvfs_metacache_fsop_open,
    .unlink	= _mvfs_metacache_fsop_unlink,
    .stat       = _mvfs_metacache_fsop_stat,
    .chmod      = _mvfs_metacache_fsop_chmod,
    .readlink   = _mvfs_metacache_fsop_readlink
};

typedef struct
{
    MVFS_STAT* stat;
    uint64_t   mtime;
    char*      filename;
} METACACHE_RECORD;

typedef struct 
{
    MVFS_FILE*   cfid;
    char*        pathname;
} METACACHE_FILE_PRIV;

typedef struct
{
    MVFS_FILESYSTEM*	fs;    
    hash		cache;
} METACACHE_FS_PRIV;

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
	METACACHE_FS_PRIV* fspriv = (file->fs->priv.ptr);	\
	if (fspriv == NULL)					\
	{							\
	    ERRMSG("corrupt fs handle");			\
	    return err;						\
	}

#define __FSOPS_HEAD(err);					\
	if (fs==NULL)						\
	{							\
	    ERRMSG("NULL fs handle");				\
	    return err;						\
	}							\
	METACACHE_FS_PRIV* fspriv = (fs->priv.ptr);		\
	if (fspriv == NULL)					\
	{							\
	    ERRMSG("corrupt fspriv");				\
	    return err;						\
	}							\
	if (fspriv->fs == NULL)					\
	{							\
	    ERRMSG("missing backend fs");			\
	    return err;						\
	}

// default cache timeout is 5sec (1^6 microseconds)
#define CACHE_TIMEOUT	(5000000)

static inline uint64_t _curtime()
{
    uint64_t t;
    struct timeval tv;
    tv.tv_sec  = 0;
    tv.tv_usec = 0;
    t = tv.tv_sec*1000000+tv.tv_usec;
    return t;
}

static METACACHE_RECORD* _cache_lookup(METACACHE_FS_PRIV* fspriv, const char* filename)
{
    METACACHE_RECORD* rec;
    
    // FIXME: do a lookup
    if ((hash_retrieve(&(fspriv->cache), (char*)filename, (void**)&rec)) && (rec!=NULL))
    {
	if ((rec->mtime+CACHE_TIMEOUT) > _curtime()) 
	{
	    DEBUGMSG("cached: %s", filename);
	    return rec;
	}
	
	DEBUGMSG("purging old cache record for %s", filename);
	mvfs_stat_free(rec->stat);
	rec->stat = NULL;
	return rec;
    }

    // ... lookup failed - no record yet
    rec = calloc(1,sizeof(METACACHE_RECORD));
    rec->filename = strdup(filename);
    
    // now insert
    hash_insert(&(fspriv->cache),rec->filename,rec);
    return rec;
}

static void _cache_set(METACACHE_RECORD* rec, MVFS_STAT* st)
{
    rec->mtime = _curtime();
    mvfs_stat_free(rec->stat);
    rec->stat = (st ? mvfs_stat_dup(st) : NULL);
}

static void _cache_clear(METACACHE_RECORD* rec)
{
    if (rec == NULL)
    {
	DEBUGMSG("NULL rec passed");
	return;
    }
    rec->mtime = _curtime();
    mvfs_stat_free(rec->stat);
    rec->stat = NULL;
}

static off64_t _mvfs_metacache_fileopseek (MVFS_FILE* file, off64_t offset, int whence)
{
    __FILEOPS_HEAD((off64_t)-1);
    return mvfs_file_seek(priv->cfid, offset, whence);
}

static ssize_t _mvfs_metacache_fileoppread (MVFS_FILE* file, void* buf, size_t count, off64_t offset)
{
    __FILEOPS_HEAD((ssize_t)-1);
    return mvfs_file_pread(priv->cfid, buf, count, offset);
}

static ssize_t _mvfs_metacache_fileopread (MVFS_FILE* file, void* buf, size_t count)
{
    __FILEOPS_HEAD((ssize_t)-1);
    return mvfs_file_read(priv->cfid, buf, count);
}

static ssize_t _mvfs_metacache_fileopwrite (MVFS_FILE* file, const void* buf, size_t count)
{
    __FILEOPS_HEAD((ssize_t)-1);
    return mvfs_file_write(priv->cfid, buf, count);
}

static ssize_t _mvfs_metacache_fileoppwrite (MVFS_FILE* file, const void* buf, size_t count, off64_t offset)
{
    __FILEOPS_HEAD(-1);
    return mvfs_file_pwrite(priv->cfid, buf, count, offset);
}

static int _mvfs_metacache_fileopsetflag (MVFS_FILE* file, MVFS_FILE_FLAG flag, long value)
{
    __FILEOPS_HEAD(-1);
    return mvfs_file_setflag(priv->cfid, flag, value);
}

static int _mvfs_metacache_fileopgetflag (MVFS_FILE* file, MVFS_FILE_FLAG flag, long* value)
{
    __FILEOPS_HEAD(-1);
    return mvfs_file_getflag(priv->cfid, flag, value);
}

static MVFS_STAT* _mvfs_metacache_fileopstat(MVFS_FILE* file)
{
    __FILEOPS_HEAD(NULL);

    METACACHE_RECORD* rec = _cache_lookup(fspriv, priv->pathname);
    if (rec->stat != NULL)
    {
	DEBUGMSG("got an stat record for %s", priv->pathname);
	return mvfs_stat_dup(rec->stat);
    }

    MVFS_STAT* st = mvfs_file_stat(priv->cfid);
    if (st==NULL)
    {
	DEBUGMSG("stat() failed :((");
	return NULL;
    }

    _cache_set(rec, st);
    DEBUGMSG("refreshed / added stat to cache for: %s", priv->pathname);

    return st;
}

static MVFS_FILE* _open_cfid(MVFS_FILESYSTEM* fs, MVFS_FILE* cfid, const char* name)
{
    MVFS_FILE* file = mvfs_file_alloc(fs, _fileops);
    METACACHE_FILE_PRIV* priv = calloc(1,sizeof(METACACHE_FILE_PRIV));
    file->priv.ptr = priv;
    priv->cfid = cfid;
    priv->pathname = strdup(name);
    return file;
}

static MVFS_FILE* _mvfs_metacache_fsop_open(MVFS_FILESYSTEM* fs, const char* name, mode_t mode)
{
    __FSOPS_HEAD(NULL);

    MVFS_FILE* fid = mvfs_fs_openfile(fspriv->fs, name, mode);
    if (fid == NULL)
    {
	DEBUGMSG("couldnt open file: \"%s\"", name);
	fs->errcode = fspriv->fs->errcode;
	return NULL;
    }
    
    return _open_cfid(fs, fid, name);
}

static MVFS_STAT* _mvfs_metacache_fsop_stat(MVFS_FILESYSTEM* fs, const char* filename)
{
    __FSOPS_HEAD(NULL);
    DEBUGMSG("lookup: %s", filename);


    METACACHE_RECORD* rec = _cache_lookup(fspriv, filename);
    if (rec->stat != NULL)
    {
	DEBUGMSG("got an stat record for %s (%s)", filename, rec->stat->name);
	MVFS_STAT* newst = mvfs_stat_dup(rec->stat);
	return newst;
    }
    
    MVFS_STAT* st = mvfs_fs_statfile(fspriv->fs, filename);
    if (st==NULL)
    {
	DEBUGMSG("stat() failed :((");
	return NULL;
    }

    _cache_set(rec, st);
    DEBUGMSG("refreshed / added stat to cache for: %s", filename);

    return st;
}

static int _mvfs_metacache_fsop_unlink(MVFS_FILESYSTEM* fs, const char* filename)
{
    __FSOPS_HEAD(-EFAULT);
    DEBUGMSG("name=\"%s\"", filename);

    METACACHE_RECORD* rec = _cache_lookup(fspriv, filename);
    _cache_clear(rec);
    int ret = mvfs_fs_unlink(fspriv->fs, filename);

    DEBUGMSG("unlink() fs returned %d", ret);
    return ret;
}

// FIXME: should also cache this, perhaps in the stat structure ? or some extension ?
static MVFS_SYMLINK _mvfs_metacache_fsop_readlink(MVFS_FILESYSTEM* fs, const char* filename)
{
    __FSOPS_HEAD(((MVFS_SYMLINK){.errcode = -EFAULT}));
    return mvfs_fs_readlink(fspriv->fs, filename);
}

static int _mvfs_metacache_fsop_chmod(MVFS_FILESYSTEM* fs, const char* filename, mode_t mode)
{
    __FSOPS_HEAD(-EFAULT);
    DEBUGMSG("name=\"%s\"", filename);

    METACACHE_RECORD* rec = _cache_lookup(fspriv, filename);
    _cache_clear(rec);
    int ret = mvfs_fs_chmod(fspriv->fs, filename, mode);

    DEBUGMSG("fs returned %d", ret);
    return ret;
}

static void free_CACHEENT(void* ptr)
{
    METACACHE_RECORD* rec = (METACACHE_RECORD*)ptr;
    if (rec->stat)
	mvfs_stat_free(rec->stat);
    free(rec);
}

MVFS_FILESYSTEM* mvfs_metacachefs_create_1(MVFS_FILESYSTEM* clientfs)
{
    if (clientfs==NULL)
    {
	DEBUGMSG("NULL fs passed");
	return NULL;
    }

    MVFS_FILESYSTEM* newfs = mvfs_fs_alloc(_fsops, FS_MAGIC);
    METACACHE_FS_PRIV* fspriv = calloc(1,sizeof(METACACHE_FS_PRIV));
    newfs->priv.ptr=fspriv;
    fspriv->fs = clientfs;
    hash_initialise(&(fspriv->cache), 997U, hash_hash_string, hash_compare_string, hash_copy_string, free, free_CACHEENT);

    return newfs;
}

static int _mvfs_metacache_fileopclose(MVFS_FILE* file)
{
    __FILEOPS_HEAD(-1);
    if (priv->cfid)
	mvfs_file_close(priv->cfid);
    priv->cfid=NULL;
    if (priv->pathname)
	free(priv->pathname);
    priv->pathname = NULL;
    return 0;
}

static int _mvfs_metacache_fileopfree(MVFS_FILE* file)
{
    __FILEOPS_HEAD(-1);
    _mvfs_metacache_fileopclose(file);
    free(priv);
    file->priv.ptr = NULL;
    return 0;
}

static int _mvfs_metacache_fileopeof(MVFS_FILE* file)
{
    __FILEOPS_HEAD(1);
    return mvfs_file_eof(priv->cfid);
}

static MVFS_FILE* _mvfs_metacache_fileoplookup(MVFS_FILE* file, const char* name)
{
    __FILEOPS_HEAD(NULL);
    MVFS_FILE* f = mvfs_file_lookup(file, name);
    if (f==NULL)
	return NULL;
    
    return _open_cfid(file->fs, f, name);
}

static MVFS_STAT* _mvfs_metacache_fileopscan(MVFS_FILE* file)
{
    __FILEOPS_HEAD(NULL);
    MVFS_STAT* st = mvfs_file_scan(priv->cfid);
    if (st==NULL)
	return NULL;

    char* fn = calloc(1,strlen(priv->pathname)+strlen(st->name)+3);
    sprintf(fn, "%s/%s", priv->pathname, st->name);
    
    METACACHE_RECORD* rec = _cache_lookup(fspriv, fn);
    _cache_set(rec, st);
    return st;
}

static int _mvfs_metacache_fileopreset(MVFS_FILE* file)
{
    __FILEOPS_HEAD(-1);
    return mvfs_file_reset(priv->cfid);
}
