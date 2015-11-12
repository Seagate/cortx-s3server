#!/bin/sh -x

rm -rf googlemock

git clone -b release-1.7.0 https://github.com/google/googlemock.git
cd googlemock && ln -s ../googletest gtest

mkdir build

cd build
CFLAGS=-fPIC CXXFLAGS=-fPIC cmake ..
make

cd ..
cd ..
