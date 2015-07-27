#!/bin/sh -x

rm -rf libevhtp

git clone -b 1.2.10 https://github.com/ellzey/libevhtp.git

cd libevhtp

cmake . -DCMAKE_PREFIX_PATH=`pwd`/../../libevent/local_dist
make
#make test
