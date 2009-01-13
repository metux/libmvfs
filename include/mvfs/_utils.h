/*
    libmvfs - metux Virtual Filesystem Library

    Internal helpers - don't use outside of libmvfs

    Copyright (C) 2008 Enrico Weigelt, metux IT service <weigelt@metux.de>
    This code is published under the terms of the GNU Public License 2.0
*/

#ifndef __MVFS_INTERNAL_UTILS_H
#define __MVFS_INTERNAL_UTILS_H

#include <stdio.h>

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

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif
