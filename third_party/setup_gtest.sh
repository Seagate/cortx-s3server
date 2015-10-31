#!/bin/sh -x

rm -rf googletest

git clone -b release-1.7.0 https://github.com/google/googletest.git
cd googletest

mkdir build

cd build
CFLAGS=-fPIC CXXFLAGS=-fPIC cmake ..
make

cd ..
cd ..
