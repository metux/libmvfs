#include <stdio.h>

int __mvfs_sock_get_line (FILE* logfile, int sock, char *buf, int buf_len, char term)
{
    int i, status;
    char c;

//    fprintf(stderr,"__mvfs_sock_get_line: reading from socket %d (%d bytes) - term \"%c\"\n", sock, buf_len, term);

    for (i = 0; i < buf_len - 1; i++, buf++){
	if (read (sock, buf, sizeof(char)) <= 0)
	    return 0;
	if (logfile){
	    fwrite (buf, 1, 1, logfile);
	    fflush (logfile);
	}
	if (*buf == term){
	    *buf = 0;
	    return 1;
	}
    }

    /* Line is too long - terminate buffer and discard the rest of line */
    *buf = 0;
    while ((status = read (sock, &c, sizeof (c))) > 0){
	if (logfile){
	    fwrite (&c, 1, 1, logfile);
	    fflush (logfile);
	}
	if (c == '\n')
	    return 1;
    }
    return 0;
}
