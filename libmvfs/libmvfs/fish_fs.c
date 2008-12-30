/* Virtual File System: FISH implementation for transfering files over
   shell connections.

   Copyright (C) 1998 The Free Software Foundation
   
   Written by: 1998 Pavel Machek
   Spaces fix: 2000 Michal Svec

   Derived from ftpfs.c.
   
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License
   as published by the Free Software Foundation; either version 2 of
   the License, or (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

/*
 * Read README.fish for protocol specification.
 *
 * Syntax of path is: /#sh:user@host[:Cr]/path
 *	where C means you want compressed connection,
 *	and r means you want to use rsh
 *
 * Namespace: fish_vfs_ops exported.
 */

/* Define this if your ssh can take -I option */

//
//	FD private data layout
//
//	id	fd
//	name	filename
//	ptr	DIR* pointer

#define __DEBUG

#define PRIV_FD(file)			(file->priv.id)
#define PRIV_NAME(file)			(file->priv.name)
#define PRIV_DIRP(file)			((DIR*)(file->priv.ptr))

#define PRIV_SET_FD(file,fd)	 	file->priv.id = fd;
#define PRIV_SET_NAME(file,name)	file->priv.name = strdup(name)
#define PRIV_SET_DIRP(file,dirp)	file->priv.ptr = dirp;

#include <mvfs/mvfs.h>
#include <mvfs/default_ops.h>
#include <mvfs/hostfs.h>

#include <sys/time.h>
#include <time.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>
#include <stdarg.h>

#define FS_MAGIC 	"fishfs"

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

#define _(a)	a

typedef struct fish_connection
{
    const char* hostname;
    const char* path;
    const char* username;
    const char* secret;
    const char* url;
    const char* port;
    const char* cwdir;
    int   sockr;
    int   sockw;
    int   error;
    FILE* logfile;
} FISH_CONNECTION;

#define FISH_MAX_STAT	128
typedef struct fish_file
{
    MVFS_STAT*  stats[FISH_MAX_STAT];
    int         scanpos;
    int         statcount;
    const char* name;
    long        size;
    short       got_local_copy;
    long	readpos;
    char*       tmpname;
    int         tmp_fd;
    short 	eof;
} FISH_FILE;

#define CLEARERR()	conn->error = 0;

#undef HAVE_HACKED_SSH

/* FIXME: this was used in mcvfs, don't know how to handle it properly yet */
static inline void enable_interrupt_key() { } 
static inline void disable_interrupt_key() { }

/*
#include "utilvfs.h"

#include "xdirentry.h"
#include "vfs.h"
#include "vfs-impl.h"
#include "gc.h"		/* vfs_stamp_create */
/* #include "tcputil.h" */

#define FISH_DIRECTORY_TIMEOUT 30 * 60

#define DO_RESOLVE_SYMLINK 1
#define DO_OPEN            2
#define DO_FREE_RESOURCE   4

#define FISH_FLAG_COMPRESSED 1
#define FISH_FLAG_RSH	     2

#define OPT_FLUSH        1
#define OPT_IGNORE_ERROR 2

/*
 * Reply codes.
 */
#define PRELIM		1	/* positive preliminary */
#define COMPLETE	2	/* positive completion */
#define CONTINUE	3	/* positive intermediate */
#define TRANSIENT	4	/* transient negative completion */
#define ERROR		5	/* permanent negative completion */

/* command wait_flag: */
#define NONE        0x00
#define WAIT_REPLY  0x01
#define WANT_STRING 0x02
static char reply_str [80];

static int
 fish_command (FISH_CONNECTION* conn, void* foo,
	       int wait_reply, const char *fmt, ...)
    __attribute__ ((format (printf, 4, 5)));

static int fish_decode_reply (char *s, int was_garbage)
{
    int code;
    if (!sscanf(s, "%d", &code)) {
	code = 500;
	return 5;
    }
    if (code<100) return was_garbage ? ERROR : (!code ? COMPLETE : PRELIM);
    return code / 100;
}

/* Returns a reply code, check /usr/include/arpa/ftp.h for possible values */
static int fish_get_reply (FISH_CONNECTION* conn, char *string_buf, int string_len)
{
    char answer[1024];
    int was_garbage = 0;

    for (;;) 
    {
        if (!__mvfs_sock_get_line(
		stderr, // logfile, 
		conn->sockr, 
		answer, 
		sizeof(answer), 
		'\n'
	    )) 
	{
		if (string_buf)
		    *string_buf = 0;
		return 4;
	}

	if (strncmp(answer, "### ", 4)) 
	{
		was_garbage = 1;
		if (string_buf)
		    strncpy(string_buf, answer, string_len);
	} 
	else 
		return fish_decode_reply(answer+4, was_garbage);
    }
}

#define SUP super->u.fish

static int
fish_command (FISH_CONNECTION* conn, void* foo,
	      int wait_reply, const char *fmt, ...)
{
    va_list ap;
    char str[65565]; 	// perhaps a bit too big ;-o
    int status;
    FILE* logfile = conn->logfile;
    
    va_start (ap, fmt);
    vsnprintf(str, sizeof(str)-1, fmt, ap);
    va_end (ap);

    if (conn->logfile) {
	fwrite (str, strlen (str), 1, conn->logfile);
	fflush (conn->logfile);
    }

    enable_interrupt_key ();

    status = write (conn->sockw, str, strlen (str));

    disable_interrupt_key ();
    if (status < 0)
	return TRANSIENT;

    if (wait_reply)
	return fish_get_reply (conn, (wait_reply & WANT_STRING) ? reply_str :
			       NULL, sizeof (reply_str) - 1);
    return COMPLETE;
}

