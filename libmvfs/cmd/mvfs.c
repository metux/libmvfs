
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#define _GNU_SOURCE
#include <getopt.h>

#include <mvfs/mvfs.h>
#include <mvfs/hostfs.h>
#include <mvfs/mixpfs.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

// #include <mvfs/mountfs.h>

int catfile(MVFS_FILE* file)
{
    if (file==NULL)
    {
	fprintf(stderr,"file is NULL\n");
	return -1;
    }
    
    char buffer[1024];
    buffer[sizeof(buffer)-1] = 0;

    while (mvfs_file_eof(file)==0)
    {
	int ret = mvfs_file_read(file, buffer, sizeof(buffer)-1);
	if (ret<0)
	{
	    fprintf(stderr,"UGH! error: %d\n", file->errcode);
	    return 0;	    
	}
	printf("%s", buffer);
    }
    printf("\n");
    return 0;
}

int run_cat(MVFS_FILESYSTEM* fs, const char* filename)
{
    MVFS_FILE* file = mvfs_fs_openfile(fs, filename, O_RDONLY);
    if (file==NULL)
    {
	fprintf(stderr,"Cannot open file: \"%s\"\n", filename);
	return -1;
    }
    catfile(file);
    mvfs_file_close(file);
}

/*
int run_ls(MVFS_FILESYSTEM* fs, const char* filename)
{
    MVFS_DIR* dir = mvfs_fs_opendir(fs, filename);
    while (mvfs_dir_scan(dir)==1)
    {
	printf("GOT: \"%s\" (%d)\n", dir->current.name, dir->current.type);
    }
}
*/

#define MAX(a,b)	((a>b) ? a : b)

// calculate how long an printed integer will be
int decsize(uint64_t x)
{
    int count = 1;
    while (x>9)
    {
	count++;
	x = x/10;
    }
    return count;
}

int run_ls2(MVFS_FILESYSTEM* fs, const char* filename)
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
	strftime(date_buf, sizeof(date_buf)-1, "%c", localtime(&s->mtime));
	mvfs_strmode(s->mode,mode_buf);	
	printf(fmtbuf, mode_buf, s->size/4096, s->uid, s->gid, s->size, date_buf, s->name);
	mvfs_stat_free(s);
    }
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
	if (strcmp(argv[optind],"cat")==0)
	{
	    optind++;
	    if (optind < argc)
	    {
		run_cat(fs, argv[optind]);
	    }
	    else
	    {
		fprintf(stderr,"%s cat <filename>\n", argv[0]);
		return 1;
	    }
	}
	else if (strcmp(argv[optind],"read")==0)
	{
	    optind++;
	    if (optind < argc)
	    {
		run_cat(fs, argv[optind]);
	    }
	    else
	    {
		fprintf(stderr,"%s read <filename>\n", argv[0]);
		return 1;
	    }
	}
	else if (strcmp(argv[optind],"ls")==0)
	{
	    optind++;
	    if (optind < argc)
	    {
		run_ls2(fs, argv[optind]);
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


//    MVFS_ARGS* args = mvfs_args_from_url("ninep://localhost:9999/");
//    MVFS_FILESYSTEM* mixerfs = mvfs_mixpfs_create_args(args);

/*
    if (argc<x+2)
    {
	printf("%s: [cat|write|ls] <filename>\n", argv[0]);
	return -1;
    }
    
    if ((strcmp(argv[x],"cat")==0)||(strcmp(argv[x],"read")==0))
    {
	run_cat(mixerfs, argv[2]);
	return 0;
    }
    else if (strcmp(argv[x],"write")==0)
    {
	fprintf(stderr,"write not implemented yet\n");
	return -1;
    }
    else if (strcmp(argv[1],"set")==0)
    {
	if (argc<4)
	{
	    fprintf(stderr,"store needs four parameters\n");
	    return -1;
	}
	run_store(mixerfs, argv[2], argv[3]);
	return 1;
    }
    else if (strcmp(argv[1],"ls")==0)
    {
	run_ls2(mixerfs,argv[2]);
	return 1;
    }
    else
	fprintf(stderr,"unknown command \"%s\"\n", argv[1]);
*/
}
