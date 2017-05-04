#!/bin/sh -xe
# Script to build gtest.
# git repo: https://github.com/google/googletest.git
# tag: release-1.7.0 commit: c99458533a9b4c743ed51537e25989ea55944908

cd googletest

INSTALL_DIR=/usr/local/seagate/googletest/
rm -rf $INSTALL_DIR
mkdir $INSTALL_DIR

rm -rf build/
mkdir build

cd build
CFLAGS=-fPIC CXXFLAGS=-fPIC cmake ..
make

cp -RfP * $INSTALL_DIR/
cp -RfP ../include $INSTALL_DIR/

cd ..
cd ..
