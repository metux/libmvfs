
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <getopt.h>

#include <mvfs/mvfs.h>
#include <stdio.h>

static int _readfd(int fd)
{
    char buffer[1024];
    buffer[sizeof(buffer)-1] = 0;
    int ret;
    while ((ret = read(fd, buffer, sizeof(buffer)-2))>0)
    {
	buffer[ret]=0;
	write(0, buffer, ret);
    }
    return 0;
}

int run_cat(MVFS_FILESYSTEM* fs, const char* filename)
{
    return _readfd(mvfs_fs_read_fd(fs, filename));
}

int run_ls(MVFS_FILESYSTEM* fs, const char* filename)
{
    return _readfd(mvfs_fs_ls_fd(fs, filename));
}

int run_store(MVFS_FILESYSTEM* fs, const char* filename, const char* text)
{
    MVFS_FILE* file = mvfs_fs_openfile(fs, filename, O_WRONLY);
    mvfs_file_write(file, text, strlen(text));
}

int verbose_flag = 0;

int main(int argc, char* argv[])
{
    const char* server_url = NULL;

    int c;
    int digit_optind;
    while (1)
    {
	int this_optind = optind ? optind : 1;
	int option_index = 0;
	static struct option long_options[] = 
	{
	    { "server",  required_argument, NULL, 's' },
	    { "verbose", no_argument,       NULL, 'v' },
	    { 0,        0, 0, 0 }
	};
	
	c=getopt_long(argc, argv, "vs:", long_options, &option_index);
	if (c==-1)
	    break;
	    
	switch (c)
	{
	    case 's':
		server_url = optarg;
	    break;
	    default:
		printf("unknown option %c\n", c);
	    break;
	}
    }
    
    // try to connect the server
    if (server_url == NULL)
    {
	server_url = "ninep://localhost:9999/";
	fprintf(stderr,"WARN: no server url specified. defaulting to %s\n",server_url);
    }
        
    MVFS_ARGS* args = mvfs_args_from_url(server_url);
    MVFS_FILESYSTEM* fs = mvfs_fs_create_args(args); 	// FIXME !!!
    
    if (fs==NULL)
    {
	fprintf(stderr,"Could not connect to filesystem \"%s\"\n", server_url);
	return 1;
    }

    if (optind < argc)
    {
	if ((strcmp(argv[optind],"cat")==0)||(strcmp(argv[optind],"read")==0))
	{
	    optind++;
	    if (optind < argc)
	    {
		run_cat(fs, argv[optind]);
	    }
	    else
	    {
		fprintf(stderr,"%s %s <filename>\n", argv[0], argv[optind]);
		return 1;
	    }
	}
	else if (strcmp(argv[optind],"ls")==0)
	{
	    optind++;
	    if (optind < argc)
	    {
		run_ls(fs, argv[optind]);
	    }
	    else
	    {
		fprintf(stderr,"%s ls <filename>\n", argv[0]);
		return 1;
	    }
	}
	else
	{
	    fprintf(stderr,"unknown command: %s\n", argv[optind]);
	}
    }
}
