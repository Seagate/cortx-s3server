#!/bin/sh -x

LIBEVENT_DIST=/opt/seagate/s3/libevent
LIBEVHTP_BUILD=`pwd`/libevhtp
GOOGLE_PROTOBUF_DIST=/opt/seagate/s3/gprotobuf
MERO_SRC=`pwd`/../..

S3_SERVER_CFLAGS="-DEVHTP_HAS_C99 -DEVHTP_SYS_ARCH=64 -DGCC_VERSION=4002 -DHAVE_CONFIG_H -DM0_TARGET=ClovisTest -D_REENTRANT -D_GNU_SOURCE -DM0_INTERNAL= -DM0_EXTERN=extern -iquote $MERO_SRC -iquote . -include config.h  -I/usr/src/lustre-2.5.1-headers/libcfs/include -I /usr/src/lustre-2.5.1-headers/lnet/include -I/usr/src/lustre-2.5.1-headers/lustre/include -fno-common -Wall -Wno-attributes -fno-strict-aliasing -fno-omit-frame-pointer -L $MERO_SRC/mero/.libs -L $MERO_SRC/extra-libs/gf-complete/src/.libs/ -L $LIBEVHTP_BUILD/libevhtp -ggdb3 -O3 -DNDEBUG -I$LIBEVHTP_BUILD/libevhtp -I$LIBEVHTP_BUILD/libevhtp/compat -I$LIBEVHTP_BUILD/libevhtp/htparse -I$LIBEVHTP_BUILD/libevhtp/evthr -I$LIBEVHTP_BUILD/libevhtp/oniguruma -I$GOOGLE_PROTOBUF_DIST/include"

S3_SERVER_LDFLAGS="-rdynamic $LIBEVHTP_BUILD/libevhtp/libevhtp.a $LIBEVENT_DIST/lib/libevent.so $LIBEVENT_DIST/lib/libevent_pthreads.so $LIBEVENT_DIST/lib/libevent_openssl.so -lssl -lcrypto -lpthread -ldl -lrt -lmero -lgf_complete -lm -lpthread -laio -lrt -lyaml -luuid -pthread -lprotobuf -lpthread -Wl,-rpath,$LIBEVENT_DIST/lib,-rpath,$GOOGLE_PROTOBUF_DIST/lib"

S3SERVER_SRCS='server/clovis_cat.c server/clovis_copy.c server/clovis_common.c server/clovis_delete.c server/mero_object_header.pb.cc server/murmur3_hash.cc server/s3server.cc'

rm -rf *.o

# Before we build s3, get all dependencies built.
S3_SRC_FOLDER=`pwd`
cd gprotobuf/ && ./setup.sh && cd $S3_SRC_FOLDER
cd libevent/ && ./setup.sh && cd $S3_SRC_FOLDER
cd libevhtp/ && ./setup.sh && cd $S3_SRC_FOLDER

g++ $S3_SERVER_CFLAGS -c $S3SERVER_SRCS

g++ $S3_SERVER_CFLAGS  *.o -o s3server $S3_SERVER_LDFLAGS


#make install things for testing
sudo cp ../../mero/.libs/libmero*.so /opt/seagate/s3/lib/
sudo cp ../../extra-libs/gf-complete/src/.libs/libgf_complete.so* /opt/seagate/s3/lib/
sudo cp s3server /opt/seagate/s3/bin/

