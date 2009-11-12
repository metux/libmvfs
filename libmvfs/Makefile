
LIBNAME=mvfs
SONAME=$(LIBNAME)
LIBC_LIBS=-lc

SRCNAMES  = \
	strmode		\
	urlparse	\
	arglist		\
	default_ops 	\
	fileops 	\
	fsops		\
	$(FS_SRCNAMES)

include _fs.*.mk

NOPIC_OBJ = $(addsuffix .nopic.o, $(SRCNAMES))
PIC_OBJ   = $(addsuffix .pic.o, $(SRCNAMES))
UNO_OBJ   = $(addsuffix .uno, $(SRCNAMES))

CFLAGS+=-I../include $(FS_CFLAGS) $(HASH_CFLAGS) -D_GNU_SOURCE
LDFLAGS+=$(FS_LIBS) $(HASH_LIBS) -no-undefined

all:	info lib$(LIBNAME).a lib$(LIBNAME).so

info:
	echo "CFLAGS=$(CFLAGS)"
	echo "LDFLAGS=$(LDFLAGS)"

include ../build.mk

lib$(LIBNAME).a::	$(NOPIC_OBJ)

lib$(LIBNAME).so::	$(PIC_OBJ)

install:	lib$(LIBNAME).a	lib$(LIBNAME).so
	mkdir -p $(DESTDIR)$(LIBDIR)
	cp lib$(LIBNAME).a lib$(LIBNAME).so $(DESTDIR)$(LIBDIR)
	chmod ugo+x $(DESTDIR)$(LIBDIR)/lib$(LIBNAME).so

uno-check:	$(UNO_OBJ)

clean:
	rm -f *.o *.a *.so
