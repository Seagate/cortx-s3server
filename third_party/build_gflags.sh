#!/bin/sh -xe
# Script to build gflags.
# git repo: https://github.com/gflags/gflags.git
# tag: v2.1.2 commit: 1a02f2851ee3d48d32d2c8f4d8f390a0bc25565c

cd gflags

INSTALL_DIR=`pwd`/s3_dist
rm -rf build/ $INSTALL_DIR
mkdir $INSTALL_DIR
mkdir build && cd build

CFLAGS=-fPIC CXXFLAGS=-fPIC cmake .. -DCMAKE_INSTALL_PREFIX:PATH=$INSTALL_DIR
make
make install

cd ../..
