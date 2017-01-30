#!/bin/sh -x

rm -rf libevhtp

git clone -b develop https://github.com/ellzey/libevhtp.git
cd libevhtp
git checkout a89d9b3f9fdf2ebef41893b3d5e4466f4b0ecfda

git apply ../../patches/libevhtp.patch

INSTALL_DIR=`pwd`/s3_dist
mkdir $INSTALL_DIR

# Note: googletest regex clashes with our regex. Disable regex by default

CFLAGS='-fPIC -DEVHTP_DISABLE_EVTHR' cmake . -DEVHTP_DISABLE_REGEX=ON -DCMAKE_PREFIX_PATH=`pwd`/../libevent/s3_dist -DCMAKE_INSTALL_PREFIX:PATH=$INSTALL_DIR
make
make install

cd ..
