#!/bin/sh -xe
# Script to build libevhtp.
# git repo: https://github.com/ellzey/libevhtp.git
# branch: develop commit: a89d9b3f9fdf2ebef41893b3d5e4466f4b0ecfda

cd libevhtp

# discard previously applied patch
git checkout .
# Apply the libevhtp patch
git apply ../../patches/libevhtp.patch

INSTALL_DIR=`pwd`/s3_dist
rm -rf $INSTALL_DIR
mkdir $INSTALL_DIR

# Note: googletest regex clashes with our regex. Disable regex by default

CFLAGS='-fPIC -DEVHTP_DISABLE_EVTHR' cmake . -DEVHTP_DISABLE_REGEX=ON -DCMAKE_PREFIX_PATH=`pwd`/../libevent/s3_dist -DCMAKE_INSTALL_PREFIX:PATH=$INSTALL_DIR
make clean
make
make install

cd ..
