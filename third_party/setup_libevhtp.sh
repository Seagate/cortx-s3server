#!/bin/sh -x

# Check if we are building for tests (googletest regex clashes with our regex)
if [[ "$1" = "test" ]]; then
  TESTFLAGS="-DEVHTP_DISABLE_REGEX=ON"
fi

rm -rf libevhtp

git clone -b develop https://github.com/ellzey/libevhtp.git
cd libevhtp
git checkout a89d9b3f9fdf2ebef41893b3d5e4466f4b0ecfda

INSTALL_DIR=`pwd`/s3_dist
mkdir $INSTALL_DIR

CFLAGS='-fPIC -DEVHTP_DISABLE_EVTHR' cmake . -DCMAKE_PREFIX_PATH=`pwd`/../libevent/s3_dist -DCMAKE_INSTALL_PREFIX:PATH=$INSTALL_DIR $TESTFLAGS
make
make install

cd ..
