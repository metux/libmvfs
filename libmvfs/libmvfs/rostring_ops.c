
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#include <mvfs/mvfs.h>
#include <mvfs/rostring_file.h>
#include <mvfs/_utils.h>

int mvfs_rostring_ops_reopen(MVFS_FILE* f, mode_t mode)
{
    // FIXME: we currently are only working RO on plain files.
    return 0;
}

ssize_t mvfs_rostring_ops_read(MVFS_FILE* f, void* buf, size_t size)
{
    return mvfs_rostring_ops_pread(f, buf, size, 0);
}

ssize_t mvfs_rostring_ops_pread(MVFS_FILE* f, void* buf, size_t size, int64_t offset)
{
    if (offset>=(f->priv.id))
    {
	DEBUGMSG("attempt to ready beyond end of file");
	return 0;
    }
    else if ((offset+size)>(f->priv.id))
    {
	size = (f->priv.id) - offset;
	DEBUGMSG("reducing size to %ld @ offset %ld size %d", size, offset, f->priv.id);
    }
    
    memcpy(buf,(f->priv.name)+offset, size);
    return size;
}

MVFS_FILE* mvfs_rostring_ops_create(const char* str)
{
    MVFS_FILE* f = mvfs_file_alloc(NULL,mvfs_rostring_ops);
    f->priv.name = ((str==NULL) ? "" : str);
    f->priv.id   = strlen(f->priv.name);
    return f;
}

MVFS_FILE* mvfs_rostring_create_args(MVFS_ARGS* args)
{
    return mvfs_rostring_ops_create(mvfs_args_get(args,"value"));    
}

int64_t mvfs_rostring_ops_size(MVFS_FILE* f)
{
    return f->priv.id;
}
