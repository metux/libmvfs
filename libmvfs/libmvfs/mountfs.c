/*
    libmvfs - metux Virtual Filesystem Library

    Filesystem driver: Mounting filesystem

    Copyright (C) 2008 Enrico Weigelt, metux IT service <weigelt@metux.de>
    This code is published under the terms of the GNU Public License 2.0
*/

#define _LARGEFILE64_SOURCE

#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <malloc.h>
#include <strings.h>

#include <mvfs/mvfs.h>
#include <mvfs/default_ops.h>
#include <mvfs/mountfs.h>
#include <mvfs/_utils.h>

#include <9p-mixp/mixp.h>

#define MIXP_FILE_POS(fp)	(fp->priv.pos)
#define MIXP_FILE_HANDLE(fp)	((MIXP_CFID*)(fp->priv.ptr))
#define MIXP_FILE_EOFSTAT(fp)	(fp->priv.status)
#define MIXP_FS_CLIENT(fs)	((MIXP_CLIENT*)(fs->priv.ptr))

#define FS_MAGIC		"metux/mountfs-1"

typedef struct _mountfs_node	MOUNTFS_NODE;
typedef struct _mountfs_def     MOUNTFS_DEF;

struct _mountfs_node
{
    int              refcount;
    const char*      name;
    MVFS_FILESYSTEM* fs;
    MOUNTFS_NODE*    childs;
    MOUNTFS_NODE*    next;
};

static inline MOUNTFS_NODE* _node_alloc(const char* name)
{
    MOUNTFS_NODE* node = calloc(1,sizeof(MOUNTFS_NODE));
    node->refcount = 1;
    node->name     = strdup(name);
    return node;
}

static inline MOUNTFS_NODE* _node_find(MOUNTFS_NODE* parent, const char* name)
{
    if ((parent == NULL)||(name==NULL))
	return NULL;
    MOUNTFS_NODE* ch;
    for (ch=parent->childs; ch; ch=ch->next)
	if (strcmp(name,ch->name)==0)
	    return ch;

    return NULL;
}

static inline int _node_ref(MOUNTFS_NODE* node)
{
    if (node==NULL)
	return -EFAULT;
	
    node->refcount++;
    return 0;
}

static inline int _node_unref(MOUNTFS_NODE* node)
{
    if (node==NULL)
	return -EFAULT;
    
    node->refcount--;
    if (node->refcount>0)
	return 0;

    // node not referenced anymore ... unref childs and release memory
    MOUNTFS_NODE* walk;
    for (walk=node->childs; walk; walk=walk->next)
	_node_unref(walk);
    
    if (node->name)
	free((char*)node->name);
	
    free(node);
    return 1;
}

static inline MOUNTFS_NODE* _node_get(MOUNTFS_NODE* parent, const char* name)
{
    MOUNTFS_NODE* n = _node_find(parent, name);

    if (n!=NULL)
	return n;

    DEBUGMSG("Couldnt find node \"%s\" ... creating new one", name);
    n = _node_alloc(name);
    n->next = parent->childs;
    parent->childs = n;

    return n;
}

static inline int _mount_fs_on_node(MOUNTFS_NODE* node, MVFS_FILESYSTEM* fs)
{
    if (node->fs != NULL)
    {
	DEBUGMSG("node already mounted ... unmounting prev fs");
	mvfs_fs_unref(node->fs);
	node->fs = NULL;
    }

    mvfs_fs_ref(fs);
    node->fs = fs;
    
    return 1;
}

static inline int _mount_fs(MOUNTFS_NODE* root, MVFS_FILESYSTEM* fs, const char* name)
{
    char* buffer = strdup(name);
    char* saveptr = NULL;
    char* tok = NULL;

    if (strcmp(name,"/")==0)
    {
	DEBUGMSG("mounting on root node");
	return _mount_fs_on_node(root, fs);
    }

    while (tok = strtok_r(buffer, "/", &saveptr))
    {
	DEBUGMSG("mount: component: \"%s\"", tok);
	root = _node_get(root, tok);
    }
    return _mount_fs_on_node(root, fs);
}

int mvfs_mountfs_mount(MVFS_FILESYSTEM* fs, MVFS_FILESYSTEM* newfs, const char* mountpoint)
{
    if (fs==NULL)
    {
	ERRMSG("mvfs_mountfs_mount() NULL fs");
	return -EFAULT;
    }
    if (newfs==NULL)
	ERRMSG("mvfs_mountfs_mount() NULL newfs");
    if (strcmp(fs->magic, FS_MAGIC))
	ERRMSG("mvfs_mountfs_mount() corrupt magic: is=\"%s\" should be \"%s\"", fs->magic, FS_MAGIC);
    
    return -1;
}

static off64_t mvfs_mountfs_fileops_seek  (MVFS_FILE* file, off64_t offset, int whence);
static ssize_t mvfs_mountfs_fileops_pread  (MVFS_FILE* file, void* buf, size_t count);
static ssize_t mvfs_mountfs_fileops_pwrite (MVFS_FILE* file, const void* buf, size_t count);
static int     mvfs_mountfs_fileops_close (MVFS_FILE* file);
static int     mvfs_mountfs_fileops_free  (MVFS_FILE* file);
static int     mvfs_mountfs_fileops_eof   (MVFS_FILE* file);

static MVFS_FILE_OPS mountfs_fileops = 
{
    .seek	= mvfs_mountfs_fileops_seek,
    .read	= mvfs_mountfs_fileops_pread,
    .write	= mvfs_mountfs_fileops_pwrite,
    .close	= mvfs_mountfs_fileops_close,
    .free	= mvfs_mountfs_fileops_free,
    .eof        = mvfs_mountfs_fileops_eof
};

static MVFS_FILE* mvfs_mountfs_fsops_openfile (MVFS_FILESYSTEM* fs, const char* name, mode_t mode);
static int        mvfs_mountfs_fsops_unlink   (MVFS_FILESYSTEM* fs, const char* name);
MVFS_STAT*        mvfs_mountfs_fsops_statfile(MVFS_FILESYSTEM* fs, const char* name);

static MVFS_FILESYSTEM_OPS mountfs_fsops = 
{
    .openfile	= mvfs_mountfs_fsops_openfile,
    .unlink	= mvfs_mountfs_fsops_unlink,
    .stat       = mvfs_mountfs_fsops_statfile
};

off64_t mvfs_mountfs_fileops_seek (MVFS_FILE* file, off64_t offset, int whence)
{
    if (MIXP_FILE_HANDLE(file) == NULL)
    {
	ERRMSG("mixp::seek() corrupt handle");
	return -1;
    }

    // FIXME: should check if the position is valid
    switch (whence)
    {
	case SEEK_SET:	MIXP_FILE_POS(file) = offset;	break;
	case SEEK_CUR:	MIXP_FILE_POS(file) += offset;	break;
	case SEEK_END:	
	    ERRMSG("WARN: 9p/mixp: SEEK_END not implemented yet");
	    file->errcode = EINVAL;
	    return (off64_t)-1;
	default:
	    ERRMSG("WARN: mixp::seek() unknown whence %d", whence);
	    file->errcode = EINVAL;
	    return (off64_t)-1;
    }
}

ssize_t mvfs_mountfs_fileops_pread (MVFS_FILE* file, void* buf, size_t count)
{
    if (MIXP_FILE_HANDLE(file) == NULL)
    {
	ERRMSG("mixp::pread() corrupt handle");
	return -1;
    }

    memset(buf, 0, count);
    ssize_t ret = ixp_read(MIXP_FILE_HANDLE(file), buf, count);
    if (ret<1)		// 0 and -1 signal EOF ;-o
    {
	MIXP_FILE_EOFSTAT(file)=1;
	return 0;
    }

    MIXP_FILE_POS(file)+=ret;
    return ret;
}

