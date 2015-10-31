#!/bin/sh -x

rm -rf libevhtp

git clone -b 1.2.10 https://github.com/ellzey/libevhtp.git
cd libevhtp

INSTALL_DIR=`pwd`/s3_dist
mkdir $INSTALL_DIR

CFLAGS=-fPIC cmake . -DCMAKE_PREFIX_PATH=`pwd`/../libevent/s3_dist -DCMAKE_INSTALL_PREFIX:PATH=$INSTALL_DIR
make
make install
#make test

cd ..
