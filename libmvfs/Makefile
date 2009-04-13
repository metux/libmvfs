
SONAME=mvfs
LIBC_LIBS=-lc

SRCNAMES  = \
	strmode		\
	urlparse	\
	arglist		\
	default_ops 	\
	fileops 	\
	fsops		\
	$(FS_SRCNAMES)

CFLAGS+=-I../include -D_GNU_SOURCE

include _fs.*.mk

NOPIC_OBJ = $(addsuffix .nopic.o, $(SRCNAMES))
PIC_OBJ   = $(addsuffix .pic.o, $(SRCNAMES))
UNO_OBJ   = $(addsuffix .uno, $(SRCNAMES))

CFLAGS+=$(FS_CFLAGS) $(HASH_CFLAGS)
LDFLAGS+=$(FS_LIBS) $(HASH_LIBS) -no-undefined

all:	info lib$(SONAME).a lib$(SONAME).so

info:
	echo "CFLAGS=$(CFLAGS)"
	echo "LDFLAGS=$(LDFLAGS)"

include ../build.mk

lib$(SONAME).a::	$(NOPIC_OBJ)

lib$(SONAME).so::	$(PIC_OBJ)

install:	lib$(SONAME).a	lib$(SONAME).so
	mkdir -p $(DESTDIR)$(LIBDIR)
	cp lib$(SONAME).a lib$(SONAME).so $(DESTDIR)$(LIBDIR)
	chmod ugo+x $(DESTDIR)$(LIBDIR)/lib$(SONAME).so

uno-check:	$(UNO_OBJ)

clean:
	rm -f *.o *.a *.so
