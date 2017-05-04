#!/bin/sh -xe
# Script to build libevent.
# git repo: https://github.com/libevent/libevent.git
# tag: release-2.0.22-stable commit: c51b159cff9f5e86696f5b9a4c6f517276056258

cd libevent

# Apply the libevent patch for memory pool support
patch -f -p1 < ../../patches/libevent.patch

INSTALL_DIR=/usr/local/seagate/libevent/
rm -rf $INSTALL_DIR
mkdir $INSTALL_DIR

./autogen.sh
./configure --prefix=$INSTALL_DIR

make clean
make
#make verify
make install

cd ..
