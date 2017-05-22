#!/bin/sh -xe
# Script to build s3cmd.
# git repo: https://github.com/s3tools/s3cmd.git
# branch: develop commit: 4c2489361d353db1a1815172a6143c8f5a2d1c40

cd s3cmd

# Apply s3cmd patch
patch -f -p1 < ../../patches/s3cmd.patch

INSTALL_DIR=/usr/local/seagate/s3cmd/
rm -rf $INSTALL_DIR
mkdir $INSTALL_DIR

cp s3cmd $INSTALL_DIR
cp -rf S3 $INSTALL_DIR/

cd ..
