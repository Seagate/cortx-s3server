#!/bin/sh -x

rm -rf libevent

git clone -b release-2.0.22-stable --depth=1 https://github.com/libevent/libevent.git
cd libevent

INSTALL_DIR=`pwd`/s3_dist
mkdir $INSTALL_DIR

./autogen.sh
./configure --prefix=$INSTALL_DIR

make
#make verify
make install

cd ..
