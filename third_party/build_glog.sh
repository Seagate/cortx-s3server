#!/bin/sh -xe
# Script to build glog.
# git repo: https://github.com/google/glog.git
# tag: v0.3.4 commit: d8cb47f77d1c31779f3ff890e1a5748483778d6a

cd glog

INSTALL_DIR=/usr/local/seagate/glog/
rm -rf $INSTALL_DIR
mkdir $INSTALL_DIR

GFLAGS_HDR_DIR=/usr/local/seagate/gflags/include
GFLAGS_LIB_DIR=/usr/local/seagate/gflags/lib

CXXFLAGS="-fPIC -I$GFLAGS_HDR_DIR" LDFLAGS="-L$GFLAGS_LIB_DIR" ./configure --prefix=$INSTALL_DIR

touch configure.ac aclocal.m4 configure Makefile.am Makefile.in
make clean
make
make install

cd ..
