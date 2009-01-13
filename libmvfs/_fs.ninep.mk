#
# Rules for the mixp/9P filesystem driver
#

FS_SRCNAMES += mixp_ops
FS_LIBS     += `$(PKG_CONFIG) --libs   libmixp`
FS_CFLAGS   += `$(PKG_CONFIG) --cflags libmixp`
