#!/bin/sh

set -e

SCRIPT_PATH=$(readlink -f "$0")
BASEDIR=$(dirname "$SCRIPT_PATH")

VERSION=1.0.0

cd ~/rpmbuild/SOURCES/
rm -rf s3server*

git clone http://gerrit-sage.dco.colo.seagate.com:8080/s3server s3server-${VERSION}
tar -zcvf s3server-${VERSION}.tar.gz s3server-${VERSION}
rm -rf s3server-${VERSION}

cd -

rpmbuild -ba ${BASEDIR}/s3rpm.spec
