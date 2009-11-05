
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <mvfs/mvfs.h>
#include <stdio.h>
#include <sys/socket.h>

static void bg_catfile_server(MVFS_FILESYSTEM* fs, const char* name, int srvfd)
{
    MVFS_FILE* file = mvfs_fs_openfile(fs, name, O_RDONLY);
    if (file==NULL)
    {
	fprintf(stderr,"file is NULL\n");
	return;
    }

    char buffer[1024];
    buffer[sizeof(buffer)-1] = 0;

    while (mvfs_file_eof(file)==0)
    {
	memset(buffer,0,sizeof(buffer));
	int ret = mvfs_file_read(file, buffer, sizeof(buffer)-1);
	if (ret<0)
	{
	    fprintf(stderr,"UGH! error: %d\n", file->errcode);
	    return;
	}
	write(srvfd, buffer, ret);
    }
    close(srvfd);
    exit(0);
}

// open fd for sequential reading from an MVFS_FILE
int mvfs_fs_read_fd(MVFS_FILESYSTEM* fs, const char* name)
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
	bg_catfile_server(fs, name, sockets[1]);
	exit(0);
    }
}
