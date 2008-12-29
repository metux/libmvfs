
#ifndef __MVFS_URL_H
#define __MVFS_URL_H

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
    char* secret;
    char* pathname;
} MVFS_URL;

#define MVFS_ERR_URL_SUCCESS		0
#define MVFS_ERR_URL_INCOMPLETE		-1
#define MVFS_ERR_URL_MISSING_TYPE	-2
#define MVFS_ERR_URL_MISSING_SLASHSLASH	-3

MVFS_URL* mvfs_url_parse(const char* url);

#endif
