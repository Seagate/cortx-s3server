#!/bin/sh -x

# Check if we are building for tests (googletest regex clashes with our regex)
if [[ "$1" = "test" ]]; then
  TESTFLAGS="-DEVHTP_DISABLE_REGEX=ON"
fi

rm -rf libevhtp

git clone -b 1.2.10 https://github.com/ellzey/libevhtp.git
cd libevhtp

INSTALL_DIR=`pwd`/s3_dist
mkdir $INSTALL_DIR

CFLAGS=-fPIC cmake . -DCMAKE_PREFIX_PATH=`pwd`/../libevent/s3_dist -DCMAKE_INSTALL_PREFIX:PATH=$INSTALL_DIR $TESTFLAGS
make
make install

cd ..
