#!/bin/sh

set -e

SCRIPT_PATH=$(readlink -f "$0")
BASEDIR=$(dirname "$SCRIPT_PATH")

VERSION=1.5.0
COMMIT_VER=4c2489361d353db1a1815172a6143c8f5a2d1c40
SHORT_COMMIT_VER=4c248936

cd ~/rpmbuild/SOURCES/
rm -rf s3cmd*

wget -O s3cmd-${VERSION}-${SHORT_COMMIT_VER}.tar.gz https://github.com/s3tools/s3cmd/archive/${COMMIT_VER}/s3cmd-${VERSION}-${SHORT_COMMIT_VER}.tar.gz
cp ${BASEDIR}/s3cmd_max_retries.patch .

cd -

rpmbuild -ba ${BASEDIR}/s3cmd.spec

