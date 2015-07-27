#!/bin/sh -x

rm -rf libevent

git clone -b release-2.0.22-stable https://github.com/libevent/libevent.git

cd libevent
INSTALL_DIR=/opt/seagate/s3/libevent
mkdir $INSTALL_DIR

./autogen.sh
./configure --prefix=$INSTALL_DIR

make
#make verify
sudo make install