static void
fish_free_archive (FISH_CONNECTION* conn)
{
    if ((conn->sockw != -1) || (conn->sockr != -1)) {
	print_vfs_message (_("fish: Disconnecting from %s"),
			   conn->hostname ? conn->hostname : "???");
	fish_command (conn, NULL, NONE, "#BYE\nexit\n");
	DEBUGMSG("closing socket %d / %d", conn->sockw, conn->sockr);
	close (conn->sockw);
	close (conn->sockr);
	conn->sockw = conn->sockr = -1;
    }
//    if (conn->hostname) { free(conn->hostname); conn->hostname = NULL; }
//    if (conn->username) { free(conn->username); conn->username = NULL; }
//    if (conn->path)     { free(conn->path);     conn->path     = NULL; }

    conn->hostname = NULL;
    conn->username = NULL;
    conn->path     = NULL;
}

static void
fish_pipeopen(FISH_CONNECTION* conn, const char *path, const char *argv[])
{
    int fileset1[2], fileset2[2];
    int res;

    if ((pipe(fileset1)<0) || (pipe(fileset2)<0)) 
    {
	ERRMSG("fishfs: Cannot pipe()");
	return;
    }

    if ((res = fork())) {
        if (res<0) 
	{
	    ERRMSG("fishfs: Cannot fork(): %m.");
	    return;
	}
	// We are the parent
	close(fileset1[0]);
	conn->sockw = fileset1[1];
	close(fileset2[1]);
	conn->sockr = fileset2[0];
    } else {
        close(0);
	dup(fileset1[0]);
	close(fileset1[0]); close(fileset1[1]);
	close(1); close(2);
	dup(fileset2[1]);
	// stderr to /dev/null
	open ("/dev/null", O_WRONLY);
	close(fileset2[0]); close(fileset2[1]);
	execvp(path, (char **)argv);
	_exit(3);
    }
}

/* The returned directory should always contain a trailing slash */
static inline char *fish_getcwd(FISH_CONNECTION* conn)
{
    if (fish_command (conn, NULL, WANT_STRING, "#PWD\npwd; echo '### 200'\n") == COMPLETE)
    {
	strcat(reply_str,"/");
	return reply_str;
    }
    conn->error = EIO;
    return NULL;
}


static int
fish_connect(FISH_CONNECTION* conn)
{
    {
	const char *argv[10];
//	const char *xsh = (conn->flags == FISH_FLAG_RSH ? "rsh" : "ssh");
	const char* xsh = "ssh";
	int i = 0;

	argv[i++] = xsh;
#ifdef HAVE_HACKED_SSH
	argv[i++] = "-I";
#endif
//	if (conn->flags == FISH_FLAG_COMPRESSED)
//	    argv[i++] = "-C";
	argv[i++] = "-l";
	argv[i++] = conn->username;
	argv[i++] = conn->hostname;
	argv[i++] = "echo FISH:; /bin/sh";
	argv[i++] = NULL;

	fish_pipeopen (conn, xsh, argv);
    }
    {
	char answer[2048];
	print_vfs_message (_("fish: Waiting for initial line..."));
	if (!__mvfs_sock_get_line (conn->logfile, conn->sockr, answer, sizeof (answer), ':'))
	{
	    ERRMSG("Fish: protocol error (1)\n");
	    return -1;
	}

	print_vfs_message ("%s", answer);
	if (strstr (answer, "assword")) {

	    // Currently, this does not work. ssh reads passwords from
	    //   /dev/tty, not from stdin :-(. 

#ifndef HAVE_HACKED_SSH
//	    message (1, MSG_ERROR,
//		     _
//		     ("Sorry, we cannot do password authenticated connections for now."));
	    ERRMSG("Sorry, we cant to password authentication for now :(\n");
	    return -EPERM;
#endif
	    if (!conn->secret) {
		ERRMSG("Password required\n");
		return -EPERM;
	    }
	    print_vfs_message (_("fish: Sending password..."));
	    write (conn->sockw, conn->secret, strlen (conn->secret));
	    write (conn->sockw, "\n", 1);
	}
    }

    print_vfs_message (_("fish: Sending initial line..."));
    //
    // Run `start_fish_server'. If it doesn't exist - no problem,
    // we'll talk directly to the shell.
    //
    if (fish_command
	(conn, NULL, WAIT_REPLY,
	 "#FISH\necho; start_fish_server 2>&1; echo '### 200'\n") !=
	COMPLETE)
    {
	ERRMSG("protocol error - required 200\n");
	return -EPROTO;
    }
    
    DEBUGMSG("fish: Handshaking version...");
    if (fish_command
	(conn, NULL, WAIT_REPLY,
	 "#VER 0.0.0\necho '### 000'\n") != COMPLETE)
    {
	ERRMSG("protocol error - required version string\n");
	return -EPROTO;
    }

    // Set up remote locale to C, otherwise dates cannot be recognized 
    if (fish_command
	(conn, NULL, WAIT_REPLY,
	 "LANG=C; LC_ALL=C; LC_TIME=C\n"
	 "export LANG; export LC_ALL; export LC_TIME\n" "echo '### 200'\n")
	!= COMPLETE)
    {
	ERRMSG("protocol error - cannot set locale\n");
	return -EPROTO;
    }

    print_vfs_message (_("fish: Setting up current directory..."));
    conn->cwdir = strdup(fish_getcwd (conn));
    print_vfs_message (_("fish: Connected, home %s."), conn->cwdir);

    ERRMSG("fish connected\n");
    return 0;
}

