/*
    libmvfs - metux Virtual Filesystem Library

    Filesystem driver: Propertylist-based filesystem

    Copyright (C) 2008 Enrico Weigelt, metux IT service <weigelt@metux.de>
    This code is published under the terms of the GNU Public License 2.0
*/

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>

#include <mvfs/mvfs.h>
#include <mvfs/propertylist_ops.h>

#define PROPLISTPRIV(f)		((MVFS_PROPERTYLIST_DEF*)(f->priv.name))
#define PROPENTPRIV(f)		((MVFS_PROPERTYLIST_DEF*)(f->priv.text))

// static MVFS_FILE* mvfs_propertylist_ops_lookup   (MVFS_FILE* f, const char* name);
static int        mvfs_propertylist_ops_stat     (MVFS_FILE* f, MVFS_STAT* stat);
static ssize_t    mvfs_propertylist_entops_read  (MVFS_FILE* f, void* buf, size_t size);
static long       mvfs_propertylist_entops_write (MVFS_FILE* f, const void* buf, size_t size);
static ssize_t    mvfs_propertylist_entops_pread (MVFS_FILE* f, void* buf, size_t size, int64_t offset);
static long       mvfs_propertylist_entops_pwrite(MVFS_FILE* f, const void* buf, size_t size, int64_t offset);
static int        mvfs_propertylist_entops_open  (MVFS_FILE* f, mode_t mode);
static MVFS_STAT* mvfs_propertylist_entops_stat  (MVFS_FILE* f);
static long       mvfs_propertylist_entops_size  (MVFS_FILE* f);

/*
static MVS_FILE_OPS propertylist_ops = 
{
    .lookup = mvfs_propertylist_ops_lookup,
    .stat   = mvfs_propertylist_ops_stat
};
*/

static MVFS_FILE_OPS propertylist_entops = 
{
//    .read   = mvfs_propertylist_entops_read,
//    .write  = mvfs_propertylist_entops_write,
//    .pread  = mvfs_propertylist_entops_pread,
//    .pwrite = mvfs_propertylist_entops_pwrite,
//    .reopen = mvfs_propertylist_entops_open,
//    .stat   = mvfs_propertylist_entops_stat,
//    .size   = mvfs_propertylist_entops_size
};

/*
MVFS_FILE* mvfs_propertylist_create(const char* name, MVFS_PROPERTYLIST_DEF* pr)
{
    MVFS_FILE* f = p9srv_get_file();
    f->name      = strdup(name);
    f->qtype     = P9_QTDIR;
    f->type      = 0;
    f->perm      = 0555|P9_DMDIR;
    f->priv.name = (char*)pr;
    f->ops       = propertylist_ops;
    return f;
}
*/

static MVFS_FILE* mvfs_propertylist_createent(MVFS_PROPERTYLIST_DEF* pr, MVFS_PROPERTYLIST_ENT* ent)
{
    if (ent->getFileHandle)
	return ent->getFileHandle(pr,ent);

    MVFS_FILE* f = mvfs_file_alloc(NULL,propertylist_entops);
    f->priv.ptr    = pr;
    f->priv.buffer = (char*)ent;
    return f;
}

