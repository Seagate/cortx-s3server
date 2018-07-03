#!/bin/sh

set -e

SCRIPT_PATH=$(readlink -f "$0")
BASEDIR=$(dirname "$SCRIPT_PATH")

VERSION=1.6.1
COMMIT_VER=4c2489361d353db1a1815172a6143c8f5a2d1c40
SHORT_COMMIT_VER=4c248936

cd ~/rpmbuild/SOURCES/
rm -rf s3cmd*

git clone -b v${VERSION} http://gerrit-sage.mero.colo.seagate.com:8080/s3cmd s3cmd-${VERSION}-${SHORT_COMMIT_VER}
tar -zcvf s3cmd-${VERSION}-${SHORT_COMMIT_VER}.tar.gz s3cmd-${VERSION}-${SHORT_COMMIT_VER}
rm -rf s3cmd-${VERSION}-${SHORT_COMMIT_VER}

cp ${BASEDIR}/s3cmd_${VERSION}_max_retries.patch .

cd -

rpmbuild -ba ${BASEDIR}/s3cmd.spec