/*
static int
fish_open_archive (struct vfs_class *me, struct vfs_s_super *super,
		   const char *archive_name, char *op)
{
    char *host, *user, *password, *p;
    int flags;

    p = vfs_split_url (strchr (op, ':') + 1, &host, &user, &flags,
		       &password, 0, URL_NOSLASH);

    g_free (p);

    conn->host = host;
    conn->user = user;
    conn->flags = flags;
    if (!strncmp (op, "rsh:", 4))
	conn->flags |= FISH_FLAG_RSH;
    conn->cwdir = NULL;
    if (password)
	conn->password = password;
    return fish_open_archive_int (me, super);
}
*/
/*
static int
fish_archive_same (struct vfs_class *me, struct vfs_s_super *super,
		   const char *archive_name, char *op, void *cookie)
{
    char *host, *user;
    int flags;

    op = vfs_split_url (strchr (op, ':') + 1, &host, &user, &flags, 0, 0,
			URL_NOSLASH);

    g_free (op);

    flags = ((strcmp (host, conn->host) == 0)
	     && (strcmp (user, conn->user) == 0) && (flags == conn->flags));
    g_free (host);
    g_free (user);

    return flags;
}
*/
/*
static int
fish_dir_uptodate(struct vfs_class *me, struct vfs_s_inode *ino)
{
    struct timeval tim;

    gettimeofday(&tim, NULL);
    if (MEDATA->flush) {
	MEDATA->flush = 0;
	return 0;
    }
    if (tim.tv_sec < ino->timestamp.tv_sec)
	return 1;
    return 0;
}
*/

/*
static int
fish_file_store(struct vfs_class *me, struct vfs_s_fh *fh, char *name, char *localname)
{
    struct vfs_s_super *super = FH_SUPER;
    int n, total;
    char buffer[8192];
    struct stat s;
    int was_error = 0;
    int h;
    char *quoted_name;

    h = open (localname, O_RDONLY);

    if (h == -1)
	ERRNOR (EIO, -1);
    if (fstat(h, &s)<0) {
	close (h);
	ERRNOR (EIO, -1);
    }
    // Use this as stor: ( dd block ; dd smallblock ) | ( cat > file; cat > /dev/null )

    print_vfs_message(_("fish: store %s: sending command..."), name );
    quoted_name = name_quote (name, 0);

    // FIXME: File size is limited to ULONG_MAX
    if (!fh->u.fish.append)
	n = fish_command (conn, NULL, WAIT_REPLY,
		 "#STOR %lu /%s\n"
		 "> /%s\n"
		 "echo '### 001'\n"
		 "(\n"
		   "dd bs=1 count=%lu\n"
		 ") 2>/dev/null | (\n"
		   "cat > /%s\n"
		   "cat > /dev/null\n"
		 "); echo '### 200'\n",
		 (unsigned long) s.st_size, name, quoted_name,
		 (unsigned long) s.st_size, quoted_name);
    else
	n = fish_command (conn, NULL, WAIT_REPLY,
		 "#STOR %lu /%s\n"
		 "echo '### 001'\n"
		 "(\n"
		   "dd bs=1 count=%lu\n"
		 ") 2>/dev/null | (\n"
		   "cat >> /%s\n"
		   "cat > /dev/null\n"
		 "); echo '### 200'\n",
		 (unsigned long) s.st_size, name,
		 (unsigned long) s.st_size, quoted_name);

    g_free (quoted_name);
    if (n != PRELIM) {
	close (h);
        ERRNOR(E_REMOTE, -1);
    }
    total = 0;
    
    while (1) {
	int t;
	while ((n = read(h, buffer, sizeof(buffer))) < 0) {
	    if ((errno == EINTR) && got_interrupt())
	        continue;
	    print_vfs_message(_("fish: Local read failed, sending zeros") );
	    close(h);
	    h = open( "/dev/zero", O_RDONLY );
	}
	if (n == 0)
	    break;
    	if ((t = write (conn->sockw, buffer, n)) != n) {
	    if (t == -1) {
		me->verrno = errno;
	    } else { 
		me->verrno = EIO;
	    }
	    goto error_return;
	}
	disable_interrupt_key();
	total += n;
	print_vfs_message(_("fish: storing %s %d (%lu)"), 
			  was_error ? _("zeros") : _("file"), total,
			  (unsigned long) s.st_size);
    }
    close(h);
    if ((fish_get_reply (conn, NULL, 0) != COMPLETE) || was_error)
        ERRNOR (E_REMOTE, -1);
    return 0;
error_return:
    close(h);
    fish_get_reply(conn, NULL, 0);
    return -1;
}
*/

static int 
fish_get_filesize (FISH_CONNECTION* conn, const char* filename)
{
    int ret = fish_command (conn, NULL, WANT_STRING,
	"ls -l /%s 2>/dev/null | (\n"
		  "read var1 var2 var3 var4 var5 var6\n"
		  "echo \"$var5\"\n"
		")\n"
		"echo '### 100'\n", filename);
    int size; 
    if (sscanf( reply_str, "%d", &(size) )!=1)
    {
	ERRMSG("could not get filesize ...");
	return -EREMOTE;
    }
    return size;
}

