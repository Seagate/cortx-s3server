#!/bin/sh

set -e

BASEDIR=$(dirname $0)
VERSION=1.7.0

cd ~/rpmbuild/SOURCES/
rm -rf gtest* gooletest*

git clone -b release-${VERSION} http://gerrit-sage.dco.colo.seagate.com:8080/googletest gtest-${VERSION}
tar -zcvf gtest-${VERSION}.tar.gz gtest-${VERSION}
rm -rf gtest-${VERSION} gooletest

cd -

rpmbuild -ba ${BASEDIR}/gtest.spec
