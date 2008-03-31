// 
// metadata caching filesystem
//

// #define _DEBUG

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

#define	FS_MAGIC	"metux/metacache-fs-1"

static off64_t    _fileop_seek   (MVFS_FILE* file, off64_t offset, int whence);
static ssize_t    _fileop_pread  (MVFS_FILE* file, void* buf, size_t count, off64_t offset);
static ssize_t    _fileop_pwrite (MVFS_FILE* file, const void* buf, size_t count, off64_t offset);
static ssize_t    _fileop_read   (MVFS_FILE* file, void* buf, size_t count);
static ssize_t    _fileop_write  (MVFS_FILE* file, const void* buf, size_t count);
static int        _fileop_close  (MVFS_FILE* file);
static int        _fileop_free   (MVFS_FILE* file);
static int        _fileop_eof    (MVFS_FILE* file);
static MVFS_STAT* _fileop_stat   (MVFS_FILE* file);
static MVFS_FILE* _fileop_lookup (MVFS_FILE* file, const char* name);
static MVFS_STAT* _fileop_scan   (MVFS_FILE* file);
static int        _fileop_reset  (MVFS_FILE* file);

static MVFS_FILE_OPS _fileops = 
{
    .seek	= _fileop_seek,
    .read       = _fileop_read,
    .write      = _fileop_write,
    .pread	= _fileop_pread,
    .pwrite	= _fileop_pwrite,
    .close	= _fileop_close,
    .free	= _fileop_free,
    .eof        = _fileop_eof,
    .lookup     = _fileop_lookup,
    .scan       = _fileop_scan,
    .reset      = _fileop_reset,
    .stat       = _fileop_stat
};

static MVFS_STAT* _fsops_stat   (MVFS_FILESYSTEM* fs, const char* name);
static MVFS_FILE* _fsops_open   (MVFS_FILESYSTEM* fs, const char* name, mode_t mode);
static int        _fsops_unlink (MVFS_FILESYSTEM* fs, const char* name);

static MVFS_FILESYSTEM_OPS _fsops = 
{
    .openfile	= _fsops_open,
    .unlink	= _fsops_unlink,
    .stat       = _fsops_stat
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
	METACACHE_FS_PRIV* fspriv = (file->fs->priv.ptr);		\
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
	METACACHE_FS_PRIV* fspriv = (fs->priv.ptr);			\
	if (fspriv == NULL)						\
	{								\
	    fprintf(stderr,"%s() corrupt fs handle\n", __FUNCTION__);	\
	    return err;							\
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
#ifdef _DEBUG
	    fprintf(stderr,"metacache_fs->lookup: cached: %s\n", filename);
#endif
	    return rec;
	}
	
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
    if (rec->stat != NULL)
	mvfs_stat_free(rec->stat);
    if (stat != NULL)
        rec->stat = mvfs_stat_dup(st);
    else
	rec->stat = NULL;
}

off64_t _fileop_seek (MVFS_FILE* file, off64_t offset, int whence)
{
    __FILEOPS_HEAD((off64_t)-1);
    return mvfs_file_seek(priv->cfid, offset, whence);
}

ssize_t _fileop_pread (MVFS_FILE* file, void* buf, size_t count, off64_t offset)
{
    __FILEOPS_HEAD((ssize_t)-1);
    return mvfs_file_pread(priv->cfid, buf, count, offset);
}

ssize_t _fileop_read (MVFS_FILE* file, void* buf, size_t count)
{
    __FILEOPS_HEAD((ssize_t)-1);
    return mvfs_file_read(priv->cfid, buf, count);
}

ssize_t _fileop_write (MVFS_FILE* file, const void* buf, size_t count)
{
    __FILEOPS_HEAD((ssize_t)-1);
    return mvfs_file_write(priv->cfid, buf, count);
}

ssize_t _fileop_pwrite (MVFS_FILE* file, const void* buf, size_t count, off64_t offset)
{
    __FILEOPS_HEAD(-1);
    return mvfs_file_pwrite(priv->cfid, buf, count, offset);
}

int _fileop_setflag (MVFS_FILE* file, MVFS_FILE_FLAG flag, long value)
{
    __FILEOPS_HEAD(-1);
    return mvfs_file_setflag(priv->cfid, flag, value);
}

int _fileop_getflag (MVFS_FILE* file, MVFS_FILE_FLAG flag, long* value)
{
    __FILEOPS_HEAD(-1);
    return mvfs_file_getflag(priv->cfid, flag, value);
}

