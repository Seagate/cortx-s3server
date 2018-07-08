#!/bin/sh

set -e

SCRIPT_PATH=$(readlink -f "$0")
BASEDIR=$(dirname "$SCRIPT_PATH")

VERSION=1.0.0

cd ~/rpmbuild/SOURCES/
rm -rf s3iamcli*

git clone http://gerrit.mero.colo.seagate.com:8080/s3server s3server-${VERSION}
cd s3server-${VERSION}/auth-utils
mv s3iamcli s3iamcli-${VERSION}
tar -zcvf s3iamcli-${VERSION}.tar.gz s3iamcli-${VERSION}
cp s3iamcli-${VERSION}.tar.gz ~/rpmbuild/SOURCES/
cd ~/rpmbuild/SOURCES/
rm -rf s3server-${VERSION}

rpmbuild -ba ${BASEDIR}/s3iamcli.spec --with python3
