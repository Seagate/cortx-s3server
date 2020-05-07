#!/bin/sh

set -e

SCRIPT_PATH=$(readlink -f "$0")
BASEDIR=$(dirname "$SCRIPT_PATH")

VERSION=1.7.0

cd ~/rpmbuild/SOURCES/
rm -rf gmock* googlemock*

git clone -b release-${VERSION} http://gerrit.mero.colo.seagate.com:8080/googlemock gmock-${VERSION}
# gmock working commit version: c440c8fafc6f60301197720617ce64028e09c79d
git clone -b release-${VERSION} http://gerrit.mero.colo.seagate.com:8080/googletest gmock-${VERSION}/gtest
# gmock working commit version: c99458533a9b4c743ed51537e25989ea55944908

tar -zcvf gmock-${VERSION}.tar.gz gmock-${VERSION}
rm -rf gmock-${VERSION} googlemock

cd -

yum-builddep -y ${BASEDIR}/gmock.spec
rpmbuild -ba ${BASEDIR}/gmock.spec
