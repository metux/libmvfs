
#ifndef __MVFS_INTERNAL_UTILS_H
#define __MVFS_INTERNAL_UTILS_H

#include <stdio.h>

/* internal utils ... DO NOT use in clients */
int __mvfs_sock_get_line (FILE* logfile, int sock, char *buf, int buf_len, char term);
int mvfs_decode_filetype (char t);

#ifndef ERROR_CHANNEL	
#define ERROR_CHANNEL	stderr
#endif

#ifndef DEBUG_CHANNEL
#define DEBUG_CHANNEL	stderr
#endif

#define ERRMSG(text...)				\
    {						\
	fprintf(ERROR_CHANNEL,"[ERR] ");		\
	fprintf(ERROR_CHANNEL, __FUNCTION__);		\
	fprintf(ERROR_CHANNEL,"() ");			\
	fprintf(ERROR_CHANNEL,##text);			\
	fprintf(ERROR_CHANNEL,"\n");			\
    }

#ifdef __DEBUG
#define DEBUGMSG(text...)			\
    {						\
	fprintf(DEBUG_CHANNEL,"[DBG] ");		\
	fprintf(DEBUG_CHANNEL, __FUNCTION__);		\
	fprintf(DEBUG_CHANNEL,"() ");			\
	fprintf(DEBUG_CHANNEL,##text);			\
	fprintf(DEBUG_CHANNEL,"\n");			\
    }
#else
#define DEBUGMSG(text...)
#endif

#endif