static int
fish_copy2local (FISH_CONNECTION* conn, FISH_FILE* file, off_t offset)
{
    if (offset)
	return -ENOTSUP;

    char* name = strdup(file->name);
    DEBUGMSG("Starting linear read transfer for: %s", name);

    offset = fish_command (conn, NULL, WANT_STRING,
		"#RETR /%s\n"
		"ls -l /%s 2>/dev/null | (\n"
		  "read var1 var2 var3 var4 var5 var6\n"
		  "echo \"$var5\"\n"
		")\n"
		"echo '### 100'\n"
		"cat /%s\n"
		"echo '### 200'\n", 
		name, name, name );

    if (offset != PRELIM) 
    {
	ERRMSG("something went wrong ... %d", offset);
	return -EREMOTE;
    }
    if (sscanf( reply_str, "%d", &(file->size) )!=1)
    {
	ERRMSG("could not get filesize ...");
	return -EREMOTE;
    }

    {
	char tmpbuf[1024];
	struct timeval tv;
	gettimeofday(&tv, NULL);
	sprintf(tmpbuf,"/tmp/mvfs-fish-%ld-%ld.tmp", tv.tv_sec, tv.tv_usec);
	file->tmp_fd = open(tmpbuf, O_RDWR|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR);
	file->tmpname = strdup(tmpbuf);
	DEBUGMSG("using tempfile: %s fd %d", tmpbuf, file->tmp_fd);
    }

    long pos = 0;
    for (pos = 0; pos<file->size; )
    {
	long count = ((file->size-pos) > 1024 ? 1024 : (file->size-pos));
	char buffer[1024];
	count = read(conn->sockr, buffer, count);
	write(file->tmp_fd, buffer, count);
	pos+=count;
    }

    fsync(file->tmp_fd);
    lseek(file->tmp_fd, 0, SEEK_SET);

    DEBUGMSG("finished writing file (%d bytes)", pos);
    close(file->tmp_fd);
    if ((file->tmp_fd = open(file->tmpname, O_RDONLY)) == -1)
    {
	ERRMSG("cannot re-open file %s: %s", file->tmpname, strerror(errno));
    }
    else
    {
	DEBUGMSG("successfully reopened %s -> %d", file->tmpname, file->tmp_fd);
    }
    {
	char reply[4096];
	if (fish_get_reply (conn, (char*)&reply, sizeof(reply)) != COMPLETE)
	{
//	    fprintf(stderr,"reply failed: %s\n", reply);
	}
//	else
//	    fprintf(stderr,"reply ok: %s\n", reply);
    }

    return 0;
}

/*
static void
fish_linear_abort (struct vfs_class *me, struct vfs_s_fh *fh)
{
    struct vfs_s_super *super = FH_SUPER;
    char buffer[8192];
    int n;

    print_vfs_message( _("Aborting transfer...") );
    do {
	n = MIN(8192, fh->u.fish.total - fh->u.fish.got);
	if (n) {
	    if ((n = read(conn->sockr, buffer, n)) < 0)
	        return;
	    fh->u.fish.got += n;
	}
    } while (n);

    if (fish_get_reply (conn, NULL, 0) != COMPLETE)
        print_vfs_message( _("Error reported after abort.") );
    else
        print_vfs_message( _("Aborted transfer would be successful.") );
}

static int
fish_ctl (void *fh, int ctlop, void *arg)
{
    return 0;
    switch (ctlop) {
        case VFS_CTL_IS_NOTREADY:
	    {
	        int v;

		if (!FH->linear)
		{
		    ERRMSG("You may not do this");
		    return -1;
		}
		if (FH->linear == LS_LINEAR_CLOSED)
		    return 0;

		v = vfs_s_select_on_two (FH_SUPER->u.fish.sockr, 0);
		if (((v < 0) && (errno == EINTR)) || v == 0)
		    return 1;
		return 0;
	    }
        default:
	    return 0;
    }
}
*/

static int
fish_send_command(FISH_CONNECTION* conn, const char *cmd, int flags)
{
    int r;

    r = fish_command (conn, NULL, WAIT_REPLY, "%s", cmd);
    if (r != COMPLETE) 
    {
	conn->error = EREMOTEIO;
	ERRMSG("remote command failed: \"%s\"", cmd);
	return -1;
    }

//    if (flags & OPT_FLUSH)
//	vfs_s_invalidate(me, super);
    return 0;
}

#define BUF_LARGE	4096

#define PREFIX \
    char buf[BUF_LARGE]; \
    char *rpath = strdup (path); \

#define POSTFIX(flags) \
    free (rpath); \
    return fish_send_command(conn, buf, flags);

static int
fish_chmod (FISH_CONNECTION* conn, const char *path, int mode)
{
    PREFIX
    snprintf(buf, sizeof(buf), "#CHMOD %4.4o /%s\n"
			       "chmod %4.4o \"/%s\" 2>/dev/null\n"
			       "echo '### 000'\n", 
	    mode & 07777, rpath,
	    mode & 07777, rpath);
    POSTFIX(OPT_FLUSH);
}

/*
#define FISH_OP(name, chk, string) \
static int fish_##name (struct vfs_class *me, const char *path1, const char *path2) \
{ \
    char buf[BUF_LARGE]; \
    char *rpath1, *rpath2, *mpath1, *mpath2; \
    struct vfs_s_super *super1, *super2; \
    if (!(rpath1 = vfs_s_get_path_mangle (me, mpath1 = g_strdup(path1), &super1, 0))) { \
	g_free (mpath1); \
	return -1; \
    } \
    if (!(rpath2 = vfs_s_get_path_mangle (me, mpath2 = g_strdup(path2), &super2, 0))) { \
	g_free (mpath1); \
	g_free (mpath2); \
	return -1; \
    } \
    rpath1 = name_quote (rpath1, 0); \
    g_free (mpath1); \
    rpath2 = name_quote (rpath2, 0); \
    g_free (mpath2); \
    g_snprintf(buf, sizeof(buf), string "\n", rpath1, rpath2, rpath1, rpath2); \
    g_free (rpath1); \
    g_free (rpath2); \
    return fish_send_command(conn, buf, OPT_FLUSH); \
}

#define XTEST if (bucket1 != bucket2) { ERRNOR (EXDEV, -1); }
FISH_OP(rename, XTEST, "#RENAME /%s /%s\n"
		       "mv /%s /%s 2>/dev/null\n"
		       "echo '### 000'" )
FISH_OP(link,   XTEST, "#LINK /%s /%s\n"
		       "ln /%s /%s 2>/dev/null\n"
		       "echo '### 000'" )
*/

static int fish_symlink (FISH_CONNECTION* conn, const char *setto, const char *path)
{
    char *qsetto;
    PREFIX
//    qsetto = name_quote (setto, 0);
    qsetto = strdup(setto);
    snprintf(buf, sizeof(buf),
            "#SYMLINK %s /%s\n"
	    "ln -s %s /%s 2>/dev/null\n"
	    "echo '### 000'\n",
	    qsetto, rpath, qsetto, rpath);
    free (qsetto);
    POSTFIX(OPT_FLUSH);
}

