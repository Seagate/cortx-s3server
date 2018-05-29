#!/bin/sh

set -e

SCRIPT_PATH=$(readlink -f "$0")
BASEDIR=$(dirname "$SCRIPT_PATH")

VERSION=3.0

cd ~/rpmbuild/SOURCES/
rm -rf ossperf

mkdir ossperf-${VERSION}

git clone https://github.com/christianbaun/ossperf  ossperf-${VERSION}
tar -zcvf ossperf-${VERSION}.tar.gz ossperf-${VERSION}
rm -rf ossperf-${VERSION}
cp ${BASEDIR}/ossperf.patch .

cd -

rpmbuild -ba ${BASEDIR}/ossperf.spec
