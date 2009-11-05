#
# Author(s): Enrico Weigelt <weigelt@metux.de>
#

all:		mvfs urltest mvfsp

include ../build.mk

CFLAGS := -DVERSION=\"${VERSION}\" -I../include $(CFLAGS) $(MIXP_CFLAGS) $(HASH_CFLAGS)
LIBMVFS=../libmvfs/libmvfs.a $(MIXP_LIBS) $(HASH_LIBS)

#all:		ixp_client	ixpc

#ixpc:		ixpc.o
#	$(CC) -L../libmixp -lixp -o $@ $^ -lpthread
#
#ixp_client:	ixp_client.o util.o
#	$(CC) -L../libmixp -lixp -o $@ $^ -lpthread

#ixp_client::	../libmixp/libmixp.a

mvfs:	mvfs.o
	$(CC) -o $@ $^ $(LIBMVFS)

urltest:	urltest.o
	$(CC) -o $@ $^ $(LIBMVFS)

mvfsp:	mvfsp.o
	$(CC) -o $@ $^ $(LIBMVFS)

clean:
	rm -f *.o mvfs urltest mvfsp
