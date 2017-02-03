#!/bin/sh -xe
# Script to build nodejs.
# git repo: https://github.com/nodejs/node.git
# tag: v7.5.0 commit: a34f1d644905b1989bebfb8658220b2a692a1589

cd nodejs

INSTALL_DIR=`pwd`/s3_dist
rm -rf $INSTALL_DIR
mkdir $INSTALL_DIR

./configure --prefix=$INSTALL_DIR
make clean
make -j4
make install

cd ..