static int
fish_chown (FISH_CONNECTION* conn, const char *path, const char* owner, const char* group)
{
    PREFIX
    snprintf (buf, sizeof(buf),
    	"#CHOWN /%s /%s\n"
	"chown %s /%s 2>/dev/null\n"
	"echo '### 000'\n", 
	owner, rpath,
	owner, rpath);
    fish_send_command (conn, buf, OPT_FLUSH); 
    // FIXME: what should we report if chgrp succeeds but chown fails?
    snprintf (buf, sizeof(buf),
        "#CHGRP /%s /%s\n"
	"chgrp %s /%s 2>/dev/null\n"
	"echo '### 000'\n", 
	group, rpath,
	group, rpath);
    // fish_send_command(conn, buf, OPT_FLUSH); 
    POSTFIX (OPT_FLUSH)
}

static int fish_unlink (FISH_CONNECTION* conn, const char *path)
{
    PREFIX
    snprintf(buf, sizeof(buf),
            "#DELE /%s\n"
	    "rm -f /%s 2>/dev/null\n"
	    "echo '### 000'\n",
	    rpath, rpath);
    POSTFIX(OPT_FLUSH);
}

static int fish_mkdir (FISH_CONNECTION* conn, const char *path, mode_t mode)
{
    PREFIX
    snprintf(buf, sizeof(buf),
            "#MKD /%s\n"
	    "mkdir /%s 2>/dev/null\n"
	    "echo '### 000'\n",
	    rpath, rpath);
    POSTFIX(OPT_FLUSH);
}

static int fish_rmdir (FISH_CONNECTION* conn, const char *path)
{
    PREFIX
    snprintf(buf, sizeof(buf),
            "#RMD /%s\n"
	    "rmdir /%s 2>/dev/null\n"
	    "echo '### 000'\n",
	    rpath, rpath);
    POSTFIX(OPT_FLUSH);
}

/*
static int
fish_fh_open (struct vfs_class *me, struct vfs_s_fh *fh, int flags,
	      int mode)
{
    fh->u.fish.append = 0;
    // File will be written only, so no need to retrieve it
    if (((flags & O_WRONLY) == O_WRONLY) && !(flags & (O_RDONLY | O_RDWR))) {
	fh->u.fish.append = flags & O_APPEND;
	if (!fh->ino->localname) {
	    int tmp_handle =
		vfs_mkstemps (&fh->ino->localname, me->name,
			      fh->ino->ent->name);
	    if (tmp_handle == -1)
		return -1;
	    close (tmp_handle);
	}
	return 0;
    }
    if (!fh->ino->localname)
	if (vfs_s_retrieve_file (me, fh->ino) == -1)
	    return -1;
    if (!fh->ino->localname)
    {
	ERRMSG("retrieve_file failed to fill in localname");
	return -1;
    }
    return 0;
}
*/
/*
static void
fish_fill_names (struct vfs_class *me, fill_names_f func)
{
    struct vfs_s_super *super = MEDATA->supers;
    const char *flags;
    char *name;
    
    while (super){
	switch (conn->flags & (FISH_FLAG_RSH | FISH_FLAG_COMPRESSED)) {
	case FISH_FLAG_RSH:
		flags = ":r";
		break;
	case FISH_FLAG_COMPRESSED:
		flags = ":C";
		break;
	case FISH_FLAG_RSH | FISH_FLAG_COMPRESSED:
		flags = "";
		break;
	default:
		flags = "";
		break;
	}

	name = g_strconcat ("/#sh:", conn->user, "@", conn->host, flags,
			    "/", conn->cwdir, (char *) NULL);
	(*func)(name);
	g_free (name);
	super = super->next;
    }
}
*/
/*
void
init_fish (void)
{
    static struct vfs_s_subclass fish_subclass;

    fish_subclass.flags = VFS_S_REMOTE;
    fish_subclass.archive_same = fish_archive_same;
    fish_subclass.open_archive = fish_open_archive;
    fish_subclass.free_archive = fish_free_archive;
    fish_subclass.fh_open = fish_fh_open;
    fish_subclass.dir_load = fish_dir_load;
    fish_subclass.dir_uptodate = fish_dir_uptodate;

    vfs_s_init_class (&vfs_fish_ops, &fish_subclass);
    vfs_fish_ops.name = "fish";
    vfs_fish_ops.prefix = "sh:";
    vfs_fish_ops.fill_names = fish_fill_names;
    vfs_fish_ops.chmod = fish_chmod;
    vfs_fish_ops.chown = fish_chown;
    vfs_fish_ops.symlink = fish_symlink;
    vfs_fish_ops.link = fish_link;
    vfs_fish_ops.rename = fish_rename;
    vfs_fish_ops.rmdir = fish_rmdir;
    vfs_fish_ops.ctl = fish_ctl;
    vfs_register_class (&vfs_fish_ops);
}
*/


static int        mvfs_fishfs_fileops_open    (MVFS_FILE* file, mode_t mode);
static off64_t    mvfs_fishfs_fileops_seek    (MVFS_FILE* file, off64_t offset, int whence);
static ssize_t    mvfs_fishfs_fileops_read    (MVFS_FILE* file, void* buf, size_t count);
static ssize_t    mvfs_fishfs_fileops_write   (MVFS_FILE* file, const void* buf, size_t count);
static ssize_t    mvfs_fishfs_fileops_pread   (MVFS_FILE* file, void* buf, size_t count, off64_t offset);
static ssize_t    mvfs_fishfs_fileops_pwrite  (MVFS_FILE* file, const void* buf, size_t count, off64_t offset);
static int        mvfs_fishfs_fileops_setflag (MVFS_FILE* file, MVFS_FILE_FLAG flag, long value);
static int        mvfs_fishfs_fileops_getflag (MVFS_FILE* file, MVFS_FILE_FLAG flag, long* value);
static MVFS_STAT* mvfs_fishfs_fileops_stat    (MVFS_FILE* file);
static int        mvfs_fishfs_fileops_close   (MVFS_FILE* file);
static int        mvfs_fishfs_fileops_eof     (MVFS_FILE* file);
static MVFS_FILE* mvfs_fishfs_fileops_lookup  (MVFS_FILE* file, const char* name);
static MVFS_STAT* mvfs_fishfs_fileops_scan    (MVFS_FILE* file);
static int        mvfs_fishfs_fileops_reset   (MVFS_FILE* file);

