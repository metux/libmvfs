
#ifndef __MVFS_INTERNAL_UTILS_H
#define __MVFS_INTERNAL_UTILS_H

#include <stdio.h>

/* internal utils ... DO NOT use in clients */
int __mvfs_sock_get_line (FILE* logfile, int sock, char *buf, int buf_len, char term);

// maximum URL size
#define MVFS_URL_MAX	2048

typedef struct __mvfs_url
{
    char url[MVFS_URL_MAX];
    char buffer[MVFS_URL_MAX];
    int  error;

    // the following ptrs point somewhere into the buffer above. you don't need any free'ing here
    char* hostname;
    char* port;
    char* type;
    char* username;
    char* password;
    char* pathname;
} MVFS_URL;

#define MVFS_ERR_URL_SUCCESS		0
#define MVFS_ERR_URL_INCOMPLETE		-1
#define MVFS_ERR_URL_MISSING_TYPE	-2
#define MVFS_ERR_URL_MISSING_SLASHSLASH	-3

MVFS_URL __mvfs_parse_url(const char* url);

#endif
