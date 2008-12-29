
#ifndef __MVFS_INTERNAL_UTILS_H
#define __MVFS_INTERNAL_UTILS_H

#include <stdio.h>

/* internal utils ... DO NOT use in clients */
int __mvfs_sock_get_line (FILE* logfile, int sock, char *buf, int buf_len, char term);
int mvfs_decode_filetype (char t);

#endif
