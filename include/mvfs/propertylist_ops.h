
#ifndef __IXPSERV_PROPERTYLIST_OPS_H
#define __IXPSERV_PROPERTYLIST_OPS_H

#include <mvfs/mvfs.h>
#include <malloc.h>
#include <memory.h>

typedef struct _MVFS_PROPERTYLIST_DEF MVFS_PROPERTYLIST_DEF;
typedef struct _MVFS_PROPERTYLIST_ENT MVFS_PROPERTYLIST_ENT;

typedef enum
{
    MVFS_PL_STRING = 1,
    MVFS_PL_INT    = 2,
    MVFS_PL_DIR    = 3,
    MVFS_PL_FILE   = 4
} MVFS_PROPERTYLIST_ENT_TYPE;
    
struct _MVFS_PROPERTYLIST_ENT
{
    int                       id;		// ID (for user only)
    const char*               name;		// NULL means: last entry in list
    const char*               value;		// NULL means: ask the getValue() proc
    MVFS_PROPERTYLIST_ENT_TYPE type;
    void*                     priv;		// private data
    const char*               uid;		// User-ID (NULL to use default)
    const char*               gid;		// Group-ID (NULL to use default)
    MVFS_FILE* (*getFileHandle)(MVFS_PROPERTYLIST_DEF* def, MVFS_PROPERTYLIST_ENT* ent);
};

typedef struct 
{
    int        (*free)     (MVFS_PROPERTYLIST_DEF* def);
    int        (*getInt)   (MVFS_PROPERTYLIST_DEF* def, MVFS_PROPERTYLIST_ENT* ent, long* ret);
    int        (*setInt)   (MVFS_PROPERTYLIST_DEF* def, MVFS_PROPERTYLIST_ENT* ent, long value);
    int        (*setString)(MVFS_PROPERTYLIST_DEF* def, MVFS_PROPERTYLIST_ENT* ent, const char* value);
    int        (*getString)(MVFS_PROPERTYLIST_DEF* def, MVFS_PROPERTYLIST_ENT* ent, char* buffer, long max);
    MVFS_FILE* (*getChild) (MVFS_PROPERTYLIST_DEF* def, MVFS_PROPERTYLIST_ENT* ent);
} MVFS_PROPERTYLIST_OPS;

struct _MVFS_PROPERTYLIST_DEF
{
    MVFS_PROPERTYLIST_ENT* entries;
    MVFS_PROPERTYLIST_OPS  ops;
    void*                  priv;
    const char*            uid;
    const char*            gid;
};

/* -- file access to an in-memory string (read-only) -- */
MVFS_FILE* mvfs_propertylist_create(const char* str, MVFS_PROPERTYLIST_DEF* def);

MVFS_PROPERTYLIST_DEF* mvfs_propertylist_create_def(
    MVFS_PROPERTYLIST_ENT* entries, 
    MVFS_PROPERTYLIST_OPS ops,
    const char* uid,
    const char* gid,
    void* private
);

#endif
