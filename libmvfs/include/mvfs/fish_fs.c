
#define ERRMSG(text...)	\
    {						\
	fprintf(stderr,"[ERR] ");		\
	fprintf(stderr, __FUNCTION__);		\
	fprintf(stderr,"() ");			\
	fprintf(stderr,##text);			\
	fprintf(stderr,"\n");			\
    }

#ifdef __DEBUG
#define DEBUGMSG(text...)	\
    {						\
	fprintf(stderr,"[DBG] ");		\
	fprintf(stderr, __FUNCTION__);		\
	fprintf(stderr,"() ");			\
	fprintf(stderr,##text);			\
	fprintf(stderr,"\n");			\
    }

#define print_vfs_message(text...)	\
    {						\
	fprintf(stderr,"[VFS] ");		\
	fprintf(stderr, __FUNCTION__);		\
	fprintf(stderr,"() ");			\
	fprintf(stderr,##text);			\
	fprintf(stderr,"\n");			\
    }
#else
#define DEBUGMSG(text...)
#define print_vfs_message(text...)
#endif