ssize_t mvfs_mountfs_fileops_pwrite (MVFS_FILE* file, const void* buf, size_t count)
{
    if (MIXP_FILE_HANDLE(file) == NULL)
    {
	ERRMSG("mixp::write() corrupt handle");
	return -1;
    }
    ssize_t s = ixp_write(MIXP_FILE_HANDLE(file), buf, count);
    MIXP_FILE_POS(file)+=s;
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

int mvfs_mountfs_fileops_setflag (MVFS_FILE* fp, MVFS_FILE_FLAG flag, long value)
{
    ERRMSG("mvfs_mountfs_fileops_setflag() %s not supported", __mvfs_flag2str(flag));
    fp->errcode = EINVAL;
    return -1;
}

int mvfs_mountfs_fileops_getflag (MVFS_FILE* fp, MVFS_FILE_FLAG flag, long* value)
{
    ERRMSG("mvfs_mountfs_fileops_getflag() %s not supported", __mvfs_flag2str(flag));
    fp->errcode = EINVAL;
    return -1;
}

MVFS_STAT* mvfs_mountfs_fileops_stat(MVFS_FILE* fp)
{
    ERRMSG("mvfs_mountfs_fileops_stat() not supported");
    fp->errcode = EINVAL;
    return NULL;
}

typedef struct 
{
    MOUNTFS_NODE* node;
    const char* filename;
} MOUNTFS_LOC;

//
// The _walktree() functions tries to find the filesystem responsible for path
// On success, true is returned and result_fs + result_path is set to the
// appropriate values (result_path is the path relative to the fs' root)
//
int _walktree(MOUNTFS_NODE* root, const char* path, MVFS_FILESYSTEM** result_fs, const char** result_path)
{
    // its legal to us w/ NULL root, eg. if the prev instance hasn't found a child - just return false
    if (root==NULL)
	return 0;
    
    // skip leading slashes
    while ((*path)=='/')
	path++;
	
    // split the current path into current_name and rest_path
    const char* rest_path = strchr(path,'/');
    MOUNTFS_NODE* sub_node = NULL;

    {
	char* sub_name;
	if (rest_path == NULL)
	{
	    sub_name = strdup(path);
	    rest_path = "";
	}
	else
	{
	    // rest_path stands 
	    sub_name = strndup(path,(rest_path-path));
	    while ((*rest_path)=='/')
		rest_path++;
	}

	DEBUGMSG("Diving sub_name=\"%s\"", sub_name);
	DEBUGMSG("       rest_path=\"%s\"", rest_path);

	// look if we have an sub-node
	sub_node = _node_find(root,sub_name);
	if (sub_node)
	    DEBUGMSG("Found sub");
	else
	    DEBUGMSG("No sub");

	free(sub_name);
    }

    if (_walktree(sub_node, rest_path, result_fs, result_path))
	return 1;

    // if sub call has found a matching fs, just return true

    // diving into sub hasn't succeed, 
    // maybe our current node fits ?
    if (root->fs != NULL)
    {
	*result_fs   = root->fs;
	*result_path = rest_path;
	return 1;
    }
    
    return 0;
}

// FIXME: handle the various file modes !!!
MVFS_FILE* mvfs_mountfs_fsops_openfile(MVFS_FILESYSTEM* fs, const char* name, mode_t mode)
{
    MOUNTFS_NODE* root = (MOUNTFS_NODE*)fs->priv.ptr;

    MVFS_FILESYSTEM* found_fs; 
    const char*      found_path;
    
    if (_walktree(root, name, &found_fs, &found_path))
	return mvfs_fs_openfile(found_fs, found_path, mode);

    ERRMSG("Could not find mountpoint for \"%s\" mode=%d", name, mode);
    return NULL;
}

MVFS_STAT* mvfs_mountfs_fsops_statfile(MVFS_FILESYSTEM* fs, const char* name)
{
    MOUNTFS_NODE* root = (MOUNTFS_NODE*)fs->priv.ptr;
    
    MVFS_FILESYSTEM* found_fs;
    const char*      found_path;
    
    if (_walktree(root, name, &found_fs, &found_path))
	return mvfs_fs_statfile(found_fs, found_path);
	
    ERRMSG("Could not find mountpoint for \"%s\"", name);
    return NULL;
}

int mvfs_mountfs_fsops_unlink(MVFS_FILESYSTEM* fs, const char* name)
{
    MOUNTFS_NODE* root = (MOUNTFS_NODE*)fs->priv.ptr;
    
    MVFS_FILESYSTEM* found_fs;
    const char*      found_path;
    
    if (_walktree(root, name, &found_fs, &found_path))
	return mvfs_fs_unlink(found_fs, found_path);
    
    ERRMSG("Could not find mountpoint for \"%s\"", name);
    return -ENOENT;
}

MVFS_FILESYSTEM* mvfs_mountfs_create(MVFS_MOUNTFS_PARAM par)
{
    MVFS_FILESYSTEM* fs = mvfs_fs_alloc(mountfs_fsops,FS_MAGIC);
    fs->priv.ptr=_node_alloc("/");
    return fs;
}

int mvfs_mountfs_fileops_close(MVFS_FILE* file)
{
//    if (MIXP_FILE_HANDLE(file) == NULL)
//	return -1;
//
//    file->priv.ptr=NULL;
//    return 0;
}

int mvfs_mountfs_fileops_free(MVFS_FILE* file)
{
//    if (MIXP_FILE_HANDLE(file) != NULL)
//	mvfs_mountfs_fileops_close(file);
//    return 0;
}

int mvfs_mountfs_fileops_eof(MVFS_FILE* file)
{
//    return ((MIXP_FILE_EOFSTAT(file)) ? 1 : 0);
}