static MVFS_FILE_OPS fishfs_fileops = 
{
    .seek	= mvfs_fishfs_fileops_seek,
    .read	= mvfs_fishfs_fileops_read,
    .write	= mvfs_fishfs_fileops_write,
    .pread	= mvfs_fishfs_fileops_pread,
    .pwrite	= mvfs_fishfs_fileops_pwrite,
    .close	= mvfs_fishfs_fileops_close,
    .eof        = mvfs_fishfs_fileops_eof,
    .lookup     = mvfs_fishfs_fileops_lookup,
    .reset      = mvfs_fishfs_fileops_reset,
    .scan       = mvfs_fishfs_fileops_scan,
    .stat	= mvfs_fishfs_fileops_stat
};

static MVFS_FILE* mvfs_fishfs_fsops_open    (MVFS_FILESYSTEM* fs, const char* name, mode_t mode);
static MVFS_STAT* mvfs_fishfs_fsops_stat    (MVFS_FILESYSTEM* fs, const char* name);
static int        mvfs_fishfs_fsops_unlink  (MVFS_FILESYSTEM* fs, const char* name);
static int        mvfs_fishfs_fsops_free    (MVFS_FILESYSTEM* fs);
static int        mvfs_fishfs_fsops_mkdir   (MVFS_FILESYSTEM* file, const char* name, mode_t mode);

static MVFS_FILESYSTEM_OPS fishfs_fsops = 
{
    .openfile	= mvfs_fishfs_fsops_open,
    .unlink	= mvfs_fishfs_fsops_unlink,
    .stat       = mvfs_fishfs_fsops_stat,
    .mkdir      = mvfs_fishfs_fsops_mkdir
};

#define FILEOP_HEAD		\
    FISH_CONNECTION* conn = ((FISH_CONNECTION*)(file->fs->priv.ptr));	\
    FISH_FILE*       ff   = ((FISH_FILE*)(file->priv.buffer));

#define FILEOP_HEAD_SAFE(err)						\
    if (file->fs == NULL)						\
    {									\
	ERRMSG("file has no fs ptr");					\
	return err;							\
    }									\
    FISH_CONNECTION* conn = ((FISH_CONNECTION*)(file->fs->priv.ptr));	\
    FISH_FILE*       ff   = ((FISH_FILE*)(file->priv.buffer));		\
    if (conn == NULL)							\
    {									\
	ERRMSG("file's fs has no priv data");				\
	return err;							\
    }									\
    if (ff == NULL)							\
    {									\
	ERRMSG("file has no priv data");				\
    }

#define FSOP_HEAD		\
    FISH_CONNECTION* conn = ((FISH_CONNECTION*)(fs->priv.ptr));

static inline int __getlocal(MVFS_FILE* file)
{
    FILEOP_HEAD_SAFE(0)

    if (ff->got_local_copy)
	return 1;

    DEBUGMSG("Fetching local copy of file: %s", ff->name);    
    int ret;
    if (ret = fish_copy2local(conn, ff, 0))
    {
	ERRMSG("failed with: %d\n", ret);
	file->errcode = ret;
	return 0;
    }

    ff->got_local_copy = 1;
    return 1;
}

static off64_t mvfs_fishfs_fileops_seek (MVFS_FILE* file, off64_t offset, int whence)
{
    FILEOP_HEAD_SAFE(-1)

    fprintf(stderr,"fileops_seek() off=%ld\n", offset);    
    __getlocal(file);
    off_t ret = lseek(ff->tmp_fd, offset, whence);
    file->errcode = errno;
    return ret;
}

static ssize_t mvfs_fishfs_fileops_read (MVFS_FILE* file, void* buf, size_t count)
{
    FILEOP_HEAD_SAFE(-1)

    fprintf(stderr,"fileops_read() count=%ld\n", count);

    __getlocal(file);
    memset(buf, 0, count);
    ssize_t s = read(ff->tmp_fd, buf, count);
    file->errcode = errno;
    if (s==0)
	ff->eof = 1;
    return s;
}

static ssize_t mvfs_fishfs_fileops_pread (MVFS_FILE* file, void* buf, size_t count, off64_t offset)
{
    mvfs_fishfs_fileops_seek(file, offset, SEEK_SET);
    return mvfs_fishfs_fileops_read(file, buf, count);
}

static ssize_t mvfs_fishfs_fileops_write (MVFS_FILE* file, const void* buf, size_t count)
{
    ERRMSG("writing not supported yet");
    file->errcode = ENOTSUP;
    return -1;
}

static ssize_t mvfs_fishfs_fileops_pwrite (MVFS_FILE* file, const void* buf, size_t count, off64_t offset)
{
    mvfs_fishfs_fileops_seek(file,offset,SEEK_SET);
    return mvfs_fishfs_fileops_write(file,buf,count);
}

static inline const char* __mvfs_flag2str(MVFS_FILE_FLAG f)
{
    switch (f)
    {
	case NONBLOCK:		return "NONBLOCK";
	case READ_TIMEOUT:	return "READ_TIMEOUT";
	case WRITE_TIMEOUT:	return "WRITE_TIMEOUT";
	case READ_AHEAD:	return "READ_AHEAD";
	case WRITE_ASYNC:	return "WRITE_ASYNNC";
	default:		return "UNKNOWN";
    }
}

