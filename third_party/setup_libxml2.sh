#!/bin/sh -x

rm -rf libxml2

git clone -b v2.9.2 https://git.gnome.org/browse/libxml2
cd libxml2

INSTALL_DIR=`pwd`/s3_dist
mkdir $INSTALL_DIR

./autogen.sh
./configure --prefix=$INSTALL_DIR --without-python --without-icu

make
#make verify
make install

cd ..
