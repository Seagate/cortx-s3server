#!/bin/sh

set -e

SCRIPT_PATH=$(readlink -f "$0")
BASEDIR=$(dirname "$SCRIPT_PATH")

VERSION=1.7.0

cd ~/rpmbuild/SOURCES/
rm -rf gtest* gooletest*

git clone -b release-${VERSION} https://github.com/Seagate/googletest.git gtest-${VERSION}
tar -zcvf gtest-${VERSION}.tar.gz gtest-${VERSION}
rm -rf gtest-${VERSION} gooletest

cd -

yum-builddep -y ${BASEDIR}/gtest.spec
rpmbuild -ba ${BASEDIR}/gtest.spec
