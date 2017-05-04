#!/bin/sh -xe
# Script to build gmock.
# git repo: https://github.com/google/googlemock.git
# tag: release-1.7.0 commit: c440c8fafc6f60301197720617ce64028e09c79d

cd googlemock

INSTALL_DIR=/usr/local/seagate/googlemock/
rm -rf $INSTALL_DIR
mkdir $INSTALL_DIR

rm -f gtest
ln -s ../googletest gtest

rm -rf build/
mkdir build

cd build
CFLAGS=-fPIC CXXFLAGS=-fPIC cmake ..
make

cp -RfP * $INSTALL_DIR/
cp -RfP ../include $INSTALL_DIR/

cd ..
cd ..
