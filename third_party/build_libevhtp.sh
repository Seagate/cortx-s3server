#!/bin/sh -xe
# Script to build libevhtp.
# git repo: https://github.com/ellzey/libevhtp.git
# branch: develop commit: a89d9b3f9fdf2ebef41893b3d5e4466f4b0ecfda

cd libevhtp

# Apply the libevhtp patch
patch -f -p1 < ../../patches/libevhtp.patch

INSTALL_DIR=/usr/local/seagate/libevhtp/
rm -rf $INSTALL_DIR
mkdir $INSTALL_DIR

# Note: googletest regex clashes with our regex. Disable regex by default

CFLAGS='-fPIC -DEVHTP_DISABLE_EVTHR' cmake . -DEVHTP_DISABLE_REGEX=ON -DCMAKE_PREFIX_PATH=/usr/local/seagate/libevent/ -DCMAKE_INSTALL_PREFIX:PATH=$INSTALL_DIR
make clean
make
make install

cd ..
