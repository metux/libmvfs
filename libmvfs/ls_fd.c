
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <mvfs/mvfs.h>
#include <time.h>

#include <sys/socket.h>

#define MAX(a,b)	((a>b) ? a : b)

// calculate how long an printed integer will be
static int decsize(uint64_t xxx)
{
    long x = xxx;
    int count = 1;
    while (x>9)
    {
	count++;
	x = x/10;
    }
    return count;
}

static int bg_ls_server(MVFS_FILESYSTEM* fs, const char* filename, int fd)
{
    MVFS_FILE* dir = mvfs_fs_openfile(fs, filename, O_RDONLY);
    MVFS_STAT* s; 

    // first scan: count the table row sizes
    int rl_size = 0;
    int rl_name = 0;
    int rl_uid  = 0;
    int rl_gid  = 0;
    int rl_blocks = 0;
    int rl_mtime  = 0;

    while (s = mvfs_file_scan(dir))
    {
	char date_buf[64];
	strftime(date_buf, sizeof(date_buf)-1, "%c", localtime(&s->mtime));
	rl_blocks = MAX(rl_blocks, decsize(s->size/4096));
	rl_size   = MAX(rl_size,   decsize(s->size));
	rl_name   = MAX(rl_name,   strlen(s->name));
	rl_uid    = MAX(rl_uid,    strlen(s->uid));
	rl_gid    = MAX(rl_gid,    strlen(s->gid));
	rl_mtime  = MAX(rl_mtime,  strlen(date_buf));	
	mvfs_stat_free(s);
    }

    char fmtbuf[128];
    sprintf(fmtbuf, "%%s %%-%dd %%-%ds %%-%ds %%-%dd %%-%ds %%-%ds\n", 
	    rl_blocks, 
	    rl_uid,
	    rl_gid,
	    rl_size,
	    rl_mtime,
	    rl_name);

    mvfs_file_reset(dir);
    while (s = mvfs_file_scan(dir))
    {
	char mode_buf[64];
	char date_buf[64];
	char writebuf[4096];
	strftime(date_buf, sizeof(date_buf)-1, "%c", localtime(&s->mtime));
	mvfs_strmode(s->mode,mode_buf);	
	sprintf(writebuf, fmtbuf, mode_buf, s->size/4096, s->uid, s->gid, s->size, date_buf, s->name);
	write(fd,writebuf,strlen(writebuf));
	mvfs_stat_free(s);
    }
}

// open fd for sequential reading from an MVFS_FILE
int mvfs_fs_ls_fd(MVFS_FILESYSTEM* fs, const char* name)
{
    int sockets[2] = { -1, -1 };

    socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);

    int pid;    
    if ((pid=fork())==-1)
    {
	perror("fork failed: ");
	return -1;
    }
    
    if (pid)
    {
	close(sockets[1]);
	return sockets[0];
    }
    else
    {
	close(sockets[0]);
	bg_ls_server(fs, name, sockets[1]);
	exit(0);
    }
}
