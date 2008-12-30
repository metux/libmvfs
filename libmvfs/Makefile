
SONAME=mvfs

SRCNAMES  = default_ops fileops hostfs_ops mixp_ops fsops strmode arglist autoconnect_ops metacache_fs
NOPIC_OBJ = $(addsuffix .nopic.o, $(SRCNAMES))
PIC_OBJ   = $(addsuffix .pic.o, $(SRCNAMES))
UNO_OBJ   = $(addsuffix .uno, $(SRCNAMES))

CFLAGS+=-I../include $(MIXP_CFLAGS) $(HASH_CFLAGS) -D_GNU_SOURCE
LDFLAGS+=$(MIXP_LIBS) $(HASH_LIBS) -lc -no-undefined

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
