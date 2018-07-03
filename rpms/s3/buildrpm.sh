#!/bin/sh

set -e

SCRIPT_PATH=$(readlink -f "$0")
BASEDIR=$(dirname "$SCRIPT_PATH")

VERSION=1.0.0
GIT_VER=git8ee7e61

cd ~/rpmbuild/SOURCES/
rm -rf s3server*

git clone http://gerrit-sage.mero.colo.seagate.com:8080/s3server s3server-${VERSION}-${GIT_VER}
tar -zcvf s3server-${VERSION}-${GIT_VER}.tar.gz s3server-${VERSION}-${GIT_VER}
rm -rf s3server-${VERSION}-${GIT_VER}

cd -

rpmbuild -ba ${BASEDIR}/s3rpm.spec