static int mvfs_fishfs_fileops_setflag (MVFS_FILE* fp, MVFS_FILE_FLAG flag, long value)
{
    ERRMSG("%s not supported", __mvfs_flag2str(flag));
    fp->errcode = EINVAL;
    return -1;
}

static int mvfs_fishfs_fileops_getflag (MVFS_FILE* fp, MVFS_FILE_FLAG flag, long* value)
{
    ERRMSG("%s not supported", __mvfs_flag2str(flag));
    fp->errcode = EINVAL;
    return -1;
}

static MVFS_STAT* mvfs_stat_from_unix(const char* name, struct stat s)
{
    struct passwd* pw = getpwuid(s.st_uid);
    struct group*  gr = getgrgid(s.st_gid);

    const char* uid="???";
    const char* gid="???";
    
    if (pw)
	uid = pw->pw_name;
    if (gr)
	gid = gr->gr_name;

    MVFS_STAT* mstat = mvfs_stat_alloc(name, uid, gid);
    mstat->mode  = s.st_mode;
    mstat->size  = s.st_size;
    mstat->atime = s.st_atime;
    mstat->mtime = s.st_mtime;
    mstat->ctime = s.st_ctime;

    return mstat;
}

static MVFS_STAT* mvfs_fishfs_fileops_stat(MVFS_FILE* fp)
{
    struct stat ust;
    int ret = fstat(PRIV_FD(fp), &ust);

    if (ret!=0)
    {
	fp->fs->errcode = errno;
	return NULL;
    }
    
    return mvfs_stat_from_unix(PRIV_NAME(fp), ust);
}

static MVFS_FILE* mvfs_fishfs_fsops_open(MVFS_FILESYSTEM* fs, const char* name, mode_t mode)
{
    int fd = open(name, mode);
//    if (fd<0)
//    {
//	fs->errcode = errno;
//	return NULL;
//    }

    MVFS_FILE* file = mvfs_file_alloc(fs,fishfs_fileops);
    file->priv.name = strdup(name);
    file->priv.id   = fd;

    FISH_FILE* ff = (FISH_FILE*)calloc(1,sizeof(FISH_FILE));
    ff->scanpos   = -1;
    ff->name      = strdup(name);
    ff->tmp_fd    = -1;
    ff->tmpname   = NULL;
    ff->eof       = 0;
    file->priv.buffer = (char*)ff;

    return file;
}

static MVFS_STAT* mvfs_fishfs_fsops_stat(MVFS_FILESYSTEM* fs, const char* name)
{
    ERRMSG("fsops_stat() not implemented yet");
    if (fs==NULL)
    {
	ERRMSG("fs==NULL");;
	return NULL;
    }
    
    if (name==NULL)
    {
	ERRMSG("name==NULL");
	fs->errcode = EFAULT;
	return NULL;
    }

    struct stat ust;
    int ret = lstat(name, &ust);

    if (ret!=0)
    {
	fs->errcode = errno;
	return NULL;
    }

    return mvfs_stat_from_unix(name, ust);
}

static int mvfs_fishfs_fsops_unlink(MVFS_FILESYSTEM* fs, const char* name)
{
    FSOP_HEAD

    ERRMSG("unlink: %s", name);

    int ret = fish_unlink(conn, name);

    if ((ret != 0) && (errno == EISDIR))
	ret = fish_rmdir(conn, name);

    if (ret == 0)
	return 0;

    fs->errcode = errno;
    return errno;
}

static int mvfs_fishfs_fsops_mkdir(MVFS_FILESYSTEM* fs, const char* fn, mode_t mode)
{
    FSOP_HEAD
    return fish_mkdir(conn, fn, 0);
}

MVFS_FILESYSTEM* mvfs_fishfs_create_args(MVFS_ARGS* args)
{
    const char* chroot = mvfs_args_get(args,"chroot");
    if ((chroot) && (strlen(chroot)) && (!strcmp(chroot,"/")))
    {
	ERRMSG("chroot not supported yet!");
	return NULL;
    }
    
    MVFS_FILESYSTEM* fs = mvfs_fs_alloc(fishfs_fsops, FS_MAGIC);

    FISH_CONNECTION* conn = (FISH_CONNECTION*)calloc(1,sizeof(FISH_CONNECTION));
    conn->hostname = mvfs_args_get(args,"host");
    conn->username = mvfs_args_get(args,"username");
    conn->secret   = mvfs_args_get(args,"secret");
    conn->path     = mvfs_args_get(args,"path");
    conn->url      = mvfs_args_get(args,"url");
    conn->port     = mvfs_args_get(args,"port");

    DEBUGMSG("FishFS: initializing ... 0002\n");
    DEBUGMSG("url=%s\nhost=%s\nport=%s\npath=%s\n", conn->url, conn->hostname, conn->port, conn->path);

    fish_connect(conn);

    fs->priv.ptr = conn;

    return fs;
}

static int mvfs_fishfs_fileops_close(MVFS_FILE* file)
{
    FILEOP_HEAD_SAFE(-1)

//    close(PRIV_FD(file));
    DEBUGMSG("closing tmp_fd %d", ff->tmp_fd);
    close(ff->tmp_fd);
    if (ff->tmpname)
    {
	DEBUGMSG("removing tempfile \"%s\"", ff->tmpname);
	unlink(ff->tmpname);
	free(ff->tmpname);
	ff->tmpname = NULL;
    }
    file->priv.id = -1;
    ff->tmp_fd = -1;
    return 0;
}

static int mvfs_fishfs_fileops_eof(MVFS_FILE* file)
{
    FILEOP_HEAD_SAFE(-1)

    return ((ff->eof) ? 1 : 0);
}

