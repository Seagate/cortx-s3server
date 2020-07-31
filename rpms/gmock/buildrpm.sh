 #!/bin/sh
#
# Copyright (c) 2020 Seagate Technology LLC and/or its Affiliates
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# For any questions about this software or licensing,
# please email opensource@seagate.com or cortx-questions@seagate.com.
#


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
