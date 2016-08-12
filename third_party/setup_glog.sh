#!/bin/sh -x

rm -rf glog

git clone -b v0.3.4 --depth=1 https://github.com/google/glog.git
cd glog

INSTALL_DIR=`pwd`/s3_dist
mkdir $INSTALL_DIR

GFLAGS_HDR_DIR=../gflags/s3_dist/include
GFLAGS_LIB_DIR=../gflags/s3_dist/lib

CXXFLAGS="-fPIC -I$GFLAGS_HDR_DIR" LDFLAGS="-L$GFLAGS_LIB_DIR" ./configure --prefix=$INSTALL_DIR

touch configure.ac aclocal.m4 configure Makefile.am Makefile.in
make
make install

cd ..
