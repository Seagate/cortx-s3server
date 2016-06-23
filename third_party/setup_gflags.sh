#!/bin/sh -x

rm -rf gflags

git clone -b v2.1.2 --depth=1 https://github.com/gflags/gflags.git
cd gflags

INSTALL_DIR=`pwd`/s3_dist
mkdir $INSTALL_DIR

mkdir build && cd build

CFLAGS=-fPIC CXXFLAGS=-fPIC cmake .. -DCMAKE_INSTALL_PREFIX:PATH=$INSTALL_DIR
make
make install

cd ../..