/*
static inline MVFS_FILE* mvfs_propertylist_ops_lookup_list(MVFS_PROPERTYLIST_DEF* def)
{
    int x;
    MVFS_FILE* fh = NULL;
    MVFS_FILE* f  = NULL;
    for (x=0; def->entries[x].name; x++)
    {
	fprintf(stderr,"MVFS_PROPERTYLIST_OPS_lookup_list() walking: \"%s\"\n", def->entries[x].name);
	if (fh==NULL)
	{
	    f = fh = mvfs_propertylist_createent(def,&(def->entries[x]));
	}
	else
	{
	    fh->next = mvfs_propertylist_createent(def,&(def->entries[x]));
	    fh = fh->next;
	}
    }

    return f;
}
*/
/*
static inline MVFS_FILE* mvfs_propertylist_ops_lookup_ent(MVFS_PROPERTYLIST_DEF* def, const char* name)
{
    int x;
    for (x=0; def->entries[x].name; x++)
    {
	if (strcmp(def->entries[x].name,name)==0)
	{
	    return mvfs_propertylist_createent(def, &(def->entries[x]));
	}
    }
    return NULL;
}

static MVFS_FILE* mvfs_propertylist_ops_lookup(MVFS_FILE* f, char* name)
{
    if (name==NULL)
	return mvfs_propertylist_ops_lookup_list(PROPLISTPRIV(f));
    else
	return mvfs_propertylist_ops_lookup_ent(PROPLISTPRIV(f),name);
}

static inline long mvfs_propertylist_write_long(MVFS_PROPERTYLIST_DEF* pr, MVFS_PROPERTYLIST_ENT* ent, const char* text)
{
    if (pr->ops.setInt == NULL)
    {
	fprintf(stderr,"ixp_propertylist_write_long(): Propertylist has no setInt() method. Sending EPERM\n");
	return -EPERM;
    }

    long l = -666;
    errno = 0;
    l = strtol(text,NULL,0);
    
    if (errno!=0)
    {
	fprintf(stderr,"ixp_propertylist_write_long(): hmm, could not parse long out of text \"%s\" - sending EPERM\n", text);
	perror("conversion failed: ");
	return -EPERM;
    }

    return pr->ops.setInt(pr,ent,l);
}

static inline long mvfs_propertylist_write_string(MVFS_PROPERTYLIST_DEF* pr, MVFS_PROPERTYLIST_ENT* ent, const char* text)
{
    if (pr->ops.setString == NULL)
    {
	fprintf(stderr,"ixp_propertylist_write_string(): Propertylist has no setInt() method. Sending EPERM\n");
	return -EPERM;
    }

    fprintf(stderr,"ixp_propertylist_write_string() writing string \"%s\"\n", text);
    return pr->ops.setString(pr,ent,text);
}

static long mvfs_propertylist_entops_write(MVFS_FILE* f, long offset, long size, void* buf)
{
    if (size==0)
    {
//	fprintf(stderr,"DEBUG ixp_propertylist_entops_write() ignoring empty write (kind of EOF ?)\n");
	return 0;
    }

    MVFS_PROPERTYLIST_DEF * def = (MVFS_PROPERTYLIST_DEF*) f->priv.name;
    MVFS_PROPERTYLIST_ENT * ent = (MVFS_PROPERTYLIST_ENT*) f->priv.text;

//    fprintf(stderr,"ixp_propertylist_entops_write() offset=%ld size=%ld ptr=%ld\n", offset, size, buf);
//    fprintf(stderr," -> name=\"%s\"\n", ent->name);

    if (buf==NULL)
    {
	fprintf(stderr,"ERROR: ixp_propertylist_entops_write() NULL buffer !\n");
	return -EPERM;
    }

    fprintf(stderr,"ixp_propertylist_entops_write() offset=%ld size=%ld\n", offset, size);

    // convert counted-size memchunk to 0-terminated char
    char* text = malloc(size+1);
    memset(text,0,size+1);
    memcpy(text,buf,size);

    if (offset)
    {
	fprintf(stderr,"FIXME: ixp_propertylist_entops_write() offset=%ld > 0 - not supported. sending EPERM\n", offset);
	return -EPERM;
    }
    
    switch (ent->type)
    {
	case P9_PL_INT:
	    return ((mvfs_propertylist_write_long(def,ent,text)) ? size : 0);
	case P9_PL_STRING:
	    return ((mvfs_propertylist_write_string(def,ent,text)) ? size : 0);
	default:
	    fprintf(stderr,"ixp_propertylist_entops_write() unsupported ent type %ud - sending EPERM\n", ent->type);
	    return -EPERM;
    }
}

// FIMXE: add sanity checks
static long mvfs_propertylist_entops_read(MVFS_FILE* f, long offset, long size, void* buf)
{
    MVFS_PROPERTYLIST_DEF * def = (MVFS_PROPERTYLIST_DEF*) f->priv.name;
    MVFS_PROPERTYLIST_ENT * ent = (MVFS_PROPERTYLIST_ENT*) f->priv.text;

    fprintf(stderr,"ixp_propertylist_entops_read() reading from \"%s\" offset %ld size %ld\n", ent->name, offset, size);

    // the buffer is *always* zero'ed first. our clients can rely on that
    memset(buf,0,size);

    switch (ent->type)
    {
	case P9_PL_STRING:
	    // we've got an constant value
	    if (ent->value)
	    {
		// FIXME: should handle offsets !
		if (offset)
		{
		    fprintf(stderr,"read_string() does not yet support offsets in consts (%ld)\n", offset);
		    return 0;
		}
		strncpy(buf,ent->value,size-1);
		return strlen(buf);
	    }

	    // no constant value -- check for getString() handler
	    if (def->ops.getString == NULL)
	    {
		fprintf(stderr,"read_string() no getString handler in this propertylist\n");
		strcpy(buf,"");
		return 0;
	    }

	    // offsets are not yet support :(
	    if (offset)
	    {
		fprintf(stderr,"read_string() does not support offsets (%ld)\n", offset);
		return 0;
	    }

	    // evrything seems okay - call the getString() handler
	    fprintf(stderr,"reading string\n");
	    def->ops.getString(def,ent,buf,size);
	    return strlen(buf);
	break;

	case P9_PL_INT:
	    // check if we have an constant value
	    if (ent->value)
	    {
		strncpy(buf,ent->value,size-1);
		return strlen(buf);
	    }

	    if (offset)
	    {
		fprintf(stderr,"read_int() does not support offsets (%ld)\n", offset);
		return 0;
	    }

	    // do we have an getInt() handler in the propertylist ?
	    if (def->ops.getInt==NULL)
	    {
		fprintf(stderr,"read_int() propertylist has no getInt() handler\n");
		return 0;
	    }

	    // evrything seems okay - get 
	    long ret;
	    def->ops.getInt(def,ent,&ret);
	    return snprintf(buf,size-1,"%ld",ret);
	break;

	case P9_PL_DIR:
	    fprintf(stderr,"Reading text from directories not supported yet\n");
	    return 0;
	break;

	default:
	    fprintf(stderr,"ixp_propertylist_entops_read() unsupported entry type %ud\n", ent->type);
	    return 0;
    }

    return 0;
}

static long mvfs_propertylist_entops_size(MVFS_FILE* f)
{
    char buffer[1024];
    buffer[0] = 0;
    mvfs_propertylist_entops_read(f, 0, sizeof(buffer)-1, buffer);
    return strlen(buffer);
}

static int mvfs_propertylist_entops_open(MVFS_FILE* f, long mode)
{
//    MVFS_PROPERTYLIST_DEF * def = (MVFS_PROPERTYLIST_DEF*) f->priv.name;
//    MVFS_PROPERTYLIST_ENT * ent = (MVFS_PROPERTYLIST_ENT*) f->priv.text;

//    fprintf(stderr,"ixp_propertylist_entops_open() mode %ld\n", mode);
//    fprintf(stderr,"ixp_propertylist_entops_open() \"%s\" mode %ld\n", ent->name, mode);
    return 0;
}

static MVFS_STAT* mvfs_propertylist_ops_stat (MVFS_FILE* f)
{
    MVFS_PROPERTYLIST_DEF * def = (MVFS_PROPERTYLIST_DEF*) f->priv.name;

    mixpsrv_default_ops_stat(f,stat);
    if (def->uid)
	stat->uid = strdup(def->uid);

    if (def->gid)
	stat->gid = strdup(def->gid);

    return 1;
}

static int mvfs_propertylist_entops_stat (MVFS_FILE* f, MVFS_STAT* stat)
{
    MVFS_PROPERTYLIST_DEF * def = (MVFS_PROPERTYLIST_DEF*) f->priv.name;
    MVFS_PROPERTYLIST_ENT * ent = (MVFS_PROPERTYLIST_ENT*) f->priv.text;

    mixpsrv_default_ops_stat(f,stat);
    if (ent->uid)
	stat->uid = strdup(ent->uid);
    else if (def->uid)
	stat->uid = strdup(def->uid);

    if (ent->gid)
	stat->gid = strdup(ent->gid);
    else if (def->gid)
	stat->gid = strdup(def->gid);

    return 1;
}

MVFS_PROPERTYLIST_DEF* mvfs_propertylist_create_def(
    MVFS_PROPERTYLIST_ENT* entries, 
    MVFS_PROPERTYLIST_OPS ops,
    const char* uid,
    const char* gid,
    void* private
)
{
    MVFS_PROPERTYLIST_DEF* def = (MVFS_PROPERTYLIST_DEF*)malloc(sizeof(MVFS_PROPERTYLIST_DEF));
    memset(def,0,sizeof(MVFS_PROPERTYLIST_DEF));
    def->entries = entries;
    def->ops     = ops;
    def->priv    = private;
    def->uid     = strdup(uid);
    def->gid     = strdup(gid);
    return def;
}

*/