MVFS_STAT* _fileop_stat(MVFS_FILE* file)
{
    __FILEOPS_HEAD(NULL);

    METACACHE_RECORD* rec = _cache_lookup(fspriv, priv->pathname);
    if (rec->stat != NULL)
    {
#ifdef _DEBUG
	fprintf(stderr,"got an stat record for %s\n", priv->pathname);
#endif
	return mvfs_stat_dup(rec->stat);
    }

    MVFS_STAT* st = mvfs_file_stat(priv->cfid);
    if (st==NULL)
    {
	fprintf(stderr,"metacache_fs: stat() failed :((\n");
	return NULL;
    }

    _cache_set(rec, st);    
#ifdef _DEBUG
    fprintf(stderr,"metacache_fs: refreshed / added stat to cache for: %s\n", priv->pathname);
#endif
    return st;
}

MVFS_FILE* _open_cfid(MVFS_FILESYSTEM* fs, MVFS_FILE* cfid, const char* name)
{
    MVFS_FILE* file = mvfs_file_alloc(fs, _fileops);
    METACACHE_FILE_PRIV* priv = calloc(1,sizeof(METACACHE_FILE_PRIV));
    file->priv.ptr = priv;
    priv->cfid = cfid;
    priv->pathname = strdup(name);
    return file;
}

MVFS_FILE* _fsops_open(MVFS_FILESYSTEM* fs, const char* name, mode_t mode)
{
    __FSOPS_HEAD(NULL);

    MVFS_FILE* fid = mvfs_fs_openfile(fspriv->fs, name, mode);
    if (fid == NULL)
    {
#ifdef _DEBUG
	fprintf(stderr,"_fileop_open() couldnt open file: \"%s\"\n", name);
#endif
	fs->errcode = fspriv->fs->errcode;
	return NULL;
    }
    
    return _open_cfid(fs, fid, name);
}

MVFS_STAT* _fsops_stat(MVFS_FILESYSTEM* fs, const char* filename)
{
    __FSOPS_HEAD(NULL);
#ifdef _DEBUG
    fprintf(stderr,"metacache::FS::stat() lookup: %s\n", filename);
#endif

    METACACHE_RECORD* rec = _cache_lookup(fspriv, filename);
    if (rec->stat != NULL)
    {
#ifdef _DEBUG
	fprintf(stderr,"metacache::FS::stat() got an stat record for %s (%s)\n", filename, rec->stat->name);
#endif
	MVFS_STAT* newst = mvfs_stat_dup(rec->stat);
	return newst;
    }
    
    MVFS_STAT* st = mvfs_fs_statfile(fspriv->fs, filename);
    if (st==NULL)
    {
	fprintf(stderr,"metacache_fs: stat() failed :((\n");
	return NULL;
    }

    _cache_set(rec, st);    
#ifdef _DEBUG
    fprintf(stderr,"metacache_fs: refreshed / added stat to cache for: %s\n", filename);
#endif
    return st;
}

int _fsops_unlink(MVFS_FILESYSTEM* fs, const char* filename)
{
    __FSOPS_HEAD(-EFAULT);
#ifdef _DEBUG
    fprintf(stderr,"metacache::unlink() name=\"%s\"\n", filename);
#endif
    METACACHE_RECORD* rec = _cache_lookup(fspriv, filename);
    _cache_set(rec, NULL);
    return mvfs_fs_unlink(fspriv->fs, filename);
}

void free_CACHEENT(void* ptr)
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
	fprintf(stderr,"mvfs_metacachefs_create_1() NULL fs passed\n");
	return NULL;
    }

    MVFS_FILESYSTEM* newfs = mvfs_fs_alloc(_fsops, FS_MAGIC);
    METACACHE_FS_PRIV* fspriv = calloc(1,sizeof(METACACHE_FS_PRIV));
    newfs->priv.ptr=fspriv;
    fspriv->fs = clientfs;
    hash_initialise(&(fspriv->cache), 997U, hash_hash_string, hash_compare_string, hash_copy_string, free, free_CACHEENT);

    return newfs;
}

int _fileop_close(MVFS_FILE* file)
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

int _fileop_free(MVFS_FILE* file)
{
    __FILEOPS_HEAD(-1);
    _fileop_close(file);
    free(priv);
    file->priv.ptr = NULL;
    return 0;
}

int _fileop_eof(MVFS_FILE* file)
{
    __FILEOPS_HEAD(1);
    return mvfs_file_eof(priv->cfid);
}

MVFS_FILE* _fileop_lookup(MVFS_FILE* file, const char* name)
{
    __FILEOPS_HEAD(NULL);
    MVFS_FILE* f = mvfs_file_lookup(file, name);
    if (f==NULL)
	return NULL;
    
    return _open_cfid(file->fs, f, name);
}

MVFS_STAT* _fileop_scan(MVFS_FILE* file)
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

int _fileop_reset(MVFS_FILE* file)
{
    __FILEOPS_HEAD(-1);
    return mvfs_file_reset(priv->cfid);
}
