#
# Author(s): Enrico Weigelt <weigelt@metux.de>
#

all:	lib	libmvfs.pc	client

include ./build.mk

lib:
	make -C libmvfs

client:
	make -C cmd

install-lib:	
	make -C libmvfs install

install-pkgconfig:	libmvfs.pc
	mkdir -p $(DESTDIR)$(PKGCONFIGDIR)
	cp libmvfs.pc $(DESTDIR)$(PKGCONFIGDIR)
	
install-includes:	include/mvfs/*.h
	mkdir -p $(DESTDIR)$(INCLUDEDIR)/mvfs
	for i in include/mvfs/*.h ; do cp $$i $(DESTDIR)$(INCLUDEDIR)/mvfs ; done

install:	install-pkgconfig install-includes install-lib

clean:	
	rm -f *.o *.a *.so libmvfs.pc
	make -C libmvfs clean
	make -C cmd clean
