/*

    mVFS API
    
*/

#ifndef __LIBMVFS_TYPES_H
#define __LIBMVFS_TYPES_H

// FIXME !
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <sys/types.h>
#include <inttypes.h>
#include <hash.h>
#include <mvfs/stat.h>

typedef int64_t	off64_t;

typedef struct __mvfs_file	MVFS_FILE;
typedef struct __mvfs_fs	MVFS_FILESYSTEM;
typedef struct __mvfs_file_ops  MVFS_FILE_OPS;
typedef struct __mvfs_fs_ops	MVFS_FILESYSTEM_OPS;
typedef struct __mvfs_dir	MVFS_DIR;
typedef struct __mvfs_dir_ops	MVFS_DIR_OPS;
typedef struct __mvfs_symlink   MVFS_SYMLINK;

typedef enum
{
    NONBLOCK	  = 1,
    READ_TIMEOUT  = 2,
    WRITE_TIMEOUT = 3,
    READ_AHEAD    = 4,
    WRITE_ASYNC   = 5
} MVFS_FILE_FLAG;

struct __mvfs_symlink
{
    int   errcode;
    char  target[1020];
};

struct __mvfs_file_ops
{
    int          (*reopen)   (MVFS_FILE* fp, mode_t mode);			// reopen the file with another mode
    off64_t      (*seek)     (MVFS_FILE* fp, off64_t offset, int whence);		// set file position
    ssize_t      (*read)     (MVFS_FILE* fp, void* buf, size_t count);		// read a chunk
    ssize_t      (*write)    (MVFS_FILE* fp, const void*  buf, size_t count);	// write a chunk
    ssize_t      (*pread)    (MVFS_FILE* fp, void* buf, size_t count, off64_t offset);		// read a chunk @ offset
    ssize_t      (*pwrite)   (MVFS_FILE* fp, const void*  buf, size_t count, off64_t offset);	// write a chunk @ offset
    int          (*setflag)  (MVFS_FILE* fp, MVFS_FILE_FLAG flag, long value);	// set an file flag
    int          (*getflag)  (MVFS_FILE* fp, MVFS_FILE_FLAG flag, long* value);	// retrieve an file flag
    int          (*close)    (MVFS_FILE* fp);					// close file
    int          (*eof)      (MVFS_FILE* fp);
    MVFS_STAT*   (*stat)     (MVFS_FILE* fp);					
    int          (*free)     (MVFS_FILE* fp);					// free private data (NOT the MVFS_FILE struct !)
    
    // dir operations
    MVFS_FILE*   (*lookup)   (MVFS_FILE* fp, const char* name);			// open an specific direntry
    MVFS_STAT*   (*scan)     (MVFS_FILE* fp);					// scan for next dir entry, returned stat MAY be incomplete
    int          (*reset)    (MVFS_FILE* fp);					// reset dir scanning
};

struct __mvfs_file
{
    int 		refcount;
    int  		errcode;	// error code of last operation
    MVFS_FILE_OPS	ops;		// file operations
    MVFS_FILESYSTEM*	fs;

    struct
    {
	void* 		ptr;
	int     	id;
	const char* 	name;
	char* 		buffer;
	int             status;
	off64_t		pos;
    } priv;
};

struct __mvfs_fs_ops
{
    MVFS_FILE*   (*openfile) (MVFS_FILESYSTEM* fs, const char* name, mode_t mode);
    MVFS_STAT*   (*stat)     (MVFS_FILESYSTEM* fs, const char* name);
    int          (*unlink)   (MVFS_FILESYSTEM* fs, const char* name);
    int          (*free)     (MVFS_FILESYSTEM* fs);
    MVFS_SYMLINK (*readlink) (MVFS_FILESYSTEM* fs, const char* path);
    int          (*symlink)  (MVFS_FILESYSTEM* fs, const char* n1, const char* n2);
    int          (*rename)   (MVFS_FILESYSTEM* fs, const char* n1, const char* n2);
    int          (*chmod)    (MVFS_FILESYSTEM* fs, const char* filename, mode_t mode);
    int          (*chown)    (MVFS_FILESYSTEM* fs, const char* filename, const char* uid, const char* gid);
    int          (*mkdir)    (MVFS_FILESYSTEM* fs, const char* filename, mode_t mode);
};

struct __mvfs_fs
{
    MVFS_FILESYSTEM_OPS	ops;
    int                 errcode;
    int			refcount;
    char*		magic;
    struct
    {
	void*	ptr;
    } priv;
};

#endif