static MVFS_FILE* mvfs_fishfs_fileops_lookup  (MVFS_FILE* file, const char* name)
{
    ERRMSG("lookup() not implemented yet");

    int fd = openat(PRIV_FD(file), name, O_RDONLY);
    if (fd<0)
	return NULL;

    MVFS_FILE* f2 = mvfs_file_alloc(file->fs,fishfs_fileops);
    f2->priv.name = strdup(name);
    f2->priv.id   = fd;

    return file;
}

static int mvfs_fishfs_fileops_init_dir(MVFS_FILE* file)
{
    FILEOP_HEAD_SAFE(-1)

    /* new scanning algo */

    if (ff->scanpos != -1)
    {
	return 0;
    }

    const char* remote_path = file->priv.name;
    const char* quoted_path = file->priv.name;

    fish_command (conn, NULL, NONE,
	    "#LIST /%s\n"
	    "ls -lLa /%s 2>/dev/null | grep '^[^cbt]' | (\n"
	      "while read p x u g s m d y n; do\n"
	        "echo \"P$p $u.$g\nS$s\nd$m $d $y\n:$n\n\"\n"
	      "done\n"
	    ")\n"
	    "ls -lLa /%s 2>/dev/null | grep '^[cb]' | (\n"
	      "while read p x u g a i m d y n; do\n"
	        "echo \"P$p $u.$g\nE$a$i\nd$m $d $y\n:$n\n\"\n"
	      "done\n"
	    ")\n"
	    "echo '### 200'\n",
	    remote_path, quoted_path, quoted_path);

    char buffer[8192];
    MVFS_STAT* curstat = ff->stats[0] = mvfs_stat_alloc("<unknown>", "<nobody>", "<nogroup>");
    ff->statcount = 0;

    while (1) 
    {
	int res = __mvfs_sock_get_line(conn->logfile, conn->sockr, buffer, sizeof (buffer), '\n');

	if ((!res) || (res == EINTR))
	{
	    ERRMSG("connection reset or interrupted\n");
	    return -1;
	}

	if (conn->logfile) 
	{
	    fputs (buffer, conn->logfile);
            fputs ("\n", conn->logfile);
	    fflush (conn->logfile);
	}

	if (!strncmp(buffer, "### ", 4))
	    break;

	if ((!buffer[0])) 
	{
	    // silently ignore . and .. 
	    if ((curstat->name) && (strcmp(curstat->name,".")) && (strcmp(curstat->name,"..")))
	    {
		
		ff->statcount++;
		ff->stats[ff->statcount] = curstat = mvfs_stat_alloc("<unknown>", "<nobody>", "<nogroup>");
	    }
	    continue;
	}

	switch(buffer[0]) 
	{
	    case ':': 
	    {
		curstat->name = strdup(buffer+1);
		break;
	    }
	    case 'S':
#ifdef HAVE_ATOLL
		curstat->size = (off_t) atoll (buffer+1);
#else
		curstat->size = (off_t) atof (buffer+1);
#endif
	    break;
	    case 'P': 
	    {
	        int i;
		if ((i = mvfs_decode_filetype(buffer[1])) ==-1)
		    break;
		curstat->mode = i; 
		if ((i = mvfs_decode_filemode(buffer+2)) ==-1)
		    break;
		curstat->mode |= i;
		if (S_ISLNK(curstat->mode))
		    curstat->mode = 0;
		    
		// now fetch the username and groupname
		char* p1 = strchr(buffer,' ');
		if (p1)
		{
		    while ((*p1)==' ')
			p1++;
		
		    char* p2 = strchr(p1, '.');
		    if (p2)
		    {
			*p2 = 0;
			p2++;
			curstat->gid = strdup(p2);
		    }
		    curstat->uid = strdup(p1);
		}
	    }
	    break;
	    case 'd': 
	    {
		mvfs_split_text(buffer+1);
		if (!mvfs_decode_filedate(0, &curstat->ctime))
		    break;
		curstat->atime = curstat->mtime = curstat->ctime;
	    }
	    break;
	    case 'D': 
	    {
		struct tm tim;
		if (sscanf(buffer+1, "%d %d %d %d %d %d", &tim.tm_year, &tim.tm_mon, 
			 &tim.tm_mday, &tim.tm_hour, &tim.tm_min, &tim.tm_sec) != 6)
		    break;
		curstat->atime = curstat->mtime = curstat->ctime = mktime(&tim);
	    }
	    break;
	    case 'E': 
	    {
		int maj, min;
	        if (sscanf(buffer+1, "%d,%d", &maj, &min) != 2)
		    break;
#ifdef HAVE_STRUCT_STAT_ST_RDEV
		curstat->rdev = (maj << 8) | min;
#endif
	    }
	    case 'L': 
		ERRMSG("skipping linkname: %s\n", (buffer+1));
	    break;
	}
    }

    if (fish_decode_reply(buffer+4, 0) == COMPLETE) 
    {
	free (conn->cwdir);
	conn->cwdir = strdup (remote_path);
	print_vfs_message (_("%s: done."), remote_path);
	ff->scanpos = 0;
	return 0;
    }

    print_vfs_message (_("%s: failure"), remote_path);
    return 1;
}

static MVFS_STAT* mvfs_fishfs_fileops_scan(MVFS_FILE* file)
{
    FILEOP_HEAD_SAFE(NULL)

    mvfs_fishfs_fileops_init_dir(file);

    if ((ff->scanpos != -1) && (ff->scanpos<ff->statcount))
	return mvfs_stat_dup(ff->stats[ff->scanpos++]);
    else
	return NULL;
}

static int mvfs_fishfs_fileops_reset(MVFS_FILE* file)
{
    FILEOP_HEAD_SAFE(-1)
    mvfs_fishfs_fileops_init_dir(file);
    ff->scanpos = 0;
    return 1;
}
