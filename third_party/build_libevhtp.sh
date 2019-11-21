#!/bin/sh -xe
# Script to build libevhtp.
# git repo: https://github.com/ellzey/libevhtp.git
# gerrit repo http://gerrit.mero.colo.seagate.com:8080/libevhtp
# previous branch: develop commit: a89d9b3f9fdf2ebef41893b3d5e4466f4b0ecfda
# previous patch: libevhtp-v1.2.11-prev.patch
# current branch: develop commit: c84f68d258d07c4015820ceb87fd17decd054bfc

VERSION=$(cat /etc/os-release | grep -w VERSION_ID | cut -d '=' -f 2)

cd libevhtp
if [ "$VERSION" = "\"8.0\"" ]; then
  # Apply the libevhtp rhel patch
  patch -f -p1 < ../../patches/libevhtp-v1.2.11-rhel8.patch
else
  # Apply the libevhtp patch
  patch -f -p1 < ../../patches/libevhtp-v1.2.11.patch
fi

INSTALL_DIR=`pwd`/s3_dist
rm -rf $INSTALL_DIR
mkdir $INSTALL_DIR

# Note: googletest regex clashes with our regex. Disable regex by default

CFLAGS='-fPIC -DEVHTP_DISABLE_EVTHR' cmake . -DEVHTP_DISABLE_REGEX=ON -DCMAKE_PREFIX_PATH=`pwd`/../libevent/s3_dist -DCMAKE_INSTALL_PREFIX:PATH=$INSTALL_DIR
make clean
make
make install

cd ..
